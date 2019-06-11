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

/*#define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraSensorInfoBase"
#include <cutils/log.h>

#include "ExynosCameraSensorInfoBase.h"
#include "ExynosExif.h"

#ifdef BOARD_CAMERA_USES_DUAL_CAMERA
#include "ExynosCameraFusionInclude.h"
#endif

namespace android {

#ifdef SENSOR_NAME_GET_FROM_FILE
int g_rearSensorId = -1;
int g_frontSensorId = -1;
#endif

/* #define CALIBRATE_BCROP5_SIZE */  /* HACK for istor EVT0 3aa h/w bcrop5 */

#ifdef CALIBRATE_BCROP5_SIZE
typedef int     (*LutType)[SIZE_OF_LUT];

int calibrateB3(int srcSize, int dstSize) {
    int ratio = srcSize * 256 / dstSize;
    if (ratio < 0)
        ALOGE("ERR(%s:%d):devide by 0", __FUNCTION__, __LINE__);

    int calibrateDstSize = srcSize * 256 / ratio;

    /* make even number */
    calibrateDstSize -= (calibrateDstSize & 0x01);

    return calibrateDstSize;
}
#endif

int getSensorId(int camId)
{
    int sensorId = -1;

#ifdef SENSOR_NAME_GET_FROM_FILE
    int &curSensorId = (camId == CAMERA_ID_BACK) ? g_rearSensorId : g_frontSensorId;

    if (curSensorId < 0) {
        curSensorId = getSensorIdFromFile(camId);
        if (curSensorId < 0) {
            ALOGE("ERR(%s): invalid sensor ID %d", __FUNCTION__, sensorId);
        }
    }

    sensorId = curSensorId;
#else
    if (camId == CAMERA_ID_BACK) {
        sensorId = MAIN_CAMERA_SENSOR_NAME;
    } else if (camId == CAMERA_ID_FRONT) {
        sensorId = FRONT_CAMERA_SENSOR_NAME;
    } else if (camId == CAMERA_ID_BACK_1) {
#ifdef MAIN_1_CAMERA_SENSOR_NAME
        sensorId = MAIN_1_CAMERA_SENSOR_NAME;
#endif
    } else if (camId == CAMERA_ID_FRONT_1) {
#ifdef FRONT_1_CAMERA_SENSOR_NAME
        sensorId = FRONT_1_CAMERA_SENSOR_NAME;
#endif
    } else {
        ALOGE("ERR(%s):Unknown camera ID(%d)", __FUNCTION__, camId);
    }
#endif

    if (sensorId == SENSOR_NAME_NOTHING) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):camId(%d):sensorId == SENSOR_NAME_NOTHING, assert!!!!",
            __FUNCTION__, __LINE__, camId);
    }

done:
    return sensorId;
}

void getDualCameraId(int *cameraId_0, int *cameraId_1)
{
    if (cameraId_0 == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):cameraId_0 == NULL, assert!!!!",
            __FUNCTION__, __LINE__);
    }

    if (cameraId_1 == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):cameraId_1 == NULL, assert!!!!",
            __FUNCTION__, __LINE__);
    }

    int sensor1Name = -1;
    int tempCameraId_0 = -1;
    int tempCameraId_1 = -1;

#ifdef MAIN_1_CAMERA_SENSOR_NAME
    sensor1Name = MAIN_1_CAMERA_SENSOR_NAME;

    if (sensor1Name != SENSOR_NAME_NOTHING) {
        tempCameraId_0 = CAMERA_ID_BACK;
        tempCameraId_1 = CAMERA_ID_BACK_1;

        goto done;
    }
#endif

#ifdef FRONT_1_CAMERA_SENSOR_NAME
    sensor1Name = FRONT_1_CAMERA_SENSOR_NAME;

    if (sensor1Name != SENSOR_NAME_NOTHING) {
        tempCameraId_0 = CAMERA_ID_FRONT;
        tempCameraId_1 = CAMERA_ID_FRONT_1;

        goto done;
    }
#endif

done:
    *cameraId_0 = tempCameraId_0;
    *cameraId_1 = tempCameraId_1;
}

ExynosSensorInfoBase::ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 4128;
    maxPictureH = 3096;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 4128;
    maxSensorH = 3096;
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 22;
    fNumberDen = 10;
    focalLengthNum = 420;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    minExposureTime = 0;
    maxExposureTime = 0;
    minWBK = 0;
    maxWBK = 0;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxBasicZoomLevel = MAX_BASIC_ZOOM_LEVEL;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = false;
    drcSupport = false;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;
    minFps = 0;
    maxFps = 30;
    /* flite->3aa otf support */
    flite3aaOtfSupport = true;

    rearPreviewListMax = 0;
    frontPreviewListMax = 0;
    rearPictureListMax = 0;
    frontPictureListMax = 0;
    hiddenRearPreviewListMax = 0;
    hiddenFrontPreviewListMax = 0;
    hiddenRearPictureListMax = 0;
    hiddenFrontPictureListMax = 0;
    thumbnailListMax = 0;
    rearVideoListMax = 0;
    frontVideoListMax = 0;
    hiddenRearVideoListMax = 0;
    hiddenFrontVideoListMax = 0;
    rearFPSListMax = 0;
    frontFPSListMax = 0;
    hiddenRearFPSListMax = 0;
    hiddenFrontFPSListMax = 0;

    rearPreviewList = NULL;
    frontPreviewList = NULL;
    rearPictureList = NULL;
    frontPictureList = NULL;
    hiddenRearPreviewList = NULL;
    hiddenFrontPreviewList = NULL;
    hiddenRearPictureList = NULL;
    hiddenFrontPictureList = NULL;
    thumbnailList = NULL;
    rearVideoList = NULL;
    frontVideoList = NULL;
    hiddenRearVideoList = NULL;
    hiddenFrontVideoList = NULL;
    rearFPSList = NULL;
    frontFPSList = NULL;
    hiddenRearFPSList = NULL;
    hiddenFrontFPSList = NULL;

    meteringList =
          METERING_MODE_MATRIX
        | METERING_MODE_CENTER
        | METERING_MODE_SPOT
        ;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        | FLASH_MODE_RED_EYE
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        | FOCUS_MODE_FIXED
        | FOCUS_MODE_EDOF
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        | WHITE_BALANCE_WARM_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        | WHITE_BALANCE_TWILIGHT
        | WHITE_BALANCE_SHADE;

    isoValues =
          ISO_AUTO
        | ISO_100
        | ISO_200
        | ISO_400
        | ISO_800;

    previewSizeLutMax           = 0;
    pictureSizeLutMax           = 0;
    videoSizeLutMax             = 0;
    vtcallSizeLutMax            = 0;
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;
    videoSizeLutHighSpeed240Max = 0;
    liveBroadcastSizeLutMax     = 0;
    depthMapSizeLutMax          = 0;
    fastAeStableLutMax          = 0;

    previewSizeLut              = NULL;
    pictureSizeLut              = NULL;
    videoSizeLut                = NULL;
    videoSizeBnsLut             = NULL;
    dualPreviewSizeLut          = NULL;
    dualVideoSizeLut            = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    videoSizeLutHighSpeed240    = NULL;
    vtcallSizeLut               = NULL;
    liveBroadcastSizeLut        = NULL;
    depthMapSizeLut             = NULL;
    fastAeStableLut             = NULL;
    sizeTableSupport            = false;

#ifdef BOARD_CAMERA_USES_DUAL_CAMERA
    dof = new DOF_BASE;
#endif
}

