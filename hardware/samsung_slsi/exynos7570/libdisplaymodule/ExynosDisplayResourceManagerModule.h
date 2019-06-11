#ifndef EXYNOS_DISPLAY_RESOURCE_MANAGER_MODULE_H
#define EXYNOS_DISPLAY_RESOURCE_MANAGER_MODULE_H

#include "ExynosDisplayResourceManager.h"

class ExynosDisplayResourceManagerModule : public ExynosDisplayResourceManager {
    public:
        ExynosDisplayResourceManagerModule(struct exynos5_hwc_composer_device_1_t *pdev);
        virtual ~ExynosDisplayResourceManagerModule();
};

#endif
