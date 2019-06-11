#define ATRACE_TAG ATRACE_TAG_GRAPHICS
#include "ExynosOverlayDisplay.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"
#include <utils/Trace.h>

#ifdef G2D_COMPOSITION
#include "ExynosG2DWrapper.h"
#endif

ExynosOverlayDisplay::ExynosOverlayDisplay(int numMPPs, struct exynos5_hwc_composer_device_1_t *pdev)
    :   ExynosDisplay(numMPPs),
        mLastFbWindow(NO_FB_NEEDED),
        mGrallocModule(NULL),
        mLastOverlayWindowIndex(-1),
        mLastOverlayLayerIndex(-1),
        mVirtualOverlayFlag(0),
        mForceFbYuvLayer(0),
        mCountSameConfig(0),
        mConfigMode(0),
        mGscLayers(0),
        mPopupPlayYuvContents(false),
        mHasCropSurface(false),
        mYuvLayers(0),
        mBypassSkipStaticLayer(false),
        mGscUsed(false),
        mCurrentGscIndex(0),
        mFbNeeded(false),
        mFirstFb(0),
        mLastFb(0),
        mForceFb(false),
        mForceOverlayLayerIndex(-1),
        mRetry(false),
        mBlanked(true),
        mMaxWindowOverlapCnt(NUM_HW_WINDOWS),
        mAllowedOverlays(5)
{
    mMPPs = new ExynosMPPModule*[mNumMPPs];
    for (int i = 0; i < mNumMPPs; i++)
        mMPPs[i] = new ExynosMPPModule(this, i);

    for (size_t i = 0; i < MAX_NUM_FIMD_DMA_CH; i++) {
        mDmaChannelMaxBandwidth[i] = 2560 * 1600;
        mDmaChannelMaxOverlapCount[i] = 1;
    }

    memset(&mPostData, 0, sizeof(mPostData));
    memset(&mLastConfigData, 0, sizeof(mLastConfigData));
    memset(mLastGscMap, 0, sizeof(mLastGscMap));
    memset(mLastHandles, 0, sizeof(mLastHandles));
    memset(mLastLayerHandles, 0, sizeof(mLastLayerHandles));
    memset(mPrevDstConfig, 0, sizeof(mPrevDstConfig));

    mCurrentRGBMPPIndex = 0;
    memset(&mFbUpdateRegion, 0, sizeof(mFbUpdateRegion));

    mOtfMode = OTF_OFF;
    this->mHwc = pdev;
}

ExynosOverlayDisplay::~ExynosOverlayDisplay()
{
    for (int i = 0; i < mNumMPPs; i++)
        delete mMPPs[i];
    delete[] mMPPs;
}

bool ExynosOverlayDisplay::isOverlaySupported(hwc_layer_1_t &layer, size_t i)
{
    int mMPPIndex = 0;
    int ret = 0;

    if (layer.flags & HWC_SKIP_LAYER) {
        mLayerInfos[i]->mCheckOverlayFlag |= eSkipLayer;
        ALOGV("\tlayer %u: skipping", i);
        return false;
    }

    if (!layer.planeAlpha)
        return true;

    if (i == 0 && layer.planeAlpha < 255) {
        mLayerInfos[i]->mCheckOverlayFlag |= eUnsupportedPlaneAlpha;
        return false;
    }

    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

    if (!handle) {
        ALOGV("\tlayer %u: handle is NULL", i);
        mLayerInfos[i]->mCheckOverlayFlag |= eInvalidHandle;
        return false;
    }

    if (isFormatRgb(handle->format) && handle->stride >= MAX_BUF_STRIDE) {
        mLayerInfos[i]->mCheckOverlayFlag |= eExceedHStrideMaximum;
        return false;
    }

    mMPPIndex = getMPPForUHD(layer);
    if (isFormatRgb(handle->format))
        mMPPIndex = getRGBMPPIndex(0);

    if (!(mMPPs[mMPPIndex]->formatRequiresGsc(handle->format)) &&
          (formatToBpp(handle->format) == 16) && ((layer.displayFrame.left % 2 != 0) || (layer.displayFrame.right % 2 != 0)))
        return false;

    if (isSrcCropFloat(layer.sourceCropf) &&
            getDrmMode(handle->flags) == NO_DRM) {
        ALOGV("\tlayer %u: has floating crop info [%7.1f %7.1f %7.1f %7.1f]", i,
            layer.sourceCropf.left, layer.sourceCropf.top, layer.sourceCropf.right,
            layer.sourceCropf.bottom);
        mLayerInfos[i]->mCheckOverlayFlag |= eHasFloatSrcCrop;
        return false;
    }

    if (visibleWidth(mMPPs[mMPPIndex], layer, handle->format, this->mXres) < BURSTLEN_BYTES) {
        ALOGV("\tlayer %u: visible area is too narrow", i);
        mLayerInfos[i]->mCheckOverlayFlag |= eUnsupportedDstWidth;
        return false;
    }

    if (mMPPs[mMPPIndex]->isProcessingRequired(layer, handle->format)) {
        int downNumerator, downDenominator;
        int downError = mMPPs[mMPPIndex]->getDownscaleRatio(&downNumerator, &downDenominator);
        /* Check whether GSC can handle using local or M2M */
        ret = mMPPs[mMPPIndex]->isProcessingSupported(layer, handle->format, false);
        if (ret < 0) {
            ALOGV("\tlayer %u: MPP M2M required but not supported", i);
            mLayerInfos[i]->mCheckOverlayFlag |= eMPPUnsupported;
            mLayerInfos[i]->mCheckMPPFlag |= -ret;

            ret = mMPPs[mMPPIndex]->isProcessingSupported(layer, handle->format, true, downNumerator, downDenominator);
            if (ret < 0) {
                ALOGV("\tlayer %u: MPP OTF required but not supported", i);
                mLayerInfos[i]->mCheckOverlayFlag |= eMPPUnsupported;
                mLayerInfos[i]->mCheckMPPFlag |= -ret;
                return false;
            }
        }
    } else {
#ifdef USE_FB_PHY_LINEAR
#ifdef G2D_COMPOSITION
        if ((this->mG2dComposition) && (this->mG2dLayers < (int)(NUM_HW_WIN_FB_PHY - 1)))
            this->mG2d.ovly_lay_idx[this->mG2dLayers++] = i;
        else
#endif
        return false;
#endif
        if (!isFormatSupported(handle->format)) {
            mLayerInfos[i]->mCheckOverlayFlag |= eUnsupportedFormat;
            ALOGV("\tlayer %u: pixel format %u not supported", i, handle->format);
            return false;
        }
    }

    if (!isBlendingSupported(layer.blending)) {
        ALOGV("\tlayer %u: blending %d not supported", i, layer.blending);
        mLayerInfos[i]->mCheckOverlayFlag |= eUnsupportedBlending;
        return false;
    }

#ifdef USE_FB_PHY_LINEAR
    if (CC_UNLIKELY(isOffscreen(layer, mXres, mYres))) {
        ALOGW("\tlayer %u: off-screen", i);
        return false;
    }
#endif

    return true;
}

int ExynosOverlayDisplay::prepare(hwc_display_contents_1_t* contents)
{
    ATRACE_CALL();
    ALOGV("preparing %u layers for FIMD", contents->numHwLayers);

    memset(mPostData.gsc_map, 0, sizeof(mPostData.gsc_map));

    mForceFb = mHwc->force_gpu;

    checkTVBandwidth();

    mForceFbYuvLayer = 0;
    mConfigMode = 0;
    mRetry = false;

    ExynosDisplay::allocateLayerInfos(contents);
    /*
     * check whether same config or different config,
     * should be waited until meeting the NUM_COFIG)STABLE
     * before stablizing config, should be composed by GPU
     * faster stablizing config, should be returned by OVERLAY
     */
    forceYuvLayersToFb(contents);

    if (mHwc->force_gpu == true)
        mForceFb = true;

    do {
#ifdef G2D_COMPOSITION
        int tot_pixels = 0;
        mG2dLayers= 0;
        memset(&mG2d, 0, sizeof(mG2d));
        mG2dComposition = 0;

        if (contents->flags & HWC_GEOMETRY_CHANGED) {
            mG2dBypassCount = 4;
            goto SKIP_G2D_OVERLAY;
        }

        if (mG2dBypassCount > 0)
            goto SKIP_G2D_OVERLAY;

        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];

            if ((layer.compositionType == HWC_FRAMEBUFFER_TARGET) ||
                (layer.compositionType == HWC_BACKGROUND && !mForceFb))
                continue;

            if (is_transformed(layer) || is_scaled(layer) || (!layer.handle))
                 goto SKIP_G2D_OVERLAY;

            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
            if (!exynos5_format_is_rgb(handle->format))
                goto SKIP_G2D_OVERLAY;

            tot_pixels += WIDTH(layer.displayFrame) * HEIGHT(layer.displayFrame);
         }

        if ((tot_pixels == mXres * mYres) &&
            (contents->numHwLayers <= NUM_HW_WINDOWS + 1))
            mG2dComposition = 1;
        else
            mG2dComposition = 0;

    SKIP_G2D_OVERLAY:
