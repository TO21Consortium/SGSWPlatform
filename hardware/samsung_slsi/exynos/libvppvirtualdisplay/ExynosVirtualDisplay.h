#ifndef EXYNOS_VPP_VIRTUALDISPLAY_H
#define EXYNOS_VPP_VIRTUALDISPLAY_H

#include "ExynosHWC.h"
#include "ExynosDisplay.h"
#include "../../exynos/kernel-3.10-headers/videodev2.h"
#ifdef USES_VDS_BGRA8888
#include "ExynosMPPModule.h"
#endif

#define NUM_FRAME_BUFFER 5
#define HWC_SKIP_RENDERING 0x80000000
#define MAX_BUFFER_COUNT 8

#define MAX_VIRTUALDISPLAY_VIDEO_LAYERS 1

class ExynosVirtualDisplay : public ExynosDisplay {
    public:
        /* Methods */
        ExynosVirtualDisplay(struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosVirtualDisplay();

        virtual int32_t getDisplayAttributes(const uint32_t attribute);
        virtual void configureWriteBack(hwc_display_contents_1_t *contents, decon_win_config_data &win_data);

        virtual int blank();
        virtual int unblank();

        virtual int getConfig();

        virtual int prepare(hwc_display_contents_1_t* contents);
        virtual int set(hwc_display_contents_1_t* contents);

        virtual void allocateLayerInfos(hwc_display_contents_1_t* contents);
        virtual void determineYuvOverlay(hwc_display_contents_1_t *contents);
        virtual void determineSupportedOverlays(hwc_display_contents_1_t *contents);
        virtual void determineBandwidthSupport(hwc_display_contents_1_t *contents);

        virtual void init(hwc_display_contents_1_t* contents);
        virtual void deInit();

        void setWFDOutputResolution(unsigned int width, unsigned int height, unsigned int disp_w, unsigned int disp_h);
#ifdef USES_VDS_OTHERFORMAT
        void setVDSGlesFormat(int format);
#endif
        void setPriContents(hwc_display_contents_1_t* contents);

        enum CompositionType {
            COMPOSITION_UNKNOWN = 0,
            COMPOSITION_GLES    = 1,
            COMPOSITION_HWC     = 2,
            COMPOSITION_MIXED   = COMPOSITION_GLES | COMPOSITION_HWC
        };

        unsigned int mWidth;
        unsigned int mHeight;
        unsigned int mDisplayWidth;
        unsigned int mDisplayHeight;

        bool mIsWFDState;
        bool mIsRotationState;

        bool mPresentationMode;
        unsigned int mDeviceOrientation;
        unsigned int mFrameBufferTargetTransform;

        CompositionType mCompositionType;
        CompositionType mPrevCompositionType;
        int mGLESFormat;
        int mSinkUsage;

    protected:
        void setSinkBufferUsage();
        void processGles(hwc_display_contents_1_t* contents);
        void processHwc(hwc_display_contents_1_t* contents);
        void processMixed(hwc_display_contents_1_t* contents);

        virtual void configureHandle(private_handle_t *handle, size_t index, hwc_layer_1_t &layer, int fence_fd, decon_win_config &cfg);
        virtual int postFrame(hwc_display_contents_1_t *contents);
#ifdef USES_VDS_BGRA8888
        virtual bool isBothMPPProcessingRequired(hwc_layer_1_t &layer);
#endif
        virtual void determineSkipLayer(hwc_display_contents_1_t *contents);
#ifdef USES_VDS_OTHERFORMAT
        virtual bool isSupportGLESformat();
#endif
        bool mIsSecureDRM;
        bool mIsNormalDRM;

        hwc_layer_1_t            *mOverlayLayer;
        hwc_layer_1_t            *mFBTargetLayer;
        hwc_layer_1_t            *mFBLayer[NUM_FRAME_BUFFER];
        size_t                   mNumFB;

        void calcDisplayRect(hwc_layer_1_t &layer);

#ifdef USES_DISABLE_COMPOSITIONTYPE_GLES
        ExynosMPPModule *mExternalMPPforCSC;
#endif

#ifdef USES_VDS_BGRA8888
        bool             mForceDoubleOperation;
        ExynosMPPModule *mExternalMPPforCSC;
#endif

#ifdef USE_VIDEO_EXT_FOR_WFD_DRM
    protected:
        int                      mSMemFd;
        bool                     mSMemProtected;

        void setMemoryProtection(int protection);

        int                      mReserveMemFd;
        bool                     mFlagIONBufferAllocated;
        bool checkIONBufferPrepared();
    public:
        void requestIONMemory();
        void freeIONMemory();
#endif

};

#endif
