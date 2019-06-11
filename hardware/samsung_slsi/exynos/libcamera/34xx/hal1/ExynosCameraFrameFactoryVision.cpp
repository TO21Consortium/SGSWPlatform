/*
**
** Copyright 2014, Samsung Electronics Co. LTD
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
#define LOG_TAG "ExynosCameraFrameFactoryVision"
#include <cutils/log.h>

#include "ExynosCameraFrameFactoryVision.h"

namespace android {

ExynosCameraFrameFactoryVision::~ExynosCameraFrameFactoryVision()
{
    int ret = 0;

    ret = destroy();
    if (ret < 0)
        CLOGE("ERR(%s[%d]):destroy fail", __FUNCTION__, __LINE__);
}

status_t ExynosCameraFrameFactoryVision::create(__unused bool active)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    m_setupConfig();

    int ret = 0;
    int32_t nodeNums[MAX_NODE] = {-1, -1, -1};

    m_pipes[INDEX(PIPE_FLITE)] = (ExynosCameraPipe*)new ExynosCameraPipeFlite(m_cameraId, m_parameters, false, m_nodeNums[INDEX(PIPE_FLITE)]);
    m_pipes[INDEX(PIPE_FLITE)]->setPipeId(PIPE_FLITE);
    m_pipes[INDEX(PIPE_FLITE)]->setPipeName("PIPE_FLITE");

    /* flite pipe initialize */
    ret = m_pipes[INDEX(PIPE_FLITE)]->create();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    m_setCreate(true);

    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_FLITE));

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryVision::destroy(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    for (int i = 0; i < MAX_NUM_PIPES; i++) {
        if (m_pipes[i] != NULL) {
            ret = m_pipes[i]->destroy();
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_pipes[%d]->destroy() fail", __FUNCTION__, __LINE__, i);
                return ret;
            }

            SAFE_DELETE(m_pipes[i]);

            CLOGD("DEBUG(%s):Pipe(%d) destroyed", __FUNCTION__, i);
        }
    }

    m_setCreate(false);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryVision::m_fillNodeGroupInfo(__unused ExynosCameraFrame *frame)
{
    /* Do nothing */
    return NO_ERROR;
}

ExynosCameraFrame *ExynosCameraFrameFactoryVision::createNewFrame(void)
{
    int ret = 0;
    ExynosCameraFrameEntity *newEntity[MAX_NUM_PIPES];

    ExynosCameraFrame *frame = m_frameMgr->createFrame(m_parameters, m_frameCount);

    int requestEntityCount = 0;

    CLOGV("INFO(%s[%d])", __FUNCTION__, __LINE__);

    ret = m_initFrameMetadata(frame);
    if (ret < 0)
        CLOGE("(%s[%d]): frame(%d) metadata initialize fail", __FUNCTION__, __LINE__, m_frameCount);

    /* set flite pipe to linkageList */
    newEntity[INDEX(PIPE_FLITE)] = new ExynosCameraFrameEntity(PIPE_FLITE, ENTITY_TYPE_OUTPUT_ONLY, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_FLITE)]);
    requestEntityCount++;

    ret = m_initPipelines(frame);
    if (ret < 0) {
        CLOGE("ERR(%s):m_initPipelines fail, ret(%d)", __FUNCTION__, ret);
    }

    m_fillNodeGroupInfo(frame);

    /* TODO: make it dynamic */
    frame->setNumRequestPipe(requestEntityCount);

    m_frameCount++;

    return frame;
}

