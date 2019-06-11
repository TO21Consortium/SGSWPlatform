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

#ifndef EXYNOS_CAMERA_MCPIPE_H
#define EXYNOS_CAMERA_MCPIPE_H

#include "ExynosCameraPipe.h"

namespace android {

enum HW_CONNECTION_MODE {
    HW_CONNECTION_MODE_M2M                  = 0,
    HW_CONNECTION_MODE_OTF,
    HW_CONNECTION_MODE_M2M_BUFFER_HIDING,
};

typedef struct ExynosCameraDeviceInfo {
    int32_t nodeNum[MAX_NODE];
    int32_t secondaryNodeNum[MAX_NODE];
    char    nodeName[MAX_NODE][EXYNOS_CAMERA_NAME_STR_SIZE];
    char    secondaryNodeName[MAX_NODE][EXYNOS_CAMERA_NAME_STR_SIZE];

    int pipeId[MAX_NODE]; /* enum pipeline */
    unsigned int connectionMode[MAX_NODE];

    ExynosCameraDeviceInfo()
    {
        for (int i = 0; i < MAX_NODE; i++) {
            nodeNum[i] = -1;
            secondaryNodeNum[i] = -1;

            memset(nodeName[i], 0, EXYNOS_CAMERA_NAME_STR_SIZE);
            memset(secondaryNodeName[i], 0, EXYNOS_CAMERA_NAME_STR_SIZE);

            pipeId[i] = -1;
            connectionMode[i] = 0;
        }
    }

    ExynosCameraDeviceInfo& operator =(const ExynosCameraDeviceInfo &other)
    {
        for (int i = 0; i < MAX_NODE; i++) {
            nodeNum[i] = other.nodeNum[i];
            secondaryNodeNum[i] = other.secondaryNodeNum[i];

            strncpy(nodeName[i],          other.nodeName[i],          EXYNOS_CAMERA_NAME_STR_SIZE - 1);
            strncpy(secondaryNodeName[i], other.secondaryNodeName[i], EXYNOS_CAMERA_NAME_STR_SIZE - 1);

            pipeId[i] = other.pipeId[i];
        }

        return *this;
    }
} camera_device_info_t;

class ExynosCameraMCPipe : protected virtual ExynosCameraPipe {
public:
    ExynosCameraMCPipe()
    {
        m_init(NULL);
    }

    ExynosCameraMCPipe(
        int cameraId,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        camera_device_info_t *deviceInfo) : ExynosCameraPipe(cameraId, obj_param, isReprocessing, NULL)
    {
        m_init(deviceInfo);
    }

    virtual ~ExynosCameraMCPipe();

    virtual status_t        create(int32_t *sensorIds = NULL);
    virtual status_t        precreate(int32_t *sensorIds = NULL);
    virtual status_t        postcreate(int32_t *sensorIds = NULL);
    virtual status_t        destroy(void);

    virtual status_t        setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds = NULL);
    virtual status_t        setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds, int32_t *secondarySensorIds);
    virtual status_t        prepare(void);

    virtual status_t        start(void);
    virtual status_t        stop(void);
    virtual bool            flagStart(void);

    virtual status_t        startThread(void);
    virtual status_t        stopThread(void);
    virtual status_t        stopThreadAndWait(int sleep, int times);

    virtual bool            flagStartThread(void);

    virtual status_t        sensorStream(bool on);
    virtual status_t        forceDone(unsigned int cid, int value);
    virtual status_t        setControl(int cid, int value);
    virtual status_t        getControl(int cid, int *value);
    virtual status_t        setExtControl(struct v4l2_ext_controls *ctrl);
    virtual status_t        setParam(struct v4l2_streamparm streamParam);

    virtual status_t        pushFrame(ExynosCameraFrame **newFrame);

    virtual status_t        instantOn(int32_t numFrames);
/* Don't use this function, this is regacy code */
    virtual status_t        instantOnQbuf(ExynosCameraFrame **frame, BUFFER_POS::POS pos);
/* Don't use this function, this is regacy code */
    virtual status_t        instantOnDQbuf(ExynosCameraFrame **frame, BUFFER_POS::POS pos);
    virtual status_t        instantOff(void);
/* Don't use this function, this is regacy code */
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

/* MC Pipe have several nodes. It need to specify nodes.
 * Add interface set/get control with specify nodes.
 */
    virtual status_t        setControl(int cid, int value, enum NODE_TYPE nodeType);
    virtual status_t        getControl(int cid, int *value, enum NODE_TYPE nodeType);
    virtual status_t        setExtControl(struct v4l2_ext_controls *ctrl, enum NODE_TYPE nodeType);

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

#ifdef USE_MCPIPE_SERIALIZATION_MODE
    virtual void            needSerialization(bool enable);
#endif

