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
#define LOG_TAG "ExynosCameraFrameFactoryFront"
#include <cutils/log.h>

#include "ExynosCameraFrameFactoryFront.h"

namespace android {

ExynosCameraFrameFactoryFront::~ExynosCameraFrameFactoryFront()
{
    int ret = 0;

    ret = destroy();
    if (ret < 0)
        CLOGE("ERR(%s[%d]):destroy fail", __FUNCTION__, __LINE__);
}

status_t ExynosCameraFrameFactoryFront::create(__unused bool active)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    m_setupConfig();

    int ret = 0;
    int32_t nodeNums[MAX_NODE];
    for (int i = 0; i < MAX_NODE; i++)
        nodeNums[i] = -1;

    m_pipes[INDEX(PIPE_FLITE)] = (ExynosCameraPipe*)new ExynosCameraPipeFlite(m_cameraId, m_parameters, false, m_nodeNums[INDEX(PIPE_FLITE)]);
    m_pipes[INDEX(PIPE_FLITE)]->setPipeId(PIPE_FLITE);
    m_pipes[INDEX(PIPE_FLITE)]->setPipeName("PIPE_FLITE");

    m_pipes[INDEX(PIPE_3AA)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(m_cameraId, m_parameters, false, &m_deviceInfo[INDEX(PIPE_3AA)]);
    m_pipes[INDEX(PIPE_3AA)]->setPipeId(PIPE_3AA);
    m_pipes[INDEX(PIPE_3AA)]->setPipeName("PIPE_3AA");

    m_pipes[INDEX(PIPE_ISP)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(m_cameraId, m_parameters, false, &m_deviceInfo[INDEX(PIPE_ISP)]);
    m_pipes[INDEX(PIPE_ISP)]->setPipeId(PIPE_ISP);
    m_pipes[INDEX(PIPE_ISP)]->setPipeName("PIPE_ISP");

    m_pipes[INDEX(PIPE_GSC)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(m_cameraId, m_parameters, true, m_nodeNums[INDEX(PIPE_GSC)]);
    m_pipes[INDEX(PIPE_GSC)]->setPipeId(PIPE_GSC);
    m_pipes[INDEX(PIPE_GSC)]->setPipeName("PIPE_GSC");

    m_pipes[INDEX(PIPE_GSC_VIDEO)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(m_cameraId, m_parameters, false, m_nodeNums[INDEX(PIPE_GSC_VIDEO)]);
    m_pipes[INDEX(PIPE_GSC_VIDEO)]->setPipeId(PIPE_GSC_VIDEO);
    m_pipes[INDEX(PIPE_GSC_VIDEO)]->setPipeName("PIPE_GSC_VIDEO");

    if (m_supportReprocessing == false) {
        m_pipes[INDEX(PIPE_GSC_PICTURE)] = (ExynosCameraPipe*)new ExynosCameraPipeGSC(m_cameraId, m_parameters, true, m_nodeNums[INDEX(PIPE_GSC_PICTURE)]);
        m_pipes[INDEX(PIPE_GSC_PICTURE)]->setPipeId(PIPE_GSC_PICTURE);
        m_pipes[INDEX(PIPE_GSC_PICTURE)]->setPipeName("PIPE_GSC_PICTURE");

        m_pipes[INDEX(PIPE_JPEG)] = (ExynosCameraPipe*)new ExynosCameraPipeJpeg(m_cameraId, m_parameters, true, m_nodeNums[INDEX(PIPE_JPEG)]);
        m_pipes[INDEX(PIPE_JPEG)]->setPipeId(PIPE_JPEG);
        m_pipes[INDEX(PIPE_JPEG)]->setPipeName("PIPE_JPEG");
    }

    /* flite pipe initialize */
    ret = m_pipes[INDEX(PIPE_FLITE)]->create(m_sensorIds[INDEX(PIPE_FLITE)]);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_FLITE));

    /* ISP pipe initialize */
    ret = m_pipes[INDEX(PIPE_ISP)]->create();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):ISP create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_ISP));

    /* 3AA pipe initialize */
    ret = m_pipes[INDEX(PIPE_3AA)]->create();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):3AA create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_3AA));

    /* GSC_PREVIEW pipe initialize */
    ret = m_pipes[INDEX(PIPE_GSC)]->create();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):GSC create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_GSC));

    ret = m_pipes[INDEX(PIPE_GSC_VIDEO)]->create();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):PIPE_GSC_VIDEO create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }
    CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_GSC_VIDEO));

    if (m_supportReprocessing == false) {
        /* GSC_PICTURE pipe initialize */
        ret = m_pipes[INDEX(PIPE_GSC_PICTURE)]->create();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):GSC_PICTURE create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
        CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_GSC_PICTURE));

        /* JPEG pipe initialize */
        ret = m_pipes[INDEX(PIPE_JPEG)]->create();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):JPEG create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
        CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_JPEG));
    }

    /* EOS */
    ret = m_pipes[INDEX(PIPE_3AA)]->setControl(V4L2_CID_IS_END_OF_STREAM, 1);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):PIPE_%d V4L2_CID_IS_END_OF_STREAM fail, ret(%d)", __FUNCTION__, __LINE__, PIPE_3AA, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    m_setCreate(true);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryFront::m_fillNodeGroupInfo(ExynosCameraFrame *frame)
{
    camera2_node_group node_group_info_3aa, node_group_info_isp, node_group_info_dis;
    int zoom = m_parameters->getZoomLevel();
    int previewW = 0, previewH = 0;
    int pictureW = 0, pictureH = 0;
    ExynosRect bnsSize;       /* == bayerCropInputSize */
    ExynosRect bayerCropSize;
    ExynosRect bdsSize;
    int perFramePos = 0;
    bool tpu = false;
    bool dual = true;

    m_parameters->getHwPreviewSize(&previewW, &previewH);
    m_parameters->getPictureSize(&pictureW, &pictureH);
    m_parameters->getPreviewBayerCropSize(&bnsSize, &bayerCropSize);
    m_parameters->getPreviewBdsSize(&bdsSize);

    memset(&node_group_info_3aa, 0x0, sizeof(camera2_node_group));
    memset(&node_group_info_isp, 0x0, sizeof(camera2_node_group));
    memset(&node_group_info_dis, 0x0, sizeof(camera2_node_group));

    /* should add this request value in FrameFactory */
    /* 3AA */
    node_group_info_3aa.leader.request = 1;

    /* 3AC */
    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_3AC_POS : PERFRAME_FRONT_3AC_POS;
    node_group_info_3aa.capture[perFramePos].request = frame->getRequest(PIPE_3AC);

    /* 3AP */
    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_3AP_POS : PERFRAME_FRONT_3AP_POS;
    node_group_info_3aa.capture[perFramePos].request = frame->getRequest(PIPE_3AP);

    /* should add this request value in FrameFactory */
    /* ISP */
    node_group_info_isp.leader.request = 1;

    /* SCC */
    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_SCC_POS : PERFRAME_FRONT_SCC_POS;

    if (m_supportSCC == true)
        node_group_info_isp.capture[perFramePos].request = frame->getRequest(PIPE_SCC);
    else
        node_group_info_isp.capture[perFramePos].request = frame->getRequest(PIPE_ISPC);

    memcpy(&node_group_info_dis, &node_group_info_isp, sizeof (camera2_node_group));

    if (m_requestISPC == true) {
        perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_ISPC_POS : PERFRAME_FRONT_ISPC_POS;
        node_group_info_3aa.capture[perFramePos].request = frame->getRequest(PIPE_ISPC);
    }

    ExynosCameraNodeGroup3AA::updateNodeGroupInfo(
        m_cameraId,
        &node_group_info_3aa,
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
        tpu);

    ExynosCameraNodeGroupDIS::updateNodeGroupInfo(
        m_cameraId,
        &node_group_info_dis,
        bayerCropSize,
        bdsSize,
        previewW, previewH,
        pictureW, pictureH);

    if (m_requestISPC == true) {
        perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_SCP_POS : PERFRAME_FRONT_SCP_POS;
        for(int i=0 ; i < 4 ; i++) {
            node_group_info_3aa.capture[PERFRAME_FRONT_ISPC_POS].input.cropRegion[i] = node_group_info_3aa.capture[perFramePos].input.cropRegion[i];
            node_group_info_3aa.capture[PERFRAME_FRONT_ISPC_POS].output.cropRegion[i] = node_group_info_3aa.capture[perFramePos].input.cropRegion[i];
        }
    }

    frame->storeNodeGroupInfo(&node_group_info_3aa, PERFRAME_INFO_3AA, zoom);
    frame->storeNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_ISP, zoom);
    frame->storeNodeGroupInfo(&node_group_info_dis, PERFRAME_INFO_DIS, zoom);

    return NO_ERROR;
}

