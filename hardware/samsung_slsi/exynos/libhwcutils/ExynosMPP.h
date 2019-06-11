#ifndef EXYNOS_MPP_H
#define EXYNOS_MPP_H

#include "ExynosDisplay.h"
#include "MppFactory.h"
#include <utils/Mutex.h>
#include <utils/Thread.h>
#include <utils/Condition.h>
#include <utils/List.h>

#define FIRST_PRESCALER_THRESHOLD 4
#define SECOND_PRESCALER_THRESHOLD 8

class BufferFreeThread;
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
    eMPPExeedMinDstHeight         =     0x01000000,
};

class ExynosMPP {
	MppFactory *mppFact;
	LibMpp *libmpp;
    public:
        /* Methods */
        ExynosMPP();
        ExynosMPP(ExynosDisplay *display, int gscIndex);
        virtual ~ExynosMPP();

        static bool isSrcConfigChanged(exynos_mpp_img &c1, exynos_mpp_img &c2);
        virtual bool isFormatSupportedByGsc(int format);
        bool formatRequiresGsc(int format);
        static bool isFormatSupportedByGscOtf(int format);

        virtual bool rotationSupported(bool rotation);
        virtual bool paritySupported(int w, int h);

        virtual bool isProcessingRequired(hwc_layer_1_t &layer, int format);
        virtual int getDownscaleRatio(int *numerator, int *denominator);
        virtual int isProcessingSupported(hwc_layer_1_t &layer, int format, bool otf, int downNumerator = 1, int downDenominator = 16);
#ifdef USES_VIRTUAL_DISPLAY
        virtual int processM2M(hwc_layer_1_t &layer, int dstFormat, hwc_frect_t *sourceCrop, bool isNeedBufferAlloc = true);
#else
        virtual int processM2M(hwc_layer_1_t &layer, int dstFormat, hwc_frect_t *sourceCrop);
#endif
        virtual int processOTF(hwc_layer_1_t &layer);
        virtual void cleanupM2M();
        virtual void cleanupM2M(bool noFenceWait);
        virtual void cleanupOTF();

	virtual int getAdjustedCrop(int rawSrcSize, int dstSize, int format, bool isVertical, bool isRotated);

        bool isM2M();
        bool isOTF();
        virtual bool isUsingMSC();
        void setMode(int mode);
        void free();
        bool bufferChanged(hwc_layer_1_t &layer);
        bool needsReqbufs();
        bool inUse();
        void freeBuffers();

        /* Fields */
        uint32_t                        mIndex;
        void                            *mGscHandle;
        bool                            mNeedReqbufs;
        int                             mWaitVsyncCount;
        int                             mCountSameConfig;
        ExynosDisplay                   *mDisplay;
        exynos_mpp_img                  mSrcConfig;
        exynos_mpp_img                  mMidConfig;
        exynos_mpp_img                  mDstConfig;
        buffer_handle_t                 mDstBuffers[NUM_GSC_DST_BUFS];
        buffer_handle_t                 mMidBuffers[NUM_GSC_DST_BUFS];
        int                             mDstBufFence[NUM_GSC_DST_BUFS];
        int                             mMidBufFence[NUM_GSC_DST_BUFS];
        size_t                          mCurrentBuf;
        int                             mGSCMode;
        ptrdiff_t                       mLastGSCLayerHandle;
        int                             mS3DMode;
        bool                            mDoubleOperation;
        android::List<buffer_handle_t > mFreedBuffers;
        BufferFreeThread                *mBufferFreeThread;
        android::Mutex                  mMutex;
        size_t                          mNumAvailableDstBuffers;

    protected:
        /* Methods */
        static bool isDstConfigChanged(exynos_mpp_img &c1, exynos_mpp_img &c2);
        static bool isReallocationRequired(int w, int h, exynos_mpp_img &c1, exynos_mpp_img &c2);
        static int minWidth(hwc_layer_1_t &layer);
        static int minHeight(hwc_layer_1_t &layer);
        int maxWidth(hwc_layer_1_t &layer);
        int maxHeight(hwc_layer_1_t &layer);
        static int srcXOffsetAlign(hwc_layer_1_t &layer);
        static int srcYOffsetAlign(hwc_layer_1_t &layer);

        virtual void setupSource(exynos_mpp_img &src_img, hwc_layer_1_t &layer);
        void setupOtfDestination(exynos_mpp_img &src_img, exynos_mpp_img &dst_img, hwc_layer_1_t &layer);
        int reconfigureOtf(exynos_mpp_img *src_img, exynos_mpp_img *dst_img);

        virtual void setupM2MDestination(exynos_mpp_img &src_img, exynos_mpp_img &dst_img, int dst_format, hwc_layer_1_t &layer, hwc_frect_t *sourceCrop);
        bool setupDoubleOperation(exynos_mpp_img &src_img, exynos_mpp_img &mid_img, hwc_layer_1_t &layer);
        int reallocateBuffers(private_handle_t *src_handle, exynos_mpp_img &dst_img, exynos_mpp_img &mid_img, bool need_gsc_op_twice);

        /*
         * Override these virtual functions in chip directory to handle per-chip differences
         * as well as GSC/FIMC differences
         */
        virtual uint32_t halFormatToMPPFormat(int format);

        virtual int sourceWidthAlign(int format);
        virtual int sourceHeightAlign(int format);
        virtual int destinationAlign(int format);
        virtual bool isSourceCropChanged(exynos_mpp_img &c1, exynos_mpp_img &c2);
        virtual bool isPerFrameSrcChanged(exynos_mpp_img &c1, exynos_mpp_img &c2);
        virtual bool isPerFrameDstChanged(exynos_mpp_img &c1, exynos_mpp_img &c2);

        virtual void *createMPP(int id, int mode, int outputMode, int drm);
        virtual int configMPP(void *handle, exynos_mpp_img *src, exynos_mpp_img *dst);
        virtual int runMPP(void *handle, exynos_mpp_img *src, exynos_mpp_img *dst);
        virtual int stopMPP(void *handle);
        virtual void destroyMPP(void *handle);
        virtual int setCSCProperty(void *handle, unsigned int eqAuto, unsigned int fullRange, unsigned int colorspace);
	virtual int freeMPP(void *handle);
        virtual int setInputCrop(void *handle, exynos_mpp_img *src, exynos_mpp_img *dst);
        virtual bool isSrcCropAligned(hwc_layer_1_t &layer, bool rotation);
};

size_t visibleWidth(ExynosMPP *processor, hwc_layer_1_t &layer, int format,
        int xres);

class BufferFreeThread: public android::Thread {
    public:
        BufferFreeThread(ExynosMPP *exynosMPP): mRunning(false) {mExynosMPP = exynosMPP;};
        virtual bool threadLoop();
        bool mRunning;
        android::Condition mCondition;
    private:
        ExynosMPP *mExynosMPP;
};

#endif
