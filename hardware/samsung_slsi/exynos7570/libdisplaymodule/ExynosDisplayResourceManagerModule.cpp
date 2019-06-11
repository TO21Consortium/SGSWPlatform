#define LOG_TAG "DisplayResourceManagerModule"
#define LOG_NDEBUG 0

#include "ExynosDisplayResourceManagerModule.h"
#include "ExynosHWCModule.h"
#include "ExynosMPPModule.h"
#include "ExynosPrimaryDisplay.h"

ExynosDisplayResourceManagerModule::ExynosDisplayResourceManagerModule(struct exynos5_hwc_composer_device_1_t *pdev)
    : ExynosDisplayResourceManager(pdev)
{
    size_t num_mpp_units = sizeof(AVAILABLE_EXTERNAL_MPP_UNITS)/sizeof(exynos_mpp_t);
    for (size_t i = 0; i < num_mpp_units; i++) {
        exynos_mpp_t exynos_mpp = AVAILABLE_EXTERNAL_MPP_UNITS[i];
        ALOGV("externalMPP type(%d), index(%d)", exynos_mpp.type, exynos_mpp.index);
        ExynosMPPModule* exynosMPP = new ExynosMPPModule(NULL, exynos_mpp.type, exynos_mpp.index);
        exynosMPP->setAllocDevice(pdev->primaryDisplay->mAllocDevice);
        mExternalMPPs.add(exynosMPP);
    }
}

ExynosDisplayResourceManagerModule::~ExynosDisplayResourceManagerModule()
{
}
