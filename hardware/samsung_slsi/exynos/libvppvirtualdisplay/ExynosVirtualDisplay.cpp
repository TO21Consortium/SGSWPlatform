#define ATRACE_TAG ATRACE_TAG_GRAPHICS

//#define LOG_NDEBUG 0
#define LOG_TAG "virtualdisplay"
#include "exynos_format.h"
#include "ExynosHWC.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"
#include "ExynosVirtualDisplay.h"
#include "decon_tv.h"
#ifdef USES_DISABLE_COMPOSITIONTYPE_GLES
#include "ExynosPrimaryDisplay.h"
#endif
#include <errno.h>
#include <utils/Trace.h>

#ifdef USE_VIDEO_EXT_FOR_WFD_DRM
#define WAIT_COUNT_FOR_ISOLATION 50
#endif

ExynosVirtualDisplay::ExynosVirtualDisplay(struct exynos5_hwc_composer_device_1_t *pdev) :
    ExynosDisplay(EXYNOS_VIRTUAL_DISPLAY, pdev),
    mWidth(0),
    mHeight(0),
    mDisplayWidth(0),
    mDisplayHeight(0),
    mIsWFDState(false),
    mIsRotationState(false),
    mPresentationMode(0),
    mDeviceOrientation(0),
    mFrameBufferTargetTransform(0),
    mCompositionType(COMPOSITION_GLES),
    mPrevCompositionType(COMPOSITION_GLES),
    mGLESFormat(HAL_PIXEL_FORMAT_RGBA_8888),
    mSinkUsage(GRALLOC_USAGE_HW_COMPOSER),
    mIsSecureDRM(false),
    mIsNormalDRM(false)
#ifdef USE_VIDEO_EXT_FOR_WFD_DRM
    , mSMemFd(-1),
    mSMemProtected(false),
    mReserveMemFd(-1),
    mFlagIONBufferAllocated(false)
#endif
{
    mXres = 0;
    mYres = 0;
    mXdpi = 0;
    mYdpi = 0;
#ifdef USES_DISABLE_COMPOSITIONTYPE_GLES
    mExternalMPPforCSC = new ExynosMPPModule(this, MPP_MSC, 0);
    mExternalMPPforCSC->setAllocDevice(pdev->primaryDisplay->mAllocDevice);
#endif

    mOverlayLayer = NULL;
    mFBTargetLayer = NULL;
    memset(mFBLayer, 0x0, sizeof(hwc_layer_1_t *) * NUM_FRAME_BUFFER);
    mNumFB = 0;

#ifdef USES_VDS_BGRA8888
    mForceDoubleOperation = false;
    mExternalMPPDstFormat = HAL_PIXEL_FORMAT_RGBA_8888;
#endif

#ifdef USE_VIDEO_EXT_FOR_WFD_DRM
    mSMemFd = open(SMEM_PATH, O_RDWR);
    if (mSMemFd < 0)
        ALOGE("Fail to open smem_fd %s, error(%d)", SMEM_PATH, mSMemFd);
    else
        ALOGI("Open %s", SMEM_PATH);

    mReserveMemFd = open(HDMI_RESERVE_MEM_DEV_NAME, O_RDWR);
    if (mReserveMemFd < 0)
        ALOGE("Fail to open hdmi_reserve_mem_fd %s, error(%d)", HDMI_RESERVE_MEM_DEV_NAME, mReserveMemFd);
    else
        ALOGI("Open %s", HDMI_RESERVE_MEM_DEV_NAME);
#endif
}

ExynosVirtualDisplay::~ExynosVirtualDisplay()
{
#ifdef USE_VIDEO_EXT_FOR_WFD_DRM
    if (mSMemFd > 0)
        close(mSMemFd);
    if (mReserveMemFd > 0)
        close(mReserveMemFd);
#endif
}

void ExynosVirtualDisplay::allocateLayerInfos(hwc_display_contents_1_t* contents)
{
    ExynosDisplay::allocateLayerInfos(contents);

}

void ExynosVirtualDisplay::setSinkBufferUsage()
{
    ALOGV("setSinkBufferUsage() mSinkUsage 0x%x, mIsSecureDRM %d, mIsNormalDRM %d",
        mSinkUsage, mIsSecureDRM, mIsNormalDRM);
    mSinkUsage = GRALLOC_USAGE_HW_COMPOSER;

    if (mIsSecureDRM) {
        mSinkUsage |= GRALLOC_USAGE_SW_READ_NEVER |
            GRALLOC_USAGE_SW_WRITE_NEVER |
            GRALLOC_USAGE_PROTECTED;
#ifdef USE_VIDEO_EXT_FOR_WFD_DRM
        if (mReserveMemFd > 0) {
            mSinkUsage |= GRALLOC_USAGE_VIDEO_EXT;
            if (!mSMemProtected) {
                setMemoryProtection(1);
                mSMemProtected = true;
            }
        }
#endif
    } else if (mIsNormalDRM)
        mSinkUsage |= GRALLOC_USAGE_PRIVATE_NONSECURE;

#ifdef USE_VIDEO_EXT_FOR_WFD_DRM
    if (mReserveMemFd > 0 && !mIsSecureDRM && mSMemProtected) {
        setMemoryProtection(0);
        mSMemProtected = false;
    }
#endif

    ALOGV("Sink Buffer's Usage: 0x%x", mSinkUsage);
}

