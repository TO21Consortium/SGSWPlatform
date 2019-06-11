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
#define LOG_TAG "ExynosCameraPipe3AA_ISP"
#include <cutils/log.h>

#include "ExynosCameraPipe3AA_ISP.h"

namespace android {

ExynosCameraPipe3AA_ISP::~ExynosCameraPipe3AA_ISP()
{
    this->destroy();
}

status_t ExynosCameraPipe3AA_ISP::create(int32_t *sensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;
    int fd = -1;

    for (int i = 0; i < MAX_NODE; i++) {
        if (sensorIds) {
            CLOGD("DEBUG(%s[%d]):set new sensorIds[%d] : %d", __FUNCTION__, __LINE__, i, sensorIds[i]);
            m_sensorIds[i] = sensorIds[i];
        } else {
            m_sensorIds[i] = -1;
        }

        m_ispSensorIds[i] = -1;
    }

    if (sensorIds)
        m_copyNodeInfo2IspNodeInfo();

    /* ISP output */
    if (m_flagValidInt(m_ispNodeNum[OUTPUT_NODE]) == true) {
        m_ispNode[OUTPUT_NODE] = new ExynosCameraNode();
        ret = m_ispNode[OUTPUT_NODE]->create("ISP_OUTPUT", m_cameraId);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): m_ispNode[OUTPUT_NODE] create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }

        ret = m_ispNode[OUTPUT_NODE]->open(m_ispNodeNum[OUTPUT_NODE]);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): m_ispNode[OUTPUT_NODE] open fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }
        CLOGD("DEBUG(%s[%d]):Node(%d) opened", __FUNCTION__, __LINE__, m_ispNodeNum[OUTPUT_NODE]);
    }

#if 0
    /*
     * Isp capture node will be open on PIPE_ISP.
     * not remove for history.
     */
    /* ISP capture */
    if (m_flagValidInt(m_ispNodeNum[CAPTURE_NODE]) == true) {
        m_ispNode[CAPTURE_NODE] = new ExynosCameraNode();
        ret = m_ispNode[CAPTURE_NODE]->create("ISP_CAPTURE", m_cameraId);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): m_ispNode[CAPTURE_NODE] create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }

        ret = m_ispNode[CAPTURE_NODE]->open(m_ispNodeNum[CAPTURE_NODE]);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): m_ispNode[CAPTURE_NODE] open fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }
        CLOGD("DEBUG(%s[%d]):Node(%d) opened", __FUNCTION__, __LINE__, m_ispNodeNum[CAPTURE_NODE]);
    }
#endif

    m_ispThread = ExynosCameraThreadFactory::createThread(this, &ExynosCameraPipe3AA_ISP::m_ispThreadFunc, "ISPThread", PRIORITY_URGENT_DISPLAY);

    m_ispBufferQ = new isp_buffer_queue_t;

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
        CLOGD("DEBUG(%s[%d]):Node(%d) opened", __FUNCTION__, __LINE__, m_nodeNum[OUTPUT_NODE]);

        /* mainNode is OUTPUT_NODE */
        m_mainNodeNum = OUTPUT_NODE;
        m_mainNode = m_node[m_mainNodeNum];

        ret = m_node[OUTPUT_NODE]->getFd(&fd);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):OUTPUT_NODE->getFd failed", __FUNCTION__, __LINE__);
            return ret;
        }
    }

    /* 3AA capture */
    if (m_flagValidInt(m_nodeNum[CAPTURE_NODE]) == true) {
        m_node[CAPTURE_NODE] = new ExynosCameraNode();

        /* HACK for helsinki. this must fix on istor */
        /* if (1) { */
        if (m_nodeNum[OUTPUT_NODE] == m_nodeNum[CAPTURE_NODE]) {
            ret = m_node[OUTPUT_NODE]->getFd(&fd);
            if (ret != NO_ERROR || m_flagValidInt(fd) == false) {
                CLOGE("ERR(%s[%d]): OUTPUT_NODE->getFd(%d) failed", __FUNCTION__, __LINE__, fd);
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
            CLOGD("DEBUG(%s[%d]):Node(%d) opened", __FUNCTION__, __LINE__, m_nodeNum[CAPTURE_NODE]);
        }
    }

    /* setInput for 54xx */
    ret = m_setInput(m_ispNode, m_ispNodeNum, m_ispSensorIds);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): m_setInput(isp) fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    ret = m_setInput(m_node, m_nodeNum, m_sensorIds);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): m_setInput() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    m_mainThread = ExynosCameraThreadFactory::createThread(this, &ExynosCameraPipe3AA_ISP::m_mainThreadFunc, "3AAThread", PRIORITY_URGENT_DISPLAY);

    m_inputFrameQ = new frame_queue_t;

    m_prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];

    CLOGI("INFO(%s[%d]):create() is succeed (%d) prepare (%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA_ISP::precreate(int32_t *sensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;
    int fd = -1;

    for (int i = 0; i < MAX_NODE; i++) {
        if (sensorIds) {
            CLOGD("DEBUG(%s[%d]):set new sensorIds[%d] : %d", __FUNCTION__, __LINE__, i, sensorIds[i]);
            m_sensorIds[i] = sensorIds[i];
        } else {
            m_sensorIds[i] = -1;
        }

        m_ispSensorIds[i] = -1;
    }

    if (sensorIds)
        m_copyNodeInfo2IspNodeInfo();

    /* ISP output */
    if (m_flagValidInt(m_ispNodeNum[OUTPUT_NODE]) == true) {
        m_ispNode[OUTPUT_NODE] = new ExynosCameraNode();
        ret = m_ispNode[OUTPUT_NODE]->create("ISP", m_cameraId);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): m_ispNode[OUTPUT_NODE] create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }

        ret = m_ispNode[OUTPUT_NODE]->open(m_ispNodeNum[OUTPUT_NODE]);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): m_ispNode[OUTPUT_NODE] open fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }
        CLOGD("DEBUG(%s[%d]):Node(%d) opened", __FUNCTION__, __LINE__, m_ispNodeNum[OUTPUT_NODE]);
    }

#if 0
    /*
     * Isp capture node will be open on PIPE_ISP.
     * not remove for history.
     */
    /* ISP capture */
    if (m_flagValidInt(m_ispNodeNum[CAPTURE_NODE]) == true) {
        m_ispNode[CAPTURE_NODE] = new ExynosCameraNode();
        ret = m_ispNode[CAPTURE_NODE]->create("ISP_CAPTURE", m_cameraId);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): m_ispNode[CAPTURE_NODE] create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }

        ret = m_ispNode[CAPTURE_NODE]->open(m_ispNodeNum[CAPTURE_NODE]);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): m_ispNode[CAPTURE_NODE] open fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return ret;
        }
        CLOGD("DEBUG(%s[%d]):Node(%d) opened", __FUNCTION__, __LINE__, m_ispNodeNum[CAPTURE_NODE]);
    }
