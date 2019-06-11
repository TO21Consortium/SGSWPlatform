/*
**
** Copyright 2013, Samsung Electronics Co. LTD
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
#define LOG_TAG "ExynosCameraFrame"
#include <cutils/log.h>

#include "ExynosCameraFrame.h"

namespace android {

#ifdef DEBUG_FRAME_MEMORY_LEAK
unsigned long long ExynosCameraFrame::m_checkLeakCount;
Mutex ExynosCameraFrame::m_countLock;
#endif

ExynosCameraFrame::ExynosCameraFrame(
        ExynosCameraParameters *obj_param,
        uint32_t frameCount,
        uint32_t frameType)
{
    ALOGV("DEBUG(%s[%d]): create frame type(%d), frameCount(%d)", __FUNCTION__, __LINE__, frameType, frameCount);
    m_parameters = obj_param;
    m_frameCount = frameCount;
    m_frameType = frameType;

    m_init();
}

ExynosCameraFrame::ExynosCameraFrame()
{
    m_parameters = NULL;
    m_frameCount = 0;
    m_frameType = FRAME_TYPE_OTHERS;
    m_init();
}

ExynosCameraFrame::~ExynosCameraFrame()
{
    m_deinit();
}

#ifdef DEBUG_FRAME_MEMORY_LEAK
long long int ExynosCameraFrame::getCheckLeakCount()
{
    return m_privateCheckLeakCount;
}
#endif

status_t ExynosCameraFrame::addSiblingEntity(
        __unused ExynosCameraFrameEntity *curEntity,
        ExynosCameraFrameEntity *newEntity)
{
    Mutex::Autolock l(m_linkageLock);
    m_linkageList.push_back(newEntity);

    return NO_ERROR;
}

status_t ExynosCameraFrame::addChildEntity(
        ExynosCameraFrameEntity *parentEntity,
        ExynosCameraFrameEntity *newEntity)
{
    status_t ret = NO_ERROR;

    if (parentEntity == NULL) {
        ALOGE("ERR(%s):parentEntity is NULL", __FUNCTION__);
        return BAD_VALUE;
    }

    /* TODO: This is not suit in case of newEntity->next != NULL */
    ExynosCameraFrameEntity *tmpEntity;

    tmpEntity = parentEntity->getNextEntity();
    ret = parentEntity->setNextEntity(newEntity);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):setNextEntity fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    newEntity->setNextEntity(tmpEntity);

    return ret;
}

status_t ExynosCameraFrame::addChildEntity(
        ExynosCameraFrameEntity *parentEntity,
        ExynosCameraFrameEntity *newEntity,
        int parentPipeId)
{
    status_t ret = NO_ERROR;

    if (parentEntity == NULL) {
        ALOGE("ERR(%s):parentEntity is NULL", __FUNCTION__);
        return BAD_VALUE;
    }

    /* TODO: This is not suit in case of newEntity->next != NULL */
    ExynosCameraFrameEntity *tmpEntity;

    tmpEntity = parentEntity->getNextEntity();
    ret = parentEntity->setNextEntity(newEntity);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):setNextEntity fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    if (0 <= parentPipeId) {
        ret = newEntity->setParentPipeId((enum pipeline)parentPipeId);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):setParentPipeId(%d) fail, ret(%d)", __FUNCTION__, __LINE__, parentPipeId, ret);
            return ret;
        }
    } else {
        ALOGW("WARN(%s[%d]):parentPipeId(%d) < 0. you may set parentPipeId which connect between parent(%d) and child(%d)",
            __FUNCTION__, __LINE__, parentPipeId, parentEntity->getPipeId(), newEntity->getPipeId());
    }

    newEntity->setNextEntity(tmpEntity);
    return ret;
}

ExynosCameraFrameEntity *ExynosCameraFrame::getFirstEntity(void)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *firstEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        ALOGE("ERR(%s):m_linkageList is empty", __FUNCTION__);
        firstEntity = NULL;
        return firstEntity;
    }

    r = m_linkageList.begin()++;
    m_currentEntity = r;
    firstEntity = *r;

    return firstEntity;
}

ExynosCameraFrameEntity *ExynosCameraFrame::getNextEntity(void)
{
    ExynosCameraFrameEntity *nextEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    m_currentEntity++;

    if (m_currentEntity == m_linkageList.end()) {
        return nextEntity;
    }

    nextEntity = *m_currentEntity;

    return nextEntity;
}
/* Unused, but useful */
/*
ExynosCameraFrameEntity *ExynosCameraFrame::getChildEntity(ExynosCameraFrameEntity *parentEntity)
{
    ExynosCameraFrameEntity *childEntity = NULL;

    if (parentEntity == NULL) {
        ALOGE("ERR(%s):parentEntity is NULL", __FUNCTION__);
        return childEntity;
    }

    childEntity = parentEntity->getNextEntity();

    return childEntity;
}
*/

ExynosCameraFrameEntity *ExynosCameraFrame::searchEntityByPipeId(uint32_t pipeId)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;
    int listSize = 0;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        ALOGE("ERR(%s):m_linkageList is empty", __FUNCTION__);
        return NULL;
    }

    listSize = m_linkageList.size();
    r = m_linkageList.begin();

    for (int i = 0; i < listSize; i++) {
        curEntity = *r;
        if (curEntity == NULL) {
            ALOGE("ERR(%s):curEntity is NULL, index(%d), linkageList size(%d)",
                __FUNCTION__, i, listSize);
            return NULL;
        }

        while (curEntity != NULL) {
            if (curEntity->getPipeId() == pipeId)
                return curEntity;
            curEntity = curEntity->getNextEntity();
        }
        r++;
    }

    ALOGD("DEBUG(%s):Cannot find matched entity, frameCount(%d), pipeId(%d)", __FUNCTION__, getFrameCount(), pipeId);

    return NULL;
}

status_t ExynosCameraFrame::setSrcBuffer(uint32_t pipeId,
                                         ExynosCameraBuffer srcBuf)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->setSrcBuf(srcBuf);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Could not set src buffer, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::setDstBuffer(uint32_t pipeId,
                                         ExynosCameraBuffer dstBuf,
                                         uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        ALOGE("ERR(%s[%d]):Invalid buffer index, index(%d)", __FUNCTION__, __LINE__, nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->setDstBuf(dstBuf, nodeIndex);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Could not set dst buffer, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    /* TODO: set buffer to child node's source */
    entity = entity->getNextEntity();
    if (entity != NULL) {
        ret = entity->setSrcBuf(dstBuf);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):Could not set dst buffer, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }
    }

    return ret;
}

