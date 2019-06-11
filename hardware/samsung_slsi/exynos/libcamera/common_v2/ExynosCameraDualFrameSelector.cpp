/*
 * Copyright@ Samsung Electronics Co. LTD
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

#define LOG_TAG "ExynosCameraDualFrameSelector"

#include "ExynosCameraDualFrameSelector.h"

//#define EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_DEBUG

#define EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_MAX_OBJ    (100)
#define EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_MIN_OBJ    (1)
#define EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_CALIB_TIME (2) // 2msec

#ifdef EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_DEBUG
#define DUAL_FRAME_SELECTOR_LOG ALOGD
#else
#define DUAL_FRAME_SELECTOR_LOG ALOGV
#endif

ExynosCameraDualFrameSelector::SyncObj::SyncObj()
{
    m_cameraId = -1;
    m_frame = NULL;
    m_pipeID = -1;
    m_isSrc = false;
    m_dstPos = -1;

#ifdef USE_FRAMEMANAGER
    m_frameMgr = NULL;
#endif
    m_bufMgr = NULL;

    m_timeStamp = 0;
}

ExynosCameraDualFrameSelector::SyncObj::~SyncObj()
{}

status_t ExynosCameraDualFrameSelector::SyncObj::create(int cameraId,
                        ExynosCameraFrame *frame,
                        int pipeID,
                        bool isSrc,
                        int  dstPos,
#ifdef USE_FRAMEMANAGER
                        ExynosCameraFrameManager *frameMgr,
#endif
                        ExynosCameraBufferManager *bufMgr)
{
    status_t ret = NO_ERROR;

    m_cameraId = cameraId;
    m_frame = frame;
    m_pipeID = pipeID;
    m_isSrc = isSrc;
    m_dstPos = dstPos;

#ifdef USE_FRAMEMANAGER
    m_frameMgr = frameMgr;
#endif

    m_bufMgr = bufMgr;

    m_timeStamp = (int)(ns2ms(m_frame->getTimeStamp()));

    return ret;
}

status_t ExynosCameraDualFrameSelector::SyncObj::destroy(void)
{
    status_t ret = NO_ERROR;

    ExynosCameraBuffer buffer;

    if (m_bufMgr == NULL) {
        ALOGV("DEBUG(%s[%d]):m_bufMgr == NULL, ignore frames's buffer(it will delete by caller)", __FUNCTION__, __LINE__);
        return NO_ERROR;
    }

    // from ExynosCameraFrameSelector
    ret = m_getBufferFromFrame(&buffer);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):m_getBufferFromFrame fail pipeID(%d) BufferType(%s)",
            __FUNCTION__, __LINE__, m_pipeID, (m_isSrc)?"Src":"Dst");
        return ret;
    }

    ret = m_bufMgr->putBuffer(buffer.index, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL);
    if (ret < 0) {
        ALOGE("ERR(%s[%d]):putIndex is %d", __FUNCTION__, __LINE__, buffer.index);
        m_bufMgr->printBufferState();
        m_bufMgr->printBufferQState();
    }

    DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):delete frame ([%d]:%d)", __FUNCTION__, __LINE__, m_cameraId, getTimeStamp());

    m_lockedFrameComplete();

    return NO_ERROR;
}

int ExynosCameraDualFrameSelector::SyncObj::getCameraId(void)
{
    return m_cameraId;
}

ExynosCameraFrame *ExynosCameraDualFrameSelector::SyncObj::getFrame(void)
{
    return m_frame;
}

int ExynosCameraDualFrameSelector::SyncObj::getPipeID(void)
{
    return m_pipeID;
}

ExynosCameraBufferManager *ExynosCameraDualFrameSelector::SyncObj::getBufferManager(void)
{
    return m_bufMgr;
}

/*
 * Check complete flag of the Frame and deallocate it if it is completed.
 * This function ignores lock flag of the frame(Lock flag is usually set to protect
 * the frame from deallocation), so please use with caution.
 * This function is required to remove a frame from frameHoldingList.
 */
