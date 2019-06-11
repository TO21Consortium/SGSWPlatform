#include "ExynosHWC.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"
#include "ExynosExternalDisplay.h"
#ifdef USES_VIRTUAL_DISPLAY_DECON_EXT_WB
#include "ExynosVirtualDisplayModule.h"
#endif
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
    :   ExynosDisplay(1),
        mEnabled(false),
        mBlanked(false),
        mVirtualOverlayFlag(0),
        mRetry(false),
        mForceOverlayLayerIndex(-1),
        mYuvLayers(0),
        mFbNeeded(false),
        mFirstFb(0),
        mLastFb(0),
        mGscUsed(false),
        mCurrentGscIndex(0),
        mBypassSkipStaticLayer(false),
        mLastOverlayWindowIndex(-1),
        mLastOverlayLayerIndex(-1),
        mGscLayers(0),
        mLastFbWindow(NO_FB_NEEDED),
        mUseSubtitles(false)
{
    this->mHwc = pdev;
    mMPPs[0] = new ExynosMPPModule(this, HDMI_GSC_IDX);
    mXres = 0;
    mYres = 0;
    mXdpi = 0;
    mYdpi = 0;
    mVsyncPeriod = 0;
    mNumMPPs = 1;
    mOtfMode = OTF_OFF;
    mUseProtectedLayer = false;
#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
    mUseScreenshootLayer = false;
    mLocalExternalDisplayPause = false;
#endif

    memset(mLastLayerHandles, 0, sizeof(mLastLayerHandles));
    memset(&mPostData, 0, sizeof(mPostData));
    memset(&mLastConfigData, 0, sizeof(mLastConfigData));
    memset(mLastGscMap, 0, sizeof(mLastGscMap));
    memset(mLastHandles, 0, sizeof(mLastHandles));

    for (size_t i = 0; i < MAX_NUM_HDMI_DMA_CH; i++) {
        mDmaChannelMaxBandwidth[i] = HDMI_DMA_CH_BW_SET[i];
        mDmaChannelMaxOverlapCount[i] = HDMI_DMA_CH_OVERLAP_CNT_SET[i];
    }
}

ExynosExternalDisplay::~ExynosExternalDisplay()
{
    delete mMPPs[0];
}

bool ExynosExternalDisplay::isOverlaySupported(hwc_layer_1_t &layer, size_t i)
{
    int ret = 0;

    if (layer.flags & HWC_SKIP_LAYER) {
        mLayerInfos[i]->mCheckOverlayFlag |= eSkipLayer;
        ALOGV("\tlayer %u: skipping", i);
        return false;
    }

    if (!layer.planeAlpha) {
        mLayerInfos[i]->mCheckOverlayFlag |= eUnsupportedPlaneAlpha;
        return false;
    }

    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

    if (!handle) {
        ALOGV("\tlayer %u: handle is NULL", i);
        mLayerInfos[i]->mCheckOverlayFlag |= eInvalidHandle;
        return false;
    }

    if (visibleWidth(mMPPs[0], layer, handle->format, this->mXres) < BURSTLEN_BYTES) {
        ALOGV("\tlayer %u: visible area is too narrow", i);
        mLayerInfos[i]->mCheckOverlayFlag |= eUnsupportedDstWidth;
        return false;
    }

    if (mMPPs[0]->isProcessingRequired(layer, handle->format)) {
        /* Check whether GSC can handle using local or M2M, local is not used */
        ret = mMPPs[0]->isProcessingSupported(layer, handle->format, false);
        if (ret < 0) {
            ALOGV("\tlayer %u: MPP M2M required but not supported", i);
            mLayerInfos[i]->mCheckOverlayFlag |= eMPPUnsupported;
            mLayerInfos[i]->mCheckMPPFlag |= -ret;
            return false;
        }
    } else {
        if (!isFormatSupported(handle->format)) {
            mLayerInfos[i]->mCheckOverlayFlag |= eUnsupportedFormat;
            ALOGV("\tlayer %u: pixel format %u not supported", i, handle->format);
            return false;
        }
    }
    if ((layer.blending != HWC_BLENDING_NONE) &&
         mMPPs[0]->isFormatSupportedByGsc(handle->format) &&
         !isFormatRgb(handle->format)) {
        mLayerInfos[i]->mCheckOverlayFlag |= eUnsupportedBlending;
        return false;
    }

    if (!isBlendingSupported(layer.blending)) {
        ALOGV("\tlayer %u: blending %d not supported", i, layer.blending);
        mLayerInfos[i]->mCheckOverlayFlag |= eUnsupportedBlending;
        return false;
    }
    if (CC_UNLIKELY(isOffscreen(layer, mXres, mYres))) {
        ALOGW("\tlayer %u: off-screen", i);
        mLayerInfos[i]->mCheckOverlayFlag |= eUnsupportedCoordinate;
        return false;
    }

    return true;
}

