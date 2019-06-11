//#define LOG_NDEBUG 0
#include "ExynosHWC.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"
#include "ExynosExternalDisplay.h"
#include "ExynosSecondaryDisplayModule.h"
#include "decon_tv.h"
#include <errno.h>

extern struct v4l2_dv_timings dv_timings[];
bool is_same_dv_timings(const struct v4l2_dv_timings *t1,
        const struct v4l2_dv_timings *t2)
{
    if (t1->type == t2->type &&
            t1->bt.width == t2->bt.width &&
            t1->bt.height == t2->bt.height &&
            t1->bt.interlaced == t2->bt.interlaced &&
            t1->bt.polarities == t2->bt.polarities &&
            t1->bt.pixelclock == t2->bt.pixelclock &&
            t1->bt.hfrontporch == t2->bt.hfrontporch &&
            t1->bt.vfrontporch == t2->bt.vfrontporch &&
            t1->bt.vsync == t2->bt.vsync &&
            t1->bt.vbackporch == t2->bt.vbackporch &&
            (!t1->bt.interlaced ||
             (t1->bt.il_vfrontporch == t2->bt.il_vfrontporch &&
              t1->bt.il_vsync == t2->bt.il_vsync &&
              t1->bt.il_vbackporch == t2->bt.il_vbackporch)))
        return true;
    return false;
}

int ExynosExternalDisplay::getDVTimingsIndex(int preset)
{
    for (int i = 0; i < SUPPORTED_DV_TIMINGS_NUM; i++) {
        if (preset == preset_index_mappings[i].preset)
            return preset_index_mappings[i].dv_timings_index;
    }
    return -1;
}

ExynosExternalDisplay::ExynosExternalDisplay(struct exynos5_hwc_composer_device_1_t *pdev)
    :   ExynosDisplay(EXYNOS_EXTERNAL_DISPLAY, pdev),
        mEnabled(false),
        mBlanked(false),
        mUseSubtitles(false),
        mReserveMemFd(-1),
        mDRMTempBuffer(NULL),
        mFlagIONBufferAllocated(false)
{
    mXres = 0;
    mYres = 0;
    mXdpi = 0;
    mYdpi = 0;
    mVsyncPeriod = 0;
    mInternalDMAs.add(IDMA_G3);
    mReserveMemFd = open(HDMI_RESERVE_MEM_DEV_NAME, O_RDWR);
    if (mReserveMemFd < 0)
        ALOGE("Fail to open hdmi_reserve_mem_fd %s, error(%d)", HDMI_RESERVE_MEM_DEV_NAME, mReserveMemFd);
    else
        ALOGI("Open %s", HDMI_RESERVE_MEM_DEV_NAME);
}

ExynosExternalDisplay::~ExynosExternalDisplay()
{
    if (mDRMTempBuffer != NULL) {
        mAllocDevice->free(mAllocDevice, mDRMTempBuffer);
        mDRMTempBuffer = NULL;
    }
    if (mReserveMemFd > 0)
        close(mReserveMemFd);
}

void ExynosExternalDisplay::allocateLayerInfos(hwc_display_contents_1_t* contents)
{
    ExynosDisplay::allocateLayerInfos(contents);
}

int ExynosExternalDisplay::prepare(hwc_display_contents_1_t* contents)
{
    ExynosDisplay::prepare(contents);
    return 0;
}

int ExynosExternalDisplay::postMPPM2M(hwc_layer_1_t &layer, struct decon_win_config *config, int win_map, int index)
{
    int err = 0;
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

    if ((mHasDrmSurface == true) & (getDrmMode(handle->flags) != NO_DRM)) {
        if (checkIONBufferPrepared() == false) {
            ALOGV("skip DRM video");
            handle->flags &= ~GRALLOC_USAGE_VIDEO_EXT;
            err = configureDRMSkipHandle(config[win_map]);
            config[win_map].idma_type = (decon_idma_type)mLayerInfos[index]->mDmaType;
            if (layer.acquireFenceFd >= 0)
                close(layer.acquireFenceFd);
            layer.acquireFenceFd = -1;
            layer.releaseFenceFd = -1;
            return err;
        } else {
            handle->flags |= GRALLOC_USAGE_VIDEO_EXT;
        }
    }

    return ExynosDisplay::postMPPM2M(layer, config, win_map, index);
}

void ExynosExternalDisplay::handleStaticLayers(hwc_display_contents_1_t *contents, struct decon_win_config_data &win_data, int __unused tot_ovly_wins)
{
    int win_map = 0;
    if (mLastFbWindow >= NUM_HW_WINDOWS - 1) {
        ALOGE("handleStaticLayers:: invalid mLastFbWindow(%d)", mLastFbWindow);
        return;
    }
    win_map = mLastFbWindow + 1;
    ALOGV("[USE] SKIP_STATIC_LAYER_COMP, mLastFbWindow(%d), win_map(%d)\n", mLastFbWindow, win_map);

    memcpy(&win_data.config[win_map],
            &mLastConfigData.config[win_map], sizeof(struct decon_win_config));
    win_data.config[win_map].fence_fd = -1;

    for (size_t i = mFirstFb; i <= mLastFb; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.compositionType == HWC_OVERLAY) {
            ALOGV("[SKIP_STATIC_LAYER_COMP] %d layer.handle: 0x%p, layer.acquireFenceFd: %d\n", i, layer.handle, layer.acquireFenceFd);
            if (layer.acquireFenceFd >= 0)
                close(layer.acquireFenceFd);
            layer.acquireFenceFd = -1;
            layer.releaseFenceFd = -1;
        }
    }
}

