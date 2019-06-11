#include "ExynosVirtualDisplayModule.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"

#ifdef EVD_DBUG
#define DISPLAY_LOGD(msg, ...) ALOGD("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGV(msg, ...) ALOGV("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGI(msg, ...) ALOGI("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGW(msg, ...) ALOGW("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#define DISPLAY_LOGE(msg, ...) ALOGE("[%s] " msg, mDisplayName.string(), ##__VA_ARGS__)
#else
#define DISPLAY_LOGD(msg, ...)
#define DISPLAY_LOGV(msg, ...)
#define DISPLAY_LOGI(msg, ...)
#define DISPLAY_LOGW(msg, ...)
#define DISPLAY_LOGE(msg, ...)
#endif

ExynosVirtualDisplayModule::ExynosVirtualDisplayModule(struct exynos5_hwc_composer_device_1_t *pdev)
    : ExynosVirtualDisplay(pdev)
{
    mGLESFormat = HAL_PIXEL_FORMAT_RGBA_8888;
    mDisplayFd  = 0;
}

void ExynosVirtualDisplayModule::determineYuvOverlay(hwc_display_contents_1_t *contents)
{
    DISPLAY_LOGD("EVD::determineYuvOverlay");

    mForceOverlayLayerIndex = -1;
    mHasDrmSurface = false;
    mYuvLayers = 0;
    mForceFb = false;
    mIsSecureDRM = false;
    mIsNormalDRM = false;

    if (contents->outbuf) {
        private_handle_t *h = private_handle_t::dynamicCast(contents->outbuf);
        mExternalMPPDstFormat = h->format;
    } else {
        DISPLAY_LOGD("BufferQueue is abandoned.");
    }

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        ExynosMPPModule* supportedExternalMPP = NULL;
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.handle) {
            private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);

            /* check yuv drm surface */
            if (!mForceFb && !isFormatRgb(handle->format) && (getDrmMode(handle->flags) != NO_DRM)) {
                if (isOverlaySupported(contents->hwLayers[i], i, false, NULL, &supportedExternalMPP))
                {
                        this->mYuvLayers++;

                        /* If not DRM layer is addressed before */
                        if (mHasDrmSurface == false) {
                            /* Assign MPP */
                            if (supportedExternalMPP != NULL)
                                supportedExternalMPP->mState = MPP_STATE_ASSIGNED;

                            mForceOverlayLayerIndex = i;
                            layer.compositionType = HWC_OVERLAY;
                            mLayerInfos[i]->mExternalMPP = supportedExternalMPP;
                            mLayerInfos[i]->mInternalMPP = NULL;
                            mLayerInfos[i]->compositionType = layer.compositionType;
                            mLayerInfos[i]->mWindowIndex = i;
                            supportedExternalMPP->setDisplay(this);

                            if (getDrmMode(handle->flags) == SECURE_DRM) {
                                mIsSecureDRM = true;
                                mOverlayLayer = &layer;
                                DISPLAY_LOGD("determineYuvOverlay: secure drm layer, %d", i);
                            }

                            /* todo: selecting overlay composition but need to check for EGL */
                            if (getDrmMode(handle->flags) == NORMAL_DRM) {
                                mIsNormalDRM = true;
                                mOverlayLayer = &layer;
                                calcDisplayRect(layer);
                                DISPLAY_LOGD("determineYuvOverlay: normal drm layer, %d", i);
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

void ExynosVirtualDisplayModule::determineSupportedOverlays(hwc_display_contents_1_t *contents)
{
    DISPLAY_LOGD("determineSupportedOverlays");

    int ret = 0;
    mIsRotationState = false;
    mCompositionType = COMPOSITION_GLES;
    mOverlayLayer = NULL;
    mFBTargetLayer = NULL;
    memset(mFBLayer, 0x0, sizeof(mFBLayer));
    mNumFB = 0;
    mFbNeeded = false;

    if (contents->outbuf) {
        private_handle_t *h = private_handle_t::dynamicCast(contents->outbuf);
        DISPLAY_LOGD("DSO: Outbuf layer f=%x, fds[%d](%d, %d, %d), sz(%d) wxh(%dx%d)m stride(%d) fence(%d) P(%d)",
                    h->format, h->sNumFds, h->fd, h->fd1, h->fd2, h->size, h->width,
                    h->height, h->stride, contents->outbufAcquireFenceFd, getDrmMode(h->flags));
        mExternalMPPDstFormat = h->format;
    }

    /* determine composition type */
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.handle) {
            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
            DISPLAY_LOGD("DSO: layer %d type=%d, f=%x, w=%d, h=%d, s=%d, vs=%d, "
                  "{%.1f, %.1f, %.1f, %.1f}, {%d, %d, %d, %d}"
                  "fl=%08x, hdl=%p, trf=%02x, bl=%04x, Af=%d, Rf=%d, P=%d",
                    i, layer.compositionType, h->format, h->width, h->height, h->stride,
                    h->vstride, layer.sourceCropf.left, layer.sourceCropf.top,
                    layer.sourceCropf.right, layer.sourceCropf.bottom,
                    layer.displayFrame.left, layer.displayFrame.top,
                    layer.displayFrame.right, layer.displayFrame.bottom,
                    layer.flags, layer.handle, layer.transform, layer.blending,
                    layer.acquireFenceFd, layer.releaseFenceFd, getDrmMode(layer.flags));
        }

        if (!mFbNeeded) {
            mFirstFb = i;
            mFbNeeded = true;
        }
        mLastFb = i;
    }
}

bool ExynosVirtualDisplayModule::isOverlaySupported(hwc_layer_1_t &layer, size_t index,
        bool useVPPOverlay, ExynosMPPModule **supportedInternalMPP,
        ExynosMPPModule **supportedExternalMPP)
{
    int ret = 0;
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    ExynosMPPModule *externalMPP = mExternalMPPs[WFD_EXT_MPP_IDX];
    unsigned int bpp = formatToBpp(handle->format);
    unsigned int left = max(layer.displayFrame.left, 0);
    unsigned int right = layer.displayFrame.right;
    unsigned int visible_width = 0;

    DISPLAY_LOGD(" isOverlaySupported:: index(%d)", index);

    float width = 0;
    float height = 0;

    if (layer.flags & HWC_SKIP_LAYER) {
        mLayerInfos[index]->mCheckOverlayFlag |= eSkipLayer;
        DISPLAY_LOGD("\tlayer %u: skipping", index);
        return false;
    }

    if (!handle) {
        DISPLAY_LOGD("\tlayer %u: handle is NULL", index);
        mLayerInfos[index]->mCheckOverlayFlag |= eInvalidHandle;
        return false;
    }

    width = layer.sourceCropf.right - layer.sourceCropf.left;
    height = layer.sourceCropf.bottom - layer.sourceCropf.top;

    if ((bpp == 16) &&
            ((layer.displayFrame.left % 2 != 0) || (layer.displayFrame.right % 2 != 0))) {
        DISPLAY_LOGD("\tlayer %u: eNotAlignedDstPosition", index);
        mLayerInfos[index]->mCheckOverlayFlag |= eNotAlignedDstPosition;
        return false;
    }

    visible_width = (right - left) * bpp / 8;
    if (visible_width < BURSTLEN_BYTES) {
        DISPLAY_LOGD("\tlayer %u: visible area is too narrow", index);
        mLayerInfos[index]->mCheckOverlayFlag |= eUnsupportedDstWidth;
        return false;
    }

    if (!isFormatRgb(handle->format)) {
        ret = externalMPP->isFormatSupportedByMPP(mExternalMPPDstFormat);
        if (ret > 0) {
            *supportedExternalMPP = externalMPP;
            return true;
        } else {
            DISPLAY_LOGD("\tlayer %u: %d ProcessingNotSupported", index, -ret);
            mLayerInfos[index]->mCheckMPPFlag |= -ret;
        }
    }

    /* Can't find valid MPP */
    DISPLAY_LOGD("\tlayer %u: can't find valid MPP", index);
    mLayerInfos[index]->mCheckOverlayFlag |= eInsufficientMPP;
    return false;
}

int ExynosVirtualDisplayModule::prepare(hwc_display_contents_1_t *contents)
{
    int ret = 0;
    mIsRotationState = false;
    mCompositionType = COMPOSITION_GLES;
    mOverlayLayer = NULL;
    mFBTargetLayer = NULL;
    memset(mFBLayer, 0x0, sizeof(mFBLayer));
    mNumFB = 0;

    if (contents->outbuf) {
        private_handle_t *h = private_handle_t::dynamicCast(contents->outbuf);
        DISPLAY_LOGD("PREP: Outbuf layer f=%x, fds[%d](%d, %d, %d), sz(%d) wxh(%dx%d)m stride(%d) Af(%d) P(%d)",
                    h->format, h->sNumFds, h->fd, h->fd1, h->fd2, h->size, h->width,
                    h->height, h->stride, contents->outbufAcquireFenceFd, getDrmMode(h->flags));
        mExternalMPPDstFormat = h->format;
    }

    /* determine composition type */
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.compositionType == HWC_FRAMEBUFFER) {
            if (!layer.handle)
                mNumFB++;
            goto cont;
        }

        if (layer.compositionType == HWC_OVERLAY) {
            if (!layer.handle)
                goto cont;

            if (layer.flags & HWC_SKIP_RENDERING) {
                layer.releaseFenceFd = layer.acquireFenceFd;
                goto cont;
            }

            mOverlayLayer = &layer;
            goto cont;
        }

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            if (!layer.handle)
                goto cont;

            mFBTargetLayer = &layer;
            goto cont;
        }

cont:
        if (layer.handle) {
            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
            DISPLAY_LOGD("PREP: layer %d type=%d, f=%x, w=%d, h=%d, s=%d, vs=%d, "
                  "{%.1f, %.1f, %.1f, %.1f}, {%d, %d, %d, %d}"
                  "fl=%08x, hdl=%p, trf=%02x, bl=%04x, Af=%d, Rf=%d, P=%d",
                    i, layer.compositionType, h->format, h->width, h->height, h->stride,
                    h->vstride, layer.sourceCropf.left, layer.sourceCropf.top,
                    layer.sourceCropf.right, layer.sourceCropf.bottom,
                    layer.displayFrame.left, layer.displayFrame.top,
                    layer.displayFrame.right, layer.displayFrame.bottom,
                    layer.flags, layer.handle, layer.transform, layer.blending,
                    layer.acquireFenceFd, layer.releaseFenceFd, getDrmMode(layer.flags));
        }
    }

    if (mOverlayLayer && (mNumFB > 0))
        mCompositionType = COMPOSITION_MIXED;
    else if (mOverlayLayer || mIsRotationState)
        mCompositionType = COMPOSITION_HWC;

    DISPLAY_LOGD("prepare cType 0x%x, PrevCType 0x%x, ovl 0x%p, mNumFB %d",
        mCompositionType, mPrevCompositionType, mOverlayLayer, mNumFB);

    setSinkBufferUsage();

    if (mCompositionType != mPrevCompositionType) {
        DISPLAY_LOGD("prepare mCompositionType 0x%x, mPrevCompositionType are not same\n");
            ExynosMPPModule* externalMPP = mExternalMPPs[WFD_EXT_MPP_IDX];

            externalMPP->mDstBuffers[externalMPP->mCurrentBuf] = NULL;
            externalMPP->mDstBufFence[externalMPP->mCurrentBuf] = -1;
            externalMPP->cleanupM2M();
            externalMPP->setDisplay(this);
    }
    return 0;
}

