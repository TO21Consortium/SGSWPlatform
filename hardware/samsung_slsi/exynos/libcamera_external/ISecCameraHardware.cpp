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
 * \file      ISecCameraHardware.cpp
 * \brief     source file for Android Camera Ext HAL
 * \author    teahyung kim (tkon.kim@samsung.com)
 * \date      2013/04/30
 *
 */

#ifndef ANDROID_HARDWARE_ISECCAMERAHARDWARE_CPP
#define ANDROID_HARDWARE_ISECCAMERAHARDWARE_CPP

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ISecCameraHardware"

#include <ISecCameraHardware.h>

namespace android {

ISecCameraHardware::ISecCameraHardware(int cameraId, camera_device_t *dev)
    : mCameraId(cameraId),
    mParameters(),
    mFlagANWindowRegister(false),
    mPreviewHeap(NULL),
    mPostviewHeap(NULL),
    mPostviewHeapTmp(NULL),
    mRawHeap(NULL),
    mRecordingHeap(NULL),
    mJpegHeap(NULL),
    mHDRHeap(NULL),
    mYUVHeap(NULL),
    mPreviewFormat(CAM_PIXEL_FORMAT_YUV420SP),
    mPictureFormat(CAM_PIXEL_FORMAT_JPEG),
    mFliteFormat(CAM_PIXEL_FORMAT_YUV420SP),
    mflipHorizontal(false),
    mflipVertical(false),
    mNeedSizeChange(false),
    mFastCaptureCalled(false),
    mRecordingTrace(false),
    mMsgEnabled(0),
    mGetMemoryCb(0),
    mPreviewWindow(NULL),
    mNotifyCb(0),
    mDataCb(0),
    mDataCbTimestamp(0),
    mCallbackCookie(0),
    mDisablePostview(false),
    mAntibanding60Hz(-1),

    mHalDevice(dev)
{
    const image_rect_type *videoSizes, *previewFLiteSizes, *captureFLiteSizes, *videoSize, *fliteSize;
    int videoSizesCount, previewFLiteCount, captureFLiteCount;

    if (mCameraId == CAMERA_ID_BACK) {
        mZoomSupport = IsZoomSupported();
        mEnableDZoom = mZoomSupport ? IsAPZoomSupported() : false;
        mFastCaptureSupport = IsFastCaptureSupportedOnRear();

        mSensorSize = backSensorSize;
        mPreviewSize = backPreviewSizes[0];
        mPictureSize = backPictureSizes[0];
        mThumbnailSize = backThumbSizes[0];
        videoSizes = backRecordingSizes;
        videoSizesCount = ARRAY_SIZE(backRecordingSizes);

        previewFLiteSizes = backFLitePreviewSizes;
        previewFLiteCount = ARRAY_SIZE(backFLitePreviewSizes);
        captureFLiteSizes = backFLiteCaptureSizes;
        captureFLiteCount = ARRAY_SIZE(backFLiteCaptureSizes);
    } else {
        mZoomSupport = IsZoomSupported();
        mEnableDZoom = mZoomSupport ? IsAPZoomSupported() : false;
        mFastCaptureSupport = IsFastCaptureSupportedOnFront();

        mSensorSize = frontSensorSize;
        mPreviewSize = frontPreviewSizes[0];
        mPictureSize = frontPictureSizes[0];
        mThumbnailSize = frontThumbSizes[0];
        videoSizes = frontRecordingSizes;
        videoSizesCount = ARRAY_SIZE(frontRecordingSizes);

        previewFLiteSizes = frontFLitePreviewSizes;
        previewFLiteCount = ARRAY_SIZE(frontFLitePreviewSizes);
        captureFLiteSizes = frontFLiteCaptureSizes;
        captureFLiteCount = ARRAY_SIZE(frontFLiteCaptureSizes);
    }

    mOrgPreviewSize = mPreviewSize;
    mPreviewWindowSize.width = mPreviewWindowSize.height = 0;

    videoSize = getFrameSizeRatio(videoSizes, videoSizesCount, mPreviewSize.width, mPreviewSize.height);
    mVideoSize = videoSize ? *videoSize : videoSizes[0];

    fliteSize = getFrameSizeRatio(previewFLiteSizes, previewFLiteCount, mPreviewSize.width, mPreviewSize.height);
    mFLiteSize = fliteSize ? *fliteSize : previewFLiteSizes[0];
    fliteSize = getFrameSizeRatio(captureFLiteSizes, captureFLiteCount, mPictureSize.width, mPictureSize.height);
    mFLiteCaptureSize = fliteSize ? *fliteSize : captureFLiteSizes[0];

    mRawSize = mPreviewSize;
    mPostviewSize = mPreviewSize;
    mCaptureMode = RUNNING_MODE_SINGLE;

#if FRONT_ZSL
    rawImageMem = NULL;
    mFullPreviewHeap = NULL;
#endif

    mPreviewWindowSize.width = mPreviewWindowSize.height = 0;

    mFrameRate = 0;
    mFps = 0;
    mJpegQuality= 96;
    mSceneMode = (cam_scene_mode)sceneModes[0].val;
    mFlashMode = (cam_flash_mode)flashModes[0].val;
    if (mCameraId == CAMERA_ID_BACK)
        mFocusMode = (cam_focus_mode)backFocusModes[0].val;
    else
        mFocusMode = (cam_focus_mode)frontFocusModes[0].val;

    mMaxFrameRate = 30000;
    mDropFrameCount = 3;
    mbFirst_preview_started = false;

    mIonCameraClient = -1;
    mPictureFrameSize = 0;
#if FRONT_ZSL
    mFullPreviewFrameSize = 0;
#endif
    CLEAR(mAntiBanding);
    mAutoFocusExit = false;
    mPreviewRunning = false;
    mRecordingRunning = false;
    mAutoFocusRunning = false;
    mPictureRunning = false;
    mRecordSrcIndex = -1;
    CLEAR(mRecordSrcSlot);
    mRecordDstIndex = -1;
    CLEAR(mRecordFrameAvailable);
    mRecordFrameAvailableCnt = 0;
    mFlagFirstFrameDump = false;
    mPostRecordIndex = -1;
    mRecordTimestamp = 0;
    mLastRecordTimestamp = 0;
    mPostRecordExit = false;
    mPreviewInitialized = false;
    mPreviewHeapFd = -1;
    mRecordHeapFd = -1;
    mPostviewHeapFd = -1;
    mPostviewHeapTmpFd = -1;
    CLEAR(mFrameMetadata);
    CLEAR(mFaces);
#if FRONT_ZSL
    mZSLindex = -1;
#endif
    mFullPreviewRunning = false;
    mPreviewFrameSize = 0;
    mRecordingFrameSize = 0;
    mRawFrameSize = 0;
    mPostviewFrameSize = 0;
    mFirstStart = 0;
    mTimerSet = 0;
    mZoomParamSet = 1;
    mZoomSetVal = 0;
    mZoomStatus = 0;
    mLensStatus = 0;
    mZoomStatusBak = 0;
    mLensChecked = 0;
    mLensStatus = 0;
    mCameraPower = true;

    roi_x_pos = 0;
    roi_y_pos = 0;
    roi_width = 0;
    roi_height = 0;

    mBurstShotExit = false;
    mPreviewFrameReceived = false;
}

ISecCameraHardware::~ISecCameraHardware()
{
    if (mPreviewHeap) {
        mPreviewHeap->release(mPreviewHeap);
        mPreviewHeap = 0;
        mPreviewHeapFd = -1;
    }

    if (mRecordingHeap) {
        mRecordingHeap->release(mRecordingHeap);
        mRecordingHeap = 0;
    }

    if (mRawHeap != NULL) {
        mRawHeap->release(mRawHeap);
        mRawHeap = 0;
    }

    if (mHDRHeap) {
        mHDRHeap->release(mHDRHeap);
        mHDRHeap = NULL;
    }

#ifndef RCJUNG
    if (mYUVHeap) {
        mYUVHeap->release(mYUVHeap);
        mYUVHeap = NULL;
    }
#endif

    if (mJpegHeap) {
        mJpegHeap->release(mJpegHeap);
        mJpegHeap = 0;
    }

    if (mPostviewHeap) {
        mPostviewHeap->release(mPostviewHeap);
        mPostviewHeap = 0;
    }

    if (mPostviewHeapTmp) {
        mPostviewHeapTmp->release(mPostviewHeapTmp);
        mPostviewHeapTmp = 0;
    }

    if (0 < mIonCameraClient)
        ion_close(mIonCameraClient);
    mIonCameraClient = -1;
}

bool ISecCameraHardware::init()
{
    mPreviewRunning = false;
    mFullPreviewRunning = false; /* for FRONT_ZSL */
    mPreviewInitialized = false;
#ifdef DEBUG_PREVIEW_CALLBACK
    mPreviewCbStarted = false;
#endif
    mRecordingRunning = false;
    mPictureRunning = false;
    mPictureStart = false;
    mCaptureStarted = false;
    mCancelCapture = false;
    mAutoFocusRunning = false;
    mAutoFocusExit = false;
    mFaceDetectionStatus = V4L2_FACE_DETECTION_OFF;
    mPreviewFrameReceived = false;

    if (mEnableDZoom) {
        /* Thread for zoom */
        mPreviewZoomThread = new CameraThread(this, &ISecCameraHardware::previewZoomThread, "previewZoomThread");
        mPostRecordThread = new CameraThread(this, &ISecCameraHardware::postRecordThread, "postRecordThread");
        mPreviewThread = mRecordingThread = NULL;
    } else {
        mPreviewThread = new CameraThread(this, &ISecCameraHardware::previewThread, "previewThread");
        mRecordingThread = new CameraThread(this, &ISecCameraHardware::recordingThread, "recordingThread");
        mPreviewZoomThread = mPostRecordThread = NULL;
    }

#ifdef USE_DEDICATED_PREVIEW_ENQUEUE_THREAD
    mPreviewEnqueueThread = new CameraThread(this,
        &ISecCameraHardware::previewEnqueueThread, "previewEnqueueThread");
#endif

    mPictureThread = new CameraThread(this, &ISecCameraHardware::pictureThread, "pictureThread");
#if FRONT_ZSL
    mZSLPictureThread = new CameraThread(this, &ISecCameraHardware::zslpictureThread, "zslpictureThread");
#endif

    if (mCameraId == CAMERA_ID_BACK) {
        mAutoFocusThread = new CameraThread(this, &ISecCameraHardware::autoFocusThread, "autoFocusThread");
        mAutoFocusThread->run("autoFocusThread", PRIORITY_DEFAULT);
        mHDRPictureThread = new CameraThread(this, &ISecCameraHardware::HDRPictureThread);
        mRecordingPictureThread = new CameraThread(this, &ISecCameraHardware::RecordingPictureThread);
        mDumpPictureThread = new CameraThread(this, &ISecCameraHardware::dumpPictureThread);
    }

    mIonCameraClient = ion_open();
    if (mIonCameraClient < 0) {
        CLOGE("ERR(%s):ion_open() fail", __func__);
        mIonCameraClient = -1;
    }

    for (int i = 0; i < REC_BUF_CNT; i++) {
        for (int j = 0; j < REC_PLANE_CNT; j++) {
            mRecordDstHeap[i][j] = NULL;
            mRecordDstHeapFd[i][j] = -1;
        }
    }

    mInitRecSrcQ();
    mInitRecDstBuf();

#ifdef USE_DEDICATED_PREVIEW_ENQUEUE_THREAD
    m_previewFrameQ.setWaitTime(2000000000);
#endif

    CLOGI("INFO(%s) : out ",__FUNCTION__);
    return true;
}

void ISecCameraHardware::initDefaultParameters()
{
    String8 tempStr;

    /* Preview */
    mParameters.setPreviewSize(mPreviewSize.width, mPreviewSize.height);

    if (mCameraId == CAMERA_ID_BACK) {
        mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,
                SecCameraParameters::createSizesStr(backPreviewSizes, ARRAY_SIZE(backPreviewSizes)).string());

        mParameters.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, B_KEY_PREVIEW_FPS_RANGE_VALUE);
        mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, B_KEY_SUPPORTED_PREVIEW_FPS_RANGE_VALUE);

        mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, B_KEY_SUPPORTED_PREVIEW_FRAME_RATES_VALUE);
        mParameters.setPreviewFrameRate(B_KEY_PREVIEW_FRAME_RATE_VALUE);

        mParameters.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, B_KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO_VALUE);
    } else {
        mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,
                SecCameraParameters::createSizesStr(frontPreviewSizes, ARRAY_SIZE(frontPreviewSizes)).string());

        mParameters.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, F_KEY_PREVIEW_FPS_RANGE_VALUE);
        mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, F_KEY_SUPPORTED_PREVIEW_FPS_RANGE_VALUE);

        mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES, F_KEY_SUPPORTED_PREVIEW_FRAME_RATES_VALUE);
        mParameters.setPreviewFrameRate(F_KEY_PREVIEW_FRAME_RATE_VALUE);

        mParameters.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, F_KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO_VALUE);
    }

    mParameters.setPreviewFormat(previewPixelFormats[0].desc);
    mParameters.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS,
        SecCameraParameters::createValuesStr(previewPixelFormats, ARRAY_SIZE(previewPixelFormats)).string());

    /* Picture */
    mParameters.setPictureSize(mPictureSize.width, mPictureSize.height);
    mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, mThumbnailSize.width);
    mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, mThumbnailSize.height);

    if (mCameraId == CAMERA_ID_BACK) {
        mParameters.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
                SecCameraParameters::createSizesStr(backPictureSizes, ARRAY_SIZE(backPictureSizes)).string());

        mParameters.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES,
                SecCameraParameters::createSizesStr(backThumbSizes, ARRAY_SIZE(backThumbSizes)).string());
    } else {
        mParameters.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
                SecCameraParameters::createSizesStr(frontPictureSizes, ARRAY_SIZE(frontPictureSizes)).string());

        mParameters.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES,
                SecCameraParameters::createSizesStr(frontThumbSizes, ARRAY_SIZE(frontThumbSizes)).string());
    }

    mParameters.setPictureFormat(picturePixelFormats[0].desc);
    mParameters.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS,
        SecCameraParameters::createValuesStr(picturePixelFormats, ARRAY_SIZE(picturePixelFormats)).string());

    mParameters.set(CameraParameters::KEY_JPEG_QUALITY, 96);
    mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, 100);

    mParameters.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);

    /* Video */
    mParameters.setVideoSize(mVideoSize.width, mVideoSize.height);
    if (mCameraId == CAMERA_ID_BACK) {
        mParameters.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES,
            SecCameraParameters::createSizesStr(backRecordingSizes, ARRAY_SIZE(backRecordingSizes)).string());
        mParameters.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED, B_KEY_VIDEO_STABILIZATION_SUPPORTED_VALUE);
    } else {
        mParameters.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES,
                SecCameraParameters::createSizesStr(frontRecordingSizes, ARRAY_SIZE(frontRecordingSizes)).string());
        mParameters.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED, F_KEY_VIDEO_STABILIZATION_SUPPORTED_VALUE);
    }

    mParameters.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, KEY_VIDEO_SNAPSHOT_SUPPORTED_VALUE);

    /* UI settings */
    mParameters.set(CameraParameters::KEY_WHITE_BALANCE, whiteBalances[0].desc);
    mParameters.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE,
        SecCameraParameters::createValuesStr(whiteBalances, ARRAY_SIZE(whiteBalances)).string());

    mParameters.set(CameraParameters::KEY_EFFECT, effects[0].desc);
    mParameters.set(CameraParameters::KEY_SUPPORTED_EFFECTS,
       SecCameraParameters::createValuesStr(effects, ARRAY_SIZE(effects)).string());

    if (mCameraId == CAMERA_ID_BACK) {
        mParameters.set(CameraParameters::KEY_SCENE_MODE, sceneModes[0].desc);
        mParameters.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES,
                SecCameraParameters::createValuesStr(sceneModes, ARRAY_SIZE(sceneModes)).string());

        if (IsFlashSupported()) {
            mParameters.set(CameraParameters::KEY_FLASH_MODE, flashModes[0].desc);
            mParameters.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES,
                    SecCameraParameters::createValuesStr(flashModes, ARRAY_SIZE(flashModes)).string());
        }

        mParameters.set(CameraParameters::KEY_FOCUS_MODE, backFocusModes[0].desc);
        mParameters.set(CameraParameters::KEY_FOCUS_DISTANCES, B_KEY_NORMAL_FOCUS_DISTANCES_VALUE);
        mParameters.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES,
                SecCameraParameters::createValuesStr(backFocusModes, ARRAY_SIZE(backFocusModes)).string());

        if (IsAutoFocusSupported()) {
            /* FOCUS AREAS supported.
             * MAX_NUM_FOCUS_AREAS > 0 : supported
             * MAX_NUM_FOCUS_AREAS = 0 : not supported
             *
             * KEY_FOCUS_AREAS = "left,top,right,bottom,weight"
             */
            mParameters.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, "1");
            mParameters.set(CameraParameters::KEY_FOCUS_AREAS, "(0,0,0,0,0)");
        }
    } else {
        mParameters.set(CameraParameters::KEY_FOCUS_MODE, frontFocusModes[0].desc);
        mParameters.set(CameraParameters::KEY_FOCUS_DISTANCES, F_KEY_FOCUS_DISTANCES_VALUE);
        mParameters.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES,
                SecCameraParameters::createValuesStr(frontFocusModes, ARRAY_SIZE(frontFocusModes)).string());
    }

    /* Face Detect */
    if (mCameraId == CAMERA_ID_BACK) {
        mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW,
            B_KEY_MAX_NUM_DETECTED_FACES_HW_VALUE);
        mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW,
            B_KEY_MAX_NUM_DETECTED_FACES_SW_VALUE);
    } else {
        mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW,
            F_KEY_MAX_NUM_DETECTED_FACES_HW_VALUE);
        mParameters.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW,
            F_KEY_MAX_NUM_DETECTED_FACES_SW_VALUE);
    }

    /* Zoom */
    if (mZoomSupport) {
        int maxZoom = getMaxZoomLevel();
        int maxZoomRatio = getMaxZoomRatio();

        mParameters.set(CameraParameters::KEY_ZOOM_SUPPORTED,
            B_KEY_ZOOM_SUPPORTED_VALUE);
        mParameters.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED,
            B_KEY_SMOOTH_ZOOM_SUPPORTED_VALUE);
        mParameters.set(CameraParameters::KEY_MAX_ZOOM, maxZoom - 1);
        mParameters.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);

        tempStr.setTo("");
        if (getZoomRatioList(tempStr, maxZoom, maxZoomRatio, zoomRatioList) == NO_ERROR)
            mParameters.set(CameraParameters::KEY_ZOOM_RATIOS, tempStr.string());
        else
            mParameters.set(CameraParameters::KEY_ZOOM_RATIOS, "100");

        mParameters.set("constant-growth-rate-zoom-supported", "true");

        CLOGD("INFO(%s):zoomRatioList=%s", "setDefaultParameter", tempStr.string());
    } else {
        mParameters.set(CameraParameters::KEY_ZOOM_SUPPORTED, "false");
        mParameters.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
        mParameters.set(CameraParameters::KEY_MAX_ZOOM, ZOOM_LEVEL_0);
        mParameters.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);
    }

    mParameters.set(CameraParameters::KEY_MAX_NUM_METERING_AREAS, "0");

    mParameters.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, 0);
    mParameters.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, 4);
    mParameters.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, -4);
    mParameters.setFloat(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, 0.5);

    /* AE, AWB Lock */
    if (mCameraId == CAMERA_ID_BACK) {
        mParameters.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED,
            B_KEY_AUTO_EXPOSURE_LOCK_SUPPORTED_VALUE);
        mParameters.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED,
            B_KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED_VALUE);
    } else {
        mParameters.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED,
            F_KEY_AUTO_EXPOSURE_LOCK_SUPPORTED_VALUE);
        mParameters.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED,
            F_KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED_VALUE);
    }

    /* AntiBanding */
    char supportedAntiBanding[20] = {0,};
    sprintf(supportedAntiBanding,"auto,%s", (char *)mAntiBanding);
    mParameters.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING, supportedAntiBanding);
    mParameters.set(CameraParameters::KEY_ANTIBANDING, antibandings[0].desc);

    mParameters.set(CameraParameters::KEY_ISO, CameraParameters::ISO_AUTO);
    mParameters.set("iso-values", SecCameraParameters::createValuesStr(isos, ARRAY_SIZE(isos)).string());

    /* View Angle */
    setHorizontalViewAngle(640, 480);
    mParameters.setFloat(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, getHorizontalViewAngle());
    mParameters.setFloat(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, getVerticalViewAngle());

    /* Burst FPS Value */
