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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCamera1Parameters"
#include <cutils/log.h>

#include "ExynosCamera1Parameters.h"

namespace android {

void ExynosCamera1Parameters::vendorSpecificConstructor(__unused int cameraId)
{
    mDebugInfo.debugSize[APP_MARKER_4] = sizeof(struct camera2_udm);

    m_zoom_activated = false;
}

status_t ExynosCamera1Parameters::setParameters(const CameraParameters& params)
{
    status_t ret = NO_ERROR;

#ifdef TEST_GED_HIGH_SPEED_RECORDING
    int minFpsRange = 0, maxFpsRange = 0;
    int frameRate = 0;

    params.getPreviewFpsRange(&minFpsRange, &maxFpsRange);
    frameRate = params.getPreviewFrameRate();
    CLOGD("DEBUG(%s[%d]):getFastFpsMode=%d, maxFpsRange=%d, frameRate=%d",
        __FUNCTION__, __LINE__, getFastFpsMode(), maxFpsRange, frameRate);
    if (frameRate == 60) {
        setFastFpsMode(1);
    } else if (frameRate == 120) {
        setFastFpsMode(2);
    } else {
        setFastFpsMode(0);
    }

    CLOGD("DEBUG(%s[%d]):getFastFpsMode=%d", __FUNCTION__, __LINE__, getFastFpsMode());
#endif

    /* Return OK means that the vision mode is enabled */
    if (checkVisionMode(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkVisionMode fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (getVisionMode() == true) {
        CLOGD("DEBUG(%s[%d]): Vision mode enabled", __FUNCTION__, __LINE__);
        return NO_ERROR;
    }

    if (checkRecordingHint(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkRecordingHint fail", __FUNCTION__, __LINE__);

    if (checkDualMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkDualMode fail", __FUNCTION__, __LINE__);

    if (checkDualRecordingHint(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkDualRecordingHint fail", __FUNCTION__, __LINE__);

    if (checkEffectHint(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkEffectHint fail", __FUNCTION__, __LINE__);

    if (checkEffectRecordingHint(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkEffectRecordingHint fail", __FUNCTION__, __LINE__);

    if (checkPreviewFps(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkPreviewFps fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (getRecordingRunning() == false) {
        if (checkVideoSize(params) != NO_ERROR)
            CLOGE("ERR(%s[%d]): checkVideoSize fail", __FUNCTION__, __LINE__);
    }

    if (getCameraId() == CAMERA_ID_BACK) {
        if (checkFastFpsMode(params) != NO_ERROR)
            CLOGE("ERR(%s[%d]): checkFastFpsMode fail", __FUNCTION__, __LINE__);
    }

    if (checkVideoStabilization(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkVideoStabilization fail", __FUNCTION__, __LINE__);

    if (checkSWVdisMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSWVdisMode fail", __FUNCTION__, __LINE__);

    bool swVdisUIMode = false;
    m_setSWVdisUIMode(swVdisUIMode);

    if (checkVtMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkVtMode fail", __FUNCTION__, __LINE__);

    if (checkHWVdisMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkHWVdisMode fail", __FUNCTION__, __LINE__);

    if (check3dnrMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): check3dnrMode fail", __FUNCTION__, __LINE__);

    if (checkDrcMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkDrcMode fail", __FUNCTION__, __LINE__);

    if (checkOdcMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkOdcMode fail", __FUNCTION__, __LINE__);

    if (checkPreviewSize(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkPreviewSize fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkPreviewFormat(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkPreviewFormat fail", __FUNCTION__, __LINE__);

    if (checkPictureSize(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkPictureSize fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkPictureFormat(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkPictureFormat fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkJpegQuality(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkJpegQuality fail", __FUNCTION__, __LINE__);

    if (checkThumbnailSize(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkThumbnailSize fail", __FUNCTION__, __LINE__);

    if (checkThumbnailQuality(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkThumbnailQuality fail", __FUNCTION__, __LINE__);

    if (checkZoomLevel(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkZoomLevel fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkRotation(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkRotation fail", __FUNCTION__, __LINE__);

    if (checkAutoExposureLock(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkAutoExposureLock fail", __FUNCTION__, __LINE__);

    ret = checkExposureCompensation(params);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkExposureCompensation fail", __FUNCTION__, __LINE__);
        return ret;
    }

    if (checkMeteringMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkMeteringMode fail", __FUNCTION__, __LINE__);

    if (checkMeteringAreas(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkMeteringAreas fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkAntibanding(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkAntibanding fail", __FUNCTION__, __LINE__);

    if (checkSceneMode(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkSceneMode fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkFocusMode(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkFocusMode fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkFlashMode(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkFlashMode fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkWhiteBalanceMode(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkWhiteBalanceMode fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkAutoWhiteBalanceLock(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkAutoWhiteBalanceLock fail", __FUNCTION__, __LINE__);

/* W/B setting is available, but it is not used */
#if 0
    if (checkWbLevel(params) != NO_ERROR) {
        ALOGE("ERR(%s[%d]): checkWbLevel fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
#endif

    if (checkFocusAreas(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkFocusAreas fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkColorEffectMode(params) != NO_ERROR) {
        CLOGE("ERR(%s[%d]): checkColorEffectMode fail", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (checkGpsAltitude(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkGpsAltitude fail", __FUNCTION__, __LINE__);

    if (checkGpsLatitude(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkGpsLatitude fail", __FUNCTION__, __LINE__);

    if (checkGpsLongitude(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkGpsLongitude fail", __FUNCTION__, __LINE__);

    if (checkGpsProcessingMethod(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkGpsProcessingMethod fail", __FUNCTION__, __LINE__);

    if (checkGpsTimeStamp(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkGpsTimeStamp fail", __FUNCTION__, __LINE__);

#if 0
    if (checkCityId(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkCityId fail", __FUNCTION__, __LINE__);

    if (checkWeatherId(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkWeatherId fail", __FUNCTION__, __LINE__);
#endif

    if (checkBrightness(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkBrightness fail", __FUNCTION__, __LINE__);

    if (checkSaturation(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSaturation fail", __FUNCTION__, __LINE__);

    if (checkSharpness(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSharpness fail", __FUNCTION__, __LINE__);

    if (checkHue(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkHue fail", __FUNCTION__, __LINE__);

    if (checkIso(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkIso fail", __FUNCTION__, __LINE__);

    if (checkContrast(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkContrast fail", __FUNCTION__, __LINE__);

    if (checkHdrMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkHdrMode fail", __FUNCTION__, __LINE__);

    if (checkShotMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkShotMode fail", __FUNCTION__, __LINE__);

    if (checkAntiShake(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkAntiShake fail", __FUNCTION__, __LINE__);

    if (checkVtMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkVtMode fail", __FUNCTION__, __LINE__);

    if (checkGamma(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkGamma fail", __FUNCTION__, __LINE__);

    if (checkSlowAe(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSlowAe fail", __FUNCTION__, __LINE__);

    if (checkScalableSensorMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkScalableSensorMode fail", __FUNCTION__, __LINE__);

    if (checkImageUniqueId(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkImageUniqueId fail", __FUNCTION__, __LINE__);

    if (checkSeriesShotMode(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSeriesShotMode fail", __FUNCTION__, __LINE__);

#ifdef BURST_CAPTURE
    if (checkSeriesShotFilePath(params) != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSeriesShotFilePath fail", __FUNCTION__, __LINE__);
#endif

    if (m_getRestartPreviewChecked() == true) {
        CLOGD("DEBUG(%s[%d]):Need restart preview", __FUNCTION__, __LINE__);
        m_setRestartPreview(m_flagRestartPreviewChecked);
    }

    if (checkSetfileYuvRange() != NO_ERROR)
        CLOGE("ERR(%s[%d]): checkSetfileYuvRange fail", __FUNCTION__, __LINE__);

    checkHorizontalViewAngle();

    return ret;
}

void ExynosCamera1Parameters::setDefaultParameter(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    status_t ret = NO_ERROR;
    CameraParameters p;
    String8 tempStr;
    char strBuf[256];

    m_cameraInfo.autoFocusMacroPosition = ExynosCameraActivityAutofocus::AUTOFOCUS_MACRO_POSITION_BASE;

    /* Preview Size */
    getMaxPreviewSize(&m_cameraInfo.previewW, &m_cameraInfo.previewH);
    m_setHwPreviewSize(m_cameraInfo.previewW, m_cameraInfo.previewH);

    tempStr.setTo("");
    if (getResolutionList(tempStr, m_staticInfo, &m_cameraInfo.previewW, &m_cameraInfo.previewH, MODE_PREVIEW, m_cameraId) != NO_ERROR) {
        CLOGE("ERR(%s):getResolutionList(MODE_PREVIEW) fail", __FUNCTION__);

        m_cameraInfo.previewW = 640;
        m_cameraInfo.previewH = 480;
        tempStr = String8::format("%dx%d", m_cameraInfo.previewW, m_cameraInfo.previewH);
    }

    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES, tempStr.string());
    CLOGD("DEBUG(%s): Default preview size is %dx%d", __FUNCTION__, m_cameraInfo.previewW, m_cameraInfo.previewH);
    p.setPreviewSize(m_cameraInfo.previewW, m_cameraInfo.previewH);

    /* Preview Format */
    tempStr.setTo("");
    tempStr = String8::format("%s,%s", CameraParameters::PIXEL_FORMAT_YUV420SP, CameraParameters::PIXEL_FORMAT_YUV420P);
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS, tempStr);
    p.setPreviewFormat(CameraParameters::PIXEL_FORMAT_YUV420SP);


    /* Video Size */
    getMaxVideoSize(&m_cameraInfo.maxVideoW, &m_cameraInfo.maxVideoH);

    tempStr.setTo("");
    if (getResolutionList(tempStr, m_staticInfo, &m_cameraInfo.maxVideoW, &m_cameraInfo.maxVideoH, MODE_VIDEO, m_cameraId) != NO_ERROR) {
        CLOGE("ERR(%s):getResolutionList(MODE_VIDEO) fail", __FUNCTION__);

        m_cameraInfo.videoW = 640;
        m_cameraInfo.videoH = 480;
        tempStr = String8::format("%dx%d", m_cameraInfo.maxVideoW, m_cameraInfo.maxVideoH);
    }
#ifdef CAMERA_GED_FEATURE
    else {
#ifdef USE_WQHD_RECORDING
        if (m_addHiddenResolutionList(tempStr, m_staticInfo, 2560, 1440, MODE_VIDEO, m_cameraId) != NO_ERROR) {
            CLOGW("WARN(%s):getResolutionList(MODE_VIDEO) fail", __FUNCTION__);
        }
#endif
#ifdef USE_UHD_RECORDING
        if (m_addHiddenResolutionList(tempStr, m_staticInfo, 3840, 2160, MODE_VIDEO, m_cameraId) != NO_ERROR) {
            CLOGW("WARN(%s):getResolutionList(MODE_VIDEO) fail", __FUNCTION__);
        }
#endif
    }
#endif

    CLOGD("DEBUG(%s): KEY_SUPPORTED_VIDEO_SIZES %s", __FUNCTION__, tempStr.string());

    p.set(CameraParameters::KEY_SUPPORTED_VIDEO_SIZES, tempStr.string());

    CLOGD("DEBUG(%s): Max video size is %dx%d", __FUNCTION__, m_cameraInfo.maxVideoW, m_cameraInfo.maxVideoH);
    CLOGD("DEBUG(%s): Default video size is %dx%d", __FUNCTION__, m_cameraInfo.videoW, m_cameraInfo.videoH);
    p.setVideoSize(m_cameraInfo.videoW, m_cameraInfo.videoH);

    /* Video Format */
    if (getAdaptiveCSCRecording() == true) {
        CLOGI("INFO(%s[%d]):video_frame_foramt == YUV420SP_NV21", __FUNCTION__, __LINE__);
        p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP_NV21);
    } else {
        CLOGI("INFO(%s[%d]):video_frame_foramt == YUV420SP", __FUNCTION__, __LINE__);
        p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT, CameraParameters::PIXEL_FORMAT_YUV420SP);
    }

    /* Preferred preview size for Video */
    tempStr.setTo("");
    tempStr = String8::format("%dx%d", m_cameraInfo.previewW, m_cameraInfo.previewH);
    p.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO, tempStr.string());

    /* Picture Size */
    getMaxPictureSize(&m_cameraInfo.pictureW, &m_cameraInfo.pictureH);

    tempStr.setTo("");
    if (getResolutionList(tempStr, m_staticInfo, &m_cameraInfo.pictureW, &m_cameraInfo.pictureH, MODE_PICTURE, m_cameraId) != NO_ERROR) {
        CLOGE("ERR(%s):m_getResolutionList(MODE_PICTURE) fail", __FUNCTION__);

        m_cameraInfo.pictureW = 640;
        m_cameraInfo.pictureW = 480;
        tempStr = String8::format("%dx%d", m_cameraInfo.pictureW, m_cameraInfo.pictureH);
    }

    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES, tempStr.string());
    CLOGD("DEBUG(%s): Default picture size is %dx%d", __FUNCTION__, m_cameraInfo.pictureW, m_cameraInfo.pictureH);
    p.setPictureSize(m_cameraInfo.pictureW, m_cameraInfo.pictureH);

    /* Picture Format */
    p.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS, CameraParameters::PIXEL_FORMAT_JPEG);
    p.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);

    /* Jpeg Quality */
    p.set(CameraParameters::KEY_JPEG_QUALITY, "96"); /* maximum quality */

    /* Thumbnail Size */
    getMaxThumbnailSize(&m_cameraInfo.thumbnailW, &m_cameraInfo.thumbnailH);

    tempStr.setTo("");
    if (getResolutionList(tempStr, m_staticInfo, &m_cameraInfo.thumbnailW, &m_cameraInfo.thumbnailH, MODE_THUMBNAIL, m_cameraId) != NO_ERROR) {
        tempStr = String8::format("%dx%d", m_cameraInfo.thumbnailW, m_cameraInfo.thumbnailH);
    }
    /* 0x0 is no thumbnail mode */
    tempStr.append(",0x0");
    p.set(CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES, tempStr.string());
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH,  m_cameraInfo.thumbnailW);
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, m_cameraInfo.thumbnailH);

    /* Thumbnail Quality */
    p.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, "100");

    /* Exposure */
    p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, getMinExposureCompensation());
    p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, getMaxExposureCompensation());
    p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, 0);
    p.setFloat(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, getExposureCompensationStep());

    /* Auto Exposure Lock supported */
    if (getAutoExposureLockSupported() == true)
        p.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, "true");
    else
        p.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED, "false");

    /* Face Detection */
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW, getMaxNumDetectedFaces());
    p.set(CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW, 0);

    /* Video Sanptshot Supported */
    if (getVideoSnapshotSupported() == true)
        p.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, "true");
    else
        p.set(CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED, "false");

    /* Video Stabilization Supported */
    if (getVideoStabilizationSupported() == true)
        p.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED, "true");
    else
        p.set(CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED, "false");

    /* Focus Mode */
    int focusMode = getSupportedFocusModes();
    tempStr.setTo("");
    if (focusMode & FOCUS_MODE_AUTO) {
        tempStr.append(CameraParameters::FOCUS_MODE_AUTO);
    } else if (focusMode & FOCUS_MODE_FIXED){
        tempStr.append(CameraParameters::FOCUS_MODE_FIXED);
    }
    if (focusMode & FOCUS_MODE_INFINITY) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FOCUS_MODE_INFINITY);
    }
    if (focusMode & FOCUS_MODE_MACRO) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FOCUS_MODE_MACRO);
    }
    if (focusMode & FOCUS_MODE_EDOF) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FOCUS_MODE_EDOF);
    }
    if (focusMode & FOCUS_MODE_CONTINUOUS_VIDEO) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO);
    }
    if (focusMode & FOCUS_MODE_CONTINUOUS_PICTURE) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE);
    }
    if (focusMode & FOCUS_MODE_CONTINUOUS_PICTURE_MACRO) {
        tempStr.append(",");
        tempStr.append("continuous-picture-macro");
    }

    p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES,
          tempStr.string());

    if (focusMode & FOCUS_MODE_AUTO)
        p.set(CameraParameters::KEY_FOCUS_MODE,
              CameraParameters::FOCUS_MODE_AUTO);
    else if (focusMode & FOCUS_MODE_CONTINUOUS_PICTURE)
        p.set(CameraParameters::KEY_FOCUS_MODE,
              CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE);
    else if (focusMode & FOCUS_MODE_CONTINUOUS_VIDEO)
        p.set(CameraParameters::KEY_FOCUS_MODE,
              CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO);
    else if (focusMode & FOCUS_MODE_FIXED)
        p.set(CameraParameters::KEY_FOCUS_MODE,
          CameraParameters::FOCUS_MODE_FIXED);
    else
        p.set(CameraParameters::KEY_FOCUS_MODE,
          CameraParameters::FOCUS_MODE_INFINITY);

/*TODO: This values will be changed */
#define BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR       "0.10,1.20,Infinity"
#define FRONT_CAMERA_FOCUS_DISTANCES_STR           "0.20,0.25,Infinity"

#define BACK_CAMERA_MACRO_FOCUS_DISTANCES_STR      "0.10,0.20,Infinity"
#define BACK_CAMERA_INFINITY_FOCUS_DISTANCES_STR   "0.10,1.20,Infinity"

    /* Focus Distances */
    if (getCameraId() == CAMERA_ID_BACK)
        p.set(CameraParameters::KEY_FOCUS_DISTANCES,
              BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR);
    else
        p.set(CameraParameters::KEY_FOCUS_DISTANCES,
              FRONT_CAMERA_FOCUS_DISTANCES_STR);

    p.set(CameraParameters::FOCUS_DISTANCE_INFINITY, "Infinity");

    /* Max number of Focus Areas */
    p.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, 0);
    if (focusMode & FOCUS_MODE_TOUCH) {
        p.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, 1);
        p.set(CameraParameters::KEY_FOCUS_AREAS, "(0,0,0,0,0)");
    }

    /* Flash */
    int flashMode = getSupportedFlashModes();
    tempStr.setTo("");
    if (flashMode & FLASH_MODE_OFF) {
        tempStr.append(CameraParameters::FLASH_MODE_OFF);
    }
    if (flashMode & FLASH_MODE_AUTO) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FLASH_MODE_AUTO);
    }
    if (flashMode & FLASH_MODE_ON) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FLASH_MODE_ON);
    }
    if (flashMode & FLASH_MODE_RED_EYE) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FLASH_MODE_RED_EYE);
    }
    if (flashMode & FLASH_MODE_TORCH) {
        tempStr.append(",");
        tempStr.append(CameraParameters::FLASH_MODE_TORCH);
    }

    p.set(CameraParameters::KEY_SUPPORTED_FLASH_MODES, tempStr.string());
    p.set(CameraParameters::KEY_FLASH_MODE, CameraParameters::FLASH_MODE_OFF);

    /* scene mode */
    int sceneMode = getSupportedSceneModes();
    tempStr.setTo("");
    if (sceneMode & SCENE_MODE_AUTO) {
        tempStr.append(CameraParameters::SCENE_MODE_AUTO);
    }
    if (sceneMode & SCENE_MODE_ACTION) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_ACTION);
    }
    if (sceneMode & SCENE_MODE_PORTRAIT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_PORTRAIT);
    }
    if (sceneMode & SCENE_MODE_LANDSCAPE) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_LANDSCAPE);
    }
    if (sceneMode & SCENE_MODE_NIGHT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_NIGHT);
    }
    if (sceneMode & SCENE_MODE_NIGHT_PORTRAIT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_NIGHT_PORTRAIT);
    }
    if (sceneMode & SCENE_MODE_THEATRE) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_THEATRE);
    }
    if (sceneMode & SCENE_MODE_BEACH) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_BEACH);
    }
    if (sceneMode & SCENE_MODE_SNOW) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_SNOW);
    }
    if (sceneMode & SCENE_MODE_SUNSET) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_SUNSET);
    }
    if (sceneMode & SCENE_MODE_STEADYPHOTO) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_STEADYPHOTO);
    }
    if (sceneMode & SCENE_MODE_FIREWORKS) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_FIREWORKS);
    }
    if (sceneMode & SCENE_MODE_SPORTS) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_SPORTS);
    }
    if (sceneMode & SCENE_MODE_PARTY) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_PARTY);
    }
    if (sceneMode & SCENE_MODE_CANDLELIGHT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::SCENE_MODE_CANDLELIGHT);
    }

    p.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES,
          tempStr.string());
    p.set(CameraParameters::KEY_SCENE_MODE,
          CameraParameters::SCENE_MODE_AUTO);

    if (getHalVersion() != IS_HAL_VER_3_2) {
        /* effect */
        int effect = getSupportedColorEffects();
        tempStr.setTo("");
        if (effect & EFFECT_NONE) {
            tempStr.append(CameraParameters::EFFECT_NONE);
        }
        if (effect & EFFECT_MONO) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_MONO);
        }
        if (effect & EFFECT_NEGATIVE) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_NEGATIVE);
        }
        if (effect & EFFECT_SOLARIZE) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_SOLARIZE);
        }
        if (effect & EFFECT_SEPIA) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_SEPIA);
        }
        if (effect & EFFECT_POSTERIZE) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_POSTERIZE);
        }
        if (effect & EFFECT_WHITEBOARD) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_WHITEBOARD);
        }
        if (effect & EFFECT_BLACKBOARD) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_BLACKBOARD);
        }
        if (effect & EFFECT_AQUA) {
            tempStr.append(",");
            tempStr.append(CameraParameters::EFFECT_AQUA);
        }

        p.set(CameraParameters::KEY_SUPPORTED_EFFECTS, tempStr.string());
        p.set(CameraParameters::KEY_EFFECT, CameraParameters::EFFECT_NONE);
    }

    /* white balance */
    int whiteBalance = getSupportedWhiteBalance();
    tempStr.setTo("");
    if (whiteBalance & WHITE_BALANCE_AUTO) {
        tempStr.append(CameraParameters::WHITE_BALANCE_AUTO);
    }
    if (whiteBalance & WHITE_BALANCE_INCANDESCENT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_INCANDESCENT);
    }
    if (whiteBalance & WHITE_BALANCE_FLUORESCENT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_FLUORESCENT);
    }
    if (whiteBalance & WHITE_BALANCE_WARM_FLUORESCENT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT);
    }
    if (whiteBalance & WHITE_BALANCE_DAYLIGHT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_DAYLIGHT);
    }
    if (whiteBalance & WHITE_BALANCE_CLOUDY_DAYLIGHT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT);
    }
    if (whiteBalance & WHITE_BALANCE_TWILIGHT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_TWILIGHT);
    }
    if (whiteBalance & WHITE_BALANCE_SHADE) {
        tempStr.append(",");
        tempStr.append(CameraParameters::WHITE_BALANCE_SHADE);
    }

    p.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE,
          tempStr.string());
    p.set(CameraParameters::KEY_WHITE_BALANCE, CameraParameters::WHITE_BALANCE_AUTO);

    /* Auto Whitebalance Lock supported */
    if (getAutoWhiteBalanceLockSupported() == true)
        p.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, "true");
    else
        p.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED, "false");

    /*  anti banding */
    tempStr.setTo("");
    int antiBanding = getSupportedAntibanding();

    if (antiBanding & ANTIBANDING_AUTO) {
        tempStr.append(CameraParameters::ANTIBANDING_AUTO);
    }
    if (antiBanding & ANTIBANDING_50HZ) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ANTIBANDING_50HZ);
    }
    if (antiBanding & ANTIBANDING_60HZ) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ANTIBANDING_60HZ);
    }
    if (antiBanding & ANTIBANDING_OFF) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ANTIBANDING_OFF);
    }

