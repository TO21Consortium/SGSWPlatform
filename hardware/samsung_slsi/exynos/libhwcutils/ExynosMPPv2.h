#ifndef EXYNOS_MPP_H
#define EXYNOS_MPP_H

#include "ExynosDisplay.h"
#include "MppFactory.h"
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <utils/Condition.h>
#include <utils/List.h>
#include <utils/String8.h>

class BufferFreeThread;

const size_t NUM_MPP_DST_BUFS = 3;
const size_t NUM_DRM_MPP_DST_BUFS = 2;
#define INTERNAL_MPP_DST_FORMAT HAL_PIXEL_FORMAT_RGBX_8888

#ifndef VPP_CLOCK
#define VPP_CLOCK       400
#endif
#ifndef VPP_VCLK
#define VPP_VCLK        133
#endif
#ifndef VPP_MIC_FACTOR
#define VPP_MIC_FACTOR  2
#endif

enum {
    eMPPUnsupportedDownScale      =     0x00000001,
    eMPPUnsupportedHW             =     0x00000002,
    eMPPUnsupportedRotation       =     0x00000004,
    eMPPUnsupportedCoordinate     =     0x00000008,
    eMPPUnsupportedDRMContents    =     0x00000010,
    eMPPUnsupportedS3DContents    =     0x00000020,
    eMPPUnsupportedBlending       =     0x00000040,
    eMPPUnsupportedFormat         =     0x00000080,
    eMPPNotAlignedDstSize         =     0x00000100,
    eMPPNotAlignedSrcCropPosition =     0x00000200,
    eMPPNotAlignedHStride         =     0x00000400,
    eMPPNotAlignedVStride         =     0x00000800,
    eMPPExceedHStrideMaximum      =     0x00001000,
    eMPPExceedVStrideMaximum      =     0x00002000,
    eMPPExeedMaxDownScale         =     0x00004000,
    eMPPExeedMaxDstWidth          =     0x00008000,
    eMPPExeedMaxDstHeight         =     0x00010000,
    eMPPExeedMinSrcWidth          =     0x00020000,
    eMPPExeedMinSrcHeight         =     0x00040000,
    eMPPExeedMaxUpScale           =     0x00080000,
    eMPPExeedSrcWCropMax          =     0x00100000,
    eMPPExeedSrcHCropMax          =     0x00200000,
    eMPPExeedSrcWCropMin          =     0x00400000,
    eMPPExeedSrcHCropMin          =     0x00800000,
    eMPPNotAlignedCrop            =     0x01000000,
    eMPPNotAlignedOffset          =     0x02000000,
    eMPPExeedMinDstWidth          =     0x04000000,
    eMPPExeedMinDstHeight         =     0x08000000,
    eMPPUnsupportedCompression    =     0x10000000,
    eMPPUnsupportedCSC            =     0x20000000,
};

enum {
    MPP_NONE = 0,
    MPP_M2M,
    MPP_LOCAL,
};

enum {
    MPP_MEM_MMAP = 1,
    MPP_MEM_USERPTR,
    MPP_MEM_OVERLAY,
    MPP_MEM_DMABUF,
};

enum {
    MPP_STATE_FREE = 0,
    MPP_STATE_ASSIGNED,
    MPP_STATE_TRANSITION,
};

enum {
    MPP_BUFFER_NORMAL = 0,
    MPP_BUFFER_NORMAL_DRM,
    MPP_BUFFER_SECURE_DRM,
    MPP_BUFFER_VIDEO_EXT,
};

class ExynosMPP {
	MppFactory *mppFact;
	LibMpp *libmpp;

    struct deleteBufferInfo {
        buffer_handle_t buffer;
        int bufFence;
    };

    public:
        /* Methods */
        ExynosMPP();
        ExynosMPP(ExynosDisplay *display, int gscIndex);
        ExynosMPP(ExynosDisplay *display, unsigned int mppType, unsigned int mppIndex);
        virtual ~ExynosMPP();
        void initMPP();
        const android::String8& getName() const;

        bool isSrcConfigChanged(exynos_mpp_img &c1, exynos_mpp_img &c2);
        bool formatRequiresMPP(int format);
        void setDisplay(ExynosDisplay *display);
        void preAssignDisplay(ExynosDisplay *display);
        bool isAssignable(ExynosDisplay *display);
        bool bufferChanged(hwc_layer_1_t &layer);
        bool needsReqbufs();
        bool inUse();
        void freeBuffers();
        void setAllocDevice(alloc_device_t* allocDevice);
        bool wasUsedByDisplay(ExynosDisplay *display);
        void startTransition(ExynosDisplay *display);
        bool checkNoExtVideoBuffer();
        void adjustSourceImage(hwc_layer_1_t &layer, exynos_mpp_img &srcImg);

        /* For compatibility with libhwc */
        bool isOTF();
        void cleanupOTF();

