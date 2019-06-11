#define ATRACE_TAG ATRACE_TAG_GRAPHICS

//#define LOG_NDEBUG 0
#define LOG_TAG "display"
#include "ExynosDisplay.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"
#include "ExynosHWCDebug.h"
#include <utils/misc.h>
#include <utils/Trace.h>
#include <inttypes.h>
#if defined(USES_DUAL_DISPLAY)
#include "ExynosSecondaryDisplayModule.h"
#endif

int getGCD(int a, int b)
{
    if (b == 0)
        return a;
    else
        return getGCD(b, a%b);
}

int getLCM(int a, int b)
{
    return (a*b)/getGCD(a,b);
}

bool frameChanged(decon_frame *f1, decon_frame *f2)
{
    return f1->x != f2->x ||
            f1->y != f2->y ||
            f1->w != f2->w ||
            f1->h != f2->h ||
            f1->f_w != f2->f_w ||
            f1->f_h != f2->f_h;
}

bool winConfigChanged(decon_win_config *c1, decon_win_config *c2)
{
    return c1->state != c2->state ||
            c1->fd_idma[0] != c2->fd_idma[0] ||
            c1->fd_idma[1] != c2->fd_idma[1] ||
            c1->fd_idma[2] != c2->fd_idma[2] ||
            frameChanged(&c1->src, &c2->src) ||
            frameChanged(&c1->dst, &c2->dst) ||
            c1->format != c2->format ||
            c1->blending != c2->blending ||
            c1->plane_alpha != c2->plane_alpha;
}

void ExynosDisplay::dumpConfig(decon_win_config &c)
{
    DISPLAY_LOGD(eDebugWinConfig, "\tstate = %u", c.state);
    if (c.state == c.DECON_WIN_STATE_COLOR) {
        DISPLAY_LOGD(eDebugWinConfig, "\t\tcolor = %u", c.color);
    } else if (c.state != c.DECON_WIN_STATE_DISABLED) {
        DISPLAY_LOGD(eDebugWinConfig, "\t\tfd = %d, dma = %u "
                "src_f_w = %u, src_f_h = %u, src_x = %d, src_y = %d, src_w = %u, src_h = %u, "
                "dst_f_w = %u, dst_f_h = %u, dst_x = %d, dst_y = %d, dst_w = %u, dst_h = %u, "
                "format = %u, blending = %u, protection = %u, transparent(x:%d, y:%d, w:%d, h:%d), "
                "block(x:%d, y:%d, w:%d, h:%d)",
                c.fd_idma[0], c.idma_type,
                c.src.f_w, c.src.f_h, c.src.x, c.src.y, c.src.w, c.src.h,
                c.dst.f_w, c.dst.f_h, c.dst.x, c.dst.y, c.dst.w, c.dst.h,
                c.format, c.blending, c.protection,
                c.transparent_area.x, c.transparent_area.y, c.transparent_area.w, c.transparent_area.h,
                c.covered_opaque_area.x, c.covered_opaque_area.y, c.covered_opaque_area.w, c.covered_opaque_area.h);
    }
}

void ExynosDisplay::dumpConfig(decon_win_config &c, android::String8& result)
{
    result.appendFormat("\tstate = %u", c.state);
    if (c.state == c.DECON_WIN_STATE_COLOR) {
        result.appendFormat("\t\tcolor = %u", c.color);
    } else {
        result.appendFormat("\t\tfd = %d, dma = %u "
                "src_f_w = %u, src_f_h = %u, src_x = %d, src_y = %d, src_w = %u, src_h = %u, "
                "dst_f_w = %u, dst_f_h = %u, dst_x = %d, dst_y = %d, dst_w = %u, dst_h = %u, "
                "format = %u, blending = %u, protection = %u, transparent(x:%d, y:%d, w:%d, h:%d), "
                "block(x:%d, y:%d, w:%d, h:%d)\n",
                c.fd_idma[0], c.idma_type,
                c.src.f_w, c.src.f_h, c.src.x, c.src.y, c.src.w, c.src.h,
                c.dst.f_w, c.dst.f_h, c.dst.x, c.dst.y, c.dst.w, c.dst.h,
                c.format, c.blending, c.protection,
                c.transparent_area.x, c.transparent_area.y, c.transparent_area.w, c.transparent_area.h,
                c.covered_opaque_area.x, c.covered_opaque_area.y, c.covered_opaque_area.w, c.covered_opaque_area.h);
    }
}

enum decon_pixel_format halFormatToS3CFormat(int format)
{
    switch (format) {
    case HAL_PIXEL_FORMAT_RGBA_8888:
        return DECON_PIXEL_FORMAT_RGBA_8888;
    case HAL_PIXEL_FORMAT_RGBX_8888:
        return DECON_PIXEL_FORMAT_RGBX_8888;
    case HAL_PIXEL_FORMAT_RGB_565:
        return DECON_PIXEL_FORMAT_RGB_565;
    case HAL_PIXEL_FORMAT_BGRA_8888:
        return DECON_PIXEL_FORMAT_BGRA_8888;
#ifdef EXYNOS_SUPPORT_BGRX_8888
    case HAL_PIXEL_FORMAT_BGRX_8888:
        return DECON_PIXEL_FORMAT_BGRX_8888;
#endif
    case HAL_PIXEL_FORMAT_EXYNOS_YV12_M:
        return DECON_PIXEL_FORMAT_YVU420M;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M:
        return DECON_PIXEL_FORMAT_YUV420M;
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL:
        return DECON_PIXEL_FORMAT_NV21M;
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
        return DECON_PIXEL_FORMAT_NV21;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV:
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B:
        return DECON_PIXEL_FORMAT_NV12M;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B:
        return DECON_PIXEL_FORMAT_NV12N_10B;
    case HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN:
        return DECON_PIXEL_FORMAT_NV12N;
    default:
        return DECON_PIXEL_FORMAT_MAX;
    }
}

int S3CFormatToHalFormat(enum decon_pixel_format format)
{
    switch (format) {
    case DECON_PIXEL_FORMAT_RGBA_8888:
        return HAL_PIXEL_FORMAT_RGBA_8888;
    case DECON_PIXEL_FORMAT_RGBX_8888:
        return HAL_PIXEL_FORMAT_RGBX_8888;
    case DECON_PIXEL_FORMAT_RGB_565:
        return HAL_PIXEL_FORMAT_RGB_565;
    case DECON_PIXEL_FORMAT_BGRA_8888:
        return HAL_PIXEL_FORMAT_BGRA_8888;
#ifdef EXYNOS_SUPPORT_BGRX_8888
    case DECON_PIXEL_FORMAT_BGRX_8888:
        return HAL_PIXEL_FORMAT_BGRX_8888;
#endif
    case DECON_PIXEL_FORMAT_YVU420M:
        return HAL_PIXEL_FORMAT_EXYNOS_YV12_M;
    case DECON_PIXEL_FORMAT_YUV420M:
        return HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M;
    /* HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL ?? */
    case DECON_PIXEL_FORMAT_NV21M:
        return HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;
    case DECON_PIXEL_FORMAT_NV21:
        return HAL_PIXEL_FORMAT_YCrCb_420_SP;
    /* HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B */
    case DECON_PIXEL_FORMAT_NV12M:
        return HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M;
    case DECON_PIXEL_FORMAT_NV12N:
        return HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN;
    case DECON_PIXEL_FORMAT_NV12N_10B:
        return HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B;
    default:
        return -1;
    }
}

bool isFormatSupported(int format)
{
    return halFormatToS3CFormat(format) < DECON_PIXEL_FORMAT_MAX;
}

enum decon_blending halBlendingToS3CBlending(int32_t blending)
{
    switch (blending) {
    case HWC_BLENDING_NONE:
        return DECON_BLENDING_NONE;
    case HWC_BLENDING_PREMULT:
        return DECON_BLENDING_PREMULT;
    case HWC_BLENDING_COVERAGE:
        return DECON_BLENDING_COVERAGE;

    default:
        return DECON_BLENDING_MAX;
    }
}

bool isBlendingSupported(int32_t blending)
{
    return halBlendingToS3CBlending(blending) < DECON_BLENDING_MAX;
}

#define NUMA(a) (sizeof(a) / sizeof(a [0]))
const char *deconFormat2str(uint32_t format)
{
    android::String8 result;

    for (unsigned int n1 = 0; n1 < NUMA(deconFormat); n1++) {
        if (format == deconFormat[n1].format) {
            return deconFormat[n1].desc;
        }
    }

    result.appendFormat("? %08x", format);
    return result;
}

enum vpp_rotate halTransformToHWRot(uint32_t halTransform)
{
    switch (halTransform) {
    case HAL_TRANSFORM_FLIP_H:
        return VPP_ROT_YFLIP;
    case HAL_TRANSFORM_FLIP_V:
        return VPP_ROT_XFLIP;
    case HAL_TRANSFORM_ROT_180:
        return VPP_ROT_180;
    case HAL_TRANSFORM_ROT_90:
        return VPP_ROT_90;
    case (HAL_TRANSFORM_ROT_90|HAL_TRANSFORM_FLIP_H):
        /*
         * HAL: HAL_TRANSFORM_FLIP_H -> HAL_TRANSFORM_ROT_90
         * VPP: ROT_90 -> XFLIP
         */
        return VPP_ROT_90_XFLIP;
    case (HAL_TRANSFORM_ROT_90|HAL_TRANSFORM_FLIP_V):
        /*
         * HAL: HAL_TRANSFORM_FLIP_V -> HAL_TRANSFORM_ROT_90
         * VPP: ROT_90 -> YFLIP
         */
        return VPP_ROT_90_YFLIP;
    case HAL_TRANSFORM_ROT_270:
        return VPP_ROT_270;
    default:
        return VPP_ROT_NORMAL;
    }
}

ExynosDisplay::ExynosDisplay(int __unused numGSCs)
    :   mDisplayFd(-1),
        mType(0),
        mPanelType(PANEL_LEGACY),
        mDSCHSliceNum(WINUPDATE_DSC_H_SLICE_NUM),
        mDSCYSliceSize(WINUPDATE_DSC_Y_SLICE_SIZE),
        mXres(0),
        mYres(0),
        mXdpi(0),
        mYdpi(0),
        mVsyncPeriod(0),
        mBlanked(true),
        mHwc(NULL),
        mAllocDevice(NULL),
        mGrallocModule(NULL),
        mLastFbWindow(NO_FB_NEEDED),
        mVirtualOverlayFlag(0),
        mBypassSkipStaticLayer(false),
        mMPPLayers(0),
        mYuvLayers(0),
        mHasDrmSurface(false),
        mFbNeeded(false),
        mFirstFb(0),
        mLastFb(0),
        mFbWindow(0),
        mForceFb(false),
        mForceOverlayLayerIndex(-1),
        mAllowedOverlays(5),
        mOtfMode(OTF_OFF),
        mGscUsed(false),
        mMaxWindowOverlapCnt(NUM_HW_WINDOWS),
        mUseSecureDMA(false),
        mExternalMPPDstFormat(HAL_PIXEL_FORMAT_RGBX_8888),
        mSkipStaticInitFlag(false),
        mNumStaticLayers(0),
        mLastRetireFenceFd(-1),
        mFbPreAssigned(false),
        mActiveConfigIndex(0),
        mWinData(NULL)
{
    memset(mLastMPPMap, 0, sizeof(mLastMPPMap));
    memset(mLastHandles, 0, sizeof(mLastHandles));
    memset(&mLastConfigData, 0, sizeof(mLastConfigData));
    memset(mLastLayerHandles, 0, sizeof(mLastLayerHandles));
    memset(&mFbUpdateRegion, 0, sizeof(mFbUpdateRegion));

    mPreProcessedInfo.mHasDrmSurface = false;
    mCheckIntMPP = new ExynosMPPModule(NULL, MPP_VGR, 0);

    mWinData = (struct decon_win_config_data *)malloc(sizeof(*mWinData));
    if (mWinData == NULL)
        DISPLAY_LOGE("Fail to allocate mWinData");
}
ExynosDisplay::ExynosDisplay(uint32_t type, struct exynos5_hwc_composer_device_1_t *pdev)
    :   mDisplayFd(-1),
        mType(type),
        mPanelType(PANEL_LEGACY),
        mDSCHSliceNum(WINUPDATE_DSC_H_SLICE_NUM),
        mDSCYSliceSize(WINUPDATE_DSC_Y_SLICE_SIZE),
        mXres(0),
        mYres(0),
        mXdpi(0),
        mYdpi(0),
        mVsyncPeriod(0),
        mBlanked(true),
        mHwc(pdev),
        mAllocDevice(NULL),
        mGrallocModule(NULL),
        mLastFbWindow(NO_FB_NEEDED),
        mVirtualOverlayFlag(0),
        mBypassSkipStaticLayer(false),
        mMPPLayers(0),
        mYuvLayers(0),
        mHasDrmSurface(false),
        mFbNeeded(false),
        mFirstFb(0),
        mLastFb(0),
        mFbWindow(0),
        mForceFb(false),
        mForceOverlayLayerIndex(-1),
        mAllowedOverlays(5),
        mOtfMode(OTF_OFF),
        mGscUsed(false),
        mMaxWindowOverlapCnt(NUM_HW_WINDOWS),
        mUseSecureDMA(false),
        mExternalMPPDstFormat(HAL_PIXEL_FORMAT_RGBX_8888),
        mSkipStaticInitFlag(false),
        mNumStaticLayers(0),
        mLastRetireFenceFd(-1),
        mFbPreAssigned(false),
        mActiveConfigIndex(0),
        mWinData(NULL)
{
    memset(mLastMPPMap, 0, sizeof(mLastMPPMap));
    memset(mLastHandles, 0, sizeof(mLastHandles));
    memset(&mLastConfigData, 0, sizeof(mLastConfigData));
    memset(mLastLayerHandles, 0, sizeof(mLastLayerHandles));

    switch (mType) {
    case EXYNOS_VIRTUAL_DISPLAY:
        mDisplayName = android::String8("VirtualDisplay");
        break;
    case EXYNOS_EXTERNAL_DISPLAY:
        mDisplayName = android::String8("ExternalDisplay");
        break;
    case EXYNOS_PRIMARY_DISPLAY:
        mDisplayName = android::String8("PrimaryDisplay");
        break;
#if defined(USES_DUAL_DISPLAY)
    case EXYNOS_SECONDARY_DISPLAY:
        mDisplayName = android::String8("SecondaryDisplay");
        break;
#endif
    default:
        mDisplayName = android::String8("Unknown");
        break;
    }
    memset(&mFbUpdateRegion, 0, sizeof(mFbUpdateRegion));

    mPreProcessedInfo.mHasDrmSurface = false;
    mCheckIntMPP = new ExynosMPPModule(NULL, MPP_VGR, 0);

    mWinData = (struct decon_win_config_data *)malloc(sizeof(*mWinData));
    if (mWinData == NULL)
        DISPLAY_LOGE("Fail to allocate mWinData");
}

ExynosDisplay::~ExynosDisplay()
{
    if (!mLayerInfos.isEmpty()) {
        for (size_t i = 0; i < mLayerInfos.size(); i++) {
            delete mLayerInfos[i];
        }
        mLayerInfos.clear();
    }
    if (mCheckIntMPP != NULL) {
        delete mCheckIntMPP;
        mCheckIntMPP = NULL;
    }

    if (mWinData != NULL)
        free(mWinData);
}

int ExynosDisplay::prepare(hwc_display_contents_1_t *contents)
{
    ATRACE_CALL();
    DISPLAY_LOGD(eDebugDefault, "preparing %u layers for FIMD", contents->numHwLayers);

    if (!mForceFb)
        skipStaticLayers(contents);

    if (mVirtualOverlayFlag)
        mFbNeeded = 0;

    if (!mFbNeeded)
        mFbWindow = NO_FB_NEEDED;

    return 0;
}

int ExynosDisplay::set(hwc_display_contents_1_t *contents)
{
    hwc_layer_1_t *fb_layer = NULL;
    int err = 0;

    if (mFbWindow != NO_FB_NEEDED) {
        if (contents->numHwLayers >= 1 &&
                contents->hwLayers[contents->numHwLayers - 1].compositionType == HWC_FRAMEBUFFER_TARGET)
            fb_layer = &contents->hwLayers[contents->numHwLayers - 1];

        if (CC_UNLIKELY(!fb_layer)) {
            DISPLAY_LOGE("framebuffer target expected, but not provided");
            err = -EINVAL;
        } else {
            DISPLAY_LOGD(eDebugDefault, "framebuffer target buffer:");
            dumpLayer(eDebugDefault, fb_layer);
        }
    }

    int fence;
    if (!err) {
        fence = postFrame(contents);
        if (fence < 0)
            err = fence;
    }

#if defined(USES_DUAL_DISPLAY)
    if (mType != EXYNOS_SECONDARY_DISPLAY)
    {
#endif
    if (err)
        fence = clearDisplay();

    if (fence == 0) {
        /*
         * WIN_CONFIG is skipped, not error
         */
        fence = -1;
        if (mLastRetireFenceFd >= 0) {
            int dup_fd = dup(mLastRetireFenceFd);
            if (dup_fd >= 0)  {
                fence = dup_fd;
                mLastRetireFenceFd = dup_fd;
                dupFence(fence, contents);
            } else {
                DISPLAY_LOGW("mLastRetireFenceFd dup failed: %s", strerror(errno));
                mLastRetireFenceFd = -1;
            }
        } else {
            ALOGE("WIN_CONFIG is skipped, but mLastRetireFenceFd is not valid");
        }
    } else {
        mLastRetireFenceFd = fence;
        dupFence(fence, contents);
    }
#if defined(USES_DUAL_DISPLAY)
    }
#endif

    return err;
}

