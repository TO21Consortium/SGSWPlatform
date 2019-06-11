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
#define LOG_TAG "ExynosCameraPipeDIS"
#include <cutils/log.h>

#include "ExynosCameraPipeDIS.h"

namespace android {

ExynosCameraPipeDIS::~ExynosCameraPipeDIS()
{
    this->destroy();
}

status_t ExynosCameraPipeDIS::create(int32_t *sensorIds)
{
    CLOGI("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;
    int fd = -1;

    for (int i = 0; i < MAX_NODE; i++) {
        if (sensorIds) {
            CLOGD("DEBUG(%s[%d]):set new sensorIds[%d] : %d", __FUNCTION__, __LINE__, i, sensorIds[i]);
            m_sensorIds[i] = sensorIds[i];
        } else {
            m_sensorIds[i] = -1;
        }
    }

    /* DIS output */
    if (m_flagValidInt(m_nodeNum[OUTPUT_NODE]) == true) {
        m_node[OUTPUT_NODE] = new ExynosCameraNode();
        ret = m_node[OUTPUT_NODE]->create("DIS_OUTPUT", m_cameraId);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): OUTPUT_NODE create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }

        ret = m_node[OUTPUT_NODE]->open(m_nodeNum[OUTPUT_NODE]);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): OUTPUT_NODE open fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }
        CLOGD("DEBUG(%s):Node(%d) opened", __FUNCTION__, m_nodeNum[OUTPUT_NODE]);
    }

    /* DIS capture */
    if (m_flagValidInt(m_nodeNum[CAPTURE_NODE]) == true) {
        m_node[CAPTURE_NODE] = new ExynosCameraNode();

        ret = m_node[CAPTURE_NODE]->create("DIS_CAPTURE", m_cameraId);
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
    }

    /* mainNode is OUTPUT_NODE */
    m_mainNodeNum = OUTPUT_NODE;
    m_mainNode = m_node[m_mainNodeNum];

    /* setInput for 54xx */
    ret = m_setInput(m_node, m_nodeNum, m_sensorIds);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): m_setInput fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    m_mainThread = ExynosCameraThreadFactory::createThread(this, &ExynosCameraPipeDIS::m_mainThreadFunc, "DISThread");

    m_inputFrameQ = new frame_queue_t(m_mainThread);

    m_inputFrameQ->setWaitTime(500000000); /* .5 sec */

    m_prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];

    CLOGI("INFO(%s[%d]):create() is succeed (%d) prepare (%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    return NO_ERROR;
}

