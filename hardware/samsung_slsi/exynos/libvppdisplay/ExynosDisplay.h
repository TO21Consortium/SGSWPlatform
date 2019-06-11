#ifndef EXYNOS_DISPLAY_H
#define EXYNOS_DISPLAY_H

#include <utils/Mutex.h>
#include <utils/Vector.h>
#include <utils/String8.h>
#include "ExynosHWC.h"
#include "ExynosDisplayResourceManager.h"

class ExynosMPPModule;

#define HWC_SKIP_RENDERING 0x80000000
enum {
    eSkipLayer                    =     0x00000001,
    eUnsupportedPlaneAlpha        =     0x00000002,
    eInvalidHandle                =     0x00000004,
    eHasFloatSrcCrop              =     0x00000008,
    eUnsupportedDstWidth          =     0x00000010,
    eUnsupportedCoordinate        =     0x00000020,
    eUnsupportedFormat            =     0x00000040,
    eUnsupportedBlending          =     0x00000080,
    eDynamicRecomposition         =     0x00000100,
    eForceFbEnabled               =     0x00000200,
    eSandwitchedBetweenGLES       =     0x00000400,
    eHasPopupVideo                =     0x00000800,
    eHasDRMVideo                  =     0x00001000,
    eInsufficientBandwidth        =     0x00002000,
    eInsufficientOverlapCount     =     0x00004000,
    eInsufficientWindow           =     0x00008000,
    eInsufficientMPP              =     0x00010000,
    eSwitchingLocalPath           =     0x00020000,
    eRGBLayerDuringVideoPlayback  =     0x00040000,
    eSkipStaticLayer              =     0x00080000,
    eNotAlignedDstPosition        =     0x00100000,
    eUnSupportedUseCase           =     0x00200000,
    eMPPUnsupported               =     0x40000000,
    eUnknown                      =     0x80000000,
};

enum {
    eWindowUpdateDisabled = 0,
    eWindowUpdateInvalidIndex = 1,
    eWindowUpdateGeometryChanged = 2,
    eWindowUpdateInvalidRegion = 3,
    eWindowUpdateNotUpdated = 4,
    eWindowUpdateOverThreshold = 5,
    eWindowUpdateAdjustmentFail = 6,
    eWindowUpdateInvalidConfig = 7,
    eWindowUpdateUnsupportedUseCase = 8,
    eWindowUpdateUnknownError,
};

enum {
    eDamageRegionFull = 0,
    eDamageRegionPartial,
    eDamageRegionSkip,
    eDamageRegionError,
};

const struct deconFormat {
    uint32_t format;
    const char *desc;
} deconFormat[] = {
    {DECON_PIXEL_FORMAT_ARGB_8888, "ARGB8888"},
    {DECON_PIXEL_FORMAT_ABGR_8888, "ABGR8888"},
    {DECON_PIXEL_FORMAT_RGBA_8888, "RGBA8888"},
    {DECON_PIXEL_FORMAT_BGRA_8888, "BGRA8888"},
    {DECON_PIXEL_FORMAT_XRGB_8888, "XRGB8888"},
    {DECON_PIXEL_FORMAT_XBGR_8888, "XBGR8888"},
    {DECON_PIXEL_FORMAT_RGBX_8888, "RGBX8888"},
    {DECON_PIXEL_FORMAT_BGRX_8888, "BGRX8888"},
    {DECON_PIXEL_FORMAT_RGBA_5551, "RGBA5551"},
    {DECON_PIXEL_FORMAT_RGB_565,   "RGB565"},
    {DECON_PIXEL_FORMAT_NV16, "FORMATNV16"},
    {DECON_PIXEL_FORMAT_NV61, "FORMATNV61"},
    {DECON_PIXEL_FORMAT_YVU422_3P, "YVU4223P"},
    {DECON_PIXEL_FORMAT_NV12, "FORMATNV12"},
    {DECON_PIXEL_FORMAT_NV21, "FORMATNV21"},
    {DECON_PIXEL_FORMAT_NV12M, "FORMATNV12M"},
    {DECON_PIXEL_FORMAT_NV21M, "FORMATNV21M"},
    {DECON_PIXEL_FORMAT_YUV420, "FORMATYUV420"},
    {DECON_PIXEL_FORMAT_YVU420, "FORMATYVU420"},
    {DECON_PIXEL_FORMAT_YUV420M, "FORMATYUV420M"},
    {DECON_PIXEL_FORMAT_YVU420M, "FORMATYVU420M"},
};

