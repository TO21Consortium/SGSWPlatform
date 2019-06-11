#ifndef EXYNOS_PRIMARY_DISPLAY_H
#define EXYNOS_PRIMARY_DISPLAY_H

#include "ExynosHWC.h"
#include "ExynosDisplay.h"

class ExynosOverlayDisplay : public ExynosDisplay {
    public:
        /* Methods */
        ExynosOverlayDisplay(int numGSCs, struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosOverlayDisplay();
};

#endif
