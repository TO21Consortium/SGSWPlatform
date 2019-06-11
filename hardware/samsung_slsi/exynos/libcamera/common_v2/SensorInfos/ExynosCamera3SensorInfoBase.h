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

#ifndef EXYNOS_CAMERA_3_SENSOR_INFO_BASE_H
#define EXYNOS_CAMERA_3_SENSOR_INFO_BASE_H

#include "ExynosCameraConfig.h"
#include "ExynosCameraSensorInfoBase.h"
#include "ExynosCameraAvailabilityTable.h"

namespace android {

struct ExynosCamera3SensorInfoBase : public ExynosSensorInfoBase {
private:

public:
    ExynosCamera3SensorInfoBase();
};

struct ExynosCamera3SensorIMX135Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorIMX135Base();
};

struct ExynosCamera3SensorIMX134Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorIMX134Base();
};

struct ExynosCamera3SensorS5K3M2Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K3M2Base();
};

struct ExynosCamera3SensorS5K3L2Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K3L2Base();
};

#if 0
struct ExynosCamera3SensorS5K2P2Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K2P2Base();
};
#endif

struct ExynosCamera3SensorS5K2P2_12MBase : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K2P2_12MBase();
};

struct ExynosCamera3SensorS5K2P3Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K2P3Base();
};

struct ExynosCamera3SensorS5K2T2Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K2T2Base();
};

struct ExynosCamera3SensorS5K2P8Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K2P8Base();
};

struct ExynosCamera3SensorS5K6B2Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K6B2Base();
};

struct ExynosCamera3SensorSR261Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorSR261Base();
};

struct ExynosCamera3SensorSR259Base : public ExynosCamera3SensorInfoBase {
public:
    ExynosCamera3SensorSR259Base();
};

struct ExynosCamera3SensorS5K3H7Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K3H7Base();
};

struct ExynosCamera3SensorS5K3H5Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K3H5Base();
};

struct ExynosCamera3SensorS5K4H5Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K4H5Base();
};

struct ExynosCamera3SensorS5K6A3Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K6A3Base();
};

struct ExynosCamera3SensorIMX175Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorIMX175Base();
};

#if 0
struct ExynosCamera3SensorIMX240Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorIMX240Base();
};
#endif

struct ExynosCamera3SensorIMX219Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorIMX219Base();
};

struct ExynosCamera3SensorS5K8B1Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K8B1Base();
};

struct ExynosCamera3SensorS5K6D1Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K6D1Base();
};

struct ExynosCamera3SensorS5K4E6Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K4E6Base();
};

struct ExynosCamera3SensorS5K5E2Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K5E2Base();
};

struct ExynosCamera3SensorS5K5E3Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K5E3Base();
};

struct ExynosCamera3SensorSR544Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorSR544Base();
};

struct ExynosCamera3SensorIMX240_2P2Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorIMX240_2P2Base(int sensorId);
};

struct ExynosCamera3SensorIMX260_2L1Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorIMX260_2L1Base(int sensorId);
};

struct ExynosCamera3SensorS5K3P3Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorS5K3P3Base(int sensorId);
};

struct ExynosCamera3SensorOV5670Base : public ExynosCamera3SensorInfoBase {
private:

public:
    ExynosCamera3SensorOV5670Base();
};

}; /* namespace android */
#endif