status_t ExynosCameraFrame::setDstBuffer(uint32_t pipeId,
                                         ExynosCameraBuffer dstBuf,
                                         uint32_t nodeIndex,
                                         int      parentPipeId)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        ALOGE("ERR(%s[%d]):Invalid buffer index, index(%d)", __FUNCTION__, __LINE__, nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->setDstBuf(dstBuf, nodeIndex);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Could not set dst buffer, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    /* TODO: set buffer to child node's source */
    entity = entity->getNextEntity();
    if (entity != NULL) {
        bool flagSetChildEntity = false;

        /*
         * it  will set child's source buffer
         * when no specific parent set. (for backward compatibility)
         * when specific parent only. (for MCPipe)
         */
        if (entity->flagSpecficParent() == true) {
            if (parentPipeId == entity->getParentPipeId()) {
                flagSetChildEntity = true;
            } else {
                ALOGV("DEBUG(%s[%d]):parentPipeId(%d) != entity->getParentPipeId()(%d). so skip setting child src Buf",
                    __FUNCTION__, __LINE__, parentPipeId, entity->getParentPipeId());
            }
        } else {
            /* this is for backward compatiblity */
            flagSetChildEntity = true;
        }

        /* child mode need to setting next */
        if (flagSetChildEntity == true) {
            ret = entity->setSrcBuf(dstBuf);
            if (ret != NO_ERROR) {
                ALOGE("ERR(%s[%d]):Could not set dst buffer, ret(%d)", __FUNCTION__, __LINE__, ret);
                return ret;
            }
        }
    }

    return ret;
}

status_t ExynosCameraFrame::getSrcBuffer(uint32_t pipeId,
                                         ExynosCameraBuffer *srcBuf)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->getSrcBuf(srcBuf);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Could not get src buffer, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::getDstBuffer(uint32_t pipeId,
                                         ExynosCameraBuffer *dstBuf,
                                         uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        ALOGE("ERR(%s[%d]):Invalid buffer index, index(%d)", __FUNCTION__, __LINE__, nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->getDstBuf(dstBuf, nodeIndex);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Could not get dst buffer, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::setSrcRect(uint32_t pipeId, ExynosRect srcRect)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->setSrcRect(srcRect);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Could not set src rect, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::setDstRect(uint32_t pipeId, ExynosRect dstRect)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->setDstRect(dstRect);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Could not set dst rect, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    /* TODO: set buffer to child node's source */
    entity = entity->getNextEntity();
    if (entity != NULL) {
        ret = entity->setSrcRect(dstRect);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):Could not set dst rect, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }
    }

    return ret;
}

status_t ExynosCameraFrame::getSrcRect(uint32_t pipeId, ExynosRect *srcRect)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->getSrcRect(srcRect);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Could not get src rect, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::getDstRect(uint32_t pipeId, ExynosRect *dstRect)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->getDstRect(dstRect);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Could not get dst rect, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::getSrcBufferState(uint32_t pipeId,
                                         entity_buffer_state_t *state)
{
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    *state = entity->getSrcBufState();

    return NO_ERROR;
}

status_t ExynosCameraFrame::getDstBufferState(uint32_t pipeId,
                                         entity_buffer_state_t *state,
                                         uint32_t nodeIndex)
{
    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        ALOGE("ERR(%s[%d]):Invalid buffer index, index(%d)", __FUNCTION__, __LINE__, nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    *state = entity->getDstBufState(nodeIndex);

    return NO_ERROR;
}

status_t ExynosCameraFrame::setSrcBufferState(uint32_t pipeId,
                                         entity_buffer_state_t state)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->setSrcBufState(state);

    return ret;
}

status_t ExynosCameraFrame::setDstBufferState(uint32_t pipeId,
                                         entity_buffer_state_t state,
                                         uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        ALOGE("ERR(%s[%d]):Invalid buffer index, index(%d)", __FUNCTION__, __LINE__, nodeIndex);
        return BAD_VALUE;
    }

    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->setDstBufState(state, nodeIndex);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):Could not set dst buffer, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    /* Set buffer to child node's source */
    entity = entity->getNextEntity();
    if (entity != NULL) {
        ret = entity->setSrcBufState(state);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):Could not set src buffer, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }
    }

    return ret;
}

status_t ExynosCameraFrame::ensureSrcBufferState(uint32_t pipeId,
                                         entity_buffer_state_t state)
{
    status_t ret = NO_ERROR;
    int retry = 0;
    entity_buffer_state_t curState;

    do {
        ret = getSrcBufferState(pipeId, &curState);
        if (ret != NO_ERROR)
            continue;

        if (state == curState) {
            ret = OK;
            break;
        } else {
            ret = BAD_VALUE;
            usleep(100);
        }

        retry++;
        if (retry == 10)
            ret = TIMED_OUT;
    } while (ret != OK && retry < 100);

    ALOGV("DEBUG(%s[%d]): retry count %d", __FUNCTION__, __LINE__, retry);

    return ret;
}

status_t ExynosCameraFrame::ensureDstBufferState(uint32_t pipeId,
                                         entity_buffer_state_t state)
{
    status_t ret = NO_ERROR;
    int retry = 0;
    entity_buffer_state_t curState;

    do {
        ret = getDstBufferState(pipeId, &curState);
        if (ret != NO_ERROR)
            continue;

        if (state == curState) {
            ret = OK;
            break;
        } else {
            ret = BAD_VALUE;
            usleep(100);
        }

        retry++;
        if (retry == 10)
            ret = TIMED_OUT;
    } while (ret != OK && retry < 100);

    ALOGV("DEBUG(%s[%d]): retry count %d", __FUNCTION__, __LINE__, retry);

    return ret;
}

status_t ExynosCameraFrame::setEntityState(uint32_t pipeId,
                                           entity_state_t state)
{
#ifdef USE_FRAME_REFERENCE_COUNT
    Mutex::Autolock lock(m_entityLock);
#endif
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    if (entity->getEntityState() == ENTITY_STATE_COMPLETE &&
        state != ENTITY_STATE_REWORK) {
        return NO_ERROR;
    }

    if (state == ENTITY_STATE_COMPLETE) {
        m_numCompletePipe++;
        if (m_numCompletePipe >= m_numRequestPipe)
            setFrameState(FRAME_STATE_COMPLETE);
    }

    entity->setEntityState(state);

    return NO_ERROR;
}