enum {
    EXYNOS_PRIMARY_DISPLAY      =       0,
#if defined(USES_DUAL_DISPLAY)
    EXYNOS_SECONDARY_DISPLAY,
#endif
    EXYNOS_EXTERNAL_DISPLAY,
    EXYNOS_VIRTUAL_DISPLAY
};

enum regionType {
    eTransparentRegion          =       0,
    eCoveredOpaqueRegion        =       1,
    eDamageRegion               =       2,
};

class ExynosLayerInfo {
    public:
        int32_t         compositionType;
        uint32_t        mCheckOverlayFlag;
        uint32_t        mCheckMPPFlag;
        int32_t         mWindowIndex;
        int32_t        mDmaType;
        bool            mCompressed;
        ExynosMPPModule *mExternalMPP;     /* For MSC, GSC, FIMC ... */
        ExynosMPPModule *mInternalMPP;     /* For VPP */
};

class ExynosPreProcessedInfo {
    public:
        bool mHasDrmSurface;
};

struct exynos_mpp_map_t {
    exynos_mpp_t internal_mpp;
    exynos_mpp_t external_mpp;
};

enum decon_pixel_format halFormatToS3CFormat(int format);
bool isFormatSupported(int format);
enum decon_blending halBlendingToS3CBlending(int32_t blending);
bool isBlendingSupported(int32_t blending);
const char *deconFormat2str(uint32_t format);
bool winConfigChanged(decon_win_config *c1, decon_win_config *c2);
bool frameChanged(decon_frame *f1, decon_frame *f2);
enum vpp_rotate halTransformToHWRot(uint32_t halTransform);

class ExynosDisplay {
    public:
        /* Methods */
        ExynosDisplay(int numMPPs);
        ExynosDisplay(uint32_t type, struct exynos5_hwc_composer_device_1_t *pdev);
        virtual ~ExynosDisplay();

        virtual int prepare(hwc_display_contents_1_t *contents);
        virtual int set(hwc_display_contents_1_t *contents);
        virtual int setPowerMode(int mode);
        virtual void dump(android::String8& result);
        virtual void freeMPP();
        virtual void allocateLayerInfos(hwc_display_contents_1_t* contents);
        virtual void dumpLayerInfo(android::String8& result);

        virtual bool handleTotalBandwidthOverload(hwc_display_contents_1_t *contents);
        virtual int clearDisplay();
        int getCompModeSwitch();
        virtual int32_t getDisplayAttributes(const uint32_t attribute, uint32_t config = 0);
        virtual void preAssignFbTarget(hwc_display_contents_1_t *contents, bool assign);
        virtual void determineYuvOverlay(hwc_display_contents_1_t *contents);
        virtual void determineSupportedOverlays(hwc_display_contents_1_t *contents);
        virtual void determineBandwidthSupport(hwc_display_contents_1_t *contents);
        virtual void assignWindows(hwc_display_contents_1_t *contents);
        bool getPreviousDRMDMA(int *dma);
        void removeIDMA(decon_idma_type idma);
        void dumpMPPs(android::String8& result);
        void dumpConfig(decon_win_config &c);
        void dumpConfig(decon_win_config &c, android::String8& result);
        void dumpContents(android::String8& result, hwc_display_contents_1_t *contents);
        int checkConfigValidation(decon_win_config *config);

        virtual int getDisplayConfigs(uint32_t *configs, size_t *numConfigs);
        virtual int getActiveConfig() {return mActiveConfigIndex;};
        virtual int setActiveConfig(int __unused index) {return 0;};
        virtual void dupFence(int fence, hwc_display_contents_1_t *contents);

        /* Fields */
        int                     mDisplayFd;
        uint32_t                mType;
        int                     mPanelType;
        int32_t                 mDSCHSliceNum;
        int32_t                 mDSCYSliceSize;
        int32_t                 mXres;
        int32_t                 mYres;
        int32_t                 mXdpi;
        int32_t                 mYdpi;
        int32_t                 mVsyncPeriod;
        bool                    mBlanked;
        struct exynos5_hwc_composer_device_1_t *mHwc;
        alloc_device_t          *mAllocDevice;
        const private_module_t   *mGrallocModule;
        android::Mutex          mLayerInfoMutex;
        android::Vector<ExynosLayerInfo *> mLayerInfos;
        ExynosMPPVector         mInternalMPPs;
        android::Vector< ExynosMPPModule* > mExternalMPPs;
        android::Vector<hwc_frect_t> mBackUpFrect;
        android::Vector<hwc_frect_t> mOriginFrect;

