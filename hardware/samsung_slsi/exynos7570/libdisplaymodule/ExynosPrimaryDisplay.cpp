#include "ExynosPrimaryDisplay.h"
#include "ExynosHWCModule.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"

#define DISPLAY_LOGD(msg, ...) ALOGD("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGV(msg, ...) ALOGV("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGI(msg, ...) ALOGI("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGW(msg, ...) ALOGW("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGE(msg, ...) ALOGE("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)

ExynosPrimaryDisplay::ExynosPrimaryDisplay(int numGSCs, struct exynos5_hwc_composer_device_1_t *pdev) :
    ExynosOverlayDisplay(numGSCs, pdev)
{
}

ExynosPrimaryDisplay::~ExynosPrimaryDisplay()
{
}

int ExynosPrimaryDisplay::getDeconWinMap(int overlayIndex, int totalOverlays)
{
	return 0;
}

void ExynosPrimaryDisplay::forceYuvLayersToFb(hwc_display_contents_1_t __unused *contents)
{
}

int ExynosPrimaryDisplay::getMPPForUHD(hwc_layer_1_t __unused &layer)
{
    return 0;
}

int ExynosPrimaryDisplay::getRGBMPPIndex(int index)
{
    return index;
}

bool ExynosPrimaryDisplay::isOverlaySupported(hwc_layer_1_t &layer, size_t index, bool useVPPOverlay,
        ExynosMPPModule** supportedInternalMPP, ExynosMPPModule** supportedExternalMPP)
{
    // Exynos755555oesn't have any VPP Overlays
    return ExynosDisplay::isOverlaySupported(layer, index, false, supportedInternalMPP, supportedExternalMPP);
}
bool ExynosPrimaryDisplay::isYuvDmaAvailable(int format, uint32_t dma)
{
    return (format == HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL &&
        dma == IDMA_G2);
}

void ExynosPrimaryDisplay::assignWindows(hwc_display_contents_1_t *contents)
{
    // call the ExynosDisplay default implementation of assignWindows()
    ExynosDisplay::assignWindows(contents);

    // allocate the max bandwidth non-rgb layer to IDMA_G2
    // consider only non-secure dma case - secure dma is already allocated to IDMA_G2

    bool videoLayerFound = 0, curDmaYuvFound = 0, hasDRMlayer = 0;
    uint32_t videoLayerIndex = 0, curDmaYuvIndex = 0;
    uint32_t mTemp = 0, bandWidth = 0;
    int fbLayerIndex = -1;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (!layer.handle)
            continue;

        private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET)
            fbLayerIndex = i;

        // If there is DRM layer available, it has to go to IDMA_G2 channel.
        if (layer.compositionType == HWC_OVERLAY &&
                getDrmMode(handle->flags) == SECURE_DRM &&
                !(layer.flags & HWC_SKIP_RENDERING)) {
            hasDRMlayer = 1;
            videoLayerFound = 1;
            videoLayerIndex = i;
        }

        // If no DRM layer, find non-rgb overlay layer which can be supported by IDMA_G2.
        if ((layer.compositionType == HWC_OVERLAY) &&
                    isYuvDmaAvailable(handle->format, IDMA_G2) &&
                    !(layer.flags & HWC_SKIP_RENDERING) &&
                    !hasDRMlayer) {

            mTemp = (layer.sourceCropf.right - layer.sourceCropf.left) *
                (layer.sourceCropf.bottom - layer.sourceCropf.top);
            if (mTemp > bandWidth) {
                bandWidth = mTemp;
                videoLayerFound = 1;
                videoLayerIndex = i;
            }
        }

        if (((layer.compositionType == HWC_OVERLAY) ||
            (layer.compositionType == HWC_FRAMEBUFFER_TARGET)) &&
            (!curDmaYuvFound) &&
            (mLayerInfos[i]->mDmaType == IDMA_G2)) {
            curDmaYuvFound = 1;
            curDmaYuvIndex = i;
        }
    }

    if (curDmaYuvFound && videoLayerFound) {
        mTemp = mLayerInfos[videoLayerIndex]->mDmaType;
        mLayerInfos[videoLayerIndex]->mDmaType = mLayerInfos[curDmaYuvIndex]->mDmaType;
        mLayerInfos[curDmaYuvIndex]->mDmaType = mTemp;
    }

    if (videoLayerFound && !curDmaYuvFound) {
        if (prevfbTargetIdma == IDMA_G2) {
            prevfbTargetIdma = (enum decon_idma_type) mLayerInfos[videoLayerIndex]->mDmaType;
            mLayerInfos[videoLayerIndex]->mDmaType = IDMA_G2;
            return;
        }
        mLayerInfos[videoLayerIndex]->mDmaType = IDMA_G2;
    }

    if (mFbNeeded && mFbWindow < NUM_HW_WINDOWS && fbLayerIndex >= 0)
       prevfbTargetIdma = (enum decon_idma_type) mLayerInfos[fbLayerIndex]->mDmaType;
}

