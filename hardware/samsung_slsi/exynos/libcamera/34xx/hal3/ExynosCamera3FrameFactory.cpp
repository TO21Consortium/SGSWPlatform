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
#define LOG_TAG "ExynosCamera3FrameFactory"
#include <cutils/log.h>

#include "ExynosCamera3FrameFactory.h"

namespace android {

ExynosCamera3FrameFactory::~ExynosCamera3FrameFactory()
{
    int ret = 0;

    ret = destroy();
    if (ret < 0)
        CLOGE2("destroy fail");
}

status_t ExynosCamera3FrameFactory::destroy(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    for (int i = 0; i < MAX_NUM_PIPES; i++) {
        if (m_pipes[i] != NULL) {
            ret = m_pipes[i]->destroy();
            if (ret != NO_ERROR) {
                CLOGE2("m_pipes[%d]->destroy() fail", i);
                return ret;
            }

            SAFE_DELETE(m_pipes[i]);

            CLOGD2("Pipe(%d) destroyed", i);
        }
    }

    m_setCreate(false);

    return ret;
}

status_t ExynosCamera3FrameFactory::setFrameManager(ExynosCameraFrameManager *manager)
{
    m_frameMgr = manager;
    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::getFrameManager(ExynosCameraFrameManager **manager)
{
    *manager = m_frameMgr;
    return NO_ERROR;
}

bool ExynosCamera3FrameFactory::isCreated(void)
{
    return m_getCreate();
}

status_t ExynosCamera3FrameFactory::m_setCreate(bool create)
{
    Mutex::Autolock lock(m_createLock);
    CLOGD2("setCreate old(%s) new(%s)", (m_create)?"true":"false", (create)?"true":"false");
    m_create = create;
    return NO_ERROR;
}

bool ExynosCamera3FrameFactory::m_getCreate()
{
    Mutex::Autolock lock(m_createLock);
    return m_create;
}

int ExynosCamera3FrameFactory::m_getFliteNodenum()
{
    int fliteNodeNim = FIMC_IS_VIDEO_SS0_NUM;

    fliteNodeNim = (m_cameraId == CAMERA_ID_BACK)?MAIN_CAMERA_FLITE_NUM:FRONT_CAMERA_FLITE_NUM;

    return fliteNodeNim;
}

int ExynosCamera3FrameFactory::m_getSensorId(__unused unsigned int nodeNum, bool reprocessing)
{
    unsigned int reprocessingBit = 0;
    unsigned int nodeNumBit = 0;
    unsigned int sensorIdBit = 0;
    unsigned int sensorId = getSensorId(m_cameraId);

    if (reprocessing == true)
        reprocessingBit = (1 << REPROCESSING_SHIFT);

    /*
     * hack
     * nodeNum - FIMC_IS_VIDEO_BAS_NUM is proper.
     * but, historically, FIMC_IS_VIDEO_SS0_NUM - FIMC_IS_VIDEO_SS0_NUM is worked properly
     */
    //nodeNumBit = ((nodeNum - FIMC_IS_VIDEO_BAS_NUM) << SSX_VINDEX_SHIFT);
    nodeNumBit = ((FIMC_IS_VIDEO_SS0_NUM - FIMC_IS_VIDEO_SS0_NUM) << SSX_VINDEX_SHIFT);

    sensorIdBit = (sensorId << 0);

    return (reprocessingBit) |
           (nodeNumBit) |
           (sensorIdBit);
}

int ExynosCamera3FrameFactory::m_getSensorId(unsigned int nodeNum, bool flagOTFInterface, bool flagLeader, bool reprocessing)
{
    /* sub 100, and make index */
    nodeNum -= 100;

    unsigned int reprocessingBit = 0;
    unsigned int otfInterfaceBit = 0;
    unsigned int leaderBit = 0;
    unsigned int sensorId = getSensorId(m_cameraId);

    if (reprocessing == true)
        reprocessingBit = 1;

    if (flagLeader == true)
        leaderBit = 1;

    if (flagOTFInterface == true)
        otfInterfaceBit = 1;

    return ((reprocessingBit << INPUT_STREAM_SHIFT) & INPUT_STREAM_MASK) |
           ((sensorId        << INPUT_MODULE_SHIFT) & INPUT_MODULE_MASK) |
           ((nodeNum         << INPUT_VINDEX_SHIFT) & INPUT_VINDEX_MASK) |
           ((otfInterfaceBit << INPUT_MEMORY_SHIFT) & INPUT_MEMORY_MASK) |
           ((leaderBit       << INPUT_LEADER_SHIFT) & INPUT_LEADER_MASK);
}


status_t ExynosCamera3FrameFactory::m_initFrameMetadata(ExynosCameraFrame *frame)
{
    int ret = 0;
    struct camera2_shot_ext *shot_ext = new struct camera2_shot_ext;

    if (shot_ext == NULL) {
        CLOGE2("new struct camera2_shot_ext fail");
        return INVALID_OPERATION;
    }

    memset(shot_ext, 0x0, sizeof(struct camera2_shot_ext));

    shot_ext->shot.magicNumber = SHOT_MAGIC_NUMBER;

    /* TODO: These bypass values are enabled at per-frame control */
#if 1
    m_bypassDRC = m_parameters->getDrcEnable();
    m_bypassDNR = m_parameters->getDnrEnable();
    m_bypassDIS = m_parameters->getDisEnable();
    m_bypassFD = m_parameters->getFdEnable();
#endif
    setMetaBypassDrc(shot_ext, m_bypassDRC);
    setMetaBypassDnr(shot_ext, m_bypassDNR);
    setMetaBypassDis(shot_ext, m_bypassDIS);
    setMetaBypassFd(shot_ext, m_bypassFD);

    ret = frame->initMetaData(shot_ext);
    if (ret < 0)
        CLOGE2("initMetaData fail");

    frame->setRequest(m_request3AP,
                      m_request3AC,
                      m_requestISP,
                      m_requestISPP,
                      m_requestISPC,
                      m_requestSCC,
                      m_requestDIS,
                      m_requestSCP);

    if (m_flagReprocessing == true) {
        frame->setRequest(PIPE_MCSC0_REPROCESSING, m_requestSCP);
        frame->setRequest(PIPE_HWFC_JPEG_SRC_REPROCESSING, m_requestJPEG);
        frame->setRequest(PIPE_HWFC_JPEG_DST_REPROCESSING, m_requestJPEG);
        frame->setRequest(PIPE_HWFC_THUMB_SRC_REPROCESSING, m_requestThumbnail);
        frame->setRequest(PIPE_HWFC_THUMB_DST_REPROCESSING, m_requestThumbnail);
    }

    delete shot_ext;
    shot_ext = NULL;

    return ret;
}

int ExynosCamera3FrameFactory::setSrcNodeEmpty(int sensorId)
{
    return (sensorId & INPUT_STREAM_MASK) |
           (sensorId & INPUT_MODULE_MASK) |
           (0        & INPUT_VINDEX_MASK) |
           (sensorId & INPUT_MEMORY_MASK) |
           (sensorId & INPUT_LEADER_MASK);
}

int ExynosCamera3FrameFactory::setLeader(int sensorId, bool flagLeader)
{
    return (sensorId & INPUT_STREAM_MASK) |
           (sensorId & INPUT_MODULE_MASK) |
           (sensorId & INPUT_VINDEX_MASK) |
           (sensorId & INPUT_MEMORY_MASK) |
           ((flagLeader)?1:0 & INPUT_LEADER_MASK);
}

ExynosCameraFrame *ExynosCamera3FrameFactory::createNewFrameOnlyOnePipe(int pipeId, int frameCnt)
{
    Mutex::Autolock lock(m_frameLock);
    int ret = 0;
    ExynosCameraFrameEntity *newEntity[MAX_NUM_PIPES] = {};

    if (frameCnt < 0) {
        frameCnt = m_frameCount;
    }

    ExynosCameraFrame *frame = m_frameMgr->createFrame(m_parameters, frameCnt);
    if (frame == NULL)
        return NULL;

    /* set pipe to linkageList */
    newEntity[INDEX(pipeId)] = new ExynosCameraFrameEntity(pipeId, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(pipeId)]);

    return frame;
}

ExynosCameraFrame *ExynosCamera3FrameFactory::createNewFrameVideoOnly(void)
{
    int ret = 0;
    ExynosCameraFrameEntity *newEntity[MAX_NUM_PIPES] = {};
    ExynosCameraFrame *frame = m_frameMgr->createFrame(m_parameters, m_frameCount);
    if (frame == NULL)
        return NULL;

    /* set GSC-Video pipe to linkageList */
    newEntity[INDEX(PIPE_GSC_VIDEO)] = new ExynosCameraFrameEntity(PIPE_GSC_VIDEO, ENTITY_TYPE_INPUT_OUTPUT, ENTITY_BUFFER_FIXED);
    frame->addSiblingEntity(NULL, newEntity[INDEX(PIPE_GSC_VIDEO)]);

    return frame;
}

status_t ExynosCamera3FrameFactory::m_initPipelines(ExynosCameraFrame *frame)
{
    ExynosCameraFrameEntity *curEntity = NULL;
    ExynosCameraFrameEntity *childEntity = NULL;
    frame_queue_t *frameQ = NULL;
    int ret = 0;

    curEntity = frame->getFirstEntity();

    while (curEntity != NULL) {
        childEntity = curEntity->getNextEntity();
        if (childEntity != NULL) {
            ret = getInputFrameQToPipe(&frameQ, childEntity->getPipeId());
            if (ret < 0 || frameQ == NULL) {
                CLOGE2("getInputFrameQToPipe fail, ret(%d), frameQ(%p)", ret, frameQ);
                return ret;
            }

            ret = setOutputFrameQToPipe(frameQ, curEntity->getPipeId());
            if (ret < 0) {
                CLOGE2("setOutputFrameQToPipe fail, ret(%d)", ret);
                return ret;
            }

            if (childEntity->getPipeId() != PIPE_VRA) {
                /* check Image Configuration Equality */
                ret = m_checkPipeInfo(curEntity->getPipeId(), childEntity->getPipeId());
                if (ret < 0) {
                    CLOGE2("checkPipeInfo fail, Pipe[%d], Pipe[%d]", curEntity->getPipeId(), childEntity->getPipeId());
                    return ret;
                }
            }

            curEntity = childEntity;
        } else {
            curEntity = frame->getNextEntity();
        }
    }

    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::pushFrameToPipe(ExynosCameraFrame **newFrame, uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->pushFrame(newFrame);
    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::setOutputFrameQToPipe(frame_queue_t *outputQ, uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->setOutputFrameQ(outputQ);
    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::getOutputFrameQToPipe(frame_queue_t **outputQ, uint32_t pipeId)
{
    CLOGV2("pipeId=%d", pipeId);
    m_pipes[INDEX(pipeId)]->getOutputFrameQ(outputQ);

    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::setFrameDoneQToPipe(frame_queue_t *frameDoneQ, uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->setFrameDoneQ(frameDoneQ);
    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::getFrameDoneQToPipe(frame_queue_t **frameDoneQ, uint32_t pipeId)
{
    CLOGV2("pipeId=%d", pipeId);
    m_pipes[INDEX(pipeId)]->getFrameDoneQ(frameDoneQ);

    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::getInputFrameQToPipe(frame_queue_t **inputFrameQ, uint32_t pipeId)
{
    CLOGV2("pipeId=%d", pipeId);

    m_pipes[INDEX(pipeId)]->getInputFrameQ(inputFrameQ);

    if (inputFrameQ == NULL)
        CLOGE2("inputFrameQ is NULL");

    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::setBufferManagerToPipe(ExynosCameraBufferManager **bufferManager, uint32_t pipeId)
{
    if (m_pipes[INDEX(pipeId)] == NULL) {
        CLOGE2("m_pipes[%d] is NULL. pipeId(%d)", INDEX(pipeId), pipeId);
        return INVALID_OPERATION;
    }

    return m_pipes[INDEX(pipeId)]->setBufferManager(bufferManager);
}

status_t ExynosCamera3FrameFactory::getThreadState(int **threadState, uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->getThreadState(threadState);

    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::getThreadInterval(uint64_t **threadInterval, uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->getThreadInterval(threadInterval);

    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::getThreadRenew(int **threadRenew, uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->getThreadRenew(threadRenew);

    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::incThreadRenew(uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->incThreadRenew();

    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::preparePipes(__unused uint32_t prepareCnt)
{
    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::startThread(uint32_t pipeId)
{
    int ret = 0;

    CLOGI2("pipeId=%d", pipeId);

    ret = m_pipes[INDEX(pipeId)]->startThread();
    if (ret < 0) {
        CLOGE2("start thread fail, pipeId(%d), ret(%d)", pipeId, ret);
        /* TODO: exception handling */
    }
    return ret;
}

status_t ExynosCamera3FrameFactory::stopThread(uint32_t pipeId)
{
    int ret = 0;

    CLOGI2("pipeId=%d", pipeId);

    if (m_pipes[INDEX(pipeId)] == NULL) {
        CLOGE2("m_pipes[INDEX(%d)] == NULL. so, fail", pipeId);
        return INVALID_OPERATION;
    }

    ret = m_pipes[INDEX(pipeId)]->stopThread();
    if (ret < 0) {
        CLOGE2("stop thread fail, pipeId(%d), ret(%d)", pipeId, ret);
        /* TODO: exception handling */
    }
    return ret;
}

status_t ExynosCamera3FrameFactory::stopThreadAndWait(uint32_t pipeId, int sleep, int times)
{
    status_t status = NO_ERROR;

    CLOGI2("pipeId=%d", pipeId);
    status = m_pipes[INDEX(pipeId)]->stopThreadAndWait(sleep, times);
    if (status < 0) {
        CLOGE2("pipe(%d) stopThreadAndWait fail, ret(%d)", pipeId);
        /* TODO: exception handling */
        status = INVALID_OPERATION;
    }
    return status;
}

status_t ExynosCamera3FrameFactory::setStopFlag(void)
{
    CLOGE2("Must use the concreate class, don't use superclass");
    return INVALID_OPERATION;
}

status_t ExynosCamera3FrameFactory::stopPipe(uint32_t pipeId)
{
    int ret = 0;

    ret = m_pipes[INDEX(pipeId)]->stopThread();
    if (ret < 0) {
        CLOGE2("Pipe:%d stopThread fail, ret(%d)", pipeId, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    ret = m_pipes[INDEX(pipeId)]->stop();
    if (ret < 0) {
        CLOGE2("Pipe:%d stop fail, ret(%d)", pipeId, ret);
        /* TODO: exception handling */
        /* return INVALID_OPERATION; */
    }

    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::stopPipes(void)
{
    CLOGE2("Must use the concreate class, don't use superclass");
    return INVALID_OPERATION;
}

void ExynosCamera3FrameFactory::dump()
{
    CLOGI("INFO(%s[%d]):", __FUNCTION__, __LINE__);

    for (int i = 0; i < MAX_NUM_PIPES; i++) {
        if (m_pipes[i] != NULL) {
            m_pipes[i]->dump();
        }
    }

    return;
}

void ExynosCamera3FrameFactory::setRequest(int pipeId, bool enable)
{
    switch (pipeId) {
    case PIPE_FLITE:
    case PIPE_FLITE_REPROCESSING:
        m_requestFLITE = enable ? 1 : 0;
        break;
    case PIPE_3AC:
    case PIPE_3AC_REPROCESSING:
        m_request3AC = enable ? 1 : 0;
        break;
    case PIPE_3AP:
    case PIPE_3AP_REPROCESSING:
        m_request3AP = enable ? 1 : 0;
        break;
    case PIPE_ISPC:
    case PIPE_ISPC_REPROCESSING:
        m_requestISPC = enable ? 1 : 0;
        break;
    case PIPE_ISPP:
    case PIPE_ISPP_REPROCESSING:
        m_requestISPP = enable ? 1 : 0;
        break;
    case PIPE_MCSC0:
    case PIPE_MCSC0_REPROCESSING:
        m_requestSCP = enable ? 1 : 0;
        break;
    case PIPE_HWFC_JPEG_SRC_REPROCESSING:
    case PIPE_HWFC_JPEG_DST_REPROCESSING:
        m_requestJPEG = enable ? 1 : 0;
        break;
    case PIPE_HWFC_THUMB_SRC_REPROCESSING:
    case PIPE_HWFC_THUMB_DST_REPROCESSING:
        m_requestThumbnail = enable ? 1 : 0;
        break;
    default:
        CLOGW("WRN(%s[%d]):Invalid pipeId(%d)", __FUNCTION__, __LINE__, pipeId);
        break;
    }
}

void ExynosCamera3FrameFactory::setRequestFLITE(bool enable)
{
#if 1
    m_requestFLITE = enable ? 1 : 0;
#else
    /* If not FLite->3AA OTF, FLite must be on */
    if (m_flagFlite3aaOTF == true) {
        m_requestFLITE = enable ? 1 : 0;
    } else {
        CLOGW2("isFlite3aaOtf (%d) == false). so Skip set m_requestFLITE(%d) as (%d)", m_cameraId, m_requestFLITE, enable);
    }
#endif

}

void ExynosCamera3FrameFactory::setRequest3AC(bool enable)
{
#if 1
    m_request3AC = enable ? 1 : 0;
#else
    /* From 74xx, Front will use reprocessing. so, we need to prepare BDS */
    if (isReprocessing(m_cameraId) == true) {
        if (m_parameters->getUsePureBayerReprocessing() == true) {
            m_request3AC = 0;
        } else {
            m_request3AC = enable ? 1 : 0;
        }
    } else {
        m_request3AC = 0;
    }
#endif
}

void ExynosCamera3FrameFactory::setRequestISPC(bool enable)
{
    m_requestISPC = enable ? 1 : 0;
}

void ExynosCamera3FrameFactory::setRequestISPP(bool enable)
{
    m_requestISPP = enable ? 1 : 0;
}

void ExynosCamera3FrameFactory::setRequestSCC(bool enable)
{
    m_requestSCC = enable ? 1 : 0;
}

void ExynosCamera3FrameFactory::setRequestSCP(bool enable)
{
    m_requestSCP = enable ? 1 : 0;
}

void ExynosCamera3FrameFactory::setRequestDIS(bool enable)
{
    m_requestDIS = enable ? 1 : 0;
}

status_t ExynosCamera3FrameFactory::setParam(struct v4l2_streamparm *streamParam, uint32_t pipeId)
{
    int ret = 0;

    ret = m_pipes[INDEX(pipeId)]->setParam(*streamParam);

    return ret;
}

status_t ExynosCamera3FrameFactory::m_checkPipeInfo(uint32_t srcPipeId, uint32_t dstPipeId)
{
    int srcFullW, srcFullH, srcColorFormat;
    int dstFullW, dstFullH, dstColorFormat;
    int isDifferent = 0;
    int ret = 0;

    ret = m_pipes[INDEX(srcPipeId)]->getPipeInfo(&srcFullW, &srcFullH, &srcColorFormat, SRC_PIPE);
    if (ret < 0) {
        CLOGE2("Source getPipeInfo fail");
        return ret;
    }
    ret = m_pipes[INDEX(dstPipeId)]->getPipeInfo(&dstFullW, &dstFullH, &dstColorFormat, DST_PIPE);
    if (ret < 0) {
        CLOGE2("Destination getPipeInfo fail");
        return ret;
    }

    if (srcFullW != dstFullW || srcFullH != dstFullH || srcColorFormat != dstColorFormat) {
        CLOGE2("Video Node Image Configuration is NOT matching. so, fail");

        CLOGE2("fail info : srcPipeId(%d), srcFullW(%d), srcFullH(%d), srcColorFormat(%d)",
            srcPipeId, srcFullW, srcFullH, srcColorFormat);

        CLOGE2("fail info : dstPipeId(%d), dstFullW(%d), dstFullH(%d), dstColorFormat(%d)",
            dstPipeId, dstFullW, dstFullH, dstColorFormat);

        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t ExynosCamera3FrameFactory::dumpFimcIsInfo(uint32_t pipeId, bool bugOn)
{
    int ret = 0;
    int pipeIdIsp = 0;

    if (m_pipes[INDEX(pipeId)] != NULL)
        ret = m_pipes[INDEX(pipeId)]->dumpFimcIsInfo(bugOn);
    else
        CLOGE2("pipe is not ready (%d/%d)", pipeId, bugOn);

    return ret;
}

#ifdef MONITOR_LOG_SYNC
status_t ExynosCamera3FrameFactory::syncLog(uint32_t pipeId, uint32_t syncId)
{
    int ret = 0;
    int pipeIdIsp = 0;

    if (m_pipes[INDEX(pipeId)] != NULL)
        ret = m_pipes[INDEX(pipeId)]->syncLog(syncId);
    else
        CLOGE2("pipe is not ready (%d/%d)", pipeId, syncId);

    return ret;
}
#endif

status_t ExynosCamera3FrameFactory::setControl(int cid, int value, uint32_t pipeId)
{
    int ret = 0;

    ret = m_pipes[INDEX(pipeId)]->setControl(cid, value);

    return ret;
}

bool ExynosCamera3FrameFactory::checkPipeThreadRunning(uint32_t pipeId)
{
    int ret = 0;

    ret = m_pipes[INDEX(pipeId)]->isThreadRunning();

    return ret;
}

status_t ExynosCamera3FrameFactory::getControl(int cid, int *value, uint32_t pipeId)
{
    int ret = 0;

    ret = m_pipes[INDEX(pipeId)]->getControl(cid, value);

    return ret;
}

status_t ExynosCamera3FrameFactory::m_checkNodeSetting(int pipeId)
{
    status_t ret = NO_ERROR;

    for (int i = 0; i < MAX_NODE; i++) {
        /* in case of wrong nodeNums set */
        if (m_nodeInfo[pipeId].nodeNum[i] != m_nodeNums[pipeId][i]) {
            CLOGE2("m_nodeInfo[%d].nodeNum[%d](%d) != m_nodeNums[%d][%d](%d). so, fail",
                pipeId, i, m_nodeInfo[pipeId].nodeNum[i],
                pipeId, i, m_nodeNums[pipeId][i]);

            ret = BAD_VALUE;
            goto err;
        }

        /* in case of not set sensorId */
        if (0 < m_nodeInfo[pipeId].nodeNum[i] && m_sensorIds[pipeId][i] < 0) {
            CLOGE2("0 < m_nodeInfo[%d].nodeNum[%d](%d) && m_sensorIds[%d][%d](%d) < 0. so, fail",
                pipeId, i, m_nodeInfo[pipeId].nodeNum[i],
                pipeId, i, m_sensorIds[pipeId][i]);

            ret = BAD_VALUE;
            goto err;
        }

        /* in case of strange set sensorId */
        if (m_nodeInfo[pipeId].nodeNum[i] < 0 && 0 < m_sensorIds[pipeId][i]) {
            CLOGE2("m_nodeInfo[%d].nodeNum[%d](%d) < 0 && 0 < m_sensorIds[%d][%d](%d). so, fail",
                pipeId, i, m_nodeInfo[pipeId].nodeNum[i],
                pipeId, i, m_sensorIds[pipeId][i]);

            ret = BAD_VALUE;
            goto err;
        }

        /* in case of not set secondarySensorId */
        if (0 < m_nodeInfo[pipeId].secondaryNodeNum[i] && m_secondarySensorIds[pipeId][i] < 0) {
            CLOGE2("0 < m_nodeInfo[%d].secondaryNodeNum[%d](%d) && m_secondarySensorIds[%d][%d](%d) < 0. so, fail",
                pipeId, i, m_nodeInfo[pipeId].secondaryNodeNum[i],
                pipeId, i, m_secondarySensorIds[pipeId][i]);

            ret = BAD_VALUE;
            goto err;
        }

        /* in case of strange set secondarySensorId */
        if (m_nodeInfo[pipeId].secondaryNodeNum[i] < 0 && 0 < m_secondarySensorIds[pipeId][i]) {
            CLOGE2("m_nodeInfo[%d].secondaryNodeNum[%d](%d) < 0 && 0 < m_secondarySensorIds[%d][%d](%d). so, fail",
                pipeId, i, m_nodeInfo[pipeId].secondaryNodeNum[i],
                pipeId, i, m_secondarySensorIds[pipeId][i]);

            ret = BAD_VALUE;
            goto err;
        }
    }

err:
    return ret;
}

int ExynosCamera3FrameFactory::m_initFlitePipe(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    int ret = 0;
    camera_pipe_info_t pipeInfo[MAX_NODE];
    camera_pipe_info_t nullPipeInfo;
    enum NODE_TYPE nodeType = (enum NODE_TYPE)0;

    ExynosRect tempRect;
    int maxSensorW = 0, maxSensorH = 0, hwSensorW = 0, hwSensorH = 0;
    int bayerFormat = m_parameters->getBayerFormat(PIPE_FLITE);

#ifdef DEBUG_RAWDUMP
    if (m_parameters->checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    struct ExynosConfigInfo *config = m_parameters->getConfig();

    m_parameters->getMaxSensorSize(&maxSensorW, &maxSensorH);
    m_parameters->getHwSensorSize(&hwSensorW, &hwSensorH);

    CLOGI2("MaxSensorSize(%dx%d), HWSensorSize(%dx%d)", maxSensorW, maxSensorH, hwSensorW, hwSensorH);

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
    CLOGI2("set framerate (denominator=%d)", frameRate);
    ret = setParam(&streamParam, PIPE_FLITE);
    if (ret < 0) {
        CLOGE2("FLITE setParam fail, ret(%d)", ret);
        return INVALID_OPERATION;
    }

    tempRect.fullW = hwSensorW;
    tempRect.fullH = hwSensorH;
    tempRect.colorFormat = bayerFormat;

    pipeInfo[nodeType].rectInfo = tempRect;
    pipeInfo[nodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    pipeInfo[nodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;
    pipeInfo[nodeType].bufInfo.count = MAX_BUFFERS;
    /* per frame info */
    pipeInfo[nodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = 0;
    pipeInfo[nodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_NONE;

    /* set v4l2 video node bytes per plane */
    switch (bayerFormat) {
    case V4L2_PIX_FMT_SBGGR16:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
        break;
    case V4L2_PIX_FMT_SBGGR12:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 3 / 2), CAMERA_16PX_ALIGN);
        break;
    case V4L2_PIX_FMT_SBGGR10:
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 5 / 4), CAMERA_16PX_ALIGN);
        break;
    default:
        CLOGW("WRN(%s[%d]):Invalid bayer format(%d)", __FUNCTION__, __LINE__, bayerFormat);
        pipeInfo[nodeType].bytesPerPlane[0] = ROUND_UP((tempRect.fullW * 2), CAMERA_16PX_ALIGN);
        break;
    }

    ret = m_pipes[INDEX(PIPE_FLITE)]->setupPipe(pipeInfo, m_sensorIds[INDEX(PIPE_FLITE)]);
    if (ret < 0) {
        CLOGE2("FLITE setupPipe fail, ret(%d)", ret);
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
        CLOGE2("set BNS(%d) fail, ret(%d)", bnsScaleRatio, ret);
    } else {
        ret = m_pipes[INDEX(PIPE_FLITE)]->getControl(V4L2_CID_IS_G_BNS_SIZE, &bnsSize);
        if (ret < 0) {
            CLOGE2("get BNS size fail, ret(%d)", ret);
            bnsSize = -1;
        }
    }

    int bnsWidth = 0;
    int bnsHeight = 0;
    if (bnsSize > 0) {
        bnsHeight = bnsSize & 0xffff;
        bnsWidth = bnsSize >> 16;

        CLOGI2("BNS scale down ratio(%.1f), size (%dx%d)", (float)(bnsScaleRatio / 1000), bnsWidth, bnsHeight);
        m_parameters->setBnsSize(bnsWidth, bnsHeight);
    }

    return NO_ERROR;
}


/* added by 3.2 HAL */
status_t ExynosCamera3FrameFactory::setFrameCreateHandler(factory_handler_t handler)
{
    status_t ret = NO_ERROR;
    m_frameCreateHandler = handler;
    return ret;
}

/* added by 3.2 HAL */
factory_handler_t ExynosCamera3FrameFactory::getFrameCreateHandler()
{
    return m_frameCreateHandler;
}

status_t ExynosCamera3FrameFactory::setFrameDoneHandler(factory_donehandler_t handler)
{
    status_t ret = NO_ERROR;
    m_frameDoneHandler = handler;
    return ret;
}

/* added by 3.2 HAL */
factory_donehandler_t ExynosCamera3FrameFactory::getFrameDoneHandler()
{
    return m_frameDoneHandler;
}

void ExynosCamera3FrameFactory::m_initDeviceInfo(int pipeId)
{
    camera_device_info_t nullDeviceInfo;

    m_nodeInfo[pipeId] = nullDeviceInfo;

    for (int i = 0; i < MAX_NODE; i++) {
        // set nodeNum
        m_nodeNums[pipeId][i] = m_nodeInfo[pipeId].nodeNum[i];

        // set default sensorId
        m_sensorIds[pipeId][i]  = -1;

        // set second sensorId
        m_secondarySensorIds[pipeId][i]  = -1;
    }
}

void ExynosCamera3FrameFactory::m_init(void)
{
    m_cameraId = 0;
    memset(m_name, 0x00, sizeof(m_name));
    m_frameCount = 0;

    memset(m_nodeNums, -1, sizeof(m_nodeNums));
    memset(m_sensorIds, -1, sizeof(m_sensorIds));
    memset(m_secondarySensorIds, -1, sizeof(m_secondarySensorIds));

    for (int i = 0; i < MAX_NUM_PIPES; i++)
        m_pipes[i] = NULL;

    /* setting about request */
    m_requestFLITE = 0;

    m_request3AP = 0;
    m_request3AC = 0;
    m_requestISP = 1;

    m_requestISPP = 0;
    m_requestISPC = 0;
    m_requestSCC = 0;

    m_requestDIS = 0;
    m_requestSCP = 1;

    m_requestVRA = 0;

    /* setting about bypass */
    m_bypassDRC = true;
    m_bypassDIS = true;
    m_bypassDNR = true;
    m_bypassFD = true;

    m_setCreate(false);

    m_flagFlite3aaOTF = false;
    m_flag3aaIspOTF = false;
    m_flagIspTpuOTF = false;
    m_flagIspMcscOTF = false;
    m_flagTpuMcscOTF = false;
    m_flagMcscVraOTF = false;
    m_supportReprocessing = false;
    m_flagReprocessing = false;
    m_supportPureBayerReprocessing = false;
    m_supportSCC = false;
    m_supportMCSC = false;

#ifdef USE_INTERNAL_FRAME
    /* internal frame */
    //m_internalFrameCount = 0;
#endif /* #ifdef USE_INTERNAL_FRAME */
}

}; /* namespace android */
