#ifndef EXYNOS_DISPLAY_MODULE_H
#define EXYNOS_DISPLAY_MODULE_H

#include "ExynosOverlayDisplay.h"

class ExynosPrimaryDisplay : public ExynosOverlayDisplay {
    public:
        ExynosPrimaryDisplay(int numGSCs, struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosPrimaryDisplay();

};

#endif