// from ExynosCameraFrameSelector
status_t ExynosCameraDualFrameSelector::SyncObj::m_lockedFrameComplete(void)
#ifdef USE_FRAMEMANAGER
{
    status_t ret = NO_ERROR;

#ifdef USE_FRAME_REFERENCE_COUNT
    if (m_frameMgr == NULL) {
        ALOGE("ERR(%s[%d]):m_frameMgr is NULL (%d)", __FUNCTION__, __LINE__, m_frame->getFrameCount());
        return INVALID_OPERATION;
    }

    m_frame->decRef();

    m_frameMgr->deleteFrame(m_frame);

    m_frame = NULL;
#else
    if (m_frame->isComplete() == true) {
        if (m_frame->getFrameLockState() == true) {
            ALOGW("WARN(%s[%d]):Deallocating locked frame, count(%d)", __FUNCTION__, __LINE__, m_frame->getFrameCount());
        }

        if (m_frameMgr != NULL) {
            m_frameMgr->deleteFrame(m_frame);
        }
        m_frame = NULL;
    }
#endif

    return ret;
}
#else // USE_FRAMEMANAGER
{
    status_t ret = NO_ERROR;

    if (m_frame == NULL) {
        ALOGE("ERR(%s[%d]):m_frame == NULL", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    if (m_frame->isComplete() == true) {
        if (m_frame->getFrameLockState() == true) {
            ALOGW("WARN(%s[%d]):Deallocating locked frame, count(%d)", __FUNCTION__, __LINE__, m_frame->getFrameCount());
            ALOGW("WARN(%s[%d]):frame is locked : isComplete(%d) count(%d) LockState(%d)", __FUNCTION__, __LINE__,
                m_frame->isComplete(),
                m_frame->getFrameCount(),
                m_frame->getFrameLockState());
        }
        delete m_frame;
        m_frame = NULL;
    }
    return ret;
}
#endif // USE_FRAMEMANAGER

status_t ExynosCameraDualFrameSelector::SyncObj::m_getBufferFromFrame(ExynosCameraBuffer *outBuffer)
{
    status_t ret = NO_ERROR;
    ExynosCameraBuffer selectedBuffer;

    if (m_frame == NULL) {
        ALOGE("ERR(%s[%d]):m_frame == NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (outBuffer == NULL) {
        ALOGE("ERR(%s[%d]):outBuffer == NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (m_isSrc) {
        ret = m_frame->getSrcBuffer(m_pipeID, &selectedBuffer);
        if (ret != NO_ERROR)
            ALOGE("ERR(%s[%d]):getSrcBuffer(pipeID %d) fail", __FUNCTION__, __LINE__, m_pipeID);
    } else {
        if (m_dstPos < 0) {
            ret = m_frame->getDstBuffer(m_pipeID, &selectedBuffer);
            if (ret != NO_ERROR)
                ALOGE("ERR(%s[%d]):getDstBuffer(pipeID %d) fail", __FUNCTION__, __LINE__, m_pipeID);
        } else {
            ret = m_frame->getDstBuffer(m_pipeID, &selectedBuffer, m_dstPos);
            if (ret != NO_ERROR)
                ALOGE("ERR(%s[%d]):getDstBuffer(pipeID %d, dstPos %d) fail", __FUNCTION__, __LINE__, m_pipeID, m_dstPos);
        }
    }
    *outBuffer = selectedBuffer;
    return ret;
}

int ExynosCameraDualFrameSelector::SyncObj::getTimeStamp(void)
{
    //return (int)(ns2ms(m_frame->getTimeStamp()));
    return m_timeStamp;
}

bool ExynosCameraDualFrameSelector::SyncObj::isSimilarTimeStamp(SyncObj *other)
{
    bool ret = false;

    int calibTime = EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_CALIB_TIME;

#if 1
    // if this is close other than calib time
    /*
     * sysnObj's time - 16msec < other's time <= sysnObj's time -> consider sync
     * sysnObj's time <= other's time < sysnObj's time + 16msec -> consider sync
     */
    if (other->getTimeStamp() < this->getTimeStamp()) {
        if (this->getTimeStamp() - calibTime <= other->getTimeStamp())
            ret = true;
    } else if (this->getTimeStamp() < other->getTimeStamp()) {
        if (other->getTimeStamp() <= this->getTimeStamp() + calibTime)
            ret = true;
    } else { // time is same.
        ret = true;
    }
#else
    // if this is more latest time than other
    if (other->getTimeStamp() <= this->getTimeStamp()) {
        ret = true;

        /* if two time is too long.
         * ex :
         * other's time : 100msec / this's time : 110msec
         *                       100msec + 16msec >= 110msec -> consider sync
         *
         * other's time : 100msec / this's time : 190msec
         *                       100msec + 16msec <  190msec -> consider not sync
         */
        if (other->getTimeStamp() + calibTime < this->getTimeStamp()) {
            ret = false;
        }
    }
#endif

    return ret;
}

ExynosCameraDualFrameSelector::ExynosCameraDualFrameSelector()
{
    ALOGD("DEBUG(%s[%d]):new %s object allocated", __FUNCTION__, __LINE__, __FUNCTION__);

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        m_outputList[i] = NULL;
        m_frameMgr[i] = NULL;
        m_holdCount[i] = 1;
        m_flagValidCameraId[i] = false;
    }
}

ExynosCameraDualFrameSelector::~ExynosCameraDualFrameSelector()
{
    status_t ret = NO_ERROR;

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        ret = clear(i);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):clear(%d) fail", __FUNCTION__, __LINE__, i);
        }
    }
}

