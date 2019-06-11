/*
**
** Copyright 2015, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_FRAME_REPROCESSING_FACTORY_NV21_H
#define EXYNOS_CAMERA_FRAME_REPROCESSING_FACTORY_NV21_H

#include "ExynosCameraFrameReprocessingFactory.h"

namespace android {

class ExynosCameraFrameReprocessingFactoryNV21 : public ExynosCameraFrameReprocessingFactory {
public:
    ExynosCameraFrameReprocessingFactoryNV21()
    {
        m_init();
    }

    ExynosCameraFrameReprocessingFactoryNV21(int cameraId, ExynosCamera1Parameters *param) : ExynosCameraFrameReprocessingFactory(cameraId, param)
    {
        m_init();

        strncpy(m_name, "ReprocessingFactoryNV21",  EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    }

    virtual ~ExynosCameraFrameReprocessingFactoryNV21();

    virtual status_t        initPipes(void);

protected:

private:
    void                    m_init(void) {};

};

}; /* namespace android */

#endif
