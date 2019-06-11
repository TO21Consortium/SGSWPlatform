#ifndef EXYNOS_VIRTUAL_DISPLAY_H
#define EXYNOS_VIRTUAL_DISPLAY_H

#include "ExynosHWC.h"
#include "ExynosDisplay.h"

#define NUM_FB_TARGET 4
#define NUM_FRAME_BUFFER 5
#define HWC_SKIP_RENDERING 0x80000000
#define MAX_BUFFER_COUNT 8

class ExynosG2DWrapper;

class ExynosVirtualDisplay : public ExynosDisplay {
    public:
        /* Methods */
        ExynosVirtualDisplay(struct exynos5_hwc_composer_device_1_t *pdev);
        ~ExynosVirtualDisplay();

        virtual int prepare(hwc_display_contents_1_t* contents);
        virtual int set(hwc_display_contents_1_t* contents);
        virtual void calcDisplayRect(hwc_layer_1_t &layer);
        virtual void* getMappedAddrFBTarget(int fd);
        virtual void unmapAddrFBTarget();
        virtual int blank();
        virtual int getConfig();
        virtual int32_t getDisplayAttributes(const uint32_t attribute);
        bool isNewHandle(void *dstHandle);
        bool isLayerResized(hwc_layer_1_t *layer);
        bool isLayerFullSize(hwc_layer_1_t *layer);
        virtual void init();
        virtual void init(hwc_display_contents_1_t* contents);
        virtual void deInit();

        /* Fields */
        enum CompositionType {
            COMPOSITION_UNKNOWN = 0,
            COMPOSITION_GLES    = 1,
            COMPOSITION_HWC     = 2,
            COMPOSITION_MIXED   = COMPOSITION_GLES | COMPOSITION_HWC
        };

        struct FB_TARGET_INFO {
            int32_t         fd;
            void            *mappedAddr;
            int             mapSize;
        };

        unsigned int mWidth;
        unsigned int mHeight;
        unsigned int mDisplayWidth;
        unsigned int mDisplayHeight;

        bool mIsWFDState;
        bool mIsRotationState;
        bool mIsSecureDRM;
        bool mIsNormalDRM;
        buffer_handle_t mPhysicallyLinearBuffer;
        unsigned long mPhysicallyLinearBufferAddr;
        struct FB_TARGET_INFO fbTargetInfo[NUM_FB_TARGET];

        bool mPresentationMode;
        unsigned int mDeviceOrientation;
        unsigned int mFrameBufferTargetTransform;

        CompositionType mCompositionType;
        CompositionType mPrevCompositionType;
        int mGLESFormat;
        int mSinkUsage;

        ExynosMPPModule *mMPPs[1];
        ExynosG2DWrapper *mG2D;

        void* mDstHandles[MAX_BUFFER_COUNT];
        hwc_rect_t mPrevDisplayFrame;
        void* mPrevFbHandle[NUM_FRAME_BUFFER];
};

#endif