int ExynosVirtualDisplayModule::set(hwc_display_contents_1_t *contents)
{
    DISPLAY_LOGD("set %u layers for virtual, mCompositionType %d, contents->outbuf %p",
        contents->numHwLayers, mCompositionType, contents->outbuf);
    mOverlayLayer = NULL;
    mFBTargetLayer = NULL;
    memset(mFBLayer, 0x0, sizeof(mFBLayer));
    int IsNormalDRMWithSkipLayer = false;
    mNumFB = 0;

    if (contents->outbuf) {
        private_handle_t *h = private_handle_t::dynamicCast(contents->outbuf);
        DISPLAY_LOGD("prepare: Outbuf layer f=%x, fds[%d](%d, %d, %d), sz(%d) wxh(%dx%d)m stride(%d) fence(%d) P(%d)",
                    h->format, h->sNumFds, h->fd, h->fd1, h->fd2, h->size, h->width,
                    h->height, h->stride, contents->outbufAcquireFenceFd, getDrmMode(h->flags));
        mExternalMPPDstFormat = h->format;
    }

    /* Find normal drm layer with HWC_SKIP_LAYER, HDCP disabled and normal DRM */
     for (size_t i = 0; i < contents->numHwLayers; i++) {
         hwc_layer_1_t &layer = contents->hwLayers[i];
         if (layer.flags & HWC_SKIP_LAYER) {
             DISPLAY_LOGE("set skipping layer %d", i);
             if (layer.handle) {
                 private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
                 if (getDrmMode(h->flags) == NORMAL_DRM && mIsWFDState) {
                     DISPLAY_LOGE("set skipped normal drm layer %d", i);
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
                goto cont;
            mNumFB++;
        }

        if (layer.compositionType == HWC_OVERLAY) {
            if (!layer.handle)
                goto cont;

            if (layer.flags & HWC_SKIP_RENDERING) {
                goto cont;
            }

            mOverlayLayer = &layer;
            goto cont;
        }

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            if (!layer.handle)
                goto cont;

            mFBTargetLayer = &layer;
            goto cont;
        }

cont:
        if (layer.handle) {
            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
            DISPLAY_LOGD("SET: layer %d type=%d, f=%x, w=%d, h=%d, s=%d, vs=%d, "
                  "{%.1f, %.1f, %.1f, %.1f}, {%d, %d, %d, %d}"
                  "fl=%08x, hdl=%p, trf=%02x, bl=%04x, Af=%d, Rf=%d, P=%d",
                    i, layer.compositionType, h->format, h->width, h->height, h->stride,
                    h->vstride, layer.sourceCropf.left, layer.sourceCropf.top,
                    layer.sourceCropf.right, layer.sourceCropf.bottom,
                    layer.displayFrame.left, layer.displayFrame.top,
                    layer.displayFrame.right, layer.displayFrame.bottom,
                    layer.flags, layer.handle, layer.transform, layer.blending,
                    layer.acquireFenceFd, layer.releaseFenceFd, getDrmMode(layer.flags));
        }
    }

    if (mFBTargetLayer && IsNormalDRMWithSkipLayer) {
        for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

            if (layer.acquireFenceFd >= 0 && layer.compositionType != HWC_FRAMEBUFFER_TARGET) {
                close(layer.acquireFenceFd);
                layer.acquireFenceFd = -1;
            }
        }

        if (mFBTargetLayer && mFBTargetLayer->acquireFenceFd >= 0)
            contents->retireFenceFd = mFBTargetLayer->acquireFenceFd;

        if (contents->outbufAcquireFenceFd >= 0) {
            close(contents->outbufAcquireFenceFd);
            contents->outbufAcquireFenceFd = -1;
        }
    } else if (mFBTargetLayer && mCompositionType == COMPOSITION_GLES) {
        processGles(contents);
    } else if (mFBTargetLayer && mOverlayLayer && mCompositionType == COMPOSITION_MIXED) {
        processMixed(contents);
    } else if (mOverlayLayer) {
        processHwc(contents);
    } else {
        for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

            if (layer.acquireFenceFd >= 0 && layer.compositionType != HWC_FRAMEBUFFER_TARGET) {
                close(layer.acquireFenceFd);
                layer.acquireFenceFd = -1;
            }
        }

        if (mFBTargetLayer && mFBTargetLayer->acquireFenceFd >= 0)
            contents->retireFenceFd = mFBTargetLayer->acquireFenceFd;

        if (contents->outbufAcquireFenceFd >= 0) {
            close(contents->outbufAcquireFenceFd);
            contents->outbufAcquireFenceFd = -1;
        }
    }

    mPrevCompositionType = mCompositionType;

    return 0;
}

