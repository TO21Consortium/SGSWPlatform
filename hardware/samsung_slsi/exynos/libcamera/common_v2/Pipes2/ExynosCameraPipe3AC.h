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

#ifndef EXYNOS_CAMERA_PIPE_3AC_H
#define EXYNOS_CAMERA_PIPE_3AC_H

#include "ExynosCameraPipe.h"

namespace android {

typedef ExynosCameraList<ExynosCameraFrame *> frame_queue_t;

class ExynosCameraPipe3AC : protected virtual ExynosCameraPipe {
public:
    ExynosCameraPipe3AC()
    {
        m_init();
    }

    ExynosCameraPipe3AC(
        int cameraId,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums)  : ExynosCameraPipe(cameraId, obj_param, isReprocessing, nodeNums)
    {
        m_init();
    }

    virtual ~ExynosCameraPipe3AC();

    virtual status_t        create(int32_t *sensorIds = NULL);
    virtual status_t        destroy(void);

    virtual status_t        setupPipe(camera_pipe_info_t *pipeInfos, int32_t *sensorIds = NULL);

    virtual status_t        sensorStream(bool on);

    virtual bool            m_checkThreadLoop(void);

private:
    virtual bool            m_mainThreadFunc(void);
    virtual status_t        m_getBuffer(void);

private:
    void                    m_init(void);
    int                     m_getPrepareBufferCount(void);
};

}; /* namespace android */

#endif
