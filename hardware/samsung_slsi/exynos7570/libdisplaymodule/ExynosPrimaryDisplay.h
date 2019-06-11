#ifndef EXYNOS_DISPLAY_MODULE_H
#define EXYNOS_DISPLAY_MODULE_H

#include "ExynosOverlayDisplay.h"

class ExynosPrimaryDisplay : public ExynosOverlayDisplay {
        enum decon_idma_type prevfbTargetIdma;

    public:
        ExynosPrimaryDisplay(int numGSCs, struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosPrimaryDisplay();

        virtual int getDeconWinMap(int overlayIndex, int totalOverlays);
        virtual void forceYuvLayersToFb(hwc_display_contents_1_t *contents);
        virtual int getMPPForUHD(hwc_layer_1_t &layer);
        virtual int getRGBMPPIndex(int index);

        void assignWindows(hwc_display_contents_1_t *contents);
        int postMPPM2M(hwc_layer_1_t &layer, struct decon_win_config *config, int win_map, int index);
        bool isOverlaySupported(hwc_layer_1_t &layer, size_t index,
                bool useVPPOverlay,
                ExynosMPPModule** supportedInternalMPP,
                ExynosMPPModule** supportedExternalMPP);
        bool isYuvDmaAvailable(int format, uint32_t dma);
        void handleStaticLayers(hwc_display_contents_1_t *contents,
                struct decon_win_config_data &win_data,
                int __unused tot_ovly_wins);
        int handleWindowUpdate(hwc_display_contents_1_t __unused *contents,
                struct decon_win_config __unused *config);
        void dump_win_config(struct decon_win_config *config);
        int getIDMAWidthAlign(int format);
        int getIDMAHeightAlign(int format);
};

#endif