status_t ExynosCameraDualFrameSelector::setInfo(int cameraId,
#ifdef USE_FRAMEMANAGER
                                                ExynosCameraFrameManager *frameMgr,
#endif
                                                int holdCount)
{
    status_t ret = NO_ERROR;

    /* we must protect list of various camera. */
    Mutex::Autolock lock(m_lock);

    if (CAMERA_ID_MAX <= cameraId) {
        ALOGE("ERR(%s[%d]):invalid cameraId(%d). so, fail", __FUNCTION__, __LINE__, cameraId);
        return INVALID_OPERATION;
    }

#ifdef USE_FRAMEMANAGER
    if (frameMgr == NULL) {
        ALOGE("ERR(%s[%d]):frameMgr == NULL. so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }
#endif

    m_frameMgr[cameraId]  = frameMgr;
    m_holdCount[cameraId] = holdCount;

    m_flagValidCameraId[cameraId] = true;

    return ret;
}

status_t ExynosCameraDualFrameSelector::manageNormalFrameHoldList(int cameraId,
                                        ExynosCameraList<ExynosCameraFrame *> *outputList,
                                        ExynosCameraFrame *frame,
                                        int pipeID, bool isSrc, int32_t dstPos, ExynosCameraBufferManager *bufMgr)
{
#ifdef EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_DEBUG
    //ExynosCameraAutoTimer autoTimer(__func__);
#endif

    status_t ret = NO_ERROR;

    if (CAMERA_ID_MAX <= cameraId) {
        ALOGE("ERR(%s[%d]):invalid cameraId(%d). so, fail", __FUNCTION__, __LINE__, cameraId);
        return INVALID_OPERATION;
    }

    if (outputList == NULL) {
        ALOGE("ERR(%s[%d]):outputList == NULL. so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    if (frame == NULL) {
        ALOGE("ERR(%s[%d]):frame == NULL. so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    if (bufMgr == NULL) {
        ALOGE("ERR(%s[%d]):bufMgr == NULL. so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    /* we must protect list of various camera. */
    Mutex::Autolock lock(m_lock);

    if (EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_MAX_OBJ < m_noSyncObjList[cameraId].size()) {
        ALOGE("ERR(%s[%d]):EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_MAX_OBJ < m_noSyncObjList[%d].size(). so, just return true",
            __FUNCTION__, __LINE__, cameraId);
        return INVALID_OPERATION;
    }

    bool flagAllCameraSync = false;
    SyncObj syncObj;
    SyncObj syncObjArr[CAMERA_ID_MAX];

    ret = syncObj.create(cameraId, frame, pipeID, isSrc, dstPos,
#ifdef USE_FRAMEMANAGER
                        m_frameMgr[cameraId],
#endif
                        bufMgr);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):syncObj.create([%d]:%d)) fail", __FUNCTION__, __LINE__, cameraId, m_getTimeStamp(frame));
        return ret;
    }

    m_outputList[cameraId] = outputList;

    DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):=== start with cameraId(%d) frameCount(%d), timeStamp(%d) addr(%p)===",
        __FUNCTION__, __LINE__, cameraId, frame->getFrameCount(), syncObj.getTimeStamp(), frame);

#ifdef EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_DEBUG
    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        m_printList(&m_noSyncObjList[i], i);
    }
#endif

    // check all list are sync. just assume this is synced.
    flagAllCameraSync = true;

    int validCameraCnt = 0;

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        if (i == cameraId) {
            validCameraCnt++;
            continue;
        }

        if (m_flagValidCameraId[i] == false)
            continue;

        validCameraCnt++;

        // search sync obj on another camera's list.
        // compare other list.
        if (m_checkSyncObjOnList(&syncObj, i, &m_noSyncObjList[i], &syncObjArr[i]) == false) {
            flagAllCameraSync = false;
        }
    }

    /* when smaller than 2, single stream is can be assume not synced frame. */
    if (validCameraCnt < 2) {
        flagAllCameraSync = false;

        DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):validCameraCnt(%d) < 2. so, flagAllCameraSync = false",
            __FUNCTION__, __LINE__, validCameraCnt);
    }

    syncObjArr[cameraId] = syncObj;

    // if found the all sync
    if (flagAllCameraSync == true) {
        /*
         * if all sybcObj are matched,
         * remove all old sybcObj of each synObjList.
         * then, pushQ(the last matched frames) to all synObjList.
         */

        for (int i = 0; i < CAMERA_ID_MAX; i++) {
            if (m_flagValidCameraId[i] == false)
                continue;

            syncObj = syncObjArr[i];

            // pop the previous list and same time listfm_captureObjList
            ret = m_popListUntilTimeStamp(&m_noSyncObjList[i], &syncObj);
            if (ret != NO_ERROR){
                ALOGE("ERR(%s[%d]):m_popListUntilTimeStamp([%d]:%d)),  fail",
                    __FUNCTION__, __LINE__, i, syncObj.getTimeStamp());
                continue;
            }

            // move sync obj to syncObjList.
            ret = m_pushList(&m_syncObjList[i], &syncObj);
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):m_pushList(m_syncObjList [%d]:%d)) fail",
                    __FUNCTION__, __LINE__, i, syncObj.getTimeStamp());
                continue;
            }

            if (m_holdCount[i] < (int)m_syncObjList[i].size() + m_outputList[i]->getSizeOfProcessQ()) {

                int newHoldCount = m_holdCount[i] - m_outputList[i]->getSizeOfProcessQ();

                if (newHoldCount < 0)
                    newHoldCount = 0;

                DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):m_holdCount[%d](%d) < m_syncObjList[%d].size()(%d) + m_outputList[%d]->getSizeOfProcessQ()(%d). so pop and remove until holdCount(%d)",
                    __FUNCTION__, __LINE__, i, m_holdCount[i], i, m_syncObjList[i].size(), i, m_outputList[i]->getSizeOfProcessQ(), newHoldCount);

                // remove sync object until min number
                ret = m_popListUntilNumber(&m_syncObjList[i], newHoldCount);
                if (ret != NO_ERROR) {
                    ALOGE("ERR(%s[%d]):m_popListUntilNumber([%d], %d)) fail",
                        __FUNCTION__, __LINE__, i, newHoldCount);
                    continue;
                }
            }
        }
    } else {
        // remove sync object until min number
        ret = m_popListUntilNumber(&m_noSyncObjList[cameraId], EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_MIN_OBJ, &syncObj);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):m_popListUntilNumber([%d], %d, %d)) fail",
                __FUNCTION__, __LINE__, cameraId, EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_MIN_OBJ, syncObj.getTimeStamp());
        }

        // to remember the not sync obj
        ret = m_pushList(&m_noSyncObjList[cameraId], &syncObj);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):m_pushList(m_noSyncObjList [%d]:%d)) fail",
                __FUNCTION__, __LINE__, cameraId, syncObj.getTimeStamp());
            return ret;
        }
    }

    DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):==========================================", __FUNCTION__, __LINE__);

    return ret;
}