int ExynosVirtualDisplay::prepare(hwc_display_contents_1_t* contents)
{
    ATRACE_CALL();
    ALOGV("prepare %u layers for virtual, outbuf %p", contents->numHwLayers, contents->outbuf);

    int ret = 0;
    mCompositionType = COMPOSITION_GLES;
    mOverlayLayer = NULL;
    mFBTargetLayer = NULL;
    mNumFB = 0;

    determineSkipLayer(contents);

    /* determine composition type */
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.compositionType == HWC_FRAMEBUFFER) {
            mNumFB++;
            if (!layer.handle)
                continue;

            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
            ALOGV("FB layer %d  f=%x, w=%d, h=%d, s=%d, vs=%d, "
                  "{%.1f,%.1f,%.1f,%.1f}, {%d,%d,%d,%d}"
                  "type=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x",
                    i, h->format, h->width, h->height, h->stride, h->vstride,
                    layer.sourceCropf.left, layer.sourceCropf.top,
                    layer.sourceCropf.right, layer.sourceCropf.bottom,
                    layer.displayFrame.left, layer.displayFrame.top,
                    layer.displayFrame.right, layer.displayFrame.bottom,
                    layer.compositionType, layer.flags, layer.handle, layer.transform, layer.blending);
        }

        if (layer.compositionType == HWC_OVERLAY) {
            if (!layer.handle)
                continue;

            if (layer.flags & HWC_SKIP_RENDERING) {
                layer.releaseFenceFd = layer.acquireFenceFd;
                continue;
            }

            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
            ALOGV("Overlay layer %d  f=%x, w=%d, h=%d, s=%d, vs=%d, "
                  "{%.1f,%.1f,%.1f,%.1f}, {%d,%d,%d,%d}"
                  "type=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x",
                    i, h->format, h->width, h->height, h->stride, h->vstride,
                    layer.sourceCropf.left, layer.sourceCropf.top,
                    layer.sourceCropf.right, layer.sourceCropf.bottom,
                    layer.displayFrame.left, layer.displayFrame.top,
                    layer.displayFrame.right, layer.displayFrame.bottom,
                    layer.compositionType, layer.flags, layer.handle, layer.transform, layer.blending);

            mOverlayLayer = &layer;
            continue;
        }

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            if (!layer.handle)
                continue;

            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
            ALOGV("FB target layer %d  f=%x, w=%d, h=%d, s=%d, vs=%d, "
                  "{%.1f,%.1f,%.1f,%.1f}, {%d,%d,%d,%d}"
                  "type=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x",
                    i, h->format, h->width, h->height, h->stride, h->vstride,
                    layer.sourceCropf.left, layer.sourceCropf.top,
                    layer.sourceCropf.right, layer.sourceCropf.bottom,
                    layer.displayFrame.left, layer.displayFrame.top,
                    layer.displayFrame.right, layer.displayFrame.bottom,
                    layer.compositionType, layer.flags, layer.handle, layer.transform, layer.blending);

            mFBTargetLayer = &layer;
            continue;
        }
    }

    if (mOverlayLayer && (mNumFB > 0))
        mCompositionType = COMPOSITION_MIXED;
    else if (mOverlayLayer || mIsRotationState)
        mCompositionType = COMPOSITION_HWC;
    ALOGV("mCompositionType 0x%x, mPrevCompositionType 0x%x, overlay_layer 0x%p, mNumFB %d",
        mCompositionType, mPrevCompositionType, mOverlayLayer, mNumFB);

    if (mCompositionType == COMPOSITION_GLES)
        blank();
    else
        unblank();
#ifdef USES_DISABLE_COMPOSITIONTYPE_GLES
    if (mCompositionType == COMPOSITION_GLES) {
        mHwc->mVirtualDisplayRect.left = 0;
        mHwc->mVirtualDisplayRect.top = 0;
        mHwc->mVirtualDisplayRect.width = mWidth;
        mHwc->mVirtualDisplayRect.height = mHeight;
        mCompositionType = COMPOSITION_MIXED;
    }
#endif

#ifdef USES_VDS_BGRA8888
    if (mCompositionType != COMPOSITION_GLES && contents->outbuf) {
        private_handle_t *outbuf_handle = private_handle_t::dynamicCast(contents->outbuf);
        if (outbuf_handle) {
            mGLESFormat = outbuf_handle->format;
        }
    }
#endif

    setSinkBufferUsage();

    return 0;
}

