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
#define LOG_TAG "ExynosCameraPipeFlite"
#include <cutils/log.h>

#include "ExynosCameraPipeFlite.h"

namespace android {

/* global variable for multi-singleton */
Mutex             ExynosCameraPipeFlite::g_nodeInstanceMutex;
ExynosCameraNode *ExynosCameraPipeFlite::g_node[FLITE_CNTS] = {0};
int               ExynosCameraPipeFlite::g_nodeRefCount[FLITE_CNTS] = {0};
#ifdef SUPPORT_DEPTH_MAP
Mutex             ExynosCameraPipeFlite::g_vcNodeInstanceMutex;
ExynosCameraNode *ExynosCameraPipeFlite::g_vcNode[VC_CNTS] = {0};
int               ExynosCameraPipeFlite::g_vcNodeRefCount[VC_CNTS] = {0};
#endif

ExynosCameraPipeFlite::~ExynosCameraPipeFlite()
{
    this->destroy();
}

status_t ExynosCameraPipeFlite::create(int32_t *sensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;

    for (int i = 0; i < MAX_NODE; i++) {
        if (sensorIds) {
            CLOGV("DEBUG(%s[%d]):set new sensorIds[%d] : %d", __FUNCTION__, __LINE__, i, sensorIds[i]);
            m_sensorIds[i] = sensorIds[i];
        } else {
            m_sensorIds[i] = -1;
        }
    }

    /*
     * Flite must open once. so we will take cover as global variable.
     * we will use only m_createNode and m_destroyNode.
     */
    /*
    m_node[CAPTURE_NODE] = new ExynosCameraNode();
    ret = m_node[CAPTURE_NODE]->create("FLITE", m_cameraId);
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
    */
    m_node[CAPTURE_NODE] = m_createNode(m_cameraId, m_nodeNum[CAPTURE_NODE]);
#ifdef SUPPORT_DEPTH_MAP
    if (m_parameters->getUseDepthMap()) {
        m_node[CAPTURE_NODE_2] = m_createVcNode(m_cameraId, m_nodeNum[CAPTURE_NODE_2]);
    }
#endif

    /* mainNode is CAPTURE_NODE */
    m_mainNodeNum = CAPTURE_NODE;
    m_mainNode = m_node[m_mainNodeNum];

    /* setInput for 54xx */
    ret = m_setInput(m_node, m_nodeNum, m_sensorIds);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): m_setInput fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    m_mainThread = ExynosCameraThreadFactory::createThread(this, &ExynosCameraPipeFlite::m_mainThreadFunc, "fliteThread", PRIORITY_URGENT_DISPLAY);

    m_inputFrameQ = new frame_queue_t;

    m_prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];
    CLOGI("INFO(%s[%d]):create() is succeed (%d) prepare (%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    return NO_ERROR;
}

status_t ExynosCameraPipeFlite::destroy(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    if (m_node[CAPTURE_NODE] != NULL) {
        /*
        if (m_node[CAPTURE_NODE]->close() != NO_ERROR) {
            CLOGE("ERR(%s):close fail", __FUNCTION__);
            return INVALID_OPERATION;
        }
        delete m_node[CAPTURE_NODE];
        */
        m_destroyNode(m_cameraId, m_node[CAPTURE_NODE]);

        m_node[CAPTURE_NODE] = NULL;
        CLOGD("DEBUG(%s):Node(CAPTURE_NODE, m_nodeNum : %d, m_sensorIds : %d) closed",
            __FUNCTION__, m_nodeNum[CAPTURE_NODE], m_sensorIds[CAPTURE_NODE]);
    }

#ifdef SUPPORT_DEPTH_MAP
    if (m_node[CAPTURE_NODE_2] != NULL) {
        m_destroyVcNode(m_cameraId, m_node[CAPTURE_NODE_2]);

        m_node[CAPTURE_NODE_2] = NULL;
        CLOGD("DEBUG(%s):Node(CAPTURE_NODE_2, m_nodeNum : %d, m_sensorIds : %d) closed",
                __FUNCTION__, m_nodeNum[CAPTURE_NODE_2], m_sensorIds[CAPTURE_NODE_2]);
    }
#endif

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

#ifdef SUPPORT_DEPTH_MAP
status_t ExynosCameraPipeFlite::start(void)
{
    status_t ret = 0;

    ret = ExynosCameraPipe::start();

    if (m_parameters->getUseDepthMap()) {
        ret = m_node[CAPTURE_NODE_2]->start();
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Failed to start VCI node", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }
    }

    return NO_ERROR;
}

status_t ExynosCameraPipeFlite::stop(void)
{
    status_t ret = 0;

    ret = ExynosCameraPipe::stop();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):Failed to stopPipe", __FUNCTION__, __LINE__);
        return ret;
    }

    if (m_parameters->getUseDepthMap()) {
        ret = m_node[CAPTURE_NODE_2]->stop();
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Failed to stop VCI node", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }

        ret = m_node[CAPTURE_NODE_2]->clrBuffers();
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Failed to clrBuffers VCI node", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }

        m_node[CAPTURE_NODE_2]->removeItemBufferQ();
    }

    return NO_ERROR;
}
#endif

