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

#ifndef EXYNOS_CAMERA_SENSOR_INFO_BASE_H
#define EXYNOS_CAMERA_SENSOR_INFO_BASE_H

#include <videodev2.h>
#include <videodev2_exynos_camera.h>
#include <camera/CameraMetadata.h>
#include "ExynosCameraConfig.h"
#include "ExynosCameraSizeTable.h"
#include "fimc-is-metadata.h"

/*TODO: This values will be changed */
#define BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR       "0.10,1.20,Infinity"
#define FRONT_CAMERA_FOCUS_DISTANCES_STR           "0.20,0.25,Infinity"

#define BACK_CAMERA_MACRO_FOCUS_DISTANCES_STR      "0.10,0.20,Infinity"
#define BACK_CAMERA_INFINITY_FOCUS_DISTANCES_STR   "0.10,1.20,Infinity"

#define BACK_CAMERA_FOCUS_DISTANCE_INFINITY        "Infinity"
#define FRONT_CAMERA_FOCUS_DISTANCE_INFINITY       "Infinity"

#define UNIQUE_ID_BUF_SIZE          (32)

#if defined(SUPPORT_X8_ZOOM)
#define MAX_ZOOM_LEVEL ZOOM_LEVEL_X8_MAX
#define MAX_ZOOM_RATIO (8000)
#define MAX_ZOOM_LEVEL_FRONT ZOOM_LEVEL_MAX
#define MAX_ZOOM_RATIO_FRONT (4000)
#define MAX_BASIC_ZOOM_LEVEL ZOOM_LEVEL_X8_MAX  /* CTS and 3rd-Party */
#elif defined(SUPPORT_X8_ZOOM_AND_800STEP)
#define MAX_ZOOM_LEVEL ZOOM_LEVEL_X8_800STEP_MAX
#define MAX_ZOOM_RATIO (8000)
#define MAX_ZOOM_LEVEL_FRONT ZOOM_LEVEL_MAX
#define MAX_ZOOM_RATIO_FRONT (4000)
#define MAX_BASIC_ZOOM_LEVEL ZOOM_LEVEL_X8_MAX  /* CTS and 3rd-Party */
#elif defined(SUPPORT_X4_ZOOM_AND_400STEP)
#define MAX_ZOOM_LEVEL ZOOM_LEVEL_X4_400STEP_MAX
#define MAX_ZOOM_RATIO (4000)
#define MAX_ZOOM_LEVEL_FRONT ZOOM_LEVEL_MAX
#define MAX_ZOOM_RATIO_FRONT (4000)
#define MAX_BASIC_ZOOM_LEVEL ZOOM_LEVEL_MAX  /* CTS and 3rd-Party */
#else
#define MAX_ZOOM_LEVEL ZOOM_LEVEL_MAX
#define MAX_ZOOM_RATIO (4000)
#define MAX_ZOOM_LEVEL_FRONT ZOOM_LEVEL_MAX
#define MAX_ZOOM_RATIO_FRONT (4000)
#define MAX_BASIC_ZOOM_LEVEL ZOOM_LEVEL_MAX  /* CTS and 3rd-Party */
#endif

#define ARRAY_LENGTH(x)          (sizeof(x)/sizeof(x[0]))
#define COMMON_DENOMINATOR       (100)
#define EFFECTMODE_META_2_HAL(x) (1 << (x -1))

#define SENSOR_ID_EXIF_SIZE         27
#define SENSOR_ID_EXIF_TAG         "ssuniqueid"

