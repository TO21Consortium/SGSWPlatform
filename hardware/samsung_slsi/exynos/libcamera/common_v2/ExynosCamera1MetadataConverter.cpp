/*
 * Copyright (C) 2014, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "ExynosCameraMetadataConverter"

#include "ExynosCamera1MetadataConverter.h"

namespace android {
#define SET_BIT(x)      (1 << x)

ExynosCamera1MetadataConverter::ExynosCamera1MetadataConverter(int cameraId, __unused ExynosCameraParameters *parameters)
{
    ExynosCameraActivityControl *activityControl = NULL;

    m_cameraId = cameraId;
}

ExynosCamera1MetadataConverter::~ExynosCamera1MetadataConverter()
{
}

status_t ExynosCamera1MetadataConverter::constructStaticInfo(int cameraId, camera_metadata_t **cameraInfo)
{
    status_t ret = NO_ERROR;
    uint8_t flashAvailable = 0x0;

    ALOGD("DEBUG(%s[%d]):ID(%d)", __FUNCTION__, __LINE__, cameraId);
    struct ExynosSensorInfoBase *sensorStaticInfo = NULL;
    CameraMetadata info;

    sensorStaticInfo = createExynosCamera1SensorInfo(cameraId);
    if (sensorStaticInfo == NULL) {
        ALOGE("ERR(%s[%d]): sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* andorid.flash static attributes */
    if (sensorStaticInfo->flashModeList & FLASH_MODE_TORCH) {
        flashAvailable = 0x1;
    } else {
        flashAvailable = 0x0;
    }

    ret = info.update(ANDROID_FLASH_INFO_AVAILABLE, &flashAvailable, 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_FLASH_INFO_AVAILABLE update failed(%d)",  __FUNCTION__, ret);

    *cameraInfo = info.release();

    return OK;
}
}; /* namespace android */
