/*
**
** Copyright 2015, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCamera"
#include <cutils/log.h>

#include "ExynosCamera.h"

namespace android {

#ifdef MONITOR_LOG_SYNC
uint32_t ExynosCamera::cameraSyncLogId = 0;
#endif

ExynosCamera::ExynosCamera(int cameraId, camera_device_t *dev)
{
    ExynosCameraActivityUCTL *uctlMgr = NULL;

    BUILD_DATE();

    checkAndroidVersion();

    m_cameraId = cameraId;
    m_dev = dev;

    initialize();
}

void ExynosCamera::initialize()
{
    CLOGI("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);
    int ret = 0;

    ExynosCameraActivityUCTL *uctlMgr = NULL;
    memset(m_name, 0x00, sizeof(m_name));

    m_parameters = new ExynosCamera1Parameters(m_cameraId);
    CLOGD("DEBUG(%s):Parameters(Id=%d) created", __FUNCTION__, m_cameraId);

    m_parameters->setHalVersion(IS_HAL_VER_1_0);

    m_exynosCameraActivityControl = m_parameters->getActivityControl();

    m_previewFrameFactory      = NULL;
    m_reprocessingFrameFactory = NULL;
    /* vision */
    m_visionFrameFactory= NULL;

    m_ionAllocator = NULL;
    m_grAllocator  = NULL;
    m_mhbAllocator = NULL;

    m_frameMgr = NULL;

    m_createInternalBufferManager(&m_bayerBufferMgr, "BAYER_BUF");
    m_createInternalBufferManager(&m_3aaBufferMgr, "3A1_BUF");
    m_createInternalBufferManager(&m_ispBufferMgr, "ISP_BUF");
    m_createInternalBufferManager(&m_hwDisBufferMgr, "HW_DIS_BUF");
    m_createInternalBufferManager(&m_vraBufferMgr, "VRA_BUF");

    /* reprocessing Buffer */
    m_createInternalBufferManager(&m_ispReprocessingBufferMgr, "ISP_RE_BUF");
    m_createInternalBufferManager(&m_sccReprocessingBufferMgr, "SCC_RE_BUF");

    m_createInternalBufferManager(&m_sccBufferMgr, "SCC_BUF");
    m_createInternalBufferManager(&m_gscBufferMgr, "GSC_BUF");
    m_createInternalBufferManager(&m_jpegBufferMgr, "JPEG_BUF");
    m_createInternalBufferManager(&m_thumbnailBufferMgr, "THUMBNAIL_BUF");

    /* preview Buffer */
    m_scpBufferMgr = NULL;
    m_createInternalBufferManager(&m_previewCallbackBufferMgr, "PREVIEW_CB_BUF");
    m_createInternalBufferManager(&m_highResolutionCallbackBufferMgr, "HIGH_RESOLUTION_CB_BUF");
    m_fdCallbackHeap = NULL;

    /* recording Buffer */
    m_recordingCallbackHeap = NULL;
    m_createInternalBufferManager(&m_recordingBufferMgr, "REC_BUF");

    m_createThreads();

    m_pipeFrameDoneQ     = new frame_queue_t;
    dstIspReprocessingQ  = new frame_queue_t;
    dstSccReprocessingQ  = new frame_queue_t;
    dstGscReprocessingQ  = new frame_queue_t;
    m_zoomPreviwWithCscQ = new frame_queue_t;
    dstJpegReprocessingQ = new frame_queue_t;
    /* vision */
    m_pipeFrameVisionDoneQ     = new frame_queue_t;

    m_frameFactoryQ = new framefactory_queue_t;
    m_facedetectQ = new frame_queue_t;
    m_facedetectQ->setWaitTime(500000000);

    m_previewQ     = new frame_queue_t;
    m_previewQ->setWaitTime(500000000);

    m_vraThreadQ    = new frame_queue_t;
    m_vraThreadQ->setWaitTime(500000000);
    m_vraGscDoneQ    = new frame_queue_t;
    m_vraGscDoneQ->setWaitTime(500000000);
    m_vraPipeDoneQ    = new frame_queue_t;
    m_vraPipeDoneQ->setWaitTime(500000000);

    m_previewCallbackGscFrameDoneQ = new frame_queue_t;
    m_recordingQ   = new frame_queue_t;
    m_recordingQ->setWaitTime(500000000);
    m_postPictureQ = new frame_queue_t(m_postPictureThread);
    for(int i = 0 ; i < MAX_NUM_PIPES ; i++ ) {
        m_mainSetupQ[i] = new frame_queue_t;
        m_mainSetupQ[i]->setWaitTime(500000000);
    }
    m_jpegCallbackQ = new jpeg_callback_queue_t;
    m_postviewCallbackQ = new postview_callback_queue_t;

    for (int threadNum = JPEG_SAVE_THREAD0; threadNum < JPEG_SAVE_THREAD_MAX_COUNT; threadNum++) {
        m_jpegSaveQ[threadNum] = new jpeg_callback_queue_t;
        m_jpegSaveQ[threadNum]->setWaitTime(2000000000);
        m_burst[threadNum] = false;
        m_running[threadNum] = false;
    }

    dstIspReprocessingQ->setWaitTime(20000000);
    dstSccReprocessingQ->setWaitTime(50000000);
    dstGscReprocessingQ->setWaitTime(500000000);
    dstJpegReprocessingQ->setWaitTime(500000000);
    /* vision */
    m_pipeFrameVisionDoneQ->setWaitTime(2000000000);

    m_jpegCallbackQ->setWaitTime(1000000000);
    m_postviewCallbackQ->setWaitTime(1000000000);

    memset(&m_frameMetadata, 0, sizeof(camera_frame_metadata_t));
    memset(m_faces, 0, sizeof(camera_face_t) * NUM_OF_DETECTED_FACES);

    m_exitAutoFocusThread = false;
    m_autoFocusRunning    = false;
    m_previewEnabled   = false;
    m_pictureEnabled   = false;
    m_recordingEnabled = false;
    m_zslPictureEnabled   = false;
    m_flagStartFaceDetection = false;
    m_flagLLSStart = false;
    m_flagLightCondition = false;
    m_fdThreshold = 0;
    m_captureSelector = NULL;
    m_sccCaptureSelector = NULL;
    m_autoFocusType = 0;
    m_hdrEnabled = false;
    m_doCscRecording = true;
    m_recordingBufferCount = NUM_RECORDING_BUFFERS;
    m_frameSkipCount = 0;
    m_isZSLCaptureOn = false;
    m_isSuccessedBufferAllocation = false;
    m_skipCount = 0;

#ifdef FPS_CHECK
    for (int i = 0; i < DEBUG_MAX_PIPE_NUM; i++)
        m_debugFpsCount[i] = 0;
#endif

    m_stopBurstShot = false;
    m_disablePreviewCB = false;
    m_checkFirstFrameLux = false;

    m_callbackState = 0;
    m_callbackStateOld = 0;
    m_callbackMonitorCount = 0;

    m_highResolutionCallbackRunning = false;
    m_highResolutionCallbackQ = new frame_queue_t(m_highResolutionCallbackThread);
    m_highResolutionCallbackQ->setWaitTime(500000000);
    m_skipReprocessing = false;
    m_isFirstStart = true;
    m_parameters->setIsFirstStartFlag(m_isFirstStart);
#ifdef RAWDUMP_CAPTURE
    m_RawCaptureDumpQ = new frame_queue_t(m_RawCaptureDumpThread);
#endif
#ifdef RAWDUMP_CAPTURE
    ExynosCameraActivitySpecialCapture *m_sCapture = m_exynosCameraActivityControl->getSpecialCaptureMgr();
    m_sCapture->resetRawCaptureFcount();
#endif

    m_exynosconfig = NULL;
    m_setConfigInform();

    m_setFrameManager();


    /* HACK Reset Preview Flag*/
    m_resetPreview = false;

    m_dynamicSccCount = 0;
    m_previewBufferCount = NUM_PREVIEW_BUFFERS;

    /* HACK for CTS2.0 */
    m_oldPreviewW = 0;
    m_oldPreviewH = 0;

#ifdef FIRST_PREVIEW_TIME_CHECK
    m_flagFirstPreviewTimerOn = false;
#endif

    /* init infomation of fd orientation*/
    m_parameters->setDeviceOrientation(0);
    uctlMgr = m_exynosCameraActivityControl->getUCTLMgr();
    if (uctlMgr != NULL)
        uctlMgr->setDeviceRotation(m_parameters->getFdOrientation());
#ifdef MONITOR_LOG_SYNC
    m_syncLogDuration = 0;
#endif
    vendorSpecificConstructor(m_cameraId, m_dev);

    m_callbackCookie = 0;
    m_fliteFrameCount = 0;
    m_3aa_ispFrameCount = 0;
    m_ispFrameCount = 0;
    m_sccFrameCount = 0;
    m_scpFrameCount = 0;
    m_vraRunningCount = 0;
    m_lastRecordingTimeStamp = 0;
    m_recordingStartTimeStamp = 0;
    m_fdCallbackHeap = 0;
    m_burstSaveTimerTime = 0;
    m_burstDuration = 0;
    m_burstInitFirst = 0;
    m_burstRealloc = 0;
    m_displayPreviewToggle = 0;
    m_hdrSkipedFcount = 0;
    m_curMinFps = 0;
    m_visionFps = 0;
    m_visionAe = 0;
    m_hackForAlignment = 0;
    m_recordingFrameSkipCount = 0;
    m_faceDetected = false;
    m_flagThreadStop = false;
    m_isNeedAllocPictureBuffer = false;
    m_isCancelBurstCapture = false;
    m_isTryStopFlash = false;
    m_notifyCb        = NULL;
    m_dataCb          = NULL;
    m_dataCbTimestamp = NULL;
    m_getMemoryCb     = NULL;
    m_previewWindow = NULL;
    m_initFrameFactory();

    m_tempshot = new struct camera2_shot_ext;
    m_fdmeta_shot = new struct camera2_shot_ext;
    m_meta_shot  = new struct camera2_shot_ext;
}

status_t  ExynosCamera::m_setConfigInform() {
    struct ExynosConfigInfo exynosConfig;
    memset((void *)&exynosConfig, 0x00, sizeof(exynosConfig));

    exynosConfig.mode = CONFIG_MODE::NORMAL;

    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_sensor_buffers = (getCameraId() == CAMERA_ID_BACK) ? NUM_SENSOR_BUFFERS : FRONT_NUM_SENSOR_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_bayer_buffers = (getCameraId() == CAMERA_ID_BACK) ? NUM_BAYER_BUFFERS : FRONT_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.init_bayer_buffers = INIT_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_3aa_buffers = (getCameraId() == CAMERA_ID_BACK) ? NUM_3AA_BUFFERS : FRONT_NUM_3AA_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_hwdis_buffers = NUM_HW_DIS_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_vra_buffers = NUM_VRA_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_preview_buffers = NUM_PREVIEW_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_picture_buffers = (getCameraId() == CAMERA_ID_BACK) ? NUM_PICTURE_BUFFERS : FRONT_NUM_PICTURE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_reprocessing_buffers = NUM_REPROCESSING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_recording_buffers = NUM_RECORDING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_fastaestable_buffer = INITIAL_SKIP_FRAME;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.reprocessing_bayer_hold_count = REPROCESSING_BAYER_HOLD_COUNT;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.preview_buffer_margin = NUM_PREVIEW_BUFFERS_MARGIN;

    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_FLITE] = (getCameraId() == CAMERA_ID_BACK) ? PIPE_FLITE_PREPARE_COUNT : PIPE_FLITE_FRONT_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_3AC] = PIPE_3AC_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_3AA] = PIPE_3AA_ISP_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_3AA_ISP] = PIPE_3AA_ISP_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_ISP] = PIPE_ISP_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_ISPC] = (getCameraId() == CAMERA_ID_BACK) ? PIPE_3AA_ISP_PREPARE_COUNT : PIPE_FLITE_FRONT_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_SCC] = (getCameraId() == CAMERA_ID_BACK) ? PIPE_3AA_ISP_PREPARE_COUNT : PIPE_FLITE_FRONT_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_SCP] = (getCameraId() == CAMERA_ID_BACK) ? PIPE_SCP_PREPARE_COUNT : PIPE_SCP_FRONT_PREPARE_COUNT;

    /* reprocessing */
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_SCP_REPROCESSING] = PIPE_SCP_REPROCESSING_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_ISPC_REPROCESSING] = PIPE_SCC_REPROCESSING_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_SCC_REPROCESSING] = PIPE_SCC_REPROCESSING_PREPARE_COUNT;

#if (USE_HIGHSPEED_RECORDING)
    /* Config HIGH_SPEED 60 buffer & pipe info */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_sensor_buffers = (getCameraId() == CAMERA_ID_BACK) ? FPS60_NUM_NUM_SENSOR_BUFFERS : FPS60_FRONT_NUM_SENSOR_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_bayer_buffers = (getCameraId() == CAMERA_ID_BACK) ? FPS60_NUM_NUM_BAYER_BUFFERS : FPS60_FRONT_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.init_bayer_buffers = FPS60_NUM_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_3aa_buffers = FPS60_NUM_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_hwdis_buffers = FPS60_NUM_HW_DIS_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_vra_buffers = FPS60_NUM_VRA_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_preview_buffers = FPS60_NUM_PREVIEW_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_picture_buffers = (getCameraId() == CAMERA_ID_BACK) ? FPS60_NUM_PICTURE_BUFFERS : FPS60_FRONT_NUM_PICTURE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_reprocessing_buffers = FPS60_NUM_REPROCESSING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_recording_buffers = FPS60_NUM_RECORDING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_fastaestable_buffer = FPS60_INITIAL_SKIP_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.reprocessing_bayer_hold_count = FPS60_REPROCESSING_BAYER_HOLD_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.preview_buffer_margin = FPS60_NUM_PREVIEW_BUFFERS_MARGIN;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].pipeInfo.prepare[PIPE_FLITE] = FPS60_PIPE_FLITE_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].pipeInfo.prepare[PIPE_3AC] = FPS60_PIPE_3AC_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].pipeInfo.prepare[PIPE_3AA] = FPS60_PIPE_3AA_ISP_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].pipeInfo.prepare[PIPE_3AA_ISP] = FPS60_PIPE_3AA_ISP_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].pipeInfo.prepare[PIPE_SCP] = FPS60_PIPE_SCP_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].pipeInfo.prepare[PIPE_SCP_REPROCESSING] = FPS60_PIPE_SCP_REPROCESSING_PREPARE_COUNT;

    /* Config HIGH_SPEED 120 buffer & pipe info */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_sensor_buffers = (getCameraId() == CAMERA_ID_BACK) ? FPS120_NUM_NUM_SENSOR_BUFFERS : FPS120_FRONT_NUM_SENSOR_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_bayer_buffers = (getCameraId() == CAMERA_ID_BACK) ? FPS120_NUM_NUM_BAYER_BUFFERS : FPS120_FRONT_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.init_bayer_buffers = FPS120_NUM_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_3aa_buffers = FPS120_NUM_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_hwdis_buffers = FPS120_NUM_HW_DIS_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_vra_buffers = FPS120_NUM_VRA_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_preview_buffers = (FPS120_NUM_PREVIEW_BUFFERS > MAX_BUFFERS) ? MAX_BUFFERS : FPS120_NUM_PREVIEW_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_picture_buffers = (getCameraId() == CAMERA_ID_BACK) ? FPS120_NUM_PICTURE_BUFFERS : FPS120_FRONT_NUM_PICTURE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_reprocessing_buffers = FPS120_NUM_REPROCESSING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_recording_buffers = FPS120_NUM_RECORDING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_fastaestable_buffer = FPS120_INITIAL_SKIP_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.reprocessing_bayer_hold_count = FPS120_REPROCESSING_BAYER_HOLD_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.preview_buffer_margin = FPS120_NUM_PREVIEW_BUFFERS_MARGIN;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].pipeInfo.prepare[PIPE_FLITE] = FPS120_PIPE_FLITE_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].pipeInfo.prepare[PIPE_3AC] = FPS120_PIPE_3AC_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].pipeInfo.prepare[PIPE_3AA] = FPS120_PIPE_3AA_ISP_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].pipeInfo.prepare[PIPE_3AA_ISP] = FPS120_PIPE_3AA_ISP_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].pipeInfo.prepare[PIPE_SCP] = FPS120_PIPE_SCP_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].pipeInfo.prepare[PIPE_SCP_REPROCESSING] = FPS120_PIPE_SCP_REPROCESSING_PREPARE_COUNT;
#ifdef SUPPORT_HIGH_SPEED_240FPS
    /* Config HIGH_SPEED 240 buffer & pipe info */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_sensor_buffers = (getCameraId() == CAMERA_ID_BACK) ? FPS240_NUM_NUM_SENSOR_BUFFERS : FPS240_FRONT_NUM_SENSOR_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_bayer_buffers = (getCameraId() == CAMERA_ID_BACK) ? FPS240_NUM_NUM_BAYER_BUFFERS : FPS240_FRONT_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.init_bayer_buffers = FPS240_INIT_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_3aa_buffers = FPS240_NUM_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_hwdis_buffers = FPS240_NUM_HW_DIS_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_vra_buffers = FPS240_NUM_VRA_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_preview_buffers = (FPS240_NUM_PREVIEW_BUFFERS > MAX_BUFFERS) ? MAX_BUFFERS : FPS240_NUM_PREVIEW_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_picture_buffers = (getCameraId() == CAMERA_ID_BACK) ? FPS240_NUM_PICTURE_BUFFERS : FPS240_FRONT_NUM_PICTURE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_reprocessing_buffers = FPS240_NUM_REPROCESSING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_recording_buffers = FPS240_NUM_RECORDING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.num_fastaestable_buffer = FPS240_INITIAL_SKIP_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.reprocessing_bayer_hold_count = FPS240_REPROCESSING_BAYER_HOLD_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].bufInfo.preview_buffer_margin = FPS240_NUM_PREVIEW_BUFFERS_MARGIN;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].pipeInfo.prepare[PIPE_FLITE] = FPS240_PIPE_FLITE_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].pipeInfo.prepare[PIPE_3AC] = FPS240_PIPE_3AC_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].pipeInfo.prepare[PIPE_3AA] = FPS240_PIPE_3AA_ISP_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].pipeInfo.prepare[PIPE_3AA_ISP] = FPS240_PIPE_3AA_ISP_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].pipeInfo.prepare[PIPE_SCP] = FPS240_PIPE_SCP_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_240].pipeInfo.prepare[PIPE_SCP_REPROCESSING] = FPS240_PIPE_SCP_REPROCESSING_PREPARE_COUNT;
#endif
#endif

    m_parameters->setConfig(&exynosConfig);

    m_exynosconfig = m_parameters->getConfig();

    return NO_ERROR;
}

void ExynosCamera::m_createThreads(void)
{
    m_mainThread = new mainCameraThread(this, &ExynosCamera::m_mainThreadFunc, "ExynosCameraThread", PRIORITY_URGENT_DISPLAY);
    CLOGD("DEBUG(%s):mainThread created", __FUNCTION__);

    m_previewThread = new mainCameraThread(this, &ExynosCamera::m_previewThreadFunc, "previewThread", PRIORITY_DISPLAY);
    CLOGD("DEBUG(%s):previewThread created", __FUNCTION__);

    /*
     * In here, we cannot know single, dual scenario.
     * So, make all threads.
     */
    /* if (m_parameters->isFlite3aaOtf() == true) { */
    if (1) {
        m_mainSetupQThread[INDEX(PIPE_FLITE)] = new mainCameraThread(this, &ExynosCamera::m_mainThreadQSetupFLITE, "mainThreadQSetupFLITE", PRIORITY_URGENT_DISPLAY);
        CLOGD("DEBUG(%s):mainThreadQSetupFLITEThread created", __FUNCTION__);

/* Change 3AA_ISP, 3AC, SCP to ISP */
/*
        m_mainSetupQThread[INDEX(PIPE_3AC)] = new mainCameraThread(this, &ExynosCamera::m_mainThreadQSetup3AC, "mainThreadQSetup3AC", PRIORITY_URGENT_DISPLAY);
        CLOGD("DEBUG(%s):mainThreadQSetup3ACThread created", __FUNCTION__);

        m_mainSetupQThread[INDEX(PIPE_3AA_ISP)] = new mainCameraThread(this, &ExynosCamera::m_mainThreadQSetup3AA_ISP, "mainThreadQSetup3AA_ISP", PRIORITY_URGENT_DISPLAY);
        CLOGD("DEBUG(%s):mainThreadQSetup3AA_ISPThread created", __FUNCTION__);

        m_mainSetupQThread[INDEX(PIPE_ISP)] = new mainCameraThread(this, &ExynosCamera::m_mainThreadQSetupISP, "mainThreadQSetupISP", PRIORITY_URGENT_DISPLAY);
        CLOGD("DEBUG(%s):mainThreadQSetupISPThread created", __FUNCTION__);

        m_mainSetupQThread[INDEX(PIPE_SCP)] = new mainCameraThread(this, &ExynosCamera::m_mainThreadQSetupSCP, "mainThreadQSetupSCP", PRIORITY_URGENT_DISPLAY);
        CLOGD("DEBUG(%s):mainThreadQSetupSCPThread created", __FUNCTION__);
*/

        m_mainSetupQThread[INDEX(PIPE_3AA)] = new mainCameraThread(this, &ExynosCamera::m_mainThreadQSetup3AA, "mainThreadQSetup3AA", PRIORITY_URGENT_DISPLAY);
        CLOGD("DEBUG(%s):mainThreadQSetup3AAThread created", __FUNCTION__);

    }
    m_setBuffersThread = new mainCameraThread(this, &ExynosCamera::m_setBuffersThreadFunc, "setBuffersThread");
    CLOGD("DEBUG(%s):setBuffersThread created", __FUNCTION__);

    m_startPictureInternalThread = new mainCameraThread(this, &ExynosCamera::m_startPictureInternalThreadFunc, "startPictureInternalThread");
    CLOGD("DEBUG(%s):startPictureInternalThread created", __FUNCTION__);

    m_startPictureBufferThread = new mainCameraThread(this, &ExynosCamera::m_startPictureBufferThreadFunc, "startPictureBufferThread");
    CLOGD("DEBUG(%s):startPictureBufferThread created", __FUNCTION__);

    m_prePictureThread = new mainCameraThread(this, &ExynosCamera::m_prePictureThreadFunc, "prePictureThread");
    CLOGD("DEBUG(%s):prePictureThread created", __FUNCTION__);

    m_pictureThread = new mainCameraThread(this, &ExynosCamera::m_pictureThreadFunc, "PictureThread");
    CLOGD("DEBUG(%s):pictureThread created", __FUNCTION__);

    m_postPictureThread = new mainCameraThread(this, &ExynosCamera::m_postPictureThreadFunc, "postPictureThread");
    CLOGD("DEBUG(%s):postPictureThread created", __FUNCTION__);

    m_vraThread = new mainCameraThread(this, &ExynosCamera::m_vraThreadFunc, "vraThread");
    CLOGD("DEBUG(%s[%d]):recordingThread created", __FUNCTION__, __LINE__);

    m_recordingThread = new mainCameraThread(this, &ExynosCamera::m_recordingThreadFunc, "recordingThread");
    CLOGD("DEBUG(%s):recordingThread created", __FUNCTION__);

    m_autoFocusThread = new mainCameraThread(this, &ExynosCamera::m_autoFocusThreadFunc, "AutoFocusThread");
    CLOGD("DEBUG(%s):autoFocusThread created", __FUNCTION__);

    m_autoFocusContinousThread = new mainCameraThread(this, &ExynosCamera::m_autoFocusContinousThreadFunc, "AutoFocusContinousThread");
    CLOGD("DEBUG(%s):autoFocusContinousThread created", __FUNCTION__);

    m_facedetectThread = new mainCameraThread(this, &ExynosCamera::m_facedetectThreadFunc, "FaceDetectThread");
    CLOGD("DEBUG(%s):FaceDetectThread created", __FUNCTION__);

    m_monitorThread = new mainCameraThread(this, &ExynosCamera::m_monitorThreadFunc, "monitorThread");
    CLOGD("DEBUG(%s):monitorThread created", __FUNCTION__);

    m_framefactoryThread = new mainCameraThread(this, &ExynosCamera::m_frameFactoryInitThreadFunc, "FrameFactoryInitThread");
    CLOGD("DEBUG(%s):FrameFactoryInitThread created", __FUNCTION__);

    m_jpegCallbackThread = new mainCameraThread(this, &ExynosCamera::m_jpegCallbackThreadFunc, "jpegCallbackThread");
    CLOGD("DEBUG(%s):jpegCallbackThread created", __FUNCTION__);

    /* saveThread */
    char threadName[20];
    for (int threadNum = JPEG_SAVE_THREAD0; threadNum < JPEG_SAVE_THREAD_MAX_COUNT; threadNum++) {
        snprintf(threadName, sizeof(threadName), "jpegSaveThread%d", threadNum);
        m_jpegSaveThread[threadNum] = new mainCameraThread(this, &ExynosCamera::m_jpegSaveThreadFunc, threadName);
        CLOGD("DEBUG(%s):%s created", __FUNCTION__, threadName);
    }

    /* high resolution preview callback Thread */
    m_highResolutionCallbackThread = new mainCameraThread(this, &ExynosCamera::m_highResolutionCallbackThreadFunc, "m_highResolutionCallbackThread");
    CLOGD("DEBUG(%s):highResolutionCallbackThread created", __FUNCTION__);

    /* vision */
    m_visionThread = new mainCameraThread(this, &ExynosCamera::m_visionThreadFunc, "VisionThread", PRIORITY_URGENT_DISPLAY);
    CLOGD("DEBUG(%s):visionThread created", __FUNCTION__);

    /* Shutter callback */
    m_shutterCallbackThread = new mainCameraThread(this, &ExynosCamera::m_shutterCallbackThreadFunc, "shutterCallbackThread");
    CLOGD("DEBUG(%s):shutterCallbackThread created", __FUNCTION__);

}

