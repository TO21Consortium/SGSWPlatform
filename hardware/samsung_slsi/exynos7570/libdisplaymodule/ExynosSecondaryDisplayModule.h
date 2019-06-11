#ifndef EXYNOS_SECONDARY_DISPLAY_MODULE_H
#define EXYNOS_SECONDARY_DISPLAY_MODULE_H

#include "ExynosSecondaryDisplay.h"

class ExynosSecondaryDisplayModule : public ExynosSecondaryDisplay {
    public:
        ExynosSecondaryDisplayModule(struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosSecondaryDisplayModule();
};

#endif