#ifdef BURSTSHOT_MAX_FPS
    tempStr.setTo("");
    tempStr = String8::format("(%d,%d)", BURSTSHOT_MAX_FPS, BURSTSHOT_MAX_FPS);
    mParameters.set("burstshot-fps-values", tempStr.string());
#else
    mParameters.set("burstshot-fps-values", "(0,0)");
#endif

    CLOGV("INFO(%s) : out - %s ",__FUNCTION__, mParameters.flatten().string());

}

status_t ISecCameraHardware::setPreviewWindow(preview_stream_ops *w)
{
    mPreviewWindow = w;

    if (CC_UNLIKELY(!w)) {
        mPreviewWindowSize.width = mPreviewWindowSize.height = 0;
        ALOGE("setPreviewWindow: NULL Surface!");
        return OK;
    }

    int halPixelFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_FULL;

    if (mMovieMode)
        halPixelFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP;
    CLOGD("DEBUG(%s) :size(%d/%d)", __FUNCTION__, mPreviewSize.width, mPreviewSize.height);

    /* YV12 */
    CLOGV("setPreviewWindow: halPixelFormat = %s",
        halPixelFormat == HAL_PIXEL_FORMAT_YV12 ? "YV12" :
        halPixelFormat == HAL_PIXEL_FORMAT_YCrCb_420_SP ? "NV21" :
        halPixelFormat == HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP ? "NV21M" :
        halPixelFormat == HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL ? "NV21 FULL" :
        "Others");

    mPreviewWindowSize = mPreviewSize;

    CLOGD("DEBUG [%s(%d)] setPreviewWindow window Size width=%d height=%d",
                 __FUNCTION__, __LINE__, mPreviewWindowSize.width, mPreviewWindowSize.height);
    if (nativeCreateSurface(mPreviewWindowSize.width, mPreviewWindowSize.height, halPixelFormat) == false) {
        CLOGE("setPreviewWindow: error, nativeCreateSurface");
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

status_t ISecCameraHardware::startPreview()
{
    CLOGI("INFO(%s) : in ",__FUNCTION__);

    LOG_PERFORMANCE_START(1);

    Mutex::Autolock lock(mLock);

    if (mPictureRunning) {
        CLOGW("startPreview: warning, picture is not completed yet");
        if ((mMsgEnabled & CAMERA_MSG_RAW_IMAGE) ||
            (mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME)) {
            /* upper layer can access the mmaped memory if raw or postview message is enabled
            But the data will be changed after preview is started */
            CLOGE("startPreview: error, picture data is not transferred yet");
            return INVALID_OPERATION;
        }
    }

    status_t ret;
    if (mEnableDZoom)
        ret = nativeStartPreviewZoom();
    else
        ret = nativeStartPreview();

    if (ret != NO_ERROR) {
        CLOGE("startPreview: error, nativeStartPreview");

        return NO_INIT;
    }

    setDropUnstableInitFrames();

#ifdef USE_DEDICATED_PREVIEW_ENQUEUE_THREAD
    m_flagEnqueueThreadStop = false;
    ret = mPreviewEnqueueThread->run("previewEnqueueThread", PRIORITY_URGENT_DISPLAY);
    if (ret != NO_ERROR) {
        CLOGE("startPreview: error, starting previewEnqueueThread");
        return UNKNOWN_ERROR;
    }
#endif

    mPreviewFrameReceived = false;
    mFlagFirstFrameDump = false;
    if (mEnableDZoom) {
        ret = mPreviewZoomThread->run("previewZoomThread", PRIORITY_URGENT_DISPLAY);
    } else {
        ret = mPreviewThread->run("previewThread", PRIORITY_URGENT_DISPLAY);
    }

    if (ret != NO_ERROR) {
        CLOGE("startPreview: error, Not starting preview");
        return UNKNOWN_ERROR;
    }

#if FRONT_ZSL
    if (/*mSamsungApp &&*/ !mMovieMode && mCameraId == CAMERA_ID_FRONT) {
        if (nativeStartFullPreview() != NO_ERROR) {
            CLOGE("startPreview: error, nativeStartPreview");
            return NO_INIT;
        }

        if (mZSLPictureThread->run("zslpictureThread", PRIORITY_URGENT_DISPLAY) != NO_ERROR) {
            CLOGE("startPreview: error, Not starting preview");
            return UNKNOWN_ERROR;
        }

        mFullPreviewRunning = true;
    }
#endif
    mPreviewRunning = true;

    LOG_PERFORMANCE_END(1, "total");

    CLOGI("INFO(%s) : out ",__FUNCTION__);

    return NO_ERROR;
}

#ifdef USE_DEDICATED_PREVIEW_ENQUEUE_THREAD
bool ISecCameraHardware::previewEnqueueThread()
{
    int ret = 0;
    buffer_handle_t * buf_handle = NULL;

    ret = m_previewFrameQ.waitAndPopProcessQ(&buf_handle);
    if (m_flagEnqueueThreadStop == true) {
        ALOGD("INFO(%s[%d]):m_flagEnqueueThreadStop(%d)", __FUNCTION__, __LINE__,
            m_flagEnqueueThreadStop);

        if(buf_handle != NULL && mPreviewWindow != NULL) {
            if(mPreviewWindow->cancel_buffer(mPreviewWindow, buf_handle) != 0) {
                ALOGE("ERR(%s):Fail to cancel buffer", __func__);
            }
        }
        return false;
    }
    if (ret < 0) {
        ALOGW("WARN(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        goto func_exit;
    }

    if (0 != mPreviewWindow->enqueue_buffer(mPreviewWindow, buf_handle)) {
        ALOGE("Could not enqueue gralloc buffer!\n");
        goto func_exit;
    }

func_exit:
    return true;
}
#endif

bool ISecCameraHardware::previewThread()
{
    CLOGI("INFO(%s) : in ",__FUNCTION__);

    int index = nativeGetPreview();
    if (CC_UNLIKELY(index < 0)) {
        if (mFastCaptureCalled) {
            return false;
        }
        CLOGE("previewThread: error, nativeGetPreview");

        if (!mPreviewThread->exitRequested()) {
            mNotifyCb(CAMERA_MSG_ERROR, CAMERA_MSG_ERROR, 0,  mCallbackCookie);
            CLOGI("previewZoomThread: X, after callback");
        }
        return false;
    }else if (mPreviewThread->exitRequested()) {
        return false;
    }

#ifdef DUMP_LAST_PREVIEW_FRAME
    mLastIndex = index;
#endif

    mLock.lock();

    if (mDropFrameCount > 0) {
        mDropFrameCount--;
        mLock.unlock();
        nativeReleasePreviewFrame(index);
        return true;
    }

    mLock.unlock();

    /* Notify the client of a new frame. */
    if (mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME) {
#ifdef DEBUG_PREVIEW_CALLBACK
        if (!mPreviewCbStarted) {
            mPreviewCbStarted = true;
            ALOGD("preview callback started");
        }
#endif
        mDataCb(CAMERA_MSG_PREVIEW_FRAME, mPreviewHeap, index, NULL, mCallbackCookie);
    }

    /* Display a new frame */
    if (CC_LIKELY(mFlagANWindowRegister)) {
        bool ret = nativeFlushSurface(mPreviewWindowSize.width, mPreviewWindowSize.height, mPreviewFrameSize, index);
        if (CC_UNLIKELY(!ret))
            CLOGE("previewThread: error, nativeFlushSurface");
        mPreviewFrameReceived = true;
    }

    mLock.lock();
    if (mFirstStart == 0)
        mFirstStart = 1;
    mLock.unlock();

#if DUMP_FILE
    static int i = 0;
    if (++i % 15 == 0) {
        dumpToFile(mPreviewHeap->data + (mPreviewFrameSize*index), mPreviewFrameSize, "/data/media/0/preview.yuv");
        i = 0;
    }
#endif

    /* Release the frame */
    int err = nativeReleasePreviewFrame(index);
    if (CC_UNLIKELY(err < 0)) {
        CLOGE("previewThread: error, nativeReleasePreviewFrame");
        return false;
    }

    /* prevent a frame rate from getting higher than the max value */
    mPreviewThread->calcFrameWaitTime(mMaxFrameRate);

    CLOGV("INFO(%s) : out ",__FUNCTION__);
    return true;
}

bool ISecCameraHardware::previewZoomThread()
{
    CLOGV("INFO(%s) : in ",__FUNCTION__);

    int index = nativeGetPreview();
    int err = -1;

    if (CC_UNLIKELY(index < 0)) {
        if (mFastCaptureCalled) {
            return false;
        }
        CLOGE("previewZoomThread: error, nativeGetPreview in %s", recordingEnabled() ? "recording" : "preview");
        if (!mPreviewZoomThread->exitRequested()) {
            mNotifyCb(CAMERA_MSG_ERROR, CAMERA_MSG_ERROR, 0,  mCallbackCookie);
            CLOGI("INFO(%s): Exit, after callback",__FUNCTION__);
        }

        return false;
    } else if (mPreviewZoomThread->exitRequested()) {
        return false;
    }

#ifdef DUMP_LAST_PREVIEW_FRAME
    mLastIndex = index;
#endif

    mPostRecordIndex = index;

    mLock.lock();
    if (mDropFrameCount > 0) {
        mDropFrameCount--;
        mLock.unlock();
        nativeReleasePreviewFrame(index);
        CLOGW("DEBUG [%s(%d)] mDropFrameCount(%d), index(%d)",__FUNCTION__, __LINE__,mDropFrameCount, index);		
        return true;
    }
    mLock.unlock();

    /* first frame dump to jpeg */
    if (mFlagFirstFrameDump == true) {
        memcpy(mPictureBuf.virt.extP[0], mFliteNode.buffer[index].virt.extP[0], mPictureBuf.size.extS[0]);
        nativeMakeJpegDump();
        mFlagFirstFrameDump = false;
    }

    /* when recording mode, push frame of dq from FLite */
    if (mRecordingRunning) {
        int videoSlotIndex = getRecSrcBufSlotIndex();
        if (videoSlotIndex < 0) {
            CLOGE("ERROR(%s): videoSlotIndex is -1", __func__);
        } else {
                mRecordSrcSlot[videoSlotIndex].buf = &(mFliteNode.buffer[index]);
                mRecordSrcSlot[videoSlotIndex].timestamp = mRecordTimestamp;
                /* ALOGV("DEBUG(%s): recording src(%d) adr %p, timestamp %lld", __func__, videoSlotIndex,
                    (mRecordSrcSlot[videoSlotIndex].buf)->virt.p, mRecordSrcSlot[videoSlotIndex].timestamp); */
                mPushRecSrcQ(&mRecordSrcSlot[videoSlotIndex]);
                mPostRecordCondition.signal();
            }
        }

    mPreviewRunning = true;

#if DUMP_FILE
    static int j = 0;

    int buftype=ExynosBuffer::BUFFER_TYPE(&(mFliteNode.buffer[index]));
    if( buftype & 1 /*BUFFER_TYPE_VIRT*/ ) {
        if (++j % 15 == 0) {
            dumpToFile(mFliteNode.buffer[index].virt.p,
                mFliteNode.buffer[index].size.extS[0], "/data/media/0/preview_1.yuv");
            j = 0;
        }
    }
#endif

    /* Display a new frame */
    if (CC_LIKELY(mFlagANWindowRegister)) {
        bool ret = nativeFlushSurface(mPreviewWindowSize.width, mPreviewWindowSize.height, mPreviewFrameSize, index);
        if (CC_UNLIKELY(!ret))
            CLOGE("ERROR(%s): error, nativeFlushSurface", __func__);
        mPreviewFrameReceived = true;
    } else {
        /* if not register ANativeWindow, just prepare callback buffer on CAMERA_MSG_PREVIEW_FRAME */
        if (mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME)
            if (nativePreviewCallback(index, NULL) < 0)
                CLOGE("ERROR(%s): nativePreviewCallback failed", __func__);
    }

    /* Notify the client of a new frame. */
    if (mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME) {
#ifdef DEBUG_PREVIEW_CALLBACK
        if (!mPreviewCbStarted) {
            mPreviewCbStarted = true;
            ALOGD("preview callback started");
        }
#endif
        mDataCb(CAMERA_MSG_PREVIEW_FRAME, mPreviewHeap, index, NULL, mCallbackCookie);
    }

#if DUMP_FILE
    static int i = 0;
    if (++i % 15 == 0) {
        dumpToFile(mPreviewHeap->data + (mPreviewFrameSize*index), mPreviewFrameSize, "/data/media/0/preview.yuv");
        i = 0;
    }
#endif

    /* Release the frame */
    err = nativeReleasePreviewFrame(index);
    if (CC_UNLIKELY(err < 0)) {
        CLOGE("ERROR(%s):  error, nativeReleasePreviewFrame", __func__);
        return false;
    }

    CLOGV("INFO(%s) : out ",__FUNCTION__);
    return true;
}

void ISecCameraHardware::stopPreview()
{
    CLOGI("INFO(%s) : in ",__FUNCTION__);

    /*
    * try count to wait for stopping previewThread
    * maximum wait time = 30 * 10ms
    */
    int waitForCnt = 600; // 30 -->. 600 (300ms --> 6sec) because, Polling timeout is 5 sec.

    LOG_PERFORMANCE_START(1);

    {
        Mutex::Autolock lock(mLock);
        if (!mPreviewRunning) {
            CLOGW("stopPreview: warning, preview has been stopped");
            return;
        }
    }

    nativeDestroySurface();
    /* don't hold the lock while waiting for the thread to quit */
#if FRONT_ZSL
    if (mFullPreviewRunning) {
        mZSLPictureThread->requestExitAndWait();
        nativeForceStopFullPreview();
        nativeStopFullPreview();
        mFullPreviewRunning = false;
    }
#endif
    if (mEnableDZoom) {
        mPreviewZoomThread->requestExit();
        /* if previewThread can't finish, wait for 25ms */
        while (waitForCnt > 0 && mPreviewZoomThread->getTid() >= 0) {
            usleep(10000);
            waitForCnt--;
        }
    } else {
        mPreviewThread->requestExitAndWait();
    }

#ifdef USE_DEDICATED_PREVIEW_ENQUEUE_THREAD
    m_flagEnqueueThreadStop = true;
    m_previewFrameQ.sendCmd(WAKE_UP);
    mPreviewEnqueueThread->requestExitAndWait();
    m_clearPreviewFrameList(&m_previewFrameQ);
#endif

#ifdef DUMP_LAST_PREVIEW_FRAME
    uint32_t offset = mPreviewFrameSize * mLastIndex;
    dumpToFile(mPreviewHeap->base() + offset, mPreviewFrameSize, "/data/media/0/preview-last.dump");
#endif

    Mutex::Autolock lock(mLock);

    nativeStopPreview();

    if (mEnableDZoom == true && waitForCnt > 0) {
        mPreviewZoomThread->requestExitAndWait();
    }

    mPreviewRunning = false;
    mPreviewInitialized = false;
#ifdef DEBUG_PREVIEW_CALLBACK
    mPreviewCbStarted = false;
#endif
    mPreviewFrameReceived = false;
    LOG_PERFORMANCE_END(1, "total");

    if (mPreviewHeap) {
        mPreviewHeap->release(mPreviewHeap);
        mPreviewHeap = 0;
        mPreviewHeapFd = -1;
    }

    if (mRecordingHeap) {
        mRecordingHeap->release(mRecordingHeap);
        mRecordingHeap = 0;
    }

    if (mRawHeap != NULL) {
        mRawHeap->release(mRawHeap);
        mRawHeap = 0;
    }

    if (mJpegHeap) {
        mJpegHeap->release(mJpegHeap);
        mJpegHeap = 0;
    }

    if (mHDRHeap) {
        mHDRHeap->release(mHDRHeap);
        mHDRHeap = 0;
    }

    if (mPostviewHeap) {
        mPostviewHeap->release(mPostviewHeap);
        mPostviewHeap = 0;
    }

    if (mPostviewHeapTmp) {
        mPostviewHeapTmp->release(mPostviewHeapTmp);
        mPostviewHeapTmp = 0;
    }

    CLOGI("INFO(%s) : out ",__FUNCTION__);
}

status_t ISecCameraHardware::storeMetaDataInBuffers(bool enable)
{
    ALOGV("%s", __FUNCTION__);

    if (!enable) {
        ALOGE("Non-m_frameMetadata buffer mode is not supported!");
        return INVALID_OPERATION;
    }

    return OK;
}

status_t ISecCameraHardware::startRecording()
{
    CLOGI("INFO(%s) : in ",__FUNCTION__);

    Mutex::Autolock lock(mLock);

    status_t ret;
    mLastRecordTimestamp = 0;
#if FRONT_ZSL
    if (mFullPreviewRunning) {
        nativeForceStopFullPreview();
        mZSLPictureThread->requestExitAndWait();
        nativeStopFullPreview();
        mFullPreviewRunning = false;
    }
#endif

    if (mEnableDZoom) {
        ret = nativeStartRecordingZoom();
    } else {
        ret = nativeStartRecording();
    }

    if (CC_UNLIKELY(ret != NO_ERROR)) {
        CLOGE("startRecording X: error, nativeStartRecording");
        return UNKNOWN_ERROR;
    }

    if (mEnableDZoom) {
        mPostRecordExit = false;
        ret = mPostRecordThread->run("postRecordThread", PRIORITY_URGENT_DISPLAY);
    } else
        ret = mRecordingThread->run("recordingThread", PRIORITY_URGENT_DISPLAY);

    if (CC_UNLIKELY(ret != NO_ERROR)) {
        mRecordingTrace = true;
        CLOGE("startRecording: error %d, Not starting recording", ret);
        return ret;
    }

    mRecordingRunning = true;

    CLOGD("DEBUG [%s(%d)] -out-",__FUNCTION__, __LINE__);
    return NO_ERROR;
}

bool ISecCameraHardware::recordingThread()
{
    return true;
}

bool ISecCameraHardware::postRecordThread()
{
    mPostRecordLock.lock();
    mPostRecordCondition.wait(mPostRecordLock);
    mPostRecordLock.unlock();

    if (mSizeOfRecSrcQ() == 0) {
        ALOGW("WARN(%s): mSizeOfRecSrcQ size is zero", __func__);
    } else {
        rec_src_buf_t srcBuf;

        while (mSizeOfRecSrcQ() > 0) {
            int index;

            /* get dst video buf index */
            index = getRecDstBufIndex();
            if (index < 0) {
                ALOGW("WARN(%s): getRecDstBufIndex(%d) sleep and continue, skip frame buffer", __func__, index);
                usleep(13000);
                continue;
            }

            /* get src video buf */
            if (mPopRecSrcQ(&srcBuf) == false) {
                ALOGW("WARN(%s): mPopRecSrcQ(%d) failed", __func__, index);
                return false;
            }

            /* ALOGV("DEBUG(%s): SrcBuf(%d, %d, %lld), Dst idx(%d)", __func__,
                srcBuf.buf->fd.extFd[0], srcBuf.buf->fd.extFd[1], srcBuf.timestamp, index); */

            /* Notify the client of a new frame. */
            if (mMsgEnabled & CAMERA_MSG_VIDEO_FRAME) {
                bool ret;
                /* csc from flite to video MHB and callback */
                ret = nativeCSCRecording(&srcBuf, index);
                if (ret == false) {
                    ALOGE("ERROR(%s): nativeCSCRecording failed.. SrcBuf(%d, %d, %lld), Dst idx(%d)", __func__,
                        srcBuf.buf->fd.extFd[0], srcBuf.buf->fd.extFd[1], srcBuf.timestamp, index);
                    setAvailDstBufIndex(index);
                    return false;
                } else {
                    if (0L < srcBuf.timestamp && mLastRecordTimestamp < srcBuf.timestamp) {
                        mDataCbTimestamp(srcBuf.timestamp, CAMERA_MSG_VIDEO_FRAME,
                                         mRecordingHeap, index, mCallbackCookie);
                        mLastRecordTimestamp = srcBuf.timestamp;
                        LOG_RECORD_TRACE("callback returned");
                    } else {
                        ALOGW("WARN(%s): timestamp(%lld) invaild - last timestamp(%lld) systemtime(%lld)",
                            __func__, srcBuf.timestamp, mLastRecordTimestamp, systemTime(SYSTEM_TIME_MONOTONIC));
                        setAvailDstBufIndex(index);
                    }
                }
            }
        }
    }
    LOG_RECORD_TRACE("X");
    return true;
}

void ISecCameraHardware::stopRecording()
{
    CLOGI("INFO(%s) : in ",__FUNCTION__);

    Mutex::Autolock lock(mLock);
    if (!mRecordingRunning) {
        ALOGW("stopRecording: warning, recording has been stopped");
        return;
    }

    /* We request thread to exit. Don't wait. */
    if (mEnableDZoom) {
        mPostRecordExit = true;

        /* Change calling order of requestExit...() and signal
         * if you want to change requestExit() to requestExitAndWait().
         */
        mPostRecordThread->requestExit();
        mPostRecordCondition.signal();
        nativeStopRecording();
    } else {
        mRecordingThread->requestExit();
        nativeStopRecording();
    }

    mRecordingRunning = false;

    ALOGD("stopRecording X");
}

void ISecCameraHardware::releaseRecordingFrame(const void *opaque)
{
    status_t ret = NO_ERROR;
    bool found = false;
    int i;

    /* We does not release frames recorded any longer
     * if this function is called after stopRecording().
     */
    if (mEnableDZoom)
        ret = mPostRecordThread->exitRequested();
    else
        ret = mRecordingThread->exitRequested();

    if (CC_UNLIKELY(ret)) {
        ALOGW("releaseRecordingFrame: warning, we do not release any more!!");
        return;
    }

    {
        Mutex::Autolock lock(mLock);
        if (CC_UNLIKELY(!mRecordingRunning)) {
            ALOGW("releaseRecordingFrame: warning, recording is not running");
            return;
        }
    }

    struct addrs *addrs = (struct addrs *)mRecordingHeap->data;

    /* find MHB handler to match */
    if (addrs) {
        for (i = 0; i < REC_BUF_CNT; i++) {
            if ((char *)(&(addrs[i].type)) == (char *)opaque) {
                found = true;
                break;
            }
        }
    }

    mRecordDstLock.lock();
    if (found) {
        mRecordFrameAvailableCnt++;
        /* ALOGV("DEBUG(%s): found index[%d] FDy(%d), FDcbcr(%d) availableCount(%d)", __func__,
            i, addrs[i].fd_y, addrs[i].fd_cbcr, mRecordFrameAvailableCnt); */
        mRecordFrameAvailable[i] = true;
    } else
        ALOGE("ERR(%s):no matched index(%p)", __func__, (char *)opaque);

    if (mRecordFrameAvailableCnt > REC_BUF_CNT) {
        ALOGW("WARN(%s): mRecordFrameAvailableCnt is more than REC_BUF!!", __func__);
        mRecordFrameAvailableCnt = REC_BUF_CNT;
    }
    mRecordDstLock.unlock();
}

status_t ISecCameraHardware::autoFocus()
{
    ALOGV("autoFocus EX");
    /* signal autoFocusThread to run once */

    if (mCameraId != CAMERA_ID_BACK) {
        ALOGV("Do not support autoFocus in front camera");
        mNotifyCb(CAMERA_MSG_FOCUS, true, 0, mCallbackCookie);
        return NO_ERROR;
    }

    mAutoFocusCondition.signal();
    return NO_ERROR;
}

bool ISecCameraHardware::autoFocusThread()
{
    /* block until we're told to start.  we don't want to use
     * a restartable thread and requestExitAndWait() in cancelAutoFocus()
     * because it would cause deadlock between our callbacks and the
     * caller of cancelAutoFocus() which both want to grab the same lock
     * in CameraServices layer.
     */
    mAutoFocusLock.lock();
    mAutoFocusCondition.wait(mAutoFocusLock);
    mAutoFocusLock.unlock();

    /* check early exit request */
    if (mAutoFocusExit)
        return false;

    ALOGV("autoFocusThread E");
    LOG_PERFORMANCE_START(1);

    mAutoFocusRunning = true;

    if (!IsAutoFocusSupported()) {
        ALOGV("autofocus not supported");
        mNotifyCb(CAMERA_MSG_FOCUS, true, 0, mCallbackCookie);
        goto out;
    }

    if (!autoFocusCheckAndWaitPreview()) {
        ALOGI("autoFocusThread: preview not started");
        mNotifyCb(CAMERA_MSG_FOCUS, false, 0, mCallbackCookie);
        goto out;
    }

    /* Start AF operations */
    if (!nativeSetAutoFocus()) {
        ALOGE("autoFocusThread X: error, nativeSetAutofocus");
        goto out;
    }

    if (!IsAutofocusRunning()) {
        ALOGV("autoFocusThread X: AF is canceled");
        nativeSetParameters(CAM_CID_FOCUS_MODE, mFocusMode | FOCUS_MODE_DEFAULT);
        goto out;
    }

    if (mMsgEnabled & CAMERA_MSG_FOCUS) {
        switch (nativeGetAutoFocus()) {
        case 0x02:
            ALOGV("autoFocusThread X: AF success");
            mNotifyCb(CAMERA_MSG_FOCUS, true, 0, mCallbackCookie);
            break;

        case 0x04:
            ALOGV("autoFocusThread X: AF cancel");
            nativeSetParameters(CAM_CID_FOCUS_MODE, mFocusMode | FOCUS_MODE_DEFAULT);
            break;

        default:
            ALOGW("autoFocusThread X: AF fail");
            mNotifyCb(CAMERA_MSG_FOCUS, false, 0, mCallbackCookie);
            break;
        }
    }

out:
    mAutoFocusRunning = false;

    LOG_PERFORMANCE_END(1, "total");
    return true;
}

status_t ISecCameraHardware::cancelAutoFocus()
{
    ALOGV("cancelAutoFocus: autoFocusThread is %s",
        mAutoFocusRunning ? "running" : "not running");

    if (!IsAutoFocusSupported())
        return NO_ERROR;
    status_t err = NO_ERROR;

    if (mAutoFocusRunning) {
        err = nativeCancelAutoFocus();
    } else {
        err = nativeSetParameters(CAM_CID_FOCUS_MODE, mFocusMode | FOCUS_MODE_DEFAULT);
    }

    if (CC_UNLIKELY(err != NO_ERROR)) {
        ALOGE("cancelAutoFocus: error, nativeCancelAutofocus");
        return UNKNOWN_ERROR;
    }
    mAutoFocusRunning = false;
    return NO_ERROR;
}

#if FRONT_ZSL
bool ISecCameraHardware::zslpictureThread()
{
    mZSLindex = nativeGetFullPreview();

    if (CC_UNLIKELY(mZSLindex < 0)) {
        CLOGE("zslpictureThread: error, nativeGetFullPreview");
        return true;
    }

    nativeReleaseFullPreviewFrame(mZSLindex);

    return true;
}
#endif

/* --1 */
status_t ISecCameraHardware::takePicture()
{
    CLOGI("INFO(%s) : in ",__FUNCTION__);

    int facedetVal = 0;

    if (mPreviewRunning == false || mPreviewFrameReceived == false) {
        CLOGW("WARN(%s): preview is not initialized", __func__);
        int retry = 70;
        while (retry > 0 && (mPreviewRunning == false || mPreviewFrameReceived == false)) {
            usleep(5000);
            retry--;
        }
    }

#ifdef RECORDING_CAPTURE
    if (mMovieMode || mRecordingRunning) {
        if (mPictureRunning) {
            CLOGW("WARN(%s): pictureThread is alive. wait to finish it", __func__);
            int retry_mov = 50;
            while (retry_mov > 0 && mPictureRunning == true) {
                usleep(5000);
                retry_mov--;
            }
            if (retry_mov > 0) {
                CLOGW("WARN(%s): pictureThread is finished", __func__);
            } else {
                CLOGW("WARN(%s): wait timeout", __func__);
            }
        }
    }
#endif

    mPictureStart = true;

    if (mCaptureMode != RUNNING_MODE_SINGLE || facedetVal == V4L2_FACE_DETECTION_BLINK) {
        if (mPreviewRunning) {
            CLOGW("takePicture: warning, preview is running");
            stopPreview();
        }
        nativeSetFastCapture(false);
    }
    mPictureStart = false;

#ifndef RECORDING_CAPTURE
    if (mMovieMode || mRecordingRunning) {
        CLOGW("takePicture: warning, not support taking picture in recording mode");
        return NO_ERROR;
    }
#endif /* RECORDING_CAPTURE */

    Mutex::Autolock lock(mLock);
    if (mPictureRunning) {
        CLOGE("takePicture: error, picture already running");
        return INVALID_OPERATION;
    }

    if (mPictureThread->run("pictureThread", PRIORITY_DEFAULT) != NO_ERROR) {
        CLOGE("takePicture: error, Not starting take picture");
        return UNKNOWN_ERROR;
    }

    CLOGI("INFO(%s) : out ",__FUNCTION__);
    return NO_ERROR;
}

/* --2 */
bool ISecCameraHardware::pictureThread()
{
    mPictureRunning = true;

#ifdef RECORDING_CAPTURE
    if (mMovieMode || mRecordingRunning) {
        doRecordingCapture();
    } else
#endif
    {
        if (mPreviewRunning && !mFullPreviewRunning) {
            CLOGW("takePicture: warning, preview is running");
            stopPreview();
        }

        doCameraCapture();
    }

    return false;
}

status_t ISecCameraHardware::doCameraCapture()
{
    CLOGI("INFO(%s) : in ",__FUNCTION__);

    mPictureLock.lock();

    LOG_PERFORMANCE_START(1);

    LOG_PERFORMANCE_START(2);
    if (!nativeStartSnapshot()) {
        CLOGE("doCameraCapture: error, nativeStartSnapshot");
        mPictureLock.unlock();
        goto out;
    }
    LOG_PERFORMANCE_END(2, "nativeStartSnapshot");

    if ((mMsgEnabled & CAMERA_MSG_SHUTTER) && (mSceneMode != SCENE_MODE_NIGHTSHOT)) {
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
        mPictureLock.lock();
    }

    LOG_PERFORMANCE_START(3);
    int postviewOffset;
    if (!nativeGetSnapshot(1, &postviewOffset)) {
        CLOGE("doCameraCapture: error, nativeGetSnapshot");
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
        goto out;
    }
    LOG_PERFORMANCE_END(3, "nativeGetSnapshot");

    mPictureLock.unlock();

    if ((mMsgEnabled & CAMERA_MSG_SHUTTER) && (mSceneMode == SCENE_MODE_NIGHTSHOT))
        mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);

    /* Display postview */
    LOG_PERFORMANCE_START(4);

    /* callbacks for capture */
    if ((mMsgEnabled & CAMERA_MSG_RAW_IMAGE) && (mRawHeap != NULL))
        mDataCb(CAMERA_MSG_RAW_IMAGE, mRawHeap, 0, NULL, mCallbackCookie);

    if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY)
        mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);

    if ((mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) && (mRawHeap != NULL))
        mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mRawHeap, 0, NULL, mCallbackCookie);

    if ((mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) && (mJpegHeap != NULL))
        mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);

    LOG_PERFORMANCE_END(4, "callback functions");

