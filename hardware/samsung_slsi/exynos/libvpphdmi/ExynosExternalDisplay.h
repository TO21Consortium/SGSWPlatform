#ifndef EXYNOS_VPP_HDMI_H
#define EXYNOS_VPP_HDMI_H

#include <utils/Vector.h>
#include "ExynosHWC.h"
#include "ExynosDisplay.h"
#include <linux/videodev2.h>
#include "videodev2_exynos_hdmi.h"

#define MAX_HDMI_VIDEO_LAYERS 1

#define SUPPORTED_DV_TIMINGS_NUM        28

struct preset_index_mapping {
    int preset;
    int dv_timings_index;
};

const struct preset_index_mapping preset_index_mappings[SUPPORTED_DV_TIMINGS_NUM] = {
    {V4L2_DV_480P59_94, 0},
    {V4L2_DV_576P50, 1},
    {V4L2_DV_720P50, 2},
    {V4L2_DV_720P60, 3},
    {V4L2_DV_1080I50, 4},
    {V4L2_DV_1080I60, 5},
    {V4L2_DV_1080P24, 6},
    {V4L2_DV_1080P25, 7},
    {V4L2_DV_1080P30, 8},
    {V4L2_DV_1080P50, 9},
    {V4L2_DV_1080P60, 10},
    {V4L2_DV_2160P24, 11},
    {V4L2_DV_2160P25, 12},
    {V4L2_DV_2160P30, 13},
    {V4L2_DV_2160P24_1, 14},
    {V4L2_DV_720P60_SB_HALF, 15},
    {V4L2_DV_720P60_TB, 16},
    {V4L2_DV_720P50_SB_HALF, 17},
    {V4L2_DV_720P50_TB, 18},
    {V4L2_DV_1080P24_FP, 19},
    {V4L2_DV_1080P24_SB_HALF, 20},
    {V4L2_DV_1080P24_TB, 21},
    {V4L2_DV_1080I60_SB_HALF, 22},
    {V4L2_DV_1080I50_SB_HALF, 23},
    {V4L2_DV_1080P60_SB_HALF, 24},
    {V4L2_DV_1080P60_TB, 25},
    {V4L2_DV_1080P30_SB_HALF, 26},
    {V4L2_DV_1080P30_TB, 27}
};

enum {
    HDMI_RESOLUTION_BASE = 0,
    HDMI_480P_59_94,
    HDMI_576P_50,
    HDMI_720P_24,
    HDMI_720P_25,
    HDMI_720P_30,
    HDMI_720P_50,
    HDMI_720P_59_94,
    HDMI_720P_60,
    HDMI_1080I_29_97,
    HDMI_1080I_30,
    HDMI_1080I_25,
    HDMI_1080I_50,
    HDMI_1080I_60,
    HDMI_1080P_24,
    HDMI_1080P_25,
    HDMI_1080P_30,
    HDMI_1080P_50,
    HDMI_1080P_60,
    HDMI_480P_60,
    HDMI_1080I_59_94,
    HDMI_1080P_59_94,
    HDMI_1080P_23_98,
    HDMI_2160P_30 = 47,
};

#define S3D_720P_60_BASE        22
#define S3D_720P_59_94_BASE     25
#define S3D_720P_50_BASE        28
#define S3D_1080P_24_BASE       31
#define S3D_1080P_23_98_BASE    34
#define S3D_1080P_60_BASE       39
#define S3D_1080P_30_BASE       42

class ExynosExternalDisplay : public ExynosDisplay {
    public:
        /* Methods */
        ExynosExternalDisplay(struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosExternalDisplay();

        void setHdmiStatus(bool status);

        bool isPresetSupported(unsigned int preset);
        int getConfig();
        int enable();
        void disable();
        void setPreset(int preset);
        int convert3DTo2D(int preset);
        void setHdcpStatus(int status);
        void setAudioChannel(uint32_t channels);
        uint32_t getAudioChannel();
        int getCecPaddr();
        bool isIONBufferAllocated() {return mFlagIONBufferAllocated;};

        virtual int openHdmi();
        virtual void closeHdmi();
        virtual int blank();
        virtual int prepare(hwc_display_contents_1_t* contents);
        virtual int set(hwc_display_contents_1_t* contents);
        virtual void allocateLayerInfos(hwc_display_contents_1_t* contents);
        virtual int32_t getDisplayAttributes(const uint32_t attribute, uint32_t config = 0);
        virtual void determineYuvOverlay(hwc_display_contents_1_t *contents);
        virtual void determineSupportedOverlays(hwc_display_contents_1_t *contents);
        virtual int clearDisplay();
        virtual void freeExtVideoBuffers();
        virtual int getDisplayConfigs(uint32_t *configs, size_t *numConfigs);
        virtual int getActiveConfig();
        virtual int setActiveConfig(int index);

        bool                    mEnabled;
        bool                    mBlanked;

        bool                    mUseSubtitles;
        int                     mReserveMemFd;
        android::Vector< unsigned int > mConfigurations;

    protected:
        void skipUILayers(hwc_display_contents_1_t *contents);
        int getDVTimingsIndex(int preset);
        virtual void handleStaticLayers(hwc_display_contents_1_t *contents, struct decon_win_config_data &win_data, int tot_ovly_wins);
        void cleanConfigurations();
        void dumpConfigurations();
        virtual void configureHandle(private_handle_t *handle, size_t index, hwc_layer_1_t &layer, int fence_fd, decon_win_config &cfg);
        virtual int postMPPM2M(hwc_layer_1_t &layer, struct decon_win_config *config, int win_map, int index);
        virtual int postFrame(hwc_display_contents_1_t *contents);
        virtual void doPreProcessing(hwc_display_contents_1_t* contents);
    private:
        buffer_handle_t         mDRMTempBuffer;
        bool mFlagIONBufferAllocated;
        void requestIONMemory();
        void freeIONMemory();
        bool checkIONBufferPrepared();
        int configureDRMSkipHandle(decon_win_config &cfg);
        void setHdmiResolution(int resolution, int s3dMode);
};

#endif