    p.set(CameraParameters::KEY_SUPPORTED_ANTIBANDING,
          tempStr.string());

    p.set(CameraParameters::KEY_ANTIBANDING, CameraParameters::ANTIBANDING_AUTO);

    /* rotation */
    p.set(CameraParameters::KEY_ROTATION, 0);

    /* view angle */
    p.setFloat(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, getHorizontalViewAngle());
    p.setFloat(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, getVerticalViewAngle());

    /* metering */
    p.set(CameraParameters::KEY_MAX_NUM_METERING_AREAS, getMaxNumMeteringAreas());
    p.set(CameraParameters::KEY_METERING_AREAS, "");

    /* zoom */
    if (getZoomSupported() == true) {
        int maxZoom = getMaxZoomLevel();
        ALOGI("INFO(%s):getMaxZoomLevel(%d)", __FUNCTION__, maxZoom);

        if (0 < maxZoom) {
            p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "true");

            if (getSmoothZoomSupported() == true)
                p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "true");
            else
                p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");

            p.set(CameraParameters::KEY_MAX_ZOOM, maxZoom - 1);
            p.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);

            int max_zoom_ratio = (int)getMaxZoomRatio();
            tempStr.setTo("");
            if (getZoomRatioList(tempStr, maxZoom, max_zoom_ratio, m_staticInfo->zoomRatioList) == NO_ERROR)
                p.set(CameraParameters::KEY_ZOOM_RATIOS, tempStr.string());
            else
                p.set(CameraParameters::KEY_ZOOM_RATIOS, "100");

            p.set("constant-growth-rate-zoom-supported", "true");

            ALOGV("INFO(%s):zoomRatioList=%s", "setDefaultParameter", tempStr.string());
        } else {
            p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "false");
            p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
            p.set(CameraParameters::KEY_MAX_ZOOM, ZOOM_LEVEL_0);
            p.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);
        }
    } else {
        p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "false");
        p.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
        p.set(CameraParameters::KEY_MAX_ZOOM, ZOOM_LEVEL_0);
        p.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);
    }

    /* fps */
    uint32_t minFpsRange = 15;
    uint32_t maxFpsRange = 30;

    getPreviewFpsRange(&minFpsRange, &maxFpsRange);
#ifdef TEST_GED_HIGH_SPEED_RECORDING
    maxFpsRange = 120;
#endif
    CLOGI("INFO(%s[%d]):minFpsRange=%d, maxFpsRange=%d", "getPreviewFpsRange", __LINE__, (int)minFpsRange, (int)maxFpsRange);
    int minFps = (minFpsRange == 0) ? 0 : (int)minFpsRange;
    int maxFps = (maxFpsRange == 0) ? 0 : (int)maxFpsRange;

    tempStr.setTo("");
    snprintf(strBuf, 256, "%d", minFps);
    tempStr.append(strBuf);

    for (int i = minFps + 1; i <= maxFps; i++) {
        snprintf(strBuf, 256, ",%d", i);
        tempStr.append(strBuf);
    }
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES,  tempStr.string());

    minFpsRange = minFpsRange * 1000;
    maxFpsRange = maxFpsRange * 1000;

    tempStr.setTo("");
    getSupportedFpsList(tempStr, minFpsRange, maxFpsRange, m_cameraId, m_staticInfo);
    CLOGI("INFO(%s):supportedFpsList=%s", "setDefaultParameter", tempStr.string());
    p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, tempStr.string());
    /* p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE, "(15000,30000),(30000,30000)"); */

    /* limit 30 fps on default setting. */
    if (30 < maxFps)
        maxFps = 30;
    p.setPreviewFrameRate(maxFps);

    if (30000 < maxFpsRange)
        maxFpsRange = 30000;
    snprintf(strBuf, 256, "%d,%d", maxFpsRange/2, maxFpsRange);
    p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, strBuf);

    /* focal length */
    int num = 0;
    int den = 0;
    int precision = 0;
    getFocalLength(&num, &den);

    switch (den) {
    default:
    case 1000:
        precision = 3;
        break;
    case 100:
        precision = 2;
        break;
    case 10:
        precision = 1;
        break;
    case 1:
        precision = 0;
        break;
    }
    snprintf(strBuf, 256, "%.*f", precision, ((float)num / (float)den));
    p.set(CameraParameters::KEY_FOCAL_LENGTH, strBuf);

    /* Additional params. */
    p.set("contrast", "auto");
    p.set("iso", "auto");

    // Set supported ISO values
    int isoValues = getSupportedISO();
    tempStr.setTo("");
    if (isoValues & ISO_AUTO) {
        tempStr.append(CameraParameters::ISO_AUTO);
    }
    if (isoValues & ISO_100) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ISO_100);
    }
    if (isoValues & ISO_200) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ISO_200);
    }
    if (isoValues & ISO_400) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ISO_400);
    }
    if (isoValues & ISO_800) {
        tempStr.append(",");
        tempStr.append(CameraParameters::ISO_800);
    }

    p.set("iso-values",
          tempStr.string());

    p.set("wdr", 0);
    p.set("hdr-mode", 0);
    p.set("metering", "center");

    // Set Supported Metering Mode
    int meteringModes = getSupportedMeteringMode();
    tempStr.setTo("");
    if (meteringModes & METERING_MODE_MATRIX) {
        tempStr.append(CameraParameters::METERING_MATRIX);
    }
    if (meteringModes & METERING_MODE_CENTER) {
        tempStr.append(",");
        tempStr.append(CameraParameters::METERING_CENTER);
    }
    if (meteringModes & METERING_MODE_SPOT) {
        tempStr.append(",");
        tempStr.append(CameraParameters::METERING_SPOT);
    }

    p.set("brightness", 0);
    p.set("brightness-max", 2);
    p.set("brightness-min", -2);

    p.set("saturation", 0);
    p.set("saturation-max", 2);
    p.set("saturation-min", -2);

    p.set("sharpness", 0);
    p.set("sharpness-max", 2);
    p.set("sharpness-min", -2);

    p.set("hue", 0);
    p.set("hue-max", 2);
    p.set("hue-min", -2);

    /* For Series shot */
    p.set("burst-capture", 0);
    p.set("best-capture", 0);

    /* fnumber */
    getFnumber(&num, &den);
    p.set("fnumber-value-numerator", num);
    p.set("fnumber-value-denominator", den);

    /* max aperture value */
    getApertureValue(&num, &den);
    p.set("maxaperture-value-numerator", num);
    p.set("maxaperture-value-denominator", den);

    /* focal length */
    getFocalLength(&num, &den);
    p.set("focallength-value-numerator", num);
    p.set("focallength-value-denominator", den);

    /* focal length in 35mm film */
    int focalLengthIn35mmFilm = 0;
    focalLengthIn35mmFilm = getFocalLengthIn35mmFilm();
    p.set("focallength-35mm-value", focalLengthIn35mmFilm);

#if defined(TEST_APP_HIGH_SPEED_RECORDING)
    p.set("fast-fps-mode", 0);
#endif
    p.set(CameraParameters::KEY_DYNAMIC_RANGE_CONTROL, "off");

    p.set("imageuniqueid-value", 0);

    p.set("drc", "false");
    p.set("3dnr", "false");
    p.set("odc", "false");

    p.set("effectrecording-hint", 0);

    m_params = p;

    /* make sure m_secCamera has all the settings we do.  applications
     * aren't required to call setParameters themselves (only if they
     * want to change something.
     */
    ret = setParameters(p);
    if (ret < 0)
        CLOGE("ERR(%s[%d]):setParameters is fail", __FUNCTION__, __LINE__);
}

status_t ExynosCamera1Parameters::checkVisionMode(const CameraParameters& params)
{
    status_t ret;

    /* Check vision mode */
    int intelligent_mode = params.getInt("intelligent-mode");
    CLOGD("DEBUG(%s):intelligent_mode : %d", "setParameters", intelligent_mode);

    ret = m_setIntelligentMode(intelligent_mode);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s): Inavalid Intelligent mode", "setParameters");
        return ret;
    }
    m_params.set("intelligent-mode", intelligent_mode);

    CLOGD("DEBUG(%s):intelligent_mode(%d) getVisionMode(%d)", "setParameters", intelligent_mode, getVisionMode());

    /* Smart stay need to skip more frames */
    int skipCompensation = m_frameSkipCounter.getCompensation();
    if (intelligent_mode == 1) {
        m_frameSkipCounter.setCompensation(skipCompensation + SMART_STAY_SKIP_COMPENSATION);
    } else {
        m_frameSkipCounter.setCompensation(skipCompensation);
    }

    if (getVisionMode() == true) {
        /* preset for each vision mode */
        switch (intelligent_mode) {
        case 2:
            m_setVisionModeFps(10);
            break;
        case 3:
            m_setVisionModeFps(5);
            break;
        default:
            m_setVisionModeFps(10);
            break;
        }

/* Vision mode custom frame rate will be enabled when application ready */
#if 0
        /* If user wants to set custom fps, vision mode set max fps to frame rate */
        int minFps = -1;
        int maxFps = -1;
        params.getPreviewFpsRange(&minFps, &maxFps);

        if (minFps > 0 && maxFps > 0) {
            CLOGD("DEBUG(%s): set user frame rate (%d)", __FUNCTION__, maxFps / 1000);
            m_setVisionModeFps(maxFps / 1000);
        }
#endif

        /* smart-screen-exposure */
        int newVisionAeTarget = params.getInt("smart-screen-exposure");
        if (0 < newVisionAeTarget) {
            CLOGD("DEBUG(%s):newVisionAeTarget : %d", "setParameters", newVisionAeTarget);
            m_setVisionModeAeTarget(newVisionAeTarget);
            m_params.set("smart-screen-exposure", newVisionAeTarget);
        }

        return OK;
    } else {
        return NO_ERROR;
    }
}

status_t ExynosCamera1Parameters::m_setIntelligentMode(int intelligentMode)
{
    status_t ret = NO_ERROR;
    bool visionMode = false;

    m_cameraInfo.intelligentMode = intelligentMode;

    if (intelligentMode > 1) {
        if (m_staticInfo->visionModeSupport == true) {
            visionMode = true;
        } else {
            CLOGE("ERR(%s): tried to set vision mode(not supported)", "setParameters");
            ret = BAD_VALUE;
        }
    } else if (getVisionMode()) {
        CLOGE("ERR(%s[%d]):vision mode can not change before stoppreview", __FUNCTION__, __LINE__);
        visionMode = true;
    }

    m_setVisionMode(visionMode);

    return ret;
 }

int ExynosCamera1Parameters::getIntelligentMode(void)
{
    return m_cameraInfo.intelligentMode;
}

void ExynosCamera1Parameters::m_setVisionMode(bool vision)
{
    m_cameraInfo.visionMode = vision;
}

bool ExynosCamera1Parameters::getVisionMode(void)
{
    return m_cameraInfo.visionMode;
}

void ExynosCamera1Parameters::m_setVisionModeFps(int fps)
{
    m_cameraInfo.visionModeFps = fps;
}

int ExynosCamera1Parameters::getVisionModeFps(void)
{
    return m_cameraInfo.visionModeFps;
}

void ExynosCamera1Parameters::m_setVisionModeAeTarget(int ae)
{
    m_cameraInfo.visionModeAeTarget = ae;
}

int ExynosCamera1Parameters::getVisionModeAeTarget(void)
{
    return m_cameraInfo.visionModeAeTarget;
}