#if DUMP_FILE
    dumpToFile(mJpegHeap->data, mPictureFrameSize, "/data/media/0/capture01.jpg");
#endif

out:
    nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();

    LOG_PERFORMANCE_END(1, "total");

    CLOGI("INFO(%s) : out ",__FUNCTION__);
    return NO_ERROR;
}

#ifdef RECORDING_CAPTURE
status_t ISecCameraHardware::doRecordingCapture()
{
    CLOGI("INFO(%s) : in ",__FUNCTION__);

    if (!mMovieMode && !mRecordingRunning) {
        ALOGI("doRecordingCapture: nothing to do");
        mLock.lock();
        mPictureRunning = false;
        mLock.unlock();
        return NO_ERROR;
    }

    if ((mMsgEnabled & CAMERA_MSG_SHUTTER) && (mSceneMode != SCENE_MODE_NIGHTSHOT)) {
        mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
    }

    mPictureLock.lock();

    bool ret = false;
    int index = mPostRecordIndex;
    if (index < 0) {
        CLOGW("WARN(%s):(%d)mPostRecordIndex(%d) invalid", __func__, __LINE__, mPostRecordIndex);
        index = 0;
    }

    ExynosBuffer rawFrame422Buf;

    int dstW = mPictureSize.width;
    int dstH = mPictureSize.height;


    rawFrame422Buf.size.extS[0] = dstW * dstH * 2;

    if (allocMem(mIonCameraClient, &rawFrame422Buf, 1 << 1) == false) {
        CLOGE("ERR(%s):(%d)allocMem(rawFrame422Buf) fail", __FUNCTION__, __LINE__);
        mPictureLock.unlock();
        goto out;
    }

    /* csc start flite(s) -> rawFrame422Buf */
    if (nativeCSCRecordingCapture(&(mFliteNode.buffer[index]), &rawFrame422Buf) != NO_ERROR)
        ALOGE("ERR(%s):(%d)nativeCSCRecordingCapture() fail", __FUNCTION__, __LINE__);

    ret = nativeGetRecordingJpeg(&rawFrame422Buf, dstW, dstH);
    if (CC_UNLIKELY(!ret)) {
        CLOGE("doRecordingCapture: error, nativeGetRecordingJpeg");
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
        goto out;
    }

    mPictureLock.unlock();

#if DUMP_FILE
    dumpToFile(mJpegHeap->data, mPictureFrameSize, "/data/media/0/capture01.jpg");
#endif

    if ((mMsgEnabled & CAMERA_MSG_RAW_IMAGE) && (mRawHeap != NULL))
        mDataCb(CAMERA_MSG_RAW_IMAGE, mRawHeap, 0, NULL, mCallbackCookie);

    if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY)
        mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);

    if (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE)
        mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);