int ExynosExternalDisplay::prepare(hwc_display_contents_1_t* contents)
{
    memset(mPostData.gsc_map, 0, sizeof(mPostData.gsc_map));
    mRetry = false;

    ExynosDisplay::allocateLayerInfos(contents);

    checkGrallocFlags(contents);

    do {
        determineYuvOverlay(contents);
        determineSupportedOverlays(contents);
        determineBandwidthSupport(contents);
        assignWindows(contents);
    } while (mRetry);
#if defined(GSC_VIDEO)
    if ((mHwc->mS3DMode != S3D_MODE_DISABLED) && (this->mYuvLayers == 1) && !mUseSubtitles) {
        // UI layers will be skiped when S3D video is playing
        mPostData.fb_window = NO_FB_NEEDED;
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            layer.compositionType = HWC_OVERLAY;
        }
    }
#endif

    skipStaticLayers(contents);

    if (mVirtualOverlayFlag)
        mFbNeeded = 0;

    if (!mFbNeeded)
        mPostData.fb_window = NO_FB_NEEDED;

    /*
     * Usually, layer cnt must be more than 1 for HWC 1.1.
     * But, Surfaceflinger is passing some special case to HWC with layer cnt is equal to 1.
     * To handle that special case: enable fb window if the layer cnt is 1.
     */
    if (contents->numHwLayers <= 1)
        mPostData.fb_window = 0;

    return 0;
}



void ExynosExternalDisplay::configureHandle(private_handle_t *handle,
        hwc_frect_t &sourceCrop, hwc_rect_t &displayFrame,
        int32_t blending, int32_t planeAlpha, int fence_fd, s3c_fb_win_config &cfg)
{
    uint32_t x, y;
    uint32_t w = WIDTH(displayFrame);
    uint32_t h = HEIGHT(displayFrame);
    uint8_t bpp = formatToBpp(handle->format);
    uint32_t offset = (sourceCrop.top * handle->stride + sourceCrop.left) * bpp / 8;

    if (displayFrame.left < 0) {
        unsigned int crop = -displayFrame.left;
        ALOGV("layer off left side of screen; cropping %u pixels from left edge",
                crop);
        x = 0;
        w -= crop;
        offset += crop * bpp / 8;
    } else {
        x = displayFrame.left;
    }

    if (displayFrame.right > this->mXres) {
        unsigned int crop = displayFrame.right - this->mXres;
        ALOGV("layer off right side of screen; cropping %u pixels from right edge",
                crop);
        w -= crop;
    }

    if (displayFrame.top < 0) {
        unsigned int crop = -displayFrame.top;
        ALOGV("layer off top side of screen; cropping %u pixels from top edge",
                crop);
        y = 0;
        h -= crop;
        offset += handle->stride * crop * bpp / 8;
    } else {
        y = displayFrame.top;
    }

    if (displayFrame.bottom > this->mYres) {
        int crop = displayFrame.bottom - this->mYres;
        ALOGV("layer off bottom side of screen; cropping %u pixels from bottom edge",
                crop);
        h -= crop;
    }
    cfg.state = cfg.S3C_FB_WIN_STATE_BUFFER;
    cfg.fd = handle->fd;
    cfg.x = x;
    cfg.y = y;
    cfg.w = w;
    cfg.h = h;
    cfg.format = halFormatToS3CFormat(handle->format);
    cfg.offset = offset;
    cfg.stride = handle->stride * bpp / 8;
    cfg.blending = halBlendingToS3CBlending(blending);
    cfg.fence_fd = fence_fd;
    cfg.plane_alpha = 255;
    if (planeAlpha && (planeAlpha < 255)) {
        cfg.plane_alpha = planeAlpha;
    }

#ifdef USES_DRM_SETTING_BY_DECON
    if (getDrmMode(handle->flags) != NO_DRM)
        cfg.protection = 1;
    else
        cfg.protection = 0;
#endif
}

void ExynosExternalDisplay::configureOverlay(hwc_layer_1_t *layer, s3c_fb_win_config &cfg)
{
    if (layer->compositionType == HWC_BACKGROUND) {
        hwc_color_t color = layer->backgroundColor;
        cfg.state = cfg.S3C_FB_WIN_STATE_COLOR;
        cfg.color = (color.r << 16) | (color.g << 8) | color.b;
        cfg.x = 0;
        cfg.y = 0;
        cfg.w = this->mXres;
        cfg.h = this->mYres;
        return;
    }

    private_handle_t *handle = private_handle_t::dynamicCast(layer->handle);
    configureHandle(handle, layer->sourceCropf, layer->displayFrame,
            layer->blending, layer->planeAlpha, layer->acquireFenceFd, cfg);


}