void ExynosDisplay::dupFence(int fence, hwc_display_contents_1_t *contents)
{
    if (contents == NULL)
        return;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        private_handle_t *handle = NULL;
        if (layer.handle != NULL)
            handle = private_handle_t::dynamicCast(layer.handle);

        if ((mVirtualOverlayFlag == true) && (layer.compositionType == HWC_OVERLAY) &&
                ((handle != NULL) && (getDrmMode(handle->flags) == NO_DRM)) &&
                (mFirstFb <= i) && (i <= mLastFb))
            continue;

        if (!(layer.flags & HWC_SKIP_RENDERING) && ((layer.compositionType == HWC_OVERLAY) ||
                    ((mFbNeeded == true || this->mVirtualOverlayFlag) && layer.compositionType == HWC_FRAMEBUFFER_TARGET))) {
            int dup_fd = dup(fence);
            DISPLAY_LOGD(eDebugFence, "%d layer[type: %d, dst: %d, %d, %d, %d] fence is duplicated(%d)",
                    i, layer.compositionType,
                    layer.displayFrame.left, layer.displayFrame.top,
                    layer.displayFrame.right, layer.displayFrame.bottom,
                    dup_fd);
            if (dup_fd < 0)
                DISPLAY_LOGW("release fence dup failed: %s", strerror(errno));
            if (mLayerInfos[i]->mInternalMPP != NULL) {
                ExynosMPPModule *exynosMPP = mLayerInfos[i]->mInternalMPP;
                if (mLayerInfos[i]->mInternalMPP->mDstBufFence[0] >= 0)
                    close(mLayerInfos[i]->mInternalMPP->mDstBufFence[0]);
                exynosMPP->mDstBufFence[0] = dup(fence);
            }
            if (mLayerInfos[i]->mExternalMPP != NULL) {
                ExynosMPPModule *exysnosMPP = mLayerInfos[i]->mExternalMPP;
                if (exysnosMPP->mDstBufFence[exysnosMPP->mCurrentBuf] >= 0) {
                    close (exysnosMPP->mDstBufFence[exysnosMPP->mCurrentBuf]);
                    exysnosMPP->mDstBufFence[exysnosMPP->mCurrentBuf] = -1;
                }
                exysnosMPP->mDstBufFence[exysnosMPP->mCurrentBuf] = dup_fd;
                exysnosMPP->mCurrentBuf = (exysnosMPP->mCurrentBuf + 1) % exysnosMPP->mNumAvailableDstBuffers;
            } else {
                if (this->mVirtualOverlayFlag && (layer.compositionType == HWC_FRAMEBUFFER_TARGET)) {
                    if (layer.releaseFenceFd >= 0)
                        close(layer.releaseFenceFd);
                }
                layer.releaseFenceFd = dup_fd;
            }
        }
    }

#if defined(USES_DUAL_DISPLAY)
    if (mType == EXYNOS_SECONDARY_DISPLAY)
        contents->retireFenceFd = dup(fence);
    else
        contents->retireFenceFd = fence;
#else
    contents->retireFenceFd = fence;
#endif
}

void ExynosDisplay::dump(android::String8& result)
{
    result.append(
            "   type   |    handle    |  color   | blend | pa |    format     |   position    |     size      | intMPP  | extMPP \n"
            "----------+--------------|----------+-------+----+---------------+---------------+----------------------------------\n");
    //        8_______ | 12__________ | 8_______ | 5____ | 2_ | 13___________ | [5____,5____] | [5____,5____] | [2_,2_] | [2_,2_]\n"

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        struct decon_win_config &config = mLastConfigData.config[i];
        if ((config.state == config.DECON_WIN_STATE_DISABLED) &&
            (mLastMPPMap[i].internal_mpp.type == -1) &&
            (mLastMPPMap[i].external_mpp.type == -1)){
            result.appendFormat(" %8s | %12s | %8s | %5s | %2s | %13s | %13s | %13s",
                    "OVERLAY", "-", "-", "-", "-", "-", "-", "-");
        }
        else {
            if (config.state == config.DECON_WIN_STATE_COLOR)
                result.appendFormat(" %8s | %12s | %8x | %5s | %2s | %13s", "COLOR",
                        "-", config.color, "-", "-", "-");
            else
                result.appendFormat(" %8s | %12" PRIxPTR " | %8s | %5x | %2x | %13s",
                        mLastFbWindow == i ? "FB" : "OVERLAY",
                        intptr_t(mLastHandles[i]),
                        "-", config.blending, config.plane_alpha, deconFormat2str(config.format));

            result.appendFormat(" | [%5d,%5d] | [%5u,%5u]", config.dst.x, config.dst.y,
                    config.dst.w, config.dst.h);
        }
        if (mLastMPPMap[i].internal_mpp.type == -1) {
            result.appendFormat(" | [%2s,%2s]", "-", "-");
        } else {
            result.appendFormat(" | [%2d,%2d]", mLastMPPMap[i].internal_mpp.type, mLastMPPMap[i].internal_mpp.index);
        }

        if (mLastMPPMap[i].external_mpp.type == -1) {
            result.appendFormat(" | [%2s,%2s]", "-", "-");
        } else {
            result.appendFormat(" | [%2d,%2d]", mLastMPPMap[i].external_mpp.type, mLastMPPMap[i].external_mpp.index);
        }
        result.append("\n");
    }
}

void ExynosDisplay::freeMPP()
{
}

void ExynosDisplay::doPreProcessing(hwc_display_contents_1_t* contents)
{
    mPreProcessedInfo.mHasDrmSurface = false;
    mForceOverlayLayerIndex = -1;
    this->mHasDrmSurface = false;
    mYuvLayers = 0;
    mLayersNeedScaling = false;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.handle) {
            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
            if (h->flags & GRALLOC_USAGE_PROTECTED) {
                mPreProcessedInfo.mHasDrmSurface = true;
                this->mHasDrmSurface = true;
                mForceOverlayLayerIndex = i;
            }
            if (!isFormatRgb(h->format)) {
                this->mYuvLayers++;
            }
            if (isScaled(layer))
                mLayersNeedScaling = true;
        }
    }
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

    for (size_t i= 0; i < contents->numHwLayers; i++) {
        ExynosLayerInfo *layerInfo = new ExynosLayerInfo();
        memset(layerInfo, 0, sizeof(ExynosLayerInfo));
        layerInfo->mDmaType = -1;
        mLayerInfos.push(layerInfo);
    }

    mForceFb = mHwc->force_gpu;

    doPreProcessing(contents);
}

void ExynosDisplay::dumpLayerInfo(android::String8& result)
{
    if (!mLayerInfos.isEmpty()) {
        result.append(
                "    type    | CheckOverlayFlag | CheckMPPFlag | Comp | mWinIndex | mDmaType |   mIntMPP |  mExtMPP \n"
                "------------+------------------+--------------+------+-----------+----------+-----------+----------\n");
    //            10________ |         8_______ |     8_______ | 3__  | 9________ | 8_______ | [3__, 2_] | [3__, 2_]\n"
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
                    " %10s |       0x%8x |   0x%8x |    %1s | %9d | %8d",
                    compositionTypeName[type],
                    mLayerInfos[i]->mCheckOverlayFlag, mLayerInfos[i]->mCheckMPPFlag,
                    mLayerInfos[i]->mCompressed ? "Y" : "N",
                    mLayerInfos[i]->mWindowIndex, mLayerInfos[i]->mDmaType);

            if (mLayerInfos[i]->mInternalMPP == NULL)
                result.appendFormat(" | [%3s, %2s]", "-", "-");
            else {
                result.appendFormat(" | [%3s, %2d]", mLayerInfos[i]->mInternalMPP->getName().string(), mLayerInfos[i]->mInternalMPP->mIndex);
            }

            if (mLayerInfos[i]->mExternalMPP == NULL)
                result.appendFormat(" | [%3s, %2s]", "-", "-");
            else {
                result.appendFormat(" | [%3s, %2d]", mLayerInfos[i]->mExternalMPP->getName().string(), mLayerInfos[i]->mExternalMPP->mIndex);
            }
            result.append("\n");
        }
    }
    result.append("\n");
}

bool ExynosDisplay::handleTotalBandwidthOverload(hwc_display_contents_1_t *contents)
{
    bool changed = false;
    bool addFB = true;
    if (mHwc->totPixels >= FIMD_TOTAL_BW_LIMIT) {
        changed = true;
        if (mFbNeeded) {
            for (int i = mFirstFb - 1; i >= 0; i--) {
                if (mForceOverlayLayerIndex == 0 && i == 0)
                    break;
                hwc_layer_1_t &layer = contents->hwLayers[i];
                layer.compositionType = HWC_FRAMEBUFFER;
                mLayerInfos[i]->compositionType = layer.compositionType;
                mLayerInfos[i]->mCheckOverlayFlag |= eInsufficientBandwidth;
                mLayerInfos[i]->mInternalMPP = NULL;
                mLayerInfos[i]->mExternalMPP = NULL;
                mFirstFb = (size_t)i;
                mHwc->totPixels -= WIDTH(layer.displayFrame) * HEIGHT(layer.displayFrame);
                if (mHwc->totPixels < FIMD_TOTAL_BW_LIMIT)
                    break;
            }
            if (mHwc->totPixels >= FIMD_TOTAL_BW_LIMIT) {
                for (size_t i = mLastFb + 1; i < contents->numHwLayers - 1; i++) {
                    hwc_layer_1_t &layer = contents->hwLayers[i];
                    layer.compositionType = HWC_FRAMEBUFFER;
                    mLayerInfos[i]->compositionType = layer.compositionType;
                    mLayerInfos[i]->mCheckOverlayFlag |= eInsufficientBandwidth;
                    mLayerInfos[i]->mInternalMPP = NULL;
                    mLayerInfos[i]->mExternalMPP = NULL;
                    mLastFb = i;
                    mHwc->totPixels -= WIDTH(layer.displayFrame) * HEIGHT(layer.displayFrame);
                    if (mHwc->totPixels < FIMD_TOTAL_BW_LIMIT)
                        break;
                }
            }
        } else {
            for (size_t i = 0; i < contents->numHwLayers; i++) {
                hwc_layer_1_t &layer = contents->hwLayers[i];
                if (layer.compositionType == HWC_OVERLAY &&
                        mForceOverlayLayerIndex != (int)i) {
                    layer.compositionType = HWC_FRAMEBUFFER;
                    mLastFb = max(mLastFb, i);
                    mLayerInfos[i]->compositionType = layer.compositionType;
                    mLayerInfos[i]->mCheckOverlayFlag |= eInsufficientBandwidth;
                    mLayerInfos[i]->mInternalMPP = NULL;
                    mLayerInfos[i]->mExternalMPP = NULL;
                    if (addFB) {
                        addFB = false;
                        mHwc->totPixels += mXres * mYres;
                    }
                    mHwc->totPixels -= WIDTH(layer.displayFrame) * HEIGHT(layer.displayFrame);
                    if (mHwc->totPixels < FIMD_TOTAL_BW_LIMIT)
                        break;
                }
             }
            if (mForceOverlayLayerIndex == 0)
                mFirstFb = 1;
            else
                mFirstFb = 0;
        }
        mFbNeeded = true;
    }

    return changed;
}

int ExynosDisplay::clearDisplay()
{
    struct decon_win_config_data win_data;
    memset(&win_data, 0, sizeof(win_data));

    int ret = ioctl(this->mDisplayFd, S3CFB_WIN_CONFIG, &win_data);
    LOG_ALWAYS_FATAL_IF(ret < 0,
            "%s ioctl S3CFB_WIN_CONFIG failed to clear screen: %s",
            mDisplayName.string(), strerror(errno));
    // the causes of an empty config failing are all unrecoverable

    return win_data.fence;
}

int ExynosDisplay::getCompModeSwitch()
{
    unsigned int tot_win_size = 0, updateFps = 0;
    unsigned int lcd_size = this->mXres * this->mYres;
    uint64_t TimeStampDiff;
    float Temp;

    if (!mHwc->hwc_ctrl.dynamic_recomp_mode) {
        mHwc->LastModeSwitchTimeStamp = 0;
        mHwc->CompModeSwitch = NO_MODE_SWITCH;
        return 0;
    }

    /* initialize the Timestamps */
    if (!mHwc->LastModeSwitchTimeStamp) {
        mHwc->LastModeSwitchTimeStamp = mHwc->LastUpdateTimeStamp;
        mHwc->CompModeSwitch = NO_MODE_SWITCH;
        return 0;
    }

    /* If video layer is there, skip the mode switch */
    if (mYuvLayers || mLayersNeedScaling) {
        if (mHwc->CompModeSwitch != HWC_2_GLES) {
            return 0;
        } else {
            mHwc->CompModeSwitch = GLES_2_HWC;
            mHwc->updateCallCnt = 0;
            mHwc->LastModeSwitchTimeStamp = mHwc->LastUpdateTimeStamp;
            DISPLAY_LOGI("[DYNAMIC_RECOMP] GLES_2_HWC by video layer");
            return GLES_2_HWC;
        }
    }

    /* Mode Switch is not required if total pixels are not more than the threshold */
    if ((uint32_t)mHwc->incomingPixels <= lcd_size * HWC_FIMD_BW_TH) {
        if (mHwc->CompModeSwitch != HWC_2_GLES) {
            return 0;
        } else {
            mHwc->CompModeSwitch = GLES_2_HWC;
            mHwc->updateCallCnt = 0;
            mHwc->LastModeSwitchTimeStamp = mHwc->LastUpdateTimeStamp;
            DISPLAY_LOGI("[DYNAMIC_RECOMP] GLES_2_HWC by BW check");
            return GLES_2_HWC;
        }
    }

    /*
     * There will be at least one composition call per one minute (because of time update)
     * To minimize the analysis overhead, just analyze it once in a second
     */
    TimeStampDiff = systemTime(SYSTEM_TIME_MONOTONIC) - mHwc->LastModeSwitchTimeStamp;

    /*
     * previous CompModeSwitch was GLES_2_HWC: check fps every 250ms from LastModeSwitchTimeStamp
     * previous CompModeSwitch was HWC_2_GLES: check immediately
     */
    if ((mHwc->CompModeSwitch != HWC_2_GLES) && (TimeStampDiff < (VSYNC_INTERVAL * 15))) {
        return 0;
    }
    mHwc->LastModeSwitchTimeStamp = mHwc->LastUpdateTimeStamp;
    if ((mHwc->update_event_cnt != 1) && // This is not called by hwc_update_stat_thread
        (mHwc->CompModeSwitch == HWC_2_GLES) && (mHwc->updateCallCnt == 1)) {
        DISPLAY_LOGI("[DYNAMIC_RECOMP] first frame after HWC_2_GLES");
        updateFps = HWC_FPS_TH;
    } else {
        Temp = (VSYNC_INTERVAL * 60) / TimeStampDiff;
        updateFps = (int)(mHwc->updateCallCnt * Temp + 0.5);
    }
    mHwc->updateCallCnt = 0;
    /*
     * FPS estimation.
     * If FPS is lower than HWC_FPS_TH, try to switch the mode to GLES
     */
    if (updateFps < HWC_FPS_TH) {
        if (mHwc->CompModeSwitch != HWC_2_GLES) {
            mHwc->CompModeSwitch = HWC_2_GLES;
            DISPLAY_LOGI("[DYNAMIC_RECOMP] HWC_2_GLES by low FPS(%d)", updateFps);
            return HWC_2_GLES;
        } else {
            return 0;
        }
    } else {
         if (mHwc->CompModeSwitch == HWC_2_GLES) {
            mHwc->CompModeSwitch = GLES_2_HWC;
            DISPLAY_LOGI("[DYNAMIC_RECOMP] GLES_2_HWC by high FPS(%d)", updateFps);
            return GLES_2_HWC;
        } else {
            return 0;
        }
    }

    return 0;
}

int32_t ExynosDisplay::getDisplayAttributes(const uint32_t attribute, uint32_t __unused config)
{
    switch(attribute) {
    case HWC_DISPLAY_VSYNC_PERIOD:
        return this->mVsyncPeriod;

    case HWC_DISPLAY_WIDTH:
#if defined(USES_DUAL_DISPLAY)
        if ((mType == EXYNOS_PRIMARY_DISPLAY) || (mType == EXYNOS_SECONDARY_DISPLAY))
            return this->mXres/2;
        else
            return mXres;
#else
        return this->mXres;
#endif

    case HWC_DISPLAY_HEIGHT:
        return this->mYres;

    case HWC_DISPLAY_DPI_X:
        return this->mXdpi;

    case HWC_DISPLAY_DPI_Y:
        return this->mYdpi;

    default:
        DISPLAY_LOGE("unknown display attribute %u", attribute);
        return -EINVAL;
    }
}

bool ExynosDisplay::isOverlaySupportedByIDMA(hwc_layer_1_t __unused &layer, size_t __unused index)
{
    if (isCompressed(layer))
        return false;
    else
        return true;
}

void ExynosDisplay::getIDMAMinSize(hwc_layer_1_t __unused &layer, int *w, int *h)
{
    *w = 1;
    *h = 1;
}

