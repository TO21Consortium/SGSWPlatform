#ifndef EXYNOS_PRIMARY_DISPLAY_H
#define EXYNOS_PRIMARY_DISPLAY_H

#include "ExynosHWC.h"
#include "ExynosDisplay.h"

#define S3D_ERROR -1
#ifndef HDMI_INCAPABLE
#define HDMI_PRESET_DEFAULT V4L2_DV_1080P60
#endif
#define HDMI_PRESET_ERROR -1

class ExynosMPPModule;

class ExynosOverlayDisplay : public ExynosDisplay {
    public:
        /* Methods */
        ExynosOverlayDisplay(int numGSCs, struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosOverlayDisplay();
        int set_dual(hwc_display_contents_1_t *contentsPrimary, hwc_display_contents_1_t *contentsSecondary);
    protected:
        virtual void doPreProcessing(hwc_display_contents_1_t* contents);

};

#endif
