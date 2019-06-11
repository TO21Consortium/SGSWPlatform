/*
 * Copyright 2012, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "SKIA"
#include <cutils/log.h>
#include <stdlib.h>
#include "SkFimgApi.h"
#include "sec_g2d_comp.h"
#define FLOAT_TO_INT_PRECISION (14)

#include "SkDraw.h"
#include "SkBlitter.h"
#include "SkCanvas.h"
#include "SkColorPriv.h"

enum color_format formatSkiaToDriver[] = {
    SRC_DST_RGB_FORMAT_END, //!< Unkown color type.
    SRC_DST_RGB_FORMAT_END, //!< Mask 8bit is not supported by FIMG2D
    CF_RGB_565,
    SRC_DST_RGB_FORMAT_END, //!< ARGB4444 is not supported by FIMG2D
    CF_ARGB_8888,
    CF_ARGB_8888,
    SRC_DST_RGB_FORMAT_END, //!< kIndex_8_color type is not supported by FIMG2D
    SRC_DST_RGB_FORMAT_END, //!< kIndex_8_color type is not supported by FIMG2D
    CF_ARGB_8888,
    CF_ARGB_8888,
};

enum pixel_order orderSkiaToDriver[] = {
    ARGB_ORDER_END,
    ARGB_ORDER_END,
    PO_RGB,
    ARGB_ORDER_END,
    PO_RGBA,
    PO_BGRA,
    ARGB_ORDER_END,
    ARGB_ORDER_END,
#if SK_PMCOLOR_BYTE_ORDER(B,G,R,A)
    PO_BGRA,
#elif SK_PMCOLOR_BYTE_ORDER(R,G,B,A)
    PO_RGBA,
#endif
#ifdef SK_SUPPORT_LEGACY_N32_NAME
#if SK_PMCOLOR_BYTE_ORDER(B,G,R,A)
    PO_BGRA,
#elif SK_PMCOLOR_BYTE_ORDER(R,G,B,A)
    PO_RGBA,
#endif
#endif
};

enum blit_op blendingSkiaToDriver[] = {
    BLIT_OP_CLR,
    BLIT_OP_SRC,
    BLIT_OP_DST,
    BLIT_OP_SRC_OVER,
};

enum scaling filterSkiaToDriver[] = {
    SCALING_BILINEAR,
    SCALING_BILINEAR,
};

enum repeat repeatSkiaToDriver[] = {
    REPEAT_CLAMP,
    REPEAT_NORMAL,
    REPEAT_MIRROR,
    NO_REPEAT,
};

bool FimgApiCheckPossible(Fimg *fimg)
{
    if (fimg->srcAddr != NULL) {
        switch (fimg->srcColorFormat) {
            case kRGBA_8888_SkColorType:
            case kBGRA_8888_SkColorType:
            case kRGB_565_SkColorType:
                break;
            default:
                return false;
        }
    }

    switch (fimg->dstColorFormat) {
        case kRGBA_8888_SkColorType:
        case kBGRA_8888_SkColorType:
        case kRGB_565_SkColorType:
            break;
        default:
            return false;
    }

    switch (fimg->xfermode) {
        case SkXfermode::kSrcOver_Mode:
        case SkXfermode::kClear_Mode:
        case SkXfermode::kSrc_Mode:
        case SkXfermode::kDst_Mode:
            break;
        default:
            return false;
    }

    if (fimg->matrixType & ~(SkMatrix::kScale_Mask | SkMatrix::kTranslate_Mask))
        return false;

    if (fimg->dstBPP <= 0 || fimg->srcBPP <= 0 || fimg->dstX < 0 || fimg->dstY < 0)
        return false;

    if ((fimg->srcX + fimg->srcW) > FIMGAPI_MAXSIZE || (fimg->srcY + fimg->srcH) > FIMGAPI_MAXSIZE)
        return false;

    if ((fimg->dstX + fimg->dstW) > FIMGAPI_MAXSIZE || (fimg->dstY + fimg->dstH) > FIMGAPI_MAXSIZE)
        return false;

    return true;
}

bool FimgApiIsDstMode(Fimg *fimg)
{
    if (fimg->xfermode == SkXfermode::kDst_Mode)
        return true;
    else
        return false;
}

bool FimgApiClipping(Fimg *fimg)
{
    if ((fimg->clipT < 0) || (fimg->clipB < 0) || (fimg->clipL < 0) || (fimg->clipR < 0)) {
        SkDebugf("Invalid clip value: TBLR = (%d, %d, %d, %d)",fimg->clipT, fimg->clipB, fimg->clipL, fimg->clipR);
        return false;
    }

    if ((fimg->clipT >= fimg->clipB) || (fimg->clipL >= fimg->clipR)) {
        SkDebugf("Invalid clip value: TBLR = (%d, %d, %d, %d)",fimg->clipT, fimg->clipB, fimg->clipL, fimg->clipR);
        return false;
    }
    /* if dst region > clipBound, we have to intersect */
    SkIRect clipBounds;
    SkIRect devRect;

    clipBounds.set(fimg->clipL, fimg->clipT, fimg->clipR, fimg->clipB);
    devRect.set(fimg->dstX, fimg->dstY, fimg->dstX + fimg->dstW, fimg->dstY + fimg->dstH);

    if (clipBounds.contains(devRect)) {
        fimg->dstX = devRect.fLeft;
        fimg->dstY = devRect.fTop;
        fimg->dstW = devRect.width();
        fimg->dstH = devRect.height();
    } else {
        SkIRect rr = devRect;
        if (rr.intersect(clipBounds)) {
            fimg->dstX = rr.fLeft;
            fimg->dstY = rr.fTop;
            fimg->dstW = rr.width();
            fimg->dstH = rr.height();
       } else {
            return false;
       }
    }

    if (fimg->clipL < fimg->dstX)
        fimg->clipL = fimg->dstX;
    if (fimg->clipT < fimg->dstY)
        fimg->clipT = fimg->dstY;
    if (fimg->clipR > (fimg->dstX + fimg->dstW))
        fimg->clipR = fimg->dstX + fimg->dstW;
    if (fimg->clipB > (fimg->dstY + fimg->dstH))
        fimg->clipB = fimg->dstY + fimg->dstH;
    if (fimg->clipR > fimg->dstFWStride / fimg->dstBPP)
        fimg->clipR = fimg->dstFWStride / fimg->dstBPP;
    if (fimg->clipB > fimg->dstFH)
        fimg->clipB = fimg->dstFH;

    return true;
}