status_t ExynosCameraPipeDIS::destroy(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    int ret = 0;

    for (int i = MAX_NODE - 1; 0 <= i; i--) {
        if (m_node[i] != NULL) {
            if (m_node[i]->close() != NO_ERROR) {
                CLOGE("ERR(%s): close(%d) fail", __FUNCTION__, i);
                return INVALID_OPERATION;
            }
            delete m_node[i];
            m_node[i] = NULL;
            CLOGD("DEBUG(%s):Node(%d, m_nodeNum : %d, m_sensorIds : %d) closed",
                __FUNCTION__, i, m_nodeNum[i], m_sensorIds[i]);
        }
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

status_t ExynosCameraPipeDIS::m_setPipeInfo(camera_pipe_info_t *pipeInfos)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (pipeInfos == NULL) {
        CLOGE("ERR(%s[%d]): pipeInfos == NULL. so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    status_t ret = NO_ERROR;
    unsigned int bpp = 0;
    unsigned int planes = 0;

    if (m_node[OUTPUT_NODE] != NULL) {
        int disFormat = m_parameters->getHWVdisFormat();
        unsigned int disPlanes = 0;
        getYuvFormatInfo(disFormat, &bpp, &disPlanes);

        getYuvFormatInfo(pipeInfos[0].rectInfo.colorFormat, &bpp, &planes);

        if (planes != disPlanes) {
            CLOGE("ERR(%s[%d]):planes(%d) of colorFormat(%d) != disPlanes(%d) of disFormat(%d). so. fail(please check colorFormat scenario)",
                __FUNCTION__, __LINE__, planes, pipeInfos[0].rectInfo.colorFormat, disPlanes, disFormat);
            return INVALID_OPERATION;
        }

        /* add meta */
        planes += 1;

        ret = m_setNodeInfo(m_node[OUTPUT_NODE], &pipeInfos[0],
                        planes, YUV_FULL_RANGE,
                        true);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]): m_setNodeInfo(%d, %d, %d) fail",
                __FUNCTION__, __LINE__, pipeInfos[0].rectInfo.fullW, pipeInfos[0].rectInfo.fullH, pipeInfos[0].bufInfo.count);
            return INVALID_OPERATION;
        }

        m_numBuffers = pipeInfos[0].bufInfo.count;
        m_perframeMainNodeGroupInfo = pipeInfos[0].perFrameNodeGroupInfo;
    }

    if (m_node[CAPTURE_NODE] != NULL &&
        m_isOtf(m_sensorIds[CAPTURE_NODE]) == false) {

        int previewFormat = m_parameters->getHwPreviewFormat();
        unsigned int previewPlanes = 0;
        getYuvFormatInfo(previewFormat, &bpp, &previewPlanes);

        getYuvFormatInfo(pipeInfos[1].rectInfo.colorFormat, &bpp, &planes);

        if (planes != previewPlanes) {
            CLOGE("ERR(%s[%d]):planes(%d) of colorFormat(%d) != previewPlanes(%d) of previewFormat(%d). so. fail(please check colorFormat scenario)",
                __FUNCTION__, __LINE__, planes, pipeInfos[1].rectInfo.colorFormat, previewPlanes, previewFormat);
            return INVALID_OPERATION;
        }

        /* add meta */
        planes += 1;

        ret = m_setNodeInfo(m_node[CAPTURE_NODE], &pipeInfos[1],
                            planes, YUV_FULL_RANGE,
                            true);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]): m_setNodeInfo(%d, %d, %d) fail",
                __FUNCTION__, __LINE__, pipeInfos[1].rectInfo.fullW, pipeInfos[1].rectInfo.fullH, pipeInfos[1].bufInfo.count);
            return INVALID_OPERATION;
        }

        m_numCaptureBuf = pipeInfos[1].bufInfo.count;
    }

    return NO_ERROR;
}

status_t ExynosCameraPipeDIS::setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds)
{
    CLOGI("INFO(%s[%d]): -IN-", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

#ifdef DEBUG_RAWDUMP
    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
#endif

    /* TODO: check node state */
    /*       stream on? */

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

    for(int i = 0; i < MAX_NODE; i++) {
        for (int j = 0; j < m_numBuffers; j++) {
            m_runningFrameList[j] = NULL;
            m_nodeRunningFrameList[i][j] = NULL;
        }
    }

    m_numOfRunningFrame = 0;

    m_prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];

    CLOGI("INFO(%s[%d]):setupPipe() is succeed (%d) prepare (%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    CLOGI("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraPipeDIS::prepare(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraPipeDIS::start(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    ret = m_startNode();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]): m_startNode() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    m_flagStartPipe = true;
    m_flagTryStop = false;

    return ret;
}

status_t ExynosCameraPipeDIS::stop(void)
{
    CLOGD("INFO(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    ret = m_stopNode();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]): m_stopNode() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    m_mainThread->requestExitAndWait();

    CLOGD("DEBUG(%s[%d]): thead exited", __FUNCTION__, __LINE__);

    m_inputFrameQ->release();

    m_flagStartPipe = false;
    m_flagTryStop = false;

    return ret;
}

status_t ExynosCameraPipeDIS::getPipeInfo(int *fullW, int *fullH, int *colorFormat, int pipePosition)
{
    int planeCount = 0;
    status_t ret = NO_ERROR;

    enum NODE_TYPE nodeType = OUTPUT_NODE;

    if (pipePosition == SRC_PIPE) {
        nodeType = CAPTURE_NODE;
    } else {
        nodeType = OUTPUT_NODE;
    }

    if (m_node[nodeType] == NULL) {
        CLOGE("ERR(%s): m_node[%d] == NULL. so, fail", __FUNCTION__, nodeType);
        return INVALID_OPERATION;
    }

    ret = m_node[nodeType]->getSize(fullW, fullH);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s):m_node[%d]->getSize fail", __FUNCTION__, nodeType);
        return ret;
    }

    ret = m_node[nodeType]->getColorFormat(colorFormat, &planeCount);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s):m_node[%d]->getColorFormat fail", __FUNCTION__, nodeType);
        return ret;
    }

    return ret;
}