ExynosSensorIMX135Base::ExynosSensorIMX135Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 4128;
    maxPictureH = 3096;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 4144;
    maxSensorH = 3106;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 22;
    fNumberDen = 10;
    focalLengthNum = 420;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        /*| FLASH_MODE_RED_EYE*/
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        /*| FOCUS_MODE_INFINITY*/
        | FOCUS_MODE_MACRO
        /*| FOCUS_MODE_FIXED*/
        /*| FOCUS_MODE_EDOF*/
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /*| WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /*| WHITE_BALANCE_TWILIGHT*/
        /*| WHITE_BALANCE_SHADE*/
        ;

    previewSizeLutMax           = 0;
    pictureSizeLutMax           = 0;
    videoSizeLutMax             = 0;
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut              = NULL;
    pictureSizeLut              = NULL;
    videoSizeLut                = NULL;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport            = false;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(IMX135_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(IMX135_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(IMX135_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(IMX135_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(IMX135_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(IMX135_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(IMX135_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(IMX135_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(IMX135_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = IMX135_PREVIEW_LIST;
    rearPictureList     = IMX135_PICTURE_LIST;
    hiddenRearPreviewList   = IMX135_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = IMX135_HIDDEN_PICTURE_LIST;
    thumbnailList   = IMX135_THUMBNAIL_LIST;
    rearVideoList       = IMX135_VIDEO_LIST;
    hiddenRearVideoList   = IMX135_HIDDEN_VIDEO_LIST;
    rearFPSList       = IMX135_FPS_RANGE_LIST;
    hiddenRearFPSList   = IMX135_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorIMX134Base::ExynosSensorIMX134Base() : ExynosSensorInfoBase()
{
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080)
    maxPreviewW = 3264;
    maxPreviewH = 2448;
#else
    maxPreviewW = 1920;
    maxPreviewH = 1080;
#endif
    maxPictureW = 3264;
    maxPictureH = 2448;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 3280;
    maxSensorH = 2458;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 24;
    fNumberDen = 10;
    focalLengthNum = 340;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 253;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 56.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 44.3f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 34.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 48.1f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 44.3f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 52.8f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 44.3f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = false;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
#ifndef USE_CAMERA2_API_SUPPORT
        | EFFECT_BEAUTY_FACE
#endif
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        /*| FLASH_MODE_RED_EYE*/
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        /*| FOCUS_MODE_INFINITY*/
        | FOCUS_MODE_MACRO
        /*| FOCUS_MODE_FIXED*/
        /*| FOCUS_MODE_EDOF*/
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /*| WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /*| WHITE_BALANCE_TWILIGHT*/
        /*| WHITE_BALANCE_SHADE*/
        ;

    previewSizeLutMax           = 0;
    pictureSizeLutMax           = 0;
    videoSizeLutMax             = 0;
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut              = NULL;
    pictureSizeLut              = NULL;
    videoSizeLut                = NULL;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport            = false;

    /* vendor specifics */
    /*
    burstPanoramaW = 3264;
    burstPanoramaH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    */
    bnsSupport = false;

    if (bnsSupport == true) {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        sizeTableSupport            = false;
    } else {
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_IMX134) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX134)   / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_IMX134) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX134) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX134) / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = PREVIEW_SIZE_LUT_IMX134;
        videoSizeLut                = VIDEO_SIZE_LUT_IMX134;
        pictureSizeLut              = PICTURE_SIZE_LUT_IMX134;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX134;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX134;
        sizeTableSupport            = true;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(IMX134_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(IMX134_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(IMX134_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(IMX134_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(IMX134_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(IMX134_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(IMX134_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(IMX134_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(IMX134_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = IMX134_PREVIEW_LIST;
    rearPictureList     = IMX134_PICTURE_LIST;
    hiddenRearPreviewList   = IMX134_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = IMX134_HIDDEN_PICTURE_LIST;
    thumbnailList   = IMX134_THUMBNAIL_LIST;
    rearVideoList       = IMX134_VIDEO_LIST;
    hiddenRearVideoList   = IMX134_HIDDEN_VIDEO_LIST;
    rearFPSList       = IMX134_FPS_RANGE_LIST;
    hiddenRearFPSList   = IMX134_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K3L2Base::ExynosSensorS5K3L2Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 4128;
    maxPictureH = 3096;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 4144;
    maxSensorH = 3106;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 20;
    fNumberDen = 10;
    focalLengthNum = 420;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

#ifdef USE_SUBDIVIDED_EV
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        | EFFECT_COLD_VINTAGE
        | EFFECT_BLUE
        | EFFECT_RED_YELLOW
        | EFFECT_AQUA
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        /*| FLASH_MODE_RED_EYE*/
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        /*| FOCUS_MODE_INFINITY*/
        | FOCUS_MODE_MACRO
        /*| FOCUS_MODE_FIXED*/
        /*| FOCUS_MODE_EDOF*/
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /*| WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /*| WHITE_BALANCE_TWILIGHT*/
        /*| WHITE_BALANCE_SHADE*/
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 2056;
    highSpeedRecording60H = 1152;
    highSpeedRecording120W = 1024;
    highSpeedRecording120H = 574;
    scalableSensorSupport = true;
    bnsSupport = false;

    if (bnsSupport == true) {
        previewSizeLutMax = sizeof(PREVIEW_SIZE_LUT_3L2_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax   = sizeof(VIDEO_SIZE_LUT_3L2)   / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax  = 0;
        previewSizeLut    = PREVIEW_SIZE_LUT_3L2_BNS;
        videoSizeLut      = VIDEO_SIZE_LUT_3L2;
        videoSizeBnsLut   = VIDEO_SIZE_LUT_3L2_BNS;
        vtcallSizeLut     = NULL;
    } else {
        previewSizeLutMax = sizeof(PREVIEW_SIZE_LUT_3L2) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax   = sizeof(VIDEO_SIZE_LUT_3L2)   / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax  = sizeof(VTCALL_SIZE_LUT_3L2) / (sizeof(int) * SIZE_OF_LUT);
        previewSizeLut    = PREVIEW_SIZE_LUT_3L2;
        videoSizeLut      = VIDEO_SIZE_LUT_3L2;
        videoSizeBnsLut   = NULL;
        vtcallSizeLut     = VTCALL_SIZE_LUT_3L2;
    }
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_3L2) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3L2) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3L2) / (sizeof(int) * SIZE_OF_LUT);

    pictureSizeLut              = PICTURE_SIZE_LUT_3L2;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3L2;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3L2;
    sizeTableSupport            = true;

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K3L2_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K3L2_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K3L2_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K3L2_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K3L2_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K3L2_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K3L2_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K3L2_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K3L2_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K3L2_PREVIEW_LIST;
    rearPictureList     = S5K3L2_PICTURE_LIST;
    hiddenRearPreviewList   = S5K3L2_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K3L2_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K3L2_THUMBNAIL_LIST;
    rearVideoList       = S5K3L2_VIDEO_LIST;
    hiddenRearVideoList   = S5K3L2_HIDDEN_VIDEO_LIST;
    rearFPSList       = S5K3L2_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K3L2_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K3L8Base::ExynosSensorS5K3L8Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 4128;
    maxPictureH = 3096;
    maxVideoW = 3840;
    maxVideoH = 2160;
    maxSensorW = 4208;
    maxSensorH = 3120;
    sensorMarginW = 0;
    sensorMarginH = 0;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 20;
    fNumberDen = 10;
    focalLengthNum = 365;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 185;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 53.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 41.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 76.5f;
    focalLengthIn35mmLength = 27;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    minExposureTime = 32;
    maxExposureTime = 10000000;
    minWBK = 2300;
    maxWBK = 10000;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        | EFFECT_BEAUTY_FACE
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        //| FLASH_MODE_RED_EYE
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        //| FOCUS_MODE_FIXED
        //| FOCUS_MODE_EDOF
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_PARTY
        | SCENE_MODE_SPORTS
        | SCENE_MODE_CANDLELIGHT
        */
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        //| WHITE_BALANCE_WARM_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| WHITE_BALANCE_TWILIGHT
        //| WHITE_BALANCE_SHADE
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = true;

    if (bnsSupport == true) {

#if defined(USE_BNS_PREVIEW)
        previewSizeLutMax     = sizeof(PREVIEW_SIZE_LUT_3L8_BNS_15) / (sizeof(int) * SIZE_OF_LUT);
#else
        previewSizeLutMax     = sizeof(PREVIEW_SIZE_LUT_3L8_BDS)    / (sizeof(int) * SIZE_OF_LUT);
#endif
        videoSizeLutMax       = sizeof(VIDEO_SIZE_LUT_3L8_BDS)      / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax     = sizeof(PICTURE_SIZE_LUT_3L8)        / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax      = sizeof(VTCALL_SIZE_LUT_3L8_BNS)     / (sizeof(int) * SIZE_OF_LUT);

        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_HIGH_SPEED_3L8_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_HIGH_SPEED_3L8_BNS) / (sizeof(int) * SIZE_OF_LUT);

#if defined(USE_BNS_PREVIEW)
        previewSizeLut        = PREVIEW_SIZE_LUT_3L8_BNS_15;
#else
        previewSizeLut        = PREVIEW_SIZE_LUT_3L8_BDS;
#endif
        videoSizeLut          = VIDEO_SIZE_LUT_3L8_BDS;
        videoSizeBnsLut       = VIDEO_SIZE_LUT_3L8_BNS_15;
        pictureSizeLut        = PICTURE_SIZE_LUT_3L8;

#if defined(USE_BNS_DUAL_PREVIEW)
#if defined(DUAL_BNS_RATIO) && (DUAL_BNS_RATIO == 1500)
        dualPreviewSizeLut    = PREVIEW_SIZE_LUT_3L8_BNS_15;
#else
        dualPreviewSizeLut    = PREVIEW_SIZE_LUT_3L8_BNS_20;
#endif
#else
        dualPreviewSizeLut    = PREVIEW_SIZE_LUT_3L8_BDS;
#endif // USE_BNS_DUAL_PREVIEW

#if defined(USE_BNS_DUAL_RECORDING)
#if defined(DUAL_BNS_RATIO) && (DUAL_BNS_RATIO == 1500)
        dualVideoSizeLut      = VIDEO_SIZE_LUT_3L8_BNS_15;
#else
        dualVideoSizeLut      = VIDEO_SIZE_LUT_3L8_BNS_20;
#endif
#else
        dualVideoSizeLut      = VIDEO_SIZE_LUT_3L8_BDS;
#endif //  USE_BNS_DUAL_RECORDING

        videoSizeLutHighSpeed60  = VIDEO_SIZE_LUT_HIGH_SPEED_3L8_BNS;
        videoSizeLutHighSpeed120 = VIDEO_SIZE_LUT_HIGH_SPEED_3L8_BNS;
        vtcallSizeLut         = VTCALL_SIZE_LUT_3L8_BNS;
        sizeTableSupport      = true;
    } else {
        previewSizeLutMax     = 0;
        pictureSizeLutMax     = 0;
        videoSizeLutMax       = 0;
        vtcallSizeLutMax      = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut        = NULL;
        pictureSizeLut        = NULL;
        videoSizeLut          = NULL;
        videoSizeBnsLut       = NULL;
        vtcallSizeLut         = NULL;
        videoSizeLutHighSpeed60  = NULL;
        videoSizeLutHighSpeed120 = NULL;

        sizeTableSupport      = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K3L8_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K3L8_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K3L8_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K3L8_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K3L8_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K3L8_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K3L8_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K3L8_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K3L8_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K3L8_PREVIEW_LIST;
    rearPictureList     = S5K3L8_PICTURE_LIST;
    hiddenRearPreviewList   = S5K3L8_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K3L8_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K3L8_THUMBNAIL_LIST;
    rearVideoList       = S5K3L8_VIDEO_LIST;
    hiddenRearVideoList   = S5K3L8_HIDDEN_VIDEO_LIST;
    rearFPSList       = S5K3L8_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K3L8_HIDDEN_FPS_RANGE_LIST;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(S5K3L8_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(S5K3L8_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(S5K3L8_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(S5K3L8_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K3L8_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(S5K3L8_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(S5K3L8_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(S5K3L8_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(S5K3L8_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = S5K3L8_PREVIEW_LIST;
    frontPictureList     = S5K3L8_PICTURE_LIST;
    hiddenFrontPreviewList   = S5K3L8_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = S5K3L8_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K3L8_THUMBNAIL_LIST;
    frontVideoList       = S5K3L8_VIDEO_LIST;
    hiddenFrontVideoList   = S5K3L8_HIDDEN_VIDEO_LIST;
    frontFPSList       = S5K3L8_FPS_RANGE_LIST;
    hiddenFrontFPSList   = S5K3L8_HIDDEN_FPS_RANGE_LIST;

#ifdef BOARD_CAMERA_USES_DUAL_CAMERA
    dof = new DOF_3L8;
#endif
};

ExynosSensorS5K3L8DualBdsBase::ExynosSensorS5K3L8DualBdsBase() : ExynosSensorS5K3L8Base()
{
    /* this sensor is for BDS when Dual */
    if (bnsSupport == true) {
        dualPreviewSizeLut    = PREVIEW_SIZE_LUT_3L8_BDS;
        dualVideoSizeLut      = VIDEO_SIZE_LUT_3L8_BDS;
    } else {
        dualPreviewSizeLut    = NULL;
        dualVideoSizeLut      = NULL;
    }
}

#if 0
ExynosSensorS5K2P2Base::ExynosSensorS5K2P2Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 3840;
    maxPreviewH = 2160;
    maxPictureW = 5312;
    maxPictureH = 2988;
    maxVideoW = 3840;
    maxVideoH = 2160;
    maxSensorW = 5328;
    maxSensorH = 3000;
    sensorMarginW = 16;
    sensorMarginH = 12;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 430;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 185;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 53.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 41.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 41.0f;
    focalLengthIn35mmLength = 28;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        | EFFECT_BEAUTY_FACE
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        //| FLASH_MODE_RED_EYE
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        //| FOCUS_MODE_FIXED
        //| FOCUS_MODE_EDOF
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_PARTY
        | SCENE_MODE_SPORTS
        | SCENE_MODE_CANDLELIGHT
        */
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        //| WHITE_BALANCE_WARM_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| WHITE_BALANCE_TWILIGHT
        //| WHITE_BALANCE_SHADE
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = true;

    if (bnsSupport == true) {
#if defined(USE_BNS_PREVIEW)
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);
#else
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2P2) / (sizeof(int) * SIZE_OF_LUT);
#endif
#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2P2_8MP_FULL) / (sizeof(int) * SIZE_OF_LUT);
#else
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2P2) / (sizeof(int) * SIZE_OF_LUT);
#endif
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2P2) / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);

#if defined(USE_BNS_PREVIEW)
        previewSizeLut              = PREVIEW_SIZE_LUT_2P2_BNS;
#else
        previewSizeLut              = PREVIEW_SIZE_LUT_2P2;
#endif
        dualPreviewSizeLut          = PREVIEW_SIZE_LUT_2P2_BNS;
#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLut                = VIDEO_SIZE_LUT_2P2_8MP_FULL;
#else
        videoSizeLut                = VIDEO_SIZE_LUT_2P2;
#endif
        videoSizeBnsLut             = VIDEO_SIZE_LUT_2P2_BNS;
        pictureSizeLut              = PICTURE_SIZE_LUT_2P2;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P2_BNS;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P2_BNS;
        vtcallSizeLut               = VTCALL_SIZE_LUT_2P2_BNS;
        sizeTableSupport            = true;
    } else {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        vtcallSizeLutMax            = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeBnsLut             = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        vtcallSizeLut               = NULL;
        sizeTableSupport            = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K2P2_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K2P2_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K2P2_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K2P2_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K2P2_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K2P2_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K2P2_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K2P2_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K2P2_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K2P2_PREVIEW_LIST;
    rearPictureList     = S5K2P2_PICTURE_LIST;
    hiddenRearPreviewList   = S5K2P2_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K2P2_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K2P2_THUMBNAIL_LIST;
    rearVideoList       = S5K2P2_VIDEO_LIST;
    hiddenRearVideoList   = S5K2P2_HIDDEN_VIDEO_LIST;
    rearFPSList       = S5K2P2_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K2P2_HIDDEN_FPS_RANGE_LIST;
};
#endif

ExynosSensorS5K3P3Base::ExynosSensorS5K3P3Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
#ifdef USE_CAMERA2_API_SUPPORT /* HACK : current HAL3.2 running with 4:3 ratio */
    maxPreviewW = 1440;
    maxPreviewH = 1080;
#endif
    maxPictureW = 4608;
    maxPictureH = 3456;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 4624;
    maxSensorH = 3466;
    sensorMarginW = 16;
    sensorMarginH = 10;
    //check until here
    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 22;
    fNumberDen = 10;
    focalLengthNum = 480;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

#ifdef USE_SUBDIVIDED_EV
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_RED_YELLOW
        | EFFECT_BLUE
        | EFFECT_COLD_VINTAGE
        | EFFECT_AQUA
#ifndef USE_CAMERA2_API_SUPPORT
        | EFFECT_BEAUTY_FACE
#endif
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        //| FLASH_MODE_RED_EYE
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        //| FOCUS_MODE_FIXED
        //| FOCUS_MODE_EDOF
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_PARTY
        | SCENE_MODE_SPORTS
        | SCENE_MODE_CANDLELIGHT*/
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        //| WHITE_BALANCE_WARM_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| WHITE_BALANCE_TWILIGHT
        //| WHITE_BALANCE_SHADE
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = true;

    if (bnsSupport == true) {
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_3P3) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_3P3)   / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_3P3) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3P3_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3P3_BNS) / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = PREVIEW_SIZE_LUT_3P3;
        dualPreviewSizeLut          = PREVIEW_SIZE_LUT_3P3_BNS;
        videoSizeLut                = VIDEO_SIZE_LUT_3P3;
        videoSizeBnsLut             = VIDEO_SIZE_LUT_3P3_BNS;
        pictureSizeLut              = PICTURE_SIZE_LUT_3P3;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3P3_BNS;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3P3_BNS;

        sizeTableSupport      = true;
    } else {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeBnsLut             = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        sizeTableSupport            = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K3P3_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K3P3_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K3P3_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K3P3_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K3P3_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K3P3_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K3P3_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K3P3_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K3P3_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K3P3_PREVIEW_LIST;
    rearPictureList     = S5K3P3_PICTURE_LIST;
    hiddenRearPreviewList   = S5K3P3_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K3P3_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K3P3_THUMBNAIL_LIST;
    rearVideoList       = S5K3P3_VIDEO_LIST;
    hiddenRearVideoList   = S5K3P3_HIDDEN_VIDEO_LIST;
    rearFPSList       = S5K3P3_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K3P3_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K2P2_12MBase::ExynosSensorS5K2P2_12MBase() : ExynosSensorInfoBase()
{
    maxPreviewW = 3840;
    maxPreviewH = 2160;
    maxPictureW = 4608;
    maxPictureH = 2592;
    maxVideoW = 3840;
    maxVideoH = 2160;
    maxSensorW = 4624;
    maxSensorH = 2604;
    sensorMarginW = 16;
    sensorMarginH = 12;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 22;
    fNumberDen = 10;
    focalLengthNum = 409;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
#ifndef USE_CAMERA2_API_SUPPORT
        | EFFECT_BEAUTY_FACE
#endif
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        //| FLASH_MODE_RED_EYE
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        //| FOCUS_MODE_FIXED
        //| FOCUS_MODE_EDOF
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_PARTY
        | SCENE_MODE_SPORTS
        | SCENE_MODE_CANDLELIGHT*/
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        //| WHITE_BALANCE_WARM_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| WHITE_BALANCE_TWILIGHT
        //| WHITE_BALANCE_SHADE
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = true;

    if (bnsSupport == true) {
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2P2_12M_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2P2_12M)   / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2P2_12M)     / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_2P2_12M_BNS)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P2_12M_BNS)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P2_12M_BNS)     / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = PREVIEW_SIZE_LUT_2P2_12M_BNS;
        videoSizeLut                = VIDEO_SIZE_LUT_2P2_12M;
        videoSizeBnsLut             = VIDEO_SIZE_LUT_2P2_12M_BNS;
        pictureSizeLut              = PICTURE_SIZE_LUT_2P2_12M;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P2_12M_BNS;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P2_12M_BNS;
        vtcallSizeLut               = VTCALL_SIZE_LUT_2P2_12M_BNS;
        sizeTableSupport            = true;
    } else {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        vtcallSizeLutMax            = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeBnsLut             = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        vtcallSizeLut               = NULL;
        sizeTableSupport            = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K2P2_12M_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K2P2_12M_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K2P2_12M_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K2P2_12M_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K2P2_12M_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K2P2_12M_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K2P2_12M_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K2P2_12M_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K2P2_12M_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K2P2_12M_PREVIEW_LIST;
    rearPictureList     = S5K2P2_12M_PICTURE_LIST;
    hiddenRearPreviewList   = S5K2P2_12M_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K2P2_12M_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K2P2_12M_THUMBNAIL_LIST;
    rearVideoList       = S5K2P2_12M_VIDEO_LIST;
    hiddenRearVideoList   = S5K2P2_12M_HIDDEN_VIDEO_LIST;
    rearFPSList       = S5K2P2_12M_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K2P2_12M_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K2P3Base::ExynosSensorS5K2P3Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 3840;
    maxPreviewH = 2160;
    maxPictureW = 5312;
    maxPictureH = 2990;
    maxVideoW = 3840;
    maxVideoH = 2160;
    maxSensorW = 5328;
    maxSensorH = 3000;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 22;
    fNumberDen = 10;
    focalLengthNum = 480;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
        EFFECT_NONE
        ;

    hiddenEffectList =
         EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        | EFFECT_BEAUTY_FACE
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        //| FLASH_MODE_RED_EYE
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        //| FOCUS_MODE_FIXED
        //| FOCUS_MODE_EDOF
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
                | SCENE_MODE_PORTRAIT
                | SCENE_MODE_LANDSCAPE
                | SCENE_MODE_NIGHT
                | SCENE_MODE_NIGHT_PORTRAIT
                | SCENE_MODE_THEATRE
                | SCENE_MODE_BEACH
                | SCENE_MODE_SNOW
                | SCENE_MODE_SUNSET
                | SCENE_MODE_STEADYPHOTO
                | SCENE_MODE_FIREWORKS
                | SCENE_MODE_PARTY
                | SCENE_MODE_SPORTS
                | SCENE_MODE_CANDLELIGHT*/
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        //| WHITE_BALANCE_WARM_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| WHITE_BALANCE_TWILIGHT
        //| WHITE_BALANCE_SHADE
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = true;

    if (bnsSupport == true) {
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2P3_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2P3_BNS)   / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2P3)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P3_BNS)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P3_BNS)     / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = PREVIEW_SIZE_LUT_2P3_BNS;
        videoSizeLut                = VIDEO_SIZE_LUT_2P3_BNS;
        videoSizeBnsLut             = VIDEO_SIZE_LUT_2P3_BNS;
        pictureSizeLut              = PICTURE_SIZE_LUT_2P3;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P3_BNS;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P3_BNS;
        sizeTableSupport            = true;
    } else {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        vtcallSizeLutMax            = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        vtcallSizeLut               = NULL;
        sizeTableSupport            = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K2P3_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K2P3_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K2P3_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K2P3_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K2P3_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K2P3_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K2P3_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K2P3_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K2P3_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K2P3_PREVIEW_LIST;
    rearPictureList     = S5K2P3_PICTURE_LIST;
    hiddenRearPreviewList   = S5K2P3_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K2P3_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K2P3_THUMBNAIL_LIST;
    rearVideoList       = S5K2P3_VIDEO_LIST;
    hiddenRearVideoList   = S5K2P3_HIDDEN_VIDEO_LIST;
    rearFPSList       = S5K2P3_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K2P3_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K2T2Base::ExynosSensorS5K2T2Base() : ExynosSensorInfoBase()
{
#if defined(ENABLE_13MP_FULL_FRAME)
    maxPreviewW = 4800;
    maxPreviewH = 2700;
#else
    maxPreviewW = 3840;
    maxPreviewH = 2160;
#endif
    maxPictureW = 5952;
    maxPictureH = 3348;
    maxVideoW = 3840;
    maxVideoH = 2160;
    maxSensorW = 5968;
    maxSensorH = 3368;
    sensorMarginW = 16;
    sensorMarginH = 12;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 430;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 68.13f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 41.0f;
    focalLengthIn35mmLength = 28;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        | EFFECT_BEAUTY_FACE
        ;

    flashModeList =
        FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        //| FLASH_MODE_RED_EYE
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        //| FOCUS_MODE_FIXED
        //| FOCUS_MODE_EDOF
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_PARTY
        | SCENE_MODE_SPORTS
        | SCENE_MODE_CANDLELIGHT
        */
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        //| WHITE_BALANCE_WARM_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| WHITE_BALANCE_TWILIGHT
        //| WHITE_BALANCE_SHADE
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = true;

    if (bnsSupport == true) {
#if defined(USE_BNS_PREVIEW)
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2T2_BNS) / (sizeof(int) * SIZE_OF_LUT);
#else
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2T2) / (sizeof(int) * SIZE_OF_LUT);
#endif
#ifdef ENABLE_13MP_FULL_FRAME
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2T2_13MP_FULL)       / (sizeof(int) * SIZE_OF_LUT);
#elif defined(ENABLE_8MP_FULL_FRAME)
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2T2_8MP_FULL)       / (sizeof(int) * SIZE_OF_LUT);
#else
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2T2)       / (sizeof(int) * SIZE_OF_LUT);
#endif
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2T2)     / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_2T2_BNS)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2T2_BNS)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2T2_BNS)     / (sizeof(int) * SIZE_OF_LUT);