bool ExynosDisplay::isOverlaySupported(hwc_layer_1_t &layer, size_t index, bool useVPPOverlay,
        ExynosMPPModule** supportedInternalMPP, ExynosMPPModule** supportedExternalMPP)
{
    int mMPPIndex = 0;
    int ret = 0;
    ExynosMPPModule* transitionInternalMPP = NULL;
    private_handle_t *handle = NULL;
    int handleFormat = 0;
    bool firstFrameFramebufferTarget = false;

    DISPLAY_LOGD(eDebugOverlaySupported, "isOverlaySupported:: index(%d), useVPPOverlay(%d)", index, useVPPOverlay);

    if (layer.flags & HWC_SKIP_LAYER) {
        mLayerInfos[index]->mCheckOverlayFlag |= eSkipLayer;
        DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: skipping", index);
        return false;
    }

    if (!layer.planeAlpha)
        return true;

    if (index == 0 && layer.planeAlpha < 255) {
        DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: eUnsupportedPlaneAlpha", index);
        mLayerInfos[index]->mCheckOverlayFlag |= eUnsupportedPlaneAlpha;
        return false;
    }

    if (layer.handle) {
        handle = private_handle_t::dynamicCast(layer.handle);
        handleFormat = handle->format;
    }

    if ((layer.compositionType != HWC_FRAMEBUFFER_TARGET) && !handle) {
        DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: handle is NULL, type is %d", index, layer.compositionType);
        mLayerInfos[index]->mCheckOverlayFlag |= eInvalidHandle;
        return false;
    }

    if (!handle && (layer.compositionType == HWC_FRAMEBUFFER_TARGET)) {
        firstFrameFramebufferTarget = true;
        handleFormat = HAL_PIXEL_FORMAT_RGBA_8888;
    }

    if (handle && (getDrmMode(handle->flags) == NO_DRM) &&
        (isFloat(layer.sourceCropf.left) || isFloat(layer.sourceCropf.top) ||
         isFloat(layer.sourceCropf.right - layer.sourceCropf.left) ||
         isFloat(layer.sourceCropf.bottom - layer.sourceCropf.top))) {
        if (isSourceCropfSupported(layer) == false)
            return false;
    }

    if (!isBlendingSupported(layer.blending)) {
        DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: blending %d not supported", index, layer.blending);
        mLayerInfos[index]->mCheckOverlayFlag |= eUnsupportedBlending;
        return false;
    }

    int32_t bpp = formatToBpp(handleFormat);
    int32_t left = max(layer.displayFrame.left, 0);
    int32_t right = min(layer.displayFrame.right, mXres);
    uint32_t visible_width = 0;

    if ((bpp == 16) &&
            ((layer.displayFrame.left % 2 != 0) || (layer.displayFrame.right % 2 != 0))) {
        DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: eNotAlignedDstPosition", index);
        mLayerInfos[index]->mCheckOverlayFlag |= eNotAlignedDstPosition;
        return false;
    }

    visible_width = (right - left) * bpp / 8;
    if (visible_width < BURSTLEN_BYTES) {
#ifdef USE_DRM_BURST_LEN
        if (handle && (getDrmMode(handle->flags) != NO_DRM)) {
            if (visible_width < DRM_BURSTLEN_BYTES) {
                DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: visible area is too narrow", index);
                mLayerInfos[index]->mCheckOverlayFlag |= eUnsupportedDstWidth;
                return false;
            }
        } else {
#endif
            DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: visible area is too narrow", index);
            mLayerInfos[index]->mCheckOverlayFlag |= eUnsupportedDstWidth;
            return false;
#ifdef USE_DRM_BURST_LEN
        }
#endif
    }

    if (!isProcessingRequired(layer) && !useVPPOverlay)
        return true;

    hwc_layer_1_t extMPPOutLayer = layer;
    int originalHandleFormt =  handleFormat;
    int dst_format = handleFormat;
    bool isBothMPPUsed = isBothMPPProcessingRequired(layer, &extMPPOutLayer);
    DISPLAY_LOGD(eDebugOverlaySupported, "isOverlaySupported:: index(%d), isBothMPPUsed(%d)", index, isBothMPPUsed);

    if (isBothMPPUsed) {
        if ((*supportedInternalMPP != NULL) && (*supportedExternalMPP != NULL))
            return true;
    } else {
        if ((*supportedInternalMPP != NULL) || (*supportedExternalMPP != NULL && !useVPPOverlay))
            return true;
    }

    if (*supportedExternalMPP == NULL && isBothMPPUsed)
    {
        /* extMPPOutLayer is output of ExtMPP
         * The output of ExtMPP is the input of IntMPP
         */
        if (!isFormatRgb(handleFormat) &&
            (WIDTH(extMPPOutLayer.displayFrame) % mCheckIntMPP->getCropWidthAlign(layer) != 0 ||
             HEIGHT(extMPPOutLayer.displayFrame) % mCheckIntMPP->getCropHeightAlign(layer) != 0 ||
             !(mCheckIntMPP->isFormatSupportedByMPP(handleFormat)) ||
             !(mCheckIntMPP->isCSCSupportedByMPP(handleFormat, HAL_PIXEL_FORMAT_RGBX_8888, layer.dataSpace))))
            dst_format = mExternalMPPDstFormat;

        /* extMPPOutLayer is output of ExtMPP */
        for (size_t i = 0; i < mExternalMPPs.size(); i++)
        {
            ExynosMPPModule* externalMPP = mExternalMPPs[i];
            if (externalMPP->mState == MPP_STATE_FREE) {
                ret = externalMPP->isProcessingSupported(extMPPOutLayer, dst_format);
                if (ret > 0) {
                    *supportedExternalMPP = externalMPP;
                    break;
                } else {
                    mLayerInfos[index]->mCheckMPPFlag |= -ret;
                }
            }
        }

        /* Can't find valid externalMPP */
        if (*supportedExternalMPP == NULL) {
            DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: Can't find valid externalMPP", index);
            mLayerInfos[index]->mCheckOverlayFlag |= eInsufficientMPP;
            return false;
        }
    }
    if (*supportedInternalMPP == NULL) {
        for (size_t i = 0; i < mInternalMPPs.size(); i++)
        {
            ExynosMPPModule* internalMPP = mInternalMPPs[i];
            hwc_layer_1_t extMPPTempOutLayer = extMPPOutLayer;
            if (isBothMPPUsed) {
                if (internalMPP->mType == MPP_VPP_G)
                    continue;
                /* extMPPOutLayer is output of ExtMPP
                 * The output of ExtMPP is the input of IntMPP
                 */
                if (!isFormatRgb(handleFormat) &&
                    (WIDTH(extMPPTempOutLayer.displayFrame) % internalMPP->getCropWidthAlign(layer) != 0 ||
                     HEIGHT(extMPPTempOutLayer.displayFrame) % internalMPP->getCropHeightAlign(layer) != 0 ||
                     !(internalMPP->isFormatSupportedByMPP(handleFormat))))
                    dst_format = mExternalMPPDstFormat;

                extMPPTempOutLayer.sourceCropf.left = extMPPOutLayer.displayFrame.left;
                extMPPTempOutLayer.sourceCropf.top = extMPPOutLayer.displayFrame.top;
                extMPPTempOutLayer.sourceCropf.right = extMPPOutLayer.displayFrame.right;
                extMPPTempOutLayer.sourceCropf.bottom = extMPPOutLayer.displayFrame.bottom;
                extMPPTempOutLayer.displayFrame.left = layer.displayFrame.left;
                extMPPTempOutLayer.displayFrame.top = layer.displayFrame.top;
                extMPPTempOutLayer.displayFrame.right = layer.displayFrame.right;
                extMPPTempOutLayer.displayFrame.bottom = layer.displayFrame.bottom;
                ((private_handle_t *)extMPPTempOutLayer.handle)->format = dst_format;
                extMPPTempOutLayer.transform = 0;
            }
            ExynosDisplay *addedDisplay = (mHwc->hdmi_hpd ? (ExynosDisplay *)mHwc->externalDisplay : (ExynosDisplay *)mHwc->virtualDisplay);
            ExynosDisplay *otherDisplay = (mType ? (ExynosDisplay *)mHwc->primaryDisplay : addedDisplay);

            /*
             * If MPP was assigned to other Device in previous frame
             * then doesn't assign it untill it is cleared
             */
            if ((internalMPP->mState == MPP_STATE_FREE) &&
                    (internalMPP->mDisplay == NULL || internalMPP->mDisplay == this)) {
                /* InternalMPP doesn't need to check dst_format. Set dst_format with source format */
                if (firstFrameFramebufferTarget)
                    ret = 1;
                else {
                    ret = internalMPP->isProcessingSupported(extMPPTempOutLayer, ((private_handle_t *)extMPPTempOutLayer.handle)->format);
                    handle->format = originalHandleFormt;
                }
                if (ret > 0) {
                    *supportedInternalMPP = internalMPP;
                    return true;
                } else {
                    mLayerInfos[index]->mCheckMPPFlag |= -ret;
                }
            } else if (internalMPP->wasUsedByDisplay(otherDisplay)) {
                DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: internalMPP[%d, %d] was used by other device", index, internalMPP->mType, internalMPP->mIndex);
                if (transitionInternalMPP == NULL)
                    transitionInternalMPP = internalMPP;
            }
            if (handle)
                handle->format = originalHandleFormt;
        }
    }

    if ((*supportedInternalMPP == NULL) && (useVPPOverlay == true) && !isProcessingRequired(layer)) {
        DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: eInsufficientMPP", index);
        mLayerInfos[index]->mCheckOverlayFlag |= eInsufficientMPP;
        if (transitionInternalMPP != NULL) {
            DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: internalMPP[%d, %d] transition is started", index, transitionInternalMPP->mType, transitionInternalMPP->mIndex);
            transitionInternalMPP->startTransition(this);
        }
        return false;
    }

    /* Can't find valid internalMPP */
    if (isBothMPPProcessingRequired(layer) && *supportedInternalMPP == NULL) {
        DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: Can't find valid internalMPP", index);
        if (transitionInternalMPP != NULL) {
            DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: internalMPP[%d, %d] transition is started", index, transitionInternalMPP->mType, transitionInternalMPP->mIndex);
            transitionInternalMPP->startTransition(this);
        }
        mLayerInfos[index]->mCheckOverlayFlag |= eInsufficientMPP;
        return false;
    }

    if (*supportedExternalMPP == NULL) {
        for (size_t i = 0; i < mExternalMPPs.size(); i++)
        {
            ExynosMPPModule* externalMPP = mExternalMPPs[i];
            int dst_format = handleFormat;
            if (!isFormatRgb(handleFormat))
                dst_format = mExternalMPPDstFormat;

            if (externalMPP->mState == MPP_STATE_FREE) {
                if (firstFrameFramebufferTarget)
                    ret = 1;
                else
                    ret = externalMPP->isProcessingSupported(layer, dst_format);
                if (ret > 0) {
                    *supportedExternalMPP = externalMPP;
                    if (useVPPOverlay)
                        break;
                    return true;
                } else {
                    mLayerInfos[index]->mCheckMPPFlag |= -ret;
                }
            }
        }
    }

    if (*supportedExternalMPP != NULL && useVPPOverlay == true && *supportedInternalMPP == NULL) {
        int originalHandleFormt =  handleFormat;
        dst_format = handleFormat;
        if (!isFormatRgb(handleFormat))
            dst_format = mExternalMPPDstFormat;
        for (size_t i = 0; i < mInternalMPPs.size(); i++)
        {
            extMPPOutLayer = layer;
            /* extMPPOutLayer is output of ExtMPP
             * The output of ExtMPP is the input of IntMPP
             */
            extMPPOutLayer.sourceCropf.left = layer.displayFrame.left;
            extMPPOutLayer.sourceCropf.top = layer.displayFrame.top;
            extMPPOutLayer.sourceCropf.right = layer.displayFrame.right;
            extMPPOutLayer.sourceCropf.bottom = layer.displayFrame.bottom;
            extMPPOutLayer.transform = 0;
            if (handle)
                ((private_handle_t *)extMPPOutLayer.handle)->format = dst_format;
            ExynosMPPModule* internalMPP = mInternalMPPs[i];

            /*
             * If MPP was assigned to other Device in previous frame
             * then doesn't assign it untill it is cleared
             */
            if ((internalMPP->mState == MPP_STATE_FREE) &&
                    (internalMPP->mDisplay == NULL || internalMPP->mDisplay == this)) {
                if (firstFrameFramebufferTarget)
                    ret = 1;
                else {
                    ret = internalMPP->isProcessingSupported(extMPPOutLayer, ((private_handle_t *)extMPPOutLayer.handle)->format);
                    handle->format = originalHandleFormt;
                }
                if (ret > 0) {
                    *supportedInternalMPP = internalMPP;
                    return true;
                } else {
                    mLayerInfos[index]->mCheckMPPFlag |= -ret;
                }
            } else {
                ExynosDisplay *addedDisplay = (mHwc->hdmi_hpd ? (ExynosDisplay *)mHwc->externalDisplay : (ExynosDisplay *)mHwc->virtualDisplay);
                ExynosDisplay *otherDisplay = (mType ? (ExynosDisplay *)mHwc->primaryDisplay : addedDisplay);
                if (firstFrameFramebufferTarget) {
                    if ((internalMPP->wasUsedByDisplay(otherDisplay)) && (transitionInternalMPP == NULL))
                        transitionInternalMPP = internalMPP;
                } else {
                    if ((internalMPP->wasUsedByDisplay(otherDisplay)) &&
                        ((transitionInternalMPP == NULL) ||
                         ((transitionInternalMPP->isProcessingSupported(extMPPOutLayer, ((private_handle_t *)extMPPOutLayer.handle)->format) < 0) &&
                          (internalMPP->isProcessingSupported(extMPPOutLayer, ((private_handle_t *)extMPPOutLayer.handle)->format) > 0))))
                        transitionInternalMPP = internalMPP;
                }
            }

            if (handle)
                handle->format = originalHandleFormt;
        }
    }

    /* Transit display for next frame */
    if ((*supportedInternalMPP == NULL) && (transitionInternalMPP != NULL)) {
        DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: internalMPP[%d, %d] transition is started", index, transitionInternalMPP->mType, transitionInternalMPP->mIndex);
        transitionInternalMPP->startTransition(this);
    }

    /* Can't find valid MPP */
    DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: can't find valid MPP", index);
    mLayerInfos[index]->mCheckOverlayFlag |= eInsufficientMPP;
    return false;

}

void ExynosDisplay::configureHandle(private_handle_t *handle, size_t index,
        hwc_layer_1_t &layer, int fence_fd, decon_win_config &cfg)
{
    int isOpaque = 0;

    if (handle == NULL)
        return;

    if ((layer.flags & HWC_SET_OPAQUE) && handle && (handle->format == HAL_PIXEL_FORMAT_RGBA_8888)
            && (layer.compositionType == HWC_OVERLAY)) {
        handle->format = HAL_PIXEL_FORMAT_RGBX_8888;
        isOpaque = 1;
    }
    hwc_frect_t &sourceCrop = layer.sourceCropf;
    hwc_rect_t &displayFrame = layer.compositionType == HWC_FRAMEBUFFER_TARGET ? mFbUpdateRegion : layer.displayFrame;
    int32_t blending = layer.blending;
    int32_t planeAlpha = layer.planeAlpha;
    uint32_t x, y;
    uint32_t w = WIDTH(displayFrame);
    uint32_t h = HEIGHT(displayFrame);
    uint8_t bpp = formatToBpp(handle->format);
    uint32_t offset = ((uint32_t)sourceCrop.top * handle->stride + (uint32_t)sourceCrop.left) * bpp / 8;
    ExynosMPPModule* internalMPP = mLayerInfos[index]->mInternalMPP;
    ExynosMPPModule* externalMPP = mLayerInfos[index]->mExternalMPP;

#ifdef USES_DECON_AFBC_DECODER
    cfg.compression = isCompressed(layer);
#endif

    if (displayFrame.left < 0) {
        unsigned int crop = -displayFrame.left;
        DISPLAY_LOGD(eDebugWinConfig, "layer off left side of screen; cropping %u pixels from left edge",
                crop);
        x = 0;
        w -= crop;
        offset += crop * bpp / 8;
    } else {
        x = displayFrame.left;
    }

    if (displayFrame.right > this->mXres) {
        unsigned int crop = displayFrame.right - this->mXres;
        DISPLAY_LOGD(eDebugWinConfig, "layer off right side of screen; cropping %u pixels from right edge",
                crop);
        w -= crop;
    }

    if (displayFrame.top < 0) {
        unsigned int crop = -displayFrame.top;
        DISPLAY_LOGD(eDebugWinConfig, "layer off top side of screen; cropping %u pixels from top edge",
                crop);
        y = 0;
        h -= crop;
        offset += handle->stride * crop * bpp / 8;
    } else {
        y = displayFrame.top;
    }

    if (displayFrame.bottom > this->mYres) {
        int crop = displayFrame.bottom - this->mYres;
        DISPLAY_LOGD(eDebugWinConfig, "layer off bottom side of screen; cropping %u pixels from bottom edge",
                crop);
        h -= crop;
    }

    cfg.fd_idma[0] = handle->fd;
    cfg.fd_idma[1] = handle->fd1;
    cfg.fd_idma[2] = handle->fd2;
    if (mLayerInfos[index]->mDmaType == -1) {
        cfg.state = cfg.DECON_WIN_STATE_DISABLED;
    } else {
        cfg.state = cfg.DECON_WIN_STATE_BUFFER;
        cfg.idma_type = (decon_idma_type)mLayerInfos[index]->mDmaType;
    }
    cfg.dst.x = x;
    cfg.dst.y = y;
    cfg.dst.w = w;
    cfg.dst.h = h;
    cfg.dst.f_w = mXres;
    cfg.dst.f_h = mYres;
    cfg.format = halFormatToS3CFormat(handle->format);

    cfg.src.f_w = handle->stride;
    cfg.src.f_h = handle->vstride;
    if (handle->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV) {
        if (handle->fd2 >= 0) {
            void *metaData = NULL;
            int interlacedType = -1;
            metaData = mmap(0, 64, PROT_READ|PROT_WRITE, MAP_SHARED, handle->fd2, 0);
            if (metaData)
                interlacedType = *(int *)metaData;
            else
                interlacedType = -1;

            if (interlacedType == V4L2_FIELD_INTERLACED_TB ||
                interlacedType == V4L2_FIELD_INTERLACED_BT) {
                cfg.src.f_w = handle->stride * 2;
                cfg.src.f_h = handle->vstride / 2;
            }
            if (metaData)
                munmap(metaData, 64);
        }
    }

    cfg.src.x = (int)sourceCrop.left;
    cfg.src.y = (int)sourceCrop.top;

    if (cfg.src.x < 0)
        cfg.src.x = 0;
    if (cfg.src.y < 0)
        cfg.src.y = 0;

    if (internalMPP != NULL) {
        if (cfg.src.f_w > (unsigned int)internalMPP->getMaxWidth(layer))
            cfg.src.f_w = (unsigned int)internalMPP->getMaxWidth(layer);
        if (cfg.src.f_h > (unsigned int)internalMPP->getMaxHeight(layer))
            cfg.src.f_h = (unsigned int)internalMPP->getMaxHeight(layer);
        cfg.src.f_w = ALIGN_DOWN((unsigned int)cfg.src.f_w, internalMPP->getSrcWidthAlign(layer));
        cfg.src.f_h = ALIGN_DOWN((unsigned int)cfg.src.f_h, internalMPP->getSrcHeightAlign(layer));

        cfg.src.x = ALIGN((unsigned int)sourceCrop.left, internalMPP->getSrcXOffsetAlign(layer));
        cfg.src.y = ALIGN((unsigned int)sourceCrop.top, internalMPP->getSrcYOffsetAlign(layer));
    }

    cfg.src.w = WIDTH(sourceCrop) - (cfg.src.x - (uint32_t)sourceCrop.left);
    if (cfg.src.x + cfg.src.w > cfg.src.f_w)
        cfg.src.w = cfg.src.f_w - cfg.src.x;
    cfg.src.h = HEIGHT(sourceCrop) - (cfg.src.y - (uint32_t)sourceCrop.top);
    if (cfg.src.y + cfg.src.h > cfg.src.f_h)
        cfg.src.h = cfg.src.f_h - cfg.src.y;

    if (internalMPP != NULL) {
        if (cfg.src.w > (unsigned int)internalMPP->getMaxCropWidth(layer))
            cfg.src.w = (unsigned int)internalMPP->getMaxCropWidth(layer);
        if (cfg.src.h > (unsigned int)internalMPP->getMaxCropHeight(layer))
            cfg.src.h = (unsigned int)internalMPP->getMaxCropHeight(layer);
        cfg.src.w = ALIGN_DOWN(cfg.src.w, internalMPP->getCropWidthAlign(layer));
        cfg.src.h = ALIGN_DOWN(cfg.src.h, internalMPP->getCropHeightAlign(layer));
    }

    if (isSrcCropFloat(layer.sourceCropf))
    {
        if (internalMPP != NULL) {
            exynos_mpp_img srcImg;
            internalMPP->adjustSourceImage(layer, srcImg);
            cfg.src.f_w = srcImg.fw;
            cfg.src.f_h = srcImg.fh;
            cfg.src.x = srcImg.x;
            cfg.src.y = srcImg.y;
            cfg.src.w = srcImg.w;
            cfg.src.h = srcImg.h;
        } else {
            if (externalMPP == NULL)
                ALOGE("float sourceCrop should be handled by MPP");
        }
#if 0
        ALOGD("x = %7.1f, 0x%8x", sourceCrop.left, cfg.src.x);
        ALOGD("y = %7.1f, 0x%8x", sourceCrop.top, cfg.src.y);
        ALOGD("w = %7.1f, 0x%8x", sourceCrop.right - sourceCrop.left, cfg.src.w);
        ALOGD("h = %7.1f, 0x%8x", sourceCrop.bottom -  sourceCrop.top, cfg.src.h);
#endif
    }

    cfg.blending = halBlendingToS3CBlending(blending);
    cfg.fence_fd = fence_fd;
    cfg.plane_alpha = 255;
    if (planeAlpha && (planeAlpha < 255)) {
        cfg.plane_alpha = planeAlpha;
    }
    if (mLayerInfos[index]->mInternalMPP) {
        cfg.vpp_parm.rot = (vpp_rotate)halTransformToHWRot(layer.transform);
        cfg.vpp_parm.eq_mode = isFullRangeColor(layer) ? BT_601_WIDE : BT_601_NARROW;

        if ((!mLayerInfos[index]->mExternalMPP &&
            (mHwc->mS3DMode == S3D_MODE_READY || mHwc->mS3DMode == S3D_MODE_RUNNING) &&
            !isFormatRgb(handle->format)) &&
            mType == EXYNOS_PRIMARY_DISPLAY) {
            int S3DFormat = getS3DFormat(mHwc->mHdmiPreset);
            if (S3DFormat == S3D_SBS)
                cfg.src.w /= 2;
            else if (S3DFormat == S3D_TB)
                cfg.src.h /= 2;
        }
    }
    /* transparent region coordinates is on source buffer */
    getLayerRegion(layer, cfg.transparent_area, eTransparentRegion);
    cfg.transparent_area.x += cfg.dst.x;
    cfg.transparent_area.y += cfg.dst.y;

    /* opaque region coordinates is on screen */
    getLayerRegion(layer, cfg.covered_opaque_area, eCoveredOpaqueRegion);

    if (isOpaque && (handle->format == HAL_PIXEL_FORMAT_RGBX_8888)) {
        handle->format = HAL_PIXEL_FORMAT_RGBA_8888;
        isOpaque = 0;
    }
}

