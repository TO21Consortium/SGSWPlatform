/*
 * Copyright 2008, The Android Open Source Project
 * Copyright 2013, Samsung Electronics Co. LTD
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

 /*!
 * \file      SecCameraParameters.cpp
 * \brief     source file for Android Camera Ext HAL
 * \author    teahyung kim (tkon.kim@samsung.com)
 * \date      2013/04/30
 *
 */

#define LOG_TAG "SecCameraParams"
#include <utils/Log.h>

#include <string.h>
#include <stdlib.h>
#include <camera/CameraParameters.h>

#include "SecCameraParameters.h"

namespace android {

/* Parameter keys to communicate between camera application and driver. */
const char SecCameraParameters::KEY_DTP_MODE[] = "chk_dataline";

const char SecCameraParameters::KEY_VT_MODE[] = "vtmode";
const char SecCameraParameters::KEY_MOVIE_MODE[] = "cam_mode";

const char SecCameraParameters::KEY_ISO[] = "iso";
const char SecCameraParameters::KEY_METERING[] = "metering";
const char SecCameraParameters::KEY_AUTO_CONTRAST[] = "wdr";
const char SecCameraParameters::KEY_ANTI_SHAKE[] = "anti-shake";
const char SecCameraParameters::KEY_FACE_BEAUTY[] = "face_beauty";
const char SecCameraParameters::KEY_HDR_MODE[] = "hdr-mode";
const char SecCameraParameters::KEY_BLUR[] = "blur";
const char SecCameraParameters::KEY_ANTIBANDING[] = "antibanding";


/* for Image Adjust  */
const char SecCameraParameters::KEY_COLOR[] = "color";
const char SecCameraParameters::KEY_CONTRAST[] = "contrast";
const char SecCameraParameters::KEY_SHARPNESS[] = "sharpness";
const char SecCameraParameters::KEY_SATURATION[] = "saturation";


/* Values for scene mode settings. */
const char SecCameraParameters::SCENE_MODE_DUSK_DAWN[] = "dusk-dawn";
const char SecCameraParameters::SCENE_MODE_FALL_COLOR[] = "fall-color";
const char SecCameraParameters::SCENE_MODE_BACK_LIGHT[] = "back-light";
const char SecCameraParameters::SCENE_MODE_TEXT[] = "text";

/* Values for focus mode settings. */
const char SecCameraParameters::FOCUS_MODE_FACEDETECT[] = "facedetect";

/* Values for iso settings. */
const char SecCameraParameters::ISO_AUTO[] = "auto";
const char SecCameraParameters::ISO_50[] = "50";
const char SecCameraParameters::ISO_100[] = "100";
const char SecCameraParameters::ISO_200[] = "200";
const char SecCameraParameters::ISO_400[] = "400";
const char SecCameraParameters::ISO_800[] = "800";
const char SecCameraParameters::ISO_1600[] = "1600";
const char SecCameraParameters::ISO_SPORTS[] = "sports";
const char SecCameraParameters::ISO_NIGHT[] = "night";

/* Values for metering settings. */
const char SecCameraParameters::METERING_CENTER[] = "center";
const char SecCameraParameters::METERING_MATRIX[] = "matrix";
const char SecCameraParameters::METERING_SPOT[] = "spot";

SecCameraParameters::SecCameraParameters()
{
}

SecCameraParameters::~SecCameraParameters()
{
}

int SecCameraParameters::lookupAttr(const cam_strmap_t arr[], int len, const char *name)
{
    if (name) {
        for (int i = 0; i < len; i++) {
            if (!strcmp(arr[i].desc, name))
                return arr[i].val;
        }
    }
    return NOT_FOUND;
}

String8 SecCameraParameters::createSizesStr(const image_rect_type *sizes, int len)
{
    String8 str;
    char buffer[32];

    if (len > 0) {
        snprintf(buffer, sizeof(buffer), "%dx%d", sizes[0].width, sizes[0].height);
        str.append(buffer);
    }

    for (int i = 1; i < len; i++) {
        snprintf(buffer, sizeof(buffer), ",%dx%d", sizes[i].width, sizes[i].height);
        str.append(buffer);
    }
    return str;
}

String8 SecCameraParameters::createValuesStr(const cam_strmap_t *values, int len)
{
    String8 str;

    if (len > 0)
        str.append(values[0].desc);

    for (int i=1; i<len; i++) {
        str.append(",");
        str.append(values[i].desc);
    }
    return str;
}

};
