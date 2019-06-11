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
#define LOG_TAG "ExynosCameraFrameFactory"
#include <cutils/log.h>

#include "ExynosCameraFrameFactory.h"

namespace android {

ExynosCameraFrameFactory::~ExynosCameraFrameFactory()
{
    int ret = 0;

    ret = destroy();
    if (ret < 0)
        CLOGE("ERR(%s[%d]):destroy fail", __FUNCTION__, __LINE__);
}

status_t ExynosCameraFrameFactory::destroy(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    for (int i = 0; i < MAX_NUM_PIPES; i++) {
        if (m_pipes[i] != NULL) {
            ret = m_pipes[i]->destroy();
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_pipes[%d]->destroy() fail", __FUNCTION__, __LINE__, i);
                if (m_shot_ext != NULL) {
                    delete m_shot_ext;
                    m_shot_ext = NULL;
                }
                return ret;
            }

            SAFE_DELETE(m_pipes[i]);

            CLOGD("DEBUG(%s):Pipe(%d) destroyed", __FUNCTION__, i);
        }
    }
    if (m_shot_ext != NULL) {
        delete m_shot_ext;
        m_shot_ext = NULL;
    }

    m_setCreate(false);

    return ret;
}

status_t ExynosCameraFrameFactory::setFrameManager(ExynosCameraFrameManager *manager)
{
    m_frameMgr = manager;
    return NO_ERROR;
}

status_t ExynosCameraFrameFactory::getFrameManager(ExynosCameraFrameManager **manager)
{
    *manager = m_frameMgr;
    return NO_ERROR;
}

bool ExynosCameraFrameFactory::isCreated(void)
{
    return m_getCreate();
}

status_t ExynosCameraFrameFactory::m_setCreate(bool create)
{
    Mutex::Autolock lock(m_createLock);
    CLOGD("DEBUG(%s[%d]) setCreate old(%s) new(%s)", __FUNCTION__, __LINE__, (m_create)?"true":"false", (create)?"true":"false");
    m_create = create;
    return NO_ERROR;
}

bool ExynosCameraFrameFactory::m_getCreate()
{
    Mutex::Autolock lock(m_createLock);
    return m_create;
}

int ExynosCameraFrameFactory::m_getFliteNodenum()
{
    int fliteNodeNim = FIMC_IS_VIDEO_SS0_NUM;

    fliteNodeNim = (m_cameraId == CAMERA_ID_BACK)?MAIN_CAMERA_FLITE_NUM:FRONT_CAMERA_FLITE_NUM;

    return fliteNodeNim;
}

int ExynosCameraFrameFactory::m_getSensorId(__unused unsigned int nodeNum, bool reprocessing)
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

int ExynosCameraFrameFactory::m_getSensorId(unsigned int nodeNum, bool flagOTFInterface, bool flagLeader, bool reprocessing)
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