void ExynosDisplay::configureOverlay(hwc_layer_1_t *layer, size_t index, decon_win_config &cfg)
{
    if (layer->compositionType == HWC_BACKGROUND) {
        hwc_color_t color = layer->backgroundColor;
        cfg.state = cfg.DECON_WIN_STATE_COLOR;
        cfg.color = (color.r << 16) | (color.g << 8) | color.b;
        cfg.dst.x = 0;
        cfg.dst.y = 0;
        cfg.dst.w = this->mXres;
        cfg.dst.h = this->mYres;
        return;
    }
    private_handle_t *handle = private_handle_t::dynamicCast(layer->handle);
    hwc_frect_t originalCrop = layer->sourceCropf;
    if (layer->compositionType == HWC_FRAMEBUFFER_TARGET) {
        /* Adjust FbUpdateRegion */
        int minCropWidth = 0;
        int minCropHeight = 0;
        int cropWidthAlign = 1;
        if (mLayerInfos[index]->mInternalMPP != NULL) {
            minCropWidth = mLayerInfos[index]->mInternalMPP->getMinWidth(*layer);
            minCropHeight = mLayerInfos[index]->mInternalMPP->getMinHeight(*layer);
            cropWidthAlign = mLayerInfos[index]->mInternalMPP->getCropWidthAlign(*layer);
        } else {
            getIDMAMinSize(*layer, &minCropWidth, &minCropHeight);
        }
#if defined(USES_DUAL_DISPLAY)
        int32_t minLeftPosition = (mType != EXYNOS_SECONDARY_DISPLAY)? 0:(mXres/2);
        int32_t maxRightPosition = (mType == EXYNOS_PRIMARY_DISPLAY)?(mXres/2):mXres;
#else
        int32_t minLeftPosition = 0;
        int32_t maxRightPosition = mXres;
#endif
        if (mFbUpdateRegion.left < minLeftPosition) mFbUpdateRegion.left = minLeftPosition;
        if (mFbUpdateRegion.right < minLeftPosition) mFbUpdateRegion.right = minLeftPosition;
        if (mFbUpdateRegion.left > maxRightPosition) mFbUpdateRegion.left = maxRightPosition;
        if (mFbUpdateRegion.right > maxRightPosition) mFbUpdateRegion.right = maxRightPosition;
        if (mFbUpdateRegion.top < 0) mFbUpdateRegion.top  = 0;
        if (mFbUpdateRegion.bottom < 0) mFbUpdateRegion.bottom = 0;
        if (mFbUpdateRegion.top > mYres) mFbUpdateRegion.top = mYres;
        if (mFbUpdateRegion.bottom > mYres) mFbUpdateRegion.bottom = mYres;

        if ((WIDTH(mFbUpdateRegion) % cropWidthAlign) != 0) {
            mFbUpdateRegion.left = ALIGN_DOWN(mFbUpdateRegion.left, cropWidthAlign);
            mFbUpdateRegion.right = ALIGN_UP(mFbUpdateRegion.right, cropWidthAlign);
        }
        if (WIDTH(mFbUpdateRegion) < minCropWidth) {
#if defined(USES_DUAL_DISPLAY)
            if (mFbUpdateRegion.left + minCropWidth <= maxRightPosition)
                mFbUpdateRegion.right = mFbUpdateRegion.left + minCropWidth;
            else
                mFbUpdateRegion.left = mFbUpdateRegion.right - minCropWidth;
#else
            if (mFbUpdateRegion.left + minCropWidth <= mXres)
                mFbUpdateRegion.right = mFbUpdateRegion.left + minCropWidth;
            else
                mFbUpdateRegion.left = mFbUpdateRegion.right - minCropWidth;
#endif
        }
        if (HEIGHT(mFbUpdateRegion) < minCropHeight) {
            if (mFbUpdateRegion.top + minCropHeight <= mYres)
                mFbUpdateRegion.bottom = mFbUpdateRegion.top + minCropHeight;
            else
                mFbUpdateRegion.top = mFbUpdateRegion.bottom - minCropHeight;
        }

        if ((mFbUpdateRegion.left >= minLeftPosition) && (mFbUpdateRegion.top >= 0) &&
            (mFbUpdateRegion.right <= maxRightPosition) && (mFbUpdateRegion.bottom <= mYres)) {
#ifdef USES_DUAL_DISPLAY
            if (mType == EXYNOS_SECONDARY_DISPLAY) {
                layer->sourceCropf.left = (double)mFbUpdateRegion.left - (mXres/2);
                layer->sourceCropf.right = (double)mFbUpdateRegion.right - (mXres/2);
            } else {
                layer->sourceCropf.left = (double)mFbUpdateRegion.left;
                layer->sourceCropf.right = (double)mFbUpdateRegion.right;
            }
#else
            layer->sourceCropf.left = (double)mFbUpdateRegion.left;
            layer->sourceCropf.right = (double)mFbUpdateRegion.right;
#endif
            layer->sourceCropf.top = (double)mFbUpdateRegion.top;
            layer->sourceCropf.bottom = (double)mFbUpdateRegion.bottom;
        } else {
            mFbUpdateRegion = layer->displayFrame;
        }
    }
    configureHandle(handle, index, *layer, layer->acquireFenceFd, cfg);
    layer->sourceCropf = originalCrop;
}

int ExynosDisplay::handleWindowUpdate(hwc_display_contents_1_t __unused *contents,
    struct decon_win_config __unused *config)
{
    int layerIdx = -1;
    int updatedWinCnt = 0;
    int totalWinCnt = 0;
    int bitsPerPixel = 0;
    size_t winUpdateInfoIdx;
    hwc_rect updateRect = {this->mXres, this->mYres, 0, 0};
    hwc_rect currentRect = {0, 0, 0, 0};
    bool burstLengthCheckDone = false;
    int alignAdjustment = 1;
    int intersectionWidth = 0;
    int xAlign = 0;
    int wAlign = 0;
    int yAlign = 0;
    int hAlign = 0;

#if defined(USES_DUAL_DISPLAY)
    return -eWindowUpdateDisabled;
#endif


    char value[PROPERTY_VALUE_MAX];
    property_get("debug.hwc.winupdate", value, NULL);

    if (!(!strcmp(value, "1") || !strcmp(value, "true")))
        return -eWindowUpdateDisabled;

    if (DECON_WIN_UPDATE_IDX < 0)
        return -eWindowUpdateInvalidIndex;
    winUpdateInfoIdx = DECON_WIN_UPDATE_IDX;

    if (contents->flags & HWC_GEOMETRY_CHANGED)
        return -eWindowUpdateGeometryChanged;

    if (mPanelType == PANEL_DSC) {
        xAlign = this->mXres / mDSCHSliceNum;
        wAlign = this->mXres / mDSCHSliceNum;
        yAlign = mDSCYSliceSize;
        hAlign = mDSCYSliceSize;
    } else {
        xAlign = WINUPDATE_X_ALIGNMENT;
        wAlign = WINUPDATE_W_ALIGNMENT;
        yAlign = 1;
        hAlign = 1;
    }

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        if (contents->hwLayers[i].compositionType == HWC_FRAMEBUFFER)
            continue;

        if (!mFbNeeded && contents->hwLayers[i].compositionType == HWC_FRAMEBUFFER_TARGET)
            continue;
        int32_t windowIndex = mLayerInfos[i]->mWindowIndex;
        if ((windowIndex < 0) || (windowIndex > MAX_DECON_WIN))
            return -eWindowUpdateInvalidConfig;

        if (config[windowIndex].state != config[windowIndex].DECON_WIN_STATE_DISABLED) {
            totalWinCnt++;

            if (winConfigChanged(&config[windowIndex], &this->mLastConfigData.config[windowIndex])) {
                updatedWinCnt++;

                currentRect.left   = config[windowIndex].dst.x;
                currentRect.right  = config[windowIndex].dst.x + config[windowIndex].dst.w;
                currentRect.top    = config[windowIndex].dst.y;
                currentRect.bottom = config[windowIndex].dst.y + config[windowIndex].dst.h;

                if (hwcHasApiVersion((hwc_composer_device_1_t*)mHwc, HWC_DEVICE_API_VERSION_1_5))
                {
                    private_handle_t *handle = NULL;
                    hwc_rect damageRect = {0, 0, 0, 0};
                    hwc_layer_1_t &layer = contents->hwLayers[i];
                    if (layer.handle)
                        handle = private_handle_t::dynamicCast(layer.handle);
                    unsigned int damageRegionMod = getLayerRegion(layer, damageRect, eDamageRegion);

                    if (damageRegionMod == eDamageRegionSkip)
                        continue;

                    if (handle && !isScaled(layer) && !isRotated(layer) && (damageRegionMod == eDamageRegionPartial)) {
                        DISPLAY_LOGD(eDebugWindowUpdate, "[WIN_UPDATE][surfaceDamage]  layer w(%4d) h(%4d),  dirty (%4d, %4d) - (%4d, %4d)",
                                handle->width, handle->height, damageRect.left, damageRect.top, damageRect.right, damageRect.bottom);

                        currentRect.left   = config[windowIndex].dst.x - (int32_t)layer.sourceCropf.left + damageRect.left;
                        currentRect.right  = config[windowIndex].dst.x - (int32_t)layer.sourceCropf.left + damageRect.right;
                        currentRect.top    = config[windowIndex].dst.y - (int32_t)layer.sourceCropf.top  + damageRect.top;
                        currentRect.bottom = config[windowIndex].dst.y - (int32_t)layer.sourceCropf.top  + damageRect.bottom;
                        adjustRect(currentRect, mXres, mYres);

                    }
                }

                if ((currentRect.left > currentRect.right) || (currentRect.top > currentRect.bottom)) {
                    DISPLAY_LOGD(eDebugWindowUpdate, "[WIN_UPDATE] window(%d) layer(%d) invalid region (%4d, %4d) - (%4d, %4d)",
                        i, layerIdx, currentRect.left, currentRect.top, currentRect.right, currentRect.bottom);
                    return -eWindowUpdateInvalidRegion;
                }
                DISPLAY_LOGD(eDebugWindowUpdate, "[WIN_UPDATE] Updated Window(%d) Layer(%d)  (%4d, %4d) - (%4d, %4d)",
                    windowIndex, i, currentRect.left, currentRect.top, currentRect.right, currentRect.bottom);
                updateRect = expand(updateRect, currentRect);
            }
        }
    }
    if (updatedWinCnt == 0)
        return -eWindowUpdateNotUpdated;

    /* Alignment check */
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        if (contents->hwLayers[i].compositionType == HWC_FRAMEBUFFER)
            continue;

        if (!mFbNeeded && contents->hwLayers[i].compositionType == HWC_FRAMEBUFFER_TARGET)
            continue;

        int32_t windowIndex = mLayerInfos[i]->mWindowIndex;
        currentRect.left   = config[windowIndex].dst.x;
        currentRect.right  = config[windowIndex].dst.x + config[windowIndex].dst.w;
        currentRect.top    = config[windowIndex].dst.y;
        currentRect.bottom = config[windowIndex].dst.y + config[windowIndex].dst.h;

        if ((config[windowIndex].state != config[windowIndex].DECON_WIN_STATE_DISABLED) &&
            intersect(currentRect, updateRect)) {
            private_handle_t *handle = NULL;
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (layer.handle)
                handle = private_handle_t::dynamicCast(layer.handle);
            else
                return -eWindowUpdateInvalidConfig;

            int originalFormat = handle->format;
            int originalTransform = layer.transform;
            if (mLayerInfos[i]->mInternalMPP != NULL) {
                /* VPP scaling case */
                if ((config[windowIndex].src.w != config[windowIndex].dst.w) ||
                    (config[windowIndex].src.h != config[windowIndex].dst.h))
                    return -eWindowUpdateUnsupportedUseCase;

                handle->format = S3CFormatToHalFormat(config[windowIndex].format);
                if (handle->format >= 0) {
                    /* rotation was handled by externalMPP */
                    if (mLayerInfos[i]->mExternalMPP != NULL)
                        layer.transform = 0;
                    xAlign = getLCM(xAlign, mLayerInfos[i]->mInternalMPP->getSrcXOffsetAlign(layer));
                    yAlign = getLCM(yAlign, mLayerInfos[i]->mInternalMPP->getSrcYOffsetAlign(layer));
                    wAlign = getLCM(wAlign, mLayerInfos[i]->mInternalMPP->getCropWidthAlign(layer));
                    hAlign = getLCM(hAlign, mLayerInfos[i]->mInternalMPP->getCropHeightAlign(layer));
                } else {
                    handle->format = originalFormat;
                    layer.transform = originalTransform;
                    return -eWindowUpdateInvalidConfig;
                }
            }
            handle->format = originalFormat;
            layer.transform = originalTransform;
        }
    }

    updateRect.left   = ALIGN_DOWN(updateRect.left, xAlign);
    updateRect.top    = ALIGN_DOWN(updateRect.top, yAlign);

    if (HEIGHT(updateRect) < WINUPDATE_MIN_HEIGHT) {
        if (updateRect.top + WINUPDATE_MIN_HEIGHT <= mYres)
            updateRect.bottom = updateRect.top + WINUPDATE_MIN_HEIGHT;
        else
            updateRect.top = updateRect.bottom - WINUPDATE_MIN_HEIGHT;
    }

    if ((100 * (WIDTH(updateRect) * HEIGHT(updateRect)) / (this->mXres * this->mYres)) > WINUPDATE_THRESHOLD)
        return -eWindowUpdateOverThreshold;

    alignAdjustment = getLCM(alignAdjustment, xAlign);
    alignAdjustment = getLCM(alignAdjustment, wAlign);

    while (1) {
        burstLengthCheckDone = true;
        updateRect.left = ALIGN_DOWN(updateRect.left, xAlign);
        if ((WIDTH(updateRect) % wAlign) != 0)
            updateRect.right  = updateRect.left + ALIGN_DOWN(WIDTH(updateRect), wAlign) + wAlign;
        updateRect.top = ALIGN_DOWN(updateRect.top, yAlign);
        if ((HEIGHT(updateRect) % hAlign) != 0)
            updateRect.bottom  = updateRect.top + ALIGN_DOWN(HEIGHT(updateRect), hAlign) + hAlign;

        for (size_t i = 0; i < contents->numHwLayers; i++) {
            if (contents->hwLayers[i].compositionType == HWC_FRAMEBUFFER)
                continue;
            if (!mFbNeeded && contents->hwLayers[i].compositionType == HWC_FRAMEBUFFER_TARGET)
                continue;
            int32_t windowIndex = mLayerInfos[i]->mWindowIndex;
            if (config[windowIndex].state != config[windowIndex].DECON_WIN_STATE_DISABLED) {
                enum decon_pixel_format fmt = config[windowIndex].format;
                if (fmt == DECON_PIXEL_FORMAT_RGBA_5551 || fmt == DECON_PIXEL_FORMAT_RGB_565)
                    bitsPerPixel = 16;
                else if (fmt == DECON_PIXEL_FORMAT_NV12 || fmt == DECON_PIXEL_FORMAT_NV21 ||
                        fmt == DECON_PIXEL_FORMAT_NV12M || fmt == DECON_PIXEL_FORMAT_NV21M)
                    bitsPerPixel = 12;
                else
                    bitsPerPixel = 32;

                currentRect.left   = config[windowIndex].dst.x;
                currentRect.right  = config[windowIndex].dst.x + config[windowIndex].dst.w;
                currentRect.top    = config[windowIndex].dst.y;
                currentRect.bottom = config[windowIndex].dst.y + config[windowIndex].dst.h;

                intersectionWidth = WIDTH(intersection(currentRect, updateRect));

                DISPLAY_LOGD(eDebugWindowUpdate, "[WIN_UPDATE] win[%d] left(%d) right(%d) intersection(%d)", windowIndex, currentRect.left, currentRect.right, intersectionWidth);

                if (intersectionWidth != 0 && (size_t)((intersectionWidth * bitsPerPixel) / 8) < BURSTLEN_BYTES) {
#ifdef USE_DRM_BURST_LEN
                    if (mHasDrmSurface) {
                        if ((size_t)((intersectionWidth * bitsPerPixel) / 8) < DRM_BURSTLEN_BYTES) {
                            DISPLAY_LOGD(eDebugWindowUpdate, "[WIN_UPDATE] win[%d] insufficient burst length (%d)*(%d) < %d", windowIndex, intersectionWidth, bitsPerPixel, BURSTLEN_BYTES);
                            burstLengthCheckDone = false;
                            break;

                        }
                    } else {
#endif
                        DISPLAY_LOGD(eDebugWindowUpdate, "[WIN_UPDATE] win[%d] insufficient burst length (%d)*(%d) < %d", windowIndex, intersectionWidth, bitsPerPixel, BURSTLEN_BYTES);
                        burstLengthCheckDone = false;
                        break;
#ifdef USE_DRM_BURST_LEN
                    }
#endif
                }
            }
        }

        if (burstLengthCheckDone)
            break;
        DISPLAY_LOGD(eDebugWindowUpdate, "[WIN_UPDATE] Adjusting update width. current left(%d) right(%d)", updateRect.left, updateRect.right);
        if (updateRect.left >= alignAdjustment) {
            updateRect.left -= alignAdjustment;
        } else if (updateRect.right + alignAdjustment <= this->mXres) {
            updateRect.right += alignAdjustment;
        } else {
            DISPLAY_LOGD(eDebugWindowUpdate, "[WIN_UPDATE] Error during update width adjustment");
            return -eWindowUpdateAdjustmentFail;
        }
    }

    config[winUpdateInfoIdx].state = config[winUpdateInfoIdx].DECON_WIN_STATE_UPDATE;
    config[winUpdateInfoIdx].dst.x = ALIGN_DOWN(updateRect.left, xAlign);
    if ((WIDTH(updateRect) % wAlign) != 0)
        updateRect.right  = updateRect.left + ALIGN_DOWN(WIDTH(updateRect), wAlign) + wAlign;
    config[winUpdateInfoIdx].dst.w = WIDTH(updateRect);

    config[winUpdateInfoIdx].dst.y = ALIGN_DOWN(updateRect.top, yAlign);
    if ((HEIGHT(updateRect) % hAlign) != 0)
        updateRect.bottom  = updateRect.top + ALIGN_DOWN(HEIGHT(updateRect), hAlign) + hAlign;
    config[winUpdateInfoIdx].dst.h = HEIGHT(updateRect);

    /* Final check */
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        if (contents->hwLayers[i].compositionType == HWC_FRAMEBUFFER)
            continue;

        if (!mFbNeeded && contents->hwLayers[i].compositionType == HWC_FRAMEBUFFER_TARGET)
            continue;

        int32_t windowIndex = mLayerInfos[i]->mWindowIndex;
        currentRect.left   = config[windowIndex].dst.x;
        currentRect.right  = config[windowIndex].dst.x + config[windowIndex].dst.w;
        currentRect.top    = config[windowIndex].dst.y;
        currentRect.bottom = config[windowIndex].dst.y + config[windowIndex].dst.h;

        if ((config[windowIndex].state != config[windowIndex].DECON_WIN_STATE_DISABLED) &&
            intersect(currentRect, updateRect)) {
            private_handle_t *handle = NULL;
            hwc_rect intersect_rect = intersection(currentRect, updateRect);
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (layer.handle)
                handle = private_handle_t::dynamicCast(layer.handle);
            else
                return -eWindowUpdateInvalidConfig;

            int originalFormat = handle->format;
            int originalTransform = layer.transform;
            if (mLayerInfos[i]->mInternalMPP != NULL) {
                handle->format = S3CFormatToHalFormat(config[windowIndex].format);
                /* rotation was handled by externalMPP */
                if (mLayerInfos[i]->mExternalMPP != NULL)
                    layer.transform = 0;
                if (((mLayerInfos[i]->mInternalMPP->getSrcXOffsetAlign(layer) % intersect_rect.left) != 0) ||
                    ((mLayerInfos[i]->mInternalMPP->getSrcYOffsetAlign(layer) % intersect_rect.top) != 0) ||
                    ((mLayerInfos[i]->mInternalMPP->getCropWidthAlign(layer) % WIDTH(intersect_rect)) != 0) ||
                    ((mLayerInfos[i]->mInternalMPP->getCropHeightAlign(layer) % HEIGHT(intersect_rect)) != 0)) {
                    handle->format = originalFormat;
                    layer.transform = originalTransform;
                    config[winUpdateInfoIdx].state = config[winUpdateInfoIdx].DECON_WIN_STATE_DISABLED;
                    return -eWindowUpdateAdjustmentFail;
                }
            }
            handle->format = originalFormat;
            layer.transform = originalTransform;
        }
    }

    DISPLAY_LOGD(eDebugWindowUpdate, "[WIN_UPDATE] UpdateRegion cfg  (%4d, %4d) w(%4d) h(%4d) updatedWindowCnt(%d)",
        config[winUpdateInfoIdx].dst.x, config[winUpdateInfoIdx].dst.y, config[winUpdateInfoIdx].dst.w, config[winUpdateInfoIdx].dst.h, updatedWinCnt);

    /* Disable block mode if window update region is not full screen */
    if ((config[winUpdateInfoIdx].dst.x != 0) || (config[winUpdateInfoIdx].dst.y != 0) ||
        (config[winUpdateInfoIdx].dst.w != (uint32_t)mXres) || (config[winUpdateInfoIdx].dst.h != (uint32_t)mXres)) {
        for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
            memset(&config[i].transparent_area, 0, sizeof(config[i].transparent_area));
            memset(&config[i].covered_opaque_area, 0, sizeof(config[i].covered_opaque_area));
        }
    }

    return 1;
}

