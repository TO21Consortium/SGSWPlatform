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
#define LOG_TAG "ExynosCameraRequestManager"

#include "ExynosCameraRequestManager.h"

namespace android {

ExynosCamera3RequestResult::ExynosCamera3RequestResult(uint32_t key, uint32_t frameCount, EXYNOS_REQUEST_RESULT::TYPE type, camera3_capture_result *captureResult, camera3_notify_msg_t *notityMsg)
{
    m_init();

    m_key = key;
    m_type = type;
    m_frameCount = frameCount;

    switch (type) {
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY:
        if (notityMsg == NULL) {
            ALOGE("ERR(%s[%d]):ExynosCamera3RequestResult CALLBACK_NOTIFY_ONLY frameCount(%u) notityMsg is NULL notityMsg(%p) ", __FUNCTION__, __LINE__, frameCount, notityMsg);
        } else {
            memcpy(&m_notityMsg, notityMsg, sizeof(camera3_notify_msg_t));
        }
        break;

    case EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY:
        if (captureResult != NULL) {
            ALOGE("ERR(%s[%d]):ExynosCamera3RequestResult CALLBACK_BUFFER_ONLY frameCount(%u) captureResult is NULL captureResult(%p) ", __FUNCTION__, __LINE__, frameCount, captureResult);
        }
        break;

    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA:
        if (captureResult != NULL) {
            ALOGE("ERR(%s[%d]):ExynosCamera3RequestResult CALLBACK_PARTIAL_3AA frameCount(%u) captureResult is NULL captureResult(%p) ", __FUNCTION__, __LINE__, frameCount, captureResult);
        }
        break;

    case EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT:
        if (captureResult == NULL) {
            ALOGE("ERR(%s[%d]):ExynosCamera3RequestResult CALLBACK_ALL_RESULT frameCount(%u) captureResult is NULL captureResult(%p) ", __FUNCTION__, __LINE__, frameCount, captureResult);
        } else {
            memcpy(&m_captureResult, captureResult, sizeof(camera3_capture_result_t));
        }
        break;

    case EXYNOS_REQUEST_RESULT::CALLBACK_INVALID:
    default:
        ALOGE("ERR(%s[%d]):ExynosCamera3RequestResult type have INVALID value type(%d) frameCount(%u) ", __FUNCTION__, __LINE__, type, frameCount);

        break;
    }

}

ExynosCamera3RequestResult::~ExynosCamera3RequestResult()
{
    m_deinit();
}

status_t ExynosCamera3RequestResult::m_init()
{
    status_t ret = NO_ERROR;
    m_key = 0;
    m_type = EXYNOS_REQUEST_RESULT::CALLBACK_INVALID;
    m_frameCount = 0;
    memset(&m_captureResult, 0x00, sizeof(camera3_capture_result_t));
    memset(&m_notityMsg, 0x00, sizeof(camera3_notify_msg_t));

    m_streamBufferList.clear();

    return ret;
}

status_t ExynosCamera3RequestResult::m_deinit()
{
    status_t ret = NO_ERROR;
    m_key = 0;
    m_type = EXYNOS_REQUEST_RESULT::CALLBACK_INVALID;
    m_frameCount = 0;
    memset(&m_captureResult, 0x00, sizeof(camera3_capture_result_t));
    memset(&m_notityMsg, 0x00, sizeof(camera3_notify_msg_t));

    return ret;
}

uint32_t ExynosCamera3RequestResult::getKey()
{
    return m_key;
}

uint32_t ExynosCamera3RequestResult::getFrameCount()
{
    return m_frameCount;
}

EXYNOS_REQUEST_RESULT::TYPE ExynosCamera3RequestResult::getType()
{
    EXYNOS_REQUEST_RESULT::TYPE ret = EXYNOS_REQUEST_RESULT::CALLBACK_INVALID;
    switch (m_type) {
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY:
    case EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY:
    case EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT:
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA:
        ret = m_type;
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_INVALID:
    default:
        ALOGE("ERR(%s[%d]):getResultType type have INVALID value type(%d) key(%u) frameCount(%u) ", __FUNCTION__, __LINE__, m_type, m_key, m_frameCount);
        break;
    }
    return ret;
}

status_t ExynosCamera3RequestResult::setCaptureRequest(camera3_capture_result_t *captureResult)
{
    memcpy(&m_captureResult, captureResult, sizeof(camera3_capture_result_t));
    return OK;
}

status_t ExynosCamera3RequestResult::getCaptureRequest(camera3_capture_result_t *captureResult)
{
    memcpy(captureResult, &m_captureResult, sizeof(camera3_capture_result_t));
    return OK;
}

status_t ExynosCamera3RequestResult::setNofityRequest(camera3_notify_msg_t *notifyResult)
{
    memcpy(&m_notityMsg, notifyResult, sizeof(camera3_notify_msg_t));
    return OK;
}

status_t ExynosCamera3RequestResult::getNofityRequest(camera3_notify_msg_t *notifyResult)
{
    memcpy(notifyResult, &m_notityMsg, sizeof(camera3_notify_msg_t));
    return OK;
}

status_t ExynosCamera3RequestResult::pushStreamBuffer(camera3_stream_buffer_t *streamBuffer)
{
    status_t ret = NO_ERROR;
    if (streamBuffer == NULL){
        ALOGE("ERR(%s[%d]):pushStreamBuffer is failed streamBuffer == NULL streamBuffer(%p)", __FUNCTION__, __LINE__, streamBuffer);
        ret = INVALID_OPERATION;
        return ret;
    }

    ret = m_pushBuffer(streamBuffer, &m_streamBufferList, &m_streamBufferListLock);
    if (ret < 0){
        ALOGE("ERR(%s[%d]):m_pushBuffer is failed ", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        return ret;
    }

    return ret;
}

status_t ExynosCamera3RequestResult::popStreamBuffer(camera3_stream_buffer_t *streamBuffer)
{
    status_t ret = NO_ERROR;
    if (streamBuffer == NULL){
        ALOGE("ERR(%s[%d]):popStreamBuffer is failed streamBuffer == NULL streamBuffer(%p)", __FUNCTION__, __LINE__, streamBuffer);
        ret = INVALID_OPERATION;
        return ret;
    }

    ret = m_popBuffer(streamBuffer, &m_streamBufferList, &m_streamBufferListLock);
    if (ret < 0){
        ALOGE("ERR(%s[%d]):m_pushBuffer is failed ", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        return ret;
    }

    return ret;
}

int ExynosCamera3RequestResult::getNumOfStreamBuffer()
{
     return m_getNumOfBuffer(&m_streamBufferList, &m_streamBufferListLock);
}

status_t ExynosCamera3RequestResult::m_pushBuffer(camera3_stream_buffer_t *src, StreamBufferList *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    camera3_stream_buffer_t *dst = new camera3_stream_buffer_t;
    memcpy(dst, src, sizeof(camera3_stream_buffer_t));

    lock->lock();
    list->push_back(dst);
    lock->unlock();
    return ret;
}

status_t ExynosCamera3RequestResult::m_popBuffer(camera3_stream_buffer_t *dst, StreamBufferList *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    camera3_stream_buffer_t *src = NULL;

    lock->lock();
    if (list->size() > 0) {
        src = list->front();
        list->pop_front();
        lock->unlock();
    } else {
        lock->unlock();
        ALOGE("ERR(%s[%d]):m_popBuffer failed, size(%zu)", __FUNCTION__, __LINE__, list->size());
        ret = INVALID_OPERATION;
        return ret;
    }

    memcpy(dst, src, sizeof(camera3_stream_buffer_t));
    return ret;
}

int ExynosCamera3RequestResult::m_getNumOfBuffer(StreamBufferList *list, Mutex *lock)
{
    int count = 0;
    lock->lock();
    count = list->size();
    lock->unlock();
    return count;
}
#if 0
ExynosCameraCbRequest::ExynosCameraCbRequest(uint32_t frameCount)
{
    m_init();
    m_frameCount = frameCount;
}

ExynosCameraCbRequest::~ExynosCameraCbRequest()
{
    m_deinit();
}

uint32_t ExynosCameraCbRequest::getFrameCount()
{
    return m_frameCount;
}

status_t ExynosCameraCbRequest::pushRequest(ResultRequest result)
{
    status_t ret = NO_ERROR;
    ret = m_push(result, &m_callbackList, &m_callbackListLock);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):push request is failed, result frameCount(%u) type(%d)", __FUNCTION__, __LINE__, result->getFrameCount(), result->getType());
    }

    return ret;
}

status_t ExynosCameraCbRequest::popRequest(EXYNOS_REQUEST_RESULT::TYPE reqType, ResultRequestList *resultList)
{
    status_t ret = NO_ERROR;

    resultList->clear();

    ret = m_pop(reqType, &m_callbackList, resultList, &m_callbackListLock);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):m_pop request is failed, request type(%d) resultSize(%d)", __FUNCTION__, __LINE__, reqType, resultList->size());
    }

    return ret;
}

status_t ExynosCameraCbRequest::getRequest(EXYNOS_REQUEST_RESULT::TYPE reqType, ResultRequestList *resultList)
{
    status_t ret = NO_ERROR;
    ret = m_get(reqType, &m_callbackList, resultList, &m_callbackListLock);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):m_get request is failed, request type(%d) resultSize(%d)", __FUNCTION__, __LINE__, reqType, resultList->size());
    }

    return ret;
}

status_t ExynosCameraCbRequest::setCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType, bool flag)
{
    status_t ret = NO_ERROR;
    ret = m_setCallbackDone(reqType, flag, &m_statusLock);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):m_get request is failed, request type(%d) ", __FUNCTION__, __LINE__, reqType);
    }

    return ret;
}

bool ExynosCameraCbRequest::getCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType)
{
    bool ret = false;
    ret = m_getCallbackDone(reqType, &m_statusLock);
    return ret;
}

bool ExynosCameraCbRequest::isComplete()
{
    bool ret = false;
    bool notify = false;
    bool capture = false;

    notify = m_getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY, &m_statusLock);
    capture = m_getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT, &m_statusLock);

    if (notify == true && capture == true) {
        ret = true;
    }

    return ret;
}

status_t ExynosCameraCbRequest::m_init()
{
    status_t ret = NO_ERROR;

    m_callbackList.clear();
    for (int i = 0 ; i < EXYNOS_REQUEST_RESULT::CALLBACK_MAX ; i++) {
        m_status[i] = false;
    }
    return ret;
}

status_t ExynosCameraCbRequest::m_deinit()
{
    status_t ret = NO_ERROR;
    ResultRequestListIterator iter;
    ResultRequest result;

    if (m_callbackList.size() > 0) {
        ALOGE("ERR(%s[%d]):delete cb objects, but result size is not ZERO frameCount(%u) result size(%u)", __FUNCTION__, __LINE__, m_frameCount, m_callbackList.size());
        for (iter = m_callbackList.begin(); iter != m_callbackList.end();) {
            result = *iter;
            ALOGE("ERR(%s[%d]):delete cb objects, frameCount(%u) type(%d)", __FUNCTION__, __LINE__, result->getFrameCount(), result->getType());
            m_callbackList.erase(iter++);
            result = NULL;
        }
    }

    m_callbackList.clear();
    for (int i = 0 ; i < EXYNOS_REQUEST_RESULT::CALLBACK_MAX ; i++) {
        m_status[i] = false;
    }

    return ret;
}

status_t ExynosCameraCbRequest::m_push(ResultRequest result, ResultRequestList *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    if (result->getFrameCount() != m_frameCount) {
        ALOGE("ERR(%s[%d]):m_push, check m_frame(%u) getFrameCount(%u)", __FUNCTION__, __LINE__, m_frameCount, result->getFrameCount());
    }

    lock->lock();
    list->push_back(result);
    lock->unlock();
    return ret;
}

