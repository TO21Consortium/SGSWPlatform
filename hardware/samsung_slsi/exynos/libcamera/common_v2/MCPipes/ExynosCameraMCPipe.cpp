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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraMCPipe"
#include <cutils/log.h>

#include "ExynosCameraMCPipe.h"

namespace android {

#ifdef USE_MCPIPE_SERIALIZATION_MODE
Mutex ExynosCameraMCPipe::g_serializationLock;
#endif

ExynosCameraMCPipe::~ExynosCameraMCPipe()
{
    this->destroy();
}

status_t ExynosCameraMCPipe::create(int32_t *sensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    ret = m_preCreate();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_preCreate() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    ret = m_postCreate(sensorIds);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_postCreate() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraMCPipe::precreate(__unused int32_t *sensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    ret = m_preCreate();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_preCreate() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    return ret;
}

status_t ExynosCameraMCPipe::postcreate(int32_t *sensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    ret = m_postCreate(sensorIds);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_postCreate() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    CLOGI("INFO(%s[%d]):postcreate() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId());

    return ret;
}

status_t ExynosCameraMCPipe::destroy(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    for (int i = (MAX_NODE - 1); i >= OUTPUT_NODE; i--) {
        if (m_node[i] != NULL) {
            if (OUTPUT_NODE < i
                && m_node[OUTPUT_NODE] != NULL
                && m_deviceInfo->nodeNum[OUTPUT_NODE] == m_deviceInfo->nodeNum[i]) {
                /* In this case(3AA of 54xx), 3AA, 3AP node is same.
                 * So, should close one node. Skip 3AP node.
                 */
            } else {
                ret = m_node[i]->close();
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Main node(%s) close fail, ret(%d)",
                            __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                    return ret;
                }
            }

            SAFE_DELETE(m_node[i]);
            CLOGD("DEBUG(%s[%d]):Main node(%s, sensorIds:%d) closed",
                    __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], m_sensorIds[i]);
        }
    }

    for (int i = (MAX_NODE - 1); i >= OUTPUT_NODE; i--) {
        if (m_secondaryNode[i] != NULL) {
            ret = m_secondaryNode[i]->close();
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):secondary node(%s) close fail, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->secondaryNodeName[i], ret);
                return ret;
            }
            SAFE_DELETE(m_secondaryNode[i]);
            CLOGD("DEBUG(%s[%d]):secondary node(%s, sensorIds:%d) closed",
                    __FUNCTION__, __LINE__, m_deviceInfo->secondaryNodeName[i], m_secondarySensorIds[i]);
        }
    }

    CLOGD("DEBUG(%s[%d]):Node destroyed", __FUNCTION__, __LINE__);

    if (m_inputFrameQ != NULL) {
        m_inputFrameQ->release();
        SAFE_DELETE(m_inputFrameQ);
    }

    if (m_requestFrameQ != NULL) {
        m_requestFrameQ->release();
        SAFE_DELETE(m_requestFrameQ);
    }

    CLOGI("INFO(%s[%d]):destroy() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId());

    return ret;
}

status_t ExynosCameraMCPipe::setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds)
{
    status_t ret = NO_ERROR;

    ret = this->setupPipe(pipeInfos, sensorIds, NULL);

    return ret;
}

status_t ExynosCameraMCPipe::setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds, int32_t *secondarySensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;
    uint32_t pipeId = 0;
    int result = 0;
    ExynosCameraNode *setFileSettingNode = NULL;

    /* TODO: check node state */

    /* set new sensorId to m_sensorIds */
    if (sensorIds != NULL) {
        CLOGD("DEBUG(%s[%d]):set new sensorIds", __FUNCTION__, __LINE__);
        for (int i = OUTPUT_NODE; i < MAX_NODE; i++)
            m_sensorIds[i] = sensorIds[i];
    }

    if (secondarySensorIds != NULL) {
        CLOGD("DEBUG(%s[%d]):set new ispSensorIds", __FUNCTION__, __LINE__);
        for (int i = OUTPUT_NODE; i < MAX_NODE; i++)
            m_secondarySensorIds[i] = secondarySensorIds[i];
    }

    ret = m_setInput(m_node, m_deviceInfo->nodeNum, m_sensorIds);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_setInput(Main) fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    ret = m_setInput(m_secondaryNode, m_deviceInfo->secondaryNodeNum, m_secondarySensorIds);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_setInput(secondary) fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    if (pipeInfos != NULL) {
        ret = m_setPipeInfo(pipeInfos);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_setPipeInfo() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }
    } else {
        CLOGE("ERR(%s[%d]):pipeInfos is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        result = getPipeId((enum NODE_TYPE)i);
        if (0 <= result) {
            if (m_node[i] != NULL)
                setFileSettingNode = m_node[i];
            else if (m_secondaryNode[i] != NULL)
                setFileSettingNode = m_secondaryNode[i];
            else
                continue;

            pipeId = (uint32_t)result;

            ret = m_setSetfile(setFileSettingNode, pipeId);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_setSetfile() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                return ret;
            }
        }

        for (uint32_t j = 0; j < m_numBuffers[i]; j++) {
            m_runningFrameList[i][j] = NULL;
        }
        m_numOfRunningFrame[i] = 0;
    }

    m_prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];

    CLOGI("INFO(%s[%d]):setupPipe() is succeed, Pipe(%d), prepare(%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    return ret;
}

status_t ExynosCameraMCPipe::prepare(void)
{
    /* need modify */
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;
    bool retVal = true;

    /*
     * prepare on only capture node
     * output node doesn't need prepare
     */
    for (uint32_t i = 0; i < m_prepareBufferCount; i++) {
        retVal = m_putBufferThreadFunc();
        if (retVal == false) {
            CLOGE("WRN(%s):m_putBufferThreadFunc no Frame(count = %d)", __FUNCTION__, m_inputFrameQ->getSizeOfProcessQ());
            ret = INVALID_OPERATION;
        }
    }

    CLOGI("INFO(%s[%d]):prepare() is succeed, Pipe(%d), prepare(%d)",
            __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    return ret;
}

status_t ExynosCameraMCPipe::start(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    /* TODO: check state ready for start */
    ret = m_startNode();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_startNode() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    m_threadState = 0;
    m_threadRenew = 0;

    m_flagStartPipe = true;
    m_flagTryStop = false;

    CLOGI("INFO(%s[%d]):start() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId());

    return ret;
}

status_t ExynosCameraMCPipe::stop(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    ret = m_stopNode();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_stopNode() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    m_putBufferThread->requestExitAndWait();
    m_getBufferThread->requestExitAndWait();

    CLOGD("DEBUG(%s[%d]):Thread exited", __FUNCTION__, __LINE__);

    m_inputFrameQ->release();
    m_requestFrameQ->release();

#ifdef USE_MCPIPE_SERIALIZATION_MODE
    if (m_serializeOperation == true) {
        ExynosCameraMCPipe::g_serializationLock.unlock();
        CLOGD("DEBUG(%s[%d]):%s Critical Section END",
                __FUNCTION__, __LINE__, m_name);
    }
#endif

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        for (uint32_t j = 0; j < m_numBuffers[i]; j++) {
            m_runningFrameList[i][j] = NULL;
        }
        m_numOfRunningFrame[i] = 0;
        m_skipBuffer[i].index = -2;
        m_skipPutBuffer[i] = false;

        if (m_node[i] != NULL)
            m_node[i]->removeItemBufferQ();

    }

    m_flagStartPipe = false;
    m_flagTryStop = false;

    CLOGI("INFO(%s[%d]):stop() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId());

    return ret;
}

bool ExynosCameraMCPipe::flagStart(void)
{
    return m_flagStartPipe;
}

status_t ExynosCameraMCPipe::startThread(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    if (m_outputFrameQ == NULL) {
        CLOGE("ERR(%s[%d]):outputFrameQ is NULL, cannot start", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    m_putBufferThread->run(PRIORITY_URGENT_DISPLAY);
    m_getBufferThread->run(PRIORITY_URGENT_DISPLAY);

    CLOGI("INFO(%s[%d]):startThread is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId());

    return ret;
}

status_t ExynosCameraMCPipe::stopThread(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    m_putBufferThread->requestExit();
    m_getBufferThread->requestExit();

    m_inputFrameQ->sendCmd(WAKE_UP);
    m_requestFrameQ->sendCmd(WAKE_UP);

#ifdef USE_MCPIPE_SERIALIZATION_MODE
    if (m_serializeOperation == true) {
        ExynosCameraMCPipe::g_serializationLock.unlock();
        CLOGD("DEBUG(%s[%d]):%s Critical Section END",
                __FUNCTION__, __LINE__, m_name);
    }
#endif

    m_dumpRunningFrameList();

    CLOGI("INFO(%s[%d]):stopThread is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId());

    return ret;
}

status_t ExynosCameraMCPipe::stopThreadAndWait(int sleep, int times)
{
    CLOGD("DEBUG(%s[%d]) IN", __FUNCTION__, __LINE__);
    status_t status = NO_ERROR;
    int i = 0;

    for (i = 0; i < times ; i++) {
        if (m_putBufferThread->isRunning() == false && m_getBufferThread->isRunning() == false) {
            break;
        }
        usleep(sleep * 1000);
    }

    if (i >= times) {
        status = TIMED_OUT;
        CLOGE("ERR(%s[%d]): stopThreadAndWait failed, waitTime(%d)ms", __FUNCTION__, __LINE__, sleep*times);
    }

    CLOGD("DEBUG(%s[%d]) OUT", __FUNCTION__, __LINE__);
    return status;
}

bool ExynosCameraMCPipe::flagStartThread(void)
{
    return m_putBufferThread->isRunning();
}

status_t ExynosCameraMCPipe::sensorStream(bool on)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    int value = on ? IS_ENABLE_STREAM : IS_DISABLE_STREAM;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->setControl(V4L2_CID_IS_S_STREAM, value);
            if (ret != NO_ERROR)
                CLOGE("ERR(%s[%d]):sensorStream failed, %s node, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);

            return ret;
        }
    }

    CLOGE("ERR(%s[%d]):All Nodes is NULL", __FUNCTION__, __LINE__);

    return INVALID_OPERATION;
}

status_t ExynosCameraMCPipe::forceDone(unsigned int cid, int value)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    if (m_node[OUTPUT_NODE] == NULL) {
        CLOGE("ERR(%s[%d]):m_node[OUTPUT_NODE] is NULL", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    ret = m_forceDone(m_node[OUTPUT_NODE], cid, value);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s):m_forceDone() is failed, ret", __FUNCTION__);
        return ret;
    }

    CLOGI("INFO(%s[%d]):forceDone() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId());

    return ret;
}

status_t ExynosCameraMCPipe::setControl(int cid, int value)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->setControl(cid, value);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_node(%s)->setControl failed, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                return ret;
            }
            CLOGI("INFO(%s[%d]):setControl() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId((enum NODE_TYPE)i));
            return ret;
        }
    }

    CLOGE("ERR(%s[%d]):All nodes is NULL", __FUNCTION__, __LINE__);

    return INVALID_OPERATION;
}

status_t ExynosCameraMCPipe::setControl(int cid, int value, enum NODE_TYPE nodeType)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    if (m_node[nodeType] == NULL) {
        CLOGE("ERR(%s[%d]):m_node[%d] == NULL. so, fail", __FUNCTION__, __LINE__, nodeType);
        return INVALID_OPERATION;
    }

    ret = m_node[nodeType]->setControl(cid, value);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_node(%s)->setControl failed, ret(%d)",
                __FUNCTION__, __LINE__, m_deviceInfo->nodeName[nodeType], ret);
        return ret;
    }
    CLOGI("INFO(%s[%d]):setControl() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId());
    return ret;
}

status_t ExynosCameraMCPipe::getControl(int cid, int *value)
{
    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;
    uint32_t nodeCount = 0;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->getControl(cid, value);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_node(%s)->getControl failed, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                return ret;
            }
            CLOGI("INFO(%s[%d]):getControl() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId((enum NODE_TYPE)i));
            return ret;
        }
    }

    CLOGE("ERR(%s[%d]):All nodes is NULL", __FUNCTION__, __LINE__);

    return INVALID_OPERATION;
}

status_t ExynosCameraMCPipe::getControl(int cid, int *value, enum NODE_TYPE nodeType)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    if (m_node[nodeType] == NULL) {
        CLOGE("ERR(%s[%d]):m_node[%d] == NULL. so, fail", __FUNCTION__, __LINE__, nodeType);
        return INVALID_OPERATION;
    }

    ret = m_node[nodeType]->getControl(cid, value);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_node(%s)->getControl failed, ret(%d)",
                __FUNCTION__, __LINE__, m_deviceInfo->nodeName[nodeType], ret);
        return ret;
    }
    CLOGI("INFO(%s[%d]):getControl() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId());
    return ret;
}

status_t ExynosCameraMCPipe::setExtControl(struct v4l2_ext_controls *ctrl)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->setExtControl(ctrl);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_node(%s)->setControl failed, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                return ret;
            }
            CLOGI("INFO(%s[%d]):setControl() is succeed, Pipe(%d)",
                    __FUNCTION__, __LINE__, getPipeId((enum NODE_TYPE)i));
            return ret;
        }
    }

    CLOGE("ERR(%s[%d]):All nodes is NULL", __FUNCTION__, __LINE__);

    return INVALID_OPERATION;
}

status_t ExynosCameraMCPipe::setExtControl(struct v4l2_ext_controls *ctrl, enum NODE_TYPE nodeType)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    if (m_node[nodeType] == NULL) {
        CLOGE("ERR(%s[%d]):m_node[%d] == NULL. so, fail",
                __FUNCTION__, __LINE__, nodeType);
        return INVALID_OPERATION;
    }

    ret = m_node[nodeType]->setExtControl(ctrl);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_node(%s)->setControl failed, ret(%d)",
                __FUNCTION__, __LINE__, m_deviceInfo->nodeName[nodeType], ret);
        return ret;
    }
    CLOGI("INFO(%s[%d]):setControl() is succeed, Pipe(%d)",
            __FUNCTION__, __LINE__, getPipeId());
    return ret;
}

status_t ExynosCameraMCPipe::setParam(struct v4l2_streamparm streamParam)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;
    uint32_t nodeCount = 0;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->setParam(&streamParam);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_node(%s)->setParam failed, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                return ret;
            }
            CLOGI("INFO(%s[%d]):setParam() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId((enum NODE_TYPE)i));
            return ret;
        }
    }

    CLOGE("ERR(%s[%d]):All nodes is NULL", __FUNCTION__, __LINE__);

    return INVALID_OPERATION;
}

status_t ExynosCameraMCPipe::pushFrame(ExynosCameraFrame **newFrame)
{
    Mutex::Autolock lock(m_pipeframeLock);
    if (newFrame == NULL) {
        CLOGE("ERR(%s):newFrame is NULL", __FUNCTION__);
        return BAD_VALUE;
    }

    m_inputFrameQ->pushProcessQ(newFrame);

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::instantOn(int32_t numFrames)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;
    uint32_t nodeCount = 0;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer newBuffer;

    if (m_inputFrameQ->getSizeOfProcessQ() != numFrames) {
        CLOGE("ERR(%s[%d]):instantOn need %d Frames, but %d Frames are queued",
                __FUNCTION__, __LINE__, numFrames, m_inputFrameQ->getSizeOfProcessQ());
        return BAD_VALUE;
    }

    for (int i = (OTF_NODE_BASE - 1); i >= OUTPUT_NODE; i--) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->start();
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):node(%s) instantOn fail, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                return ret;
            }
            nodeCount++;
        }
    }

    if (nodeCount == 0) {
        CLOGE("ERR(%s[%d]):All nodes is NULL", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    for (int i = 0; i < numFrames; i++) {
        ret = m_inputFrameQ->popProcessQ(&newFrame);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }

        if (newFrame == NULL) {
            CLOGE("ERR(%s[%d]):newFrame is NULL", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }

        ret = newFrame->getSrcBuffer(getPipeId(), &newBuffer);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Frame get buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }

        if (m_node[OUTPUT_NODE] != NULL) {
            CLOGD("DEBUG(%s[%d]):Put instantOn Buffer (index %d)", __FUNCTION__, __LINE__, newBuffer.index);

            ret = m_node[OUTPUT_NODE]->putBuffer(&newBuffer);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):putBuffer() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                return ret;
                /* TODO: doing exception handling */
            }
        } else {
            CLOGE("ERR(%s[%d]):m_node[OUTPUT_NODE] is NULL", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }
    }

    CLOGI("INFO(%s[%d]):instantOn() is succeed, Pipe(%d), Frames(%d)",
            __FUNCTION__, __LINE__, getPipeId(), numFrames);

    return ret;
}