int ExynosExternalDisplay::postFrame(hwc_display_contents_1_t* contents)
{
    struct decon_win_config_data win_data;
    struct decon_win_config *config = win_data.config;
    int win_map = 0;
    int tot_ovly_wins = 0;
    bool hdmiDisabled = false;

    memset(mLastHandles, 0, sizeof(mLastHandles));
    memset(mLastMPPMap, 0, sizeof(mLastMPPMap));
    memset(config, 0, sizeof(win_data.config));

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
        // window 0 is background layer
        size_t window_index = mLayerInfos[i]->mWindowIndex + 1;

        if ((layer.flags & HWC_SKIP_RENDERING) &&
            ((handle == NULL) || (getDrmMode(handle->flags) == NO_DRM)))  {
            if (layer.acquireFenceFd >= 0)
		    close(layer.acquireFenceFd);
            layer.acquireFenceFd = -1;
	    layer.releaseFenceFd = -1;
            continue;
        }

        if ((layer.compositionType == HWC_OVERLAY) ||
            (mFbNeeded == true &&  layer.compositionType == HWC_FRAMEBUFFER_TARGET)) {
            mLastHandles[window_index] = layer.handle;

            if (handle == NULL) {
                if (layer.acquireFenceFd >= 0)
                    close(layer.acquireFenceFd);
                layer.acquireFenceFd = -1;
                layer.releaseFenceFd = -1;
                continue;
            }

            if (getDrmMode(handle->flags) == SECURE_DRM)
                config[window_index].protection = 1;
            else
                config[window_index].protection = 0;

            if ((int)i == mForceOverlayLayerIndex &&
                mHwc->mS3DMode != S3D_MODE_DISABLED && mHwc->mHdmiResolutionChanged) {
                if (isPresetSupported(mHwc->mHdmiPreset)) {
                    mHwc->mS3DMode = S3D_MODE_RUNNING;
                    setPreset(mHwc->mHdmiPreset);
                    /*
                     * HDMI was disabled by setPreset
                     * This frame will be handled from next frame
                     */
                    if (layer.acquireFenceFd >= 0)
                        close(layer.acquireFenceFd);
                    layer.acquireFenceFd = -1;
                    layer.releaseFenceFd = -1;
                    layer.flags = HWC_SKIP_RENDERING;
                    hdmiDisabled = true;
                    continue;
                } else {
                    mHwc->mS3DMode = S3D_MODE_RUNNING;
                    mHwc->mHdmiResolutionChanged = false;
                    mHwc->mHdmiResolutionHandled = true;
                }
            }
            if (mLayerInfos[i]->mInternalMPP != NULL) {
                mLastMPPMap[window_index].internal_mpp.type = mLayerInfos[i]->mInternalMPP->mType;
                mLastMPPMap[window_index].internal_mpp.index = mLayerInfos[i]->mInternalMPP->mIndex;
            }
            if (mLayerInfos[i]->mExternalMPP != NULL) {
                mLastMPPMap[window_index].external_mpp.type = mLayerInfos[i]->mExternalMPP->mType;
                mLastMPPMap[window_index].external_mpp.index = mLayerInfos[i]->mExternalMPP->mIndex;
                if ((int)i == mForceOverlayLayerIndex && mHwc->mS3DMode == S3D_MODE_RUNNING) {
                    if (isPresetSupported(mHwc->mHdmiPreset)) {
                        mLayerInfos[i]->mExternalMPP->mS3DMode = S3D_NONE;
                    } else {
                        int S3DFormat = getS3DFormat(mHwc->mHdmiPreset);
                        if (S3DFormat == S3D_SBS)
                            mLayerInfos[i]->mExternalMPP->mS3DMode = S3D_SBS;
                        else if (S3DFormat == S3D_TB)
                            mLayerInfos[i]->mExternalMPP->mS3DMode = S3D_TB;
                    }
                }
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
        ALOGV("external display: window %u configuration:", i);
        dumpConfig(config[i]);
    }

    if (hdmiDisabled) {
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (layer.acquireFenceFd >= 0)
		    close(layer.acquireFenceFd);
            layer.acquireFenceFd = -1;
	    layer.releaseFenceFd = -1;
        }
        return 0;
    }

    if (this->mVirtualOverlayFlag) {
        handleStaticLayers(contents, win_data, tot_ovly_wins);
    }

    if (contents->numHwLayers == 1) {
        hwc_layer_1_t &layer = contents->hwLayers[0];
        if (layer.acquireFenceFd >= 0)
            close(layer.acquireFenceFd);
        layer.acquireFenceFd = -1;
        layer.releaseFenceFd = -1;
    }

    if (checkConfigChanged(win_data, mLastConfigData) == false)
    {
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            size_t window_index = mLayerInfos[i]->mWindowIndex;

            if ((layer.compositionType == HWC_OVERLAY) ||
                (layer.compositionType == HWC_FRAMEBUFFER_TARGET)) {
                if (layer.acquireFenceFd >= 0)
                    close(layer.acquireFenceFd);

                if (mLayerInfos[i]->mExternalMPP != NULL) {
                    mLayerInfos[i]->mExternalMPP->mCurrentBuf = (mLayerInfos[i]->mExternalMPP->mCurrentBuf + 1) % mLayerInfos[i]->mExternalMPP->mNumAvailableDstBuffers;
                }
            }
        }
        return 0;
    }

    int ret = ioctl(this->mDisplayFd, S3CFB_WIN_CONFIG, &win_data);
    for (size_t i = 0; i < NUM_HW_WINDOWS; i++)
        if (config[i].fence_fd != -1)
            close(config[i].fence_fd);
    if (ret < 0) {
        ALOGE("ioctl S3CFB_WIN_CONFIG failed: %s", strerror(errno));
        return ret;
    }

    memcpy(&(this->mLastConfigData), &win_data, sizeof(win_data));

    if (!this->mVirtualOverlayFlag)
        this->mLastFbWindow = mFbWindow;

    if ((mYuvLayers != 0) && (mDRMTempBuffer != NULL)) {
        mAllocDevice->free(mAllocDevice, mDRMTempBuffer);
        mDRMTempBuffer = NULL;
    }
    return win_data.fence;
}

