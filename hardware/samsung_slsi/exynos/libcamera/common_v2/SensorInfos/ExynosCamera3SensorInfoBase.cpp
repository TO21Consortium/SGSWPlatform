/*
**
** Copyright 2015, Samsung Electronics Co. LTD
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
#define LOG_TAG "ExynosCamera3SensorInfoBase"
#include <cutils/log.h>

#include "ExynosCamera3SensorInfoBase.h"

namespace android {

#if 0//def SENSOR_NAME_GET_FROM_FILE
int g_rearSensorId = -1;
int g_frontSensorId = -1;
#endif

ExynosCamera3SensorInfoBase::ExynosCamera3SensorInfoBase() : ExynosSensorInfoBase()
{
    /* implement AP chip variation */
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 4128;
    maxPictureH = 3096;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 4128;
    maxSensorH = 3096;
    sensorMarginW = 16;
    sensorMarginH = 10;
    sensorMarginBase[LEFT_BASE] = 0;
    sensorMarginBase[TOP_BASE] = 0;
    sensorMarginBase[WIDTH_BASE] = 0;
    sensorMarginBase[HEIGHT_BASE] = 0;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;

    /* TODO : Where should we go? */
    minFps = 0;
    maxFps = 30;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    videoSnapshotSupport = true;

    maxBasicZoomLevel = MAX_BASIC_ZOOM_LEVEL;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;


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
    visionModeSupport = false;
    drcSupport = false;

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
    highSpeedVideoListMax = 0;
    rearFPSListMax = 0;
    frontFPSListMax = 0;
    hiddenRearFPSListMax = 0;
    hiddenFrontFPSListMax = 0;
    highSpeedVideoFPSListMax = 0;

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
    highSpeedVideoList = NULL;
    rearFPSList = NULL;
    frontFPSList = NULL;
    hiddenRearFPSList = NULL;
    hiddenFrontFPSList = NULL;
    highSpeedVideoFPSList = NULL;

    previewSizeLutMax           = 0;
    pictureSizeLutMax           = 0;
    videoSizeLutMax             = 0;
    vtcallSizeLutMax            = 0;
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;
    fastAeStableLutMax          = 0;

    previewSizeLut              = NULL;
    pictureSizeLut              = NULL;
    videoSizeLut                = NULL;
    videoSizeBnsLut             = NULL;
    dualPreviewSizeLut          = NULL;
    dualVideoSizeLut            = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;

    vtcallSizeLut               = NULL;
    fastAeStableLut             = NULL;

    sizeTableSupport      = false;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */
    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = NULL;
    colorAberrationModesLength = 0;

    /* Android Control Static Metadata */
    antiBandingModes = NULL;
    aeModes = NULL;
    exposureCompensationRange[MIN] = -2;
    exposureCompensationRange[MAX] = 2;
    exposureCompensationStep = 1.0f;
    afModes = NULL;
    effectModes = NULL;
    sceneModes = NULL;
    videoStabilizationModes = NULL;
    awbModes = NULL;
    controlModes = NULL;
    controlModesLength = 0;
    max3aRegions[AE] = 0;
    max3aRegions[AWB] = 0;
    max3aRegions[AF] = 0;
    sceneModeOverrides = NULL;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_FALSE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_FALSE;
    antiBandingModesLength = 0;
    aeModesLength = 0;
    afModesLength = 0;
    effectModesLength = 0;
    sceneModesLength = 0;
    videoStabilizationModesLength = 0;
    awbModesLength = 0;
    sceneModeOverridesLength = 0;

    /* Android Edge Static Metadata */
    edgeModes = NULL;
    edgeModesLength = 0;

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = NULL;
    hotPixelModesLength = 0;

    /* Android Lens Static Metadata */
    aperture = 2.2f;
    fNumber = 2.2f;
    filterDensity = 0.0f;
    focalLength = 4.8f;
    focalLengthIn35mmLength = 31;
    opticalStabilization = NULL;
    hyperFocalDistance = 0.0f;
    minimumFocusDistance = 0.0f;
    shadingMapSize[WIDTH] = 0;
    shadingMapSize[HEIGHT] = 0;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED;
    lensFacing = ANDROID_LENS_FACING_BACK;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 0.0f;
    lensPosition[Z_3D] = 0.0f;
    opticalStabilizationLength = 0;

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = NULL;
    noiseReductionModesLength = 0;

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 0;
    maxPipelineDepth = 5;
    partialResultCount = 1;
    capabilities = NULL;
    requestKeys = NULL;
    resultKeys = NULL;
    characteristicsKeys = NULL;
    capabilitiesLength = 0;
    requestKeysLength = 0;
    resultKeysLength = 0;
    characteristicsKeysLength = 0;

    /* Android Scaler Static Metadata */
    zoomSupport = false;
    smoothZoomSupport = false;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;
    stallDurations = NULL;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY;
    stallDurationsLength = 0;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 4000;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 1000;
    blackLevelPattern[GR] = 1000;
    blackLevelPattern[GB] = 1000;
    blackLevelPattern[B] = 1000;
    maxAnalogSensitivity = 800;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = NULL;
    testPatternModesLength = 0;

    /* Android Statistics Static Metadata */
    faceDetectModes = NULL;
    histogramBucketCount = 64;
    maxNumDetectedFaces = 1;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = NULL;
    faceDetectModesLength = 0;
    hotPixelMapModesLength = 0;
    lensShadingMapModes = NULL;
    lensShadingMapModesLength = 0;
    shadingAvailableModes = NULL;
    shadingAvailableModesLength = 0;

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = NULL;
    toneMapModesLength = 0;

    /* Android LED Static Metadata */
    leds = NULL;
    ledsLength = 0;

    /* Android Info Static Metadata */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY;

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

#if 0
ExynosCamera3SensorS5K2P2Base::ExynosCamera3SensorS5K2P2Base() : ExynosCamera3SensorInfoBase()
{
    /* Sensor Max Size Infos */
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
    sensorMarginBase[LEFT_BASE] = 2;
    sensorMarginBase[TOP_BASE] = 2;
    sensorMarginBase[WIDTH_BASE] = 4;
    sensorMarginBase[HEIGHT_BASE] = 4;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    /* Sensor FOV Infos */
    horizontalViewAngle[SIZE_RATIO_16_9] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 53.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 41.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 41.0f;

    /* TODO : Where should we go? */
    minFps = 1;
    maxFps = 30;
    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 430;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 185;
    apertureDen = 100;

    /* Hal1 info - prevent setparam fail */
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

    /* Hal1 info - prevent setparam fail */
    antiBandingList =
          ANTIBANDING_AUTO
        ;

    flashModeList =
          FLASH_MODE_OFF
        ;

    focusModeList =
        FOCUS_MODE_FIXED
        | FOCUS_MODE_INFINITY
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        ;

    bnsSupport = true;

    if (bnsSupport == true) {
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);
#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2P2_8MP_FULL) / (sizeof(int) * SIZE_OF_LUT);
#else
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2P2) / (sizeof(int) * SIZE_OF_LUT);
#endif
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2P2) / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = PREVIEW_SIZE_LUT_2P2_BNS;
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

   /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    lensFacing = ANDROID_LENS_FACING_BACK;
    switch (supportedHwLevel) {
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
#if 0
            capabilities = AVAILABLE_CAPABILITIES_LIMITED;
            requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED;
            resultKeys = AVAILABLE_RESULT_KEYS_LIMITED;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
#else
            capabilities = AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL;
            requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL;
            resultKeys = AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL);
#endif
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
            capabilities = AVAILABLE_CAPABILITIES_FULL;
            requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
            resultKeys = AVAILABLE_RESULT_KEYS_FULL;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
            capabilities = AVAILABLE_CAPABILITIES_LEGACY;
            requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
            resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid supported HW level(%d)", __FUNCTION__, __LINE__,
                    supportedHwLevel);
            break;
    }
    switch (lensFacing) {
        case ANDROID_LENS_FACING_FRONT:
            aeModes = AVAILABLE_AE_MODES_FRONT;
            afModes = AVAILABLE_AF_MODES_FRONT;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
            break;
        case ANDROID_LENS_FACING_BACK:
            aeModes = AVAILABLE_AE_MODES_BACK;
            afModes = AVAILABLE_AF_MODES_BACK;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid lens facing info(%d)", __FUNCTION__, __LINE__,
                    lensFacing);
            break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.85f;
    fNumber = 1.9f;
    filterDensity = 0.0f;
    focalLength = 4.3f;
    focalLengthIn35mmLength = 28;
    hyperFocalDistance = 1.0f / 5.0f;
    minimumFocusDistance = 1.0f / 0.1f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 0;
    maxPipelineDepth = 5;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    smoothZoomSupport = false;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 4000;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 1000;
    blackLevelPattern[GR] = 1000;
    blackLevelPattern[GB] = 1000;
    blackLevelPattern[B] = 1000;
    maxAnalogSensitivity = 800;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};
#endif

ExynosCamera3SensorS5K2P2_12MBase::ExynosCamera3SensorS5K2P2_12MBase() : ExynosCamera3SensorInfoBase()
{
#if 0
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
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;

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
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P2_12M_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P2_12M_BNS) / (sizeof(int) * SIZE_OF_LUT);

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
#endif
};

ExynosCamera3SensorS5K2P3Base::ExynosCamera3SensorS5K2P3Base() : ExynosCamera3SensorInfoBase()
{

};

