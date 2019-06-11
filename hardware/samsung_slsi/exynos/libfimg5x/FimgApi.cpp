/*
**
** Copyright 2009 Samsung Electronics Co, Ltd.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
**
*/

#define LOG_NDEBUG 0
#define LOG_TAG "SKIA"
#include <utils/Log.h>

#include "FimgApi.h"

#ifdef FIMG2D_USE_M2M1SHOT2
#include "videodev2.h"
#include <linux/m2m1shot2.h>

/* include freq leveling and compromise table */
#include "sec_g2d_comp.h"
unsigned int fmt_m2m1shot1_format[] = {
    V4L2_PIX_FMT_RGB565,
    V4L2_PIX_FMT_ARGB32,
    V4L2_PIX_FMT_ABGR32,
};

unsigned int xfer_m2m1shot2_format[] = {
    M2M1SHOT2_BLEND_CLEAR,
    M2M1SHOT2_BLEND_SRC,
    M2M1SHOT2_BLEND_DST,
    M2M1SHOT2_BLEND_SRCOVER,
};

unsigned int filter_m2m1shot2_format[] = {
    M2M1SHOT2_SCFILTER_BILINEAR,
    M2M1SHOT2_SCFILTER_BILINEAR,
};

unsigned int repeat_m2m1shot2_format[] = {
    M2M1SHOT2_REPEAT_CLAMP,
    M2M1SHOT2_REPEAT_REPEAT,
    M2M1SHOT2_REPEAT_REFLECT,
    M2M1SHOT2_REPEAT_NONE,
};
#endif

pthread_mutex_t s_g2d_lock = PTHREAD_MUTEX_INITIALIZER;

struct blitinfo_table optbl[] = {
    { (int)BLIT_OP_NONE, "NONE" },
    { (int)BLIT_OP_SOLID_FILL, "FILL" },
    { (int)BLIT_OP_CLR, "CLR" },
    { (int)BLIT_OP_SRC, "SRC" },
    { (int)BLIT_OP_DST, "DST" },
    { (int)BLIT_OP_SRC_OVER, "SRC_OVER" },
    { (int)BLIT_OP_DST_OVER, "DST_OVER" },
    { (int)BLIT_OP_SRC_IN, "SRC_IN" },
    { (int)BLIT_OP_DST_IN, "DST_IN" },
    { (int)BLIT_OP_SRC_OUT, "SRC_OUT" },
    { (int)BLIT_OP_DST_OUT, "DST_OUT" },
    { (int)BLIT_OP_SRC_ATOP, "SRC_ATOP" },
    { (int)BLIT_OP_DST_ATOP, "DST_ATOP" },
    { (int)BLIT_OP_XOR, "XOR" },
    { (int)BLIT_OP_ADD, "ADD" },
    { (int)BLIT_OP_MULTIPLY, "MULTIPLY" },
    { (int)BLIT_OP_SCREEN, "SCREEN" },
    { (int)BLIT_OP_DARKEN, "DARKEN" },
    { (int)BLIT_OP_LIGHTEN, "LIGHTEN" },
    { (int)BLIT_OP_DISJ_SRC_OVER, "DISJ_SRC_OVER" },
    { (int)BLIT_OP_DISJ_DST_OVER, "DISJ_DST_OVER" },
    { (int)BLIT_OP_DISJ_SRC_IN, "DISJ_SRC_IN" },
    { (int)BLIT_OP_DISJ_DST_IN, "DISJ_DST_IN" },
    { (int)BLIT_OP_DISJ_SRC_OUT, "DISJ_SRC_OUT" },
    { (int)BLIT_OP_DISJ_DST_OUT, "DISJ_DST_OUT" },
    { (int)BLIT_OP_DISJ_SRC_ATOP, "DISJ_SRC_ATOP" },
    { (int)BLIT_OP_DISJ_DST_ATOP, "DISJ_DST_ATOP" },
    { (int)BLIT_OP_DISJ_XOR, "DISJ_XOR" },
    { (int)BLIT_OP_CONJ_SRC_OVER, "CONJ_SRC_OVER" },
    { (int)BLIT_OP_CONJ_DST_OVER, "CONJ_DST_OVER" },
    { (int)BLIT_OP_CONJ_SRC_IN, "CONJ_SRC_IN" },
    { (int)BLIT_OP_CONJ_DST_IN, "CONJ_DST_IN" },
    { (int)BLIT_OP_CONJ_SRC_OUT, "CONJ_SRC_OUT" },
    { (int)BLIT_OP_CONJ_DST_OUT, "CONJ_DST_OUT" },
    { (int)BLIT_OP_CONJ_SRC_ATOP, "CONJ_SRC_ATOP" },
    { (int)BLIT_OP_CONJ_DST_ATOP, "CONJ_DST_ATOP" },
    { (int)BLIT_OP_CONJ_XOR, "CONJ_XOR" },
    { (int)BLIT_OP_USER_COEFF, "USER_COEFF" },
    { (int)BLIT_OP_END, "" },
};