int ExynosExternalDisplay::set(hwc_display_contents_1_t* contents)
{
    int err = 0;
    bool drm_skipped = false;

    if (!mEnabled || mBlanked) {
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (layer.acquireFenceFd >= 0) {
                close(layer.acquireFenceFd);
                layer.acquireFenceFd = -1;
            }
        }
        return 0;
    }

    if ((mHasDrmSurface == true) && (mForceOverlayLayerIndex != -1) &&
        (mLayerInfos[mForceOverlayLayerIndex]->mExternalMPP != NULL)) {
        hwc_layer_1_t &layer = contents->hwLayers[mForceOverlayLayerIndex];
        if (mFlagIONBufferAllocated == false) {
            layer.flags |= HWC_SKIP_RENDERING;
            drm_skipped = true;
        } else {
            layer.flags &= ~(HWC_SKIP_RENDERING);
        }
    }
    err = ExynosDisplay::set(contents);

    /* HDMI was disabled to change S3D mode */
    if (mEnabled == false)
        return 0;

    /* Restore flags */
    if (drm_skipped) {
        if ((mHasDrmSurface == true) && (mForceOverlayLayerIndex != -1) &&
                (mLayerInfos[mForceOverlayLayerIndex]->mExternalMPP != NULL)) {
            hwc_layer_1_t &layer = contents->hwLayers[mForceOverlayLayerIndex];
            layer.flags &= ~(HWC_SKIP_RENDERING);
        }
    }

    if (this->mYuvLayers == 0 && !mHwc->local_external_display_pause) {
        if (mHwc->mS3DMode == S3D_MODE_RUNNING && contents->numHwLayers > 1) {
            int preset = convert3DTo2D(mHwc->mHdmiCurrentPreset);
            if (isPresetSupported(preset)) {
                ALOGI("S3D video is removed, Set Resolution(%d)", preset);
                setPreset(preset);
                mHwc->mS3DMode = S3D_MODE_STOPPING;
                mHwc->mHdmiPreset = preset;
                if (mHwc->procs)
                    mHwc->procs->invalidate(mHwc->procs);
            } else {
                ALOGI("S3D video is removed, Resolution(%d) is not supported. mHdmiCurrentPreset(%d)", preset, mHwc->mHdmiCurrentPreset);
                mHwc->mS3DMode = S3D_MODE_DISABLED;
                mHwc->mHdmiPreset = mHwc->mHdmiCurrentPreset;
            }
        }
    }

    return err;
}

void ExynosExternalDisplay::determineYuvOverlay(hwc_display_contents_1_t *contents)
{
    mForceOverlayLayerIndex = -1;
    mHasDrmSurface = false;
    mYuvLayers = 0;
    bool useVPPOverlayFlag = false;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        ExynosMPPModule* supportedInternalMPP = NULL;
        ExynosMPPModule* supportedExternalMPP = NULL;

        hwc_layer_1_t &layer = contents->hwLayers[i];
        useVPPOverlayFlag = false;
        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

            if (getDrmMode(handle->flags) != NO_DRM)
                useVPPOverlayFlag = true;
#if defined(GSC_VIDEO)
            /* check yuv surface */
            if (!mForceFb && !isFormatRgb(handle->format)) {
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

                        if ((mHwc->mS3DMode != S3D_MODE_DISABLED) &&
                            mHwc->mHdmiResolutionChanged)
                            mHwc->mS3DMode = S3D_MODE_RUNNING;
                        /* Set destination size as full screen */
                        if (mHwc->mS3DMode != S3D_MODE_DISABLED) {
                            layer.displayFrame.left = 0;
                            layer.displayFrame.top = 0;
                            layer.displayFrame.right = mXres;
                            layer.displayFrame.bottom = mYres;
                        }

                        if ((getDrmMode(handle->flags) != NO_DRM) &&
                                isBothMPPProcessingRequired(layer) &&
                                (supportedInternalMPP != NULL)) {
                            layer.displayFrame.right = layer.displayFrame.left +
                                ALIGN_DOWN(WIDTH(layer.displayFrame), supportedInternalMPP->getCropWidthAlign(layer));
                            layer.displayFrame.bottom = layer.displayFrame.top +
                                ALIGN_DOWN(HEIGHT(layer.displayFrame), supportedInternalMPP->getCropHeightAlign(layer));
                            layer.flags &= ~HWC_SKIP_RENDERING;
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

                if (getDrmMode(handle->flags) != NO_DRM) {
                    this->mHasDrmSurface = true;
                    mForceOverlayLayerIndex = i;
                }
            }
#endif
        }
    }
}