namespace android {

enum max_3a_region {
    AE,
    AWB,
    AF,
    REGIONS_INDEX_MAX,
};
enum size_direction {
    WIDTH,
    HEIGHT,
    SIZE_DIRECTION_MAX,
};
enum coordinate_3d {
    X_3D,
    Y_3D,
    Z_3D,
    COORDINATE_3D_MAX,
};
enum output_streams_type {
    RAW,
    PROCESSED,
    PROCESSED_STALL,
    OUTPUT_STREAM_TYPE_MAX,
};
enum range_type {
    MIN,
    MAX,
    RANGE_TYPE_MAX,
};
enum bayer_cfa_mosaic_channel {
    R,
    GR,
    GB,
    B,
    BAYER_CFA_MOSAIC_CHANNEL_MAX,
};
enum hue_sat_value_index {
    HUE,
    SATURATION,
    VALUE,
    HUE_SAT_VALUE_INDEX_MAX,
};
enum sensor_margin_base_index {
    LEFT_BASE,
    TOP_BASE,
    WIDTH_BASE,
    HEIGHT_BASE,
    BASE_MAX,
};

#ifdef SENSOR_NAME_GET_FROM_FILE
int getSensorIdFromFile(int camId);
#endif
#ifdef SENSOR_FW_GET_FROM_FILE
const char *getSensorFWFromFile(struct ExynosSensorInfoBase *info, int camId);
#endif

struct sensor_id_exif_data {
    char sensor_id_exif[SENSOR_ID_EXIF_SIZE];
};

struct exynos_camera_info {
public:
    int     previewW;
    int     previewH;
    int     previewFormat;
    int     previewStride;

    int     pictureW;
    int     pictureH;
    int     pictureFormat;
    int     hwPictureFormat;

    int     videoW;
    int     videoH;
    int     hwVideoW;
    int     hwVideoH;
    int     maxVideoW;
    int     maxVideoH;

    int     callbackW;
    int     callbackH;
    int     callbackFormat;

    int     yuvWidth[3];
    int     yuvHeight[3];
    int     yuvFormat[3];

    /* This size for internal */
    int     hwSensorW;
    int     hwSensorH;
    int     hwPreviewW;
    int     hwPreviewH;
    int     previewSizeRatioId;
    int     hwPictureW;
    int     hwPictureH;
    int     pictureSizeRatioId;
    int     hwDisW;
    int     hwDisH;
    int     videoSizeRatioId;
    int     hwPreviewFormat;

    int     hwBayerCropW;
    int     hwBayerCropH;
    int     hwBayerCropX;
    int     hwBayerCropY;

    int     bnsW;
    int     bnsH;

    int     jpegQuality;
    int     thumbnailW;
    int     thumbnailH;
    int     thumbnailQuality;

    int     intelligentMode;
    bool    visionMode;
    int     visionModeFps;
    int     visionModeAeTarget;

    bool    recordingHint;
    bool    dualMode;
    bool    dualRecordingHint;
#ifdef BOARD_CAMERA_USES_DUAL_CAMERA
    bool    dualCameraMode; // stereo camera
#endif

    bool    effectHint;
    bool    effectRecordingHint;
    bool    highSpeedRecording;
    bool    videoStabilization;
    bool    swVdisMode;
    bool    swVdisUIMode;
    bool    highResolutionCallbackMode;
    bool    is3dnrMode;
    bool    isDrcMode;
    bool    isOdcMode;

    int     zoom;
    int     rotation;
    int     flipHorizontal;
    int     flipVertical;
    bool    autoExposureLock;

    int     meteringMode;
    bool    isTouchMetering;

    int     sceneMode;
    int     focusMode;
    int     flashMode;
    int     whiteBalanceMode;
    bool    autoWhiteBalanceLock;
    int     numValidFocusArea;

    double  gpsLatitude;
    double  gpsLongitude;
    double  gpsAltitude;
    long    gpsTimeStamp;

    long long int cityId;
    unsigned char weatherId;

    bool    hdrMode;
    bool    wdrMode;
    int     shotMode;
    bool    antiShake;
    int     vtMode;
    int     vrMode;
    int     plbMode;
    bool    gamma;
    bool    slowAe;
    int     seriesShotCount;

    bool    scalableSensorMode;
    char    imageUniqueId[UNIQUE_ID_BUF_SIZE];
    bool    samsungCamera;

    int     autoFocusMacroPosition;
    int     deviceOrientation;
    uint32_t    bnsScaleRatio;
    uint32_t    binningScaleRatio;

    int     seriesShotMode;

};

struct ExynosSensorInfoBase {
public:
#ifdef SENSOR_FW_GET_FROM_FILE
    char	sensor_fw[25];
#endif
    struct sensor_id_exif_data sensor_id_exif_info;

    int     maxPreviewW;
    int     maxPreviewH;
    int     maxPictureW;
    int     maxPictureH;
    int     maxVideoW;
    int     maxVideoH;
    int     maxSensorW;
    int     maxSensorH;
    int     sensorMarginW;
    int     sensorMarginH;
    int     sensorMarginBase[BASE_MAX];