int ExynosExternalDisplay::postFrame(hwc_display_contents_1_t* contents)
{
    exynos5_hwc_post_data_t *pdata = &mPostData;
    struct s3c_fb_win_config_data win_data;
    struct s3c_fb_win_config *config = win_data.config;
    int win_map = 0;
    int tot_ovly_wins = 0;

    memset(config, 0, sizeof(win_data.config));
    for (size_t i = 0; i < NUM_HW_WINDOWS; i++)
        config[i].fence_fd = -1;

    for (size_t i = 0; i < NUM_HDMI_WINDOWS; i++) {
        if ( pdata->overlay_map[i] != -1)
            tot_ovly_wins++;
    }
    if (mVirtualOverlayFlag)
        tot_ovly_wins++;

#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
    if (mLocalExternalDisplayPause) {
        for (int i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (layer.acquireFenceFd > 0)
		    close(layer.acquireFenceFd);
	    layer.releaseFenceFd = -1;
        }
        return 0;
    }
#endif

    for (size_t i = 0; i < NUM_HDMI_WINDOWS; i++) {
        int layer_idx = pdata->overlay_map[i];
        if (layer_idx != -1) {
            hwc_layer_1_t &layer = contents->hwLayers[layer_idx];
            private_handle_t *handle =
                private_handle_t::dynamicCast(layer.handle);
            win_map = i + 1;  /* Window 0 is background layer, We can't use */
            if (handle == NULL) {
                ALOGE("compositionType is OVERLAY but handle is NULL");
                continue;
            }
#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
            if (layer.flags & HWC_SKIP_HDMI_RENDERING) {
                layer.releaseFenceFd = layer.acquireFenceFd;
                continue;
            }
#endif
            if (pdata->gsc_map[i].mode == exynos5_gsc_map_t::GSC_M2M) {
                if (mHwc->mS3DMode != S3D_MODE_DISABLED && mHwc->mHdmiResolutionChanged) {
                    if (isPresetSupported(mHwc->mHdmiPreset)) {
                        mHwc->mS3DMode = S3D_MODE_RUNNING;
                        setPreset(mHwc->mHdmiPreset);
                    } else {
                        mHwc->mS3DMode = S3D_MODE_RUNNING;
                        mHwc->mHdmiResolutionChanged = false;
                        mHwc->mHdmiResolutionHandled = true;
                        int S3DFormat = getS3DFormat(mHwc->mHdmiPreset);
                        if (S3DFormat == S3D_SBS)
                            mMPPs[0]->mS3DMode = S3D_SBS;
                        else if (S3DFormat == S3D_TB)
                            mMPPs[0]->mS3DMode = S3D_TB;
                    }
                }
                if (postGscM2M(layer, config, win_map, i) < 0)
                    continue;
            } else {
                configureOverlay(&layer, config[win_map]);
            }
        }
        if (i == 0 && config[i].blending != S3C_FB_BLENDING_NONE) {
            ALOGV("blending not supported on window 0; forcing BLENDING_NONE");
            config[i].blending = S3C_FB_BLENDING_NONE;
        }

        dumpConfig(config[win_map]);
    }

#if defined(GSC_VIDEO)
    if ((mHwc->mS3DMode != S3D_MODE_DISABLED) && (this->mYuvLayers == 1) && !mUseSubtitles)
        skipUILayers(contents);
#endif

    if (this->mVirtualOverlayFlag) {
        handleStaticLayers(contents, win_data, tot_ovly_wins);
    }

    if (checkConfigChanged(win_data, mLastConfigData) == false)
    {
        ExynosMPP &gsc = *mMPPs[0];
        gsc.mCurrentBuf = (gsc.mCurrentBuf + 1) % gsc.mNumAvailableDstBuffers;
        return 0;
    }

    int ret = ioctl(this->mDisplayFd, S3CFB_WIN_CONFIG, &win_data);
    for (size_t i = 0; i < NUM_HDMI_WINDOWS; i++)
        if (config[i].fence_fd != -1)
            close(config[i].fence_fd);
    if (ret < 0) {
        ALOGE("ioctl S3CFB_WIN_CONFIG failed: %s", strerror(errno));
        return ret;
    }

    if (mGscLayers < MAX_HDMI_VIDEO_LAYERS) {
        cleanupGscs();
    }

    memcpy(&(this->mLastConfigData), &win_data, sizeof(win_data));
    memcpy(this->mLastGscMap, pdata->gsc_map, sizeof(pdata->gsc_map));
    this->mLastFbWindow = pdata->fb_window;
    for (size_t i = 0; i < NUM_HDMI_WINDOWS; i++) {
        int layer_idx = pdata->overlay_map[i];
        if (layer_idx != -1) {
            hwc_layer_1_t &layer = contents->hwLayers[layer_idx];
            this->mLastHandles[i] = layer.handle;
        }
    }

    return win_data.fence;
}

int ExynosExternalDisplay::clearDisplay()
{
    if (!mEnabled)
        return 0;

    struct s3c_fb_win_config_data win_data;
    memset(&win_data, 0, sizeof(win_data));

    int ret = ioctl(this->mDisplayFd, S3CFB_WIN_CONFIG, &win_data);
    LOG_ALWAYS_FATAL_IF(ret < 0,
            "ioctl S3CFB_WIN_CONFIG failed to clear screen: %s",
            strerror(errno));
    // the causes of an empty config failing are all unrecoverable

    return win_data.fence;
}

