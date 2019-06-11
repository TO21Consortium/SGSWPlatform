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
#define LOG_TAG "ExynosCameraFrameFactoryPreview"
#include <cutils/log.h>

#include "ExynosCameraFrameFactoryPreview.h"

namespace android {

ExynosCameraFrameFactoryPreview::~ExynosCameraFrameFactoryPreview()
{
    int ret = 0;

    ret = destroy();
    if (ret < 0)
        CLOGE("ERR(%s[%d]):destroy fail", __FUNCTION__, __LINE__);
}

status_t ExynosCameraFrameFactoryPreview::create(__unused bool active)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    m_setupConfig();

    int ret = 0;
    int leaderPipe = PIPE_3AA;
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

    if (m_parameters->getHWVdisMode()) {
        m_pipes[INDEX(PIPE_DIS)] = (ExynosCameraPipe*)new ExynosCameraMCPipe(m_cameraId, m_parameters, false, &m_deviceInfo[INDEX(PIPE_DIS)]);
        m_pipes[INDEX(PIPE_DIS)]->setPipeId(PIPE_DIS);
        m_pipes[INDEX(PIPE_DIS)]->setPipeName("PIPE_DIS");
    }

    if (m_flagMcscVraOTF == false) {
        m_pipes[INDEX(PIPE_VRA)] = (ExynosCameraPipe*)new ExynosCameraPipeVRA(m_cameraId, m_parameters, false, m_nodeNums[INDEX(PIPE_VRA)]);
        m_pipes[INDEX(PIPE_VRA)]->setPipeId(PIPE_VRA);
        m_pipes[INDEX(PIPE_VRA)]->setPipeName("PIPE_VRA");
    }

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

    /* DIS pipe initialize */
    if (m_parameters->getHWVdisMode()) {
        ret = m_pipes[INDEX(PIPE_DIS)]->create();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):DIS create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
        CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_DIS));
    }

    /* VRA pipe initialize */
    if (m_flagMcscVraOTF == false) {
        /* EOS */
        ret = m_pipes[INDEX(leaderPipe)]->setControl(V4L2_CID_IS_END_OF_STREAM, 1);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):PIPE_%d V4L2_CID_IS_END_OF_STREAM fail, ret(%d)", __FUNCTION__, __LINE__, leaderPipe, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }

        /* Change leaderPipe to VRA, Create new instance */
        leaderPipe = PIPE_VRA;

        ret = m_pipes[INDEX(PIPE_VRA)]->create();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):VRA create fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
        CLOGD("DEBUG(%s):Pipe(%d) created", __FUNCTION__, INDEX(PIPE_VRA));
    }

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
    ret = m_pipes[INDEX(leaderPipe)]->setControl(V4L2_CID_IS_END_OF_STREAM, 1);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):PIPE_%d V4L2_CID_IS_END_OF_STREAM fail, ret(%d)", __FUNCTION__, __LINE__, leaderPipe, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    m_setCreate(true);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::m_fillNodeGroupInfo(ExynosCameraFrame *frame)
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
    bool dis = false;

    m_parameters->getHwPreviewSize(&previewW, &previewH);
    m_parameters->getPictureSize(&pictureW, &pictureH);
    m_parameters->getPreviewBayerCropSize(&bnsSize, &bayerCropSize);
    m_parameters->getPreviewBdsSize(&bdsSize);
    tpu = m_parameters->getTpuEnabledMode();
    dis = m_parameters->getHWVdisMode();

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

    /* DIS */
    memcpy(&node_group_info_dis, &node_group_info_isp, sizeof (camera2_node_group));

    if (tpu == true) {
        /* ISPP */
        if (m_requestISPP == true) {
            perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_ISPP_POS : PERFRAME_FRONT_ISPP_POS;
            node_group_info_isp.capture[perFramePos].request = frame->getRequest(PIPE_ISPP);
        }

        /* SCP */
        perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_SCP_POS : PERFRAME_FRONT_SCP_POS;
        node_group_info_dis.capture[perFramePos].request = frame->getRequest(PIPE_SCP);
    } else {
        /* SCP */
        perFramePos = (m_cameraId == CAMERA_ID_BACK) ? PERFRAME_BACK_SCP_POS : PERFRAME_FRONT_SCP_POS;

        if (m_flag3aaIspOTF == true)
            node_group_info_3aa.capture[perFramePos].request = frame->getRequest(PIPE_SCP);
        else
            node_group_info_isp.capture[perFramePos].request = frame->getRequest(PIPE_SCP);
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
        dis);

    ExynosCameraNodeGroupDIS::updateNodeGroupInfo(
        m_cameraId,
        &node_group_info_dis,
        bayerCropSize,
        bdsSize,
        previewW, previewH,
        pictureW, pictureH,
        dis);

    frame->storeNodeGroupInfo(&node_group_info_3aa, PERFRAME_INFO_3AA, zoom);
    frame->storeNodeGroupInfo(&node_group_info_isp, PERFRAME_INFO_ISP, zoom);
    frame->storeNodeGroupInfo(&node_group_info_dis, PERFRAME_INFO_DIS, zoom);

    return NO_ERROR;
}