out:
    freeMem(&rawFrame422Buf);

    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();

    CLOGI("INFO(%s) : out ",__FUNCTION__);
    return NO_ERROR;
}
#endif

bool ISecCameraHardware::HDRPictureThread()
{
    int i, ncnt;
	int count = 3;
    int postviewOffset;
    int err;

    ALOGD("HDRPictureThread E");

    mPictureLock.lock();

    LOG_PERFORMANCE_START(1);

//	nativeSetParameters(CAM_CID_CONTINUESHOT_PROC, V4L2_INT_STATE_START_CAPTURE);

#if 0
    for ( i = count ; i ; i--) {
        ncnt = count-i+1;
        ALOGD("HDRPictureThread: AEB %d frame", ncnt);
        nativeSetParameters(CAM_CID_CONTINUESHOT_PROC, V4L2_INT_STATE_FRAME_SYNC);
        if (mMsgEnabled & CAMERA_MSG_SHUTTER) {
            mPictureLock.unlock();
            mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);
            mPictureLock.lock();
        }
    }
#endif

    for ( i = count ; i ; i--) {

        if (i != count) {
            mPictureLock.lock();
        }

        if (!nativeStartYUVSnapshot()) {
	        ALOGE("HDRPictureThread: error, nativeStartYUVSnapshot");
	        mPictureLock.unlock();
	        goto out;
	    }
	    ALOGE("nativeGetYUVSnapshot: count[%d], i[%d]",count,i);

	    if (!nativeGetYUVSnapshot(count-i+1, &postviewOffset)) {
	        ALOGE("HDRPictureThread: error, nativeGetYUVSnapshot");
	        mPictureLock.unlock();
	        mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
	        goto out;
	    } else {
	        ALOGE("HDRPictureThread: success, nativeGetYUVSnapshot");
	    }

		mPictureLock.unlock();

        if (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) {
            ALOGD("YUV mHDRTotalFrameSize(mHDRFrameSize) = %d, %d frame", mHDRFrameSize, i);
            mPictureLock.unlock();
            mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mHDRHeap, 0, NULL, mCallbackCookie);
        }
        nativeStopSnapshot();
    }

out:
    nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();

    LOG_PERFORMANCE_END(1, "total");

    ALOGD("HDRPictureThread X");
    return false;

}

 bool ISecCameraHardware::RecordingPictureThread()
{
    int i, ncnt;
	int count = mMultiFullCaptureNum;
    int postviewOffset;
    int err;

    ALOGD("RecordingPictureThread : count (%d)E", count);

	if (mCaptureMode == RUNNING_MODE_SINGLE) {
		if (mPreviewRunning && !mFullPreviewRunning) {
			ALOGW("RecordingPictureThread: warning, preview is running");
			stopPreview();
		}
	}

    mPictureLock.lock();

    LOG_PERFORMANCE_START(1);
    ALOGD("RecordingPictureThread : postview width(%d), height(%d)",
        mOrgPreviewSize.width, mOrgPreviewSize.height);
    //nativeSetParameters(CAM_CID_SET_POSTVIEW_SIZE,
    //    (int)(mOrgPreviewSize.width<<16|mOrgPreviewSize.height));
    //nativeSetParameters(CAM_CID_CONTINUESHOT_PROC, V4L2_INT_STATE_START_CAPTURE);

    for (i = count ; i ; i--) {
        ncnt = count-i+1;
        ALOGD("RecordingPictureThread: StartPostview %d frame E", ncnt);
        if (!nativeStartPostview()) {
            ALOGE("RecordingPictureThread: error, nativeStartPostview");
            mPictureLock.unlock();
            goto out;
        } else {
            ALOGD("RecordingPictureThread: StartPostview %d frame X", ncnt);
        }

        if (!nativeGetPostview(ncnt)) {
            ALOGE("RecordingPictureThread: error, nativeGetPostview");
            mPictureLock.unlock();
            mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
            goto out;
        } else {
            ALOGE("RecordingPictureThread: nativeGetPostview");
        }

        if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
            ALOGD("RAW mRawFrameSize = %d", mRawFrameSize);
            mDataCb(CAMERA_MSG_RAW_IMAGE, mPostviewHeap, 0, NULL, mCallbackCookie);
        }
        if (mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) {
            ALOGD("JPEG mRawFrameSize = %d", mPostviewFrameSize);
            mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mPostviewHeap, 0, NULL, mCallbackCookie);
        }

        LOG_PERFORMANCE_START(2);
        ALOGD("RecordingPictureThread: StartSnapshot %d frame E", ncnt);
        if (!nativeStartSnapshot()) {
            ALOGE("RecordingPictureThread: error, nativeStartSnapshot");
            mPictureLock.unlock();
            goto out;
        } else {
            ALOGD("RecordingPictureThread: StartSnapshot %d frame X", ncnt);
        }
        LOG_PERFORMANCE_END(2, "nativeStartSnapshot");

        LOG_PERFORMANCE_START(3);
        if (!nativeGetSnapshot(ncnt, &postviewOffset)) {
            ALOGE("RecordingPictureThread: error, nativeGetSnapshot");
            mPictureLock.unlock();
            mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
            goto out;
        } else {
            ALOGD("RecordingPictureThread: nativeGetSnapshot %d frame ", ncnt);
        }
        LOG_PERFORMANCE_END(3, "nativeGetSnapshot");

        mPictureLock.unlock();

        LOG_PERFORMANCE_START(4);

        if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
            mDataCb(CAMERA_MSG_RAW_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);
        }
        if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY) {
            mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
        }
        if ((mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) && (mJpegHeap != NULL)) {
            ALOGD("RecordingPictureThread: CAMERA_MSG_COMPRESSED_IMAGE (%d) ", ncnt);
            mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);

            // This delay is added for waiting until saving jpeg file is completed.
            // if it take short time to save jpeg file, this delay can be deleted.
            usleep(300*1000);
        }

        LOG_PERFORMANCE_END(4, "callback functions");
    }

out:
    nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();

    LOG_PERFORMANCE_END(1, "total");

    ALOGD("RecordingPictureThread X");
    return false;

}

bool ISecCameraHardware::dumpPictureThread()
{
	int err;
	mPictureRunning = true;

	if (mPreviewRunning && !mFullPreviewRunning) {
		ALOGW("takePicture: warning, preview is running");
		stopPreview();
	}

	/* Start fast capture */
	mCaptureStarted = true;
//	err = nativeSetParameters(CAM_CID_SET_FAST_CAPTURE, 0);
	mCaptureStarted = false;
	if (mCancelCapture/* && mSamsungApp*/) {
		ALOGD("pictureThread mCancelCapture %d", mCancelCapture);
		mCancelCapture = false;
		return false;
	}
	if (err != NO_ERROR) {
		ALOGE("%s: Fast capture command is failed.", __func__);
	} else {
		ALOGD("%s: Mode change command is issued for fast capture.", __func__);
	}

    mPictureLock.lock();

    LOG_PERFORMANCE_START(1);
	LOG_PERFORMANCE_START(2);
    if (!nativeStartSnapshot()) {
        ALOGE("pictureThread: error, nativeStartSnapshot");
        mPictureLock.unlock();
        goto out;
    }
	LOG_PERFORMANCE_END(2, "nativeStartSnapshot");

    LOG_PERFORMANCE_START(3);
	/* --4 */
    int postviewOffset;

	if (!nativeGetSnapshot(1, &postviewOffset)) {
		ALOGE("pictureThread: error, nativeGetSnapshot");
		mPictureLock.unlock();
		mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
		goto out;
	}
	LOG_PERFORMANCE_END(3, "nativeGetSnapshot");

	mPictureLock.unlock();

	/* Display postview */
	LOG_PERFORMANCE_START(4);

	/* callbacks for capture */
	if (mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) {
		ALOGD("Postview mPostviewFrameSize = %d", mPostviewFrameSize);
		mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mPostviewHeap, 0, NULL, mCallbackCookie);
	}
	if ((mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) && (mJpegHeap != NULL)) {
		ALOGD("JPEG COMPLRESSED");
		mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);
	}
	if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
		ALOGD("RAW mRawFrameSize = %d", mRawFrameSize);
		mDataCb(CAMERA_MSG_RAW_IMAGE, mPostviewHeap, 0, NULL, mCallbackCookie);
	}
	if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY) {
		ALOGD("RAW image notify");
		mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
	}

    LOG_PERFORMANCE_END(4, "callback functions");

#if DUMP_FILE
    dumpToFile(mJpegHeap->data, mPictureFrameSize, "/data/media/0/capture01.jpg");
#endif

out:
    nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();
    LOG_PERFORMANCE_END(1, "total");

    ALOGD("pictureThread X");
    return false;
}

status_t ISecCameraHardware::pictureThread_RAW()
{
    int postviewOffset;
	int jpegSize;
	uint8_t *RAWsrc;
    ALOGE("pictureThread_RAW E");

    mPictureLock.lock();

//	nativeSetParameters(CAM_CID_MAIN_FORMAT, 5);

    if (!nativeStartSnapshot()) {
        ALOGE("pictureThread: error, nativeStartSnapshot");
        mPictureLock.unlock();
        goto out;
    }

    if (!nativeGetSnapshot(0, &postviewOffset)) {	//if (!nativeGetSnapshot(1, &postviewOffset)) {
        ALOGE("pictureThread: error, nativeGetSnapshot");
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
        goto out;
    }

    RAWsrc = (uint8_t *)mPictureBufDummy[0].virt.extP[0];

    if(mJpegHeap != NULL)
        memcpy((uint8_t *)mJpegHeap->data, RAWsrc, mPictureFrameSize );

    if ((mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME) && (mJpegHeap != NULL)) {	//CAMERA_MSG_COMPRESSED_IMAGE
        ALOGD("RAW COMPLRESSED");
        mDataCb(CAMERA_MSG_POSTVIEW_FRAME, mJpegHeap, 0, NULL, mCallbackCookie);	//CAMERA_MSG_COMPRESSED_IMAGE
    }

//    nativeSetParameters(CAM_CID_MAIN_FORMAT, 1);

    if (!nativeStartSnapshot()) {
        ALOGE("pictureThread: error, nativeStartSnapshot");
        mPictureLock.unlock();
        goto out;
    }

    if (!nativeGetSnapshot(1, &postviewOffset)) {
        ALOGE("pictureThread_RAW : error, nativeGetMultiSnapshot");
        mPictureLock.unlock();
        mNotifyCb(CAMERA_MSG_ERROR, -1, 0, mCallbackCookie);
        goto out;
    }

    if (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) {
		ALOGD("JPEG COMPRESSED");
        mPictureLock.unlock();
        mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mJpegHeap, 0, NULL, mCallbackCookie);
        mPictureLock.lock();
    }

	if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY) {
		ALOGD("RAW image notify");
		mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, mCallbackCookie);
	}

    mPictureLock.unlock();

    out:
    nativeStopSnapshot();
    mLock.lock();
    mPictureRunning = false;
    mLock.unlock();

    ALOGD("pictureThread_RAW X");
    return false;
}

status_t ISecCameraHardware::cancelPicture()
{
    mPictureThread->requestExitAndWait();

    ALOGD("cancelPicture EX");
    return NO_ERROR;
}

status_t ISecCameraHardware::sendCommand(int32_t command, int32_t arg1, int32_t arg2)
{
    CLOGV("INFO(%s) : in - command %d, arg1 %d, arg2 %d", __FUNCTION__,command, arg1, arg2);
    int max;

    switch(command) {
    case CAMERA_CMD_DISABLE_POSTVIEW:
        mDisablePostview = arg1;
        break;

    case CAMERA_CMD_SET_TOUCH_AF_POSITION:
        CLOGD("CAMERA_CMD_SET_TOUCH_AF_POSITION  X:%d, Y:%d", arg1, arg2);
        nativeSetParameters(CAM_CID_SET_TOUCH_AF_POSX, arg1);
        nativeSetParameters(CAM_CID_SET_TOUCH_AF_POSY, arg2);
        break;

    case CAMERA_CMD_START_STOP_TOUCH_AF:
        if (!mPreviewRunning && arg1) {
            CLOGW("Preview is not started before Touch AF");
            return NO_INIT;
        }
        CLOGD("CAMERA_CMD_START_STOP_TOUCH_AF  ~~~~~~~~~ arg1 == %d", arg1);
        nativeSetParameters(CAM_CID_SET_TOUCH_AF, arg1);
        break;

    case CAMERA_CMD_SET_FLIP:
        CLOGD("CAMERA_CMD_SET_FLIP arg1 == %d", arg1);
        mflipHorizontal = arg1;
        break;

    default:
        CLOGV("no matching case");
        break;
    }

    return NO_ERROR;
}

void ISecCameraHardware::release()
{
    if (mPreviewThread != NULL) {
        mPreviewThread->requestExitAndWait();
        mPreviewThread.clear();
    }

    if (mPreviewZoomThread != NULL) {
        mPreviewZoomThread->requestExitAndWait();
        mPreviewZoomThread.clear();
    }

#ifdef USE_DEDICATED_PREVIEW_ENQUEUE_THREAD
    if(mPreviewEnqueueThread != NULL) {
        mPreviewEnqueueThread->requestExitAndWait();
        mPreviewEnqueueThread.clear();
    }
#endif

#if FRONT_ZSL
    if (mZSLPictureThread != NULL) {
        mZSLPictureThread->requestExitAndWait();
        mZSLPictureThread.clear();
    }
#endif

    if (mRecordingThread != NULL) {
        mRecordingThread->requestExitAndWait();
        mRecordingThread.clear();
    }

    if (mPostRecordThread != NULL) {
        mPostRecordThread->requestExit();
        mPostRecordExit = true;
        mPostRecordCondition.signal();
        mPostRecordThread->requestExitAndWait();
        mPostRecordThread.clear();
    }

    if (mAutoFocusThread != NULL) {
    /* this thread is normally already in it's threadLoop but blocked
    * on the condition variable.  signal it so it wakes up and can exit.
    */
        mAutoFocusThread->requestExit();
        mAutoFocusExit = true;
        mAutoFocusCondition.signal();
        mAutoFocusThread->requestExitAndWait();
        mAutoFocusThread.clear();
    }

    if (mPictureThread != NULL) {
        mPictureThread->requestExitAndWait();
        mPictureThread.clear();
    }

    if (mPreviewHeap) {
        mPreviewHeap->release(mPreviewHeap);
        mPreviewHeap = 0;
        mPreviewHeapFd = -1;
    }

    if (mRecordingHeap) {
        mRecordingHeap->release(mRecordingHeap);
        mRecordingHeap = 0;
    }

    if (mRawHeap != NULL) {
        mRawHeap->release(mRawHeap);
        mRawHeap = 0;
    }

    if (mJpegHeap) {
        mJpegHeap->release(mJpegHeap);
        mJpegHeap = 0;
    }

    if (mHDRHeap) {
        mHDRHeap->release(mHDRHeap);
        mHDRHeap = 0;
    }

    if (mPostviewHeap) {
        mPostviewHeap->release(mPostviewHeap);
        mPostviewHeap = 0;
    }

    if (mHDRPictureThread != NULL) {
        mHDRPictureThread->requestExitAndWait();
        mHDRPictureThread.clear();
    }
 
    if (mRecordingPictureThread != NULL) {
        mRecordingPictureThread->requestExitAndWait();
        mRecordingPictureThread.clear();
    }

    if (mDumpPictureThread != NULL) {
        mDumpPictureThread->requestExitAndWait();
        mDumpPictureThread.clear();
    }

    nativeDestroySurface();
}

status_t ISecCameraHardware::dump(int fd) const
{
    return NO_ERROR;
}

int ISecCameraHardware::getCameraId() const
{
    return mCameraId;
}