void ExynosVirtualDisplayModule::processGles(hwc_display_contents_1_t *contents)
{
    DISPLAY_LOGD("processGles, FBTgt->acqFence %d, FBTgt->relFence %d, outbufAcqFence %d",
        mFBTargetLayer->acquireFenceFd, mFBTargetLayer->releaseFenceFd,
        contents->outbufAcquireFenceFd);

    if (mFBTargetLayer != NULL && mFBTargetLayer->acquireFenceFd >= 0)
        contents->retireFenceFd = mFBTargetLayer->acquireFenceFd;

    if (contents->outbufAcquireFenceFd >= 0) {
        close(contents->outbufAcquireFenceFd);
        contents->outbufAcquireFenceFd = -1;
    }
}

void ExynosVirtualDisplayModule::processHwc(hwc_display_contents_1_t *contents)
{
    DISPLAY_LOGD("processHwc, outbufAcqFence %d", contents->outbufAcquireFenceFd);

    int fence = postFrame(contents);

    manageFences(contents, fence);
}

void ExynosVirtualDisplayModule::processMixed(hwc_display_contents_1_t *contents)
{
    DISPLAY_LOGD("processMixed, FBTgt->acqFence %d, FBTgt->relFence %d, outbufAcqFence %d",
        mFBTargetLayer->acquireFenceFd, mFBTargetLayer->releaseFenceFd,
        contents->outbufAcquireFenceFd);

    int fence = postFrame(contents);

    manageFences(contents, fence);
}