int ExynosPrimaryDisplay::postMPPM2M(hwc_layer_1_t &layer, struct decon_win_config *config, int win_map, int index)
{
    int dst_format = mExternalMPPDstFormat;
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    ExynosMPPModule *exynosMPP = mLayerInfos[index]->mExternalMPP;

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

    /* OFF_Screen to ON_Screen changes */
    if (getDrmMode(handle->flags) == SECURE_DRM)
        recalculateDisplayFrame(layer, mXres, mYres);

    if (mType != EXYNOS_VIRTUAL_DISPLAY &&
        (isFormatRgb(handle->format) ||
         (isYuvDmaAvailable(handle->format, mLayerInfos[index]->mDmaType) &&
          WIDTH(layer.displayFrame) % getIDMAWidthAlign(handle->format) == 0 &&
          HEIGHT(layer.displayFrame) % getIDMAHeightAlign(handle->format) == 0)))
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

    /* ExtMPP out is the input of Decon
     * and Trsform was processed by ExtMPP
     */
    layer.sourceCropf = sourceCrop;
    layer.transform = 0;
    configureHandle(dst_handle, index, layer,  fence, config[win_map]);

    /* Restore sourceCropf and transform */
    layer.sourceCropf = originalCrop;
    layer.transform = originalTransform;
    return 0;
}

void ExynosPrimaryDisplay::handleStaticLayers(hwc_display_contents_1_t *contents,
    struct decon_win_config_data &win_data, int __unused tot_ovly_wins)
{
    unsigned int bitmap = 0;
    bool incorrect_mapping = false;

    ExynosDisplay::handleStaticLayers(contents, win_data, tot_ovly_wins);

    if (mLastFbWindow >= NUM_HW_WINDOWS) {
        DISPLAY_LOGE("handleStaticLayers:: invalid mLastFbWindow(%d)", mLastFbWindow);
        return;
    }

    win_data.config[mLastFbWindow].idma_type = prevfbTargetIdma;

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        if (win_data.config[i].state == win_data.config[i].DECON_WIN_STATE_BUFFER) {
            if (bitmap & (1 << win_data.config[i].idma_type)) {
                ALOGE("handleStaticLayers: Channel-%d is mapped to multiple windows\n", win_data.config[i].idma_type);
                incorrect_mapping = true;
            }

            bitmap |= 1 << win_data.config[i].idma_type;
        }
    }

    if (incorrect_mapping) {
        android::String8 result;
        result.clear();
        dump(result);
        ALOGE("%s", result.string());
        result.clear();
        dumpLayerInfo(result);
        ALOGE("%s", result.string());
        ALOGE("Display Config:");
        dump_win_config(&win_data.config[0]);
    }
}

