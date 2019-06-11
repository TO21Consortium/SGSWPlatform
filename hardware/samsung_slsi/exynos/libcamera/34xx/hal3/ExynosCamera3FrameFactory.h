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

#ifndef EXYNOS_CAMERA3_FRAME_FACTORY_H
#define EXYNOS_CAMERA3_FRAME_FACTORY_H

#include "ExynosCameraConfig.h"

#include "ExynosCameraFrame.h"
#include "ExynosCameraPipe.h"
#include "ExynosCameraMCPipe.h"
#include "ExynosCameraPipeFlite.h"
#include "ExynosCameraPipeVRA.h"
#include "ExynosCameraPipeGSC.h"
#include "ExynosCameraPipeJpeg.h"
#include "ExynosCameraFrameManager.h"

#include "ExynosCamera3Parameters.h"

#include "ExynosCameraUtilsModule.h"

namespace android {

#define SET_OUTPUT_DEVICE_BASIC_INFO(perframeInfo) \
    pipeInfo[nodeType].rectInfo = tempRect;\
    pipeInfo[nodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;\
    pipeInfo[nodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;\
    pipeInfo[nodeType].perFrameNodeGroupInfo.perframeSupportNodeNum = CAPTURE_NODE_MAX;\
    pipeInfo[nodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perframeInfoIndex = perframeInfo;\
    pipeInfo[nodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameNodeType = PERFRAME_NODE_TYPE_LEADER;\
    pipeInfo[nodeType].perFrameNodeGroupInfo.perFrameLeaderInfo.perFrameVideoID = (m_nodeInfo[INDEX(pipeId)].nodeNum[nodeType] - FIMC_IS_VIDEO_BAS_NUM);
#define SET_CAPTURE_DEVICE_BASIC_INFO() \
    pipeInfo[nodeType].rectInfo = tempRect;\
    pipeInfo[nodeType].bufInfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;\
    pipeInfo[nodeType].bufInfo.memory = V4L2_CAMERA_MEMORY_TYPE;\
    pipeInfo[leaderNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameNodeType = PERFRAME_NODE_TYPE_CAPTURE;\
    pipeInfo[leaderNodeType].perFrameNodeGroupInfo.perFrameCaptureInfo[perFramePos].perFrameVideoID = (m_nodeInfo[INDEX(pipeId)].nodeNum[nodeType] - FIMC_IS_VIDEO_BAS_NUM);\

/* added by 3.2 HAL */
class ExynosCamera3;
class ExynosCameraRequest;
class ExynosCamera3FrameFactory;
typedef status_t (ExynosCamera3::*factory_handler_t)(ExynosCameraRequest*, ExynosCamera3FrameFactory*);
typedef bool (ExynosCamera3::*factory_donehandler_t)();

class ExynosCamera3FrameFactory {
public:
    ExynosCamera3FrameFactory()
    {
        m_init();
    }

    ExynosCamera3FrameFactory(int cameraId, ExynosCamera3Parameters *param)
    {
        m_init();

        m_cameraId = cameraId;
        m_parameters = param;
        m_activityControl = m_parameters->getActivityControl();

        const char *myName = (m_cameraId == CAMERA_ID_BACK) ? "FrameFactoryBack" : "FrameFactoryFront";
        strncpy(m_name, myName,  EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    }

public:
    virtual ~ExynosCamera3FrameFactory();

    virtual status_t        create(bool active = true) = 0;

    virtual status_t        setFrameManager(ExynosCameraFrameManager *manager);
    virtual status_t        getFrameManager(ExynosCameraFrameManager **manager);
    virtual status_t        destroy(void);
    virtual bool            isCreated(void);
    virtual enum NODE_TYPE  getNodeType(uint32_t pipeId) = 0;

    virtual ExynosCameraFrame *createNewFrameOnlyOnePipe(int pipeId, int frameCnt=-1);
    virtual ExynosCameraFrame *createNewFrameVideoOnly(void);
    virtual ExynosCameraFrame *createNewFrame(uint32_t frameCount = 0) = 0;

    virtual status_t        initPipes(void) = 0;
    virtual status_t        preparePipes(void) = 0;
    virtual status_t        preparePipes(uint32_t prepareCnt);
    virtual status_t        startPipes(void) = 0;
    virtual status_t        stopPipes(void) = 0;
    virtual status_t        startInitialThreads(void) = 0;

    virtual status_t        pushFrameToPipe(ExynosCameraFrame **newFrame, uint32_t pipeId);
    virtual status_t        setOutputFrameQToPipe(frame_queue_t *outputQ, uint32_t pipeId);
    virtual status_t        getOutputFrameQToPipe(frame_queue_t **outputQ, uint32_t pipeId);
    virtual status_t        setFrameDoneQToPipe(frame_queue_t *frameDoneQ, uint32_t pipeId);
    virtual status_t        getFrameDoneQToPipe(frame_queue_t **frameDoneQ, uint32_t pipeId);
    virtual status_t        getInputFrameQToPipe(frame_queue_t **inputFrameQ, uint32_t pipeId);

    virtual status_t        setBufferManagerToPipe(ExynosCameraBufferManager **bufferManager, uint32_t pipeId);

    virtual status_t        startThread(uint32_t pipeId);
    virtual status_t        stopThread(uint32_t pipeId);
    virtual status_t        stopThreadAndWait(uint32_t pipeId, int sleep = 5, int times = 40);
    virtual status_t        setStopFlag(void);
    virtual status_t        stopPipe(uint32_t pipeId);

    virtual status_t        getThreadState(int **threadState, uint32_t pipeId);
    virtual status_t        getThreadInterval(uint64_t **threadInterval, uint32_t pipeId);
    virtual status_t        getThreadRenew(int **threadRenew, uint32_t pipeId);
    virtual status_t        incThreadRenew(uint32_t pipeId);
    virtual void            dump(void);

    virtual void            setRequest(int pipeId, bool enable);
    virtual void            setRequestFLITE(bool enable);
    virtual void            setRequest3AC(bool enable);
    virtual void            setRequestISPC(bool enable);
    virtual void            setRequestSCC(bool enable);
    virtual void            setRequestSCP(bool enable);
    virtual void            setRequestISPP(bool enable);
    virtual void            setRequestDIS(bool enable);

    virtual status_t        setParam(struct v4l2_streamparm *streamParam, uint32_t pipeId);
    virtual status_t        setControl(int cid, int value, uint32_t pipeId);
    virtual status_t        getControl(int cid, int *value, uint32_t pipeId);

    virtual bool            checkPipeThreadRunning(uint32_t pipeId);

    /* only for debugging */
    virtual status_t        dumpFimcIsInfo(uint32_t pipeId, bool bugOn);
#ifdef MONITOR_LOG_SYNC
    virtual status_t        syncLog(uint32_t pipeId, uint32_t syncId);
#endif

/* added by 3.2 HAL */
    status_t setFrameCreateHandler(factory_handler_t handler);
    factory_handler_t getFrameCreateHandler();
    status_t setFrameDoneHandler(factory_donehandler_t handler);
    factory_donehandler_t getFrameDoneHandler();

protected:
    virtual status_t        m_initPipelines(ExynosCameraFrame *frame);
    virtual status_t        m_initFrameMetadata(ExynosCameraFrame *frame);
    virtual status_t        m_fillNodeGroupInfo(ExynosCameraFrame *frame) = 0;
    virtual status_t        m_checkPipeInfo(uint32_t srcPipeId, uint32_t dstPipeId);
    virtual status_t        m_setCreate(bool create);
    virtual bool            m_getCreate();
    virtual int             m_getFliteNodenum();

    /* 54xx style*/
    virtual int             m_getSensorId(unsigned int nodeNum, bool reprocessing);

    /* 74xx style*/
    virtual int             m_getSensorId(unsigned int nodeNum, bool flagOTFInterface, bool flagLeader, bool reprocessing);

    virtual int             setSrcNodeEmpty(int sensorId);
    virtual int             setLeader(int sensorId, bool flagLeader);
    virtual status_t        m_setupConfig(void) = 0;
    virtual status_t        m_checkNodeSetting(int pipeId);
    virtual void            m_initDeviceInfo(int pipeId);
    virtual status_t        m_initFlitePipe(void);

private:
    void                    m_init(void);

protected:
    int                         m_cameraId;
    char                        m_name[EXYNOS_CAMERA_NAME_STR_SIZE];

    ExynosCameraPipe           *m_pipes[MAX_NUM_PIPES];

    int32_t                     m_nodeNums[MAX_NUM_PIPES][MAX_NODE];
    int32_t                     m_sensorIds[MAX_NUM_PIPES][MAX_NODE];
    int32_t                     m_secondarySensorIds[MAX_NUM_PIPES][MAX_NODE];
    camera_device_info_t          m_nodeInfo[MAX_NUM_PIPES];

    ExynosCamera3Parameters     *m_parameters;

    ExynosCameraFrameManager   *m_frameMgr;

    uint32_t                    m_frameCount;
    Mutex                       m_frameLock;

    ExynosCameraActivityControl *m_activityControl;

    uint32_t                    m_requestFLITE;
    uint32_t                    m_request3AP;
    uint32_t                    m_request3AC;
    uint32_t                    m_requestISP;
    uint32_t                    m_requestISPP;
    uint32_t                    m_requestISPC;
    uint32_t                    m_requestSCC;
    uint32_t                    m_requestDIS;
    uint32_t                    m_requestSCP;
    uint32_t                    m_requestVRA;
    uint32_t                    m_requestJPEG;
    uint32_t                    m_requestThumbnail;

    bool                        m_bypassDRC;
    bool                        m_bypassDIS;
    bool                        m_bypassDNR;
    bool                        m_bypassFD;

    Mutex                       m_createLock;

    bool                        m_flagFlite3aaOTF;
    bool                        m_flag3aaIspOTF;
    bool                        m_flagIspTpuOTF;
    bool                        m_flagIspMcscOTF;
    bool                        m_flagTpuMcscOTF;
    bool                        m_flagMcscVraOTF;
    bool                        m_supportReprocessing;
    bool                        m_flagReprocessing;
    bool                        m_supportPureBayerReprocessing;
    bool                        m_supportSCC;
    bool                        m_supportMCSC;

    /* added by 3.2 HAL */
    factory_handler_t m_frameCreateHandler;
    factory_donehandler_t m_frameDoneHandler;

private:
    bool                        m_create;
};

}; /* namespace android */

#endif
