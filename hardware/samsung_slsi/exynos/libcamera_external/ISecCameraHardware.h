/*
 * Copyright 2008, The Android Open Source Project
 * Copyright 2013, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 /*!
 * \file      ISecCameraHardware.h
 * \brief     source file for Android Camera Ext HAL
 * \author    teahyung kim (tkon.kim@samsung.com)
 * \date      2013/04/30
 *
 */

#ifndef ANDROID_HARDWARE_ISECCAMERAHARDWARE_H
#define ANDROID_HARDWARE_ISECCAMERAHARDWARE_H

/*
 * Common Header file
 */
#include <utils/List.h>
#include <utils/threads.h>
#include <utils/RefBase.h>
#include <cutils/compiler.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <ui/GraphicBufferMapper.h>
#include <hardware/camera.h>
#include <hardware/gralloc.h>
#include <camera/CameraParameters.h>
#include <cutils/properties.h>
#include <utils/Log.h>

#ifndef HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_FULL
#define HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_FULL  (HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL) /* 0x11E, */
#endif

#ifndef HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP
#define HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP  (HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M) /* 0x11D, */
#endif

/* use exynos feature */
#include <ion/ion.h>
#include "ExynosBuffer.h"
#include "exynos_v4l2.h"
#include "exynos_format.h"
#include "csc.h"
#include "gralloc_priv.h"

/*
 * Model-specific Header file
 */
#include "model/include/SecCameraHardware-model.h"
#include "SecCameraCommonConfig.h"
#include "SecCameraParameters.h"

#include "ExynosCameraAutoTimer.h"
#include "ExynosCameraList.h"

/*
 * Define debug feature
 */
#ifdef CAMERA_DEBUG_ALL
#define DEBUG_THREAD_EXIT_WAIT
#define DEBUG_PREVIEW_CALLBACK
#define DEBUG_CAPTURE_RETRY
#endif

//#define SAVE_DUMP

#define SPARE_SIZE  (32 * 1024)
#define SPARE_PLANE (1)

#define FD_SERVICE_CAMERA_ID            2

#define DUMP_FIRST_PREVIEW_FRAME

#define USE_USERPTR
#define USE_CAPTURE_MODE
//#define USE_VIDIOC_ENUM_FORMAT
//#define USE_VIDIOC_QUERY_CAP
#define NUM_FLITE_BUF_PLANE (1)
#define FLITE_BUF_CNT (8)
#define PREVIEW_BUF_CNT (8)
#define REC_BUF_CNT (8)
#define REC_PLANE_CNT (2)
#define SKIP_CAPTURE_CNT (1)
#define NUM_OF_DETECTED_FACES (16)

#define NUM_OF_AE_WINDOW_ROW        (17)
#define NUM_OF_AE_WINDOW_COLUMN     (13)
#define SIZE_OF_AE_WINDOW_ROW       (1)
#define SIZE_OF_AE_WINDOW_COLUMN    (1)

#if !defined(LOG_NFPS) || LOG_NFPS
#define LOG_FPS(...)    ((void)0)
#else
#define LOG_FPS(...)    ((void)ALOG(LOG_DEBUG, LOG_TAG, __VA_ARGS__))
#endif

