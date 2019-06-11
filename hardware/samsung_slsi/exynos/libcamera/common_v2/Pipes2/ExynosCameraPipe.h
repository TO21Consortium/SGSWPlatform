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

#ifndef EXYNOS_CAMERA_PIPE_H
#define EXYNOS_CAMERA_PIPE_H

#include "ExynosCameraConfig.h"

#include "ExynosCameraThread.h"
#include "ExynosCameraThreadFactory.h"

#include "ExynosCameraNode.h"
#include "ExynosCameraNodeJpegHAL.h"
#include "ExynosCameraFrame.h"
#include "ExynosCameraSensorInfo.h"
#include "ExynosCameraParameters.h"
#include "ExynosCameraList.h"
#include "ExynosCameraBufferManager.h"

#include "ExynosCameraUtilsModule.h"
#include "ExynosCameraSizeControl.h"

#include "ExynosJpegApi.h"

namespace android {

typedef ExynosCameraList<ExynosCameraFrame *> frame_queue_t;

#ifdef USE_CAMERA2_API_SUPPORT
/* for reprocessing pipe, if timeout happend, prohibit loging until this define */
#define TIME_LOG_COUNT 5
#endif

enum PIPE_POSITION {
    SRC_PIPE            = 0,
    DST_PIPE
};

enum NODE_TYPE {
    INVALID_NODE        = -1,
    OUTPUT_NODE         = 0,    /* Node for output  device */
    CAPTURE_NODE,               /* deprecated enum */
    SUB_NODE,                   /* deprecated enum */

    /* MCPipe CAPTURE NODE */
    CAPTURE_NODE_1 = CAPTURE_NODE, /* MCPipe use CAPTURE_NODE_X. so, this start from OUTPUT_NODE + 1 */
    CAPTURE_NODE_2,
    CAPTURE_NODE_3,
    CAPTURE_NODE_4,
    CAPTURE_NODE_5,
    CAPTURE_NODE_6,
    CAPTURE_NODE_7,
    CAPTURE_NODE_8,
    CAPTURE_NODE_9,
    CAPTURE_NODE_10,
    CAPTURE_NODE_11,
    CAPTURE_NODE_12,
    CAPTURE_NODE_13,
    CAPTURE_NODE_14,
    CAPTURE_NODE_15,
    CAPTURE_NODE_16,
    CAPTURE_NODE_17,

    /* OTF NODE */
    OTF_NODE_BASE,
    OTF_NODE_1,
    OTF_NODE_2,
    OTF_NODE_3,
    OTF_NODE_4,
    OTF_NODE_5,
    OTF_NODE_6,