ExynosCameraFrame *ExynosCameraFrameFactoryFront::createNewFrame(void)
{
    int ret = 0;
    ExynosCameraFrameEntity *newEntity[MAX_NUM_PIPES] = {0};
    ExynosCameraFrame *frame = m_frameMgr->createFrame(m_parameters, m_frameCount, FRAME_TYPE_PREVIEW);

    int requestEntityCount = 0;
    bool dzoomScaler = false;
    dzoomScaler = m_parameters->getZoomPreviewWIthScaler();

    ret = m_initFrameMetadata(frame);
    if (ret < 0)
        CLOGE("(%s[%d]): frame(%d) metadata initialize fail", __FUNCTION__, __LINE__, m_frameCount);

    if (m_requestFLITE) {
        /* set flite pipe to linkageList */
        newEntity[INDEX(PIPE_FLITE)] = new ExynosCameraFrameEntity(PIPE_FLITE, ENTITY_TYPE_OUTPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_FLITE)]);
        requestEntityCount++;
    }

    /* set 3AA_ISP pipe to linkageList */
    newEntity[INDEX(PIPE_3AA)] = new ExynosCameraFrameEntity(PIPE_3AA, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_3AA)]);
    requestEntityCount++;

    if (m_supportReprocessing == false) {
        /* set GSC-Picture pipe to linkageList */
        newEntity[INDEX(PIPE_GSC_PICTURE)] = new ExynosCameraFrameEntity(PIPE_GSC_PICTURE, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_GSC_PICTURE)]);
    }

    /* set GSC pipe to linkageList */
    newEntity[INDEX(PIPE_GSC)] = new ExynosCameraFrameEntity(PIPE_GSC, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_GSC)]);
    if (dzoomScaler) {
        requestEntityCount++;
    }

    newEntity[INDEX(PIPE_GSC_VIDEO)] = new ExynosCameraFrameEntity(PIPE_GSC_VIDEO, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_GSC_VIDEO)]);

    if (m_supportReprocessing == false) {
        /* set JPEG pipe to linkageList */
        newEntity[INDEX(PIPE_JPEG)] = new ExynosCameraFrameEntity(PIPE_JPEG, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_JPEG)]);
    }

    ret = m_initPipelines(frame);
    if (ret < 0) {
        CLOGE("ERR(%s):m_initPipelines fail, ret(%d)", __FUNCTION__, ret);
    }

    /* TODO: make it dynamic */
    frame->setNumRequestPipe(requestEntityCount);

    m_fillNodeGroupInfo(frame);

    m_frameCount++;
    return frame;
}