#if !defined(LOG_NPERFORMANCE) || LOG_NPERFORMANCE
#define LOG_PERFORMANCE_START(n)        ((void)0)
#define LOG_PERFORMANCE_END(n, tag)     ((void)0)
#else
#define LOG_PERFORMANCE_START(n) \
    struct timeval time_start_##n, time_stop_##n; \
    gettimeofday(&time_start_##n, NULL)

#define LOG_PERFORMANCE_END(n, tag) \
    gettimeofday(&time_stop_##n, NULL); \
    ALOGD("%s: %s %ld us", __FUNCTION__, tag, \
         (time_stop_##n.tv_sec * 1000000 + time_stop_##n.tv_usec) \
         - (time_start_##n.tv_sec * 1000000 + time_start_##n.tv_usec))
#endif

#define LOG_RECORD_TRACE(fmt, ...) \
    if (CC_UNLIKELY(mRecordingTrace)) { \
        ALOGI("%s: " fmt, __FUNCTION__, ##__VA_ARGS__); \
    }

#define HDR_BUF_CNT    3
#define CAP_CNT_MAGIC  32

#define HWC_ON

//#define ALLOCATION_REC_BUF_BY_MEM_CB

#define CLEAR(x)    memset(&(x), 0, sizeof(x))

namespace android {

/* cmdType in sendCommand functions */
enum {
    CAMERA_CMD_SET_TOUCH_AF_POSITION    = 1503, // CAMERA_CMD_SET_TOUCH_AF_POSITION    = 1103,
    HAL_OBJECT_TRACKING_STARTSTOP       = 1504,
    CAMERA_CMD_START_STOP_TOUCH_AF      = 1505,
    CAMERA_CMD_DISABLE_POSTVIEW         = 1109,
    CAMERA_CMD_SET_FLIP                 = 1510,
    CAMERA_CMD_SET_AEAWB_LOCk           = 1111,
    RECORDING_TAKE_PICTURE              = 1201,

    HAL_START_CONTINUOUS_AF = 1551,
    HAL_STOP_CONTINUOUS_AF = 1552,
    HAL_AF_LAMP_CONTROL = 1555,
};

enum {
    CAMERA_BURST_MEMORY_NONE= 0,
    CAMERA_BURST_MEMORY_ION = 1,
    CAMERA_BURST_MEMORY_HEAP = 2,
};

enum {
    CAMERA_BURST_STOP_NONE=0,
    CAMERA_BURST_STOP_REQ,
    CAMERA_BURST_STOP_PROC,
    CAMERA_BURST_STOP_END,
};

enum {
    CAMERA_HEAP_PREVIEW = 0,
    CAMERA_HEAP_POSTVIEW,
};

enum {
    PREVIEW_THREAD_NONE = 0,
    PREVIEW_THREAD_NORMAL,
    PREVIEW_THREAD_CHANGED,
};

/* Structure for Focus Area */
typedef struct FocusArea {
    int top;
    int left;
    int bottom;
    int right;
    int weight;
} FocusArea;

typedef struct FocusPoint {
    int x;
    int y;
} FocusPoint;

typedef struct node_info {
    int fd;
    int width;
    int height;
    int format;
    int planes;
    int buffers;
    enum v4l2_memory   memory;
    enum v4l2_buf_type type;
    int         ionClient;
    ExynosBuffer       buffer[VIDEO_MAX_FRAME];
    bool               flagStart;
} node_info_t;

/* for recording */
typedef struct rec_src_buf {
    ExynosBuffer *buf;
    nsecs_t timestamp;
} rec_src_buf_t;

/* for MC */
enum  {
    CAMERA_EXT_PREVIEW,
    CAMERA_EXT_CAPTURE_YUV,
    CAMERA_EXT_CAPTURE_JPEG
};

struct addrs {
    /* make sure that this is 4 byte. */
    uint32_t type;
    unsigned int fd_y;
    unsigned int fd_cbcr;
    unsigned int buf_index;
    unsigned int reserved;
};

struct addrs_cap {
    unsigned int addr_y;
    unsigned int width;
    unsigned int height;
};

class ISecCameraHardware : public virtual RefBase {
public:
    virtual status_t setPreviewWindow(preview_stream_ops *w);

    virtual void        setCallbacks(camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void* user) {
        Mutex::Autolock lock(mLock);
        mNotifyCb = notify_cb;
        mDataCb = data_cb;
        mDataCbTimestamp = data_cb_timestamp;
        mGetMemoryCb = get_memory;
        mCallbackCookie = user;
    }

    virtual void        enableMsgType(int32_t msgType) {
        /* ALOGV("%s: msg=0x%X, ++msg=0x%X", __FUNCTION__, mMsgEnabled, msgType); */
        Mutex::Autolock lock(mLock);
        mMsgEnabled |= msgType;
    }
    virtual void        disableMsgType(int32_t msgType) {
        /* ALOGV("%s: msg=0x%X, --msg=0x%X", __FUNCTION__, mMsgEnabled, msgType); */
        Mutex::Autolock lock(mLock);
#ifdef DEBUG_PREVIEW_CALLBACK
        if (msgType & CAMERA_MSG_PREVIEW_FRAME) {
            if (true == mPreviewCbStarted) {
                ALOGD("disable ongoing preview callbacks");
            }
            mPreviewCbStarted = false;
        }
#endif
        mMsgEnabled &= ~msgType;
    }
    virtual bool        msgTypeEnabled(int32_t msgType) {
        Mutex::Autolock lock(mLock);
        return (mMsgEnabled & msgType);
    }

    /* Preview */
    virtual status_t    startPreview();
    virtual void        stopPreview();
    virtual bool        previewEnabled() {
        Mutex::Autolock lock(mLock);
        return mPreviewRunning;
    }

    virtual status_t    storeMetaDataInBuffers      (bool enable);

    /* Recording */
    virtual status_t    startRecording();
    virtual void        stopRecording();
    virtual bool        recordingEnabled() {
        Mutex::Autolock lock(mLock);
        return mRecordingRunning;
    }

    virtual void        releaseRecordingFrame(const void *opaque);

    /* Auto Focus*/
    virtual status_t    autoFocus();
    virtual status_t    cancelAutoFocus();

    /* Picture */
    virtual status_t    takePicture();
    virtual status_t    cancelPicture();

    /* Raw Capture */
    virtual status_t    pictureThread_RAW();

    /* Parameter */
    virtual status_t    setParameters(const CameraParameters& params);
    virtual CameraParameters getParameters() const {
        Mutex::Autolock lock(mLock);
        return mParameters;
    }
    virtual status_t    sendCommand(int32_t command, int32_t arg1, int32_t arg2);

    virtual void        release();
    virtual status_t    dump(int fd) const;
    virtual int         getCameraId() const;
    bool    IsAutofocusRunning() { return mAutoFocusRunning; };

protected:
    int                 mCameraId;
    CameraParameters    mParameters;

    bool                mFlagANWindowRegister;
    bool                mPreviewInitialized;
#ifdef DEBUG_PREVIEW_CALLBACK
    bool                mPreviewCbStarted;      /* for debugging */
#endif

    camera_memory_t     *mPreviewHeap;
    int                 mPreviewHeapFd;
    camera_memory_t     *mPostviewHeap;
    int                 mPostviewHeapFd;
    camera_memory_t     *mPostviewHeapTmp;
    int                 mPostviewHeapTmpFd;

    camera_memory_t     *mRawHeap;
    camera_memory_t     *mRecordingHeap;
    int                 mRecordHeapFd;
    camera_memory_t     *mJpegHeap;
    ExynosBuffer        mPictureBuf;
    ExynosBuffer        mPictureBufDummy[SKIP_CAPTURE_CNT];
    camera_frame_metadata_t  mFrameMetadata;
    camera_face_t            mFaces[NUM_OF_DETECTED_FACES];
    int                 mJpegHeapFd;
    int                 mRawHeapFd;
    int                 mHDRHeapFd;

#if FRONT_ZSL
    camera_memory_t     *mFullPreviewHeap;
    int                 mZSLindex;
    sp<MemoryHeapBase>  rawImageMem;
#endif
    bool                mFullPreviewRunning; /* for FRONT_ZSL */

    uint32_t            mPreviewFrameSize;
    uint32_t            mRecordingFrameSize;
    uint32_t            mRawFrameSize;
    uint32_t            mPostviewFrameSize;
    uint32_t            mPictureFrameSize;
    uint32_t            mRawThumbnailSize;
#ifdef RECORDING_CAPTURE
    uint32_t            mRecordingPictureFrameSize;
#endif
#if FRONT_ZSL
    uint32_t            mFullPreviewFrameSize;
#endif
    camera_memory_t     *mHDRHeap;
#ifndef RCJUNG
    camera_memory_t     *mYUVHeap;
#endif
    uint32_t            mHDRFrameSize;

    image_rect_type     mPreviewSize;
    image_rect_type     mOrgPreviewSize;
    image_rect_type     mPreviewWindowSize;
    image_rect_type     mRawSize;
    image_rect_type     mPictureSize;
    image_rect_type     mThumbnailSize;
    image_rect_type     mVideoSize;
    image_rect_type     mSensorSize;
    image_rect_type     mFLiteSize; /* for FLite */
    image_rect_type     mFLiteCaptureSize; /* for FLite during capture */
    image_rect_type     mPostviewSize;

    cam_pixel_format    mPreviewFormat;
    cam_pixel_format    mPictureFormat;
    cam_pixel_format    mFliteFormat;

    int                 mJpegQuality;
    int                 mFrameRate;
    int                 mFps;
    int                 mflipHorizontal;
    int                 mflipVertical;
    cam_scene_mode      mSceneMode;
    cam_flash_mode      mFlashMode;
    cam_focus_mode      mFocusMode;
    cam_focus_area      mFocusArea;
    bool                mPrevMovieMode;
    bool                mMovieMode;
    bool                mZoomSupport;
    bool                mEnableDZoom;
    bool                mFastCaptureSupport;

    bool                mNeedSizeChange;
    bool                mFastCaptureCalled;

    int                mMultiFullCaptureNum;

    int                mCaptureMode;
    int                 mFirstStart;
    int                 mTimerSet;
    int                 mExifFnum;
    int                 mExifShutterSpeed;
    int                 mExifLightSource;
    int                 mWeather;
    long long int       mCityId;

    int                 mZoomParamSet;
    int                 mZoomSetVal;
    int                 mZoomActionMethod;
    int                 mZoomStatus;
    int                 mZoomStatusBak;
    int                 mLensStatus;
    int                 mLensChecked;

    bool                mCameraPower;

    char                mAntiBanding[10];
    int                mIonCameraClient;
    int                zoomRatioList[MAX_ZOOM_LEVEL];

    ISecCameraHardware(int cameraId, camera_device_t *dev);
    virtual ~ISecCameraHardware();

    bool                initialized(sp<MemoryHeapBase> heap) const {
        return heap != NULL && heap->base() != MAP_FAILED;
    }

    virtual bool        init();
    virtual void        initDefaultParameters();

    virtual status_t    nativeSetParameters(cam_control_id id, int value,
                                            bool recordingMode = false) = 0;
    virtual status_t    nativeGetParameters(cam_control_id id, int *value,
                                            bool recordingMode = false) = 0;

    virtual image_rect_type nativeGetWindowSize() = 0;
    virtual status_t    nativeStartPreview() = 0;
    virtual status_t    nativeStartPreviewZoom() = 0;
    virtual int         nativeGetPreview() = 0;
    virtual int         nativeReleasePreviewFrame(int index) = 0;
    virtual void        nativeStopPreview() = 0;
#if FRONT_ZSL
    virtual status_t    nativeStartFullPreview() = 0;
    virtual int         nativeGetFullPreview() = 0;
    virtual int         nativeReleaseFullPreviewFrame(int index) = 0;
    virtual void        nativeStopFullPreview() = 0;
    virtual void        nativeForceStopFullPreview() = 0;
#endif

    virtual status_t    nativeSetZoomRatio(int value) = 0;
    virtual status_t    nativePreviewCallback(int index, ExynosBuffer *grallocBuf) = 0;
    virtual status_t    nativeCSCPreview(int index, int type) = 0;
    virtual status_t    nativeStartRecording() = 0;
    virtual status_t    nativeCSCRecording(rec_src_buf_t *srcBuf, int dstIndex) = 0;
    virtual status_t    nativeStartRecordingZoom() = 0;
    virtual void        nativeStopRecording() = 0;
#ifdef RECORDING_CAPTURE
    virtual bool        nativeGetRecordingJpeg(ExynosBuffer *yuvBuf,
                                               uint32_t width, uint32_t height) = 0;
#endif
    virtual bool        nativeSetAutoFocus() = 0;
    virtual int         nativeGetPreAutoFocus() = 0;
    virtual int         nativeGetAutoFocus() = 0;
    virtual status_t    nativeCancelAutoFocus() = 0;

    virtual bool        nativeStartYUVSnapshot() = 0;
    virtual bool        nativeGetYUVSnapshot(int numF, int *postviewOffset) = 0;
    virtual bool        nativeStartSnapshot() = 0;
    virtual bool        nativeStartPostview() = 0;
    virtual void        nativeMakeJpegDump() = 0;
    virtual bool        nativeGetSnapshot(int numF, int *postviewOffset) = 0;
    virtual bool        nativeGetPostview(int numF) = 0;
    virtual void        nativeStopSnapshot() = 0;
    virtual bool        nativeStartDualCapture(int numF) = 0;
    virtual status_t    nativeCSCCapture(ExynosBuffer *srcBuf, ExynosBuffer *dstBuf) = 0;
    virtual status_t    nativeCSCRecordingCapture(ExynosBuffer *srcBuf, ExynosBuffer *dstBuf) = 0;

    virtual int        nativegetWBcustomX() = 0;
    virtual int        nativegetWBcustomY() = 0;

    virtual int        nativeSetFastCapture(bool onOff) = 0;

    virtual bool        nativeCreateSurface(uint32_t width, uint32_t height, uint32_t halPixelFormat) = 0;
    virtual bool        nativeDestroySurface(void) = 0;
    virtual bool        nativeFlushSurfaceYUV420(uint32_t width, uint32_t height,
                                                 uint32_t size, uint32_t index,
                                                 int type = CAMERA_HEAP_POSTVIEW) = 0;
    virtual bool        nativeFlushSurface(uint32_t width, uint32_t height,
                                           uint32_t size, uint32_t index,
                                           int type=CAMERA_HEAP_PREVIEW) = 0;
    virtual bool        beautyLiveFlushSurface(uint32_t width, uint32_t height,
                                               uint32_t size, uint32_t index,
                                               int type=CAMERA_HEAP_PREVIEW) = 0;
 
#ifdef RECORDING_CAPTURE
    virtual bool        conversion420to422(uint8_t *src, uint8_t *dest, int width, int height) = 0;
    virtual bool        conversion420Tto422(uint8_t *src, uint8_t *dest, int width, int height) = 0;
#endif

    virtual int         dumpToFile(void *buf, uint32_t size, char *filePath) {
        FILE *fd = NULL;

        fd = fopen(filePath, "wb");
        if (!fd) {
            ALOGE("dumpToFile: error, fail to open %s", filePath);
            return -1;
        }
        size_t nwrite = fwrite(buf, sizeof(char), size, fd);
        fclose(fd);
        ALOGD("dumped: %s (size=%dbytes)", filePath, nwrite);

        return 0;
    }

    virtual int getMaxZoomLevel(void) = 0;
    virtual int getMaxZoomRatio(void) = 0;
    virtual float getZoomRatio(int zoomLevel) = 0;

private:
    typedef bool (ISecCameraHardware::*thread_loop)(void);
    class CameraThread : public Thread {
        ISecCameraHardware *mHardware;
        thread_loop     mThreadLoop;
        char            mName[32];
#ifdef DEBUG_THREAD_EXIT_WAIT
        pid_t           threadId;
#endif
        bool            mExitRequested;
        struct timeval mTimeStart;
        struct timeval mTimeStop;
    public:
        CameraThread(ISecCameraHardware *hw, thread_loop loop,
                     const char *name = "camera:unamed_thread") :
#ifdef SINGLE_PROCESS
            /* In single process mode this thread needs to be a java thread,
               since we won't be calling through the binder.
             */
            Thread(true),
#else
            Thread(false),
#endif
            mHardware(hw),
            mThreadLoop(loop),
#ifdef DEBUG_THREAD_EXIT_WAIT
            threadId(-1),
#endif
            mExitRequested(false) {
            memset(&mTimeStart, 0, sizeof(mTimeStart));
            memset(&mTimeStop, 0, sizeof(mTimeStop));
            CLEAR(mName);
            if (name)
                strncpy(mName, name, sizeof(mName) - 1);
        }

        virtual status_t run(const char *name = 0,
            int32_t priority = PRIORITY_DEFAULT,
            size_t stack = 0) {
            memset(&mTimeStart, 0, sizeof(mTimeStart));
            memset(&mTimeStop, 0, sizeof(mTimeStop));
            mExitRequested = false;

            status_t err = Thread::run(mName, priority, stack);
            if (CC_UNLIKELY(err)) {
                mExitRequested = true;
            }
#if defined(DEBUG_THREAD_EXIT_WAIT) && defined(HAVE_ANDROID_OS)
            else {
                threadId = getTid();
                ALOGV("Thread started (%s %d)", mName, threadId);
            }
#endif

            return err;
        }

        virtual void requestExit() {
            mExitRequested = true;
            Thread::requestExit();
        }

        status_t requestExitAndWait() {
#if defined(DEBUG_THREAD_EXIT_WAIT) && defined(HAVE_ANDROID_OS)
            const int waitSliceTime = 5; /* 5ms */
            int timeOutMsec = 200; /* 200ms */

            if (timeOutMsec < waitSliceTime)
                timeOutMsec = waitSliceTime;

            int waitCnt = timeOutMsec / waitSliceTime;
            if (timeOutMsec % waitSliceTime)
                waitCnt++;

            requestExit();
            ALOGD("request thread exit (%s)", mName);

            pid_t tid = -1;
            int cnt;

            for (cnt = 0; cnt <= waitCnt; cnt++) {
                tid = getTid();
                if (-1 == tid)
                    break;

                if (!((cnt + 1) % 4)) {
                    ALOGV("wait for thread to be finished (%s). %d %d (%d)", mName, threadId, tid, cnt + 1);
                }
                usleep(waitSliceTime * 1000);
            }

            if (-1 == tid)
                ALOGD("thread exit %s (%s)", !cnt ? "already" : "", mName);
            else
                ALOGD("request thread exit again, blocked (%s)", mName);
#else
            mExitRequested = true;
#endif
            return Thread::requestExitAndWait();
        }

        bool exitRequested() {
            return mExitRequested;
        }
        void calcFrameWaitTime(int maxFps) {
            /* Calculate how long to wait between frames */
            if (mTimeStart.tv_sec == 0 && mTimeStart.tv_usec == 0) {
                gettimeofday(&mTimeStart, NULL);
            } else {
                gettimeofday(&mTimeStop, NULL);
                unsigned long timeUs = (mTimeStop.tv_sec * 1000000 + mTimeStop.tv_usec)
                    - (mTimeStart.tv_sec*1000000 + mTimeStart.tv_usec);
                gettimeofday(&mTimeStart, NULL);
                unsigned long framerateUs = 1000.0 / maxFps * 1000000;
                unsigned long delay = framerateUs > timeUs ? framerateUs - timeUs : 0;
                LOG_FPS("%s: time %ld us, delay %ld us", mName, timeUs, delay);
                usleep(delay);
            }
        }

    private:
        virtual bool threadLoop() {
            /* loop until we need to quit */
            return (mHardware->*mThreadLoop)();
        }
    };

    mutable Mutex       mLock;
    mutable Mutex       mPictureLock;
    mutable Mutex       mBurstThreadLock;

    mutable Mutex       mAutoFocusLock;
    mutable Condition   mAutoFocusCondition;
    bool mAutoFocusExit;

    mutable Mutex       mBurstShotLock;
    mutable Condition   mBurstShotCondition;
    bool                mBurstShotExit;

    bool                mPreviewRunning;
    bool                mRecordingRunning;
    bool                mAutoFocusRunning;
    bool                mPictureRunning;
    bool                mRecordingTrace;
    bool                mPictureStart;
    bool                mCaptureStarted;
    bool                mCancelCapture;
    bool                mPreviewFrameReceived;

    int                 roi_x_pos;
    int                 roi_y_pos;
    int                 roi_width;
    int                 roi_height;

    /* protected by mLock */
    sp<CameraThread>    mPreviewThread;
    sp<CameraThread>    mRecordingThread;
    sp<CameraThread>    mAutoFocusThread;
    sp<CameraThread>    mPictureThread;

    sp<CameraThread>    mHDRPictureThread; 
    sp<CameraThread>    mRecordingPictureThread;
    sp<CameraThread>    mDumpPictureThread;

#if FRONT_ZSL
    sp<CameraThread>    mZSLPictureThread;
#endif

    /* Thread for zoom */
    sp<CameraThread>    mPostRecordThread;
    sp<CameraThread>    mPreviewZoomThread;
#ifdef USE_DEDICATED_PREVIEW_ENQUEUE_THREAD
    sp<CameraThread>    mPreviewEnqueueThread;
#endif

protected:
    int32_t                 mMsgEnabled;
    camera_request_memory   mGetMemoryCb;
    preview_stream_ops      *mPreviewWindow;
    camera_notify_callback  mNotifyCb;
    camera_data_callback    mDataCb;
    camera_data_timestamp_callback mDataCbTimestamp;
    void                    *mCallbackCookie;
    node_info_t             mFliteNode;

    /* for recording */
    mutable Mutex       mRecordSrcLock;
    List<rec_src_buf_t*> mRecordSrcQ;
    int                 mRecordSrcIndex;
    rec_src_buf_t       mRecordSrcSlot[FLITE_BUF_CNT];
    mutable Mutex       mRecordDstLock;
    int                 mRecordDstIndex;
    camera_memory_t     *mRecordDstHeap[REC_BUF_CNT][REC_PLANE_CNT];
    ExynosBuffer        mRecordingDstBuf[REC_BUF_CNT];
    int                 mRecordDstHeapFd[REC_BUF_CNT][REC_PLANE_CNT];
    bool                mRecordFrameAvailable[REC_BUF_CNT];
    int                 mRecordFrameAvailableCnt;

#ifdef USE_DEDICATED_PREVIEW_ENQUEUE_THREAD
    bool            m_flagEnqueueThreadStop;
    ExynosCameraList<buffer_handle_t *>    m_previewFrameQ;
#endif

    /* Focus Areas values */
    static const ssize_t FOCUS_AREA_TOP = -1000;
    static const ssize_t FOCUS_AREA_LEFT = -1000;
    static const ssize_t FOCUS_AREA_BOTTOM = 1000;
    static const ssize_t FOCUS_AREA_RIGHT = 1000;
    static const ssize_t FOCUS_AREA_WEIGHT_MIN = 1;
    static const ssize_t FOCUS_AREA_WEIGHT_MAX = 1000;
    bool        allocMemSinglePlane(int ionClient, ExynosBuffer *buf,
                                    int index, bool flagCache = true);
    void        freeMemSinglePlane(ExynosBuffer *buf, int index);
    bool        allocMem(int ionClient, ExynosBuffer *buf, int cacheIndex = 0xff);
    void        freeMem(ExynosBuffer *buf);

    /* for recording */
    void        mInitRecSrcQ(void);
    void        mInitRecDstBuf(void);
    int         getRecSrcBufSlotIndex(void);
    void        mPushRecSrcQ(rec_src_buf_t *buf);
    bool        mPopRecSrcQ(rec_src_buf_t *buf);
    int         mSizeOfRecSrcQ(void);
    int         getRecDstBufIndex(void);
    void        setAvailDstBufIndex(int index);
    int         getAlignedYUVSize(int colorFormat, int w, int h, ExynosBuffer *buf,
                                  bool flagAndroidColorFormat = false);

private:

    int                 mDropFrameCount;
    bool                mbFirst_preview_started;
    int                 mMaxFrameRate;

    bool                mDisablePostview;
    int                 mLastIndex;
#ifdef DUMP_FIRST_PREVIEW_FRAME
    int                 mFlagFirstFrameDump;
#endif
    int                 mAntibanding60Hz;
    int                 mFaceDetectionStatus;
    float               m_calculatedHorizontalViewAngle;

    void        setDropFrame(int count);
    void        setDropUnstableInitFrames();

    bool        autoFocusCheckAndWaitPreview() {
        for (int i = 0; i < 50; i++) {
            if (mPreviewInitialized)
                return true;

            usleep(10000); /* 10ms*/
        }

        return false;
    }
    status_t    setAELock(const CameraParameters& params);
    status_t    setAWBLock(const CameraParameters& params);
    status_t    setRecordingMode(const CameraParameters& params);
    status_t    setPreviewSize(const CameraParameters& params);
    status_t    setPreviewFormat(const CameraParameters& params);
    status_t    setPictureSize(const CameraParameters& params);
    status_t    setPictureFormat(const CameraParameters& params);
    status_t    setThumbnailSize(const CameraParameters& params);
    status_t    setJpegThumbnailQuality(const CameraParameters& params);
    status_t    setJpegQuality(const CameraParameters& params);
    status_t    setFrameRate(const CameraParameters& params);
    status_t    setRotation(const CameraParameters& params);
    status_t    setVideoSize(const CameraParameters& params);
    status_t    setPreviewFrameRate(const CameraParameters& params);

    status_t    setSceneMode(const CameraParameters& params);

    /* Focus Area start */
    status_t    findCenter(struct FocusArea *focusArea,
                       struct FocusPoint *center);
    status_t    normalizeArea(struct FocusPoint *center);
    status_t    checkArea(ssize_t top, ssize_t left, ssize_t bottom,
                       ssize_t right, ssize_t weight);
    status_t    parseAreas(const char *area, size_t areaLength,
    struct      FocusArea *focusArea, int *num_areas);
    status_t    setFocusAreas(const CameraParameters& params);
    /* Focus Area end */

    status_t    setIso(const CameraParameters& params);
    status_t    setBrightness(const CameraParameters& params);
    status_t    setWhiteBalance(const CameraParameters& params);
    status_t    setFlash(const CameraParameters& params);
    status_t    setMetering(const CameraParameters& params);
    status_t    setMeteringAreas(const CameraParameters& params);
    status_t    setFocusMode(const CameraParameters& params);
    status_t    setEffect(const CameraParameters& params);
    status_t    setZoom(const CameraParameters& params);
    status_t    setSharpness(const CameraParameters& params);
    status_t    setSaturation(const CameraParameters& params);
    status_t    setContrast(const CameraParameters& params);
    status_t    setColorAdjust(const CameraParameters& params);
    status_t    setAntiShake(const CameraParameters& params);
    status_t    setBlur(const CameraParameters& params);
    status_t    setAntiBanding();
    status_t    setAntiBanding(const CameraParameters& params);
    status_t    setGps(const CameraParameters& params);
    int        checkFnumber(int f_val, int zoomLevel);
    status_t    setDzoom(const CameraParameters& params);
    status_t    doCameraCapture();
#ifdef RECORDING_CAPTURE
    status_t    doRecordingCapture();
#endif

    const image_rect_type *getFrameSizeSz(const image_rect_type *sizes,
        int count, uint32_t width, uint32_t height);
    const image_rect_type *getFrameSizeRatio(const image_rect_type *sizes,
        int count, uint32_t width, uint32_t height);

    void checkHorizontalViewAngle(void);
    void setHorizontalViewAngle(int pictureW, int pictureH);
    float findHorizontalViewAngleByRatio(uint32_t ratio);
    float getHorizontalViewAngle(void);
    float getVerticalViewAngle(void);

    void setZoomRatioList(int *list, int len, float maxZoomRatio);
    status_t getZoomRatioList(String8 & string8Buf, int maxZoom, int maxZoomRatio, int *list);

    bool        previewThread();
    bool        recordingThread();
    bool        autoFocusThread();
    bool        pictureThread();

    bool        HDRPictureThread();
    bool        RecordingPictureThread();
    bool        dumpPictureThread();

#if FRONT_ZSL
    bool    zslpictureThread();
#endif

#ifdef USE_DEDICATED_PREVIEW_ENQUEUE_THREAD
    status_t m_clearPreviewFrameList(ExynosCameraList<buffer_handle_t *> *queue);
#endif
    camera_device_t *mHalDevice;

public:
    int             mPostRecordIndex;
    nsecs_t         mRecordTimestamp;
    nsecs_t         mLastRecordTimestamp;

    bool            mPostRecordExit;
    mutable Mutex   mPostRecordLock;
    mutable Condition   mPostRecordCondition;
    bool            postRecordThread();
    bool            previewZoomThread();
    bool            previewEnqueueThread();
};
}; /* namespace android */

#endif /* ANDROID_HARDWARE_ISECCAMERAHARDWARE_H */