status_t ExynosCamera::m_setupFrameFactory(void)
{
    CLOGI("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;

    if (m_parameters->getVisionMode() == true) {
        /* about vision */
        if( m_frameFactory[FRAME_FACTORY_TYPE_VISION] == NULL) {
            m_frameFactory[FRAME_FACTORY_TYPE_VISION] = new ExynosCameraFrameFactoryVision(m_cameraId, m_parameters);
            m_frameFactory[FRAME_FACTORY_TYPE_VISION]->setFrameManager(m_frameMgr);
        }
        m_visionFrameFactory = m_frameFactory[FRAME_FACTORY_TYPE_VISION];

        if (m_frameFactory[FRAME_FACTORY_TYPE_VISION] != NULL && m_frameFactory[FRAME_FACTORY_TYPE_VISION]->isCreated() == false) {
            CLOGD("DEBUG(%s[%d]):setupFrameFactory pushProcessQ(%d)", __FUNCTION__, __LINE__, FRAME_FACTORY_TYPE_VISION);
            m_frameFactoryQ->pushProcessQ(&m_frameFactory[FRAME_FACTORY_TYPE_VISION]);
        }
    } else {
        /* about preview */
        if (m_parameters->getDualMode() == true) {
            m_previewFrameFactory      = m_frameFactory[FRAME_FACTORY_TYPE_DUAL_PREVIEW];
        } else if (m_parameters->getTpuEnabledMode() == true) {
            if (m_parameters->is3aaIspOtf() == true)
                m_previewFrameFactory  = m_frameFactory[FRAME_FACTORY_TYPE_3AA_ISP_OTF_TPU];
            else
                m_previewFrameFactory  = m_frameFactory[FRAME_FACTORY_TYPE_3AA_ISP_M2M_TPU];
        } else {
            if (m_parameters->is3aaIspOtf() == true)
                m_previewFrameFactory  = m_frameFactory[FRAME_FACTORY_TYPE_3AA_ISP_OTF];
            else
                m_previewFrameFactory  = m_frameFactory[FRAME_FACTORY_TYPE_3AA_ISP_M2M];
        }

        /* find previewFrameFactory and push */
        for (int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++) {
            if (m_previewFrameFactory == m_frameFactory[i]) {
                if (m_frameFactory[i] != NULL && m_frameFactory[i]->isCreated() == false) {
                    CLOGD("DEBUG(%s[%d]):setupFrameFactory pushProcessQ(%d)", __FUNCTION__, __LINE__, i);
                    m_frameFactoryQ->pushProcessQ(&m_frameFactory[i]);
                }
                break;
            }
        }

        /* about reprocessing */
        if (m_parameters->isReprocessing() == true) {
            int numOfReprocessingFactory = m_parameters->getNumOfReprocessingFactory();

            for (int i = FRAME_FACTORY_TYPE_REPROCESSING; i < numOfReprocessingFactory + FRAME_FACTORY_TYPE_REPROCESSING; i++) {
                if (m_frameFactory[i] != NULL && m_frameFactory[i]->isCreated() == false) {
                    CLOGD("DEBUG(%s[%d]):setupFrameFactory pushProcessQ(%d)", __FUNCTION__, __LINE__, FRAME_FACTORY_TYPE_REPROCESSING);
                    m_frameFactoryQ->pushProcessQ(&m_frameFactory[i]);
                }
            }
        }
    }

    /*
     * disable until multi-instace is possible.
     */
    /*
    for (int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++) {
        if (m_frameFactory[i] != NULL && m_frameFactory[i]->isCreated() == false) {
            CLOGD("DEBUG(%s[%d]):setupFrameFactory pushProcessQ(%d)", __FUNCTION__, __LINE__, i);
            m_frameFactoryQ->pushProcessQ(&m_frameFactory[i]);
        } else {
            CLOGD("DEBUG(%s[%d]):setupFrameFactory no Push(%d)", __FUNCTION__, __LINE__, i);
        }
    }
    */

    CLOGI("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

status_t ExynosCamera::m_initFrameFactory(void)
{
    CLOGI("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    status_t ret = NO_ERROR;
    ExynosCameraFrameFactory *factory = NULL;

    m_previewFrameFactory = NULL;
    m_pictureFrameFactory = NULL;
    m_reprocessingFrameFactory = NULL;
    m_visionFrameFactory = NULL;

    for(int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++)
        m_frameFactory[i] = NULL;

    /*
     * new all FrameFactories.
     * because this called on open(). so we don't know current scenario
     */

    factory = new ExynosCameraFrameFactory3aaIspM2M(m_cameraId, m_parameters);
    m_frameFactory[FRAME_FACTORY_TYPE_3AA_ISP_M2M] = factory;
    /* hack : for dual */
    if (getCameraId() == CAMERA_ID_FRONT) {
        factory = new ExynosCameraFrameFactoryFront(m_cameraId, m_parameters);
        m_frameFactory[FRAME_FACTORY_TYPE_DUAL_PREVIEW] = factory;
    } else {
        factory = new ExynosCameraFrameFactory3aaIspM2M(m_cameraId, m_parameters);
        m_frameFactory[FRAME_FACTORY_TYPE_DUAL_PREVIEW] = factory;
    }

    factory = new ExynosCameraFrameFactory3aaIspM2MTpu(m_cameraId, m_parameters);
    m_frameFactory[FRAME_FACTORY_TYPE_3AA_ISP_M2M_TPU] = factory;

    factory = new ExynosCameraFrameFactory3aaIspOtf(m_cameraId, m_parameters);
    m_frameFactory[FRAME_FACTORY_TYPE_3AA_ISP_OTF] = factory;

    factory = new ExynosCameraFrameFactory3aaIspOtfTpu(m_cameraId, m_parameters);
    m_frameFactory[FRAME_FACTORY_TYPE_3AA_ISP_OTF_TPU] = factory;

    factory = new ExynosCameraFrameReprocessingFactory(m_cameraId, m_parameters);
    m_frameFactory[FRAME_FACTORY_TYPE_REPROCESSING] = factory;

    factory = new ExynosCameraFrameReprocessingFactoryNV21(m_cameraId, m_parameters);
    m_frameFactory[FRAME_FACTORY_TYPE_REPROCESSING_NV21] = factory;

    for (int i = 0 ; i < FRAME_FACTORY_TYPE_MAX ; i++) {
        factory = m_frameFactory[i];
        if( factory != NULL )
            factory->setFrameManager(m_frameMgr);
    }

    CLOGI("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

status_t ExynosCamera::m_deinitFrameFactory(void)
{
    CLOGI("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;
    ExynosCameraFrameFactory *frameFactory = NULL;

    for (int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++) {
        if (m_frameFactory[i] != NULL) {
            frameFactory = m_frameFactory[i];

            for (int k = i + 1; k < FRAME_FACTORY_TYPE_MAX; k++) {
               if (frameFactory == m_frameFactory[k]) {
                   CLOGD("DEBUG(%s[%d]): m_frameFactory index(%d) and index(%d) are same instance, set index(%d) = NULL",
                       __FUNCTION__, __LINE__, i, k, k);
                   m_frameFactory[k] = NULL;
               }
            }

            if (m_frameFactory[i]->isCreated() == true) {
                ret = m_frameFactory[i]->destroy();
                if (ret < 0)
                    CLOGE("ERR(%s[%d]):m_frameFactory[%d] destroy fail", __FUNCTION__, __LINE__, i);
            }

            SAFE_DELETE(m_frameFactory[i]);

            CLOGD("DEBUG(%s[%d]):m_frameFactory[%d] destroyed", __FUNCTION__, __LINE__, i);
        }
    }

    m_previewFrameFactory = NULL;
    m_pictureFrameFactory = NULL;
    m_reprocessingFrameFactory = NULL;
    m_visionFrameFactory = NULL;

    CLOGI("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return ret;
}

status_t ExynosCamera::m_setFrameManager()
{
    sp<FrameWorker> worker;
    m_frameMgr = new ExynosCameraFrameManager("FRAME MANAGER", m_cameraId, FRAMEMGR_OPER::SLIENT, 100, 150);

    worker = new CreateWorker("CREATE FRAME WORKER", m_cameraId, FRAMEMGR_OPER::SLIENT, 200);
    m_frameMgr->setWorker(FRAMEMGR_WORKER::CREATE, worker);

    worker = new DeleteWorker("DELETE FRAME WORKER", m_cameraId, FRAMEMGR_OPER::SLIENT);
    m_frameMgr->setWorker(FRAMEMGR_WORKER::DELETE, worker);

    sp<KeyBox> key = new KeyBox("FRAME KEYBOX", m_cameraId);

    m_frameMgr->setKeybox(key);

    return NO_ERROR;
}


ExynosCamera::~ExynosCamera()
{
    this->release();
}

void ExynosCamera::release()
{
    CLOGI("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);
    int ret = 0;

    m_stopCompanion();

    if (m_frameMgr != NULL) {
        m_frameMgr->stop();
    }

    /* release all framefactory */
    m_deinitFrameFactory();

    if (m_parameters != NULL) {
        delete m_parameters;
        m_parameters = NULL;
        CLOGD("DEBUG(%s):Parameters(Id=%d) destroyed", __FUNCTION__, m_cameraId);
    }

    /* free all buffers */
    m_releaseBuffers();

    if (m_ionAllocator != NULL) {
        delete m_ionAllocator;
        m_ionAllocator = NULL;
    }

    if (m_grAllocator != NULL) {
        delete m_grAllocator;
        m_grAllocator = NULL;
    }

    if (m_mhbAllocator != NULL) {
        delete m_mhbAllocator;
        m_mhbAllocator = NULL;
    }

    if (m_pipeFrameDoneQ != NULL) {
        delete m_pipeFrameDoneQ;
        m_pipeFrameDoneQ = NULL;
    }

    if (m_zoomPreviwWithCscQ != NULL) {
        delete m_zoomPreviwWithCscQ;
        m_zoomPreviwWithCscQ = NULL;
    }

    /* vision */
    if (m_pipeFrameVisionDoneQ != NULL) {
        delete m_pipeFrameVisionDoneQ;
        m_pipeFrameVisionDoneQ = NULL;
    }

    if (dstIspReprocessingQ != NULL) {
        delete dstIspReprocessingQ;
        dstIspReprocessingQ = NULL;
    }

    if (dstSccReprocessingQ != NULL) {
        delete dstSccReprocessingQ;
        dstSccReprocessingQ = NULL;
    }

    if (dstGscReprocessingQ != NULL) {
        delete dstGscReprocessingQ;
        dstGscReprocessingQ = NULL;
    }

#ifdef STK_PICTURE
    if (dstStkPictureQ != NULL) {
        delete dstStkPictureQ;
        dstStkPictureQ = NULL;
    }
#endif

    if (dstJpegReprocessingQ != NULL) {
        delete dstJpegReprocessingQ;
        dstJpegReprocessingQ = NULL;
    }

    if (m_postPictureQ != NULL) {
        delete m_postPictureQ;
        m_postPictureQ = NULL;
    }

    if (m_jpegCallbackQ != NULL) {
        delete m_jpegCallbackQ;
        m_jpegCallbackQ = NULL;
    }

    if (m_postviewCallbackQ != NULL) {
        delete m_postviewCallbackQ;
        m_postviewCallbackQ = NULL;
    }

    if (m_facedetectQ != NULL) {
        delete m_facedetectQ;
        m_facedetectQ = NULL;
    }

    if (m_previewQ != NULL) {
        delete m_previewQ;
        m_previewQ = NULL;
    }

    if (m_vraThreadQ != NULL) {
        delete m_vraThreadQ;
        m_vraThreadQ = NULL;
    }

    if (m_vraGscDoneQ != NULL) {
        delete m_vraGscDoneQ;
        m_vraGscDoneQ = NULL;
    }

    if (m_vraPipeDoneQ != NULL) {
        delete m_vraPipeDoneQ;
        m_vraPipeDoneQ = NULL;
    }

    for(int i = 0 ; i < MAX_NUM_PIPES ; i++ ) {
        if (m_mainSetupQ[i] != NULL) {
            delete m_mainSetupQ[i];
            m_mainSetupQ[i] = NULL;
        }
    }

    if (m_previewCallbackGscFrameDoneQ != NULL) {
        delete m_previewCallbackGscFrameDoneQ;
        m_previewCallbackGscFrameDoneQ = NULL;
    }

    if (m_recordingQ != NULL) {
        delete m_recordingQ;
        m_recordingQ = NULL;
    }

    for (int threadNum = JPEG_SAVE_THREAD0; threadNum < JPEG_SAVE_THREAD_MAX_COUNT; threadNum++) {
        if (m_jpegSaveQ[threadNum] != NULL) {
            delete m_jpegSaveQ[threadNum];
            m_jpegSaveQ[threadNum] = NULL;
        }
    }

    if (m_highResolutionCallbackQ != NULL) {
        delete m_highResolutionCallbackQ;
        m_highResolutionCallbackQ = NULL;
    }

    if (m_frameFactoryQ != NULL) {
        delete m_frameFactoryQ;
        m_frameFactoryQ = NULL;
        CLOGD("DEBUG(%s):FrameFactoryQ destroyed", __FUNCTION__);
    }

    if (m_bayerBufferMgr != NULL) {
        delete m_bayerBufferMgr;
        m_bayerBufferMgr = NULL;
        CLOGD("DEBUG(%s):BufferManager(bayerBufferMgr) destroyed", __FUNCTION__);
    }

    if (m_3aaBufferMgr != NULL) {
        delete m_3aaBufferMgr;
        m_3aaBufferMgr = NULL;
        CLOGD("DEBUG(%s):BufferManager(3aaBufferMgr) destroyed", __FUNCTION__);
    }

    if (m_ispBufferMgr != NULL) {
        delete m_ispBufferMgr;
        m_ispBufferMgr = NULL;
        CLOGD("DEBUG(%s):BufferManager(ispBufferMgr) destroyed", __FUNCTION__);
    }

    if (m_hwDisBufferMgr != NULL) {
        delete m_hwDisBufferMgr;
        m_hwDisBufferMgr = NULL;
        CLOGD("DEBUG(%s):BufferManager(m_hwDisBufferMgr) destroyed", __FUNCTION__);
    }

    if (m_scpBufferMgr != NULL) {
        delete m_scpBufferMgr;
        m_scpBufferMgr = NULL;
        CLOGD("DEBUG(%s):BufferManager(scpBufferMgr) destroyed", __FUNCTION__);
    }

    if (m_vraBufferMgr != NULL) {
        delete m_vraBufferMgr;
        m_vraBufferMgr = NULL;
        CLOGD("DEBUG(%s[%d]):BufferManager(vraBufferMgr) destroyed", __FUNCTION__, __LINE__);
    }

    if (m_ispReprocessingBufferMgr != NULL) {
        delete m_ispReprocessingBufferMgr;
        m_ispReprocessingBufferMgr = NULL;
        CLOGD("DEBUG(%s):BufferManager(ispReprocessingBufferMgr) destroyed", __FUNCTION__);
    }

    if (m_sccReprocessingBufferMgr != NULL) {
        delete m_sccReprocessingBufferMgr;
        m_sccReprocessingBufferMgr = NULL;
        CLOGD("DEBUG(%s):BufferManager(sccReprocessingBufferMgr) destroyed", __FUNCTION__);
    }

    if (m_sccBufferMgr != NULL) {
        delete m_sccBufferMgr;
        m_sccBufferMgr = NULL;
        CLOGD("DEBUG(%s):BufferManager(sccBufferMgr) destroyed", __FUNCTION__);
    }

    if (m_gscBufferMgr != NULL) {
        delete m_gscBufferMgr;
        m_gscBufferMgr = NULL;
        CLOGD("DEBUG(%s):BufferManager(gscBufferMgr) destroyed", __FUNCTION__);
    }

    if (m_jpegBufferMgr != NULL) {
        delete m_jpegBufferMgr;
        m_jpegBufferMgr = NULL;
        CLOGD("DEBUG(%s):BufferManager(jpegBufferMgr) destroyed", __FUNCTION__);
    }

    if (m_thumbnailBufferMgr != NULL) {
        delete m_thumbnailBufferMgr;
        m_thumbnailBufferMgr = NULL;
        CLOGD("DEBUG(%s[%d]):BufferManager(thumbnailBufferMgr) destroyed", __FUNCTION__, __LINE__);
    }

    if (m_previewCallbackBufferMgr != NULL) {
        delete m_previewCallbackBufferMgr;
        m_previewCallbackBufferMgr = NULL;
        CLOGD("DEBUG(%s):BufferManager(previewCallbackBufferMgr) destroyed", __FUNCTION__);
    }

    if (m_highResolutionCallbackBufferMgr != NULL) {
        delete m_highResolutionCallbackBufferMgr;
        m_highResolutionCallbackBufferMgr = NULL;
        CLOGD("DEBUG(%s):BufferManager(m_highResolutionCallbackBufferMgr) destroyed", __FUNCTION__);
    }

    if (m_recordingBufferMgr != NULL) {
        delete m_recordingBufferMgr;
        m_recordingBufferMgr = NULL;
        CLOGD("DEBUG(%s):BufferManager(recordingBufferMgr) destroyed", __FUNCTION__);
    }

    if (m_captureSelector != NULL) {
        delete m_captureSelector;
        m_captureSelector = NULL;
    }

    if (m_sccCaptureSelector != NULL) {
        delete m_sccCaptureSelector;
        m_sccCaptureSelector = NULL;
    }

    if (m_recordingCallbackHeap != NULL) {
        m_recordingCallbackHeap->release(m_recordingCallbackHeap);
        delete m_recordingCallbackHeap;
        m_recordingCallbackHeap = NULL;
        CLOGD("DEBUG(%s):BufferManager(recordingCallbackHeap) destroyed", __FUNCTION__);
    }

    m_isFirstStart = true;
    m_previewBufferCount = NUM_PREVIEW_BUFFERS;

    if (m_frameMgr != NULL) {
        delete m_frameMgr;
        m_frameMgr = NULL;
    }

    if (m_tempshot != NULL) {
        delete m_tempshot;
        m_tempshot = NULL;
    }

    if (m_fdmeta_shot != NULL) {
        delete m_fdmeta_shot;
        m_fdmeta_shot = NULL;
    }

    if (m_meta_shot != NULL) {
        delete m_meta_shot;
        m_meta_shot = NULL;
    }

    vendorSpecificDestructor();

    CLOGI("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);
}

int ExynosCamera::getCameraId() const
{
    return m_cameraId;
}

int ExynosCamera::getShotBufferIdex() const
{
    return NUM_PLANES(V4L2_PIX_2_HAL_PIXEL_FORMAT(SCC_OUTPUT_COLOR_FMT));
}

void ExynosCamera::setCallbacks(
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void *user)
{
    CLOGI("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    int ret = 0;

    m_notifyCb        = notify_cb;
    m_dataCb          = data_cb;
    m_dataCbTimestamp = data_cb_timestamp;
    m_getMemoryCb     = get_memory;
    m_callbackCookie  = user;

    if (m_mhbAllocator == NULL)
        m_mhbAllocator = new ExynosCameraMHBAllocator();

    ret = m_mhbAllocator->init(get_memory);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]:m_mhbAllocator init failed", __FUNCTION__, __LINE__);
    }
}

void ExynosCamera::enableMsgType(int32_t msgType)
{
    if (m_parameters) {
        CLOGV("INFO(%s[%d]): enable Msg (%x)", __FUNCTION__, __LINE__, msgType);
        m_parameters->enableMsgType(msgType);
    }
}

void ExynosCamera::disableMsgType(int32_t msgType)
{
    if (m_parameters) {
        CLOGV("INFO(%s[%d]): disable Msg (%x)", __FUNCTION__, __LINE__, msgType);
        m_parameters->disableMsgType(msgType);
    }
}

bool ExynosCamera::msgTypeEnabled(int32_t msgType)
{
    bool IsEnabled = false;

    if (m_parameters) {
        CLOGV("INFO(%s[%d]): Msg type enabled (%x)", __FUNCTION__, __LINE__, msgType);
        IsEnabled = m_parameters->msgTypeEnabled(msgType);
    }

    return IsEnabled;
}

bool ExynosCamera::previewEnabled()
{
    CLOGI("INFO(%s[%d]):m_previewEnabled=%d",
        __FUNCTION__, __LINE__, (int)m_previewEnabled);

    /* in scalable mode, we should controll out state */
    if (m_parameters != NULL &&
        (m_parameters->getScalableSensorMode() == true) &&
        (m_scalableSensorMgr.getMode() == EXYNOS_CAMERA_SCALABLE_CHANGING))
        return true;
    else
        return m_previewEnabled;
}

status_t ExynosCamera::storeMetaDataInBuffers(__unused bool enable)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    return OK;
}

bool ExynosCamera::recordingEnabled()
{
    bool ret = m_getRecordingEnabled();
    CLOGI("INFO(%s[%d]):m_recordingEnabled=%d",
        __FUNCTION__, __LINE__, (int)ret);

    return ret;
}

void ExynosCamera::releaseRecordingFrame(const void *opaque)
{
    if (m_parameters != NULL) {
        if (m_parameters->getVisionMode() == true) {
            CLOGW("WRN(%s[%d]): Vision mode does not support", __FUNCTION__, __LINE__);
            android_printAssert(NULL, LOG_TAG, "Cannot support this operation");

            return;
        }
    }

    if (m_getRecordingEnabled() == false) {
        CLOGW("WARN(%s[%d]):m_recordingEnabled equals false", __FUNCTION__, __LINE__);
        /* m_stopRecordingInternal() will wait for recording frame release */
        /* return; */
    }

    if (m_recordingCallbackHeap == NULL) {
        CLOGW("WARN(%s[%d]):recordingCallbackHeap equals NULL", __FUNCTION__, __LINE__);
        return;
    }

    bool found = false;
    struct addrs *recordAddrs  = (struct addrs *)m_recordingCallbackHeap->data;
    struct addrs *releaseFrame = (struct addrs *)opaque;

    if (recordAddrs != NULL) {
        for (int32_t i = 0; i < m_recordingBufferCount; i++) {
            if ((char *)(&(recordAddrs[i])) == (char *)opaque) {
                found = true;
                CLOGV("DEBUG(%s[%d]):releaseFrame->bufIndex=%d, fdY=%d",
                    __FUNCTION__, __LINE__, releaseFrame->bufIndex, releaseFrame->fdPlaneY);

                if (m_doCscRecording == true) {
                    m_releaseRecordingBuffer(releaseFrame->bufIndex);
                } else {
                    m_recordingTimeStamp[releaseFrame->bufIndex] = 0L;
                    m_recordingBufAvailable[releaseFrame->bufIndex] = true;
                }

                break;
            }
            m_isFirstStart = false;
            if (m_parameters != NULL) {
                m_parameters->setIsFirstStartFlag(m_isFirstStart);
            }
        }
    }

    if (found == false)
        CLOGW("WARN(%s[%d]):**** releaseFrame not founded ****", __FUNCTION__, __LINE__);

}

status_t ExynosCamera::autoFocus()
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    if (m_parameters != NULL) {
        if (m_parameters->getVisionMode() == true) {
            CLOGW("WRN(%s[%d]): Vision mode does not support", __FUNCTION__, __LINE__);
            android_printAssert(NULL, LOG_TAG, "Cannot support this operation");

            return INVALID_OPERATION;
        }
    }

    if (m_previewEnabled == false) {
        CLOGE("ERR(%s[%d]): preview is not enabled", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    if (m_autoFocusRunning == false) {
        m_autoFocusType = AUTO_FOCUS_SERVICE;
        m_autoFocusThread->requestExitAndWait();
        m_autoFocusThread->run(PRIORITY_DEFAULT);
    } else {
        CLOGW("WRN(%s[%d]): auto focus is inprogressing", __FUNCTION__, __LINE__);
    }

#if 0 // not used.
    if (m_parameters != NULL) {
        if (m_parameters->getFocusMode() == FOCUS_MODE_AUTO) {
            CLOGI("INFO(%s[%d]) ae awb lock", __FUNCTION__, __LINE__);
            m_parameters->m_setAutoExposureLock(true);
            m_parameters->m_setAutoWhiteBalanceLock(true);
        }
    }
#endif

    return NO_ERROR;
}

status_t ExynosCamera::cancelAutoFocus()
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    if (m_parameters != NULL) {
        if (m_parameters->getVisionMode() == true) {
            CLOGW("WRN(%s[%d]): Vision mode does not support", __FUNCTION__, __LINE__);
            android_printAssert(NULL, LOG_TAG, "Cannot support this operation");

            return INVALID_OPERATION;
        }
    }

    m_autoFocusLock.lock();
    m_autoFocusRunning = false;
    m_autoFocusLock.unlock();

#if 0 // not used.
    if (m_parameters != NULL) {
        if (m_parameters->getFocusMode() == FOCUS_MODE_AUTO) {
            CLOGI("INFO(%s[%d]) ae awb unlock", __FUNCTION__, __LINE__);
            m_parameters->m_setAutoExposureLock(false);
            m_parameters->m_setAutoWhiteBalanceLock(false);
        }
    }
#endif

    if (m_exynosCameraActivityControl->cancelAutoFocus() == false) {
        CLOGE("ERR(%s):Fail on m_secCamera->cancelAutoFocus()", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    /* if autofocusThread is running, we should be wait to receive the AF reseult. */
    m_autoFocusLock.lock();
    m_autoFocusLock.unlock();

    return NO_ERROR;
}

status_t ExynosCamera::cancelPicture()
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    if (m_parameters != NULL) {
        if (m_parameters->getVisionMode() == true) {
            CLOGW("WRN(%s[%d]): Vision mode does not support", __FUNCTION__, __LINE__);
            /* android_printAssert(NULL, LOG_TAG, "Cannot support this operation"); */

            return NO_ERROR;
        }
    }

/*
    m_takePictureCounter.clearCount();
    m_reprocessingCounter.clearCount();
    m_pictureCounter.clearCount();
    m_jpegCounter.clearCount();
*/

    return NO_ERROR;
}

CameraParameters ExynosCamera::getParameters() const
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    return m_parameters->getParameters();
}

int ExynosCamera::getMaxNumDetectedFaces(void)
{
    return m_parameters->getMaxNumDetectedFaces();
}

bool ExynosCamera::m_startFaceDetection(bool toggle)
{
    CLOGD("DEBUG(%s[%d]) toggle : %d", __FUNCTION__, __LINE__, toggle);

    if (toggle == true) {
        m_parameters->setFdEnable(true);
        m_parameters->setFdMode(FACEDETECT_MODE_FULL);
        if (m_parameters->isMcscVraOtf() == false)
            m_previewFrameFactory->startThread(PIPE_VRA);
    } else {
        m_parameters->setFdEnable(false);
        m_parameters->setFdMode(FACEDETECT_MODE_OFF);
        if (m_parameters->isMcscVraOtf() == false) {
            m_vraThread->requestExit();

            if (m_vraThreadQ != NULL)
                m_clearList(m_vraThreadQ);

            if (m_vraGscDoneQ != NULL)
                m_clearList(m_vraGscDoneQ);

            if (m_vraPipeDoneQ != NULL)
                m_clearList(m_vraPipeDoneQ);
        }
    }

    memset(&m_frameMetadata, 0, sizeof(camera_frame_metadata_t));

    return true;
}

bool ExynosCamera::startFaceDetection(void)
{
    if (m_flagStartFaceDetection == true) {
        CLOGD("DEBUG(%s):Face detection already started..", __FUNCTION__);
        return true;
    }

    /* FD-AE is always on */
#ifdef USE_FD_AE
#else
    m_startFaceDetection(true);
#endif

#ifdef ENABLE_FDAF_WITH_FD
    /* Enable FD-AF according to FD condition */
    if(m_parameters->getFdEnable() == 0)
#else
    /* Block FD-AF except for special shot modes */
    if(m_parameters->getShotMode() == SHOT_MODE_BEAUTY_FACE ||
        m_parameters->getShotMode() == SHOT_MODE_SELFIE_ALARM)
#endif
    {
        ExynosCameraActivityAutofocus *autoFocusMgr = m_exynosCameraActivityControl->getAutoFocusMgr();

        if (autoFocusMgr->setFaceDetection(true) == false) {
            CLOGE("ERR(%s[%d]):setFaceDetection(%d)", __FUNCTION__, __LINE__, true);
        } else {
            /* restart CAF when FD mode changed */
            switch (autoFocusMgr->getAutofocusMode()) {
            case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE:
            case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
                if (autoFocusMgr->flagAutofocusStart() == true &&
                    autoFocusMgr->flagLockAutofocus() == false) {
                    autoFocusMgr->stopAutofocus();
                    autoFocusMgr->startAutofocus();
                }
                break;
            default:
                break;
            }
        }
    }

    if (m_facedetectQ->getSizeOfProcessQ() > 0) {
        CLOGE("ERR(%s[%d]):startFaceDetection recordingQ(%d)", __FUNCTION__, __LINE__, m_facedetectQ->getSizeOfProcessQ());
        /*
         * just empty q on m_facedetectQ.
         * m_clearList() can make deadlock by accessing frame
         * deleted on stopPreview()
         */
        /* m_clearList(m_facedetectQ); */
        m_facedetectQ->release();
    }

    m_flagStartFaceDetection = true;

    if (m_facedetectThread->isRunning() == false)
        m_facedetectThread->run();

    return true;
}

bool ExynosCamera::stopFaceDetection(void)
{
    if (m_flagStartFaceDetection == false) {
        CLOGD("DEBUG(%s [%d]):Face detection already stopped..", __FUNCTION__, __LINE__);
        return true;
    }

    ExynosCameraActivityAutofocus *autoFocusMgr = m_exynosCameraActivityControl->getAutoFocusMgr();

    if (autoFocusMgr->setFaceDetection(false) == false) {
        CLOGE("ERR(%s[%d]):setFaceDetection(%d)", __FUNCTION__, __LINE__, false);
    } else {
        /* restart CAF when FD mode changed */
        switch (autoFocusMgr->getAutofocusMode()) {
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
            if (autoFocusMgr->flagAutofocusStart() == true &&
                autoFocusMgr->flagLockAutofocus() == false) {
                autoFocusMgr->stopAutofocus();
                autoFocusMgr->startAutofocus();
            }
            break;
        default:
            break;
        }
    }

    /* FD-AE is always on */
#ifdef USE_FD_AE
#else
    m_startFaceDetection(false);
#endif
    m_flagStartFaceDetection = false;

    return true;
}

bool ExynosCamera::m_getRecordingEnabled(void)
{
    Mutex::Autolock lock(m_recordingStateLock);
    return m_recordingEnabled;
}

void ExynosCamera::m_setRecordingEnabled(bool enable)
{
    Mutex::Autolock lock(m_recordingStateLock);
    m_recordingEnabled = enable;
    return;
}

int ExynosCamera::m_calibratePosition(int w, int new_w, int pos)
{
    return (float)(pos * new_w) / (float)w;
}

bool ExynosCamera::m_facedetectThreadFunc(void)
{
    int32_t status = 0;
    bool ret = true;

    int index = 0;
    int count = 0;

    ExynosCameraFrame *newFrame = NULL;
    uint32_t frameCnt = 0;

    if (m_previewEnabled == false) {
        CLOGD("DEBUG(%s):preview is stopped, thread stop", __FUNCTION__);
        ret = false;
        goto func_exit;
    }

    status = m_facedetectQ->waitAndPopProcessQ(&newFrame);
    if (m_flagThreadStop == true) {
        CLOGI("INFO(%s[%d]):m_flagThreadStop(%d) m_flagStartFaceDetection(%d)", __FUNCTION__, __LINE__, m_flagThreadStop, m_flagStartFaceDetection);
        ret = false;
        goto func_exit;
    }

    if (status < 0) {
        if (status == TIMED_OUT) {
            /* Face Detection time out is not meaningful */
        } else {
            /* TODO: doing exception handling */
            CLOGE("ERR(%s):wait and pop fail, ret(%d)", __FUNCTION__, ret);
        }
        goto func_exit;
    }

    count = m_facedetectQ->getSizeOfProcessQ();
    if (count >= MAX_FACEDETECT_THREADQ_SIZE) {
        CLOGE("ERR(%s[%d]):m_facedetectQ  skipped QSize(%d)", __FUNCTION__, __LINE__, count);
        if (newFrame != NULL) {
            newFrame->decRef();
            m_frameMgr->deleteFrame(newFrame);
            newFrame = NULL;
        }
        for (int i = 0 ; i < count-1 ; i++) {
            m_facedetectQ->popProcessQ(&newFrame);
            if (newFrame != NULL) {
                newFrame->decRef();
                m_frameMgr->deleteFrame(newFrame);
                newFrame = NULL;
            }
        }
        m_facedetectQ->popProcessQ(&newFrame);
    }

    if (newFrame != NULL) {
        status = m_doFdCallbackFunc(newFrame);
        if (status < 0) {
            CLOGE("ERR(%s[%d]) m_doFdCallbackFunc failed(%d).", __FUNCTION__, __LINE__, status);
        }
    }

    if (m_facedetectQ->getSizeOfProcessQ() > 0) {
        ret = true;
    }

    if (newFrame != NULL) {
        newFrame->decRef();
        m_frameMgr->deleteFrame(newFrame);
        newFrame = NULL;
    }

func_exit:

    return ret;
}

status_t ExynosCamera::generateFrame(int32_t frameCount, ExynosCameraFrame **newFrame)
{
    Mutex::Autolock lock(m_frameLock);

    int ret = 0;
    *newFrame = NULL;

    if (frameCount >= 0) {
        ret = m_searchFrameFromList(&m_processList, frameCount, newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):searchFrameFromList fail", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }
    }

    if (*newFrame == NULL) {
        *newFrame = m_previewFrameFactory->createNewFrame();

        if (*newFrame == NULL) {
            CLOGE("ERR(%s):newFrame is NULL", __FUNCTION__);
            return UNKNOWN_ERROR;
        }

        bool flagRequested = false;

        if (m_parameters->isOwnScc(getCameraId()) == true)
            flagRequested = (*newFrame)->getRequest(PIPE_SCC);
        else
            flagRequested = (*newFrame)->getRequest(PIPE_ISPC);

        if (flagRequested == true) {
            m_dynamicSccCount++;
            CLOGV("DEBUG(%s[%d]):dynamicSccCount inc(%d) frameCount(%d)", __FUNCTION__, __LINE__, m_dynamicSccCount, (*newFrame)->getFrameCount());
        }

        m_processList.push_back(*newFrame);
    }

    return ret;
}

status_t ExynosCamera::generateFrameSccScp(uint32_t pipeId, uint32_t *frameCount, ExynosCameraFrame **frame)
{
    int  ret = 0;
    int  regenerateFrame = 0;
    int  count = *frameCount;
    ExynosCameraFrame *newframe = NULL;

    do {
        *frame = NULL;
        ret = generateFrame(count, frame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):generateFrame fail, pipeID: %d", __FUNCTION__, __LINE__, pipeId);
            return ret;
        }

        newframe = *frame;
        if (((PIPE_SCP == pipeId) && newframe->getScpDrop()) ||
            ((m_cameraId == CAMERA_ID_FRONT) && (PIPE_SCC == pipeId) && (newframe->getSccDrop() == true)) ||
            ((m_cameraId == CAMERA_ID_FRONT) && (PIPE_ISPC == pipeId) && (newframe->getIspcDrop() == true))) {
            count++;

            ret = newframe->setEntityState(pipeId, ENTITY_STATE_FRAME_SKIP);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):setEntityState fail, pipeId(%d), state(%d), ret(%d)",
                    __FUNCTION__, __LINE__, pipeId, ENTITY_STATE_FRAME_SKIP, ret);
                *frameCount = count;
                return ret;
            }

            /* let m_mainThreadFunc handle the processlist cleaning part */
            m_pipeFrameDoneQ->pushProcessQ(&newframe);

            regenerateFrame = 1;
            continue;
        }
        regenerateFrame = 0;
    } while (regenerateFrame);

    *frameCount = count;
    return NO_ERROR;
}

status_t ExynosCamera::m_setupEntity(
        uint32_t pipeId,
        ExynosCameraFrame *newFrame,
        ExynosCameraBuffer *srcBuf,
        ExynosCameraBuffer *dstBuf)
{
    int ret = 0;
    entity_buffer_state_t entityBufferState;

    /* set SRC buffer */
    ret = newFrame->getSrcBufferState(pipeId, &entityBufferState);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):getSrcBufferState fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
        return ret;
    }

    if (entityBufferState == ENTITY_BUFFER_STATE_REQUESTED) {
        ret = m_setSrcBuffer(pipeId, newFrame, srcBuf);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_setSrcBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
            return ret;
        }
    }

    /* set DST buffer */
    ret = newFrame->getDstBufferState(pipeId, &entityBufferState);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):getDstBufferState fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
        return ret;
    }

    if (entityBufferState == ENTITY_BUFFER_STATE_REQUESTED) {
        ret = m_setDstBuffer(pipeId, newFrame, dstBuf);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_setDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
            return ret;
        }
    }

    ret = newFrame->setEntityState(pipeId, ENTITY_STATE_PROCESSING);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setEntityState(ENTITY_STATE_PROCESSING) fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
        return ret;
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_setSrcBuffer(
        uint32_t pipeId,
        ExynosCameraFrame *newFrame,
        ExynosCameraBuffer *buffer)
{
    int ret = 0;
    int bufIndex = -1;
    ExynosCameraBufferManager *bufferMgr = NULL;
    ExynosCameraBuffer srcBuf;

    if (buffer == NULL) {
        buffer = &srcBuf;

        ret = m_getBufferManager(pipeId, &bufferMgr, SRC_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getBufferManager(SRC) fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
            return ret;
        }

        if (bufferMgr == NULL) {
            CLOGE("ERR(%s[%d]):buffer manager is NULL, pipeId(%d)", __FUNCTION__, __LINE__, pipeId);
            return BAD_VALUE;
        }

        /* get buffers */
        ret = bufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, buffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getBuffer fail, pipeId(%d), frameCount(%d), ret(%d)",
                __FUNCTION__, __LINE__, pipeId, newFrame->getFrameCount(), ret);
            bufferMgr->dump();
            return ret;
        }
    }

    /* set buffers */
    ret = newFrame->setSrcBuffer(pipeId, *buffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setSrcBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
        return ret;
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_setDstBuffer(
        uint32_t pipeId,
        ExynosCameraFrame *newFrame,
        ExynosCameraBuffer *buffer)
{
    int ret = 0;
    int bufIndex = -1;
    ExynosCameraBufferManager *bufferMgr = NULL;
    ExynosCameraBuffer dstBuf;

    if (buffer == NULL) {
        buffer = &dstBuf;

        ret = m_getBufferManager(pipeId, &bufferMgr, DST_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getBufferManager(DST) fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
            return ret;
        }

        if (bufferMgr == NULL) {
            CLOGE("ERR(%s[%d]):buffer manager is NULL, pipeId(%d)", __FUNCTION__, __LINE__, pipeId);
            return BAD_VALUE;
        }

        /* get buffers */
        ret = bufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, buffer);
        if (ret < 0) {
            ExynosCameraFrameEntity *curEntity = newFrame->searchEntityByPipeId(pipeId);
            if (curEntity != NULL) {
                if (curEntity->getBufType() == ENTITY_BUFFER_DELIVERY) {
                    CLOGV("DEBUG(%s[%d]): pipe(%d) buffer is empty for delivery", __FUNCTION__, __LINE__, pipeId);
                    buffer->index = -1;
                } else {
                    CLOGE("ERR(%s[%d]):getBuffer fail, pipeId(%d), frameCount(%d), ret(%d)",
                            __FUNCTION__, __LINE__, pipeId, newFrame->getFrameCount(), ret);
                    return ret;
                }
            } else {
                CLOGE("ERR(%s[%d]):curEntity is NULL, pipeId(%d)", __FUNCTION__, __LINE__, pipeId);
                return ret;
            }
        }
    }

    /* set buffers */
    ret = newFrame->setDstBuffer(pipeId, *buffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
        return ret;
    }

    return NO_ERROR;
}