/* Don't use this function, this is regacy code */
status_t ExynosCameraMCPipe::instantOnQbuf(ExynosCameraFrame **frame, BUFFER_POS::POS pos)
{
    if (m_node[OUTPUT_NODE] == NULL) {
        CLOGE("ERR(%s): m_mainNode == NULL. so, fail", __FUNCTION__);
        return INVALID_OPERATION;
    }

    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer newBuffer;
    int ret = 0;
    ret = m_inputFrameQ->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW("WARN(%s):wait timeout", __FUNCTION__);
            m_mainNode->dumpState();
        } else {
            CLOGE("ERR(%s):wait and pop fail, ret(%d)", __FUNCTION__, ret);
            /* TODO: doing exception handling */
        }
        return ret;
    }

    if (newFrame == NULL) {
        CLOGE("ERR(%s):newFrame is NULL", __FUNCTION__);
        return INVALID_OPERATION;
    }

    if(pos == BUFFER_POS::DST)
        ret = newFrame->getDstBuffer(getPipeId(), &newBuffer);
    else if(pos == BUFFER_POS::SRC)
        ret = newFrame->getSrcBuffer(getPipeId(), &newBuffer);

    if (ret < 0) {
        CLOGE("ERR(%s[%d]):frame get buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return OK;
    }

    if (m_runningFrameList[OUTPUT_NODE][newBuffer.index] != NULL) {
        CLOGE("ERR(%s):new buffer is invalid, we already get buffer index(%d), newFrame->frameCount(%d)",
            __FUNCTION__, newBuffer.index, newFrame->getFrameCount());
        return BAD_VALUE;
    }

    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(newBuffer.addr[newBuffer.planeCount - 1]);

    if (shot_ext != NULL) {
        newFrame->getMetaData(shot_ext);
        m_parameters->duplicateCtrlMetadata((void *)shot_ext);
        m_activityControl->activityBeforeExecFunc(getPipeId(), (void *)&newBuffer);

        /* set metadata for instant on */
        shot_ext->shot.ctl.scaler.cropRegion[0] = 0;
        shot_ext->shot.ctl.scaler.cropRegion[1] = 0;
#if defined(FASTEN_AE_WIDTH) && defined(FASTEN_AE_HEIGHT)
        shot_ext->shot.ctl.scaler.cropRegion[2] = FASTEN_AE_WIDTH;
        shot_ext->shot.ctl.scaler.cropRegion[3] = FASTEN_AE_HEIGHT;
#else
        int bcropW = 0;
        int bcropH = 0;

        shot_ext->shot.ctl.scaler.cropRegion[2] = bcropW;
        shot_ext->shot.ctl.scaler.cropRegion[3] = bcropH;
#endif
        uint32_t frameRate = 0;
        if (m_parameters->getCameraId() == CAMERA_ID_FRONT) {
            frameRate  = FASTEN_AE_FPS_FRONT;
        } else {
            frameRate  = FASTEN_AE_FPS;
        }
        setMetaCtlAeTargetFpsRange(shot_ext, frameRate, frameRate);
        setMetaCtlSensorFrameDuration(shot_ext, (uint64_t)((1000 * 1000 * 1000) / (uint64_t)frameRate));

        /* set afMode into INFINITY */
        shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_CANCEL;
        shot_ext->shot.ctl.aa.vendor_afmode_option &= (0 << AA_AFMODE_OPTION_BIT_MACRO);

        if (m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameLeaderInfo.perFrameNodeType == PERFRAME_NODE_TYPE_LEADER) {
            camera2_node_group node_group_info;
            memset(&shot_ext->node_group, 0x0, sizeof(camera2_node_group));
            newFrame->getNodeGroupInfo(&node_group_info, m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameLeaderInfo.perframeInfoIndex);

            /* Per - Leader */
            if (node_group_info.leader.request == 1) {

                if (m_checkNodeGroupInfo(m_mainNode->getName(), &m_curNodeGroupInfo.leader, &node_group_info.leader) != NO_ERROR)
                    CLOGW("WARN(%s[%d]): m_checkNodeGroupInfo(%s) fail", __FUNCTION__, __LINE__, m_mainNode->getName());

                setMetaNodeLeaderInputSize(shot_ext,
                    node_group_info.leader.input.cropRegion[0],
                    node_group_info.leader.input.cropRegion[1],
                    node_group_info.leader.input.cropRegion[2],
                    node_group_info.leader.input.cropRegion[3]);
                setMetaNodeLeaderOutputSize(shot_ext,
                    node_group_info.leader.output.cropRegion[0],
                    node_group_info.leader.output.cropRegion[1],
                    node_group_info.leader.output.cropRegion[2],
                    node_group_info.leader.output.cropRegion[3]);
                setMetaNodeLeaderRequest(shot_ext,
                    node_group_info.leader.request);
                setMetaNodeLeaderVideoID(shot_ext,
                    m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameLeaderInfo.perFrameVideoID);
            }

            /* Per - Captures */
            if (CAPTURE_NODE_MAX < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum) {
                android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):PipeId(%d) has Invalid perframeSupportNodeNum:CAPTURE_NODE_MAX(%d) < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum(%d), assert!!!!",
                    __FUNCTION__, __LINE__, getPipeId(), CAPTURE_NODE_MAX, m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum);
            }

            for (int i = 0; i < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum; i ++) {
                if (node_group_info.capture[i].request == 1) {

                    if (m_checkNodeGroupInfo(i, &m_curNodeGroupInfo.capture[i], &node_group_info.capture[i]) != NO_ERROR)
                        CLOGW("WARN(%s[%d]): m_checkNodeGroupInfo(%d) fail", __FUNCTION__, __LINE__, i);

                    setMetaNodeCaptureInputSize(shot_ext, i,
                        node_group_info.capture[i].input.cropRegion[0],
                        node_group_info.capture[i].input.cropRegion[1],
                        node_group_info.capture[i].input.cropRegion[2],
                        node_group_info.capture[i].input.cropRegion[3]);
                    setMetaNodeCaptureOutputSize(shot_ext, i,
                        node_group_info.capture[i].output.cropRegion[0],
                        node_group_info.capture[i].output.cropRegion[1],
                        node_group_info.capture[i].output.cropRegion[2],
                        node_group_info.capture[i].output.cropRegion[3]);
                    setMetaNodeCaptureRequest(shot_ext,  i, node_group_info.capture[i].request);
                    setMetaNodeCaptureVideoID(shot_ext, i, m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameCaptureInfo[i].perFrameVideoID);
                }
            }
        }
    }
    ret = m_mainNode->putBuffer(&newBuffer);
    if (ret < 0) {
        CLOGE("ERR(%s):putBuffer fail", __FUNCTION__);
        return ret;
        /* TODO: doing exception handling */
    }

    ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_PROCESSING);
    if (ret < 0) {
        CLOGE("ERR(%s): setDstBuffer state fail", __FUNCTION__);
        return ret;
    }

    m_runningFrameList[OUTPUT_NODE][newBuffer.index] = newFrame;

    m_numOfRunningFrame[OUTPUT_NODE]++;

    *frame = newFrame;

    return NO_ERROR;
}

/* Don't use this function, this is regacy code */
status_t ExynosCameraMCPipe::instantOnDQbuf(ExynosCameraFrame **frame, __unused BUFFER_POS::POS pos)
{
    if (m_mainNode == NULL) {
        CLOGE("ERR(%s): m_mainNode == NULL. so, fail", __FUNCTION__);
        return INVALID_OPERATION;
    }

    ExynosCameraFrame *curFrame = NULL;
    ExynosCameraBuffer curBuffer;
    int index = -1;
    int ret = 0;

    if (m_numOfRunningFrame[OUTPUT_NODE] <= 0 ) {
        CLOGD("DEBUG(%s[%d]): skip getBuffer, numOfRunningFrame = %d", __FUNCTION__, __LINE__, m_numOfRunningFrame[OUTPUT_NODE]);
        return NO_ERROR;
    }

    ret = m_mainNode->getBuffer(&curBuffer, &index);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):getBuffer fail", __FUNCTION__, __LINE__);
        /* TODO: doing exception handling */
        return ret;
    }

    if (index < 0) {
        CLOGE("ERR(%s[%d]):Invalid index(%d) fail", __FUNCTION__, __LINE__, index);
        return INVALID_OPERATION;
    }

    m_activityControl->activityAfterExecFunc(getPipeId(), (void *)&curBuffer);

    ret = m_updateMetadataToFrame(curBuffer.addr[curBuffer.planeCount - 1], curBuffer.index);
    if (ret < 0)
        CLOGE("ERR(%s[%d]): updateMetadataToFrame fail, ret(%d)", __FUNCTION__, __LINE__, ret);

    if (curBuffer.index < 0) {
        CLOGE("ERR(%s):index(%d) is invalid", __FUNCTION__, curBuffer.index);
        return BAD_VALUE;
    }

    curFrame = m_runningFrameList[OUTPUT_NODE][curBuffer.index];

    if (curFrame == NULL) {
        CLOGE("ERR(%s):Unknown buffer, frame is NULL", __FUNCTION__);
        dump();
        return BAD_VALUE;
    }

    *frame = curFrame;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::instantOff(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    for (int i = OUTPUT_NODE; i < OTF_NODE_BASE; i++) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->stop();
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):node(%s) stop fail, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                return ret;
            }

            ret = m_node[i]->clrBuffers();
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):node(%s) clrBuffers fail, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                return ret;
            }
        }
    }

    CLOGI("INFO(%s[%d]):instantOff() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId());

    return ret;
}

/* Don't use this function, this is regacy code */
status_t ExynosCameraMCPipe::instantOnPushFrameQ(BUFFERQ_TYPE::TYPE type, ExynosCameraFrame **frame)
{
    if( type == BUFFERQ_TYPE::OUTPUT )
        m_outputFrameQ->pushProcessQ(frame);
    else
        m_inputFrameQ->pushProcessQ(frame);

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::getPipeInfo(int *fullW, int *fullH, int *colorFormat, int pipePosition)
{
    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;
    int planeCount = 0;

    if (pipePosition == DST_PIPE) {
        if (m_node[OUTPUT_NODE] == NULL) {
            CLOGE("ERR(%s[%d]):m_node[OUTPUT_NODE] is NULL", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }

        ret = m_node[OUTPUT_NODE]->getSize(fullW, fullH);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):node(%s) getSize fail, ret(%d)",
                    __FUNCTION__, __LINE__, m_deviceInfo->nodeName[OUTPUT_NODE], ret);
            return ret;
        }

        ret = m_node[OUTPUT_NODE]->getColorFormat(colorFormat, &planeCount);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):node(%s) getColorFormat fail, ret(%d)",
                    __FUNCTION__, __LINE__, m_deviceInfo->nodeName[OUTPUT_NODE], ret);
            return ret;
        }
    } else if (pipePosition == SRC_PIPE) {
        for (int i = (OTF_NODE_BASE - 1); i > OUTPUT_NODE; i--) {
            if (m_node[i] != NULL) {
                ret = m_node[i]->getSize(fullW, fullH);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):node(%s) getSize fail, ret(%d)",
                            __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                    return ret;
                }

                ret = m_node[i]->getColorFormat(colorFormat, &planeCount);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):node(%s) getColorFormat fail, ret(%d)",
                            __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                    return ret;
                }

                CLOGV("INFO(%s[%d]):getPipeInfo() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId((enum NODE_TYPE)i));
                return ret;
            }
        }
        CLOGE("ERR(%s[%d]):all capture m_node is NULL", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    } else {
        CLOGE("ERR(%s[%d]):Pipe position is Invalid, position(%d)", __FUNCTION__, __LINE__, pipePosition);
        return BAD_VALUE;
    }

    CLOGV("INFO(%s[%d]):getPipeInfo() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId());
    return ret;
}

int ExynosCameraMCPipe::getCameraId(void)
{
    return this->m_cameraId;
}

status_t ExynosCameraMCPipe::setPipeId(uint32_t id)
{
    return this->setPipeId(OUTPUT_NODE, id);
}

uint32_t ExynosCameraMCPipe::getPipeId(void)
{
    return (uint32_t)this->getPipeId(OUTPUT_NODE);
}

status_t ExynosCameraMCPipe::setPipeId(enum NODE_TYPE nodeType, uint32_t id)
{
    if (nodeType < OUTPUT_NODE || MAX_NODE <= nodeType) {
        CLOGE("ERR(%s[%d]):Invalid nodeType(%d). so, fail", __FUNCTION__, __LINE__, nodeType);
        return BAD_VALUE;
    }

    CLOGD("DEBUG(%s[%d]):nodeType(%d), id(%d)", __FUNCTION__, __LINE__, nodeType, id);

    m_pipeIdArr[nodeType] = id;

    if (nodeType == OUTPUT_NODE)
        m_pipeId = id;

    return NO_ERROR;
}

int ExynosCameraMCPipe::getPipeId(enum NODE_TYPE nodeType)
{
    if (nodeType < OUTPUT_NODE || MAX_NODE <= nodeType) {
        CLOGE("ERR(%s[%d]):Invalid nodeType(%d). so, fail", __FUNCTION__, __LINE__, nodeType);
        return -1;
    }

    return m_pipeIdArr[nodeType];
}


status_t ExynosCameraMCPipe::setPipeName(const char *pipeName)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    strncpy(m_name,  pipeName, (EXYNOS_CAMERA_NAME_STR_SIZE - 1));

    return NO_ERROR;
}

char *ExynosCameraMCPipe::getPipeName(void)
{
    return m_name;
}