    int     maxThumbnailW;
    int     maxThumbnailH;

    int     fNumberNum;
    int     fNumberDen;
    int     focalLengthNum;
    int     focalLengthDen;

    float   horizontalViewAngle[SIZE_RATIO_END];
    float   verticalViewAngle;

    /* TODO : Remove unused variables */
    int     focusDistanceNum;
    int     focusDistanceDen;
    int     apertureNum;
    int     apertureDen;
    int     minFps;
    int     maxFps;

    int     minExposureCompensation;
    int     maxExposureCompensation;
    int     minExposureTime;
    int     maxExposureTime;
    int     minWBK;
    int     maxWBK;
    uint32_t     maxNumFocusAreas;
    uint32_t     maxNumMeteringAreas;

    bool    videoSnapshotSupport;
    bool    videoStabilizationSupport;
    bool    autoWhiteBalanceLockSupport;
    bool    autoExposureLockSupport;
    bool    visionModeSupport;
    bool    drcSupport;

    /*
    ** Camera HAL 3.2 Static Metadatas
    **
    ** The order of declaration follows the order of
    ** Android Camera HAL3.2 Properties.
    ** Please refer the "/system/media/camera/docs/docs.html"
    */
    /* Android ColorCorrection Static Metadata */
    uint8_t    *colorAberrationModes;
    size_t     colorAberrationModesLength;

    /* Android Control Static Metadata */
    uint8_t    *antiBandingModes;
    uint8_t    *aeModes;
    int32_t    exposureCompensationRange[RANGE_TYPE_MAX];
    float      exposureCompensationStep;
    uint8_t    *afModes;
    uint8_t    *effectModes;
    uint8_t    *sceneModes;
    uint8_t    *videoStabilizationModes;
    uint8_t    *awbModes;
    int32_t    max3aRegions[REGIONS_INDEX_MAX];
    uint8_t    *controlModes;
    size_t     controlModesLength;
    uint8_t    *sceneModeOverrides;
    uint8_t    aeLockAvailable;
    uint8_t    awbLockAvailable;
    size_t     antiBandingModesLength;
    size_t     aeModesLength;
    size_t     afModesLength;
    size_t     effectModesLength;
    size_t     sceneModesLength;
    size_t     videoStabilizationModesLength;
    size_t     awbModesLength;
    size_t     sceneModeOverridesLength;

    /* Android Edge Static Metadata */
    uint8_t    *edgeModes;
    size_t     edgeModesLength;

    /* Android Flash Static Metadata */
    uint8_t    flashAvailable;
    int64_t    chargeDuration;
    uint8_t    colorTemperature;
    uint8_t    maxEnergy;

    /* Android Hot Pixel Static Metadata */
    uint8_t   *hotPixelModes;
    size_t    hotPixelModesLength;

    /* Android Lens Static Metadata */
    float     aperture;
    float     fNumber;
    float     filterDensity;
    float     focalLength;
    int       focalLengthIn35mmLength;
    uint8_t   *opticalStabilization;
    float     hyperFocalDistance;
    float     minimumFocusDistance;
    int32_t   shadingMapSize[SIZE_DIRECTION_MAX];
    uint8_t   focusDistanceCalibration;
    uint8_t   lensFacing;
    float     opticalAxisAngle[2];
    float     lensPosition[COORDINATE_3D_MAX];
    size_t    opticalStabilizationLength;

    /* Android Noise Reduction Static Metadata */
    uint8_t   *noiseReductionModes;
    size_t    noiseReductionModesLength;

    /* Android Request Static Metadata */
    int32_t   maxNumOutputStreams[OUTPUT_STREAM_TYPE_MAX];
    int32_t   maxNumInputStreams;
    uint8_t   maxPipelineDepth;
    int32_t   partialResultCount;
    uint8_t   *capabilities;
    int32_t   *requestKeys;
    int32_t   *resultKeys;
    int32_t   *characteristicsKeys;
    size_t    capabilitiesLength;
    size_t    requestKeysLength;
    size_t    resultKeysLength;
    size_t    characteristicsKeysLength;