ExynosCameraFrame* ExynosCameraDualFrameSelector::selectFrames(int cameraId)
{
#ifdef EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_DEBUG
    //ExynosCameraAutoTimer autoTimer(__func__);
#endif

    status_t ret = NO_ERROR;

    /* we must protect list of various camera. */
    Mutex::Autolock lock(m_lock);

    SyncObj syncObj;
    SyncObj nullSyncObj;
    ExynosCameraFrame *selectedFrame = NULL;

    /*
     * if captureObjList is ready by other camera, use m_outputList.
     * else just pop on m_syncObjList, and push m_outputList.
     * purpose is... be assure, selectFrames() can give same timeStamp on back and front.
     */
    if (0 < m_outputList[cameraId]->getSizeOfProcessQ()) {
        goto done;
    }

    // search sync obj on another camera's list.
    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        if (m_flagValidCameraId[i] == false)
            continue;

        syncObj = nullSyncObj;

        // if m_outputList is full, assert.
        if  (m_holdCount[i] <= m_outputList[i]->getSizeOfProcessQ()) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):m_holdCount[%d](%d) <= m_outputList[%d].size()(%d). because, capture happened on other side only, assert!!!!",
                __FUNCTION__, __LINE__, i, m_holdCount[i], i, m_outputList[i]->getSizeOfProcessQ());
        }

        // pop from sync obj and push to capture objectList
        if (m_popList(&m_syncObjList[i], &syncObj) != NO_ERROR) {
            ALOGE("ERR(%s[%d]):m_popList(m_syncObjList[%d]) fail", __FUNCTION__, __LINE__, i);
            continue;
        }

        DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):m_popList(m_syncObjList([%d]:%d)) and m_pushList(m_outputList([%d]:%d))",
            __FUNCTION__, __LINE__, i, syncObj.getTimeStamp(), i, syncObj.getTimeStamp());

        ret = m_pushQ(m_outputList[i], syncObj.getFrame(), i);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):m_pushQ(m_outputList [%d]:%d)) fail",
            __FUNCTION__, __LINE__, i, syncObj.getTimeStamp());
            continue;
        }
    }

done:
    /*
     * if pop on caller, we don't need the below code.
     * but, until now, we will pop here to get frame
     */
    if (m_popQ(m_outputList[cameraId], &selectedFrame, cameraId) != NO_ERROR) {
        ALOGE("ERR(%s[%d]):m_popQ(m_outputList[%d]) fail", __FUNCTION__, __LINE__, cameraId);
    }

    if (selectedFrame == NULL) {
        ALOGE("ERR(%s[%d]):selectedFrame == NULL([%d]:%d) fail", __FUNCTION__, __LINE__, cameraId, syncObj.getTimeStamp());
    } else {
        ALOGD("DEBUG(%s[%d]):selectedFrame is [%d]:frameCount(%d), timeStamp(%d)",
                __FUNCTION__, __LINE__, cameraId, selectedFrame->getFrameCount(), m_getTimeStamp(selectedFrame));
    }

    return selectedFrame;
}

status_t ExynosCameraDualFrameSelector::releaseFrames(int cameraId, ExynosCameraFrame *frame)
{
    status_t ret = NO_ERROR;

    // force release(== putBuffer() to bufferManger) sync frame
    // when error case, find frame on my and other's m_outputList() by frame'timeStamp.
    // then, release(== putBuffer() to bufferManger) sync frame of all cameras
    ALOGD("DEBUG(%s[%d]):releaseFrames([%d]:%d) called", __FUNCTION__, __LINE__, cameraId, m_getTimeStamp(frame));

    return ret;
}

status_t ExynosCameraDualFrameSelector::clear(int cameraId)
{
    status_t ret = NO_ERROR;

    /* we must protect list of various camera. */
    Mutex::Autolock lock(m_lock);

    ret = m_clearList(&m_syncObjList[cameraId]);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):m_clearList(m_syncObjList[%d]:All) fail", __FUNCTION__, __LINE__, cameraId);
        //return ret;
    }

    ret = m_clearList(&m_noSyncObjList[cameraId]);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):m_clearList(m_noSyncObjList[%d]:All) fail", __FUNCTION__, __LINE__, cameraId);
        //return ret;
    }

    return ret;
}

bool ExynosCameraDualFrameSelector::m_checkSyncObjOnList(SyncObj *syncObj,
                                                         int otherCameraId,
                                                         List<ExynosCameraDualFrameSelector::SyncObj> *list,
                                                         SyncObj *resultSyncObj)
{
    List<SyncObj>::iterator r;

    bool ret = false;
    SyncObj curSyncObj;

    if (list->empty()) {
        DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):when checking [%d]:%d on [%d]'s list. list is empty",
            __FUNCTION__, __LINE__, syncObj->getCameraId(), syncObj->getTimeStamp(), otherCameraId);
        return false;
    }

    r = list->begin()++;

    do {
        curSyncObj = *r;

        DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):compare [%d]:%d with [%d]:%d",
            __FUNCTION__, __LINE__,
            syncObj->getCameraId(), syncObj->getTimeStamp(),
            curSyncObj.getCameraId(), curSyncObj.getTimeStamp());

        if (syncObj->isSimilarTimeStamp(&curSyncObj) == true) {

            DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):[%d]:%d match with [%d]:%d",
                __FUNCTION__, __LINE__,
                syncObj->getCameraId(), syncObj->getTimeStamp(),
                curSyncObj.getCameraId(), curSyncObj.getTimeStamp());

            *resultSyncObj = curSyncObj;

            return true;
        }
        r++;
    } while (r != list->end());

    DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):[%d]:%d does not match with [%d]'s list",
            __FUNCTION__, __LINE__,
            syncObj->getCameraId(), syncObj->getTimeStamp(),
            otherCameraId);

    return false;
}

