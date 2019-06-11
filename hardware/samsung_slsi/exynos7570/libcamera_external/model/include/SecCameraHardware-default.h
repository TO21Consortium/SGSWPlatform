/*
**
** Copyright 2008, The Android Open Source Project
** Copyright (c) 2011, Samsung Electronics Co. LTD
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
**
*/
#ifndef ANDROID_HARDWARE_SECCAMERAHARDWARE_DEFAULT_H
#define ANDROID_HARDWARE_SECCAMERAHARDWARE_DEFAULT_H

#include "Exif.h"
#include "SecCameraParameters.h"

/**
 * Define function feature
**/
#define MFC_7X_BUFFER_OFFSET            (256)
#define GRALLOC_LOCK_FOR_CAMERA         (GRALLOC_SET_USAGE_FOR_CAMERA)

#define USE_MEM2MEM_GSC
#ifdef USE_MEM2MEM_GSC
#define GRALLOC_SET_USAGE_FOR_CAMERA \
    (GRALLOC_USAGE_SW_READ_OFTEN | \
     GRALLOC_USAGE_SW_WRITE_OFTEN | \
     GRALLOC_USAGE_HW_TEXTURE | \
     GRALLOC_USAGE_HW_COMPOSER | \
     GRALLOC_USAGE_EXTERNAL_DISP | \
     GRALLOC_USAGE_HW_CAMERA_MASK)
#else
#define GRALLOC_SET_USAGE_FOR_CAMERA \
    (GRALLOC_USAGE_SW_READ_OFTEN | \
     GRALLOC_USAGE_SW_WRITE_OFTEN | \
     GRALLOC_USAGE_HW_TEXTURE | \
     GRALLOC_USAGE_HW_COMPOSER | \
     GRALLOC_USAGE_EXTERNAL_DISP)
#endif

#define PREVIEW_GSC_NODE_NUM            (4)  /* 4 = MSC from Exynos5420 */
#define PICTURE_GSC_NODE_NUM            (5)  /* 0,1,2 = GSC */
#define VIDEO_GSC_NODE_NUM              (4)

#define SUPPORT_64BITS
#define FLITE0_DEV_PATH  "/dev/video101" /* MAIN_CAMERA_FLITE_NUM */
#define FLITE1_DEV_PATH  "/dev/video102" /* FRONT_CAMERA_FLITE_NUM */

#define ZOOM_FUNCTION       true
#define FRONT_ZSL           false
/* #define DUMP_FILE         1 */

#define FIMC1_NODE_NUM (PICTURE_GSC_NODE_NUM) /* for preview */
#define FIMC2_NODE_NUM (VIDEO_GSC_NODE_NUM)   /* for recording */

/* #define VENDOR_FEATURE (1) */

#define JPEG_THUMBNAIL_QUALITY (30)
#define JPEG_QUALITY_ADJUST_TARGET    (97)

#define USE_LIMITATION_FOR_THIRD_PARTY

#define USE_DEDICATED_PREVIEW_ENQUEUE_THREAD

#define USE_CAMERA_PREVIEW_FRAME_SCHEDULER

/* video snapshot */
#define RECORDING_CAPTURE

/* In case of YUV capture on Rear camera */
#define REAR_USE_YUV_CAPTURE

/* #define FAKE_SENSOR */
/* #define DEBUG_RECORDING */

#define IsZoomSupported()                   (true)
#define IsAPZoomSupported()                 (ZOOM_FUNCTION)
#define IsAutoFocusSupported()              (true)
#define IsFlashSupported()                  (true)
#define IsFastCaptureSupportedOnRear()      (false)
#define IsFastCaptureSupportedOnFront()     (false)

#if (IsFlashSupported())
#define BACK_FLASH_LED_SUPPORT  (true)
#define FRONT_FLASH_LED_SUPPORT (false)

#define CHECK_FLASH_LED(id)     ((id)?(FRONT_FLASH_LED_SUPPORT):(BACK_FLASH_LED_SUPPORT))
#else
#define CHECK_FLASH_LED(id)     (false)
#endif

#ifdef CAMERA_GED_FEATURE
#else
#define SENSOR_NAME_GET_FROM_FILE
#endif

#ifdef CAMERA_GED_FEATURE
#else
#define SENSOR_FW_GET_FROM_FILE
#endif

#ifdef CAMERA_GED_FEATURE
#else
//#define SUPPORT_X4_ZOOM_AND_400STEP
#endif