status_t ExynosCameraFrame::getEntityState(uint32_t pipeId,
                                           entity_state_t *state)
{
#ifdef USE_FRAME_REFERENCE_COUNT
    Mutex::Autolock lock(m_entityLock);
#endif
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    *state = entity->getEntityState();
    return NO_ERROR;
}

status_t ExynosCameraFrame::getEntityBufferType(uint32_t pipeId,
                                                entity_buffer_type_t *type)
{
#ifdef USE_FRAME_REFERENCE_COUNT
    Mutex::Autolock lock(m_entityLock);
#endif
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    *type = entity->getBufType();
    return NO_ERROR;
}

uint32_t ExynosCameraFrame::getFrameCount(void)
{
    return m_frameCount;
}

status_t ExynosCameraFrame::setNumRequestPipe(uint32_t num)
{
    m_numRequestPipe = num;
    return NO_ERROR;
}

uint32_t ExynosCameraFrame::getNumRequestPipe(void)
{
    return m_numRequestPipe;
}

bool ExynosCameraFrame::isComplete(void)
{
    return checkFrameState(FRAME_STATE_COMPLETE);
}

ExynosCameraFrameEntity *ExynosCameraFrame::getFrameDoneEntity(void)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        ALOGE("ERR(%s):m_linkageList is empty", __FUNCTION__);
        return NULL;
    }

    r = m_linkageList.begin()++;
    curEntity = *r;

    while (r != m_linkageList.end()) {
        if (curEntity != NULL) {
            switch (curEntity->getEntityState()) {
            case ENTITY_STATE_FRAME_SKIP:
            case ENTITY_STATE_REWORK:
            case ENTITY_STATE_FRAME_DONE:
                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                return curEntity;
                break;
            default:
                break;
            }
        }
        r++;
        curEntity = *r;
    }

    return NULL;
}

ExynosCameraFrameEntity *ExynosCameraFrame::getFrameDoneEntity(uint32_t pipeID)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        ALOGE("ERR(%s):m_linkageList is empty", __FUNCTION__);
        return NULL;
    }

    r = m_linkageList.begin()++;
    curEntity = *r;

    while (r != m_linkageList.end()) {
        if (curEntity != NULL && pipeID == curEntity->getPipeId()) {
            switch (curEntity->getEntityState()) {
            case ENTITY_STATE_FRAME_SKIP:
            case ENTITY_STATE_REWORK:
            case ENTITY_STATE_FRAME_DONE:
                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                return curEntity;
                break;
            default:
                break;
            }
        }
        r++;
        curEntity = *r;
    }

    return NULL;
}

ExynosCameraFrameEntity *ExynosCameraFrame::getFrameDoneFirstEntity(void)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        ALOGE("ERR(%s):m_linkageList is empty", __FUNCTION__);
        return NULL;
    }

    r = m_linkageList.begin()++;
    curEntity = *r;

    while (r != m_linkageList.end()) {
        if (curEntity != NULL) {
            switch (curEntity->getEntityState()) {
            case ENTITY_STATE_REWORK:
                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                return curEntity;
                break;
            case ENTITY_STATE_FRAME_DONE:
                return curEntity;
                break;
            case ENTITY_STATE_FRAME_SKIP:
            case ENTITY_STATE_COMPLETE:
                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                break;
            default:
                break;
            }
        }
        r++;
        curEntity = *r;
    }

    return NULL;
}

ExynosCameraFrameEntity *ExynosCameraFrame::getFrameDoneFirstEntity(uint32_t pipeID)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        ALOGE("ERR(%s):m_linkageList is empty", __FUNCTION__);
        return NULL;
    }

    r = m_linkageList.begin()++;
    curEntity = *r;

    while (r != m_linkageList.end()) {
        if (curEntity != NULL) {
            switch (curEntity->getEntityState()) {
            case ENTITY_STATE_REWORK:
                if (curEntity->getPipeId() == pipeID)
                    return curEntity;

                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                break;
            case ENTITY_STATE_FRAME_DONE:
                if (curEntity->getPipeId() == pipeID)
                    return curEntity;

                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                break;
            case ENTITY_STATE_FRAME_SKIP:
            case ENTITY_STATE_COMPLETE:
                if (curEntity->getNextEntity() != NULL) {
                    curEntity = curEntity->getNextEntity();
                    continue;
                }
                break;
            default:
                break;
            }
        }
        r++;
        curEntity = *r;
    }

    return NULL;
}

status_t ExynosCameraFrame::skipFrame(void)
{
#ifdef USE_FRAME_REFERENCE_COUNT
    Mutex::Autolock lock(m_frameStateLock);
#endif
    m_frameState = FRAME_STATE_SKIPPED;

    return NO_ERROR;
}

void ExynosCameraFrame::setFrameState(frame_status_t state)
{
#ifdef USE_FRAME_REFERENCE_COUNT
    Mutex::Autolock lock(m_frameStateLock);
#endif
    /* TODO: We need state machine */
    if (state > FRAME_STATE_INVALID)
        m_frameState = FRAME_STATE_INVALID;
    else
        m_frameState = state;
}

frame_status_t ExynosCameraFrame::getFrameState(void)
{
#ifdef USE_FRAME_REFERENCE_COUNT
    Mutex::Autolock lock(m_frameStateLock);
#endif
    return m_frameState;
}

bool ExynosCameraFrame::checkFrameState(frame_status_t state)
{
#ifdef USE_FRAME_REFERENCE_COUNT
    Mutex::Autolock lock(m_frameStateLock);
#endif
    return (m_frameState == state) ? true : false;
}

void ExynosCameraFrame::printEntity(void)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;
    int listSize = 0;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        ALOGE("ERR(%s):m_linkageList is empty", __FUNCTION__);
        return;
    }

    listSize = m_linkageList.size();
    r = m_linkageList.begin();

    ALOGD("DEBUG(%s): FrameCount(%d), request(%d), complete(%d)", __FUNCTION__, getFrameCount(), m_numRequestPipe, m_numCompletePipe);

    for (int i = 0; i < listSize; i++) {
        curEntity = *r;
        if (curEntity == NULL) {
            ALOGE("ERR(%s):curEntity is NULL, index(%d)", __FUNCTION__, i);
            return;
        }

        ALOGD("DEBUG(%s):sibling id(%d), state(%d)",
            __FUNCTION__, curEntity->getPipeId(), curEntity->getEntityState());

        while (curEntity != NULL) {
            ALOGD("DEBUG(%s):----- Child id(%d), state(%d)",
                __FUNCTION__, curEntity->getPipeId(), curEntity->getEntityState());
            curEntity = curEntity->getNextEntity();
        }
        r++;
    }

    return;
}