int ExynosExternalDisplay::set(hwc_display_contents_1_t* contents)
{
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

    hwc_layer_1_t *fb_layer = NULL;
    int err = 0;

    if (this->mPostData.fb_window != NO_FB_NEEDED) {
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            if (contents->hwLayers[i].compositionType ==
                    HWC_FRAMEBUFFER_TARGET) {
                this->mPostData.overlay_map[this->mPostData.fb_window] = i;
                fb_layer = &contents->hwLayers[i];
                break;
            }
        }

        if (CC_UNLIKELY(!fb_layer)) {
            ALOGE("framebuffer target expected, but not provided");
            err = -EINVAL;
        } else {
            if (fb_layer != NULL) {
                dumpLayer(fb_layer);
            }
        }
    }

    int fence;
    if (!err) {
        fence = postFrame(contents);
        if (fence < 0)
            err = fence;
    }

    if (err)
        fence = clearDisplay();
    if (fence == 0) {
        //Only happens in mLocalExternalPause scenario, not error
        fence = -1;
    } else {
        for (size_t i = 0; i < NUM_HDMI_WINDOWS; i++) {
            if (this->mPostData.overlay_map[i] != -1) {
                hwc_layer_1_t &layer =
                    contents->hwLayers[this->mPostData.overlay_map[i]];
                int dup_fd = dup(fence);
                if (dup_fd < 0)
                    ALOGW("release fence dup failed: %s", strerror(errno));
                if (this->mPostData.gsc_map[i].mode == exynos5_gsc_map_t::GSC_M2M) {
                    //int gsc_idx = this->mPostData.gsc_map[i].idx;
                    ExynosMPP &gsc = *mMPPs[0];
                    if (gsc.mDstBufFence[gsc.mCurrentBuf] >= 0) {
                        close (gsc.mDstBufFence[gsc.mCurrentBuf]);
                        gsc.mDstBufFence[gsc.mCurrentBuf] = -1;
                    }
                    gsc.mDstBufFence[gsc.mCurrentBuf] = dup_fd;
                    gsc.mCurrentBuf = (gsc.mCurrentBuf + 1) % gsc.mNumAvailableDstBuffers;
                } else {
                    layer.releaseFenceFd = dup_fd;
                }
            }
        }
    }
    contents->retireFenceFd = fence;

    if (this->mYuvLayers == 0 && !mHwc->local_external_display_pause) {
        if (mHwc->mS3DMode == S3D_MODE_RUNNING && contents->numHwLayers > 1) {
            int preset = convert3DTo2D(mHwc->mHdmiCurrentPreset);
            if (isPresetSupported(preset)) {
                setPreset(preset);
                mHwc->mS3DMode = S3D_MODE_STOPPING;
                mHwc->mHdmiPreset = preset;
                if (mHwc->procs)
                    mHwc->procs->invalidate(mHwc->procs);
            } else {
                mHwc->mS3DMode = S3D_MODE_DISABLED;
                mHwc->mHdmiPreset = mHwc->mHdmiCurrentPreset;
            }
        }
    }

    return err;
}

void ExynosExternalDisplay::skipStaticLayers(hwc_display_contents_1_t* contents)
{
    static int init_flag = 0;
    int last_ovly_lay_idx = -1;

    mVirtualOverlayFlag = 0;
    mLastOverlayWindowIndex = -1;

    if (!mHwc->hwc_ctrl.skip_static_layer_mode)
        return;

    if (mBypassSkipStaticLayer)
        return;

    if (contents->flags & HWC_GEOMETRY_CHANGED) {
        init_flag = 0;
        return;
    }

    for (size_t i = 0; i < NUM_HDMI_WINDOWS; i++) {
        if (mPostData.overlay_map[i] != -1) {
            last_ovly_lay_idx = mPostData.overlay_map[i];
            mLastOverlayWindowIndex = i;
        }
    }

    if ((last_ovly_lay_idx == -1) || ((uint32_t)last_ovly_lay_idx >= (contents->numHwLayers - 2)) ||
        ((contents->numHwLayers - last_ovly_lay_idx - 1) >= NUM_VIRT_OVER)) {
        init_flag = 0;
        return;
    }
    mLastOverlayLayerIndex = last_ovly_lay_idx;
    last_ovly_lay_idx++;
    if (init_flag == 1) {
        for (size_t i = last_ovly_lay_idx; i < contents->numHwLayers -1; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (!layer.handle || (layer.flags & HWC_SKIP_LAYER) || (mLastLayerHandles[i - last_ovly_lay_idx] !=  layer.handle)) {
                init_flag = 0;
                return;
            }
        }

        mVirtualOverlayFlag = 1;
        for (size_t i = last_ovly_lay_idx; i < contents->numHwLayers-1; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (layer.compositionType == HWC_FRAMEBUFFER) {
                layer.compositionType = HWC_OVERLAY;
                mLayerInfos[i]->compositionType = layer.compositionType;
                mLayerInfos[i]->mCheckOverlayFlag |= eSkipStaticLayer;
            }
        }
        return;
    }

    init_flag = 1;
    for (size_t i = last_ovly_lay_idx; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        mLastLayerHandles[i - last_ovly_lay_idx] = layer.handle;
    }
    for (size_t i = contents->numHwLayers - last_ovly_lay_idx; i < NUM_VIRT_OVER; i++)
        mLastLayerHandles[i] = 0;

    return;
}