void ExynosVirtualDisplay::configureHandle(private_handle_t *handle, size_t index,
        hwc_layer_1_t &layer, int fence_fd, decon_win_config &cfg)
{
    ExynosDisplay::configureHandle(handle, index, layer, fence_fd, cfg);
#ifdef USES_VDS_BGRA8888
    if (mForceDoubleOperation == true && handle->format == HAL_PIXEL_FORMAT_RGBA_8888)
        cfg.format = halFormatToS3CFormat(HAL_PIXEL_FORMAT_BGRA_8888);
#endif
}

void ExynosVirtualDisplay::configureWriteBack(hwc_display_contents_1_t __unused *contents,
        decon_win_config_data __unused &win_data)
{
}

int ExynosVirtualDisplay::postFrame(hwc_display_contents_1_t* contents)
{
#ifdef USES_VIRTUAL_DISPLAY_DECON_EXT_WB
    ATRACE_CALL();
    struct decon_win_config_data win_data;
    struct decon_win_config *config = win_data.config;
    int win_map = 0;
    int tot_ovly_wins = 0;
    bool hasSecureLayer = false;

    memset(mLastHandles, 0, sizeof(mLastHandles));
    memset(mLastMPPMap, 0, sizeof(mLastMPPMap));
    memset(config, 0, sizeof(win_data.config));

    if (contents->outbuf) {
        configureWriteBack(contents, win_data);
    }

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        config[i].fence_fd = -1;
        mLastMPPMap[i].internal_mpp.type = -1;
        mLastMPPMap[i].external_mpp.type = -1;
    }

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        private_handle_t *handle = NULL;
        if (layer.handle != NULL)
            handle = private_handle_t::dynamicCast(layer.handle);
        int32_t window_index = mLayerInfos[i]->mWindowIndex + DECON_EXT_BASE_WINDOW;

        if ((layer.flags & HWC_SKIP_RENDERING) || handle == NULL ||
            ((layer.compositionType == HWC_OVERLAY) &&
             ((window_index < 0) || (window_index > MAX_DECON_WIN))))  {
            if (layer.acquireFenceFd >= 0)
                close(layer.acquireFenceFd);
            layer.acquireFenceFd = -1;
            layer.releaseFenceFd = -1;

            if ((window_index < 0) || (window_index > MAX_DECON_WIN)) {
                android::String8 result;
                ALOGW("window of layer %d was not assigned (window_index: %d)", i, window_index);
                dumpContents(result, contents);
                ALOGW(result.string());
                result.clear();
                dumpLayerInfo(result);
                ALOGW(result.string());
            }

            continue;
        }

        if ((layer.compositionType == HWC_OVERLAY) ||
            (mFbNeeded == true &&  layer.compositionType == HWC_FRAMEBUFFER_TARGET)) {
            mLastHandles[window_index] = layer.handle;

            if ((getDrmMode(handle->flags) == SECURE_DRM) && layer.compositionType != HWC_FRAMEBUFFER_TARGET) {
                config[window_index].protection = 1;
                hasSecureLayer = true;
            }
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
            ALOGV("blending not supported on window 0; forcing BLENDING_NONE");
            config[window_index].blending = DECON_BLENDING_NONE;
        }
    }

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        ALOGV("window %u configuration:", i);
        dumpConfig(config[i]);
    }

    /* This should be enabled */
#if 0
    if (this->mVirtualOverlayFlag) {
        handleStaticLayers(contents, win_data, tot_ovly_wins);
    }
#endif

    if (mIsSecureDRM != hasSecureLayer) {
        ALOGW("secure state mismatch (mIsSecureDRM: %d, hasSecureLayer: %d)", mIsSecureDRM, hasSecureLayer);
        private_handle_t *outbuf_handle = private_handle_t::dynamicCast(contents->outbuf);
        if (outbuf_handle)
            ALOGW("outbuf format: 0x%x, mCompositionType: %d", outbuf_handle->format, mCompositionType);
    }

    int ret = winconfigIoctl(&win_data);
    ALOGV("ExynosDisplay::postFrame() ioctl(S3CFB_WIN_CONFIG) ret %d", ret);

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++)
        if (config[i].fence_fd != -1)
            close(config[i].fence_fd);
    if (ret < 0) {
        ALOGE("ioctl S3CFB_WIN_CONFIG failed: %s", strerror(errno));
        return ret;
    }
    if (contents->numHwLayers == 1) {
        hwc_layer_1_t &layer = contents->hwLayers[0];
        if (layer.acquireFenceFd >= 0)
            close(layer.acquireFenceFd);
    }

    memcpy(&(this->mLastConfigData), &win_data, sizeof(win_data));

    if (!this->mVirtualOverlayFlag)
        this->mLastFbWindow = mFbWindow;

    return win_data.fence;
#else
    return 0;
#endif
}