struct blitinfo_table repeat_tbl[] = {
   { (int)NO_REPEAT, "NON" },
   { (int)REPEAT_NORMAL, "DEFAULT" },
   { (int)REPEAT_PAD, "PAD" },
   { (int)REPEAT_REFLECT, "REFLECT, MIRROR" },
   { (int)REPEAT_CLAMP, "CLAMP" },
};

#ifndef REAL_DEBUG
    void VOID_FUNC(__attribute__((__unused__)) const char *format, ...)
    {}
#endif

FimgApi::FimgApi()
{
    m_flagCreate = false;
}

FimgApi::~FimgApi()
{
    if (m_flagCreate == true)
        PRINT("%s::this is not Destroyed fail\n", __func__);
}

bool FimgApi::Create(void)
{
    bool ret = false;

    if (t_Lock() == false) {
        PRINT("%s::t_Lock() fail\n", __func__);
        goto CREATE_DONE;
    }

    if (m_flagCreate == true) {
        PRINT("%s::Already Created fail\n", __func__);
        goto CREATE_DONE;
    }

    if (t_Create() == false) {
        PRINT("%s::t_Create() fail\n", __func__);
        goto CREATE_DONE;
    }

    m_flagCreate = true;

    ret = true;

CREATE_DONE :

    t_UnLock();

    return ret;
}

bool FimgApi::Destroy(void)
{
    bool ret = false;

    if (t_Lock() == false) {
        PRINT("%s::t_Lock() fail\n", __func__);
        goto DESTROY_DONE;
    }

    if (m_flagCreate == false) {
        PRINT("%s::Already Destroyed fail\n", __func__);
        goto DESTROY_DONE;
    }

    if (t_Destroy() == false) {
        PRINT("%s::t_Destroy() fail\n", __func__);
        goto DESTROY_DONE;
    }

    m_flagCreate = false;

    ret = true;

DESTROY_DONE :

    t_UnLock();

    return ret;
}
#ifdef FIMG2D_USE_M2M1SHOT2
bool FimgApi::Stretch_v5(struct m2m1shot2 *cmd)
{
    bool ret = false;

    if (t_Lock() == false) {
        PRINT("%s::t_Lock() fail\n", __func__);
        goto STRETCH_DONE;
    }

    if (m_flagCreate == false) {
        PRINT("%s::This is not Created fail\n", __func__);
        goto STRETCH_DONE;
    }

    if (t_Stretch_v5(cmd) == false) {
        goto STRETCH_DONE;
    }

    ret = true;

STRETCH_DONE :

    t_UnLock();

    return ret;
}
#endif
bool FimgApi::Stretch(struct fimg2d_blit *cmd)
{
    bool ret = false;

    if (t_Lock() == false) {
        PRINT("%s::t_Lock() fail\n", __func__);
        goto STRETCH_DONE;
    }

    if (m_flagCreate == false) {
        PRINT("%s::This is not Created fail\n", __func__);
        goto STRETCH_DONE;
    }

    if (t_Stretch(cmd) == false) {
        goto STRETCH_DONE;
    }

    ret = true;

STRETCH_DONE :

    t_UnLock();

    return ret;
}

bool FimgApi::Sync(void)
{
    bool ret = false;

    if (m_flagCreate == false) {
        PRINT("%s::This is not Created fail\n", __func__);
        goto SYNC_DONE;
    }

    if (t_Sync() == false)
        goto SYNC_DONE;

    ret = true;

SYNC_DONE :

    return ret;
}

