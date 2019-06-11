#ifndef EXYNOS_SECONDARY_DISPLAY_H
#define EXYNOS_SECONDARY_DISPLAY_H

#include "ExynosHWC.h"
#include "ExynosDisplay.h"

class ExynosMPPModule;

class ExynosSecondaryDisplay : public ExynosDisplay {
    public:
        /* Methods */
        ExynosSecondaryDisplay(struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosSecondaryDisplay();
        int enable();
        int disable();
        virtual int prepare(hwc_display_contents_1_t *contents);
        virtual int set(hwc_display_contents_1_t *contents);

        bool                    mEnabled;
    protected:
        virtual void doPreProcessing(hwc_display_contents_1_t* contents);

};

#endif