status_t ExynosCameraFrameFactory::m_initFrameMetadata(ExynosCameraFrame *frame)
{
    int ret = 0;

    if (m_shot_ext == NULL) {
        CLOGE("ERR(%s[%d]): new struct camera2_shot_ext fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    memset(m_shot_ext, 0x0, sizeof(struct camera2_shot_ext));

    m_shot_ext->shot.magicNumber = SHOT_MAGIC_NUMBER;

    /* TODO: These bypass values are enabled at per-frame control */
#if 1
    m_bypassDRC = m_parameters->getDrcEnable();
    m_bypassDNR = m_parameters->getDnrEnable();
    m_bypassDIS = m_parameters->getDisEnable();
    m_bypassFD = m_parameters->getFdEnable();
#endif
    setMetaBypassDrc(m_shot_ext, m_bypassDRC);
    setMetaBypassDnr(m_shot_ext, m_bypassDNR);
    setMetaBypassDis(m_shot_ext, m_bypassDIS);
    setMetaBypassFd(m_shot_ext, m_bypassFD);

    ret = frame->initMetaData(m_shot_ext);
    if (ret < 0)
        CLOGE("ERR(%s[%d]): initMetaData fail", __FUNCTION__, __LINE__);

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

    return ret;
}

int ExynosCameraFrameFactory::setSrcNodeEmpty(int sensorId)
{
    return (sensorId & INPUT_STREAM_MASK) |
           (sensorId & INPUT_MODULE_MASK) |
           (0        & INPUT_VINDEX_MASK) |
           (sensorId & INPUT_MEMORY_MASK) |
           (sensorId & INPUT_LEADER_MASK);
}

int ExynosCameraFrameFactory::setLeader(int sensorId, bool flagLeader)
{
    return (sensorId & INPUT_STREAM_MASK) |
           (sensorId & INPUT_MODULE_MASK) |
           (sensorId & INPUT_VINDEX_MASK) |
           (sensorId & INPUT_MEMORY_MASK) |
           ((flagLeader)?1:0 & INPUT_LEADER_MASK);
}

ExynosCameraFrame *ExynosCameraFrameFactory::createNewFrameOnlyOnePipe(int pipeId, int frameCnt)
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

ExynosCameraFrame *ExynosCameraFrameFactory::createNewFrameVideoOnly(void)
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

status_t ExynosCameraFrameFactory::m_initPipelines(ExynosCameraFrame *frame)
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
                CLOGE("ERR(%s):getInputFrameQToPipe fail, ret(%d), frameQ(%p)", __FUNCTION__, ret, frameQ);
                return ret;
            }

            ret = setOutputFrameQToPipe(frameQ, curEntity->getPipeId());
            if (ret < 0) {
                CLOGE("ERR(%s):setOutputFrameQToPipe fail, ret(%d)", __FUNCTION__, ret);
                return ret;
            }

            if (childEntity->getPipeId() != PIPE_VRA) {
                /* check Image Configuration Equality */
                ret = m_checkPipeInfo(curEntity->getPipeId(), childEntity->getPipeId());
                if (ret < 0) {
                    CLOGE("ERR(%s):checkPipeInfo fail, Pipe[%d], Pipe[%d]", __FUNCTION__, curEntity->getPipeId(), childEntity->getPipeId());
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

status_t ExynosCameraFrameFactory::pushFrameToPipe(ExynosCameraFrame **newFrame, uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->pushFrame(newFrame);
    return NO_ERROR;
}

status_t ExynosCameraFrameFactory::setOutputFrameQToPipe(frame_queue_t *outputQ, uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->setOutputFrameQ(outputQ);
    return NO_ERROR;
}

status_t ExynosCameraFrameFactory::getOutputFrameQToPipe(frame_queue_t **outputQ, uint32_t pipeId)
{
    CLOGV("DEBUG(%s[%d]):pipeId=%d", __FUNCTION__, __LINE__, pipeId);
    m_pipes[INDEX(pipeId)]->getOutputFrameQ(outputQ);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactory::setFrameDoneQToPipe(frame_queue_t *frameDoneQ, uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->setFrameDoneQ(frameDoneQ);
    return NO_ERROR;
}

status_t ExynosCameraFrameFactory::getFrameDoneQToPipe(frame_queue_t **frameDoneQ, uint32_t pipeId)
{
    CLOGV("DEBUG(%s[%d]):pipeId=%d", __FUNCTION__, __LINE__, pipeId);
    m_pipes[INDEX(pipeId)]->getFrameDoneQ(frameDoneQ);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactory::getInputFrameQToPipe(frame_queue_t **inputFrameQ, uint32_t pipeId)
{
    CLOGV("DEBUG(%s[%d]):pipeId=%d", __FUNCTION__, __LINE__, pipeId);

    m_pipes[INDEX(pipeId)]->getInputFrameQ(inputFrameQ);

    if (inputFrameQ == NULL)
        CLOGE("ERR(%s[%d])inputFrameQ is NULL", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactory::setBufferManagerToPipe(ExynosCameraBufferManager **bufferManager, uint32_t pipeId)
{
    if (m_pipes[INDEX(pipeId)] == NULL) {
        CLOGE("ERR(%s[%d])m_pipes[INDEX(%d)] == NULL. pipeId(%d)", __FUNCTION__, __LINE__, INDEX(pipeId), pipeId);
        return INVALID_OPERATION;
    }

    return m_pipes[INDEX(pipeId)]->setBufferManager(bufferManager);
}

status_t ExynosCameraFrameFactory::getThreadState(int **threadState, uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->getThreadState(threadState);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactory::getThreadInterval(uint64_t **threadInterval, uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->getThreadInterval(threadInterval);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactory::getThreadRenew(int **threadRenew, uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->getThreadRenew(threadRenew);

    return NO_ERROR;
}

status_t ExynosCameraFrameFactory::incThreadRenew(uint32_t pipeId)
{
    m_pipes[INDEX(pipeId)]->incThreadRenew();

    return NO_ERROR;
}

status_t ExynosCameraFrameFactory::startThread(uint32_t pipeId)
{
    int ret = 0;

    CLOGI("INFO(%s[%d]):pipeId=%d", __FUNCTION__, __LINE__, pipeId);

    ret = m_pipes[INDEX(pipeId)]->startThread();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):start thread fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
        /* TODO: exception handling */
    }
    return ret;
}

status_t ExynosCameraFrameFactory::stopThread(uint32_t pipeId)
{
    int ret = 0;

    CLOGI("INFO(%s[%d]):pipeId=%d", __FUNCTION__, __LINE__, pipeId);

    if (m_pipes[INDEX(pipeId)] == NULL) {
        CLOGE("ERR(%s[%d]):m_pipes[INDEX(%d)] == NULL. so, fail", __FUNCTION__, __LINE__, pipeId);
        return INVALID_OPERATION;
    }

    ret = m_pipes[INDEX(pipeId)]->stopThread();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):stop thread fail, pipeId(%d), ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
        /* TODO: exception handling */
    }
    return ret;
}

status_t ExynosCameraFrameFactory::setStopFlag(void)
{
    CLOGE("ERR(%s[%d]):Must use the concreate class, don't use superclass", __FUNCTION__, __LINE__);
    return INVALID_OPERATION;
}

status_t ExynosCameraFrameFactory::stopThreadAndWait(uint32_t pipeId, int sleep, int times)
{
    status_t status = NO_ERROR;

    CLOGI("INFO(%s[%d]):pipeId=%d", __FUNCTION__, __LINE__, pipeId);
    status = m_pipes[INDEX(pipeId)]->stopThreadAndWait(sleep, times);
    if (status < 0) {
        CLOGE("ERR(%s[%d]):pipe(%d) stopThreadAndWait fail, ret(%d)", __FUNCTION__, __LINE__, pipeId);
        /* TODO: exception handling */
        status = INVALID_OPERATION;
    }
    return status;
}

status_t ExynosCameraFrameFactory::stopPipe(uint32_t pipeId)
{
    int ret = 0;

    ret = m_pipes[INDEX(pipeId)]->stopThread();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):Pipe:%d stopThread fail, ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
        /* TODO: exception handling */
        return INVALID_OPERATION;
    }

    ret = m_pipes[INDEX(pipeId)]->stop();
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):Pipe:%d stop fail, ret(%d)", __FUNCTION__, __LINE__, pipeId, ret);
        /* TODO: exception handling */
        /* return INVALID_OPERATION; */
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameFactory::stopPipes(void)
{
    CLOGE("ERR(%s[%d]):Must use the concreate class, don't use superclass", __FUNCTION__, __LINE__);
    return INVALID_OPERATION;
}

void ExynosCameraFrameFactory::dump()
{
    CLOGI("INFO(%s[%d]):", __FUNCTION__, __LINE__);

    for (int i = 0; i < MAX_NUM_PIPES; i++) {
        if (m_pipes[i] != NULL) {
            m_pipes[i]->dump();
        }
    }

    return;
}

void ExynosCameraFrameFactory::setRequest(int pipeId, bool enable)
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

void ExynosCameraFrameFactory::setRequestFLITE(bool enable)
{
#if 1
    m_requestFLITE = enable ? 1 : 0;
#else
    /* If not FLite->3AA OTF, FLite must be on */
    if (m_flagFlite3aaOTF == true) {
        m_requestFLITE = enable ? 1 : 0;
    } else {
        CLOGW("WRN(%s[%d]): isFlite3aaOtf (%d) == false). so Skip set m_requestFLITE(%d) as (%d)",
            __FUNCTION__, __LINE__, m_cameraId, m_requestFLITE, enable);
    }
#endif

}

void ExynosCameraFrameFactory::setRequest3AC(bool enable)
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

void ExynosCameraFrameFactory::setRequestISPC(bool enable)
{
    m_requestISPC = enable ? 1 : 0;
}

void ExynosCameraFrameFactory::setRequestISPP(bool enable)
{
    m_requestISPP = enable ? 1 : 0;
}


void ExynosCameraFrameFactory::setRequestSCC(bool enable)
{
    m_requestSCC = enable ? 1 : 0;
}

void ExynosCameraFrameFactory::setRequestDIS(bool enable)
{
    m_requestDIS = enable ? 1 : 0;
}

status_t ExynosCameraFrameFactory::setParam(struct v4l2_streamparm *streamParam, uint32_t pipeId)
{
    int ret = 0;

    ret = m_pipes[INDEX(pipeId)]->setParam(*streamParam);

    return ret;
}

status_t ExynosCameraFrameFactory::m_checkPipeInfo(uint32_t srcPipeId, uint32_t dstPipeId)
{
    int srcFullW, srcFullH, srcColorFormat;
    int dstFullW, dstFullH, dstColorFormat;
    int isDifferent = 0;
    int ret = 0;

    ret = m_pipes[INDEX(srcPipeId)]->getPipeInfo(&srcFullW, &srcFullH, &srcColorFormat, SRC_PIPE);
    if (ret < 0) {
        CLOGE("ERR(%s):Source getPipeInfo fail", __FUNCTION__);
        return ret;
    }
    ret = m_pipes[INDEX(dstPipeId)]->getPipeInfo(&dstFullW, &dstFullH, &dstColorFormat, DST_PIPE);
    if (ret < 0) {
        CLOGE("ERR(%s):Destination getPipeInfo fail", __FUNCTION__);
        return ret;
    }

    if (srcFullW != dstFullW || srcFullH != dstFullH || srcColorFormat != dstColorFormat) {
        CLOGE("ERR(%s[%d]):Video Node Image Configuration is NOT matching. so, fail", __FUNCTION__, __LINE__);

        CLOGE("ERR(%s[%d]):fail info : srcPipeId(%d), srcFullW(%d), srcFullH(%d), srcColorFormat(%d)",
            __FUNCTION__, __LINE__, srcPipeId, srcFullW, srcFullH, srcColorFormat);

        CLOGE("ERR(%s[%d]):fail info : dstPipeId(%d), dstFullW(%d), dstFullH(%d), dstColorFormat(%d)",
            __FUNCTION__, __LINE__, dstPipeId, dstFullW, dstFullH, dstColorFormat);

        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t ExynosCameraFrameFactory::dumpFimcIsInfo(uint32_t pipeId, bool bugOn)
{
    int ret = 0;
    int pipeIdIsp = 0;

    if (m_pipes[INDEX(pipeId)] != NULL)
        ret = m_pipes[INDEX(pipeId)]->dumpFimcIsInfo(bugOn);
    else
        ALOGE("ERR(%s): pipe is not ready (%d/%d)", __FUNCTION__, pipeId, bugOn);

    return ret;
}

#ifdef MONITOR_LOG_SYNC
status_t ExynosCameraFrameFactory::syncLog(uint32_t pipeId, uint32_t syncId)
{
    int ret = 0;
    int pipeIdIsp = 0;

    if (m_pipes[INDEX(pipeId)] != NULL)
        ret = m_pipes[INDEX(pipeId)]->syncLog(syncId);
    else
        ALOGE("ERR(%s): pipe is not ready (%d/%d)", __FUNCTION__, pipeId, syncId);

    return ret;
}
#endif

status_t ExynosCameraFrameFactory::setControl(int cid, int value, uint32_t pipeId)
{
    int ret = 0;

    ret = m_pipes[INDEX(pipeId)]->setControl(cid, value);

    return ret;
}

bool ExynosCameraFrameFactory::checkPipeThreadRunning(uint32_t pipeId)
{
    int ret = 0;

    ret = m_pipes[INDEX(pipeId)]->isThreadRunning();

    return ret;
}

status_t ExynosCameraFrameFactory::getControl(int cid, int *value, uint32_t pipeId)
{
    int ret = 0;

    ret = m_pipes[INDEX(pipeId)]->getControl(cid, value);

    return ret;
}

status_t ExynosCameraFrameFactory::m_checkNodeSetting(int pipeId)
{
    status_t ret = NO_ERROR;

    for (int i = 0; i < MAX_NODE; i++) {
        /* in case of wrong nodeNums set */
        if (m_deviceInfo[pipeId].nodeNum[i] != m_nodeNums[pipeId][i]) {
            CLOGE("ERR(%s[%d]):m_deviceInfo[%d].nodeNum[%d](%d) != m_nodeNums[%d][%d](%d). so, fail",
                __FUNCTION__, __LINE__,
                pipeId, i, m_deviceInfo[pipeId].nodeNum[i],
                pipeId, i, m_nodeNums[pipeId][i]);

            ret = BAD_VALUE;
            goto err;
        }

        /* in case of not set sensorId */
        if (0 < m_deviceInfo[pipeId].nodeNum[i] && m_sensorIds[pipeId][i] < 0) {
            CLOGE("ERR(%s[%d]):0 < m_deviceInfo[%d].nodeNum[%d](%d) && m_sensorIds[%d][%d](%d) < 0. so, fail",
                __FUNCTION__, __LINE__,
                pipeId, i, m_deviceInfo[pipeId].nodeNum[i],
                pipeId, i, m_sensorIds[pipeId][i]);

            ret = BAD_VALUE;
            goto err;
        }

        /* in case of strange set sensorId */
        if (m_deviceInfo[pipeId].nodeNum[i] < 0 && 0 < m_sensorIds[pipeId][i]) {
            CLOGE("ERR(%s[%d]):m_deviceInfo[%d].nodeNum[%d](%d) < 0 && 0 < m_sensorIds[%d][%d](%d). so, fail",
                __FUNCTION__, __LINE__,
                pipeId, i, m_deviceInfo[pipeId].nodeNum[i],
                pipeId, i, m_sensorIds[pipeId][i]);

            ret = BAD_VALUE;
            goto err;
        }

        /* in case of not set secondarySensorId */
        if (0 < m_deviceInfo[pipeId].secondaryNodeNum[i] && m_secondarySensorIds[pipeId][i] < 0) {
            CLOGE("ERR(%s[%d]):0 < m_deviceInfo[%d].secondaryNodeNum[%d](%d) && m_secondarySensorIds[%d][%d](%d) < 0. so, fail",
                __FUNCTION__, __LINE__,
                pipeId, i, m_deviceInfo[pipeId].secondaryNodeNum[i],
                pipeId, i, m_secondarySensorIds[pipeId][i]);

            ret = BAD_VALUE;
            goto err;
        }

        /* in case of strange set secondarySensorId */
        if (m_deviceInfo[pipeId].secondaryNodeNum[i] < 0 && 0 < m_secondarySensorIds[pipeId][i]) {
            CLOGE("ERR(%s[%d]):m_deviceInfo[%d].secondaryNodeNum[%d](%d) < 0 && 0 < m_secondarySensorIds[%d][%d](%d). so, fail",
                __FUNCTION__, __LINE__,
                pipeId, i, m_deviceInfo[pipeId].secondaryNodeNum[i],
                pipeId, i, m_secondarySensorIds[pipeId][i]);

            ret = BAD_VALUE;
            goto err;
        }
    }

err:
    return ret;
}

enum NODE_TYPE ExynosCameraFrameFactory::getNodeType(uint32_t pipeId)
{
    android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Unexpected call.. pipe_id(%d), assert!!!!", __FUNCTION__, __LINE__, pipeId);

    return INVALID_NODE;
};

int ExynosCameraFrameFactory::m_initFlitePipe(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    int ret = 0;
    camera_pipe_info_t pipeInfo[MAX_NODE];
    camera_pipe_info_t nullPipeInfo;

    ExynosRect tempRect;
    int maxSensorW = 0, maxSensorH = 0, hwSensorW = 0, hwSensorH = 0;
    int bayerFormat = CAMERA_BAYER_FORMAT;

#ifdef DEBUG_RAWDUMP
    if (m_parameters->checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    struct ExynosConfigInfo *config = m_parameters->getConfig();

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

    return NO_ERROR;
}

void ExynosCameraFrameFactory::m_initDeviceInfo(int pipeId)
{
    camera_device_info_t nullDeviceInfo;

    m_deviceInfo[pipeId] = nullDeviceInfo;

    for (int i = 0; i < MAX_NODE; i++) {
        // set nodeNum
        m_nodeNums[pipeId][i] = m_deviceInfo[pipeId].nodeNum[i];

        // set default sensorId
        m_sensorIds[pipeId][i]  = -1;

        // set second sensorId
        m_secondarySensorIds[pipeId][i]  = -1;
    }
}

void ExynosCameraFrameFactory::m_init(void)
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
    m_requestFLITE = 1;

    m_request3AP = 1;
    m_request3AC = 0;
    m_requestISP = 1;

    m_requestISPC = 0;
    m_requestISPP = 0;
    m_requestSCC = 0;

    m_requestDIS = 0;
    m_requestSCP = 1;

    m_requestVRA = 0;

    m_requestJPEG = 0;
    m_requestThumbnail = 0;

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
    m_shot_ext = new struct camera2_shot_ext;
}

}; /* namespace android */