    MAX_NODE
};

typedef enum perframe_node_type {
    PERFRAME_NODE_TYPE_NONE        = 0,
    PERFRAME_NODE_TYPE_LEADER      = 1,
    PERFRAME_NODE_TYPE_CAPTURE     = 2,
} perframe_node_type_t;

typedef struct ExynosCameraNodeObjects {
    ExynosCameraNode    *node[MAX_NODE];
    ExynosCameraNode    *secondaryNode[MAX_NODE];
    bool isInitalize;
} camera_node_objects_t;

typedef struct ExynosCameraPerframeNodeInfo {
    perframe_node_type_t perFrameNodeType;
    int perframeInfoIndex;
    int perFrameVideoID;

    ExynosCameraPerframeNodeInfo()
    {
        perFrameNodeType  = PERFRAME_NODE_TYPE_NONE;
        perframeInfoIndex = 0;
        perFrameVideoID   = 0;
    }

    ExynosCameraPerframeNodeInfo& operator =(const ExynosCameraPerframeNodeInfo &other)
    {
        perFrameNodeType  = other.perFrameNodeType;
        perframeInfoIndex = other.perframeInfoIndex;
        perFrameVideoID   = other.perFrameVideoID;

        return *this;
    }
} camera_pipe_perframe_node_info_t;

typedef struct ExynosCameraPerframeNodeGroupInfo {
    int perframeSupportNodeNum;
    camera_pipe_perframe_node_info_t perFrameLeaderInfo;
    camera_pipe_perframe_node_info_t perFrameCaptureInfo[CAPTURE_NODE_MAX];

    ExynosCameraPerframeNodeGroupInfo()
    {
        perframeSupportNodeNum = 0;
    }

    ExynosCameraPerframeNodeGroupInfo& operator =(const ExynosCameraPerframeNodeGroupInfo &other)
    {
        perframeSupportNodeNum  = other.perframeSupportNodeNum;
        perFrameLeaderInfo      = other.perFrameLeaderInfo;

        for (int i = 0; i < CAPTURE_NODE_MAX; i++)
            perFrameCaptureInfo[i] = other.perFrameCaptureInfo[i];

        return *this;
    }
} camera_pipe_perframe_node_group_info_t;

typedef struct ExynosCameraPipeInfo {
    struct ExynosRect rectInfo;
    struct v4l2_requestbuffers bufInfo;
    camera_pipe_perframe_node_group_info_t perFrameNodeGroupInfo;
    unsigned int bytesPerPlane[EXYNOS_CAMERA_BUFFER_MAX_PLANES];

    ExynosCameraPipeInfo()
    {
        memset(&bufInfo, 0, sizeof(v4l2_requestbuffers));

        for (int i = 0; i < EXYNOS_CAMERA_BUFFER_MAX_PLANES; i++)
            bytesPerPlane[i] = 0;
    }

    ExynosCameraPipeInfo& operator =(const ExynosCameraPipeInfo &other)
    {
        rectInfo = other.rectInfo;
        memcpy(&bufInfo, &(other.bufInfo), sizeof(v4l2_requestbuffers));
        perFrameNodeGroupInfo = other.perFrameNodeGroupInfo;

        for (int i = 0; i < EXYNOS_CAMERA_BUFFER_MAX_PLANES; i++)
            bytesPerPlane[i] = other.bytesPerPlane[i];

        return *this;
    }
} camera_pipe_info_t;

namespace BUFFER_POS {
    enum POS {
        SRC = 0x00,
        DST = 0x01
    };
}

namespace BUFFERQ_TYPE {
    enum TYPE {
        INPUT = 0x00,
        OUTPUT = 0x01
    };
};

class ExynosCameraPipe {
public:
    ExynosCameraPipe()
    {
        m_init();
    }

    ExynosCameraPipe(
        int cameraId,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums)
    {
        m_init();

        m_cameraId = cameraId;
        m_reprocessing = isReprocessing ? 1 : 0;
        m_oneShotMode = isReprocessing;

        if (nodeNums) {
            for (int i = 0; i < MAX_NODE; i++)
                m_nodeNum[i] = nodeNums[i];
        }

        if (obj_param) {
            m_parameters = obj_param;
            m_activityControl = m_parameters->getActivityControl();
            m_exynosconfig = m_parameters->getConfig();
        }
    }

    virtual ~ExynosCameraPipe();

    virtual status_t        create(int32_t *sensorIds = NULL);
    virtual status_t        precreate(int32_t *sensorIds = NULL);
    virtual status_t        postcreate(int32_t *sensorIds = NULL);
    virtual status_t        destroy(void);

