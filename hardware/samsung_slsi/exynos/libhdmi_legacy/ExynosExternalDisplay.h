#ifndef EXYNOS_LEGACY_HDMI_H
#define EXYNOS_LEGACY_HDMI_H

#include "ExynosHWC.h"
#include "ExynosDisplay.h"

#define VIDEO_LAYER_INDEX       2
#define NUM_VIRT_OVER_HDMI 5

#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
#define HWC_SKIP_HDMI_RENDERING 0x80000000
#endif
#define SUPPORTED_DV_TIMINGS_NUM 24
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
	{V4L2_DV_720P60_SB_HALF, 11},
	{V4L2_DV_720P60_TB, 12},
	{V4L2_DV_720P50_SB_HALF, 13},
	{V4L2_DV_720P50_TB, 14},
	{V4L2_DV_1080P24_FP, 15},
	{V4L2_DV_1080P24_SB_HALF, 16},
	{V4L2_DV_1080P24_TB, 17},
	{V4L2_DV_1080I60_SB_HALF, 18},
	{V4L2_DV_1080I50_SB_HALF, 19},
	{V4L2_DV_1080P60_SB_HALF, 20},
	{V4L2_DV_1080P60_TB, 21},
	{V4L2_DV_1080P30_SB_HALF, 22},
	{V4L2_DV_1080P30_TB, 23}
};

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
        void checkGrallocFlags(hwc_display_contents_1_t *contents);
        int getDVTimingsIndex(int preset);
        int getCecPaddr();
        bool isVideoOverlaySupported(hwc_layer_1_t &layer, int format);
        bool isVPSupported(hwc_layer_1_t &layer, int format);

        virtual int openHdmi();
        virtual int blank();
        virtual int prepare(hwc_display_contents_1_t* contents);
        virtual int set(hwc_display_contents_1_t* contents);

        virtual int waitForRenderFinish(private_module_t *grallocModule, buffer_handle_t *handle, int buffers);
        int clearDisplay();

        /* Fields */
        ExynosMPPModule         *mMPPs[1];

        int                     mMixer;
        bool                    mEnabled;
        bool                    mBlanked;
        hdmi_layer_t            mMixerLayers[3];
        int                     mIsFbLayer;
        int                     mIsVideoLayer;
        int                     mFbStarted;
        int                     mVideoStarted;
        bool                    mHasFbComposition;
        bool                    mHasSkipLayer;
        int                     mUiIndex;
        int                     mVideoIndex;
        bool                    mUseSubtitles;
        const void              *mLastLayerHandles[NUM_VIRT_OVER_HDMI];
        int                     mVirtualOverlayFlag;
        bool                    mIsCameraStarted;
        uint32_t                mFBT_Transform;
        bool                    mUseProtectedLayer;
#if defined(USE_GRALLOC_FLAG_FOR_HDMI)
        bool                    mLocalExternalDisplayPause;
        bool                    mUseScreenshootLayer;
#endif
};

#endif
