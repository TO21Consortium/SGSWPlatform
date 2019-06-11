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
#define LOG_TAG "ExynosCameraFrameFactory3aaIspOtf"
#include <cutils/log.h>

#include "ExynosCameraFrameFactory3aaIspOtf.h"

namespace android {

ExynosCameraFrameFactory3aaIspOtf::~ExynosCameraFrameFactory3aaIspOtf()
{
    int ret = 0;

    ret = destroy();
    if (ret < 0)
        CLOGE("ERR(%s[%d]):destroy fail", __FUNCTION__, __LINE__);
}

enum NODE_TYPE ExynosCameraFrameFactory3aaIspOtf::getNodeType(uint32_t pipeId)
{
    enum NODE_TYPE nodeType = INVALID_NODE;

    switch (pipeId) {
    case PIPE_FLITE:
        nodeType = CAPTURE_NODE_1;
        break;
    case PIPE_3AA:
        nodeType = OUTPUT_NODE;
        break;
    case PIPE_3AC:
        nodeType = CAPTURE_NODE_1;
        break;
    case PIPE_3AP:
        nodeType = OTF_NODE_1;
        break;
    case PIPE_ISP:
        nodeType = OTF_NODE_2;
        break;
    case PIPE_ISPP:
        nodeType = OTF_NODE_3;
        break;
    case PIPE_DIS:
        nodeType = OTF_NODE_4;
        break;
    case PIPE_MCSC:
        nodeType = OTF_NODE_5;
        break;
    case PIPE_ISPC:
    case PIPE_SCC:
    case PIPE_JPEG:
        nodeType = CAPTURE_NODE_6;
        break;
    case PIPE_SCP:
        nodeType = CAPTURE_NODE_7;
        break;
    case PIPE_VRA:
        nodeType = OUTPUT_NODE;
        break;
    default:
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Unexpected pipe_id(%d), assert!!!!",
            __FUNCTION__, __LINE__, pipeId);
        break;
    }

    return nodeType;
}