void ExynosDisplay::getLayerRegion(hwc_layer_1_t &layer, decon_win_rect &rect_area, uint32_t regionType)
{
    hwc_rect_t const *hwcRects = NULL;
    unsigned int numRects = 0;
    switch (regionType) {
    case eTransparentRegion:
        hwcRects = layer.transparentRegion.rects;
        numRects = layer.transparentRegion.numRects;
        break;
    case eCoveredOpaqueRegion:
        hwcRects = layer.coveredOpaqueRegion.rects;
        numRects = layer.coveredOpaqueRegion.numRects;
        break;
    default:
        ALOGE("%s:: Invalid regionType (%d)", __func__, regionType);
        return;
    }

    rect_area.x = rect_area.y = rect_area.w = rect_area.h = 0;
    if (hwcRects != NULL) {
        for (size_t j = 0; j < numRects; j++) {
            hwc_rect_t rect;
            rect.left = hwcRects[j].left;
            rect.top = hwcRects[j].top;
            rect.right = hwcRects[j].right;
            rect.bottom = hwcRects[j].bottom;
            adjustRect(rect, mXres, mYres);
            /* Find the largest rect */
            if ((rect_area.w * rect_area.h) <
                (uint32_t)(WIDTH(rect) * HEIGHT(rect))) {
                rect_area.x = rect.left;
                rect_area.y = rect.top;
                rect_area.w = WIDTH(rect);
                rect_area.h = HEIGHT(rect);
            }
        }
    }
}

unsigned int ExynosDisplay::getLayerRegion(hwc_layer_1_t &layer, hwc_rect &rect_area, uint32_t regionType) {
    hwc_rect_t const *hwcRects = NULL;
    unsigned int numRects = 0;

    switch (regionType) {
    case eDamageRegion:
        hwcRects = layer.surfaceDamage.rects;
        numRects = layer.surfaceDamage.numRects;
        break;
    default:
        ALOGE("%s:: Invalid regionType (%d)", __func__, regionType);
        return eDamageRegionError;
    }

    if ((numRects == 0) || (hwcRects == NULL))
        return eDamageRegionFull;

    if ((numRects == 1) && (hwcRects[0].left == 0) && (hwcRects[0].top == 0) &&
        (hwcRects[0].right == 0) && (hwcRects[0].bottom == 0))
        return eDamageRegionSkip;

    rect_area.left = INT_MAX;
    rect_area.top = INT_MAX;
    rect_area.right = rect_area.bottom = 0;
    if (hwcRects != NULL) {
        for (size_t j = 0; j < numRects; j++) {
            hwc_rect_t rect;
            rect.left = hwcRects[j].left;
            rect.top = hwcRects[j].top;
            rect.right = hwcRects[j].right;
            rect.bottom = hwcRects[j].bottom;
            adjustRect(rect, INT_MAX, INT_MAX);
            /* Get sums of rects */
            rect_area = expand(rect_area, rect);
        }
    }

    return eDamageRegionPartial;
}

bool ExynosDisplay::getPreviousDRMDMA(int *dma)
{
    *dma = -1;

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        if (mLastConfigData.config[i].protection == 1) {
            *dma = (int)mLastConfigData.config[i].idma_type;
            return true;
        }
    }
    return false;
}

int ExynosDisplay::winconfigIoctl(decon_win_config_data *win_data)
{
    ATRACE_CALL();
    return ioctl(this->mDisplayFd, S3CFB_WIN_CONFIG, win_data);
}

int ExynosDisplay::postFrame(hwc_display_contents_1_t* contents)
{
    ATRACE_CALL();

    if (mWinData == NULL) {
        DISPLAY_LOGE("mWinData is not valid");
        return -1;
    }
    struct decon_win_config *config = mWinData->config;
    int win_map = 0;
    int tot_ovly_wins = 0;
    uint32_t rectCount = 0;
    int ret = 0;

    memset(mLastHandles, 0, sizeof(mLastHandles));
    memset(mLastMPPMap, 0, sizeof(mLastMPPMap));
    memset(config, 0, sizeof(mWinData->config));

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        config[i].fence_fd = -1;
        mLastMPPMap[i].internal_mpp.type = -1;
        mLastMPPMap[i].external_mpp.type = -1;
    }
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
            if (handle->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV
                    && rectCount < mBackUpFrect.size())
                layer.sourceCropf = mBackUpFrect[rectCount++];
        }
    }

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        int32_t window_index = mLayerInfos[i]->mWindowIndex;
        private_handle_t *handle = NULL;
        if (layer.handle)
            handle = private_handle_t::dynamicCast(layer.handle);

        if ((layer.flags & HWC_SKIP_RENDERING) ||
            ((layer.compositionType == HWC_OVERLAY) &&
             ((window_index < 0) || (window_index >= NUM_HW_WINDOWS)))) {
            if (layer.acquireFenceFd >= 0)
                close(layer.acquireFenceFd);
            layer.acquireFenceFd = -1;
            layer.releaseFenceFd = -1;

            if ((window_index < 0) || (window_index >= NUM_HW_WINDOWS)) {
                android::String8 result;
                DISPLAY_LOGE("window of layer %d was not assigned (window_index: %d)", i, window_index);
                dumpContents(result, contents);
                ALOGE(result.string());
                result.clear();
                dumpLayerInfo(result);
                ALOGE(result.string());
            }
            continue;
        }

        if ((mVirtualOverlayFlag == true) && (layer.compositionType == HWC_OVERLAY) &&
            handle && (getDrmMode(handle->flags) == NO_DRM) &&
            (mFirstFb <= i) && (i <= mLastFb))
            continue;

        if (((layer.compositionType == HWC_OVERLAY) ||
            (mFbNeeded == true &&  layer.compositionType == HWC_FRAMEBUFFER_TARGET))) {
            mLastHandles[window_index] = layer.handle;

            if (handle && (getDrmMode(handle->flags) == SECURE_DRM))
                config[window_index].protection = 1;
            else
                config[window_index].protection = 0;

            if (mLayerInfos[i]->mInternalMPP != NULL) {
                mLastMPPMap[window_index].internal_mpp.type = mLayerInfos[i]->mInternalMPP->mType;
                mLastMPPMap[window_index].internal_mpp.index = mLayerInfos[i]->mInternalMPP->mIndex;
            }
            if (mLayerInfos[i]->mExternalMPP != NULL) {
                mLastMPPMap[window_index].external_mpp.type = mLayerInfos[i]->mExternalMPP->mType;
                mLastMPPMap[window_index].external_mpp.index = mLayerInfos[i]->mExternalMPP->mIndex;
                if (postMPPM2M(layer, config, window_index, i) < 0)
                    continue;
            } else {
                configureOverlay(&layer, i, config[window_index]);
            }
        }
        if (window_index == 0 && config[window_index].blending != DECON_BLENDING_NONE) {
            DISPLAY_LOGD(eDebugWinConfig, "blending not supported on window 0; forcing BLENDING_NONE");
            config[window_index].blending = DECON_BLENDING_NONE;
        }
        if ((window_index < DECON_WIN_UPDATE_IDX) &&
                (config[window_index].state != config[window_index].DECON_WIN_STATE_DISABLED) &&
                (config[window_index].src.w == 0 || config[window_index].src.h == 0 ||
                 config[window_index].dst.w == 0 || config[window_index].dst.h == 0)) {
            config[window_index].state = config[window_index].DECON_WIN_STATE_DISABLED;
        }
    }

    if (this->mVirtualOverlayFlag) {
        handleStaticLayers(contents, *mWinData, tot_ovly_wins);
    }

    if ((ret = handleWindowUpdate(contents, config)) < 0)
        DISPLAY_LOGD(eDebugWindowUpdate, "[WIN_UPDATE] UpdateRegion is FullScreen, ret(%d)", ret);

#if defined(USES_DUAL_DISPLAY)
    if (mType == EXYNOS_PRIMARY_DISPLAY) {
        int8_t indexLastConfig = 0;
        for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
            if (config[i].state == config[i].DECON_WIN_STATE_DISABLED) {
                indexLastConfig = i;
                break;
            }
        }
        if (mHwc->secondaryDisplay->mEnabled) {
            for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
                struct decon_win_config &secondary_config = mHwc->secondaryDisplay->mLastConfigData.config[i];
                int8_t index = 0;
                if (indexLastConfig == NUM_HW_WINDOWS) {
                    DISPLAY_LOGE("primaryDisplay last config index is not valid(primaryLastIndex: %d)",
                            indexLastConfig);
                    break;
                }
                if (i == (NUM_HW_WINDOWS - 1))
                    index = i;
                else
                    index = indexLastConfig + i;
                DISPLAY_LOGD(eDebugWinConfig, "secondary_config window %u configuration:", i);
                dumpConfig(secondary_config);
                if (secondary_config.state != secondary_config.DECON_WIN_STATE_DISABLED) {
                    if (index >= NUM_HW_WINDOWS) {
                        DISPLAY_LOGE("secondaryDisplay config index is not valid(primaryLastIndex: %d, index:%d",
                                indexLastConfig, index);
                    } else {
                        memcpy(&config[index],&secondary_config, sizeof(struct decon_win_config));
                        mLastHandles[index] = mHwc->secondaryDisplay->mLastHandles[i];
                    }
                }
            }
        }
    }

    if (mType != EXYNOS_SECONDARY_DISPLAY)
    {
#endif
        for (size_t i = 0; i <= NUM_HW_WINDOWS; i++) {
            DISPLAY_LOGD(eDebugWinConfig, "window %u configuration:", i);
            dumpConfig(config[i]);
        }

        if (checkConfigValidation(config) < 0) {
                android::String8 result;
                DISPLAY_LOGE("WIN_CONFIG is not valid");
                for (size_t i = 0; i <= MAX_DECON_WIN; i++) {
                    result.appendFormat("window %zu configuration:\n", i);
                    dumpConfig(config[i], result);
                }
                ALOGE(result.string());
                result.clear();
                dumpContents(result, contents);
                ALOGE(result.string());
                result.clear();
                dumpLayerInfo(result);
                ALOGE(result.string());
        }

        if (checkConfigChanged(*mWinData, mLastConfigData) == false) {
            ret = 0;
        } else {
            ret = winconfigIoctl(mWinData);
            if (ret < 0) {
                DISPLAY_LOGE("ioctl S3CFB_WIN_CONFIG failed: %s", strerror(errno));
            } else {
                ret = mWinData->fence;
                memcpy(&(this->mLastConfigData), mWinData, sizeof(*mWinData));
            }
        }

        /*
         * Acquire fence of all of OVERLAY layers(including layers for secondary LCD)
         * should be closed even if WIN_CONFIG is skipped
         */
        for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
            if (config[i].fence_fd != -1)
                close(config[i].fence_fd);
        }
#if defined(USES_DUAL_DISPLAY)
    } else {
        memcpy(&(this->mLastConfigData), mWinData, sizeof(*mWinData));
    }
#endif
    if (contents->numHwLayers == 1) {
        hwc_layer_1_t &layer = contents->hwLayers[0];
        if (layer.acquireFenceFd >= 0)
            close(layer.acquireFenceFd);
        layer.acquireFenceFd = -1;
        layer.releaseFenceFd = -1;
    }
    rectCount = 0;
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
            if (handle->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV
                    && rectCount < mOriginFrect.size())
                layer.sourceCropf = mOriginFrect[rectCount++];
        }
    }
    mOriginFrect.clear();
    mBackUpFrect.clear();

    if (!this->mVirtualOverlayFlag && (ret >= 0))
        this->mLastFbWindow = mFbWindow;

    return ret;
}

void ExynosDisplay::skipStaticLayers(hwc_display_contents_1_t* contents)
{
    mVirtualOverlayFlag = 0;
    int win_map = 0;
    int fbIndex = contents->numHwLayers - 1;

    if (!mHwc->hwc_ctrl.skip_static_layer_mode)
        return;

    if (mBypassSkipStaticLayer)
        return;

    if (contents->flags & HWC_GEOMETRY_CHANGED) {
        mSkipStaticInitFlag = false;
        return;
    }

    if (!mFbNeeded || ((mLastFb - mFirstFb + 1) > NUM_VIRT_OVER)) {
        mSkipStaticInitFlag = false;
        return;
    }

    if (mSkipStaticInitFlag) {
        if (mNumStaticLayers != (mLastFb - mFirstFb + 1)) {
            mSkipStaticInitFlag = false;
            return;
        }

        for (size_t i = mFirstFb; i <= mLastFb; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (!layer.handle || (layer.flags & HWC_SKIP_LAYER) || (mLastLayerHandles[i - mFirstFb] !=  layer.handle)) {
                mSkipStaticInitFlag = false;
                return;
            }
        }

        if ((mLastFbWindow >= NUM_HW_WINDOWS) || (fbIndex < 0)) {
            mSkipStaticInitFlag = false;
            DISPLAY_LOGE("skipStaticLayers:: invalid mLastFbWindow(%d), fbIndex(%d)", mLastFbWindow, fbIndex);
            return;
        }
        /* DMA mapping is changed */
        if (mLastConfigData.config[mLastFbWindow].idma_type != mLayerInfos[fbIndex]->mDmaType) {
            mSkipStaticInitFlag = false;
            return;
        }

        mVirtualOverlayFlag = 1;
        for (size_t i = 0; i < contents->numHwLayers-1; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (layer.compositionType == HWC_FRAMEBUFFER) {
                layer.compositionType = HWC_OVERLAY;
                mLayerInfos[i]->compositionType = layer.compositionType;
                mLayerInfos[i]->mCheckOverlayFlag |= eSkipStaticLayer;
            }
        }
        mLastFbWindow = mFbWindow;
        return;
    }

    mSkipStaticInitFlag = true;
    for (size_t i = 0; i < NUM_VIRT_OVER; i++)
        mLastLayerHandles[i] = 0;

    for (size_t i = mFirstFb; i <= mLastFb; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        mLastLayerHandles[i - mFirstFb] = layer.handle;
    }
    mNumStaticLayers = (mLastFb - mFirstFb + 1);

    return;
}

