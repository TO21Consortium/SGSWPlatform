#ifndef EXYNOS_DUMMY_HDMI_H
#define EXYNOS_DUMMY_HDMI_H

#include "ExynosHWC.h"
#include "ExynosDisplay.h"

#define NUM_VIRT_OVER_HDMI 5

class ExynosExternalDisplay : public ExynosDisplay {
    public:
        /* Methods */
        ExynosExternalDisplay(struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosExternalDisplay();

        void setHdmiStatus(bool status);

        bool isPresetSupported(unsigned int preset);
        int getConfig();
        int getDisplayConfigs(uint32_t *configs, size_t *numConfigs);
        int enableLayer(hdmi_layer_t &hl);
        void disableLayer(hdmi_layer_t &hl);
        int enable();
        void disable();
        int output(hdmi_layer_t &hl, hwc_layer_1_t &layer, private_handle_t *h, int acquireFenceFd, int *releaseFenceFd);
        void skipStaticLayers(hwc_display_contents_1_t *contents, int ovly_idx);
        void setPreset(int preset);
        int convert3DTo2D(int preset);
        void calculateDstRect(int src_w, int src_h, int dst_w, int dst_h, struct v4l2_rect *dst_rect);
        void setHdcpStatus(int status);
        void setAudioChannel(uint32_t channels);
        uint32_t getAudioChannel();
        bool isIONBufferAllocated() {return mFlagIONBufferAllocated;};

        virtual int openHdmi();
        virtual int blank();
        virtual int prepare(hwc_display_contents_1_t* contents);
        virtual int set(hwc_display_contents_1_t* contents);
        int clearDisplay();
        virtual void freeExtVideoBuffers() {}
        virtual int waitForRenderFinish(private_module_t *grallocModule, buffer_handle_t *handle, int buffers);

        /* Fields */
        ExynosMPPModule         *mMPPs[1];

        bool                    mEnabled;
        bool                    mBlanked;
        bool                    mUseSubtitles;
        bool mFlagIONBufferAllocated;
};

#endif