status_t ExynosCameraMCPipe::setBufferManager(ExynosCameraBufferManager **bufferManager)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++)
        m_bufferManager[i] = bufferManager[i];

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::clearInputFrameQ(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (m_inputFrameQ != NULL)
        m_inputFrameQ->release();

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::getInputFrameQ(frame_queue_t **inputFrameQ)
{
    *inputFrameQ = m_inputFrameQ;

    if (*inputFrameQ == NULL)
        CLOGE("ERR(%s[%d])inputFrameQ is NULL", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::setOutputFrameQ(frame_queue_t *outputFrameQ)
{
    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    m_outputFrameQ = outputFrameQ;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::getOutputFrameQ(frame_queue_t **outputFrameQ)
{
    *outputFrameQ = m_outputFrameQ;

    if (*outputFrameQ == NULL)
        CLOGE("ERR(%s[%d]):outputFrameQ is NULL", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::setFrameDoneQ(frame_queue_t *frameDoneQ)
{
    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    m_frameDoneQ = frameDoneQ;
    m_flagFrameDoneQ = true;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::getFrameDoneQ(frame_queue_t **frameDoneQ)
{
    *frameDoneQ = m_frameDoneQ;

    if (*frameDoneQ == NULL)
        CLOGE("ERR(%s[%d]):frameDoneQ is NULL", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::setNodeInfos(camera_node_objects_t *nodeObjects, bool flagReset)
{
    CLOGD("DEBUG(%s[%d]):setNodeInfos flagReset(%s)",
            __FUNCTION__, __LINE__, (flagReset) ? "True" : "False");

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        m_node[i] = nodeObjects->node[i];
        m_secondaryNode[i] = nodeObjects->secondaryNode[i];

        if (flagReset == true) {
            if (m_node[i] != NULL)
                m_node[i]->resetInput();

            if (m_secondaryNode[i] != NULL)
                m_secondaryNode[i]->resetInput();
        }
    }

    if (flagReset == true) {
        m_frameDoneQ = NULL;
        m_flagFrameDoneQ = false;
        m_outputFrameQ = NULL;
    }

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::getNodeInfos(camera_node_objects_t *nodeObjects)
{
    CLOGD("DEBUG(%s[%d]):getNodeInfos", __FUNCTION__, __LINE__);

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        nodeObjects->node[i] = m_node[i];
        nodeObjects->secondaryNode[i] = m_secondaryNode[i];
    }

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::setMapBuffer(__unused ExynosCameraBuffer *srcBuf, __unused ExynosCameraBuffer *dstBuf)
{
    status_t ret = NO_ERROR;

    if (m_node[OUTPUT_NODE] != NULL
        && m_bufferManager[OUTPUT_NODE] != NULL)
        ret |= m_setMapBuffer(OUTPUT_NODE);

    for (int i = CAPTURE_NODE; i < OTF_NODE_BASE; i++) {
        if (m_deviceInfo->connectionMode[i] == HW_CONNECTION_MODE_M2M_BUFFER_HIDING
            && m_node[i] != NULL
            && m_bufferManager[i] != NULL)
            ret |= m_setMapBuffer(i);
    }

    return ret;
}

status_t ExynosCameraMCPipe::setBoosting(bool isBoosting)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    m_isBoosting = isBoosting;

    return NO_ERROR;
}

bool ExynosCameraMCPipe::isThreadRunning(void)
{
    if (m_putBufferThread->isRunning() || m_getBufferThread->isRunning())
        return true;

    return false;
}

status_t ExynosCameraMCPipe::getThreadState(int **threadState)
{
    *threadState = &m_threadState;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::getThreadInterval(uint64_t **timeInterval)
{
    *timeInterval = &m_timeInterval;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::getThreadRenew(int **timeRenew)
{
    *timeRenew = &m_threadRenew;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::incThreadRenew(void)
{
    m_threadRenew ++;

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::setStopFlag(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    m_flagTryStop = true;

    return NO_ERROR;
}

int ExynosCameraMCPipe::getRunningFrameCount(void)
{
    int runningFrameCount = 0;

    for (uint32_t i = 0; i < m_numBuffers[OUTPUT_NODE]; i++) {
        if (m_runningFrameList[OUTPUT_NODE][i] != NULL) {
            runningFrameCount++;
        }
    }

    return runningFrameCount;
}

#ifdef USE_MCPIPE_SERIALIZATION_MODE
void ExynosCameraMCPipe::needSerialization(bool enable)
{
    CLOGI("INFO(%s[%d]):%s serialized operation %s",
            __FUNCTION__, __LINE__, m_name,
            (enable == true)? "enabled" : "disabled");

    m_serializeOperation = enable;
}
#endif

void ExynosCameraMCPipe::dump(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    m_dumpRunningFrameList();

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            m_node[i]->dump();
        }
    }

    return;
}

status_t ExynosCameraMCPipe::dumpFimcIsInfo(bool bugOn)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    ret = m_node[OUTPUT_NODE]->setControl(V4L2_CID_IS_DEBUG_DUMP, bugOn);
    if (ret != NO_ERROR)
        CLOGE("ERR(%s[%d]):m_node[OUTPUT_NODE]->setControl failed", __FUNCTION__, __LINE__);

    return ret;
}

//#ifdef MONITOR_LOG_SYNC
status_t ExynosCameraMCPipe::syncLog(uint32_t syncId)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    ret = m_node[OUTPUT_NODE]->setControl(V4L2_CID_IS_DEBUG_SYNC_LOG, syncId);
    if (ret != NO_ERROR)
        CLOGE("ERR(%s[%d]):m_node[OUTPUT_NODE]->setControl failed", __FUNCTION__, __LINE__);

    return ret;
}
//#endif

status_t ExynosCameraMCPipe::m_preCreate(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    ExynosCameraNode *jpegNode = NULL;

    /* Create & open output/capture nodes */
    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_flagValidInt(m_deviceInfo->nodeNum[i]) == true) {
            if ((m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_HWFC_JPEG_NUM
                || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_HWFC_THUMB_NUM)
                && jpegNode != NULL) {
                enum EXYNOS_CAMERA_NODE_JPEG_HAL_LOCATION location = NODE_LOCATION_DST;

                if (m_deviceInfo->pipeId[i] == PIPE_HWFC_JPEG_SRC_REPROCESSING
                    || m_deviceInfo->pipeId[i] == PIPE_HWFC_THUMB_SRC_REPROCESSING)
                    location = NODE_LOCATION_SRC;

                /* JpegHAL Destinaion node case */
                m_node[i] = (ExynosCameraNode*)new ExynosCameraNodeJpegHAL();

                ExynosJpegEncoderForCamera *jpegEncoder = NULL;
                ret = jpegNode->getJpegEncoder(&jpegEncoder);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s):jpegNode->gejpegEncoder failed", __FUNCTION__);
                    return ret;
                }

                ret = m_node[i]->create(m_deviceInfo->nodeName[i], m_cameraId, location, jpegEncoder);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Create node fail(Node:%s), ret(%d)",
                            __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                    return ret;
                }

                ret = m_node[i]->open(m_deviceInfo->nodeNum[i], m_parameters->isUseThumbnailHWFC());
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Open node fail(Node:%s), ret(%d)",
                            __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                    return ret;
                }

                CLOGD("DEBUG(%s[%d]):JpegHAL Node(%d) opened",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeNum[i]);
            } else if (i > OUTPUT_NODE
                       && m_node[OUTPUT_NODE] != NULL
                       && m_deviceInfo->nodeNum[i] == m_deviceInfo->nodeNum[OUTPUT_NODE]) {

                /* W/A for under Helsinki Prime (same node number) */
                m_node[i] = new ExynosCameraNode();

                int fd = -1;
                ret = m_node[OUTPUT_NODE]->getFd(&fd);
                if (ret != NO_ERROR || m_flagValidInt(fd) == false) {
                    CLOGE("ERR(%s):OUTPUT_NODE->getFd failed", __FUNCTION__);
                    return ret;
                }

                ret = m_node[i]->create(m_deviceInfo->nodeName[i], m_cameraId, fd);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Create node fail(Node:%s), ret(%d)",
                            __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                    return ret;
                }

                CLOGD("DEBUG(%s[%d]):Node(%d) opened, fd(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeNum[i], fd);
            } else {
                if (m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_HWFC_JPEG_NUM
                    || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_HWFC_THUMB_NUM) {
                    /* JpegHAL Node case */
                    m_node[i] = new ExynosCameraNodeJpegHAL();
                    jpegNode = m_node[i];
                } else {
                    /* Normal case */
                    m_node[i] = new ExynosCameraNode();
                }

                ret = m_node[i]->create(m_deviceInfo->nodeName[i], m_cameraId);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Create node fail(Node:%s), ret(%d)",
                            __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                    return ret;
                }

                ret = m_node[i]->open(m_deviceInfo->nodeNum[i]);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Open node fail(Node:%s), ret(%d)",
                            __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                    return ret;
                }

                CLOGV("DEBUG(%s[%d]):Node(%d) opened",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeNum[i]);
            }
        }
    }

    /* Create & open OTF nodes */
    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_flagValidInt(m_deviceInfo->secondaryNodeNum[i]) == true) {
            m_secondaryNode[i] = new ExynosCameraNode();

            ret = m_secondaryNode[i]->create(m_deviceInfo->secondaryNodeName[i], m_cameraId);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):Create node fail(Node:%s), ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->secondaryNodeName[i], ret);
                return ret;
            }

            ret = m_secondaryNode[i]->open(m_deviceInfo->secondaryNodeNum[i]);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):Open node fail(Node:%s), ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->secondaryNodeName[i], ret);
                return ret;
            }

            CLOGD("DEBUG(%s[%d]):Node(%s) opened", __FUNCTION__, __LINE__, m_deviceInfo->secondaryNodeName[i]);
        }
    }

    m_putBufferThread = new MCPipeThread(this,
            &ExynosCameraMCPipe::m_putBufferThreadFunc, "putBufferThread", PRIORITY_URGENT_DISPLAY);
    m_getBufferThread = new MCPipeThread(this,
            &ExynosCameraMCPipe::m_getBufferThreadFunc, "getBufferThread", PRIORITY_URGENT_DISPLAY);

    if (m_reprocessing == true) {
        m_inputFrameQ = new frame_queue_t(m_putBufferThread);
        m_requestFrameQ = new frame_queue_t(m_getBufferThread);
    } else {
        m_inputFrameQ = new frame_queue_t;
        m_requestFrameQ = new frame_queue_t;
    }

    /* Set wait time 0.55 sec. Because, it support 2fps */
    m_inputFrameQ->setWaitTime(550000000);      /* .55 sec */
    m_requestFrameQ->setWaitTime(550000000);    /* .55 sec */

    CLOGI("INFO(%s[%d]):m_preCreate() is succeed, Pipe(%d), prepare(%d)",
            __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    return ret;
}

status_t ExynosCameraMCPipe::m_postCreate(int32_t *sensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (sensorIds != NULL) {
            CLOGD("DEBUG(%s[%d]):Set new sensorIds[%d] : %d", __FUNCTION__, __LINE__, i, sensorIds[i]);
            m_sensorIds[i] = sensorIds[i];
        } else {
            m_sensorIds[i] = -1;
        }
    }

    ret = m_setInput(m_node, m_deviceInfo->nodeNum, m_sensorIds);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_setInput(Main) fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    ret = m_setInput(m_secondaryNode, m_deviceInfo->secondaryNodeNum, m_secondarySensorIds);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_setInput(secondary) fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    CLOGI("INFO(%s[%d]):m_postCreate() is succeed, Pipe(%d)", __FUNCTION__, __LINE__, getPipeId());

    return ret;
}

bool ExynosCameraMCPipe::m_putBufferThreadFunc(void)
{
    status_t ret = NO_ERROR;

#ifdef TEST_WATCHDOG_THREAD
    testErrorDetect++;
    if (testErrorDetect == 100)
        m_threadState = ERROR_POLLING_DETECTED;
#endif

    if (m_flagTryStop == true) {
        usleep(5000);
        return true;
    }

    ret = m_putBuffer();
    if (ret != NO_ERROR)
        CLOGW("WARN(%s[%d]):m_putbuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);

    return m_checkThreadLoop(m_inputFrameQ);
}

bool ExynosCameraMCPipe::m_getBufferThreadFunc(void)
{
    status_t ret = NO_ERROR;

    if (m_flagTryStop == true) {
        usleep(5000);
        return true;
    }

    ret = m_getBuffer();
    if (ret != NO_ERROR)
        CLOGW("WARN(%s[%d]):m_getBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);

    m_timer.stop();
    m_timeInterval = m_timer.durationMsecs();
    m_timer.start();

    /* update renew count */
    if (ret >= 0)
        m_threadRenew = 0;

    return m_checkThreadLoop(m_requestFrameQ);
}

status_t ExynosCameraMCPipe::m_putBuffer(void)
{
    CLOGV("DEBUG(%s[%d]):-IN-", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer buffer[OTF_NODE_BASE];
    ExynosCameraFrameEntity *entity = NULL;
    int pipeId = 0;
    int bufferIndex[OTF_NODE_BASE];
    for (int i = OUTPUT_NODE; i < OTF_NODE_BASE; i++)
        bufferIndex[i] = -2;
    uint32_t captureNodeCount = 0;

    /* 1. Pop from input frame queue */
    ret = m_inputFrameQ->waitAndPopProcessQ(&newFrame);
    if (ret == TIMED_OUT) {
        CLOGW("WARN(%s[%d]):inputFrameQ wait timeout", __FUNCTION__, __LINE__);
        return ret;
    } else if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):inputFrameQ wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return ret;
    }

    if (newFrame == NULL) {
        CLOGE("ERR(%s[%d]):New frame is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (newFrame->getFrameState() == FRAME_STATE_SKIPPED
        || newFrame->getFrameState() == FRAME_STATE_INVALID) {
        if (newFrame->getFrameType() == FRAME_TYPE_INTERNAL) {
            CLOGI("INFO(%s[%d]):Internal Frame(%d), frameCount(%d) (%d)",
                    __FUNCTION__, __LINE__, newFrame->getFrameType(), newFrame->getFrameCount());
        } else {
            CLOGE("ERR(%s[%d]):New frame is INVALID, frameCount(%d)",
                    __FUNCTION__, __LINE__, newFrame->getFrameCount());
        }
        goto CLEAN_FRAME;
    }

#ifdef USE_MCPIPE_SERIALIZATION_MODE
    if (m_serializeOperation == true) {
        CLOGD("DEBUG(%s[%d]):%s Critical Section WAIT",
                __FUNCTION__, __LINE__, m_name);
        ExynosCameraMCPipe::g_serializationLock.lock();
        CLOGD("DEBUG(%s[%d]):%s Critical Section START",
                __FUNCTION__, __LINE__, m_name);
    }
#endif

    for (int i = (OTF_NODE_BASE - 1); i > OUTPUT_NODE; i--) {
        if (m_node[i] == NULL)
            continue;

        pipeId = getPipeId((enum NODE_TYPE)i);
        if (pipeId < 0) {
            CLOGE("ERR(%s[%d]):getPipeId(%d) fail", __FUNCTION__, __LINE__, i);
            return BAD_VALUE;
        }

    /* 2. Get capture node buffer(DstBuffer) from buffer manager */
        if (m_node[i] != NULL
            && newFrame->getRequest(pipeId) == true
            && m_skipPutBuffer[i] == false) {
            if (m_bufferManager[i] == NULL) {
                CLOGE("ERR(%s[%d]):Buffer manager is NULL, i(%d), piepId(%d), frameCount(%d)",
                        __FUNCTION__, __LINE__, i, pipeId, newFrame->getFrameCount());
                continue;
            }

            ret = newFrame->getDstBuffer(getPipeId(), &buffer[i], i);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):getDstBuffer fail. pipeId(%d), frameCount(%d), ret(%d)",
                        __FUNCTION__, __LINE__, pipeId, newFrame->getFrameCount(), ret);
                continue;
            }

            if (buffer[i].index < 0) {
                ret = m_bufferManager[i]->getBuffer(&(bufferIndex[i]), EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &(buffer[i]));
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Buffer manager getBuffer fail, manager(%d), frameCount(%d), ret(%d)",
                            __FUNCTION__, __LINE__, i, newFrame->getFrameCount(), ret);

                    newFrame->dump();

                    newFrame->setRequest(pipeId, false);
                    /* m_bufferManager[i]->dump(); */
                    continue;
                }
            } else {
                CLOGD("DEBUG(%s[%d]):Skip to get buffer from bufferMgr.\
                        pipeId(%d), frameCount(%d) bufferIndex %d)",
                        __FUNCTION__, __LINE__,
                        pipeId, newFrame->getFrameCount(), buffer[i].index);
                bufferIndex[i] = buffer[i].index;
            }

            if (bufferIndex[i] < 0
                || m_runningFrameList[i][(bufferIndex[i])] != NULL) {
                CLOGE("ERR(%s[%d]):New buffer is invalid, we already get buffer, index(%d), frameCount(%d)",
                        __FUNCTION__, __LINE__, bufferIndex[i], newFrame->getFrameCount());
                newFrame->setRequest(pipeId, false);
                /* dump(); */
                continue;
            }

    /* 3. Put capture buffer(DstBuffer) to node */
            if (bufferIndex[i] >= 0
                && newFrame->getRequest(pipeId) == true) {
                /* Set JPEG node's perframe information only for HWFC*/
                if (m_reprocessing == true
                    && (m_deviceInfo->pipeId[i] == PIPE_HWFC_JPEG_SRC_REPROCESSING
                        || m_deviceInfo->pipeId[i] == PIPE_HWFC_JPEG_DST_REPROCESSING
                        || m_deviceInfo->pipeId[i] == PIPE_HWFC_THUMB_SRC_REPROCESSING)) {

                    camera2_shot_ext *shot_ext = NULL;
                    shot_ext = (struct camera2_shot_ext *)(buffer[i].addr[buffer[i].planeCount-1]);
                    newFrame->getMetaData(shot_ext);

                    ret = m_setJpegInfo(i, &(buffer[i]));
                    if (ret != NO_ERROR) {
                        CLOGE("ERR(%s[%d]):Failed to setJpegInfo, pipeId %s buffer.index %d",
                                __FUNCTION__, __LINE__,
                                (m_deviceInfo->pipeId[i] == PIPE_HWFC_JPEG_SRC_REPROCESSING)?
                                "PIPE_HWFC_JPEG_SRC_REPROCESSING":"PIPE_HWFC_THUMB_SRC_REPROCESSING",
                                buffer[i].index);
                        continue;
                    }
                }

                ret = m_node[i]->putBuffer(&(buffer[i]));
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):node(%s)->putBuffer() fail, frameCount(%d), ret(%d)",
                            __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], newFrame->getFrameCount(), ret);

                    /* TODO: doing exception handling */
                    ret = m_bufferManager[i]->putBuffer(bufferIndex[i], EXYNOS_CAMERA_BUFFER_POSITION_NONE);
                    if (ret != NO_ERROR) {
                        CLOGE("ERR(%s[%d]):Buffer manager putBuffer fail, manager(%d), frameCount(%d), ret(%d)",
                                __FUNCTION__, __LINE__, i, newFrame->getFrameCount(), ret);
                    }

                    newFrame->setRequest(pipeId, false);
                } else {
                    m_skipPutBuffer[i] = true;
                    m_skipBuffer[i] = buffer[i];
                }
            }
        } else if (m_skipPutBuffer[i] == true) {
            CLOGD("DEBUG(%s[%d]):%s:Skip putBuffer. framecount %d bufferIndex %d",
                    __FUNCTION__, __LINE__,
                    m_deviceInfo->nodeName[i], newFrame->getFrameCount(), m_skipBuffer[i].index);
        }

        if (m_skipPutBuffer[i] == true)
            captureNodeCount++;
    }

    if (captureNodeCount == 0) {
        CLOGW("WRN(%s[%d]):Capture node putbuffer is Zero, frameCount(%d)",
                __FUNCTION__, __LINE__, newFrame->getFrameCount());
        /* Comment out: 3AA and ISP must running, because it save stat and refered next frame.
         *              So, put SRC buffer to output node when DST buffers all empty(zero).
         */
        /* goto CLEAN_FRAME; */
    }

    /* 4. Get output node(SrcBuffer) buffer from frame */
    if (m_node[OUTPUT_NODE] != NULL) {
        ret = newFrame->getSrcBuffer(getPipeId(OUTPUT_NODE), &(buffer[OUTPUT_NODE]));
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Frame get src buffer fail, frameCount(%d), ret(%d)",
                    __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
            /* TODO: doing exception handling */
            goto CLEAN_FRAME;
        }

        if (m_runningFrameList[OUTPUT_NODE][(buffer[OUTPUT_NODE].index)] != NULL) {
            if ( (m_parameters->isReprocessing() == true)
                 && (m_parameters->getUsePureBayerReprocessing() == false)  /* if Dirty bayer reprocessing */
                 && (newFrame->getFrameType() == FRAME_TYPE_INTERNAL)
                 && (getPipeId(OUTPUT_NODE) == PIPE_ISP) ) {                /* if internal frame at ISP pipe */
                    /* In dirty bayer mode, Internal frame would not have valid
                       output buffer on ISP pipe. So suppress error message */
                    CLOGI("INFO(%s[%d]):Internal frame will raises an error here, but it's normal operation, index(%d), frameCount(%d)",
                            __FUNCTION__, __LINE__, buffer[OUTPUT_NODE].index, newFrame->getFrameCount());
            } else {
                CLOGE("ERR(%s[%d]):New buffer is invalid, we already get buffer, index(%d), frameCount(%d)",
                        __FUNCTION__, __LINE__, buffer[OUTPUT_NODE].index, newFrame->getFrameCount());
            }
            /* dump(); */
            goto CLEAN_FRAME;
        }

    /* 5. Update control metadata for request, Zoom, ... */
        if (useSizeControlApi() == true)
            ret = m_updateMetadataFromFrame_v2(newFrame, &(buffer[OUTPUT_NODE]));
        else
            ret = m_updateMetadataFromFrame(newFrame, &(buffer[OUTPUT_NODE]));
        if (ret != NO_ERROR) {
            CLOGW("WARN(%s[%d]):Update metadata fail, frameCount(%d), ret(%d)",
                    __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
        }

    /* 6. Put output buffer(SrcBuffer) to node */
        ret = m_node[OUTPUT_NODE]->putBuffer(&(buffer[OUTPUT_NODE]));
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):node(%s)->putBuffer() fail, frameCount(%d), ret(%d)",
                    __FUNCTION__, __LINE__, m_deviceInfo->nodeName[OUTPUT_NODE], newFrame->getFrameCount(), ret);
            /* TODO: doing exception handling */
            goto CLEAN_FRAME;
        }

        ret = newFrame->setSrcBufferState(getPipeId(OUTPUT_NODE), ENTITY_BUFFER_STATE_PROCESSING);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setSrcBuffer state fail, frameCount(%d), ret(%d)",
                    __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
        }

        m_runningFrameList[OUTPUT_NODE][(buffer[OUTPUT_NODE].index)] = newFrame;
        m_numOfRunningFrame[OUTPUT_NODE]++;

    /* 7. Link capture node buffer(DstBuffer) to frame */
        for (int i = (OTF_NODE_BASE - 1); i > OUTPUT_NODE; i--) {
            if (m_node[i] == NULL)
                continue;

            pipeId = getPipeId((enum NODE_TYPE)i);
            if (pipeId < 0) {
                CLOGE("ERR(%s[%d]):getPipeId(%d) fail", __FUNCTION__, __LINE__, i);
                return BAD_VALUE;
            }


            if (m_node[i] != NULL
                && newFrame->getRequest(pipeId) == true) {
                /* HACK: Should change ExynosCamera, Frame */
                ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_REQUESTED, i);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                            __FUNCTION__, __LINE__, pipeId, newFrame->getFrameCount(), ret);
                }

                if (m_skipPutBuffer[i] == true) {
                    buffer[i] = m_skipBuffer[i];
                    bufferIndex[i] = buffer[i].index;
                }

                ret = newFrame->setDstBuffer(getPipeId(), buffer[i], i, INDEX(pipeId));
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Frame set dst buffer fail, frameCount(%d), ret(%d)",
                            __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
                    /* TODO: doing exception handling */
                    if (m_bufferManager[i] != NULL)
                        ret = m_bufferManager[i]->putBuffer(buffer[i].index, EXYNOS_CAMERA_BUFFER_POSITION_NONE);

                    if (ret != NO_ERROR) {
                        CLOGE("ERR(%s[%d]):Buffer manager putBuffer fail, manager(%d), frameCount(%d), ret(%d)",
                                __FUNCTION__, __LINE__, i, newFrame->getFrameCount(), ret);
                    }

                    newFrame->setRequest(pipeId, false);
                }

                ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_PROCESSING, i);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                            __FUNCTION__, __LINE__, getPipeId(), newFrame->getFrameCount(), ret);
                }

                m_runningFrameList[i][(bufferIndex[i])] = newFrame;
                m_numOfRunningFrame[i]++;
                m_skipPutBuffer[i] = false;
                m_skipBuffer[i].index = -2;
            }
        }
    }

    /* 8. Push frame to getBufferThread */
    m_requestFrameQ->pushProcessQ(&newFrame);

    CLOGV("DEBUG(%s[%d]):OUT-", __FUNCTION__, __LINE__);
    return NO_ERROR;

    /* Error handling for SrcBuffer and Frame */