#endif

    m_ispThread = ExynosCameraThreadFactory::createThread(this, &ExynosCameraPipe3AA_ISP::m_ispThreadFunc, "ISPThread", PRIORITY_URGENT_DISPLAY);

    m_ispBufferQ = new isp_buffer_queue_t;

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

        CLOGD("DEBUG(%s[%d]):Node(%d) opened", __FUNCTION__, __LINE__, m_nodeNum[OUTPUT_NODE]);

        /* mainNode is OUTPUT_NODE */
        m_mainNodeNum = OUTPUT_NODE;
        m_mainNode = m_node[m_mainNodeNum];

        ret = m_node[OUTPUT_NODE]->getFd(&fd);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_node[OUTPUT_NODE]->getFd(%d) failed", __FUNCTION__, __LINE__, fd);
            return ret;
        }
    }

    /* 3AA capture */
    if (m_flagValidInt(m_nodeNum[CAPTURE_NODE]) == true) {
        m_node[CAPTURE_NODE] = new ExynosCameraNode();

        /* HACK for helsinki. this must fix on istor */
        /* if (1) { */
        if (m_nodeNum[OUTPUT_NODE] == m_nodeNum[CAPTURE_NODE]) {
            ret = m_node[OUTPUT_NODE]->getFd(&fd);
            if (ret != NO_ERROR || m_flagValidInt(fd) == false) {
                CLOGE("ERR(%s[%d]): OUTPUT_NODE->getFd(%d) failed", __FUNCTION__, __LINE__, fd);
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
            CLOGD("DEBUG(%s[%d]):Node(%d) opened", __FUNCTION__, __LINE__, m_nodeNum[CAPTURE_NODE]);
        }
    }

    m_mainThread = ExynosCameraThreadFactory::createThread(this, &ExynosCameraPipe3AA_ISP::m_mainThreadFunc, "3AAThread", PRIORITY_URGENT_DISPLAY);

    CLOGI("INFO(%s[%d]):precreate() is succeed (%d) prepare", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA_ISP::postcreate(int32_t *sensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;
    int fd = -1;

    for (int i = 0; i < MAX_NODE; i++) {
        if (sensorIds) {
            CLOGD("DEBUG(%s[%d]):set new sensorIds[%d] : %d", __FUNCTION__, __LINE__, i, sensorIds[i]);
            m_sensorIds[i] = sensorIds[i];
        } else {
            m_sensorIds[i] = -1;
        }

        m_ispSensorIds[i] = -1;
    }

    if (sensorIds)
        m_copyNodeInfo2IspNodeInfo();

    ret = m_setInput(m_ispNode, m_ispNodeNum, m_ispSensorIds);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): m_setInput(isp) fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    ret = m_setInput(m_node, m_nodeNum, m_sensorIds);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): m_setInput() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    m_inputFrameQ = new frame_queue_t;

    m_prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];
    CLOGI("INFO(%s[%d]):postcreate() is succeed (%d) prepare (%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA_ISP::destroy(void)
{
    int ret = 0;
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

#if 0
    /*
     * Isp capture node will be open on PIPE_ISP.
     * not remove for history.
     */
    if (m_ispNode[CAPTURE_NODE] != NULL) {
        ret = m_ispNode[CAPTURE_NODE]->close();
        if (ret < 0) {
            CLOGE("ERR(%s):m_ispNode[CAPTURE_NODE] close fail(ret = %d)", __FUNCTION__, ret);
            return ret;
        }
        delete m_ispNode[CAPTURE_NODE];
        m_ispNode[CAPTURE_NODE] = NULL;
        CLOGD("DEBUG(%s):Node(m_ispNode[CAPTURE_NODE], m_nodeNum : %d, m_sensorIds : %d) closed",
            __FUNCTION__, m_ispNodeNum[CAPTURE_NODE], m_ispSensorIds[CAPTURE_NODE]);
    }
    CLOGD("DEBUG(%s[%d]):m_ispNode[OUTPUT_NODE] destroyed", __FUNCTION__, __LINE__);
#endif

    if (m_ispNode[OUTPUT_NODE] != NULL) {
        ret = m_ispNode[OUTPUT_NODE]->close();
        if (ret < 0) {
            CLOGE("ERR(%s):m_ispNode[OUTPUT_NODE] close fail(ret = %d)", __FUNCTION__, ret);
            return ret;
        }
        delete m_ispNode[OUTPUT_NODE];
        m_ispNode[OUTPUT_NODE] = NULL;
        CLOGD("DEBUG(%s):Node(m_ispNode[OUTPUT_NODE], m_nodeNum : %d, m_sensorIds : %d) closed",
            __FUNCTION__, m_ispNodeNum[OUTPUT_NODE], m_ispSensorIds[OUTPUT_NODE]);
    }
    CLOGD("DEBUG(%s[%d]):m_ispNode[OUTPUT_NODE] destroyed", __FUNCTION__, __LINE__);

    if (m_node[CAPTURE_NODE] != NULL) {
        ret = m_node[CAPTURE_NODE]->close();
        if (ret < 0) {
            CLOGE("ERR(%s):m_node[CAPTURE_NODE] close fail(ret = %d)", __FUNCTION__, ret);
            return ret;
        }
        delete m_node[CAPTURE_NODE];
        m_node[CAPTURE_NODE] = NULL;
        CLOGD("DEBUG(%s):Node(CAPTURE_NODE, m_nodeNum : %d, m_sensorIds : %d) closed",
            __FUNCTION__, m_nodeNum[CAPTURE_NODE], m_sensorIds[CAPTURE_NODE]);
    }
    CLOGD("DEBUG(%s[%d]):m_nodeNum[CAPTURE_NODE] destroyed", __FUNCTION__, __LINE__);

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
    CLOGD("DEBUG(%s[%d]):3AA OUTPUT_NODE destroyed", __FUNCTION__, __LINE__);

    m_mainNodeNum = -1;
    m_mainNode = NULL;

    if (m_inputFrameQ != NULL) {
        m_inputFrameQ->release();
        delete m_inputFrameQ;
        m_inputFrameQ = NULL;
    }

    if (m_ispBufferQ != NULL) {
        m_ispBufferQ->release();
        delete m_ispBufferQ;
        m_ispBufferQ = NULL;
    }

    CLOGI("INFO(%s[%d]):destroy() is succeed (%d)", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA_ISP::m_setPipeInfo(camera_pipe_info_t *pipeInfos)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    status_t ret = NO_ERROR;

    if (pipeInfos == NULL) {
        CLOGE("ERR(%s[%d]): pipeInfos == NULL. so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    /* isp output */
    if (0 < pipeInfos[2].rectInfo.fullW &&
        0 < pipeInfos[2].rectInfo.fullH) {
        ret = m_setNodeInfo(m_ispNode[OUTPUT_NODE], &pipeInfos[2],
                        2, YUV_FULL_RANGE,
                        true);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]): m_setNodeInfo(%d, %d, %d) fail",
                __FUNCTION__, __LINE__, pipeInfos[2].rectInfo.fullW, pipeInfos[2].rectInfo.fullH, pipeInfos[2].bufInfo.count);
            return INVALID_OPERATION;
        }

        m_numBuffers = pipeInfos[2].bufInfo.count;
        m_perframeIspNodeGroupInfo = pipeInfos[2].perFrameNodeGroupInfo;
    }

    /* 3a1 output */
    ret = m_setNodeInfo(m_node[OUTPUT_NODE], &pipeInfos[0],
                    2, YUV_FULL_RANGE);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]): m_setNodeInfo(%d, %d, %d) fail",
            __FUNCTION__, __LINE__, pipeInfos[0].rectInfo.fullW, pipeInfos[0].rectInfo.fullH, pipeInfos[0].bufInfo.count);
        return INVALID_OPERATION;
    }

    m_numBuffers = pipeInfos[0].bufInfo.count;
    m_perframeMainNodeGroupInfo = pipeInfos[0].perFrameNodeGroupInfo;


    /* 3a1 capture */
    if (0 < pipeInfos[1].rectInfo.fullW &&
        0 < pipeInfos[1].rectInfo.fullH) {
        ret = m_setNodeInfo(m_node[CAPTURE_NODE], &pipeInfos[1],
                        2, YUV_FULL_RANGE,
                        true);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]): m_setNodeInfo(%d, %d, %d) fail",
                __FUNCTION__, __LINE__, pipeInfos[1].rectInfo.fullW, pipeInfos[1].rectInfo.fullH, pipeInfos[1].bufInfo.count);
            return INVALID_OPERATION;
        }

        m_numBuffers = pipeInfos[1].bufInfo.count;
        m_perframeSubNodeGroupInfo = pipeInfos[1].perFrameNodeGroupInfo;
    }

    return NO_ERROR;
}


