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

#ifndef EXYNOS_CAMERA_PIPE_SCP_H
#define EXYNOS_CAMERA_PIPE_SCP_H

#include "ExynosCameraPipe.h"

/* #define TEST_WATCHDOG_THREAD */

namespace android {

typedef ExynosCameraList<ExynosCameraFrame *> frame_queue_t;

class ExynosCameraPipeSCP : protected virtual ExynosCameraPipe {
public:
    ExynosCameraPipeSCP()
    {
        m_init();
    }

    ExynosCameraPipeSCP(
        int cameraId,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums)  : ExynosCameraPipe(cameraId, obj_param, isReprocessing, nodeNums)
    {
        m_init();
    }

    virtual ~ExynosCameraPipeSCP();

    virtual status_t        create(int32_t *sensorIds = NULL);
    virtual status_t        destroy(void);

    virtual status_t        setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds = NULL);

protected:
    virtual bool            m_mainThreadFunc(void);
    virtual status_t        m_putBuffer(void);
    virtual status_t        m_getBuffer(void);
    virtual status_t        m_setPipeInfo(camera_pipe_info_t *pipeInfos);

private:
    void                    m_init(void);
    status_t                m_checkPolling(void);

    frame_queue_t           m_requestFrameQ;
    frame_queue_t           m_skipFrameQ;
    ExynosCameraBuffer      prevDqBuffer;
    bool                    prevDqBufferValid;
};
}; /* namespace android */
#endif