status_t ExynosCameraFrameFactory3aaIspOtf::m_setDeviceInfo(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    bool flagDirtyBayer = false;

    if (m_supportReprocessing == true && m_supportPureBayerReprocessing == false)
        flagDirtyBayer = true;

    int pipeId = -1;
    int previousPipeId = -1;
    enum NODE_TYPE nodeType = INVALID_NODE;

    int32_t *nodeNums = NULL;
    int32_t *controlId = NULL;

    int t3aaNums[MAX_NODE];
    int ispNums[MAX_NODE];

    if (m_parameters->getDualMode() == true) {
        t3aaNums[OUTPUT_NODE]    = FIMC_IS_VIDEO_31S_NUM;
        t3aaNums[CAPTURE_NODE_1] = FIMC_IS_VIDEO_31C_NUM;
        t3aaNums[CAPTURE_NODE_2] = FIMC_IS_VIDEO_31P_NUM;
    } else {
        t3aaNums[OUTPUT_NODE]    = FIMC_IS_VIDEO_30S_NUM;
        t3aaNums[CAPTURE_NODE_1] = FIMC_IS_VIDEO_30C_NUM;
        t3aaNums[CAPTURE_NODE_2] = FIMC_IS_VIDEO_30P_NUM;
    }

    ispNums[OUTPUT_NODE]    = FIMC_IS_VIDEO_I0S_NUM;
    ispNums[CAPTURE_NODE_1] = FIMC_IS_VIDEO_I0C_NUM;
    ispNums[CAPTURE_NODE_2] = FIMC_IS_VIDEO_I0P_NUM;

    m_initDeviceInfo(INDEX(PIPE_3AA));
    m_initDeviceInfo(INDEX(PIPE_ISP));
    m_initDeviceInfo(INDEX(PIPE_DIS));

    /*******
     * 3AA
     ******/
    pipeId = INDEX(PIPE_3AA);

    // 3AS
    nodeType = getNodeType(PIPE_3AA);
    m_deviceInfo[pipeId].nodeNum[nodeType] = t3aaNums[OUTPUT_NODE];
    strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "3AA_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_sensorIds[pipeId][nodeType]  = m_getSensorId(m_nodeNums[INDEX(PIPE_FLITE)][getNodeType(PIPE_FLITE)], m_flagFlite3aaOTF, true, m_flagReprocessing);
    m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_3AA;

    // 3AC
    nodeType = getNodeType(PIPE_3AC);
    if (flagDirtyBayer == true || m_parameters->isUsing3acForIspc() == true) {
        m_deviceInfo[pipeId].nodeNum[nodeType] = t3aaNums[CAPTURE_NODE_1];
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "3AA_CAPTURE", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA)], false, false, m_flagReprocessing);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AC;
    } else {
        m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = t3aaNums[CAPTURE_NODE_1];
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "3AA_CAPTURE", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_secondarySensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA)], true, false, m_flagReprocessing);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AC;
    }

    // 3AP
    nodeType = getNodeType(PIPE_3AP);
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = t3aaNums[CAPTURE_NODE_2];
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "3AA_PREVIEW", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_secondarySensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].nodeNum[getNodeType(PIPE_3AA)], m_flag3aaIspOTF, false, m_flagReprocessing);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_3AP;

    // ISPS
    nodeType = getNodeType(PIPE_ISP);
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = ispNums[OUTPUT_NODE];
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "ISP_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_secondarySensorIds[pipeId][nodeType]  = m_getSensorId(m_deviceInfo[pipeId].secondaryNodeNum[getNodeType(PIPE_3AP)], m_flag3aaIspOTF, false, m_flagReprocessing);
    m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_ISP;

    // ISPP
    nodeType = getNodeType(PIPE_ISPP);
    m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = ispNums[CAPTURE_NODE_2];
    strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "ISP_PREVIEW", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_secondarySensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].secondaryNodeNum[getNodeType(PIPE_ISP)], true, false, m_flagReprocessing);
    m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_ISPP;

    // DIS
    if (m_parameters->getHWVdisMode()) {
        nodeType = getNodeType(PIPE_DIS);
        m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = FIMC_IS_VIDEO_TPU_NUM;
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "DIS_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_secondarySensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].secondaryNodeNum[getNodeType(PIPE_ISPP)], true, false, m_flagReprocessing);
        m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_DIS;
    }

    if (m_supportMCSC == true) {
        // MCSC
        nodeType = getNodeType(PIPE_MCSC);
        m_deviceInfo[pipeId].secondaryNodeNum[nodeType] = FIMC_IS_VIDEO_M0S_NUM;
        strncpy(m_deviceInfo[pipeId].secondaryNodeName[nodeType], "MCSC_OUTPUT", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_secondarySensorIds[pipeId][nodeType]  = m_getSensorId(m_deviceInfo[pipeId].secondaryNodeNum[getNodeType(PIPE_ISPP)], m_flagIspMcscOTF, false, m_flagReprocessing);
        m_deviceInfo[pipeId].pipeId[nodeType]  = PIPE_MCSC;

        // MCSC0
        nodeType = getNodeType(PIPE_MCSC0);
        m_deviceInfo[pipeId].nodeNum[nodeType] = FIMC_IS_VIDEO_M0P_NUM;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "MCSC_PREVIEW", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].secondaryNodeNum[getNodeType(PIPE_MCSC)], true, false, m_flagReprocessing);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_MCSC0;
    } else {
        // SCP
        nodeType = getNodeType(PIPE_SCP);
        m_deviceInfo[pipeId].nodeNum[nodeType] = FIMC_IS_VIDEO_SCP_NUM;
        strncpy(m_deviceInfo[pipeId].nodeName[nodeType], "SCP_PREVIEW", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
        m_sensorIds[pipeId][nodeType] = m_getSensorId(m_deviceInfo[pipeId].secondaryNodeNum[getNodeType(PIPE_ISPP)], true, false, m_flagReprocessing);
        m_deviceInfo[pipeId].pipeId[nodeType] = PIPE_SCP;
    }

    // set nodeNum
    for (int i = 0; i < MAX_NODE; i++)
        m_nodeNums[pipeId][i] = m_deviceInfo[pipeId].nodeNum[i];

    if (m_checkNodeSetting(pipeId) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_checkNodeSetting(%d) fail", __FUNCTION__, __LINE__, pipeId);
        return INVALID_OPERATION;
    }

    // VRA
    previousPipeId = pipeId;
    nodeNums = m_nodeNums[INDEX(PIPE_VRA)];
    nodeNums[OUTPUT_NODE] = FIMC_IS_VIDEO_VRA_NUM;
    nodeNums[CAPTURE_NODE_1] = -1;
    nodeNums[CAPTURE_NODE_2] = -1;
    controlId = m_sensorIds[INDEX(PIPE_VRA)];
    controlId[OUTPUT_NODE] = m_getSensorId(m_deviceInfo[previousPipeId].nodeNum[getNodeType(PIPE_SCP)], m_flagMcscVraOTF, true, true);
    controlId[CAPTURE_NODE_1] = -1;
    controlId[CAPTURE_NODE_2] = -1;

    return NO_ERROR;
}