status_t ExynosCameraPipeFlite::m_getBuffer(void)
{
    ExynosCameraFrame *curFrame = NULL;
    ExynosCameraBuffer curBuffer;
    int index = -1;
    int ret = 0;
    struct camera2_shot_ext *shot_ext = NULL;

    if (m_numOfRunningFrame <= 0 || m_flagStartPipe == false) {
#if defined(USE_CAMERA2_API_SUPPORT)
        if (m_timeLogCount > 0)
#endif
            CLOGD("DEBUG(%s[%d]): skip getBuffer, flagStartPipe(%d), numOfRunningFrame = %d",
                __FUNCTION__, __LINE__, m_flagStartPipe, m_numOfRunningFrame);
        return NO_ERROR;
    }

    ret = m_node[CAPTURE_NODE]->getBuffer(&curBuffer, &index);
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

    ret = m_updateMetadataToFrame(curBuffer.addr[1], curBuffer.index);
    if (ret < 0)
        CLOGE("ERR(%s[%d]): updateMetadataToFrame fail, ret(%d)", __FUNCTION__, __LINE__, ret);

    shot_ext = (struct camera2_shot_ext *)curBuffer.addr[1];

//#ifdef SHOT_RECOVERY
    if (shot_ext != NULL) {
        retryGetBufferCount = shot_ext->complete_cnt;

        if (retryGetBufferCount > 0) {
            CLOGI("INFO(%s[%d]): ( %d %d %d %d )", __FUNCTION__, __LINE__,
                shot_ext->free_cnt,
                shot_ext->request_cnt,
                shot_ext->process_cnt,
                shot_ext->complete_cnt);
        }
    }
//#endif

#ifdef SUPPORT_DEPTH_MAP
    curFrame = m_runningFrameList[curBuffer.index];

    if (curFrame->getRequest(PIPE_VC1) == true) {
        ExynosCameraBuffer depthMapBuffer;

        ret = m_node[CAPTURE_NODE_2]->getBuffer(&depthMapBuffer, &index);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Failed to get DepthMap buffer", __FUNCTION__, __LINE__);
            return ret;
        }

        if (index < 0) {
            CLOGE("ERR(%s[%d]):Invalid index %d", __FUNCTION__, __LINE__, index);
            return INVALID_OPERATION;
        }

        ret = curFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_REQUESTED, CAPTURE_NODE_2);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Failed to setDstBufferState into REQUESTED. pipeId %d frameCount %d ret %d",
                    __FUNCTION__, __LINE__, getPipeId(), curFrame->getFrameCount(), ret);
        }

        ret = curFrame->setDstBuffer(getPipeId(), depthMapBuffer, CAPTURE_NODE_2, INDEX(getPipeId()));
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Failed to set DepthMapBuffer to frame. bufferIndex %d frameCount %d",
                    __FUNCTION__, __LINE__, index, curFrame->getFrameCount());
            return ret;
        }

        ret = curFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_COMPLETE, CAPTURE_NODE_2);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Failed to setDstBufferState with COMPLETE",
                    __FUNCTION__, __LINE__);
            return ret;
        }
    }
#endif

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

status_t ExynosCameraPipeFlite::m_setPipeInfo(camera_pipe_info_t *pipeInfos)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    status_t ret = NO_ERROR;

    if (pipeInfos == NULL) {
        CLOGE("ERR(%s[%d]): pipeInfos == NULL. so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    /* initialize node */
    ret = m_setNodeInfo(m_node[CAPTURE_NODE], &pipeInfos[0],
                    2, YUV_FULL_RANGE,
                    true);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]): m_setNodeInfo(%d, %d, %d) fail",
            __FUNCTION__, __LINE__, pipeInfos[0].rectInfo.fullW, pipeInfos[0].rectInfo.fullH, pipeInfos[0].bufInfo.count);
        return INVALID_OPERATION;
    }

#ifdef SUPPORT_DEPTH_MAP
    if (m_parameters->getUseDepthMap()) {
        ret = m_setNodeInfo(m_node[CAPTURE_NODE_2], &pipeInfos[CAPTURE_NODE_2],
                2, YUV_FULL_RANGE,
                true);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_setNodeInfo(%d, %d, %d) fail",
                    __FUNCTION__, __LINE__,
                    pipeInfos[CAPTURE_NODE_2].rectInfo.fullW, pipeInfos[CAPTURE_NODE_2].rectInfo.fullH,
                    pipeInfos[CAPTURE_NODE_2].bufInfo.count);
            return INVALID_OPERATION;
        }
    }
#endif

    m_numBuffers = pipeInfos[0].bufInfo.count;

    return NO_ERROR;
}

status_t ExynosCameraPipeFlite::setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

#ifdef DEBUG_RAWDUMP
    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
#endif

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

    m_prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];
    CLOGI("INFO(%s[%d]):setupPipe() is succeed (%d) setupPipe (%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    return NO_ERROR;
}