void ExynosCameraFrame::printNotDoneEntity(void)
{
    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;
    int listSize = 0;

    Mutex::Autolock l(m_linkageLock);
    if (m_linkageList.empty()) {
        ALOGE("ERR(%s):m_linkageList is empty", __FUNCTION__);
        return;
    }

    listSize = m_linkageList.size();
    r = m_linkageList.begin();

    ALOGD("DEBUG(%s): FrameCount(%d), request(%d), complete(%d)",
            __FUNCTION__, getFrameCount(), m_numRequestPipe, m_numCompletePipe);

    for (int i = 0; i < listSize; i++) {
        curEntity = *r;
        if (curEntity == NULL) {
            ALOGE("ERR(%s):curEntity is NULL, index(%d)", __FUNCTION__, i);
            return;
        }

        if (curEntity->getEntityState() != ENTITY_STATE_COMPLETE) {
            ALOGD("DEBUG(%s):sibling id(%d), state(%d)",
                    __FUNCTION__, curEntity->getPipeId(), curEntity->getEntityState());
        }

        while (curEntity != NULL) {
            if (curEntity->getEntityState() != ENTITY_STATE_COMPLETE) {
                ALOGD("DEBUG(%s):----- Child id(%d), state(%d)",
                        __FUNCTION__, curEntity->getPipeId(), curEntity->getEntityState());
            }
            curEntity = curEntity->getNextEntity();
        }
        r++;
    }

    return;
}

void ExynosCameraFrame::dump(void)
{
    printEntity();

    for (int i = 0; i < MAX_NUM_PIPES; i ++) {
        if (m_request[INDEX(i)] == true)
            ALOGI("INFO(%s[%d]):pipeId(%d)'s request is ture", __FUNCTION__, __LINE__, i);
    }
}

void ExynosCameraFrame::frameLock(void)
{
#ifndef USE_FRAME_REFERENCE_COUNT
    Mutex::Autolock lock(m_frameLock);
#endif
    m_frameLocked = true;
}

void ExynosCameraFrame::frameUnlock(void)
{
#ifndef USE_FRAME_REFERENCE_COUNT
        Mutex::Autolock lock(m_frameLock);
#endif
    m_frameLocked = false;
}

bool ExynosCameraFrame::getFrameLockState(void)
{
#ifndef USE_FRAME_REFERENCE_COUNT
        Mutex::Autolock lock(m_frameLock);
#endif
    return m_frameLocked;
}

status_t ExynosCameraFrame::initMetaData(struct camera2_shot_ext *shot)
{
    status_t ret = NO_ERROR;

    if (shot != NULL) {
        ALOGV("DEBUG(%s[%d]): initialize shot_ext", __FUNCTION__, __LINE__);
        memcpy(&m_metaData, shot, sizeof(struct camera2_shot_ext));
    }

    ret = m_parameters->duplicateCtrlMetadata(&m_metaData);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):duplicate Ctrl metadata fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    return ret;
}

