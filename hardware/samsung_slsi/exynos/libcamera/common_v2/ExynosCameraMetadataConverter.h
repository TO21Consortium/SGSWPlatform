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

#include "ExynosCameraConfig.h"

#include "ExynosCameraParameters.h"

#include "ExynosCamera3SensorInfo.h"

#include "fimc-is-metadata.h"

#define FIMC_IS_METADATA(x) (x + 1)
#define CAMERA_METADATA(x)  ((x < 1)? 0 : x - 1)

#define PREVIEW_FORMAT_MIN_DURATION         (33331760L)
#define PICTURE_FORMAT_MIN_DURATION         (100000000L)
#define ACTUAL_PIPELINE_DEPTH               (4)

namespace android {

class ExynosCameraRequestManager;
class ExynosCameraRequest;

enum map_index {
    CAMERA_META,
    FIMC_IS_META,
    MAX_INDEX
};
const int32_t PREVIEW_FORMATS[] =
{
    HAL_PIXEL_FORMAT_RGBA_8888,
    HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED,
    HAL_PIXEL_FORMAT_YCbCr_420_888
};

const int32_t PICTURE_FORMATS[] =
{
    /* HAL_PIXEL_FORMAT_RAW_SENSOR, */
    HAL_PIXEL_FORMAT_RAW16,
    HAL_PIXEL_FORMAT_BLOB,
};

enum rectangle_index {
    X1,
    Y1,
    X2,
    Y2,
    RECTANGLE_MAX_INDEX,
};

enum face_landmarks_index {
    LEFT_EYE_X,
    LEFT_EYE_Y,
    RIGHT_EYE_X,
    RIGHT_EYE_Y,
    MOUTH_X,
    MOUTH_Y,
    FACE_LANDMARKS_MAX_INDEX,
};

class ExynosCameraMetadataConverter : public virtual RefBase {
public:
    ExynosCameraMetadataConverter(){};
    ~ExynosCameraMetadataConverter(){};

    //virtual status_t        constructStaticInfo(int cameraId, camera_metadata_t **info) = 0;
    virtual status_t        constructDefaultRequestSettings(int type, camera_metadata_t **request) = 0;
    virtual status_t        initShotData(struct camera2_shot_ext *shot_ext) = 0;
    virtual status_t        updateDynamicMeta(ExynosCameraRequest *requestInfo) = 0;
    virtual status_t        convertRequestToShot(CameraMetadata &request, struct camera2_shot_ext *dst_ext, int *reqId = NULL) = 0;
    virtual status_t        checkAvailableStreamFormat(int format) = 0;
    virtual void            setStaticInfo(int camId, camera_metadata_t *info) = 0;
};

class ExynosCamera3MetadataConverter : public virtual ExynosCameraMetadataConverter {
public:
    ExynosCamera3MetadataConverter(int cameraId, ExynosCameraParameters *parameters);
    ~ExynosCamera3MetadataConverter();
    static  status_t        constructStaticInfo(int cameraId, camera_metadata_t **info);
    virtual status_t        constructDefaultRequestSettings(int type, camera_metadata_t **request);

    /* helper functions for android control and meta data */
    virtual status_t        convertRequestToShot(CameraMetadata &request, struct camera2_shot_ext *dst_ext, int *reqId = NULL);
    virtual status_t        updateDynamicMeta(ExynosCameraRequest *requestInfo);