status_t ExynosCameraCbRequest::m_pop(EXYNOS_REQUEST_RESULT::TYPE reqType, ResultRequestList *list, ResultRequestList *resultList, Mutex *lock)
{
    status_t ret = NO_ERROR;
    ResultRequestListIterator iter;

    ResultRequest obj;
    lock->lock();

    if (list->size() > 0) {
        for (iter = list->begin() ; iter != list->end() ;) {
            obj = *iter;
            if (obj->getType() == reqType) {
                resultList->push_back(obj);
                list->erase(iter++);
            } else {
                iter++;
            }
        }
    }

    lock->unlock();
    return ret;
}

status_t ExynosCameraCbRequest::m_get(EXYNOS_REQUEST_RESULT::TYPE reqType, ResultRequestList *list, ResultRequestList *resultList, Mutex *lock)
{
    status_t ret = NO_ERROR;
    ResultRequestListIterator iter;

    ResultRequest obj;
    lock->lock();

    if (list->size() > 0) {
        for (iter = list->begin() ; iter != list->end() ;iter++) {
            obj = *iter;
            if (obj->getType() == reqType) {
                resultList->push_back(obj);
            }
        }
    } else {
        obj = NULL;
        ret = INVALID_OPERATION;
        ALOGE("ERR(%s[%d]):m_getCallbackResults failed, size is ZERO, reqType(%d) size(%d)", __FUNCTION__, __LINE__, reqType, list->size());
    }

    lock->unlock();
    return ret;
}

status_t ExynosCameraCbRequest::m_setCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType, bool flag, Mutex *lock)
{
    status_t ret = NO_ERROR;
    if (reqType >= EXYNOS_REQUEST_RESULT::CALLBACK_MAX) {
        ALOGE("ERR(%s[%d]):m_setCallback failed, status erray out of bounded reqType(%d)", __FUNCTION__, __LINE__, reqType);
        ret = INVALID_OPERATION;
        return ret;
    }

    lock->lock();
    m_status[reqType] = flag;
    lock->unlock();
    return ret;
}

bool ExynosCameraCbRequest::m_getCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType, Mutex *lock)
{
    bool ret = false;
    if (reqType >= EXYNOS_REQUEST_RESULT::CALLBACK_MAX) {
        ALOGE("ERR(%s[%d]):m_getCallback failed, status erray out of bounded reqType(%d)", __FUNCTION__, __LINE__, reqType);
        return ret;
    }

    lock->lock();
    ret = m_status[reqType];
    lock->unlock();
    return ret;
}
#endif

ExynosCamera3Request::ExynosCamera3Request(camera3_capture_request_t* request, CameraMetadata previousMeta)
{
    ExynosCameraStream *stream = NULL;
    int streamId = -1;

    m_init();

    m_request = new camera3_capture_request_t;
    memcpy(m_request, request, sizeof(camera3_capture_request_t));
    memset(m_streamIdList, 0x00, sizeof(m_streamIdList));

    /* Deep copy the input buffer object, because the Camera sevice can reuse it
       in successive request with different contents.
    */
    if(request->input_buffer != NULL) {
        ALOGD("DEBUG(%s[%d]):Allocating input buffer (%p)", __FUNCTION__, __LINE__, request->input_buffer);
        m_request->input_buffer = new camera3_stream_buffer_t();
        memcpy(m_request->input_buffer, request->input_buffer, sizeof(camera3_stream_buffer_t));
    }

    m_key = m_request->frame_number;
    m_numOfOutputBuffers = request->num_output_buffers;
    m_isNeedInternalFrame = false;

    /* Deep copy the output buffer objects as well */
    camera3_stream_buffer_t* newOutputBuffers = NULL;
    if((request != NULL) && (request->output_buffers != NULL) && (m_numOfOutputBuffers > 0)) {
        newOutputBuffers = new camera3_stream_buffer_t[m_numOfOutputBuffers];
        memcpy(newOutputBuffers, request->output_buffers, sizeof(camera3_stream_buffer_t) * m_numOfOutputBuffers);
    }
    /* Nasty casting to assign a value to const pointer */
    *(camera3_stream_buffer_t**)(&m_request->output_buffers) = newOutputBuffers;

    for (int i = 0; i < m_numOfOutputBuffers; i++) {
        stream = static_cast<ExynosCameraStream *>(request->output_buffers[i].stream->priv);
        stream->getID(&streamId);
        m_streamIdList[i] = streamId;
    }

    if (request->settings != NULL) {
        m_serviceMeta = request->settings;
        m_resultMeta = request->settings;
    } else {
        ALOGV("DEBUG(%s[%d]):serviceMeta is NULL, use previousMeta", __FUNCTION__, __LINE__);
        if (previousMeta.isEmpty()) {
            ALOGE("ERR(%s[%d]):previous meta is empty, ERROR ", __FUNCTION__, __LINE__);
        } else {
            m_serviceMeta = previousMeta;
            m_resultMeta = previousMeta;
        }
    }

    if (m_serviceMeta.exists(ANDROID_CONTROL_CAPTURE_INTENT)) {
        m_captureIntent = m_resultMeta.find(ANDROID_CONTROL_CAPTURE_INTENT).data.u8[0];
        ALOGV("DEBUG(%s[%d]):ANDROID_CONTROL_CAPTURE_INTENT is (%d)", __FUNCTION__, __LINE__, m_captureIntent);
    }

    ALOGV("DEBUG(%s[%d]):key(%d), request->frame_count(%d), num_output_buffers(%d)",
        __FUNCTION__, __LINE__,
        m_key, request->frame_number, request->num_output_buffers);

/*    m_resultMeta = request->settings;*/
}

ExynosCamera3Request::~ExynosCamera3Request()
{
    m_deinit();
}

uint32_t ExynosCamera3Request::getKey()
{
    return m_key;
}

void ExynosCamera3Request::setFrameCount(uint32_t frameCount)
{
    m_frameCount = frameCount;
}

status_t ExynosCamera3Request::m_init()
{
    status_t ret = NO_ERROR;

    m_key = 0;
    m_request = NULL;
    m_requestId = 0;
    m_captureIntent = 0;
    m_frameCount = 0;
    m_serviceMeta.clear();
    memset(&m_serviceShot, 0x00, sizeof(struct camera2_shot_ext));
    m_resultMeta.clear();
    memset(&m_resultShot, 0x00, sizeof(struct camera2_shot_ext));
    memset(&m_prevShot, 0x00, sizeof(struct camera2_shot_ext));
    m_requestState = EXYNOS_REQUEST::SERVICE;
    m_factoryMap.clear();
    m_resultList.clear();
    m_numOfCompleteBuffers = 0;
    m_numOfDuplicateBuffers = 0;
    m_pipelineDepth = 0;

    for (int i = 0 ; i < EXYNOS_REQUEST_RESULT::CALLBACK_MAX ; i++) {
        m_resultStatus[i] = false;
    }

    return ret;
}

status_t ExynosCamera3Request::m_deinit()
{
    status_t ret = NO_ERROR;

    if (m_request->input_buffer != NULL) {
        delete m_request->input_buffer;
    }
    if (m_request->output_buffers != NULL) {
        delete[] m_request->output_buffers;
    }

    if (m_request != NULL) {
        delete m_request;
    }
    m_request = NULL;

    m_frameCount = 0;
    m_serviceMeta = NULL;
    memset(&m_serviceShot, 0x00, sizeof(struct camera2_shot_ext));
    m_resultMeta = NULL;
    memset(&m_resultShot, 0x00, sizeof(struct camera2_shot_ext));
    memset(&m_prevShot, 0x00, sizeof(struct camera2_shot_ext));
    m_requestState = EXYNOS_REQUEST::SERVICE;
    m_resultList.clear();
    m_numOfCompleteBuffers = 0;
    m_numOfDuplicateBuffers = 0;

    for (int i = 0 ; i < EXYNOS_REQUEST_RESULT::CALLBACK_MAX ; i++) {
        m_resultStatus[i] = false;
    }

    return ret;
}


uint32_t ExynosCamera3Request::getFrameCount()
{
    return m_frameCount;
}

uint8_t ExynosCamera3Request::getCaptureIntent()
{
    return m_captureIntent;
}

camera3_capture_request_t* ExynosCamera3Request::getService()
{
    if (m_request == NULL)
    {
        ALOGE("ERR(%s[%d]):getService is NULL m_request(%p) ", __FUNCTION__, __LINE__, m_request);
    }
    return m_request;
}

uint32_t ExynosCamera3Request::setServiceMeta(CameraMetadata request)
{
    status_t ret = NO_ERROR;
    m_serviceMeta = request;
    return ret;
}

CameraMetadata ExynosCamera3Request::getServiceMeta()
{
    return m_serviceMeta;
}

status_t ExynosCamera3Request::setServiceShot(struct camera2_shot_ext *metadata)
{
    status_t ret = NO_ERROR;
    memcpy(&m_serviceShot, metadata, sizeof(struct camera2_shot_ext));

    return ret;
}

status_t ExynosCamera3Request::getServiceShot(struct camera2_shot_ext *metadata)
{
    status_t ret = NO_ERROR;
    memcpy(metadata, &m_serviceShot, sizeof(struct camera2_shot_ext));

    return ret;
}


status_t ExynosCamera3Request::setResultMeta(CameraMetadata request)
{
    status_t ret = NO_ERROR;
    m_resultMeta = request;
    return ret;
}

CameraMetadata ExynosCamera3Request::getResultMeta()
{
    return m_resultMeta;
}


status_t ExynosCamera3Request::setResultShot(struct camera2_shot_ext *metadata)
{
    status_t ret = NO_ERROR;
    memcpy(&m_resultShot, metadata, sizeof(struct camera2_shot_ext));

    return ret;
}

status_t ExynosCamera3Request::getResultShot(struct camera2_shot_ext *metadata)
{
    status_t ret = NO_ERROR;
    memcpy(metadata, &m_resultShot, sizeof(struct camera2_shot_ext));

    return ret;
}

status_t ExynosCamera3Request::setPrevShot(struct camera2_shot_ext *metadata)
{
    status_t ret = NO_ERROR;
    if (metadata == NULL) {
        ret = INVALID_OPERATION;
        ALOGE("ERR(%s[%d]):setPrevShot metadata is NULL ret(%d) ", __FUNCTION__, __LINE__, ret);
    } else {
        memcpy(&m_prevShot, metadata, sizeof(struct camera2_shot_ext));
    }

    return ret;
}

status_t ExynosCamera3Request::getPrevShot(struct camera2_shot_ext *metadata)
{
    status_t ret = NO_ERROR;
    if (metadata == NULL) {
        ret = INVALID_OPERATION;
        ALOGE("ERR(%s[%d]):getPrevShot metadata is NULL ret(%d) ", __FUNCTION__, __LINE__, ret);
    } else {
        memcpy(metadata, &m_prevShot, sizeof(struct camera2_shot_ext));
    }

    return ret;
}

status_t ExynosCamera3Request::setRequestState(EXYNOS_REQUEST::STATE state)
{
    status_t ret = NO_ERROR;
    switch(state) {
    case EXYNOS_REQUEST::SERVICE:
    case EXYNOS_REQUEST::RUNNING:
        m_requestState = state;
        break;
    default:
        ALOGE("ERR(%s[%d]):setState is invalid newstate(%d) ", __FUNCTION__, __LINE__, state);
        break;
    }

    return ret;
}