void ExynosExternalDisplay::determineSupportedOverlays(hwc_display_contents_1_t *contents)
{
#if defined(GSC_VIDEO)
    if ((mHwc->mS3DMode != S3D_MODE_DISABLED) && (this->mYuvLayers == 1) && !mUseSubtitles) {
        // UI layers will be skiped when S3D video is playing
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (mForceOverlayLayerIndex != (int)i) {
                layer.compositionType = HWC_OVERLAY;
                layer.flags = HWC_SKIP_RENDERING;
            }
        }
    }
#endif

#if !defined(GSC_VIDEO)
    mForceFb = true;
#endif

    ExynosDisplay::determineSupportedOverlays(contents);
    /*
     * If GSC_VIDEO is not defined,
     * all of layers are GLES except DRM video
     */
}

void ExynosExternalDisplay::configureHandle(private_handle_t *handle, size_t index,
        hwc_layer_1_t &layer, int fence_fd, decon_win_config &cfg)
{
    ExynosDisplay::configureHandle(handle, index, layer, fence_fd, cfg);
    if ((mHwc->mS3DMode == S3D_MODE_RUNNING) &&
        ((int)index == mForceOverlayLayerIndex) &&
        (isPresetSupported(mHwc->mHdmiPreset) == false) &&
        (mLayerInfos[index]->mInternalMPP != NULL) &&
        (mLayerInfos[index]->mExternalMPP == NULL)) {
        int S3DFormat = getS3DFormat(mHwc->mHdmiPreset);
        if (S3DFormat == S3D_SBS)
            cfg.src.w /= 2;
        else if (S3DFormat == S3D_TB)
            cfg.src.h /= 2;
    }
}

int ExynosExternalDisplay::openHdmi()
{
    int ret = 0;
    int sw_fd;

    if (mHwc->externalDisplay->mDisplayFd > 0)
        ret = mHwc->externalDisplay->mDisplayFd;
    else {
        mHwc->externalDisplay->mDisplayFd = open("/dev/graphics/fb1", O_RDWR);
        if (mHwc->externalDisplay->mDisplayFd < 0) {
            ALOGE("failed to open framebuffer for externalDisplay");
        }
        ret = mHwc->externalDisplay->mDisplayFd;
    }

    ALOGD("open fd for HDMI(%d)", ret);

    return ret;
}

void ExynosExternalDisplay::closeHdmi()
{
    if (mDisplayFd > 0) {
        close(mDisplayFd);
        ALOGD("Close fd for HDMI");
    }
    mDisplayFd = -1;
}

void ExynosExternalDisplay::setHdmiStatus(bool status)
{
    if (status) {
#if defined(USES_VIRTUAL_DISPLAY)
        char value[PROPERTY_VALUE_MAX];
        property_get("wlan.wfd.status", value, "disconnected");
        bool bWFDDisconnected = !strcmp(value, "disconnected");

        if (bWFDDisconnected) {
#endif
        if (mEnabled == false && mHwc->mS3DMode != S3D_MODE_DISABLED)
            mHwc->mHdmiResolutionChanged = true;

        if (mEnabled == false)
            requestIONMemory();
        enable();
#if defined(USES_VIRTUAL_DISPLAY)
        }
#endif
    } else {
        disable();
        closeHdmi();

        if (mDRMTempBuffer != NULL) {
            mAllocDevice->free(mAllocDevice, mDRMTempBuffer);
            mDRMTempBuffer = NULL;
        }
    }
}

bool ExynosExternalDisplay::isPresetSupported(unsigned int preset)
{
    bool found = false;
    int index = 0;
    int ret = 0;
    exynos_hdmi_data hdmi_data;
    int dv_timings_index = getDVTimingsIndex(preset);

    if (dv_timings_index < 0) {
        ALOGE("%s: unsupported preset, %d", __func__, preset);
        return -1;
    }

    hdmi_data.state = hdmi_data.EXYNOS_HDMI_STATE_ENUM_PRESET;
    while (true) {
        hdmi_data.etimings.index = index++;
        ret = ioctl(this->mDisplayFd, EXYNOS_GET_HDMI_CONFIG, &hdmi_data);

        if (ret < 0) {
            if (errno == EINVAL)
                break;
            ALOGE("%s: enum_dv_timings error, %d", __func__, errno);
            return -1;
        }

        ALOGV("%s: %d width=%d height=%d",
                __func__, hdmi_data.etimings.index,
                hdmi_data.etimings.timings.bt.width, hdmi_data.etimings.timings.bt.height);

        if (is_same_dv_timings(&hdmi_data.etimings.timings, &dv_timings[dv_timings_index])) {
            mXres  = hdmi_data.etimings.timings.bt.width;
            mYres  = hdmi_data.etimings.timings.bt.height;
            found = true;
            mHwc->mHdmiCurrentPreset = preset;
            break;
        }
    }
    return found;
}