bool FimgApi::t_Create(void)
{
    PRINT("%s::This is empty virtual function fail\n", __func__);
    return false;
}

bool FimgApi::t_Destroy(void)
{
    PRINT("%s::This is empty virtual function fail\n", __func__);
    return false;
}

bool FimgApi::t_Stretch(__attribute__((__unused__)) struct fimg2d_blit *cmd)
{
    PRINT("%s::This is empty virtual function fail\n", __func__);
    return false;
}
#ifdef FIMG2D_USE_M2M1SHOT2
bool FimgApi::t_Stretch_v5(__attribute__((__unused__)) struct m2m1shot2 *cmd)
{
    PRINT("%s::This is empty virtual function fail\n", __func__);
    return false;
}
#endif
bool FimgApi::t_Sync(void)
{
    PRINT("%s::This is empty virtual function fail\n", __func__);
    return false;
}

bool FimgApi::t_Lock(void)
{
    PRINT("%s::This is empty virtual function fail\n", __func__);
    return false;
}

bool FimgApi::t_UnLock(void)
{
    PRINT("%s::This is empty virtual function fail\n", __func__);
    return false;
}

//---------------------------------------------------------------------------//
// extern function
//---------------------------------------------------------------------------//
extern "C" int stretchFimgApi(struct fimg2d_blit *cmd)
{
    pthread_mutex_lock(&s_g2d_lock);

    FimgApi * fimgApi = createFimgApi();

    if (fimgApi == NULL) {
        PRINT("%s::createFimgApi() fail\n", __func__);
        pthread_mutex_unlock(&s_g2d_lock);
        return -1;
    }

    if (fimgApi->Stretch(cmd) == false) {
        if (fimgApi != NULL)
            destroyFimgApi(fimgApi);

        pthread_mutex_unlock(&s_g2d_lock);
        return -1;
    }

    if (fimgApi != NULL)
        destroyFimgApi(fimgApi);

    pthread_mutex_unlock(&s_g2d_lock);
    return 0;
}

extern "C" int SyncFimgApi(void)
{
    pthread_mutex_lock(&s_g2d_lock);
    FimgApi * fimgApi = createFimgApi();
    if (fimgApi == NULL) {
        PRINT("%s::createFimgApi() fail\n", __func__);
        pthread_mutex_unlock(&s_g2d_lock);
        return -1;
    }

    if (fimgApi->Sync() == false) {
        if (fimgApi != NULL)
            destroyFimgApi(fimgApi);

        pthread_mutex_unlock(&s_g2d_lock);
        return -1;
    }

    if (fimgApi != NULL)
        destroyFimgApi(fimgApi);

    pthread_mutex_unlock(&s_g2d_lock);
    return 0;
}

#ifdef FIMG2D_USE_M2M1SHOT2
extern "C" int stretchFimgApi_v5(struct m2m1shot2 *cmd)
{
    pthread_mutex_lock(&s_g2d_lock);

    FimgApi * fimgApi = createFimgApi();

    if (fimgApi == NULL) {
        PRINT("%s::createFimgApi() fail\n", __func__);
        pthread_mutex_unlock(&s_g2d_lock);
        return -1;
    }

    if (fimgApi->Stretch_v5(cmd) == false) {
        if (fimgApi != NULL)
            destroyFimgApi(fimgApi);

        pthread_mutex_unlock(&s_g2d_lock);
        return -1;
    }

    if (fimgApi != NULL)
        destroyFimgApi(fimgApi);

    pthread_mutex_unlock(&s_g2d_lock);
    return 0;
}

#ifdef FIMGAPI_HAL_FREQLEVELING
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
    if ((current.tv_sec - prev.tv_sec) * 1000000 +
        (current.tv_usec - prev.tv_usec) < 20000 && prev_level != -1) {
        if (prev_level > 0)
            prev_level--;
        prev = current;

        return prev_level;
    }

    for (int i = 0; i < MIN_LEVEL; i++) {
        if (fimg_standard_size[i] < size)
            result =  i;
    }
    prev = current;
    prev_level = result;

#ifdef FIMGAPI_HAL_DEBUG
    ALOGE("freq leveling : %d", result);