    /* Android Scaler Static Metadata */
    bool      zoomSupport;
    bool      smoothZoomSupport;
    int       maxZoomLevel;
    int       maxZoomRatio;
    int       zoomRatioList[MAX_ZOOM_LEVEL];
    int       maxBasicZoomLevel;
    int64_t   *stallDurations;
    uint8_t   croppingType;
    size_t    stallDurationsLength;

    /* Android Sensor Static Metadata */
    int32_t   sensitivityRange[RANGE_TYPE_MAX];
    uint8_t   colorFilterArrangement;
    int64_t   exposureTimeRange[RANGE_TYPE_MAX];
    int64_t   maxFrameDuration;
    float     sensorPhysicalSize[SIZE_DIRECTION_MAX];
    int32_t   whiteLevel;
    uint8_t   timestampSource;
    uint8_t   referenceIlluminant1;
    uint8_t   referenceIlluminant2;
    int32_t   blackLevelPattern[BAYER_CFA_MOSAIC_CHANNEL_MAX];
    int32_t   maxAnalogSensitivity;
    int32_t   orientation;
    int32_t   profileHueSatMapDimensions[HUE_SAT_VALUE_INDEX_MAX];
    int32_t   *testPatternModes;
    size_t    testPatternModesLength;
    camera_metadata_rational *colorTransformMatrix1;
    camera_metadata_rational *colorTransformMatrix2;
    camera_metadata_rational *forwardMatrix1;
    camera_metadata_rational *forwardMatrix2;
    camera_metadata_rational *calibration1;
    camera_metadata_rational *calibration2;

    /* Android Statistics Static Metadata */
    uint8_t   *faceDetectModes;
    int32_t   histogramBucketCount;
    int32_t   maxNumDetectedFaces;
    int32_t   maxHistogramCount;
    int32_t   maxSharpnessMapValue;
    int32_t   sharpnessMapSize[SIZE_DIRECTION_MAX];
    uint8_t   *hotPixelMapModes;
    uint8_t   *lensShadingMapModes;
    size_t    lensShadingMapModesLength;
    uint8_t   *shadingAvailableModes;
    size_t    shadingAvailableModesLength;
    size_t    faceDetectModesLength;
    size_t    hotPixelMapModesLength;

    /* Android Tone Map Static Metadata */
    int32_t   tonemapCurvePoints;
    uint8_t   *toneMapModes;
    size_t    toneMapModesLength;

    /* Android LED Static Metadata */
    uint8_t   *leds;
    size_t    ledsLength;

    /* Android Info Static Metadata */
    uint8_t   supportedHwLevel;

    /* Android Sync Static Metadata */
    int32_t   maxLatency;
    /* END of Camera HAL 3.2 Static Metadatas */

    /* vendor specifics */
    int    highResolutionCallbackW;
    int    highResolutionCallbackH;
    int    highSpeedRecording60WFHD;
    int    highSpeedRecording60HFHD;
    int    highSpeedRecording60W;
    int    highSpeedRecording60H;
    int    highSpeedRecording120W;
    int    highSpeedRecording120H;
    bool   scalableSensorSupport;
    bool   bnsSupport;
    bool   flite3aaOtfSupport;

    /* The number of preview(picture) sizes in each list */
    int    rearPreviewListMax;
    int    frontPreviewListMax;
    int    rearPictureListMax;
    int    frontPictureListMax;
    int    hiddenRearPreviewListMax;
    int    hiddenFrontPreviewListMax;
    int    hiddenRearPictureListMax;
    int    hiddenFrontPictureListMax;
    int    thumbnailListMax;
    int    rearVideoListMax;
    int    frontVideoListMax;
    int    hiddenRearVideoListMax;
    int    hiddenFrontVideoListMax;
    int    highSpeedVideoListMax;
    int    rearFPSListMax;
    int    frontFPSListMax;
    int    hiddenRearFPSListMax;
    int    hiddenFrontFPSListMax;
    int    highSpeedVideoFPSListMax;