bool FimgApiCompromise(Fimg *fimg)
{
    struct compromise_param param;

    /* source format setting*/
    switch (fimg->srcColorFormat) {
        case kRGB_565_SkColorType:
            param.src_fmt = 0;
            break;
        case kRGBA_8888_SkColorType:
        case kBGRA_8888_SkColorType:
            param.src_fmt = 1;
            break;
        case kUnknown_SkColorType:
            param.src_fmt = 2;
            break;
        default :
            return false;
    }
    /* destination format setting */
    switch (fimg->dstColorFormat) {
        case kRGB_565_SkColorType:
            param.dst_fmt = 0;
            break;
        case kRGBA_8888_SkColorType:
        case kBGRA_8888_SkColorType:
            param.dst_fmt = 1;
            break;
        default :
            return false;
    }

    /* scaling setting */
    if (fimg->srcW == fimg->dstW && fimg->srcH == fimg->dstH)
        param.isScaling = 0;
    else if (fimg->srcW * fimg->srcH < fimg->dstW * fimg->dstH)
        param.isScaling = 1;
    else
        param.isScaling = 2;
    /* filter_mode setting */
    param.isFilter = fimg->isFilter;
    /* blending mode setting */
    if (fimg->xfermode == SkXfermode::kSrc_Mode)
        param.isSrcOver = 0;
    else
        param.isSrcOver = 1;

    param.clipW = (fimg->clipR - fimg->clipL) * 1.2;
    param.clipH = (fimg->clipB - fimg->clipT) * 0.8;

    if ((param.clipW * param.clipH) < comp_value[param.src_fmt][param.dst_fmt][param.isScaling][param.isFilter][param.isSrcOver])
        return false;
    return true;
}

