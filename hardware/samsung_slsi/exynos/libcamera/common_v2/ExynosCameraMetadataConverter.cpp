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

#include "ExynosCameraMetadataConverter.h"
#include "ExynosCameraRequestManager.h"

namespace android {
#define SET_BIT(x)      (1 << x)

ExynosCamera3MetadataConverter::ExynosCamera3MetadataConverter(int cameraId, ExynosCameraParameters *parameters)
{
    ExynosCameraActivityControl *activityControl = NULL;

    m_cameraId = cameraId;
    m_parameters = parameters;
    activityControl = m_parameters->getActivityControl();
    m_flashMgr = activityControl->getFlashMgr();
    m_sensorStaticInfo = m_parameters->getSensorStaticInfo();
    m_preCaptureTriggerOn = false;
    m_isManualAeControl = false;

    m_blackLevelLockOn = false;
    m_faceDetectModeOn = false;

    m_lockVendorIsoValue = 0;
    m_lockExposureTime = 0;

    m_afMode = AA_AFMODE_CONTINUOUS_PICTURE;
    m_preAfMode = AA_AFMODE_CONTINUOUS_PICTURE;

    m_focusDistance = -1;

    m_maxFps = 30;
    m_overrideFlashControl= false;
    memset(m_gpsProcessingMethod, 0x00, sizeof(m_gpsProcessingMethod));
}

ExynosCamera3MetadataConverter::~ExynosCamera3MetadataConverter()
{
    m_defaultRequestSetting.release();
}

status_t ExynosCamera3MetadataConverter::constructDefaultRequestSettings(int type, camera_metadata_t **request)
{
    Mutex::Autolock l(m_requestLock);

    ALOGD("DEBUG(%s[%d]):Type(%d), cameraId(%d)", __FUNCTION__, __LINE__, type, m_cameraId);

    CameraMetadata settings;
    m_preExposureTime = 0;
    const int64_t USEC = 1000LL;
    const int64_t MSEC = USEC * 1000LL;
    const int64_t SEC = MSEC * 1000LL;
    /** android.request */
    /* request type */
    const uint8_t requestType = ANDROID_REQUEST_TYPE_CAPTURE;
    settings.update(ANDROID_REQUEST_TYPE, &requestType, 1);

    /* meta data mode */
    const uint8_t metadataMode = ANDROID_REQUEST_METADATA_MODE_FULL;
    settings.update(ANDROID_REQUEST_METADATA_MODE, &metadataMode, 1);

    /* id */
    const int32_t id = 0;
    settings.update(ANDROID_REQUEST_ID, &id, 1);

    /* frame count */
    const int32_t frameCount = 0;
    settings.update(ANDROID_REQUEST_FRAME_COUNT, &frameCount, 1);

    /** android.control */
    /* control intent */
    uint8_t controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
    switch (type) {
    case CAMERA3_TEMPLATE_PREVIEW:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
        ALOGD("DEBUG(%s[%d]):type is ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW", __FUNCTION__, __LINE__);
        break;
    case CAMERA3_TEMPLATE_STILL_CAPTURE:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
        ALOGD("DEBUG(%s[%d]):type is CAMERA3_TEMPLATE_STILL_CAPTURE", __FUNCTION__, __LINE__);
        break;
    case CAMERA3_TEMPLATE_VIDEO_RECORD:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;
        ALOGD("DEBUG(%s[%d]):type is CAMERA3_TEMPLATE_VIDEO_RECORD", __FUNCTION__, __LINE__);
        break;
    case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT;
        ALOGD("DEBUG(%s[%d]):type is CAMERA3_TEMPLATE_VIDEO_SNAPSHOT", __FUNCTION__, __LINE__);
        break;
    case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG;
        ALOGD("DEBUG(%s[%d]):type is CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG", __FUNCTION__, __LINE__);
        break;
    case CAMERA3_TEMPLATE_MANUAL:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_MANUAL;
        ALOGD("DEBUG(%s[%d]):type is CAMERA3_TEMPLATE_MANUAL", __FUNCTION__, __LINE__);
        break;
    default:
        ALOGD("ERR(%s[%d]):Custom intent type is selected for setting control intent(%d)", __FUNCTION__, __LINE__, type);
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
    break;
    }
    settings.update(ANDROID_CONTROL_CAPTURE_INTENT, &controlIntent, 1);

    /* 3AA control */
    uint8_t controlMode = ANDROID_CONTROL_MODE_OFF;
    uint8_t afMode = ANDROID_CONTROL_AF_MODE_OFF;
    uint8_t aeMode = ANDROID_CONTROL_AE_MODE_OFF;
    uint8_t awbMode = ANDROID_CONTROL_AWB_MODE_OFF;
    int32_t aeTargetFpsRange[2] = {15, 30};
    switch (type) {
    case CAMERA3_TEMPLATE_PREVIEW:
        controlMode = ANDROID_CONTROL_MODE_AUTO;
        if (m_cameraId == CAMERA_ID_BACK)
            afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        aeMode = ANDROID_CONTROL_AE_MODE_ON;
        awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
        break;
    case CAMERA3_TEMPLATE_STILL_CAPTURE:
        controlMode = ANDROID_CONTROL_MODE_AUTO;
        if (m_cameraId == CAMERA_ID_BACK)
            afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        aeMode = ANDROID_CONTROL_AE_MODE_ON;
        awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
        break;
    case CAMERA3_TEMPLATE_VIDEO_RECORD:
        controlMode = ANDROID_CONTROL_MODE_AUTO;
        if (m_cameraId == CAMERA_ID_BACK)
            afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
        aeMode = ANDROID_CONTROL_AE_MODE_ON;
        awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
        /* Fix FPS for Recording */
        aeTargetFpsRange[0] = 30;
        aeTargetFpsRange[1] = 30;
        break;
    case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        controlMode = ANDROID_CONTROL_MODE_AUTO;
        if (m_cameraId == CAMERA_ID_BACK)
            afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO;
        aeMode = ANDROID_CONTROL_AE_MODE_ON;
        awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
        break;
    case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
        controlMode = ANDROID_CONTROL_MODE_AUTO;
        if (m_cameraId == CAMERA_ID_BACK)
            afMode = ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE;
        aeMode = ANDROID_CONTROL_AE_MODE_ON;
        awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
        break;
    case CAMERA3_TEMPLATE_MANUAL:
        controlMode = ANDROID_CONTROL_MODE_OFF;
        afMode = ANDROID_CONTROL_AF_MODE_OFF;
        aeMode = ANDROID_CONTROL_AE_MODE_OFF;
        awbMode = ANDROID_CONTROL_AWB_MODE_OFF;
        break;
    default:
        ALOGD("ERR(%s[%d]):Custom intent type is selected for setting 3AA control(%d)", __FUNCTION__, __LINE__, type);
        break;
    }
    settings.update(ANDROID_CONTROL_MODE, &controlMode, 1);
    settings.update(ANDROID_CONTROL_AF_MODE, &afMode, 1);
    settings.update(ANDROID_CONTROL_AE_MODE, &aeMode, 1);
    settings.update(ANDROID_CONTROL_AWB_MODE, &awbMode, 1);
    settings.update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE, aeTargetFpsRange, 2);

    const uint8_t afTrigger = ANDROID_CONTROL_AF_TRIGGER_IDLE;
    settings.update(ANDROID_CONTROL_AF_TRIGGER, &afTrigger, 1);

    const uint8_t aePrecaptureTrigger = ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE;
    settings.update(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER, &aePrecaptureTrigger, 1);

    /* effect mode */
    const uint8_t effectMode = ANDROID_CONTROL_EFFECT_MODE_OFF;
    settings.update(ANDROID_CONTROL_EFFECT_MODE, &effectMode, 1);

    /* scene mode */
    const uint8_t sceneMode = ANDROID_CONTROL_SCENE_MODE_DISABLED;
    settings.update(ANDROID_CONTROL_SCENE_MODE, &sceneMode, 1);

    /* ae lock mode */
    const uint8_t aeLock = ANDROID_CONTROL_AE_LOCK_OFF;
    settings.update(ANDROID_CONTROL_AE_LOCK, &aeLock, 1);

    /* ae region */
    int w,h;
    m_parameters->getMaxSensorSize(&w, &h);
    const int32_t controlRegions[5] = {
        0, 0, w, h, 0
    };
    if (m_cameraId == CAMERA_ID_BACK) {
        settings.update(ANDROID_CONTROL_AE_REGIONS, controlRegions, 5);
        settings.update(ANDROID_CONTROL_AWB_REGIONS, controlRegions, 5);
        settings.update(ANDROID_CONTROL_AF_REGIONS, controlRegions, 5);
    }

    /* exposure compensation */
    const int32_t aeExpCompensation = 0;
    settings.update(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &aeExpCompensation, 1);

    /* anti-banding mode */
    const uint8_t aeAntibandingMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
    settings.update(ANDROID_CONTROL_AE_ANTIBANDING_MODE, &aeAntibandingMode, 1);

    /* awb lock */
    const uint8_t awbLock = ANDROID_CONTROL_AWB_LOCK_OFF;
    settings.update(ANDROID_CONTROL_AWB_LOCK, &awbLock, 1);

    /* video stabilization mode */
    const uint8_t vstabMode = ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF;
    settings.update(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &vstabMode, 1);

    /** android.lens */
    const float focusDistance = -1.0f;
    settings.update(ANDROID_LENS_FOCUS_DISTANCE, &focusDistance, 1);
    settings.update(ANDROID_LENS_FOCAL_LENGTH, &(m_sensorStaticInfo->focalLength), 1);
    settings.update(ANDROID_LENS_APERTURE, &(m_sensorStaticInfo->fNumber), 1); // ExifInterface :  TAG_APERTURE = "FNumber";
    const float filterDensity = 0.0f;
    settings.update(ANDROID_LENS_FILTER_DENSITY, &filterDensity, 1);
    const uint8_t opticalStabilizationMode = ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
    settings.update(ANDROID_LENS_OPTICAL_STABILIZATION_MODE, &opticalStabilizationMode, 1);

    /** android.sensor */
    const int64_t exposureTime = 0 * MSEC;
    settings.update(ANDROID_SENSOR_EXPOSURE_TIME, &exposureTime, 1);
    const int64_t frameDuration = 33333333L; /* 1/30 s */
    settings.update(ANDROID_SENSOR_FRAME_DURATION, &frameDuration, 1);
    const int32_t sensitivity = 400;
    settings.update(ANDROID_SENSOR_SENSITIVITY, &sensitivity, 1);

    /** android.flash */
    const uint8_t flashMode = ANDROID_FLASH_MODE_OFF;
    settings.update(ANDROID_FLASH_MODE, &flashMode, 1);
    const uint8_t firingPower = 0;
    settings.update(ANDROID_FLASH_FIRING_POWER, &firingPower, 1);
    const int64_t firingTime = 0;
    settings.update(ANDROID_FLASH_FIRING_TIME, &firingTime, 1);

    /** android.noise_reduction */
    const uint8_t noiseStrength = 5;
    settings.update(ANDROID_NOISE_REDUCTION_STRENGTH, &noiseStrength, 1);

    /** android.color_correction */
    const camera_metadata_rational_t colorTransform[9] = {
        {1,1}, {0,1}, {0,1},
        {0,1}, {1,1}, {0,1},
        {0,1}, {0,1}, {1,1}
    };
    settings.update(ANDROID_COLOR_CORRECTION_TRANSFORM, colorTransform, 9);

    /** android.tonemap */
    const float tonemapCurve[4] = {
        0.0f, 0.0f,
        1.0f, 1.0f
    };
    settings.update(ANDROID_TONEMAP_CURVE_RED, tonemapCurve, 4);
    settings.update(ANDROID_TONEMAP_CURVE_GREEN, tonemapCurve, 4);
    settings.update(ANDROID_TONEMAP_CURVE_BLUE, tonemapCurve, 4);

    /** android.edge */
    const uint8_t edgeStrength = 5;
    settings.update(ANDROID_EDGE_STRENGTH, &edgeStrength, 1);

    /** android.scaler */
    const int32_t cropRegion[4] = {
        0, 0, w, h
    };
    settings.update(ANDROID_SCALER_CROP_REGION, cropRegion, 4);

    /** android.jpeg */
    const uint8_t jpegQuality = 96;
    settings.update(ANDROID_JPEG_QUALITY, &jpegQuality, 1);
    const int32_t thumbnailSize[2] = {
        512, 384
    };
    settings.update(ANDROID_JPEG_THUMBNAIL_SIZE, thumbnailSize, 2);
    const uint8_t thumbnailQuality = 100;
    settings.update(ANDROID_JPEG_THUMBNAIL_QUALITY, &thumbnailQuality, 1);
    const double gpsCoordinates[3] = {
        0, 0
    };
    settings.update(ANDROID_JPEG_GPS_COORDINATES, gpsCoordinates, 3);
    const uint8_t gpsProcessingMethod[32] = "None";
    settings.update(ANDROID_JPEG_GPS_PROCESSING_METHOD, gpsProcessingMethod, 32);
    const int64_t gpsTimestamp = 0;
    settings.update(ANDROID_JPEG_GPS_TIMESTAMP, &gpsTimestamp, 1);
    const int32_t jpegOrientation = 0;
    settings.update(ANDROID_JPEG_ORIENTATION, &jpegOrientation, 1);

    /** android.stats */
    const uint8_t faceDetectMode = ANDROID_STATISTICS_FACE_DETECT_MODE_OFF;
    settings.update(ANDROID_STATISTICS_FACE_DETECT_MODE, &faceDetectMode, 1);
    const uint8_t histogramMode = ANDROID_STATISTICS_HISTOGRAM_MODE_OFF;
    settings.update(ANDROID_STATISTICS_HISTOGRAM_MODE, &histogramMode, 1);
    const uint8_t sharpnessMapMode = ANDROID_STATISTICS_SHARPNESS_MAP_MODE_OFF;
    settings.update(ANDROID_STATISTICS_SHARPNESS_MAP_MODE, &sharpnessMapMode, 1);
    const uint8_t hotPixelMapMode = 0;
    settings.update(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE, &hotPixelMapMode, 1);

    /** android.blacklevel */
    const uint8_t blackLevelLock = ANDROID_BLACK_LEVEL_LOCK_OFF;
    settings.update(ANDROID_BLACK_LEVEL_LOCK, &blackLevelLock, 1);

    /** Processing block modes */
    uint8_t hotPixelMode = ANDROID_HOT_PIXEL_MODE_OFF;
    uint8_t demosaicMode = ANDROID_DEMOSAIC_MODE_FAST;
    uint8_t noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_OFF;
    uint8_t shadingMode = ANDROID_SHADING_MODE_OFF;
    uint8_t colorCorrectionMode = ANDROID_COLOR_CORRECTION_MODE_TRANSFORM_MATRIX;
    uint8_t tonemapMode = ANDROID_TONEMAP_MODE_CONTRAST_CURVE;
    uint8_t edgeMode = ANDROID_EDGE_MODE_OFF;
    uint8_t lensShadingMapMode = ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF;
    switch (type) {
    case CAMERA3_TEMPLATE_STILL_CAPTURE:
        if (m_cameraId == CAMERA_ID_BACK)
            lensShadingMapMode = ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_ON;
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_HIGH_QUALITY;
        noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY;
        shadingMode = ANDROID_SHADING_MODE_FAST;
        colorCorrectionMode = ANDROID_COLOR_CORRECTION_MODE_HIGH_QUALITY;
        tonemapMode = ANDROID_TONEMAP_MODE_HIGH_QUALITY;
        edgeMode = ANDROID_EDGE_MODE_HIGH_QUALITY;
        break;
    case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_HIGH_QUALITY;
        noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_FAST;
        shadingMode = ANDROID_SHADING_MODE_FAST;
        colorCorrectionMode = ANDROID_COLOR_CORRECTION_MODE_HIGH_QUALITY;
        tonemapMode = ANDROID_TONEMAP_MODE_FAST;
        edgeMode = ANDROID_EDGE_MODE_FAST;
        break;
    case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_HIGH_QUALITY;
        noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_ZERO_SHUTTER_LAG;
        shadingMode = ANDROID_SHADING_MODE_FAST;
        colorCorrectionMode = ANDROID_COLOR_CORRECTION_MODE_HIGH_QUALITY;
        tonemapMode = ANDROID_TONEMAP_MODE_FAST;
        edgeMode = ANDROID_EDGE_MODE_ZERO_SHUTTER_LAG;
        break;
    case CAMERA3_TEMPLATE_PREVIEW:
    case CAMERA3_TEMPLATE_VIDEO_RECORD:
    default:
        hotPixelMode = ANDROID_HOT_PIXEL_MODE_FAST;
        noiseReductionMode = ANDROID_NOISE_REDUCTION_MODE_FAST;
        shadingMode = ANDROID_SHADING_MODE_FAST;
        colorCorrectionMode = ANDROID_COLOR_CORRECTION_MODE_FAST;
        tonemapMode = ANDROID_TONEMAP_MODE_FAST;
        edgeMode = ANDROID_EDGE_MODE_FAST;
        break;
    }
    settings.update(ANDROID_HOT_PIXEL_MODE, &hotPixelMode, 1);
    settings.update(ANDROID_DEMOSAIC_MODE, &demosaicMode, 1);
    settings.update(ANDROID_NOISE_REDUCTION_MODE, &noiseReductionMode, 1);
    settings.update(ANDROID_SHADING_MODE, &shadingMode, 1);
    settings.update(ANDROID_COLOR_CORRECTION_MODE, &colorCorrectionMode, 1);
    settings.update(ANDROID_TONEMAP_MODE, &tonemapMode, 1);
    settings.update(ANDROID_EDGE_MODE, &edgeMode, 1);
    settings.update(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE, &lensShadingMapMode, 1);

    *request = settings.release();
    m_defaultRequestSetting = *request;

    ALOGD("DEBUG(%s[%d]):Registered default request template(%d)", __FUNCTION__, __LINE__, type);
    return OK;
}