status_t ExynosCameraFrame::getMetaData(struct camera2_shot_ext *shot)
{
    if (shot == NULL) {
        ALOGE("ERR(%s[%d]): buffer is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    memcpy(shot, &m_metaData, sizeof(struct camera2_shot_ext));

    return NO_ERROR;
}

status_t ExynosCameraFrame::setMetaData(struct camera2_shot_ext *shot)
{
    if (shot == NULL) {
        ALOGE("ERR(%s[%d]): buffer is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    memcpy(&m_metaData, shot, sizeof(struct camera2_shot_ext));

    return NO_ERROR;
}

status_t ExynosCameraFrame::storeDynamicMeta(struct camera2_shot_ext *shot)
{
    if (shot == NULL) {
        ALOGE("ERR(%s[%d]): buffer is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (getMetaDmRequestFrameCount(shot) == 0)
        ALOGW("WRN(%s[%d]): DM Frame count is ZERO", __FUNCTION__, __LINE__);

    memcpy(&m_metaData.shot.dm, &shot->shot.dm, sizeof(struct camera2_dm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::storeDynamicMeta(struct camera2_dm *dm)
{
    if (dm == NULL) {
        ALOGE("ERR(%s[%d]): buffer is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (getMetaDmRequestFrameCount(dm) == 0)
        ALOGW("WRN(%s[%d]): DM Frame count is ZERO", __FUNCTION__, __LINE__);

    memcpy(&m_metaData.shot.dm, dm, sizeof(struct camera2_dm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::storeUserDynamicMeta(struct camera2_shot_ext *shot)
{
    if (shot == NULL) {
        ALOGE("ERR(%s[%d]): buffer is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    memcpy(&m_metaData.shot.udm, &shot->shot.udm, sizeof(struct camera2_udm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::storeUserDynamicMeta(struct camera2_udm *udm)
{
    if (udm == NULL) {
        ALOGE("ERR(%s[%d]): buffer is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    memcpy(&m_metaData.shot.udm, udm, sizeof(struct camera2_udm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::getDynamicMeta(struct camera2_shot_ext *shot)
{
    if (shot == NULL) {
        ALOGE("ERR(%s[%d]): buffer is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    memcpy(&shot->shot.dm, &m_metaData.shot.dm, sizeof(struct camera2_dm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::getDynamicMeta(struct camera2_dm *dm)
{
    if (dm == NULL) {
        ALOGE("ERR(%s[%d]): buffer is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    memcpy(dm, &m_metaData.shot.dm, sizeof(struct camera2_dm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::getUserDynamicMeta(struct camera2_shot_ext *shot)
{
    if (shot == NULL) {
        ALOGE("ERR(%s[%d]): buffer is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    memcpy(&shot->shot.udm, &m_metaData.shot.udm, sizeof(struct camera2_udm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::getUserDynamicMeta(struct camera2_udm *udm)
{
    if (udm == NULL) {
        ALOGE("ERR(%s[%d]): buffer is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    memcpy(udm, &m_metaData.shot.udm, sizeof(struct camera2_udm));

    return NO_ERROR;
}

status_t ExynosCameraFrame::setMetaDataEnable(bool flag)
{
    m_metaDataEnable = flag;
    return NO_ERROR;
}

bool ExynosCameraFrame::getMetaDataEnable()
{
    long count = 0;

    while (count < DM_WAITING_COUNT) {
        if (m_metaDataEnable == true) {
            if (0 < count)
                ALOGD("DEBUG(%s[%d]): metadata enable count(%ld) ", __FUNCTION__, __LINE__, count);

            break;
        }

        count++;
        usleep(WAITING_TIME);
    }

    return m_metaDataEnable;
}

status_t ExynosCameraFrame::getNodeGroupInfo(struct camera2_node_group *node_group, int index)
{
    if (node_group == NULL) {
        ALOGE("ERR(%s[%d]): node_group is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (index >= PERFRAME_NODE_GROUP_MAX) {
        ALOGE("ERR(%s[%d]): index is bigger than PERFRAME_NODE_GROUP_MAX", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    memcpy(node_group, &m_node_gorup[index], sizeof(struct camera2_node_group));

    return NO_ERROR;
}

status_t ExynosCameraFrame::storeNodeGroupInfo(struct camera2_node_group *node_group, int index)
{
    if (node_group == NULL) {
        ALOGE("ERR(%s[%d]): node_group is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (index >= PERFRAME_NODE_GROUP_MAX) {
        ALOGE("ERR(%s[%d]): index is bigger than PERFRAME_NODE_GROUP_MAX", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    memcpy(&m_node_gorup[index], node_group, sizeof(struct camera2_node_group));

    return NO_ERROR;
}

status_t ExynosCameraFrame::getNodeGroupInfo(struct camera2_node_group *node_group, int index, int *zoom)
{
    getNodeGroupInfo(node_group, index);
    *zoom = m_zoom;

    return NO_ERROR;
}

status_t ExynosCameraFrame::storeNodeGroupInfo(struct camera2_node_group *node_group, int index, int zoom)
{
    storeNodeGroupInfo(node_group, index);
    m_zoom = zoom;

    return NO_ERROR;
}

void ExynosCameraFrame::dumpNodeGroupInfo(const char *name)
{
    if (name != NULL)
        ALOGD("INFO(%s[%d]):(%s)++++++++++++++++++++ frameCount(%d)", __FUNCTION__, __LINE__, name, m_frameCount);
    else
        ALOGD("INFO(%s[%d]):()++++++++++++++++++++ frameCount(%d)", __FUNCTION__, __LINE__, m_frameCount);

    for (int i = 0; i < PERFRAME_NODE_GROUP_MAX; i ++) {
        ALOGI("INFO(%s[%d]):Leader[%d] (%d, %d, %d, %d)(%d, %d, %d, %d)(%d %d)",
            __FUNCTION__, __LINE__,
            i,
            m_node_gorup[i].leader.input.cropRegion[0],
            m_node_gorup[i].leader.input.cropRegion[1],
            m_node_gorup[i].leader.input.cropRegion[2],
            m_node_gorup[i].leader.input.cropRegion[3],
            m_node_gorup[i].leader.output.cropRegion[0],
            m_node_gorup[i].leader.output.cropRegion[1],
            m_node_gorup[i].leader.output.cropRegion[2],
            m_node_gorup[i].leader.output.cropRegion[3],
            m_node_gorup[i].leader.request,
            m_node_gorup[i].leader.vid);

        for (int j = 0; j < CAPTURE_NODE_MAX; j ++) {
            ALOGI("INFO(%s[%d]):Capture[%d][%d] (%d, %d, %d, %d)(%d, %d, %d, %d)(%d, %d)",
                __FUNCTION__, __LINE__,
                i,
                j,
                m_node_gorup[i].capture[j].input.cropRegion[0],
                m_node_gorup[i].capture[j].input.cropRegion[1],
                m_node_gorup[i].capture[j].input.cropRegion[2],
                m_node_gorup[i].capture[j].input.cropRegion[3],
                m_node_gorup[i].capture[j].output.cropRegion[0],
                m_node_gorup[i].capture[j].output.cropRegion[1],
                m_node_gorup[i].capture[j].output.cropRegion[2],
                m_node_gorup[i].capture[j].output.cropRegion[3],
                m_node_gorup[i].capture[j].request,
                m_node_gorup[i].capture[j].vid);
        }

        if (name != NULL)
            ALOGD("INFO(%s[%d]):(%s)------------------------ ", __FUNCTION__, __LINE__, name);
        else
            ALOGD("INFO(%s[%d]):()------------------------ ", __FUNCTION__, __LINE__);
    }

    if (name != NULL)
        ALOGD("INFO(%s[%d]):(%s)++++++++++++++++++++", __FUNCTION__, __LINE__, name);
    else
        ALOGD("INFO(%s[%d]):()++++++++++++++++++++", __FUNCTION__, __LINE__);

    return;
}

void ExynosCameraFrame::setJpegSize(int size)
{
    m_jpegSize = size;
}

int ExynosCameraFrame::getJpegSize(void)
{
    return m_jpegSize;
}

int64_t ExynosCameraFrame::getTimeStamp(void)
{
    return (int64_t)getMetaDmSensorTimeStamp(&m_metaData);
}

void ExynosCameraFrame::getFpsRange(uint32_t *min, uint32_t *max)
{
    getMetaCtlAeTargetFpsRange(&m_metaData, min, max);
}

void ExynosCameraFrame::setRequest(bool tap,
                                   bool tac,
                                   bool isp,
                                   bool scc,
                                   bool dis,
                                   bool scp)
{
    m_request[PIPE_3AP] = tap;
    m_request[PIPE_3AC] = tac;
    m_request[PIPE_ISP] = isp;
    m_request[PIPE_SCC] = scc;
    m_request[PIPE_DIS] = dis;
    m_request[PIPE_SCP] = scp;
}

void ExynosCameraFrame::setRequest(bool tap,
                                   bool tac,
                                   bool isp,
                                   bool ispp,
                                   bool ispc,
                                   bool scc,
                                   bool dis,
                                   bool scp)
{
    setRequest(tap,
               tac,
               isp,
               scc,
               dis,
               scp);

    m_request[PIPE_ISPP] = ispp;
    m_request[PIPE_ISPC] = ispc;
}

void ExynosCameraFrame::setRequest(uint32_t pipeId, bool val)
{
    switch (pipeId) {
    case PIPE_3AC_FRONT:
    case PIPE_3AC_REPROCESSING:
        pipeId = PIPE_3AC;
        break;
    case PIPE_3AP_FRONT:
    case PIPE_3AP_REPROCESSING:
        pipeId = PIPE_3AP;
        break;
    case PIPE_ISP_FRONT:
        pipeId = PIPE_ISP;
        break;
    case PIPE_ISPC_FRONT:
    case PIPE_ISPC_REPROCESSING:
        pipeId = PIPE_ISPC;
        break;
    case PIPE_SCC_FRONT:
    case PIPE_SCC_REPROCESSING:
        pipeId = PIPE_SCC;
        break;
    case PIPE_SCP_FRONT:
    case PIPE_SCP_REPROCESSING:
        pipeId = PIPE_SCP;
        break;
    default:
        break;
    }

    if ((pipeId % 100) >= MAX_NUM_PIPES)
        ALOGW("WRN(%s[%d]):Invalid pipeId(%d)", __FUNCTION__, __LINE__, pipeId);
    else
        m_request[INDEX(pipeId)] = val;
}

bool ExynosCameraFrame::getRequest(uint32_t pipeId)
{
    bool request = false;

    switch (pipeId) {
    case PIPE_3AC_FRONT:
    case PIPE_3AC_REPROCESSING:
        pipeId = PIPE_3AC;
        break;
    case PIPE_3AP_FRONT:
    case PIPE_3AP_REPROCESSING:
        pipeId = PIPE_3AP;
        break;
    case PIPE_ISP_FRONT:
        pipeId = PIPE_ISP;
        break;
    case PIPE_ISPC_FRONT:
    case PIPE_ISPC_REPROCESSING:
        pipeId = PIPE_ISPC;
        break;
    case PIPE_SCC_FRONT:
    case PIPE_SCC_REPROCESSING:
        pipeId = PIPE_SCC;
        break;
    case PIPE_SCP_FRONT:
    case PIPE_SCP_REPROCESSING:
        pipeId = PIPE_SCP;
        break;
    default:
        break;
    }

    if ((pipeId % 100) >= MAX_NUM_PIPES)
        ALOGW("WRN(%s[%d]):Invalid pipeId(%d)", __FUNCTION__, __LINE__, pipeId);
    else
        request = m_request[INDEX(pipeId)];

    return request;
}

bool ExynosCameraFrame::getIspDone(void)
{
    return m_ispDoneFlag;
}

void ExynosCameraFrame::setIspDone(bool done)
{
    m_ispDoneFlag = done;
}

bool ExynosCameraFrame::get3aaDrop()
{
    return m_3aaDropFlag;
}

void ExynosCameraFrame::set3aaDrop(bool flag)
{
    m_3aaDropFlag = flag;
}

void ExynosCameraFrame::setIspcDrop(bool flag)
{
    m_ispcDropFlag = flag;
}

bool ExynosCameraFrame::getIspcDrop(void)
{
    return m_ispcDropFlag;
}

void ExynosCameraFrame::setDisDrop(bool flag)
{
    m_disDropFlag = flag;
}

bool ExynosCameraFrame::getDisDrop(void)
{
    return m_disDropFlag;
}

bool ExynosCameraFrame::getScpDrop()
{
    return m_scpDropFlag;
}

void ExynosCameraFrame::setScpDrop(bool flag)
{
    m_scpDropFlag = flag;
}

bool ExynosCameraFrame::getSccDrop()
{
    return m_sccDropFlag;
}

void ExynosCameraFrame::setSccDrop(bool flag)
{
    m_sccDropFlag = flag;
}

uint32_t ExynosCameraFrame::getUniqueKey(void)
{
    return m_uniqueKey;
}

status_t ExynosCameraFrame::setUniqueKey(uint32_t uniqueKey)
{
    m_uniqueKey = uniqueKey;
    return NO_ERROR;
}

#ifdef USE_FRAME_REFERENCE_COUNT
int32_t ExynosCameraFrame::incRef()
{
    Mutex::Autolock lock(m_refCountLock);
    m_refCount++;
    return m_refCount;
}

int32_t ExynosCameraFrame::decRef()
{
    Mutex::Autolock lock(m_refCountLock);
    m_refCount--;
    if (m_refCount < 0)
        ALOGE("ERR(%s[%d]):reference count do not have negatve value, m_refCount(%d)", __FUNCTION__, __LINE__, m_refCount);
    return m_refCount;
}

int32_t ExynosCameraFrame::getRef()
{
    Mutex::Autolock lock(m_refCountLock);
    return m_refCount;
}
#endif

status_t ExynosCameraFrame::setFrameInfo(ExynosCameraParameters *obj_param, uint32_t frameCount, uint32_t frameType)
{
    status_t ret = NO_ERROR;

    m_parameters = obj_param;
    m_frameCount = frameCount;
    m_frameType = frameType;
    return ret;
}

uint32_t ExynosCameraFrame::getFrameType()
{
    return m_frameType;
}

status_t ExynosCameraFrame::m_init()
{
    m_numRequestPipe = 0;
    m_numCompletePipe = 0;
    m_frameState = FRAME_STATE_READY;
    m_frameLocked = false;
    m_metaDataEnable = false;
    m_zoom = 0;
    memset(&m_metaData, 0x0, sizeof(struct camera2_shot_ext));
    m_jpegSize = 0;
    m_ispDoneFlag = false;
    m_3aaDropFlag = false;
    m_ispcDropFlag = false;
    m_disDropFlag = false;
    m_scpDropFlag = false;
    m_sccDropFlag = false;

    for (int i = 0; i < MAX_NUM_PIPES; i++)
        m_request[i] = false;

    m_uniqueKey = 0;
    m_capture = 0;
    m_recording = false;
    m_preview = false;
    m_previewCb = false;
    m_serviceBayer = false;
    m_zsl = false;

#ifdef USE_FRAME_REFERENCE_COUNT
    m_refCount = 1;
#endif

    for (int i = 0; i < PERFRAME_NODE_GROUP_MAX; i++)
        memset(&m_node_gorup[i], 0x0, sizeof(struct camera2_node_group));
    ALOGV("DEBUG(%s[%d]): Generate frame type(%d), frameCount(%d)", __FUNCTION__, __LINE__, m_frameType, m_frameCount);

#ifdef DEBUG_FRAME_MEMORY_LEAK
    m_privateCheckLeakCount = 0;
    m_countLock.lock();
    m_checkLeakCount ++;
    m_privateCheckLeakCount = m_checkLeakCount;
    ALOGE("CONSTRUCTOR (%lld)", m_privateCheckLeakCount);
    m_countLock.unlock();
#endif

    m_dupBufferInfo.streamID = 0;
    m_dupBufferInfo.extScalerPipeID = 0;

    return NO_ERROR;
}

status_t ExynosCameraFrame::m_deinit()
{
    ALOGV("DEBUG(%s[%d]): Delete frame type(%d), frameCount(%d)", __FUNCTION__, __LINE__, m_frameType, m_frameCount);
#ifdef DEBUG_FRAME_MEMORY_LEAK
    ALOGI("DESTRUCTOR (%lld)", m_privateCheckLeakCount);
#endif

    List<ExynosCameraFrameEntity *>::iterator r;
    ExynosCameraFrameEntity *curEntity = NULL;
    ExynosCameraFrameEntity *tmpEntity = NULL;

    Mutex::Autolock l(m_linkageLock);
    while (!m_linkageList.empty()) {
        r = m_linkageList.begin()++;
        if (*r) {
            curEntity = *r;

            while (curEntity != NULL) {
                tmpEntity = curEntity->getNextEntity();
                ALOGV("DEBUG(%s[%d])", __FUNCTION__, curEntity->getPipeId());

                delete curEntity;
                curEntity = tmpEntity;
            }

        }
        m_linkageList.erase(r);
    }

    return NO_ERROR;
}

status_t ExynosCameraFrame::setRotation(uint32_t pipeId, int rotation)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->setRotation(rotation);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):pipeId(%d)->setRotation(%d) fail", __FUNCTION__, __LINE__, pipeId, rotation);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::getRotation(uint32_t pipeId)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    return entity->getRotation();
}

#ifdef PERFRAME_CONTROL_FOR_FLIP
status_t ExynosCameraFrame::setFlipHorizontal(uint32_t pipeId, int flipHorizontal)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->setFlipHorizontal(flipHorizontal);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):pipeId(%d)->setRotation(%d) fail", __FUNCTION__, __LINE__, pipeId, flipHorizontal);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::getFlipHorizontal(uint32_t pipeId)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    return entity->getFlipHorizontal();
}

status_t ExynosCameraFrame::setFlipVertical(uint32_t pipeId, int flipVertical)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    ret = entity->setFlipVertical(flipVertical);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):pipeId(%d)->setRotation(%d) fail", __FUNCTION__, __LINE__, pipeId, flipVertical);
        return ret;
    }

    return ret;
}

status_t ExynosCameraFrame::getFlipVertical(uint32_t pipeId)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrameEntity *entity = searchEntityByPipeId(pipeId);
    if (entity == NULL) {
        ALOGE("ERR(%s[%d]):Could not find entity, pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    return entity->getFlipVertical();
}
#endif

/*
 * ExynosCameraFrameEntity class
 */

ExynosCameraFrameEntity::ExynosCameraFrameEntity(
        uint32_t pipeId,
        entity_type_t type,
        entity_buffer_type_t bufType)
{
    m_pipeId = pipeId;

    if (m_setEntityType(type) != NO_ERROR)
        ALOGE("ERR(%s[%d]):setEntityType fail, pipeId(%d), type(%d)", __FUNCTION__, __LINE__, pipeId, type);

    m_bufferType = bufType;
    m_entityState = ENTITY_STATE_READY;

    m_prevEntity = NULL;
    m_nextEntity = NULL;

    m_flagSpecificParent = false;
    m_parentPipeId = -1;

    m_rotation = 0;

#ifdef PERFRAME_CONTROL_FOR_FLIP
    m_flipHorizontal = 0;
    m_flipVertical = 0;
#endif
}

status_t ExynosCameraFrameEntity::m_setEntityType(entity_type_t type)
{
    status_t ret = NO_ERROR;

    m_EntityType = type;

    switch (type) {
    case ENTITY_TYPE_INPUT_ONLY:
        m_srcBufState = ENTITY_BUFFER_STATE_REQUESTED;
        m_dstBufState[DST_BUFFER_DEFAULT] = ENTITY_BUFFER_STATE_NOREQ;
        break;
    case ENTITY_TYPE_OUTPUT_ONLY:
        m_srcBufState = ENTITY_BUFFER_STATE_NOREQ;
        m_dstBufState[DST_BUFFER_DEFAULT] = ENTITY_BUFFER_STATE_REQUESTED;
        break;
    case ENTITY_TYPE_INPUT_OUTPUT:
        m_srcBufState = ENTITY_BUFFER_STATE_REQUESTED;
        m_dstBufState[DST_BUFFER_DEFAULT] = ENTITY_BUFFER_STATE_REQUESTED;
        break;
    default:
        m_srcBufState = ENTITY_BUFFER_STATE_NOREQ;
        m_dstBufState[DST_BUFFER_DEFAULT] = ENTITY_BUFFER_STATE_NOREQ;
        m_EntityType = ENTITY_TYPE_INVALID;
        ret = BAD_VALUE;
        break;
    }

    return ret;
}

uint32_t ExynosCameraFrameEntity::getPipeId(void)
{
    return m_pipeId;
}

status_t ExynosCameraFrameEntity::setSrcBuf(ExynosCameraBuffer buf)
{
    status_t ret = NO_ERROR;

    if (m_srcBufState == ENTITY_BUFFER_STATE_COMPLETE) {
        ALOGV("WRN(%s[%d]):Buffer completed, state(%d)", __FUNCTION__, __LINE__, m_srcBufState);
        return NO_ERROR;
    }

    if (m_bufferType != ENTITY_BUFFER_DELIVERY &&
        m_srcBufState != ENTITY_BUFFER_STATE_REQUESTED) {
        ALOGE("ERR(%s[%d]):Invalid buffer state(%d)", __FUNCTION__, __LINE__, m_srcBufState);
        return INVALID_OPERATION;
    }

    this->m_srcBuf = buf;

    ret = setSrcBufState(ENTITY_BUFFER_STATE_READY);

    return ret;
}

status_t ExynosCameraFrameEntity::setDstBuf(ExynosCameraBuffer buf, uint32_t nodeIndex)
{
    status_t ret = NO_ERROR;

    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        ALOGE("ERR(%s[%d]):Invalid buffer index, index(%d)", __FUNCTION__, __LINE__, nodeIndex);
        return BAD_VALUE;
    }

    if (m_bufferType != ENTITY_BUFFER_DELIVERY &&
        m_dstBufState[nodeIndex] != ENTITY_BUFFER_STATE_REQUESTED) {
        ALOGE("ERR(%s[%d]):Invalid buffer state(%d)", __FUNCTION__, __LINE__, m_dstBufState[nodeIndex]);
        return INVALID_OPERATION;
    }

    this->m_dstBuf[nodeIndex] = buf;
    ret = setDstBufState(ENTITY_BUFFER_STATE_READY, nodeIndex);

#ifndef SUPPORT_DEPTH_MAP
    /* HACK: Combine with old pipe */
    if (nodeIndex != DST_BUFFER_DEFAULT)
        this->m_dstBuf[DST_BUFFER_DEFAULT] = buf;
#endif

    return ret;
}

status_t ExynosCameraFrameEntity::getSrcBuf(ExynosCameraBuffer *buf)
{
    *buf = this->m_srcBuf;

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::getDstBuf(ExynosCameraBuffer *buf, uint32_t nodeIndex)
{
    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        ALOGE("ERR(%s[%d]):Invalid buffer index, index(%d)", __FUNCTION__, __LINE__, nodeIndex);
        return BAD_VALUE;
    }

    /* Comment out: It was collide with ExynosCamera's dirty dynamic bayer handling routine.
     * (make error log, but no side effect)
     * This code added for block human error.
     * I will add this code after check the side effect closely.
     */
    /*
    if (this->m_dstBuf[nodeIndex].index == -1) {
        ALOGE("ERR(%s[%d]):Invalid buffer index(%d)", __FUNCTION__, __LINE__, nodeIndex);
        return BAD_VALUE;
    }
    */

    *buf = this->m_dstBuf[nodeIndex];

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::setSrcRect(ExynosRect rect)
{
    this->m_srcRect = rect;

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::setDstRect(ExynosRect rect)
{
    this->m_dstRect = rect;

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::getSrcRect(ExynosRect *rect)
{
    *rect = this->m_srcRect;

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::getDstRect(ExynosRect *rect)
{
    *rect = this->m_dstRect;

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::setSrcBufState(entity_buffer_state_t state)
{
    if (m_srcBufState == ENTITY_BUFFER_STATE_COMPLETE) {
        ALOGV("WRN(%s[%d]):Buffer completed, state(%d)", __FUNCTION__, __LINE__, m_srcBufState);
        return NO_ERROR;
    }

    m_srcBufState = state;
    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::setDstBufState(entity_buffer_state_t state, uint32_t nodeIndex)
{
    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        ALOGE("ERR(%s[%d]):Invalid buffer index, index(%d)", __FUNCTION__, __LINE__, nodeIndex);
        return BAD_VALUE;
    }

    m_dstBufState[nodeIndex] = state;

#ifndef SUPPORT_DEPTH_MAP
    /* HACK: Combine with old pipe */
    if (nodeIndex != DST_BUFFER_DEFAULT)
        m_dstBufState[DST_BUFFER_DEFAULT] = state;
#endif

    return NO_ERROR;
}

entity_buffer_state_t ExynosCameraFrameEntity::getSrcBufState(void)
{
    return m_srcBufState;
}

entity_buffer_state_t ExynosCameraFrameEntity::getDstBufState(uint32_t nodeIndex)
{
    if (nodeIndex >= DST_BUFFER_COUNT_MAX) {
        ALOGE("ERR(%s[%d]):Invalid buffer index, index(%d)", __FUNCTION__, __LINE__, nodeIndex);
        return ENTITY_BUFFER_STATE_INVALID;
    }

    return m_dstBufState[nodeIndex];
}

entity_buffer_type_t ExynosCameraFrameEntity::getBufType(void)
{
    return m_bufferType;
}

status_t ExynosCameraFrameEntity::setEntityState(entity_state_t state)
{
    this->m_entityState = state;

    return NO_ERROR;
}

entity_state_t ExynosCameraFrameEntity::getEntityState(void)
{
    return this->m_entityState;
}

ExynosCameraFrameEntity *ExynosCameraFrameEntity::getPrevEntity(void)
{
    return this->m_prevEntity;
}

ExynosCameraFrameEntity *ExynosCameraFrameEntity::getNextEntity(void)
{
    return this->m_nextEntity;
}

status_t ExynosCameraFrameEntity::setPrevEntity(ExynosCameraFrameEntity *entity)
{
    this->m_prevEntity = entity;

    return NO_ERROR;
}

status_t ExynosCameraFrameEntity::setNextEntity(ExynosCameraFrameEntity *entity)
{
    this->m_nextEntity = entity;

    return NO_ERROR;
}

bool ExynosCameraFrameEntity::flagSpecficParent(void)
{
    return m_flagSpecificParent;
}

status_t ExynosCameraFrameEntity::setParentPipeId(enum pipeline parentPipeId)
{
    if (0 <= m_parentPipeId) {
        ALOGE("ERR(%s[%d]):m_parentPipeId(%d) is already set. parentPipeId(%d)",
            __FUNCTION__, __LINE__, m_parentPipeId, parentPipeId);
        return BAD_VALUE;
    }

    m_flagSpecificParent = true;
    m_parentPipeId = parentPipeId;

    return NO_ERROR;
}

int ExynosCameraFrameEntity::getParentPipeId(void)
{
    return m_parentPipeId;
}

status_t ExynosCameraFrameEntity::setRotation(int rotation)
{
    m_rotation = rotation;

    return NO_ERROR;
}

int ExynosCameraFrameEntity::getRotation(void)
{
    return m_rotation;
}

#ifdef PERFRAME_CONTROL_FOR_FLIP
status_t ExynosCameraFrameEntity::setFlipHorizontal(int flipHorizontal)
{
    m_flipHorizontal = flipHorizontal;

    return NO_ERROR;
}

int ExynosCameraFrameEntity::getFlipHorizontal(void)
{
    return m_flipHorizontal;
}

status_t ExynosCameraFrameEntity::setFlipVertical(int flipVertical)
{
    m_flipVertical = flipVertical;

    return NO_ERROR;
}

int ExynosCameraFrameEntity::getFlipVertical(void)
{
    return m_flipVertical;
}
#endif

}; /* namespace android */
