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

#ifndef EXYNOS_CAMERA_PIPE_FUSION_H
#define EXYNOS_CAMERA_PIPE_FUSION_H

#include "ExynosCameraPipe.h"

#include "ExynosCameraFusionInclude.h"
#include "ExynosCameraDualFrameSelector.h"

namespace android {

typedef ExynosCameraList<ExynosCameraFrame *> frame_queue_t;

class ExynosCameraPipeFusion : protected virtual ExynosCameraPipe {
public:
    ExynosCameraPipeFusion()
    {
        m_init(NULL);
    }

    ExynosCameraPipeFusion(
        int cameraId,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums)  : ExynosCameraPipe(cameraId, obj_param, isReprocessing, nodeNums)
    {
        m_init(nodeNums);
    }

    virtual ~ExynosCameraPipeFusion();

    virtual status_t        create(int32_t *sensorIds = NULL);
    virtual status_t        destroy(void);

    virtual status_t        start(void);
    virtual status_t        stop(void);
    virtual status_t        startThread(void);

protected:
    virtual status_t        m_run(void);
    virtual bool            m_mainThreadFunc(void);

private:
    void                    m_init(int32_t *nodeNums);

    status_t                m_manageFusion(ExynosCameraFrame *newFrame);

    status_t                m_createFusion(void);
    bool                    m_flagReadyFusion(void);
    status_t                m_executeFusion(struct camera2_shot_ext *shot_ext[], DOF *dof[],
                                            ExynosCameraBuffer srcBuffer[], ExynosRect srcRect[], ExynosCameraBufferManager *srcBufferManager[],
                                            ExynosCameraBuffer dstBuffer,   ExynosRect dstRect,   ExynosCameraBufferManager *dstBufferManager);
    status_t                m_executeEmulationFusion(ExynosCameraBuffer srcBuffer[], ExynosRect srcRect[],
                                                     ExynosCameraBuffer dstBuffer,   ExynosRect dstRect);

    status_t                m_destroyFusion(void);

private:
    int                     m_mainCameraId;
    int                     m_subCameraId;
    bool                    m_flagValidCameraId[CAMERA_ID_MAX];

    ExynosCameraDurationTimer m_fusionProcessTimer;
};

}; /* namespace android */

#endif