void ExynosVirtualDisplay::processGles(hwc_display_contents_1_t* contents)
{
    ALOGV("processGles, mFBTargetLayer->acquireFenceFd %d, mFBTargetLayer->releaseFenceFd %d, contents->outbufAcquireFenceFd %d",
        mFBTargetLayer->acquireFenceFd, mFBTargetLayer->releaseFenceFd, contents->outbufAcquireFenceFd);
    int ret = 0;
    if (mFBTargetLayer != NULL && mFBTargetLayer->acquireFenceFd >= 0)
        contents->retireFenceFd = mFBTargetLayer->acquireFenceFd;

    if (contents->outbufAcquireFenceFd >= 0) {
        close(contents->outbufAcquireFenceFd);
        contents->outbufAcquireFenceFd = -1;
    }
}

void ExynosVirtualDisplay::processHwc(hwc_display_contents_1_t* contents)
{
    ALOGV("processHwc, mFBTargetLayer->acquireFenceFd %d, mFBTargetLayer->releaseFenceFd %d, contents->outbufAcquireFenceFd %d",
        mFBTargetLayer->acquireFenceFd, mFBTargetLayer->releaseFenceFd, contents->outbufAcquireFenceFd);

    ExynosDisplay::set(contents);

    if (contents->outbufAcquireFenceFd >= 0) {
        close(contents->outbufAcquireFenceFd);
        contents->outbufAcquireFenceFd = -1;
    }
}

void ExynosVirtualDisplay::processMixed(hwc_display_contents_1_t* contents)
{
    ALOGV("processMixed, mFBTargetLayer->acquireFenceFd %d, mFBTargetLayer->releaseFenceFd %d, contents->outbufAcquireFenceFd %d",
        mFBTargetLayer->acquireFenceFd, mFBTargetLayer->releaseFenceFd, contents->outbufAcquireFenceFd);

    ExynosDisplay::set(contents);

    if (contents->outbufAcquireFenceFd >= 0) {
        close(contents->outbufAcquireFenceFd);
        contents->outbufAcquireFenceFd = -1;
    }
}

int ExynosVirtualDisplay::set(hwc_display_contents_1_t* contents)
{
    ALOGV("set %u layers for virtual, mCompositionType %d, contents->outbuf %p",
        contents->numHwLayers, mCompositionType, contents->outbuf);
    mOverlayLayer = NULL;
    mFBTargetLayer = NULL;
    mNumFB = 0;
    int IsNormalDRMWithSkipLayer = false;
    int err = 0;
    private_handle_t *outBufHandle = private_handle_t::dynamicCast(contents->outbuf);

    /* Find normal drm layer with HWC_SKIP_LAYER */
    /* HDCP disabled and normal DRM playback */
     for (size_t i = 0; i < contents->numHwLayers; i++) {
         hwc_layer_1_t &layer = contents->hwLayers[i];
         if (layer.flags & HWC_SKIP_LAYER) {
             ALOGV("skipping layer %d", i);
             if (layer.handle) {
                 private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
                 if (getDrmMode(h->flags) == NORMAL_DRM && mIsWFDState) {
                     ALOGV("skipped normal drm layer %d", i);
                     IsNormalDRMWithSkipLayer = true;
                 }
             }
             continue;
         }
    }

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.compositionType == HWC_FRAMEBUFFER) {
            if (!layer.handle)
                continue;
            ALOGV("framebuffer layer %d", i);
            mNumFB++;
        }

        if (layer.compositionType == HWC_OVERLAY) {
            if (!layer.handle)
                continue;

            if (layer.flags & HWC_SKIP_RENDERING) {
                layer.releaseFenceFd = layer.acquireFenceFd;
                continue;
            }

            if (outBufHandle == NULL && layer.acquireFenceFd != -1) {
                close(layer.acquireFenceFd);
                layer.acquireFenceFd = -1;
                continue;
            }

            ALOGV("overlay layer %d", i);
            mOverlayLayer = &layer;
            continue;
        }

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            if (!layer.handle)
                continue;

            ALOGV("FB target layer %d", i);

            mFBTargetLayer = &layer;
            continue;
        }
    }

    if (outBufHandle == NULL) {
        ALOGE("set, outbuf is invalid, no overlay");
        mOverlayLayer = NULL;
        mCompositionType = COMPOSITION_GLES;
    }

    if (mFBTargetLayer && IsNormalDRMWithSkipLayer) {
        if (mFBTargetLayer->acquireFenceFd >= 0)
            contents->retireFenceFd = mFBTargetLayer->acquireFenceFd;
        if (contents->outbufAcquireFenceFd >= 0) {
            close(contents->outbufAcquireFenceFd);
            contents->outbufAcquireFenceFd = -1;
        }
    } else if (mFBTargetLayer && mCompositionType == COMPOSITION_GLES) {
        processGles(contents);
    } else if (mFBTargetLayer && mOverlayLayer && mNumFB > 0 && mCompositionType == COMPOSITION_MIXED) {
        processMixed(contents);
    } else if (mOverlayLayer) {
        processHwc(contents);
#ifdef USES_DISABLE_COMPOSITIONTYPE_GLES
    } else if (mOverlayLayer == NULL && mCompositionType == COMPOSITION_MIXED) {
        ALOGV("Use Scaler to copy from SCRATCH buffer to SINK buffer");
        int ret = 0;
        buffer_handle_t tempBuffers = mExternalMPPforCSC->mDstBuffers[mExternalMPPforCSC->mCurrentBuf];
        int tempFence = mExternalMPPforCSC->mDstBufFence[mExternalMPPforCSC->mCurrentBuf];

        mExternalMPPforCSC->mDstBuffers[mExternalMPPforCSC->mCurrentBuf] = contents->outbuf;
        mExternalMPPforCSC->mDstBufFence[mExternalMPPforCSC->mCurrentBuf] = contents->outbufAcquireFenceFd;

        ret = mExternalMPPforCSC->processM2M(*mFBTargetLayer, outBufHandle->format, NULL, false);
        if (ret < 0)
            ALOGE("failed to configure scaler");
        contents->outbufAcquireFenceFd = -1;
        contents->retireFenceFd = mExternalMPPforCSC->mDstConfig.releaseFenceFd;
        mExternalMPPforCSC->mDstBuffers[mExternalMPPforCSC->mCurrentBuf] = tempBuffers;
        mExternalMPPforCSC->mDstBufFence[mExternalMPPforCSC->mCurrentBuf] = tempFence;

        if (contents->outbufAcquireFenceFd >= 0) {
            close(contents->outbufAcquireFenceFd);
            contents->outbufAcquireFenceFd = -1;
        }
#endif
    } else {
        ALOGV("animation layer skip");
        if (mFBTargetLayer && mFBTargetLayer->acquireFenceFd >= 0)
            contents->retireFenceFd = mFBTargetLayer->acquireFenceFd;
        if (contents->outbufAcquireFenceFd >= 0) {
            close(contents->outbufAcquireFenceFd);
            contents->outbufAcquireFenceFd = -1;
        }
    }

    mPrevCompositionType = mCompositionType;

    return err;
}

