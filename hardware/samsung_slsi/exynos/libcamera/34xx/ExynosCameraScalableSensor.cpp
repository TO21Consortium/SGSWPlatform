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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCamera"
#include <cutils/log.h>

#include "ExynosCameraScalableSensor.h"

namespace android {

ExynosCameraScalableSensor::ExynosCameraScalableSensor()
{
    m_mode = EXYNOS_CAMERA_SCALABLE_NONE;
    m_isAllocedPreview = false;
    m_state = 0;
}

ExynosCameraScalableSensor::~ExynosCameraScalableSensor()
{
}

int ExynosCameraScalableSensor::getState()
{
    return m_state;
}

int ExynosCameraScalableSensor::getMode()
{
    return m_mode;
}

void ExynosCameraScalableSensor::setMode(int Mode)
{
    m_mode = Mode;

    return;
}

bool ExynosCameraScalableSensor::isAllocedPreview()
{
    return m_isAllocedPreview;
}

void ExynosCameraScalableSensor::setAllocedPreivew(bool value)
{
    m_isAllocedPreview = value;

    return;
}

}; /* namespace android */