    /* Supported Preview/Picture/Video Lists */
    int    (*rearPreviewList)[SIZE_OF_RESOLUTION];
    int    (*frontPreviewList)[SIZE_OF_RESOLUTION];
    int    (*rearPictureList)[SIZE_OF_RESOLUTION];
    int    (*frontPictureList)[SIZE_OF_RESOLUTION];
    int    (*hiddenRearPreviewList)[SIZE_OF_RESOLUTION];
    int    (*hiddenFrontPreviewList)[SIZE_OF_RESOLUTION];
    int    (*hiddenRearPictureList)[SIZE_OF_RESOLUTION];
    int    (*hiddenFrontPictureList)[SIZE_OF_RESOLUTION];
    int    (*highSpeedVideoList)[SIZE_OF_RESOLUTION];
    int    (*thumbnailList)[SIZE_OF_RESOLUTION];
    int    (*rearVideoList)[SIZE_OF_RESOLUTION];
    int    (*frontVideoList)[SIZE_OF_RESOLUTION];
    int    (*hiddenRearVideoList)[SIZE_OF_RESOLUTION];
    int    (*hiddenFrontVideoList)[SIZE_OF_RESOLUTION];
    int    (*rearFPSList)[2];
    int    (*frontFPSList)[2];
    int    (*hiddenRearFPSList)[2];
    int    (*hiddenFrontFPSList)[2];
    int    (*highSpeedVideoFPSList)[2];

    int    antiBandingList;
    int    effectList;
    int    hiddenEffectList;
    int    flashModeList;
    int    focusModeList;
    int    sceneModeList;
    int    whiteBalanceList;
    int    isoValues;
    int    meteringList;

    int    previewSizeLutMax;
    int    pictureSizeLutMax;
    int    videoSizeLutMax;
    int    vtcallSizeLutMax;
	int    videoSizeLutHighSpeedMax;
    int    videoSizeLutHighSpeed60Max;
    int    videoSizeLutHighSpeed120Max;
    int    videoSizeLutHighSpeed240Max;
    int    liveBroadcastSizeLutMax;
    int    depthMapSizeLutMax;
    int    fastAeStableLutMax;

    int    (*previewSizeLut)[SIZE_OF_LUT];
    int    (*pictureSizeLut)[SIZE_OF_LUT];
    int    (*videoSizeLut)[SIZE_OF_LUT];
    int    (*videoSizeBnsLut)[SIZE_OF_LUT];
    int    (*dualPreviewSizeLut)[SIZE_OF_LUT];
    int    (*dualVideoSizeLut)[SIZE_OF_LUT];
	int    (*videoSizeLutHighSpeed)[SIZE_OF_LUT];
    int    (*videoSizeLutHighSpeed60)[SIZE_OF_LUT];
    int    (*videoSizeLutHighSpeed120)[SIZE_OF_LUT];
    int    (*videoSizeLutHighSpeed240)[SIZE_OF_LUT];
    int    (*vtcallSizeLut)[SIZE_OF_LUT];
    int    (*liveBroadcastSizeLut)[SIZE_OF_LUT];
    int    (*depthMapSizeLut)[SIZE_OF_RESOLUTION];
    int    (*fastAeStableLut)[SIZE_OF_LUT];
    bool   sizeTableSupport;

#ifdef BOARD_CAMERA_USES_DUAL_CAMERA
    void   *dof;
#endif

public:
    ExynosSensorInfoBase();
};

struct ExynosSensorIMX135Base : public ExynosSensorInfoBase {
public:
    ExynosSensorIMX135Base();
};

struct ExynosSensorIMX134Base : public ExynosSensorInfoBase {
public:
    ExynosSensorIMX134Base();
};

struct ExynosSensorS5K3L2Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K3L2Base();
};

struct ExynosSensorS5K3L8Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K3L8Base();
};

struct ExynosSensorS5K3L8DualBdsBase : public ExynosSensorS5K3L8Base {
public:
    ExynosSensorS5K3L8DualBdsBase();
};

#if 0
struct ExynosSensorS5K2P2Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K2P2Base();
};
#endif

struct ExynosSensorS5K3P3Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K3P3Base();
};

struct ExynosSensorS5K2P2_12MBase : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K2P2_12MBase();
};

struct ExynosSensorS5K2P3Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K2P3Base();
};

struct ExynosSensorS5K2P8Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K2P8Base();
};

struct ExynosSensorS5K2T2Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K2T2Base();
};

struct ExynosSensorS5K6B2Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K6B2Base();
};

struct ExynosSensorSR261Base : public ExynosSensorInfoBase {
public:
    ExynosSensorSR261Base();
};

struct ExynosSensorSR259Base : public ExynosSensorInfoBase {
public:
    ExynosSensorSR259Base();
};

struct ExynosSensorS5K3H7Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K3H7Base();
};