ExynosCamera3SensorS5K2T2Base::ExynosCamera3SensorS5K2T2Base() : ExynosCamera3SensorInfoBase()
{
    /* Sensor Max Size Infos */
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

    /* Sensor FOV Infos */
    horizontalViewAngle[SIZE_RATIO_16_9] = 68.13f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 37.4f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 41.0f;

    /* TODO : Where should we go? */
    minFps = 1;
    maxFps = 30;
    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 430;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    /* Hal1 info - prevent setparam fail */
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

    /* Hal1 info - prevent setparam fail */
    antiBandingList =
          ANTIBANDING_AUTO
        ;

    effectList =
          EFFECT_NONE
        ;

    flashModeList =
          FLASH_MODE_OFF
        ;

    focusModeList =
        FOCUS_MODE_FIXED
        | FOCUS_MODE_INFINITY
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        ;

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
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2T2_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2T2_BNS) / (sizeof(int) * SIZE_OF_LUT);

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

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    lensFacing = ANDROID_LENS_FACING_BACK;
    switch (supportedHwLevel) {
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
            capabilities = AVAILABLE_CAPABILITIES_LIMITED;
            requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED;
            resultKeys = AVAILABLE_RESULT_KEYS_LIMITED;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
            capabilities = AVAILABLE_CAPABILITIES_FULL;
            requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
            resultKeys = AVAILABLE_RESULT_KEYS_FULL;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
            capabilities = AVAILABLE_CAPABILITIES_LEGACY;
            requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
            resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid supported HW level(%d)", __FUNCTION__, __LINE__,
                    supportedHwLevel);
            break;
    }
    switch (lensFacing) {
        case ANDROID_LENS_FACING_FRONT:
            aeModes = AVAILABLE_AE_MODES_FRONT;
            afModes = AVAILABLE_AF_MODES_FRONT;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
            break;
        case ANDROID_LENS_FACING_BACK:
            aeModes = AVAILABLE_AE_MODES_BACK;
            afModes = AVAILABLE_AF_MODES_BACK;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid lens facing info(%d)", __FUNCTION__, __LINE__,
                    lensFacing);
            break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 2.27f;
    fNumber = 1.9f;
    filterDensity = 0.0f;
    focalLength = 4.3f;
    focalLengthIn35mmLength = 28;
    hyperFocalDistance = 1.0f / 5.0f;
    minimumFocusDistance = 1.0f / 0.05f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 0;
    maxPipelineDepth = 5;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    smoothZoomSupport = false;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 4000;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 1000;
    blackLevelPattern[GR] = 1000;
    blackLevelPattern[GB] = 1000;
    blackLevelPattern[B] = 1000;
    maxAnalogSensitivity = 800;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCamera3SensorS5K3H5Base::ExynosCamera3SensorS5K3H5Base() : ExynosCamera3SensorInfoBase()
{
#if 0
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
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;

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
#endif
};

ExynosCamera3SensorS5K3H7Base::ExynosCamera3SensorS5K3H7Base() : ExynosCamera3SensorInfoBase()
{
#if 0
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
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;

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
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_3H7) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_3H7)   / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_3H7) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3H7) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3H7) / (sizeof(int) * SIZE_OF_LUT);

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
#endif
};