#ifdef USES_VDS_OTHERFORMAT
bool ExynosVirtualDisplay::isSupportGLESformat()
{
    return false;
}
#endif

void ExynosVirtualDisplay::determineSkipLayer(hwc_display_contents_1_t *contents)
{
    /* If there is rotation animation layer, */
    /* all layer is HWC_OVERLAY and HWC_SKIP_RENDERING */
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            continue;
        }

        if (mIsRotationState) {
            // normal layers can be skip layer.
            if (layer.handle) {
                private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
                if (getDrmMode(h->flags) != NO_DRM)
                    continue;
            }
            layer.compositionType = HWC_OVERLAY;
            layer.flags = HWC_SKIP_RENDERING;
        }

        if (!layer.handle)
            layer.compositionType = HWC_FRAMEBUFFER;
    }
}

void ExynosVirtualDisplay::determineYuvOverlay(hwc_display_contents_1_t *contents)
{
    ALOGV("ExynosVirtualDisplay::determineYuvOverlay");

    mForceOverlayLayerIndex = -1;
    mHasDrmSurface = false;
    mYuvLayers = 0;
    mIsRotationState = false;
    mIsSecureDRM = false;
    mIsNormalDRM = false;
    bool useVPPOverlayFlag = false;

    if (mDisplayFd < 0) {
        ALOGE("determineYuvOverlay, mDisplayFd is invalid , no overlay");
        return;
    }

#ifdef USES_VDS_OTHERFORMAT
    if (!isSupportGLESformat()) {
        ALOGE("determineYuvOverlay, GLES format is not suppoted, no overlay");
        return;
    }
#endif

    /* find rotation animation layer */
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.flags & HWC_SCREENSHOT_ANIMATOR_LAYER) {
            ALOGV("include rotation animation layer");
            mIsRotationState = true;
            return;
        }
    }

    private_handle_t *outBufHandle = private_handle_t::dynamicCast(contents->outbuf);
    if (outBufHandle == NULL) {
        ALOGE("determineYuvOverlay, outbuf is invalid, no overlay");
        return;
    }

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        ExynosMPPModule* supportedInternalMPP = NULL;
        ExynosMPPModule* supportedExternalMPP = NULL;
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET)
            continue;

        useVPPOverlayFlag = false;
        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

            if (getDrmMode(handle->flags) != NO_DRM) {
                useVPPOverlayFlag = true;
                layer.flags &= ~HWC_SKIP_RENDERING;
            }

            /* check yuv surface */
            if (!mForceFb && !isFormatRgb(handle->format)) {

                /* HACK: force integer source crop */
                layer.sourceCropf.top = floor(layer.sourceCropf.top);
                layer.sourceCropf.left = floor(layer.sourceCropf.left);
                layer.sourceCropf.bottom = ceil(layer.sourceCropf.bottom);
                layer.sourceCropf.right = ceil(layer.sourceCropf.right);

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

                        if (getDrmMode(handle->flags) == SECURE_DRM) {
                            mIsSecureDRM = true;
                            mOverlayLayer = &layer;
                            calcDisplayRect(layer);
                            ALOGV("include secure drm layer");
                        }
                        if (getDrmMode(handle->flags) == NORMAL_DRM) {
                            mIsNormalDRM = true;
                            mOverlayLayer = &layer;
                            calcDisplayRect(layer);
                            ALOGV("include normal drm layer");
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
                if (getDrmMode(handle->flags) != NO_DRM) {
                    this->mHasDrmSurface = true;
                    mForceOverlayLayerIndex = i;
                }
            }
        }
    }
}