status_t ExynosCameraDualFrameSelector::m_destroySyncObj(SyncObj *syncObj, ExynosCameraList<ExynosCameraFrame *> *outputList)
{
    status_t ret = NO_ERROR;

    if (outputList) {
        ExynosCameraFrame *frame = syncObj->getFrame();
        int pipeID = syncObj->getPipeID();
        ExynosCameraFrameEntity *entity = frame->searchEntityByPipeId(pipeID);

        if (entity == NULL) {
            ALOGE("ERR(%s[%d]):frame([%d]:%d)'s entity == NULL, fail", __FUNCTION__, __LINE__, syncObj->getCameraId(), syncObj->getTimeStamp());
            return INVALID_OPERATION;
        }

        ret = entity->setDstBufState(ENTITY_BUFFER_STATE_ERROR);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):frame([%d]:%d)'s entity->setDstBufState(ENTITY_BUFFER_STATE_ERROR) fail",
                __FUNCTION__, __LINE__, syncObj->getCameraId(), syncObj->getTimeStamp());
        }

        ret = frame->setEntityState(pipeID, ENTITY_STATE_FRAME_DONE);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):frame([%d]:%d)'s setEntityState(%d, ENTITY_STATE_FRAME_DONE) fail",
                __FUNCTION__, __LINE__, syncObj->getCameraId(), syncObj->getTimeStamp());
        }

        /*
         * this will goto outputList.
         * frame is dropped
         */
        ret = m_pushQ(outputList, frame, syncObj->getCameraId());
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):m_pushQ([%d]:%d) fail", __FUNCTION__, __LINE__, syncObj->getCameraId(), syncObj->getTimeStamp());
        }
    } else {
        ret = syncObj->destroy();
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):syncObj->destroy();([%d]:%d) fail", __FUNCTION__, __LINE__, syncObj->getCameraId(), syncObj->getTimeStamp());
        }
    }

    return ret;
}

status_t ExynosCameraDualFrameSelector::m_pushList(List<ExynosCameraDualFrameSelector::SyncObj> *list, SyncObj *syncObj)
{
    DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):push([%d]:%d)", __FUNCTION__, __LINE__, syncObj->getCameraId(), syncObj->getTimeStamp());

    list->push_back(*syncObj);

    return NO_ERROR;
}

status_t ExynosCameraDualFrameSelector::m_popList(List<ExynosCameraDualFrameSelector::SyncObj> *list, SyncObj *syncObj)
{
    List<SyncObj>::iterator r;

    if (syncObj == NULL) {
        ALOGE("ERR(%s[%d]):syncObj == NULL, so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    if (list->empty())
        return TIMED_OUT;

    r = list->begin()++;
    *syncObj = *r;

    DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):pop([%d]:%d)",
        __FUNCTION__, __LINE__, syncObj->getCameraId(), syncObj->getTimeStamp());

    list->erase(r);

    return NO_ERROR;
};

status_t ExynosCameraDualFrameSelector::m_popListUntilTimeStamp(List<ExynosCameraDualFrameSelector::SyncObj> *list, SyncObj *syncObj,
                                                                ExynosCameraList<ExynosCameraFrame *> *outputList)
{
    status_t ret = NO_ERROR;

    List<SyncObj>::iterator r;

    SyncObj curSyncObj;
    bool flagFound = false;

    if (list->empty()) {
        ALOGV("DEBUG(%s[%d]):list is empty, when [%d]:%d", __FUNCTION__, __LINE__, syncObj->getCameraId(), syncObj->getTimeStamp());
        return NO_ERROR;
    }

    r = list->begin()++;

    do {
        curSyncObj = *r;

        if (syncObj->getTimeStamp() <= curSyncObj.getTimeStamp()) {
            flagFound = true;
        }

        DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):pop([%d]:%d), by [%d]:%d",
            __FUNCTION__, __LINE__, curSyncObj.getCameraId(), curSyncObj.getTimeStamp(), syncObj->getCameraId(), syncObj->getTimeStamp());

        list->erase(r);
        r++;

        if (flagFound == true) {
            break;
        } else {
            ret = m_destroySyncObj(&curSyncObj, outputList);
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):m_destroySyncObj(([%d]:%d)) fail", __FUNCTION__, __LINE__, curSyncObj.getCameraId(), curSyncObj.getTimeStamp());
            }
        }
    } while (r != list->end());

    return NO_ERROR;
}

status_t ExynosCameraDualFrameSelector::m_popListUntilNumber(List<ExynosCameraDualFrameSelector::SyncObj> *list, int minNum, SyncObj *syncObj,
                                                             ExynosCameraList<ExynosCameraFrame *> *outputList)
{
    status_t ret = NO_ERROR;

    List<SyncObj>::iterator r;

    SyncObj curSyncObj;
    bool flagFound = false;

    if (list->empty()) {
        if (syncObj == NULL) {
            ALOGV("DEBUG(%s[%d]):list is empty, when remove by [%d], %d, %d", __FUNCTION__, __LINE__, syncObj->getCameraId(), minNum, syncObj->getTimeStamp());
        } else {
            ALOGV("DEBUG(%s[%d]):list is empty, when remove by %d", __FUNCTION__, __LINE__, minNum);
        }

        return NO_ERROR;
    }

    int size = list->size();

    if (size <= minNum) {
        return NO_ERROR;
    }

    r = list->begin()++;

    do {
        curSyncObj = *r;

        if (size <= minNum)
            break;

        if (syncObj) {
            if (syncObj->getTimeStamp() <= curSyncObj.getTimeStamp()) {
                ALOGE("ERR(%s[%d]):[%d]:%d <= [%d]:%d. weird. minNum(%d), size(%d)",
                    __FUNCTION__, __LINE__, syncObj->getCameraId(), syncObj->getTimeStamp(), curSyncObj.getCameraId(), curSyncObj.getTimeStamp(), minNum, size);
                flagFound = true;
            }
        }

        DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):pop([%d]:%d), by minNum(%d) <= size(%d)",
                __FUNCTION__, __LINE__, curSyncObj.getCameraId(), curSyncObj.getTimeStamp(),  minNum, size);

        list->erase(r);
        r++;

        if (flagFound == true) {
            break;
        } else {
            ret = m_destroySyncObj(&curSyncObj, outputList);
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):m_destroySyncObj(([%d]:%d)) fail", __FUNCTION__, __LINE__, curSyncObj.getCameraId(), curSyncObj.getTimeStamp());
            }
        }

        size--;
    } while (r != list->end());

    return NO_ERROR;
}

