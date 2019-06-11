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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraActivityUCTL"
#include <cutils/log.h>

#include "ExynosCameraActivityUCTL.h"
//#include "ExynosCamera.h"

namespace android {

class ExynosCamera;

ExynosCameraActivityUCTL::ExynosCameraActivityUCTL()
{
    m_rotation = 0;
}

ExynosCameraActivityUCTL::~ExynosCameraActivityUCTL()
{
}

int ExynosCameraActivityUCTL::t_funcNull(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;

    return 1;
}

int ExynosCameraActivityUCTL::t_funcSensorBefore(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);

    return 1;
}

int ExynosCameraActivityUCTL::t_funcSensorAfter(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);

    return 1;
}

int ExynosCameraActivityUCTL::t_funcISPBefore(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);

    return 1;
}

int ExynosCameraActivityUCTL::t_funcISPAfter(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);

    return 1;
}

int ExynosCameraActivityUCTL::t_func3ABefore(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);

    if (shot_ext != NULL) {
#ifdef FD_ROTATION
       shot_ext->shot.uctl.scalerUd.orientation = m_rotation;
#endif
    }

    return 1;
}

int ExynosCameraActivityUCTL::t_func3AAfter(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);

    return 1;
}

int ExynosCameraActivityUCTL::t_func3ABeforeHAL3(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityUCTL::t_func3AAfterHAL3(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityUCTL::t_funcSCPBefore(__unused void *args)
{
    ALOGV("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    return 1;
}


int ExynosCameraActivityUCTL::t_funcSCPAfter(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_stream *shot_stream = (struct camera2_stream *)(buf->addr[2]);

    return 1;
}

int ExynosCameraActivityUCTL::t_funcSCCBefore(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;

    ALOGV("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    return 1;
}

int ExynosCameraActivityUCTL::t_funcSCCAfter(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;

    ALOGV("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    return 1;
}

void ExynosCameraActivityUCTL::setDeviceRotation(int rotation)
{
    m_rotation = rotation;
}
} /* namespace android */