status_t ExynosCamera3MetadataConverter::constructStaticInfo(int cameraId, camera_metadata_t **cameraInfo)
{
    status_t ret = NO_ERROR;

    ALOGD("DEBUG(%s[%d]):ID(%d)", __FUNCTION__, __LINE__, cameraId);
    struct ExynosSensorInfoBase *sensorStaticInfo = NULL;
    CameraMetadata info;
    Vector<int64_t> i64Vector;
    Vector<int32_t> i32Vector;

    sensorStaticInfo = createExynosCamera3SensorInfo(cameraId);
    if (sensorStaticInfo == NULL) {
        ALOGE("ERR(%s[%d]): sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* android.colorCorrection static attributes */
    if (sensorStaticInfo->colorAberrationModes != NULL) {
        ret = info.update(ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES,
                sensorStaticInfo->colorAberrationModes,
                sensorStaticInfo->colorAberrationModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_COLOR_CORRECTION_ABERRATION_MODE update failed(%d)", __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):colorAberrationModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    /* andorid.control static attributes */
    if (sensorStaticInfo->antiBandingModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
                sensorStaticInfo->antiBandingModes,
                sensorStaticInfo->antiBandingModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):antiBandingModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    if (sensorStaticInfo->aeModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AE_AVAILABLE_MODES,
                sensorStaticInfo->aeModes,
                sensorStaticInfo->aeModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_CONTROL_AE_AVAILABLE_MODES update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):aeModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    i32Vector.clear();
    m_createAeAvailableFpsRanges(sensorStaticInfo, &i32Vector, cameraId);
    ret = info.update(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
            i32Vector.array(), i32Vector.size());
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_CONTROL_AE_COMPENSATION_RANGE,
            sensorStaticInfo->exposureCompensationRange,
            ARRAY_LENGTH(sensorStaticInfo->exposureCompensationRange));
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_CONTROL_AE_COMPENSATION_RANGE update failed(%d)",  __FUNCTION__, ret);

    const camera_metadata_rational exposureCompensationStep =
    {(int32_t)((sensorStaticInfo->exposureCompensationStep) * 100.0), 100};
    ret = info.update(ANDROID_CONTROL_AE_COMPENSATION_STEP,
            &exposureCompensationStep, 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_CONTROL_AE_COMPENSATION_STEP update failed(%d)",  __FUNCTION__, ret);

    if (sensorStaticInfo->afModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AF_AVAILABLE_MODES,
                sensorStaticInfo->afModes,
                sensorStaticInfo->afModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_CONTROL_AF_AVAILABLE_MODES update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):afModes is NULL", __FUNCTION__, __LINE__);
    }

    if (sensorStaticInfo->effectModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AVAILABLE_EFFECTS,
                sensorStaticInfo->effectModes,
                sensorStaticInfo->effectModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_CONTROL_AVAILABLE_EFFECTS update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):effectModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    if (sensorStaticInfo->sceneModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AVAILABLE_SCENE_MODES,
                sensorStaticInfo->sceneModes,
                sensorStaticInfo->sceneModesLength);
        if (ret < 0)
            ALOGE("DEBUG(%s):ANDROID_CONTROL_AVAILABLE_SCENE_MODES update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):sceneModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    if (sensorStaticInfo->videoStabilizationModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
                sensorStaticInfo->videoStabilizationModes,
                sensorStaticInfo->videoStabilizationModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES update failed(%d)",
                    __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):videoStabilizationModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    if (sensorStaticInfo->awbModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AWB_AVAILABLE_MODES,
                sensorStaticInfo->awbModes,
                sensorStaticInfo->awbModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_CONTROL_AWB_AVAILABLE_MODES update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):awbModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    ret = info.update(ANDROID_CONTROL_MAX_REGIONS,
            sensorStaticInfo->max3aRegions,
            ARRAY_LENGTH(sensorStaticInfo->max3aRegions));
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_CONTROL_MAX_REGIONS update failed(%d)",  __FUNCTION__, ret);

    if (sensorStaticInfo->sceneModeOverrides != NULL) {
        ret = info.update(ANDROID_CONTROL_SCENE_MODE_OVERRIDES,
                sensorStaticInfo->sceneModeOverrides,
                sensorStaticInfo->sceneModeOverridesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_CONTROL_SCENE_MODE_OVERRIDES update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):sceneModeOverrides at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    i32Vector.clear();
    if ( m_createControlAvailableHighSpeedVideoConfigurations(sensorStaticInfo, &i32Vector, cameraId) == NO_ERROR ) {
        ret = info.update(ANDROID_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS,
             i32Vector.array(), i32Vector.size());
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS update failed(%d)",
                    __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):ANDROID_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS is NULL", __FUNCTION__, __LINE__);
    }

    ret = info.update(ANDROID_CONTROL_AE_LOCK_AVAILABLE,
            &(sensorStaticInfo->aeLockAvailable), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_CONTROL_AE_LOCK_AVAILABLE update failed(%d)", __FUNCTION__, ret);

    ret = info.update(ANDROID_CONTROL_AWB_LOCK_AVAILABLE,
            &(sensorStaticInfo->awbLockAvailable), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_CONTROL_AWB_LOCK_AVAILABLE update failed(%d)", __FUNCTION__, ret);

    if (sensorStaticInfo->controlModes != NULL) {
        ret = info.update(ANDROID_CONTROL_AVAILABLE_MODES,
                sensorStaticInfo->controlModes,
                sensorStaticInfo->controlModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_CONTROL_AVAILABLE_MODES update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):controlModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    /* android.edge static attributes */
    if (sensorStaticInfo->edgeModes != NULL) {
        ret = info.update(ANDROID_EDGE_AVAILABLE_EDGE_MODES,
                sensorStaticInfo->edgeModes,
                sensorStaticInfo->edgeModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_EDGE_AVAILABLE_EDGE_MODES update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):edgeModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    /* andorid.flash static attributes */
    ret = info.update(ANDROID_FLASH_INFO_AVAILABLE,
            &(sensorStaticInfo->flashAvailable), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_FLASH_INFO_AVAILABLE update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_FLASH_INFO_CHARGE_DURATION,
            &(sensorStaticInfo->chargeDuration), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_FLASH_INFO_CHARGE_DURATION update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_FLASH_COLOR_TEMPERATURE,
            &(sensorStaticInfo->colorTemperature), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_FLASH_COLOR_TEMPERATURE update failed(%d)", __FUNCTION__, ret);

    ret = info.update(ANDROID_FLASH_MAX_ENERGY,
            &(sensorStaticInfo->maxEnergy), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_FLASH_MAX_ENERGY update failed(%d)", __FUNCTION__, ret);

    /* android.hotPixel static attributes */
    if (sensorStaticInfo->hotPixelModes != NULL) {
        ret = info.update(ANDROID_HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES,
                sensorStaticInfo->hotPixelModes,
                sensorStaticInfo->hotPixelModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES update failed(%d)", __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):hotPixelModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    /* andorid.jpeg static attributes */
    i32Vector.clear();
    m_createJpegAvailableThumbnailSizes(sensorStaticInfo, &i32Vector);
    ret = info.update(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES, i32Vector.array(), i32Vector.size());
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES update failed(%d)",  __FUNCTION__, ret);

    const int32_t jpegMaxSize = sensorStaticInfo->maxPictureW * sensorStaticInfo->maxPictureH * 2;
    ret = info.update(ANDROID_JPEG_MAX_SIZE, &jpegMaxSize, 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_JPEG_MAX_SIZE update failed(%d)",  __FUNCTION__, ret);


    /* android.lens static attributes */
    ret = info.update(ANDROID_LENS_INFO_AVAILABLE_APERTURES,
            &(sensorStaticInfo->fNumber), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_LENS_INFO_AVAILABLE_APERTURES update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES,
            &(sensorStaticInfo->filterDensity), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
            &(sensorStaticInfo->focalLength), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS update failed(%d)",  __FUNCTION__, ret);

    if (sensorStaticInfo->opticalStabilization != NULL
        && m_hasTagInList(sensorStaticInfo->requestKeys,
                          sensorStaticInfo->requestKeysLength,
                          ANDROID_LENS_OPTICAL_STABILIZATION_MODE))
    {
        ret = info.update(ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
                sensorStaticInfo->opticalStabilization,
                sensorStaticInfo->opticalStabilizationLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION update failed(%d)",
                    __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):opticalStabilization at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    ret = info.update(ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE,
            &(sensorStaticInfo->hyperFocalDistance), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,
            &(sensorStaticInfo->minimumFocusDistance), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_LENS_INFO_SHADING_MAP_SIZE,
            sensorStaticInfo->shadingMapSize,
            ARRAY_LENGTH(sensorStaticInfo->shadingMapSize));
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_LENS_INFO_SHADING_MAP_SIZE update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION,
            &(sensorStaticInfo->focusDistanceCalibration), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_LENS_FACING,
            &(sensorStaticInfo->lensFacing), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_LENS_FACING update failed(%d)",  __FUNCTION__, ret);

    /* android.noiseReduction static attributes */
    if (sensorStaticInfo->noiseReductionModes != NULL
        && m_hasTagInList(sensorStaticInfo->requestKeys,
                          sensorStaticInfo->requestKeysLength,
                          ANDROID_NOISE_REDUCTION_MODE))
    {
        ret = info.update(ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES,
                sensorStaticInfo->noiseReductionModes,
                sensorStaticInfo->noiseReductionModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES update failed(%d)",
                    __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):noiseReductionModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    /* android.request static attributes */
    ret = info.update(ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS,
            sensorStaticInfo->maxNumOutputStreams,
            ARRAY_LENGTH(sensorStaticInfo->maxNumOutputStreams));
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_REQUEST_MAX_NUM_INPUT_STREAMS,
            &(sensorStaticInfo->maxNumInputStreams), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_REQUEST_MAX_NUM_INPUT_STREAMS update failed(%d)", __FUNCTION__, ret);

    ret = info.update(ANDROID_REQUEST_PIPELINE_MAX_DEPTH,
            &(sensorStaticInfo->maxPipelineDepth), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_REQUEST_PIPELINE_MAX_DEPTH update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_REQUEST_PARTIAL_RESULT_COUNT,
            &(sensorStaticInfo->partialResultCount), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_REQUEST_PARTIAL_RESULT_COUNT update failed(%d)",  __FUNCTION__, ret);

    if (sensorStaticInfo->capabilities != NULL) {
        ret = info.update(ANDROID_REQUEST_AVAILABLE_CAPABILITIES,
                sensorStaticInfo->capabilities,
                sensorStaticInfo->capabilitiesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_REQUEST_AVAILABLE_CAPABILITIES update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):capabilities at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    if (sensorStaticInfo->requestKeys != NULL) {
        ret = info.update(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS,
                sensorStaticInfo->requestKeys,
                sensorStaticInfo->requestKeysLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):requestKeys at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    if (sensorStaticInfo->resultKeys != NULL) {
        ret = info.update(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS,
                sensorStaticInfo->resultKeys,
                sensorStaticInfo->resultKeysLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_REQUEST_AVAILABLE_RESULT_KEYS update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):resultKeys at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    if (sensorStaticInfo->characteristicsKeys != NULL) {
        ret = info.update(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS,
                sensorStaticInfo->characteristicsKeys,
                sensorStaticInfo->characteristicsKeysLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):characteristicsKeys at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    /* android.scaler static attributes */
    const float maxZoom = (sensorStaticInfo->maxZoomRatio / 1000);
    ret = info.update(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM, &maxZoom, 1);
    if (ret < 0) {
        ALOGD("DEBUG(%s):ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM update failed(%d)",  __FUNCTION__, ret);
    }

    /* TODO:implement input/output format map */
#if 0
    ret = info.update(ANDROID_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP,
            ,
            );
    if (ret < 0)
        ALOGE("DEBUG(%s):ANDROID_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP update failed(%d)", __FUNCTION__, ret);
#endif

    i32Vector.clear();
    if(m_createScalerAvailableInputOutputFormatsMap(sensorStaticInfo, &i32Vector, cameraId) == NO_ERROR) {
        /* Update AvailableInputOutputFormatsMap only if private reprocessing is supported */
        ret = info.update(ANDROID_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP, i32Vector.array(), i32Vector.size());
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP update failed(%d)",  __FUNCTION__, ret);
    }

    i32Vector.clear();
    m_createScalerAvailableStreamConfigurationsOutput(sensorStaticInfo, &i32Vector, cameraId);
    ret = info.update(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, i32Vector.array(), i32Vector.size());
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS update failed(%d)",  __FUNCTION__, ret);

    i64Vector.clear();
    m_createScalerAvailableMinFrameDurations(sensorStaticInfo, &i64Vector, cameraId);
    ret = info.update(ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS, i64Vector.array(), i64Vector.size());
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS update failed(%d)",  __FUNCTION__, ret);

    if (m_hasTagInList(sensorStaticInfo->capabilities, sensorStaticInfo->capabilitiesLength,
                       ANDROID_REQUEST_AVAILABLE_CAPABILITIES_PRIVATE_REPROCESSING) == true) {
        /* Set Stall duration for reprocessing */
#ifdef HAL3_REPROCESSING_MAX_CAPTURE_STALL
        int32_t maxCaptureStall = HAL3_REPROCESSING_MAX_CAPTURE_STALL;
        ret = info.update(ANDROID_REPROCESS_MAX_CAPTURE_STALL, &maxCaptureStall, 1);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_REPROCESS_MAX_CAPTURE_STALL update failed(%d)",  __FUNCTION__, ret);
#else
        ALOGE("ERR(%s):Private reprocessing is supported but ANDROID_REPROCESS_MAX_CAPTURE_STALL has not specified.",  __FUNCTION__);
#endif
    }

    if (sensorStaticInfo->stallDurations != NULL) {
        ret = info.update(ANDROID_SCALER_AVAILABLE_STALL_DURATIONS,
                sensorStaticInfo->stallDurations,
                sensorStaticInfo->stallDurationsLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_SCALER_AVAILABLE_STALL_DURATIONS update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):stallDurations at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    ret = info.update(ANDROID_SCALER_CROPPING_TYPE,
            &(sensorStaticInfo->croppingType), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SCALER_CROPPING_TYPE update failed(%d)",  __FUNCTION__, ret);

    /* android.sensor static attributes */
    const int32_t kResolution[4] =
    {0, 0, sensorStaticInfo->maxSensorW, sensorStaticInfo->maxSensorH};
    ret = info.update(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE, kResolution, 4);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_INFO_SENSITIVITY_RANGE,
            sensorStaticInfo->sensitivityRange,
            ARRAY_LENGTH(sensorStaticInfo->sensitivityRange));
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_INFO_SENSITIVITY_RANGE update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
            &(sensorStaticInfo->colorFilterArrangement), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE,
            sensorStaticInfo->exposureTimeRange,
            ARRAY_LENGTH(sensorStaticInfo->exposureTimeRange));
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_INFO_MAX_FRAME_DURATION,
            &(sensorStaticInfo->maxFrameDuration), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_INFO_MAX_FRAME_DURATION update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_INFO_PHYSICAL_SIZE,
            sensorStaticInfo->sensorPhysicalSize,
            ARRAY_LENGTH(sensorStaticInfo->sensorPhysicalSize));
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_INFO_PHYSICAL_SIZE update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE, &(kResolution[2]), 2);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_INFO_WHITE_LEVEL,
            &(sensorStaticInfo->whiteLevel), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_INFO_WHITE_LEVEL update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE,
            &(sensorStaticInfo->timestampSource), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE update failed(%d)", __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_REFERENCE_ILLUMINANT1,
            &(sensorStaticInfo->referenceIlluminant1), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_REFERENCE_ILLUMINANT1 update failed(%d)", __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_REFERENCE_ILLUMINANT2,
            &(sensorStaticInfo->referenceIlluminant2), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_REFERENCE_ILLUMINANT2 update failed(%d)", __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_CALIBRATION_TRANSFORM1, sensorStaticInfo->calibration1, 9);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_CALIBRATION_TRANSFORM2 update failed(%d)", __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_CALIBRATION_TRANSFORM2, sensorStaticInfo->calibration2, 9);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_CALIBRATION_TRANSFORM2 update failed(%d)", __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_COLOR_TRANSFORM1, sensorStaticInfo->colorTransformMatrix1, 9);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_COLOR_TRANSFORM1 update failed(%d)", __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_COLOR_TRANSFORM2, sensorStaticInfo->colorTransformMatrix2, 9);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_COLOR_TRANSFORM2 update failed(%d)", __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_FORWARD_MATRIX1, sensorStaticInfo->forwardMatrix1, 9);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_FORWARD_MATRIX1 update failed(%d)", __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_FORWARD_MATRIX2, sensorStaticInfo->forwardMatrix2, 9);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_FORWARD_MATRIX2 update failed(%d)", __FUNCTION__, ret);

#if 0
    ret = info.update(ANDROID_SENSOR_BASE_GAIN_FACTOR,
            ,
            );
    if (ret < 0)
        ALOGE("DEBUG(%s):ANDROID_SENSOR_BASE_GAIN_FACTOR update failed(%d)", __FUNCTION__, ret);
#endif

    ret = info.update(ANDROID_SENSOR_BLACK_LEVEL_PATTERN,
            sensorStaticInfo->blackLevelPattern,
            ARRAY_LENGTH(sensorStaticInfo->blackLevelPattern));
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_BLACK_LEVEL_PATTERN update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_MAX_ANALOG_SENSITIVITY,
            &(sensorStaticInfo->maxAnalogSensitivity), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_MAX_ANALOG_SENSITIVITY update failed(%d)", __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_ORIENTATION,
            &(sensorStaticInfo->orientation), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_ORIENTATION update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_SENSOR_PROFILE_HUE_SAT_MAP_DIMENSIONS,
            sensorStaticInfo->profileHueSatMapDimensions,
            ARRAY_LENGTH(sensorStaticInfo->profileHueSatMapDimensions));
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SENSOR_PROFILE_HUE_SAT_MAP_DIMENSIONS update failed(%d)", __FUNCTION__, ret);

    if (sensorStaticInfo->testPatternModes != NULL) {
        ret = info.update(ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES,
                sensorStaticInfo->testPatternModes,
                sensorStaticInfo->testPatternModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES update failed(%d)", __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):testPatternModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    /* android.statistics static attributes */
    if (sensorStaticInfo->faceDetectModes != NULL) {
        ret = info.update(ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES,
                sensorStaticInfo->faceDetectModes,
                sensorStaticInfo->faceDetectModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES update failed(%d)",
                    __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):faceDetectModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    ret = info.update(ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT,
            &(sensorStaticInfo->histogramBucketCount), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_STATISTICS_INFO_MAX_FACE_COUNT,
            &sensorStaticInfo->maxNumDetectedFaces, 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_STATISTICS_INFO_MAX_FACE_COUNT update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT,
            &sensorStaticInfo->maxHistogramCount, 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE,
            &(sensorStaticInfo->maxSharpnessMapValue), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE update failed(%d)",  __FUNCTION__, ret);

    ret = info.update(ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE,
            sensorStaticInfo->sharpnessMapSize,
            ARRAY_LENGTH(sensorStaticInfo->sharpnessMapSize));
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE update failed(%d)",  __FUNCTION__, ret);

    if (sensorStaticInfo->hotPixelMapModes != NULL) {
        ret = info.update(ANDROID_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES,
                sensorStaticInfo->hotPixelMapModes,
                sensorStaticInfo->hotPixelMapModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES update failed(%d)",
                    __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):hotPixelMapModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    if (sensorStaticInfo->lensShadingMapModes != NULL) {
        ret = info.update(ANDROID_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES,
                sensorStaticInfo->lensShadingMapModes,
                sensorStaticInfo->lensShadingMapModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES update failed(%d)",
                    __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):lensShadingMapModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    if (sensorStaticInfo->shadingAvailableModes != NULL) {
        ret = info.update(ANDROID_SHADING_AVAILABLE_MODES,
                sensorStaticInfo->shadingAvailableModes,
                sensorStaticInfo->shadingAvailableModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_SHADING_AVAILABLE_MODES update failed(%d)", __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):shadingAvailableModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    /* andorid.tonemap static attributes */
    ret = info.update(ANDROID_TONEMAP_MAX_CURVE_POINTS,
            &(sensorStaticInfo->tonemapCurvePoints), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_TONEMAP_MAX_CURVE_POINTS update failed(%d)",  __FUNCTION__, ret);

    if (sensorStaticInfo->toneMapModes != NULL) {
        ret = info.update(ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES,
                sensorStaticInfo->toneMapModes,
                sensorStaticInfo->toneMapModesLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):toneMapModes at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    /* android.led static attributes */
    if (sensorStaticInfo->leds != NULL) {
        ret = info.update(ANDROID_LED_AVAILABLE_LEDS,
                sensorStaticInfo->leds,
                sensorStaticInfo->ledsLength);
        if (ret < 0)
            ALOGD("DEBUG(%s):ANDROID_LED_AVAILABLE_LEDS update failed(%d)",  __FUNCTION__, ret);
    } else {
        ALOGD("DEBUG(%s[%d]):leds at sensorStaticInfo is NULL", __FUNCTION__, __LINE__);
    }

    /* andorid.info static attributes */
    ret = info.update(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL,
            &(sensorStaticInfo->supportedHwLevel), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL update failed(%d)",  __FUNCTION__, ret);

    /* android.sync static attributes */
    ret = info.update(ANDROID_SYNC_MAX_LATENCY,
            &(sensorStaticInfo->maxLatency), 1);
    if (ret < 0)
        ALOGD("DEBUG(%s):ANDROID_SYNC_MAX_LATENCY update failed(%d)",  __FUNCTION__, ret);

    *cameraInfo = info.release();

    return OK;
}

void ExynosCamera3MetadataConverter::setStaticInfo(int camId, camera_metadata_t *info)
{
    if (info == NULL) {
        camera_metadata_t *meta;
        ALOGW("WARN(%s[%d]):info is null", __FUNCTION__, __LINE__);
        ExynosCamera3MetadataConverter::constructStaticInfo(camId, &meta);
        m_staticInfo = meta;
    } else {
        m_staticInfo = info;
    }
}

status_t ExynosCamera3MetadataConverter::initShotData(struct camera2_shot_ext *shot_ext)
{
    ALOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    memset(shot_ext, 0x00, sizeof(struct camera2_shot_ext));

    struct camera2_shot *shot = &shot_ext->shot;

    // TODO: make this from default request settings
    /* request */
    shot->ctl.request.id = 0;
    shot->ctl.request.metadataMode = METADATA_MODE_FULL;
    shot->ctl.request.frameCount = 0;

    /* lens */
    shot->ctl.lens.focusDistance = -1.0f;
    shot->ctl.lens.aperture = m_sensorStaticInfo->fNumber; // ExifInterface :  TAG_APERTURE = "FNumber";
    shot->ctl.lens.focalLength = m_sensorStaticInfo->focalLength;
    shot->ctl.lens.filterDensity = 0.0f;
    shot->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_OFF;

    shot->uctl.lensUd.pos = 0;
    shot->uctl.lensUd.posSize = 0;
    shot->ctl.aa.vendor_afState = AA_AFSTATE_INACTIVE;

    int minFps = (m_sensorStaticInfo->minFps == 0) ? 0 : (m_sensorStaticInfo->maxFps / 2);
    int maxFps = (m_sensorStaticInfo->maxFps == 0) ? 0 : m_sensorStaticInfo->maxFps;

    /* The min fps can not be '0'. Therefore it is set up default value '15'. */
    if (minFps == 0) {
        ALOGW("WRN(%s): Invalid min fps value(%d)", __FUNCTION__, minFps);
        minFps = 15;
    }
    /*  The initial fps can not be '0' and bigger than '30'. Therefore it is set up default value '30'. */
    if (maxFps == 0 || 30 < maxFps) {
        ALOGW("WRN(%s): Invalid max fps value(%d)", __FUNCTION__, maxFps);
        maxFps = 30;
    }

    m_maxFps = maxFps;

    /* sensor */
    shot->ctl.sensor.exposureTime = 0;
    shot->ctl.sensor.frameDuration = (1000 * 1000 * 1000) / maxFps;
    shot->ctl.sensor.sensitivity = 0;

    /* flash */
    shot->ctl.flash.flashMode = CAM2_FLASH_MODE_OFF;
    shot->ctl.flash.firingPower = 0;
    shot->ctl.flash.firingTime = 0;
    m_overrideFlashControl = false;

    /* hotpixel */
    shot->ctl.hotpixel.mode = (enum processing_mode)0;

    /* demosaic */
    shot->ctl.demosaic.mode = (enum demosaic_processing_mode)0;

    /* noise */
    shot->ctl.noise.mode = ::PROCESSING_MODE_OFF;
    shot->ctl.noise.strength = 5;

    /* shading */
    shot->ctl.shading.mode = (enum processing_mode)0;

    /* color */
    shot->ctl.color.mode = COLORCORRECTION_MODE_FAST;
    static const camera_metadata_rational_t colorTransform[9] = {
        {1, 1}, {0, 1}, {0, 1},
        {0, 1}, {1, 1}, {0, 1},
        {0, 1}, {0, 1}, {1, 1},
    };
    memcpy(shot->ctl.color.transform, colorTransform, sizeof(shot->ctl.color.transform));

    /* tonemap */
    shot->ctl.tonemap.mode = ::TONEMAP_MODE_FAST;
    static const float tonemapCurve[4] = {
        0.f, 0.f,
        1.f, 1.f
    };

    int tonemapCurveSize = sizeof(tonemapCurve);
    int sizeOfCurve = sizeof(shot->ctl.tonemap.curveRed) / sizeof(shot->ctl.tonemap.curveRed[0]);

    for (int i = 0; i < sizeOfCurve; i += 4) {
        memcpy(&(shot->ctl.tonemap.curveRed[i]),   tonemapCurve, tonemapCurveSize);
        memcpy(&(shot->ctl.tonemap.curveGreen[i]), tonemapCurve, tonemapCurveSize);
        memcpy(&(shot->ctl.tonemap.curveBlue[i]),  tonemapCurve, tonemapCurveSize);
    }

    /* edge */
    shot->ctl.edge.mode = ::PROCESSING_MODE_OFF;
    shot->ctl.edge.strength = 5;

    /* scaler */
    float zoomRatio = m_parameters->getZoomRatio(0) / 1000;
    if (setMetaCtlCropRegion(shot_ext, 0,
                             m_sensorStaticInfo->maxSensorW,
                             m_sensorStaticInfo->maxSensorH,
                             m_sensorStaticInfo->maxPreviewW,
                             m_sensorStaticInfo->maxPreviewH,
                             zoomRatio) != NO_ERROR) {
        ALOGE("ERR(%s):m_setZoom() fail", __FUNCTION__);
    }

    /* jpeg */
    shot->ctl.jpeg.quality = 96;
    shot->ctl.jpeg.thumbnailSize[0] = m_sensorStaticInfo->maxThumbnailW;
    shot->ctl.jpeg.thumbnailSize[1] = m_sensorStaticInfo->maxThumbnailH;
    shot->ctl.jpeg.thumbnailQuality = 100;
    shot->ctl.jpeg.gpsCoordinates[0] = 0;
    shot->ctl.jpeg.gpsCoordinates[1] = 0;
    shot->ctl.jpeg.gpsCoordinates[2] = 0;
    memset(&shot->ctl.jpeg.gpsProcessingMethod, 0x0,
        sizeof(shot->ctl.jpeg.gpsProcessingMethod));
    shot->ctl.jpeg.gpsTimestamp = 0L;
    shot->ctl.jpeg.orientation = 0L;

    /* stats */
    shot->ctl.stats.faceDetectMode = ::FACEDETECT_MODE_OFF;
    shot->ctl.stats.histogramMode = ::STATS_MODE_OFF;
    shot->ctl.stats.sharpnessMapMode = ::STATS_MODE_OFF;

    /* aa */
    shot->ctl.aa.captureIntent = ::AA_CAPTURE_INTENT_CUSTOM;
    shot->ctl.aa.mode = ::AA_CONTROL_AUTO;
    shot->ctl.aa.effectMode = ::AA_EFFECT_OFF;
    shot->ctl.aa.sceneMode = ::AA_SCENE_MODE_FACE_PRIORITY;
    shot->ctl.aa.videoStabilizationMode = VIDEO_STABILIZATION_MODE_OFF;

    /* default metering is center */
    shot->ctl.aa.aeMode = ::AA_AEMODE_CENTER;
    shot->ctl.aa.aeRegions[0] = 0;
    shot->ctl.aa.aeRegions[1] = 0;
    shot->ctl.aa.aeRegions[2] = 0;
    shot->ctl.aa.aeRegions[3] = 0;
    shot->ctl.aa.aeRegions[4] = 1000;
    shot->ctl.aa.aeExpCompensation = 0; /* 0 is middle */
    shot->ctl.aa.vendor_aeExpCompensationStep = m_sensorStaticInfo->exposureCompensationStep;
    shot->ctl.aa.aeLock = ::AA_AE_LOCK_OFF;

    shot->ctl.aa.aeTargetFpsRange[0] = minFps;
    shot->ctl.aa.aeTargetFpsRange[1] = maxFps;

    shot->ctl.aa.aeAntibandingMode = ::AA_AE_ANTIBANDING_AUTO;
    shot->ctl.aa.vendor_aeflashMode = ::AA_FLASHMODE_OFF;

    shot->ctl.aa.awbMode = ::AA_AWBMODE_WB_AUTO;
    shot->ctl.aa.awbLock = ::AA_AWB_LOCK_OFF;
    shot->ctl.aa.afMode = ::AA_AFMODE_OFF;
    shot->ctl.aa.afRegions[0] = 0;
    shot->ctl.aa.afRegions[1] = 0;
    shot->ctl.aa.afRegions[2] = 0;
    shot->ctl.aa.afRegions[3] = 0;
    shot->ctl.aa.afRegions[4] = 1000;
    shot->ctl.aa.afTrigger = AA_AF_TRIGGER_IDLE;

    shot->ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
    shot->ctl.aa.vendor_isoValue = 0;
    shot->ctl.aa.vendor_videoMode = AA_VIDEOMODE_OFF;

    /* 2. dm */

    /* 3. utrl */
#ifdef USE_FW_OPMODE
    shot->uctl.opMode = CAMERA_OP_MODE_HAL3_GED;
#endif

    /* 4. udm */

    /* 5. magicNumber */
    shot->magicNumber = SHOT_MAGIC_NUMBER;

    /* 6. default setfile index */
    setMetaSetfile(shot_ext, ISS_SUB_SCENARIO_STILL_PREVIEW);

    /* user request */
    shot_ext->drc_bypass = 1;
    shot_ext->dis_bypass = 1;
    shot_ext->dnr_bypass = 1;
    shot_ext->fd_bypass  = 1;
/*
    m_dummyShot.request_taap = 1;
    m_dummyShot.request_taac = 0;
    m_dummyShot.request_isp = 1;
    m_dummyShot.request_scc = 0;
    m_dummyShot.request_scp = 1;
    m_dummyShot.request_dis = 0;
*/

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateColorControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_COLOR_CORRECTION_MODE)) {
        entry = settings.find(ANDROID_COLOR_CORRECTION_MODE);
        dst->ctl.color.mode = (enum colorcorrection_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_COLOR_CORRECTION_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_COLOR_CORRECTION_TRANSFORM)) {
        entry = settings.find(ANDROID_COLOR_CORRECTION_TRANSFORM);
        for (size_t i = 0; i < entry.count && i < 9; i++) {
            /* Convert rational to float */
            dst->ctl.color.transform[i].num = entry.data.r[i].numerator;
            dst->ctl.color.transform[i].den = entry.data.r[i].denominator;
        }
        ALOGV("DEBUG(%s):ANDROID_COLOR_CORRECTION_TRANSFORM(%zu)", __FUNCTION__, entry.count);
    }

    if (settings.exists(ANDROID_COLOR_CORRECTION_GAINS)) {
        entry = settings.find(ANDROID_COLOR_CORRECTION_GAINS);
        for (size_t i = 0; i < entry.count && i < 4; i++) {
            dst->ctl.color.gains[i] = entry.data.f[i];
        }
        ALOGV("DEBUG(%s):ANDROID_COLOR_CORRECTION_GAINS(%f,%f,%f,%f)", __FUNCTION__,
                entry.data.f[0], entry.data.f[1], entry.data.f[2], entry.data.f[3]);
    }

    if (settings.exists(ANDROID_COLOR_CORRECTION_ABERRATION_MODE)) {
        entry = settings.find(ANDROID_COLOR_CORRECTION_ABERRATION_MODE);
        dst->ctl.color.aberrationCorrectionMode = (enum processing_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_COLOR_CORRECTION_ABERRATION_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateControlControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;
    uint32_t bnsRatio = DEFAULT_BNS_RATIO;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (m_flashMgr == NULL) {
        ALOGE("ERR(%s[%d]):FlashMgr is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

#ifdef USE_BNS_PREVIEW
    bnsRatio = m_parameters->getBnsScaleRatio()/1000;
#endif

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_CONTROL_AE_ANTIBANDING_MODE)) {
        entry = settings.find(ANDROID_CONTROL_AE_ANTIBANDING_MODE);
        dst->ctl.aa.aeAntibandingMode = (enum aa_ae_antibanding_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_COLOR_AE_ANTIBANDING_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION)) {
        entry = settings.find(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
        dst->ctl.aa.aeExpCompensation = (int32_t) (entry.data.i32[0]);
        ALOGV("DEBUG(%s):ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION(%d)", __FUNCTION__, entry.data.i32[0]);
    }

    if (settings.exists(ANDROID_CONTROL_AE_MODE)) {
        enum aa_aemode aeMode = AA_AEMODE_OFF;
        entry = settings.find(ANDROID_CONTROL_AE_MODE);
        aeMode = (enum aa_aemode) FIMC_IS_METADATA(entry.data.u8[0]);
        m_flashMgr->setFlashExposure(aeMode);
        dst->ctl.aa.aeMode = aeMode;

        enum ExynosCameraActivityFlash::FLASH_REQ flashReq = ExynosCameraActivityFlash::FLASH_REQ_OFF;
        switch (aeMode) {
        case AA_AEMODE_ON_AUTO_FLASH:
        case AA_AEMODE_ON_AUTO_FLASH_REDEYE:
            flashReq = ExynosCameraActivityFlash::FLASH_REQ_AUTO;
            dst->ctl.aa.aeMode = AA_AEMODE_CENTER;
            m_overrideFlashControl = true;
            break;
        case AA_AEMODE_ON_ALWAYS_FLASH:
            flashReq = ExynosCameraActivityFlash::FLASH_REQ_ON;
            dst->ctl.aa.aeMode = AA_AEMODE_CENTER;
            m_overrideFlashControl = true;
            break;
        case AA_AEMODE_ON:
            dst->ctl.aa.aeMode = AA_AEMODE_CENTER;
        case AA_AEMODE_OFF:
        default:
            m_overrideFlashControl = false;
            break;
        }
        if (m_flashMgr != NULL) {
            ALOGV("DEBUG(%s):m_flashMgr(%d)", __FUNCTION__,flashReq);
            m_flashMgr->setFlashReq(flashReq, m_overrideFlashControl);
        }
        ALOGV("DEBUG(%s):ANDROID_CONTROL_AE_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_CONTROL_AE_LOCK)) {
        entry = settings.find(ANDROID_CONTROL_AE_LOCK);
        dst->ctl.aa.aeLock = (enum aa_ae_lock) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_CONTROL_AE_LOCK(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_CONTROL_AE_REGIONS)) {
        ExynosRect2 aeRegion;

        entry = settings.find(ANDROID_CONTROL_AE_REGIONS);
        aeRegion.x1 = entry.data.i32[0];
        aeRegion.y1 = entry.data.i32[1];
        aeRegion.x2 = entry.data.i32[2];
        aeRegion.y2 = entry.data.i32[3];
        dst->ctl.aa.aeRegions[4] = entry.data.i32[4];
        m_convert3AARegion(&aeRegion);

        dst->ctl.aa.aeRegions[0] = aeRegion.x1;
        dst->ctl.aa.aeRegions[1] = aeRegion.y1;
        dst->ctl.aa.aeRegions[2] = aeRegion.x2;
        dst->ctl.aa.aeRegions[3] = aeRegion.y2;
        ALOGV("DEBUG(%s):ANDROID_CONTROL_AE_REGIONS(%d,%d,%d,%d,%d)", __FUNCTION__,
                entry.data.i32[0],
                entry.data.i32[1],
                entry.data.i32[2],
                entry.data.i32[3],
                entry.data.i32[4]);
    }

    if (settings.exists(ANDROID_CONTROL_AWB_REGIONS)) {
        ExynosRect2 awbRegion;

        /* AWB region value would not be used at the f/w,
        because AWB is not related with a specific region */
        entry = settings.find(ANDROID_CONTROL_AWB_REGIONS);
        awbRegion.x1 = entry.data.i32[0];
        awbRegion.y1 = entry.data.i32[1];
        awbRegion.x2 = entry.data.i32[2];
        awbRegion.y2 = entry.data.i32[3];
        dst->ctl.aa.awbRegions[4] = entry.data.i32[4];
        m_convert3AARegion(&awbRegion);

        dst->ctl.aa.awbRegions[0] = awbRegion.x1;
        dst->ctl.aa.awbRegions[1] = awbRegion.y1;
        dst->ctl.aa.awbRegions[2] = awbRegion.x2;
        dst->ctl.aa.awbRegions[3] = awbRegion.y2;
        ALOGV("DEBUG(%s):ANDROID_CONTROL_AWB_REGIONS(%d,%d,%d,%d,%d)", __FUNCTION__,
                entry.data.i32[0],
                entry.data.i32[1],
                entry.data.i32[2],
                entry.data.i32[3],
                entry.data.i32[4]);
    }

    if (settings.exists(ANDROID_CONTROL_AF_REGIONS)) {
        ExynosRect2 afRegion;

        entry = settings.find(ANDROID_CONTROL_AF_REGIONS);
        afRegion.x1 = entry.data.i32[0];
        afRegion.y1 = entry.data.i32[1];
        afRegion.x2 = entry.data.i32[2];
        afRegion.y2 = entry.data.i32[3];
        dst->ctl.aa.afRegions[4] = entry.data.i32[4];
        m_convert3AARegion(&afRegion);

        dst->ctl.aa.afRegions[0] = afRegion.x1;
        dst->ctl.aa.afRegions[1] = afRegion.y1;
        dst->ctl.aa.afRegions[2] = afRegion.x2;
        dst->ctl.aa.afRegions[3] = afRegion.y2;
        ALOGV("DEBUG(%s):ANDROID_CONTROL_AF_REGIONS(%d,%d,%d,%d,%d)", __FUNCTION__,
                entry.data.i32[0],
                entry.data.i32[1],
                entry.data.i32[2],
                entry.data.i32[3],
                entry.data.i32[4]);
    }

    if (settings.exists(ANDROID_CONTROL_AE_TARGET_FPS_RANGE)) {
        entry = settings.find(ANDROID_CONTROL_AE_TARGET_FPS_RANGE);
        for (size_t i = 0; i < entry.count && i < 2; i++)
            dst->ctl.aa.aeTargetFpsRange[i] = entry.data.i32[i];
        m_maxFps = dst->ctl.aa.aeTargetFpsRange[1];
        ALOGV("DEBUG(%s):ANDROID_CONTROL_AE_TARGET_FPS_RANGE(%d-%d)", __FUNCTION__,
                entry.data.i32[0], entry.data.i32[1]);
    }

    if (settings.exists(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER)) {
        entry = settings.find(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER);
        dst->ctl.aa.aePrecaptureTrigger = (enum aa_ae_precapture_trigger) entry.data.u8[0];
        ALOGV("DEBUG(%s):ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER(%d)", __FUNCTION__,
                entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_CONTROL_AF_MODE)) {
        entry = settings.find(ANDROID_CONTROL_AF_MODE);
        dst->ctl.aa.afMode = (enum aa_afmode) FIMC_IS_METADATA(entry.data.u8[0]);
        m_preAfMode = m_afMode;
        m_afMode = dst->ctl.aa.afMode;

        switch (dst->ctl.aa.afMode) {
        case AA_AFMODE_AUTO:
            dst->ctl.aa.vendor_afmode_option = 0x00;
            break;
        case AA_AFMODE_MACRO:
            dst->ctl.aa.vendor_afmode_option = 0x00 | SET_BIT(AA_AFMODE_OPTION_BIT_MACRO);
            break;
        case AA_AFMODE_CONTINUOUS_VIDEO:
            dst->ctl.aa.vendor_afmode_option = 0x00 | SET_BIT(AA_AFMODE_OPTION_BIT_VIDEO);
            /* The afRegion value should be (0,0,0,0) at the Continuous Video mode */
            dst->ctl.aa.afRegions[0] = 0;
            dst->ctl.aa.afRegions[1] = 0;
            dst->ctl.aa.afRegions[2] = 0;
            dst->ctl.aa.afRegions[3] = 0;
            break;
        case AA_AFMODE_CONTINUOUS_PICTURE:
            dst->ctl.aa.vendor_afmode_option = 0x00;
            /* The afRegion value should be (0,0,0,0) at the Continuous Picture mode */
            dst->ctl.aa.afRegions[0] = 0;
            dst->ctl.aa.afRegions[1] = 0;
            dst->ctl.aa.afRegions[2] = 0;
            dst->ctl.aa.afRegions[3] = 0;
            break;
        case AA_AFMODE_OFF:
        default:
            dst->ctl.aa.vendor_afmode_option = 0x00;
            break;
        }
        ALOGV("DEBUG(%s):ANDROID_CONTROL_AF_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_CONTROL_AF_TRIGGER)) {
        entry = settings.find(ANDROID_CONTROL_AF_TRIGGER);
        dst->ctl.aa.afTrigger = (enum aa_af_trigger)entry.data.u8[0];
        ALOGV("DEBUG(%s):ANDROID_CONTROL_AF_TRIGGER(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_CONTROL_AWB_LOCK)) {
        entry = settings.find(ANDROID_CONTROL_AWB_LOCK);
        dst->ctl.aa.awbLock = (enum aa_awb_lock) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_CONTROL_AWB_LOCK(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_CONTROL_AWB_MODE)) {
        entry = settings.find(ANDROID_CONTROL_AWB_MODE);
        dst->ctl.aa.awbMode = (enum aa_awbmode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_CONTROL_AWB_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_CONTROL_CAPTURE_INTENT)) {
        entry = settings.find(ANDROID_CONTROL_CAPTURE_INTENT);
        dst->ctl.aa.captureIntent = (enum aa_capture_intent) entry.data.u8[0];
        if (dst->ctl.aa.captureIntent == AA_CAPTURE_INTENT_VIDEO_RECORD) {
            dst->ctl.aa.vendor_videoMode = AA_VIDEOMODE_ON;
            setMetaSetfile(dst_ext, ISS_SUB_SCENARIO_VIDEO);
        } else {
            setMetaSetfile(dst_ext, ISS_SUB_SCENARIO_STILL_PREVIEW);
        }
        ALOGV("DEBUG(%s):ANDROID_CONTROL_CAPTURE_INTENT(%d) setfile(%d)", __FUNCTION__, dst->ctl.aa.captureIntent, dst_ext->setfile);
    }
    if (settings.exists(ANDROID_CONTROL_EFFECT_MODE)) {
        entry = settings.find(ANDROID_CONTROL_EFFECT_MODE);
        dst->ctl.aa.effectMode = (enum aa_effect_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_CONTROL_EFFECT_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
        ALOGV("DEBUG(%s):dst->ctl.aa.effectMode(%d)", __FUNCTION__, dst->ctl.aa.effectMode);
    }

    if (settings.exists(ANDROID_CONTROL_MODE)) {
        entry = settings.find(ANDROID_CONTROL_MODE);
        dst->ctl.aa.mode = (enum aa_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_CONTROL_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_CONTROL_SCENE_MODE)) {
        entry = settings.find(ANDROID_CONTROL_SCENE_MODE);
        /* HACK : Temporary save the Mode info for adjusting value for CTS Test */
        if (entry.data.u8[0] == ANDROID_CONTROL_SCENE_MODE_HDR)
            dst->ctl.aa.sceneMode = AA_SCENE_MODE_HDR;
        else
            dst->ctl.aa.sceneMode = (enum aa_scene_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_CONTROL_SCENE_MODE(%d)", __FUNCTION__, dst->ctl.aa.sceneMode);
    }

    if (settings.exists(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE)) {
        entry = settings.find(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE);
        dst->ctl.aa.videoStabilizationMode = (enum aa_videostabilization_mode) entry.data.u8[0];
        ALOGV("DEBUG(%s):ANDROID_CONTROL_VIDEO_STABILIZATION_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    enum ExynosCameraActivityFlash::FLASH_STEP flashStep = ExynosCameraActivityFlash::FLASH_STEP_OFF;
    bool isFlashStepChanged = false;

    /* Check AF Trigger to turn on the pre-flash */
    switch (dst->ctl.aa.afTrigger) {
    case AA_AF_TRIGGER_START:
        if (m_flashMgr->getNeedCaptureFlash() == true
            && m_flashMgr->getFlashStatus() == AA_FLASHMODE_OFF) {
            flashStep = ExynosCameraActivityFlash::FLASH_STEP_PRE_START;
            m_flashMgr->setCaptureStatus(true);
            isFlashStepChanged = true;
        }
        break;
    case AA_AF_TRIGGER_CANCEL:
        if (m_flashMgr->getNeedCaptureFlash() == true) {
            m_flashMgr->setFlashStep(ExynosCameraActivityFlash::FLASH_STEP_CANCEL);
            isFlashStepChanged = true;
        }
        break;
    case AA_AF_TRIGGER_IDLE:
    default:
        break;
    }
    /* Check Precapture Trigger to turn on the pre-flash */
    switch (dst->ctl.aa.aePrecaptureTrigger) {
    case AA_AE_PRECAPTURE_TRIGGER_START:
        if (m_flashMgr->getNeedCaptureFlash() == true
            && m_flashMgr->getFlashStatus() == AA_FLASHMODE_OFF) {
            flashStep = ExynosCameraActivityFlash::FLASH_STEP_PRE_START;
            m_flashMgr->setCaptureStatus(true);
            isFlashStepChanged = true;
        }
        break;
    case AA_AE_PRECAPTURE_TRIGGER_CANCEL:
        if (m_flashMgr->getNeedCaptureFlash() == true
            && m_flashMgr->getFlashStatus() != AA_FLASHMODE_OFF
            && m_flashMgr->getFlashStatus() != AA_FLASHMODE_CANCEL) {
            flashStep = ExynosCameraActivityFlash::FLASH_STEP_CANCEL;
            m_flashMgr->setCaptureStatus(false);
            isFlashStepChanged = true;
        }
        break;
    case AA_AE_PRECAPTURE_TRIGGER_IDLE:
    default:
        break;
    }
    /* Check Capture Intent to turn on the main-flash */
    switch (dst->ctl.aa.captureIntent) {
    case AA_CAPTURE_INTENT_STILL_CAPTURE:
        if (m_flashMgr->getNeedCaptureFlash() == true) {
            flashStep = ExynosCameraActivityFlash::FLASH_STEP_MAIN_START;
            isFlashStepChanged = true;
            m_parameters->setMarkingOfExifFlash(1);
        } else {
            m_parameters->setMarkingOfExifFlash(0);
        }
        break;
    case AA_CAPTURE_INTENT_CUSTOM:
    case AA_CAPTURE_INTENT_PREVIEW:
    case AA_CAPTURE_INTENT_VIDEO_RECORD:
    case AA_CAPTURE_INTENT_VIDEO_SNAPSHOT:
    case AA_CAPTURE_INTENT_ZERO_SHUTTER_LAG:
    case AA_CAPTURE_INTENT_MANUAL:
    default:
        break;
    }

    if (isFlashStepChanged == true && m_flashMgr != NULL)
        m_flashMgr->setFlashStep(flashStep);

    /* If aeMode or Mode is NOT Off, Manual AE control can NOT be operated */
    if (dst->ctl.aa.aeMode == AA_AEMODE_OFF
        || dst->ctl.aa.mode == AA_CONTROL_OFF) {
        m_isManualAeControl = true;
        ALOGV("DEBUG(%s):Operate Manual AE Control, aeMode(%d), Mode(%d)", __FUNCTION__,
                dst->ctl.aa.aeMode, dst->ctl.aa.mode);
    } else {
        m_isManualAeControl = false;
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateDemosaicControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_DEMOSAIC_MODE)) {
        entry = settings.find(ANDROID_DEMOSAIC_MODE);
        dst->ctl.demosaic.mode = (enum demosaic_processing_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_DEMOSAIC_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateEdgeControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_EDGE_STRENGTH)) {
        entry = settings.find(ANDROID_EDGE_STRENGTH);
        dst->ctl.edge.strength = (uint32_t) entry.data.u8[0];
        ALOGV("DEBUG(%s):ANDROID_EDGE_STRENGTH(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_EDGE_MODE)) {
        entry = settings.find(ANDROID_EDGE_MODE);
        dst->ctl.edge.mode = (enum processing_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_EDGE_MODE(%d)", __FUNCTION__, entry.data.u8[0]);

        switch (dst->ctl.edge.mode) {
        case PROCESSING_MODE_HIGH_QUALITY:
            dst->ctl.edge.strength = 10;
            break;
        case PROCESSING_MODE_FAST:
        case PROCESSING_MODE_OFF:
        case PROCESSING_MODE_MINIMAL:
        default:
            break;
        }
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateFlashControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (m_overrideFlashControl == true) {
        return OK;
    }

    if (settings.exists(ANDROID_FLASH_FIRING_POWER)) {
        entry = settings.find(ANDROID_FLASH_FIRING_POWER);
        dst->ctl.flash.firingPower = (uint32_t) entry.data.u8[0];
        ALOGV("DEBUG(%s):ANDROID_FLASH_FIRING_POWER(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_FLASH_FIRING_TIME)) {
        entry = settings.find(ANDROID_FLASH_FIRING_TIME);
        dst->ctl.flash.firingTime = (uint64_t) entry.data.i64[0];
        ALOGV("DEBUG(%s):ANDROID_FLASH_FIRING_TIME(%lld)", __FUNCTION__, entry.data.i64[0]);
    }

    if (settings.exists(ANDROID_FLASH_MODE)) {
        entry = settings.find(ANDROID_FLASH_MODE);
        dst->ctl.flash.flashMode = (enum flash_mode) FIMC_IS_METADATA(entry.data.u8[0]);

        ALOGV("DEBUG(%s):ANDROID_FLASH_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateHotPixelControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_HOT_PIXEL_MODE)) {
        entry = settings.find(ANDROID_HOT_PIXEL_MODE);
        dst->ctl.hotpixel.mode = (enum processing_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_HOT_PIXEL_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateJpegControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_JPEG_GPS_COORDINATES)) {
        entry = settings.find(ANDROID_JPEG_GPS_COORDINATES);
        for (size_t i = 0; i < entry.count && i < 3; i++)
            dst->ctl.jpeg.gpsCoordinates[i] = entry.data.d[i];
        ALOGV("DEBUG(%s):ANDROID_JPEG_GPS_COORDINATES(%f,%f,%f)", __FUNCTION__,
                entry.data.d[0], entry.data.d[1], entry.data.d[2]);
    }

    if (settings.exists(ANDROID_JPEG_GPS_PROCESSING_METHOD)) {
        entry = settings.find(ANDROID_JPEG_GPS_PROCESSING_METHOD);

        /* HAKC for Exif CTS Test */
#if 0
        dst->ctl.jpeg.gpsProcessingMethod = entry.data.u8;
#else
        if (strcmp((char *)entry.data.u8, "None") != 0) {
            strncpy((char *)m_gpsProcessingMethod, (char *)entry.data.u8, entry.count);
            strncpy((char *)dst->ctl.jpeg.gpsProcessingMethod, (char *)entry.data.u8, entry.count);
        }
#endif

        ALOGV("DEBUG(%s):ANDROID_JPEG_GPS_PROCESSING_METHOD(%s)", __FUNCTION__,
                dst->ctl.jpeg.gpsProcessingMethod);
    }

    if (settings.exists(ANDROID_JPEG_GPS_TIMESTAMP)) {
        entry = settings.find(ANDROID_JPEG_GPS_TIMESTAMP);
        dst->ctl.jpeg.gpsTimestamp = (uint64_t) entry.data.i64[0];
        ALOGV("DEBUG(%s):ANDROID_JPEG_GPS_TIMESTAMP(%lld)", __FUNCTION__, entry.data.i64[0]);
    }

    if (settings.exists(ANDROID_JPEG_ORIENTATION)) {
        entry = settings.find(ANDROID_JPEG_ORIENTATION);
        dst->ctl.jpeg.orientation = (uint32_t) entry.data.i32[0];
        ALOGV("DEBUG(%s):ANDROID_JPEG_ORIENTATION(%d)", __FUNCTION__, entry.data.i32[0]);
    }

    if (settings.exists(ANDROID_JPEG_QUALITY)) {
        entry = settings.find(ANDROID_JPEG_QUALITY);
        dst->ctl.jpeg.quality = (uint32_t) entry.data.u8[0];
        ALOGV("DEBUG(%s):ANDROID_JPEG_QUALITY(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_JPEG_THUMBNAIL_QUALITY)) {
        entry = settings.find(ANDROID_JPEG_THUMBNAIL_QUALITY);
        dst->ctl.jpeg.thumbnailQuality = (uint32_t) entry.data.u8[0];
        ALOGV("DEBUG(%s):ANDROID_JPEG_THUMBNAIL_QUALITY(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_JPEG_THUMBNAIL_SIZE)) {
        entry = settings.find(ANDROID_JPEG_THUMBNAIL_SIZE);
        for (size_t i = 0; i < entry.count && i < 2; i++)
            dst->ctl.jpeg.thumbnailSize[i] = (uint32_t) entry.data.i32[i];
        ALOGV("DEBUG(%s):ANDROID_JPEG_THUMBNAIL_SIZE(%d,%d)", __FUNCTION__,
                entry.data.i32[0], entry.data.i32[1]);
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateLensControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_LENS_APERTURE)) {
        entry = settings.find(ANDROID_LENS_APERTURE);
        dst->ctl.lens.aperture = entry.data.f[0];
        ALOGV("DEBUG(%s):ANDROID_LENS_APERTURE(%f)", __FUNCTION__, entry.data.f[0]);
    }

    if (settings.exists(ANDROID_LENS_FILTER_DENSITY)) {
        entry = settings.find(ANDROID_LENS_FILTER_DENSITY);
        dst->ctl.lens.filterDensity = entry.data.f[0];
        ALOGV("DEBUG(%s):ANDROID_LENS_FILTER_DENSITY(%f)", __FUNCTION__, entry.data.f[0]);
    }

    if (settings.exists(ANDROID_LENS_FOCAL_LENGTH)) {
        entry = settings.find(ANDROID_LENS_FOCAL_LENGTH);
        dst->ctl.lens.focalLength = entry.data.f[0];
        ALOGV("DEBUG(%s):ANDROID_LENS_FOCAL_LENGTH(%f)", __FUNCTION__, entry.data.f[0]);
    }

    if (settings.exists(ANDROID_LENS_FOCUS_DISTANCE)) {
        entry = settings.find(ANDROID_LENS_FOCUS_DISTANCE);
        /* should not control afMode and focusDistance at the same time
        should not set the same focusDistance continuously
        set the -1 to focusDistance if you do not need to change focusDistance
        */
        if (m_afMode != AA_AFMODE_OFF || m_afMode != m_preAfMode || m_focusDistance == entry.data.f[0]) {
            dst->ctl.lens.focusDistance = -1;
        } else {
            dst->ctl.lens.focusDistance = entry.data.f[0];
        }
        m_focusDistance = dst->ctl.lens.focusDistance;
        ALOGV("DEBUG(%s):ANDROID_LENS_FOCUS_DISTANCE(%f)", __FUNCTION__, entry.data.f[0]);
    }

    if (settings.exists(ANDROID_LENS_OPTICAL_STABILIZATION_MODE)) {
        entry = settings.find(ANDROID_LENS_OPTICAL_STABILIZATION_MODE);
        dst->ctl.lens.opticalStabilizationMode = (enum optical_stabilization_mode) (entry.data.u8[0]);

        switch ((enum optical_stabilization_mode) entry.data.u8[0]) {
        case OPTICAL_STABILIZATION_MODE_ON:
            dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_STILL;
            break;
        case OPTICAL_STABILIZATION_MODE_OFF:
        default:
            dst->ctl.lens.opticalStabilizationMode = OPTICAL_STABILIZATION_MODE_CENTERING;
            break;
        }
        ALOGV("DEBUG(%s):ANDROID_LENS_OPTICAL_STABILIZATION_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateNoiseControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_NOISE_REDUCTION_STRENGTH)) {
        entry = settings.find(ANDROID_NOISE_REDUCTION_STRENGTH);
        dst->ctl.noise.strength = (uint32_t) entry.data.u8[0];
        ALOGV("DEBUG(%s):ANDROID_NOISE_REDUCTION_STRENGTH(%d)", __FUNCTION__,
                dst->ctl.noise.strength);
    }

    if (settings.exists(ANDROID_NOISE_REDUCTION_MODE)) {
        entry = settings.find(ANDROID_NOISE_REDUCTION_MODE);
        dst->ctl.noise.mode = (enum processing_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_NOISE_REDUCTION_MODE(%d)", __FUNCTION__, entry.data.u8[0]);

        switch (dst->ctl.noise.mode) {
        case PROCESSING_MODE_HIGH_QUALITY:
            dst->ctl.noise.strength = 10;
            break;
        case PROCESSING_MODE_FAST:
        case PROCESSING_MODE_OFF:
        case PROCESSING_MODE_MINIMAL:
        default:
            break;
        }
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateRequestControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext, int *reqId)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_REQUEST_ID)) {
        entry = settings.find(ANDROID_REQUEST_ID);
        dst->ctl.request.id = (uint32_t) entry.data.i32[0];
        ALOGV("DEBUG(%s):ANDROID_REQUEST_ID(%d)", __FUNCTION__, entry.data.i32[0]);

        if (reqId != NULL)
            *reqId = dst->ctl.request.id;
    }

    if (settings.exists(ANDROID_REQUEST_METADATA_MODE)) {
        entry = settings.find(ANDROID_REQUEST_METADATA_MODE);
        dst->ctl.request.metadataMode = (enum metadata_mode) entry.data.u8[0];
        ALOGV("DEBUG(%s):ANDROID_REQUEST_METADATA_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateScalerControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_SCALER_CROP_REGION)) {
        entry = settings.find(ANDROID_SCALER_CROP_REGION);
        /* HACK: Temporary save the cropRegion for CTS */
        m_cropRegion.x = entry.data.i32[0];
        m_cropRegion.y = entry.data.i32[1];
        m_cropRegion.w = entry.data.i32[2];
        m_cropRegion.h = entry.data.i32[3];
        for (size_t i = 0; i < entry.count && i < 4; i++)
            dst->ctl.scaler.cropRegion[i] = (uint32_t) entry.data.i32[i];
        ALOGV("DEBUG(%s):ANDROID_SCALER_CROP_REGION(%d,%d,%d,%d)", __FUNCTION__,
                entry.data.i32[0], entry.data.i32[1],
                entry.data.i32[2], entry.data.i32[3]);
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateSensorControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (m_isManualAeControl == true
        && settings.exists(ANDROID_SENSOR_EXPOSURE_TIME)) {
        entry = settings.find(ANDROID_SENSOR_EXPOSURE_TIME);
        dst->ctl.sensor.exposureTime = (uint64_t) entry.data.i64[0];
        ALOGV("DEBUG(%s):ANDROID_SENSOR_EXPOSURE_TIME(%lld)", __FUNCTION__, entry.data.i64[0]);
    }

    if (m_isManualAeControl == true
        && settings.exists(ANDROID_SENSOR_FRAME_DURATION)) {
        entry = settings.find(ANDROID_SENSOR_FRAME_DURATION);
        dst->ctl.sensor.frameDuration = (uint64_t) entry.data.i64[0];
        ALOGV("DEBUG(%s):ANDROID_SENSOR_FRAME_DURATION(%lld)", __FUNCTION__, entry.data.i64[0]);
    } else {
        /* default value */
        dst->ctl.sensor.frameDuration = (1000 * 1000 * 1000) / m_maxFps;
    }

    if (m_isManualAeControl == true
        && settings.exists(ANDROID_SENSOR_SENSITIVITY)) {
        entry = settings.find(ANDROID_SENSOR_SENSITIVITY);
        dst->ctl.aa.vendor_isoMode = AA_ISOMODE_MANUAL;
        dst->ctl.sensor.sensitivity = (uint32_t) entry.data.i32[0];
        dst->ctl.aa.vendor_isoValue = (uint32_t) entry.data.i32[0];
        ALOGV("DEBUG(%s):ANDROID_SENSOR_SENSITIVITY(%d)", __FUNCTION__, entry.data.i32[0]);
    } else {
        dst->ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
        dst->ctl.sensor.sensitivity = 0;
        dst->ctl.aa.vendor_isoValue = 0;
    }

    if (settings.exists(ANDROID_SENSOR_TEST_PATTERN_DATA)) {
        entry = settings.find(ANDROID_SENSOR_TEST_PATTERN_DATA);
        for (size_t i = 0; i < entry.count && i < 4; i++)
            dst->ctl.sensor.testPatternData[i] = entry.data.i32[i];
        ALOGV("DEBUG(%s):ANDROID_SENSOR_TEST_PATTERN_DATA(%d,%d,%d,%d)", __FUNCTION__,
                entry.data.i32[0], entry.data.i32[1], entry.data.i32[2], entry.data.i32[3]);
    }

    if (settings.exists(ANDROID_SENSOR_TEST_PATTERN_MODE)) {
        entry = settings.find(ANDROID_SENSOR_TEST_PATTERN_MODE);
        /* TODO : change SENSOR_TEST_PATTERN_MODE_CUSTOM1 from 256 to 267 */
        if (entry.data.i32[0] == ANDROID_SENSOR_TEST_PATTERN_MODE_CUSTOM1)
            dst->ctl.sensor.testPatternMode = SENSOR_TEST_PATTERN_MODE_CUSTOM1;
        else
            dst->ctl.sensor.testPatternMode = (enum sensor_test_pattern_mode) FIMC_IS_METADATA(entry.data.i32[0]);
        ALOGV("DEBUG(%s):ANDROID_SENSOR_TEST_PATTERN_MODE(%d)", __FUNCTION__, entry.data.i32[0]);
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateShadingControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_SHADING_MODE)) {
        entry = settings.find(ANDROID_SHADING_MODE);
        dst->ctl.shading.mode = (enum processing_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_SHADING_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_SHADING_STRENGTH)) {
        entry = settings.find(ANDROID_SHADING_STRENGTH);
        dst->ctl.shading.strength = (uint32_t) entry.data.u8[0];
        ALOGV("DEBUG(%s):ANDROID_SHADING_STRENGTH(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateStatisticsControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_STATISTICS_FACE_DETECT_MODE)) {
        entry = settings.find(ANDROID_STATISTICS_FACE_DETECT_MODE);
        /* HACK : F/W does NOT support FD Off */
        if (entry.data.u8[0] == ANDROID_STATISTICS_FACE_DETECT_MODE_OFF) {
            m_faceDetectModeOn = false;
            dst_ext->fd_bypass = 1;
        } else {
            m_faceDetectModeOn = true;
            dst_ext->fd_bypass = 0;
        }
        dst->ctl.stats.faceDetectMode = (enum facedetect_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_STATISTICS_FACE_DETECT_MODE(%d)", __FUNCTION__,
                entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_STATISTICS_HISTOGRAM_MODE)) {
        entry = settings.find(ANDROID_STATISTICS_HISTOGRAM_MODE);
        dst->ctl.stats.histogramMode = (enum stats_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_STATISTICS_HISTOGRAM_MODE(%d)", __FUNCTION__,
                entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_STATISTICS_SHARPNESS_MAP_MODE)) {
        entry = settings.find(ANDROID_STATISTICS_SHARPNESS_MAP_MODE);
        dst->ctl.stats.sharpnessMapMode = (enum stats_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_STATISTICS_SHARPNESS_MAP_MODE(%d)", __FUNCTION__,
                entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE)) {
        entry = settings.find(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE);
        dst->ctl.stats.hotPixelMapMode = (enum stats_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE(%d)", __FUNCTION__,
                entry.data.u8[0]);
    }

    if (settings.exists(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE)) {
        entry = settings.find(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE);
        dst->ctl.stats.lensShadingMapMode = (enum stats_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_STATISTICS_LENS_SHADING_MAP_MODE(%d)", __FUNCTION__,
                entry.data.u8[0]);
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateTonemapControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_TONEMAP_MODE)) {
        entry = settings.find(ANDROID_TONEMAP_MODE);
        dst->ctl.tonemap.mode = (enum tonemap_mode) FIMC_IS_METADATA(entry.data.u8[0]);
        ALOGV("DEBUG(%s):ANDROID_TONEMAP_MODE(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    if(dst->ctl.tonemap.mode == TONEMAP_MODE_CONTRAST_CURVE) {
        if (settings.exists(ANDROID_TONEMAP_CURVE_BLUE)) {
            float tonemapCurveBlue[64];

            entry = settings.find(ANDROID_TONEMAP_CURVE_BLUE);
            if (entry.count < 64) {
                if (entry.count == 4) {
                    float deltaIn, deltaOut;

                    deltaIn = entry.data.f[2] - entry.data.f[0];
                    deltaOut = entry.data.f[3] - entry.data.f[1];
                    for (size_t i = 0; i < 61; i += 2) {
                        tonemapCurveBlue[i] = deltaIn * i / 64.0 + entry.data.f[0];
                        tonemapCurveBlue[i+1] = deltaOut * i / 64.0 + entry.data.f[1];
                        ALOGV("DEBUG(%s):ANDROID_TONEMAP_CURVE_BLUE([%d]:%f)", __FUNCTION__, i, tonemapCurveBlue[i]);
                    }
                    tonemapCurveBlue[62] = entry.data.f[2];
                    tonemapCurveBlue[63] = entry.data.f[3];
                } else if (entry.count == 32) {
                    size_t i;
                    for (i = 0; i < 30; i += 2) {
                        tonemapCurveBlue[2*i] = entry.data.f[i];
                        tonemapCurveBlue[2*i+1] = entry.data.f[i+1];
                        tonemapCurveBlue[2*i+2] = (entry.data.f[i] + entry.data.f[i+2])/2;
                        tonemapCurveBlue[2*i+3] = (entry.data.f[i+1] + entry.data.f[i+3])/2;
                    }
                    i = 30;
                    tonemapCurveBlue[2*i] = entry.data.f[i];
                    tonemapCurveBlue[2*i+1] = entry.data.f[i+1];
                    tonemapCurveBlue[2*i+2] = entry.data.f[i];
                    tonemapCurveBlue[2*i+3] = entry.data.f[i+1];
                } else {
                    ALOGE("ERROR(%s):ANDROID_TONEMAP_CURVE_BLUE( entry count : %d)", __FUNCTION__, entry.count);
                }
            } else {
                for (size_t i = 0; i < entry.count && i < 64; i++) {
                    tonemapCurveBlue[i] = entry.data.f[i];
                    ALOGV("DEBUG(%s):ANDROID_TONEMAP_CURVE_BLUE([%d]:%f)", __FUNCTION__, i, entry.data.f[i]);
                }
            }
            memcpy(&(dst->ctl.tonemap.curveBlue[0]), tonemapCurveBlue, sizeof(float)*64);
        }

        if (settings.exists(ANDROID_TONEMAP_CURVE_GREEN)) {
            float tonemapCurveGreen[64];

            entry = settings.find(ANDROID_TONEMAP_CURVE_GREEN);
            if (entry.count < 64) {
                if (entry.count == 4) {
                    float deltaIn, deltaOut;

                    deltaIn = entry.data.f[2] - entry.data.f[0];
                    deltaOut = entry.data.f[3] - entry.data.f[1];
                    for (size_t i = 0; i < 61; i += 2) {
                        tonemapCurveGreen[i] = deltaIn * i / 64.0 + entry.data.f[0];
                        tonemapCurveGreen[i+1] = deltaOut * i / 64.0 + entry.data.f[1];
                        ALOGV("DEBUG(%s):ANDROID_TONEMAP_CURVE_GREEN([%d]:%f)", __FUNCTION__, i, tonemapCurveGreen[i]);
                    }
                    tonemapCurveGreen[62] = entry.data.f[2];
                    tonemapCurveGreen[63] = entry.data.f[3];
                } else if (entry.count == 32) {
                    size_t i;
                    for (i = 0; i < 30; i += 2) {
                        tonemapCurveGreen[2*i] = entry.data.f[i];
                        tonemapCurveGreen[2*i+1] = entry.data.f[i+1];
                        tonemapCurveGreen[2*i+2] = (entry.data.f[i] + entry.data.f[i+2])/2;
                        tonemapCurveGreen[2*i+3] = (entry.data.f[i+1] + entry.data.f[i+3])/2;
                    }
                    i = 30;
                    tonemapCurveGreen[2*i] = entry.data.f[i];
                    tonemapCurveGreen[2*i+1] = entry.data.f[i+1];
                    tonemapCurveGreen[2*i+2] = entry.data.f[i];
                    tonemapCurveGreen[2*i+3] = entry.data.f[i+1];
                } else {
                    ALOGE("ERROR(%s):ANDROID_TONEMAP_CURVE_GREEN( entry count : %d)", __FUNCTION__, entry.count);
                }
            } else {
                for (size_t i = 0; i < entry.count && i < 64; i++) {
                    tonemapCurveGreen[i] = entry.data.f[i];
                    ALOGV("DEBUG(%s):ANDROID_TONEMAP_CURVE_GREEN([%d]:%f)", __FUNCTION__, i, entry.data.f[i]);
                }
            }
            memcpy(&(dst->ctl.tonemap.curveGreen[0]), tonemapCurveGreen, sizeof(float)*64);
        }

        if (settings.exists(ANDROID_TONEMAP_CURVE_RED)) {
            float tonemapCurveRed[64];

            entry = settings.find(ANDROID_TONEMAP_CURVE_RED);
            if (entry.count < 64) {
                if (entry.count == 4) {
                    float deltaIn, deltaOut;

                    deltaIn = entry.data.f[2] - entry.data.f[0];
                    deltaOut = entry.data.f[3] - entry.data.f[1];
                    for (size_t i = 0; i < 61; i += 2) {
                        tonemapCurveRed[i] = deltaIn * i / 64.0 + entry.data.f[0];
                        tonemapCurveRed[i+1] = deltaOut * i / 64.0 + entry.data.f[1];
                        ALOGV("DEBUG(%s):ANDROID_TONEMAP_CURVE_RED([%d]:%f)", __FUNCTION__, i, tonemapCurveRed[i]);
                    }
                    tonemapCurveRed[62] = entry.data.f[2];
                    tonemapCurveRed[63] = entry.data.f[3];
                } else if (entry.count == 32) {
                    size_t i;
                    for (i = 0; i < 30; i += 2) {
                        tonemapCurveRed[2*i] = entry.data.f[i];
                        tonemapCurveRed[2*i+1] = entry.data.f[i+1];
                        tonemapCurveRed[2*i+2] = (entry.data.f[i] + entry.data.f[i+2])/2;
                        tonemapCurveRed[2*i+3] = (entry.data.f[i+1] + entry.data.f[i+3])/2;
                    }
                    i = 30;
                    tonemapCurveRed[2*i] = entry.data.f[i];
                    tonemapCurveRed[2*i+1] = entry.data.f[i+1];
                    tonemapCurveRed[2*i+2] = entry.data.f[i];
                    tonemapCurveRed[2*i+3] = entry.data.f[i+1];
                } else {
                    ALOGE("ERROR(%s):ANDROID_TONEMAP_CURVE_RED( entry count : %d)", __FUNCTION__, entry.count);
                }
            } else {
                for (size_t i = 0; i < entry.count && i < 64; i++) {
                    tonemapCurveRed[i] = entry.data.f[i];
                    ALOGV("DEBUG(%s):ANDROID_TONEMAP_CURVE_RED([%d]:%f)", __FUNCTION__, i, entry.data.f[i]);
                }
            }
            memcpy(&(dst->ctl.tonemap.curveRed[0]), tonemapCurveRed, sizeof(float)*64);
        }
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateLedControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_LED_TRANSMIT)) {
        entry = settings.find(ANDROID_LED_TRANSMIT);
        dst->ctl.led.transmit = (enum led_transmit) entry.data.u8[0];
        ALOGV("DEBUG(%s):ANDROID_LED_TRANSMIT(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateBlackLevelControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext)
{
    struct camera2_shot *dst = NULL;
    camera_metadata_entry_t entry;

    if (settings.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    dst = &dst_ext->shot;
    dst->magicNumber = SHOT_MAGIC_NUMBER;

    if (settings.exists(ANDROID_BLACK_LEVEL_LOCK)) {
        entry = settings.find(ANDROID_BLACK_LEVEL_LOCK);
        dst->ctl.blacklevel.lock = (enum blacklevel_lock) entry.data.u8[0];
        /* HACK : F/W does NOT support thie field */
        if (entry.data.u8[0] == ANDROID_BLACK_LEVEL_LOCK_ON)
            m_blackLevelLockOn = true;
        else
            m_blackLevelLockOn = false;
        ALOGV("DEBUG(%s):ANDROID_BLACK_LEVEL_LOCK(%d)", __FUNCTION__, entry.data.u8[0]);
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::convertRequestToShot(CameraMetadata &request, struct camera2_shot_ext *dst_ext, int *reqId)
{
    status_t ret = OK;
    uint32_t errorFlag = 0;

    if (request.isEmpty()) {
        ALOGE("ERR(%s[%d]):Settings is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (dst_ext == NULL) {
        ALOGE("ERR(%s[%d]):dst_ext is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    ret = translateColorControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 0);
    ret = translateControlControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 1);
    ret = translateDemosaicControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 2);
    ret = translateEdgeControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 3);
    ret = translateFlashControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 4);
    ret = translateHotPixelControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 5);
    ret = translateJpegControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 6);
    ret = translateLensControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 7);
    ret = translateNoiseControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 8);
    ret = translateRequestControlData(request, dst_ext, reqId);
    if (ret != OK)
        errorFlag |= (1 << 9);
    ret = translateScalerControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 10);
    ret = translateSensorControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 11);
    ret = translateShadingControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 12);
    ret = translateStatisticsControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 13);
    ret = translateTonemapControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 14);
    ret = translateLedControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 15);
    ret = translateBlackLevelControlData(request, dst_ext);
    if (ret != OK)
        errorFlag |= (1 << 16);

    if (errorFlag != 0) {
        ALOGE("ERR(%s[%d]):failed to translate Control Data(%d)", __FUNCTION__, __LINE__, errorFlag);
        return INVALID_OPERATION;
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateColorMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    const uint8_t colorMode = (uint8_t) CAMERA_METADATA(src->dm.color.mode);
    settings.update(ANDROID_COLOR_CORRECTION_MODE, &colorMode, 1);
    ALOGV("DEBUG(%s):dm.color.mode(%d)", __FUNCTION__, src->dm.color.mode);

    camera_metadata_rational_t colorTransform[9];
    for (int i = 0; i < 9; i++) {
        colorTransform[i].numerator = (int32_t) src->dm.color.transform[i].num;
        colorTransform[i].denominator = (int32_t) src->dm.color.transform[i].den;
    }
    settings.update(ANDROID_COLOR_CORRECTION_TRANSFORM, colorTransform, 9);
    ALOGV("DEBUG(%s):dm.color.transform",  __FUNCTION__);

    float colorGains[4];
    for (int i = 0; i < 4; i++) {
        colorGains[i] = src->dm.color.gains[i];
    }
    settings.update(ANDROID_COLOR_CORRECTION_GAINS, colorGains, 4);
    ALOGV("DEBUG(%s):dm.color.gains(%f,%f,%f,%f)",  __FUNCTION__,
            colorGains[0], colorGains[1], colorGains[2], colorGains[3]);

    const uint8_t aberrationMode = (uint8_t) CAMERA_METADATA(src->dm.color.aberrationCorrectionMode);
    settings.update(ANDROID_COLOR_CORRECTION_ABERRATION_MODE, &aberrationMode, 1);
    ALOGV("DEBUG(%s):dm.color.aberrationCorrectionMode(%d)", __FUNCTION__,
            src->dm.color.aberrationCorrectionMode);

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateControlMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    CameraMetadata service_settings;
    camera_metadata_entry_t entry;
    camera_metadata_entry_t cropRegionEntry;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;
    uint8_t controlState = 0;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* HACK : F/W does NOT support some fields */
    struct camera2_shot_ext service_shot_ext;
    struct camera2_shot *service_shot = NULL;
    requestInfo->getServiceShot(&service_shot_ext);
    service_shot = &service_shot_ext.shot;

    service_settings = requestInfo->getServiceMeta();
    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    const uint8_t antibandingMode = (uint8_t) CAMERA_METADATA(src->dm.aa.aeAntibandingMode);
    settings.update(ANDROID_CONTROL_AE_ANTIBANDING_MODE, &antibandingMode, 1);
    ALOGV("DEBUG(%s):dm.aa.aeAntibandingMode(%d)",  __FUNCTION__, src->dm.aa.aeAntibandingMode);

    const int32_t aeExposureCompensation = src->dm.aa.aeExpCompensation;
    settings.update(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &aeExposureCompensation, 1);
    ALOGV("DEBUG(%s):dm.aa.aeExpCompensation(%d)", __FUNCTION__, src->dm.aa.aeExpCompensation);
    uint8_t aeMode = ANDROID_CONTROL_AE_MODE_OFF;

    if (src->dm.aa.aeMode == AA_AEMODE_OFF) {
        aeMode = ANDROID_CONTROL_AE_MODE_OFF;
    } else {
        if (m_flashMgr != NULL) {
            switch (m_flashMgr->getFlashReq()) {
            case ExynosCameraActivityFlash::FLASH_REQ_AUTO:
                aeMode = ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH;
                break;
            case ExynosCameraActivityFlash::FLASH_REQ_ON:
                aeMode = ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH;
                break;
            case ExynosCameraActivityFlash::FLASH_REQ_RED_EYE:
                aeMode = ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE;
                break;
            case ExynosCameraActivityFlash::FLASH_REQ_TORCH:
            case ExynosCameraActivityFlash::FLASH_REQ_OFF:
            default:
                aeMode = ANDROID_CONTROL_AE_MODE_ON;
                break;
            }
        }
    }
    settings.update(ANDROID_CONTROL_AE_MODE, &aeMode, 1);
    ALOGV("DEBUG(%s):dm.aa.aeMode(%d), AE_MODE(%d)", __FUNCTION__, src->dm.aa.aeMode, aeMode);

    const uint8_t aeLock = (uint8_t) CAMERA_METADATA(src->dm.aa.aeLock);
    settings.update(ANDROID_CONTROL_AE_LOCK, &aeLock, 1);
    ALOGV("DEBUG(%s):dm.aa.aeLock(%d)", __FUNCTION__, aeLock);

    if (m_cameraId == CAMERA_ID_BACK) {
        /* HACK: Result AE_REGION must be updated based of the value from  F/W */
        int32_t aeRegion[5];
        if (service_settings.exists(ANDROID_SCALER_CROP_REGION) &&
            service_settings.exists(ANDROID_CONTROL_AE_REGIONS)) {
            cropRegionEntry = service_settings.find(ANDROID_SCALER_CROP_REGION);
            entry = service_settings.find(ANDROID_CONTROL_AE_REGIONS);
            /* ae region is bigger than crop region */
            if (cropRegionEntry.data.i32[2] < entry.data.i32[2] - entry.data.i32[0]
                    || cropRegionEntry.data.i32[3] < entry.data.i32[3] - entry.data.i32[1]) {
                aeRegion[0] = cropRegionEntry.data.i32[0];
                aeRegion[1] = cropRegionEntry.data.i32[1];
                aeRegion[2] = cropRegionEntry.data.i32[2] + aeRegion[0];
                aeRegion[3] = cropRegionEntry.data.i32[3] + aeRegion[1];
                aeRegion[4] = entry.data.i32[4];
            } else {
                aeRegion[0] = entry.data.i32[0];
                aeRegion[1] = entry.data.i32[1];
                aeRegion[2] = entry.data.i32[2];
                aeRegion[3] = entry.data.i32[3];
                aeRegion[4] = entry.data.i32[4];
            }
        } else {
            aeRegion[0] = service_shot->ctl.aa.aeRegions[0];
            aeRegion[1] = service_shot->ctl.aa.aeRegions[1];
            aeRegion[2] = service_shot->ctl.aa.aeRegions[2];
            aeRegion[3] = service_shot->ctl.aa.aeRegions[3];
            aeRegion[4] = service_shot->ctl.aa.aeRegions[4];
        }

        settings.update(ANDROID_CONTROL_AE_REGIONS, aeRegion, 5);
        ALOGV("DEBUG(%s):dm.aa.aeRegions(%d,%d,%d,%d,%d)",  __FUNCTION__,
                src->dm.aa.aeRegions[0],
                src->dm.aa.aeRegions[1],
                src->dm.aa.aeRegions[2],
                src->dm.aa.aeRegions[3],
                src->dm.aa.aeRegions[4]);


        /* HACK: Result AWB_REGION must be updated based of the value from  F/W */
        int32_t awbRegion[5];
        if (service_settings.exists(ANDROID_SCALER_CROP_REGION) &&
            service_settings.exists(ANDROID_CONTROL_AWB_REGIONS)) {
            cropRegionEntry = service_settings.find(ANDROID_SCALER_CROP_REGION);
            entry = service_settings.find(ANDROID_CONTROL_AWB_REGIONS);
            /* awb region is bigger than crop region */
            if (cropRegionEntry.data.i32[2] < entry.data.i32[2] - entry.data.i32[0]
                    || cropRegionEntry.data.i32[3] < entry.data.i32[3] - entry.data.i32[1]) {
                awbRegion[0] = cropRegionEntry.data.i32[0];
                awbRegion[1] = cropRegionEntry.data.i32[1];
                awbRegion[2] = cropRegionEntry.data.i32[2] + awbRegion[0];
                awbRegion[3] = cropRegionEntry.data.i32[3] + awbRegion[1];
                awbRegion[4] = entry.data.i32[4];
            } else {
                awbRegion[0] = entry.data.i32[0];
                awbRegion[1] = entry.data.i32[1];
                awbRegion[2] = entry.data.i32[2];
                awbRegion[3] = entry.data.i32[3];
                awbRegion[4] = entry.data.i32[4];
            }
        } else {
            awbRegion[0] = service_shot->ctl.aa.awbRegions[0];
            awbRegion[1] = service_shot->ctl.aa.awbRegions[1];
            awbRegion[2] = service_shot->ctl.aa.awbRegions[2];
            awbRegion[3] = service_shot->ctl.aa.awbRegions[3];
            awbRegion[4] = service_shot->ctl.aa.awbRegions[4];
        }

        settings.update(ANDROID_CONTROL_AWB_REGIONS, awbRegion, 5);
        ALOGV("DEBUG(%s):dm.aa.awbRegions(%d,%d,%d,%d,%d)", __FUNCTION__,
                src->dm.aa.awbRegions[0],
                src->dm.aa.awbRegions[1],
                src->dm.aa.awbRegions[2],
                src->dm.aa.awbRegions[3],
                src->dm.aa.awbRegions[4]);

        /* HACK: Result AF_REGION must be updated based of the value from  F/W */
        int32_t afRegion[5];
        if (service_settings.exists(ANDROID_SCALER_CROP_REGION) &&
            service_settings.exists(ANDROID_CONTROL_AF_REGIONS)) {
            cropRegionEntry = service_settings.find(ANDROID_SCALER_CROP_REGION);
            entry = service_settings.find(ANDROID_CONTROL_AF_REGIONS);
            /* af region is bigger than crop region */
            if (cropRegionEntry.data.i32[2] < entry.data.i32[2] - entry.data.i32[0]
                    || cropRegionEntry.data.i32[3] < entry.data.i32[3] - entry.data.i32[1]) {
                afRegion[0] = cropRegionEntry.data.i32[0];
                afRegion[1] = cropRegionEntry.data.i32[1];
                afRegion[2] = cropRegionEntry.data.i32[2] + afRegion[0];
                afRegion[3] = cropRegionEntry.data.i32[3] + afRegion[1];
                afRegion[4] = entry.data.i32[4];
            } else {
                afRegion[0] = entry.data.i32[0];
                afRegion[1] = entry.data.i32[1];
                afRegion[2] = entry.data.i32[2];
                afRegion[3] = entry.data.i32[3];
                afRegion[4] = entry.data.i32[4];
            }
        } else {
            afRegion[0] = service_shot->ctl.aa.afRegions[0];
            afRegion[1] = service_shot->ctl.aa.afRegions[1];
            afRegion[2] = service_shot->ctl.aa.afRegions[2];
            afRegion[3] = service_shot->ctl.aa.afRegions[3];
            afRegion[4] = service_shot->ctl.aa.afRegions[4];
        }
        settings.update(ANDROID_CONTROL_AF_REGIONS, afRegion, 5);
        ALOGV("DEBUG(%s):dm.aa.afRegions(%d,%d,%d,%d,%d)", __FUNCTION__,
                src->dm.aa.afRegions[0],
                src->dm.aa.afRegions[1],
                src->dm.aa.afRegions[2],
                src->dm.aa.afRegions[3],
                src->dm.aa.afRegions[4]);
    }

    const int32_t aeTargetFps[2] =
    { (int32_t) src->dm.aa.aeTargetFpsRange[0], (int32_t) src->dm.aa.aeTargetFpsRange[1] };
    settings.update(ANDROID_CONTROL_AE_TARGET_FPS_RANGE, aeTargetFps, 2);
    ALOGV("DEBUG(%s):dm.aa.aeTargetFpsRange(%d,%d)", __FUNCTION__,
            src->dm.aa.aeTargetFpsRange[0], src->dm.aa.aeTargetFpsRange[1]);

    const uint8_t aePrecaptureTrigger = (uint8_t) src->dm.aa.aePrecaptureTrigger;
    settings.update(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER, &aePrecaptureTrigger ,1);
    ALOGV("DEBUG(%s):dm.aa.aePrecaptureTrigger(%d)", __FUNCTION__,
            src->dm.aa.aePrecaptureTrigger);

    uint8_t afMode = (uint8_t) CAMERA_METADATA(src->dm.aa.afMode);
    settings.update(ANDROID_CONTROL_AF_MODE, &afMode, 1);
    ALOGV("DEBUG(%s):dm.aa.afMode(%d)", __FUNCTION__, src->dm.aa.afMode);

    const uint8_t afTrigger = (uint8_t )src->dm.aa.afTrigger;
    settings.update(ANDROID_CONTROL_AF_TRIGGER, &afTrigger, 1);
    ALOGV("DEBUG(%s):dm.aa.afTrigger(%d)", __FUNCTION__, src->dm.aa.afTrigger);

    const uint8_t awbLock = (uint8_t) CAMERA_METADATA(src->dm.aa.awbLock);
    settings.update(ANDROID_CONTROL_AWB_LOCK, &awbLock, 1);
    ALOGV("DEBUG(%s):dm.aa.awbLock(%d)", __FUNCTION__, src->dm.aa.awbLock);

    const uint8_t awbMode = (uint8_t) CAMERA_METADATA(src->dm.aa.awbMode);
    settings.update(ANDROID_CONTROL_AWB_MODE, &awbMode, 1);
    ALOGV("DEBUG(%s):dm.aa.awbMode(%d)",  __FUNCTION__, src->dm.aa.awbMode);

    //const uint8_t captureIntent = (uint8_t) src->dm.aa.captureIntent;
    const uint8_t captureIntent = (uint8_t)service_shot->ctl.aa.captureIntent;
    settings.update(ANDROID_CONTROL_CAPTURE_INTENT, &captureIntent, 1);
    ALOGV("DEBUG(%s):dm.aa.captureIntent(%d)",  __FUNCTION__, src->dm.aa.captureIntent);

    const uint8_t effectMode = (uint8_t) CAMERA_METADATA(src->dm.aa.effectMode);
    settings.update(ANDROID_CONTROL_EFFECT_MODE, &effectMode, 1);
    ALOGV("DEBUG(%s):dm.aa.effectMode(%d)",  __FUNCTION__, src->dm.aa.effectMode);

    const uint8_t mode = (uint8_t) CAMERA_METADATA(src->dm.aa.mode);
    settings.update(ANDROID_CONTROL_MODE, &mode, 1);
    ALOGV("DEBUG(%s):dm.aa.mode(%d)",  __FUNCTION__, src->dm.aa.mode);

    uint8_t sceneMode = (uint8_t) CAMERA_METADATA(src->dm.aa.sceneMode);
    /* HACK : Adjust the Scene mode for unsupported scene mode by F/W */
    if (src->dm.aa.sceneMode == AA_SCENE_MODE_HDR)
        sceneMode = ANDROID_CONTROL_SCENE_MODE_HDR;
    else if (service_shot->ctl.aa.sceneMode == AA_SCENE_MODE_FACE_PRIORITY)
        sceneMode = ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY;
    settings.update(ANDROID_CONTROL_SCENE_MODE, &sceneMode, 1);
    ALOGV("DEBUG(%s):dm.aa.sceneMode(%d)", __FUNCTION__, src->dm.aa.sceneMode);

    const uint8_t videoStabilizationMode = (enum aa_videostabilization_mode) src->dm.aa.videoStabilizationMode;
    settings.update(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE, &videoStabilizationMode, 1);
    ALOGV("DEBUG(%s):dm.aa.videoStabilizationMode(%d)", __FUNCTION__, src->dm.aa.videoStabilizationMode);

    uint8_t tmpAeState = (uint8_t) CAMERA_METADATA(src->dm.aa.aeState);
    /* Convert Sec specific AE_STATE_LOCKED_* to valid state value
       (Based on the guide from ist.song@samsung.com) */
    switch(src->dm.aa.aeState) {
        case AE_STATE_LOCKED_CONVERGED:
            ALOGV("DEBUG(%s):dm.aa.aeState(%d) -> Changed to (%d)-AE_STATE_CONVERGED"
                ,  __FUNCTION__, src->dm.aa.aeState, AE_STATE_CONVERGED);
            tmpAeState = (uint8_t) CAMERA_METADATA(AE_STATE_CONVERGED);
            break;

        case AE_STATE_LOCKED_FLASH_REQUIRED:
            ALOGV("DEBUG(%s):dm.aa.aeState(%d) -> Changed to (%d)-AE_STATE_FLASH_REQUIRED"
                ,  __FUNCTION__, src->dm.aa.aeState, AE_STATE_FLASH_REQUIRED);
            tmpAeState = (uint8_t) CAMERA_METADATA(AE_STATE_FLASH_REQUIRED);
            break;
        default:
            // Keep the original value
            break;
    }

    /* HACK: forcely set AE state during init skip count (FW not supported) */
    if (src->dm.request.frameCount < INITIAL_SKIP_FRAME) {
        tmpAeState = (uint8_t) CAMERA_METADATA(AE_STATE_SEARCHING);
    }

#ifdef USE_AE_CONVERGED_UDM
    if (m_cameraId == CAMERA_ID_BACK &&
        tmpAeState == (uint8_t) CAMERA_METADATA(AE_STATE_CONVERGED)) {
        uint32_t aeUdmState = (uint32_t)src->udm.ae.vendorSpecific[397];
        /*  1: converged, 0: searching */
        if (aeUdmState == 0) {
            tmpAeState = (uint8_t) CAMERA_METADATA(AE_STATE_SEARCHING);
        }
    }
#endif

    const uint8_t aeState = tmpAeState;
    settings.update(ANDROID_CONTROL_AE_STATE, &aeState, 1);
    ALOGV("DEBUG(%s):dm.aa.aeState(%d), AE_STATE(%d)",  __FUNCTION__, src->dm.aa.aeState, aeState);

    const uint8_t awbState = (uint8_t) CAMERA_METADATA(src->dm.aa.awbState);
    settings.update(ANDROID_CONTROL_AWB_STATE, &awbState, 1);
    ALOGV("DEBUG(%s):dm.aa.awbState(%d)",  __FUNCTION__, src->dm.aa.awbState);

    const uint8_t afState = (uint8_t) CAMERA_METADATA(src->dm.aa.afState);
    settings.update(ANDROID_CONTROL_AF_STATE, &afState, 1);
    ALOGV("DEBUG(%s):dm.aa.afState(%d)",  __FUNCTION__, src->dm.aa.afState);

    switch (src->dm.aa.aeState) {
    case AE_STATE_CONVERGED:
    case AE_STATE_LOCKED:
        if (m_flashMgr != NULL)
        m_flashMgr->notifyAeResult();
        break;
    case AE_STATE_INACTIVE:
    case AE_STATE_SEARCHING:
    case AE_STATE_FLASH_REQUIRED:
    case AE_STATE_PRECAPTURE:
    default:
        break;
    }

    switch (src->dm.aa.afState) {
    case AA_AFSTATE_FOCUSED_LOCKED:
    case AA_AFSTATE_NOT_FOCUSED_LOCKED:
        if (m_flashMgr != NULL)
            m_flashMgr->notifyAfResultHAL3();
        break;
    case AA_AFSTATE_INACTIVE:
    case AA_AFSTATE_PASSIVE_SCAN:
    case AA_AFSTATE_PASSIVE_FOCUSED:
    case AA_AFSTATE_ACTIVE_SCAN:
    case AA_AFSTATE_PASSIVE_UNFOCUSED:
    default:
        break;
    }

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateEdgeMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    struct camera2_shot_ext service_shot_ext;
    struct camera2_shot *service_shot = NULL;
    requestInfo->getServiceShot(&service_shot_ext);
    service_shot = &service_shot_ext.shot;

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    const uint8_t edgeMode = (uint8_t) CAMERA_METADATA(service_shot->ctl.edge.mode);
    settings.update(ANDROID_EDGE_MODE, &edgeMode, 1);
    ALOGV("DEBUG(%s):dm.edge.mode(%d)",  __FUNCTION__, src->dm.edge.mode);

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateFlashMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;
    uint8_t controlState = 0;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    const uint8_t firingPower = (uint8_t) src->dm.flash.firingPower;
    settings.update(ANDROID_FLASH_FIRING_POWER, &firingPower, 1);
    ALOGV("DEBUG(%s):dm.flash.firingPower(%d)",  __FUNCTION__, src->dm.flash.firingPower);

    const int64_t firingTime = (int64_t) src->dm.flash.firingTime;
    settings.update(ANDROID_FLASH_FIRING_TIME, &firingTime, 1);
    ALOGV("DEBUG(%s):dm.flash.firingTime(%lld)",  __FUNCTION__, src->dm.flash.firingTime);

    const uint8_t flashMode = (uint8_t)CAMERA_METADATA(src->dm.flash.flashMode);
    settings.update(ANDROID_FLASH_MODE, &flashMode, 1);
    ALOGV("DEBUG(%s):dm.flash.flashMode(%d), flashMode=%d",  __FUNCTION__, src->dm.flash.flashMode, flashMode);

    uint8_t flashState = ANDROID_FLASH_STATE_READY;
    if (m_flashMgr == NULL)
        flashState = ANDROID_FLASH_STATE_UNAVAILABLE;
    else if (m_sensorStaticInfo->flashAvailable == ANDROID_FLASH_INFO_AVAILABLE_FALSE)
        flashState = ANDROID_FLASH_STATE_UNAVAILABLE;
    else
        flashState = src->dm.flash.flashState;
    settings.update(ANDROID_FLASH_STATE, &flashState , 1);
    ALOGV("DEBUG(%s):flashState=%d", __FUNCTION__, flashState);

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateHotPixelMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* HACK : F/W does NOT support some fields */
    struct camera2_shot_ext service_shot_ext;
    struct camera2_shot *service_shot = NULL;
    requestInfo->getServiceShot(&service_shot_ext);
    service_shot = &service_shot_ext.shot;

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    //const uint8_t hotPixelMode = (uint8_t) CAMERA_METADATA(src->dm.hotpixel.mode);
    const uint8_t hotPixelMode = (uint8_t) CAMERA_METADATA(service_shot->ctl.hotpixel.mode);
    settings.update(ANDROID_HOT_PIXEL_MODE, &hotPixelMode, 1);
    ALOGV("DEBUG(%s):dm.hotpixel.mode(%d)",  __FUNCTION__, src->dm.hotpixel.mode);

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateJpegMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    settings = requestInfo->getResultMeta();
    requestInfo->getServiceShot(&shot_ext);
    src = &shot_ext.shot;

    const double gpsCoordinates[3] =
    { src->ctl.jpeg.gpsCoordinates[0], src->ctl.jpeg.gpsCoordinates[1], src->ctl.jpeg.gpsCoordinates[2] };
    settings.update(ANDROID_JPEG_GPS_COORDINATES, gpsCoordinates, 3);
    ALOGV("DEBUG(%s):ctl.jpeg.gpsCoordinates(%f,%f,%f)",  __FUNCTION__,
            src->ctl.jpeg.gpsCoordinates[0],
            src->ctl.jpeg.gpsCoordinates[1],
            src->ctl.jpeg.gpsCoordinates[2]);
#if 0
    if (src->ctl.jpeg.gpsProcessingMethod != NULL) {
        size_t gpsProcessingMethodLength = strlen((char *)src->ctl.jpeg.gpsProcessingMethod) + 1;
        uint8_t *gpsProcessingMethod = src->ctl.jpeg.gpsProcessingMethod;
        settings.update(ANDROID_JPEG_GPS_PROCESSING_METHOD, gpsProcessingMethod, gpsProcessingMethodLength);
        ALOGV("DEBUG(%s):ctl.jpeg.gpsProcessingMethod(%s)", __FUNCTION__,
                gpsProcessingMethod);

        if (gpsProcessingMethod != NULL) {
            free(gpsProcessingMethod);
            gpsProcessingMethod = NULL;
        }
    }
#endif
    const int64_t gpsTimestamp = (int64_t) src->ctl.jpeg.gpsTimestamp;
    settings.update(ANDROID_JPEG_GPS_TIMESTAMP, &gpsTimestamp, 1);
    ALOGV("DEBUG(%s):ctl.jpeg.gpsTimestamp(%lld)",  __FUNCTION__,
            src->ctl.jpeg.gpsTimestamp);

    const int32_t orientation = src->ctl.jpeg.orientation;
    settings.update(ANDROID_JPEG_ORIENTATION, &orientation, 1);
    ALOGV("DEBUG(%s):ctl.jpeg.orientation(%d)", __FUNCTION__, src->ctl.jpeg.orientation);

    const uint8_t quality = (uint8_t) src->ctl.jpeg.quality;
    settings.update(ANDROID_JPEG_QUALITY, &quality, 1);
    ALOGV("DEBUG(%s):ctl.jpeg.quality(%d)", __FUNCTION__, src->ctl.jpeg.quality);

    const uint8_t thumbnailQuality = (uint8_t) src->ctl.jpeg.thumbnailQuality;
    settings.update(ANDROID_JPEG_THUMBNAIL_QUALITY, &thumbnailQuality, 1);
    ALOGV("DEBUG(%s):ctl.jpeg.thumbnailQuality(%d)", __FUNCTION__,
            src->ctl.jpeg.thumbnailQuality);

    const int32_t thumbnailSize[2] =
    { (int32_t) src->ctl.jpeg.thumbnailSize[0], (int32_t) src->ctl.jpeg.thumbnailSize[1] };
    settings.update(ANDROID_JPEG_THUMBNAIL_SIZE, thumbnailSize, 2);
    ALOGV("DEBUG(%s):ctl.jpeg.thumbnailSize(%d,%d)", __FUNCTION__,
            src->ctl.jpeg.thumbnailSize[0], src->ctl.jpeg.thumbnailSize[1]);

    const int32_t jpegSize = (int32_t) src->dm.jpeg.size;
    settings.update(ANDROID_JPEG_SIZE, &jpegSize, 1);
    ALOGV("DEBUG(%s):dm.jpeg.size(%d)", __FUNCTION__, src->dm.jpeg.size);

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateLensMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;
    uint8_t controlState = 0;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    settings.update(ANDROID_LENS_APERTURE, &(m_sensorStaticInfo->fNumber), 1);
    ALOGV("DEBUG(%s):dm.lens.aperture is fNumber(%f)", __FUNCTION__, m_sensorStaticInfo->fNumber);

    settings.update(ANDROID_LENS_FILTER_DENSITY, &m_sensorStaticInfo->filterDensity, 1);
    ALOGV("DEBUG(%s):dm.lens.filterDensity(%f)", __FUNCTION__, m_sensorStaticInfo->filterDensity);

    settings.update(ANDROID_LENS_FOCAL_LENGTH, &(m_sensorStaticInfo->focalLength), 1);
    ALOGV("DEBUG(%s):dm.lens.focalLength(%f)", __FUNCTION__, m_sensorStaticInfo->focalLength);

    /* Focus distance 0 means infinite */
    float focusDistance = src->dm.lens.focusDistance;
    if(focusDistance < 0) {
        focusDistance = 0;
    } else if (focusDistance > m_sensorStaticInfo->minimumFocusDistance) {
        focusDistance = m_sensorStaticInfo->minimumFocusDistance;
    }
    settings.update(ANDROID_LENS_FOCUS_DISTANCE, &focusDistance, 1);
    ALOGV("DEBUG(%s):dm.lens.focusDistance(%f)", __FUNCTION__, src->dm.lens.focusDistance);

    uint8_t opticalStabilizationMode = (uint8_t) src->dm.lens.opticalStabilizationMode;

    switch (opticalStabilizationMode) {
    case OPTICAL_STABILIZATION_MODE_STILL:
        opticalStabilizationMode = ANDROID_LENS_OPTICAL_STABILIZATION_MODE_ON;
        break;
    case OPTICAL_STABILIZATION_MODE_VIDEO:
        opticalStabilizationMode = ANDROID_LENS_OPTICAL_STABILIZATION_MODE_ON;
        break;
    case OPTICAL_STABILIZATION_MODE_CENTERING:
    default:
        opticalStabilizationMode = ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
        break;
    }
    settings.update(ANDROID_LENS_OPTICAL_STABILIZATION_MODE, &opticalStabilizationMode, 1);
    ALOGV("DEBUG(%s):dm.lens.opticalStabilizationMode(%d)", __FUNCTION__,
            src->dm.lens.opticalStabilizationMode);

    const uint8_t lensState = src->dm.lens.state;
    settings.update(ANDROID_LENS_STATE, &lensState, 1);
    ALOGV("DEBUG(%s):dm.lens.state(%d)", __FUNCTION__, src->dm.lens.state);

    const float focusRange[2] =
    { src->dm.lens.focusRange[0], src->dm.lens.focusRange[1] };
    settings.update(ANDROID_LENS_FOCUS_RANGE, focusRange, 2);
    ALOGV("DEBUG(%s):dm.lens.focusRange(%f,%f)", __FUNCTION__,
            focusRange[0], focusRange[1]);

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateNoiseMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    struct camera2_shot_ext service_shot_ext;
    struct camera2_shot *service_shot = NULL;
    requestInfo->getServiceShot(&service_shot_ext);
    service_shot = &service_shot_ext.shot;

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    uint8_t noiseReductionMode = (uint8_t) CAMERA_METADATA(service_shot->ctl.noise.mode);
    //uint8_t noiseReductionMode = (uint8_t) CAMERA_METADATA(src->dm.noise.mode);
    settings.update(ANDROID_NOISE_REDUCTION_MODE, &noiseReductionMode, 1);
    ALOGV("DEBUG(%s):dm.noise.mode(%d)", __FUNCTION__, src->dm.noise.mode);

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateQuirksMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    //settings.update(ANDROID_QUIRKS_PARTIAL_RESULT, ,);

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateRequestMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    src->dm.request.id = requestInfo->getRequestId();

    const int32_t requestId = src->dm.request.id;
    settings.update(ANDROID_REQUEST_ID, &requestId, 1);
    ALOGV("DEBUG(%s):dm.request.id(%d)", __FUNCTION__, src->dm.request.id);

    const uint8_t metadataMode = src->dm.request.metadataMode;
    settings.update(ANDROID_REQUEST_METADATA_MODE, &metadataMode, 1);
    ALOGV("DEBUG(%s):dm.request.metadataMode(%d)",  __FUNCTION__,
            src->dm.request.metadataMode);

/*
 *
 * pipelineDepth is filed of 'REQUEST'
 *
 * but updating pipelineDepth data can be conflict
 * and we separeted this data not using data but request's private data
 *
 * remaining this code as comment is that to prevent missing update pieplineDepth data in the medta of 'REQUEST' field
 *
 */
/*
 *   const uint8_t pipelineDepth = src->dm.request.pipelineDepth;
 *   settings.update(ANDROID_REQUEST_PIPELINE_DEPTH, &pipelineDepth, 1);
 *   ALOGV("DEBUG(%s):ANDROID_REQUEST_PIPELINE_DEPTH(%d)", __FUNCTION__, pipelineDepth);
 */

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateScalerMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    const int32_t cropRegion[4] =
    {
        m_cropRegion.x,
        m_cropRegion.y,
        m_cropRegion.w,
        m_cropRegion.h
    };
    settings.update(ANDROID_SCALER_CROP_REGION, cropRegion, 4);
    ALOGV("DEBUG(%s):dm.scaler.cropRegion(%d,%d,%d,%d)", __FUNCTION__,
            src->dm.scaler.cropRegion[0],
            src->dm.scaler.cropRegion[1],
            src->dm.scaler.cropRegion[2],
            src->dm.scaler.cropRegion[3]);

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateSensorMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* HACK : F/W does NOT support some fields */
    struct camera2_shot_ext service_shot_ext;
    struct camera2_shot *service_shot = NULL;
    requestInfo->getServiceShot(&service_shot_ext);
    service_shot = &service_shot_ext.shot;

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    int64_t frameDuration = (int64_t) src->dm.sensor.frameDuration;
    if (frameDuration == 0) {
        frameDuration = service_shot_ext.shot.ctl.sensor.frameDuration;
    }
    settings.update(ANDROID_SENSOR_FRAME_DURATION, &frameDuration, 1);
    ALOGV("DEBUG(%s):dm.sensor.frameDuration(%lld)",  __FUNCTION__, src->dm.sensor.frameDuration);

    int64_t exposureTime = (int64_t)src->dm.sensor.exposureTime;
    if (exposureTime == 0 || exposureTime > frameDuration) {
        exposureTime = frameDuration;
    }
    src->dm.sensor.exposureTime = exposureTime; //  for  EXIF Data
    settings.update(ANDROID_SENSOR_EXPOSURE_TIME, &exposureTime, 1);

    int32_t sensitivity = (int32_t) src->dm.sensor.sensitivity;
    if (sensitivity < m_sensorStaticInfo->sensitivityRange[MIN]) {
        sensitivity = m_sensorStaticInfo->sensitivityRange[MIN];
    } else if (sensitivity > m_sensorStaticInfo->sensitivityRange[MAX]) {
        sensitivity = m_sensorStaticInfo->sensitivityRange[MAX];
    }
    src->dm.sensor.sensitivity = sensitivity; //  for  EXIF Data
    settings.update(ANDROID_SENSOR_SENSITIVITY, &sensitivity, 1);

    ALOGV("DEBUG(%s):[frameCount is %d] exposureTime(%lld) sensitivity(%d)", __FUNCTION__,
            src->dm.request.frameCount, exposureTime, sensitivity);

    int32_t testPatternMode = (int32_t) CAMERA_METADATA(src->dm.sensor.testPatternMode);
    if (src->dm.sensor.testPatternMode == SENSOR_TEST_PATTERN_MODE_CUSTOM1)
        testPatternMode = ANDROID_SENSOR_TEST_PATTERN_MODE_CUSTOM1;
    settings.update(ANDROID_SENSOR_TEST_PATTERN_MODE, &testPatternMode, 1);
    ALOGV("DEBUG(%s):dm.sensor.testPatternMode(%d)", __FUNCTION__,
            src->dm.sensor.testPatternMode);

    const int32_t testPatternData[4] =
    {
        src->dm.sensor.testPatternData[0], src->dm.sensor.testPatternData[1],
        src->dm.sensor.testPatternData[2], src->dm.sensor.testPatternData[3]
    };
    settings.update(ANDROID_SENSOR_TEST_PATTERN_DATA, testPatternData, 4);
    ALOGV("DEBUG(%s):dm.sensor.testPatternData(%d,%d,%d,%d)", __FUNCTION__,
            src->dm.sensor.testPatternData[0], src->dm.sensor.testPatternData[1],
            src->dm.sensor.testPatternData[2], src->dm.sensor.testPatternData[3]);

    const int64_t timeStamp = (int64_t) src->udm.sensor.timeStampBoot;
    settings.update(ANDROID_SENSOR_TIMESTAMP, &timeStamp, 1);
    ALOGV("DEBUG(%s):udm.sensor.timeStampBoot(%lld)",  __FUNCTION__, src->udm.sensor.timeStampBoot);

    const camera_metadata_rational_t neutralColorPoint[3] =
    {
        {(int32_t) src->dm.sensor.neutralColorPoint[0].num,
         (int32_t) src->dm.sensor.neutralColorPoint[0].den},
        {(int32_t) src->dm.sensor.neutralColorPoint[1].num,
         (int32_t) src->dm.sensor.neutralColorPoint[1].den},
        {(int32_t) src->dm.sensor.neutralColorPoint[2].num,
         (int32_t) src->dm.sensor.neutralColorPoint[2].den}
    };

    settings.update(ANDROID_SENSOR_NEUTRAL_COLOR_POINT, neutralColorPoint, 3);
    ALOGV("DEBUG(%s):dm.sensor.neutralColorPoint(%d/%d,%d/%d,%d/%d)", __FUNCTION__,
            src->dm.sensor.neutralColorPoint[0].num,
            src->dm.sensor.neutralColorPoint[0].den,
            src->dm.sensor.neutralColorPoint[1].num,
            src->dm.sensor.neutralColorPoint[1].den,
            src->dm.sensor.neutralColorPoint[2].num,
            src->dm.sensor.neutralColorPoint[2].den);

    /* HACK : F/W does NOT support this field */
    const double noiseProfile[8] =
    {
        src->dm.sensor.noiseProfile[0][0], src->dm.sensor.noiseProfile[0][1],
        src->dm.sensor.noiseProfile[1][0], src->dm.sensor.noiseProfile[1][1],
        src->dm.sensor.noiseProfile[2][0], src->dm.sensor.noiseProfile[2][1],
        src->dm.sensor.noiseProfile[3][0], src->dm.sensor.noiseProfile[3][1]
    };
    settings.update(ANDROID_SENSOR_NOISE_PROFILE, noiseProfile , 8);
    ALOGV("DEBUG(%s):dm.sensor.noiseProfile({%f,%f},{%f,%f},{%f,%f},{%f,%f})", __FUNCTION__,
            src->dm.sensor.noiseProfile[0][0],
            src->dm.sensor.noiseProfile[0][1],
            src->dm.sensor.noiseProfile[1][0],
            src->dm.sensor.noiseProfile[1][1],
            src->dm.sensor.noiseProfile[2][0],
            src->dm.sensor.noiseProfile[2][1],
            src->dm.sensor.noiseProfile[3][0],
            src->dm.sensor.noiseProfile[3][1]);

    const float greenSplit = src->dm.sensor.greenSplit;
    settings.update(ANDROID_SENSOR_GREEN_SPLIT, &greenSplit, 1);
    ALOGV("DEBUG(%s):dm.sensor.greenSplit(%f)", __FUNCTION__, src->dm.sensor.greenSplit);

    const int64_t rollingShutterSkew = (int64_t) src->dm.sensor.rollingShutterSkew;
    settings.update(ANDROID_SENSOR_ROLLING_SHUTTER_SKEW, &rollingShutterSkew, 1);
    ALOGV("DEBUG(%s):dm.sensor.rollingShutterSkew(%lld)", __FUNCTION__,
            src->dm.sensor.rollingShutterSkew);

    //settings.update(ANDROID_SENSOR_TEMPERATURE, , );
    //settings.update(ANDROID_SENSOR_PROFILE_HUE_SAT_MAP, , );
    //settings.update(ANDROID_SENSOR_PROFILE_TONE_CURVE, , );

    requestInfo->setResultMeta(settings);

    /* HACK: SensorMetaData sync with shot_ext. These values should be used for EXIF */
    requestInfo->setResultShot(&shot_ext);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateShadingMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* HACK : F/W does NOT support some fields */
    struct camera2_shot_ext service_shot_ext;
    struct camera2_shot *service_shot = NULL;
    requestInfo->getServiceShot(&service_shot_ext);
    service_shot = &service_shot_ext.shot;

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    //const uint8_t shadingMode = (uint8_t) CAMERA_METADATA(src->dm.shading.mode);
    const uint8_t shadingMode = (uint8_t) CAMERA_METADATA(service_shot->ctl.shading.mode);
    settings.update(ANDROID_SHADING_MODE, &shadingMode, 1);
    ALOGV("DEBUG(%s):dm.shading.mode(%d)",  __FUNCTION__, src->dm.shading.mode);

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateStatisticsMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot_ext service_shot_ext;
    struct camera2_shot *src = NULL;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    settings = requestInfo->getResultMeta();
    requestInfo->getServiceShot(&service_shot_ext);
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;


    src->dm.stats.faceDetectMode = service_shot_ext.shot.ctl.stats.faceDetectMode;
    const uint8_t faceDetectMode = (uint8_t) CAMERA_METADATA(src->dm.stats.faceDetectMode);
    settings.update(ANDROID_STATISTICS_FACE_DETECT_MODE, &faceDetectMode, 1);
    ALOGV("DEBUG(%s):dm.stats.faceDetectMode(%d)",  __FUNCTION__,
            src->dm.stats.faceDetectMode);

    if (faceDetectMode > ANDROID_STATISTICS_FACE_DETECT_MODE_OFF)
        m_updateFaceDetectionMetaData(&settings, &shot_ext);

    const uint8_t histogramMode = (uint8_t) CAMERA_METADATA(src->dm.stats.histogramMode);
    settings.update(ANDROID_STATISTICS_HISTOGRAM_MODE, &histogramMode, 1);
    ALOGV("DEBUG(%s):dm.stats.histogramMode(%d)",  __FUNCTION__,
            src->dm.stats.histogramMode);

    const uint8_t sharpnessMapMode = (uint8_t) CAMERA_METADATA(src->dm.stats.sharpnessMapMode);
    settings.update(ANDROID_STATISTICS_SHARPNESS_MAP_MODE, &sharpnessMapMode, 1);
    ALOGV("DEBUG(%s):dm.stats.sharpnessMapMode(%d)",  __FUNCTION__,
            src->dm.stats.sharpnessMapMode);

    const uint8_t hotPixelMapMode = (uint8_t) CAMERA_METADATA(src->dm.stats.hotPixelMapMode);
    settings.update(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE, &hotPixelMapMode, 1);
    ALOGV("DEBUG(%s):dm.stats.hotPixelMapMode(%d)", __FUNCTION__,
        src->dm.stats.hotPixelMapMode);

    /* HACK : F/W does NOT support this field */
    //int32_t *hotPixelMap = (int32_t *) src->dm.stats.hotPixelMap;
    const int32_t hotPixelMap[] = {};
    settings.update(ANDROID_STATISTICS_HOT_PIXEL_MAP, hotPixelMap, ARRAY_LENGTH(hotPixelMap));
    ALOGV("DEBUG(%s):dm.stats.hotPixelMap", __FUNCTION__);

    const uint8_t lensShadingMapMode = (uint8_t) src->dm.stats.lensShadingMapMode;
    settings.update(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE, &lensShadingMapMode, 1);
    ALOGV("DEBUG(%s):dm.stats.lensShadingMapMode(%d)", __FUNCTION__,
        src->dm.stats.lensShadingMapMode);

    /* HACK : F/W does NOT support this field */
    //float *lensShadingMap = (float *) src->dm.stats.lensShadingMap;
    const float lensShadingMap[] = {1.0, 1.0, 1.0, 1.0};
    settings.update(ANDROID_STATISTICS_LENS_SHADING_MAP, lensShadingMap, 4);
    ALOGV("DEBUG(%s):dm.stats.lensShadingMap(%f,%f,%f,%f)", __FUNCTION__,
            lensShadingMap[0], lensShadingMap[1],
            lensShadingMap[2], lensShadingMap[3]);

    uint8_t sceneFlicker = (uint8_t) CAMERA_METADATA(src->dm.stats.sceneFlicker);
    settings.update(ANDROID_STATISTICS_SCENE_FLICKER, &sceneFlicker, 1);
    ALOGV("DEBUG(%s):dm.stats.sceneFlicker(%d)", __FUNCTION__, src->dm.stats.sceneFlicker);

    //settings.update(ANDROID_STATISTICS_HISTOGRAM, , );
    //settings.update(ANDROID_STATISTICS_SHARPNESS_MAP, , );
    //settings.update(ANDROID_STATISTICS_LENS_SHADING_CORRECTION_MAP, , );

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateTonemapMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;
    uint8_t controlState = 0;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    /* HACK : F/W does NOT support some fields */
    struct camera2_shot_ext service_shot_ext;
    struct camera2_shot *service_shot = NULL;
    requestInfo->getServiceShot(&service_shot_ext);
    service_shot = &service_shot_ext.shot;

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    float *curveBlue = (float *) src->dm.tonemap.curveBlue;
    settings.update(ANDROID_TONEMAP_CURVE_BLUE, curveBlue, 64);
    ALOGV("DEBUG(%s):dm.tonemap.curveBlue", __FUNCTION__);

    float *curveGreen = (float *) src->dm.tonemap.curveGreen;
    settings.update(ANDROID_TONEMAP_CURVE_GREEN, curveGreen, 64);
    ALOGV("DEBUG(%s):dm.tonemap.curveGreen", __FUNCTION__);

    float *curveRed = (float *) src->dm.tonemap.curveRed;
    settings.update(ANDROID_TONEMAP_CURVE_RED, curveRed, 64);
    ALOGV("DEBUG(%s):dm.tonemap.curveRed", __FUNCTION__);

    //const uint8_t toneMapMode = (uint8_t) CAMERA_METADATA(src->dm.tonemap.mode);
    const uint8_t toneMapMode = (uint8_t) CAMERA_METADATA(service_shot->ctl.tonemap.mode);
    settings.update(ANDROID_TONEMAP_MODE, &toneMapMode, 1);
    ALOGV("DEBUG(%s):dm.tonemap.mode(%d)", __FUNCTION__, src->dm.tonemap.mode);

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateLedMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    //settings.update(ANDROID_LED_TRANSMIT, (uint8_t *) NULL, 0);
    ALOGV("DEBUG(%s):dm.led.transmit(%d)", __FUNCTION__, src->dm.led.transmit);

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::translateBlackLevelMetaData(ExynosCameraRequest *requestInfo)
{
    CameraMetadata settings;
    struct camera2_shot_ext shot_ext;
    struct camera2_shot *src = NULL;

    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    settings = requestInfo->getResultMeta();
    requestInfo->getResultShot(&shot_ext);
    src = &shot_ext.shot;

    /* HACK: F/W does NOT support this field */
    //const uint8_t blackLevelLock = (uint8_t) src->dm.blacklevel.lock;
    const uint8_t blackLevelLock = (uint8_t) m_blackLevelLockOn;
    settings.update(ANDROID_BLACK_LEVEL_LOCK, &blackLevelLock, 1);
    ALOGV("DEBUG(%s):dm.blacklevel.lock(%d)", __FUNCTION__, src->dm.blacklevel.lock);

    requestInfo->setResultMeta(settings);

    return OK;
}

status_t ExynosCamera3MetadataConverter::updateDynamicMeta(ExynosCameraRequest *requestInfo)
{
    status_t ret = OK;
    uint32_t errorFlag = 0;

    ALOGV("DEBUG(%s[%d]):%d frame", __FUNCTION__, __LINE__, requestInfo->getFrameCount());
    /* Validation check */
    if (requestInfo == NULL) {
        ALOGE("ERR(%s[%d]):RequestInfo is NULL!!", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    ret = translateColorMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 0);
    ret = translateControlMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 1);
    ret = translateEdgeMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 2);
    ret = translateFlashMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 3);
    ret = translateHotPixelMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 4);
    ret = translateJpegMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 5);
    ret = translateLensMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 6);
    ret = translateNoiseMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 7);
    ret = translateQuirksMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 8);
    ret = translateRequestMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 9);
    ret = translateScalerMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 10);
    ret = translateSensorMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 11);
    ret = translateShadingMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 12);
    ret = translateStatisticsMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 13);
    ret = translateTonemapMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 14);
    ret = translateLedMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 15);
    ret = translateBlackLevelMetaData(requestInfo);
    if (ret != OK)
        errorFlag |= (1 << 16);

    if (errorFlag != 0) {
        ALOGE("ERR(%s[%d]):failed to translate Meta Data(%d)", __FUNCTION__, __LINE__, errorFlag);
        return INVALID_OPERATION;
    }

    return OK;
}

status_t ExynosCamera3MetadataConverter::checkAvailableStreamFormat(int format)
{
    int ret = OK;
    ALOGD("DEBUG(%s[%d]) format(%d)", __FUNCTION__, __LINE__, format);

    // TODO:check available format
    return ret;
}

status_t ExynosCamera3MetadataConverter::m_createControlAvailableHighSpeedVideoConfigurations(
        const struct ExynosSensorInfoBase *sensorStaticInfo,
        Vector<int32_t> *streamConfigs,
        int cameraId)
{
    status_t ret = NO_ERROR;
    int (*highSpeedVideoSizeList)[3] = NULL;
    int highSpeedVideoSizeListLength = 0;
    int (*highSpeedVideoFPSList)[2] = NULL;
    int highSpeedVideoFPSListLength = 0;
    int streamConfigSize = 0;

    if (sensorStaticInfo == NULL) {
        ALOGE("ERR(%s[%d]):Sensor static info is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        ALOGE("ERR(%s[%d]):Stream configs is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (cameraId == CAMERA_ID_FRONT) {
        ALOGD("DEBUG(%s[%d]) Front camera does not support High Speed Video", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    highSpeedVideoSizeList = sensorStaticInfo->highSpeedVideoList;
    highSpeedVideoSizeListLength = sensorStaticInfo->highSpeedVideoListMax;
    highSpeedVideoFPSList = sensorStaticInfo->highSpeedVideoFPSList;
    highSpeedVideoFPSListLength = sensorStaticInfo->highSpeedVideoFPSListMax;

    streamConfigSize = (highSpeedVideoSizeListLength * highSpeedVideoFPSListLength * 5);

    for (int i = 0; i < highSpeedVideoFPSListLength; i++) {
        for (int j = 0; j < highSpeedVideoSizeListLength; j++) {
            streamConfigs->add(highSpeedVideoSizeList[j][0]);
            streamConfigs->add(highSpeedVideoSizeList[j][1]);
            streamConfigs->add(highSpeedVideoFPSList[i][0]/1000);
            streamConfigs->add(highSpeedVideoFPSList[i][1]/1000);
            streamConfigs->add(1);
        }
    }

    return ret;
}

/*
   - Returns NO_ERROR if private reprocessing is supported: streamConfigs will have valid entries.
   - Returns NAME_NOT_FOUND if private reprocessing is not supported: streamConfigs will be returned as is,
     and scaler.AvailableInputOutputFormatsMap should not be updated.
*/
status_t ExynosCamera3MetadataConverter::m_createScalerAvailableInputOutputFormatsMap(const struct ExynosSensorInfoBase *sensorStaticInfo,
                                                                         Vector<int32_t> *streamConfigs,
                                                                         __unused int cameraId)
{
    int streamConfigSize = 0;
    bool isSupportPrivReprocessing = false;

    if (sensorStaticInfo == NULL) {
        ALOGE("ERR(%s[%d]):Sensor static info is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        ALOGE("ERR(%s[%d]):Stream configs is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    isSupportPrivReprocessing = m_hasTagInList(
            sensorStaticInfo->capabilities,
            sensorStaticInfo->capabilitiesLength,
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES_PRIVATE_REPROCESSING);

    if(isSupportPrivReprocessing == true) {
        streamConfigs->add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
        streamConfigs->add(2);
        streamConfigs->add(HAL_PIXEL_FORMAT_YCbCr_420_888);
        streamConfigs->add(HAL_PIXEL_FORMAT_BLOB);
        streamConfigs->setCapacity(streamConfigSize);

        return NO_ERROR;
    } else {
        return NAME_NOT_FOUND;
    }
}

status_t ExynosCamera3MetadataConverter::m_createScalerAvailableStreamConfigurationsOutput(const struct ExynosSensorInfoBase *sensorStaticInfo,
                                                                                           Vector<int32_t> *streamConfigs,
                                                                                           int cameraId)
{
    status_t ret = NO_ERROR;
    int (*yuvSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int yuvSizeListLength = 0;
    int (*jpegSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int jpegSizeListLength = 0;
    int streamConfigSize = 0;
    bool isSupportHighResolution = false;
    bool isSupportPrivReprocessing = false;

    if (sensorStaticInfo == NULL) {
        ALOGE("ERR(%s[%d]):Sensor static info is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (streamConfigs == NULL) {
        ALOGE("ERR(%s[%d]):Stream configs is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    isSupportHighResolution = m_hasTagInList(
            sensorStaticInfo->capabilities,
            sensorStaticInfo->capabilitiesLength,
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE);

    if (cameraId == CAMERA_ID_BACK) {
        yuvSizeList = sensorStaticInfo->rearPreviewList;
        yuvSizeListLength = sensorStaticInfo->rearPreviewListMax;
        jpegSizeList = sensorStaticInfo->rearPictureList;
        jpegSizeListLength = sensorStaticInfo->rearPictureListMax;
    } else { /* CAMERA_ID_FRONT */
        yuvSizeList = sensorStaticInfo->frontPreviewList;
        yuvSizeListLength = sensorStaticInfo->frontPreviewListMax;
        jpegSizeList = sensorStaticInfo->frontPictureList;
        jpegSizeListLength = sensorStaticInfo->frontPictureListMax;
    }

    /* Check wheather the private reprocessing is supported or not */
    isSupportPrivReprocessing = m_hasTagInList(
            sensorStaticInfo->capabilities,
            sensorStaticInfo->capabilitiesLength,
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES_PRIVATE_REPROCESSING);

    /* TODO: Add YUV reprocessing if necessary */

    /* HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED stream configuration list size */
    streamConfigSize = yuvSizeListLength * 4;
    /* YUV output stream configuration list size */
    streamConfigSize += (yuvSizeListLength * 4) * (ARRAY_LENGTH(YUV_FORMATS));
    /* Stall output stream configuration list size */
    streamConfigSize += (jpegSizeListLength * 4) * (ARRAY_LENGTH(STALL_FORMATS));
    /* RAW output stream configuration list size */
    streamConfigSize += (1 * 4) * (ARRAY_LENGTH(RAW_FORMATS));
    /* ZSL input stream configuration list size */
    if(isSupportPrivReprocessing == true) {
        streamConfigSize += 4;
    }
    streamConfigs->setCapacity(streamConfigSize);

    /* HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED stream supported size list */
    for (int i = 0; i < yuvSizeListLength; i++) {
        streamConfigs->add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
        streamConfigs->add(yuvSizeList[i][0]);
        streamConfigs->add(yuvSizeList[i][1]);
        streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
    }

    /* YUV output stream supported size list */
    for (size_t i = 0; i < ARRAY_LENGTH(YUV_FORMATS); i++) {
        for (int j = 0; j < yuvSizeListLength; j++) {
            int pixelSize = yuvSizeList[j][0] * yuvSizeList[j][1];
            if (isSupportHighResolution == false
                && pixelSize > HIGH_RESOLUTION_MIN_PIXEL_SIZE) {
                streamConfigSize -= 4;
                continue;
            }

            streamConfigs->add(YUV_FORMATS[i]);
            streamConfigs->add(yuvSizeList[j][0]);
            streamConfigs->add(yuvSizeList[j][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

    /* Stall output stream supported size list */
    for (size_t i = 0; i < ARRAY_LENGTH(STALL_FORMATS); i++) {
        for (int j = 0; j < jpegSizeListLength; j++) {
            int pixelSize = jpegSizeList[j][0] * jpegSizeList[j][1];
            if (isSupportHighResolution == false
                && pixelSize > HIGH_RESOLUTION_MIN_PIXEL_SIZE) {
                streamConfigSize -= 4;
                continue;
            }

            streamConfigs->add(STALL_FORMATS[i]);
            streamConfigs->add(jpegSizeList[j][0]);
            streamConfigs->add(jpegSizeList[j][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
        }
    }

    /* RAW output stream supported size list */
    for (size_t i = 0; i < ARRAY_LENGTH(RAW_FORMATS); i++) {
        /* Add sensor max size */
        streamConfigs->add(RAW_FORMATS[i]);
        streamConfigs->add(sensorStaticInfo->maxSensorW);
        streamConfigs->add(sensorStaticInfo->maxSensorH);
        streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT);
    }

    /* ZSL input stream supported size list */
    {
        if(isSupportPrivReprocessing == true) {
            streamConfigs->add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
            streamConfigs->add(yuvSizeList[0][0]);
            streamConfigs->add(yuvSizeList[0][1]);
            streamConfigs->add(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT);
        }
    }

    streamConfigs->setCapacity(streamConfigSize);

#ifdef DEBUG_STREAM_CONFIGURATIONS
    const int32_t* streamConfigArray = NULL;
    streamConfigArray = streamConfigs->array();
    for (int i = 0; i < streamConfigSize; i = i + 4) {
        ALOGD("DEBUG(%s[%d]):ID %d Size %4dx%4d Format %2x %6s",
                __FUNCTION__, __LINE__,
                cameraId,
                streamConfigArray[i+1], streamConfigArray[i+2],
                streamConfigArray[i],
                (streamConfigArray[i+3] == ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT)?
                "OUTPUT" : "INPUT");
    }
#endif

    return ret;
}

status_t ExynosCamera3MetadataConverter::m_createScalerAvailableMinFrameDurations(const struct ExynosSensorInfoBase *sensorStaticInfo,
                                                                                 Vector<int64_t> *minDurations,
                                                                                 int cameraId)
{
    status_t ret = NO_ERROR;
    int (*yuvSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int yuvSizeListLength = 0;
    int (*jpegSizeList)[SIZE_OF_RESOLUTION] = NULL;
    int jpegSizeListLength = 0;
    int minDurationSize = 0;
    int64_t currentMinDuration = 0L;
    bool isSupportHighResolution = false;

    if (sensorStaticInfo == NULL) {
        ALOGE("ERR(%s[%d]):Sensor static info is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (minDurations == NULL) {
        ALOGE("ERR(%s[%d]):Stream configs is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    isSupportHighResolution = m_hasTagInList(
            sensorStaticInfo->capabilities,
            sensorStaticInfo->capabilitiesLength,
            ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE);

    if (cameraId == CAMERA_ID_BACK) {
        yuvSizeList = sensorStaticInfo->rearPreviewList;
        yuvSizeListLength = sensorStaticInfo->rearPreviewListMax;
        jpegSizeList = sensorStaticInfo->rearPictureList;
        jpegSizeListLength = sensorStaticInfo->rearPictureListMax;
    } else { /* CAMERA_ID_FRONT */
        yuvSizeList = sensorStaticInfo->frontPreviewList;
        yuvSizeListLength = sensorStaticInfo->frontPreviewListMax;
        jpegSizeList = sensorStaticInfo->frontPictureList;
        jpegSizeListLength = sensorStaticInfo->frontPictureListMax;
    }

    /* HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED stream min frame duration list size */
    minDurationSize = yuvSizeListLength * 4;
    /* YUV output stream min frame duration list size */
    minDurationSize += (yuvSizeListLength * 4) * (ARRAY_LENGTH(YUV_FORMATS));
    /* Stall output stream configuration list size */
    minDurationSize += (jpegSizeListLength * 4) * (ARRAY_LENGTH(STALL_FORMATS));
    /* RAW output stream min frame duration list size */
    minDurationSize += (1 * 4) * (ARRAY_LENGTH(RAW_FORMATS));
    minDurations->setCapacity(minDurationSize);

    /* HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED stream min frame duration list */
    for (int i = 0; i < yuvSizeListLength; i++) {
        minDurations->add(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
        minDurations->add((int64_t)yuvSizeList[i][0]);
        minDurations->add((int64_t)yuvSizeList[i][1]);
        minDurations->add((int64_t)YUV_FORMAT_MIN_DURATION);
    }

    /* YUV output stream min frame duration list */
    for (size_t i = 0; i < ARRAY_LENGTH(YUV_FORMATS); i++) {
        for (int j = 0; j < yuvSizeListLength; j++) {
            int pixelSize = yuvSizeList[j][0] * yuvSizeList[j][1];
            if (isSupportHighResolution == false
                && pixelSize > HIGH_RESOLUTION_MIN_PIXEL_SIZE) {
                minDurationSize -= 4;
                continue;
            }

            minDurations->add((int64_t)YUV_FORMATS[i]);
            minDurations->add((int64_t)yuvSizeList[j][0]);
            minDurations->add((int64_t)yuvSizeList[j][1]);
            minDurations->add((int64_t)YUV_FORMAT_MIN_DURATION);
        }
    }

    /* Stall output stream min frame duration list */
    for (size_t i = 0; i < ARRAY_LENGTH(STALL_FORMATS); i++) {
        for (int j = 0; j < jpegSizeListLength; j++) {
            int pixelSize = jpegSizeList[j][0] * jpegSizeList[j][1];
            if (isSupportHighResolution == false
                && pixelSize > HIGH_RESOLUTION_MIN_PIXEL_SIZE) {
                minDurationSize -= 4;
                continue;
            }

            minDurations->add((int64_t)STALL_FORMATS[i]);
            minDurations->add((int64_t)jpegSizeList[j][0]);
            minDurations->add((int64_t)jpegSizeList[j][1]);

            if (pixelSize > HIGH_RESOLUTION_MIN_PIXEL_SIZE)
                currentMinDuration = HIGH_RESOLUTION_MIN_DURATION;
            else if (pixelSize > FHD_PIXEL_SIZE)
                currentMinDuration = STALL_FORMAT_MIN_DURATION;
            else
                currentMinDuration = YUV_FORMAT_MIN_DURATION;
            minDurations->add((int64_t)currentMinDuration);
        }
    }

    /* RAW output stream min frame duration list */
    for (size_t i = 0; i < ARRAY_LENGTH(RAW_FORMATS); i++) {
        /* Add sensor max size */
        minDurations->add((int64_t)RAW_FORMATS[i]);
        minDurations->add((int64_t)sensorStaticInfo->maxSensorW);
        minDurations->add((int64_t)sensorStaticInfo->maxSensorH);
        minDurations->add((int64_t)YUV_FORMAT_MIN_DURATION);
    }

    minDurations->setCapacity(minDurationSize);

#ifdef DEBUG_STREAM_CONFIGURATIONS
    const int64_t* minDurationArray = NULL;
    minDurationArray = minDurations->array();
    for (int i = 0; i < minDurationSize; i = i + 4) {
        ALOGD("DEBUG(%s[%d]):ID %d Size %4lldx%4lld Format %2x MinDuration %9lld",
                __FUNCTION__, __LINE__,
                cameraId,
                minDurationArray[i+1], minDurationArray[i+2],
                (int)minDurationArray[i], minDurationArray[i+3]);
    }
#endif

    return ret;
}

status_t ExynosCamera3MetadataConverter::m_createJpegAvailableThumbnailSizes(const struct ExynosSensorInfoBase *sensorStaticInfo,
                                                                             Vector<int32_t> *thumbnailSizes)
{
    int ret = OK;
    int (*thumbnailSizeList)[3] = NULL;
    size_t thumbnailSizeListLength = 0;

    if (sensorStaticInfo == NULL) {
        ALOGE("ERR(%s[%d]):Sensor static info is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (thumbnailSizes == NULL) {
        ALOGE("ERR(%s[%d]):Thumbnail sizes is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    thumbnailSizeList = sensorStaticInfo->thumbnailList;
    thumbnailSizeListLength = sensorStaticInfo->thumbnailListMax;
    thumbnailSizes->setCapacity(thumbnailSizeListLength * 2);

    /* JPEG thumbnail sizes must be delivered with ascending ordering */
    for (int i = (int)thumbnailSizeListLength - 1; i >= 0; i--) {
        thumbnailSizes->add(thumbnailSizeList[i][0]);
        thumbnailSizes->add(thumbnailSizeList[i][1]);
    }

    return ret;
}

status_t ExynosCamera3MetadataConverter::m_createAeAvailableFpsRanges(const struct ExynosSensorInfoBase *sensorStaticInfo,
                                                                      Vector<int32_t> *fpsRanges,
                                                                      int cameraId)
{
    int ret = OK;
    int (*fpsRangesList)[2] = NULL;
    size_t fpsRangesLength = 0;

    if (sensorStaticInfo == NULL) {
        ALOGE("ERR(%s[%d]):Sensor static info is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
    if (fpsRanges == NULL) {
        ALOGE("ERR(%s[%d]):FPS ranges is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (cameraId == CAMERA_ID_BACK) {
        fpsRangesList = sensorStaticInfo->rearFPSList;
        fpsRangesLength = sensorStaticInfo->rearFPSListMax;
    } else { /* CAMERA_ID_FRONT */
        fpsRangesList = sensorStaticInfo->frontFPSList;
        fpsRangesLength = sensorStaticInfo->frontFPSListMax;
    }

    fpsRanges->setCapacity(fpsRangesLength * 2);

    for (size_t i = 0; i < fpsRangesLength; i++) {
        fpsRanges->add(fpsRangesList[i][0]/1000);
        fpsRanges->add(fpsRangesList[i][1]/1000);
    }

    return ret;
}

bool ExynosCamera3MetadataConverter::m_hasTagInList(int32_t *list, size_t listSize, int32_t tag)
{
    bool hasTag = false;

    for (size_t i = 0; i < listSize; i++) {
        if (list[i] == tag) {
            hasTag = true;
            break;
        }
    }

    return hasTag;
}

bool ExynosCamera3MetadataConverter::m_hasTagInList(uint8_t *list, size_t listSize, int32_t tag)
{
    bool hasTag = false;

    for (size_t i = 0; i < listSize; i++) {
        if (list[i] == tag) {
            hasTag = true;
            break;
        }
    }

    return hasTag;
}

status_t ExynosCamera3MetadataConverter::m_integrateOrderedSizeList(int (*list1)[SIZE_OF_RESOLUTION], size_t list1Size,
                                                                    int (*list2)[SIZE_OF_RESOLUTION], size_t list2Size,
                                                                    int (*orderedList)[SIZE_OF_RESOLUTION])
{
    int *currentSize = NULL;
    size_t sizeList1Index = 0;
    size_t sizeList2Index = 0;

    if (list1 == NULL || list2 == NULL || orderedList == NULL) {
        ALOGE("ERR(%s[%d]):Arguments are NULL. list1 %p list2 %p orderedlist %p",
                __FUNCTION__, __LINE__,
                list1, list2, orderedList);
        return BAD_VALUE;
    }

    /* This loop will integrate two size list in descending order */
    for (size_t i = 0; i < list1Size + list2Size; i++) {
        if (sizeList1Index >= list1Size) {
            currentSize = list2[sizeList2Index++];
        } else if (sizeList2Index >= list2Size) {
            currentSize = list1[sizeList1Index++];
        } else {
            if (list1[sizeList1Index][0] < list2[sizeList2Index][0]) {
                currentSize = list2[sizeList2Index++];
            } else if (list1[sizeList1Index][0] > list2[sizeList2Index][0]) {
                currentSize = list1[sizeList1Index++];
            } else {
                if (list1[sizeList1Index][1] < list2[sizeList2Index][1])
                    currentSize = list2[sizeList2Index++];
                else
                    currentSize = list1[sizeList1Index++];
            }
        }
        orderedList[i][0] = currentSize[0];
        orderedList[i][1] = currentSize[1];
        orderedList[i][2] = currentSize[2];
    }

    return NO_ERROR;
}

void ExynosCamera3MetadataConverter::m_updateFaceDetectionMetaData(CameraMetadata *settings, struct camera2_shot_ext *shot_ext)
{
    int32_t faceIds[NUM_OF_DETECTED_FACES];
    /* {leftEyeX, leftEyeY, rightEyeX, rightEyeY, mouthX, mouthY} */
    int32_t faceLandmarks[NUM_OF_DETECTED_FACES * FACE_LANDMARKS_MAX_INDEX];
    /* {xmin, ymin, xmax, ymax} with the absolute coordinate */
    int32_t faceRectangles[NUM_OF_DETECTED_FACES * RECTANGLE_MAX_INDEX];
    uint8_t faceScores[NUM_OF_DETECTED_FACES];
    uint8_t detectedFaceCount = 0;
    int maxSensorW = 0, maxSensorH = 0;
    ExynosRect bnsSize, bayerCropSize;
    int xOffset = 0, yOffset = 0;
    int hwPreviewW = 0, hwPreviewH = 0;
    float scaleRatioW = 0.0f, scaleRatioH = 0.0f;

    if (settings == NULL) {
        ALOGE("ERR(%s[%d]:CameraMetadata is NULL", __FUNCTION__, __LINE__);
        return;
    }

    if (shot_ext == NULL) {
        ALOGE("ERR(%s[%d]:camera2_shot_ext is NULL", __FUNCTION__, __LINE__);
        return;
    }

    /* Original FD region : based on FD input size (e.g. preview size)
     * Camera Metadata FD region : based on sensor active array size (e.g. max sensor size)
     * The FD region from firmware must be scaled into the size based on sensor active array size
     */
    m_parameters->getMaxSensorSize(&maxSensorW, &maxSensorH);
    m_parameters->getPreviewBayerCropSize(&bnsSize, &bayerCropSize);
    if ((maxSensorW - bayerCropSize.w) / 2 > 0)
        xOffset = ALIGN_DOWN(((maxSensorW - bayerCropSize.w) / 2), 2);
    if ((maxSensorH - bayerCropSize.h) / 2 > 0)
        yOffset = ALIGN_DOWN(((maxSensorH - bayerCropSize.h) / 2), 2);
    if (m_parameters->isMcscVraOtf() == true)
        m_parameters->getYuvSize(&hwPreviewW, &hwPreviewH, 0);
    else
        m_parameters->getHwVraInputSize(&hwPreviewW, &hwPreviewH);
    scaleRatioW = (float)bayerCropSize.w / (float)hwPreviewW;
    scaleRatioH = (float)bayerCropSize.h / (float)hwPreviewH;

    for (int i = 0; i < NUM_OF_DETECTED_FACES; i++) {
        if (shot_ext->shot.dm.stats.faceIds[i]) {
            switch (shot_ext->shot.dm.stats.faceDetectMode) {
                case FACEDETECT_MODE_FULL:
                    faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + LEFT_EYE_X] = -1;
                    faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + LEFT_EYE_Y] = -1;
                    faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + RIGHT_EYE_X] = -1;
                    faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + RIGHT_EYE_Y] = -1;
                    faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + MOUTH_X] = -1;
                    faceLandmarks[(i * FACE_LANDMARKS_MAX_INDEX) + MOUTH_Y] = -1;
                case FACEDETECT_MODE_SIMPLE:
                    faceIds[i] = shot_ext->shot.dm.stats.faceIds[i];
                    /* Normalize the score into the range of [0, 100] */
                    faceScores[i] = (uint8_t) ((float)shot_ext->shot.dm.stats.faceScores[i] / (255.0f / 100.0f));

                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + X1] = (int32_t) ((shot_ext->shot.dm.stats.faceRectangles[i][X1] * scaleRatioW) + xOffset);
                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y1] = (int32_t) ((shot_ext->shot.dm.stats.faceRectangles[i][Y1] * scaleRatioH) + yOffset);
                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + X2] = (int32_t) ((shot_ext->shot.dm.stats.faceRectangles[i][X2] * scaleRatioW) + xOffset);
                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y2] = (int32_t) ((shot_ext->shot.dm.stats.faceRectangles[i][Y2] * scaleRatioH) + yOffset);
                    ALOGV("DEBUG(%s):faceIds[%d](%d), faceScores[%d](%d), original(%d,%d,%d,%d), converted(%d,%d,%d,%d)",
                        __FUNCTION__,
                        i, faceIds[i], i, faceScores[i],
                        shot_ext->shot.dm.stats.faceRectangles[i][X1],
                        shot_ext->shot.dm.stats.faceRectangles[i][Y1],
                        shot_ext->shot.dm.stats.faceRectangles[i][X2],
                        shot_ext->shot.dm.stats.faceRectangles[i][Y2],
                        faceRectangles[(i * RECTANGLE_MAX_INDEX) + X1],
                        faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y1],
                        faceRectangles[(i * RECTANGLE_MAX_INDEX) + X2],
                        faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y2]);

                    detectedFaceCount++;
                    break;
                case FACEDETECT_MODE_OFF:
                    faceScores[i] = 0;
                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + X1] = 0;
                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y1] = 0;
                    faceRectangles[(i * RECTANGLE_MAX_INDEX) + X2] = 0;
                    faceRectangles[(i * RECTANGLE_MAX_INDEX)+ Y2] = 0;
                    break;
                default:
                    ALOGE("ERR(%s[%d]):Not supported FD mode(%d)", __FUNCTION__, __LINE__,
                            shot_ext->shot.dm.stats.faceDetectMode);
                    break;
            }
        } else {
            faceIds[i] = 0;
            faceScores[i] = 0;
            faceRectangles[(i * RECTANGLE_MAX_INDEX) + X1] = 0;
            faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y1] = 0;
            faceRectangles[(i * RECTANGLE_MAX_INDEX) + X2] = 0;
            faceRectangles[(i * RECTANGLE_MAX_INDEX) + Y2] = 0;
        }
    }

    if (detectedFaceCount > 0) {
        switch (shot_ext->shot.dm.stats.faceDetectMode) {
            case FACEDETECT_MODE_FULL:
                settings->update(ANDROID_STATISTICS_FACE_LANDMARKS, faceLandmarks,
                        detectedFaceCount * FACE_LANDMARKS_MAX_INDEX);
                ALOGV("DEBUG(%s):dm.stats.faceLandmarks(%d)", __FUNCTION__, detectedFaceCount);
            case FACEDETECT_MODE_SIMPLE:
                settings->update(ANDROID_STATISTICS_FACE_IDS, faceIds, detectedFaceCount);
                ALOGV("DEBUG(%s):dm.stats.faceIds(%d)", __FUNCTION__, detectedFaceCount);

                settings->update(ANDROID_STATISTICS_FACE_RECTANGLES, faceRectangles,
                        detectedFaceCount * RECTANGLE_MAX_INDEX);
                ALOGV("DEBUG(%s):dm.stats.faceRectangles(%d)", __FUNCTION__, detectedFaceCount);

                settings->update(ANDROID_STATISTICS_FACE_SCORES, faceScores, detectedFaceCount);
                ALOGV("DEBUG(%s):dm.stats.faceScores(%d)", __FUNCTION__, detectedFaceCount);
                break;
            case FACEDETECT_MODE_OFF:
            default:
                ALOGE("ERR(%s[%d]):Not supported FD mode(%d)", __FUNCTION__, __LINE__,
                        shot_ext->shot.dm.stats.faceDetectMode);
                break;
        }
    }

    return;
}

void ExynosCamera3MetadataConverter::m_convert3AARegion(ExynosRect2 *region)
{
    ExynosRect2 newRect2;
    ExynosRect maxSensorSize;
    ExynosRect hwBcropSize;

    m_parameters->getMaxSensorSize(&maxSensorSize.w, &maxSensorSize.h);
    m_parameters->getHwBayerCropRegion(&hwBcropSize.w, &hwBcropSize.h,
            &hwBcropSize.x, &hwBcropSize.y);

    newRect2 = convertingSrcArea2DstArea(region, &maxSensorSize, &hwBcropSize);

    region->x1 = newRect2.x1;
    region->y1 = newRect2.y1;
    region->x2 = newRect2.x2;
    region->y2 = newRect2.y2;
}

status_t ExynosCamera3MetadataConverter::checkMetaValid(camera_metadata_tag_t tag, const void *data)
{
    status_t ret = NO_ERROR;
    camera_metadata_entry_t entry;

    int32_t value = 0;
    const int32_t *i32Range = NULL;

    if (m_staticInfo.exists(tag) == false) {
        ALOGE("ERR(%s[%d]):Cannot find entry, tag(%d)", __FUNCTION__, __LINE__, tag);
        return BAD_VALUE;
    }

    entry = m_staticInfo.find(tag);

    /* TODO: handle all tags
     *       need type check
     */
    switch (tag) {
    case ANDROID_SENSOR_INFO_SENSITIVITY_RANGE:
         value = *(int32_t *)data;
         i32Range = entry.data.i32;
        if (value < i32Range[0] || value > i32Range[1]) {
            ALOGE("ERR(%s[%d]):Invalid Sensitivity value(%d), range(%d, %d)",
                __FUNCTION__, __LINE__, value, i32Range[0], i32Range[1]);
            ret = BAD_VALUE;
        }
        break;
    default:
        ALOGE("ERR(%s[%d]):Tag (%d) is not implemented", __FUNCTION__, __LINE__, tag);
        break;
    }

    return ret;
}

status_t ExynosCamera3MetadataConverter::getDefaultSetting(camera_metadata_tag_t tag, void *data)
{
    status_t ret = NO_ERROR;
    camera_metadata_entry_t entry;

    const int32_t *i32Range = NULL;

    if (m_defaultRequestSetting.exists(tag) == false) {
        ALOGE("ERR(%s[%d]):Cannot find entry, tag(%d)", __FUNCTION__, __LINE__, tag);
        return BAD_VALUE;
    }

    entry = m_defaultRequestSetting.find(tag);

    /* TODO: handle all tags
     *       need type check
     */
    switch (tag) {
    case ANDROID_SENSOR_SENSITIVITY:
         i32Range = entry.data.i32;
         *(int32_t *)data = i32Range[0];
        break;
    default:
        ALOGE("ERR(%s[%d]):Tag (%d) is not implemented", __FUNCTION__, __LINE__, tag);
        break;
    }

    return ret;
}

}; /* namespace android */