int ExynosVirtualDisplayModule::postFrame(hwc_display_contents_1_t *contents)
{
    int ret = -1;
    int win_map = 0;
    int tot_ovly_wins = 0;
    private_handle_t *handle_op, *h;

    memset(mLastHandles, 0, sizeof(mLastHandles));
    memset(mLastMPPMap, 0, sizeof(mLastMPPMap));

    if (contents->outbuf) {
        handle_op = private_handle_t::dynamicCast(contents->outbuf);
    }

    for (size_t i = 0; i < NUM_HW_WINDOWS; i++) {
        mLastMPPMap[i].internal_mpp.type = -1;
        mLastMPPMap[i].external_mpp.type = -1;
    }

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        size_t window_index = mLayerInfos[i]->mWindowIndex + 1;

        if (layer.flags & HWC_SKIP_RENDERING) {
            layer.releaseFenceFd = layer.acquireFenceFd;
            continue;
        }

        if (layer.compositionType == HWC_OVERLAY) {
            mLastHandles[window_index] = layer.handle;

            if (mLayerInfos[i]->mExternalMPP != NULL) {
                mLastMPPMap[window_index].external_mpp.type = mLayerInfos[i]->mExternalMPP->mType;
                mLastMPPMap[window_index].external_mpp.index = mLayerInfos[i]->mExternalMPP->mIndex;

                ret = postToMPP(layer, mFBTargetLayer, i, contents);
                if (ret < 0) {
                    DISPLAY_LOGE("postToMPP failed in extended/drm mode.");
                }
                mLayerInfos[i]->mExternalMPP->mCurrentBuf =
                    (mLayerInfos[i]->mExternalMPP->mCurrentBuf + 1)%
                    mLayerInfos[i]->mExternalMPP->mNumAvailableDstBuffers;
            }
        }
    }

    return ret;
}