status_t ExynosCameraDualFrameSelector::m_clearList(List<ExynosCameraDualFrameSelector::SyncObj> *list,
                                                    ExynosCameraList<ExynosCameraFrame *> *outputList)
{
    status_t ret = NO_ERROR;

    List<SyncObj>::iterator r;

    SyncObj curSyncObj;

    ALOGD("DEBUG(%s[%d]):remaining list size(%d), we remove them all", __FUNCTION__, __LINE__, list->size());

    while (!list->empty()) {
        r = list->begin()++;
        curSyncObj = *r;

        DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):pop([%d]:%d)",
                __FUNCTION__, __LINE__, curSyncObj.getCameraId(), curSyncObj.getTimeStamp());

        ret = m_destroySyncObj(&curSyncObj, outputList);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):m_destroySyncObj(([%d]:%d)) fail", __FUNCTION__, __LINE__, curSyncObj.getCameraId(), curSyncObj.getTimeStamp());
        }

        list->erase(r);
    }

    ALOGD("DEBUG(%s[%d]):EXIT", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

void ExynosCameraDualFrameSelector::m_printList(List<ExynosCameraDualFrameSelector::SyncObj> *list, int cameraId)
{
    List<SyncObj>::iterator r;

    SyncObj curSyncObj;

    if (m_flagValidCameraId[cameraId] == false) {
        ALOGD("DEBUG(%s[%d]):=== [%d] is not valid sensor ===", __FUNCTION__, __LINE__, cameraId);
        goto done;
    }

    ALOGD("DEBUG(%s[%d]):=== [%d]'s size(%d) ===", __FUNCTION__, __LINE__, cameraId, list->size());
    if (list->empty()) {
        goto done;
    }

    r = list->begin()++;

    do {
        curSyncObj = *r;
        ALOGD("DEBUG(%s[%d]):[%d]%d frameCount(%d), addr(%p)", __FUNCTION__, __LINE__, cameraId, curSyncObj.getTimeStamp(), curSyncObj.getFrame()->getFrameCount(), curSyncObj.getFrame());

        r++;
    } while (r != list->end());

done:
    ALOGD("DEBUG(%s[%d]):======================", __FUNCTION__, __LINE__);
}

status_t ExynosCameraDualFrameSelector::m_pushQ(ExynosCameraList<ExynosCameraFrame *> *list, ExynosCameraFrame* frame, int cameraId)
{
    status_t ret = NO_ERROR;

    if (list == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):list == NULL, assert!!!!", __FUNCTION__, __LINE__);
    }

    if (frame == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):frame == NULL, assert!!!!", __FUNCTION__, __LINE__);
    }

    DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):pushProcessQ([%d]:%d)", __FUNCTION__, __LINE__, cameraId, m_getTimeStamp(frame));

    list->pushProcessQ(&frame);

    return ret;
}

status_t ExynosCameraDualFrameSelector::m_popQ(ExynosCameraList<ExynosCameraFrame *> *list, ExynosCameraFrame** outframe, int cameraId)
{
    status_t ret = NO_ERROR;

#if 0
#ifdef EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_DEBUG
    ALOGD("DEBUG(%s[%d]):========== print list (%d:size(%d) ==========", __FUNCTION__, __LINE__, cameraId, list->getSizeOfProcessQ());
    if (syncObj->list->empty() == false) {
        ExynosCameraList<ExynosCameraFrame *>::iterator t;
        ExynosCameraFrame *frame;

        t = list->begin()++;

        do {
            frame = *t;
            ALOGD("DEBUG(%s[%d]):list[%d]:%d fCount(%d)",
                __FUNCTION__, __LINE__, cameraId, m_getTimeStamp(frame), frame->getFrameCount());

            t++;
        } while (t != list->end());
    }

    ALOGD("DEBUG(%s[%d]):====================================", __FUNCTION__, __LINE__);
    /*
     * you can debug about what item is in obj->list.
     * you need put this method on class ExynosCameraList
       iterator begin(void) { return m_processQ.begin(); };
       iterator end(void) { return m_processQ.end(); };
       bool empty(void) { return m_processQ.empty(); };
    */
#endif
#endif

    int iter = 0;
    int tryCount = 1;

    do {
        ret = list->popProcessQ(outframe);
        if (ret < 0) {
            if (ret == TIMED_OUT) {
                ALOGD("DEBUG(%s[%d]):PopQ Time out -> retry[max cur](%d %d)", __FUNCTION__, __LINE__, tryCount, iter);
                iter++;
                continue;
            }
        }
    } while (ret != OK && iter < tryCount);

    if (ret != OK) {
        ALOGE("ERR(%s[%d]):popQ fail(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    if (*outframe == NULL) {
        ALOGE("ERR(%s[%d]):popQ frame = NULL frame(%p)", __FUNCTION__, __LINE__, *outframe);
        return INVALID_OPERATION;
    }

    DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):popProcessQ([%d]:%d). so, remain size(%d)",
        __FUNCTION__, __LINE__, cameraId, m_getTimeStamp(*outframe), list->getSizeOfProcessQ());

    return ret;
}

int ExynosCameraDualFrameSelector::m_getTimeStamp(ExynosCameraFrame *frame)
{
    return (int)(ns2ms(frame->getTimeStamp()));
}