status_t ExynosCameraPipe3AA_ISP::setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    status_t ret = NO_ERROR;

    ret = ExynosCameraPipe3AA_ISP::setupPipe(pipeInfos, sensorIds, NULL);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]): ExynosCameraPipe3AA_ISP::setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    CLOGI("INFO(%s[%d]):setupPipe() is succeed (%d))", __FUNCTION__, __LINE__, getPipeId());

    return ret;
}

status_t ExynosCameraPipe3AA_ISP::setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds, int32_t *ispSensorIds)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

#ifdef DEBUG_RAWDUMP
    unsigned int bytesPerLine[EXYNOS_CAMERA_BUFFER_MAX_PLANES] = {0};
#endif
    /* TODO: check node state */

    /* set new sensorId to m_sensorIds */
    if (sensorIds) {
        for (int i = 0; i < MAX_NODE; i++) {
            CLOGD("DEBUG(%s[%d]):set new sensorIds[%d] : %d -> %d", __FUNCTION__, __LINE__, i, m_sensorIds[i], sensorIds[i]);

            m_sensorIds[i] = sensorIds[i];
        }
    }

    if (ispSensorIds) {
        for (int i = 0; i < MAX_NODE; i++) {
            CLOGD("DEBUG(%s[%d]):set new ispSensorIds[%d] : %d -> %d", __FUNCTION__, __LINE__, i, m_ispSensorIds[i], ispSensorIds[i]);

            m_ispSensorIds[i] = ispSensorIds[i];
        }
    }

    ret = m_setInput(m_ispNode, m_ispNodeNum, m_ispSensorIds);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): m_setInput(isp) fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    ret = m_setInput(m_node, m_nodeNum, m_sensorIds);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): m_setInput() fail, ret(%d)", __FUNCTION__, __LINE__, ret);
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
#if SET_SETFILE_BY_SET_CTRL_3AA_ISP
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

    if (m_isOtf(m_node[CAPTURE_NODE]->getInput()) == true) {
        CLOGD("DEBUG(%s[%d]):m_isOtf(m_node[CAPTURE_NODE]) == true. so, stop m_ispThread", __FUNCTION__, __LINE__);
        m_ispThread->requestExit();
        m_ispBufferQ->sendCmd(WAKE_UP);
    }

    for (uint32_t i = 0; i < m_numBuffers; i++) {
        m_runningFrameList[i] = NULL;
    }
    m_numOfRunningFrame = 0;

    m_prepareBufferCount = m_exynosconfig->current->pipeInfo.prepare[getPipeId()];

    CLOGI("INFO(%s[%d]):setupPipe() is succeed (%d) prepare (%d)", __FUNCTION__, __LINE__, getPipeId(), m_prepareBufferCount);
    return NO_ERROR;
}

status_t ExynosCameraPipe3AA_ISP::prepare(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;

    for (uint32_t i = 0; i < m_prepareBufferCount; i++) {
        ret = m_putBuffer();
        if (ret < 0) {
            CLOGE("ERR(%s):m_putBuffer fail(ret = %d)", __FUNCTION__, ret);
            return ret;
        }
    }

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA_ISP::instantOn(int32_t numFrames)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer newBuffer;

    if (m_inputFrameQ->getSizeOfProcessQ() != numFrames) {
        CLOGE("ERR(%s[%d]): instantOn need %d Frames, but %d Frames are queued",
                __FUNCTION__, __LINE__, numFrames, m_inputFrameQ->getSizeOfProcessQ());
        return INVALID_OPERATION;
    }

    if (m_ispNode[OUTPUT_NODE]) {
        ret = m_ispNode[OUTPUT_NODE]->start();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): m_ispNode[OUTPUT_NODE] instantOn fail", __FUNCTION__, __LINE__);
            return ret;
        }
    }

    if (m_node[CAPTURE_NODE]) {
        ret = m_node[CAPTURE_NODE]->start();
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]): Starting CAPTURE_NODE Error!", __FUNCTION__, __LINE__);
            return ret;
        }
    }

    if (m_node[OUTPUT_NODE]) {
        ret = m_node[OUTPUT_NODE]->start();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): OUTPUT_NODE instantOn fail", __FUNCTION__, __LINE__);
            return ret;
        }
    }

    for (int i = 0; i < numFrames; i++) {
        ret = m_inputFrameQ->popProcessQ(&newFrame);
        if (ret < 0) {
            CLOGE("ERR(%s):wait and pop fail, ret(%d)", __FUNCTION__, ret);
            return ret;
        }

        if (newFrame == NULL) {
            CLOGE("ERR(%s[%d]):newFrame is NULL", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }

        ret = newFrame->getSrcBuffer(getPipeId(), &newBuffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):frame get buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return INVALID_OPERATION;
        }

        if (m_node[OUTPUT_NODE]) {
            CLOGD("DEBUG(%s[%d]): put instantOn Buffer (index %d)", __FUNCTION__, __LINE__, newBuffer.index);

            ret = m_node[OUTPUT_NODE]->putBuffer(&newBuffer);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):putBuffer fail", __FUNCTION__, __LINE__);
                return ret;
                /* TODO: doing exception handling */
            }
        }
    }

    return ret;
}