int ExynosExternalDisplay::getConfig()
{
    if (!mHwc->hdmi_hpd)
        return -1;

    exynos_hdmi_data hdmi_data;
    int dv_timings_index = 0;

    hdmi_data.state = hdmi_data.EXYNOS_HDMI_STATE_PRESET;
    if (ioctl(this->mDisplayFd, EXYNOS_GET_HDMI_CONFIG, &hdmi_data) < 0) {
        ALOGE("%s: g_dv_timings error, %d", __func__, errno);
        return -1;
    }

    if (hwcHasApiVersion((hwc_composer_device_1_t*)mHwc, HWC_DEVICE_API_VERSION_1_4) == false)
        mActiveConfigIndex = 0;
    else {
        /*
         * getConfig is called only if cable is connected
         * mActiveConfigIndex is 0 at this time
         */
        mActiveConfigIndex = 0;
    }

    for (int i = 0; i < SUPPORTED_DV_TIMINGS_NUM; i++) {
        dv_timings_index = preset_index_mappings[i].dv_timings_index;
        if (is_same_dv_timings(&hdmi_data.timings, &dv_timings[dv_timings_index])) {
            float refreshRate = (float)((float)hdmi_data.timings.bt.pixelclock /
                    ((hdmi_data.timings.bt.width + hdmi_data.timings.bt.hfrontporch + hdmi_data.timings.bt.hsync + hdmi_data.timings.bt.hbackporch) *
                     (hdmi_data.timings.bt.height + hdmi_data.timings.bt.vfrontporch + hdmi_data.timings.bt.vsync + hdmi_data.timings.bt.vbackporch)));
            mXres = hdmi_data.timings.bt.width;
            mYres = hdmi_data.timings.bt.height;
            mVsyncPeriod = 1000000000 / refreshRate;
            mHwc->mHdmiCurrentPreset = preset_index_mappings[i].preset;
            break;
        }
    }
    ALOGD("HDMI resolution is (%d x %d)", mXres, mYres);

    return 0;
}

int ExynosExternalDisplay::getDisplayConfigs(uint32_t *configs, size_t *numConfigs)
{
    int ret = 0;
    if (!mHwc->hdmi_hpd)
        return -1;

    exynos_hdmi_data hdmi_data;
    size_t index = 0;

    cleanConfigurations();

    /* configs store the index of mConfigurations */
    hdmi_data.state = hdmi_data.EXYNOS_HDMI_STATE_ENUM_PRESET;
    while (index < (*numConfigs)) {
        hdmi_data.etimings.index = index;
        ret = ioctl(this->mDisplayFd, EXYNOS_GET_HDMI_CONFIG, &hdmi_data);

        if (ret < 0) {
            if (errno == EINVAL) {
                ALOGI("%s:: Total configurations %d", __func__, index);
                break;
            }
            ALOGE("%s: enum_dv_timings error, %d", __func__, errno);
            return -1;
        }

        for (size_t i = 0; i < SUPPORTED_DV_TIMINGS_NUM; i++) {
            int dv_timings_index = preset_index_mappings[i].dv_timings_index;
            if (is_same_dv_timings(&hdmi_data.etimings.timings, &dv_timings[dv_timings_index])) {
                mConfigurations.push_back(dv_timings_index);
                configs[mConfigurations.size() - 1] = dv_timings_index;
                break;
            }
        }
        index++;
    }

    ALOGD("HDMI resolution is (%d x %d)", mXres, mYres);
    *numConfigs = mConfigurations.size();
    dumpConfigurations();
    return 0;
}

int ExynosExternalDisplay::getActiveConfig()
{
    if (!mHwc->hdmi_hpd)
        return -1;

    return mActiveConfigIndex;
}

void ExynosExternalDisplay::setHdmiResolution(int resolution, int s3dMode)
{
    if (resolution == 0)
        resolution = mHwc->mHdmiCurrentPreset;
    if (s3dMode == S3D_NONE) {
        if (mHwc->mHdmiCurrentPreset == resolution)
            return;
        mHwc->mHdmiPreset = resolution;
        mHwc->mHdmiResolutionChanged = true;
        mHwc->procs->invalidate(mHwc->procs);
        return;
    }

    switch (resolution) {
    case HDMI_720P_60:
        resolution = S3D_720P_60_BASE + s3dMode;
        break;
    case HDMI_720P_59_94:
        resolution = S3D_720P_59_94_BASE + s3dMode;
        break;
    case HDMI_720P_50:
        resolution = S3D_720P_50_BASE + s3dMode;
        break;
    case HDMI_1080P_24:
        resolution = S3D_1080P_24_BASE + s3dMode;
        break;
    case HDMI_1080P_23_98:
        resolution = S3D_1080P_23_98_BASE + s3dMode;
        break;
    case HDMI_1080P_30:
        resolution = S3D_1080P_30_BASE + s3dMode;
        break;
    case HDMI_1080I_60:
        if (s3dMode != S3D_SBS)
            return;
        resolution = V4L2_DV_1080I60_SB_HALF;
        break;
    case HDMI_1080I_59_94:
        if (s3dMode != S3D_SBS)
            return;
        resolution = V4L2_DV_1080I59_94_SB_HALF;
        break;
    case HDMI_1080P_60:
        if (s3dMode != S3D_SBS && s3dMode != S3D_TB)
            return;
        resolution = S3D_1080P_60_BASE + s3dMode;
        break;
    default:
        return;
    }
    mHwc->mHdmiPreset = resolution;
    mHwc->mHdmiResolutionChanged = true;
    mHwc->mS3DMode = S3D_MODE_READY;
    mHwc->procs->invalidate(mHwc->procs);
}