status_t ExynosCamera1Parameters::checkEffectRecordingHint(const CameraParameters& params)
{
    /* Effect recording hint */
    bool flagEffectRecordingHint = false;
    int newEffectRecordingHint = params.getInt("effectrecording-hint");
    int curEffectRecordingHint = m_params.getInt("effectrecording-hint");

    if (newEffectRecordingHint < 0) {
        CLOGV("DEBUG(%s):Invalid newEffectRecordingHint", "setParameters");
        return NO_ERROR;
    }

    if (newEffectRecordingHint != curEffectRecordingHint) {
        CLOGD("DEBUG(%s):newEffectRecordingHint : %d", "setParameters", newEffectRecordingHint);
        if (newEffectRecordingHint == 1)
            flagEffectRecordingHint = true;
        m_setEffectRecordingHint(flagEffectRecordingHint);
        m_params.set("effectrecording-hint", newEffectRecordingHint);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setEffectRecordingHint(bool hint)
{
    ExynosCameraActivityAutofocus *autoFocusMgr = m_activityControl->getAutoFocusMgr();
    ExynosCameraActivityFlash *flashMgr = m_activityControl->getFlashMgr();

    m_cameraInfo.effectRecordingHint = hint;

    if (hint) {
        setMetaVideoMode(&m_metadata, AA_VIDEOMODE_ON);
    } else if (!hint && !getRecordingHint() && !getDualRecordingHint()) {
        setMetaVideoMode(&m_metadata, AA_VIDEOMODE_OFF);
    }
    autoFocusMgr->setRecordingHint(hint);
    flashMgr->setRecordingHint(hint);
}


status_t ExynosCamera1Parameters::checkSWVdisMode(const CameraParameters& params)
{
    const char *newSwVdis = params.get("sw-vdis");
    bool currSwVdis = getSWVdisMode();
    if (newSwVdis != NULL) {
        CLOGD("DEBUG(%s):newSwVdis %s", "setParameters", newSwVdis);
        bool swVdisMode = true;

        if (!strcmp(newSwVdis, "off"))
            swVdisMode = false;

        m_setSWVdisMode(swVdisMode);
        m_params.set("sw-vdis", newSwVdis);
    }

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::checkHWVdisMode(__unused const CameraParameters& params)
{
    status_t ret = NO_ERROR;

    bool hwVdisMode = this->getHWVdisMode();

    CLOGD("DEBUG(%s):newHwVdis %d", "setParameters", hwVdisMode);

    ret = setDisEnable(hwVdisMode);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):setDisEnable(%d) fail", __FUNCTION__, __LINE__, hwVdisMode);
    } else {
        if (m_flagHWVDisMode != hwVdisMode) {
            m_flagHWVDisMode = hwVdisMode;
        }
    }

    return ret;
}

bool ExynosCamera1Parameters::isSWVdisMode(void)
{
    bool swVDIS_mode = false;
    bool use3DNR_dmaout = false;

    int nPreviewW, nPreviewH;
    getPreviewSize(&nPreviewW, &nPreviewH);

    if ((getRecordingHint() == true) &&
        (getHighSpeedRecording() == false) &&
        (use3DNR_dmaout == false) &&
        (getSWVdisUIMode() == true) &&
        ((nPreviewW == 1920 && nPreviewH == 1080) || (nPreviewW == 1280 && nPreviewH == 720)))
    {
        swVDIS_mode = true;
    }

    return swVDIS_mode;
}

bool ExynosCamera1Parameters::isSWVdisModeWithParam(int nPreviewW, int nPreviewH)
{
    bool swVDIS_mode = false;
    bool use3DNR_dmaout = false;

    if ((getRecordingHint() == true) &&
        (getHighSpeedRecording() == false) &&
        (use3DNR_dmaout == false) &&
        (getSWVdisUIMode() == true) &&
        ((nPreviewW == 1920 && nPreviewH == 1080) || (nPreviewW == 1280 && nPreviewH == 720)))
    {
        swVDIS_mode = true;
    }

    return swVDIS_mode;
}

bool ExynosCamera1Parameters::getHWVdisMode(void)
{
    bool ret = this->getVideoStabilization();

    /*
     * Only true case,
     * we will test whether support or not.
     */
    if (ret == true) {
        switch (getCameraId()) {
        case CAMERA_ID_BACK:
#ifdef SUPPORT_BACK_HW_VDIS
            ret = SUPPORT_BACK_HW_VDIS;
#else
            ret = false;
#endif
            break;
        case CAMERA_ID_FRONT:
#ifdef SUPPORT_FRONT_HW_VDIS
            ret = SUPPORT_FRONT_HW_VDIS;
#else
            ret = false;
#endif
            break;
        default:
            ret = false;
            break;
        }
    }

    return ret;
}

int ExynosCamera1Parameters::getHWVdisFormat(void)
{
    return V4L2_PIX_FMT_YUYV;
}

void ExynosCamera1Parameters::m_setSWVdisMode(bool swVdis)
{
    m_cameraInfo.swVdisMode = swVdis;
}

bool ExynosCamera1Parameters::getSWVdisMode(void)
{
    return m_cameraInfo.swVdisMode;
}

void ExynosCamera1Parameters::m_setSWVdisUIMode(bool swVdisUI)
{
    m_cameraInfo.swVdisUIMode = swVdisUI;
}

bool ExynosCamera1Parameters::getSWVdisUIMode(void)
{
    return m_cameraInfo.swVdisUIMode;
}

status_t ExynosCamera1Parameters::m_adjustPreviewSize(__unused int previewW, __unused int previewH,
                                                     int *newPreviewW, int *newPreviewH,
                                                     int *newCalHwPreviewW, int *newCalHwPreviewH)
{
    /* hack : when app give 1446, we calibrate to 1440 */
    if (*newPreviewW == 1446 && *newPreviewH == 1080) {
        CLOGW("WARN(%s):Invalid previewSize(%d/%d). so, calibrate to (1440/%d)", __FUNCTION__, *newPreviewW, *newPreviewH, *newPreviewH);
        *newPreviewW = 1440;
    }

    if (getRecordingHint() == true && getHighSpeedRecording() == true) {
        int sizeList[SIZE_LUT_INDEX_END];

        if (m_getPreviewSizeList(sizeList) == NO_ERROR) {
            /* On high-speed recording, scaling-up by SCC/SCP occurs the IS-ISP performance degradation.
               The scaling-up might be done by GSC for recording */
            *newPreviewW = (sizeList[BDS_W] < sizeList[TARGET_W])? sizeList[BDS_W] : sizeList[TARGET_W];
            *newPreviewH = (sizeList[BDS_H] < sizeList[TARGET_H])? sizeList[BDS_H] : sizeList[TARGET_H];
        } else {
            ALOGE("ERR(%s):m_getPreviewSizeList() fail", __FUNCTION__);
        }
    }

    /* calibrate H/W aligned size*/
    if (getRecordingHint() == true) {
        int videoW = 0, videoH = 0;
        ExynosRect bdsRect;

        getVideoSize(&videoW, &videoH);

        if ((videoW <= *newPreviewW) && (videoH <= *newPreviewH)) {
            {
#if defined(LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING)
                if ((videoW <= 1920 || videoH <= 1080) &&
                    (1920 < *newPreviewW || 1080 < *newPreviewH)) {

                    float videoRatio = ROUND_OFF(((float)videoW / (float)videoH), 2);

                    if (videoRatio == 1.33f) { /* 4:3 */
                        *newCalHwPreviewW = 1440;
                        *newCalHwPreviewH = 1080;
                    } else if (videoRatio == 1.77f) { /* 16:9 */
                        *newCalHwPreviewW = 1920;
                        *newCalHwPreviewH = 1080;
                    } else if (videoRatio == 1.00f) { /* 1:1 */
                        *newCalHwPreviewW = 1088;
                        *newCalHwPreviewH = 1088;
                    } else {
                        *newCalHwPreviewW = *newPreviewW;
                        *newCalHwPreviewH = *newPreviewH;
                    }

                    if (*newCalHwPreviewW != *newPreviewW  ||
                        *newCalHwPreviewH != *newPreviewH) {
                        CLOGW("WARN(%s[%d]):Limit hw preview size until %d x %d when videoSize(%d x %d)",
                            __FUNCTION__, __LINE__, *newCalHwPreviewW, *newCalHwPreviewH, videoW, videoH);
                    }
                } else
#endif
                {
                    *newCalHwPreviewW = *newPreviewW;
                    *newCalHwPreviewH = *newPreviewH;
                }
            }
        } else {
            /* video size > preview size : Use BDS size for SCP output size */
            {
                ALOGV("DEBUG(%s[%d]):preview(%dx%d) is smaller than video(%dx%d)",
                        __FUNCTION__, __LINE__, *newPreviewW, *newPreviewH, videoW, videoH);

                /* If the video ratio is differ with preview ratio,
                   the default ratio is set into preview ratio */
                if (SIZE_RATIO(*newPreviewW, *newPreviewH) != SIZE_RATIO(videoW, videoH))
                    ALOGW("WARN(%s): preview ratio(%dx%d) is not matched with video ratio(%dx%d)", __FUNCTION__,
                            *newPreviewW, *newPreviewH, videoW, videoH);

                if (m_isSupportedPreviewSize(*newPreviewW, *newPreviewH) == false) {
                    ALOGE("ERR(%s): new preview size is invalid(%dx%d)",
                            __FUNCTION__, *newPreviewW, *newPreviewH);
                    return BAD_VALUE;
                }

                /*
                 * This call is to get real preview size.
                 * so, HW dis size must not be added.
                 */
                m_getPreviewBdsSize(&bdsRect);

                if ((bdsRect.w <= videoW) && (bdsRect.h <= videoH)) {
                    *newCalHwPreviewW = bdsRect.w;
                    *newCalHwPreviewH = bdsRect.h;
                }
                else {
                    *newCalHwPreviewW = videoW;
                    *newCalHwPreviewH = videoH;

                    if (SIZE_RATIO(*newPreviewW, *newPreviewH) != SIZE_RATIO(videoW, videoH)) {
                        int  cropX, cropY, cropW, cropH;

                        getCropRectAlign(videoW, videoH,
                                         *newPreviewW, *newPreviewH,
                                         &cropX, &cropY,
                                         &cropW, &cropH,
                                         CAMERA_MAGIC_ALIGN, 2,
                                         0, 1);

                        *newCalHwPreviewW = cropW;
                        *newCalHwPreviewH = cropH;
                    }
                }
            }
        }
    } else if (getHighResolutionCallbackMode() == true) {
        if(CAMERA_LCD_SIZE == LCD_SIZE_1280_720) {
            *newCalHwPreviewW = 1280;
            *newCalHwPreviewH = 720;
        } else {
            *newCalHwPreviewW = 1920;
            *newCalHwPreviewH = 1080;
        }
    } else {
        *newCalHwPreviewW = *newPreviewW;
        *newCalHwPreviewH = *newPreviewH;
    }

#if defined(SCALER_MAX_SCALE_DOWN_RATIO)
    {
        ExynosRect bdsRect;

        if (getPreviewBdsSize(&bdsRect) == NO_ERROR) {
            int minW = bdsRect.w / SCALER_MAX_SCALE_DOWN_RATIO;
            int minH = bdsRect.h / SCALER_MAX_SCALE_DOWN_RATIO;
            int adjustW = 1;
            int adjustH = 1;

            if ((*newCalHwPreviewW < minW) || (*newCalHwPreviewH < minH)) {
                if (*newCalHwPreviewW < minW) {
                    adjustW = ROUND_UP(minW / *newCalHwPreviewW, 2);
                    CLOGW("WARN(%s): newCalHwPreviewW=%d,  minW=%d, adjustW=%d", __FUNCTION__,
                        *newCalHwPreviewW, minW, adjustW);
                }
                if (*newCalHwPreviewH < minH) {
                    adjustH = ROUND_UP(minH / *newCalHwPreviewH, 2);
                    CLOGW("WARN(%s): newCalHwPreviewH=%d,  minH=%d, adjustH=%d", __FUNCTION__,
                        *newCalHwPreviewH, minH, adjustH);
                }
                adjustW = (adjustW > adjustH) ? adjustW : adjustH;
                *newCalHwPreviewW *= adjustW;
                *newCalHwPreviewH *= adjustW;
            }
        }
    }
#endif

    if (getHalVersion() == IS_HAL_VER_3_2) {
#if defined(ENABLE_FULL_FRAME)
        ExynosRect bdsRect;
        getPreviewBdsSize(&bdsRect);
        *newCalHwPreviewW = bdsRect.w;
        *newCalHwPreviewH = bdsRect.h;
#else
        /* 1. try to get exact ratio */
        if (m_isSupportedPreviewSize(*newPreviewW, *newPreviewH) == false) {
            ALOGE("ERR(%s): new preview size is invalid(%dx%d)", "Parameters", newPreviewW, newPreviewH);
        }

        /* 2. get bds size to set size to scp node due to internal scp buffer */
        int sizeList[SIZE_LUT_INDEX_END];
        if (m_getPreviewSizeList(sizeList) == NO_ERROR) {
            *newCalHwPreviewW = sizeList[BDS_W];
            *newCalHwPreviewH = sizeList[BDS_H];
        } else {
            ExynosRect bdsRect;
            getPreviewBdsSize(&bdsRect);
            *newCalHwPreviewW = bdsRect.w;
            *newCalHwPreviewH = bdsRect.h;
        }
#endif
    }

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::setCropRegion(int x, int y, int w, int h)
{
    status_t ret = NO_ERROR;

    ret = setMetaCtlCropRegion(&m_metadata, x, y, w, h);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):Failed to setMetaCtlCropRegion(%d, %d, %d, %d)",
                __FUNCTION__, __LINE__, x, y, w, h);
    }

    return ret;
}

void ExynosCamera1Parameters::m_adjustAeMode(enum aa_aemode curAeMode, enum aa_aemode *newAeMode)
{
    if (getHalVersion() != IS_HAL_VER_3_2) {
        int curMeteringMode = getMeteringMode();
        if (curAeMode == AA_AEMODE_OFF) {
            switch(curMeteringMode) {
            case METERING_MODE_AVERAGE:
                *newAeMode = AA_AEMODE_AVERAGE;
                break;
            case METERING_MODE_CENTER:
                *newAeMode = AA_AEMODE_CENTER;
                break;
            case METERING_MODE_MATRIX:
                *newAeMode = AA_AEMODE_MATRIX;
                break;
            case METERING_MODE_SPOT:
                *newAeMode = AA_AEMODE_SPOT;
                break;
            default:
                *newAeMode = curAeMode;
                break;
            }
        }
    }
}

/* TODO: Who explane this offset value? */
/* F/W's middle value is 5, and step is -4, -3, -2, -1, 0, 1, 2, 3, 4 */
void ExynosCamera1Parameters::m_setExposureCompensation(int32_t value)
{
    setMetaCtlExposureCompensation(&m_metadata, value);
#if defined(USE_SUBDIVIDED_EV)
    setMetaCtlExposureCompensationStep(&m_metadata, m_staticInfo->exposureCompensationStep);
#endif
}

int32_t ExynosCamera1Parameters::getExposureCompensation(void)
{
    int32_t expCompensation;
    getMetaCtlExposureCompensation(&m_metadata, &expCompensation);
    return expCompensation;
}

void ExynosCamera1Parameters::m_setMeteringAreas(uint32_t num, ExynosRect2 *rect2s, int *weights)
{
    uint32_t maxNumMeteringAreas = getMaxNumMeteringAreas();

    if (maxNumMeteringAreas == 0) {
        CLOGV("DEBUG(%s):maxNumMeteringAreas is 0. so, ignored", __FUNCTION__);
        return;
    }

    if (maxNumMeteringAreas < num)
        num = maxNumMeteringAreas;

    if (getAutoExposureLock() == true) {
        CLOGD("DEBUG(%s):autoExposure is Locked", __FUNCTION__);
        return;
    }

    if (num == 1) {
#ifdef CAMERA_GED_FEATURE
        int meteringMode = getMeteringMode();

        if (isRectNull(&rect2s[0]) == true) {
            switch (meteringMode) {
                case METERING_MODE_SPOT:
                    /*
                     * Even if SPOT metering mode, area must set valid values,
                     * but areas was invalid values, we change mode to CENTER.
                     */
                    m_setMeteringMode(METERING_MODE_CENTER);
                    m_cameraInfo.isTouchMetering = false;
                    break;
                case METERING_MODE_AVERAGE:
                case METERING_MODE_CENTER:
                case METERING_MODE_MATRIX:
                default:
                    /* adjust metering setting */
                    break;
            }
        } else {
            switch (meteringMode) {
                case METERING_MODE_CENTER:
                    /*
                     * SPOT metering mode in GED camera App was not set METERING_MODE_SPOT,
                     * but set metering areas only.
                     */
                    m_setMeteringMode(METERING_MODE_SPOT);
                    m_cameraInfo.isTouchMetering = true;
                    break;
                case METERING_MODE_AVERAGE:
                case METERING_MODE_MATRIX:
                case METERING_MODE_SPOT:
                default:
                    /* adjust metering setting */
                    break;
            }
        }
#endif
    } else {
        if (num > 1 && isRectEqual(&rect2s[0], &rect2s[1]) == false) {
            /* if MATRIX mode support, mode set METERING_MODE_MATRIX */
            m_setMeteringMode(METERING_MODE_AVERAGE);
            m_cameraInfo.isTouchMetering = false;
        } else {
            m_setMeteringMode(METERING_MODE_AVERAGE);
            m_cameraInfo.isTouchMetering = false;
        }
    }

    ExynosRect cropRegionRect;
    ExynosRect2 newRect2;

    getHwBayerCropRegion(&cropRegionRect.w, &cropRegionRect.h, &cropRegionRect.x, &cropRegionRect.y);

    for (uint32_t i = 0; i < num; i++) {
        bool isChangeMeteringArea = false;
#ifdef CAMERA_GED_FEATURE
        if (isRectNull(&rect2s[i]) == false)
            isChangeMeteringArea = true;
        else
            isChangeMeteringArea = false;
#else
        if ((isRectNull(&rect2s[i]) == false) ||((isRectNull(&rect2s[i]) == true) && (getMeteringMode() == METERING_MODE_SPOT)))
            isChangeMeteringArea = true;
#ifdef TOUCH_AE
        else if((getMeteringMode() == METERING_MODE_SPOT_TOUCH) || (getMeteringMode() == METERING_MODE_MATRIX_TOUCH)
            || (getMeteringMode() == METERING_MODE_CENTER_TOUCH) || (getMeteringMode() == METERING_MODE_AVERAGE_TOUCH))
            isChangeMeteringArea = true;
#endif
        else
            isChangeMeteringArea = false;
#endif
        if (isChangeMeteringArea == true) {
            CLOGD("DEBUG(%s) (%d %d %d %d) %d", __FUNCTION__, rect2s->x1, rect2s->y1, rect2s->x2, rect2s->y2, getMeteringMode());
            newRect2 = convertingAndroidArea2HWAreaBcropOut(&rect2s[i], &cropRegionRect);
            setMetaCtlAeRegion(&m_metadata, newRect2.x1, newRect2.y1,
                                newRect2.x2, newRect2.y2, weights[i]);
        }
    }
}