void ExynosExternalDisplay::determineYuvOverlay(hwc_display_contents_1_t *contents)
{
    mForceOverlayLayerIndex = -1;
    mHasDrmSurface = false;
    mYuvLayers = 0;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
            if (getDrmMode(handle->flags) != NO_DRM) {
                this->mHasDrmSurface = true;
                mForceOverlayLayerIndex = i;
                break;
            }

            /* check yuv surface */
            if (((int)get_yuv_planes(halFormatToV4L2Format(handle->format)) > 0) &&
                    (this->mYuvLayers < MAX_HDMI_VIDEO_LAYERS)) {
                this->mYuvLayers++;
                mForceOverlayLayerIndex = i;

            }
        }
    }
}


void ExynosExternalDisplay::determineSupportedOverlays(hwc_display_contents_1_t *contents)
{
    bool videoLayer = false;

    mFbNeeded = false;
    mFirstFb = mLastFb = 0;

    for (size_t i = 0; i < NUM_HDMI_WINDOWS; i++)
        mPostData.overlay_map[i] = -1;

    // find unsupported overlays
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            ALOGV("\tlayer %u: framebuffer target", i);
            mLayerInfos[i]->compositionType = layer.compositionType;
            continue;
        }

        if (layer.compositionType == HWC_BACKGROUND) {
            ALOGV("\tlayer %u: background supported", i);
            dumpLayer(&contents->hwLayers[i]);
            mLayerInfos[i]->compositionType = layer.compositionType;
            continue;
        }
        
#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
        if (mLocalExternalDisplayPause) {
            layer.compositionType = HWC_OVERLAY;
            layer.flags = HWC_SKIP_HDMI_RENDERING;
            continue;
        }
#endif

        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
#if defined(GSC_VIDEO)
            if ((getDrmMode(handle->flags) != NO_DRM)
                    || ((!mHwc->force_mirror_mode) && ((int)get_yuv_planes(halFormatToV4L2Format(handle->format)) > 0) &&
                        (mMPPs[0]->isProcessingSupported(layer, handle->format, false) > 0))) {
#else
            if (getDrmMode(handle->flags) != NO_DRM) {
#endif
                if (((mUseProtectedLayer == true) && (getDrmMode(handle->flags) != NO_DRM)) ||
                        ((mUseProtectedLayer == false) && !videoLayer)) {
                    videoLayer = true;
                    layer.compositionType = HWC_OVERLAY;
                    mLayerInfos[i]->compositionType = layer.compositionType;
                    ALOGV("\tlayer %u: video layer", i);
                    dumpLayer(&layer);
                    continue;
                }
            }
            layer.compositionType = HWC_FRAMEBUFFER;
            dumpLayer(&layer);
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
        mLayerInfos[i]->mCheckOverlayFlag |= eUnSupportedUseCase;

        dumpLayer(&contents->hwLayers[i]);
    }

    mFirstFb = min(mFirstFb, (size_t)NUM_HDMI_WINDOWS-1);
    // can't composite overlays sandwiched between framebuffers
    if (mFbNeeded) {
        for (size_t i = mFirstFb; i < mLastFb; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            private_handle_t *handle = NULL;
            if (layer.handle) {
                handle = private_handle_t::dynamicCast(layer.handle);
                if (getDrmMode(handle->flags) != NO_DRM)
                    continue;
            }
            contents->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
            mLayerInfos[i]->compositionType = contents->hwLayers[i].compositionType;
            mLayerInfos[i]->mCheckOverlayFlag |= eSandwitchedBetweenGLES;
        }
    }
}

