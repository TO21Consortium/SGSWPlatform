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

#ifndef EXYNOS_CAMERA_1_SENSOR_INFO_H
#define EXYNOS_CAMERA_1_SENSOR_INFO_H

#if 0
#include <videodev2.h>
#include <videodev2_exynos_camera.h>
#include "ExynosCameraConfig.h"
#include "ExynosCameraSizeTable.h"

#include "fimc-is-metadata.h"
#else

#include "ExynosCameraConfig.h"
#include "ExynosCameraSensorInfoBase.h"
#endif /* USE_CAMERA2_API_SUPPORT */

namespace android {


/****************************************/
/* Project Specific Sensor LUT          */
/****************************************/


/* Helpper functions */
struct ExynosSensorInfoBase *createExynosCamera1SensorInfo(int sensorName);

struct ExynosSensorS5K3M2 : public ExynosSensorS5K3M2Base {
public:
    ExynosSensorS5K3M2();
};

struct ExynosSensorS5K5E2 : public ExynosSensorS5K5E2Base {
public:
    ExynosSensorS5K5E2();
};

struct ExynosSensorS5K5E8 : public ExynosSensorS5K5E8Base {
public:
    ExynosSensorS5K5E8();
};
}; /* namespace android */
#endif