namespace android {

#ifdef ANDROID_HARDWARE_SECCAMERAHARDWARE_CPP
/**
 * Exif Info dependent on Camera Module
**/

/* F Number */
const int Exif::DEFAULT_BACK_FNUMBER_NUM = 22;
const int Exif::DEFAULT_BACK_FNUMBER_DEN = 10;
const int Exif::DEFAULT_FRONT_FNUMBER_NUM = 22;
const int Exif::DEFAULT_FRONT_FNUMBER_DEN = 10;

/* Focal length */
const int Exif::DEFAULT_BACK_FOCAL_LEN_NUM = 330;
const int Exif::DEFAULT_BACK_FOCAL_LEN_DEN = 100;
const int Exif::DEFAULT_FRONT_FOCAL_LEN_NUM = 244;
const int Exif::DEFAULT_FRONT_FOCAL_LEN_DEN = 100;

/* Focal length 35mm */
const int Exif::DEFAULT_BACK_FOCAL_LEN_35mm = 33;
const int Exif::DEFAULT_FRONT_FOCAL_LEN_35mm = 29;
#endif

/**
 * Define Supported parameters for ISecCameraHardware
**/
#ifdef ANDROID_HARDWARE_ISECCAMERAHARDWARE_CPP

/* Frame Rates */
#define B_KEY_PREVIEW_FPS_RANGE_VALUE "15000,30000"
#define B_KEY_SUPPORTED_PREVIEW_FPS_RANGE_VALUE "(7000,7000),(15000,15000),(15000,30000),(30000,30000)"
#define B_KEY_SUPPORTED_PREVIEW_FRAME_RATES_VALUE "7,15,30"
#define B_KEY_PREVIEW_FRAME_RATE_VALUE 30

#define F_KEY_PREVIEW_FPS_RANGE_VALUE "15000,30000"
#define F_KEY_SUPPORTED_PREVIEW_FPS_RANGE_VALUE "(15000,15000),(24000,24000),(15000,30000)"
#define F_KEY_SUPPORTED_PREVIEW_FRAME_RATES_VALUE "15,24,30"
#define F_KEY_PREVIEW_FRAME_RATE_VALUE 30

/* Preferred preview size for video.
 * This preferred preview size must be in supported preview size list
 */
#define B_KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO_VALUE "1280x720"
#define F_KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO_VALUE "640x480"

/* Video */
#define B_KEY_VIDEO_STABILIZATION_SUPPORTED_VALUE       "false"
#define F_KEY_VIDEO_STABILIZATION_SUPPORTED_VALUE       "false"

#ifdef RECORDING_CAPTURE
#define KEY_VIDEO_SNAPSHOT_SUPPORTED_VALUE              "true"
#else
#define KEY_VIDEO_SNAPSHOT_SUPPORTED_VALUE              "false"
#endif

/* Focus */
#define B_KEY_NORMAL_FOCUS_DISTANCES_VALUE "0.15,1.20,Infinity" /* Normal */
#define B_KEY_MACRO_FOCUS_DISTANCES_VALUE   "0.10,0.15,0.30" /* Macro*/
#define F_KEY_FOCUS_DISTANCES_VALUE  "0.20,0.25,Infinity" /* Fixed */

/* Zoom */
#define B_KEY_ZOOM_SUPPORTED_VALUE                  "true"
#define B_KEY_SMOOTH_ZOOM_SUPPORTED_VALUE           "false"
#define F_KEY_ZOOM_SUPPORTED_VALUE                  "false"
#define F_KEY_SMOOTH_ZOOM_SUPPORTED_VALUE           "false"

/* AE, AWB Lock */
#define B_KEY_AUTO_EXPOSURE_LOCK_SUPPORTED_VALUE            "true"
#define B_KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED_VALUE        "true"
#define F_KEY_AUTO_EXPOSURE_LOCK_SUPPORTED_VALUE            "false"
#define F_KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED_VALUE        "false"

/* Face Detect */
#define B_KEY_MAX_NUM_DETECTED_FACES_HW_VALUE "0"
#define B_KEY_MAX_NUM_DETECTED_FACES_SW_VALUE "3"
#define F_KEY_MAX_NUM_DETECTED_FACES_HW_VALUE "0"
#define F_KEY_MAX_NUM_DETECTED_FACES_SW_VALUE "0"

/* AF Status */
#define AF_STATUS_FOCUSING        (0)
#define AF_STATUS_FOCUSED        (1)    /*SUCCESS*/
#define AF_STATUS_UNFOCUSED    (2)    /*FAIL*/

static const image_rect_type backSensorSize = {
    2576,    1932
};

static const image_rect_type frontSensorSize = {
    1600,    1200
};

static const image_rect_type backFLitePreviewSizes[] = {
    { 1280,   720 }, /* default */
    { 1024,   768 },
    { 720,    720 },
    { 720,    480 },
    { 640,    480 },
    { 352,    288 },
    { 320,    240 },
    { 176,    144 },
};

static const image_rect_type backFLiteCaptureSizes[] = {
    { 2576,   1932 },
    { 2560,   1440 },
    { 2560,   1536 },
    { 2560,   1920 },
    { 2048,   1536 },
    { 2048,   1152 },
    { 1920,   1920 },
    { 1600,   1200 },
    { 1280,   960 },
    { 1024,   768 },
    { 640,    480 },
};

static const image_rect_type frontFLitePreviewSizes[] = {
    { 640,    480 },
};

#ifdef USE_LIMITATION_FOR_THIRD_PARTY
static const image_rect_type frontFLitePreviewSizeHangout = {
    320,    240
};
#endif

static const image_rect_type frontFLiteCaptureSizes[] = {
    { 1600,    1200 },
    { 1280,    960 },
    { 640,    480 },
};

static const image_rect_type backPreviewSizes[] = {
    { 1280,   720 },
    { 1280,   960 },
    { 1024,   768 },
    { 720,    720 },
    { 720,    480 },
    { 640,    480 },
    { 352,    288 },
    { 320,    240 },
    { 176,    144 },
};

static const image_rect_type hiddenBackPreviewSizes[0] = {
};

static const image_rect_type backRecordingSizes[] = {
    { 1280,   720 }, /* default */
    { 720,    480 },
    { 640,    480 },
    { 320,    240 },
    { 176,    144 },
};

static const image_rect_type hiddenBackRecordingSizes[0] = {
};

static const image_rect_type backPictureSizes[] = {
    { 2576,   1932 },
    { 2560,   1440 },
    { 2560,   1920 },
    { 2048,   1536 },
    { 2048,   1152 },
    { 1920,   1920 },
    { 1600,   1200 },
    { 1280,   960 },
    { 1280,   720 },
    { 1024,   768 },
    { 640,    480 },
};

static const image_rect_type hiddenBackPictureSizes[] = {
    { 2560,   1536 },
};

static const image_rect_type frontPreviewSizes[] = {
    { 800,    600 },    /* Preview */
    { 640,    480 },    /* Recording */
    { 320,    240 },
    { 352,    288 },
    { 176,    144 },
};

static const image_rect_type hiddenFrontPreviewSizes[0] = {
};

static const image_rect_type frontRecordingSizes[] = {
    { 640,    480 },    /* default */
    { 352,    288 },
    { 320,    240 },
    { 176,    144 },
};

static const image_rect_type hiddenFrontRecordingSizes[0] = {
};

static const image_rect_type frontPictureSizes[] = {
    { 1600,    1200 },
    { 1280,    960 },
    { 640,    480 },
};

static const image_rect_type hiddenFrontPictureSizes[0] = {
};

static const image_rect_type backThumbSizes[] = {
    { 160,    120 },    /* default 4 : 3 */
    { 160,    160 },    /* 1 : 1 */
    { 160,    90 },    /* 16 : 9 */
    { 0,      0 },
};

static const image_rect_type frontThumbSizes[] = {
    { 160,    120 },    /* default 4 : 3 */
    { 0,      0 },
};

static const view_angle_type backHorizontalViewAngle[] = {
    { SIZE_RATIO(16, 9), 56.0f },
    { SIZE_RATIO(4, 3), 56.0f },
    { SIZE_RATIO(1, 1), 45.0f },
    { SIZE_RATIO(3, 2), 57.2f },
    { SIZE_RATIO(5, 3), 56.0f },
};

static const float backVerticalViewAngle = 41.0f;

static const view_angle_type frontHorizontalViewAngle[] = {
    { SIZE_RATIO(16, 9), 60.3f },
    { SIZE_RATIO(4, 3), 60.3f },
};

static const float frontVerticalViewAngle = 47.4f;

static const cam_strmap_t whiteBalances[] = {
    { CameraParameters::WHITE_BALANCE_AUTO,         WHITE_BALANCE_AUTO },
    { CameraParameters::WHITE_BALANCE_INCANDESCENT, WHITE_BALANCE_TUNGSTEN },
    { CameraParameters::WHITE_BALANCE_FLUORESCENT,  WHITE_BALANCE_FLUORESCENT },
    { CameraParameters::WHITE_BALANCE_DAYLIGHT,     WHITE_BALANCE_SUNNY },
    { CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT, WHITE_BALANCE_CLOUDY },
};

static const cam_strmap_t effects[] = {
    { CameraParameters::EFFECT_NONE,        IMAGE_EFFECT_NONE },
    { CameraParameters::EFFECT_MONO,        IMAGE_EFFECT_BNW },
    { CameraParameters::EFFECT_NEGATIVE,    IMAGE_EFFECT_NEGATIVE },
    { CameraParameters::EFFECT_SEPIA,       IMAGE_EFFECT_SEPIA }
};

static const cam_strmap_t sceneModes[] = {
    { CameraParameters::SCENE_MODE_AUTO,        SCENE_MODE_NONE },
    { CameraParameters::SCENE_MODE_PORTRAIT,    SCENE_MODE_PORTRAIT },
    { CameraParameters::SCENE_MODE_LANDSCAPE,   SCENE_MODE_LANDSCAPE },
    { CameraParameters::SCENE_MODE_NIGHT,       SCENE_MODE_NIGHTSHOT },
    { CameraParameters::SCENE_MODE_BEACH,       SCENE_MODE_BEACH_SNOW },
    { CameraParameters::SCENE_MODE_SUNSET,      SCENE_MODE_SUNSET },
    { CameraParameters::SCENE_MODE_FIREWORKS,    SCENE_MODE_FIREWORKS },
    { CameraParameters::SCENE_MODE_SPORTS,      SCENE_MODE_SPORTS },
    { CameraParameters::SCENE_MODE_PARTY,       SCENE_MODE_PARTY_INDOOR },
    { CameraParameters::SCENE_MODE_CANDLELIGHT, SCENE_MODE_CANDLE_LIGHT },
};

static const cam_strmap_t flashModes[] = {
    { CameraParameters::FLASH_MODE_OFF,         FLASH_MODE_OFF },
    { CameraParameters::FLASH_MODE_AUTO,        FLASH_MODE_AUTO },
    { CameraParameters::FLASH_MODE_ON,          FLASH_MODE_ON },
    { CameraParameters::FLASH_MODE_TORCH,       FLASH_MODE_TORCH },
};

static const cam_strmap_t previewPixelFormats[] = {
    { CameraParameters::PIXEL_FORMAT_YUV420SP,  CAM_PIXEL_FORMAT_YUV420SP },
    { CameraParameters::PIXEL_FORMAT_YUV420P,   CAM_PIXEL_FORMAT_YVU420P },
/*  { CameraParameters::PIXEL_FORMAT_YUV422I,   CAM_PIXEL_FORMAT_YUV422I },
    { CameraParameters::PIXEL_FORMAT_YUV422SP,  CAM_PIXEL_FORMAT_YUV422SP },
    { CameraParameters::PIXEL_FORMAT_RGB565,    CAM_PIXEL_FORMAT_RGB565, }, */
};

static const cam_strmap_t picturePixelFormats[] = {
    { CameraParameters::PIXEL_FORMAT_JPEG,      CAM_PIXEL_FORMAT_JPEG, },
};

#if IsAutoFocusSupported()
static const cam_strmap_t backFocusModes[] = {
    { CameraParameters::FOCUS_MODE_AUTO,        FOCUS_MODE_AUTO },
    { CameraParameters::FOCUS_MODE_INFINITY,    FOCUS_MODE_INFINITY },
    { CameraParameters::FOCUS_MODE_MACRO,       FOCUS_MODE_MACRO },
    { CameraParameters::FOCUS_MODE_FIXED,       FOCUS_MODE_FIXED },
/*  { CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO, FOCUS_MODE_CONTINUOUS },
    { SecCameraParameters::FOCUS_MODE_FACEDETECT, FOCUS_MODE_FACEDETECT }, */
};
#else
static const cam_strmap_t backFocusModes[] = {
    { CameraParameters::FOCUS_MODE_FIXED,       FOCUS_MODE_FIXED },
};
#endif

static const cam_strmap_t frontFocusModes[] = {
    { CameraParameters::FOCUS_MODE_FIXED,   FOCUS_MODE_FIXED },
    { CameraParameters::FOCUS_MODE_INFINITY,   FOCUS_MODE_INFINITY },
};

static const cam_strmap_t isos[] = {
    { SecCameraParameters::ISO_AUTO,    ISO_AUTO },
    { SecCameraParameters::ISO_100,     ISO_100 },
    { SecCameraParameters::ISO_200,     ISO_200 },
    { SecCameraParameters::ISO_400,     ISO_400 },
};

static const cam_strmap_t meterings[] = {
    { SecCameraParameters::METERING_CENTER, METERING_CENTER },
    { SecCameraParameters::METERING_MATRIX, METERING_MATRIX },
    { SecCameraParameters::METERING_SPOT,   METERING_SPOT },
};

static const cam_strmap_t antibandings[] = {
    { CameraParameters::ANTIBANDING_AUTO, ANTI_BANDING_AUTO },
    { CameraParameters::ANTIBANDING_50HZ, ANTI_BANDING_50HZ },
    { CameraParameters::ANTIBANDING_60HZ, ANTI_BANDING_60HZ },
    { CameraParameters::ANTIBANDING_OFF, ANTI_BANDING_OFF },
};

/* Define initial skip frame count */
static const int INITIAL_REAR_SKIP_FRAME = 3;
static const int INITIAL_FRONT_SKIP_FRAME = 3;

#endif /* ANDROID_HARDWARE_ISECCAMERAHARDWARE_CPP */
}
#endif /* ANDROID_HARDWARE_SECCAMERAHARDWARE_JOSHUA_H */