void ExynosDisplay::dumpMPPs(android::String8& result)
{
    result.appendFormat("displayType(%d)\n", mType);
    result.appendFormat("Internal MPPs number: %zu\n", mInternalMPPs.size());
    result.append(
            " mType | mIndex | mState \n"
            "-------+--------+-----------\n");
        //    5____ | 6_____ | 9________ \n

    for (size_t i = 0; i < mInternalMPPs.size(); i++) {
        ExynosMPPModule* internalMPP = mInternalMPPs[i];
        result.appendFormat(" %5d | %6d | %9d \n",
                internalMPP->mType, internalMPP->mIndex, internalMPP->mState);
    }

    result.append("\n");
    result.appendFormat("External MPPs number: %zu\n", mExternalMPPs.size());
    result.append(
            " mType | mIndex | mState \n"
            "-------+--------+-----------\n");
        //    5____ | 6_____ | 9________ \n

    for (size_t i = 0; i < mExternalMPPs.size(); i++) {
        ExynosMPPModule* internalMPP = mExternalMPPs[i];
        result.appendFormat(" %5d | %6d | %9d \n",
                internalMPP->mType, internalMPP->mIndex, internalMPP->mState);
    }
}

void ExynosDisplay::preAssignFbTarget(hwc_display_contents_1_t *contents, bool assign)
{
    ExynosMPPModule* supportedInternalMPP = NULL;
    ExynosMPPModule* supportedExternalMPP = NULL;

    int fbIndex = contents->numHwLayers - 1;
    hwc_layer_1_t &layer = contents->hwLayers[fbIndex];
    mFbPreAssigned = false;

    if (!assign)
        return;

    if (layer.compositionType != HWC_FRAMEBUFFER_TARGET) {
        ALOGE("preAssignFbTarget: FRAMEBUFFER_TARGET is not set properly");
        return;
    }

    bool ret = isOverlaySupported(layer, fbIndex, true, &supportedInternalMPP, &supportedExternalMPP);
    if (ret && (supportedInternalMPP != NULL) && (supportedExternalMPP == NULL)) {
        DISPLAY_LOGD(eDebugResourceAssigning, "Preassigning FramebufferTarget with internalMPP(%d, %d)", supportedInternalMPP->mType, supportedInternalMPP->mIndex);
        supportedInternalMPP->mState = MPP_STATE_ASSIGNED;
        mLayerInfos[fbIndex]->mInternalMPP = supportedInternalMPP;
        mLayerInfos[fbIndex]->mDmaType = getDeconDMAType(mLayerInfos[fbIndex]->mInternalMPP);
        for (size_t i = 0; i < mInternalMPPs.size(); i++) {
            if ((ExynosMPPModule *)mInternalMPPs[i] == supportedInternalMPP) {
                mInternalMPPs.removeItemsAt(i);
            }
        }
        mFbPreAssigned = true;
    } else {
        ALOGE("preAssignFbTarget: preassigning FB failed");
        return;
    }
}

void ExynosDisplay::determineYuvOverlay(hwc_display_contents_1_t *contents)
{
    mYuvLayers = 0;
    bool useVPPOverlayFlag = false, hasDrmLayer = mHasDrmSurface;
    uint32_t rectCount = 0;
    int drmLayerIndex = mForceOverlayLayerIndex;
    size_t i;
    mForceOverlayLayerIndex = -1;
    mHasDrmSurface = false;

    for (size_t j = 0; j < contents->numHwLayers; j++) {
        i = hasDrmLayer ? ((j + drmLayerIndex) % contents->numHwLayers) : j;

        ExynosMPPModule* supportedInternalMPP = NULL;
        ExynosMPPModule* supportedExternalMPP = NULL;
        hwc_layer_1_t &layer = contents->hwLayers[i];
        useVPPOverlayFlag = false;
        hwc_frect_t origin;
        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

            if (getDrmMode(handle->flags) != NO_DRM) {
                useVPPOverlayFlag = true;
                if (mHwc->hdmi_hpd && (!mHwc->video_playback_status)) {
                    layer.flags |= HWC_SKIP_RENDERING;
                    continue;
                } else
                    layer.flags &= ~HWC_SKIP_RENDERING;
            }

            /* check yuv surface */
            if (!isFormatRgb(handle->format)) {
                if (mForceFb && (getDrmMode(handle->flags) == NO_DRM)) {
                    layer.compositionType = HWC_FRAMEBUFFER;
                    mLayerInfos[i]->compositionType = layer.compositionType;
                    mLayerInfos[i]->mCheckOverlayFlag |= eForceFbEnabled;
                    continue;
                }
                /* HACK: force integer source crop */
                layer.sourceCropf.top = (int)layer.sourceCropf.top;
                layer.sourceCropf.left = (int)layer.sourceCropf.left;
                layer.sourceCropf.bottom = (int)(layer.sourceCropf.bottom + 0.9);
                layer.sourceCropf.right = (int)(layer.sourceCropf.right + 0.9);

                /* support to process interlaced color format data */
                if (handle->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV) {
                    void *metaData = NULL;
                    int interlacedType = -1;
                    mOriginFrect.push(layer.sourceCropf);

                    if (handle->fd2 >= 0) {
                        metaData = mmap(0, 64, PROT_READ|PROT_WRITE, MAP_SHARED, handle->fd2, 0);
                        if (metaData)
                            interlacedType = *(int *)metaData;
                        else
                            interlacedType = -1;
                    }

                    if (interlacedType == V4L2_FIELD_INTERLACED_BT) {
                        if ((int)layer.sourceCropf.left < (int)(handle->stride)) {
                            layer.sourceCropf.left = (int)layer.sourceCropf.left + handle->stride;
                            layer.sourceCropf.right = (int)layer.sourceCropf.right + handle->stride;
                        }
                    }
                    if (interlacedType == V4L2_FIELD_INTERLACED_TB || interlacedType == V4L2_FIELD_INTERLACED_BT) {
                            layer.sourceCropf.top = (int)(layer.sourceCropf.top)/2;
                            layer.sourceCropf.bottom = (int)(layer.sourceCropf.bottom)/2;
                    }
                    mBackUpFrect.push(layer.sourceCropf);

                    if (metaData)
                        munmap(metaData, 64);
                }

                if (isOverlaySupported(contents->hwLayers[i], i, useVPPOverlayFlag, &supportedInternalMPP, &supportedExternalMPP)) {
                    this->mYuvLayers++;
                    if (this->mHasDrmSurface == false) {
                        /* Assign MPP */
                        if (supportedExternalMPP != NULL)
                            supportedExternalMPP->mState = MPP_STATE_ASSIGNED;
                        if (supportedInternalMPP != NULL)
                            supportedInternalMPP->mState = MPP_STATE_ASSIGNED;

                        mForceOverlayLayerIndex = i;
                        layer.compositionType = HWC_OVERLAY;
                        mLayerInfos[i]->mExternalMPP = supportedExternalMPP;
                        mLayerInfos[i]->mInternalMPP = supportedInternalMPP;
                        mLayerInfos[i]->compositionType = layer.compositionType;

                        if ((getDrmMode(handle->flags) != NO_DRM) &&
                                isBothMPPProcessingRequired(layer) &&
                                (supportedInternalMPP != NULL)) {
                            layer.displayFrame.right = layer.displayFrame.left +
                                ALIGN_DOWN(WIDTH(layer.displayFrame), supportedInternalMPP->getCropWidthAlign(layer));
                            layer.displayFrame.bottom = layer.displayFrame.top +
                                ALIGN_DOWN(HEIGHT(layer.displayFrame), supportedInternalMPP->getCropHeightAlign(layer));
                        }

                        if ((getDrmMode(handle->flags) != NO_DRM) &&
                                (supportedInternalMPP != NULL)) {
                            if (WIDTH(layer.displayFrame) < supportedInternalMPP->getMinWidth(layer)) {
                                ALOGE("determineYuvOverlay layer %d displayFrame width %d is smaller than vpp minWidth %d",
                                    i, WIDTH(layer.displayFrame), supportedInternalMPP->getMinWidth(layer));
                                layer.displayFrame.right = layer.displayFrame.left +
                                    ALIGN_DOWN(WIDTH(layer.displayFrame), supportedInternalMPP->getMinWidth(layer));
                            }
                            if (HEIGHT(layer.displayFrame) < supportedInternalMPP->getMinHeight(layer)) {
                                ALOGE("determineYuvOverlay layer %d displayFrame height %d is smaller than vpp minHeight %d",
                                    i, HEIGHT(layer.displayFrame), supportedInternalMPP->getMinHeight(layer));
                                layer.displayFrame.bottom = layer.displayFrame.top +
                                    ALIGN_DOWN(HEIGHT(layer.displayFrame), supportedInternalMPP->getMinHeight(layer));
                            }
                        }
                    }
                } else {
                    if (getDrmMode(handle->flags) != NO_DRM) {
                        /* This layer should be overlay but HWC can't handle it */
                        layer.compositionType = HWC_OVERLAY;
                        mLayerInfos[i]->compositionType = layer.compositionType;
                        layer.flags |= HWC_SKIP_RENDERING;
                    }
                }
            }

            if (getDrmMode(handle->flags) != NO_DRM) {
                this->mHasDrmSurface = true;
                mForceOverlayLayerIndex = i;
            }
        }
        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
            if (handle->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV
                    && rectCount < mOriginFrect.size())
                layer.sourceCropf = mOriginFrect[rectCount++];
        }
    }
}

void ExynosDisplay::determineSupportedOverlays(hwc_display_contents_1_t *contents)
{
    bool videoLayer = false;

    mFbNeeded = false;
    mFirstFb = ~0;
    mLastFb = 0;
    uint32_t rectCount = 0;

    // find unsupported overlays
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        mLayerInfos[i]->mCompressed = isCompressed(layer);
        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: framebuffer target", i);
            mLayerInfos[i]->compositionType = layer.compositionType;
            continue;
        }

        if (layer.compositionType == HWC_BACKGROUND && !mForceFb) {
            DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: background supported", i);
            dumpLayer(eDebugOverlaySupported, &contents->hwLayers[i]);
            mLayerInfos[i]->compositionType = layer.compositionType;
            continue;
        }

        if (layer.flags & HWC_SKIP_RENDERING) {
            layer.compositionType = HWC_OVERLAY;
            mLayerInfos[i]->compositionType = layer.compositionType;
            continue;
        }

        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
            if (handle->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV
                    && rectCount < mBackUpFrect.size())
                layer.sourceCropf = mBackUpFrect[rectCount++];

            ExynosMPPModule* supportedInternalMPP = NULL;
            ExynosMPPModule* supportedExternalMPP = NULL;

            if ((int)get_yuv_planes(halFormatToV4L2Format(handle->format)) > 0) {
                videoLayer = true;
                if (!mHwc->hdmi_hpd && mHwc->mS3DMode == S3D_MODE_READY)
                    mHwc->mS3DMode = S3D_MODE_RUNNING;
            }
            mHwc->incomingPixels += WIDTH(layer.displayFrame) * HEIGHT(layer.displayFrame);
            DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer(%d), type=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, "
                    "{%7.1f,%7.1f,%7.1f,%7.1f}, {%d,%d,%d,%d}", i,
                    layer.compositionType, layer.flags, layer.handle, layer.transform,
                    layer.blending,
                    layer.sourceCropf.left,
                    layer.sourceCropf.top,
                    layer.sourceCropf.right,
                    layer.sourceCropf.bottom,
                    layer.displayFrame.left,
                    layer.displayFrame.top,
                    layer.displayFrame.right,
                    layer.displayFrame.bottom);
            /* Video layer's compositionType was set in determineYuvOverlay */
            if (!isFormatRgb(handle->format) && layer.compositionType == HWC_OVERLAY)
                continue;

            if(((getDrmMode(handle->flags) != NO_DRM) ||
                (!mForceFb && (!mHwc->hwc_ctrl.dynamic_recomp_mode || mHwc->CompModeSwitch != HWC_2_GLES))) &&
               isOverlaySupported(contents->hwLayers[i], i, false, &supportedInternalMPP, &supportedExternalMPP)) {
                DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: overlay supported", i);
                if (supportedExternalMPP != NULL) {
                    supportedExternalMPP->mState = MPP_STATE_ASSIGNED;
                    mLayerInfos[i]->mExternalMPP = supportedExternalMPP;
                }
                if (supportedInternalMPP != NULL) {
                    supportedInternalMPP->mState = MPP_STATE_ASSIGNED;
                    mLayerInfos[i]->mInternalMPP = supportedInternalMPP;
                }

                layer.compositionType = HWC_OVERLAY;
                mLayerInfos[i]->compositionType = layer.compositionType;

                dumpLayer(eDebugOverlaySupported, &contents->hwLayers[i]);
                continue;
            } else {
                ExynosMPPModule *dummyInternal = NULL;
                ExynosMPPModule *dummyExternal = NULL;
                DISPLAY_LOGD(eDebugOverlaySupported, "\tlayer %u: overlay is not supported, dynamic_recomp_mode(%d), CompModeSwitch(%d)", i, mHwc->hwc_ctrl.dynamic_recomp_mode, mHwc->CompModeSwitch);
                if (mForceFb)
                    mLayerInfos[i]->mCheckOverlayFlag |= eForceFbEnabled;
                else if (mHwc->hwc_ctrl.dynamic_recomp_mode && mHwc->CompModeSwitch == HWC_2_GLES)
                    mLayerInfos[i]->mCheckOverlayFlag |= eDynamicRecomposition;
                else if (isOverlaySupported(contents->hwLayers[i], i, false, &dummyInternal, &dummyExternal))
                    mLayerInfos[i]->mCheckOverlayFlag |= eUnknown;
            }
        } else {
            mLayerInfos[i]->mCheckOverlayFlag |= eInvalidHandle;
        }

        if (!mFbNeeded) {
            mFirstFb = i;
            mFbNeeded = true;
        }
        mLastFb = i;
        layer.compositionType = HWC_FRAMEBUFFER;
        mLayerInfos[i]->compositionType = layer.compositionType;

        dumpLayer(eDebugOverlaySupported, &contents->hwLayers[i]);
    }

    if (!mHwc->hdmi_hpd && mHwc->mS3DMode == S3D_MODE_RUNNING && !videoLayer)
        mHwc->mS3DMode = S3D_MODE_DISABLED;
    hwc_rect_t base_rect = {0, 0, 0, 0};
    hwc_rect_t intersect_rect;
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.compositionType == HWC_OVERLAY) {
            if (i == 0) {
                base_rect =  layer.displayFrame;
            } else if (hasPlaneAlpha(layer)) {
                //if alpha layer is not completely overlapped with base layer, bypass the alpha layer to GLES.
                intersect_rect = intersection(base_rect, layer.displayFrame);
                if (!rectEqual(intersect_rect, layer.displayFrame)) {
                    layer.compositionType = HWC_FRAMEBUFFER;
                    mLayerInfos[i]->compositionType = layer.compositionType;
                    mLayerInfos[i]->mCheckOverlayFlag |= eUnsupportedBlending;
                    mFirstFb = min(mFirstFb, i);
                    mLastFb = max(mLastFb, i);
                    mFbNeeded = true;
                    break;
                }
            }
        } else {
            // if one of the bottom layer is HWC_FRAMEBUFFER type, no need to force the alpha layer to FRAMEBUFFER type.
            break;
        }
    }
    mFirstFb = min(mFirstFb, (size_t)NUM_HW_WINDOWS-1);
    // can't composite overlays sandwiched between framebuffers
    if (mFbNeeded) {
        private_handle_t *handle = NULL;
        for (size_t i = mFirstFb; i < mLastFb; i++) {
            if (contents->hwLayers[i].flags & HWC_SKIP_RENDERING)
                continue;

            hwc_layer_1_t &layer = contents->hwLayers[i];
            handle = NULL;
            if (layer.handle)
                handle = private_handle_t::dynamicCast(layer.handle);
            if (handle && getDrmMode(handle->flags) != NO_DRM) {
                layer.hints = HWC_HINT_CLEAR_FB;
            } else {
                contents->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
                mLayerInfos[i]->compositionType = contents->hwLayers[i].compositionType;
                mLayerInfos[i]->mCheckOverlayFlag |= eSandwitchedBetweenGLES;
                if (mLayerInfos[i]->mExternalMPP != NULL) {
                    mLayerInfos[i]->mExternalMPP->mState = MPP_STATE_FREE;
                    mLayerInfos[i]->mExternalMPP = NULL;
                }
                if (mLayerInfos[i]->mInternalMPP != NULL) {
                    mLayerInfos[i]->mInternalMPP->mState = MPP_STATE_FREE;
                    mLayerInfos[i]->mInternalMPP = NULL;
                }
            }
        }
    }
}

