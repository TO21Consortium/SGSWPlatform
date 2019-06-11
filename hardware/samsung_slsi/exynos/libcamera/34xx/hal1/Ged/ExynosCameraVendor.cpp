/*
**
** Copyright 2014, Samsung Electronics Co. LTD
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
#define LOG_TAG "ExynosCameraGed"
#include <cutils/log.h>

#include "ExynosCamera.h"

namespace android {

void ExynosCamera::vendorSpecificConstructor(__unused int cameraId, __unused camera_device_t *dev)
{
    CLOGI("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

#ifdef RAWDUMP_CAPTURE
    /* RawCaptureDump Thread */
    m_RawCaptureDumpThread = new mainCameraThread(this, &ExynosCamera::m_RawCaptureDumpThreadFunc, "m_RawCaptureDumpThread");
    CLOGD("DEBUG(%s):RawCaptureDumpThread created", __FUNCTION__);
#endif

    CLOGI("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return;
}

void ExynosCamera::vendorSpecificDestructor(void)
{
    CLOGI("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);
    CLOGI("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return;
}

status_t ExynosCamera::startRecording()
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    if (m_parameters != NULL) {
        if (m_parameters->getVisionMode() == true) {
            CLOGW("WRN(%s[%d]): Vision mode does not support", __FUNCTION__, __LINE__);
            android_printAssert(NULL, LOG_TAG, "Cannot support this operation");

            return INVALID_OPERATION;
        }
    }

    int ret = 0;
    ExynosCameraActivityAutofocus *autoFocusMgr = m_exynosCameraActivityControl->getAutoFocusMgr();
    ExynosCameraActivityFlash *flashMgr = m_exynosCameraActivityControl->getFlashMgr();

    if (m_getRecordingEnabled() == true) {
        CLOGW("WARN(%s[%d]):m_recordingEnabled equals true", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

#ifdef USE_FD_AE
    if (m_parameters != NULL) {
        if (m_parameters->getFaceDetectionMode() == false) {
            m_startFaceDetection(false);
        } else {
            /* stay current fd mode */
        }
    } else {
        CLOGW("(%s[%d]):m_parameters is NULL", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }
#endif


    /* Do start recording process */
    ret = m_startRecordingInternal();
    if (ret < 0) {
        CLOGE("ERR");
        return ret;
    }

    m_lastRecordingTimeStamp  = 0;
    m_recordingStartTimeStamp = 0;
    m_recordingFrameSkipCount = 0;

    m_setRecordingEnabled(true);
    m_parameters->setRecordingRunning(true);

    autoFocusMgr->setRecordingHint(true);
    flashMgr->setRecordingHint(true);

func_exit:
    /* wait for initial preview skip */
    if (m_parameters != NULL) {
        int retry = 0;
        while (m_parameters->getFrameSkipCount() > 0 && retry < 3) {
            retry++;
            usleep(33000);
            CLOGI("INFO(%s[%d]): -OUT- (frameSkipCount:%d) (retry:%d)", __FUNCTION__, __LINE__, m_frameSkipCount, retry);
        }
    }

    return NO_ERROR;
}

void ExynosCamera::stopRecording()
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    if (m_parameters != NULL) {
        if (m_parameters->getVisionMode() == true) {
            CLOGW("WRN(%s[%d]): Vision mode does not support", __FUNCTION__, __LINE__);
            android_printAssert(NULL, LOG_TAG, "Cannot support this operation");

            return;
        }
    }

    int ret = 0;
    ExynosCameraActivityAutofocus *autoFocusMgr = m_exynosCameraActivityControl->getAutoFocusMgr();
    ExynosCameraActivityFlash *flashMgr = m_exynosCameraActivityControl->getFlashMgr();
    m_skipCount = 0;

    if (m_getRecordingEnabled() == false) {
        return;
    }
    m_setRecordingEnabled(false);
    m_parameters->setRecordingRunning(false);
    /* Do stop recording process */

    ret = m_stopRecordingInternal();
    if (ret < 0)
        CLOGE("ERR(%s[%d]):m_stopRecordingInternal fail", __FUNCTION__, __LINE__);

#ifdef USE_FD_AE
    if (m_parameters != NULL) {
        m_startFaceDetection(m_parameters->getFaceDetectionMode(false));
    }
#endif

    autoFocusMgr->setRecordingHint(false);
    flashMgr->setRecordingHint(false);
    flashMgr->setNeedFlashOffDelay(false);
}

status_t ExynosCamera::setParameters(const CameraParameters& params)
{
    status_t ret = NO_ERROR;
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    if(m_parameters == NULL)
        return INVALID_OPERATION;

#ifdef SCALABLE_ON
    m_parameters->setScalableSensorMode(true);
#else
    m_parameters->setScalableSensorMode(false);
#endif

    ret = m_parameters->setParameters(params);
#if 1
    /* HACK Reset Preview Flag*/
    if (m_parameters->getRestartPreview() == true && m_previewEnabled == true) {
        m_resetPreview = true;
        ret = m_restartPreviewInternal();
        m_resetPreview = false;
        CLOGI("INFO(%s[%d]) m_resetPreview(%d)", __FUNCTION__, __LINE__, m_resetPreview);

        if (ret < 0)
            CLOGE("(%s[%d]): restart preview internal fail", __FUNCTION__, __LINE__);
    }
#endif
    return ret;

}

status_t ExynosCamera::m_doFdCallbackFunc(ExynosCameraFrame *frame)
{
    ExynosCameraDurationTimer       m_fdcallbackTimer;
    long long                       m_fdcallbackTimerTime;

    struct camera2_shot_ext *meta_shot_ext = NULL;
    meta_shot_ext = new struct camera2_shot_ext;
    if (meta_shot_ext == NULL) {
        CLOGE("ERR(%s[%d]) meta_shot_ext is null", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    memset(meta_shot_ext, 0x00, sizeof(struct camera2_shot_ext));
    if (frame->getDynamicMeta(meta_shot_ext) != NO_ERROR) {
        CLOGE("ERR(%s[%d]) meta_shot_ext is null", __FUNCTION__, __LINE__);
        if (meta_shot_ext) {
            delete meta_shot_ext;
            meta_shot_ext = NULL;
        }
        return INVALID_OPERATION;
    }

    if (m_flagStartFaceDetection == true) {
        int id[NUM_OF_DETECTED_FACES];
        int score[NUM_OF_DETECTED_FACES];
        ExynosRect2 detectedFace[NUM_OF_DETECTED_FACES];
        ExynosRect2 detectedLeftEye[NUM_OF_DETECTED_FACES];
        ExynosRect2 detectedRightEye[NUM_OF_DETECTED_FACES];
        ExynosRect2 detectedMouth[NUM_OF_DETECTED_FACES];
        int numOfDetectedFaces = 0;
        int num = 0;
        struct camera2_dm *dm = NULL;
        int previewW, previewH;

        memset(&id, 0x00, sizeof(int) * NUM_OF_DETECTED_FACES);
        memset(&score, 0x00, sizeof(int) * NUM_OF_DETECTED_FACES);

        m_parameters->getHwPreviewSize(&previewW, &previewH);

        camera2_node_group node_group_info;
        frame->getNodeGroupInfo(&node_group_info, PERFRAME_INFO_3AA);

        dm = &(meta_shot_ext->shot.dm);
        if (dm == NULL) {
            CLOGE("ERR(%s[%d]) dm data is null", __FUNCTION__, __LINE__);
            delete meta_shot_ext;
            meta_shot_ext = NULL;
            return INVALID_OPERATION;
        }

        CLOGV("DEBUG(%s[%d]) faceDetectMode(%d)", __FUNCTION__, __LINE__, dm->stats.faceDetectMode);
        CLOGV("[%d %d]", dm->stats.faceRectangles[0][0], dm->stats.faceRectangles[0][1]);
        CLOGV("[%d %d]", dm->stats.faceRectangles[0][2], dm->stats.faceRectangles[0][3]);

       num = NUM_OF_DETECTED_FACES;
        if (getMaxNumDetectedFaces() < num)
            num = getMaxNumDetectedFaces();

        switch (dm->stats.faceDetectMode) {
        case FACEDETECT_MODE_SIMPLE:
        case FACEDETECT_MODE_FULL:
            break;
        case FACEDETECT_MODE_OFF:
        default:
            num = 0;
            break;
        }

        for (int i = 0; i < num; i++) {
            if (dm->stats.faceIds[i]) {
                switch (dm->stats.faceDetectMode) {
                case FACEDETECT_MODE_FULL:
                    id[i] = dm->stats.faceIds[i];
                    score[i] = dm->stats.faceScores[i];

                    detectedLeftEye[i].x1
                        = detectedLeftEye[i].y1
                        = detectedLeftEye[i].x2
                        = detectedLeftEye[i].y2 = -1;

                    detectedRightEye[i].x1
                        = detectedRightEye[i].y1
                        = detectedRightEye[i].x2
                        = detectedRightEye[i].y2 = -1;

                    detectedMouth[i].x1
                        = detectedMouth[i].y1
                        = detectedMouth[i].x2
                        = detectedMouth[i].y2 = -1;
                case FACEDETECT_MODE_SIMPLE:
                    if (m_parameters->isMcscVraOtf() == false) {
                        int vraWidth = 0, vraHeight = 0;
                        m_parameters->getHwVraInputSize(&vraWidth, &vraHeight);

                        detectedFace[i].x1 = m_calibratePosition(vraWidth, previewW, dm->stats.faceRectangles[i][0]);
                        detectedFace[i].y1 = m_calibratePosition(vraHeight, previewH, dm->stats.faceRectangles[i][1]);
                        detectedFace[i].x2 = m_calibratePosition(vraWidth, previewW, dm->stats.faceRectangles[i][2]);
                        detectedFace[i].y2 = m_calibratePosition(vraHeight, previewH, dm->stats.faceRectangles[i][3]);
                    } else if ((int)(node_group_info.leader.output.cropRegion[2]) < previewW
                               || (int)(node_group_info.leader.output.cropRegion[3]) < previewH) {
                        detectedFace[i].x1 = m_calibratePosition(node_group_info.leader.output.cropRegion[2], previewW, dm->stats.faceRectangles[i][0]);
                        detectedFace[i].y1 = m_calibratePosition(node_group_info.leader.output.cropRegion[3], previewH, dm->stats.faceRectangles[i][1]);
                        detectedFace[i].x2 = m_calibratePosition(node_group_info.leader.output.cropRegion[2], previewW, dm->stats.faceRectangles[i][2]);
                        detectedFace[i].y2 = m_calibratePosition(node_group_info.leader.output.cropRegion[3], previewH, dm->stats.faceRectangles[i][3]);
                    } else {
                        detectedFace[i].x1 = dm->stats.faceRectangles[i][0];
                        detectedFace[i].y1 = dm->stats.faceRectangles[i][1];
                        detectedFace[i].x2 = dm->stats.faceRectangles[i][2];
                        detectedFace[i].y2 = dm->stats.faceRectangles[i][3];
                    }
                    numOfDetectedFaces++;
                    break;
                default:
                    break;
                }
            }
        }

        if (0 < numOfDetectedFaces) {
            /*
             * camera.h
             * width   : -1000~1000
             * height  : -1000~1000
             * if eye, mouth is not detectable : -2000, -2000.
             */
            memset(m_faces, 0, sizeof(camera_face_t) * NUM_OF_DETECTED_FACES);

            int realNumOfDetectedFaces = 0;

            for (int i = 0; i < numOfDetectedFaces; i++) {
                /*
                 * over 50s, we will catch
                 * if (score[i] < 50)
                 *    continue;
                */
                m_faces[realNumOfDetectedFaces].rect[0] = m_calibratePosition(previewW, 2000, detectedFace[i].x1) - 1000;
                m_faces[realNumOfDetectedFaces].rect[1] = m_calibratePosition(previewH, 2000, detectedFace[i].y1) - 1000;
                m_faces[realNumOfDetectedFaces].rect[2] = m_calibratePosition(previewW, 2000, detectedFace[i].x2) - 1000;
                m_faces[realNumOfDetectedFaces].rect[3] = m_calibratePosition(previewH, 2000, detectedFace[i].y2) - 1000;

                m_faces[realNumOfDetectedFaces].id = id[i];
                m_faces[realNumOfDetectedFaces].score = score[i] > 100 ? 100 : score[i];

                m_faces[realNumOfDetectedFaces].left_eye[0] = (detectedLeftEye[i].x1 < 0) ? -2000 : m_calibratePosition(previewW, 2000, detectedLeftEye[i].x1) - 1000;
                m_faces[realNumOfDetectedFaces].left_eye[1] = (detectedLeftEye[i].y1 < 0) ? -2000 : m_calibratePosition(previewH, 2000, detectedLeftEye[i].y1) - 1000;

                m_faces[realNumOfDetectedFaces].right_eye[0] = (detectedRightEye[i].x1 < 0) ? -2000 : m_calibratePosition(previewW, 2000, detectedRightEye[i].x1) - 1000;
                m_faces[realNumOfDetectedFaces].right_eye[1] = (detectedRightEye[i].y1 < 0) ? -2000 : m_calibratePosition(previewH, 2000, detectedRightEye[i].y1) - 1000;

                m_faces[realNumOfDetectedFaces].mouth[0] = (detectedMouth[i].x1 < 0) ? -2000 : m_calibratePosition(previewW, 2000, detectedMouth[i].x1) - 1000;
                m_faces[realNumOfDetectedFaces].mouth[1] = (detectedMouth[i].y1 < 0) ? -2000 : m_calibratePosition(previewH, 2000, detectedMouth[i].y1) - 1000;

                CLOGV("face posision(cal:%d,%d %dx%d)(org:%d,%d %dx%d), id(%d), score(%d)",
                    m_faces[realNumOfDetectedFaces].rect[0], m_faces[realNumOfDetectedFaces].rect[1],
                    m_faces[realNumOfDetectedFaces].rect[2], m_faces[realNumOfDetectedFaces].rect[3],
                    detectedFace[i].x1, detectedFace[i].y1,
                    detectedFace[i].x2, detectedFace[i].y2,
                    m_faces[realNumOfDetectedFaces].id,
                    m_faces[realNumOfDetectedFaces].score);

                CLOGV("DEBUG(%s[%d]): left eye(%d,%d), right eye(%d,%d), mouth(%dx%d), num of facese(%d)",
                        __FUNCTION__, __LINE__,
                        m_faces[realNumOfDetectedFaces].left_eye[0],
                        m_faces[realNumOfDetectedFaces].left_eye[1],
                        m_faces[realNumOfDetectedFaces].right_eye[0],
                        m_faces[realNumOfDetectedFaces].right_eye[1],
                        m_faces[realNumOfDetectedFaces].mouth[0],
                        m_faces[realNumOfDetectedFaces].mouth[1],
                        realNumOfDetectedFaces
                     );

                realNumOfDetectedFaces++;
            }

            m_frameMetadata.number_of_faces = realNumOfDetectedFaces;
            m_frameMetadata.faces = m_faces;

            m_faceDetected = true;
            m_fdThreshold = 0;
        } else {
            if (m_faceDetected == true && m_fdThreshold < NUM_OF_DETECTED_FACES_THRESHOLD) {
                /* waiting the unexpected fail about face detected */
                m_fdThreshold++;
            } else {
                if (0 < m_frameMetadata.number_of_faces)
                    memset(m_faces, 0, sizeof(camera_face_t) * NUM_OF_DETECTED_FACES);

                m_frameMetadata.number_of_faces = 0;
                m_frameMetadata.faces = m_faces;
                m_fdThreshold = 0;
                m_faceDetected = false;
            }
        }
    } else {
        if (0 < m_frameMetadata.number_of_faces)
            memset(m_faces, 0, sizeof(camera_face_t) * NUM_OF_DETECTED_FACES);

        m_frameMetadata.number_of_faces = 0;
        m_frameMetadata.faces = m_faces;

        m_fdThreshold = 0;

        m_faceDetected = false;
    }

#ifdef TOUCH_AE
    if(((m_parameters->getMeteringMode() >= METERING_MODE_CENTER_TOUCH) && (m_parameters->getMeteringMode() <= METERING_MODE_AVERAGE_TOUCH))
        && ((meta_shot_ext->shot.dm.aa.aeState == AE_STATE_CONVERGED) || (meta_shot_ext->shot.dm.aa.aeState == AE_STATE_FLASH_REQUIRED))) {
        m_notifyCb(AE_RESULT, 0, 0, m_callbackCookie);
        CLOGV("INFO(%s[%d]): AE_RESULT(%d)", __FUNCTION__, __LINE__, meta_shot_ext->shot.dm.aa.aeState);
    }
#endif

    delete meta_shot_ext;
    meta_shot_ext = NULL;

    m_fdcallbackTimer.start();

    if (m_parameters->msgTypeEnabled(CAMERA_MSG_PREVIEW_METADATA) &&
            (m_flagStartFaceDetection || m_flagLLSStart || m_flagLightCondition))
    {
        setBit(&m_callbackState, CALLBACK_STATE_PREVIEW_META, false);
        m_dataCb(CAMERA_MSG_PREVIEW_METADATA, m_fdCallbackHeap, 0, &m_frameMetadata, m_callbackCookie);
        clearBit(&m_callbackState, CALLBACK_STATE_PREVIEW_META, false);
    }
    m_fdcallbackTimer.stop();
    m_fdcallbackTimerTime = m_fdcallbackTimer.durationUsecs();

    if((int)m_fdcallbackTimerTime / 1000 > 50) {
        CLOGD("INFO(%s[%d]): FD callback duration time : (%d)mec", __FUNCTION__, __LINE__, (int)m_fdcallbackTimerTime / 1000);
    }

    return NO_ERROR;
}

status_t ExynosCamera::sendCommand(int32_t command, int32_t arg1, __unused int32_t arg2)
{
    ExynosCameraActivityUCTL *uctlMgr = NULL;
    CLOGV("INFO(%s[%d])", __FUNCTION__, __LINE__);

    if (m_parameters != NULL) {
        if (m_parameters->getVisionMode() == true) {
            CLOGW("WRN(%s[%d]): Vision mode does not support", __FUNCTION__, __LINE__);
            android_printAssert(NULL, LOG_TAG, "Cannot support this operation");

            return INVALID_OPERATION;
        }
    } else {
        CLOGE("ERR(%s):m_parameters is NULL", __FUNCTION__);
        return INVALID_OPERATION;
    }

    /* TO DO : implemented based on the command */
    switch(command) {
    case CAMERA_CMD_START_FACE_DETECTION:
    case CAMERA_CMD_STOP_FACE_DETECTION:
        if (getMaxNumDetectedFaces() == 0) {
            CLOGE("ERR(%s):getMaxNumDetectedFaces == 0", __FUNCTION__);
            return BAD_VALUE;
        }

        if (arg1 == CAMERA_FACE_DETECTION_SW) {
            CLOGD("DEBUG(%s):only support HW face dectection", __FUNCTION__);
            return BAD_VALUE;
        }

        if (command == CAMERA_CMD_START_FACE_DETECTION) {
            CLOGD("DEBUG(%s):CAMERA_CMD_START_FACE_DETECTION is called!", __FUNCTION__);
            if (m_flagStartFaceDetection == false
                && startFaceDetection() == false) {
                CLOGE("ERR(%s):startFaceDetection() fail", __FUNCTION__);
                return BAD_VALUE;
            }
        } else {
            CLOGD("DEBUG(%s):CAMERA_CMD_STOP_FACE_DETECTION is called!", __FUNCTION__);
            if ( m_flagStartFaceDetection == true
                && stopFaceDetection() == false) {
                CLOGE("ERR(%s):stopFaceDetection() fail", __FUNCTION__);
                return BAD_VALUE;
            }
        }
        break;
    case 1351: /*CAMERA_CMD_AUTO_LOW_LIGHT_SET */
        CLOGD("DEBUG(%s):CAMERA_CMD_AUTO_LOW_LIGHT_SET is called!%d", __FUNCTION__, arg1);
        if(arg1) {
            if( m_flagLLSStart != true) {
                m_flagLLSStart = true;
            }
        } else {
            m_flagLLSStart = false;
        }
        break;
    case 1801: /* HAL_ENABLE_LIGHT_CONDITION*/
        CLOGD("DEBUG(%s):HAL_ENABLE_LIGHT_CONDITION is called!%d", __FUNCTION__, arg1);
        if(arg1) {
            m_flagLightCondition = true;
        } else {
            m_flagLightCondition = false;
        }
        break;
    /* 1510: CAMERA_CMD_SET_FLIP */
    case 1510 :
        CLOGD("DEBUG(%s):CAMERA_CMD_SET_FLIP is called!%d", __FUNCTION__, arg1);
        m_parameters->setFlipHorizontal(arg1);
        break;
    /* 1521: CAMERA_CMD_DEVICE_ORIENTATION */
    case 1521:
        CLOGD("DEBUG(%s):CAMERA_CMD_DEVICE_ORIENTATION is called!%d", __FUNCTION__, arg1);
        m_parameters->setDeviceOrientation(arg1);
        uctlMgr = m_exynosCameraActivityControl->getUCTLMgr();
        if (uctlMgr != NULL)
            uctlMgr->setDeviceRotation(m_parameters->getFdOrientation());
        break;
    /*1641: CAMERA_CMD_ADVANCED_MACRO_FOCUS*/
    case 1641:
        CLOGD("DEBUG(%s):CAMERA_CMD_ADVANCED_MACRO_FOCUS is called!%d", __FUNCTION__, arg1);
        m_parameters->setAutoFocusMacroPosition(ExynosCameraActivityAutofocus::AUTOFOCUS_MACRO_POSITION_CENTER);
        break;
    /*1642: CAMERA_CMD_FOCUS_LOCATION*/
    case 1642:
        CLOGD("DEBUG(%s):CAMERA_CMD_FOCUS_LOCATION is called!%d", __FUNCTION__, arg1);
        m_parameters->setAutoFocusMacroPosition(ExynosCameraActivityAutofocus::AUTOFOCUS_MACRO_POSITION_CENTER_UP);
        break;
    /*1661: CAMERA_CMD_START_ZOOM */
    case 1661:
        CLOGD("DEBUG(%s):CAMERA_CMD_START_ZOOM is called!", __FUNCTION__);
        m_parameters->setZoomActiveOn(true);
        m_parameters->setFocusModeLock(true);
        break;
    /*1662: CAMERA_CMD_STOP_ZOOM */
    case 1662:
        CLOGD("DEBUG(%s):CAMERA_CMD_STOP_ZOOM is called!", __FUNCTION__);
        m_parameters->setZoomActiveOn(false);
        m_parameters->setFocusModeLock(false);
        break;
    default:
        CLOGV("DEBUG(%s):unexpectect command(%d)", __FUNCTION__, command);
        break;
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_startRecordingInternal(void)
{
    int ret = 0;

    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    unsigned int planeSize[EXYNOS_CAMERA_BUFFER_MAX_PLANES]    = {0};
    int videoW = 0, videoH = 0;
    int planeCount  = 1;
    int minBufferCount = 1;
    int maxBufferCount = 1;
    exynos_camera_buffer_type_t type = EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE;
    buffer_manager_allocation_mode_t allocMode = BUFFER_MANAGER_ALLOCATION_SILENT;
    int heapFd = 0;

    m_parameters->getVideoSize(&videoW, &videoH);
    CLOGD("DEBUG(%s[%d]):videoSize = %d x %d",  __FUNCTION__, __LINE__, videoW, videoH);

    m_doCscRecording = true;
    if (m_parameters->doCscRecording() == true) {
        m_recordingBufferCount = m_exynosconfig->current->bufInfo.num_recording_buffers;
        CLOGI("INFO(%s[%d]):do Recording CSC !!! m_recordingBufferCount(%d)", __FUNCTION__, __LINE__, m_recordingBufferCount);
    } else {
        m_doCscRecording = false;
        m_recordingBufferCount = m_exynosconfig->current->bufInfo.num_preview_buffers;
        CLOGI("INFO(%s[%d]):skip Recording CSC !!! m_recordingBufferCount(%d->%d)",
            __FUNCTION__, __LINE__, m_exynosconfig->current->bufInfo.num_recording_buffers, m_recordingBufferCount);
    }

    /* clear previous recording frame */
    CLOGD("DEBUG(%s[%d]):Recording m_recordingProcessList(%d) IN",
        __FUNCTION__, __LINE__, m_recordingProcessList.size());
    m_recordingListLock.lock();
    ret = m_clearList(&m_recordingProcessList);
    if (ret < 0) {
        CLOGE("ERR(%s):m_clearList fail", __FUNCTION__);
    }
    m_recordingListLock.unlock();
    CLOGD("DEBUG(%s[%d]):Recording m_recordingProcessList(%d) OUT",
        __FUNCTION__, __LINE__, m_recordingProcessList.size());

    for (int32_t i = 0; i < m_recordingBufferCount; i++) {
        m_recordingTimeStamp[i] = 0L;
        m_recordingBufAvailable[i] = true;
    }

    /* alloc recording Callback Heap */
    m_recordingCallbackHeap = m_getMemoryCb(-1, sizeof(struct addrs), m_recordingBufferCount, &heapFd);
    if (!m_recordingCallbackHeap || m_recordingCallbackHeap->data == MAP_FAILED) {
        CLOGE("ERR(%s[%d]):m_getMemoryCb(%zd) fail", __FUNCTION__, __LINE__, sizeof(struct addrs));
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_doCscRecording == true) {
        /* alloc recording Image buffer */
        planeSize[0] = ROUND_UP(videoW, CAMERA_MAGIC_ALIGN) * ROUND_UP(videoH, CAMERA_MAGIC_ALIGN) + MFC_7X_BUFFER_OFFSET;
        planeSize[1] = ROUND_UP(videoW, CAMERA_MAGIC_ALIGN) * ROUND_UP(videoH / 2, CAMERA_MAGIC_ALIGN) + MFC_7X_BUFFER_OFFSET;
        planeCount   = 2;
        if( m_parameters->getHighSpeedRecording() == true)
            minBufferCount = m_recordingBufferCount;
        else
            minBufferCount = 1;

        maxBufferCount = m_recordingBufferCount;

        ret = m_allocBuffers(m_recordingBufferMgr, planeCount, planeSize, bytesPerLine, minBufferCount, maxBufferCount, type, allocMode, false, true);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_recordingBufferMgr m_allocBuffers(minBufferCount=%d, maxBufferCount=%d) fail",
                __FUNCTION__, __LINE__, minBufferCount, maxBufferCount);
        }
    }

    if (m_doCscRecording == true) {
        int recPipeId = PIPE_GSC_VIDEO;

        m_previewFrameFactory->startThread(recPipeId);

        if (m_recordingQ->getSizeOfProcessQ() > 0) {
            CLOGE("ERR(%s[%d]):m_startRecordingInternal recordingQ(%d)", __FUNCTION__, __LINE__, m_recordingQ->getSizeOfProcessQ());
           m_clearList(m_recordingQ);
        }

        m_recordingThread->run();
    }

func_exit:

    return ret;
}

status_t ExynosCamera::m_stopRecordingInternal(void)
{
    int ret = 0;
    if (m_doCscRecording == true) {
        int recPipeId = PIPE_GSC_VIDEO;

        {
            Mutex::Autolock _l(m_recordingStopLock);
            m_previewFrameFactory->stopPipe(recPipeId);
        }

        m_recordingQ->sendCmd(WAKE_UP);

        m_recordingThread->requestExitAndWait();
        m_recordingQ->release();

        m_recordingBufferMgr->deinit();
    } else {
        CLOGI("INFO(%s[%d]):reset m_recordingBufferCount(%d->%d)",
            __FUNCTION__, __LINE__, m_recordingBufferCount, m_exynosconfig->current->bufInfo.num_recording_buffers);
        m_recordingBufferCount = m_exynosconfig->current->bufInfo.num_recording_buffers;
    }

    /* Checking all frame(buffer) released from Media recorder */
    int sleepUsecs = 33300;
    int retryCount = 3;     /* 33.3ms*3 = 100ms */
    bool allBufferReleased = true;

    for (int i = 0; i < retryCount; i++) {
        allBufferReleased = true;

        for (int bufferIndex = 0; bufferIndex < m_recordingBufferCount; bufferIndex++) {
            if (m_recordingBufAvailable[bufferIndex] == false) {
                CLOGW("WRN(%s[%d]):Media recorder doesn't release frame(buffer), index(%d)",
                        __FUNCTION__, __LINE__, bufferIndex);
                allBufferReleased = false;
            }
        }

        if(allBufferReleased == true) {
            break;
        }

        usleep(sleepUsecs);
    }

    if (allBufferReleased == false) {
        CLOGE("ERR(%s[%d]):Media recorder doesn't release frame(buffer) all!!", __FUNCTION__, __LINE__);
    }

    if (m_recordingCallbackHeap != NULL) {
        m_recordingCallbackHeap->release(m_recordingCallbackHeap);
        m_recordingCallbackHeap = NULL;
    }

    return NO_ERROR;
}

bool ExynosCamera::m_shutterCallbackThreadFunc(void)
{
    CLOGI("INFO(%s[%d]):", __FUNCTION__, __LINE__);
    int loop = false;

    if (m_parameters->msgTypeEnabled(CAMERA_MSG_SHUTTER)) {
        CLOGI("INFO(%s[%d]): CAMERA_MSG_SHUTTER callback S", __FUNCTION__, __LINE__);

        m_notifyCb(CAMERA_MSG_SHUTTER, 0, 0, m_callbackCookie);

        CLOGI("INFO(%s[%d]): CAMERA_MSG_SHUTTER callback E", __FUNCTION__, __LINE__);
    }

    /* one shot */
    return loop;
}

bool ExynosCamera::m_recordingThreadFunc(void)
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif

    int  ret  = 0;
    int  pipeId = PIPE_GSC_VIDEO;
    nsecs_t timeStamp = 0;

    ExynosCameraBuffer buffer;
    ExynosCameraFrame  *frame = NULL;

    CLOGV("INFO(%s[%d]):wait gsc done output", __FUNCTION__, __LINE__);
    ret = m_recordingQ->waitAndPopProcessQ(&frame);
    if (m_getRecordingEnabled() == false) {
        CLOGI("INFO(%s[%d]):recording stopped", __FUNCTION__, __LINE__);
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
    CLOGV("INFO(%s[%d]):gsc done for recording callback", __FUNCTION__, __LINE__);

    ret = frame->getDstBuffer(pipeId, &buffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):getDstBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        goto func_exit;
    }

    if (buffer.index < 0 || buffer.index >= (int)m_recordingBufferCount) {
        CLOGE("ERR(%s[%d]):Out of Index! (Max: %d, Index: %d)", __FUNCTION__, __LINE__, m_recordingBufferCount, buffer.index);
        goto func_exit;
    }

    timeStamp = m_recordingTimeStamp[buffer.index];

    if (m_recordingStartTimeStamp == 0) {
        m_recordingStartTimeStamp = timeStamp;
        CLOGI("INFO(%s[%d]):m_recordingStartTimeStamp=%lld",
                __FUNCTION__, __LINE__, m_recordingStartTimeStamp);
    }

    if ((0L < timeStamp)
        && (m_lastRecordingTimeStamp < timeStamp)
        && (m_recordingStartTimeStamp <= timeStamp)) {
        if (m_getRecordingEnabled() == true
            && m_parameters->msgTypeEnabled(CAMERA_MSG_VIDEO_FRAME)) {
#ifdef CHECK_MONOTONIC_TIMESTAMP
            CLOGD("DEBUG(%s[%d]):m_dataCbTimestamp::recordingFrameIndex=%d, recordingTimeStamp=%lld",
                __FUNCTION__, __LINE__, buffer.index, timeStamp);
#endif
#ifdef DEBUG
            CLOGD("DEBUG(%s[%d]): - lastTimeStamp(%lld), systemTime(%lld), recordingStart(%lld)",
                __FUNCTION__, __LINE__,
                m_lastRecordingTimeStamp,
                systemTime(SYSTEM_TIME_MONOTONIC),
                m_recordingStartTimeStamp);
#endif
            struct addrs *recordAddrs = NULL;

            recordAddrs = (struct addrs *)m_recordingCallbackHeap->data;
            recordAddrs[buffer.index].type        = kMetadataBufferTypeCameraSource;
            recordAddrs[buffer.index].fdPlaneY    = (unsigned int)buffer.fd[0];
            recordAddrs[buffer.index].fdPlaneCbcr = (unsigned int)buffer.fd[1];
            recordAddrs[buffer.index].bufIndex    = buffer.index;

            m_recordingBufAvailable[buffer.index] = false;

            m_dataCbTimestamp(
                    timeStamp,
                    CAMERA_MSG_VIDEO_FRAME,
                    m_recordingCallbackHeap,
                    buffer.index,
                    m_callbackCookie);
            m_lastRecordingTimeStamp = timeStamp;
        }
    } else {
        CLOGW("WARN(%s[%d]):recordingFrameIndex=%d, timeStamp(%lld) invalid -"
            " lastTimeStamp(%lld), systemTime(%lld), recordingStart(%lld)",
            __FUNCTION__, __LINE__, buffer.index, timeStamp,
            m_lastRecordingTimeStamp,
            systemTime(SYSTEM_TIME_MONOTONIC),
            m_recordingStartTimeStamp);
        m_releaseRecordingBuffer(buffer.index);
    }

func_exit:

    m_recordingListLock.lock();
    if (frame != NULL) {
        ret = m_removeFrameFromList(&m_recordingProcessList, frame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):remove frame from processList fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }
        frame->decRef();
        m_frameMgr->deleteFrame(frame);;
        frame = NULL;
    }
    m_recordingListLock.unlock();

    return m_recordingEnabled;
}

bool ExynosCamera::m_releasebuffersForRealloc()
{
    status_t ret = NO_ERROR;
    /* skip to free and reallocate buffers : flite / 3aa / isp / ispReprocessing */
    CLOGD("DEBUG(%s[%d]):m_setBuffers free all buffers", __FUNCTION__, __LINE__);
    if (m_bayerBufferMgr != NULL) {
        m_bayerBufferMgr->deinit();
    }
    if (m_3aaBufferMgr != NULL) {
        m_3aaBufferMgr->deinit();
    }
    if (m_ispBufferMgr != NULL) {
        m_ispBufferMgr->deinit();
    }
    if (m_hwDisBufferMgr != NULL) {
        m_hwDisBufferMgr->deinit();
    }
    if (m_vraBufferMgr != NULL) {
        m_vraBufferMgr->deinit();
    }

    /* realloc callback buffers */
    if (m_scpBufferMgr != NULL) {
        m_scpBufferMgr->deinit();
        m_scpBufferMgr->setBufferCount(0);
    }

    if (m_sccBufferMgr != NULL) {
        m_sccBufferMgr->deinit();
    }

    if (m_previewCallbackBufferMgr != NULL) {
        m_previewCallbackBufferMgr->deinit();
    }
    if (m_highResolutionCallbackBufferMgr != NULL) {
        m_highResolutionCallbackBufferMgr->deinit();
    }

    m_parameters->setReallocBuffer(false);

    if (m_parameters->getRestartPreview() == true) {
        ret = setPreviewWindow(m_previewWindow);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):setPreviewWindow fail", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }
    }

   return true;
}


status_t ExynosCamera::m_setBuffers(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    CLOGI("INFO(%s[%d]):alloc buffer - camera ID: %d",
        __FUNCTION__, __LINE__, m_cameraId);
    int ret = 0;
    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    unsigned int planeSize[EXYNOS_CAMERA_BUFFER_MAX_PLANES]    = {0};
    int hwPreviewW, hwPreviewH;
    int hwPictureW, hwPictureH;

    int ispBufferW, ispBufferH;
    int previewMaxW, previewMaxH;
    int pictureMaxW, pictureMaxH;
    int sensorMaxW, sensorMaxH;
    int sensorMarginW, sensorMarginH;
    ExynosRect bdsRect;

    int planeCount  = 1;
    int minBufferCount = 1;
    int maxBufferCount = 1;
    exynos_camera_buffer_type_t type = EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE;
    buffer_manager_allocation_mode_t allocMode = BUFFER_MANAGER_ALLOCATION_ATONCE;

    if( m_parameters->getReallocBuffer() ) {
        /* skip to free and reallocate buffers : flite / 3aa / isp / ispReprocessing */
        m_releasebuffersForRealloc();
    }

    m_parameters->getHwPreviewSize(&hwPreviewW, &hwPreviewH);
    CLOGI("(%s):HW Preview width x height = %dx%d", __FUNCTION__, hwPreviewW, hwPreviewH);
    m_parameters->getHwPictureSize(&hwPictureW, &hwPictureH);
    CLOGI("(%s):HW Picture width x height = %dx%d", __FUNCTION__, hwPictureW, hwPictureH);
    m_parameters->getMaxPictureSize(&pictureMaxW, &pictureMaxH);
    CLOGI("(%s):Picture MAX width x height = %dx%d", __FUNCTION__, pictureMaxW, pictureMaxH);
    if( m_parameters->getHighSpeedRecording() ) {
        m_parameters->getHwSensorSize(&sensorMaxW, &sensorMaxH);
        CLOGI("(%s):HW Sensor(HighSpeed) MAX width x height = %dx%d", __FUNCTION__, sensorMaxW, sensorMaxH);
        m_parameters->getHwPreviewSize(&previewMaxW, &previewMaxH);
        CLOGI("(%s):HW Preview(HighSpeed) MAX width x height = %dx%d", __FUNCTION__, previewMaxW, previewMaxH);
    } else {
        m_parameters->getMaxSensorSize(&sensorMaxW, &sensorMaxH);
        CLOGI("(%s):Sensor MAX width x height = %dx%d", __FUNCTION__, sensorMaxW, sensorMaxH);
        m_parameters->getMaxPreviewSize(&previewMaxW, &previewMaxH);
        CLOGI("(%s):Preview MAX width x height = %dx%d", __FUNCTION__, previewMaxW, previewMaxH);
    }

#if (SUPPORT_BACK_HW_VDIS || SUPPORT_FRONT_HW_VDIS)
    /*
     * we cannot expect TPU on or not, when open() api.
     * so extract memory TPU size
     */
    int w = 0, h = 0;
    m_parameters->calcNormalToTpuSize(previewMaxW, previewMaxH, &w, &h);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):Hw vdis buffer calulation fail src(%d x %d) dst(%d x %d)",__FUNCTION__, __LINE__, previewMaxW, previewMaxH, w, h);
    }
    previewMaxW = w;
    previewMaxH = h;
    CLOGI("(%s): TPU based Preview MAX width x height = %dx%d", __FUNCTION__, previewMaxW, previewMaxH);
#endif

    m_parameters->getPreviewBdsSize(&bdsRect);

    /* FLITE */
#ifdef CAMERA_PACKED_BAYER_ENABLE
#ifdef DEBUG_RAWDUMP
    if (m_parameters->checkBayerDumpEnable()) {
        bytesPerLine[0] = sensorMaxW * 2;
        planeSize[0] = sensorMaxW * sensorMaxH * 2;
    } else
#endif /* DEBUG_RAWDUMP */
    {
        bytesPerLine[0] = ROUND_UP(sensorMaxW , 10) * 8 / 5;
        planeSize[0]    = bytesPerLine[0] * sensorMaxH;
    }
#else
    planeSize[0] = sensorMaxW * sensorMaxH * 2;
#endif
    planeCount  = 2;

    /* TO DO : make num of buffers samely */
    maxBufferCount = m_exynosconfig->current->bufInfo.num_bayer_buffers;
#ifdef RESERVED_MEMORY_ENABLE
    if (getCameraId() == CAMERA_ID_BACK) {
        type = EXYNOS_CAMERA_BUFFER_ION_RESERVED_TYPE;
        m_bayerBufferMgr->setContigBufCount(RESERVED_NUM_BAYER_BUFFERS);
    } else {
        if (m_parameters->getDualMode() == false) {
            type = EXYNOS_CAMERA_BUFFER_ION_RESERVED_TYPE;
            m_bayerBufferMgr->setContigBufCount(FRONT_RESERVED_NUM_BAYER_BUFFERS);
        }
    }
#endif

#ifndef DEBUG_RAWDUMP
    if (m_parameters->isUsing3acForIspc() == false
        || m_parameters->getDualMode() == true)
#endif
    {
        ret = m_allocBuffers(m_bayerBufferMgr, planeCount, planeSize, bytesPerLine, maxBufferCount, maxBufferCount, type, true, false);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):bayerBuffer m_allocBuffers(bufferCount=%d) fail",
                    __FUNCTION__, __LINE__, maxBufferCount);
            return ret;
        }
    }

    type = EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE;

#ifdef CAMERA_PACKED_BAYER_ENABLE
    memset(&bytesPerLine, 0, sizeof(unsigned int) * EXYNOS_CAMERA_BUFFER_MAX_PLANES);
#endif

    /* for preview */
    planeSize[0] = 32 * 64 * 2;
    planeCount  = 2;
    /* TO DO : make num of buffers samely */
    if (m_parameters->isFlite3aaOtf() == true)
        maxBufferCount = m_exynosconfig->current->bufInfo.num_3aa_buffers;
    else
        maxBufferCount = m_exynosconfig->current->bufInfo.num_bayer_buffers;

    ret = m_allocBuffers(m_3aaBufferMgr, planeCount, planeSize, bytesPerLine, maxBufferCount, true, false);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_3aaBufferMgr m_allocBuffers(bufferCount=%d) fail",
            __FUNCTION__, __LINE__, maxBufferCount);
        return ret;
    }

    if (m_parameters->isUsing3acForIspc() == true) {
    if (m_parameters->isReprocessing() == true) {
#ifdef CAMERA_PACKED_BAYER_ENABLE
#ifdef DEBUG_RAWDUMP
        if (m_parameters->checkBayerDumpEnable()) {
            bytesPerLine[0] = previewMaxW * 2;
            planeSize[0] = previewMaxW * previewMaxH * 2;
        } else
#endif /* DEBUG_RAWDUMP */
        {
            if (m_parameters->getDualMode() == true && getCameraId() == CAMERA_ID_FRONT) {
                planeSize[0] = previewMaxW * previewMaxH * 2;
            } else {
                bytesPerLine[0] = ROUND_UP((previewMaxW * 3 / 2), 16);
                planeSize[0]    = bytesPerLine[0] * previewMaxH;
            }
        }
#else
        /* planeSize[0] = width * height * 2; */
        planeSize[0] = previewMaxW * previewMaxH * 2;
#endif
    } else {
        /* Picture Max Size == Sensor Max Size - Sensor Margin */
#ifdef CAMERA_PACKED_BAYER_ENABLE
#ifdef DEBUG_RAWDUMP
        if (m_parameters->checkBayerDumpEnable()) {
            bytesPerLine[0] = pictureMaxW * 2;
            planeSize[0] = pictureMaxW * pictureMaxH * 2;
        } else
#endif /* DEBUG_RAWDUMP */
        {
            bytesPerLine[0] = ROUND_UP((pictureMaxW * 3 / 2), 16);
            planeSize[0]    = bytesPerLine[0] * pictureMaxH;
        }
#else
        /* planeSize[0] = width * height * 2; */
        planeSize[0] = pictureMaxW * pictureMaxH * 2;
#endif
        }
    } else {
#if defined (USE_ISP_BUFFER_SIZE_TO_BDS)
        ispBufferW = bdsRect.w;
        ispBufferH = bdsRect.h;
#else
        ispBufferW = previewMaxW;
        ispBufferH = previewMaxH;
#endif

#ifdef CAMERA_PACKED_BAYER_ENABLE
        bytesPerLine[0] = ROUND_UP((ispBufferW* 3 / 2), 16);
        planeSize[0]    = bytesPerLine[0] * ispBufferH;
#else
        bytesPerLine[0] = ispBufferW * 2;
        planeSize[0] = ispBufferW * ispBufferH * 2;
#endif
    }
    planeCount  = 2;
    /* TO DO : make num of buffers samely */
    if (m_parameters->isFlite3aaOtf() == true) {
        maxBufferCount = m_exynosconfig->current->bufInfo.num_3aa_buffers;
#ifdef RESERVED_MEMORY_ENABLE
        if (getCameraId() == CAMERA_ID_BACK) {
            type = EXYNOS_CAMERA_BUFFER_ION_RESERVED_TYPE;
            if(m_parameters->getUHDRecordingMode() == true) {
                m_ispBufferMgr->setContigBufCount(RESERVED_NUM_ISP_BUFFERS_ON_UHD);
            } else {
                m_ispBufferMgr->setContigBufCount(RESERVED_NUM_ISP_BUFFERS);
            }
        } else {
            if (m_parameters->getDualMode() == false) {
                type = EXYNOS_CAMERA_BUFFER_ION_RESERVED_TYPE;
                m_ispBufferMgr->setContigBufCount(FRONT_RESERVED_NUM_ISP_BUFFERS);
            }
        }
#endif
    } else {
        maxBufferCount = m_exynosconfig->current->bufInfo.num_bayer_buffers;
#ifdef RESERVED_MEMORY_ENABLE
        if (m_parameters->getDualMode() == false) {
            type = EXYNOS_CAMERA_BUFFER_ION_RESERVED_TYPE;
            m_ispBufferMgr->setContigBufCount(FRONT_RESERVED_NUM_ISP_BUFFERS);
        }
#endif
    }

    if (m_parameters->is3aaIspOtf() == false) {
        ret = m_allocBuffers(m_ispBufferMgr, planeCount, planeSize, bytesPerLine, maxBufferCount, maxBufferCount, type, true, false);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_ispBufferMgr m_allocBuffers(bufferCount=%d) fail",
                    __FUNCTION__, __LINE__, maxBufferCount);
            return ret;
        }
    }

    /* HW VDIS memory */
    if (m_parameters->getTpuEnabledMode() == true) {
        maxBufferCount = m_exynosconfig->current->bufInfo.num_hwdis_buffers;

        /* DIS MEMORY */
        int disFormat = m_parameters->getHWVdisFormat();
        unsigned int bpp = 0;
        unsigned int disPlanes = 0;

        getYuvFormatInfo(disFormat, &bpp, &disPlanes);

        switch (disFormat) {
        case V4L2_PIX_FMT_YUYV:
            planeSize[0] = bdsRect.w * bdsRect.h * 2;
            break;
        default:
            CLOGE("ERR(%s[%d]):unexpected VDIS format(%d). so, fail", __FUNCTION__, __LINE__, disFormat);
            return INVALID_OPERATION;
            break;
        }

        exynos_camera_buffer_type_t type = EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE;

        ret = m_allocBuffers(m_hwDisBufferMgr, planeCount, planeSize, bytesPerLine, maxBufferCount, maxBufferCount, type, true, false);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_hwDisBufferMgr m_allocBuffers(bufferCount=%d) fail",
                __FUNCTION__, __LINE__, maxBufferCount);
            return ret;
        }

        CLOGD("DEBUG(%s[%d]):m_allocBuffers(m_hwDisBufferMgr): %d x %d, planeCount(%d), maxBufferCount(%d)",
            __FUNCTION__, __LINE__, bdsRect.w, bdsRect.h, planeCount, maxBufferCount);
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
            CLOGE("ERR(%s[%d]):m_vraBufferMgr m_allocBuffers(bufferCount=%d) fail",
                    __FUNCTION__, __LINE__, maxBufferCount);
            return ret;
        }
    }

    /* SW VDIS memory */
    type = EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE;

    planeSize[0] = hwPreviewW * hwPreviewH;
    planeSize[1] = hwPreviewW * hwPreviewH / 2;
    planeCount  = 3;
    if(m_parameters->increaseMaxBufferOfPreview()){
        maxBufferCount = m_parameters->getPreviewBufferCount();
    } else {
        maxBufferCount = m_exynosconfig->current->bufInfo.num_preview_buffers;
    }

    bool needMmap = false;
    if (m_previewWindow == NULL)
        needMmap = true;

    ret = m_allocBuffers(m_scpBufferMgr, planeCount, planeSize, bytesPerLine, maxBufferCount, true, needMmap);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_scpBufferMgr m_allocBuffers(bufferCount=%d) fail",
            __FUNCTION__, __LINE__, maxBufferCount);
        return ret;
    }

    int stride = m_scpBufferMgr->getBufStride();
    if (stride != hwPreviewW) {
        CLOGI("INFO(%s[%d]):hwPreviewW(%d), stride(%d)", __FUNCTION__, __LINE__, hwPreviewW, stride);
        if (stride == 0) {
            /* If the SCP buffer manager is not instance of GrallocExynosCameraBufferManager
               (In case of setPreviewWindow(null) is called), return value of setHwPreviewStride()
               will be zero. If this value is passed as SCP width to firmware, firmware will
               generate PABORT error. */
            CLOGW("WARN(%s[%d]):HACK: Invalid stride(%d). It will be replaced as hwPreviewW(%d) value.",
                __FUNCTION__, __LINE__, stride, hwPreviewW);
            stride = hwPreviewW;
        }
    }

    m_parameters->setHwPreviewStride(stride);

    if (m_parameters->isSccCapture() == true
        || m_parameters->isUsing3acForIspc() == true) {
        m_parameters->getHwPictureSize(&hwPictureW, &hwPictureH);
        if (m_parameters->isUsing3acForIspc() == true){
            hwPictureW = sensorMaxW;
            hwPictureH = sensorMaxW;
        }
        CLOGI("(%s):HW Picture width x height = %dx%d", __FUNCTION__, hwPictureW, hwPictureH);
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
        /* TO DO : make same num of buffers */
        if (m_parameters->isFlite3aaOtf() == true)
            maxBufferCount = m_exynosconfig->current->bufInfo.num_picture_buffers;
        else
            maxBufferCount = m_exynosconfig->current->bufInfo.num_bayer_buffers;

        if (m_parameters->isUsing3acForIspc() == true) {
            allocMode = BUFFER_MANAGER_ALLOCATION_SILENT;
            minBufferCount = 1;
        } else {
            allocMode = BUFFER_MANAGER_ALLOCATION_ATONCE;
            minBufferCount = maxBufferCount;
        }
        type = EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE;

        ret = m_allocBuffers(m_sccBufferMgr, planeCount, planeSize, bytesPerLine, minBufferCount, maxBufferCount, type, allocMode, true, false);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_sccBufferMgr m_allocBuffers(bufferCount=%d) fail",
                __FUNCTION__, __LINE__, maxBufferCount);
            return ret;
        }
    }

    CLOGI("INFO(%s[%d]):alloc buffer done - camera ID: %d",
        __FUNCTION__, __LINE__, m_cameraId);

    return NO_ERROR;
}

status_t ExynosCamera::m_setReprocessingBuffer(void)
{
    int ret = 0;
    int pictureMaxW, pictureMaxH;
    unsigned int planeSize[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
    int planeCount  = 0;
    int bufferCount = 0;
    int minBufferCount = NUM_REPROCESSING_BUFFERS;
    int maxBufferCount = NUM_PICTURE_BUFFERS;
    exynos_camera_buffer_type_t type = EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE;
    buffer_manager_allocation_mode_t allocMode = BUFFER_MANAGER_ALLOCATION_ONDEMAND;

    m_parameters->getMaxPictureSize(&pictureMaxW, &pictureMaxH);
    CLOGI("(%s):HW Picture MAX width x height = %dx%d", __FUNCTION__, pictureMaxW, pictureMaxH);

    /* for reprocessing */
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

    if (m_parameters->getHighResolutionCallbackMode() == true) {
        /* ISP Reprocessing Buffer realloc for high resolution callback */
        minBufferCount = 2;
    }

    ret = m_allocBuffers(m_ispReprocessingBufferMgr, planeCount, planeSize, bytesPerLine, minBufferCount, maxBufferCount, type, allocMode, true, false);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_ispReprocessingBufferMgr m_allocBuffers(minBufferCount=%d/maxBufferCount=%d) fail",
            __FUNCTION__, __LINE__, minBufferCount, maxBufferCount);
        return ret;
    }

    return NO_ERROR;
}

status_t ExynosCamera::m_setPictureBuffer(void)
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
    pictureFormat = m_parameters->getHwPictureFormat();
    if ((m_parameters->needGSCForCapture(getCameraId()) == true)) {
        if (JPEG_INPUT_COLOR_FMT == V4L2_PIX_FMT_NV21M) {
            planeSize[0] = pictureW * pictureH;
            planeSize[1] = pictureW * pictureH / 2;
            planeCount = 2;
        } else if (JPEG_INPUT_COLOR_FMT == V4L2_PIX_FMT_NV21) {
            planeSize[0] = pictureW * pictureH * 3 / 2;
            planeCount = 1;
        }else {
            planeSize[0] = pictureW * pictureH * 2;
            planeCount = 1;
        }

        minBufferCount = 1;
        maxBufferCount = m_exynosconfig->current->bufInfo.num_picture_buffers;

        // Pre-allocate certain amount of buffers enough to fed into 3 JPEG save threads.
        if (m_parameters->getSeriesShotCount() > 0)
            minBufferCount = NUM_BURST_GSC_JPEG_INIT_BUFFER;

        ret = m_allocBuffers(m_gscBufferMgr, planeCount, planeSize, bytesPerLine, minBufferCount, maxBufferCount, type, allocMode, false, false);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_gscBufferMgr m_allocBuffers(minBufferCount=%d, maxBufferCount=%d) fail",
                __FUNCTION__, __LINE__, minBufferCount, maxBufferCount);
            return ret;
        }
    }

    if( m_hdrEnabled == false ) {
        if (JPEG_INPUT_COLOR_FMT == V4L2_PIX_FMT_NV21M) {
            planeSize[0] = pictureW * pictureH * 3 / 2;
            planeCount = 2;
        } else if (JPEG_INPUT_COLOR_FMT == V4L2_PIX_FMT_NV21) {
            planeSize[0] = pictureW * pictureH * 3 / 2;
            planeCount = 1;
        } else {
            planeSize[0] = FRAME_SIZE(V4L2_PIX_2_HAL_PIXEL_FORMAT(pictureFormat), pictureW, pictureH);
            planeCount = 1;
        }
        minBufferCount = 1;
        maxBufferCount = m_exynosconfig->current->bufInfo.num_picture_buffers;

        type = EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE;
        CLOGD("DEBUG(%s[%d]): jpegBuffer picture(%dx%d) size(%d)", __FUNCTION__, __LINE__, pictureW, pictureH, planeSize[0]);

        // Same with above GSC buffers
        if (m_parameters->getSeriesShotCount() > 0)
            minBufferCount = NUM_BURST_GSC_JPEG_INIT_BUFFER;

#ifdef RESERVED_MEMORY_ENABLE
        if (getCameraId() == CAMERA_ID_BACK) {
            type = EXYNOS_CAMERA_BUFFER_ION_CACHED_RESERVED_TYPE;
            if (m_parameters->getUHDRecordingMode() == true) {
                m_jpegBufferMgr->setContigBufCount(RESERVED_NUM_JPEG_BUFFERS_ON_UHD);
            } else {
                m_jpegBufferMgr->setContigBufCount(RESERVED_NUM_JPEG_BUFFERS);

                /* alloc at once */
                minBufferCount = NUM_BURST_GSC_JPEG_INIT_BUFFER;
            }
        }
#endif

        ret = m_allocBuffers(m_jpegBufferMgr, planeCount, planeSize, bytesPerLine, minBufferCount, maxBufferCount, type, allocMode, false, true);
        if (ret < 0)
            CLOGE("ERR(%s:%d):jpegSrcHeapBuffer m_allocBuffers(bufferCount=%d) fail",
                    __FUNCTION__, __LINE__, NUM_REPROCESSING_BUFFERS);
    }

    return ret;
}

status_t ExynosCamera::m_releaseBuffers(void)
{
    CLOGI("INFO(%s[%d]):release buffer", __FUNCTION__, __LINE__);
    int ret = 0;

    if (m_bayerBufferMgr != NULL) {
        m_bayerBufferMgr->deinit();
    }
    if (m_3aaBufferMgr != NULL) {
        m_3aaBufferMgr->deinit();
    }
    if (m_ispBufferMgr != NULL) {
        m_ispBufferMgr->deinit();
    }
    if (m_hwDisBufferMgr != NULL) {
        m_hwDisBufferMgr->deinit();
    }
    if (m_scpBufferMgr != NULL) {
        m_scpBufferMgr->deinit();
    }
    if (m_vraBufferMgr != NULL) {
        m_vraBufferMgr->deinit();
    }
    if (m_ispReprocessingBufferMgr != NULL) {
        m_ispReprocessingBufferMgr->deinit();
    }
    if (m_sccReprocessingBufferMgr != NULL) {
        m_sccReprocessingBufferMgr->deinit();
    }
    if (m_sccBufferMgr != NULL) {
        m_sccBufferMgr->deinit();
    }
    if (m_gscBufferMgr != NULL) {
        m_gscBufferMgr->deinit();
    }
    if (m_jpegBufferMgr != NULL) {
        m_jpegBufferMgr->deinit();
    }
    if (m_thumbnailBufferMgr != NULL) {
        m_thumbnailBufferMgr->deinit();
    }
    if (m_recordingBufferMgr != NULL) {
        m_recordingBufferMgr->deinit();
    }
    if (m_previewCallbackBufferMgr != NULL) {
        m_previewCallbackBufferMgr->deinit();
    }
    if (m_highResolutionCallbackBufferMgr != NULL) {
        m_highResolutionCallbackBufferMgr->deinit();
    }

    CLOGI("INFO(%s[%d]):free buffer done", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

bool ExynosCamera::m_monitorThreadFunc(void)
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
    int pipeIdErrorCheck = 0;

    for (loopCount = 0; loopCount < MONITOR_THREAD_INTERVAL; loopCount += (MONITOR_THREAD_INTERVAL/20)) {
        if (m_flagThreadStop == true) {
            CLOGI("INFO(%s[%d]): m_flagThreadStop(%d)", __FUNCTION__, __LINE__, m_flagThreadStop);

            return false;
        }

        usleep(MONITOR_THREAD_INTERVAL/20);
    }

    if (m_previewFrameFactory == NULL) {
        CLOGW("WARN(%s[%d]): m_previewFrameFactory is NULL. Skip monitoring.", __FUNCTION__, __LINE__);

        return false;
    }

    pipeIdFlite = PIPE_FLITE;

    if (m_parameters->is3aaIspOtf() == true) {
        if (m_parameters->getTpuEnabledMode() == true)
            pipeIdErrorCheck = PIPE_DIS;
        else
            pipeIdErrorCheck = PIPE_3AA;
    } else {
        pipeIdErrorCheck = PIPE_ISP;
    }

#ifdef MONITOR_LOG_SYNC
    uint32_t pipeIdIsp = 0;

    /* define pipe for isp node cause of sync log sctrl */
    if (m_parameters->isFlite3aaOtf() == true)
        pipeIdIsp = PIPE_3AA;

    /* If it is not front camera in dual and sensor pipe is running, do sync log */
    if (m_previewFrameFactory->checkPipeThreadRunning(pipeIdIsp) &&
        !(getCameraId() == CAMERA_ID_FRONT && m_parameters->getDualMode())){
        if (!(m_syncLogDuration % MONITOR_LOG_SYNC_INTERVAL)) {
            uint32_t syncLogId = m_getSyncLogId();
            CLOGI("INFO(%s[%d]): @FIMC_IS_SYNC %d", __FUNCTION__, __LINE__, syncLogId);
            m_previewFrameFactory->syncLog(pipeIdIsp, syncLogId);
        }
        m_syncLogDuration++;
    }
#endif

    m_previewFrameFactory->getControl(V4L2_CID_IS_G_DTPSTATUS, &dtpStatus, pipeIdFlite);
    if (dtpStatus == 1) {
        CLOGD("DEBUG(%s[%d]):DTP Detected. dtpStatus(%d)", __FUNCTION__, __LINE__, dtpStatus);
        dump();

        /* in GED */
        m_notifyCb(CAMERA_MSG_ERROR, 100, 0, m_callbackCookie);
        return false;
    }

#ifdef SENSOR_OVERFLOW_CHECK
    m_previewFrameFactory->getControl(V4L2_CID_IS_G_DTPSTATUS, &dtpStatus, pipeIdFlite);
    if (dtpStatus == 1) {
        CLOGD("DEBUG(%s[%d]):DTP Detected. dtpStatus(%d)", __FUNCTION__, __LINE__, dtpStatus);
        dump();

        /* in GED */
        /* m_notifyCb(CAMERA_MSG_ERROR, 100, 0, m_callbackCookie); */
        /* specifically defined */
        /* m_notifyCb(CAMERA_MSG_ERROR, 1002, 0, m_callbackCookie); */
        /* or */
        android_printAssert(NULL, LOG_TAG, "killed by itself");

        return false;
    }
#endif

    m_previewFrameFactory->getThreadState(&threadState, pipeIdErrorCheck);
    m_previewFrameFactory->getThreadRenew(&countRenew, pipeIdErrorCheck);

    if ((*threadState == ERROR_POLLING_DETECTED) || (*countRenew > ERROR_DQ_BLOCKED_COUNT)) {
        CLOGD("DEBUG(%s[%d]):ESD Detected. threadState(%d) *countRenew(%d)", __FUNCTION__, __LINE__, *threadState, *countRenew);
        dump();

        /* in GED */
        /* skip error callback */
        /* m_notifyCb(CAMERA_MSG_ERROR, 100, 0, m_callbackCookie); */

        return false;
    } else {
        CLOGV("[%s] (%d) (%d)", __FUNCTION__, __LINE__, *threadState);
    }

#if 0
    m_checkThreadState(threadState, countRenew)?:ret = false;
    m_checkThreadInterval(PIPE_SCP, WARNING_SCP_THREAD_INTERVAL, threadState)?:ret = false;

    enum pipeline pipe;

    /* check PIPE_3AA thread state & interval */
    if (m_parameters->isFlite3aaOtf() == true) {
        pipe = PIPE_3AA_ISP;

        m_previewFrameFactory->getThreadRenew(&countRenew, pipe);
        m_checkThreadState(threadState, countRenew)?:ret = false;

        if (ret == false) {
            dump();

            /* in GED */
            m_notifyCb(CAMERA_MSG_ERROR, 100, 0, m_callbackCookie);
            /* specifically defined */
            /* m_notifyCb(CAMERA_MSG_ERROR, 1001, 0, m_callbackCookie); */
            /* or */
            android_printAssert(NULL, LOG_TAG, "killed by itself");
        }
    } else {
        pipe = PIPE_3AA;

        m_previewFrameFactory->getThreadRenew(&countRenew, pipe);
        m_checkThreadState(threadState, countRenew)?:ret = false;

        if (ret == false) {
            dump();

            /* in GED */
            m_notifyCb(CAMERA_MSG_ERROR, 100, 0, m_callbackCookie);
            /* specifically defined */
            /* m_notifyCb(CAMERA_MSG_ERROR, 1001, 0, m_callbackCookie); */
            /* or */
            android_printAssert(NULL, LOG_TAG, "killed by itself");
        }
    }

    m_checkThreadInterval(pipe, WARNING_3AA_THREAD_INTERVAL, threadState)?:ret = false;

    if (m_callbackState == 0) {
        m_callbackStateOld = 0;
        m_callbackState = 0;
        m_callbackMonitorCount = 0;
    } else {
        if (m_callbackStateOld != m_callbackState) {
            m_callbackStateOld = m_callbackState;
            CLOGD("INFO(%s[%d]):callback state is updated (0x%x)", __FUNCTION__, __LINE__, m_callbackStateOld);
        } else {
            if ((m_callbackStateOld & m_callbackState) != 0)
                CLOGE("ERR(%s[%d]):callback is blocked (0x%x), Duration:%d msec", __FUNCTION__, __LINE__, m_callbackState, m_callbackMonitorCount*(MONITOR_THREAD_INTERVAL/1000));
        }
    }
#endif

    gettimeofday(&dqTime, NULL);
    m_previewFrameFactory->getThreadInterval(&timeInterval, pipeIdErrorCheck);

    CLOGV("Thread IntervalTime [%lld]", *timeInterval);
    CLOGV("Thread Renew Count [%d]", *countRenew);

    m_previewFrameFactory->incThreadRenew(pipeIdErrorCheck);

    return true;
}

bool ExynosCamera::m_autoFocusResetNotify(int focusMode)
{
    /* show restart */
    CLOGD("DEBUG(%s):CAMERA_MSG_FOCUS(%d) mode(%d)", __func__, 4, focusMode);
    m_notifyCb(CAMERA_MSG_FOCUS, 4, 0, m_callbackCookie);

    /* show focusing */
    CLOGD("DEBUG(%s):CAMERA_MSG_FOCUS(%d) mode(%d)", __func__, 3, focusMode);
    m_notifyCb(CAMERA_MSG_FOCUS, 3, 0, m_callbackCookie);

    return true;
}

bool ExynosCamera::m_autoFocusThreadFunc(void)
{
    CLOGI("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    bool afResult = false;
    int focusMode = 0;

    /* block until we're told to start.  we don't want to use
     * a restartable thread and requestExitAndWait() in cancelAutoFocus()
     * because it would cause deadlock between our callbacks and the
     * caller of cancelAutoFocus() which both want to grab the same lock
     * in CameraServices layer.
     */

    if (getCameraId() == CAMERA_ID_FRONT) {
        if (m_parameters->msgTypeEnabled(CAMERA_MSG_FOCUS)) {
            if (m_notifyCb != NULL) {
                CLOGD("DEBUG(%s):Do not support autoFocus in front camera.", __FUNCTION__);
                m_notifyCb(CAMERA_MSG_FOCUS, true, 0, m_callbackCookie);
            } else {
                CLOGD("DEBUG(%s):m_notifyCb is NULL!", __FUNCTION__);
            }
        } else {
            CLOGD("DEBUG(%s):autoFocus msg disabled !!", __FUNCTION__);
        }
        return false;
    }

    if (m_autoFocusType == AUTO_FOCUS_SERVICE) {
        focusMode = m_parameters->getFocusMode();
    } else if (m_autoFocusType == AUTO_FOCUS_HAL) {
        focusMode = FOCUS_MODE_AUTO;

        m_autoFocusResetNotify(focusMode);
    }

    /* check early exit request */
    if (m_exitAutoFocusThread == true) {
        CLOGD("DEBUG(%s):exiting on request", __FUNCTION__);
        goto done;
    }

    m_autoFocusLock.lock();
    m_autoFocusRunning = true;

    if (m_autoFocusRunning == true) {
        afResult = m_exynosCameraActivityControl->autoFocus(focusMode, m_autoFocusType);
        if (afResult == true)
            CLOGV("DEBUG(%s):autoFocus Success!!", __FUNCTION__);
        else
            CLOGV("DEBUG(%s):autoFocus Fail !!", __FUNCTION__);
    } else {
        CLOGV("DEBUG(%s):autoFocus canceled !!", __FUNCTION__);
    }

    /*
     * CAMERA_MSG_FOCUS only takes a bool.  true for
     * finished and false for failure.
     * If cancelAutofocus() called, no callback.
     */
    if ((m_autoFocusRunning == true) &&
        m_parameters->msgTypeEnabled(CAMERA_MSG_FOCUS)) {

        if (m_notifyCb != NULL) {
            int afFinalResult = (int)afResult;

            /* if inactive detected, tell it */
            if (focusMode == FOCUS_MODE_CONTINUOUS_PICTURE) {
                if (m_exynosCameraActivityControl->getCAFResult() == 2) {
                    afFinalResult = 2;
                }
            }

            CLOGD("DEBUG(%s):CAMERA_MSG_FOCUS(%d) mode(%d)", __FUNCTION__, afFinalResult, focusMode);
            m_notifyCb(CAMERA_MSG_FOCUS, afFinalResult, 0, m_callbackCookie);
        }  else {
            CLOGD("DEBUG(%s):m_notifyCb is NULL mode(%d)", __FUNCTION__, focusMode);
        }
    } else {
        CLOGV("DEBUG(%s):autoFocus canceled, no callback !!", __FUNCTION__);
    }

    m_autoFocusRunning = false;

    CLOGV("DEBUG(%s):exiting with no error", __FUNCTION__);

done:
    m_autoFocusLock.unlock();

    CLOGI("DEBUG(%s):end", __FUNCTION__);

    return false;
}

bool ExynosCamera::m_autoFocusContinousThreadFunc(void)
{
    int ret = 0;
    int index = 0;
    uint32_t frameCnt = 0;
    uint32_t count = 0;


    ret = m_autoFocusContinousQ.waitAndPopProcessQ(&frameCnt);
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

    count = m_autoFocusContinousQ.getSizeOfProcessQ();
    if( count >= MAX_FOCUSCONTINUS_THREADQ_SIZE ) {
        for( uint32_t i = 0 ; i < count ; i++) {
            m_autoFocusContinousQ.popProcessQ(&frameCnt);
        }
        CLOGD("DEBUG(%s[%d]):m_autoFocusContinousQ  skipped QSize(%d) frame(%d)", __FUNCTION__, __LINE__,  count, frameCnt);
    }

    /* Continuous Auto-focus */
    if (m_parameters->getFocusMode() == FOCUS_MODE_CONTINUOUS_PICTURE) {
        int afstatus = 0;
        static int afResult = 1;
        int prev_afstatus = afResult;
        afstatus = m_exynosCameraActivityControl->getCAFResult();
        afResult = afstatus;

        if (afstatus == 3 && (prev_afstatus == 0 || prev_afstatus == 1)) {
            afResult = 4;
        }

        if (m_parameters->msgTypeEnabled(CAMERA_MSG_FOCUS)
            && (prev_afstatus != afstatus)) {
            CLOGD("DEBUG(%s):CAMERA_MSG_FOCUS(%d) mode(%d)",
                __FUNCTION__, afResult, m_parameters->getFocusMode());
            m_notifyCb(CAMERA_MSG_FOCUS, afResult, 0, m_callbackCookie);
        }
    }

    return true;
}

status_t ExynosCamera::m_getBufferManager(uint32_t pipeId, ExynosCameraBufferManager **bufMgr, uint32_t direction)
{
    status_t ret = NO_ERROR;
    ExynosCameraBufferManager **bufMgrList[2] = {NULL};
    *bufMgr = NULL;
    int internalPipeId  = pipeId;

    /*
     * front / back is different up to scenario(3AA OTF/M2M, etc)
     * so, we don't need to distinguish front / back camera.
     * but. reprocessing must handle the separated operation
     */
    if (pipeId < PIPE_FLITE_REPROCESSING)
        internalPipeId = INDEX(pipeId);

    switch (internalPipeId) {
    case PIPE_FLITE:
        bufMgrList[0] = NULL;
        bufMgrList[1] = &m_bayerBufferMgr;
        break;
    case PIPE_3AA_ISP:
        bufMgrList[0] = &m_3aaBufferMgr;
        bufMgrList[1] = &m_ispBufferMgr;
        break;
    case PIPE_3AC:
        bufMgrList[0] = NULL;
        bufMgrList[1] = &m_bayerBufferMgr;
        break;
    case PIPE_3AA:
        if (m_parameters->getDualMode() == true && getCameraId() == CAMERA_ID_FRONT) {
            bufMgrList[0] = &m_bayerBufferMgr;
            bufMgrList[1] = &m_sccBufferMgr;
        } else {
            bufMgrList[0] = &m_3aaBufferMgr;
            if (m_parameters->isUsing3acForIspc() == true)
                bufMgrList[1] = &m_sccBufferMgr;
            else
                bufMgrList[1] = &m_ispBufferMgr;
        }
        break;
    case PIPE_ISP:
        bufMgrList[0] = &m_ispBufferMgr;

        if (m_parameters->getTpuEnabledMode() == true)
            bufMgrList[1] = &m_hwDisBufferMgr;
        else
            bufMgrList[1] = &m_scpBufferMgr;
        break;
    case PIPE_DIS:
        bufMgrList[0] = &m_ispBufferMgr;
        bufMgrList[1] = &m_scpBufferMgr;
        break;
    case PIPE_ISPC:
    case PIPE_SCC:
        bufMgrList[0] = NULL;
        bufMgrList[1] = &m_sccBufferMgr;
        break;
    case PIPE_SCP:
        bufMgrList[0] = NULL;
        bufMgrList[1] = &m_scpBufferMgr;
        break;
    case PIPE_VRA:
        bufMgrList[0] = &m_vraBufferMgr;
        bufMgrList[1] = NULL;
        break;
    case PIPE_GSC:
        if (m_parameters->getDualMode() == true && getCameraId() == CAMERA_ID_FRONT)
            bufMgrList[0] = &m_sccBufferMgr;
        else
            bufMgrList[0] = &m_scpBufferMgr;
       bufMgrList[1] = &m_scpBufferMgr;
       break;
    case PIPE_GSC_PICTURE:
        bufMgrList[0] = &m_sccBufferMgr;
        bufMgrList[1] = &m_gscBufferMgr;
        break;
    case PIPE_3AA_REPROCESSING:
        bufMgrList[0] = &m_bayerBufferMgr;
        if (m_parameters->getDualMode() == false)
            bufMgrList[1] = &m_ispReprocessingBufferMgr;
        else
            bufMgrList[1] = &m_sccReprocessingBufferMgr;
        break;
    case PIPE_ISP_REPROCESSING:
        bufMgrList[0] = &m_ispReprocessingBufferMgr;
        bufMgrList[1] = &m_sccReprocessingBufferMgr;
        break;
    case PIPE_MCSC_REPROCESSING:
        bufMgrList[0] = &m_sccBufferMgr;
        bufMgrList[1] = &m_sccReprocessingBufferMgr;
        break;
    case PIPE_ISPC_REPROCESSING:
    case PIPE_SCC_REPROCESSING:
        bufMgrList[0] = NULL;
        bufMgrList[1] = &m_sccReprocessingBufferMgr;
        break;
    case PIPE_GSC_REPROCESSING:
        bufMgrList[0] = &m_sccReprocessingBufferMgr;
        bufMgrList[1] = &m_gscBufferMgr;
        break;
    default:
        CLOGE("ERR(%s[%d]): Unknown pipeId(%d)", __FUNCTION__, __LINE__, pipeId);
        bufMgrList[0] = NULL;
        bufMgrList[1] = NULL;
        ret = BAD_VALUE;
        break;
    }

    if (bufMgrList[direction] != NULL)
        *bufMgr = *bufMgrList[direction];

    return ret;
}

#ifdef RAWDUMP_CAPTURE
bool ExynosCamera::m_RawCaptureDumpThreadFunc(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    CLOGI("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    int ret = 0;
    int sensorMaxW, sensorMaxH;
    int sensorMarginW, sensorMarginH;
    bool bRet;
    char filePath[70];
    ExynosCameraBufferManager *bufferMgr = NULL;
    int bayerPipeId = 0;
    ExynosCameraBuffer bayerBuffer;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraFrame *inListFrame = NULL;
    unsigned int fliteFcount = 0;

    ret = m_RawCaptureDumpQ->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        // TODO: doing exception handling
        goto CLEAN;
    }
    if (newFrame == NULL) {
        CLOGE("ERR(%s[%d]):newFrame is NULL", __FUNCTION__, __LINE__);
        goto CLEAN;
    }

    bayerPipeId = newFrame->getFirstEntity()->getPipeId();
    ret = newFrame->getDstBuffer(bayerPipeId, &bayerBuffer);
    ret = m_getBufferManager(bayerPipeId, &bufferMgr, DST_BUFFER_DIRECTION);

    /* Rawdump capture is available in pure bayer only */
    if (m_parameters->getUsePureBayerReprocessing() == true) {
        camera2_shot_ext *shot_ext = NULL;
        shot_ext = (camera2_shot_ext *)(bayerBuffer.addr[1]);
        if (shot_ext != NULL)
            fliteFcount = shot_ext->shot.dm.request.frameCount;
        else
            ALOGE("ERR(%s[%d]):fliteReprocessingBuffer is null", __FUNCTION__, __LINE__);
    } else {
        camera2_stream *shot_stream = NULL;
        shot_stream = (camera2_stream *)(bayerBuffer.addr[1]);
        if (shot_stream != NULL)
            fliteFcount = shot_stream->fcount;
        else
            ALOGE("ERR(%s[%d]):fliteReprocessingBuffer is null", __FUNCTION__, __LINE__);
    }

    memset(filePath, 0, sizeof(filePath));
    snprintf(filePath, sizeof(filePath), "/data/media/0/RawCapture%d_%d.raw",
        m_parameters->getCameraId(), fliteFcount);
    /* Pure Bayer Buffer Size == MaxPictureSize + Sensor Margin == Max Sensor Size */
    m_parameters->getMaxSensorSize(&sensorMaxW, &sensorMaxH);

    CLOGD("INFO(%s[%d]):Raw Dump start (%s)", __FUNCTION__, __LINE__, filePath);

    bRet = dumpToFile((char *)filePath,
        bayerBuffer.addr[0],
        sensorMaxW * sensorMaxH * 2);
    if (bRet != true)
        ALOGE("couldn't make a raw file", __FUNCTION__, __LINE__);

    ret = bufferMgr->putBuffer(bayerBuffer.index, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):putIndex is %d", __FUNCTION__, __LINE__, bayerBuffer.index);
        bufferMgr->printBufferState();
        bufferMgr->printBufferQState();
    }

CLEAN:

    if (newFrame != NULL) {
        newFrame->frameUnlock();

        ret = m_searchFrameFromList(&m_processList, newFrame->getFrameCount(), &inListFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):searchFrameFromList fail", __FUNCTION__, __LINE__);
        } else {
            if (inListFrame == NULL) {
                CLOGD("DEBUG(%s[%d]): Selected frame(%d) complete, Delete",
                    __FUNCTION__, __LINE__, newFrame->getFrameCount());
                newFrame->decRef();
                m_frameMgr->deleteFrame(newFrame);
            }
            newFrame = NULL;
        }
    }

    return ret;
}
#endif

status_t ExynosCamera::m_getBayerBuffer(uint32_t pipeId, ExynosCameraBuffer *buffer, camera2_shot_ext *updateDmShot)
{
    status_t ret = NO_ERROR;
    bool isSrc = false;
    int retryCount = 30; /* 200ms x 30 */
    camera2_shot_ext *shot_ext = NULL;
    camera2_stream *shot_stream = NULL;
    ExynosCameraFrame *inListFrame = NULL;
    ExynosCameraFrame *bayerFrame = NULL;

    m_captureSelector->setWaitTime(200000000);
#ifdef RAWDUMP_CAPTURE
    for (int i = 0; i < 2; i++) {
        bayerFrame = m_captureSelector->selectFrames(m_reprocessingCounter.getCount(), pipeId, isSrc, retryCount);

        if(i == 0) {
            m_RawCaptureDumpQ->pushProcessQ(&bayerFrame);
        } else if (i == 1) {
            m_parameters->setRawCaptureModeOn(false);
        }
    }
#else
        bayerFrame = m_captureSelector->selectFrames(m_reprocessingCounter.getCount(), pipeId, isSrc, retryCount);
#endif
    if (bayerFrame == NULL) {
        CLOGE("ERR(%s[%d]):bayerFrame is NULL", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        goto CLEAN;
    }

    if (pipeId == PIPE_3AA) {
        ret = bayerFrame->getDstBuffer(pipeId, buffer, m_previewFrameFactory->getNodeType(PIPE_3AC));
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): getDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
            goto CLEAN;
        }
    } else if (pipeId == PIPE_FLITE) {
        ret = bayerFrame->getDstBuffer(pipeId, buffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): getDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
            goto CLEAN;
        }
    }

    if (m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON ||
        m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_PURE_DYNAMIC) {
        shot_ext = (struct camera2_shot_ext *)buffer->addr[1];
        CLOGD("DEBUG(%s[%d]): Selected frame count(hal : %d / driver : %d)", __FUNCTION__, __LINE__,
            bayerFrame->getFrameCount(), shot_ext->shot.dm.request.frameCount);
    } else if (m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON ||
        m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC) {
        if (updateDmShot == NULL) {
            CLOGE("ERR(%s[%d]): updateDmShot is NULL", __FUNCTION__, __LINE__);
            goto CLEAN;
        }

        retryCount = 12; /* 80ms * 12 */
        while(retryCount > 0) {
            if(bayerFrame->getMetaDataEnable() == false) {
                CLOGD("DEBUG(%s[%d]): Waiting for update jpeg metadata failed (%d), retryCount(%d)", __FUNCTION__, __LINE__, ret, retryCount);
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
        CLOGD("DEBUG(%s[%d]): Selected fcount(hal : %d / driver : %d)", __FUNCTION__, __LINE__,
            bayerFrame->getFrameCount(), shot_stream->fcount);
    } else {
        CLOGE("ERR(%s[%d]): reprocessing is not valid pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
        goto CLEAN;
    }

CLEAN:

    if (bayerFrame != NULL) {
        bayerFrame->frameUnlock();

        ret = m_searchFrameFromList(&m_processList, bayerFrame->getFrameCount(), &inListFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):searchFrameFromList fail", __FUNCTION__, __LINE__);
        } else {
            CLOGD("DEBUG(%s[%d]): Selected frame(%d) complete, Delete", __FUNCTION__, __LINE__, bayerFrame->getFrameCount());
            bayerFrame->decRef();
            m_frameMgr->deleteFrame(bayerFrame);
            bayerFrame = NULL;
        }
    }

    return ret;
}

/* vision */
status_t ExynosCamera::m_startVisionInternal(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    CLOGI("DEBUG(%s[%d]):IN", __FUNCTION__, __LINE__);

    CLOGI("DEBUG(%s[%d]):OUT", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCamera::m_stopVisionInternal(void)
{
    int ret = 0;

    CLOGI("DEBUG(%s[%d]):IN", __FUNCTION__, __LINE__);

    CLOGI("DEBUG(%s[%d]):OUT", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCamera::generateFrameVision(__unused int32_t frameCount, __unused ExynosCameraFrame **newFrame)
{
    Mutex::Autolock lock(m_frameLock);

    int ret = 0;

    return ret;
}

status_t ExynosCamera::m_setVisionBuffers(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    CLOGI("INFO(%s[%d]):alloc buffer - camera ID: %d",
        __FUNCTION__, __LINE__, m_cameraId);

    return NO_ERROR;
}

status_t ExynosCamera::m_setVisionCallbackBuffer(void)
{
    return NO_ERROR;
}


bool ExynosCamera::m_visionThreadFunc(void)
{

    return false;
}

status_t ExynosCamera::m_startCompanion(void)
{
    return NO_ERROR;
}

status_t ExynosCamera::m_stopCompanion(void)
{
    return NO_ERROR;
}

status_t ExynosCamera::startPreview()
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);
    bool needRestartPreview = false;
    if ((m_parameters== NULL) && (m_frameMgr == NULL)) {
        CLOGE("INFO(%s[%d]) initialize HAL", __FUNCTION__, __LINE__);
        needRestartPreview = true;
        this->initialize();
    }

    int ret = 0;
    int32_t skipFrameCount = INITIAL_SKIP_FRAME;
    unsigned int fdCallbackSize = 0;

    m_hdrSkipedFcount = 0;
    m_isTryStopFlash= false;
    m_exitAutoFocusThread = false;
    m_curMinFps = 0;
    m_isNeedAllocPictureBuffer = false;
    m_flagThreadStop= false;
    m_frameSkipCount = 0;

#ifdef FIRST_PREVIEW_TIME_CHECK
    if (m_flagFirstPreviewTimerOn == false) {
        m_firstPreviewTimer.start();
        m_flagFirstPreviewTimerOn = true;

        CLOGD("DEBUG(%s[%d]):m_firstPreviewTimer start", __FUNCTION__, __LINE__);
    }
#endif

    if (m_previewEnabled == true) {
        return INVALID_OPERATION;
    }

    /* frame manager start */
    m_frameMgr->start();

    fdCallbackSize = sizeof(camera_frame_metadata_t) * NUM_OF_DETECTED_FACES;

    if (m_getMemoryCb != NULL) {
        m_fdCallbackHeap = m_getMemoryCb(-1, fdCallbackSize, 1, m_callbackCookie);
        if (!m_fdCallbackHeap || m_fdCallbackHeap->data == MAP_FAILED) {
            CLOGE("ERR(%s[%d]):m_getMemoryCb(%d) fail", __FUNCTION__, __LINE__, fdCallbackSize);
            m_fdCallbackHeap = NULL;
            goto err;
        }
    }
    /*
     * This is for updating parameter value at once.
     * This must be just before making factory
     */
    m_parameters->updateTpuParameters();

    /* setup frameFactory with scenario */
    m_setupFrameFactory();

    /* vision */
    CLOGI("INFO(%s[%d]): getVisionMode(%d)", __FUNCTION__, __LINE__, m_parameters->getVisionMode());
    if (m_parameters->getVisionMode() == true) {
        ret = m_setVisionBuffers();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_setVisionCallbackBuffer() fail", __FUNCTION__, __LINE__);
            return ret;
        }

        ret = m_setVisionCallbackBuffer();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_setVisionCallbackBuffer() fail", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }

        if (m_visionFrameFactory->isCreated() == false) {
            ret = m_visionFrameFactory->create();
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):m_visionFrameFactory->create() failed", __FUNCTION__, __LINE__);
                goto err;
            }
            CLOGD("DEBUG(%s):FrameFactory(VisionFrameFactory) created", __FUNCTION__);
        }

        m_parameters->setFrameSkipCount(INITIAL_SKIP_FRAME);

        ret = m_startVisionInternal();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_startVisionInternal() failed", __FUNCTION__, __LINE__);
            goto err;
        }

        m_visionThread->run(PRIORITY_DEFAULT);
        return NO_ERROR;
    } else {
        m_parameters->setSeriesShotMode(SERIES_SHOT_MODE_NONE);

        if(m_parameters->increaseMaxBufferOfPreview()) {
            m_parameters->setPreviewBufferCount(NUM_PREVIEW_BUFFERS + NUM_PREVIEW_SPARE_BUFFERS);
        } else {
            m_parameters->setPreviewBufferCount(NUM_PREVIEW_BUFFERS);
        }

        if ((m_parameters->getRestartPreview() == true) ||
            m_previewBufferCount != m_parameters->getPreviewBufferCount() ||
            needRestartPreview == true) {
            ret = setPreviewWindow(m_previewWindow);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):setPreviewWindow fail", __FUNCTION__, __LINE__);
                return INVALID_OPERATION;
            }

            m_previewBufferCount = m_parameters->getPreviewBufferCount();
        }

        CLOGI("INFO(%s[%d]):setBuffersThread is run", __FUNCTION__, __LINE__);
        m_setBuffersThread->run(PRIORITY_DEFAULT);

        if (m_captureSelector == NULL) {
            ExynosCameraBufferManager *bufMgr = NULL;
#ifdef DEBUG_RAWDUMP
            bufMgr = m_bayerBufferMgr;
#else
            if (m_parameters->isReprocessing() == true) {
                if (m_parameters->isUseYuvReprocessing() == true
                    && m_parameters->isUsing3acForIspc() == true)
                    bufMgr = m_sccBufferMgr;
                else
                    bufMgr = m_bayerBufferMgr;
            }
#endif
            m_captureSelector = new ExynosCameraFrameSelector(m_parameters, bufMgr, m_frameMgr);

            if (m_parameters->isReprocessing() == true) {
                ret = m_captureSelector->setFrameHoldCount(REPROCESSING_BAYER_HOLD_COUNT);
                if (ret < 0)
                    CLOGE("ERR(%s[%d]): setFrameHoldCount(%d) is fail", __FUNCTION__, __LINE__, REPROCESSING_BAYER_HOLD_COUNT);
            }
        }

        if (m_sccCaptureSelector == NULL) {
            ExynosCameraBufferManager *bufMgr = NULL;

            if (m_parameters->isSccCapture() == true
                || m_parameters->isUsing3acForIspc() == true) {
                /* TODO: Dynamic select buffer manager for capture */
                bufMgr = m_sccBufferMgr;
            }

            m_sccCaptureSelector = new ExynosCameraFrameSelector(m_parameters, bufMgr, m_frameMgr);
        }

        if (m_captureSelector != NULL)
            m_captureSelector->release();

        if (m_sccCaptureSelector != NULL)
            m_sccCaptureSelector->release();

#ifdef RAWDUMP_CAPTURE
        ExynosCameraActivitySpecialCapture *m_sCapture = m_exynosCameraActivityControl->getSpecialCaptureMgr();
        m_sCapture->resetRawCaptureFcount();
#endif
        if (m_previewFrameFactory->isCreated() == false) {
            ret = m_previewFrameFactory->create();
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):m_previewFrameFactory->create() failed", __FUNCTION__, __LINE__);
                goto err;
            }
            CLOGD("DEBUG(%s):FrameFactory(previewFrameFactory) created", __FUNCTION__);
        }

        if (m_parameters->getDualMode() == true) {
            skipFrameCount = INITIAL_SKIP_FRAME + 2;
        }

#ifdef SET_FPS_SCENE /* This codes for 5260, Do not need other project */
        struct camera2_shot_ext *initMetaData = new struct camera2_shot_ext;
        if (initMetaData != NULL) {
            m_parameters->duplicateCtrlMetadata(initMetaData);

            ret = m_previewFrameFactory->setControl(V4L2_CID_IS_MIN_TARGET_FPS, initMetaData->shot.ctl.aa.aeTargetFpsRange[0], PIPE_FLITE);
            if (ret < 0)
                CLOGE("ERR(%s[%d]):FLITE setControl fail, ret(%d)", __FUNCTION__, __LINE__, ret);

            ret = m_previewFrameFactory->setControl(V4L2_CID_IS_MAX_TARGET_FPS, initMetaData->shot.ctl.aa.aeTargetFpsRange[1], PIPE_FLITE);
            if (ret < 0)
                CLOGE("ERR(%s[%d]):FLITE setControl fail, ret(%d)", __FUNCTION__, __LINE__, ret);

            ret = m_previewFrameFactory->setControl(V4L2_CID_IS_SCENE_MODE, initMetaData->shot.ctl.aa.sceneMode, PIPE_FLITE);
            if (ret < 0)
                CLOGE("ERR(%s[%d]):FLITE setControl fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            delete initMetaData;
            initMetaData = NULL;
        } else {
            CLOGE("ERR(%s[%d]):initMetaData is NULL", __FUNCTION__, __LINE__);
        }
#elif SET_FPS_FRONTCAM
        if (m_parameters->getCameraId() == CAMERA_ID_FRONT) {
            struct camera2_shot_ext *initMetaData = new struct camera2_shot_ext;
            if (initMetaData != NULL) {
                m_parameters->duplicateCtrlMetadata(initMetaData);
                CLOGD("(%s:[%d]) : setControl for Frame Range.", __FUNCTION__, __LINE__);

                ret = m_previewFrameFactory->setControl(V4L2_CID_IS_MIN_TARGET_FPS, initMetaData->shot.ctl.aa.aeTargetFpsRange[0], PIPE_FLITE_FRONT);
                if (ret < 0)
                    CLOGE("ERR(%s[%d]):FLITE setControl fail, ret(%d)", __FUNCTION__, __LINE__, ret);

                ret = m_previewFrameFactory->setControl(V4L2_CID_IS_MAX_TARGET_FPS, initMetaData->shot.ctl.aa.aeTargetFpsRange[1], PIPE_FLITE_FRONT);
                if (ret < 0)
                    CLOGE("ERR(%s[%d]):FLITE setControl fail, ret(%d)", __FUNCTION__, __LINE__, ret);

                delete initMetaData;
                initMetaData = NULL;
            } else {
                CLOGE("ERR(%s[%d]):initMetaData is NULL", __FUNCTION__, __LINE__);
            }
        }
#endif

        m_parameters->setFrameSkipCount(skipFrameCount);

        m_setBuffersThread->join();

        if (m_isSuccessedBufferAllocation == false) {
            CLOGE("ERR(%s[%d]):m_setBuffersThread() failed", __FUNCTION__, __LINE__);
            goto err;
        }

        ret = m_startPreviewInternal();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_startPreviewInternal() failed", __FUNCTION__, __LINE__);
            goto err;
        }

        if (m_parameters->isReprocessing() == true) {
#ifdef START_PICTURE_THREAD
#if !defined(USE_SNAPSHOT_ON_UHD_RECORDING)
            if (!m_parameters->getDualRecordingHint() && !m_parameters->getUHDRecordingMode())
#endif
            {
            m_startPictureInternalThread->run(PRIORITY_DEFAULT);
            }
#endif
        } else {
            m_pictureFrameFactory = m_previewFrameFactory;
            CLOGD("DEBUG(%s[%d]):FrameFactory(pictureFrameFactory) created", __FUNCTION__, __LINE__);

            /*
             * Make remained frameFactory here.
             * in case of SCC capture, make here.
             */
            m_framefactoryThread->run();
        }

#if defined(USE_UHD_RECORDING) && !defined(USE_SNAPSHOT_ON_UHD_RECORDING)
        if (!m_parameters->getDualRecordingHint() && !m_parameters->getUHDRecordingMode())
#endif
        {
            m_startPictureBufferThread->run(PRIORITY_DEFAULT);
        }

        if (m_previewWindow != NULL)
            m_previewWindow->set_timestamp(m_previewWindow, systemTime(SYSTEM_TIME_MONOTONIC));

#if defined(RAWDUMP_CAPTURE) || defined(DEBUG_RAWDUMP)
        m_mainSetupQThread[INDEX(PIPE_FLITE)]->run(PRIORITY_URGENT_DISPLAY);
#endif
        /* setup frame thread */
        if (m_parameters->getDualMode() == true && getCameraId() == CAMERA_ID_FRONT) {
            CLOGD("DEBUG(%s[%d]):setupThread Thread start pipeId(%d)", __FUNCTION__, __LINE__, PIPE_FLITE);
            m_mainSetupQThread[INDEX(PIPE_FLITE)]->run(PRIORITY_URGENT_DISPLAY);
        } else {
            switch (m_parameters->getReprocessingBayerMode()) {
            case REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON:
                m_mainSetupQ[INDEX(PIPE_FLITE)]->setup(NULL);
                CLOGD("DEBUG(%s[%d]):setupThread Thread start pipeId(%d)", __FUNCTION__, __LINE__, PIPE_FLITE);
                m_mainSetupQThread[INDEX(PIPE_FLITE)]->run(PRIORITY_URGENT_DISPLAY);
                break;
            case REPROCESSING_BAYER_MODE_PURE_DYNAMIC:
                CLOGD("DEBUG(%s[%d]):setupThread with List pipeId(%d)", __FUNCTION__, __LINE__, PIPE_FLITE);
                m_mainSetupQ[INDEX(PIPE_FLITE)]->setup(m_mainSetupQThread[INDEX(PIPE_FLITE)]);
                break;
            default:
                CLOGI("INFO(%s[%d]):setupThread not started pipeID(%d)", __FUNCTION__, __LINE__, PIPE_FLITE);
                break;
            }
            CLOGD("DEBUG(%s[%d]):setupThread Thread start pipeId(%d)", __FUNCTION__, __LINE__, PIPE_3AA);
            m_mainSetupQThread[INDEX(PIPE_3AA)]->run(PRIORITY_URGENT_DISPLAY);
        }

        if (m_facedetectThread->isRunning() == false)
            m_facedetectThread->run();

        m_previewThread->run(PRIORITY_DISPLAY);
        m_mainThread->run(PRIORITY_DEFAULT);
        if(m_parameters->getCameraId() == CAMERA_ID_BACK)
            m_autoFocusContinousThread->run(PRIORITY_DEFAULT);

        m_monitorThread->run(PRIORITY_DEFAULT);

        if ((m_parameters->getHighResolutionCallbackMode() == true) &&
            (m_highResolutionCallbackRunning == false)) {
            CLOGD("DEBUG(%s[%d]):High resolution preview callback start", __FUNCTION__, __LINE__);

            m_highResolutionCallbackRunning = true;
            if (skipFrameCount > 0)
                m_skipReprocessing = true;

            if (m_parameters->isReprocessing() == true) {
                if (m_parameters->isHWFCEnabled() == true) {
                    m_reprocessingFrameFactory->setRequest(PIPE_HWFC_JPEG_DST_REPROCESSING, false);
                    m_reprocessingFrameFactory->setRequest(PIPE_HWFC_THUMB_DST_REPROCESSING, false);
                }

                m_startPictureInternalThread->run(PRIORITY_DEFAULT);
                m_startPictureInternalThread->join();
            }
            m_prePictureThread->run(PRIORITY_DEFAULT);
        }

        /* FD-AE is always on */
#ifdef USE_FD_AE
        m_startFaceDetection(m_parameters->getFaceDetectionMode());
#endif
    }

#ifdef BURST_CAPTURE
    m_burstInitFirst = true;
#endif
    return NO_ERROR;

err:

    /* frame manager stop */
    m_frameMgr->stop();

    m_setBuffersThread->join();

    m_releaseBuffers();

    return ret;
}

void ExynosCamera::stopPreview()
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;

    ExynosCameraActivityFlash *m_flashMgr = m_exynosCameraActivityControl->getFlashMgr();
    ExynosCameraActivityAutofocus *autoFocusMgr = m_exynosCameraActivityControl->getAutoFocusMgr();
    ExynosCameraFrame *frame = NULL;

    if (m_previewEnabled == false) {
        CLOGD("DEBUG(%s[%d]): preview is not enabled", __FUNCTION__, __LINE__);
        return;
    }

    if (m_pictureEnabled == true) {
        CLOGW("WARN(%s[%d]):m_pictureEnabled == true (picture is not finished)", __FUNCTION__, __LINE__);
        int retry = 0;
        do {
            usleep(WAITING_TIME);
            retry++;
        } while(m_pictureEnabled == true && retry < (TOTAL_WAITING_TIME/WAITING_TIME));
        CLOGW("WARN(%s[%d]):wait (%d)msec (because, picture is not finished)", __FUNCTION__, __LINE__, WAITING_TIME * retry / 1000);
    }
    if (m_parameters->getVisionMode() == true) {
        m_frameFactoryQ->release();
        m_visionThread->requestExitAndWait();
        ret = m_stopVisionInternal();
        if (ret < 0)
            CLOGE("ERR(%s[%d]):m_stopVisionInternal fail", __FUNCTION__, __LINE__);
    } else {
        m_startPictureInternalThread->join();

        /* release about frameFactory */
        m_framefactoryThread->stop();
        m_frameFactoryQ->sendCmd(WAKE_UP);
        m_framefactoryThread->requestExitAndWait();
        m_frameFactoryQ->release();

        m_startPictureBufferThread->join();

        m_autoFocusRunning = false;
        m_exynosCameraActivityControl->cancelAutoFocus();

        CLOGD("DEBUG(%s[%d]): (%d, %d)", __FUNCTION__, __LINE__, m_flashMgr->getNeedCaptureFlash(), m_pictureEnabled);
        if (m_flashMgr->getNeedCaptureFlash() == true && m_pictureEnabled == true) {
            CLOGD("DEBUG(%s[%d]): force flash off", __FUNCTION__, __LINE__);
            m_exynosCameraActivityControl->cancelFlash();
            autoFocusMgr->stopAutofocus();
            m_isTryStopFlash = true;
            /* m_exitAutoFocusThread = true; */
        }

        /* Wait the end of the autoFocus Thread in order to the autofocus and the pre-flash is completed.*/
        m_autoFocusLock.lock();
        m_exitAutoFocusThread = true;
        m_autoFocusLock.unlock();

        int flashMode = AA_FLASHMODE_OFF;
        int waitingTime = FLASH_OFF_MAX_WATING_TIME / TOTAL_FLASH_WATING_COUNT; /* Max waiting time: 500ms, Count:10, Waiting time: 50ms */

        flashMode = m_flashMgr->getFlashStatus();
        if ((flashMode == AA_FLASHMODE_ON_ALWAYS) || (m_flashMgr->getNeedFlashOffDelay() == true)) {
            int i = 0;
            CLOGD("DEBUG(%s[%d]): flash torch was enabled", __FUNCTION__, __LINE__);

            m_parameters->setFrameSkipCount(100);
            do {
                if (m_flashMgr->checkFlashOff() == false) {
                    usleep(waitingTime);
                } else {
                    CLOGD("DEBUG(%s[%d]):turn off the flash torch.(%d)", __FUNCTION__, __LINE__, i);

                    flashMode = m_flashMgr->getFlashStatus();
                    if (flashMode == AA_FLASHMODE_OFF || flashMode == AA_FLASHMODE_CANCEL)
                        m_flashMgr->setFlashStep(ExynosCameraActivityFlash::FLASH_STEP_OFF);
                    usleep(waitingTime);
                    break;
                }
            } while(++i < TOTAL_FLASH_WATING_COUNT);
            if (i >= TOTAL_FLASH_WATING_COUNT) {
                CLOGD("DEBUG(%s[%d]):timeOut-flashMode(%d),checkFlashOff(%d)",
                    __FUNCTION__, __LINE__, flashMode, m_flashMgr->checkFlashOff());
            }
        } else if (m_isTryStopFlash == true) {
            usleep(waitingTime*3);  /* 150ms */
            m_flashMgr->setFlashStep(ExynosCameraActivityFlash::FLASH_STEP_OFF);
        }

        m_flashMgr->setNeedFlashOffDelay(false);

        m_previewFrameFactory->setStopFlag();
        if (m_parameters->isReprocessing() == true && m_reprocessingFrameFactory->isCreated() == true)
            m_reprocessingFrameFactory->setStopFlag();
        m_flagThreadStop = true;

        m_takePictureCounter.clearCount();
        m_reprocessingCounter.clearCount();
        m_pictureCounter.clearCount();
        m_jpegCounter.clearCount();
        m_captureSelector->cancelPicture();

        if ((m_parameters->getHighResolutionCallbackMode() == true) &&
            (m_highResolutionCallbackRunning == true)) {
            m_skipReprocessing = false;
            m_highResolutionCallbackRunning = false;

            if (m_parameters->isReprocessing() == true
                && m_parameters->isHWFCEnabled() == true) {
                m_reprocessingFrameFactory->setRequest(PIPE_HWFC_JPEG_DST_REPROCESSING, true);
                m_reprocessingFrameFactory->setRequest(PIPE_HWFC_THUMB_DST_REPROCESSING, true);
            }

            CLOGD("DEBUG(%s[%d]):High resolution preview callback stop", __FUNCTION__, __LINE__);

            if (m_parameters->isReprocessing() == false) {
                m_sccCaptureSelector->cancelPicture();
                m_sccCaptureSelector->wakeupQ();
                CLOGD("DEBUG(%s[%d]):High resolution m_sccCaptureSelector cancel", __FUNCTION__, __LINE__);
            }

            m_prePictureThread->requestExitAndWait();
            m_highResolutionCallbackQ->release();
        }

        ret = m_stopPictureInternal();
        if (ret < 0)
            CLOGE("ERR(%s[%d]):m_stopPictureInternal fail", __FUNCTION__, __LINE__);

        m_exynosCameraActivityControl->stopAutoFocus();
        m_autoFocusThread->requestExitAndWait();

        if (m_previewQ != NULL) {
            m_previewQ->sendCmd(WAKE_UP);
        } else {
            CLOGI("INFO(%s[%d]): m_previewQ is NULL", __FUNCTION__, __LINE__);
        }

        m_pipeFrameDoneQ->sendCmd(WAKE_UP);
        m_mainThread->requestExitAndWait();
        m_monitorThread->requestExitAndWait();

        if (m_parameters->isMcscVraOtf() == false) {
            m_vraThreadQ->sendCmd(WAKE_UP);
            m_vraGscDoneQ->sendCmd(WAKE_UP);
            m_vraPipeDoneQ->sendCmd(WAKE_UP);
            m_vraThread->requestExitAndWait();
            m_previewFrameFactory->stopThread(PIPE_VRA);
        }

        m_shutterCallbackThread->requestExitAndWait();

        m_previewThread->stop();
        if (m_previewQ != NULL) {
            m_previewQ->sendCmd(WAKE_UP);
        }
        m_previewThread->requestExitAndWait();

        if (m_parameters->isFlite3aaOtf() == true) {
            m_mainSetupQThread[INDEX(PIPE_FLITE)]->stop();
            m_mainSetupQ[INDEX(PIPE_FLITE)]->sendCmd(WAKE_UP);
            m_mainSetupQThread[INDEX(PIPE_FLITE)]->requestExitAndWait();

            if (m_mainSetupQThread[INDEX(PIPE_3AC)] != NULL) {
                m_mainSetupQThread[INDEX(PIPE_3AC)]->stop();
                m_mainSetupQ[INDEX(PIPE_3AC)]->sendCmd(WAKE_UP);
                m_mainSetupQThread[INDEX(PIPE_3AC)]->requestExitAndWait();
            }

            m_mainSetupQThread[INDEX(PIPE_3AA)]->stop();
            m_mainSetupQ[INDEX(PIPE_3AA)]->sendCmd(WAKE_UP);
            m_mainSetupQThread[INDEX(PIPE_3AA)]->requestExitAndWait();

            if (m_mainSetupQThread[INDEX(PIPE_ISP)] != NULL) {
                m_mainSetupQThread[INDEX(PIPE_ISP)]->stop();
                m_mainSetupQ[INDEX(PIPE_ISP)]->sendCmd(WAKE_UP);
                m_mainSetupQThread[INDEX(PIPE_ISP)]->requestExitAndWait();
            }

            /* Comment out, because it included ISP */
            /* m_mainSetupQThread[INDEX(PIPE_SCP)]->requestExitAndWait(); */
        } else {
            if (m_mainSetupQThread[INDEX(PIPE_FLITE)] != NULL) {
                m_mainSetupQThread[INDEX(PIPE_FLITE)]->stop();
                m_mainSetupQ[INDEX(PIPE_FLITE)]->sendCmd(WAKE_UP);
                m_mainSetupQThread[INDEX(PIPE_FLITE)]->requestExitAndWait();
            }
        }

        m_autoFocusContinousThread->stop();
        m_autoFocusContinousQ.sendCmd(WAKE_UP);
        m_autoFocusContinousThread->requestExitAndWait();
        m_autoFocusContinousQ.release();

        m_facedetectThread->stop();
        m_facedetectQ->sendCmd(WAKE_UP);
        m_facedetectThread->requestExitAndWait();
        while (m_facedetectQ->getSizeOfProcessQ()) {
            m_facedetectQ->popProcessQ(&frame);
            frame->decRef();
            m_frameMgr->deleteFrame(frame);
            frame = NULL;
        }

        for(int i = 0 ; i < MAX_NUM_PIPES ; i++ ) {
            m_clearList(m_mainSetupQ[i]);
        }

        ret = m_stopPreviewInternal();
        if (ret < 0)
            CLOGE("ERR(%s[%d]):m_stopPreviewInternal fail", __FUNCTION__, __LINE__);

        if (m_previewQ != NULL)
             m_clearList(m_previewQ);

        if (m_vraThreadQ != NULL)
             m_clearList(m_vraThreadQ);

        if (m_vraGscDoneQ != NULL)
             m_clearList(m_vraGscDoneQ);

        if (m_vraPipeDoneQ != NULL)
             m_clearList(m_vraPipeDoneQ);

        if (m_zoomPreviwWithCscQ != NULL)
             m_zoomPreviwWithCscQ->release();

        if (m_previewCallbackGscFrameDoneQ != NULL)
            m_clearList(m_previewCallbackGscFrameDoneQ);
    }

    /* skip to free and reallocate buffers : flite / 3aa / isp / ispReprocessing */
    if (m_bayerBufferMgr != NULL) {
        m_bayerBufferMgr->resetBuffers();
    }
    if (m_3aaBufferMgr != NULL) {
        m_3aaBufferMgr->resetBuffers();
    }
    if (m_ispBufferMgr != NULL) {
        m_ispBufferMgr->resetBuffers();
    }
    if (m_hwDisBufferMgr != NULL) {
        m_hwDisBufferMgr->resetBuffers();
    }
    if (m_vraBufferMgr != NULL) {
        m_vraBufferMgr->resetBuffers();
    }

    /* realloc reprocessing buffer for change burst panorama <-> normal mode */
    if (m_ispReprocessingBufferMgr != NULL) {
        m_ispReprocessingBufferMgr->resetBuffers();
    }
    if (m_sccReprocessingBufferMgr != NULL) {
        m_sccReprocessingBufferMgr->resetBuffers();
    }
    if (m_thumbnailBufferMgr != NULL) {
        m_thumbnailBufferMgr->resetBuffers();
    }

    /* realloc callback buffers */
    if (m_scpBufferMgr != NULL) {
        m_scpBufferMgr->deinit();
        m_scpBufferMgr->setBufferCount(0);
    }

    if (m_sccBufferMgr != NULL) {
        // libcamera: 75xx: Fix size issue in preview and capture // Vijayakumar S N <vijay.sathenahallin@samsung.com>
        if (m_parameters->isUsing3acForIspc() == true) {
            m_sccBufferMgr->deinit();
            m_sccBufferMgr->setBufferCount(0);
        } else {
            m_sccBufferMgr->resetBuffers();
        }
    }
    if (m_gscBufferMgr != NULL) {
        m_gscBufferMgr->resetBuffers();
    }

    if (m_jpegBufferMgr != NULL) {
        m_jpegBufferMgr->deinit();
    }

    if (m_hwDisBufferMgr != NULL) {
        m_hwDisBufferMgr->deinit();
    }
    if (m_vraBufferMgr != NULL) {
        m_vraBufferMgr->deinit();
    }

    if (m_recordingBufferMgr != NULL) {
        m_recordingBufferMgr->deinit();
    }
    if (m_previewCallbackBufferMgr != NULL) {
        m_previewCallbackBufferMgr->deinit();
    }
    if (m_highResolutionCallbackBufferMgr != NULL) {
        m_highResolutionCallbackBufferMgr->deinit();
    }
    if (m_captureSelector != NULL) {
        m_captureSelector->release();
    }
    if (m_sccCaptureSelector != NULL) {
        m_sccCaptureSelector->release();
    }

#if 0
    /* skip to free and reallocate buffers : flite / 3aa / isp / ispReprocessing */
    CLOGE(" m_setBuffers free all buffers");
    if (m_bayerBufferMgr != NULL) {
        m_bayerBufferMgr->deinit();
    }
    if (m_3aaBufferMgr != NULL) {
        m_3aaBufferMgr->deinit();
    }
    if (m_ispBufferMgr != NULL) {
        m_ispBufferMgr->deinit();
    }
    if (m_hwDisBufferMgr != NULL) {
        m_hwDisBufferMgr->deinit();
    }
#endif
    /* frame manager stop */
    m_frameMgr->stop();
    m_frameMgr->deleteAllFrame();


    m_reprocessingCounter.clearCount();
    m_pictureCounter.clearCount();

    m_hdrSkipedFcount = 0;
    m_dynamicSccCount = 0;

    /* HACK Reset Preview Flag*/
    m_resetPreview = false;

    m_isTryStopFlash= false;
    m_exitAutoFocusThread = false;
    m_isNeedAllocPictureBuffer = false;

    if (m_fdCallbackHeap != NULL) {
        m_fdCallbackHeap->release(m_fdCallbackHeap);
        m_fdCallbackHeap = NULL;
    }

    m_burstInitFirst = false;

#ifdef FORCE_RESET_MULTI_FRAME_FACTORY
    /*
     * HACK
     * This is force-reset frameFactory adn companion
     */
    m_deinitFrameFactory();
    m_initFrameFactory();
#endif
}

status_t ExynosCamera::setPreviewWindow(preview_stream_ops *w)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    status_t ret = NO_ERROR;
    int width, height;
    int halPreviewFmt = 0;
    bool flagRestart = false;
    buffer_manager_type bufferType = BUFFER_MANAGER_ION_TYPE;

    if (m_parameters != NULL) {
        if (m_parameters->getVisionMode() == true) {
            CLOGW("WRN(%s[%d]): Vision mode does not support", __FUNCTION__, __LINE__);
            /* android_printAssert(NULL, LOG_TAG, "Cannot support this operation"); */

            return NO_ERROR;
        }
    } else {
        CLOGW("(%s):m_parameters is NULL. Skipped", __FUNCTION__);
        return INVALID_OPERATION;
    }

    if (previewEnabled() == true) {
        CLOGW("WRN(%s[%d]): preview is started, we forcely re-start preview", __FUNCTION__, __LINE__);
        flagRestart = true;
        m_disablePreviewCB = true;
        stopPreview();
    }

    m_previewWindow = w;

    if (m_scpBufferMgr != NULL) {
        CLOGD("DEBUG(%s[%d]): scp buffer manager need recreate", __FUNCTION__, __LINE__);
        m_scpBufferMgr->deinit();

        delete m_scpBufferMgr;
        m_scpBufferMgr = NULL;
    }

    if (w == NULL) {
        bufferType = BUFFER_MANAGER_ION_TYPE;
        CLOGW("WARN(%s[%d]):window NULL, create internal buffer for preview", __FUNCTION__, __LINE__);
    } else {
        halPreviewFmt = m_parameters->getHalPixelFormat();
        bufferType = BUFFER_MANAGER_GRALLOC_TYPE;
        m_parameters->getHwPreviewSize(&width, &height);
        if (m_grAllocator == NULL)
            m_grAllocator = new ExynosCameraGrallocAllocator();

#ifdef RESERVED_MEMORY_FOR_GRALLOC_ENABLE
        if (!(((m_parameters->getShotMode() == SHOT_MODE_BEAUTY_FACE) && (getCameraId() == CAMERA_ID_BACK))
            || m_parameters->getRecordingHint() == true)) {
            ret = m_grAllocator->init(m_previewWindow, m_exynosconfig->current->bufInfo.num_preview_buffers,
                                    m_exynosconfig->current->bufInfo.preview_buffer_margin, (GRALLOC_SET_USAGE_FOR_CAMERA | GRALLOC_USAGE_CAMERA_RESERVED));
        } else
#endif
        {
            ret = m_grAllocator->init(m_previewWindow, m_exynosconfig->current->bufInfo.num_preview_buffers, m_exynosconfig->current->bufInfo.preview_buffer_margin);
        }

        if (ret < 0) {
            CLOGE("ERR(%s[%d]):gralloc init fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            goto func_exit;
        }

        ret = m_grAllocator->setBuffersGeometry(width, height, halPreviewFmt);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):gralloc setBufferGeomety fail, size(%dx%d), fmt(%d), ret(%d)",
                __FUNCTION__, __LINE__, width, height, halPreviewFmt, ret);
            goto func_exit;
        }
    }

    m_createBufferManager(&m_scpBufferMgr, "SCP_BUF", bufferType);

    if (bufferType == BUFFER_MANAGER_GRALLOC_TYPE)
        m_scpBufferMgr->setAllocator(m_grAllocator);

    if (flagRestart == true) {
        startPreview();
    }

func_exit:
    m_disablePreviewCB = false;

    return ret;
}

status_t ExynosCamera::m_startPreviewInternal(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    CLOGI("DEBUG(%s[%d]):IN", __FUNCTION__, __LINE__);

    uint32_t minBayerFrameNum = 0;
    uint32_t min3AAFrameNum = 0;
    int ret = 0;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer dstBuf;
    int32_t reprocessingBayerMode = m_parameters->getReprocessingBayerMode();
    enum pipeline pipe;
    int retrycount = 0;

    m_fliteFrameCount = 0;
    m_3aa_ispFrameCount = 0;
    m_ispFrameCount = 0;
    m_sccFrameCount = 0;
    m_scpFrameCount = 0;
    m_displayPreviewToggle = 0;

    if (m_parameters->isFlite3aaOtf() == true)
        minBayerFrameNum = m_exynosconfig->current->bufInfo.init_bayer_buffers;
    else
        minBayerFrameNum = m_exynosconfig->current->bufInfo.num_bayer_buffers
                    - m_exynosconfig->current->bufInfo.reprocessing_bayer_hold_count;

    /*
     * with MCPipe, we need to putBuffer 3 buf.
     */
    /*
    if (reprocessingBayerMode == REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON)
        min3AAFrameNum = minBayerFrameNum;
    else
        min3AAFrameNum = m_exynosconfig->current->pipeInfo.prepare[PIPE_3AA];
    */

    min3AAFrameNum = m_exynosconfig->current->pipeInfo.prepare[PIPE_3AA];

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

#ifdef FPS_CHECK
    for (int i = 0; i < DEBUG_MAX_PIPE_NUM; i++)
        m_debugFpsCount[i] = 0;
#endif

    switch (reprocessingBayerMode) {
        case REPROCESSING_BAYER_MODE_NONE : /* Not using reprocessing */
            CLOGD("DEBUG(%s[%d]): Use REPROCESSING_BAYER_MODE_NONE", __FUNCTION__, __LINE__);
#ifdef DEBUG_RAWDUMP
            m_previewFrameFactory->setRequestFLITE(true);
#else
            m_previewFrameFactory->setRequestFLITE(false);
#endif
            m_previewFrameFactory->setRequest3AC(false);
            break;
        case REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON :
            CLOGD("DEBUG(%s[%d]): Use REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON", __FUNCTION__, __LINE__);
            m_previewFrameFactory->setRequestFLITE(true);
            m_previewFrameFactory->setRequest3AC(false);
            break;
        case REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON :
            CLOGD("DEBUG(%s[%d]): Use REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON", __FUNCTION__, __LINE__);
            m_previewFrameFactory->setRequestFLITE(false);
            m_previewFrameFactory->setRequest3AC(true);
            break;
        case REPROCESSING_BAYER_MODE_PURE_DYNAMIC :
            CLOGD("DEBUG(%s[%d]): Use REPROCESSING_BAYER_MODE_PURE_DYNAMIC", __FUNCTION__, __LINE__);
            m_previewFrameFactory->setRequestFLITE(false);
            m_previewFrameFactory->setRequest3AC(false);
            break;
        case REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC :
            CLOGD("DEBUG(%s[%d]): Use REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC", __FUNCTION__, __LINE__);
            m_previewFrameFactory->setRequestFLITE(false);
            m_previewFrameFactory->setRequest3AC(false);
            break;
        default :
            CLOGE("ERR(%s[%d]): Unknown dynamic bayer mode", __FUNCTION__, __LINE__);
            m_previewFrameFactory->setRequest3AC(false);
            break;
    }

    if (m_parameters->getTpuEnabledMode() == true) {
        m_previewFrameFactory->setRequestISPP(true);
        m_previewFrameFactory->setRequestDIS(true);

        if (m_parameters->is3aaIspOtf() == true) {
            taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_3AA)] = m_3aaBufferMgr;
            taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_3AC)] = m_bayerBufferMgr;
            taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_ISPP)] = m_hwDisBufferMgr;

            disBufferManager[m_previewFrameFactory->getNodeType(PIPE_DIS)] = m_hwDisBufferMgr;
            disBufferManager[m_previewFrameFactory->getNodeType(PIPE_SCP)] = m_scpBufferMgr;
        } else {
            taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_3AA)] = m_3aaBufferMgr;
            taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_3AC)] = m_bayerBufferMgr;
            taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_3AP)] = m_ispBufferMgr;

            ispBufferManager[m_previewFrameFactory->getNodeType(PIPE_ISP)] = m_ispBufferMgr;
            ispBufferManager[m_previewFrameFactory->getNodeType(PIPE_ISPP)] = m_hwDisBufferMgr;

            disBufferManager[m_previewFrameFactory->getNodeType(PIPE_DIS)] = m_hwDisBufferMgr;
            disBufferManager[m_previewFrameFactory->getNodeType(PIPE_SCP)] = m_scpBufferMgr;
        }
     } else {
        m_previewFrameFactory->setRequestISPP(false);
        m_previewFrameFactory->setRequestDIS(false);

        if (m_parameters->is3aaIspOtf() == true) {
            if (m_parameters->getDualMode() == true && getCameraId() == CAMERA_ID_FRONT) {
                m_previewFrameFactory->setRequestFLITE(true);
                m_previewFrameFactory->setRequestISPC(true);

                taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_3AA)] = m_bayerBufferMgr;
                taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_ISPC)] = m_sccBufferMgr;
            } else {
                if (m_parameters->isUsing3acForIspc() == true) {
                    if (m_parameters->getRecordingHint() == true)
                        m_previewFrameFactory->setRequest3AC(false);
                    else
                        m_previewFrameFactory->setRequest3AC(true);
                }

                taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_3AA)] = m_3aaBufferMgr;
                if (m_parameters->isUsing3acForIspc() == true)
                    taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_3AC)] = m_sccBufferMgr;
#ifndef RAWDUMP_CAPTURE
                else
                    taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_3AC)] = m_bayerBufferMgr;
#endif
                taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_SCP)] = m_scpBufferMgr;
            }
        } else {
            taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_3AA)] = m_3aaBufferMgr;
            taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_3AC)] = m_bayerBufferMgr;
            taaBufferManager[m_previewFrameFactory->getNodeType(PIPE_3AP)] = m_ispBufferMgr;

            ispBufferManager[m_previewFrameFactory->getNodeType(PIPE_ISP)] = m_ispBufferMgr;
            ispBufferManager[m_previewFrameFactory->getNodeType(PIPE_SCP)] = m_scpBufferMgr;
        }
    }

    if (m_parameters->isMcscVraOtf() == false) {
        vraBufferManager[OUTPUT_NODE] = m_vraBufferMgr;

        ret = m_previewFrameFactory->setBufferManagerToPipe(vraBufferManager, PIPE_VRA);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_previewFrameFactory->setBufferManagerToPipe(vraBufferManager, %d) failed",
                    __FUNCTION__, __LINE__, PIPE_VRA);
            return ret;
        }
    }

    for (int i = 0; i < MAX_NODE; i++) {
        /* If even one buffer slot is valid. call setBufferManagerToPipe() */
        if (taaBufferManager[i] != NULL) {
            ret = m_previewFrameFactory->setBufferManagerToPipe(taaBufferManager, PIPE_3AA);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_previewFrameFactory->setBufferManagerToPipe(taaBufferManager, %d) failed",
                    __FUNCTION__, __LINE__, PIPE_3AA);
                return ret;
            }
            break;
        }
    }

    for (int i = 0; i < MAX_NODE; i++) {
        /* If even one buffer slot is valid. call setBufferManagerToPipe() */
        if (ispBufferManager[i] != NULL) {
            ret = m_previewFrameFactory->setBufferManagerToPipe(ispBufferManager, PIPE_ISP);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_previewFrameFactory->setBufferManagerToPipe(ispBufferManager, %d) failed",
                    __FUNCTION__, __LINE__, PIPE_ISP);
                return ret;
            }
            break;
        }
    }
    if (m_parameters->getHWVdisMode()) {
        for (int i = 0; i < MAX_NODE; i++) {
            /* If even one buffer slot is valid. call setBufferManagerToPipe() */
            if (disBufferManager[i] != NULL) {
                ret = m_previewFrameFactory->setBufferManagerToPipe(disBufferManager, PIPE_DIS);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):m_previewFrameFactory->setBufferManagerToPipe(disBufferManager, %d) failed",
                        __FUNCTION__, __LINE__, PIPE_DIS);
                    return ret;
                }
                break;
            }
        }
    }

    ret = m_previewFrameFactory->initPipes();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_previewFrameFactory->initPipes() failed", __FUNCTION__, __LINE__);
        return ret;
    }

    m_printExynosCameraInfo(__FUNCTION__);

    for (uint32_t i = 0; i < minBayerFrameNum; i++) {
        retrycount = 0;
        do {
            ret = generateFrame(i, &newFrame);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):generateFrame fail", __FUNCTION__, __LINE__);
                usleep(100);
            }
            if (++retrycount >= 10) {
                return ret;
            }
        } while((ret < 0) && (retrycount < 10));

        if (newFrame == NULL) {
            CLOGE("ERR(%s[%d]):new faame is NULL", __FUNCTION__, __LINE__);
            return ret;
        }

        m_fliteFrameCount++;

        if (m_parameters->isFlite3aaOtf() == true) {
#ifndef DEBUG_RAWDUMP
            if (reprocessingBayerMode == REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON)
#endif
            {
                m_setupEntity(m_getBayerPipeId(), newFrame);
                m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, m_getBayerPipeId());
                m_previewFrameFactory->pushFrameToPipe(&newFrame, m_getBayerPipeId());
            }

            if (i < min3AAFrameNum) {
                m_setupEntity(PIPE_3AA, newFrame);

                if (m_parameters->is3aaIspOtf() == true) {
                    if (m_parameters->isMcscVraOtf() == true)
                        m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, PIPE_3AA);

                    if (m_parameters->getTpuEnabledMode() == true) {
                        m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, PIPE_DIS);
                    }
                } else {
                    m_previewFrameFactory->setFrameDoneQToPipe(m_pipeFrameDoneQ, PIPE_3AA);

                    if (m_parameters->getTpuEnabledMode() == true) {
                        m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, PIPE_DIS);
                        m_previewFrameFactory->setFrameDoneQToPipe(m_pipeFrameDoneQ, PIPE_ISP);
                    } else {
                        m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, PIPE_ISP);
                    }
                }

                m_previewFrameFactory->pushFrameToPipe(&newFrame, PIPE_3AA);
                m_3aa_ispFrameCount++;
            }
        } else {
            m_setupEntity(m_getBayerPipeId(), newFrame);
            m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, m_getBayerPipeId());
            m_previewFrameFactory->pushFrameToPipe(&newFrame, m_getBayerPipeId());

            if (m_parameters->is3aaIspOtf() == true) {
                m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, PIPE_3AA);
            } else {
                m_setupEntity(PIPE_3AA, newFrame);
                m_previewFrameFactory->setFrameDoneQToPipe(m_pipeFrameDoneQ, PIPE_3AA);
                m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, PIPE_ISP);
                m_previewFrameFactory->pushFrameToPipe(&newFrame, PIPE_3AA);
                m_3aa_ispFrameCount++;
            }

        }

        if (m_parameters->isMcscVraOtf() == false) {
            m_previewFrameFactory->setFrameDoneQToPipe(m_pipeFrameDoneQ, PIPE_3AA);
            m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, PIPE_VRA);
        }

#if 0
        /* SCC */
        if(m_parameters->isSccCapture() == true) {
            m_sccFrameCount++;

            if (isOwnScc(getCameraId()) == true) {
                pipe = PIPE_SCC;
            } else {
                pipe = PIPE_ISPC;
            }

            if(newFrame->getRequest(pipe)) {
                m_setupEntity(pipe, newFrame);
                m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, pipe);
                m_previewFrameFactory->pushFrameToPipe(&newFrame, pipe);
            }
        }
#endif
        /* SCP */
/* Comment out, because it included ISP */
/*
        m_scpFrameCount++;

        m_setupEntity(PIPE_SCP, newFrame);
        m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, PIPE_SCP);
        m_previewFrameFactory->pushFrameToPipe(&newFrame, PIPE_SCP);
*/
    }

/* Comment out, because it included ISP */
/*
    for (uint32_t i = minFrameNum; i < INIT_SCP_BUFFERS; i++) {
        ret = generateFrame(i, &newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):generateFrame fail", __FUNCTION__, __LINE__);
            return ret;
        }
        if (newFrame == NULL) {
            CLOGE("ERR(%s[%d]):new faame is NULL", __FUNCTION__, __LINE__);
            return ret;
        }

        m_scpFrameCount++;

        m_setupEntity(PIPE_SCP, newFrame);
        m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, PIPE_SCP);
        m_previewFrameFactory->pushFrameToPipe(&newFrame, PIPE_SCP);
    }
*/

    /* prepare pipes */
    ret = m_previewFrameFactory->preparePipes();
    if (ret < 0) {
        CLOGE("ERR(%s):preparePipe fail", __FUNCTION__);
        return ret;
    }

#ifndef START_PICTURE_THREAD
    if (m_parameters->isReprocessing() == true) {
        m_startPictureInternal();
    }
#endif

#ifdef RAWDUMP_CAPTURE
    /* s_ctrl HAL version for selecting dvfs table */
    ret = m_previewFrameFactory->setControl(V4L2_CID_IS_HAL_VERSION, IS_HAL_VER_3_2, PIPE_3AA);
    ALOGD("WARN(%s): V4L2_CID_IS_HAL_VERSION_%d pipe(%d)", __FUNCTION__, IS_HAL_VER_3_2, PIPE_3AA);
    if (ret < 0)
        ALOGW("WARN(%s): V4L2_CID_IS_HAL_VERSION is fail", __FUNCTION__);
#endif

    /* stream on pipes */
    ret = m_previewFrameFactory->startPipes();
    if (ret < 0) {
        m_previewFrameFactory->stopPipes();
        CLOGE("ERR(%s):startPipe fail", __FUNCTION__);
        return ret;
    }

    /* start all thread */
    ret = m_previewFrameFactory->startInitialThreads();
    if (ret < 0) {
        CLOGE("ERR(%s):startInitialThreads fail", __FUNCTION__);
        return ret;
    }

    m_previewEnabled = true;
    m_parameters->setPreviewRunning(m_previewEnabled);

    if (m_parameters->getFocusModeSetting() == true) {
        CLOGD("set Focus Mode(%s[%d])", __FUNCTION__, __LINE__);
        int focusmode = m_parameters->getFocusMode();
        m_exynosCameraActivityControl->setAutoFocusMode(focusmode);
        m_parameters->setFocusModeSetting(false);
    }

    CLOGI("DEBUG(%s[%d]):OUT", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCamera::m_stopPreviewInternal(void)
{
    int ret = 0;

    CLOGI("DEBUG(%s[%d]):IN", __FUNCTION__, __LINE__);
    if (m_previewFrameFactory != NULL) {
        ret = m_previewFrameFactory->stopPipes();
        if (ret < 0) {
            CLOGE("ERR(%s):stopPipe fail", __FUNCTION__);
            return ret;
        }
    }

    CLOGD("DEBUG(%s[%d]):clear process Frame list", __FUNCTION__, __LINE__);
    ret = m_clearList(&m_processList);
    if (ret < 0) {
        CLOGE("ERR(%s):m_clearList fail", __FUNCTION__);
        return ret;
    }

    /* clear previous recording frame */
    CLOGD("DEBUG(%s[%d]):Recording m_recordingProcessList(%d) IN",
        __FUNCTION__, __LINE__, m_recordingProcessList.size());
    m_recordingListLock.lock();
    ret = m_clearList(&m_recordingProcessList);
    if (ret < 0) {
        CLOGE("ERR(%s):m_clearList fail", __FUNCTION__);
    }
    m_recordingListLock.unlock();
    CLOGD("DEBUG(%s[%d]):Recording m_recordingProcessList(%d) OUT",
        __FUNCTION__, __LINE__, m_recordingProcessList.size());

    m_pipeFrameDoneQ->release();

    m_fliteFrameCount = 0;
    m_3aa_ispFrameCount = 0;
    m_ispFrameCount = 0;
    m_sccFrameCount = 0;
    m_scpFrameCount = 0;

    m_previewEnabled = false;
    m_parameters->setPreviewRunning(m_previewEnabled);

    CLOGI("DEBUG(%s[%d]):OUT", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCamera::m_restartPreviewInternal(void)
{
    CLOGI("INFO(%s[%d]): Internal restart preview", __FUNCTION__, __LINE__);
    int ret = 0;
    int err = 0;

    m_flagThreadStop = true;

    m_startPictureInternalThread->join();

    /* release about frameFactory */
    m_framefactoryThread->stop();
    m_frameFactoryQ->sendCmd(WAKE_UP);
    m_framefactoryThread->requestExitAndWait();
    m_frameFactoryQ->release();

    m_startPictureBufferThread->join();

    if (m_previewFrameFactory != NULL)
        m_previewFrameFactory->setStopFlag();

    m_mainThread->requestExitAndWait();

    ret = m_stopPictureInternal();
    if (ret < 0)
        CLOGE("ERR(%s[%d]):m_stopPictureInternal fail", __FUNCTION__, __LINE__);

    m_previewThread->stop();
    if(m_previewQ != NULL)
        m_previewQ->sendCmd(WAKE_UP);
    m_previewThread->requestExitAndWait();

    if (m_parameters->isMcscVraOtf() == false) {
        m_vraThreadQ->sendCmd(WAKE_UP);
        m_vraGscDoneQ->sendCmd(WAKE_UP);
        m_vraPipeDoneQ->sendCmd(WAKE_UP);
        m_vraThread->requestExitAndWait();
        m_previewFrameFactory->stopThread(PIPE_VRA);
    }

    if (m_parameters->isFlite3aaOtf() == true) {
        m_mainSetupQThread[INDEX(PIPE_FLITE)]->stop();
        m_mainSetupQ[INDEX(PIPE_FLITE)]->sendCmd(WAKE_UP);
        m_mainSetupQThread[INDEX(PIPE_FLITE)]->requestExitAndWait();

        if (m_mainSetupQThread[INDEX(PIPE_3AC)] != NULL) {
            m_mainSetupQThread[INDEX(PIPE_3AC)]->stop();
            m_mainSetupQ[INDEX(PIPE_3AC)]->sendCmd(WAKE_UP);
            m_mainSetupQThread[INDEX(PIPE_3AC)]->requestExitAndWait();
        }

        m_mainSetupQThread[INDEX(PIPE_3AA)]->stop();
        m_mainSetupQ[INDEX(PIPE_3AA)]->sendCmd(WAKE_UP);
        m_mainSetupQThread[INDEX(PIPE_3AA)]->requestExitAndWait();

       if (m_mainSetupQThread[INDEX(PIPE_ISP)] != NULL) {
            m_mainSetupQThread[INDEX(PIPE_ISP)]->stop();
            m_mainSetupQ[INDEX(PIPE_ISP)]->sendCmd(WAKE_UP);
            m_mainSetupQThread[INDEX(PIPE_ISP)]->requestExitAndWait();
        }

        /* Comment out, because it included ISP */
        /*
        m_mainSetupQThread[INDEX(PIPE_SCP)]->stop();
        m_mainSetupQ[INDEX(PIPE_SCP)]->sendCmd(WAKE_UP);
        m_mainSetupQThread[INDEX(PIPE_SCP)]->requestExitAndWait();
        */

        m_clearList(m_mainSetupQ[INDEX(PIPE_FLITE)]);
        m_clearList(m_mainSetupQ[INDEX(PIPE_3AA)]);
        m_clearList(m_mainSetupQ[INDEX(PIPE_ISP)]);
        /* Comment out, because it included ISP */
        /* m_clearList(m_mainSetupQ[INDEX(PIPE_SCP)]); */

        m_mainSetupQ[INDEX(PIPE_FLITE)]->release();
        m_mainSetupQ[INDEX(PIPE_3AA)]->release();
        m_mainSetupQ[INDEX(PIPE_ISP)]->release();
        /* Comment out, because it included ISP */
        /* m_mainSetupQ[INDEX(PIPE_SCP)]->release(); */
    } else {
        if (m_mainSetupQThread[INDEX(PIPE_FLITE)] != NULL) {
            m_mainSetupQThread[INDEX(PIPE_FLITE)]->stop();
            m_mainSetupQ[INDEX(PIPE_FLITE)]->sendCmd(WAKE_UP);
            m_mainSetupQThread[INDEX(PIPE_FLITE)]->requestExitAndWait();
            m_clearList(m_mainSetupQ[INDEX(PIPE_FLITE)]);
            m_mainSetupQ[INDEX(PIPE_FLITE)]->release();
        }
    }

    ret = m_stopPreviewInternal();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_stopPreviewInternal fail", __FUNCTION__, __LINE__);
       err = ret;
    }

    if (m_previewQ != NULL)
        m_clearList(m_previewQ);

    if (m_vraThreadQ != NULL)
        m_clearList(m_vraThreadQ);

    if (m_vraGscDoneQ != NULL)
        m_clearList(m_vraGscDoneQ);

    if (m_vraPipeDoneQ != NULL)
        m_clearList(m_vraPipeDoneQ);

    if (m_zoomPreviwWithCscQ != NULL)
        m_zoomPreviwWithCscQ->release();

    if (m_previewCallbackGscFrameDoneQ != NULL)
        m_clearList(m_previewCallbackGscFrameDoneQ);

    /* skip to free and reallocate buffers */
    if (m_bayerBufferMgr != NULL) {
        m_bayerBufferMgr->resetBuffers();
    }
    if (m_3aaBufferMgr != NULL) {
        m_3aaBufferMgr->resetBuffers();
    }
    if (m_ispBufferMgr != NULL) {
        m_ispBufferMgr->resetBuffers();
    }
    if (m_hwDisBufferMgr != NULL) {
        m_hwDisBufferMgr->resetBuffers();
    }
    if (m_vraBufferMgr != NULL) {
        m_vraBufferMgr->deinit();
    }
    if (m_sccBufferMgr != NULL) {
        // libcamera: 75xx: Fix size issue in preview and capture // Vijayakumar S N <vijay.sathenahallin@samsung.com>
        if (m_parameters->isUsing3acForIspc() == true) {
            m_sccBufferMgr->deinit();
            m_sccBufferMgr->setBufferCount(0);
        } else {
            m_sccBufferMgr->resetBuffers();
        }
    }

    if (m_highResolutionCallbackBufferMgr != NULL) {
        m_highResolutionCallbackBufferMgr->resetBuffers();
    }

    /* skip to free and reallocate buffers */
    if (m_ispReprocessingBufferMgr != NULL) {
        m_ispReprocessingBufferMgr->resetBuffers();
    }
    if (m_sccReprocessingBufferMgr != NULL) {
        m_sccReprocessingBufferMgr->resetBuffers();
    }
    if (m_thumbnailBufferMgr != NULL) {
        m_thumbnailBufferMgr->resetBuffers();
    }

    if (m_gscBufferMgr != NULL) {
        m_gscBufferMgr->resetBuffers();
    }
    if (m_jpegBufferMgr != NULL) {
        m_jpegBufferMgr->resetBuffers();
    }
    if (m_recordingBufferMgr != NULL) {
        m_recordingBufferMgr->resetBuffers();
    }

    /* realloc callback buffers */
    if (m_scpBufferMgr != NULL) {
        m_scpBufferMgr->deinit();
        m_scpBufferMgr->setBufferCount(0);
    }
    if (m_previewCallbackBufferMgr != NULL) {
        m_previewCallbackBufferMgr->deinit();
    }

    if (m_captureSelector != NULL) {
        m_captureSelector->release();
    }
    if (m_sccCaptureSelector != NULL) {
        m_sccCaptureSelector->release();
    }

    if (m_parameters->getHighSpeedRecording() && m_parameters->getReallocBuffer()) {
        CLOGD("DEBUG(%s): realloc buffer all buffer deinit ", __FUNCTION__);
        if (m_bayerBufferMgr != NULL) {
            m_bayerBufferMgr->deinit();
        }
        if (m_3aaBufferMgr != NULL) {
            m_3aaBufferMgr->deinit();
        }
        if (m_ispBufferMgr != NULL) {
            m_ispBufferMgr->deinit();
        }

        if (m_sccBufferMgr != NULL) {
            m_sccBufferMgr->deinit();
        }
/*
        if (m_highResolutionCallbackBufferMgr != NULL) {
            m_highResolutionCallbackBufferMgr->deinit();
        }
*/
        if (m_gscBufferMgr != NULL) {
            m_gscBufferMgr->deinit();
        }
        if (m_jpegBufferMgr != NULL) {
            m_jpegBufferMgr->deinit();
        }
        if (m_recordingBufferMgr != NULL) {
            m_recordingBufferMgr->deinit();
        }
    }

    if (m_parameters->getTpuEnabledMode() == true) {
        if (m_hwDisBufferMgr != NULL) {
            m_hwDisBufferMgr->deinit();
        }
    }

    m_flagThreadStop = false;

    ret = setPreviewWindow(m_previewWindow);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setPreviewWindow fail", __FUNCTION__, __LINE__);
        err = ret;
    }

    CLOGI("INFO(%s[%d]):setBuffersThread is run", __FUNCTION__, __LINE__);
    m_setBuffersThread->run(PRIORITY_DEFAULT);
    m_setBuffersThread->join();

    if (m_isSuccessedBufferAllocation == false) {
        CLOGE("ERR(%s[%d]):m_setBuffersThread() failed", __FUNCTION__, __LINE__);
        err = INVALID_OPERATION;
    }

#ifdef START_PICTURE_THREAD
    m_startPictureInternalThread->join();
#endif
    m_startPictureBufferThread->join();

    if (m_parameters->isReprocessing() == true) {
#ifdef START_PICTURE_THREAD
        m_startPictureInternalThread->run(PRIORITY_DEFAULT);
#endif
    } else {
        m_pictureFrameFactory = m_previewFrameFactory;
        CLOGD("DEBUG(%s[%d]):FrameFactory(pictureFrameFactory) created", __FUNCTION__, __LINE__);

        /*
         * Make remained frameFactory here.
         * in case of SCC capture, make here.
         */
        m_framefactoryThread->run();
    }

    ret = m_startPreviewInternal();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_startPreviewInternal fail", __FUNCTION__, __LINE__);
        err = ret;
    }

#if defined(RAWDUMP_CAPTURE) || defined(DEBUG_RAWDUMP)
    m_mainSetupQThread[INDEX(PIPE_FLITE)]->run(PRIORITY_URGENT_DISPLAY);
    m_mainSetupQThread[INDEX(PIPE_3AA)]->run(PRIORITY_URGENT_DISPLAY);
#else
    /* setup frame thread */
    if (m_parameters->getDualMode() == true && getCameraId() == CAMERA_ID_FRONT) {
        CLOGD("DEBUG(%s[%d]):setupThread Thread start pipeId(%d)", __FUNCTION__, __LINE__, PIPE_FLITE);
        m_mainSetupQThread[INDEX(PIPE_FLITE)]->run(PRIORITY_URGENT_DISPLAY);
    } else {
        switch (m_parameters->getReprocessingBayerMode()) {
        case REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON:
            m_mainSetupQ[INDEX(PIPE_FLITE)]->setup(NULL);
            CLOGD("DEBUG(%s[%d]):setupThread Thread start pipeId(%d)", __FUNCTION__, __LINE__, PIPE_FLITE);
            m_mainSetupQThread[INDEX(PIPE_FLITE)]->run(PRIORITY_URGENT_DISPLAY);
            break;
        case REPROCESSING_BAYER_MODE_PURE_DYNAMIC:
            CLOGD("DEBUG(%s[%d]):setupThread with List pipeId(%d)", __FUNCTION__, __LINE__, PIPE_FLITE);
            m_mainSetupQ[INDEX(PIPE_FLITE)]->setup(m_mainSetupQThread[INDEX(PIPE_FLITE)]);
            break;
        default:
            CLOGI("INFO(%s[%d]):setupThread not started pipeID(%d)", __FUNCTION__, __LINE__, PIPE_FLITE);
            break;
        }
        CLOGD("DEBUG(%s[%d]):setupThread Thread start pipeId(%d)", __FUNCTION__, __LINE__, PIPE_3AA);
        m_mainSetupQThread[INDEX(PIPE_3AA)]->run(PRIORITY_URGENT_DISPLAY);
    }
#endif

    if (m_facedetectThread->isRunning() == false)
        m_facedetectThread->run();

    if (m_monitorThread->isRunning() == false)
        m_monitorThread->run(PRIORITY_DEFAULT);

    if (m_parameters->isMcscVraOtf() == false)
        m_previewFrameFactory->startThread(PIPE_VRA);

    m_previewThread->run(PRIORITY_DISPLAY);

    m_mainThread->run(PRIORITY_DEFAULT);
    m_startPictureInternalThread->join();

    return err;
}

status_t ExynosCamera::m_stopPictureInternal(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;

    m_prePictureThread->join();
    m_pictureThread->join();
    m_postPictureThread->join();

    m_jpegCallbackThread->join();

    for (int threadNum = JPEG_SAVE_THREAD0; threadNum < JPEG_SAVE_THREAD_MAX_COUNT; threadNum++) {
        m_jpegSaveThread[threadNum]->join();
    }

    if (m_zslPictureEnabled == true) {
        int numOfReprocessingFactory = m_parameters->getNumOfReprocessingFactory();

        for (int i = FRAME_FACTORY_TYPE_REPROCESSING; i < numOfReprocessingFactory + FRAME_FACTORY_TYPE_REPROCESSING; i++) {
            ret = m_frameFactory[i]->stopPipes();
            if (ret < 0) {
                CLOGE("ERR(%s):m_reprocessingFrameFactory0>stopPipe() fail", __FUNCTION__);
            }
        }
    }

    if (m_parameters->getHighResolutionCallbackMode() == true) {
        m_highResolutionCallbackThread->stop();
        if (m_highResolutionCallbackQ != NULL)
            m_highResolutionCallbackQ->sendCmd(WAKE_UP);
        m_highResolutionCallbackThread->requestExitAndWait();
    }

    /* Clear frames & buffers which remain in capture processingQ */
    m_clearFrameQ(dstSccReprocessingQ, PIPE_SCC, PIPE_SCC, DST_BUFFER_DIRECTION);
    m_clearFrameQ(m_postPictureQ, PIPE_SCC, PIPE_SCC, DST_BUFFER_DIRECTION);
    m_clearFrameQ(dstJpegReprocessingQ, PIPE_JPEG, PIPE_JPEG, SRC_BUFFER_DIRECTION);

    dstIspReprocessingQ->release();
    dstGscReprocessingQ->release();
    dstJpegReprocessingQ->release();

    m_jpegCallbackQ->release();

    for (int threadNum = JPEG_SAVE_THREAD0; threadNum < JPEG_SAVE_THREAD_MAX_COUNT; threadNum++) {
        m_jpegSaveQ[threadNum]->release();
    }

    if (m_highResolutionCallbackQ->getSizeOfProcessQ() != 0){
        CLOGD("DEBUG(%s[%d]):m_highResolutionCallbackQ->getSizeOfProcessQ(%d). release the highResolutionCallbackQ.",
            __FUNCTION__, __LINE__, m_highResolutionCallbackQ->getSizeOfProcessQ());
        m_highResolutionCallbackQ->release();
    }

    /*
     * HACK :
     * Just sleep for
     * all picture-related thread(having m_postProcessList) is over.
     *  if not :
     *    m_clearList will delete frames.
     *    and then, the internal mutex of other thead's deleted frame
     *    will sleep forever (pThread's tech report)
     *  to remove this hack :
     *    stopPreview()'s burstPanorama-related sequence.
     *    stop all pipe -> wait all thread. -> clear all frameQ.
     */
    usleep(5000);

    CLOGD("DEBUG(%s[%d]):clear postProcess(Picture) Frame list", __FUNCTION__, __LINE__);

    ret = m_clearList(&m_postProcessList);
    if (ret < 0) {
        CLOGE("ERR(%s):m_clearList fail", __FUNCTION__);
        return ret;
    }

    m_zslPictureEnabled = false;

    /* TODO: need timeout */
    return NO_ERROR;
}

status_t ExynosCamera::m_handlePreviewFrame(ExynosCameraFrame *frame)
{
    int ret = 0;
    ExynosCameraFrameEntity *entity = NULL;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraFrame *fdFrame = NULL;

    ExynosCameraBuffer buffer;
    ExynosCameraBuffer t3acBuffer;
    int pipeID = 0;
    /* to handle the high speed frame rate */
    bool skipPreview = false;
    int ratio = 1;
    uint32_t minFps = 0, maxFps = 0;
    uint32_t dispFps = EXYNOS_CAMERA_PREVIEW_FPS_REFERENCE;
    uint32_t fvalid = 0;
    uint32_t fcount = 0;
    uint32_t skipCount = 0;
    struct camera2_stream *shot_stream = NULL;
    ExynosCameraBuffer resultBuffer;
    camera2_node_group node_group_info_isp;
    int32_t reprocessingBayerMode = m_parameters->getReprocessingBayerMode();
    int ispDstBufferIndex = -1;

    entity = frame->getFrameDoneFirstEntity();
    if (entity == NULL) {
        CLOGE("ERR(%s[%d]):current entity is NULL", __FUNCTION__, __LINE__);
        /* TODO: doing exception handling */
        return true;
    }

    pipeID = entity->getPipeId();
#ifdef FPS_CHECK
    m_debugFpsCheck(entity->getPipeId());
#endif
    /* TODO: remove hard coding */
    switch(INDEX(entity->getPipeId())) {
    case PIPE_3AA_ISP:
        m_debugFpsCheck(entity->getPipeId());
        ret = frame->getSrcBuffer(entity->getPipeId(), &buffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getSrcBuffer fail, pipeId(%d), ret(%d)",
                __FUNCTION__, __LINE__, entity->getPipeId(), ret);
            return ret;
        }
        ret = m_putBuffers(m_3aaBufferMgr, buffer.index);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):put Buffer fail", __FUNCTION__, __LINE__);
        }

        CLOGV("DEBUG(%s[%d]):3AA_ISP frameCount(%d) frame.Count(%d)",
                __FUNCTION__, __LINE__,
                getMetaDmRequestFrameCount((struct camera2_shot_ext *)buffer.addr[1]),
                frame->getFrameCount());

        ret = frame->getDstBuffer(entity->getPipeId(), &buffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                __FUNCTION__, __LINE__, entity->getPipeId(), ret);
            return ret;
        }

        ret = m_putBuffers(m_ispBufferMgr, buffer.index);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):put Buffer fail", __FUNCTION__, __LINE__);
            break;
        }

        frame->setMetaDataEnable(true);

        /* Face detection */
        if(!m_parameters->getHighSpeedRecording()) {
            skipCount = m_parameters->getFrameSkipCount();
            if( m_parameters->msgTypeEnabled(CAMERA_MSG_PREVIEW_METADATA) &&
                skipCount <= 0 && m_flagStartFaceDetection == true) {
                fdFrame = m_frameMgr->createFrame(m_parameters, frame->getFrameCount());
                if (fdFrame != NULL) {
                    m_copyMetaFrameToFrame(frame, fdFrame, true, true);
                    m_facedetectQ->pushProcessQ(&fdFrame);
                }
            }
        }

        /* ISP capture mode q/dq for vdis */
        if (m_parameters->getTpuEnabledMode() == true) {
#if 0
            /* case 1 : directly push on isp, tpu. */
            ret = m_pushFrameToPipeIspDIS();
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_pushFrameToPipeIspDIS() fail", __FUNCTION__, __LINE__);
            }
#else
            /* case 2 : indirectly push on isp, tpu. */
            newFrame = m_frameMgr->createFrame(m_parameters, 0);
            m_mainSetupQ[INDEX(PIPE_ISP)]->pushProcessQ(&newFrame);
#endif
        }

        newFrame = m_frameMgr->createFrame(m_parameters, 0);
        m_mainSetupQ[INDEX(entity->getPipeId())]->pushProcessQ(&newFrame);
        break;
    case PIPE_3AC:
    case PIPE_FLITE:
        m_debugFpsCheck(entity->getPipeId());

        if (m_parameters->getHighSpeedRecording()) {
            ret = frame->getDstBuffer(entity->getPipeId(), &buffer);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, entity->getPipeId(), ret);
                return ret;
            }
            ret = m_putBuffers(m_bayerBufferMgr, buffer.index);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):put Buffer fail", __FUNCTION__, __LINE__);
                break;
            }
        } else {
            if (frame->getSccDrop() == true || frame->getIspcDrop() == true) {
                CLOGE("ERR(%s[%d]):getSccDrop() == %d || getIspcDrop()== %d. so drop this frame(frameCount : %d)",
                    __FUNCTION__, __LINE__, frame->getSccDrop(), frame->getIspcDrop(), frame->getFrameCount());

                ret = frame->getDstBuffer(entity->getPipeId(), &buffer);
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                        __FUNCTION__, __LINE__, entity->getPipeId(), ret);
                    return ret;
                }

                ret = m_putBuffers(m_bayerBufferMgr, buffer.index);
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):m_putBuffers(%d) fail", __FUNCTION__, __LINE__, buffer.index);
                    break;
                }
            } else {
                ret = m_captureSelector->manageFrameHoldList(frame, entity->getPipeId(), false);
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):manageFrameHoldList fail", __FUNCTION__, __LINE__);
                    return ret;
                }
#ifdef DEBUG_RAWDUMP
        newFrame = m_frameMgr->createFrame(m_parameters, 0);
        m_mainSetupQ[INDEX(entity->getPipeId())]->pushProcessQ(&newFrame);
#endif
            }
        }

        /* TODO: Dynamic bayer capture, currently support only single shot */
        if (reprocessingBayerMode == REPROCESSING_BAYER_MODE_PURE_DYNAMIC
            || reprocessingBayerMode == REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC)
            m_previewFrameFactory->stopThread(entity->getPipeId());

        if (reprocessingBayerMode == REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON
            || reprocessingBayerMode == REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON) {
            newFrame = m_frameMgr->createFrame(m_parameters, 0);
            m_mainSetupQ[INDEX(entity->getPipeId())]->pushProcessQ(&newFrame);
        }
        break;
    case PIPE_ISP:
        /*
        if (entity->getSrcBufState() == ENTITY_BUFFER_STATE_ERROR)
            m_previewFrameFactory->dump();
        */
        /* 3AP buffer handling */
        ret = frame->getSrcBuffer(entity->getPipeId(), &buffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getSrcBuffer fail, pipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, entity->getPipeId(), ret);
            return ret;
        }

        if (buffer.index >= 0) {
            ret = m_putBuffers(m_ispBufferMgr, buffer.index);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):put Buffer fail", __FUNCTION__, __LINE__);
                break;
            }
        }

        /* Face detection */
        if (!m_parameters->getHighSpeedRecording()
            && frame->getFrameState() != FRAME_STATE_SKIPPED) {
            skipCount = m_parameters->getFrameSkipCount();
            if( m_parameters->msgTypeEnabled(CAMERA_MSG_PREVIEW_METADATA) &&
                skipCount <= 0 && m_flagStartFaceDetection == true) {
                ret = m_doFdCallbackFunc(frame);
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):m_doFdCallbackFunc fail, ret(%d)",
                        __FUNCTION__, __LINE__, ret);
                    return ret;
                }
            }
        }

        /* ISP capture mode q/dq for vdis */
        if (m_parameters->getTpuEnabledMode() == true) {
            break;
        }
    case PIPE_3AA:
    case PIPE_DIS:
       /* The following switch allows both  PIPE_3AA and PIPE_ISP to
        * fall through to PIPE_SCP
        */

       switch(INDEX(entity->getPipeId())) {
            case PIPE_3AA:
                m_debugFpsCheck(entity->getPipeId());

                /*
                if (entity->getSrcBufState() == ENTITY_BUFFER_STATE_ERROR)
                    m_previewFrameFactory->dump();
                */

                ret = frame->getSrcBuffer(entity->getPipeId(), &buffer);
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):getSrcBuffer fail, pipeId(%d), ret(%d)",
                            __FUNCTION__, __LINE__, entity->getPipeId(), ret);
                    return ret;
                }

                if (buffer.index >= 0) {
                    ret = m_putBuffers(m_3aaBufferMgr, buffer.index);
                    if (ret < 0) {
                        CLOGE("ERR(%s[%d]):put Buffer fail", __FUNCTION__, __LINE__);
                    }
                }

                frame->setMetaDataEnable(true);

                newFrame = m_frameMgr->createFrame(m_parameters, 0);
                m_mainSetupQ[INDEX(entity->getPipeId())]->pushProcessQ(&newFrame);
                /* 3AC buffer handling */
                    t3acBuffer.index = -1;

                    if (frame->getRequest(PIPE_3AC) == true) {
                        ret = frame->getDstBuffer(entity->getPipeId(), &t3acBuffer, m_previewFrameFactory->getNodeType(PIPE_3AC));
                        if (ret != NO_ERROR) {
                            CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                                    __FUNCTION__, __LINE__, entity->getPipeId(), ret);
                        }
                    }

                    if (0 <= t3acBuffer.index) {
                        if (frame->getRequest(PIPE_3AC) == true) {
                            if (m_parameters->getHighSpeedRecording() == true) {
                                if (m_parameters->isUsing3acForIspc() == true)
                                    ret = m_putBuffers(m_sccBufferMgr, t3acBuffer.index);
                                else
                                    ret = m_putBuffers(m_bayerBufferMgr, t3acBuffer.index);
                                if (ret < 0) {
                                    CLOGE("ERR(%s[%d]):m_putBuffers(m_bayerBufferMgr, %d) fail", __FUNCTION__, __LINE__, t3acBuffer.index);
                                    break;
                                }
                            } else {
                                entity_buffer_state_t bufferstate = ENTITY_BUFFER_STATE_NOREQ;
                                ret = frame->getDstBufferState(entity->getPipeId(), &bufferstate, m_previewFrameFactory->getNodeType(PIPE_3AC));
                                if (ret == NO_ERROR && bufferstate != ENTITY_BUFFER_STATE_ERROR) {
                                    if (m_parameters->isUseYuvReprocessing() == false
                                        && m_parameters->isUsing3acForIspc() == true)
                                        ret = m_sccCaptureSelector->manageFrameHoldList(frame, entity->getPipeId(), false, m_previewFrameFactory->getNodeType(PIPE_3AC));
                                    else
                                        ret = m_captureSelector->manageFrameHoldList(frame, entity->getPipeId(), false, m_previewFrameFactory->getNodeType(PIPE_3AC));
                                    if (ret < 0) {
                                        CLOGE("ERR(%s[%d]):manageFrameHoldList fail", __FUNCTION__, __LINE__);
                                        return ret;
                                    }
                                } else {
                                    if (m_parameters->isUsing3acForIspc() == true)
                                        ret = m_putBuffers(m_sccBufferMgr, t3acBuffer.index);
                                    else
                                        ret = m_putBuffers(m_bayerBufferMgr, t3acBuffer.index);
                                    if (ret < 0) {
                                        CLOGE("ERR(%s[%d]):m_putBuffers(m_bayerBufferMgr, %d) fail", __FUNCTION__, __LINE__, t3acBuffer.index);
                                        break;
                                    }
                                }
                            }
                        } else {
                            if (reprocessingBayerMode == REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON) {
                                CLOGW("WARN(%s[%d]):frame->getRequest(PIPE_3AC) == false. so, just m_putBuffers(t3acBuffer.index(%d)..., pipeId(%d), ret(%d)",
                                    __FUNCTION__, __LINE__, t3acBuffer.index, entity->getPipeId(), ret);
                            }

                            if (m_parameters->isUsing3acForIspc() == true)
                                ret = m_putBuffers(m_sccBufferMgr, t3acBuffer.index);
                            else
                                ret = m_putBuffers(m_bayerBufferMgr, t3acBuffer.index);
                            if (ret < 0) {
                                CLOGE("ERR(%s[%d]):m_putBuffers(m_bayerBufferMgr, %d) fail", __FUNCTION__, __LINE__, t3acBuffer.index);
                                break;
                            }
                        }
                    }

                /* Face detection for using 3AA-ISP OTF mode. */
                if (m_parameters->is3aaIspOtf() == true
                    && m_parameters->isMcscVraOtf() == true) {
                    skipCount = m_parameters->getFrameSkipCount();
                    if (skipCount <= 0) {
                        /* Face detection */
                        struct camera2_shot_ext shot;
                        camera2_node_group node_group_info;
                        frame->getDynamicMeta(&shot);
                        fdFrame = m_frameMgr->createFrame(m_parameters, frame->getFrameCount());
                        if (fdFrame != NULL) {
                            fdFrame->storeDynamicMeta(&shot);
                            m_facedetectQ->pushProcessQ(&fdFrame);

                            frame->getNodeGroupInfo(&node_group_info, PERFRAME_INFO_3AA);
                            fdFrame->storeNodeGroupInfo(&node_group_info, PERFRAME_INFO_3AA);
                        }

                    }
                }

                CLOGV("DEBUG(%s[%d]):3AA_ISP frameCount(%d) frame.Count(%d)",
                        __FUNCTION__, __LINE__,
                        getMetaDmRequestFrameCount((struct camera2_shot_ext *)buffer.addr[1]),
                        frame->getFrameCount());

                break;
            case PIPE_DIS:
                m_debugFpsCheck(pipeID);
                if (m_parameters->getTpuEnabledMode() == true) {
                    ret = frame->getSrcBuffer(PIPE_DIS, &buffer);
                    if (ret < 0) {
                        CLOGE("ERR(%s[%d]):getSrcBuffer fail, pipeId(%d), ret(%d)",
                            __FUNCTION__, __LINE__, PIPE_ISP, ret);
                        return ret;
                    }

                    if (buffer.index >= 0) {
                        ret = m_putBuffers(m_hwDisBufferMgr, buffer.index);
                        if (ret < 0) {
                            CLOGE("ERR(%s[%d]):m_putBuffers(m_hwDisBufferMgr, %d) fail", __FUNCTION__, __LINE__, buffer.index);
                        }
                    }
                }

                CLOGV("DEBUG(%s[%d]):DIS done HAL-frameCount(%d)", __FUNCTION__, __LINE__, frame->getFrameCount());

                break;
            default:
                    CLOGE("ERR(%s[%d]):Its impossible to be here. ",
                            __FUNCTION__, __LINE__);
                break;
        }

        if ((INDEX(entity->getPipeId())) == PIPE_3AA &&
            m_parameters->is3aaIspOtf() == true &&
            m_parameters->getTpuEnabledMode() == false) {
            /* Fall through to PIPE_SCP */
        } else if ((INDEX(entity->getPipeId())) == PIPE_DIS) {
            /* Fall through to PIPE_SCP */
        } else {
            /* Break out of the outer switch and reach entity_state_complete: */
            break;
        }

    case PIPE_SCP:
        if (entity->getDstBufState() == ENTITY_BUFFER_STATE_ERROR) {
            // libcamera: 75xx: Change SCP cancelbuffer condition to SCP request true. // Siyoung Hur <siyoung.hur@samsung.com>
            if (frame->getRequest(PIPE_SCP) == true) {
                ret = frame->getDstBuffer(entity->getPipeId(), &buffer, m_previewFrameFactory->getNodeType(PIPE_SCP));
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                            __FUNCTION__, __LINE__, entity->getPipeId(), ret);
                    return ret;
                }

                if (buffer.index >= 0) {
#ifdef USE_GRALLOC_REUSE_SUPPORT
                    ret = m_scpBufferMgr->cancelBuffer(buffer.index, true);
#else
                    ret = m_scpBufferMgr->cancelBuffer(buffer.index);
#endif
                    if (ret < 0)
                        CLOGE("ERR(%s[%d]):SCP buffer return fail", __FUNCTION__, __LINE__);
                }
                /* For debug */
                /* m_previewFrameFactory->dump(); */

                /* Comment out, because it included ISP */
                /*
                   newFrame = m_previewFrameFactory->createNewFrameOnlyOnePipe(PIPE_SCP, frame->getFrameCount());
                   newFrame->setDstBuffer(PIPE_SCP, buffer);
                   newFrame->setFrameState(FRAME_STATE_SKIPPED);

                   m_mainSetupQ[INDEX(entity->getPipeId())]->pushProcessQ(&newFrame);
                 */
            }
            CLOGV("DEBUG(%s[%d]):SCP done HAL-frameCount(%d)", __FUNCTION__, __LINE__, frame->getFrameCount());
        } else if (entity->getDstBufState() == ENTITY_BUFFER_STATE_COMPLETE) {
            m_debugFpsCheck(entity->getPipeId());

            ret = m_doZoomPrviewWithCSC(entity->getPipeId(), PIPE_GSC, frame);
            if (ret < 0) {
                CLOGW("WARN(%s[%d]):m_doPrviewWithCSC failed", __FUNCTION__, __LINE__);
            }

            ret = frame->getDstBuffer(entity->getPipeId(), &buffer, m_previewFrameFactory->getNodeType(PIPE_SCP));
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, entity->getPipeId(), ret);
                return ret;
            }

            /* TO DO : skip frame for HDR */
            shot_stream = (struct camera2_stream *)buffer.addr[2];

            if (shot_stream != NULL) {
                getStreamFrameValid(shot_stream, &fvalid);
                getStreamFrameCount(shot_stream, &fcount);
            } else {
                CLOGE("ERR(%s[%d]):shot_stream is NULL", __FUNCTION__, __LINE__);
                fvalid = false;
                fcount = 0;
            }

            /* drop preview frame if lcd supported frame rate < scp frame rate */
            frame->getFpsRange(&minFps, &maxFps);
            if (dispFps < maxFps) {
                ratio = (int)((maxFps * 10 / dispFps) / 10);
                m_displayPreviewToggle = (m_displayPreviewToggle + 1) % ratio;
                skipPreview = (m_displayPreviewToggle == 0) ? true : false;
#ifdef DEBUG
                CLOGE("DEBUG(%s[%d]):preview frame skip! frameCount(%d) (m_displayPreviewToggle=%d, maxFps=%d, dispFps=%d, ratio=%d, skipPreview=%d)",
                        __FUNCTION__, __LINE__, frame->getFrameCount(), m_displayPreviewToggle, maxFps, dispFps, ratio, (int)skipPreview);
#endif
            }

            newFrame = m_previewFrameFactory->createNewFrameOnlyOnePipe(PIPE_SCP, frame->getFrameCount());
            if (newFrame == NULL) {
#ifdef USE_GRALLOC_REUSE_SUPPORT
               ret = m_scpBufferMgr->cancelBuffer(buffer.index, true);
#else
               ret = m_scpBufferMgr->cancelBuffer(buffer.index);
#endif
               if (ret < 0)
                   CLOGE("ERR(%s[%d]):SCP buffer return fail", __FUNCTION__, __LINE__);

               goto entity_state_complete;
            }

            newFrame->setDstBuffer(PIPE_SCP, buffer);

            m_parameters->getFrameSkipCount(&m_frameSkipCount);
            if (m_frameSkipCount > 0) {
                CLOGD("INFO(%s[%d]):Skip frame for frameSkipCount(%d) buffer.index(%d)",
                        __FUNCTION__, __LINE__, m_frameSkipCount, buffer.index);
                newFrame->setFrameState(FRAME_STATE_SKIPPED);
                if (buffer.index >= 0) {
#ifdef USE_GRALLOC_REUSE_SUPPORT
                    ret = m_scpBufferMgr->cancelBuffer(buffer.index, true);
#else
                    ret = m_scpBufferMgr->cancelBuffer(buffer.index);
#endif
                    if (ret < 0)
                        CLOGE("ERR(%s[%d]):SCP buffer return fail", __FUNCTION__, __LINE__);
                }
            } else {
                if (m_skipReprocessing == true)
                    m_skipReprocessing = false;
                nsecs_t timeStamp = (nsecs_t)frame->getTimeStamp();
                if (m_getRecordingEnabled() == true
                    && m_parameters->msgTypeEnabled(CAMERA_MSG_VIDEO_FRAME)) {
                    if (timeStamp <= 0L) {
                        CLOGE("WARN(%s[%d]):timeStamp(%lld) Skip", __FUNCTION__, __LINE__, timeStamp);
                    } else {
                        if (m_parameters->doCscRecording() == true) {
                            /* get Recording Image buffer */
                            int bufIndex = -2;
                            ExynosCameraBuffer recordingBuffer;
                            ret = m_recordingBufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &recordingBuffer);
                            if (ret < 0 || bufIndex < 0) {
                                if ((++m_recordingFrameSkipCount % 100) == 0) {
                                    CLOGE("ERR(%s[%d]): Recording buffer is not available!! Recording Frames are Skipping(%d frames) (bufIndex=%d)",
                                            __FUNCTION__, __LINE__, m_recordingFrameSkipCount, bufIndex);
                                    m_recordingBufferMgr->printBufferQState();
                                }
                            } else {
                                if (m_recordingFrameSkipCount != 0) {
                                    CLOGE("ERR(%s[%d]): Recording buffer is not available!! Recording Frames are Skipped(%d frames) (bufIndex=%d) (recordingQ=%d)",
                                            __FUNCTION__, __LINE__, m_recordingFrameSkipCount, bufIndex, m_recordingQ->getSizeOfProcessQ());
                                    m_recordingFrameSkipCount = 0;
                                    m_recordingBufferMgr->printBufferQState();
                                }
                                m_recordingTimeStamp[bufIndex] = timeStamp;

                                ret = m_doPrviewToRecordingFunc(PIPE_GSC_VIDEO, buffer, recordingBuffer, timeStamp);
                                if (ret < 0) {
                                    CLOGW("WARN(%s[%d]):recordingCallback Skip", __FUNCTION__, __LINE__);
                                }
                            }
                        } else {
                            m_recordingTimeStamp[buffer.index] = timeStamp;

                            if (m_recordingStartTimeStamp == 0) {
                                m_recordingStartTimeStamp = timeStamp;
                                CLOGI("INFO(%s[%d]):m_recordingStartTimeStamp=%lld",
                                        __FUNCTION__, __LINE__, m_recordingStartTimeStamp);
                            }

                            if ((0L < timeStamp)
                                && (m_lastRecordingTimeStamp < timeStamp)
                                && (m_recordingStartTimeStamp <= timeStamp)) {
                                if (m_getRecordingEnabled() == true
                                        && m_parameters->msgTypeEnabled(CAMERA_MSG_VIDEO_FRAME)) {
#ifdef CHECK_MONOTONIC_TIMESTAMP
                                    CLOGD("DEBUG(%s[%d]):m_dataCbTimestamp::recordingFrameIndex=%d, recordingTimeStamp=%lld, fd[0]=%d",
                                            __FUNCTION__, __LINE__, buffer.index, timeStamp, buffer.fd[0]);
#endif
#ifdef DEBUG
                                    CLOGD("DEBUG(%s[%d]): - lastTimeStamp(%lld), systemTime(%lld), recordingStart(%lld)",
                                            __FUNCTION__, __LINE__,
                                            m_lastRecordingTimeStamp,
                                            systemTime(SYSTEM_TIME_MONOTONIC),
                                            m_recordingStartTimeStamp);
#endif

                                    if (m_recordingBufAvailable[buffer.index] == false) {
                                        CLOGW("WARN(%s[%d]):recordingFrameIndex(%d) didn't release yet !!! drop the frame !!! "
                                                " timeStamp(%lld) m_recordingBufAvailable(%d)",
                                                __FUNCTION__, __LINE__, buffer.index, timeStamp, (int)m_recordingBufAvailable[buffer.index]);
                                    } else {
                                        struct addrs *recordAddrs = NULL;

                                        recordAddrs = (struct addrs *)m_recordingCallbackHeap->data;
                                        recordAddrs[buffer.index].type        = kMetadataBufferTypeCameraSource;
                                        recordAddrs[buffer.index].fdPlaneY    = (unsigned int)buffer.fd[0];
                                        recordAddrs[buffer.index].fdPlaneCbcr = (unsigned int)buffer.fd[1];
                                        recordAddrs[buffer.index].bufIndex    = buffer.index;

                                        m_recordingBufAvailable[buffer.index] = false;
                                        m_lastRecordingTimeStamp = timeStamp;

                                        if (m_getRecordingEnabled() == true
                                            && m_parameters->msgTypeEnabled(CAMERA_MSG_VIDEO_FRAME)) {
                                        m_dataCbTimestamp(
                                                timeStamp,
                                                CAMERA_MSG_VIDEO_FRAME,
                                                m_recordingCallbackHeap,
                                                buffer.index,
                                                m_callbackCookie);
                                        }
                                    }
                                }
                            } else {
                                CLOGW("WARN(%s[%d]):recordingFrameIndex=%d, timeStamp(%lld) invalid -"
                                    " lastTimeStamp(%lld), systemTime(%lld), recordingStart(%lld), m_recordingBufAvailable(%d)",
                                    __FUNCTION__, __LINE__, buffer.index, timeStamp,
                                    m_lastRecordingTimeStamp,
                                    systemTime(SYSTEM_TIME_MONOTONIC),
                                    m_recordingStartTimeStamp,
                                    (int)m_recordingBufAvailable[buffer.index]);
                                m_recordingTimeStamp[buffer.index] = 0L;
                            }
                        }
                    }
                }

                ExynosCameraBuffer callbackBuffer;
                ExynosCameraFrame *callbackFrame = NULL;
                struct camera2_shot_ext *shot_ext = new struct camera2_shot_ext;

                callbackFrame = m_previewFrameFactory->createNewFrameOnlyOnePipe(PIPE_SCP, frame->getFrameCount());
                if (callbackFrame == NULL) {
#ifdef USE_GRALLOC_REUSE_SUPPORT
                    ret = m_scpBufferMgr->cancelBuffer(buffer.index, true);
#else
                    ret = m_scpBufferMgr->cancelBuffer(buffer.index);
#endif
                    if (ret < 0)
                        CLOGE("ERR(%s[%d]):SCP buffer return fail", __FUNCTION__, __LINE__);

                    if (newFrame != NULL) {
                        newFrame->decRef();
                        m_frameMgr->deleteFrame(newFrame);
                    }
                    goto entity_state_complete;
                }

                ret = frame->getDstBuffer(entity->getPipeId(), &callbackBuffer, m_previewFrameFactory->getNodeType(PIPE_SCP));
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):getDstBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                }
                frame->getMetaData(shot_ext);
                callbackFrame->storeDynamicMeta(shot_ext);
                callbackFrame->setDstBuffer(PIPE_SCP, callbackBuffer);

                if (((m_parameters->getPreviewBufferCount() == NUM_PREVIEW_BUFFERS + NUM_PREVIEW_SPARE_BUFFERS &&
                    m_previewQ->getSizeOfProcessQ() >= 2) ||
                    (m_parameters->getPreviewBufferCount() == NUM_PREVIEW_BUFFERS &&
                    m_previewQ->getSizeOfProcessQ() >= 1)) &&
                    (m_previewThread->isRunning() == true)) {

                    if ((m_getRecordingEnabled() == true) && (m_parameters->doCscRecording() == false)) {
                        CLOGW("WARN(%s[%d]):push frame to previewQ. PreviewQ(%d), PreviewBufferCount(%d)",
                                __FUNCTION__,
                                __LINE__,
                                 m_previewQ->getSizeOfProcessQ(),
                                 m_parameters->getPreviewBufferCount());
                        m_previewQ->pushProcessQ(&callbackFrame);
                    } else {
                        CLOGW("WARN(%s[%d]):Frames are stacked in previewQ. Skip frame. PreviewQ(%d), PreviewBufferCount(%d)",
                            __FUNCTION__, __LINE__,
                            m_previewQ->getSizeOfProcessQ(),
                            m_parameters->getPreviewBufferCount());
                        newFrame->setFrameState(FRAME_STATE_SKIPPED);
                        if (buffer.index >= 0) {
                            /* only apply in the Full OTF of Exynos75xx. */
#ifdef USE_GRALLOC_REUSE_SUPPORT
                            ret = m_scpBufferMgr->cancelBuffer(buffer.index, true);
#else
                            ret = m_scpBufferMgr->cancelBuffer(buffer.index);
#endif
                            if (ret < 0)
                                CLOGE("ERR(%s[%d]):SCP buffer return fail", __FUNCTION__, __LINE__);
                        }

                        callbackFrame->decRef();
                        m_frameMgr->deleteFrame(callbackFrame);
                        callbackFrame = NULL;
                    }

                } else if((m_parameters->getFastFpsMode() > 1) && (m_parameters->getRecordingHint() == 1)) {
                    m_skipCount++;
                    CLOGV("INFO(%s[%d]):push frame to previewQ", __FUNCTION__, __LINE__);
                    m_previewQ->pushProcessQ(&callbackFrame);
                } else {
                    if (m_getRecordingEnabled() == true) {
                        CLOGV("INFO(%s[%d]):push frame to previewQ", __FUNCTION__, __LINE__);
                        m_previewQ->pushProcessQ(&callbackFrame);
                    } else {
                        CLOGV("INFO(%s[%d]):push frame to previewQ", __FUNCTION__, __LINE__);
                        m_previewQ->pushProcessQ(&callbackFrame);
                    }
                }
                delete shot_ext;
                shot_ext = NULL;
            }
            newFrame->decRef();
            m_frameMgr->deleteFrame(newFrame);
            //m_mainSetupQ[INDEX(entity->getPipeId())]->pushProcessQ(&newFrame);
            CLOGV("DEBUG(%s[%d]):SCP done HAL-frameCount(%d)", __FUNCTION__, __LINE__, frame->getFrameCount());
        } else {
            CLOGV("DEBUG(%s[%d]):SCP droped - SCP buffer is not ready HAL-frameCount(%d)", __FUNCTION__, __LINE__, frame->getFrameCount());

            ret = frame->getDstBuffer(entity->getPipeId(), &buffer, m_previewFrameFactory->getNodeType(PIPE_SCP));
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, entity->getPipeId(), ret);
                return ret;
            }

            if (buffer.index >= 0) {
#ifdef USE_GRALLOC_REUSE_SUPPORT
                ret = m_scpBufferMgr->cancelBuffer(buffer.index, true);
#else
                ret = m_scpBufferMgr->cancelBuffer(buffer.index);
#endif
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):SCP buffer return fail", __FUNCTION__, __LINE__);
                }
            }

            /* For debug */
            /* m_previewFrameFactory->dump(); */

            /* Comment out, because it included ISP */
            /*
            newFrame = m_previewFrameFactory->createNewFrameOnlyOnePipe(PIPE_SCP, frame->getFrameCount());
            newFrame->setDstBuffer(PIPE_SCP, buffer);
            newFrame->setFrameState(FRAME_STATE_SKIPPED);

            m_mainSetupQ[INDEX(entity->getPipeId())]->pushProcessQ(&newFrame);
            */
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
            if (entity->getDstBufState() != ENTITY_BUFFER_STATE_ERROR) {
                /* Face detection callback */
                struct camera2_shot_ext fd_shot;
                frame->getDynamicMeta(&fd_shot);

                ExynosCameraFrame *fdFrame = m_frameMgr->createFrame(m_parameters, frame->getFrameCount());
                if (fdFrame != NULL) {
                    fdFrame->storeDynamicMeta(&fd_shot);
                    m_facedetectQ->pushProcessQ(&fdFrame);
                }
            }

            ret = m_vraBufferMgr->putBuffer(buffer.index, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL);
            if (ret != NO_ERROR)
                CLOGW("WARN(%s[%d]):Put VRA buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        break;
    default:
        break;
    }

entity_state_complete:

    ret = frame->setEntityState(entity->getPipeId(), ENTITY_STATE_COMPLETE);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setEntityState fail, pipeId(%d), state(%d), ret(%d)",
            __FUNCTION__, __LINE__, entity->getPipeId(), ENTITY_STATE_COMPLETE, ret);
        return ret;
    }

    if (frame->isComplete() == true) {
        ret = m_removeFrameFromList(&m_processList, frame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):remove frame from processList fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        frame->decRef();
        m_frameMgr->deleteFrame(frame);
    }


    return NO_ERROR;
}

status_t ExynosCamera::m_handlePreviewFrameFrontDual(ExynosCameraFrame *frame)
{
    int ret = 0;
    ExynosCameraFrameEntity *entity = NULL;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraFrame *fdFrame = NULL;

    ExynosCameraBuffer buffer;
    ExynosCameraBuffer t3acBuffer;
    int pipeID = 0;
    /* to handle the high speed frame rate */
    bool skipPreview = false;
    int ratio = 1;
    uint32_t minFps = 0, maxFps = 0;
    uint32_t dispFps = EXYNOS_CAMERA_PREVIEW_FPS_REFERENCE;
    uint32_t fvalid = 0;
    uint32_t fcount = 0;
    uint32_t skipCount = 0;
    struct camera2_stream *shot_stream = NULL;
    ExynosCameraBuffer resultBuffer;
    camera2_node_group node_group_info_isp;
    int32_t reprocessingBayerMode = m_parameters->getReprocessingBayerMode();
    int ispDstBufferIndex = -1;

    entity = frame->getFrameDoneFirstEntity();
    if (entity == NULL) {
        CLOGE("ERR(%s[%d]):current entity is NULL frame(%d)", __FUNCTION__, __LINE__, frame->getFrameCount());
        /* TODO: doing exception handling */
        return true;
    }

    pipeID = entity->getPipeId();

    /* TODO: remove hard coding */
    switch(INDEX(entity->getPipeId())) {
    case PIPE_3AA_ISP:
        break;
    case PIPE_3AC:
    case PIPE_FLITE:
        m_debugFpsCheck(entity->getPipeId());

        ret = frame->getDstBuffer(entity->getPipeId(), &buffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                __FUNCTION__, __LINE__, entity->getPipeId(), ret);
            return ret;
        }

        m_setupEntity(PIPE_3AA, frame, &buffer, NULL);
        m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, PIPE_3AA);
        m_previewFrameFactory->pushFrameToPipe(&frame, PIPE_3AA);

        if (m_parameters->isUsing3acForIspc() == true) {
            newFrame = m_frameMgr->createFrame(m_parameters, 0);
            m_mainSetupQ[INDEX(entity->getPipeId())]->pushProcessQ(&newFrame);
        }
        break;
    case PIPE_3AA:
        m_debugFpsCheck(entity->getPipeId());

        ret = frame->getSrcBuffer(entity->getPipeId(), &buffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getSrcBuffer fail, pipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, entity->getPipeId(), ret);
            return ret;
        }

        if (m_parameters->isUsing3acForIspc() == true) {
            ret = m_sccCaptureSelector->manageFrameHoldList(frame, entity->getPipeId(), false, CAPTURE_NODE);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):manageFrameHoldList fail", __FUNCTION__, __LINE__);
                return ret;
            }
        }

        if (buffer.index >= 0) {
            ret = m_putBuffers(m_bayerBufferMgr, buffer.index);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):put Buffer fail", __FUNCTION__, __LINE__);
            }
        }

        if (m_parameters->isUsing3acForIspc() == false ) {
            newFrame = m_frameMgr->createFrame(m_parameters, 0);
            m_mainSetupQ[INDEX(m_getBayerPipeId())]->pushProcessQ(&newFrame);
        }

        CLOGV("DEBUG(%s[%d]):3AA_ISP frameCount(%d) frame.Count(%d)",
                __FUNCTION__, __LINE__,
                getMetaDmRequestFrameCount((struct camera2_shot_ext *)buffer.addr[1]),
                frame->getFrameCount());
        break;
    default:
        break;
    }

entity_state_complete:

    ret = frame->setEntityState(entity->getPipeId(), ENTITY_STATE_COMPLETE);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setEntityState fail, pipeId(%d), state(%d), ret(%d)",
            __FUNCTION__, __LINE__, entity->getPipeId(), ENTITY_STATE_COMPLETE, ret);
        return ret;
    }

    if (frame->isComplete() == true) {
        ret = m_removeFrameFromList(&m_processList, frame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):remove frame from processList fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        frame->decRef();
        m_frameMgr->deleteFrame(frame);
    }

    return NO_ERROR;
}

bool ExynosCamera::m_previewThreadFunc(void)
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif

    int  ret  = 0;
    bool loop = true;
    int  pipeId    = 0;
    int  pipeIdCsc = 0;
    int  maxbuffers   = 0;
#ifdef USE_PREVIEW_DURATION_CONTROL
    uint32_t offset = 1000; /* 1ms */
    uint32_t curMinFps = 0;
    uint32_t curMaxFps = 0;
    uint64_t frameDurationUs;
#endif
    ExynosCameraBuffer buffer;
    ExynosCameraFrame  *frame = NULL;
    nsecs_t timeStamp = 0;
    int frameCount = -1;
    frame_queue_t *previewQ;

#ifdef USE_PREVIEW_DURATION_CONTROL
    m_parameters->getPreviewFpsRange(&curMinFps, &curMaxFps);

    /* Check the Slow/Fast Motion Scenario - sensor : 120fps, preview : 60fps */
    if(((curMinFps == 120) && (curMaxFps == 120))
         || (curMinFps == 240) && (curMaxFps == 240)) {
        CLOGV("(%s[%d]) : Change PreviewDuration from (%d,%d) to (60000, 60000)", __FUNCTION__, __LINE__, curMinFps, curMaxFps);
        curMinFps = 60;
        curMaxFps = 60;
    }

    frameDurationUs = 1000000/curMaxFps;
    if (frameDurationUs > offset) {
        frameDurationUs = frameDurationUs - offset; /* add the offset value for timing issue */
    }
    PreviewDurationTimer.start();
#endif

    pipeId    = PIPE_SCP;
    pipeIdCsc = PIPE_GSC;

    previewQ = m_previewQ;

    CLOGV("INFO(%s[%d]):wait previewQ", __FUNCTION__, __LINE__);
    ret = previewQ->waitAndPopProcessQ(&frame);
    if (m_flagThreadStop == true) {
        CLOGI("INFO(%s[%d]):m_flagThreadStop(%d)", __FUNCTION__, __LINE__, m_flagThreadStop);

        if (frame != NULL) {
            frame->decRef();
            m_frameMgr->deleteFrame(frame);
            frame = NULL;
        }

        return false;
    }
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (frame == NULL) {
        CLOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    CLOGV("INFO(%s[%d]):get frame from previewQ", __FUNCTION__, __LINE__);
    timeStamp = (nsecs_t)frame->getTimeStamp();
    frameCount = frame->getFrameCount();
    ret = frame->getDstBuffer(pipeId, &buffer);

    if (ret < 0) {
        CLOGE("ERR(%s[%d]):getDstBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        goto func_exit;
    }

    /* ------------- frome here "frame" cannot use ------------- */
    CLOGV("INFO(%s[%d]):push frame to previewReturnQ", __FUNCTION__, __LINE__);
    if(m_parameters->increaseMaxBufferOfPreview()) {
        maxbuffers = m_parameters->getPreviewBufferCount();
    } else {
        maxbuffers = (int)m_exynosconfig->current->bufInfo.num_preview_buffers;
    }

    if (buffer.index < 0 || buffer.index >= maxbuffers ) {
        CLOGE("ERR(%s[%d]):Out of Index! (Max: %d, Index: %d)", __FUNCTION__, __LINE__, maxbuffers, buffer.index);
        goto func_exit;
    }

    CLOGV("INFO(%s[%d]):m_previewQ->getSizeOfProcessQ(%d) m_scpBufferMgr->getNumOfAvailableBuffer(%d)", __FUNCTION__, __LINE__,
        previewQ->getSizeOfProcessQ(), m_scpBufferMgr->getNumOfAvailableBuffer());

    /* Prevent displaying unprocessed beauty images in beauty shot. */
    if ((m_parameters->getShotMode() == SHOT_MODE_BEAUTY_FACE)) {
        if (m_parameters->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME) &&
            checkBit(&m_callbackState, CALLBACK_STATE_COMPRESSED_IMAGE)) {
            CLOGV("INFO(%s[%d]):skip the preview callback and the preview display while compressed callback.",
                    __FUNCTION__, __LINE__);
            ret = m_scpBufferMgr->cancelBuffer(buffer.index);
            goto func_exit;
        }
    }

    if ((m_previewWindow == NULL) ||
        (m_getRecordingEnabled() == true) ||
        (m_parameters->getPreviewBufferCount() == NUM_PREVIEW_BUFFERS + NUM_PREVIEW_SPARE_BUFFERS &&
        m_scpBufferMgr->getNumOfAvailableAndNoneBuffer() > 4 &&
        previewQ->getSizeOfProcessQ() < 2) ||
        (m_parameters->getPreviewBufferCount() == NUM_PREVIEW_BUFFERS &&
        m_scpBufferMgr->getNumOfAvailableAndNoneBuffer() > 2 &&
        previewQ->getSizeOfProcessQ() < 1)) {
        if (m_parameters->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME) &&
            m_highResolutionCallbackRunning == false) {
            ExynosCameraBuffer previewCbBuffer;

            ret = m_setPreviewCallbackBuffer();
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):m_setPreviewCallback Buffer fail", __FUNCTION__, __LINE__);
                return ret;
            }

            int bufIndex = -2;
            m_previewCallbackBufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &previewCbBuffer);

            ExynosCameraFrame  *newFrame = NULL;

            newFrame = m_previewFrameFactory->createNewFrameOnlyOnePipe(pipeIdCsc);
            if (newFrame == NULL) {
                CLOGE("ERR(%s):newFrame is NULL", __FUNCTION__);
                m_scpBufferMgr->cancelBuffer(buffer.index);
                m_previewCallbackBufferMgr->putBuffer(bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_NONE);
                goto func_exit;
            }

            m_copyMetaFrameToFrame(frame, newFrame, true, true);

            ret = m_doPreviewToCallbackFunc(pipeIdCsc, newFrame, buffer, previewCbBuffer);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):m_doPreviewToCallbackFunc fail", __FUNCTION__, __LINE__);
                m_previewCallbackBufferMgr->putBuffer(bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_NONE);
                m_scpBufferMgr->cancelBuffer(buffer.index);
                goto func_exit;
            } else {
                if (m_parameters->getCallbackNeedCopy2Rendering() == true) {
                    ret = m_doCallbackToPreviewFunc(pipeIdCsc, frame, previewCbBuffer, buffer);
                    if (ret < 0) {
                        CLOGE("ERR(%s[%d]):m_doCallbackToPreviewFunc fail", __FUNCTION__, __LINE__);
                        m_previewCallbackBufferMgr->putBuffer(bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_NONE);
                        m_scpBufferMgr->cancelBuffer(buffer.index);
                        goto func_exit;
                    }
                }
            }

            if (newFrame != NULL) {
                newFrame->decRef();
                m_frameMgr->deleteFrame(newFrame);
                newFrame = NULL;
            }

            m_previewCallbackBufferMgr->putBuffer(bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_NONE);
        }

        if (m_previewWindow != NULL) {
            if (timeStamp > 0L) {
                m_previewWindow->set_timestamp(m_previewWindow, (int64_t)timeStamp);
            } else {
                uint32_t fcount = 0;
                getStreamFrameCount((struct camera2_stream *)buffer.addr[2], &fcount);
                CLOGW("WRN(%s[%d]): frameCount(%d)(%d), Invalid timeStamp(%lld)",
                        __FUNCTION__, __LINE__,
                        frameCount,
                        fcount,
                        timeStamp);
            }
        }

#ifdef FIRST_PREVIEW_TIME_CHECK
        if (m_flagFirstPreviewTimerOn == true) {
            ExynosCameraActivityAutofocus *autoFocusMgr = m_exynosCameraActivityControl->getAutoFocusMgr();
            m_firstPreviewTimer.stop();
            m_flagFirstPreviewTimerOn = false;

            CLOGD("DEBUG(%s[%d]):m_firstPreviewTimer stop", __FUNCTION__, __LINE__);

            CLOGD("DEBUG(%s[%d]):============= First Preview time ==================", "m_printExynosCameraInfo", __LINE__);
            CLOGD("DEBUG(%s[%d]):= startPreview ~ first frame  : %d msec", "m_printExynosCameraInfo", __LINE__, (int)m_firstPreviewTimer.durationMsecs());
            CLOGD("DEBUG(%s[%d]):===================================================", "m_printExynosCameraInfo", __LINE__);
            autoFocusMgr->displayAFInfo();
        }
#endif

        /* display the frame */
        ret = m_putBuffers(m_scpBufferMgr, buffer.index);
        if (ret < 0) {
            /* TODO: error handling */
            CLOGE("ERR(%s[%d]):put Buffer fail", __FUNCTION__, __LINE__);
        }
    } else {
        ALOGW("WARN(%s[%d]):Preview frame buffer is canceled."
                "PreviewThread is blocked or too many buffers are in Service."
                "PreviewBufferCount(%d), ScpBufferMgr(%d), PreviewQ(%d)",
                __FUNCTION__, __LINE__,
                m_parameters->getPreviewBufferCount(),
                m_scpBufferMgr->getNumOfAvailableAndNoneBuffer(),
                previewQ->getSizeOfProcessQ());
        ret = m_scpBufferMgr->cancelBuffer(buffer.index);
    }

func_exit:

    if (frame != NULL) {
        frame->decRef();
        m_frameMgr->deleteFrame(frame);
        frame = NULL;
    }

#ifdef USE_PREVIEW_DURATION_CONTROL
    PreviewDurationTimer.stop();
    if ((m_getRecordingEnabled() == true) && (curMinFps == curMaxFps) &&
            (m_parameters->getShotMode() != SHOT_MODE_SEQUENCE)) {
        PreviewDurationTime = PreviewDurationTimer.durationUsecs();

        if ( frameDurationUs > PreviewDurationTime ) {
            uint64_t delay = frameDurationUs - PreviewDurationTime;
            CLOGV("(%s):Delay Time(%lld),fpsRange(%d,%d), Duration(%lld)", __FUNCTION__,
                frameDurationUs - PreviewDurationTime,
                curMinFps,
                curMaxFps,
                frameDurationUs);
            usleep(delay);
        }
    }
#endif
    return loop;
}

status_t ExynosCamera::m_doPreviewToCallbackFunc(
        int32_t pipeId,
        ExynosCameraFrame *newFrame,
        ExynosCameraBuffer previewBuf,
        ExynosCameraBuffer callbackBuf)
{
    CLOGV("DEBUG(%s): converting preview to callback buffer", __FUNCTION__);

    int ret = 0;
    status_t statusRet = NO_ERROR;

    int hwPreviewW = 0, hwPreviewH = 0;
    int hwPreviewFormat = m_parameters->getHwPreviewFormat();
    bool useCSC = m_parameters->getCallbackNeedCSC();

    ExynosCameraDurationTimer probeTimer;
    int probeTimeMSEC;
    uint32_t fcount = 0;

    m_parameters->getHwPreviewSize(&hwPreviewW, &hwPreviewH);

    ExynosRect srcRect, dstRect;

    camera_memory_t *previewCallbackHeap = NULL;
    previewCallbackHeap = m_getMemoryCb(callbackBuf.fd[0], callbackBuf.size[0], 1, m_callbackCookie);
    if (!previewCallbackHeap || previewCallbackHeap->data == MAP_FAILED) {
        CLOGE("ERR(%s[%d]):m_getMemoryCb(%d) fail", __FUNCTION__, __LINE__, callbackBuf.size[0]);
        statusRet = INVALID_OPERATION;
        goto done;
    }

    ret = m_setCallbackBufferInfo(&callbackBuf, (char *)previewCallbackHeap->data);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): setCallbackBufferInfo fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        statusRet = INVALID_OPERATION;
        goto done;
    }

    if (m_flagThreadStop == true || m_previewEnabled == false) {
        CLOGE("ERR(%s[%d]): preview was stopped!", __FUNCTION__, __LINE__);
        statusRet = INVALID_OPERATION;
        goto done;
    }

    ret = m_calcPreviewGSCRect(&srcRect, &dstRect);

    if (useCSC) {
        ret = newFrame->setSrcRect(pipeId, &srcRect);
        ret = newFrame->setDstRect(pipeId, &dstRect);

        ret = m_setupEntity(pipeId, newFrame, &previewBuf, &callbackBuf);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):setupEntity fail, pipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, pipeId, ret);
            statusRet = INVALID_OPERATION;
            goto done;
        }
        m_previewFrameFactory->pushFrameToPipe(&newFrame, pipeId);
        m_previewFrameFactory->setOutputFrameQToPipe(m_previewCallbackGscFrameDoneQ, pipeId);

        CLOGV("INFO(%s[%d]):wait preview callback output", __FUNCTION__, __LINE__);
        ret = m_previewCallbackGscFrameDoneQ->waitAndPopProcessQ(&newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            statusRet = INVALID_OPERATION;
            goto done;
        }
        if (newFrame == NULL) {
            CLOGE("ERR(%s[%d]):newFrame is NULL", __FUNCTION__, __LINE__);
            statusRet = INVALID_OPERATION;
            goto done;
        }
        CLOGV("INFO(%s[%d]):preview callback done", __FUNCTION__, __LINE__);

#if 0
        int remainedH = m_orgPreviewRect.h - dst_height;

        if (remainedH != 0) {
            char *srcAddr = NULL;
            char *dstAddr = NULL;
            int planeDiver = 1;

            for (int plane = 0; plane < 2; plane++) {
                planeDiver = (plane + 1) * 2 / 2;

                srcAddr = previewBuf.virt.extP[plane] + (ALIGN_UP(hwPreviewW, CAMERA_ISP_ALIGN) * dst_crop_height / planeDiver);
                dstAddr = callbackBuf->virt.extP[plane] + (m_orgPreviewRect.w * dst_crop_height / planeDiver);

                for (int i = 0; i < remainedH; i++) {
                    memcpy(dstAddr, srcAddr, (m_orgPreviewRect.w / planeDiver));

                    srcAddr += (ALIGN_UP(hwPreviewW, CAMERA_ISP_ALIGN) / planeDiver);
                    dstAddr += (m_orgPreviewRect.w                   / planeDiver);
                }
            }
        }
#endif
    } else { /* neon memcpy */
        char *srcAddr = NULL;
        char *dstAddr = NULL;
        unsigned int size = 0;
        int planeCount = getYuvPlaneCount(hwPreviewFormat);
        if (planeCount <= 0) {
            CLOGE("ERR(%s[%d]):getYuvPlaneCount(%d) fail", __FUNCTION__, __LINE__, hwPreviewFormat);
            statusRet = INVALID_OPERATION;
            goto done;
        }

        /* TODO : have to consider all fmt(planes) and stride */
        for (int plane = 0; plane < planeCount; plane++) {
            srcAddr = previewBuf.addr[plane];
            dstAddr = callbackBuf.addr[plane];

            if (previewBuf.size[plane] != callbackBuf.size[plane])
                CLOGW("WARN(%s[%d]):Preview buffer(%d), Preview callback buffer(%d) size mismatch, plane(%d)",
                    __FUNCTION__, __LINE__,
                    previewBuf.size[plane],
                    callbackBuf.size[plane],
                    plane);
            size = (previewBuf.size[plane] < callbackBuf.size[plane])? previewBuf.size[plane] : callbackBuf.size[plane];
            memcpy(dstAddr, srcAddr, size);
        }
    }

    probeTimer.start();
    if (m_parameters->msgTypeEnabled(CAMERA_MSG_PREVIEW_FRAME) &&
        !checkBit(&m_callbackState, CALLBACK_STATE_COMPRESSED_IMAGE) &&
        m_disablePreviewCB == false) {
        setBit(&m_callbackState, CALLBACK_STATE_PREVIEW_FRAME, false);
        m_dataCb(CAMERA_MSG_PREVIEW_FRAME, previewCallbackHeap, 0, NULL, m_callbackCookie);
        clearBit(&m_callbackState, CALLBACK_STATE_PREVIEW_FRAME, false);
    }
    probeTimer.stop();
    getStreamFrameCount((struct camera2_stream *)previewBuf.addr[2], &fcount);
    probeTimeMSEC = (int)probeTimer.durationMsecs();

    if (probeTimeMSEC > 33 && probeTimeMSEC <= 66)
        CLOGV("(%s[%d]):(%d) duration time(%5d msec)", __FUNCTION__, __LINE__, fcount, (int)probeTimer.durationMsecs());
    else if(probeTimeMSEC > 66)
        CLOGD("(%s[%d]):(%d) duration time(%5d msec)", __FUNCTION__, __LINE__, fcount, (int)probeTimer.durationMsecs());
    else
        CLOGV("(%s[%d]):(%d) duration time(%5d msec)", __FUNCTION__, __LINE__, fcount, (int)probeTimer.durationMsecs());

done:
    if (previewCallbackHeap != NULL) {
        previewCallbackHeap->release(previewCallbackHeap);
    }

    return statusRet;
}

status_t ExynosCamera::m_doCallbackToPreviewFunc(
        __unused int32_t pipeId,
        __unused ExynosCameraFrame *newFrame,
        ExynosCameraBuffer callbackBuf,
        ExynosCameraBuffer previewBuf)
{
    CLOGV("DEBUG(%s): converting callback to preview buffer", __FUNCTION__);

    int ret = 0;
    status_t statusRet = NO_ERROR;

    int hwPreviewW = 0, hwPreviewH = 0;
    int hwPreviewFormat = m_parameters->getHwPreviewFormat();
    bool useCSC = m_parameters->getCallbackNeedCSC();

    m_parameters->getHwPreviewSize(&hwPreviewW, &hwPreviewH);

    camera_memory_t *previewCallbackHeap = NULL;
    previewCallbackHeap = m_getMemoryCb(callbackBuf.fd[0], callbackBuf.size[0], 1, m_callbackCookie);
    if (!previewCallbackHeap || previewCallbackHeap->data == MAP_FAILED) {
        CLOGE("ERR(%s[%d]):m_getMemoryCb(%d) fail", __FUNCTION__, __LINE__, callbackBuf.size[0]);
        statusRet = INVALID_OPERATION;
        goto done;
    }

    ret = m_setCallbackBufferInfo(&callbackBuf, (char *)previewCallbackHeap->data);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): setCallbackBufferInfo fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        statusRet = INVALID_OPERATION;
        goto done;
    }

    if (useCSC) {
#if 0
        if (m_exynosPreviewCSC) {
            csc_set_src_format(m_exynosPreviewCSC,
                    ALIGN_DOWN(m_orgPreviewRect.w, CAMERA_MAGIC_ALIGN), ALIGN_DOWN(m_orgPreviewRect.h, CAMERA_MAGIC_ALIGN),
                    0, 0, ALIGN_DOWN(m_orgPreviewRect.w, CAMERA_MAGIC_ALIGN), ALIGN_DOWN(m_orgPreviewRect.h, CAMERA_MAGIC_ALIGN),
                    V4L2_PIX_2_HAL_PIXEL_FORMAT(m_orgPreviewRect.colorFormat),
                    1);

            csc_set_dst_format(m_exynosPreviewCSC,
                    previewW, previewH,
                    0, 0, previewW, previewH,
                    V4L2_PIX_2_HAL_PIXEL_FORMAT(previewFormat),
                    0);

            csc_set_src_buffer(m_exynosPreviewCSC,
                    (void **)callbackBuf->virt.extP, CSC_MEMORY_USERPTR);

            csc_set_dst_buffer(m_exynosPreviewCSC,
                    (void **)previewBuf.fd.extFd, CSC_MEMORY_TYPE);

            if (csc_convert_with_rotation(m_exynosPreviewCSC, 0, m_flip_horizontal, 0) != 0)
                CLOGE("ERR(%s):csc_convert() from callback to lcd fail", __FUNCTION__);
        } else {
            CLOGE("ERR(%s):m_exynosPreviewCSC == NULL", __FUNCTION__);
        }
#else
        CLOGW("WRN(%s[%d]): doCallbackToPreview use CSC is not yet possible", __FUNCTION__, __LINE__);
#endif
    } else { /* neon memcpy */
        char *srcAddr = NULL;
        char *dstAddr = NULL;
        int planeCount = getYuvPlaneCount(hwPreviewFormat);
        if (planeCount <= 0) {
            CLOGE("ERR(%s[%d]):getYuvPlaneCount(%d) fail", __FUNCTION__, __LINE__, hwPreviewFormat);
            statusRet = INVALID_OPERATION;
            goto done;
        }

        /* TODO : have to consider all fmt(planes) and stride */
        for (int plane = 0; plane < planeCount; plane++) {
            srcAddr = callbackBuf.addr[plane];
            dstAddr = previewBuf.addr[plane];
            memcpy(dstAddr, srcAddr, callbackBuf.size[plane]);
        }
    }

done:
    if (previewCallbackHeap != NULL) {
        previewCallbackHeap->release(previewCallbackHeap);
    }

    return statusRet;
}

status_t ExynosCamera::m_handlePreviewFrameFront(ExynosCameraFrame *frame)
{
    int ret = 0;
    uint32_t skipCount = 0;
    ExynosCameraFrameEntity *entity = NULL;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraFrame *fdFrame = NULL;
    ExynosCameraBuffer buffer;
    int pipeID = 0;
    ExynosCameraBuffer resultBuffer;
    camera2_node_group node_group_info_isp;
    enum pipeline pipe;

    entity = frame->getFrameDoneEntity();
    if (entity == NULL) {
        CLOGE("ERR(%s[%d]):current entity is NULL, frameCount(%d)",
            __FUNCTION__, __LINE__, frame->getFrameCount());
        /* TODO: doing exception handling */
        return true;
    }

    if (entity->getEntityState() == ENTITY_STATE_FRAME_SKIP)
            goto entity_state_complete;

    pipeID = entity->getPipeId();

    switch(entity->getPipeId()) {
    case PIPE_ISP:
        m_debugFpsCheck(entity->getPipeId());
        ret = frame->getSrcBuffer(entity->getPipeId(), &buffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getSrcBuffer fail, pipeId(%d), ret(%d)",
                __FUNCTION__, __LINE__, entity->getPipeId(), ret);
            return ret;
        }
        frame->setMetaDataEnable(true);

        ret = m_putBuffers(m_ispBufferMgr, buffer.index);

        ret = frame->getSrcBuffer(PIPE_3AA, &buffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getSrcBuffer fail, pipeId(%d), ret(%d)",
                __FUNCTION__, __LINE__, entity->getPipeId(), ret);
            return ret;
        }

        if (m_parameters->isReprocessing() == true) {
            ret = m_captureSelector->manageFrameHoldList(frame, pipeID, true);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):manageFrameHoldList fail", __FUNCTION__, __LINE__);
                return ret;
            }
        } else {
            /* TODO: This is unusual case, flite buffer and isp buffer */
            ret = m_putBuffers(m_bayerBufferMgr, buffer.index);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):put Buffer fail", __FUNCTION__, __LINE__);
            }
        }

        skipCount = m_parameters->getFrameSkipCount();
        if (skipCount <= 0 && m_flagStartFaceDetection == true) {
            /* Face detection */
            struct camera2_shot_ext shot;
            frame->getDynamicMeta(&shot);
            fdFrame = m_frameMgr->createFrame(m_parameters, frame->getFrameCount());
            fdFrame->storeDynamicMeta(&shot);
            m_facedetectQ->pushProcessQ(&fdFrame);
        }

        ret = generateFrame(m_3aa_ispFrameCount, &newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):generateFrame fail", __FUNCTION__, __LINE__);
            return ret;
        }

        m_setupEntity(PIPE_FLITE, newFrame);
        m_previewFrameFactory->pushFrameToPipe(&newFrame, PIPE_FLITE);

        m_setupEntity(PIPE_3AA, newFrame);
        m_setupEntity(PIPE_ISP, newFrame);
        m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, PIPE_ISP);

        m_3aa_ispFrameCount++;

        /* HACK: When SCC pipe is stopped, generate frame for SCC */
        if ((m_sccBufferMgr->getNumOfAvailableBuffer() > 2)) {
            CLOGW("WRN(%s[%d]): Too many available SCC buffers, generating frame for SCC", __FUNCTION__, __LINE__);

            pipe = (m_parameters->isOwnScc(getCameraId()) == true) ? PIPE_SCC : PIPE_ISPC;

            while (m_sccBufferMgr->getNumOfAvailableBuffer() > 0) {
                ret = generateFrame(m_sccFrameCount, &newFrame);
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):generateFrame fail", __FUNCTION__, __LINE__);
                    return ret;
                }

                m_setupEntity(pipe, newFrame);
                m_previewFrameFactory->pushFrameToPipe(&newFrame, pipe);
                m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, pipe);

                m_sccFrameCount++;
            }
        }

        break;
    case PIPE_ISPC:
    case PIPE_SCC:
        m_debugFpsCheck(entity->getPipeId());
        if (entity->getDstBufState() == ENTITY_BUFFER_STATE_COMPLETE) {
            ret = m_sccCaptureSelector->manageFrameHoldList(frame, entity->getPipeId(), false);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):manageFrameHoldList fail", __FUNCTION__, __LINE__);
                return ret;
            }
        }

        pipe = (m_parameters->isOwnScc(getCameraId()) == true) ? PIPE_SCC : PIPE_ISPC;

        while (m_sccBufferMgr->getNumOfAvailableBuffer() > 0) {
            ret = generateFrameSccScp(pipe, &m_sccFrameCount, &newFrame);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):generateFrameSccScp fail", __FUNCTION__, __LINE__);
                return ret;
            }

            m_setupEntity(pipe, newFrame);
            m_previewFrameFactory->pushFrameToPipe(&newFrame, pipe);
            m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, pipe);

            m_sccFrameCount++;
        }
        break;
    case PIPE_SCP:
        if (entity->getDstBufState() == ENTITY_BUFFER_STATE_ERROR) {
            ret = frame->getDstBuffer(entity->getPipeId(), &buffer, m_previewFrameFactory->getNodeType(PIPE_SCP));
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, entity->getPipeId(), ret);
                return ret;
            }
            ret = m_scpBufferMgr->cancelBuffer(buffer.index);
            if (ret < 0)
                CLOGE("ERR(%s[%d]):SCP buffer return fail", __FUNCTION__, __LINE__);

        } else if (entity->getDstBufState() == ENTITY_BUFFER_STATE_COMPLETE) {
            m_debugFpsCheck(entity->getPipeId());
            ret = frame->getDstBuffer(entity->getPipeId(), &buffer, m_previewFrameFactory->getNodeType(PIPE_SCP));
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, entity->getPipeId(), ret);
                return ret;
            }

            m_parameters->getFrameSkipCount(&m_frameSkipCount);
            if (m_frameSkipCount > 0) {
                CLOGD("INFO(%s[%d]):frameSkipCount=%d", __FUNCTION__, __LINE__, m_frameSkipCount);
                ret = m_scpBufferMgr->cancelBuffer(buffer.index);
                if (ret < 0)
                    CLOGE("ERR(%s[%d]):SCP buffer return fail", __FUNCTION__, __LINE__);
            } else {
                nsecs_t timeStamp = (nsecs_t)frame->getTimeStamp();
                if (m_getRecordingEnabled() == true
                    && m_parameters->msgTypeEnabled(CAMERA_MSG_VIDEO_FRAME)) {
                    if (timeStamp <= 0L) {
                        CLOGE("WARN(%s[%d]):timeStamp(%lld) Skip", __FUNCTION__, __LINE__, timeStamp);
                    } else {
                        /* get Recording Image buffer */
                        int bufIndex = -2;
                        ExynosCameraBuffer recordingBuffer;
                        ret = m_recordingBufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &recordingBuffer);
                        if (ret < 0 || bufIndex < 0) {
                            if ((++m_recordingFrameSkipCount % 100) == 0) {
                                CLOGE("ERR(%s[%d]): Recording buffer is not available!! Recording Frames are Skipping(%d frames) (bufIndex=%d)",
                                        __FUNCTION__, __LINE__, m_recordingFrameSkipCount, bufIndex);
                            }
                        } else {
                            if (m_recordingFrameSkipCount != 0) {
                                CLOGE("ERR(%s[%d]): Recording buffer is not available!! Recording Frames are Skipped(%d frames) (bufIndex=%d)",
                                        __FUNCTION__, __LINE__, m_recordingFrameSkipCount, bufIndex);
                                m_recordingFrameSkipCount = 0;
                            }
                            m_recordingTimeStamp[bufIndex] = timeStamp;
                            ret = m_doPrviewToRecordingFunc(PIPE_GSC_VIDEO, buffer, recordingBuffer, timeStamp);
                            if (ret < 0) {
                                CLOGW("WARN(%s[%d]):recordingCallback Skip", __FUNCTION__, __LINE__);
                            }
                        }
                    }
                }

                ExynosCameraBuffer callbackBuffer;
                ExynosCameraFrame *callbackFrame = NULL;
                struct camera2_shot_ext *shot_ext = new struct camera2_shot_ext;

                callbackFrame = m_previewFrameFactory->createNewFrameOnlyOnePipe(PIPE_SCP);
                ret = frame->getDstBuffer(PIPE_SCP, &callbackBuffer, m_previewFrameFactory->getNodeType(PIPE_SCP));
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):getDstBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                }
                frame->getMetaData(shot_ext);
                callbackFrame->storeDynamicMeta(shot_ext);
                callbackFrame->setDstBuffer(PIPE_SCP, callbackBuffer);
                delete shot_ext;

                CLOGV("INFO(%s[%d]):push frame to front previewQ", __FUNCTION__, __LINE__);
                m_previewQ->pushProcessQ(&callbackFrame);

                CLOGV("DEBUG(%s[%d]):SCP done HAL-frameCount(%d)", __FUNCTION__, __LINE__, frame->getFrameCount());
            }
        } else {
            CLOGV("DEBUG(%s[%d]):SCP droped - SCP buffer is not ready HAL-frameCount(%d)", __FUNCTION__, __LINE__, frame->getFrameCount());

            ret = frame->getDstBuffer(entity->getPipeId(), &buffer, m_previewFrameFactory->getNodeType(PIPE_SCP));
            if (buffer.index >= 0) {
                ret = m_scpBufferMgr->cancelBuffer(buffer.index);
                if (ret < 0)
                    CLOGE("ERR(%s[%d]):SCP buffer return fail", __FUNCTION__, __LINE__);
            }
            m_previewFrameFactory->dump();
        }

        ret = generateFrameSccScp(PIPE_SCP, &m_scpFrameCount, &newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):generateFrameSccScp fail", __FUNCTION__, __LINE__);
            return ret;
        }

        m_setupEntity(PIPE_SCP, newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):setupEntity fail", __FUNCTION__, __LINE__);
            break;
        }

        /*check preview drop...*/
        ret = newFrame->getDstBuffer(PIPE_SCP, &resultBuffer, m_previewFrameFactory->getNodeType(PIPE_SCP));
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getDstBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }
        if (resultBuffer.index < 0) {
            newFrame->setRequest(PIPE_SCP, false);
            newFrame->getNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_ISP);
            node_group_info_isp.capture[PERFRAME_FRONT_SCP_POS].request = 0;
            newFrame->storeNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_ISP);

            m_previewFrameFactory->dump();

            /* when preview buffer is not ready, we should drop preview to make preview buffer ready */
            CLOGW("WARN(%s[%d]):Front preview drop. Failed to get preview buffer. FrameSkipcount(%d)",
                    __FUNCTION__, __LINE__, FRAME_SKIP_COUNT_PREVIEW_FRONT);
            m_parameters->setFrameSkipCount(FRAME_SKIP_COUNT_PREVIEW_FRONT);
        }

        m_previewFrameFactory->pushFrameToPipe(&newFrame, PIPE_SCP);
        m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, PIPE_SCP);

        m_scpFrameCount++;
        break;
    default:
        break;
    }

    if (ret < 0) {
        CLOGE("ERR(%s[%d]):put Buffer fail", __FUNCTION__, __LINE__);
        return ret;
    }

entity_state_complete:

    ret = frame->setEntityState(entity->getPipeId(), ENTITY_STATE_COMPLETE);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setEntityState fail, pipeId(%d), state(%d), ret(%d)",
            __FUNCTION__, __LINE__, entity->getPipeId(), ENTITY_STATE_COMPLETE, ret);
        return ret;
    }

    if (frame->isComplete() == true) {
        ret = m_removeFrameFromList(&m_processList, frame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):remove frame from processList fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        frame->decRef();
        m_frameMgr->deleteFrame(frame);
    }

    return NO_ERROR;
}

status_t ExynosCamera::takePicture()
{
    int ret = 0;
    int takePictureCount = m_takePictureCounter.getCount();
    int seriesShotCount = 0;
    int currentSeriesShotMode = 0;
    ExynosCameraFrame *newFrame = NULL;
    int32_t reprocessingBayerMode = 0;

    int retryCount = 0;

    if (m_previewEnabled == false) {
        CLOGE("DEBUG(%s):preview is stopped, return error", __FUNCTION__);
        return INVALID_OPERATION;
    }

    if (m_parameters != NULL) {
        seriesShotCount = m_parameters->getSeriesShotCount();
        currentSeriesShotMode = m_parameters->getSeriesShotMode();
        reprocessingBayerMode = m_parameters->getReprocessingBayerMode();
        if (m_parameters->getVisionMode() == true) {
            CLOGW("WRN(%s[%d]): Vision mode does not support", __FUNCTION__, __LINE__);
            android_printAssert(NULL, LOG_TAG, "Cannot support this operation");

            return INVALID_OPERATION;
        }
    } else {
        CLOGE("ERR(%s):m_parameters is NULL", __FUNCTION__);
        return INVALID_OPERATION;
    }

    /* wait autoFocus is over for turning on preFlash */
    m_autoFocusThread->join();

    m_parameters->setMarkingOfExifFlash(0);

    /* HACK Reset Preview Flag*/
    while ((m_resetPreview == true) && (retryCount < 10)) {
        usleep(200000);
        retryCount ++;
        CLOGI("INFO(%s[%d]) retryCount(%d) m_resetPreview(%d)", __FUNCTION__, __LINE__, retryCount, m_resetPreview);
    }

    if (takePictureCount < 0) {
        CLOGE("ERR(%s[%d]): takePicture is called too much. takePictureCount(%d) / seriesShotCount(%d) . so, fail",
            __FUNCTION__, __LINE__, takePictureCount, seriesShotCount);
        return INVALID_OPERATION;
    } else if (takePictureCount == 0) {
        if (seriesShotCount == 0) {
            m_captureLock.lock();
            if (m_pictureEnabled == true) {
                CLOGE("ERR(%s[%d]): take picture is inprogressing", __FUNCTION__, __LINE__);
                /* return NO_ERROR; */
                m_captureLock.unlock();
                return INVALID_OPERATION;
            }
            m_captureLock.unlock();

            /* general shot */
            seriesShotCount = 1;
        }
        m_takePictureCounter.setCount(seriesShotCount);
    }

    CLOGI("INFO(%s[%d]): takePicture start m_takePictureCounter(%d), currentSeriesShotMode(%d) seriesShotCount(%d)",
        __FUNCTION__, __LINE__, m_takePictureCounter.getCount(), currentSeriesShotMode, seriesShotCount);

    m_printExynosCameraInfo(__FUNCTION__);

    if(m_parameters->getShotMode() == SHOT_MODE_RICH_TONE) {
        m_hdrEnabled = true;
    } else {
        m_hdrEnabled = false;
    }

    if (m_parameters->isReprocessing() == true) {
        if (m_parameters->getPictureFormat() == V4L2_PIX_FMT_NV21) {
            m_reprocessingFrameFactory = m_frameFactory[FRAME_FACTORY_TYPE_REPROCESSING_NV21];
            m_pictureFrameFactory = m_reprocessingFrameFactory;
        } else {
            m_reprocessingFrameFactory = m_frameFactory[FRAME_FACTORY_TYPE_REPROCESSING];
            m_pictureFrameFactory = m_reprocessingFrameFactory;
        }
    }

    /* TODO: Dynamic bayer capture, currently support only single shot */
    if (reprocessingBayerMode == REPROCESSING_BAYER_MODE_PURE_DYNAMIC) {
        int pipeId = m_getBayerPipeId();

        if (m_bayerBufferMgr->getNumOfAvailableBuffer() > 0) {
            m_previewFrameFactory->setRequestFLITE(true);
            ret = generateFrame(-1, &newFrame);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):generateFrame fail", __FUNCTION__, __LINE__);
                return ret;
            }
            m_previewFrameFactory->setRequestFLITE(false);

            ret = m_setupEntity(pipeId, newFrame);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):setupEntity fail", __FUNCTION__, __LINE__);
                return ret;
            }

            m_previewFrameFactory->pushFrameToPipe(&newFrame, pipeId);
            m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, pipeId);
        }

        m_previewFrameFactory->startThread(pipeId);
    } else if (reprocessingBayerMode == REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC) {
        /* Comment out, because it included 3AA, it always running */
        /*
        int pipeId = m_getBayerPipeId();

        if (m_bayerBufferMgr->getNumOfAvailableBuffer() > 0) {
            m_previewFrameFactory->setRequest3AC(true);
            ret = generateFrame(-1, &newFrame);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):generateFrame fail", __FUNCTION__, __LINE__);
                return ret;
            }
            m_previewFrameFactory->setRequest3AC(false);

            ret = m_setupEntity(pipeId, newFrame);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):setupEntity fail", __FUNCTION__, __LINE__);
                return ret;
            }

            m_previewFrameFactory->pushFrameToPipe(&newFrame, pipeId);
            m_previewFrameFactory->setOutputFrameQToPipe(m_pipeFrameDoneQ, pipeId);
        }

        m_previewFrameFactory->startThread(pipeId);
        */
        if (m_bayerBufferMgr->getNumOfAvailableBuffer() > 0)
            m_previewFrameFactory->setRequest3AC(true);
    } else if (m_parameters->getRecordingHint() == true
               && m_parameters->isUsing3acForIspc() == true) {
        if (m_sccBufferMgr->getNumOfAvailableBuffer() > 0)
            m_previewFrameFactory->setRequest3AC(true);
    }


    if (m_takePictureCounter.getCount() == seriesShotCount) {
        ExynosCameraActivitySpecialCapture *m_sCaptureMgr = m_exynosCameraActivityControl->getSpecialCaptureMgr();
        ExynosCameraActivityFlash *m_flashMgr = m_exynosCameraActivityControl->getFlashMgr();

        m_stopBurstShot = false;

        if (m_parameters->isReprocessing() == true)
            m_captureSelector->setIsFirstFrame(true);
        else
            m_sccCaptureSelector->setIsFirstFrame(true);

#if 0
        if (m_parameters->isReprocessing() == false || m_parameters->getSeriesShotCount() > 0 ||
                m_hdrEnabled == true) {
            m_pictureFrameFactory = m_previewFrameFactory;
            if (m_parameters->getUseDynamicScc() == true) {
                if (isOwnScc(getCameraId()) == true)
                    m_previewFrameFactory->setRequestSCC(true);
                else
                    m_previewFrameFactory->setRequestISPC(true);

                /* boosting dynamic SCC */
                if (m_hdrEnabled == false &&
                    currentSeriesShotMode == SERIES_SHOT_MODE_NONE) {
                    ret = m_boostDynamicCapture();
                    if (ret < 0)
                        CLOGW("WRN(%s[%d]): fail to boosting dynamic capture", __FUNCTION__, __LINE__);
                }

            }
        } else {
            m_pictureFrameFactory = m_reprocessingFrameFactory;
        }
#endif

        if (m_parameters->getScalableSensorMode()) {
            m_parameters->setScalableSensorMode(false);
            stopPreview();
            setPreviewWindow(m_previewWindow);
            startPreview();
            m_parameters->setScalableSensorMode(true);
        }

        CLOGI("INFO(%s[%d]): takePicture enabled, takePictureCount(%d)",
                __FUNCTION__, __LINE__, m_takePictureCounter.getCount());
        m_pictureEnabled = true;
        m_takePictureCounter.decCount();
        m_isNeedAllocPictureBuffer = true;

        m_startPictureBufferThread->join();

        if (m_parameters->isReprocessing() == true) {
            m_startPictureInternalThread->join();

#ifdef BURST_CAPTURE
            if (seriesShotCount > 1) {
                int allocCount = 0;
                int addCount = 0;
                CLOGD("DEBUG(%s[%d]): realloc buffer for burst shot", __FUNCTION__, __LINE__);
                m_burstRealloc = false;

                if (m_parameters->isHWFCEnabled() == false) {
                    allocCount = m_sccReprocessingBufferMgr->getAllocatedBufferCount();
                    addCount = (allocCount <= NUM_BURST_GSC_JPEG_INIT_BUFFER)?(NUM_BURST_GSC_JPEG_INIT_BUFFER-allocCount):0;
                    if (addCount > 0)
                        m_sccReprocessingBufferMgr->increase(addCount);
                }

                allocCount = m_jpegBufferMgr->getAllocatedBufferCount();
                addCount = (allocCount <= NUM_BURST_GSC_JPEG_INIT_BUFFER)?(NUM_BURST_GSC_JPEG_INIT_BUFFER-allocCount):0;
                if (addCount > 0)
                    m_jpegBufferMgr->increase(addCount);

                m_isNeedAllocPictureBuffer = true;
            }
#endif
        }

        CLOGD("DEBUG(%s[%d]): currentSeriesShotMode(%d), m_flashMgr->getNeedCaptureFlash(%d)",
            __FUNCTION__, __LINE__, currentSeriesShotMode, m_flashMgr->getNeedCaptureFlash());

#ifdef RAWDUMP_CAPTURE
        if(m_use_companion == true) {
            CLOGD("DEBUG(%s[%d]): start set Raw Capture mode", __FUNCTION__, __LINE__);
            m_sCaptureMgr->resetRawCaptureFcount();
            m_sCaptureMgr->setCaptureMode(ExynosCameraActivitySpecialCapture::SCAPTURE_MODE_RAW);

            m_parameters->setRawCaptureModeOn(true);

            enum aa_capture_intent captureIntent = AA_CAPTRUE_INTENT_STILL_CAPTURE_COMP_BYPASS;

            ret = m_previewFrameFactory->setControl(V4L2_CID_IS_INTENT, captureIntent, PIPE_3AA);
            if (ret < 0)
                CLOGE("ERR(%s[%d]):setControl(STILL_CAPTURE_RAW) fail. ret(%d) intent(%d)",
                __FUNCTION__, __LINE__, ret, captureIntent);
            m_sCaptureMgr->setCaptureStep(ExynosCameraActivitySpecialCapture::SCAPTURE_STEP_START);
        }
#else
        if (m_hdrEnabled == true) {
            seriesShotCount = HDR_REPROCESSING_COUNT;
            m_sCaptureMgr->setCaptureStep(ExynosCameraActivitySpecialCapture::SCAPTURE_STEP_START);
            m_sCaptureMgr->resetHdrStartFcount();
            m_parameters->setFrameSkipCount(13);
        } else if ((m_flashMgr->getNeedCaptureFlash() == true && currentSeriesShotMode == SERIES_SHOT_MODE_NONE)) {
            m_parameters->setMarkingOfExifFlash(1);

            if (m_flashMgr->checkPreFlash() == false && m_isTryStopFlash == false) {
                m_flashMgr->setCaptureStatus(true);
                CLOGD("DEBUG(%s[%d]): checkPreFlash(false), Start auto focus internally", __FUNCTION__, __LINE__);
                m_autoFocusType = AUTO_FOCUS_HAL;
                m_flashMgr->setFlashTrigerPath(ExynosCameraActivityFlash::FLASH_TRIGGER_SHORT_BUTTON);
                m_flashMgr->setFlashWaitCancel(false);

                /* execute autoFocus for preFlash */
                m_autoFocusThread->requestExitAndWait();
                m_autoFocusThread->run(PRIORITY_DEFAULT);
            }
        }
#endif /* RAWDUMP_CAPTURE */
#ifndef RAWDUMP_CAPTURE
        if (currentSeriesShotMode == SERIES_SHOT_MODE_NONE && m_flashMgr->getNeedCaptureFlash() == false)
            m_isZSLCaptureOn = true;
#endif
        m_parameters->setSetfileYuvRange();

        m_reprocessingCounter.setCount(seriesShotCount);
        if (m_prePictureThread->isRunning() == false) {
            if (m_prePictureThread->run(PRIORITY_DEFAULT) != NO_ERROR) {
                CLOGE("ERR(%s[%d]):couldn't run pre-picture thread", __FUNCTION__, __LINE__);
                return INVALID_OPERATION;
            }
        }

        m_jpegCounter.setCount(seriesShotCount);
        m_pictureCounter.setCount(seriesShotCount);
        if (m_pictureThread->isRunning() == false)
            ret = m_pictureThread->run();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):couldn't run picture thread, ret(%d)", __FUNCTION__, __LINE__, ret);
            return INVALID_OPERATION;
        }

        /* HDR, LLS, SIS should make YUV callback data. so don't use jpeg thread */
        if (!(m_hdrEnabled == true ||
                currentSeriesShotMode == SERIES_SHOT_MODE_LLS ||
                currentSeriesShotMode == SERIES_SHOT_MODE_SIS ||
                m_parameters->getShotMode() == SHOT_MODE_FRONT_PANORAMA)) {
            m_jpegCallbackThread->join();
            ret = m_jpegCallbackThread->run();
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):couldn't run jpeg callback thread, ret(%d)", __FUNCTION__, __LINE__, ret);
                return INVALID_OPERATION;
            }
        }
    } else {
        /* HDR, LLS, SIS should make YUV callback data. so don't use jpeg thread */
        if (!(m_hdrEnabled == true ||
                currentSeriesShotMode == SERIES_SHOT_MODE_LLS ||
                currentSeriesShotMode == SERIES_SHOT_MODE_SIS ||
                m_parameters->getShotMode() == SHOT_MODE_FRONT_PANORAMA)) {
            /* series shot : push buffer to callback thread. */
            m_jpegCallbackThread->join();
            ret = m_jpegCallbackThread->run();
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):couldn't run jpeg callback thread, ret(%d)", __FUNCTION__, __LINE__, ret);
                return INVALID_OPERATION;
            }
        }
        CLOGI("INFO(%s[%d]): series shot takePicture, takePictureCount(%d)",
                __FUNCTION__, __LINE__, m_takePictureCounter.getCount());
        m_takePictureCounter.decCount();

        /* TODO : in case of no reprocesssing, we make zsl scheme or increse buf */
        if (m_parameters->isReprocessing() == false)
            m_pictureEnabled = true;
    }

    return NO_ERROR;
}

bool ExynosCamera::m_reprocessingPrePictureInternal(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    CLOGI("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    int ret = 0;
    bool loop = false;
    int retry = 0;
    int retryIsp = 0;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraFrame *doneFrame = NULL;
    ExynosCameraFrameEntity *entity = NULL;
    camera2_shot_ext *shot_ext = NULL;
    camera2_stream *shot_stream = NULL;
    uint32_t bayerFrameCount = 0;
    struct camera2_node_output output_crop_info;

    ExynosCameraBufferManager *bufferMgr = NULL;

    int bayerPipeId = 0;
    int prePictureDonePipeId = 0;
    enum pipeline pipe;
    ExynosCameraBuffer bayerBuffer;
    ExynosCameraBuffer ispReprocessingBuffer;
    ExynosCameraBuffer yuvReprocessingBuffer;
    ExynosCameraBuffer thumbnailBuffer;
    int bufferIndex = -2;
    enum REPROCESSING_BAYER_MODE reprocessingBayerMode = (enum REPROCESSING_BAYER_MODE)(m_parameters->getReprocessingBayerMode());

    camera2_shot_ext *updateDmShot = new struct camera2_shot_ext;
    memset(updateDmShot, 0x0, sizeof(struct camera2_shot_ext));

    bayerBuffer.index = -2;
    ispReprocessingBuffer.index = -2;
    yuvReprocessingBuffer.index = -2;
    thumbnailBuffer.index = -2;

    int thumbnailW = 0, thumbnailH = 0;

    /*
     * in case of pureBayer and 3aa_isp OTF, buffer will go isp directly
     */
    if (m_parameters->isUseYuvReprocessing() == false) {
        if (m_parameters->getUsePureBayerReprocessing() == true) {
            if (m_parameters->isReprocessing3aaIspOTF() == true)
                prePictureDonePipeId = PIPE_3AA_REPROCESSING;
            else
                prePictureDonePipeId = PIPE_ISP_REPROCESSING;
        } else {
            prePictureDonePipeId = PIPE_ISP_REPROCESSING;
        }
    } else {
        prePictureDonePipeId = PIPE_MCSC_REPROCESSING;
    }

    if (m_parameters->getHighResolutionCallbackMode() == true) {
        if (m_highResolutionCallbackRunning == true) {
            /* will be removed */
            while (m_skipReprocessing == true) {
                usleep(WAITING_TIME);
                if (m_skipReprocessing == false) {
                    CLOGD("DEBUG(%s[%d]:stop skip frame for high resolution preview callback", __FUNCTION__, __LINE__);
                    break;
                }
            }
        } else if (m_highResolutionCallbackRunning == false) {
            CLOGW("WRN(%s[%d]): m_reprocessingThreadfunc stop for high resolution preview callback", __FUNCTION__, __LINE__);
            loop = false;
            goto CLEAN_FRAME;
        }
    }

    if (m_isZSLCaptureOn == true) {
        CLOGD("INFO(%s[%d]):fast shutter callback!!", __FUNCTION__, __LINE__);
        m_shutterCallbackThread->join();
        m_shutterCallbackThread->run();
    }

    /* Get Bayer buffer for reprocessing */
    if (reprocessingBayerMode == REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON
        || reprocessingBayerMode == REPROCESSING_BAYER_MODE_PURE_DYNAMIC) {
        ret = m_getBayerBuffer(m_getBayerPipeId(), &bayerBuffer);
    } else if (reprocessingBayerMode == REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON
               || reprocessingBayerMode == REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC) {
        ret = m_getBayerBuffer(m_getBayerPipeId(), &bayerBuffer, updateDmShot);
    } else {
        CLOGE("ERR(%s[%d]): bayer mode is not valid (%d)",
                __FUNCTION__, __LINE__, reprocessingBayerMode);
        goto CLEAN_FRAME;
    }

    if (ret < 0) {
        CLOGE("ERR(%s[%d]): getBayerBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        goto CLEAN_FRAME;
    }

    CLOGD("DEBUG(%s[%d]):bayerBuffer index %d", __FUNCTION__, __LINE__, bayerBuffer.index);

    if (m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC)
        m_captureSelector->clearList(m_getBayerPipeId(), false, m_previewFrameFactory->getNodeType(PIPE_3AC));

    if (m_isZSLCaptureOn == false) {
        m_shutterCallbackThread->join();
        m_shutterCallbackThread->run();
    }

    m_isZSLCaptureOn = false;

    if (m_parameters->isUseYuvReprocessingForThumbnail() == true)
        m_parameters->getThumbnailSize(&thumbnailW, &thumbnailH);

    if (m_parameters->isHWFCEnabled() == true) {
        if (m_hdrEnabled == true
            || m_parameters->getHighResolutionCallbackMode() == true
            || m_parameters->getPictureFormat() == V4L2_PIX_FMT_NV21
            || (m_parameters->isUseYuvReprocessingForThumbnail() == true
                && thumbnailW > 0 && thumbnailH > 0)) {
            m_reprocessingFrameFactory->setRequest(PIPE_HWFC_JPEG_SRC_REPROCESSING, false);
            m_reprocessingFrameFactory->setRequest(PIPE_HWFC_THUMB_SRC_REPROCESSING, false);
        } else {
            m_reprocessingFrameFactory->setRequest(PIPE_HWFC_JPEG_SRC_REPROCESSING, true);
            m_reprocessingFrameFactory->setRequest(PIPE_HWFC_THUMB_SRC_REPROCESSING, true);
        }
    }

    /* This is reprocessing path for Thumbnail */
    if (m_parameters->isUseYuvReprocessingForThumbnail() == true
        && m_parameters->getPictureFormat() != V4L2_PIX_FMT_NV21
        && m_parameters->getHighResolutionCallbackMode() == false
        && m_hdrEnabled == false
        && thumbnailW > 0 && thumbnailH > 0) {
        /* Generate reprocessing Frame */
        newFrame = NULL;
        ret = generateFrameReprocessing(&newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):generateFrameReprocessing fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            goto CLEAN_FRAME;
        }

        /* TODO: HACK: Will be removed, this is driver's job */
        ret = m_convertingStreamToShotExt(&bayerBuffer, &output_crop_info);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): shot_stream to shot_ext converting fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            goto CLEAN_FRAME;
        }

        camera2_node_group node_group_info;
        ExynosRect ratioCropSize;

        memset(&node_group_info, 0x0, sizeof(camera2_node_group));
        newFrame->getNodeGroupInfo(&node_group_info, PERFRAME_INFO_YUV_REPROCESSING_MCSC);

        setLeaderSizeToNodeGroupInfo(&node_group_info,
                                     output_crop_info.cropRegion[0],
                                     output_crop_info.cropRegion[1],
                                     output_crop_info.cropRegion[2],
                                     output_crop_info.cropRegion[3]);

        ret = getCropRectAlign(
                node_group_info.leader.input.cropRegion[2],
                node_group_info.leader.input.cropRegion[3],
                thumbnailW, thumbnailH,
                &ratioCropSize.x, &ratioCropSize.y, &ratioCropSize.w, &ratioCropSize.h,
                CAMERA_MCSC_ALIGN, 2, 0, 1.0);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):getCropRectAlign failed. MCSC in_crop %dx%d, MCSC out_size %dx%d",
                    __FUNCTION__, __LINE__,
                    node_group_info.leader.input.cropRegion[2],
                    node_group_info.leader.input.cropRegion[3],
                    thumbnailW, thumbnailH);

            ratioCropSize.x = 0;
            ratioCropSize.y = 0;
            ratioCropSize.w = node_group_info.leader.input.cropRegion[2];
            ratioCropSize.h = node_group_info.leader.input.cropRegion[3];
        }

        setCaptureCropNScaleSizeToNodeGroupInfo(&node_group_info,
                                                PERFRAME_REPROCESSING_SCC_POS,
                                                ratioCropSize.x, ratioCropSize.y,
                                                ratioCropSize.w, ratioCropSize.h,
                                                thumbnailW, thumbnailH);

        CLOGV("DEBUG(%s[%d]):leader input(%d %d %d %d), output(%d %d %d %d)", __FUNCTION__, __LINE__,
                                                                                    node_group_info.leader.input.cropRegion[0],
                                                                                    node_group_info.leader.input.cropRegion[1],
                                                                                    node_group_info.leader.input.cropRegion[2],
                                                                                    node_group_info.leader.input.cropRegion[3],
                                                                                    node_group_info.leader.output.cropRegion[0],
                                                                                    node_group_info.leader.output.cropRegion[1],
                                                                                    node_group_info.leader.output.cropRegion[2],
                                                                                    node_group_info.leader.output.cropRegion[3]);

        CLOGV("DEBUG(%s[%d]):capture input(%d %d %d %d), output(%d %d %d %d)", __FUNCTION__, __LINE__,
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[0],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[1],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[2],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[3],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[0],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[1],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[2],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[3]);

        if (node_group_info.leader.output.cropRegion[2] < node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[2]) {
            CLOGI("INFO(%s[%d]:(%d -> %d))", __FUNCTION__, __LINE__,
                node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[2],
                node_group_info.leader.output.cropRegion[2]);

            node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[2] = node_group_info.leader.output.cropRegion[2];
        }
        if (node_group_info.leader.output.cropRegion[3] < node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[3]) {
            CLOGI("INFO(%s[%d]:(%d -> %d))", __FUNCTION__, __LINE__,
                node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[3],
                node_group_info.leader.output.cropRegion[3]);

            node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[3] = node_group_info.leader.output.cropRegion[3];
        }

        newFrame->storeNodeGroupInfo(&node_group_info, PERFRAME_INFO_YUV_REPROCESSING_MCSC);

        shot_ext = (struct camera2_shot_ext *)(bayerBuffer.addr[bayerBuffer.planeCount-1]);

        /* Meta setting */
        if (shot_ext != NULL) {
            ret = newFrame->storeDynamicMeta(updateDmShot);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]): storeDynamicMeta fail ret(%d)", __FUNCTION__, __LINE__, ret);
                goto CLEAN_FRAME;
            }

            ret = newFrame->storeUserDynamicMeta(updateDmShot);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]): storeUserDynamicMeta fail ret(%d)", __FUNCTION__, __LINE__, ret);
                goto CLEAN_FRAME;
            }

            newFrame->getMetaData(shot_ext);
            m_parameters->duplicateCtrlMetadata((void *)shot_ext);

            CLOGD("DEBUG(%s[%d]):meta_shot_ext->shot.dm.request.frameCount : %d",
                    __FUNCTION__, __LINE__,
                    getMetaDmRequestFrameCount(shot_ext));
        } else {
            CLOGE("DEBUG(%s[%d]):shot_ext is NULL", __FUNCTION__, __LINE__);
        }

        ret = m_reprocessingFrameFactory->startInitialThreads();
        if (ret < 0) {
            CLOGE("ERR(%s):startInitialThreads fail", __FUNCTION__);
            goto CLEAN_FRAME;
        }

        /* Get bayerPipeId at first entity */
        bayerPipeId = newFrame->getFirstEntity()->getPipeId();
        CLOGD("DEBUG(%s[%d]): bayer Pipe ID(%d)", __FUNCTION__, __LINE__, bayerPipeId);

        /* Check available buffer */
        ret = m_getBufferManager(bayerPipeId, &bufferMgr, DST_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): getBufferManager fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            goto CLEAN_FRAME;
        }

        if (bufferMgr != NULL) {
            ret = m_checkBufferAvailable(bayerPipeId, bufferMgr);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]): Waiting buffer timeout, bayerPipeId(%d), ret(%d)",
                        __FUNCTION__, __LINE__, bayerPipeId, ret);
                goto CLEAN_FRAME;
            }
        }

        ret = m_setupEntity(bayerPipeId, newFrame, &bayerBuffer, NULL);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]:setupEntity fail, bayerPipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, bayerPipeId, ret);
            goto CLEAN_FRAME;
        }

        m_reprocessingFrameFactory->setOutputFrameQToPipe(dstIspReprocessingQ, prePictureDonePipeId);

        while (dstIspReprocessingQ->getSizeOfProcessQ() > 0) {
            dstIspReprocessingQ->popProcessQ(&doneFrame);

            doneFrame->decRef();
            m_frameMgr->deleteFrame(doneFrame);
            doneFrame = NULL;
        }

        newFrame->incRef();

        /* push the newFrameReprocessing to pipe */
        m_reprocessingFrameFactory->pushFrameToPipe(&newFrame, bayerPipeId);

        /* wait ISP done */
        CLOGI("INFO(%s[%d]):wait ISP output", __FUNCTION__, __LINE__);

        doneFrame = NULL;
        do {
            ret = dstIspReprocessingQ->waitAndPopProcessQ(&doneFrame);
            retryIsp++;
        } while (ret == TIMED_OUT && retryIsp < 100 && m_flagThreadStop != true);

        if (ret < 0) {
            CLOGW("WARN(%s[%d]):ISP wait and pop return, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            /* goto CLEAN; */
        }

        if (doneFrame == NULL) {
            CLOGE("ERR(%s[%d]):doneFrame is NULL", __FUNCTION__, __LINE__);
            goto CLEAN_FRAME;
        }

        doneFrame->decRef();

        ret = newFrame->setEntityState(bayerPipeId, ENTITY_STATE_COMPLETE);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):setEntityState(ENTITY_STATE_PROCESSING) fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, bayerPipeId, ret);

            if (updateDmShot != NULL) {
                delete updateDmShot;
                updateDmShot = NULL;
            }

            return ret;
        }

        CLOGI("INFO(%s[%d]):ISP output done", __FUNCTION__, __LINE__);

        newFrame->setMetaDataEnable(true);

        /* Copy thumbnail image to thumbnail buffer */
        ret = newFrame->getDstBuffer(bayerPipeId, &yuvReprocessingBuffer, m_reprocessingFrameFactory->getNodeType(PIPE_MCSC0_REPROCESSING));
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):getDstBuffer fail, bayerPipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, bayerPipeId, ret);
            goto CLEAN_FRAME;
        }

        ret = m_thumbnailBufferMgr->getBuffer(&bufferIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &thumbnailBuffer);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):get thumbnail Buffer fail, ret(%d)",
                    __FUNCTION__, __LINE__, ret);
            goto CLEAN_FRAME;
        }

        memcpy(thumbnailBuffer.addr[0], yuvReprocessingBuffer.addr[0], thumbnailBuffer.size[0]);

        /* Put buffers */
        ret = m_putBuffers(m_thumbnailBufferMgr, thumbnailBuffer.index);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):ThumbnailBuffer putBuffer fail, index(%d), ret(%d)",
                    __FUNCTION__, __LINE__, thumbnailBuffer.index, ret);
            goto CLEAN_FRAME;
        }

        /* Put reprocessing dst buffer */
        ret = m_getBufferManager(bayerPipeId, &bufferMgr, DST_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getBufferManager fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            goto CLEAN_FRAME;
        }

        if (bufferMgr != NULL) {
            ret = m_putBuffers(bufferMgr, yuvReprocessingBuffer.index);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):DstBuffer putBuffer fail, index(%d), ret(%d)",
                        __FUNCTION__, __LINE__, yuvReprocessingBuffer.index, ret);
                goto CLEAN_FRAME;
            }
        }

        /* Delete new frame */
        CLOGD("DEBUG(%s[%d]):Reprocessing frame for thumbnail delete(%d)",
                __FUNCTION__, __LINE__, newFrame->getFrameCount());
        newFrame->decRef();
        m_frameMgr->deleteFrame(newFrame);
        newFrame = NULL;

        /* Set JPEG request true */
        if (m_parameters->isHWFCEnabled() == true) {
            m_reprocessingFrameFactory->setRequest(PIPE_HWFC_JPEG_SRC_REPROCESSING, true);
            m_reprocessingFrameFactory->setRequest(PIPE_HWFC_THUMB_SRC_REPROCESSING, true);
        }
    }

    /* Generate reprocessing Frame */
    newFrame = NULL;
    ret = generateFrameReprocessing(&newFrame);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):generateFrameReprocessing fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        goto CLEAN_FRAME;
    }

#ifndef RAWDUMP_CAPTURE
#ifdef DEBUG_RAWDUMP
    if (m_parameters->checkBayerDumpEnable()) {
        int sensorMaxW, sensorMaxH;
        int sensorMarginW, sensorMarginH;
        bool bRet;
        char filePath[70];

        memset(filePath, 0, sizeof(filePath));
        snprintf(filePath, sizeof(filePath), "/data/media/0/RawCapture%d_%d.raw",m_cameraId, m_fliteFrameCount);
        if (m_parameters->getUsePureBayerReprocessing() == true)
            /* Pure Bayer Buffer Size == MaxPictureSize + Sensor Margin == Max Sensor Size */
        m_parameters->getMaxSensorSize(&sensorMaxW, &sensorMaxH);
        else
            m_parameters->getMaxPictureSize(&sensorMaxW, &sensorMaxH);

        bRet = dumpToFile((char *)filePath,
            bayerBuffer.addr[0],
            sensorMaxW * sensorMaxH * 2);
        if (bRet != true)
            CLOGE("couldn't make a raw file");
    }
#endif /* DEBUG_RAWDUMP */
#endif

    if (m_parameters->getUsePureBayerReprocessing() == false) {
        if (m_parameters->isUseYuvReprocessingForThumbnail() == false
            || m_parameters->getPictureFormat() == V4L2_PIX_FMT_NV21
            || m_parameters->getHighResolutionCallbackMode() == true
            || m_hdrEnabled == true
            || thumbnailW <= 0 || thumbnailH <= 0) {
            /* TODO: HACK: Will be removed, this is driver's job */
            ret = m_convertingStreamToShotExt(&bayerBuffer, &output_crop_info);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]): shot_stream to shot_ext converting fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                goto CLEAN_FRAME;
            }
        }

        camera2_node_group node_group_info;
        ExynosRect ratioCropSize;
        int pictureW = 0, pictureH = 0;

        memset(&node_group_info, 0x0, sizeof(camera2_node_group));

        if (m_parameters->isUseYuvReprocessing() == true)
            newFrame->getNodeGroupInfo(&node_group_info, PERFRAME_INFO_YUV_REPROCESSING_MCSC);
        else
            newFrame->getNodeGroupInfo(&node_group_info, PERFRAME_INFO_DIRTY_REPROCESSING_ISP);

        m_parameters->getPictureSize(&pictureW, &pictureH);

        setLeaderSizeToNodeGroupInfo(&node_group_info,
                                     output_crop_info.cropRegion[0],
                                     output_crop_info.cropRegion[1],
                                     output_crop_info.cropRegion[2],
                                     output_crop_info.cropRegion[3]);

        if (m_parameters->isUseYuvReprocessing() == true) {
            ret = getCropRectAlign(
                    node_group_info.leader.input.cropRegion[2],
                    node_group_info.leader.input.cropRegion[3],
                    pictureW, pictureH,
                    &ratioCropSize.x, &ratioCropSize.y, &ratioCropSize.w, &ratioCropSize.h,
                    CAMERA_MCSC_ALIGN, 2, 0, 1.0);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):getCropRectAlign failed. MCSC in_crop %dx%d, MCSC out_size %dx%d",
                        __FUNCTION__, __LINE__,
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
        } else {
            setCaptureCropNScaleSizeToNodeGroupInfo(&node_group_info,
                                                    PERFRAME_REPROCESSING_SCC_POS,
                                                    output_crop_info.cropRegion[0],
                                                    output_crop_info.cropRegion[1],
                                                    output_crop_info.cropRegion[2],
                                                    output_crop_info.cropRegion[3],
                                                    output_crop_info.cropRegion[2],
                                                    output_crop_info.cropRegion[3]);
        }

        CLOGV("DEBUG(%s[%d]):leader input(%d %d %d %d), output(%d %d %d %d)", __FUNCTION__, __LINE__,
                                                                                    node_group_info.leader.input.cropRegion[0],
                                                                                    node_group_info.leader.input.cropRegion[1],
                                                                                    node_group_info.leader.input.cropRegion[2],
                                                                                    node_group_info.leader.input.cropRegion[3],
                                                                                    node_group_info.leader.output.cropRegion[0],
                                                                                    node_group_info.leader.output.cropRegion[1],
                                                                                    node_group_info.leader.output.cropRegion[2],
                                                                                    node_group_info.leader.output.cropRegion[3]);

        CLOGV("DEBUG(%s[%d]):capture input(%d %d %d %d), output(%d %d %d %d)", __FUNCTION__, __LINE__,
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[0],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[1],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[2],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[3],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[0],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[1],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[2],
                                                                                    node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].output.cropRegion[3]);

        if (node_group_info.leader.output.cropRegion[2] < node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[2]) {
            CLOGI("INFO(%s[%d]:(%d -> %d))", __FUNCTION__, __LINE__,
                node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[2],
                node_group_info.leader.output.cropRegion[2]);

            node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[2] = node_group_info.leader.output.cropRegion[2];
        }
        if (node_group_info.leader.output.cropRegion[3] < node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[3]) {
            CLOGI("INFO(%s[%d]:(%d -> %d))", __FUNCTION__, __LINE__,
                node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[3],
                node_group_info.leader.output.cropRegion[3]);

            node_group_info.capture[PERFRAME_REPROCESSING_SCC_POS].input.cropRegion[3] = node_group_info.leader.output.cropRegion[3];
        }

        if (m_parameters->isUseYuvReprocessing() == true)
            newFrame->storeNodeGroupInfo(&node_group_info, PERFRAME_INFO_YUV_REPROCESSING_MCSC);
        else
            newFrame->storeNodeGroupInfo(&node_group_info, PERFRAME_INFO_DIRTY_REPROCESSING_ISP);
    }

    shot_ext = (struct camera2_shot_ext *)(bayerBuffer.addr[bayerBuffer.planeCount-1]);

    /* Meta setting */
    if (shot_ext != NULL) {
        if (m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON ||
            m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_PURE_DYNAMIC) {
            ret = newFrame->storeDynamicMeta(shot_ext);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]): storeDynamicMeta fail ret(%d)", __FUNCTION__, __LINE__, ret);
                goto CLEAN_FRAME;
            }

            ret = newFrame->storeUserDynamicMeta(shot_ext);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]): storeUserDynamicMeta fail ret(%d)", __FUNCTION__, __LINE__, ret);
                goto CLEAN_FRAME;
            }
        } else if (m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON ||
            m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC) {
            ret = newFrame->storeDynamicMeta(updateDmShot);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]): storeDynamicMeta fail ret(%d)", __FUNCTION__, __LINE__, ret);
                goto CLEAN_FRAME;
            }

            ret = newFrame->storeUserDynamicMeta(updateDmShot);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]): storeUserDynamicMeta fail ret(%d)", __FUNCTION__, __LINE__, ret);
                goto CLEAN_FRAME;
            }
        }

        newFrame->getMetaData(shot_ext);
        m_parameters->duplicateCtrlMetadata((void *)shot_ext);

        CLOGD("DEBUG(%s[%d]):meta_shot_ext->shot.dm.request.frameCount : %d",
                __FUNCTION__, __LINE__,
                getMetaDmRequestFrameCount(shot_ext));
    } else {
        CLOGE("DEBUG(%s[%d]):shot_ext is NULL", __FUNCTION__, __LINE__);
    }

    if (m_parameters->isUseYuvReprocessing() == true)
        pipe = PIPE_MCSC_REPROCESSING;
    else if (m_parameters->isReprocessing3aaIspOTF() == false)
        pipe = PIPE_ISP_REPROCESSING;
    else
        pipe = PIPE_3AA_REPROCESSING;

    if (m_parameters->getHighResolutionCallbackMode() == true
        && m_highResolutionCallbackRunning == true)
        m_reprocessingFrameFactory->setFrameDoneQToPipe(m_highResolutionCallbackQ, pipe);
    else if (m_parameters->isUseYuvReprocessingForThumbnail() == false)
        m_reprocessingFrameFactory->setFrameDoneQToPipe(dstSccReprocessingQ, pipe);

    /* Add frame to post processing list */
    CLOGD("DEBUG(%s[%d]):postPictureList size(%zd), frame(%d)",
            __FUNCTION__, __LINE__, m_postProcessList.size(), newFrame->getFrameCount());
    newFrame->frameLock();
    m_postProcessList.push_back(newFrame);

    ret = m_reprocessingFrameFactory->startInitialThreads();
    if (ret < 0) {
        CLOGE("ERR(%s):startInitialThreads fail", __FUNCTION__);
        goto CLEAN_FRAME;
    }

    /* Get bayerPipeId at first entity */
    bayerPipeId = newFrame->getFirstEntity()->getPipeId();
    CLOGD("DEBUG(%s[%d]): bayer Pipe ID(%d)", __FUNCTION__, __LINE__, bayerPipeId);

    /* Check available buffer */
    ret = m_getBufferManager(bayerPipeId, &bufferMgr, DST_BUFFER_DIRECTION);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): getBufferManager fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        goto CLEAN_FRAME;
    }

    if (bufferMgr != NULL) {
        ret = m_checkBufferAvailable(bayerPipeId, bufferMgr);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): Waiting buffer timeout, bayerPipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, bayerPipeId, ret);
            goto CLEAN_FRAME;
        }
    }

    if (m_parameters->isHWFCEnabled() == true) {
        ret = m_checkBufferAvailable(PIPE_HWFC_JPEG_DST_REPROCESSING, m_jpegBufferMgr);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]): Waiting buffer timeout, bayerPipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, bayerPipeId, ret);
            goto CLEAN_FRAME;
        }

        ret = m_checkBufferAvailable(PIPE_HWFC_THUMB_SRC_REPROCESSING, m_thumbnailBufferMgr);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]): Waiting buffer timeout, bayerPipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, bayerPipeId, ret);
            goto CLEAN_FRAME;
        }
    }

    ret = m_setupEntity(bayerPipeId, newFrame, &bayerBuffer, NULL);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]:setupEntity fail, bayerPipeId(%d), ret(%d)",
            __FUNCTION__, __LINE__, bayerPipeId, ret);
        goto CLEAN_FRAME;
    }

    m_reprocessingFrameFactory->setOutputFrameQToPipe(dstIspReprocessingQ, prePictureDonePipeId);

    while (dstIspReprocessingQ->getSizeOfProcessQ() > 0) {
        dstIspReprocessingQ->popProcessQ(&doneFrame);

        if (m_parameters->isUseYuvReprocessingForThumbnail() == true
            && m_parameters->getHighResolutionCallbackMode() == false) {
            doneFrame->decRef();
            m_frameMgr->deleteFrame(doneFrame);
        }
        doneFrame = NULL;
    }

    newFrame->incRef();

    /* push the newFrameReprocessing to pipe */
    m_reprocessingFrameFactory->pushFrameToPipe(&newFrame, bayerPipeId);

    /* When enabled SCC capture or pureBayerReprocessing, we need to start bayer pipe thread */
    if (m_parameters->getUsePureBayerReprocessing() == true ||
        m_parameters->isSccCapture() == true)
        m_reprocessingFrameFactory->startThread(bayerPipeId);

    /* wait ISP done */
    CLOGI("INFO(%s[%d]):wait ISP output", __FUNCTION__, __LINE__);

    doneFrame = NULL;
    do {
        ret = dstIspReprocessingQ->waitAndPopProcessQ(&doneFrame);
        retryIsp++;
    } while (ret == TIMED_OUT && retryIsp < 100 && m_flagThreadStop != true);

    if (ret < 0) {
        CLOGW("WARN(%s[%d]):ISP wait and pop return, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        /* goto CLEAN; */
    }

    if (doneFrame == NULL) {
        CLOGE("ERR(%s[%d]):doneFrame is NULL", __FUNCTION__, __LINE__);
        goto CLEAN_FRAME;
    }

    if (m_parameters->isUseYuvReprocessingForThumbnail() == true
        && m_parameters->getHighResolutionCallbackMode() == false)
        dstSccReprocessingQ->pushProcessQ(&newFrame);

    ret = newFrame->setEntityState(bayerPipeId, ENTITY_STATE_COMPLETE);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):setEntityState(ENTITY_STATE_PROCESSING) fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, bayerPipeId, ret);

        if (updateDmShot != NULL) {
            delete updateDmShot;
            updateDmShot = NULL;
        }

        return ret;
    }

    CLOGI("INFO(%s[%d]):ISP output done", __FUNCTION__, __LINE__);

    newFrame->setMetaDataEnable(true);

    if (m_parameters->isUseYuvReprocessing() == true) {
        /* put YUV buffer */
        ret = m_putBuffers(m_sccBufferMgr, bayerBuffer.index);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):MCSC src putBuffer fail, index(%d), ret(%d)", __FUNCTION__, __LINE__, bayerBuffer.index, ret);
            goto CLEAN_FRAME;
        }
    } else {
        /* put bayer buffer */
        ret = m_putBuffers(m_bayerBufferMgr, bayerBuffer.index);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):3AA src putBuffer fail, index(%d), ret(%d)", __FUNCTION__, __LINE__, bayerBuffer.index, ret);
            goto CLEAN_FRAME;
        }
    }

    /* put isp buffer */
    if (m_parameters->getUsePureBayerReprocessing() == true) {
        ret = m_getBufferManager(bayerPipeId, &bufferMgr, DST_BUFFER_DIRECTION);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): getBufferManager fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            goto CLEAN_FRAME;
        }

        if (bufferMgr != NULL) {
            ret = newFrame->getDstBuffer(bayerPipeId, &ispReprocessingBuffer, m_previewFrameFactory->getNodeType(PIPE_FLITE));
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):getDstBuffer fail, bayerPipeId(%d), ret(%d)",
                        __FUNCTION__, __LINE__, bayerPipeId, ret);
                goto CLEAN_FRAME;
            }

            ret = m_putBuffers(m_ispReprocessingBufferMgr, ispReprocessingBuffer.index);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]): ISP src putBuffer fail, index(%d), ret(%d)", __FUNCTION__, __LINE__, bayerBuffer.index, ret);
                goto CLEAN_FRAME;
            }
        }
    }

    m_reprocessingCounter.decCount();

    CLOGI("INFO(%s[%d]):reprocessing complete, remaining count(%d)", __FUNCTION__, __LINE__, m_reprocessingCounter.getCount());

    if (m_hdrEnabled) {
        ExynosCameraActivitySpecialCapture *m_sCaptureMgr;

        m_sCaptureMgr = m_exynosCameraActivityControl->getSpecialCaptureMgr();

        if (m_reprocessingCounter.getCount() == 0)
            m_sCaptureMgr->setCaptureStep(ExynosCameraActivitySpecialCapture::SCAPTURE_STEP_OFF);
    }

    if (newFrame != NULL) {
        CLOGD("DEBUG(%s[%d]): Reprocessing frame delete(%d)", __FUNCTION__, __LINE__, newFrame->getFrameCount());
        newFrame->decRef();
        m_frameMgr->deleteFrame(newFrame);
        newFrame = NULL;
    }

    if ((m_parameters->getHighResolutionCallbackMode() == true) &&
        (m_highResolutionCallbackRunning == true))
        loop = true;

    if (m_reprocessingCounter.getCount() > 0)
        loop = true;

    if (updateDmShot != NULL) {
        delete updateDmShot;
        updateDmShot = NULL;
    }

    /* one shot */
    return loop;

CLEAN_FRAME:
    /* newFrame is not pushed any pipes, we can delete newFrame */
    if (newFrame != NULL) {
        newFrame->printEntity();
        CLOGD("DEBUG(%s[%d]): Reprocessing frame delete(%d)", __FUNCTION__, __LINE__, newFrame->getFrameCount());
        newFrame->decRef();
        m_frameMgr->deleteFrame(newFrame);
        newFrame = NULL;
    }

CLEAN:
    if (0 <= bayerBuffer.index) {
        if (m_parameters->isUseYuvReprocessing() == true) {
            /* put YUV buffer */
            ret = m_putBuffers(m_sccBufferMgr, bayerBuffer.index);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_putBuffer(m_sccBufferMgr, %d) fail, ret(%d)",
                        __FUNCTION__, __LINE__, bayerBuffer.index, ret);
            }
        } else {
            /* put Bayer buffer */
            ret = m_putBuffers(m_bayerBufferMgr, bayerBuffer.index);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_putBuffer(m_bayerBufferMgr, %d) fail, ret(%d)",
                        __FUNCTION__, __LINE__, bayerBuffer.index, ret);
            }
        }
    }
    if (ispReprocessingBuffer.index != -2 && m_ispReprocessingBufferMgr != NULL)
        m_putBuffers(m_ispReprocessingBufferMgr, ispReprocessingBuffer.index);

    /* newFrame is already pushed some pipes, we can not delete newFrame until frame is complete */
    if (newFrame != NULL) {
        newFrame->frameUnlock();
        ret = m_removeFrameFromList(&m_postProcessList, newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):remove frame from processList fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        newFrame->printEntity();
        CLOGD("DEBUG(%s[%d]): Reprocessing frame delete(%d)", __FUNCTION__, __LINE__, newFrame->getFrameCount());
        newFrame->decRef();
        m_frameMgr->deleteFrame(newFrame);
        newFrame = NULL;
    }

    if (updateDmShot != NULL) {
        delete updateDmShot;
        updateDmShot = NULL;
    }

    if (m_hdrEnabled) {
        ExynosCameraActivitySpecialCapture *m_sCaptureMgr;

        m_sCaptureMgr = m_exynosCameraActivityControl->getSpecialCaptureMgr();

        if (m_reprocessingCounter.getCount() == 0)
            m_sCaptureMgr->setCaptureStep(ExynosCameraActivitySpecialCapture::SCAPTURE_STEP_OFF);
    }

    if ((m_parameters->getHighResolutionCallbackMode() == true) &&
        (m_highResolutionCallbackRunning == true))
        loop = true;

    if (m_reprocessingCounter.getCount() > 0)
        loop = true;

    CLOGI("INFO(%s[%d]): reprocessing fail, remaining count(%d)", __FUNCTION__, __LINE__, m_reprocessingCounter.getCount());

    return loop;
}

bool ExynosCamera::m_CheckBurstJpegSavingPath(char *dir)
{
    int ret = false;

    struct dirent **items;
    struct stat fstat;

    char *burstPath = m_parameters->getSeriesShotFilePath();

    char ChangeDirPath[BURST_CAPTURE_FILEPATH_SIZE] = {'\0',};

    memset(m_burstSavePath, 0, sizeof(m_burstSavePath));

    // Check access path
    if (burstPath && sizeof(m_burstSavePath) >= sizeof(burstPath)) {
        strncpy(m_burstSavePath, burstPath, sizeof(m_burstSavePath)-1);
    } else {
        CLOGW("WARN(%s[%d]) Parameter burstPath is NULL. Change to Default Path", __FUNCTION__, __LINE__);
        snprintf(m_burstSavePath, sizeof(m_burstSavePath), "%s/DCIM/Camera/", dir);
    }

    if (access(m_burstSavePath, 0)==0) {
        CLOGW("WARN(%s[%d]) success access dir = %s", __FUNCTION__, __LINE__, m_burstSavePath);
        return true;
    }

    CLOGW("WARN(%s[%d]) can't find dir = %s", __FUNCTION__, __LINE__, m_burstSavePath);

    // If directory cant't access, then search "DCIM/Camera" folder in current directory
    int iitems = scandir(dir, &items, NULL, alphasort);
    int lstatRet = -1;
    for (int i = 0; i < iitems; i++) {
        // Search only dcim directory
        if (lstat(items[i]->d_name, &fstat) < 0)
            continue;
        if ((fstat.st_mode & S_IFDIR) == S_IFDIR) {
            if (!strcmp(items[i]->d_name, ".") || !strcmp(items[i]->d_name, ".."))
                continue;
            if (strcasecmp(items[i]->d_name, "DCIM")==0) {
                sprintf(ChangeDirPath, "%s/%s", dir, items[i]->d_name);
                int jitems = scandir(ChangeDirPath, &items, NULL, alphasort);
                for (int j = 0; j < jitems; j++) {
                    // Search only camera directory
                    lstatRet = lstat(items[j]->d_name, &fstat);
                    if (lstatRet != 0)
                        CLOGE("ERR(%s[%d]):lstat returned error", __FUNCTION__, __LINE__);

                    if ((fstat.st_mode & S_IFDIR) == S_IFDIR) {
                        if (!strcmp(items[j]->d_name, ".") || !strcmp(items[j]->d_name, ".."))
                            continue;
                        if (strcasecmp(items[j]->d_name, "CAMERA")==0) {
                            sprintf(m_burstSavePath, "%s/%s/", ChangeDirPath, items[j]->d_name);
                            CLOGW("WARN(%s[%d]) change save path = %s", __FUNCTION__, __LINE__, m_burstSavePath);
                            j = jitems;
                            ret = true;
                            break;
                        }
                    }
                }
                i = iitems;
                break;
            }
        }
    }

    if (items != NULL) {
        free(items);
    }

    return ret;
}

bool ExynosCamera::m_pictureThreadFunc(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    CLOGI("INFO(%s[%d]):", __FUNCTION__, __LINE__);

    int ret = 0;
    int loop = false;
    int bufIndex = -2;
    int retryCountGSC = 4;

    ExynosCameraFrame *newFrame = NULL;

    ExynosCameraBuffer sccReprocessingBuffer;
    ExynosCameraBufferManager *bufferMgr = NULL;
    struct camera2_stream *shot_stream = NULL;
    ExynosRect srcRect, dstRect;
    int pictureW = 0, pictureH = 0, pictureFormat = 0;
    int hwPictureW = 0, hwPictureH = 0, hwPictureFormat = 0;
    int buffer_idx = getShotBufferIdex();
    float zoomRatio = m_parameters->getZoomRatio(0) / 1000;

    sccReprocessingBuffer.index = -2;

    int pipeId_scc = 0;
    int pipeId_gsc = 0;
    bool isSrc = false;

    if (m_parameters->isReprocessing() == true) {
        if (m_parameters->isUseYuvReprocessing() == true)
            pipeId_scc = PIPE_MCSC_REPROCESSING;
        else if (m_parameters->isReprocessing3aaIspOTF() == false)
            pipeId_scc = (m_parameters->isOwnScc(getCameraId()) == true) ? PIPE_SCC_REPROCESSING : PIPE_ISP_REPROCESSING;
        else
            pipeId_scc = PIPE_3AA_REPROCESSING;

        pipeId_gsc = PIPE_GSC_REPROCESSING;
        isSrc = true;
    } else if (m_parameters->isUsing3acForIspc() == true) {
        pipeId_scc = PIPE_3AA;
        pipeId_gsc = PIPE_GSC_PICTURE;
        isSrc = false;
    } else {
        switch (getCameraId()) {
            case CAMERA_ID_FRONT:
                if (m_parameters->getDualMode() == true) {
                    pipeId_scc = PIPE_3AA;
                } else {
                    pipeId_scc = (m_parameters->isOwnScc(getCameraId()) == true) ? PIPE_SCC : PIPE_ISPC;
                }
                pipeId_gsc = PIPE_GSC_PICTURE;
                break;
            default:
                CLOGE("ERR(%s[%d]):Current picture mode is not yet supported, CameraId(%d), reprocessing(%d)",
                    __FUNCTION__, __LINE__, getCameraId(), m_parameters->isReprocessing());
                break;
        }
    }

    /* wait SCC */
    CLOGI("INFO(%s[%d]):wait SCC output", __FUNCTION__, __LINE__);
    int retry = 0;
    do {
        ret = dstSccReprocessingQ->waitAndPopProcessQ(&newFrame);
        retry++;
    } while (ret == TIMED_OUT && retry < 40 &&
             (m_takePictureCounter.getCount() > 0 || m_parameters->getSeriesShotCount() == 0));

    if (ret < 0) {
        CLOGW("WARN(%s[%d]):wait and pop fail, ret(%d), retry(%d), takePictuerCount(%d), seriesShotCount(%d)",
                __FUNCTION__, __LINE__, ret, retry, m_takePictureCounter.getCount(), m_parameters->getSeriesShotCount());
        // TODO: doing exception handling
        goto CLEAN;
    }

    if (newFrame == NULL) {
        CLOGE("ERR(%s[%d]):newFrame is NULL", __FUNCTION__, __LINE__);
        goto CLEAN;
    }

    if (m_postProcessList.size() <= 0) {
        CLOGE("ERR(%s[%d]): postPictureList size(%zd)", __FUNCTION__, __LINE__, m_postProcessList.size());
        usleep(5000);
        if(m_postProcessList.size() <= 0) {
            CLOGE("ERR(%s[%d]):Retry postPictureList size(%zd)", __FUNCTION__, __LINE__, m_postProcessList.size());
            goto CLEAN;
        }
    }

    /*
     * When Non-reprocessing scenario does not setEntityState,
     * because Non-reprocessing scenario share preview and capture frames
     */
    if (m_parameters->isReprocessing() == true) {
        ret = newFrame->setEntityState(pipeId_scc, ENTITY_STATE_COMPLETE);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):setEntityState(ENTITY_STATE_COMPLETE) fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_scc, ret);
            return ret;
        }
    }

    CLOGI("INFO(%s[%d]):SCC output done, frame Count(%d)", __FUNCTION__, __LINE__, newFrame->getFrameCount());

    /* Shutter Callback */
    if (pipeId_scc != PIPE_SCC_REPROCESSING &&
        pipeId_scc != PIPE_ISP_REPROCESSING) {
        if (m_parameters->msgTypeEnabled(CAMERA_MSG_SHUTTER)) {
            CLOGI("INFO(%s[%d]): CAMERA_MSG_SHUTTER callback S", __FUNCTION__, __LINE__);
#ifdef BURST_CAPTURE
            m_notifyCb(CAMERA_MSG_SHUTTER, m_parameters->getSeriesShotDuration(), 0, m_callbackCookie);
#else
            m_notifyCb(CAMERA_MSG_SHUTTER, 0, 0, m_callbackCookie);
#endif
            CLOGI("INFO(%s[%d]): CAMERA_MSG_SHUTTER callback E", __FUNCTION__, __LINE__);
        }
    }

    if (m_parameters->needGSCForCapture(getCameraId()) == true) {
        /* set GSC buffer */
        if (m_parameters->isReprocessing() == true)
            ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_reprocessingFrameFactory->getNodeType(PIPE_ISPC_REPROCESSING));
        else if (m_parameters->isUsing3acForIspc() == true)
            ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_previewFrameFactory->getNodeType(PIPE_3AC));
        else
            ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_previewFrameFactory->getNodeType(PIPE_ISPC));

        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, pipeId_scc, ret);
            goto CLEAN;
        }

        shot_stream = (struct camera2_stream *)(sccReprocessingBuffer.addr[buffer_idx]);
        if (shot_stream != NULL) {
            CLOGD("DEBUG(%s[%d]):(%d %d %d %d)", __FUNCTION__, __LINE__,
                shot_stream->fcount,
                shot_stream->rcount,
                shot_stream->findex,
                shot_stream->fvalid);
            CLOGD("DEBUG(%s[%d]):(%d %d %d %d)(%d %d %d %d)", __FUNCTION__, __LINE__,
                shot_stream->input_crop_region[0],
                shot_stream->input_crop_region[1],
                shot_stream->input_crop_region[2],
                shot_stream->input_crop_region[3],
                shot_stream->output_crop_region[0],
                shot_stream->output_crop_region[1],
                shot_stream->output_crop_region[2],
                shot_stream->output_crop_region[3]);
        } else {
            CLOGE("DEBUG(%s[%d]):shot_stream is NULL", __FUNCTION__, __LINE__);
            goto CLEAN;
        }

        int retry = 0;
        m_getBufferManager(pipeId_gsc, &bufferMgr, DST_BUFFER_DIRECTION);
        do {
            ret = -1;
            retry++;
            if (bufferMgr->getNumOfAvailableBuffer() > 0) {
                ret = m_setupEntity(pipeId_gsc, newFrame, &sccReprocessingBuffer, NULL);
            } else {
                /* wait available SCC buffer */
                usleep(WAITING_TIME);
            }

            if (retry % 10 == 0) {
                CLOGW("WRAN(%s[%d]):retry setupEntity for GSC postPictureQ(%d), saveQ0(%d), saveQ1(%d), saveQ2(%d)",
                        __FUNCTION__, __LINE__,
                        m_postPictureQ->getSizeOfProcessQ(),
                        m_jpegSaveQ[JPEG_SAVE_THREAD0]->getSizeOfProcessQ(),
                        m_jpegSaveQ[JPEG_SAVE_THREAD1]->getSizeOfProcessQ(),
                        m_jpegSaveQ[JPEG_SAVE_THREAD2]->getSizeOfProcessQ());
            }
        } while(ret < 0 && retry < (TOTAL_WAITING_TIME/WAITING_TIME) && m_stopBurstShot == false);

        if (ret < 0) {
            if (retry >= (TOTAL_WAITING_TIME/WAITING_TIME)) {
                CLOGE("ERR(%s[%d]):setupEntity fail, pipeId(%d), retry(%d), ret(%d), m_stopBurstShot(%d)",
                        __FUNCTION__, __LINE__, pipeId_gsc, retry, ret, m_stopBurstShot);
		/* HACK for debugging P150108-08143 */
		bufferMgr->printBufferState();
		android_printAssert(NULL, LOG_TAG, "BURST_SHOT_TIME_ASSERT(%s[%d]): unexpected error, get GSC buffer failed, assert!!!!", __FUNCTION__, __LINE__);
            } else {
                CLOGD("DEBUG(%s[%d]):setupEntity stopped, pipeId(%d), retry(%d), ret(%d), m_stopBurstShot(%d)",
                        __FUNCTION__, __LINE__, pipeId_gsc, retry, ret, m_stopBurstShot);
            }
            goto CLEAN;
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
        pictureFormat = m_parameters->getHwPictureFormat();
#if 1   /* HACK in case of 3AA-OTF-ISP input_cropRegion always 0, use output crop region, check the driver */
        srcRect.x = shot_stream->output_crop_region[0];
        srcRect.y = shot_stream->output_crop_region[1];
        srcRect.w = shot_stream->output_crop_region[2];
        srcRect.h = shot_stream->output_crop_region[3];
#endif
        srcRect.fullW = shot_stream->output_crop_region[2];
        srcRect.fullH = shot_stream->output_crop_region[3];
        srcRect.colorFormat = pictureFormat;

        dstRect.x = 0;
        dstRect.y = 0;
        dstRect.w = pictureW;
        dstRect.h = pictureH;
        dstRect.fullW = pictureW;
        dstRect.fullH = pictureH;
        dstRect.colorFormat = JPEG_INPUT_COLOR_FMT;

        ret = getCropRectAlign(srcRect.w,  srcRect.h,
                               pictureW,   pictureH,
                               &srcRect.x, &srcRect.y,
                               &srcRect.w, &srcRect.h,
                               2, 2, 0, zoomRatio);

        ret = newFrame->setSrcRect(pipeId_gsc, &srcRect);
        ret = newFrame->setDstRect(pipeId_gsc, &dstRect);
#endif

        CLOGD("DEBUG(%s):size (%d, %d, %d, %d %d %d)", __FUNCTION__,
            srcRect.x, srcRect.y, srcRect.w, srcRect.h, srcRect.fullW, srcRect.fullH);
        CLOGD("DEBUG(%s):size (%d, %d, %d, %d %d %d)", __FUNCTION__,
            dstRect.x, dstRect.y, dstRect.w, dstRect.h, dstRect.fullW, dstRect.fullH);

        /* push frame to GSC pipe */
        m_pictureFrameFactory->pushFrameToPipe(&newFrame, pipeId_gsc);
        m_pictureFrameFactory->setOutputFrameQToPipe(dstGscReprocessingQ, pipeId_gsc);

        /* wait GSC */
        newFrame = NULL;
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
        if (m_parameters->isReprocessing() == true) {
            if (m_parameters->isUseYuvReprocessing() == true)
                ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_reprocessingFrameFactory->getNodeType(PIPE_MCSC0_REPROCESSING));
            else
                ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_reprocessingFrameFactory->getNodeType(PIPE_ISPC_REPROCESSING));
        } else if (m_parameters->isUsing3acForIspc() == true) {
            ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_previewFrameFactory->getNodeType(PIPE_3AC));
        } else {
            ret = newFrame->getDstBuffer(pipeId_scc, &sccReprocessingBuffer, m_previewFrameFactory->getNodeType(PIPE_ISPC));
        }

        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_scc, ret);
            goto CLEAN;
        }

        m_getBufferManager(pipeId_scc, &bufferMgr, DST_BUFFER_DIRECTION);
        ret = m_putBuffers(bufferMgr, sccReprocessingBuffer.index);
        if (ret < 0) {
            CLOGE("ERR(%s)(%d):m_putBuffers fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            goto CLEAN;
        }
    }

    /* push postProcess */
    m_postPictureQ->pushProcessQ(&newFrame);

    m_pictureCounter.decCount();

    CLOGI("INFO(%s[%d]):picture thread complete, remaining count(%d)", __FUNCTION__, __LINE__, m_pictureCounter.getCount());

    if (m_pictureCounter.getCount() > 0) {
        loop = true;
    } else {
        if (m_parameters->isReprocessing() == true) {
            if (m_parameters->getReprocessingBayerMode() == REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC) {
                CLOGD("DEBUG(%s[%d]):Use dynamic bayer", __FUNCTION__, __LINE__);
                m_previewFrameFactory->setRequest(PIPE_3AC, false);
            }
        } else {
            if (m_parameters->getUseDynamicScc() == true) {
                CLOGD("DEBUG(%s[%d]): Use dynamic bayer", __FUNCTION__, __LINE__);

                if (m_parameters->isOwnScc(getCameraId()) == true)
                    m_previewFrameFactory->setRequestSCC(false);
                else
                    m_previewFrameFactory->setRequestISPC(false);
            } else if (m_parameters->getRecordingHint() == true
                       && m_parameters->isUsing3acForIspc() == true) {
                CLOGD("DEBUG(%s[%d]): Use dynamic bayer", __FUNCTION__, __LINE__);
                m_previewFrameFactory->setRequest3AC(false);
            }

            if (m_parameters->isUsing3acForIspc() == true)
                m_sccCaptureSelector->clearList(pipeId_scc, isSrc, m_previewFrameFactory->getNodeType(PIPE_3AC));
            else
                m_sccCaptureSelector->clearList(pipeId_scc, isSrc);
        }

        dstSccReprocessingQ->release();
    }

    /* one shot */
    return loop;

CLEAN:
    if (sccReprocessingBuffer.index != -2) {
        CLOGD("DEBUG(%s[%d]): putBuffer sccReprocessingBuffer(index:%d) in error state",
                __FUNCTION__, __LINE__, sccReprocessingBuffer.index);
        m_getBufferManager(pipeId_scc, &bufferMgr, DST_BUFFER_DIRECTION);
        m_putBuffers(bufferMgr, sccReprocessingBuffer.index);
    }

    CLOGI("INFO(%s[%d]):take picture fail, remaining count(%d)", __FUNCTION__, __LINE__, m_pictureCounter.getCount());

    if (m_pictureCounter.getCount() > 0)
        loop = true;

    /* one shot */
    return loop;
}

camera_memory_t *ExynosCamera::m_getJpegCallbackHeap(ExynosCameraBuffer jpegBuf, int seriesShotNumber)
{
    CLOGI("INFO(%s[%d]):", __FUNCTION__, __LINE__);

    camera_memory_t *jpegCallbackHeap = NULL;

#ifdef BURST_CAPTURE
    if (1 < m_parameters->getSeriesShotCount()) {
        int seriesShotSaveLocation = m_parameters->getSeriesShotSaveLocation();

        if (seriesShotNumber < 0 || seriesShotNumber > m_parameters->getSeriesShotCount()) {
             CLOGE("ERR(%s[%d]): Invalid shot number (%d)", __FUNCTION__, __LINE__, seriesShotNumber);
             goto done;
        }

        if (seriesShotSaveLocation == BURST_SAVE_CALLBACK) {
            CLOGD("DEBUG(%s[%d]):burst callback : size (%d), count(%d)", __FUNCTION__, __LINE__, jpegBuf.size[0], seriesShotNumber);

            jpegCallbackHeap = m_getMemoryCb(jpegBuf.fd[0], jpegBuf.size[0], 1, m_callbackCookie);
            if (!jpegCallbackHeap || jpegCallbackHeap->data == MAP_FAILED) {
                CLOGE("ERR(%s[%d]):m_getMemoryCb(%d) fail", __FUNCTION__, __LINE__, jpegBuf.size[0]);
                goto done;
            }
            if (jpegBuf.fd[0] < 0)
                memcpy(jpegCallbackHeap->data, jpegBuf.addr[0], jpegBuf.size[0]);
        } else {
            char filePath[100];
            int nw, cnt = 0;
            uint32_t written = 0;
            camera_memory_t *tempJpegCallbackHeap = NULL;

            memset(filePath, 0, sizeof(filePath));

            snprintf(filePath, sizeof(filePath), "%sBurst%02d.jpg", m_burstSavePath, seriesShotNumber);
            CLOGD("DEBUG(%s[%d]):burst callback : size (%d), filePath(%s)", __FUNCTION__, __LINE__, jpegBuf.size[0], filePath);

            jpegCallbackHeap = m_getMemoryCb(-1, sizeof(filePath), 1, m_callbackCookie);
            if (!jpegCallbackHeap || jpegCallbackHeap->data == MAP_FAILED) {
                CLOGE("ERR(%s[%d]):m_getMemoryCb(%s) fail", __FUNCTION__, __LINE__, filePath);
                goto done;
            }

            memcpy(jpegCallbackHeap->data, filePath, sizeof(filePath));
        }
    } else
#endif
    {
        CLOGD("DEBUG(%s[%d]):general callback : size (%d)", __FUNCTION__, __LINE__, jpegBuf.size[0]);

        jpegCallbackHeap = m_getMemoryCb(jpegBuf.fd[0], jpegBuf.size[0], 1, m_callbackCookie);
        if (!jpegCallbackHeap || jpegCallbackHeap->data == MAP_FAILED) {
            CLOGE("ERR(%s[%d]):m_getMemoryCb(%d) fail", __FUNCTION__, __LINE__, jpegBuf.size[0]);
            goto done;
        }

        if (jpegBuf.fd[0] < 0)
            memcpy(jpegCallbackHeap->data, jpegBuf.addr[0], jpegBuf.size[0]);
    }

done:
    if (jpegCallbackHeap == NULL ||
        jpegCallbackHeap->data == MAP_FAILED) {

        if (jpegCallbackHeap) {
            jpegCallbackHeap->release(jpegCallbackHeap);
            jpegCallbackHeap = NULL;
        }

        m_notifyCb(CAMERA_MSG_ERROR, -1, 0, m_callbackCookie);
    }

    CLOGD("INFO(%s[%d]):making callback buffer done", __FUNCTION__, __LINE__);

    return jpegCallbackHeap;
}

bool ExynosCamera::m_postPictureThreadFunc(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    CLOGI("INFO(%s[%d]):", __FUNCTION__, __LINE__);

    int ret = 0;
    int loop = false;
    int bufIndex = -2;
    int buffer_idx = getShotBufferIdex();
    int retryCountJPEG = 4;

    ExynosCameraFrame *newFrame = NULL;

    ExynosCameraBuffer gscReprocessingBuffer;
    ExynosCameraBuffer jpegReprocessingBuffer;
    ExynosCameraBuffer thumbnailReprocessingBuffer;

    gscReprocessingBuffer.index = -2;
    jpegReprocessingBuffer.index = -2;
    thumbnailReprocessingBuffer.index = -2;

    int pipeId_gsc = 0;
    int pipeId_jpeg = 0;

    int currentSeriesShotMode = 0;

    if (m_parameters->isReprocessing() == true) {
        if (m_parameters->needGSCForCapture(getCameraId()) == true) {
            pipeId_gsc = PIPE_GSC_REPROCESSING;
        } else {
            if (m_parameters->isOwnScc(getCameraId()) == false) {
                if (m_parameters->isUseYuvReprocessing() == true)
                    pipeId_gsc = PIPE_MCSC_REPROCESSING;
                else if (m_parameters->isReprocessing3aaIspOTF() == true)
                    pipeId_gsc = PIPE_3AA_REPROCESSING;
                else
                    pipeId_gsc = PIPE_ISP_REPROCESSING;
            } else {
                pipeId_gsc = PIPE_SCC_REPROCESSING;
            }
        }
        pipeId_jpeg = PIPE_JPEG_REPROCESSING;
    } else {
        if (m_parameters->needGSCForCapture(getCameraId()) == true) {
            pipeId_gsc = PIPE_GSC_PICTURE;
        } else {
            if (m_parameters->isOwnScc(getCameraId()) == true) {
                pipeId_gsc = PIPE_SCC;
            } else {
                if (m_parameters->isUsing3acForIspc() == true)
                    pipeId_gsc = PIPE_3AA;
                else
                    pipeId_gsc = PIPE_ISPC;
            }
        }

        pipeId_jpeg = PIPE_JPEG;
    }

    ExynosCameraBufferManager *bufferMgr = NULL;
    ret = m_getBufferManager(pipeId_gsc, &bufferMgr, DST_BUFFER_DIRECTION);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):getBufferManager(SRC) fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_gsc, ret);
        return ret;
    }

    CLOGI("INFO(%s[%d]):wait postPictureQ output", __FUNCTION__, __LINE__);
    ret = m_postPictureQ->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        CLOGW("WARN(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        goto CLEAN;
    }
    if (newFrame == NULL) {
        CLOGE("ERR(%s[%d]):newFrame is NULL", __FUNCTION__, __LINE__);
        goto CLEAN;
    }

    if (m_jpegCounter.getCount() <= 0) {
        CLOGD("DEBUG(%s[%d]): Picture canceled", __FUNCTION__, __LINE__);
        goto CLEAN;
    }

    CLOGI("INFO(%s[%d]):postPictureQ output done", __FUNCTION__, __LINE__);

    /* put picture callback buffer */
    /* get gsc dst buffers */
    ret = newFrame->getDstBuffer(pipeId_gsc, &gscReprocessingBuffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_gsc, ret);
        goto CLEAN;
    }

    /* callback */
    if (m_hdrEnabled == false && m_parameters->getSeriesShotCount() <= 0) {
        if (m_parameters->msgTypeEnabled(CAMERA_MSG_RAW_IMAGE)) {
            CLOGD("DEBUG(%s[%d]): RAW callabck", __FUNCTION__, __LINE__);
            camera_memory_t *rawCallbackHeap = NULL;
            rawCallbackHeap = m_getMemoryCb(gscReprocessingBuffer.fd[0], gscReprocessingBuffer.size[0], 1, m_callbackCookie);
            if (!rawCallbackHeap || rawCallbackHeap->data == MAP_FAILED) {
                CLOGE("ERR(%s[%d]):m_getMemoryCb(%d) fail", __FUNCTION__, __LINE__, gscReprocessingBuffer.size[0]);
                goto CLEAN;
            }

            setBit(&m_callbackState, CALLBACK_STATE_RAW_IMAGE, true);
            m_dataCb(CAMERA_MSG_RAW_IMAGE, rawCallbackHeap, 0, NULL, m_callbackCookie);
            clearBit(&m_callbackState, CALLBACK_STATE_RAW_IMAGE, true);
            rawCallbackHeap->release(rawCallbackHeap);
        }

        if (m_parameters->msgTypeEnabled(CAMERA_MSG_RAW_IMAGE_NOTIFY)) {
            CLOGD("DEBUG(%s[%d]): RAW_IMAGE_NOTIFY callabck", __FUNCTION__, __LINE__);

            m_notifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0, m_callbackCookie);
        }

        if ((m_parameters->msgTypeEnabled(CAMERA_MSG_POSTVIEW_FRAME))) {
            CLOGD("DEBUG(%s[%d]): POSTVIEW callabck", __FUNCTION__, __LINE__);

            camera_memory_t *postviewCallbackHeap = NULL;
            postviewCallbackHeap = m_getMemoryCb(gscReprocessingBuffer.fd[0], gscReprocessingBuffer.size[0], 1, m_callbackCookie);
            if (!postviewCallbackHeap || postviewCallbackHeap->data == MAP_FAILED) {
                CLOGE("ERR(%s[%d]):m_getMemoryCb(%d) fail", __FUNCTION__, __LINE__, gscReprocessingBuffer.size[0]);
                goto CLEAN;
            }

            setBit(&m_callbackState, CALLBACK_STATE_POSTVIEW_FRAME, true);
            m_dataCb(CAMERA_MSG_POSTVIEW_FRAME, postviewCallbackHeap, 0, NULL, m_callbackCookie);
            clearBit(&m_callbackState, CALLBACK_STATE_POSTVIEW_FRAME, true);
            postviewCallbackHeap->release(postviewCallbackHeap);
        }
    }

    currentSeriesShotMode = m_parameters->getSeriesShotMode();

    /* Make compressed image */
    if (m_parameters->msgTypeEnabled(CAMERA_MSG_COMPRESSED_IMAGE) ||
        m_parameters->getSeriesShotCount() > 0 ||
        m_hdrEnabled == true) {

        /* HDR callback */
        if (m_hdrEnabled == true ||
                currentSeriesShotMode == SERIES_SHOT_MODE_LLS ||
                currentSeriesShotMode == SERIES_SHOT_MODE_SIS ||
                m_parameters->getShotMode() == SHOT_MODE_FRONT_PANORAMA) {
            CLOGD("DEBUG(%s[%d]): HDR callback", __FUNCTION__, __LINE__);

            /* send yuv image with jpeg callback */
            camera_memory_t    *jpegCallbackHeap = NULL;
            jpegCallbackHeap = m_getMemoryCb(gscReprocessingBuffer.fd[0], gscReprocessingBuffer.size[0], 1, m_callbackCookie);
            if (!jpegCallbackHeap || jpegCallbackHeap->data == MAP_FAILED) {
                CLOGE("ERR(%s[%d]):m_getMemoryCb(%d) fail", __FUNCTION__, __LINE__, gscReprocessingBuffer.size[0]);
                goto CLEAN;
            }

            setBit(&m_callbackState, CALLBACK_STATE_COMPRESSED_IMAGE, true);
            m_dataCb(CAMERA_MSG_COMPRESSED_IMAGE, jpegCallbackHeap, 0, NULL, m_callbackCookie);
            clearBit(&m_callbackState, CALLBACK_STATE_COMPRESSED_IMAGE, true);

            jpegCallbackHeap->release(jpegCallbackHeap);

            /* put GSC buffer */
            ret = m_putBuffers(bufferMgr, gscReprocessingBuffer.index);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):bufferMgr->putBuffers() fail, pipeId(%d), ret(%d)",
                        __FUNCTION__, __LINE__, pipeId_gsc, ret);
                goto CLEAN;
            }

            m_jpegCounter.decCount();
        } else {
            if (m_parameters->isHWFCEnabled() == true) {
                /* get gsc dst buffers */
                entity_buffer_state_t jpegMainBufferState = ENTITY_BUFFER_STATE_NOREQ;
                ret = newFrame->getDstBufferState(pipeId_gsc, &jpegMainBufferState,
                                                    m_reprocessingFrameFactory->getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING));
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):getDstBufferState fail, pipeId(%d), nodeType(%d), ret(%d)",
                            __FUNCTION__, __LINE__,
                            pipeId_gsc,
                            m_reprocessingFrameFactory->getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING),
                            ret);
                    goto CLEAN;
                }

                if (jpegMainBufferState == ENTITY_BUFFER_STATE_ERROR) {
                    CLOGE("ERR(%s[%d]):Failed to get the encoded JPEG buffer", __FUNCTION__, __LINE__);
                    goto CLEAN;
                }

                /* put Thumbnail buffer */
                ret = newFrame->getDstBuffer(pipeId_gsc, &thumbnailReprocessingBuffer,
                                                m_reprocessingFrameFactory->getNodeType(PIPE_HWFC_THUMB_SRC_REPROCESSING));
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_gsc, ret);
                    goto CLEAN;
                }

                ret = m_putBuffers(m_thumbnailBufferMgr, thumbnailReprocessingBuffer.index);
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):bufferMgr->putBuffers() fail, pipeId(%d), ret(%d)",
                            __FUNCTION__, __LINE__, pipeId_gsc, ret);
                    goto CLEAN;
                }

                ret = newFrame->getDstBuffer(pipeId_gsc, &jpegReprocessingBuffer,
                                                m_reprocessingFrameFactory->getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING));
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):getDstBuffer fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_gsc, ret);
                    goto CLEAN;
                }

#if 0
                /* in case OTF until JPEG, we should be call below function to this position
                                in order to update debugData from frame. */
                m_parameters->setDebugInfoAttributeFromFrame(udm);
#endif

                /* in case OTF until JPEG, we should overwrite debugData info to Jpeg data. */
                if (jpegReprocessingBuffer.size[0] != 0) {
                    UpdateDebugData(jpegReprocessingBuffer.addr[0],
                                    jpegReprocessingBuffer.size[0],
                                    m_parameters->getDebugAttribute());
                }
            } else {
                int retry = 0;

                /* 1. get wait available JPEG src buffer */
                do {
                    bufIndex = -2;
                    retry++;

                    if (m_pictureEnabled == false) {
                        CLOGI("INFO(%s[%d]):m_pictureEnable is false", __FUNCTION__, __LINE__);
                        goto CLEAN;
                    }
                    if (m_jpegBufferMgr->getNumOfAvailableBuffer() > 0)
                        m_jpegBufferMgr->getBuffer(&bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &jpegReprocessingBuffer);

                    if (bufIndex < 0) {
                        usleep(WAITING_TIME);

                        if (retry % 20 == 0) {
                            CLOGW("WRN(%s[%d]):retry JPEG getBuffer(%d) postPictureQ(%d), saveQ0(%d), saveQ1(%d), saveQ2(%d)",
                                    __FUNCTION__, __LINE__, bufIndex,
                                    m_postPictureQ->getSizeOfProcessQ(),
                                    m_jpegSaveQ[JPEG_SAVE_THREAD0]->getSizeOfProcessQ(),
                                    m_jpegSaveQ[JPEG_SAVE_THREAD1]->getSizeOfProcessQ(),
                                    m_jpegSaveQ[JPEG_SAVE_THREAD2]->getSizeOfProcessQ());
                            m_jpegBufferMgr->dump();
                        }
                    }
                    /* this will retry until 300msec */
                } while (bufIndex < 0 && retry < (TOTAL_WAITING_TIME / WAITING_TIME) && m_stopBurstShot == false);

                if (bufIndex < 0) {
                    if (retry >= (TOTAL_WAITING_TIME / WAITING_TIME)) {
                        CLOGE("ERR(%s[%d]):getBuffer totally fail, retry(%d), m_stopBurstShot(%d)",
                                __FUNCTION__, __LINE__, retry, m_stopBurstShot);
                        /* HACK for debugging P150108-08143 */
                        bufferMgr->printBufferState();
                        android_printAssert(NULL, LOG_TAG, "BURST_SHOT_TIME_ASSERT(%s[%d]): unexpected error, get jpeg buffer failed, assert!!!!", __FUNCTION__, __LINE__);
                    } else {
                        CLOGD("DEBUG(%s[%d]):getBuffer stopped, retry(%d), m_stopBurstShot(%d)",
                                __FUNCTION__, __LINE__, retry, m_stopBurstShot);
                    }
                    ret = m_putBuffers(bufferMgr, gscReprocessingBuffer.index);
                    goto CLEAN;
                }

                /* 2. setup Frame Entity */
                ret = m_setupEntity(pipeId_jpeg, newFrame, &gscReprocessingBuffer, &jpegReprocessingBuffer);
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]):setupEntity fail, pipeId(%d), ret(%d)",
                            __FUNCTION__, __LINE__, pipeId_jpeg, ret);
                    goto CLEAN;
                }

                /* 3. Q Set-up */
                m_pictureFrameFactory->setOutputFrameQToPipe(dstJpegReprocessingQ, pipeId_jpeg);

                /* 4. push the newFrame to pipe */
                m_pictureFrameFactory->pushFrameToPipe(&newFrame, pipeId_jpeg);

                /* 5. wait outputQ */
                CLOGI("INFO(%s[%d]):wait Jpeg output", __FUNCTION__, __LINE__);
                while (retryCountJPEG > 0) {
                    ret = dstJpegReprocessingQ->waitAndPopProcessQ(&newFrame);
                    if (ret == TIMED_OUT) {
                        CLOGW("WRN(%s)(%d):wait and pop timeout, ret(%d)", __FUNCTION__, __LINE__, ret);
                        m_pictureFrameFactory->startThread(pipeId_jpeg);
                    } else if (ret < 0) {
                        CLOGE("ERR(%s)(%d):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                        /* TODO: doing exception handling */
                        goto CLEAN;
                    } else {
                        break;
                    }
                    retryCountJPEG--;
                }

                if (newFrame == NULL) {
                    CLOGE("ERR(%s[%d]):newFrame is NULL", __FUNCTION__, __LINE__);
                    goto CLEAN;
                }

                /*
                 * When Non-reprocessing scenario does not setEntityState,
                 * because Non-reprocessing scenario share preview and capture frames
                 */
                if (m_parameters->isReprocessing() == true) {
                    ret = newFrame->setEntityState(pipeId_jpeg, ENTITY_STATE_COMPLETE);
                    if (ret < 0) {
                        CLOGE("ERR(%s[%d]):setEntityState(ENTITY_STATE_COMPLETE) fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId_jpeg, ret);
                        return ret;
                    }
                }
            }

            /* put GSC buffer */
            ret = m_putBuffers(bufferMgr, gscReprocessingBuffer.index);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):bufferMgr->putBuffers() fail, pipeId(%d), ret(%d)",
                        __FUNCTION__, __LINE__, pipeId_gsc, ret);
                goto CLEAN;
            }

            int jpegOutputSize = newFrame->getJpegSize();
            if (jpegOutputSize <= 0) {
                jpegOutputSize = jpegReprocessingBuffer.size[0];
                if (jpegOutputSize <= 0)
                    CLOGW("WRN(%s[%d]):jpegOutput size(%d) is invalid", __FUNCTION__, __LINE__, jpegOutputSize);
            }

            CLOGI("INFO(%s[%d]):Jpeg output done, jpeg size(%d)", __FUNCTION__, __LINE__, jpegOutputSize);

            jpegReprocessingBuffer.size[0] = jpegOutputSize;

            /* push postProcess to call CAMERA_MSG_COMPRESSED_IMAGE */
            jpeg_callback_buffer_t jpegCallbackBuf;
            jpegCallbackBuf.buffer = jpegReprocessingBuffer;
#ifdef BURST_CAPTURE
            m_burstCaptureCallbackCount++;
            CLOGI("INFO(%s[%d]): burstCaptureCallbackCount(%d)", __FUNCTION__, __LINE__, m_burstCaptureCallbackCount);
#endif
retry:
            if ((m_parameters->getSeriesShotCount() > 0)) {
                int threadNum = 0;

                if (m_burst[JPEG_SAVE_THREAD0] == false && m_jpegSaveThread[JPEG_SAVE_THREAD0]->isRunning() == false) {
                    threadNum = JPEG_SAVE_THREAD0;
                } else if (m_burst[JPEG_SAVE_THREAD1] == false && m_jpegSaveThread[JPEG_SAVE_THREAD1]->isRunning() == false) {
                    threadNum = JPEG_SAVE_THREAD1;
                } else if (m_burst[JPEG_SAVE_THREAD2] == false && m_jpegSaveThread[JPEG_SAVE_THREAD2]->isRunning() == false) {
                    threadNum = JPEG_SAVE_THREAD2;
                } else {
                    CLOGW("WARN(%s[%d]): wait for available save thread, thread running(%d, %d, %d,)",
                            __FUNCTION__, __LINE__,
                            m_jpegSaveThread[JPEG_SAVE_THREAD0]->isRunning(),
                            m_jpegSaveThread[JPEG_SAVE_THREAD1]->isRunning(),
                            m_jpegSaveThread[JPEG_SAVE_THREAD2]->isRunning());
                    usleep(WAITING_TIME * 10);
                    goto retry;
                }

                m_burst[threadNum] = true;
                ret = m_jpegSaveThread[threadNum]->run();
                if (ret < 0) {
                    CLOGE("ERR(%s[%d]): m_jpegSaveThread%d run fail, ret(%d)", __FUNCTION__, __LINE__, threadNum, ret);
                    m_burst[threadNum] = false;
                    m_running[threadNum] = false;
                    goto retry;
                }

                jpegCallbackBuf.callbackNumber = m_burstCaptureCallbackCount;
                m_jpegSaveQ[threadNum]->pushProcessQ(&jpegCallbackBuf);
            } else {
                jpegCallbackBuf.callbackNumber = 0;
                m_jpegCallbackQ->pushProcessQ(&jpegCallbackBuf);
            }

            m_jpegCounter.decCount();
        }
    } else {
        CLOGD("DEBUG(%s[%d]): Disabled compressed image", __FUNCTION__, __LINE__);

        /* put GSC buffer */
        ret = m_putBuffers(bufferMgr, gscReprocessingBuffer.index);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):bufferMgr->putBuffers() fail, pipeId(%d), ret(%d)",
                    __FUNCTION__, __LINE__, pipeId_gsc, ret);
            goto CLEAN;
        }

        m_jpegCounter.decCount();
    }

    if (newFrame != NULL) {
        newFrame->printEntity();
        newFrame->frameUnlock();
        ret = m_removeFrameFromList(&m_postProcessList, newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):remove frame from processList fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        CLOGD("DEBUG(%s[%d]): Picture frame delete(%d)", __FUNCTION__, __LINE__, newFrame->getFrameCount());
        newFrame->decRef();
        m_frameMgr->deleteFrame(newFrame);
        newFrame = NULL;
    }

    CLOGI("INFO(%s[%d]):postPicture thread complete, remaining count(%d)", __FUNCTION__, __LINE__, m_jpegCounter.getCount());

    if (m_jpegCounter.getCount() <= 0) {
        if (m_hdrEnabled == true) {
            CLOGI("INFO(%s[%d]): End of HDR capture!", __FUNCTION__, __LINE__);
            m_hdrEnabled = false;
            m_pictureEnabled = false;
        }
        if (currentSeriesShotMode == SERIES_SHOT_MODE_LLS ||
            currentSeriesShotMode == SERIES_SHOT_MODE_SIS) {
            CLOGI("INFO(%s[%d]): End of LLS/SIS capture!", __FUNCTION__, __LINE__);
            m_pictureEnabled = false;
        }

        if(m_parameters->getShotMode() == SHOT_MODE_FRONT_PANORAMA) {
            CLOGI("INFO(%s[%d]): End of wideselfie capture!", __FUNCTION__, __LINE__);
            m_pictureEnabled = false;
        }

        CLOGD("DEBUG(%s[%d]): free gsc buffers", __FUNCTION__, __LINE__);
        m_gscBufferMgr->resetBuffers();

        if (currentSeriesShotMode != SERIES_SHOT_MODE_BURST) {
            CLOGD("DEBUG(%s[%d]): clearList postProcessList, series shot mode(%d)", __FUNCTION__, __LINE__, currentSeriesShotMode);
            if (m_clearList(&m_postProcessList) < 0) {
                CLOGE("ERR(%s):m_clearList fail", __FUNCTION__);
            }
        }
    }

    if (m_parameters->getScalableSensorMode()) {
        m_scalableSensorMgr.setMode(EXYNOS_CAMERA_SCALABLE_CHANGING);
        ret = m_restartPreviewInternal();
        if (ret < 0)
            CLOGE("(%s[%d]): restart preview internal fail", __FUNCTION__, __LINE__);
        m_scalableSensorMgr.setMode(EXYNOS_CAMERA_SCALABLE_NONE);
    }

CLEAN:
    /* HACK: Sometimes, m_postPictureThread is finished without waiting the last picture */
    int waitCount = 5;
    while (m_postPictureQ->getSizeOfProcessQ() == 0 && 0 < waitCount) {
        usleep(10000);
        waitCount--;
    }

    if (m_postPictureQ->getSizeOfProcessQ() > 0 ||
            currentSeriesShotMode != SERIES_SHOT_MODE_NONE) {
        CLOGD("DEBUG(%s[%d]):postPicture thread will run again.  currentSeriesShotMode(%d), postPictureQ size(%d)",
            __func__, __LINE__, currentSeriesShotMode, m_postPictureQ->getSizeOfProcessQ());
        loop = true;
    }

    return loop;
}

bool ExynosCamera::m_jpegSaveThreadFunc(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    CLOGI("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    int ret = 0;
    int loop = false;
    int curThreadNum = -1;
    char burstFilePath[100];
#ifdef BURST_CAPTURE
    int fd = -1;
#endif

    jpeg_callback_buffer_t jpegCallbackBuf;
    ExynosCameraBuffer jpegSaveBuffer;
    int seriesShotNumber = -1;
//    camera_memory_t *jpegCallbackHeap = NULL;

    for (int threadNum = JPEG_SAVE_THREAD0; threadNum < JPEG_SAVE_THREAD_MAX_COUNT; threadNum++) {
        if (m_burst[threadNum] == true && m_running[threadNum] == false) {
            m_running[threadNum] = true;
            curThreadNum = threadNum;
            if (m_jpegSaveQ[curThreadNum]->waitAndPopProcessQ(&jpegCallbackBuf) < 0) {
                CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                goto done;
            }
            break;
        }
    }

    if (curThreadNum < 0 || curThreadNum > JPEG_SAVE_THREAD2) {
        CLOGE("ERR(%s[%d]): invalid thrad num (%d)", __FUNCTION__, __LINE__, curThreadNum);
        goto done;
    }

    jpegSaveBuffer = jpegCallbackBuf.buffer;
    seriesShotNumber = jpegCallbackBuf.callbackNumber;

#ifdef BURST_CAPTURE
    if (m_parameters->getSeriesShotCount() > 0) {

        int seriesShotSaveLocation = m_parameters->getSeriesShotSaveLocation();

        if (seriesShotSaveLocation == BURST_SAVE_CALLBACK) {
            jpegCallbackBuf.buffer = jpegSaveBuffer;
            jpegCallbackBuf.callbackNumber = 0;
            m_jpegCallbackQ->pushProcessQ(&jpegCallbackBuf);
            goto done;
        } else {
            int nw, cnt = 0;
            uint32_t written = 0;
            camera_memory_t *tempJpegCallbackHeap = NULL;

            memset(burstFilePath, 0, sizeof(burstFilePath));

            m_burstCaptureCallbackCountLock.lock();
            snprintf(burstFilePath, sizeof(burstFilePath), "%sBurst%02d.jpg", m_burstSavePath, seriesShotNumber);
            m_burstCaptureCallbackCountLock.unlock();

            CLOGD("DEBUG(%s[%d]):%s fd:%d jpegSize : %d", __FUNCTION__, __LINE__, burstFilePath, jpegSaveBuffer.fd[0], jpegSaveBuffer.size[0]);

            m_burstCaptureSaveLock.lock();

            fd = open(burstFilePath, O_RDWR | O_CREAT, 0664);
            if (fd < 0) {
                CLOGD("DEBUG(%s[%d]):failed to create file [%s]: %s",
                    __FUNCTION__, __LINE__, burstFilePath, strerror(errno));
                m_burstCaptureSaveLock.unlock();
                goto done;
            }

            m_burstSaveTimer.start();
            CLOGD("DEBUG(%s[%d]):%s jpegSize : %d", __FUNCTION__, __LINE__, burstFilePath, jpegSaveBuffer.size[0]);

           char *data = NULL;

            if (jpegSaveBuffer.fd[0] < 0) {
                data = jpegSaveBuffer.addr[0];
            } else {
                /* TODO : we need to use jpegBuf's buffer directly */
                tempJpegCallbackHeap = m_getMemoryCb(jpegSaveBuffer.fd[0], jpegSaveBuffer.size[0], 1, m_callbackCookie);
                if (!tempJpegCallbackHeap || tempJpegCallbackHeap->data == MAP_FAILED) {
                    CLOGE("ERR(%s[%d]):m_getMemoryCb(fd:%d, size:%d) fail", __FUNCTION__, __LINE__, jpegSaveBuffer.fd[0], jpegSaveBuffer.size[0]);
                    m_burstCaptureSaveLock.unlock();
                    goto done;
                }

                data = (char *)tempJpegCallbackHeap->data;
            }

            CLOGD("DEBUG(%s[%d]):(%s)file write start)", __FUNCTION__, __LINE__, burstFilePath);
            while (written < jpegSaveBuffer.size[0]) {
                nw = ::write(fd, (const char *)(data) + written, jpegSaveBuffer.size[0] - written);

                if (nw < 0) {
                    CLOGD("DEBUG(%s[%d]):failed to write file [%s]: %s",
                        __FUNCTION__, __LINE__, burstFilePath, strerror(errno));
                    break;
                }

                written += nw;
                cnt++;
            }
            CLOGD("DEBUG(%s[%d]):(%s)file write end)", __FUNCTION__, __LINE__, burstFilePath);

            if (fd >= 0)
                ::close(fd);

            if (chmod(burstFilePath,0664) < 0) {
                CLOGE("failed chmod [%s]", burstFilePath);
            }
            if (chown(burstFilePath,AID_MEDIA,AID_MEDIA_RW) < 0) {
                CLOGE("failed chown [%s] user(%d), group(%d)", burstFilePath,AID_MEDIA,AID_MEDIA_RW);
            }

            m_burstCaptureSaveLock.unlock();

            if (tempJpegCallbackHeap) {
                tempJpegCallbackHeap->release(tempJpegCallbackHeap);
                tempJpegCallbackHeap = NULL;
            }

            m_burstSaveTimer.stop();
            m_burstSaveTimerTime = m_burstSaveTimer.durationUsecs();
            if (m_burstSaveTimerTime > (m_burstDuration - 33000)) {
                m_burstDuration += (int)((m_burstSaveTimerTime - m_burstDuration + 33000) / 33000) * 33000;
                CLOGD("Increase burst duration = %d", m_burstDuration);
            }

            CLOGD("DEBUG(%s[%d]):m_burstSaveTimerTime : %d msec, path(%s)", __FUNCTION__, __LINE__, (int)m_burstSaveTimerTime / 1000, burstFilePath);
        }
        jpegCallbackBuf.buffer = jpegSaveBuffer;
        jpegCallbackBuf.callbackNumber = seriesShotNumber;
        m_jpegCallbackQ->pushProcessQ(&jpegCallbackBuf);
    } else
#endif
    {
        jpegCallbackBuf.buffer = jpegSaveBuffer;
        jpegCallbackBuf.callbackNumber = 0;
        m_jpegCallbackQ->pushProcessQ(&jpegCallbackBuf);
    }

done:
/*
    if (jpegCallbackHeap == NULL ||
        jpegCallbackHeap->data == MAP_FAILED) {

        if (jpegCallbackHeap) {
            jpegCallbackHeap->release(jpegCallbackHeap);
            jpegCallbackHeap = NULL;
        }

        m_notifyCb(CAMERA_MSG_ERROR, -1, 0, m_callbackCookie);
    }
*/
    if (JPEG_SAVE_THREAD0 <= curThreadNum && curThreadNum < JPEG_SAVE_THREAD_MAX_COUNT) {
        m_burst[curThreadNum] = false;
        m_running[curThreadNum] = false;
    }

    CLOGI("INFO(%s[%d]):saving jpeg buffer done", __FUNCTION__, __LINE__);

    /* one shot */
    return false;
}

bool ExynosCamera::m_jpegCallbackThreadFunc(void)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    CLOGI("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    int ret = 0;
    int retry = 0, maxRetry = 0;
    int loop = false;
    int seriesShotNumber = -1;

    jpeg_callback_buffer_t jpegCallbackBuf;
    ExynosCameraBuffer jpegCallbackBuffer;
    camera_memory_t *jpegCallbackHeap = NULL;

    jpegCallbackBuffer.index = -2;

    ExynosCameraActivityFlash *m_flashMgr = m_exynosCameraActivityControl->getFlashMgr();
    if (m_flashMgr->getNeedFlash() == true) {
        maxRetry = TOTAL_FLASH_WATING_COUNT;
    } else {
        maxRetry = TOTAL_WAITING_COUNT;
    }

    do {
        ret = m_jpegCallbackQ->waitAndPopProcessQ(&jpegCallbackBuf);
        if (ret < 0) {
            retry++;
            CLOGW("WARN(%s[%d]):jpegCallbackQ pop fail, retry(%d)", __FUNCTION__, __LINE__, retry);
        }
    } while(ret < 0 && retry < maxRetry && m_jpegCounter.getCount() > 0);

    if (ret < 0) {
        CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        loop = true;
        goto CLEAN;
    }

    jpegCallbackBuffer = jpegCallbackBuf.buffer;
    seriesShotNumber = jpegCallbackBuf.callbackNumber;

    CLOGD("DEBUG(%s[%d]):jpeg calllback is start", __FUNCTION__, __LINE__);

    /* Make compressed image */
    if (m_parameters->msgTypeEnabled(CAMERA_MSG_COMPRESSED_IMAGE) ||
        m_parameters->getSeriesShotCount() > 0) {
            m_captureLock.lock();
            camera_memory_t *jpegCallbackHeap = m_getJpegCallbackHeap(jpegCallbackBuffer, seriesShotNumber);
            if (jpegCallbackHeap == NULL) {
                CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                /* TODO: doing exception handling */
                android_printAssert(NULL, LOG_TAG, "Cannot recoverable, assert!!!!");
            }

            setBit(&m_callbackState, CALLBACK_STATE_COMPRESSED_IMAGE, true);
            m_dataCb(CAMERA_MSG_COMPRESSED_IMAGE, jpegCallbackHeap, 0, NULL, m_callbackCookie);
            clearBit(&m_callbackState, CALLBACK_STATE_COMPRESSED_IMAGE, true);
            CLOGD("DEBUG(%s[%d]): CAMERA_MSG_COMPRESSED_IMAGE callabck (%d)", __FUNCTION__, __LINE__, m_burstCaptureCallbackCount);

            /* put JPEG callback buffer */
            if (m_jpegBufferMgr->putBuffer(jpegCallbackBuffer.index, EXYNOS_CAMERA_BUFFER_POSITION_NONE) != NO_ERROR)
                CLOGE("ERR(%s[%d]):putBuffer(%d) fail", __FUNCTION__, __LINE__, jpegCallbackBuffer.index);

            jpegCallbackHeap->release(jpegCallbackHeap);
    } else {
        CLOGD("DEBUG(%s[%d]): Disabled compressed image", __FUNCTION__, __LINE__);
    }

CLEAN:
    CLOGI("INFO(%s[%d]):jpeg callback thread complete, remaining count(%d)", __FUNCTION__, __LINE__, m_takePictureCounter.getCount());
    if (m_takePictureCounter.getCount() == 0) {
        m_pictureEnabled = false;
        loop = false;
        m_clearJpegCallbackThread(true);
        m_captureLock.unlock();
    } else {
        m_captureLock.unlock();
    }

    return loop;
}

void ExynosCamera::m_clearJpegCallbackThread(bool callFromJpeg)
{
    jpeg_callback_buffer_t jpegCallbackBuf;
    ExynosCameraBuffer jpegCallbackBuffer;
    int ret = 0;

    CLOGI("INFO(%s[%d]): takePicture disabled, takePicture callback done takePictureCounter(%d)",
            __FUNCTION__, __LINE__, m_takePictureCounter.getCount());
    m_pictureEnabled = false;

    if (m_parameters->getUseDynamicScc() == true) {
        CLOGD("DEBUG(%s[%d]): Use dynamic bayer", __FUNCTION__, __LINE__);
        if (m_parameters->isOwnScc(getCameraId()) == true)
            m_previewFrameFactory->setRequestSCC(false);
        else
            m_previewFrameFactory->setRequestISPC(false);
    }

    m_prePictureThread->requestExit();
    m_pictureThread->requestExit();
    m_postPictureThread->requestExit();
    m_jpegCallbackThread->requestExit();

    CLOGI("INFO(%s[%d]): wait m_prePictureThrad", __FUNCTION__, __LINE__);
    m_prePictureThread->requestExitAndWait();
    CLOGI("INFO(%s[%d]): wait m_pictureThrad", __FUNCTION__, __LINE__);
    m_pictureThread->requestExitAndWait();
    CLOGI("INFO(%s[%d]): wait m_postPictureThrad", __FUNCTION__, __LINE__);
    m_postPictureThread->requestExitAndWait();
    CLOGI("INFO(%s[%d]): wait m_jpegCallbackThrad", __FUNCTION__, __LINE__);
    if (!callFromJpeg)
        m_jpegCallbackThread->requestExitAndWait();

    for (int threadNum = JPEG_SAVE_THREAD0; threadNum < JPEG_SAVE_THREAD_MAX_COUNT; threadNum++) {
         CLOGI("INFO(%s[%d]): wait m_jpegSaveThrad%d", __FUNCTION__, __LINE__, threadNum);
         m_jpegSaveThread[threadNum]->requestExitAndWait();
    }

    CLOGI("INFO(%s[%d]): All picture threads done", __FUNCTION__, __LINE__);

    if (m_parameters->isReprocessing() == true) {
        enum pipeline pipe = (m_parameters->isOwnScc(getCameraId()) == true) ? PIPE_SCC_REPROCESSING : PIPE_ISP_REPROCESSING;

        m_reprocessingFrameFactory->stopThread(pipe);
    }

    while (m_jpegCallbackQ->getSizeOfProcessQ() > 0) {
        m_jpegCallbackQ->popProcessQ(&jpegCallbackBuf);
        jpegCallbackBuffer = jpegCallbackBuf.buffer;

        CLOGD("DEBUG(%s[%d]):put remaining jpeg buffer(index: %d)", __FUNCTION__, __LINE__, jpegCallbackBuffer.index);
        if (m_jpegBufferMgr->putBuffer(jpegCallbackBuffer.index, EXYNOS_CAMERA_BUFFER_POSITION_NONE) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):putBuffer(%d) fail", __FUNCTION__, __LINE__, jpegCallbackBuffer.index);
        }

        int seriesShotSaveLocation = m_parameters->getSeriesShotSaveLocation();
        char command[100];
        memset(command, 0, sizeof(command));

        snprintf(command, sizeof(command), "rm %sBurst%02d.jpg", m_burstSavePath, jpegCallbackBuf.callbackNumber);

        system(command);
        CLOGD("run %s", command);
    }

    for (int threadNum = JPEG_SAVE_THREAD0; threadNum < JPEG_SAVE_THREAD_MAX_COUNT; threadNum++) {
        while (m_jpegSaveQ[threadNum]->getSizeOfProcessQ() > 0) {
            m_jpegSaveQ[threadNum]->popProcessQ(&jpegCallbackBuf);
            jpegCallbackBuffer = jpegCallbackBuf.buffer;

            CLOGD("DEBUG(%s[%d]):put remaining SaveQ%d jpeg buffer(index: %d)",
                    __FUNCTION__, __LINE__, threadNum, jpegCallbackBuffer.index);
            if (m_jpegBufferMgr->putBuffer(jpegCallbackBuffer.index, EXYNOS_CAMERA_BUFFER_POSITION_NONE) != NO_ERROR) {
                CLOGE("ERR(%s[%d]):putBuffer(%d) fail", __FUNCTION__, __LINE__, jpegCallbackBuffer.index);
            }

        }

            m_burst[threadNum] = false;
        }

    if (m_parameters->isReprocessing() == true) {
        enum pipeline pipe = (m_parameters->isOwnScc(getCameraId()) == true) ? PIPE_SCC_REPROCESSING : PIPE_ISP_REPROCESSING;
        CLOGD("DEBUG(%s[%d]): Wait thread exit Pipe(%d) ", __FUNCTION__, __LINE__, pipe);
        m_reprocessingFrameFactory->stopThreadAndWait(pipe);
    }

    CLOGD("DEBUG(%s[%d]): clear postProcessList", __FUNCTION__, __LINE__);
    if (m_clearList(&m_postProcessList) < 0) {
        CLOGE("ERR(%s):m_clearList fail", __FUNCTION__);
    }

#if 1
    CLOGD("DEBUG(%s[%d]): clear postPictureQ", __FUNCTION__, __LINE__);
    m_postPictureQ->release();

    CLOGD("DEBUG(%s[%d]): clear dstSccReprocessingQ", __FUNCTION__, __LINE__);
    dstSccReprocessingQ->release();

    CLOGD("DEBUG(%s[%d]): clear dstJpegReprocessingQ", __FUNCTION__, __LINE__);
    dstJpegReprocessingQ->release();
#else
    ExynosCameraFrame *frame = NULL;

    CLOGD("DEBUG(%s[%d]): clear postPictureQ", __FUNCTION__, __LINE__);
    while(m_postPictureQ->getSizeOfProcessQ()) {
        m_postPictureQ->popProcessQ(&frame);
        if (frame != NULL) {
            delete frame;
            frame = NULL;
        }
    }

    CLOGD("DEBUG(%s[%d]): clear dstSccReprocessingQ", __FUNCTION__, __LINE__);
    while(dstSccReprocessingQ->getSizeOfProcessQ()) {
        dstSccReprocessingQ->popProcessQ(&frame);
        if (frame != NULL) {
            delete frame;
            frame = NULL;
        }
    }
#endif

    CLOGD("DEBUG(%s[%d]): reset buffer gsc buffers", __FUNCTION__, __LINE__);
    m_gscBufferMgr->resetBuffers();
    CLOGD("DEBUG(%s[%d]): reset buffer jpeg buffers", __FUNCTION__, __LINE__);
    m_jpegBufferMgr->resetBuffers();
    CLOGD("DEBUG(%s[%d]): reset buffer sccReprocessing buffers", __FUNCTION__, __LINE__);
    m_sccReprocessingBufferMgr->resetBuffers();
}

}; /* namespace android */
