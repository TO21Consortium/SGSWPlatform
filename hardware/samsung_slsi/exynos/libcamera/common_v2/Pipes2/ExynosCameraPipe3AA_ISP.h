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

#ifndef EXYNOS_CAMERA_PIPE_3AA_ISP_H
#define EXYNOS_CAMERA_PIPE_3AA_ISP_H

#include "ExynosCameraPipe.h"

namespace android {

typedef ExynosCameraList<ExynosCameraBuffer> isp_buffer_queue_t;

class ExynosCameraPipe3AA_ISP : protected virtual ExynosCameraPipe {
public:
    ExynosCameraPipe3AA_ISP()
    {
        m_init(NULL);
    }

    ExynosCameraPipe3AA_ISP(
        int cameraId,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums)  : ExynosCameraPipe(cameraId, obj_param, isReprocessing, nodeNums)
    {
        m_init(NULL);
    }

    ExynosCameraPipe3AA_ISP(
        int cameraId,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums, int32_t *ispNodeNums)  : ExynosCameraPipe(cameraId, obj_param, isReprocessing, nodeNums)
    {
        m_init(ispNodeNums);
    }

    virtual ~ExynosCameraPipe3AA_ISP();

    virtual status_t        create(int32_t *sensorIds = NULL);
    virtual status_t        destroy(void);

    virtual status_t        setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds = NULL);
    virtual status_t        setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds, int32_t *ispSensorIds);
    virtual status_t        prepare(void);

    virtual status_t        start(void);
    virtual status_t        stop(void);
    virtual status_t        startThread(void);
    virtual status_t        stopThread(void);

    virtual status_t        setControl(int cid, int value);
    virtual status_t        instantOn(int32_t numFrames);
    virtual status_t        instantOff(void);
    virtual status_t        forceDone(unsigned int cid, int value);

    virtual void            dump(void);
    virtual status_t        precreate(int32_t *sensorIds = NULL);
    virtual status_t        postcreate(int32_t *sensorIds = NULL);

    virtual status_t        dumpFimcIsInfo(bool bugOn);
//#ifdef MONITOR_LOG_SYNC
    virtual status_t        syncLog(uint32_t syncId);
//#endif

protected:
    status_t                m_getBuffer(void);
    status_t                m_putBuffer(void);
    status_t                m_getIspBuffer(void);

    virtual status_t        m_checkShotDone(struct camera2_shot_ext *shot_ext);
    virtual status_t        m_setPipeInfo(camera_pipe_info_t *pipeInfos);

protected:
    virtual bool            m_mainThreadFunc(void);
    virtual bool            m_ispThreadFunc(void);
    bool                    m_checkValidIspFrameCount(struct camera2_shot_ext * shot_ext);

private:
    void                    m_init(int32_t *ispNodeNums = NULL);
    void                    m_copyNodeInfo2IspNodeInfo(void);

protected:
    camera_pipe_perframe_node_group_info_t m_perframeSubNodeGroupInfo;
    camera_pipe_perframe_node_group_info_t m_perframeIspNodeGroupInfo;
    camera2_node_group          m_curIspNodeGroupInfo;

private:
    int32_t                     m_ispNodeNum[MAX_NODE];
    ExynosCameraNode           *m_ispNode[MAX_NODE];

    int32_t                     m_ispSensorIds[MAX_NODE];

    sp<Thread>                  m_ispThread;
    isp_buffer_queue_t          *m_ispBufferQ;
//#ifdef SHOT_RECOVERY
    int                     retryGetBufferCount;
//#endif
    bool                        m_flagIndependantIspNode;
    int                         m_lastIspFrameCount;
};

}; /* namespace android */

#endif