#endif
    return result;
}
#endif

#ifdef FIMGAPI_HAL_COMPROMISE
bool FimgApiCompromise(Fimg *fimg)
{
    struct compromise_param param;

    /* source format setting*/
    param.src_fmt = (fimg->srcColorFormat == 0)? 0 : 1;
    param.dst_fmt = (fimg->dstColorFormat == 0)? 0 : 1;

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
    if (fimg->xfermode == 1)
        param.isSrcOver = 0;
    else if (fimg->xfermode == 3)
        param.isSrcOver = 1;
    else
        return false;

    param.clipW = (fimg->clipR - fimg->clipL) * 1.2;
    param.clipH = (fimg->clipB - fimg->clipT) * 0.8;
#ifdef FIMGAPI_HAL_DEBUG
    ALOGE("compromise [%d %d %d %d %d] [comp %d and %d]", param.src_fmt, param.dst_fmt,
        param.isScaling, param.isFilter, param.isSrcOver, param.clipW * param.clipH,
        comp_value[param.src_fmt][param.dst_fmt][param.isScaling][param.isFilter][param.isSrcOver]);
#endif

    if ((param.clipW * param.clipH) < comp_value[param.src_fmt][param.dst_fmt][param.isScaling][param.isFilter][param.isSrcOver])
        return false;
    return true;
}
#endif

void printDataBlitImage_v5(struct m2m1shot2_image* image)
{
    /* image feature */
    SLOGI("flags : %u", image->flags);
    SLOGI("memory : %u", image->memory);
    /* image.buffer : address and buffer size */

    struct m2m1shot2_buffer &buffer = image->plane[0];
    SLOGI("address %x", buffer.userptr);
    SLOGI("length %u", buffer.length);

    /* image.format : color format and coordinate */
    struct m2m1shot2_format &image_fmt = image->fmt;
    SLOGI("width : %u", image_fmt.width);
    SLOGI("height: %u", image_fmt.height);
    SLOGI("format: %u", image_fmt.pixelformat);

    /* image.format : color format and coordinate */
    SLOGI("crop : %d, %d (%u x %u)", image_fmt.crop.left,
        image_fmt.crop.top, image_fmt.crop.width, image_fmt.crop.height);
    SLOGI("widnow : %d, %d (%u x %u)", image_fmt.window.left,
        image_fmt.window.top, image_fmt.window.width, image_fmt.window.height);
    /* image.extra : parameter (only source image) */
    struct m2m1shot2_extra &extra = image->ext;
    SLOGI("scaler_filter : %u", extra.scaler_filter);
    SLOGI("composite : %u",extra.composit_mode);
    SLOGI("g_alpha : %u", extra.galpha);
    SLOGI("repeat : %u", extra.xrepeat);
}

void printDataBlit_v5(const char *title, int called, struct m2m1shot2 cmd)
{
    SLOGI("%s (from %d)\n", title, called);
    SLOGI("cmd flag %x", cmd.flags);
    SLOGI("- - - - - - - destination - - - - - -");
    printDataBlitImage_v5(&cmd.target);
    SLOGI("- - - - - - - destination (source[0]-");
    printDataBlitImage_v5(&cmd.sources[0]);
    SLOGI("- - - - - - - source - - - - - - - - ");
    printDataBlitImage_v5(&cmd.sources[1]);
}