status_t ExynosCameraPipe3AA_ISP::instantOff(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    int ret = 0;

    if (m_node[OUTPUT_NODE])
        ret = m_node[OUTPUT_NODE]->stop();

    if (m_node[CAPTURE_NODE])
        ret = m_node[CAPTURE_NODE]->stop();

    if (m_ispNode[OUTPUT_NODE])
        ret = m_ispNode[OUTPUT_NODE]->stop();

    if (m_node[OUTPUT_NODE]) {
        ret = m_node[OUTPUT_NODE]->clrBuffers();
        if (ret < 0) {
            CLOGE("ERR(%s):3AA OUTPUT_NODE clrBuffers fail, ret(%d)", __FUNCTION__, ret);
            return ret;
        }
    }

    if (m_node[CAPTURE_NODE]) {
        ret = m_node[CAPTURE_NODE]->clrBuffers();
        if (ret < 0) {
            CLOGE("ERR(%s):3AA CAPTURE_NODE clrBuffers fail, ret(%d)", __FUNCTION__, ret);
            return ret;
        }
    }

    if (m_ispNode[OUTPUT_NODE]) {
        ret = m_ispNode[OUTPUT_NODE]->clrBuffers();
        if (ret < 0) {
            CLOGE("ERR(%s):m_ispNode[OUTPUT_NODE] clrBuffers fail, ret(%d)", __FUNCTION__, ret);
            return ret;
        }
    }

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA_ISP::forceDone(unsigned int cid, int value)
{
    ALOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    if (m_node[OUTPUT_NODE] == NULL) {
        CLOGE("ERR(%s):m_node[OUTPUT_NODE] is NULL", __FUNCTION__);
        return INVALID_OPERATION;
    }


    ret = m_forceDone(m_node[OUTPUT_NODE], cid, value);
    if (ret != NO_ERROR)
        CLOGE("ERR(%s):m_forceDone is failed", __FUNCTION__);


    if (m_ispNode[OUTPUT_NODE] == NULL) {
        CLOGE("ERR(%s):m_node[OUTPUT_NODE] m_ispNode[OUTPUT_NODE] is NULL", __FUNCTION__);
        return INVALID_OPERATION;
    }

    ret = m_forceDone(m_ispNode[OUTPUT_NODE], cid, value);
    if (ret != NO_ERROR)
        CLOGE("ERR(%s):m_ispNode[OUTPUT_NODE] m_forceDone is failed", __FUNCTION__);

    return ret;

}

status_t ExynosCameraPipe3AA_ISP::setControl(int cid, int value)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    int ret = NO_ERROR;

    if (m_node[OUTPUT_NODE]) {
        ret = m_node[OUTPUT_NODE]->setControl(cid, value);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_node[OUTPUT_NODE]->setControl failed", __FUNCTION__, __LINE__);
            return ret;
        }
    }

    return ret;
}

