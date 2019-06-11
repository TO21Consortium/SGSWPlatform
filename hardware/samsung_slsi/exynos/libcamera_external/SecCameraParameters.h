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
 * \file      SecCameraParameters.h
 * \brief     source file for Android Camera Ext HAL
 * \author    teahyung kim (tkon.kim@samsung.com)
 * \date      2013/04/30
 *
 */

#ifndef ANDROID_HARDWARE_SEC_CAMERA_PARAMETERS_H
#define ANDROID_HARDWARE_SEC_CAMERA_PARAMETERS_H

#include <exynos_format.h>
#include <videodev2.h>
#include <videodev2_exynos_camera_ext.h>
#include <videodev2_exynos_media.h>

#define NOT_FOUND       -1

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(*x))
#define FRM_RATIO(x)    ((x).width * 10 / (x).height)
#define SIZE_RATIO(w, h)     ((w) * 10 / (h))

namespace android {
/**
 *  The size of image for display.
 */
typedef struct image_rect_struct {
    uint32_t width;      /* Image width */
    uint32_t height;     /* Image height */
} image_rect_type;

typedef struct cam_strmap {
    const char *desc;
    int val;
} cam_strmap_t;

typedef struct view_angle {
    uint32_t size_ratio;
    float view_angle;
} view_angle_type;

typedef enum {
    CAM_CID_FW_MODE         = V4L2_CID_CAM_UPDATE_FW,
    CAM_CID_DTP_MODE        = V4L2_CID_CAMERA_CHECK_DATALINE,
    CAM_CID_VT_MODE         = V4L2_CID_CAMERA_VT_MODE,
    CAM_CID_MOVIE_MODE      = V4L2_CID_CAMERA_SENSOR_MODE,
    CAM_CID_JPEG_QUALITY    = V4L2_CID_CAMERA_JPEG_QUALITY,
    CAM_CID_ROTATION        = V4L2_CID_ROTATION,
    CAM_CID_SCENE_MODE      = V4L2_CID_CAMERA_SCENE_MODE,
    CAM_CID_ISO             = V4L2_CID_CAMERA_ISO,
    CAM_CID_BRIGHTNESS      = V4L2_CID_CAMERA_BRIGHTNESS,
    CAM_CID_SHARPNESS		= V4L2_CID_CAMERA_SHARPNESS,
    CAM_CID_CONTRAST		= V4L2_CID_CAMERA_CONTRAST,
    CAM_CID_SATURATION		= V4L2_CID_CAMERA_SATURATION,
    CAM_CID_WHITE_BALANCE   = V4L2_CID_WHITE_BALANCE_PRESET,
    CAM_CID_FLASH           = V4L2_CID_CAMERA_FLASH_MODE,
    CAM_CID_METERING        = V4L2_CID_CAMERA_METERING,
    CAM_CID_EFFECT          = V4L2_CID_CAMERA_EFFECT,
    CAM_CID_ZOOM            = V4L2_CID_CAMERA_ZOOM,
    CAM_CID_BLUR            = V4L2_CID_CAMERA_VGA_BLUR,
    CAM_CID_AUTO_CONTRAST   = V4L2_CID_CAMERA_WDR,
    CAM_CID_ANTISHAKE       = V4L2_CID_CAMERA_ANTI_SHAKE,
    CAM_CID_FRAME_RATE      = V4L2_CID_CAM_FRAME_RATE,
    CAM_CID_CHECK_ESD       = V4L2_CID_CAMERA_CHECK_ESD,
    CAM_CID_ANTIBANDING     = V4L2_CID_CAMERA_ANTI_BANDING,
    CAM_CID_OBJ_TRACKING    = V4L2_CID_CAMERA_OBJ_TRACKING_START_STOP,

    CAM_CID_FOCUS_MODE      = V4L2_CID_CAMERA_FOCUS_MODE,

    CAM_CID_SET_TOUCH_AF_POSX = V4L2_CID_CAMERA_OBJECT_POSITION_X,
    CAM_CID_SET_TOUCH_AF_POSY = V4L2_CID_CAMERA_OBJECT_POSITION_Y,
    CAM_CID_SET_TOUCH_AF    = V4L2_CID_CAMERA_TOUCH_AF_START_STOP,

    CAM_CID_VFLIP           = V4L2_CID_VFLIP,
    CAM_CID_HFLIP           = V4L2_CID_HFLIP,
    CAM_CID_AE_LOCK_UNLOCK      = V4L2_CID_CAMERA_AE_LOCK_UNLOCK,
    CAM_CID_AWB_LOCK_UNLOCK     = V4L2_CID_CAMERA_AWB_LOCK_UNLOCK,
    CAM_CID_AEAWB_LOCK_UNLOCK   = V4L2_CID_CAMERA_AEAWB_LOCK_UNLOCK,
    CAM_CID_IS_S_FORMAT_SCENARIO    = V4L2_CID_IS_S_FORMAT_SCENARIO,
    CAM_CID_ANTI_BANDING    = V4L2_CID_CAMERA_ANTI_BANDING,

    CAM_CID_CAPTURE_MODE = V4L2_CID_CAMERA_CAPTURE,
    CAM_CID_JPEG_MAIN_SIZE = V4L2_CID_CAM_JPEG_MAIN_SIZE,
} cam_control_id;

typedef enum {
    CAM_FRMRATIO_QCIF   = 12,   /* 11 : 9 */
    CAM_FRMRATIO_VGA    = 13,   /* 4 : 3 */
    CAM_FRMRATIO_D1     = 15,   /* 3 : 2 */
    CAM_FRMRATIO_WVGA   = 16,   /* 5 : 3 */
    CAM_FRMRATIO_HD     = 17,   /* 16 : 9 */
    CAM_FRMRATIO_SQUARE = 10,   /* 1 : 1 */
} cam_frmratio;

typedef enum {
    CAM_PIXEL_FORMAT_YUV422SP   = V4L2_PIX_FMT_NV61,
    CAM_PIXEL_FORMAT_YUV420SP   = V4L2_PIX_FMT_NV21,
    CAM_PIXEL_FORMAT_YUV420P    = V4L2_PIX_FMT_YUV420,
/*  CAM_PIXEL_FORMAT_YUV422I    = V4L2_PIX_FMT_VYUY, */
    CAM_PIXEL_FORMAT_YUV422I    = V4L2_PIX_FMT_YUYV,
    CAM_PIXEL_FORMAT_RGB565     = V4L2_PIX_FMT_RGB565,
    CAM_PIXEL_FORMAT_JPEG       = V4L2_PIX_FMT_JPEG,
/* Only for SAMSUNG */
/*CAM_PIXEL_FORMAT_YUV420     = V4L2_PIX_FMT_NV12, */
    CAM_PIXEL_FORMAT_YUV420     = V4L2_PIX_FMT_NV12M,
    CAM_PIXEL_FORMAT_YVU420P    = V4L2_PIX_FMT_YVU420, /* For support YV12 */
} cam_pixel_format;

class SecCameraParameters {
public:
    SecCameraParameters();
    ~SecCameraParameters();