#endif

        determineYuvOverlay(contents);
        determineSupportedOverlays(contents);
        determineBandwidthSupport(contents);
        assignWindows(contents);
    } while (mRetry);

#ifdef G2D_COMPOSITION
    int alloc_fail = 0;
CHANGE_COMPOS_MODE:
    if (mG2dComposition) {
        if (mGscUsed || mFbNeeded || alloc_fail) {
            //use SKIP_STATIC_LAYER_COMP
            for (size_t i = 0; i < (size_t)mG2dLayers; i++) {
                int lay_idx = mG2d.ovly_lay_idx[i];
                hwc_layer_1_t &layer = contents->hwLayers[lay_idx];
                layer.compositionType = HWC_FRAMEBUFFER;
                mFbNeeded = true;
                for (size_t j = 0; j < NUM_HW_WIN_FB_PHY; j++) {
                    if (mPostData.overlay_map[j] == lay_idx) {
                        mPostData.overlay_map[j] = -1;
                        break;
                    }
                }
            }
            if (mFbNeeded)
                mFirstFb = min(mFirstFb, (size_t)mG2d.ovly_lay_idx[0]);
            mG2dLayers= 0;
            mG2dComposition = 0;
        }
    }

    if (mG2dComposition) {
        alloc_fail = exynos5_g2d_buf_alloc(mHwc, contents);
        if (alloc_fail)
            goto CHANGE_COMPOS_MODE;
    }

    if (contents->flags & HWC_GEOMETRY_CHANGED) {
        for (int i = 0; i < (int)NUM_HW_WIN_FB_PHY; i++)
            mLastG2dLayerHandle[i] = 0;
    }
#endif

    if (mPopupPlayYuvContents)
        mVirtualOverlayFlag = 0;
    else
        skipStaticLayers(contents);
    if (mVirtualOverlayFlag)
        mFbNeeded = 0;

    if (!mFbNeeded)
        mPostData.fb_window = NO_FB_NEEDED;
    else
        if (mPopupPlayYuvContents)
            mPostData.fb_window = 1;

    return 0;
}

void ExynosOverlayDisplay::configureOtfWindow(hwc_rect_t &displayFrame,
        int32_t blending, int32_t planeAlpha, int fence_fd, int format, s3c_fb_win_config &cfg)
{
    uint8_t bpp = formatToBpp(format);

    cfg.state = cfg.S3C_FB_WIN_STATE_OTF;
    cfg.fd = -1;
    cfg.x = displayFrame.left;
    cfg.y = displayFrame.top;
    cfg.w = WIDTH(displayFrame);
    cfg.h = HEIGHT(displayFrame);
    cfg.format = halFormatToS3CFormat(format);
    cfg.offset = 0;
    cfg.stride = cfg.w * bpp / 8;
    cfg.blending = halBlendingToS3CBlending(blending);
    cfg.fence_fd = fence_fd;
    cfg.plane_alpha = 255;
    if (planeAlpha && (planeAlpha < 255)) {
        cfg.plane_alpha = planeAlpha;
    }
}

void ExynosOverlayDisplay::configureHandle(private_handle_t *handle,
        hwc_frect_t &sourceCrop, hwc_rect_t &displayFrame,
        int32_t blending, int32_t planeAlpha, int fence_fd, s3c_fb_win_config &cfg)
{
    uint32_t x, y;
    uint32_t w = WIDTH(displayFrame);
    uint32_t h = HEIGHT(displayFrame);
    uint8_t bpp = formatToBpp(handle->format);
    uint32_t offset = ((uint32_t)sourceCrop.top * handle->stride + (uint32_t)sourceCrop.left) * bpp / 8;

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

void ExynosOverlayDisplay::configureOverlay(hwc_layer_1_t *layer, s3c_fb_win_config &cfg)
{
    hwc_rect_t displayFrame;

    displayFrame.left   = layer->displayFrame.left;
    displayFrame.top    = layer->displayFrame.top;
    displayFrame.right  = layer->displayFrame.right;
    displayFrame.bottom = layer->displayFrame.bottom;

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
    if ((layer->acquireFenceFd >= 0) && this->mForceFbYuvLayer) {
        if (sync_wait(layer->acquireFenceFd, 1000) < 0)
            ALOGE("sync_wait error");
        close(layer->acquireFenceFd);
        layer->acquireFenceFd = -1;
    }
    private_handle_t *handle = private_handle_t::dynamicCast(layer->handle);
    hwc_frect_t originalCrop = layer->sourceCropf;
    if (layer->compositionType == HWC_FRAMEBUFFER_TARGET) {
        layer->sourceCropf.left = (double)mFbUpdateRegion.left;
        layer->sourceCropf.top = (double)mFbUpdateRegion.top;
        layer->sourceCropf.right = (double)mFbUpdateRegion.right;
        layer->sourceCropf.bottom = (double)mFbUpdateRegion.bottom;
        displayFrame.left = mFbUpdateRegion.left;
        displayFrame.top = mFbUpdateRegion.top;
        displayFrame.right = mFbUpdateRegion.right;
        displayFrame.bottom = mFbUpdateRegion.bottom;
    }
    configureHandle(handle, layer->sourceCropf, displayFrame,
            layer->blending, layer->planeAlpha, layer->acquireFenceFd, cfg);
    layer->sourceCropf = originalCrop;
}

void ExynosOverlayDisplay::configureDummyOverlay(hwc_layer_1_t *layer, s3c_fb_win_config &cfg)
{
    hwc_rect_t displayFrame;

    displayFrame.left   = layer->displayFrame.left;
    displayFrame.top    = layer->displayFrame.top;
    displayFrame.right  = layer->displayFrame.right;
    displayFrame.bottom = layer->displayFrame.bottom;

    if (layer->acquireFenceFd >= 0) {
        close(layer->acquireFenceFd);
        layer->acquireFenceFd = -1;
    }

    uint32_t x, y;
    uint32_t w = WIDTH(displayFrame);
    uint32_t h = HEIGHT(displayFrame);

    if (displayFrame.left < 0) {
        unsigned int crop = -displayFrame.left;
        x = 0;
        w -= crop;
    } else {
        x = displayFrame.left;
    }

    if (displayFrame.right > this->mXres) {
        unsigned int crop = displayFrame.right - this->mXres;
        w -= crop;
    }

    if (displayFrame.top < 0) {
        unsigned int crop = -displayFrame.top;
        y = 0;
        h -= crop;
    } else {
        y = displayFrame.top;
    }

    if (displayFrame.bottom > this->mYres) {
        int crop = displayFrame.bottom - this->mYres;
        h -= crop;
    }

    cfg.state = cfg.S3C_FB_WIN_STATE_COLOR;
    cfg.color = 0;

    cfg.x = x;
    cfg.y = y;
    cfg.w = w;
    cfg.h = h;
    cfg.fence_fd = -1;

    return;
}

int ExynosOverlayDisplay::handleWindowUpdate(hwc_display_contents_1_t* contents,
    struct s3c_fb_win_config *config)
{
    exynos5_hwc_post_data_t *pdata = &mPostData;
    int layerIdx = -1;
    int updatedWinCnt = 0;
    int totalWinCnt = 0;
    int bytePerPixel = 0;
    size_t winUpdateInfoIdx;
    hwc_rect updateRect = {this->mXres, this->mYres, 0, 0};
    hwc_rect currentRect = {0, 0, 0, 0};
    bool burstLengthCheckDone = false;
    int alignAdjustment = 0;
    int intersectionWidth = 0;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.hwc.winupdate", value, NULL);

    if (!(!strcmp(value, "1") || !strcmp(value, "true")))
        return 0;

    if (S3C_WIN_UPDATE_IDX < 0)
        return 0;
    winUpdateInfoIdx = S3C_WIN_UPDATE_IDX;

    if (contents->flags & HWC_GEOMETRY_CHANGED)
        return 0;

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        if (config[i].state != config[i].S3C_FB_WIN_STATE_DISABLED) {
            if (mLastGscMap[i].mode == exynos5_gsc_map_t::GSC_LOCAL) {
                HLOGV("[WIN_UPDATE] disabled by GSC-OTF in window(%d)", i);
                return 0;
            }
            totalWinCnt++;
            layerIdx = pdata->overlay_map[totalWinCnt - 1];

            if (winConfigChanged(&config[i], &this->mLastConfigData.config[i])) {
                updatedWinCnt++;

                currentRect.left   = config[i].x;
                currentRect.right  = config[i].x + config[i].w;
                currentRect.top    = config[i].y;
                currentRect.bottom = config[i].y + config[i].h;

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

                    if (handle && !isScaled(layer) && !isRotated(layer)
                        && (damageRegionMod == eDamageRegionPartial)) {
                        HLOGD("[WIN_UPDATE][surfaceDamage]  layer w(%4d) h(%4d),  dirty (%4d, %4d) - (%4d, %4d)",
                                handle->width, handle->height, damageRect.left, damageRect.top, damageRect.right, damageRect.bottom);

                        currentRect.left   = config[i].x - (int32_t)layer.sourceCropf.left + damageRect.left;
                        currentRect.right  = config[i].x - (int32_t)layer.sourceCropf.left + damageRect.right;
                        currentRect.top    = config[i].y - (int32_t)layer.sourceCropf.top  + damageRect.top;
                        currentRect.bottom = config[i].y - (int32_t)layer.sourceCropf.top  + damageRect.bottom;
                        adjustRect(currentRect, mXres, mYres);

                    }
                }
                if ((currentRect.left > currentRect.right) || (currentRect.top > currentRect.bottom)) {
                    HLOGD("[WIN_UPDATE] window(%d) layer(%d) invalid region (%4d, %4d) - (%4d, %4d)",
                        i, layerIdx, currentRect.left, currentRect.top, currentRect.right, currentRect.bottom);
                    return 0;
                }
                HLOGD("[WIN_UPDATE] Updated Window(%d) Layer(%d)  (%4d, %4d) - (%4d, %4d)",
                    i, layerIdx, currentRect.left, currentRect.top, currentRect.right, currentRect.bottom);
                updateRect = expand(updateRect, currentRect);
            }
        }
    }
    if (updatedWinCnt == 0)
        return 0;

    updateRect.left  = ALIGN_DOWN(updateRect.left, WINUPDATE_X_ALIGNMENT);
    updateRect.right = updateRect.left + ALIGN_UP(WIDTH(updateRect), WINUPDATE_W_ALIGNMENT);

    if ((100 * (WIDTH(updateRect) * HEIGHT(updateRect)) / (this->mXres * this->mYres)) > WINUPDATE_THRESHOLD)
        return 0;

    alignAdjustment = max(WINUPDATE_X_ALIGNMENT, WINUPDATE_W_ALIGNMENT);

    while (1) {
        burstLengthCheckDone = true;

        for (int i = 0; i < NUM_HW_WINDOWS; i++) {
            if (config[i].state != config[i].S3C_FB_WIN_STATE_DISABLED) {
                if (config[i].format == S3C_FB_PIXEL_FORMAT_RGB_565)
                    bytePerPixel = 2;
                else
                    bytePerPixel = 4;

                currentRect.left   = config[i].x;
                currentRect.right  = config[i].x + config[i].w;
                currentRect.top    = config[i].y;
                currentRect.bottom = config[i].y + config[i].h;

                intersectionWidth = WIDTH(intersection(currentRect, updateRect));

                HLOGV("[WIN_UPDATE] win[%d] left(%d) right(%d) intersection(%d)", i, currentRect.left, currentRect.right, intersectionWidth);

                if (intersectionWidth != 0 && intersectionWidth * bytePerPixel < BURSTLEN_BYTES) {
                    HLOGV("[WIN_UPDATE] win[%d] insufficient burst length (%d)*(%d) < %d", i, intersectionWidth, bytePerPixel, BURSTLEN_BYTES);
                    burstLengthCheckDone = false;
                    break;
                }
            }
        }
        if (burstLengthCheckDone)
            break;
        HLOGD("[WIN_UPDATE] Adjusting update width. current left(%d) right(%d)", updateRect.left, updateRect.right);
        if (updateRect.left >= alignAdjustment) {
            updateRect.left -= alignAdjustment;
        } else if (updateRect.right + alignAdjustment <= this->mXres) {
            updateRect.right += alignAdjustment;
        } else {
            ALOGD("[WIN_UPDATE] Error during update width adjustment");
            return 0;
        }
    }

    config[winUpdateInfoIdx].state = config[winUpdateInfoIdx].S3C_FB_WIN_STATE_UPDATE;
    config[winUpdateInfoIdx].x = updateRect.left;
    config[winUpdateInfoIdx].y = updateRect.top;
    config[winUpdateInfoIdx].w = ALIGN_UP(WIDTH(updateRect), WINUPDATE_W_ALIGNMENT);
    config[winUpdateInfoIdx].h = HEIGHT(updateRect);

    HLOGD("[WIN_UPDATE] UpdateRegion cfg  (%4d, %4d) w(%4d) h(%4d) updatedWindowCnt(%d)",
        config[winUpdateInfoIdx].x, config[winUpdateInfoIdx].y, config[winUpdateInfoIdx].w, config[winUpdateInfoIdx].h, updatedWinCnt);

    return 1;
}