status_t ExynosCameraPipeDIS::m_putBuffer(void)
{
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer buffer[MAX_NODE];
    camera2_shot_ext *shot_ext = NULL;
    int ret = 0;
    int runningFrameIndex = -1;

    int v4l2Colorformat = 0;
    int planeCount[MAX_NODE] = {0};
    int metaBufferPlaneCount = 0;
    bool flagOutputNodeRunning = false;

    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    ret = m_inputFrameQ->waitAndPopProcessQ(&newFrame);
    if (m_flagTryStop == true) {
        CLOGD("DEBUG(%s):m_flagTryStop(%d)", __FUNCTION__, m_flagTryStop);
        return false;
    }
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW("WARN(%s):wait timeout", __FUNCTION__);

            for (int i = 0; i < MAX_NODE; i++) {
                if (m_node[i] != NULL)
                    m_node[i]->dumpState();
            }
        } else {
            CLOGE("ERR(%s):wait and pop fail, ret(%d)", __FUNCTION__, ret);
            /* TODO: doing exception handling */
        }
        return ret;
    }

    if (newFrame == NULL) {
        CLOGE("ERR(%s[%d]):new frame is NULL", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    if(m_node[OUTPUT_NODE] != NULL &&
        m_isOtf(m_node[OUTPUT_NODE]->getInput()) == false) {
        ret = newFrame->getSrcBuffer(getPipeId(), &buffer[OUTPUT_NODE]);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):frame get src buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            return OK;
        }

        if (m_nodeRunningFrameList[OUTPUT_NODE][buffer[OUTPUT_NODE].index] != NULL) {
            CLOGE("ERR(%s[%d]):OUTPUT_NODE new buffer is invalid, we already get buffer index(%d)",
                __FUNCTION__, __LINE__, buffer[OUTPUT_NODE].index);
            return BAD_VALUE;
        }

        if (runningFrameIndex < 0)
            runningFrameIndex = buffer[OUTPUT_NODE].index;

        m_node[OUTPUT_NODE]->getColorFormat(&v4l2Colorformat, &planeCount[OUTPUT_NODE]);

        metaBufferPlaneCount = planeCount[OUTPUT_NODE] - 1;

        shot_ext = (struct camera2_shot_ext *)(buffer[OUTPUT_NODE].addr[metaBufferPlaneCount]);

        flagOutputNodeRunning = true;

        /* DROP CASE */
        if (newFrame->getDisDrop() == true) {
            int frameCount = -1;

            if (shot_ext != NULL)
                frameCount = getMetaDmRequestFrameCount(shot_ext);

            CLOGW("WARN(%s[%d]):newFrame->getDisDrop() == true, (qbuf is too late) so, dis drop frameCount(%d)",
                __FUNCTION__, __LINE__, frameCount);

            return m_handleInvalidFrame(buffer[OUTPUT_NODE].index, newFrame, &buffer[OUTPUT_NODE]);
        }
    }

    if (m_node[CAPTURE_NODE] != NULL &&
        m_isOtf(m_node[CAPTURE_NODE]->getInput()) == false) {
        ret = newFrame->getDstBuffer(getPipeId(), &buffer[CAPTURE_NODE]);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):frame get dst buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            return OK;
        }

        if (m_nodeRunningFrameList[CAPTURE_NODE][buffer[CAPTURE_NODE].index] != NULL) {
            CLOGE("ERR(%s[%d]):CAPTURE_NODE new buffer is invalid, we already get buffer index(%d)",
                __FUNCTION__, __LINE__, buffer[CAPTURE_NODE].index);
            return BAD_VALUE;
        }

        if (runningFrameIndex < 0)
            runningFrameIndex = buffer[CAPTURE_NODE].index;
    }

    /* hack for debugging */
    CLOGV("DEBUG(%s[%d]): dis is running (%d)", __FUNCTION__, __LINE__, buffer[OUTPUT_NODE].index);

    if (shot_ext == NULL) {
        if (flagOutputNodeRunning == true)
            CLOGW("WARN(%s[%d]):shot_ext == NULL. but, skip", __FUNCTION__, __LINE__);
    } else {
        if (newFrame->getMetaDataEnable() == false) {
            CLOGE("ERR(%s[%d]):newFrame->getMetaDataEnable() == false. so, fail", __FUNCTION__, __LINE__);

            return m_handleInvalidFrame(buffer[OUTPUT_NODE].index, newFrame, &buffer[OUTPUT_NODE]);
        }

        newFrame->getMetaData(shot_ext);

        ret = m_parameters->duplicateCtrlMetadata((void *)shot_ext);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):duplicate Ctrl metadata fail", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }

        if (m_perframeMainNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType == PERFRAME_NODE_TYPE_LEADER) {
            int zoomParamInfo = m_parameters->getZoomLevel();
            int zoomFrameInfo = 0;
            camera2_node_group node_group_info;

            newFrame->getNodeGroupInfo(&node_group_info, PERFRAME_INFO_DIS, &zoomFrameInfo);

            if (node_group_info.leader.request == 1) {

                if (m_checkNodeGroupInfo(-1, &m_curNodeGroupInfo.leader, &node_group_info.leader) != NO_ERROR)
                    CLOGW("WARN(%s[%d]): m_checkNodeGroupInfo(leader) fail", __FUNCTION__, __LINE__);

                /* DIS src size is same with ISP dst size. DIS dst, too. */
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

            for (int i = 0; i < m_perframeMainNodeGroupInfo.perframeSupportNodeNum; i ++) {
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
                    setMetaNodeCaptureRequest(shot_ext, i, node_group_info.capture[i].request);
                    setMetaNodeCaptureVideoID(shot_ext, i, m_perframeMainNodeGroupInfo.perFrameCaptureInfo[i].perFrameVideoID);
                }
            }
            /* CLOGI("INFO(%s[%d]):fcount(%d)", __FUNCTION__, __LINE__, shot_ext_dst->shot.dm.request.frameCount); */
            /* newFrame->dumpNodeGroupInfo("DIS"); */
            /* m_dumpPerframeNodeGroupInfo(m_name, m_perframeMainNodeGroupInfo); */
        }
    }

    if (m_node[CAPTURE_NODE] != NULL &&
        m_isOtf(m_node[CAPTURE_NODE]->getInput()) == false) {
        if (m_node[CAPTURE_NODE]->putBuffer(&buffer[CAPTURE_NODE]) != NO_ERROR) {
            CLOGE("ERR(%s):capture putBuffer fail ret(%d)", __FUNCTION__, ret);
            /* TODO: doing exception handling */
            return ret;
        }

        ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_PROCESSING);
        if (ret < 0) {
            CLOGE("ERR(%s): setDstBuffer state fail", __FUNCTION__);
            return ret;
        }

        m_nodeRunningFrameList[CAPTURE_NODE][buffer[CAPTURE_NODE].index] = newFrame;
    }

    if (m_node[OUTPUT_NODE] != NULL &&
        m_isOtf(m_node[OUTPUT_NODE]->getInput()) == false) {

        /* check reversed frameCount (on error situation */
        if (shot_ext != NULL) {
            if (m_checkValidFrameCount(shot_ext) == false) {
                CLOGE("ERR(%s[%d]):m_checkValidFrameCount() fail", __FUNCTION__, __LINE__);
                return INVALID_OPERATION;
            }
        }

        if (m_node[OUTPUT_NODE]->putBuffer(&buffer[OUTPUT_NODE]) != NO_ERROR) {
            CLOGE("ERR(%s):output putBuffer fail ret(%d)", __FUNCTION__, ret);
            /* TODO: doing exception handling */
            return ret;
        }

        ret = newFrame->setSrcBufferState(getPipeId(), ENTITY_BUFFER_STATE_PROCESSING);
        if (ret < 0) {
            CLOGE("ERR(%s): setSrcBuffer state fail", __FUNCTION__);
            return ret;
        }

        m_nodeRunningFrameList[OUTPUT_NODE][buffer[OUTPUT_NODE].index] = newFrame;
    }

    /*
     * setting m_runningFrameList set on head of function.
     * OUTPUT_NODE has more priority than CAPTURE_NODE.
     */
    if (0 <= runningFrameIndex) {
        m_runningFrameList[runningFrameIndex] = newFrame;
        m_numOfRunningFrame++;
    } else {
        CLOGE("ERR(%s[%d]):runningFrameIndex(%d) is weird. so, fail", __FUNCTION__, __LINE__, runningFrameIndex);
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t ExynosCameraPipeDIS::m_getBuffer(void)
{
    ExynosCameraFrame *curFrame = NULL;
    ExynosCameraBuffer buffer[MAX_NODE];
    ExynosCameraBuffer metaBuffer;
    int v4l2Colorformat = 0;
    int planeCount[MAX_NODE] = {0};
    int metaBufferPlaneCount = 0;
    bool flagOutputNodeRunning = false;
    camera2_shot_ext *shot_ext = NULL;

    int index = 0;
    status_t ret = NO_ERROR;
    status_t error = NO_ERROR;

    CLOGV("DEBUG(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    if (m_numOfRunningFrame <= 0 || m_flagStartPipe == false) {
        CLOGD("DEBUG(%s[%d]): skip getBuffer, flagStartPipe(%d), numOfRunningFrame = %d", __FUNCTION__, __LINE__, m_flagStartPipe, m_numOfRunningFrame);
        return NO_ERROR;
    }

    if (m_node[CAPTURE_NODE] != NULL &&
        m_isOtf(m_node[CAPTURE_NODE]->getInput()) == false) {

        ret = m_node[CAPTURE_NODE]->getBuffer(&buffer[CAPTURE_NODE], &index);

        if (m_flagTryStop == true) {
            CLOGD("DEBUG(%s[%d]):m_flagTryStop(%d)", __FUNCTION__, __LINE__, m_flagTryStop);
            return false;
        }
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_node[CAPTURE_NODE]->getBuffer fail ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            error = ret;
        }

        CLOGV("DEBUG(%s[%d]):index : %d", __FUNCTION__, __LINE__, index);

        if (0 <= buffer[CAPTURE_NODE].index)
            m_nodeRunningFrameList[CAPTURE_NODE][buffer[CAPTURE_NODE].index] = NULL;

        metaBuffer = buffer[CAPTURE_NODE];
    }

    if (m_node[OUTPUT_NODE] != NULL &&
        m_isOtf(m_node[OUTPUT_NODE]->getInput()) == false) {

        ret = m_node[OUTPUT_NODE]->getBuffer(&buffer[OUTPUT_NODE], &index);

        if (m_flagTryStop == true) {
            CLOGD("DEBUG(%s[%d]):getBuffer out, ret(%d)", __FUNCTION__, __LINE__, ret);
            return false;
        }

        if (0 <= buffer[OUTPUT_NODE].index)
            m_nodeRunningFrameList[OUTPUT_NODE][buffer[OUTPUT_NODE].index] = NULL;

        metaBuffer = buffer[OUTPUT_NODE];

        m_node[OUTPUT_NODE]->getColorFormat(&v4l2Colorformat, &planeCount[OUTPUT_NODE]);
        metaBufferPlaneCount = planeCount[OUTPUT_NODE] - 1;

        shot_ext = (struct camera2_shot_ext *)(metaBuffer.addr[metaBufferPlaneCount]);

        flagOutputNodeRunning = true;
    }

    if ((ret != NO_ERROR || error != NO_ERROR || index < 0) &&
        (shot_ext != NULL)) {
        CLOGE("ERR(%s[%d]):m_node[OUTPUT_NODE]->getBuffer fail ret(%d)", __FUNCTION__, __LINE__, ret);

        metaBuffer.index = index;

        CLOGE("ERR(%s[%d]):Shot done invalid, frame(cnt:%d, index(%d)) skip", __FUNCTION__, __LINE__, getMetaDmRequestFrameCount(shot_ext), index);

        /* complete frame */
        ret = m_completeFrame(&curFrame, metaBuffer, false);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):complete frame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            return ret;
        }

        if (curFrame == NULL) {
            CLOGE("ERR(%s[%d]):curFrame is fail", __FUNCTION__, __LINE__);
            return ret;
        }

        /* Push to outputQ */
        if (m_outputFrameQ != NULL) {
            m_outputFrameQ->pushProcessQ(&curFrame);
        } else {
            CLOGE("ERR(%s[%d]):m_outputFrameQ is NULL", __FUNCTION__, __LINE__);
        }

        CLOGV("DEBUG(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

        return NO_ERROR;
    }

    if (flagOutputNodeRunning == true) {
        ret = m_updateMetadataToFrame(metaBuffer.addr[metaBufferPlaneCount], metaBuffer.index);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): updateMetadataToFrame(%d) fail, ret(%d)", __FUNCTION__, __LINE__, metaBuffer.index, ret);
        }

        m_activityControl->activityAfterExecFunc(getPipeId(), (void *)&metaBuffer);
    }

    CLOGV("DEBUG(%s[%d]):index : %d", __FUNCTION__, __LINE__, index);

    /* complete frame */
    ret = m_completeFrame(&curFrame, metaBuffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):complete frame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return ret;
    }

    if (curFrame == NULL) {
        CLOGE("ERR(%s[%d]):curFrame is fail", __FUNCTION__, __LINE__);
        return ret;
    }

    if (m_outputFrameQ != NULL)
        m_outputFrameQ->pushProcessQ(&curFrame);
    else
        CLOGE("ERR(%s[%d]):m_outputFrameQ is NULL", __FUNCTION__, __LINE__);

    CLOGV("DEBUG(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

bool ExynosCameraPipeDIS::m_mainThreadFunc(void)
{
    bool bRet = true;
    status_t ret = 0;

    if (m_flagTryStop == true) {
        usleep(5000);
        return true;
    }

    ret = m_putBuffer();
    if (ret < 0) {
        if (ret != TIMED_OUT)
            CLOGE("ERR(%s):m_putBuffer fail", __FUNCTION__);

        /* will do m_getBuffer */
        ret = NO_ERROR;
    }

    if (0 < m_numOfRunningFrame) {
        ret = m_getBuffer();
        if (ret < 0) {
            CLOGE("ERR(%s):m_getBuffer fail", __FUNCTION__);
            /* TODO: doing exception handling */
            bRet = true;
            goto done;
        }
    }

    m_timer.stop();
    m_timeInterval = m_timer.durationMsecs();
    m_timer.start();

    /* update renew count */
    if (ret >= 0)
        m_threadRenew = 0;

    bRet = m_checkThreadLoop();

done:
    return bRet;
}

void ExynosCameraPipeDIS::m_init(void)
{
}

}; /* namespace android */