status_t ExynosCameraPipe3AA_ISP::start(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    /* TODO: check state ready for start */

    int ret = NO_ERROR;

    if (m_ispNode[OUTPUT_NODE]) {
        ret = m_ispNode[OUTPUT_NODE]->start();
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]): Starting m_ispNode[OUTPUT_NODE] Error!", __FUNCTION__, __LINE__);
            return ret;
        }
    }

    if (m_node[CAPTURE_NODE]) {
        ret = m_node[CAPTURE_NODE]->start();
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]): Starting CAPTURE_NODE Error!", __FUNCTION__, __LINE__);
            return ret;
        }
    }

    if (m_node[OUTPUT_NODE]) {
        ret = m_node[OUTPUT_NODE]->start();
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]): Starting OUTPUT_NODE Error!", __FUNCTION__, __LINE__);
            return ret;
        }
    }

    m_flagStartPipe = true;
    m_flagTryStop = false;

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA_ISP::stop(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    int ret = NO_ERROR;

    /* 3AA output stop */
    if (m_node[OUTPUT_NODE]) {
        ret = m_node[OUTPUT_NODE]->stop();
        if (ret < 0)
            CLOGE("ERR(%s):3AA output node stop fail, ret(%d)", __FUNCTION__, ret);

        ret = m_node[OUTPUT_NODE]->clrBuffers();
        if (ret < 0)
            CLOGE("ERR(%s):3AA output node clrBuffers fail, ret(%d)", __FUNCTION__, ret);
    }

    /* 3AA capture stop */
    if (m_node[CAPTURE_NODE]) {
        ret = m_node[CAPTURE_NODE]->stop();
        if (ret < 0)
            CLOGE("ERR(%s):3AA capture node stop fail, ret(%d)", __FUNCTION__, ret);

        ret = m_node[CAPTURE_NODE]->clrBuffers();
        if (ret < 0)
            CLOGE("ERR(%s):3AA capture node clrBuffers fail, ret(%d)", __FUNCTION__, ret);
    }

    /* isp output stop */
    if (m_ispNode[OUTPUT_NODE]) {
        ret = m_ispNode[OUTPUT_NODE]->stop();
        if (ret < 0)
            CLOGE("ERR(%s):m_ispNode[OUTPUT_NODE] stop fail, ret(%d)", __FUNCTION__, ret);

        ret = m_ispNode[OUTPUT_NODE]->clrBuffers();
        if (ret < 0)
            CLOGE("ERR(%s):m_ispNode[OUTPUT_NODE] clrBuffers fail, ret(%d)", __FUNCTION__, ret);
    }

    m_mainThread->requestExitAndWait();
    m_ispThread->requestExitAndWait();

    m_inputFrameQ->release();
    m_ispBufferQ->release();

    for (uint32_t i = 0; i < m_numBuffers; i++)
        m_runningFrameList[i] = NULL;

    m_numOfRunningFrame = 0;

    CLOGD("DEBUG(%s[%d]): thead exited", __FUNCTION__, __LINE__);

    if (m_node[OUTPUT_NODE])
        m_node[OUTPUT_NODE]->removeItemBufferQ();

    if (m_node[CAPTURE_NODE])
        m_node[CAPTURE_NODE]->removeItemBufferQ();

    if (m_ispNode[OUTPUT_NODE])
        m_ispNode[OUTPUT_NODE]->removeItemBufferQ();

    m_flagStartPipe = false;
    m_flagTryStop = false;

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA_ISP::startThread(void)
{
    if (m_outputFrameQ == NULL) {
        CLOGE("ERR(%s):outputFrameQ is NULL, cannot start", __FUNCTION__);
        return INVALID_OPERATION;
    }

    m_mainThread->run();
    m_ispThread->run();

    CLOGI("INFO(%s[%d]):startThread is succeed (%d)", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA_ISP::stopThread(void)
{
    /* stop thread */
    m_mainThread->requestExit();
    m_ispThread->requestExit();

    m_inputFrameQ->sendCmd(WAKE_UP);
    m_ispBufferQ->sendCmd(WAKE_UP);

    m_dumpRunningFrameList();

    return NO_ERROR;
}

void ExynosCameraPipe3AA_ISP::dump()
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    m_dumpRunningFrameList();
    m_node[OUTPUT_NODE]->dump();
    m_node[CAPTURE_NODE]->dump();
    m_ispNode[OUTPUT_NODE]->dump();

    return;
}

status_t ExynosCameraPipe3AA_ISP::m_putBuffer(void)
{
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer fliteBuffer;
    ExynosCameraBuffer ispBuffer;
    int bufIndex = -1;
    int ret = 0;

    ret = m_inputFrameQ->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
        if (ret == TIMED_OUT) {
            CLOGW("WARN(%s): wait timeout", __FUNCTION__);
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
        //return INVALID_OPERATION;
        return NO_ERROR;
    }

    ret = newFrame->getSrcBuffer(getPipeId(), &fliteBuffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):frame get src buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return OK;
    }

    if (m_isOtf(m_node[CAPTURE_NODE]->getInput()) == true) {
        bufIndex = fliteBuffer.index;
    } else {
        ret = newFrame->getDstBuffer(getPipeId(), &ispBuffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):frame get dst buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            return OK;
        }

        bufIndex = ispBuffer.index;
    }

    if (bufIndex < 0) {
        CLOGE("ERR(%s[%d]):bufIndex(%d) < 0 fail", __FUNCTION__, __LINE__, bufIndex);
        /* TODO: doing exception handling */
        return OK;
    }

    if (m_runningFrameList[bufIndex] != NULL) {
        CLOGE("ERR(%s):new buffer is invalid, we already get buffer index(%d), newFrame->frameCount(%d)",
            __FUNCTION__, bufIndex, newFrame->getFrameCount());
        m_dumpRunningFrameList();
        return BAD_VALUE;
    }

    if (m_isOtf(m_node[CAPTURE_NODE]->getInput()) == false) {
        ret = m_node[CAPTURE_NODE]->putBuffer(&ispBuffer);
        if (ret < 0) {
            CLOGE("ERR(%s):m_node[CAPTURE_NODE]->putBuffer fail ret(%d)", __FUNCTION__, ret);
            /* TODO: doing exception handling */
            return ret;
        }
    }

    camera2_shot_ext *shot_ext;
    shot_ext = (struct camera2_shot_ext *)(fliteBuffer.addr[1]);

    if (shot_ext != NULL) {
        int previewW = 0, previewH = 0;
        int pictureW = 0, pictureH = 0;
        int cropW = 0, cropH = 0, cropX = 0, cropY = 0;

        m_parameters->getHwPreviewSize(&previewW, &previewH);
        m_parameters->getPictureSize(&pictureW, &pictureH);
        m_parameters->getHwBayerCropRegion(&cropW, &cropH, &cropX, &cropY);

        newFrame->getMetaData(shot_ext);
        ret = m_parameters->duplicateCtrlMetadata((void *)shot_ext);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):duplicate Ctrl metadata fail", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }

        /* setfile setting */
        ALOGV("INFO(%s[%d]):setfile(%d), m_reprocessing(%d)", __FUNCTION__, __LINE__, m_setfile, m_reprocessing);
        setMetaSetfile(shot_ext, m_setfile);

        m_activityControl->activityBeforeExecFunc(getPipeId(), (void *)&fliteBuffer);

        if (m_perframeMainNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType == PERFRAME_NODE_TYPE_LEADER) {
            int zoomParamInfo = m_parameters->getZoomLevel();
            int zoomFrameInfo = 0;
            int previewW = 0, previewH = 0;
            int pictureW = 0, pictureH = 0;
            ExynosRect sensorSize;
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

                m_parameters->getHwPreviewSize(&previewW, &previewH);
                m_parameters->getPictureSize(&pictureW, &pictureH);
                m_parameters->getPreviewBayerCropSize(&sensorSize, &bayerCropSize);
                m_parameters->getPreviewBdsSize(&bdsSize);

                newFrame->getNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_ISP, &zoomFrameInfo);
                newFrame->getNodeGroupInfo(&node_group_info_dis, PERFRAME_INFO_DIS, &zoomFrameInfo);

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
#endif
            memset(&shot_ext->node_group, 0x0, sizeof(camera2_node_group));

            /* Per - 3AA */
            if (node_group_info.leader.request == 1) {

                if (m_checkNodeGroupInfo(m_node[OUTPUT_NODE]->getName(), &m_curNodeGroupInfo.leader, &node_group_info.leader) != NO_ERROR)
                    CLOGW("WARN(%s[%d]): m_checkNodeGroupInfo(%s) fail", __FUNCTION__, __LINE__, m_node[OUTPUT_NODE]->getName());

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

            /* Per - 0:3AC 1:3AP 2:SCP*/
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

            /* CLOGI("INFO(%s[%d]):fcount(%d)", __FUNCTION__, __LINE__, shot_ext->shot.dm.request.frameCount); */
            /* newFrame->dumpNodeGroupInfo("3AA_ISP"); */
            /* m_dumpPerframeNodeGroupInfo("m_perframeIspNodeGroupInfo", m_perframeIspNodeGroupInfo); */
            /* m_dumpPerframeNodeGroupInfo("m_perframeMainNodeGroupInfo", m_perframeMainNodeGroupInfo); */
        }
    }

    ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_PROCESSING);
    if (ret < 0) {
        CLOGE("ERR(%s): setDstBuffer state fail", __FUNCTION__);
        return ret;
    }

    ret = m_node[OUTPUT_NODE]->putBuffer(&fliteBuffer);
    if (ret < 0) {
        CLOGE("ERR(%s):output putBuffer fail ret(%d)", __FUNCTION__, ret);
        /* TODO: doing exception handling */
        return ret;
    }

    ret = newFrame->setSrcBufferState(getPipeId(), ENTITY_BUFFER_STATE_PROCESSING);
    if (ret < 0) {
        CLOGE("ERR(%s): setSrcBuffer state fail", __FUNCTION__);
        return ret;
    }

    m_runningFrameList[bufIndex] = newFrame;
    m_numOfRunningFrame++;

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA_ISP::m_getBuffer(void)
{
    ExynosCameraFrame *curFrame = NULL;
    ExynosCameraFrame *perframeFrame = NULL;
    ExynosCameraBuffer fliteBuffer;
    ExynosCameraBuffer ispBuffer;
    ExynosCameraBuffer resultBuffer;

    int index = 0;
    int ret = 0;
    int error = 0;
    camera2_node_group node_group_info;

    memset(&node_group_info, 0x0, sizeof(camera2_node_group));
    fliteBuffer.addr[1] = NULL;

    ispBuffer.index = -1;
    resultBuffer.index = -1;

    if (m_numOfRunningFrame <= 0 || m_flagStartPipe == false) {
        CLOGD("DEBUG(%s[%d]): skip getBuffer, flagStartPipe(%d), numOfRunningFrame = %d", __FUNCTION__, __LINE__, m_flagStartPipe, m_numOfRunningFrame);
        return NO_ERROR;
    }

    if (m_isOtf(m_node[CAPTURE_NODE]->getInput()) == false) {
        ret = m_node[CAPTURE_NODE]->getBuffer(&ispBuffer, &index);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_node[CAPTURE_NODE]->getBuffer fail ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            error = ret;
        } else {
            struct camera2_stream *stream = (struct camera2_stream *)(ispBuffer.addr[1]);
            if (m_checkValidFrameCount(stream) == false) {
                CLOGW("WARN(%s[%d]):m_checkValidFrameCount() fail. so, frame(cnt:%d)) skip", __FUNCTION__, __LINE__, stream->fcount);
                error = INVALID_OPERATION;
            }
        }

        resultBuffer = ispBuffer;
    }

    CLOGV("DEBUG(%s[%d]):index : %d", __FUNCTION__, __LINE__, index);

    ret = m_node[OUTPUT_NODE]->getBuffer(&fliteBuffer, &index);

    /* in case of 3aa_isp OTF and 3ap capture fail.*/
    if (resultBuffer.index < 0) {
        resultBuffer = fliteBuffer;
    }

    if (ret != NO_ERROR || error != NO_ERROR) {

        CLOGE("ERR(%s[%d]):m_node[OUTPUT_NODE]->getBuffer fail ret(%d)", __FUNCTION__, __LINE__, ret);
        camera2_shot_ext *shot_ext;
        shot_ext = (struct camera2_shot_ext *)(fliteBuffer.addr[1]);
        CLOGE("ERR(%s[%d]):Shot done invalid, frame(cnt:%d, index(%d)) skip", __FUNCTION__, __LINE__, getMetaDmRequestFrameCount(shot_ext), index);

        if (fliteBuffer.addr[1] != NULL) {
            ret = m_updateMetadataToFrame(fliteBuffer.addr[1], resultBuffer.index);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]): updateMetadataToFrame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            }
        }

        /* complete frame */
        ret = m_completeFrame(&curFrame, resultBuffer, false);
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
        curFrame->setIspDone(true);
        curFrame->setDisDrop(true);
        curFrame->setScpDrop(true);

        if (m_parameters->isReprocessing() == false) {
            curFrame->setIspcDrop(true);
            curFrame->setSccDrop(true);
        }

        /* Push to outputQ */
        if (m_outputFrameQ != NULL) {
            m_outputFrameQ->pushProcessQ(&curFrame);
        } else {
            CLOGE("ERR(%s[%d]):m_outputFrameQ is NULL", __FUNCTION__, __LINE__);
        }

        CLOGV("DEBUG(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

        /* add delay to run m_monitoer thread */
        usleep(15000);

        return NO_ERROR;

    }

    m_activityControl->activityAfterExecFunc(getPipeId(), (void *)&resultBuffer);

    ret = m_updateMetadataToFrame(fliteBuffer.addr[1], resultBuffer.index);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): updateMetadataToFrame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
    }

    ret = m_getFrameByIndex(&perframeFrame, resultBuffer.index);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): getFrameByIndex fail, ret(%d)", __FUNCTION__, __LINE__, ret);
    } else {
        perframeFrame->getNodeGroupInfo(&node_group_info, m_perframeIspNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex);
#if 0
    ret = m_checkShotDone((struct camera2_shot_ext*)fliteBuffer.addr[1]);
    if (ret < 0) {
        CLOGE("ERR(%s):Shot done invalid, frame skip", __FUNCTION__);
        /* TODO: doing exception handling */

        /* complete frame */
        ret = m_completeFrame(&curFrame, resultBuffer, false);
        if (ret < 0) {
            CLOGE("ERR(%s):m_completeFrame is fail", __FUNCTION__);
            /* TODO: doing exception handling */
            return ret;
        }

        if (curFrame == NULL) {
            CLOGE("ERR(%s):curFrame is fail", __FUNCTION__);
        }

        m_outputFrameQ->pushProcessQ(&curFrame);

        return NO_ERROR;
#endif
    }

    /* TODO: Is it necessary memcpy shot.ctl from parameter? */
    camera2_shot_ext *shot_ext_src = (struct camera2_shot_ext *)(fliteBuffer.addr[1]);
    camera2_shot_ext *shot_ext_dst = (struct camera2_shot_ext *)(ispBuffer.addr[1]);

    if (shot_ext_src == NULL) {
        CLOGE("ERR(%s[%d]):shot_ext_src == NULL. so, fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (shot_ext_dst == NULL &&
        m_isOtf(m_node[CAPTURE_NODE]->getInput()) == false) {
        CLOGE("ERR(%s[%d]):shot_ext_dst == NULL. so, fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if ((shot_ext_src != NULL) && (shot_ext_dst != NULL)) {
        int previewW, previewH;
        m_parameters->getHwPreviewSize(&previewW, &previewH);
        memcpy(&shot_ext_dst->shot.ctl, &shot_ext_src->shot.ctl, sizeof(struct camera2_ctl) - sizeof(struct camera2_entry_ctl));
        memcpy(&shot_ext_dst->shot.udm, &shot_ext_src->shot.udm, sizeof(struct camera2_udm));
        memcpy(&shot_ext_dst->shot.dm, &shot_ext_src->shot.dm, sizeof(struct camera2_dm));

        if (m_perframeIspNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType == PERFRAME_NODE_TYPE_LEADER) {
            memset(&shot_ext_dst->node_group, 0x0, sizeof(camera2_node_group));

            /* Per - ISP */
            if (node_group_info.leader.request == 1) {

                if (m_checkNodeGroupInfo(m_ispNode[OUTPUT_NODE]->getName(), &m_curIspNodeGroupInfo.leader, &node_group_info.leader) != NO_ERROR)
                    CLOGW("WARN(%s[%d]): m_checkNodeGroupInfo(%s) fail", __FUNCTION__, __LINE__, m_ispNode[OUTPUT_NODE]->getName());

                setMetaNodeLeaderInputSize(shot_ext_dst,
                    node_group_info.leader.input.cropRegion[0],
                    node_group_info.leader.input.cropRegion[1],
                    node_group_info.leader.input.cropRegion[2],
                    node_group_info.leader.input.cropRegion[3]);
                setMetaNodeLeaderOutputSize(shot_ext_dst,
                    node_group_info.leader.output.cropRegion[0],
                    node_group_info.leader.output.cropRegion[1],
                    node_group_info.leader.output.cropRegion[2],
                    node_group_info.leader.output.cropRegion[3]);
                setMetaNodeLeaderRequest(shot_ext_dst,
                    node_group_info.leader.request);
                setMetaNodeLeaderVideoID(shot_ext_dst,
                    m_perframeIspNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID);
            }

            /* Per - SCP */
            for (int i = 0; i < m_perframeIspNodeGroupInfo.perframeSupportNodeNum; i ++) {
                if (node_group_info.capture[i].request == 1) {

                    if (m_checkNodeGroupInfo(i, &m_curIspNodeGroupInfo.capture[i], &node_group_info.capture[i]) != NO_ERROR)
                        CLOGW("WARN(%s[%d]): m_checkNodeGroupInfo(%d) fail", __FUNCTION__, __LINE__, i);

                    setMetaNodeCaptureInputSize(shot_ext_dst, i,
                        node_group_info.capture[i].input.cropRegion[0],
                        node_group_info.capture[i].input.cropRegion[1],
                        node_group_info.capture[i].input.cropRegion[2],
                        node_group_info.capture[i].input.cropRegion[3]);
                    setMetaNodeCaptureOutputSize(shot_ext_dst, i,
                        node_group_info.capture[i].output.cropRegion[0],
                        node_group_info.capture[i].output.cropRegion[1],
                        node_group_info.capture[i].output.cropRegion[2],
                        node_group_info.capture[i].output.cropRegion[3]);
                    setMetaNodeCaptureRequest(shot_ext_dst, i, node_group_info.capture[i].request);
                    setMetaNodeCaptureVideoID(shot_ext_dst, i, m_perframeIspNodeGroupInfo.perFrameCaptureInfo[i].perFrameVideoID);
                }
            }
            /* CLOGE("INFO(%s[%d]):fcount(%d)", __FUNCTION__, __LINE__, shot_ext_dst->shot.dm.request.frameCount); */
            /* perframeFrame->dumpNodeGroupInfo("ISP__"); */
            /* m_dumpPerframeNodeGroupInfo("m_perframeIspNodeGroupInfo", m_perframeIspNodeGroupInfo); */
            /* m_dumpPerframeNodeGroupInfo("m_perframeMainNodeGroupInfo", m_perframeMainNodeGroupInfo); */
        }

        shot_ext_dst->setfile = shot_ext_src->setfile;
        shot_ext_dst->drc_bypass = shot_ext_src->drc_bypass;
        shot_ext_dst->dis_bypass = shot_ext_src->dis_bypass;
        shot_ext_dst->dnr_bypass = shot_ext_src->dnr_bypass;
        shot_ext_dst->fd_bypass = shot_ext_src->fd_bypass;
        shot_ext_dst->shot.dm.request.frameCount = shot_ext_src->shot.dm.request.frameCount;
        shot_ext_dst->shot.magicNumber= shot_ext_src->shot.magicNumber;
    }

//#ifdef SHOT_RECOVERY
    if (shot_ext_src != NULL) {

        retryGetBufferCount = shot_ext_src->complete_cnt;

        if (retryGetBufferCount > 0) {
            CLOGI("INFO(%s[%d]): ( %d %d %d %d )", __FUNCTION__, __LINE__,
                shot_ext_src->free_cnt,
                shot_ext_src->request_cnt,
                shot_ext_src->process_cnt,
                shot_ext_src->complete_cnt);
        }
    }
//#endif

    /* to set metadata of ISP buffer */
    m_activityControl->activityBeforeExecFunc(PIPE_POST_3AA_ISP, (void *)&resultBuffer);

    if (shot_ext_dst != NULL) {
        if (m_checkValidIspFrameCount(shot_ext_dst) == false) {
            CLOGE("ERR(%s[%d]):m_checkValidIspFrameCount() fail", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }
    }

    if (m_isOtf(m_node[CAPTURE_NODE]->getInput()) == true) {
        ret = m_updateMetadataToFrame(resultBuffer.addr[1], resultBuffer.index);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): updateMetadataToFrame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        }

        CLOGV("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

        /* complete frame */
        ret = m_completeFrame(&curFrame, resultBuffer);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):complete frame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            return ret;
        }

        if (curFrame == NULL) {
            CLOGE("ERR(%s[%d]):curFrame is fail", __FUNCTION__, __LINE__);
            return ret;
        }

        /* set ISP done flag to true */
        curFrame->setIspDone(true);

        /* Push to outputQ */
        if (m_outputFrameQ != NULL) {
            m_outputFrameQ->pushProcessQ(&curFrame);
        } else {
            CLOGE("ERR(%s[%d]):m_outputFrameQ is NULL", __FUNCTION__, __LINE__);
        }
    } else {
        if (ispBuffer.index < 0) {
            ALOGE("ERR(%s):ispBuffer index is invalid", __FUNCTION__);
            return BAD_VALUE;
        } else {
            ret = m_ispNode[OUTPUT_NODE]->putBuffer(&ispBuffer);
            if (ret < 0) {
                ALOGE("ERR(%s):m_ispNode[OUTPUT_NODE]->putBuffer fail ret(%d)", __FUNCTION__, ret);
                /* TODO: doing exception handling */
                return ret;
            }
        }

        m_ispBufferQ->pushProcessQ(&ispBuffer);
    }

    return NO_ERROR;
}

