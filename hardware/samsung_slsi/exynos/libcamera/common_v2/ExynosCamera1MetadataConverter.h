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

#ifndef EXYNOS_CAMERA_METADATA_CONVERTER_H__
#define EXYNOS_CAMERA_METADATA_CONVERTER_H__

#include <cutils/log.h>
#include <utils/RefBase.h>
#include <hardware/camera3.h>
#include <camera/CameraMetadata.h>

#include "ExynosCameraParameters.h"

#include "ExynosCameraSensorInfo.h"

namespace android {
enum rectangle_index {
    X1,
    Y1,
    X2,
    Y2,
    RECTANGLE_MAX_INDEX,
};

class ExynosCameraMetadataConverter : public virtual RefBase {
public:
    ExynosCameraMetadataConverter(){};
    ~ExynosCameraMetadataConverter(){};
};

class ExynosCamera1MetadataConverter : public virtual ExynosCameraMetadataConverter {
public:
    ExynosCamera1MetadataConverter(int cameraId, ExynosCameraParameters *parameters);
    ~ExynosCamera1MetadataConverter();
    static  status_t        constructStaticInfo(int cameraId, camera_metadata_t **info);
private:
    int                             m_cameraId;
};

}; /* namespace android */
#endif