int ExynosVirtualDisplayModule::postToMPP(hwc_layer_1_t &layer, hwc_layer_1_t *layerB,
        int index, hwc_display_contents_1_t *contents)
{
    int dst_format = mExternalMPPDstFormat;
    private_handle_t *handle = private_handle_t::dynamicCast(layer.handle);
    ExynosMPPModule *exynosMPP = mLayerInfos[index]->mExternalMPP;
    ExynosMPPModule *exynosInternalMPP = mLayerInfos[index]->mInternalMPP;
    hwc_layer_1_t extMPPOutLayer;
    hwc_frect_t sourceCrop = { 0, 0,  (float)WIDTH(layer.displayFrame),
            (float)HEIGHT(layer.displayFrame) };
    int originalTransform = layer.transform;
    hwc_rect_t originalDisplayFrame = layer.displayFrame;
    int err;

    DISPLAY_LOGD("postToMPP is called with mFBTargetLayer(0x%x)", mFBTargetLayer);

    if (exynosMPP == NULL) {
        DISPLAY_LOGE("postToMPP is called but externMPP is NULL");
        return -1;
    }

    exynosMPP->mDstBuffers[exynosMPP->mCurrentBuf] = contents->outbuf;
    exynosMPP->mDstBufFence[exynosMPP->mCurrentBuf] = contents->outbufAcquireFenceFd;

    if (layerB) {
        if (is2StepBlendingRequired(layer, contents->outbuf)) {
            private_handle_t *handleB = private_handle_t::dynamicCast(layerB->handle);
            hwc_frect_t sourceCropB = { 0, 0,
                    (float)WIDTH(layerB->displayFrame), (float)HEIGHT(layerB->displayFrame) };

            DISPLAY_LOGD("Performing 2-Step blending operation");

            calcDisplayRect(*layerB);

            err = exynosMPP->processM2M(*layerB, dst_format, &sourceCropB, false);
            if (err < 0) {
                DISPLAY_LOGE("2 step - failed to configure MPP, %d", err);
                return -1;
            }

            exynosMPP->mDstBufFence[exynosMPP->mCurrentBuf] =
                            exynosMPP->mDstConfig.releaseFenceFd;
            exynosMPP->mDstConfig.releaseFenceFd = -1;
        }else {
            DISPLAY_LOGD("Performing 1-Step blending operation");
            int tempFence = sync_merge("scaler_blend", layerB->acquireFenceFd,
                    layer.acquireFenceFd);

            close(layerB->acquireFenceFd);
            layerB->acquireFenceFd = -1;
            close(layer.acquireFenceFd);
            layer.acquireFenceFd = tempFence;
        }

        calcDisplayRect(layer);

        err = exynosMPP->processM2MWithB(layer, *layerB, dst_format, &sourceCrop);
        if (err < 0) {
            DISPLAY_LOGE("failed to configure MPP for blending, %d", err);
            return -1;
        }
    } else {
        DISPLAY_LOGD("Performing Only-Scaling operation");

        err = exynosMPP->processM2M(layer, dst_format, &sourceCrop, false);
        if (err < 0) {
            DISPLAY_LOGE("failed to configure MPP for scaling, %d", err);
            return -1;
        }
    }

    /* Restore displayFrame*/
    layer.displayFrame = originalDisplayFrame;

    return exynosMPP->mDstConfig.releaseFenceFd;
}