ExynosCameraFrame *ExynosCameraFrameFactoryPreview::createNewFrame(void)
{
    int ret = 0;
    ExynosCameraFrameEntity *newEntity[MAX_NUM_PIPES] = {0};
    ExynosCameraFrame *frame = m_frameMgr->createFrame(m_parameters, m_frameCount, FRAME_TYPE_PREVIEW);
    if (frame == NULL)
        return NULL;

    int requestEntityCount = 0;

    ret = m_initFrameMetadata(frame);
    if (ret < 0)
        CLOGE("(%s[%d]): frame(%d) metadata initialize fail", __FUNCTION__, __LINE__, m_frameCount);

    if (m_flagFlite3aaOTF == true) {
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

        if (m_requestDIS == true) {
            if (m_flag3aaIspOTF == true) {
                /* set DIS pipe to linkageList */
                newEntity[INDEX(PIPE_DIS)] = new ExynosCameraFrameEntity(PIPE_DIS, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_DELIVERY);
                frame->addChildEntity(newEntity[INDEX(PIPE_3AA)], newEntity[INDEX(PIPE_DIS)], INDEX(PIPE_ISPP));
                requestEntityCount++;
            } else {
                /* set ISP pipe to linkageList */
                newEntity[INDEX(PIPE_ISP)] = new ExynosCameraFrameEntity(PIPE_ISP, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
                frame->addChildEntity(newEntity[INDEX(PIPE_3AA)], newEntity[INDEX(PIPE_ISP)], INDEX(PIPE_3AP));
                requestEntityCount++;

                /* set DIS pipe to linkageList */
                newEntity[INDEX(PIPE_DIS)] = new ExynosCameraFrameEntity(PIPE_DIS, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_DELIVERY);
                frame->addChildEntity(newEntity[INDEX(PIPE_ISP)], newEntity[INDEX(PIPE_DIS)], INDEX(PIPE_ISPP));
                requestEntityCount++;
            }
        } else {
            if (m_flag3aaIspOTF == true) {
                /* skip ISP pipe to linkageList */
            } else {
                /* set ISP pipe to linkageList */
                newEntity[INDEX(PIPE_ISP)] = new ExynosCameraFrameEntity(PIPE_ISP, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
                frame->addChildEntity(newEntity[INDEX(PIPE_3AA)], newEntity[INDEX(PIPE_ISP)], INDEX(PIPE_3AP));
                requestEntityCount++;
            }
        }
    } else {
        /* set flite pipe to linkageList */
        newEntity[INDEX(PIPE_FLITE)] = new ExynosCameraFrameEntity(PIPE_FLITE, ENTITY_TYPE_OUTPUT_ONLY, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_FLITE)]);

        /* set 3AA pipe to linkageList */
        newEntity[INDEX(PIPE_3AA)] = new ExynosCameraFrameEntity(PIPE_3AA, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addChildEntity(newEntity[INDEX(PIPE_FLITE)], newEntity[INDEX(PIPE_3AA)]);

        /* set ISP pipe to linkageList */
        if (m_requestISP == true) {
            newEntity[INDEX(PIPE_ISP)] = new ExynosCameraFrameEntity(PIPE_ISP, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_FIXED);
            frame->addChildEntity(newEntity[INDEX(PIPE_3AA)], newEntity[INDEX(PIPE_ISP)]);
        }

        /* set DIS pipe to linkageList */
        if (m_requestDIS == true) {
            newEntity[INDEX(PIPE_DIS)] = new ExynosCameraFrameEntity(PIPE_DIS, ENTITY_TYPE_INPUT_ONLY, ENTITY_BUFFER_DELIVERY);
            frame->addChildEntity(newEntity[INDEX(PIPE_ISP)], newEntity[INDEX(PIPE_DIS)]);
        }

        /* flite, 3aa, isp, dis as one. */
        requestEntityCount++;
    }

    if (m_flagMcscVraOTF == false) {
        newEntity[INDEX(PIPE_VRA)] = new ExynosCameraFrameEntity(PIPE_VRA, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addChildEntity(newEntity[INDEX(PIPE_3AA)], newEntity[INDEX(PIPE_VRA)]);
        requestEntityCount++;
    }

    if (m_supportReprocessing == false) {
        /* set GSC-Picture pipe to linkageList */
        newEntity[INDEX(PIPE_GSC_PICTURE)] = new ExynosCameraFrameEntity(PIPE_GSC_PICTURE, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
        frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_GSC_PICTURE)]);
    }

    /* set GSC pipe to linkageList */
    newEntity[INDEX(PIPE_GSC)] = new ExynosCameraFrameEntity(PIPE_GSC, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_GSC)]);

    newEntity[INDEX(PIPE_GSC_VIDEO)] = new ExynosCameraFrameEntity(PIPE_GSC_VIDEO, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_GSC_VIDEO)]);

    /* PIPE_VRA's internal pipe entity */
    newEntity[INDEX(PIPE_GSC_VRA)] = new ExynosCameraFrameEntity(PIPE_GSC_VRA, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_GSC_VRA)]);

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