void copy_m2m1shot2_image(struct m2m1shot2_image *dst, struct m2m1shot2_image *src)
{
    /* initialize the image */
    memset(dst, 0, sizeof(struct m2m1shot2_image));

    /* image feature */
    dst->flags = src->flags;
    dst->fence = src->fence;
    dst->memory = src->memory;
    dst->num_planes = src->num_planes;

    /* image.buffer : address and buffer size */
    struct m2m1shot2_buffer &d_buffer = dst->plane[0];
    struct m2m1shot2_buffer &s_buffer = src->plane[0];
    d_buffer.userptr = s_buffer.userptr;
    d_buffer.length = s_buffer.length;
    d_buffer.offset = s_buffer.offset;

    /* image.format : color format and coordinate */
    struct m2m1shot2_format &d_format = dst->fmt;
    struct m2m1shot2_format &s_format = src->fmt;
    d_format.width = s_format.width;
    d_format.height = s_format.height;
    d_format.pixelformat = s_format.pixelformat;

    d_format.crop.left = s_format.crop.left;
    d_format.crop.top = s_format.crop.top;
    d_format.crop.width = s_format.crop.width;
    d_format.crop.height = s_format.crop.height;

    d_format.window.left = s_format.window.left;
    d_format.window.top = s_format.window.top;
    d_format.window.width = s_format.window.width;
    d_format.window.height = s_format.window.height;

    /* image.extra : parameter (only source image) */
    struct m2m1shot2_extra &d_extra = dst->ext;
    struct m2m1shot2_extra &s_extra = src->ext;
    d_extra.scaler_filter = s_extra.scaler_filter;
    d_extra.fillcolor = s_extra.fillcolor;
    d_extra.transform = s_extra.transform;
    d_extra.composit_mode = s_extra.composit_mode;
    d_extra.galpha_red = d_extra.galpha_green =
        d_extra.galpha = d_extra.galpha_blue = s_extra.galpha;
    d_extra.xrepeat = s_extra.xrepeat;
    d_extra.yrepeat = s_extra.yrepeat;
}

unsigned int scale_factor_to_fixed(float m) {
    unsigned int value = m * (1 << 16);
    return value & 0xFFFFFFFF;
}