protected:
    virtual bool            m_putBufferThreadFunc(void);
    virtual bool            m_getBufferThreadFunc(void);

    virtual status_t        m_putBuffer(void);
    virtual status_t        m_getBuffer(void);

    virtual status_t        m_updateMetadataToFrame(void *metadata, int index, ExynosCameraFrame *frame = NULL, enum NODE_TYPE nodeLocation = OUTPUT_NODE);
    virtual status_t        m_getFrameByIndex(ExynosCameraFrame **frame, int index, enum NODE_TYPE nodeLocation = OUTPUT_NODE);
    virtual status_t        m_completeFrame(ExynosCameraFrame *frame, bool isValid = true);

    virtual status_t        m_setInput(ExynosCameraNode *nodes[], int32_t *nodeNums, int32_t *sensorIds);
    virtual status_t        m_setPipeInfo(camera_pipe_info_t *pipeInfos);
    virtual status_t        m_setNodeInfo(ExynosCameraNode *node, camera_pipe_info_t *pipeInfos,
                                          uint32_t planeCount, enum YUV_RANGE yuvRange,
                                          bool flagBayer = false);
    virtual status_t        m_forceDone(ExynosCameraNode *node, unsigned int cid, int value);

    virtual status_t        m_startNode(void);
    virtual status_t        m_stopNode(void);
    virtual status_t        m_clearNode(void);

    virtual status_t        m_checkNodeGroupInfo(char *name, camera2_node *oldNode, camera2_node *newNode);
    virtual status_t        m_checkNodeGroupInfo(char *name, int index, camera2_node *oldNode, camera2_node *newNode);
    virtual status_t        m_checkNodeGroupInfo(int index, camera2_node *oldNode, camera2_node *newNode);

    virtual void            m_dumpRunningFrameList(void);

    virtual void            m_dumpPerframeNodeGroupInfo(const char *name, camera_pipe_perframe_node_group_info_t nodeInfo);
    virtual void            m_dumpPerframeShotInfo(const char *name, int frameCount, camera2_shot_ext *shot_ext);

    virtual void            m_configDvfs(void);
    virtual bool            m_flagValidInt(int num);
    virtual bool            m_checkThreadLoop(frame_queue_t *frameQ);

    virtual status_t        m_preCreate(void);
    virtual status_t        m_postCreate(int32_t *sensorIds = NULL);

    virtual status_t        m_checkShotDone(struct camera2_shot_ext *shot_ext);
    /* m_updateMetadataFromFrame() will be deprecated */
    virtual status_t        m_updateMetadataFromFrame(ExynosCameraFrame *frame, ExynosCameraBuffer *buffer);
    virtual status_t        m_updateMetadataFromFrame_v2(ExynosCameraFrame *frame, ExynosCameraBuffer *buffer);

    virtual status_t        m_getPerframePosition(int *perframePosition, uint32_t pipeId);

    virtual status_t        m_setSetfile(ExynosCameraNode *node, uint32_t pipeId);

    virtual status_t        m_setMapBuffer(int nodeIndex);
    virtual status_t        m_setMapBuffer(ExynosCameraNode *node, ExynosCameraBuffer *buffer);

    virtual status_t        m_setJpegInfo(int nodeType, ExynosCameraBuffer *buffer);

    status_t                m_checkPolling(ExynosCameraNode *node);

private:
    void                    m_init(camera_device_info_t *deviceInfo);

protected:
/*
 * For replace old pipe, MCPipe have all variables.
 * but, if exist old pipe, same variable not declared by MCPpipe.
 */

/*
    ExynosCameraParameters     *m_parameters;
    ExynosCameraBufferManager  *m_bufferManager[MAX_NODE];
    ExynosCameraActivityControl *m_activityControl;
*/
    typedef ExynosCameraThread<ExynosCameraMCPipe> MCPipeThread;
    sp<MCPipeThread>            m_putBufferThread;
    sp<MCPipeThread>            m_getBufferThread;
/*
    struct ExynosConfigInfo    *m_exynosconfig;

    ExynosCameraNode           *m_node[MAX_NODE];
    int32_t                     m_nodeNum[MAX_NODE];
    int32_t                     m_sensorIds[MAX_NODE];

    char                        m_name[EXYNOS_CAMERA_NAME_STR_SIZE];

    frame_queue_t              *m_inputFrameQ;
    frame_queue_t              *m_outputFrameQ;
    frame_queue_t              *m_frameDoneQ;
*/

    ExynosCameraFrame          *m_runningFrameList[MAX_NODE][MAX_BUFFERS];
    uint32_t                    m_numOfRunningFrame[MAX_NODE];

    uint32_t                    m_pipeIdArr[MAX_NODE];

/*
    uint32_t                    m_pipeId;
    int32_t                     m_cameraId;

    uint32_t                    m_prepareBufferCount;
*/
    uint32_t                    m_numBuffers[MAX_NODE];

/*
    bool                        m_reprocessing;
    bool                        m_flagStartPipe;
    bool                        m_flagTryStop;
    bool                        m_dvfsLocked;
    bool                        m_isBoosting;
    bool                        m_metadataTypeShot;

    ExynosCameraDurationTimer   m_timer;
    int                         m_threadCommand;
    uint64_t                    m_timeInterval;
    int                         m_threadState;
    int                         m_threadRenew;

    Mutex                       m_pipeframeLock;

    camera2_node_group          m_curNodeGroupInfo;
*/
    camera_pipe_perframe_node_group_info_t m_perframeMainNodeGroupInfo[MAX_NODE];

/*    int                         m_setfile;    */

    ExynosCameraNode           *m_secondaryNode[MAX_NODE];
    int32_t                     m_secondaryNodeNum[MAX_NODE];
    int32_t                     m_secondarySensorIds[MAX_NODE];
    camera_device_info_t       *m_deviceInfo;

    frame_queue_t              *m_requestFrameQ;

    ExynosCameraBuffer          m_skipBuffer[MAX_NODE];
    bool                        m_skipPutBuffer[MAX_NODE];

#ifdef USE_MCPIPE_SERIALIZATION_MODE
    bool                        m_serializeOperation;
    static Mutex                g_serializationLock;
#endif
};

}; /* namespace android */

#endif