void ExynosVirtualDisplay::determineSupportedOverlays(hwc_display_contents_1_t *contents)
{
    ALOGV("ExynosVirtualDisplay::determineSupportedOverlays");

    if (mDisplayFd < 0) {
        ALOGE("determineSupportedOverlays, mDisplayFd is invalid , no overlay");
        return;
    }

#ifdef USES_VDS_OTHERFORMAT
    if (!isSupportGLESformat()) {
        ALOGE("determineSupportedOverlays, GLES format is not suppoted, no overlay");
        return;
    }
#endif

    if (mIsRotationState)
        return;

    private_handle_t *outBufHandle = private_handle_t::dynamicCast(contents->outbuf);
    if (outBufHandle == NULL) {
        ALOGE("determineSupportedOverlays, outbuf is invalid, no overlay");
        return;
    }

    ExynosDisplay::determineSupportedOverlays(contents);
}

void ExynosVirtualDisplay::determineBandwidthSupport(hwc_display_contents_1_t *contents)
{
    ALOGV("ExynosVirtualDisplay::determineBandwidthSupport");

    if (mDisplayFd < 0) {
        ALOGE("determineBandwidthSupport, mDisplayFd is invalid , no overlay");
        return;
    }

#ifdef USES_VDS_OTHERFORMAT
    if (!isSupportGLESformat()) {
        ALOGE("determineBandwidthSupport, GLES format is not suppoted, no overlay");
        return;
    }
#endif

    if (mIsRotationState)
        return;

    private_handle_t *outBufHandle = private_handle_t::dynamicCast(contents->outbuf);
    if (outBufHandle == NULL) {
        ALOGE("determineBandwidthSupport, outbuf is invalid, no overlay");
        return;
    }

    ExynosDisplay::determineBandwidthSupport(contents);
}

#ifdef USES_VDS_BGRA8888
bool ExynosVirtualDisplay::isBothMPPProcessingRequired(hwc_layer_1_t &layer)
{
    if (mForceDoubleOperation)
        return true;
    else
        return ExynosDisplay::isBothMPPProcessingRequired(layer);
}
#endif