CLEAN_FRAME:
    CLOGD("DEBUG(%s[%d]):clean frame, frameCount(%d)",
            __FUNCTION__, __LINE__, newFrame->getFrameCount());

#ifdef USE_MCPIPE_SERIALIZATION_MODE
    if (m_serializeOperation == true) {
        ExynosCameraMCPipe::g_serializationLock.unlock();
        CLOGD("DEBUG(%s[%d]):%s Critical Section END",
                __FUNCTION__, __LINE__, m_name);
    }
#endif

    ret = newFrame->setSrcBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):setSrcBuffer state fail, frameCount(%d), ret(%d)",
                __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
    }

    ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                __FUNCTION__, __LINE__, getPipeId(), newFrame->getFrameCount(), ret);
    }

    for (int i = (OTF_NODE_BASE - 1); i > OUTPUT_NODE; i--) {
        if (m_node[i] != NULL
            && newFrame->getRequest(getPipeId((enum NODE_TYPE)i)) == true)
            newFrame->setRequest(getPipeId((enum NODE_TYPE)i), false);
    }

    if (newFrame->getFrameState() != FRAME_STATE_SKIPPED
        && newFrame->getFrameState() != FRAME_STATE_INVALID) {
        newFrame->setFrameState(FRAME_STATE_SKIPPED);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setFrameState fail, frameCount(%d), ret(%d)",
                    __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
        }
    }

    ret = m_completeFrame(newFrame, false);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):Complete frame fail, frameCount(%d), ret(%d)", __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
        /* TODO: doing exception handling */
    }

    if (m_frameDoneQ != NULL && m_flagFrameDoneQ == true)
        m_frameDoneQ->pushProcessQ(&newFrame);

    m_outputFrameQ->pushProcessQ(&newFrame);

    return INVALID_OPERATION;
}

status_t ExynosCameraMCPipe::m_getBuffer(void)
{
    CLOGV("DEBUG(%s[%d]):-IN-", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;
    status_t nodeDqRet[OTF_NODE_BASE];
    status_t checkRet = NO_ERROR;

    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer buffer[OTF_NODE_BASE];
    int pipeId = 0;
    int v4l2Colorformat = 0;
    int planeCount[OTF_NODE_BASE] = {0};
    int bufferIndex[OTF_NODE_BASE];
    for (int i = OUTPUT_NODE; i < OTF_NODE_BASE; i++) {
        bufferIndex[i] = -2;
        nodeDqRet[i] = NO_ERROR;
    }
    uint32_t captureNodeCount = 0;
    uint32_t checkPollingCount = 0;

    /* 1. Pop from request frame queue */
    ret = m_requestFrameQ->waitAndPopProcessQ(&newFrame);
    if (ret == TIMED_OUT) {
        CLOGW("WARN(%s[%d]):requestFrameQ wait timeout", __FUNCTION__, __LINE__);
        return ret;
    } else if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):requestFrameQ wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return ret;
    }

    if (newFrame == NULL) {
        CLOGE("ERR(%s[%d]):New frame is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* 2. Get output buffer(SrcBuffer) from node */
    if (m_node[OUTPUT_NODE] != NULL) {
        ret = m_node[OUTPUT_NODE]->getBuffer(&(buffer[OUTPUT_NODE]), &(bufferIndex[OUTPUT_NODE]));
        nodeDqRet[OUTPUT_NODE] = ret;
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):node(%s)->getBuffer() fail, index(%d), frameCount(%d), ret(%d)",
                    __FUNCTION__, __LINE__, m_deviceInfo->nodeName[OUTPUT_NODE],
                    bufferIndex[OUTPUT_NODE], newFrame->getFrameCount(), ret);
            /* TODO: doing exception handling */
            /* Comment out : dqblock case was disappeared */
            /*
            for (int i = (MAX_NODE - 1); i > OUTPUT_NODE; i--) {
                if (newFrame->getRequest(getPipeId() + i) == true)
                    m_skipPutBuffer[i] = true;
            }

            ret = m_completeFrame(newFrame, false);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):Complete frame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            }

            goto CLEAN;
            */

            if (bufferIndex[OUTPUT_NODE] >= 0) {
                newFrame = m_runningFrameList[OUTPUT_NODE][bufferIndex[OUTPUT_NODE]];
            } else {
                ret = newFrame->getSrcBuffer(getPipeId(), &buffer[OUTPUT_NODE]);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Frame get buffer fail, frameCount(%d), ret(%d)",
                            __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
                } else {
                    buffer[OUTPUT_NODE].index = -2;
                }
            }

            if (newFrame == NULL) {
                CLOGE("ERR(%s[%d]):Invalid DQ buffer index(%d)", __FUNCTION__, __LINE__, bufferIndex[OUTPUT_NODE]);
                ret = BAD_VALUE;
                goto EXIT;
            }

            ret = newFrame->setSrcBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):setSrcBuffer state fail, frameCount(%d), ret(%d)",
                        __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
            }
        } else {
            if (bufferIndex[OUTPUT_NODE] < 0) {
                CLOGE("ERR(%s[%d]):Invalid DQ buffer index(%d)", __FUNCTION__, __LINE__, bufferIndex[OUTPUT_NODE]);
                ret = BAD_VALUE;
                goto EXIT;
            }

            /*
                prevent the frame null pointer exception, get the frame from m_runningFrameList
                1. in case of NDONE,  dq order was reversed.
                2. always use the m_runningFrameList instead of m_requestFrameQ
            */
            newFrame = m_runningFrameList[OUTPUT_NODE][bufferIndex[OUTPUT_NODE]];
            ret = newFrame->getSrcBuffer(getPipeId(), &buffer[OUTPUT_NODE]);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):Frame get buffer fail, frameCount(%d), ret(%d)",
                        __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
            } else {
                if (bufferIndex[OUTPUT_NODE] != buffer[OUTPUT_NODE].index) {
                    ret = newFrame->getSrcBuffer(getPipeId(), &buffer[OUTPUT_NODE]);
                    if (ret != NO_ERROR) {
                        CLOGE("ERR(%s[%d]):Frame get buffer fail, frameCount(%d), ret(%d)",
                                __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
                    }
                }

                ret = newFrame->setSrcBufferState(getPipeId(), ENTITY_BUFFER_STATE_COMPLETE);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):setSrcBuffer state fail, frameCount(%d), ret(%d)",
                            __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
                }

                if (m_reprocessing == false)
                    m_activityControl->activityAfterExecFunc(getPipeId(), (void *)&buffer[OUTPUT_NODE]);
            }
        }

        if (bufferIndex[OUTPUT_NODE] >= 0) {
    /* 3. Update frame from dynamic metadata of output node buffer(SrcBuffer) for request, ... */
            ret = m_updateMetadataToFrame(buffer[OUTPUT_NODE].addr[buffer[OUTPUT_NODE].planeCount - 1], buffer[OUTPUT_NODE].index, newFrame, OUTPUT_NODE);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):Update metadata fail, frameCount(%d), ret(%d)",
                        __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
            }

            m_runningFrameList[OUTPUT_NODE][bufferIndex[OUTPUT_NODE]] = NULL;
            m_numOfRunningFrame[OUTPUT_NODE]--;
        }
    }

    if (m_parameters->isUseEarlyFrameReturn() == true
        && m_reprocessing == false
        && m_frameDoneQ != NULL\
        && m_flagFrameDoneQ == true) {
        newFrame->incRef();
        m_frameDoneQ->pushProcessQ(&newFrame);
    }

    /* 4. Get capture buffer(DstBuffer) from node */
    for (int i = (OTF_NODE_BASE - 1); i > OUTPUT_NODE; i--) {
        ret = NO_ERROR;

        if (m_node[i] == NULL)
            continue;

        pipeId = getPipeId((enum NODE_TYPE)i);
        if (pipeId < 0) {
            CLOGE("ERR(%s[%d]):getPipeId(%d) fail", __FUNCTION__, __LINE__, i);
            ret = BAD_VALUE;
            goto EXIT;
        }

        if (m_node[i] != NULL
            && newFrame->getRequest(pipeId) == true) {
#ifndef SKIP_SCHECK_POLLING
            if (pipeId == PIPE_SCP && checkPollingCount == 0)
                ret = m_checkPolling(m_node[i]);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):m_checkPolling fail, frameCount(%d), ret(%d)",
                        __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
                /* TODO: doing exception handling */
                // HACK: for panorama shot
                //return false;
            }
            checkPollingCount++;
