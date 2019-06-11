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
#define LOG_TAG "ExynosCameraFrameReprocessingFactoryNV21"
#include <cutils/log.h>

#include "ExynosCameraFrameReprocessingFactoryNV21.h"

namespace android {

ExynosCameraFrameReprocessingFactoryNV21::~ExynosCameraFrameReprocessingFactoryNV21()
{
    status_t ret = NO_ERROR;

    ret = destroy();
    if (ret != NO_ERROR)
        CLOGE("ERR(%s[%d]):destroy fail", __FUNCTION__, __LINE__);

    m_setCreate(false);
}

status_t ExynosCameraFrameReprocessingFactoryNV21::initPipes(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    status_t ret = NO_ERROR;
    camera_pipe_info_t pipeInfo[MAX_NODE];
    camera_pipe_info_t nullPipeInfo;

    int pipeId = -1;
    enum NODE_TYPE nodeType = INVALID_NODE;
    enum NODE_TYPE leaderNodeType = OUTPUT_NODE;

    int32_t nodeNums[MAX_NODE];
    int32_t sensorIds[MAX_NODE];
    int32_t secondarySensorIds[MAX_NODE];
    for (int i = 0; i < MAX_NODE; i++) {
        nodeNums[i] = -1;
        sensorIds[i] = -1;
        secondarySensorIds[i] = -1;
    }

    ExynosRect tempRect;
    int hwSensorW = 0, hwSensorH = 0;
    int previewW = 0, previewH = 0;
    int pictureW = 0, pictureH = 0;
    int maxPreviewW = 0, maxPreviewH = 0, hwPreviewW = 0, hwPreviewH = 0;
    int maxPictureW = 0, maxPictureH = 0, hwPictureW = 0, hwPictureH = 0;
    int maxThumbnailW = 0, maxThumbnailH = 0;
    int bayerFormat = m_parameters->getBayerFormat(PIPE_3AA_REPROCESSING);
    int previewFormat = m_parameters->getPreviewFormat();
    int pictureFormat = m_parameters->getHwPictureFormat();
    struct ExynosConfigInfo *config = m_parameters->getConfig();
    int perFramePos = 0;

    memset(&nullPipeInfo, 0, sizeof(camera_pipe_info_t));

#ifdef DEBUG_RAWDUMP
    if (m_parameters->checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    m_parameters->getHwSensorSize(&hwSensorW, &hwSensorH);
    m_parameters->getMaxPreviewSize(&maxPreviewW, &maxPreviewH);
    m_parameters->getHwPreviewSize(&hwPreviewW, &hwPreviewH);
    m_parameters->getMaxPictureSize(&maxPictureW, &maxPictureH);
    m_parameters->getHwPictureSize(&hwPictureW, &hwPictureH);
    m_parameters->getPreviewSize(&previewW, &previewH);
    m_parameters->getPictureSize(&pictureW, &pictureH);
    m_parameters->getMaxThumbnailSize(&maxThumbnailW, &maxThumbnailH);

    CLOGI("INFO(%s[%d]): MaxPreviewSize(%dx%d), HwPreviewSize(%dx%d)", __FUNCTION__, __LINE__, maxPreviewW, maxPreviewH, hwPreviewW, hwPreviewH);
    CLOGI("INFO(%s[%d]): MaxPixtureSize(%dx%d), HwPixtureSize(%dx%d)", __FUNCTION__, __LINE__, maxPictureW, maxPictureH, hwPictureW, hwPictureH);
    CLOGI("INFO(%s[%d]): PreviewSize(%dx%d), PictureSize(%dx%d)", __FUNCTION__, __LINE__, previewW, previewH, pictureW, pictureH);
    CLOGI("INFO(%s[%d]): MaxThumbnailSize(%dx%d)", __FUNCTION__, __LINE__, maxThumbnailW, maxThumbnailH);


    if (m_parameters->isUseYuvReprocessing() == false) {
        /*
         * 3AA for Reprocessing
         */
        if (m_supportPureBayerReprocessing == true) {
            pipeId = PIPE_3AA_REPROCESSING;

            /* 3AS */
            nodeType = getNodeType(PIPE_3AA_REPROCESSING);
            bayerFormat = m_parameters->getBayerFormat(PIPE_3AA_REPROCESSING);

            /* set v4l2 buffer size */
            tempRect.fullW = hwSensorW;
            tempRect.fullH = hwSensorH;
            tempRect.colorFormat = bayerFormat;

            /* set v4l2 video node bytes per plane */
            pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);

            /* set v4l2 video node buffer count */
            pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_bayer_buffers;

            /* Set output node default info */
            SET_OUTPUT_DEVICE_BASIC_INFO(PERFRAME_INFO_PURE_REPROCESSING_3AA);

#if 0
            /* 3AC */
            nodeType = getNodeType(PIPE_3AC_REPROCESSING);
            perFramePos = PERFRAME_REPROCESSING_3AC_POS;
            bayerFormat = m_parameters->getBayerFormat(PIPE_3AC_REPROCESSING);

            /* set v4l2 buffer size */
            tempRect.fullW = hwPictureW;
            tempRect.fullH = hwPictureH;
            tempRect.colorFormat = bayerFormat;

            /* set v4l2 video node buffer count */
            pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

            /* Set capture node default info */
            SET_CAPTURE_DEVICE_BASIC_INFO();
#endif

            /* 3AP */
            nodeType = getNodeType(PIPE_3AP_REPROCESSING);
            perFramePos = PERFRAME_REPROCESSING_3AP_POS;
            bayerFormat = m_parameters->getBayerFormat(PIPE_3AP_REPROCESSING);

            /* set v4l2 buffer size */
            tempRect.fullW = hwPictureW;
            tempRect.fullH = hwPictureH;
            tempRect.colorFormat = bayerFormat;

            /* set v4l2 video node bytes per plane */
            pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);

            /* set v4l2 video node buffer count */
            pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

            /* Set capture node default info */
            SET_CAPTURE_DEVICE_BASIC_INFO();

            /* setup pipe info to 3AA pipe */
            if (m_flag3aaIspOTF == false) {
                ret = m_pipes[INDEX(pipeId)]->setupPipe(pipeInfo, m_sensorIds[INDEX(pipeId)]);
                if (ret != NO_ERROR) {
                    CLOGE("ERR(%s[%d]):3AA setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                    /* TODO: exception handling */
                    return INVALID_OPERATION;
                }

                /* clear pipeInfo for next setupPipe */
                for (int i = 0; i < MAX_NODE; i++)
                    pipeInfo[i] = nullPipeInfo;
            }
        } else {
            /*
             * 3A video node is opened for dirty bayer.
             * So, we have to do setinput to 3A video node.
             */
            pipeId = PIPE_3AA_REPROCESSING;

            /* setup pipe info to 3AA pipe */
            ret = m_pipes[INDEX(pipeId)]->setupPipe(pipeInfo, m_sensorIds[INDEX(pipeId)]);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):3AA setupPipe for dirty bayer reprocessing  fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                /* TODO: exception handling */
                return INVALID_OPERATION;
            }

            /* clear pipeInfo for next setupPipe */
            for (int i = 0; i < MAX_NODE; i++)
                pipeInfo[i] = nullPipeInfo;
        }