status_t ExynosCamera::generateFrameReprocessing(ExynosCameraFrame **newFrame)
{
    Mutex::Autolock lock(m_frameLock);

    int ret = 0;
    struct ExynosCameraBuffer tempBuffer;
    int bufIndex = -1;

     /* 1. Make Frame */
    *newFrame = m_reprocessingFrameFactory->createNewFrame();
    if (*newFrame == NULL) {
        CLOGE("ERR(%s):newFrame is NULL", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_startPictureInternal(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = 0;
    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    unsigned int planeSize[EXYNOS_CAMERA_BUFFER_MAX_PLANES]    = {0};
    int hwPictureW, hwPictureH;
    int maxThumbnailW = 0, maxThumbnailH = 0;
    int planeCount  = 1;
    int minBufferCount = 1;
    int maxBufferCount = 1;
    int numOfReprocessingFactory = 0;
    int pictureFormat = m_parameters->getHwPictureFormat();
    bool needMmap = false;
    exynos_camera_buffer_type_t type = EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE;
    buffer_manager_allocation_mode_t allocMode = BUFFER_MANAGER_ALLOCATION_ONDEMAND;
    ExynosCameraBufferManager *taaBufferManager[MAX_NODE];
    ExynosCameraBufferManager *ispBufferManager[MAX_NODE];
    ExynosCameraBufferManager *mcscBufferManager[MAX_NODE];
    for (int i = 0; i < MAX_NODE; i++) {
        taaBufferManager[i] = NULL;
        ispBufferManager[i] = NULL;
        mcscBufferManager[i] = NULL;
    }

    if (m_zslPictureEnabled == true) {
        CLOGD("DEBUG(%s[%d]): zsl picture is already initialized", __FUNCTION__, __LINE__);
        return NO_ERROR;
    }

    if (m_parameters->isReprocessing() == true) {
        if( m_parameters->getHighSpeedRecording() ) {
            m_parameters->getHwSensorSize(&hwPictureW, &hwPictureH);
            CLOGI("(%s):HW Picture(HighSpeed) width x height = %dx%d", __FUNCTION__, hwPictureW, hwPictureH);
        } else {
            m_parameters->getMaxPictureSize(&hwPictureW, &hwPictureH);
            CLOGI("(%s):HW Picture width x height = %dx%d", __FUNCTION__, hwPictureW, hwPictureH);
        }

        m_parameters->getMaxThumbnailSize(&maxThumbnailW, &maxThumbnailH);

        if (m_parameters->isUseYuvReprocessingForThumbnail() == true)
            needMmap = true;
        else
            needMmap = false;

        if (m_parameters->isHWFCEnabled() == true
            && m_parameters->getHighResolutionCallbackMode() == false) {
            allocMode = BUFFER_MANAGER_ALLOCATION_ATONCE;
        }

        if (SCC_OUTPUT_COLOR_FMT == V4L2_PIX_FMT_NV21M) {
            planeSize[0] = ALIGN_UP(hwPictureW, GSCALER_IMG_ALIGN) * ALIGN_UP(hwPictureH, GSCALER_IMG_ALIGN);
            planeSize[1] = ALIGN_UP(hwPictureW, GSCALER_IMG_ALIGN) * ALIGN_UP(hwPictureH, GSCALER_IMG_ALIGN) / 2;
            planeCount  = 3;
        } else if (SCC_OUTPUT_COLOR_FMT == V4L2_PIX_FMT_NV21) {
            planeSize[0] = ALIGN_UP(hwPictureW, GSCALER_IMG_ALIGN) * ALIGN_UP(hwPictureH, GSCALER_IMG_ALIGN) * 3 / 2;
            planeCount  = 2;
        } else {
        planeSize[0] = ALIGN_UP(hwPictureW, GSCALER_IMG_ALIGN) * ALIGN_UP(hwPictureH, GSCALER_IMG_ALIGN) * 2;
        planeCount  = 2;
        }
        minBufferCount = 1;
        maxBufferCount = NUM_PICTURE_BUFFERS;

        if (m_parameters->getHighResolutionCallbackMode() == true) {
            /* SCC Reprocessing Buffer realloc for high resolution callback */
            minBufferCount = 2;
        }

        ret = m_allocBuffers(m_sccReprocessingBufferMgr, planeCount, planeSize, bytesPerLine,
                minBufferCount, maxBufferCount, type, allocMode, true, needMmap);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_sccReprocessingBufferMgr m_allocBuffers(minBufferCount=%d, maxBufferCount=%d) fail",
                __FUNCTION__, __LINE__, minBufferCount, maxBufferCount);
            return ret;
        }

        if (m_parameters->getUsePureBayerReprocessing() == true) {
            ret = m_setReprocessingBuffer();
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):m_setReprocessing Buffer fail", __FUNCTION__, __LINE__);
                return ret;
            }
        }

        /* Reprocessing Thumbnail Buffer */
        if (pictureFormat == V4L2_PIX_FMT_NV21M) {
            planeCount      = 3;
            planeSize[0]    = maxThumbnailW * maxThumbnailH;
            planeSize[1]    = maxThumbnailW * maxThumbnailH / 2;
        } else {
            planeCount      = 2;
            planeSize[0]    = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(pictureFormat), maxThumbnailW, maxThumbnailH);
        }
        minBufferCount = 1;
        maxBufferCount = m_exynosconfig->current->bufInfo.num_picture_buffers;
        type = EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE;

        ret = m_allocBuffers(m_thumbnailBufferMgr, planeCount, planeSize, bytesPerLine,
                minBufferCount, maxBufferCount, type, allocMode, true, needMmap);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_thumbnailBufferMgr m_allocBuffers(minBufferCount=%d, maxBufferCount=%d) fail",
                    __FUNCTION__, __LINE__, minBufferCount, maxBufferCount);
            return ret;
        }
    }

    numOfReprocessingFactory = m_parameters->getNumOfReprocessingFactory();

    for (int i = FRAME_FACTORY_TYPE_REPROCESSING; i < numOfReprocessingFactory + FRAME_FACTORY_TYPE_REPROCESSING; i++) {
        if (m_frameFactory[i]->isCreated() == false) {
            ret = m_frameFactory[i]->create();
            if (ret < 0) {
                CLOGE("ERR(%s):m_reprocessingFrameFactory->create() failed", __FUNCTION__);
                return ret;
            }

        }

        if (i == FRAME_FACTORY_TYPE_REPROCESSING) {
            m_reprocessingFrameFactory = m_frameFactory[i];
            m_pictureFrameFactory = m_reprocessingFrameFactory;
        }
        CLOGD("DEBUG(%s[%d]):FrameFactory(pictureFrameFactory) created", __FUNCTION__, __LINE__);
        /* If we want set buffer namanger from m_getBufferManager, use this */
#if 0
        ret = m_getBufferManager(PIPE_3AA_REPROCESSING, bufferManager[m_reprocessingFrameFactory->getNodeType(PIPE_3AA_REPROCESSING)], SRC_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getBufferManager() fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, PIPE_3AA_REPROCESSING, ret);
            return ret;
        }

        ret = m_getBufferManager(PIPE_3AA_REPROCESSING, bufferManager[m_reprocessingFrameFactory->getNodeType(PIPE_3AP_REPROCESSING)], DST_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getBufferManager() fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, PIPE_3AP_REPROCESSING, ret);
            return ret;
        }
