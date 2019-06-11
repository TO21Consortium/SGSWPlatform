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

/*#define LOG_NDEBUG 0 */
#include "ExynosCameraSensorInfoBase.h"

namespace android {

ExynosSensorS5K2P8Base::ExynosSensorS5K2P8Base()
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
    minExposureTime = 32;
    maxExposureTime = 10000000;
    minWBK = 2300;
    maxWBK = 10000;
    maxNumDetectedFaces = 16;
    maxNumFocusAreas = 2;
    maxNumMeteringAreas = 0;
    maxBasicZoomLevel = MAX_BASIC_ZOOM_LEVEL;
    maxZoomLevel = MAX_ZOOM_LEVEL;
    maxZoomRatio = MAX_ZOOM_RATIO;

    zoomSupport = true;
    smoothZoomSupport = false;
    videoSnapshotSupport = true;
    videoStabilizationSupport = true;
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
        | EFFECT_SOLARIZE
        | EFFECT_SEPIA
        | EFFECT_POSTERIZE
        | EFFECT_WHITEBOARD
        | EFFECT_BLACKBOARD
        | EFFECT_AQUA
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
        | SCENE_MODE_PARTY
        | SCENE_MODE_SPORTS
        | SCENE_MODE_CANDLELIGHT
        ;

    whiteBalanceList =
          WHITE_BALANCE_AUTO
        | WHITE_BALANCE_INCANDESCENT
        | WHITE_BALANCE_FLUORESCENT
        | WHITE_BALANCE_WARM_FLUORESCENT
        | WHITE_BALANCE_DAYLIGHT
        | WHITE_BALANCE_CLOUDY_DAYLIGHT
        | WHITE_BALANCE_TWILIGHT
        | WHITE_BALANCE_SHADE
        | WHITE_BALANCE_CUSTOM_K
        ;

    /* Set the max of preview/picture/video/vtcall LUT */
    previewSizeLutMax           = sizeof(PREVIEW_SIZE_LUT_2P8)                  / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutMax             = sizeof(VIDEO_SIZE_LUT_2P8_WQHD)               / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed60Max  = sizeof(VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P8)   / (sizeof(int) * SIZE_OF_LUT);
    videoSizeLutHighSpeed120Max = sizeof(VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P8)  / (sizeof(int) * SIZE_OF_LUT);
    pictureSizeLutMax           = sizeof(PICTURE_SIZE_LUT_2P8)                  / (sizeof(int) * SIZE_OF_LUT);
    vtcallSizeLutMax            = sizeof(VTCALL_SIZE_LUT_2P8)                   / (sizeof(int) * SIZE_OF_LUT);

    /* Set preview/picture/video/vtcall LUT */
    previewSizeLut              = PREVIEW_SIZE_LUT_2P8;

#if defined(USE_BNS_DUAL_PREVIEW)
#if defined(DUAL_BNS_RATIO) && (DUAL_BNS_RATIO == 1500)
    dualPreviewSizeLut    = PREVIEW_SIZE_LUT_2P8_BDS_BNS15;
#else
    dualPreviewSizeLut    = PREVIEW_SIZE_LUT_2P8_BDS_BNS20_FHD;
#endif
#else
    dualPreviewSizeLut    = PREVIEW_SIZE_LUT_2P8_BDS;
#endif // USE_BNS_DUAL_PREVIEW

#if defined(USE_BNS_DUAL_RECORDING)
#if defined(DUAL_BNS_RATIO) && (DUAL_BNS_RATIO == 1500)
    dualVideoSizeLut      = PREVIEW_SIZE_LUT_2P8_BDS_BNS15;
#else
    dualVideoSizeLut      = PREVIEW_SIZE_LUT_2P8_BDS_BNS20_FHD;
#endif
#else
    dualVideoSizeLut      = PREVIEW_SIZE_LUT_2P8_BDS;
#endif //  USE_BNS_DUAL_RECORDING

    videoSizeLut                = VIDEO_SIZE_LUT_2P8_WQHD;
    videoSizeBnsLut             = VIDEO_SIZE_LUT_2P8_BDS_BNS15_WQHD;
    pictureSizeLut              = PICTURE_SIZE_LUT_2P8;
    videoSizeLutHighSpeed       = VIDEO_SIZE_LUT_HIGH_SPEED_2P8;
    videoSizeLutHighSpeed60     = VIDEO_SIZE_LUT_60FPS_HIGH_SPEED_2P8;
    videoSizeLutHighSpeed120    = VIDEO_SIZE_LUT_120FPS_HIGH_SPEED_2P8;
    vtcallSizeLut               = VTCALL_SIZE_LUT_2P8;
    sizeTableSupport            = true;

    /* Set the max of preview/picture/video lists */
    rearPreviewListMax          = sizeof(S5K2P8_PREVIEW_LIST)           / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearPictureListMax          = sizeof(S5K2P8_PICTURE_LIST)           / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPreviewListMax    = sizeof(S5K2P8_HIDDEN_PREVIEW_LIST)    / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearPictureListMax    = sizeof(S5K2P8_HIDDEN_PICTURE_LIST)    / (sizeof(int) * SIZE_OF_RESOLUTION);
    thumbnailListMax            = sizeof(S5K2P8_THUMBNAIL_LIST)         / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearVideoListMax            = sizeof(S5K2P8_VIDEO_LIST)             / (sizeof(int) * SIZE_OF_RESOLUTION);
    hiddenRearVideoListMax      = sizeof(S5K2P8_HIDDEN_VIDEO_LIST)      / (sizeof(int) * SIZE_OF_RESOLUTION);
    rearFPSListMax              = sizeof(S5K2P8_FPS_RANGE_LIST)         / (sizeof(int) * 2);
    hiddenRearFPSListMax        = sizeof(S5K2P8_HIDDEN_FPS_RANGE_LIST)  / (sizeof(int) * 2);

    /* Set supported preview/picture/video lists */
    rearPreviewList             = S5K2P8_PREVIEW_LIST;
    rearPictureList             = S5K2P8_PICTURE_LIST;
    hiddenRearPreviewList       = S5K2P8_HIDDEN_PREVIEW_LIST;
    hiddenRearPictureList       = S5K2P8_HIDDEN_PICTURE_LIST;
    thumbnailList               = S5K2P8_THUMBNAIL_LIST;
    rearVideoList               = S5K2P8_VIDEO_LIST;
    hiddenRearVideoList         = S5K2P8_HIDDEN_VIDEO_LIST;
    rearFPSList                 = S5K2P8_FPS_RANGE_LIST;
    hiddenRearFPSList           = S5K2P8_HIDDEN_FPS_RANGE_LIST;

#if 0
    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */

    /* lensFacing, supportedHwLevel are keys for selecting some availability table below */
    focusDistanceNum = 0;
    focusDistanceDen = 0;

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
#endif
};
}; /* namespace android */