        /*
         * ISP for Reprocessing
         */

        /* ISP */
        if (m_supportPureBayerReprocessing == false || m_flag3aaIspOTF == false) {
            pipeId = PIPE_ISP_REPROCESSING;
            nodeType = getNodeType(PIPE_ISP_REPROCESSING);
            bayerFormat = m_parameters->getBayerFormat(PIPE_ISP_REPROCESSING);

            /* set v4l2 buffer size */
            tempRect.fullW = hwPictureW;
            tempRect.fullH = hwPictureH;
            tempRect.colorFormat = bayerFormat;

            /* set v4l2 video node bytes per plane */
            pipeInfo[nodeType].bytesPerPlane[0] = getBayerLineSize(tempRect.fullW, bayerFormat);

            /* set v4l2 video node buffer count */
            pipeInfo[nodeType].bufInfo.count = m_supportPureBayerReprocessing ? config->current->bufInfo.num_picture_buffers : config->current->bufInfo.num_bayer_buffers;

            /* Set output node default info */
            int ispPerframeInfoIndex = m_supportPureBayerReprocessing ? PERFRAME_INFO_PURE_REPROCESSING_ISP : PERFRAME_INFO_DIRTY_REPROCESSING_ISP;
            SET_OUTPUT_DEVICE_BASIC_INFO(ispPerframeInfoIndex);
        }

