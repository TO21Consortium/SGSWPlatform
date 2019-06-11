#include "ExynosPrimaryDisplay.h"

ExynosPrimaryDisplay::ExynosPrimaryDisplay(int numGSCs, struct exynos5_hwc_composer_device_1_t *pdev) :
    ExynosOverlayDisplay(numGSCs, pdev)
{
}

ExynosPrimaryDisplay::~ExynosPrimaryDisplay()
{
}