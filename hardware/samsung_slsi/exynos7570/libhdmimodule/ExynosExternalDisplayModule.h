#ifndef EXYNOS_EXTERNAL_DISPLAY_MODULE_H
#define EXYNOS_EXTERNAL_DISPLAY_MODULE_H

#include "ExynosExternalDisplay.h"

class ExynosExternalDisplayModule : public ExynosExternalDisplay {
    public:
        ExynosExternalDisplayModule(struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosExternalDisplayModule();
};

#endif
