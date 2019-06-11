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
#define LOG_TAG "ExynosCameraPipeSCP"
#include <cutils/log.h>

#include "ExynosCameraPipeSCP.h"

namespace android {

#ifdef TEST_WATCHDOG_THREAD
int testErrorDetect = 0;
#endif

ExynosCameraPipeSCP::~ExynosCameraPipeSCP()
{
    this->destroy();
#ifdef TEST_WATCHDOG_THREAD
    testErrorDetect = 0;
#endif
}

status_t ExynosCameraPipeSCP::create(int32_t *sensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
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
    ret = m_node[CAPTURE_NODE]->create("SCP", m_cameraId);
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

    m_mainThread = ExynosCameraThreadFactory::createThread(this, &ExynosCameraPipeSCP::m_mainThreadFunc, "SCPThread", PRIORITY_URGENT_DISPLAY);

    m_inputFrameQ = new frame_queue_t;

    m_prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];
    CLOGI("INFO(%s[%d]):create() is succeed (%d) setupPipe (%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    return NO_ERROR;
}

status_t ExynosCameraPipeSCP::destroy(void)
{
    ExynosCameraBuffer *dqBuffer = NULL;
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (m_node[CAPTURE_NODE] != NULL) {
        if (m_node[CAPTURE_NODE]->close() != NO_ERROR) {
            CLOGE("ERR(%s):close fail", __FUNCTION__);
            return INVALID_OPERATION;
        }
        delete m_node[CAPTURE_NODE];
        m_node[CAPTURE_NODE] = NULL;
        CLOGD("DEBUG(%s):Node(CAPTURE_NODE, m_nodeNum : %d, m_sensorIds : %d) closed",
            __FUNCTION__, m_nodeNum[CAPTURE_NODE], m_sensorIds[CAPTURE_NODE]);
    }

    if (m_inputFrameQ != NULL) {
        m_inputFrameQ->release();
        delete m_inputFrameQ;
        m_inputFrameQ = NULL;
    }

    m_mainNode = NULL;
    m_mainNodeNum = -1;

    m_requestFrameQ.release();
    m_skipFrameQ.release();

    prevDqBufferValid = false;

    CLOGI("INFO(%s[%d]):destroy() is succeed (%d)", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipeSCP::m_setPipeInfo(camera_pipe_info_t *pipeInfos)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    status_t ret = NO_ERROR;

    if (pipeInfos == NULL) {
        CLOGE("ERR(%s[%d]): pipeInfos == NULL. so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    int setfile = 0;
    int yuvRange = 0;
    m_parameters->getSetfileYuvRange(m_reprocessing, &setfile, &yuvRange);

    /* initialize node */
    ret = m_setNodeInfo(m_node[CAPTURE_NODE], &pipeInfos[0],
                    4, (enum YUV_RANGE)yuvRange,
                    true);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]): m_setNodeInfo(%d, %d, %d) fail",
            __FUNCTION__, __LINE__, pipeInfos[0].rectInfo.fullW, pipeInfos[0].rectInfo.fullH, pipeInfos[0].bufInfo.count);
        return INVALID_OPERATION;
    }

    m_numBuffers = pipeInfos[0].bufInfo.count;

    return NO_ERROR;
}

status_t ExynosCameraPipeSCP::setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds)
{
    ExynosCameraBuffer *dqBuffer = NULL;
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;
    /* TODO: check node state stream on? */

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

    /* setfile setting */
#ifdef SET_SETFILE_BY_SHOT
    /* nop */
#else
#if SET_SETFILE_BY_SET_CTRL_SCP
    int setfile = 0;
    int yuvRange = 0;
    m_parameters->getSetfileYuvRange(m_reprocessing, &setfile, &yuvRange);

    /* colorRange set by setFormat */
    /*
    ret = m_node[CAPTURE_NODE]->setControl(V4L2_CID_IS_COLOR_RANGE, yuvRange);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):setControl(%d) fail(ret = %d)", __FUNCTION__, __LINE__, setfile, ret);
        return ret;
    }
    */
#endif
#endif

    for (uint32_t i = 0; i < m_numBuffers; i++) {
        m_runningFrameList[i] = NULL;
    }
    m_numOfRunningFrame = 0;

    m_requestFrameQ.release();
    m_skipFrameQ.release();

    prevDqBufferValid = false;

    m_prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];
    CLOGI("INFO(%s[%d]):setupPipe() is succeed (%d) setupPipe (%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    return NO_ERROR;
}

status_t ExynosCameraPipeSCP::m_checkPolling(void)
{
    int ret = 0;

    ret = m_node[CAPTURE_NODE]->polling();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):polling fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */

        m_threadState = ERROR_POLLING_DETECTED;
        return ERROR_POLLING_DETECTED;
    }

    return NO_ERROR;
}