status_t ISecCameraHardware::setParameters(const CameraParameters &params)
{
    LOG_PERFORMANCE_START(1);

    if (mPictureRunning) {
        CLOGW("WARN(%s): warning, capture is not complete. please wait...", __FUNCTION__);
        Mutex::Autolock l(&mPictureLock);
    }

    Mutex::Autolock l(&mLock);

    CLOGD("DEBUG(%s): [Before Param] %s", __FUNCTION__, params.flatten().string());

    status_t rc, final_rc = NO_ERROR;
    if ((rc = setRecordingMode(params)))
        final_rc = rc;
    if ((rc = setVideoSize(params)))
        final_rc = rc;
    if ((rc = setPreviewSize(params)))
        final_rc = rc;
    if ((rc = setPreviewFormat(params)))
        final_rc = rc;
    if ((rc = setPictureSize(params)))
        final_rc = rc;
    if ((rc = setPictureFormat(params)))
        final_rc = rc;
    if ((rc = setThumbnailSize(params)))
        final_rc = rc;
    if ((rc = setJpegThumbnailQuality(params)))
        final_rc = rc;
    if ((rc = setJpegQuality(params)))
        final_rc = rc;
    if ((rc = setFrameRate(params)))
        final_rc = rc;
    if ((rc = setRotation(params)))
        final_rc = rc;
    if ((rc = setFocusMode(params)))
        final_rc = rc;
    /* Set anti-banding both rear and front camera if needed. */
    if ((rc = setAntiBanding(params)))
        final_rc = rc;
    /* UI settings */
    if (mCameraId == CAMERA_ID_BACK) {
        if ((rc = setSceneMode(params)))    final_rc = rc;
        if ((rc = setFocusAreas(params)))    final_rc = rc;
        if ((rc = setFlash(params)))    final_rc = rc;
        if ((rc = setMetering(params)))    final_rc = rc;
        if ((rc = setMeteringAreas(params)))    final_rc = rc;
        /* Set for Adjust Image */
        if ((rc = setSharpness(params)))    final_rc = rc;
        if ((rc = setContrast(params)))    final_rc = rc;
        if ((rc = setSaturation(params)))    final_rc = rc;
        if ((rc = setAntiShake(params)))    final_rc = rc;
        /* setCapturemode: Do not set before setPASMmode() */
        if ((rc = setZoom(params)))    final_rc = rc;
        if ((rc = setDzoom(params)))    final_rc = rc;
    } else {
        if ((rc = setBlur(params)))    final_rc = rc;
    }
    if ((rc = setZoom(params)))    final_rc = rc;
    if ((rc = setWhiteBalance(params)))    final_rc = rc;
    if ((rc = setEffect(params)))    final_rc = rc;
    if ((rc = setBrightness(params)))    final_rc = rc;
    if ((rc = setIso(params)))    final_rc = rc;
    if ((rc = setAELock(params)))    final_rc = rc;
    if ((rc = setAWBLock(params)))    final_rc = rc;
    if ((rc = setGps(params)))    final_rc = rc;

    checkHorizontalViewAngle();

    LOG_PERFORMANCE_END(1, "total");

    CLOGD("DEBUG(%s): [After Param] %s", __FUNCTION__, params.flatten().string());

    CLOGD("DEBUG(%s): out - %s", __FUNCTION__,  final_rc == NO_ERROR ? "success" : "failed");
    return final_rc;
}

void ISecCameraHardware::setDropFrame(int count)
{
    /* should be locked */
    if (mDropFrameCount < count)
        mDropFrameCount = count;

    CLOGD("DEBUG(%s): mDropFrameCount = %d", __FUNCTION__, mDropFrameCount);
}

status_t ISecCameraHardware::setAELock(const CameraParameters &params)
{
    const char *str_support = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED);
    if (str_support == NULL || (!strcmp(str_support, "false"))) {
        CLOGW("WARN(%s): warning, not supported",__FUNCTION__);
        return NO_ERROR;
    }

    const char *str = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
    const char *prevStr = mParameters.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
    if (str == NULL || (prevStr && !strcmp(str, prevStr))) {
        return NO_ERROR;
    }

    CLOGI("DEBUG(%s): %s", __FUNCTION__, str);
    if (!(!strcmp(str, "true") || !strcmp(str, "false"))) {
        CLOGE("ERR(%s): error, invalid value %s",__FUNCTION__,  str);
        return BAD_VALUE;
    }

    mParameters.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, str);

    int val;
    if (!strcmp(str, "true")) {
        val = AE_LOCK;
    } else {
        val = AE_UNLOCK;
    }

    return nativeSetParameters(CAM_CID_AE_LOCK_UNLOCK, val);
}

status_t ISecCameraHardware::setAWBLock(const CameraParameters &params)
{
    const char *str_support = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED);
    if (str_support == NULL || (!strcmp(str_support, "false"))) {
        CLOGW("WARN(%s): warning, not supported",__FUNCTION__);
        return NO_ERROR;
    }

    const char *str = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK);
    const char *prevStr = mParameters.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK);
    if (str == NULL || (prevStr && !strcmp(str, prevStr))) {
        return NO_ERROR;
    }

    CLOGI("DEBUG(%s): %s", __FUNCTION__, str);
    if (!(!strcmp(str, "true") || !strcmp(str, "false"))) {
        CLOGE("ERR(%s): error, invalid value %s",__FUNCTION__,  str);

        return BAD_VALUE;
    }

    mParameters.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, str);

    int val;
    if (!strcmp(str, "true")) {
        val = AWB_LOCK;
    } else {
        val = AWB_UNLOCK;
    }

    return nativeSetParameters(CAM_CID_AWB_LOCK_UNLOCK, val);
}

/*
* called when starting preview
*/
inline void ISecCameraHardware::setDropUnstableInitFrames()
{
    int32_t frameCount = 3;

    if (mCameraId == CAMERA_ID_BACK) {
        if (mbFirst_preview_started == false) {
            /* When camera_start_preview is called for the first time after camera application starts. */
            if (mMovieMode == true) {
                frameCount = INITIAL_REAR_SKIP_FRAME + 5;
            } else {
                frameCount = INITIAL_REAR_SKIP_FRAME;
            }
            mbFirst_preview_started = true;
        } else {
            /* When startPreview is called after camera application got started. */
            frameCount = INITIAL_REAR_SKIP_FRAME;
        }
    } else {
        if (mbFirst_preview_started == false) {
            /* When camera_start_preview is called for the first time after camera application starts. */
            frameCount = INITIAL_FRONT_SKIP_FRAME;
            mbFirst_preview_started = true;
        } else {
            /* When startPreview is called after camera application got started. */
            frameCount = INITIAL_FRONT_SKIP_FRAME;
        }
    }

    setDropFrame(frameCount);
}

status_t ISecCameraHardware::setRecordingMode(const CameraParameters &params)
{
    const char *str = params.get(CameraParameters::KEY_RECORDING_HINT);
    const char *prevStr = mParameters.get(CameraParameters::KEY_RECORDING_HINT);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    mParameters.set(CameraParameters::KEY_RECORDING_HINT, str);

    String8 recordHint(str);
    CLOGD("DEBUG(%s): %s", __FUNCTION__, recordHint.string());

    if (recordHint == "true"){
        mFps = mMaxFrameRate / 1000;
        CLOGD("DEBUG(%s): fps(%d) %s ", __FUNCTION__, mFps, recordHint.string());

        mMovieMode = true;
    } else {
        mMovieMode = false;
    }

    return 0 /*nativeSetParameters(CAM_CID_MOVIE_MODE, mMovieMode)*/;

}

status_t ISecCameraHardware::setPreviewSize(const CameraParameters &params)
{
    int width, height;
    params.getPreviewSize(&width, &height);

    if ((mPreviewSize.width == (uint32_t)width) && (mPreviewSize.height == (uint32_t)height)
#if defined(USE_RECORDING_FLITE_SIZE)
        && (mPrevMovieMode == mMovieMode)
#endif
    ) {
        return NO_ERROR;
    }

    if (width <= 0 || height <= 0)
        return BAD_VALUE;

    int count, FLiteCount;
    const image_rect_type *sizes, *FLiteSizes, *defaultSizes = NULL, *size = NULL, *fliteSize = NULL;;
    const image_rect_type *sameRatioSize = NULL;

    if (mCameraId == CAMERA_ID_BACK) {
        count = ARRAY_SIZE(backPreviewSizes);
        sizes = backPreviewSizes;
#if defined(USE_RECORDING_FLITE_SIZE)
        if (mMovieMode == true) {
            FLiteCount = ARRAY_SIZE(backFLiteRecordingSizes);
            FLiteSizes = backFLiteRecordingSizes;
        } else
#endif
        {
            FLiteCount = ARRAY_SIZE(backFLitePreviewSizes);
            FLiteSizes = backFLitePreviewSizes;
        }
        mPrevMovieMode = mMovieMode;
    } else {
        count = ARRAY_SIZE(frontPreviewSizes);
        sizes = frontPreviewSizes;
        FLiteCount = ARRAY_SIZE(frontFLitePreviewSizes);
        FLiteSizes = frontFLitePreviewSizes;
    }

retry:
    size = getFrameSizeSz(sizes, count, (uint32_t)width, (uint32_t)height);

    if (CC_UNLIKELY(!size)) {
        if (!defaultSizes) {
            defaultSizes = sizes;
            sameRatioSize = getFrameSizeRatio(sizes, count, (uint32_t)width, (uint32_t)height);
            if (mCameraId == CAMERA_ID_BACK) {
                count = ARRAY_SIZE(hiddenBackPreviewSizes);
                sizes = hiddenBackPreviewSizes;
            } else {
                count = ARRAY_SIZE(hiddenFrontPreviewSizes);
                sizes = hiddenFrontPreviewSizes;
            }
            goto retry;
        } else if (!sameRatioSize) {
            sameRatioSize = getFrameSizeRatio(sizes, count, (uint32_t)width, (uint32_t)height);
        }

        CLOGW("WARN(%s): not supported size(%dx%d)", __FUNCTION__, width, height);
        size = sameRatioSize ? sameRatioSize : &defaultSizes[0];
    }

    mPreviewSize = *size;
    mParameters.setPreviewSize((int)size->width, (int)size->height);

    fliteSize = getFrameSizeRatio(FLiteSizes, FLiteCount, size->width, size->height);
    mFLiteSize = fliteSize ? *fliteSize : FLiteSizes[0];

    /* backup orginal preview size due to ALIGN */
    mOrgPreviewSize = mPreviewSize;
    mPreviewSize.width = ALIGN(mPreviewSize.width, 16);
    mPreviewSize.height = ALIGN(mPreviewSize.height, 2);

    CLOGD("DEBUG(%s): preview size %dx%d/%dx%d/%dx%d", __FUNCTION__, width, height,
            mOrgPreviewSize.width, mOrgPreviewSize.height, mPreviewSize.width, mPreviewSize.height);
    CLOGD("DEBUG(%s): Flite size %dx%d", __FUNCTION__,mFLiteSize.width, mFLiteSize.height);

    return NO_ERROR;
}

status_t ISecCameraHardware::setPreviewFormat(const CameraParameters &params)
{
    const char *str = params.getPreviewFormat();
    const char *prevStr = mParameters.getPreviewFormat();
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(previewPixelFormats, ARRAY_SIZE(previewPixelFormats), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        CLOGW("setPreviewFormat: warning, not supported value(%s)", str);
        str = reinterpret_cast<const char*>(previewPixelFormats[0].desc);
        goto retry;
    }

    CLOGD("DEBUG(%s): %s", __FUNCTION__, str);
    mPreviewFormat = (cam_pixel_format)val;
    CLOGV("DEBUG(%s): mPreviewFormat = %s",
        __FUNCTION__,
        mPreviewFormat == CAM_PIXEL_FORMAT_YVU420P ? "YV12" :
        mPreviewFormat == CAM_PIXEL_FORMAT_YUV420SP ? "NV21" :
        "Others");
    mParameters.setPreviewFormat(str);
    return NO_ERROR;
}

status_t ISecCameraHardware::setVideoSize(const CameraParameters &params)
{

    int width = 0, height = 0;
    params.getVideoSize(&width, &height);

    if ((mVideoSize.width == (uint32_t)width) && (mVideoSize.height == (uint32_t)height))
        return NO_ERROR;

    int count;
    const image_rect_type *sizes, *defaultSizes = NULL, *size = NULL;

    if (mCameraId == CAMERA_ID_BACK) {
        count = ARRAY_SIZE(backRecordingSizes);
        sizes = backRecordingSizes;
    } else {
        count = ARRAY_SIZE(frontRecordingSizes);
        sizes = frontRecordingSizes;
    }

retry:
    for (int i = 0; i < count; i++) {
        if (((uint32_t)width == sizes[i].width) && ((uint32_t)height == sizes[i].height)) {
            size = &sizes[i];
            break;
        }
    }

    if (CC_UNLIKELY(!size)) {
        if (!defaultSizes) {
            defaultSizes = sizes;
            if (mCameraId == CAMERA_ID_BACK) {
                count = ARRAY_SIZE(hiddenBackRecordingSizes);
                sizes = hiddenBackRecordingSizes;
            } else {
                count = ARRAY_SIZE(hiddenFrontRecordingSizes);
                sizes = hiddenFrontRecordingSizes;
            }
            goto retry;
        } else {
            sizes = defaultSizes;
        }

        CLOGW("WARN(%s): warning, not supported size (%dx%d)", __FUNCTION__, size->width, size->height);
        size = &sizes[0];
    }

    CLOGD("DEBUG(%s): recording %dx%d", __FUNCTION__, size->width, size->height);

    mVideoSize = *size;
    mParameters.setVideoSize((int)size->width, (int)size->height);

    /* const char *str = mParameters.get(CameraParameters::KEY_VIDEO_SIZE); */
    return NO_ERROR;
}

status_t ISecCameraHardware::setPictureSize(const CameraParameters &params)
{
    int width, height;
    params.getPictureSize(&width, &height);
    int right_ratio = 177;

    if ((mPictureSize.width == (uint32_t)width) && (mPictureSize.height == (uint32_t)height))
        return NO_ERROR;

    int count, FLiteCount;
    const image_rect_type *sizes, *FLiteSizes, *defaultSizes = NULL, *size = NULL, *fliteSize = NULL;
    const image_rect_type *sameRatioSize = NULL;

    if (mCameraId == CAMERA_ID_BACK) {
        count = ARRAY_SIZE(backPictureSizes);
        sizes = backPictureSizes;
        FLiteCount = ARRAY_SIZE(backFLiteCaptureSizes);
        FLiteSizes = backFLiteCaptureSizes;
    } else {
        count = ARRAY_SIZE(frontPictureSizes);
        sizes = frontPictureSizes;
        FLiteCount = ARRAY_SIZE(frontFLiteCaptureSizes);
        FLiteSizes = frontFLiteCaptureSizes;
    }

retry:
    size = getFrameSizeSz(sizes, count, (uint32_t)width, (uint32_t)height);

    if (CC_UNLIKELY(!size)) {
        if (!defaultSizes) {
            defaultSizes = sizes;
	    sameRatioSize = getFrameSizeRatio(sizes, count, (uint32_t)width, (uint32_t)height);
            if (mCameraId == CAMERA_ID_BACK) {
                count = ARRAY_SIZE(hiddenBackPictureSizes);
                sizes = hiddenBackPictureSizes;
            } else {
                count = ARRAY_SIZE(hiddenFrontPictureSizes);
                sizes = hiddenFrontPictureSizes;
            }
            goto retry;
        } else if (!sameRatioSize) {
            sameRatioSize = getFrameSizeRatio(sizes, count, (uint32_t)width, (uint32_t)height);
        }

        CLOGW("DEBUG(%s): not supported size(%dx%d)",__FUNCTION__, width, height);
        size = sameRatioSize ? sameRatioSize : &defaultSizes[0];
    }

    CLOGD("DEBUG(%s): %dx%d", __FUNCTION__, size->width, size->height);
    mPictureSize = mRawSize = *size;
    mParameters.setPictureSize((int)size->width, (int)size->height);

    if ((int)(mSensorSize.width * 100 / mSensorSize.height) == right_ratio) {
        setHorizontalViewAngle(size->width, size->height);
    }
    mParameters.setFloat(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, getHorizontalViewAngle());

    fliteSize = getFrameSizeRatio(FLiteSizes, FLiteCount, size->width, size->height);
    mFLiteCaptureSize = fliteSize ? *fliteSize : FLiteSizes[0];

    return NO_ERROR;
}

status_t ISecCameraHardware::setPictureFormat(const CameraParameters &params)
{
    const char *str = params.getPictureFormat();
    const char *prevStr = mParameters.getPictureFormat();
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(picturePixelFormats,
        ARRAY_SIZE(picturePixelFormats), str);

    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setPictureFormat: warning, not supported value(%s)", str);
#if 0
        str = reinterpret_cast<const char*>(picturePixelFormats[0].desc);
        goto retry;
#else
        return BAD_VALUE;
#endif	/* FOR HAL TEST */
    }

    CLOGD("DEBUG(%s) : %s ",__FUNCTION__, str);
    mPictureFormat = (cam_pixel_format)val;
    mParameters.setPictureFormat(str);
    return NO_ERROR;
}

status_t ISecCameraHardware::setThumbnailSize(const CameraParameters &params)
{
    int width, height;
    width = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    height = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);

    if (mThumbnailSize.width == (uint32_t)width && mThumbnailSize.height == (uint32_t)height)
        return NO_ERROR;

    int count;
    const image_rect_type *sizes, *size = NULL;

    if (mCameraId == CAMERA_ID_BACK) {
        count = ARRAY_SIZE(backThumbSizes);
        sizes = backThumbSizes;
    } else {
        count = ARRAY_SIZE(frontThumbSizes);
        sizes = frontThumbSizes;
    }

    for (int i = 0; i < count; i++) {
        if ((uint32_t)width == sizes[i].width && (uint32_t)height == sizes[i].height) {
            size = &sizes[i];
            break;
        }
    }

    if (!size) {
        CLOGW("setThumbnailSize: warning, not supported size(%dx%d)", width, height);
        size = &sizes[0];
    }

    CLOGD("DEBUG(%s) : %dx%d ",__FUNCTION__, size->width, size->height);
    mThumbnailSize = *size;
    mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH, (int)size->width);
    mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, (int)size->height);

    return NO_ERROR;
}

status_t ISecCameraHardware::setJpegThumbnailQuality(const CameraParameters &params)
{
    int val = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    int prevVal = mParameters.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    if (val == -1 || prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < 1 || val > 100)) {
        CLOGE("setJpegThumbnailQuality: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    //CLOGD("setJpegThumbnailQuality: %d", val);
    CLOGD("DEBUG(%s) : %d ",__FUNCTION__, val);
	
    mParameters.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, val);

    return 0;
}