status_t ExynosCameraFrameFactoryFront::initPipes(void)
{
    CLOGI("INFO(%s[%d]) IN", __FUNCTION__, __LINE__);

    int ret = 0;
    camera_pipe_info_t pipeInfo[MAX_NODE];
    camera_pipe_info_t nullPipeInfo;

    int32_t nodeNums[MAX_NODE];
    int32_t sensorIds[MAX_NODE];
    int32_t secondarySensorIds[MAX_NODE];
    for (int i = 0; i < MAX_NODE; i++) {
        nodeNums[i] = -1;
        sensorIds[i] = -1;
        secondarySensorIds[i] = -1;
    }

    ExynosRect tempRect;
    int maxSensorW = 0, maxSensorH = 0, hwSensorW = 0, hwSensorH = 0;
    int maxPreviewW = 0, maxPreviewH = 0, hwPreviewW = 0, hwPreviewH = 0;
    int maxPictureW = 0, maxPictureH = 0, hwPictureW = 0, hwPictureH = 0;
    int bayerFormat = CAMERA_BAYER_FORMAT;
    int previewFormat = m_parameters->getHwPreviewFormat();
    int pictureFormat = m_parameters->getHwPictureFormat();
    int hwVdisformat = m_parameters->getHWVdisFormat();
    bool hwVdis = m_parameters->getTpuEnabledMode();
    struct ExynosConfigInfo *config = m_parameters->getConfig();
    ExynosRect bdsSize;
    int perFramePos = 0;
    int stride = 0;

#ifdef DEBUG_RAWDUMP
    if (m_parameters->checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    m_parameters->getMaxSensorSize(&maxSensorW, &maxSensorH);
    m_parameters->getHwSensorSize(&hwSensorW, &hwSensorH);

    m_parameters->getMaxPreviewSize(&maxPreviewW, &maxPreviewH);
    m_parameters->getHwPreviewSize(&hwPreviewW, &hwPreviewH);

    m_parameters->getMaxPictureSize(&maxPictureW, &maxPictureH);
    m_parameters->getHwPictureSize(&hwPictureW, &hwPictureH);

    m_parameters->getPreviewBdsSize(&bdsSize);

    /* When high speed recording mode, hw sensor size is fixed.
     * So, maxPreview size cannot exceed hw sensor size
     */
    if (m_parameters->getHighSpeedRecording()) {
        maxPreviewW = hwSensorW;
        maxPreviewH = hwSensorH;
    }

    CLOGI("INFO(%s[%d]): MaxSensorSize(%dx%d), HWSensorSize(%dx%d)", __FUNCTION__, __LINE__, maxSensorW, maxSensorH, hwSensorW, hwSensorH);
    CLOGI("INFO(%s[%d]): MaxPreviewSize(%dx%d), HwPreviewSize(%dx%d)", __FUNCTION__, __LINE__, maxPreviewW, maxPreviewH, hwPreviewW, hwPreviewH);
    CLOGI("INFO(%s[%d]): HWPictureSize(%dx%d)", __FUNCTION__, __LINE__, hwPictureW, hwPictureH);



#ifdef DEBUG_RAWDUMP
    if (m_parameters->checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    m_parameters->getMaxSensorSize(&maxSensorW, &maxSensorH);
    m_parameters->getHwSensorSize(&hwSensorW, &hwSensorH);

    CLOGI("INFO(%s[%d]): MaxSensorSize(%dx%d), HWSensorSize(%dx%d)", __FUNCTION__, __LINE__, maxSensorW, maxSensorH, hwSensorW, hwSensorH);

    /* FLITE pipe */
    for (int i = 0; i < MAX_NODE; i++)
        pipeInfo[i] = nullPipeInfo;

    /* setParam for Frame rate : must after setInput on Flite */
    uint32_t min, max, frameRate;
    struct v4l2_streamparm streamParam;

    memset(&streamParam, 0x0, sizeof(v4l2_streamparm));
    m_parameters->getPreviewFpsRange(&min, &max);

    if (m_parameters->getScalableSensorMode() == true)
        frameRate = 24;
    else
        frameRate = max;

    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    streamParam.parm.capture.timeperframe.numerator   = 1;
    streamParam.parm.capture.timeperframe.denominator = frameRate;
    CLOGI("INFO(%s[%d]:set framerate (denominator=%d)", __FUNCTION__, __LINE__, frameRate);
    ret = setParam(&streamParam, PIPE_FLITE);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE setParam fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return INVALID_OPERATION;
    }

#ifdef FIXED_SENSOR_SIZE
    tempRect.fullW = maxSensorW;
    tempRect.fullH = maxSensorH;
#else
    tempRect.fullW = hwSensorW;
    tempRect.fullH = hwSensorH;
#endif
    tempRect.colorFormat = bayerFormat;

    pipeInfo[0].rectInfo = tempRect;
    pipeInfo[0].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pipeInfo[0].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
    pipeInfo[0].bufInfo.count = config->current->bufInfo.num_bayer_buffers;
    /* per frame info */
    pipeInfo[0].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[0].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;

#ifdef CAMERA_PACKED_BAYER_ENABLE
#ifdef DEBUG_RAWDUMP
    if (m_parameters->checkBayerDumpEnable()) {
        /* packed bayer bytesPerPlane */
        pipeInfo[0].bytesPerPlane[0] = ROUND_UP(pipeInfo[0].rectInfo.fullW, 10) * 2;
    }
    else
#endif
    {
        /* packed bayer bytesPerPlane */
        pipeInfo[0].bytesPerPlane[0] = ROUND_UP(pipeInfo[0].rectInfo.fullW, 10) * 8 / 5;
    }
#endif

    ret = m_pipes[INDEX(PIPE_FLITE)]->setupPipe(pipeInfo, m_sensorIds[INDEX(PIPE_FLITE)]);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    /* set BNS ratio */

    int bnsScaleRatio = 0;
    int bnsSize = 0;
    if( m_parameters->getHighSpeedRecording()
#ifdef USE_BINNING_MODE
        || m_parameters->getBinningMode()
#endif
    ) {
        bnsScaleRatio = 1000;
    } else {
        bnsScaleRatio = m_parameters->getBnsScaleRatio();
    }
    ret = m_pipes[INDEX(PIPE_FLITE)]->setControl(V4L2_CID_IS_S_BNS, bnsScaleRatio);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): set BNS(%d) fail, ret(%d)", __FUNCTION__, __LINE__, bnsScaleRatio, ret);
    } else {
        ret = m_pipes[INDEX(PIPE_FLITE)]->getControl(V4L2_CID_IS_G_BNS_SIZE, &bnsSize);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): get BNS size fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            bnsSize = -1;
        }
    }

    int bnsWidth = 0;
    int bnsHeight = 0;
    if (bnsSize > 0) {
        bnsHeight = bnsSize & 0xffff;
        bnsWidth = bnsSize >> 16;

        CLOGI("INFO(%s[%d]): BNS scale down ratio(%.1f), size (%dx%d)", __FUNCTION__, __LINE__, (float)(bnsScaleRatio / 1000), bnsWidth, bnsHeight);
        m_parameters->setBnsSize(bnsWidth, bnsHeight);
    }

    /* 3AS */
    enum NODE_TYPE t3asNodeType = getNodeType(PIPE_3AA);

    for (int i = 0; i < MAX_NODE; i++)
        pipeInfo[i] = nullPipeInfo;

    for (int i = 0; i < MAX_NODE; i++) {
        ret = m_pipes[INDEX(PIPE_3AA)]->setPipeId((enum NODE_TYPE)i, m_deviceInfo[PIPE_3AA].pipeId[i]);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):setPipeId(%d, %d) fail, ret(%d)", __FUNCTION__, __LINE__, i, m_deviceInfo[PIPE_3AA].pipeId[i], ret);
            return ret;
        }

        sensorIds[i] = m_sensorIds[INDEX(PIPE_3AA)][i];
        secondarySensorIds[i] = m_secondarySensorIds[INDEX(PIPE_3AA)][i];
    }

    if (m_flagFlite3aaOTF == true) {
        tempRect.fullW = 32;
        tempRect.fullH = 64;
        tempRect.colorFormat = bayerFormat;

        pipeInfo[t3asNodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;
    } else {
        tempRect.fullW = maxSensorW;
        tempRect.fullH = maxSensorH;
        tempRect.colorFormat = bayerFormat;

#ifdef CAMERA_PACKED_BAYER_ENABLE
#ifdef DEBUG_RAWDUMP
        if (m_parameters->checkBayerDumpEnable()) {
            /* packed bayer bytesPerPlane */
            pipeInfo[t3asNodeType].bytesPerPlane[0] = ROUND_UP(tempRect.fullW, 10) * 2;
        }
        else
#endif
        {
            /* packed bayer bytesPerPlane */
            pipeInfo[t3asNodeType].bytesPerPlane[0] = ROUND_UP(tempRect.fullW, 10) * 8 / 5;
        }
#endif

        pipeInfo[t3asNodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;
    }

    pipeInfo[t3asNodeType].rectInfo = tempRect;
    pipeInfo[t3asNodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    pipeInfo[t3asNodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;

    /* per frame info */
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = CAPTURE_NODE_MAX;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex = PERFRAME_INFO_3AA;

    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_LEADER;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].nodeNum[t3asNodeType] - FIMC_IS_VIDEO_BAS_NUM);

    /* 3AP */
    enum NODE_TYPE t3apNodeType = getNodeType(PIPE_3AP);

    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_3AP_POS : PERFRAME_FRONT_3AP_POS;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameNodeType = PERFRAME_NODE_TYPE_CAPTURE;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].secondaryNodeNum[t3apNodeType] - FIMC_IS_VIDEO_BAS_NUM);
