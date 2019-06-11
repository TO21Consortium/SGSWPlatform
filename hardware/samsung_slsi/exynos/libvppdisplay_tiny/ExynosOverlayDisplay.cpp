#include "ExynosOverlayDisplay.h"

ExynosOverlayDisplay::ExynosOverlayDisplay(int __UNUSED__ numMPPs, struct exynos5_hwc_composer_device_1_t *pdev)
    :   ExynosDisplay(EXYNOS_PRIMARY_DISPLAY, pdev)
{
    this->mHwc = pdev;
}

ExynosOverlayDisplay::~ExynosOverlayDisplay()
{
}

