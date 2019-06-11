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
#define LOG_TAG "ExynosCameraPipe3AC"
#include <cutils/log.h>

#include "ExynosCameraPipe3AC.h"

namespace android {

ExynosCameraPipe3AC::~ExynosCameraPipe3AC()
{
        this->destroy();
}

status_t ExynosCameraPipe3AC::create(int32_t *sensorIds)
{
    CLOGD("[%s(%d)]", __FUNCTION__, __LINE__);
    int ret = 0;

    for (int i = 0; i < MAX_NODE; i++) {
        if (sensorIds) {
            CLOGD("DEBUG(%s[%d]):set new sensorIds[%d] : %d", __FUNCTION__, __LINE__, i, sensorIds[i]);
            m_sensorIds[i] = sensorIds[i];
        } else {
            m_sensorIds[i] = -1;
        }
    }

    m_node[CAPTURE_NODE] = new ExynosCameraNode();
    ret = m_node[CAPTURE_NODE]->create("3AC", m_cameraId);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): CAPTURE_NODE create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    ret = m_node[CAPTURE_NODE]->open(m_nodeNum[CAPTURE_NODE]);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): CAPTURE_NODE open fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }
    CLOGD("DEBUG(%s):Node(%d) opened", __FUNCTION__, m_nodeNum[CAPTURE_NODE]);

    /* mainNode is CAPTURE_NODE */
    m_mainNodeNum = CAPTURE_NODE;
    m_mainNode = m_node[m_mainNodeNum];

    /* setInput for 54xx */
    ret = m_setInput(m_node, m_nodeNum, m_sensorIds);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): m_setInput fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    m_mainThread = ExynosCameraThreadFactory::createThread(this, &ExynosCameraPipe3AC::m_mainThreadFunc, "3ACThread");

    m_inputFrameQ = new frame_queue_t(m_mainThread);

    m_prepareBufferCount = m_getPrepareBufferCount();

    CLOGI("INFO(%s[%d]):create() is succeed (%d) prepare (%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    return NO_ERROR;
}

status_t ExynosCameraPipe3AC::destroy(void)
{
    CLOGD("[%s(%d)]", __FUNCTION__, __LINE__);
    if (m_node[CAPTURE_NODE] != NULL) {
        if (m_node[CAPTURE_NODE]->close() != NO_ERROR) {
            CLOGE("ERR(%s): close fail", __FUNCTION__);
            return INVALID_OPERATION;
        }
        delete m_node[CAPTURE_NODE];
        m_node[CAPTURE_NODE] = NULL;
        CLOGD("DEBUG(%s):Node(CAPTURE_NODE, m_nodeNum : %d, m_sensorIds : %d) closed",
            __FUNCTION__, m_nodeNum[CAPTURE_NODE], m_sensorIds[CAPTURE_NODE]);
    }

    m_mainNode = NULL;
    m_mainNodeNum = -1;

    if (m_inputFrameQ != NULL) {
        m_inputFrameQ->release();
        delete m_inputFrameQ;
        m_inputFrameQ = NULL;
    }

    CLOGI("INFO(%s[%d]):destroy() is succeed (%d)", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipe3AC::m_getBuffer(void)
{
    ExynosCameraFrame *curFrame = NULL;
    ExynosCameraBuffer curBuffer;
    int index = -1;
    int ret = 0;
    struct camera2_shot_ext *shot_ext = NULL;

    if (m_numOfRunningFrame <= 0 || m_flagStartPipe == false) {
        CLOGD("DEBUG(%s[%d]): skip getBuffer, flagStartPipe(%d), numOfRunningFrame = %d", __FUNCTION__, __LINE__, m_flagStartPipe, m_numOfRunningFrame);
        return NO_ERROR;
    }

    ret = m_node[CAPTURE_NODE]->getBuffer(&curBuffer, &index);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):getBuffer(index : %d) fail", __FUNCTION__, __LINE__, index);
    }

    if (ret != NO_ERROR) {
        if (index < 0) {
            CLOGE("ERR(%s[%d]):index(%d) < 0. so just return without error handling...",
                __FUNCTION__, __LINE__, index);

            return INVALID_OPERATION;
        }

        if (curBuffer.addr[1] == NULL) {
            CLOGE("ERR(%s[%d]):curBuffer.addr[1] == NULL. so just return without error handling...",
                __FUNCTION__, __LINE__);

            return INVALID_OPERATION;
        }

        CLOGE("ERR(%s[%d]):error handling start...", __FUNCTION__, __LINE__);

        shot_ext = (struct camera2_shot_ext *)(curBuffer.addr[1]);

        CLOGE("ERR(%s[%d]):Shot done invalid, frame(cnt:%d, index(%d)) skip",
            __FUNCTION__, __LINE__, getMetaDmRequestFrameCount(shot_ext), index);

        ret = m_updateMetadataToFrame(curBuffer.addr[1], curBuffer.index);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): updateMetadataToFrame(%d) fail, ret(%d)", __FUNCTION__, __LINE__, curBuffer.index, ret);
        }

        /* complete frame */
        ret = m_completeFrame(&curFrame, curBuffer, false);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):complete frame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            return ret;
        }

        if (curFrame == NULL) {
            CLOGE("ERR(%s[%d]):curFrame is fail", __FUNCTION__, __LINE__);
            return ret;
        }

        /* Update the frame information with 3AA Drop */
        curFrame->set3aaDrop(true);
        curFrame->setIspcDrop(true);
        curFrame->setSccDrop(true);

        /* Push to outputQ */
        if (m_outputFrameQ != NULL) {
            m_outputFrameQ->pushProcessQ(&curFrame);
        } else {
            CLOGE("ERR(%s[%d]):m_outputFrameQ is NULL", __FUNCTION__, __LINE__);
        }

        return NO_ERROR;
    }

    m_activityControl->activityAfterExecFunc(getPipeId(), (void *)&curBuffer);

    ret = m_updateMetadataToFrame(curBuffer.addr[1], curBuffer.index);
    if (ret < 0)
        CLOGE("ERR(%s[%d]): updateMetadataToFrame fail, ret(%d)", __FUNCTION__, __LINE__, ret);

    shot_ext = (struct camera2_shot_ext *)curBuffer.addr[1];

    /* complete frame */
    ret = m_completeFrame(&curFrame, curBuffer);
    if (ret < 0) {
        CLOGE("ERR(%s):m_comleteFrame fail", __FUNCTION__);
        /* TODO: doing exception handling */
    }

    if (curFrame == NULL) {
        CLOGE("ERR(%s):curFrame is fail", __FUNCTION__);
    }

    m_outputFrameQ->pushProcessQ(&curFrame);

    return NO_ERROR;
}

