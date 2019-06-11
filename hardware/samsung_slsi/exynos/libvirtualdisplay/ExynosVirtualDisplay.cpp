#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "ExynosVirtualDisplay"
#include "ExynosHWC.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"
#include "ExynosG2DWrapper.h"
#include "ExynosVirtualDisplay.h"
#include <errno.h>

ExynosVirtualDisplay::ExynosVirtualDisplay(struct exynos5_hwc_composer_device_1_t *pdev) :
    ExynosDisplay(1),
    mWidth(0),
    mHeight(0),
    mDisplayWidth(0),
    mDisplayHeight(0),
    mIsWFDState(false),
    mIsRotationState(false),
    mIsSecureDRM(false),
    mIsNormalDRM(false),
    mPhysicallyLinearBuffer(NULL),
    mPhysicallyLinearBufferAddr(0),
    mPresentationMode(0),
    mDeviceOrientation(0),
    mFrameBufferTargetTransform(0),
    mCompositionType(COMPOSITION_GLES),
    mPrevCompositionType(COMPOSITION_GLES),
    mGLESFormat(HAL_PIXEL_FORMAT_RGBA_8888),
    mSinkUsage(GRALLOC_USAGE_HW_COMPOSER)
{
    this->mHwc = pdev;
    mMPPs[0] = new ExynosMPPModule(this, WFD_GSC_IDX);
    mG2D = new ExynosG2DWrapper(NULL, NULL, this);

    for (int i = 0; i < NUM_FB_TARGET; i++) {
        fbTargetInfo[i].fd = -1;
        fbTargetInfo[i].mappedAddr = 0;
        fbTargetInfo[i].mapSize = 0;
    }

    memset(mDstHandles, 0x0, sizeof(int) * MAX_BUFFER_COUNT);
    mPrevDisplayFrame.left = 0;
    mPrevDisplayFrame.top = 0;
    mPrevDisplayFrame.right = 0;
    mPrevDisplayFrame.bottom = 0;
    memset(mPrevFbHandle, 0x0, sizeof(int) * NUM_FRAME_BUFFER);
}

ExynosVirtualDisplay::~ExynosVirtualDisplay()
{
    delete mMPPs[0];
    delete mG2D;
    unmapAddrFBTarget();
}