struct ExynosSensorS5K3H5Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K3H5Base();
};

struct ExynosSensorS5K4H5Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K4H5Base();
};

struct ExynosSensorS5K4H5YCBase : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K4H5YCBase();
};

struct ExynosSensorS5K3M2Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K3M2Base();
};

struct ExynosSensorS5K3M3Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K3M3Base();
};

struct ExynosSensorS5K3M3DualBdsBase : public ExynosSensorS5K3M3Base {
public:
    ExynosSensorS5K3M3DualBdsBase();
};

struct ExynosSensorS5K5E2Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K5E2Base();
};

struct ExynosSensorS5K5E8Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K5E8Base();
};
struct ExynosSensorS5K6A3Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K6A3Base();
};

struct ExynosSensorIMX175Base : public ExynosSensorInfoBase {
public:
    ExynosSensorIMX175Base();
};

#if 0
struct ExynosSensorIMX240Base: public ExynosSensorInfoBase {
public:
    ExynosSensorIMX240Base();
};
#endif

struct ExynosSensorIMX228Base : public ExynosSensorInfoBase {
public:
    ExynosSensorIMX228Base();
};

struct ExynosSensorIMX219Base : public ExynosSensorInfoBase {
public:
    ExynosSensorIMX219Base();
};

struct ExynosSensorS5K8B1Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K8B1Base();
};

struct ExynosSensorS5K6D1Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K6D1Base();
};

struct ExynosSensorS5K4E6Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K4E6Base();
};

struct ExynosSensorS5K5E3Base : public ExynosSensorInfoBase {
public:
    ExynosSensorS5K5E3Base();
};

struct ExynosSensorSR544Base : public ExynosSensorInfoBase {
public:
    ExynosSensorSR544Base();
};

struct ExynosSensorIMX240_2P2Base : public ExynosSensorInfoBase {
public:
    ExynosSensorIMX240_2P2Base();
};

struct ExynosSensorIMX260_2L1Base : public ExynosSensorInfoBase {
public:
    ExynosSensorIMX260_2L1Base();
};

struct ExynosSensorOV5670Base : public ExynosSensorInfoBase {
public:
    ExynosSensorOV5670Base();
};

/* Helpper functions */
int getSensorId(int camId);
void getDualCameraId(int *cameraId_0, int *cameraId_1);

enum CAMERA_ID {
    CAMERA_ID_BACK    = 0,
    CAMERA_ID_FRONT   = 1,
    CAMERA_ID_BACK_0  = CAMERA_ID_BACK,
    CAMERA_ID_FRONT_0 = CAMERA_ID_FRONT,
    CAMERA_ID_BACK_1  = 2,
    CAMERA_ID_FRONT_1 = 3,
    CAMERA_ID_MAX,
};

enum MODE {
    MODE_PREVIEW = 0,
    MODE_PICTURE,
    MODE_VIDEO,
    MODE_THUMBNAIL,
};

enum {
    ANTIBANDING_AUTO = (1 << 0),
    ANTIBANDING_50HZ = (1 << 1),
    ANTIBANDING_60HZ = (1 << 2),
    ANTIBANDING_OFF  = (1 << 3),
};

enum {
    SCENE_MODE_AUTO           = (1 << 0),
    SCENE_MODE_ACTION         = (1 << 1),
    SCENE_MODE_PORTRAIT       = (1 << 2),
    SCENE_MODE_LANDSCAPE      = (1 << 3),
    SCENE_MODE_NIGHT          = (1 << 4),
    SCENE_MODE_NIGHT_PORTRAIT = (1 << 5),
    SCENE_MODE_THEATRE        = (1 << 6),
    SCENE_MODE_BEACH          = (1 << 7),
    SCENE_MODE_SNOW           = (1 << 8),
    SCENE_MODE_SUNSET         = (1 << 9),
    SCENE_MODE_STEADYPHOTO    = (1 << 10),
    SCENE_MODE_FIREWORKS      = (1 << 11),
    SCENE_MODE_SPORTS         = (1 << 12),
    SCENE_MODE_PARTY          = (1 << 13),
    SCENE_MODE_CANDLELIGHT    = (1 << 14),
    SCENE_MODE_AQUA           = (1 << 17),
};