#endif
            ret = m_node[i]->getBuffer(&(buffer[i]), &(bufferIndex[i]));
            nodeDqRet[i] = ret;
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):node(%s)->getBuffer() fail, index(%d), frameCount(%d), ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i],
                        bufferIndex[i], newFrame->getFrameCount(), ret);
                /* TODO: doing exception handling */

                if (bufferIndex[i] >= 0) {
                    newFrame = m_runningFrameList[i][bufferIndex[i]];
                } else {
                    ret = newFrame->getDstBuffer(getPipeId(), &buffer[i], i);
                    if (ret != NO_ERROR) {
                        CLOGE("ERR(%s[%d]):Frame get buffer fail, frameCount(%d), ret(%d)",
                                __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
                    } else {
                        bufferIndex[i] = buffer[i].index = -2;
                    }
                    newFrame->setRequest(pipeId, false);
                }

                if (newFrame == NULL) {
                    CLOGE("ERR(%s[%d]):Invalid DQ buffer index(%d)", __FUNCTION__, __LINE__, bufferIndex[i]);
                    ret = BAD_VALUE;
                    goto EXIT;
                }

                ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR, i);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                            __FUNCTION__, __LINE__, getPipeId(), newFrame->getFrameCount(), ret);
                }
            }

            if (bufferIndex[i] >= 0) {
                m_runningFrameList[i][bufferIndex[i]] = NULL;
                m_numOfRunningFrame[i]--;
            }

    /* 5. Link capture node buffer(DstBuffer) to frame */
            if (bufferIndex[i] >= 0 && nodeDqRet[i] == NO_ERROR) {
                if (bufferIndex[i] != buffer[i].index)
                    newFrame = m_runningFrameList[i][bufferIndex[i]];

                /* HACK: Should change ExynosCamera, Frame */
                ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_REQUESTED, i);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                            __FUNCTION__, __LINE__, pipeId, newFrame->getFrameCount(), ret);
                }

                ret = newFrame->setDstBuffer(getPipeId(), buffer[i], i, INDEX(pipeId));
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):Frame set dst buffer fail, frameCount(%d), ret(%d)",
                            __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
                    /* TODO: doing exception handling */
                    if (m_bufferManager[i] != NULL)
                        ret = m_bufferManager[i]->putBuffer(buffer[i].index, EXYNOS_CAMERA_BUFFER_POSITION_NONE);

                    if (ret != NO_ERROR) {
                        CLOGE("ERR(%s[%d]):Buffer manager putBuffer fail, manager(%d), frameCount(%d), ret(%d)",
                                __FUNCTION__, __LINE__, i, newFrame->getFrameCount(), ret);
                    }

                    newFrame->setRequest(pipeId, false);
                }

                ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_COMPLETE, i);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):setDstBuffer state fail, pipeID(%d), frameCount(%d), ret(%d)",
                            __FUNCTION__, __LINE__, getPipeId(), newFrame->getFrameCount(), ret);
                }

                captureNodeCount++;
            }

    /* 6. Update metadata of capture node buffer(DstBuffer) from output node buffer(SrcBuffer) */
            if (m_node[OUTPUT_NODE] != NULL
                && m_deviceInfo->nodeNum[i] != FIMC_IS_VIDEO_HWFC_JPEG_NUM
                && m_deviceInfo->nodeNum[i] != FIMC_IS_VIDEO_HWFC_THUMB_NUM) {
                m_node[OUTPUT_NODE]->getColorFormat(&v4l2Colorformat, &planeCount[OUTPUT_NODE]);
                m_node[i]->getColorFormat(&v4l2Colorformat, &planeCount[i]);

                camera2_shot_ext *shot_ext_src = (struct camera2_shot_ext *)(buffer[OUTPUT_NODE].addr[(planeCount[OUTPUT_NODE] - 1)]);
                camera2_shot_ext *shot_ext_dst = (struct camera2_shot_ext *)(buffer[i].addr[(planeCount[i] - 1)]);
                if (shot_ext_src != NULL && shot_ext_dst != NULL) {
                    memcpy(&shot_ext_dst->shot.ctl, &shot_ext_src->shot.ctl, sizeof(struct camera2_ctl) - sizeof(struct camera2_entry_ctl));
                    memcpy(&shot_ext_dst->shot.udm, &shot_ext_src->shot.udm, sizeof(struct camera2_udm));
                    memcpy(&shot_ext_dst->shot.dm, &shot_ext_src->shot.dm, sizeof(struct camera2_dm));

                    shot_ext_dst->setfile = shot_ext_src->setfile;
                    shot_ext_dst->drc_bypass = shot_ext_src->drc_bypass;
                    shot_ext_dst->dis_bypass = shot_ext_src->dis_bypass;
                    shot_ext_dst->dnr_bypass = shot_ext_src->dnr_bypass;
                    shot_ext_dst->fd_bypass = shot_ext_src->fd_bypass;
                    shot_ext_dst->shot.dm.request.frameCount = shot_ext_src->shot.dm.request.frameCount;
                    shot_ext_dst->shot.magicNumber= shot_ext_src->shot.magicNumber;
                } else {
                    CLOGE("ERR(%s[%d]):metadata address fail, frameCount(%d) shot_ext src(%p) dst(%p) ",
                                __FUNCTION__, __LINE__, newFrame->getFrameCount(), shot_ext_src, shot_ext_dst);
                }
                /* Comment out : It was not useful for metadate fully update, I want know reasons for the existence. */
                /* memcpy(buffer[i].addr[(planeCount[i] - 1)], buffer[OUTPUT_NODE].addr[(planeCount[OUTPUT_NODE] - 1)], sizeof(struct camera2_shot_ext)); */
            }
        }
    }

    /*
     * skip condition :
     * 1 : all capture nodes are not valid.
     * 2 : one of capture nodes is not valid.
     */
    for (int i = OUTPUT_NODE; i < OTF_NODE_BASE; i++) {
        checkRet |= nodeDqRet[i];
    }

    if (captureNodeCount == 0 || checkRet != NO_ERROR) {
        if (newFrame->getFrameType() == FRAME_TYPE_INTERNAL) {
            CLOGI("INFO(%s[%d]):InternalFrame(%d) frameCount(%d)\
                    : captureNodeCount == %d || checkRet(%d) != NO_ERROR.\
                    so, setFrameState(FRAME_STATE_SKIPPED)",
                __FUNCTION__, __LINE__, newFrame->getFrameType(), newFrame->getFrameCount(), captureNodeCount, checkRet);
        } else {
            CLOGE("ERR(%s[%d]):frameCount(%d)\
                    : captureNodeCount == %d || checkRet(%d) != NO_ERROR.\
                    so, setFrameState(FRAME_STATE_SKIPPED)",
                __FUNCTION__, __LINE__, newFrame->getFrameCount(), captureNodeCount, checkRet);
        }

        /* set err on frame */
        newFrame->setFrameState(FRAME_STATE_SKIPPED);

        /* set err on src */
        ret = newFrame->setSrcBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setSrcBufferState(%d, ENTITY_BUFFER_STATE_ERROR) fail, frameCount(%d), ret(%d)",
                __FUNCTION__, __LINE__, getPipeId(), newFrame->getFrameCount(), ret);
        }

        /* set err on dst */
        for (int i = OUTPUT_NODE + 1; i < OTF_NODE_BASE; i++) {
            int dstPipeId = getPipeId((enum NODE_TYPE)i);

            if (dstPipeId < 0)
                continue;

            if (newFrame->getRequest(dstPipeId) == true) {
                ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR, i);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):setDstBufferState(Pipe ID(%d),\
                            ENTITY_BUFFER_STATE_ERROR, %d) fail, \
                            frameCount(%d), ret(%d)",
                            __FUNCTION__, __LINE__, getPipeId(), i, newFrame->getFrameCount(), ret);
                }
            }
        }
    }

    /* 7. Complete frame */
    ret = m_completeFrame(newFrame);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):Complete frame fail, frameCount(%d), ret(%d)",
                __FUNCTION__, __LINE__, newFrame->getFrameCount(), ret);
        /* TODO: doing exception handling */
    }

    /* 8. Push frame to out of Pipe */
CLEAN:
    m_outputFrameQ->pushProcessQ(&newFrame);
    if ((m_parameters->isUseEarlyFrameReturn() == false
        || m_reprocessing == true)
        && m_frameDoneQ != NULL
        && m_flagFrameDoneQ == true)
        m_frameDoneQ->pushProcessQ(&newFrame);

    for (int i = OUTPUT_NODE; i < OTF_NODE_BASE; i++)
        ret |= nodeDqRet[i];

    CLOGV("DEBUG(%s[%d]):OUT-", __FUNCTION__, __LINE__);

EXIT:
#ifdef USE_MCPIPE_SERIALIZATION_MODE
    if (m_serializeOperation == true) {
        ExynosCameraMCPipe::g_serializationLock.unlock();
        CLOGD("DEBUG(%s[%d]):%s Critical Section END",
                __FUNCTION__, __LINE__, m_name);
    }
#endif

    return ret;
}

status_t ExynosCameraMCPipe::m_checkShotDone(struct camera2_shot_ext *shot_ext)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (shot_ext == NULL) {
        CLOGE("ERR(%s[%d]):shot_ext is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (shot_ext->node_group.leader.request != 1) {
        CLOGW("WARN(%s[%d]):3a1 NOT DONE, frameCount(%d)", __FUNCTION__, __LINE__,
                getMetaDmRequestFrameCount(shot_ext));
        /* TODO: doing exception handling */
        return INVALID_OPERATION;
    }

    return OK;
}

/* m_updateMetadataFromFrame() will be deprecated */
status_t ExynosCameraMCPipe::m_updateMetadataFromFrame(ExynosCameraFrame *frame, ExynosCameraBuffer *buffer)
{
    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buffer->addr[buffer->planeCount - 1]);

    if (shot_ext != NULL) {
        int perframePosition = 0;
        int zoomParamInfo = m_parameters->getZoomLevel();
        int zoomFrameInfo = 0;
        int previewW = 0, previewH = 0;
        int pictureW = 0, pictureH = 0;
        int videoW = 0, videoH = 0;
        ExynosRect sensorSize;
        ExynosRect bnsSize;
        ExynosRect previewBayerCropSize;
        ExynosRect pictureBayerCropSize;
        ExynosRect bdsSize;
        camera2_node_group node_group_info;
        camera2_node_group node_group_info_isp;
        camera2_node_group node_group_info_tpu_sc;
        char captureNodeName[CAPTURE_NODE_MAX][EXYNOS_CAMERA_NAME_STR_SIZE];
        for (int i = 0; i < CAPTURE_NODE_MAX; i++)
            memset(captureNodeName[i], 0, EXYNOS_CAMERA_NAME_STR_SIZE);

        if (INDEX(getPipeId()) == (uint32_t)m_parameters->getPerFrameControlPipe()
            || getPipeId() == (uint32_t)m_parameters->getPerFrameControlReprocessingPipe()) {
            frame->getMetaData(shot_ext);

            if (m_parameters->getHalVersion() != IS_HAL_VER_3_2) {
                ret = m_parameters->duplicateCtrlMetadata((void *)shot_ext);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):duplicate Ctrl metadata fail", __FUNCTION__, __LINE__);
                    return INVALID_OPERATION;
                }
                // Setfile index is updated by capture intent at HAL3
                setMetaSetfile(shot_ext, m_setfile);
            }
        }

        if (m_reprocessing == false)
            m_activityControl->activityBeforeExecFunc(getPipeId(), (void *)buffer);

#ifdef SET_SETFILE_BY_SHOT_REPROCESSING
        /* setfile setting */
        if (m_reprocessing == true) {
            int yuvRange = 0;
            int setfile = 0;
            m_parameters->getSetfileYuvRange(m_reprocessing, &setfile, &yuvRange);
            ALOGD("INFO(%s[%d]):setfile(%d),m_reprocessing(%d)", __FUNCTION__, __LINE__, setfile, m_reprocessing);
            setMetaSetfile(shot_ext, setfile);
        }
#endif

        CLOGV("DEBUG(%s[%d]):frameCount(%d), rCount(%d)",
                __FUNCTION__, __LINE__,
                frame->getFrameCount(), getMetaDmRequestFrameCount(shot_ext));

        frame->getNodeGroupInfo(&node_group_info, m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameLeaderInfo.perframeInfoIndex, &zoomFrameInfo);

        /* HACK: To speed up DZOOM */
        if ((getPipeId() == (uint32_t)m_parameters->getPerFrameControlPipe()
            || getPipeId() == (uint32_t)m_parameters->getPerFrameControlReprocessingPipe())
            && zoomFrameInfo != zoomParamInfo) {
            CLOGI("INFO(%s[%d]):zoomFrameInfo(%d), zoomParamInfo(%d)",
                    __FUNCTION__, __LINE__, zoomFrameInfo, zoomParamInfo);

            frame->getNodeGroupInfo(&node_group_info_isp, m_parameters->getPerFrameInfoIsp(), &zoomFrameInfo);
            frame->getNodeGroupInfo(&node_group_info_tpu_sc, m_parameters->getPerFrameInfoDis(), &zoomFrameInfo);

            m_parameters->getPictureSize(&pictureW, &pictureH);
            m_parameters->getPreviewBayerCropSize(&sensorSize, &previewBayerCropSize);

            if (m_reprocessing == false) {
                m_parameters->getPreviewBdsSize(&bdsSize);
                m_parameters->getHwPreviewSize(&previewW, &previewH);
                m_parameters->getVideoSize(&videoW, &videoH);

                ExynosCameraNodeGroup3AA::updateNodeGroupInfo(
                        m_cameraId,
                        &node_group_info,
                        previewBayerCropSize,
                        bdsSize,
                        previewW, previewH,
                        videoW, videoH);

                ExynosCameraNodeGroupISP::updateNodeGroupInfo(
                        m_cameraId,
                        &node_group_info_isp,
                        previewBayerCropSize,
                        bdsSize,
                        previewW, previewH,
                        videoW, videoH,
                        m_parameters->getHWVdisMode());

                ExynosCameraNodeGroupDIS::updateNodeGroupInfo(
                        m_cameraId,
                        &node_group_info_tpu_sc,
                        previewBayerCropSize,
                        bdsSize,
                        previewW, previewH,
                        videoW, videoH,
                        m_parameters->getHWVdisMode());

                frame->storeNodeGroupInfo(&node_group_info,     m_parameters->getPerFrameInfo3AA(), zoomParamInfo);
                frame->storeNodeGroupInfo(&node_group_info_isp, m_parameters->getPerFrameInfoIsp(), zoomParamInfo);
                frame->storeNodeGroupInfo(&node_group_info_tpu_sc, m_parameters->getPerFrameInfoDis(), zoomParamInfo);
            } else {
                m_parameters->getPictureBayerCropSize(&bnsSize, &pictureBayerCropSize);
                m_parameters->getPictureBdsSize(&bdsSize);

                ExynosCameraNodeGroup::updateNodeGroupInfo(
                        m_cameraId,
                        &node_group_info,
                        &node_group_info_isp,
                        previewBayerCropSize,
                        pictureBayerCropSize,
                        bdsSize,
                        pictureW, pictureH,
                        m_parameters->getUsePureBayerReprocessing(),
                        m_parameters->isReprocessing3aaIspOTF());

                frame->storeNodeGroupInfo(&node_group_info, m_parameters->getPerFrameInfoReprocessingPure3AA(), zoomParamInfo);
                frame->storeNodeGroupInfo(&node_group_info_isp, m_parameters->getPerFrameInfoReprocessingPureIsp(), zoomParamInfo);
            }
        }

        /* Update node's size & request */
        memset(&shot_ext->node_group, 0x0, sizeof(camera2_node_group));

        if (node_group_info.leader.request == 1) {
            if (m_checkNodeGroupInfo(m_deviceInfo->nodeName[OUTPUT_NODE], &m_curNodeGroupInfo.leader, &node_group_info.leader) != NO_ERROR)
                CLOGW("WARN(%s[%d]): m_checkNodeGroupInfo(leader) fail", __FUNCTION__, __LINE__);

            setMetaNodeLeaderInputSize(shot_ext,
                    node_group_info.leader.input.cropRegion[0],
                    node_group_info.leader.input.cropRegion[1],
                    node_group_info.leader.input.cropRegion[2],
                    node_group_info.leader.input.cropRegion[3]);

            setMetaNodeLeaderOutputSize(shot_ext,
                    node_group_info.leader.output.cropRegion[0],
                    node_group_info.leader.output.cropRegion[1],
                    node_group_info.leader.output.cropRegion[2],
                    node_group_info.leader.output.cropRegion[3]);

            setMetaNodeLeaderRequest(shot_ext, node_group_info.leader.request);
            setMetaNodeLeaderVideoID(shot_ext,
                    m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameLeaderInfo.perFrameVideoID);
        }

        if (CAPTURE_NODE_MAX < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):PipeId(%d) has Invalid perframeSupportNodeNum: \
                    CAPTURE_NODE_MAX(%d) < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum(%d), assert!!!!",
                    __FUNCTION__, __LINE__, getPipeId(), CAPTURE_NODE_MAX, m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum);
        }

        /* Update capture node request from Frame */
        for (int i = CAPTURE_NODE; i < OTF_NODE_BASE; i++) {
            int funcRet = NO_ERROR;
            if (m_node[i] != NULL) {
                funcRet = m_getPerframePosition(&perframePosition, getPipeId((enum NODE_TYPE)i));
                if (funcRet != NO_ERROR)
                    continue;

                node_group_info.capture[perframePosition].request = frame->getRequest(getPipeId((enum NODE_TYPE)i));
                strncpy(captureNodeName[perframePosition], m_deviceInfo->nodeName[i], EXYNOS_CAMERA_NAME_STR_SIZE - 1);
            }
        }

        for (int i = 0; i < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum; i ++) {
            /*
             * To set 3AP BDS size on full OTF,
             * We need to set perframeSize.
             * set size when request is 0. so, no side effect.
             */
            /* if (node_group_info.capture[i].request == 1) { */

            if (m_checkNodeGroupInfo(captureNodeName[i], &m_curNodeGroupInfo.capture[i], &node_group_info.capture[i]) != NO_ERROR)
                CLOGW("WARN(%s[%d]): m_checkNodeGroupInfo(%d) fail", __FUNCTION__, __LINE__, i);

            setMetaNodeCaptureInputSize(shot_ext, i,
                    node_group_info.capture[i].input.cropRegion[0],
                    node_group_info.capture[i].input.cropRegion[1],
                    node_group_info.capture[i].input.cropRegion[2],
                    node_group_info.capture[i].input.cropRegion[3]);

            setMetaNodeCaptureOutputSize(shot_ext, i,
                    node_group_info.capture[i].output.cropRegion[0],
                    node_group_info.capture[i].output.cropRegion[1],
                    node_group_info.capture[i].output.cropRegion[2],
                    node_group_info.capture[i].output.cropRegion[3]);

            setMetaNodeCaptureRequest(shot_ext, i, node_group_info.capture[i].request);
            setMetaNodeCaptureVideoID(shot_ext, i,
                    m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameCaptureInfo[i].perFrameVideoID);
        }

        /*
           CLOGI("INFO(%s[%d]):frameCount(%d)", __FUNCTION__, __LINE__, shot_ext->shot.dm.request.frameCount);
           frame->dumpNodeGroupInfo(m_deviceInfo->nodeName[OUTPUT_NODE]);
           m_dumpPerframeNodeGroupInfo("m_perframeMainNodeGroupInfo", m_perframeMainNodeGroupInfo[OUTPUT_NODE]);

           for (int i = (OUTPUT_NODE + 1); i < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum; i++)
           m_dumpPerframeNodeGroupInfo("m_perframeCaptureNodeGroupInfo", m_perframeMainNodeGroupInfo[i]);
         */

        /* dump info on shot_ext, just before qbuf */
        /* m_dumpPerframeShotInfo(m_deviceInfo->nodeName[OUTPUT_NODE], frame->getFrameCount(), shot_ext); */
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_updateMetadataFromFrame_v2(ExynosCameraFrame *frame, ExynosCameraBuffer *buffer)
{
    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buffer->addr[buffer->planeCount - 1]);

    if (shot_ext != NULL) {
        int perframePosition = 0;
        int zoomParamInfo = m_parameters->getZoomLevel();
        int zoomFrameInfo = 0;
        int previewW = 0, previewH = 0;
        int pictureW = 0, pictureH = 0;
        int videoW = 0, videoH = 0;
        ExynosRect sensorSize;
        ExynosRect bnsSize;
        ExynosRect previewBayerCropSize;
        ExynosRect pictureBayerCropSize;
        ExynosRect bdsSize;
        camera2_node_group node_group_info;
        char captureNodeName[CAPTURE_NODE_MAX][EXYNOS_CAMERA_NAME_STR_SIZE];
        for (int i = 0; i < CAPTURE_NODE_MAX; i++)
            memset(captureNodeName[i], 0, EXYNOS_CAMERA_NAME_STR_SIZE);

        frame->getMetaData(shot_ext);

        if (INDEX(getPipeId()) == (uint32_t)m_parameters->getPerFrameControlPipe()
            || getPipeId() == (uint32_t)m_parameters->getPerFrameControlReprocessingPipe()) {
            if (m_parameters->getHalVersion() != IS_HAL_VER_3_2) {
                ret = m_parameters->duplicateCtrlMetadata((void *)shot_ext);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):duplicate Ctrl metadata fail", __FUNCTION__, __LINE__);
                    return INVALID_OPERATION;
                }
            }

            setMetaSetfile(shot_ext, m_setfile);
        }

        if (m_reprocessing == false)
            m_activityControl->activityBeforeExecFunc(getPipeId(), (void *)buffer);