ExynosCameraDualPreviewFrameSelector::ExynosCameraDualPreviewFrameSelector()
{
    ALOGD("DEBUG(%s[%d]):new %s object allocated", __FUNCTION__, __LINE__, __FUNCTION__);

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        m_dof[i] = NULL;
    }
}

ExynosCameraDualPreviewFrameSelector::~ExynosCameraDualPreviewFrameSelector()
{
}

status_t ExynosCameraDualPreviewFrameSelector::setInfo(int cameraId,
                                                       int holdCount,
                                                       DOF *dof)
{
    status_t ret = NO_ERROR;

    if (CAMERA_ID_MAX <= cameraId) {
        ALOGE("ERR(%s[%d]):invalid cameraId(%d). so, fail", __FUNCTION__, __LINE__, cameraId);
        return INVALID_OPERATION;
    }

    if (dof == NULL) {
        ALOGE("ERR(%s[%d]):dof == NULL. so, fail", __FUNCTION__, __LINE__, cameraId);
        return INVALID_OPERATION;
    }

    if (m_holdCount[cameraId] != holdCount) {
        ALOGD("DEBUG(%s[%d]):m_holdCount[%d](%d) changed to %d", __FUNCTION__, __LINE__, cameraId, m_holdCount[cameraId], holdCount);
        m_holdCount[cameraId] = holdCount;
    }

    m_dof[cameraId] = dof;

    m_flagValidCameraId[cameraId] = true;

    return ret;
}

DOF *ExynosCameraDualPreviewFrameSelector::getDOF(int cameraId)
{
    return m_dof[cameraId];
}

status_t ExynosCameraDualPreviewFrameSelector::managePreviewFrameHoldList(int cameraId,
                                        ExynosCameraList<ExynosCameraFrame *> *outputList,
                                        ExynosCameraFrame *frame,
                                        int pipeID, bool isSrc, int32_t dstPos, ExynosCameraBufferManager *bufMgr)
{
#ifdef EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_DEBUG
    //ExynosCameraAutoTimer autoTimer(__func__);
#endif

    status_t ret = NO_ERROR;

    if (CAMERA_ID_MAX <= cameraId) {
        ALOGE("ERR(%s[%d]):invalid cameraId(%d). so, fail", __FUNCTION__, __LINE__, cameraId);
        return INVALID_OPERATION;
    }

    if (outputList == NULL) {
        ALOGE("ERR(%s[%d]):outputList == NULL. so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    if (frame == NULL) {
        ALOGE("ERR(%s[%d]):frame == NULL. so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    if (bufMgr == NULL) {
        ALOGE("ERR(%s[%d]):bufMgr == NULL. so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    /* we must protect list of various camera. */
    Mutex::Autolock lock(m_lock);

    if (EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_MAX_OBJ < m_noSyncObjList[cameraId].size()) {
        ALOGE("ERR(%s[%d]):EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_MAX_OBJ < m_noSyncObjList[%d].size(). so, just return true",
            __FUNCTION__, __LINE__, cameraId);
        return INVALID_OPERATION;
    }

    bool flagAllCameraSync = false;
    SyncObj syncObj;
    SyncObj syncObjArr[CAMERA_ID_MAX];

    ret = syncObj.create(cameraId, frame, pipeID, isSrc, dstPos,
#ifdef USE_FRAMEMANAGER
                        NULL,
#endif
                        bufMgr);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):syncObj.create([%d]:%d)) fail", __FUNCTION__, __LINE__, cameraId, m_getTimeStamp(frame));
        return ret;
    }

    m_outputList[cameraId] = outputList;

    DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):=== start with cameraId(%d) syncObj(%d) ===", __FUNCTION__, __LINE__, cameraId, syncObj.getTimeStamp());

#ifdef EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_DEBUG
    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        m_printList(&m_noSyncObjList[i], i);
    }
#endif

    // check all list are sync. just assume this is synced.
    flagAllCameraSync = true;

    int validCameraCnt = 0;

    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        if (i == cameraId) {
            validCameraCnt++;
            continue;
        }

        if (m_flagValidCameraId[i] == false)
            continue;

        validCameraCnt++;

        // search sync obj on another camera's list.
        // compare other list.
        if (m_checkSyncObjOnList(&syncObj, i, &m_noSyncObjList[i], &syncObjArr[i]) == false) {
            flagAllCameraSync = false;
        }
    }

    /* when smaller than 2, single stream is can be assume not synced frame. */
    if (validCameraCnt < 2) {
        flagAllCameraSync = false;

        DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):validCameraCnt(%d) < 2. so, flagAllCameraSync = false",
            __FUNCTION__, __LINE__, validCameraCnt);
    }

    syncObjArr[cameraId] = syncObj;

    // if found the all sync
    if (flagAllCameraSync == true) {
        /*
         * if all sybcObj are matched,
         * remove all old sybcObj of each synObjList.
         * then, pushQ(the last matched frames) to all synObjList.
         */

        for (int i = 0; i < CAMERA_ID_MAX; i++) {
            if (m_flagValidCameraId[i] == false)
                continue;

            syncObj = syncObjArr[i];

            // pop the previous list and same time listfm_captureObjList
            ret = m_popListUntilTimeStamp(&m_noSyncObjList[i], &syncObj, m_outputList[i]);
            if (ret != NO_ERROR){
                ALOGE("ERR(%s[%d]):m_popListUntilTimeStamp([%d]:%d)),  fail",
                    __FUNCTION__, __LINE__, i, syncObj.getTimeStamp());
                continue;
            }

            // move sync obj to syncObjList.
            ret = m_pushList(&m_syncObjList[i], &syncObj);
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):m_pushList(m_syncObjList [%d]:%d)) fail",
                    __FUNCTION__, __LINE__, i, syncObj.getTimeStamp());
                continue;
            }

            if (m_holdCount[i] < (int)m_syncObjList[i].size()) {

                int newHoldCount = m_holdCount[i];

                if (newHoldCount < 0)
                    newHoldCount = 0;

                DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):m_holdCount[%d](%d) < m_syncObjList[%d].size()(%d). so pop and remove until holdCount(%d)",
                    __FUNCTION__, __LINE__, i, m_holdCount[i], i, m_syncObjList[i].size(), newHoldCount);

                // remove sync object until min number
                ret = m_popListUntilNumber(&m_syncObjList[i], newHoldCount, NULL, m_outputList[i]);
                if (ret != NO_ERROR) {
                    ALOGE("ERR(%s[%d]):m_popListUntilNumber([%d], %d)) fail",
                        __FUNCTION__, __LINE__, i, newHoldCount);
                    continue;
                }
            }
        }
    } else {
        // remove sync object until min number
        ret = m_popListUntilNumber(&m_noSyncObjList[cameraId], EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_MIN_OBJ, &syncObj, m_outputList[cameraId]);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):m_popListUntilNumber([%d], %d, %d)) fail",
                __FUNCTION__, __LINE__, cameraId, EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_MIN_OBJ, syncObj.getTimeStamp());
        }

        // to remember the not sync obj
        ret = m_pushList(&m_noSyncObjList[cameraId], &syncObj);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):m_pushList(m_noSyncObjList [%d]:%d)) fail",
                __FUNCTION__, __LINE__, cameraId, syncObj.getTimeStamp());
            return ret;
        }
    }

    DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):==========================================", __FUNCTION__, __LINE__);

    return ret;
}