int ExynosExternalDisplay::setActiveConfig(int index)
{
    if (!mHwc->hdmi_hpd)
        return -1;
    /* Find Preset with index*/
    int preset = -1;
    unsigned int s3dMode = S3D_NONE;
    preset = (int)preset_index_mappings[mConfigurations[index]].preset;

    if (preset < 0) {
        ALOGE("%s:: Unsupported preset, index(%d)", __func__, index);
        return -1;
    }

    v4l2_dv_timings dv_timing = dv_timings[preset_index_mappings[mConfigurations[index]].dv_timings_index];
    if (dv_timing.type == V4L2_DV_BT_SB_HALF)
        s3dMode = S3D_SBS;
    else if (dv_timing.type == V4L2_DV_BT_TB)
        s3dMode = S3D_TB;
    else
        s3dMode = S3D_NONE;

    setHdmiResolution(preset, s3dMode);
    mActiveConfigIndex = index;
    return 0;
}

int32_t ExynosExternalDisplay::getDisplayAttributes(const uint32_t attribute, uint32_t config)
{
    if (config >= SUPPORTED_DV_TIMINGS_NUM) {
        ALOGE("%s:: Invalid config(%d), mConfigurations(%d)", __func__, config, mConfigurations.size());
        return -EINVAL;
    }

    v4l2_dv_timings dv_timing = dv_timings[preset_index_mappings[config].dv_timings_index];
    switch(attribute) {
    case HWC_DISPLAY_VSYNC_PERIOD:
        {
            float refreshRate = (float)((float)dv_timing.bt.pixelclock /
                    ((dv_timing.bt.width + dv_timing.bt.hfrontporch + dv_timing.bt.hsync + dv_timing.bt.hbackporch) *
                     (dv_timing.bt.height + dv_timing.bt.vfrontporch + dv_timing.bt.vsync + dv_timing.bt.vbackporch)));
            return (1000000000/refreshRate);
        }
    case HWC_DISPLAY_WIDTH:
        return dv_timing.bt.width;

    case HWC_DISPLAY_HEIGHT:
        return dv_timing.bt.height;

    case HWC_DISPLAY_DPI_X:
        return this->mXdpi;

    case HWC_DISPLAY_DPI_Y:
        return this->mYdpi;

    default:
        ALOGE("unknown display attribute %u", attribute);
        return -EINVAL;
    }
}

void ExynosExternalDisplay::cleanConfigurations()
{
    mConfigurations.clear();
}

void ExynosExternalDisplay::dumpConfigurations()
{
    ALOGI("External display configurations:: total(%d), active configuration(%d)",
            mConfigurations.size(), mActiveConfigIndex);
    for (size_t i = 0; i <  mConfigurations.size(); i++ ) {
        unsigned int dv_timings_index = preset_index_mappings[mConfigurations[i]].dv_timings_index;
        v4l2_dv_timings configuration = dv_timings[dv_timings_index];
        float refresh_rate = (float)((float)configuration.bt.pixelclock /
                ((configuration.bt.width + configuration.bt.hfrontporch + configuration.bt.hsync + configuration.bt.hbackporch) *
                 (configuration.bt.height + configuration.bt.vfrontporch + configuration.bt.vsync + configuration.bt.vbackporch)));
        uint32_t vsyncPeriod = 1000000000 / refresh_rate;
        ALOGI("%d : type(%d), %d x %d, fps(%f), vsyncPeriod(%d)", i, configuration.type, configuration.bt.width,
                configuration.bt.height,
                refresh_rate, vsyncPeriod);
    }
}