bool FimgSupportNegativeCoordinate(Fimg *fimg)
{
    unsigned int dstL, dstR, dstT, dstB;
    int dstFW, dstFH;

    if (fimg->dstBPP <= 0 || fimg->srcBPP <= 0)
        return false;

    if (fimg->dstX < 0) {
        if ((fimg->dstW + fimg->dstX) < (fimg->dstFWStride / fimg->dstBPP))
            dstFW = fimg->dstW + fimg->dstX;
        else
            dstFW = fimg->dstFWStride / fimg->dstBPP;

        dstL = ((unsigned int)(0 - fimg->dstX) << FLOAT_TO_INT_PRECISION) / fimg->dstW;
        dstR = ((unsigned int)(dstFW - fimg->dstX) << FLOAT_TO_INT_PRECISION) / fimg->dstW;
        fimg->srcX = (int)((fimg->srcW * dstL + (1 << (FLOAT_TO_INT_PRECISION - 1))) >> FLOAT_TO_INT_PRECISION) + fimg->srcX;
        fimg->srcW = (int)((fimg->srcW * dstR + (1 << (FLOAT_TO_INT_PRECISION - 1))) >> FLOAT_TO_INT_PRECISION) - fimg->srcX;
        fimg->dstW = dstFW;
        fimg->dstX = 0;
    }

    if (fimg->dstY < 0) {
        if ((fimg->dstH + fimg->dstY) < fimg->dstFH)
            dstFH = fimg->dstH + fimg->dstY;
        else
            dstFH = fimg->dstFH;

        dstT = ((unsigned int)(0 - fimg->dstY) << FLOAT_TO_INT_PRECISION) / fimg->dstH;
        dstB = ((unsigned int)(dstFH - fimg->dstY) << FLOAT_TO_INT_PRECISION) / fimg->dstH;
        fimg->srcY = (int)((fimg->srcH * dstT + (1 << (FLOAT_TO_INT_PRECISION - 1))) >> FLOAT_TO_INT_PRECISION) + fimg->srcY;
        fimg->srcH = (int)((fimg->srcH * dstB + (1 << (FLOAT_TO_INT_PRECISION - 1))) >> FLOAT_TO_INT_PRECISION) - fimg->srcY;
        fimg->dstH = dstFH;
        fimg->dstY = 0;
    }

    return true;
}
#ifdef FIMGAPI_FREQLEVELING_USE
int FimgApiFreqleveling(Fimg *fimg)
{
    static struct timeval prev;
    static int prev_level = -1;
    struct timeval current;
    unsigned int size = (unsigned int)(fimg->clipR - fimg->clipL) *
                    (unsigned int)(fimg->clipB - fimg->clipT);
    int result = MIN_LEVEL;
    unsigned long time = 0;

    gettimeofday(&current, NULL);
    if ((current.tv_sec - prev.tv_sec) * 1000000 + (current.tv_usec - prev.tv_usec) < 20000 && prev_level != -1) {
        if (prev_level > 0)
            prev_level--;
        prev = current;

        return prev_level;
    }

    for (int i=0; i < MIN_LEVEL; i++) {
        if (fimg_standard_size[i] < size)
            result =  i;
    }
    prev = current;
    prev_level = result;

    return result;
}
#endif
int FimgApiStretch(Fimg *fimg, const char *func_name)
{
    static unsigned int seq_no = 100;
    unsigned int i;
    struct fimg2d_blit cmd;
    struct fimg2d_image srcImage;
    struct fimg2d_image dstImage;
    struct fimg2d_param *s_param;
    struct fimg2d_param *d_param;

    /* initialize the command */
    memset(&cmd, 0, sizeof(cmd));
    memset(&srcImage, 0, sizeof(srcImage));
    memset(&dstImage, 0, sizeof(dstImage));

    s_param = &srcImage.param;
    d_param = &dstImage.param;
    cmd.dst = &dstImage;

    /* It does not use at Skia */
    cmd.src[0] = &dstImage;
    cmd.src[1] = &srcImage;
    cmd.src[2] = NULL;

    if (FimgApiCheckPossible(fimg) == false)
        return false;

    if (FimgApiIsDstMode(fimg) == true)
        return FIMGAPI_FINISHED;

#if FIMGAPI_COMPROMISE_USE
    if (FimgApiCompromise(fimg) == false)
        return false;
#endif

#ifdef FIMGAPI_FREQLEVELING_USE
    static int prev_level[4];
    static int level_count = -1;
    static int current_level = 4;
    int level;

    level = FimgApiFreqleveling(fimg);
    if (level_count == -1) {
        for (int i=0; i<4; i++)
            prev_level[i] = 4;
        level_count++;
    }
    prev_level[level_count % 4] = level;
    level_count++;
    fimg->level = prev_level[0];
    for (int i=1; i<4; i++) {
        if (prev_level[i] < fimg->level)
            fimg->level = prev_level[i];
    }
#endif

    cmd.qos_lv = (fimg2d_qos_level)(G2D_LV0 + fimg->level);
    cmd.dither = fimg->isDither;
    srcImage.op = blendingSkiaToDriver[fimg->xfermode];
    s_param->g_alpha = fimg->alpha;
    s_param->premult = PREMULTIPLIED;
    s_param->rotate = (rotation)(ORIGIN + fimg->rotate);
    s_param->solid_color = fimg->fillcolor;

    if (fimg->srcAddr != NULL && (fimg->matrixSw != 1.0f || fimg->matrixSh != 1.0f)) {
        s_param->scaling.mode = filterSkiaToDriver[fimg->isFilter];
        s_param->scaling.src_w = fimg->srcW;
        s_param->scaling.src_h = fimg->srcH;
        s_param->scaling.dst_w = fimg->srcW * fimg->matrixSw;
        s_param->scaling.dst_h = fimg->srcH * fimg->matrixSh;
    } else
        s_param->scaling.mode = NO_SCALING;

    s_param->repeat.mode = repeatSkiaToDriver[fimg->tileModeX];
    s_param->repeat.pad_color = 0x0;

    s_param->clipping.enable = 1;
    s_param->clipping.x1 = fimg->dstX;
    s_param->clipping.y1 = fimg->dstY;
    s_param->clipping.x2 = fimg->dstX + fimg->dstW;
    s_param->clipping.y2 = fimg->dstY + fimg->dstH;

    srcImage.op = blendingSkiaToDriver[fimg->xfermode];
    if (fimg->srcAlphaType == kOpaque_SkAlphaType && fimg->dstAlphaType == kOpaque_SkAlphaType && fimg->alpha == 0xFF)
        srcImage.op = BLIT_OP_SRC;

    if (fimg->srcAddr != NULL) {
        srcImage.layer_num = 1;
        srcImage.addr.type = ADDR_USER;
        srcImage.addr.start = (long unsigned)fimg->srcAddr;
        srcImage.need_cacheopr = true;
        srcImage.width = fimg->srcFWStride / fimg->srcBPP;
        srcImage.height = fimg->srcFH;
        srcImage.stride = fimg->srcFWStride;

        srcImage.order = orderSkiaToDriver[fimg->srcColorFormat];
        srcImage.fmt = formatSkiaToDriver[fimg->srcColorFormat];
        srcImage.rect.x1 = fimg->srcX;
        srcImage.rect.y1 = fimg->srcY;
        srcImage.rect.x2 = fimg->srcX + fimg->srcW;
        srcImage.rect.y2 = fimg->srcY + fimg->srcH;
    } else
        cmd.src[1] = NULL;

    d_param->scaling.mode = NO_SCALING;
    d_param->repeat.mode = NO_REPEAT;
    d_param->repeat.pad_color = 0x0;

    d_param->clipping.enable = 1;
    d_param->clipping.x1 = fimg->clipL;
    d_param->clipping.y1 = fimg->clipT;
    d_param->clipping.x2 = fimg->clipR;
    d_param->clipping.y2 = fimg->clipB;

    dstImage.op = BLIT_OP_SRC;
    dstImage.layer_num = 0;

    if (fimg->dstAddr != NULL) {
        dstImage.layer_num = 0;
        dstImage.addr.type = ADDR_USER;
        dstImage.addr.start = (long unsigned)fimg->dstAddr;
        dstImage.need_cacheopr = true;
        dstImage.width = fimg->dstFWStride / fimg->dstBPP;
        dstImage.height = fimg->dstFH;
        dstImage.stride = fimg->dstFWStride;

        dstImage.order = orderSkiaToDriver[fimg->dstColorFormat];
        dstImage.fmt = formatSkiaToDriver[fimg->dstColorFormat];
        dstImage.rect.x1 = fimg->clipL;
        dstImage.rect.y1 = fimg->clipT;
        dstImage.rect.x2 = fimg->clipR;
        dstImage.rect.y2 = fimg->clipB;
    } else {
        cmd.src[0] = NULL;
        cmd.dst = NULL;
    }

    cmd.seq_no = seq_no++;
    cmd.use_fence = false;

#if defined(FIMGAPI_DEBUG_MESSAGE)
    printDataBlit("Before stretchFimgApi:",
                   fimg->called == 0 ? "drawRect" : "drawBitmap", &cmd);
#endif

    if (stretchFimgApi(&cmd) < 0) {
#if defined(FIMGAPI_DEBUG_MESSAGE)
        ALOGE("%s:stretch failed\n", __FUNCTION__);
#endif
        return false;
    }
    return FIMGAPI_FINISHED;
}