EXYNOS_REQUEST::STATE ExynosCamera3Request::getRequestState()
{
    EXYNOS_REQUEST::STATE ret = EXYNOS_REQUEST::INVALID;
    switch(m_requestState) {
    case EXYNOS_REQUEST::SERVICE:
    case EXYNOS_REQUEST::RUNNING:
        ret = m_requestState;
        break;
    default:
        ALOGE("ERR(%s[%d]):getState is invalid curstate(%d) ", __FUNCTION__, __LINE__, m_requestState);
        break;
    }

    return ret;
}

uint32_t ExynosCamera3Request::getNumOfInputBuffer()
{
    uint32_t numOfInputBuffer = 0;
    if (m_request->input_buffer != NULL) {
        numOfInputBuffer = 1;
    }
    return numOfInputBuffer;
}

camera3_stream_buffer_t* ExynosCamera3Request::getInputBuffer()
{
    if (m_request == NULL){
        ALOGE("ERR(%s[%d]):getInputBuffer m_request is NULL m_request(%p) ", __FUNCTION__, __LINE__, m_request);
        return NULL;
    }

    if (m_request->input_buffer == NULL){
        ALOGV("INFO(%s[%d]):getInputBuffer input_buffer is NULL m_request(%p) ", __FUNCTION__, __LINE__, m_request->input_buffer);
    }

    return m_request->input_buffer;
}

uint64_t ExynosCamera3Request::getSensorTimestamp()
{
    return m_resultShot.shot.udm.sensor.timeStampBoot;
}

uint32_t ExynosCamera3Request::getNumOfOutputBuffer()
{
    return m_numOfOutputBuffers;
}

void ExynosCamera3Request::increaseCompleteBufferCount(void)
{
    m_resultStatusLock.lock();
    m_numOfCompleteBuffers++;
    m_resultStatusLock.unlock();
}

void ExynosCamera3Request::resetCompleteBufferCount(void)
{
    m_resultStatusLock.lock();
    m_numOfCompleteBuffers = 0;
    m_resultStatusLock.unlock();
}

int ExynosCamera3Request::getCompleteBufferCount(void)
{
     return m_numOfCompleteBuffers;
}

void ExynosCamera3Request::increaseDuplicateBufferCount(void)
{
    m_resultStatusLock.lock();
    m_numOfDuplicateBuffers++;
    m_resultStatusLock.unlock();
}

void ExynosCamera3Request::resetDuplicateBufferCount(void)
{
    m_resultStatusLock.lock();
    m_numOfDuplicateBuffers = 0;
    m_resultStatusLock.unlock();
}
int ExynosCamera3Request::getDuplicateBufferCount(void)
{
     return m_numOfDuplicateBuffers;
}

const camera3_stream_buffer_t* ExynosCamera3Request::getOutputBuffers()
{
    if (m_request == NULL){
        ALOGE("ERR(%s[%d]):getNumOfOutputBuffer m_request is NULL m_request(%p) ", __FUNCTION__, __LINE__, m_request);
        return NULL;
    }

    if (m_request->output_buffers == NULL){
        ALOGE("ERR(%s[%d]):getNumOfOutputBuffer output_buffers is NULL m_request(%p) ", __FUNCTION__, __LINE__, m_request->output_buffers);
        return NULL;
    }

    ALOGV("DEBUG(%s[%d]):m_request->output_buffers(%p)", __FUNCTION__, __LINE__, m_request->output_buffers);

    return m_request->output_buffers;
}

status_t ExynosCamera3Request::pushResult(ResultRequest result)
{
    status_t ret = NO_ERROR;

    switch(result->getType()) {
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY:
    case EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY:
    case EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT:
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA:
        ret = m_pushResult(result, &m_resultList, &m_resultListLock);
        if (ret < 0){
            ALOGE("ERR(%s[%d]):pushResult is failed request - Key(%u) frameCount(%u) /  result - Key(%u) frameCount(%u)", __FUNCTION__, __LINE__, m_key, m_frameCount, result->getKey(), result->getFrameCount());
            ret = INVALID_OPERATION;
        }
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_INVALID:
    default:
        ret = INVALID_OPERATION;
        ALOGE("ERR(%s[%d]):getResultType type have INVALID value type(%d) key(%u) frameCount(%u)", __FUNCTION__, __LINE__, result->getType(), m_key, m_frameCount);
        break;
    }


    return ret;
}

void ExynosCamera3Request::setRequestId(int reqId) {
    m_requestId = reqId;
}

int ExynosCamera3Request::getRequestId(void) {
    return m_requestId;
}

ResultRequest ExynosCamera3Request::popResult(uint32_t resultKey)
{
    ResultRequest result = NULL;

    result = m_popResult(resultKey, &m_resultList, &m_resultListLock);
    if (result < 0){
        ALOGE("ERR(%s[%d]):popResult is failed request - Key(%u) frameCount(%u) /  result - Key(%u) frameCount(%u)",
            __FUNCTION__, __LINE__, m_key, m_frameCount, result->getKey(), result->getFrameCount());
        result = NULL;
    }

    return result;
}

ResultRequest ExynosCamera3Request::getResult(uint32_t resultKey)
{
    ResultRequest result = NULL;

    result = m_getResult(resultKey, &m_resultList, &m_resultListLock);
    if (result < 0){
        ALOGE("ERR(%s[%d]):popResult is failed request - Key(%u) frameCount(%u) /  result - Key(%u) frameCount(%u)",
            __FUNCTION__, __LINE__, m_key, m_frameCount, result->getKey(), result->getFrameCount());
        result = NULL;
    }

    return result;
}

status_t ExynosCamera3Request::m_pushResult(ResultRequest item, ResultRequestMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<ResultRequestMap::iterator,bool> listRet;
    lock->lock();
    listRet = list->insert( pair<uint32_t, ResultRequest>(item->getKey(), item));
    if (listRet.second == false) {
        ret = INVALID_OPERATION;
        ALOGE("ERR(%s[%d]):m_push failed, request already exist!! Request frameCnt( %d )",
            __FUNCTION__, __LINE__, item->getFrameCount());
    }
    lock->unlock();

    return ret;
}

ResultRequest ExynosCamera3Request::m_popResult(uint32_t key, ResultRequestMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<ResultRequestMap::iterator,bool> listRet;
    ResultRequestMapIterator iter;
    ResultRequest request = NULL;

    lock->lock();
    iter = list->find(key);
    if (iter != list->end()) {
        request = iter->second;
        list->erase( iter );
    } else {
        ALOGE("ERR(%s[%d]):m_pop failed, request is not EXIST Request frameCnt( %d )",
            __FUNCTION__, __LINE__, request->getFrameCount());
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return request;
}

ResultRequest ExynosCamera3Request::m_getResult(uint32_t key, ResultRequestMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<ResultRequestMap::iterator,bool> listRet;
    ResultRequestMapIterator iter;
    ResultRequest request = NULL;

    lock->lock();
    iter = list->find(key);
    if (iter != list->end()) {
        request = iter->second;
    } else {
        ALOGE("ERR(%s[%d]):m_getResult failed, request is not EXIST Request frameCnt( %d )",
            __FUNCTION__, __LINE__, request->getFrameCount());
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return request;
}

status_t ExynosCamera3Request::m_getAllResultKeys(ResultRequestkeys *keylist, ResultRequestMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    ResultRequestMapIterator iter;

    lock->lock();
    for (iter = list->begin(); iter != list->end() ; iter++) {
        keylist->push_back(iter->first);
    }
    lock->unlock();
    return ret;
}

status_t ExynosCamera3Request::m_getResultKeys(ResultRequestkeys *keylist, ResultRequestMap *list, EXYNOS_REQUEST_RESULT::TYPE type, Mutex *lock)
{
    status_t ret = NO_ERROR;
    ResultRequestMapIterator iter;
    ResultRequest result;
    camera3_capture_result_t capture_result;

    lock->lock();
    /* validation check */
    if ((type < EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY) ||
        (type >= EXYNOS_REQUEST_RESULT::CALLBACK_MAX)) {
        ALOGE("ERR(%s[%d]):INVALID value type(%d)", __FUNCTION__, __LINE__, type);
        lock->unlock();
        return BAD_VALUE;
    }

    for (iter = list->begin(); iter != list->end() ; iter++) {
        result = iter->second;

        ALOGV("DEBUG(%s[%d]):result->getKey(%d)", __FUNCTION__, __LINE__, result->getKey());

        if (type == result->getType())
            keylist->push_back(iter->first);
    }
    lock->unlock();

    return ret;
}

status_t ExynosCamera3Request::m_push(int key, ExynosCamera3FrameFactory* item, FrameFactoryMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<FrameFactoryMap::iterator,bool> listRet;
    lock->lock();
    listRet = list->insert( pair<uint32_t, ExynosCamera3FrameFactory*>(key, item));
    if (listRet.second == false) {
        ret = INVALID_OPERATION;
        ALOGE("ERR(%s[%d]):m_push failed, request already exist!! Request frameCnt( %d )",
            __FUNCTION__, __LINE__, key);
    }
    lock->unlock();

    return ret;
}

status_t ExynosCamera3Request::m_pop(int key, ExynosCamera3FrameFactory** item, FrameFactoryMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<FrameFactoryMap::iterator,bool> listRet;
    FrameFactoryMapIterator iter;
    ExynosCamera3FrameFactory *factory = NULL;

    lock->lock();
    iter = list->find(key);
    if (iter != list->end()) {
        factory = iter->second;
        *item = factory;
        list->erase( iter );
    } else {
        ALOGE("ERR(%s[%d]):m_pop failed, factory is not EXIST Request frameCnt( %d )",
            __FUNCTION__, __LINE__, key);
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return ret;
}

status_t ExynosCamera3Request::m_get(int streamID, ExynosCamera3FrameFactory** item, FrameFactoryMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<FrameFactoryMap::iterator,bool> listRet;
    FrameFactoryMapIterator iter;
    ExynosCamera3FrameFactory *factory = NULL;

    lock->lock();
    iter = list->find(streamID);
    if (iter != list->end()) {
        factory = iter->second;
        *item = factory;
    } else {
        ALOGE("ERR(%s[%d]):m_pop failed, request is not EXIST Request streamID( %d )",
            __FUNCTION__, __LINE__, streamID);
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return ret;
}

bool ExynosCamera3Request::m_find(int streamID, FrameFactoryMap *list, Mutex *lock)
{
    bool ret = false;
    pair<FrameFactoryMap::iterator,bool> listRet;
    FrameFactoryMapIterator iter;

    lock->lock();
    iter = list->find(streamID);
    if (iter != list->end()) {
        ret = true;
    }
    lock->unlock();

    return ret;
}

status_t ExynosCamera3Request::m_getList(FrameFactoryList *factorylist, FrameFactoryMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    FrameFactoryMapIterator iter;
    ExynosCamera3FrameFactory *factory = NULL;

    lock->lock();
    for (iter = list->begin(); iter != list->end() ; iter++) {
        factory = iter->second;
        factorylist->push_back(factory);
    }
    lock->unlock();

    return ret;
}

status_t ExynosCamera3Request::getAllResultKeys(ResultRequestkeys *keys)
{
    status_t ret = NO_ERROR;

    keys->clear();

    ret = m_getAllResultKeys(keys, &m_resultList, &m_resultListLock);
    if (ret < 0){
        ALOGE("ERR(%s[%d]):m_getAllResultKeys is failed Request-Key(%u) frameCount(%u) / m_resultList.Size(%zu)",
            __FUNCTION__, __LINE__, m_key, m_frameCount, m_resultList.size());
    }

    return ret;
}

status_t ExynosCamera3Request::getResultKeys(ResultRequestkeys *keys, EXYNOS_REQUEST_RESULT::TYPE type)
{
    status_t ret = NO_ERROR;

    keys->clear();

    ret = m_getResultKeys(keys, &m_resultList, type, &m_resultListLock);
    if (ret < 0){
        ALOGE("ERR(%s[%d]):getResultKeys is failed Request-Key(%u) frameCount(%u) / m_resultList.Size(%zu)",
            __FUNCTION__, __LINE__, m_key, m_frameCount, m_resultList.size());
    }

    return ret;
}

status_t ExynosCamera3Request::pushFrameFactory(int StreamID, ExynosCamera3FrameFactory* factory)
{
    status_t ret = NO_ERROR;
    ret = m_push(StreamID, factory, &m_factoryMap, &m_factoryMapLock);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):pushFrameFactory is failed StreamID(%d) factory(%p)",
            __FUNCTION__, __LINE__, StreamID, factory);
    }

    return ret;
}

ExynosCamera3FrameFactory* ExynosCamera3Request::popFrameFactory(int streamID)
{
    status_t ret = NO_ERROR;
    ExynosCamera3FrameFactory* factory = NULL;

    ret = m_pop(streamID, &factory, &m_factoryMap, &m_factoryMapLock);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):popFrameFactory is failed StreamID(%d) factory(%p)",
            __FUNCTION__, __LINE__, streamID, factory);
    }
    return factory;
}