int ExynosOverlayDisplay::winconfigIoctl(s3c_fb_win_config_data *win_data)
{
    ATRACE_CALL();
    return ioctl(this->mDisplayFd, S3CFB_WIN_CONFIG, win_data);
}

int ExynosOverlayDisplay::postFrame(hwc_display_contents_1_t* contents)
{
    ATRACE_CALL();
    exynos5_hwc_post_data_t *pdata = &mPostData;
    struct s3c_fb_win_config_data win_data;
    struct s3c_fb_win_config *config = win_data.config;
    int win_map = 0;
    int tot_ovly_wins = 0;

#ifdef G2D_COMPOSITION
    int num_g2d_overlayed = 0;
#endif

    memset(config, 0, sizeof(win_data.config));
    for (size_t i = 0; i < NUM_HW_WINDOWS; i++)
        config[i].fence_fd = -1;

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        if ( pdata->overlay_map[i] != -1)
            tot_ovly_wins++;
    }
    if (mVirtualOverlayFlag)
        tot_ovly_wins++;

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        int layer_idx = pdata->overlay_map[i];
        if (layer_idx != -1) {
            hwc_layer_1_t &layer = contents->hwLayers[layer_idx];
            win_map = getDeconWinMap(i, tot_ovly_wins);
            if (pdata->gsc_map[i].mode == exynos5_gsc_map_t::GSC_M2M) {
                if (contents->flags & HWC_GEOMETRY_CHANGED)
                    mMPPs[pdata->gsc_map[i].idx]->mLastGSCLayerHandle = -1;
                if (postGscM2M(layer, config, win_map, i, !!(i == 0)) < 0)
                    continue;
            } else if (pdata->gsc_map[i].idx == FIMD_GSC_IDX && this->mOtfMode == OTF_RUNNING &&
                    pdata->gsc_map[i].mode == exynos5_gsc_map_t::GSC_LOCAL) {
                postGscOtf(layer, config, win_map, i);
#ifdef G2D_COMPOSITION
            } else if (this->mG2dComposition && (num_g2d_overlayed < this->mG2dLayers)){
                waitForRenderFinish(&layer.handle, 1);
                private_handle_t dstHandle(*(private_handle_t::dynamicCast(layer.handle)));
                hwc_frect_t sourceCrop = {0, 0, WIDTH(layer.displayFrame), HEIGHT(layer.displayFrame)};
                int fence = exynos5_config_g2d(mHwc, layer, &dstHandle, config[win_map], num_g2d_overlayed, i);
                if (fence < 0) {
                    ALOGE("config_g2d failed for layer %u", i);
                    continue;
                }
                configureHandle(&dstHandle, sourceCrop, layer.displayFrame,
                        layer.blending, layer.planeAlpha, fence, *config);
                num_g2d_overlayed++;
                this->mG2d.win_used[i] = num_g2d_overlayed;
#endif
            } else {
                waitForRenderFinish(&layer.handle, 1);
                configureOverlay(&layer, config[win_map]);
            }
        }
        if (i == 0 && config[i].blending != S3C_FB_BLENDING_NONE) {
            ALOGV("blending not supported on window 0; forcing BLENDING_NONE");
            config[i].blending = S3C_FB_BLENDING_NONE;
        }
        if ((i < S3C_WIN_UPDATE_IDX) &&
                (config[i].state != config[i].S3C_FB_WIN_STATE_DISABLED) &&
                (config[i].w == 0 || config[i].h == 0)) {
            config[i].state = config[i].S3C_FB_WIN_STATE_DISABLED;
        }

        ALOGV("window %u configuration:", i);
        dumpConfig(config[win_map]);
    }

    if (this->mVirtualOverlayFlag) {
        handleStaticLayers(contents, win_data, tot_ovly_wins);
    }

    if (!handleWindowUpdate(contents, config))
        HLOGD("[WIN_UPDATE] Not Triggered");

    int ret = winconfigIoctl(&win_data);
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

    if (!mGscLayers || this->mOtfMode == OTF_TO_M2M) {
        cleanupGscs();
    }

#ifdef G2D_COMPOSITION
    if (!this->mG2dComposition)
        exynos5_cleanup_g2d(mHwc, 0);

    this->mG2dBypassCount = max( this->mG2dBypassCount-1, 0);