int ExynosVirtualDisplay::prepare(hwc_display_contents_1_t* contents)
{
    ALOGV("preparing %u layers for virtual", contents->numHwLayers);
    hwc_layer_1_t *video_layer = NULL;
    int ret = 0;
    mIsRotationState = false;
    hwc_layer_1_t *overlay_layer = NULL;
    hwc_layer_1_t *fb_layer = NULL;

    mCompositionType = COMPOSITION_GLES;
    mIsSecureDRM = false;
    mIsNormalDRM = false;
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.flags & HWC_SCREENSHOT_ANIMATOR_LAYER) {
            ALOGV("include rotation animation layer");
            mIsRotationState = true;
            overlay_layer = &layer;
            break;
        }

        if (layer.flags & HWC_SKIP_LAYER) {
            ALOGV("include skipped layer");
            if (layer.handle) {
                private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
                if (getDrmMode(h->flags) != NORMAL_DRM || !mIsWFDState)
                    continue;
            } else
                continue;
        }

        if (layer.handle) {
            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);
            if(mMPPs[0]->isProcessingSupported(layer, h->format, false) > 0) {
                if (getDrmMode(h->flags) == SECURE_DRM) {
                    layer.compositionType = HWC_OVERLAY;
                    mIsSecureDRM = true;
                    overlay_layer = &layer;
                    ALOGV("include secure drm layer");
                    continue;
                }
                if (getDrmMode(h->flags) == NORMAL_DRM) {
                    layer.compositionType = HWC_OVERLAY;
                    mIsNormalDRM = true;
                    overlay_layer = &layer;
                    ALOGV("include normal drm layer");
                    continue;
                }
#ifdef VIRTUAL_DISPLAY_VIDEO_IS_OVERLAY
                if ((h->flags & GRALLOC_USAGE_EXTERNAL_DISP) && mIsWFDState &&
                    (h->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M ||
                     h->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV ||
                     h->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED)) {
                    layer.compositionType = HWC_OVERLAY;
                    overlay_layer = &layer;
                    ALOGV("include normal video layer as overlay");
                    continue;
                }
#endif
            }
            if (layer.compositionType == HWC_FRAMEBUFFER) {
                fb_layer = &layer;
                ALOGV("include fb layer");
            }
        }
    }

    if (overlay_layer && fb_layer)
        mCompositionType = COMPOSITION_MIXED;
    else if (overlay_layer)
        mCompositionType = COMPOSITION_HWC;

    ALOGV("mCompositionType 0x%x, mPrevCompositionType 0x%x, overlay_layer 0x%x, fb_layer 0x%x",
        mCompositionType, mPrevCompositionType, overlay_layer, fb_layer);

    if (mPrevCompositionType != mCompositionType) {
        ExynosMPPModule &gsc = *mMPPs[0];
        gsc.mDstBuffers[gsc.mCurrentBuf] = NULL;
        gsc.mDstBufFence[gsc.mCurrentBuf] = -1;
        gsc.cleanupM2M();
    }

    mSinkUsage = GRALLOC_USAGE_HW_COMPOSER;

    if (mIsSecureDRM)
        mSinkUsage |= GRALLOC_USAGE_SW_READ_NEVER |
            GRALLOC_USAGE_SW_WRITE_NEVER |
            GRALLOC_USAGE_PROTECTED |
            GRALLOC_USAGE_PHYSICALLY_LINEAR;
    else if (mIsNormalDRM)
        mSinkUsage |= GRALLOC_USAGE_PRIVATE_NONSECURE;
    ALOGV("Sink Buffer's Usage: 0x%x", mSinkUsage);

    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            ALOGV("\tlayer %u: framebuffer target", i);

            if (!mIsSecureDRM && !mIsNormalDRM)
                calcDisplayRect(layer);
            layer.transform = mFrameBufferTargetTransform;
            continue;
        }

        if (mIsRotationState) {
            layer.compositionType = HWC_OVERLAY;
            layer.flags = HWC_SKIP_RENDERING;
            if (mIsSecureDRM)
                ret = true;
            continue;
        }

        if (layer.compositionType == HWC_BACKGROUND) {
            ALOGV("\tlayer %u: background layer", i);
            dumpLayer(&layer);
            continue;
        }

        if (layer.handle) {
            private_handle_t *h = private_handle_t::dynamicCast(layer.handle);

            if (overlay_layer && (h->flags & GRALLOC_USAGE_EXTERNAL_DISP)) {
                if (mMPPs[0]->isProcessingSupported(layer, h->format, false) > 0) {
                    if (!video_layer) {
                        if (mIsSecureDRM)
                            ret = mG2D->InitSecureG2D();
                        video_layer = &layer;
                        calcDisplayRect(layer);

                        ALOGV("\tlayer %u: video layer, mIsSecureDRM %d, mPhysicallyLinearBuffer 0x%x",
                            i, mIsSecureDRM, mPhysicallyLinearBuffer);
                        dumpLayer(&layer);
                        continue;
                    }
                } else {
                    layer.compositionType = HWC_OVERLAY;
                    layer.flags = HWC_SKIP_RENDERING;
                    ALOGV("\tlayer %u: skip drm layer", i);
                    continue;
                }
            }
        }
        layer.compositionType = HWC_FRAMEBUFFER;
        dumpLayer(&layer);
    }

    if (!ret && mPhysicallyLinearBufferAddr) {
        mG2D->TerminateSecureG2D();
        unmapAddrFBTarget();
    }

    return 0;
}

bool ExynosVirtualDisplay::isLayerFullSize(hwc_layer_1_t *layer)
{
    if (layer == NULL) {
        ALOGE("layer is null");
        return false;
    }

    if (layer->displayFrame.left == 0 &&
        layer->displayFrame.top == 0 &&
        layer->displayFrame.right == mWidth &&
        layer->displayFrame.bottom == mHeight) {
        return true;
    } else {
        return false;
    }
}

bool ExynosVirtualDisplay::isLayerResized(hwc_layer_1_t *layer)
{
    if (layer == NULL) {
        ALOGE("layer is null");
        return false;
    }

    if (layer->displayFrame.left == mPrevDisplayFrame.left &&
        layer->displayFrame.top == mPrevDisplayFrame.top &&
        layer->displayFrame.right == mPrevDisplayFrame.right &&
        layer->displayFrame.bottom == mPrevDisplayFrame.bottom) {
        return false;
    } else {
        mPrevDisplayFrame.left = layer->displayFrame.left;
        mPrevDisplayFrame.top = layer->displayFrame.top;
        mPrevDisplayFrame.right = layer->displayFrame.right;
        mPrevDisplayFrame.bottom = layer->displayFrame.bottom;
        return true;
    }
}

bool ExynosVirtualDisplay::isNewHandle(void *dstHandle)
{
    int i = 0;
    for (i = 0; i < MAX_BUFFER_COUNT; i++) {
        if (mDstHandles[i] == dstHandle) {
            return false;
        } else if (mDstHandles[i] == NULL) {
            mDstHandles[i] = dstHandle;
            break;
        }
    }

    if (i == MAX_BUFFER_COUNT) {
        memset(mDstHandles, 0x0, sizeof(int) * MAX_BUFFER_COUNT);
        mDstHandles[0] = dstHandle;
    }
    return true;
}

