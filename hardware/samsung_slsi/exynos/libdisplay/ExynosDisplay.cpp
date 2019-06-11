#include "ExynosDisplay.h"
#include "ExynosHWCUtils.h"
#include <utils/misc.h>

bool winConfigChanged(s3c_fb_win_config *c1, s3c_fb_win_config *c2)
{
    return c1->state != c2->state ||
            c1->fd != c2->fd ||
            c1->x != c2->x ||
            c1->y != c2->y ||
            c1->w != c2->w ||
            c1->h != c2->h ||
            c1->format != c2->format ||
            c1->offset != c2->offset ||
            c1->stride != c2->stride ||
            c1->blending != c2->blending ||
            c1->plane_alpha != c2->plane_alpha;
}

void dumpConfig(s3c_fb_win_config &c)
{
    ALOGV("\tstate = %u", c.state);
    if (c.state == c.S3C_FB_WIN_STATE_BUFFER) {
        ALOGV("\t\tfd = %d, offset = %u, stride = %u, "
                "x = %d, y = %d, w = %u, h = %u, "
#ifdef USES_DRM_SETTING_BY_DECON
                "format = %u, blending = %u, protection = %u",
#else
                "format = %u, blending = %u",
#endif
                c.fd, c.offset, c.stride,
                c.x, c.y, c.w, c.h,
#ifdef USES_DRM_SETTING_BY_DECON
                c.format, c.blending, c.protection);
#else
                c.format, c.blending);
#endif
    }
    else if (c.state == c.S3C_FB_WIN_STATE_COLOR) {
        ALOGV("\t\tcolor = %u", c.color);
    }
}

enum s3c_fb_pixel_format halFormatToS3CFormat(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
        return S3C_FB_PIXEL_FORMAT_RGBA_8888;
    case HAL_PIXEL_FORMAT_RGBX_8888:
        return S3C_FB_PIXEL_FORMAT_RGBX_8888;
    case HAL_PIXEL_FORMAT_RGB_565:
        return S3C_FB_PIXEL_FORMAT_RGB_565;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return S3C_FB_PIXEL_FORMAT_BGRA_8888;
#ifdef EXYNOS_SUPPORT_BGRX_8888
    case HAL_PIXEL_FORMAT_BGRX_8888:
        return S3C_FB_PIXEL_FORMAT_BGRX_8888;
#endif
    default:
        return S3C_FB_PIXEL_FORMAT_MAX;
    }
}

bool isFormatSupported(int format)
{
    return halFormatToS3CFormat(format) < S3C_FB_PIXEL_FORMAT_MAX;
}

enum s3c_fb_blending halBlendingToS3CBlending(int32_t blending)
{
    switch (blending) {
    case HWC_BLENDING_NONE:
        return S3C_FB_BLENDING_NONE;
    case HWC_BLENDING_PREMULT:
        return S3C_FB_BLENDING_PREMULT;
    case HWC_BLENDING_COVERAGE:
        return S3C_FB_BLENDING_COVERAGE;

    default:
        return S3C_FB_BLENDING_MAX;
    }
}

bool isBlendingSupported(int32_t blending)
{
    return halBlendingToS3CBlending(blending) < S3C_FB_BLENDING_MAX;
}

#define NUMA(a) (sizeof(a) / sizeof(a [0]))
const char *s3cFormat2str(uint32_t format)
{
    const static char *unknown = "unknown";

    for (unsigned int n1 = 0; n1 < NUMA(s3cFormat); n1++) {
        if (format == s3cFormat[n1].format) {
            return s3cFormat[n1].desc;
        }
    }

    return unknown;
}

ExynosDisplay::ExynosDisplay(int numGSCs)
    :   mDisplayFd(-1),
        mXres(0),
        mYres(0),
        mXdpi(0),
        mYdpi(0),
        mVsyncPeriod(0),
        mOtfMode(OTF_OFF),
        mHasDrmSurface(false),
        mAllocDevice(NULL),
        mNumMPPs(numGSCs),
        mHwc(NULL)
{
}

ExynosDisplay::~ExynosDisplay()
{
    if (!mLayerInfos.isEmpty()) {
        for (size_t i = 0; i < mLayerInfos.size(); i++) {
            delete mLayerInfos[i];
        }
        mLayerInfos.clear();
    }
}

int ExynosDisplay::getDeconWinMap(int overlayIndex, int totalOverlays)
{
    if (totalOverlays == 4 && overlayIndex == 3)
        return 4;
    return overlayIndex;
}

int ExynosDisplay::inverseWinMap(int windowIndex, int totalOverlays)
{
    if (totalOverlays == 4 && windowIndex == 4)
        return 3;
    return windowIndex;
}

int ExynosDisplay::prepare(hwc_display_contents_1_t *contents)
{
    return 0;
}

int ExynosDisplay::set(hwc_display_contents_1_t *contents)
{
    return 0;
}

void ExynosDisplay::dump(android::String8& result)
{
}

void ExynosDisplay::freeMPP()
{
}

void ExynosDisplay::allocateLayerInfos(hwc_display_contents_1_t* contents)
{
    if (contents == NULL)
        return;

    if (!mLayerInfos.isEmpty()) {
        for (size_t i = 0; i < mLayerInfos.size(); i++) {
            delete mLayerInfos[i];
        }
        mLayerInfos.clear();
    }

    for (int i= 0; i < contents->numHwLayers; i++) {
        ExynosLayerInfo *layerInfo = new ExynosLayerInfo();
        mLayerInfos.push(layerInfo);
    }
}

void ExynosDisplay::dumpLayerInfo(android::String8& result)
{
    if (!mLayerInfos.isEmpty()) {
        result.append(
                "    type    | CheckOverlayFlag | CheckMPPFlag \n"
                "------------+------------------+--------------\n");
        for (size_t i = 0; i < mLayerInfos.size(); i++) {
            unsigned int type = mLayerInfos[i]->compositionType;
            static char const* compositionTypeName[] = {
                "GLES",
                "HWC",
                "BACKGROUND",
                "FB TARGET",
                "UNKNOWN"};

            if (type >= NELEM(compositionTypeName))
                type = NELEM(compositionTypeName) - 1;
            result.appendFormat(
                    " %10s |       0x%8x |   0x%8x \n",
                    compositionTypeName[type],
                    mLayerInfos[i]->mCheckOverlayFlag, mLayerInfos[i]->mCheckMPPFlag);
        }
    }
    result.append("\n");
}