status_t ExynosCameraPipe3AC::setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    /* TODO: check node state */

    /* set new sensorId to m_sensorIds */
    if (sensorIds) {
        CLOGD("DEBUG(%s[%d]):set new sensorIds", __FUNCTION__, __LINE__);

        for (int i = 0; i < MAX_NODE; i++)
            m_sensorIds[i] = sensorIds[i];
    }

    ret = m_setInput(m_node, m_nodeNum, m_sensorIds);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): m_setInput fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    if (pipeInfos) {
        ret = m_setPipeInfo(pipeInfos);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): m_setPipeInfo fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }
    }

    for (uint32_t i = 0; i < m_numBuffers; i++) {
        m_runningFrameList[i] = NULL;
    }
    m_numOfRunningFrame = 0;

    m_prepareBufferCount = m_getPrepareBufferCount();

    CLOGI("INFO(%s[%d]):setupPipe() is succeed (%d) prepare (%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);
    return NO_ERROR;
}

int ExynosCameraPipe3AC::m_getPrepareBufferCount(void)
{
    int prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];

    bool dynamicBayer = false;

    if (m_parameters->getRecordingHint() == true)
        dynamicBayer = m_parameters->getUseDynamicBayerVideoSnapShot();
    /* currently normal dynamic bayer is not qualified */
    /*
    else
        dynamicBayer = m_parameters->getUseDynamicBayer();
    */

    if (dynamicBayer == true) {
        CLOGD("DEBUG(%s[%d]): Dynamic bayer. so, set prepareBufferCount(%d) as 0",
            __FUNCTION__, __LINE__, prepareBufferCount);

        prepareBufferCount = 0;
    }

    return prepareBufferCount;
}

status_t ExynosCameraPipe3AC::sensorStream(bool on)
{
    CLOGD("[%s(%d)]", __FUNCTION__, __LINE__);

    int ret = 0;
    int value = on ? IS_ENABLE_STREAM: IS_DISABLE_STREAM;

    ret = m_node[CAPTURE_NODE]->setControl(V4L2_CID_IS_S_STREAM, value);
    if (ret != NO_ERROR)
        CLOGE("ERR(%s): sensor S_STREAM(%d) fail", __FUNCTION__, value);

    return ret;
}

bool ExynosCameraPipe3AC::m_checkThreadLoop(void)
{
    Mutex::Autolock lock(m_pipeframeLock);
    bool loop = false;

    if (m_isReprocessing() == false)
        loop = true;

    if (m_inputFrameQ->getSizeOfProcessQ() > 0)
        loop = true;

    /*
    if (m_inputFrameQ->getSizeOfProcessQ() == 0 &&
        m_numOfRunningFrame == 0)
        loop = false;
    */
    if (0 < m_numOfRunningFrame)
        loop = true;

    if (m_flagTryStop == true)
        loop = false;

    return loop;
}

bool ExynosCameraPipe3AC::m_mainThreadFunc(void)
{
    int ret = 0;

    if (m_flagStartPipe == false) {
        /* waiting for pipe started */
        usleep(5000);
        return m_checkThreadLoop();
    }

    if (m_numOfRunningFrame == 0 && m_inputFrameQ->getSizeOfProcessQ() != 0) {
        ret = prepare();
        if (ret < 0)
            CLOGE("ERR(%s[%d]):3AC prepare fail, ret(%d)", __FUNCTION__, __LINE__, ret);
    }

    ret = m_getBuffer();
    if (m_flagTryStop == true) {
        CLOGD("DEBUG(%s[%d]):m_flagTryStop(%d)", __FUNCTION__, __LINE__, m_flagTryStop);
        return false;
    }
    if (ret < 0) {
        CLOGE("ERR(%s): m_getBuffer fail", __FUNCTION__);
        /* TODO: doing exception handling */
    }

    if (m_numOfRunningFrame < m_prepareBufferCount - 1) {
        int cnt = m_inputFrameQ->getSizeOfProcessQ();
        do {
            ret = m_putBuffer();
            if (ret < 0) {
                if (ret == TIMED_OUT)
                    return m_checkThreadLoop();

                CLOGE("ERR(%s):m_putBuffer fail", __FUNCTION__);
                /* TODO: doing exception handling */
                return m_checkThreadLoop();
            }
            cnt--;
        } while (cnt > 0);
    } else {
        ret = m_putBuffer();
        if (ret < 0) {
            CLOGE("ERR(%s): m_putBuffer fail", __FUNCTION__);
            /* TODO: doing exception handling */
            return m_checkThreadLoop();
        }
    }

    return m_checkThreadLoop();
}

void ExynosCameraPipe3AC::m_init(void)
{
    m_metadataTypeShot = false;
}

}; /* namespace android */