status_t ExynosCameraFrameFactory3aaIspOtf::m_initPipes(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

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

    /* 3AC */
    if ((m_supportReprocessing == true && m_supportPureBayerReprocessing == false) || m_parameters->isUsing3acForIspc() == true) {
        enum NODE_TYPE t3acNodeType = getNodeType(PIPE_3AC);

        perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_3AC_POS : PERFRAME_FRONT_3AC_POS;
        pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameNodeType = PERFRAME_NODE_TYPE_CAPTURE;
        pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].nodeNum[t3acNodeType] - FIMC_IS_VIDEO_BAS_NUM);

#ifdef FIXED_SENSOR_SIZE
        tempRect.fullW = maxSensorW;
        tempRect.fullH = maxSensorH;
#else
        tempRect.fullW = hwSensorW;
        tempRect.fullH = hwSensorH;
#endif
        if (m_parameters->isUsing3acForIspc() == true) {
            tempRect.colorFormat = SCC_OUTPUT_COLOR_FMT;
            tempRect.fullW = hwPictureW;
            tempRect.fullH = hwPictureH;
        } else {
            tempRect.colorFormat = bayerFormat;
        }

        pipeInfo[t3acNodeType].rectInfo = tempRect;
        pipeInfo[t3acNodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        pipeInfo[t3acNodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
        pipeInfo[t3acNodeType].bufInfo.count = config->current->bufInfo.num_3aa_buffers;
        /* per frame info */
        pipeInfo[t3acNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
        pipeInfo[t3acNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;
    }

    /* 3AP */
    enum NODE_TYPE t3apNodeType = getNodeType(PIPE_3AP);

    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_3AP_POS : PERFRAME_FRONT_3AP_POS;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameNodeType = PERFRAME_NODE_TYPE_CAPTURE;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].secondaryNodeNum[t3apNodeType] - FIMC_IS_VIDEO_BAS_NUM);

    pipeInfo[t3apNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[t3apNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;

    /* ISPS */
    enum NODE_TYPE ispsNodeType = getNodeType(PIPE_ISP);

    pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex = PERFRAME_INFO_ISP;
    pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_LEADER;
    pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].secondaryNodeNum[ispsNodeType] - FIMC_IS_VIDEO_BAS_NUM);

    /* ISPP */
    enum NODE_TYPE isppNodeType = getNodeType(PIPE_ISPP);

    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_ISPP_POS : PERFRAME_FRONT_ISPP_POS;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameNodeType = PERFRAME_NODE_TYPE_CAPTURE;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameVideoID = (m_deviceInfo[PIPE_3AA].secondaryNodeNum[isppNodeType] - FIMC_IS_VIDEO_BAS_NUM);

    pipeInfo[isppNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[isppNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;
    if (m_parameters->getHWVdisMode()) {
        /* DIS */
        enum NODE_TYPE disNodeType = getNodeType(PIPE_DIS);

        pipeInfo[disNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
        pipeInfo[disNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex = PERFRAME_INFO_DIS;
        pipeInfo[disNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_LEADER;
        pipeInfo[disNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].secondaryNodeNum[disNodeType] - FIMC_IS_VIDEO_BAS_NUM);
    }

    if (m_supportMCSC == true) {
        /* MCSC */
        enum NODE_TYPE mcscNodeType = getNodeType(PIPE_MCSC);

        pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
        pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex = PERFRAME_INFO_MCSC;
        pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_LEADER;
        pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].secondaryNodeNum[mcscNodeType] - FIMC_IS_VIDEO_BAS_NUM);
    }

    /* SCP & MCSC0 */
    enum NODE_TYPE scpNodeType = getNodeType(PIPE_SCP);

    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_SCP_POS : PERFRAME_FRONT_SCP_POS;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameNodeType = PERFRAME_NODE_TYPE_CAPTURE;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].nodeNum[scpNodeType] - FIMC_IS_VIDEO_BAS_NUM);

    pipeInfo[scpNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[scpNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;

    stride = m_parameters->getHwPreviewStride();
    CLOGV("INFO(%s[%d]):stride=%d", __FUNCTION__, __LINE__, stride);
    tempRect.fullW = stride;
    tempRect.fullH = hwPreviewH;
    tempRect.colorFormat = previewFormat;
#ifdef USE_BUFFER_WITH_STRIDE
/* to use stride for preview buffer, set the bytesPerPlane */
    pipeInfo[scpNodeType].bytesPerPlane[0] = tempRect.fullW;
#endif

    pipeInfo[scpNodeType].rectInfo = tempRect;
    pipeInfo[scpNodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pipeInfo[scpNodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;

    if (m_parameters->increaseMaxBufferOfPreview() == true) {
        pipeInfo[scpNodeType].bufInfo.count = m_parameters->getPreviewBufferCount();
    } else {
        pipeInfo[scpNodeType].bufInfo.count = config->current->bufInfo.num_preview_buffers;
    }

    ret = m_pipes[INDEX(PIPE_3AA)]->setupPipe(pipeInfo, sensorIds, secondarySensorIds);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):3AA setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    if (m_flagMcscVraOTF == false) {
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;

        int vraWidth = 0, vraHeight = 0;
        m_parameters->getHwVraInputSize(&vraWidth, &vraHeight);

        /* VRA pipe */
        tempRect.fullW = vraWidth;
        tempRect.fullH = vraHeight;
        tempRect.colorFormat = m_parameters->getHwVraInputFormat();

        pipeInfo[0].rectInfo = tempRect;
        pipeInfo[0].bufInfo.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        pipeInfo[0].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
        pipeInfo[0].bufInfo.count = config->current->bufInfo.num_vra_buffers;
        /* per frame info */
        pipeInfo[0].perFrameNodeGroupInfo.perframeSupportNodeNum = CAPTURE_NODE_MAX;
        pipeInfo[0].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex = PERFRAME_INFO_VRA;
        pipeInfo[0].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_LEADER;
        pipeInfo[0].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID = (FIMC_IS_VIDEO_VRA_NUM - FIMC_IS_VIDEO_BAS_NUM);

        ret = m_pipes[INDEX(PIPE_VRA)]->setupPipe(pipeInfo, m_sensorIds[INDEX(PIPE_VRA)]);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):VRA setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameFactory3aaIspOtf::m_initPipesFastenAeStable(int32_t numFrames,
                                                                      int hwSensorW, int hwSensorH,
                                                                      int hwPreviewW, int hwPreviewH)
{
    status_t ret = NO_ERROR;

    /* TODO 1. setup pipes for 120FPS */
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
    int bayerFormat = CAMERA_BAYER_FORMAT;
    int previewFormat = m_parameters->getHwPreviewFormat();
    int hwVdisformat = m_parameters->getHWVdisFormat();
    struct ExynosConfigInfo *config = m_parameters->getConfig();
    ExynosRect bdsSize;
    uint32_t frameRate = 0;
    struct v4l2_streamparm streamParam;
    int perFramePos = 0;

#ifdef DEBUG_RAWDUMP
    if (m_parameters->checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    m_parameters->getPreviewBdsSize(&bdsSize);

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


    tempRect.fullW = 32;
    tempRect.fullH = 64;
    tempRect.colorFormat = bayerFormat;

    pipeInfo[t3asNodeType].rectInfo = tempRect;
    pipeInfo[t3asNodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    pipeInfo[t3asNodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
    pipeInfo[t3asNodeType].bufInfo.count = numFrames;

    /* per frame info */
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = CAPTURE_NODE_MAX;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex = PERFRAME_INFO_3AA;

    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_LEADER;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].nodeNum[t3asNodeType] - FIMC_IS_VIDEO_BAS_NUM);

    /* 3AC */
    if ((m_supportReprocessing == true && m_supportPureBayerReprocessing == false) || m_parameters->isUsing3acForIspc() == true) {
        enum NODE_TYPE t3acNodeType = getNodeType(PIPE_3AC);

        perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_3AC_POS : PERFRAME_FRONT_3AC_POS;
        pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameNodeType = PERFRAME_NODE_TYPE_CAPTURE;
        pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].nodeNum[t3acNodeType] - FIMC_IS_VIDEO_BAS_NUM);

#ifdef FIXED_SENSOR_SIZE
        tempRect.fullW = maxSensorW;
        tempRect.fullH = maxSensorH;
#else
        tempRect.fullW = hwSensorW;
        tempRect.fullH = hwSensorH;
#endif
        if (m_parameters->isUsing3acForIspc() == true) {
            tempRect.colorFormat = SCC_OUTPUT_COLOR_FMT;
        } else {
            tempRect.colorFormat = bayerFormat;
        }

        pipeInfo[t3acNodeType].rectInfo = tempRect;
        pipeInfo[t3acNodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        pipeInfo[t3acNodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
        pipeInfo[t3acNodeType].bufInfo.count = numFrames;
        /* per frame info */
        pipeInfo[t3acNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
        pipeInfo[t3acNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;
    }


    /* 3AP */
    enum NODE_TYPE t3apNodeType = getNodeType(PIPE_3AP);

    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_3AP_POS : PERFRAME_FRONT_3AP_POS;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameNodeType = PERFRAME_NODE_TYPE_CAPTURE;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].secondaryNodeNum[t3apNodeType] - FIMC_IS_VIDEO_BAS_NUM);

    pipeInfo[t3apNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[t3apNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;

    /* ISP pipe */
    enum NODE_TYPE ispsNodeType = getNodeType(PIPE_ISP);

    pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex = PERFRAME_INFO_ISP;
    pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_LEADER;
    pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].secondaryNodeNum[ispsNodeType] - FIMC_IS_VIDEO_BAS_NUM);

    /* ISPP */
    enum NODE_TYPE isppNodeType = getNodeType(PIPE_ISPP);

    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_ISPP_POS : PERFRAME_FRONT_ISPP_POS;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameNodeType = PERFRAME_NODE_TYPE_CAPTURE;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameVideoID = (m_deviceInfo[PIPE_3AA].secondaryNodeNum[isppNodeType] - FIMC_IS_VIDEO_BAS_NUM);

    pipeInfo[isppNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[isppNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;

    if (m_parameters->getHWVdisMode()) {
        /* DIS */
        enum NODE_TYPE disNodeType = getNodeType(PIPE_DIS);

        /* per frame info */
        pipeInfo[disNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
        pipeInfo[disNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex = PERFRAME_INFO_DIS;
        pipeInfo[disNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_LEADER;
        pipeInfo[disNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID = (m_deviceInfo[PIPE_3AA].secondaryNodeNum[disNodeType] - FIMC_IS_VIDEO_BAS_NUM);
    }

    if (m_supportMCSC == true) {
        /* MCSC */
        enum NODE_TYPE mcscNodeType = getNodeType(PIPE_MCSC);

        pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
        pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex = PERFRAME_INFO_MCSC;
        pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_LEADER;
        pipeInfo[ispsNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].secondaryNodeNum[mcscNodeType] - FIMC_IS_VIDEO_BAS_NUM);
    }

    /* SCP & MCSC0 */
    enum NODE_TYPE scpNodeType = getNodeType(PIPE_SCP);

    perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_SCP_POS : PERFRAME_FRONT_SCP_POS;

    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameNodeType = PERFRAME_NODE_TYPE_CAPTURE;
    pipeInfo[t3asNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameVideoID = (m_deviceInfo[INDEX(PIPE_3AA)].nodeNum[scpNodeType] - FIMC_IS_VIDEO_BAS_NUM);

    pipeInfo[scpNodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[scpNodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;

    tempRect.fullW = hwPreviewW;
    tempRect.fullH = hwPreviewH;
    tempRect.colorFormat = previewFormat;

#ifdef CAMERA_PACKED_BAYER_ENABLE
#ifdef DEBUG_RAWDUMP
    if (m_parameters->checkBayerDumpEnable()) {
        /* packed bayer bytesPerPlane */
        pipeInfo[scpNodeType].bytesPerPlane[0] = ROUND_UP(tempRect.fullW * 2, 16);
    }
    else
#endif
    {
        /* packed bayer bytesPerPlane */
        pipeInfo[scpNodeType].bytesPerPlane[0] = ROUND_UP(tempRect.fullW * 3 / 2, 16);
    }
#endif

    pipeInfo[scpNodeType].rectInfo = tempRect;
    pipeInfo[scpNodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pipeInfo[scpNodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
    pipeInfo[scpNodeType].bufInfo.count = numFrames;

    ret = m_pipes[INDEX(PIPE_3AA)]->setupPipe(pipeInfo, sensorIds, secondarySensorIds);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):3AA setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    if (m_flagMcscVraOTF == false) {
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;

        int vraWidth = 0, vraHeight = 0;
        m_parameters->getHwVraInputSize(&vraWidth, &vraHeight);

        /* VRA pipe */
        tempRect.fullW = vraWidth;
        tempRect.fullH = vraHeight;
        tempRect.colorFormat = m_parameters->getHwVraInputFormat();

        pipeInfo[0].rectInfo = tempRect;
        pipeInfo[0].bufInfo.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        pipeInfo[0].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
        pipeInfo[0].bufInfo.count = numFrames;
        /* per frame info */
        pipeInfo[0].perFrameNodeGroupInfo.perframeSupportNodeNum = CAPTURE_NODE_MAX;
        pipeInfo[0].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex = PERFRAME_INFO_VRA;
        pipeInfo[0].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_LEADER;
        pipeInfo[0].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID = (FIMC_IS_VIDEO_VRA_NUM - FIMC_IS_VIDEO_BAS_NUM);

        ret = m_pipes[INDEX(PIPE_VRA)]->setupPipe(pipeInfo, m_sensorIds[INDEX(PIPE_VRA)]);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):VRA setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    return NO_ERROR;
}

void ExynosCameraFrameFactory3aaIspOtf::m_init(void)
{
}

}; /* namespace android */