#ifdef SET_SETFILE_BY_SHOT_REPROCESSING
        /* setfile setting */
        if (m_reprocessing == true) {
            int yuvRange = 0;
            int setfile = 0;
            m_parameters->getSetfileYuvRange(m_reprocessing, &setfile, &yuvRange);
            ALOGD("INFO(%s[%d]):setfile(%d),m_reprocessing(%d)", __FUNCTION__, __LINE__, setfile, m_reprocessing);
            setMetaSetfile(shot_ext, setfile);
        }
#endif

        CLOGV("DEBUG(%s[%d]):frameCount(%d), rCount(%d)",
                __FUNCTION__, __LINE__,
                frame->getFrameCount(), getMetaDmRequestFrameCount(shot_ext));

        frame->getNodeGroupInfo(&node_group_info, m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameLeaderInfo.perframeInfoIndex, &zoomFrameInfo);

#if 0
        /* HACK: To speed up DZOOM */
        if ((getPipeId() == m_parameters->getPerFrameControlPipe()
            || getPipeId() == m_parameters->getPerFrameControlReprocessingPipe())
            && zoomFrameInfo != zoomParamInfo) {
            CLOGI("INFO(%s[%d]):zoomFrameInfo(%d), zoomParamInfo(%d)",
                    __FUNCTION__, __LINE__, zoomFrameInfo, zoomParamInfo);

            updateNodeGroupInfo(
                    getPipeId(),
                    m_parameters,
                    &node_group_info);

            frame->storeNodeGroupInfo(&node_group_info, m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameLeaderInfo.perframeInfoIndex, zoomParamInfo);
        }