    /* meta -> shot */
    virtual status_t        translateColorControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateControlControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateDemosaicControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateEdgeControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateFlashControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateHotPixelControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateJpegControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateLensControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateNoiseControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateRequestControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext, int *reqId);
    virtual status_t        translateScalerControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateSensorControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateShadingControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateStatisticsControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateTonemapControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateLedControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);
    virtual status_t        translateBlackLevelControlData(CameraMetadata &settings, struct camera2_shot_ext *dst_ext);

    /* shot -> meta */
    virtual status_t        translateColorMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateControlMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateEdgeMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateFlashMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateHotPixelMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateJpegMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateLensMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateNoiseMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateQuirksMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateRequestMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateScalerMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateSensorMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateShadingMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateStatisticsMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateTonemapMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateLedMetaData(ExynosCameraRequest *requestInfo);
    virtual status_t        translateBlackLevelMetaData(ExynosCameraRequest *requestInfo);

    /* Other helper functions */
    virtual status_t        initShotData(struct camera2_shot_ext *shot_ext);
    virtual status_t        checkAvailableStreamFormat(int format);

    virtual void            setStaticInfo(int camId, camera_metadata_t *info);
    virtual status_t        checkMetaValid(camera_metadata_tag_t tag, const void *data);
    virtual status_t        getDefaultSetting(camera_metadata_tag_t tag, void *data);
private:
    static status_t         m_createControlAvailableHighSpeedVideoConfigurations(const struct ExynosSensorInfoBase *sensorStaticInfo,
                                                                                 Vector<int32_t> *streamConfigs,
                                                                                 int cameraId);
    static status_t         m_createScalerAvailableInputOutputFormatsMap(const struct ExynosSensorInfoBase *sensorStaticInfo,
                                                                         Vector<int32_t> *streamConfigs,
                                                                         int cameraId);
    static status_t         m_createScalerAvailableStreamConfigurationsOutput(const struct ExynosSensorInfoBase *sensorStaticInfo,
                                                                              Vector<int32_t> *streamConfigs,
                                                                              int cameraId);
    static status_t         m_createScalerAvailableMinFrameDurations(const struct ExynosSensorInfoBase *sensorStaticInfo,
                                                                     Vector<int64_t> *minDurations,
                                                                     int cameraId);
    static status_t         m_createJpegAvailableThumbnailSizes(const struct ExynosSensorInfoBase *sensorStaticInfo,
                                                                Vector<int32_t> *thumbnailSizes);
    static status_t         m_createAeAvailableFpsRanges(const struct ExynosSensorInfoBase *sensorStaticInfo,
                                                         Vector<int32_t> *fpsRanges,
                                                         int cameraId);
    static bool             m_hasTagInList(int32_t *list, size_t listSize, int32_t tag);
    static bool             m_hasTagInList(uint8_t *list, size_t listSize, int32_t tag);
    static status_t         m_integrateOrderedSizeList(int (*list1)[SIZE_OF_RESOLUTION], size_t list1Size,
                                                       int (*list2)[SIZE_OF_RESOLUTION], size_t list2Size,
                                                       int (*orderedList)[SIZE_OF_RESOLUTION]);

    void                    m_updateFaceDetectionMetaData(CameraMetadata *settings,
                                                          struct camera2_shot_ext *shot_ext);
    void                    m_convert3AARegion(ExynosRect2 *region);

private:
    mutable Mutex                   m_requestLock;
    int                             m_cameraId;
    ExynosCameraParameters          *m_parameters;
    ExynosCameraActivityFlash       *m_flashMgr;

    CameraMetadata                  m_staticInfo;
    CameraMetadata                  m_defaultRequestSetting;
    struct ExynosSensorInfoBase     *m_sensorStaticInfo;

    bool                            m_preCaptureTriggerOn;
    bool                            m_isManualAeControl;

    /* HACK : Temporary save the Mode info for adjusting value for CTS Test */
    ExynosRect                      m_cropRegion;
    bool                            m_blackLevelLockOn;
    bool                            m_faceDetectModeOn;
    uint32_t                        m_lockVendorIsoValue;
    uint64_t                        m_lockExposureTime;
    uint64_t                        m_preExposureTime;
    uint32_t                        m_afMode;
    uint32_t                        m_preAfMode;
    float                           m_focusDistance;
    uint32_t                        m_maxFps;
    bool                            m_overrideFlashControl;
    uint8_t                         m_gpsProcessingMethod[32];
};

}; /* namespace android */
#endif
