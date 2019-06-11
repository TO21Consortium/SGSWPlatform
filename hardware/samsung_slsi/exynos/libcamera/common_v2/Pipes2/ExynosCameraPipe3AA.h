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

#ifndef EXYNOS_CAMERA_PIPE_3AA_H
#define EXYNOS_CAMERA_PIPE_3AA_H

#include "ExynosCameraPipe.h"

namespace android {

typedef ExynosCameraList<ExynosCameraFrame *> frame_queue_t;

class ExynosCameraPipe3AA : protected virtual ExynosCameraPipe {
public:
    ExynosCameraPipe3AA()
    {
        m_init();
    }

    ExynosCameraPipe3AA(
        int cameraId,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums) : ExynosCameraPipe(cameraId, obj_param, isReprocessing, nodeNums)
    {
        m_init();
    }

    virtual ~ExynosCameraPipe3AA();

    virtual status_t        create(int32_t *sensorIds = NULL);
    virtual status_t        destroy(void);

    virtual status_t        setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds = NULL);
    virtual status_t        prepare(void);

    virtual status_t        start(void);
    virtual status_t        stop(void);

    virtual status_t        getPipeInfo(int *fullW, int *fullH, int *colorFormat, int pipePosition);

private:
    virtual bool            m_mainThreadFunc(void);

    virtual status_t        m_putBuffer(void);
    virtual status_t        m_getBuffer(void);
    virtual status_t        m_setPipeInfo(camera_pipe_info_t *pipeInfos);

private:
    void                    m_init(void);

protected:
    camera_pipe_perframe_node_group_info_t m_perframeSubNodeGroupInfo;
};

}; /* namespace android */

#endif