#else
        if (m_parameters->isReprocessing3aaIspOTF() == false) {
            taaBufferManager[m_frameFactory[i]->getNodeType(PIPE_3AA_REPROCESSING)] = m_bayerBufferMgr;
            taaBufferManager[m_frameFactory[i]->getNodeType(PIPE_3AP_REPROCESSING)] = m_ispReprocessingBufferMgr;
        } else {
            taaBufferManager[m_frameFactory[i]->getNodeType(PIPE_3AA_REPROCESSING)] = m_bayerBufferMgr;
            taaBufferManager[m_frameFactory[i]->getNodeType(PIPE_ISPC_REPROCESSING)] = m_sccReprocessingBufferMgr;

            taaBufferManager[OUTPUT_NODE] = m_bayerBufferMgr;
            taaBufferManager[CAPTURE_NODE] = m_sccReprocessingBufferMgr;
        }
#endif

        ret = m_frameFactory[i]->setBufferManagerToPipe(taaBufferManager, PIPE_3AA_REPROCESSING);
        if (ret < 0) {
            CLOGE("ERR(%s):m_reprocessingFrameFactory->setBufferManagerToPipe() failed", __FUNCTION__);
            return ret;
        }

        ispBufferManager[m_frameFactory[i]->getNodeType(PIPE_ISP_REPROCESSING)] = m_ispReprocessingBufferMgr;
        ispBufferManager[m_frameFactory[i]->getNodeType(PIPE_ISPC_REPROCESSING)] = m_sccReprocessingBufferMgr;

        ret = m_frameFactory[i]->setBufferManagerToPipe(ispBufferManager, PIPE_ISP_REPROCESSING);
        if (ret < 0) {
            CLOGE("ERR(%s):m_reprocessingFrameFactory->setBufferManagerToPipe() failed", __FUNCTION__);
            return ret;
        }

        mcscBufferManager[m_frameFactory[i]->getNodeType(PIPE_MCSC_REPROCESSING)] = m_sccBufferMgr;
        mcscBufferManager[m_frameFactory[i]->getNodeType(PIPE_MCSC0_REPROCESSING)] = m_sccReprocessingBufferMgr;
        mcscBufferManager[m_frameFactory[i]->getNodeType(PIPE_HWFC_JPEG_SRC_REPROCESSING)] = m_sccReprocessingBufferMgr;
        mcscBufferManager[m_frameFactory[i]->getNodeType(PIPE_HWFC_THUMB_SRC_REPROCESSING)] = m_thumbnailBufferMgr;
        mcscBufferManager[m_frameFactory[i]->getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING)] = m_jpegBufferMgr;
        mcscBufferManager[m_frameFactory[i]->getNodeType(PIPE_HWFC_THUMB_DST_REPROCESSING)] = m_thumbnailBufferMgr;

        ret = m_frameFactory[i]->setBufferManagerToPipe(mcscBufferManager, PIPE_MCSC_REPROCESSING);
        if (ret < 0) {
            CLOGE("ERR(%s):m_reprocessingFrameFactory->setBufferManagerToPipe() failed", __FUNCTION__);
            return ret;
        }

        ret = m_frameFactory[i]->initPipes();
        if (ret < 0) {
            CLOGE("ERR(%s):m_reprocessingFrameFactory->initPipes() failed", __FUNCTION__);
            return ret;
        }

        ret = m_frameFactory[i]->preparePipes();
        if (ret < 0) {
            CLOGE("ERR(%s):m_reprocessingFrameFactory preparePipe fail", __FUNCTION__);
            return ret;
        }

        /* stream on pipes */
        ret = m_frameFactory[i]->startPipes();
        if (ret < 0) {
            CLOGE("ERR(%s):m_reprocessingFrameFactory startPipe fail", __FUNCTION__);
            return ret;
        }
    }

    m_zslPictureEnabled = true;

    /*
     * Make remained frameFactory here.
     * in case of reprocessing capture, make here.
     */
    m_framefactoryThread->run();

    return NO_ERROR;
}

bool ExynosCamera::m_mainThreadQSetup3AA()
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif

    int  ret  = 0;
    bool loop = true;
    int  pipeId_3AA    = PIPE_3AA;
    int  pipeId_3AC    = PIPE_3AC;
    int  pipeId_ISP    = PIPE_ISP;
    int  pipeId_DIS    = PIPE_DIS;
    int  pipeIdCsc = 0;
    int  maxbuffers   = 0;
    int  retrycount   = 0;

    ExynosCameraBuffer buffer;
    ExynosCameraFrame  *frame = NULL;
    ExynosCameraFrame  *newframe = NULL;
    nsecs_t timeStamp = 0;
    int frameCount = -1;

    CLOGV("INFO(%s[%d]):wait previewCancelQ", __FUNCTION__, __LINE__);
    ret = m_mainSetupQ[INDEX(pipeId_3AA)]->waitAndPopProcessQ(&frame);
    if (m_flagThreadStop == true) {
        CLOGI("INFO(%s[%d]):m_flagThreadStop(%d)", __FUNCTION__, __LINE__, m_flagThreadStop);
        goto func_exit;
    }

    /* HACK :
     * Prevent to deliver the changed size with per-frame control
     * before the H/W size setting is finished.
     */
    if (m_parameters->getPreviewSizeChanged() == true) {
        CLOGI("INFO(%s[%d]):Preview size is changed. Skip to generate new frame",
                __FUNCTION__, __LINE__);
        goto func_exit;
    }

    if (ret < 0) {
        if (ret == TIMED_OUT) {
            CLOGW("WARN(%s):wait timeout", __FUNCTION__);
        } else {
            CLOGE("ERR(%s):wait and pop fail, ret(%d)", __FUNCTION__, ret);
            /* TODO: doing exception handling */
        }
        goto func_exit;
    }

    if (frame == NULL) {
        CLOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
        goto func_exit;
    }

    do {
        ret = generateFrame(m_3aa_ispFrameCount, &newframe);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):generateFrame fail", __FUNCTION__, __LINE__);
            usleep(100);
        }
        if(++retrycount >= 10) {
           goto func_exit;
        }
    } while((ret < 0) && (retrycount < 10));

    if (newframe == NULL) {
        CLOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
        goto func_exit;
    }

    ret = m_setupEntity(pipeId_3AA, newframe);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setupEntity fail", __FUNCTION__, __LINE__);
    }


    if (m_parameters->is3aaIspOtf() == true) {
        if (m_parameters->isMcscVraOtf() == true)
            m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, pipeId_3AA);

        if (m_parameters->getTpuEnabledMode() == true)
            m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, pipeId_DIS);
    } else {
        m_previewFrameFactory->setFrameDoneQToPipe(m_pipeFrameDoneQ, pipeId_3AA);

        if (m_parameters->getTpuEnabledMode() == true) {
            m_previewFrameFactory->setFrameDoneQToPipe(m_pipeFrameDoneQ, pipeId_ISP);
            m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, pipeId_DIS);
        } else {
            m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, pipeId_ISP);
        }
    }

    if (m_parameters->isMcscVraOtf() == false) {
        m_previewFrameFactory->setFrameDoneQToPipe(m_pipeFrameDoneQ, PIPE_3AA);
        m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, PIPE_VRA);
    }

    m_previewFrameFactory->pushFrameToPipe(&newframe, pipeId_3AA);
    m_3aa_ispFrameCount++;

func_exit:
    if( frame != NULL ) {
        frame->decRef();
        m_frameMgr->deleteFrame(frame);;
        frame = NULL;
    }

    return loop;
}

bool ExynosCamera::m_mainThreadQSetup3AA_ISP()
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif

    int  ret  = 0;
    bool loop = true;
    int  pipeId    = PIPE_3AA_ISP;
    int  pipeIdCsc = 0;
    int  maxbuffers   = 0;

    ExynosCameraBuffer buffer;
    ExynosCameraFrame  *frame = NULL;
    ExynosCameraFrame  *newframe = NULL;
    nsecs_t timeStamp = 0;
    int frameCount = -1;

    CLOGV("INFO(%s[%d]):wait previewCancelQ", __FUNCTION__, __LINE__);
    ret = m_mainSetupQ[INDEX(pipeId)]->waitAndPopProcessQ(&frame);
    if (m_flagThreadStop == true) {
        CLOGI("INFO(%s[%d]):m_flagThreadStop(%d)", __FUNCTION__, __LINE__, m_flagThreadStop);
        goto func_exit;
    }
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        goto func_exit;
    }

    if (frame == NULL) {
        CLOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
        goto func_exit;
    }

    while (m_3aaBufferMgr->getNumOfAvailableBuffer() > 0 &&
            m_ispBufferMgr->getNumOfAvailableBuffer() > 0) {
        ret = generateFrame(m_3aa_ispFrameCount, &newframe);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):generateFrame fail", __FUNCTION__, __LINE__);
            goto func_exit;
        }

        if (newframe == NULL) {
            CLOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
            goto func_exit;
        }

        ret = m_setupEntity(pipeId, newframe);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):setupEntity fail", __FUNCTION__, __LINE__);
        }

        m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, pipeId);
        m_previewFrameFactory->pushFrameToPipe(&newframe, pipeId);

        m_3aa_ispFrameCount++;

    }

func_exit:
    if( frame != NULL ) {
        frame->decRef();
        m_frameMgr->deleteFrame(frame);;
        frame = NULL;
    }

    /*
    if (m_mainSetupQ[INDEX(pipeId)]->getSizeOfProcessQ() <= 0)
        loop = false;
    */

    return loop;
}

bool ExynosCamera::m_mainThreadQSetupISP()
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif

    int  ret  = 0;
    bool loop = false;
    int  pipeId = PIPE_ISP;

    ExynosCameraBuffer buffer;
    ExynosCameraFrame  *frame = NULL;
    nsecs_t timeStamp = 0;
    int frameCount = -1;

    CLOGV("INFO(%s[%d]):wait previewCancelQ", __FUNCTION__, __LINE__);
    ret = m_mainSetupQ[INDEX(pipeId)]->waitAndPopProcessQ(&frame);
    if (m_flagThreadStop == true) {
        CLOGI("INFO(%s[%d]):m_flagThreadStop(%d)", __FUNCTION__, __LINE__, m_flagThreadStop);
        goto func_exit;
    }
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        goto func_exit;
    }

    if (frame == NULL) {
        CLOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
        goto func_exit;
    }

func_exit:
    if( frame != NULL ) {
        frame->decRef();
        m_frameMgr->deleteFrame(frame);
        frame = NULL;
    }

    if (m_mainSetupQ[INDEX(pipeId)]->getSizeOfProcessQ() > 0)
        loop = true;

    return loop;
}

bool ExynosCamera::m_mainThreadQSetupFLITE()
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif

    int  ret  = 0;
    bool loop = true;
    int  pipeId    = PIPE_FLITE;
    int  pipeIdCsc = 0;
    int  maxbuffers   = 0;

    ExynosCameraBuffer buffer;
    ExynosCameraFrame  *frame = NULL;
    ExynosCameraFrame  *newframe = NULL;
    nsecs_t timeStamp = 0;
    int frameCount = -1;

    CLOGV("INFO(%s[%d]):wait previewCancelQ", __FUNCTION__, __LINE__);
    ret = m_mainSetupQ[INDEX(pipeId)]->waitAndPopProcessQ(&frame);
    if (m_flagThreadStop == true) {
        CLOGI("INFO(%s[%d]):m_flagThreadStop(%d)", __FUNCTION__, __LINE__, m_flagThreadStop);
        goto func_exit;
    }
    if (ret < 0) {
        if (ret == TIMED_OUT) {
            CLOGW("WARN(%s):wait timeout", __FUNCTION__);
        } else {
            CLOGE("ERR(%s):wait and pop fail, ret(%d)", __FUNCTION__, ret);
            /* TODO: doing exception handling */
        }
        goto func_exit;
    }


    if (frame == NULL) {
        CLOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
        goto func_exit;
    }

    if (m_bayerBufferMgr->getNumOfAvailableBuffer() > 0) {
        ret = generateFrame(m_fliteFrameCount, &newframe);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):generateFrame fail", __FUNCTION__, __LINE__);
            goto func_exit;
        }

        ret = m_setupEntity(pipeId, newframe);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):setupEntity fail", __FUNCTION__, __LINE__);
        }
        ret = newframe->getDstBuffer(pipeId, &buffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getDstBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, pipeId);
        m_previewFrameFactory->pushFrameToPipe(&newframe, pipeId);

        m_fliteFrameCount++;
    }

func_exit:
    if( frame != NULL ) {
        frame->decRef();
        m_frameMgr->deleteFrame(frame);;
        frame = NULL;
    }

    /*
    if (m_mainSetupQ[INDEX(pipeId)]->getSizeOfProcessQ() <= 0)
        loop = false;
    */

    return loop;
}

bool ExynosCamera::m_mainThreadQSetup3AC()
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif

    int  ret  = 0;
    bool loop = true;
    int  pipeId    = PIPE_3AC;
    int  pipeIdCsc = 0;
    int  maxbuffers   = 0;

    ExynosCameraBuffer buffer;
    ExynosCameraFrame  *frame = NULL;
    ExynosCameraFrame  *newframe = NULL;
    nsecs_t timeStamp = 0;
    int frameCount = -1;

    CLOGV("INFO(%s[%d]):wait previewCancelQ", __FUNCTION__, __LINE__);
    ret = m_mainSetupQ[INDEX(pipeId)]->waitAndPopProcessQ(&frame);
    if (m_flagThreadStop == true) {
        CLOGI("INFO(%s[%d]):m_flagThreadStop(%d)", __FUNCTION__, __LINE__, m_flagThreadStop);
        goto func_exit;
    }
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        goto func_exit;
    }

    if (frame == NULL) {
        CLOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
        goto func_exit;
    }
    if (m_bayerBufferMgr->getNumOfAvailableBuffer() > 0) {
        do {
            ret = generateFrame(m_fliteFrameCount, &newframe);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):generateFrame fail", __FUNCTION__, __LINE__);
                goto func_exit;
            }

            ret = m_setupEntity(pipeId, newframe);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):setupEntity fail", __FUNCTION__, __LINE__);
            }
            ret = newframe->getDstBuffer(pipeId, &buffer);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):getDstBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            }

            m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, pipeId);
            m_previewFrameFactory->pushFrameToPipe(&newframe, pipeId);

            m_fliteFrameCount++;
        //} while (m_bayerBufferMgr->getAllocatedBufferCount() - PIPE_3AC_PREPARE_COUNT
        //         < m_bayerBufferMgr->getNumOfAvailableBuffer());
        } while (0 < m_bayerBufferMgr->getNumOfAvailableBuffer());
    }

func_exit:
    if( frame != NULL ) {
        frame->decRef();
        m_frameMgr->deleteFrame(frame);;
    }

    /*
    if (m_mainSetupQ[INDEX(pipeId)]->getSizeOfProcessQ() <= 0)
        loop = false;
    */

    return loop;
}

bool ExynosCamera::m_mainThreadQSetupSCP()
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif

    int  ret  = 0;
    bool loop = true;
    int  pipeId    = PIPE_SCP;
    int  pipeIdCsc = 0;
    int  maxbuffers   = 0;

    camera2_node_group node_group_info_isp;
    ExynosCameraBuffer resultBuffer;
    ExynosCameraFrame  *frame = NULL;
    ExynosCameraFrame  *newframe = NULL;
    nsecs_t timeStamp = 0;
    int frameCount = -1;
    int frameGen = 1;

    CLOGV("INFO(%s[%d]):wait previewCancelQ", __FUNCTION__, __LINE__);
    ret = m_mainSetupQ[INDEX(pipeId)]->waitAndPopProcessQ(&frame);
    if (m_flagThreadStop == true) {
        CLOGI("INFO(%s[%d]):m_flagThreadStop(%d)", __FUNCTION__, __LINE__, m_flagThreadStop);
        goto func_exit;
    }
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        goto func_exit;
    }

    if (frame == NULL) {
        CLOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
        goto func_exit;
    }

    ret = generateFrameSccScp(pipeId, &m_scpFrameCount, &newframe);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):generateFrameSccScp fail", __FUNCTION__, __LINE__);
        goto func_exit;
    }

    if( frame->getFrameState() == FRAME_STATE_SKIPPED ) {
        ret = frame->getDstBuffer(pipeId, &resultBuffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getDstBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }
        ret = m_setupEntity(pipeId, newframe, NULL, &resultBuffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):setupEntity fail", __FUNCTION__, __LINE__);
        }
        ret = newframe->getDstBuffer(pipeId, &resultBuffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getDstBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }
    } else {
        ret = m_setupEntity(pipeId, newframe);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):setupEntity fail", __FUNCTION__, __LINE__);
        }
        ret = newframe->getDstBuffer(pipeId, &resultBuffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getDstBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }
    }

    /*check preview drop...*/
    ret = newframe->getDstBuffer(pipeId, &resultBuffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):getDstBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
    }
    if (resultBuffer.index < 0) {
        newframe->setRequest(pipeId, false);
        newframe->getNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_ISP);
        node_group_info_isp.capture[PERFRAME_BACK_SCP_POS].request = 0;
        newframe->storeNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_ISP);
        m_previewFrameFactory->dump();

        if (m_getRecordingEnabled() == true
            && m_parameters->msgTypeEnabled(CAMERA_MSG_VIDEO_FRAME)
            && m_parameters->getHighSpeedRecording() == false) {
            CLOGW("WARN(%s[%d]):Recording preview drop. Failed to get preview buffer. FrameSkipCount(%d)",
                    __FUNCTION__, __LINE__, FRAME_SKIP_COUNT_RECORDING);
            /* when preview buffer is not ready, we should drop preview to make preview buffer ready */
            m_parameters->setFrameSkipCount(FRAME_SKIP_COUNT_RECORDING);
        } else {
           CLOGW("WARN(%s[%d]):Preview drop. Failed to get preview buffer. FrameSkipCount (%d)",
                   __FUNCTION__, __LINE__, FRAME_SKIP_COUNT_PREVIEW);
            /* when preview buffer is not ready, we should drop preview to make preview buffer ready */
            m_parameters->setFrameSkipCount(FRAME_SKIP_COUNT_PREVIEW);
        }
    }

    m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, pipeId);
    m_previewFrameFactory->pushFrameToPipe(&newframe, pipeId);

    m_scpFrameCount++;