#if defined(USE_BNS_PREVIEW)
        previewSizeLut              = PREVIEW_SIZE_LUT_2T2_BNS;
#else
        previewSizeLut              = PREVIEW_SIZE_LUT_2T2;
#endif
        dualPreviewSizeLut          = PREVIEW_SIZE_LUT_2T2_BNS;
#ifdef ENABLE_13MP_FULL_FRAME
        videoSizeLut                = VIDEO_SIZE_LUT_2T2_13MP_FULL;
#elif defined(ENABLE_8MP_FULL_FRAME)
        videoSizeLut                = VIDEO_SIZE_LUT_2T2_8MP_FULL;
#else
        videoSizeLut                = VIDEO_SIZE_LUT_2T2;
#endif
        videoSizeBnsLut             = VIDEO_SIZE_LUT_2T2_BNS;
        pictureSizeLut              = PICTURE_SIZE_LUT_2T2;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2T2_BNS;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2T2_BNS;
        vtcallSizeLut               = VTCALL_SIZE_LUT_2T2_BNS;
        sizeTableSupport            = true;
    } else {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        vtcallSizeLutMax            = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;
        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeBnsLut             = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        vtcallSizeLut               = NULL;
        sizeTableSupport            = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K2T2_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K2T2_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K2T2_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K2T2_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K2T2_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K2T2_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K2T2_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K2T2_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K2T2_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K2T2_PREVIEW_LIST;
    rearPictureList     = S5K2T2_PICTURE_LIST;
    hiddenRearPreviewList   = S5K2T2_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K2T2_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K2T2_THUMBNAIL_LIST;
    rearVideoList       = S5K2T2_VIDEO_LIST;
    hiddenRearVideoList   = S5K2T2_HIDDEN_VIDEO_LIST;
    rearFPSList       = S5K2T2_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K2T2_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K6B2Base::ExynosSensorS5K6B2Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 1920;
    maxPictureH = 1080;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 1936;
    maxSensorH = 1090;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 220;
    fNumberDen = 100;
    focalLengthNum = 186;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 240;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 54.2f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 42.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 60.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 54.2f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 64.8f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 54.2f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 27;

    minFps = 1;
    maxFps = 30;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL_FRONT;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
#ifndef USE_CAMERA2_API_SUPPORT
        | EFFECT_RED_YELLOW
        | EFFECT_BLUE
        | EFFECT_COLD_VINTAGE
        | EFFECT_BEAUTY_FACE
#endif
        ;

    flashModeList =
          FLASH_MODE_OFF
        /*| FLASH_MODE_AUTO*/
        /*| FLASH_MODE_ON*/
        /*| FLASH_MODE_RED_EYE*/
        /*| FLASH_MODE_TORCH*/
        ;

    focusModeList =
        /* FOCUS_MODE_AUTO*/
        FOCUS_MODE_INFINITY
        /*| FOCUS_MODE_MACRO*/
        | FOCUS_MODE_FIXED
        /*| FOCUS_MODE_EDOF*/
        /*| FOCUS_MODE_CONTINUOUS_VIDEO*/
        /*| FOCUS_MODE_CONTINUOUS_PICTURE*/
        /*| FOCUS_MODE_TOUCH*/
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /* WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /* WHITE_BALANCE_TWILIGHT*/
        /* WHITE_BALANCE_SHADE*/
        ;

    previewSizeLutMax           = 0;
    pictureSizeLutMax           = 0;
    videoSizeLutMax             = 0;
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut              = NULL;
    pictureSizeLut              = NULL;
    videoSizeLut                = NULL;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport            = false;
#if defined(USE_FRONT_PREVIEW_DRC)
    drcSupport                  = true;
#endif

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(S5K6B2_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(S5K6B2_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(S5K6B2_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(S5K6B2_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K6B2_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(S5K6B2_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(S5K6B2_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(S5K6B2_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(S5K6B2_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = S5K6B2_PREVIEW_LIST;
    frontPictureList     = S5K6B2_PICTURE_LIST;
    hiddenFrontPreviewList   = S5K6B2_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = S5K6B2_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K6B2_THUMBNAIL_LIST;
    frontVideoList       = S5K6B2_VIDEO_LIST;
    hiddenFrontVideoList   = S5K6B2_HIDDEN_VIDEO_LIST;
    frontFPSList       = S5K6B2_FPS_RANGE_LIST;
    hiddenFrontFPSList   = S5K6B2_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorSR261Base::ExynosSensorSR261Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 1920;
    maxPictureH = 1080;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 1936;
    maxSensorH = 1090;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 245;
    fNumberDen = 100;
    focalLengthNum = 185;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 27;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 32;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_RED_YELLOW
        | EFFECT_BLUE
        | EFFECT_COLD_VINTAGE
        | EFFECT_AQUA
        ;

    flashModeList =
          FLASH_MODE_OFF
        /*| FLASH_MODE_AUTO*/
        /*| FLASH_MODE_ON*/
        /*| FLASH_MODE_RED_EYE*/
        /*| FLASH_MODE_TORCH*/
        ;

    focusModeList =
        /* FOCUS_MODE_AUTO*/
        FOCUS_MODE_INFINITY
        /*| FOCUS_MODE_MACRO*/
        | FOCUS_MODE_FIXED
        /*| FOCUS_MODE_EDOF*/
        /*| FOCUS_MODE_CONTINUOUS_VIDEO*/
        /*| FOCUS_MODE_CONTINUOUS_PICTURE*/
        /*| FOCUS_MODE_TOUCH*/
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /* WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /* WHITE_BALANCE_TWILIGHT*/
        /* WHITE_BALANCE_SHADE*/
        ;

    previewSizeLutMax           = 0;
    pictureSizeLutMax           = 0;
    videoSizeLutMax             = 0;
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut              = NULL;
    pictureSizeLut              = NULL;
    videoSizeLut                = NULL;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport      = false;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(SR261_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(SR261_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(SR261_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(SR261_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(SR261_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(SR261_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(SR261_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(SR261_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(SR261_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = SR261_PREVIEW_LIST;
    frontPictureList     = SR261_PICTURE_LIST;
    hiddenFrontPreviewList   = SR261_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = SR261_HIDDEN_PICTURE_LIST;
    thumbnailList   = SR261_THUMBNAIL_LIST;
    frontVideoList       = SR261_VIDEO_LIST;
    hiddenFrontVideoList   = SR261_HIDDEN_VIDEO_LIST;
    frontFPSList       = SR261_FPS_RANGE_LIST;
    hiddenFrontFPSList   = SR261_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorSR259Base::ExynosSensorSR259Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1440;
    maxPreviewH = 1080;
    maxPictureW = 1616;
    maxPictureH = 1212;
    maxVideoW = 1280;
    maxVideoH = 720;
    maxSensorW = 1632;
    maxSensorH = 1228;
    sensorMarginW = 16;
    sensorMarginH = 16;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 245;
    fNumberDen = 100;
    focalLengthNum = 185;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 27;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 32;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_RED_YELLOW
        | EFFECT_BLUE
        | EFFECT_COLD_VINTAGE
        | EFFECT_AQUA
        ;

    flashModeList =
          FLASH_MODE_OFF
        /*| FLASH_MODE_AUTO*/
        /*| FLASH_MODE_ON*/
        /*| FLASH_MODE_RED_EYE*/
        /*| FLASH_MODE_TORCH*/
        ;

    focusModeList =
        /* FOCUS_MODE_AUTO*/
        FOCUS_MODE_INFINITY
        /*| FOCUS_MODE_MACRO*/
        | FOCUS_MODE_FIXED
        /*| FOCUS_MODE_EDOF*/
        /*| FOCUS_MODE_CONTINUOUS_VIDEO*/
        /*| FOCUS_MODE_CONTINUOUS_PICTURE*/
        /*| FOCUS_MODE_TOUCH*/
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /* WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /* WHITE_BALANCE_TWILIGHT*/
        /* WHITE_BALANCE_SHADE*/
        ;

    previewSizeLutMax           = 0;
    pictureSizeLutMax           = 0;
    videoSizeLutMax             = 0;
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut              = NULL;
    pictureSizeLut              = NULL;
    videoSizeLut                = NULL;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport      = false;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(SR259_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(SR259_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(SR259_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(SR259_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(SR259_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(SR259_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(SR259_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(SR259_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(SR259_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = SR259_PREVIEW_LIST;
    frontPictureList     = SR259_PICTURE_LIST;
    hiddenFrontPreviewList   = SR259_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = SR259_HIDDEN_PICTURE_LIST;
    thumbnailList   = SR259_THUMBNAIL_LIST;
    frontVideoList       = SR259_VIDEO_LIST;
    hiddenFrontVideoList   = SR259_HIDDEN_VIDEO_LIST;
    frontFPSList       = SR259_FPS_RANGE_LIST;
    hiddenFrontFPSList   = SR259_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K3H7Base::ExynosSensorS5K3H7Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 3248;
    maxPictureH = 2438;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 3264;
    maxSensorH = 2448;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 22;
    fNumberDen = 10;
    focalLengthNum = 420;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        /*| FLASH_MODE_RED_EYE*/
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        /*| FOCUS_MODE_INFINITY*/
        | FOCUS_MODE_MACRO
        /*| FOCUS_MODE_FIXED*/
        /*| FOCUS_MODE_EDOF*/
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /*| WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /*| WHITE_BALANCE_TWILIGHT*/
        /*| WHITE_BALANCE_SHADE*/
        ;

    /* vendor specifics */
    /*
    burstPanoramaW = 3264;
    burstPanoramaH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    */
    bnsSupport = false;

    if (bnsSupport == true) {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeBnsLut             = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        sizeTableSupport      = false;
    } else {
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_3H7) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_3H7)   / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_3H7) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3H7)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3H7)     / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = PREVIEW_SIZE_LUT_3H7;
        videoSizeLut                = VIDEO_SIZE_LUT_3H7;
        videoSizeBnsLut             = NULL;
        pictureSizeLut              = PICTURE_SIZE_LUT_3H7;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3H7;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3H7;
        sizeTableSupport            = true;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K3H7_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K3H7_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K3H7_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K3H7_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K3H7_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K3H7_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K3H7_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K3H7_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K3H7_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K3H7_PREVIEW_LIST;
    rearPictureList     = S5K3H7_PICTURE_LIST;
    hiddenRearPreviewList   = S5K3H7_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K3H7_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K3H7_THUMBNAIL_LIST;
    rearVideoList       = S5K3H7_VIDEO_LIST;
    hiddenRearVideoList   = S5K3H7_HIDDEN_VIDEO_LIST;
    rearFPSList       = S5K3H7_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K3H7_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K3H5Base::ExynosSensorS5K3H5Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 3248;
    maxPictureH = 2438;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 3264;
    maxSensorH = 2448;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 22;
    fNumberDen = 10;
    focalLengthNum = 420;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        /*| FLASH_MODE_RED_EYE*/
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        /*| FOCUS_MODE_INFINITY*/
        | FOCUS_MODE_MACRO
        /*| FOCUS_MODE_FIXED*/
        /*| FOCUS_MODE_EDOF*/
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /*| WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /*| WHITE_BALANCE_TWILIGHT*/
        /*| WHITE_BALANCE_SHADE*/
        ;

    previewSizeLutMax     = 0;
    pictureSizeLutMax     = 0;
    videoSizeLutMax       = 0;
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut        = NULL;
    pictureSizeLut        = NULL;
    videoSizeLut          = NULL;
    videoSizeBnsLut       = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport      = false;

    /* vendor specifics */
    /*
    burstPanoramaW = 3264;
    burstPanoramaH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    */
    bnsSupport = false;
    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K3H5_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K3H5_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K3H5_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K3H5_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K3H5_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K3H5_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K3H5_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K3H5_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K3H5_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K3H5_PREVIEW_LIST;
    rearPictureList     = S5K3H5_PICTURE_LIST;
    hiddenRearPreviewList   = S5K3H5_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K3H5_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K3H5_THUMBNAIL_LIST;
    rearVideoList       = S5K3H5_VIDEO_LIST;
    hiddenRearVideoList   = S5K3H5_HIDDEN_VIDEO_LIST;
    rearFPSList       = S5K3H5_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K3H5_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K4H5Base::ExynosSensorS5K4H5Base() : ExynosSensorInfoBase()
{
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080)
    maxPreviewW = 3264;
    maxPreviewH = 2448;
#else
    maxPreviewW = 2560;
    maxPreviewH = 1440;
#endif
    maxPictureW = 3264;
    maxPictureH = 2448;
    maxVideoW = 2560;
    maxVideoH = 1440;
    maxSensorW = 3280;
    maxSensorH = 2458;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 290;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 59.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 59.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 46.1f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 48.8f;
    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
#ifndef USE_CAMERA2_API_SUPPORT
        | EFFECT_RED_YELLOW
        | EFFECT_BLUE
        | EFFECT_COLD_VINTAGE
#endif
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        /*| FLASH_MODE_RED_EYE*/
        | FLASH_MODE_TORCH
        ;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        /*| FOCUS_MODE_FIXED*/
        /*| FOCUS_MODE_EDOF*/
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /*| WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /*| WHITE_BALANCE_TWILIGHT*/
        /*| WHITE_BALANCE_SHADE*/
        ;

    /* vendor specifics */
    /*
    burstPanoramaW = 3264;
    burstPanoramaH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    */
    bnsSupport = false;

    if (bnsSupport == true) {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeBnsLut             = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        sizeTableSupport            = false;
    } else {
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_4H5) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_4H5)   / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_4H5) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_4H5)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_4H5)     / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = PREVIEW_SIZE_LUT_4H5;
        videoSizeLut                = VIDEO_SIZE_LUT_4H5;
        videoSizeBnsLut             = NULL;
        pictureSizeLut              = PICTURE_SIZE_LUT_4H5;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_4H5;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_4H5;
        sizeTableSupport            = true;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K4H5_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K4H5_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K4H5_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K4H5_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K4H5_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K4H5_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K4H5_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K4H5_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K4H5_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K4H5_PREVIEW_LIST;
    rearPictureList     = S5K4H5_PICTURE_LIST;
    hiddenRearPreviewList   = S5K4H5_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K4H5_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K4H5_THUMBNAIL_LIST;
    rearVideoList       = S5K4H5_VIDEO_LIST;
    hiddenRearVideoList   = S5K4H5_HIDDEN_VIDEO_LIST;
    rearFPSList       = S5K4H5_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K4H5_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K4H5YCBase::ExynosSensorS5K4H5YCBase() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 3264;
    maxPictureH = 2448;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 3280;
    maxSensorH = 2458;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 24;
    fNumberDen = 10;
    focalLengthNum = 330;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 56.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 43.4f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 33.6f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
#ifndef USE_CAMERA2_API_SUPPORT
        | EFFECT_RED_YELLOW
        | EFFECT_BLUE
        | EFFECT_COLD_VINTAGE
#endif
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        /*| FLASH_MODE_RED_EYE*/
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        /*| FOCUS_MODE_FIXED*/
        /*| FOCUS_MODE_EDOF*/
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /*| WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /*| WHITE_BALANCE_TWILIGHT*/
        /*| WHITE_BALANCE_SHADE*/
        ;

    /* vendor specifics */
    /*
    burstPanoramaW = 3264;
    burstPanoramaH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    */
    bnsSupport = false;

    if (bnsSupport == true) {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeBnsLut             = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        sizeTableSupport            = false;
    } else {
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_4H5) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_4H5)   / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_4H5) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_4H5) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_4H5) / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = PREVIEW_SIZE_LUT_4H5;
        videoSizeLut                = VIDEO_SIZE_LUT_4H5;
        videoSizeBnsLut             = NULL;
        pictureSizeLut              = PICTURE_SIZE_LUT_4H5;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_4H5;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_4H5;
        sizeTableSupport            = true;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K4H5_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K4H5_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K4H5_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K4H5_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K4H5_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K4H5_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K4H5_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K4H5_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K4H5_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K4H5_PREVIEW_LIST;
    rearPictureList     = S5K4H5_PICTURE_LIST;
    hiddenRearPreviewList   = S5K4H5_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K4H5_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K4H5_THUMBNAIL_LIST;
    rearVideoList       = S5K4H5_VIDEO_LIST;
    hiddenRearVideoList   = S5K4H5_HIDDEN_VIDEO_LIST;
    rearFPSList       = S5K4H5_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K4H5_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K3M2Base::ExynosSensorS5K3M2Base()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 4128;
    maxPictureH = 3096;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 4144;
    maxSensorH = 3106;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 22;
    fNumberDen = 10;
    focalLengthNum = 420;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        /*| EFFECT_RED_YELLOW*/
        /*| EFFECT_BLUE*/
        /*| EFFECT_COLD_VINTAGE*/
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        /*| FLASH_MODE_RED_EYE*/
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        /*| FOCUS_MODE_INFINITY*/
        | FOCUS_MODE_MACRO
        /*| FOCUS_MODE_FIXED*/
        /*| FOCUS_MODE_EDOF*/
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /*| WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /*| WHITE_BALANCE_TWILIGHT*/
        /*| WHITE_BALANCE_SHADE*/
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 2056;
    highSpeedRecording60H = 1152;
    highSpeedRecording120W = 1020;
    highSpeedRecording120H = 574;
    scalableSensorSupport = true;
    bnsSupport = false;

    if (bnsSupport == true) {
        previewSizeLutMax = sizeof(PREVIEW_SIZE_LUT_3M2_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax   = sizeof(VIDEO_SIZE_LUT_3M2)   / (sizeof(int) * SIZE_OF_LUT);
        previewSizeLut    = PREVIEW_SIZE_LUT_3M2_BNS;
        videoSizeLut      = VIDEO_SIZE_LUT_3M2;
        videoSizeBnsLut   = VIDEO_SIZE_LUT_3M2_BNS;
    } else {
        previewSizeLutMax = sizeof(PREVIEW_SIZE_LUT_3M2) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax   = sizeof(VIDEO_SIZE_LUT_3M2)   / (sizeof(int) * SIZE_OF_LUT);
        previewSizeLut    = PREVIEW_SIZE_LUT_3M2;
        videoSizeLut      = VIDEO_SIZE_LUT_3M2;
        videoSizeBnsLut   = NULL;
    }
    pictureSizeLutMax     = sizeof(PICTURE_SIZE_LUT_3M2) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLut        = PICTURE_SIZE_LUT_3M2;
    videoSizeLutHighSpeed60 = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3M2;
    videoSizeLutHighSpeed120 = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3M2;
    sizeTableSupport = true;

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K3M2_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K3M2_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K3M2_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K3M2_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K3M2_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K3M2_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K3M2_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K3M2_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K3M2_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K3M2_PREVIEW_LIST;
    rearPictureList     = S5K3M2_PICTURE_LIST;
    hiddenRearPreviewList   = S5K3M2_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K3M2_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K3M2_THUMBNAIL_LIST;
    rearVideoList       = S5K3M2_VIDEO_LIST;
    hiddenRearVideoList   = S5K3M2_HIDDEN_VIDEO_LIST;
    rearFPSList       = S5K3M2_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K3M2_HIDDEN_FPS_RANGE_LIST;
}

ExynosSensorS5K3M3Base::ExynosSensorS5K3M3Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 4128;
    maxPictureH = 3096;
    maxVideoW = 3840;
    maxVideoH = 2160;
    maxSensorW = 4208;
    maxSensorH = 3120;
    sensorMarginW = 0;
    sensorMarginH = 0;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 20;
    fNumberDen = 10;
    focalLengthNum = 365;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 185;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 53.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 41.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 76.5f;
    focalLengthIn35mmLength = 27;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    minExposureTime = 32;
    maxExposureTime = 10000000;
    minWBK = 2300;
    maxWBK = 10000;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        | EFFECT_BEAUTY_FACE
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        //| FLASH_MODE_RED_EYE
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        //| FOCUS_MODE_FIXED
        //| FOCUS_MODE_EDOF
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_PARTY
        | SCENE_MODE_SPORTS
        | SCENE_MODE_CANDLELIGHT
        */
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        //| WHITE_BALANCE_WARM_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| WHITE_BALANCE_TWILIGHT
        //| WHITE_BALANCE_SHADE
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = true;

    if (bnsSupport == true) {

#if defined(USE_BNS_PREVIEW)
        previewSizeLutMax     = sizeof(PREVIEW_SIZE_LUT_3M3_BNS_15) / (sizeof(int) * SIZE_OF_LUT);
#else
        previewSizeLutMax     = sizeof(PREVIEW_SIZE_LUT_3M3_BDS)    / (sizeof(int) * SIZE_OF_LUT);
#endif
        videoSizeLutMax       = sizeof(VIDEO_SIZE_LUT_3M3_BDS)      / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax     = sizeof(PICTURE_SIZE_LUT_3M3)        / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax      = sizeof(VTCALL_SIZE_LUT_3M3_BNS)     / (sizeof(int) * SIZE_OF_LUT);

        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_HIGH_SPEED_3M3_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_HIGH_SPEED_3M3_BNS) / (sizeof(int) * SIZE_OF_LUT);

#if defined(USE_BNS_PREVIEW)
        previewSizeLut        = PREVIEW_SIZE_LUT_3M3_BNS_15;
#else
        previewSizeLut        = PREVIEW_SIZE_LUT_3M3_BDS;
#endif
        videoSizeLut          = VIDEO_SIZE_LUT_3M3_BDS;
        videoSizeBnsLut       = VIDEO_SIZE_LUT_3M3_BNS_15;
        pictureSizeLut        = PICTURE_SIZE_LUT_3M3;

#if defined(USE_BNS_DUAL_PREVIEW)
#if defined(DUAL_BNS_RATIO) && (DUAL_BNS_RATIO == 1500)
        dualPreviewSizeLut    = PREVIEW_SIZE_LUT_3M3_BNS_15;
#else
        dualPreviewSizeLut    = PREVIEW_SIZE_LUT_3M3_BNS_20;
#endif
#else
        dualPreviewSizeLut    = PREVIEW_SIZE_LUT_3M3_BDS;
#endif // USE_BNS_DUAL_PREVIEW

#if defined(USE_BNS_DUAL_RECORDING)
#if defined(DUAL_BNS_RATIO) && (DUAL_BNS_RATIO == 1500)
        dualVideoSizeLut      = VIDEO_SIZE_LUT_3M3_BNS_15;
#else
        dualVideoSizeLut      = VIDEO_SIZE_LUT_3M3_BNS_20;
#endif
#else
        dualVideoSizeLut      = VIDEO_SIZE_LUT_3M3_BDS;
#endif //  USE_BNS_DUAL_RECORDING

        videoSizeLutHighSpeed60  = VIDEO_SIZE_LUT_HIGH_SPEED_3M3_BNS;
        videoSizeLutHighSpeed120 = VIDEO_SIZE_LUT_HIGH_SPEED_3M3_BNS;
        vtcallSizeLut         = VTCALL_SIZE_LUT_3M3_BNS;
        sizeTableSupport      = true;
    } else {
        previewSizeLutMax     = 0;
        pictureSizeLutMax     = 0;
        videoSizeLutMax       = 0;
        vtcallSizeLutMax      = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut        = NULL;
        pictureSizeLut        = NULL;
        videoSizeLut          = NULL;
        videoSizeBnsLut       = NULL;
        vtcallSizeLut         = NULL;
        videoSizeLutHighSpeed60  = NULL;
        videoSizeLutHighSpeed120 = NULL;

        sizeTableSupport      = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K3M3_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K3M3_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K3M3_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K3M3_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K3M3_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K3M3_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K3M3_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K3M3_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K3M3_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K3M3_PREVIEW_LIST;
    rearPictureList     = S5K3M3_PICTURE_LIST;
    hiddenRearPreviewList   = S5K3M3_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K3M3_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K3M3_THUMBNAIL_LIST;
    rearVideoList       = S5K3M3_VIDEO_LIST;
    hiddenRearVideoList   = S5K3M3_HIDDEN_VIDEO_LIST;
    rearFPSList       = S5K3M3_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K3M3_HIDDEN_FPS_RANGE_LIST;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(S5K3M3_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(S5K3M3_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(S5K3M3_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(S5K3M3_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K3M3_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(S5K3M3_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(S5K3M3_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(S5K3M3_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(S5K3M3_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = S5K3M3_PREVIEW_LIST;
    frontPictureList     = S5K3M3_PICTURE_LIST;
    hiddenFrontPreviewList   = S5K3M3_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = S5K3M3_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K3M3_THUMBNAIL_LIST;
    frontVideoList       = S5K3M3_VIDEO_LIST;
    hiddenFrontVideoList   = S5K3M3_HIDDEN_VIDEO_LIST;
    frontFPSList       = S5K3M3_FPS_RANGE_LIST;
    hiddenFrontFPSList   = S5K3M3_HIDDEN_FPS_RANGE_LIST;

#ifdef BOARD_CAMERA_USES_DUAL_CAMERA
    dof = new DOF_3M3;
#endif
};

ExynosSensorS5K3M3DualBdsBase::ExynosSensorS5K3M3DualBdsBase() : ExynosSensorS5K3M3Base()
{
    /* this sensor is for BDS when Dual */
    if (bnsSupport == true) {
        previewSizeLutMax     = sizeof(PREVIEW_SIZE_LUT_3M3_BDS)    / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax       = sizeof(VIDEO_SIZE_LUT_3M3_BDS)      / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax     = sizeof(PICTURE_SIZE_LUT_3M3)        / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax      = sizeof(VTCALL_SIZE_LUT_3M3_BNS)     / (sizeof(int) * SIZE_OF_LUT);

        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_HIGH_SPEED_3M3_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_HIGH_SPEED_3M3_BNS) / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut        = PREVIEW_SIZE_LUT_3M3_BDS;
        videoSizeLut          = VIDEO_SIZE_LUT_3M3_BDS;
        videoSizeBnsLut       = VIDEO_SIZE_LUT_3M3_BNS_15;
        pictureSizeLut        = PICTURE_SIZE_LUT_3M3;

        dualPreviewSizeLut    = PREVIEW_SIZE_LUT_3M3_BDS;
        dualVideoSizeLut      = VIDEO_SIZE_LUT_3M3_BDS;

        videoSizeLutHighSpeed60  = VIDEO_SIZE_LUT_HIGH_SPEED_3M3_BNS;
        videoSizeLutHighSpeed120 = VIDEO_SIZE_LUT_HIGH_SPEED_3M3_BNS;
        vtcallSizeLut         = VTCALL_SIZE_LUT_3M3_BNS;

        sizeTableSupport      = true;
    } else {
        previewSizeLutMax     = 0;
        pictureSizeLutMax     = 0;
        videoSizeLutMax       = 0;
        vtcallSizeLutMax      = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut        = NULL;
        pictureSizeLut        = NULL;
        videoSizeLut          = NULL;
        videoSizeBnsLut       = NULL;
        vtcallSizeLut         = NULL;
        videoSizeLutHighSpeed60  = NULL;
        videoSizeLutHighSpeed120 = NULL;

        sizeTableSupport      = false;
    }
};

ExynosSensorS5K5E2Base::ExynosSensorS5K5E2Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 2560;
    maxPictureH = 1920;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 2576;
    maxSensorH = 1932;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 245;
    fNumberDen = 100;
    focalLengthNum = 185;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 69.8f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 42.8f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 27;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
#ifndef USE_CAMERA2_API_SUPPORT
        | EFFECT_RED_YELLOW
        | EFFECT_BLUE
        | EFFECT_COLD_VINTAGE
#endif
        ;

    flashModeList =
          FLASH_MODE_OFF
        /*| FLASH_MODE_AUTO*/
        /*| FLASH_MODE_ON*/
        /*| FLASH_MODE_RED_EYE*/
        /*| FLASH_MODE_TORCH*/
        ;

    focusModeList =
        /* FOCUS_MODE_AUTO*/
        FOCUS_MODE_INFINITY
        /*| FOCUS_MODE_MACRO*/
        | FOCUS_MODE_FIXED
        /*| FOCUS_MODE_EDOF*/
        /*| FOCUS_MODE_CONTINUOUS_VIDEO*/
        /*| FOCUS_MODE_CONTINUOUS_PICTURE*/
        /*| FOCUS_MODE_TOUCH*/
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT*/;


    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /* WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /* WHITE_BALANCE_TWILIGHT*/
        /* WHITE_BALANCE_SHADE*/
        ;

    previewSizeLutMax           = 0;
    pictureSizeLutMax           = 0;
    videoSizeLutMax             = 0;
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;
    previewSizeLut              = NULL;
    pictureSizeLut              = NULL;
    videoSizeLut                = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport            = false;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;

    previewSizeLutMax     = sizeof(PREVIEW_SIZE_LUT_5E2_YC) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax     = sizeof(PICTURE_SIZE_LUT_5E2_YC) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax       = sizeof(VIDEO_SIZE_LUT_5E2_YC) / (sizeof(int) * SIZE_OF_LUT);
    previewSizeLut        = PREVIEW_SIZE_LUT_5E2_YC;
    pictureSizeLut        = PICTURE_SIZE_LUT_5E2_YC;
    videoSizeLut          = VIDEO_SIZE_LUT_5E2_YC;
    dualVideoSizeLut      = VIDEO_SIZE_LUT_5E2_YC;
    videoSizeBnsLut       = NULL;
    sizeTableSupport      = true;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(S5K5E2_YC_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(S5K5E2_YC_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(S5K5E2_YC_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(S5K5E2_YC_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K5E2_YC_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(S5K5E2_YC_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(S5K5E2_YC_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(S5K5E2_YC_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(S5K5E2_YC_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = S5K5E2_YC_PREVIEW_LIST;
    frontPictureList     = S5K5E2_YC_PICTURE_LIST;
    hiddenFrontPreviewList   = S5K5E2_YC_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = S5K5E2_YC_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K5E2_YC_THUMBNAIL_LIST;
    frontVideoList       = S5K5E2_YC_VIDEO_LIST;
    hiddenFrontVideoList   = S5K5E2_YC_HIDDEN_VIDEO_LIST;
    frontFPSList       = S5K5E2_YC_FPS_RANGE_LIST;
    hiddenFrontFPSList   = S5K5E2_YC_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K5E8Base::ExynosSensorS5K5E8Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 400;
    maxPreviewH = 400;
    maxPictureW = 2576;
    maxPictureH = 1934;
    maxVideoW = 2576;
    maxVideoH = 1934;
    maxSensorW = 2576;
    maxSensorH = 1932;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 245;
    fNumberDen = 100;
    focalLengthNum = 185;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 69.8f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 42.8f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 27;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
#ifndef USE_CAMERA2_API_SUPPORT
        | EFFECT_RED_YELLOW
        | EFFECT_BLUE
        | EFFECT_COLD_VINTAGE
#endif
        ;

    flashModeList =
          FLASH_MODE_OFF
        /*| FLASH_MODE_AUTO*/
        /*| FLASH_MODE_ON*/
        /*| FLASH_MODE_RED_EYE*/
        /*| FLASH_MODE_TORCH*/
        ;

    focusModeList =
        /* FOCUS_MODE_AUTO*/
        FOCUS_MODE_INFINITY
        /*| FOCUS_MODE_MACRO*/
        | FOCUS_MODE_FIXED
        /*| FOCUS_MODE_EDOF*/
        /*| FOCUS_MODE_CONTINUOUS_VIDEO*/
        /*| FOCUS_MODE_CONTINUOUS_PICTURE*/
        /*| FOCUS_MODE_TOUCH*/
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT*/;


    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /* WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /* WHITE_BALANCE_TWILIGHT*/
        /* WHITE_BALANCE_SHADE*/
        ;

    previewSizeLutMax           = 0;
    pictureSizeLutMax           = 0;
    videoSizeLutMax             = 0;
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;
    previewSizeLut              = NULL;
    pictureSizeLut              = NULL;
    videoSizeLut                = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport            = false;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;

    previewSizeLutMax     = sizeof(PREVIEW_SIZE_LUT_5E8) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax     = sizeof(PICTURE_SIZE_LUT_5E8) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax       = sizeof(VIDEO_SIZE_LUT_5E8) / (sizeof(int) * SIZE_OF_LUT);
    previewSizeLut        = PREVIEW_SIZE_LUT_5E8;
    pictureSizeLut        = PICTURE_SIZE_LUT_5E8;
    videoSizeLut          = VIDEO_SIZE_LUT_5E8;
    dualVideoSizeLut      = VIDEO_SIZE_LUT_5E8;
    videoSizeBnsLut       = NULL;
    sizeTableSupport      = true;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(S5K5E8_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(S5K5E8_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(S5K5E8_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(S5K5E8_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K5E8_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(S5K5E8_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(S5K5E8_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(S5K5E8_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(S5K5E8_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = S5K5E8_PREVIEW_LIST;
    frontPictureList     = S5K5E8_PICTURE_LIST;
    hiddenFrontPreviewList   = S5K5E8_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = S5K5E8_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K5E8_THUMBNAIL_LIST;
    frontVideoList       = S5K5E8_VIDEO_LIST;
    hiddenFrontVideoList   = S5K5E8_HIDDEN_VIDEO_LIST;
    frontFPSList       = S5K5E8_FPS_RANGE_LIST;
    hiddenFrontFPSList   = S5K5E8_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K6A3Base::ExynosSensorS5K6A3Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1280;
    maxPreviewH = 720;
    maxPictureW = 1392;
    maxPictureH = 1402;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 1408;
    maxSensorH = 1412;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 22;
    fNumberDen = 10;
    focalLengthNum = 420;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        ;

    flashModeList =
          FLASH_MODE_OFF;
        /*| FLASH_MODE_AUTO*/
        /*| FLASH_MODE_ON*/
        /*| FLASH_MODE_RED_EYE*/
        /*| FLASH_MODE_TORCH;*/

    focusModeList =
        /*FOCUS_MODE_AUTO*/
        FOCUS_MODE_INFINITY;
        /*| FOCUS_MODE_INFINITY*/
        /*| FOCUS_MODE_MACRO*/
        /*| FOCUS_MODE_FIXED*/
        /*| FOCUS_MODE_EDOF*/
        /*| FOCUS_MODE_CONTINUOUS_VIDEO*/
        /*| FOCUS_MODE_CONTINUOUS_PICTURE*/
        /*| FOCUS_MODE_TOUCH*/
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /*| WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /*| WHITE_BALANCE_TWILIGHT*/
        /*| WHITE_BALANCE_SHADE*/
        ;

    previewSizeLutMax           = 0;
    pictureSizeLutMax           = 0;
    videoSizeLutMax             = 0;
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut              = NULL;
    pictureSizeLut              = NULL;
    videoSizeLut                = NULL;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport            = false;

    /* vendor specifics */
    /*
    burstPanoramaW = 3264;
    burstPanoramaH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    */
    bnsSupport = false;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(S5K6A3_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(S5K6A3_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(S5K6A3_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(S5K6A3_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K6A3_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(S5K6A3_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(S5K6A3_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(S5K6A3_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(S5K6A3_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = S5K6A3_PREVIEW_LIST;
    frontPictureList     = S5K6A3_PICTURE_LIST;
    hiddenFrontPreviewList   = S5K6A3_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = S5K6A3_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K6A3_THUMBNAIL_LIST;
    frontVideoList       = S5K6A3_VIDEO_LIST;
    hiddenFrontVideoList   = S5K6A3_HIDDEN_VIDEO_LIST;
    frontFPSList       = S5K6A3_FPS_RANGE_LIST;
    hiddenFrontFPSList   = S5K6A3_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorIMX175Base::ExynosSensorIMX175Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 3264;
    maxPictureH = 2448;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 3280;
    maxSensorH = 2458;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 26;
    fNumberDen = 10;
    focalLengthNum = 370;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 276;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
        ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
        EFFECT_NONE
        ;

    hiddenEffectList =
        EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        ;

    flashModeList =
        FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        /*| FLASH_MODE_RED_EYE*/
        | FLASH_MODE_TORCH;

    focusModeList =
        FOCUS_MODE_AUTO
        /*| FOCUS_MODE_INFINITY*/
        | FOCUS_MODE_MACRO
        /*| FOCUS_MODE_FIXED*/
        /*| FOCUS_MODE_EDOF*/
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH;

    sceneModeList =
        SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
        WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /*| WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /*| WHITE_BALANCE_TWILIGHT*/
        /*| WHITE_BALANCE_SHADE*/
        ;

    /* vendor specifics */
    /*
       burstPanoramaW = 3264;
       burstPanoramaH = 1836;
       highSpeedRecording60WFHD = 1920;
       highSpeedRecording60HFHD = 1080;
       highSpeedRecording60W = 1008;
       highSpeedRecording60H = 566;
       highSpeedRecording120W = 1008;
       highSpeedRecording120H = 566;
       scalableSensorSupport = true;
     */
    bnsSupport = false;

    if (bnsSupport == true) {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeBnsLut             = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        sizeTableSupport            = false;
    } else {
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_IMX175) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX175)   / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_IMX175) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX175)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX175)     / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = PREVIEW_SIZE_LUT_IMX175;
        videoSizeLut                = VIDEO_SIZE_LUT_IMX175;
        videoSizeBnsLut             = NULL;
        pictureSizeLut              = PICTURE_SIZE_LUT_IMX175;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX175;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX175;
        sizeTableSupport            = true;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(IMX175_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(IMX175_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(IMX175_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(IMX175_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(IMX175_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(IMX175_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(IMX175_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(IMX175_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(IMX175_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = IMX175_PREVIEW_LIST;
    rearPictureList     = IMX175_PICTURE_LIST;
    hiddenRearPreviewList   = IMX175_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = IMX175_HIDDEN_PICTURE_LIST;
    thumbnailList   = IMX175_THUMBNAIL_LIST;
    rearVideoList       = IMX175_VIDEO_LIST;
    hiddenRearVideoList   = IMX175_HIDDEN_VIDEO_LIST;
    rearFPSList       = IMX175_FPS_RANGE_LIST;
    hiddenRearFPSList   = IMX175_HIDDEN_FPS_RANGE_LIST;
};

#if 0
ExynosSensorIMX240Base::ExynosSensorIMX240Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 3840;
    maxPreviewH = 2160;
    maxPictureW = 5312;
    maxPictureH = 2988;
    maxVideoW = 3840;
    maxVideoH = 2160;
    maxSensorW = 5328;
    maxSensorH = 3000;
    sensorMarginW = 16;
    sensorMarginH = 12;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 430;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 185;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 53.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 41.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 41.0f;
    focalLengthIn35mmLength = 28;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        | EFFECT_BEAUTY_FACE
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        //| FLASH_MODE_RED_EYE
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        //| FOCUS_MODE_FIXED
        //| FOCUS_MODE_EDOF
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_PARTY
        | SCENE_MODE_SPORTS
        | SCENE_MODE_CANDLELIGHT
        */
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        //| WHITE_BALANCE_WARM_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| WHITE_BALANCE_TWILIGHT
        //| WHITE_BALANCE_SHADE
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = true;

    if (bnsSupport == true) {

#if defined(USE_BNS_PREVIEW)
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_IMX240_BNS) / (sizeof(int) * SIZE_OF_LUT);
#else
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_IMX240) / (sizeof(int) * SIZE_OF_LUT);
#endif

#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX240_8MP_FULL)       / (sizeof(int) * SIZE_OF_LUT);
#else
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX240)       / (sizeof(int) * SIZE_OF_LUT);
#endif
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_IMX240)     / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_IMX240_BNS)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX240_BNS)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX240_BNS)     / (sizeof(int) * SIZE_OF_LUT);

#if defined(USE_BNS_PREVIEW)
        previewSizeLut              = PREVIEW_SIZE_LUT_IMX240_BNS;
#else
        previewSizeLut              = PREVIEW_SIZE_LUT_IMX240;
#endif
        dualPreviewSizeLut          = PREVIEW_SIZE_LUT_IMX240_BNS;

#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLut                = VIDEO_SIZE_LUT_IMX240_8MP_FULL;
#else
        videoSizeLut                = VIDEO_SIZE_LUT_IMX240;
#endif
        videoSizeBnsLut             = VIDEO_SIZE_LUT_IMX240_BNS;
        pictureSizeLut              = PICTURE_SIZE_LUT_IMX240;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX240_BNS;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX240_BNS;
        vtcallSizeLut               = VTCALL_SIZE_LUT_IMX240_BNS;
        sizeTableSupport            = true;
    } else {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        vtcallSizeLutMax            = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeBnsLut             = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        vtcallSizeLut               = NULL;
        sizeTableSupport            = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(IMX240_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(IMX240_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(IMX240_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(IMX240_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(IMX240_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(IMX240_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(IMX240_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(IMX240_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(IMX240_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = IMX240_PREVIEW_LIST;
    rearPictureList     = IMX240_PICTURE_LIST;
    hiddenRearPreviewList   = IMX240_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = IMX240_HIDDEN_PICTURE_LIST;
    thumbnailList   = IMX240_THUMBNAIL_LIST;
    rearVideoList       = IMX240_VIDEO_LIST;
    hiddenRearVideoList   = IMX240_HIDDEN_VIDEO_LIST;
    rearFPSList       = IMX240_FPS_RANGE_LIST;
    hiddenRearFPSList   = IMX240_HIDDEN_FPS_RANGE_LIST;
};
#endif

ExynosSensorIMX228Base::ExynosSensorIMX228Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 3840;
    maxPreviewH = 2160;
    maxPictureW = 5952;
    maxPictureH = 3348;
    maxVideoW = 3840;
    maxVideoH = 2160;
    maxSensorW = 5968;
    maxSensorH = 3368;
    sensorMarginW = 16;
    sensorMarginH = 12;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 430;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 68.13f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 41.0f;
    focalLengthIn35mmLength = 28;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        | EFFECT_BEAUTY_FACE
        ;

    flashModeList =
        FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        //| FLASH_MODE_RED_EYE
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        //| FOCUS_MODE_FIXED
        //| FOCUS_MODE_EDOF
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_PARTY
        | SCENE_MODE_SPORTS
        | SCENE_MODE_CANDLELIGHT
        */
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        //| WHITE_BALANCE_WARM_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| WHITE_BALANCE_TWILIGHT
        //| WHITE_BALANCE_SHADE
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = true;

    if (bnsSupport == true) {
#if defined(USE_BNS_PREVIEW)
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_IMX228_BNS) / (sizeof(int) * SIZE_OF_LUT);
#else
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_IMX228) / (sizeof(int) * SIZE_OF_LUT);
#endif
#ifdef ENABLE_13MP_FULL_FRAME
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX228_13MP_FULL)       / (sizeof(int) * SIZE_OF_LUT);
#elif defined(ENABLE_8MP_FULL_FRAME)
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX228_8MP_FULL)       / (sizeof(int) * SIZE_OF_LUT);
#else
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX228)       / (sizeof(int) * SIZE_OF_LUT);
#endif
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_IMX228)     / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_IMX228_BNS)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX228_BNS)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX228_BNS)     / (sizeof(int) * SIZE_OF_LUT);

#if defined(USE_BNS_PREVIEW)
        previewSizeLut              = PREVIEW_SIZE_LUT_IMX228_BNS;
#else
        previewSizeLut              = PREVIEW_SIZE_LUT_IMX228;
#endif
        dualPreviewSizeLut          = PREVIEW_SIZE_LUT_IMX228_BNS;
#ifdef ENABLE_13MP_FULL_FRAME
        videoSizeLut                = VIDEO_SIZE_LUT_IMX228_13MP_FULL;
#elif defined(ENABLE_8MP_FULL_FRAME)
        videoSizeLut                = VIDEO_SIZE_LUT_IMX228_8MP_FULL;
#else
        videoSizeLut                = VIDEO_SIZE_LUT_IMX228;
#endif
        videoSizeBnsLut             = VIDEO_SIZE_LUT_IMX228_BNS;
        pictureSizeLut              = PICTURE_SIZE_LUT_IMX228;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX228_BNS;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX228_BNS;
        vtcallSizeLut         = VTCALL_SIZE_LUT_IMX228_BNS;
        sizeTableSupport      = true;
    } else {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        vtcallSizeLutMax            = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeBnsLut             = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        vtcallSizeLut               = NULL;
        sizeTableSupport            = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(IMX228_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(IMX228_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(IMX228_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(IMX228_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(IMX228_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(IMX228_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(IMX228_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(IMX228_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(IMX228_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = IMX228_PREVIEW_LIST;
    rearPictureList     = IMX228_PICTURE_LIST;
    hiddenRearPreviewList   = IMX228_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = IMX228_HIDDEN_PICTURE_LIST;
    thumbnailList   = IMX228_THUMBNAIL_LIST;
    rearVideoList       = IMX228_VIDEO_LIST;
    hiddenRearVideoList   = IMX228_HIDDEN_VIDEO_LIST;
    rearFPSList       = IMX228_FPS_RANGE_LIST;
    hiddenRearFPSList   = IMX228_HIDDEN_FPS_RANGE_LIST;
};


ExynosSensorIMX219Base::ExynosSensorIMX219Base() : ExynosSensorInfoBase()
{
#if defined(CAMERA_LCD_SIZE) && (CAMERA_LCD_SIZE >= LCD_SIZE_1920_1080)
    maxPreviewW = 3264;
    maxPreviewH = 2448;
#else
    maxPreviewW = 1920;
    maxPreviewH = 1080;
#endif
    maxPictureW = 3264;
    maxPictureH = 2448;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 3280;
    maxSensorH = 2458;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 160;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 1900;
    apertureDen = 1000;

    horizontalViewAngle[SIZE_RATIO_16_9] = 56.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 44.3f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 34.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 48.1f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 44.3f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 52.8f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 44.3f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 31;

    minFps = 20;
    maxFps = 24;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL_FRONT;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;

    zoomSupport = false;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = true;
    drcSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        | EFFECT_BEAUTY_FACE
        ;

    flashModeList =
          FLASH_MODE_OFF
        /*| FLASH_MODE_AUTO*/
        /*| FLASH_MODE_ON*/
        /*| FLASH_MODE_RED_EYE*/
        /*| FLASH_MODE_TORCH*/
        ;

    focusModeList =
          FOCUS_MODE_AUTO
        /*| FOCUS_MODE_INFINITY*/
        | FOCUS_MODE_MACRO
        /*| FOCUS_MODE_FIXED*/
        /*| FOCUS_MODE_EDOF*/
        /*| FOCUS_MODE_CONTINUOUS_VIDEO*/
        /*| FOCUS_MODE_CONTINUOUS_PICTURE*/
        /*| FOCUS_MODE_TOUCH*/
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /*| WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /*| WHITE_BALANCE_TWILIGHT*/
        /*| WHITE_BALANCE_SHADE*/
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_IMX219) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_IMX219) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX219) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut              = PREVIEW_SIZE_LUT_IMX219;
    pictureSizeLut              = PICTURE_SIZE_LUT_IMX219;
    videoSizeLut                = VIDEO_SIZE_LUT_IMX219;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport      = true;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(IMX219_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(IMX219_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(IMX219_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(IMX219_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(IMX219_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(IMX219_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(IMX219_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(IMX219_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(IMX219_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = IMX219_PREVIEW_LIST;
    frontPictureList     = IMX219_PICTURE_LIST;
    hiddenFrontPreviewList   = IMX219_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = IMX219_HIDDEN_PICTURE_LIST;
    thumbnailList   = IMX219_THUMBNAIL_LIST;
    frontVideoList       = IMX219_VIDEO_LIST;
    hiddenFrontVideoList   = IMX219_HIDDEN_VIDEO_LIST;
    frontFPSList       = IMX219_FPS_RANGE_LIST;
    hiddenFrontFPSList   = IMX219_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K8B1Base::ExynosSensorS5K8B1Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 1920;
    maxPictureH = 1080;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 1936;
    maxSensorH = 1090;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 24;
    fNumberDen = 10;
    focalLengthNum = 120;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 2400;
    apertureDen = 1000;

    horizontalViewAngle[SIZE_RATIO_16_9] = 79.8f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.2f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 50.8f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 71.8f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 65.2f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 74.8f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 65.2f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 22;

    minFps = 1;
    maxFps = 30;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        ;

    flashModeList =
          FLASH_MODE_OFF
        /*| FLASH_MODE_AUTO*/
        /*| FLASH_MODE_ON*/
        /*| FLASH_MODE_RED_EYE*/
        /*| FLASH_MODE_TORCH*/
        ;

    focusModeList =
        /* FOCUS_MODE_AUTO*/
          FOCUS_MODE_FIXED
        /*| FOCUS_MODE_MACRO*/
          | FOCUS_MODE_INFINITY
        /*| FOCUS_MODE_EDOF*/
        /*| FOCUS_MODE_CONTINUOUS_VIDEO*/
        /*| FOCUS_MODE_CONTINUOUS_PICTURE*/
        /*| FOCUS_MODE_TOUCH*/
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /* WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /* WHITE_BALANCE_TWILIGHT*/
        /* WHITE_BALANCE_SHADE*/
        ;

    previewSizeLutMax           = 0;
    pictureSizeLutMax           = 0;
    videoSizeLutMax             = 0;
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut              = NULL;
    pictureSizeLut              = NULL;
    videoSizeLut                = NULL;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport            = false;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(S5K8B1_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(S5K8B1_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(S5K8B1_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(S5K8B1_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K8B1_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(S5K8B1_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(S5K8B1_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(S5K8B1_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(S5K8B1_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = S5K8B1_PREVIEW_LIST;
    frontPictureList     = S5K8B1_PICTURE_LIST;
    hiddenFrontPreviewList   = S5K8B1_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = S5K8B1_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K8B1_THUMBNAIL_LIST;
    frontVideoList       = S5K8B1_VIDEO_LIST;
    hiddenFrontVideoList   = S5K8B1_HIDDEN_VIDEO_LIST;
    frontFPSList       = S5K8B1_FPS_RANGE_LIST;
    hiddenFrontFPSList   = S5K8B1_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K6D1Base::ExynosSensorS5K6D1Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 2560;
    maxPreviewH = 1440;
    maxPictureW = 2560;
    maxPictureH = 1440;
    maxVideoW = 2560;
    maxVideoH = 1440;
    maxSensorW = 2576;
    maxSensorH = 1456;
    sensorMarginW = 16;
    sensorMarginH = 16;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 160;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 1900;
    apertureDen = 1000;

    horizontalViewAngle[SIZE_RATIO_16_9] = 79.8f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.2f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 50.8f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 71.8f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 65.2f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 74.8f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 65.2f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 22;

    minFps = 1;
    maxFps = 30;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL_FRONT;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;

    zoomSupport = false;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = true;
    drcSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
#ifndef USE_CAMERA2_API_SUPPORT
        | EFFECT_BEAUTY_FACE
#endif
        ;

    flashModeList =
          FLASH_MODE_OFF
        /*| FLASH_MODE_AUTO*/
        /*| FLASH_MODE_ON*/
        /*| FLASH_MODE_RED_EYE*/
        /*| FLASH_MODE_TORCH*/
        ;

    focusModeList =
        /* FOCUS_MODE_AUTO*/
        FOCUS_MODE_FIXED
        /*| FOCUS_MODE_MACRO*/
        | FOCUS_MODE_INFINITY
        /*| FOCUS_MODE_EDOF*/
        /*| FOCUS_MODE_CONTINUOUS_VIDEO*/
        /*| FOCUS_MODE_CONTINUOUS_PICTURE*/
        /*| FOCUS_MODE_TOUCH*/
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /* WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /* WHITE_BALANCE_TWILIGHT*/
        /* WHITE_BALANCE_SHADE*/
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_6D1) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_6D1) / (sizeof(int) * SIZE_OF_LUT);
#ifdef ENABLE_8MP_FULL_FRAME
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_6D1_8MP_FULL) / (sizeof(int) * SIZE_OF_LUT);
#else
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_6D1) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

#endif
    previewSizeLut              = PREVIEW_SIZE_LUT_6D1;
    pictureSizeLut              = PICTURE_SIZE_LUT_6D1;
#ifdef ENABLE_8MP_FULL_FRAME
    videoSizeLut                = VIDEO_SIZE_LUT_6D1_8MP_FULL;
#else
    videoSizeLut                = VIDEO_SIZE_LUT_6D1;
#endif
    dualPreviewSizeLut          = DUAL_PREVIEW_SIZE_LUT_6D1;
    dualVideoSizeLut            = DUAL_VIDEO_SIZE_LUT_6D1;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport            = true;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(S5K6D1_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(S5K6D1_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(S5K6D1_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(S5K6D1_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K6D1_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(S5K6D1_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(S5K6D1_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(S5K6D1_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(S5K6D1_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = S5K6D1_PREVIEW_LIST;
    frontPictureList     = S5K6D1_PICTURE_LIST;
    hiddenFrontPreviewList   = S5K6D1_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = S5K6D1_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K6D1_THUMBNAIL_LIST;
    frontVideoList       = S5K6D1_VIDEO_LIST;
    hiddenFrontVideoList   = S5K6D1_HIDDEN_VIDEO_LIST;
    frontFPSList       = S5K6D1_FPS_RANGE_LIST;
    hiddenFrontFPSList   = S5K6D1_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K4E6Base::ExynosSensorS5K4E6Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 2560;
    maxPreviewH = 1440;
    maxPictureW = 2592;
    maxPictureH = 1950;
    maxVideoW = 2560;
    maxVideoH = 1440;
    maxSensorW = 2608;
    maxSensorH = 1960;
    sensorMarginW = 16;
    sensorMarginH = 10;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 17;
    fNumberDen = 10;
    focalLengthNum = 210;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureDen = 1000;
    apertureNum = APEX_FNUM_TO_APERTURE((double)(fNumberNum) / (double)(fNumberDen)) * apertureDen;;

    horizontalViewAngle[SIZE_RATIO_16_9] = 77.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 77.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 60.8f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 71.8f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 65.2f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 74.8f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 73.0f;
    verticalViewAngle = 61.0f;
    focalLengthIn35mmLength = 22;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL_FRONT;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = true;
    drcSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        | EFFECT_BEAUTY_FACE
        ;

    flashModeList =
          FLASH_MODE_OFF
        /*| FLASH_MODE_AUTO*/
        /*| FLASH_MODE_ON*/
        /*| FLASH_MODE_RED_EYE*/
        /*| FLASH_MODE_TORCH*/
        ;

    focusModeList =
        /* FOCUS_MODE_AUTO*/
        FOCUS_MODE_FIXED
        /*| FOCUS_MODE_MACRO*/
        | FOCUS_MODE_INFINITY
        /*| FOCUS_MODE_EDOF*/
        /*| FOCUS_MODE_CONTINUOUS_VIDEO*/
        /*| FOCUS_MODE_CONTINUOUS_PICTURE*/
        /*| FOCUS_MODE_TOUCH*/
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        | SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /* WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /* WHITE_BALANCE_TWILIGHT*/
        /* WHITE_BALANCE_SHADE*/
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_4E6) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_4E6) / (sizeof(int) * SIZE_OF_LUT);
#if defined(ENABLE_8MP_FULL_FRAME) || defined(ENABLE_13MP_FULL_FRAME)
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_4E6_FULL) / (sizeof(int) * SIZE_OF_LUT);
#else
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_4E6) / (sizeof(int) * SIZE_OF_LUT);
#endif
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_4E6) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_4E6) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_4E6) / (sizeof(int) * SIZE_OF_LUT);
    fastAeStableLutMax          = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_4E6) / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_4E6;
    pictureSizeLut              = PICTURE_SIZE_LUT_4E6;
#if defined(ENABLE_8MP_FULL_FRAME) || defined(ENABLE_13MP_FULL_FRAME)
    videoSizeLut                = VIDEO_SIZE_LUT_4E6_FULL;
#else
    videoSizeLut                = VIDEO_SIZE_LUT_4E6;
#endif
    dualPreviewSizeLut          = DUAL_PREVIEW_SIZE_LUT_4E6;
    dualVideoSizeLut            = DUAL_VIDEO_SIZE_LUT_4E6;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_4E6;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_4E6;
    fastAeStableLut             = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_4E6;
    vtcallSizeLut               = VTCALL_SIZE_LUT_4E6;
    sizeTableSupport            = true;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(S5K4E6_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(S5K4E6_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(S5K4E6_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(S5K4E6_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K4E6_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(S5K4E6_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(S5K4E6_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(S5K4E6_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(S5K4E6_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = S5K4E6_PREVIEW_LIST;
    frontPictureList     = S5K4E6_PICTURE_LIST;
    hiddenFrontPreviewList   = S5K4E6_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = S5K4E6_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K4E6_THUMBNAIL_LIST;
    frontVideoList       = S5K4E6_VIDEO_LIST;
    hiddenFrontVideoList   = S5K4E6_HIDDEN_VIDEO_LIST;
    frontFPSList       = S5K4E6_FPS_RANGE_LIST;
    hiddenFrontFPSList   = S5K4E6_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorS5K5E3Base::ExynosSensorS5K5E3Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 2560;
    maxPictureH = 1920;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 2576;
    maxSensorH = 1932;
    sensorMarginW = 16;
    sensorMarginH = 12;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 245;
    fNumberDen = 100;
    focalLengthNum = 185;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 69.8f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 42.8f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 27;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL_FRONT;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_AQUA
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        | EFFECT_COLD_VINTAGE
        | EFFECT_BLUE
        | EFFECT_RED_YELLOW
        ;

    flashModeList =
          FLASH_MODE_OFF
        /*| FLASH_MODE_AUTO*/
        /*| FLASH_MODE_ON*/
        /*| FLASH_MODE_RED_EYE*/
        /*| FLASH_MODE_TORCH*/
        ;

    focusModeList =
        /* FOCUS_MODE_AUTO*/
        FOCUS_MODE_INFINITY
        /*| FOCUS_MODE_MACRO*/
        | FOCUS_MODE_FIXED
        /*| FOCUS_MODE_EDOF*/
        /*| FOCUS_MODE_CONTINUOUS_VIDEO*/
        /*| FOCUS_MODE_CONTINUOUS_PICTURE*/
        /*| FOCUS_MODE_TOUCH*/
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT*/;


    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /* WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /* WHITE_BALANCE_TWILIGHT*/
        /* WHITE_BALANCE_SHADE*/
        ;

    previewSizeLutMax     = 0;
    pictureSizeLutMax     = 0;
    videoSizeLutMax       = 0;
    previewSizeLut        = NULL;
    pictureSizeLut        = NULL;
    videoSizeLut          = NULL;

    sizeTableSupport      = false;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_5E3) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_5E3) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_5E3) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut              = PREVIEW_SIZE_LUT_5E3;
    pictureSizeLut              = PICTURE_SIZE_LUT_5E3;
    videoSizeLut                = VIDEO_SIZE_LUT_5E3;
    dualVideoSizeLut            = VIDEO_SIZE_LUT_5E3;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport            = true;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(S5K5E3_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(S5K5E3_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(S5K5E3_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(S5K5E3_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K5E3_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(S5K5E3_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(S5K5E3_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(S5K5E3_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(S5K5E3_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = S5K5E3_PREVIEW_LIST;
    frontPictureList     = S5K5E3_PICTURE_LIST;
    hiddenFrontPreviewList   = S5K5E3_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = S5K5E3_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K5E3_THUMBNAIL_LIST;
    frontVideoList       = S5K5E3_VIDEO_LIST;
    hiddenFrontVideoList   = S5K5E3_HIDDEN_VIDEO_LIST;
    frontFPSList       = S5K5E3_FPS_RANGE_LIST;
    hiddenFrontFPSList   = S5K5E3_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorSR544Base::ExynosSensorSR544Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1280;
    maxPreviewH = 960;
    maxPictureW = 2592;
    maxPictureH = 1944;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 2600;
    maxSensorH = 1952;
    sensorMarginW = 8;
    sensorMarginH = 8;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 22;
    fNumberDen = 10;
    focalLengthNum = 330;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 54.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 42.8f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 41.0f;
    focalLengthIn35mmLength = 27;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_RED_YELLOW
        | EFFECT_BLUE
        | EFFECT_COLD_VINTAGE
        | EFFECT_AQUA
#ifndef USE_CAMERA2_API_SUPPORT
        | EFFECT_BEAUTY_FACE
#endif
        ;

    flashModeList =
          FLASH_MODE_OFF
          ;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        //| FOCUS_MODE_FIXED
        //| FOCUS_MODE_EDOF
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_PARTY
        | SCENE_MODE_SPORTS
        | SCENE_MODE_CANDLELIGHT*/
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_SR544) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_SR544) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_SR544) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_SR544) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_SR544) / (sizeof(int) * SIZE_OF_LUT);

    previewSizeLut              = PREVIEW_SIZE_LUT_SR544;
    pictureSizeLut              = PICTURE_SIZE_LUT_SR544;
    videoSizeLut                = VIDEO_SIZE_LUT_SR544;
    dualVideoSizeLut            = VIDEO_SIZE_LUT_SR544;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_SR544;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_SR544;
    sizeTableSupport      = true;

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(SR544_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(SR544_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(SR544_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(SR544_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(SR544_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(SR544_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(SR544_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(SR544_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(SR544_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = SR544_PREVIEW_LIST;
    rearPictureList     = SR544_PICTURE_LIST;
    hiddenRearPreviewList   = SR544_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = SR544_HIDDEN_PICTURE_LIST;
    thumbnailList   = SR544_THUMBNAIL_LIST;
    rearVideoList       = SR544_VIDEO_LIST;
    hiddenRearVideoList   = SR544_HIDDEN_VIDEO_LIST;
    rearFPSList       = SR544_FPS_RANGE_LIST;
    hiddenRearFPSList   = SR544_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorIMX240_2P2Base::ExynosSensorIMX240_2P2Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 3840;
    maxPreviewH = 2160;
    maxPictureW = 5312;
    maxPictureH = 2988;
    maxVideoW = 3840;
    maxVideoH = 2160;
    maxSensorW = 5328;
    maxSensorH = 3000;
    sensorMarginW = 16;
    sensorMarginH = 12;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 430;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 185;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 53.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 41.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 41.0f;
    focalLengthIn35mmLength = 28;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    minExposureTime = 32;
    maxExposureTime = 10000000;
    minWBK = 2300;
    maxWBK = 10000;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        | EFFECT_BEAUTY_FACE
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        //| FLASH_MODE_RED_EYE
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        //| FOCUS_MODE_FIXED
        //| FOCUS_MODE_EDOF
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_PARTY
        | SCENE_MODE_SPORTS
        | SCENE_MODE_CANDLELIGHT
        */
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        //| WHITE_BALANCE_WARM_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| WHITE_BALANCE_TWILIGHT
        //| WHITE_BALANCE_SHADE
        | WHITE_BALANCE_CUSTOM_K
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = true;

    if (bnsSupport == true) {

#if defined(USE_BNS_PREVIEW)
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_IMX240_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);
#else
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_IMX240_2P2) / (sizeof(int) * SIZE_OF_LUT);
#endif

#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX240_2P2_8MP_FULL)       / (sizeof(int) * SIZE_OF_LUT);
#else
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX240_2P2)       / (sizeof(int) * SIZE_OF_LUT);
#endif
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_IMX240_2P2)     / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_IMX240_2P2_BNS)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX240_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX240_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);
        fastAeStableLutMax          = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX240_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);

#if defined(USE_BNS_PREVIEW)
        previewSizeLut              = PREVIEW_SIZE_LUT_IMX240_2P2_BNS;
#else
        previewSizeLut              = PREVIEW_SIZE_LUT_IMX240_2P2;
#endif
        dualPreviewSizeLut          = PREVIEW_SIZE_LUT_IMX240_2P2_BNS;

#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLut                = VIDEO_SIZE_LUT_IMX240_2P2_8MP_FULL;
#else
        videoSizeLut                = VIDEO_SIZE_LUT_IMX240_2P2;
#endif
        videoSizeBnsLut             = VIDEO_SIZE_LUT_IMX240_2P2_BNS;
        pictureSizeLut              = PICTURE_SIZE_LUT_IMX240_2P2;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX240_2P2_BNS;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX240_2P2_BNS;
        fastAeStableLut             = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX240_2P2_BNS;
        vtcallSizeLut               = VTCALL_SIZE_LUT_IMX240_2P2_BNS;
        sizeTableSupport            = true;
    } else {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        vtcallSizeLutMax            = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeBnsLut             = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        fastAeStableLut             = NULL;
        vtcallSizeLut               = NULL;
        sizeTableSupport            = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(IMX240_2P2_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(IMX240_2P2_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(IMX240_2P2_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(IMX240_2P2_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(IMX240_2P2_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(IMX240_2P2_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(IMX240_2P2_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(IMX240_2P2_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(IMX240_2P2_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = IMX240_2P2_PREVIEW_LIST;
    rearPictureList     = IMX240_2P2_PICTURE_LIST;
    hiddenRearPreviewList   = IMX240_2P2_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = IMX240_2P2_HIDDEN_PICTURE_LIST;
    thumbnailList   = IMX240_2P2_THUMBNAIL_LIST;
    rearVideoList       = IMX240_2P2_VIDEO_LIST;
    hiddenRearVideoList   = IMX240_2P2_HIDDEN_VIDEO_LIST;
    rearFPSList       = IMX240_2P2_FPS_RANGE_LIST;
    hiddenRearFPSList   = IMX240_2P2_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorIMX260_2L1Base::ExynosSensorIMX260_2L1Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 3840;
    maxPreviewH = 2160;
    maxPictureW = 4032;
    maxPictureH = 3024;
    maxVideoW = 3840;
    maxVideoH = 2160;
    maxSensorW = 4032;
    maxSensorH = 3024;
    sensorMarginW = 0;
    sensorMarginH = 0;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 17;
    fNumberDen = 10;
    focalLengthNum = 420;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureDen = 100;
    apertureNum = APEX_FNUM_TO_APERTURE((double)(fNumberNum) / (double)(fNumberDen)) * apertureDen;

    horizontalViewAngle[SIZE_RATIO_16_9] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 53.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 41.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 41.0f;
    focalLengthIn35mmLength = 26;

    minFps = 1;
    maxFps = 30;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif
    minExposureTime = 32;
    maxExposureTime = 10000000;
    minWBK = 2300;
    maxWBK = 10000;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_NONE
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /*| EFFECT_SOLARIZE*/
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        /*| EFFECT_WHITEBOARD*/
        /*| EFFECT_BLACKBOARD*/
        | EFFECT_AQUA
        | EFFECT_BEAUTY_FACE
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        //| FLASH_MODE_RED_EYE
        | FLASH_MODE_TORCH;

    focusModeList =
          FOCUS_MODE_AUTO
        | FOCUS_MODE_INFINITY
        | FOCUS_MODE_MACRO
        //| FOCUS_MODE_FIXED
        //| FOCUS_MODE_EDOF
        | FOCUS_MODE_CONTINUOUS_VIDEO
        | FOCUS_MODE_CONTINUOUS_PICTURE
        | FOCUS_MODE_TOUCH
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_PARTY
        | SCENE_MODE_SPORTS
        | SCENE_MODE_CANDLELIGHT
        */
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        //| WHITE_BALANCE_WARM_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        //| WHITE_BALANCE_TWILIGHT
        //| WHITE_BALANCE_SHADE
        | WHITE_BALANCE_CUSTOM_K
        ;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = true;

    if (bnsSupport == true) {

#if defined(USE_BNS_PREVIEW)
        previewSizeLutMax     = sizeof(PREVIEW_SIZE_LUT_IMX260_2L1_BNS) / (sizeof(int) * SIZE_OF_LUT);
#else
        previewSizeLutMax     = sizeof(PREVIEW_SIZE_LUT_IMX260_2L1) / (sizeof(int) * SIZE_OF_LUT);
#endif

#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLutMax       = sizeof(VIDEO_SIZE_LUT_IMX260_2L1_8MP_FULL)       / (sizeof(int) * SIZE_OF_LUT);
#else
        videoSizeLutMax       = sizeof(VIDEO_SIZE_LUT_IMX260_2L1)       / (sizeof(int) * SIZE_OF_LUT);
#endif
        pictureSizeLutMax     = sizeof(PICTURE_SIZE_LUT_IMX260_2L1)     / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax      = sizeof(VTCALL_SIZE_LUT_IMX260_2L1)     / (sizeof(int) * SIZE_OF_LUT);

        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_IMX260_2L1) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX260_2L1) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_IMX260_2L1) / (sizeof(int) * SIZE_OF_LUT);
        fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_IMX260_2L1) / (sizeof(int) * SIZE_OF_LUT);
        liveBroadcastSizeLutMax     = sizeof(LIVE_BROADCAST_SIZE_LUT_IIMX260_2L1) / (sizeof(int) * SIZE_OF_LUT);
#if defined(USE_BNS_PREVIEW)
        previewSizeLut              = PREVIEW_SIZE_LUT_IMX260_2L1_BNS;
#else
        previewSizeLut              = PREVIEW_SIZE_LUT_IMX260_2L1;
#endif
        dualPreviewSizeLut          = PREVIEW_SIZE_LUT_IMX260_2L1_BNS;

#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLut                = VIDEO_SIZE_LUT_IMX260_2L1_8MP_FULL;
#else
        videoSizeLut                = VIDEO_SIZE_LUT_IMX260_2L1;
#endif
        pictureSizeLut              = PICTURE_SIZE_LUT_IMX260_2L1;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_IMX260_2L1;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX260_2L1;
        videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_IMX260_2L1;
        fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_IMX260_2L1;
        vtcallSizeLut               = VTCALL_SIZE_LUT_IMX260_2L1;
        liveBroadcastSizeLut        = LIVE_BROADCAST_SIZE_LUT_IIMX260_2L1;
        sizeTableSupport            = true;
    } else {
        previewSizeLutMax     = 0;
        pictureSizeLutMax     = 0;
        videoSizeLutMax       = 0;
        vtcallSizeLutMax      = 0;
        fastAeStableLutMax    = 0;
        previewSizeLut        = NULL;
        pictureSizeLut        = NULL;
        videoSizeLut          = NULL;
        videoSizeBnsLut       = NULL;
        videoSizeLutHighSpeed = NULL;
        vtcallSizeLut         = NULL;
        fastAeStableLut       = NULL;
        sizeTableSupport      = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(IMX260_2L1_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(IMX260_2L1_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(IMX260_2L1_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(IMX260_2L1_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(IMX260_2L1_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(IMX260_2L1_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    depthMapSizeLutMax      = sizeof(DEPTH_MAP_SIZE_LUT_IMX260_2L1) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(IMX260_2L1_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(IMX260_2L1_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(IMX260_2L1_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = IMX260_2L1_PREVIEW_LIST;
    rearPictureList     = IMX260_2L1_PICTURE_LIST;
    hiddenRearPreviewList   = IMX260_2L1_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = IMX260_2L1_HIDDEN_PICTURE_LIST;
    thumbnailList   = IMX260_2L1_THUMBNAIL_LIST;
    rearVideoList       = IMX260_2L1_VIDEO_LIST;
    depthMapSizeLut = DEPTH_MAP_SIZE_LUT_IMX260_2L1;
    hiddenRearVideoList   = IMX260_2L1_HIDDEN_VIDEO_LIST;
    rearFPSList       = IMX260_2L1_FPS_RANGE_LIST;
    hiddenRearFPSList   = IMX260_2L1_HIDDEN_FPS_RANGE_LIST;
};

ExynosSensorOV5670Base::ExynosSensorOV5670Base() : ExynosSensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 2592;
    maxPictureH = 1944;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 2608;
    maxSensorH = 1960;
    sensorMarginW = 16;
    sensorMarginH = 16;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    fNumberNum = 245;
    fNumberDen = 100;
    focalLengthNum = 185;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 69.8f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 42.8f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 27;

    minFps = 1;
    maxFps = 30;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = true;

    antiBandingList =
          ANTIBANDING_AUTO
        | ANTIBANDING_50HZ
        | ANTIBANDING_60HZ
        | ANTIBANDING_OFF
        ;

    effectList =
          EFFECT_NONE
        ;

    hiddenEffectList =
          EFFECT_AQUA
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        | EFFECT_COLD_VINTAGE
        | EFFECT_BLUE
        | EFFECT_RED_YELLOW
        ;

    flashModeList =
          FLASH_MODE_OFF
        /*| FLASH_MODE_AUTO*/
        /*| FLASH_MODE_ON*/
        /*| FLASH_MODE_RED_EYE*/
        /*| FLASH_MODE_TORCH*/
        ;

    focusModeList =
        /* FOCUS_MODE_AUTO*/
        FOCUS_MODE_INFINITY
        /*| FOCUS_MODE_MACRO*/
        | FOCUS_MODE_FIXED
        /*| FOCUS_MODE_EDOF*/
        /*| FOCUS_MODE_CONTINUOUS_VIDEO*/
        /*| FOCUS_MODE_CONTINUOUS_PICTURE*/
        /*| FOCUS_MODE_TOUCH*/
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        /*| SCENE_MODE_ACTION
        | SCENE_MODE_PORTRAIT
        | SCENE_MODE_LANDSCAPE
        | SCENE_MODE_NIGHT
        | SCENE_MODE_NIGHT_PORTRAIT
        | SCENE_MODE_THEATRE
        | SCENE_MODE_BEACH
        | SCENE_MODE_SNOW
        | SCENE_MODE_SUNSET
        | SCENE_MODE_STEADYPHOTO
        | SCENE_MODE_FIREWORKS
        | SCENE_MODE_SPORTS
        | SCENE_MODE_PARTY
        | SCENE_MODE_CANDLELIGHT*/;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        /* WHITE_BALANCE_WARM_FLUORESCENT*/
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        /* WHITE_BALANCE_TWILIGHT*/
        /* WHITE_BALANCE_SHADE*/
        ;

    previewSizeLutMax           = 0;
    pictureSizeLutMax           = 0;
    videoSizeLutMax             = 0;
    previewSizeLut              = NULL;
    pictureSizeLut              = NULL;
    videoSizeLut                = NULL;
    sizeTableSupport            = false;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;
    scalableSensorSupport = true;
    bnsSupport = false;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_OV5670) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_OV5670) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_OV5670) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut              = PREVIEW_SIZE_LUT_OV5670;
    pictureSizeLut              = PICTURE_SIZE_LUT_OV5670;
    videoSizeLut                = VIDEO_SIZE_LUT_OV5670;
    dualVideoSizeLut            = VIDEO_SIZE_LUT_OV5670;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport            = true;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(OV5670_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(OV5670_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(OV5670_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(OV5670_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(OV5670_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(OV5670_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(OV5670_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(OV5670_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(OV5670_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList     = OV5670_PREVIEW_LIST;
    frontPictureList     = OV5670_PICTURE_LIST;
    hiddenFrontPreviewList   = OV5670_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = OV5670_HIDDEN_PICTURE_LIST;
    thumbnailList   = OV5670_THUMBNAIL_LIST;
    frontVideoList       = OV5670_VIDEO_LIST;
    hiddenFrontVideoList   = OV5670_HIDDEN_VIDEO_LIST;
    frontFPSList       = OV5670_FPS_RANGE_LIST;
    hiddenFrontFPSList   = OV5670_HIDDEN_FPS_RANGE_LIST;
};

}; /* namespace android */