int ExynosExternalDisplay::enable()
{
    if (mEnabled)
        return 0;

    if (mBlanked)
        return 0;

    char value[PROPERTY_VALUE_MAX];
    property_get("persist.hdmi.hdcp_enabled", value, "1");
    int hdcp_enabled = atoi(value);
    ALOGD("%s:: hdcp_enabled (%d)", __func__, hdcp_enabled);

    exynos_hdmi_data hdmi_data;
    hdmi_data.state = hdmi_data.EXYNOS_HDMI_STATE_HDCP;
    hdmi_data.hdcp = hdcp_enabled;

    if ((mDisplayFd < 0) && (openHdmi() < 0))
        return -1;

    if (ioctl(this->mDisplayFd, EXYNOS_SET_HDMI_CONFIG, &hdmi_data) < 0) {
        ALOGE("%s: failed to set HDCP status %d", __func__, errno);
    }

    /* "2" is RGB601_16_235 */
    property_get("persist.hdmi.color_range", value, "2");
    int color_range = atoi(value);

#if 0 // This should be changed
    if (exynos_v4l2_s_ctrl(mMixerLayers[mUiIndex].fd, V4L2_CID_TV_SET_COLOR_RANGE,
                           color_range) < 0)
        ALOGE("%s: s_ctrl(CID_TV_COLOR_RANGE) failed %d", __func__, errno);
#endif

    int err = ioctl(mDisplayFd, FBIOBLANK, FB_BLANK_UNBLANK);
    if (err < 0) {
        if (errno == EBUSY)
            ALOGI("unblank ioctl failed (display already unblanked)");
        else
            ALOGE("unblank ioctl failed: %s", strerror(errno));
        return -errno;
    }

    mEnabled = true;
    return 0;
}

void ExynosExternalDisplay::disable()
{
    if (!mEnabled)
        return;

    blank();

    mEnabled = false;
    checkIONBufferPrepared();
}

void ExynosExternalDisplay::setPreset(int preset)
{
    mHwc->mHdmiResolutionChanged = false;
    mHwc->mHdmiResolutionHandled = false;
    mHwc->hdmi_hpd = false;
    int dv_timings_index = getDVTimingsIndex(preset);
    if (dv_timings_index < 0) {
        ALOGE("invalid preset(%d)", preset);
        return;
    }

    disable();

    exynos_hdmi_data hdmi_data;
    hdmi_data.state = hdmi_data.EXYNOS_HDMI_STATE_PRESET;
    hdmi_data.timings = dv_timings[dv_timings_index];
    if (ioctl(this->mDisplayFd, EXYNOS_SET_HDMI_CONFIG, &hdmi_data) != -1) {
        if (mHwc->procs)
            mHwc->procs->hotplug(mHwc->procs, HWC_DISPLAY_EXTERNAL, false);
    }
}

int ExynosExternalDisplay::convert3DTo2D(int preset)
{
    switch (preset) {
    case V4L2_DV_720P60_FP:
    case V4L2_DV_720P60_SB_HALF:
    case V4L2_DV_720P60_TB:
        return V4L2_DV_720P60;
    case V4L2_DV_720P50_FP:
    case V4L2_DV_720P50_SB_HALF:
    case V4L2_DV_720P50_TB:
        return V4L2_DV_720P50;
    case V4L2_DV_1080P60_SB_HALF:
    case V4L2_DV_1080P60_TB:
        return V4L2_DV_1080P60;
    case V4L2_DV_1080P30_FP:
    case V4L2_DV_1080P30_SB_HALF:
    case V4L2_DV_1080P30_TB:
        return V4L2_DV_1080P30;
    default:
        return HDMI_PRESET_ERROR;
    }
}

void ExynosExternalDisplay::setHdcpStatus(int status)
{
    exynos_hdmi_data hdmi_data;
    hdmi_data.state = hdmi_data.EXYNOS_HDMI_STATE_HDCP;
    hdmi_data.hdcp = !!status;
    if (ioctl(this->mDisplayFd, EXYNOS_SET_HDMI_CONFIG, &hdmi_data) < 0) {
        ALOGE("%s: failed to set HDCP status %d", __func__, errno);
    }
}

void ExynosExternalDisplay::setAudioChannel(uint32_t channels)
{
    exynos_hdmi_data hdmi_data;
    hdmi_data.state = hdmi_data.EXYNOS_HDMI_STATE_AUDIO;
    hdmi_data.audio_info = channels;
    if (ioctl(this->mDisplayFd, EXYNOS_SET_HDMI_CONFIG, &hdmi_data) < 0) {
        ALOGE("%s: failed to set audio channels %d", __func__, errno);
    }
}

uint32_t ExynosExternalDisplay::getAudioChannel()
{
    int channels = 0;

    exynos_hdmi_data hdmi_data;
    hdmi_data.state = hdmi_data.EXYNOS_HDMI_STATE_AUDIO;
    if (ioctl(this->mDisplayFd, EXYNOS_GET_HDMI_CONFIG, &hdmi_data) < 0) {
        ALOGE("%s: failed to get audio channels %d", __func__, errno);
    }
    channels = hdmi_data.audio_info;

    return channels;
}

int ExynosExternalDisplay::getCecPaddr()
{
    if (!mHwc->hdmi_hpd)
        return -1;

    exynos_hdmi_data hdmi_data;

    hdmi_data.state = hdmi_data.EXYNOS_HDMI_STATE_CEC_ADDR;
    if (ioctl(this->mDisplayFd, EXYNOS_GET_HDMI_CONFIG, &hdmi_data) < 0) {
        ALOGE("%s: g_dv_timings error, %d", __func__, errno);
        return -1;
    }

    return (int)hdmi_data.cec_addr;
}