    virtual status_t        setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds = NULL);
    virtual status_t        setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds, int32_t *ispSensorIds);
    virtual status_t        prepare(void);
    virtual status_t        prepare(uint32_t prepareCnt);

    virtual status_t        start(void);
    virtual status_t        stop(void);
    virtual bool            flagStart(void);

    virtual status_t        startThread(void);
    virtual status_t        stopThread(void);
    virtual bool            flagStartThread(void);
    virtual status_t        stopThreadAndWait(int sleep, int times);

    virtual status_t        sensorStream(bool on);
    virtual status_t        forceDone(unsigned int cid, int value);
    virtual status_t        setControl(int cid, int value);
    virtual status_t        setControl(int cid, int value, enum NODE_TYPE nodeType);
    virtual status_t        getControl(int cid, int *value);
    virtual status_t        setExtControl(struct v4l2_ext_controls *ctrl);
    virtual status_t        setParam(struct v4l2_streamparm streamParam);

    virtual status_t        pushFrame(ExynosCameraFrame **newFrame);

    virtual status_t        instantOn(int32_t numFrames);
    virtual status_t        instantOnQbuf(ExynosCameraFrame **frame, BUFFER_POS::POS pos);
    virtual status_t        instantOnDQbuf(ExynosCameraFrame **frame, BUFFER_POS::POS pos);
    virtual status_t        instantOff(void);
    virtual status_t        instantOnPushFrameQ(BUFFERQ_TYPE::TYPE type, ExynosCameraFrame **frame);

    virtual status_t        getPipeInfo(int *fullW, int *fullH, int *colorFormat, int pipePosition);
    virtual int             getCameraId(void);
    virtual status_t        setPipeId(uint32_t id);
    virtual uint32_t        getPipeId(void);
    virtual status_t        setPipeId(enum NODE_TYPE nodeType, uint32_t id);
    virtual int             getPipeId(enum NODE_TYPE nodeType);

    virtual status_t        setPipeName(const char *pipeName);
    virtual char           *getPipeName(void);

    virtual status_t        clearInputFrameQ(void);
    virtual status_t        getInputFrameQ(frame_queue_t **inputQ);
    virtual status_t        setOutputFrameQ(frame_queue_t *outputQ);
    virtual status_t        getOutputFrameQ(frame_queue_t **outputQ);

    virtual status_t        setBoosting(bool isBoosting);

    virtual bool            isThreadRunning(void);

    virtual status_t        getThreadState(int **threadState);
    virtual status_t        getThreadInterval(uint64_t **timeInterval);
    virtual status_t        getThreadRenew(int **timeRenew);
    virtual status_t        incThreadRenew(void);
    virtual status_t        setStopFlag(void);

    virtual int             getRunningFrameCount(void);

    virtual void            dump(void);

    /* only for debugging */
    virtual status_t        dumpFimcIsInfo(bool bugOn);
//#ifdef MONITOR_LOG_SYNC
    virtual status_t        syncLog(uint32_t syncId);
//#endif

/* MC Pipe include buffer manager, so FrameFactory(ExynosCamera) must set buffer manager to pipe.
 * Add interface for set buffer manager to pipe.
 */
    virtual status_t        setBufferManager(ExynosCameraBufferManager **bufferManager);

/* Set map buffer is makes node operation faster at first start.
 * Thereby map buffer before start, reduce map buffer time at start.
 * It use first Exynos5430/5433 in old pipe.
 */
    virtual status_t        setMapBuffer(ExynosCameraBuffer *srcBuf = NULL, ExynosCameraBuffer *dstBuf = NULL);

/* MC Pipe have two output queue.
 * If you want push frame to FrameDoneQ in ExynosCamera explicitly, use this interface.
 */

    virtual status_t        setFrameDoneQ(frame_queue_t *frameDoneQ);
    virtual status_t        getFrameDoneQ(frame_queue_t **frameDoneQ);

    virtual status_t        setNodeInfos(camera_node_objects_t *nodeObjects, bool flagReset = false);
    virtual status_t        getNodeInfos(camera_node_objects_t *nodeObjects);

    virtual void            setOneShotMode(bool enable);
#ifdef USE_MCPIPE_SERIALIZATION_MODE
    virtual void            needSerialization(bool enable);
#endif