status_t ISecCameraHardware::setJpegQuality(const CameraParameters &params)
{
    int val = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
    int prevVal = mParameters.getInt(CameraParameters::KEY_JPEG_QUALITY);
    if (val == -1 || prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < 1 || val > 100)) {
        ALOGE("setJpegQuality: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    //ALOGD("setJpegQuality: %d", val);
    CLOGD("DEBUG(%s) : %d ",__FUNCTION__, val);
    mJpegQuality = val;

    mParameters.set(CameraParameters::KEY_JPEG_QUALITY, val);

    return nativeSetParameters(CAM_CID_JPEG_QUALITY, mJpegQuality);
}

status_t ISecCameraHardware::setFrameRate(const CameraParameters &params)
{
    int min, max;
    params.getPreviewFpsRange(&min, &max);
    int frameRate = params.getPreviewFrameRate();
    int prevFrameRate = mParameters.getPreviewFrameRate();
    if ((frameRate != -1) && (frameRate != prevFrameRate))
        mParameters.setPreviewFrameRate(frameRate);

    if (CC_UNLIKELY(min < 0 || max < 0 || max < min)) {
        CLOGE("setFrameRate: error, invalid range(%d, %d)", min, max);
        return BAD_VALUE;
    }

    /* 0 means auto frame rate */
    int val = (min == max) ? min : 0;
    mMaxFrameRate = max;

    if (mMovieMode)
        mFps = mMaxFrameRate / 1000;
    else
        mFps = val / 1000;

    CLOGD("DEBUG(%s) : %d,%d,%d ",__FUNCTION__, min, max, mFps);

    if (mFrameRate == val)
        return NO_ERROR;

    mFrameRate = val;

    const char *str = params.get(CameraParameters::KEY_PREVIEW_FPS_RANGE);
    if (CC_LIKELY(str)) {
        mParameters.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, str);
    } else {
        CLOGE("setFrameRate: corrupted data (params)");
        char buffer[32];
        CLEAR(buffer);
        snprintf(buffer, sizeof(buffer), "%d,%d", min, max);
        mParameters.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, buffer);
    }

    mParameters.setPreviewFrameRate(val/1000);

    return nativeSetParameters(CAM_CID_FRAME_RATE, val/1000);
}