status_t ExynosCamera1Parameters::checkPictureFormat(const CameraParameters& params)
{
    int curPictureFormat = 0;
    int newPictureFormat = 0;
    int newHwPictureFormat = 0;
    const char *strNewPictureFormat = params.getPictureFormat();
    const char *strCurPictureFormat = m_params.getPictureFormat();

    if (strNewPictureFormat == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):newPictureFormat %s", "setParameters", strNewPictureFormat);

    if (!strcmp(strNewPictureFormat, CameraParameters::PIXEL_FORMAT_JPEG)) {
        newPictureFormat = V4L2_PIX_FMT_JPEG;
        newHwPictureFormat = SCC_OUTPUT_COLOR_FMT;
    } else {
        CLOGE("ERR(%s[%d]): Picture format(%s) is not supported!",
                __FUNCTION__, __LINE__, strNewPictureFormat);
        return BAD_VALUE;
    }

    curPictureFormat = getPictureFormat();

    if (newPictureFormat != curPictureFormat) {
        CLOGI("INFO(%s[%d]): Picture format changed, cur(%s) -> new(%s)",
                "Parameters", __LINE__, strCurPictureFormat, strNewPictureFormat);
        m_setPictureFormat(newPictureFormat);
        m_setHwPictureFormat(newHwPictureFormat);
        m_params.setPictureFormat(strNewPictureFormat);
    }

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::checkMeteringMode(const CameraParameters& params)
{
    const char *strNewMeteringMode = params.get("metering");
    int newMeteringMode = -1;
    int curMeteringMode = -1;
    if (strNewMeteringMode == NULL) {
        return NO_ERROR;
    }

    ALOGD("DEBUG(%s):strNewMeteringMode %s", "setParameters", strNewMeteringMode);

    if (!strcmp(strNewMeteringMode, "average"))
        newMeteringMode = METERING_MODE_AVERAGE;
    else if (!strcmp(strNewMeteringMode, "center"))
        newMeteringMode = METERING_MODE_CENTER;
    else if (!strcmp(strNewMeteringMode, "matrix"))
        newMeteringMode = METERING_MODE_MATRIX;
    else if (!strcmp(strNewMeteringMode, "spot"))
        newMeteringMode = METERING_MODE_SPOT;
#ifdef TOUCH_AE
    else if (!strcmp(strNewMeteringMode, "weighted-center"))
        newMeteringMode = METERING_MODE_CENTER_TOUCH;
    else if (!strcmp(strNewMeteringMode, "weighted-matrix"))
        newMeteringMode = METERING_MODE_MATRIX_TOUCH;
    else if (!strcmp(strNewMeteringMode, "weighted-spot"))
        newMeteringMode = METERING_MODE_SPOT_TOUCH;
    else if (!strcmp(strNewMeteringMode, "weighted-average"))
        newMeteringMode = METERING_MODE_AVERAGE_TOUCH;
#endif
    else {
        ALOGE("ERR(%s):Invalid metering newMetering(%s)", __FUNCTION__, strNewMeteringMode);
        return UNKNOWN_ERROR;
    }

    curMeteringMode = getMeteringMode();

    m_setMeteringMode(newMeteringMode);
    m_params.set("metering", strNewMeteringMode);

    if (curMeteringMode != newMeteringMode) {
        ALOGI("INFO(%s): Metering Area is changed (%d -> %d)", __FUNCTION__, curMeteringMode, newMeteringMode);
        m_flagMeteringRegionChanged = true;
    }

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::checkSceneMode(const CameraParameters& params)
{
    int  newSceneMode = -1;
    int  curSceneMode = -1;
    const char *strNewSceneMode = params.get(CameraParameters::KEY_SCENE_MODE);

    if (strNewSceneMode == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):strNewSceneMode %s", "setParameters", strNewSceneMode);

    if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_AUTO)) {
        newSceneMode = SCENE_MODE_AUTO;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_ACTION)) {
        newSceneMode = SCENE_MODE_ACTION;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_PORTRAIT)) {
        newSceneMode = SCENE_MODE_PORTRAIT;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_LANDSCAPE)) {
        newSceneMode = SCENE_MODE_LANDSCAPE;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_NIGHT)) {
        newSceneMode = SCENE_MODE_NIGHT;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_NIGHT_PORTRAIT)) {
        newSceneMode = SCENE_MODE_NIGHT_PORTRAIT;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_THEATRE)) {
        newSceneMode = SCENE_MODE_THEATRE;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_BEACH)) {
        newSceneMode = SCENE_MODE_BEACH;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_SNOW)) {
        newSceneMode = SCENE_MODE_SNOW;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_SUNSET)) {
        newSceneMode = SCENE_MODE_SUNSET;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_STEADYPHOTO)) {
        newSceneMode = SCENE_MODE_STEADYPHOTO;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_FIREWORKS)) {
        newSceneMode = SCENE_MODE_FIREWORKS;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_SPORTS)) {
        newSceneMode = SCENE_MODE_SPORTS;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_PARTY)) {
        newSceneMode = SCENE_MODE_PARTY;
    } else if (!strcmp(strNewSceneMode, CameraParameters::SCENE_MODE_CANDLELIGHT)) {
        newSceneMode = SCENE_MODE_CANDLELIGHT;
    } else {
        CLOGE("ERR(%s):unmatched scene_mode(%s)", "Parameters", strNewSceneMode);
        return BAD_VALUE;
    }

    curSceneMode = getSceneMode();

    if (curSceneMode != newSceneMode) {
        m_setSceneMode(newSceneMode);
        m_params.set(CameraParameters::KEY_SCENE_MODE, strNewSceneMode);
        updatePreviewFpsRange();
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setSceneMode(int value)
{
    enum aa_mode mode = AA_CONTROL_AUTO;
    enum aa_scene_mode sceneMode = AA_SCENE_MODE_FACE_PRIORITY;

    switch (value) {
    case SCENE_MODE_PORTRAIT:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_PORTRAIT;
        break;
    case SCENE_MODE_LANDSCAPE:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_LANDSCAPE;
        break;
    case SCENE_MODE_NIGHT:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_NIGHT;
        break;
    case SCENE_MODE_BEACH:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_BEACH;
        break;
    case SCENE_MODE_SNOW:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_SNOW;
        break;
    case SCENE_MODE_SUNSET:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_SUNSET;
        break;
    case SCENE_MODE_FIREWORKS:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_FIREWORKS;
        break;
    case SCENE_MODE_SPORTS:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_SPORTS;
        break;
    case SCENE_MODE_PARTY:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_PARTY;
        break;
    case SCENE_MODE_CANDLELIGHT:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_CANDLELIGHT;
        break;
    case SCENE_MODE_STEADYPHOTO:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_STEADYPHOTO;
        break;
    case SCENE_MODE_ACTION:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_ACTION;
        break;
    case SCENE_MODE_NIGHT_PORTRAIT:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_NIGHT_PORTRAIT;
        break;
    case SCENE_MODE_THEATRE:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_THEATRE;
        break;
    case SCENE_MODE_AQUA:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_AQUA;
        break;
    case SCENE_MODE_AUTO:
    default:
        mode = AA_CONTROL_AUTO;
        sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
        break;
    }

    m_cameraInfo.sceneMode = value;
    setMetaCtlSceneMode(&m_metadata, mode, sceneMode);
    m_cameraInfo.whiteBalanceMode = m_convertMetaCtlAwbMode(&m_metadata);
}

status_t ExynosCamera1Parameters::checkFocusMode(const CameraParameters& params)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return NO_ERROR;
    } else {
        int  newFocusMode = -1;
        int  curFocusMode = -1;
        const char *strFocusMode = params.get(CameraParameters::KEY_FOCUS_MODE);
        const char *strNewFocusMode = m_adjustFocusMode(strFocusMode);

        if (strNewFocusMode == NULL) {
            return NO_ERROR;
        }

        CLOGD("DEBUG(%s):strNewFocusMode %s", "setParameters", strNewFocusMode);

        if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_AUTO)) {
            newFocusMode = FOCUS_MODE_AUTO;
            m_params.set(CameraParameters::KEY_FOCUS_DISTANCES,
                    BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR);
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_INFINITY)) {
            newFocusMode = FOCUS_MODE_INFINITY;
            m_params.set(CameraParameters::KEY_FOCUS_DISTANCES,
                    BACK_CAMERA_INFINITY_FOCUS_DISTANCES_STR);
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_MACRO)) {
            newFocusMode = FOCUS_MODE_MACRO;
            m_params.set(CameraParameters::KEY_FOCUS_DISTANCES,
                    BACK_CAMERA_MACRO_FOCUS_DISTANCES_STR);
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_FIXED)) {
            newFocusMode = FOCUS_MODE_FIXED;
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_EDOF)) {
            newFocusMode = FOCUS_MODE_EDOF;
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO)) {
            newFocusMode = FOCUS_MODE_CONTINUOUS_VIDEO;
        } else if (!strcmp(strNewFocusMode, CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE)) {
            newFocusMode = FOCUS_MODE_CONTINUOUS_PICTURE;
        } else if (!strcmp(strNewFocusMode, "face-priority")) {
            newFocusMode = FOCUS_MODE_CONTINUOUS_PICTURE;
        } else if (!strcmp(strNewFocusMode, "continuous-picture-macro")) {
            newFocusMode = FOCUS_MODE_CONTINUOUS_PICTURE_MACRO;
        } else {
            CLOGE("ERR(%s):unmatched focus_mode(%s)", __FUNCTION__, strNewFocusMode);
            return BAD_VALUE;
        }

        if (!(newFocusMode & getSupportedFocusModes())){
            CLOGE("ERR(%s[%d]): Focus mode(%s) is not supported!", __FUNCTION__, __LINE__, strNewFocusMode);
            return BAD_VALUE;
        }

        m_setFocusMode(newFocusMode);
        m_params.set(CameraParameters::KEY_FOCUS_MODE, strNewFocusMode);

        return NO_ERROR;
    }
}

const char *ExynosCamera1Parameters::m_adjustFocusMode(const char *focusMode)
{
    int sceneMode = getSceneMode();
    const char *newFocusMode = NULL;

    /* TODO: vendor specific adjust */

    newFocusMode = focusMode;

    return newFocusMode;
}

void ExynosCamera1Parameters::m_setFocusMode(int focusMode)
{
    m_cameraInfo.focusMode = focusMode;

    if(getZoomActiveOn()) {
        ALOGD("DEBUG(%s):zoom moving..", "setParameters");
        return;
    }

    /* TODO: Notify auto focus activity */
    if(getPreviewRunning() == true) {
        ALOGD("set Focus Mode(%s[%d]) !!!!", __FUNCTION__, __LINE__);
        m_activityControl->setAutoFocusMode(focusMode);
    } else {
        m_setFocusmodeSetting = true;
    }
}

status_t ExynosCamera1Parameters::checkFocusAreas(const CameraParameters& params)
{
    int ret = NO_ERROR;
    const char *newFocusAreas = params.get(CameraParameters::KEY_FOCUS_AREAS);
    int curFocusMode = getFocusMode();
    uint32_t maxNumFocusAreas = getMaxNumFocusAreas();

    if (newFocusAreas == NULL) {
        ExynosRect2 nullRect2[1];
        nullRect2[0].x1 = 0;
        nullRect2[0].y1 = 0;
        nullRect2[0].x2 = 0;
        nullRect2[0].y2 = 0;

        if (m_cameraInfo.numValidFocusArea != 0)
            m_setFocusAreas(0, nullRect2, NULL);

        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):newFocusAreas %s", "setParameters", newFocusAreas);

    /* In CameraParameters.h */
    /*
     * Focus area only has effect if the cur focus mode is FOCUS_MODE_AUTO,
     * FOCUS_MODE_MACRO, FOCUS_MODE_CONTINUOUS_VIDEO, or
     * FOCUS_MODE_CONTINUOUS_PICTURE.
     */
    if (curFocusMode & FOCUS_MODE_AUTO
         || curFocusMode & FOCUS_MODE_MACRO
         || curFocusMode & FOCUS_MODE_CONTINUOUS_VIDEO
         || curFocusMode & FOCUS_MODE_CONTINUOUS_PICTURE
         || curFocusMode & FOCUS_MODE_CONTINUOUS_PICTURE_MACRO) {

        /* ex : (-10,-10,0,0,300),(0,0,10,10,700) */
        ExynosRect2 *rect2s = new ExynosRect2[maxNumFocusAreas];
        int         *weights = new int[maxNumFocusAreas];

        uint32_t validFocusedAreas = bracketsStr2Ints((char *)newFocusAreas, maxNumFocusAreas, rect2s, weights, 1);

        /* Check duplicate area */
        if (validFocusedAreas > 1) {
            for (uint32_t k = 0; k < validFocusedAreas; k++) {
                if (rect2s[k].x1 == rect2s[k+1].x1 &&
                        rect2s[k].y1 == rect2s[k+1].y1 &&
                        rect2s[k].x2 == rect2s[k+1].x2 &&
                        rect2s[k].y2 == rect2s[k+1].y2)
                    validFocusedAreas = 0;
            }
        }

        if (0 < validFocusedAreas) {
            /* CameraParameters.h */
            /*
              * A special case of single focus area (0,0,0,0,0) means driver to decide
              * the focus area. For example, the driver may use more signals to decide
              * focus areas and change them dynamically. Apps can set (0,0,0,0,0) if they
              * want the driver to decide focus areas.
              */
            m_setFocusAreas(validFocusedAreas, rect2s, weights);
            m_params.set(CameraParameters::KEY_FOCUS_AREAS, newFocusAreas);
        } else {
            CLOGE("ERR(%s):FocusAreas value is invalid", __FUNCTION__);
            ret = UNKNOWN_ERROR;
        }

        delete [] rect2s;
        delete [] weights;
    }

    return ret;
}

void ExynosCamera1Parameters::m_setFocusAreas(uint32_t numValid, ExynosRect2 *rect2s, int *weights)
{
    uint32_t maxNumFocusAreas = getMaxNumFocusAreas();
    if (maxNumFocusAreas < numValid)
        numValid = maxNumFocusAreas;

    if ((numValid == 1 || numValid == 0) && (isRectNull(&rect2s[0]) == true)) {
        /* m_setFocusMode(FOCUS_MODE_AUTO); */
        ExynosRect2 newRect2(0,0,0,0);
        m_activityControl->setAutoFcousArea(newRect2, 1000);

        m_activityControl->touchAFMode = false;
        m_activityControl->touchAFModeForFlash = false;
    } else {
        ExynosRect cropRegionRect;
        ExynosRect2 newRect2;

        getHwBayerCropRegion(&cropRegionRect.w, &cropRegionRect.h, &cropRegionRect.x, &cropRegionRect.y);

        for (uint32_t i = 0; i < numValid; i++) {
            newRect2 = convertingAndroidArea2HWAreaBcropOut(&rect2s[i], &cropRegionRect);
            /*setMetaCtlAfRegion(&m_metadata, rect2s[i].x1, rect2s[i].y1,
                                rect2s[i].x2, rect2s[i].y2, weights[i]);*/
            m_activityControl->setAutoFcousArea(newRect2, weights[i]);
        }
        m_activityControl->touchAFMode = true;
        m_activityControl->touchAFModeForFlash = true;
    }

    m_cameraInfo.numValidFocusArea = numValid;
}

void ExynosCamera1Parameters::m_setExifChangedAttribute(exif_attribute_t *exifInfo,
                                                       ExynosRect       *pictureRect,
                                                       ExynosRect       *thumbnailRect,
                                                       __unused camera2_dm       *dm,
                                                       camera2_udm      *udm)
{
    /* 2 0th IFD TIFF Tags */
    /* 3 Width */
    exifInfo->width = pictureRect->w;
    /* 3 Height */
    exifInfo->height = pictureRect->h;

    /* 3 Orientation */
    switch (m_cameraInfo.rotation) {
    case 90:
        exifInfo->orientation = EXIF_ORIENTATION_90;
        break;
    case 180:
        exifInfo->orientation = EXIF_ORIENTATION_180;
        break;
    case 270:
        exifInfo->orientation = EXIF_ORIENTATION_270;
        break;
    case 0:
    default:
        exifInfo->orientation = EXIF_ORIENTATION_UP;
        break;
    }

    /* 3 Maker note */
    /* back-up udm info for exif's maker note */
    memcpy((void *)mDebugInfo.debugData[APP_MARKER_4], (void *)udm, mDebugInfo.debugSize[APP_MARKER_4]);
    exifInfo->maker_note_size = 0;

    /* 3 Date time */
    struct timeval rawtime;
    struct tm timeinfo;
    gettimeofday(&rawtime, NULL);
    localtime_r((time_t *)&rawtime.tv_sec, &timeinfo);
    strftime((char *)exifInfo->date_time, 20, "%Y:%m:%d %H:%M:%S", &timeinfo);
    snprintf((char *)exifInfo->sec_time, 5, "%04d", (int)(rawtime.tv_usec/1000));

    /* 2 0th IFD Exif Private Tags */
    bool flagSLSIAlgorithm = true;
    /*
     * vendorSpecific2[100]      : exposure
     * vendorSpecific2[101]      : iso(gain)
     * vendorSpecific2[102] /256 : Bv
     * vendorSpecific2[103]      : Tv
     */

    /* 3 ISO Speed Rating */
    exifInfo->iso_speed_rating = udm->internal.vendorSpecific2[101];

    /* 3 Exposure Time */
    exifInfo->exposure_time.num = 1;

    if (udm->ae.vendorSpecific[0] == 0xAEAEAEAE)
        exifInfo->exposure_time.den = (uint32_t)udm->ae.vendorSpecific[64];
    else
        exifInfo->exposure_time.den = (uint32_t)udm->internal.vendorSpecific2[100];

    /* 3 Shutter Speed */
    exifInfo->shutter_speed.num = (uint32_t)(ROUND_OFF_HALF(((double)(udm->internal.vendorSpecific2[103] / 256.f) * EXIF_DEF_APEX_DEN), 0));
    exifInfo->shutter_speed.den = EXIF_DEF_APEX_DEN;

    if (getHalVersion() == IS_HAL_VER_3_2) {
        /* 3 Aperture */
        exifInfo->aperture.num = APEX_FNUM_TO_APERTURE((double)(exifInfo->fnumber.num) / (double)(exifInfo->fnumber.den)) * COMMON_DENOMINATOR;
        exifInfo->aperture.den = COMMON_DENOMINATOR;

        /* 3 Max Aperture */
        exifInfo->max_aperture.num = APEX_FNUM_TO_APERTURE((double)(exifInfo->fnumber.num) / (double)(exifInfo->fnumber.den)) * COMMON_DENOMINATOR;
        exifInfo->max_aperture.den = COMMON_DENOMINATOR;
    } else {
        /* 3 Aperture */
        exifInfo->aperture.num = APEX_FNUM_TO_APERTURE((double)(exifInfo->fnumber.num) / (double)(exifInfo->fnumber.den)) * m_staticInfo->apertureDen;
        exifInfo->aperture.den = m_staticInfo->apertureDen;

        /* 3 Max Aperture */
        exifInfo->max_aperture.num = APEX_FNUM_TO_APERTURE((double)(exifInfo->fnumber.num) / (double)(exifInfo->fnumber.den)) * m_staticInfo->apertureDen;
        exifInfo->max_aperture.den = m_staticInfo->apertureDen;
    }

    /* 3 Brightness */
    int temp = udm->internal.vendorSpecific2[102];
    if ((int)udm->ae.vendorSpecific[102] < 0)
        temp = -temp;

    exifInfo->brightness.num = (int32_t)(ROUND_OFF_HALF((double)((temp * EXIF_DEF_APEX_DEN) / 256.f), 0));
    if ((int)udm->ae.vendorSpecific[102] < 0)
        exifInfo->brightness.num = -exifInfo->brightness.num;

    exifInfo->brightness.den = EXIF_DEF_APEX_DEN;

    CLOGD("DEBUG(%s):udm->internal.vendorSpecific2[100](%d)", __FUNCTION__, udm->internal.vendorSpecific2[100]);
    CLOGD("DEBUG(%s):udm->internal.vendorSpecific2[101](%d)", __FUNCTION__, udm->internal.vendorSpecific2[101]);
    CLOGD("DEBUG(%s):udm->internal.vendorSpecific2[102](%d)", __FUNCTION__, udm->internal.vendorSpecific2[102]);
    CLOGD("DEBUG(%s):udm->internal.vendorSpecific2[103](%d)", __FUNCTION__, udm->internal.vendorSpecific2[103]);

    CLOGD("DEBUG(%s):iso_speed_rating(%d)", __FUNCTION__, exifInfo->iso_speed_rating);
    CLOGD("DEBUG(%s):exposure_time(%d/%d)", __FUNCTION__, exifInfo->exposure_time.num, exifInfo->exposure_time.den);
    CLOGD("DEBUG(%s):shutter_speed(%d/%d)", __FUNCTION__, exifInfo->shutter_speed.num, exifInfo->shutter_speed.den);
    CLOGD("DEBUG(%s):aperture     (%d/%d)", __FUNCTION__, exifInfo->aperture.num, exifInfo->aperture.den);
    CLOGD("DEBUG(%s):brightness   (%d/%d)", __FUNCTION__, exifInfo->brightness.num, exifInfo->brightness.den);

    /* 3 Exposure Bias */
    exifInfo->exposure_bias.num = (int32_t)getExposureCompensation() * (m_staticInfo->exposureCompensationStep * 10);
    exifInfo->exposure_bias.den = 10;

    /* 3 Metering Mode */
    switch (m_cameraInfo.meteringMode) {
    case METERING_MODE_CENTER:
        exifInfo->metering_mode = EXIF_METERING_CENTER;
        break;
    case METERING_MODE_MATRIX:
        exifInfo->metering_mode = EXIF_METERING_AVERAGE;
        break;
    case METERING_MODE_SPOT:
        exifInfo->metering_mode = EXIF_METERING_SPOT;
        break;
    case METERING_MODE_AVERAGE:
    default:
        exifInfo->metering_mode = EXIF_METERING_AVERAGE;
        break;
    }

    /* 3 Flash */
    if (m_cameraInfo.flashMode == FLASH_MODE_OFF) {
        exifInfo->flash = 0;
    } else if (m_cameraInfo.flashMode == FLASH_MODE_TORCH) {
        exifInfo->flash = 1;
    } else {
        exifInfo->flash = getMarkingOfExifFlash();
    }

    /* 3 White Balance */
    switch (m_cameraInfo.whiteBalanceMode) {
    case WHITE_BALANCE_AUTO:
         exifInfo->white_balance = EXIF_WB_AUTO;
        break;
    case WHITE_BALANCE_INCANDESCENT:
        exifInfo->white_balance = EXIF_WB_INCANDESCENT;
        break;
    case WHITE_BALANCE_FLUORESCENT:
        exifInfo->white_balance = EXIF_WB_FLUORESCENT;
        break;
    case WHITE_BALANCE_DAYLIGHT:
        exifInfo->white_balance = EXIF_WB_DAYLIGHT;
        break;
    case WHITE_BALANCE_CLOUDY_DAYLIGHT:
        exifInfo->white_balance = EXIF_WB_CLOUDY;
        break;
    default:
        exifInfo->white_balance = EXIF_WB_MANUAL;
        break;
    }

    /* 3 Focal Length in 35mm length */
    exifInfo->focal_length_in_35mm_length = m_staticInfo->focalLengthIn35mmLength;

    /* 3 Scene Capture Type */
    switch (m_cameraInfo.sceneMode) {
    case SCENE_MODE_PORTRAIT:
        exifInfo->scene_capture_type = EXIF_SCENE_PORTRAIT;
        break;
    case SCENE_MODE_LANDSCAPE:
        exifInfo->scene_capture_type = EXIF_SCENE_LANDSCAPE;
        break;
    case SCENE_MODE_NIGHT:
        exifInfo->scene_capture_type = EXIF_SCENE_NIGHT;
        break;
    default:
        exifInfo->scene_capture_type = EXIF_SCENE_STANDARD;
        break;
    }

    /* 3 Image Unique ID */
    struct v4l2_ext_controls ctrls;
    struct v4l2_ext_control ctrl;
    int uniqueId = 0;
    char uniqueIdBuf[32] = {0,};

    memset(&ctrls, 0, sizeof(struct v4l2_ext_controls));
    memset(&ctrl, 0, sizeof(struct v4l2_ext_control));

    ctrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ctrls.count = 1;
    ctrls.controls = &ctrl;
    ctrl.id = V4L2_CID_CAM_SENSOR_FW_VER;
    ctrl.string = uniqueIdBuf;

    /* 2 0th IFD GPS Info Tags */
    if (m_cameraInfo.gpsLatitude != 0 && m_cameraInfo.gpsLongitude != 0) {
        if (m_cameraInfo.gpsLatitude > 0)
            strncpy((char *)exifInfo->gps_latitude_ref, "N", 2);
        else
            strncpy((char *)exifInfo->gps_latitude_ref, "S", 2);

        if (m_cameraInfo.gpsLongitude > 0)
            strncpy((char *)exifInfo->gps_longitude_ref, "E", 2);
        else
            strncpy((char *)exifInfo->gps_longitude_ref, "W", 2);

        if (m_cameraInfo.gpsAltitude > 0)
            exifInfo->gps_altitude_ref = 0;
        else
            exifInfo->gps_altitude_ref = 1;

        double latitude = fabs(m_cameraInfo.gpsLatitude);
        double longitude = fabs(m_cameraInfo.gpsLongitude);
        double altitude = fabs(m_cameraInfo.gpsAltitude);

        exifInfo->gps_latitude[0].num = (uint32_t)latitude;
        exifInfo->gps_latitude[0].den = 1;
        exifInfo->gps_latitude[1].num = (uint32_t)((latitude - exifInfo->gps_latitude[0].num) * 60);
        exifInfo->gps_latitude[1].den = 1;
        exifInfo->gps_latitude[2].num = (uint32_t)(round((((latitude - exifInfo->gps_latitude[0].num) * 60)
                                        - exifInfo->gps_latitude[1].num) * 60));
        exifInfo->gps_latitude[2].den = 1;

        exifInfo->gps_longitude[0].num = (uint32_t)longitude;
        exifInfo->gps_longitude[0].den = 1;
        exifInfo->gps_longitude[1].num = (uint32_t)((longitude - exifInfo->gps_longitude[0].num) * 60);
        exifInfo->gps_longitude[1].den = 1;
        exifInfo->gps_longitude[2].num = (uint32_t)(round((((longitude - exifInfo->gps_longitude[0].num) * 60)
                                        - exifInfo->gps_longitude[1].num) * 60));
        exifInfo->gps_longitude[2].den = 1;

        exifInfo->gps_altitude.num = (uint32_t)altitude;
        exifInfo->gps_altitude.den = 1;

        struct tm tm_data;
        gmtime_r(&m_cameraInfo.gpsTimeStamp, &tm_data);
        exifInfo->gps_timestamp[0].num = tm_data.tm_hour;
        exifInfo->gps_timestamp[0].den = 1;
        exifInfo->gps_timestamp[1].num = tm_data.tm_min;
        exifInfo->gps_timestamp[1].den = 1;
        exifInfo->gps_timestamp[2].num = tm_data.tm_sec;
        exifInfo->gps_timestamp[2].den = 1;
        snprintf((char*)exifInfo->gps_datestamp, sizeof(exifInfo->gps_datestamp),
                "%04d:%02d:%02d", tm_data.tm_year + 1900, tm_data.tm_mon + 1, tm_data.tm_mday);

        exifInfo->enableGps = true;
    } else {
        exifInfo->enableGps = false;
    }

    /* 2 1th IFD TIFF Tags */
    exifInfo->widthThumb = thumbnailRect->w;
    exifInfo->heightThumb = thumbnailRect->h;

    setMarkingOfExifFlash(0);
}

