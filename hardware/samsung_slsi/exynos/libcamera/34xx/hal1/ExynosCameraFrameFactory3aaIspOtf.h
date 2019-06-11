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

#ifndef EXYNOS_CAMERA_FRAME_FACTORY_3AA_ISP_OTF_H
#define EXYNOS_CAMERA_FRAME_FACTORY_3AA_ISP_OTF_H

#include "ExynosCameraFrameFactoryPreview.h"

namespace android {

class ExynosCameraFrameFactory3aaIspOtf : public ExynosCameraFrameFactoryPreview {
public:
    ExynosCameraFrameFactory3aaIspOtf()
    {
        m_init();
    }

    ExynosCameraFrameFactory3aaIspOtf(int cameraId, ExynosCamera1Parameters *param)
    {
        m_init();

        m_cameraId = cameraId;
        m_parameters = param;
        m_activityControl = m_parameters->getActivityControl();

        const char *myName = "ExynosCameraFrameFactory3aaIspOtf";
        strncpy(m_name, myName,  EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    }

public:
    virtual ~ExynosCameraFrameFactory3aaIspOtf();

    virtual enum NODE_TYPE  getNodeType(uint32_t pipeId);

protected:
    /* setting node number on every pipe */
    virtual status_t        m_setDeviceInfo(void);

    /* pipe setting */
    virtual status_t        m_initPipes(void);

    /* pipe setting for fastAE */
    virtual status_t        m_initPipesFastenAeStable(int32_t numFrames,
                                                      int hwSensorW, int hwSensorH,
                                                      int hwPreviewW, int hwPreviewH);

private:
    void                    m_init(void);

};

}; /* namespace android */

#endif
