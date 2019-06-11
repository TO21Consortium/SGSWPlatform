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
#define LOG_TAG "ExynosCameraPipe3AA"
#include <cutils/log.h>

#include "ExynosCameraPipe3AA.h"

namespace android {

ExynosCameraPipe3AA::~ExynosCameraPipe3AA()
{
    this->destroy();
}

status_t ExynosCameraPipe3AA::create(int32_t *sensorIds)
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

    /* 3AA output */
    if (m_flagValidInt(m_nodeNum[OUTPUT_NODE]) == true) {
        m_node[OUTPUT_NODE] = new ExynosCameraNode();
        ret = m_node[OUTPUT_NODE]->create("3AA_OUTPUT", m_cameraId);
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

    /* 3AA capture */
    if (m_flagValidInt(m_nodeNum[CAPTURE_NODE]) == true) {
        m_node[CAPTURE_NODE] = new ExynosCameraNode();

        /* HACK for helsinki. this must fix on istor */
        /* if (1) { */
        if (m_nodeNum[OUTPUT_NODE] == m_nodeNum[CAPTURE_NODE]) {
            ret = m_node[OUTPUT_NODE]->getFd(&fd);
            if (ret != NO_ERROR || m_flagValidInt(fd) == false) {
                CLOGE("ERR(%s):OUTPUT_NODE->getFd failed", __FUNCTION__);
                return ret;
            }

            ret = m_node[CAPTURE_NODE]->create("3AA_CAPTURE", m_cameraId, fd);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]): CAPTURE_NODE create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                return ret;
            }
        } else {
            ret = m_node[CAPTURE_NODE]->create("3AA_CAPTURE", m_cameraId);
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

    m_mainThread = ExynosCameraThreadFactory::createThread(this, &ExynosCameraPipe3AA::m_mainThreadFunc, "3AAThread");

    if (m_reprocessing == true)
        m_inputFrameQ = new frame_queue_t(m_mainThread);
    else
        m_inputFrameQ = new frame_queue_t;
    m_inputFrameQ->setWaitTime(500000000); /* .5 sec */

    m_prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];
    CLOGI("INFO(%s[%d]):create() is succeed (%d) prepare (%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA::destroy(void)
{
    int ret = 0;
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (m_node[OUTPUT_NODE] != NULL) {
        ret = m_node[OUTPUT_NODE]->close();
        if (ret < 0) {
            CLOGE("ERR(%s):3AA OUTPUT_NODE close fail(ret = %d)", __FUNCTION__, ret);
            return INVALID_OPERATION;
        }
        delete m_node[OUTPUT_NODE];
        m_node[OUTPUT_NODE] = NULL;
        CLOGD("DEBUG(%s):Node(OUTPUT_NODE, m_nodeNum : %d, m_sensorIds : %d) closed",
            __FUNCTION__, m_nodeNum[OUTPUT_NODE], m_sensorIds[OUTPUT_NODE]);
    }

    if (m_node[CAPTURE_NODE] != NULL) {
        /* CAPTURE_NODE is created by fd */

        ret = m_node[CAPTURE_NODE]->close();
        if (ret < 0) {
            CLOGE("ERR(%s):3AA CAPTURE_NODE close fail(ret = %d)", __FUNCTION__, ret);
            return INVALID_OPERATION;
        }

        delete m_node[CAPTURE_NODE];
        m_node[CAPTURE_NODE] = NULL;
        CLOGD("DEBUG(%s):Node(CAPTURE_NODE, m_nodeNum : %d, m_sensorIds : %d) closed",
            __FUNCTION__, m_nodeNum[CAPTURE_NODE], m_sensorIds[CAPTURE_NODE]);
    }

    m_mainNodeNum = -1;
    m_mainNode = NULL;

    if (m_inputFrameQ != NULL) {
        m_inputFrameQ->release();
        delete m_inputFrameQ;
        m_inputFrameQ = NULL;
    }

    CLOGI("INFO(%s[%d]):destroy() is succeed (%d)", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA::m_setPipeInfo(camera_pipe_info_t *pipeInfos)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (pipeInfos == NULL) {
        CLOGE("ERR(%s[%d]): pipeInfos == NULL. so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    status_t ret = NO_ERROR;

    /* 3AA output */
    ret = m_setNodeInfo(m_node[OUTPUT_NODE], &pipeInfos[0],
                        2, YUV_FULL_RANGE,
                        true);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]): m_setNodeInfo(%d, %d, %d) fail",
            __FUNCTION__, __LINE__, pipeInfos[0].rectInfo.fullW, pipeInfos[0].rectInfo.fullH, pipeInfos[0].bufInfo.count);
        return INVALID_OPERATION;
    }

    m_numBuffers = pipeInfos[0].bufInfo.count;
    m_perframeMainNodeGroupInfo = pipeInfos[0].perFrameNodeGroupInfo;

    /* 3AA capture */
    ret = m_setNodeInfo(m_node[CAPTURE_NODE], &pipeInfos[1],
                        2, YUV_FULL_RANGE,
                        true);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]): m_setNodeInfo(%d, %d, %d) fail",
            __FUNCTION__, __LINE__, pipeInfos[1].rectInfo.fullW, pipeInfos[1].rectInfo.fullH, pipeInfos[1].bufInfo.count);
        return INVALID_OPERATION;
    }

    m_numCaptureBuf = pipeInfos[1].bufInfo.count;
    m_perframeSubNodeGroupInfo = pipeInfos[1].perFrameNodeGroupInfo;

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA::setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds)
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

    /* setfile setting */
    int setfile = 0;
    int yuvRange = 0;
    m_parameters->getSetfileYuvRange(m_reprocessing, &m_setfile, &yuvRange);