#endif

        /* Update node's size & request */
        memset(&shot_ext->node_group, 0x0, sizeof(camera2_node_group));

        if (node_group_info.leader.request == 1) {
            if (m_checkNodeGroupInfo(m_deviceInfo->nodeName[OUTPUT_NODE], &m_curNodeGroupInfo.leader, &node_group_info.leader) != NO_ERROR)
                CLOGW("WARN(%s[%d]): m_checkNodeGroupInfo(leader) fail", __FUNCTION__, __LINE__);

            setMetaNodeLeaderInputSize(shot_ext,
                    node_group_info.leader.input.cropRegion[0],
                    node_group_info.leader.input.cropRegion[1],
                    node_group_info.leader.input.cropRegion[2],
                    node_group_info.leader.input.cropRegion[3]);

            setMetaNodeLeaderOutputSize(shot_ext,
                    node_group_info.leader.output.cropRegion[0],
                    node_group_info.leader.output.cropRegion[1],
                    node_group_info.leader.output.cropRegion[2],
                    node_group_info.leader.output.cropRegion[3]);

            setMetaNodeLeaderRequest(shot_ext, node_group_info.leader.request);
            setMetaNodeLeaderVideoID(shot_ext,
                    m_perframeMainNodeGroupInfo[OUTPUT_NODE].perFrameLeaderInfo.perFrameVideoID);
        }

        if (CAPTURE_NODE_MAX < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum) {
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):PipeId(%d) has Invalid perframeSupportNodeNum:CAPTURE_NODE_MAX(%d) < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum(%d), assert!!!!",
                    __FUNCTION__, __LINE__, getPipeId(), CAPTURE_NODE_MAX, m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum);
        }

        /* Update capture node request from Frame */
        for (int i = CAPTURE_NODE; i < MAX_NODE; i++) {
            if (m_node[i] != NULL) {
                uint32_t videoId = m_deviceInfo->nodeNum[i] - FIMC_IS_VIDEO_BAS_NUM;
                for (perframePosition = 0; perframePosition < CAPTURE_NODE_MAX; perframePosition++) {
                    if (node_group_info.capture[perframePosition].vid == videoId) {
                        node_group_info.capture[perframePosition].request = frame->getRequest(getPipeId((enum NODE_TYPE)i));
                        strncpy(captureNodeName[perframePosition], m_deviceInfo->nodeName[i], EXYNOS_CAMERA_NAME_STR_SIZE - 1);
                        break;
                    }
                }
            }
        }

        for (int i = 0; i < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum; i ++) {
            /*
             * To set 3AP BDS size on full OTF,
             * We need to set perframeSize.
             * set size when request is 0. so, no side effect.
             */
            /* if (node_group_info.capture[i].request == 1) { */

            if (m_checkNodeGroupInfo(captureNodeName[i], &m_curNodeGroupInfo.capture[i], &node_group_info.capture[i]) != NO_ERROR)
                CLOGW("WARN(%s[%d]): m_checkNodeGroupInfo(%d) fail", __FUNCTION__, __LINE__, i);

            setMetaNodeCaptureInputSize(shot_ext, i,
                    node_group_info.capture[i].input.cropRegion[0],
                    node_group_info.capture[i].input.cropRegion[1],
                    node_group_info.capture[i].input.cropRegion[2],
                    node_group_info.capture[i].input.cropRegion[3]);

            setMetaNodeCaptureOutputSize(shot_ext, i,
                    node_group_info.capture[i].output.cropRegion[0],
                    node_group_info.capture[i].output.cropRegion[1],
                    node_group_info.capture[i].output.cropRegion[2],
                    node_group_info.capture[i].output.cropRegion[3]);

            setMetaNodeCaptureRequest(shot_ext, i, node_group_info.capture[i].request);
            setMetaNodeCaptureVideoID(shot_ext, i, node_group_info.capture[i].vid);
        }

        /*
           CLOGI("INFO(%s[%d]):frameCount(%d)", __FUNCTION__, __LINE__, shot_ext->shot.dm.request.frameCount);
           frame->dumpNodeGroupInfo(m_deviceInfo->nodeName[OUTPUT_NODE]);
           m_dumpPerframeNodeGroupInfo("m_perframeMainNodeGroupInfo", m_perframeMainNodeGroupInfo[OUTPUT_NODE]);

           for (int i = (OUTPUT_NODE + 1); i < m_perframeMainNodeGroupInfo[OUTPUT_NODE].perframeSupportNodeNum; i++)
           m_dumpPerframeNodeGroupInfo("m_perframeCaptureNodeGroupInfo", m_perframeMainNodeGroupInfo[i]);
         */

        /* dump info on shot_ext, just before qbuf */
        /* m_dumpPerframeShotInfo(m_deviceInfo->nodeName[OUTPUT_NODE], frame->getFrameCount(), shot_ext); */
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_updateMetadataToFrame(void *metadata, int index, ExynosCameraFrame *frame, enum NODE_TYPE nodeLocation)
{
    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;
    ExynosCameraFrame *curFrame = NULL;
    camera2_shot_ext *shot_ext;
    shot_ext = (struct camera2_shot_ext *)metadata;

    if (shot_ext == NULL) {
        CLOGE("ERR(%s[%d]):Meta buffer is null", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (index < 0) {
        CLOGE("ERR(%s[%d]):Invalid index(%d)", __FUNCTION__, __LINE__, index);
        return BAD_VALUE;
    }
    if (frame == NULL) {
        CLOGE("ERR(%s[%d]):frame is Null", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (nodeLocation < OUTPUT_NODE) {
        CLOGE("ERR(%s[%d]):Invalid node location(%d)", __FUNCTION__, __LINE__, nodeLocation);
        return BAD_VALUE;
    }

    if (m_metadataTypeShot == false) {
        CLOGV("DEBUG(%s[%d]):Stream type do not need update metadata", __FUNCTION__, __LINE__);
        return NO_ERROR;
    }

    /*
    ret = m_getFrameByIndex(&curFrame, index, nodeLocation);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_getFrameByIndex() fail, node(%s), index(%d), ret(%d)",
                __FUNCTION__, __LINE__, m_deviceInfo->nodeName[nodeLocation], index, ret);
        return ret;
    }
    */

    ret = frame->storeDynamicMeta(shot_ext);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):storeDynamicMeta() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    ret = frame->storeUserDynamicMeta(shot_ext);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):storeUserDynamicMeta() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    if (shot_ext->shot.dm.request.frameCount != 0)
        ret = frame->setMetaDataEnable(true);

    return ret;
}

status_t ExynosCameraMCPipe::m_getFrameByIndex(ExynosCameraFrame **frame, int index, enum NODE_TYPE nodeLocation)
{
    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (nodeLocation < OUTPUT_NODE) {
        CLOGE("ERR(%s[%d]):Invalid node location(%d)", __FUNCTION__, __LINE__, nodeLocation);
        return BAD_VALUE;
    }
    if (index < 0) {
        CLOGE("ERR(%s[%d]):Invalid index(%d)", __FUNCTION__, __LINE__, index);
        return BAD_VALUE;
    }

    *frame = m_runningFrameList[nodeLocation][index];
    if (*frame == NULL) {
        CLOGE("ERR(%s[%d]):Unknown buffer, index %d frame is NULL", __FUNCTION__, __LINE__, index);
        dump();
        return BAD_VALUE;
    }

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::m_completeFrame(
        ExynosCameraFrame *frame,
        bool isValid)
{
    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    if (frame == NULL) {
        CLOGE("ERR(%s[%d]):Frame is NULL", __FUNCTION__, __LINE__);
        dump();
        return BAD_VALUE;
    }

    if (isValid == false) {
        CLOGD("DEBUG(%s[%d]):NOT DONE frameCount(%d)", __FUNCTION__, __LINE__,
                frame->getFrameCount());
    }

    ret = frame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):Set entity state fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return ret;
    }

    CLOGV("DEBUG(%s[%d]):Entity pipeId(%d), frameCount(%d)",
            __FUNCTION__, __LINE__, getPipeId(), frame->getFrameCount());

    return ret;
}

status_t ExynosCameraMCPipe::m_setInput(ExynosCameraNode *nodes[], int32_t *nodeNums, int32_t *sensorIds)
{
    status_t ret = NO_ERROR;
    int currentSensorId[MAX_NODE] = {0};

    if (nodes == NULL || nodeNums == NULL || sensorIds == NULL) {
        CLOGE("ERR(%s[%d]): nodes == %p || nodeNum == %p || sensorId == %p",
                __FUNCTION__, __LINE__, nodes, nodeNums, sensorIds);
        return INVALID_OPERATION;
    }

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_flagValidInt(nodeNums[i]) == false)
            continue;

        if (m_flagValidInt(sensorIds[i]) == false)
            continue;

        if (nodes[i] == NULL)
            continue;

        currentSensorId[i] = nodes[i]->getInput();

        if (m_flagValidInt(currentSensorId[i]) == false ||
            currentSensorId[i] != sensorIds[i]) {

#ifdef INPUT_STREAM_MASK
            CLOGD("DEBUG(%s[%d]): setInput(sensorIds : %d) [src nodeNum : %d][nodeNums : %d][otf : %d][leader : %d][reprocessing : %d][unique sensorId : %d]",
                __FUNCTION__, __LINE__,
                 sensorIds[i],
                ((sensorIds[i] & INPUT_VINDEX_MASK) >>  INPUT_VINDEX_SHIFT) + FIMC_IS_VIDEO_BAS_NUM,
                nodeNums[i],
                ((sensorIds[i] & INPUT_MEMORY_MASK) >>  INPUT_MEMORY_SHIFT),
                ((sensorIds[i] & INPUT_LEADER_MASK) >>  INPUT_LEADER_SHIFT),
                ((sensorIds[i] & INPUT_STREAM_MASK) >>  INPUT_STREAM_SHIFT),
                ((sensorIds[i] & INPUT_MODULE_MASK) >>  INPUT_MODULE_SHIFT));
#else
            CLOGD("DEBUG(%s[%d]): setInput(sensorIds : %d)",
                __FUNCTION__, __LINE__, sensorIds[i]);
#endif
            ret = nodes[i]->setInput(sensorIds[i]);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]): nodeNums[%d] : %d, setInput(sensorIds : %d fail, ret(%d)",
                    __FUNCTION__, __LINE__, i, nodeNums[i], sensorIds[i],
                    ret);

                return ret;
            }
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_setPipeInfo(camera_pipe_info_t *pipeInfos)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;
    uint32_t planeCount = 2;
    enum YUV_RANGE yuvRange = YUV_FULL_RANGE;

    if (pipeInfos == NULL) {
        CLOGE("ERR(%s[%d]):pipeInfos is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    for (int i = OUTPUT_NODE; i < OTF_NODE_BASE; i++) {
        if (m_node[i] != NULL &&
            0 < pipeInfos[i].rectInfo.fullW &&
            0 < pipeInfos[i].rectInfo.fullH) {
            /* check about OUTPUT_NODE */
            if (i == OUTPUT_NODE
                && pipeInfos[i].bufInfo.type != V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) {
                CLOGE("ERR(%s[%d]):pipeInfos[%d].bufInfo.type is not Valid(type:%d)",
                        __FUNCTION__, __LINE__, i, pipeInfos[i].bufInfo.type);
                return BAD_VALUE;
            }

            /* check about CAPTURE_NODE */
            if (i >= CAPTURE_NODE
                && m_deviceInfo->connectionMode[i] != HW_CONNECTION_MODE_M2M_BUFFER_HIDING
                && pipeInfos[i].bufInfo.type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
                CLOGE("ERR(%s[%d]):pipeInfos[%d].bufInfo.type is not Valid(type:%d)",
                        __FUNCTION__, __LINE__, i, pipeInfos[i].bufInfo.type);
                return BAD_VALUE;
            }

            uint32_t bytePerPlane = 0;
            int colorFormat = pipeInfos[i].rectInfo.colorFormat;

            getYuvFormatInfo(colorFormat, &bytePerPlane, &planeCount);

            /* Add medadata plane count */
            planeCount++;

            if (m_reprocessing == false
                && (m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_SCP_NUM
                    || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_M0P_NUM
                    || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_M1P_NUM
                    || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_M2P_NUM
                    || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_M3P_NUM
                    || m_deviceInfo->nodeNum[i] == FIMC_IS_VIDEO_M4P_NUM)) {
                int setfile = 0;
                int previewYuvRange = 0;

                /* MC scaler can set different format with preview */
                /*
                   int colorFormat = m_parameters->getHwPreviewFormat();

                   if (colorFormat != pipeInfos[i].rectInfo.colorFormat) {
                   CLOGE("ERR(%s[%d]):SCP colorformat is not Valid(%d)",
                   __FUNCTION__, __LINE__, pipeInfos[i].rectInfo.colorFormat);
                   return BAD_VALUE;
                   }
                */

                m_parameters->getSetfileYuvRange(m_reprocessing, &setfile, &previewYuvRange);

                yuvRange = (enum YUV_RANGE)previewYuvRange;
            } else {
                planeCount = 2;
                yuvRange = YUV_FULL_RANGE;
            }

            ret = m_setNodeInfo(m_node[i], &pipeInfos[i], planeCount, yuvRange);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_setNodeInfo(W:%d, H:%d, buffer count:%d) fail(Node:%s), ret(%d)",
                        __FUNCTION__, __LINE__,
                        pipeInfos[i].rectInfo.fullW, pipeInfos[i].rectInfo.fullH,
                        pipeInfos[i].bufInfo.count, m_deviceInfo->nodeName[i], ret);
                return ret;
            }

            m_numBuffers[i] = pipeInfos[i].bufInfo.count;
            m_perframeMainNodeGroupInfo[i] = pipeInfos[i].perFrameNodeGroupInfo;
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_setNodeInfo(ExynosCameraNode *node, camera_pipe_info_t *pipeInfos,
                                         uint32_t planeCount, enum YUV_RANGE yuvRange,
                                         __unused bool flagBayer)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    bool flagSetRequest = false;
    unsigned int requestBufCount = 0;

    int currentW = 0;
    int currentH = 0;
    int currentV4l2Colorformat = 0;
    int currentPlanesCount = 0;
    enum YUV_RANGE currentYuvRange = YUV_FULL_RANGE;
    int  currentBufferCount = 0;
    enum v4l2_buf_type currentBufType;
    enum v4l2_memory currentMemType;

    if (node == NULL) {
        CLOGE("ERR(%s[%d]):node is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (pipeInfos == NULL) {
        CLOGE("ERR(%s[%d]):pipeInfos is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    requestBufCount = node->reqBuffersCount();

    /* If it already set */
    if (0 < requestBufCount) {
        node->getSize(&currentW, &currentH);
        node->getColorFormat(&currentV4l2Colorformat, &currentPlanesCount, &currentYuvRange);
        node->getBufferType(&currentBufferCount, &currentBufType, &currentMemType);

        if (/* setSize */
            currentW               != pipeInfos->rectInfo.fullW ||
            currentH               != pipeInfos->rectInfo.fullH ||
            /* setColorFormat */
            currentV4l2Colorformat != pipeInfos->rectInfo.colorFormat ||
            currentPlanesCount     != (int)planeCount ||
            currentYuvRange        != yuvRange ||
            /* setBufferType */
            currentBufferCount     != (int)pipeInfos->bufInfo.count ||
            currentBufType         != (enum v4l2_buf_type)pipeInfos->bufInfo.type ||
            currentMemType         != (enum v4l2_memory)pipeInfos->bufInfo.memory) {

            flagSetRequest = true;

            CLOGW("WARN(%s[%d]):Node is already requested. call clrBuffers()", __FUNCTION__, __LINE__);

            CLOGW("WARN(%s[%d]):W(%d -> %d), H(%d -> %d)",
                __FUNCTION__, __LINE__,
                currentW, pipeInfos->rectInfo.fullW,
                currentH, pipeInfos->rectInfo.fullH);

            CLOGW("WARN(%s[%d]):colorFormat(%d -> %d), planeCount(%d -> %d), yuvRange(%d -> %d)",
                __FUNCTION__, __LINE__,
                currentV4l2Colorformat, pipeInfos->rectInfo.colorFormat,
                currentPlanesCount,     planeCount,
                currentYuvRange,        yuvRange);

            CLOGW("WARN(%s[%d]):bufferCount(%d -> %d), bufType(%d -> %d), memType(%d -> %d)",
                __FUNCTION__, __LINE__,
                currentBufferCount, pipeInfos->bufInfo.count,
                currentBufType,     pipeInfos->bufInfo.type,
                currentMemType,     pipeInfos->bufInfo.memory);

            ret = node->clrBuffers();
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]): node->clrBuffers() fail", __FUNCTION__, __LINE__);
                return ret;
            }
        }
    } else {
        flagSetRequest = true;
    }

    if (flagSetRequest == true) {
        CLOGD("DEBUG(%s[%d]):set pipeInfos on %s, setFormat(%d, %d) and reqBuffers(%d)",
            __FUNCTION__, __LINE__,
            node->getName(), pipeInfos->rectInfo.fullW,
            pipeInfos->rectInfo.fullH, pipeInfos->bufInfo.count);

        bool flagValidSetFormatInfo = true;

        if (pipeInfos->rectInfo.fullW == 0 || pipeInfos->rectInfo.fullH == 0) {
            CLOGW("WARN(%s[%d]):Invalid size(%d x %d), skip setSize()",
                __FUNCTION__, __LINE__,
                pipeInfos->rectInfo.fullW, pipeInfos->rectInfo.fullH);

            flagValidSetFormatInfo = false;
        }
        node->setSize(pipeInfos->rectInfo.fullW, pipeInfos->rectInfo.fullH);

        if (pipeInfos->rectInfo.colorFormat == 0 || planeCount == 0) {
            CLOGW("WARN(%s[%d]):invalid colorFormat(%d), planeCount(%d), skip setColorFormat()",
                __FUNCTION__, __LINE__,
                pipeInfos->rectInfo.colorFormat, planeCount);

            flagValidSetFormatInfo = false;
        }
        node->setColorFormat(pipeInfos->rectInfo.colorFormat, planeCount, yuvRange);

        if ((int)pipeInfos->bufInfo.type == 0 || pipeInfos->bufInfo.memory == 0) {
            CLOGW("WARN(%s[%d]):Invalid bufInfo.type(%d), bufInfo.memory(%d), skip setBufferType()",
                __FUNCTION__, __LINE__,
                (int)pipeInfos->bufInfo.type, (int)pipeInfos->bufInfo.memory);

            flagValidSetFormatInfo = false;
        }
        node->setBufferType(pipeInfos->bufInfo.count,
                            (enum v4l2_buf_type)pipeInfos->bufInfo.type,
                            (enum v4l2_memory)pipeInfos->bufInfo.memory);

        if (flagValidSetFormatInfo == true) {
            ret = node->setFormat(pipeInfos->bytesPerPlane);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):node->setFormat() fail", __FUNCTION__, __LINE__);
                return ret;
            }
        }

        node->getBufferType(&currentBufferCount, &currentBufType, &currentMemType);

    } else {
        CLOGD("DEBUG(%s[%d]):Skip set pipeInfos setFormat(%d, %d) and reqBuffers(%d).",
            __FUNCTION__, __LINE__,
            pipeInfos->rectInfo.fullW, pipeInfos->rectInfo.fullH, pipeInfos->bufInfo.count);
    }

    if (currentBufferCount <= 0) {
        CLOGW("WARN(%s[%d]):Invalid currentBufferCount(%d), skip reqBuffers()",
                __FUNCTION__, __LINE__, currentBufferCount);
    } else {
        ret = node->reqBuffers();
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):node->reqBuffers() fail", __FUNCTION__, __LINE__);
            return ret;
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_getPerframePosition(int *perframePosition, uint32_t pipeId)
{
    status_t ret = NO_ERROR;

    if (perframePosition == NULL) {
        CLOGE("ERR(%s[%d]):perframePosition is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    switch(pipeId) {
    case PIPE_3AC:
        *perframePosition = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_3AC_POS : PERFRAME_FRONT_3AC_POS;
        break;
    case PIPE_3AP:
        *perframePosition = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_3AP_POS : PERFRAME_FRONT_3AP_POS;
        break;
    case PIPE_ISPC:
        *perframePosition = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_ISPC_POS : PERFRAME_FRONT_ISPC_POS;
        break;
    case PIPE_ISPP:
        *perframePosition = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_ISPP_POS : PERFRAME_FRONT_ISPP_POS;
        break;
    case PIPE_SCC:
        *perframePosition = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_SCC_POS : PERFRAME_FRONT_SCC_POS;
        break;
    case PIPE_SCP: /* Same as case of PIPE_MCSC0 */
        *perframePosition = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_SCP_POS : PERFRAME_FRONT_SCP_POS;
        break;
    case PIPE_MCSC1:
        *perframePosition = PERFRAME_BACK_MCSC1_POS;
        break;
    case PIPE_MCSC2:
        *perframePosition = PERFRAME_BACK_MCSC2_POS;
        break;
    case PIPE_3AC_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_3AC_POS;
        break;
    case PIPE_3AP_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_3AP_POS;
        break;
    case PIPE_ISPC_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_ISPC_POS;
        break;
    case PIPE_ISPP_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_ISPP_POS;
        break;
    case PIPE_MCSC0_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_MCSC0_POS;
        break;
    case PIPE_MCSC2_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_MCSC2_POS;
        break;
    case PIPE_MCSC3_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_MCSC3_POS;
        break;
    case PIPE_MCSC4_REPROCESSING:
        *perframePosition = PERFRAME_REPROCESSING_MCSC4_POS;
        break;
    default:
        CLOGV("ERR(%s[%d]):Invalid pipeID(%d)", __FUNCTION__, __LINE__, pipeId);
        ret = BAD_VALUE;
        break;
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_setSetfile(ExynosCameraNode *node, uint32_t pipeId)
{
    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;
    int yuvRange = 0;

    m_parameters->getSetfileYuvRange(m_reprocessing, &m_setfile, &yuvRange);

    if (m_parameters->getSetFileCtlMode() == true) {
        if (m_parameters->getSetFileCtl3AA() == true && INDEX(pipeId) == INDEX(PIPE_3AA)) {
            ret = node->setControl(V4L2_CID_IS_SET_SETFILE, m_setfile);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):setControl(%d) fail(ret = %d)", __FUNCTION__, __LINE__, m_setfile, ret);
                return ret;
            }
        } else if (m_parameters->getSetFileCtlISP() == true && INDEX(pipeId) == INDEX(PIPE_ISP)) {
            ret = node->setControl(V4L2_CID_IS_SET_SETFILE, m_setfile);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):setControl(%d) fail(ret = %d)", __FUNCTION__, __LINE__, m_setfile, ret);
                return ret;
            }
        } else if (m_parameters->getSetFileCtlSCP() == true && INDEX(pipeId) == INDEX(PIPE_SCP)) {
            ret = node->setControl(V4L2_CID_IS_COLOR_RANGE, yuvRange);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):setControl(%d) fail(ret = %d)", __FUNCTION__, __LINE__, m_setfile, ret);
                return ret;
            }
        }
    } else {
        m_setfile = mergeSetfileYuvRange(m_setfile, yuvRange);
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_forceDone(ExynosCameraNode *node, unsigned int cid, int value)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    if (node == NULL) {
        CLOGE("ERR(%s[%d]):node is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (cid != V4L2_CID_IS_FORCE_DONE) {
        CLOGW("ERR(%s[%d]):cid != V4L2_CID_IS_FORCE_DONE", __FUNCTION__, __LINE__);
    }

    /* "value" is not meaningful */
    ret = node->setControl(V4L2_CID_IS_FORCE_DONE, value);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):node V4L2_CID_IS_FORCE_DONE failed", __FUNCTION__, __LINE__);
        node->dump();
        return ret;
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_setMapBuffer(int nodeIndex)
{
    status_t ret = NO_ERROR;

    int bufferIndex[VIDEO_MAX_FRAME];
    for (int i = 0; i < VIDEO_MAX_FRAME; i++)
        bufferIndex[i] = -2;
    ExynosCameraBuffer buffer;

    if (m_bufferManager[nodeIndex]->getAllocatedBufferCount() > 0) {
        int index = 0;
        while (m_bufferManager[nodeIndex]->getNumOfAvailableBuffer() > 0) {
            ret |= m_bufferManager[nodeIndex]->getBuffer(&(bufferIndex[index]), EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL, &buffer);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):Buffer manager getBuffer fail, manager(%d), ret(%d)",
                        __FUNCTION__, __LINE__, nodeIndex, ret);
            }

            ret |= m_node[nodeIndex]->prepareBuffer(&buffer);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):node(%s)->putBuffer() fail, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[nodeIndex], ret);

            }
            index++;
        }

        while (index > 0) {
            index--;
            /* TODO: doing exception handling */
            ret |= m_bufferManager[nodeIndex]->putBuffer(bufferIndex[index], EXYNOS_CAMERA_BUFFER_POSITION_NONE);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):Buffer manager putBuffer fail, manager(%d), ret(%d)",
                        __FUNCTION__, __LINE__, nodeIndex, ret);
            }
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_setMapBuffer(ExynosCameraNode *node, ExynosCameraBuffer *buffer)
{
    status_t ret = NO_ERROR;

    if (buffer == NULL) {
        CLOGE("ERR(%s[%d]):Buffer is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (node == NULL) {
        CLOGE("ERR(%s[%d]):Node is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* Require code sync release-git to Repo */
#if 0
    ret = node->mapBuffer(buffer);
    if (ret != NO_ERROR)
        CLOGE("ERR(%s[%d]):mapBuffer() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
#endif

    return ret;
}

status_t ExynosCameraMCPipe::m_setJpegInfo(int nodeType, ExynosCameraBuffer *buffer)
{
    status_t ret = NO_ERROR;
    int pipeId = MAX_PIPE_NUM;
    ExynosRect pictureRect;
    ExynosRect thumbnailRect;
    int jpegQuality = -1;
    int thumbnailQuality = -1;
    exif_attribute_t exifInfo;
    debug_attribute_t *debugInfo;
    camera2_shot_ext *shot_ext = NULL;

    /* 1. Check the invalid parameters */
    if (buffer == NULL) {
        CLOGE("ERR(%s[%d]):buffer is NULL. pipeId %d",
                __FUNCTION__, __LINE__, pipeId);
        return BAD_VALUE;
    }

    /* 2. Get control metadata from buffer and pipeId */
    shot_ext = (struct camera2_shot_ext *)(buffer->addr[buffer->planeCount - 1]);
    pipeId = m_deviceInfo->pipeId[nodeType];

    /* 3. Get control informations from parameter & metadata */
    m_parameters->getPictureSize(&pictureRect.w, &pictureRect.h);
    m_parameters->getThumbnailSize(&thumbnailRect.w, &thumbnailRect.h);
    jpegQuality = m_parameters->getJpegQuality();
    thumbnailQuality = m_parameters->getThumbnailQuality();

    debugInfo = m_parameters->getDebugAttribute();

    /* 3. Set JPEG node perframe control information for each node */
    switch (pipeId) {
    case PIPE_HWFC_JPEG_SRC_REPROCESSING:
    case PIPE_HWFC_JPEG_DST_REPROCESSING:
        /* JPEG HAL setSize */
        ret = m_node[nodeType]->setSize(pictureRect.w, pictureRect.h);
        if (ret != NO_ERROR)
            CLOGE("ERR(%s[%d]):Failed to set size %dx%d into %s, ret %d",
                    __FUNCTION__, __LINE__,
                    pictureRect.w, pictureRect.h,
                    m_deviceInfo->nodeName[nodeType], ret);

        /* JPEG HAL setQuality */
        CLOGD("DEBUG(%s[%d]):m_node[nodeType]->setQuality(int)", __FUNCTION__, __LINE__);
        ret = m_node[nodeType]->setQuality(jpegQuality);
        if (ret != NO_ERROR)
            CLOGE("ERR(%s[%d]):Failed to set jpeg quality %d into %s, ret %d",
                    __FUNCTION__, __LINE__,
                    jpegQuality,
                    m_deviceInfo->nodeName[nodeType], ret);

        /* Create EXIF info */
        m_parameters->getFixedExifInfo(&exifInfo);
        if (ret != NO_ERROR)
            CLOGE("ERR(%s[%d]):Failed to get Fixed Exif Info, ret %d",
                    __FUNCTION__, __LINE__, ret);
        if (thumbnailRect.w > 0 && thumbnailRect.h > 0) {
            exifInfo.enableThumb = true;
        } else {
            exifInfo.enableThumb = false;
        }
        m_parameters->setExifChangedAttribute(&exifInfo, &pictureRect, &thumbnailRect, &shot_ext->shot);

        /* JPEG HAL setExifInfo */
        ret = m_node[nodeType]->setExifInfo(&exifInfo);
        if (ret != NO_ERROR)
            CLOGE("ERR(%s[%d]):Failed to set EXIF info into %s, ret %d",
                    __FUNCTION__, __LINE__,
                    m_deviceInfo->nodeName[nodeType], ret);

        /* JPEG HAL setDebugInfo */
        debugInfo = m_parameters->getDebugAttribute();
        ret = m_node[nodeType]->setDebugInfo(debugInfo);
        if (ret != NO_ERROR)
            CLOGE("ERR(%s[%d]):Failed to set DEBUG Info into %s, ret %d",
                    __FUNCTION__, __LINE__,
                    m_deviceInfo->nodeName[nodeType], ret);
        break;
    case PIPE_HWFC_THUMB_SRC_REPROCESSING:
        /* JPEG HAL setSize */
        if (thumbnailRect.w > 0 && thumbnailRect.h > 0) {
            ret = m_node[nodeType]->setSize(thumbnailRect.w, thumbnailRect.h);
            if (ret != NO_ERROR)
                CLOGE("ERR(%s[%d]):Failed to set thumbnail size %dx%d into %s, ret %d",
                        __FUNCTION__, __LINE__,
                        thumbnailRect.w, thumbnailRect.h,
                        m_deviceInfo->nodeName[nodeType], ret);
        }

        /* JPEG HAL setQuality */
        m_node[nodeType]->setQuality(thumbnailQuality);
        if (ret != NO_ERROR)
            CLOGE("ERR(%s[%d]):Failed to setQuality %d into %s, ret %d",
                    __FUNCTION__, __LINE__,
                    thumbnailQuality,
                    m_deviceInfo->nodeName[nodeType], ret);
        break;
    default:
        CLOGE("ERR(%s[%d]):Invalid pipeId %d", __FUNCTION__, __LINE__, pipeId);
        ret = BAD_VALUE;
        break;
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_startNode(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    for (int i = (OTF_NODE_BASE - 1); i >= OUTPUT_NODE; i--) {
        /* only M2M mode need stream on/off */
        /* TODO : flite has different sensorId bit */
        if (m_node[i] != NULL) {
            ret = m_node[i]->start();
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):node(%s)->start fail, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                return ret;
            }
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_stopNode(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    for (int i = OUTPUT_NODE; i < OTF_NODE_BASE; i++) {
        /* only M2M mode need stream on/off */
        /* TODO : flite has different sensorId bit */
        if (m_node[i] != NULL) {
            ret = m_node[i]->stop();
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):node(%s)->stop fail, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                return ret;
            }

            ret = m_node[i]->clrBuffers();
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):node(%s)->clrBuffers fail, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                return ret;
            }

            m_node[i]->removeItemBufferQ();
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_clearNode(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        if (m_node[i] != NULL) {
            ret = m_node[i]->clrBuffers();
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):node(%s)->clrBuffers fail, ret(%d)",
                        __FUNCTION__, __LINE__, m_deviceInfo->nodeName[i], ret);
                return ret;
            }
        }
    }

    return ret;
}

status_t ExynosCameraMCPipe::m_checkNodeGroupInfo(char *name, camera2_node *oldNode, camera2_node *newNode)
{
    if (oldNode == NULL || newNode == NULL) {
        CLOGE("ERR(%s[%d]): oldNode(%p) == NULL || newNode(%p) == NULL", __FUNCTION__, __LINE__, oldNode, newNode);
        return INVALID_OPERATION;
    }

    bool flagCropRegionChanged = false;

    for (int i = 0; i < 4; i++) {
        if (oldNode->input.cropRegion[i] != newNode->input.cropRegion[i] ||
            oldNode->output.cropRegion[i] != newNode->output.cropRegion[i]) {

            CLOGD("DEBUG(%s[%d]):(%s):oldCropSize(%d, %d, %d, %d / %d, %d, %d, %d) -> newCropSize(%d, %d, %d, %d / %d, %d, %d, %d)",
                __FUNCTION__, __LINE__,
                name,
                oldNode->input. cropRegion[0], oldNode->input. cropRegion[1], oldNode->input. cropRegion[2], oldNode->input. cropRegion[3],
                oldNode->output.cropRegion[0], oldNode->output.cropRegion[1], oldNode->output.cropRegion[2], oldNode->output.cropRegion[3],
                newNode->input. cropRegion[0], newNode->input. cropRegion[1], newNode->input. cropRegion[2], newNode->input. cropRegion[3],
                newNode->output.cropRegion[0], newNode->output.cropRegion[1], newNode->output.cropRegion[2], newNode->output.cropRegion[3]);

            break;
        }
    }

    for (int i = 0; i < 4; i++) {
        oldNode->input. cropRegion[i] = newNode->input. cropRegion[i];
        oldNode->output.cropRegion[i] = newNode->output.cropRegion[i];
    }

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::m_checkNodeGroupInfo(char *name, int index, camera2_node *oldNode, camera2_node *newNode)
{
    if (oldNode == NULL || newNode == NULL) {
        CLOGE("ERR(%s[%d]): oldNode(%p) == NULL || newNode(%p) == NULL", __FUNCTION__, __LINE__, oldNode, newNode);
        return INVALID_OPERATION;
    }

    bool flagCropRegionChanged = false;

    for (int i = 0; i < 4; i++) {
        if (oldNode->input.cropRegion[i] != newNode->input.cropRegion[i] ||
            oldNode->output.cropRegion[i] != newNode->output.cropRegion[i]) {

            CLOGD("DEBUG(%s[%d]): name %s : index %d: PerFrame oldCropSize (%d, %d, %d, %d / %d, %d, %d, %d) -> newCropSize (%d, %d, %d, %d / %d, %d, %d, %d)",
                __FUNCTION__, __LINE__,
                name,
                index,
                oldNode->input. cropRegion[0], oldNode->input. cropRegion[1], oldNode->input. cropRegion[2], oldNode->input. cropRegion[3],
                oldNode->output.cropRegion[0], oldNode->output.cropRegion[1], oldNode->output.cropRegion[2], oldNode->output.cropRegion[3],
                newNode->input. cropRegion[0], newNode->input. cropRegion[1], newNode->input. cropRegion[2], newNode->input. cropRegion[3],
                newNode->output.cropRegion[0], newNode->output.cropRegion[1], newNode->output.cropRegion[2], newNode->output.cropRegion[3]);

            break;
        }
    }

    for (int i = 0; i < 4; i++) {
        oldNode->input. cropRegion[i] = newNode->input. cropRegion[i];
        oldNode->output.cropRegion[i] = newNode->output.cropRegion[i];
    }

    return NO_ERROR;
}

status_t ExynosCameraMCPipe::m_checkNodeGroupInfo(int index, camera2_node *oldNode, camera2_node *newNode)
{
    return m_checkNodeGroupInfo(m_deviceInfo->nodeName[index], oldNode, newNode);
}

void ExynosCameraMCPipe::m_dumpRunningFrameList(void)
{
    CLOGI("INFO(%s[%d]):*********runningFrameList dump***********", __FUNCTION__, __LINE__);
    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        CLOGI("INFO(%s[%d]):m_numBuffers[%d] : %d", __FUNCTION__, __LINE__, i, m_numBuffers[i]);
        for (uint32_t j = 0; j < m_numBuffers[i]; j++) {
            if (m_runningFrameList[i][j] == NULL) {
                CLOGI("runningFrameList[%d][%d] is NULL", i, j);
            } else {
                CLOGI("runningFrameList[%d][%d]: fcount = %d",
                        i, j, m_runningFrameList[i][j]->getFrameCount());
            }
        }
    }

    return;
}

void ExynosCameraMCPipe::m_dumpPerframeNodeGroupInfo(const char *name, camera_pipe_perframe_node_group_info_t nodeInfo)
{
    if (name != NULL)
        CLOGI("DEBUG(%s[%d]):(%s) ++++++++++++++++++++", __FUNCTION__, __LINE__, name);

    CLOGI("\t\t perframeSupportNodeNum : %d", nodeInfo.perframeSupportNodeNum);
    CLOGI("\t\t perFrameLeaderInfo.perframeInfoIndex : %d", nodeInfo.perFrameLeaderInfo.perframeInfoIndex);
    CLOGI("\t\t perFrameLeaderInfo.perFrameVideoID : %d", nodeInfo.perFrameLeaderInfo.perFrameVideoID);

    for (int i = 0; i < CAPTURE_NODE_MAX; i++)
        CLOGI("\t\t perFrameCaptureInfo[%d].perFrameVideoID : %d", i, nodeInfo.perFrameCaptureInfo[i].perFrameVideoID);

    if (name != NULL)
        CLOGI("DEBUG(%s[%d]):(%s) ------------------------------", __FUNCTION__, __LINE__, name);

    return;
}

void ExynosCameraMCPipe::m_dumpPerframeShotInfo(const char *name, int frameCount, camera2_shot_ext *shot_ext)
{
    if (name != NULL)
        CLOGI("DEBUG(%s[%d]):(%s) frameCount(%d) ++++++++++++++++++++", __FUNCTION__, __LINE__, name, frameCount);

    if (shot_ext != NULL) {
        for (int i = 0; i < CAPTURE_NODE_MAX; i++) {
            CLOGI("DEBUG(%s[%d]):\t\t index(%d), vid(%d) request(%d) input (%d, %d, %d, %d) output (%d, %d, %d, %d)",
                __FUNCTION__, __LINE__,
                i,
                shot_ext->node_group.capture[i].vid,
                shot_ext->node_group.capture[i].request,
                shot_ext->node_group.capture[i].input.cropRegion[0],
                shot_ext->node_group.capture[i].input.cropRegion[1],
                shot_ext->node_group.capture[i].input.cropRegion[2],
                shot_ext->node_group.capture[i].input.cropRegion[3],
                shot_ext->node_group.capture[i].output.cropRegion[0],
                shot_ext->node_group.capture[i].output.cropRegion[1],
                shot_ext->node_group.capture[i].output.cropRegion[2],
                shot_ext->node_group.capture[i].output.cropRegion[3]);
        }
    } else {
        CLOGI("DEBUG(%s[%d]):\t\t shot_ext == NULL", __FUNCTION__, __LINE__);
    }

    if (name != NULL)
        CLOGI("DEBUG(%s[%d]):(%s) ------------------------------", __FUNCTION__, __LINE__, name);
}

void ExynosCameraMCPipe::m_configDvfs(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    bool newDvfs = m_parameters->getDvfsLock();

    if (newDvfs != m_dvfsLocked) {
        setControl(V4L2_CID_IS_DVFS_LOCK, 533000);
        m_dvfsLocked = newDvfs;
    }
}

bool ExynosCameraMCPipe::m_flagValidInt(int num)
{
    bool ret = false;

    if (num == -1 || num == 0)
        ret = false;
    else
        ret = true;

    return ret;
}

bool ExynosCameraMCPipe::m_checkThreadLoop(frame_queue_t *frameQ)
{
    Mutex::Autolock lock(m_pipeframeLock);
    bool loop = false;

    if (m_reprocessing == false)
        loop = true;

    if (m_oneShotMode == false)
        loop = true;

    if (frameQ->getSizeOfProcessQ() > 0)
        loop = true;

    if (m_flagTryStop == true)
        loop = false;

    return loop;
}

status_t ExynosCameraMCPipe::m_checkPolling(ExynosCameraNode *node)
{
    int ret = 0;

    ret = node->polling();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):polling fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */

        m_threadState = ERROR_POLLING_DETECTED;
        return ERROR_POLLING_DETECTED;
    }

    return NO_ERROR;
}