func_exit:
    if( frame != NULL ) {
        frame->decRef();
        m_frameMgr->deleteFrame(frame);
        frame = NULL;
    }

    /*
    if (m_mainSetupQ[INDEX(pipeId)]->getSizeOfProcessQ() <= 0)
        loop = false;
    */

    return loop;
}

bool ExynosCamera::m_mainThreadFunc(void)
{
    int ret = 0;
    int index = 0;
    ExynosCameraFrame *newFrame = NULL;
    uint32_t frameCnt = 0;

    if (m_previewEnabled == false) {
        CLOGD("DEBUG(%s):preview is stopped, thread stop", __FUNCTION__);
        return false;
    }

    ret = m_pipeFrameDoneQ->waitAndPopProcessQ(&newFrame);
    if (m_flagThreadStop == true) {
        CLOGI("INFO(%s[%d]):m_flagThreadStop(%d)", __FUNCTION__, __LINE__, m_flagThreadStop);
        return false;
    }
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW("WARN(%s):wait timeout", __FUNCTION__);
        } else {
            CLOGE("ERR(%s):wait and pop fail, ret(%d)", __FUNCTION__, ret);
            /* TODO: doing exception handling */
        }
        return true;
    }

    if (newFrame == NULL) {
        CLOGE("ERR(%s[%d]):newFrame is NULL", __FUNCTION__, __LINE__);
        return true;
    }

    frameCnt = newFrame->getFrameCount();

/* HACK Reset Preview Flag*/
#if 0
    if (m_parameters->getRestartPreview() == true) {
        m_resetPreview = true;
        ret = m_restartPreviewInternal();
        m_resetPreview = false;
        CLOGE("INFO(%s[%d]) m_resetPreview(%d)", __FUNCTION__, __LINE__, m_resetPreview);
        if (ret < 0)
            CLOGE("(%s[%d]): restart preview internal fail", __FUNCTION__, __LINE__);

        return true;
    }
#endif

    if (m_parameters->isFlite3aaOtf() == true) {
        ret = m_handlePreviewFrame(newFrame);
    } else {
        if (m_parameters->getDualMode())
            ret = m_handlePreviewFrameFrontDual(newFrame);
        else
            ret = m_handlePreviewFrameFront(newFrame);
    }
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):handle preview frame fail", __FUNCTION__, __LINE__);
        return ret;
    }

    /* Below code block is moved to m_handlePreviewFrame() and m_handlePreviewFrameFront() functions
     * because we want to delete the frame as soon as the setFrameState(FRAME_STATE_COMPLETE) is called.
     * Otherwise, some other thread might be executed between "setFrameState(FRAME_STATE_COMPLETE)" and
     * "delete frame" statements and might delete the same frame. This would cause the second "delete frame"
     * (trying to delete the same frame) to behave abnormally since that frame is already deleted.
     */
#if 0
    /* Don't use this lock */
    m_frameFliteDeleteBetweenPreviewReprocessing.lock();
    if (newFrame->isComplete() == true) {
        ret = m_removeFrameFromList(&m_processList, newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):remove frame from processList fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        if (newFrame->getFrameLockState() == false)
        {
            delete newFrame;
            newFrame = NULL;
        }
    }
    m_frameFliteDeleteBetweenPreviewReprocessing.unlock();
#endif


    /*
     * HACK
     * By using MCpipe. we don't use seperated pipe_scc.
     * so, we will not meet inputFrameQ's fail issue.
     */
    /* m_checkFpsAndUpdatePipeWaitTime(); */

    if(getCameraId() == CAMERA_ID_BACK) {
        m_autoFocusContinousQ.pushProcessQ(&frameCnt);
    }

    return true;
}

bool ExynosCamera::m_frameFactoryInitThreadFunc(void)
{
    CLOGI("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif
    bool loop = false;
    status_t ret = NO_ERROR;

    ExynosCameraFrameFactory *framefactory = NULL;

    ret = m_frameFactoryQ->waitAndPopProcessQ(&framefactory);
    if (m_flagThreadStop == true) {
        CLOGI("INFO(%s[%d]):m_flagThreadStop(%d)", __FUNCTION__, __LINE__, m_flagThreadStop);
        return false;
    }

    if (ret < 0) {
        CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        goto func_exit;
    }

    if (framefactory == NULL) {
        CLOGE("ERR(%s[%d]):framefactory is NULL", __FUNCTION__, __LINE__);
        goto func_exit;
    }

    if (framefactory->isCreated() == false) {
        CLOGD("DEBUG(%s[%d]):framefactory create", __FUNCTION__, __LINE__);
        framefactory->create(false);
    } else {
        CLOGD("DEBUG(%s[%d]):framefactory already create", __FUNCTION__, __LINE__);
    }

func_exit:
    if (0 < m_frameFactoryQ->getSizeOfProcessQ()) {
        if (m_previewEnabled == true) {
            loop = true;
        } else {
            CLOGW("WARN(%s[%d]):Sudden stopPreview. so, stop making frameFactory (m_frameFactoryQ->getSizeOfProcessQ() : %d)",
                __FUNCTION__, __LINE__, m_frameFactoryQ->getSizeOfProcessQ());
            loop = false;
        }
    }

    CLOGI("INFO(%s[%d]):m_framefactoryThread end loop(%d) m_frameFactoryQ(%d), m_previewEnabled(%d)",
        __FUNCTION__, __LINE__, loop, m_frameFactoryQ->getSizeOfProcessQ(), m_previewEnabled);

    CLOGI("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return loop;
}

status_t ExynosCamera::m_setCallbackBufferInfo(ExynosCameraBuffer *callbackBuf, char *baseAddr)
{
    /*
     * If it is not 16-aligend, shrink down it as 16 align. ex) 1080 -> 1072
     * But, memory is set on Android format. so, not aligned area will be black.
     */
    int dst_width = 0, dst_height = 0, dst_crop_width = 0, dst_crop_height = 0;
    int dst_format = m_parameters->getPreviewFormat();

    m_parameters->getPreviewSize(&dst_width, &dst_height);
    dst_crop_width = dst_width;
    dst_crop_height = dst_height;

    if (dst_format == V4L2_PIX_FMT_NV21 ||
        dst_format == V4L2_PIX_FMT_NV21M) {

        callbackBuf->size[0] = (dst_width * dst_height);
        callbackBuf->size[1] = (dst_width * dst_height) / 2;

        callbackBuf->addr[0] = baseAddr;
        callbackBuf->addr[1] = callbackBuf->addr[0] + callbackBuf->size[0];
    } else if (dst_format == V4L2_PIX_FMT_YVU420 ||
               dst_format == V4L2_PIX_FMT_YVU420M) {
        callbackBuf->size[0] = dst_width * dst_height;
        callbackBuf->size[1] = dst_width / 2 * dst_height / 2;
        callbackBuf->size[2] = callbackBuf->size[1];

        callbackBuf->addr[0] = baseAddr;
        callbackBuf->addr[1] = callbackBuf->addr[0] + callbackBuf->size[0];
        callbackBuf->addr[2] = callbackBuf->addr[1] + callbackBuf->size[1];
    }

    CLOGV("DEBUG(%s): dst_size(%dx%d), dst_crop_size(%dx%d)", __FUNCTION__, dst_width, dst_height, dst_crop_width, dst_crop_height);

    return NO_ERROR;
}

status_t ExynosCamera::m_doZoomPrviewWithCSC(int32_t pipeId, int32_t gscPipe, ExynosCameraFrame *frame)
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif
    int32_t previewFormat = 0;
    status_t ret = NO_ERROR;
    ExynosRect srcRect, dstRect;
    ExynosCameraBuffer srcBuf;
    ExynosCameraBuffer dstBuf;
    uint32_t *output = NULL;
    struct camera2_stream *meta = NULL;
    int previewH, previewW;
    int bufIndex = -1;
    int waitCount = 0;
    int scpNodeIndex = -1;
    int srcNodeIndex = -1;
    int srcFmt = -1;

    previewFormat = m_parameters->getHwPreviewFormat();
    m_parameters->getHwPreviewSize(&previewW, &previewH);

    /* To pass the CTS2.0 Test "CameraDeviceTest",
     * When changing of preview size is happen consequently,
     * Prevent to do the Pipe CSC dqbuf before the H/W size setting is finished.
     */
    CLOGV("DEBUG(%s[%d]): getPreviewSizeChanged() : %d",
            __FUNCTION__, __LINE__, m_parameters->getPreviewSizeChanged());

    if (m_parameters->getPreviewSizeChanged() == true) {
        CLOGW("INFO(%s[%d]):Preview size is changed. Skip to doZoomPrviewWithCSC",
                __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    if ((m_parameters->getPreviewSizeChanged() == false) &&
        (m_oldPreviewW != (unsigned)previewW) &&
        (m_oldPreviewH != (unsigned)previewH)) {
        CLOGI("INFO(%s[%d]):HW Preview size is changed. Update size(%d x %d)",
                __FUNCTION__, __LINE__, previewW, previewH);

        m_oldPreviewW = previewW;
        m_oldPreviewH = previewH;
    }

    /* get Scaler src Buffer Node Index*/
    if (m_parameters->getDualMode() != true) {
        srcNodeIndex = m_previewFrameFactory->getNodeType(PIPE_SCP);
        srcFmt = previewFormat;
        scpNodeIndex = m_previewFrameFactory->getNodeType(PIPE_SCP);
    } else {
        srcFmt = previewFormat;
        return true;
    }

    /* get scaler source buffer */
    srcBuf.index = -1;
    ret = frame->getDstBuffer(pipeId, &srcBuf, srcNodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",__FUNCTION__, __LINE__, pipeId, ret);
        return ret;
    }

    /* getMetadata to get buffer size */
    meta = (struct camera2_stream*)srcBuf.addr[srcBuf.planeCount-1];
    if (meta == NULL) {
        CLOGE("ERR(%s[%d]):srcBuf.addr is NULL, srcBuf.addr(0x%x)",__FUNCTION__, __LINE__, srcBuf.addr[srcBuf.planeCount-1]);
        return INVALID_OPERATION;
    }

    output = meta->output_crop_region;

    /* check scaler conditions( compare the crop size & format ) */
    if (output[2] == (unsigned)previewW && output[3] == (unsigned)previewH && srcFmt == previewFormat
#ifdef USE_PREVIEW_CROP_FOR_ROATAION
        && m_parameters->getRotationProperty() != FORCE_PREVIEW_BUFFER_CROP_ROTATION_270
#endif
        ) {
        /* HACK for CTS2.0 */
        m_oldPreviewW = previewW;
        m_oldPreviewH = previewH;

        return ret;
    }

    /* wait unitil get buffers */
    do {
        dstBuf.index = -2;
        waitCount++;

        if (m_scpBufferMgr->getNumOfAvailableBuffer() > 0)
            m_scpBufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &dstBuf);

        if (dstBuf.index < 0) {
            usleep(WAITING_TIME);

            if (waitCount % 20 == 0) {
                CLOGW("WRN(%s[%d]):retry JPEG getBuffer(%d) m_zoomPreviwWithCscQ->getSizeOfProcessQ(%d)",
                    __FUNCTION__, __LINE__, bufIndex, m_zoomPreviwWithCscQ->getSizeOfProcessQ());
                m_scpBufferMgr->dump();
            }
        } else {
            break;
        }
        /* this will retry until 300msec */
    } while (waitCount < (TOTAL_WAITING_TIME / WAITING_TIME) && previewEnabled() == false);

    if (bufIndex == -1) {
        m_scpBufferMgr->cancelBuffer(srcBuf.index);
        CLOGE("ERR(%s[%d]):Failed to get the gralloc Buffer for GSC dstBuf ",__FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    /* csc and scaling */
    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.w = output[2];
    srcRect.h = output[3];
    srcRect.fullW = output[2];
    srcRect.fullH = output[3];
    srcRect.colorFormat = srcFmt;

#ifdef USE_PREVIEW_CROP_FOR_ROATAION
    if (m_parameters->getRotationProperty() == FORCE_PREVIEW_BUFFER_CROP_ROTATION_270) {
        int old_width = srcRect.w;
        if (srcRect.w > srcRect.h) {
            srcRect.w = ALIGN_DOWN(srcRect.h*srcRect.h / srcRect.w, GSCALER_IMG_ALIGN);
            srcRect.x = ALIGN_DOWN((old_width-srcRect.w) / 2, GSCALER_IMG_ALIGN);
        }
    }
#endif

    dstRect.x = 0;
    dstRect.y = 0;
    dstRect.w = previewW;
    dstRect.h = previewH;
    dstRect.fullW = previewW;
    dstRect.fullH = previewH;
    dstRect.colorFormat = previewFormat;

    CLOGV("DEBUG(%s[%d]): srcBuf(%d) dstBuf(%d) (%d, %d, %d, %d) format(%d) actual(%x) -> (%d, %d, %d, %d) format(%d)  actual(%x)",
        __FUNCTION__, __LINE__, srcBuf.index, dstBuf.index,
        srcRect.x, srcRect.y, srcRect.w, srcRect.h, srcFmt, V4L2_PIX_2_HAL_PIXEL_FORMAT(srcFmt),
        dstRect.x, dstRect.y, dstRect.w, dstRect.h, previewFormat, V4L2_PIX_2_HAL_PIXEL_FORMAT(previewFormat));

    ret = frame->setSrcRect(gscPipe, srcRect);
    ret = frame->setDstRect(gscPipe, dstRect);

    ret = m_setupEntity(gscPipe, frame, &srcBuf, &dstBuf);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setupEntity fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, gscPipe, ret);
    }

#ifdef USE_PREVIEW_CROP_FOR_ROATAION
    if (m_parameters->getRotationProperty() == FORCE_PREVIEW_BUFFER_CROP_ROTATION_270) {
        ret = frame->setRotation(gscPipe, 270);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setRotation(%d) fail", __FUNCTION__, __LINE__, 270);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }
#endif

    m_previewFrameFactory->setOutputFrameQToPipe(m_zoomPreviwWithCscQ, gscPipe);
    m_previewFrameFactory->pushFrameToPipe(&frame, gscPipe);

    /* wait GSC done */
    CLOGV("INFO(%s[%d]):wait GSC output", __FUNCTION__, __LINE__);
    waitCount = 0;

    do {
        ret = m_zoomPreviwWithCscQ->waitAndPopProcessQ(&frame);
        waitCount++;
    } while (ret == TIMED_OUT && waitCount < 100 && m_flagThreadStop != true);

    if (ret < 0) {
        CLOGW("WARN(%s[%d]):GSC wait and pop return, ret(%d)", __FUNCTION__, __LINE__, ret);
    }
    if (frame == NULL) {
        CLOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
        goto func_exit;
    }

    /* copy metadata src to dst*/
    memcpy(dstBuf.addr[dstBuf.planeCount-1],srcBuf.addr[srcBuf.planeCount-1], sizeof(struct camera2_stream));

    if (m_parameters->getDualMode() == true && getCameraId() == CAMERA_ID_FRONT) {
        /* in case of dual front path buffer returned frameSelector, do not return buffer. */
    } else {
        m_scpBufferMgr->cancelBuffer(srcBuf.index);
    }

    ret = frame->setDstBufferState(pipeId, ENTITY_BUFFER_STATE_REQUESTED, scpNodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s): setDstBufferState state fail", __FUNCTION__);
        return ret;
    }

    ret = frame->setDstBuffer(pipeId, dstBuf, scpNodeIndex);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setdst Buffer failed(%d)", __FUNCTION__, __LINE__, ret);
    }

    ret = frame->setDstBufferState(pipeId, ENTITY_BUFFER_STATE_COMPLETE, scpNodeIndex);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s): setDstBufferState state fail", __FUNCTION__);
        return ret;
    }

func_exit:

    CLOGV("DEBUG(%s[%d]):--OUT--", __FUNCTION__, __LINE__);

    return ret;
}

bool ExynosCamera::m_setBuffersThreadFunc(void)
{
    int ret = 0;

    ret = m_setBuffers();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_setBuffers failed, releaseBuffer", __FUNCTION__, __LINE__);

        /* TODO: Need release buffers and error exit */

        m_releaseBuffers();
        m_isSuccessedBufferAllocation = false;
        return false;
    }

    m_isSuccessedBufferAllocation = true;
    return false;
}

bool ExynosCamera::m_startPictureInternalThreadFunc(void)
{
    int ret = 0;

    ret = m_startPictureInternal();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_setBuffers failed", __FUNCTION__, __LINE__);

        /* TODO: Need release buffers and error exit */

        return false;
    }

    return false;
}

bool ExynosCamera::m_prePictureThreadFunc(void)
{
    bool loop = false;
    bool isProcessed = true;

    ExynosCameraDurationTimer m_burstPrePictureTimer;
    uint64_t m_burstPrePictureTimerTime;
    uint64_t burstWaitingTime;

    status_t ret = NO_ERROR;

    uint64_t seriesShotDuration = m_parameters->getSeriesShotDuration();

    m_burstPrePictureTimer.start();

    if (m_parameters->isReprocessing())
        loop = m_reprocessingPrePictureInternal();
    else
        loop = m_prePictureInternal(&isProcessed);

    m_burstPrePictureTimer.stop();
    m_burstPrePictureTimerTime = m_burstPrePictureTimer.durationUsecs();

    if(isProcessed && loop && seriesShotDuration > 0 && m_burstPrePictureTimerTime < seriesShotDuration) {
        CLOGD("DEBUG(%s[%d]): The time between shots is too short(%lld)us. Extended to (%lld)us"
            , __FUNCTION__, __LINE__, m_burstPrePictureTimerTime, seriesShotDuration);

        burstWaitingTime = seriesShotDuration - m_burstPrePictureTimerTime;
        usleep(burstWaitingTime);
    }

    return loop;
}

/*
 * pIsProcessed : out parameter
 *                true if the frame is properly handled.
 *                false if frame processing is failed or there is no frame to process
 */
bool ExynosCamera::m_prePictureInternal(bool* pIsProcessed)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    CLOGI("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    int ret = 0;
    bool loop = false;
    ExynosCameraFrame *newFrame = NULL;
    camera2_shot_ext *shot_ext = NULL;

    ExynosCameraBuffer fliteReprocessingBuffer;
    ExynosCameraBuffer ispReprocessingBuffer;
#ifdef DEBUG_RAWDUMP
    ExynosCameraBuffer bayerBuffer;
    ExynosCameraBuffer pictureBuffer;
    ExynosCameraFrame *inListFrame = NULL;
    ExynosCameraFrame *bayerFrame = NULL;
#endif
    int pipeId = 0;
    int bufPipeId = 0;
    bool isSrc = false;
    int retryCount = 3;

    if (m_hdrEnabled)
        retryCount = 15;

    if (m_parameters->isOwnScc(getCameraId()) == true)
        bufPipeId = PIPE_SCC;
    else if (m_parameters->isUsing3acForIspc() == true)
        bufPipeId = PIPE_3AC;
    else
        bufPipeId = PIPE_ISPC;

    if (m_parameters->is3aaIspOtf() == true)
        pipeId = PIPE_3AA;
    else
        pipeId = PIPE_ISP;

    int postProcessQSize = m_postPictureQ->getSizeOfProcessQ();
    if (postProcessQSize > 2) {
        CLOGW("DEBUG(%s[%d]): post picture is delayed(stacked %d frames), skip", __FUNCTION__, __LINE__, postProcessQSize);
        usleep(WAITING_TIME);
        goto CLEAN;
    }

    if (m_parameters->getRecordingHint() == true
        && m_parameters->isUsing3acForIspc() == true)
        m_sccCaptureSelector->clearList(pipeId, false, m_previewFrameFactory->getNodeType(bufPipeId));

    newFrame = m_sccCaptureSelector->selectFrames(m_reprocessingCounter.getCount(), pipeId, isSrc, retryCount, m_pictureFrameFactory->getNodeType(bufPipeId));
    if (newFrame == NULL) {
        CLOGE("ERR(%s[%d]):newFrame is NULL", __FUNCTION__, __LINE__);
        goto CLEAN;
    }
    newFrame->frameLock();

    CLOGI("DEBUG(%s[%d]):Frame Count (%d)", __FUNCTION__, __LINE__, newFrame->getFrameCount());

    m_postProcessList.push_back(newFrame);

    if ((m_parameters->getHighResolutionCallbackMode() == true) &&
        (m_highResolutionCallbackRunning == true)) {
        m_highResolutionCallbackQ->pushProcessQ(&newFrame);
    } else {
        dstSccReprocessingQ->pushProcessQ(&newFrame);
    }

    m_reprocessingCounter.decCount();

    CLOGI("INFO(%s[%d]):prePicture complete, remaining count(%d)", __FUNCTION__, __LINE__, m_reprocessingCounter.getCount());

    if (m_hdrEnabled) {
        ExynosCameraActivitySpecialCapture *m_sCaptureMgr;

        m_sCaptureMgr = m_exynosCameraActivityControl->getSpecialCaptureMgr();

        if (m_reprocessingCounter.getCount() == 0)
            m_sCaptureMgr->setCaptureStep(ExynosCameraActivitySpecialCapture::SCAPTURE_STEP_OFF);
    }

    if ((m_parameters->getHighResolutionCallbackMode() == true) &&
        (m_highResolutionCallbackRunning == true))
        loop = true;

    if (m_reprocessingCounter.getCount() > 0) {
        loop = true;

    }

    *pIsProcessed = true;
#ifdef DEBUG_RAWDUMP
    retryCount = 30; /* 200ms x 30 */
    bayerBuffer.index = -2;

    m_captureSelector->setWaitTime(200000000);
    bayerFrame = m_captureSelector->selectFrames(1, PIPE_FLITE, isSrc, retryCount);
    if (bayerFrame == NULL) {
        CLOGE("ERR(%s[%d]):bayerFrame is NULL", __FUNCTION__, __LINE__);
    } else {
        ret = bayerFrame->getDstBuffer(PIPE_FLITE, &bayerBuffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): getDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, PIPE_FLITE, ret);
        } else {
            if (m_parameters->checkBayerDumpEnable()) {
                int sensorMaxW, sensorMaxH;
                int sensorMarginW, sensorMarginH;
                bool bRet;
                char filePath[70];
                int fliteFcount = 0;
                int pictureFcount = 0;

                camera2_shot_ext *shot_ext = NULL;

                ret = newFrame->getDstBuffer(PIPE_3AA, &pictureBuffer);
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]): getDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, PIPE_3AA, ret);
                }

                shot_ext = (camera2_shot_ext *)(bayerBuffer.addr[1]);
                if (shot_ext != NULL)
                    fliteFcount = shot_ext->shot.dm.request.frameCount;
                else
                    ALOGE("ERR(%s[%d]):fliteBayerBuffer is null", __FUNCTION__, __LINE__);

                shot_ext->shot.dm.request.frameCount = 0;
                shot_ext = (camera2_shot_ext *)(pictureBuffer.addr[1]);
                if (shot_ext != NULL)
                    pictureFcount = shot_ext->shot.dm.request.frameCount;
                else
                    ALOGE("ERR(%s[%d]):PictureBuffer is null", __FUNCTION__, __LINE__);

                CLOGD("DEBUG(%s[%d]):bayer fcount(%d) picture fcount(%d)", __FUNCTION__, __LINE__, fliteFcount, pictureFcount);
                /* The driver frame count is used to check the match between the 3AA frame and the FLITE frame.
                   if the match fails then the bayer buffer does not correspond to the capture output and hence
                   not written to the file */
                if (fliteFcount == pictureFcount) {
                    memset(filePath, 0, sizeof(filePath));
                    snprintf(filePath, sizeof(filePath), "/data/media/0/RawCapture%d_%d.raw",m_cameraId, pictureFcount);

                    bRet = dumpToFile((char *)filePath,
                        bayerBuffer.addr[0],
                        bayerBuffer.size[0]);
                    if (bRet != true)
                        CLOGE("couldn't make a raw file");
                }
            }

            if (bayerFrame != NULL) {
                ret = m_bayerBufferMgr->putBuffer(bayerBuffer.index, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL);
                if (ret < 0) {
                    ALOGE("ERR(%s[%d]):putBuffer failed Index is %d", __FUNCTION__, __LINE__, bayerBuffer.index);
                    m_bayerBufferMgr->printBufferState();
                    m_bayerBufferMgr->printBufferQState();
                }
                if (m_frameMgr != NULL) {
#ifdef USE_FRAME_REFERENCE_COUNT
                    bayerFrame->decRef();
#endif
                    m_frameMgr->deleteFrame(bayerFrame);
                } else {
                        ALOGE("ERR(%s[%d]):m_frameMgr is NULL (%d)", __FUNCTION__, __LINE__, bayerFrame->getFrameCount());
                }
                bayerFrame = NULL;
            }
        }
    }