int ExynosExternalDisplay::blank()
{
    int fence = clearDisplay();
    if (fence >= 0)
        close(fence);
    int err = ioctl(mDisplayFd, FBIOBLANK, FB_BLANK_POWERDOWN);
    if (err < 0) {
        if (errno == EBUSY)
            ALOGI("blank ioctl failed (display already blanked)");
        else
            ALOGE("blank ioctl failed: %s", strerror(errno));
        return -errno;
    }

    return 0;
}

int ExynosExternalDisplay::clearDisplay()
{
    if (!mEnabled)
        return 0;
    return ExynosDisplay::clearDisplay();
}

void ExynosExternalDisplay::requestIONMemory()
{
    if (mReserveMemFd > 0) {
        unsigned int value;
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        int err = lseek(mReserveMemFd, 0, SEEK_SET);
        err = read(mReserveMemFd, buffer, sizeof(buffer));
        value = atoi(buffer);

        if ((err > 0) && (value == 0)) {
            memset(buffer, 0, sizeof(buffer));
            buffer[0] = '2';
            if (write(mReserveMemFd, buffer, sizeof(buffer)) < 0)
                ALOGE("fail to request isolation of memmory for HDMI");
            else
                ALOGV("isolation of memmory for HDMI was requested");
        } else {
            if (err < 0)
                ALOGE("fail to read hdmi_reserve_mem_fd");
            else
                ALOGE("ion memmory for HDMI is isolated already");
        }
    }
}
void ExynosExternalDisplay::freeIONMemory()
{
    if ((mHwc->hdmi_hpd == false) && (mReserveMemFd > 0)) {
        unsigned int value;
        char buffer[4096];
        int ret = 0;
        memset(buffer, 0, sizeof(buffer));
        int err = lseek(mReserveMemFd, 0, SEEK_SET);
        err = read(mReserveMemFd, buffer, sizeof(buffer));
        value = atoi(buffer);
        if ((err > 0) && (value == 1)) {
            memset(buffer, 0, sizeof(buffer));
            buffer[0] = '0';
            if (write(mReserveMemFd, buffer, sizeof(buffer)) < 0)
                ALOGE("fail to request isolation of memmory for HDMI");
            else
                ALOGV("deisolation of memmory for HDMI was requested");
        } else {
            if (err < 0)
                ALOGE("fail to read hdmi_reserve_mem_fd");
            else
                ALOGE("ion memmory for HDMI is deisolated already");
        }
        mFlagIONBufferAllocated = false;
    }
}
bool ExynosExternalDisplay::checkIONBufferPrepared()
{
    if (mFlagIONBufferAllocated)
        return true;

    if ((mReserveMemFd > 0)) {
        unsigned int value;
        char buffer[4096];
        int ret = 0;
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
        return false;
    } else {
        /* isolation of video_ext is not used */
        mFlagIONBufferAllocated = true;
        return true;
    }
}

int ExynosExternalDisplay::configureDRMSkipHandle(decon_win_config &cfg)
{
    int err = 0;
    private_handle_t *dst_handle = NULL;

    if (mDRMTempBuffer == NULL) {
        int dst_stride;
        int usage = GRALLOC_USAGE_SW_READ_NEVER |
            GRALLOC_USAGE_SW_WRITE_NEVER |
            GRALLOC_USAGE_HW_COMPOSER;

        err = mAllocDevice->alloc(mAllocDevice, 32, 32, HAL_PIXEL_FORMAT_BGRA_8888,
                usage, &mDRMTempBuffer, &dst_stride);
        if (err < 0) {
            ALOGE("failed to allocate destination buffer(%dx%d): %s", 32, 32,
                    strerror(-err));
            return err;
        } else {
            ALOGV("temBuffer for DRM video was allocated");
        }
    }

    dst_handle = private_handle_t::dynamicCast(mDRMTempBuffer);
    cfg.state = cfg.DECON_WIN_STATE_BUFFER;
    cfg.fd_idma[0] = dst_handle->fd;
    cfg.fd_idma[1] = dst_handle->fd1;
    cfg.fd_idma[2] = dst_handle->fd2;
    cfg.dst.f_w = dst_handle->stride;
    cfg.dst.f_h = dst_handle->vstride;
    cfg.dst.x = 0;
    cfg.dst.y = 0;
    cfg.dst.w = cfg.dst.f_w;
    cfg.dst.h = cfg.dst.f_h;
    cfg.format = halFormatToS3CFormat(HAL_PIXEL_FORMAT_RGBX_8888);

    cfg.src.f_w = dst_handle->stride;
    cfg.src.f_h = dst_handle->vstride;
    cfg.src.x = 0;
    cfg.src.y = 0;
    cfg.src.w = cfg.dst.f_w;
    cfg.src.h = cfg.dst.f_h;
    cfg.blending = DECON_BLENDING_NONE;
    cfg.fence_fd = -1;
    cfg.plane_alpha = 255;

    return 0;
}

void ExynosExternalDisplay::freeExtVideoBuffers()
{
    if (mFlagIONBufferAllocated)
        freeIONMemory();
}

void ExynosExternalDisplay::doPreProcessing(hwc_display_contents_1_t* contents)
{
    mInternalDMAs.clear();
    mInternalDMAs.add(IDMA_G3);
    ExynosDisplay::doPreProcessing(contents);
}