        /* For SkipStatic Layer feature */
        size_t                   mLastFbWindow;
        const void               *mLastLayerHandles[NUM_VIRT_OVER];
        int                      mVirtualOverlayFlag;
        bool                     mBypassSkipStaticLayer;

        int                      mMPPLayers;
        int                      mYuvLayers;
        bool                     mHasDrmSurface;
        bool                     mLayersNeedScaling;

        bool                     mFbNeeded;
        size_t                   mFirstFb;
        size_t                   mLastFb;
        size_t                   mFbWindow;
        bool                     mForceFb;
        hwc_rect                 mFbUpdateRegion;
        int                      mForceOverlayLayerIndex;
        int                      mAllowedOverlays;
        android::Vector< unsigned int > mInternalDMAs;

        /* For dump last information */
        struct decon_win_config_data mLastConfigData;
        exynos_mpp_map_t         mLastMPPMap[NUM_HW_WINDOWS];
        const void               *mLastHandles[NUM_HW_WINDOWS];

        /* mOtfMode is not used.
         * It is only for compatibility with ExynosMPP */
        int                     mOtfMode;
        /* Below members are not used.
         * They are only for compatibility with HWC */
        bool                     mGscUsed;
        size_t                   mMaxWindowOverlapCnt;
        android::String8         mDisplayName;
        bool                     mUseSecureDMA;

        uint32_t                 mExternalMPPDstFormat;
        bool                     mSkipStaticInitFlag;
        size_t                   mNumStaticLayers;
        int                      mLastRetireFenceFd;

        ExynosPreProcessedInfo   mPreProcessedInfo;

        bool                     mFbPreAssigned;
        uint32_t                 mActiveConfigIndex;
        ExynosMPPModule          *mCheckIntMPP;
        struct decon_win_config_data *mWinData;
    protected:
        /* Methods */
        void skipStaticLayers(hwc_display_contents_1_t *contents);
        virtual int handleWindowUpdate(hwc_display_contents_1_t *contents, struct decon_win_config *config);
        void getLayerRegion(hwc_layer_1_t &layer, decon_win_rect &rect_area, uint32_t regionType);
        unsigned int getLayerRegion(hwc_layer_1_t &layer, hwc_rect &rect_area, uint32_t regionType);

        virtual void handleStaticLayers(hwc_display_contents_1_t *contents, struct decon_win_config_data &win_data, int tot_ovly_wins);
        virtual void configureHandle(private_handle_t *handle, size_t index, hwc_layer_1_t &layer, int fence_fd, decon_win_config &cfg);
        virtual int postMPPM2M(hwc_layer_1_t &layer, struct decon_win_config *config, int win_map, int index);
        virtual void configureOverlay(hwc_layer_1_t *layer, size_t index, decon_win_config &cfg);
        virtual bool isOverlaySupported(hwc_layer_1_t &layer, size_t index, bool useVPPOverlay, ExynosMPPModule** supportedInternalMPP, ExynosMPPModule** supportedExternalMPP);
        virtual int postFrame(hwc_display_contents_1_t *contents);
        virtual int winconfigIoctl(decon_win_config_data *win_data);
        virtual bool isProcessingRequired(hwc_layer_1_t &layer);
        virtual bool isBothMPPProcessingRequired(hwc_layer_1_t &layer);
        virtual bool isBothMPPProcessingRequired(hwc_layer_1_t &layer, hwc_layer_1_t *extMPPOutLayer);
        virtual bool multipleRGBScaling(int format);
        virtual void doPreProcessing(hwc_display_contents_1_t* contents);
        virtual bool isSourceCropfSupported(hwc_layer_1_t layer);
        virtual int getDeconDMAType(ExynosMPPModule* internalMPP);
        virtual bool isOverlaySupportedByIDMA(hwc_layer_1_t &layer, size_t index);
        virtual void getIDMAMinSize(hwc_layer_1_t &layer, int *w, int *h);
        bool checkConfigChanged(struct decon_win_config_data &lastConfigData, struct decon_win_config_data &newConfigData);
};
#endif
