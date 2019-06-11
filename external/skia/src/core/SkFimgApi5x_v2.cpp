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
#define FLOAT_TO_INT_PRECISION (14)

#include "SkFimgApi.h"
#include "SkDraw.h"
#include "SkBlitter.h"
#include "SkCanvas.h"
#include "SkColorPriv.h"

#include <linux/m2m1shot2.h>

enum format_index {
    FM_RGB565,
    FM_ARGB32,
    FM_ABGR32,
    FM_FORMAT_END,
};

int formatSkiaToDriver[] = {
    FM_FORMAT_END, /* unknown */
    FM_FORMAT_END, /* alpha_8 */
    FM_RGB565, /* rgb_565 */
    FM_FORMAT_END, /* argb_4444 */
    FM_ABGR32, /* rgba_8888 */
    FM_ARGB32, /* bgra_8888 */
    FM_FORMAT_END, /* index_8 */
};

bool FimgApiCheckPossible(Fimg *fimg)
{
    if (fimg->srcColorFormat < 0 || fimg->srcColorFormat >= FM_FORMAT_END)
        return false;

    if (fimg->dstColorFormat < 0 || fimg->dstColorFormat >= FM_FORMAT_END)
        return false;

    if ((fimg->xfermode != SkXfermode::kSrc_Mode) && (fimg->xfermode != SkXfermode::kSrcOver_Mode))
        return false;

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

int FimgApiStretch(Fimg *fimg, const char *func_name)
{
    fimg->dstColorFormat = formatSkiaToDriver[fimg->dstColorFormat];
    fimg->srcColorFormat = formatSkiaToDriver[fimg->srcColorFormat];

    if (FimgApiCheckPossible(fimg) == false)
        return false;

    if (FimgApiIsDstMode(fimg) == true)
        return FIMGAPI_FINISHED;

    if (requestFimgApi_v5(fimg) < 0) {
        return false;
    }
    return FIMGAPI_FINISHED;
}