protected:
    virtual bool            m_mainThreadFunc(void);

    virtual status_t        m_putBuffer(void);
    virtual status_t        m_getBuffer(void);

    virtual status_t        m_updateMetadataToFrame(void *metadata, int index);
    virtual status_t        m_getFrameByIndex(ExynosCameraFrame **frame, int index);
    virtual status_t        m_completeFrame(
                                ExynosCameraFrame **frame,
                                ExynosCameraBuffer buffer,
                                bool isValid = true);

    virtual status_t        m_setInput(ExynosCameraNode *nodes[], int32_t *nodeNums, int32_t *sensorIds);
    virtual status_t        m_setPipeInfo(camera_pipe_info_t *pipeInfos);
    virtual status_t        m_setNodeInfo(ExynosCameraNode *node, camera_pipe_info_t *pipeInfos,
                                          int planeCount, enum YUV_RANGE yuvRange,
                                          bool flagBayer = false);
    virtual status_t        m_forceDone(ExynosCameraNode *node, unsigned int cid, int value);

    virtual status_t        m_startNode(void);
    virtual status_t        m_stopNode(void);
    virtual status_t        m_clearNode(void);

    virtual status_t        m_checkNodeGroupInfo(char *name, camera2_node *oldNode, camera2_node *newNode);
    virtual status_t        m_checkNodeGroupInfo(int index, camera2_node *oldNode, camera2_node *newNode);

    virtual void            m_dumpRunningFrameList(void);

    virtual void            m_dumpPerframeNodeGroupInfo(const char *name, camera_pipe_perframe_node_group_info_t nodeInfo);

    virtual void            m_configDvfs(void);
    virtual bool            m_flagValidInt(int num);
            bool            m_isOtf(int sensorId);
            bool            m_checkValidFrameCount(struct camera2_shot_ext *shot_ext);
            bool            m_checkValidFrameCount(struct camera2_stream *stream);
    virtual status_t        m_handleInvalidFrame(int index, ExynosCameraFrame *newFrame, ExynosCameraBuffer *buffer);
    virtual bool            m_checkLeaderNode(int sensorId);
    virtual bool            m_isReprocessing(void);
    virtual bool            m_checkThreadLoop(void);

private:
    void                    m_init(void);

protected:
    ExynosCameraParameters     *m_parameters;
    ExynosCameraBufferManager  *m_bufferManager[MAX_NODE];
    ExynosCameraActivityControl *m_activityControl;

    sp<Thread>                  m_mainThread;

    struct ExynosConfigInfo     *m_exynosconfig;

    ExynosCameraNode           *m_node[MAX_NODE];
    int32_t                     m_nodeNum[MAX_NODE];
    int32_t                     m_sensorIds[MAX_NODE];
    ExynosCameraNode           *m_mainNode;
    int32_t                     m_mainNodeNum;

    char                        m_name[EXYNOS_CAMERA_NAME_STR_SIZE];

    frame_queue_t              *m_inputFrameQ;
    frame_queue_t              *m_outputFrameQ;
    frame_queue_t              *m_frameDoneQ;

    ExynosCameraFrame          *m_runningFrameList[MAX_BUFFERS];
    ExynosCameraFrame          *m_nodeRunningFrameList[MAX_NODE][MAX_BUFFERS];
    uint32_t                    m_numOfRunningFrame;

protected:
    uint32_t                    m_pipeId;
    int32_t                     m_cameraId;

    uint32_t                    m_prepareBufferCount;
    uint32_t                    m_numBuffers;
    /* Node for capture Interface : destination port */
    int                         m_numCaptureBuf;

    uint32_t                    m_reprocessing;
    bool                        m_oneShotMode;
    bool                        m_flagStartPipe;
    bool                        m_flagTryStop;
    bool                        m_dvfsLocked;
    bool                        m_isBoosting;
    bool                        m_metadataTypeShot;
    bool                        m_flagFrameDoneQ;

    ExynosCameraDurationTimer   m_timer;
    int                         m_threadCommand;
    uint64_t                    m_timeInterval;
    int                         m_threadState;
    int                         m_threadRenew;

    Mutex                       m_pipeframeLock;

    camera2_node_group          m_curNodeGroupInfo;

    camera_pipe_perframe_node_group_info_t m_perframeMainNodeGroupInfo;

    int                         m_lastSrcFrameCount;
    int                         m_lastDstFrameCount;

    int                         m_setfile;
#ifdef USE_CAMERA2_API_SUPPORT
    int                         m_timeLogCount;
#endif
};

}; /* namespace android */

#endif