status_t ISecCameraHardware::setRotation(const CameraParameters &params)
{
    int val = params.getInt(CameraParameters::KEY_ROTATION);
    int prevVal = mParameters.getInt(CameraParameters::KEY_ROTATION);
    if (val == -1 || prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val != 0 && val != 90 && val != 180 && val != 270)) {
        ALOGE("setRotation: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    CLOGD("DEBUG(%s) : %d ",__FUNCTION__, val);
    mParameters.set(CameraParameters::KEY_ROTATION, val);

    return NO_ERROR;
}

status_t ISecCameraHardware::setPreviewFrameRate(const CameraParameters &params)
{
    int val = params.getPreviewFrameRate();
    int prevVal = mParameters.getPreviewFrameRate();
    if (val == -1 || prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < 0 || val > (mMaxFrameRate / 1000) )) {
        ALOGE("setPreviewFrameRate: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    CLOGD("DEBUG(%s) : %d ",__FUNCTION__, val);
    mFrameRate = val * 1000;
    mParameters.setPreviewFrameRate(val);

    return NO_ERROR;
}

status_t ISecCameraHardware::setSceneMode(const CameraParameters &params)
{
    const char *str = params.get(CameraParameters::KEY_SCENE_MODE);
    const char *prevStr = mParameters.get(CameraParameters::KEY_SCENE_MODE);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;

retry:
    val = SecCameraParameters::lookupAttr(sceneModes, ARRAY_SIZE(sceneModes), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setSceneMode: warning, not supported value(%s)", str);
#if 0
        str = reinterpret_cast<const char*>(sceneModes[0].desc);
        goto retry;
#else
        return BAD_VALUE;
#endif	/* FOR HAL TEST */
    }

    //ALOGD("setSceneMode: %s", str);
    CLOGD("DEBUG(%s) : %s ",__FUNCTION__, str);

    mSceneMode = (cam_scene_mode)val;
    mParameters.set(CameraParameters::KEY_SCENE_MODE, str);

    return nativeSetParameters(CAM_CID_SCENE_MODE, val);
}

/* -------------------Focus Area STARTS here---------------------------- */
status_t ISecCameraHardware::findCenter(struct FocusArea *focusArea,
        struct FocusPoint *center)
{
    /* range check */
    if ((focusArea->top > focusArea->bottom) || (focusArea->right < focusArea->left)) {
        CLOGE("findCenter: Invalid value range");
        return -EINVAL;
    }

    center->x = (focusArea->left + focusArea->right) / 2;
    center->y = (focusArea->top + focusArea->bottom) / 2;

    /* ALOGV("%s: center point (%d, %d)", __func__, center->x, center->y); */
    return NO_ERROR;
}

status_t ISecCameraHardware::normalizeArea(struct FocusPoint *center)
{
    struct FocusPoint tmpPoint;
    size_t hRange, vRange;
    double hScale, vScale;

    tmpPoint.x = center->x;
    tmpPoint.y = center->y;

    /* ALOGD("%s: before x = %d, y = %d", __func__, tmpPoint.x, tmpPoint.y); */

    hRange = FOCUS_AREA_RIGHT - FOCUS_AREA_LEFT;
    vRange = FOCUS_AREA_BOTTOM - FOCUS_AREA_TOP;
    hScale = (double)mPreviewSize.height / (double) hRange;
    vScale = (double)mPreviewSize.width / (double) vRange;

    /* Nomalization */
    /* ALOGV("normalizeArea: mPreviewSize.width = %d, mPreviewSize.height = %d",
            mPreviewSize.width, mPreviewSize.height);
     */

    tmpPoint.x = (center->x + vRange / 2) * vScale;
    tmpPoint.y = (center->y + hRange / 2) * hScale;

    center->x = tmpPoint.x;
    center->y = tmpPoint.y;

    if (center->x == 0 && center->y == 0) {
        CLOGE("normalizeArea: Invalid focus center point");
        return -EINVAL;
    }

    return NO_ERROR;
}

status_t ISecCameraHardware::checkArea(ssize_t top,
        ssize_t left,
        ssize_t bottom,
        ssize_t right,
        ssize_t weight)
{
    /* Handles the invalid regin corner case. */
    if ((0 == top) && (0 == left) && (0 == bottom) && (0 == right) && (0 == weight)) {
        ALOGD("checkArea: error, All values are zero");
        return NO_ERROR;
    }

    if ((FOCUS_AREA_WEIGHT_MIN > weight) || (FOCUS_AREA_WEIGHT_MAX < weight)) {
        ALOGE("checkArea: error, Camera area weight is invalid %d", weight);
        return -EINVAL;
    }

    if ((FOCUS_AREA_TOP > top) || (FOCUS_AREA_BOTTOM < top)) {
        ALOGE("checkArea: error, Camera area top coordinate is invalid %d", top );
        return -EINVAL;
    }

    if ((FOCUS_AREA_TOP > bottom) || (FOCUS_AREA_BOTTOM < bottom)) {
        ALOGE("checkArea: error, Camera area bottom coordinate is invalid %d", bottom );
        return -EINVAL;
    }

    if ((FOCUS_AREA_LEFT > left) || (FOCUS_AREA_RIGHT < left)) {
        ALOGE("checkArea: error, Camera area left coordinate is invalid %d", left );
        return -EINVAL;
    }

    if ((FOCUS_AREA_LEFT > right) || (FOCUS_AREA_RIGHT < right)) {
        ALOGE("checkArea: error, Camera area right coordinate is invalid %d", right );
        return -EINVAL;
    }

    if (left >= right) {
        ALOGE("checkArea: error, Camera area left larger than right");
        return -EINVAL;
    }

    if (top >= bottom) {
        ALOGE("checkArea: error, Camera area top larger than bottom");
        return -EINVAL;
    }

    return NO_ERROR;
}

/* TODO : muliple focus area is not supported yet */
status_t ISecCameraHardware::parseAreas(const char *area,
        size_t areaLength,
        struct FocusArea *focusArea,
        int *num_areas)
{
    status_t ret = NO_ERROR;
    char *ctx;
    char *pArea = NULL;
    char *pEnd = NULL;
    const char *startToken = "(";
    const char endToken = ')';
    const char sep = ',';
    ssize_t left, top, bottom, right, weight;
    char *tmpBuffer = NULL;

    if (( NULL == area ) || ( 0 >= areaLength)) {
        ALOGE("parseAreas: error, area is NULL or areaLength is less than 0");
        return -EINVAL;
    }

    tmpBuffer = (char *)malloc(areaLength);
    if (NULL == tmpBuffer) {
        ALOGE("parseAreas: error, tmpBuffer is NULL");
        return -ENOMEM;
    }

    memcpy(tmpBuffer, area, areaLength);

    pArea = strtok_r(tmpBuffer, startToken, &ctx);

    do {
		char *pStart = NULL;
        pStart = pArea;
        if (NULL == pStart) {
            ALOGE("parseAreas: error, Parsing of the left area coordinate failed!");
            ret = -EINVAL;
            break;
        } else {
            left = static_cast<ssize_t>(strtol(pStart, &pEnd, 10));
        }

        if (sep != *pEnd) {
            ALOGE("parseAreas: error, Parsing of the top area coordinate failed!");
            ret = -EINVAL;
            break;
        } else {
            top = static_cast<ssize_t>(strtol(pEnd + 1, &pEnd, 10));
        }

        if (sep != *pEnd) {
            ALOGE("parseAreas: error, Parsing of the right area coordinate failed!");
            ret = -EINVAL;
            break;
        } else {
            right = static_cast<ssize_t>(strtol(pEnd + 1, &pEnd, 10));
        }

        if (sep != *pEnd) {
            ALOGE("parseAreas: error, Parsing of the bottom area coordinate failed!");
            ret = -EINVAL;
            break;
        } else {
            bottom = static_cast<ssize_t>(strtol(pEnd + 1, &pEnd, 10));
        }

        if (sep != *pEnd) {
            ALOGE("parseAreas: error, Parsing of the weight area coordinate failed!");
            ret = -EINVAL;
            break;
        } else {
            weight = static_cast<ssize_t>(strtol(pEnd + 1, &pEnd, 10));
        }

        if (endToken != *pEnd) {
            ALOGE("parseAreas: error, malformed area!");
            ret = -EINVAL;
            break;
        }

        ret = checkArea(top, left, bottom, right, weight);
        if (NO_ERROR != ret)
            break;

        /*
        ALOGV("parseAreas: Area parsed [%dx%d, %dx%d] %d",
                ( int ) left,
                ( int ) top,
                ( int ) right,
                ( int ) bottom,
                ( int ) weight);
         */

        pArea = strtok_r(NULL, startToken, &ctx);

        focusArea->left = (int)left;
        focusArea->top = (int)top;
        focusArea->right = (int)right;
        focusArea->bottom = (int)bottom;
        focusArea->weight = (int)weight;
        (*num_areas)++;
    } while ( NULL != pArea );

    if (NULL != tmpBuffer)
        free(tmpBuffer);

    return ret;
}

/* TODO : muliple focus area is not supported yet */
status_t ISecCameraHardware::setFocusAreas(const CameraParameters &params)
{
    if (!IsAutoFocusSupported())
        return NO_ERROR;

    const char *str = params.get(CameraParameters::KEY_FOCUS_AREAS);
    const char *prevStr = mParameters.get(CameraParameters::KEY_FOCUS_AREAS);
    if ((str == NULL) || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    struct FocusArea focusArea;
    struct FocusPoint center;
    int err, num_areas = 0;
    const char *maxFocusAreasStr = params.get(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS);
    if (!maxFocusAreasStr) {
        ALOGE("setFocusAreas: error, KEY_MAX_NUM_FOCUS_AREAS is NULL");
        return NO_ERROR;
    }

    int maxFocusAreas = atoi(maxFocusAreasStr);
    if (!maxFocusAreas) {
        ALOGD("setFocusAreas: FocusAreas is not supported");
        return NO_ERROR;
    }

    /* Focus area parse here */
    err = parseAreas(str, (strlen(str) + 1), &focusArea, &num_areas);
    if (CC_UNLIKELY(err < 0)) {
        ALOGE("setFocusAreas: error, parseAreas %s", str);
        return BAD_VALUE;
    }
    if (CC_UNLIKELY(num_areas > maxFocusAreas)) {
        ALOGE("setFocusAreas: error, the number of areas is more than max");
        return BAD_VALUE;
    }

    /* find center point */
    err = findCenter(&focusArea, &center);
    if (CC_UNLIKELY(err < 0)) {
        ALOGE("setFocusAreas: error, findCenter");
        return BAD_VALUE;
    }

    /* Normalization */
    err = normalizeArea(&center);
    if (err < 0) {
        ALOGE("setFocusAreas: error, normalizeArea");
        return BAD_VALUE;
    }

    ALOGD("setFocusAreas: FocusAreas(%s) to (%d, %d)", str, center.x, center.y);

    mParameters.set(CameraParameters::KEY_FOCUS_AREAS, str);

#if 0//def ENABLE_TOUCH_AF
    if (CC_UNLIKELY(mFocusArea != V4L2_FOCUS_AREA_TRACKING)) {
        err = nativeSetParameters(CAM_CID_SET_TOUCH_AF_POSX, center.x);
        if (CC_UNLIKELY(err < 0)) {
            ALOGE("setFocusAreas: error, SET_TOUCH_AF_POSX");
            return UNKNOWN_ERROR;
        }

        err = nativeSetParameters(CAM_CID_SET_TOUCH_AF_POSY, center.y);
        if (CC_UNLIKELY(err < 0)) {
            ALOGE("setFocusAreas: error, SET_TOUCH_AF_POSX");
            return UNKNOWN_ERROR;
        }

        return nativeSetParameters(CAM_CID_SET_TOUCH_AF, 1);
    } else{
        return nativeSetParameters(CAM_CID_SET_TOUCH_AF, 0);
    }
#endif

    return NO_ERROR;
}

/* -------------------Focus Area ENDS here---------------------------- */
status_t ISecCameraHardware::setIso(const CameraParameters &params)
{
    const char *str = params.get(CameraParameters::KEY_ISO);
    const char *prevStr = mParameters.get(CameraParameters::KEY_ISO);
    if (str == NULL || (prevStr && !strcmp(str, prevStr))) {
        return NO_ERROR;
    }
    if (prevStr == NULL && !strcmp(str, isos[0].desc)) {  /* default */
        return NO_ERROR;
    }

    int val;
retry:
    val = SecCameraParameters::lookupAttr(isos, ARRAY_SIZE(isos), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setIso: warning, not supported value(%s)", str);
        /* str = reinterpret_cast<const char*>(isos[0].desc);
        goto retry;
         */
        return BAD_VALUE;
    }

    CLOGD("DEBUG(%s) : %s ",__FUNCTION__, str);
    mParameters.set(CameraParameters::KEY_ISO, str);

    return nativeSetParameters(CAM_CID_ISO, val);
}

status_t ISecCameraHardware::setBrightness(const CameraParameters &params)
{
    int val;
    if (CC_LIKELY(mSceneMode == SCENE_MODE_NONE)) {
        val = params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    } else {
        switch (mSceneMode) {
        case SCENE_MODE_BEACH_SNOW:
            val = 2;
            break;

        default:
            val = 0;
            break;
        }
    }

    int prevVal = mParameters.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    int max = mParameters.getInt(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION);
    int min = mParameters.getInt(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION);
    if (prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < min || val > max)) {
        ALOGE("setBrightness: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    ALOGD("setBrightness: %d", val);
    mParameters.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, val);

    return nativeSetParameters(CAM_CID_BRIGHTNESS, val);
}

status_t ISecCameraHardware::setWhiteBalance(const CameraParameters &params)
{
    const char *str;
	
    if (CC_LIKELY(mSceneMode == SCENE_MODE_NONE)) {
        str = params.get(CameraParameters::KEY_WHITE_BALANCE);
    } else {
        switch (mSceneMode) {
        case SCENE_MODE_SUNSET:
        case SCENE_MODE_CANDLE_LIGHT:
            str = CameraParameters::WHITE_BALANCE_DAYLIGHT;
            break;

        case SCENE_MODE_DUSK_DAWN:
            str = CameraParameters::WHITE_BALANCE_FLUORESCENT;
            break;

        default:
            str = CameraParameters::WHITE_BALANCE_AUTO;
            break;
        }
    }
   // str = params.get(CameraParameters::KEY_WHITE_BALANCE);

    const char *prevStr = mParameters.get(CameraParameters::KEY_WHITE_BALANCE);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(whiteBalances, ARRAY_SIZE(whiteBalances), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setWhiteBalance: warning, not supported value(%s)", str);
        str = reinterpret_cast<const char*>(whiteBalances[0].desc);
        goto retry;
    }

    CLOGD("DEBUG(%s) : %s ",__FUNCTION__, str);
    mParameters.set(CameraParameters::KEY_WHITE_BALANCE, str);

    return nativeSetParameters(CAM_CID_WHITE_BALANCE, val);
}

status_t ISecCameraHardware::setFlash(const CameraParameters &params)
{
    if (!IsFlashSupported())
        return NO_ERROR;

    const char *str;

    if (CC_LIKELY(mSceneMode == SCENE_MODE_NONE)) {
        str = params.get(CameraParameters::KEY_FLASH_MODE);
    } else {
        switch (mSceneMode) {
        case SCENE_MODE_PORTRAIT:
        case SCENE_MODE_PARTY_INDOOR:
        case SCENE_MODE_BACK_LIGHT:
        case SCENE_MODE_TEXT:
            str = params.get(CameraParameters::KEY_FLASH_MODE);
            break;

        default:
            str = CameraParameters::FLASH_MODE_OFF;
            break;
        }
    }
 //   str = params.get(CameraParameters::KEY_FLASH_MODE);

    const char *prevStr = mParameters.get(CameraParameters::KEY_FLASH_MODE);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(flashModes, ARRAY_SIZE(flashModes), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setFlash: warning, not supported value(%s)", str);
        return BAD_VALUE; /* return BAD_VALUE if invalid parameter */
    }

    ALOGD("setFlash: %s", str);
    mFlashMode = (cam_flash_mode)val;
    mParameters.set(CameraParameters::KEY_FLASH_MODE, str);

    return nativeSetParameters(CAM_CID_FLASH, val);
}

status_t ISecCameraHardware::setMetering(const CameraParameters &params)
{
    const char *str;
    if (CC_LIKELY(mSceneMode == SCENE_MODE_NONE)) {
        str = params.get(SecCameraParameters::KEY_METERING);
    } else {
        switch (mSceneMode) {
        case SCENE_MODE_LANDSCAPE:
            str = SecCameraParameters::METERING_MATRIX;
            break;

        case SCENE_MODE_BACK_LIGHT:
            if (mFlashMode == V4L2_FLASH_MODE_OFF)
                str = SecCameraParameters::METERING_SPOT;
            else
                str = SecCameraParameters::METERING_CENTER;
            break;

        default:
            str = SecCameraParameters::METERING_CENTER;
            break;
        }
    }

    const char *prevStr = mParameters.get(SecCameraParameters::KEY_METERING);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;
    if (prevStr == NULL && !strcmp(str, meterings[0].desc)) /* default */
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(meterings, ARRAY_SIZE(meterings), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setMetering: warning, not supported value(%s)", str);
        str = reinterpret_cast<const char*>(meterings[0].desc);
        goto retry;
    }

    CLOGD("DEBUG(%s) : %s ",__FUNCTION__, str);
    mParameters.set(SecCameraParameters::KEY_METERING, str);

    return nativeSetParameters(CAM_CID_METERING, val);
}

status_t ISecCameraHardware::setMeteringAreas(const CameraParameters &params)
{
    const char *str = params.get(CameraParameters::KEY_METERING_AREAS);
    const char *prevStr = mParameters.get(CameraParameters::KEY_METERING_AREAS);
     if ((str == NULL) || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    /* Metering area use same strcut with Focus area */
    struct FocusArea meteringArea;
    int err, num_areas = 0;
    const char *maxMeteringAreasStr = params.get(CameraParameters::KEY_MAX_NUM_METERING_AREAS);
    if (!maxMeteringAreasStr) {
        ALOGE("setMeteringAreas: error, KEY_MAX_NUM_METERING_AREAS is NULL");
        return NO_ERROR;
    }

    int maxMeteringAreas = atoi(maxMeteringAreasStr);
    if (!maxMeteringAreas) {
        ALOGD("setMeteringAreas: FocusAreas is not supported");
        return NO_ERROR;
    }

    /* Metering area parse and check(max value) here */
    err = parseAreas(str, (strlen(str) + 1), &meteringArea, &num_areas);
    if (CC_UNLIKELY(err < 0)) {
        ALOGE("setMeteringAreas: error, parseAreas %s", str);
        return BAD_VALUE;
    }
    if (CC_UNLIKELY(num_areas > maxMeteringAreas)) {
        ALOGE("setMeteringAreas: error, the number of areas is more than max");
        return BAD_VALUE;
    }

    ALOGD("setMeteringAreas = %s\n", str);
    mParameters.set(CameraParameters::KEY_METERING_AREAS, str);

    return NO_ERROR;
}

status_t ISecCameraHardware::setFocusMode(const CameraParameters &params)
{
	status_t Ret = NO_ERROR;

    const char *str = params.get(CameraParameters::KEY_FOCUS_MODE);
    const char *prevStr = mParameters.get(CameraParameters::KEY_FOCUS_MODE);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int count, val;
    const cam_strmap_t *focusModes;

    if (mCameraId == CAMERA_ID_BACK) {
        count = ARRAY_SIZE(backFocusModes);
        focusModes = backFocusModes;
    } else {
        count = ARRAY_SIZE(frontFocusModes);
        focusModes = frontFocusModes;
    }

retry:
    val = SecCameraParameters::lookupAttr(focusModes, count, str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setFocusMode: warning, not supported value(%s)", str);
        return BAD_VALUE; /* return BAD_VALUE if invalid parameter */
    }

    CLOGD("DEBUG(%s) : %s ",__FUNCTION__, str);
    mFocusMode = (cam_focus_mode)val;
    mParameters.set(CameraParameters::KEY_FOCUS_MODE, str);
    if (val == V4L2_FOCUS_MODE_MACRO) {
        mParameters.set(CameraParameters::KEY_FOCUS_DISTANCES,
                B_KEY_MACRO_FOCUS_DISTANCES_VALUE);
    } else {
        mParameters.set(CameraParameters::KEY_FOCUS_DISTANCES,
                B_KEY_NORMAL_FOCUS_DISTANCES_VALUE);
    }

    return nativeSetParameters(CAM_CID_FOCUS_MODE, val);
}

status_t ISecCameraHardware::setEffect(const CameraParameters &params)
{
    const char *str = params.get(CameraParameters::KEY_EFFECT);
    const char *prevStr = mParameters.get(CameraParameters::KEY_EFFECT);
    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;
retry:
    val = SecCameraParameters::lookupAttr(effects, ARRAY_SIZE(effects), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setEffect: warning, not supported value(%s)", str);
#if 0
        str = reinterpret_cast<const char*>(effects[0].desc);
        goto retry;
#else
        return BAD_VALUE;
#endif	/* FOR HAL TEST */
    }

    CLOGD("DEBUG(%s) : %s ",__FUNCTION__, str);
    mParameters.set(CameraParameters::KEY_EFFECT, str);

    return nativeSetParameters(CAM_CID_EFFECT, val);
}

status_t ISecCameraHardware::setZoom(const CameraParameters &params)
{
    if (!mZoomSupport)
        return NO_ERROR;

    int val = params.getInt(CameraParameters::KEY_ZOOM);
    int prevVal = mParameters.getInt(CameraParameters::KEY_ZOOM);
    int err;

    if (val == -1 || prevVal == val)
        return NO_ERROR;

    int max = params.getInt(CameraParameters::KEY_MAX_ZOOM);
    if (CC_UNLIKELY(val < 0 || val > max)) {
        ALOGE("setZoom: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    mParameters.set(CameraParameters::KEY_ZOOM, val);

    if (mEnableDZoom)
        /* Set AP zoom ratio */
        return nativeSetZoomRatio(val);
    else
        /* Set ISP/sensor zoom ratio */
        return nativeSetParameters(CAM_CID_ZOOM, val);
}

status_t ISecCameraHardware::setDzoom(const CameraParameters& params)
{
    int val = params.getInt("dzoom");
    int prevVal = mParameters.getInt("dzoom");
    int err;

    if (prevVal == val)
        return NO_ERROR;
    if (val < V4L2_ZOOM_LEVEL_0 || val >= 12) {
        ALOGE("invalid value for DZOOM val = %d", val);
        return BAD_VALUE;
    }

    CLOGD("setDZoom LEVEL %d->%d", prevVal, val);
    mParameters.set("dzoom", val);

//    err = nativeSetParameters(CAM_CID_DZOOM, val);
    if (err < 0) {
        CLOGE("%s: setDZoom failed", __func__);
        return err;
    }

    return NO_ERROR;
}

status_t ISecCameraHardware::setSharpness(const CameraParameters& params)
{
    int val = params.getInt("sharpness");
    int prevVal = mParameters.getInt("sharpness");
    if (prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < -2 || val > 2)) {
        ALOGE("setSharpness: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    ALOGD("setSharpness: %d", val);
    mParameters.set("sharpness", val);

    if (mSceneMode == SCENE_MODE_NONE)
        return nativeSetParameters(CAM_CID_SHARPNESS, val + 2);

    return NO_ERROR;
}

status_t ISecCameraHardware::setContrast(const CameraParameters& params)
{
    int val = params.getInt("contrast");
    int prevVal = mParameters.getInt("contrast");
    if (prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < -2 || val > 2)) {
        ALOGE("setContrast: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    ALOGD("setContrast: %d", val);
    mParameters.set("contrast", val);

    if (mSceneMode == SCENE_MODE_NONE)
        return nativeSetParameters(CAM_CID_CONTRAST, val + 2);

    return NO_ERROR;
}

status_t ISecCameraHardware::setSaturation(const CameraParameters& params)
{
    int val = params.getInt("saturation");
    int prevVal = mParameters.getInt("saturation");
    if (prevVal == val)
        return NO_ERROR;

    if (CC_UNLIKELY(val < -2 || val > 2)) {
        ALOGE("setSaturation: error, invalid value(%d)", val);
        return BAD_VALUE;
    }

    ALOGD("setSaturation: %d", val);
    mParameters.set("saturation", val);

    if (mSceneMode == SCENE_MODE_NONE)
        return nativeSetParameters(CAM_CID_SATURATION, val + 2);

    return NO_ERROR;
}

status_t ISecCameraHardware::setAntiShake(const CameraParameters &params)
{
    int val = params.getInt(SecCameraParameters::KEY_ANTI_SHAKE);
    int prevVal = mParameters.getInt(SecCameraParameters::KEY_ANTI_SHAKE);
    if (val == -1 || prevVal == val)
        return NO_ERROR;
    if (prevVal == -1 && val == 0)  /* default */
        return NO_ERROR;

    ALOGD("setAntiShake: %d", val);
    mParameters.set(SecCameraParameters::KEY_ANTI_SHAKE, val);

    return nativeSetParameters(CAM_CID_ANTISHAKE, val);
}

status_t ISecCameraHardware::setBlur(const CameraParameters &params)
{
    int val = params.getInt(SecCameraParameters::KEY_BLUR);
    int prevVal = mParameters.getInt(SecCameraParameters::KEY_BLUR);
    if (val == -1 || prevVal == val)
        return NO_ERROR;
    if (prevVal == -1 && val == 0)  /* default */
        return NO_ERROR;

    ALOGD("setBlur: %d", val);
    mParameters.set(SecCameraParameters::KEY_BLUR, val);
    if (val > 0)
        setDropFrame(2);

    return nativeSetParameters(CAM_CID_BLUR, val);
}

int ISecCameraHardware::checkFnumber(int f_val, int zoomLevel)
{
    int err = NO_ERROR;

    if (f_val == 0) {
        ALOGD("checkFnumber: f number is set to default value. f_val = %d",
                f_val);
        return err;
    }

    switch (zoomLevel) {
    case 0:
        if (f_val != 31 && f_val != 90)
        err = BAD_VALUE;
        break;
    case 1:
        if (f_val != 34 && f_val != 95)
        err = BAD_VALUE;
        break;
    case 2:
        if (f_val != 35 && f_val != 100)
        err = BAD_VALUE;
        break;
    case 3:
        if (f_val != 37 && f_val != 104)
        err = BAD_VALUE;
        break;
    case 4:
        if (f_val != 38 && f_val != 109)
        err = BAD_VALUE;
        break;
    case 5:
        if (f_val != 40 && f_val != 113)
        err = BAD_VALUE;
        break;
    case 6:
        if (f_val != 41 && f_val != 116)
        err = BAD_VALUE;
        break;
    case 7:
        if (f_val != 42 && f_val != 119)
        err = BAD_VALUE;
        break;
    case 8:
        if (f_val != 43 && f_val != 122)
        err = BAD_VALUE;
        break;
    case 9:
        if (f_val != 44 && f_val != 125)
        err = BAD_VALUE;
        break;
    case 10:
        if (f_val != 45 && f_val != 127)
        err = BAD_VALUE;
        break;
    case 11:
        if (f_val != 46 && f_val != 129)
        err = BAD_VALUE;
        break;
    case 12:
        if (f_val != 46 && f_val != 131)
        err = BAD_VALUE;
        break;
    case 13:
        if (f_val != 47 && f_val != 134)
        err = BAD_VALUE;
        break;
    case 14:
        if (f_val != 48 && f_val != 136)
        err = BAD_VALUE;
        break;
    case 15:
        if (f_val != 49 && f_val != 139)
        err = BAD_VALUE;
        break;
    case 16:
        if (f_val != 50 && f_val != 142)
        err = BAD_VALUE;
        break;
    case 17:
        if (f_val != 51 && f_val != 145)
        err = BAD_VALUE;
        break;
    case 18:
        if (f_val != 52 && f_val != 148)
        err = BAD_VALUE;
        break;
    case 19:
        if (f_val != 54 && f_val != 152)
        err = BAD_VALUE;
        break;
    case 20:
        if (f_val != 55 && f_val != 156)
        err = BAD_VALUE;
        break;
    case 21:
        if (f_val != 56 && f_val != 159)
        err = BAD_VALUE;
        break;
    case 22:
        if (f_val != 58 && f_val != 163)
        err = BAD_VALUE;
        break;
    case 23:
        if (f_val != 59 && f_val != 167)
        err = BAD_VALUE;
        break;
    case 24:
        if (f_val != 60 && f_val != 170)
        err = BAD_VALUE;
        break;
    case 25:
        if (f_val != 61 && f_val != 173)
        err = BAD_VALUE;
        break;
    case 26:
        if (f_val != 62 && f_val != 176)
        err = BAD_VALUE;
        break;
    case 27:
        if (f_val != 63 && f_val != 179)
        err = BAD_VALUE;
        break;
    case 28:
        if (f_val != 63 && f_val != 182)
        err = BAD_VALUE;
        break;
    case 29:
        if (f_val != 63 && f_val != 184)
        err = BAD_VALUE;
        break;
    default:
        err = NO_ERROR;
        break;
    }
    return err;
}

status_t ISecCameraHardware::setAntiBanding()
{
    status_t ret = NO_ERROR;
    const char *prevStr = mParameters.get(CameraParameters::KEY_ANTIBANDING);

    if (prevStr && !strcmp(mAntiBanding, prevStr))
        return NO_ERROR;

retry:
    int val = SecCameraParameters::lookupAttr(antibandings, ARRAY_SIZE(antibandings), mAntiBanding);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGE("setAntiBanding: error, not supported value(%s)", mAntiBanding);
        return BAD_VALUE;
    }
    ALOGD("setAntiBanding: %s", mAntiBanding);
    mParameters.set(CameraParameters::KEY_ANTIBANDING, mAntiBanding);
    return nativeSetParameters(CAM_CID_ANTIBANDING, val);
}

status_t ISecCameraHardware::setAntiBanding(const CameraParameters &params)
{
    const char *str = params.get(CameraParameters::KEY_ANTIBANDING);
    const char *prevStr = mParameters.get(CameraParameters::KEY_ANTIBANDING);

    if (str == NULL || (prevStr && !strcmp(str, prevStr)))
        return NO_ERROR;

    int val;

retry:
    val = SecCameraParameters::lookupAttr(antibandings, ARRAY_SIZE(antibandings), str);
    if (CC_UNLIKELY(val == NOT_FOUND)) {
        ALOGW("setAntiBanding: warning, not supported value(%s)", str);
        str = reinterpret_cast<const char*>(antibandings[0].desc);
        goto retry;
    }

    ALOGD("setAntiBanding: %s, val: %d", str, val);
    mParameters.set(CameraParameters::KEY_ANTIBANDING, str);

    return nativeSetParameters(CAM_CID_ANTIBANDING, val);
}

status_t ISecCameraHardware::setGps(const CameraParameters &params)
{
    const char *latitude = params.get(CameraParameters::KEY_GPS_LATITUDE);
    const char *logitude = params.get(CameraParameters::KEY_GPS_LONGITUDE);
    const char *altitude = params.get(CameraParameters::KEY_GPS_ALTITUDE);
    if (latitude && logitude && altitude) {
        ALOGV("setParameters: GPS latitude %f, logitude %f, altitude %f",
             atof(latitude), atof(logitude), atof(altitude));
        mParameters.set(CameraParameters::KEY_GPS_LATITUDE, latitude);
        mParameters.set(CameraParameters::KEY_GPS_LONGITUDE, logitude);
        mParameters.set(CameraParameters::KEY_GPS_ALTITUDE, altitude);
    } else {
        mParameters.remove(CameraParameters::KEY_GPS_LATITUDE);
        mParameters.remove(CameraParameters::KEY_GPS_LONGITUDE);
        mParameters.remove(CameraParameters::KEY_GPS_ALTITUDE);
    }

    const char *timestamp = params.get(CameraParameters::KEY_GPS_TIMESTAMP);
    if (timestamp) {
        ALOGV("setParameters: GPS timestamp %s", timestamp);
        mParameters.set(CameraParameters::KEY_GPS_TIMESTAMP, timestamp);
    } else {
        mParameters.remove(CameraParameters::KEY_GPS_TIMESTAMP);
    }

    const char *progressingMethod = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    if (progressingMethod) {
        ALOGV("setParameters: GPS timestamp %s", timestamp);
        mParameters.set(CameraParameters::KEY_GPS_PROCESSING_METHOD, progressingMethod);
    } else {
        mParameters.remove(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    }

    return NO_ERROR;
}

const image_rect_type *ISecCameraHardware::getFrameSizeSz
    (const image_rect_type *sizes, int count, uint32_t width, uint32_t height)
{
    for (int i = 0; i < count; i++) {
        if ((sizes[i].width == width) && (sizes[i].height == height))
            return &sizes[i];
    }

    return NULL;
}

const image_rect_type *ISecCameraHardware::getFrameSizeRatio
    (const image_rect_type *sizes, int count, uint32_t width, uint32_t height)
{
    const uint32_t ratio = SIZE_RATIO(width, height);
    int found = -ENOENT;

    for (int i = 0; i < count; i++) {
        if ((sizes[i].width == width) && (sizes[i].height == height))
            return &sizes[i];

        if (FRM_RATIO(sizes[i]) == ratio) {
            if ((-ENOENT == found) ||
                ((sizes[i].width < sizes[found].width) &&
                (sizes[i].width > width)))
                found = i;
        }
    }

    if (found != -ENOENT) {
        ALOGD("get_framesize: %dx%d -> %dx%d\n", width, height,
        sizes[found].width, sizes[found].height);
        return &sizes[found];
    }

    return NULL;
}

void ISecCameraHardware::setZoomRatioList(int *list, int len, float maxZoomRatio)
{
    float zoom_ratio_delta = pow(maxZoomRatio, 1.0f / len);

    for (int i = 0; i <= len; i++) {
        list[i] = (int)(pow(zoom_ratio_delta, i) * 1000);
        ALOGD("INFO(%s):list[%d]:(%d), (%f)",
            __func__, i, list[i], (float)((float)list[i] / 1000));
    }
}

status_t ISecCameraHardware::getZoomRatioList(String8 & string8Buf,
    int maxZoom, int maxZoomRatio, int *list)
{
    char strBuf[32];
    int cur = 0;
    int step = maxZoom - 1;

    setZoomRatioList(list, maxZoom - 1, (float)(maxZoomRatio / 1000));

    for (int i = 0; i < step; i++) {
        cur = (int)(list[i] / 10);
        snprintf(strBuf, sizeof(strBuf), "%d", cur);
        string8Buf.append(strBuf);
        string8Buf.append(",");
    }

    snprintf(strBuf, sizeof(strBuf), "%d", (maxZoomRatio / 10));
    string8Buf.append(strBuf);

    /* ex : "100,130,160,190,220,250,280,310,340,360,400" */

    return NO_ERROR;
}

bool ISecCameraHardware::allocMemSinglePlane(int ionClient,
    ExynosBuffer *buf, int index, bool flagCache)
{
    if (ionClient == 0) {
        ALOGE("ERR(%s): ionClient is zero (%d)", __func__, ionClient);
        return false;
    }

    if (buf->size.extS[index] != 0) {
        int ret = NO_ERROR;
        int flagIon = ((flagCache == true) ?
            (ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC ) : 0);

        ret  = ion_alloc_fd(ionClient, buf->size.extS[index], 0,
            ION_HEAP_SYSTEM_MASK, flagIon, &buf->fd.extFd[index]);
        if (ret < 0) {
            ALOGE("ERR(%s):ion_alloc_fd(fd=%d) failed(%s)", __FUNCTION__,
                buf->fd.extFd[index], strerror(errno));
            buf->fd.extFd[index] = -1;
            return false;
        }

        buf->virt.extP[index] = (char *)mmap(NULL, buf->size.extS[index],
            PROT_READ|PROT_WRITE, MAP_SHARED, buf->fd.extFd[index], 0);

        if (buf->virt.extP[index] == (char *)MAP_FAILED || buf->virt.extP[index] == NULL) {
            ALOGE("ERR(%s):ion_map(size=%d) failed", __FUNCTION__, buf->size.extS[index]);
            buf->virt.extP[index] = NULL;
            return false;
        }
    }

    return true;
}

void ISecCameraHardware::freeMemSinglePlane(ExynosBuffer *buf, int index)
{
    if (0 < buf->fd.extFd[index]) {
        if (buf->virt.extP[index] != NULL) {
            if (munmap(buf->virt.extP[index], buf->size.extS[index]) < 0) {
                ALOGE("ERR(%s):munmap failed", __FUNCTION__);
            }
        }
        ion_close(buf->fd.extFd[index]);
    }

    buf->fd.extFd[index] = -1;
    buf->virt.extP[index] = NULL;
    buf->size.extS[index] = 0;
}

bool ISecCameraHardware::allocMem(int ionClient, ExynosBuffer *buf, int cacheIndex)
{
    for (int i = 0; i < ExynosBuffer::BUFFER_PLANE_NUM_DEFAULT; i++) {
        bool flagCache = ((1 << i) & cacheIndex) ? true : false;
        if (allocMemSinglePlane(ionClient, buf, i, flagCache) == false) {
            freeMem(buf);
            ALOGE("ERR(%s): allocMemSinglePlane(%d) fail", __func__, i);
            return false;
        }
    }

    return true;
}

void ISecCameraHardware::freeMem(ExynosBuffer *buf)
{
    for (int i = 0; i < ExynosBuffer::BUFFER_PLANE_NUM_DEFAULT; i++)
        freeMemSinglePlane(buf, i);
}

void ISecCameraHardware::mInitRecSrcQ(void)
{
    Mutex::Autolock lock(mRecordSrcLock);
    mRecordSrcIndex = -1;

    mRecordSrcQ.clear();
}

int ISecCameraHardware::getRecSrcBufSlotIndex(void)
{
    Mutex::Autolock lock(mRecordSrcLock);
    mRecordSrcIndex++;
    mRecordSrcIndex = mRecordSrcIndex % FLITE_BUF_CNT;
    return mRecordSrcIndex;
}

void ISecCameraHardware::mPushRecSrcQ(rec_src_buf_t *buf)
{
    Mutex::Autolock lock(mRecordSrcLock);
    mRecordSrcQ.push_back(buf);
}

bool ISecCameraHardware::mPopRecSrcQ(rec_src_buf_t *buf)
{
    List<rec_src_buf_t *>::iterator r;

    Mutex::Autolock lock(mRecordSrcLock);

    if (mRecordSrcQ.size() == 0)
        return false;

    r = mRecordSrcQ.begin()++;

    buf->buf = (*r)->buf;
    buf->timestamp = (*r)->timestamp;
    mRecordSrcQ.erase(r);

    return true;
}

int ISecCameraHardware::mSizeOfRecSrcQ(void)
{
    Mutex::Autolock lock(mRecordSrcLock);

    return mRecordSrcQ.size();
}

#if 0
bool ISecCameraHardware::setRecDstBufStatus(int index, enum REC_BUF_STATUS status)
{
    Mutex::Autolock lock(mRecordDstLock);

    if (index < 0 || index >= REC_BUF_CNT) {
        ALOGE("ERR(%s): index(%d) out of range, status(%d)", __func__, index, status);
        return false;
    }

    mRecordDstStatus[index] = status;
    return true;
}
#endif

int ISecCameraHardware::getRecDstBufIndex(void)
{
    Mutex::Autolock lock(mRecordDstLock);

    for (int i = 0; i < REC_BUF_CNT; i++) {
        mRecordDstIndex++;
        mRecordDstIndex = mRecordDstIndex % REC_BUF_CNT;

        if (mRecordFrameAvailable[mRecordDstIndex] == true) {
            mRecordFrameAvailableCnt--;
            mRecordFrameAvailable[mRecordDstIndex] = false;
            return mRecordDstIndex;
        }
    }

    return -1;
}

void ISecCameraHardware::setAvailDstBufIndex(int index)
{
    Mutex::Autolock lock(mRecordDstLock);
    mRecordFrameAvailableCnt++;
    mRecordFrameAvailable[index] = true;
    return;
}

void ISecCameraHardware::mInitRecDstBuf(void)
{
    Mutex::Autolock lock(mRecordDstLock);

    ExynosBuffer nullBuf;

    mRecordDstIndex = -1;
    mRecordFrameAvailableCnt = REC_BUF_CNT;

    for (int i = 0; i < REC_BUF_CNT; i++) {
#ifdef BOARD_USE_MHB_ION
        for (int j = 0; j < REC_PLANE_CNT; j++) {
            if (mRecordDstHeap[i][j] != NULL)
                mRecordDstHeap[i][j]->release(mRecordDstHeap[i][j]);
            mRecordDstHeap[i][j] = NULL;
            mRecordDstHeapFd[i][j] = -1;
        }
#else
        if (0 < mRecordingDstBuf[i].fd.extFd[0])
            freeMem(&mRecordingDstBuf[i]);
#endif
        mRecordingDstBuf[i] = nullBuf;
        mRecordFrameAvailable[i] = true;
    }
}

int ISecCameraHardware::getAlignedYUVSize(int colorFormat, int w, int h, ExynosBuffer *buf, bool flagAndroidColorFormat)
{
    int FrameSize = 0;
    ExynosBuffer alignedBuf;

    /* ALOGV("[%s] (%d) colorFormat %d", __func__, __LINE__, colorFormat); */
    switch (colorFormat) {
    /* 1p */
    case V4L2_PIX_FMT_RGB565 :
    case V4L2_PIX_FMT_YUYV :
    case V4L2_PIX_FMT_UYVY :
    case V4L2_PIX_FMT_VYUY :
    case V4L2_PIX_FMT_YVYU :
        alignedBuf.size.extS[0] = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(colorFormat), w, h);
	if(h==1080){
		alignedBuf.size.extS[0] += 1024*28;
	}
        /* ALOGV("V4L2_PIX_FMT_YUYV buf->size.extS[0] %d", alignedBuf->size.extS[0]); */
        alignedBuf.size.extS[1] = SPARE_SIZE;
        alignedBuf.size.extS[2] = 0;
        break;
    /* 2p */
    case V4L2_PIX_FMT_NV12 :
    case V4L2_PIX_FMT_NV12T :
    case V4L2_PIX_FMT_NV21 :
    case V4L2_PIX_FMT_NV12M :
    case V4L2_PIX_FMT_NV21M :
        alignedBuf.size.extS[0] = w * h;
        alignedBuf.size.extS[1] = w * h / 2;
        alignedBuf.size.extS[2] = SPARE_SIZE;
        /* ALOGV("V4L2_PIX_FMT_NV21 buf->size.extS[0] %d buf->size.extS[1] %d",
            alignedBuf->size.extS[0], alignedBuf->size.extS[1]); */
        break;
    case V4L2_PIX_FMT_NV12MT_16X16 :
        if (flagAndroidColorFormat == true) {
            alignedBuf.size.extS[0] = w * h;
            alignedBuf.size.extS[1] = w * h / 2;
            alignedBuf.size.extS[2] = SPARE_SIZE;
        } else {
            alignedBuf.size.extS[0] = ALIGN_UP(w, 16) * ALIGN_UP(h, 16);
            alignedBuf.size.extS[1] = ALIGN(alignedBuf.size.extS[0] / 2, 256);
            alignedBuf.size.extS[2] = SPARE_SIZE;
        }
        /* ALOGV("V4L2_PIX_FMT_NV12M buf->size.extS[0] %d buf->size.extS[1] %d",
            alignedBuf->size.extS[0], alignedBuf->size.extS[1]); */
        break;
    case V4L2_PIX_FMT_NV16 :
    case V4L2_PIX_FMT_NV61 :
        alignedBuf.size.extS[0] = ALIGN_UP(w, 16) * ALIGN_UP(h, 16);
        alignedBuf.size.extS[1] = ALIGN_UP(w, 16) * ALIGN_UP(h, 16);
        alignedBuf.size.extS[2] = SPARE_SIZE;
        /* ALOGV("V4L2_PIX_FMT_NV16 buf->size.extS[0] %d buf->size.extS[1] %d",
            alignedBuf->size.extS[0], alignedBuf->size.extS[1]); */
        break;
    /* 3p */
    case V4L2_PIX_FMT_YUV420 :
    case V4L2_PIX_FMT_YVU420 :
        /* http://developer.android.com/reference/android/graphics/ImageFormat.html#YV12 */
        alignedBuf.size.extS[0] = ALIGN_UP(w, 16) * h;
        alignedBuf.size.extS[1] = ALIGN_UP(w / 2, 16) * h / 2;
        alignedBuf.size.extS[2] = ALIGN_UP(w / 2, 16) * h / 2;
        alignedBuf.size.extS[3] = SPARE_SIZE;
        /* ALOGV("V4L2_PIX_FMT_YUV420 Buf.size.extS[0] %d Buf.size.extS[1] %d Buf.size.extS[2] %d",
            alignedBuf.size.extS[0], alignedBuf.size.extS[1], alignedBuf.size.extS[2]); */
        break;
    case V4L2_PIX_FMT_YUV420M :
    case V4L2_PIX_FMT_YVU420M :
        if (flagAndroidColorFormat == true) {
            alignedBuf.size.extS[0] = ALIGN_UP(w, 16) * h;
            alignedBuf.size.extS[1] = ALIGN_UP(w / 2, 16) * h / 2;
            alignedBuf.size.extS[2] = ALIGN_UP(w / 2, 16) * h / 2;
            alignedBuf.size.extS[3] = SPARE_SIZE;
        } else {
            alignedBuf.size.extS[0] = ALIGN_UP(w,   32) * ALIGN_UP(h,  16);
            alignedBuf.size.extS[1] = ALIGN_UP(w/2, 16) * ALIGN_UP(h/2, 8);
            alignedBuf.size.extS[2] = ALIGN_UP(w/2, 16) * ALIGN_UP(h/2, 8);
            alignedBuf.size.extS[3] = SPARE_SIZE;
        }
        /* ALOGV("V4L2_PIX_FMT_YUV420M buf->size.extS[0] %d buf->size.extS[1] %d buf->size.extS[2] %d",
            alignedBuf->size.extS[0], alignedBuf->size.extS[1], alignedBuf->size.extS[2]); */
        break;
    case V4L2_PIX_FMT_YUV422P :
        alignedBuf.size.extS[0] = ALIGN_UP(w,   16) * ALIGN_UP(h,  16);
        alignedBuf.size.extS[1] = ALIGN_UP(w/2, 16) * ALIGN_UP(h/2, 8);
        alignedBuf.size.extS[2] = ALIGN_UP(w/2, 16) * ALIGN_UP(h/2, 8);
        alignedBuf.size.extS[3] = SPARE_SIZE;
        /* ALOGV("V4L2_PIX_FMT_YUV422P Buf.size.extS[0] %d Buf.size.extS[1] %d Buf.size.extS[2] %d",
            alignedBuf.size.extS[0], alignedBuf.size.extS[1], alignedBuf.size.extS[2]); */
        break;
	case V4L2_PIX_FMT_JPEG:
		alignedBuf.size.extS[0] = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(V4L2_PIX_FMT_YUYV), w, h);
		alignedBuf.size.extS[1] = SPARE_SIZE;
		alignedBuf.size.extS[2] = 0;
		ALOGD("V4L2_PIX_FMT_JPEG buf->size.extS[0] = %d", alignedBuf.size.extS[0]);
		break;
    default:
        ALOGE("ERR(%s):unmatched colorFormat(%d)", __func__, colorFormat);
        return 0;
        break;
    }

    for (int i = 0; i < ExynosBuffer::BUFFER_PLANE_NUM_DEFAULT; i++)
        FrameSize += alignedBuf.size.extS[i];

    if (buf != NULL) {
        for (int i = 0; i < ExynosBuffer::BUFFER_PLANE_NUM_DEFAULT; i++) {
            buf->size.extS[i] = alignedBuf.size.extS[i];

            /* if buf has vadr, calculate another vadr per plane */
            if (buf->virt.extP[0] != NULL && i > 0) {
                if (buf->size.extS[i] != 0)
                    buf->virt.extP[i] = buf->virt.extP[i - 1] + buf->size.extS[i - 1];
                else
                    buf->virt.extP[i] = NULL;
            }
        }
    }
    return (FrameSize - SPARE_SIZE);
}

#ifdef USE_DEDICATED_PREVIEW_ENQUEUE_THREAD
status_t ISecCameraHardware::m_clearPreviewFrameList(ExynosCameraList<buffer_handle_t *>* queue)
{
    buffer_handle_t *curFrame = NULL;

    if(queue->getSizeOfProcessQ() == 0) {
        return NO_ERROR;
    }

    ALOGD("DEBUG(%s):remaining frame(%d), we remove them all",
        __FUNCTION__, queue->getSizeOfProcessQ());

    while (0 < queue->getSizeOfProcessQ()) {
        queue->popProcessQ(&curFrame);
        if (curFrame != NULL) {
            ALOGD("DEBUG(%s):remove frame", __FUNCTION__);
            if (mPreviewWindow) {
                if(mPreviewWindow->cancel_buffer(mPreviewWindow, curFrame) != 0) {
                    ALOGE("ERR(%s):Fail to cancel buffer", __func__);
                }
            } else {
                ALOGW("DEBUG(%s):mPreviewWindow is NULL", __FUNCTION__);
            }
            curFrame = NULL;
        }
    }
    ALOGD("DEBUG(%s):EXIT ", __FUNCTION__);

    return NO_ERROR;
}
#endif

void ISecCameraHardware::checkHorizontalViewAngle(void)
{
    mParameters.setFloat(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, getHorizontalViewAngle());
}

void ISecCameraHardware::setHorizontalViewAngle(int pictureW, int pictureH)
{
    double pi_camera = 3.1415926f;
    double distance;
    double ratio;
    double hViewAngle_half_rad = pi_camera / 180 *
        (double)findHorizontalViewAngleByRatio(SIZE_RATIO(16, 9)) / 2;

    distance = ((double)mSensorSize.width / (double)mSensorSize.height * 9 / 2)
                / tan(hViewAngle_half_rad);
    ratio = (double)pictureW / (double)pictureH;

    m_calculatedHorizontalViewAngle = (float)(atan(ratio * 9 / 2 / distance) * 2 * 180 / pi_camera);
}


float ISecCameraHardware::findHorizontalViewAngleByRatio(uint32_t ratio)
{
    uint32_t i;
    float view_angle = 0.0f;

    if (mCameraId == CAMERA_ID_BACK) {
        for (i = 0; i < ARRAY_SIZE(backHorizontalViewAngle); i++) {
            if (ratio == backHorizontalViewAngle[i].size_ratio) {
                view_angle = backHorizontalViewAngle[i].view_angle;
                break;
            }
        }
    } else {
        for (i = 0; i < ARRAY_SIZE(frontHorizontalViewAngle); i++) {
            if (ratio == frontHorizontalViewAngle[i].size_ratio) {
                view_angle = frontHorizontalViewAngle[i].view_angle;
                break;
            }
        }
    }

    return view_angle;
}

float ISecCameraHardware::getHorizontalViewAngle(void)
{
    int right_ratio = 177;

    if ((int)(mSensorSize.width * 100 / mSensorSize.height) == right_ratio) {
        return m_calculatedHorizontalViewAngle;
    } else {
        return findHorizontalViewAngleByRatio(SIZE_RATIO(mPictureSize.width, mPictureSize.height));
    }
}

float ISecCameraHardware::getVerticalViewAngle(void)
{
    if (mCameraId == CAMERA_ID_BACK) {
        return backVerticalViewAngle;
    } else {
        return frontVerticalViewAngle;
    }
}

}; /* namespace android */
#endif /* ANDROID_HARDWARE_ISECCAMERAHARDWARE_CPP */