enum {
    FOCUS_MODE_AUTO               = (1 << 0),
    FOCUS_MODE_INFINITY           = (1 << 1),
    FOCUS_MODE_MACRO              = (1 << 2),
    FOCUS_MODE_FIXED              = (1 << 3),
    FOCUS_MODE_EDOF               = (1 << 4),
    FOCUS_MODE_CONTINUOUS_VIDEO   = (1 << 5),
    FOCUS_MODE_CONTINUOUS_PICTURE = (1 << 6),
    FOCUS_MODE_TOUCH              = (1 << 7),
    FOCUS_MODE_CONTINUOUS_PICTURE_MACRO = (1 << 8),
};

enum {
    WHITE_BALANCE_AUTO             = (1 << 0),
    WHITE_BALANCE_INCANDESCENT     = (1 << 1),
    WHITE_BALANCE_FLUORESCENT      = (1 << 2),
    WHITE_BALANCE_WARM_FLUORESCENT = (1 << 3),
    WHITE_BALANCE_DAYLIGHT         = (1 << 4),
    WHITE_BALANCE_CLOUDY_DAYLIGHT  = (1 << 5),
    WHITE_BALANCE_TWILIGHT         = (1 << 6),
    WHITE_BALANCE_SHADE            = (1 << 7),
    WHITE_BALANCE_CUSTOM_K         = (1 << 8),
};

enum {
    FLASH_MODE_OFF     = (1 << 0),
    FLASH_MODE_AUTO    = (1 << 1),
    FLASH_MODE_ON      = (1 << 2),
    FLASH_MODE_RED_EYE = (1 << 3),
    FLASH_MODE_TORCH   = (1 << 4),
};

/* Metering */
enum {
    METERING_MODE_AVERAGE       = (1 << 0),
    METERING_MODE_CENTER        = (1 << 1),
    METERING_MODE_MATRIX        = (1 << 2),
    METERING_MODE_SPOT          = (1 << 3),
    METERING_MODE_CENTER_TOUCH  = (1 << 4),
    METERING_MODE_MATRIX_TOUCH  = (1 << 5),
    METERING_MODE_SPOT_TOUCH	= (1 << 6),
    METERING_MODE_AVERAGE_TOUCH = (1 << 7),
};

/* Contrast */
enum {
    CONTRAST_AUTO    = (1 << 0),
    CONTRAST_MINUS_2 = (1 << 1),
    CONTRAST_MINUS_1 = (1 << 2),
    CONTRAST_DEFAULT = (1 << 3),
    CONTRAST_PLUS_1  = (1 << 4),
    CONTRAST_PLUS_2  = (1 << 5),
};

/* Shot mode */
enum SHOT_MODE {
    SHOT_MODE_NORMAL         = 0x00,
    SHOT_MODE_AUTO           = 0x01,
    SHOT_MODE_BEAUTY_FACE    = 0x02,
    SHOT_MODE_BEST_PHOTO     = 0x03,
    SHOT_MODE_DRAMA          = 0x04,
    SHOT_MODE_BEST_FACE      = 0x05,
    SHOT_MODE_ERASER         = 0x06,
    SHOT_MODE_PANORAMA       = 0x07,
    SHOT_MODE_3D_PANORAMA    = 0x08,
    SHOT_MODE_RICH_TONE      = 0x09,
    SHOT_MODE_NIGHT          = 0x0A,
    SHOT_MODE_STORY          = 0x0B,
    SHOT_MODE_AUTO_PORTRAIT  = 0x0C,
    SHOT_MODE_PET            = 0x0D,
    SHOT_MODE_GOLF           = 0x0E,
    SHOT_MODE_ANIMATED_SCENE = 0x0F,
    SHOT_MODE_NIGHT_SCENE    = 0x10,
    SHOT_MODE_SPORTS         = 0x11,
    SHOT_MODE_AQUA           = 0x12,
    SHOT_MODE_MAGIC          = 0x13,
    SHOT_MODE_OUTFOCUS	     = 0x14,
    SHOT_MODE_3DTOUR         = 0x15,
    SHOT_MODE_SEQUENCE       = 0x16,
    SHOT_MODE_LIGHT_TRACE    = 0x17,
#ifdef USE_LIMITATION_FOR_THIRD_PARTY
    THIRD_PARTY_BLACKBOX_MODE   = 0x19,
    THIRD_PARTY_VTCALL_MODE = 0x20,
    THIRD_PARTY_HANGOUT_MODE = 0x21,
#endif
    SHOT_MODE_FRONT_PANORAMA = 0x1B,
    SHOT_MODE_SELFIE_ALARM = 0x1C,
    SHOT_MODE_INTERACTIVE = 0x1D,
    SHOT_MODE_DUAL = 0x1E,
    SHOT_MODE_FASTMOTION = 0x1F,
    SHOT_MODE_PRO_MODE          = 0x22,
    SHOT_MODE_VIDEO_COLLAGE     = 0x24,
    SHOT_MODE_ANTI_FOG          = 0x25,
    SHOT_MODE_MAX,
};