ExynosCamera3FrameFactory* ExynosCamera3Request::getFrameFactory(int streamID)
{
    status_t ret = NO_ERROR;
    ExynosCamera3FrameFactory* factory = NULL;

    ret = m_get(streamID, &factory, &m_factoryMap, &m_factoryMapLock);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):getFrameFactory is failed StreamID(%d) factory(%p)",
            __FUNCTION__, __LINE__, streamID, factory);
    }

    return factory;
}

bool ExynosCamera3Request::isFrameFactory(int streamID)
{
    return m_find(streamID, &m_factoryMap, &m_factoryMapLock);
}

status_t ExynosCamera3Request::getFrameFactoryList(FrameFactoryList *list)
{
    status_t ret = NO_ERROR;

    ret = m_getList(list, &m_factoryMap, &m_factoryMapLock);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):getFrameFactoryList is failed", __FUNCTION__, __LINE__);
    }

    return ret;
}

status_t ExynosCamera3Request::getAllRequestOutputStreams(List<int> **list)
{
    status_t ret = NO_ERROR;

    ALOGV("DEBUG (%s[%d]):m_requestOutputStreamList.size(%zu)",
        __FUNCTION__, __LINE__, m_requestOutputStreamList.size());

    /* lock->lock(); */
    *list = &m_requestOutputStreamList;
    /* lock->unlock(); */

    return ret;
}

status_t ExynosCamera3Request::pushRequestOutputStreams(int requestStreamId)
{
    status_t ret = NO_ERROR;

    /* lock->lock(); */
    m_requestOutputStreamList.push_back(requestStreamId);
    /* lock->unlock(); */

    return ret;
}

status_t ExynosCamera3Request::getAllRequestInputStreams(List<int> **list)
{
    status_t ret = NO_ERROR;

    ALOGV("DEBUG (%s[%d]):m_requestOutputStreamList.size(%zu)", __FUNCTION__, __LINE__, m_requestInputStreamList.size());

    /* lock->lock(); */
    *list = &m_requestInputStreamList;
    /* lock->unlock(); */

    return ret;
}

status_t ExynosCamera3Request::pushRequestInputStreams(int requestStreamId)
{
    status_t ret = NO_ERROR;

    /* lock->lock(); */
    m_requestInputStreamList.push_back(requestStreamId);
    /* lock->unlock(); */

    return ret;
}

status_t ExynosCamera3Request::popAndEraseResultsByType(EXYNOS_REQUEST_RESULT::TYPE reqType, ResultRequestList *resultList)
{
    status_t ret = NO_ERROR;

    resultList->clear();

    ret = m_popAndEraseResultsByType(reqType, &m_resultList, resultList, &m_resultListLock);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):m_pop request is failed, request type(%d) resultSize(%zu)",
            __FUNCTION__, __LINE__, reqType, resultList->size());
    }

    return ret;
}

status_t ExynosCamera3Request::popResultsByType(EXYNOS_REQUEST_RESULT::TYPE reqType, ResultRequestList *resultList)
{
    status_t ret = NO_ERROR;

    resultList->clear();

    ret = m_popResultsByType(reqType, &m_resultList, resultList, &m_resultListLock);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):m_pop request is failed, request type(%d) resultSize(%zu)",
            __FUNCTION__, __LINE__, reqType, resultList->size());
    }

    return ret;
}

status_t ExynosCamera3Request::setCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType, bool flag)
{
    status_t ret = NO_ERROR;
    ret = m_setCallbackDone(reqType, flag, &m_resultStatusLock);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):m_get request is failed, request type(%d) ", __FUNCTION__, __LINE__, reqType);
    }

    return ret;
}

bool ExynosCamera3Request::getCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType)
{
    bool ret = false;
    ret = m_getCallbackDone(reqType, &m_resultStatusLock);
    return ret;
}

bool ExynosCamera3Request::isComplete()
{
    bool ret = false;
    bool notify = false;
    bool capture = false;

    notify = m_getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY, &m_resultStatusLock);
    capture = m_getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT, &m_resultStatusLock);

    if (notify == true && capture == true) {
        ret = true;
    }

    return ret;
}

int ExynosCamera3Request::getStreamId(int bufferIndex)
{
    if (bufferIndex < 0 || bufferIndex >= m_numOfOutputBuffers) {
        ALOGE("ERR(%s[%d]):Invalid buffer index(%d), outputBufferCount(%d)",
                __FUNCTION__, __LINE__, bufferIndex, m_numOfOutputBuffers);
        return -1;
    }

    if (m_streamIdList == NULL) {
        ALOGE("ERR(%s[%d]):m_streamIdList is NULL", __FUNCTION__, __LINE__);
        return -1;
    }

    return m_streamIdList[bufferIndex];
}

void ExynosCamera3Request::setNeedInternalFrame(bool isNeedInternalFrame)
{
    m_isNeedInternalFrame = isNeedInternalFrame;
}

bool ExynosCamera3Request::getNeedInternalFrame(void)
{
    return m_isNeedInternalFrame;
}

void ExynosCamera3Request::increasePipelineDepth(void)
{
    m_pipelineDepth++;
}

void ExynosCamera3Request::updatePipelineDepth(void)
{
    const uint8_t pipelineDepth = m_pipelineDepth;

    m_resultShot.shot.dm.request.pipelineDepth = m_pipelineDepth;
    m_resultMeta.update(ANDROID_REQUEST_PIPELINE_DEPTH, &pipelineDepth, 1);
    ALOGV("DEBUG(%s):ANDROID_REQUEST_PIPELINE_DEPTH(%d)", __FUNCTION__, pipelineDepth);
}

status_t ExynosCamera3Request::m_setCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType, bool flag, Mutex *lock)
{
    status_t ret = NO_ERROR;
    if (reqType >= EXYNOS_REQUEST_RESULT::CALLBACK_MAX) {
        ALOGE("ERR(%s[%d]):m_setCallback failed, status erray out of bounded reqType(%d)",
            __FUNCTION__, __LINE__, reqType);

        ret = INVALID_OPERATION;
        return ret;
    }

    lock->lock();
    m_resultStatus[reqType] = flag;
    lock->unlock();
    return ret;
}

bool ExynosCamera3Request::m_getCallbackDone(EXYNOS_REQUEST_RESULT::TYPE reqType, Mutex *lock)
{
    bool ret = false;
    if (reqType >= EXYNOS_REQUEST_RESULT::CALLBACK_MAX) {
        ALOGE("ERR(%s[%d]):m_getCallback failed, status erray out of bounded reqType(%d)",
            __FUNCTION__, __LINE__, reqType);

        return ret;
    }

    lock->lock();
    ret = m_resultStatus[reqType];
    lock->unlock();
    return ret;
}

void ExynosCamera3Request::printCallbackDoneState()
{
    for (int i = 0 ; i < EXYNOS_REQUEST_RESULT::CALLBACK_MAX ; i++)
        ALOGD("DEBUG(%s[%d]):m_key(%d), m_resultStatus[%d](%d)",
            __FUNCTION__, __LINE__, m_key, i, m_resultStatus[i]);
}

status_t ExynosCamera3Request::m_popAndEraseResultsByType(EXYNOS_REQUEST_RESULT::TYPE reqType,
                                                            ResultRequestMap *list,
                                                            ResultRequestList *resultList,
                                                            Mutex *lock)
{
    status_t ret = NO_ERROR;
    ResultRequestMapIterator iter;
    ResultRequest obj;

    lock->lock();

    if (list->size() > 0) {
        for (iter = list->begin(); iter != list->end();) {
            obj = iter->second;
            if (obj->getType() == reqType) {
                resultList->push_back(obj);
                list->erase(iter++);
            } else {
                ++iter;
            }
        }
    }

    lock->unlock();

    return ret;
}

status_t ExynosCamera3Request::m_popResultsByType(EXYNOS_REQUEST_RESULT::TYPE reqType,
                                                    ResultRequestMap *list,
                                                    ResultRequestList *resultList,
                                                    Mutex *lock)
{
    status_t ret = NO_ERROR;
    ResultRequestMapIterator iter;
    ResultRequest obj;

    lock->lock();

    if (list->size() > 0) {
        for (iter = list->begin(); iter != list->end(); iter++) {
            obj = iter->second;
            if (obj->getType() == reqType) {
                resultList->push_back(obj);
            }
        }
    }

    lock->unlock();

    return ret;
}

ExynosCameraRequestManager::ExynosCameraRequestManager(int cameraId, ExynosCameraParameters *param)
{
    CLOGD("DEBUG(%s[%d])Create-ID(%d)", __FUNCTION__, __LINE__, cameraId);

    m_cameraId = cameraId;
    m_parameters = param;
    m_converter = NULL;
    m_callbackOps = NULL;
    m_requestKey = 0;
    m_requestResultKey = 0;
    memset(&m_dummyShot, 0x00, sizeof(struct camera2_shot_ext));
    memset(&m_currShot, 0x00, sizeof(struct camera2_shot_ext));
    memset(m_name, 0x00, sizeof(m_name));

    for (int i = 0; i < CAMERA3_TEMPLATE_COUNT; i++)
        m_defaultRequestTemplate[i] = NULL;

    m_factoryMap.clear();
    m_zslFactoryMap.clear();

    m_callbackSequencer = new ExynosCameraCallbackSequencer();

    m_preShot = NULL;
    m_preShot = new struct camera2_shot_ext;
    m_callbackTraceCount = 0;
}

ExynosCameraRequestManager::~ExynosCameraRequestManager()
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    memset(&m_dummyShot, 0x00, sizeof(struct camera2_shot_ext));
    memset(&m_currShot, 0x00, sizeof(struct camera2_shot_ext));

    for (int i = 0; i < CAMERA3_TEMPLATE_COUNT; i++)
        free(m_defaultRequestTemplate[i]);

    for (int i = 0 ; i < EXYNOS_REQUEST_TYPE::MAX ; i++) {
        m_serviceRequests[i].clear();
        m_runningRequests[i].clear();
    }

    m_requestFrameCountMap.clear();

    if (m_converter != NULL) {
        delete m_converter;
        m_converter = NULL;
    }

    m_factoryMap.clear();
    m_zslFactoryMap.clear();

    if (m_callbackSequencer != NULL) {
        delete m_callbackSequencer;
        m_callbackSequencer= NULL;
    }

    if (m_preShot != NULL) {
        delete m_preShot;
        m_preShot = NULL;
    }
    m_callbackTraceCount = 0;
}