int ExynosVirtualDisplay::set(hwc_display_contents_1_t* contents)
{
    hwc_layer_1_t *overlay_layer = NULL;
    hwc_layer_1_t *target_layer = NULL;
    hwc_layer_1_t *fb_layer[NUM_FRAME_BUFFER] = {NULL};
    int number_of_fb = 0;
    int IsNormalDRMWithSkipLayer = false;

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

        if (layer.compositionType == HWC_FRAMEBUFFER) {
            if (!layer.handle)
                continue;
            ALOGV("framebuffer layer %d", i);
            fb_layer[number_of_fb++] = &layer;
            if (number_of_fb >= NUM_FRAME_BUFFER)
                number_of_fb = NUM_FRAME_BUFFER-1;
        }

        if (layer.compositionType == HWC_OVERLAY) {
            if (!layer.handle)
                continue;

            if (layer.flags & HWC_SKIP_RENDERING) {
                layer.releaseFenceFd = layer.acquireFenceFd;
                continue;
            }

            ALOGV("overlay layer %d", i);
            overlay_layer = &layer;
            continue;
        }

        if (layer.compositionType == HWC_FRAMEBUFFER_TARGET) {
            if (!layer.handle)
                continue;

            ALOGV("FB target layer %d", i);

            target_layer = &layer;
            continue;
        }
    }

    if (contents->outbuf == NULL) {
        ALOGE("BufferQueue is abandoned");
        return 0;
    }

    if (target_layer) {
        int ret = 0;
        ExynosMPPModule &gsc = *mMPPs[0];
        gsc.mDstBuffers[gsc.mCurrentBuf] = contents->outbuf;
        gsc.mDstBufFence[gsc.mCurrentBuf] = contents->outbufAcquireFenceFd;
        private_handle_t *dstHandle = private_handle_t::dynamicCast(contents->outbuf);

        if (IsNormalDRMWithSkipLayer) {
            if (target_layer->acquireFenceFd >= 0)
                contents->retireFenceFd = target_layer->acquireFenceFd;
            if (contents->outbufAcquireFenceFd >= 0) {
                close(contents->outbufAcquireFenceFd);
                contents->outbufAcquireFenceFd = -1;
            }
            mG2D->runCompositor(*target_layer, dstHandle, 0, 0xff, 0xff000000, BLIT_OP_SOLID_FILL, true, 0, 0, 0);
        } else if (mCompositionType == COMPOSITION_GLES) {
            ALOGV("COMPOSITION_GLES");
            if (target_layer->acquireFenceFd >= 0)
                contents->retireFenceFd = target_layer->acquireFenceFd;
            if (contents->outbufAcquireFenceFd >= 0) {
                close(contents->outbufAcquireFenceFd);
                contents->outbufAcquireFenceFd = -1;
            }
        } else if (overlay_layer && mCompositionType == COMPOSITION_MIXED) {
            void *newFbHandle[NUM_FRAME_BUFFER] = {NULL};
            for(size_t i = 0; i < number_of_fb; i++) {
                newFbHandle[i] = (void *)fb_layer[i]->handle;
            }

            if (isLayerResized(overlay_layer) ||
                (!isLayerFullSize(overlay_layer) && number_of_fb > 0 && memcmp(mPrevFbHandle, newFbHandle, sizeof(int) * NUM_FRAME_BUFFER) != 0)) {
                memset(mDstHandles, 0x0, sizeof(int) * MAX_BUFFER_COUNT);
            }

            if (isNewHandle(dstHandle)) {
                if (mIsSecureDRM) {
                    private_handle_t *secureHandle = private_handle_t::dynamicCast(mPhysicallyLinearBuffer);
                    ret = mG2D->runSecureCompositor(*target_layer, dstHandle, secureHandle, 0xff, 0xff000000, BLIT_OP_SOLID_FILL, true);
                } else {
                    ret = mG2D->runCompositor(*target_layer, dstHandle, 0, 0xff, 0xff000000, BLIT_OP_SOLID_FILL, true, 0, 0, 0);
                }
            }
            if (number_of_fb > 0) {
                ALOGV("COMPOSITION_MIXED");
                ret = gsc.processM2M(*overlay_layer, dstHandle->format, NULL, false);
                if (ret < 0)
                    ALOGE("failed to configure gscaler for video layer");

                if (gsc.mDstConfig.releaseFenceFd >= 0) {
                    if (sync_wait(gsc.mDstConfig.releaseFenceFd, 1000) < 0)
                        ALOGE("sync_wait error");
                    close(gsc.mDstConfig.releaseFenceFd);
                    gsc.mDstConfig.releaseFenceFd = -1;
                }
                if (target_layer->acquireFenceFd > 0) {
                    close(target_layer->acquireFenceFd);
                    target_layer->acquireFenceFd = -1;
                }

                if (mIsSecureDRM) {
                    ALOGV("Secure DRM playback");
                    private_handle_t *targetBufferHandle = private_handle_t::dynamicCast(target_layer->handle);
                    void* srcAddr = getMappedAddrFBTarget(targetBufferHandle->fd);
                    private_handle_t *secureHandle = private_handle_t::dynamicCast(mPhysicallyLinearBuffer);

                    if (memcmp(mPrevFbHandle, newFbHandle, sizeof(int) * NUM_FRAME_BUFFER) != 0) {
                        ALOGV("Buffer of fb layer is changed");
                        memcpy(mPrevFbHandle, newFbHandle, sizeof(int) * NUM_FRAME_BUFFER);
                        if ((srcAddr != NULL) && mPhysicallyLinearBufferAddr) {
                            memcpy((void *)mPhysicallyLinearBufferAddr, (void *)srcAddr, mWidth * mHeight * 4);
                        } else {
                            ALOGE("can't memcpy for secure G2D input buffer");
                        }
                    }

                    ret = mG2D->runSecureCompositor(*target_layer, dstHandle, secureHandle, 0xff,
                            0, BLIT_OP_SRC_OVER, false);
                    if (ret < 0) {
                        mG2D->TerminateSecureG2D();
                        unmapAddrFBTarget();
                        ALOGE("runSecureCompositor is failed");
                    }
                } else {  /* Normal video layer + Blending */
                    ALOGV("Normal DRM playback");
                    ret = mG2D->runCompositor(*target_layer, dstHandle, 0, 0xff, 0,
                            BLIT_OP_SRC_OVER, false, 0, 0, 0);
                    if (ret < 0) {
                        ALOGE("runCompositor is failed");
                    }

                    if (target_layer->releaseFenceFd > 0) {
                        close(target_layer->releaseFenceFd);
                        target_layer->releaseFenceFd = -1;
                    }
                }
            }
        } else if (overlay_layer) {
            ALOGV("COMPOSITION_HWC");
            ret = gsc.processM2M(*overlay_layer, dstHandle->format, NULL, false);
            if (ret < 0)
                ALOGE("failed to configure gscaler for video layer");
            contents->retireFenceFd = gsc.mDstConfig.releaseFenceFd;
            if (target_layer->acquireFenceFd > 0) {
                close(target_layer->acquireFenceFd);
                target_layer->acquireFenceFd = -1;
            }
        } else {
            ALOGV("animation layer skip");
            if (target_layer->acquireFenceFd >= 0)
                contents->retireFenceFd = target_layer->acquireFenceFd;
            if (contents->outbufAcquireFenceFd >= 0) {
                close(contents->outbufAcquireFenceFd);
                contents->outbufAcquireFenceFd = -1;
            }
        }
    }

    mPrevCompositionType = mCompositionType;

    return 0;
}

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