status_t ExynosCameraFrameFactoryPreview::initPipes(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    int ret = 0;

    ret = m_initFlitePipe();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_initFlitePipe() fail", __FUNCTION__, __LINE__);
        return ret;
    }

    ret = m_setDeviceInfo();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_setDeviceInfo() fail", __FUNCTION__, __LINE__);
        return ret;
    }

    ret = m_initPipes();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_initPipes() fail", __FUNCTION__, __LINE__);
        return ret;
    }

    m_frameCount = 0;

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::preparePipes(void)
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

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::startPipes(void)
{
    int ret = 0;

    if (m_flagMcscVraOTF == false) {
        ret = m_pipes[INDEX(PIPE_VRA)]->start();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):VRA start fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_parameters->getTpuEnabledMode() == true) {
        ret = m_pipes[INDEX(PIPE_DIS)]->start();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):DIS start fail, ret(%d)", __FUNCTION__, __LINE__, ret);
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

status_t ExynosCameraFrameFactoryPreview::startInitialThreads(void)
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

    if (m_parameters->is3aaIspOtf() == false) {
        ret = startThread(PIPE_ISP);
        if (ret < 0)
            return ret;
    }

    if (m_parameters->getTpuEnabledMode() == true) {
        ret = startThread(PIPE_DIS);
        if (ret < 0)
            return ret;
    }

    if (m_parameters->isMcscVraOtf() == false) {
        ret = startThread(PIPE_VRA);
        if (ret < 0)
            return ret;
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameFactoryPreview::setStopFlag(void)
{
    CLOGI("INFO(%s[%d]):", __FUNCTION__, __LINE__);

    int ret = 0;

    ret |= m_pipes[INDEX(PIPE_FLITE)]->setStopFlag();

    if (m_pipes[INDEX(PIPE_3AA)]->flagStart() == true)
        ret |= m_pipes[INDEX(PIPE_3AA)]->setStopFlag();

    if (m_pipes[INDEX(PIPE_ISP)]->flagStart() == true)
        ret |= m_pipes[INDEX(PIPE_ISP)]->setStopFlag();

    if (m_parameters->getHWVdisMode() == true
        && m_pipes[INDEX(PIPE_DIS)]->flagStart() == true)
        ret |= m_pipes[INDEX(PIPE_DIS)]->setStopFlag();

    if (m_flagMcscVraOTF == false
        && m_pipes[INDEX(PIPE_VRA)]->flagStart() == true)
        ret |= m_pipes[INDEX(PIPE_VRA)]->setStopFlag();

    return ret;
}

status_t ExynosCameraFrameFactoryPreview::stopPipes(void)
{
    int ret = 0;

    if (m_pipes[INDEX(PIPE_VRA)] != NULL
        && m_pipes[INDEX(PIPE_VRA)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_VRA)]->stopThread();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):VRA stopThread fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            /* TODO: exception handling */
            return INVALID_OPERATION;
        }
    }

    if (m_pipes[INDEX(PIPE_DIS)] != NULL
        && m_pipes[INDEX(PIPE_DIS)]->isThreadRunning() == true) {
        ret = m_pipes[INDEX(PIPE_DIS)]->stopThread();
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):DIS stopThread fail, ret(%d)", __FUNCTION__, __LINE__, ret);
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
    if (m_pipes[INDEX(PIPE_ISP)] != NULL
        && m_pipes[INDEX(PIPE_ISP)]->isThreadRunning() == true) {
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

    if (m_parameters->getHWVdisMode()) {
        if (m_pipes[INDEX(PIPE_DIS)]->flagStart() == true) {
            /* DIS force done */
            ret = m_pipes[INDEX(PIPE_DIS)]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):PIPE_DIS force done fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                /* TODO: exception handling */
                /* return INVALID_OPERATION; */
            }

            ret = m_pipes[INDEX(PIPE_DIS)]->stop();
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):DIS stop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                /* TODO: exception handling */
                return INVALID_OPERATION;
            }
        }
    }

    if (m_flagMcscVraOTF == false) {
        if (m_pipes[INDEX(PIPE_VRA)]->flagStart() == true) {
            /* VRA force done */
            ret = m_pipes[INDEX(PIPE_VRA)]->forceDone(V4L2_CID_IS_FORCE_DONE, 0x1000);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):PIPE_VRA force done fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                /* TODO: exception handling */
                /* return INVALID_OPERATION; */
            }

            ret = m_pipes[INDEX(PIPE_VRA)]->stop();
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):VRA stop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                /* TODO: exception handling */
                return INVALID_OPERATION;
            }
        }
    }

    ret = stopThreadAndWait(INDEX(PIPE_GSC));
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):PIPE_GSC stopThreadAndWait fail, ret(%d)", __FUNCTION__, __LINE__, ret);
    }
    CLOGI("INFO(%s[%d]):Stopping  Success!", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

void ExynosCameraFrameFactoryPreview::m_init(void)
{
    m_supportReprocessing = false;
    m_flagFlite3aaOTF = false;
    m_flagIspMcscOTF = false;
    m_flagMcscVraOTF = false;
    m_supportSCC = false;
    m_supportMCSC = false;
    m_supportPureBayerReprocessing = false;
    m_flagReprocessing = false;
}

status_t ExynosCameraFrameFactoryPreview::m_setupConfig()
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    status_t ret = NO_ERROR;

    int32_t *nodeNums = NULL;
    int32_t *controlId = NULL;
    int32_t *secondaryControlId = NULL;
    int32_t *prevNode = NULL;

    m_flagFlite3aaOTF = m_parameters->isFlite3aaOtf();
    m_flag3aaIspOTF = m_parameters->is3aaIspOtf();
    m_flagIspMcscOTF = m_parameters->isIspMcscOtf();
    m_flagMcscVraOTF = m_parameters->isMcscVraOtf();
    m_supportReprocessing = m_parameters->isReprocessing();
    m_supportSCC = m_parameters->isOwnScc(m_cameraId);
    m_supportMCSC = m_parameters->isOwnMCSC();

    if (m_parameters->getRecordingHint() == true) {
        m_supportPureBayerReprocessing = (m_cameraId == CAMERA_ID_BACK) ? USE_PURE_BAYER_REPROCESSING_ON_RECORDING : USE_PURE_BAYER_REPROCESSING_FRONT_ON_RECORDING;
    } else {
        m_supportPureBayerReprocessing = (m_cameraId == CAMERA_ID_BACK) ? USE_PURE_BAYER_REPROCESSING : USE_PURE_BAYER_REPROCESSING_FRONT;
    }

    m_flagReprocessing = false;

    if (m_supportReprocessing == false) {
        if (m_supportSCC == true)
            m_requestSCC = 1;
        else
            m_requestISPC = 1;
    }

    if (m_flag3aaIspOTF == true) {
        m_request3AP = 0;
        m_requestISP = 0;
    } else {
        m_request3AP = 1;
        m_requestISP = 1;
    }

    if (m_flagMcscVraOTF == true)
        m_requestVRA = 0;
    else
        m_requestVRA = 1;

    nodeNums = m_nodeNums[INDEX(PIPE_FLITE)];
    nodeNums[OUTPUT_NODE] = -1;
    nodeNums[CAPTURE_NODE_1] = m_getFliteNodenum();
    nodeNums[CAPTURE_NODE_2] = -1;
    controlId = m_sensorIds[INDEX(PIPE_FLITE)];
    controlId[CAPTURE_NODE_1] = m_getSensorId(nodeNums[CAPTURE_NODE_1], m_flagReprocessing);

    ret = m_setDeviceInfo();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_setDeviceInfo() fail", __FUNCTION__, __LINE__);
        return ret;
    }

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

    return NO_ERROR;
}

}; /* namespace android */