int FimgARGB32_Rect(uint32_t *device, int x, int y, int width, int height,
                    size_t rowbyte, uint32_t color)
{
    Fimg fimg;
    memset(&fimg, 0, sizeof(Fimg));

    fimg.srcAddr        = (unsigned char *)NULL;
    fimg.srcColorFormat = kUnknown_SkColorType;
    fimg.dstColorFormat = kBGRA_8888_SkColorType;

    fimg.fillcolor      = toARGB32(color);

    fimg.dstX           = x;
    fimg.dstY           = y;
    fimg.dstW           = width;
    fimg.dstH           = height;
    fimg.dstFWStride    = rowbyte;
    fimg.dstFH          = y + height;
    fimg.dstBPP         = 4; /* 4Byte */
    fimg.dstAddr        = (unsigned char *)device;

    fimg.clipT          = y;
    fimg.clipB          = y + height;
    fimg.clipL          = x;
    fimg.clipR          = x + width;

    fimg.rotate         = 0;

    fimg.xfermode       = SkXfermode::kSrcOver_Mode;
    fimg.isDither       = false;
    fimg.matrixType     = 0;
    fimg.alpha          = 0xFF;
    fimg.level          = 2;

    return FimgApiStretch(&fimg, __func__);
}

int FimgRGB16_Rect(uint32_t *device, int x, int y, int width, int height,
                    size_t rowbyte, uint32_t color)
{
    Fimg fimg;
    memset(&fimg, 0, sizeof(Fimg));

    fimg.srcAddr        = (unsigned char *)NULL;
    fimg.srcColorFormat = kUnknown_SkColorType;
    fimg.dstColorFormat = kBGRA_8888_SkColorType;
    fimg.fillcolor      = toARGB32(color);

    fimg.dstX           = x;
    fimg.dstY           = y;
    fimg.dstW           = width;
    fimg.dstH           = height;
    fimg.dstFWStride    = rowbyte;
    fimg.dstFH          = y + height;
    fimg.dstBPP         = 2;
    fimg.dstAddr        = (unsigned char *)device;

    fimg.clipT          = y;
    fimg.clipB          = y + height;
    fimg.clipL          = x;
    fimg.clipR          = x + width;

    fimg.rotate         = 0;

    fimg.xfermode       = SkXfermode::kSrcOver_Mode;
    fimg.isDither       = false;
    fimg.matrixType     = 0;
    fimg.alpha          = 0xFF;
    fimg.level          = 2;

    return FimgApiStretch(&fimg, __func__);
}
uint32_t toARGB32(uint32_t color)
{
    U8CPU a = SkGetPackedA32(color);
    U8CPU r = SkGetPackedR32(color);
    U8CPU g = SkGetPackedG32(color);
    U8CPU b = SkGetPackedB32(color);

    return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}