status_t ExynosCameraFrameFactoryVision::initPipes(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    int ret = 0;
    camera_pipe_info_t pipeInfo[3];
    int32_t nodeNums[MAX_NODE] = {-1, -1, -1};
    int32_t sensorIds[MAX_NODE] = {-1, -1, -1};

    ExynosRect tempRect;
    int maxSensorW = 0, maxSensorH = 0, hwSensorW = 0, hwSensorH = 0;
    int maxPreviewW = 0, maxPreviewH = 0, hwPreviewW = 0, hwPreviewH = 0;
    int maxPictureW = 0, maxPictureH = 0, hwPictureW = 0, hwPictureH = 0;
    int bayerFormat = V4L2_PIX_FMT_SBGGR12;
    int previewFormat = m_parameters->getHwPreviewFormat();
    int pictureFormat = m_parameters->getHwPictureFormat();

    m_parameters->getMaxSensorSize(&maxSensorW, &maxSensorH);
    m_parameters->getHwSensorSize(&hwSensorW, &hwSensorH);
    m_parameters->getMaxPreviewSize(&maxPreviewW, &maxPreviewH);
    m_parameters->getHwPreviewSize(&hwPreviewW, &hwPreviewH);
    m_parameters->getMaxPictureSize(&maxPictureW, &maxPictureH);
    m_parameters->getHwPictureSize(&hwPictureW, &hwPictureH);

    CLOGI("INFO(%s[%d]): MaxSensorSize(%dx%d), HwSensorSize(%dx%d)", __FUNCTION__, __LINE__, maxSensorW, maxSensorH, hwSensorW, hwSensorH);
    CLOGI("INFO(%s[%d]): MaxPreviewSize(%dx%d), HwPreviewSize(%dx%d)", __FUNCTION__, __LINE__, maxPreviewW, maxPreviewH, hwPreviewW, hwPreviewH);
    CLOGI("INFO(%s[%d]): MaxPixtureSize(%dx%d), HwPixtureSize(%dx%d)", __FUNCTION__, __LINE__, maxPictureW, maxPictureH, hwPictureW, hwPictureH);

    memset(pipeInfo, 0, (sizeof(camera_pipe_info_t) * 3));

    /* FLITE pipe */
#if 0
    tempRect.fullW = maxSensorW + 16;
    tempRect.fullH = maxSensorH + 10;
    tempRect.colorFormat = bayerFormat;
#else
    tempRect.fullW = VISION_WIDTH;
    tempRect.fullH = VISION_HEIGHT;
    tempRect.colorFormat = V4L2_PIX_FMT_SGRBG8;
#endif

    pipeInfo[0].rectInfo = tempRect;
    pipeInfo[0].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pipeInfo[0].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
    pipeInfo[0].bufInfo.count = FRONT_NUM_BAYER_BUFFERS;
    /* per frame info */
    pipeInfo[0].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[0].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;

    ret = m_pipes[INDEX(PIPE_FLITE)]->setupPipe(pipeInfo, m_sensorIds[INDEX(PIPE_FLITE)]);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    m_frameCount = 0;

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryVision::preparePipes(void)
{
    int ret = 0;

    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    ret = m_pipes[INDEX(PIPE_FLITE)]->prepare();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE prepare fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryVision::startPipes(void)
{
    int ret = 0;

    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    ret = m_pipes[INDEX(PIPE_FLITE)]->start();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE start fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    ret = m_pipes[INDEX(PIPE_FLITE)]->sensorStream(true);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE sensorStream on fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    CLOGI("INFO(%s[%d]):Starting Front [FLITE] Success!", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryVision::startInitialThreads(void)
{
    int ret = 0;

    CLOGI("INFO(%s[%d]):start pre-ordered initial pipe thread", __FUNCTION__, __LINE__);

    ret = startThread(PIPE_FLITE);
    if (ret < 0)
        return ret;

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryVision::stopPipes(void)
{
    int ret = 0;

    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    ret = m_pipes[INDEX(PIPE_FLITE)]->sensorStream(false);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE sensorStream fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    ret = m_pipes[INDEX(PIPE_FLITE)]->stop();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE stop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    CLOGI("INFO(%s[%d]):Stopping Front [FLITE] Success!", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

void ExynosCameraFrameFactoryVision::setRequest3AC(__unused bool enable)
{
    /* Do nothing */

    return;
}

void ExynosCameraFrameFactoryVision::m_init(void)
{
    memset(m_nodeNums, -1, sizeof(m_nodeNums));
    memset(m_sensorIds, -1, sizeof(m_sensorIds));

    /* This seems all need to set 0 */
    m_requestFLITE = 0;
    m_request3AP = 0;
    m_request3AC = 0;
    m_requestISP = 1;
    m_requestSCC = 0;
    m_requestDIS = 0;
    m_requestSCP = 1;

    m_supportReprocessing = false;
    m_flagFlite3aaOTF = false;
    m_supportSCC = false;
    m_supportPureBayerReprocessing = false;
    m_flagReprocessing = false;

}

status_t ExynosCameraFrameFactoryVision::m_setupConfig(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    int32_t *nodeNums = NULL;
    int32_t *controlId = NULL;
    int32_t *prevNode = NULL;
    bool enableVision = true;

    nodeNums = m_nodeNums[INDEX(PIPE_FLITE)];
    nodeNums[OUTPUT_NODE] = -1;
    nodeNums[CAPTURE_NODE_1] = FRONT_CAMERA_FLITE_NUM;
    nodeNums[CAPTURE_NODE_2] = -1;
    controlId = m_sensorIds[INDEX(PIPE_FLITE)];
    controlId[CAPTURE_NODE_1] = m_getSensorId(nodeNums[CAPTURE_NODE_1], enableVision);
    prevNode = nodeNums;

    return NO_ERROR;
}

}; /* namespace android */