void ExynosExternalDisplay::determineBandwidthSupport(hwc_display_contents_1_t *contents)
{
    bool changed;
    this->mBypassSkipStaticLayer = false;
    unsigned int cannotComposeFlag = 0;

    uint32_t pixel_used[MAX_NUM_HDMI_DMA_CH];
    do {
        android::Vector<hwc_rect> rects[MAX_NUM_HDMI_DMA_CH];
        android::Vector<hwc_rect> overlaps[MAX_NUM_HDMI_DMA_CH];
        int dma_ch_idx;
        uint32_t win_idx = 0;
        size_t windows_left;
        memset(&pixel_used[0], 0, sizeof(pixel_used));
        mGscUsed = false;

        if (mFbNeeded) {
            hwc_rect_t fb_rect;
            fb_rect.top = fb_rect.left = 0;
            fb_rect.right = this->mXres - 1;
            fb_rect.bottom = this->mYres - 1;
            dma_ch_idx = HDMI_DMA_CH_IDX[mFirstFb];
            pixel_used[dma_ch_idx] = (uint32_t) (this->mXres * this->mYres);
            win_idx = (win_idx == mFirstFb) ? (win_idx + 1) : win_idx;
            windows_left = NUM_HDMI_WINDOWS - 1;
            rects[dma_ch_idx].push_back(fb_rect);
        }
        else {
            windows_left = NUM_HDMI_WINDOWS;
        }
        changed = false;
        mGscLayers = 0;
        mCurrentGscIndex = 0;
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if ((layer.flags & HWC_SKIP_LAYER) ||
                    layer.compositionType == HWC_FRAMEBUFFER_TARGET)
                continue;

            private_handle_t *handle = private_handle_t::dynamicCast(
                    layer.handle);

            // we've already accounted for the framebuffer above
            if (layer.compositionType == HWC_FRAMEBUFFER)
                continue;

            // only layer 0 can be HWC_BACKGROUND, so we can
            // unconditionally allow it without extra checks
            if (layer.compositionType == HWC_BACKGROUND) {
                windows_left--;
                continue;
            }

#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
            if (layer.flags & HWC_SKIP_HDMI_RENDERING)
                continue;
#endif

            dma_ch_idx = HDMI_DMA_CH_IDX[win_idx];

            size_t pixels_needed = 0;
            if (getDrmMode(handle->flags) != SECURE_DRM)
                pixels_needed = getRequiredPixels(layer, mXres, mYres);
            else
                pixels_needed = WIDTH(layer.displayFrame) *
                    HEIGHT(layer.displayFrame);

            cannotComposeFlag = 0;
            bool can_compose = windows_left && (win_idx < NUM_HDMI_WINDOWS) &&
                            ((pixel_used[dma_ch_idx] + pixels_needed) <=
                            (uint32_t)this->mDmaChannelMaxBandwidth[dma_ch_idx]);

            if (windows_left <= 0 || (win_idx >= NUM_HDMI_WINDOWS))
                cannotComposeFlag |= eInsufficientWindow;
            if ((pixel_used[dma_ch_idx] + pixels_needed) > (uint32_t)this->mDmaChannelMaxBandwidth[dma_ch_idx])
                cannotComposeFlag |= eInsufficientBandwidth;

            bool gsc_required = mMPPs[0]->isProcessingRequired(layer, handle->format);
            if (gsc_required) {
                if (mGscLayers >= MAX_HDMI_VIDEO_LAYERS) {
                    can_compose = can_compose && !mGscUsed;
                    if (mGscUsed)
                        cannotComposeFlag |= eInsufficientMPP;
                }
#if 0
                if (mHwc->hwc_ctrl.num_of_video_ovly <= mGscLayers)
                    can_compose = false;
#endif
            }
            // hwc_rect_t right and bottom values are normally exclusive;
            // the intersection logic is simpler if we make them inclusive
            hwc_rect_t visible_rect = layer.displayFrame;
            visible_rect.right--; visible_rect.bottom--;

            if (can_compose) {
                switch (this->mDmaChannelMaxOverlapCount[dma_ch_idx]) {
                case 1: // It means, no layer overlap is allowed
                    for (size_t j = 0; j < rects[dma_ch_idx].size(); j++) {
                         if (intersect(visible_rect, rects[dma_ch_idx].itemAt(j))) {
                            can_compose = false;
                            cannotComposeFlag |= eInsufficientOverlapCount;
                         }
                    }
                    break;
                case 2: //It means, upto 2 layer overlap is allowed.
                    for (size_t j = 0; j < overlaps[dma_ch_idx].size(); j++) {
                        if (intersect(visible_rect, overlaps[dma_ch_idx].itemAt(j))) {
                            can_compose = false;
                            cannotComposeFlag |= eInsufficientOverlapCount;
                        }
                    }
                    break;
                default:
                    break;
                }
                if (!can_compose)
                    this->mBypassSkipStaticLayer = true;
            }

            if (!can_compose) {
                layer.compositionType = HWC_FRAMEBUFFER;
                mLayerInfos[i]->compositionType = layer.compositionType;
                mLayerInfos[i]->mCheckOverlayFlag |= cannotComposeFlag;
                if (!mFbNeeded) {
                    mFirstFb = mLastFb = i;
                    mFbNeeded = true;
                }
                else {
                    mFirstFb = min(i, mFirstFb);
                    mLastFb = max(i, mLastFb);
                }
                changed = true;
                mFirstFb = min(mFirstFb, (size_t)NUM_HDMI_WINDOWS-1);
                break;
            }

            for (size_t j = 0; j < rects[dma_ch_idx].size(); j++) {
                const hwc_rect_t &other_rect = rects[dma_ch_idx].itemAt(j);
                if (intersect(visible_rect, other_rect))
                    overlaps[dma_ch_idx].push_back(intersection(visible_rect, other_rect));
            }

            rects[dma_ch_idx].push_back(visible_rect);
            pixel_used[dma_ch_idx] += pixels_needed;
            win_idx++;
            win_idx = (win_idx == mFirstFb) ? (win_idx + 1) : win_idx;
            win_idx = min(win_idx, (uint32_t)(NUM_HDMI_WINDOWS - 1));
            windows_left--;
            if (gsc_required) {
                mGscUsed = true;
                mGscLayers++;
            }
        }

        if (changed)
            for (size_t i = mFirstFb; i < mLastFb; i++) {
                hwc_layer_1_t &layer = contents->hwLayers[i];
                private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
                if ((handle != NULL) && (getDrmMode(handle->flags) != NO_DRM))
                    continue;
                contents->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
                mLayerInfos[i]->compositionType = contents->hwLayers[i].compositionType;
                mLayerInfos[i]->mCheckOverlayFlag |= eSandwitchedBetweenGLES;
            }
    } while(changed);

}

