/*
 * Copyright 2012, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraActivityBase"
#include <cutils/log.h>

#include "ExynosCameraActivityBase.h"

namespace android {

ExynosCameraActivityBase::ExynosCameraActivityBase()
{
    t_isExclusiveReq = false;
    t_isActivated = false;
    t_reqNum = 0;
    t_reqStatus = 0;
    pFunc = NULL;
}

ExynosCameraActivityBase::~ExynosCameraActivityBase()
{
}

int ExynosCameraActivityBase::execFunction(CALLBACK_TYPE callbackType, void *args)
{
    switch (callbackType) {
    case CALLBACK_TYPE_SENSOR_BEFORE:
        pFunc = &ExynosCameraActivityBase::t_funcSensorBefore;
        break;
    case CALLBACK_TYPE_SENSOR_AFTER:
        pFunc = &ExynosCameraActivityBase::t_funcSensorAfter;
        break;
    case CALLBACK_TYPE_3A_BEFORE:
        pFunc = &ExynosCameraActivityBase::t_func3ABefore;
        break;
    case CALLBACK_TYPE_3A_AFTER:
        pFunc = &ExynosCameraActivityBase::t_func3AAfter;
        break;
    case CALLBACK_TYPE_3A_BEFORE_HAL3:
        pFunc = &ExynosCameraActivityBase::t_func3ABeforeHAL3;
        break;
    case CALLBACK_TYPE_3A_AFTER_HAL3:
        pFunc = &ExynosCameraActivityBase::t_func3AAfterHAL3;
        break;
    case CALLBACK_TYPE_ISP_BEFORE:
        pFunc = &ExynosCameraActivityBase::t_funcISPBefore;
        break;
    case CALLBACK_TYPE_ISP_AFTER:
        pFunc = &ExynosCameraActivityBase::t_funcISPAfter;
        break;
    case CALLBACK_TYPE_SCC_BEFORE:
        pFunc = &ExynosCameraActivityBase::t_funcSCCBefore;
        break;
    case CALLBACK_TYPE_SCC_AFTER:
        pFunc = &ExynosCameraActivityBase::t_funcSCCAfter;
        break;
    case CALLBACK_TYPE_SCP_BEFORE:
        pFunc = &ExynosCameraActivityBase::t_funcSCPBefore;
        break;
    case CALLBACK_TYPE_SCP_AFTER:
        pFunc = &ExynosCameraActivityBase::t_funcSCPAfter;
        break;
    default:
        break;
    }

    return (this->*pFunc)(args);
}

} /* namespace android */
