#include "ExynosExternalDisplayModule.h"

ExynosExternalDisplayModule::ExynosExternalDisplayModule(struct exynos5_hwc_composer_device_1_t *pdev)
    : ExynosExternalDisplay(pdev)
{
}

ExynosExternalDisplayModule::~ExynosExternalDisplayModule()
{
}
