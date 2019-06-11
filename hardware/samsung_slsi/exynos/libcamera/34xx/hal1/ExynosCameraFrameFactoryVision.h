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

#ifndef EXYNOS_CAMERA_FRAME_FACTORY_VISION_H
#define EXYNOS_CAMERA_FRAME_FACTORY_VISION_H

#include "ExynosCameraFrameFactory.h"

namespace android {

class ExynosCameraFrameFactoryVision : public ExynosCameraFrameFactory {
public:
    ExynosCameraFrameFactoryVision()
    {
        m_init();

        strncpy(m_name, "FrameFactoryVision",  EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    }

    ExynosCameraFrameFactoryVision(int cameraId, ExynosCamera1Parameters *param) : ExynosCameraFrameFactory(cameraId, param)
    {
        m_init();

        strncpy(m_name, "FrameFactoryVision",  EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    }

    virtual ~ExynosCameraFrameFactoryVision();

    virtual status_t        create(bool active = true);
    virtual status_t        destroy(void);

    virtual ExynosCameraFrame *createNewFrame(void);

    virtual status_t        initPipes(void);
    virtual status_t        preparePipes(void);

    virtual status_t        startPipes(void);
    virtual status_t        startInitialThreads(void);
    virtual status_t        stopPipes(void);

    virtual void            setRequest3AC(bool enable);
protected:
    status_t                m_fillNodeGroupInfo(ExynosCameraFrame *frame);
    virtual status_t        m_setupConfig(void);

private:
    void                    m_init(void);

};

}; /* namespace android */

#endif