ExynosCamera3SensorS5K3M2Base::ExynosCamera3SensorS5K3M2Base() : ExynosCamera3SensorInfoBase()
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
    minFps = 1;
    maxFps = 30;
    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 370;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 48.2f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 28;


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
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /* | EFFECT_SOLARIZE */
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        | EFFECT_COLD_VINTAGE
        | EFFECT_BLUE
        | EFFECT_RED_YELLOW
        | EFFECT_AQUA
        /* | EFFECT_WHITEBOARD */
        /* | EFFECT_BLACKBOARD */
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
        previewSizeLutMax = sizeof(PREVIEW_SIZE_LUT_3M2_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax   = sizeof(VIDEO_SIZE_LUT_3M2)   / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax  = 0;
        previewSizeLut    = PREVIEW_SIZE_LUT_3M2_BNS;
        videoSizeLut      = VIDEO_SIZE_LUT_3M2;
        videoSizeBnsLut   = VIDEO_SIZE_LUT_3M2_BNS;
        vtcallSizeLut     = NULL;
    } else {
        previewSizeLutMax = sizeof(PREVIEW_SIZE_LUT_3M2) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax   = sizeof(VIDEO_SIZE_LUT_3M2)   / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax  = sizeof(VTCALL_SIZE_LUT_3M2) / (sizeof(int) * SIZE_OF_LUT);
        previewSizeLut    = PREVIEW_SIZE_LUT_3M2;
        videoSizeLut      = VIDEO_SIZE_LUT_3M2;
        videoSizeBnsLut   = NULL;
        vtcallSizeLut     = VTCALL_SIZE_LUT_3M2;
    }
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_3M2) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3M2) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3M2) / (sizeof(int) * SIZE_OF_LUT);

    pictureSizeLut              = PICTURE_SIZE_LUT_3M2;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_3M2;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_3M2;
    sizeTableSupport            = true;

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax      = sizeof(S5K3M2_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K3M2_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K3M2_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K3M2_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K3M2_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K3M2_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K3M2_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax = sizeof(S5K3M2_HIGH_SPEED_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K3M2_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K3M2_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);
    highSpeedVideoFPSListMax = sizeof(S5K3M2_HIGH_SPEED_VIDEO_FPS_RANGE_LIST) / (sizeof(int) *2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K3M2_YUV_LIST;
    rearPictureList     = S5K3M2_YUV_LIST;
    hiddenRearPreviewList   = S5K3M2_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K3M2_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K3M2_THUMBNAIL_LIST;
    rearVideoList       = S5K3M2_VIDEO_LIST;
    hiddenRearVideoList   = S5K3M2_HIDDEN_VIDEO_LIST;
    highSpeedVideoList = S5K3M2_HIGH_SPEED_VIDEO_LIST;
    rearFPSList       = S5K3M2_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K3M2_HIDDEN_FPS_RANGE_LIST;
    highSpeedVideoFPSList = S5K3M2_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    lensFacing = ANDROID_LENS_FACING_BACK;
    switch (supportedHwLevel) {
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
#if 1
        capabilities = AVAILABLE_CAPABILITIES_LIMITED;
        requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED;
        resultKeys = AVAILABLE_RESULT_KEYS_LIMITED;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
#else
        capabilities = AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL;
        requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL;
        resultKeys = AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL);
#endif
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
        capabilities = AVAILABLE_CAPABILITIES_FULL;
        requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
        resultKeys = AVAILABLE_RESULT_KEYS_FULL;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
        capabilities = AVAILABLE_CAPABILITIES_LEGACY;
        requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
        resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
        break;
    default:
        ALOGE("ERR(%s[%d]):Invalid supported HW level(%d)", __FUNCTION__, __LINE__,
                supportedHwLevel);
        break;
    }
    switch (lensFacing) {
    case ANDROID_LENS_FACING_FRONT:
        aeModes = AVAILABLE_AE_MODES_FRONT;
        afModes = AVAILABLE_AF_MODES_FRONT;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
        break;
    case ANDROID_LENS_FACING_BACK:
        aeModes = AVAILABLE_AE_MODES_BACK;
        afModes = AVAILABLE_AF_MODES_BACK;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
        break;
    default:
        ALOGE("ERR(%s[%d]):Invalid lens facing info(%d)", __FUNCTION__, __LINE__,
                lensFacing);
        break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.85f;
    fNumber = 1.9f;
    filterDensity = 0.0f;
    focalLength = 4.3f;
    focalLengthIn35mmLength = 28;
    hyperFocalDistance = 1.0f / 5.0f;
    minimumFocusDistance = 1.0f / 0.1f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 0;
    maxPipelineDepth = 7;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    smoothZoomSupport = false;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 50;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 500000000L;
    maxFrameDuration = 500000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    colorTransformMatrix1 = COLOR_MATRIX1_3M2_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_3M2_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_3M2_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_3M2_3X3;
    calibration1 = UNIT_MATRIX_3M2_3X3;
    calibration2 = UNIT_MATRIX_3M2_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCamera3SensorS5K3L2Base::ExynosCamera3SensorS5K3L2Base() : ExynosCamera3SensorInfoBase()
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
    minFps = 1;
    maxFps = 30;
    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 370;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 62.2f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 48.2f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 28;


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
        | EFFECT_MONO
        | EFFECT_NEGATIVE
        /* | EFFECT_SOLARIZE */
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        | EFFECT_COLD_VINTAGE
        | EFFECT_BLUE
        | EFFECT_RED_YELLOW
        | EFFECT_AQUA
        /* | EFFECT_WHITEBOARD */
        /* | EFFECT_BLACKBOARD */
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
    rearPreviewListMax      = sizeof(S5K3L2_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax      = sizeof(S5K3L2_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K3L2_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K3L2_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K3L2_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax        = sizeof(S5K3L2_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax    = sizeof(S5K3L2_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax = sizeof(S5K3L2_HIGH_SPEED_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax        = sizeof(S5K3L2_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax    = sizeof(S5K3L2_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);
    highSpeedVideoFPSListMax = sizeof(S5K3L2_HIGH_SPEED_VIDEO_FPS_RANGE_LIST) / (sizeof(int) *2);

    /* Set supported preview/picture/video lists */
    rearPreviewList     = S5K3L2_YUV_LIST;
    rearPictureList     = S5K3L2_YUV_LIST;
    hiddenRearPreviewList   = S5K3L2_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = S5K3L2_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K3L2_THUMBNAIL_LIST;
    rearVideoList       = S5K3L2_VIDEO_LIST;
    hiddenRearVideoList   = S5K3L2_HIDDEN_VIDEO_LIST;
    highSpeedVideoList = S5K3L2_HIGH_SPEED_VIDEO_LIST;
    rearFPSList       = S5K3L2_FPS_RANGE_LIST;
    hiddenRearFPSList   = S5K3L2_HIDDEN_FPS_RANGE_LIST;
    highSpeedVideoFPSList = S5K3L2_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    lensFacing = ANDROID_LENS_FACING_BACK;
    switch (supportedHwLevel) {
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
#if 1
        capabilities = AVAILABLE_CAPABILITIES_LIMITED;
        requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED;
        resultKeys = AVAILABLE_RESULT_KEYS_LIMITED;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
#else
        capabilities = AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL;
        requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL;
        resultKeys = AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL);
#endif
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
        capabilities = AVAILABLE_CAPABILITIES_FULL;
        requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
        resultKeys = AVAILABLE_RESULT_KEYS_FULL;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
        capabilities = AVAILABLE_CAPABILITIES_LEGACY;
        requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
        resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
        break;
    default:
        ALOGE("ERR(%s[%d]):Invalid supported HW level(%d)", __FUNCTION__, __LINE__,
                supportedHwLevel);
        break;
    }
    switch (lensFacing) {
    case ANDROID_LENS_FACING_FRONT:
        aeModes = AVAILABLE_AE_MODES_FRONT;
        afModes = AVAILABLE_AF_MODES_FRONT;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
        break;
    case ANDROID_LENS_FACING_BACK:
        aeModes = AVAILABLE_AE_MODES_BACK;
        afModes = AVAILABLE_AF_MODES_BACK;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
        break;
    default:
        ALOGE("ERR(%s[%d]):Invalid lens facing info(%d)", __FUNCTION__, __LINE__,
                lensFacing);
        break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.85f;
    fNumber = 1.9f;
    filterDensity = 0.0f;
    focalLength = 4.3f;
    focalLengthIn35mmLength = 28;
    hyperFocalDistance = 1.0f / 5.0f;
    minimumFocusDistance = 1.0f / 0.1f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 0;
    maxPipelineDepth = 7;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    smoothZoomSupport = false;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 50;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 500000000L;
    maxFrameDuration = 500000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    colorTransformMatrix1 = COLOR_MATRIX1_3L2_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_3L2_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_3L2_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_3L2_3X3;
    calibration1 = UNIT_MATRIX_3L2_3X3;
    calibration2 = UNIT_MATRIX_3L2_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCamera3SensorS5K4E6Base::ExynosCamera3SensorS5K4E6Base() : ExynosCamera3SensorInfoBase()
{
    maxPreviewW = 2592;
    maxPreviewH = 1944;
    maxPictureW = 2592;
    maxPictureH = 1944;
    maxVideoW = 2560;
    maxVideoH = 1440;
    maxSensorW = 2608;
    maxSensorH = 1960;
    sensorMarginW = 16;
    sensorMarginH = 10;
    sensorMarginBase[LEFT_BASE] = 2;
    sensorMarginBase[TOP_BASE] = 2;
    sensorMarginBase[WIDTH_BASE] = 4;
    sensorMarginBase[HEIGHT_BASE] = 4;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    horizontalViewAngle[SIZE_RATIO_16_9] = 77.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 77.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 60.8f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 71.8f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 65.2f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 74.8f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 65.2f;
    verticalViewAngle = 61.0f;

    /* TODO : Where should we go? */
    minFps = 1;
    maxFps = 30;
    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 220;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 1900;
    apertureDen = 1000;
    videoSnapshotSupport = true;

    /* Hal1 info - prevent setparam fail */
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;

    maxNumFocusAreas = 1;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL_FRONT;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = true;
    drcSupport = true;

    bnsSupport = false;

    effectList =
          EFFECT_NONE
        ;

    flashModeList =
          FLASH_MODE_OFF
        ;

    focusModeList =
        FOCUS_MODE_FIXED
        | FOCUS_MODE_INFINITY
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        ;

    previewSizeLutMax           = sizeof(YUV_SIZE_LUT_4E6) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(YUV_SIZE_LUT_4E6) / (sizeof(int) * SIZE_OF_LUT);
#if defined(ENABLE_8MP_FULL_FRAME) || defined(ENABLE_13MP_FULL_FRAME)
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_4E6_FULL) / (sizeof(int) * SIZE_OF_LUT);
#else
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_4E6) / (sizeof(int) * SIZE_OF_LUT);
#endif
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_4E6) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut              = YUV_SIZE_LUT_4E6;
    pictureSizeLut              = YUV_SIZE_LUT_4E6;
#if defined(ENABLE_8MP_FULL_FRAME) || defined(ENABLE_13MP_FULL_FRAME)
    videoSizeLut                = VIDEO_SIZE_LUT_4E6_FULL;
#else
    videoSizeLut                = VIDEO_SIZE_LUT_4E6;
#endif
    dualVideoSizeLut            = DUAL_VIDEO_SIZE_LUT_4E6;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    vtcallSizeLut               = VTCALL_SIZE_LUT_4E6;
    sizeTableSupport            = true;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax     = sizeof(S5K4E6_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax     = sizeof(S5K4E6_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(S5K4E6_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(S5K4E6_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K4E6_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(S5K4E6_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(S5K4E6_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(S5K4E6_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(S5K4E6_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    frontPreviewList        = S5K4E6_YUV_LIST;
    frontPictureList        = S5K4E6_YUV_LIST;
    hiddenFrontPreviewList   = S5K4E6_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = S5K4E6_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K4E6_THUMBNAIL_LIST;
    frontVideoList       = S5K4E6_VIDEO_LIST;
    hiddenFrontVideoList   = S5K4E6_HIDDEN_VIDEO_LIST;
    frontFPSList       = S5K4E6_FPS_RANGE_LIST;
    hiddenFrontFPSList   = S5K4E6_HIDDEN_FPS_RANGE_LIST;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    lensFacing = ANDROID_LENS_FACING_FRONT;
    switch (supportedHwLevel) {
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
        capabilities = AVAILABLE_CAPABILITIES_LIMITED_BURST;
        requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED_FRONT;
        resultKeys = AVAILABLE_RESULT_KEYS_LIMITED_FRONT;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED_BURST);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED_FRONT);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED_FRONT);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
        capabilities = AVAILABLE_CAPABILITIES_FULL;
        requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
        resultKeys = AVAILABLE_RESULT_KEYS_FULL;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
        capabilities = AVAILABLE_CAPABILITIES_LEGACY;
        requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
        resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
        break;
    default:
        break;
    }
    switch (lensFacing) {
    case ANDROID_LENS_FACING_FRONT:
        aeModes = AVAILABLE_AE_MODES_FRONT;
        afModes = AVAILABLE_AF_MODES_FRONT;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
        break;
    case ANDROID_LENS_FACING_BACK:
        aeModes = AVAILABLE_AE_MODES_BACK;
        afModes = AVAILABLE_AF_MODES_BACK;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
        break;
    default:
        break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 0;
    max3aRegions[AWB] = 0;
    max3aRegions[AF] = 0;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.9f;
    fNumber = 1.9f;
    filterDensity = 0.0f;
    focalLength = 2.2f;
    focalLengthIn35mmLength = 22;
    hyperFocalDistance = 0.0f;
    minimumFocusDistance = 0.0f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 20.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = 0.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 0;
    maxPipelineDepth = NUM_REQUEST_BLOCK_MAX;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    smoothZoomSupport = false;
    maxZoomLevel = MAX_ZOOM_LEVEL_FRONT;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 125000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 3.495f;
    sensorPhysicalSize[HEIGHT] = 2.626f;
    whiteLevel = 4000;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 1000;
    blackLevelPattern[GR] = 1000;
    blackLevelPattern[GB] = 1000;
    blackLevelPattern[B] = 1000;
    maxAnalogSensitivity = 800;
    orientation = FRONT_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    colorTransformMatrix1 = UNIT_MATRIX_4E6_3X3;
    colorTransformMatrix2 = UNIT_MATRIX_4E6_3X3;
    forwardMatrix1 = UNIT_MATRIX_4E6_3X3;
    forwardMatrix2 = UNIT_MATRIX_4E6_3X3;
    calibration1 = UNIT_MATRIX_4E6_3X3;
    calibration2 = UNIT_MATRIX_4E6_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[0] = 64;
    sharpnessMapSize[1] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCamera3SensorS5K4H5Base::ExynosCamera3SensorS5K4H5Base() : ExynosCamera3SensorInfoBase()
{
#if 0
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

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;

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
#endif
};

ExynosCamera3SensorS5K5E2Base::ExynosCamera3SensorS5K5E2Base() : ExynosCamera3SensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 2576;
    maxPictureH = 1930;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 2576;
    maxSensorH = 1930;
    sensorMarginW = 0;
    sensorMarginH = 0;

    minFps = 1;
    maxFps = 30;
    fNumberNum = 190;
    fNumberDen = 100;
    focalLengthNum = 200;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 185;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 70.3f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 70.3f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 55.8f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 23;

    maxZoomLevel = MAX_ZOOM_LEVEL_FRONT;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;

    effectList =
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
        ;

    hiddenEffectList =
          EFFECT_AQUA
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        /*| FLASH_MODE_RED_EYE*/
        | FLASH_MODE_TORCH
        ;

    previewSizeLutMax     = sizeof(PREVIEW_SIZE_LUT_5E2_YC) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax     = sizeof(PICTURE_SIZE_LUT_5E2_YC) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax       = sizeof(VIDEO_SIZE_LUT_5E2_YC) / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax      = sizeof(VTCALL_SIZE_LUT_5E2) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut        = PREVIEW_SIZE_LUT_5E2_YC;
    pictureSizeLut        = PICTURE_SIZE_LUT_5E2_YC;
    videoSizeLut          = VIDEO_SIZE_LUT_5E2_YC;
    dualVideoSizeLut      = VIDEO_SIZE_LUT_5E2_YC;
    vtcallSizeLut         = VTCALL_SIZE_LUT_5E2;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport      = true;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(S5K5E2_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(S5K5E2_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(S5K5E2_YC_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(S5K5E2_YC_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K5E2_YC_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(S5K5E2_YC_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(S5K5E2_YC_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(S5K5E2_YC_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(S5K5E2_YC_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);
    highSpeedVideoListMax = 0;
    highSpeedVideoFPSListMax = 0;

    /* Set supported preview/picture/video lists */
    frontPreviewList     = S5K5E2_YUV_LIST;
    frontPictureList     = S5K5E2_YUV_LIST;
    hiddenFrontPreviewList   = S5K5E2_YC_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = S5K5E2_YC_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K5E2_YC_THUMBNAIL_LIST;
    frontVideoList       = S5K5E2_YC_VIDEO_LIST;
    hiddenFrontVideoList   = S5K5E2_YC_HIDDEN_VIDEO_LIST;
    frontFPSList       = S5K5E2_YC_FPS_RANGE_LIST;
    hiddenFrontFPSList   = S5K5E2_YC_HIDDEN_FPS_RANGE_LIST;
    highSpeedVideoList = NULL;
    highSpeedVideoFPSList = NULL;

        /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    lensFacing = ANDROID_LENS_FACING_FRONT;
    switch (supportedHwLevel) {
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
        capabilities = AVAILABLE_CAPABILITIES_LIMITED_BURST;
        requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED_FRONT;
        resultKeys = AVAILABLE_RESULT_KEYS_LIMITED_FRONT;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED_BURST);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED_FRONT);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED_FRONT);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
        capabilities = AVAILABLE_CAPABILITIES_FULL;
        requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
        resultKeys = AVAILABLE_RESULT_KEYS_FULL;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
        capabilities = AVAILABLE_CAPABILITIES_LEGACY;
        requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
        resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
        break;
    default:
        break;
    }
    switch (lensFacing) {
    case ANDROID_LENS_FACING_FRONT:
        aeModes = AVAILABLE_AE_MODES_FRONT;
        afModes = AVAILABLE_AF_MODES_FRONT;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
        break;
    case ANDROID_LENS_FACING_BACK:
        aeModes = AVAILABLE_AE_MODES_BACK;
        afModes = AVAILABLE_AF_MODES_BACK;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
        break;
    default:
        break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 0;
    max3aRegions[AWB] = 0;
    max3aRegions[AF] = 0;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 2.4f;
    fNumber = 2.4f;
    filterDensity = 0.0f;
    focalLength = 1.86f;
    focalLengthIn35mmLength = 27;
    hyperFocalDistance = 0.0f;
    minimumFocusDistance = 0.0f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 20.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = 0.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 0;
    maxPipelineDepth = 7;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    smoothZoomSupport = false;
    maxZoomLevel = MAX_ZOOM_LEVEL_FRONT;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    colorTransformMatrix1 = UNIT_MATRIX_5E2_3X3;
    colorTransformMatrix2 = UNIT_MATRIX_5E2_3X3;
    forwardMatrix1 = UNIT_MATRIX_5E2_3X3;
    forwardMatrix2 = UNIT_MATRIX_5E2_3X3;
    calibration1 = UNIT_MATRIX_5E2_3X3;
    calibration2 = UNIT_MATRIX_5E2_3X3;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 125000000L;
    maxFrameDuration = 500000000L; //125000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 4000;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 1000;
    blackLevelPattern[GR] = 1000;
    blackLevelPattern[GB] = 1000;
    blackLevelPattern[B] = 1000;
    maxAnalogSensitivity = 800;
    orientation = FRONT_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[0] = 64;
    sharpnessMapSize[1] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCamera3SensorS5K5E3Base::ExynosCamera3SensorS5K5E3Base() : ExynosCamera3SensorInfoBase()
{
    maxPreviewW = 1920;
    maxPreviewH = 1080;
    maxPictureW = 2576;
    maxPictureH = 1932;
    maxVideoW = 1920;
    maxVideoH = 1080;
    maxSensorW = 2576;
    maxSensorH = 1932;
    sensorMarginW = 0;
    sensorMarginH = 0;

    minFps = 1;
    maxFps = 30;
    fNumberNum = 190;
    fNumberDen = 100;
    focalLengthNum = 200;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 185;
    apertureDen = 100;

    horizontalViewAngle[SIZE_RATIO_16_9] = 70.3f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 70.3f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 55.8f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;
    focalLengthIn35mmLength = 23;

    maxZoomLevel = MAX_ZOOM_LEVEL_FRONT;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;

    effectList =
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
        ;

    hiddenEffectList =
          EFFECT_AQUA
        ;

    flashModeList =
          FLASH_MODE_OFF
        | FLASH_MODE_AUTO
        | FLASH_MODE_ON
        /*| FLASH_MODE_RED_EYE*/
        | FLASH_MODE_TORCH
        ;

    previewSizeLutMax     = sizeof(PREVIEW_SIZE_LUT_5E3) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax     = sizeof(PICTURE_SIZE_LUT_5E3) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax       = sizeof(VIDEO_SIZE_LUT_5E3) / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax      = sizeof(VTCALL_SIZE_LUT_5E3) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut        = PREVIEW_SIZE_LUT_5E3;
    pictureSizeLut        = PICTURE_SIZE_LUT_5E3;
    videoSizeLut          = VIDEO_SIZE_LUT_5E3;
    dualVideoSizeLut      = VIDEO_SIZE_LUT_5E3;
    vtcallSizeLut         = VTCALL_SIZE_LUT_5E3;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport      = true;

    /* Set the max of preview/picture/video lists */
    frontPreviewListMax      = sizeof(S5K5E3_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontPictureListMax      = sizeof(S5K5E3_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPreviewListMax    = sizeof(S5K5E3_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontPictureListMax    = sizeof(S5K5E3_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax    = sizeof(S5K5E3_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontVideoListMax        = sizeof(S5K5E3_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenFrontVideoListMax    = sizeof(S5K5E3_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    frontFPSListMax        = sizeof(S5K5E3_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenFrontFPSListMax    = sizeof(S5K5E3_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);
    highSpeedVideoListMax = 0;
    highSpeedVideoFPSListMax = 0;

    /* Set supported preview/picture/video lists */
    frontPreviewList     = S5K5E3_YUV_LIST;
    frontPictureList     = S5K5E3_YUV_LIST;
    hiddenFrontPreviewList   = S5K5E3_HIDDEN_PREVIEW_LIST;
    hiddenFrontPictureList   = S5K5E3_HIDDEN_PICTURE_LIST;
    thumbnailList   = S5K5E3_THUMBNAIL_LIST;
    frontVideoList       = S5K5E3_VIDEO_LIST;
    hiddenFrontVideoList   = S5K5E3_HIDDEN_VIDEO_LIST;
    frontFPSList       = S5K5E3_FPS_RANGE_LIST;
    hiddenFrontFPSList   = S5K5E3_HIDDEN_FPS_RANGE_LIST;
    highSpeedVideoList = NULL;
    highSpeedVideoFPSList = NULL;

        /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    lensFacing = ANDROID_LENS_FACING_FRONT;
    switch (supportedHwLevel) {
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
        capabilities = AVAILABLE_CAPABILITIES_LIMITED_BURST;
        requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED_FRONT;
        resultKeys = AVAILABLE_RESULT_KEYS_LIMITED_FRONT;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED_BURST);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED_FRONT);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED_FRONT);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
        capabilities = AVAILABLE_CAPABILITIES_FULL;
        requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
        resultKeys = AVAILABLE_RESULT_KEYS_FULL;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
        capabilities = AVAILABLE_CAPABILITIES_LEGACY;
        requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
        resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
        break;
    default:
        break;
    }
    switch (lensFacing) {
    case ANDROID_LENS_FACING_FRONT:
        aeModes = AVAILABLE_AE_MODES_FRONT;
        afModes = AVAILABLE_AF_MODES_FRONT;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
        break;
    case ANDROID_LENS_FACING_BACK:
        aeModes = AVAILABLE_AE_MODES_BACK;
        afModes = AVAILABLE_AF_MODES_BACK;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
        break;
    default:
        break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationRange[MIN] = -20;
    exposureCompensationRange[MAX] = 20;
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
#endif
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 0;
    max3aRegions[AWB] = 0;
    max3aRegions[AF] = 0;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 2.4f;
    fNumber = 2.4f;
    filterDensity = 0.0f;
    focalLength = 1.86f;
    focalLengthIn35mmLength = 27;
    hyperFocalDistance = 0.0f;
    minimumFocusDistance = 0.0f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 20.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = 0.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 0;
    maxPipelineDepth = 7;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    smoothZoomSupport = false;
    maxZoomLevel = MAX_ZOOM_LEVEL_FRONT;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);
    colorTransformMatrix1 = UNIT_MATRIX_5E3_3X3;
    colorTransformMatrix2 = UNIT_MATRIX_5E3_3X3;
    forwardMatrix1 = UNIT_MATRIX_5E3_3X3;
    forwardMatrix2 = UNIT_MATRIX_5E3_3X3;
    calibration1 = UNIT_MATRIX_5E3_3X3;
    calibration2 = UNIT_MATRIX_5E3_3X3;

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 125000000L;
    maxFrameDuration = 500000000L; //125000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 4000;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 1000;
    blackLevelPattern[GR] = 1000;
    blackLevelPattern[GB] = 1000;
    blackLevelPattern[B] = 1000;
    maxAnalogSensitivity = 800;
    orientation = FRONT_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[0] = 64;
    sharpnessMapSize[1] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCamera3SensorS5K6A3Base::ExynosCamera3SensorS5K6A3Base() : ExynosCamera3SensorInfoBase()
{
#if 0
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
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;

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
#endif
};

ExynosCamera3SensorS5K2P8Base::ExynosCamera3SensorS5K2P8Base() : ExynosCamera3SensorInfoBase()
{
    /* Sensor Max Size Infos */
    maxPreviewW = 5312;
    maxPreviewH = 2988;
    maxPictureW = 5312;
    maxPictureH = 2988;
    maxVideoW = 3840;
    maxVideoH = 2160;
    maxSensorW = 5328;
    maxSensorH = 3000;
    sensorMarginW = 16;
    sensorMarginH = 12;
    sensorMarginBase[LEFT_BASE] = 2;
    sensorMarginBase[TOP_BASE] = 2;
    sensorMarginBase[WIDTH_BASE] = 4;
    sensorMarginBase[HEIGHT_BASE] = 4;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    horizontalViewAngle[SIZE_RATIO_16_9] = 56.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 43.4f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 33.6f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 39.4f;

    minFps = 1;
    maxFps = 30;
    fNumberNum = 22;
    fNumberDen = 10;
    focalLengthNum = 480;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 227;
    apertureDen = 100;

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

    bnsSupport = true;

#if defined(USE_SUBDIVIDED_EV)
    minExposureCompensation = -20;
    maxExposureCompensation = 20;
    exposureCompensationStep = 0.1f;
#else
    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
#endif

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

    if (bnsSupport == true) {
        previewSizeLutMax           = sizeof(YUV_SIZE_LUT_2P8) / (sizeof(int) * SIZE_OF_LUT);
#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2P8_8MP_FULL) / (sizeof(int) * SIZE_OF_LUT);
#else
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2P8_WQHD) / (sizeof(int) * SIZE_OF_LUT);
#endif
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2P8) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P8) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P8) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2P8) / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_2P8)     / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = YUV_SIZE_LUT_2P8;
#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLut                = VIDEO_SIZE_LUT_2P8_8MP_FULL;
#else
        videoSizeLut                = VIDEO_SIZE_LUT_2P8_BDS_WQHD;
#endif
        pictureSizeLut              = PICTURE_SIZE_LUT_2P8;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P8;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P8;
        videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_2P8;
        vtcallSizeLut               = VTCALL_SIZE_LUT_2P8;

        sizeTableSupport            = true;
    } else {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        videoSizeLutHighSpeed60Max  = 0;
        videoSizeLutHighSpeed120Max = 0;
        videoSizeLutHighSpeed240Max = 0;
        vtcallSizeLutMax            = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeBnsLut             = NULL;
        videoSizeLutHighSpeed60     = NULL;
        videoSizeLutHighSpeed120    = NULL;
        videoSizeLutHighSpeed240    = NULL;
        vtcallSizeLut               = NULL;

        sizeTableSupport            = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax          = sizeof(S5K2P8_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax          = sizeof(S5K2P8_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K2P8_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K2P8_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax            = sizeof(S5K2P8_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax            = sizeof(S5K2P8_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax      = sizeof(S5K2P8_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(S5K2P8_HIGH_SPEED_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax              = sizeof(S5K2P8_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax        = sizeof(S5K2P8_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(S5K2P8_HIGH_SPEED_VIDEO_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList             = S5K2P8_YUV_LIST;
    rearPictureList             = S5K2P8_YUV_LIST;
    hiddenRearPreviewList       = S5K2P8_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList       = S5K2P8_HIDDEN_PICTURE_LIST;
    thumbnailList               = S5K2P8_THUMBNAIL_LIST;
    rearVideoList               = S5K2P8_VIDEO_LIST;
    hiddenRearVideoList         = S5K2P8_HIDDEN_VIDEO_LIST;
    highSpeedVideoList          = S5K2P8_HIGH_SPEED_VIDEO_LIST;
    rearFPSList                 = S5K2P8_FPS_RANGE_LIST;
    hiddenRearFPSList           = S5K2P8_HIDDEN_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = S5K2P8_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    lensFacing = ANDROID_LENS_FACING_BACK;
    switch (supportedHwLevel) {
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
            capabilities = AVAILABLE_CAPABILITIES_LIMITED;
            requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED;
            resultKeys = AVAILABLE_RESULT_KEYS_LIMITED;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
            capabilities = AVAILABLE_CAPABILITIES_FULL;
            requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
            resultKeys = AVAILABLE_RESULT_KEYS_FULL;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
            capabilities = AVAILABLE_CAPABILITIES_LEGACY;
            requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
            resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid supported HW level(%d)", __FUNCTION__, __LINE__,
                    supportedHwLevel);
            break;
    }
    switch (lensFacing) {
        case ANDROID_LENS_FACING_FRONT:
            aeModes = AVAILABLE_AE_MODES_FRONT;
            afModes = AVAILABLE_AF_MODES_FRONT;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
            break;
        case ANDROID_LENS_FACING_BACK:
            aeModes = AVAILABLE_AE_MODES_BACK;
            afModes = AVAILABLE_AF_MODES_BACK;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid lens facing info(%d)", __FUNCTION__, __LINE__,
                    lensFacing);
            break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 2.2f;
    fNumber = 2.2f;
    filterDensity = 0.0f;
    focalLength = 4.8f;
    focalLengthIn35mmLength = 31;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    hyperFocalDistance = 1.0f / 5.0f;
    minimumFocusDistance = 1.0f / 0.05f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 0;
    maxPipelineDepth = 5;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    smoothZoomSupport = false;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 4000;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 1000;
    blackLevelPattern[GR] = 1000;
    blackLevelPattern[GB] = 1000;
    blackLevelPattern[B] = 1000;
    maxAnalogSensitivity = 800;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    colorTransformMatrix1 = COLOR_MATRIX1_2P8_3X3;
    colorTransformMatrix2 = COLOR_MATRIX2_2P8_3X3;
    forwardMatrix1 = FORWARD_MATRIX1_2P8_3X3;
    forwardMatrix2 = FORWARD_MATRIX2_2P8_3X3;
    calibration1 = UNIT_MATRIX_2P8_3X3;
    calibration2 = UNIT_MATRIX_2P8_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCamera3SensorS5K6B2Base::ExynosCamera3SensorS5K6B2Base() : ExynosCamera3SensorInfoBase()
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
    sensorMarginBase[LEFT_BASE] = 0;
    sensorMarginBase[TOP_BASE] = 2;
    sensorMarginBase[WIDTH_BASE] = 8;
    sensorMarginBase[HEIGHT_BASE] = 4;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    horizontalViewAngle[SIZE_RATIO_16_9] = 69.7f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 54.2f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 42.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 60.0f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 54.2f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 64.8f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 54.2f;
    verticalViewAngle = 39.4f;

    minFps = 1;
    maxFps = 30;
    fNumberNum = 240;
    fNumberDen = 100;
    focalLengthNum = 186;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 240;
    apertureDen = 100;
    videoSnapshotSupport = true;

    minExposureCompensation = -4;
    maxExposureCompensation = 4;
    exposureCompensationStep = 0.5f;
    maxNumDetectedFaces = 16;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    /* vendor specifics */
    highResolutionCallbackW = 3264;
    highResolutionCallbackH = 1836;
    highSpeedRecording60WFHD = 1920;
    highSpeedRecording60HFHD = 1080;
    highSpeedRecording60W = 1008;
    highSpeedRecording60H = 566;
    highSpeedRecording120W = 1008;
    highSpeedRecording120H = 566;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;
    visionModeSupport = true;
    drcSupport = true;

    bnsSupport = false;

    effectList =
          EFFECT_NONE
        ;

    flashModeList =
          FLASH_MODE_OFF
        ;

    focusModeList =
        FOCUS_MODE_FIXED
        | FOCUS_MODE_INFINITY
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        ;

    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_6B2) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_6B2) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_6B2) / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_6B2) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = 0;
    videoSizeLutHighSpeed120Max = 0;

    previewSizeLut              = PREVIEW_SIZE_LUT_6B2;
    pictureSizeLut              = PICTURE_SIZE_LUT_6B2;
    videoSizeLut                = VIDEO_SIZE_LUT_6B2;
    dualVideoSizeLut            = VIDEO_SIZE_LUT_6B2;
    vtcallSizeLut               = VTCALL_SIZE_LUT_6B2;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;
    sizeTableSupport            = true;

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

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    lensFacing = ANDROID_LENS_FACING_FRONT;
    switch (supportedHwLevel) {
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
        capabilities = AVAILABLE_CAPABILITIES_LIMITED_BURST;
        requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED_FRONT;
        resultKeys = AVAILABLE_RESULT_KEYS_LIMITED_FRONT;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED_BURST);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED_FRONT);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED_FRONT);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
        capabilities = AVAILABLE_CAPABILITIES_FULL;
        requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
        resultKeys = AVAILABLE_RESULT_KEYS_FULL;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
        capabilities = AVAILABLE_CAPABILITIES_LEGACY;
        requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
        resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
        break;
    default:
        break;
    }
    switch (lensFacing) {
    case ANDROID_LENS_FACING_FRONT:
        aeModes = AVAILABLE_AE_MODES_FRONT;
        afModes = AVAILABLE_AF_MODES_FRONT;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
        break;
    case ANDROID_LENS_FACING_BACK:
        aeModes = AVAILABLE_AE_MODES_BACK;
        afModes = AVAILABLE_AF_MODES_BACK;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
        break;
    default:
        break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 0;
    max3aRegions[AWB] = 0;
    max3aRegions[AF] = 0;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 2.4f;
    fNumber = 2.4f;
    filterDensity = 0.0f;
    focalLength = 1.86f;
    focalLengthIn35mmLength = 27;
    hyperFocalDistance = 0.0f;
    minimumFocusDistance = 0.0f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 20.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = 0.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 0;
    maxPipelineDepth = NUM_REQUEST_BLOCK_MAX;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    smoothZoomSupport = false;
    maxZoomLevel = MAX_ZOOM_LEVEL_FRONT;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 250000000L;
    maxFrameDuration = 250000000L;
    sensorPhysicalSize[WIDTH] = 3.495f;
    sensorPhysicalSize[HEIGHT] = 2.626f;
    whiteLevel = 4000;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 1000;
    blackLevelPattern[GR] = 1000;
    blackLevelPattern[GB] = 1000;
    blackLevelPattern[B] = 1000;
    maxAnalogSensitivity = 800;
    orientation = FRONT_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    colorTransformMatrix1 = UNIT_MATRIX_6B2_3X3;
    colorTransformMatrix2 = UNIT_MATRIX_6B2_3X3;
    forwardMatrix1 = UNIT_MATRIX_6B2_3X3;
    forwardMatrix2 = UNIT_MATRIX_6B2_3X3;
    calibration1 = UNIT_MATRIX_6B2_3X3;
    calibration2 = UNIT_MATRIX_6B2_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[0] = 64;
    sharpnessMapSize[1] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCamera3SensorS5K6D1Base::ExynosCamera3SensorS5K6D1Base() : ExynosCamera3SensorInfoBase()
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

    horizontalViewAngle[SIZE_RATIO_16_9] = 79.8f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 65.2f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 50.8f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 71.8f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 65.2f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 74.8f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 65.2f;
    verticalViewAngle = 39.4f;

    /* TODO : Where should we go? */
    minFps = 1;
    maxFps = 30;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    videoSnapshotSupport = true;

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
    visionModeSupport = true;
    drcSupport = true;

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
    dualVideoSizeLut            = DUAL_VIDEO_SIZE_LUT_6D1;
    videoSizeBnsLut             = NULL;
    videoSizeLutHighSpeed60     = NULL;
    videoSizeLutHighSpeed120    = NULL;

    sizeTableSupport      = true;

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

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    lensFacing = ANDROID_LENS_FACING_FRONT;
    switch (supportedHwLevel) {
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
        capabilities = AVAILABLE_CAPABILITIES_LIMITED;
        requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED;
        resultKeys = AVAILABLE_RESULT_KEYS_LIMITED;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
        capabilities = AVAILABLE_CAPABILITIES_FULL;
        requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
        resultKeys = AVAILABLE_RESULT_KEYS_FULL;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
        break;
    case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
        capabilities = AVAILABLE_CAPABILITIES_LEGACY;
        requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
        resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
        characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
        capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
        requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
        resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
        characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
        break;
    default:
        break;
    }
    switch (lensFacing) {
    case ANDROID_LENS_FACING_FRONT:
        aeModes = AVAILABLE_AE_MODES_FRONT;
        afModes = AVAILABLE_AF_MODES_FRONT;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
        break;
    case ANDROID_LENS_FACING_BACK:
        aeModes = AVAILABLE_AE_MODES_BACK;
        afModes = AVAILABLE_AF_MODES_BACK;
        aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
        afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
        break;
    default:
        break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 0;
    max3aRegions[AWB] = 0;
    max3aRegions[AF] = 0;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_FALSE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.9f;
    fNumber = 1.9f;
    filterDensity = 0.0f;
    focalLength = 1.6f;
    focalLengthIn35mmLength = 22;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION;
    hyperFocalDistance = 0.0f;
    minimumFocusDistance = 0.0f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 20.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = 0.0f;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1; //RAW
    maxNumOutputStreams[PROCESSED] = 3; //PROC
    maxNumOutputStreams[PROCESSED_STALL] = 1; //PROC_STALL
    maxNumInputStreams = 0;
    maxPipelineDepth = 5;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    smoothZoomSupport = false;
    maxZoomLevel = MAX_ZOOM_LEVEL_FRONT;
    maxZoomRatio = MAX_ZOOM_RATIO_FRONT;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 125000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 4000;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 1000;
    blackLevelPattern[GR] = 1000;
    blackLevelPattern[GB] = 1000;
    blackLevelPattern[B] = 1000;
    maxAnalogSensitivity = 800;
    orientation = FRONT_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[0] = 64;
    sharpnessMapSize[1] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCamera3SensorS5K8B1Base::ExynosCamera3SensorS5K8B1Base() : ExynosCamera3SensorInfoBase()
{
#if 0
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
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    visionModeSupport = true;

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
#endif
};

ExynosCamera3SensorSR261Base::ExynosCamera3SensorSR261Base() : ExynosCamera3SensorInfoBase()
{

};

ExynosCamera3SensorSR544Base::ExynosCamera3SensorSR544Base() : ExynosCamera3SensorInfoBase()
{

};

ExynosCamera3SensorIMX134Base::ExynosCamera3SensorIMX134Base() : ExynosCamera3SensorInfoBase()
{
#if 0
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
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;

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
#endif
};

ExynosCamera3SensorIMX135Base::ExynosCamera3SensorIMX135Base() : ExynosCamera3SensorInfoBase()
{
#if 0
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
#endif
};

ExynosCamera3SensorIMX175Base::ExynosCamera3SensorIMX175Base() : ExynosCamera3SensorInfoBase()
{
#if 0
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
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;

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
        previewSizeLutMax     = sizeof(PREVIEW_SIZE_LUT_IMX175) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutMax       = sizeof(VIDEO_SIZE_LUT_IMX175)   / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax     = sizeof(PICTURE_SIZE_LUT_IMX175) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX175) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX175) / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut        = PREVIEW_SIZE_LUT_IMX175;
        videoSizeLut          = VIDEO_SIZE_LUT_IMX175;
        videoSizeBnsLut       = NULL;
        pictureSizeLut        = PICTURE_SIZE_LUT_IMX175;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX175;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX175;
        sizeTableSupport      = true;
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
#endif
};

ExynosCamera3SensorIMX219Base::ExynosCamera3SensorIMX219Base() : ExynosCamera3SensorInfoBase()
{

};

#if 0
ExynosCamera3SensorIMX240Base::ExynosCamera3SensorIMX240Base() : ExynosCamera3SensorInfoBase()
{
    /* Sensor Max Size Infos */
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
    sensorMarginBase[LEFT_BASE] = 2;
    sensorMarginBase[TOP_BASE] = 2;
    sensorMarginBase[WIDTH_BASE] = 4;
    sensorMarginBase[HEIGHT_BASE] = 4;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    /* Sensor FOV Infos */
    horizontalViewAngle[SIZE_RATIO_16_9] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 53.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 41.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 41.0f;

    /* TODO : Where should we go? */
    minFps = 1;
    maxFps = 30;
    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 430;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 185;
    apertureDen = 100;

    /* Hal1 info - prevent setparam fail */
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

    /* Hal1 info - prevent setparam fail */
    antiBandingList =
          ANTIBANDING_AUTO
        ;

    flashModeList =
          FLASH_MODE_OFF
        ;

    focusModeList =
        FOCUS_MODE_FIXED
        | FOCUS_MODE_INFINITY
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        ;

    bnsSupport = true;

    if (bnsSupport == true) {
        previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_IMX240_BNS) / (sizeof(int) * SIZE_OF_LUT);
#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX240_8MP_FULL)       / (sizeof(int) * SIZE_OF_LUT);
#else
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX240)       / (sizeof(int) * SIZE_OF_LUT);
#endif
        pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_IMX240)     / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_IMX240_BNS)     / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX240_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX240_BNS) / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = PREVIEW_SIZE_LUT_IMX240_BNS;
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

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED;
    lensFacing = ANDROID_LENS_FACING_BACK;
    switch (supportedHwLevel) {
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
#if 0
            capabilities = AVAILABLE_CAPABILITIES_LIMITED;
            requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED;
            resultKeys = AVAILABLE_RESULT_KEYS_LIMITED;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
#else
            capabilities = AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL;
            requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL;
            resultKeys = AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL);
#endif
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
            capabilities = AVAILABLE_CAPABILITIES_FULL;
            requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
            resultKeys = AVAILABLE_RESULT_KEYS_FULL;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
            capabilities = AVAILABLE_CAPABILITIES_LEGACY;
            requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
            resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid supported HW level(%d)", __FUNCTION__, __LINE__,
                    supportedHwLevel);
            break;
    }
    switch (lensFacing) {
        case ANDROID_LENS_FACING_FRONT:
            aeModes = AVAILABLE_AE_MODES_FRONT;
            afModes = AVAILABLE_AF_MODES_FRONT;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
            break;
        case ANDROID_LENS_FACING_BACK:
            aeModes = AVAILABLE_AE_MODES_BACK;
            afModes = AVAILABLE_AF_MODES_BACK;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid lens facing info(%d)", __FUNCTION__, __LINE__,
                    lensFacing);
            break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.85f;
    fNumber = 1.9f;
    filterDensity = 0.0f;
    focalLength = 4.3f;
    focalLengthIn35mmLength = 28;
    hyperFocalDistance = 1.0f / 5.0f;
    minimumFocusDistance = 1.0f / 0.1f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 0;
    maxPipelineDepth = 5;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    smoothZoomSupport = false;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 100;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 4000;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT;
    blackLevelPattern[R] = 1000;
    blackLevelPattern[GR] = 1000;
    blackLevelPattern[GB] = 1000;
    blackLevelPattern[B] = 1000;
    maxAnalogSensitivity = 800;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};
#endif

ExynosCamera3SensorIMX240_2P2Base::ExynosCamera3SensorIMX240_2P2Base(int sensorId) : ExynosCamera3SensorInfoBase()
{
    /* Sensor Max Size Infos */
    maxPreviewW = 5312;
    maxPreviewH = 2988;
    maxPictureW = 5312;
    maxPictureH = 2988;
    maxVideoW = 3840;
    maxVideoH = 2160;
    maxSensorW = 5328;
    maxSensorH = 3000;
    sensorMarginW = 16;
    sensorMarginH = 12;
    sensorMarginBase[LEFT_BASE] = 2;
    sensorMarginBase[TOP_BASE] = 2;
    sensorMarginBase[WIDTH_BASE] = 4;
    sensorMarginBase[HEIGHT_BASE] = 4;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    /* Sensor FOV Infos */
    horizontalViewAngle[SIZE_RATIO_16_9] = 68.0f;
    horizontalViewAngle[SIZE_RATIO_4_3] = 53.0f;
    horizontalViewAngle[SIZE_RATIO_1_1] = 41.0f;
    horizontalViewAngle[SIZE_RATIO_3_2] = 55.2f;
    horizontalViewAngle[SIZE_RATIO_5_4] = 48.8f;
    horizontalViewAngle[SIZE_RATIO_5_3] = 58.4f;
    horizontalViewAngle[SIZE_RATIO_11_9] = 48.8f;
    verticalViewAngle = 41.0f;

    /* TODO : Where should we go? */
    minFps = 1;
    maxFps = 30;
    fNumberNum = 19;
    fNumberDen = 10;
    focalLengthNum = 430;
    focalLengthDen = 100;
    focusDistanceNum = 0;
    focusDistanceDen = 0;
    apertureNum = 185;
    apertureDen = 100;

    /* Hal1 info - prevent setparam fail */
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

    /* Hal1 info - prevent setparam fail */
    antiBandingList =
          ANTIBANDING_AUTO
        ;

    effectList =
          EFFECT_NONE
        ;

    flashModeList =
          FLASH_MODE_OFF
        ;

    focusModeList =
        FOCUS_MODE_FIXED
        | FOCUS_MODE_INFINITY
        ;

    sceneModeList =
          SCENE_MODE_AUTO
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        ;

    bnsSupport = true;

    if (bnsSupport == true) {
        previewSizeLutMax           = sizeof(YUV_SIZE_LUT_IMX240_2P2) / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax           = sizeof(YUV_SIZE_LUT_IMX240_2P2) / (sizeof(int) * SIZE_OF_LUT);
#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX240_2P2_8MP_FULL)       / (sizeof(int) * SIZE_OF_LUT);
#else
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX240_2P2)       / (sizeof(int) * SIZE_OF_LUT);
#endif
        videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX240_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX240_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);
        vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_IMX240_2P2_BNS) / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = YUV_SIZE_LUT_IMX240_2P2;
        pictureSizeLut              = YUV_SIZE_LUT_IMX240_2P2;
#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLut                = VIDEO_SIZE_LUT_IMX240_2P2_8MP_FULL;
#else
        videoSizeLut                = VIDEO_SIZE_LUT_IMX240_2P2;
#endif
        videoSizeBnsLut             = VIDEO_SIZE_LUT_IMX240_2P2_BNS;
        videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_IMX240_2P2_BNS;
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX240_2P2_BNS;
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
        vtcallSizeLut               = NULL;
        sizeTableSupport            = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax          = sizeof(IMX240_2P2_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax          = sizeof(IMX240_2P2_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(IMX240_2P2_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(IMX240_2P2_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax            = sizeof(IMX240_2P2_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax            = sizeof(IMX240_2P2_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax      = sizeof(IMX240_2P2_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(IMX240_2P2_HIGH_SPEED_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax              = sizeof(IMX240_2P2_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax        = sizeof(IMX240_2P2_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(IMX240_2P2_HIGH_SPEED_VIDEO_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList         = IMX240_2P2_YUV_LIST;
    rearPictureList         = IMX240_2P2_YUV_LIST;
    hiddenRearPreviewList   = IMX240_2P2_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList   = IMX240_2P2_HIDDEN_PICTURE_LIST;
    thumbnailList           = IMX240_2P2_THUMBNAIL_LIST;
    rearVideoList           = IMX240_2P2_VIDEO_LIST;
    hiddenRearVideoList     = IMX240_2P2_HIDDEN_VIDEO_LIST;
    highSpeedVideoList      = IMX240_2P2_HIGH_SPEED_VIDEO_LIST;
    rearFPSList             = IMX240_2P2_FPS_RANGE_LIST;
    hiddenRearFPSList       = IMX240_2P2_HIDDEN_FPS_RANGE_LIST;
    highSpeedVideoFPSList   = IMX240_2P2_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    lensFacing = ANDROID_LENS_FACING_BACK;
    switch (supportedHwLevel) {
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
#if 0
            capabilities = AVAILABLE_CAPABILITIES_LIMITED;
            requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED;
            resultKeys = AVAILABLE_RESULT_KEYS_LIMITED;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
#else
            capabilities = AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL;
            requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL;
            resultKeys = AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL);
#endif
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
            capabilities = AVAILABLE_CAPABILITIES_FULL;
            requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
            resultKeys = AVAILABLE_RESULT_KEYS_FULL;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
            capabilities = AVAILABLE_CAPABILITIES_LEGACY;
            requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
            resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid supported HW level(%d)", __FUNCTION__, __LINE__,
                    supportedHwLevel);
            break;
    }
    switch (lensFacing) {
        case ANDROID_LENS_FACING_FRONT:
            aeModes = AVAILABLE_AE_MODES_FRONT;
            afModes = AVAILABLE_AF_MODES_FRONT;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
            break;
        case ANDROID_LENS_FACING_BACK:
            aeModes = AVAILABLE_AE_MODES_BACK;
            afModes = AVAILABLE_AF_MODES_BACK;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid lens facing info(%d)", __FUNCTION__, __LINE__,
                    lensFacing);
            break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.85f;
    fNumber = 1.9f;
    filterDensity = 0.0f;
    focalLength = 4.3f;
    focalLengthIn35mmLength = 28;
    hyperFocalDistance = 1.0f / 5.0f;
    minimumFocusDistance = 1.0f / 0.1f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 0;
    maxPipelineDepth = NUM_REQUEST_BLOCK_MAX;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    smoothZoomSupport = false;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 50;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 5.967f;
    sensorPhysicalSize[HEIGHT] = 3.360f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    if (sensorId == SENSOR_NAME_IMX240) {
        colorTransformMatrix1 = COLOR_MATRIX1_IMX240_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_IMX240_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_IMX240_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_IMX240_3X3;
    } else {
        colorTransformMatrix1 = COLOR_MATRIX1_2P2_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_2P2_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_2P2_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_2P2_3X3;
    }
    calibration1 = UNIT_MATRIX_IMX240_2P2_3X3;
    calibration2 = UNIT_MATRIX_IMX240_2P2_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCamera3SensorIMX260_2L1Base::ExynosCamera3SensorIMX260_2L1Base(int sensorId) : ExynosCamera3SensorInfoBase()
{
    maxPreviewW = 4032;
    maxPreviewH = 3024;
    maxPictureW = 4032;
    maxPictureH = 3024;
    maxVideoW = 3840;
    maxVideoH = 2160;
    maxSensorW = 4032;
    maxSensorH = 3024;
    sensorMarginW = 0;
    sensorMarginH = 0;
    sensorMarginBase[LEFT_BASE] = 0;
    sensorMarginBase[TOP_BASE] = 0;
    sensorMarginBase[WIDTH_BASE] = 0;
    sensorMarginBase[HEIGHT_BASE] = 0;

    maxThumbnailW = 512;
    maxThumbnailH = 384;

    minFps = 1;
    maxFps = 30;

    exposureCompensationStep = 0.5f;

    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

    bnsSupport = true;

#if defined(USE_SUBDIVIDED_EV)
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationStep = 0.5f;
#endif

    if (bnsSupport == true) {
        previewSizeLutMax           = sizeof(YUV_SIZE_LUT_IMX260_2L1) / (sizeof(int) * SIZE_OF_LUT);
        pictureSizeLutMax           = sizeof(YUV_SIZE_LUT_IMX260_2L1) / (sizeof(int) * SIZE_OF_LUT);
#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX260_2L1_8MP_FULL) / (sizeof(int) * SIZE_OF_LUT);
#else
        videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_IMX260_2L1) / (sizeof(int) * SIZE_OF_LUT);
#endif
        videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX260_2L1) / (sizeof(int) * SIZE_OF_LUT);
        videoSizeLutHighSpeed240Max = sizeof(VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_IMX260_2L1) / (sizeof(int) * SIZE_OF_LUT);
        fastAeStableLutMax          = sizeof(FAST_AE_STABLE_SIZE_LUT_IMX260_2L1) / (sizeof(int) * SIZE_OF_LUT);
        depthMapSizeLutMax          = sizeof(DEPTH_MAP_SIZE_LUT_IMX260_2L1) / (sizeof(int) * SIZE_OF_RESOLUTION);
        vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_IMX260_2L1)     / (sizeof(int) * SIZE_OF_LUT);

        previewSizeLut              = YUV_SIZE_LUT_IMX260_2L1;
        pictureSizeLut              = YUV_SIZE_LUT_IMX260_2L1;
#ifdef ENABLE_8MP_FULL_FRAME
        videoSizeLut                = VIDEO_SIZE_LUT_IMX260_2L1_8MP_FULL;
#else
        videoSizeLut                = VIDEO_SIZE_LUT_IMX260_2L1;
#endif
        videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_IMX260_2L1;
        videoSizeLutHighSpeed240    = VIDEO_SIZE_LUT_240FPS_HIGH_SPEED_IMX260_2L1;
        depthMapSizeLut             = DEPTH_MAP_SIZE_LUT_IMX260_2L1;
        vtcallSizeLut               = VTCALL_SIZE_LUT_IMX260_2L1;
        fastAeStableLut             = FAST_AE_STABLE_SIZE_LUT_IMX260_2L1;
        sizeTableSupport            = true;
    } else {
        previewSizeLutMax           = 0;
        pictureSizeLutMax           = 0;
        videoSizeLutMax             = 0;
        videoSizeLutHighSpeed120Max = 0;
        videoSizeLutHighSpeed240Max = 0;
        vtcallSizeLutMax            = 0;
        fastAeStableLutMax          = 0;

        previewSizeLut              = NULL;
        pictureSizeLut              = NULL;
        videoSizeLut                = NULL;
        videoSizeBnsLut             = NULL;
        videoSizeLutHighSpeed120    = NULL;
        videoSizeLutHighSpeed240    = NULL;
        vtcallSizeLut               = NULL;
        fastAeStableLut             = NULL;

        sizeTableSupport            = false;
    }

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax          = sizeof(IMX260_2L1_YUV_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax          = sizeof(IMX260_2L1_JPEG_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(IMX260_2L1_HIDDEN_PREVIEW_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(IMX260_2L1_HIDDEN_PICTURE_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax            = sizeof(IMX260_2L1_THUMBNAIL_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax            = sizeof(IMX260_2L1_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax      = sizeof(IMX260_2L1_HIDDEN_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    highSpeedVideoListMax       = sizeof(IMX260_2L1_HIGH_SPEED_VIDEO_LIST) / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax              = sizeof(IMX260_2L1_FPS_RANGE_LIST) / (sizeof(int) * 2);
    hiddenRearFPSListMax        = sizeof(IMX260_2L1_HIDDEN_FPS_RANGE_LIST) / (sizeof(int) * 2);
    highSpeedVideoFPSListMax    = sizeof(IMX260_2L1_HIGH_SPEED_VIDEO_FPS_RANGE_LIST) / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList             = IMX260_2L1_YUV_LIST;
    rearPictureList             = IMX260_2L1_JPEG_LIST;
    hiddenRearPreviewList       = IMX260_2L1_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList       = IMX260_2L1_HIDDEN_PICTURE_LIST;
    thumbnailList               = IMX260_2L1_THUMBNAIL_LIST;
    rearVideoList               = IMX260_2L1_VIDEO_LIST;
    hiddenRearVideoList         = IMX260_2L1_HIDDEN_VIDEO_LIST;
    highSpeedVideoList          = IMX260_2L1_HIGH_SPEED_VIDEO_LIST;
    rearFPSList                 = IMX260_2L1_FPS_RANGE_LIST;
    hiddenRearFPSList           = IMX260_2L1_HIDDEN_FPS_RANGE_LIST;
    highSpeedVideoFPSList       = IMX260_2L1_HIGH_SPEED_VIDEO_FPS_RANGE_LIST;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    lensFacing = ANDROID_LENS_FACING_BACK;
    switch (supportedHwLevel) {
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
#if 0
            capabilities = AVAILABLE_CAPABILITIES_LIMITED;
            requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED;
            resultKeys = AVAILABLE_RESULT_KEYS_LIMITED;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
#else
            capabilities = AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL;
            requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL;
            resultKeys = AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL);
#endif
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
            capabilities = AVAILABLE_CAPABILITIES_FULL;
            requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
            resultKeys = AVAILABLE_RESULT_KEYS_FULL;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
            capabilities = AVAILABLE_CAPABILITIES_LEGACY;
            requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
            resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid supported HW level(%d)", __FUNCTION__, __LINE__,
                    supportedHwLevel);
            break;
    }
    switch (lensFacing) {
        case ANDROID_LENS_FACING_FRONT:
            aeModes = AVAILABLE_AE_MODES_FRONT;
            afModes = AVAILABLE_AF_MODES_FRONT;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
            break;
        case ANDROID_LENS_FACING_BACK:
            aeModes = AVAILABLE_AE_MODES_BACK;
            afModes = AVAILABLE_AF_MODES_BACK;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid lens facing info(%d)", __FUNCTION__, __LINE__,
                    lensFacing);
            break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 1.70f;
    fNumber = 1.7f;
    filterDensity = 0.0f;
    focalLength = 4.2f;
    focalLengthIn35mmLength = 26;
    hyperFocalDistance = 1.0f / 3.6f;
    minimumFocusDistance = 1.0f / 0.1f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 0;
    maxPipelineDepth = NUM_REQUEST_BLOCK_MAX;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 64;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 22000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 3.20f;
    sensorPhysicalSize[HEIGHT] = 2.40f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    if (sensorId == SENSOR_NAME_IMX260) {
        colorTransformMatrix1 = COLOR_MATRIX1_IMX260_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_IMX260_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_IMX260_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_IMX260_3X3;
    } else {
        colorTransformMatrix1 = COLOR_MATRIX1_2L1_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_2L1_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_2L1_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_2L1_3X3;
    }
    calibration1 = UNIT_MATRIX_IMX260_2L1_3X3;
    calibration2 = UNIT_MATRIX_IMX260_2L1_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCamera3SensorS5K3P3Base::ExynosCamera3SensorS5K3P3Base(int sensorId) : ExynosCamera3SensorInfoBase()
{
    maxPreviewW = 4608;
    maxPreviewH = 3456;
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

    focalLengthIn35mmLength = 31;

    minFps = 1;
    maxFps = 30;

#ifdef USE_SUBDIVIDED_EV
    exposureCompensationStep = 0.1f;
#else
    exposureCompensationStep = 0.5f;
#endif
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    videoSnapshotSupport = true;
    videoStabilizationSupport = false;
    autoWhiteBalanceLockSupport = true;
    autoExposureLockSupport = true;

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

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    supportedHwLevel = ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL;
    lensFacing = ANDROID_LENS_FACING_BACK;
    switch (supportedHwLevel) {
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED:
#if 0
            capabilities = AVAILABLE_CAPABILITIES_LIMITED;
            requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED;
            resultKeys = AVAILABLE_RESULT_KEYS_LIMITED;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED);
#else
            capabilities = AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL;
            requestKeys = AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL;
            resultKeys = AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LIMITED_OPTIONAL);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LIMITED_OPTIONAL);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LIMITED_OPTIONAL);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LIMITED_OPTIONAL);
#endif
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL:
            capabilities = AVAILABLE_CAPABILITIES_FULL;
            requestKeys = AVAILABLE_REQUEST_KEYS_FULL;
            resultKeys = AVAILABLE_RESULT_KEYS_FULL;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_FULL;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_FULL);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_FULL);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_FULL);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_FULL);
            break;
        case ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY:
            capabilities = AVAILABLE_CAPABILITIES_LEGACY;
            requestKeys = AVAILABLE_REQUEST_KEYS_LEGACY;
            resultKeys = AVAILABLE_RESULT_KEYS_LEGACY;
            characteristicsKeys = AVAILABLE_CHARACTERISTICS_KEYS_LEGACY;
            capabilitiesLength = ARRAY_LENGTH(AVAILABLE_CAPABILITIES_LEGACY);
            requestKeysLength = ARRAY_LENGTH(AVAILABLE_REQUEST_KEYS_LEGACY);
            resultKeysLength = ARRAY_LENGTH(AVAILABLE_RESULT_KEYS_LEGACY);
            characteristicsKeysLength = ARRAY_LENGTH(AVAILABLE_CHARACTERISTICS_KEYS_LEGACY);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid supported HW level(%d)", __FUNCTION__, __LINE__,
                    supportedHwLevel);
            break;
    }
    switch (lensFacing) {
        case ANDROID_LENS_FACING_FRONT:
            aeModes = AVAILABLE_AE_MODES_FRONT;
            afModes = AVAILABLE_AF_MODES_FRONT;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_FRONT);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_FRONT);
            break;
        case ANDROID_LENS_FACING_BACK:
            aeModes = AVAILABLE_AE_MODES_BACK;
            afModes = AVAILABLE_AF_MODES_BACK;
            aeModesLength = ARRAY_LENGTH(AVAILABLE_AE_MODES_BACK);
            afModesLength = ARRAY_LENGTH(AVAILABLE_AF_MODES_BACK);
            break;
        default:
            ALOGE("ERR(%s[%d]):Invalid lens facing info(%d)", __FUNCTION__, __LINE__,
                    lensFacing);
            break;
    }

    /* Android ColorCorrection Static Metadata */
    colorAberrationModes = AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES;
    colorAberrationModesLength = ARRAY_LENGTH(AVAILABLE_COLOR_CORRECTION_ABERRATION_MODES);

    /* Android Control Static Metadata */
    antiBandingModes = AVAILABLE_ANTIBANDING_MODES;
    exposureCompensationRange[MIN] = -4;
    exposureCompensationRange[MAX] = 4;
    exposureCompensationStep = 0.5f;
    effectModes = AVAILABLE_EFFECT_MODES;
    sceneModes = AVAILABLE_SCENE_MODES;
    videoStabilizationModes = AVAILABLE_VIDEO_STABILIZATION_MODES;
    awbModes = AVAILABLE_AWB_MODES;
    controlModes = AVAILABLE_CONTROL_MODES;
    controlModesLength = ARRAY_LENGTH(AVAILABLE_CONTROL_MODES);
    max3aRegions[AE] = 1;
    max3aRegions[AWB] = 1;
    max3aRegions[AF] = 1;
    sceneModeOverrides = SCENE_MODE_OVERRIDES;
    aeLockAvailable = ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE;
    awbLockAvailable = ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE;
    antiBandingModesLength = ARRAY_LENGTH(AVAILABLE_ANTIBANDING_MODES);
    effectModesLength = ARRAY_LENGTH(AVAILABLE_EFFECT_MODES);
    sceneModesLength = ARRAY_LENGTH(AVAILABLE_SCENE_MODES);
    videoStabilizationModesLength = ARRAY_LENGTH(AVAILABLE_VIDEO_STABILIZATION_MODES);
    awbModesLength = ARRAY_LENGTH(AVAILABLE_AWB_MODES);
    sceneModeOverridesLength = ARRAY_LENGTH(SCENE_MODE_OVERRIDES);

    /* Android Edge Static Metadata */
    edgeModes = AVAILABLE_EDGE_MODES;
    edgeModesLength = ARRAY_LENGTH(AVAILABLE_EDGE_MODES);

    /* Android Flash Static Metadata */
    flashAvailable = ANDROID_FLASH_INFO_AVAILABLE_TRUE;
    chargeDuration = 0L;
    colorTemperature = 0;
    maxEnergy = 0;

    /* Android Hot Pixel Static Metadata */
    hotPixelModes = AVAILABLE_HOT_PIXEL_MODES;
    hotPixelModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MODES);

    /* Android Lens Static Metadata */
    aperture = 2.27;
    fNumber = 2.2f;
    filterDensity = 0.0f;
    focalLength = 4.8f;
    focalLengthIn35mmLength = 31;
    hyperFocalDistance = 1.0f / 5.0f;
    minimumFocusDistance = 1.0f / 0.1f;
    shadingMapSize[WIDTH] = 1;
    shadingMapSize[HEIGHT] = 1;
    focusDistanceCalibration = ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED;
    opticalAxisAngle[0] = 0.0f;
    opticalAxisAngle[1] = 0.0f;
    lensPosition[X_3D] = 0.0f;
    lensPosition[Y_3D] = 20.0f;
    lensPosition[Z_3D] = -5.0f;
    opticalStabilization = AVAILABLE_OPTICAL_STABILIZATION_BACK;
    opticalStabilizationLength = ARRAY_LENGTH(AVAILABLE_OPTICAL_STABILIZATION_BACK);

    /* Android Noise Reduction Static Metadata */
    noiseReductionModes = AVAILABLE_NOISE_REDUCTION_MODES;
    noiseReductionModesLength = ARRAY_LENGTH(AVAILABLE_NOISE_REDUCTION_MODES);

    /* Android Request Static Metadata */
    maxNumOutputStreams[RAW] = 1;
    maxNumOutputStreams[PROCESSED] = 3;
    maxNumOutputStreams[PROCESSED_STALL] = 1;
    maxNumInputStreams = 0;
    maxPipelineDepth = NUM_REQUEST_BLOCK_MAX;
    partialResultCount = 1;

    /* Android Scaler Static Metadata */
    zoomSupport = true;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;
    stallDurations = AVAILABLE_STALL_DURATIONS;
    croppingType = ANDROID_SCALER_CROPPING_TYPE_FREEFORM;
    stallDurationsLength = ARRAY_LENGTH(AVAILABLE_STALL_DURATIONS);

    /* Android Sensor Static Metadata */
    sensitivityRange[MIN] = 50;
    sensitivityRange[MAX] = 1600;
    colorFilterArrangement = ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG;
    exposureTimeRange[MIN] = 14000L;
    exposureTimeRange[MAX] = 100000000L;
    maxFrameDuration = 125000000L;
    sensorPhysicalSize[WIDTH] = 5.967f;
    sensorPhysicalSize[HEIGHT] = 3.360f;
    whiteLevel = 1023;
    timestampSource = ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME;
    referenceIlluminant1 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65;
    referenceIlluminant2 = ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A;
    blackLevelPattern[R] = 0;
    blackLevelPattern[GR] = 0;
    blackLevelPattern[GB] = 0;
    blackLevelPattern[B] = 0;
    maxAnalogSensitivity = 640;
    orientation = BACK_ROTATION;
    profileHueSatMapDimensions[HUE] = 1;
    profileHueSatMapDimensions[SATURATION] = 2;
    profileHueSatMapDimensions[VALUE] = 1;
    testPatternModes = AVAILABLE_TEST_PATTERN_MODES;
    testPatternModesLength = ARRAY_LENGTH(AVAILABLE_TEST_PATTERN_MODES);
    if (sensorId == SENSOR_NAME_IMX240) {
        colorTransformMatrix1 = COLOR_MATRIX1_IMX240_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_IMX240_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_IMX240_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_IMX240_3X3;
    } else {
        colorTransformMatrix1 = COLOR_MATRIX1_2P2_3X3;
        colorTransformMatrix2 = COLOR_MATRIX2_2P2_3X3;
        forwardMatrix1 = FORWARD_MATRIX1_2P2_3X3;
        forwardMatrix2 = FORWARD_MATRIX2_2P2_3X3;
    }
    calibration1 = UNIT_MATRIX_IMX240_2P2_3X3;
    calibration2 = UNIT_MATRIX_IMX240_2P2_3X3;

    /* Android Statistics Static Metadata */
    faceDetectModes = AVAILABLE_FACE_DETECT_MODES;
    faceDetectModesLength = ARRAY_LENGTH(AVAILABLE_FACE_DETECT_MODES);
    histogramBucketCount = 64;
    maxNumDetectedFaces = 16;
    maxHistogramCount = 1000;
    maxSharpnessMapValue = 1000;
    sharpnessMapSize[WIDTH] = 64;
    sharpnessMapSize[HEIGHT] = 64;
    hotPixelMapModes = AVAILABLE_HOT_PIXEL_MAP_MODES;
    hotPixelMapModesLength = ARRAY_LENGTH(AVAILABLE_HOT_PIXEL_MAP_MODES);
    lensShadingMapModes = AVAILABLE_LENS_SHADING_MAP_MODES;
    lensShadingMapModesLength = ARRAY_LENGTH(AVAILABLE_LENS_SHADING_MAP_MODES);
    shadingAvailableModes = SHADING_AVAILABLE_MODES;
    shadingAvailableModesLength = ARRAY_LENGTH(SHADING_AVAILABLE_MODES);

    /* Android Tone Map Static Metadata */
    tonemapCurvePoints = 128;
    toneMapModes = AVAILABLE_TONE_MAP_MODES;
    toneMapModesLength = ARRAY_LENGTH(AVAILABLE_TONE_MAP_MODES);

    /* Android LED Static Metadata */
    leds = AVAILABLE_LEDS;
    ledsLength = ARRAY_LENGTH(AVAILABLE_LEDS);

    /* Android Sync Static Metadata */
    maxLatency = ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL; //0
    /* END of Camera HAL 3.2 Static Metadatas */
};

ExynosCamera3SensorOV5670Base::ExynosCamera3SensorOV5670Base() : ExynosCamera3SensorInfoBase()
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
        ;

    hiddenEffectList =
          EFFECT_AQUA
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

    previewSizeLutMax     = sizeof(PREVIEW_SIZE_LUT_OV5670) / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax     = sizeof(PICTURE_SIZE_LUT_OV5670) / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax       = sizeof(VIDEO_SIZE_LUT_OV5670) / (sizeof(int) * SIZE_OF_LUT);
    previewSizeLut        = PREVIEW_SIZE_LUT_OV5670;
    pictureSizeLut        = PICTURE_SIZE_LUT_OV5670;
    videoSizeLut          = VIDEO_SIZE_LUT_OV5670;
    dualVideoSizeLut      = VIDEO_SIZE_LUT_OV5670;
    videoSizeBnsLut       = NULL;
    sizeTableSupport      = true;

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