void ExynosPrimaryDisplay::dump_win_config(struct decon_win_config *config)
{
    int win;
    struct decon_win_config *cfg;
    const char *state[] = { "DSBL", "COLR", "BUFF", "UPDT" };
    const char *blending[] = { "NOBL", "PMUL", "COVR" };
    const char *idma[] = { "G0", "G1", "V0", "V1", "VR0", "VR1", "G2" };

    for (win = 0; win < MAX_DECON_WIN; win++) {
        cfg = &config[win];

        if (cfg->state == cfg->DECON_WIN_STATE_DISABLED)
        continue;

        if (cfg->state == cfg->DECON_WIN_STATE_COLOR) {
            ALOGE("Win[%d]: %s, C(%d), Dst(%d,%d,%d,%d) "
                "P(%d)\n",
                win, state[cfg->state], cfg->color,
                cfg->dst.x, cfg->dst.y,
                cfg->dst.x + cfg->dst.w,
                cfg->dst.y + cfg->dst.h,
                cfg->protection);
        } else {
            ALOGE("Win[%d]: %s,(%d,%d,%d), F(%d) P(%d)"
                " A(%d), %s, %s, fmt(%d) Src(%d,%d,%d,%d,f_w=%d,f_h=%d) ->"
                " Dst(%d,%d,%d,%d,f_w=%d,f_h=%d) T(%d,%d,%d,%d) B(%d,%d,%d,%d)\n",
                win, state[cfg->state], cfg->fd_idma[0],
                cfg->fd_idma[1], cfg->fd_idma[2],
                cfg->fence_fd, cfg->protection,
                cfg->plane_alpha, blending[cfg->blending],
                idma[cfg->idma_type], cfg->format,
                cfg->src.x, cfg->src.y, cfg->src.x + cfg->src.w, cfg->src.y + cfg->src.h,
                cfg->src.f_w, cfg->src.f_h,
                cfg->dst.x, cfg->dst.y, cfg->dst.x + cfg->dst.w, cfg->dst.y + cfg->dst.h,
                cfg->dst.f_w, cfg->dst.f_h,
                cfg->transparent_area.x, cfg->transparent_area.y,
                cfg->transparent_area.w, cfg->transparent_area.h,
                cfg->covered_opaque_area.x, cfg->covered_opaque_area.y,
                cfg->covered_opaque_area.w, cfg->covered_opaque_area.h);
        }
    }
}

