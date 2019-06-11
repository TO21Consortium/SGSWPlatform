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
 * distributed under the License is distributed toggle an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * \file      ExynosCameraActivityBase.h
 * \brief     hearder file for CAMERA HAL MODULE
 * \author    Pilsun Jang(pilsun.jang@samsung.com)
 * \date      2012/12/19
 *
 */

#ifndef EXYNOS_CAMERA_ACTIVITY_BASE_H__
#define EXYNOS_CAMERA_ACTIVITY_BASE_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utils/threads.h>

#include <videodev2.h>
#include <videodev2_exynos_camera.h>
#include <linux/vt.h>

#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/List.h>
#include "cutils/properties.h"

#include "exynos_format.h"
#include "ExynosBuffer.h"
#include "ExynosRect.h"
#include "ExynosExif.h"
#include "exynos_v4l2.h"

#include "ExynosJpegEncoderForCamera.h"
#include "ExynosCameraBuffer.h"
#include "ExynosCameraUtils.h"
#include "fimc-is-metadata.h"

namespace android {

class ExynosCameraActivityBase {
public:
enum CALLBACK_TYPE {
    CALLBACK_TYPE_SENSOR_BEFORE,
    CALLBACK_TYPE_SENSOR_AFTER,
    CALLBACK_TYPE_3A_BEFORE,
    CALLBACK_TYPE_3A_AFTER,
    CALLBACK_TYPE_3A_BEFORE_HAL3,
    CALLBACK_TYPE_3A_AFTER_HAL3,
    CALLBACK_TYPE_ISP_BEFORE,
    CALLBACK_TYPE_ISP_AFTER,
    CALLBACK_TYPE_SCC_BEFORE,
    CALLBACK_TYPE_SCC_AFTER,
    CALLBACK_TYPE_SCP_BEFORE,
    CALLBACK_TYPE_SCP_AFTER,
    CALLBACK_TYPE_END
};

public:
    ExynosCameraActivityBase();
    virtual ~ExynosCameraActivityBase();

    int execFunction(CALLBACK_TYPE callbackType, void *args);

protected:
    virtual int t_funcNull(void *args) = 0;
    virtual int t_funcSensorBefore(void *args) = 0;
    virtual int t_funcSensorAfter(void *args) = 0;
    virtual int t_func3ABefore(void *args) = 0;
    virtual int t_func3AAfter(void *args) = 0;
    virtual int t_func3ABeforeHAL3(void *args) = 0;
    virtual int t_func3AAfterHAL3(void *args) = 0;
    virtual int t_funcISPBefore(void *args) = 0;
    virtual int t_funcISPAfter(void *args) = 0;
    virtual int t_funcSCPBefore(void *args) = 0;
    virtual int t_funcSCPAfter(void *args) = 0;
    virtual int t_funcSCCBefore(void *args) = 0;
    virtual int t_funcSCCAfter(void *args) = 0;

protected:
    int  (ExynosCameraActivityBase::*pFunc)(void *args);
    bool t_isExclusiveReq;
    bool t_isActivated;
    int  t_reqNum;
    int  t_reqStatus;
};
}

#endif /* EXYNOS_CAMERA_ACTIVITY_BASE_H__ */