void ExynosExternalDisplay::assignWindows(hwc_display_contents_1_t *contents)
{
    unsigned int nextWindow = 0;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (mFbNeeded && i == mFirstFb) {
            mPostData.fb_window = nextWindow;
            nextWindow++;
            continue;
        }

#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
        if (layer.flags & HWC_SKIP_HDMI_RENDERING)
            continue;
#endif

        if (layer.compositionType != HWC_FRAMEBUFFER &&
            layer.compositionType != HWC_FRAMEBUFFER_TARGET) {
            this->mPostData.overlay_map[nextWindow] = i;
            if (layer.compositionType == HWC_OVERLAY) {
                private_handle_t *handle =
                        private_handle_t::dynamicCast(layer.handle);
                if (mMPPs[0]->isProcessingRequired(layer, handle->format)) {
                    if (assignGscLayer(layer, i, nextWindow))
                        mCurrentGscIndex++;
                }
            }
            nextWindow++;
        }
    }
}

bool ExynosExternalDisplay::assignGscLayer(hwc_layer_1_t __unused &layer, int __unused index, int nextWindow)
{
    mPostData.gsc_map[nextWindow].mode = exynos5_gsc_map_t::GSC_M2M;
    mMPPs[0]->setMode(exynos5_gsc_map_t::GSC_M2M);
    mPostData.gsc_map[nextWindow].idx = HDMI_GSC_IDX;
    return true;
}

int ExynosExternalDisplay::postGscM2M(hwc_layer_1_t &layer, struct s3c_fb_win_config *config, int win_map, int index)
{
    exynos5_hwc_post_data_t *pdata = &mPostData;
    //int gsc_idx = pdata->gsc_map[index].idx;
    int gsc_idx = 0;
    int dst_format = HAL_PIXEL_FORMAT_RGBX_8888;
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

    hwc_frect_t sourceCrop = { 0, 0,
            (float)WIDTH(layer.displayFrame), (float)HEIGHT(layer.displayFrame) };

    /* OFF_Screen to ON_Screen changes */
    if (getDrmMode(handle->flags) == SECURE_DRM)
        recalculateDisplayFrame(layer, mXres, mYres);

    int err = mMPPs[0]->processM2M(layer, dst_format, &sourceCrop);
    if (err < 0) {
        ALOGE("failed to configure gscaler %u for layer %u",
                gsc_idx, index);
        pdata->gsc_map[index].mode = exynos5_gsc_map_t::GSC_NONE;
        return -1;
    }

    buffer_handle_t dst_buf = mMPPs[0]->mDstBuffers[mMPPs[0]->mCurrentBuf];
    private_handle_t *dst_handle =
            private_handle_t::dynamicCast(dst_buf);
    int fence = mMPPs[0]->mDstConfig.releaseFenceFd;
    configureHandle(dst_handle, sourceCrop,
            layer.displayFrame, layer.blending, layer.planeAlpha, fence,
            config[win_map]);

#ifdef USES_DRM_SETTING_BY_DECON
    if (getDrmMode(handle->flags) != NO_DRM)
        config[win_map].protection = 1;
    else
        config[win_map].protection = 0;
#endif
    return 0;
}

void ExynosExternalDisplay::handleStaticLayers(hwc_display_contents_1_t *contents, struct s3c_fb_win_config_data &win_data, int __unused tot_ovly_wins)
{
    ALOGV("[USE] SKIP_STATIC_LAYER_COMP\n");
    int last_ovly_win_map = mLastOverlayWindowIndex + 2;
    memcpy(&win_data.config[last_ovly_win_map],
        &mLastConfigData.config[last_ovly_win_map], sizeof(struct s3c_fb_win_config));
    win_data.config[last_ovly_win_map].fence_fd = -1;
    for (size_t i = mLastOverlayLayerIndex + 1; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.compositionType == HWC_OVERLAY) {
            ALOGV("[SKIP_STATIC_LAYER_COMP] layer.handle: 0x%p, layer.acquireFenceFd: %d\n", layer.handle, layer.acquireFenceFd);
            layer.releaseFenceFd = layer.acquireFenceFd;
        }
    }
}