int requestFimgApi_v5(Fimg *fimg)
{
    struct m2m1shot2 cmd;

#ifdef FIMGAPI_HAL_COMPROMISE
    if (FimgApiCompromise(fimg) == false)
        return -1;
#endif
#ifdef FIMGAPI_G2D_NEAREST_UNSUPPORT
    if (fimg->isFilter == false)
        return -1;
#endif
    /* initialize the command */
    memset(&cmd, 0, sizeof(cmd));
    cmd.sources = new m2m1shot2_image[3];

    /* m2m1shot2 */
    struct m2m1shot2_image &srcImage = cmd.sources[1];
    struct m2m1shot2_image &dstImage = cmd.sources[0];
    cmd.num_sources = 2;
    cmd.flags = (fimg->isDither) & 0x1; /* dither | nonblock | error response */

    /* initialize the image */
    memset(&srcImage, 0, sizeof(srcImage));
    memset(&dstImage, 0, sizeof(dstImage));

    /* image feature */
    srcImage.flags = M2M1SHOT2_IMGFLAG_PREMUL_ALPHA | M2M1SHOT2_IMGFLAG_GLOBAL_ALPHA;
    srcImage.fence = 0; /* no use */
    srcImage.memory = M2M1SHOT2_BUFTYPE_USERPTR;
    srcImage.num_planes = 1;

    /* image.buffer : address and buffer size */
    struct m2m1shot2_buffer &s_buffer = srcImage.plane[0];
    s_buffer.userptr = (unsigned long) fimg->srcAddr;
    s_buffer.length = s_buffer.payload = fimg->srcFH * fimg->srcFWStride;
    s_buffer.offset = 0;

    /* image.format : color format and coordinate */
    struct m2m1shot2_format &s_format = srcImage.fmt;
    s_format.width = fimg->srcFWStride / fimg->srcBPP;
    s_format.height = fimg->srcFH;
    s_format.pixelformat = fmt_m2m1shot1_format[fimg->srcColorFormat];

    s_format.crop.left = fimg->srcX;
    s_format.crop.top = fimg->srcY;
    s_format.crop.width = fimg->srcW;
    s_format.crop.height = fimg->srcH;

    s_format.window.left = fimg->dstX;
    s_format.window.top = fimg->dstY;
    s_format.window.width = fimg->dstW;
    s_format.window.height = fimg->dstH;

    /* image.extra : parameter (only source image) */
    struct m2m1shot2_extra &s_extra = srcImage.ext;
    if (fimg->matrixSw != 1.0f || fimg->matrixSh != 1.0f) {
        /* caculate factor and enable factor */
        /* or not, draw from source size to destination size */
        s_extra.scaler_filter = M2M1SHOT2_SCFILTER_BILINEAR;
        /* set the scaling ratio */
        float Sw = 1.0f / fimg->matrixSw;
        float Sh = 1.0f / fimg->matrixSh;
        s_extra.horizontal_factor = scale_factor_to_fixed(Sw);
        s_extra.vertical_factor = scale_factor_to_fixed(Sh);
        srcImage.flags |= M2M1SHOT2_IMGFLAG_XSCALE_FACTOR;
        srcImage.flags |= M2M1SHOT2_IMGFLAG_YSCALE_FACTOR;
    } else {
        s_extra.scaler_filter = M2M1SHOT2_SCFILTER_NONE;
        cmd.flags |= M2M1SHOT2_IMGFLAG_NO_RESCALING;
    }

    s_extra.fillcolor = fimg->fillcolor;
    s_extra.transform = 0;
    s_extra.composit_mode = xfer_m2m1shot2_format[fimg->xfermode];
    s_extra.galpha_red = s_extra.galpha_green =
	    s_extra.galpha_blue = s_extra.galpha = fimg->alpha;
    s_extra.xrepeat = s_extra.yrepeat = repeat_m2m1shot2_format[fimg->tileModeX];

    /* image feature */
    dstImage.flags = M2M1SHOT2_IMGFLAG_PREMUL_ALPHA;
    dstImage.fence = 0;
    dstImage.memory = M2M1SHOT2_BUFTYPE_USERPTR;
    dstImage.num_planes = 1;

    /* image.buffer : address and buffer size */
    struct m2m1shot2_buffer &d_buffer = dstImage.plane[0];
    d_buffer.userptr = (unsigned long) fimg->dstAddr;
    d_buffer.length = d_buffer.payload = fimg->dstFH * fimg->dstFWStride;
    d_buffer.offset = 0;

    /* image.format : color format and coordinate */
    struct m2m1shot2_format &d_format = dstImage.fmt;
    d_format.width = fimg->dstFWStride / fimg->dstBPP;
    d_format.height = fimg->dstFH;
    d_format.pixelformat = fmt_m2m1shot1_format[fimg->dstColorFormat];

    d_format.crop.left = d_format.window.left = fimg->clipL;
    d_format.crop.top = d_format.window.top = fimg->clipT;
    d_format.crop.width = d_format.window.width = fimg->clipR - fimg->clipL;
    d_format.crop.height = d_format.window.height = fimg->clipB - fimg->clipT;

    /* image.extra : parameter (only source image) */
    struct m2m1shot2_extra &d_extra = dstImage.ext;
    d_extra.scaler_filter = M2M1SHOT2_SCFILTER_NONE;
    d_extra.fillcolor = fimg->fillcolor;
    d_extra.transform = 0;
    d_extra.composit_mode = M2M1SHOT2_BLEND_NONE;
    d_extra.galpha_red = d_extra.galpha_green =
        d_extra.galpha = d_extra.galpha_blue = 0x0;
    d_extra.xrepeat = d_extra.yrepeat = M2M1SHOT2_REPEAT_NONE;

    copy_m2m1shot2_image(&cmd.target, &dstImage);
    /* src img[0] need to set xfermode::src for drawing dst */
    d_extra.composit_mode = M2M1SHOT2_BLEND_SRC;

    /* freq supports 4 level as frequency and size*/
#ifdef FIMGAPI_HAL_FREQLEVELING
    static int prev_level[4];
    static int level_count = -1;
    static int current_level = 4;
    int level;

    level = FimgApiFreqleveling(fimg);
    if (level_count == -1) {
        for (int i = 0; i < 4; i++)
            prev_level[i] = 4;
        level_count++;
    }
    prev_level[level_count % 4] = level;
    level_count++;
    fimg->level = prev_level[0];
    for (int i = 1; i < 4; i++) {
        if (prev_level[i] < fimg->level)
            fimg->level = prev_level[i];
    }
#endif

#ifdef FIMGAPI_HAL_DEBUG
    printDataBlit_v5(__func__, fimg->called, cmd);
#endif
    /* if it does not success, need to debug */
    if (stretchFimgApi_v5(&cmd) < 0) {
        ALOGE("%s : g2d hardware operation failed", __func__);
        return -1;
    }

    return 0;
}
#endif
void printDataBlit(char *title, const char *called, struct fimg2d_blit *cmd)
{
    struct fimg2d_image *srcImage;
    struct fimg2d_image *dstImage;
    struct fimg2d_param *srcParam;
    srcImage = cmd->src[1];
    dstImage = cmd->dst;
    srcParam = &srcImage->param;

    SLOGI("%s (From %s)\n", title, called);
    SLOGI("    sequence_no. = %u\n", cmd->seq_no);
    SLOGI("    blit_op      = %d(%s)\n", srcImage->op, optbl[srcImage->op].str);
    SLOGI("    fill_color   = %X\n", srcParam->solid_color);
    SLOGI("    global_alpha = %u\n", (unsigned int)srcParam->g_alpha);
    SLOGI("    PREMULT      = %s\n", srcParam->premult == PREMULTIPLIED ? "PREMULTIPLIED" : "NON-PREMULTIPLIED");
    SLOGI("    do_dither    = %s\n", cmd->dither == true ? "dither" : "no-dither");
    SLOGI("    repeat       = %d(%s)\n", srcParam->repeat.mode,
              repeat_tbl[srcParam->repeat.mode].str);

    printDataBlitRotate(srcParam->rotate);

    printDataBlitScale(&srcParam->scaling);

    printDataBlitImage("SRC", srcImage);
    printDataBlitImage("DST", dstImage);

    if (srcImage != NULL)
        printDataBlitRect("SRC", &srcImage->rect);
    if (srcParam != NULL)
        printDataBlitRect("SRC_CLIP(same as DST)", &srcParam->clipping);
    if (dstImage != NULL)
        printDataBlitRect("DST_CLIP", &dstImage->rect);
}