status_t ExynosCameraPipe3AA_ISP::m_checkShotDone(struct camera2_shot_ext *shot_ext)
{
    if (shot_ext == NULL) {
        CLOGE("ERR(%s[%d]):shot_ext is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (shot_ext->node_group.leader.request != 1) {
        CLOGW("WARN(%s[%d]): 3a1 NOT DONE, frameCount(%d)", __FUNCTION__, __LINE__,
                getMetaDmRequestFrameCount(shot_ext));
        /* TODO: doing exception handling */
        return INVALID_OPERATION;
    }

    return OK;
}

status_t ExynosCameraPipe3AA_ISP::m_getIspBuffer(void)
{
    ExynosCameraFrame *curFrame = NULL;
    ExynosCameraBuffer newBuffer;
    ExynosCameraBuffer curBuffer;
    camera2_shot_ext *shot_ext;
    int index = 0;
    int ret = 0;

    ret = m_ispBufferQ->waitAndPopProcessQ(&curBuffer);
    if (ret < 0) {
        /* TODO: We need to make timeout duration depends on FPS */
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

    ret = m_ispNode[OUTPUT_NODE]->getBuffer(&newBuffer, &index);
    if (ret != NO_ERROR || index < 0) {
        CLOGE("ERR(%s[%d]):m_ispNode[OUTPUT_NODE]->getBuffer fail ret(%d)", __FUNCTION__, __LINE__, ret);
        shot_ext = (struct camera2_shot_ext *)(newBuffer.addr[1]);
        newBuffer.index = index;
        if (shot_ext != NULL) {
            CLOGW("(%s[%d]):Shot done invalid, frame(cnt:%d, index(%d)) skip",
            __FUNCTION__, __LINE__, getMetaDmRequestFrameCount(shot_ext), index);
        }

        /* complete frame */
        ret = m_completeFrame(&curFrame, newBuffer, false);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):complete frame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: doing exception handling */
            return ret;
        }

        if (curFrame == NULL) {
            CLOGE("ERR(%s[%d]):curFrame is fail", __FUNCTION__, __LINE__);
            return ret;
        }

        /* check scp frame drop */
        curFrame->setDisDrop(true);
        curFrame->setScpDrop(true);

        /* Push to outputQ */
        if (m_outputFrameQ != NULL) {
            m_outputFrameQ->pushProcessQ(&curFrame);
        } else {
            CLOGE("ERR(%s[%d]):m_outputFrameQ is NULL", __FUNCTION__, __LINE__);
        }

        CLOGV("DEBUG(%s[%d]): -OUT-", __FUNCTION__, __LINE__);

        return NO_ERROR;

    }

    if (curBuffer.index != newBuffer.index) {
        CLOGW("ERR(%s[%d]):Frame mismatch, we expect index %d, but we got index %d",
            __FUNCTION__, __LINE__, curBuffer.index, newBuffer.index);
        /* TODO: doing exception handling */
    }

    ret = m_updateMetadataToFrame(curBuffer.addr[1], curBuffer.index);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): updateMetadataToFrame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
    }

    CLOGV("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    /* complete frame */
    ret = m_completeFrame(&curFrame, newBuffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):complete frame fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: doing exception handling */
        return ret;
    }

    if (curFrame == NULL) {
        CLOGE("ERR(%s[%d]):curFrame is fail", __FUNCTION__, __LINE__);
        return ret;
    }

    /* set ISP done flag to true */
    curFrame->setIspDone(true);

    shot_ext = (struct camera2_shot_ext *)(newBuffer.addr[1]);

    int perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_SCP_POS : PERFRAME_FRONT_SCP_POS;

    /* check scp frame drop */
    if (shot_ext == NULL) {
        CLOGW("(%s[%d]):shot_ext is NULL", __FUNCTION__, __LINE__);
    } else if (shot_ext->node_group.capture[perFramePos].request == 0) {
        curFrame->setScpDrop(true);
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

bool ExynosCameraPipe3AA_ISP::m_mainThreadFunc(void)
{
    int ret = 0;

    if (m_flagTryStop == true) {
        usleep(5000);
        return true;
    }

    /* deliver buffer from 3AA node to ISP node */
    ret = m_getBuffer();
    if (ret < 0) {
        CLOGE("ERR(%s):m_getBuffer fail", __FUNCTION__);
        /* TODO: doing exception handling */
        return true;
    }

    /* put buffer to 3AA node */
    ret = m_putBuffer();
    if (ret < 0) {
        if (ret == TIMED_OUT)
            return true;
        CLOGE("ERR(%s):m_putBuffer fail", __FUNCTION__);
        /* TODO: doing exception handling */
        return false;
    }

//#ifdef SHOT_RECOVERY
    if (retryGetBufferCount > 0) {
//#ifdef SHOT_RECOVERY_ONEBYONE
        retryGetBufferCount = 1;
//#endif
        for (int i = 0; i < retryGetBufferCount; i ++) {
            CLOGI("INFO(%s[%d]): retryGetBufferCount( %d)", __FUNCTION__, __LINE__, retryGetBufferCount);
            ret = m_getBuffer();
            if (ret < 0) {
                CLOGE("ERR(%s):m_getBuffer fail", __FUNCTION__);
                /* TODO: doing exception handling */
            }

            ret = m_putBuffer();
            if (ret < 0) {
                if (ret == TIMED_OUT)
                    return true;
                CLOGE("ERR(%s):m_putBuffer fail", __FUNCTION__);
                /* TODO: doing exception handling */
            }
        }
        retryGetBufferCount = 0;
    }
//#endif

    return true;
}

status_t ExynosCameraPipe3AA_ISP::dumpFimcIsInfo(bool bugOn)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;

    ret = m_ispNode[OUTPUT_NODE]->setControl(V4L2_CID_IS_DEBUG_DUMP, bugOn);
    if (ret != NO_ERROR)
        CLOGE("ERR(%s):m_ispNode[OUTPUT_NODE]->setControl failed", __FUNCTION__);

    return ret;
}