    static const char KEY_DTP_MODE[];

    static const char KEY_VT_MODE[];
    static const char KEY_MOVIE_MODE[];

    static const char KEY_ISO[];
    static const char KEY_METERING[];
    static const char KEY_AUTO_CONTRAST[];
    static const char KEY_ANTI_SHAKE[];
    static const char KEY_FACE_BEAUTY[];
    static const char KEY_HDR_MODE[];
    static const char KEY_BLUR[];
    static const char KEY_ANTIBANDING[];

    static const char KEY_COLOR[];
    static const char KEY_CONTRAST[];
    static const char KEY_SHARPNESS[];
    static const char KEY_SATURATION[];

    // Values for scene mode settings.
    static const char SCENE_MODE_DUSK_DAWN[];
    static const char SCENE_MODE_FALL_COLOR[];
    static const char SCENE_MODE_BACK_LIGHT[];
    static const char SCENE_MODE_TEXT[];

    // Values for focus mode settings.
    static const char FOCUS_MODE_FACEDETECT[];

    // Values for ISO settings.
    static const char ISO_AUTO[];
    static const char ISO_50[];
    static const char ISO_100[];
    static const char ISO_200[];
    static const char ISO_400[];
    static const char ISO_800[];
    static const char ISO_1600[];
    static const char ISO_SPORTS[];
    static const char ISO_NIGHT[];

    // Values for metering settings.
    static const char METERING_CENTER[];
    static const char METERING_MATRIX[];
    static const char METERING_SPOT[];

    static int lookupAttr(const cam_strmap_t arr[], int len, const char *name);
    static String8 createSizesStr(const image_rect_type *sizes, int len);
    static String8 createValuesStr(const cam_strmap_t *values, int len);
};

}; // namespace android

#endif /* ANDROID_HARDWARE_SEC_CAMERA_PARAMETERS_H */