bool ExynosCameraPipeSCP::m_mainThreadFunc(void)
{
    int ret = 0;

#ifdef TEST_WATCHDOG_THREAD
    testErrorDetect++;
    if (testErrorDetect == 100)
        m_threadState = ERROR_POLLING_DETECTED;
#endif

    if  (m_flagTryStop == true)
        return true;

    if (m_numOfRunningFrame > 0) {

#ifndef SKIP_SCHECK_POLLING
        if (!prevDqBufferValid)
            ret = m_checkPolling();
#endif
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):m_checkPolling fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            // HACK: for panorama shot
            //return false;
        }

        ret = m_getBuffer();
        if (ret < 0) {
            CLOGE("ERR(%s):m_getBuffer fail", __FUNCTION__);
            /* TODO: doing exception handling */
            return true;
        }
    }

    m_timer.stop();
    m_timeInterval = m_timer.durationMsecs();
    m_timer.start();

    ret = m_putBuffer();
    if (ret < 0) {
        if (ret == TIMED_OUT)
            return true;
        CLOGE("ERR(%s):m_putBuffer fail", __FUNCTION__);
        /* TODO: doing exception handling */
        return true;
    }

    /* update renew count */
    if (ret >= 0)
        m_threadRenew = 0;

    return true;
}

status_t ExynosCameraPipeSCP::m_putBuffer(void)
{
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer newBuffer;
    ExynosCameraFrame *skipFrame = NULL;
    int ret = 0;

retry:
    newFrame = NULL;
    ret = m_inputFrameQ->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW("WARN(%s):wait timeout, m_numOfRunningFrame: %d, m_requestFrameQSize: %d",
                __FUNCTION__, m_numOfRunningFrame, m_requestFrameQ.getSizeOfProcessQ());
            m_node[CAPTURE_NODE]->dumpState();
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

    if (m_skipFrameQ.getSizeOfProcessQ()) {
        CLOGW("WARN(%s[%d]): DROP_SCP return skip buffer", __FUNCTION__, __LINE__);
        skipFrame = NULL;
        m_skipFrameQ.popProcessQ(&skipFrame);
        if (skipFrame == NULL) {
            CLOGE("ERR(%s):skipFrame is NULL", __FUNCTION__);
            goto retry;
        }

        ret = newFrame->getDstBuffer(getPipeId(), &newBuffer);
        if (ret != NO_ERROR) {
            CLOGW("WRN(%s[%d]):Get destination buffer fail", __FUNCTION__, __LINE__);
        }

        ret = skipFrame->setDstBuffer(getPipeId(), newBuffer);
        if (ret != NO_ERROR) {
            CLOGW("WRN(%s[%d]):Set destination buffer fail", __FUNCTION__, __LINE__);
        }

        m_requestFrameQ.pushProcessQ(&newFrame);

        ret = skipFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
        if (ret < 0) {
            CLOGW("WRN(%s[%d]): setEntity state fail", __FUNCTION__, __LINE__);
        }

        ret = skipFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_ERROR);
        if (ret < 0) {
            CLOGE("ERR(%s): setDstBuffer state fail", __FUNCTION__);
            goto retry;
        }

        m_outputFrameQ->pushProcessQ(&skipFrame);
        if (m_inputFrameQ->getSizeOfProcessQ() > 0)
            goto retry;
        else
            return NO_ERROR;
    }

    ret = newFrame->getDstBuffer(getPipeId(), &newBuffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):frame get buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return OK;
    }

    /* check buffer index */
    if (newBuffer.index < 0) {
        CLOGD("DEBUG(%s[%d]): no buffer to QBUF (%d)", __FUNCTION__, __LINE__, newFrame->getFrameCount());

        ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_REQUESTED);
        if (ret < 0) {
            CLOGE("ERR(%s): setDstBuffer state fail", __FUNCTION__);
            return ret;
        }

        ret = newFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):set entity state fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            return OK;
        }

        CLOGV("DEBUG(%s):entity pipeId(%d), frameCount(%d), numOfRunningFrame(%d)",
                __FUNCTION__, getPipeId(), newFrame->getFrameCount(), m_numOfRunningFrame);

        usleep(33000);
        m_outputFrameQ->pushProcessQ(&newFrame);

        goto retry;
    } else {

        camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(newBuffer.addr[2]);

        if (shot_ext != NULL) {
            newFrame->getMetaData(shot_ext);
            m_parameters->duplicateCtrlMetadata((void *)shot_ext);
            m_activityControl->activityBeforeExecFunc(getPipeId(), (void *)&newBuffer);

            if (m_perframeMainNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType == PERFRAME_NODE_TYPE_LEADER) {
                camera2_node_group node_group_info;
                memset(&shot_ext->node_group, 0x0, sizeof(camera2_node_group));
                newFrame->getNodeGroupInfo(&node_group_info, m_perframeMainNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex);

                /* Per - Leader */
                if (node_group_info.leader.request == 1) {

                    if (m_checkNodeGroupInfo(-1, &m_curNodeGroupInfo.leader, &node_group_info.leader) != NO_ERROR)
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
                    setMetaNodeLeaderRequest(shot_ext,
                        node_group_info.leader.request);
                    setMetaNodeLeaderVideoID(shot_ext,
                        m_perframeMainNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID);
                }

                /* Per - Captures */
                for (int i = 0; i < m_perframeMainNodeGroupInfo.perframeSupportNodeNum - 1; i ++) {
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
                        setMetaNodeCaptureVideoID(shot_ext, i, m_perframeMainNodeGroupInfo.perFrameCaptureInfo[i].perFrameVideoID);
                    }
                }
            }
        }

        ret = m_node[CAPTURE_NODE]->putBuffer(&newBuffer);
        if (ret < 0) {
            CLOGE("ERR(%s):putBuffer fail", __FUNCTION__);
            return ret;
            /* TODO: doing exception handling */
        }
        m_numOfRunningFrame++;

        ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_PROCESSING);
        if (ret < 0) {
            CLOGE("ERR(%s): setDstBuffer state fail", __FUNCTION__);
            return ret;
        }

        if (newFrame->getScpDrop()) {
            CLOGW("WARN(%s): DROP_SCP Queue to skipFrameQ", __FUNCTION__);
            m_skipFrameQ.pushProcessQ(&newFrame);
        } else {
            m_requestFrameQ.pushProcessQ(&newFrame);
        }
    }

    return NO_ERROR;
}