void* ExynosVirtualDisplay::getMappedAddrFBTarget(int fd)
{
    for (int i = 0; i < NUM_FB_TARGET; i++) {
        if (fbTargetInfo[i].fd == fd)
            return fbTargetInfo[i].mappedAddr;

        if (fbTargetInfo[i].fd == -1) {
            fbTargetInfo[i].fd = fd;
            fbTargetInfo[i].mappedAddr = mmap(NULL, mWidth * mHeight * 4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            fbTargetInfo[i].mapSize = mWidth * mHeight * 4;
            return fbTargetInfo[i].mappedAddr;
        }
    }
    return 0;
}

void ExynosVirtualDisplay::unmapAddrFBTarget()
{
    for (int i = 0; i < NUM_FB_TARGET; i++) {
        if (fbTargetInfo[i].fd != -1) {
            munmap((void *)fbTargetInfo[i].mappedAddr, fbTargetInfo[i].mapSize);
            fbTargetInfo[i].fd = -1;
            fbTargetInfo[i].mappedAddr = 0;
            fbTargetInfo[i].mapSize = 0;
        }
    }
}

void ExynosVirtualDisplay::init()
{

}

void ExynosVirtualDisplay::init(hwc_display_contents_1_t* contents)
{
    init();
}

void ExynosVirtualDisplay::deInit()
{
    ExynosMPPModule &gsc = *mMPPs[0];
    gsc.mDstBuffers[gsc.mCurrentBuf] = NULL;
    gsc.mDstBufFence[gsc.mCurrentBuf] = -1;
    gsc.cleanupM2M();
    mG2D->TerminateSecureG2D();
    unmapAddrFBTarget();
    mPrevCompositionType = COMPOSITION_GLES;
}

int ExynosVirtualDisplay::blank()
{
    return 0;
}

int ExynosVirtualDisplay::getConfig()
{
    return 0;
}

int32_t ExynosVirtualDisplay::getDisplayAttributes(const uint32_t attribute)
{
    return 0;
}
