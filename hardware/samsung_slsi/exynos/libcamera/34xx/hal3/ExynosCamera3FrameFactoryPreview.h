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

#ifndef EXYNOS_CAMERA3_FRAME_FACTORY_PREVIEW_H
#define EXYNOS_CAMERA3_FRAME_FACTORY_PREVIEW_H

#include "ExynosCamera3FrameFactory.h"

#include "ExynosCameraFrame.h"

namespace android {

class ExynosCamera3FrameFactoryPreview : public ExynosCamera3FrameFactory {
public:
    ExynosCamera3FrameFactoryPreview()
    {
        m_init();
    }

    ExynosCamera3FrameFactoryPreview(int cameraId, ExynosCamera3Parameters *param)
    {
        m_init();

        m_cameraId = cameraId;
        m_parameters = param;
        m_activityControl = m_parameters->getActivityControl();

        const char *myName = (m_cameraId == CAMERA_ID_BACK) ? "FrameFactoryBackPreview" : "FrameFactoryFrontPreview";
        strncpy(m_name, myName,  EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    }

public:
    virtual ~ExynosCamera3FrameFactoryPreview();
    virtual enum NODE_TYPE  getNodeType(uint32_t pipeId);
    virtual status_t        create(bool active = true);

    virtual ExynosCameraFrame *createNewFrame(void);
    virtual ExynosCameraFrame *createNewFrame(uint32_t frameCount = 0);

    virtual status_t        initPipes(void);
    virtual status_t        preparePipes(void);
    virtual status_t        preparePipes(uint32_t prepareCnt);
    virtual status_t        startPipes(void);
    virtual status_t        startInitialThreads(void);
    virtual status_t        setStopFlag(void);
    virtual status_t        stopPipes(void);

protected:
    virtual status_t        m_fillNodeGroupInfo(ExynosCameraFrame *frame);
    virtual status_t        m_setupConfig(void);
    /* setting node number on every pipe */
    virtual status_t        m_setDeviceInfo(void);

    /* pipe setting */
    virtual status_t        m_initPipes(void);

private:
    void                    m_init(void);

};

}; /* namespace android */

#endif