        /* ISPC */
        nodeType = getNodeType(PIPE_ISPC_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_ISPC_POS;

        /* set v4l2 buffer size */
        tempRect.fullW = hwPictureW;
        tempRect.fullH = hwPictureH;
        tempRect.colorFormat = pictureFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        /* ISPP */
        nodeType = getNodeType(PIPE_ISPP_REPROCESSING);
        perFramePos = PERFRAME_REPROCESSING_ISPP_POS;

        /* set v4l2 buffer size */
        tempRect.fullW = hwPictureW;
        tempRect.fullH = hwPictureH;
        tempRect.colorFormat = pictureFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

        /* Set capture node default info */
        SET_CAPTURE_DEVICE_BASIC_INFO();

        /* setup pipe info to ISP pipe */
        if (m_flagIspTpuOTF == false && m_flagIspMcscOTF == false) {
        ret = m_pipes[INDEX(pipeId)]->setupPipe(pipeInfo, m_sensorIds[INDEX(pipeId)]);
        if (ret != NO_ERROR) {
            CLOGE("ERR(%s[%d]):ISP setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* clear pipeInfo for next setupPipe */
        for (int i = 0; i < MAX_NODE; i++)
            pipeInfo[i] = nullPipeInfo;
        }
    }


    /*
     * MCSC for Reprocessing
     */

    /* MCSC */
    if (m_parameters->isUseYuvReprocessing() == true
        || (m_flagIspMcscOTF == false && m_flagTpuMcscOTF == false)) {
        pipeId = PIPE_MCSC_REPROCESSING;
        nodeType = getNodeType(PIPE_MCSC_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = hwPictureW;
        tempRect.fullH = hwPictureH;
        tempRect.colorFormat = pictureFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

        /* Set output node default info */
        int mcscPerframeInfoIndex = !(0) ? PERFRAME_INFO_YUV_REPROCESSING_MCSC : (m_supportPureBayerReprocessing ? PERFRAME_INFO_PURE_REPROCESSING_MCSC : PERFRAME_INFO_DIRTY_REPROCESSING_MCSC);
        SET_OUTPUT_DEVICE_BASIC_INFO(mcscPerframeInfoIndex);
    }

    /* MCSC0 */
    nodeType = getNodeType(PIPE_MCSC0_REPROCESSING);
    perFramePos = PERFRAME_REPROCESSING_MCSC0_POS;

    /* set v4l2 buffer size */
    tempRect.fullW = maxPictureW;
    tempRect.fullH = maxPictureH;
    tempRect.colorFormat = V4L2_PIX_FMT_NV21;

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();

    /* 34xx hasn't multi-port */
#if 0
    /* MCSC3 */
    nodeType = getNodeType(PIPE_MCSC3_REPROCESSING);
    perFramePos = PERFRAME_REPROCESSING_MCSC3_POS;

    /* set v4l2 buffer size */
    tempRect.fullW = maxPictureW;
    tempRect.fullH = maxPictureH;
    tempRect.colorFormat = pictureFormat;

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();

    /* MCSC4 */
    nodeType = getNodeType(PIPE_MCSC4_REPROCESSING);
    perFramePos = PERFRAME_REPROCESSING_MCSC4_POS;

    /* set v4l2 buffer size */
    tempRect.fullW = maxThumbnailW;
    tempRect.fullH = maxThumbnailH;
    tempRect.colorFormat = pictureFormat;

    /* set v4l2 video node buffer count */
    pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

    /* Set capture node default info */
    SET_CAPTURE_DEVICE_BASIC_INFO();
#endif

    if (m_flagHWFCEnabled == true) {
        /* JPEG Src */
        nodeType = getNodeType(PIPE_HWFC_JPEG_SRC_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = maxPictureW;
        tempRect.fullH = maxPictureH;
        tempRect.colorFormat = pictureFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

        /* Set capture node default info */
        pipeInfo[nodeType].rectInfo = tempRect;
        pipeInfo[nodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        pipeInfo[nodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;

        /* Thumbnail Src */
        nodeType = getNodeType(PIPE_HWFC_THUMB_SRC_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = maxThumbnailW;
        tempRect.fullH = maxThumbnailH;
        tempRect.colorFormat = pictureFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

        /* Set capture node default info */
        pipeInfo[nodeType].rectInfo = tempRect;
        pipeInfo[nodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        pipeInfo[nodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;

        /* JPEG Dst */
        nodeType = getNodeType(PIPE_HWFC_JPEG_DST_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = maxPictureW;
        tempRect.fullH = maxPictureH;
        tempRect.colorFormat = pictureFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

        /* Set capture node default info */
        pipeInfo[nodeType].rectInfo = tempRect;
        pipeInfo[nodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        pipeInfo[nodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;

        /* Thumbnail Dst */
        nodeType = getNodeType(PIPE_HWFC_THUMB_DST_REPROCESSING);

        /* set v4l2 buffer size */
        tempRect.fullW = maxThumbnailW;
        tempRect.fullH = maxThumbnailH;
        tempRect.colorFormat = pictureFormat;

        /* set v4l2 video node buffer count */
        pipeInfo[nodeType].bufInfo.count = config->current->bufInfo.num_picture_buffers;

        /* Set capture node default info */
        pipeInfo[nodeType].rectInfo = tempRect;
        pipeInfo[nodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        pipeInfo[nodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
    }

    ret = m_pipes[INDEX(pipeId)]->setupPipe(pipeInfo, m_sensorIds[INDEX(pipeId)]);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):ISP setupPipe fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

}; /* namespace android */