void printDataBlitImage(const char *title, struct fimg2d_image *image)
{
    if (NULL != image) {
        SLOGI("    Image_%s\n", title);
        SLOGI("        addr = %lx\n", image->addr.start);
        SLOGI("        format = %s\n", image->fmt == 0 ? "ARGB_8888" : "RGB_565");
        SLOGI("        size = (%d, %d)\n", image->width, image->height);
    } else {
        SLOGI("    Image_%s : NULL\n", title);
    }
}

void printDataBlitRect(const char *title, struct fimg2d_clip *clipping)
{
    if (NULL != clipping && clipping->enable == 1) {
        SLOGI("    RECT_%s\n", title);
        SLOGI("        (x1, y1) = (%d, %d)\n", clipping->x1, clipping->y1);
        SLOGI("        (x2, y2) = (%d, %d)\n", clipping->x2, clipping->y2);
        SLOGI("        (width, height) = (%d, %d)\n", clipping->x2 - clipping->x1, clipping->y2 - clipping->y1);
    } else {
        SLOGI("    RECT_%s : NULL\n", title);
    }
}

void printDataBlitRect(const char *title, struct fimg2d_rect *rect)
{
    if (NULL != rect) {
        SLOGI("    RECT_%s\n", title);
        SLOGI("        (x1, y1) = (%d, %d)\n", rect->x1, rect->y1);
        SLOGI("        (x2, y2) = (%d, %d)\n", rect->x2, rect->y2);
        SLOGI("        (width, height) = (%d, %d)\n", rect->x2 - rect->x1, rect->y2 - rect->y1);
    } else {
        SLOGI("    RECT_%s : NULL\n", title);
    }
}

void printDataBlitRotate(int rotate)
{
    SLOGI("    ROTATE : %d\n", rotate);
}

void printDataBlitScale(struct fimg2d_scale *scaling)
{
    SLOGI("    SCALING\n");
    if (scaling->mode != 0) {
        SLOGI("        scale_mode : %s\n",
                  (scaling->mode == 1 ? "SCALING_BILINEAR" : "NO_SCALING"));
        SLOGI("        src : (src_w, src_h) = (%d, %d)\n", scaling->src_w, scaling->src_h);
        SLOGI("        dst : (dst_w, dst_h) = (%d, %d)\n", scaling->dst_w, scaling->dst_h);
        SLOGI("        scaling_factor : (scale_w, scale_y) = (%3.2f, %3.2f)\n", (double)scaling->dst_w / scaling->src_w, (double)scaling->dst_h / scaling->src_h);
    } else {
        SLOGI("        scale_mode : NO_SCALING\n");
    }
}
