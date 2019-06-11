//#define LOG_NDEBUG 0
#define LOG_TAG "display"
#include "ExynosOverlayDisplay.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"
#include "ExynosHWCDebug.h"
#if defined(USES_DUAL_DISPLAY)
#include "ExynosSecondaryDisplayModule.h"
#endif

#ifdef G2D_COMPOSITION
#include "ExynosG2DWrapper.h"
#endif

ExynosOverlayDisplay::ExynosOverlayDisplay(int __unused numMPPs, struct exynos5_hwc_composer_device_1_t *pdev)
    :   ExynosDisplay(EXYNOS_PRIMARY_DISPLAY, pdev)
{
    this->mHwc = pdev;
#ifndef USES_DUAL_DISPLAY
    mInternalDMAs.add(IDMA_G0);
#endif
    mInternalDMAs.add(IDMA_G1);
}

void ExynosOverlayDisplay::doPreProcessing(hwc_display_contents_1_t* contents)
{
    mInternalDMAs.clear();
#ifndef USES_DUAL_DISPLAY
    mInternalDMAs.add(IDMA_G0);
#endif
    mInternalDMAs.add(IDMA_G1);
    ExynosDisplay::doPreProcessing(contents);
}

ExynosOverlayDisplay::~ExynosOverlayDisplay()
{
}

#if defined(USES_DUAL_DISPLAY)
int ExynosOverlayDisplay::set_dual(hwc_display_contents_1_t *contentsPrimary, hwc_display_contents_1_t *contentsSecondary)
{
    hwc_layer_1_t *fb_layer = NULL;
    int err = 0;

    if (mFbWindow != NO_FB_NEEDED) {
        if (contentsPrimary->numHwLayers >= 1 &&
                contentsPrimary->hwLayers[contentsPrimary->numHwLayers - 1].compositionType == HWC_FRAMEBUFFER_TARGET)
            fb_layer = &contentsPrimary->hwLayers[contentsPrimary->numHwLayers - 1];

        if (CC_UNLIKELY(!fb_layer)) {
            DISPLAY_LOGE("framebuffer target expected, but not provided");
            err = -EINVAL;
        } else {
            DISPLAY_LOGD(eDebugDefault, "framebuffer target buffer:");
            dumpLayer(fb_layer);
        }
    }

    int fence;
    if (!err) {
        fence = postFrame(contentsPrimary);
        if (fence < 0)
            err = fence;
    }

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
                dupFence(fence, contentsPrimary);
            } else {
                DISPLAY_LOGW("mLastRetireFenceFd dup failed: %s", strerror(errno));
                mLastRetireFenceFd = -1;
            }
        } else {
            ALOGE("WIN_CONFIG is skipped, but mLastRetireFenceFd is not valid");
        }
    } else {
        mLastRetireFenceFd = fence;
        dupFence(fence, contentsPrimary);
    }
    mHwc->secondaryDisplay->mLastRetireFenceFd = fence;
    mHwc->secondaryDisplay->dupFence(fence, contentsSecondary);

    return err;
}
#else
int ExynosOverlayDisplay::set_dual(hwc_display_contents_1_t __unused *contentsPrimary, hwc_display_contents_1_t __unused *contentsSecondary)
{
    return -1;
}
#endif