void ExynosDisplay::determineBandwidthSupport(hwc_display_contents_1_t *contents)
{
    // Incrementally try to add our supported layers to hardware windows.
    // If adding a layer would violate a hardware constraint, force it
    // into the framebuffer and try again.  (Revisiting the entire list is
    // necessary because adding a layer to the framebuffer can cause other
    // windows to retroactively violate constraints.)
    bool changed;
    this->mBypassSkipStaticLayer = false;
    unsigned int cannotComposeFlag = 0;
    int internalDMAsUsed = 0;
    int retry = 0;
    int fbIndex = contents->numHwLayers - 1;

    // Initialize to inverse values so that
    // min(left, l) = l, min(top, t) = t
    // max(right, r) = r, max(bottom, b) = b
    // for all l, t, r, b
    mFbUpdateRegion.left = mXres;
    mFbUpdateRegion.top = mYres;
    mFbUpdateRegion.right = 0;
    mFbUpdateRegion.bottom = 0;

    do {
        uint32_t win_idx = 0;
        size_t windows_left;
        unsigned int directFbNum = 0;
        int videoOverlays = 0;
        mHwc->totPixels = 0;
        ExynosMPPModule* supportedInternalMPP = NULL;
        ExynosMPPModule* supportedExternalMPP = NULL;
        bool ret = 0;

        for (size_t i = 0; i < mInternalMPPs.size(); i++) {
            if (mInternalMPPs[i]->mState != MPP_STATE_TRANSITION &&
                (!mHasDrmSurface ||
                 (mLayerInfos[mForceOverlayLayerIndex]->mInternalMPP != mInternalMPPs[i]))) {
                mInternalMPPs[i]->mState = MPP_STATE_FREE;
            }
        }

        for (size_t i = 0; i < mExternalMPPs.size(); i++) {
            if (mExternalMPPs[i]->mState != MPP_STATE_TRANSITION &&
                (!mHasDrmSurface ||
                 (mLayerInfos[mForceOverlayLayerIndex]->mExternalMPP != mExternalMPPs[i]))) {
                mExternalMPPs[i]->mState = MPP_STATE_FREE;
            }
        }

        for (size_t i = 0; i < contents->numHwLayers; i++) {
            if (!mHasDrmSurface || (int)i != mForceOverlayLayerIndex) {
                mLayerInfos[i]->mInternalMPP = NULL;
                mLayerInfos[i]->mExternalMPP = NULL;
            }
        }

        changed = false;
        mMPPLayers = 0;

        if (mFbPreAssigned) {
            DISPLAY_LOGD(eDebugResourceAssigning, "fb has been pre-assigned already");
            windows_left = min(mAllowedOverlays, mHwc->hwc_ctrl.max_num_ovly) - 1;
        } else if (mFbNeeded && (contents->numHwLayers - 1 > 0)) {
            hwc_layer_1_t &layer = contents->hwLayers[fbIndex];
            if (mUseSecureDMA && (mLastFb == (contents->numHwLayers - 2)) && isOverlaySupportedByIDMA(layer, fbIndex)) {
                /* FramebufferTarget is the top layer, Secure DMA is used */
                windows_left = min(mAllowedOverlays, mHwc->hwc_ctrl.max_num_ovly);
                mLayerInfos[contents->numHwLayers - 1]->mDmaType = IDMA_SECURE;
            } else if ((mInternalDMAs.size() > 0) && isOverlaySupportedByIDMA(layer, fbIndex)) {
                /* Internal DMA is used */
                windows_left = min(mAllowedOverlays, mHwc->hwc_ctrl.max_num_ovly) - 1;
                mLayerInfos[contents->numHwLayers - 1]->mDmaType = mInternalDMAs[directFbNum];
                win_idx = (win_idx == mFirstFb) ? (win_idx + 1) : win_idx;
                directFbNum++;
            } else {
                /* VPP should be used for DMA */
                windows_left = min(mAllowedOverlays, mHwc->hwc_ctrl.max_num_ovly) - 1;
                ret = isOverlaySupported(layer, fbIndex, true, &supportedInternalMPP, &supportedExternalMPP);
                if (ret && (supportedInternalMPP != NULL)) {
                    DISPLAY_LOGD(eDebugResourceAssigning, "FramebufferTarget internalMPP(%d, %d)", supportedInternalMPP->mType, supportedInternalMPP->mIndex);
                    supportedInternalMPP->mState = MPP_STATE_ASSIGNED;
                    mLayerInfos[fbIndex]->mInternalMPP = supportedInternalMPP;
                    mLayerInfos[fbIndex]->mDmaType = getDeconDMAType(mLayerInfos[fbIndex]->mInternalMPP);
                    if (mLayerInfos[fbIndex]->mDmaType >= MAX_DECON_DMA_TYPE) {
                        ALOGE("getDeconDMAType with InternalMPP for FramebufferTarget failed (MPP type: %d, MPP index: %d)",
                                mLayerInfos[fbIndex]->mInternalMPP->mType, mLayerInfos[fbIndex]->mInternalMPP->mIndex);
                        mLayerInfos[fbIndex]->mDmaType = 0;
                    }
                    win_idx = (win_idx == mFirstFb) ? (win_idx + 1) : win_idx;
                } else {
                    ALOGE("VPP should be assigned to FramebufferTarget but it was failed ret(%d)", ret);
                }
            }
            mHwc->totPixels += mXres * mYres;
        } else {
            windows_left = min(mAllowedOverlays, mHwc->hwc_ctrl.max_num_ovly);
        }

        DISPLAY_LOGD(eDebugResourceAssigning, "determineBandwidthSupport:: retry(%d), mAllowedOverlays(%d), windows_left(%d), win_idx(%d)",
                retry, mAllowedOverlays, windows_left, win_idx);

        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            bool isTopLayer = (i == contents->numHwLayers - 2) ? true:false;
            private_handle_t *handle = NULL;

            if (layer.flags & HWC_SKIP_RENDERING)
                continue;

            if ((layer.flags & HWC_SKIP_LAYER) ||
                (layer.compositionType == HWC_FRAMEBUFFER_TARGET))
                continue;

            if (!layer.planeAlpha)
                continue;

            if (layer.handle)
                handle = private_handle_t::dynamicCast(layer.handle);
            else
                continue;

            // we've already accounted for the framebuffer above
            if (layer.compositionType == HWC_FRAMEBUFFER)
                continue;

            // only layer 0 can be HWC_BACKGROUND, so we can
            // unconditionally allow it without extra checks
            if (layer.compositionType == HWC_BACKGROUND) {
                windows_left--;
                continue;
            }

            cannotComposeFlag = 0;
            bool can_compose = windows_left && (win_idx < NUM_HW_WINDOWS);
            if (mUseSecureDMA && !isCompressed(layer) && isTopLayer)
                can_compose = true;
            else if (windows_left <= 0 || (win_idx >= NUM_HW_WINDOWS))
                cannotComposeFlag |= eInsufficientWindow;
            if (!isFormatRgb(handle->format) && videoOverlays >= mHwc->hwc_ctrl.num_of_video_ovly) {
                can_compose = false;
                cannotComposeFlag |= eInsufficientMPP;
            }

            /* mInternalMPP, mExternalMPP could be set by determineYuvOverlay */
            supportedInternalMPP = mLayerInfos[i]->mInternalMPP;
            supportedExternalMPP = mLayerInfos[i]->mExternalMPP;

            if (can_compose && !isProcessingRequired(layer) && isOverlaySupportedByIDMA(layer, i) &&
                (directFbNum < mInternalDMAs.size() || (mUseSecureDMA && isTopLayer))) {
                if (directFbNum < mInternalDMAs.size())
                    directFbNum++;
                DISPLAY_LOGD(eDebugResourceAssigning, "layer(%d) is directFB", i);
            } else if (can_compose && isOverlaySupported(layer, i,
                        !isProcessingRequired(layer) |
                        (directFbNum >= mInternalDMAs.size() && !(mUseSecureDMA && isTopLayer)),
                        &supportedInternalMPP, &supportedExternalMPP)) {
                DISPLAY_LOGD(eDebugResourceAssigning, "layer(%d) is OVERLAY ",i);
                if (supportedInternalMPP != NULL) {
                    DISPLAY_LOGD(eDebugResourceAssigning, "layer(%d) is OVERLAY internalMPP(%d, %d)", i, supportedInternalMPP->mType, supportedInternalMPP->mIndex);
                    supportedInternalMPP->mState = MPP_STATE_ASSIGNED;
                    mLayerInfos[i]->mInternalMPP = supportedInternalMPP;
                    mLayerInfos[i]->mDmaType = getDeconDMAType(mLayerInfos[i]->mInternalMPP);
                }
                if (supportedExternalMPP != NULL) {
                    DISPLAY_LOGD(eDebugResourceAssigning, "layer(%d) is OVERLAY externalMPP(%d, %d)", i, supportedExternalMPP->mType, supportedExternalMPP->mIndex);
                    supportedExternalMPP->mState = MPP_STATE_ASSIGNED;
                    mLayerInfos[i]->mExternalMPP = supportedExternalMPP;
                    if ((supportedInternalMPP == NULL) && isOverlaySupportedByIDMA(layer, i) &&
                        ((directFbNum < mInternalDMAs.size()) || (mUseSecureDMA && isTopLayer))) {
                        if (directFbNum < mInternalDMAs.size()) {
                            mLayerInfos[i]->mDmaType = mInternalDMAs[directFbNum];
                            directFbNum++;
                        } else {
                            mLayerInfos[i]->mDmaType = IDMA_SECURE;
                        }
                    }
                }
                mMPPLayers++;
                if (!isFormatRgb(handle->format))
                    videoOverlays++;
            } else {
                DISPLAY_LOGD(eDebugResourceAssigning, "layer(%d) is changed to FRAMEBUFFER", i);
                can_compose = false;
            }

            if (!can_compose) {
                size_t changed_index = i;
                hwc_layer_1_t *layer_for_gles = &contents->hwLayers[i];
                if (getDrmMode(handle->flags) != NO_DRM) {
                    int j = 0;
                    for (j = i - 1; j >= 0 ; j--) {
                        layer_for_gles = &contents->hwLayers[j];
                        if (layer_for_gles->compositionType == HWC_OVERLAY) {
                            changed_index = j;
                            break;
                        }
                    }
                    if (j < 0) {
                        mFirstFb = 0;
                        changed_index = 0;
                        layer_for_gles = &contents->hwLayers[changed_index];
                    }
                }
                layer_for_gles->compositionType = HWC_FRAMEBUFFER;
                mLayerInfos[changed_index]->compositionType = layer_for_gles->compositionType;
                mLayerInfos[changed_index]->mCheckOverlayFlag |= cannotComposeFlag;
                if (mLayerInfos[changed_index]->mInternalMPP != NULL)
                    mLayerInfos[changed_index]->mInternalMPP->mState = MPP_STATE_FREE;
                if (mLayerInfos[changed_index]->mExternalMPP != NULL)
                    mLayerInfos[changed_index]->mExternalMPP->mState = MPP_STATE_FREE;
                mLayerInfos[changed_index]->mInternalMPP = NULL;
                mLayerInfos[changed_index]->mExternalMPP = NULL;
                if (!mFbNeeded) {
                    mFirstFb = mLastFb = changed_index;
                    mFbNeeded = true;
                    mHwc->totPixels += mXres * mYres;
                }
                else {
                    mFirstFb = min(changed_index, mFirstFb);
                    mLastFb = max(changed_index, mLastFb);
                }
                changed = true;
                mFirstFb = min(mFirstFb, (size_t)NUM_HW_WINDOWS-1);
                break;
            } else {
                mHwc->totPixels += WIDTH(layer.displayFrame) * HEIGHT(layer.displayFrame);
            }

            if ((mUseSecureDMA == false) || (isTopLayer == false)) {
                win_idx++;
                /* FB Target is not top layer */
                if (mFbNeeded && ((mUseSecureDMA && !isCompressed(layer))|| (mLastFb != (contents->numHwLayers - 2))))
                    win_idx = (win_idx == mFirstFb) ? (win_idx + 1) : win_idx;
                win_idx = min((size_t)win_idx, (size_t)(NUM_HW_WINDOWS - 1));
                windows_left--;
            }
        }

        if (changed)
            for (size_t i = mFirstFb; i < mLastFb; i++) {
                hwc_layer_1_t &layer = contents->hwLayers[i];
                private_handle_t *handle = NULL;
                if (layer.handle)
                    handle = private_handle_t::dynamicCast(layer.handle);
                if (handle && getDrmMode(handle->flags) != NO_DRM) {
                    layer.hints = HWC_HINT_CLEAR_FB;
                } else {
                    contents->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
                    mLayerInfos[i]->compositionType = contents->hwLayers[i].compositionType;
                    mLayerInfos[i]->mCheckOverlayFlag |= eSandwitchedBetweenGLES;
                    if (mLayerInfos[i]->mInternalMPP != NULL)
                        mLayerInfos[i]->mInternalMPP->mState = MPP_STATE_FREE;
                    if (mLayerInfos[i]->mExternalMPP != NULL)
                        mLayerInfos[i]->mExternalMPP->mState = MPP_STATE_FREE;
                    mLayerInfos[i]->mInternalMPP = NULL;
                    mLayerInfos[i]->mExternalMPP = NULL;
                    if (handle && !isFormatRgb(handle->format))
                        videoOverlays--;
                }
            }
        if (handleTotalBandwidthOverload(contents))
            changed = true;
        retry++;

        if (retry > 100) {
            DISPLAY_LOGE("%s", "retry 100, can't allocate vpp, dump error state");
            android::String8 result;
            result.clear();
            dumpLayerInfo(result);
            DISPLAY_LOGE("%s", result.string());
            break;
        }
    } while(changed);

    uint32_t rectCount = 0;
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
            if (handle->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV
                    && rectCount < mOriginFrect.size())
                layer.sourceCropf = mOriginFrect[rectCount++];
        }
    }

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.compositionType == HWC_FRAMEBUFFER)
            mFbUpdateRegion = expand(mFbUpdateRegion, layer.displayFrame);
    }

    for (size_t i = mLastFb + 1; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.compositionType == HWC_OVERLAY && layer.planeAlpha > 0 && layer.planeAlpha < 255)
            mFbUpdateRegion = expand(mFbUpdateRegion, contents->hwLayers[i].displayFrame);
    }

    int minWidth = 0;
#ifdef USE_DRM_BURST_LEN
    if (mHasDrmSurface)
        minWidth = DRM_BURSTLEN_BYTES * 8 / formatToBpp(HAL_PIXEL_FORMAT_RGBA_8888);
    else
#endif
        minWidth = BURSTLEN_BYTES * 8 / formatToBpp(HAL_PIXEL_FORMAT_RGBA_8888);

    int w = WIDTH(mFbUpdateRegion);
    if (w < minWidth) {
#if defined(USES_DUAL_DISPLAY)
        uint32_t maxRightPosition = (mType == EXYNOS_PRIMARY_DISPLAY)?(mXres/2):mXres;
        if (mFbUpdateRegion.left + minWidth <= maxRightPosition)
            mFbUpdateRegion.right = mFbUpdateRegion.left + minWidth;
        else
            mFbUpdateRegion.left = mFbUpdateRegion.right - minWidth;
#else
        if (mFbUpdateRegion.left + minWidth <= mXres)
            mFbUpdateRegion.right = mFbUpdateRegion.left + minWidth;
        else
            mFbUpdateRegion.left = mFbUpdateRegion.right - minWidth;
#endif
    }
}

void ExynosDisplay::assignWindows(hwc_display_contents_1_t *contents)
{
    unsigned int nextWindow = 0;
    unsigned int directFbNum = 0;
    bool isTopLayer = false;

    hwc_layer_1_t &fbLayer = contents->hwLayers[contents->numHwLayers - 1];
    if (mFbNeeded && (contents->numHwLayers - 1 > 0)) {
        /* FramebufferTarget is the top layer */
        if (mUseSecureDMA && !isCompressed(fbLayer) && mLastFb == (contents->numHwLayers - 2))
            mLayerInfos[contents->numHwLayers - 1]->mDmaType = IDMA_SECURE;
        else if (mInternalDMAs.size() > 0 && !isCompressed(fbLayer)) {
            mLayerInfos[contents->numHwLayers - 1]->mDmaType = mInternalDMAs[directFbNum];
            directFbNum++;
        } else {
            /* mDmaType was set by determineBandwidthSupport() */
        }

        DISPLAY_LOGD(eDebugResourceAssigning, "assigning layer %u to DMA %u", contents->numHwLayers - 1, mLayerInfos[contents->numHwLayers - 1]->mDmaType);
    }

    if (mFbNeeded && mUseSecureDMA && !isCompressed(fbLayer) && (mLastFb == (contents->numHwLayers - 2)))
        mFbWindow = NUM_HW_WINDOWS - 1;
    else
        mFbWindow = NO_FB_NEEDED;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        mLayerInfos[i]->mWindowIndex = -1;
        isTopLayer = (i == contents->numHwLayers - 2) ? true:false;

        if (mFbNeeded && (mFbWindow != NUM_HW_WINDOWS - 1)) {
            if (i == mLastFb) {
                mFbWindow = nextWindow;
                nextWindow++;
                continue;
            }
        }

        if (layer.flags & HWC_SKIP_RENDERING)
            continue;

        if (!layer.planeAlpha) {
            if (layer.acquireFenceFd >= 0)
                close(layer.acquireFenceFd);
            layer.acquireFenceFd = -1;
            layer.releaseFenceFd = -1;
            continue;
        }

        if (layer.compositionType != HWC_FRAMEBUFFER) {
            if (mFbNeeded && (layer.compositionType == HWC_FRAMEBUFFER_TARGET)) {
                DISPLAY_LOGD(eDebugResourceAssigning, "assigning framebuffer target %u to window %u", i, nextWindow);
                mLayerInfos[i]->mWindowIndex = mFbWindow;
                if (mLayerInfos[i]->mInternalMPP != NULL)
                    mLayerInfos[i]->mInternalMPP->setDisplay(this);
                continue;
            }
            if (layer.compositionType == HWC_OVERLAY) {
                if ((!isProcessingRequired(layer) ||
                     ((mLayerInfos[i]->mInternalMPP == NULL) && (mLayerInfos[i]->mExternalMPP != NULL))) &&
                     isOverlaySupportedByIDMA(layer, i) && (directFbNum < mInternalDMAs.size() || (mUseSecureDMA && isTopLayer)))
                {
                    if (directFbNum < mInternalDMAs.size()) {
                        DISPLAY_LOGD(eDebugResourceAssigning, "assigning layer %u to DMA %u", i, mInternalDMAs[directFbNum]);
                        mLayerInfos[i]->mDmaType = mInternalDMAs[directFbNum];
                        mLayerInfos[i]->mWindowIndex = nextWindow;
                        directFbNum++;
                    } else {
                        DISPLAY_LOGD(eDebugResourceAssigning, "assigning layer %u to DMA %u", i, IDMA_SECURE);
                        mLayerInfos[i]->mDmaType = IDMA_SECURE;
                        mLayerInfos[i]->mWindowIndex = NUM_HW_WINDOWS - 1;
                    }
                } else {
                    DISPLAY_LOGD(eDebugResourceAssigning, "%u layer can't use internalDMA, isProcessingRequired(%d)", i, isProcessingRequired(layer));
                    unsigned int dmaType = 0;
                    mLayerInfos[i]->mWindowIndex = nextWindow;
                    if (mLayerInfos[i]->mInternalMPP != NULL) {
                        mLayerInfos[i]->mInternalMPP->setDisplay(this);
                        mLayerInfos[i]->mDmaType = getDeconDMAType(mLayerInfos[i]->mInternalMPP);
                        DISPLAY_LOGD(eDebugResourceAssigning, "assigning layer %u to DMA %u", i, mLayerInfos[i]->mDmaType);
                    } else {
                        /* Find unused DMA connected with VPP */
                        for (size_t j = 0; j < mInternalMPPs.size(); j++ )
                        {
                            if ((mInternalMPPs[j]->mState == MPP_STATE_FREE) &&
                                    ((mInternalMPPs[j]->mDisplay == NULL) || (mInternalMPPs[j]->mDisplay == this))) {
                                mLayerInfos[i]->mInternalMPP = mInternalMPPs[j];
                                mLayerInfos[i]->mDmaType = getDeconDMAType(mLayerInfos[i]->mInternalMPP);
                                mLayerInfos[i]->mInternalMPP->setDisplay(this);
                                mLayerInfos[i]->mInternalMPP->mState = MPP_STATE_ASSIGNED;
                                DISPLAY_LOGD(eDebugResourceAssigning, "assigning layer %u to DMA %u", i, mLayerInfos[i]->mDmaType);
                                break;
                            }
                        }
                    }
                }
                if (mLayerInfos[i]->mExternalMPP != NULL)
                    mLayerInfos[i]->mExternalMPP->setDisplay(this);
            }
            if (mLayerInfos[i]->mWindowIndex != NUM_HW_WINDOWS - 1)
                nextWindow++;
        }
    }

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.compositionType == HWC_FRAMEBUFFER ||
            layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            mLayerInfos[i]->mWindowIndex = mFbWindow;
            if (contents->numHwLayers - 1 > 0) {
                mLayerInfos[i]->mDmaType = mLayerInfos[contents->numHwLayers - 1]->mDmaType;
            }
        }
    }
}