#endif
    return loop;

CLEAN:
    if (newFrame != NULL) {
        newFrame->printEntity();
        CLOGD("DEBUG(%s[%d]): Picture frame delete(%d)", __FUNCTION__, __LINE__, newFrame->getFrameCount());
        newFrame->decRef();
        m_frameMgr->deleteFrame(newFrame);
        newFrame = NULL;
    }

    if (m_hdrEnabled) {
        ExynosCameraActivitySpecialCapture *m_sCaptureMgr;

        m_sCaptureMgr = m_exynosCameraActivityControl->getSpecialCaptureMgr();

        if (m_reprocessingCounter.getCount() == 0)
            m_sCaptureMgr->setCaptureStep(ExynosCameraActivitySpecialCapture::SCAPTURE_STEP_OFF);
    }

    if (m_reprocessingCounter.getCount() > 0)
        loop = true;

    CLOGI("INFO(%s[%d]): prePicture fail, remaining count(%d)", __FUNCTION__, __LINE__, m_reprocessingCounter.getCount());
    *pIsProcessed = false;   // Notify failure
    return loop;

}


bool ExynosCamera::m_highResolutionCallbackThreadFunc(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    CLOGI("INFO(%s[%d]):", __FUNCTION__, __LINE__);

    int ret = 0;
    int loop = false;
    int retryCountGSC = 4;

    ExynosCameraFrame *newFrame = NULL;
    camera2_stream *shot_stream = NULL;

    ExynosCameraBuffer sccReprocessingBuffer;
    ExynosCameraBuffer highResolutionCbBuffer;

    int cbPreviewW = 0, cbPreviewH = 0;
    int previewFormat = 0;
    ExynosRect srcRect, dstRect;
    m_parameters->getPreviewSize(&cbPreviewW, &cbPreviewH);
    previewFormat = m_parameters->getPreviewFormat();

    int pipeId_scc = 0;
    int pipeId_gsc = 0;

    unsigned int planeSize[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    int planeCount = getYuvPlaneCount(previewFormat);
    int minBufferCount = 1;
    int maxBufferCount = 1;
    int buffer_idx = getShotBufferIdex();

    sccReprocessingBuffer.index = -2;
    highResolutionCbBuffer.index = -2;

    if (m_parameters->isUseYuvReprocessing() == true) {
        pipeId_scc = PIPE_MCSC_REPROCESSING;
        pipeId_gsc = PIPE_GSC_REPROCESSING;
    } else if (m_parameters->isUsing3acForIspc() == true) {
        pipeId_scc = PIPE_3AA;
        pipeId_gsc = PIPE_GSC_PICTURE;
    } else {
        pipeId_scc = (m_parameters->isOwnScc(getCameraId()) == true) ? PIPE_SCC_REPROCESSING : PIPE_ISP_REPROCESSING;
        pipeId_gsc = PIPE_GSC_REPROCESSING;
    }

    exynos_camera_buffer_type_t type = EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE;
    buffer_manager_allocation_mode_t allocMode = BUFFER_MANAGER_ALLOCATION_ONDEMAND;

    if (m_parameters->getHighResolutionCallbackMode() == false &&
        m_highResolutionCallbackRunning == false) {
        CLOGD("DEBUG(%s[%d]): High Resolution Callback Stop", __FUNCTION__, __LINE__);
        goto CLEAN;
    }

    ret = getYuvPlaneSize(previewFormat, planeSize, cbPreviewW, cbPreviewH);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): BAD value, format(%x), size(%dx%d)",
            __FUNCTION__, __LINE__, previewFormat, cbPreviewW, cbPreviewH);
        return ret;
    }

    /* wait SCC */
    CLOGV("INFO(%s[%d]):wait SCC output", __FUNCTION__, __LINE__);
    ret = m_highResolutionCallbackQ->waitAndPopProcessQ(&newFrame);
    if (m_flagThreadStop == true) {
        CLOGI("INFO(%s[%d]):m_flagThreadStop(%d)", __FUNCTION__, __LINE__, m_flagThreadStop);
        goto CLEAN;
    }
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        // TODO: doing exception handling
        goto CLEAN;
    }
    if (newFrame == NULL) {
        CLOGE("ERR(%s[%d]):newFrame is NULL", __FUNCTION__, __LINE__);
        goto CLEAN;
    }

    ret = newFrame->setEntityState(pipeId_scc, ENTITY_STATE_COMPLETE);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setEntityState(ENTITY_STATE_PROCESSING) fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_scc, ret);
        return ret;
    }
    CLOGV("INFO(%s[%d]):SCC output done", __FUNCTION__, __LINE__);

    if (m_parameters->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME)) {
        /* get GSC src buffer */
        if (m_parameters->isUseYuvReprocessing() == true) {
            ret = newFrame->getDstBuffer(pipeId_scc, &highResolutionCbBuffer, m_reprocessingFrameFactory->getNodeType(PIPE_MCSC0_REPROCESSING));
        } else if (m_parameters->isUsing3acForIspc() == true)
            ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_previewFrameFactory->getNodeType(PIPE_3AC));
        else
            ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_reprocessingFrameFactory->getNodeType(PIPE_SCC_REPROCESSING));
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, pipeId_scc, ret);
            goto CLEAN;
        }

        if (m_parameters->isUseYuvReprocessing() == false) {
            shot_stream = (struct camera2_stream *)(sccReprocessingBuffer.addr[buffer_idx]);
            if (shot_stream == NULL) {
                CLOGE("ERR(%s[%d]):shot_stream is NULL. buffer(%d)",
                        __FUNCTION__, __LINE__, sccReprocessingBuffer.index);
                goto CLEAN;
            }

            /* alloc GSC buffer */
            if (m_highResolutionCallbackBufferMgr->isAllocated() == false) {
                ret = m_allocBuffers(m_highResolutionCallbackBufferMgr, planeCount, planeSize, bytesPerLine, minBufferCount, maxBufferCount, type, allocMode, false, false);
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):m_highResolutionCallbackBufferMgr m_allocBuffers(minBufferCount=%d, maxBufferCount=%d) fail",
                            __FUNCTION__, __LINE__, minBufferCount, maxBufferCount);
                    return ret;
                }
            }

            /* get GSC dst buffer */
            int bufIndex = -2;
            m_highResolutionCallbackBufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &highResolutionCbBuffer);
        }

        /* get preview callback heap */
        camera_memory_t *previewCallbackHeap = NULL;
        previewCallbackHeap = m_getMemoryCb(highResolutionCbBuffer.fd[0], highResolutionCbBuffer.size[0], 1, m_callbackCookie);
        if (!previewCallbackHeap || previewCallbackHeap->data == MAP_FAILED) {
            CLOGE("ERR(%s[%d]):m_getMemoryCb(%d) fail", __FUNCTION__, __LINE__, highResolutionCbBuffer.size[0]);
            goto CLEAN;
        }

        ret = m_setCallbackBufferInfo(&highResolutionCbBuffer, (char *)previewCallbackHeap->data);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): setCallbackBufferInfo fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            goto CLEAN;
        }

        if (m_parameters->isUseYuvReprocessing() == false) {
            /* set src/dst rect */
            srcRect.x = shot_stream->output_crop_region[0];
            srcRect.y = shot_stream->output_crop_region[1];
            srcRect.w = shot_stream->output_crop_region[2];
            srcRect.h = shot_stream->output_crop_region[3];

            ret = m_calcHighResolutionPreviewGSCRect(&srcRect, &dstRect);
            ret = newFrame->setSrcRect(pipeId_gsc, &srcRect);
            ret = newFrame->setDstRect(pipeId_gsc, &dstRect);

            CLOGV("DEBUG(%s[%d]):srcRect x : %d, y : %d, w : %d, h : %d", __FUNCTION__, __LINE__, srcRect.x, srcRect.y, srcRect.w, srcRect.h);
            CLOGV("DEBUG(%s[%d]):dstRect x : %d, y : %d, w : %d, h : %d", __FUNCTION__, __LINE__, dstRect.x, dstRect.y, dstRect.w, dstRect.h);

            ret = m_setupEntity(pipeId_gsc, newFrame, &sccReprocessingBuffer, &highResolutionCbBuffer);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):setupEntity fail, pipeId(%d), ret(%d)",
                        __FUNCTION__, __LINE__, pipeId_gsc, ret);
                goto CLEAN;
            }

            /* push frame to GSC pipe */
            m_pictureFrameFactory->setOutputFrameQToPipe(dstGscReprocessingQ, pipeId_gsc);
            m_pictureFrameFactory->pushFrameToPipe(&newFrame, pipeId_gsc);

            /* wait GSC for high resolution preview callback */
            CLOGI("INFO(%s[%d]):wait GSC output", __FUNCTION__, __LINE__);
            while (retryCountGSC > 0) {
                ret = dstGscReprocessingQ->waitAndPopProcessQ(&newFrame);
                if (ret == TIMED_OUT) {
                    CLOGW("WRN(%s)(%d):wait and pop timeout, ret(%d)", __FUNCTION__, __LINE__, ret);
                    m_pictureFrameFactory->startThread(pipeId_gsc);
                } else if (ret < 0) {
                    CLOGE("ERR(%s)(%d):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                    /* TODO: doing exception handling */
                    goto CLEAN;
                } else {
                    break;
                }
                retryCountGSC--;
            }

            if (newFrame == NULL) {
                CLOGE("ERR(%s):newFrame is NULL", __FUNCTION__);
                goto CLEAN;
            }
            CLOGI("INFO(%s[%d]):GSC output done", __FUNCTION__, __LINE__);

            /* put SCC buffer */
            if (m_parameters->isUsing3acForIspc() == true) {
                ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_previewFrameFactory->getNodeType(PIPE_3AC));
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_scc, ret);
                    goto CLEAN;
                }
                ret = m_putBuffers(m_sccBufferMgr, sccReprocessingBuffer.index);
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):put Buffer fail", __FUNCTION__, __LINE__);
                }
            } else {
                ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_reprocessingFrameFactory->getNodeType(PIPE_SCC_REPROCESSING));
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_scc, ret);
                    goto CLEAN;
                }
                ret = m_putBuffers(m_sccReprocessingBufferMgr, sccReprocessingBuffer.index);
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):put Buffer fail", __FUNCTION__, __LINE__);
                }
            }
        }

        CLOGV("DEBUG(%s[%d]):high resolution preview callback", __FUNCTION__, __LINE__);
        if (m_parameters->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME)) {
            setBit(&m_callbackState, CALLBACK_STATE_PREVIEW_FRAME, false);
            m_dataCb(CAMERA_MSG_PREVIEW_FRAME, previewCallbackHeap, 0, NULL, m_callbackCookie);
            clearBit(&m_callbackState, CALLBACK_STATE_PREVIEW_FRAME, false);
        }

        previewCallbackHeap->release(previewCallbackHeap);

        /* put high resolution callback buffer */
        if (m_parameters->isUseYuvReprocessing() == true)
            ret = m_putBuffers(m_sccReprocessingBufferMgr, highResolutionCbBuffer.index);
        else
            ret = m_putBuffers(m_highResolutionCallbackBufferMgr, highResolutionCbBuffer.index);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_putBuffers fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_gsc, ret);
            goto CLEAN;
        }
    } else {
        CLOGD("DEBUG(%s[%d]): Preview callback message disabled, skip callback", __FUNCTION__, __LINE__);
        /* put SCC buffer */
        if (m_parameters->isUseYuvReprocessing() == true) {
            ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_reprocessingFrameFactory->getNodeType(PIPE_MCSC0_REPROCESSING));
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_scc, ret);
                goto CLEAN;
            }
            ret = m_putBuffers(m_sccReprocessingBufferMgr, sccReprocessingBuffer.index);
        } else if (m_parameters->isUsing3acForIspc() == true) {
            ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_previewFrameFactory->getNodeType(PIPE_3AC));
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_scc, ret);
                goto CLEAN;
            }
            ret = m_putBuffers(m_sccBufferMgr, sccReprocessingBuffer.index);
        } else {
            ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_reprocessingFrameFactory->getNodeType(PIPE_SCC_REPROCESSING));
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_scc, ret);
                goto CLEAN;
            }
            ret = m_putBuffers(m_sccReprocessingBufferMgr, sccReprocessingBuffer.index);
        }
    }

    if (newFrame != NULL) {
        newFrame->frameUnlock();
        ret = m_removeFrameFromList(&m_postProcessList, newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):remove frame from processList fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        CLOGD("DEBUG(%s[%d]): Reprocessing frame delete(%d)", __FUNCTION__, __LINE__, newFrame->getFrameCount());
        newFrame->decRef();
        m_frameMgr->deleteFrame(newFrame);
        newFrame = NULL;
    }

    if(m_flagThreadStop != true) {
        if (m_highResolutionCallbackQ->getSizeOfProcessQ() > 0 ||
            m_parameters->getHighResolutionCallbackMode() == true) {
            CLOGD("DEBUG(%s[%d]):highResolutionCallbackQ size(%d), highResolutionCallbackMode(%s), start again",
                    __FUNCTION__, __LINE__,
                    m_highResolutionCallbackQ->getSizeOfProcessQ(),
                    (m_parameters->getHighResolutionCallbackMode() == true)? "TRUE" : "FALSE");
            loop = true;
        }
    }

    CLOGI("INFO(%s[%d]):high resolution callback thread complete, loop(%d)", __FUNCTION__, __LINE__, loop);

    /* one shot */
    return loop;

CLEAN:
    if (sccReprocessingBuffer.index != -2)
        ret = m_putBuffers(m_sccReprocessingBufferMgr, sccReprocessingBuffer.index);
    if (highResolutionCbBuffer.index != -2)
        m_putBuffers(m_highResolutionCallbackBufferMgr, highResolutionCbBuffer.index);

    if (newFrame != NULL) {
        newFrame->printEntity();

        newFrame->frameUnlock();
        ret = m_removeFrameFromList(&m_postProcessList, newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):remove frame from processList fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        ret = m_deleteFrame(&newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_deleteFrame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }
    }

    if(m_flagThreadStop != true) {
        if (m_highResolutionCallbackQ->getSizeOfProcessQ() > 0 ||
                m_parameters->getHighResolutionCallbackMode() == true) {
            CLOGD("DEBUG(%s[%d]):highResolutionCallbackQ size(%d), highResolutionCallbackMode(%s), start again",
                    __FUNCTION__, __LINE__,
                    m_highResolutionCallbackQ->getSizeOfProcessQ(),
                    (m_parameters->getHighResolutionCallbackMode() == true)? "TRUE" : "FALSE");
            loop = true;
        }
    }

    CLOGI("INFO(%s[%d]):high resolution callback thread fail, loop(%d)", __FUNCTION__, __LINE__, loop);

    /* one shot */
    return loop;
}

status_t ExynosCamera::m_doPrviewToRecordingFunc(
        int32_t pipeId,
        ExynosCameraBuffer previewBuf,
        ExynosCameraBuffer recordingBuf,
        nsecs_t timeStamp)
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif

    CLOGV("DEBUG(%s[%d]):--IN-- (previewBuf.index=%d, recordingBuf.index=%d)",
        __FUNCTION__, __LINE__, previewBuf.index, recordingBuf.index);

    status_t ret = NO_ERROR;
    ExynosRect srcRect, dstRect;
    ExynosCameraFrame  *newFrame = NULL;
    struct camera2_node_output node;

    newFrame = m_previewFrameFactory->createNewFrameVideoOnly();
    if (newFrame == NULL) {
        CLOGE("ERR(%s):newFrame is NULL", __FUNCTION__);
        return UNKNOWN_ERROR;
    }

    /* TODO: HACK: Will be removed, this is driver's job */
    m_convertingStreamToShotExt(&previewBuf, &node);
    setMetaDmSensorTimeStamp((struct camera2_shot_ext*)previewBuf.addr[previewBuf.planeCount-1], timeStamp);

    /* csc and scaling */
    ret = m_calcRecordingGSCRect(&srcRect, &dstRect);
    ret = newFrame->setSrcRect(pipeId, srcRect);
    ret = newFrame->setDstRect(pipeId, dstRect);

    ret = m_setupEntity(pipeId, newFrame, &previewBuf, &recordingBuf);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setupEntity fail, pipeId(%d), ret(%d)",
            __FUNCTION__, __LINE__, pipeId, ret);
        ret = INVALID_OPERATION;
        if (newFrame != NULL) {
            newFrame->decRef();
            m_frameMgr->deleteFrame(newFrame);
            newFrame = NULL;
        }
        goto func_exit;
    }
    m_recordingListLock.lock();
    m_recordingProcessList.push_back(newFrame);
    m_recordingListLock.unlock();
    m_previewFrameFactory->setOutputFrameQToPipe(m_recordingQ, pipeId);

    m_recordingStopLock.lock();
    if (m_getRecordingEnabled() == false) {
        m_recordingStopLock.unlock();
        CLOGD("DEBUG(%s[%d]): m_getRecordingEnabled is false, skip frame(%d) previewBuf(%d) recordingBuf(%d)",
            __FUNCTION__, __LINE__, newFrame->getFrameCount(), previewBuf.index, recordingBuf.index);

        if (newFrame != NULL) {
            newFrame->decRef();
            m_frameMgr->deleteFrame(newFrame);
            newFrame = NULL;
        }

        if (recordingBuf.index >= 0){
            m_putBuffers(m_recordingBufferMgr, recordingBuf.index);
        }
        goto func_exit;
    }
    m_previewFrameFactory->pushFrameToPipe(&newFrame, pipeId);
    m_recordingStopLock.unlock();

func_exit:

    CLOGV("DEBUG(%s[%d]):--OUT--", __FUNCTION__, __LINE__);
    return ret;

}

