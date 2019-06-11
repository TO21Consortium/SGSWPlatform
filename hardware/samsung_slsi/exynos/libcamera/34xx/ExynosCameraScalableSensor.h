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


#ifndef EXYNOS_CAMERA_SCALABLE_SENSOR_H
#define EXYNOS_CAMERA_SCALABLE_SENSOR_H

#include <utils/threads.h>
#include <utils/RefBase.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <hardware/camera.h>
#include <hardware/gralloc.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <media/hardware/MetadataBufferType.h>

#include <fcntl.h>
#include <sys/mman.h>

namespace android {

#define EXYNOS_CAMERA_SCALABLE_NONE                (0)
#define EXYNOS_CAMERA_SCALABLE_CHANGING            (1)
#define EXYNOS_CAMERA_SCALABLE_2M                  (2)
#define EXYNOS_CAMERA_SCALABLE_13M                 (3)

#define EXYNOS_CAMERA_SCALABLE_2M_2_13M            (11)
#define EXYNOS_CAMERA_SCALABLE_13M_2_2M            (12)


class ExynosCameraScalableSensor {
public:
    ExynosCameraScalableSensor();
    virtual             ~ExynosCameraScalableSensor();
    int         getState(void);
    int         getMode(void);
    void        setMode(int Mode);
    bool        isAllocedPreview(void);
    void        setAllocedPreivew(bool value);
private:
    int         m_state;
    int         m_mode;
    bool        m_isAllocedPreview;
};

};
#endif /* EXYNOS_CAMERA_SCALABLE_SENSOR_H */