/* for CAMERA2_API_SUPPORT */
void ExynosCamera1Parameters::m_setExifChangedAttribute(exif_attribute_t *exifInfo,
                                                       ExynosRect       *pictureRect,
                                                       ExynosRect       *thumbnailRect,
                                                       camera2_shot_t   *shot)
{
    /* JPEG Picture Size */
    exifInfo->width = pictureRect->w;
    exifInfo->height = pictureRect->h;

    /* Orientation */
    switch (shot->ctl.jpeg.orientation) {
    case 90:
        exifInfo->orientation = EXIF_ORIENTATION_90;
        break;
    case 180:
        exifInfo->orientation = EXIF_ORIENTATION_180;
        break;
    case 270:
        exifInfo->orientation = EXIF_ORIENTATION_270;
        break;
    case 0:
    default:
        exifInfo->orientation = EXIF_ORIENTATION_UP;
        break;
    }

    /* Maker Note Size */
    /* back-up udm info for exif's maker note */
    memcpy((void *)mDebugInfo.debugData[4], (void *)&shot->udm, mDebugInfo.debugSize[APP_MARKER_4]);

    /* TODO */
#if 0
    if (getSeriesShotCount() && getShotMode() != SHOT_MODE_BEST_PHOTO) {
        unsigned char l_makernote[98] = { 0x07, 0x00, 0x01, 0x00, 0x07, 0x00, 0x04, 0x00, 0x00, 0x00,
            0x30, 0x31, 0x30, 0x30, 0x02, 0x00, 0x04, 0x00, 0x01, 0x00,
            0x00, 0x00, 0x00, 0x20, 0x01, 0x00, 0x40, 0x00, 0x04, 0x00,
            0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00,
            0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x10, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0x5A, 0x00,
            0x00, 0x00, 0x50, 0x00, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, 0x01, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        long long int mCityId = getCityId();
        l_makernote[46] = getWeatherId();
        memcpy(l_makernote + 90, &mCityId, 8);
        exifInfo->maker_note_size = 98;
        memcpy(exifInfo->maker_note, l_makernote, sizeof(l_makernote));
    } else {
        exifInfo->maker_note_size = 0;
    }
#else
    exifInfo->maker_note_size = 0;
#endif

    /* Date Time */
    struct timeval rawtime;
    struct tm timeinfo;
    gettimeofday(&rawtime, NULL);
    localtime_r((time_t *)&rawtime.tv_sec, &timeinfo);
    strftime((char *)exifInfo->date_time, 20, "%Y:%m:%d %H:%M:%S", &timeinfo);
    sprintf((char *)exifInfo->sec_time, "%d", (int)(rawtime.tv_usec/1000));

    /* Exif Private Tags */
    bool flagSLSIAlgorithm = true;
    /*
     * vendorSpecific2[0] : info
     * vendorSpecific2[100] : 0:sirc 1:cml
     * vendorSpecific2[101] : cml exposure
     * vendorSpecific2[102] : cml iso(gain)
     * vendorSpecific2[103] : cml Bv
     */

    /* ISO Speed Rating */
    exifInfo->iso_speed_rating = shot->dm.aa.vendor_isoValue;

    /* Exposure Time */
    exifInfo->exposure_time.num = 1;
    /* HACK : Sometimes, F/W does NOT send the exposureTime */
    if (shot->dm.sensor.exposureTime != 0)
        exifInfo->exposure_time.den = (uint32_t) 1e9 / shot->dm.sensor.exposureTime;
    else
        exifInfo->exposure_time.num = 0;

    /* Shutter Speed */
    exifInfo->shutter_speed.num = (uint32_t) (ROUND_OFF_HALF(((double) (shot->udm.internal.vendorSpecific2[104] / 256.f) * EXIF_DEF_APEX_DEN), 0));
    exifInfo->shutter_speed.den = EXIF_DEF_APEX_DEN;

    /* Aperture */
    exifInfo->aperture.num = APEX_FNUM_TO_APERTURE((double) (exifInfo->fnumber.num) / (double) (exifInfo->fnumber.den)) * COMMON_DENOMINATOR;
    exifInfo->aperture.den = COMMON_DENOMINATOR;

    /* Max Aperture */
    exifInfo->max_aperture.num = APEX_FNUM_TO_APERTURE((double) (exifInfo->fnumber.num) / (double) (exifInfo->fnumber.den)) * COMMON_DENOMINATOR;
    exifInfo->max_aperture.den = COMMON_DENOMINATOR;

    /* Brightness */
    int temp = shot->udm.internal.vendorSpecific2[103];
    if ((int) shot->udm.ae.vendorSpecific[103] < 0)
        temp = -temp;
    exifInfo->brightness.num = (int32_t) (ROUND_OFF_HALF((double)((temp * EXIF_DEF_APEX_DEN)/256.f), 0));
    if ((int) shot->udm.ae.vendorSpecific[103] < 0)
        exifInfo->brightness.num = -exifInfo->brightness.num;
    exifInfo->brightness.den = EXIF_DEF_APEX_DEN;

    ALOGD("DEBUG(%s):udm->internal.vendorSpecific2[101](%d)", __FUNCTION__, shot->udm.internal.vendorSpecific2[101]);
    ALOGD("DEBUG(%s):udm->internal.vendorSpecific2[102](%d)", __FUNCTION__, shot->udm.internal.vendorSpecific2[102]);
    ALOGD("DEBUG(%s):udm->internal.vendorSpecific2[103](%d)", __FUNCTION__, shot->udm.internal.vendorSpecific2[103]);
    ALOGD("DEBUG(%s):udm->internal.vendorSpecific2[104](%d)", __FUNCTION__, shot->udm.internal.vendorSpecific2[104]);

    ALOGD("DEBUG(%s):iso_speed_rating(%d)", __FUNCTION__, exifInfo->iso_speed_rating);
    ALOGD("DEBUG(%s):exposure_time(%d/%d)", __FUNCTION__, exifInfo->exposure_time.num, exifInfo->exposure_time.den);
    ALOGD("DEBUG(%s):shutter_speed(%d/%d)", __FUNCTION__, exifInfo->shutter_speed.num, exifInfo->shutter_speed.den);
    ALOGD("DEBUG(%s):aperture     (%d/%d)", __FUNCTION__, exifInfo->aperture.num, exifInfo->aperture.den);
    ALOGD("DEBUG(%s):brightness   (%d/%d)", __FUNCTION__, exifInfo->brightness.num, exifInfo->brightness.den);

    /* Exposure Bias */
#if defined(USE_SUBDIVIDED_EV)
    exifInfo->exposure_bias.num = shot->ctl.aa.aeExpCompensation * (m_staticInfo->exposureCompensationStep * 10);
#else
    exifInfo->exposure_bias.num =
        (shot->ctl.aa.aeExpCompensation) * (m_staticInfo->exposureCompensationStep * 10);
#endif
    exifInfo->exposure_bias.den = 10;

    /* Metering Mode */
    {
        switch (shot->ctl.aa.aeMode) {
        case AA_AEMODE_CENTER:
            exifInfo->metering_mode = EXIF_METERING_CENTER;
            break;
        case AA_AEMODE_MATRIX:
            exifInfo->metering_mode = EXIF_METERING_AVERAGE;
            break;
        case AA_AEMODE_SPOT:
            exifInfo->metering_mode = EXIF_METERING_SPOT;
            break;
        default:
            exifInfo->metering_mode = EXIF_METERING_AVERAGE;
            break;
        }
    }

    /* Flash Mode */
    if (shot->ctl.flash.flashMode == CAM2_FLASH_MODE_OFF) {
        exifInfo->flash = 0;
    } else if (shot->ctl.flash.flashMode == CAM2_FLASH_MODE_TORCH) {
        exifInfo->flash = 1;
    } else {
        exifInfo->flash = getMarkingOfExifFlash();
    }

    /* White Balance */
    if (shot->ctl.aa.awbMode == AA_AWBMODE_WB_AUTO)
        exifInfo->white_balance = EXIF_WB_AUTO;
    else
        exifInfo->white_balance = EXIF_WB_MANUAL;

    /* Focal Length in 35mm length */
    exifInfo->focal_length_in_35mm_length = getFocalLengthIn35mmFilm();

    /* Scene Capture Type */
    switch (shot->ctl.aa.sceneMode) {
    case AA_SCENE_MODE_PORTRAIT:
    case AA_SCENE_MODE_FACE_PRIORITY:
        exifInfo->scene_capture_type = EXIF_SCENE_PORTRAIT;
        break;
    case AA_SCENE_MODE_LANDSCAPE:
        exifInfo->scene_capture_type = EXIF_SCENE_LANDSCAPE;
        break;
    case AA_SCENE_MODE_NIGHT:
        exifInfo->scene_capture_type = EXIF_SCENE_NIGHT;
        break;
    default:
        exifInfo->scene_capture_type = EXIF_SCENE_STANDARD;
        break;
    }

    switch (this->getShotMode()) {
    case SHOT_MODE_BEAUTY_FACE:
    case SHOT_MODE_BEST_FACE:
        exifInfo->scene_capture_type = EXIF_SCENE_PORTRAIT;
        break;
    default:
        break;
    }

    /* Image Unique ID */
    /* GPS Coordinates */
    double gpsLatitude = shot->ctl.jpeg.gpsCoordinates[0];
    double gpsLongitude = shot->ctl.jpeg.gpsCoordinates[1];
    double gpsAltitude = shot->ctl.jpeg.gpsCoordinates[2];
    if (gpsLatitude != 0 && gpsLongitude != 0) {
        if (gpsLatitude > 0)
            strncpy((char *) exifInfo->gps_latitude_ref, "N", 2);
        else
            strncpy((char *) exifInfo->gps_latitude_ref, "s", 2);

        if (gpsLongitude > 0)
            strncpy((char *) exifInfo->gps_longitude_ref, "E", 2);
        else
            strncpy((char *) exifInfo->gps_longitude_ref, "W", 2);

        if (gpsAltitude > 0)
            exifInfo->gps_altitude_ref = 0;
        else
            exifInfo->gps_altitude_ref = 1;

        gpsLatitude = fabs(gpsLatitude);
        gpsLongitude = fabs(gpsLongitude);
        gpsAltitude = fabs(gpsAltitude);

        exifInfo->gps_latitude[0].num = (uint32_t) gpsLatitude;
        exifInfo->gps_latitude[0].den = 1;
        exifInfo->gps_latitude[1].num = (uint32_t)((gpsLatitude - exifInfo->gps_latitude[0].num) * 60);
        exifInfo->gps_latitude[1].den = 1;
        exifInfo->gps_latitude[2].num = (uint32_t)(round((((gpsLatitude - exifInfo->gps_latitude[0].num) * 60)
                        - exifInfo->gps_latitude[1].num) * 60));
        exifInfo->gps_latitude[2].den = 1;

        exifInfo->gps_longitude[0].num = (uint32_t)gpsLongitude;
        exifInfo->gps_longitude[0].den = 1;
        exifInfo->gps_longitude[1].num = (uint32_t)((gpsLongitude - exifInfo->gps_longitude[0].num) * 60);
        exifInfo->gps_longitude[1].den = 1;
        exifInfo->gps_longitude[2].num = (uint32_t)(round((((gpsLongitude - exifInfo->gps_longitude[0].num) * 60)
                        - exifInfo->gps_longitude[1].num) * 60));
        exifInfo->gps_longitude[2].den = 1;

        exifInfo->gps_altitude.num = (uint32_t)gpsAltitude;
        exifInfo->gps_altitude.den = 1;

        struct tm tm_data;
        long gpsTimestamp = (long) shot->ctl.jpeg.gpsTimestamp;
        gmtime_r(&gpsTimestamp, &tm_data);
        exifInfo->gps_timestamp[0].num = tm_data.tm_hour;
        exifInfo->gps_timestamp[0].den = 1;
        exifInfo->gps_timestamp[1].num = tm_data.tm_min;
        exifInfo->gps_timestamp[1].den = 1;
        exifInfo->gps_timestamp[2].num = tm_data.tm_sec;
        exifInfo->gps_timestamp[2].den = 1;
        snprintf((char*)exifInfo->gps_datestamp, sizeof(exifInfo->gps_datestamp),
                "%04d:%02d:%02d", tm_data.tm_year + 1900, tm_data.tm_mon + 1, tm_data.tm_mday);

        exifInfo->enableGps = true;
    } else {
        exifInfo->enableGps = false;
    }

    /* Thumbnail Size */
    exifInfo->widthThumb = thumbnailRect->w;
    exifInfo->heightThumb = thumbnailRect->h;

    setMarkingOfExifFlash(0);
}

#ifdef USE_BINNING_MODE
status_t ExynosCamera1Parameters::needBinningMode(void)
{
    char cameraModeProperty[PROPERTY_VALUE_MAX];
    int ret = 0;

    if (m_staticInfo->vtcallSizeLutMax == 0 || m_staticInfo->vtcallSizeLut == NULL) {
       ALOGV("(%s):vtCallSizeLut is NULL, can't support the binnig mode", __FUNCTION__);
       return ret;
    }

    /* For VT Call with DualCamera Scenario */
    if (getDualMode()) {
        ALOGV("(%s):DualMode can't support the binnig mode.(%d,%d)", __FUNCTION__, getCameraId(), getDualMode());
        return ret;
    }

    if (getVtMode() > 0 && getVtMode() < 3) {
        ret = 1;
    } else {
        property_get("sys.cameramode.blackbox", cameraModeProperty, "0");
        if (strcmp(cameraModeProperty, "1") == 0) {
            ret = 1;
        } else {
            property_get("sys.hangouts.fps", cameraModeProperty, "0");
            int newHangOutFPS = atoi(cameraModeProperty);
            if (newHangOutFPS > 0) {
                ret = 1;
            } else {
                property_get("sys.cameramode.vtcall", cameraModeProperty, "0");
                if (strcmp(cameraModeProperty, "1") == 0) {
                    ret = 1;
                }
            }
        }
    }
    return ret;
}
#endif

status_t ExynosCamera1Parameters::checkShotMode(const CameraParameters& params)
{
    int newShotMode = params.getInt("shot-mode");
    int curShotMode = -1;
    char cameraModeProperty[PROPERTY_VALUE_MAX];

#ifdef USE_LIMITATION_FOR_THIRD_PARTY
    property_get("sys.cameramode.blackbox", cameraModeProperty, "0");
    if (strcmp(cameraModeProperty, "1") == 0) {
        newShotMode = THIRD_PARTY_BLACKBOX_MODE;
    } else {
        property_get("sys.hangouts.fps", cameraModeProperty, "0");
        int newHangOutFPS = atoi(cameraModeProperty);
        if (newHangOutFPS > 0) {
            newShotMode = THIRD_PARTY_HANGOUT_MODE;
        } else {
            int vtMode = params.getInt("vtmode");
            if ((vtMode <= 0) || (vtMode > 2)) {
                property_get("sys.cameramode.vtcall", cameraModeProperty, "0");
                if (strcmp(cameraModeProperty, "1") == 0) {
                    newShotMode = THIRD_PARTY_VTCALL_MODE;
                }
            }
        }
    }
#endif
    if (newShotMode < 0) {
        return NO_ERROR;
    }

    ALOGD("DEBUG(%s):newShotMode %d", "setParameters", newShotMode);

    curShotMode = getShotMode();

    if ((getRecordingHint() == true)
		&& (newShotMode != SHOT_MODE_SEQUENCE)
		) {
        m_setShotMode(SHOT_MODE_NORMAL);
        m_params.set("shot-mode", SHOT_MODE_NORMAL);
    } else if (curShotMode != newShotMode) {
        m_setShotMode(newShotMode);
        m_params.set("shot-mode", newShotMode);

        updatePreviewFpsRange();
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setShotMode(int shotMode)
{
    enum aa_mode mode = AA_CONTROL_AUTO;
    enum aa_scene_mode sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
    bool changeSceneMode = true;

    switch (shotMode) {
    case SHOT_MODE_DRAMA:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_DRAMA;
        break;
    case SHOT_MODE_3D_PANORAMA:
    case SHOT_MODE_PANORAMA:
    case SHOT_MODE_FRONT_PANORAMA:
    case SHOT_MODE_INTERACTIVE:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_PANORAMA;
        break;
    case SHOT_MODE_NIGHT:
    case SHOT_MODE_NIGHT_SCENE:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_LLS;
        break;
    case SHOT_MODE_ANIMATED_SCENE:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_ANIMATED;
        break;
    case SHOT_MODE_SPORTS:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_SPORTS;
        break;
    case SHOT_MODE_GOLF:
    case SHOT_MODE_SEQUENCE:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_GOLF;
        break;
    case SHOT_MODE_NORMAL:
    case SHOT_MODE_AUTO:
    case SHOT_MODE_BEAUTY_FACE:
    case SHOT_MODE_BEST_PHOTO:
    case SHOT_MODE_BEST_FACE:
    case SHOT_MODE_ERASER:
    case SHOT_MODE_RICH_TONE:
    case SHOT_MODE_STORY:
    case SHOT_MODE_SELFIE_ALARM:
    case SHOT_MODE_FASTMOTION:
        mode = AA_CONTROL_AUTO;
        sceneMode = AA_SCENE_MODE_FACE_PRIORITY;
        break;
    case SHOT_MODE_AQUA:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_AQUA;
        break;
    case SHOT_MODE_AUTO_PORTRAIT:
    case SHOT_MODE_PET:
#ifdef USE_LIMITATION_FOR_THIRD_PARTY
    case THIRD_PARTY_BLACKBOX_MODE:
    case THIRD_PARTY_VTCALL_MODE:
#endif
    default:
        changeSceneMode = false;
        break;
    }

    m_cameraInfo.shotMode = shotMode;
    if (changeSceneMode == true)
        setMetaCtlSceneMode(&m_metadata, mode, sceneMode);
}

int ExynosCamera1Parameters::getShotMode(void)
{
    return m_cameraInfo.shotMode;
}

status_t ExynosCamera1Parameters::checkVtMode(const CameraParameters& params)
{
    int newVTMode = params.getInt("vtmode");
    int curVTMode = -1;

    CLOGD("DEBUG(%s):newVTMode %d", "setParameters", newVTMode);

    /*
     * VT mode
     *   1: 3G vtmode (176x144, Fixed 7fps)
     *   2: LTE or WIFI vtmode (640x480, Fixed 15fps)
     */
    if (newVTMode < 0 || newVTMode > 2) {
        newVTMode = 0;
    }

    curVTMode = getVtMode();

    if (curVTMode != newVTMode) {
        m_setVtMode(newVTMode);
        m_params.set("vtmode", newVTMode);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setVtMode(int vtMode)
{
    m_cameraInfo.vtMode = vtMode;
}

int ExynosCamera1Parameters::getVtMode(void)
{
    return m_cameraInfo.vtMode;
}

status_t ExynosCamera1Parameters::checkGamma(const CameraParameters& params)
{
    bool newGamma = false;
    bool curGamma = false;
    const char *strNewGamma = params.get("video_recording_gamma");

    if (strNewGamma == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):strNewGamma %s", "setParameters", strNewGamma);

    if (!strcmp(strNewGamma, "off")) {
        newGamma = false;
    } else if (!strcmp(strNewGamma, "on")) {
        newGamma = true;
    } else {
        CLOGE("ERR(%s):unmatched gamma(%s)", __FUNCTION__, strNewGamma);
        return BAD_VALUE;
    }

    curGamma = getGamma();

    if (curGamma != newGamma) {
        m_setGamma(newGamma);
        m_params.set("video_recording_gamma", strNewGamma);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setGamma(bool gamma)
{
    m_cameraInfo.gamma = gamma;
}

bool ExynosCamera1Parameters::getGamma(void)
{
    return m_cameraInfo.gamma;
}

status_t ExynosCamera1Parameters::checkSlowAe(const CameraParameters& params)
{
    bool newSlowAe = false;
    bool curSlowAe = false;
    const char *strNewSlowAe = params.get("slow_ae");

    if (strNewSlowAe == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):strNewSlowAe %s", "setParameters", strNewSlowAe);

    if (!strcmp(strNewSlowAe, "off"))
        newSlowAe = false;
    else if (!strcmp(strNewSlowAe, "on"))
        newSlowAe = true;
    else {
        CLOGE("ERR(%s):unmatched slow_ae(%s)", __FUNCTION__, strNewSlowAe);
        return BAD_VALUE;
    }

    curSlowAe = getSlowAe();

    if (curSlowAe != newSlowAe) {
        m_setSlowAe(newSlowAe);
        m_params.set("slow_ae", strNewSlowAe);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setSlowAe(bool slowAe)
{
    m_cameraInfo.slowAe = slowAe;
}

bool ExynosCamera1Parameters::getSlowAe(void)
{
    return m_cameraInfo.slowAe;
}

status_t ExynosCamera1Parameters::checkImageUniqueId(__unused const CameraParameters& params)
{
    const char *strCurImageUniqueId = m_params.get("imageuniqueid-value");
    const char *strNewImageUniqueId = NULL;

    if(strCurImageUniqueId == NULL || strcmp(strCurImageUniqueId, "") == 0 || strcmp(strCurImageUniqueId, "0") == 0) {
        strNewImageUniqueId = getImageUniqueId();

        if (strNewImageUniqueId != NULL && strcmp(strNewImageUniqueId, "") != 0) {
            ALOGD("DEBUG(%s):newImageUniqueId %s ", "setParameters", strNewImageUniqueId );
            m_params.set("imageuniqueid-value", strNewImageUniqueId);
        }
    }
    return NO_ERROR;
}

const char *ExynosCamera1Parameters::getImageUniqueId(void)
{
    return NULL;
}

#ifdef BURST_CAPTURE
status_t ExynosCamera1Parameters::checkSeriesShotFilePath(const CameraParameters& params)
{
    const char *seriesShotFilePath = params.get("capture-burst-filepath");

    if (seriesShotFilePath != NULL) {
        snprintf(m_seriesShotFilePath, sizeof(m_seriesShotFilePath), "%s", seriesShotFilePath);
        CLOGD("DEBUG(%s): seriesShotFilePath %s", "setParameters", seriesShotFilePath);
        m_params.set("capture-burst-filepath", seriesShotFilePath);
    } else {
        CLOGD("DEBUG(%s): seriesShotFilePath NULL", "setParameters");
        memset(m_seriesShotFilePath, 0, 100);
    }

    return NO_ERROR;
}
#endif

status_t ExynosCamera1Parameters::checkSeriesShotMode(const CameraParameters& params)
{
    int burstCount = params.getInt("burst-capture");
    int bestCount = params.getInt("best-capture");

    CLOGD("DEBUG(%s): burstCount(%d), bestCount(%d)", "setParameters", burstCount, bestCount);

    if (burstCount < 0 || bestCount < 0) {
        CLOGE("ERR(%s[%d]): Invalid burst-capture count(%d), best-capture count(%d)", __FUNCTION__, __LINE__, burstCount, bestCount);
        return BAD_VALUE;
    }

    /* TODO: select shot count */
    if (bestCount > burstCount) {
        m_setSeriesShotCount(bestCount);
        m_params.set("burst-capture", 0);
        m_params.set("best-capture", bestCount);
    } else {
        m_setSeriesShotCount(burstCount);
        m_params.set("burst-capture", burstCount);
        m_params.set("best-capture", 0);
    }
    return NO_ERROR;
}
#ifdef BURST_CAPTURE
int ExynosCamera1Parameters::getSeriesShotSaveLocation(void)
{
    int seriesShotSaveLocation = m_seriesShotSaveLocation;
    int shotMode = getShotMode();

    /* GED's series shot work as callback */
    seriesShotSaveLocation = BURST_SAVE_CALLBACK;

    return seriesShotSaveLocation;
}

void ExynosCamera1Parameters::setSeriesShotSaveLocation(int ssaveLocation)
{
    m_seriesShotSaveLocation = ssaveLocation;
}

char *ExynosCamera1Parameters::getSeriesShotFilePath(void)
{
    return m_seriesShotFilePath;
}
#endif

int ExynosCamera1Parameters::getSeriesShotDuration(void)
{
    return NORMAL_BURST_DURATION;
}

int ExynosCamera1Parameters::getSeriesShotMode(void)
{
    return m_cameraInfo.seriesShotMode;
}

void ExynosCamera1Parameters::setSeriesShotMode(int sshotMode, int count)
{
    int sshotCount = 0;
    int shotMode = getShotMode();
    if (sshotMode == SERIES_SHOT_MODE_BURST) {
        if (shotMode == SHOT_MODE_BEST_PHOTO) {
            sshotMode = SERIES_SHOT_MODE_BEST_PHOTO;
            sshotCount = 8;
        } else if (shotMode == SHOT_MODE_BEST_FACE) {
            sshotMode = SERIES_SHOT_MODE_BEST_FACE;
            sshotCount = 5;
        } else if (shotMode == SHOT_MODE_ERASER) {
            sshotMode = SERIES_SHOT_MODE_ERASER;
            sshotCount = 5;
        } else {
            sshotMode = SERIES_SHOT_MODE_BURST;
            sshotCount = MAX_SERIES_SHOT_COUNT;
        }
    } else if (sshotMode == SERIES_SHOT_MODE_LLS ||
            sshotMode == SERIES_SHOT_MODE_SIS) {
            if(count > 0) {
                sshotCount = count;
            } else {
                sshotCount = 5;
            }
    }

    CLOGD("DEBUG(%s[%d]: set shotmode(%d), shotCount(%d)", __FUNCTION__, __LINE__, sshotMode, sshotCount);

    m_cameraInfo.seriesShotMode = sshotMode;
    m_setSeriesShotCount(sshotCount);
}

void ExynosCamera1Parameters::m_setSeriesShotCount(int seriesShotCount)
{
    m_cameraInfo.seriesShotCount = seriesShotCount;
}

int ExynosCamera1Parameters::getSeriesShotCount(void)
{
    return m_cameraInfo.seriesShotCount;
}

bool ExynosCamera1Parameters::getSamsungCamera(void)
{
    return false;
}

void ExynosCamera1Parameters::m_initMetadata(void)
{
    memset(&m_metadata, 0x00, sizeof(struct camera2_shot_ext));
    struct camera2_shot *shot = &m_metadata.shot;

    // 1. ctl
    // request
    shot->ctl.request.id = 0;
    shot->ctl.request.metadataMode = METADATA_MODE_FULL;
    shot->ctl.request.frameCount = 0;

    // lens
    shot->ctl.lens.focusDistance = -1.0f;
    if (getHalVersion() == IS_HAL_VER_3_2) {
        shot->ctl.lens.aperture = m_staticInfo->aperture;
        shot->ctl.lens.focalLength = m_staticInfo->focalLength;
    } else {
        shot->ctl.lens.aperture = (float)m_staticInfo->apertureNum / (float)m_staticInfo->apertureDen;
        shot->ctl.lens.focalLength = (float)m_staticInfo->focalLengthNum / (float)m_staticInfo->focalLengthDen;
    }
    shot->ctl.lens.filterDensity = 0.0f;
    shot->ctl.lens.opticalStabilizationMode = ::OPTICAL_STABILIZATION_MODE_OFF;

    int minFps = (m_staticInfo->minFps == 0) ? 0 : (m_staticInfo->maxFps / 2);
    int maxFps = (m_staticInfo->maxFps == 0) ? 0 : m_staticInfo->maxFps;

    /* The min fps can not be '0'. Therefore it is set up default value '15'. */
    if (minFps == 0) {
        CLOGW("WRN(%s): Invalid min fps value(%d)", __FUNCTION__, minFps);
        minFps = 15;
    }

    /*  The initial fps can not be '0' and bigger than '30'. Therefore it is set up default value '30'. */
    if (maxFps == 0 || 30 < maxFps) {
        CLOGW("WRN(%s): Invalid max fps value(%d)", __FUNCTION__, maxFps);
        maxFps = 30;
    }

    /* sensor */
    shot->ctl.sensor.exposureTime = 0;
    shot->ctl.sensor.frameDuration = (1000 * 1000 * 1000) / maxFps;
    shot->ctl.sensor.sensitivity = 0;

    /* flash */
    shot->ctl.flash.flashMode = ::CAM2_FLASH_MODE_OFF;
    shot->ctl.flash.firingPower = 0;
    shot->ctl.flash.firingTime = 0;

    /* hotpixel */
    shot->ctl.hotpixel.mode = (enum processing_mode)0;

    /* demosaic */
    shot->ctl.demosaic.mode = (enum demosaic_processing_mode)0;

    /* noise */
#ifdef USE_NEW_NOISE_REDUCTION_ALGORITHM
    shot->ctl.noise.mode = ::PROCESSING_MODE_FAST;
#else
    shot->ctl.noise.mode = ::PROCESSING_MODE_OFF;
#endif
    shot->ctl.noise.strength = 5;

    /* shading */
    shot->ctl.shading.mode = (enum processing_mode)0;

    /* color */
    shot->ctl.color.mode = ::COLORCORRECTION_MODE_FAST;
    static const float colorTransform_hal3[9] = {
        1.0f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f
    };
    static const struct rational colorTransform[9] = {
        {1, 0}, {0, 0}, {0, 0},
        {0, 0}, {1, 0}, {0, 0},
        {0, 0}, {0, 0}, {1, 0}
    };

    if (getHalVersion() == IS_HAL_VER_3_2) {
        for (size_t i = 0; i < sizeof(colorTransform)/sizeof(colorTransform[0]); i++) {
            shot->ctl.color.transform[i].num = colorTransform_hal3[i] * COMMON_DENOMINATOR;
            shot->ctl.color.transform[i].den = COMMON_DENOMINATOR;
        }
    } else {
        memcpy(shot->ctl.color.transform, colorTransform, sizeof(shot->ctl.color.transform));
    }

    /* tonemap */
    shot->ctl.tonemap.mode = ::TONEMAP_MODE_FAST;
    static const float tonemapCurve[4] = {
        0.f, 0.f,
        1.f, 1.f
    };

    int tonemapCurveSize = sizeof(tonemapCurve);
    int sizeOfCurve = sizeof(shot->ctl.tonemap.curveRed) / sizeof(shot->ctl.tonemap.curveRed[0]);

    for (int i = 0; i < sizeOfCurve; i ++) {
        memcpy(&(shot->ctl.tonemap.curveRed[i]),   tonemapCurve, tonemapCurveSize);
        memcpy(&(shot->ctl.tonemap.curveGreen[i]), tonemapCurve, tonemapCurveSize);
        memcpy(&(shot->ctl.tonemap.curveBlue[i]),  tonemapCurve, tonemapCurveSize);
    }

    /* edge */
#ifdef USE_NEW_NOISE_REDUCTION_ALGORITHM
    shot->ctl.edge.mode = ::PROCESSING_MODE_FAST;
#else
    shot->ctl.edge.mode = ::PROCESSING_MODE_OFF;
#endif
    shot->ctl.edge.strength = 5;

    /* scaler
     * Max Picture Size == Max Sensor Size - Sensor Margin
     */
    if (m_setParamCropRegion(0,
                             m_staticInfo->maxPictureW, m_staticInfo->maxPictureH,
                             m_staticInfo->maxPreviewW, m_staticInfo->maxPreviewH
                             ) != NO_ERROR) {
        ALOGE("ERR(%s):m_setZoom() fail", __FUNCTION__);
    }

    /* jpeg */
    shot->ctl.jpeg.quality = 96;
    shot->ctl.jpeg.thumbnailSize[0] = m_staticInfo->maxThumbnailW;
    shot->ctl.jpeg.thumbnailSize[1] = m_staticInfo->maxThumbnailH;
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
    shot->ctl.aa.videoStabilizationMode =
        (enum aa_videostabilization_mode) VIDEO_STABILIZATION_MODE_OFF ;

    /* default metering is center */
    shot->ctl.aa.aeMode = ::AA_AEMODE_CENTER;
    shot->ctl.aa.aeRegions[0] = 0;
    shot->ctl.aa.aeRegions[1] = 0;
    shot->ctl.aa.aeRegions[2] = 0;
    shot->ctl.aa.aeRegions[3] = 0;
    shot->ctl.aa.aeRegions[4] = 1000;
    shot->ctl.aa.aeLock = ::AA_AE_LOCK_OFF;
#if defined(USE_SUBDIVIDED_EV)
    shot->ctl.aa.aeExpCompensation = 0; /* 21 is middle */
#else
    shot->ctl.aa.aeExpCompensation = 5; /* 5 is middle */
#endif
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
    shot->ctl.aa.afTrigger = (enum aa_af_trigger) AA_AF_TRIGGER_IDLE;
    shot->ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
    shot->ctl.aa.vendor_isoValue = 0;

    /* 2. dm */

    /* 3. utrl */
#ifdef USE_FW_ZOOMRATIO
    m_metadata.shot.uctl.zoomRatio = 1.00f;
#else
    m_metadata.shot.uctl.zoomRatio = 0;
#endif

    /* 4. udm */

    /* 5. magicNumber */
    shot->magicNumber = SHOT_MAGIC_NUMBER;

    setMetaSetfile(&m_metadata, 0x0);

    /* user request */
    m_metadata.drc_bypass = 1;
    m_metadata.dis_bypass = 1;
    m_metadata.dnr_bypass = 1;
    m_metadata.fd_bypass  = 1;
}

status_t ExynosCamera1Parameters::duplicateCtrlMetadata(void *buf)
{
    if (buf == NULL) {
        CLOGE("ERR: buf is NULL");
        return BAD_VALUE;
    }

    struct camera2_shot_ext *meta_shot_ext = (struct camera2_shot_ext *)buf;
    memcpy(&meta_shot_ext->shot.ctl, &m_metadata.shot.ctl, sizeof(struct camera2_ctl));

#ifdef USE_FW_ZOOMRATIO
    if(getCameraId() == CAMERA_ID_BACK) {
        meta_shot_ext->shot.uctl.zoomRatio = m_metadata.shot.uctl.zoomRatio;
    }
#endif

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_getSetfileYuvRange(bool flagReprocessing, int *setfile, int *yuvRange)
{
    uint32_t currentSetfile = 0;
    uint32_t stateReg = 0;
    int flagYUVRange = YUV_FULL_RANGE;

    unsigned int minFps = 0;
    unsigned int maxFps = 0;
    getPreviewFpsRange(&minFps, &maxFps);

    if (getRecordingHint() == true) {
        stateReg |= STATE_REG_RECORDINGHINT;
    }

    if (m_isUHDRecordingMode() == true)
        stateReg |= STATE_REG_UHD_RECORDING;

    if (getDualMode() == true) {
        stateReg |= STATE_REG_DUAL_MODE;
        if (getDualRecordingHint() == true)
            stateReg |= STATE_REG_DUAL_RECORDINGHINT;
    } else {
        if (getRecordingHint() == true)
            stateReg |= STATE_REG_RECORDINGHINT;
    }

    if (flagReprocessing == true)
        stateReg |= STATE_REG_FLAG_REPROCESSING;

    if ((stateReg & STATE_REG_RECORDINGHINT)||
        (stateReg & STATE_REG_UHD_RECORDING)||
        (stateReg & STATE_REG_DUAL_RECORDINGHINT)) {
        if (flagReprocessing == false) {
            flagYUVRange = YUV_LIMITED_RANGE;
        }
    }

    if (m_cameraId == CAMERA_ID_FRONT) {
        int vtMode = getVtMode();

        if (0 < vtMode) {
            switch (vtMode) {
            case 1:
                currentSetfile = ISS_SUB_SCENARIO_FRONT_VT1;
                break;
            case 2:
            default:
                currentSetfile = ISS_SUB_SCENARIO_FRONT_VT2;
                break;
            }
        } else if (getIntelligentMode() == 1) {
            currentSetfile = ISS_SUB_SCENARIO_FRONT_SMART_STAY;
        } else if (getShotMode() == SHOT_MODE_FRONT_PANORAMA) {
            currentSetfile = ISS_SUB_SCENARIO_FRONT_PANORAMA;
        } else {
            switch(stateReg) {
                case STATE_STILL_PREVIEW:
                case STATE_UHD_PREVIEW:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW;
                    break;

                case STATE_VIDEO:
                case STATE_UHD_VIDEO:
                    currentSetfile = ISS_SUB_SCENARIO_VIDEO;
                    break;

                case STATE_DUAL_STILL_PREVIEW:
                    currentSetfile = ISS_SUB_SCENARIO_DUAL_STILL;
                    break;

                case STATE_DUAL_VIDEO:
                    currentSetfile = ISS_SUB_SCENARIO_DUAL_VIDEO;
                    break;

                default:
                    ALOGD("(%s)can't define senario of setfile.(0x%4x)",__func__, stateReg);
                    break;
            }
        }
    } else {
        switch(stateReg) {
            case STATE_STILL_PREVIEW:
                currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW;
                break;

            case STATE_STILL_CAPTURE:
            case STATE_VIDEO_CAPTURE:
            case STATE_DUAL_STILL_CAPTURE:
            case STATE_DUAL_VIDEO_CAPTURE:
            case STATE_UHD_PREVIEW_CAPTURE:
            case STATE_UHD_VIDEO_CAPTURE:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE;
                break;

            case STATE_VIDEO:
                if (30 < minFps && 30 < maxFps) {
                    if (300 == minFps && 300 == maxFps) {
                        currentSetfile = ISS_SUB_SCENARIO_WVGA_300FPS;
                    } else if (60 == minFps && 60 == maxFps) {
                        currentSetfile = ISS_SUB_SCENARIO_FHD_60FPS;
                    } else {
                        currentSetfile = ISS_SUB_SCENARIO_VIDEO_HIGH_SPEED;
                    }
                } else {
                    currentSetfile = ISS_SUB_SCENARIO_VIDEO;
                }
                break;

            case STATE_DUAL_VIDEO:
                currentSetfile = ISS_SUB_SCENARIO_DUAL_VIDEO;
                break;

            case STATE_DUAL_STILL_PREVIEW:
                currentSetfile = ISS_SUB_SCENARIO_DUAL_STILL;
                break;

            case STATE_UHD_PREVIEW:
            case STATE_UHD_VIDEO:
#if 1 /* HACK: enable when FW ready */
                currentSetfile = ISS_SUB_SCENARIO_UHD_30FPS;
#else
                currentSetfile = ISS_SUB_SCENARIO_VIDEO;
#endif
                break;

            default:
                ALOGD("(%s)can't define senario of setfile.(0x%4x)",__func__, stateReg);
                break;
        }
    }
#if 0
    ALOGD("(%s)[%d] : ===============================================================================",__func__, __LINE__);
    ALOGD("(%s)[%d] : CurrentState(0x%4x)",__func__, __LINE__, stateReg);
    ALOGD("(%s)[%d] : getRTHdr()(%d)",__func__, __LINE__, getRTHdr());
    ALOGD("(%s)[%d] : getRecordingHint()(%d)",__func__, __LINE__, getRecordingHint());
    ALOGD("(%s)[%d] : m_isUHDRecordingMode()(%d)",__func__, __LINE__, m_isUHDRecordingMode());
    ALOGD("(%s)[%d] : getDualMode()(%d)",__func__, __LINE__, getDualMode());
    ALOGD("(%s)[%d] : getDualRecordingHint()(%d)",__func__, __LINE__, getDualRecordingHint());
    ALOGD("(%s)[%d] : flagReprocessing(%d)",__func__, __LINE__, flagReprocessing);
    ALOGD("(%s)[%d] : ===============================================================================",__func__, __LINE__);
    ALOGD("(%s)[%d] : currentSetfile(%d)",__func__, __LINE__, currentSetfile);
    ALOGD("(%s)[%d] : flagYUVRange(%d)",__func__, __LINE__, flagYUVRange);
    ALOGD("(%s)[%d] : ===============================================================================",__func__, __LINE__);
#else
    ALOGD("(%s)[%d] : CurrentState (0x%4x), currentSetfile(%d)",__func__, __LINE__, stateReg, currentSetfile);
#endif

done:
    *setfile = currentSetfile;
    *yuvRange = flagYUVRange;
}

int *ExynosCamera1Parameters::getHighSpeedSizeTable(int fpsMode) {
    return getHighSpeedSizeTable(fpsMode);
}

status_t ExynosCamera1Parameters::calcPreviewGSCRect(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;

    int previewW = 0, previewH = 0, previewFormat = 0;
    int hwPreviewW = 0, hwPreviewH = 0, hwPreviewFormat = 0;
    previewFormat = getPreviewFormat();
    hwPreviewFormat = getHwPreviewFormat();

    getHwPreviewSize(&hwPreviewW, &hwPreviewH);
    getPreviewSize(&previewW, &previewH);

    ret = getCropRectAlign(hwPreviewW, hwPreviewH,
                           previewW, previewH,
                           &cropX, &cropY,
                           &cropW, &cropH,
                           CAMERA_MAGIC_ALIGN, 2,
                           0, 1);

    srcRect->x = cropX;
    srcRect->y = cropY;
    srcRect->w = cropW;
    srcRect->h = cropH;
    srcRect->fullW = cropW;
    srcRect->fullH = cropH;
    srcRect->colorFormat = hwPreviewFormat;

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = previewW;
    dstRect->h = previewH;
    dstRect->fullW = previewW;
    dstRect->fullH = previewH;
    dstRect->colorFormat = previewFormat;

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::calcRecordingGSCRect(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;

    int hwPreviewW = 0, hwPreviewH = 0, hwPreviewFormat = 0;
    int videoW = 0, videoH = 0, videoFormat = 0;
    float zoomRatio = getZoomRatio(0) / 1000;

    hwPreviewFormat = getHwPreviewFormat();
    videoFormat     = getVideoFormat();

    getHwPreviewSize(&hwPreviewW, &hwPreviewH);
    getVideoSize(&videoW, &videoH);

    if (SIZE_RATIO(hwPreviewW, hwPreviewH) == SIZE_RATIO(videoW, videoH)) {
        cropW = hwPreviewW;
        cropH = hwPreviewH;
    } else  {
        ret = getCropRectAlign(hwPreviewW, hwPreviewH,
                         videoW, videoH,
                         &cropX, &cropY,
                         &cropW, &cropH,
                         2, 2,
                         0, zoomRatio);
    }

    srcRect->x = cropX;
    srcRect->y = cropY;
    srcRect->w = cropW;
    srcRect->h = cropH;
    srcRect->fullW = hwPreviewW;
    srcRect->fullH = hwPreviewH;
    srcRect->colorFormat = hwPreviewFormat;

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = videoW;
    dstRect->h = videoH;
    dstRect->fullW = videoW;
    dstRect->fullH = videoH;
    dstRect->colorFormat = videoFormat;

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::calcPreviewBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int hwSensorW = 0, hwSensorH = 0;
    int hwPictureW = 0, hwPictureH = 0;
    int pictureW = 0, pictureH = 0;
    int previewW = 0, previewH = 0;
    int hwSensorMarginW = 0, hwSensorMarginH = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
#if 0
    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;
    int pictureFormat = 0, hwPictureFormat = 0;
#endif
    int zoomLevel = 0;
    int maxZoomRatio = 0;
    int bayerFormat = CAMERA_BAYER_FORMAT;
    float zoomRatio = getZoomRatio(0) / 1000;

#ifdef DEBUG_RAWDUMP
    if (checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    /* TODO: check state ready for start */
#if 0
    pictureFormat = getHwPictureFormat();
#endif
    zoomLevel = getZoomLevel();
    maxZoomRatio = getMaxZoomRatio() / 1000;
    getHwPictureSize(&hwPictureW, &hwPictureH);
    getPictureSize(&pictureW, &pictureH);

    getHwSensorSize(&hwSensorW, &hwSensorH);
    getPreviewSize(&previewW, &previewH);
    getSensorMargin(&hwSensorMarginW, &hwSensorMarginH);
    m_adjustSensorMargin(&hwSensorMarginW, &hwSensorMarginH);

    zoomRatio = getZoomRatio(zoomLevel) / 1000;

    hwSensorW -= hwSensorMarginW;
    hwSensorH -= hwSensorMarginH;

    if (getHalVersion() == IS_HAL_VER_3_2) {
        int cropRegionX = 0, cropRegionY = 0, cropRegionW = 0, cropRegionH = 0;
        int maxSensorW = 0, maxSensorH = 0;
        float scaleRatioX = 0.0f, scaleRatioY = 0.0f;

        m_getCropRegion(&cropRegionX, &cropRegionY, &cropRegionW, &cropRegionH);
        getMaxSensorSize(&maxSensorW, &maxSensorH);

        /* 1. Scale down the crop region to adjust with the bcrop input size */
        scaleRatioX = (float) hwSensorW / (float) maxSensorW;
        scaleRatioY = (float) hwSensorH / (float) maxSensorH;
        cropRegionX = (int) (cropRegionX * scaleRatioX);
        cropRegionY = (int) (cropRegionY * scaleRatioY);
        cropRegionW = (int) (cropRegionW * scaleRatioX);
        cropRegionH = (int) (cropRegionH * scaleRatioY);

        if (cropRegionW < 1 || cropRegionH < 1) {
            cropRegionW = hwSensorW;
            cropRegionH = hwSensorH;
        }

        /* 2. Calculate the real crop region with considering the target ratio */
        ret = getCropRectAlign(cropRegionW, cropRegionH,
                previewW, previewH,
                &cropX, &cropY,
                &cropW, &cropH,
                CAMERA_BCROP_ALIGN, 2,
                0, 0.0f);

        cropX = ALIGN_DOWN((cropRegionX + cropX), 2);
        cropY = ALIGN_DOWN((cropRegionY + cropY), 2);
    } else {
        ret = getCropRectAlign(hwSensorW, hwSensorH,
                previewW, previewH,
                &cropX, &cropY,
                &cropW, &cropH,
                CAMERA_BCROP_ALIGN, 2,
                zoomLevel, zoomRatio);

        cropX = ALIGN_DOWN(cropX, 2);
        cropY = ALIGN_DOWN(cropY, 2);
        cropW = ALIGN_UP(hwSensorW - (cropX * 2), CAMERA_BCROP_ALIGN);
        cropH = hwSensorH - (cropY * 2);
    }

    if (getUsePureBayerReprocessing() == false) {
        int pictureCropX = 0, pictureCropY = 0;
        int pictureCropW = 0, pictureCropH = 0;

        zoomLevel = 0;
        zoomRatio = getZoomRatio(zoomLevel) / 1000;

        ret = getCropRectAlign(cropW, cropH,
                pictureW, pictureH,
                &pictureCropX, &pictureCropY,
                &pictureCropW, &pictureCropH,
                CAMERA_BCROP_ALIGN, 2,
                zoomLevel, zoomRatio);

        pictureCropX = ALIGN_DOWN(pictureCropX, 2);
        pictureCropY = ALIGN_DOWN(pictureCropY, 2);
        pictureCropW = cropW - (pictureCropX * 2);
        pictureCropH = cropH - (pictureCropY * 2);

        if (pictureCropW < pictureW / maxZoomRatio || pictureCropH < pictureH / maxZoomRatio) {
            ALOGW("WRN(%s[%d]): zoom ratio is upto x%d, crop(%dx%d), picture(%dx%d)", __FUNCTION__, __LINE__, maxZoomRatio, cropW, cropH, pictureW, pictureH);
            float src_ratio = 1.0f;
            float dst_ratio = 1.0f;
            /* ex : 1024 / 768 */
            src_ratio = ROUND_OFF_HALF(((float)cropW / (float)cropH), 2);
            /* ex : 352  / 288 */
            dst_ratio = ROUND_OFF_HALF(((float)pictureW / (float)pictureH), 2);

            if (dst_ratio <= src_ratio) {
                /* shrink w */
                cropX = ALIGN_DOWN(((int)(hwSensorW - ((pictureH / maxZoomRatio) * src_ratio)) >> 1), 2);
                cropY = ALIGN_DOWN(((hwSensorH - (pictureH / maxZoomRatio)) >> 1), 2);
            } else {
                /* shrink h */
                cropX = ALIGN_DOWN(((hwSensorW - (pictureW / maxZoomRatio)) >> 1), 2);
                cropY = ALIGN_DOWN(((int)(hwSensorH - ((pictureW / maxZoomRatio) / src_ratio)) >> 1), 2);
            }
            cropW = ALIGN_UP(hwSensorW - (cropX * 2), CAMERA_BCROP_ALIGN);
            cropH = hwSensorH - (cropY * 2);
        }
    }

#if 0
    ALOGD("DEBUG(%s):hwSensorSize (%dx%d), previewSize (%dx%d)",
            __FUNCTION__, hwSensorW, hwSensorH, previewW, previewH);
    ALOGD("DEBUG(%s):hwPictureSize (%dx%d), pictureSize (%dx%d)",
            __FUNCTION__, hwPictureW, hwPictureH, pictureW, pictureH);
    ALOGD("DEBUG(%s):size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, cropX, cropY, cropW, cropH, zoomLevel);
    ALOGD("DEBUG(%s):size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    ALOGD("DEBUG(%s):size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            __FUNCTION__, pictureFormat, JPEG_INPUT_COLOR_FMT);
#endif

    srcRect->x = 0;
    srcRect->y = 0;
    srcRect->w = hwSensorW;
    srcRect->h = hwSensorH;
    srcRect->fullW = hwSensorW;
    srcRect->fullH = hwSensorH;
    srcRect->colorFormat = bayerFormat;

    dstRect->x = cropX;
    dstRect->y = cropY;
    dstRect->w = cropW;
    dstRect->h = cropH;
    dstRect->fullW = cropW;
    dstRect->fullH = cropH;
    dstRect->colorFormat = bayerFormat;

    m_setHwBayerCropRegion(dstRect->w, dstRect->h, dstRect->x, dstRect->y);
    return NO_ERROR;
}

status_t ExynosCamera1Parameters::calcPreviewDzoomCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int previewW = 0, previewH = 0;
    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;

    int zoomLevel = 0;
    int maxZoomRatio = 0;
    float zoomRatio = getZoomRatio(0) / 1000;

    /* TODO: check state ready for start */
    zoomLevel = getZoomLevel();
    maxZoomRatio = getMaxZoomRatio() / 1000;
    getHwPreviewSize(&previewW, &previewH);
    zoomRatio = getZoomRatio(zoomLevel) / 1000;

    ret = getCropRectAlign(srcRect->w, srcRect->h,
                     previewW, previewH,
                     &srcRect->x, &srcRect->y,
                     &srcRect->w, &srcRect->h,
                     2, 2,
                     zoomLevel, zoomRatio);
    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = previewW;
    dstRect->h = previewH;

    ALOGV("INFO(%s[%d]):SRC cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d ratio = %f", __FUNCTION__, __LINE__, srcRect->x, srcRect->y, srcRect->w, srcRect->h, zoomLevel, zoomRatio);
    ALOGV("INFO(%s[%d]):DST cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d ratio = %f", __FUNCTION__, __LINE__, dstRect->x, dstRect->y, dstRect->w, dstRect->h, zoomLevel, zoomRatio);

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::getPictureBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int hwBnsW   = 0;
    int hwBnsH   = 0;
    int hwBcropW = 0;
    int hwBcropH = 0;
    int zoomLevel = 0;
    float zoomRatio = 1.00f;
    int hwSensorMarginW = 0;
    int hwSensorMarginH = 0;

    /* matched ratio LUT is not existed, use equation */
    if (m_useSizeTable == false
        || m_staticInfo->pictureSizeLut == NULL
        || m_staticInfo->pictureSizeLutMax <= m_cameraInfo.pictureSizeRatioId
        || m_cameraInfo.pictureSizeRatioId != m_cameraInfo.previewSizeRatioId)
        return calcPictureBayerCropSize(srcRect, dstRect);

    /* use LUT */
    hwBnsW   = m_staticInfo->pictureSizeLut[m_cameraInfo.pictureSizeRatioId][BNS_W];
    hwBnsH   = m_staticInfo->pictureSizeLut[m_cameraInfo.pictureSizeRatioId][BNS_H];
    hwBcropW = m_staticInfo->pictureSizeLut[m_cameraInfo.pictureSizeRatioId][BCROP_W];
    hwBcropH = m_staticInfo->pictureSizeLut[m_cameraInfo.pictureSizeRatioId][BCROP_H];

    if (getHalVersion() != IS_HAL_VER_3_2) {
        zoomLevel = getZoomLevel();
        zoomRatio = getZoomRatio(zoomLevel) / 1000;

#if defined(SCALER_MAX_SCALE_UP_RATIO)
        /*
         * After dividing float & casting int,
         * zoomed size can be smaller too much.
         * so, when zoom until max, ceil up about floating point.
         */
        if (ALIGN_UP((int)((float)hwBcropW / zoomRatio), CAMERA_BCROP_ALIGN) * SCALER_MAX_SCALE_UP_RATIO < hwBcropW ||
                ALIGN_UP((int)((float)hwBcropH / zoomRatio), 2) * SCALER_MAX_SCALE_UP_RATIO < hwBcropH) {
            hwBcropW = ALIGN_UP((int)ceil((float)hwBcropW / zoomRatio), CAMERA_BCROP_ALIGN);
            hwBcropH = ALIGN_UP((int)ceil((float)hwBcropH / zoomRatio), 2);
        } else
#endif
        {
            hwBcropW = ALIGN_UP((int)((float)hwBcropW / zoomRatio), CAMERA_BCROP_ALIGN);
            hwBcropH = ALIGN_UP((int)((float)hwBcropH / zoomRatio), 2);
        }
    }

    /* Re-calculate the BNS size for removing Sensor Margin.
       On Capture Stream(3AA_M2M_Input), the BNS is not used.
       So, the BNS ratio is not needed to be considered for sensor margin */
    getSensorMargin(&hwSensorMarginW, &hwSensorMarginH);
    hwBnsW = hwBnsW - hwSensorMarginW;
    hwBnsH = hwBnsH - hwSensorMarginH;

    /* src */
    srcRect->x = 0;
    srcRect->y = 0;
    srcRect->w = hwBnsW;
    srcRect->h = hwBnsH;

    if (getHalVersion() == IS_HAL_VER_3_2) {
        int cropRegionX = 0, cropRegionY = 0, cropRegionW = 0, cropRegionH = 0;
        int maxSensorW = 0, maxSensorH = 0;
        float scaleRatioX = 0.0f, scaleRatioY = 0.0f;
        status_t ret = NO_ERROR;

        m_getCropRegion(&cropRegionX, &cropRegionY, &cropRegionW, &cropRegionH);
        getMaxSensorSize(&maxSensorW, &maxSensorH);

        /* 1. Scale down the crop region to adjust with the bcrop input size */
        scaleRatioX = (float) hwBnsW / (float) maxSensorW;
        scaleRatioY = (float) hwBnsH / (float) maxSensorH;
        cropRegionX = (int) (cropRegionX * scaleRatioX);
        cropRegionY = (int) (cropRegionY * scaleRatioY);
        cropRegionW = (int) (cropRegionW * scaleRatioX);
        cropRegionH = (int) (cropRegionH * scaleRatioY);

        if (cropRegionW < 1 || cropRegionH < 1) {
            cropRegionW = hwBnsW;
            cropRegionH = hwBnsH;
        }

        /* 2. Calculate the real crop region with considering the target ratio */
        if ((cropRegionW > hwBcropW) && (cropRegionH > hwBcropH)) {
            dstRect->x = ALIGN_DOWN((cropRegionX + ((cropRegionW - hwBcropW) >> 1)), 2);
            dstRect->y = ALIGN_DOWN((cropRegionY + ((cropRegionH - hwBcropH) >> 1)), 2);
            dstRect->w = hwBcropW;
            dstRect->h = hwBcropH;
        } else {
            ret = getCropRectAlign(cropRegionW, cropRegionH,
                    hwBcropW, hwBcropH,
                    &(dstRect->x), &(dstRect->y),
                    &(dstRect->w), &(dstRect->h),
                    CAMERA_MAGIC_ALIGN, 2,
                    0, 0.0f);
            dstRect->x = ALIGN_DOWN((cropRegionX + dstRect->x), 2);
            dstRect->y = ALIGN_DOWN((cropRegionY + dstRect->y), 2);
        }
    } else {
        /* dst */
        if (hwBnsW > hwBcropW) {
            dstRect->x = ALIGN_UP(((hwBnsW - hwBcropW) >> 1), 2);
            dstRect->w = hwBcropW;
        } else {
            dstRect->x = 0;
            dstRect->w = hwBnsW;
        }

        if (hwBnsH > hwBcropH) {
            dstRect->y = ALIGN_UP(((hwBnsH - hwBcropH) >> 1), 2);
            dstRect->h = hwBcropH;
        } else {
            dstRect->y = 0;
            dstRect->h = hwBnsH;
        }
    }

#if DEBUG
    ALOGD("DEBUG(%s):zoomRatio=%f", __FUNCTION__, zoomRatio);
    ALOGD("DEBUG(%s):hwBnsSize (%dx%d), hwBcropSize (%d, %d)(%dx%d)",
            __FUNCTION__, srcRect->w, srcRect->h, dstRect->x, dstRect->y, dstRect->w, dstRect->h);
#endif

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::calcPictureBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int maxSensorW = 0, maxSensorH = 0;
    int hwSensorW = 0, hwSensorH = 0;
    int hwPictureW = 0, hwPictureH = 0, hwPictureFormat = 0;
    int hwSensorCropX = 0, hwSensorCropY = 0;
    int hwSensorCropW = 0, hwSensorCropH = 0;
    int pictureW = 0, pictureH = 0, pictureFormat = 0;
    int previewW = 0, previewH = 0;
    int hwSensorMarginW = 0, hwSensorMarginH = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;

    int zoomLevel = 0;
    float zoomRatio = 1.0f;
    int maxZoomRatio = 0;
    int bayerFormat = CAMERA_BAYER_FORMAT;

#ifdef DEBUG_RAWDUMP
    if (checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    /* TODO: check state ready for start */
    pictureFormat = getHwPictureFormat();
    zoomLevel = getZoomLevel();
    maxZoomRatio = getMaxZoomRatio() / 1000;
    getHwPictureSize(&hwPictureW, &hwPictureH);
    getPictureSize(&pictureW, &pictureH);

    getMaxSensorSize(&maxSensorW, &maxSensorH);
    getHwSensorSize(&hwSensorW, &hwSensorH);
    getPreviewSize(&previewW, &previewH);
    getSensorMargin(&hwSensorMarginW, &hwSensorMarginH);

    zoomRatio = getZoomRatio(zoomLevel) / 1000;

    hwSensorW -= hwSensorMarginW;
    hwSensorH -= hwSensorMarginH;

    if (getUsePureBayerReprocessing() == true) {
        if (getHalVersion() == IS_HAL_VER_3_2) {
            int cropRegionX = 0, cropRegionY = 0, cropRegionW = 0, cropRegionH = 0;
            float scaleRatioX = 0.0f, scaleRatioY = 0.0f;

            m_getCropRegion(&cropRegionX, &cropRegionY, &cropRegionW, &cropRegionH);

            /* 1. Scale down the crop region to adjust with the bcrop input size */
            scaleRatioX = (float) hwSensorW / (float) maxSensorW;
            scaleRatioY = (float) hwSensorH / (float) maxSensorH;
            cropRegionX = (int) (cropRegionX * scaleRatioX);
            cropRegionY = (int) (cropRegionY * scaleRatioY);
            cropRegionW = (int) (cropRegionW * scaleRatioX);
            cropRegionH = (int) (cropRegionH * scaleRatioY);

            if (cropRegionW < 1 || cropRegionH < 1) {
                cropRegionW = hwSensorW;
                cropRegionH = hwSensorH;
            }

            ret = getCropRectAlign(cropRegionW, cropRegionH,
                    pictureW, pictureH,
                    &cropX, &cropY,
                    &cropW, &cropH,
                    CAMERA_MAGIC_ALIGN, 2,
                    0, 0.0f);

            cropX = ALIGN_DOWN((cropRegionX + cropX), 2);
            cropY = ALIGN_DOWN((cropRegionY + cropY), 2);
        } else {
            ret = getCropRectAlign(hwSensorW, hwSensorH,
                    pictureW, pictureH,
                    &cropX, &cropY,
                    &cropW, &cropH,
                    CAMERA_MAGIC_ALIGN, 2,
                    zoomLevel, zoomRatio);

            cropX = ALIGN_DOWN(cropX, 2);
            cropY = ALIGN_DOWN(cropY, 2);
            cropW = hwSensorW - (cropX * 2);
            cropH = hwSensorH - (cropY * 2);
        }

        if (cropW < pictureW / maxZoomRatio || cropH < pictureH / maxZoomRatio) {
            ALOGW("WRN(%s[%d]): zoom ratio is upto x%d, crop(%dx%d), picture(%dx%d)", __FUNCTION__, __LINE__, maxZoomRatio, cropW, cropH, pictureW, pictureH);
            cropX = ALIGN_DOWN(((hwSensorW - (pictureW / maxZoomRatio)) >> 1), 2);
            cropY = ALIGN_DOWN(((hwSensorH - (pictureH / maxZoomRatio)) >> 1), 2);
            cropW = hwSensorW - (cropX * 2);
            cropH = hwSensorH - (cropY * 2);
        }
    } else {
        zoomLevel = 0;
        zoomRatio = getZoomRatio(zoomLevel) / 1000;
        getHwBayerCropRegion(&hwSensorCropW, &hwSensorCropH, &hwSensorCropX, &hwSensorCropY);

        ret = getCropRectAlign(hwSensorCropW, hwSensorCropH,
                     pictureW, pictureH,
                     &cropX, &cropY,
                     &cropW, &cropH,
                     CAMERA_MAGIC_ALIGN, 2,
                     zoomLevel, zoomRatio);

        cropX = ALIGN_DOWN(cropX, 2);
        cropY = ALIGN_DOWN(cropY, 2);
        cropW = hwSensorCropW - (cropX * 2);
        cropH = hwSensorCropH - (cropY * 2);

        if (cropW < pictureW / maxZoomRatio || cropH < pictureH / maxZoomRatio) {
            ALOGW("WRN(%s[%d]): zoom ratio is upto x%d, crop(%dx%d), picture(%dx%d)", __FUNCTION__, __LINE__, maxZoomRatio, cropW, cropH, pictureW, pictureH);
            cropX = ALIGN_DOWN(((hwSensorCropW - (pictureW / maxZoomRatio)) >> 1), 2);
            cropY = ALIGN_DOWN(((hwSensorCropH - (pictureH / maxZoomRatio)) >> 1), 2);
            cropW = hwSensorCropW - (cropX * 2);
            cropH = hwSensorCropH - (cropY * 2);
        }
    }

#if 1
    ALOGD("DEBUG(%s):maxSensorSize (%dx%d), hwSensorSize (%dx%d), previewSize (%dx%d)",
            __FUNCTION__, maxSensorW, maxSensorH, hwSensorW, hwSensorH, previewW, previewH);
    ALOGD("DEBUG(%s):hwPictureSize (%dx%d), pictureSize (%dx%d)",
            __FUNCTION__, hwPictureW, hwPictureH, pictureW, pictureH);
    ALOGD("DEBUG(%s):size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, cropX, cropY, cropW, cropH, zoomLevel);
    ALOGD("DEBUG(%s):size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    ALOGD("DEBUG(%s):size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            __FUNCTION__, pictureFormat, JPEG_INPUT_COLOR_FMT);
#endif

    srcRect->x = 0;
    srcRect->y = 0;
    srcRect->w = maxSensorW;
    srcRect->h = maxSensorH;
    srcRect->fullW = maxSensorW;
    srcRect->fullH = maxSensorH;
    srcRect->colorFormat = bayerFormat;

    dstRect->x = cropX;
    dstRect->y = cropY;
    dstRect->w = cropW;
    dstRect->h = cropH;
    dstRect->fullW = cropW;
    dstRect->fullH = cropH;
    dstRect->colorFormat = bayerFormat;
    return NO_ERROR;
}

status_t ExynosCamera1Parameters::m_getPreviewBdsSize(ExynosRect *dstRect)
{
    int hwBdsW = 0;
    int hwBdsH = 0;
    int sizeList[SIZE_LUT_INDEX_END];

    /* matched ratio LUT is not existed, use equation */
    if (m_useSizeTable == false
        || m_staticInfo->previewSizeLut == NULL
        || m_staticInfo->previewSizeLutMax <= m_cameraInfo.previewSizeRatioId
        || m_getPreviewSizeList(sizeList) != NO_ERROR) {
        ExynosRect rect;
        return calcPreviewBDSSize(&rect, dstRect);
    }

    /* use LUT */
    hwBdsW = sizeList[BDS_W];
    hwBdsH = sizeList[BDS_H];

    if (getRecordingHint() == true) {
        int videoW = 0, videoH = 0;
        getVideoSize(&videoW, &videoH);

        if (m_cameraInfo.previewSizeRatioId != m_cameraInfo.videoSizeRatioId)
            ALOGV("WARN(%s[%d]):preview ratioId(%d) != videoRatioId(%d), use previewRatioId",
                    __FUNCTION__, __LINE__,
                    m_cameraInfo.previewSizeRatioId, m_cameraInfo.videoSizeRatioId);

        if ((videoW == 3840 && videoH == 2160) || (videoW == 2560 && videoH == 1440)) {
            hwBdsW = videoW;
            hwBdsH = videoH;
        }
    }

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = hwBdsW;
    dstRect->h = hwBdsH;

#ifdef DEBUG_PERFRAME
    ALOGD("DEBUG(%s):hwBdsSize (%dx%d)", __FUNCTION__, dstRect->w, dstRect->h);
#endif

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::calcPreviewBDSSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int hwSensorW = 0, hwSensorH = 0;
    int hwPictureW = 0, hwPictureH = 0;
    int pictureW = 0, pictureH = 0;
    int previewW = 0, previewH = 0;
    ExynosRect bnsSize;
    ExynosRect bayerCropSize;
#if 0
    int pictureFormat = 0, hwPictureFormat = 0;
#endif
    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;

    int zoomLevel = 0;
    int bayerFormat = CAMERA_BAYER_FORMAT;
    float zoomRatio = 1.0f;

#ifdef DEBUG_RAWDUMP
    if (checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    /* TODO: check state ready for start */
#if 0
    pictureFormat = getHwPictureFormat();
#endif
    zoomLevel = getZoomLevel();
    getHwPictureSize(&hwPictureW, &hwPictureH);
    getPictureSize(&pictureW, &pictureH);
    zoomRatio = getZoomRatio(zoomLevel) / 1000;

    getHwSensorSize(&hwSensorW, &hwSensorH);
    getPreviewSize(&previewW, &previewH);

    /* TODO: get crop size from ctlMetadata */
    ret = getCropRectAlign(hwSensorW, hwSensorH,
                     previewW, previewH,
                     &cropX, &cropY,
                     &cropW, &cropH,
                     CAMERA_MAGIC_ALIGN, 2,
                     zoomLevel, zoomRatio);

    zoomRatio = getZoomRatio(0) / 1000;

    ret = getCropRectAlign(cropW, cropH,
                     previewW, previewH,
                     &crop_crop_x, &crop_crop_y,
                     &crop_crop_w, &crop_crop_h,
                     2, 2,
                     0, zoomRatio);

    cropX = ALIGN_UP(cropX, 2);
    cropY = ALIGN_UP(cropY, 2);
    cropW = hwSensorW - (cropX * 2);
    cropH = hwSensorH - (cropY * 2);

//    ALIGN_UP(crop_crop_x, 2);
//    ALIGN_UP(crop_crop_y, 2);

#if 0
    ALOGD("DEBUG(%s):hwSensorSize (%dx%d), previewSize (%dx%d)",
            __FUNCTION__, hwSensorW, hwSensorH, previewW, previewH);
    ALOGD("DEBUG(%s):hwPictureSize (%dx%d), pictureSize (%dx%d)",
            __FUNCTION__, hwPictureW, hwPictureH, pictureW, pictureH);
    ALOGD("DEBUG(%s):size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, cropX, cropY, cropW, cropH, zoomLevel);
    ALOGD("DEBUG(%s):size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    ALOGD("DEBUG(%s):size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            __FUNCTION__, pictureFormat, JPEG_INPUT_COLOR_FMT);
#endif

    srcRect->x = cropX;
    srcRect->y = cropY;
    srcRect->w = cropW;
    srcRect->h = cropH;
    srcRect->fullW = cropW;
    srcRect->fullH = cropH;
    srcRect->colorFormat = bayerFormat;

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->colorFormat = JPEG_INPUT_COLOR_FMT;
    /* For Front Single Scenario, BDS should not be used */
    if (m_cameraId == CAMERA_ID_FRONT && getDualMode() == false) {
        getPreviewBayerCropSize(&bnsSize, &bayerCropSize);
        dstRect->w = bayerCropSize.w;
        dstRect->h = bayerCropSize.h;
        dstRect->fullW = bayerCropSize.w;
        dstRect->fullH = bayerCropSize.h;
    } else {
        dstRect->w = previewW;
        dstRect->h = previewH;
        dstRect->fullW = previewW;
        dstRect->fullH = previewH;
    }

    if (dstRect->w > srcRect->w)
        dstRect->w = srcRect->w;
    if (dstRect->h > srcRect->h)
        dstRect->h = srcRect->h;

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::calcPictureBDSSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int maxSensorW = 0, maxSensorH = 0;
    int hwPictureW = 0, hwPictureH = 0;
    int pictureW = 0, pictureH = 0;
    int previewW = 0, previewH = 0;
#if 0
    int pictureFormat = 0, hwPictureFormat = 0;
#endif
    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;

    int zoomLevel = 0;
    int bayerFormat = CAMERA_BAYER_FORMAT;
    float zoomRatio = 1.0f;

#ifdef DEBUG_RAWDUMP
    if (checkBayerDumpEnable()) {
        bayerFormat = CAMERA_DUMP_BAYER_FORMAT;
    }
#endif

    /* TODO: check state ready for start */
#if 0
    pictureFormat = getHwPictureFormat();
#endif
    zoomLevel = getZoomLevel();
    getHwPictureSize(&hwPictureW, &hwPictureH);
    getPictureSize(&pictureW, &pictureH);

    getMaxSensorSize(&maxSensorW, &maxSensorH);
    getPreviewSize(&previewW, &previewH);

    zoomRatio = getZoomRatio(zoomLevel) / 1000;
    /* TODO: get crop size from ctlMetadata */
    ret = getCropRectAlign(maxSensorW, maxSensorH,
                     pictureW, pictureH,
                     &cropX, &cropY,
                     &cropW, &cropH,
                     CAMERA_MAGIC_ALIGN, 2,
                     zoomLevel, zoomRatio);

    zoomRatio = getZoomRatio(0) / 1000;
    ret = getCropRectAlign(cropW, cropH,
                     pictureW, pictureH,
                     &crop_crop_x, &crop_crop_y,
                     &crop_crop_w, &crop_crop_h,
                     2, 2,
                     0, zoomRatio);

    cropX = ALIGN_UP(cropX, 2);
    cropY = ALIGN_UP(cropY, 2);
    cropW = maxSensorW - (cropX * 2);
    cropH = maxSensorH - (cropY * 2);

//    ALIGN_UP(crop_crop_x, 2);
//    ALIGN_UP(crop_crop_y, 2);

#if 0
    ALOGD("DEBUG(%s):SensorSize (%dx%d), previewSize (%dx%d)",
            __FUNCTION__, maxSensorW, maxSensorH, previewW, previewH);
    ALOGD("DEBUG(%s):hwPictureSize (%dx%d), pictureSize (%dx%d)",
            __FUNCTION__, hwPictureW, hwPictureH, pictureW, pictureH);
    ALOGD("DEBUG(%s):size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, cropX, cropY, cropW, cropH, zoomLevel);
    ALOGD("DEBUG(%s):size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    ALOGD("DEBUG(%s):size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            __FUNCTION__, pictureFormat, JPEG_INPUT_COLOR_FMT);
#endif

    srcRect->x = cropX;
    srcRect->y = cropY;
    srcRect->w = cropW;
    srcRect->h = cropH;
    srcRect->fullW = cropW;
    srcRect->fullH = cropH;
    srcRect->colorFormat = bayerFormat;

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = pictureW;
    dstRect->h = pictureH;
    dstRect->fullW = pictureW;
    dstRect->fullH = pictureH;
    dstRect->colorFormat = JPEG_INPUT_COLOR_FMT;

    if (dstRect->w > srcRect->w)
        dstRect->w = srcRect->w;
    if (dstRect->h > srcRect->h)
        dstRect->h = srcRect->h;

    return NO_ERROR;
}

bool ExynosCamera1Parameters::doCscRecording(void)
{
    bool ret = true;
    int hwPreviewW = 0, hwPreviewH = 0;
    int videoW = 0, videoH = 0;

    getHwPreviewSize(&hwPreviewW, &hwPreviewH);
    getVideoSize(&videoW, &videoH);
    CLOGV("DEBUG(%s[%d]):hwPreviewSize = %d x %d",  __FUNCTION__, __LINE__, hwPreviewW, hwPreviewH);
    CLOGV("DEBUG(%s[%d]):videoSize     = %d x %d",  __FUNCTION__, __LINE__, videoW, videoH);

    if (((videoW == 3840 && videoH == 2160) || (videoW == 1920 && videoH == 1080) || (videoW == 2560 && videoH == 1440))
        && m_useAdaptiveCSCRecording == true
        && videoW == hwPreviewW
        && videoH == hwPreviewH) {
        ret = false;
    }

    return ret;
}

bool ExynosCamera1Parameters::increaseMaxBufferOfPreview(void)
{
    if((getShotMode() == SHOT_MODE_BEAUTY_FACE)||(getShotMode() == SHOT_MODE_FRONT_PANORAMA)
        ) {
        return true;
    } else {
        return false;
    }
}

bool ExynosCamera1Parameters::getSupportedZoomPreviewWIthScaler(void)
{
    return true;
}

#ifdef DEBUG_RAWDUMP
bool ExynosCamera1Parameters::checkBayerDumpEnable(void)
{
    char enableRawDump[PROPERTY_VALUE_MAX];
    property_get("ro.debug.rawdump", enableRawDump, "0");

    if (strcmp(enableRawDump, "1") == 0) {
        /*CLOGD("checkBayerDumpEnable : 1");*/
        return true;
    } else {
        /*CLOGD("checkBayerDumpEnable : 0");*/
        return false;
    }
}
#endif  /* DEBUG_RAWDUMP */

void ExynosCamera1Parameters::setIsThumbnailCallbackOn(bool enable)
{
    m_IsThumbnailCallbackOn = enable;
}

bool ExynosCamera1Parameters::getIsThumbnailCallbackOn()
{
    return m_IsThumbnailCallbackOn;
}
}; /* namespace android */
