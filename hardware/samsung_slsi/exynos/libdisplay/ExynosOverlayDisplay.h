#ifndef EXYNOS_PRIMARY_DISPLAY_H
#define EXYNOS_PRIMARY_DISPLAY_H

#include "ExynosHWC.h"
#include "ExynosDisplay.h"

#define S3D_ERROR -1
#define HDMI_PRESET_DEFAULT V4L2_DV_1080P60
#define HDMI_PRESET_ERROR -1

class ExynosMPPModule;

enum regionType {
    eTransparentRegion          =       0,
    eCoveredOpaqueRegion        =       1,
    eDamageRegion               =       2,
};

enum {
    eDamageRegionFull = 0,
    eDamageRegionPartial,
    eDamageRegionSkip,
    eDamageRegionError,
};

class ExynosOverlayDisplay : public ExynosDisplay {
    public:
        /* Methods */
        ExynosOverlayDisplay(int numGSCs, struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosOverlayDisplay();

        virtual int prepare(hwc_display_contents_1_t *contents);
        virtual int set(hwc_display_contents_1_t *contents);
        virtual void dump(android::String8& result);
        virtual void freeMPP();
        virtual void handleTotalBandwidthOverload(hwc_display_contents_1_t *contents);

        int clearDisplay();
        int getCompModeSwitch();
        int32_t getDisplayAttributes(const uint32_t attribute);

        bool switchOTF(bool enable);

        /* Fields */
        ExynosMPPModule         **mMPPs;

        exynos5_hwc_post_data_t  mPostData;
        const private_module_t   *mGrallocModule;

#ifdef USE_FB_PHY_LINEAR
        buffer_handle_t          mWinBuf[NUM_HW_WINDOWS][NUM_GSC_DST_BUFS];
#ifdef G2D_COMPOSITION
        int                      mG2dComposition;
        exynos5_g2d_data_t       mG2d;
        int                      mG2dLayers;
        int                      mAllocatedLayers;
        uint32_t                 mWinBufVirtualAddress[NUM_HW_WINDOWS][NUM_GSC_DST_BUFS];
        int                      mWinBufFence[NUM_HW_WINDOWS][NUM_GSC_DST_BUFS];
        int                      mG2dCurrentBuffer[NUM_HW_WINDOWS];
        uint32_t	             mLastG2dLayerHandle[NUM_HW_WINDOWS];
        uint32_t                 mWinBufMapSize[NUM_HW_WINDOWS];
        int                      mG2dMemoryAllocated;
        int                      mG2dBypassCount;
#endif
#endif

        struct s3c_fb_win_config_data mLastConfigData;
        size_t                   mLastFbWindow;
        const void               *mLastHandles[NUM_HW_WINDOWS];
        exynos5_gsc_map_t        mLastGscMap[NUM_HW_WINDOWS];
        const void               *mLastLayerHandles[NUM_VIRT_OVER];
        int                      mLastOverlayWindowIndex;
        int                      mLastOverlayLayerIndex;
        int                      mVirtualOverlayFlag;

        bool                     mForceFbYuvLayer;
        int                      mCountSameConfig;
        /* g3d = 0, gsc = 1 */
        int                      mConfigMode;
        video_layer_config       mPrevDstConfig[MAX_VIDEO_LAYERS];

        int                      mGscLayers;

        bool                     mPopupPlayYuvContents;
        bool                     mHasCropSurface;
        int                      mYuvLayers;

        bool                     mBypassSkipStaticLayer;
        uint32_t                 mDmaChannelMaxBandwidth[MAX_NUM_FIMD_DMA_CH];
        uint32_t                 mDmaChannelMaxOverlapCount[MAX_NUM_FIMD_DMA_CH];

        bool                     mGscUsed;
        int                      mCurrentGscIndex;
        int                      mCurrentRGBMPPIndex;
        bool                     mBlanked;
        hwc_rect                 mFbUpdateRegion;
        bool                     mFbNeeded;
        size_t                   mFirstFb;
        size_t                   mLastFb;
        bool                     mForceFb;
        int                      mForceOverlayLayerIndex;
        bool                     mRetry;
        int                      mAllowedOverlays;
        size_t                   mMaxWindowOverlapCnt;

    protected:
        /* Methods */
        void configureOtfWindow(hwc_rect_t &displayFrame,
                int32_t blending, int32_t planeAlpha, int fence_fd, int format, s3c_fb_win_config &cfg);
        void configureHandle(private_handle_t *handle, hwc_frect_t &sourceCrop,
                hwc_rect_t &displayFrame, int32_t blending, int32_t planeAlpha, int fence_fd, s3c_fb_win_config &cfg);
        void skipStaticLayers(hwc_display_contents_1_t *contents);
        void determineSupportedOverlays(hwc_display_contents_1_t *contents);
        void determineBandwidthSupport(hwc_display_contents_1_t *contents);
        void assignWindows(hwc_display_contents_1_t *contents);
        bool assignGscLayer(hwc_layer_1_t &layer, int index, int nextWindow);
        void postGscOtf(hwc_layer_1_t &layer, struct s3c_fb_win_config *config, int win_map, int index);
        void handleStaticLayers(hwc_display_contents_1_t *contents, struct s3c_fb_win_config_data &win_data, int tot_ovly_wins);
        void cleanupGscs();
        int handleWindowUpdate(hwc_display_contents_1_t *contents, struct s3c_fb_win_config *config);
        unsigned int getLayerRegion(hwc_layer_1_t &layer, hwc_rect &rect_area, uint32_t regionType);

        virtual void determineYuvOverlay(hwc_display_contents_1_t *contents);
        virtual int postGscM2M(hwc_layer_1_t &layer, struct s3c_fb_win_config *config, int win_map, int index, bool isBottom);
        virtual void forceYuvLayersToFb(hwc_display_contents_1_t *contents);
        virtual void configureOverlay(hwc_layer_1_t *layer, s3c_fb_win_config &cfg);
        virtual void configureDummyOverlay(hwc_layer_1_t *layer, s3c_fb_win_config &cfg);
        virtual bool isOverlaySupported(hwc_layer_1_t &layer, size_t i);
        virtual void refreshGscUsage(hwc_layer_1_t &layer);
        virtual int postFrame(hwc_display_contents_1_t *contents);
        virtual int winconfigIoctl(s3c_fb_win_config_data *win_data);
        virtual int waitForRenderFinish(buffer_handle_t *handle, int buffers);
        virtual void handleOffscreenRendering(hwc_layer_1_t &layer, hwc_display_contents_1_t *contents, int index);
        virtual int getMPPForUHD(hwc_layer_1_t &layer);
        virtual int getRGBMPPIndex(int index);
        virtual bool multipleRGBScaling(int format);
        virtual void checkTVBandwidth();
};

#endif