void ExynosCameraMCPipe::m_init(camera_device_info_t *deviceInfo)
{
    if (deviceInfo != NULL)
        m_deviceInfo = deviceInfo;
    else
        m_deviceInfo = NULL;

/*
 * For replace old pipe, MCPipe have all variables.
 * but, if exist old pipe, same variable not declared by MCPipe.
 */
    for (int i = OUTPUT_NODE; i < MAX_NODE; i++) {
        /*
        m_bufferManager[i] = NULL;
        m_node[i] = NULL;
        m_nodeNum[i] = -1;
        m_sensorIds[i] = -1;
        */
        m_secondaryNode[i] = NULL;
        m_secondaryNodeNum[i] = -1;
        m_secondarySensorIds[i] = -1;
        m_numOfRunningFrame[i] = 0;
        m_skipPutBuffer[i] = false;
        m_numBuffers[i] = 0;
        for (int j = 0; j < MAX_BUFFERS; j++)
            m_runningFrameList[i][j] = NULL;
        memset(&m_perframeMainNodeGroupInfo[i], 0x00, sizeof(camera_pipe_perframe_node_group_info_t));

        if (m_deviceInfo != NULL)
            m_pipeIdArr[i] = m_deviceInfo->pipeId[i];
        else
            m_pipeIdArr[i] = 0;
    }

    /*
    m_parameters = NULL;
    m_activityControl = NULL;
    m_exynosconfig = NULL;

    memset(m_name, 0x00, sizeof(m_name));

    m_inputFrameQ = NULL;
    */
    m_requestFrameQ = NULL;
    /*
    m_outputFrameQ = NULL;
    m_frameDoneQ = NULL;

    m_pipeId = 0;
    m_cameraId = -1;

    m_setfile = 0x0;

    m_prepareBufferCount = 0;

    m_reprocessing = false;
    m_flagStartPipe = false;
    m_flagTryStop = false;
    m_dvfsLocked = false;
    m_isBoosting = false;
    m_flagFrameDoneQ = false;

    m_threadCommand = 0;
    m_timeInterval = 0;
    m_threadState = 0;
    m_threadRenew = 0;

    memset(&m_curNodeGroupInfo, 0x00, sizeof(camera2_node_group));
    */
#ifdef USE_MCPIPE_SERIALIZATION_MODE
    m_serializeOperation = false;
#endif
#ifdef TEST_WATCHDOG_THREAD
    int testErrorDetect = 0;
#endif
}

}; /* namespace android */