//#ifdef MONITOR_LOG_SYNC
status_t ExynosCameraPipe3AA_ISP::syncLog(uint32_t syncId)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;

    ret = m_ispNode[OUTPUT_NODE]->setControl(V4L2_CID_IS_DEBUG_SYNC_LOG, syncId);
    if (ret != NO_ERROR)
        CLOGE("ERR(%s):m_ispNode[OUTPUT_NODE]->setControl failed", __FUNCTION__);

    return ret;
}
//#endif

bool ExynosCameraPipe3AA_ISP::m_ispThreadFunc(void)
{
    int ret = 0;

    if (m_flagTryStop == true) {
        usleep(5000);
        return true;
    }

    /* get buffer from ISP node */
    ret = m_getIspBuffer();
    if (ret < 0) {
        if (ret == TIMED_OUT)
            return true;
        CLOGE("ERR(%s):m_getIspBuffer fail", __FUNCTION__);
        /* TODO: doing exception handling */
        return false;
    }

    m_timer.stop();
    m_timeInterval = m_timer.durationMsecs();
    m_timer.start();

    /* update renew count */
    if (ret >= 0)
        m_threadRenew = 0;

    return true;
}

void ExynosCameraPipe3AA_ISP::m_init(int32_t *ispNodeNums)
{
    memset(&m_perframeSubNodeGroupInfo, 0x00, sizeof(camera_pipe_perframe_node_group_info_t));
    memset(&m_perframeIspNodeGroupInfo, 0x00, sizeof(camera_pipe_perframe_node_group_info_t));

    memset(&m_curIspNodeGroupInfo, 0x00, sizeof(camera2_node_group));

    m_ispBufferQ = NULL;

    retryGetBufferCount = 0;

    if (ispNodeNums) {
        /* this will get info though ispNodeNum */
        for (int i = 0; i < MAX_NODE; i++)
            m_ispNodeNum[i] = ispNodeNums[i];

        m_flagIndependantIspNode = true;
    } else {
        for (int i = 0; i < MAX_NODE; i++)
            m_ispNodeNum[i] = -1;

        /*
         * m_node[OUTPUT_NODE] will be isp Node
         * we will use isp node through m_nodeNum[SUB_NODE]
         * for 54xx maintain.
         */
        m_ispNodeNum[OUTPUT_NODE] = m_nodeNum[SUB_NODE];
        m_nodeNum[SUB_NODE] = -1;

        m_flagIndependantIspNode = false;

        CLOGD("DEBUG(%s[%d]): m_ispNodeNum set By m_nodeNum", __FUNCTION__, __LINE__);

        for (int i = 0; i < MAX_NODE; i++)
            CLOGD("DEBUG(%s[%d]): m_ispNodeNum[%d] : %d / m_nodeNum[%d] : %d", __FUNCTION__, __LINE__, i, m_ispNodeNum[i], i, m_nodeNum[i]);
    }

    for (int i = 0; i < MAX_NODE; i++) {
        m_ispNode[i] = NULL;
        m_ispSensorIds[i] = -1;
    }

    m_lastIspFrameCount = 0;
}