status_t ExynosCameraRequestManager::setMetaDataConverter(ExynosCameraMetadataConverter *converter)
{
    status_t ret = NO_ERROR;
    if (m_converter != NULL)
        CLOGD("DEBUG(%s[%d]):m_converter is not NULL(%p)", __FUNCTION__, __LINE__, m_converter);

    m_converter = converter;
    return ret;
}

ExynosCameraMetadataConverter* ExynosCameraRequestManager::getMetaDataConverter()
{
    if (m_converter == NULL)
        CLOGD("DEBUG(%s[%d]):m_converter is NULL(%p)", __FUNCTION__, __LINE__, m_converter);

    return m_converter;
}

status_t ExynosCameraRequestManager::setRequestsInfo(int key, ExynosCamera3FrameFactory *factory, ExynosCamera3FrameFactory *zslFactory)
{
    status_t ret = NO_ERROR;
    if (factory == NULL) {
        CLOGE("ERR(%s[%d]):m_factory is NULL key(%d) factory(%p)",
            __FUNCTION__, __LINE__, key, factory);

        ret = INVALID_OPERATION;
        return ret;
    }
    /* zslFactory can be NULL. In this case, use factory insted.
       (Same frame factory for both normal capture and ZSL input)
    */
    if (zslFactory == NULL) {
        zslFactory = factory;
    }

    ret = m_pushFactory(key ,factory, &m_factoryMap, &m_factoryMapLock);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_pushFactory is failed key(%d) factory(%p)",
            __FUNCTION__, __LINE__, key, factory);

        ret = INVALID_OPERATION;
        return ret;
    }
    ret = m_pushFactory(key ,zslFactory, &m_zslFactoryMap, &m_zslFactoryMapLock);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_pushFactory is failed key(%d) zslFactory(%p)",
            __FUNCTION__, __LINE__, key, factory);

        ret = INVALID_OPERATION;
        return ret;
    }

    return ret;
}

ExynosCamera3FrameFactory* ExynosCameraRequestManager::getFrameFactory(int key)
{
    status_t ret = NO_ERROR;
    ExynosCamera3FrameFactory *factory = NULL;
    if (key < 0) {
        CLOGE("ERR(%s[%d]):getFrameFactory, type is invalid key(%d)",
            __FUNCTION__, __LINE__, key);

        ret = INVALID_OPERATION;
        return NULL;
    }

    ret = m_popFactory(key ,&factory, &m_factoryMap, &m_factoryMapLock);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_pushFactory is failed key(%d) factory(%p)",
            __FUNCTION__, __LINE__, key, factory);

        ret = INVALID_OPERATION;
        return NULL;
    }
    return factory;
}

status_t ExynosCameraRequestManager::isPrevRequest()
{
    if (m_previousMeta.isEmpty())
        return BAD_VALUE;
    else
        return OK;
}

status_t ExynosCameraRequestManager::clearPrevRequest()
{
    m_previousMeta.clear();
    return OK;
}

status_t ExynosCameraRequestManager::clearPrevShot()
{
    status_t ret = NO_ERROR;
    if (m_preShot == NULL) {
        ret = INVALID_OPERATION;
        CLOGE("ERR(%s[%d]):clearPrevShot previous meta is NULL ret(%d) ",
            __FUNCTION__, __LINE__, ret);
    } else {
        memset(m_preShot, 0x00, sizeof(struct camera2_shot_ext));
    }
    return ret;
}

status_t ExynosCameraRequestManager::constructDefaultRequestSettings(int type, camera_metadata_t **request)
{
    Mutex::Autolock l(m_requestLock);

    CLOGD("DEBUG(%s[%d]):Type = %d", __FUNCTION__, __LINE__, type);

    struct camera2_shot_ext shot_ext;

    if (m_defaultRequestTemplate[type]) {
        *request = m_defaultRequestTemplate[type];
        return OK;
    }

    m_converter->constructDefaultRequestSettings(type, request);

    m_defaultRequestTemplate[type] = *request;

    /* create default shot */
    m_converter->initShotData(&m_dummyShot);

    CLOGD("DEBUG(%s[%d]):Registered default request template(%d)",
        __FUNCTION__, __LINE__, type);
    return OK;
}

status_t ExynosCameraRequestManager::m_pushBack(ExynosCameraRequest* item, RequestInfoList *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    lock->lock();
    list->push_back(item);
    lock->unlock();
    return ret;
}

status_t ExynosCameraRequestManager::m_popBack(ExynosCameraRequest** item, RequestInfoList *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    lock->lock();
    if (list->size() > 0) {
        *item = list->back();
        list->pop_back();
    } else {
        CLOGE("ERR(%s[%d]):m_popBack failed, size(%zu)", __FUNCTION__, __LINE__, list->size());
        ret = INVALID_OPERATION;
    }
    lock->unlock();
    return ret;
}

status_t ExynosCameraRequestManager::m_pushFront(ExynosCameraRequest* item, RequestInfoList *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    lock->lock();
    list->push_back(item);
    lock->unlock();
    return ret;
}