status_t ExynosCameraPipeFlite::sensorStream(bool on)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    int ret = 0;
    int value = on ? IS_ENABLE_STREAM: IS_DISABLE_STREAM;

    ret = m_node[CAPTURE_NODE]->setControl(V4L2_CID_IS_S_STREAM, value);
    if (ret != NO_ERROR)
        CLOGE("ERR(%s):sensor S_STREAM(%d) fail", __FUNCTION__, value);

    return ret;
}

bool ExynosCameraPipeFlite::m_mainThreadFunc(void)
{
    int ret = 0;

    if (m_flagTryStop == true) {
        usleep(5000);
        return true;
    }

    ret = m_getBuffer();
    if (m_flagTryStop == true) {
        CLOGD("DEBUG(%s[%d]):m_flagTryStop(%d)", __FUNCTION__, __LINE__, m_flagTryStop);
        return false;
    }
    if (ret < 0) {
        CLOGE("ERR(%s):m_getBuffer fail", __FUNCTION__);
        /* TODO: doing exception handling */
    }

//#ifdef SHOT_RECOVERY
    for (int i = 0; i < retryGetBufferCount; i ++) {
        CLOGE("INFO(%s[%d]): retryGetBufferCount( %d)", __FUNCTION__, __LINE__, retryGetBufferCount);
        ret = m_getBuffer();
        if (ret < 0) {
            CLOGE("ERR(%s):m_getBuffer fail", __FUNCTION__);
            /* TODO: doing exception handling */
        }

        ret = m_putBuffer();
        if (m_flagTryStop == true) {
            CLOGD("DEBUG(%s[%d]):m_flagTryStop(%d)", __FUNCTION__, __LINE__, m_flagTryStop);
            return false;
        }
        if (ret < 0) {
            if (ret == TIMED_OUT)
                return true;
            CLOGE("ERR(%s):m_putBuffer fail", __FUNCTION__);
            /* TODO: doing exception handling */
        }
    }
    retryGetBufferCount = 0;
//#endif

    if (m_numOfRunningFrame < 2) {
        int cnt = m_inputFrameQ->getSizeOfProcessQ();
        do {
            ret = m_putBuffer();
            if (m_flagTryStop == true) {
                CLOGD("DEBUG(%s[%d]):m_flagTryStop(%d)", __FUNCTION__, __LINE__, m_flagTryStop);
                return false;
            }
            if (ret < 0) {
                if (ret == TIMED_OUT)
                    return true;
                CLOGE("ERR(%s):m_putBuffer fail", __FUNCTION__);
                /* TODO: doing exception handling */
            }
            cnt--;
        } while (cnt > 0);
    } else {
        ret = m_putBuffer();
        if (m_flagTryStop == true) {
            CLOGD("DEBUG(%s[%d]):m_flagTryStop(%d)", __FUNCTION__, __LINE__, m_flagTryStop);
            return false;
        }
        if (ret < 0) {
            if (ret == TIMED_OUT)
                return true;
            CLOGE("ERR(%s):m_putBuffer fail", __FUNCTION__);
            /* TODO: doing exception handling */
        }
     }

    return true;
}

void ExynosCameraPipeFlite::m_init(void)
{
//#ifdef SHOT_RECOVERY
    retryGetBufferCount = 0;
//#endif
}

status_t ExynosCameraPipeFlite::m_updateMetadataToFrame(void *metadata, int index)
{
    status_t ret = NO_ERROR;
    ExynosCameraFrame *frame = NULL;
    camera2_shot_ext *shot_ext = NULL;
    camera2_dm frame_dm;

    shot_ext = (struct camera2_shot_ext *) metadata;
    if (shot_ext == NULL) {
        CLOGE("ERR(%s[%d]):shot_ext is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (index < 0) {
        CLOGE("ERR(%s[%d]):Invalid index %d", __FUNCTION__, __LINE__, index);
        return BAD_VALUE;
    }

    if (m_metadataTypeShot == false) {
        CLOGV("DEBUG(%s[%d]):Stream type do not need update metadata", __FUNCTION__, __LINE__);
        return NO_ERROR;
    }

    ret = m_getFrameByIndex(&frame, index);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):Failed to getFrameByIndex. index %d ret %d",
                __FUNCTION__, __LINE__, index, ret);
        return ret;
    }

    ret = frame->getDynamicMeta(&frame_dm);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):Failed to getDynamicMeta. frameCount %d ret %d",
                __FUNCTION__, __LINE__, frame->getFrameCount(), ret);
        return ret;
    }

    if (frame_dm.request.frameCount == 0 || frame_dm.sensor.timeStamp == 0) {
        frame_dm.request.frameCount = shot_ext->shot.dm.request.frameCount;
        frame_dm.sensor.timeStamp = shot_ext->shot.dm.sensor.timeStamp;

        ret = frame->storeDynamicMeta(&frame_dm);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):Failed to storeDynamicMeta. frameCount %d ret %d",
                    __FUNCTION__, __LINE__, frame->getFrameCount(), ret);
            return ret;
        }
    }

    return NO_ERROR;
}

}; /* namespace android */