void ExynosVirtualDisplay::calcDisplayRect(hwc_layer_1_t &layer)
{
    bool needToTransform = false;
    unsigned int newTransform = 0;
    unsigned int calc_w = (mWidth - mDisplayWidth) >> 1;
    unsigned int calc_h = (mHeight - mDisplayHeight) >> 1;

    if (layer.compositionType) {
        if (mPresentationMode) {
            /* Use EXTERNAL_TB directly (DRM-extention) */
            newTransform = layer.transform;
            needToTransform = false;
        } else if (mFrameBufferTargetTransform) {
            switch(mFrameBufferTargetTransform) {
            case HAL_TRANSFORM_ROT_90:
                newTransform = 0;
                needToTransform = true;
                break;
            case HAL_TRANSFORM_ROT_180:
                newTransform = HAL_TRANSFORM_ROT_90;
                needToTransform = false;
                break;
            case HAL_TRANSFORM_ROT_270:
                newTransform = HAL_TRANSFORM_ROT_180;
                needToTransform = true;
                break;
            default:
                newTransform = 0;
                needToTransform = false;
                break;
            }
        } else {
            switch(mDeviceOrientation) {
            case 1: /* HAL_TRANSFORM_ROT_90 */
                newTransform = HAL_TRANSFORM_ROT_270;
                needToTransform = false;
                break;
            case 3: /* HAL_TRANSFORM_ROT_270 */
                newTransform = HAL_TRANSFORM_ROT_90;
                needToTransform = false;
                break;
            default: /* Default | HAL_TRANSFORM_ROT_180 */
                newTransform = 0;
                needToTransform = false;
                break;
            }
        }

        if (layer.compositionType == HWC_OVERLAY) {
            if (needToTransform) {
                mHwc->mVirtualDisplayRect.left = layer.displayFrame.left + calc_h;
                mHwc->mVirtualDisplayRect.top = layer.displayFrame.top + calc_w;
                mHwc->mVirtualDisplayRect.width = WIDTH(layer.displayFrame) - (calc_h << 1);
                mHwc->mVirtualDisplayRect.height = HEIGHT(layer.displayFrame) - (calc_w << 1);
            } else {
                mHwc->mVirtualDisplayRect.left = layer.displayFrame.left + calc_w;
                mHwc->mVirtualDisplayRect.top = layer.displayFrame.top + calc_h;
                mHwc->mVirtualDisplayRect.width = WIDTH(layer.displayFrame) - (calc_w << 1);
                mHwc->mVirtualDisplayRect.height = HEIGHT(layer.displayFrame) - (calc_h << 1);
            }

            if (layer.displayFrame.left < 0 || layer.displayFrame.top < 0 ||
                mWidth < (unsigned int)WIDTH(layer.displayFrame) || mHeight < (unsigned int)HEIGHT(layer.displayFrame)) {
                if (needToTransform) {
                    mHwc->mVirtualDisplayRect.left = 0 + calc_h;
                    mHwc->mVirtualDisplayRect.top = 0 + calc_w;

                    mHwc->mVirtualDisplayRect.width = mWidth - (calc_h << 1);
                    mHwc->mVirtualDisplayRect.height = mHeight - (calc_w << 1);
                } else {
                    mHwc->mVirtualDisplayRect.left = 0 + calc_w;
                    mHwc->mVirtualDisplayRect.top = 0 + calc_h;

                    mHwc->mVirtualDisplayRect.width = mWidth - (calc_w << 1);
                    mHwc->mVirtualDisplayRect.height = mHeight - (calc_h << 1);
                }
            }
        } else { /* HWC_FRAMEBUFFER_TARGET */
            if (needToTransform) {
                mHwc->mVirtualDisplayRect.width = (mDisplayHeight * mDisplayHeight) / mDisplayWidth;
                mHwc->mVirtualDisplayRect.height = mDisplayHeight;
                mHwc->mVirtualDisplayRect.left = (mDisplayWidth - mHwc->mVirtualDisplayRect.width) / 2;
                mHwc->mVirtualDisplayRect.top = 0;
            } else {
                mHwc->mVirtualDisplayRect.left = 0;
                mHwc->mVirtualDisplayRect.top = 0;
                mHwc->mVirtualDisplayRect.width = mDisplayWidth;
                mHwc->mVirtualDisplayRect.height = mDisplayHeight;
            }
        }
    }
}

void ExynosVirtualDisplay::init(hwc_display_contents_1_t __unused *contents)
{
    ALOGV("ExynosVirtualDisplay::init() mDisplayFd %d", mDisplayFd);

#ifdef USES_VDS_BGRA8888
    private_handle_t *handle = private_handle_t::dynamicCast(contents->outbuf);
    if (handle) {
        if (handle->format == HAL_PIXEL_FORMAT_BGRA_8888) {
            mForceDoubleOperation = false;
        } else {
            mForceDoubleOperation = true;
        }
    }
    ALOGV("ExynosVirtualDisplay::init() mForceDoubleOperation %d", mForceDoubleOperation);
#endif

#ifdef USES_VIRTUAL_DISPLAY_DECON_EXT_WB
    int ret = 0;
    if (mDisplayFd < 0) {
        mDisplayFd = open(DECON_WB_DEV_NAME, O_RDWR);
        if (mDisplayFd < 0) {
            ALOGE("failed to open decon ext wb for virtualDisplay");
        } else {
            ALOGD("open fd for WFD(%d)", mDisplayFd);
            int subdev_fd;
            struct v4l2_subdev_format sd_fmt;
            char devname[32];

            sd_fmt.pad   = DECON_PAD_WB;
            sd_fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
            sd_fmt.format.width  = mWidth;
            sd_fmt.format.height = mHeight;

            snprintf(devname, sizeof(devname), DECON_WB_SUBDEV_NAME);

            subdev_fd = exynos_subdev_open_devname(devname, O_RDWR);
            if (subdev_fd < 0)
                ALOGE("failed to open subdev for virtualDisplay");
            else {
                ret = exynos_subdev_s_fmt(subdev_fd, &sd_fmt);
                close(subdev_fd);
                if (ret < 0) {
                    ALOGE("failed to subdev s_fmt for virtualDisplay");
                    close(mDisplayFd);
                    mDisplayFd = -1;
                }
            }
        }
    }
#endif
}

void ExynosVirtualDisplay::deInit()
{
    ALOGV("ExynosVirtualDisplay::deInit() ,mDisplayFd %d", mDisplayFd);

    blank();

    if (mDisplayFd > 0) {
        close(mDisplayFd);
        ALOGD("Close fd for WFD");
    }
    mDisplayFd = -1;
}