#ifdef SET_SETFILE_BY_SHOT
    m_setfile = mergeSetfileYuvRange(setfile, yuvRange);
#else
#if SET_SETFILE_BY_SET_CTRL_3AA
    if (m_checkLeaderNode(m_sensorIds[OUTPUT_NODE]) == true) {
        ret = m_node[OUTPUT_NODE]->setControl(V4L2_CID_IS_SET_SETFILE, m_setfile);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):setControl(%d) fail(ret = %d)", __FUNCTION__, __LINE__, m_setfile, ret);
            return ret;
        }
        CLOGD("DEBUG(%s[%d]):set setfile(%d),m_reprocessing(%d)", __FUNCTION__, __LINE__, m_setfile, m_reprocessing);
    } else {
        CLOGW("WARN(%s[%d]):m_checkLeaderNode(%d) == false. so, skip set setfile.",
                __FUNCTION__, __LINE__, m_sensorIds[OUTPUT_NODE]);
    }
#endif
#endif

    for (uint32_t i = 0; i < m_numBuffers; i++) {
        m_runningFrameList[i] = NULL;
    }
    m_numOfRunningFrame = 0;

    m_prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];

    CLOGI("INFO(%s[%d]):setupPipe() is succeed (%d) prepare (%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    CLOGI("INFO(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA::prepare(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA::start(void)
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

status_t ExynosCameraPipe3AA::stop(void)
{
    CLOGD("INFO(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    /* 3AA  output stop */
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

status_t ExynosCameraPipe3AA::getPipeInfo(int *fullW, int *fullH, int *colorFormat, int pipePosition)
{
    int planeCount = 0;
    int ret = NO_ERROR;

    if (pipePosition == SRC_PIPE) {
        if (m_node[CAPTURE_NODE] == NULL) {
            CLOGE("ERR(%s): m_node[CAPTURE_NODE] == NULL. so, fail", __FUNCTION__);
            return INVALID_OPERATION;
        }

        ret = m_node[CAPTURE_NODE]->getSize(fullW, fullH);
        if (ret < 0) {
            CLOGE("ERR(%s):getSize fail", __FUNCTION__);
            return ret;
        }

        ret = m_node[CAPTURE_NODE]->getColorFormat(colorFormat, &planeCount);
        if (ret < 0) {
            CLOGE("ERR(%s):getColorFormat fail", __FUNCTION__);
            return ret;
        }
    } else {
        ret = ExynosCameraPipe::getPipeInfo(fullW, fullH, colorFormat, pipePosition);
    }

    return ret;
}

status_t ExynosCameraPipe3AA::m_putBuffer(void)
{
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer fliteBuffer;
    ExynosCameraBuffer ispBuffer;

    int ret = 0;

    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    ret = m_inputFrameQ->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        if (ret == TIMED_OUT) {
            CLOGW("WARN(%s):wait timeout", __FUNCTION__);
            m_node[OUTPUT_NODE]->dumpState();
            m_node[CAPTURE_NODE]->dumpState();
        } else {
            CLOGE("ERR(%s):wait and pop fail, ret(%d)", __FUNCTION__, ret);
            /* TODO: doing exception handling */
        }
        return ret;
    }

    if (newFrame == NULL) {
        CLOGE("ERR(%s):new frame is NULL", __FUNCTION__);
        return INVALID_OPERATION;
        /* return NO_ERROR; */
    }

    /*
     * Even 3aa input is OTf, we need to q, dq.
     * this is important difference from other pipe.
     * in other pipe, it will check OUTPUT_NODE also.
     * but, 3aa pipe will not check OUTPUT_NODE.
     */
    /* if(m_node[OUTPUT_NODE] != NULL &&
     *   m_isOtf(m_node[OUTPUT_NODE]->getInput()) == false) {
     */
    ret = newFrame->getSrcBuffer(getPipeId(), &fliteBuffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):frame get src buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return OK;
    }

    if (m_node[CAPTURE_NODE] != NULL &&
        m_isOtf(m_node[CAPTURE_NODE]->getInput()) == false) {
        ret = newFrame->getDstBuffer(getPipeId(), &ispBuffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):frame get dst buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            return OK;
        }
    }

    if (m_runningFrameList[fliteBuffer.index] != NULL) {
        CLOGE("ERR(%s):new buffer is invalid, we already get buffer index(%d)",
            __FUNCTION__, fliteBuffer.index);
        dump();
        return BAD_VALUE;
    }

    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(fliteBuffer.addr[1]);

    if (shot_ext != NULL) {
        int pictureW = 0, pictureH = 0;
        int cropW = 0, cropH = 0, cropX = 0, cropY = 0;

        m_parameters->getPictureSize(&pictureW, &pictureH);
        m_parameters->getHwBayerCropRegion(&cropW, &cropH, &cropX, &cropY);

        newFrame->getMetaData(shot_ext);
        ret = m_parameters->duplicateCtrlMetadata((void *)shot_ext);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):duplicate Ctrl metadata fail", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }

        CLOGV("DEBUG(%s[%d]):frameCount(%d), rCount(%d)",
                __FUNCTION__, __LINE__,
                newFrame->getFrameCount(), getMetaDmRequestFrameCount(shot_ext));

        if (m_perframeMainNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType == PERFRAME_NODE_TYPE_LEADER) {
            int zoomParamInfo = m_parameters->getZoomLevel();
            int zoomFrameInfo = 0;
            ExynosRect bnsSize;
            ExynosRect bayerCropSize;
            ExynosRect bdsSize;
            camera2_node_group node_group_info;

            newFrame->getNodeGroupInfo(&node_group_info, m_perframeMainNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex, &zoomFrameInfo);

#ifdef PERFRAME_CONTROL_NODE_3AA
            /* HACK: To speed up DZOOM */
            if (zoomFrameInfo != zoomParamInfo) {
                CLOGI("INFO(%s[%d]):zoomFrameInfo(%d), zoomParamInfo(%d)",
                    __FUNCTION__, __LINE__, zoomFrameInfo, zoomParamInfo);

                camera2_node_group node_group_info_isp;
                camera2_node_group node_group_info_dis;

                newFrame->getNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_ISP, &zoomFrameInfo);
                newFrame->getNodeGroupInfo(&node_group_info_dis, PERFRAME_INFO_DIS, &zoomFrameInfo);

                if (m_reprocessing == true) {
                    int pictureW = 0, pictureH = 0;
                    ExynosRect bayerCropSizePicture;
                    m_parameters->getPictureSize(&pictureW, &pictureH);
                    m_parameters->getPictureBayerCropSize(&bnsSize, &bayerCropSizePicture);
                    m_parameters->getPictureBdsSize(&bdsSize);
                    m_parameters->getPreviewBayerCropSize(&bnsSize, &bayerCropSize);

                    ExynosCameraNodeGroup::updateNodeGroupInfo(
                        m_cameraId,
                        &node_group_info,
                        &node_group_info_isp,
                        bayerCropSize,
                        bayerCropSizePicture,
                        bdsSize,
                        pictureW, pictureH,
                        m_parameters->getUsePureBayerReprocessing(),
                        m_parameters->isReprocessing3aaIspOTF());

                    newFrame->storeNodeGroupInfo(&node_group_info, PERFRAME_INFO_PURE_REPROCESSING_3AA, zoomParamInfo);
                    newFrame->storeNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_PURE_REPROCESSING_ISP, zoomParamInfo);
                    /* PERFRAME_INFO_DIS is not required */
                } else if (!m_flagTryStop) {
                    int previewW = 0, previewH = 0;
                    m_parameters->getHwPreviewSize(&previewW, &previewH);
                    m_parameters->getPreviewBayerCropSize(&bnsSize, &bayerCropSize);
                    m_parameters->getPreviewBdsSize(&bdsSize);

                    ExynosCameraNodeGroup3AA::updateNodeGroupInfo(
                        m_cameraId,
                        &node_group_info,
                        bayerCropSize,
                        bdsSize,
                        previewW, previewH,
                        pictureW, pictureH);

                    ExynosCameraNodeGroupISP::updateNodeGroupInfo(
                        m_cameraId,
                        &node_group_info_isp,
                        bayerCropSize,
                        bdsSize,
                        previewW, previewH,
                        pictureW, pictureH,
                        m_parameters->getHWVdisMode());

                    ExynosCameraNodeGroupDIS::updateNodeGroupInfo(
                        m_cameraId,
                        &node_group_info_dis,
                        bayerCropSize,
                        bdsSize,
                        previewW, previewH,
                        pictureW, pictureH);

                    newFrame->storeNodeGroupInfo(&node_group_info,     PERFRAME_INFO_3AA, zoomParamInfo);
                    newFrame->storeNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_ISP, zoomParamInfo);
                    newFrame->storeNodeGroupInfo(&node_group_info_dis, PERFRAME_INFO_DIS, zoomParamInfo);
                }
            }
#endif

            memset(&shot_ext->node_group, 0x0, sizeof(camera2_node_group));

            /* Per - 3AA */
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

            for (int i = 0; i < m_perframeMainNodeGroupInfo.perframeSupportNodeNum; i ++) {
                 if (node_group_info.capture[i].request == 1 ||
                     m_isOtf(m_node[CAPTURE_NODE]->getInput()) == true) {

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
            /* newFrame->dumpNodeGroupInfo("3AA"); */
            /* m_dumpPerframeNodeGroupInfo("m_perframeIspNodeGroupInfo", m_perframeIspNodeGroupInfo); */
            /* m_dumpPerframeNodeGroupInfo("m_perframeMainNodeGroupInfo", m_perframeMainNodeGroupInfo); */
        }
    }

    if (m_node[CAPTURE_NODE] != NULL &&
        m_isOtf(m_node[CAPTURE_NODE]->getInput()) == false) {
        if (m_node[CAPTURE_NODE]->putBuffer(&ispBuffer) != NO_ERROR) {
            CLOGE("ERR(%s):capture putBuffer fail ret(%d)", __FUNCTION__, ret);
            /* TODO: doing exception handling */
            return ret;
        }

        ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_PROCESSING);
        if (ret < 0) {
            CLOGE("ERR(%s): setDstBuffer state fail", __FUNCTION__);
            return ret;
        }
    }

    if (m_node[OUTPUT_NODE]->putBuffer(&fliteBuffer) != NO_ERROR) {
        CLOGE("ERR(%s):output putBuffer fail ret(%d)", __FUNCTION__, ret);
        /* TODO: doing exception handling */
        return ret;
    }

    ret = newFrame->setSrcBufferState(getPipeId(), ENTITY_BUFFER_STATE_PROCESSING);
    if (ret < 0) {
        CLOGE("ERR(%s): setSrcBuffer state fail", __FUNCTION__);
        return ret;
    }

    m_runningFrameList[fliteBuffer.index] = newFrame;
    m_numOfRunningFrame++;

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA::m_getBuffer(void)
{
    ExynosCameraFrame *curFrame = NULL;
    ExynosCameraBuffer fliteBuffer;
    ExynosCameraBuffer ispBuffer;
    int v4l2Colorformat = 0;
    int planeCount[MAX_NODE] = {0};
    int index = 0;
    status_t ret = 0;
    int error = 0;

    CLOGV("DEBUG(%s[%d]): -IN-", __FUNCTION__, __LINE__);

    if (m_numOfRunningFrame <= 0 || m_flagStartPipe == false) {
        CLOGD("DEBUG(%s[%d]): skip getBuffer, flagStartPipe(%d), numOfRunningFrame = %d", __FUNCTION__, __LINE__, m_flagStartPipe, m_numOfRunningFrame);
        return NO_ERROR;
    }

    if (m_node[CAPTURE_NODE] != NULL &&
        m_isOtf(m_node[CAPTURE_NODE]->getInput()) == false) {
        ret = m_node[CAPTURE_NODE]->getBuffer(&ispBuffer, &index);
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

        m_node[CAPTURE_NODE]->getColorFormat(&v4l2Colorformat, &planeCount[CAPTURE_NODE]);
    }

    ret = m_node[OUTPUT_NODE]->getBuffer(&fliteBuffer, &index);
    if (m_flagTryStop == true) {
        CLOGE("DEBUG(%s[%d]):m_flagTryStop(%d)", __FUNCTION__, __LINE__, m_flagTryStop);
        return false;
    }

    m_node[OUTPUT_NODE]->getColorFormat(&v4l2Colorformat, &planeCount[OUTPUT_NODE]);

    if (ret != NO_ERROR || error != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_node[OUTPUT_NODE]->getBuffer fail ret(%d)", __FUNCTION__, __LINE__, ret);
        camera2_shot_ext *shot_ext;
        shot_ext = (struct camera2_shot_ext *)(fliteBuffer.addr[1]);

        if (shot_ext == NULL)
            return BAD_VALUE;

        fliteBuffer.index = index;
        CLOGE("ERR(%s[%d]):Shot done invalid, frame(cnt:%d, index(%d)) skip", __FUNCTION__, __LINE__, getMetaDmRequestFrameCount(shot_ext), index);

        /* complete frame */
        ret = m_completeFrame(&curFrame, fliteBuffer, false);
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

    ret = m_updateMetadataToFrame(fliteBuffer.addr[1], fliteBuffer.index);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): updateMetadataToFrame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
    }

    CLOGV("DEBUG(%s[%d]):index : %d", __FUNCTION__, __LINE__, index);

    nsecs_t timeStamp = (nsecs_t)getMetaDmSensorTimeStamp((struct camera2_shot_ext *)fliteBuffer.addr[1]);
    if (timeStamp < 0) {
        CLOGW("WRN(%s[%d]): frameCount(%d), Invalid timeStamp(%lld)",
           __FUNCTION__, __LINE__,
           getMetaDmRequestFrameCount((struct camera2_shot_ext *)fliteBuffer.addr[1]),
           timeStamp);
    }

    /* complete frame */
    ret = m_completeFrame(&curFrame, fliteBuffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):complete frame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return ret;
    }

    if (curFrame == NULL) {
        CLOGE("ERR(%s[%d]):curFrame is fail", __FUNCTION__, __LINE__);
        return ret;
    }

    /* In M2M case, we copy meta data */
    if (m_node[CAPTURE_NODE] != NULL &&
        m_isOtf(m_node[CAPTURE_NODE]->getInput()) == false) {
        ret = curFrame->getSrcBuffer(getPipeId(), &fliteBuffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):frame get src buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            return OK;
        }

        ret = curFrame->getDstBuffer(getPipeId(), &ispBuffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):frame get dst buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            return OK;
        }

        memcpy(ispBuffer.addr[planeCount[CAPTURE_NODE] - 1], fliteBuffer.addr[planeCount[OUTPUT_NODE] - 1], sizeof(struct camera2_shot_ext));

        CLOGV("DEBUG(%s[%d]):isp frameCount %d", __FUNCTION__, __LINE__,
               getMetaDmRequestFrameCount((struct camera2_shot_ext *)ispBuffer.addr[1]));
    }

    if (m_outputFrameQ != NULL)
        m_outputFrameQ->pushProcessQ(&curFrame);
    else
        CLOGE("ERR(%s[%d]):m_outputFrameQ is NULL", __FUNCTION__, __LINE__);

    CLOGV("DEBUG(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

bool ExynosCameraPipe3AA::m_mainThreadFunc(void)
{
    int ret = 0;

    if (m_flagTryStop == true) {
        usleep(5000);
        return true;
    }

    ret = m_putBuffer();
    if (ret < 0) {
        if (ret == TIMED_OUT)
            return true;
        CLOGE("ERR(%s[%d]):m_putBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return m_checkThreadLoop();
    }

    ret = m_getBuffer();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_getBuffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return m_checkThreadLoop();
    }

    m_timer.stop();
    m_timeInterval = m_timer.durationMsecs();
    m_timer.start();

    /* update renew count */
    if (ret >= 0)
        m_threadRenew = 0;

    return m_checkThreadLoop();
}

void ExynosCameraPipe3AA::m_init(void)
{
    memset(&m_perframeSubNodeGroupInfo, 0x00, sizeof(camera_pipe_perframe_node_group_info_t));
}

}; /* namespace android */
