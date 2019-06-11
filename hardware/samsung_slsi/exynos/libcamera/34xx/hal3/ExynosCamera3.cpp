/*
 * Copyright (C) 2014, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCamera3"
#include <cutils/log.h>
#include <ui/Fence.h>

#include "ExynosCamera3.h"

namespace android {

#ifdef MONITOR_LOG_SYNC
uint32_t ExynosCamera3::cameraSyncLogId = 0;
#endif
ExynosCamera3::ExynosCamera3(int cameraId, camera_metadata_t **info):
    m_requestMgr(NULL),
    m_parameters(NULL),
    m_streamManager(NULL),
    m_activityControl(NULL)
{
    BUILD_DATE();
    m_cameraId = cameraId;

    memset(m_name, 0x00, sizeof(m_name));

    /* Initialize pointer variables */
    m_ionAllocator = NULL;

    m_bayerBufferMgr= NULL;
    m_fliteBufferMgr= NULL;
    m_3aaBufferMgr = NULL;
    m_ispBufferMgr = NULL;
    m_yuvCaptureBufferMgr = NULL;
    m_vraBufferMgr = NULL;
    m_gscBufferMgr = NULL;
    m_internalScpBufferMgr = NULL;
    m_ispReprocessingBufferMgr = NULL;
    m_thumbnailBufferMgr = NULL;
    m_skipBufferMgr = NULL;
    m_yuvCaptureReprocessingBufferMgr = NULL;
    m_captureSelector = NULL;
    m_sccCaptureSelector = NULL;
    m_captureZslSelector = NULL;

    /* Create related classes */
    m_parameters = new ExynosCamera3Parameters(m_cameraId);
    m_use_companion = m_parameters->isCompanion(cameraId);
    m_activityControl = m_parameters->getActivityControl();

    ExynosCameraActivityUCTL *uctlMgr = m_activityControl->getUCTLMgr();
    if (uctlMgr != NULL)
        uctlMgr->setDeviceRotation(m_parameters->getFdOrientation());

    m_metadataConverter = new ExynosCamera3MetadataConverter(cameraId, (ExynosCameraParameters *)m_parameters);
    m_requestMgr = new ExynosCameraRequestManager(cameraId, (ExynosCameraParameters *)m_parameters);
    m_requestMgr->setMetaDataConverter(m_metadataConverter);

    /* Create managers */
    m_createManagers();

    /* Create threads */
    m_createThreads();

    /* Create queue for preview path. If you want to control pipeDone in ExynosCamera3, try to create frame queue here */
    m_shotDoneQ = new ExynosCameraList<uint32_t>();
    for (int i = 0; i < MAX_PIPE_NUM; i++) {
        switch (i) {
        case PIPE_FLITE:
        case PIPE_3AA:
        case PIPE_ISP:
        case PIPE_VRA:
            m_pipeFrameDoneQ[i] = new frame_queue_t;
            break;
        default:
            m_pipeFrameDoneQ[i] = NULL;
            break;
        }
    }

    /* Create queue for capture path */
    m_pipeCaptureFrameDoneQ = new frame_queue_t(m_captureStreamThread);
    m_pipeCaptureFrameDoneQ->setWaitTime(2000000000);

    m_duplicateBufferDoneQ = new frame_queue_t;

    /* Create queue for capture path */
    m_reprocessingDoneQ = new frame_queue_t;
    m_reprocessingDoneQ->setWaitTime(2000000000);

    m_frameFactoryQ = new framefactory3_queue_t;
    m_selectBayerQ = new frame_queue_t();
    m_captureQ = new frame_queue_t(m_captureThread);

    /* construct static meta data information */
    if (ExynosCamera3MetadataConverter::constructStaticInfo(cameraId, info))
        CLOGE2("Create static meta data failed!!");

    m_metadataConverter->setStaticInfo(cameraId, *info);

    m_streamManager->setYuvStreamMaxCount(m_parameters->getYuvStreamMaxNum());

    m_setFrameManager();

    m_setConfigInform();

    m_constructFrameFactory();

    /* Setup FrameFactory to RequestManager*/
    m_setupFrameFactoryToRequest();

    /* HACK : check capture stream */
    isCaptureConfig = false;
    isRestarted = false;

    isRecordingConfig = false;
    recordingEnabled = false;
    m_checkConfigStream = false;
    m_flushFlag = false;
    m_factoryStartFlag = false;
    m_flushWaitEnable = false;
    m_frameFactoryStartDone = false;
    m_internalFrameCount = 0;
    m_isNeedInternalFrame = false;
    m_isNeedRequestFrame = false;
    m_currentShot = new struct camera2_shot_ext;
    memset(m_currentShot, 0x00, sizeof(struct camera2_shot_ext));
    m_internalFrameDoneQ = new frame_queue_t(m_internalFrameThread);
    m_captureCount = 0;
#ifdef MONITOR_LOG_SYNC
    m_syncLogDuration = 0;
#endif
    m_flagStartFrameFactory = false;
    m_flagStartReprocessingFrameFactory = false;
    m_flagBayerRequest = false;
    m_prepareFliteCnt = 0;
    m_lastFrametime = 0;
}

status_t  ExynosCamera3::m_setConfigInform() {
    struct ExynosConfigInfo exynosConfig;
    memset((void *)&exynosConfig, 0x00, sizeof(exynosConfig));

    exynosConfig.mode = CONFIG_MODE::NORMAL;

    /* Internal buffers */
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_sensor_buffers = NUM_SENSOR_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_3aa_buffers = NUM_3AA_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_hwdis_buffers = NUM_HW_DIS_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_vra_buffers = NUM_VRA_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_reprocessing_buffers = NUM_REPROCESSING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_fastaestable_buffer = NUM_FASTAESTABLE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.reprocessing_bayer_hold_count = REPROCESSING_BAYER_HOLD_COUNT;
    /* Service buffers */
#if 1 /* Consumer's buffer counts are not fixed */
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_bayer_buffers = VIDEO_MAX_FRAME - NUM_SENSOR_BUFFERS;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_preview_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_preview_cb_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_recording_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_picture_buffers = VIDEO_MAX_FRAME;
#else
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_bayer_buffers = NUM_REQUEST_RAW_BUFFER + NUM_SERVICE_RAW_BUFFER;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_preview_buffers = NUM_REQUEST_PREVIEW_BUFFER + NUM_SERVICE_PREVIEW_BUFFER;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_preview_cb_buffers = NUM_REQUEST_CALLBACK_BUFFER + NUM_SERVICE_CALLBACK_BUFFER;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_recording_buffers = NUM_REQUEST_VIDEO_BUFFER + NUM_SERVICE_VIDEO_BUFFER;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_picture_buffers = NUM_REQUEST_JPEG_BUFFER + NUM_SERVICE_JPEG_BUFFER;
#endif
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_raw_buffers = NUM_REQUEST_RAW_BUFFER;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_preview_buffers = NUM_REQUEST_PREVIEW_BUFFER;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_callback_buffers = NUM_REQUEST_CALLBACK_BUFFER;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_video_buffers = NUM_REQUEST_VIDEO_BUFFER;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_request_jpeg_buffers = NUM_REQUEST_JPEG_BUFFER;
    /* Blocking request */
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_min_block_request = NUM_REQUEST_BLOCK_MIN;
    exynosConfig.info[CONFIG_MODE::NORMAL].bufInfo.num_max_block_request = NUM_REQUEST_BLOCK_MAX;
    /* Prepare buffers */
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_FLITE] = PIPE_FLITE_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::NORMAL].pipeInfo.prepare[PIPE_3AA] = PIPE_3AA_PREPARE_COUNT;

#if (USE_HIGHSPEED_RECORDING)
    /* Config HIGH_SPEED 60 buffer & pipe info */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_sensor_buffers = (getCameraId() == CAMERA_ID_BACK) ? FPS60_NUM_SENSOR_BUFFERS : FPS60_FRONT_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_bayer_buffers = FPS60_NUM_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.init_bayer_buffers = FPS60_NUM_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_3aa_buffers = FPS60_NUM_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_hwdis_buffers = FPS60_NUM_HW_DIS_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_vra_buffers = FPS60_NUM_VRA_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_preview_buffers = FPS60_NUM_PREVIEW_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_picture_buffers = FPS60_NUM_PICTURE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_reprocessing_buffers = FPS60_NUM_REPROCESSING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_recording_buffers = FPS60_NUM_RECORDING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_fastaestable_buffer = FPS60_INITIAL_SKIP_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.reprocessing_bayer_hold_count = FPS60_REPROCESSING_BAYER_HOLD_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.front_num_bayer_buffers = FPS60_FRONT_NUM_BAYER_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.front_num_picture_buffers = FPS60_FRONT_NUM_PICTURE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.preview_buffer_margin = FPS60_NUM_PREVIEW_BUFFERS_MARGIN;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_min_block_request = FPS60_NUM_REQUEST_BLOCK_MIN;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].bufInfo.num_max_block_request = FPS60_NUM_REQUEST_BLOCK_MAX;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].pipeInfo.prepare[PIPE_FLITE] = FPS60_PIPE_FLITE_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].pipeInfo.prepare[PIPE_3AA] = FPS60_PIPE_3AA_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_60].pipeInfo.prepare[PIPE_SCP_REPROCESSING] = FPS60_PIPE_SCP_REPROCESSING_PREPARE_COUNT;

    /* Config HIGH_SPEED 120 buffer & pipe info */

    /* Internal buffers */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_sensor_buffers = FPS120_NUM_SENSOR_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_3aa_buffers = FPS120_NUM_3AA_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_hwdis_buffers = FPS120_NUM_HW_DIS_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_vra_buffers = FPS120_NUM_VRA_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_reprocessing_buffers = FPS120_NUM_REPROCESSING_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_fastaestable_buffer = FPS120_NUM_FASTAESTABLE_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.reprocessing_bayer_hold_count = FPS120_REPROCESSING_BAYER_HOLD_COUNT;

    /* Service buffers */
#if 1
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_bayer_buffers = VIDEO_MAX_FRAME - FPS120_NUM_SENSOR_BUFFERS;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_preview_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_preview_cb_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_recording_buffers = VIDEO_MAX_FRAME;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_picture_buffers = VIDEO_MAX_FRAME;
#else
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_bayer_buffers = FPS120_NUM_REQUEST_RAW_BUFFER + FPS120_NUM_SERVICE_RAW_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_preview_buffers = FPS120_NUM_REQUEST_PREVIEW_BUFFER + FPS120_NUM_SERVICE_PREVIEW_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_preview_cb_buffers = FPS120_NUM_REQUEST_CALLBACK_BUFFER + FPS120_NUM_SERVICE_CALLBACK_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_recording_buffers = FPS120_NUM_REQUEST_VIDEO_BUFFER + FPS120_NUM_SERVICE_VIDEO_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_picture_buffers = FPS120_NUM_REQUEST_JPEG_BUFFER + FPS120_NUM_SERVICE_JPEG_BUFFER;
#endif
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_request_raw_buffers = FPS120_NUM_REQUEST_RAW_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_request_preview_buffers = FPS120_NUM_REQUEST_PREVIEW_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_request_callback_buffers = FPS120_NUM_REQUEST_CALLBACK_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_request_video_buffers = FPS120_NUM_REQUEST_VIDEO_BUFFER;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_request_jpeg_buffers = FPS120_NUM_REQUEST_JPEG_BUFFER;
    /* Blocking request */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_min_block_request = FPS120_NUM_REQUEST_BLOCK_MIN;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].bufInfo.num_max_block_request = FPS120_NUM_REQUEST_BLOCK_MAX;
    /* Prepare buffers */
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].pipeInfo.prepare[PIPE_FLITE] = FPS120_PIPE_FLITE_PREPARE_COUNT;
    exynosConfig.info[CONFIG_MODE::HIGHSPEED_120].pipeInfo.prepare[PIPE_3AA] = FPS120_PIPE_3AA_PREPARE_COUNT;
#endif

    m_parameters->setConfig(&exynosConfig);

    m_exynosconfig = m_parameters->getConfig();

    return NO_ERROR;
}

status_t ExynosCamera3::m_setupFrameFactoryToRequest()
{
    status_t ret = NO_ERROR;
    const char *intentName = NULL;
    const char *streamIDName = NULL;
    ExynosCamera3FrameFactory *factory = NULL;

    factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];
#if defined(ENABLE_FULL_FRAME)
    if (factory != NULL) {
        m_requestMgr->setRequestsInfo(HAL_STREAM_ID_PREVIEW, factory);
        m_requestMgr->setRequestsInfo(HAL_STREAM_ID_VIDEO, factory);
        m_requestMgr->setRequestsInfo(HAL_STREAM_ID_CALLBACK, factory);
        m_requestMgr->setRequestsInfo(HAL_STREAM_ID_RAW, factory);
        m_requestMgr->setRequestsInfo(HAL_STREAM_ID_JPEG, factory);
        m_requestMgr->setRequestsInfo(HAL_STREAM_ID_ZSL_OUTPUT, factory);
    } else {
        CLOGE2("FRAME_FACTORY_TYPE_CAPTURE_PREVIEW factory is NULL!!!!");
    }
#else
    if (factory != NULL) {
        m_requestMgr->setRequestsInfo(HAL_STREAM_ID_PREVIEW, factory);
        m_requestMgr->setRequestsInfo(HAL_STREAM_ID_VIDEO, factory);
        m_requestMgr->setRequestsInfo(HAL_STREAM_ID_CALLBACK, factory);
        m_requestMgr->setRequestsInfo(HAL_STREAM_ID_RAW, factory);
        m_requestMgr->setRequestsInfo(HAL_STREAM_ID_ZSL_OUTPUT, factory);

        /* Set reprocessing frameFactory */
        if (m_parameters->isReprocessing() == true) {
            if (m_frameFactory[FRAME_FACTORY_TYPE_REPROCESSING] != NULL)
                factory = m_frameFactory[FRAME_FACTORY_TYPE_REPROCESSING];
            else
                CLOGE2("FRAME_FACTORY_TYPE_REPROCESSING factory is NULL!!!!");
        }

        m_requestMgr->setRequestsInfo(HAL_STREAM_ID_JPEG, factory);
    } else {
        CLOGE2("FRAME_FACTORY_TYPE_CAPTURE_PREVIEW factory is NULL!!!!");
    }
#endif

    return ret;
}

status_t ExynosCamera3::m_setStreamInfo(camera3_stream_configuration *streamList)
{
    int ret = OK;
    int id = 0;

    CLOGD2("In");

    /* sanity check */
    if (streamList == NULL) {
        CLOGE2("NULL stream configuration");
        return BAD_VALUE;
    }

    if (streamList->streams == NULL) {
        CLOGE2("NULL stream list");
        return BAD_VALUE;
    }

    if (streamList->num_streams < 1) {
        CLOGE2("Bad number of streams requested: %d", streamList->num_streams);
        return BAD_VALUE;
    }

    /* check input stream */
    camera3_stream_t *inputStream = NULL;
    for (size_t i = 0; i < streamList->num_streams; i++) {
        camera3_stream_t *newStream = streamList->streams[i];
        if (newStream == NULL) {
            CLOGE2("Stream index %zu was NULL", i);
            return BAD_VALUE;
        }
        // for debug
        CLOGD2("Stream(%p), ID(%zu), type(%d), usage(%#x) format(%#x) w(%d),h(%d)",
            newStream, i, newStream->stream_type, newStream->usage, newStream->format, newStream->width, newStream->height);

        if ((newStream->stream_type == CAMERA3_STREAM_INPUT) ||
            (newStream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL)) {
            if (inputStream != NULL) {
                CLOGE2("Multiple input streams requested!");
                return BAD_VALUE;
            }
            inputStream = newStream;
        }

        /* HACK : check capture stream */
        if (newStream->format == 0x21)
            isCaptureConfig = true;

        /* HACK : check recording stream */
        if ((newStream->format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
            && (newStream->usage & GRALLOC_USAGE_HW_VIDEO_ENCODER)) {
            CLOGI2("recording stream checked");
            isRecordingConfig = true;
        }

        // TODO: format validation
    }

    /* 1. Invalidate all current streams */
    List<int> keylist;
    List<int>::iterator iter;
    ExynosCameraStream *stream = NULL;
    keylist.clear();
    m_streamManager->getStreamKeys(&keylist);
    for (iter = keylist.begin(); iter != keylist.end(); iter++) {
        m_streamManager->getStream(*iter, &stream);
        stream->setRegisterStream(EXYNOS_STREAM::HAL_STREAM_STS_INVALID);
        stream->setRegisterBuffer(EXYNOS_STREAM::HAL_STREAM_STS_UNREGISTERED);
    }

    /* 2. Remove dead streams */
    keylist.clear();
    stream = NULL;
    id = 0;
    EXYNOS_STREAM::STATE registerStream = EXYNOS_STREAM::HAL_STREAM_STS_INIT;
    m_streamManager->getStreamKeys(&keylist);
    for (iter = keylist.begin(); iter != keylist.end(); iter++) {
        m_streamManager->getStream(*iter, &stream);
        ret = stream->getRegisterStream(&registerStream);
        if (registerStream == EXYNOS_STREAM::HAL_STREAM_STS_INVALID){
            ret = stream->getID(&id);
            if (id < 0) {
                CLOGE2("getID failed id(%d)", id);
                continue;
            }
            m_streamManager->deleteStream(id);
        }
    }

    /* 3. Update stream info */
    for (size_t i = 0; i < streamList->num_streams; i++) {
        stream = NULL;
        camera3_stream_t *newStream = streamList->streams[i];
        if (newStream->priv == NULL) {
            /* new stream case */
            ret = m_enumStreamInfo(newStream);
            if (ret) {
                CLOGE2("Register stream failed %p", newStream);
                return ret;
            }
        } else {
            /* Existing stream, reuse current stream */
            stream = static_cast<ExynosCameraStream*>(newStream->priv);
            stream->setRegisterStream(EXYNOS_STREAM::HAL_STREAM_STS_VALID);
        }
    }

    /* 4. Debug */
    CLOGD2("Out");
    return ret;
}

status_t ExynosCamera3::m_enumStreamInfo(camera3_stream_t *stream)
{
    CLOGD2("In");
    int ret = OK;
    ExynosCameraStream *newStream = NULL;
    int id = 0;
    int actualFormat = 0;
    int planeCount = 0;
    int requestBuffer = 0;
    int outputPortId = 0;
    EXYNOS_STREAM::STATE registerStream;
    EXYNOS_STREAM::STATE registerBuffer;

    registerStream = EXYNOS_STREAM::HAL_STREAM_STS_VALID;
    registerBuffer = EXYNOS_STREAM::HAL_STREAM_STS_UNREGISTERED;

    if (stream == NULL) {
        CLOGE2("stream is NULL.");
        return INVALID_OPERATION;
    }

    /* Update gralloc usage */
    switch (stream->stream_type) {
    case CAMERA3_STREAM_INPUT:
        stream->usage |= GRALLOC_USAGE_HW_CAMERA_READ;
        break;
    case CAMERA3_STREAM_OUTPUT:
        stream->usage |= GRALLOC_USAGE_HW_CAMERA_WRITE;
        break;
    case CAMERA3_STREAM_BIDIRECTIONAL:
        stream->usage |= (GRALLOC_USAGE_HW_CAMERA_READ | GRALLOC_USAGE_HW_CAMERA_WRITE);
        break;
    default:
        CLOGE2("Invalid stream_type %d", stream->stream_type);
        break;
    }

    switch (stream->stream_type) {
    case CAMERA3_STREAM_OUTPUT:
        // TODO: split this routine to function
        switch (stream->format) {
        case HAL_PIXEL_FORMAT_BLOB:
            CLOGD2("HAL_PIXEL_FORMAT_BLOB format(%#x) usage(%#x) stream_type(%#x)", stream->format, stream->usage, stream->stream_type);
            id = HAL_STREAM_ID_JPEG;
            actualFormat = HAL_PIXEL_FORMAT_BLOB;
            planeCount = 1;
            outputPortId = 0;
            requestBuffer = m_exynosconfig->current->bufInfo.num_request_jpeg_buffers;
            break;
        case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
            if (stream->usage & GRALLOC_USAGE_HW_TEXTURE || stream->usage & GRALLOC_USAGE_HW_COMPOSER) {
                CLOGD2("GRALLOC_USAGE_HW_TEXTURE foramt(%#x) usage(%#x) stream_type(%#x)", stream->format, stream->usage, stream->stream_type);
                id = HAL_STREAM_ID_PREVIEW;
                actualFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;
                planeCount = 2;
                outputPortId = m_streamManager->getYuvStreamCount();
                requestBuffer = m_exynosconfig->current->bufInfo.num_request_preview_buffers;
            } else if(stream->usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) {
                CLOGD2("GRALLOC_USAGE_HW_VIDEO_ENCODER format(%#x) usage(%#x) stream_type(%#x)", stream->format, stream->usage, stream->stream_type);
                id = HAL_STREAM_ID_VIDEO;
                actualFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M; /* NV12M */
                planeCount = 2;
                outputPortId = m_streamManager->getYuvStreamCount();
                requestBuffer = m_exynosconfig->current->bufInfo.num_request_video_buffers;
            } else if(stream->usage & GRALLOC_USAGE_HW_CAMERA_ZSL) {
               CLOGD2("GRALLOC_USAGE_HW_CAMERA_ZSL format(%#x) usage(%#x) stream_type(%#x)", stream->format, stream->usage, stream->stream_type);
               id = HAL_STREAM_ID_ZSL_OUTPUT;
               actualFormat = HAL_PIXEL_FORMAT_RAW16;
               planeCount = 1;
               requestBuffer = m_exynosconfig->current->bufInfo.num_request_raw_buffers;
            } else {
                CLOGE2("HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED unknown usage(%#x) format(%#x) stream_type(%#x)", stream->usage, stream->format, stream->stream_type);
                ret = BAD_VALUE;
                goto func_err;
                break;
            }
             break;
        case HAL_PIXEL_FORMAT_YCbCr_420_888:
            CLOGD2("HAL_PIXEL_FORMAT_YCbCr_420_888 format(%#x) usage(%#x) stream_type(%#x)", stream->format, stream->usage, stream->stream_type);
            id = HAL_STREAM_ID_CALLBACK;
            actualFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
            planeCount = 1;
            outputPortId = m_streamManager->getYuvStreamCount();
            requestBuffer = m_exynosconfig->current->bufInfo.num_request_callback_buffers;
            break;
        /* case HAL_PIXEL_FORMAT_RAW_SENSOR: */
        case HAL_PIXEL_FORMAT_RAW16:
            CLOGD2("HAL_PIXEL_FORMAT_RAW_XXX format(%#x) usage(%#x) stream_type(%#x)", stream->format, stream->usage, stream->stream_type);
            id = HAL_STREAM_ID_RAW;
            actualFormat = HAL_PIXEL_FORMAT_RAW16;
            planeCount = 1;
            outputPortId = 0;
            requestBuffer = m_exynosconfig->current->bufInfo.num_request_raw_buffers;
            break;
        default:
            CLOGE2("Not supported image format(%#x) usage(%#x) stream_type(%#x)", stream->format, stream->usage, stream->stream_type);
            ret = BAD_VALUE;
            goto func_err;
            break;
        }
        break;
    case CAMERA3_STREAM_INPUT:
    case CAMERA3_STREAM_BIDIRECTIONAL:
        switch (stream->format) {
        /* case HAL_PIXEL_FORMAT_RAW_SENSOR: */
        case HAL_PIXEL_FORMAT_RAW16:
        case HAL_PIXEL_FORMAT_RAW_OPAQUE:
            CLOGD2("HAL_PIXEL_FORMAT_RAW_XXX format(%#x) usage(%#x) stream_type(%#x)", stream->format, stream->usage, stream->stream_type);
            id = HAL_STREAM_ID_RAW;
            actualFormat = HAL_PIXEL_FORMAT_RAW16;
            planeCount = 1;
            requestBuffer = m_exynosconfig->current->bufInfo.num_request_raw_buffers;
            break;
        case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
            if (stream->usage & GRALLOC_USAGE_HW_CAMERA_ZSL) {
                CLOGD2("GRALLOC_USAGE_HW_CAMERA_ZSL foramt(%#x) usage(%#x) stream_type(%#x)", stream->format, stream->usage, stream->stream_type);
                id = HAL_STREAM_ID_ZSL_INPUT;
                actualFormat = HAL_PIXEL_FORMAT_RAW16;
                planeCount = 1;
                requestBuffer = m_exynosconfig->current->bufInfo.num_request_raw_buffers;
                break;
            } else {
                CLOGE2("HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED unknown usage(%#x) format(%#x) stream_type(%#x)", stream->usage, stream->format, stream->stream_type);
                ret = BAD_VALUE;
                goto func_err;
            }
            break;
        default:
            CLOGE2("Not supported image format(%#x) usage(%#x) stream_type(%#x)", stream->format, stream->usage, stream->stream_type);
            goto func_err;
        }
        break;
    default:
        CLOGE2("Unknown stream_type(%#x) format(%#x) usage(%#x)", stream->stream_type, stream->format, stream->usage);
        ret = BAD_VALUE;
        goto func_err;
    }

    newStream = m_streamManager->createStream(id, stream);
    if (newStream == NULL) {
        CLOGE2("createStream is NULL id(%d)", id);
        goto func_err;
    }

    newStream->setRegisterStream(registerStream);
    newStream->setRegisterBuffer(registerBuffer);
    newStream->setFormat(actualFormat);
    newStream->setPlaneCount(planeCount);
    newStream->setOutputPortId(outputPortId);
    newStream->setRequestBuffer(requestBuffer);

func_err:
    CLOGD2("Out");

    return ret;

}

void ExynosCamera3::m_createManagers(void)
{
    if (!m_streamManager) {
        m_streamManager = new ExynosCameraStreamManager(m_cameraId);
        CLOGD2("Stream Manager created");
    }

    /* Create buffer manager */
    if (!m_fliteBufferMgr)
        m_createInternalBufferManager(&m_fliteBufferMgr, "INTERNAL_BAYER_BUF");
    if (!m_3aaBufferMgr)
        m_createInternalBufferManager(&m_3aaBufferMgr, "3AA_IN_BUF");
    if (!m_ispBufferMgr)
        m_createInternalBufferManager(&m_ispBufferMgr, "ISP_IN_BUF");
    if (!m_yuvCaptureBufferMgr)
        m_createInternalBufferManager(&m_yuvCaptureBufferMgr, "YUV_CAPTURE_IN_BUF");
    if (!m_internalScpBufferMgr)
        m_createInternalBufferManager(&m_internalScpBufferMgr, "INTERNAL_SCP_BUF");
    if (!m_vraBufferMgr)
        m_createInternalBufferManager(&m_vraBufferMgr, "VRA_BUF");
    if (!m_gscBufferMgr)
        m_createInternalBufferManager(&m_gscBufferMgr, "GSC_BUF");
    if (!m_ispReprocessingBufferMgr)
        m_createInternalBufferManager(&m_ispReprocessingBufferMgr, "ISP_RE_BUF");
    if (!m_yuvCaptureReprocessingBufferMgr)
        m_createInternalBufferManager(&m_yuvCaptureReprocessingBufferMgr, "YUV_CAPTURE_RE_BUF");
    if (!m_thumbnailBufferMgr)
        m_createInternalBufferManager(&m_thumbnailBufferMgr, "THUMBNAIL_BUF");

}

void ExynosCamera3::m_createThreads(void)
{
    m_mainThread = new mainCameraThread(this, &ExynosCamera3::m_mainThreadFunc, "m_mainThreadFunc");
    CLOGD2("DEBUG(%s):Main thread created", __FUNCTION__);

    /* m_previewStreamXXXThread is for seperated frameDone each own handler */
    m_previewStreamBayerThread = new mainCameraThread(this, &ExynosCamera3::m_previewStreamBayerPipeThreadFunc, "PreviewBayerThread");
    CLOGD2("Bayer Preview stream thread created");

    m_previewStream3AAThread = new mainCameraThread(this, &ExynosCamera3::m_previewStream3AAPipeThreadFunc, "Preview3AAThread");
    CLOGD2("3AA Preview stream thread created");

    m_previewStreamVRAThread = new mainCameraThread(this, &ExynosCamera3::m_previewStreamVRAPipeThreadFunc, "PreviewVRAThread");
    CLOGD2("VRA Preview stream thread created");

    m_duplicateBufferThread = new mainCameraThread(this, &ExynosCamera3::m_duplicateBufferThreadFunc, "DuplicateThread");
    CLOGD2("Duplicate buffer thread created");

    m_captureStreamThread = new mainCameraThread(this, &ExynosCamera3::m_captureStreamThreadFunc, "CaptureThread");
    CLOGD2("Capture stream thread created");

    m_setBuffersThread = new mainCameraThread(this, &ExynosCamera3::m_setBuffersThreadFunc, "setBuffersThread");
    CLOGD2("Buffer allocation thread created");

    m_framefactoryCreateThread = new mainCameraThread(this, &ExynosCamera3::m_frameFactoryCreateThreadFunc, "FrameFactoryInitThread");
    CLOGD2("FrameFactoryInitThread created");

    m_selectBayerThread = new mainCameraThread(this, &ExynosCamera3::m_selectBayerThreadFunc, "SelectBayerThreadFunc");
    CLOGD2("SelectBayerThread created");

    m_captureThread = new mainCameraThread(this, &ExynosCamera3::m_captureThreadFunc, "m_captureThreadFunc");
    CLOGD2("FrameFactoryInitThread created");

    m_reprocessingFrameFactoryStartThread = new mainCameraThread(this, &ExynosCamera3::m_reprocessingFrameFactoryStartThreadFunc, "m_reprocessingFrameFactoryStartThread");
    CLOGD2("m_reprocessingFrameFactoryStartThread created");

    m_startPictureBufferThread = new mainCameraThread(this, &ExynosCamera3::m_startPictureBufferThreadFunc, "startPictureBufferThread");
    CLOGD2("startPictureBufferThread created");

    m_frameFactoryStartThread = new mainCameraThread(this, &ExynosCamera3::m_frameFactoryStartThreadFunc, "FrameFactoryStartThread");
    CLOGD2("FrameFactoryStartThread created");

    m_internalFrameThread = new mainCameraThread(this, &ExynosCamera3::m_internalFrameThreadFunc, "InternalFrameThread");
    CLOGD2("Internal Frame Handler Thread created");

    m_monitorThread = new mainCameraThread(this, &ExynosCamera3::m_monitorThreadFunc, "MonitorThread");
    CLOGD2("MonitorThread created");
}

ExynosCamera3::~ExynosCamera3()
{
    this->release();
}

void ExynosCamera3::release()
{
    CLOGI2("-IN-");
    int ret = 0;
//    m_mainFrameThread->requestExitAndWait();

    m_releaseBuffers();

    if (m_fliteBufferMgr!= NULL) {
        delete m_fliteBufferMgr;
        m_fliteBufferMgr = NULL;
    }

    if (m_3aaBufferMgr != NULL) {
        delete m_3aaBufferMgr;
        m_3aaBufferMgr = NULL;
    }

    if (m_ispBufferMgr != NULL) {
        delete m_ispBufferMgr;
        m_ispBufferMgr = NULL;
    }

    if (m_yuvCaptureBufferMgr != NULL) {
        delete m_yuvCaptureBufferMgr;
        m_yuvCaptureBufferMgr = NULL;
    }

    if (m_vraBufferMgr != NULL) {
        delete m_vraBufferMgr;
        m_vraBufferMgr = NULL;
    }

    if (m_gscBufferMgr != NULL) {
        delete m_gscBufferMgr;
        m_gscBufferMgr = NULL;
    }

    if (m_yuvCaptureReprocessingBufferMgr != NULL) {
        delete m_yuvCaptureReprocessingBufferMgr;
        m_yuvCaptureReprocessingBufferMgr = NULL;
    }

    if (m_internalScpBufferMgr != NULL) {
        delete m_internalScpBufferMgr;
        m_internalScpBufferMgr = NULL;
    }

    if (m_ispReprocessingBufferMgr != NULL) {
        delete m_ispReprocessingBufferMgr;
        m_ispReprocessingBufferMgr = NULL;
    }

    if (m_thumbnailBufferMgr != NULL) {
        delete m_thumbnailBufferMgr;
        m_thumbnailBufferMgr = NULL;
    }

    if (m_skipBufferMgr != NULL) {
        delete m_skipBufferMgr;
        m_skipBufferMgr = NULL;
    }

    if (m_ionAllocator != NULL) {
        delete m_ionAllocator;
        m_ionAllocator = NULL;
    }

    if (m_shotDoneQ != NULL) {
       delete m_shotDoneQ;
       m_shotDoneQ = NULL;
    }

    for (int i = 0; i < MAX_PIPE_NUM; i++) {
        if (m_pipeFrameDoneQ[i] != NULL) {
            delete m_pipeFrameDoneQ[i];
            m_pipeFrameDoneQ[i] = NULL;
        }
    }

    if (m_duplicateBufferDoneQ != NULL) {
        delete m_duplicateBufferDoneQ;
        m_duplicateBufferDoneQ = NULL;
    }

    /* 4. release queues*/
    if (m_pipeCaptureFrameDoneQ != NULL) {
        delete m_pipeCaptureFrameDoneQ;
        m_pipeCaptureFrameDoneQ = NULL;
    }

    if (m_reprocessingDoneQ != NULL) {
        delete m_reprocessingDoneQ;
        m_reprocessingDoneQ = NULL;
    }

    if (m_internalFrameDoneQ != NULL) {
        delete m_internalFrameDoneQ;
        m_internalFrameDoneQ = NULL;
    }

    if (m_frameFactoryQ != NULL) {
        delete m_frameFactoryQ;
        m_frameFactoryQ = NULL;
    }

    if (m_selectBayerQ != NULL) {
        delete m_selectBayerQ;
        m_selectBayerQ = NULL;
    }

    if (m_captureQ != NULL) {
        delete m_captureQ;
        m_captureQ = NULL;
    }

    if (m_frameMgr != NULL) {
        delete m_frameMgr;
        m_frameMgr = NULL;
    }

    if (m_streamManager != NULL) {
        delete m_streamManager;
        m_streamManager = NULL;
    }

    if (m_requestMgr!= NULL) {
        delete m_requestMgr;
        m_requestMgr = NULL;
    }

    m_deinitFrameFactory();

#if 1
    if (m_parameters != NULL) {
        delete m_parameters;
        m_parameters = NULL;
    }

    if (m_metadataConverter != NULL) {
        delete m_parameters;
        m_parameters = NULL;
    }

    if (m_captureSelector != NULL) {
        delete m_captureSelector;
        m_captureSelector = NULL;
    }

    if (m_captureZslSelector != NULL) {
        delete m_captureZslSelector;
        m_captureZslSelector = NULL;
    }

    if (m_sccCaptureSelector != NULL) {
        delete m_sccCaptureSelector;
        m_sccCaptureSelector = NULL;
    }
#endif

    // TODO: clean up
    // m_resultBufferVectorSet
    // m_processList
    // m_postProcessList
    // m_pipeFrameDoneQ
    CLOGI2("-OUT-");
}

status_t ExynosCamera3::initilizeDevice(const camera3_callback_ops *callbackOps)
{
    status_t ret = NO_ERROR;
    CLOGD2("-IN-");

    /* set callback ops */
    m_requestMgr->setCallbackOps(callbackOps);

    if (m_parameters->isReprocessing() == true) {
        ExynosCameraBufferManager *bufMgr = NULL;
        if (m_parameters->isUseYuvReprocessing() == true
            && m_parameters->isUsing3acForIspc() == true)
            bufMgr = m_yuvCaptureBufferMgr;
        else
            bufMgr = m_fliteBufferMgr;

        if (m_captureSelector == NULL) {
            m_captureSelector = new ExynosCameraFrameSelector(m_parameters, bufMgr, m_frameMgr);
            ret = m_captureSelector->setFrameHoldCount(REPROCESSING_BAYER_HOLD_COUNT);
            if (ret < 0)
                CLOGE2("m_captureSelector setFrameHoldCount(%d) is fail", REPROCESSING_BAYER_HOLD_COUNT);
        }

        if (m_captureZslSelector == NULL) {
            m_captureZslSelector = new ExynosCameraFrameSelector(m_parameters, m_bayerBufferMgr, m_frameMgr);
            ret = m_captureZslSelector->setFrameHoldCount(REPROCESSING_BAYER_HOLD_COUNT);
            if (ret < 0)
                CLOGE2("m_captureZslSelector setFrameHoldCount(%d) is fail", REPROCESSING_BAYER_HOLD_COUNT);
        }
    } else {
        if (m_sccCaptureSelector == NULL) {
            ExynosCameraBufferManager *bufMgr = NULL;

            if (m_parameters->isSccCapture() == true
                || m_parameters->isUsing3acForIspc() == true) {
                /* TODO: Dynamic select buffer manager for capture */
                bufMgr = m_yuvCaptureBufferMgr;
            }

            m_sccCaptureSelector = new ExynosCameraFrameSelector(m_parameters, bufMgr, m_frameMgr);
        }
    }

    m_framefactoryCreateThread->run();
    m_frameMgr->start();

    m_startPictureBufferThread->run();

    /*
         * NOTICE: Join is to avoid dual scanario's problem.
         * The problem is that back camera's EOS was not finished, but front camera opened.
         * Two instance was actually different but driver accepts same instance.
         */
    m_framefactoryCreateThread->join();
    return ret;
}

status_t ExynosCamera3::releaseDevice(void)
{
    status_t ret = NO_ERROR;
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    m_setBuffersThread->requestExitAndWait();
    m_framefactoryCreateThread->requestExitAndWait();
    m_monitorThread->requestExitAndWait();
    m_previewStreamBayerThread->requestExitAndWait();
    m_previewStream3AAThread->requestExitAndWait();
    m_previewStreamVRAThread->requestExitAndWait();
    m_duplicateBufferThread->requestExitAndWait();
    m_internalFrameThread->requestExitAndWait();
    m_mainThread->requestExitAndWait();
    m_selectBayerThread->requestExitAndWait();

    if (m_shotDoneQ != NULL)
        m_shotDoneQ->release();

    if (m_flushFlag == false)
        flush();

    ret = m_clearList(&m_processList, &m_processLock);
    if (ret < 0) {
        CLOGE2("m_clearList fail");
        return ret;
    }

    /* initialize frameQ */
    for (int i = 0; i < MAX_PIPE_NUM; i++) {
        if (m_pipeFrameDoneQ[i] != NULL) {
            m_pipeFrameDoneQ[i]->release();
            m_pipeFrameDoneQ[i] = NULL;
        }
    }
    m_reprocessingDoneQ->release();
    m_pipeCaptureFrameDoneQ->release();
    m_duplicateBufferDoneQ->release();

    /* internal frame */
    m_internalFrameDoneQ->release();

    m_frameMgr->stop();
    m_frameMgr->deleteAllFrame();

    return ret;
}

status_t ExynosCamera3::construct_default_request_settings(camera_metadata_t **request, int type)
{
    Mutex::Autolock l(m_requestLock);
    factory_handler_t frameCreateHandler;
    factory_donehandler_t frameDoneHandler;
    ExynosCamera3FrameFactory *factory = NULL;

    CLOGD2("Type(%d)", type);
    if ((type < 0) || (type >= CAMERA3_TEMPLATE_COUNT)) {
        CLOGE2("Unknown request settings template: %d", type);
        return -ENODEV;
    }

    m_requestMgr->constructDefaultRequestSettings(type, request);

    CLOGI2("-OUT-");
    return OK;
}

status_t ExynosCamera3::configureStreams(camera3_stream_configuration *stream_list)
{
    Mutex::Autolock l(m_requestLock);

    status_t ret = NO_ERROR;
    EXYNOS_STREAM::STATE registerStream = EXYNOS_STREAM::HAL_STREAM_STS_INIT;
    EXYNOS_STREAM::STATE registerbuffer = EXYNOS_STREAM::HAL_STREAM_STS_INIT;
    exynos_camera_buffer_type_t type = EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE;
    buffer_manager_allocation_mode_t allocMode = BUFFER_MANAGER_ALLOCATION_ATONCE;
    int id = 0;
    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    unsigned int planeSize[EXYNOS_CAMERA_BUFFER_MAX_PLANES]    = {0};
    int planeCount, width, height, maxBufferCount, startIndex;
    CameraParameters parameter;
    bool updateSize = true;
    int streamPixelFormat = 0;
    int streamPlaneCount = 0;
    int outputPortId = 0;
    int currentConfigMode = m_parameters->getConfigMode();

    /* prepare flite count init */
    m_prepareFliteCnt = 0;

    CLOGD2("-IN-");

    /* sanity check for stream_list */
    if (stream_list == NULL) {
        CLOGE2("NULL stream configuration");
        return BAD_VALUE;
    }

    if (stream_list->streams == NULL) {
        CLOGE2("NULL stream list");
        return BAD_VALUE;
    }

    if (stream_list->num_streams < 1) {
        CLOGE2("Bad number of streams requested: %d", stream_list->num_streams);
        return BAD_VALUE;
    }

    if (stream_list->operation_mode == CAMERA3_STREAM_CONFIGURATION_CONSTRAINED_HIGH_SPEED_MODE) {
        ALOGI("INFO(%s[%d]):High speed mode is configured. StreamCount %d",
                __FUNCTION__, __LINE__, stream_list->num_streams);
        m_parameters->setConfigMode(CONFIG_MODE::HIGHSPEED_120);
        m_exynosconfig = m_parameters->getConfig();
        m_flagBayerRequest = false;
    } else {
        m_parameters->setConfigMode(CONFIG_MODE::NORMAL);
        m_exynosconfig = m_parameters->getConfig();
        m_flagBayerRequest = false;
    }

    /* start allocation of internal buffers */
    if (m_checkConfigStream == false) {
        m_setBuffersThread->run(PRIORITY_DEFAULT);
    }

    ret = m_streamManager->setConfig(m_exynosconfig);
    if (ret) {
        CLOGE2("setMaxBuffers() failed!!");
        return ret;
    }
    ret = m_setStreamInfo(stream_list);
    if (ret) {
        CLOGE2("setStreams() failed!!");
        return ret;
    }

    /* flush request Mgr */
    m_requestMgr->flush();

    /* HACK :: restart frame factory */
    if (m_checkConfigStream == true ||
            ((isCaptureConfig == true) && (stream_list->num_streams == 1))
        || ((isRecordingConfig == true) && (recordingEnabled == false))
        || ((isRecordingConfig == false) && (recordingEnabled == true))) {
        CLOGI2("restart frame factory isCaptureConfig(%d), isRecordingConfig(%d), stream_list->num_streams(%d)",
            isCaptureConfig, isRecordingConfig, stream_list->num_streams);

        isCaptureConfig = false;
        /* In case of preview with Recording, enter this block even if not restart */
        if (m_checkConfigStream == true)
            isRestarted = true;

        recordingEnabled = false;
        isRecordingConfig = false;

        if (m_flagStartReprocessingFrameFactory == true)
            m_stopReprocessingFrameFactory(m_frameFactory[FRAME_FACTORY_TYPE_REPROCESSING]);

        if (m_flagStartFrameFactory == true) {
            m_stopFrameFactory(m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW]);
            m_removeInternalFrames(&m_processList, &m_processLock);
            m_clearList(&m_captureProcessList, &m_captureProcessLock);
        }

        if (m_parameters->isReprocessing() == true) {
            m_captureSelector->release();
            m_captureZslSelector->release();
        } else {
            m_sccCaptureSelector->release();
        }

        /* restart frame manager */
        m_frameMgr->stop();
        m_frameMgr->deleteAllFrame();
        m_frameMgr->start();

        /* Pull all internal buffers */
        for (int bufIndex = 0; bufIndex < m_fliteBufferMgr->getAllocatedBufferCount(); bufIndex++)
            ret = m_putBuffers(m_fliteBufferMgr, bufIndex);
        for (int bufIndex = 0; bufIndex < m_3aaBufferMgr->getAllocatedBufferCount(); bufIndex++)
            ret = m_putBuffers(m_3aaBufferMgr, bufIndex);
        for (int bufIndex = 0; bufIndex < m_internalScpBufferMgr->getAllocatedBufferCount(); bufIndex++)
            ret = m_putBuffers(m_internalScpBufferMgr, bufIndex);
        for (int bufIndex = 0; bufIndex < m_yuvCaptureBufferMgr->getAllocatedBufferCount(); bufIndex++)
            ret = m_putBuffers(m_yuvCaptureBufferMgr, bufIndex);
        for (int bufIndex = 0; bufIndex < m_vraBufferMgr->getAllocatedBufferCount(); bufIndex++)
            ret = m_putBuffers(m_vraBufferMgr, bufIndex);
        for (int bufIndex = 0; bufIndex < m_gscBufferMgr->getAllocatedBufferCount(); bufIndex++)
            ret = m_putBuffers(m_gscBufferMgr, bufIndex);
        for (int bufIndex = 0; bufIndex < m_ispBufferMgr->getAllocatedBufferCount(); bufIndex++)
            ret = m_putBuffers(m_ispBufferMgr, bufIndex);

        if (currentConfigMode != m_parameters->getConfigMode()) {
            CLOGI("INFO(%s[%d]):ConfigMode is changed. Reallocate the internal buffers. currentConfigMode %d newConfigMode %d",
                    __FUNCTION__, __LINE__,
                    currentConfigMode, m_parameters->getConfigMode());
            ret = m_releaseBuffers();
            if (ret != NO_ERROR)
                CLOGE("ERR(%s[%d]):Failed to releaseBuffers. ret %d",
                        __FUNCTION__, __LINE__, ret);

            m_setBuffersThread->run(PRIORITY_DEFAULT);
        }
    }

    /* clear previous settings */
    ret = m_requestMgr->clearPrevRequest();
    if (ret) {
        CLOGE2("clearPrevRequest() failed!!");
        return ret;
    }

    ret = m_requestMgr->clearPrevShot();
    if (ret < 0) {
        CLOGE2("clearPrevShot() failed!! status(%d)", ret);
    }

    /* check flag update size */
    updateSize = m_streamManager->findStream(HAL_STREAM_ID_PREVIEW);

    m_parameters->setSetfileYuvRange();

    /* Create service buffer manager at each stream */
    for (size_t i = 0; i < stream_list->num_streams; i++) {
        registerStream = EXYNOS_STREAM::HAL_STREAM_STS_INIT;
        registerbuffer = EXYNOS_STREAM::HAL_STREAM_STS_INIT;
        id = -1;

        camera3_stream_t *newStream = stream_list->streams[i];
        ExynosCameraStream *privStreamInfo = static_cast<ExynosCameraStream*>(newStream->priv);
        privStreamInfo->getID(&id);

        CLOGI2("list_index(%zu), streamId(%d)", i, id);
        CLOGD2("stream_type(%d), usage(%x), format(%x), width(%d), height(%d), max_buffers(%d)",
            newStream->stream_type, newStream->usage, newStream->format,
            newStream->width, newStream->height, newStream->max_buffers);

        privStreamInfo->getRegisterBuffer(&registerbuffer);
        privStreamInfo->getPlaneCount(&streamPlaneCount);
        privStreamInfo->getFormat(&streamPixelFormat);
        if (registerbuffer != EXYNOS_STREAM::HAL_STREAM_STS_UNREGISTERED) {
            CLOGE2("privStreamInfo->registerBuffer state error!!");
            return BAD_VALUE;
        }

        width = newStream->width;
        height = newStream->height;

        privStreamInfo->getRegisterStream(&registerStream);
        if (registerStream == EXYNOS_STREAM::HAL_STREAM_STS_INVALID) {
            privStreamInfo->getID(&id);
            CLOGE2("Invalid stream index(%zu) id(%d)", i, id);
            ret = BAD_VALUE;
            break;
        }

        privStreamInfo->getRegisterBuffer(&registerbuffer);

        CLOGD2("streamID(%d) registerStream(%d) registerbuffer(%d)", id, registerStream, registerbuffer);

        if ((registerStream == EXYNOS_STREAM::HAL_STREAM_STS_VALID) &&
            (registerbuffer == EXYNOS_STREAM::HAL_STREAM_STS_UNREGISTERED)) {
            ExynosCameraBufferManager *bufferMgr = NULL;
            switch (id % HAL_STREAM_ID_MAX) {
            case HAL_STREAM_ID_RAW:
                CLOGD2("Create buffer manager(RAW)");
                ret =  m_createServiceBufferManager(&m_bayerBufferMgr, "RAW_STREAM_BUF");
                if (ret < 0) {
                    CLOGE2("m_createBufferManager() failed!!");
                    return ret;
                }

                planeCount = streamPlaneCount + 1;
                planeSize[0] = width * height * 2;
                CLOGD2("planeCount(%d)+1", streamPlaneCount);
                CLOGD2("planeSize[0](%d)", planeSize[0]);
                CLOGD2("bytesPerLine[0](%d)", bytesPerLine[0]);

                /* set startIndex as the next internal buffer index */
                startIndex = m_exynosconfig->current->bufInfo.num_sensor_buffers;
                maxBufferCount = m_exynosconfig->current->bufInfo.num_bayer_buffers;
                CLOGD2("(RAW)- maxBufferCount(%d)", maxBufferCount);

                m_bayerBufferMgr->setAllocator(newStream);
                m_allocBuffers(m_bayerBufferMgr, planeCount, planeSize, bytesPerLine, startIndex, maxBufferCount, true, false);

                privStreamInfo->setRegisterBuffer(EXYNOS_STREAM::HAL_STREAM_STS_REGISTERED);
                privStreamInfo->setBufferManager(m_bayerBufferMgr);
                CLOGD2("m_bayerBufferMgr - %p", m_bayerBufferMgr);
                break;
            case HAL_STREAM_ID_ZSL_OUTPUT:
                CLOGD2("DEBUG(%s[%d]):Create buffer manager(ZSL)", __FUNCTION__, __LINE__);

                planeCount = streamPlaneCount + 1;
                planeSize[0] = width * height * 2;

                CLOGD2("planeCount %d+1", streamPlaneCount);
                CLOGD2("planeSize[0] %d", planeSize[0]);
                CLOGD2("bytesPerLine[0] %d", bytesPerLine[0]);

                /* set startIndex as the next internal buffer index */
                startIndex = NUM_BAYER_BUFFERS;
                maxBufferCount = m_exynosconfig->current->bufInfo.num_bayer_buffers;

                privStreamInfo->setRegisterBuffer(EXYNOS_STREAM::HAL_STREAM_STS_REGISTERED);
                break;
            case HAL_STREAM_ID_ZSL_INPUT:
                planeCount = streamPlaneCount + 1;
                planeSize[0] = width * height * 2;

                ALOGD("DEBUG(%s[%d]):planeCount %d+1",
                        __FUNCTION__, __LINE__, streamPlaneCount);
                ALOGD("DEBUG(%s[%d]):planeSize[0] %d",
                        __FUNCTION__, __LINE__, planeSize[0]);
                ALOGD("DEBUG(%s[%d]):bytesPerLine[0] %d",
                        __FUNCTION__, __LINE__, bytesPerLine[0]);

                privStreamInfo->setRegisterBuffer(EXYNOS_STREAM::HAL_STREAM_STS_REGISTERED);
                break;
            case HAL_STREAM_ID_PREVIEW:

                ret = privStreamInfo->getOutputPortId(&outputPortId);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to getOutputPortId for PREVIEW stream",
                            __FUNCTION__, __LINE__);
                    return ret;
                }

                ret = m_parameters->checkYuvSize(width, height, outputPortId);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to checkYuvSize for PREVIEW stream. size %dx%d outputPortId %d",
                            __FUNCTION__, __LINE__, width, height, outputPortId);
                    return ret;
                }

                ret = m_parameters->checkYuvFormat(streamPixelFormat, outputPortId);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to checkYuvFormat for PREVIEW stream. format %x outputPortId %d",
                            __FUNCTION__, __LINE__, streamPixelFormat, outputPortId);
                    return ret;
                }

                maxBufferCount = m_exynosconfig->current->bufInfo.num_preview_buffers;
                CLOGD2("(PREVIEW)- maxBufferCount(%d)", maxBufferCount);
                ret = m_parameters->setYuvBufferCount(maxBufferCount, outputPortId);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to setYuvBufferCount for PREVIEW stream. maxBufferCount %d outputPortId %d",
                            __FUNCTION__, __LINE__, maxBufferCount, outputPortId);
                    return ret;
                }

                CLOGD2("Create buffer manager(PREVIEW)");
                ret =  m_createServiceBufferManager(&bufferMgr, "PREVIEW_STREAM_BUF");
                if (ret != NO_ERROR) {
                    CLOGE2("m_createBufferManager() failed!!");
                    return ret;
                }

                planeCount = streamPlaneCount + 1;
                planeSize[0] = width * height;
                planeSize[1] = width * height / 2;
                CLOGD2("planeCount(%d)+1", streamPlaneCount);
                CLOGD2("planeSize[0](%d), planeSize[1](%d)", planeSize[0], planeSize[1]);
                CLOGD2("bytesPerLine[0](%d)", bytesPerLine[0]);

                bufferMgr->setAllocator(newStream);
                m_allocBuffers(bufferMgr, planeCount, planeSize, bytesPerLine, maxBufferCount, true, false);

                privStreamInfo->setRegisterBuffer(EXYNOS_STREAM::HAL_STREAM_STS_REGISTERED);
                privStreamInfo->setBufferManager(bufferMgr);
                CLOGD2("previewBufferMgr - %p", bufferMgr);
                break;
            case HAL_STREAM_ID_VIDEO:

                ret = privStreamInfo->getOutputPortId(&outputPortId);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to getOutputPortId for VIDEO stream",
                            __FUNCTION__, __LINE__);
                    return ret;
                }

                ret = m_parameters->checkYuvSize(width, height, outputPortId);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to checkYuvSize for VIDEO stream. size %dx%d outputPortId %d",
                            __FUNCTION__, __LINE__, width, height, outputPortId);
                    return ret;
                }

                ret = m_parameters->checkYuvFormat(streamPixelFormat, outputPortId);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to checkYuvFormat for VIDEO stream. format %x outputPortId %d",
                            __FUNCTION__, __LINE__, streamPixelFormat, outputPortId);
                    return ret;
                }

                maxBufferCount = m_exynosconfig->current->bufInfo.num_recording_buffers;
                CLOGD2("(VIDEO)- maxBufferCount(%d)", maxBufferCount);
                ret = m_parameters->setYuvBufferCount(maxBufferCount, outputPortId);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to setYuvBufferCount for VIDEO stream. maxBufferCount %d outputPortId %d",
                            __FUNCTION__, __LINE__, maxBufferCount, outputPortId);
                    return ret;
                }
                CLOGD2("Create buffer manager(VIDEO)");
                ret =  m_createServiceBufferManager(&bufferMgr, "RECORDING_STREAM_BUF");
                if (ret != NO_ERROR) {
                    CLOGE2("m_createBufferManager() failed!!");
                    return ret;
                }

                planeCount = streamPlaneCount + 1;
                planeSize[0] = width * height;
                planeSize[1] = width * height / 2;

                bufferMgr->setAllocator(newStream);
                m_allocBuffers(bufferMgr, planeCount, planeSize, bytesPerLine, maxBufferCount, false, false);

                privStreamInfo->setRegisterBuffer(EXYNOS_STREAM::HAL_STREAM_STS_REGISTERED);
                privStreamInfo->setBufferManager(bufferMgr);
                CLOGD2("recBufferMgr - %p", bufferMgr);
                break;

            case HAL_STREAM_ID_JPEG:

                ret = m_parameters->checkPictureSize(width, height);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to checkPictureSize for JPEG stream. size %dx%d",
                            __FUNCTION__, __LINE__, width, height);
                    return ret;
                }

                maxBufferCount = m_exynosconfig->current->bufInfo.num_picture_buffers;
                CLOGD2("Create buffer manager(JPEG)");
                ret =  m_createServiceBufferManager(&bufferMgr, "JPEG_STREAM_BUF");
                if (ret < 0) {
                    CLOGE2("m_createBufferManager() failed!!");
                    return ret;
                }

                planeCount = streamPlaneCount;
                planeSize[0] = width * height * 2;

                CLOGD2("planeCount(%d), planeSize[0](%d), bytesPerLine[0](%d)",
                    streamPlaneCount, planeSize[0], bytesPerLine[0]);

                bufferMgr->setAllocator(newStream);
                m_allocBuffers(bufferMgr, planeCount, planeSize, bytesPerLine, maxBufferCount, false, false);

                CLOGD2("JPEG stream size = %d", planeSize[0]);
                privStreamInfo->setRegisterBuffer(EXYNOS_STREAM::HAL_STREAM_STS_REGISTERED);
                privStreamInfo->setBufferManager(bufferMgr);
                CLOGD2("JpegBufferMgr - %p", bufferMgr);

                break;
            case HAL_STREAM_ID_CALLBACK:

                ret = privStreamInfo->getOutputPortId(&outputPortId);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to getOutputPortId for CALLBACK stream",
                            __FUNCTION__, __LINE__);
                    return ret;
                }

                ret = m_parameters->checkYuvSize(width, height, outputPortId);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to checkYuvSize for CALLBACK stream. size %dx%d outputPortId %d",
                            __FUNCTION__, __LINE__, width, height, outputPortId);
                    return ret;
                }

                ret = m_parameters->checkYuvFormat(streamPixelFormat, outputPortId);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to checkYuvFormat for CALLBACK stream. format %x outputPortId %d",
                            __FUNCTION__, __LINE__, streamPixelFormat, outputPortId);
                    return ret;
                }

                maxBufferCount = m_exynosconfig->current->bufInfo.num_preview_cb_buffers;
                ret = m_parameters->setYuvBufferCount(maxBufferCount, outputPortId);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to setYuvBufferCount for CALLBACK stream. maxBufferCount %d outputPortId %d",
                            __FUNCTION__, __LINE__, maxBufferCount, outputPortId);
                    return ret;
                }
                CLOGD2("Create buffer manager(PREVIEW_CB)");

                ret =  m_createServiceBufferManager(&bufferMgr, "PREVIEW_CB_STREAM_BUF");
                if (ret < 0) {
                    CLOGE2("m_createBufferManager() failed!!");
                    return ret;
                }

                planeCount = streamPlaneCount + 1;
                planeSize[0] = (width * height * 3) / 2;

                CLOGD2("planeCount %d", streamPlaneCount);
                CLOGD2("planeSize[0] %d",planeSize[0]);
                CLOGD2("bytesPerLine[0] %d",bytesPerLine[0]);

                bufferMgr->setAllocator(newStream);
                m_allocBuffers(bufferMgr, planeCount, planeSize, bytesPerLine, maxBufferCount, true, false);
                privStreamInfo->setRegisterBuffer(EXYNOS_STREAM::HAL_STREAM_STS_REGISTERED);
                privStreamInfo->setBufferManager(bufferMgr);
                CLOGD2("preivewCallbackBufferMgr - %p", bufferMgr);

                break;

            default:
                CLOGE2("privStreamInfo->id is invalid !! id(%d)", id);
                ret = BAD_VALUE;
                break;
            }
        }
    }

    /* Do pure bayer always reprocessing */
    m_checkConfigStream = true;

    CLOGD2("-OUT-");
    return ret;
}

status_t ExynosCamera3::registerStreamBuffers(const camera3_stream_buffer_set_t *buffer_set)
{
    /* deprecated function */
    if (buffer_set == NULL) {
        CLOGE2("buffer_set is NULL");
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t ExynosCamera3::processCaptureRequest(camera3_capture_request *request)
{
    Mutex::Autolock l(m_requestLock);
    ExynosCameraBuffer *buffer = NULL;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer dstBuf;
    ExynosCameraStream *streamInfo = NULL;
    uint32_t timeOutNs = 60 * 1000000; /* timeout default value is 60ms based on 15fps */
    uint32_t waitMaxBlockCnt = 0;
    status_t ret = NO_ERROR;
    camera3_stream_t *stream = NULL;
    EXYNOS_STREAM::STATE registerBuffer = EXYNOS_STREAM::HAL_STREAM_STS_UNREGISTERED;
    uint32_t minBlockReqCount = 0;
    uint32_t maxBlockReqCount = 0;
    CameraMetadata meta;
    camera_metadata_entry_t entry;

    CLOGV2("Capture request (%d) #out(%d)", request->frame_number, request->num_output_buffers);

    /* 1. Wait for allocation completion of internal buffers and creation of frame factory */
    m_setBuffersThread->join();
    m_framefactoryCreateThread->join();

    /* 2. Check the validation of request */
    if (request == NULL) {
        ALOGE("ERR(%s[%d]):NULL request!", __FUNCTION__, __LINE__);
        ret = BAD_VALUE;
        goto req_err;
    }

    /* m_streamManager->dumpCurrentStreamList(); */

    /* 3. Check NULL for service metadata */
    if ((request->settings == NULL) && (m_requestMgr->isPrevRequest())) {
        CLOGE2("Request%d: NULL and no prev request!!", request->frame_number);
        ret = BAD_VALUE;
        goto req_err;
    }

    /* 4. Check the registeration of input buffer on stream */
    if (request->input_buffer != NULL){
        stream = request->input_buffer->stream;
        streamInfo = static_cast<ExynosCameraStream*>(stream->priv);
        streamInfo->getRegisterBuffer(&registerBuffer);

        if (registerBuffer != EXYNOS_STREAM::HAL_STREAM_STS_REGISTERED) {
            CLOGE2("Request %d: Input buffer not from input stream!", request->frame_number);
            CLOGE2("Bad Request %p, type(%d), format(%x)", request->input_buffer->stream,
                request->input_buffer->stream->stream_type, request->input_buffer->stream->format);
            ret = BAD_VALUE;
            goto req_err;
        }
    }

    /* 5. Check the output buffer count */
    if ((request->num_output_buffers < 1) || (request->output_buffers == NULL)) {
        CLOGE2("Request %d: No output buffers provided!", request->frame_number);
        ret = BAD_VALUE;
        goto req_err;
    }

    CLOGV2("request->num_output_buffers(%d) frame_number(%d)",
        request->num_output_buffers, request->frame_number);

    /* 6. Store request settings
     * Caution : All information must be copied into internal data structure
     * before receiving another request from service
     */
    ret = m_pushRequest(request);
    ret = m_registerStreamBuffers(request);

    /* 7. Calculate the timeout value for processing request based on actual fps setting */
    meta = request->settings;
    if (request->settings != NULL && meta.exists(ANDROID_CONTROL_AE_TARGET_FPS_RANGE)) {
        uint32_t minFps = 0, maxFps = 0;
        entry = meta.find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);
        minFps = entry.data.i32[0];
        maxFps = entry.data.i32[1];
        m_parameters->checkPreviewFpsRange(minFps, maxFps);
        timeOutNs = (1000 / ((minFps == 0) ? 15 : minFps)) * 1000000;
    }

    /* 8. Process initial requests for preparing the stream */
    if (request->frame_number == 0 || m_flushFlag == true || isRestarted == true) {
        isRestarted = false;
        m_flushFlag = false;
        m_prepareFliteCnt = m_exynosconfig->current->pipeInfo.prepare[PIPE_FLITE];
        m_factoryStartFlag = true;

        ALOGV("DEBUG(%s[%d]):Start FrameFactory requestKey(%d) m_flushFlag(%d/%d)",
                __FUNCTION__, __LINE__, request->frame_number, isRestarted, m_flushFlag);
        m_frameFactoryStartDone = false;
        m_frameFactoryStartThread->run();

        if (m_parameters->isReprocessing() == true &&
            m_flagStartReprocessingFrameFactory == false) {
            m_frameFactoryStartThread->join();
            m_reprocessingFrameFactoryStartThread->run();
        }
    }

    m_flushWaitEnable = true;

    minBlockReqCount = m_exynosconfig->current->bufInfo.num_min_block_request;
    maxBlockReqCount = m_exynosconfig->current->bufInfo.num_max_block_request;
    waitMaxBlockCnt = minBlockReqCount * 10;

    /*
     * Blocked this func if request counts(in HAL) is over than MIN request count.
     * So we keeps MIN request counts in HAL.
     * If HAL received too many requests, there are some late reactive problem in case of stopping, appling effects .. etc.
     * MAX request count is not used now. But it will be used in case that HAL want to receive request as many as MAX request count
     */
    while (m_requestMgr->getRequestCount() > minBlockReqCount && m_flushFlag == false && waitMaxBlockCnt > 0) {
        if (m_frameFactoryStartDone == false) {
            m_frameFactoryStartThread->join();
        }
        if (m_parameters->isReprocessing() == true)
            m_reprocessingFrameFactoryStartThread->join();
        status_t waitRet = NO_ERROR;
        m_captureResultDoneLock.lock();
        waitRet = m_captureResultDoneCondition.waitRelative(m_captureResultDoneLock, timeOutNs);
        if (waitRet == TIMED_OUT)
            CLOGV2("time out (m_processList:%zu / totalRequestCnt:%d / "
                "blockReqCount = min:%u, max:%u / waitcnt:%u)",
                m_processList.size(), m_requestMgr->getRequestCount(),
                minBlockReqCount, maxBlockReqCount, waitMaxBlockCnt);

        m_captureResultDoneLock.unlock();
        waitMaxBlockCnt--;
    }

req_err:
    return ret;
}

void ExynosCamera3::get_metadata_vendor_tag_ops(const camera3_device_t *, vendor_tag_query_ops_t *ops)
{
    ALOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (ops == NULL) {
        CLOGE2("ops is NULL");
        return;
    }
}

status_t ExynosCamera3::flush()
{
    camera3_stream_buffer_t streamBuffer;
    CameraMetadata result;
    ExynosCamera3FrameFactory *frameFactory = NULL;
    ExynosCameraRequest* request = NULL;
    ResultRequest resultRequest = NULL;
    ExynosCameraStream *stream = NULL;
    ExynosCameraBuffer buffer;
    List<ExynosCameraFrame *> *list = &m_processList;
    ExynosCameraFrame *curFrame = NULL;
    List<ExynosCameraFrame *>::iterator r;
    List<request_info_t *>::iterator requestInfoR;
    request_info_t *requestInfo = NULL;
    int bufferIndex;
    int requestIndex = -1;
    status_t ret = NO_ERROR;
    ExynosCameraBufferManager *bufferMgr = NULL;

    /*
     * This flag should be set before stoping all pipes,
     * because other func(like processCaptureRequest) must know call state about flush() entry level
     */
    m_flushFlag = true;
    m_captureResultDoneCondition.signal();

    List<int> *outputStreamId;
    List<int>::iterator outputStreamIdIter;

    int streamId = 0;

    Mutex::Autolock l(m_resultLock);
    CLOGD2("IN+++", __FUNCTION__, __LINE__);
    CLOGV2("ProcessListCount(%d)", list->size());

    if (m_flushWaitEnable == false) {
        CLOGD2("No need to wait & flush");
        goto func_exit;
    }

    /* Wait for finishing frameFactoryStartThread */
    m_frameFactoryStartThread->requestExitAndWait();
    m_internalFrameThread->requestExit();
    m_mainThread->requestExitAndWait();

    m_captureThread->requestExit();
    if (m_sccCaptureSelector != NULL)
        m_sccCaptureSelector->wakeselectDynamicFrames();
    m_captureThread->requestExitAndWait();

    /* Create frame for the waiting request */
    while (m_requestWaitingList.size() > 0) {
        requestInfoR = m_requestWaitingList.begin();
        requestInfo = *requestInfoR;
        request = requestInfo->request;

        m_createRequestFrameFunc(request);

        m_requestWaitingList.erase(requestInfoR);
    }

    /* Stop pipeline */
    for (int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++) {
        if (m_frameFactory[i] != NULL) {
            frameFactory = m_frameFactory[i];

            for (int k = i + 1; k < FRAME_FACTORY_TYPE_MAX; k++) {
               if (frameFactory == m_frameFactory[k]) {
                   CLOGD2("m_frameFactory index(%d) and index(%d) are same instance, set index(%d) = NULL", i, k, k);
                   m_frameFactory[k] = NULL;
               }
            }

            ret = m_stopFrameFactory(m_frameFactory[i]);
            if (ret < 0)
               CLOGE2("m_frameFactory[%d] stopPipes fail", i);

            CLOGD2("m_frameFactory[%d] stopPipes", i);
        }
    }

    m_flagStartReprocessingFrameFactory = false;

    if (m_captureSelector != NULL)
        m_captureSelector->release();
    if (m_sccCaptureSelector != NULL)
        m_sccCaptureSelector->release();

    /* Wait until duplicateBufferThread stop */
    m_duplicateBufferThread->requestExitAndWait();

    /* Wait until previewStream3AA/ISPThread stop */
    m_previewStreamVRAThread->requestExitAndWait();
    m_previewStream3AAThread->requestExitAndWait();
    m_previewStreamBayerThread->requestExitAndWait();

    /* Check queued requests from camera service */
    CLOGV2("ProcessListCount(%d)", list->size());

    do {
        Mutex::Autolock l(m_processLock);
        while (!list->empty()) {
            r = list->begin()++;
            curFrame = *r;
            if (curFrame == NULL) {
                CLOGE2("curFrame is empty");
                break;
            }

            if (curFrame->getFrameType() == FRAME_TYPE_INTERNAL) {
                m_releaseInternalFrame(curFrame);
                list->erase(r);
                curFrame = NULL;

                continue;
            }

            request = m_requestMgr->getServiceRequest(curFrame->getFrameCount());

            if (request == NULL) {
                CLOGE2("request is empty");
                list->erase(r);
                curFrame->decRef();
                m_frameMgr->deleteFrame(curFrame);
                curFrame = NULL;
                continue;
            }
            CLOGV2("framecount(%d)", curFrame->getFrameCount());

            /* handle notify */
            camera3_notify_msg_t notify;
            uint64_t timeStamp = 0L;
            timeStamp = request->getSensorTimestamp();
            if (timeStamp == 0L)
                timeStamp = m_lastFrametime + 15000000; /* set dummy frame time */
            notify.type = CAMERA3_MSG_SHUTTER;
            notify.message.shutter.frame_number = request->getKey();
            notify.message.shutter.timestamp = timeStamp;
            resultRequest = m_requestMgr->createResultRequest(curFrame->getFrameCount(), EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY, NULL, &notify);
            m_requestMgr->callbackSequencerLock();
            m_requestMgr->callbackRequest(resultRequest);
            m_requestMgr->callbackSequencerUnlock();

            request = m_requestMgr->getServiceRequest(curFrame->getFrameCount());
            if (request == NULL) {
                CLOGW("WARN(%s[%d]):request is empty", __FUNCTION__, __LINE__);
                list->erase(r);
                curFrame->decRef();
                m_frameMgr->deleteFrame(curFrame);
                curFrame = NULL;
                continue;
            }

            CLOGV2("framecount(%d)", curFrame->getFrameCount());
            result = request->getResultMeta();
            result.update(ANDROID_SENSOR_TIMESTAMP, (int64_t *)&timeStamp, 1);
            request->setResultMeta(result);

            request->getAllRequestOutputStreams(&outputStreamId);
            CLOGI2("outputStreamID->size(%d)", outputStreamId->size());

            if (outputStreamId->size() > 0) {
                outputStreamIdIter = outputStreamId->begin();
                bufferIndex = 0;
                CLOGI("INFO(%s[%d]):outputStreamId->size(%zu)",
                        __FUNCTION__, __LINE__, outputStreamId->size());
                for (int i = outputStreamId->size(); i > 0; i--) {
                    CLOGI("INFO(%s[%d]):i(%d) *outputStreamIdIter(%d)",
                            __FUNCTION__, __LINE__, i, *outputStreamIdIter);
                    if (*outputStreamIdIter < 0)
                        break;

                    m_streamManager->getStream(*outputStreamIdIter, &stream);

                    if (stream == NULL) {
                        CLOGE2("stream is NULL");
                        ret = INVALID_OPERATION;
                        return ret;
                    }

                    stream->getID(&streamId);

                    ret = stream->getStream(&streamBuffer.stream);
                    if (ret < 0) {
                        CLOGE2("getStream is failed, from exynoscamerastream. Id error:HAL_STREAM_ID_PREVIEW");
                        return ret;
                    }

                    requestIndex = -1;
                    stream->getBufferManager(&bufferMgr);
                    ret = bufferMgr->getBuffer(&requestIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &buffer);
                    if (ret != NO_ERROR) {
                        CLOGE2("Buffer manager getBuffer fail, frameCount(%d), ret(%d)", curFrame->getFrameCount(), ret);
                    }

                    ret = bufferMgr->getHandleByIndex(&streamBuffer.buffer, buffer.index);
                    if (ret != OK) {
                        CLOGE2("Buffer index error(%d)!!", buffer.index);
                        return ret;
                    }

                    /* handle stream buffers */
                    streamBuffer.status = CAMERA3_BUFFER_STATUS_OK;
                    streamBuffer.acquire_fence = -1;
                    streamBuffer.release_fence = -1;

                    resultRequest = m_requestMgr->createResultRequest(curFrame->getFrameCount(), EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY, NULL, NULL);
                    resultRequest->pushStreamBuffer(&streamBuffer);
                    m_requestMgr->callbackSequencerLock();
                    request->increaseCompleteBufferCount();
                    m_requestMgr->callbackRequest(resultRequest);
                    m_requestMgr->callbackSequencerUnlock();

                    bufferIndex++;
                    outputStreamIdIter++;

                }

            }

            CLOGV2("frameCount(%d), request->getNumOfOutputBuffer(%d), result num_output_buffers(%d)",
                    request->getFrameCount(), request->getNumOfOutputBuffer(), bufferIndex);

            /*  frame to complete callback should be removed */
            list->erase(r);

            curFrame->decRef();

            m_frameMgr->deleteFrame(curFrame);

            curFrame = NULL;
        }
    } while(0);

     /* Pull all internal buffers */
    for (int bufIndex = 0; bufIndex < m_fliteBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_fliteBufferMgr, bufIndex);
    for (int bufIndex = 0; bufIndex < m_3aaBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_3aaBufferMgr, bufIndex);
    for (int bufIndex = 0; bufIndex < m_internalScpBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_internalScpBufferMgr, bufIndex);
    for (int bufIndex = 0; bufIndex < m_yuvCaptureBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_yuvCaptureBufferMgr, bufIndex);
    for (int bufIndex = 0; bufIndex < m_vraBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_vraBufferMgr, bufIndex);
    for (int bufIndex = 0; bufIndex < m_gscBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_gscBufferMgr, bufIndex);
    for (int bufIndex = 0; bufIndex < m_ispBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_ispBufferMgr, bufIndex);

func_exit:
    CLOGD2("-OUT-");
    return ret;
}

void ExynosCamera3::dump()
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
}

int ExynosCamera3::getCameraId() const
{
    return m_cameraId;
}

bool ExynosCamera3::m_mainThreadFunc(void)
{
    ExynosCameraRequest *request = NULL;
    uint32_t frameCount = 0;
    status_t ret = NO_ERROR;

    /* 1. Wait the shot done with the latest framecount */
    ret = m_shotDoneQ->waitAndPopProcessQ(&frameCount);
    if (ret < 0) {
        if (ret == TIMED_OUT)
            CLOGW("WARN(%s[%d]):wait timeout", __FUNCTION__, __LINE__);
        else
            CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return true;
    }

    if (isRestarted == true) {
        CLOGI("INFO(%s[%d]):wait configure stream", __FUNCTION__, __LINE__);
        usleep(1);

        return true;
    }

    ret = m_createFrameFunc();
    if (ret != NO_ERROR)
        CLOGE("ERR(%s[%d]):Failed to createFrameFunc. Shotdone framecount %d",
                __FUNCTION__, __LINE__, frameCount);

    return true;
}

void ExynosCamera3::m_updateCurrentShot(void)
{
    List<request_info_t *>::iterator r;
    request_info_t *requestInfo = NULL;
    ExynosCameraRequest *request = NULL;
    struct camera2_shot_ext temp_shot_ext;
    status_t ret = NO_ERROR;
    int controlInterval = 0;

    /* 1. Get the request info from the back of the list (the newest request) */
    r = m_requestWaitingList.end();
    r--;
    requestInfo = *r;
    request = requestInfo->request;

    /* 2. Get the metadata from request */
    ret = request->getServiceShot(&temp_shot_ext);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Get service shot fail, requestKey(%d), ret(%d)",
                __FUNCTION__, __LINE__, request->getKey(), ret);
    }

    /* 3. Store the frameCount that the sensor control is going to be delivered */
    if (requestInfo->sensorControledFrameCount == 0)
        requestInfo->sensorControledFrameCount = m_internalFrameCount;

    /* 4. Get the request info from the front of the list (the oldest request) */
    r = m_requestWaitingList.begin();
    requestInfo = *r;
    request = requestInfo->request;

    /* 5. Update the entire shot_ext structure */
    ret = request->getServiceShot(m_currentShot);
    if (ret != NO_ERROR) {
        CLOGV("ERR(%s[%d]):Get service shot fail, requestKey(%d), ret(%d)",
                __FUNCTION__, __LINE__, request->getKey(), ret);
    }

    /* 6. Overwrite the sensor control metadata to m_currentShot */
    m_currentShot->shot.ctl.sensor.exposureTime = temp_shot_ext.shot.ctl.sensor.exposureTime;
    m_currentShot->shot.ctl.sensor.frameDuration = temp_shot_ext.shot.ctl.sensor.frameDuration;
    m_currentShot->shot.ctl.sensor.sensitivity = temp_shot_ext.shot.ctl.sensor.sensitivity;
    m_currentShot->shot.ctl.lens = temp_shot_ext.shot.ctl.lens;
    m_currentShot->shot.ctl.aa.aeMode = temp_shot_ext.shot.ctl.aa.aeMode;
    m_currentShot->shot.ctl.aa.aeLock = temp_shot_ext.shot.ctl.aa.aeLock;
    m_currentShot->shot.ctl.aa.vendor_isoValue = temp_shot_ext.shot.ctl.aa.vendor_isoValue;
    m_currentShot->shot.ctl.aa.vendor_isoMode = temp_shot_ext.shot.ctl.aa.vendor_isoMode;
    m_currentShot->shot.ctl.aa.aeExpCompensation = temp_shot_ext.shot.ctl.aa.aeExpCompensation;

    controlInterval = m_internalFrameCount - requestInfo->sensorControledFrameCount;

    /* 7. Decide to make the internal frame */
    if (controlInterval < SENSOR_CONTROL_DELAY) {
        m_isNeedInternalFrame = true;
        m_isNeedRequestFrame = false;
    } else if (request->getNeedInternalFrame() == true) {
        m_isNeedInternalFrame = true;
        m_isNeedRequestFrame = true;
    } else {
        m_isNeedInternalFrame = false;
        m_isNeedRequestFrame = true;
    }

    CLOGV2("INFO(%s[%d]):framecount %d needRequestFrame %d needInternalFrame %d",
           __FUNCTION__, __LINE__,
           m_internalFrameCount, m_isNeedRequestFrame, m_isNeedInternalFrame);

    return;
}

status_t ExynosCamera3::m_previewframeHandler(ExynosCameraRequest *request, ExynosCamera3FrameFactory *targetfactory)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrame *newFrame = NULL;
    uint32_t bufferCnt = 0;
    uint32_t requestKey = 0;
    int32_t bufIndex = -1;
    ExynosCameraBuffer buffer;
    bool captureFlag = false;
    bool rawStreamFlag = false;
    bool zslStreamFlag = false;
    bool needDynamicBayer = false;
    bool usePureBayer = false;
    uint32_t frameCount = 0;
    int32_t reprocessingBayerMode = m_parameters->getReprocessingBayerMode();

    /* set buffers belonged to each stream as available */
    // TODO: acquire fence

    frameCount = request->getFrameCount();
    requestKey = request->getKey();

    /* Initialize the request flags in framefactory */
    targetfactory->setRequestFLITE(false);
    targetfactory->setRequest3AC(false);
    targetfactory->setRequestSCC(false);
    targetfactory->setRequestSCP(false);

    m_flagBayerRequest = false;

    /* To decide the dynamic bayer request flag for JPEG capture */
    switch (reprocessingBayerMode) {
    case REPROCESSING_BAYER_MODE_NONE :
        needDynamicBayer = false;
        break;
    case REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON :
        targetfactory->setRequest(PIPE_FLITE, true);
        needDynamicBayer = false;
        usePureBayer = true;
        break;
    case REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON :
        targetfactory->setRequest(PIPE_3AC, true);
        needDynamicBayer = false;
        usePureBayer = false;
        break;
    case REPROCESSING_BAYER_MODE_PURE_DYNAMIC :
        needDynamicBayer = true;
        usePureBayer = true;
        break;
    case REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC :
        needDynamicBayer = true;
        usePureBayer = false;
        break;
    default :
        break;
    }

    /* Setting DMA-out request flag based on stream configuration */
    bufferCnt = request->getNumOfOutputBuffer();

    for (size_t i = 0; i < bufferCnt; i++) {
        int id = -1;
        id = request->getStreamId(i);

        switch (id % HAL_STREAM_ID_MAX) {
        case HAL_STREAM_ID_RAW:
            CLOGD2("request(%d) outputBuffer-Index(%zu) buffer-StreamType(HAL_STREAM_ID_RAW) ", request->getKey(), i);
            targetfactory->setRequestFLITE(true);
            m_flagBayerRequest = true;
            rawStreamFlag = true;

            break;
        case HAL_STREAM_ID_ZSL_OUTPUT:
            CLOGD2("request(%d) outputBuffer-Index(%zu) buffer-StreamType(HAL_STREAM_ID_ZSL)", request->getKey(), i);
            if (usePureBayer == true)
                targetfactory->setRequest(PIPE_FLITE, true);
            else
                targetfactory->setRequest(PIPE_3AC, true);
            zslStreamFlag = true;

            break;
        case HAL_STREAM_ID_VIDEO:
            recordingEnabled = true;
        case HAL_STREAM_ID_PREVIEW:
        case HAL_STREAM_ID_CALLBACK:
            CLOGV2("request(%d) outputBuffer-Index(%zu) buffer-StreamType(%d) ", request->getKey(), i, id);
            targetfactory->setRequestSCP(true);

            break;
        case HAL_STREAM_ID_JPEG:
            CLOGD2("request(%d) outputBuffer-Index %zu buffer-StreamType(HAL_STREAM_ID_JPEG)", request->getKey(), i);
            captureFlag = true;
            if (m_parameters->isReprocessing() == false) {
                targetfactory->setRequest(PIPE_3AC, true);
            } else if (needDynamicBayer == true) {
                if(m_parameters->getUsePureBayerReprocessing() == true)
                    targetfactory->setRequest(PIPE_FLITE, true);
                else
                    targetfactory->setRequest(PIPE_3AC, true);
            }

            break;
        default:
            CLOGE2("Invalid stream ID %d", id);
            break;
        }
    }

    if (m_currentShot == NULL) {
        CLOGE2("m_currentShot is NULL. requestKey %d ret %d", request->getKey(), ret);
        request->getServiceShot(m_currentShot);
    }

    m_updateCropRegion(m_currentShot);

    /* Set framecount into request */
    if (frameCount == 0) {
        m_requestMgr->setFrameCount(m_internalFrameCount++, request->getKey());
        frameCount = request->getFrameCount();
    }

    ret = m_generateFrame(frameCount, targetfactory, &m_processList, &m_processLock, &newFrame);
    if (ret != NO_ERROR) {
        CLOGE2("Failed to generateRequestFrame. framecount %d", frameCount);
        goto CLEAN;
    } else if (newFrame == NULL) {
        CLOGE2("newFrame is NULL. framecount %d", frameCount);
        goto CLEAN;
    }

    CLOGV2("frameCount:%d , requestKey:%d", frameCount, request->getKey());

    ret = newFrame->setMetaData(m_currentShot);
    if (ret != NO_ERROR) {
        CLOGE2("Set metadata to frame fail, Frame count(%d), ret(%d)", frameCount, ret);
    }

    newFrame->setFrameCapture(captureFlag);
    newFrame->setFrameZsl(zslStreamFlag);

    /* newFrame->printEntity(); */
    if (m_parameters->isFlite3aaOtf() == true) {
        /* It is assumed that flite is first, So use newFrame->getFristEntity(). */
        /* TODO: If you want location of flite entity isn't first. Don't use this. */
        if (m_flagBayerRequest == true) {
            int bayerPipeId = m_getBayerPipeId();

            if ((rawStreamFlag == true) && (zslStreamFlag != true)) {
                CLOGD2("flite buffer(%d) use Service Bayer buffer", frameCount);

                ret = m_bayerBufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &buffer);
                if (ret != NO_ERROR) {
                    CLOGE2("Failed to get Service Bayer Buffer. framecount %d availableBuffer %d",
                            frameCount, m_bayerBufferMgr->getNumOfAvailableBuffer());
                } else {
                    newFrame->setFrameServiceBayer(true);
                }
            } else {
                CLOGD2("flite buffer use Internal Bayer buffer");

                ret = m_fliteBufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &buffer);
                if (ret != NO_ERROR) {
                    CLOGE2("getBuffer fail, pipeId(%d), frameCount(%d), ret(%d)", bayerPipeId, frameCount, ret);
                }

                CLOGV2("Use Internal Bayer Buffer. framecount %d bufferIndex %d", frameCount, buffer.index);
            }

            if (bufIndex < 0) {
                CLOGW2("Invalid bayer buffer index %d. Skip to pushFrame", bufIndex);
                ret = newFrame->setEntityState(bayerPipeId, ENTITY_STATE_COMPLETE);
                if (ret != NO_ERROR)
                    CLOGE2("Failed to setEntityState with COMPLETE to FLITE. framecount %d", frameCount);
            } else {
                ret = m_setupEntity(bayerPipeId, newFrame, NULL, &buffer);
                if (ret != NO_ERROR) {
                    CLOGE2("Failed to setupEntity with bayer buffer. framecount %d bufferIndex %d",
                            frameCount, buffer.index);
                } else {
                    CLOGD2("PushFrametoPipe PIPE_FLITE framecount(%d)", frameCount);
                    targetfactory->setOutputFrameQToPipe(m_pipeFrameDoneQ[bayerPipeId], bayerPipeId);
                    targetfactory->pushFrameToPipe(&newFrame, bayerPipeId);
                }
            }
        }

        m_setupEntity(PIPE_3AA, newFrame);
        if (m_parameters->isMcscVraOtf() == true) {
            targetfactory->setOutputFrameQToPipe(m_pipeFrameDoneQ[PIPE_3AA], PIPE_3AA);
        } else {
            targetfactory->setFrameDoneQToPipe(m_pipeFrameDoneQ[PIPE_3AA], PIPE_3AA);
            targetfactory->setOutputFrameQToPipe(m_pipeFrameDoneQ[PIPE_VRA], PIPE_VRA);
        }
        targetfactory->pushFrameToPipe(&newFrame, PIPE_3AA);
    } else { /* TODO: Implement M2M Path */
        m_setupEntity(PIPE_FLITE_FRONT, newFrame);
        targetfactory->pushFrameToPipe(&newFrame, PIPE_FLITE_FRONT);

        m_setupEntity(PIPE_3AA_FRONT, newFrame);
        m_setupEntity(PIPE_ISP_FRONT, newFrame);
        targetfactory->setOutputFrameQToPipe(m_pipeFrameDoneQ[INDEX(PIPE_ISP_FRONT)], PIPE_ISP_FRONT);

        m_setupEntity(PIPE_SCC_FRONT, newFrame);
        targetfactory->pushFrameToPipe(&newFrame, PIPE_SCC_FRONT);
        targetfactory->setOutputFrameQToPipe(m_pipeFrameDoneQ[INDEX(PIPE_SCC_FRONT)], PIPE_SCC_FRONT);

        m_setupEntity(PIPE_SCP_FRONT, newFrame);
        targetfactory->pushFrameToPipe(&newFrame, PIPE_SCP_FRONT);
        targetfactory->setOutputFrameQToPipe(m_pipeFrameDoneQ[INDEX(PIPE_SCP_FRONT)], PIPE_SCP_FRONT);
    }

    if (captureFlag == true
        && m_parameters->isReprocessing() == false) {
        m_captureCount++;
        CLOGI2("setFrameCapture(true), frameCount(%d) m_captureCount(%d) isRunningstate(%d)",
                frameCount, m_captureCount, m_captureThread->isRunning());
        m_captureQ->pushProcessQ(&newFrame);
    }

    if (m_flagBayerRequest == true && m_frameFactoryStartDone == true) {
        /* HAL 3.2 on 8MP Full concept is dynamic bayer */
        if (targetfactory->checkPipeThreadRunning(m_getBayerPipeId()) == false) {
            m_previewStreamBayerThread->run(PRIORITY_DEFAULT);
            targetfactory->startThread(m_getBayerPipeId());
        }
        targetfactory->setRequestFLITE(false);
    }

CLEAN:
    return ret;
}

status_t ExynosCamera3::m_captureframeHandler(ExynosCameraRequest *request, ExynosCamera3FrameFactory *targetfactory)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrame *newFrame = NULL;
    struct camera2_shot_ext shot_ext;
    uint32_t bufferCnt = 0;
    uint32_t requestKey = 0;
    uint32_t frameCount = 0;
    bool captureFlag = false;
    bool rawStreamFlag = false;
    bool zslFlag = false;
    bool isNeedThumbnail = false;

    frameCount = request->getFrameCount();

    CLOGD2("Capture request. requestKey %d frameCount %d",
            request->getKey(), frameCount);

    if (targetfactory == NULL) {
        CLOGE2("targetfactory is NULL");
        return INVALID_OPERATION;
    }

    /* set buffers belonged to each stream as available */
    /* wait for reprocessing instance of capture  */
    if (m_parameters->isReprocessing() == true) {
        if (m_flagStartReprocessingFrameFactory == false)
            m_reprocessingFrameFactoryStartThread->join();
        m_startPictureBufferThread->join();
    }

    requestKey = request->getKey();

    targetfactory->setRequest(PIPE_MCSC0_REPROCESSING, false);
    if (m_parameters->isHWFCEnabled() == true) {
        targetfactory->setRequest(PIPE_HWFC_JPEG_SRC_REPROCESSING, false);
        targetfactory->setRequest(PIPE_HWFC_THUMB_SRC_REPROCESSING, false);
    }

    /* set input buffers belonged to each stream as available */
    bufferCnt = request->getNumOfInputBuffer();

    for (size_t i = 0; i < bufferCnt ; i++) {
        int id = request->getStreamId(i);
        switch (id % HAL_STREAM_ID_MAX) {
        case HAL_STREAM_ID_ZSL_INPUT:
            CLOGD2("requestKey %d buffer-StreamType(HAL_STREAM_ID_ZSL_INPUT)", request->getKey());
            zslFlag = true;
            break;
        case HAL_STREAM_ID_JPEG:
        case HAL_STREAM_ID_PREVIEW:
        case HAL_STREAM_ID_VIDEO:
        case HAL_STREAM_ID_CALLBACK:
        case HAL_STREAM_ID_MAX:
            CLOGE2("frameCount %d requestKey %d Invalid buffer-StreamType(%d)",
                    frameCount, request->getKey(), id);
            break;
        default:
            break;
        }
    }

    /* set output buffers belonged to each stream as available */
    bufferCnt = request->getNumOfOutputBuffer();

    for (size_t i = 0; i < bufferCnt; i++) {
        int id = request->getStreamId(i);
        switch (id % HAL_STREAM_ID_MAX) {
        case HAL_STREAM_ID_JPEG:
            CLOGD2("frameCount %d requestKey %d buffer-StreamType(HAL_STREAM_ID_JPEG)",
                    frameCount, request->getKey());
            targetfactory->setRequest(PIPE_MCSC0_REPROCESSING, true);

            request->getServiceShot(&shot_ext);
            isNeedThumbnail = (shot_ext.shot.ctl.jpeg.thumbnailSize[0] > 0
                               && shot_ext.shot.ctl.jpeg.thumbnailSize[1] > 0) ? true : false;

            if (m_parameters->isHWFCEnabled() == true) {
                if (m_parameters->isUseYuvReprocessingForThumbnail() == false) {
                    targetfactory->setRequest(PIPE_HWFC_JPEG_SRC_REPROCESSING, true);
                } else if (isNeedThumbnail == false) {
                    targetfactory->setRequest(PIPE_HWFC_JPEG_SRC_REPROCESSING, true);
                    targetfactory->setRequest(PIPE_HWFC_THUMB_SRC_REPROCESSING, true);
                }
            }

            captureFlag = true;
            break;
        case HAL_STREAM_ID_RAW:
            CLOGV2("frameCount %d requestKey %d buffer-StreamType(HAL_STREAM_ID_RAW)",
                    frameCount, request->getKey());

            rawStreamFlag = true;
            break;
        case HAL_STREAM_ID_PREVIEW:
        case HAL_STREAM_ID_VIDEO:
        case HAL_STREAM_ID_CALLBACK:
        case HAL_STREAM_ID_MAX:
            CLOGE2("frameCount %d requestKey %d Invalid buffer-StreamType(%d)",
                    frameCount, request->getKey());
            break;
        default:
            break;
        }
    }

    if (m_currentShot == NULL) {
        CLOGW2("requestKey(%d) m_currentShot is NULL. Use request metadata.", request->getKey());
        request->getServiceShot(m_currentShot);
    }

    m_updateCropRegion(m_currentShot);
    m_updateJpegControlInfo(m_currentShot);

    /* Set framecount into request */
    if (request->getNeedInternalFrame() == true)
        /* Must use the same framecount with internal frame */
        m_internalFrameCount--;

    if (frameCount == 0) {
        m_requestMgr->setFrameCount(m_internalFrameCount++, request->getKey());
        frameCount = request->getFrameCount();
    }

    ret = m_generateFrame(frameCount, targetfactory, &m_captureProcessList, &m_captureProcessLock, &newFrame);
    if (ret != NO_ERROR) {
        CLOGE2("m_generateFrame fail");
        return INVALID_OPERATION;
    } else if (newFrame == NULL) {
        CLOGE2("new frame is NULL");
        return INVALID_OPERATION;
    }

    CLOGV2("generate request framecount %d requestKey %d", frameCount, request->getKey());

    ret = newFrame->setMetaData(m_currentShot);
    if (ret != NO_ERROR)
        CLOGE2("Set metadata to frame fail, Frame count(%d), ret(%d)",
                frameCount, ret);

    newFrame->setFrameServiceBayer(rawStreamFlag);
    newFrame->setFrameCapture(captureFlag);
    newFrame->setFrameZsl(zslFlag);

    m_selectBayerQ->pushProcessQ(&newFrame);

    if(m_selectBayerThread != NULL
       && m_selectBayerThread->isRunning() == false) {
        m_selectBayerThread->run();
        CLOGI2("Initiate selectBayerThread (%d)", m_selectBayerThread->getTid());
    }

    return ret;
}

status_t ExynosCamera3::m_createRequestFrameFunc(ExynosCameraRequest *request)
{
    int32_t factoryAddrIndex = 0;
    bool removeDupFlag = false;

    ExynosCamera3FrameFactory *factory = NULL;
    ExynosCamera3FrameFactory *factoryAddr[100] ={NULL,};
    FrameFactoryList factorylist;
    FrameFactoryListIterator factorylistIter;
    factory_handler_t frameCreateHandler;

    // TODO: acquire fence
    /* 1. Remove the duplicated frame factory in request */
    factoryAddrIndex = 0;
    factorylist.clear();

    request->getFrameFactoryList(&factorylist);
    for (factorylistIter = factorylist.begin(); factorylistIter != factorylist.end(); ) {
        removeDupFlag = false;
        factory = *factorylistIter;
        ALOGV("DEBUG(%s[%d]):list Factory(%p) ", __FUNCTION__, __LINE__, factory);

        for (int i = 0; i < factoryAddrIndex ; i++) {
            if (factoryAddr[i] == factory) {
                removeDupFlag = true;
                break;
            }
        }

        if (removeDupFlag) {
            ALOGV("DEBUG(%s[%d]):remove duplicate Factory factoryAddrIndex(%d)",
                    __FUNCTION__, __LINE__, factoryAddrIndex);
            factorylist.erase(factorylistIter++);
        } else {
            ALOGV("DEBUG(%s[%d]):add frame factory, factoryAddrIndex(%d)",
                    __FUNCTION__, __LINE__, factoryAddrIndex);
            factoryAddr[factoryAddrIndex] = factory;
            factoryAddrIndex++;
            factorylistIter++;
        }

    }

    /* 2. Call the frame create handler for each frame factory */
    for (int i = 0; i < factoryAddrIndex; i++) {
        ALOGV("DEBUG(%s[%d]):framefactory index(%d) maxIndex(%d) (%p)",
                __FUNCTION__, __LINE__, i, factoryAddrIndex, factoryAddr[i]);
        frameCreateHandler = factoryAddr[i]->getFrameCreateHandler();
        (this->*frameCreateHandler)(request, factoryAddr[i]);
    }

    ALOGV("DEBUG(%s[%d]):- OUT - (F:%d)", __FUNCTION__, __LINE__, request->getKey());
    return NO_ERROR;
}

status_t ExynosCamera3::m_createInternalFrameFunc(void)
{
    status_t ret = NO_ERROR;
    uint32_t waitTime = 15 * 1000000; /* 15 msec as a default */
    uint32_t maxFps = 0;
    ExynosCameraBufferManager *bufferMgr = NULL;
    ExynosCamera3FrameFactory *factory = NULL;
    ExynosCameraFrame *newFrame = NULL;
    short retryCount = 4;

    /* 1. Generate the internal frame */
    factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];
    if (m_parameters->isFlite3aaOtf() == true) {
        factory->setRequestFLITE(false);
    } else
        factory->setRequestFLITE(true);

    ret = m_generateInternalFrame(m_internalFrameCount++, factory, &m_processList, &m_processLock, &newFrame);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):m_generateFrame failed", __FUNCTION__, __LINE__);
        return ret;
    } else if (newFrame == NULL) {
        ALOGE("ERR(%s[%d]):newFrame is NULL", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    ALOGV("INFO(%s[%d]):generate internal framecount %d",
            __FUNCTION__, __LINE__,
            newFrame->getFrameCount());

    /* 2. Set DMA-out request flag into frame
     *                    3AP,   3AC,   ISP,   ISPP,  ISPC,  SCC,   DIS,   SCP */
    newFrame->setRequest(false, false, false, false, false, false, false, false);

    switch (m_parameters->getReprocessingBayerMode()) {
    case REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON :
        newFrame->setRequest(PIPE_FLITE, true);
        break;
    case REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON:
        newFrame->setRequest(PIPE_3AC, true);
        break;
    default:
        break;
    }

    /* 3. Update the metadata with m_currentShot into frame */
    if (m_currentShot == NULL) {
        CLOGE2("ERR(%s[%d]):m_currentShot is NULL", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }
    ret = newFrame->setMetaData(m_currentShot);
    if (ret != NO_ERROR) {
        CLOGE2("ERR(%s[%d]):Failed to setMetaData with m_currehtShot. framecount %d ret %d",
                __FUNCTION__, __LINE__,
                newFrame->getFrameCount(), ret);
        return ret;
    }

    /* 4. Attach the buffers into frame */
    maxFps = m_currentShot->shot.ctl.aa.aeTargetFpsRange[1];
    if (maxFps > 0)
        waitTime = (1000 / maxFps) * 1000000 / 2; // Wait for the half of MIN frame duration(MAX fps)
    while (retryCount-- > 0) {
        if (m_parameters->isFlite3aaOtf() == false) {
            ret = m_setupEntity(PIPE_FLITE, newFrame);
            if (ret != NO_ERROR) {
                ALOGW("WARN(%s[%d]):Get FLITE buffer failed!, framecount(%d), availableFLITEBuffer(%d), sleep(%d ns/%d fps(MAX)), retryCount(%d)",
                        __FUNCTION__, __LINE__, newFrame->getFrameCount(),
                        m_fliteBufferMgr->getNumOfAvailableBuffer(), waitTime, maxFps, retryCount);
                usleep(waitTime);
                continue;
            }
        } else {
            ret = m_setupEntity(PIPE_3AA, newFrame);
            if (ret != NO_ERROR) {
                ret = m_getBufferManager(PIPE_3AA, &bufferMgr, SRC_BUFFER_DIRECTION);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to getBufferManager for 3AA. framecount %d",
                            __FUNCTION__, __LINE__, newFrame->getFrameCount());
                    return ret;
                }

                ALOGW("WARN(%s[%d]):Get 3AA buffer failed!, framecount(%d), available3AABuffer(%d), sleep(%d ns/%d fps(MAX)), retryCount(%d)",
                        __FUNCTION__, __LINE__, newFrame->getFrameCount(),
                        m_3aaBufferMgr->getNumOfAvailableBuffer(), waitTime, maxFps, retryCount);

                usleep(waitTime);
                continue;
            }
        }
        break;
    }
    if (retryCount == 0 && ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Get 3AA buffer finally failed!, framecount(%d), available3AABuffer(%d)",
                __FUNCTION__, __LINE__, newFrame->getFrameCount(),
                m_3aaBufferMgr->getNumOfAvailableBuffer());
        return ret;
    }

    /* 5. Push the frame to 3AA */
    if (m_parameters->isFlite3aaOtf() == false) {
        factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[PIPE_FLITE], PIPE_FLITE);
        factory->pushFrameToPipe(&newFrame, PIPE_FLITE);
    } else {
        if (m_parameters->isMcscVraOtf() == true) {
            factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[PIPE_3AA], PIPE_3AA);
        } else {
            factory->setFrameDoneQToPipe(m_pipeFrameDoneQ[PIPE_3AA], PIPE_3AA);
            factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[PIPE_VRA], PIPE_VRA);
        }
        factory->pushFrameToPipe(&newFrame, PIPE_3AA);
    }

    return ret;
}

status_t ExynosCamera3::m_createPrepareFrameFunc(__unused ExynosCameraRequest *request)
{
    status_t ret = NO_ERROR;
    short retryCount = 4;
    uint32_t waitTime = 15 * 1000000; /* 15 msec as as defatul */
    uint32_t maxFps = 0;
    uint32_t pipeId = MAX_PIPE_NUM;
    uint32_t flitePrepareCnt = m_prepareFliteCnt;
    uint32_t taaPrepareCnt = m_exynosconfig->current->pipeInfo.prepare[PIPE_3AA];
    ExynosCamera3FrameFactory *factory = NULL;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraStream *stream = NULL;

    ExynosCameraBufferManager *taaBufferManager[MAX_NODE];
    ExynosCameraBufferManager *ispBufferManager[MAX_NODE];
    ExynosCameraBufferManager *disBufferManager[MAX_NODE];
    ExynosCameraBufferManager *vraBufferManager[MAX_NODE];

    for (int i = 0; i < MAX_NODE; i++) {
        taaBufferManager[i] = NULL;
        ispBufferManager[i] = NULL;
        disBufferManager[i] = NULL;
        vraBufferManager[i] = NULL;
    }
    factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];

    if (m_factoryStartFlag == true) {
        m_factoryStartFlag = false;

        ret = factory->initPipes();
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):m_previewFrameFactory->initPipes() failed", __FUNCTION__, __LINE__);
            return ret;
        }

        if (m_parameters->getTpuEnabledMode() == true) {
#if 0
            factory->setRequestISPP(true);
            factory->setRequestDIS(true);

            if (m_parameters->is3aaIspOtf() == true) {
                taaBufferManager[factory->getNodeType(PIPE_3AA)] = m_3aaBufferMgr;
                taaBufferManager[factory->getNodeType(PIPE_3AC)] = m_fliteBufferMgr;
                taaBufferManager[factory->getNodeType(PIPE_ISPP)] = m_hwDisBufferMgr;

                disBufferManager[factory->getNodeType(PIPE_DIS)] = m_hwDisBufferMgr;
                disBufferManager[factory->getNodeType(PIPE_SCP)] = scpBufferMgr;
            } else {
                taaBufferManager[factory->getNodeType(PIPE_3AA)] = m_3aaBufferMgr;
                taaBufferManager[factory->getNodeType(PIPE_3AC)] = m_fliteBufferMgr;
                taaBufferManager[factory->getNodeType(PIPE_3AP)] = m_ispBufferMgr;

                ispBufferManager[factory->getNodeType(PIPE_ISP)] = m_ispBufferMgr;
                ispBufferManager[factory->getNodeType(PIPE_ISPP)] = m_hwDisBufferMgr;

                disBufferManager[factory->getNodeType(PIPE_DIS)] = m_hwDisBufferMgr;
                disBufferManager[factory->getNodeType(PIPE_SCP)] = scpBufferMgr;
            }
#endif
        } else {
        factory->setRequestISPP(false);
        factory->setRequestDIS(false);

        if (m_parameters->is3aaIspOtf() == true) {
            if (m_parameters->getDualMode() == true && getCameraId() == CAMERA_ID_FRONT) {
                factory->setRequestFLITE(true);
                factory->setRequestISPC(true);

                taaBufferManager[factory->getNodeType(PIPE_3AA)] = m_fliteBufferMgr;
                taaBufferManager[factory->getNodeType(PIPE_ISPC)] = m_yuvCaptureBufferMgr;
            } else {

                taaBufferManager[factory->getNodeType(PIPE_3AA)] = m_3aaBufferMgr;
                if (m_parameters->isUsing3acForIspc() == true)
                    taaBufferManager[factory->getNodeType(PIPE_3AC)] = m_yuvCaptureBufferMgr;
#ifndef RAWDUMP_CAPTURE
                else
                    taaBufferManager[factory->getNodeType(PIPE_3AC)] = m_fliteBufferMgr;
#endif
                taaBufferManager[factory->getNodeType(PIPE_SCP)] = m_internalScpBufferMgr;
            }
        } else {
            taaBufferManager[factory->getNodeType(PIPE_3AA)] = m_3aaBufferMgr;
            taaBufferManager[factory->getNodeType(PIPE_3AC)] = m_fliteBufferMgr;
            taaBufferManager[factory->getNodeType(PIPE_3AP)] = m_ispBufferMgr;

            ispBufferManager[factory->getNodeType(PIPE_ISP)] = m_ispBufferMgr;
            ispBufferManager[factory->getNodeType(PIPE_SCP)] = m_internalScpBufferMgr;
        }
    }

    if (m_parameters->isMcscVraOtf() == false) {
        vraBufferManager[OUTPUT_NODE] = m_vraBufferMgr;

        ret = factory->setBufferManagerToPipe(vraBufferManager, PIPE_VRA);
        if (ret != NO_ERROR) {
            CLOGE2("m_previewFrameFactory->setBufferManagerToPipe(vraBufferManager, %d) failed", PIPE_VRA);
            return ret;
        }
    }

    for (int i = 0; i < MAX_NODE; i++) {
    /* If even one buffer slot is valid. call setBufferManagerToPipe() */
        if (taaBufferManager[i] != NULL) {
            ret = factory->setBufferManagerToPipe(taaBufferManager, PIPE_3AA);
            if (ret != NO_ERROR) {
                CLOGE2("m_previewFrameFactory->setBufferManagerToPipe(taaBufferManager, %d) failed", PIPE_3AA);
                return ret;
            }
            break;
        }
    }

    for (int i = 0; i < MAX_NODE; i++) {
        /* If even one buffer slot is valid. call setBufferManagerToPipe() */
        if (ispBufferManager[i] != NULL) {
            ret = factory->setBufferManagerToPipe(ispBufferManager, PIPE_ISP);
            if (ret != NO_ERROR) {
                CLOGE2("m_previewFrameFactory->setBufferManagerToPipe(ispBufferManager, %d) failed", PIPE_ISP);
                return ret;
            }
            break;
        }
    }

    if (m_parameters->getHWVdisMode()) {
        for (int i = 0; i < MAX_NODE; i++) {
            /* If even one buffer slot is valid. call setBufferManagerToPipe() */
            if (disBufferManager[i] != NULL) {
                ret = factory->setBufferManagerToPipe(disBufferManager, PIPE_DIS);
                if (ret != NO_ERROR) {
                    CLOGE2("m_previewFrameFactory->setBufferManagerToPipe(disBufferManager, %d) failed", PIPE_DIS);
                    return ret;
                }
                break;
            }
        }
    }

    }

    /* 1. Generate the internal frame */
    if (m_parameters->isFlite3aaOtf() == true) {
        factory->setRequestFLITE(false);
        pipeId = PIPE_3AA;
    } else {
        factory->setRequestFLITE(true);
        pipeId = PIPE_FLITE;
    }
    ret = m_generateInternalFrame(m_internalFrameCount++, factory, &m_processList, &m_processLock, &newFrame);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):m_generateFrame failed", __FUNCTION__, __LINE__);
        return ret;
    } else if (newFrame == NULL) {
        ALOGE("ERR(%s[%d]):newFrame is NULL", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    /* newFrame->dump(); */

    ALOGI("INFO(%s[%d]):generate prepare framecount %d",
            __FUNCTION__, __LINE__,
            newFrame->getFrameCount());

    /* 2. Set DMA-out request flag into frame
     *                    3AP,   3AC,   ISP,   ISPP,  ISPC,  SCC,   DIS,   SCP */
    /* newFrame->setRequest(false, false, false, false, false, false, false, false); */
#if 0
    if (m_parameters->is3aaIspOtf() == false) {
        newFrame->setRequest(PIPE_3AP, true);
        taaBufferManager[OUTPUT_NODE] = m_3aaBufferMgr;
        taaBufferManager[SUB_NODE] = m_ispBufferMgr;
        ret = factory->setBufferManagerToPipe(taaBufferManager, PIPE_3AA);
        if (ret < 0) {
            ALOGE("ERR(%s):m_previewFrameFactory->setBufferManagerToPipe() failed", __FUNCTION__);
            return ret;
        }
    }
#endif

    /* 3. Update the metadata with m_currentShot into frame */
    ret = newFrame->setMetaData(m_currentShot);

    /* 4. Attach the buffers into frame */
    maxFps = m_currentShot->shot.ctl.aa.aeTargetFpsRange[1];
    if (maxFps > 0)
        waitTime = (1000 / maxFps) * 1000000 / 2; // Wait for the half of MIN frame duration(MAX fps)
    while (retryCount-- > 0) {
        ret = m_setupEntity(pipeId, newFrame);
        if (ret != NO_ERROR) {
            ALOGW("WARN(%s[%d]):Get %s buffer failed!, framecount(%d), availableFLITEBuffer(%d), sleep(%d ns/%d fps(MAX)), retryCount(%d)",
                    __FUNCTION__, __LINE__,
                    (pipeId == PIPE_FLITE)?"PIPE_FLITE":"PIPE_3AA",
                    newFrame->getFrameCount(),
                    m_fliteBufferMgr->getNumOfAvailableBuffer(), waitTime, maxFps, retryCount);
            usleep(waitTime);
            continue;
        }
        break;
    }
    if (retryCount == 0 && ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Get %s buffer finally failed!, framecount(%d), available3AABuffer(%d)",
                __FUNCTION__, __LINE__,
                (pipeId == PIPE_FLITE)?"PIPE_FLITE":"PIPE_3AA",
                newFrame->getFrameCount(),
                m_3aaBufferMgr->getNumOfAvailableBuffer());
        return ret;
    }

    /* 5. Push the frame to 3AA */
    if (m_parameters->isMcscVraOtf() == true) {
        factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[PIPE_3AA], PIPE_3AA);
    } else {
        factory->setFrameDoneQToPipe(m_pipeFrameDoneQ[PIPE_3AA], PIPE_3AA);
        factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[PIPE_VRA], PIPE_VRA);
    }
    factory->pushFrameToPipe(&newFrame, pipeId);

    /* before starting framefactory, we should set bayer prepare buffer count (max prepare count depend on 3aa count) */
    if ((factory->checkPipeThreadRunning(m_getBayerPipeId()) == false) &&
            (taaPrepareCnt - 1) >= (newFrame->getFrameCount()) &&
            m_parameters->isFlite3aaOtf() == false &&
            m_flagBayerRequest == true) {
        flitePrepareCnt++;
        if (flitePrepareCnt != m_prepareFliteCnt)
            ALOGI("INFO(%s[%d]):adjust PIPE_FLITE prepare count(%d => %d)", __FUNCTION__, __LINE__,
                    m_prepareFliteCnt, flitePrepareCnt);
        m_prepareFliteCnt = flitePrepareCnt;
    }

    return ret;
}

status_t ExynosCamera3::m_createFrameFunc(void)
{
    status_t ret = NO_ERROR;
    ExynosCameraRequest *request = NULL;
    int key = 0;
    /* 1. Get new service request from request manager */
    if (m_requestMgr->getServiceRequestCount() > 0) {
        m_popRequest(&request);
        key = request->getKey();
    }
    ALOGV("Service Request Count(%d) Total Request Count(%d)",
                m_requestMgr->getServiceRequestCount(),m_requestMgr->getRequestCount());

    /* 2. Push back the new service request into m_requestWaitingList */
    /* Warning :
     * The List APIs for 'm_requestWaitingList' are called sequencially.
     * So the mutex is not required.
     * If the 'm_requestWaitingList' will be accessed by another thread,
     * using mutex must be considered.
     */
    if (request != NULL) {
        request_info_t *requestInfo = new request_info_t;
        requestInfo->request = request;
        requestInfo->sensorControledFrameCount = 0;
        m_requestWaitingList.push_back(requestInfo);
    }

    /* 3. Update the current shot */
    if (m_requestWaitingList.size() > 0)
        m_updateCurrentShot();

    ALOGV("DEBUG(%s[%d]):Create New Frame %d Key %d needRequestFrame %d needInternalFrame %d waitingSize %d",
                __FUNCTION__, __LINE__,
                m_internalFrameCount, key, m_isNeedRequestFrame, m_isNeedInternalFrame, m_requestWaitingList.size());

    /* 4. Select the frame creation logic between request frame and internal frame */
    if (m_isNeedInternalFrame == true || m_requestWaitingList.empty() == true) {

        m_createInternalFrameFunc();
    }
    if (m_isNeedRequestFrame == true && m_requestWaitingList.empty() == false) {
        List<request_info_t *>::iterator r;
        request_info_t *requestInfo = NULL;

        r = m_requestWaitingList.begin();
        requestInfo = *r;
        request = requestInfo->request;

        m_createRequestFrameFunc(request);

        m_requestWaitingList.erase(r);
        delete requestInfo;
    }

    return ret;
}

status_t ExynosCamera3::m_sendRawCaptureResult(ExynosCameraFrame *frame, uint32_t pipeId, bool isSrc)
{
    status_t ret = NO_ERROR;
    ExynosCameraStream *stream = NULL;
    ExynosCameraRequest *request = NULL;
    ExynosCameraBuffer buffer;
    camera3_stream_buffer_t streamBuffer;
    ResultRequest resultRequest = NULL;

    /* 1. Get stream object for RAW */
    ret = m_streamManager->getStream(HAL_STREAM_ID_RAW, &stream);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):getStream is failed, from streammanager. Id error:HAL_STREAM_ID_RAW",
                __FUNCTION__, __LINE__);
        return ret;
    }

    /* 2. Get camera3_stream structure from stream object */
    ret = stream->getStream(&streamBuffer.stream);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):getStream is failed, from exynoscamerastream. Id error:HAL_STREAM_ID_RAW",
                __FUNCTION__, __LINE__);
        return ret;
    }

    /* 3. Get the bayer buffer from frame */
    if (isSrc == true)
        ret = frame->getSrcBuffer(pipeId, &buffer);
    else
        ret = frame->getDstBuffer(pipeId, &buffer);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):Get bayer buffer failed, framecount(%d), isSrc(%d), pipeId(%d)",
                __FUNCTION__, __LINE__,
                frame->getFrameCount(), isSrc, pipeId);
        return ret;
    }

    /* 4. Get the service buffer handle from buffer manager */
    ret = m_bayerBufferMgr->getHandleByIndex(&(streamBuffer.buffer), buffer.index);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):Buffer index error(%d)!!", __FUNCTION__, __LINE__, buffer.index);
        return ret;
    }

    /* 5. Update the remained buffer info */
    streamBuffer.status = CAMERA3_BUFFER_STATUS_OK;
    streamBuffer.acquire_fence = -1;
    streamBuffer.release_fence = -1;

    /* 6. Create new result for RAW buffer */
    request = m_requestMgr->getServiceRequest(frame->getFrameCount());
    resultRequest = m_requestMgr->createResultRequest(frame->getFrameCount(), EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY, NULL, NULL);
    resultRequest->pushStreamBuffer(&streamBuffer);

    /* 7. Request to callback the result to request manager */
    m_requestMgr->callbackSequencerLock();
    request->increaseCompleteBufferCount();
    m_requestMgr->callbackRequest(resultRequest);
    m_requestMgr->callbackSequencerUnlock();

#if 0
    /* 8. Send the bayer buffer to frame selector */
    if (frame->getFrameCapture() == true) {
        ALOGD("DEBUG(%s[%d]):Send the service bayer buffer to m_sccCaptureSelector. frameCount(%d)",
                __FUNCTION__, __LINE__, frame->getFrameCount());
        ret = m_sccCaptureSelector->manageFrameHoldList(frame, pipeId, isSrc);
        if (ret < 0) {
            ALOGE("ERR(%s[%d]):manageFrameHoldList failed, frameCount(%d)",
                    __FUNCTION__, __LINE__, frame->getFrameCount());
            return ret;
        }
    }
#endif
    ALOGE("DEBUG(%s[%d]):request->frame_number(%d), request->getNumOfOutputBuffer(%d) request->getCompleteBufferCount(%d) frame->getFrameCapture(%d)",
            __FUNCTION__, __LINE__,
            request->getKey(),
            request->getNumOfOutputBuffer(),
            request->getCompleteBufferCount(),
            frame->getFrameCapture());

    ALOGV("DEBUG(%s[%d]):streamBuffer info: stream (%p), handle(%p)",
            __FUNCTION__, __LINE__,
            streamBuffer.stream, streamBuffer.buffer);

    return ret;
}

status_t ExynosCamera3::m_sendZSLCaptureResult(ExynosCameraFrame *frame, __unused uint32_t pipeId, __unused bool isSrc)
{
    status_t ret = NO_ERROR;
    ExynosCameraStream *stream = NULL;
    ExynosCameraRequest *request = NULL;
    camera3_stream_buffer_t streamBuffer;
    ResultRequest resultRequest = NULL;
    const camera3_stream_buffer_t *buffer;
    const camera3_stream_buffer_t *bufferList;
    int streamId = 0;
    uint32_t bufferCount = 0;


    /* 1. Get stream object for ZSL */
    ret = m_streamManager->getStream(HAL_STREAM_ID_ZSL_OUTPUT, &stream);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):getStream is failed, from streammanager. Id error:HAL_STREAM_ID_ZSL",
                __FUNCTION__, __LINE__);
        return ret;
    }

    /* 2. Get camera3_stream structure from stream object */
    ret = stream->getStream(&streamBuffer.stream);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):getStream is failed, from exynoscamerastream. Id error:HAL_STREAM_ID_RAW",
                __FUNCTION__, __LINE__);
        return ret;
    }

    /* 3. Get zsl buffer */
    request = m_requestMgr->getServiceRequest(frame->getFrameCount());
    bufferCount = request->getNumOfOutputBuffer();
    bufferList = request->getOutputBuffers();

    for (uint32_t index = 0; index < bufferCount; index++) {
        buffer = &(bufferList[index]);
        stream = static_cast<ExynosCameraStream *>(bufferList[index].stream->priv);
        stream->getID(&streamId);

        if ((streamId % HAL_STREAM_ID_MAX) == HAL_STREAM_ID_ZSL_OUTPUT) {
            streamBuffer.buffer = bufferList[index].buffer;
        }
    }

    /* 4. Update the remained buffer info */
    streamBuffer.status = CAMERA3_BUFFER_STATUS_OK;
    streamBuffer.acquire_fence = -1;
    streamBuffer.release_fence = -1;

    /* 5. Create new result for ZSL buffer */
    resultRequest = m_requestMgr->createResultRequest(frame->getFrameCount(), EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY, NULL, NULL);
    resultRequest->pushStreamBuffer(&streamBuffer);

    /* 6. Request to callback the result to request manager */
    m_requestMgr->callbackSequencerLock();
    request->increaseCompleteBufferCount();
    m_requestMgr->callbackRequest(resultRequest);
    m_requestMgr->callbackSequencerUnlock();

    CLOGE("DEBUG(%s[%d]):request->frame_number(%d), request->getNumOfOutputBuffer(%d) request->getCompleteBufferCount(%d) frame->getFrameCapture(%d)",
            __FUNCTION__, __LINE__,
            request->getKey(),
            request->getNumOfOutputBuffer(),
            request->getCompleteBufferCount(),
            frame->getFrameCapture());

    CLOGV("DEBUG(%s[%d]):streamBuffer info: stream (%p), handle(%p)",
            __FUNCTION__, __LINE__,
            streamBuffer.stream, streamBuffer.buffer);

    return ret;

}


status_t ExynosCamera3::m_sendNotify(uint32_t frameNumber, int type)
{
    camera3_notify_msg_t notify;
    ResultRequest resultRequest = NULL;
    ExynosCameraRequest *request = NULL;
    uint32_t frameCount = 0;
    nsecs_t timeStamp = m_lastFrametime;

    status_t ret = OK;
    request = m_requestMgr->getServiceRequest(frameNumber);
    frameCount = request->getKey();
    timeStamp = request->getSensorTimestamp();

    CLOGV2("(%d)frame t(%lld), key : %d", frameCount, timeStamp, frameCount);
    switch (type) {
    case CAMERA3_MSG_ERROR:
        notify.type = CAMERA3_MSG_ERROR;
        notify.message.error.frame_number = frameCount;
        notify.message.error.error_code = CAMERA3_MSG_ERROR_BUFFER;
        // TODO: how can handle this?
        //msg.message.error.error_stream = j->stream;
        resultRequest = m_requestMgr->createResultRequest(frameNumber, EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY, NULL, &notify);
        m_requestMgr->callbackSequencerLock();
        m_requestMgr->callbackRequest(resultRequest);
        m_requestMgr->callbackSequencerUnlock();
        break;
    case CAMERA3_MSG_SHUTTER:
        CLOGV2("SHUTTER (%d)frame t(%lld)", frameNumber, timeStamp);
        notify.type = CAMERA3_MSG_SHUTTER;
        notify.message.shutter.frame_number = frameCount;
        notify.message.shutter.timestamp = timeStamp;
        resultRequest = m_requestMgr->createResultRequest(frameNumber, EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY, NULL, &notify);
        /* keep current frame time for flush ops */
        m_requestMgr->callbackSequencerLock();
        m_requestMgr->callbackRequest(resultRequest);
        m_requestMgr->callbackSequencerUnlock();
        m_captureResultDoneCondition.signal();
        break;
    default:
        CLOGE2("Msg type is invalid (%d)", type);
        ret = BAD_VALUE;
        break;
    }

    return ret;
}

status_t ExynosCamera3::m_searchFrameFromList(List<ExynosCameraFrame *> *list, Mutex *listLock, uint32_t frameCount, ExynosCameraFrame **frame)
{
    ExynosCameraFrame *curFrame = NULL;
    List<ExynosCameraFrame *>::iterator r;

    Mutex::Autolock l(listLock);
    if (list->empty()) {
        CLOGD2("list is empty");
        return NO_ERROR;
    }

    r = list->begin()++;

    do {
        curFrame = *r;
        if (curFrame == NULL) {
            CLOGE2("curFrame is empty");
            return INVALID_OPERATION;
        }

        if (frameCount == curFrame->getFrameCount()) {
            CLOGV2("frame count match: expected(%d)", frameCount);
            *frame = curFrame;
            return NO_ERROR;
        }
        r++;
    } while (r != list->end());

    CLOGV2("Cannot find match frame, frameCount(%d)", frameCount);

    return OK;
}

status_t ExynosCamera3::m_removeFrameFromList(List<ExynosCameraFrame *> *list, Mutex *listLock, ExynosCameraFrame *frame)
{
    ExynosCameraFrame *curFrame = NULL;
    int frameCount = 0;
    int curFrameCount = 0;
    List<ExynosCameraFrame *>::iterator r;

    if (frame == NULL) {
        CLOGE2("frame is NULL");
        return BAD_VALUE;
    }

    Mutex::Autolock l(listLock);
    if (list->empty()) {
        CLOGE2("list is empty");
        return INVALID_OPERATION;
    }

    frameCount = frame->getFrameCount();
    r = list->begin()++;

    do {
        curFrame = *r;
        if (curFrame == NULL) {
            CLOGE2("curFrame is empty");
            return INVALID_OPERATION;
        }

        curFrameCount = curFrame->getFrameCount();
        if (frameCount == curFrameCount) {
            CLOGV2("frame count match: expected(%d), current(%d)", frameCount, curFrameCount);
            list->erase(r);
            return NO_ERROR;
        }
        CLOGW2("frame count mismatch: expected(%d), current(%d)", frameCount, curFrameCount);
        /* curFrame->printEntity(); */
        r++;
    } while (r != list->end());

    CLOGE2("Cannot find match frame!!!");

    return INVALID_OPERATION;
}

status_t ExynosCamera3::m_clearList(List<ExynosCameraFrame *> *list, Mutex *listLock)
{
    ExynosCameraFrame *curFrame = NULL;
    List<ExynosCameraFrame *>::iterator r;

    CLOGD2("remaining frame(%zu), we remove them all", list->size());

    Mutex::Autolock l(listLock);
    while (!list->empty()) {
        r = list->begin()++;
        curFrame = *r;
        if (curFrame != NULL) {
            CLOGV2("remove frame count(%d)", curFrame->getFrameCount());
            curFrame->decRef();
            m_frameMgr->deleteFrame(curFrame);
        }
        list->erase(r);
    }

    return OK;
}

status_t ExynosCamera3::m_removeInternalFrames(List<ExynosCameraFrame *> *list, Mutex *listLock)
{
    ExynosCameraFrame *curFrame = NULL;
    List<ExynosCameraFrame *>::iterator r;

    ALOGD("DEBUG(%s[%d]):remaining frame(%zu), we remove internal frames",
            __FUNCTION__, __LINE__, list->size());

    Mutex::Autolock l(listLock);
    while (!list->empty()) {
        r = list->begin()++;
        curFrame = *r;
        if (curFrame != NULL) {
            if (curFrame->getFrameType() == FRAME_TYPE_INTERNAL) {
                ALOGV("DEBUG(%s[%d]):remove internal frame(%d)",
                        __FUNCTION__, __LINE__, curFrame->getFrameCount());
                m_releaseInternalFrame(curFrame);
            } else {
                ALOGW("WARN(%s[%d]):frame(%d) is NOT internal frame and will be remained in List",
                        __FUNCTION__, __LINE__, curFrame->getFrameCount());
            }
        }
        list->erase(r);
        curFrame = NULL;
    }

    return OK;
}

status_t ExynosCamera3::m_releaseInternalFrame(ExynosCameraFrame *frame)
{
    status_t ret = NO_ERROR;
    ExynosCameraBuffer buffer;
    ExynosCameraBufferManager *bufferMgr = NULL;

    if (frame == NULL) {
        ALOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (frame->getFrameType() != FRAME_TYPE_INTERNAL) {
        ALOGE("ERR(%s[%d]):frame(%d) is NOT internal frame",
                __FUNCTION__, __LINE__, frame->getFrameCount());
        return BAD_VALUE;
    }

    /* Return bayer buffer */
    if (m_parameters->isFlite3aaOtf() == false) {
        ret = frame->getDstBuffer(PIPE_FLITE, &buffer);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):getDstBuffer failed. PIPE_FLITE, ret %d",
                    __FUNCTION__, __LINE__, ret);
        } else if (buffer.index >= 0) {
            ret = m_getBufferManager(PIPE_FLITE, &bufferMgr, DST_BUFFER_DIRECTION);
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):Failed to getBufferManager for FLITE",
                        __FUNCTION__, __LINE__);
            } else {
                ret = m_putBuffers(bufferMgr, buffer.index);
                if (ret != NO_ERROR)
                    ALOGE("ERR(%s[%d]):Failed to putBuffer for FLITE. index %d",
                            __FUNCTION__, __LINE__, buffer.index);
            }
        }
    }

    /* Return 3AS buffer */
    ret = frame->getSrcBuffer(PIPE_3AA, &buffer);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):getSrcBuffer failed. PIPE_3AA, ret(%d)",
                __FUNCTION__, __LINE__, ret);
    } else if (buffer.index >= 0) {
        ret = m_getBufferManager(PIPE_3AA, &bufferMgr, SRC_BUFFER_DIRECTION);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):Failed to getBufferManager for 3AS",
                    __FUNCTION__, __LINE__);
        } else {
            ret = m_putBuffers(bufferMgr, buffer.index);
            if (ret != NO_ERROR)
                ALOGE("ERR(%s[%d]):Failed to putBuffer for 3AS. index %d",
                        __FUNCTION__, __LINE__, buffer.index);
        }
    }

    /* Return 3AP buffer */
    if (frame->getRequest(PIPE_3AP) == true) {
        ret = frame->getDstBuffer(PIPE_3AA, &buffer);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):getDstBuffer failed. PIPE_3AA, ret %d",
                    __FUNCTION__, __LINE__, ret);
        } else if (buffer.index >= 0) {
            ret = m_getBufferManager(PIPE_3AA, &bufferMgr, DST_BUFFER_DIRECTION);
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):Failed to getBufferManager for 3AP",
                        __FUNCTION__, __LINE__);
            } else {
                ret = m_putBuffers(bufferMgr, buffer.index);
                if (ret != NO_ERROR)
                    ALOGE("ERR(%s[%d]):Failed to putBuffer for 3AP. index %d",
                            __FUNCTION__, __LINE__, buffer.index);
            }
        }
    }

    frame->decRef();
    m_frameMgr->deleteFrame(frame);
    frame = NULL;

    return ret;
}

status_t ExynosCamera3::m_setFrameManager()
{
    sp<FrameWorker> worker;
    m_frameMgr = new ExynosCameraFrameManager("FRAME MANAGER", m_cameraId, FRAMEMGR_OPER::SLIENT, 50, 100);

    worker = new CreateWorker("CREATE FRAME WORKER", m_cameraId, FRAMEMGR_OPER::SLIENT, 40);
    m_frameMgr->setWorker(FRAMEMGR_WORKER::CREATE, worker);

    worker = new DeleteWorker("DELETE FRAME WORKER", m_cameraId, FRAMEMGR_OPER::SLIENT);
    m_frameMgr->setWorker(FRAMEMGR_WORKER::DELETE, worker);

    sp<KeyBox> key = new KeyBox("FRAME KEYBOX", m_cameraId);

    m_frameMgr->setKeybox(key);

    return NO_ERROR;
}

bool ExynosCamera3::m_frameFactoryCreateThreadFunc(void)
{

#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif
    bool loop = false;
    status_t ret = NO_ERROR;

    ExynosCamera3FrameFactory *framefactory = NULL;

    ret = m_frameFactoryQ->waitAndPopProcessQ(&framefactory);
    if (ret < 0) {
        CLOGE2("wait and pop fail, ret(%d)", ret);
        goto func_exit;
    }

    if (framefactory == NULL) {
        CLOGE2("framefactory is NULL");
        goto func_exit;
    }

    if (framefactory->isCreated() == false) {
        CLOGD2("framefactory create");
        framefactory->create();
    } else {
        CLOGD2("framefactory already create");
    }

func_exit:
    if (0 < m_frameFactoryQ->getSizeOfProcessQ()) {
        loop = true;
    }

    return loop;
}

bool ExynosCamera3::m_frameFactoryStartThreadFunc(void)
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif
    status_t ret = NO_ERROR;
    ExynosCamera3FrameFactory *factory = NULL;
    ExynosCameraRequest *request = NULL;
    uint32_t prepare = 1;

    if (m_requestMgr->getServiceRequestCount() < 1) {
        ALOGE("ERR(%s[%d]):There is NO available request!!! \"processCaptureRequest()\" must be called, first!!!", __FUNCTION__, __LINE__);
        return false;
    }

    /* 1. Get the first request from the request manager */
    m_popRequest(&request);
    if (request == NULL) {
        ALOGE("ERR(%s[%d]):request is NULL", __FUNCTION__, __LINE__);
    } else {
        request_info_t *requestInfo = new request_info_t;
        requestInfo->request = request;
        requestInfo->sensorControledFrameCount = 0;
        m_requestWaitingList.push_back(requestInfo);
    }

    /* 2. Get the initial metadata from request */
    ret = request->getServiceShot(m_currentShot);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Failed to getServiceShot. requestKey %d ret %d",
                __FUNCTION__, __LINE__, request->getKey(), ret);
        return false;
    }

    m_internalFrameCount = 0;
    prepare = m_exynosconfig->current->pipeInfo.prepare[PIPE_3AA];

    ALOGD("DEBUG(%s[%d]):prepare %d", __FUNCTION__, __LINE__, prepare);

    /* 3. Push the prepare frame into 3AA Pipe
     * - call initPipes()
     */
    m_createPrepareFrameFunc(request);
    for (uint32_t i = 0; i < prepare; i++) {
        ret = m_createFrameFunc();
        if (ret != NO_ERROR)
            ALOGE("ERR(%s[%d]):Failed to createFrameFunc for preparing frame. prepareCount %d/%d",
                    __FUNCTION__, __LINE__, i, prepare);
    }

    factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];
    if (factory != NULL) {
        /* - call preparePipes();
         * - call startPipes()
         * - call startInitialThreads()
         */
        if (m_flagStartFrameFactory == false)
            m_startFrameFactory(factory);

        if (m_shotDoneQ != NULL)
            m_shotDoneQ->release();

        for (int i = 0; i < MAX_PIPE_NUM; i++) {
            if (m_pipeFrameDoneQ[i] != NULL) {
                m_pipeFrameDoneQ[i]->release();
            }
        }
        m_reprocessingDoneQ->release();
        m_pipeCaptureFrameDoneQ->release();
        m_mainThread->run(PRIORITY_URGENT_DISPLAY);

        m_previewStream3AAThread->run(PRIORITY_DEFAULT);
        if (m_parameters->isMcscVraOtf() == false)
            m_previewStreamVRAThread->run(PRIORITY_DEFAULT);
        if (m_flagBayerRequest == true)
            m_previewStreamBayerThread->run(PRIORITY_DEFAULT);
        m_duplicateBufferThread->run(PRIORITY_DEFAULT);
        m_monitorThread->run(PRIORITY_DEFAULT);

        m_internalFrameThread->run(PRIORITY_DEFAULT);

#ifdef USE_INTERNAL_FRAME
        /* internal frame */
        m_internalFrameHandlerThread->run(PRIORITY_DEFAULT);
#endif /* #ifdef USE_INTERNAL_FRAME */
        if (m_flagBayerRequest == true
            && factory->checkPipeThreadRunning(m_getBayerPipeId()) == false)
            factory->startThread(m_getBayerPipeId());
    } else {
        CLOGE2("Can't start FrameFactory!!!! FrameFactory is NULL!! Prepare(%d), Request(%d)",
                prepare, m_requestMgr != NULL ? m_requestMgr->getRequestCount(): 0);
        return false;
    }
    m_frameFactoryStartDone = true;

    return false;
}

status_t ExynosCamera3::m_constructFrameFactory(void)
{
    CLOGI2("-IN-");

    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    status_t ret = NO_ERROR;
    ExynosCamera3FrameFactory *factory = NULL;

    for(int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++)
        m_frameFactory[i] = NULL;

    factory = new ExynosCamera3FrameFactoryPreview(m_cameraId, m_parameters);
    factory->setFrameCreateHandler(&ExynosCamera3::m_previewframeHandler);
    m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW] = factory;
    m_frameFactory[FRAME_FACTORY_TYPE_RECORDING_PREVIEW] = factory;

    m_frameFactory[FRAME_FACTORY_TYPE_DUAL_PREVIEW] = factory;

    if (m_parameters->isReprocessing() == true) {
        factory = new ExynosCamera3FrameReprocessingFactory(m_cameraId, m_parameters);
        factory->setFrameCreateHandler(&ExynosCamera3::m_captureframeHandler);
        m_frameFactory[FRAME_FACTORY_TYPE_REPROCESSING] = factory;
    }

    for (int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++) {
        factory = m_frameFactory[i];
        if ((factory != NULL) && (factory->isCreated() == false)) {
            factory->setFrameManager(m_frameMgr);
            m_frameFactoryQ->pushProcessQ(&factory);
        }
    }

    CLOGI2("-OUT-");

    return ret;
}

status_t ExynosCamera3::m_startFrameFactory(ExynosCamera3FrameFactory *factory)
{
    status_t ret = OK;

    uint32_t flitePrepareCnt = m_prepareFliteCnt;
    CLOGD2("flitePrepareCnt:%d", flitePrepareCnt);

        /* prepare pipes */
#if !defined(ENABLE_FULL_FRAME)
        ret = factory->preparePipes(flitePrepareCnt);
#else
        ret = factory->preparePipes();
#endif

    if (ret < 0) {
        CLOGW("ERR(%s[%d]):Failed to prepare FLITE", __FUNCTION__, __LINE__);
    }

    /* s_ctrl HAL version for selecting dvfs table */
    ret = factory->setControl(V4L2_CID_IS_HAL_VERSION, IS_HAL_VER_3_2, PIPE_3AA);

    if (ret < 0)
        CLOGW2("V4L2_CID_IS_HAL_VERSION is fail");

    /* stream on pipes */
    ret = factory->startPipes();
    if (ret < 0) {
        CLOGE2("startPipe fail");
        return ret;
    }

    /* start all thread */
    ret = factory->startInitialThreads();
    if (ret < 0) {
        CLOGE2("startInitialThreads fail");
        return ret;
    }

    m_flagStartFrameFactory = true;

    return ret;
}

status_t ExynosCamera3::m_stopFrameFactory(ExynosCamera3FrameFactory *factory)
{
    int ret = 0;

    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    if (factory != NULL && factory->isCreated()) {
        ret = factory->stopPipes();
        if (ret < 0) {
            CLOGE2("stopPipe fail");
            return ret;
        }
    }

    m_flagStartFrameFactory = false;

    return ret;
}

status_t ExynosCamera3::m_deinitFrameFactory()
{
    CLOGI2("-IN-");

    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;
    ExynosCamera3FrameFactory *frameFactory = NULL;

    for (int i = 0; i < FRAME_FACTORY_TYPE_MAX; i++) {
        if (m_frameFactory[i] != NULL) {
            frameFactory = m_frameFactory[i];

            for (int k = i + 1; k < FRAME_FACTORY_TYPE_MAX; k++) {
               if (frameFactory == m_frameFactory[k]) {
                   CLOGD2("m_frameFactory index(%d) and index(%d) are same instance, set index(%d) = NULL", i, k, k);
                   m_frameFactory[k] = NULL;
               }
            }

            ret = m_frameFactory[i]->destroy();
            if (ret < 0)
                CLOGE2("m_frameFactory[%d] destroy fail", i);

            SAFE_DELETE(m_frameFactory[i]);

            CLOGD2("m_frameFactory[%d] destroyed", i);
        }
    }

    CLOGI2("-OUT-");

    return ret;

}

status_t ExynosCamera3::m_setupReprocessingPipeline(void)
{
    status_t ret = NO_ERROR;
    uint32_t pipeId = MAX_PIPE_NUM;
    ExynosCamera3FrameFactory *factory = m_frameFactory[FRAME_FACTORY_TYPE_REPROCESSING];
    ExynosCameraStream *stream = NULL;
    ExynosCameraBufferManager *bufferMgr = NULL;
    ExynosCameraBufferManager *taaBufferManager[MAX_NODE];
    ExynosCameraBufferManager *ispBufferManager[MAX_NODE];
    ExynosCameraBufferManager *mcscBufferManager[MAX_NODE];
    ExynosCameraBufferManager **tempBufferManager;

    for (int i = 0; i < MAX_NODE; i++) {
        taaBufferManager[i] = NULL;
        ispBufferManager[i] = NULL;
        mcscBufferManager[i] = NULL;
    }

    /* Setting bufferManager based on H/W pipeline */
    tempBufferManager = taaBufferManager;
    pipeId = PIPE_3AA_REPROCESSING;

    tempBufferManager[factory->getNodeType(PIPE_3AA_REPROCESSING)] = m_fliteBufferMgr;
    tempBufferManager[factory->getNodeType(PIPE_3AP_REPROCESSING)] = m_ispReprocessingBufferMgr;

    if (m_parameters->isReprocessing3aaIspOTF() == false) {
        ret = factory->setBufferManagerToPipe(tempBufferManager, pipeId);
        if (ret != NO_ERROR) {
            CLOGE2("Failed to setBufferManagerToPipe into pipeId %d", pipeId);
            return ret;
        }
        tempBufferManager = ispBufferManager;
        pipeId = PIPE_ISP_REPROCESSING;
    }

    tempBufferManager[factory->getNodeType(PIPE_ISP_REPROCESSING)] = m_ispReprocessingBufferMgr;
    tempBufferManager[factory->getNodeType(PIPE_ISPC_REPROCESSING)] = m_yuvCaptureBufferMgr;

    if (m_parameters->isReprocessingIspMcscOTF() == false) {
        ret = factory->setBufferManagerToPipe(tempBufferManager, pipeId);
        if (ret != NO_ERROR) {
            CLOGE2("Failed to setBufferManagerToPipe into pipeId %d", pipeId);
            return ret;
        }
        tempBufferManager = mcscBufferManager;
        pipeId = PIPE_MCSC_REPROCESSING;
    }

    tempBufferManager[factory->getNodeType(PIPE_MCSC0_REPROCESSING)] = m_yuvCaptureReprocessingBufferMgr;
    tempBufferManager[factory->getNodeType(PIPE_HWFC_JPEG_SRC_REPROCESSING)] = m_yuvCaptureReprocessingBufferMgr;
    tempBufferManager[factory->getNodeType(PIPE_HWFC_THUMB_SRC_REPROCESSING)] = m_thumbnailBufferMgr;
    /* Dummy buffer manager */
    tempBufferManager[factory->getNodeType(PIPE_HWFC_THUMB_DST_REPROCESSING)] = m_thumbnailBufferMgr;

    if (m_streamManager->findStream(HAL_STREAM_ID_JPEG) == true) {
        ret = m_streamManager->getStream(HAL_STREAM_ID_JPEG, &stream);
        if (ret != NO_ERROR)
            CLOGE2("Failed to getStream from streamMgr. HAL_STREAM_ID_JPEG");

        ret = stream->getBufferManager(&bufferMgr);
        if (ret != NO_ERROR)
            CLOGE2("Failed to getBufferMgr. HAL_STREAM_ID_JPEG");

        tempBufferManager[factory->getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING)] = bufferMgr;
    }

    ret = factory->setBufferManagerToPipe(tempBufferManager, pipeId);
    if (ret != NO_ERROR) {
        CLOGE2("Failed to setBufferManagerToPipe into pipeId %d", pipeId);
        return ret;
    }

    /* Setting OutputFrameQ/FrameDoneQ to Pipe */
    if(m_parameters->getUsePureBayerReprocessing()) {
        // Pure bayer reprocessing
        pipeId = PIPE_3AA_REPROCESSING;
    } else if (m_parameters->isUseYuvReprocessing() == true) {
        // YUV reprocessing
        pipeId = PIPE_MCSC_REPROCESSING;
    } else {
        // Dirty bayer reprocessing
        pipeId = PIPE_ISP_REPROCESSING;
    }

    /* TODO : Consider the M2M Reprocessing Scenario */
    if (m_parameters->isUseYuvReprocessingForThumbnail() == true) {
        factory->setOutputFrameQToPipe(m_reprocessingDoneQ, pipeId);
    } else {
        factory->setOutputFrameQToPipe(m_pipeCaptureFrameDoneQ, pipeId);
        factory->setFrameDoneQToPipe(m_reprocessingDoneQ, pipeId);
    }

    return ret;
}

void ExynosCamera3::m_updateCropRegion(struct camera2_shot_ext *shot_ext)
{
    int sensorMaxW = 0, sensorMaxH = 0;

    m_parameters->getMaxSensorSize(&sensorMaxW, &sensorMaxH);

    shot_ext->shot.ctl.scaler.cropRegion[0] = ALIGN_DOWN(shot_ext->shot.ctl.scaler.cropRegion[0], 2);
    shot_ext->shot.ctl.scaler.cropRegion[1] = ALIGN_DOWN(shot_ext->shot.ctl.scaler.cropRegion[1], 2);
    shot_ext->shot.ctl.scaler.cropRegion[2] = ALIGN_UP(shot_ext->shot.ctl.scaler.cropRegion[2], 2);
    shot_ext->shot.ctl.scaler.cropRegion[3] = ALIGN_UP(shot_ext->shot.ctl.scaler.cropRegion[3], 2);

    /* 1. Check the validation of the crop size(width x height).
     * The crop size must be smaller than sensor max size.
     */
    if (sensorMaxW < (int) shot_ext->shot.ctl.scaler.cropRegion[2]
        || sensorMaxH < (int)shot_ext->shot.ctl.scaler.cropRegion[3]) {
        CLOGE2("Invalid Crop Size(%d, %d), sensorMax(%d, %d)",
                shot_ext->shot.ctl.scaler.cropRegion[2],
                shot_ext->shot.ctl.scaler.cropRegion[3],
                sensorMaxW, sensorMaxH);
        shot_ext->shot.ctl.scaler.cropRegion[2] = sensorMaxW;
        shot_ext->shot.ctl.scaler.cropRegion[3] = sensorMaxH;
    }

    /* 2. Check the validation of the crop offset.
     * Offset coordinate + width or height must be smaller than sensor max size.
     */
    if ((int)(shot_ext->shot.ctl.scaler.cropRegion[0]) < 0) {
        CLOGE2("Invalid Crop Region, offsetX(%d), Change to 0",
                shot_ext->shot.ctl.scaler.cropRegion[0]);
        shot_ext->shot.ctl.scaler.cropRegion[0] = 0;
    }

    if ((int)(shot_ext->shot.ctl.scaler.cropRegion[1]) < 0) {
        CLOGE2("Invalid Crop Region, offsetY(%d), Change to 0",
                shot_ext->shot.ctl.scaler.cropRegion[1]);
        shot_ext->shot.ctl.scaler.cropRegion[1] = 0;
    }

    if (sensorMaxW < (int) shot_ext->shot.ctl.scaler.cropRegion[0]
            + (int) shot_ext->shot.ctl.scaler.cropRegion[2]) {
        CLOGE2("Invalid Crop Region, offsetX(%d), width(%d) sensorMaxW(%d)",
                shot_ext->shot.ctl.scaler.cropRegion[0],
                shot_ext->shot.ctl.scaler.cropRegion[2],
                sensorMaxW);
        shot_ext->shot.ctl.scaler.cropRegion[0] = sensorMaxW - shot_ext->shot.ctl.scaler.cropRegion[2];
    }

    if (sensorMaxH < (int) shot_ext->shot.ctl.scaler.cropRegion[1]
            + (int) shot_ext->shot.ctl.scaler.cropRegion[3]) {
        CLOGE2("Invalid Crop Region, offsetY(%d), height(%d) sensorMaxH(%d)",
                shot_ext->shot.ctl.scaler.cropRegion[1],
                shot_ext->shot.ctl.scaler.cropRegion[3],
                sensorMaxH);
        shot_ext->shot.ctl.scaler.cropRegion[1] = sensorMaxH - shot_ext->shot.ctl.scaler.cropRegion[3];
    }

    m_parameters->setCropRegion(
            shot_ext->shot.ctl.scaler.cropRegion[0],
            shot_ext->shot.ctl.scaler.cropRegion[1],
            shot_ext->shot.ctl.scaler.cropRegion[2],
            shot_ext->shot.ctl.scaler.cropRegion[3]);
}

status_t ExynosCamera3::m_updateJpegControlInfo(const struct camera2_shot_ext *shot_ext)
{
    status_t ret = NO_ERROR;
    if (shot_ext == NULL) {
        CLOGE("ERR(%s[%d]):shot_ext is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    ret = m_parameters->checkJpegQuality(shot_ext->shot.ctl.jpeg.quality);
    if (ret != NO_ERROR)
        CLOGE("ERR(%s[%d]):Failed to checkJpegQuality. quality %d",
                __FUNCTION__, __LINE__, shot_ext->shot.ctl.jpeg.quality);
    ret = m_parameters->checkThumbnailSize(
            shot_ext->shot.ctl.jpeg.thumbnailSize[0],
            shot_ext->shot.ctl.jpeg.thumbnailSize[1]);
    if (ret != NO_ERROR)
        CLOGE("ERR(%s[%d]):Failed to checkThumbnailSize. size %dx%d",
                __FUNCTION__, __LINE__,
                shot_ext->shot.ctl.jpeg.thumbnailSize[0],
                shot_ext->shot.ctl.jpeg.thumbnailSize[1]);
    ret = m_parameters->checkThumbnailQuality(shot_ext->shot.ctl.jpeg.thumbnailQuality);
    if (ret != NO_ERROR)
        CLOGE("ERR(%s[%d]):Failed to checkThumbnailQuality. quality %d",
                __FUNCTION__, __LINE__,
                shot_ext->shot.ctl.jpeg.thumbnailQuality);
    return ret;
}
status_t ExynosCamera3::m_generateFrame(int32_t frameCount, ExynosCamera3FrameFactory *factory, List<ExynosCameraFrame *> *list, Mutex *listLock, ExynosCameraFrame **newFrame)
{
    status_t ret = OK;
    *newFrame = NULL;

    CLOGV2("(%d)", frameCount);
    if (frameCount >= 0) {
        ret = m_searchFrameFromList(list, listLock, frameCount, newFrame);
        if (ret < 0) {
            CLOGE2("searchFrameFromList fail");
            return INVALID_OPERATION;
        }
    }

    if (*newFrame == NULL) {
        *newFrame = factory->createNewFrame(frameCount);
        if (*newFrame == NULL) {
            CLOGE2("newFrame is NULL");
            return UNKNOWN_ERROR;
        }
        listLock->lock();
        list->push_back(*newFrame);
        listLock->unlock();
    }

    return ret;
}

status_t ExynosCamera3::m_setupEntity(uint32_t pipeId, ExynosCameraFrame *newFrame, ExynosCameraBuffer *srcBuf, ExynosCameraBuffer *dstBuf)
{
    status_t ret = OK;
    entity_buffer_state_t entityBufferState;

    CLOGV2("pipeId : %d", pipeId);
    /* set SRC buffer */
    ret = newFrame->getSrcBufferState(pipeId, &entityBufferState);
    if (ret < 0) {
        CLOGE2("getSrcBufferState fail, pipeId(%d), ret(%d)", pipeId, ret);
        return ret;
    }

    if (entityBufferState == ENTITY_BUFFER_STATE_REQUESTED) {
        ret = m_setSrcBuffer(pipeId, newFrame, srcBuf);
        if (ret < 0) {
            CLOGE2("m_setSrcBuffer fail, pipeId(%d), ret(%d)", pipeId, ret);
            return ret;
        }
    }

    /* set DST buffer */
    ret = newFrame->getDstBufferState(pipeId, &entityBufferState);
    if (ret < 0) {
        CLOGE2("getDstBufferState fail, pipeId(%d), ret(%d)", pipeId, ret);
        return ret;
    }

    if (entityBufferState == ENTITY_BUFFER_STATE_REQUESTED) {
        ret = m_setDstBuffer(pipeId, newFrame, dstBuf);
        if (ret < 0) {
            CLOGE2("m_setDstBuffer fail, pipeId(%d), ret(%d)", pipeId, ret);
            return ret;
        }
    }

    ret = newFrame->setEntityState(pipeId, ENTITY_STATE_PROCESSING);
    if (ret < 0) {
        CLOGE2("setEntityState(ENTITY_STATE_PROCESSING) fail, pipeId(%d), ret(%d)", pipeId, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCamera3::m_setSrcBuffer(uint32_t pipeId, ExynosCameraFrame *newFrame, ExynosCameraBuffer *buffer)
{
    status_t ret = OK;
    int bufIndex = -1;
    ExynosCameraBufferManager *bufferMgr = NULL;
    ExynosCameraBuffer srcBuf;

    CLOGV2("pipeId : %d", pipeId);
    if (buffer == NULL) {
        buffer = &srcBuf;

        ret = m_getBufferManager(pipeId, &bufferMgr, SRC_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE2("getBufferManager(SRC) fail, pipeId(%d), ret(%d)", pipeId, ret);
            return ret;
        }

        if (bufferMgr == NULL) {
            CLOGE2("buffer manager is NULL, pipeId(%d)", pipeId);
            return BAD_VALUE;
        }

        /* get buffers */
        ret = bufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, buffer);
        if (ret < 0) {
            CLOGE2("getBuffer fail, pipeId(%d), frameCount(%d), ret(%d)", pipeId, newFrame->getFrameCount(), ret);
            return ret;
        }
    }

    /* set buffers */
    ret = newFrame->setSrcBuffer(pipeId, *buffer);
    if (ret < 0) {
        CLOGE2("setSrcBuffer fail, pipeId(%d), ret(%d)", pipeId, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCamera3::m_setDstBuffer(uint32_t pipeId, ExynosCameraFrame *newFrame, ExynosCameraBuffer *buffer)
{
    status_t ret = OK;
    int bufIndex = -1;
    ExynosCameraBufferManager *bufferMgr = NULL;
    ExynosCameraBuffer dstBuf;

    CLOGV2("pipeId : %d", pipeId);
    if (buffer == NULL) {
        buffer = &dstBuf;

        ret = m_getBufferManager(pipeId, &bufferMgr, DST_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE2("getBufferManager(DST) fail, pipeId(%d), ret(%d)", pipeId, ret);
            return ret;
        }

        if (bufferMgr == NULL) {
            CLOGE2("buffer manager is NULL, pipeId(%d)", pipeId);
            return BAD_VALUE;
        }

        /* get buffers */
        ret = bufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, buffer);
        if (ret < 0) {
            CLOGE2("getBuffer fail, pipeId(%d), frameCount(%d), ret(%d)", pipeId, newFrame->getFrameCount(), ret);
            return ret;
        }
    }

    /* set buffers */
    ret = newFrame->setDstBuffer(pipeId, *buffer);
    if (ret < 0) {
        CLOGE2("setDstBuffer fail, pipeId(%d), ret(%d)", pipeId, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCamera3::m_setSrcBuffer(uint32_t pipeId, ExynosCameraFrame *newFrame, ExynosCameraBuffer *buffer, ExynosCameraBufferManager *bufMgr)
{
    status_t ret = OK;
    int bufIndex = -1;
    ExynosCameraBuffer srcBuf;

    CLOGV2("pipeId : %d", pipeId);
    if (bufMgr == NULL) {

        ret = m_getBufferManager(pipeId, &bufMgr, SRC_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE2("getBufferManager(SRC) fail, pipeId(%d), ret(%d)", pipeId, ret);
            return ret;
        }

        if (bufMgr == NULL) {
            CLOGE2("buffer manager is NULL, pipeId(%d)", pipeId);
            return BAD_VALUE;
        }

    }

    /* get buffers */
    ret = bufMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, buffer);
    if (ret < 0) {
        CLOGE2("getBuffer fail, pipeId(%d), frameCount(%d), ret(%d)", pipeId, newFrame->getFrameCount(), ret);
        return ret;
    }

    /* set buffers */
    ret = newFrame->setSrcBuffer(pipeId, *buffer);
    if (ret < 0) {
        CLOGE2("setSrcBuffer fail, pipeId(%d), ret(%d)", pipeId, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCamera3::m_setDstBuffer(uint32_t pipeId, ExynosCameraFrame *newFrame, ExynosCameraBuffer *buffer, ExynosCameraBufferManager *bufMgr)
{
    status_t ret = OK;
    int bufIndex = -1;
    ExynosCameraBuffer dstBuf;

    CLOGD2("pipeId : %d", pipeId);
    if (bufMgr == NULL) {

        ret = m_getBufferManager(pipeId, &bufMgr, DST_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE2("getBufferManager(DST) fail, pipeId(%d), ret(%d)", pipeId, ret);
            return ret;
        }

        if (bufMgr == NULL) {
            CLOGE2("buffer manager is NULL, pipeId(%d)", pipeId);
            return BAD_VALUE;
        }
    }

    /* get buffers */
    ret = bufMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, buffer);
    if (ret < 0) {
        CLOGE2("getBuffer fail, pipeId(%d), frameCount(%d), ret(%d)", pipeId, newFrame->getFrameCount(), ret);
        return ret;
    }

    /* set buffers */
    ret = newFrame->setDstBuffer(pipeId, *buffer);
    if (ret < 0) {
        CLOGE2("setDstBuffer fail, pipeId(%d), ret(%d)", pipeId, ret);
        return ret;
    }

    return ret;
}

/* This function reset buffer state of pipeId.
 * Some pipes are shared by multi stream. In this case, we need reset buffer for using PIPE again.
 */
status_t ExynosCamera3::m_resetBufferState(uint32_t pipeId, ExynosCameraFrame *frame)
{
    status_t ret = NO_ERROR;
    entity_buffer_state_t bufState = ENTITY_BUFFER_STATE_NOREQ;

    if (frame == NULL) {
        CLOGE2("frame is NULL");
        ret = BAD_VALUE;
        goto ERR;
    }

    ret = frame->getSrcBufferState(pipeId, &bufState);
    if (ret < 0) {
        CLOGE2("getSrcBufferState fail, pipeId(%d), ret(%d)", pipeId, ret);
        goto ERR;
    }

    if (bufState != ENTITY_BUFFER_STATE_NOREQ && bufState != ENTITY_BUFFER_STATE_INVALID) {
        frame->setSrcBufferState(pipeId, ENTITY_BUFFER_STATE_REQUESTED);
    } else {
        CLOGW2("SrcBufferState is not COMPLETE, fail to reset buffer state, pipeId(%d), state(%d)", pipeId, bufState);
        ret = INVALID_OPERATION;
        goto ERR;
    }


    ret = frame->getDstBufferState(pipeId, &bufState);
    if (ret < 0) {
        CLOGE2("getDstBufferState fail, pipeId(%d), ret(%d)", pipeId, ret);
        goto ERR;
    }

    if (bufState != ENTITY_BUFFER_STATE_NOREQ && bufState != ENTITY_BUFFER_STATE_INVALID) {
        ret = frame->setDstBufferState(pipeId, ENTITY_BUFFER_STATE_REQUESTED);
        if (ret != NO_ERROR)
            CLOGE2("setDstBufferState fail, pipeId(%d), ret(%d)", pipeId, ret);
    } else {
        CLOGW2("DstBufferState is not COMPLETE, fail to reset buffer state, pipeId(%d), state(%d)", pipeId, bufState);
        ret = INVALID_OPERATION;
        goto ERR;
    }

ERR:
    return ret;
}

status_t ExynosCamera3::m_getBufferManager(uint32_t pipeId, ExynosCameraBufferManager **bufMgr, uint32_t direction)
{
    status_t ret = NO_ERROR;
    ExynosCameraBufferManager **bufMgrList[2] = {NULL};

    switch (pipeId) {
    case PIPE_FLITE:
        bufMgrList[0] = NULL;
        bufMgrList[1] = &m_bayerBufferMgr;
        break;
    case PIPE_3AA_ISP:
        bufMgrList[0] = &m_3aaBufferMgr;
        bufMgrList[1] = &m_ispBufferMgr;
        break;
    case PIPE_3AA:
        bufMgrList[0] = &m_3aaBufferMgr;
        bufMgrList[1] = &m_ispBufferMgr;
        break;
    case PIPE_3AC:
        bufMgrList[0] = NULL;
        bufMgrList[1] = &m_bayerBufferMgr;
        break;
    case PIPE_ISP:
        bufMgrList[0] = &m_ispBufferMgr;
        bufMgrList[1] = &m_internalScpBufferMgr;
        break;
    case PIPE_SCP:
        bufMgrList[0] = NULL;
        bufMgrList[1] = &m_internalScpBufferMgr;
        break;
    case PIPE_VRA:
        bufMgrList[0] = &m_vraBufferMgr;
        bufMgrList[1] = NULL;
        break;
    case PIPE_GSC:
        bufMgrList[0] = &m_internalScpBufferMgr;
        bufMgrList[1] = NULL;
        break;
    case PIPE_GSC_VIDEO:
        bufMgrList[0] = &m_internalScpBufferMgr;
        bufMgrList[1] = NULL;
        break;
    case PIPE_GSC_PICTURE:
        bufMgrList[0] = &m_yuvCaptureBufferMgr;
        bufMgrList[1] = &m_gscBufferMgr;
        break;
    case PIPE_JPEG:
    case PIPE_JPEG_REPROCESSING:
        bufMgrList[0] = NULL;
        bufMgrList[1] = NULL;
        break;
    case PIPE_3AA_REPROCESSING:
        bufMgrList[0] = &m_fliteBufferMgr;
        bufMgrList[1] = &m_ispReprocessingBufferMgr;
        break;
    case PIPE_ISP_REPROCESSING:
        bufMgrList[0] = &m_ispReprocessingBufferMgr;
        bufMgrList[1] = &m_yuvCaptureReprocessingBufferMgr;
        break;
    case PIPE_ISPC_REPROCESSING:
    case PIPE_SCC_REPROCESSING:
        bufMgrList[0] = NULL;
        bufMgrList[1] = &m_yuvCaptureReprocessingBufferMgr;
        break;
    case PIPE_MCSC_REPROCESSING:
        bufMgrList[0] = &m_yuvCaptureBufferMgr;
        bufMgrList[1] = &m_yuvCaptureReprocessingBufferMgr;
        break;
    case PIPE_GSC_REPROCESSING:
        bufMgrList[0] = &m_yuvCaptureReprocessingBufferMgr;
        bufMgrList[1] = &m_gscBufferMgr;
        break;

    default:
        CLOGE2("Unknown pipeId(%d)", pipeId);
        bufMgrList[0] = NULL;
        bufMgrList[1] = NULL;
        ret = BAD_VALUE;
        break;
    }

    *bufMgr = *bufMgrList[direction];
    return ret;
}

status_t ExynosCamera3::m_createIonAllocator(ExynosCameraIonAllocator **allocator)
{
    status_t ret = NO_ERROR;
    int retry = 0;
    do {
        retry++;
        CLOGI2("try(%d) to create IonAllocator", retry);
        *allocator = new ExynosCameraIonAllocator();
        ret = (*allocator)->init(false);
        if (ret < 0)
            CLOGE2("create IonAllocator fail (retryCount=%d)", retry);
        else {
            CLOGD2("m_createIonAllocator success (allocator=%p)", *allocator);
            break;
        }
    } while ((ret < 0) && (retry < 3));

    if ((ret < 0) && (retry >=3)) {
        CLOGE2("create IonAllocator fail (retryCount=%d)", retry);
        ret = INVALID_OPERATION;
    }

    return ret;
}

status_t ExynosCamera3::m_createBufferManager(ExynosCameraBufferManager **bufferManager, const char *name, buffer_manager_type type)
{
    status_t ret = NO_ERROR;

    if (m_ionAllocator == NULL) {
        ret = m_createIonAllocator(&m_ionAllocator);
        if (ret < 0)
            CLOGE2("m_createIonAllocator fail");
        else
            CLOGD2("m_createIonAllocator success");
    }

    *bufferManager = ExynosCameraBufferManager::createBufferManager(type);
    (*bufferManager)->create(name, m_cameraId, m_ionAllocator);

    CLOGD2("BufferManager(%s) created", name);

    return ret;
}

status_t ExynosCamera3::m_createInternalBufferManager(ExynosCameraBufferManager **bufferManager, const char *name)
{
    return m_createBufferManager(bufferManager, name, BUFFER_MANAGER_ION_TYPE);
}

status_t ExynosCamera3::m_createServiceBufferManager(ExynosCameraBufferManager **bufferManager, const char *name)
{
    return m_createBufferManager(bufferManager, name, BUFFER_MANAGER_SERVICE_GRALLOC_TYPE);
}

status_t ExynosCamera3::m_convertingStreamToShotExt(ExynosCameraBuffer *buffer, struct camera2_node_output *outputInfo)
{
/* TODO: HACK: Will be removed, this is driver's job */
    status_t ret = NO_ERROR;
    int bayerFrameCount = 0;
    camera2_shot_ext *shot_ext = NULL;
    camera2_stream *shot_stream = NULL;

    shot_stream = (struct camera2_stream *)buffer->addr[1];
    bayerFrameCount = shot_stream->fcount;
    outputInfo->cropRegion[0] = shot_stream->output_crop_region[0];
    outputInfo->cropRegion[1] = shot_stream->output_crop_region[1];
    outputInfo->cropRegion[2] = shot_stream->output_crop_region[2];
    outputInfo->cropRegion[3] = shot_stream->output_crop_region[3];

    memset(buffer->addr[1], 0x0, sizeof(struct camera2_shot_ext));

    shot_ext = (struct camera2_shot_ext *)buffer->addr[1];
    shot_ext->shot.dm.request.frameCount = bayerFrameCount;

    return ret;
}

bool ExynosCamera3::m_selectBayerThreadFunc()
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;
    ExynosCameraFrame *frame = NULL;
    ExynosCameraBuffer bayerBuffer;
    ExynosCameraBufferManager *bufferMgr = NULL;
    camera2_shot_ext *shot_ext = NULL;
    camera2_shot_ext updateDmShot;
    struct camera2_node_output output_crop_info;
    camera2_node_group node_group_info;
    uint32_t pipeID = 0;
    uint32_t frameCount = 0;
    ExynosRect ratioCropSize;
    int pictureW = 0, pictureH = 0;

    ret = m_selectBayerQ->waitAndPopProcessQ(&frame);
    if (ret != NO_ERROR) {
        if (ret == TIMED_OUT)
            CLOGW2("Wait timeout");
        else
            CLOGE2("Failed to waitAndPopProcessQ. ret %d", ret);

        goto CLEAN;
    } else if (frame == NULL) {
        CLOGE2("frame is NULL!!");
        goto CLEAN;
    }

    frameCount = frame->getFrameCount();

    if (frame->getFrameCapture() == false) {
        CLOGW2("frame is not capture frame. frameCount %d", frameCount);
        goto CLEAN;
    }

    CLOGV2("Start to select Bayer. frameCount %d", frameCount);

    /* Get bayer buffer based on current reprocessing mode */
    switch(m_parameters->getReprocessingBayerMode()) {
        case REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON:
        case REPROCESSING_BAYER_MODE_PURE_DYNAMIC:
            CLOGD2("REPROCESSING_BAYER_MODE_PURE. isRawCapture %d",
                    frame->getFrameServiceBayer());

            if (frame->getFrameZsl() || frame->getFrameServiceBayer())
                ret = m_getBayerServiceBuffer(frame, &bayerBuffer);
            else
                ret = m_getBayerBuffer(m_getBayerPipeId(), frameCount, &bayerBuffer, m_captureSelector);
            if (ret != NO_ERROR) {
                CLOGE2("Failed to get bayer buffer. frameCount %d useServiceBayerBuffer %d",
                        frameCount, frame->getFrameZsl());
                goto CLEAN;
            }

            shot_ext = (struct camera2_shot_ext *)(bayerBuffer.addr[bayerBuffer.planeCount-1]);
            if (shot_ext == NULL) {
                CLOGE2("shot_ext from pure bayer buffer is NULL");
                break;
            }

            ret = frame->storeDynamicMeta(shot_ext);
            if (ret < 0) {
                CLOGE2("storeDynamicMeta fail ret(%d)", ret);
                goto CLEAN;
            }

            ret = frame->storeUserDynamicMeta(shot_ext);
            if (ret < 0) {
                CLOGE2("storeUserDynamicMeta fail ret(%d)", ret);
                goto CLEAN;
            }

            break;
        case REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON:
        case REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC:
            CLOGD2("REPROCESSING_BAYER_MODE_DIRTY%s. isRawCapture %d",
                    (m_parameters->getReprocessingBayerMode()
                        == REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC) ? "_DYNAMIC" : "_ALWAYS_ON",
                    frame->getFrameServiceBayer());

            if (frame->getFrameZsl()/* || frame->getFrameServiceBayer()*/)
                ret = m_getBayerServiceBuffer(frame, &bayerBuffer);
            else
                ret = m_getBayerBuffer(PIPE_3AA, frameCount, &bayerBuffer, m_captureSelector, &updateDmShot);
            if (ret != NO_ERROR) {
                CLOGE2("Failed to get bayer buffer. frameCount %d useServiceBayerBuffer %d",
                        frameCount, frame->getFrameZsl());
                goto CLEAN;
            }

            /* Set perframe size of dirty reprocessing */
            /* TODO: HACK: Will be removed, this is driver's job */
            ret = m_convertingStreamToShotExt(&bayerBuffer, &output_crop_info);
            if (ret != NO_ERROR) {
                CLOGE2("shot_stream to shot_ext converting fail, ret(%d)", ret);
                goto CLEAN;
            }

            memset(&node_group_info, 0x0, sizeof(camera2_node_group));
            if (m_parameters->isUseYuvReprocessingForThumbnail() == true) {
                m_parameters->getThumbnailSize(&pictureW, &pictureH);
                if (pictureW <= 0 || pictureH <= 0)
                    m_parameters->getPictureSize(&pictureW, &pictureH);
            } else {
                m_parameters->getPictureSize(&pictureW, &pictureH);
            }

            if (m_parameters->isUseYuvReprocessing() == true)
                frame->getNodeGroupInfo(&node_group_info, PERFRAME_INFO_YUV_REPROCESSING_MCSC);
            else
                frame->getNodeGroupInfo(&node_group_info, PERFRAME_INFO_DIRTY_REPROCESSING_ISP);

            /* Leader */
            setLeaderSizeToNodeGroupInfo(&node_group_info,
                                         output_crop_info.cropRegion[0],
                                         output_crop_info.cropRegion[1],
                                         ALIGN_UP(output_crop_info.cropRegion[2], CAMERA_ISP_ALIGN),
                                         output_crop_info.cropRegion[3]);

            /* Capture */
            ret = getCropRectAlign(
                    node_group_info.leader.input.cropRegion[2],
                    node_group_info.leader.input.cropRegion[3],
                    pictureW, pictureH,
                    &ratioCropSize.x, &ratioCropSize.y, &ratioCropSize.w, &ratioCropSize.h,
                    CAMERA_MCSC_ALIGN, 2, 0, 1.0);
            if (ret != NO_ERROR) {
                CLOGE2("getCropRectAlign failed. MCSC in_crop %dx%d, MCSC(picture) out_size %dx%d",
                        node_group_info.leader.input.cropRegion[2],
                        node_group_info.leader.input.cropRegion[3],
                        pictureW, pictureH);

                ratioCropSize.x = 0;
                ratioCropSize.y = 0;
                ratioCropSize.w = node_group_info.leader.input.cropRegion[2];
                ratioCropSize.h = node_group_info.leader.input.cropRegion[3];
            }

            setCaptureCropNScaleSizeToNodeGroupInfo(&node_group_info,
                                                    PERFRAME_REPROCESSING_SCC_POS,
                                                    ratioCropSize.x, ratioCropSize.y,
                                                    ratioCropSize.w, ratioCropSize.h,
                                                    pictureW, pictureH);

            if (m_parameters->isUseYuvReprocessing() == true)
                frame->storeNodeGroupInfo(&node_group_info, PERFRAME_INFO_YUV_REPROCESSING_MCSC);
            else
                frame->storeNodeGroupInfo(&node_group_info, PERFRAME_INFO_DIRTY_REPROCESSING_ISP);

            ret = frame->storeDynamicMeta(&updateDmShot);
            if (ret < 0) {
                CLOGE2("storeDynamicMeta fail ret(%d)", ret);
                goto CLEAN;
            }

            ret = frame->storeUserDynamicMeta(&updateDmShot);
            if (ret < 0) {
                CLOGE2("storeUserDynamicMeta fail ret(%d)", ret);
                goto CLEAN;
            }

            shot_ext = (struct camera2_shot_ext *)(bayerBuffer.addr[bayerBuffer.planeCount-1]);
            frame->getMetaData(shot_ext);

            break;
        default:
            CLOGE2("bayer mode is not valid(%d)", m_parameters->getReprocessingBayerMode());
            goto CLEAN;
    }

    CLOGD2("meta_shot_ext->shot.dm.request.frameCount : %d", getMetaDmRequestFrameCount(shot_ext));

    /* Get pipeId for the first entity in reprocessing frame */
    pipeID = frame->getFirstEntity()->getPipeId();
    CLOGD2("Reprocessing stream first pipe ID %d", pipeID);

    /* Check available buffer */
    ret = m_getBufferManager(pipeID, &bufferMgr, DST_BUFFER_DIRECTION);
    if (ret != NO_ERROR) {
        CLOGE2("Failed to getBufferManager, ret %d", ret);
        goto CLEAN;
    } else if (bufferMgr == NULL) {
        CLOGE2("BufferMgr is NULL. pipeId %d", pipeID);
        goto CLEAN;
    }

    ret = m_checkBufferAvailable(pipeID, bufferMgr);
    if (ret != NO_ERROR) {
        CLOGE2("Waiting buffer timeout, PipeID %d, ret %d", pipeID, ret);
        goto CLEAN;
    }

    ret = m_setupEntity(pipeID, frame, &bayerBuffer, NULL);
    if (ret < 0) {
        CLOGE2("setupEntity fail, bayerPipeId(%d), ret(%d)", pipeID, ret);
        goto CLEAN;
    }

    m_captureQ->pushProcessQ(&frame);

    return true;

CLEAN:
    if (frame != NULL) {
        frame->frameUnlock();
        ret = m_removeFrameFromList(&m_captureProcessList, &m_captureProcessLock, frame);
        if (ret != NO_ERROR)
            CLOGE2("Failed to remove frame from m_captureProcessList. frameCount %d ret %d",
                    frame->getFrameCount(), ret);

        frame->printEntity();
        CLOGD2("Delete frame from m_captureProcessList. frameCount %d", frame->getFrameCount());
        frame->decRef();
        m_frameMgr->deleteFrame(frame);
        frame = NULL;
    }
    return true;

}

bool ExynosCamera3::m_captureThreadFunc()
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;
    ExynosCameraFrame *frame = NULL;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraRequest *request = NULL;
    ExynosCamera3FrameFactory *factory = NULL;

    int pipeId = 0;
    int bufPipeId = 0;
    bool isSrc = false;
    int retryCount = 3;
    uint32_t frameCount = 0;

    ret = m_captureQ->waitAndPopProcessQ(&frame);
    if (ret != NO_ERROR) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW2("Wait timeout");
        } else {
            CLOGE2("Failed to wait&pop captureQ, ret %d", ret);
            /* TODO: doing exception handling */
        }
        goto CLEAN;
    } else if (frame == NULL) {
        CLOGE2("frame is NULL!!");
        goto CLEAN;
    }

    m_captureStreamThread->run(PRIORITY_DEFAULT);

    frameCount = frame->getFrameCount();

    CLOGV2("frame frameCount(%d)", frameCount);

    request = m_requestMgr->getServiceRequest(frameCount);
    factory = request->getFrameFactory(HAL_STREAM_ID_JPEG);

    if (m_parameters->isReprocessing() == true) {
        if (m_parameters->isUseYuvReprocessingForThumbnail() == true) {
            ret = m_handleThumbnailReprocessingFrame(frame);
            if (ret != NO_ERROR) {
                CLOGE2("m_handleThumbnailReprocessingFrame fail, ret(%d)", ret);
                goto CLEAN;
            }
        } else {
            pipeId = frame->getFirstEntity()->getPipeId();

            factory->pushFrameToPipe(&frame, pipeId);
            factory->startThread(pipeId);

            /* Wait reprocesisng done */
            CLOGI2("Wait reprocessing done. frameCount %d", frameCount);
            do {
                ret = m_reprocessingDoneQ->waitAndPopProcessQ(&frame);
            } while (ret == TIMED_OUT && retryCount-- > 0);

            if (ret != NO_ERROR)
                CLOGW2("Failed to waitAndPopProcessQ to reprocessingDoneQ. ret %d", ret);
        }
    } else {
        if (m_parameters->is3aaIspOtf() == true) {
            pipeId = PIPE_3AA;
            if (m_parameters->isUsing3acForIspc() == true)
                bufPipeId = PIPE_3AC;
            else
                bufPipeId = PIPE_ISPC;
        } else {
            pipeId = PIPE_ISP;
            bufPipeId = PIPE_ISPC;
        }

        newFrame = m_sccCaptureSelector->selectDynamicFrames(1, pipeId, isSrc, retryCount, factory->getNodeType(bufPipeId));

        if (newFrame == NULL) {
            CLOGE2("newFrame is NULL");
            goto CLEAN;
        }

        if (frameCount != newFrame->getFrameCount())
            CLOGW2("Selected frame count is not match! frame(%d), selected(%d)",
                    frameCount, newFrame->getFrameCount());

        m_captureProcessList.push_back(newFrame);

        ret = m_handleIsChainDone(newFrame);
        if (ret != NO_ERROR) {
            CLOGE2("m_handleIsChainDone fail, ret(%d)", ret);
            goto CLEAN;
        }
    }

    if (m_captureQ->getSizeOfProcessQ() > 0)
        return true;
    else
        return false;

CLEAN:
    if (frame != NULL) {
        if (m_parameters->isReprocessing() == true) {
            frame->frameUnlock();
            ret = m_removeFrameFromList(&m_captureProcessList, &m_captureProcessLock, frame);
            if (ret != NO_ERROR)
                CLOGE2("remove frame from m_captureProcessList fail, ret(%d)", ret);
        }

        frame->printEntity();
        CLOGD2("Picture frame delete(%d)", frame->getFrameCount());
        frame->decRef();
        m_frameMgr->deleteFrame(frame);
        frame = NULL;
    }

    CLOGI2("captureThreadFunc fail, remaining count(%d)", m_sccCaptureSelector->getHoldCount());

    if (m_captureQ->getSizeOfProcessQ() > 0)
        return true;
    else
        return false;
}

status_t ExynosCamera3::m_getBayerServiceBuffer(ExynosCameraFrame *frame, ExynosCameraBuffer *buffer)
{
    status_t ret = NO_ERROR;
    ExynosCameraRequest *request = NULL;
    ExynosCameraBufferManager *bufferMgr = NULL;

    request = m_requestMgr->getServiceRequest(frame->getFrameCount());
    if (request != NULL) {
        camera3_stream_buffer_t *stream_buffer = request->getInputBuffer();
        buffer_handle_t *handle = stream_buffer->buffer;
        int bufIndex = -1;

        m_bayerBufferMgr->getIndexByHandle(handle, &bufIndex);
        if (bufIndex < 0) {
            CLOGE2("getIndexByHandle is fail(fcount:%d / handle:%p)", frame->getFrameCount(), handle);
            ret = BAD_VALUE;
        } else {
            ret = m_bayerBufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_SERVICE, buffer);
            CLOGI2("service bayer selected(fcount:%d / handle:%p / idx:%d / ret:%d)", frame->getFrameCount(), handle, bufIndex, ret);
        }
    } else {
        CLOGE2("request if NULL(fcount:%d)", frame->getFrameCount());
        ret = BAD_VALUE;
    }

    return ret;
}

status_t ExynosCamera3::m_getBayerBuffer(uint32_t pipeId, uint32_t frameCount, ExynosCameraBuffer *buffer, ExynosCameraFrameSelector *selector, camera2_shot_ext *updateDmShot)
{
    status_t ret = NO_ERROR;
    bool isSrc = false;
    int retryCount = 30; /* 200ms x 30 */
    camera2_shot_ext *shot_ext = NULL;
    camera2_stream *shot_stream = NULL;
    ExynosCameraFrame *inListFrame = NULL;
    ExynosCameraFrame *bayerFrame = NULL;

    if (m_parameters->isReprocessing() == false || selector == NULL) {
        CLOGE2("INVALID_OPERATION, isReprocessing(%s) or bayerFrame is NULL",
                m_parameters->isReprocessing() ? "True" : "False");
        ret = INVALID_OPERATION;
        goto CLEAN;
    }

    selector->setWaitTime(200000000);
    bayerFrame = selector->selectCaptureFrames(1, frameCount, pipeId, isSrc, retryCount, 0);
    if (bayerFrame == NULL) {
        CLOGE2("bayerFrame is NULL");
        ret = INVALID_OPERATION;
        goto CLEAN;
    }

    ret = bayerFrame->getDstBuffer(pipeId, buffer);
    if (ret < 0) {
        CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", pipeId, ret);
        goto CLEAN;
    }

    if (m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON ||
        m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_PURE_DYNAMIC) {
        shot_ext = (struct camera2_shot_ext *)buffer->addr[1];
        CLOGD2("Selected frame count(hal : %d / driver : %d)",
            bayerFrame->getFrameCount(), shot_ext->shot.dm.request.frameCount);
    } else if (m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON ||
        m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC) {
        if (updateDmShot == NULL) {
            CLOGE2("updateDmShot is NULL");
            goto CLEAN;
        }

        while(retryCount > 0) {
            if(bayerFrame->getMetaDataEnable() == false) {
                CLOGD2("Waiting for update jpeg metadata failed (%d), retryCount(%d)", ret, retryCount);
            } else {
                break;
            }
            retryCount--;
            usleep(DM_WAITING_TIME);
        }

        /* update meta like pure bayer */
        bayerFrame->getUserDynamicMeta(updateDmShot);
        bayerFrame->getDynamicMeta(updateDmShot);

        shot_stream = (struct camera2_stream *)buffer->addr[1];
        CLOGD2("Selected fcount(hal : %d / driver : %d)", bayerFrame->getFrameCount(), shot_stream->fcount);
    } else {
        CLOGE2("reprocessing is not valid pipeId(%d), ret(%d)", pipeId, ret);
        goto CLEAN;
    }

CLEAN:

    if (bayerFrame != NULL) {
        bayerFrame->frameUnlock();

        ret = m_searchFrameFromList(&m_processList, &m_processLock, bayerFrame->getFrameCount(), &inListFrame);
        if (ret < 0) {
            CLOGE2("searchFrameFromList fail");
        } else {
            CLOGD2("Selected frame(%d) complete, Delete", bayerFrame->getFrameCount());
            bayerFrame->decRef();
            m_frameMgr->deleteFrame(bayerFrame);
            bayerFrame = NULL;
        }
    }

    return ret;
}

bool ExynosCamera3::m_captureStreamThreadFunc(void)
{
    status_t ret = 0;
    ExynosCameraFrame *frame = NULL;
    ExynosCameraFrameEntity *entity = NULL;
    ExynosCameraBuffer buffer;
    ExynosCameraRequest* request = NULL;
    struct camera2_shot_ext *shot_ext = NULL;
    uint32_t frameCount = 0;
    int pipeId = -1;

    ret = m_pipeCaptureFrameDoneQ->waitAndPopProcessQ(&frame);
    if (ret != NO_ERROR) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW2("wait timeout");
        } else {
            CLOGE2("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        goto FUNC_EXIT;
    }

    if (frame == NULL) {
        CLOGE2("frame is NULL");
        goto FUNC_EXIT;
    }

    entity = frame->getFrameDoneEntity();
    if (entity == NULL) {
        CLOGE2("current entity is NULL");
        /* TODO: doing exception handling */
        goto FUNC_EXIT;
    }

    frameCount = frame->getFrameCount();
    pipeId = entity->getPipeId();

    CLOGD2("captureStream frame->frameCnt(%d), entityID(%d)", frameCount, pipeId);

    switch(pipeId) {
    case PIPE_3AA_REPROCESSING:
    case PIPE_ISP_REPROCESSING:
    case PIPE_MCSC_REPROCESSING:
        ret = frame->getSrcBuffer(pipeId, &buffer);
        if (ret != NO_ERROR) {
            CLOGE2("Failed to getSrcBuffer, pipeId %d, ret %d", pipeId, ret);
            goto FUNC_EXIT;
        }

        shot_ext = (struct camera2_shot_ext *) buffer.addr[buffer.planeCount - 1];
        if (shot_ext == NULL) {
            CLOGE2("shot_ext is NULL. pipeId %d frameCount %d", pipeId, frameCount);
            goto FUNC_EXIT;
        }

        if (m_parameters->getUsePureBayerReprocessing() == true) {
            ret = m_pushResult(frameCount, shot_ext);
            if (ret != NO_ERROR) {
                CLOGE2("Failed to pushResult. framecount %d ret %d", frameCount, ret);
                goto FUNC_EXIT;
            }
        } else {
            // In dirty bayer case, the meta is updated if the current request
            // is reprocessing only(i.e. Internal frame is created on preview path).
            // Preview path will update the meta if the current request have preview frame
            ExynosCameraRequest* request = m_requestMgr->getServiceRequest(frameCount);
            if (request == NULL) {
                CLOGE2("getServiceRequest failed");
            } else {
                if(request->getNeedInternalFrame() == true) {
                    ret = m_pushResult(frameCount, shot_ext);
                    if (ret != NO_ERROR) {
                        CLOGE2("Failed to pushResult. framecount %d ret %d", frameCount, ret);
                        goto FUNC_EXIT;
                    }
                }
            }
        }

        ret = frame->storeDynamicMeta(shot_ext);
        if (ret != NO_ERROR) {
            CLOGE2("Failed to storeUserDynamicMeta, requestKey %d, ret %d", request->getKey(), ret);
            goto FUNC_EXIT;
        }

        ret = frame->storeUserDynamicMeta(shot_ext);
        if (ret != NO_ERROR) {
            CLOGE2("Failed to storeUserDynamicMeta, requestKey %d, ret %d", request->getKey(), ret);
            goto FUNC_EXIT;
        }

        CLOGV2("REPROCESSING Done. dm.request.frameCount %d frameCount %d",
                getMetaDmRequestFrameCount(shot_ext), frameCount);

        if (m_parameters->isUseYuvReprocessing() == true) {
            ret = m_putBuffers(m_yuvCaptureBufferMgr, buffer.index);
            if (ret != NO_ERROR)
                CLOGE2("Failed to putBuffer to yuvCaptureBufferMgr, bufferIndex %d", buffer.index);
        } else if (frame->getFrameServiceBayer() == false) {
            ret = m_putBuffers(m_fliteBufferMgr, buffer.index);
            if (ret != NO_ERROR)
                CLOGE2("Failed to putBuffer to fliteBufferMgr, bufferIndex %d", buffer.index);
        }

        /* Handle yuv capture buffer */
        ret = m_handleYuvCaptureFrame(frame);
        if (ret != NO_ERROR) {
            CLOGE2("Failed to handleYuvCaptureFrame. pipeId %d ret %d", pipeId, ret);
            goto FUNC_EXIT;
        }

        /* Continue to JPEG processing stage in HWFC mode */
        if (m_parameters->isHWFCEnabled() == false)
            break;
    case PIPE_JPEG:
    case PIPE_JPEG_REPROCESSING:
        ret = m_handleJpegFrame(frame);
        if (ret != NO_ERROR) {
            CLOGE2("m_handleJpegFrame fail, pipeId(%d), ret(%d)", pipeId, ret);
            goto FUNC_EXIT;
        }
        break;
    case PIPE_GSC:
    case PIPE_GSC_VIDEO:
    case PIPE_GSC_PICTURE:
    case PIPE_GSC_REPROCESSING:
        ret = m_handleScalerDone(frame);
        if (ret != NO_ERROR) {
            CLOGE2("m_handleScalerDone fail, pipeId(%d) ret(%d)", pipeId, ret);
            goto FUNC_EXIT;
        }
        break;
    default:
        CLOGE2("Invalid pipe ID (%d)", pipeId);
        break;
    }

    ret = frame->setEntityState(pipeId, ENTITY_STATE_COMPLETE);
    if (ret < 0) {
        CLOGE2("setEntityState fail, pipeId(%d), state(%d), ret(%d)",
                pipeId, ENTITY_STATE_COMPLETE, ret);
        goto FUNC_EXIT;
    }

    if ((pipeId == PIPE_JPEG
         || m_parameters->isHWFCEnabled() == true
         || pipeId == PIPE_JPEG_REPROCESSING)
        && frame->isComplete() == true) {
        List<ExynosCameraFrame *> *list = NULL;
        Mutex *listLock = NULL;
#if defined(ENABLE_FULL_FRAME)
        list = &m_processList;
        listLock = &m_processLock;
#else
        list = &m_captureProcessList;
        listLock = &m_captureProcessLock;
#endif
       // TODO:decide proper position
        CLOGV2("frame complete, count(%d)", frameCount);
        ret = m_removeFrameFromList(list, listLock, frame);
        if (ret < 0) {
            CLOGE2("remove frame from processList fail, ret(%d)", ret);
        }

        frame->decRef();
        m_frameMgr->deleteFrame(frame);
        frame = NULL;
    }

FUNC_EXIT:
        Mutex::Autolock l(m_captureProcessLock);
        if (m_captureProcessList.size() > 0)
            return true;
        else
            return false;
}

status_t ExynosCamera3::m_handleIsChainDone(ExynosCameraFrame *frame)
{
    status_t ret = 0;
    ExynosCameraRequest* request = NULL;
    ExynosCameraBufferManager *bufferMgr = NULL;
    ExynosCameraBuffer srcBuffer;
    ExynosCameraBuffer dstBuffer;
    int bufIndex = -1;
    ExynosCamera3FrameFactory *factory = NULL;
    int pipeId_src = -1;
    int pipeId_gsc = -1;
    int pipeId_jpeg = -1;

    bool isSrc = false;
    float zoomRatio = 0.0F;
    struct camera2_stream *shot_stream = NULL;
    int pictureW = 0, pictureH = 0, pictureFormat = 0;
    ExynosRect srcRect, dstRect;
    int type = CAMERA3_MSG_SHUTTER;
    struct camera2_shot_ext *temp_ext = new struct camera2_shot_ext;
    struct camera2_shot_ext *result_ext = new struct camera2_shot_ext;

    memset(temp_ext, 0x00, sizeof(struct camera2_shot_ext));
    memset(result_ext, 0x00, sizeof(struct camera2_shot_ext));

    request = m_requestMgr->getServiceRequest(frame->getFrameCount());
    factory = request->getFrameFactory(HAL_STREAM_ID_JPEG);

    zoomRatio = m_parameters->getZoomRatio(m_parameters->getZoomLevel()) / 1000;

    ///////////////////////////////////////////////////////////
    if (m_parameters->isReprocessing() == true) {
        /* We are using only PIPE_ISP_REPROCESSING */
        pipeId_src = PIPE_ISP_REPROCESSING;
        pipeId_gsc = PIPE_GSC_REPROCESSING;
        pipeId_jpeg = PIPE_JPEG_REPROCESSING;
        isSrc = true;
    } else if(m_parameters->isUsing3acForIspc() == true){
        pipeId_src = PIPE_3AA;
        pipeId_gsc = PIPE_GSC_PICTURE;
        pipeId_jpeg = PIPE_JPEG;
    } else {
#if defined(ENABLE_FULL_FRAME)
        pipeId_src = PIPE_ISP;
        pipeId_gsc = PIPE_GSC_PICTURE;
        pipeId_jpeg = PIPE_JPEG;
#else
        switch (getCameraId()) {
            case CAMERA_ID_FRONT:
                pipeId_src = PIPE_ISP;
                pipeId_gsc = PIPE_GSC_PICTURE;
                break;
            default:
                CLOGE2("Current picture mode is not yet supported, CameraId(%d), reprocessing(%d)",
                        getCameraId(), m_parameters->isReprocessing());
                break;
        }
        pipeId_jpeg = PIPE_JPEG;
#endif
    }
    ///////////////////////////////////////////////////////////

    if (m_parameters->needGSCForCapture(getCameraId()) == true) {
        if (m_parameters->isReprocessing() == true)
            ret = frame->getDstBuffer(pipeId_src, &srcBuffer, factory->getNodeType(PIPE_ISPC_REPROCESSING));
        else if (m_parameters->isUsing3acForIspc() == true)
            ret = frame->getDstBuffer(pipeId_src, &srcBuffer, factory->getNodeType(PIPE_3AC));
        else
            ret = frame->getDstBuffer(pipeId_src, &srcBuffer, factory->getNodeType(PIPE_ISPC));

        if (ret < 0)
            CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", pipeId_src, ret);

        shot_stream = (struct camera2_stream *)(srcBuffer.addr[srcBuffer.planeCount-1]);
        if (shot_stream != NULL) {
            CLOGD2("fcount(%d), rcount(%d), findex(%d), fvalid(%d)",
                    shot_stream->fcount, shot_stream->rcount, shot_stream->findex, shot_stream->fvalid);

            CLOGD2("(%d %d %d %d)(%d %d %d %d)",
                    shot_stream->input_crop_region[0],
                    shot_stream->input_crop_region[1],
                    shot_stream->input_crop_region[2],
                    shot_stream->input_crop_region[3],
                    shot_stream->output_crop_region[0],
                    shot_stream->output_crop_region[1],
                    shot_stream->output_crop_region[2],
                    shot_stream->output_crop_region[3]);
        } else {
            CLOGE2("shot_stream is NULL");
            return INVALID_OPERATION;
        }

        /* should change size calculation code in pure bayer */
#if 0
        if (shot_stream != NULL) {
            ret = m_calcPictureRect(&srcRect, &dstRect);
            ret = newFrame->setSrcRect(pipeId_gsc, &srcRect);
            ret = newFrame->setDstRect(pipeId_gsc, &dstRect);
        }
#else
        m_parameters->getPictureSize(&pictureW, &pictureH);
#if defined(ENABLE_FULL_FRAME)
        pictureFormat = m_parameters->getHwPreviewFormat();
#else
        pictureFormat = m_parameters->getHwPictureFormat();
#endif

        srcRect.x = shot_stream->output_crop_region[0];
        srcRect.y = shot_stream->output_crop_region[1];
        srcRect.w = shot_stream->output_crop_region[2];
        srcRect.h = shot_stream->output_crop_region[3];
        srcRect.fullW = shot_stream->output_crop_region[2];
        srcRect.fullH = shot_stream->output_crop_region[3];
        srcRect.colorFormat = pictureFormat;
#if 0
        dstRect.x = 0;
        dstRect.y = 0;
        dstRect.w = srcRect.w;
        dstRect.h = srcRect.h;
        dstRect.fullW = srcRect.fullW;
        dstRect.fullH = srcRect.fullH;
        dstRect.colorFormat = JPEG_INPUT_COLOR_FMT;

#else
        dstRect.x = 0;
        dstRect.y = 0;
        dstRect.w = pictureW;
        dstRect.h = pictureH;
        dstRect.fullW = pictureW;
        dstRect.fullH = pictureH;
        dstRect.colorFormat = JPEG_INPUT_COLOR_FMT;
#endif
        ret = getCropRectAlign(srcRect.w,  srcRect.h,
                pictureW,   pictureH,
                &srcRect.x, &srcRect.y,
                &srcRect.w, &srcRect.h,
                2, 2, 0, zoomRatio);

        ret = frame->setSrcRect(pipeId_gsc, &srcRect);
        ret = frame->setDstRect(pipeId_gsc, &dstRect);
#endif

        CLOGD2("srcRect size (%d, %d, %d, %d %d %d)",
                srcRect.x, srcRect.y, srcRect.w, srcRect.h, srcRect.fullW, srcRect.fullH);
        CLOGD2("dstRect size (%d, %d, %d, %d %d %d)",
                dstRect.x, dstRect.y, dstRect.w, dstRect.h, dstRect.fullW, dstRect.fullH);

        ret = m_setupEntity(pipeId_gsc, frame, &srcBuffer, NULL);

        if (ret < 0) {
            CLOGE2("setupEntity fail, pipeId(%d), ret(%d)", pipeId_jpeg, ret);
        }

        // Update frame's exposureTime from request result's exposureTime
        frame->getMetaData(temp_ext);
        if ((temp_ext->shot.dm.sensor.exposureTime) == 0) {
            request = m_requestMgr->getServiceRequest(frame->getFrameCount());
            ret = request->getResultShot(result_ext);
            if (ret < 0) {
                CLOGE2("getResultShot fail, pipeId(%d), ret(%d)", pipeId_jpeg, ret);
            }
            temp_ext->shot.dm.sensor.exposureTime = result_ext->shot.dm.sensor.exposureTime;
            frame->setMetaData(temp_ext);
        }

        factory->pushFrameToPipe(&frame, pipeId_gsc);
        factory->setOutputFrameQToPipe(m_pipeCaptureFrameDoneQ, pipeId_gsc);

    } else { /* m_parameters->needGSCForCapture(getCameraId()) == false */
        ret = frame->getDstBuffer(pipeId_src, &srcBuffer);
#if defined(ENABLE_FULL_FRAME)
        if (ret < 0)
            CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", pipeId_src, ret);

        ret = m_setupEntity(pipeId_jpeg, frame, &srcBuffer, NULL);
        if (ret < 0) {
            CLOGE2("setupEntity fail, pipeId(%d), ret(%d)", pipeId_jpeg, ret);
        }
#else
        if (ret < 0) {
            CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", pipeId_src, ret);
        } else {
            /* getting jpeg buffer from service buffer */
            ExynosCameraStream *stream = NULL;

            int streamId = 0;
            m_streamManager->getStream(HAL_STREAM_ID_JPEG, &stream);

            if (stream == NULL) {
                CLOGE2("stream is NULL");
                return INVALID_OPERATION;
            }

            stream->getID(&streamId);
            stream->getBufferManager(&bufferMgr);
            CLOGV2("streamId(%d), bufferMgr(%p)", streamId, bufferMgr);
            /* bufferMgr->printBufferQState(); */
            ret = bufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &dstBuffer);
            if (ret < 0) {
                CLOGE2("bufferMgr getBuffer fail, frameCount(%d), ret(%d)", frame->getFrameCount(), ret);
            }
        }
        ret = m_setupEntity(pipeId_jpeg, frame, &srcBuffer, &dstBuffer);
        if (ret < 0) {
            CLOGE2("setupEntity fail, pipeId(%d), ret(%d)", pipeId_jpeg, ret);
        }
#endif
        factory->setOutputFrameQToPipe(m_pipeCaptureFrameDoneQ, pipeId_jpeg);
        factory->pushFrameToPipe(&frame, pipeId_jpeg);
    }

    return ret;
}

status_t ExynosCamera3::m_handleScalerDone(ExynosCameraFrame *frame)
{
    status_t ret = 0;
    ExynosCameraRequest* request = NULL;
    int pipeId_gsc = -1;
    int pipeId_dst = -1;
    ExynosCameraBufferManager *bufferMgr = NULL;
    ExynosCameraBuffer buffer;
    ExynosCameraBuffer srcBuffer;
    ExynosCameraBuffer dstBuffer;
    int bufIndex = -1;
    ExynosCamera3FrameFactory *factory = NULL;

    unsigned int completeBufferCount = 0;

    request = m_requestMgr->getServiceRequest(frame->getFrameCount());
    factory = request->getFrameFactory(HAL_STREAM_ID_JPEG);

    //////////////////////////////////////////////////////////
    /* TODO: Need to decision pipeId both current and next */
    if (m_parameters->isReprocessing() == true) {
        if (m_parameters->needGSCForCapture(getCameraId()) == true) {
            pipeId_gsc = PIPE_GSC_REPROCESSING;
        } else {
            pipeId_gsc = (m_parameters->isOwnScc(getCameraId()) == true) ? PIPE_SCC_REPROCESSING : PIPE_ISPC_REPROCESSING;
        }
        pipeId_dst = PIPE_JPEG_REPROCESSING;
    } else {
        if (m_parameters->needGSCForCapture(getCameraId()) == true) {
            pipeId_gsc = PIPE_GSC_PICTURE;
        } else {
            if (m_parameters->isOwnScc(getCameraId()) == true) {
                pipeId_gsc = PIPE_SCC;
            } else {
                pipeId_gsc = PIPE_ISPC;
            }
        }
        pipeId_dst = PIPE_JPEG;
    }
    //////////////////////////////////////////////////////////

#if !defined(ENABLE_FULL_FRAME)
    /* handle src buffer of gsc */
    ret = m_getBufferManager(pipeId_gsc, &bufferMgr, SRC_BUFFER_DIRECTION);
    if (ret < 0) {
        CLOGE2("getBufferManager(DST) fail, pipeId(%d), ret(%d)", pipeId_gsc, ret);
        return ret;
    }

    ret = frame->getSrcBuffer(pipeId_gsc, &buffer);
    if (ret < 0) {
        CLOGE2("getSrcBuffer fail, pipeId(%d), ret(%d)", pipeId_gsc, ret);
    }

    ret = m_putBuffers(bufferMgr, buffer.index);
    if (ret < 0) {
        CLOGE2("m_putBuffers fail, ret(%d)", ret);
        /* TODO: doing exception handling */
    }
#else
    /*
     * internal scp buffer should be return to buffer manager before creating jpeg output buffer.
     * so, we should compare complete buffer count + 1
     */
    CLOGV2("request->getNumOfOutputBuffer(%d) request->getCompleteBuffers(%d)", request->getNumOfOutputBuffer(), request->getCompleteBufferCount());

    request->increaseDuplicateBufferCount();
    completeBufferCount = request->getNumOfOutputBuffer();
    if (frame->getFrameCapture() == true)
        completeBufferCount --;
    if (frame->getFrameServiceBayer() == true)
        completeBufferCount --;

    if(completeBufferCount == (unsigned int)request->getDuplicateBufferCount()) {
        /* handle src buffer of gsc */
        ret = m_getBufferManager(pipeId_gsc, &bufferMgr, SRC_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE2("getBufferManager(DST) fail, pipeId(%d), ret(%d)", pipeId_gsc, ret);
            return ret;
        }

        ret = frame->getSrcBuffer(pipeId_gsc, &buffer);
        if (ret < 0) {
            CLOGE2("getSrcBuffer fail, pipeId(%d), ret(%d)", pipeId_gsc, ret);
        }

        CLOGV2("Internal Scp Buffer is returned index(%d)frameCount(%d)", buffer.index, frame->getFrameCount());

        ret = m_putBuffers(bufferMgr, buffer.index);
        if (ret < 0) {
            CLOGE2("m_putBuffers fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        m_captureResultDoneCondition.signal();
    }
#endif

    /*
     * handle dst buffer of gsc
     *  - pipeId_dst : Indicate to pipe ID for next pipe.
     *                 -1 means final result for this stream.
     */
    if (pipeId_dst >= 0) {
        ExynosCameraStream *stream = NULL;

        int streamId = 0;
        m_streamManager->getStream(HAL_STREAM_ID_JPEG, &stream);

        if (stream == NULL) {
            CLOGE2("stream is NULL");
            return INVALID_OPERATION;
        }

        stream->getID(&streamId);
        CLOGD2("streamID(%d)", streamId);

        stream->getBufferManager(&bufferMgr);
        CLOGV2("bufferMgr(%p)", bufferMgr);
        /* bufferMgr->printBufferQState(); */
        ret = bufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &dstBuffer);
        if (ret < 0) {
            CLOGE2("bufferMgr getBuffer fail, frameCount(%d), ret(%d)", frame->getFrameCount(), ret);
        }
        ret = m_getBufferManager(pipeId_gsc, &bufferMgr, DST_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE2("getBufferManager(DST) fail, pipeId(%d), ret(%d)", pipeId_gsc, ret);
            return ret;
        }
        /* bufferMgr->printBufferQState(); */
        ret = frame->getDstBuffer(pipeId_gsc, &srcBuffer);
        if (ret < 0) {
            CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", pipeId_gsc, ret);
        }

        ret = m_putBuffers(bufferMgr, srcBuffer.index);
        if (ret < 0) {
            CLOGE2("m_putBuffers fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }

        ret = m_setupEntity(pipeId_dst, frame, &srcBuffer, &dstBuffer);
        if (ret < 0) {
            CLOGE2("setupEntity fail, pipeId(%d), ret(%d)", pipeId_dst, ret);
        }
        factory->setOutputFrameQToPipe(m_pipeCaptureFrameDoneQ, pipeId_dst);
        factory->pushFrameToPipe(&frame, pipeId_dst);
    } else {
        // TODO: send result
    }

    return ret;
}

status_t ExynosCamera3::m_handleThumbnailReprocessingFrame(ExynosCameraFrame *frame)
{
    status_t ret = NO_ERROR;

    ExynosCameraRequest *request = NULL;
    ExynosCamera3FrameFactory *factory = NULL;

    ExynosCameraBufferManager *bufferMgr = NULL;
    ExynosCameraBuffer srcBuffer;
    ExynosCameraBuffer yuvReprocessingBuffer;
    ExynosCameraBuffer thumbnailBuffer;
    struct camera2_shot_ext shot_ext;
    struct camera2_node_output output_crop_info;
    camera2_node_group node_group_info;

    int bufferIndex = -2;
    int pipeId = 0;
    int retryCount = 3;
    uint32_t frameCount = 0;
    bool isNeedThumbnail = false;
    ExynosRect ratioCropSize;
    int pictureW = 0, pictureH = 0;

    srcBuffer.index = -2;
    yuvReprocessingBuffer.index = -2;
    thumbnailBuffer.index = -2;

    frameCount = frame->getFrameCount();
    request = m_requestMgr->getServiceRequest(frameCount);
    factory = request->getFrameFactory(HAL_STREAM_ID_JPEG);

    pipeId = frame->getFirstEntity()->getPipeId();

    factory->pushFrameToPipe(&frame, pipeId);
    factory->startThread(pipeId);

    /* Wait reprocesisng done */
    CLOGI2("Wait reprocessing done. frameCount %d", frameCount);
    do {
        ret = m_reprocessingDoneQ->waitAndPopProcessQ(&frame);
    } while (ret == TIMED_OUT && retryCount-- > 0);

    if (ret != NO_ERROR)
        CLOGW2("Failed to waitAndPopProcessQ to reprocessingDoneQ. ret %d", ret);

    frame->getMetaData(&shot_ext);
    isNeedThumbnail = (shot_ext.shot.ctl.jpeg.thumbnailSize[0] > 0
                       && shot_ext.shot.ctl.jpeg.thumbnailSize[1] > 0) ? true : false;

    if (isNeedThumbnail == false) {
        m_pipeCaptureFrameDoneQ->pushProcessQ(&frame);
        return ret;
    }

    ret = frame->setEntityState(pipeId, ENTITY_STATE_COMPLETE);
    if (ret != NO_ERROR) {
        CLOGE2("setEntityState(ENTITY_STATE_PROCESSING) fail, pipeId(%d), ret(%d)", pipeId, ret);
        return ret;
    }

    CLOGI2("Thumbnail Reprocessing done");

    frame->setMetaDataEnable(true);

    /* Copy thumbnail image to thumbnail buffer */
    ret = frame->getDstBuffer(pipeId, &yuvReprocessingBuffer, factory->getNodeType(PIPE_MCSC0_REPROCESSING));
    if (ret != NO_ERROR) {
        CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", pipeId, ret);
        goto CLEAN;
    }

    ret = m_thumbnailBufferMgr->getBuffer(&bufferIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &thumbnailBuffer);
    if (ret != NO_ERROR) {
        CLOGE2("get thumbnail Buffer fail, ret(%d)", ret);
        goto CLEAN;
    }

    memcpy(thumbnailBuffer.addr[0], yuvReprocessingBuffer.addr[0], thumbnailBuffer.size[0]);

    /* Put buffers */
    ret = m_putBuffers(m_thumbnailBufferMgr, thumbnailBuffer.index);
    if (ret != NO_ERROR) {
        CLOGE2("ThumbnailBuffer putBuffer fail, index(%d), ret(%d)", thumbnailBuffer.index, ret);
        goto CLEAN;
    }

    /* Put reprocessing dst buffer */
    ret = m_getBufferManager(pipeId, &bufferMgr, DST_BUFFER_DIRECTION);
    if (ret != NO_ERROR) {
        CLOGE2("getBufferManager fail, ret(%d)", ret);
        goto CLEAN;
    }

    if (bufferMgr != NULL) {
        ret = m_putBuffers(bufferMgr, yuvReprocessingBuffer.index);
        if (ret != NO_ERROR) {
            CLOGE2("DstBuffer putBuffer fail, index(%d), ret(%d)", yuvReprocessingBuffer.index, ret);
            goto CLEAN;
        }
    }

    /* get src buffer */
    ret = frame->getSrcBuffer(pipeId, &srcBuffer);
    if (ret != NO_ERROR) {
        CLOGE2("getSrcBuffer fail, pipeId(%d), ret(%d)", pipeId, ret);
        goto CLEAN;
    }

    memset(&node_group_info, 0x0, sizeof(camera2_node_group));
    frame->getNodeGroupInfo(&node_group_info, PERFRAME_INFO_YUV_REPROCESSING_MCSC);

    /* Delete new frame */
    CLOGI2("Reprocessing frame for thumbnail delete(%d)", frame->getFrameCount());

    ret = m_removeFrameFromList(&m_captureProcessList, &m_captureProcessLock, frame);
    if (ret != 0)
        CLOGE2("remove frame from processList fail, ret(%d)", ret);

    frame->decRef();
    m_frameMgr->deleteFrame(frame);
    frame = NULL;

    ret = m_generateFrame(frameCount, factory, &m_captureProcessList, &m_captureProcessLock, &frame);
    if (ret != NO_ERROR) {
        CLOGE2("m_generateFrame fail");
        return INVALID_OPERATION;
    } else if (frame == NULL) {
        CLOGE2("frame is NULL");
        return INVALID_OPERATION;
    }

    /* Set JPEG request true */
    frame->setRequest(PIPE_MCSC0_REPROCESSING, true);
    if (m_parameters->isHWFCEnabled() == true) {
        frame->setRequest(PIPE_HWFC_JPEG_SRC_REPROCESSING, true);
        frame->setRequest(PIPE_HWFC_JPEG_DST_REPROCESSING, true);
        frame->setRequest(PIPE_HWFC_THUMB_SRC_REPROCESSING, true);
    }

    CLOGV2("generate request framecount %d requestKey %d", frameCount, request->getKey());

    ret = frame->setMetaData(&shot_ext);
    if (ret != NO_ERROR)
        CLOGE2("Set metadata to frame fail, Frame count(%d), ret(%d)",
                frameCount, ret);

    m_parameters->getPictureSize(&pictureW, &pictureH);

    /* Capture */
    ret = getCropRectAlign(
            node_group_info.leader.input.cropRegion[2],
            node_group_info.leader.input.cropRegion[3],
            pictureW, pictureH,
            &ratioCropSize.x, &ratioCropSize.y, &ratioCropSize.w, &ratioCropSize.h,
            CAMERA_MCSC_ALIGN, 2, 0, 1.0);
    if (ret != NO_ERROR) {
        CLOGE2("getCropRectAlign failed. MCSC in_crop %dx%d, MCSC(picture) out_size %dx%d",
                node_group_info.leader.input.cropRegion[2],
                node_group_info.leader.input.cropRegion[3],
                pictureW, pictureH);

        ratioCropSize.x = 0;
        ratioCropSize.y = 0;
        ratioCropSize.w = node_group_info.leader.input.cropRegion[2];
        ratioCropSize.h = node_group_info.leader.input.cropRegion[3];
    }

    setCaptureCropNScaleSizeToNodeGroupInfo(&node_group_info,
                                            PERFRAME_REPROCESSING_SCC_POS,
                                            ratioCropSize.x, ratioCropSize.y,
                                            ratioCropSize.w, ratioCropSize.h,
                                            pictureW, pictureH);

    frame->storeNodeGroupInfo(&node_group_info, PERFRAME_INFO_YUV_REPROCESSING_MCSC);

    /* Get pipeId for the first entity in reprocessing frame */
    pipeId = frame->getFirstEntity()->getPipeId();
    CLOGD2("Reprocessing stream first pipe ID %d", pipeId);

    /* Check available buffer */
    ret = m_getBufferManager(pipeId, &bufferMgr, DST_BUFFER_DIRECTION);
    if (ret != NO_ERROR) {
        CLOGE2("Failed to getBufferManager, ret %d", ret);
        goto CLEAN;
    } else if (bufferMgr == NULL) {
        CLOGE2("BufferMgr is NULL. pipeId %d", pipeId);
        goto CLEAN;
    }

    ret = m_checkBufferAvailable(pipeId, bufferMgr);
    if (ret != NO_ERROR) {
        CLOGE2("Waiting buffer timeout, PipeID %d, ret %d", pipeId, ret);
        goto CLEAN;
    }

    ret = m_setupEntity(pipeId, frame, &srcBuffer, NULL);
    if (ret != NO_ERROR) {
        CLOGE2("setupEntity fail, bayerPipeId(%d), ret(%d)", pipeId, ret);
        goto CLEAN;
    }

    factory->pushFrameToPipe(&frame, pipeId);
    factory->startThread(pipeId);

    /* Wait reprocesisng done */
    CLOGI2("Wait reprocessing done. frameCount %d", frameCount);
    do {
        ret = m_reprocessingDoneQ->waitAndPopProcessQ(&frame);
    } while (ret == TIMED_OUT && retryCount-- > 0);

    if (ret != NO_ERROR)
        CLOGW2("Failed to waitAndPopProcessQ to reprocessingDoneQ. ret %d", ret);

PUSH_FRAME:
    m_pipeCaptureFrameDoneQ->pushProcessQ(&frame);

CLEAN:
    return ret;
}

status_t ExynosCamera3::m_handleYuvCaptureFrame(ExynosCameraFrame *frame)
{
    status_t ret = NO_ERROR;
    ExynosCameraRequest* request = NULL;
    ExynosCameraBufferManager *bufferMgr = NULL;
    ExynosCameraBuffer srcBuffer;
    ExynosCameraBuffer dstBuffer;
    int bufIndex = -1;
    ExynosCamera3FrameFactory *factory = NULL;
    int pipeId_src = -1;
    int pipeId_gsc = -1;
    int pipeId_jpeg = -1;

    bool isSrc = false;
    float zoomRatio = 0.0F;
    struct camera2_stream *shot_stream = NULL;
    int pictureW = 0, pictureH = 0, pictureFormat = 0;
    ExynosRect srcRect, dstRect;

    request = m_requestMgr->getServiceRequest(frame->getFrameCount());
    factory = request->getFrameFactory(HAL_STREAM_ID_JPEG);

    if (m_parameters->isHWFCEnabled() == true) {
        ret = frame->getDstBuffer(PIPE_MCSC_REPROCESSING, &dstBuffer, factory->getNodeType(PIPE_MCSC0_REPROCESSING));
        if (ret != NO_ERROR) {
            CLOGE2("Failed to getDstBuffer. pipeId %d node %d ret %d",
                    PIPE_MCSC_REPROCESSING, PIPE_MCSC0_REPROCESSING, ret);
            return INVALID_OPERATION;
        }

        ret = m_putBuffers(m_yuvCaptureReprocessingBufferMgr, dstBuffer.index);
        if (ret != NO_ERROR) {
            CLOGE2("Failed to putBuffer to m_yuvCaptureBufferMgr. bufferIndex %d",
                    dstBuffer.index);
            return INVALID_OPERATION;
        }

#if 0 /* TODO : Why this makes error? */
        ret = frame->getDstBuffer(PIPE_MCSC_REPROCESSING, &dstBuffer, factory->getNodeType(PIPE_HWFC_THUMB_SRC_REPROCESSING));
        if (ret != NO_ERROR) {
            CLOGE2("Failed to getDstBuffer. pipeId %d node %d ret %d",
                    PIPE_3AA_REPROCESSING, PIPE_MCSC4_REPROCESSING, ret);
            return INVALID_OPERATION;
        }

        ret = m_putBuffers(m_thumbnailBufferMgr, dstBuffer.index);
        if (ret != NO_ERROR) {
            CLOGE2("Failed to putBuffer to m_thumbnailBufferMgr. bufferIndex %d",
                    dstBuffer.index);
            return INVALID_OPERATION;
        }
#endif
    } else {
        zoomRatio = m_parameters->getZoomRatio(m_parameters->getZoomLevel()) / 1000;

        if (m_parameters->isReprocessing() == true) {
            /* We are using only PIPE_ISP_REPROCESSING */
            pipeId_src = PIPE_ISP_REPROCESSING;
            pipeId_gsc = PIPE_GSC_REPROCESSING;
            pipeId_jpeg = PIPE_JPEG_REPROCESSING;
            isSrc = true;
        } else {
#if defined(ENABLE_FULL_FRAME)
            pipeId_src = PIPE_ISP;
            pipeId_gsc = PIPE_GSC_PICTURE;
            pipeId_jpeg = PIPE_JPEG;
#else
            switch (getCameraId()) {
                case CAMERA_ID_FRONT:
                    pipeId_src = PIPE_ISP;
                    pipeId_gsc = PIPE_GSC_PICTURE;
                    break;
                default:
                    CLOGE2("Current picture mode is not yet supported, CameraId(%d), reprocessing(%d)",
                            getCameraId(), m_parameters->isReprocessing());
                    break;
            }
            pipeId_jpeg = PIPE_JPEG;
#endif
        }
        ///////////////////////////////////////////////////////////

        // Thumbnail image is currently not used
        ret = frame->getDstBuffer(pipeId_src, &dstBuffer, factory->getNodeType(PIPE_MCSC4_REPROCESSING));
        if (ret != NO_ERROR) {
            CLOGE2("Failed to getDstBuffer. pipeId %d node %d ret %d",
                    pipeId_src, PIPE_MCSC4_REPROCESSING, ret);
        } else {
            ret = m_putBuffers(m_thumbnailBufferMgr, dstBuffer.index);
            if (ret != NO_ERROR) {
                CLOGE2("Failed to putBuffer to m_thumbnailBufferMgr. bufferIndex %d",
                        dstBuffer.index);
            }
            CLOGI2("INFO(%s[%d]): Thumbnail image disposed at pipeId %d node %d, FrameCnt %d",
                    pipeId_src, PIPE_MCSC4_REPROCESSING, frame->getFrameCount());
        }

        if (m_parameters->needGSCForCapture(getCameraId()) == true) {
            ret = frame->getDstBuffer(pipeId_src, &srcBuffer);
            if (ret < 0)
                CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", pipeId_src, ret);

            shot_stream = (struct camera2_stream *)(srcBuffer.addr[srcBuffer.planeCount-1]);
            if (shot_stream != NULL) {
                CLOGD2("(%d %d %d %d)",
                        shot_stream->fcount,
                        shot_stream->rcount,
                        shot_stream->findex,
                        shot_stream->fvalid);
                CLOGD2("(%d %d %d %d)(%d %d %d %d)",
                        shot_stream->input_crop_region[0],
                        shot_stream->input_crop_region[1],
                        shot_stream->input_crop_region[2],
                        shot_stream->input_crop_region[3],
                        shot_stream->output_crop_region[0],
                        shot_stream->output_crop_region[1],
                        shot_stream->output_crop_region[2],
                        shot_stream->output_crop_region[3]);
            } else {
                CLOGE2("shot_stream is NULL");
                return INVALID_OPERATION;
            }

            /* should change size calculation code in pure bayer */
#if 0
            if (shot_stream != NULL) {
                ret = m_calcPictureRect(&srcRect, &dstRect);
                ret = newFrame->setSrcRect(pipeId_gsc, &srcRect);
                ret = newFrame->setDstRect(pipeId_gsc, &dstRect);
            }
#else
            m_parameters->getPictureSize(&pictureW, &pictureH);
#if defined(ENABLE_FULL_FRAME)
            pictureFormat = m_parameters->getHwPreviewFormat();
#else
            pictureFormat = m_parameters->getHwPictureFormat();
#endif

            srcRect.x = shot_stream->output_crop_region[0];
            srcRect.y = shot_stream->output_crop_region[1];
            srcRect.w = shot_stream->output_crop_region[2];
            srcRect.h = shot_stream->output_crop_region[3];
            srcRect.fullW = shot_stream->output_crop_region[2];
            srcRect.fullH = shot_stream->output_crop_region[3];
            srcRect.colorFormat = pictureFormat;
#if 0
            dstRect.x = 0;
            dstRect.y = 0;
            dstRect.w = srcRect.w;
            dstRect.h = srcRect.h;
            dstRect.fullW = srcRect.fullW;
            dstRect.fullH = srcRect.fullH;
            dstRect.colorFormat = JPEG_INPUT_COLOR_FMT;
#else
            dstRect.x = 0;
            dstRect.y = 0;
            dstRect.w = pictureW;
            dstRect.h = pictureH;
            dstRect.fullW = pictureW;
            dstRect.fullH = pictureH;
            dstRect.colorFormat = JPEG_INPUT_COLOR_FMT;
#endif
            ret = getCropRectAlign(srcRect.w,  srcRect.h,
                    pictureW,   pictureH,
                    &srcRect.x, &srcRect.y,
                    &srcRect.w, &srcRect.h,
                    2, 2, 0, zoomRatio);

            ret = frame->setSrcRect(pipeId_gsc, &srcRect);
            ret = frame->setDstRect(pipeId_gsc, &dstRect);
#endif

            CLOGD2("size (%d, %d, %d, %d %d %d)",
                    srcRect.x, srcRect.y, srcRect.w, srcRect.h, srcRect.fullW, srcRect.fullH);
            CLOGD2("size (%d, %d, %d, %d %d %d)",
                    dstRect.x, dstRect.y, dstRect.w, dstRect.h, dstRect.fullW, dstRect.fullH);

            ret = m_setupEntity(pipeId_gsc, frame, &srcBuffer, NULL);

            if (ret < 0)
                CLOGE2("setupEntity fail, pipeId(%d), ret(%d)", pipeId_jpeg, ret);

            factory->pushFrameToPipe(&frame, pipeId_gsc);
            factory->setOutputFrameQToPipe(m_pipeCaptureFrameDoneQ, pipeId_gsc);

        } else { /* m_parameters->needGSCForCapture(getCameraId()) == false */
            ret = frame->getDstBuffer(pipeId_src, &srcBuffer);
#if defined(ENABLE_FULL_FRAME)
            if (ret < 0)
                CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", pipeId_src, ret);

            ret = m_setupEntity(pipeId_jpeg, frame, &srcBuffer, NULL);
            if (ret < 0) {
                CLOGE2("setupEntity fail, pipeId(%d), ret(%d)",
                        pipeId_jpeg, ret);
            }
#else
            if (ret < 0) {
                CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", pipeId_src, ret);
            } else {
                /* getting jpeg buffer from service buffer */
                ExynosCameraStream *stream = NULL;

                int streamId = 0;
                m_streamManager->getStream(HAL_STREAM_ID_JPEG, &stream);

                if (stream == NULL) {
                    CLOGE2("stream is NULL");
                    return INVALID_OPERATION;
                }

                stream->getID(&streamId);
                stream->getBufferManager(&bufferMgr);
                CLOGV2("streamId(%d), bufferMgr(%p)", streamId, bufferMgr);
                /* bufferMgr->printBufferQState(); */
                ret = bufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &dstBuffer);
                if (ret < 0) {
                    CLOGE2("bufferMgr getBuffer fail, frameCount(%d), ret(%d)",
                            frame->getFrameCount(), ret);
                }
            }
            ret = m_setupEntity(pipeId_jpeg, frame, &srcBuffer, &dstBuffer);
            if (ret < 0)
                CLOGE2("setupEntity fail, pipeId(%d), ret(%d)", pipeId_jpeg, ret);
#endif
            factory->setOutputFrameQToPipe(m_pipeCaptureFrameDoneQ, pipeId_jpeg);
            factory->pushFrameToPipe(&frame, pipeId_jpeg);
        }
    }

    return ret;
}

status_t ExynosCamera3::m_handleJpegFrame(ExynosCameraFrame *frame)
{
    status_t ret = 0;
    int pipeId_jpeg = -1;
    int pipeId_src = -1;
    ExynosCameraRequest *request = NULL;
    ExynosCamera3FrameFactory * factory = NULL;
    ExynosCameraBufferManager *bufferMgr = NULL;
    ExynosCameraBuffer buffer;
    int jpegOutputSize = -1;

    request = m_requestMgr->getServiceRequest(frame->getFrameCount());
    factory = request->getFrameFactory(HAL_STREAM_ID_JPEG);

    //////////////////////////////////////////////////////////
    /* TODO: Need to decision pipeId both current and next */
    if (m_parameters->isReprocessing() == true) {
        if (m_parameters->needGSCForCapture(getCameraId()) == true) {
            pipeId_src = PIPE_GSC_REPROCESSING;
        } else {
            pipeId_src = (m_parameters->isOwnScc(getCameraId()) == true) ? PIPE_SCC_REPROCESSING : PIPE_ISPC_REPROCESSING;
        }
        pipeId_jpeg = PIPE_JPEG_REPROCESSING;
    } else {
        if (m_parameters->needGSCForCapture(getCameraId()) == true) {
            pipeId_src = PIPE_GSC_PICTURE;
        } else {
            if (m_parameters->isOwnScc(getCameraId()) == true) {
                pipeId_src = PIPE_SCC;
            } else {
                pipeId_src = PIPE_ISPC;
            }
        }
        pipeId_jpeg = PIPE_JPEG;
    }
    //////////////////////////////////////////////////////////

    if (m_parameters->isHWFCEnabled() == true) {
        entity_buffer_state_t bufferState = ENTITY_BUFFER_STATE_NOREQ;
        ret = frame->getDstBufferState(PIPE_MCSC_REPROCESSING, &bufferState, factory->getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING));
        if (ret != NO_ERROR) {
            CLOGE2("Failed to getDstBufferState. frameCount %d pipeId %d node %d",
                    frame->getFrameCount(), PIPE_MCSC_REPROCESSING, PIPE_HWFC_JPEG_DST_REPROCESSING);
            return INVALID_OPERATION;
        } else if (bufferState == ENTITY_BUFFER_STATE_ERROR) {
            CLOGE2("Invalid JPEG buffer state. frameCount %d bufferState %d",
                    frame->getFrameCount(), bufferState);
            return INVALID_OPERATION;
        }

        ret = frame->getDstBuffer(PIPE_MCSC_REPROCESSING, &buffer, factory->getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING));
        if (ret != NO_ERROR) {
            CLOGE2("Failed to getDstBuffer. frameCount %d pipeId %d node %d",
                    frame->getFrameCount(), PIPE_MCSC_REPROCESSING, PIPE_HWFC_JPEG_DST_REPROCESSING);
            return INVALID_OPERATION;
        }
    } else {
        ret = frame->setEntityState(pipeId_jpeg, ENTITY_STATE_FRAME_DONE);
        if (ret < 0) {
            CLOGE2("set entity state fail, ret(%d)", ret);
            /* TODO: doing exception handling */
            return OK;
        }

        /* handle src buffer of jpeg */
        ret = m_getBufferManager(pipeId_src, &bufferMgr, DST_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE2("getBufferManager(DST) fail, pipeId(%d), ret(%d)", pipeId_src, ret);
            return ret;
        }

        ret = frame->getSrcBuffer(pipeId_jpeg, &buffer);
        if (ret < 0) {
            CLOGE2("getSrcBuffer fail, pipeId(%d), ret(%d)", pipeId_jpeg, ret);
        }

        ret = m_putBuffers(bufferMgr, buffer.index);
        if (ret < 0) {
            CLOGE2("bufferMgr(DST)->putBuffers() fail, pipeId(%d), ret(%d)", pipeId_src, ret);
        }

        /*
         * handle dst buffer of jpeg
         *   -  JPEG image must be final result of stream.
         */
#if 0
        ret = m_getBufferManager(pipeId_jpeg, &bufferMgr, DST_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE2("getBufferManager(DST) fail, pipeId(%d), ret(%d)", pipeId_jpeg, ret);
            return ret;
        }
#endif

        ret = frame->getDstBuffer(pipeId_jpeg, &buffer);
        if (ret < 0) {
            CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", pipeId_jpeg, ret);
        }
    }

    jpegOutputSize = frame->getJpegSize();
    CLOGI2("jpeg output done, jpeg size(%d)", jpegOutputSize);

    if (jpegOutputSize <= 0) {
        CLOGW2("jpegOutput size(%d) is invalid", jpegOutputSize);
        jpegOutputSize = buffer.size[0];
    }

    /* frame->printEntity(); */
    m_pushJpegResult(frame, jpegOutputSize, &buffer);
    m_captureCount--;

    return ret;
}

status_t ExynosCamera3::m_handleBayerBuffer(ExynosCameraFrame *frame)
{
    status_t ret = NO_ERROR;
    uint32_t bufferDirection = INVALID_BUFFER_DIRECTION;
    uint32_t pipeId = MAX_PIPE_NUM;
    ExynosCameraBuffer buffer;
    ExynosCameraBufferManager *bufferMgr = NULL;
    ExynosCamera3FrameFactory *factory = NULL;

    if (frame == NULL) {
        CLOGE("ERR(%s[%d]):Frame is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* Decide the bayer buffer position and pipe ID */
    if (m_parameters->isFlite3aaOtf() == true) {
        pipeId = PIPE_FLITE;
        bufferDirection = DST_BUFFER_DIRECTION;
        ret = frame->getDstBuffer(pipeId, &buffer);
    } else {
        pipeId = PIPE_3AA;
        bufferDirection = SRC_BUFFER_DIRECTION;
        ret = frame->getSrcBuffer(pipeId, &buffer);
    }

    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):Get bayer buffer failed, framecount(%d), direction(%d), pipeId(%d)",
                __FUNCTION__, __LINE__,
                frame->getFrameCount(), bufferDirection, pipeId);
        return ret;
    }

    /* Check the validation of bayer buffer */
    if (buffer.index < 0) {
        CLOGE("ERR(%s[%d]):Invalid bayer buffer, framecount(%d), direction(%d), pipeId(%d)",
                __FUNCTION__, __LINE__,
                frame->getFrameCount(), bufferDirection, pipeId);
        return INVALID_OPERATION;
    }

    if (pipeId == PIPE_3AA) {
        struct camera2_shot_ext *shot_ext = NULL;

        shot_ext = (struct camera2_shot_ext *) buffer.addr[buffer.planeCount -1];
        CLOGW("WRN(%s[%d]):Timestamp(%lld)", __FUNCTION__, __LINE__, shot_ext->shot.dm.sensor.timeStamp);

        ret = m_updateTimestamp(frame, &buffer);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Failed to update time stamp", __FUNCTION__, __LINE__);
            return ret;
        }
    }

    /* Handle the bayer buffer */
    if (frame->getFrameServiceBayer() == true) {
        /* Raw Capture Request */
        CLOGD("INFO(%s[%d]):Handle service bayer buffer. FLITE-3AA_OTF %d Bayer_Pipe_ID %d Framecount %d",
                __FUNCTION__, __LINE__,
                m_parameters->isFlite3aaOtf(), pipeId, frame->getFrameCount());
        ret = m_sendRawCaptureResult(frame, pipeId, (bufferDirection == SRC_BUFFER_DIRECTION));
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Failed to sendRawCaptureResult. frameCount %d bayerPipeId %d bufferIndex %d",
                    __FUNCTION__, __LINE__,
                    frame->getFrameCount(), pipeId, buffer.index);
            return ret;
        }

        if (m_parameters->isReprocessing() == true && frame->getFrameCapture() == true) {
            CLOGD("DEBUG(%s[%d]):Hold service bayer buffer for reprocessing. frameCount %d bayerPipeId %d bufferIndex %d",
                    __FUNCTION__, __LINE__,
                    frame->getFrameCount(), pipeId, buffer.index);
            ret = m_captureZslSelector->manageFrameHoldListForDynamicBayer(frame);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):Failed to manageFrameHoldListForDynamicBayer to captureZslSelector. frameCount %d bayerPipeId %d bufferIndex %d",
                        __FUNCTION__, __LINE__,
                        frame->getFrameCount(), pipeId, buffer.index);
                return ret;
            }
        }
    } else if (frame->getFrameZsl() == true) {
        /* ZSL Capture Request */
        CLOGV("INFO(%s[%d]):Handle ZSL buffer. FLITE-3AA_OTF %d Bayer_Pipe_ID %d Framecount %d",
                __FUNCTION__, __LINE__,
                m_parameters->isFlite3aaOtf(), pipeId, frame->getFrameCount());
        ret = m_sendZSLCaptureResult(frame, pipeId, (bufferDirection == SRC_BUFFER_DIRECTION));
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Failed to sendZslCaptureResult. frameCount %d bayerPipeId %d",
                    __FUNCTION__, __LINE__,
                    frame->getFrameCount(), pipeId);
        }
    } else if (m_parameters->isReprocessing() == true){
        /* For ZSL Reprocessing */
        CLOGV("INFO(%s[%d]):Hold internal bayer buffer for reprocessing. FLITE-3AA_OTF %d Bayer_Pipe_ID %d Framecount %d",
                __FUNCTION__, __LINE__,
                m_parameters->isFlite3aaOtf(), pipeId, frame->getFrameCount());
        ret = m_captureSelector->manageFrameHoldList(frame, pipeId, (bufferDirection == SRC_BUFFER_DIRECTION));
    } else {
        /* No request for bayer image */
        CLOGV("INFO(%s[%d]):Return internal bayer buffer. FLITE-3AA_OTF %d Bayer_Pipe_ID %d Framecount %d",
                __FUNCTION__, __LINE__,
                m_parameters->isFlite3aaOtf(), pipeId, frame->getFrameCount());
        ret = m_getBufferManager(pipeId, &bufferMgr, bufferDirection);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):getBufferManager failed, pipeId(%d), bufferDirection(%d), ret(%d)",
                    __FUNCTION__, __LINE__, pipeId, bufferDirection, ret);
            return ret;
        }

        ret = m_putBuffers(bufferMgr, buffer.index);
        if (ret !=  NO_ERROR) {
            CLOGE("ERR(%s[%d]):putBuffers failed, pipeId(%d), bufferDirection(%d), bufferIndex(%d), ret(%d)",
                    __FUNCTION__, __LINE__, pipeId, bufferDirection, buffer.index, ret);
            return ret;
        }
    }

    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):Handling bayer buffer failed, isServiceBayer(%d), direction(%d), pipeId(%d)",
                __FUNCTION__, __LINE__,
                frame->getFrameServiceBayer(),
                bufferDirection, pipeId);
    }

    return ret;
}

bool ExynosCamera3::m_previewStreamBayerPipeThreadFunc(void)
{
    status_t ret = 0;
    ExynosCameraFrame *newFrame = NULL;

    ret = m_pipeFrameDoneQ[PIPE_FLITE]->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            /* CLOGW2("wait timeout"); */
        } else {
            CLOGE2("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return true;
    }
    return m_previewStreamFunc(newFrame, PIPE_FLITE);
}

bool ExynosCamera3::m_previewStream3AAPipeThreadFunc(void)
{
    status_t ret = 0;
    ExynosCameraFrame *newFrame = NULL;

    ret = m_pipeFrameDoneQ[PIPE_3AA]->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW2("wait timeout");
        } else {
            CLOGE2("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return true;
    }
    return m_previewStreamFunc(newFrame, PIPE_3AA);
}

bool ExynosCamera3::m_previewStreamISPPipeThreadFunc(void)
{
    status_t ret = 0;
    ExynosCameraFrame *newFrame = NULL;

    ret = m_pipeFrameDoneQ[PIPE_ISP]->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW2("wait timeout");
        } else {
            CLOGE2("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return true;
    }
    return m_previewStreamFunc(newFrame, PIPE_ISP);
}

bool ExynosCamera3::m_previewStreamVRAPipeThreadFunc(void)
{
    status_t ret = 0;
    ExynosCameraFrame *newFrame = NULL;

    ret = m_pipeFrameDoneQ[PIPE_VRA]->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW2("wait timeout");
        } else {
            CLOGE2("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return true;
    }
    return m_previewStreamFunc(newFrame, PIPE_VRA);
}

bool ExynosCamera3::m_previewStreamFunc(ExynosCameraFrame *newFrame, int pipeId)
{
    status_t ret = 0;
    int index = 0;
    //result_info_t *resultInfo = NULL;
    entity_state_t entityState = ENTITY_STATE_COMPLETE;
    int type = CAMERA3_MSG_SHUTTER;
    /* only trace */
    unsigned int halFrameCount = 0;

    if (newFrame != NULL) {
        halFrameCount = newFrame->getFrameCount();
    } else {
        CLOGE2("frame is NULL");
        return true;
    }
    CLOGV2("stream thread : frameCnt(%d) , pipeId(%d)", halFrameCount, pipeId);

    //newFrame->dump();

    /* internal frame */
    if (newFrame->getFrameType() == FRAME_TYPE_INTERNAL) {
        CLOGV2("push to m_internalFrameDoneQ handler : internalFrame frameCnt(%d)", newFrame->getFrameCount());
        m_internalFrameDoneQ->pushProcessQ(&newFrame);
        return true;
    }

    //CLOGV2("stream thread : frameCnt(%d) , pipeId(%d)", halFrameCount, pipeId);

    /* TODO: M2M path is also handled by this */
    ret = m_handlePreviewFrame(newFrame, pipeId);
    if (ret < 0) {
        CLOGE2("handle preview frame fail");
        return ret;
    }
    //CLOGD2("+++++++++++++++++++++++++++++++++++++++++++++");
    //newFrame->dump();
    //CLOGD2("---------------------------------------------");

    if (newFrame->isComplete() == true) {

        m_sendNotify(newFrame->getFrameCount(), type);

        CLOGV2("newFrame->getFrameCount(%d)", newFrame->getFrameCount());

        ret = m_removeFrameFromList(&m_processList, &m_processLock, newFrame);

        if (ret < 0) {
            CLOGE2("remove frame from processList fail, ret(%d)", ret);
        }

        CLOGV2("frame complete, count(%d)", newFrame->getFrameCount());
        newFrame->decRef();
        m_frameMgr->deleteFrame(newFrame);
        newFrame = NULL;
        m_captureResultDoneCondition.signal();
    }

    CLOGV2("stream thread : frameCnt(%d) , pipeId(%d)", halFrameCount, pipeId);

    return true;
}

status_t ExynosCamera3::m_updateTimestamp(ExynosCameraFrame *frame, ExynosCameraBuffer *timestampBuffer, bool flagPushResult)
{
    struct camera2_shot_ext *shot_ext = NULL;
    status_t ret = NO_ERROR;

    /* handle meta data */
    shot_ext = (struct camera2_shot_ext *) timestampBuffer->addr[timestampBuffer->planeCount -1];
    if (shot_ext == NULL) {
        CLOGE("ERR(%s[%d]):shot_ext is NULL. framecount %d buffer %d",
                __FUNCTION__, __LINE__, frame->getFrameCount(), timestampBuffer->index);
        return INVALID_OPERATION;
    }

    uint64_t timeStamp = shot_ext->shot.dm.sensor.timeStamp;
    uint64_t frameDuration = shot_ext->shot.dm.sensor.frameDuration;

    /* HACK: W/A for timeStamp reversion */
    if (timeStamp < (uint64_t)m_lastFrametime) {
        CLOGW2("Timestamp is %lld!, m_lastFrametime(%lld)",
                timeStamp, m_lastFrametime);

        if (frameDuration > 0)
            timeStamp = m_lastFrametime + frameDuration;
        else
            timeStamp = m_lastFrametime + 15000000;
    }

    if (m_lastFrametime > 0
        && timeStamp > (uint64_t)m_lastFrametime + 100000000) { /* 1sec */
            CLOGW2("Timestamp is %lld!, m_lastFrametime(%lld)",
                    timeStamp, m_lastFrametime);
    }

    m_lastFrametime = timeStamp;
    shot_ext->shot.udm.sensor.timeStampBoot = timeStamp;

    if (flagPushResult == true)
        ret = m_pushResult(frame->getFrameCount(), shot_ext);

    return ret;
}

status_t ExynosCamera3::m_handlePreviewFrame(ExynosCameraFrame *frame, int pipeId)
{
    ExynosCameraFrameEntity *entity = NULL;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer buffer;
    ExynosCamera3FrameFactory *factory = NULL;
    ExynosCameraBuffer t3acBuffer;
    int32_t reprocessingBayerMode = m_parameters->getReprocessingBayerMode();

    struct camera2_shot_ext *shot_ext;
    struct camera2_shot_ext meta_shot_ext;
    struct camera2_dm *dm = NULL;
    entity_state_t entityState = ENTITY_STATE_COMPLETE;
    status_t ret = NO_ERROR;
    uint32_t framecount = 0;

    entity = frame->getFrameDoneFirstEntity(pipeId);
    if (entity == NULL) {
        CLOGE2("current entity is NULL pipeID(%d)", pipeId);
        /* TODO: doing exception handling */
        return true;
    }

    CLOGV2("handle preview frame : previewStream frameCnt(%d) entityID(%d)", frame->getFrameCount(), entity->getPipeId());

    factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];

    switch(pipeId) {
    case PIPE_3AA_ISP:
        ret = frame->getSrcBuffer(entity->getPipeId(), &buffer);
        if (ret < 0) {
            CLOGE2("getSrcBuffer fail, pipeId(%d), ret(%d)", entity->getPipeId(), ret);
            return ret;
        }

        /* handle meta data */
        shot_ext = (struct camera2_shot_ext *) buffer.addr[buffer.planeCount -1];
        memset(&meta_shot_ext, 0x00, sizeof(struct camera2_shot_ext));
        memcpy(&meta_shot_ext, shot_ext, sizeof(struct camera2_shot_ext));
        ret = m_pushResult(frame->getFrameCount(), &meta_shot_ext);

        ret = m_putBuffers(m_3aaBufferMgr, buffer.index);
        if (ret < 0) {
            CLOGE2("put Buffer fail");
        }

        CLOGV2("3AA_ISP frameCount(%d) frame.Count(%d)",
            getMetaDmRequestFrameCount((struct camera2_shot_ext *)buffer.addr[buffer.planeCount-1]),
            frame->getFrameCount());

        ret = frame->getDstBuffer(entity->getPipeId(), &buffer);
        if (ret < 0) {
            CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", entity->getPipeId(), ret);
            return ret;
        }

        ret = m_putBuffers(m_ispBufferMgr, buffer.index);
        if (ret < 0) {
            CLOGE2("put Buffer fail");
            break;
        }

        frame->setMetaDataEnable(true);
        dm = &(meta_shot_ext.shot.dm);
        if (dm == NULL) {
            CLOGE2("dm data is null");
            return INVALID_OPERATION;
        }

        break;
    case PIPE_3AA:
        /* Notify ShotDone to mainThread */
        framecount = frame->getFrameCount();
        m_shotDoneQ->pushProcessQ(&framecount);

        /* 1. Handle the buffer from 3AA output node */
        if (m_parameters->isFlite3aaOtf() == true) {
            ExynosCameraBufferManager *bufferMgr = NULL;

            /* Return the dummy buffer */
            ret = frame->getSrcBuffer(entity->getPipeId(), &buffer);
            if (ret < 0) {
                ALOGE("ERR(%s[%d]):getSrcBuffer fail, pipeId(%d), ret(%d)",
                        __FUNCTION__, __LINE__, entity->getPipeId(), ret);
                return ret;
            } else if (buffer.index < 0) {
                ALOGE("ERR(%s[%d]):Invalid buffer index(%d), framecount(%d), pipeId(%d)",
                        __FUNCTION__, __LINE__,
                        buffer.index, frame->getFrameCount(), entity->getPipeId());
                return BAD_VALUE;
            }

            if (m_parameters->is3aaIspOtf() == true) {
                ret = m_updateTimestamp(frame, &buffer);
                if (ret != NO_ERROR) {
                    CLOGE2("[F%d B%d]Failed to updateTimestamp",
                            frame->getFrameCount(), buffer.index);
                    return ret;
                }
            }

            ret = m_putBuffers(m_3aaBufferMgr, buffer.index);
            if (ret < 0) {
                CLOGE2("[F%d]Failed to put buffer %d to 3aaBufferMgr",
                        frame->getFrameCount(), buffer.index);
                break;
            }
        } else {
            ret = m_handleBayerBuffer(frame);
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):Failed to handle bayerBuffer. framecount %d pipeId %d ret %d",
                        __FUNCTION__, __LINE__,
                        frame->getFrameCount(), entity->getPipeId(), ret);
                return ret;
            }
        }

        frame->setMetaDataEnable(true);

        if (frame->getFrameZsl() == true) {
            /* ZSL Capture Request */
            CLOGD2("Handle ZSL buffer. FLITE-3AA_OTF %d Framecount %d",
                    m_parameters->isFlite3aaOtf(), frame->getFrameCount());
            ret = m_sendZSLCaptureResult(frame, PIPE_3AC, false);
            if (ret != NO_ERROR) {
                CLOGE2("Failed to sendZslCaptureResult. frameCount %d",
                        frame->getFrameCount());
            }
        }

        t3acBuffer.index = -1;

        if (frame->getRequest(PIPE_3AC) == true) {
            ret = frame->getDstBuffer(entity->getPipeId(), &t3acBuffer, factory->getNodeType(PIPE_3AC));
            if (ret != NO_ERROR) {
                CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", entity->getPipeId(), ret);
            }
        }

        if (m_parameters->isReprocessing() == true) {
            if (m_captureSelector == NULL) {
                CLOGE2("m_captureSelector is NULL");
                return INVALID_OPERATION;
            }
        } else {
            if (m_sccCaptureSelector == NULL) {
                CLOGE2("m_sccCaptureSelector is NULL");
                return INVALID_OPERATION;
            }
        }

        if (0 <= t3acBuffer.index) {
            if (m_parameters->isUseYuvReprocessing() == true
                || frame->getFrameCapture() == true) {
                if (m_parameters->getHighSpeedRecording() == true) {
                    if (m_parameters->isUsing3acForIspc() == true)
                        ret = m_putBuffers(m_yuvCaptureBufferMgr, t3acBuffer.index);
                    else
                        ret = m_putBuffers(m_fliteBufferMgr, t3acBuffer.index);

                    if (ret < 0) {
                        CLOGE2("m_putBuffers(m_fliteBufferMgr, %d) fail", t3acBuffer.index);
                        break;
                    }
                } else {
                    entity_buffer_state_t bufferstate = ENTITY_BUFFER_STATE_NOREQ;
                    ret = frame->getDstBufferState(entity->getPipeId(), &bufferstate, factory->getNodeType(PIPE_3AC));
                    if (ret == NO_ERROR && bufferstate != ENTITY_BUFFER_STATE_ERROR) {
                        if (m_parameters->isUseYuvReprocessing() == false
                            && m_parameters->isUsing3acForIspc() == true)
                            ret = m_sccCaptureSelector->manageFrameHoldListForDynamicBayer(frame);
                        else
                            ret = m_captureSelector->manageFrameHoldList(frame, entity->getPipeId(), false, factory->getNodeType(PIPE_3AC));

                        if (ret < 0) {
                            CLOGE2("manageFrameHoldList fail");
                            return ret;
                        }
                    } else {
                        if (m_parameters->isUsing3acForIspc() == true)
                            ret = m_putBuffers(m_yuvCaptureBufferMgr, t3acBuffer.index);
                        else
                            ret = m_putBuffers(m_fliteBufferMgr, t3acBuffer.index);

                        if (ret < 0) {
                            CLOGE2("m_putBuffers(m_fliteBufferMgr, %d) fail", t3acBuffer.index);
                            break;
                        }
                    }
                }
            } else {
                if (reprocessingBayerMode == REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON) {
                    CLOGW2("frame->getRequest(PIPE_3AC) == false. so, just m_putBuffers(t3acBuffer.index(%d)..., pipeId(%d), ret(%d)",
                        t3acBuffer.index, entity->getPipeId(), ret);
                }

                if (m_parameters->isUsing3acForIspc() == true)
                    ret = m_putBuffers(m_yuvCaptureBufferMgr, t3acBuffer.index);
                else
                    ret = m_putBuffers(m_fliteBufferMgr, t3acBuffer.index);

                if (ret < 0) {
                    CLOGE2("m_putBuffers(m_fliteBufferMgr, %d) fail", t3acBuffer.index);
                    break;
                }
            }
        }
    case PIPE_SCP:
        ret = frame->getDstBuffer(entity->getPipeId(), &buffer, factory->getNodeType(PIPE_SCP));
        if (ret < 0) {
            CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", entity->getPipeId(), ret);
            return ret;
        }

        CLOGV2("SCP frameCount(%d) frame.Count(%d) index(%d)",
            ((struct camera2_stream *)buffer.addr[buffer.planeCount-1])->fcount,
            frame->getFrameCount(),
            buffer.index);
        // TODO: extract this

        if (frame->getFrameCapture() == true) {
            ret = frame->setEntityState(entity->getPipeId(), ENTITY_STATE_COMPLETE);
            if (ret < 0) {
                CLOGE2("setEntityState fail, pipeId(%d), state(%d), ret(%d)",
                        entity->getPipeId(), ENTITY_STATE_COMPLETE, ret);
                return ret;
            }

            CLOGI2("Capture frame(%d)", frame->getFrameCount());
#if defined(ENABLE_FULL_FRAME)
            ret = m_handleIsChainDone(frame);
            if (ret < 0)
                CLOGE2("ERR(%s[%d]):m_handleIsChainDone fail, ret(%d)", ret);

            m_captureStreamThread->run(PRIORITY_DEFAULT);
#endif
        }

        m_generateDuplicateBuffers(frame, entity->getPipeId());

        break;
    case PIPE_VRA:
        ret = frame->getDstBuffer(entity->getPipeId(), &buffer);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, entity->getPipeId(), ret);
            return ret;
        }

        if (buffer.index >= 0) {
            if (entity->getDstBufState() != ENTITY_BUFFER_STATE_ERROR) {
                /* Face detection update */
                m_pushResult(frame->getFrameCount(), (struct camera2_shot_ext*)buffer.addr[buffer.planeCount-1]);
            }

            ret = m_vraBufferMgr->putBuffer(buffer.index, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL);
            if (ret != NO_ERROR)
                CLOGW("WARN(%s[%d]):Put VRA buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        break;
    case PIPE_3AC:
    case PIPE_FLITE:
       /* 1. Handle bayer buffer */
        if (m_parameters->isFlite3aaOtf() == true) {
            ret = m_handleBayerBuffer(frame);
            if (ret < NO_ERROR) {
                ALOGE("ERR(%s[%d]):Handle bayer buffer failed, framecount(%d), pipeId(%d), ret(%d)",
                        __FUNCTION__, __LINE__, frame->getFrameCount(), entity->getPipeId(), ret);
                return ret;
            }
        } else {
            /* Send the bayer buffer to 3AA Pipe */
            ret = frame->getDstBuffer(entity->getPipeId(), &buffer);
            if (ret < 0) {
                ALOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                        __FUNCTION__, __LINE__, entity->getPipeId(), ret);
                return ret;
            }

            if (buffer.index < 0) {
                ALOGE("ERR(%s[%d]):Invalid buffer index(%d), framecount(%d), pipeId(%d)",
                        __FUNCTION__, __LINE__,
                        buffer.index, frame->getFrameCount(), entity->getPipeId());
                return BAD_VALUE;
            }
            ALOGV("DEBUG(%s[%d]):Deliver Flite Buffer to 3AA. driver->framecount %d hal->framecount %d",
                    __FUNCTION__, __LINE__,
                    getMetaDmRequestFrameCount((struct camera2_shot_ext *)buffer.addr[buffer.planeCount-1]),
                    frame->getFrameCount());

            ret = m_setupEntity(PIPE_3AA, frame, &buffer, NULL);
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):setSrcBuffer failed, pipeId(%d), ret(%d)",
                        __FUNCTION__, __LINE__, PIPE_3AA, ret);
                return ret;
            }

            factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];
            factory->setOutputFrameQToPipe(m_pipeFrameDoneQ[PIPE_3AA], PIPE_3AA);
            factory->pushFrameToPipe(&frame, PIPE_3AA);
        }

        break;
    case PIPE_ISP:
        ret = frame->getSrcBuffer(entity->getPipeId(), &buffer);
        if (ret < 0) {
            CLOGE2("getSrcBuffer fail, pipeId(%d), ret(%d)", entity->getPipeId(), ret);
            return ret;
        }

        if (buffer.index >= 0) {
            ret = m_updateTimestamp(frame, &buffer);
            if (ret != NO_ERROR) {
                CLOGE2("[F%d B%d]Failed to updateTimestamp",
                        frame->getFrameCount(), buffer.index);
                return ret;
            }

            ret = m_putBuffers(m_ispBufferMgr, buffer.index);
            if (ret < 0) {
                CLOGE2("put Buffer fail");
                break;
            }
        }

        /* break; *//* MCpipe case */
    default:
        CLOGE2("Invalid pipe ID");
        break;
    }

    ret = frame->setEntityState(entity->getPipeId(), entityState);
    if (ret < 0) {
        CLOGE2("setEntityState fail, pipeId(%d), state(%d), ret(%d)",
            entity->getPipeId(), ENTITY_STATE_COMPLETE, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCamera3::m_generateDuplicateBuffers(ExynosCameraFrame *frame, int pipeIdSrc)
{
    status_t ret = NO_ERROR;
    ExynosCameraRequest *halRequest = NULL;
    camera3_capture_request *serviceRequest = NULL;
    const camera3_stream_buffer_t *targetBuffer = NULL;
    const camera3_stream_buffer_t *targetBufferList = NULL;

    ExynosCameraStream *stream = NULL;
    int streamId = 0;

    List<int> keylist;
    List<int>::iterator iter;
    keylist.clear();

    List<int> *outputStreamId;
    List<int>::iterator outputStreamIdIter;

    if (frame == NULL) {
        CLOGE2("frame is NULL");
        return INVALID_OPERATION;
    }

    halRequest = m_requestMgr->getServiceRequest(frame->getFrameCount());
    if (halRequest == NULL) {
        CLOGE2("halRequest is NULL");
        return INVALID_OPERATION;
    }

    serviceRequest = halRequest->getService();
    if (serviceRequest == NULL) {
        CLOGE2("serviceRequest is NULL");
        return INVALID_OPERATION;
    }

    CLOGV2("frame->getFrameCount(%d) halRequest->getFrameCount(%d) serviceRequest->num_output_buffers(%d)",
        frame->getFrameCount(),
        halRequest->getFrameCount(),
        serviceRequest->num_output_buffers);

    halRequest->getAllRequestOutputStreams(&outputStreamId);

    if (outputStreamId->size() > 0) {
        for (outputStreamIdIter = outputStreamId->begin(); outputStreamIdIter != outputStreamId->end(); outputStreamIdIter++) {
            m_streamManager->getStream(*outputStreamIdIter, &stream);

            if (stream == NULL) {
                CLOGE2("stream is NULL");
                continue;
            }

            stream->getID(&streamId);

            switch (streamId % HAL_STREAM_ID_MAX) {
            case HAL_STREAM_ID_RAW:
                break;
            case HAL_STREAM_ID_PREVIEW:
                m_doDestCSC(true, frame, pipeIdSrc, streamId, PIPE_GSC);
                break;
            case HAL_STREAM_ID_VIDEO:
                m_doDestCSC(true, frame, pipeIdSrc, streamId, PIPE_GSC_VIDEO);
                break;
            case HAL_STREAM_ID_JPEG:
                break;
            case HAL_STREAM_ID_CALLBACK:
                m_doDestCSC(true, frame, pipeIdSrc, streamId, PIPE_GSC);
                break;
            default:
                break;
            }
        }
    }

    return ret;
}

bool ExynosCamera3::m_duplicateBufferThreadFunc(void)
{
    status_t ret = 0;
    int index = 0;
    ExynosCameraFrame *newFrame= NULL;
    dup_buffer_info_t dupBufferInfo;
    ExynosCameraBuffer srcBuffer;
    ExynosCameraBuffer dstBuffer;
    ExynosCameraStream *stream = NULL;
    camera3_stream_buffer_t streamBuffer;
    ExynosCameraRequest *request = NULL;
    ResultRequest resultRequest = NULL;
    int actualFormat = 0;
    int bufIndex = -1;

    unsigned int completeBufferCount = 0;

    ExynosCameraBufferManager *bufferMgr = NULL;

    ret = m_duplicateBufferDoneQ->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW2("wait timeout");
        } else {
            CLOGE2("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return true;
    }

    if (newFrame == NULL) {
        CLOGE2("frame is NULL");
        goto func_exit;
    }

    CLOGV2("CSC done (frameCount(%d))", newFrame->getFrameCount());

    dupBufferInfo = newFrame->getDupBufferInfo();
    CLOGV2("streamID(%d), extScalerPipeID(%d)", dupBufferInfo.streamID, dupBufferInfo.extScalerPipeID);

    ret = m_streamManager->getStream(dupBufferInfo.streamID, &stream);
    if (ret < 0) {
        CLOGE2("getStream is failed, from streammanager. Id error:HAL_STREAM_ID_PREVIEW");
        goto func_exit;
    }

    ret = stream->getStream(&streamBuffer.stream);
    if (ret < 0) {
        CLOGE2("ERR(%s[%d]):getStream is failed, from exynoscamerastream. Id error:HAL_STREAM_ID_PREVIEW");
        goto func_exit;
    }

    stream->getBufferManager(&bufferMgr);
    CLOGV2("bufferMgr(%p)", bufferMgr);

    ret = newFrame->getDstBuffer(dupBufferInfo.extScalerPipeID, &dstBuffer);
    if (ret < 0) {
        CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", dupBufferInfo.extScalerPipeID, ret);
        goto func_exit;
    }

    ret = bufferMgr->getHandleByIndex(&streamBuffer.buffer, dstBuffer.index);
    if (ret != OK) {
        CLOGE2("Buffer index error(%d)!!", dstBuffer.index);
        goto func_exit;
    }

    /* update output stream buffer information */
    streamBuffer.status = CAMERA3_BUFFER_STATUS_OK;
    streamBuffer.acquire_fence = -1;
    streamBuffer.release_fence = -1;

    request = m_requestMgr->getServiceRequest(newFrame->getFrameCount());

    resultRequest = m_requestMgr->createResultRequest(newFrame->getFrameCount(), EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY, NULL, NULL);
    resultRequest->pushStreamBuffer(&streamBuffer);

    m_requestMgr->callbackSequencerLock();
    request->increaseCompleteBufferCount();
    request->increaseDuplicateBufferCount();
    completeBufferCount = request->getNumOfOutputBuffer();
    if (newFrame->getFrameCapture() == true)
        completeBufferCount --;
    if (newFrame->getFrameServiceBayer() == true)
        completeBufferCount --;

    CLOGV2("OutputBuffer(%d) CompleteBufferCount(%d) DuplicateBufferCount(%d) streamID(%d), extScaler(%d), frame: Count(%d), ServiceBayer(%d), Capture(%d) completeBufferCount(%d)",
        request->getNumOfOutputBuffer(),
        request->getCompleteBufferCount(),
        request->getDuplicateBufferCount(),
        dupBufferInfo.streamID,
        dupBufferInfo.extScalerPipeID,
        newFrame->getFrameCount(),
        newFrame->getFrameServiceBayer(),
        newFrame->getFrameCapture(),
        completeBufferCount);

    if(completeBufferCount == (unsigned int)request->getDuplicateBufferCount()) {
        ret = newFrame->getSrcBuffer(dupBufferInfo.extScalerPipeID, &srcBuffer);
        if (srcBuffer.index >= 0) {
            CLOGV2("Internal Scp Buffer is returned index(%d)frameCount(%d)", srcBuffer.index, newFrame->getFrameCount());
            ret = m_putBuffers(m_internalScpBufferMgr, srcBuffer.index);
            if (ret < 0) {
                CLOGE2("put Buffer fail");
            }
        }
    }
    m_requestMgr->callbackRequest(resultRequest);
    m_requestMgr->callbackSequencerUnlock();

func_exit:
    if (newFrame != NULL ) {
        newFrame->decRef();
        m_frameMgr->deleteFrame(newFrame);;
        newFrame = NULL;
    }

    return true;
}

status_t ExynosCamera3::m_doDestCSC(bool enableCSC, ExynosCameraFrame *frame, int pipeIdSrc, int halStreamId, int pipeExtScalerId)
{
    status_t ret = OK;
    ExynosCameraFrame *newFrame = NULL;
    ExynosRect srcRect, dstRect;
    ExynosCamera3FrameFactory *factory = NULL;
    ExynosCameraBuffer srcBuffer;
    ExynosCameraBuffer dstBuffer;
    ExynosCameraStream *stream = NULL;
    camera3_stream_buffer_t streamBuffer;
    ExynosCameraRequest *request = NULL;
    ResultRequest resultRequest = NULL;
    int actualFormat = 0;
    int bufIndex = -1;
    dup_buffer_info_t dupBufferInfo;
    struct camera2_stream *meta = NULL;
    uint32_t *output = NULL;

    ExynosCameraBufferManager *bufferMgr = NULL;


    if (enableCSC == false) {
        /* TODO: memcpy srcBuffer, dstBuffer */
        return NO_ERROR;
    }

    request = m_requestMgr->getServiceRequest(frame->getFrameCount());
    factory = request->getFrameFactory(halStreamId);
    ret = frame->getDstBuffer(pipeIdSrc, &srcBuffer, factory->getNodeType(PIPE_SCP));
    if (ret < 0) {
        CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", pipeIdSrc, ret);
        return ret;
    }

    ret = m_streamManager->getStream(halStreamId, &stream);
    if (ret < 0) {
        CLOGE2("getStream is failed, from streammanager. Id error:HAL_STREAM_ID_PREVIEW");
        return ret;
    }

    ret = stream->getStream(&streamBuffer.stream);
    if (ret < 0) {
        CLOGE2("getStream is failed, from exynoscamerastream. Id error:HAL_STREAM_ID_PREVIEW");
        return ret;
    }

    meta = (struct camera2_stream*)srcBuffer.addr[srcBuffer.planeCount-1];
    output = meta->output_crop_region;

    stream->getBufferManager(&bufferMgr);
    CLOGV2("bufferMgr(%p)", bufferMgr);

    ret = bufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &dstBuffer);
    if (ret < 0) {
        CLOGE2("bufferMgr getBuffer fail, frameCount(%d), ret(%d)", frame->getFrameCount(), ret);
        return ret;
    }

    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.w = output[2];
    srcRect.h = output[3];
    srcRect.fullW = output[2];
    srcRect.fullH = output[3];
    srcRect.colorFormat = m_parameters->getHwPreviewFormat();

    dstRect.x = 0;
    dstRect.y = 0;
    dstRect.fullW = dstRect.w = streamBuffer.stream->width;
    dstRect.fullH = dstRect.h = streamBuffer.stream->height;
    stream->getFormat(&actualFormat);
    dstRect.colorFormat = HAL_PIXEL_FORMAT_2_V4L2_PIX(actualFormat);

    ret = getCropRectAlign(srcRect.w, srcRect.h,
                           dstRect.w, dstRect.h,
                           &srcRect.x, &srcRect.y,
                           &srcRect.w, &srcRect.h,
                           2, 2,
                           0, 1);

    newFrame = factory->createNewFrameOnlyOnePipe(pipeExtScalerId, frame->getFrameCount());

    ret = newFrame->setSrcRect(pipeExtScalerId, srcRect);
    if (ret != NO_ERROR) {
        CLOGE2("setSrcRect fail, frameCount(%d), ret(%d)", frame->getFrameCount(), ret);
        return ret;
    }

    ret = newFrame->setDstRect(pipeExtScalerId, dstRect);
    if (ret != NO_ERROR) {
        CLOGE2("setDstRect fail, frameCount(%d), ret(%d)", frame->getFrameCount(), ret);
        return ret;
    }

    CLOGV2("srcRect size (%d, %d, %d, %d %d %d)",
        srcRect.x, srcRect.y, srcRect.w, srcRect.h, srcRect.fullW, srcRect.fullH);
    CLOGV2("dstRect size (%d, %d, %d, %d %d %d)",
        dstRect.x, dstRect.y, dstRect.w, dstRect.h, dstRect.fullW, dstRect.fullH);

    /* GSC can be shared by preview and previewCb. Make sure dstBuffer for previewCb buffer. */
    /* m_resetBufferState(pipeExtScalerId, frame); */
    m_resetBufferState(pipeExtScalerId, newFrame);

    ret = m_setupEntity(pipeExtScalerId, newFrame, &srcBuffer, &dstBuffer);
    if (ret < 0) {
        CLOGE2("setupEntity fail, pipeExtScalerId(%d), ret(%d)", pipeExtScalerId, ret);
    }

    dupBufferInfo.streamID = halStreamId;
    dupBufferInfo.extScalerPipeID = pipeExtScalerId;
    newFrame->setDupBufferInfo(dupBufferInfo);
    newFrame->setFrameCapture(frame->getFrameCapture());
    newFrame->setFrameServiceBayer(frame->getFrameServiceBayer());

    factory->setOutputFrameQToPipe(m_duplicateBufferDoneQ, pipeExtScalerId);
    factory->pushFrameToPipe(&newFrame, pipeExtScalerId);

    return ret;
}

status_t ExynosCamera3::m_releaseBuffers(void)
{
    CLOGI2("release buffer");
    int ret = 0;
    enum EXYNOS_CAMERA_BUFFER_PERMISSION permission;
    enum EXYNOS_CAMERA_BUFFER_POSITION position;

    /* Pull all internal buffers */
    for (int bufIndex = 0; bufIndex < m_fliteBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_fliteBufferMgr, bufIndex);
    for (int bufIndex = 0; bufIndex < m_3aaBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_3aaBufferMgr, bufIndex);
    for (int bufIndex = 0; bufIndex < m_internalScpBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_internalScpBufferMgr, bufIndex);
    for (int bufIndex = 0; bufIndex < m_yuvCaptureBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_yuvCaptureBufferMgr, bufIndex);
    for (int bufIndex = 0; bufIndex < m_vraBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_vraBufferMgr, bufIndex);
    for (int bufIndex = 0; bufIndex < m_gscBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_gscBufferMgr, bufIndex);
    for (int bufIndex = 0; bufIndex < m_ispBufferMgr->getAllocatedBufferCount(); bufIndex++)
        ret = m_putBuffers(m_ispBufferMgr, bufIndex);

    if (m_bayerBufferMgr != NULL) {
        m_bayerBufferMgr->deinit();
    }

    if (m_fliteBufferMgr != NULL) {
        m_fliteBufferMgr->deinit();
    }

    if (m_3aaBufferMgr != NULL) {
        m_3aaBufferMgr->deinit();
    }
    if (m_ispBufferMgr != NULL) {
        m_ispBufferMgr->deinit();
    }
    if (m_internalScpBufferMgr != NULL) {
        m_internalScpBufferMgr->deinit();
    }
    if (m_ispReprocessingBufferMgr != NULL) {
        m_ispReprocessingBufferMgr->deinit();
    }
    if (m_yuvCaptureBufferMgr != NULL) {
        m_yuvCaptureBufferMgr->deinit();
    }
    if (m_vraBufferMgr != NULL) {
        m_vraBufferMgr->deinit();
    }
    if (m_gscBufferMgr != NULL) {
        m_gscBufferMgr->deinit();
    }
    if (m_yuvCaptureReprocessingBufferMgr != NULL) {
        m_yuvCaptureReprocessingBufferMgr->deinit();
    }
    if (m_thumbnailBufferMgr != NULL)
        m_thumbnailBufferMgr->deinit();

    if (m_skipBufferMgr != NULL) {
        m_skipBufferMgr->deinit();
    }

    CLOGI2("free buffer done");

    return NO_ERROR;
}

/* m_registerStreamBuffers
 * 1. Get the input buffers from the input request
 * 2. Get the output buffers from the input request
 * 3. Register each buffers into the matched buffer manager
 * This operation must be done before another request is delivered from the service.
 */
status_t ExynosCamera3::m_registerStreamBuffers(camera3_capture_request *request)
{
    status_t ret = NO_ERROR;
    const camera3_stream_buffer_t *buffer;
    const camera3_stream_buffer_t *bufferList;
    uint32_t bufferCount = 0;
    int streamId = -1;
    uint32_t requestKey = 0;
    ExynosCameraStream *stream = NULL;
    ExynosCameraBufferManager *bufferMgr = NULL;

    /* 1. Get the information of input buffers from the input request */
    requestKey = request->frame_number;
    buffer = request->input_buffer;

    /* 2. Register the each input buffers into the matched buffer manager */
    if (buffer != NULL) {
        stream = static_cast<ExynosCameraStream *>(buffer->stream->priv);
        stream->getID(&streamId);

        switch (streamId % HAL_STREAM_ID_MAX) {
            case HAL_STREAM_ID_ZSL_INPUT:
                m_registerBuffers(m_bayerBufferMgr, requestKey, buffer);
                ALOGV("DEBUG(%s[%d]):request(%d) inputBuffer(%p) buffer-StreamType(HAL_STREAM_ID_RAW) size(%u x %u) ",
                        __FUNCTION__, __LINE__,
                        request->frame_number, buffer,
                        buffer->stream->width,
                        buffer->stream->height);
                break;
            case HAL_STREAM_ID_PREVIEW:
            case HAL_STREAM_ID_VIDEO:
            case HAL_STREAM_ID_JPEG:
            case HAL_STREAM_ID_CALLBACK:
            default:
                ALOGE("ERR(%s[%d]):request(%d) inputBuffer(%p) buffer-stream type is inavlid(%d). size(%d x %d)",
                        __FUNCTION__, __LINE__,
                        requestKey, buffer, streamId,
                        buffer->stream->width,
                        buffer->stream->height);
                break;
        }
    }

    /* 3. Get the information of output buffers from the input request */
    bufferCount = request->num_output_buffers;
    bufferList = request->output_buffers;

    /* 4. Register the each output buffers into the matched buffer manager */
    for (uint32_t index = 0; index < bufferCount; index++) {
        buffer = &(bufferList[index]);
        stream = static_cast<ExynosCameraStream *>(bufferList[index].stream->priv);
        stream->getID(&streamId);
        stream->getBufferManager(&bufferMgr);

        switch (streamId % HAL_STREAM_ID_MAX) {
            case HAL_STREAM_ID_RAW:
                m_registerBuffers(m_bayerBufferMgr, requestKey, buffer);
                ALOGV("DEBUG(%s[%d]):request(%d) outputBuffer(%p) buffer-StreamType(HAL_STREAM_ID_RAW) size(%u x %u) ",
                        __FUNCTION__, __LINE__,
                        request->frame_number, buffer,
                        bufferList[index].stream->width,
                        bufferList[index].stream->height);
                break;
            case HAL_STREAM_ID_ZSL_OUTPUT:
                /* no buffer register */
                ALOGV("DEBUG(%s[%d]):request(%d) outputBuffer(%p) buffer-StreamType(HAL_STREAM_ID_ZSL_OUTPUT) size(%u x %u) ",
                        __FUNCTION__, __LINE__,
                        request->frame_number, buffer,
                        bufferList[index].stream->width,
                        bufferList[index].stream->height);
                break;
            case HAL_STREAM_ID_PREVIEW:
                m_registerBuffers(bufferMgr, requestKey, buffer);
                ALOGV("DEBUG(%s[%d]):request(%d) outputBuffer(%p) buffer-StreamType(HAL_STREAM_ID_PREVIEW) size(%u x %u) ",
                        __FUNCTION__, __LINE__,
                        request->frame_number, buffer,
                        bufferList[index].stream->width,
                        bufferList[index].stream->height);
                break;
            case HAL_STREAM_ID_VIDEO:
                m_registerBuffers(bufferMgr, requestKey, buffer);
                ALOGV("DEBUG(%s[%d]):request(%d) outputBuffer(%p) buffer-StreamType(HAL_STREAM_ID_VIDEO) size(%u x %u) ",
                        __FUNCTION__, __LINE__,
                        request->frame_number, buffer,
                        bufferList[index].stream->width,
                        bufferList[index].stream->height);
                break;
            case HAL_STREAM_ID_JPEG:
                m_registerBuffers(bufferMgr, requestKey, buffer);
                ALOGD("DEBUG(%s[%d]):request(%d) outputBuffer(%p) buffer-StreamType(HAL_STREAM_ID_JPEG) size(%u x %u) ",
                        __FUNCTION__, __LINE__,
                        request->frame_number, buffer,
                        bufferList[index].stream->width,
                        bufferList[index].stream->height);
                break;
            case HAL_STREAM_ID_CALLBACK:
                m_registerBuffers(bufferMgr, requestKey, buffer);
                ALOGV("DEBUG(%s[%d]):request(%d) outputBuffer(%p) buffer-StreamType(HAL_STREAM_ID_CALLBACK) size(%u x %u) ",
                        __FUNCTION__, __LINE__,
                        request->frame_number, buffer,
                        bufferList[index].stream->width,
                        bufferList[index].stream->height);
                break;
            default:
                ALOGE("ERR(%s[%d]):request(%d) outputBuffer(%p) buffer-StreamType is invalid(%d) size(%d x %d) ",
                        __FUNCTION__, __LINE__,
                        request->frame_number, buffer, streamId,
                        bufferList[index].stream->width,
                        bufferList[index].stream->height);
                break;
        }
    }

    return ret;
}

status_t ExynosCamera3::m_registerBuffers(
        ExynosCameraBufferManager *bufManager,
        int requestKey,
        const camera3_stream_buffer_t *streamBuffer)
{
    status_t ret = OK;
    buffer_handle_t *handle = streamBuffer->buffer;
    int acquireFence = streamBuffer->acquire_fence;
    int releaseFence = streamBuffer->release_fence;

    if (bufManager != NULL) {
        ret = bufManager->registerBuffer(
                            requestKey,
                            handle,
                            acquireFence,
                            releaseFence,
                            EXYNOS_CAMERA_BUFFER_POSITION_NONE);
        if (ret < 0) {
            CLOGE2("putBuffer(%d) fail(%d)", requestKey, ret);
            return BAD_VALUE;
        }
    }

    return ret;
}

status_t ExynosCamera3::m_putBuffers(ExynosCameraBufferManager *bufManager, int bufIndex)
{
    status_t ret = NO_ERROR;
    if (bufManager != NULL)
        ret = bufManager->putBuffer(bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_NONE);

    return ret;
}

status_t ExynosCamera3::m_allocBuffers(
        ExynosCameraBufferManager *bufManager,
        int  planeCount,
        unsigned int *planeSize,
        unsigned int *bytePerLine,
        int  startIndex,
        int  reqBufCount,
        bool createMetaPlane,
        bool needMmap)
{
    int ret = 0;

    ret = bufManager->setInfo(
                        planeCount,
                        planeSize,
                        bytePerLine,
                        startIndex,
                        reqBufCount,
                        createMetaPlane,
                        needMmap);
    if (ret < 0) {
        CLOGE2("setInfo fail");
        goto func_exit;
    }

    ret = bufManager->alloc();
    if (ret < 0) {
        CLOGE2("alloc fail");
        goto func_exit;
    }

func_exit:

    return ret;
}

status_t ExynosCamera3::m_allocBuffers(
        ExynosCameraBufferManager *bufManager,
        int  planeCount,
        unsigned int *planeSize,
        unsigned int *bytePerLine,
        int  reqBufCount,
        bool createMetaPlane,
        bool needMmap)
{
    int ret = 0;

    ret = bufManager->setInfo(
                        planeCount,
                        planeSize,
                        bytePerLine,
                        reqBufCount,
                        createMetaPlane,
                        needMmap);
    if (ret < 0) {
        CLOGE2("setInfo fail");
        goto func_exit;
    }

    ret = bufManager->alloc();
    if (ret < 0) {
        CLOGE2("alloc fail");
        goto func_exit;
    }

func_exit:

    return ret;
}

status_t ExynosCamera3::m_allocBuffers(
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
        CLOGE2("m_allocBuffers(minBufCount=%d, maxBufCount=%d, type=%d) fail", minBufCount, maxBufCount, type);
    }

    return ret;
}

status_t ExynosCamera3::m_allocBuffers(
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

    CLOGI2("setInfo(planeCount=%d, minBufCount=%d, maxBufCount=%d, type=%d, allocMode=%d)",
        planeCount, minBufCount, maxBufCount, (int)type, (int)allocMode);

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
        CLOGE2("setInfo fail");
        goto func_exit;
    }

    ret = bufManager->alloc();
    if (ret < 0) {
        CLOGE2("alloc fail");
        goto func_exit;
    }

func_exit:

    return ret;
}

status_t ExynosCamera3::m_setBuffers(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    int ret = OK;
    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    unsigned int planeSize[EXYNOS_CAMERA_BUFFER_MAX_PLANES]    = {0};
    int planeCount  = 1;
    int bufferCount = 1;
    int previewMaxW, previewMaxH;
    int sensorMaxW, sensorMaxH;

    int hwPreviewW, hwPreviewH;
    int hwSensorW, hwSensorH;
    int hwPictureW, hwPictureH;
    int sensorMarginW, sensorMarginH;
    exynos_camera_buffer_type_t type = EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE;

    int minBufferCount = 1;
    int maxBufferCount = 1;

    CLOGI2("alloc buffer - camera ID: %d", m_cameraId);

    // TODO: get this value from metadata class
    m_parameters->getHwPreviewSize(&hwPreviewW, &hwPreviewH);
    CLOGI2("HW Preview width x height = %dx%d", hwPreviewW, hwPreviewH);
    m_parameters->getHwSensorSize(&hwSensorW, &hwSensorH);
    CLOGI2("HW Sensor  width x height = %dx%d", hwSensorW, hwSensorH);
    m_parameters->getHwPictureSize(&hwPictureW, &hwPictureH);
    CLOGI2("HW Picture width x height = %dx%d", hwPictureW, hwPictureH);

    m_parameters->getMaxSensorSize(&sensorMaxW, &sensorMaxH);
    CLOGI2("HW Sensor MAX width x height = %dx%d", sensorMaxW, sensorMaxH);
    m_parameters->getMaxPreviewSize(&previewMaxW, &previewMaxH);
    CLOGI2("HW Preview MAX width x height = %dx%d", previewMaxW, previewMaxH);

    m_parameters->getSensorMargin(&sensorMarginW, &sensorMarginH);

    /* For preview stream */
    /* FLITE -> need bayer buffer for non zsl capture. */

    if (m_parameters->isFlite3aaOtf() == false) {
#if !defined(ENABLE_FULL_FRAME)
#ifdef CAMERA_PACKED_BAYER_ENABLE
#ifdef DEBUG_RAWDUMP
        if (m_parameters->checkBayerDumpEnable()) {
            bytesPerLine[0] = (sensorMaxW + sensorMarginW) * 2;
            planeSize[0] = (sensorMaxW + sensorMarginW) * (sensorMaxH + sensorMarginH) * 2;
        } else
#endif /* DEBUG_RAWDUMP */
        {
            bytesPerLine[0] = ROUND_UP((sensorMaxW + sensorMarginW), 10) * 8 / 5;
            planeSize[0]    = bytesPerLine[0] * (sensorMaxH + sensorMarginH);
        }
#else
        planeSize[0] = (sensorMaxW + sensorMarginW) * (sensorMaxH + sensorMarginH) * 2;
#endif
        planeCount  = 2;

        /* TO DO : make num of buffers samely */
        maxBufferCount =  NUM_BAYER_BUFFERS;

        ret = m_allocBuffers(m_fliteBufferMgr, planeCount, planeSize, bytesPerLine, maxBufferCount, maxBufferCount, type, true, false);
        if (ret < 0) {
            CLOGE2("ERR(%s[%d]):bayerBuffer m_allocBuffers(bufferCount=%d) fail", maxBufferCount);
            return ret;
        }
#endif
    }

    /* Buffers of FLITE is given by service for ZSL*/
    // TODO: consider non-zsl case
    /* 3AA */
    planeSize[0] = 32 * 64 * 2;
    planeCount  = 2;
    bufferCount = m_exynosconfig->current->bufInfo.num_3aa_buffers;
    ret = m_allocBuffers(m_3aaBufferMgr, planeCount, planeSize, bytesPerLine, bufferCount, true);
    if (ret < 0) {
        CLOGE2("m_3aaBufferMgr m_allocBuffers(bufferCount=%d) fail", bufferCount);
        return ret;
    }

    /* ISP */
    // TODO: packed bayer?
    if (m_parameters->is3aaIspOtf() == false) {
        if (m_parameters->isFlite3aaOtf() == true)
            planeSize[0] = previewMaxW * previewMaxH * 2;
        else
            planeSize[0] = sensorMaxW * sensorMaxH * 2;
        planeCount  = 2;
        bufferCount = 1;
        ret = m_allocBuffers(m_ispBufferMgr, planeCount, planeSize, bytesPerLine, bufferCount, true);
        if (ret < 0) {
            CLOGE2("m_ispBufferMgr m_allocBuffers(bufferCount=%d) fail", bufferCount);
            return ret;
        }
    }

    /* SCC */
    /* planeSize[0] = ALIGN_UP(hwPictureW, GSCALER_IMG_ALIGN) * ALIGN_UP(hwPictureH, GSCALER_IMG_ALIGN) * 2; */
    planeSize[0] = sensorMaxW * sensorMaxH * 2;
    planeCount  = 2;
    // TODO: Need dynamic buffer allocation. reduce SCC buffer
    bufferCount = NUM_PICTURE_BUFFERS;

    ret = m_allocBuffers(m_yuvCaptureBufferMgr, planeCount, planeSize, bytesPerLine, bufferCount, true);
    if (ret < 0) {
        CLOGE2("m_yuvCaptureBufferMgr m_allocBuffers(bufferCount=%d) fail", bufferCount);
        return ret;
    }

    /* VRA buffers */
    if (m_parameters->isMcscVraOtf() == false) {
        int vraWidth = 0, vraHeight = 0;
        m_parameters->getHwVraInputSize(&vraWidth, &vraHeight);

        bytesPerLine[0] = ROUND_UP((vraWidth * 3 / 2), CAMERA_16PX_ALIGN);
        planeSize[0]    = bytesPerLine[0] * vraHeight;
        planeCount      = 2;

        maxBufferCount = m_exynosconfig->current->bufInfo.num_vra_buffers;

        type = EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE;

        ret = m_allocBuffers(m_vraBufferMgr, planeCount, planeSize, bytesPerLine, maxBufferCount, maxBufferCount, type, true, true);
        if (ret < 0) {
            CLOGE2("m_vraBufferMgr m_allocBuffers(bufferCount=%d) fail", maxBufferCount);
            return ret;
        }
    }

    ret = m_setInternalScpBuffer();
    if (ret < 0) {
        CLOGE2("m_setReprocessing Buffer fail");
        return ret;
    }

    CLOGI2("alloc buffer done - camera ID: %d", m_cameraId);
    return ret;
}

status_t ExynosCamera3::m_setInternalScpBuffer(void)
{
    int ret = 0;
    int hwPreviewW = 0, hwPreviewH = 0;
    unsigned int planeSize[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    int planeCount  = 0;
    int bufferCount = 0;
    int minBufferCount = NUM_REPROCESSING_BUFFERS;
    int maxBufferCount = NUM_PREVIEW_BUFFERS;
    exynos_camera_buffer_type_t type = EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE;
    buffer_manager_allocation_mode_t allocMode = BUFFER_MANAGER_ALLOCATION_ONDEMAND;

    m_parameters->getHwPreviewSize(&hwPreviewW, &hwPreviewH);
    CLOGI2("HW Picture MAX width x height = %dx%d", hwPreviewW, hwPreviewH);

    bytesPerLine[0] = 0;
    planeSize[0] = ALIGN_UP(hwPreviewW, GSCALER_IMG_ALIGN) * ALIGN_UP(hwPreviewH, GSCALER_IMG_ALIGN);
    planeSize[1] = (ALIGN_UP(hwPreviewW, GSCALER_IMG_ALIGN) * ALIGN_UP(hwPreviewH, GSCALER_IMG_ALIGN)) / 2;
    planeCount  = 3;
    minBufferCount = m_exynosconfig->current->bufInfo.num_request_preview_buffers;
    maxBufferCount = m_exynosconfig->current->bufInfo.num_preview_buffers;

    allocMode = BUFFER_MANAGER_ALLOCATION_ONDEMAND;

    ret = m_allocBuffers(m_internalScpBufferMgr, planeCount, planeSize, bytesPerLine, minBufferCount, maxBufferCount, type, allocMode, true, false);
    if (ret < 0) {
        CLOGE2("m_internalScpBufferMgr m_allocBuffers(minBufferCount=%d, maxBufferCount=%d) fail", minBufferCount, maxBufferCount);
        return ret;
    }

    return NO_ERROR;
}

bool ExynosCamera3::m_setBuffersThreadFunc(void)
{
    int ret;

    ret = m_setBuffers();
    if (ret < 0) {
        CLOGE2("m_setBuffers failed");
        // TODO: Need release buffers and error exit
        return false;
    }

    return false;
}

uint32_t ExynosCamera3::m_getBayerPipeId(void)
{
    uint32_t pipeId = 0;
    // TODO: implement it

    pipeId = PIPE_FLITE;

    return pipeId;
}

status_t ExynosCamera3::m_pushRequest(camera3_capture_request *request)
{
    ExynosCameraRequest* req = NULL;

    CLOGV2("m_pushRequest frameCnt(%d)", request->frame_number);

    req = m_requestMgr->registerServiceRequest(request);
    if(req == NULL) {
        return INVALID_OPERATION;
    } else {
        return OK;
    }
}

status_t ExynosCamera3::m_popRequest(ExynosCameraRequest **request)
{
    status_t ret = OK;

    CLOGV2("m_popRequest ");

    *request = m_requestMgr->createServiceRequest();
    if (*request == NULL) {
        CLOGE2("createRequest failed ");
        ret = INVALID_OPERATION;
    }
    return ret;
}


/* m_needNotify is for reprocessing */
bool ExynosCamera3::m_needNotify(ExynosCameraRequest *request)
{
    camera3_stream_buffer_t *output_buffers;
    List<int> *outputStreamId = NULL;
    List<int>::iterator outputStreamIdIter;
    ExynosCameraStream *stream = NULL;
    int streamId = 0;

    request->getAllRequestOutputStreams(&outputStreamId);
    bool notifyFlag = true;

    /* HACK: can't send notify cause of one request including render, video */
    if (outputStreamId != NULL) {
        for (outputStreamIdIter = outputStreamId->begin(); outputStreamIdIter != outputStreamId->end(); outputStreamIdIter++) {

            m_streamManager->getStream(*outputStreamIdIter, &stream);
            if (stream == NULL) {
                CLOGE2("stream is NULL");
                break;
            }
            stream->getID(&streamId);

            switch (streamId % HAL_STREAM_ID_MAX) {
            case HAL_STREAM_ID_RAW:
            case HAL_STREAM_ID_PREVIEW:
            case HAL_STREAM_ID_VIDEO:
            case HAL_STREAM_ID_CALLBACK:
                notifyFlag = false;
                break;
            default:
                break;
            };
        }
    }

    return notifyFlag;
}


status_t ExynosCamera3::m_pushResult(uint32_t frameCount, struct camera2_shot_ext *src_ext)
{
    status_t ret = OK;
    ExynosCameraRequest *request = NULL;
    struct camera2_shot_ext dst_ext;
    uint8_t currentPipelineDepth = 0;

    request = m_requestMgr->getServiceRequest(frameCount);
    if (request == NULL) {
        CLOGE2("getRequest failed ");
        return INVALID_OPERATION;
    }

    ret = request->getResultShot(&dst_ext);
    if (ret < 0) {
        CLOGE2("getResultShot failed ");
        return INVALID_OPERATION;
    }

    if (dst_ext.shot.dm.request.frameCount > src_ext->shot.dm.request.frameCount) {
        CLOGI("INFO(%s[%d]):Skip to update result. frameCount %d requestKey %d shot.request.frameCount %d",
                __FUNCTION__, __LINE__,
                frameCount, request->getKey(), dst_ext.shot.dm.request.frameCount);
        return ret;
    }

    currentPipelineDepth = dst_ext.shot.dm.request.pipelineDepth;
    memcpy(&dst_ext.shot.dm, &src_ext->shot.dm, sizeof(struct camera2_dm));
    memcpy(&dst_ext.shot.udm, &src_ext->shot.udm, sizeof(struct camera2_udm));
    dst_ext.shot.dm.request.pipelineDepth = currentPipelineDepth;

    ret = request->setResultShot(&dst_ext);
    if (ret < 0) {
        CLOGE2("setResultShot failed ");
        return INVALID_OPERATION;
    }

    ret = m_metadataConverter->updateDynamicMeta(request);

    CLOGV2("result is set (%d)", request->getFrameCount());
    return ret;
}

status_t ExynosCamera3::m_pushJpegResult(ExynosCameraFrame *frame, int size, ExynosCameraBuffer *buffer)
{
    status_t ret = NO_ERROR;
    ExynosCameraStream *stream = NULL;
    camera3_stream_buffer_t streamBuffer;
    camera3_stream_buffer_t *output_buffers;
    ResultRequest resultRequest = NULL;
    ExynosCameraRequest *request = NULL;
    camera3_capture_result_t requestResult;

    ExynosCameraBufferManager *bufferMgr = NULL;

    ret = m_streamManager->getStream(HAL_STREAM_ID_JPEG, &stream);
    if (ret != NO_ERROR) {
        CLOGE2("Failed to getStream from StreamMgr. streamId HAL_STREAM_ID_JPEG");
        return ret;
    }

    if (stream == NULL) {
        CLOGE2("stream is NULL");
        return INVALID_OPERATION;
    }

    ret = stream->getStream(&streamBuffer.stream);
    if (ret != NO_ERROR) {
        CLOGE2("Failed to getStream from ExynosCameraStream. streamId HAL_STREAM_ID_JPEG");
        return ret;
    }

    ret = stream->getBufferManager(&bufferMgr);
    if (ret != NO_ERROR) {
        CLOGE2("Failed to getBufferManager. streamId HAL_STREAM_ID_JPEG");
        return ret;
    }

    ret = bufferMgr->getHandleByIndex(&streamBuffer.buffer, buffer->index);
    if (ret != NO_ERROR) {
        CLOGE2("Failed to getHandleByIndex. bufferIndex %d", buffer->index);
        return ret;
    }

    streamBuffer.status = CAMERA3_BUFFER_STATUS_OK;
    streamBuffer.acquire_fence = -1;
    streamBuffer.release_fence = -1;

    camera3_jpeg_blob_t jpeg_blob;
    jpeg_blob.jpeg_blob_id = CAMERA3_JPEG_BLOB_ID;
    jpeg_blob.jpeg_size = size;
    memcpy(buffer->addr[0]+buffer->size[0]-sizeof(camera3_jpeg_blob_t), &jpeg_blob, sizeof(camera3_jpeg_blob_t));

    request = m_requestMgr->getServiceRequest(frame->getFrameCount());

#if !defined(ENABLE_FULL_FRAME)
    /* try to notify if notify callback was not called in same framecount */
    if (request->getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY) == false) {
        /* can't send notify cause of one request including render, video */
        if (m_needNotify(request) == true) {
            CLOGV2("notify(%d)", frame->getFrameCount());
            m_sendNotify(frame->getFrameCount(), CAMERA3_MSG_SHUTTER);
        }
    }
#endif

    CameraMetadata setting = request->getResultMeta();
    int32_t jpegsize = size;
    ret = setting.update(ANDROID_JPEG_SIZE, &jpegsize, 1);
    if (ret < 0) {
        CLOGE2("ANDROID_JPEG_SIZE update failed(%d)", ret);
    }

    /* update jpeg size */
    request->setResultMeta(setting);

    CLOGD2("Set JPEG result Done. frameCount %d request->Key %d",
        frame->getFrameCount(), request->getKey());

    resultRequest = m_requestMgr->createResultRequest(frame->getFrameCount(), EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY, NULL, NULL);
    resultRequest->pushStreamBuffer(&streamBuffer);

    m_requestMgr->callbackSequencerLock();
    request->increaseCompleteBufferCount();
    m_requestMgr->callbackRequest(resultRequest);
    m_requestMgr->callbackSequencerUnlock();

    CLOGD2("result is set");

    return ret;
}

ExynosCameraRequest* ExynosCamera3::m_popResult(CameraMetadata &result, uint32_t frameCount)
{
    ExynosCameraRequest *request = NULL;
    struct camera2_shot_ext dst_ext;

    request = m_requestMgr->getServiceRequest(frameCount);
    if (request == NULL) {
        CLOGE2("getRequest failed ");
        result.clear();
        return NULL;
    }

    result = request->getResultMeta();

    CLOGV2("m_popResult(%d)", request->getFrameCount());

    return request;
}

status_t ExynosCamera3::m_deleteRequest(uint32_t frameCount)
{
    status_t ret = OK;

    ret = m_requestMgr->deleteServiceRequest(frameCount);

    return ret;
}

status_t ExynosCamera3::m_setReprocessingBuffer(void)
{
    int ret = 0;
    int pictureMaxW = 0, pictureMaxH = 0;
    int hwPictureW = 0, hwPictureH = 0;
    int maxThumbnailW = 0, maxThumbnailH = 0;
    unsigned int planeSize[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    int pictureFormat = 0;
    int planeCount  = 0;
    int bufferCount = 0;
    int minBufferCount = NUM_REPROCESSING_BUFFERS;
    int maxBufferCount = NUM_PICTURE_BUFFERS;
    bool needMmap = false;
    exynos_camera_buffer_type_t type = EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE;
    buffer_manager_allocation_mode_t allocMode = BUFFER_MANAGER_ALLOCATION_ONDEMAND;

    m_parameters->getMaxPictureSize(&pictureMaxW, &pictureMaxH);
    CLOGI2("HW Picture MAX width x height = %dx%d", pictureMaxW, pictureMaxH);
    m_parameters->getMaxThumbnailSize(&maxThumbnailW, &maxThumbnailH);
    CLOGI2("Thumbnail Max width x height = %dx%d", maxThumbnailW, maxThumbnailH);
    pictureFormat = m_parameters->getHwPictureFormat();

    /* for reprocessing */
    if (m_parameters->getUsePureBayerReprocessing() == true) {
#ifdef CAMERA_PACKED_BAYER_ENABLE
#ifdef DEBUG_RAWDUMP
        if (m_parameters->checkBayerDumpEnable()) {
            bytesPerLine[0] = pictureMaxW * 2;
            planeSize[0] = pictureMaxW * pictureMaxH * 2;
        } else
#endif /* DEBUG_RAWDUMP */
        {
            bytesPerLine[0] = ROUND_UP((pictureMaxW * 3 / 2), 16);
            planeSize[0] = bytesPerLine[0] * pictureMaxH;
        }
#else
        planeSize[0] = pictureMaxW * pictureMaxH * 2;
#endif
        planeCount  = 2;
        bufferCount = NUM_REPROCESSING_BUFFERS;

        type = EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE;
        allocMode = BUFFER_MANAGER_ALLOCATION_ONDEMAND;

        if (m_parameters->getHighResolutionCallbackMode() == true) {
            /* ISP Reprocessing Buffer realloc for high resolution callback */
            minBufferCount = 2;
        }

        ret = m_allocBuffers(m_ispReprocessingBufferMgr, planeCount, planeSize, bytesPerLine, minBufferCount, maxBufferCount, type, allocMode, true, false);
        if (ret < 0) {
            CLOGE2("m_ispReprocessingBufferMgr m_allocBuffers(minBufferCount=%d/maxBufferCount=%d) fail", minBufferCount, maxBufferCount);
            return ret;
        }
    }

    if( m_parameters->getHighSpeedRecording() ) {
        m_parameters->getHwSensorSize(&hwPictureW, &hwPictureH);
        CLOGI2("HW Picture(HighSpeed) width x height = %dx%d", hwPictureW, hwPictureH);
    } else {
        m_parameters->getMaxSensorSize(&hwPictureW, &hwPictureH);
        CLOGI2("HW Picture width x height = %dx%d", hwPictureW, hwPictureH);
    }

    if (m_parameters->isUseYuvReprocessingForThumbnail() == true)
        needMmap = true;
    else
        needMmap = false;

    bytesPerLine[0] = 0;
    planeSize[0] = ALIGN_UP(hwPictureW, GSCALER_IMG_ALIGN) * ALIGN_UP(hwPictureH, GSCALER_IMG_ALIGN) * 2;
    planeCount  = 2;
    minBufferCount = 1;
    maxBufferCount = NUM_PICTURE_BUFFERS;

    type = EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE;
    if (m_parameters->isHWFCEnabled() == true)
        allocMode = BUFFER_MANAGER_ALLOCATION_ATONCE;
    else
        allocMode = BUFFER_MANAGER_ALLOCATION_ONDEMAND;

    if (m_parameters->getHighResolutionCallbackMode() == true) {
        /* SCC Reprocessing Buffer realloc for high resolution callback */
        minBufferCount = 2;
    }

    ret = m_allocBuffers(m_yuvCaptureReprocessingBufferMgr,
                         planeCount, planeSize, bytesPerLine,
                         minBufferCount, maxBufferCount,
                         type, allocMode, true, needMmap);
    if (ret < 0) {
        CLOGE2("m_yuvCaptureReprocessingBufferMgr m_allocBuffers(minBufferCount=%d, maxBufferCount=%d) fail", minBufferCount, maxBufferCount);
        return ret;
    }

    /* Reprocessing Thumbanil buffer */
    switch (pictureFormat) {
        case V4L2_PIX_FMT_NV21M:
            planeCount      = 3;
            planeSize[0]    = maxThumbnailW * maxThumbnailH;
            planeSize[1]    = maxThumbnailW * maxThumbnailH / 2;
        case V4L2_PIX_FMT_NV21:
        default:
            planeCount      = 2;
            planeSize[0]    = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(pictureFormat), maxThumbnailW, maxThumbnailH);
    }

    minBufferCount = 1;
    maxBufferCount = m_exynosconfig->current->bufInfo.num_picture_buffers;

    type = EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE;

    ret = m_allocBuffers(m_thumbnailBufferMgr,
                         planeCount, planeSize, bytesPerLine,
                         minBufferCount, maxBufferCount,
                         type, allocMode, true, needMmap);
    if (ret != NO_ERROR) {
        CLOGE2("m_thumbnailBufferMgr m_allocBuffers(minBufferCount=%d, maxBufferCount=%d) fail",
                minBufferCount, maxBufferCount);
        return ret;
    }

    return NO_ERROR;
}

bool ExynosCamera3::m_reprocessingFrameFactoryStartThreadFunc(void)
{
    status_t ret = 0;
    ExynosCamera3FrameFactory *factory = NULL;

    factory = m_frameFactory[FRAME_FACTORY_TYPE_REPROCESSING];
    if (factory == NULL) {
        CLOGE2("Can't start FrameFactory!!!! FrameFactory is NULL!!");

        return false;
    } else if (factory->isCreated() == false) {
        CLOGE2("Reprocessing FrameFactory is NOT created!");
        return false;
    }

    /* Set buffer manager */
    ret = m_setupReprocessingPipeline();
    if (ret != NO_ERROR) {
        CLOGE2("Failed to setupReprocessingPipeline. ret %d", ret);
        return false;
    }

    ret = factory->initPipes();
    if (ret < 0) {
        CLOGE2("Failed to initPipes. ret %d", ret);
        return false;
    }

    ret = m_startReprocessingFrameFactory(factory);
    if (ret < 0) {
        CLOGE2("Failed to startReprocessingFrameFactory");
        /* TODO: Need release buffers and error exit */
        return false;
    }

    return false;
}

status_t ExynosCamera3::m_startReprocessingFrameFactory(ExynosCamera3FrameFactory *factory)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = 0;

    CLOGD2("- IN -");

    ret = factory->preparePipes();
    if (ret < 0) {
        CLOGE2("m_reprocessingFrameFactory preparePipe fail");
        return ret;
    }

    /* stream on pipes */
    ret = factory->startPipes();
    if (ret < 0) {
        CLOGE2("m_reprocessingFrameFactory startPipe fail");
        return ret;
    }

    m_flagStartReprocessingFrameFactory = true;

    return NO_ERROR;
}

status_t ExynosCamera3::m_stopReprocessingFrameFactory(ExynosCamera3FrameFactory *factory)
{
    CLOGI2("- IN -");
    status_t ret = 0;

    if (factory != NULL) {
        ret = factory->stopPipes();
        if (ret < 0) {
            CLOGE2("m_reprocessingFrameFactory0>stopPipe() fail");
        }
    }

    CLOGD2("clear m_captureProcessList(Picture) Frame list");
    ret = m_clearList(&m_captureProcessList, &m_captureProcessLock);
    if (ret < 0) {
        CLOGE2("m_clearList fail");
        return ret;
    }

    m_flagStartReprocessingFrameFactory = false;

    return NO_ERROR;
}

status_t ExynosCamera3::m_checkBufferAvailable(uint32_t pipeId, ExynosCameraBufferManager *bufferMgr)
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
            CLOGW2("retry(%d) setupEntity for pipeId(%d)", retry, pipeId);
    } while(ret < 0 && retry < (TOTAL_WAITING_TIME/WAITING_TIME));

    return ret;
}

bool ExynosCamera3::m_startPictureBufferThreadFunc(void)
{
    int ret = 0;

    ret = m_setPictureBuffer();
    if (ret < 0) {
        CLOGE2("m_setPictureBuffer failed");

        /* TODO: Need release buffers and error exit */

        return false;
    }

    if (m_parameters->isReprocessing() == true) {
        ret = m_setReprocessingBuffer();
        if (ret < 0) {
            CLOGE2("m_setReprocessing Buffer fail");
            return ret;
        }
    }

    return false;
}

status_t ExynosCamera3::m_setPictureBuffer(void)
{
    int ret = 0;
    unsigned int planeSize[3] = {0};
    unsigned int bytesPerLine[3] = {0};
    int pictureW = 0, pictureH = 0, pictureFormat = 0;
    int planeCount = 0;
    int minBufferCount = 1;
    int maxBufferCount = 1;
    exynos_camera_buffer_type_t type = EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE;
    buffer_manager_allocation_mode_t allocMode = BUFFER_MANAGER_ALLOCATION_ONDEMAND;

    m_parameters->getMaxPictureSize(&pictureW, &pictureH);
    pictureFormat = m_parameters->getPictureFormat();
    if ((m_parameters->needGSCForCapture(getCameraId()) == true)) {
        planeSize[0] = pictureW * pictureH * 2;
        planeCount = 1;
        minBufferCount = 1;
        maxBufferCount = m_exynosconfig->current->bufInfo.num_picture_buffers;

        // Pre-allocate certain amount of buffers enough to fed into 3 JPEG save threads.
        if (m_parameters->getSeriesShotCount() > 0)
            minBufferCount = NUM_BURST_GSC_JPEG_INIT_BUFFER;

        ret = m_allocBuffers(m_gscBufferMgr, planeCount, planeSize, bytesPerLine, minBufferCount, maxBufferCount, type, allocMode, false, false);
        if (ret < 0) {
            CLOGE2("m_gscBufferMgr m_allocBuffers(minBufferCount=%d, maxBufferCount=%d) fail", minBufferCount, maxBufferCount);
            return ret;
        }
    }

    return ret;
}

status_t ExynosCamera3::m_generateInternalFrame(uint32_t frameCount, ExynosCamera3FrameFactory *factory, List<ExynosCameraFrame *> *list, Mutex *listLock, ExynosCameraFrame **newFrame)
{
    status_t ret = OK;
    *newFrame = NULL;

    CLOGV2("frameCount(%d)", frameCount);
    ret = m_searchFrameFromList(list, listLock, frameCount, newFrame);
    if (ret < 0) {
        CLOGE2("searchFrameFromList fail");
        return INVALID_OPERATION;
    }

    if (*newFrame == NULL) {
        *newFrame = factory->createNewFrame(frameCount);
        if (*newFrame == NULL) {
            CLOGE2("newFrame is NULL");
            return UNKNOWN_ERROR;
        }
        listLock->lock();
        list->push_back(*newFrame);
        listLock->unlock();
    }

    /* Set frame type into FRAME_TYPE_INTERNAL */
    ret = (*newFrame)->setFrameInfo(m_parameters, frameCount, FRAME_TYPE_INTERNAL);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Failed to setFrameInfo with INTERNAL. frameCount %d",
                __FUNCTION__, __LINE__, frameCount);
        return ret;
    }

    return ret;
}

bool ExynosCamera3::m_internalFrameThreadFunc(void)
{
    status_t ret = 0;
    int index = 0;
    ExynosCameraFrame *newFrame = NULL;

    CLOGV2("Enter m_internalFrameThreadFunc");

    /* 1. Get new internal frame */
    ret = m_internalFrameDoneQ->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGV2("wait timeout");
        } else {
            CLOGE2("wait and pop fail, ret(%d)", ret);
            /* TODO: doing exception handling */
        }
        return false;
    }

    CLOGV2("handle internal frame : previewStream frameCnt(%d) (%d)", newFrame->getFrameCount(), newFrame->getFrameType());

    /* 2. Redirection for the normal frame */
    if (newFrame->getFrameType() != FRAME_TYPE_INTERNAL) {
        CLOGE2("push to m_pipeFrameDoneQ handler : previewStream frameCnt(%d)", newFrame->getFrameCount());
        m_pipeFrameDoneQ[PIPE_3AA]->pushProcessQ(&newFrame);
        return true;
    }

    /* 3. Handle the internal frame for each pipe */
    ret = m_handleInternalFrame(newFrame);

    if (ret < 0) {
        CLOGE2("handle preview frame fail");
        return ret;
    }

    if (newFrame->isComplete() == true/* && newFrame->getFrameCapture() == false */) {
        ret = m_removeFrameFromList(&m_processList, &m_processLock, newFrame);

        if (ret < 0) {
            CLOGE2("remove frame from processList fail, ret(%d)", ret);
        }

        CLOGV2("internal frame complete, count(%d)", newFrame->getFrameCount());
        newFrame->decRef();
        m_frameMgr->deleteFrame(newFrame);
        newFrame = NULL;
        m_captureResultDoneCondition.signal();
    }

    return true;
}

bool ExynosCamera3::m_doInternalFrame(ExynosCameraRequest *request)
{
    status_t ret = NO_ERROR;
    bool internalFlag = false;

    struct camera2_shot_ext cur_shot_ext;
    struct camera2_shot_ext prev_shot_ext;

    memset(&cur_shot_ext, 0x00, sizeof(struct camera2_shot_ext));
    memset(&prev_shot_ext, 0x00, sizeof(struct camera2_shot_ext));

    ret = request->getServiceShot(&cur_shot_ext);
    if (ret != NO_ERROR) {
        CLOGE2("Get service shot fail, Request Key(%d), ret(%d)", request->getKey(), ret);
        return false;
    }

    ret = request->getPrevShot(&prev_shot_ext);
    if (ret != NO_ERROR) {
        CLOGE2("Get service previous shot fail, Request Key(%d), ret(%d)", request->getKey(), ret);
        return false;
    }

    if (cur_shot_ext.shot.ctl.aa.aeMode == AA_AEMODE_OFF || cur_shot_ext.shot.ctl.aa.mode == AA_CONTROL_OFF) {
        if ((cur_shot_ext.shot.ctl.sensor.exposureTime != prev_shot_ext.shot.ctl.sensor.exposureTime)
            || (cur_shot_ext.shot.ctl.sensor.frameDuration!= prev_shot_ext.shot.ctl.sensor.frameDuration)
            || (cur_shot_ext.shot.ctl.aa.vendor_isoValue != prev_shot_ext.shot.ctl.aa.vendor_isoValue)) {
            CLOGI2("Create internal frame for manual AE setting");
            internalFlag = true;
        }
    }

    if ((cur_shot_ext.shot.ctl.lens.opticalStabilizationMode == OPTICAL_STABILIZATION_MODE_STILL)
         && (prev_shot_ext.shot.ctl.lens.opticalStabilizationMode == OPTICAL_STABILIZATION_MODE_CENTERING)) {
        CLOGI2("Create internal frame for ois mode (OFF -> ON)");
        internalFlag = true;
    }

#if 0 //for test
    if (cur_shot_ext.shot.ctl.aa.aeLock != prev_shot_ext.shot.ctl.aa.aeLock ) {
        CLOGI2("Create internal frame for ae lock (%d -> %d)",
            prev_shot_ext.shot.ctl.aa.aeLock,cur_shot_ext.shot.ctl.aa.aeLock);
        internalFlag = true;
    }
#endif

    return internalFlag;
}

status_t ExynosCamera3::m_handleInternalFrame(ExynosCameraFrame *frame)
{
    ExynosCameraFrameEntity *entity = NULL;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer buffer;
    ExynosCameraBuffer t3acBuffer;
    ExynosCamera3FrameFactory *factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];
    ExynosCameraStream *stream = NULL;
    camera3_capture_result_t captureResult;
    camera3_notify_msg_t notityMsg;
    ExynosCameraRequest* request = NULL;
    ResultRequest resultRequest = NULL;

    struct camera2_shot_ext meta_shot_ext;
    struct camera2_dm *dm = NULL;
    uint32_t framecount = 0;
    int32_t reprocessingBayerMode = m_parameters->getReprocessingBayerMode();

    entity_state_t entityState = ENTITY_STATE_COMPLETE;
    status_t ret = OK;

    entity = frame->getFrameDoneFirstEntity();
    if (entity == NULL) {
        CLOGE2("current entity is NULL");
        /* TODO: doing exception handling */
        return true;
    }
    CLOGV2("handle internal frame : previewStream frameCnt(%d) entityID(%d)", frame->getFrameCount(), entity->getPipeId());

    switch(entity->getPipeId()) {
    case PIPE_3AA:
        /* Notify ShotDone to mainThread */
        framecount = frame->getFrameCount();
        m_shotDoneQ->pushProcessQ(&framecount);

        ret = frame->getSrcBuffer(entity->getPipeId(), &buffer);
        if (ret < 0) {
            CLOGE2("getSrcBuffer fail, pipeId(%d), ret(%d)", entity->getPipeId(), ret);
            return ret;
        }

        if (buffer.index >= 0) {
            ret = m_updateTimestamp(frame, &buffer, false);
            if (ret != NO_ERROR) {
                CLOGE2("[F%d B%d]Failed to updateTimestamp",
                        frame->getFrameCount(), buffer.index);
                return ret;
            }

            if (m_parameters->isFlite3aaOtf() == false)
               ret = m_putBuffers(m_fliteBufferMgr, buffer.index);
            else
               ret = m_putBuffers(m_3aaBufferMgr, buffer.index);

            if (ret < 0) {
                CLOGE2("put Buffer fail");
            }
        }

        frame->setMetaDataEnable(true);

        t3acBuffer.index = -1;

        if (frame->getRequest(PIPE_3AC) == true) {
            ret = frame->getDstBuffer(entity->getPipeId(), &t3acBuffer, factory->getNodeType(PIPE_3AC));
            if (ret != NO_ERROR) {
                CLOGE2("getDstBuffer fail, pipeId(%d), ret(%d)", entity->getPipeId(), ret);
            }
        }

        if (m_parameters->isReprocessing() == true) {
            if (m_captureSelector == NULL) {
                CLOGE2("m_captureSelector is NULL");
                return INVALID_OPERATION;
            }
        } else {
            if (m_sccCaptureSelector == NULL) {
                CLOGE2("m_sccCaptureSelector is NULL");
                return INVALID_OPERATION;
            }
        }

        if (0 <= t3acBuffer.index) {
            if (m_parameters->isUseYuvReprocessing() == true
                || frame->getFrameCapture() == true) {
                if (m_parameters->getHighSpeedRecording() == true) {
                    if (m_parameters->isUsing3acForIspc() == true)
                        ret = m_putBuffers(m_yuvCaptureBufferMgr, t3acBuffer.index);
                    else
                        ret = m_putBuffers(m_fliteBufferMgr, t3acBuffer.index);

                    if (ret < 0) {
                        CLOGE2("m_putBuffers(m_fliteBufferMgr, %d) fail", t3acBuffer.index);
                        break;
                    }
                } else {
                    entity_buffer_state_t bufferstate = ENTITY_BUFFER_STATE_NOREQ;
                    ret = frame->getDstBufferState(entity->getPipeId(), &bufferstate, factory->getNodeType(PIPE_3AC));
                    if (ret == NO_ERROR && bufferstate != ENTITY_BUFFER_STATE_ERROR) {
                        if (m_parameters->isUseYuvReprocessing() == false
                            && m_parameters->isUsing3acForIspc() == true)
                            ret = m_sccCaptureSelector->manageFrameHoldListForDynamicBayer(frame);
                        else
                            ret = m_captureSelector->manageFrameHoldList(frame, entity->getPipeId(), false, factory->getNodeType(PIPE_3AC));

                        if (ret < 0) {
                            CLOGE2("manageFrameHoldList fail");
                            return ret;
                        }
                    } else {
                        if (m_parameters->isUsing3acForIspc() == true)
                            ret = m_putBuffers(m_yuvCaptureBufferMgr, t3acBuffer.index);
                        else
                            ret = m_putBuffers(m_fliteBufferMgr, t3acBuffer.index);

                        if (ret < 0) {
                            CLOGE2("m_putBuffers(m_fliteBufferMgr, %d) fail", t3acBuffer.index);
                            break;
                        }
                    }
                }
            } else {
                if (reprocessingBayerMode == REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON) {
                    CLOGW2("frame->getRequest(PIPE_3AC) == false. so, just m_putBuffers(t3acBuffer.index(%d)..., pipeId(%d), ret(%d)",
                        t3acBuffer.index, entity->getPipeId(), ret);
                }

                if (m_parameters->isUsing3acForIspc() == true)
                    ret = m_putBuffers(m_yuvCaptureBufferMgr, t3acBuffer.index);
                else
                    ret = m_putBuffers(m_fliteBufferMgr, t3acBuffer.index);

                if (ret < 0) {
                    CLOGE2("m_putBuffers(m_fliteBufferMgr, %d) fail", t3acBuffer.index);
                    break;
                }
            }
        }

        break;
    case PIPE_VRA:
        ret = frame->getDstBuffer(entity->getPipeId(), &buffer);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, entity->getPipeId(), ret);
            return ret;
        }

        if (buffer.index >= 0) {
            ret = m_vraBufferMgr->putBuffer(buffer.index, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL);
            if (ret != NO_ERROR)
                CLOGW("WARN(%s[%d]):Put VRA buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        break;
    case PIPE_FLITE:
        /* TODO: HACK: Will be removed, this is driver's job */
        if (m_parameters->isFlite3aaOtf() == true) {
            ret = m_handleBayerBuffer(frame);
            if (ret < NO_ERROR) {
                CLOGE("ERR(%s[%d]):Handle bayer buffer failed, framecount(%d), pipeId(%d), ret(%d)",
                        __FUNCTION__, __LINE__, frame->getFrameCount(), entity->getPipeId(), ret);
                return ret;
            }
        } else {
            ret = frame->getDstBuffer(entity->getPipeId(), &buffer);

            if (ret < 0) {
                CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                        __FUNCTION__, __LINE__, entity->getPipeId(), ret);
                return ret;
            }

            CLOGV("DEBUG(%s[%d]):Deliver Flite Buffer to 3AA. driver->framecount %d hal->framecount %d",
                    __FUNCTION__, __LINE__,
                    getMetaDmRequestFrameCount((struct camera2_shot_ext *)buffer.addr[buffer.planeCount-1]),
                    frame->getFrameCount());

            ret = m_setupEntity(PIPE_3AA, frame, &buffer, NULL);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):setSrcBuffer failed, pipeId(%d), ret(%d)",
                        __FUNCTION__, __LINE__, PIPE_3AA, ret);
                return ret;
            }

            factory->pushFrameToPipe(&frame, PIPE_3AA);
        }

        break;
    default:
        CLOGE2("Invalid pipe ID");
        break;
    }

    ret = frame->setEntityState(entity->getPipeId(), entityState);
    if (ret < 0) {
        CLOGE2("setEntityState fail, pipeId(%d), state(%d), ret(%d)", entity->getPipeId(), ENTITY_STATE_COMPLETE, ret);
        return ret;
    }

    return ret;
}

#ifdef MONITOR_LOG_SYNC
uint32_t ExynosCamera3::m_getSyncLogId(void)
{
    return ++cameraSyncLogId;
}
#endif

bool ExynosCamera3::m_monitorThreadFunc(void)
{
    CLOGV("INFO(%s[%d]):", __FUNCTION__, __LINE__);

    int *threadState;
    struct timeval dqTime;
    uint64_t *timeInterval;
    int *countRenew;
    int camId = getCameraId();
    int ret = NO_ERROR;
    int loopCount = 0;

    int dtpStatus = 0;
    int pipeIdFlite = 0;
    int pipeIdScp = 0;
    ExynosCamera3FrameFactory *factory = NULL;

    for (loopCount = 0; loopCount < MONITOR_THREAD_INTERVAL; loopCount += (MONITOR_THREAD_INTERVAL/20)) {
        if (m_flushFlag == true) {
            CLOGI2("m_flushFlag(%d)", m_flushFlag);
            return false;
        }
        usleep(MONITOR_THREAD_INTERVAL/20);
    }

    if (m_parameters->isFlite3aaOtf() == true || getCameraId() == CAMERA_ID_BACK) {
        pipeIdFlite = PIPE_FLITE;
        pipeIdScp = PIPE_3AA;
    } else {
        pipeIdFlite = PIPE_FLITE_FRONT;
        pipeIdScp = PIPE_3AA_FRONT;
    }

    factory = m_frameFactory[FRAME_FACTORY_TYPE_CAPTURE_PREVIEW];
    if (factory == NULL) {
        CLOGE2("frameFactory is NULL");
        return false;
    }

    if (factory->checkPipeThreadRunning(pipeIdScp) == false) {
        CLOGE2("Scp pipe is not running.. Skip monitoring.");
        return false;
    }
#ifdef MONITOR_LOG_SYNC
    uint32_t pipeIdIsp = 0;

    if (m_parameters->isFlite3aaOtf() == true || getCameraId() == CAMERA_ID_BACK)
        pipeIdIsp = PIPE_3AA;
    else
        pipeIdIsp = PIPE_3AA_FRONT;

    /* If it is not front camera in dual and sensor pipe is running, do sync log */
    if (factory->checkPipeThreadRunning(pipeIdIsp) &&
        !(getCameraId() == CAMERA_ID_FRONT && m_parameters->getDualMode())) {
        if (!(m_syncLogDuration % MONITOR_LOG_SYNC_INTERVAL)) {
            uint32_t syncLogId = m_getSyncLogId();
            CLOGI2("@FIMC_IS_SYNC %d", syncLogId);
            factory->syncLog(pipeIdIsp, syncLogId);
        }
        m_syncLogDuration++;
    }
#endif
    factory->getControl(V4L2_CID_IS_G_DTPSTATUS, &dtpStatus, pipeIdFlite);

    if (dtpStatus == 1) {
        CLOGE2("(%d)", dtpStatus);
        dump();

#if 0//def CAMERA_GED_FEATURE
        /* in GED */
        android_printAssert(NULL, LOG_TAG, "killed by itself");
#else
        /* specifically defined */
        /* m_notifyCb(CAMERA_MSG_ERROR, 1002, 0, m_callbackCookie); */
        /* or */
        /* android_printAssert(NULL, LOG_TAG, "killed by itself"); */
#endif
        return false;
    }

#ifdef SENSOR_OVERFLOW_CHECK
    factory->getControl(V4L2_CID_IS_G_DTPSTATUS, &dtpStatus, pipeIdFlite);
    if (dtpStatus == 1) {
        CLOGE2("(%d)", dtpStatus);
        dump();
#if 0//def CAMERA_GED_FEATURE
        /* in GED */
        android_printAssert(NULL, LOG_TAG, "killed by itself");
#else
        /* specifically defined */
        /* m_notifyCb(CAMERA_MSG_ERROR, 1002, 0, m_callbackCookie); */
        /* or */
        /* android_printAssert(NULL, LOG_TAG, "killed by itself"); */
#endif
        return false;
    }
#endif
    factory->getThreadState(&threadState, pipeIdScp);
    factory->getThreadRenew(&countRenew, pipeIdScp);

    if ((*threadState == ERROR_POLLING_DETECTED) || (*countRenew > ERROR_DQ_BLOCKED_COUNT)) {
        CLOGE2("(%d)", *threadState);
        if((*countRenew > ERROR_DQ_BLOCKED_COUNT))
            CLOGE2("ERROR_DQ_BLOCKED) ; ERROR_DQ_BLOCKED_COUNT =20");

        dump();
#if 0//def CAMERA_GED_FEATURE
        /* in GED */
        android_printAssert(NULL, LOG_TAG, "killed by itself");
#else
        /* specifically defined */
        /* m_notifyCb(CAMERA_MSG_ERROR, 1002, 0, m_callbackCookie); */
        /* or */
        /* android_printAssert(NULL, LOG_TAG, "killed by itself"); */
#endif
        return false;
    } else {
        CLOGV2(" (%d)", *threadState);
    }

    gettimeofday(&dqTime, NULL);
    factory->getThreadInterval(&timeInterval, pipeIdScp);

    CLOGV2("Thread IntervalTime [%lld]", *timeInterval);
    CLOGV2("Thread Renew Count [%d]", *countRenew);

    factory->incThreadRenew(pipeIdScp);

    return true;
}

}; /* namespace android */