#endif

    memcpy(&(this->mLastConfigData), &win_data, sizeof(win_data));
    memcpy(this->mLastGscMap, pdata->gsc_map, sizeof(pdata->gsc_map));
    if (!this->mVirtualOverlayFlag)
        this->mLastFbWindow = pdata->fb_window;
    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        int layer_idx = pdata->overlay_map[i];
        if (layer_idx != -1) {
            hwc_layer_1_t &layer = contents->hwLayers[layer_idx];
            win_map = getDeconWinMap(i, tot_ovly_wins);
            this->mLastHandles[win_map] = layer.handle;
        }
    }

    return win_data.fence;
}

int ExynosOverlayDisplay::clearDisplay()
{
    struct s3c_fb_win_config_data win_data;
    memset(&win_data, 0, sizeof(win_data));

    int ret = ioctl(this->mDisplayFd, S3CFB_WIN_CONFIG, &win_data);
    LOG_ALWAYS_FATAL_IF(ret < 0,
            "ioctl S3CFB_WIN_CONFIG failed to clear screen: %s",
            strerror(errno));
    // the causes of an empty config failing are all unrecoverable

    return win_data.fence;
}

int ExynosOverlayDisplay::set(hwc_display_contents_1_t* contents)
{
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
            ALOGV("framebuffer target buffer:");
            dumpLayer(fb_layer);
        }
    }

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        if (contents->hwLayers[i].compositionType == HWC_OVERLAY &&
            contents->hwLayers[i].planeAlpha == 0) {
            close(contents->hwLayers[i].acquireFenceFd);
            contents->hwLayers[i].acquireFenceFd = -1;
            contents->hwLayers[i].releaseFenceFd = -1;
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

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        if (this->mPostData.overlay_map[i] != -1) {
            hwc_layer_1_t &layer =
                    contents->hwLayers[this->mPostData.overlay_map[i]];
            int dup_fd = dup(fence);
            if (dup_fd < 0)
                ALOGW("release fence(%d) dup failed : %s", fence, strerror(errno));
            if (this->mPostData.gsc_map[i].mode == exynos5_gsc_map_t::GSC_M2M) {
                int gsc_idx = this->mPostData.gsc_map[i].idx;
                ExynosMPP &gsc = *mMPPs[gsc_idx];
                if (gsc.mDstBufFence[gsc.mCurrentBuf] >= 0) {
                    close (gsc.mDstBufFence[gsc.mCurrentBuf]);
                    gsc.mDstBufFence[gsc.mCurrentBuf] = -1;
                }
                gsc.mDstBufFence[gsc.mCurrentBuf] = dup_fd;
                gsc.mCurrentBuf = (gsc.mCurrentBuf + 1) % gsc.mNumAvailableDstBuffers;
            } else if (this->mPostData.gsc_map[i].mode == exynos5_gsc_map_t::GSC_LOCAL) {
                layer.releaseFenceFd = dup_fd;
#ifdef G2D_COMPOSITION
            } else if (this->mG2dComposition && this->mG2d.win_used[i]) {
                int idx = this->mG2d.win_used[i] - 1;
                this->mWinBufFence[idx][this->mG2dCurrentBuffer[idx]] = dup_fd;
                this->mG2dCurrentBuffer[idx] =  (this->mG2dCurrentBuffer[idx] + 1) % NUM_GSC_DST_BUFS;
#endif
            } else {
                layer.releaseFenceFd = dup_fd;
            }
        }
    }

    if (contents->numHwLayers >= 1) {
        hwc_layer_1_t &layer = contents->hwLayers[contents->numHwLayers-1];

        if (mVirtualOverlayFlag && layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            int dup_fd = dup(fence);
            if (dup_fd < 0) {
                ALOGW("release fence(%d) dup failed in FB_TARGET: %s", fence, strerror(errno));
            } else {
                layer.releaseFenceFd = dup_fd;
            }
        }
    }

    contents->retireFenceFd = fence;

    mHwc->hwc_ctrl.max_num_ovly = mAllowedOverlays;

    return err;
}

int ExynosOverlayDisplay::getCompModeSwitch()
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
    if (mYuvLayers || mGscLayers >= 2 ) {
        if (mHwc->CompModeSwitch != HWC_2_GLES) {
            return 0;
        } else {
            mHwc->CompModeSwitch = GLES_2_HWC;
            mHwc->updateCallCnt = 0;
            mHwc->LastModeSwitchTimeStamp = mHwc->LastUpdateTimeStamp;
            ALOGI("[DYNAMIC_RECOMP] GLES_2_HWC by video layer");
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
            ALOGI("[DYNAMIC_RECOMP] GLES_2_HWC by BW check");
            return GLES_2_HWC;
        }
    }

    /*
     * There will be at least one composition call per one minute (because of time update)
     * To minimize the analysis overhead, just analyze it once in a second
     */
    TimeStampDiff = systemTime(SYSTEM_TIME_MONOTONIC) - mHwc->LastModeSwitchTimeStamp;

    /* check fps every 250ms from LastModeSwitchTimeStamp*/
    if (TimeStampDiff < (VSYNC_INTERVAL * 15)) {
        return 0;
    }
    mHwc->LastModeSwitchTimeStamp = mHwc->LastUpdateTimeStamp;
    Temp = (VSYNC_INTERVAL * 60) / TimeStampDiff;
    updateFps = (int)(mHwc->updateCallCnt * Temp + 0.5);
    mHwc->updateCallCnt = 0;
    /*
     * FPS estimation.
     * If FPS is lower than HWC_FPS_TH, try to switch the mode to GLES
     */
    if (updateFps < HWC_FPS_TH) {
        if (mHwc->CompModeSwitch != HWC_2_GLES) {
            mHwc->CompModeSwitch = HWC_2_GLES;
            ALOGI("[DYNAMIC_RECOMP] HWC_2_GLES by low FPS(%d)", updateFps);
            return HWC_2_GLES;
        } else {
            return 0;
        }
    } else {
         if (mHwc->CompModeSwitch == HWC_2_GLES) {
            mHwc->CompModeSwitch = GLES_2_HWC;
            ALOGI("[DYNAMIC_RECOMP] GLES_2_HWC by high FPS(%d)", updateFps);
            return GLES_2_HWC;
        } else {
            return 0;
        }
    }

    return 0;
}

int32_t ExynosOverlayDisplay::getDisplayAttributes(const uint32_t attribute)
{
    switch(attribute) {
    case HWC_DISPLAY_VSYNC_PERIOD:
        return this->mVsyncPeriod;

    case HWC_DISPLAY_WIDTH:
        return this->mXres;

    case HWC_DISPLAY_HEIGHT:
        return this->mYres;

    case HWC_DISPLAY_DPI_X:
        return this->mXdpi;

    case HWC_DISPLAY_DPI_Y:
        return this->mYdpi;

    default:
        ALOGE("unknown display attribute %u", attribute);
        return -EINVAL;
    }
}

void ExynosOverlayDisplay::skipStaticLayers(hwc_display_contents_1_t* contents)
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

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        if (mPostData.overlay_map[i] != -1) {
            last_ovly_lay_idx = mPostData.overlay_map[i];
            mLastOverlayWindowIndex = i;
        }
    }

    if ((last_ovly_lay_idx == -1) || !mFbNeeded ||
        ((mLastFb - mFirstFb + 1) > NUM_VIRT_OVER)) {
        init_flag = 0;
        return;
    }
    mLastOverlayLayerIndex = last_ovly_lay_idx;

    if (init_flag == 1) {
        for (size_t i = mFirstFb; i <= mLastFb; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (!layer.handle || (layer.flags & HWC_SKIP_LAYER) || (mLastLayerHandles[i - mFirstFb] !=  layer.handle)) {
                init_flag = 0;
                return;
            }
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
        mLastFbWindow = mPostData.fb_window;
        return;
    }

    init_flag = 1;
    for (size_t i = 0; i < NUM_VIRT_OVER; i++)
        mLastLayerHandles[i] = 0;

    for (size_t i = mFirstFb; i <= mLastFb; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        mLastLayerHandles[i - mFirstFb] = layer.handle;
    }

    return;
}

void ExynosOverlayDisplay::forceYuvLayersToFb(hwc_display_contents_1_t *contents)
{
}

void ExynosOverlayDisplay::handleOffscreenRendering(hwc_layer_1_t &layer, hwc_display_contents_1_t *contents, int index)
{
}

void ExynosOverlayDisplay::determineYuvOverlay(hwc_display_contents_1_t *contents)
{
    mPopupPlayYuvContents = false;
    mForceOverlayLayerIndex = -1;
    mHasDrmSurface = false;
    mYuvLayers = 0;
    mHasCropSurface = false;
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
            if (getDrmMode(handle->flags) != NO_DRM) {
                this->mHasDrmSurface = true;
                mForceOverlayLayerIndex = i;
            }

            /* check yuv surface */
            if (mMPPs[0]->formatRequiresGsc(handle->format)) {
	        layer.sourceCropf.top = (int)layer.sourceCropf.top;
                layer.sourceCropf.left = (int)layer.sourceCropf.left;
                layer.sourceCropf.bottom = (int)(layer.sourceCropf.bottom + 0.9);
                layer.sourceCropf.right = (int)(layer.sourceCropf.right + 0.9);
                if (isOverlaySupported(contents->hwLayers[i], i)) {
                    this->mYuvLayers++;
                    if (this->mHasDrmSurface == false)
                        mForceOverlayLayerIndex = i;
                } else if (i > 0 && layer.planeAlpha > 0) {
                    mForceFb = true;
                }
            }

            if (handle->flags & GRALLOC_USAGE_HW_CAMERA_MASK) {
                mAllowedOverlays = mHwc->hwc_ctrl.max_num_ovly;
                mHwc->hwc_ctrl.max_num_ovly = 3;
            }

            handleOffscreenRendering(layer, contents, i);
        }