void ExynosExternalDisplay::skipUILayers(hwc_display_contents_1_t *contents)
{
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.flags & HWC_SKIP_LAYER) {
            ALOGV("HDMI skipping layer %d", i);
            continue;
        }

        if (layer.compositionType == HWC_OVERLAY) {
            if (!layer.handle)
                continue;

                private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
                if ((int)get_yuv_planes(halFormatToV4L2Format(handle->format)) < 0) {
                    layer.releaseFenceFd = layer.acquireFenceFd;
                    continue;
                }
        }
    }
}

void ExynosExternalDisplay::cleanupGscs()
{
    mMPPs[0]->cleanupM2M();
    mMPPs[0]->setMode(exynos5_gsc_map_t::GSC_NONE);
}

int ExynosExternalDisplay::openHdmi()
{
    int ret = 1;
    int sw_fd;

    mHwc->externalDisplay->mDisplayFd = open("/dev/graphics/fb1", O_RDWR);
    if (mHwc->externalDisplay->mDisplayFd < 0) {
        ALOGE("failed to open framebuffer for externalDisplay");
        ret = mHwc->externalDisplay->mDisplayFd;
        return ret;
    }

    ALOGD("Open fd for HDMI");

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
#ifdef USES_VIRTUAL_DISPLAY_DECON_EXT_WB
    ExynosVirtualDisplayModule *virDisplay = (ExynosVirtualDisplayModule *)mHwc->virtualDisplay;
    if (virDisplay->mDisplayFd < 0) {
#endif
    if (status) {
        char value[PROPERTY_VALUE_MAX];
        property_get("wlan.wfd.status", value, "disconnected");
        bool bWFDDisconnected = !strcmp(value, "disconnected");

        if (bWFDDisconnected) {
            enable();
        }
    } else {
        disable();
        closeHdmi();
    }
#ifdef USES_VIRTUAL_DISPLAY_DECON_EXT_WB
    }
#endif
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

int ExynosExternalDisplay::getDisplayConfigs(uint32_t *configs, size_t *numConfigs)
{
    *numConfigs = 1;
    configs[0] = 0;
    getConfig();
    return 0;
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

    for (int i = 0; i < SUPPORTED_DV_TIMINGS_NUM; i++) {
        dv_timings_index = preset_index_mappings[i].dv_timings_index;
        if (is_same_dv_timings(&hdmi_data.timings, &dv_timings[dv_timings_index])) {
            mXres = hdmi_data.timings.bt.width;
            mYres = hdmi_data.timings.bt.height;
            mHwc->mHdmiCurrentPreset = preset_index_mappings[i].preset;
            break;
        }
    }
    ALOGD("HDMI resolution is (%d x %d)", mXres, mYres);

    return 0;
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

    exynos_hdmi_data hdmi_data;
    hdmi_data.state = hdmi_data.EXYNOS_HDMI_STATE_HDCP;
    hdmi_data.hdcp = hdcp_enabled;
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

    mMPPs[0]->cleanupM2M();
    mEnabled = false;
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

void ExynosExternalDisplay::checkGrallocFlags(hwc_display_contents_1_t *contents)
{
    mUseProtectedLayer = false;
#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
    mUseScreenshootLayer = false;
    /* it can get from HWCService */
    mLocalExternalDisplayPause = mHwc->external_display_pause;
#endif

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.handle) {
            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
            if (h->flags & GRALLOC_USAGE_PROTECTED)
                mUseProtectedLayer = false;
        }
#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
        if (layer.flags & HWC_SCREENSHOT_ANIMATOR_LAYER)
            mUseScreenshootLayer = true;
#endif
    }

#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
    if (mUseScreenshootLayer)
        mLocalExternalDisplayPause = true;
    else
        mLocalExternalDisplayPause = false;
#endif
}

bool ExynosExternalDisplay::checkConfigChanged(struct s3c_fb_win_config_data lastConfigData, struct s3c_fb_win_config_data newConfigData)
{
    for (size_t i = 0; i < NUM_HDMI_WINDOWS; i++) {
        if ((lastConfigData.config[i].state != newConfigData.config[i].state) ||
                (lastConfigData.config[i].fd != newConfigData.config[i].fd) ||
                (lastConfigData.config[i].x != newConfigData.config[i].x) ||
                (lastConfigData.config[i].y != newConfigData.config[i].y) ||
                (lastConfigData.config[i].w != newConfigData.config[i].w) ||
                (lastConfigData.config[i].h != newConfigData.config[i].h) ||
                (lastConfigData.config[i].format != newConfigData.config[i].format) ||
                (lastConfigData.config[i].offset != newConfigData.config[i].offset) ||
                (lastConfigData.config[i].blending != newConfigData.config[i].blending) ||
                (lastConfigData.config[i].plane_alpha != newConfigData.config[i].plane_alpha))
            return true;
    }
    return false;
}