bool ExynosCamera::m_vraThreadFunc(void)
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif

    status_t ret        = NO_ERROR;
    int gscPipeId       = PIPE_GSC_VRA;
    int vraPipeId       = PIPE_VRA;

    ExynosCameraFrame  *gscFrame = NULL;
    ExynosCameraFrame  *gscDoneFrame = NULL;
    ExynosCameraFrame  *vraFrame = NULL;
    ExynosCameraFrame  *vraDoneFrame = NULL;
    ExynosCameraBuffer srcBuf;
    ExynosCameraBuffer dstBuf;
    int vraWidth = 0, vraHeight = 0;
    int dstBufIndex = -2;
    int frameCount = -1;

    int waitCount = 0;

    struct camera2_stream *streamMeta = NULL;
    uint32_t *mcscOutputCrop = NULL;

    ExynosRect srcRect, dstRect;
    int32_t previewFormat = m_parameters->getHwPreviewFormat();
    m_parameters->getHwVraInputSize(&vraWidth, &vraHeight);

    /* Pop frame from MCSC output Q */
    CLOGV("INFO(%s[%d]):wait MCSC output", __FUNCTION__, __LINE__);

    ret = m_vraThreadQ->waitAndPopProcessQ(&gscFrame);

    if (m_flagThreadStop == true)
        goto func_exit;

    if (ret != NO_ERROR) {
        if (ret == TIMED_OUT) {
            CLOGW("WARN(%s[%d]):wait timeout", __FUNCTION__, __LINE__);
        } else {
            CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
        }
        goto func_exit;
    }

    if (gscFrame == NULL) {
        CLOGE("ERR(%s[%d]):gscFrame is NULL", __FUNCTION__, __LINE__);
        goto func_exit;
    }

    frameCount = gscFrame->getFrameCount();
    CLOGV("INFO(%s[%d]):Get GSC frame for VRA, frameCount(%d)", __FUNCTION__, __LINE__, frameCount);

    m_vraRunningCount++;

    /* Get scaler source buffer */
    ret = gscFrame->getSrcBuffer(gscPipeId, &srcBuf);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):getSrcBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        goto func_exit;
    }

    /* Get scaler destination buffer */
    do {
        waitCount++;

        if (m_vraBufferMgr->getNumOfAvailableBuffer() > 0)
            m_vraBufferMgr->getBuffer(&dstBufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &dstBuf);

        if (dstBufIndex < 0) {
            usleep(WAITING_TIME);

            if (waitCount % 20 == 0) {
                CLOGW("WRN(%s[%d]):retry VRA getBuffer(%d)", __FUNCTION__, __LINE__, dstBufIndex);
                m_scpBufferMgr->dump();
            }
        } else {
            break;
        }
        /* this will retry until 300msec */
    } while (waitCount < (TOTAL_WAITING_TIME / WAITING_TIME) && previewEnabled() == false);

    /* Get size from metadata */
    streamMeta = (struct camera2_stream*)srcBuf.addr[srcBuf.planeCount-1];
    if (streamMeta == NULL) {
        CLOGE("ERR(%s[%d]):srcBuf.addr is NULL, srcBuf.addr(0x%x)",__FUNCTION__, __LINE__, srcBuf.addr[srcBuf.planeCount-1]);
        goto func_exit;
    }

    /* Set size to GSC frame */
    mcscOutputCrop = streamMeta->output_crop_region;

    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.w = mcscOutputCrop[2];
    srcRect.h = mcscOutputCrop[3];
    srcRect.fullW = mcscOutputCrop[2];
    srcRect.fullH = mcscOutputCrop[3];
    srcRect.colorFormat = previewFormat;

    dstRect.x = 0;
    dstRect.y = 0;
    dstRect.w = vraWidth;
    dstRect.h = vraHeight;
    dstRect.fullW = vraWidth;
    dstRect.fullH = vraHeight;
    dstRect.colorFormat = m_parameters->getHwVraInputFormat();

    ret = gscFrame->setSrcRect(gscPipeId, srcRect);
    ret = gscFrame->setDstRect(gscPipeId, dstRect);

    ret = m_setupEntity(gscPipeId, gscFrame, &srcBuf, &dstBuf);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):setupEntity fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, gscPipeId, ret);
    }

    m_previewFrameFactory->setOutputFrameQToPipe(m_vraGscDoneQ, gscPipeId);
    m_previewFrameFactory->pushFrameToPipe(&gscFrame, gscPipeId);

    /* Wait and Pop frame from GSC output Q */
    CLOGV("INFO(%s[%d]):wait GSC output", __FUNCTION__, __LINE__);

    waitCount = 0;
    do {
        ret = m_vraGscDoneQ->waitAndPopProcessQ(&gscDoneFrame);
        waitCount++;

        if (m_flagThreadStop == true) {
            if (m_vraRunningCount > 0)
                m_vraRunningCount--;
            return false;
        }
    } while (ret == TIMED_OUT && waitCount < 10);

    if (ret != NO_ERROR)
        CLOGW("WARN(%s[%d]):GSC wait and pop error, ret(%d)", __FUNCTION__, __LINE__, ret);

    if (gscDoneFrame == NULL) {
        CLOGE("ERR(%s[%d]):gscFrame is NULL", __FUNCTION__, __LINE__);
        goto func_exit;
    }

    CLOGV("INFO(%s[%d]):Get frame from GSC Pipe, frameCount(%d)", __FUNCTION__, __LINE__, frameCount);

    vraFrame = m_previewFrameFactory->createNewFrameOnlyOnePipe(vraPipeId, frameCount);

    if (vraFrame != NULL) {
        /* Set perframe size of VRA */
        camera2_node_group node_group_info;
        memset(&node_group_info, 0x0, sizeof(camera2_node_group));

        node_group_info.leader.request = 1;
        node_group_info.leader.input.cropRegion[0] = 0;
        node_group_info.leader.input.cropRegion[1] = 0;
        node_group_info.leader.input.cropRegion[2] = vraWidth;
        node_group_info.leader.input.cropRegion[3] = vraHeight;
        node_group_info.leader.output.cropRegion[0] = 0;
        node_group_info.leader.output.cropRegion[1] = 0;
        node_group_info.leader.output.cropRegion[2] = node_group_info.leader.input.cropRegion[2];
        node_group_info.leader.output.cropRegion[3] = node_group_info.leader.input.cropRegion[3];

        vraFrame->storeNodeGroupInfo(&node_group_info, PERFRAME_INFO_VRA);

        /* Copy metadata frame to dst buffer */
        struct camera2_shot_ext shot_ext;
        gscDoneFrame->getMetaData(&shot_ext);
        memcpy(dstBuf.addr[dstBuf.planeCount-1], &shot_ext, sizeof(struct camera2_shot_ext));

        /* Set FD bypass from parameters */
        shot_ext.fd_bypass = m_parameters->getFdEnable();
        switch (getCameraId()) {
        case CAMERA_ID_FRONT:
            /* HACK: Calibrate FD orientation */
            shot_ext.shot.uctl.scalerUd.orientation = (m_parameters->getDeviceOrientation() + FRONT_ROTATION + 180) % 360;
            break;
        case CAMERA_ID_BACK:
        default:
            shot_ext.shot.uctl.scalerUd.orientation = m_parameters->getFdOrientation();
            break;
        }

        vraFrame->setMetaData(&shot_ext);

        if (shot_ext.fd_bypass == false) {
            ret = m_setupEntity(vraPipeId, vraFrame, &dstBuf, &dstBuf);
            if (ret != NO_ERROR)
                CLOGE("ERR(%s[%d]):setupEntity fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, vraPipeId, ret);

            m_previewFrameFactory->setOutputFrameQToPipe(m_vraPipeDoneQ, vraPipeId);
            m_previewFrameFactory->pushFrameToPipe(&vraFrame, vraPipeId);

            /* Wait and Pop frame from VRA output Q */
            CLOGV("INFO(%s[%d]):wait VRA output", __FUNCTION__, __LINE__);

            waitCount = 0;
            do {
                ret = m_vraPipeDoneQ->waitAndPopProcessQ(&vraDoneFrame);
                waitCount++;

                if (m_flagThreadStop == true) {
                    if (m_vraRunningCount > 0)
                        m_vraRunningCount--;
                    return false;
                }
            } while (ret == TIMED_OUT && waitCount < 10);

            if (ret != NO_ERROR)
                CLOGW("WARN(%s[%d]):VRA wait and pop error, ret(%d)", __FUNCTION__, __LINE__, ret);

            if (vraDoneFrame == NULL) {
                CLOGE("ERR(%s[%d]):vraFrame is NULL", __FUNCTION__, __LINE__);
                goto func_exit;
            }

    CLOGV("INFO(%s[%d]):Get frame from VRA Pipe, frameCount(%d)", __FUNCTION__, __LINE__, frameCount);

            /* Face detection callback */
            struct camera2_shot_ext fd_shot;
            vraDoneFrame->getDynamicMeta(&fd_shot);

            ExynosCameraFrame *fdFrame = m_frameMgr->createFrame(m_parameters, frameCount);
            if (fdFrame != NULL) {
                fdFrame->storeDynamicMeta(&fd_shot);
                m_facedetectQ->pushProcessQ(&fdFrame);
            }
        }
    }

func_exit:
    /* Put VRA buffer */
    if (dstBuf.index > -1) {
        ret = m_vraBufferMgr->putBuffer(dstBuf.index, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL);
        if (ret != NO_ERROR)
            CLOGW("WARN(%s[%d]):Put VRA buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
    }

    /* Delete frame */
    if (gscFrame != NULL) {
        gscFrame->decRef();
        m_frameMgr->deleteFrame(gscFrame);;
    } else if (gscDoneFrame != NULL) {
        gscDoneFrame->decRef();
        m_frameMgr->deleteFrame(gscDoneFrame);;
    }

    if (vraFrame != NULL) {
        vraFrame->decRef();
        m_frameMgr->deleteFrame(vraFrame);;
    } else if (vraDoneFrame != NULL) {
        vraDoneFrame->decRef();
        m_frameMgr->deleteFrame(vraDoneFrame);;
    }

    if (m_vraRunningCount > 0)
        m_vraRunningCount--;

    if (m_flagThreadStop == true) {
        CLOGI("INFO(%s[%d]):m_flagThreadStop(%d)", __FUNCTION__, __LINE__, m_flagThreadStop);
        return false;
    } else {
        return true;
    }
}

status_t ExynosCamera::m_releaseRecordingBuffer(int bufIndex)
{
    status_t ret = NO_ERROR;

    if (bufIndex < 0 || bufIndex >= (int)m_recordingBufferCount) {
        CLOGE("ERR(%s):Out of Index! (Max: %d, Index: %d)", __FUNCTION__, m_recordingBufferCount, bufIndex);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    m_recordingTimeStamp[bufIndex] = 0L;
    m_recordingBufAvailable[bufIndex] = true;
    ret = m_putBuffers(m_recordingBufferMgr, bufIndex);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):put Buffer fail", __FUNCTION__, __LINE__);
    }

func_exit:

    return ret;
}

status_t ExynosCamera::m_calcPreviewGSCRect(ExynosRect *srcRect, ExynosRect *dstRect)
{
    return m_parameters->calcPreviewGSCRect(srcRect, dstRect);
}

status_t ExynosCamera::m_calcHighResolutionPreviewGSCRect(ExynosRect *srcRect, ExynosRect *dstRect)
{
    return m_parameters->calcHighResolutionPreviewGSCRect(srcRect, dstRect);
}

status_t ExynosCamera::m_calcRecordingGSCRect(ExynosRect *srcRect, ExynosRect *dstRect)
{
    return m_parameters->calcRecordingGSCRect(srcRect, dstRect);
}

status_t ExynosCamera::m_calcPictureRect(ExynosRect *srcRect, ExynosRect *dstRect)
{
    return m_parameters->calcPictureRect(srcRect, dstRect);
}

status_t ExynosCamera::m_calcPictureRect(int originW, int originH, ExynosRect *srcRect, ExynosRect *dstRect)
{
    return m_parameters->calcPictureRect(originW, originH, srcRect, dstRect);
}

status_t ExynosCamera::m_searchFrameFromList(List<ExynosCameraFrame *> *list, uint32_t frameCount, ExynosCameraFrame **frame)
{
    Mutex::Autolock lock(m_searchframeLock);
    int ret = 0;
    ExynosCameraFrame *curFrame = NULL;
    List<ExynosCameraFrame *>::iterator r;

    if (list->empty()) {
        CLOGD("DEBUG(%s[%d]):list is empty", __FUNCTION__, __LINE__);
        return NO_ERROR;
    }

    r = list->begin()++;

    do {
        curFrame = *r;
        if (curFrame == NULL) {
            CLOGE("ERR(%s):curFrame is empty", __FUNCTION__);
            return INVALID_OPERATION;
        }

        if (frameCount == curFrame->getFrameCount()) {
            CLOGV("DEBUG(%s):frame count match: expected(%d)", __FUNCTION__, frameCount);
            *frame = curFrame;
            return NO_ERROR;
        }
        r++;
    } while (r != list->end());

    CLOGV("DEBUG(%s[%d]):Cannot find match frame, frameCount(%d)", __FUNCTION__, __LINE__, frameCount);

    return NO_ERROR;
}

status_t ExynosCamera::m_removeFrameFromList(List<ExynosCameraFrame *> *list, ExynosCameraFrame *frame)
{
    Mutex::Autolock lock(m_searchframeLock);
    int ret = 0;
    ExynosCameraFrame *curFrame = NULL;
    int frameCount = 0;
    int curFrameCount = 0;
    List<ExynosCameraFrame *>::iterator r;

    if (frame == NULL) {
        CLOGE("ERR(%s):frame is NULL", __FUNCTION__);
        return BAD_VALUE;
    }

    if (list->empty()) {
        CLOGD("DEBUG(%s):list is empty", __FUNCTION__);
        return NO_ERROR;
    }

    frameCount = frame->getFrameCount();
    r = list->begin()++;

    do {
        curFrame = *r;
        if (curFrame == NULL) {
            CLOGE("ERR(%s):curFrame is empty", __FUNCTION__);
            return INVALID_OPERATION;
        }

        curFrameCount = curFrame->getFrameCount();
        if (frameCount == curFrameCount) {
            CLOGV("DEBUG(%s):frame count match: expected(%d), current(%d)", __FUNCTION__, frameCount, curFrameCount);
            list->erase(r);
            return NO_ERROR;
        }
        CLOGW("WARN(%s):frame count mismatch: expected(%d), current(%d)", __FUNCTION__, frameCount, curFrameCount);
        /* removed message */
        /* curFrame->printEntity(); */
        r++;
    } while (r != list->end());

    CLOGE("ERR(%s):Cannot find match frame!!!", __FUNCTION__);

    return INVALID_OPERATION;
}

status_t ExynosCamera::m_deleteFrame(ExynosCameraFrame **frame)
{
    status_t ret = NO_ERROR;

    /* put lock using this frame */
    Mutex::Autolock lock(m_searchframeLock);

    if (*frame == NULL) {
        CLOGE("ERR(%s[%d]):frame == NULL. so, fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if ((*frame)->getFrameLockState() == false) {
        if ((*frame)->isComplete() == true) {
            CLOGD("DEBUG(%s[%d]): Reprocessing frame delete(%d)", __FUNCTION__, __LINE__, (*frame)->getFrameCount());

            (*frame)->decRef();
            m_frameMgr->deleteFrame(*frame);
        }
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_clearList(List<ExynosCameraFrame *> *list)
{
    Mutex::Autolock lock(m_searchframeLock);
    int ret = 0;
    ExynosCameraFrame *curFrame = NULL;
    List<ExynosCameraFrame *>::iterator r;

    CLOGD("DEBUG(%s):remaining frame(%zd), we remove them all", __FUNCTION__, list->size());

    while (!list->empty()) {
        r = list->begin()++;
        curFrame = *r;
        if (curFrame != NULL) {
            CLOGV("DEBUG(%s):remove frame count %d", __FUNCTION__, curFrame->getFrameCount() );
            curFrame->decRef();
            m_frameMgr->deleteFrame(curFrame);
            curFrame = NULL;
        }
        list->erase(r);
    }
    CLOGD("DEBUG(%s):EXIT ", __FUNCTION__);

    return NO_ERROR;
}

status_t ExynosCamera::m_clearList(frame_queue_t *queue)
{
    Mutex::Autolock lock(m_searchframeLock);
    int ret = 0;
    ExynosCameraFrame *curFrame = NULL;

    CLOGD("DEBUG(%s):remaining frame(%d), we remove them all", __FUNCTION__, queue->getSizeOfProcessQ());

    while (0 < queue->getSizeOfProcessQ()) {
        queue->popProcessQ(&curFrame);
        if (curFrame != NULL) {
            CLOGV("DEBUG(%s):remove frame count %d", __FUNCTION__, curFrame->getFrameCount() );
            curFrame->decRef();
            m_frameMgr->deleteFrame(curFrame);
            curFrame = NULL;
        }
    }
    CLOGD("DEBUG(%s):EXIT ", __FUNCTION__);

    return NO_ERROR;
}

status_t ExynosCamera::m_clearFrameQ(frame_queue_t *frameQ, uint32_t pipeId, uint32_t dstPipeId, uint32_t direction) {
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraFrameEntity *entity = NULL;
    ExynosCameraBuffer deleteSccBuffer;
    ExynosCameraBufferManager *bufferMgr = NULL;
    int ret = NO_ERROR;

    if (frameQ == NULL) {
        CLOGE("ERR(%s[%d]):frameQ is NULL.", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    CLOGI("INFO(%s[%d]): IN... frameQSize(%d)", __FUNCTION__, __LINE__, frameQ->getSizeOfProcessQ());

    while (0 < frameQ->getSizeOfProcessQ()) {
        newFrame = NULL;
        ret = frameQ->popProcessQ(&newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            continue;
        }

        if (newFrame == NULL) {
            CLOGE("ERR(%s[%d]):newFrame is NULL", __FUNCTION__, __LINE__);
            ret = INVALID_OPERATION;
            continue;
        }

        if (direction == SRC_BUFFER_DIRECTION) {
            ret = newFrame->getSrcBuffer(pipeId, &deleteSccBuffer);
        } else {
            if(m_previewFrameFactory == NULL) {
                return INVALID_OPERATION;
            }
            ret = newFrame->getDstBuffer(pipeId, &deleteSccBuffer, m_previewFrameFactory->getNodeType(dstPipeId));
        }

        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getSrcBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
            continue;
        }

        ret = m_getBufferManager(pipeId, &bufferMgr, direction);
        if (ret < 0)
            CLOGE("ERR(%s[%d]):getBufferManager(SRC) fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);

        /* put SCC buffer */
        CLOGD("DEBUG(%s)(%d):m_putBuffer by clearjpegthread(dstSccRe), index(%d)", __FUNCTION__, __LINE__, deleteSccBuffer.index);
        ret = m_putBuffers(bufferMgr, deleteSccBuffer.index);
        if (ret < 0)
            CLOGE("ERR(%s[%d]):bufferMgr->putBuffers() fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
    }

    return ret;
}

status_t ExynosCamera::m_printFrameList(List<ExynosCameraFrame *> *list)
{
    int ret = 0;
    ExynosCameraFrame *curFrame = NULL;
    List<ExynosCameraFrame *>::iterator r;

    CLOGD("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
    CLOGD("\t remaining frame count(%zd)", list->size());

    r = list->begin()++;

    do {
        curFrame = *r;
        if (curFrame != NULL) {
            CLOGI("\t hal frame count %d", curFrame->getFrameCount() );
            curFrame->printEntity();
        }

        r++;
    } while (r != list->end());
    CLOGD("----------------------------------------------------------------------------");

    return NO_ERROR;
}

status_t ExynosCamera::m_createIonAllocator(ExynosCameraIonAllocator **allocator)
{
    status_t ret = NO_ERROR;
    int retry = 0;
    do {
        retry++;
        CLOGI("INFO(%s[%d]):try(%d) to create IonAllocator", __FUNCTION__, __LINE__, retry);
        *allocator = new ExynosCameraIonAllocator();
        ret = (*allocator)->init(false);
        if (ret < 0)
            CLOGE("ERR(%s[%d]):create IonAllocator fail (retryCount=%d)", __FUNCTION__, __LINE__, retry);
        else {
            CLOGD("DEBUG(%s[%d]):m_createIonAllocator success (allocator=%p)", __FUNCTION__, __LINE__, *allocator);
            break;
        }
    } while (ret < 0 && retry < 3);

    if (ret < 0 && retry >=3) {
        CLOGE("ERR(%s[%d]):create IonAllocator fail (retryCount=%d)", __FUNCTION__, __LINE__, retry);
        ret = INVALID_OPERATION;
    }

    return ret;
}

status_t ExynosCamera::m_createInternalBufferManager(ExynosCameraBufferManager **bufferManager, const char *name)
{
    return m_createBufferManager(bufferManager, name, BUFFER_MANAGER_ION_TYPE);
}

status_t ExynosCamera::m_createBufferManager(
        ExynosCameraBufferManager **bufferManager,
        const char *name,
        buffer_manager_type type)
{
    status_t ret = NO_ERROR;

    if (m_ionAllocator == NULL) {
        ret = m_createIonAllocator(&m_ionAllocator);
        if (ret < 0)
            CLOGE("ERR(%s[%d]):m_createIonAllocator fail", __FUNCTION__, __LINE__);
        else
            CLOGD("DEBUG(%s[%d]):m_createIonAllocator success", __FUNCTION__, __LINE__);
    }

    *bufferManager = ExynosCameraBufferManager::createBufferManager(type);
    (*bufferManager)->create(name, m_cameraId, m_ionAllocator);

    CLOGD("DEBUG(%s):BufferManager(%s) created", __FUNCTION__, name);

    return ret;
}

status_t ExynosCamera::m_setPreviewCallbackBuffer(void)
{
    int ret = 0;
    int previewW = 0, previewH = 0;
    int previewFormat = 0;
    m_parameters->getPreviewSize(&previewW, &previewH);
    previewFormat = m_parameters->getPreviewFormat();

    unsigned int planeSize[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};

    int planeCount  = getYuvPlaneCount(previewFormat);
    int bufferCount = 1;
    exynos_camera_buffer_type_t type = EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE;

    if (m_previewCallbackBufferMgr == NULL) {
        CLOGE("ERR(%s[%d]): m_previewCallbackBufferMgr is NULL", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    if (m_previewCallbackBufferMgr->isAllocated() == true) {
        if (m_parameters->getRestartPreview() == true) {
            CLOGD("DEBUG(%s[%d]): preview size is changed, realloc buffer", __FUNCTION__, __LINE__);
            m_previewCallbackBufferMgr->deinit();
        } else {
            return NO_ERROR;
        }
    }

    ret = getYuvPlaneSize(previewFormat, planeSize, previewW, previewH);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): BAD value, format(%x), size(%dx%d)",
            __FUNCTION__, __LINE__, previewFormat, previewW, previewH);
        return ret;
    }

    ret = m_allocBuffers(m_previewCallbackBufferMgr, planeCount, planeSize, bytesPerLine, bufferCount, bufferCount, type, false, false);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_previewCallbackBufferMgr m_allocBuffers(bufferCount=%d) fail",
            __FUNCTION__, __LINE__, bufferCount);
        return ret;
    }

    return NO_ERROR;
}

bool ExynosCamera::m_startPictureBufferThreadFunc(void)
{
    int ret = 0;

    ret = m_setPictureBuffer();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_setPictureBuffer failed", __FUNCTION__, __LINE__);

        /* TODO: Need release buffers and error exit */

        return false;
    }

    return false;
}

status_t ExynosCamera::m_putBuffers(ExynosCameraBufferManager *bufManager, int bufIndex)
{
    if (bufManager != NULL)
        bufManager->putBuffer(bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_NONE);

    return NO_ERROR;
}

status_t ExynosCamera::m_allocBuffers(
        ExynosCameraBufferManager *bufManager,
        int  planeCount,
        unsigned int *planeSize,
        unsigned int *bytePerLine,
        int  reqBufCount,
        bool createMetaPlane,
        bool needMmap)
{
    int ret = 0;

    ret = m_allocBuffers(
                bufManager,
                planeCount,
                planeSize,
                bytePerLine,
                reqBufCount,
                reqBufCount,
                EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE,
                BUFFER_MANAGER_ALLOCATION_ATONCE,
                createMetaPlane,
                needMmap);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_allocBuffers(reqBufCount=%d) fail",
            __FUNCTION__, __LINE__, reqBufCount);
    }

    return ret;
}

status_t ExynosCamera::m_allocBuffers(
        ExynosCameraBufferManager *bufManager,
        int  planeCount,
        unsigned int *planeSize,
        unsigned int *bytePerLine,
        int  minBufCount,
        int  maxBufCount,
        exynos_camera_buffer_type_t type,
        bool createMetaPlane,
        bool needMmap)
{
    int ret = 0;

    ret = m_allocBuffers(
                bufManager,
                planeCount,
                planeSize,
                bytePerLine,
                minBufCount,
                maxBufCount,
                type,
                BUFFER_MANAGER_ALLOCATION_ONDEMAND,
                createMetaPlane,
                needMmap);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_allocBuffers(minBufCount=%d, maxBufCount=%d, type=%d) fail",
            __FUNCTION__, __LINE__, minBufCount, maxBufCount, type);
    }

    return ret;
}