#if defined(USE_GRALLOC_FLAG_FOR_HDMI) || defined(USES_VIRTUAL_DISPLAY)
        /* during rotation, it use gpu comosition */
        if (layer.flags & HWC_SCREENSHOT_ANIMATOR_LAYER)
            mForceFb = true;
#endif
    }
    mPopupPlayYuvContents = !!(((mYuvLayers == 1) || (this->mHasDrmSurface)) && (mForceOverlayLayerIndex > 0));
    if (mYuvLayers > 1) {
        mHwc->allowOTF = false;
        if (mMPPs[0]->isOTF())
            mForceFb = true;
    } else {
        mHwc->allowOTF = true;
    }

#ifdef USE_FB_PHY_LINEAR
    if ((!mHasDrmSurface) &&
#ifdef G2D_COMPOSITION
            (mG2dBypassCount > 0) &&
#endif
            (contents->flags & HWC_GEOMETRY_CHANGED))
        mForceFb = true;
#endif
}

void ExynosOverlayDisplay::determineSupportedOverlays(hwc_display_contents_1_t *contents)
{
    bool videoLayer = false;

    mFbNeeded = false;
    mFirstFb = ~0;
    mLastFb = 0;

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++)
        mPostData.overlay_map[i] = -1;

    // find unsupported overlays
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            ALOGV("\tlayer %u: framebuffer target", i);
            mLayerInfos[i]->compositionType = layer.compositionType;
            continue;
        }

        if (layer.compositionType == HWC_BACKGROUND && !mForceFb) {
            ALOGV("\tlayer %u: background supported", i);
            dumpLayer(&contents->hwLayers[i]);
            mLayerInfos[i]->compositionType = layer.compositionType;
            continue;
        }

        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
            if ((int)get_yuv_planes(halFormatToV4L2Format(handle->format)) > 0) {
                videoLayer = true;
                if (!mHwc->hdmi_hpd && mHwc->mS3DMode == S3D_MODE_READY)
                    mHwc->mS3DMode = S3D_MODE_RUNNING;
            }
            if ((!mPopupPlayYuvContents)
                    || ((uint32_t)mForceOverlayLayerIndex == i)) {
                mHwc->incomingPixels += WIDTH(layer.displayFrame) * HEIGHT(layer.displayFrame);
                if ((!mHasCropSurface || mPopupPlayYuvContents) ||
                    ((mHasDrmSurface) && ((uint32_t)mForceOverlayLayerIndex == i))) {
                    if (isOverlaySupported(contents->hwLayers[i], i) &&
                        !(mForceFb && (getDrmMode(handle->flags) == NO_DRM)) && (!mHwc->hwc_ctrl.dynamic_recomp_mode ||
                                      ((mHwc->CompModeSwitch != HWC_2_GLES) ||
                                      (getDrmMode(handle->flags) != NO_DRM)))) {
                        mHwc->totPixels += WIDTH(layer.displayFrame) * HEIGHT(layer.displayFrame);
                        ALOGV("\tlayer %u: overlay supported", i);
                        layer.compositionType = HWC_OVERLAY;
                        mLayerInfos[i]->compositionType = layer.compositionType;
                        if (mPopupPlayYuvContents)
                            layer.hints = HWC_HINT_CLEAR_FB;
                        dumpLayer(&contents->hwLayers[i]);
                        continue;
                    } else {
                        if (mForceFb)
                            mLayerInfos[i]->mCheckOverlayFlag |= eForceFbEnabled;
                        else if (mHwc->hwc_ctrl.dynamic_recomp_mode && mHwc->CompModeSwitch == HWC_2_GLES)
                            mLayerInfos[i]->mCheckOverlayFlag |= eDynamicRecomposition;
                        else if (isOverlaySupported(contents->hwLayers[i], i))
                            mLayerInfos[i]->mCheckOverlayFlag |= eUnknown;
                    }
                } else {
                    if (mHasCropSurface == true)
                        mLayerInfos[i]->mCheckOverlayFlag |= eUnsupportedCoordinate;
                    else
                        mLayerInfos[i]->mCheckOverlayFlag |= eUnknown;
                }
            } else {
                /* There is popup video but this layer is not popup video layer */
                mLayerInfos[i]->mCheckOverlayFlag |= eHasPopupVideo;
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

        dumpLayer(&contents->hwLayers[i]);
    }

    if (!mHwc->hdmi_hpd && mHwc->mS3DMode == S3D_MODE_RUNNING && !videoLayer && !mForceFb)
        mHwc->mS3DMode = S3D_MODE_DISABLED;
    hwc_rect_t base_rect;
    hwc_rect_t intersect_rect;
    private_handle_t *handle = NULL;
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.handle)
            handle = private_handle_t::dynamicCast(layer.handle);
        if (layer.compositionType == HWC_OVERLAY) {
            if (i == 0) {
                base_rect =  layer.displayFrame;
            } else if (hasPlaneAlpha(layer) || (handle && hasAlpha(handle->format) && mMPPs[0]->isProcessingRequired(layer, handle->format))) {
                //if alpha layer is not completely overlapped with base layer, bypass the alpha layer to GLES.
                intersect_rect = intersection(base_rect, layer.displayFrame);
                if (!rectEqual(intersect_rect, layer.displayFrame)) {
                    layer.compositionType = HWC_FRAMEBUFFER;
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
        for (size_t i = mFirstFb; i < mLastFb; i++) {
            if (mPopupPlayYuvContents && ((uint32_t)mForceOverlayLayerIndex == i)) {
                mFirstFb = 1;
                break;
            }
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if (layer.handle) {
                private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
                if (getDrmMode(handle->flags) == NO_DRM) {
                    contents->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
                    mLayerInfos[i]->compositionType = contents->hwLayers[i].compositionType;
                    mLayerInfos[i]->mCheckOverlayFlag |= eSandwitchedBetweenGLES;
                }
            }
        }
    }
}

void ExynosOverlayDisplay::determineBandwidthSupport(hwc_display_contents_1_t *contents)
{
    // Incrementally try to add our supported layers to hardware windows.
    // If adding a layer would violate a hardware constraint, force it
    // into the framebuffer and try again.  (Revisiting the entire list is
    // necessary because adding a layer to the framebuffer can cause other
    // windows to retroactively violate constraints.)
    bool changed;
    this->mBypassSkipStaticLayer = false;
    unsigned int cannotComposeFlag = 0;
    bool addFB = true;
    hwc_rect windowIntersection;
    windowIntersection.left = 0;
    windowIntersection.top = 0;
    windowIntersection.right = mXres;
    windowIntersection.bottom = mYres;
    int intersectionCnt = 0;
    bool reduceAvailableWindowCnt = false;
    // Initialize to inverse values so that
    // min(left, l) = l, min(top, t) = t
    // max(right, r) = r, max(bottom, b) = b
    // for all l, t, r, b
    mFbUpdateRegion.left = mXres;
    mFbUpdateRegion.top = mYres;
    mFbUpdateRegion.right = 0;
    mFbUpdateRegion.bottom = 0;

    uint32_t pixel_used[MAX_NUM_FIMD_DMA_CH];
    do {
        android::Vector<hwc_rect> rects[MAX_NUM_FIMD_DMA_CH];
        android::Vector<hwc_rect> overlaps[MAX_NUM_FIMD_DMA_CH];
        int dma_ch_idx;
        size_t win_idx = 0;
        size_t windows_left;
        memset(&pixel_used[0], 0, sizeof(pixel_used));
        mGscUsed = false;
        windowIntersection.left = 0;
        windowIntersection.top = 0;
        windowIntersection.right = mXres;
        windowIntersection.bottom = mYres;
        intersectionCnt = 0;

        if (mFbNeeded) {
            hwc_rect_t fb_rect;
            fb_rect.top = fb_rect.left = 0;
            fb_rect.right = this->mXres - 1;
            fb_rect.bottom = this->mYres - 1;
            dma_ch_idx = FIMD_DMA_CH_IDX[mFirstFb];
            pixel_used[dma_ch_idx] = (uint32_t) (this->mXres * this->mYres);
            win_idx = (win_idx == mFirstFb) ? (win_idx + 1) : win_idx;
#ifdef USE_FB_PHY_LINEAR
            windows_left = 1;
#ifdef G2D_COMPOSITION
            if (this->mG2dComposition)
            windows_left = NUM_HW_WIN_FB_PHY - 1;
#endif
#else
            windows_left = mHwc->hwc_ctrl.max_num_ovly - 1;
#endif
            if (reduceAvailableWindowCnt)
                windows_left = min(windows_left, mMaxWindowOverlapCnt - 1);
            rects[dma_ch_idx].push_back(fb_rect);
            if (addFB) {
                addFB = false;
                mHwc->totPixels += mXres * mYres;
            }
        }
        else {
#ifdef USE_FB_PHY_LINEAR
            windows_left = 1;
#ifdef G2D_COMPOSITION
            if (this->mG2dComposition)
                windows_left = NUM_HW_WIN_FB_PHY;
#endif
#else
            windows_left = mHwc->hwc_ctrl.max_num_ovly;
#endif
            if (reduceAvailableWindowCnt)
                windows_left = min(windows_left, mMaxWindowOverlapCnt);
        }

        changed = false;
        mGscLayers = 0;
        mCurrentGscIndex = 0;
        mCurrentRGBMPPIndex = 0;
        bool switchingOTF = false;
        reduceAvailableWindowCnt = false;
        for (size_t i = 0; i < contents->numHwLayers; i++) {
            hwc_layer_1_t &layer = contents->hwLayers[i];
            if ((layer.flags & HWC_SKIP_LAYER) ||
                    layer.compositionType == HWC_FRAMEBUFFER_TARGET)
                continue;

            private_handle_t *handle = private_handle_t::dynamicCast(
                    layer.handle);

            if (!layer.planeAlpha)
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
            dma_ch_idx = FIMD_DMA_CH_IDX[win_idx];

            size_t pixels_needed = 0;
            if (getDrmMode(handle->flags) != SECURE_DRM)
                pixels_needed = getRequiredPixels(layer, mXres, mYres);
            else
                pixels_needed = WIDTH(layer.displayFrame) *
                    HEIGHT(layer.displayFrame);

            cannotComposeFlag = 0;
            bool can_compose = windows_left && (win_idx < NUM_HW_WINDOWS) &&
                            ((pixel_used[dma_ch_idx] + pixels_needed) <=
                            (uint32_t)this->mDmaChannelMaxBandwidth[dma_ch_idx]);
            if (windows_left <= 0 || (win_idx >= NUM_HW_WINDOWS))
                cannotComposeFlag |= eInsufficientWindow;
            if ((pixel_used[dma_ch_idx] + pixels_needed) > (uint32_t)this->mDmaChannelMaxBandwidth[dma_ch_idx])
                cannotComposeFlag |= eInsufficientBandwidth;

            int gsc_index = getMPPForUHD(layer);

            bool gsc_required = mMPPs[gsc_index]->isProcessingRequired(layer, handle->format);
            if (gsc_required) {
                if (mHasDrmSurface && getDrmMode(handle->flags) == NO_DRM && MAX_VIDEO_LAYERS == 1) {
                    can_compose = false;
                    cannotComposeFlag = eHasDRMVideo;
                }
                if (mGscLayers >= MAX_VIDEO_LAYERS) {
                    can_compose = can_compose && !mGscUsed;
                    if (mGscUsed)
                        cannotComposeFlag |= eInsufficientMPP;
                }
                if (mHwc->hwc_ctrl.num_of_video_ovly <= mGscLayers) {
                    can_compose = false;
                    cannotComposeFlag |= eInsufficientMPP;
                }
                if (switchingOTF) {
                    can_compose = false;
                    cannotComposeFlag |= eSwitchingLocalPath;
                }
                if (mOtfMode == OTF_RUNNING && win_idx > 0)
                    switchingOTF = true;
                if (multipleRGBScaling(handle->format)) {
                    can_compose = false;
                    cannotComposeFlag |= eRGBLayerDuringVideoPlayback;
                }
            }

            // hwc_rect_t right and bottom values are normally exclusive;
            // the intersection logic is simpler if we make them inclusive
            hwc_rect_t visible_rect = layer.displayFrame;
            visible_rect.right--; visible_rect.bottom--;

            if (can_compose) {
                switch (this->mDmaChannelMaxOverlapCount[dma_ch_idx]) {
                case 1: // It means, no layer overlap is allowed
                    for (size_t j = 0; j < rects[dma_ch_idx].size(); j++)
                         if (intersect(visible_rect, rects[dma_ch_idx].itemAt(j))) {
                            can_compose = false;
                            cannotComposeFlag |= eInsufficientOverlapCount;
                         }
                    break;
                case 2: //It means, upto 2 layer overlap is allowed.
                    for (size_t j = 0; j < overlaps[dma_ch_idx].size(); j++)
                        if (intersect(visible_rect, overlaps[dma_ch_idx].itemAt(j))) {
                            can_compose = false;
                            cannotComposeFlag |= eInsufficientOverlapCount;
                        }
                    break;
                default:
                    break;
                }
                if (!can_compose)
                    this->mBypassSkipStaticLayer = true;
            }

            if (!can_compose) {
                if (getDrmMode(handle->flags) != NO_DRM) {
                    mFirstFb = 1;
                    changed = true;
                    break;
                } else {
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
                    mFirstFb = min(mFirstFb, (size_t)NUM_HW_WINDOWS-1);
                    mHwc->totPixels -= WIDTH(layer.displayFrame) * HEIGHT(layer.displayFrame);
                    break;
                }
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
            win_idx = min(win_idx, NUM_HW_WINDOWS - 1);
            windows_left--;
            if (gsc_required) {
                mGscUsed = true;
                mGscLayers++;
            }

            if (intersect(visible_rect, windowIntersection)) {
                windowIntersection = intersection(visible_rect, windowIntersection);
                intersectionCnt++;
            } else {
                windowIntersection = {0, 0, 0, 0};
                intersectionCnt = 0;
            }
        }
        if (mFbNeeded && intersectionCnt != 0)
            intersectionCnt++;
        if (changed) {
            for (size_t i = mFirstFb; i < mLastFb; i++) {
                hwc_layer_1_t &layer = contents->hwLayers[i];
                if (layer.handle) {
                    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
                    if (getDrmMode(handle->flags) == NO_DRM) {
                        contents->hwLayers[i].compositionType = HWC_FRAMEBUFFER;
                        mLayerInfos[i]->compositionType = contents->hwLayers[i].compositionType;
                        mLayerInfos[i]->mCheckOverlayFlag |= eSandwitchedBetweenGLES;
                    }
                }
            }
        }
        handleTotalBandwidthOverload(contents);
        if (intersectionCnt > mMaxWindowOverlapCnt) {
            HLOGD("Total Overlap Cnt(%d) >  Max Overlap Cnt(%d)", intersectionCnt, mMaxWindowOverlapCnt);
            changed = true;
            reduceAvailableWindowCnt = true;
        }
    } while(changed);

    if (mGscLayers == 0)
        mHwc->allowOTF = true;

    if (mPopupPlayYuvContents) {
        mFbUpdateRegion.left = 0;
        mFbUpdateRegion.top = 0;
        mFbUpdateRegion.right = mXres;
        mFbUpdateRegion.bottom = mYres;
        return;
    }

    for (size_t i = mFirstFb; i <= mLastFb; i++)
        mFbUpdateRegion = expand(mFbUpdateRegion, contents->hwLayers[i].displayFrame);

    for (size_t i = mLastFb + 1; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.compositionType == HWC_OVERLAY && layer.planeAlpha > 0 && layer.planeAlpha < 255)
            mFbUpdateRegion = expand(mFbUpdateRegion, contents->hwLayers[i].displayFrame);
    }

    int minWidth = BURSTLEN_BYTES * 8 / formatToBpp(HAL_PIXEL_FORMAT_RGBA_8888);
    int w = WIDTH(mFbUpdateRegion);
    if (w < minWidth) {
        if (mFbUpdateRegion.left + minWidth <= mXres)
            mFbUpdateRegion.right = mFbUpdateRegion.left + minWidth;
        else
            mFbUpdateRegion.left = mFbUpdateRegion.right - minWidth;
    }
}

void ExynosOverlayDisplay::assignWindows(hwc_display_contents_1_t *contents)
{
    unsigned int nextWindow = 0;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (!layer.planeAlpha)
            continue;

        if (!mPopupPlayYuvContents) {
            if (mFbNeeded && i == mFirstFb) {
                ALOGV("assigning framebuffer to window %u\n",
                        nextWindow);
                mPostData.fb_window = nextWindow;
                nextWindow++;
                continue;
            }
        }

        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
            if (ExynosMPP::isFormatSupportedByGscOtf(handle->format)) {
                /* in case of changing compostiontype form GSC to FRAMEBUFFER for yuv layer */
                if ((mConfigMode == 1) && (layer.compositionType == HWC_FRAMEBUFFER)) {
                    mForceFbYuvLayer = 1;
                    mConfigMode = 0;
                    mCountSameConfig = 0;
                    /* for prepare */
                    mForceFb = 1;
                    mRetry = true;
                    return;
                }
            }
        }
        if (layer.compositionType != HWC_FRAMEBUFFER &&
                layer.compositionType != HWC_FRAMEBUFFER_TARGET) {
            ALOGV("assigning layer %u to window %u", i, nextWindow);
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

bool ExynosOverlayDisplay::assignGscLayer(hwc_layer_1_t &layer, int index, int nextWindow)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    size_t gscIndex = 0;
    int downNumerator, downDenominator;
    int downError = mMPPs[0]->getDownscaleRatio(&downNumerator, &downDenominator);
    bool ret = true;
    if (nextWindow == 0 &&
            !(handle->flags & GRALLOC_USAGE_HW_CAMERA_MASK) &&
            !downError &&
            (mMPPs[0]->isProcessingSupported(layer, handle->format, true, downNumerator, downDenominator) > 0)) {
        gscIndex = mCurrentGscIndex;
        ALOGV("\tusing gscaler %u in LOCAL-PATH", AVAILABLE_GSC_UNITS[gscIndex]);
        if (!mMPPs[gscIndex]->isOTF())
            mMPPs[gscIndex]->cleanupM2M(true);
        mPostData.gsc_map[nextWindow].mode = exynos5_gsc_map_t::GSC_LOCAL;
        mMPPs[gscIndex]->setMode(exynos5_gsc_map_t::GSC_LOCAL);
        mPostData.gsc_map[nextWindow].idx = FIMD_GSC_IDX;
        if (mOtfMode == OTF_OFF)
            mOtfMode = OTF_RUNNING;
        mMPPs[0]->mNeedReqbufs = false;
    } else {
        gscIndex = getMPPForUHD(layer);
        if (gscIndex == FIMD_GSC_IDX) {
            gscIndex = mCurrentGscIndex;
            ret = true;
        } else {
            ret = false;
        }
        if (isFormatRgb(handle->format)) {
            gscIndex = getRGBMPPIndex(mCurrentRGBMPPIndex);
            if (gscIndex != FIMD_GSC_IDX && gscIndex != FIMD_GSC_SEC_IDX)
                mCurrentRGBMPPIndex++;
        }
        ALOGV("\tusing gscaler %u in M2M", AVAILABLE_GSC_UNITS[gscIndex]);
        mPostData.gsc_map[nextWindow].mode = exynos5_gsc_map_t::GSC_M2M;
        mMPPs[gscIndex]->setMode(exynos5_gsc_map_t::GSC_M2M);
        mPostData.gsc_map[nextWindow].idx = gscIndex;
        if ((gscIndex == FIMD_GSC_IDX || (mMPPs[gscIndex]->isUsingMSC() && mCurrentGscIndex == 0)) && mOtfMode == OTF_RUNNING) {
            ALOGV("change from OTF to M2M");
            mOtfMode = OTF_TO_M2M;
            if (mMPPs[gscIndex]->isUsingMSC()) {
                mMPPs[0]->setMode(exynos5_gsc_map_t::GSC_LOCAL);
            } else {
                mMPPs[gscIndex]->setMode(exynos5_gsc_map_t::GSC_LOCAL);
                mMPPs[FIMD_GSC_SEC_IDX]->setMode(exynos5_gsc_map_t::GSC_M2M);
                mPostData.gsc_map[nextWindow].idx = FIMD_GSC_SEC_IDX;
            }
        }
    }
    ALOGV("\tusing gscaler %u",
            AVAILABLE_GSC_UNITS[gscIndex]);
    return ret;
}

int ExynosOverlayDisplay::waitForRenderFinish(buffer_handle_t *handle, int buffers)
{
    return 0;
}

int ExynosOverlayDisplay::postGscM2M(hwc_layer_1_t &layer, struct s3c_fb_win_config *config, int win_map, int index, bool isBottom)
{
    exynos5_hwc_post_data_t *pdata = &mPostData;
    int gsc_idx = pdata->gsc_map[index].idx;
    int dst_format = HAL_PIXEL_FORMAT_RGBX_8888;
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    int32_t deconBlending = HWC_BLENDING_NONE;
    int32_t originalBlending = HWC_BLENDING_NONE;

    hwc_frect_t sourceCrop = { 0, 0,
            (float)WIDTH(layer.displayFrame), (float)HEIGHT(layer.displayFrame) };
    if (mHwc->mS3DMode == S3D_MODE_READY || mHwc->mS3DMode == S3D_MODE_RUNNING) {
        int S3DFormat = getS3DFormat(mHwc->mHdmiPreset);
        if (S3DFormat == S3D_SBS)
            mMPPs[gsc_idx]->mS3DMode = S3D_SBS;
        else if (S3DFormat == S3D_TB)
            mMPPs[gsc_idx]->mS3DMode = S3D_TB;
    } else {
        mMPPs[gsc_idx]->mS3DMode = S3D_NONE;
    }

    if (isFormatRgb(handle->format))
        waitForRenderFinish(&layer.handle, 1);
    /* OFF_Screen to ON_Screen changes */
    if (getDrmMode(handle->flags) == SECURE_DRM)
        recalculateDisplayFrame(layer, mXres, mYres);

    if (handle->format == HAL_PIXEL_FORMAT_RGB_565)
        dst_format = HAL_PIXEL_FORMAT_RGB_565;

    if (isBottom) {
        originalBlending = layer.blending;
        layer.blending = HWC_BLENDING_NONE;
    }

    int err = mMPPs[gsc_idx]->processM2M(layer, dst_format, &sourceCrop);

    if (isBottom)
        layer.blending = originalBlending;

    if (err < 0) {
        ALOGE("failed to configure gscaler %u for layer %u",
                gsc_idx, index);
        pdata->gsc_map[index].mode = exynos5_gsc_map_t::GSC_NONE;
        return -1;
    }

    buffer_handle_t dst_buf = mMPPs[gsc_idx]->mDstBuffers[mMPPs[gsc_idx]->mCurrentBuf];
    private_handle_t *dst_handle =
            private_handle_t::dynamicCast(dst_buf);
    int prevFormat = -1;
    if (((handle->format == HAL_PIXEL_FORMAT_RGBA_8888) || (handle->format == HAL_PIXEL_FORMAT_RGBX_8888) || (handle->format == HAL_PIXEL_FORMAT_RGB_565))
        && dst_handle->format == HAL_PIXEL_FORMAT_BGRA_8888) {
        prevFormat = dst_handle->format;
        dst_handle->format = handle->format;
    }
    int fence = mMPPs[gsc_idx]->mDstConfig.releaseFenceFd;

    if (layer.blending == HWC_BLENDING_PREMULT)
        deconBlending  = HWC_BLENDING_COVERAGE;
    else
        deconBlending = layer.blending;

    configureHandle(dst_handle, sourceCrop,
            layer.displayFrame, deconBlending, layer.planeAlpha,  fence,
            config[win_map]);

#ifdef USES_DRM_SETTING_BY_DECON
    if (getDrmMode(handle->flags) != NO_DRM)
        config[win_map].protection = 1;
    else
        config[win_map].protection = 0;
#endif

    if (prevFormat >= 0)
        dst_handle->format = prevFormat;
    return 0;
}

void ExynosOverlayDisplay::postGscOtf(hwc_layer_1_t &layer, struct s3c_fb_win_config *config, int win_map, int index)
{
#ifdef USES_DRM_SETTING_BY_DECON
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
#endif
    exynos5_hwc_post_data_t *pdata = &mPostData;
    int gsc_idx = pdata->gsc_map[index].idx;
    if (mHwc->mS3DMode == S3D_MODE_READY || mHwc->mS3DMode == S3D_MODE_RUNNING) {
        int S3DFormat = getS3DFormat(mHwc->mHdmiPreset);
        if (S3DFormat == S3D_SBS)
            mMPPs[gsc_idx]->mS3DMode = S3D_SBS;
        else if (S3DFormat == S3D_TB)
            mMPPs[gsc_idx]->mS3DMode = S3D_TB;
    } else {
        mMPPs[gsc_idx]->mS3DMode = S3D_NONE;
    }

    int err = mMPPs[gsc_idx]->processOTF(layer);

    if (err < 0) {
        ALOGE("failed to config_gsc_localout %u input for layer %u", gsc_idx, index);
        mOtfMode = OTF_TO_M2M;
        configureDummyOverlay(&layer, config[win_map]);
    } else {
        configureOtfWindow(layer.displayFrame, layer.blending, layer.planeAlpha,
                        layer.acquireFenceFd, HAL_PIXEL_FORMAT_RGBX_8888, config[win_map]);
    }
#ifdef USES_DRM_SETTING_BY_DECON
    if (getDrmMode(handle->flags) != NO_DRM)
        config[win_map].protection = 1;
    else
        config[win_map].protection = 0;
#endif
    return;
}

void ExynosOverlayDisplay::handleStaticLayers(hwc_display_contents_1_t *contents, struct s3c_fb_win_config_data &win_data, int tot_ovly_wins)
{
    int win_map = 0;
    if (mLastFbWindow >= NUM_HW_WINDOWS) {
        ALOGE("handleStaticLayers:: invalid mLastFbWindow(%d)", mLastFbWindow);
        return;
    }
    win_map = getDeconWinMap(mLastFbWindow, tot_ovly_wins);
    ALOGV("[USE] SKIP_STATIC_LAYER_COMP, mLastFbWindow(%d), win_map(%d)\n", mLastFbWindow, win_map);

    memcpy(&win_data.config[win_map],
            &mLastConfigData.config[win_map], sizeof(struct s3c_fb_win_config));
    win_data.config[win_map].fence_fd = -1;

    for (size_t i = mFirstFb; i <= mLastFb; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        if (layer.compositionType == HWC_OVERLAY) {
            ALOGV("[SKIP_STATIC_LAYER_COMP] layer.handle: 0x%p, layer.acquireFenceFd: %d\n", layer.handle, layer.acquireFenceFd);
            layer.releaseFenceFd = layer.acquireFenceFd;
        }
    }
}

int ExynosOverlayDisplay::getMPPForUHD(hwc_layer_1_t &layer)
{
    return FIMD_GSC_IDX;
}

void ExynosOverlayDisplay::cleanupGscs()
{
    if (mMPPs[FIMD_GSC_IDX]->isM2M()) {
        mMPPs[FIMD_GSC_IDX]->cleanupM2M();
        mMPPs[FIMD_GSC_IDX]->setMode(exynos5_gsc_map_t::GSC_NONE);
    } else if (mMPPs[FIMD_GSC_IDX]->isOTF()) {
        mMPPs[FIMD_GSC_IDX]->cleanupOTF();
    } else if (mMPPs[FIMD_GSC_SEC_IDX]->isM2M()) {
        mMPPs[FIMD_GSC_SEC_IDX]->cleanupM2M();
        mMPPs[FIMD_GSC_SEC_IDX]->setMode(exynos5_gsc_map_t::GSC_NONE);
    }
    for (int i = FIMD_GSC_SEC_IDX + 1; i < mNumMPPs; i++) {
        if (mMPPs[i]->isM2M()) {
            mMPPs[i]->cleanupM2M();
            mMPPs[i]->setMode(exynos5_gsc_map_t::GSC_NONE);
        }
    }
}

void ExynosOverlayDisplay::freeMPP()
{
    bool invalidate = false;
    for (int i = 0; i < mNumMPPs; i++) {
        if (mMPPs[i]->needsReqbufs()) {
            mMPPs[i]->free();
            invalidate = true;
        }
    }
    if (invalidate)
        mHwc->procs->invalidate(mHwc->procs);
}

void ExynosOverlayDisplay::refreshGscUsage(hwc_layer_1_t &layer)
{
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    int gsc_index = getMPPForUHD(layer);
    bool gsc_required = mMPPs[gsc_index]->isProcessingRequired(layer, handle->format);
    if (gsc_required) {
        if (mGscLayers >= 1)
            mGscLayers--;
        else
            ALOGW("refreshGscUsage: GSC layer count is invalid");
        if (!mGscLayers)
            mGscUsed = false;
    }
}

void ExynosOverlayDisplay::handleTotalBandwidthOverload(hwc_display_contents_1_t *contents)
{
    if (mHwc->totPixels >= FIMD_TOTAL_BW_LIMIT) {
        if (mFbNeeded) {
            for (int i = mFirstFb - 1; i >= 0; i--) {
                if (mForceOverlayLayerIndex == 0 && i == 0)
                    break;
                hwc_layer_1_t &layer = contents->hwLayers[i];
                refreshGscUsage(layer);
                layer.compositionType = HWC_FRAMEBUFFER;
                mLayerInfos[i]->compositionType = layer.compositionType;
                mLayerInfos[i]->mCheckOverlayFlag |= eInsufficientBandwidth;
                mFirstFb = i;
                mHwc->totPixels -= WIDTH(layer.displayFrame) * HEIGHT(layer.displayFrame);
                if (mHwc->totPixels < FIMD_TOTAL_BW_LIMIT)
                    break;
            }
            if (mHwc->totPixels >= FIMD_TOTAL_BW_LIMIT) {
                for (int i = mLastFb + 1; i < contents->numHwLayers - 1; i++) {
                    hwc_layer_1_t &layer = contents->hwLayers[i];
                    refreshGscUsage(layer);
                    layer.compositionType = HWC_FRAMEBUFFER;
                    mLayerInfos[i]->compositionType = layer.compositionType;
                    mLayerInfos[i]->mCheckOverlayFlag |= eInsufficientBandwidth;
                    mLastFb = i;
                    mHwc->totPixels -= WIDTH(layer.displayFrame) * HEIGHT(layer.displayFrame);
                    if (mHwc->totPixels < FIMD_TOTAL_BW_LIMIT)
                        break;
                }
            }
        } else {
            bool fbAdded = false;
            for (size_t i = 0; i < contents->numHwLayers; i++) {
                hwc_layer_1_t &layer = contents->hwLayers[i];
                if (layer.compositionType == HWC_OVERLAY &&
                        mForceOverlayLayerIndex != i) {
                    refreshGscUsage(layer);
                    layer.compositionType = HWC_FRAMEBUFFER;
                    mLastFb = max(mLastFb, i);
                    mLayerInfos[i]->compositionType = layer.compositionType;
                    mLayerInfos[i]->mCheckOverlayFlag |= eInsufficientBandwidth;

                    if (!fbAdded) {
                        fbAdded = true;
                        mHwc->totPixels += mXres * mYres;
                    }
                    if (mHwc->totPixels < FIMD_TOTAL_BW_LIMIT)
                        break;
                }
             }
            if (mPopupPlayYuvContents || mForceOverlayLayerIndex == 0)
                mFirstFb = 1;
            else
                mFirstFb = 0;
        }
        mFbNeeded = true;
    }
}

bool ExynosOverlayDisplay::switchOTF(bool enable)
{
    bool ret;
    if (mGscLayers > 1)
        return false;
    ret = mHwc->allowOTF != enable;
    mHwc->allowOTF = enable;
    return ret;
}

int ExynosOverlayDisplay::getRGBMPPIndex(int index)
{
    return index;
}

bool ExynosOverlayDisplay::multipleRGBScaling(int format)
{
    return isFormatRgb(format) &&
            mGscLayers >= 1;
}

void ExynosOverlayDisplay::checkTVBandwidth()
{
}

void ExynosOverlayDisplay::dump(android::String8& result)
{
    result.append(
            "   type   |  handle  |  color   | blend | pa |  format  |   position    |     size      | gsc \n"
            "----------+----------|----------+-------+----|----------+---------------+---------------------\n");
    //        8_______ | 8_______ | 8_______ | 5____ | 2_ | 8_______ | [5____,5____] | [5____,5____] | 3__ \n"

    int windowsUsed = 0;
    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        struct s3c_fb_win_config &config = mLastConfigData.config[i];
        if (config.state != config.S3C_FB_WIN_STATE_DISABLED)
            windowsUsed++;
    }

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        struct s3c_fb_win_config &config = mLastConfigData.config[i];
        size_t inverse = inverseWinMap(i, windowsUsed);
        if ((config.state == config.S3C_FB_WIN_STATE_DISABLED) &&
            (mLastGscMap[inverse].mode == exynos5_gsc_map_t::GSC_NONE)){
            result.appendFormat(" %8s | %8s | %8s | %5s | %2s | %8s | %13s | %13s",
                    "OVERLAY", "-", "-", "-", "-", "-", "-", "-");
        }
        else {
            if (config.state == config.S3C_FB_WIN_STATE_COLOR)
                result.appendFormat(" %8s | %8s | %8x | %5s | %2s | %8s", "COLOR",
                        "-", config.color, "-", "-", "-");
            else
                result.appendFormat(" %8s | %8x | %8s | %5x | %2x | %8s",
                        mLastFbWindow == inverse ? "FB" : "OVERLAY",
                        intptr_t(mLastHandles[i]),
                        "-", config.blending, config.plane_alpha, s3cFormat2str(config.format));

            result.appendFormat(" | [%5d,%5d] | [%5u,%5u]", config.x, config.y,
                    config.w, config.h);
        }
        if (mLastGscMap[inverse].mode == exynos5_gsc_map_t::GSC_NONE) {
            result.appendFormat(" | %3s", "-");
        } else {
            result.appendFormat(" | %3d",
                    AVAILABLE_GSC_UNITS[mLastGscMap[inverse].idx]);
            if (mLastGscMap[inverse].mode == exynos5_gsc_map_t::GSC_M2M)
                result.appendFormat(" | %10s","GSC_M2M");
            else
                result.appendFormat(" | %10s","GSC_LOCAL");
        }
        result.append("\n");
    }
}

unsigned int ExynosOverlayDisplay::getLayerRegion(hwc_layer_1_t &layer, hwc_rect &rect_area, uint32_t regionType) {
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
