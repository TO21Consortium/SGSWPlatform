#include "ExynosSecondaryDisplay.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"
#include "ExynosPrimaryDisplay.h"

#define LOG_TAG "display"
class ExynosPrimaryDisplay;

ExynosSecondaryDisplay::ExynosSecondaryDisplay(struct exynos5_hwc_composer_device_1_t *pdev)
    :   ExynosDisplay(EXYNOS_SECONDARY_DISPLAY, pdev)
{
    this->mHwc = pdev;
    mXres = pdev->primaryDisplay->mXres;
    mYres = pdev->primaryDisplay->mYres;
    mXdpi = pdev->primaryDisplay->mXdpi;
    mYdpi = pdev->primaryDisplay->mYres;
    mVsyncPeriod  = pdev->primaryDisplay->mVsyncPeriod;

    ALOGD("using\n"
          "xres         = %d px\n"
          "yres         = %d px\n"
          "xdpi         = %f dpi\n"
          "ydpi         = %f dpi\n"
          "vsyncPeriod  = %d msec\n",
          mXres, mYres, mXdpi, mYdpi, mVsyncPeriod);
}

ExynosSecondaryDisplay::~ExynosSecondaryDisplay()
{
    disable();
}

int ExynosSecondaryDisplay::enable()
{
    if (mEnabled)
        return 0;

    /* To do: Should be implemented */

    mEnabled = true;

    return 0;
}

int ExynosSecondaryDisplay::disable()
{
    if (!mEnabled)
        return 0;

    /* To do: Should be implemented */

    mEnabled = false;

    return 0;
}

void ExynosSecondaryDisplay::doPreProcessing(hwc_display_contents_1_t* contents)
{
    mInternalDMAs.clear();
    mInternalDMAs.add(IDMA_G0);
    ExynosDisplay::doPreProcessing(contents);
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        layer.displayFrame.left += (mXres/2);
        layer.displayFrame.right += (mXres/2);
    }
}

int ExynosSecondaryDisplay::prepare(hwc_display_contents_1_t *contents)
{
    ExynosDisplay::prepare(contents);
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        layer.displayFrame.left -= (mXres/2);
        layer.displayFrame.right -= (mXres/2);
    }
    return 0;
}

int ExynosSecondaryDisplay::set(hwc_display_contents_1_t *contents)
{
    int ret = 0;
    /* Change position */
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        layer.displayFrame.left += (mXres/2);
        layer.displayFrame.right += (mXres/2);
    }

    ret = ExynosDisplay::set(contents);

    /* Restore position */
    for (size_t i = 0; i < contents->numHwLayers; i++) {
        hwc_layer_1_t &layer = contents->hwLayers[i];
        layer.displayFrame.left -= (mXres/2);
        layer.displayFrame.right -= (mXres/2);
    }

    return ret;
}