int ExynosDisplay::postMPPM2M(hwc_layer_1_t &layer, struct decon_win_config *config, int win_map, int index)
{
    //exynos5_hwc_post_data_t *pdata = &mPostData;
    //int gsc_idx = pdata->gsc_map[index].idx;
    int dst_format = mExternalMPPDstFormat;
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    ExynosMPPModule *exynosMPP = mLayerInfos[index]->mExternalMPP;
    ExynosMPPModule *exynosInternalMPP = mLayerInfos[index]->mInternalMPP;
    hwc_layer_1_t extMPPOutLayer;

    if (exynosMPP == NULL) {
        DISPLAY_LOGE("postMPPM2M is called but externMPP is NULL");
        if (layer.acquireFenceFd >= 0)
            close(layer.acquireFenceFd);
        layer.acquireFenceFd = -1;
        layer.releaseFenceFd = -1;
        return -1;
    }

    hwc_frect_t sourceCrop = { 0, 0,
            (float)WIDTH(layer.displayFrame), (float)HEIGHT(layer.displayFrame) };
    int originalTransform = layer.transform;
    hwc_rect_t originalDisplayFrame = layer.displayFrame;

    bool bothMPPUsed = isBothMPPProcessingRequired(layer, &extMPPOutLayer);
    if (bothMPPUsed) {
        sourceCrop.right = extMPPOutLayer.displayFrame.right;
        sourceCrop.bottom = extMPPOutLayer.displayFrame.bottom;
        layer.displayFrame = extMPPOutLayer.displayFrame;
    }

    if (mType == EXYNOS_PRIMARY_DISPLAY) {
        handle->flags &= ~GRALLOC_USAGE_VIDEO_EXT;
        if (mHwc->mS3DMode == S3D_MODE_READY || mHwc->mS3DMode == S3D_MODE_RUNNING) {
            int S3DFormat = getS3DFormat(mHwc->mHdmiPreset);
            if (S3DFormat == S3D_SBS)
                exynosMPP->mS3DMode = S3D_SBS;
            else if (S3DFormat == S3D_TB)
                exynosMPP->mS3DMode = S3D_TB;
        } else {
            exynosMPP->mS3DMode = S3D_NONE;
        }
    }

    /* OFF_Screen to ON_Screen changes */
    if (getDrmMode(handle->flags) != NO_DRM)
        recalculateDisplayFrame(layer, mXres, mYres);

    if (mType != EXYNOS_VIRTUAL_DISPLAY &&
        (isFormatRgb(handle->format) ||
        (bothMPPUsed && !isFormatRgb(handle->format) &&
         exynosInternalMPP != NULL &&
         exynosInternalMPP->isCSCSupportedByMPP(handle->format, HAL_PIXEL_FORMAT_RGBX_8888, layer.dataSpace) &&
         exynosInternalMPP->isFormatSupportedByMPP(handle->format) &&
         WIDTH(extMPPOutLayer.displayFrame) % exynosInternalMPP->getCropWidthAlign(layer) == 0 &&
         HEIGHT(extMPPOutLayer.displayFrame) % exynosInternalMPP->getCropHeightAlign(layer) == 0)))
        dst_format = handle->format;
    int err = exynosMPP->processM2M(layer, dst_format, &sourceCrop);

    /* Restore displayFrame*/
    layer.displayFrame = originalDisplayFrame;

    if (err < 0) {
        DISPLAY_LOGE("failed to configure MPP (type:%u, index:%u) for layer %u",
                exynosMPP->mType, exynosMPP->mIndex, index);
        return -1;
    }

    buffer_handle_t dst_buf = exynosMPP->mDstBuffers[exynosMPP->mCurrentBuf];
    private_handle_t *dst_handle =
            private_handle_t::dynamicCast(dst_buf);
    int fence = exynosMPP->mDstConfig.releaseFenceFd;
    hwc_frect originalCrop = layer.sourceCropf;
    int originalFormat = handle->format;

    /* ExtMPP out is the input of Decon
     * and Trsform was processed by ExtMPP
     */
    layer.sourceCropf = sourceCrop;
    layer.transform = 0;
    handle->format = dst_format;
    configureHandle(dst_handle, index, layer,  fence, config[win_map]);

    /* Restore sourceCropf and transform */
    layer.sourceCropf = originalCrop;
    layer.transform = originalTransform;
    handle->format = originalFormat;
    return 0;
}

void ExynosDisplay::handleStaticLayers(hwc_display_contents_1_t *contents, struct decon_win_config_data &win_data, int __unused tot_ovly_wins)
{
    int win_map = 0;
    if (mLastFbWindow >= NUM_HW_WINDOWS) {
        DISPLAY_LOGE("handleStaticLayers:: invalid mLastFbWindow(%d)", mLastFbWindow);
        return;
    }
    win_map = mLastFbWindow;
    DISPLAY_LOGD(eDebugSkipStaicLayer, "[USE] SKIP_STATIC_LAYER_COMP, mLastFbWindow(%d), win_map(%d)\n", mLastFbWindow, win_map);

    memcpy(&win_data.config[win_map],
            &mLastConfigData.config[win_map], sizeof(struct decon_win_config));
    win_data.config[win_map].fence_fd = -1;

    for (size_t i = mFirstFb; i <= mLastFb; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        private_handle_t *handle = NULL;
        if (layer.handle == NULL)
            continue;
        else
            handle = private_handle_t::dynamicCast(layer.handle);

        if ((getDrmMode(handle->flags) == NO_DRM) &&
            (layer.compositionType == HWC_OVERLAY)) {
            DISPLAY_LOGD(eDebugSkipStaicLayer, "[SKIP_STATIC_LAYER_COMP] layer.handle: 0x%p, layer.acquireFenceFd: %d\n", layer.handle, layer.acquireFenceFd);
            if (layer.acquireFenceFd >= 0)
                close(layer.acquireFenceFd);
            layer.acquireFenceFd = -1;
            layer.releaseFenceFd = -1;
        }
    }
}

bool ExynosDisplay::multipleRGBScaling(int format)
{
    return isFormatRgb(format) &&
            mMPPLayers >= 1;
}

bool ExynosDisplay::isProcessingRequired(hwc_layer_1_t &layer)
{
    if (!layer.handle)
        return false;
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    return !(isFormatRgb(handle->format)) || isScaled(layer) || isTransformed(layer) || isCompressed(layer) ||
        isFloat(layer.sourceCropf.left) || isFloat(layer.sourceCropf.top) ||
        isFloat(layer.sourceCropf.right - layer.sourceCropf.left) ||
        isFloat(layer.sourceCropf.bottom - layer.sourceCropf.top);
}

bool ExynosDisplay::isBothMPPProcessingRequired(hwc_layer_1_t &layer)
{
    bool needDoubleOperation = false;
    private_handle_t *srcHandle = NULL;

    if (layer.handle == NULL)
        return false;

    srcHandle = private_handle_t::dynamicCast(layer.handle);
    if ((mExternalMPPs.size() == 0) || (mInternalMPPs.size() == 0))
        return false;

    /* Check scale ratio */
    int maxUpscaleExt = mExternalMPPs[0]->getMaxUpscale();
    int maxUpscaleInt = mCheckIntMPP->getMaxUpscale();
    bool rot90or270 = !!(layer.transform & HAL_TRANSFORM_ROT_90);
    int srcW = WIDTH(layer.sourceCropf), srcH = HEIGHT(layer.sourceCropf);
    int dstW, dstH;
    if (rot90or270) {
        dstW = HEIGHT(layer.displayFrame);
        dstH = WIDTH(layer.displayFrame);
    } else {
        dstW = WIDTH(layer.displayFrame);
        dstH = HEIGHT(layer.displayFrame);
    }
    needDoubleOperation = ((dstW > srcW * maxUpscaleExt) && (dstW <= srcW * maxUpscaleExt * maxUpscaleInt)) ||
                          ((dstH > srcH * maxUpscaleExt) && (dstH <= srcH * maxUpscaleExt * maxUpscaleInt));

    int maxDownscaleExt = mExternalMPPs[0]->getMaxDownscale(layer);
    int maxDownscaleInt = mCheckIntMPP->getMaxDownscale(layer);

    needDoubleOperation |= ((dstW < srcW / maxDownscaleExt) && (dstW >= srcW / (maxDownscaleExt * maxDownscaleExt))) ||
                          ((dstH < srcH / maxDownscaleExt) && (dstH >= srcH / (maxDownscaleExt * maxDownscaleExt)));

    /*
     * Both VPP and MSC should be used if
     * MSC should be used for DRM contents
     */
    if (getDrmMode(srcHandle->flags) != NO_DRM) {
        bool supportVPP = false;
        for (size_t i = 0; i < mInternalMPPs.size(); i++ )
        {
            if (mInternalMPPs[i]->isProcessingSupported(layer, srcHandle->format) > 0) {
                supportVPP = true;
                break;
            }
        }
        if (supportVPP == false)
            needDoubleOperation |= true;
    }

    /*
     * UHD case
     * Destination format should be RGB if dstW or dstH is not aligned.
     * VPP is not required at this time.
     */
    if (!isFormatRgb(srcHandle->format) &&
        (srcW >= UHD_WIDTH || srcH >= UHD_HEIGHT) &&
        (mInternalMPPs.size() > 0) &&
        (dstW % mCheckIntMPP->getCropWidthAlign(layer) == 0) &&
        (dstH % mCheckIntMPP->getCropHeightAlign(layer) == 0)) {
        needDoubleOperation |= true;
    }

    return needDoubleOperation;
}

bool ExynosDisplay::isBothMPPProcessingRequired(hwc_layer_1_t &layer, hwc_layer_1_t *extMPPOutLayer)
{
    bool needDoubleOperation = false;
    if (layer.handle == NULL)
        return false;

    private_handle_t *srcHandle = private_handle_t::dynamicCast(layer.handle);
    if ((mExternalMPPs.size() == 0) || (mInternalMPPs.size() == 0))
        return false;

    int maxUpscaleExt = mExternalMPPs[0]->getMaxUpscale();
    int maxUpscaleInt = mCheckIntMPP->getMaxUpscale();
    bool rot90or270 = !!(layer.transform & HAL_TRANSFORM_ROT_90);
    int srcW = WIDTH(layer.sourceCropf), srcH = HEIGHT(layer.sourceCropf);
    int dstW, dstH;
    if (rot90or270) {
        dstW = HEIGHT(layer.displayFrame);
        dstH = WIDTH(layer.displayFrame);
    } else {
        dstW = WIDTH(layer.displayFrame);
        dstH = HEIGHT(layer.displayFrame);
    }

    int maxDownscaleExt = mExternalMPPs[0]->getMaxDownscale(layer);
    int maxDownscaleInt = mCheckIntMPP->getMaxDownscale(layer);

    needDoubleOperation = isBothMPPProcessingRequired(layer);

    /* set extMPPOutLayer */
    if (needDoubleOperation && extMPPOutLayer != NULL) {
        memcpy(extMPPOutLayer, &layer, sizeof(hwc_layer_1_t));
        extMPPOutLayer->displayFrame.left = 0;
        extMPPOutLayer->displayFrame.top = 0;

        if (dstW > srcW * maxUpscaleExt) {
            extMPPOutLayer->displayFrame.right = ALIGN_UP((int)ceilf((float)dstW/ maxUpscaleInt),
                    mExternalMPPs[0]->getDstWidthAlign(srcHandle->format));
        } else if (dstW < srcW / maxDownscaleExt) {
            extMPPOutLayer->displayFrame.right = ALIGN(dstW * maxDownscaleInt,
                    mExternalMPPs[0]->getDstWidthAlign(srcHandle->format));
        } else {
            extMPPOutLayer->displayFrame.right = dstW;
        }

        if (dstH > srcH * maxUpscaleExt) {
            extMPPOutLayer->displayFrame.bottom = ALIGN_UP((int)ceilf((float)dstH/ maxUpscaleInt),
                    mExternalMPPs[0]->getDstHeightAlign(srcHandle->format));
        } else if (dstH < srcH / maxDownscaleExt) {
            extMPPOutLayer->displayFrame.bottom = ALIGN(dstH * maxDownscaleInt,
                    mExternalMPPs[0]->getDstHeightAlign(srcHandle->format));
        } else {
            extMPPOutLayer->displayFrame.bottom = dstH;
        }

        if (rot90or270) {
            dstW = extMPPOutLayer->displayFrame.bottom;
            dstH = extMPPOutLayer->displayFrame.right;
            extMPPOutLayer->displayFrame.right = dstW;
            extMPPOutLayer->displayFrame.bottom = dstH;
        }
    }

    return needDoubleOperation;
}

bool ExynosDisplay::isSourceCropfSupported(hwc_layer_1_t layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    unsigned int bpp = formatToBpp(handle->format);

    /* HACK: Disable overlay if the layer have float position or size */
#if 0
    if ((isFormatRgb(handle->format) && (bpp == 32)) ||
         isFormatYUV420(handle->format))
        return true;
#endif

    return false;
}

bool ExynosDisplay::checkConfigChanged(struct decon_win_config_data &lastConfigData, struct decon_win_config_data &newConfigData)
{
    for (size_t i = 0; i <= MAX_DECON_WIN; i++) {
        if ((lastConfigData.config[i].state != newConfigData.config[i].state) ||
                (lastConfigData.config[i].fd_idma[0] != newConfigData.config[i].fd_idma[0]) ||
                (lastConfigData.config[i].fd_idma[1] != newConfigData.config[i].fd_idma[1]) ||
                (lastConfigData.config[i].fd_idma[2] != newConfigData.config[i].fd_idma[2]) ||
                (lastConfigData.config[i].dst.x != newConfigData.config[i].dst.x) ||
                (lastConfigData.config[i].dst.y != newConfigData.config[i].dst.y) ||
                (lastConfigData.config[i].dst.w != newConfigData.config[i].dst.w) ||
                (lastConfigData.config[i].dst.h != newConfigData.config[i].dst.h) ||
                (lastConfigData.config[i].src.x != newConfigData.config[i].src.x) ||
                (lastConfigData.config[i].src.y != newConfigData.config[i].src.y) ||
                (lastConfigData.config[i].src.w != newConfigData.config[i].src.w) ||
                (lastConfigData.config[i].src.h != newConfigData.config[i].src.h) ||
                (lastConfigData.config[i].format != newConfigData.config[i].format) ||
                (lastConfigData.config[i].blending != newConfigData.config[i].blending) ||
                (lastConfigData.config[i].plane_alpha != newConfigData.config[i].plane_alpha))
            return true;
    }
    return false;
}

void ExynosDisplay::removeIDMA(decon_idma_type idma)
{
    for (size_t i = mInternalDMAs.size(); i-- > 0;) {
        if (mInternalDMAs[i] == (unsigned int)idma) {
            mInternalDMAs.removeItemsAt(i);
        }
    }
}

int ExynosDisplay::getDisplayConfigs(uint32_t *configs, size_t *numConfigs)
{
    configs[0] = 0;
    *numConfigs = 1;
    return 0;
}

int ExynosDisplay::getDeconDMAType(ExynosMPPModule* internalMPP)
{
    if (internalMPP->mType == MPP_VG)
        return IDMA_VG0 + internalMPP->mIndex;
    else if (internalMPP->mType == MPP_VGR)
        return IDMA_VGR0 + internalMPP->mIndex;
    else if (internalMPP->mType == MPP_VPP_G) {
        switch (internalMPP->mIndex) {
        case 0:
            return IDMA_G0;
        case 1:
            return IDMA_G1;
        case 2:
            return IDMA_G2;
        case 3:
            return IDMA_G3;
        default:
            return -1;
        }
    } else
        return -1;
}

void ExynosDisplay::dumpContents(android::String8& result, hwc_display_contents_1_t *contents)
{
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        result.appendFormat("[%zu] type=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, "
                "{%7.1f,%7.1f,%7.1f,%7.1f}, {%d,%d,%d,%d}\n",
                i,
                layer.compositionType, layer.flags, layer.handle, layer.transform,
                layer.blending,
                layer.sourceCropf.left,
                layer.sourceCropf.top,
                layer.sourceCropf.right,
                layer.sourceCropf.bottom,
                layer.displayFrame.left,
                layer.displayFrame.top,
                layer.displayFrame.right,
                layer.displayFrame.bottom);

    }
}

int ExynosDisplay::checkConfigValidation(decon_win_config *config)
{
    bool flagValidConfig = true;
    for (size_t i = 0; i < MAX_DECON_WIN; i++) {
        if (config[i].state != config[i].DECON_WIN_STATE_DISABLED) {
            /* multiple dma mapping */
            for (size_t j = (i+1); j < MAX_DECON_WIN; j++) {
                if ((config[i].state == config[i].DECON_WIN_STATE_BUFFER) &&
                    (config[j].state == config[j].DECON_WIN_STATE_BUFFER)) {
                    if (config[i].idma_type == config[j].idma_type) {
                        ALOGE("WIN_CONFIG error: duplicated dma(%d) between win%d, win%d",
                                config[i].idma_type, i, j);
                        config[j].state = config[j].DECON_WIN_STATE_DISABLED;
                        flagValidConfig = false;
                    }
                }
            }
            if ((config[i].src.x < 0) || (config[i].src.y < 0)||
                (config[i].dst.x < 0) || (config[i].dst.y < 0)||
                (config[i].dst.x + config[i].dst.w > (uint32_t)mXres) ||
                (config[i].dst.y + config[i].dst.h > (uint32_t)mYres)) {
                ALOGE("WIN_CONFIG error: invalid pos or size win%d", i);
                config[i].state = config[i].DECON_WIN_STATE_DISABLED;
                flagValidConfig = false;
            }

            if (i >= NUM_HW_WINDOWS) {
                ALOGE("WIN_CONFIG error: invalid window number win%d", i);
                config[i].state = config[i].DECON_WIN_STATE_DISABLED;
                flagValidConfig = false;
            }
        }
    }

    if (flagValidConfig)
        return 0;
    else
        return -1;
}

int ExynosDisplay::setPowerMode(int mode)
{
    return ioctl(this->mDisplayFd, S3CFB_POWER_MODE, &mode);
}