enum SERIES_SHOT_MODE {
    SERIES_SHOT_MODE_NONE              = 0,
    SERIES_SHOT_MODE_LLS               = 1,
    SERIES_SHOT_MODE_SIS               = 2,
    SERIES_SHOT_MODE_BURST             = 3,
    SERIES_SHOT_MODE_ERASER            = 4,
    SERIES_SHOT_MODE_BEST_FACE         = 5,
    SERIES_SHOT_MODE_BEST_PHOTO        = 6,
    SERIES_SHOT_MODE_MAGIC             = 7,
    SERIES_SHOT_MODE_SELFIE_ALARM      = 8,
    SERIES_SHOT_MODE_MAX,
};

enum ISO_VALUES {
    ISO_AUTO = (1 << 0),
    ISO_100 = (1 << 1),
    ISO_200 = (1 << 2),
    ISO_400 = (1 << 3),
    ISO_800 = (1 << 4),
};

enum {
#ifdef EFFECT_VALUE_VERSION_2_0
    EFFECT_NONE       = (1 << 0),
    EFFECT_MONO       = (1 << 1),
    EFFECT_NEGATIVE   = (1 << 2),
    EFFECT_SOLARIZE   = (1 << 3),
    EFFECT_SEPIA      = (1 << 4),
    EFFECT_POSTERIZE  = (1 << 5),
    EFFECT_WHITEBOARD = (1 << 6),
    EFFECT_BLACKBOARD = (1 << 7),
    EFFECT_AQUA       = (1 << 8),
    EFFECT_RED_YELLOW = (1 << 9),
    EFFECT_BLUE       = (1 << 10),
    EFFECT_WARM_VINTAGE = (1 << 11),
    EFFECT_COLD_VINTAGE = (1 << 12),
    EFFECT_BEAUTY_FACE = (1 << 13),
#else
    EFFECT_NONE       = (1 << COLORCORRECTION_MODE_FAST),
    EFFECT_MONO       = (1 << COLORCORRECTION_MODE_EFFECT_MONO),
    EFFECT_NEGATIVE   = (1 << COLORCORRECTION_MODE_EFFECT_NEGATIVE),
    EFFECT_SOLARIZE   = (1 << COLORCORRECTION_MODE_EFFECT_SOLARIZE),
    EFFECT_SEPIA	  = (1 << COLORCORRECTION_MODE_EFFECT_SEPIA),
    EFFECT_POSTERIZE  = (1 << COLORCORRECTION_MODE_EFFECT_POSTERIZE),
    EFFECT_WHITEBOARD = (1 << COLORCORRECTION_MODE_EFFECT_WHITEBOARD),
    EFFECT_BLACKBOARD = (1 << COLORCORRECTION_MODE_EFFECT_BLACKBOARD),
    EFFECT_AQUA       = (1 << COLORCORRECTION_MODE_EFFECT_AQUA),
    EFFECT_RED_YELLOW = (1 << COLORCORRECTION_MODE_EFFECT_RED_YELLOW_POINT),
    EFFECT_BLUE       = (1 << COLORCORRECTION_MODE_EFFECT_BLUE_POINT),
    EFFECT_WARM_VINTAGE = (1 << COLORCORRECTION_MODE_EFFECT_WARM_VINTAGE),
    EFFECT_COLD_VINTAGE = (1 << COLORCORRECTION_MODE_EFFECT_COLD_VINTAGE),
    EFFECT_BEAUTY_FACE = (1 << COLORCORRECTION_MODE_EFFECT_BEAUTY_FACE),
#endif
};

}; /* namespace android */
#endif