        /* Override these virtual functions in chip directory to handle per-chip differences */
        virtual bool isFormatSupportedByMPP(int format);
        virtual bool isCSCSupportedByMPP(int src_format, int dst_format, uint32_t dataSpace);
        virtual bool isProcessingRequired(hwc_layer_1_t &layer, int format);
        virtual int isProcessingSupported(hwc_layer_1_t &layer, int dst_format);
        virtual int processM2M(hwc_layer_1_t &layer, int dstFormat, hwc_frect_t *sourceCrop, bool needBufferAlloc = true);
        virtual int processM2MWithB(hwc_layer_1_t &layer1, hwc_layer_1_t &layer2, int dstFormat, hwc_frect_t *sourceCrop);
        virtual int setupInternalMPP();
        virtual void cleanupM2M();
        virtual void cleanupM2M(bool noFenceWait);
        virtual void cleanupInternalMPP();
        virtual int getMaxDownscale();
        virtual int getMaxUpscale();
        virtual int getMaxDownscale(hwc_layer_1_t &layer);
        virtual int getDstWidthAlign(int format);
        virtual int getDstHeightAlign(int format);
        virtual int getSrcXOffsetAlign(hwc_layer_1_t &layer);
        virtual int getSrcYOffsetAlign(hwc_layer_1_t &layer);
        virtual int getCropWidthAlign(hwc_layer_1_t &layer);
        virtual int getCropHeightAlign(hwc_layer_1_t &layer);
        virtual int getMaxWidth(hwc_layer_1_t &layer);
        virtual int getMaxHeight(hwc_layer_1_t &layer);
        virtual int getMinWidth(hwc_layer_1_t &layer);
        virtual int getMinHeight(hwc_layer_1_t &layer);
        virtual int getSrcWidthAlign(hwc_layer_1_t &layer);
        virtual int getSrcHeightAlign(hwc_layer_1_t &layer);
        virtual int getMaxCropWidth(hwc_layer_1_t &layer);
        virtual int getMaxCropHeight(hwc_layer_1_t &layer);
        virtual int configBlendMpp(void *handle, exynos_mpp_img *src,
                        exynos_mpp_img *dst,
                        struct SrcBlendInfo  *srcblendinfo);

        /* Fields */
        static int                      mainDisplayWidth;
        uint32_t                        mType;
        uint32_t                        mIndex;
        int                             mState;
        void                            *mMPPHandle;
        ExynosDisplay                   *mDisplay;
        ExynosDisplay                   *mPreAssignedDisplay;
        exynos_mpp_img                  mSrcConfig;
        exynos_mpp_img                  mMidConfig;
        exynos_mpp_img                  mDstConfig;
        buffer_handle_t                 mDstBuffers[NUM_MPP_DST_BUFS];
        buffer_handle_t                 mMidBuffers[NUM_MPP_DST_BUFS];
        int                             mDstBufFence[NUM_MPP_DST_BUFS];
        int                             mMidBufFence[NUM_MPP_DST_BUFS];
        size_t                          mCurrentBuf;
        intptr_t                        mLastMPPLayerHandle;
        int                             mS3DMode;
        bool                            mDoubleOperation;
        bool                            mCanRotate;
        bool                            mCanBlend;
        android::List<deleteBufferInfo > mFreedBuffers;
        BufferFreeThread                *mBufferFreeThread;
        android::Mutex                  mMutex;
        alloc_device_t*                 mAllocDevice;
        size_t                          mNumAvailableDstBuffers;
        size_t                          mBufferType;
        int                             mSMemFd;
        bool                            mCanBeUsed;
        uint32_t                        mAllocatedBufferNum;
        uint32_t                        mAllocatedMidBufferNum;
        bool                            mDstRealloc;
        bool                            mMidRealloc;

    protected:
        /* Methods */
        bool isDstConfigChanged(exynos_mpp_img &c1, exynos_mpp_img &c2);
        bool setupDoubleOperation(exynos_mpp_img &srcImg, exynos_mpp_img &midImg, hwc_layer_1_t &layer);
        int reallocateBuffers(private_handle_t *srcHandle, exynos_mpp_img &dstImg, exynos_mpp_img &midImg, bool needDoubleOperation, uint32_t index);
        void reusePreviousFrame(hwc_layer_1_t &layer);
        void freeBuffersCloseFences();

        /* Override these virtual functions in chip directory to handle per-chip differences */
        virtual void setupSrc(exynos_mpp_img &srcImg, hwc_layer_1_t &layer);
        virtual void setupMid(exynos_mpp_img &srcImg);
        virtual void setupDst(exynos_mpp_img &srcImg, exynos_mpp_img &dstImg, int dst_format, hwc_layer_1_t &layer);
        virtual void setupBlendCfg(exynos_mpp_img &srcImg, exynos_mpp_img &dstImg, hwc_layer_1_t &layer1,
                hwc_layer_1_t &layer2, struct SrcBlendInfo &srcBlendInfo);

        virtual int getMinCropWidth(hwc_layer_1_t &layer);
        virtual int getMinCropHeight(hwc_layer_1_t &layer);

        virtual int getMaxDstWidth(int format);
        virtual int getMaxDstHeight(int format);
        virtual int getMinDstWidth(int format);
        virtual int getMinDstHeight(int format);

        virtual void *createMPP(int id, int mode, int outputMode, int drm);
        virtual void *createBlendMPP(int id, int mode, int outputMode, int drm);
        virtual int configMPP(void *handle, exynos_mpp_img *src, exynos_mpp_img *dst);
        virtual int runMPP(void *handle, exynos_mpp_img *src, exynos_mpp_img *dst);
        virtual int stopMPP(void *handle);
        virtual void destroyMPP(void *handle);
        virtual int setCSCProperty(void *handle, unsigned int eqAuto, unsigned int fullRange, unsigned int colorspace);
        virtual int getBufferUsage(private_handle_t *srcHandle);
        virtual int freeMPP(void *handle);

    private:
        size_t  getBufferType(uint32_t usage);
        android::String8 mName;
};

class BufferFreeThread: public android::Thread {
    public:
        BufferFreeThread(ExynosMPP *exynosMPP): mRunning(false) {mExynosMPP = exynosMPP;};
        virtual bool threadLoop();
        bool mRunning;
        android::Condition mCondition;
    private:
        ExynosMPP *mExynosMPP;
};

size_t visibleWidth(ExynosMPP *processor, hwc_layer_1_t &layer, int format,
        int xres);

#endif