bool ExynosCameraDualPreviewFrameSelector::selectFrames(int cameraId,
                                                        ExynosCameraFrame **frame0, ExynosCameraList<ExynosCameraFrame *> **outputList0, ExynosCameraBufferManager **bufManager0,
                                                        ExynosCameraFrame **frame1, ExynosCameraList<ExynosCameraFrame *> **outputList1, ExynosCameraBufferManager **bufManager1)
{
#ifdef EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_DEBUG
    //ExynosCameraAutoTimer autoTimer(__func__);
#endif

    bool flagSynced = false;
    status_t ret = NO_ERROR;

    /* we must protect list of various camera. */
    Mutex::Autolock lock(m_lock);

    SyncObj syncObj;
    SyncObj nullSyncObj;
    ExynosCameraFrame *selectedFrame = NULL;
    int frameSyncCount = 0;

    /*
     * if m_syncObjList is <= 0, just return false;
     */
    if (m_syncObjList[cameraId].size() <= 0) {
        return false;
    }

    // search sync obj on another camera's list.
    for (int i = 0; i < CAMERA_ID_MAX; i++) {
        if (m_flagValidCameraId[i] == false)
            continue;

        syncObj = nullSyncObj;

        // if m_outputList is full, assert.
        if  (m_holdCount[i] < (int)m_syncObjList[i].size()) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):m_holdCount[%d](%d) <= m_syncObjList[%d].size()(%d). weird situation, assert!!!!",
                __FUNCTION__, __LINE__, i, m_holdCount[i], i, m_syncObjList[i].size());
        }

        // pop from sync obj and push to capture objectList
        if (m_popList(&m_syncObjList[i], &syncObj) != NO_ERROR) {
            ALOGE("ERR(%s[%d]):m_popList(m_syncObjList[%d]) fail", __FUNCTION__, __LINE__, i);
            continue;
        }

        DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):m_popList(m_syncObjList([%d]:%d))", __FUNCTION__, __LINE__, i, syncObj.getTimeStamp());

        selectedFrame = syncObj.getFrame();
        if (selectedFrame == NULL) {
            ALOGE("ERR(%s[%d]):selectedFrame == NULL([%d]:%d) fail", __FUNCTION__, __LINE__, cameraId, syncObj.getTimeStamp());
            continue;
        } else {
            DUAL_FRAME_SELECTOR_LOG("DEBUG(%s[%d]):selectedFrame is [%d]:frameCount(%d), timeStamp(%d)",
                    __FUNCTION__, __LINE__, cameraId, selectedFrame->getFrameCount(), m_getTimeStamp(selectedFrame));
        }

        if (frameSyncCount == 0) {
            *frame0 = selectedFrame;
            *outputList0 = m_outputList[i];
            *bufManager0 = syncObj.getBufferManager();
        } else if (frameSyncCount == 1) {
            *frame1 = selectedFrame;
            *outputList1 = m_outputList[i];
            *bufManager1 = syncObj.getBufferManager();
        } else {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):invalid i(%d), frameSyncCount(%d), assert!!!!", __FUNCTION__, __LINE__, i, frameSyncCount);
            break;
        }

        frameSyncCount++;

        flagSynced = true;
    }

done:
    return flagSynced;
}

status_t ExynosCameraDualPreviewFrameSelector::clear(int cameraId)
{
    status_t ret = NO_ERROR;

    /* we must protect list of various camera. */
    Mutex::Autolock lock(m_lock);

    ret = m_clearList(&m_syncObjList[cameraId], m_outputList[cameraId]);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):m_clearList(m_syncObjList[%d]:All) fail", __FUNCTION__, __LINE__, cameraId);
        //return ret;
    }

    ret = m_clearList(&m_noSyncObjList[cameraId], m_outputList[cameraId]);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):m_clearList(m_noSyncObjList[%d]:All) fail", __FUNCTION__, __LINE__, cameraId);
        //return ret;
    }

    return ret;
}