status_t ExynosCamera::m_allocBuffers(
        ExynosCameraBufferManager *bufManager,
        int  planeCount,
        unsigned int *planeSize,
        unsigned int *bytePerLine,
        int  minBufCount,
        int  maxBufCount,
        exynos_camera_buffer_type_t type,
        buffer_manager_allocation_mode_t allocMode,
        bool createMetaPlane,
        bool needMmap)
{
    int ret = 0;

    CLOGI("INFO(%s[%d]):setInfo(planeCount=%d, minBufCount=%d, maxBufCount=%d, type=%d, allocMode=%d)",
        __FUNCTION__, __LINE__, planeCount, minBufCount, maxBufCount, (int)type, (int)allocMode);

    ret = bufManager->setInfo(
                        planeCount,
                        planeSize,
                        bytePerLine,
                        0,
                        minBufCount,
                        maxBufCount,
                        type,
                        allocMode,
                        createMetaPlane,
                        needMmap);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setInfo fail", __FUNCTION__, __LINE__);
        goto func_exit;
    }

    ret = bufManager->alloc();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):alloc fail", __FUNCTION__, __LINE__);
        goto func_exit;
    }

func_exit:

    return ret;
}

status_t ExynosCamera::m_checkThreadState(int *threadState, int *countRenew)
{
    int ret = NO_ERROR;

    if ((*threadState == ERROR_POLLING_DETECTED) || (*countRenew > ERROR_DQ_BLOCKED_COUNT)) {
        CLOGW("WRN(%s[%d]:SCP DQ Timeout! State:[%d], Duration:%d msec", __FUNCTION__, __LINE__, *threadState, (*countRenew)*(MONITOR_THREAD_INTERVAL/1000));
        ret = false;
    } else {
        CLOGV("[%s] (%d) (%d)", __FUNCTION__, __LINE__, *threadState);
        ret = NO_ERROR;
    }

    return ret;
}

status_t ExynosCamera::m_checkThreadInterval(uint32_t pipeId, uint32_t pipeInterval, int *threadState)
{
    uint64_t *threadInterval;
    int ret = NO_ERROR;

    m_previewFrameFactory->getThreadInterval(&threadInterval, pipeId);
    if (*threadInterval > pipeInterval) {
        CLOGW("WRN(%s[%d]:Pipe(%d) Thread Interval [%lld msec], State:[%d]", __FUNCTION__, __LINE__, pipeId, (*threadInterval)/1000, *threadState);
        ret = false;
    } else {
        CLOGV("Thread IntervalTime [%lld]", *threadInterval);
        CLOGV("Thread Renew state [%d]", *threadState);
        ret = NO_ERROR;
    }

    return ret;
}

#ifdef MONITOR_LOG_SYNC
uint32_t ExynosCamera::m_getSyncLogId(void)
{
    return ++cameraSyncLogId;
}
#endif

status_t ExynosCamera::dump(__unused int fd) const
{
    CLOGI("INFO(%s[%d]):", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

void ExynosCamera::dump()
{
    CLOGI("INFO(%s[%d]):", __FUNCTION__, __LINE__);

    m_printExynosCameraInfo(__FUNCTION__);

    if (m_previewFrameFactory != NULL)
        m_previewFrameFactory->dump();

    if (m_bayerBufferMgr != NULL)
        m_bayerBufferMgr->dump();
    if (m_3aaBufferMgr != NULL)
        m_3aaBufferMgr->dump();
    if (m_ispBufferMgr != NULL)
        m_ispBufferMgr->dump();
    if (m_hwDisBufferMgr != NULL)
        m_hwDisBufferMgr->dump();
    if (m_scpBufferMgr != NULL)
        m_scpBufferMgr->dump();
    if (m_vraBufferMgr != NULL)
        m_vraBufferMgr->dump();

    if (m_ispReprocessingBufferMgr != NULL)
        m_ispReprocessingBufferMgr->dump();
    if (m_sccReprocessingBufferMgr != NULL)
        m_sccReprocessingBufferMgr->dump();
    if (m_sccBufferMgr != NULL)
        m_sccBufferMgr->dump();
    if (m_gscBufferMgr != NULL)
        m_gscBufferMgr->dump();
    if (m_jpegBufferMgr != NULL)
        m_jpegBufferMgr->dump();
    if (m_thumbnailBufferMgr != NULL)
        m_thumbnailBufferMgr->dump();

    return;
}

uint32_t ExynosCamera::m_getBayerPipeId(void)
{
    uint32_t pipeId = 0;

    if (m_parameters->getUsePureBayerReprocessing() == true) {
        pipeId = PIPE_FLITE;
    } else {
        pipeId = PIPE_3AA;
    }
#ifdef DEBUG_RAWDUMP
    pipeId = PIPE_FLITE;
#endif
    return pipeId;
}

void ExynosCamera::m_debugFpsCheck(__unused uint32_t pipeId)
{
#ifdef FPS_CHECK
    uint32_t id = pipeId % DEBUG_MAX_PIPE_NUM;

    m_debugFpsCount[id]++;
    if (m_debugFpsCount[id] == 1) {
        m_debugFpsTimer[id].start();
    }
    if (m_debugFpsCount[id] == 31) {
        m_debugFpsTimer[id].stop();
        long long durationTime = m_debugFpsTimer[id].durationMsecs();
        CLOGI("INFO(%s[%d]): FPS_CHECK(id:%d), duration %lld / 30 = %lld ms. %lld fps",
               __FUNCTION__, __LINE__, pipeId, durationTime, durationTime / 30, 1000 / (durationTime / 30));
        m_debugFpsCount[id] = 0;
    }
#endif
}

status_t ExynosCamera::m_convertingStreamToShotExt(ExynosCameraBuffer *buffer, struct camera2_node_output *outputInfo)
{
/* TODO: HACK: Will be removed, this is driver's job */
    status_t ret = NO_ERROR;
    int bayerFrameCount = 0;
    camera2_shot_ext *shot_ext = NULL;
    camera2_stream *shot_stream = NULL;

    shot_stream = (struct camera2_stream *)buffer->addr[buffer->planeCount-1];
    bayerFrameCount = shot_stream->fcount;
    outputInfo->cropRegion[0] = shot_stream->output_crop_region[0];
    outputInfo->cropRegion[1] = shot_stream->output_crop_region[1];
    outputInfo->cropRegion[2] = shot_stream->output_crop_region[2];
    outputInfo->cropRegion[3] = shot_stream->output_crop_region[3];

    memset(buffer->addr[buffer->planeCount-1], 0x0, sizeof(struct camera2_shot_ext));

    shot_ext = (struct camera2_shot_ext *)buffer->addr[buffer->planeCount-1];
    shot_ext->shot.dm.request.frameCount = bayerFrameCount;

    return ret;
}

status_t ExynosCamera::m_checkBufferAvailable(uint32_t pipeId, ExynosCameraBufferManager *bufferMgr)
{
    status_t ret = TIMED_OUT;
    int retry = 0;

    do {
        ret = -1;
        retry++;
        if (bufferMgr->getNumOfAvailableBuffer() > 0) {
            ret = OK;
        } else {
            /* wait available ISP buffer */
            usleep(WAITING_TIME);
        }
        if (retry % 10 == 0)
            CLOGW("WRAN(%s[%d]):retry(%d) setupEntity for pipeId(%d)", __FUNCTION__, __LINE__, retry, pipeId);
    } while(ret < 0 && retry < (TOTAL_WAITING_TIME/WAITING_TIME) && m_stopBurstShot == false);

    return ret;
}

status_t ExynosCamera::m_boostDynamicCapture(void)
{
    status_t ret = NO_ERROR;
#if 0 /* TODO: need to implementation for bayer */
    uint32_t pipeId = (isOwnScc(getCameraId()) == true) ? PIPE_SCC : PIPE_ISPC;
    uint32_t size = m_processList.size();

    ExynosCameraFrame *curFrame = NULL;
    List<ExynosCameraFrame *>::iterator r;
    camera2_node_group node_group_info_isp;

    if (m_processList.empty()) {
        CLOGD("DEBUG(%s[%d]):m_processList is empty", __FUNCTION__, __LINE__);
        return NO_ERROR;
    }
    CLOGD("DEBUG(%s[%d]):m_processList size(%d)", __FUNCTION__, __LINE__, m_processList.size());
    r = m_processList.end();

    for (unsigned int i = 0; i < 3; i++) {
        r--;
        if (r == m_processList.begin())
            break;

    }

    curFrame = *r;
    if (curFrame == NULL) {
        CLOGE("ERR(%s):curFrame is empty", __FUNCTION__);
        return INVALID_OPERATION;
    }

    if (curFrame->getRequest(pipeId) == true) {
        CLOGD("DEBUG(%s[%d]): Boosting dynamic capture is not need", __FUNCTION__, __LINE__);
        return NO_ERROR;
    }

    CLOGI("INFO(%s[%d]): boosting dynamic capture (frameCount: %d)", __FUNCTION__, __LINE__, curFrame->getFrameCount());
    /* For ISP */
    curFrame->getNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_ISP);
    m_updateBoostDynamicCaptureSize(&node_group_info_isp);
    curFrame->storeNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_ISP);

    curFrame->setRequest(pipeId, true);
    curFrame->setNumRequestPipe(curFrame->getNumRequestPipe() + 1);

    ret = curFrame->setEntityState(pipeId, ENTITY_STATE_REWORK);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setEntityState fail, pipeId(%d), state(%d), ret(%d)",
                __FUNCTION__, __LINE__, pipeId, ENTITY_STATE_REWORK, ret);
        return ret;
    }

    m_previewFrameFactory->pushFrameToPipe(&curFrame, pipeId);
    m_dynamicSccCount++;
    CLOGV("DEBUG(%s[%d]): dynamicSccCount inc(%d) frameCount(%d)", __FUNCTION__, __LINE__, m_dynamicSccCount, curFrame->getFrameCount());
#endif

    return ret;
}

void ExynosCamera::m_updateBoostDynamicCaptureSize(__unused camera2_node_group *node_group_info)
{
#if 0 /* TODO: need to implementation for bayer */
    ExynosRect sensorSize;
    ExynosRect bayerCropSize;

    node_group_info->capture[PERFRAME_BACK_SCC_POS].request = 1;

    m_parameters->getPreviewBayerCropSize(&sensorSize, &bayerCropSize);

    node_group_info->leader.input.cropRegion[0] = bayerCropSize.x;
    node_group_info->leader.input.cropRegion[1] = bayerCropSize.y;
    node_group_info->leader.input.cropRegion[2] = bayerCropSize.w;
    node_group_info->leader.input.cropRegion[3] = bayerCropSize.h;
    node_group_info->leader.output.cropRegion[0] = 0;
    node_group_info->leader.output.cropRegion[1] = 0;
    node_group_info->leader.output.cropRegion[2] = node_group_info->leader.input.cropRegion[2];
    node_group_info->leader.output.cropRegion[3] = node_group_info->leader.input.cropRegion[3];

    /* Capture 0 : SCC - [scaling] */
    node_group_info->capture[PERFRAME_BACK_SCC_POS].input.cropRegion[0] = node_group_info->leader.output.cropRegion[0];
    node_group_info->capture[PERFRAME_BACK_SCC_POS].input.cropRegion[1] = node_group_info->leader.output.cropRegion[1];
    node_group_info->capture[PERFRAME_BACK_SCC_POS].input.cropRegion[2] = node_group_info->leader.output.cropRegion[2];
    node_group_info->capture[PERFRAME_BACK_SCC_POS].input.cropRegion[3] = node_group_info->leader.output.cropRegion[3];

    node_group_info->capture[PERFRAME_BACK_SCC_POS].output.cropRegion[0] = node_group_info->capture[PERFRAME_BACK_SCC_POS].input.cropRegion[0];
    node_group_info->capture[PERFRAME_BACK_SCC_POS].output.cropRegion[1] = node_group_info->capture[PERFRAME_BACK_SCC_POS].input.cropRegion[1];
    node_group_info->capture[PERFRAME_BACK_SCC_POS].output.cropRegion[2] = node_group_info->capture[PERFRAME_BACK_SCC_POS].input.cropRegion[2];
    node_group_info->capture[PERFRAME_BACK_SCC_POS].output.cropRegion[3] = node_group_info->capture[PERFRAME_BACK_SCC_POS].input.cropRegion[3];

    /* Capture 1 : SCP - [scaling] */
    node_group_info->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[0] = node_group_info->leader.output.cropRegion[0];
    node_group_info->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[1] = node_group_info->leader.output.cropRegion[1];
    node_group_info->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[2] = node_group_info->leader.output.cropRegion[2];
    node_group_info->capture[PERFRAME_BACK_SCP_POS].input.cropRegion[3] = node_group_info->leader.output.cropRegion[3];

#endif
    return;
}

void ExynosCamera::m_checkFpsAndUpdatePipeWaitTime(void)
{
    uint32_t curMinFps = 0;
    uint32_t curMaxFps = 0;
    frame_queue_t *inputFrameQ = NULL;

    m_parameters->getPreviewFpsRange(&curMinFps, &curMaxFps);

    if (m_curMinFps != curMinFps) {
        CLOGD("DEBUG(%s[%d]):(%d)(%d)", __FUNCTION__, __LINE__, curMinFps, curMaxFps);

        enum pipeline pipe = (m_parameters->isOwnScc(getCameraId()) == true) ? PIPE_SCC : PIPE_ISPC;

        m_previewFrameFactory->getInputFrameQToPipe(&inputFrameQ, pipe);

        /* 100ms * (30 / 15 fps) = 200ms */
        /* 100ms * (30 / 30 fps) = 100ms */
        /* 100ms * (30 / 10 fps) = 300ms */
        if (inputFrameQ != NULL && curMinFps != 0)
            inputFrameQ->setWaitTime(((100000000 / curMinFps) * 30));
    }

    m_curMinFps = curMinFps;

    return;
}

void ExynosCamera::m_printExynosCameraInfo(const char *funcName)
{
    int w = 0;
    int h = 0;
    ExynosRect srcRect, dstRect;

    CLOGD("DEBUG(%s[%d]):===================================================", __FUNCTION__, __LINE__);
    CLOGD("DEBUG(%s[%d]):============= ExynosCameraInfo call by %s", __FUNCTION__, __LINE__, funcName);
    CLOGD("DEBUG(%s[%d]):===================================================", __FUNCTION__, __LINE__);

    CLOGD("DEBUG(%s[%d]):============= Scenario ============================", __FUNCTION__, __LINE__);
    CLOGD("DEBUG(%s[%d]):= getCameraId                 : %d", __FUNCTION__, __LINE__, m_parameters->getCameraId());
    CLOGD("DEBUG(%s[%d]):= getDualMode                 : %d", __FUNCTION__, __LINE__, m_parameters->getDualMode());
    CLOGD("DEBUG(%s[%d]):= getScalableSensorMode       : %d", __FUNCTION__, __LINE__, m_parameters->getScalableSensorMode());
    CLOGD("DEBUG(%s[%d]):= getRecordingHint            : %d", __FUNCTION__, __LINE__, m_parameters->getRecordingHint());
    CLOGD("DEBUG(%s[%d]):= getEffectRecordingHint      : %d", __FUNCTION__, __LINE__, m_parameters->getEffectRecordingHint());
    CLOGD("DEBUG(%s[%d]):= getDualRecordingHint        : %d", __FUNCTION__, __LINE__, m_parameters->getDualRecordingHint());
    CLOGD("DEBUG(%s[%d]):= getAdaptiveCSCRecording     : %d", __FUNCTION__, __LINE__, m_parameters->getAdaptiveCSCRecording());
    CLOGD("DEBUG(%s[%d]):= doCscRecording              : %d", __FUNCTION__, __LINE__, m_parameters->doCscRecording());
    CLOGD("DEBUG(%s[%d]):= needGSCForCapture           : %d", __FUNCTION__, __LINE__, m_parameters->needGSCForCapture(getCameraId()));
    CLOGD("DEBUG(%s[%d]):= getShotMode                 : %d", __FUNCTION__, __LINE__, m_parameters->getShotMode());
    CLOGD("DEBUG(%s[%d]):= getTpuEnabledMode           : %d", __FUNCTION__, __LINE__, m_parameters->getTpuEnabledMode());
    CLOGD("DEBUG(%s[%d]):= getHWVdisMode               : %d", __FUNCTION__, __LINE__, m_parameters->getHWVdisMode());
    CLOGD("DEBUG(%s[%d]):= get3dnrMode                 : %d", __FUNCTION__, __LINE__, m_parameters->get3dnrMode());

    CLOGD("DEBUG(%s[%d]):============= Internal setting ====================", __FUNCTION__, __LINE__);
    CLOGD("DEBUG(%s[%d]):= isFlite3aaOtf               : %d", __FUNCTION__, __LINE__, m_parameters->isFlite3aaOtf());
    CLOGD("DEBUG(%s[%d]):= is3aaIspOtf                 : %d", __FUNCTION__, __LINE__, m_parameters->is3aaIspOtf());
    CLOGD("DEBUG(%s[%d]):= isReprocessing              : %d", __FUNCTION__, __LINE__, m_parameters->isReprocessing());
    CLOGD("DEBUG(%s[%d]):= isReprocessing3aaIspOTF     : %d", __FUNCTION__, __LINE__, m_parameters->isReprocessing3aaIspOTF());
    CLOGD("DEBUG(%s[%d]):= getUsePureBayerReprocessing : %d", __FUNCTION__, __LINE__, m_parameters->getUsePureBayerReprocessing());

    int reprocessingBayerMode = m_parameters->getReprocessingBayerMode();
    switch(reprocessingBayerMode) {
    case REPROCESSING_BAYER_MODE_NONE:
        CLOGD("DEBUG(%s[%d]):= getReprocessingBayerMode    : REPROCESSING_BAYER_MODE_NONE", __FUNCTION__, __LINE__);
        break;
    case REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON:
        CLOGD("DEBUG(%s[%d]):= getReprocessingBayerMode    : REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON", __FUNCTION__, __LINE__);
        break;
    case REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON:
        CLOGD("DEBUG(%s[%d]):= getReprocessingBayerMode    : REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON", __FUNCTION__, __LINE__);
        break;
    case REPROCESSING_BAYER_MODE_PURE_DYNAMIC:
        CLOGD("DEBUG(%s[%d]):= getReprocessingBayerMode    : REPROCESSING_BAYER_MODE_PURE_DYNAMIC", __FUNCTION__, __LINE__);
        break;
    case REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC:
        CLOGD("DEBUG(%s[%d]):= getReprocessingBayerMode    : REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC", __FUNCTION__, __LINE__);
        break;
    default:
        CLOGD("DEBUG(%s[%d]):= getReprocessingBayerMode    : unexpected mode %d", __FUNCTION__, __LINE__, reprocessingBayerMode);
        break;
    }

    CLOGD("DEBUG(%s[%d]):= isSccCapture                : %d", __FUNCTION__, __LINE__, m_parameters->isSccCapture());

    CLOGD("DEBUG(%s[%d]):============= size setting =======================", __FUNCTION__, __LINE__);
    m_parameters->getMaxSensorSize(&w, &h);
    CLOGD("DEBUG(%s[%d]):= getMaxSensorSize            : %d x %d", __FUNCTION__, __LINE__, w, h);

    m_parameters->getHwSensorSize(&w, &h);
    CLOGD("DEBUG(%s[%d]):= getHwSensorSize             : %d x %d", __FUNCTION__, __LINE__, w, h);

    m_parameters->getBnsSize(&w, &h);
    CLOGD("DEBUG(%s[%d]):= getBnsSize                  : %d x %d", __FUNCTION__, __LINE__, w, h);

    m_parameters->getPreviewBayerCropSize(&srcRect, &dstRect);
    CLOGD("DEBUG(%s[%d]):= getPreviewBayerCropSize     : (%d, %d, %d, %d) -> (%d, %d, %d, %d)", __FUNCTION__, __LINE__,
        srcRect.x, srcRect.y, srcRect.w, srcRect.h,
        dstRect.x, dstRect.y, dstRect.w, dstRect.h);

    m_parameters->getPreviewBdsSize(&dstRect);
    CLOGD("DEBUG(%s[%d]):= getPreviewBdsSize           : (%d, %d, %d, %d)", __FUNCTION__, __LINE__,
        dstRect.x, dstRect.y, dstRect.w, dstRect.h);

    m_parameters->getHwPreviewSize(&w, &h);
    CLOGD("DEBUG(%s[%d]):= getHwPreviewSize            : %d x %d", __FUNCTION__, __LINE__, w, h);

    m_parameters->getPreviewSize(&w, &h);
    CLOGD("DEBUG(%s[%d]):= getPreviewSize              : %d x %d", __FUNCTION__, __LINE__, w, h);

    m_parameters->getPictureBayerCropSize(&srcRect, &dstRect);
    CLOGD("DEBUG(%s[%d]):= getPictureBayerCropSize     : (%d, %d, %d, %d) -> (%d, %d, %d, %d)", __FUNCTION__, __LINE__,
        srcRect.x, srcRect.y, srcRect.w, srcRect.h,
        dstRect.x, dstRect.y, dstRect.w, dstRect.h);

    m_parameters->getPictureBdsSize(&dstRect);
    CLOGD("DEBUG(%s[%d]):= getPictureBdsSize           : (%d, %d, %d, %d)", __FUNCTION__, __LINE__,
        dstRect.x, dstRect.y, dstRect.w, dstRect.h);

    m_parameters->getHwPictureSize(&w, &h);
    CLOGD("DEBUG(%s[%d]):= getHwPictureSize            : %d x %d", __FUNCTION__, __LINE__, w, h);

    m_parameters->getPictureSize(&w, &h);
    CLOGD("DEBUG(%s[%d]):= getPictureSize              : %d x %d", __FUNCTION__, __LINE__, w, h);

    CLOGD("DEBUG(%s[%d]):===================================================", __FUNCTION__, __LINE__);
}

status_t ExynosCamera::m_copyMetaFrameToFrame(ExynosCameraFrame *srcframe, ExynosCameraFrame *dstframe, bool useDm, bool useUdm)
{
    Mutex::Autolock lock(m_metaCopyLock);

    memset(m_tempshot, 0x00, sizeof(struct camera2_shot_ext));
    if(useDm) {
        srcframe->getDynamicMeta(m_tempshot);
        dstframe->storeDynamicMeta(m_tempshot);
    }

    if(useUdm) {
        srcframe->getUserDynamicMeta(m_tempshot);
        dstframe->storeUserDynamicMeta(m_tempshot);
    }

    return NO_ERROR;
}
}; /* namespace android */