#ifdef USE_VIDEO_EXT_FOR_WFD_DRM
void ExynosVirtualDisplay::requestIONMemory()
{
    ALOGV("ExynosVirtualDisplay::requestIONMemory()");
    /* prepare reserved memory */
    if (mReserveMemFd > 0) {
        unsigned int value;
        int waitCount = WAIT_COUNT_FOR_ISOLATION;
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        int err = lseek(mReserveMemFd, 0, SEEK_SET);
        err = read(mReserveMemFd, buffer, sizeof(buffer));
        value = atoi(buffer);

        if ((err > 0) && (value == 0)) {
            memset(buffer, 0, sizeof(buffer));
            buffer[0] = '2';
            if (write(mReserveMemFd, buffer, sizeof(buffer)) < 0)
                ALOGE("fail to request isolation of memmory for WFD");
            else {
                while(!checkIONBufferPrepared()) {
                    usleep(20000);
                    if (--waitCount)
                        ALOGV("wait count for isolation: %d", waitCount);
                    else
                        ALOGE("fail to request isolation (time out)");
                }
                ALOGI("video ext ion memory for WFD was isolated");
            }
        } else {
            if (err < 0)
                ALOGE("fail to read mReserveMemFd");
            else
                ALOGE("ion memmory for WFD is isolated already");
        }
    }
}

void ExynosVirtualDisplay::freeIONMemory()
{
    ALOGV("ExynosVirtualDisplay::freeIONMemory()");
    if (mReserveMemFd > 0 && mFlagIONBufferAllocated) {
        unsigned int value;
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        int err = lseek(mReserveMemFd, 0, SEEK_SET);
        err = read(mReserveMemFd, buffer, sizeof(buffer));
        value = atoi(buffer);
        if ((err > 0) && (value == 1)) {
            setMemoryProtection(0);
            memset(buffer, 0, sizeof(buffer));
            buffer[0] = '0';
            if (write(mReserveMemFd, buffer, sizeof(buffer)) < 0)
                ALOGE("fail to request isolation of memmory for WFD");
            else
                ALOGI("deisolation of memmory for WFD was requested");
        } else {
            if (err < 0)
                ALOGE("fail to read mReserveMemFd");
            else
                ALOGE("ion memmory for WFD is deisolated already");
        }
        mFlagIONBufferAllocated = false;
    }
}

bool ExynosVirtualDisplay::checkIONBufferPrepared()
{
    ALOGV("ExynosVirtualDisplay::checkIONBufferPrepared()");
    if (mReserveMemFd > 0) {
        unsigned int value;
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        int err = lseek(mReserveMemFd, 0, SEEK_SET);
        err = read(mReserveMemFd, buffer, sizeof(buffer));
        value = atoi(buffer);

        if ((err > 0) && (value == 1)) {
            mFlagIONBufferAllocated = true;
            return true;
        } else {
            mFlagIONBufferAllocated = false;
            return false;
        }
    }
    return false;
}

void ExynosVirtualDisplay::setMemoryProtection(int protection)
{
    ALOGV("ExynosVirtualDisplay::setMemoryProtection() protection %d", protection);
    int ret = 0;

    if (mSMemFd > 0) {
        ret = ioctl(mSMemFd, SECMEM_IOC_SET_VIDEO_EXT_PROC, &protection);
        if (ret < 0)
            ALOGE("Protection failed, ret(%d)", ret);
    }
}
#endif

void ExynosVirtualDisplay::setWFDOutputResolution(unsigned int width, unsigned int height,
        unsigned int disp_w, unsigned int disp_h)
{
    mWidth = width;
    mHeight = height;
    mDisplayWidth = disp_w;
    mDisplayHeight = disp_h;
    mXres = width;
    mYres = height;
}

#ifdef USES_VDS_OTHERFORMAT
void ExynosVirtualDisplay::setVDSGlesFormat(int format)
{
    ALOGV("ExynosVirtualDisplay::setVDSGlesFormat(), format %d", format);
    mGLESFormat = format;
}
#endif

void ExynosVirtualDisplay::setPriContents(hwc_display_contents_1_t __unused *contents)
{

}

int ExynosVirtualDisplay::blank()
{
    ALOGV("ExynosVirtualDisplay::blank(), mDisplayFd %d, mBlanked %d", mDisplayFd, mBlanked);
    int ret = 0;

    if (mDisplayFd > 0 && !mBlanked) {
        ret = ioctl(mDisplayFd, FBIOBLANK, FB_BLANK_POWERDOWN);
        if (ret < 0)
            ALOGE("failed blank for virtualDisplay");
        else
            mBlanked = true;
    }
    return ret;
}

int ExynosVirtualDisplay::unblank()
{
    ALOGV("ExynosVirtualDisplay::unblank(), mDisplayFd %d, mBlanked %d", mDisplayFd, mBlanked);
    int ret = 0;
    if (mDisplayFd > 0 && mBlanked) {
        ret = ioctl(mDisplayFd, FBIOBLANK, FB_BLANK_UNBLANK);
        if (ret < 0)
            ALOGE("failed unblank for virtualDisplay");
        else
            mBlanked = false;
    }
    return ret;
}

int ExynosVirtualDisplay::getConfig()
{
    return 0;
}

int32_t ExynosVirtualDisplay::getDisplayAttributes(const uint32_t __unused attribute)
{
    return 0;
}