status_t ExynosCameraRequestManager::m_popFront(ExynosCameraRequest** item, RequestInfoList *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    lock->lock();
    if (list->size() > 0) {
        *item = list->front();
        list->pop_front();
    } else {
        CLOGE("ERR(%s[%d]):m_popFront failed, size(%zu)", __FUNCTION__, __LINE__, list->size());
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraRequestManager::m_get(uint32_t frameCount,
                                        ExynosCameraRequest** item,
                                        RequestInfoList *list,
                                        Mutex *lock)
{
    status_t ret = INVALID_OPERATION;
    RequestInfoListIterator iter;
    ExynosCameraRequest* request = NULL;

    if (*item == NULL) {
        CLOGE("ERR(%s[%d]):item is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (list == NULL) {
        CLOGE("ERR(%s[%d]):list is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (lock == NULL) {
        CLOGE("ERR(%s[%d]):lock is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    lock->lock();
    for (iter = list->begin(); iter != list->end(); ++iter) {
        request = *iter;
        if (request->getKey() == frameCount) {
            ret = NO_ERROR;
            break;
        }
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraRequestManager::m_push(ExynosCameraRequest* item, RequestInfoMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<RequestInfoMap::iterator,bool> listRet;
    lock->lock();
    listRet = list->insert( pair<uint32_t, ExynosCameraRequest*>(item->getKey(), item));
    if (listRet.second == false) {
        ret = INVALID_OPERATION;
        CLOGE("ERR(%s[%d]):m_push failed, request already exist!! Request frameCnt( %d )",
            __FUNCTION__, __LINE__, item->getFrameCount());
    }
    lock->unlock();

    m_printAllRequestInfo(list, lock);

    return ret;
}

status_t ExynosCameraRequestManager::m_pop(uint32_t frameCount, ExynosCameraRequest** item, RequestInfoMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<RequestInfoMap::iterator,bool> listRet;
    RequestInfoMapIterator iter;
    ExynosCameraRequest *request = NULL;

    lock->lock();
    iter = list->find(frameCount);
    if (iter != list->end()) {
        request = iter->second;
        *item = request;
        list->erase( iter );
    } else {
        CLOGE("ERR(%s[%d]):m_pop failed, request is not EXIST Request frameCnt(%d)",
            __FUNCTION__, __LINE__, frameCount);
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraRequestManager::m_get(uint32_t frameCount, ExynosCameraRequest** item, RequestInfoMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<RequestInfoMap::iterator,bool> listRet;
    RequestInfoMapIterator iter;
    ExynosCameraRequest *request = NULL;

    lock->lock();
    iter = list->find(frameCount);
    if (iter != list->end()) {
        request = iter->second;
        *item = request;
    } else {
        CLOGE("ERR(%s[%d]):m_pop failed, request is not EXIST Request frameCnt( %d )",
            __FUNCTION__, __LINE__, frameCount);
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    m_printAllRequestInfo(list, lock);

    return ret;
}

void ExynosCameraRequestManager::m_printAllRequestInfo(RequestInfoMap *map, Mutex *lock)
{
    RequestInfoMapIterator iter;
    ExynosCameraRequest *request = NULL;
    ExynosCameraRequest *item = NULL;
    camera3_capture_request_t *serviceRequest = NULL;

    lock->lock();
    iter = map->begin();

    while(iter != map->end()) {
        request = iter->second;
        item = request;

        serviceRequest = item->getService();
#if 0
        CLOGE("INFO(%s[%d]):key(%d), serviceFrameCount(%d), (%p) frame_number(%d), outputNum(%d)", __FUNCTION__, __LINE__,
            request->getKey(),
            request->getFrameCount(),
            serviceRequest,
            serviceRequest->frame_number,
            serviceRequest->num_output_buffers);
#endif
        iter++;
    }
    lock->unlock();
}

status_t ExynosCameraRequestManager::m_delete(ExynosCameraRequest *item)
{
    status_t ret = NO_ERROR;

    CLOGV("DEBUG(%s[%d]):m_delete -> delete request(%d)", __FUNCTION__, __LINE__, item->getFrameCount());

    delete item;
    item = NULL;

    return ret;
}

status_t ExynosCameraRequestManager::m_pushFactory(int key,
                                                    ExynosCamera3FrameFactory* item,
                                                    FrameFactoryMap *list,
                                                    Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<FrameFactoryMap::iterator,bool> listRet;
    lock->lock();
    listRet = list->insert( pair<uint32_t, ExynosCamera3FrameFactory*>(key, item));
    if (listRet.second == false) {
        ret = INVALID_OPERATION;
        CLOGE("ERR(%s[%d]):m_push failed, request already exist!! Request frameCnt( %d )",
            __FUNCTION__, __LINE__, key);
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraRequestManager::m_popFactory(int key, ExynosCamera3FrameFactory** item, FrameFactoryMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<FrameFactoryMap::iterator,bool> listRet;
    FrameFactoryMapIterator iter;
    ExynosCamera3FrameFactory *factory = NULL;

    lock->lock();
    iter = list->find(key);
    if (iter != list->end()) {
        factory = iter->second;
        *item = factory;
        list->erase( iter );
    } else {
        CLOGE("ERR(%s[%d]):m_pop failed, factory is not EXIST Request frameCnt( %d )",
            __FUNCTION__, __LINE__, key);
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraRequestManager::m_getFactory(int key, ExynosCamera3FrameFactory** item, FrameFactoryMap *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    pair<FrameFactoryMap::iterator,bool> listRet;
    FrameFactoryMapIterator iter;
    ExynosCamera3FrameFactory *factory = NULL;
    lock->lock();
    iter = list->find(key);
    if (iter != list->end()) {
        factory = iter->second;
        *item = factory;
    } else {
        CLOGE("ERR(%s[%d]):m_pop failed, request is not EXIST Request frameCnt( %d )",
            __FUNCTION__, __LINE__, key);
        ret = INVALID_OPERATION;
    }
    lock->unlock();

    return ret;
}

ExynosCameraRequest* ExynosCameraRequestManager::registerServiceRequest(camera3_capture_request_t *request)
{
    status_t ret = NO_ERROR;
    ExynosCameraRequest *obj = NULL;
    struct camera2_shot_ext shot_ext;
    CameraMetadata meta;
    int32_t captureIntent = 0;
    uint32_t bufferCnt = 0;
    camera3_stream_buffer_t *inputbuffer = NULL;
    const camera3_stream_buffer_t *outputbuffer = NULL;
    ExynosCameraStream *stream = NULL;
    ExynosCamera3FrameFactory *factory = NULL;
    int32_t streamID = 0;
    int32_t factoryID = 0;
    bool needDummyStream = true;
    bool isZslInput = false;

    if (request->settings == NULL) {
        meta = m_previousMeta;
    } else {
        meta = request->settings;
    }

    if (meta.exists(ANDROID_CONTROL_CAPTURE_INTENT)) {
        captureIntent = meta.find(ANDROID_CONTROL_CAPTURE_INTENT).data.u8[0];
        CLOGV("DEBUG(%s[%d]):ANDROID_CONTROL_CAPTURE_INTENT is (%d)",
            __FUNCTION__, __LINE__, captureIntent);
    }

    /* Check whether the input buffer (ZSL input) is specified.
       Use zslFramFactory in the following section if ZSL input is used
    */
    obj = new ExynosCamera3Request(request, m_previousMeta);
    bufferCnt = obj->getNumOfInputBuffer();
    inputbuffer = obj->getInputBuffer();
    for(uint32_t i = 0 ; i < bufferCnt ; i++) {
        stream = static_cast<ExynosCameraStream*>(inputbuffer[i].stream->priv);
        stream->getID(&streamID);
        factoryID = streamID % HAL_STREAM_ID_MAX;

        /* Stream ID validity */
        if(factoryID == HAL_STREAM_ID_ZSL_INPUT) {
            isZslInput = true;
        } else {
            /* Ignore input buffer */
            CLOGE("ERR(%s[%d]):Invalid input streamID. captureIntent(%d) streamID(%d)",
                    __FUNCTION__, __LINE__, captureIntent, streamID);
        }
    }

    bufferCnt = obj->getNumOfOutputBuffer();
    outputbuffer = obj->getOutputBuffers();
    for(uint32_t i = 0 ; i < bufferCnt ; i++) {
        stream = static_cast<ExynosCameraStream*>(outputbuffer[i].stream->priv);
        stream->getID(&streamID);
        factoryID = streamID % HAL_STREAM_ID_MAX;
        switch(streamID % HAL_STREAM_ID_MAX) {
        case HAL_STREAM_ID_PREVIEW:
        case HAL_STREAM_ID_VIDEO:
        case HAL_STREAM_ID_CALLBACK:
        case HAL_STREAM_ID_RAW:
        case HAL_STREAM_ID_ZSL_OUTPUT:
            needDummyStream = false;
            break;
        default:
            break;
        }

        if (m_parameters->isReprocessing() == false) {
            needDummyStream = false;
        }

        /* Choose appropirate frame factory depends on input buffer is specified or not */
        if(isZslInput == true) {
            ret = m_getFactory(factoryID, &factory, &m_zslFactoryMap, &m_zslFactoryMapLock);
            CLOGD("DEBUG(%s[%d]):ZSL framefactory for streamID(%d)",
                    __FUNCTION__, __LINE__, streamID);
        } else {
            ret = m_getFactory(factoryID, &factory, &m_factoryMap, &m_factoryMapLock);
            CLOGV("DEBUG(%s[%d]):Normal framefactory for streamID(%d)",
                    __FUNCTION__, __LINE__, streamID);
        }
        if (ret < 0) {
            CLOGD("DEBUG(%s[%d]):m_getFactory is failed captureIntent(%d) streamID(%d)",
                    __FUNCTION__, __LINE__, captureIntent, streamID);
        }
        obj->pushFrameFactory(streamID, factory);
        obj->pushRequestOutputStreams(streamID);
    }

#if !defined(ENABLE_FULL_FRAME)
    /* attach dummy stream to this request if this request needs dummy stream */
    obj->setNeedInternalFrame(needDummyStream);
#endif

    obj->getServiceShot(&shot_ext);
    meta = obj->getServiceMeta();
    m_converter->initShotData(&shot_ext);

    m_previousMeta = meta;

    CLOGV("DEBUG(%s[%d]):m_currReqeustList size(%zu), fn(%d)",
            __FUNCTION__, __LINE__, m_serviceRequests[EXYNOS_REQUEST_TYPE::PREVIEW].size(), obj->getFrameCount());

    if (meta.isEmpty()) {
        CLOGD("DEBUG(%s[%d]):meta is EMPTY", __FUNCTION__, __LINE__);
    } else {
        CLOGV("DEBUG(%s[%d]):meta is NOT EMPTY", __FUNCTION__, __LINE__);

    }

    int reqId;
    ret = m_converter->convertRequestToShot(meta, &shot_ext, &reqId);
    obj->setRequestId(reqId);

    obj->setServiceShot(&shot_ext);
    obj->setPrevShot(m_preShot);

    memcpy(m_preShot, &shot_ext, sizeof(struct camera2_shot_ext));

    ret = m_pushBack(obj, &m_serviceRequests[EXYNOS_REQUEST_TYPE::PREVIEW], &m_requestLock);
    if (ret < 0){
        CLOGE("ERR(%s[%d]):request m_pushBack is failed request(%d)", __FUNCTION__, __LINE__, obj->getFrameCount());

        delete obj;
        return NULL;
    }

    m_callbackSequencer->pushFrameCount(obj->getKey());

    return obj;
}

status_t ExynosCameraRequestManager::getPreviousShot(struct camera2_shot_ext *pre_shot_ext)
{
    memcpy(pre_shot_ext, m_preShot, sizeof(struct camera2_shot_ext));

    return NO_ERROR;
}

uint32_t ExynosCameraRequestManager::getRequestCount(void)
{
    Mutex::Autolock l(m_requestLock);
    return m_serviceRequests[EXYNOS_REQUEST_TYPE::PREVIEW].size() + m_runningRequests[EXYNOS_REQUEST_TYPE::PREVIEW].size();
}

uint32_t ExynosCameraRequestManager::getServiceRequestCount(void)
{
    Mutex::Autolock lock(m_requestLock);
    return m_serviceRequests[EXYNOS_REQUEST_TYPE::PREVIEW].size();
}

ExynosCameraRequest* ExynosCameraRequestManager::createServiceRequest()
{
    status_t ret = NO_ERROR;
    ExynosCameraRequest *obj = NULL;

    ret = m_popFront(&obj, &m_serviceRequests[EXYNOS_REQUEST_TYPE::PREVIEW], &m_requestLock);
    if (ret < 0){
        CLOGE("ERR(%s[%d]):request m_popFront is failed request", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        return NULL;
    }

    ret = m_push(obj, &m_runningRequests[EXYNOS_REQUEST_TYPE::PREVIEW], &m_requestLock);
    if (ret < 0){
        CLOGE("ERR(%s[%d]):request m_push is failed request", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        return NULL;
    }

    ret = m_increasePipelineDepth(&m_runningRequests[EXYNOS_REQUEST_TYPE::PREVIEW], &m_requestLock);
    if (ret != NO_ERROR)
        CLOGE("ERR(%s[%d]):Failed to increase the pipeline depth", __FUNCTION__, __LINE__);

    return obj;
}

status_t ExynosCameraRequestManager::deleteServiceRequest(uint32_t frameCount)
{
    status_t ret = NO_ERROR;
    uint32_t key = 0;
    ExynosCameraRequest *obj = NULL;

    ret = m_popKey(&key, frameCount);
    if (ret < NO_ERROR) {
        CLOGE("ERR(%s[%d]):Failed to m_popKey. frameCount %d",
                __FUNCTION__, __LINE__, frameCount);
        return INVALID_OPERATION;
    }

    ret = m_pop(key, &obj, &m_runningRequests[EXYNOS_REQUEST_TYPE::PREVIEW], &m_requestLock);
    if (ret < 0){
        ret = INVALID_OPERATION;
        CLOGE("ERR(%s[%d]):request m_popFront is failed request", __FUNCTION__, __LINE__);
    } else {
        m_delete(obj);
    }

    return ret;
}

ExynosCameraRequest* ExynosCameraRequestManager::getServiceRequest(uint32_t frameCount)
{
    status_t ret = NO_ERROR;
    uint32_t key = 0;
    ExynosCameraRequest* obj = NULL;

    ret = m_getKey(&key, frameCount);
    if (ret < NO_ERROR) {
        CLOGE("ERR(%s[%d]):Failed to m_popKey. frameCount %d",
                __FUNCTION__, __LINE__, frameCount);
        return NULL;
    }

    ret = m_get(key, &obj, &m_runningRequests[EXYNOS_REQUEST_TYPE::PREVIEW], &m_requestLock);
    if (ret < 0){
        ret = INVALID_OPERATION;
        CLOGE("ERR(%s[%d]):request m_popFront is failed request", __FUNCTION__, __LINE__);
    }

    return obj;
}

status_t ExynosCameraRequestManager::flush()
{
    status_t ret = NO_ERROR;

    for (int i = 0 ; i < EXYNOS_REQUEST_TYPE::MAX ; i++) {
        m_serviceRequests[i].clear();
        m_runningRequests[i].clear();
    }

    m_requestFrameCountMap.clear();

    if (m_callbackSequencer != NULL)
        m_callbackSequencer->flush();
    return ret;
}

status_t ExynosCameraRequestManager::m_getKey(uint32_t *key, uint32_t frameCount)
{
    status_t ret = NO_ERROR;
    RequestFrameCountMapIterator iter;

    m_requestFrameCountMapLock.lock();
    iter = m_requestFrameCountMap.find(frameCount);
    if (iter != m_requestFrameCountMap.end()) {
        *key = iter->second;
    } else {
        CLOGE("ERR(%s[%d]):get request key is failed. request for framecount(%d) is not EXIST",
                __FUNCTION__, __LINE__, frameCount);
        ret = INVALID_OPERATION;
    }
    m_requestFrameCountMapLock.unlock();

    return ret;
}

status_t ExynosCameraRequestManager::m_popKey(uint32_t *key, uint32_t frameCount)
{
    status_t ret = NO_ERROR;
    RequestFrameCountMapIterator iter;

    m_requestFrameCountMapLock.lock();
    iter = m_requestFrameCountMap.find(frameCount);
    if (iter != m_requestFrameCountMap.end()) {
        *key = iter->second;
        m_requestFrameCountMap.erase(iter);
    } else {
        CLOGE("ERR(%s[%d]):get request key is failed. request for framecount(%d) is not EXIST",
                __FUNCTION__, __LINE__, frameCount);
        ret = INVALID_OPERATION;
    }
    m_requestFrameCountMapLock.unlock();

    return ret;
}

uint32_t ExynosCameraRequestManager::m_generateResultKey()
{
    m_requestResultKeyLock.lock();
    uint32_t key = m_requestResultKey++;
    m_requestResultKeyLock.unlock();
    return key;
}

uint32_t ExynosCameraRequestManager::m_getResultKey()
{
    m_requestResultKeyLock.lock();
    uint32_t key = m_requestResultKey;
    m_requestResultKeyLock.unlock();
    return key;
}

ResultRequest ExynosCameraRequestManager::createResultRequest(uint32_t frameCount,
                                                            EXYNOS_REQUEST_RESULT::TYPE type,
                                                            camera3_capture_result_t *captureResult,
                                                            camera3_notify_msg_t *notifyMsg)
{
    status_t ret = NO_ERROR;
    uint32_t key = 0;
    ResultRequest request;

    ret = m_getKey(&key, frameCount);
    if (ret < NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_getKey is failed. framecount(%d)", __FUNCTION__, __LINE__, frameCount);
        return NULL;
    }

    request = new ExynosCamera3RequestResult(m_generateResultKey(), key, type, captureResult, notifyMsg);

    return request;
}

status_t ExynosCameraRequestManager::setCallbackOps(const camera3_callback_ops *callbackOps)
{
    status_t ret = NO_ERROR;
    m_callbackOps = callbackOps;
    return ret;
}

status_t ExynosCameraRequestManager::callbackRequest(ResultRequest result)
{
    status_t ret = NO_ERROR;

    ExynosCameraRequest* obj = NULL;
    ret = m_get(result->getFrameCount(), &obj, &m_runningRequests[EXYNOS_REQUEST_TYPE::PREVIEW], &m_requestLock);
    if (ret < NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_get is failed. requestKey(%d)",
                __FUNCTION__, __LINE__, result->getFrameCount());
        return ret;
    }
    CLOGV("INFO(%s[%d]):type(%d) key(%u) frameCount(%u) ",
            __FUNCTION__, __LINE__, result->getType(), result->getKey(), result->getFrameCount());

    switch(result->getType()){
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY:
    case EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY:
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA:
    case EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT:
        obj->pushResult(result);
        m_checkCallbackRequestSequence();
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_INVALID:
    default:
        CLOGE("ERR(%s[%d]):callbackRequest type have INVALID value type(%d) key(%u) frameCount(%u) ",
                __FUNCTION__, __LINE__, result->getType(), result->getKey(), result->getFrameCount());
        break;
    }

    return ret;
}

void ExynosCameraRequestManager::callbackSequencerLock()
{
    m_callbackSequencerLock.lock();
}

void ExynosCameraRequestManager::callbackSequencerUnlock()
{
    m_callbackSequencerLock.unlock();
}

status_t ExynosCameraRequestManager::setFrameCount(uint32_t frameCount, uint32_t requestKey)
{
    status_t ret = NO_ERROR;
    pair<RequestFrameCountMapIterator, bool> listRet;
    ExynosCameraRequest *request = NULL;

    m_requestFrameCountMapLock.lock();
    listRet = m_requestFrameCountMap.insert(pair<uint32_t, uint32_t>(frameCount, requestKey));
    if (listRet.second == false) {
        ret = INVALID_OPERATION;
        CLOGE("ERR(%s[%d]):Failed, requestKey(%d) already exist!!",
                __FUNCTION__, __LINE__, frameCount);
    }
    m_requestFrameCountMapLock.unlock();

    ret = m_get(requestKey, &request, &m_runningRequests[EXYNOS_REQUEST_TYPE::PREVIEW], &m_requestLock);
    if (ret < 0)
        CLOGE("ERR(%s[%d]):m_get is failed. requestKey(%d)", __FUNCTION__, __LINE__, requestKey);

    request->setFrameCount(frameCount);

    return ret;
}

status_t ExynosCameraRequestManager::m_checkCallbackRequestSequence()
{
    status_t ret = NO_ERROR;
    uint32_t notifyIndex = 0;
    ResultRequest callback;
    bool flag = false;
    uint32_t key = 0;
    ResultRequestList callbackList;
    ResultRequestListIterator callbackIter;
    EXYNOS_REQUEST_RESULT::TYPE cbType;

    ExynosCameraRequest* callbackReq = NULL;

    CallbackListkeys *callbackReqList= NULL;
    CallbackListkeysIter callbackReqIter;

    callbackList.clear();

    ExynosCameraRequest *camera3Request = NULL;

    /* m_callbackSequencerLock.lock(); */

    /* m_callbackSequencer->dumpList(); */

    m_callbackSequencer->getFrameCountList(&callbackReqList);

    callbackReqIter = callbackReqList->begin();
    while (callbackReqIter != callbackReqList->end()) {
        CLOGV("DEBUG(%s[%d]):(*callbackReqIter)(%d)", __FUNCTION__, __LINE__, (uint32_t)(*callbackReqIter));

        key = (uint32_t)(*callbackReqIter);
        ret = m_get(key, &callbackReq, &m_runningRequests[EXYNOS_REQUEST_TYPE::PREVIEW], &m_requestLock);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_get is failed. requestKey(%d)", __FUNCTION__, __LINE__, key);
            break;
        }
        camera3Request = callbackReq;

        CLOGV("DEBUG(%s[%d]):frameCount(%u)", __FUNCTION__, __LINE__, callbackReq->getFrameCount());

        /* Check NOTIFY Case */
        cbType = EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY;
        ret = callbackReq->popAndEraseResultsByType(cbType, &callbackList);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):popRequest type(%d) callbackList Size(%zu)",
                    __FUNCTION__, __LINE__, cbType, callbackList.size());
        }

        if (callbackList.size() > 0) {
            callbackIter = callbackList.begin();
            callback = *callbackIter;
            CLOGV("DEBUG(%s[%d]):frameCount(%u), size of list(%zu)", __FUNCTION__, __LINE__,
                callbackReq->getFrameCount(), callbackList.size());
            if (callbackReq->getCallbackDone(cbType) == false) {
                CLOGV("DEBUG(%s[%d]):frameCount(%u), size of list(%zu) getCallbackDone(%d)", __FUNCTION__, __LINE__,
                    callbackReq->getFrameCount(), callbackList.size(), callbackReq->getCallbackDone(cbType));
                m_callbackRequest(callback);
            }

            callbackReq->setCallbackDone(cbType, true);
        }

        /* Check BUFFER Case */
        cbType = EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY;
        if( callbackReq->getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY) == true ) {
            /* Output Buffers will be updated finally */
            ret = callbackReq->popResultsByType(cbType, &callbackList);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):popRequest frameCount(%u) type(%d) callbackList Size(%zu)",
                        __FUNCTION__, __LINE__, callbackReq->getFrameCount(), cbType, callbackList.size());
            }

            CLOGV("DEBUG(%s[%d]):callbackList.size(%zu)", __FUNCTION__, __LINE__, callbackList.size());
            if (callbackList.size() > 0) {
                callbackReq->setCallbackDone(cbType, true);
            }

            for (callbackIter = callbackList.begin(); callbackIter != callbackList.end(); callbackIter++) {
                callback = *callbackIter;
                /* Output Buffers will be updated finally */
                /* m_callbackRequest(callback); */
                /*  callbackReq->increaseCompleteBuffers(); */
            }

            /* if all buffer is done? :: packing buffers and making final result */
            if((int)callbackReq->getNumOfOutputBuffer() == callbackReq->getCompleteBufferCount()) {
                m_callbackPackingOutputBuffers(callbackReq);
            }

            cbType = EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA;
            ret = callbackReq->popAndEraseResultsByType(cbType, &callbackList);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):popRequest frameCount(%u) type(%d) callbackList Size(%zu)",
                        __FUNCTION__, __LINE__, callbackReq->getFrameCount(), cbType, callbackList.size());
            }

            if (callbackList.size() > 0) {
                callbackReq->setCallbackDone(cbType, true);
            }

            for (callbackIter = callbackList.begin(); callbackIter != callbackList.end(); callbackIter++) {
                callback = *callbackIter;
                m_callbackRequest(callback);
            }
        }

        /* Check ALL_RESULT Case */
        cbType = EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT;
        if( callbackReq->getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY) == true ) {
            ret = callbackReq->popAndEraseResultsByType(cbType, &callbackList);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):popRequest type(%d) callbackList Size(%zu)",
                        __FUNCTION__, __LINE__, cbType, callbackList.size());
            }

            CLOGV("DEBUG(%s[%d]):callbackList.size(%zu)", __FUNCTION__, __LINE__, callbackList.size());
            if (callbackList.size() > 0) {
                callbackReq->setCallbackDone(cbType, true);
            }

            for (callbackIter = callbackList.begin(); callbackIter != callbackList.end(); callbackIter++) {
                callback = *callbackIter;
                camera3_capture_result_t tempResult;
                callback->getCaptureRequest(&tempResult);
                m_callbackRequest(callback);
            }
        }

        if ((callbackReq->getCallbackDone(EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY) == false) ||
            ((int)camera3Request->getNumOfOutputBuffer() > camera3Request->getCompleteBufferCount())) {

            if (m_callbackTraceCount > 10) {
                CLOGW("WARN(%s[%d]):(*callbackReqIter)(%d)", __FUNCTION__, __LINE__, (uint32_t)(*callbackReqIter));
                CLOGW("WARN(%s[%d]):frameCount(%u)", __FUNCTION__, __LINE__, callbackReq->getFrameCount());
                m_callbackSequencer->dumpList();
                CLOGD("DEBUG(%s[%d]):callbackReq->getFrameCount(%d)", __FUNCTION__, __LINE__, callbackReq->getFrameCount());
                camera3Request->printCallbackDoneState();
            }

            m_callbackTraceCount++;
            break;
        }

        CLOGV("DEBUG(%s[%d]):callback is done complete(fc:%d), outcnt(%d), comcnt(%d)",
                __FUNCTION__, __LINE__, callbackReq->getFrameCount(),
                camera3Request->getNumOfOutputBuffer(), camera3Request->getCompleteBufferCount());

        callbackReqIter++;

        if (callbackReq->isComplete() &&
                (int)camera3Request->getNumOfOutputBuffer() == camera3Request->getCompleteBufferCount()) {
            CLOGV("DEBUG(%s[%d]):callback is done complete(%d)", __FUNCTION__, __LINE__, callbackReq->getFrameCount());

            m_callbackSequencer->deleteFrameCount(camera3Request->getKey());
            deleteServiceRequest(camera3Request->getFrameCount());
            m_callbackTraceCount = 0;

            m_debugCallbackFPS();
        }
    }

    /* m_callbackSequencerLock.unlock(); */

    return ret;

}

// Count number of invocation and print FPS for every 30 frames.
void ExynosCameraRequestManager::m_debugCallbackFPS() {
#ifdef CALLBACK_FPS_CHECK
    m_callbackFrameCnt++;
    if(m_callbackFrameCnt == 1) {
        // Initial invocation
        m_callbackDurationTimer.start();
    } else if(m_callbackFrameCnt >= CALLBACK_FPS_CHECK+1) {
        m_callbackDurationTimer.stop();
        long long durationTime = m_callbackDurationTimer.durationMsecs();

        float meanInterval = durationTime / (float)CALLBACK_FPS_CHECK;
        CLOGI("INFO(%s[%d]): CALLBACK_FPS_CHECK, duration %lld / 30 = %.2f ms. %.2f fps",
            __FUNCTION__, __LINE__, durationTime, meanInterval, 1000 / meanInterval);
        m_callbackFrameCnt = 0;
    }
#endif
}
status_t ExynosCameraRequestManager::m_callbackRequest(ResultRequest result)
{
    status_t ret = NO_ERROR;
    camera3_capture_result_t capture_result;
    camera3_notify_msg_t notify_msg;

    CLOGV("INFO(%s[%d]):type(%d) key(%u) frameCount(%u) ",
            __FUNCTION__, __LINE__, result->getType(), result->getKey(), result->getFrameCount());

    switch(result->getType()){
    case EXYNOS_REQUEST_RESULT::CALLBACK_NOTIFY_ONLY:
        result->getNofityRequest(&notify_msg);
        m_callbackNotifyRequest(&notify_msg);
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY:
    case EXYNOS_REQUEST_RESULT::CALLBACK_PARTIAL_3AA:
        result->getCaptureRequest(&capture_result);
        m_callbackCaptureRequest(&capture_result);

#if 1
        free((camera_metadata_t *)capture_result.result);
        if (capture_result.output_buffers != NULL) {
            delete[] capture_result.output_buffers;
            capture_result.output_buffers = NULL;
        }
#endif
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT:
        result->getCaptureRequest(&capture_result);
        m_callbackCaptureRequest(&capture_result);

#if 1
        free((camera_metadata_t *)capture_result.result);
        if (capture_result.output_buffers != NULL) {
            delete[] capture_result.output_buffers;
            capture_result.output_buffers = NULL;
        }
#endif
        break;
    case EXYNOS_REQUEST_RESULT::CALLBACK_INVALID:
    default:
        ret = BAD_VALUE;
        CLOGE("ERR(%s[%d]):callbackRequest type have INVALID value type(%d) key(%u) frameCount(%u) ",
                __FUNCTION__, __LINE__, result->getType(), result->getKey(), result->getFrameCount());
        break;
    }

    return ret;
}

status_t ExynosCameraRequestManager::m_callbackCaptureRequest(camera3_capture_result_t *result)
{
    status_t ret = NO_ERROR;
    CLOGV("DEBUG(%s[%d]):frame number(%d), #out(%d)",
            __FUNCTION__, __LINE__, result->frame_number, result->num_output_buffers);

    m_callbackOps->process_capture_result(m_callbackOps, result);
    return ret;
}

status_t ExynosCameraRequestManager::m_callbackNotifyRequest(camera3_notify_msg_t *msg)
{
    status_t ret = NO_ERROR;

    switch (msg->type) {
    case CAMERA3_MSG_ERROR:
        CLOGW("DEBUG(%s[%d]):msg frame(%d) type(%d) errorCode(%d)",
                __FUNCTION__, __LINE__, msg->message.error.frame_number, msg->type, msg->message.error.error_code);
        m_callbackOps->notify(m_callbackOps, msg);
        break;
    case CAMERA3_MSG_SHUTTER:
        CLOGV("DEBUG(%s[%d]):msg frame(%d) type(%d) timestamp(%llu)",
                __FUNCTION__, __LINE__, msg->message.shutter.frame_number, msg->type, msg->message.shutter.timestamp);
        m_callbackOps->notify(m_callbackOps, msg);
        break;
    default:
        CLOGE("ERR(%s[%d]):Msg type is invalid (%d)", __FUNCTION__, __LINE__, msg->type);
        ret = BAD_VALUE;
        break;
    }
    return ret;
}

status_t ExynosCameraRequestManager::m_callbackPackingOutputBuffers(ExynosCameraRequest* callbackRequest)
{
    status_t ret = NO_ERROR;
    camera3_stream_buffer_t *output_buffers;
    int bufferIndex = -2;
    ResultRequestkeys keys;
    ResultRequestkeysIterator iter;
    uint32_t key = 0;
    ResultRequest result;
    camera3_stream_buffer_t streamBuffer;
    camera3_capture_result_t requestResult;
    CameraMetadata resultMeta;

    CLOGV("DEBUG(%s[%d]):frameCount(%d), EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ALL",
            __FUNCTION__, __LINE__, callbackRequest->getFrameCount());

    /* make output stream buffers */
    output_buffers = new camera3_stream_buffer[callbackRequest->getNumOfOutputBuffer()];
    callbackRequest->getResultKeys(&keys, EXYNOS_REQUEST_RESULT::CALLBACK_BUFFER_ONLY);
    bufferIndex = 0;

    for (iter = keys.begin(); iter != keys.end(); ++iter) {
        key = *iter;
        result = callbackRequest->popResult(key);
        CLOGV("DEBUG(%s[%d]):result(%d)", __FUNCTION__, __LINE__, result->getKey());

        while (result->getNumOfStreamBuffer() > 0) {
            result->popStreamBuffer(&streamBuffer);
            output_buffers[bufferIndex] = streamBuffer;
            bufferIndex++;
        }
    }

    /* update pipeline depth */
    callbackRequest->updatePipelineDepth();
    resultMeta = callbackRequest->getResultMeta();

    /* construct result for service */
    requestResult.frame_number = callbackRequest->getKey();
    requestResult.result = resultMeta.release();
    requestResult.num_output_buffers = bufferIndex;
    requestResult.output_buffers = output_buffers;
    requestResult.input_buffer = callbackRequest->getInputBuffer();
    requestResult.partial_result = 1;

    ResultRequest resultRequest = NULL;
    CLOGV("INFO(%s[%d]):frame number(%d), #out(%d)",
            __FUNCTION__, __LINE__, requestResult.frame_number, requestResult.num_output_buffers);

    resultRequest = this->createResultRequest(callbackRequest->getFrameCount(),
                                            EXYNOS_REQUEST_RESULT::CALLBACK_ALL_RESULT,
                                            &requestResult,
                                            NULL);
    callbackRequest->pushResult(resultRequest);

    return ret;
}

/* Increase the pipeline depth value from each request in running request map */
status_t ExynosCameraRequestManager::m_increasePipelineDepth(RequestInfoMap *map, Mutex *lock)
{
    status_t ret = NO_ERROR;
    RequestInfoMapIterator requestIter;
    ExynosCameraRequest *request = NULL;
    struct camera2_shot_ext shot_ext;

    lock->lock();
    if (map->size() < 1) {
        CLOGV("INFO(%s[%d]):map is empty. Skip to increase the pipeline depth",
                __FUNCTION__, __LINE__);
        ret = NO_ERROR;
        goto func_exit;
    }

    requestIter = map->begin();
    while (requestIter != map->end()) {
        request = requestIter->second;

        request->increasePipelineDepth();
        requestIter++;
    }

func_exit:
    lock->unlock();
    return ret;
}

ExynosCameraCallbackSequencer::ExynosCameraCallbackSequencer()
{
    m_requestFrameCountList.clear();
}

ExynosCameraCallbackSequencer::~ExynosCameraCallbackSequencer()
{
    if (m_requestFrameCountList.size() > 0) {
        ALOGE("ERR(%s[%d]):destructor size is not ZERO(%zu)",
                __FUNCTION__, __LINE__, m_requestFrameCountList.size());
    }
}

uint32_t ExynosCameraCallbackSequencer::popFrameCount()
{
    status_t ret = NO_ERROR;
    uint32_t obj;

    obj = m_pop(EXYNOS_LIST_OPER::SINGLE_FRONT, &m_requestFrameCountList, &m_requestCbListLock);
    if (ret < 0){
        ALOGE("ERR(%s[%d]):m_get failed", __FUNCTION__, __LINE__);
        return 0;
    }

    return obj;
}

status_t ExynosCameraCallbackSequencer::pushFrameCount(uint32_t frameCount)
{
    status_t ret = NO_ERROR;

    ret = m_push(EXYNOS_LIST_OPER::SINGLE_BACK, frameCount, &m_requestFrameCountList, &m_requestCbListLock);
    if (ret < 0){
        ALOGE("ERR(%s[%d]):m_push failed, frameCount(%d)",
                __FUNCTION__, __LINE__, frameCount);
    }

    return ret;
}

uint32_t ExynosCameraCallbackSequencer::size()
{
    return m_requestFrameCountList.size();
}

status_t ExynosCameraCallbackSequencer::getFrameCountList(CallbackListkeys **list)
{
    status_t ret = NO_ERROR;

    *list = &m_requestFrameCountList;

    return ret;
}

status_t ExynosCameraCallbackSequencer::deleteFrameCount(uint32_t frameCount)
{
    status_t ret = NO_ERROR;

    ret = m_delete(frameCount, &m_requestFrameCountList, &m_requestCbListLock);
    if (ret < 0){
        ALOGE("ERR(%s[%d]):m_push failed, frameCount(%d)", __FUNCTION__, __LINE__, frameCount);
    }

    return ret;
}

void ExynosCameraCallbackSequencer::dumpList()
{
    CallbackListkeysIter iter;
    CallbackListkeys *list = &m_requestFrameCountList;

    m_requestCbListLock.lock();

    if (list->size() > 0) {
        for (iter = list->begin(); iter != list->end();) {
            ALOGE("DEBUG(%s[%d]):frameCount(%d), size(%zu)", __FUNCTION__, __LINE__, *iter, list->size());
            iter++;
        }
    } else {
        ALOGE("ERR(%s[%d]):m_getCallbackResults failed, size is ZERO, size(%zu)",
                __FUNCTION__, __LINE__, list->size());
    }

    m_requestCbListLock.unlock();

}

status_t ExynosCameraCallbackSequencer::flush()
{
    status_t ret = NO_ERROR;

    m_requestFrameCountList.clear();
    return ret;
}

status_t ExynosCameraCallbackSequencer::m_init()
{
    status_t ret = NO_ERROR;

    m_requestFrameCountList.clear();
    return ret;
}

status_t ExynosCameraCallbackSequencer::m_deinit()
{
    status_t ret = NO_ERROR;

    m_requestFrameCountList.clear();
    return ret;
}

status_t ExynosCameraCallbackSequencer::m_push(EXYNOS_LIST_OPER::MODE operMode,
                                                uint32_t frameCount,
                                                CallbackListkeys *list,
                                                Mutex *lock)
{
    status_t ret = NO_ERROR;
    bool flag = false;

    lock->lock();

    switch (operMode) {
    case EXYNOS_LIST_OPER::SINGLE_BACK:
        list->push_back(frameCount);
        break;
    case EXYNOS_LIST_OPER::SINGLE_FRONT:
        list->push_front(frameCount);
        break;
    case EXYNOS_LIST_OPER::SINGLE_ORDER:
    case EXYNOS_LIST_OPER::MULTI_GET:
    default:
        ret = INVALID_OPERATION;
        ALOGE("ERR(%s[%d]):m_push failed, mode(%d) size(%zu)",
                __FUNCTION__, __LINE__, operMode, list->size());
        break;
    }

    lock->unlock();

    ALOGV("DEBUG(%s[%d]):m_push(%d), size(%zu)",
            __FUNCTION__, __LINE__, frameCount, list->size());

    return ret;
}

uint32_t ExynosCameraCallbackSequencer::m_pop(EXYNOS_LIST_OPER::MODE operMode, CallbackListkeys *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    CallbackListkeysIter iter;
    uint32_t obj = 0;

    bool flag = false;

    lock->lock();

    switch (operMode) {
    case EXYNOS_LIST_OPER::SINGLE_BACK:
        if (list->size() > 0) {
            obj = list->back();
            list->pop_back();
        } else {
            ALOGE("ERR(%s[%d]):m_pop failed, size(%zu)", __FUNCTION__, __LINE__, list->size());
            ret = INVALID_OPERATION;
        }
        break;
    case EXYNOS_LIST_OPER::SINGLE_FRONT:
        if (list->size() > 0) {
            obj = list->front();
            list->pop_front();
        } else {
            ALOGE("ERR(%s[%d]):m_pop failed, size(%zu)", __FUNCTION__, __LINE__, list->size());
            ret = INVALID_OPERATION;
        }
        break;
    case EXYNOS_LIST_OPER::SINGLE_ORDER:
    case EXYNOS_LIST_OPER::MULTI_GET:
    default:
        ret = INVALID_OPERATION;
        obj = 0;
        ALOGE("ERR(%s[%d]):m_push failed, mode(%d) size(%zu)",
                __FUNCTION__, __LINE__, operMode, list->size());
        break;
    }

    lock->unlock();

    ALOGI("INFO(%s[%d]):m_pop(%d), size(%zu)", __FUNCTION__, __LINE__, obj, list->size());

    return obj;
}

status_t ExynosCameraCallbackSequencer::m_delete(uint32_t frameCount, CallbackListkeys *list, Mutex *lock)
{
    status_t ret = NO_ERROR;
    CallbackListkeysIter iter;

    lock->lock();

    if (list->size() > 0) {
        for (iter = list->begin(); iter != list->end();) {
            if (frameCount == (uint32_t)*iter) {
                list->erase(iter++);
                ALOGV("DEBUG(%s[%d]):frameCount(%d), size(%zu)",
                        __FUNCTION__, __LINE__, frameCount, list->size());
            } else {
                iter++;
            }
        }
    } else {
        ret = INVALID_OPERATION;
        ALOGE("ERR(%s[%d]):m_getCallbackResults failed, size is ZERO, size(%zu)",
                __FUNCTION__, __LINE__, list->size());
    }

    lock->unlock();

    ALOGV("INFO(%s[%d]):size(%zu)", __FUNCTION__, __LINE__, list->size());

    return ret;
}
}; /* namespace android */
