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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCamera3SensorInfo"
#include <cutils/log.h>

#include "ExynosCamera3SensorInfo.h"

namespace android {

struct ExynosSensorInfoBase *createExynosCamera3SensorInfo(int camId)
{
    struct ExynosSensorInfoBase *sensorInfo = NULL;
    int sensorId = getSensorId(camId);
    if (sensorId < 0) {
        ALOGE("ERR(%s[%d]): Inavalid camId, sensor name is nothing", __FUNCTION__, __LINE__);
        sensorId = SENSOR_NAME_NOTHING;
    }

    switch (sensorId) {
    case SENSOR_NAME_S5K3L2:
        sensorInfo = new ExynosCamera3SensorS5K3L2();
        break;
    case SENSOR_NAME_S5K3M2:
        sensorInfo = new ExynosCamera3SensorS5K3M2();
        break;
    case SENSOR_NAME_S5K5E2:
        sensorInfo = new ExynosCamera3SensorS5K5E2();
        break;
    case SENSOR_NAME_S5K5E3:
        sensorInfo = new ExynosCamera3SensorS5K5E3();
        break;
    default:
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Unknown sensor(%d), create default sensor, assert!!!!",
            __FUNCTION__, __LINE__, camId);
        break;
    }

    return sensorInfo;
}

ExynosCamera3SensorS5K3L2::ExynosCamera3SensorS5K3L2() : ExynosCamera3SensorS5K3L2Base()
{
    hyperFocalDistance = 1.0f / 3.6f;
};

ExynosCamera3SensorS5K3M2::ExynosCamera3SensorS5K3M2() : ExynosCamera3SensorS5K3M2Base()
{
    hyperFocalDistance = 1.0f / 3.6f;
};

ExynosCamera3SensorS5K5E2::ExynosCamera3SensorS5K5E2() : ExynosCamera3SensorS5K5E2Base()
{
};

ExynosCamera3SensorS5K5E3::ExynosCamera3SensorS5K5E3() : ExynosCamera3SensorS5K5E3Base()
{
};

}; /* namespace android */
