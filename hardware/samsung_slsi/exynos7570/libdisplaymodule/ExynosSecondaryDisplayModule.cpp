#include "ExynosSecondaryDisplayModule.h"
#include "ExynosHWCModule.h"
#include "ExynosHWCUtils.h"
#include "ExynosMPPModule.h"

ExynosSecondaryDisplayModule::ExynosSecondaryDisplayModule(struct exynos5_hwc_composer_device_1_t *pdev) :
    ExynosSecondaryDisplay(pdev)
{
}

ExynosSecondaryDisplayModule::~ExynosSecondaryDisplayModule()
{
}