int ExynosPrimaryDisplay::handleWindowUpdate(hwc_display_contents_1_t __unused *contents,
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
    int alignAdjustment = 0;
    int intersectionWidth = 0;

    char value[PROPERTY_VALUE_MAX];
    property_get("debug.hwc.winupdate", value, NULL);

    if (!(!strcmp(value, "1") || !strcmp(value, "true")))
        return -eWindowUpdateDisabled;

    if (DECON_WIN_UPDATE_IDX < 0)
        return -eWindowUpdateInvalidIndex;
    winUpdateInfoIdx = DECON_WIN_UPDATE_IDX;

    if (contents->flags & HWC_GEOMETRY_CHANGED)
        return -eWindowUpdateGeometryChanged;

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        if (contents->hwLayers[i].compositionType == HWC_FRAMEBUFFER)
            continue;

        if (!mFbNeeded && contents->hwLayers[i].compositionType == HWC_FRAMEBUFFER_TARGET)
            continue;
        int32_t windowIndex = mLayerInfos[i]->mWindowIndex;
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
                    getLayerRegion(layer, damageRect, eDamageRegion);

                    if (handle && !isScaled(layer) && !isRotated(layer)
                            && !(!(damageRect.left) && !(damageRect.top) && !(damageRect.right) && !(damageRect.bottom))) {
                        HLOGD("[WIN_UPDATE][surfaceDamage]  layer w(%4d) h(%4d),  dirty (%4d, %4d) - (%4d, %4d)",
                                handle->width, handle->height, damageRect.left, damageRect.top, damageRect.right, damageRect.bottom);

                        currentRect.left   = config[windowIndex].dst.x - (int32_t)layer.sourceCropf.left + damageRect.left;
                        currentRect.right  = config[windowIndex].dst.x - (int32_t)layer.sourceCropf.left + damageRect.right;
                        currentRect.top    = config[windowIndex].dst.y - (int32_t)layer.sourceCropf.top  + damageRect.top;
                        currentRect.bottom = config[windowIndex].dst.y - (int32_t)layer.sourceCropf.top  + damageRect.bottom;

                    }
                }

                if ((currentRect.left > currentRect.right) || (currentRect.top > currentRect.bottom)) {
                    HLOGD("[WIN_UPDATE] window(%d) layer(%d) invalid region (%4d, %4d) - (%4d, %4d)",
                        i, layerIdx, currentRect.left, currentRect.top, currentRect.right, currentRect.bottom);
                    return -eWindowUpdateInvalidRegion;
                }
                HLOGD("[WIN_UPDATE] Updated Window(%d) Layer(%d)  (%4d, %4d) - (%4d, %4d)",
                    windowIndex, i, currentRect.left, currentRect.top, currentRect.right, currentRect.bottom);
                updateRect = expand(updateRect, currentRect);
            }
        }
    }
    if (updatedWinCnt == 0)
        return -eWindowUpdateNotUpdated;

    updateRect.left  = ALIGN_DOWN(updateRect.left, WINUPDATE_X_ALIGNMENT);
    updateRect.right = updateRect.left + ALIGN_UP(WIDTH(updateRect), WINUPDATE_W_ALIGNMENT);

    if (HEIGHT(updateRect) < WINUPDATE_MIN_HEIGHT) {
        if (updateRect.top + WINUPDATE_MIN_HEIGHT <= mYres)
            updateRect.bottom = updateRect.top + WINUPDATE_MIN_HEIGHT;
        else
            updateRect.top = updateRect.bottom - WINUPDATE_MIN_HEIGHT;
    }

    if ((100 * (WIDTH(updateRect) * HEIGHT(updateRect)) / (this->mXres * this->mYres)) > WINUPDATE_THRESHOLD)
        return -eWindowUpdateOverThreshold;

    alignAdjustment = max(WINUPDATE_X_ALIGNMENT, WINUPDATE_W_ALIGNMENT);

    while (1) {
        burstLengthCheckDone = true;

        for (size_t i = 0; i < contents->numHwLayers; i++) {
            if (contents->hwLayers[i].compositionType == HWC_FRAMEBUFFER)
                continue;
            if (!mFbNeeded && contents->hwLayers[i].compositionType == HWC_FRAMEBUFFER_TARGET)
                continue;
            int32_t windowIndex = mLayerInfos[i]->mWindowIndex;
            if (config[windowIndex].state != config[windowIndex].DECON_WIN_STATE_DISABLED) {
                enum decon_pixel_format fmt = config[windowIndex].format;

                if (fmt == DECON_PIXEL_FORMAT_RGBA_5551 || fmt == DECON_PIXEL_FORMAT_RGB_565 )
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

                HLOGV("[WIN_UPDATE] win[%d] left(%d) right(%d) intersection(%d)", windowIndex, currentRect.left, currentRect.right, intersectionWidth);

                if (intersectionWidth != 0 && (size_t)((intersectionWidth * bitsPerPixel) / 8) < BURSTLEN_BYTES) {
                    HLOGV("[WIN_UPDATE] win[%d] insufficient burst length ((%d)*(%d)/8) < %d", windowIndex, intersectionWidth, bitsPerPixel, BURSTLEN_BYTES);
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
            DISPLAY_LOGD("[WIN_UPDATE] Error during update width adjustment");
            return -eWindowUpdateAdjustmentFail;
        }
    }

    config[winUpdateInfoIdx].state = config[winUpdateInfoIdx].DECON_WIN_STATE_UPDATE;
    config[winUpdateInfoIdx].dst.x = updateRect.left;
    config[winUpdateInfoIdx].dst.y = updateRect.top;
    config[winUpdateInfoIdx].dst.w = ALIGN_UP(WIDTH(updateRect), WINUPDATE_W_ALIGNMENT);
    config[winUpdateInfoIdx].dst.h = HEIGHT(updateRect);

    HLOGD("[WIN_UPDATE] UpdateRegion cfg  (%4d, %4d) w(%4d) h(%4d) updatedWindowCnt(%d)",
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

int ExynosPrimaryDisplay::getIDMAWidthAlign(int format)
{
    if (isFormatRgb(format))
        return 1;
    else
        return 2;
}

int ExynosPrimaryDisplay::getIDMAHeightAlign(int format)
{
    if (isFormatRgb(format))
        return 1;
    else
        return 2;
}