#if 1
    tempRect.fullW = maxSensorW;
    tempRect.fullH = maxSensorH;
    tempRect.colorFormat = bayerFormat;
    pipeInfo[t3apNodeType].rectInfo = tempRect;
    pipeInfo[t3apNodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    pipeInfo[t3apNodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
#endif
    pipeInfo[t3apNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[t3apNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;

    /* ISPS */
    enum NODE_TYPE ispsNodeType = getNodeType(PIPE_ISP);

    tempRect.fullW = bdsSize.w;
    tempRect.fullH = bdsSize.h;
    tempRect.colorFormat = bayerFormat;
#ifdef CAMERA_PACKED_BAYER_ENABLE
#ifdef DEBUG_RAWDUMP
    if (m_parameters->checkBayerDumpEnable()) {
        /* packed bayer bytesPerPlane */
        pipeInfo[ispsNodeType].bytesPerPlane[0] = ROUND_UP(tempRect.fullW * 2, 16);
    }
    else
#endif
    {
        /* packed bayer bytesPerPlane */
        pipeInfo[ispsNodeType].bytesPerPlane[0] = ROUND_UP(tempRect.fullW * 3 / 2, 16);
    }
#endif

    pipeInfo[ispsNodeType].rectInfo = tempRect;
    pipeInfo[ispsNodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    pipeInfo[ispsNodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
    pipeInfo[ispsNodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;

    pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex = PERFRAME_INFO_ISP;
    pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_LEADER;
    pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].secondaryNodeNum[ispsNodeType] - FIMC_IS_VIDEO_BAS_NUM);

    /* ISPC */
    enum NODE_TYPE ispcNodeType = getNodeType(PIPE_ISPC);

    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_ISPC_POS : PERFRAME_FRONT_ISPC_POS;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameNodeType = PERFRAME_NODE_TYPE_CAPTURE;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameVideoID = (m_deviceInfo[PIPE_3AA].nodeNum[ispcNodeType] - FIMC_IS_VIDEO_BAS_NUM);

    tempRect.fullW = bdsSize.w;
    tempRect.fullH = bdsSize.h;
    tempRect.colorFormat = hwVdisformat;

#ifdef USE_BUFFER_WITH_STRIDE
        /* to use stride for preview buffer, set the bytesPerPlane */
        pipeInfo[ispcNodeType].bytesPerPlane[0] = bdsSize.w;
#endif

    pipeInfo[ispcNodeType].rectInfo = tempRect;
    pipeInfo[ispcNodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pipeInfo[ispcNodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
    pipeInfo[ispcNodeType].bufInfo.count = config->current->bufInfo.num_hwdis_buffers;

    pipeInfo[ispcNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[ispcNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;

    ret = m_pipes[INDEX(PIPE_3AA)]->setupPipe(pipeInfo, sensorIds, secondarySensorIds);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):3AA setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    CLOGI("INFO(%s[%d]) OUT", __FUNCTION__, __LINE__);
    m_frameCount = 0;

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryFront::preparePipes(void)
{
    int ret = 0;

    /* NOTE: Prepare for 3AA is moved after ISP stream on */

    if (m_requestFLITE) {
        ret = m_pipes[INDEX(PIPE_FLITE)]->prepare();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):FLITE prepare fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_supportSCC == true) {
        enum pipeline pipe = (m_supportSCC == true) ? PIPE_SCC : PIPE_ISPC;

        ret = m_pipes[INDEX(pipe)]->prepare();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):%s prepare fail, ret(%d)", __FUNCTION__, __LINE__, m_pipes[INDEX(pipe)]->getPipeName(), ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryFront::startPipes(void)
{
    int ret = 0;

    if (m_supportSCC == true) {
        enum pipeline pipe = (m_supportSCC == true) ? PIPE_SCC : PIPE_ISPC;

        ret = m_pipes[INDEX(pipe)]->start();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):%s start fail, ret(%d)", __FUNCTION__, __LINE__, m_pipes[INDEX(pipe)]->getPipeName(), ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_flag3aaIspOTF == false) {
        ret = m_pipes[INDEX(PIPE_ISP)]->start();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):ISP start fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    ret = m_pipes[INDEX(PIPE_3AA)]->start();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):3AA start fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    ret = m_pipes[INDEX(PIPE_FLITE)]->start();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE start fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    if (m_flagFlite3aaOTF == true) {
        /* Here is doing 3AA prepare(qbuf) */
        ret = m_pipes[INDEX(PIPE_3AA)]->prepare();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):3AA prepare fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    ret = m_pipes[INDEX(PIPE_FLITE)]->sensorStream(true);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE sensorStream on fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    CLOGI("INFO(%s[%d]):Starting Success!", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryFront::startInitialThreads(void)
{
    int ret = 0;

    CLOGI("INFO(%s[%d]):start pre-ordered initial pipe thread", __FUNCTION__, __LINE__);

    if (m_requestFLITE) {
        ret = startThread(PIPE_FLITE);
        if (ret < 0)
            return ret;
    }

    ret = startThread(PIPE_3AA);
    if (ret < 0)
        return ret;

    if (m_flag3aaIspOTF == false) {
        ret = startThread(PIPE_ISP);
        if (ret < 0)
            return ret;
    }

    if (m_parameters->getTpuEnabledMode() == true) {
        ret = startThread(PIPE_DIS);
        if (ret < 0)
            return ret;
    }

    if(m_supportSCC) {
        enum pipeline pipe = (m_supportSCC == true) ? PIPE_SCC : PIPE_ISPC;

        ret = startThread(pipe);
        if (ret < 0)
            return ret;
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryFront::setStopFlag(void)
{
    CLOGI("INFO(%s[%d]):", __FUNCTION__, __LINE__);

    int ret = 0;

    ret = m_pipes[INDEX(PIPE_FLITE)]->setStopFlag();

    if (m_pipes[INDEX(PIPE_3AA)]->flagStart() == true)
        ret = m_pipes[INDEX(PIPE_3AA)]->setStopFlag();

    if (m_pipes[INDEX(PIPE_ISP)]->flagStart() == true)
        ret = m_pipes[INDEX(PIPE_ISP)]->setStopFlag();

    if (m_supportSCC == true) {
        enum pipeline pipe = (m_supportSCC == true) ? PIPE_SCC : PIPE_ISPC;

        ret = m_pipes[INDEX(pipe)]->setStopFlag();
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryFront::stopPipes(void)
{
    int ret = 0;
    if (m_supportSCC == true) {
        enum pipeline pipe = (m_supportSCC == true) ? PIPE_SCC : PIPE_ISPC;

        ret = m_pipes[INDEX(pipe)]->stopThread();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):%s stopThread fail, ret(%d)", __FUNCTION__, __LINE__, m_pipes[INDEX(pipe)]->getPipeName(), ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_pipes[INDEX(PIPE_3AA)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_3AA)]->stopThread();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):3AA stopThread fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    /* stream off for ISP */
    if (m_pipes[INDEX(PIPE_ISP)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_ISP)]->stopThread();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):ISP stopThread fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_requestFLITE) {
        ret = m_pipes[INDEX(PIPE_FLITE)]->stopThread();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):FLITE stopThread fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_pipes[INDEX(PIPE_GSC)]->isThreadRunning() == true) {
        ret = stopThread(INDEX(PIPE_GSC));
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):PIPE_GSC stopThread fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return INVALID_OPERATION;
        }
    }
    ret = m_pipes[INDEX(PIPE_FLITE)]->sensorStream(false);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE sensorStream off fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    ret = m_pipes[INDEX(PIPE_FLITE)]->stop();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):FLITE stop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    /* 3AA force done */
    ret = m_pipes[INDEX(PIPE_3AA)]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):PIPE_3AA force done fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        /* return INVALID_OPERATION; */
    }

    /* stream off for 3AA */
    ret = m_pipes[INDEX(PIPE_3AA)]->stop();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):3AA stop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    /* ISP force done */
    if (m_pipes[INDEX(PIPE_ISP)]->flagStart() == true) {
        ret = m_pipes[INDEX(PIPE_ISP)]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):PIPE_ISP force done fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            /* return INVALID_OPERATION; */
        }

        /* stream off for ISP */
        ret = m_pipes[INDEX(PIPE_ISP)]->stop();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):ISP stop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_supportSCC == true) {
        enum pipeline pipe = (m_supportSCC == true) ? PIPE_SCC : PIPE_ISPC;

        ret = m_pipes[INDEX(pipe)]->stop();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):%s stop fail, ret(%d)", __FUNCTION__, __LINE__, m_pipes[INDEX(pipe)]->getPipeName(), ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    ret = stopThreadAndWait(INDEX(PIPE_GSC));
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):PIPE_GSC stopThreadAndWait fail, ret(%d)", __FUNCTION__, __LINE__, ret);
    }
    CLOGI("INFO(%s[%d]):Stopping  Success!", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

void ExynosCameraFrameFactoryFront::m_init(void)
{
    m_supportReprocessing = false;
    m_flagFlite3aaOTF = false;
    m_supportSCC = false;
    m_supportPureBayerReprocessing = false;
    m_flagReprocessing = false;
    m_requestISP = 0;
}

status_t ExynosCameraFrameFactoryFront::m_setupConfig()
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    status_t ret = NO_ERROR;

    int32_t *nodeNums = NULL;
    int32_t *controlId = NULL;
    int32_t *secondaryControlId = NULL;
    int32_t *prevNode = NULL;

    enum NODE_TYPE nodeType = INVALID_NODE;
    int pipeId = -1;

    int t3aaNums[MAX_NODE];
    int ispNums[MAX_NODE];

    m_flagFlite3aaOTF = (m_cameraId == CAMERA_ID_BACK)?MAIN_CAMERA_DUAL_FLITE_3AA_OTF:FRONT_CAMERA_DUAL_FLITE_3AA_OTF;
    m_flag3aaIspOTF = (m_cameraId == CAMERA_ID_BACK)?MAIN_CAMERA_DUAL_3AA_ISP_OTF:FRONT_CAMERA_DUAL_3AA_ISP_OTF;
    m_supportReprocessing = m_parameters->isReprocessing();
    m_supportSCC = m_parameters->isOwnScc(m_cameraId);
    m_supportPureBayerReprocessing = (m_cameraId == CAMERA_ID_BACK) ? USE_PURE_BAYER_REPROCESSING_ON_DUAL : USE_PURE_BAYER_REPROCESSING_FRONT_ON_DUAL;

    m_flagReprocessing = false;

    if (m_supportReprocessing == false) {
        if (m_supportSCC == true)
            m_requestSCC = 1;
        else
            m_requestISPC = 1;
    }

    if (m_flag3aaIspOTF == true)
        m_request3AP = 0;
    else
        m_request3AP = 1;

    nodeNums = m_nodeNums[INDEX(PIPE_FLITE)];
    nodeNums[OUTPUT_NODE] = -1;
    nodeNums[CAPTURE_NODE_1] = (m_cameraId == CAMERA_ID_BACK) ? MAIN_CAMERA_FLITE_NUM : FRONT_CAMERA_FLITE_NUM;
    nodeNums[CAPTURE_NODE_2] = -1;
    controlId = m_sensorIds[INDEX(PIPE_FLITE)];
    controlId[CAPTURE_NODE_1] = m_getSensorId(nodeNums[CAPTURE_NODE_1], m_flagReprocessing);
    prevNode = nodeNums;

#if 1
    t3aaNums[OUTPUT_NODE]    = FIMC_IS_VIDEO_30S_NUM;
    t3aaNums[CAPTURE_NODE_1] = -1;
    t3aaNums[CAPTURE_NODE_2] = FIMC_IS_VIDEO_30P_NUM;
#else
    t3aaNums[OUTPUT_NODE]    = FIMC_IS_VIDEO_31S_NUM;
    t3aaNums[CAPTURE_NODE_1] = -1;
    t3aaNums[CAPTURE_NODE_2] = FIMC_IS_VIDEO_31P_NUM;
#endif
    ispNums[OUTPUT_NODE]    = FIMC_IS_VIDEO_I1S_NUM;
    ispNums[CAPTURE_NODE_1] = -1;
    ispNums[CAPTURE_NODE_2] = FIMC_IS_VIDEO_I1C_NUM;

    /* 1. 3AAS */
    pipeId = INDEX(PIPE_3AA);
    nodeType = getNodeType(PIPE_3AA);
    m_deviceInfo[pipeId].nodeNum[nodeType] = t3aaNums[OUTPUT_NODE];
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "3AA_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType]  = m_getSensorId(m_nodeNums[INDEX(PIPE_FLITE)][getNodeType(PIPE_FLITE)], m_flagFlite3aaOTF, true, m_flagReprocessing);
    m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_3AA;


    /* 2. 3AAP */
    nodeType = getNodeType(PIPE_3AP);
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = t3aaNums[CAPTURE_NODE_2];
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "3AA_PREVIEW", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_secondarySensorIds[pipeId][nodeType] = -1;
    m_secondarySensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA)], true, false, m_flagReprocessing);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AP;

    /* 3. ISPS */
    nodeType = getNodeType(PIPE_ISP);
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = ispNums[OUTPUT_NODE];
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "ISP_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_secondarySensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].secondaryNodeNum[getNodeType(PIPE_3AP)], m_flag3aaIspOTF, false, m_flagReprocessing);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_ISP;

    // ISPC
    nodeType = getNodeType(PIPE_ISPC);
    m_deviceInfo[pipeId].nodeNum[nodeType] = ispNums[CAPTURE_NODE_2];
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "ISP_PREVIEW", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType]  = -1;
    m_sensorIds[pipeId][nodeType]  = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_ISP)], true, false, m_flagReprocessing);
    m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_ISPC;

    nodeNums = m_nodeNums[INDEX(PIPE_GSC)];
    nodeNums[OUTPUT_NODE] = PREVIEW_GSC_NODE_NUM;
    nodeNums[CAPTURE_NODE_1] = -1;
    nodeNums[CAPTURE_NODE_2] = -1;

    nodeNums = m_nodeNums[INDEX(PIPE_GSC_VIDEO)];
    nodeNums[OUTPUT_NODE] = VIDEO_GSC_NODE_NUM;
    nodeNums[CAPTURE_NODE_1] = -1;
    nodeNums[CAPTURE_NODE_2] = -1;

    nodeNums = m_nodeNums[INDEX(PIPE_GSC_PICTURE)];
    nodeNums[OUTPUT_NODE] = PICTURE_GSC_NODE_NUM;
    nodeNums[CAPTURE_NODE_1] = -1;
    nodeNums[CAPTURE_NODE_2] = -1;

    nodeNums = m_nodeNums[INDEX(PIPE_JPEG)];
    nodeNums[OUTPUT_NODE] = -1;
    nodeNums[CAPTURE_NODE_1] = -1;
    nodeNums[CAPTURE_NODE_2] = -1;

    for (int i = 0; i < MAX_NODE; i++)
        m_nodeNums[pipeId][i] = m_deviceInfo[pipeId].nodeNum[i];

    if (m_checkNodeSetting(pipeId) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_checkNodeSetting(%d) fail", __FUNCTION__, __LINE__, pipeId);
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

enum NODE_TYPE  ExynosCameraFrameFactoryFront::getNodeType(uint32_t pipeId)
{
    enum NODE_TYPE nodeType = INVALID_NODE;

    switch (pipeId) {
    case PIPE_FLITE:
        nodeType = CAPTURE_NODE_1;
        break;
    case PIPE_3AA:
        nodeType = OUTPUT_NODE;
        break;
    case PIPE_ISPC:
        nodeType = CAPTURE_NODE_1;
        break;
    case PIPE_3AP:
        nodeType = CAPTURE_NODE_2;
        break;
    case PIPE_3AC:
        nodeType = CAPTURE_NODE_3;
        break;
    case PIPE_ISP:
        nodeType = OTF_NODE_1;
        break;
    case PIPE_ISPP:
        nodeType = CAPTURE_NODE_5;
        break;
    case PIPE_JPEG:
        nodeType = CAPTURE_NODE_6;
        break;
    case PIPE_SCP:
        nodeType = CAPTURE_NODE_7;
        break;
    default:
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Unexpected pipe_id(%d), assert!!!!",
            __FUNCTION__, __LINE__, pipeId);
        break;
    }

    return nodeType;
}

}; /* namespace android */