void ExynosCameraPipe3AA_ISP:: m_copyNodeInfo2IspNodeInfo(void)
{
    /*
     * transfer node info to ispNode info.
     *
     * if constructor set ispNodeNum[i], isp all node is proper.
     * else, constructor doesn't set ispNodeNum[i], only m_ispNodeNum[OUTPUT_NODE] is proper.
     * for 54xx maintain.
     */

    if (m_flagIndependantIspNode == false) {
        m_ispSensorIds[OUTPUT_NODE] = m_sensorIds[SUB_NODE];
        m_sensorIds[SUB_NODE] = -1;

        CLOGD("DEBUG(%s[%d]): m_ispSensorIds set By m_sensorIds", __FUNCTION__, __LINE__);

        for (int i = 0; i < MAX_NODE; i++)
            CLOGD("DEBUG(%s[%d]): m_ispSensorIds[%d] : %d / m_sensorIds[%d] : %d", __FUNCTION__, __LINE__, i, m_ispSensorIds[i], i, m_sensorIds[i]);
    }
}

bool ExynosCameraPipe3AA_ISP::m_checkValidIspFrameCount(struct camera2_shot_ext * shot_ext)
{
    if (shot_ext == NULL) {
        CLOGE("ERR(%s[%d]):shot_ext == NULL. so fail", __FUNCTION__, __LINE__);
        return false;
    }

    int frameCount = getMetaDmRequestFrameCount(shot_ext);

    if (frameCount < 0 ||
        frameCount < m_lastIspFrameCount) {
        CLOGE("ERR(%s[%d]):invalid frameCount(%d) < m_lastIspFrameCount(%d). so fail",
            __FUNCTION__, __LINE__, frameCount, m_lastIspFrameCount);
        return false;
    }

    m_lastIspFrameCount = frameCount;

    return true;
}

}; /* namespace android */