bool ExynosVirtualDisplayModule::is2StepBlendingRequired(hwc_layer_1_t &layer,
        buffer_handle_t &outbuf)
{
    private_handle_t *outhandle = private_handle_t::dynamicCast(outbuf);

    if (outhandle->width != WIDTH(layer.displayFrame) ||
        outhandle->height != HEIGHT(layer.displayFrame))
        return true;
    return false;
}

bool ExynosVirtualDisplayModule::manageFences(hwc_display_contents_1_t *contents, int fence)
{
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.acquireFenceFd >= 0) {
            close(layer.acquireFenceFd);
            layer.acquireFenceFd = -1;
        }

        if (layer.flags & HWC_SKIP_RENDERING) {
            layer.releaseFenceFd = layer.acquireFenceFd;
        }

        if (!(layer.flags & HWC_SKIP_RENDERING) &&
                (layer.releaseFenceFd <= -1) &&
                ((layer.compositionType == HWC_OVERLAY) ||
                (mFbNeeded == true && layer.compositionType == HWC_FRAMEBUFFER_TARGET))) {

            if (fence >= 0) {
                int dup_fd = dup(fence);
                if (dup_fd < 0)
                    DISPLAY_LOGE("release fence dup failed: %s", strerror(errno));

                layer.releaseFenceFd = dup_fd;
            } else {
                layer.releaseFenceFd = -1;
            }
        }

        if (layer.handle) {
            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
            DISPLAY_LOGD("CLEAN: layer %d type=%d, f=%x, w=%d, h=%d, s=%d, vs=%d, "
                  "{%.1f, %.1f, %.1f, %.1f}, {%d, %d, %d, %d}"
                  "fl=%08x, hdl=%p, trf=%02x, bl=%04x, Af=%d, Rf=%d, P=%d",
                    i, layer.compositionType, h->format, h->width, h->height, h->stride,
                    h->vstride, layer.sourceCropf.left, layer.sourceCropf.top,
                    layer.sourceCropf.right, layer.sourceCropf.bottom,
                    layer.displayFrame.left, layer.displayFrame.top,
                    layer.displayFrame.right, layer.displayFrame.bottom,
                    layer.flags, layer.handle, layer.transform, layer.blending,
                    layer.acquireFenceFd, layer.releaseFenceFd, getDrmMode(layer.flags));
        }
    }

    if (contents->outbufAcquireFenceFd >= 0) {
        close(contents->outbufAcquireFenceFd);
        contents->outbufAcquireFenceFd = -1;
    }

    contents->retireFenceFd = fence;
    return true;
}

void ExynosVirtualDisplayModule::deInit()
{
}

int ExynosVirtualDisplayModule::clearDisplay()
{
    return 0;
}

ExynosVirtualDisplayModule::~ExynosVirtualDisplayModule()
{
}

int32_t ExynosVirtualDisplayModule::getDisplayAttributes(const uint32_t attribute)
{
    switch(attribute) {
        case HWC_DISPLAY_COMPOSITION_TYPE:
            return mCompositionType;
        case HWC_DISPLAY_GLES_FORMAT:
            return mGLESFormat;
        case HWC_DISPLAY_SINK_BQ_FORMAT:
            if (mIsRotationState)
                return -1;
            else
                return mGLESFormat;
        case HWC_DISPLAY_SINK_BQ_USAGE:
            return mSinkUsage;
        case HWC_DISPLAY_SINK_BQ_WIDTH:
            if (mDisplayWidth == 0)
                return mWidth;
            return mDisplayWidth;
        case HWC_DISPLAY_SINK_BQ_HEIGHT:
            if (mDisplayHeight == 0)
                return mHeight;
            return mDisplayHeight;
        default:
            ALOGE("unknown display attribute %u", attribute);
            return -EINVAL;
    }
    return 0;
}