status_t ExynosCameraPipeSCP::m_getBuffer(void)
{
    ExynosCameraFrame *curFrame = NULL;
    ExynosCameraBuffer curBuffer;
    int index = -1;
    int ret = 0;
    int requestFrameQSize;
    bool isFrameDropped = false;

    requestFrameQSize = m_requestFrameQ.getSizeOfProcessQ();
    if (m_numOfRunningFrame <= 0 || m_flagStartPipe == false || requestFrameQSize == 0) {
        CLOGD("DEBUG(%s[%d]): skip getBuffer, flagStartPipe(%d), numOfRunningFrame = %d", __FUNCTION__, __LINE__, m_flagStartPipe, m_numOfRunningFrame);
        return NO_ERROR;
    }

    if (prevDqBufferValid) {
        curBuffer = prevDqBuffer;
        prevDqBufferValid = false;
    } else {
        ret = m_node[CAPTURE_NODE]->getBuffer(&curBuffer, &index);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):getBuffer fail", __FUNCTION__, __LINE__);
            /* TODO: doing exception handling */
            return ret;
        }
    }
    m_numOfRunningFrame--;

    m_activityControl->activityAfterExecFunc(getPipeId(), (void *)&curBuffer);

    do {
        curFrame = NULL;
        isFrameDropped = false;
        ret = m_requestFrameQ.popProcessQ(&curFrame);
        if (ret == TIMED_OUT || curFrame == NULL) {
            CLOGW("WARN(%s[%d]): requestFrame is NULL", __FUNCTION__, __LINE__);

            /* preserve the dequeued buffer for next iteration */
            prevDqBuffer = curBuffer;
            prevDqBufferValid = true;
            m_numOfRunningFrame++;
            return NO_ERROR;
        }

        /* check whether this frame is dropped */
        isFrameDropped = curFrame->getScpDrop();
        if (isFrameDropped == true)
            m_skipFrameQ.pushProcessQ(&curFrame);

    } while (isFrameDropped);

    ret = curFrame->setDstBuffer(getPipeId(), curBuffer);
    if (ret < 0)
        CLOGE("ERR(%s[%d]):set Dst buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);

    ret = curFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):set entity state fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return OK;
    }

    CLOGV("DEBUG(%s):entity pipeId(%d), frameCount(%d)",
            __FUNCTION__, getPipeId(), curFrame->getFrameCount());

    ret = curFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_COMPLETE);
    if (ret < 0) {
        CLOGE("ERR(%s): setDstBuffer state fail", __FUNCTION__);
        return ret;
    }

    m_outputFrameQ->pushProcessQ(&curFrame);
    return NO_ERROR;
}

void ExynosCameraPipeSCP::m_init(void)
{
    m_metadataTypeShot = false;
    prevDqBufferValid = false;
}

}; /* namespace android */
