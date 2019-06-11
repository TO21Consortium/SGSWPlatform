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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCamera3Parameters"
#include <cutils/log.h>

#include "ExynosCamera3Parameters.h"

namespace android {

ExynosCamera3Parameters::ExynosCamera3Parameters(int cameraId)
{
    m_cameraId = cameraId;

    const char *myName = (m_cameraId == CAMERA_ID_BACK) ? "ParametersBack" : "ParametersFront";
    strncpy(m_name, myName,  EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    m_staticInfo = createExynosCamera3SensorInfo(cameraId);
    m_useSizeTable = (m_staticInfo->sizeTableSupport) ? USE_CAMERA_SIZE_TABLE : false;
    m_useAdaptiveCSCRecording = (cameraId == CAMERA_ID_BACK) ? USE_ADAPTIVE_CSC_RECORDING : false;

    m_exynosconfig = NULL;
    m_activityControl = new ExynosCameraActivityControl(m_cameraId);

    memset(&m_cameraInfo, 0, sizeof(struct exynos_camera_info));
    memset(&m_exifInfo, 0, sizeof(m_exifInfo));

    m_initMetadata();

    setHalVersion(IS_HAL_VER_3_2);
    m_setExifFixedAttribute();

    m_exynosconfig = new ExynosConfigInfo();

    mDebugInfo.num_of_appmarker = 1; /* Default : APP4 */
    mDebugInfo.idx[0][0] = APP_MARKER_4; /* matching the app marker 4 */

    mDebugInfo.debugSize[APP_MARKER_4] = sizeof(struct camera2_udm);
    mDebugInfo.debugData[APP_MARKER_4] = new char[mDebugInfo.debugSize[APP_MARKER_4]];
    memset((void *)mDebugInfo.debugData[APP_MARKER_4], 0, mDebugInfo.debugSize[APP_MARKER_4]);
    memset((void *)m_exynosconfig, 0x00, sizeof(struct ExynosConfigInfo));

    // CAUTION!! : Initial values must be prior to setDefaultParameter() function.
    // Initial Values : START
    m_IsThumbnailCallbackOn = false;
    m_fastFpsMode = 0;
    m_previewRunning = false;
    m_previewSizeChanged = false;
    m_pictureRunning = false;
    m_recordingRunning = false;
    m_flagRestartPreviewChecked = false;
    m_flagRestartPreview = false;
    m_reallocBuffer = false;
    m_setFocusmodeSetting = false;
    m_flagMeteringRegionChanged = false;
    m_flagCheckDualMode = false;
    m_flagHWVDisMode = false;
    m_flagVideoStabilization = false;
    m_flag3dnrMode = false;

    m_flagCheckRecordingHint = false;
    m_zoomWithScaler = false;

    m_useDynamicBayer = (cameraId == CAMERA_ID_BACK) ? USE_DYNAMIC_BAYER : USE_DYNAMIC_BAYER_FRONT;
    m_useDynamicBayerVideoSnapShot =
        (cameraId == CAMERA_ID_BACK) ? USE_DYNAMIC_BAYER_VIDEO_SNAP_SHOT : USE_DYNAMIC_BAYER_VIDEO_SNAP_SHOT_FRONT;
    m_useDynamicScc = (cameraId == CAMERA_ID_BACK) ? USE_DYNAMIC_SCC_REAR : USE_DYNAMIC_SCC_FRONT;
    m_useFastenAeStable = (cameraId == CAMERA_ID_BACK) ? USE_FASTEN_AE_STABLE : false;

    /* we cannot know now, whether recording mode or not */
    /*
    if (getRecordingHint() == true || getDualRecordingHint() == true)
        m_usePureBayerReprocessing = (cameraId == CAMERA_ID_BACK) ? USE_PURE_BAYER_REPROCESSING_ON_RECORDING : USE_PURE_BAYER_REPROCESSING_FRONT_ON_RECORDING;
    else
    */
     m_usePureBayerReprocessing = (cameraId == CAMERA_ID_BACK) ? USE_PURE_BAYER_REPROCESSING : USE_PURE_BAYER_REPROCESSING_FRONT;

    m_enabledMsgType = 0;

    m_previewBufferCount = NUM_PREVIEW_BUFFERS;

    m_dvfsLock = false;

#ifdef USE_BINNING_MODE
    m_binningProperty = checkProperty(false);
#endif
    m_zoom_activated = false;
#ifdef FORCE_CAL_RELOAD
    m_calValid = true;
#endif

    m_zoomWithScaler = false;
    m_exposureTimeCapture = 0;
    // Initial Values : END
    setDefaultCameraInfo();
    m_initDefaultInfo();
}

ExynosCamera3Parameters::~ExynosCamera3Parameters()
{
    if (m_staticInfo != NULL) {
        delete m_staticInfo;
        m_staticInfo = NULL;
    }

    if (m_activityControl != NULL) {
        delete m_activityControl;
        m_activityControl = NULL;
    }

    for(int i = 0; i < mDebugInfo.num_of_appmarker; i++) {
        if(mDebugInfo.debugData[mDebugInfo.idx[i][0]])
                delete mDebugInfo.debugData[mDebugInfo.idx[i][0]];
        mDebugInfo.debugData[mDebugInfo.idx[i][0]] = NULL;
        mDebugInfo.debugSize[mDebugInfo.idx[i][0]] = 0;
    }

    if (m_exynosconfig != NULL) {
        memset((void *)m_exynosconfig, 0x00, sizeof(struct ExynosConfigInfo));
        delete m_exynosconfig;
        m_exynosconfig = NULL;
    }

    if (m_exifInfo.maker_note) {
        delete m_exifInfo.maker_note;
        m_exifInfo.maker_note = NULL;
    }

    if (m_exifInfo.user_comment) {
        delete m_exifInfo.user_comment;
        m_exifInfo.user_comment = NULL;
    }
}

int ExynosCamera3Parameters::getCameraId(void)
{
    return m_cameraId;
}

status_t ExynosCamera3Parameters::m_initDefaultInfo(void)
{
    status_t ret = NO_ERROR;

    uint32_t curMinFps = 0;
    uint32_t curMaxFps = 0;

    m_setRecordingHint(false);
    m_setDualMode(false);
    m_setEffectHint(0);

    /* zoom */
    if (getZoomSupported() == true) {
        int maxZoom = getMaxZoomLevel();
        if (0 < maxZoom) {
            int max_zoom_ratio = (int)getMaxZoomRatio();
            setZoomRatioList(m_staticInfo->zoomRatioList, maxZoom - 1, (float)(max_zoom_ratio / 1000));
        }
    }

    getPreviewFpsRange(&curMinFps, &curMaxFps);
    CLOGI2("curFpsRange[Min=%d, Max=%d]", curMinFps, curMaxFps);

    m_setPreviewFpsRange((uint32_t)15, (uint32_t)30);
    getPreviewFpsRange(&curMinFps, &curMaxFps);
    m_activityControl->setFpsValue(curMinFps);

    m_setHwPreviewFormat(V4L2_PIX_FMT_NV21M);
    m_setCallbackFormat(V4L2_PIX_FMT_NV21M);
    m_setHwPictureFormat(SCC_OUTPUT_COLOR_FMT);

    /* Preview Size */
    getMaxPreviewSize(&m_cameraInfo.previewW, &m_cameraInfo.previewH);
    m_setHwPreviewSize(m_cameraInfo.previewW, m_cameraInfo.previewH);

    return ret;
}

void ExynosCamera3Parameters::setDefaultCameraInfo(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);
    m_setHwSensorSize(m_staticInfo->maxSensorW, m_staticInfo->maxSensorH);
    CLOGI("INFO(%s[%d]) m_setHwPreviewSize : %d x %d", __FUNCTION__, __LINE__, m_staticInfo->maxPreviewW, m_staticInfo->maxPreviewH);
    for (int i = 0; i < this->getYuvStreamMaxNum(); i++) {
        m_setYuvSize(m_staticInfo->maxPreviewW, m_staticInfo->maxPreviewH, i);
        m_setYuvFormat(V4L2_PIX_FMT_NV21, i);
    }

    m_setHwPictureSize(m_staticInfo->maxPictureW, m_staticInfo->maxPictureH);
    m_setHwPictureFormat(SCC_OUTPUT_COLOR_FMT);

    /* Initalize BNS scale ratio, step:500, ex)1500->x1.5 scale down */
    m_setBnsScaleRatio(1000);
    /* Initalize Binning scale ratio */
    m_setBinningScaleRatio(1000);
    /* Set Default VideoSize to FHD */
    m_setVideoSize(1920,1080);
}

status_t ExynosCamera3Parameters::m_setIntelligentMode(int intelligentMode)
{
    status_t ret = NO_ERROR;
    bool visionMode = false;

    m_cameraInfo.intelligentMode = intelligentMode;

    if (intelligentMode > 1) {
        if (m_staticInfo->visionModeSupport == true) {
            visionMode = true;
        } else {
            CLOGE2("tried to set vision mode(not supported)", "setParameters");
            ret = BAD_VALUE;
        }
    } else if (getVisionMode()) {
        CLOGE2("vision mode can not change before stoppreview");
        visionMode = true;
    }

    m_setVisionMode(visionMode);

    return ret;
 }

int ExynosCamera3Parameters::getIntelligentMode(void)
{
    return m_cameraInfo.intelligentMode;
}

void ExynosCamera3Parameters::m_setVisionMode(bool vision)
{
    m_cameraInfo.visionMode = vision;
}

bool ExynosCamera3Parameters::getVisionMode(void)
{
    return m_cameraInfo.visionMode;
}

void ExynosCamera3Parameters::m_setVisionModeFps(int fps)
{
    m_cameraInfo.visionModeFps = fps;
}

int ExynosCamera3Parameters::getVisionModeFps(void)
{
    return m_cameraInfo.visionModeFps;
}

void ExynosCamera3Parameters::m_setVisionModeAeTarget(int ae)
{
    m_cameraInfo.visionModeAeTarget = ae;
}

int ExynosCamera3Parameters::getVisionModeAeTarget(void)
{
    return m_cameraInfo.visionModeAeTarget;
}

void ExynosCamera3Parameters::m_setRecordingHint(bool hint)
{
    m_cameraInfo.recordingHint = hint;

    if (hint) {
        setMetaVideoMode(&m_metadata, AA_VIDEOMODE_ON);
    } else if (!hint && !getDualRecordingHint() && !getEffectRecordingHint()) {
        setMetaVideoMode(&m_metadata, AA_VIDEOMODE_OFF);
    }

    /* RecordingHint is confirmed */
    m_flagCheckRecordingHint = true;
}

bool ExynosCamera3Parameters::getRecordingHint(void)
{
    /*
     * Before setParameters, we cannot know recordingHint is valid or not
     * So, check and make assert for fast debugging
     */
    if (m_flagCheckRecordingHint == false)
        android_printAssert(NULL, LOG_TAG, "Cannot call getRecordingHint befor setRecordingHint, assert!!!!");

    return m_cameraInfo.recordingHint;
}

void ExynosCamera3Parameters::m_setDualMode(bool dual)
{
    m_cameraInfo.dualMode = dual;
    /* dualMode is confirmed */
    m_flagCheckDualMode = true;
}

bool ExynosCamera3Parameters::getDualMode(void)
{
    /*
    * Before setParameters, we cannot know dualMode is valid or not
    * So, check and make assert for fast debugging
    */
    if (m_flagCheckDualMode == false)
        android_printAssert(NULL, LOG_TAG, "Cannot call getDualMode befor checkDualMode, assert!!!!");

    return m_cameraInfo.dualMode;
}

void ExynosCamera3Parameters::m_setDualRecordingHint(bool hint)
{
    m_cameraInfo.dualRecordingHint = hint;

    if (hint) {
        setMetaVideoMode(&m_metadata, AA_VIDEOMODE_ON);
    } else if (!hint && !getRecordingHint() && !getEffectRecordingHint()) {
        setMetaVideoMode(&m_metadata, AA_VIDEOMODE_OFF);
    }
}

bool ExynosCamera3Parameters::getDualRecordingHint(void)
{
    return m_cameraInfo.dualRecordingHint;
}

void ExynosCamera3Parameters::m_setEffectHint(bool hint)
{
    m_cameraInfo.effectHint = hint;
}

bool ExynosCamera3Parameters::getEffectHint(void)
{
    return m_cameraInfo.effectHint;
}

bool ExynosCamera3Parameters::getEffectRecordingHint(void)
{
    return m_cameraInfo.effectRecordingHint;
}

status_t ExynosCamera3Parameters::m_adjustPreviewFpsRange(int &newMinFps, int &newMaxFps)
{
    bool flagSpecialMode = false;
    int curSceneMode = 0;
    int curShotMode = 0;

    if (getDualMode() == true) {
        flagSpecialMode = true;

        /* when dual mode, fps is limited by 24fps */
        if (24000 < newMaxFps)
            newMaxFps = 24000;

        /* set fixed fps. */
        newMinFps = newMaxFps;
        CLOGD2("dualMode(true), newMaxFps=%d", newMaxFps);
    }

    if (getDualRecordingHint() == true) {
        flagSpecialMode = true;

        /* when dual recording mode, fps is limited by 24fps */
        if (24000 < newMaxFps)
            newMaxFps = 24000;

        /* set fixed fps. */
        newMinFps = newMaxFps;
        CLOGD2("dualRecordingHint(true), newMaxFps=%d", newMaxFps);
    }

    if (getEffectHint() == true) {
        flagSpecialMode = true;
#if 0   /* Don't use to set fixed fps in the hal side. */
        /* when effect mode, fps is limited by 24fps */
        if (24000 < newMaxFps)
            newMaxFps = 24000;

        /* set fixed fps due to GPU preformance. */
        newMinFps = newMaxFps;
#endif
        CLOGD2("effectHint(true), newMaxFps=%d", newMaxFps);
    }

    if (getRecordingHint() == true) {
        flagSpecialMode = true;
#if 0   /* Don't use to set fixed fps in the hal side. */
#ifdef USE_VARIABLE_FPS_OF_FRONT_RECORDING
        if (getCameraId() == CAMERA_ID_FRONT && getSamsungCamera() == true) {
            /* Supported the variable frame rate for Image Quality Performacne */
            CLOGD2("RecordingHint(true),newMinFps=%d,newMaxFps=%d", newMinFps, newMaxFps);
        } else
#endif
        {
            /* set fixed fps. */
            newMinFps = newMaxFps;
        }
        CLOGD2("RecordingHint(true), newMaxFps=%d", newMaxFps);
#endif
        CLOGD2("RecordingHint(true),newMinFps=%d,newMaxFps=%d", newMinFps, newMaxFps);
    }

    if (flagSpecialMode == true) {
        CLOGD2("special mode enabled, newMaxFps=%d", newMaxFps);
        goto done;
    }

    curSceneMode = getSceneMode();
    switch (curSceneMode) {
    case SCENE_MODE_ACTION:
        if (getHighSpeedRecording() == true){
            newMinFps = newMaxFps;
        } else {
            newMinFps = 30000;
            newMaxFps = 30000;
        }
        break;
    case SCENE_MODE_PORTRAIT:
    case SCENE_MODE_LANDSCAPE:
        if (getHighSpeedRecording() == true){
            newMinFps = newMaxFps / 2;
        } else {
            newMinFps = 15000;
            newMaxFps = 30000;
        }
        break;
    case SCENE_MODE_NIGHT:
        /* for Front MMS mode FPS */
        if (getCameraId() == CAMERA_ID_FRONT && getRecordingHint() == true)
            break;

        if (getHighSpeedRecording() == true){
            newMinFps = newMaxFps / 4;
        } else {
            newMinFps = 8000;
            newMaxFps = 30000;
        }
        break;
    case SCENE_MODE_NIGHT_PORTRAIT:
    case SCENE_MODE_THEATRE:
    case SCENE_MODE_BEACH:
    case SCENE_MODE_SNOW:
    case SCENE_MODE_SUNSET:
    case SCENE_MODE_STEADYPHOTO:
    case SCENE_MODE_FIREWORKS:
    case SCENE_MODE_SPORTS:
    case SCENE_MODE_PARTY:
    case SCENE_MODE_CANDLELIGHT:
        if (getHighSpeedRecording() == true){
            newMinFps = newMaxFps / 2;
        } else {
            newMinFps = 15000;
            newMaxFps = 30000;
        }
        break;
    default:
        break;
    }

    curShotMode = getShotMode();
    switch (curShotMode) {
    case SHOT_MODE_DRAMA:
    case SHOT_MODE_3DTOUR:
    case SHOT_MODE_3D_PANORAMA:
    case SHOT_MODE_LIGHT_TRACE:
        newMinFps = 30000;
        newMaxFps = 30000;
        break;
    case SHOT_MODE_ANIMATED_SCENE:
        newMinFps = 15000;
        newMaxFps = 15000;
        break;
#ifdef USE_LIMITATION_FOR_THIRD_PARTY
    case THIRD_PARTY_BLACKBOX_MODE:
        CLOGI2("limit the maximum 30 fps range in THIRD_PARTY_BLACKBOX_MODE(%d,%d)", newMinFps, newMaxFps);
        if (newMinFps > 30000) {
            newMinFps = 30000;
        }
        if (newMaxFps > 30000) {
            newMaxFps = 30000;
        }
        break;
    case THIRD_PARTY_VTCALL_MODE:
        CLOGI2("limit the maximum 15 fps range in THIRD_PARTY_VTCALL_MODE(%d,%d)", newMinFps, newMaxFps);
        if (newMinFps > 15000) {
            newMinFps = 15000;
        }
        if (newMaxFps > 15000) {
            newMaxFps = 15000;
        }
        break;
    case THIRD_PARTY_HANGOUT_MODE:
        CLOGI2("change fps range 15000,15000 in THIRD_PARTY_HANGOUT_MODE");
        newMinFps = 15000;
        newMaxFps = 15000;
        break;
#endif
    default:
        break;
    }

done:
    if (newMinFps != newMaxFps) {
        if (m_getSupportedVariableFpsList(newMinFps, newMaxFps, &newMinFps, &newMaxFps) == false)
            newMinFps = newMaxFps / 2;
    }

    return NO_ERROR;
}

void ExynosCamera3Parameters::updatePreviewFpsRange(void)
{
    uint32_t curMinFps = 0;
    uint32_t curMaxFps = 0;
    int newMinFps = 0;
    int newMaxFps = 0;

    getPreviewFpsRange(&curMinFps, &curMaxFps);
    newMinFps = curMinFps * 1000;
    newMaxFps = curMaxFps * 1000;

    if (m_adjustPreviewFpsRange(newMinFps, newMaxFps) != NO_ERROR) {
        CLOGE2("Fils to adjust preview fps range");
        return;
    }

    newMinFps = newMinFps / 1000;
    newMaxFps = newMaxFps / 1000;

    if (curMinFps != (uint32_t)newMinFps || curMaxFps != (uint32_t)newMaxFps) {
        m_setPreviewFpsRange((uint32_t)newMinFps, (uint32_t)newMaxFps);
    }
}

status_t ExynosCamera3Parameters::checkPreviewFpsRange(uint32_t minFps, uint32_t maxFps)
{
    status_t ret = NO_ERROR;
    uint32_t curMinFps = 0, curMaxFps = 0;

    getPreviewFpsRange(&curMinFps, &curMaxFps);
    if (curMinFps != minFps || curMaxFps != maxFps)
        m_setPreviewFpsRange(minFps, maxFps);

    return ret;
}

void ExynosCamera3Parameters::m_setPreviewFpsRange(uint32_t min, uint32_t max)
{
    setMetaCtlAeTargetFpsRange(&m_metadata, min, max);
    setMetaCtlSensorFrameDuration(&m_metadata, (uint64_t)((1000 * 1000 * 1000) / (uint64_t)max));

    CLOGI2("fps min(%d) max(%d)", min, max);
}

void ExynosCamera3Parameters::getPreviewFpsRange(uint32_t *min, uint32_t *max)
{
    /* ex) min = 15 , max = 30 */
    getMetaCtlAeTargetFpsRange(&m_metadata, min, max);
}

bool ExynosCamera3Parameters::m_getSupportedVariableFpsList(int min, int max, int *newMin, int *newMax)
{
    int (*sizeList)[2];

    if (getCameraId() == CAMERA_ID_BACK) {
        /* Try to find exactly same in REAR LIST*/
        sizeList = m_staticInfo->rearFPSList;
        for (int i = 0; i < m_staticInfo->rearFPSListMax; i++) {
            if (sizeList[i][1] == max && sizeList[i][0] == min) {
                *newMin = sizeList[i][0];
                *newMax = sizeList[i][1];

                return true;
            }
        }
        /* Try to find exactly same in HIDDEN REAR LIST*/
        sizeList = m_staticInfo->hiddenRearFPSList;
        for (int i = 0; i < m_staticInfo->hiddenRearFPSListMax; i++) {
            if (sizeList[i][1] == max && sizeList[i][0] == min) {
                *newMin = sizeList[i][0];
                *newMax = sizeList[i][1];

                return true;
            }
        }
        /* Try to find similar fps in REAR LIST*/
        sizeList = m_staticInfo->rearFPSList;
        for (int i = 0; i < m_staticInfo->rearFPSListMax; i++) {
            if (max <= sizeList[i][1] && sizeList[i][0] <= min) {
                if(sizeList[i][1] == sizeList[i][0])
                    continue;

                *newMin = sizeList[i][0];
                *newMax = sizeList[i][1];

                CLOGW2("calibrate new fps(%d/%d -> %d/%d)", min, max, *newMin, *newMax);

                return true;
            }
        }
        /* Try to find similar fps in HIDDEN REAR LIST*/
        sizeList = m_staticInfo->hiddenRearFPSList;
        for (int i = 0; i < m_staticInfo->hiddenRearFPSListMax; i++) {
            if (max <= sizeList[i][1] && sizeList[i][0] <= min) {
                if(sizeList[i][1] == sizeList[i][0])
                    continue;

                *newMin = sizeList[i][0];
                *newMax = sizeList[i][1];

                CLOGW2("calibrate new fps(%d/%d -> %d/%d)", min, max, *newMin, *newMax);

                return true;
            }
        }
    } else {
        /* Try to find exactly same in FRONT LIST*/
        sizeList = m_staticInfo->frontFPSList;
        for (int i = 0; i < m_staticInfo->frontFPSListMax; i++) {
            if (sizeList[i][1] == max && sizeList[i][0] == min) {
                *newMin = sizeList[i][0];
                *newMax = sizeList[i][1];

                return true;
            }
        }
        /* Try to find exactly same in HIDDEN FRONT LIST*/
        sizeList = m_staticInfo->hiddenFrontFPSList;
        for (int i = 0; i < m_staticInfo->hiddenFrontFPSListMax; i++) {
            if (sizeList[i][1] == max && sizeList[i][0] == min) {
                *newMin = sizeList[i][0];
                *newMax = sizeList[i][1];

                return true;
            }
        }
        /* Try to find similar fps in FRONT LIST*/
        sizeList = m_staticInfo->frontFPSList;
        for (int i = 0; i < m_staticInfo->frontFPSListMax; i++) {
            if (max <= sizeList[i][1] && sizeList[i][0] <= min) {
                if(sizeList[i][1] == sizeList[i][0])
                    continue;

                *newMin = sizeList[i][0];
                *newMax = sizeList[i][1];

                CLOGW2("calibrate new fps(%d/%d -> %d/%d)", min, max, *newMin, *newMax);

                return true;
            }
        }
        /* Try to find similar fps in HIDDEN FRONT LIST*/
        sizeList = m_staticInfo->hiddenFrontFPSList;
        for (int i = 0; i < m_staticInfo->hiddenFrontFPSListMax; i++) {
            if (max <= sizeList[i][1] && sizeList[i][0] <= min) {
                if(sizeList[i][1] == sizeList[i][0])
                    continue;

                *newMin = sizeList[i][0];
                *newMax = sizeList[i][1];

                CLOGW2("calibrate new fps(%d/%d -> %d/%d)", min, max, *newMin, *newMax);

                return true;
            }
        }
    }

    return false;
}

#if 0
status_t ExynosCamera3Parameters::checkVideoSize(const CameraParameters& params)
{
    /*  Video size */
    int newVideoW = 0;
    int newVideoH = 0;

    params.getVideoSize(&newVideoW, &newVideoH);

    if (0 < newVideoW && 0 < newVideoH &&
        m_isSupportedVideoSize(newVideoW, newVideoH) == false) {
        return BAD_VALUE;
    }

    CLOGI("INFO(%s):newVideo Size (%dx%d), ratioId(%d)",
        "setParameters", newVideoW, newVideoH, m_cameraInfo.videoSizeRatioId);
    m_setVideoSize(newVideoW, newVideoH);
    m_params.setVideoSize(newVideoW, newVideoH);

    return NO_ERROR;
}
#else
status_t ExynosCamera3Parameters::checkVideoSize(int newVideoW, int newVideoH)
{
    /*  Video size */
//    params.getVideoSize(&newVideoW, &newVideoH);

    if (0 < newVideoW && 0 < newVideoH &&
        m_isSupportedVideoSize(newVideoW, newVideoH) == false) {
        return BAD_VALUE;
    }

    CLOGI("INFO(%s):newVideo Size (%dx%d), ratioId(%d)",
        "setParameters", newVideoW, newVideoH, m_cameraInfo.videoSizeRatioId);
    m_setVideoSize(newVideoW, newVideoH);
//    m_params.setVideoSize(newVideoW, newVideoH);

    return NO_ERROR;
}
#endif

bool ExynosCamera3Parameters::m_isSupportedVideoSize(const int width,
                                                    const int height)
{
    int maxWidth = 0;
    int maxHeight = 0;
    int (*sizeList)[SIZE_OF_RESOLUTION];

    getMaxVideoSize(&maxWidth, &maxHeight);

    if (maxWidth < width || maxHeight < height) {
        CLOGE2("invalid video Size(maxSize(%d/%d) size(%d/%d)", maxWidth, maxHeight, width, height);
        return false;
    }

    if (getCameraId() == CAMERA_ID_BACK) {
        sizeList = m_staticInfo->rearVideoList;
        for (int i = 0; i < m_staticInfo->rearVideoListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;
            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                m_cameraInfo.videoSizeRatioId = sizeList[i][2];
                return true;
            }
        }
    } else {
        sizeList = m_staticInfo->frontVideoList;
        for (int i = 0; i <  m_staticInfo->frontVideoListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;
            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                m_cameraInfo.videoSizeRatioId = sizeList[i][2];
                return true;
            }
        }
    }

    if (getCameraId() == CAMERA_ID_BACK) {
        sizeList = m_staticInfo->hiddenRearVideoList;
        for (int i = 0; i < m_staticInfo->hiddenRearVideoListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;
            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                m_cameraInfo.videoSizeRatioId = sizeList[i][2];
                return true;
            }
        }
    } else {
        sizeList = m_staticInfo->hiddenFrontVideoList;
        for (int i = 0; i < m_staticInfo->hiddenFrontVideoListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;
            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                m_cameraInfo.videoSizeRatioId = sizeList[i][2];
                return true;
            }
        }
    }

    CLOGE2("Invalid video size(%dx%d)", width, height);

    return false;
}

bool ExynosCamera3Parameters::m_isUHDRecordingMode(void)
{
    bool isUHDRecording = false;
    int videoW = 0, videoH = 0;
    getVideoSize(&videoW, &videoH);

    if (((videoW == 3840 && videoH == 2160) || (videoW == 2560 && videoH == 1440)) && getRecordingHint() == true)
        isUHDRecording = true;

#if 0
    /* we need to make WQHD SCP(LCD size), when FHD recording for clear rendering */
    int hwPreviewW = 0, hwPreviewH = 0;
    getHwPreviewSize(&hwPreviewW, &hwPreviewH);

    /* regard align margin(ex:1920x1088), check size more than 1920x1088 */
    /* if (1920 < hwPreviewW && 1080 < hwPreviewH) */
    if ((ALIGN_UP(1920, CAMERA_MAGIC_ALIGN) < hwPreviewW) &&
        (ALIGN_UP(1080, CAMERA_MAGIC_ALIGN) < hwPreviewH) &&
        (getRecordingHint() == true)) {
        isUHDRecording = true;
    }
#endif

    return isUHDRecording;
}

void ExynosCamera3Parameters::m_setVideoSize(int w, int h)
{
    m_cameraInfo.videoW = w;
    m_cameraInfo.videoH = h;
}

bool ExynosCamera3Parameters::getUHDRecordingMode(void)
{
    return m_isUHDRecordingMode();
}

void ExynosCamera3Parameters::getVideoSize(int *w, int *h)
{
    *w = m_cameraInfo.videoW;
    *h = m_cameraInfo.videoH;
}

void ExynosCamera3Parameters::getMaxVideoSize(int *w, int *h)
{
    *w = m_staticInfo->maxVideoW;
    *h = m_staticInfo->maxVideoH;
}

int ExynosCamera3Parameters::getVideoFormat(void)
{
    if (getAdaptiveCSCRecording() == true) {
        return V4L2_PIX_FMT_NV21M;
    } else {
        return V4L2_PIX_FMT_NV12M;
    }
}

status_t ExynosCamera3Parameters::checkCallbackSize(int callbackW, int callbackH)
{
    status_t ret = NO_ERROR;
    int curCallbackW = -1, curCallbackH = -1;

    if (callbackW < 0 || callbackH < 0) {
        CLOGE("ERR(%s[%d]):Invalid callback size. %dx%d",
                __FUNCTION__, __LINE__, callbackW, callbackH);
        return INVALID_OPERATION;
    }

    getCallbackSize(&curCallbackW, &curCallbackH);

    if (callbackW != curCallbackW || callbackH != curCallbackH) {
        ALOGI("INFO(%s[%d]):curCallbackSize %dx%d newCallbackSize %dx%d",
                __FUNCTION__, __LINE__,
                curCallbackW, curCallbackH, callbackW, callbackH);

        m_setCallbackSize(callbackW, callbackH);
    }

    return ret;
}

void ExynosCamera3Parameters::m_setCallbackSize(int w, int h)
{
    m_cameraInfo.callbackW = w;
    m_cameraInfo.callbackH = h;
}

void ExynosCamera3Parameters::getCallbackSize(int *w, int *h)
{
    *w = m_cameraInfo.callbackW;
    *h = m_cameraInfo.callbackH;
}

status_t ExynosCamera3Parameters::checkCallbackFormat(int callbackFormat)
{
    status_t ret = NO_ERROR;
    int curCallbackFormat = -1;
    int newCallbackFormat = -1;

    if (callbackFormat < 0) {
        CLOGE("ERR(%s[%d]):Inavlid callback format. %x",
                __FUNCTION__, __LINE__, callbackFormat);
        return INVALID_OPERATION;
    }

    newCallbackFormat = HAL_PIXEL_FORMAT_2_V4L2_PIX(callbackFormat);
    curCallbackFormat = getCallbackFormat();

    if (curCallbackFormat != newCallbackFormat) {
        char curFormatName[V4L2_FOURCC_LENGTH] = {};
        char newFormatName[V4L2_FOURCC_LENGTH] = {};
        m_getV4l2Name(curFormatName, V4L2_FOURCC_LENGTH, curCallbackFormat);
        m_getV4l2Name(newFormatName, V4L2_FOURCC_LENGTH, newCallbackFormat);
        CLOGI("INFO(%s[%d]):curCallbackFormat %s newCallbackFormat %s",
                __FUNCTION__, __LINE__, curFormatName, newFormatName);

        m_setCallbackFormat(newCallbackFormat);
    }

    return ret;
}

void ExynosCamera3Parameters::m_setCallbackFormat(int colorFormat)
{
    m_cameraInfo.callbackFormat = colorFormat;
}

int ExynosCamera3Parameters::getCallbackFormat(void)
{
    return m_cameraInfo.callbackFormat;
}

bool ExynosCamera3Parameters::getReallocBuffer() {
    Mutex::Autolock lock(m_reallocLock);
    return m_reallocBuffer;
}

bool ExynosCamera3Parameters::setReallocBuffer(bool enable) {
    Mutex::Autolock lock(m_reallocLock);
    m_reallocBuffer = enable;
    return m_reallocBuffer;
}

void ExynosCamera3Parameters::setFastFpsMode(int fpsMode)
{
    m_fastFpsMode = fpsMode;
}

int ExynosCamera3Parameters::getFastFpsMode(void)
{
    return m_fastFpsMode;
}

void ExynosCamera3Parameters::m_setHighSpeedRecording(bool highSpeed)
{
    m_cameraInfo.highSpeedRecording = highSpeed;
}

bool ExynosCamera3Parameters::getHighSpeedRecording(void)
{
    return m_cameraInfo.highSpeedRecording;
}

bool ExynosCamera3Parameters::m_adjustHighSpeedRecording(int curMinFps, int curMaxFps, __unused int newMinFps, int newMaxFps)
{
    bool flagHighSpeedRecording = false;
    bool restartPreview = false;

    /* setting high speed */
    if (30 < newMaxFps) {
        flagHighSpeedRecording = true;
        /* 30 -> 60/120 */
        if (curMaxFps <= 30)
            restartPreview = true;
        /* 60 -> 120 */
        else if (curMaxFps <= 60 && 120 <= newMaxFps)
            restartPreview = true;
        /* 120 -> 60 */
        else if (curMaxFps <= 120 && newMaxFps <= 60)
            restartPreview = true;
        /* variable 60 -> fixed 60 */
        else if (curMinFps < 60 && newMaxFps <= 60)
            restartPreview = true;
        /* variable 120 -> fixed 120 */
        else if (curMinFps < 120 && newMaxFps <= 120)
            restartPreview = true;
    } else if (newMaxFps <= 30) {
        flagHighSpeedRecording = false;
        if (30 < curMaxFps)
            restartPreview = true;
    }

    if (restartPreview == true &&
        getPreviewRunning() == true) {
        CLOGD2("setRestartPreviewChecked true");
        m_setRestartPreviewChecked(true);
    }

    return flagHighSpeedRecording;
}

void ExynosCamera3Parameters::m_setRestartPreviewChecked(bool restart)
{
    CLOGD2("setRestartPreviewChecked(during SetParameters) %s", restart ? "true" : "false");
    Mutex::Autolock lock(m_parameterLock);

    m_flagRestartPreviewChecked = restart;
}

bool ExynosCamera3Parameters::m_getRestartPreviewChecked(void)
{
    Mutex::Autolock lock(m_parameterLock);

    return m_flagRestartPreviewChecked;
}

bool ExynosCamera3Parameters::getPreviewSizeChanged(void)
{
    return m_previewSizeChanged;
}

void ExynosCamera3Parameters::m_setRestartPreview(bool restart)
{
    CLOGD2("DEBUG(%s):setRestartPreview %s", restart ? "true" : "false");
    Mutex::Autolock lock(m_parameterLock);

    m_flagRestartPreview = restart;

}

void ExynosCamera3Parameters::setPreviewRunning(bool enable)
{
    Mutex::Autolock lock(m_parameterLock);

    m_previewRunning = enable;
    m_flagRestartPreviewChecked = false;
    m_flagRestartPreview = false;
    m_previewSizeChanged = false;
}

void ExynosCamera3Parameters::setPictureRunning(bool enable)
{
    Mutex::Autolock lock(m_parameterLock);

    m_pictureRunning = enable;
}

void ExynosCamera3Parameters::setRecordingRunning(bool enable)
{
    Mutex::Autolock lock(m_parameterLock);

    m_recordingRunning = enable;
}

bool ExynosCamera3Parameters::getPreviewRunning(void)
{
    Mutex::Autolock lock(m_parameterLock);

    return m_previewRunning;
}

bool ExynosCamera3Parameters::getPictureRunning(void)
{
    Mutex::Autolock lock(m_parameterLock);

    return m_pictureRunning;
}

bool ExynosCamera3Parameters::getRecordingRunning(void)
{
    Mutex::Autolock lock(m_parameterLock);

    return m_recordingRunning;
}

bool ExynosCamera3Parameters::getRestartPreview(void)
{
    Mutex::Autolock lock(m_parameterLock);

    return m_flagRestartPreview;
}

void ExynosCamera3Parameters::m_setVideoStabilization(bool stabilization)
{
    m_cameraInfo.videoStabilization = stabilization;
}

bool ExynosCamera3Parameters::getVideoStabilization(void)
{
    return m_cameraInfo.videoStabilization;
}

bool ExynosCamera3Parameters::updateTpuParameters(void)
{
    status_t ret = NO_ERROR;

    /* 1. update data video stabilization state to actual*/
    CLOGD2("video stabilization old(%d) new(%d)", m_cameraInfo.videoStabilization, m_flagVideoStabilization);
    m_setVideoStabilization(m_flagVideoStabilization);

    bool hwVdisMode  = this->getHWVdisMode();

    if (setDisEnable(hwVdisMode) != NO_ERROR) {
        CLOGE2("setDisEnable(%d) fail", hwVdisMode);
    }

    /* 2. update data 3DNR state to actual*/
    CLOGD2("3DNR old(%d) new(%d)", m_cameraInfo.is3dnrMode, m_flag3dnrMode);
    m_set3dnrMode(m_flag3dnrMode);
    if (setDnrEnable(m_flag3dnrMode) != NO_ERROR) {
        CLOGE2("setDnrEnable(%d) fail", m_flag3dnrMode);
    }

    return true;
}

bool ExynosCamera3Parameters::isSWVdisMode(void)
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

bool ExynosCamera3Parameters::isSWVdisModeWithParam(int nPreviewW, int nPreviewH)
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

bool ExynosCamera3Parameters::getHWVdisMode(void)
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

int ExynosCamera3Parameters::getHWVdisFormat(void)
{
    return V4L2_PIX_FMT_YUYV;
}

void ExynosCamera3Parameters::m_setSWVdisMode(bool swVdis)
{
    m_cameraInfo.swVdisMode = swVdis;
}

bool ExynosCamera3Parameters::getSWVdisMode(void)
{
    return m_cameraInfo.swVdisMode;
}

void ExynosCamera3Parameters::m_setSWVdisUIMode(bool swVdisUI)
{
    m_cameraInfo.swVdisUIMode = swVdisUI;
}

bool ExynosCamera3Parameters::getSWVdisUIMode(void)
{
    return m_cameraInfo.swVdisUIMode;
}
#if 0
status_t ExynosCamera3Parameters::checkPreviewSize(const CameraParameters& params)
{
    /* preview size */
    int previewW = 0;
    int previewH = 0;
    int newPreviewW = 0;
    int newPreviewH = 0;
    int newCalHwPreviewW = 0;
    int newCalHwPreviewH = 0;

    int curPreviewW = 0;
    int curPreviewH = 0;
    int curHwPreviewW = 0;
    int curHwPreviewH = 0;

    params.getPreviewSize(&previewW, &previewH);
    getPreviewSize(&curPreviewW, &curPreviewH);
    getHwPreviewSize(&curHwPreviewW, &curHwPreviewH);
    m_isHighResolutionMode(params);

    newPreviewW = previewW;
    newPreviewH = previewH;
    if (m_adjustPreviewSize(previewW, previewH, &newPreviewW, &newPreviewH, &newCalHwPreviewW, &newCalHwPreviewH) != OK) {
        CLOGE("ERR(%s): adjustPreviewSize fail, newPreviewSize(%dx%d)", "Parameters", newPreviewW, newPreviewH);
        return BAD_VALUE;
    }

    if (m_isSupportedPreviewSize(newPreviewW, newPreviewH) == false) {
        CLOGE("ERR(%s): new preview size is invalid(%dx%d)", "Parameters", newPreviewW, newPreviewH);
        return BAD_VALUE;
    }

    CLOGI("INFO(%s):Cur Preview size(%dx%d)", "setParameters", curPreviewW, curPreviewH);
    CLOGI("INFO(%s):Cur HwPreview size(%dx%d)", "setParameters", curHwPreviewW, curHwPreviewH);
    CLOGI("INFO(%s):param.preview size(%dx%d)", "setParameters", previewW, previewH);
    CLOGI("INFO(%s):Adjust Preview size(%dx%d), ratioId(%d)", "setParameters", newPreviewW, newPreviewH, m_cameraInfo.previewSizeRatioId);
    CLOGI("INFO(%s):Calibrated HwPreview size(%dx%d)", "setParameters", newCalHwPreviewW, newCalHwPreviewH);

    if (curPreviewW != newPreviewW || curPreviewH != newPreviewH ||
        curHwPreviewW != newCalHwPreviewW || curHwPreviewH != newCalHwPreviewH ||
        getHighResolutionCallbackMode() == true) {
        m_setPreviewSize(newPreviewW, newPreviewH);
        m_setHwPreviewSize(newCalHwPreviewW, newCalHwPreviewH);

        if (getHighResolutionCallbackMode() == true) {
            m_previewSizeChanged = false;
        } else {
            CLOGD2("DEBUG(%s):setRestartPreviewChecked true");
            m_setRestartPreviewChecked(true);
            m_previewSizeChanged = true;
        }
    } else {
        m_previewSizeChanged = false;
    }

    updateBinningScaleRatio();
    updateBnsScaleRatio();

    m_params.setPreviewSize(newPreviewW, newPreviewH);

    return NO_ERROR;
}
#else
status_t ExynosCamera3Parameters::checkPreviewSize(int previewW, int previewH)
{
    /* preview size */
    int newPreviewW = 0;
    int newPreviewH = 0;
    int newCalHwPreviewW = 0;
    int newCalHwPreviewH = 0;

    int curPreviewW = 0;
    int curPreviewH = 0;
    int curHwPreviewW = 0;
    int curHwPreviewH = 0;

//    params.getPreviewSize(&previewW, &previewH);
    getPreviewSize(&curPreviewW, &curPreviewH);
    getHwPreviewSize(&curHwPreviewW, &curHwPreviewH);
//    m_isHighResolutionMode(params);

    newPreviewW = previewW;
    newPreviewH = previewH;
    if (m_adjustPreviewSize(previewW, previewH, &newPreviewW, &newPreviewH, &newCalHwPreviewW, &newCalHwPreviewH) != OK) {
        CLOGE("ERR(%s): adjustPreviewSize fail, newPreviewSize(%dx%d)", "Parameters", newPreviewW, newPreviewH);
        return BAD_VALUE;
    }

    if (m_isSupportedPreviewSize(newPreviewW, newPreviewH) == false) {
        CLOGE("ERR(%s): new preview size is invalid(%dx%d)", "Parameters", newPreviewW, newPreviewH);
        return BAD_VALUE;
    }

    CLOGI("INFO(%s):Cur Preview size(%dx%d)", "setParameters", curPreviewW, curPreviewH);
    CLOGI("INFO(%s):Cur HwPreview size(%dx%d)", "setParameters", curHwPreviewW, curHwPreviewH);
    CLOGI("INFO(%s):param.preview size(%dx%d)", "setParameters", previewW, previewH);
    CLOGI("INFO(%s):Adjust Preview size(%dx%d), ratioId(%d)", "setParameters", newPreviewW, newPreviewH, m_cameraInfo.previewSizeRatioId);
    CLOGI("INFO(%s):Calibrated HwPreview size(%dx%d)", "setParameters", newCalHwPreviewW, newCalHwPreviewH);

    if (curPreviewW != newPreviewW || curPreviewH != newPreviewH ||
        curHwPreviewW != newCalHwPreviewW || curHwPreviewH != newCalHwPreviewH ||
        getHighResolutionCallbackMode() == true) {
        m_setPreviewSize(newPreviewW, newPreviewH);
        m_setHwPreviewSize(newCalHwPreviewW, newCalHwPreviewH);

        if (getHighResolutionCallbackMode() == true) {
            m_previewSizeChanged = false;
        } else {
            CLOGD2("setRestartPreviewChecked true");
            m_setRestartPreviewChecked(true);
            m_previewSizeChanged = true;
        }
    } else {
        m_previewSizeChanged = false;
    }

    updateBinningScaleRatio();
    updateBnsScaleRatio();

//    m_params.setPreviewSize(newPreviewW, newPreviewH);

    return NO_ERROR;
}
#endif

status_t ExynosCamera3Parameters::checkYuvSize(const int width, const int height, const int outputPortId)
{
    status_t ret = NO_ERROR;
    int curYuvWidth = 0;
    int curYuvHeight = 0;

    getYuvSize(&curYuvWidth, &curYuvHeight, outputPortId);

    if (m_isSupportedPictureSize(width, height) == false) {
        ALOGE("ERR(%s[%d]):Invalid YUV size. %dx%d",
                __FUNCTION__, __LINE__, width, height);
        return BAD_VALUE;
    }

    CLOGI("INFO(%s[%d]):curYuvSize %dx%d newYuvSize %dx%d outputPortId %d",
            __FUNCTION__, __LINE__,
            curYuvWidth, curYuvHeight, width, height, outputPortId);

    if (curYuvWidth != width || curYuvHeight != height) {
        m_setYuvSize(width, height, outputPortId);

        ALOGD("DEBUG(%s):setRestartPreviewChecked true", __FUNCTION__);
        m_setRestartPreviewChecked(true);
        m_previewSizeChanged = true;
    } else {
        m_previewSizeChanged = false;
    }

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::m_adjustPreviewSize(__unused int previewW, __unused int previewH,
                                                     int *newPreviewW, int *newPreviewH,
                                                     int *newCalHwPreviewW, int *newCalHwPreviewH)
{
    /* hack : when app give 1446, we calibrate to 1440 */
    if (*newPreviewW == 1446 && *newPreviewH == 1080) {
        CLOGW2("Invalid previewSize(%d/%d). so, calibrate to (1440/%d)", *newPreviewW, *newPreviewH, *newPreviewH);
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
            CLOGE2("m_getPreviewSizeList() fail");
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
                        CLOGW2("Limit hw preview size until %d x %d when videoSize(%d x %d)",
                            *newCalHwPreviewW, *newCalHwPreviewH, videoW, videoH);
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
                CLOGV2("preview(%dx%d) is smaller than video(%dx%d)", *newPreviewW, *newPreviewH, videoW, videoH);

                /* If the video ratio is differ with preview ratio,
                   the default ratio is set into preview ratio */
                if (SIZE_RATIO(*newPreviewW, *newPreviewH) != SIZE_RATIO(videoW, videoH))
                    CLOGW2("preview ratio(%dx%d) is not matched with video ratio(%dx%d)",
                            *newPreviewW, *newPreviewH, videoW, videoH);

                if (m_isSupportedPreviewSize(*newPreviewW, *newPreviewH) == false) {
                    CLOGE2("new preview size is invalid(%dx%d)", *newPreviewW, *newPreviewH);
                    return BAD_VALUE;
                }

                /*
                 * This call is to get real preview size.
                 * so, HW dis size must not be added.
                 */
                m_getPreviewBdsSize(&bdsRect);

                *newCalHwPreviewW = bdsRect.w;
                *newCalHwPreviewH = bdsRect.h;
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

#ifdef USE_CAMERA2_API_SUPPORT
#if defined(ENABLE_FULL_FRAME)
    ExynosRect bdsRect;
    getPreviewBdsSize(&bdsRect);
    *newCalHwPreviewW = bdsRect.w;
    *newCalHwPreviewH = bdsRect.h;
#else
    /* 1. try to get exact ratio */
    if (m_isSupportedPreviewSize(*newPreviewW, *newPreviewH) == false) {
        CLOGE("ERR(%s): new preview size is invalid(%dx%d)", "Parameters", newPreviewW, newPreviewH);
    }

#if 0
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
#endif
#endif

    return NO_ERROR;
}

bool ExynosCamera3Parameters::m_isSupportedPreviewSize(const int width,
                                                     const int height)
{
    int maxWidth, maxHeight = 0;
    int (*sizeList)[SIZE_OF_RESOLUTION];

    if (getHighResolutionCallbackMode() == true) {
        CLOGD2("Burst panorama mode start");
        m_cameraInfo.previewSizeRatioId = SIZE_RATIO_16_9;
        return true;
    }

    getMaxPreviewSize(&maxWidth, &maxHeight);

    if (maxWidth*maxHeight < width*height) {
        CLOGE2("invalid PreviewSize(maxSize(%d/%d) size(%d/%d)",
            maxWidth, maxHeight, width, height);
        return false;
    }

    if (getCameraId() == CAMERA_ID_BACK) {
        sizeList = m_staticInfo->rearPreviewList;
        for (int i = 0; i < m_staticInfo->rearPreviewListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;
            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                m_cameraInfo.previewSizeRatioId = sizeList[i][2];
                return true;
            }
        }
    } else {
        sizeList = m_staticInfo->frontPreviewList;
        for (int i = 0; i <  m_staticInfo->frontPreviewListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;
            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                m_cameraInfo.previewSizeRatioId = sizeList[i][2];
                return true;
            }
        }
    }

    if (getCameraId() == CAMERA_ID_BACK) {
        sizeList = m_staticInfo->hiddenRearPreviewList;
        for (int i = 0; i < m_staticInfo->hiddenRearPreviewListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;
            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                m_cameraInfo.previewSizeRatioId = sizeList[i][2];
                return true;
            }
        }
    } else {
        sizeList = m_staticInfo->hiddenFrontPreviewList;
        for (int i = 0; i < m_staticInfo->hiddenFrontPreviewListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;
            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                m_cameraInfo.previewSizeRatioId = sizeList[i][2];
                return true;
            }
        }
    }

    CLOGE2("Invalid preview size(%dx%d)", width, height);

    return false;
}

status_t ExynosCamera3Parameters::m_getPreviewSizeList(int *sizeList)
{
    int *tempSizeList = NULL;
    int configMode = -1;

    if (getHalVersion() == IS_HAL_VER_3_2) {
        /* CAMERA2_API use Video Scenario LUT as a default */
        if (m_staticInfo->videoSizeLut == NULL) {
            CLOGE2("videoSizeLut is NULL");
            return INVALID_OPERATION;
        } else if (m_staticInfo->videoSizeLutMax <= m_cameraInfo.previewSizeRatioId) {
            CLOGE2("unsupported video ratioId(%d)", m_cameraInfo.previewSizeRatioId);
            return BAD_VALUE;
        }
#if defined(ENABLE_FULL_FRAME)
        tempSizeList = m_staticInfo->videoSizeLut[m_cameraInfo.previewSizeRatioId];
#else
        configMode = this->getConfigMode();
        switch (configMode) {
        case CONFIG_MODE::NORMAL:
            tempSizeList = m_staticInfo->previewSizeLut[m_cameraInfo.previewSizeRatioId];
            break;
        case CONFIG_MODE::HIGHSPEED_120:
            tempSizeList = m_staticInfo->videoSizeLutHighSpeed120[configMode-2];
            break;
        }
#endif
    } else {
    if (getDualMode() == true) {
        if (getDualRecordingHint() == true
            && m_staticInfo->dualVideoSizeLut != NULL
            && m_cameraInfo.previewSizeRatioId < m_staticInfo->videoSizeLutMax) {
            tempSizeList = m_staticInfo->dualVideoSizeLut[m_cameraInfo.previewSizeRatioId];
        } else if (m_staticInfo->dualPreviewSizeLut != NULL
                   && m_cameraInfo.previewSizeRatioId < m_staticInfo->previewSizeLutMax) {
            tempSizeList = m_staticInfo->dualPreviewSizeLut[m_cameraInfo.previewSizeRatioId];
        } else { /* Use Preview LUT as a default */
            if (m_staticInfo->previewSizeLut == NULL) {
                CLOGE2("previewSizeLut is NULL");
                return INVALID_OPERATION;
            } else if (m_staticInfo->previewSizeLutMax <= m_cameraInfo.previewSizeRatioId) {
                CLOGE2("unsupported preview ratioId(%d)", m_cameraInfo.previewSizeRatioId);
                return BAD_VALUE;
            }

            tempSizeList = m_staticInfo->previewSizeLut[m_cameraInfo.previewSizeRatioId];
        }
    } else { /* getDualMode() == false */
        if (getRecordingHint() == true) {
            int videoW = 0, videoH = 0;
            getVideoSize(&videoW, &videoH);
            if (getHighSpeedRecording() == true) {
                int fpsmode = 0;
                fpsmode = getFastFpsMode();

                if (fpsmode <= 0) {
                    CLOGE2("getFastFpsMode fpsmode(%d) fail", fpsmode);
                    return BAD_VALUE;
                } else if (m_staticInfo->videoSizeLutHighSpeed == NULL) {
                    CLOGE2("videoSizeLutHighSpeed is NULL");
                    return INVALID_OPERATION;
                }

                fpsmode--;
                tempSizeList = m_staticInfo->videoSizeLutHighSpeed[fpsmode];
            }
#ifdef USE_BNS_RECORDING
            else if (m_staticInfo->videoSizeBnsLut != NULL
                     && videoW == 1920 && videoH == 1080) { /* Use BNS Recording only for FHD(16:9) */
                if (m_staticInfo->videoSizeLutMax <= m_cameraInfo.previewSizeRatioId) {
                    CLOGE2("unsupported video ratioId(%d)", m_cameraInfo.previewSizeRatioId);
                    return BAD_VALUE;
                }

                tempSizeList = m_staticInfo->videoSizeBnsLut[m_cameraInfo.previewSizeRatioId];
            }
#endif
            else { /* Normal Recording Mode */
                if (m_staticInfo->videoSizeLut == NULL) {
                    CLOGE2("videoSizeLut is NULL");
                    return INVALID_OPERATION;
                } else if (m_staticInfo->videoSizeLutMax <= m_cameraInfo.previewSizeRatioId) {
                    CLOGE2("unsupported video ratioId(%d)", m_cameraInfo.previewSizeRatioId);
                    return BAD_VALUE;
                }

                tempSizeList = m_staticInfo->videoSizeLut[m_cameraInfo.previewSizeRatioId];
            }
        }
#ifdef USE_BINNING_MODE
            else if (getBinningMode() == true) {
            /*
             * VT mode
             *   1: 3G vtmode (176x144, Fixed 7fps)
             *   2: LTE or WIFI vtmode (640x480, Fixed 15fps)
             */
            int index = 0;
            if (m_staticInfo->vtcallSizeLut == NULL
                || m_staticInfo->vtcallSizeLutMax == 0) {
                CLOGE2("vtcallSizeLut is NULL");
                return INVALID_OPERATION;
            }

            for (index = 0; index < m_staticInfo->vtcallSizeLutMax; index++) {
                if (m_staticInfo->vtcallSizeLut[index][0] == m_cameraInfo.previewSizeRatioId)
                    break;
            }

            if (m_staticInfo->vtcallSizeLutMax <= index)
                index = 0;

            tempSizeList = m_staticInfo->vtcallSizeLut[index];
        }
#endif
        else { /* Use Preview LUT */
            if (m_staticInfo->previewSizeLut == NULL) {
                CLOGE2("previewSizeLut is NULL");
                return INVALID_OPERATION;
            } else if (m_staticInfo->previewSizeLutMax <= m_cameraInfo.previewSizeRatioId) {
                CLOGE2("unsupported preview ratioId(%d)", m_cameraInfo.previewSizeRatioId);
                return BAD_VALUE;
            }

            tempSizeList = m_staticInfo->previewSizeLut[m_cameraInfo.previewSizeRatioId];
        }
    }
    }

    if (tempSizeList == NULL) {
        CLOGE2("fail to get LUT");
        return INVALID_OPERATION;
    }

    for (int i = 0; i < SIZE_LUT_INDEX_END; i++)
        sizeList[i] = tempSizeList[i];

    return NO_ERROR;
}

void ExynosCamera3Parameters::m_getSWVdisPreviewSize(int w, int h, int *newW, int *newH)
{
    if (w < 0 || h < 0) {
        return;
    }

    if (w == 1920 && h == 1080) {
        *newW = 2304;
        *newH = 1296;
    }
    else if (w == 1280 && h == 720) {
        *newW = 1536;
        *newH = 864;
    }
    else {
        *newW = ALIGN_UP((w * 6) / 5, CAMERA_ISP_ALIGN);
        *newH = ALIGN_UP((h * 6) / 5, CAMERA_ISP_ALIGN);
    }
}

bool ExynosCamera3Parameters::m_isHighResolutionCallbackSize(const int width, const int height)
{
    bool highResolutionCallbackMode;

    if (width == m_staticInfo->highResolutionCallbackW && height == m_staticInfo->highResolutionCallbackH)
        highResolutionCallbackMode = true;
    else
        highResolutionCallbackMode = false;

    CLOGD("DEBUG(%s):highResolutionCallSize:%s", "setParameters",
        highResolutionCallbackMode == true? "on":"off");

    m_setHighResolutionCallbackMode(highResolutionCallbackMode);

    return highResolutionCallbackMode;
}

void ExynosCamera3Parameters::m_isHighResolutionMode(const CameraParameters& params)
{
    bool highResolutionCallbackMode;
    int shotmode = params.getInt("shot-mode");

    if ((getRecordingHint() == false) && (shotmode == SHOT_MODE_PANORAMA))
        highResolutionCallbackMode = true;
    else
        highResolutionCallbackMode = false;

    CLOGD("DEBUG(%s):highResolutionMode:%s", "setParameters",
        highResolutionCallbackMode == true? "on":"off");

    m_setHighResolutionCallbackMode(highResolutionCallbackMode);
}

void ExynosCamera3Parameters::m_setHighResolutionCallbackMode(bool enable)
{
    m_cameraInfo.highResolutionCallbackMode = enable;
}

bool ExynosCamera3Parameters::getHighResolutionCallbackMode(void)
{
    return m_cameraInfo.highResolutionCallbackMode;
}

status_t ExynosCamera3Parameters::m_adjustPreviewFormat(__unused int &previewFormat, int &hwPreviewFormat)
{
#if 1
    /* HACK : V4L2_PIX_FMT_NV21M is set to FIMC-IS  *
     * and Gralloc. V4L2_PIX_FMT_YVU420 is just     *
     * color format for callback frame.             */
    hwPreviewFormat = V4L2_PIX_FMT_NV21M;
#else
    if (previewFormat == V4L2_PIX_FMT_NV21)
        hwPreviewFormat = V4L2_PIX_FMT_NV21M;
    else if (previewFormat == V4L2_PIX_FMT_YVU420)
        hwPreviewFormat = V4L2_PIX_FMT_YVU420M;
#endif

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::checkYuvFormat(const int format, const int outputPortId)
{
    status_t ret = NO_ERROR;
    int curYuvFormat = -1;
    int newYuvFormat = -1;

    newYuvFormat = HAL_PIXEL_FORMAT_2_V4L2_PIX(format);
    curYuvFormat = getYuvFormat(outputPortId);

    if (newYuvFormat != curYuvFormat) {
        char curFormatName[V4L2_FOURCC_LENGTH] = {};
        char newFormatName[V4L2_FOURCC_LENGTH] = {};
        m_getV4l2Name(curFormatName, V4L2_FOURCC_LENGTH, curYuvFormat);
        m_getV4l2Name(newFormatName, V4L2_FOURCC_LENGTH, newYuvFormat);
        CLOGI("INFO(%s[%d]):curYuvFormat %s newYuvFormat %s outputPortId %d",
                __FUNCTION__, __LINE__,
                curFormatName, newFormatName, outputPortId);
        m_setYuvFormat(newYuvFormat, outputPortId);
    }

    return ret;
}

void ExynosCamera3Parameters::m_setPreviewSize(int w, int h)
{
    m_cameraInfo.previewW = w;
    m_cameraInfo.previewH = h;
}

void ExynosCamera3Parameters::getPreviewSize(int *w, int *h)
{
    *w = m_cameraInfo.previewW;
    *h = m_cameraInfo.previewH;
}

void ExynosCamera3Parameters::m_setYuvSize(const int width, const int height, const int index)
{
    m_cameraInfo.yuvWidth[index] = width;
    m_cameraInfo.yuvHeight[index] = height;
}

void ExynosCamera3Parameters::getYuvSize(int *width, int *height, const int index)
{
    *width = m_cameraInfo.yuvWidth[index];
    *height = m_cameraInfo.yuvHeight[index];
}

void ExynosCamera3Parameters::getMaxSensorSize(int *w, int *h)
{
    *w = m_staticInfo->maxSensorW;
    *h = m_staticInfo->maxSensorH;
}

void ExynosCamera3Parameters::getSensorMargin(int *w, int *h)
{
    *w = m_staticInfo->sensorMarginW;
    *h = m_staticInfo->sensorMarginH;
}

void ExynosCamera3Parameters::m_adjustSensorMargin(int *sensorMarginW, int *sensorMarginH)
{
    float bnsRatio = 1.00f;
    float binningRatio = 1.00f;
    float sensorMarginRatio = 1.00f;

    bnsRatio = (float)getBnsScaleRatio() / 1000.00f;
    binningRatio = (float)getBinningScaleRatio() / 1000.00f;
    sensorMarginRatio = bnsRatio * binningRatio;
    if ((int)sensorMarginRatio < 1) {
        CLOGW2("Invalid sensor margin ratio(%f), bnsRatio(%f), binningRatio(%f)",
                sensorMarginRatio, bnsRatio, binningRatio);
        sensorMarginRatio = 1.00f;
    }

    if (getHalVersion() != IS_HAL_VER_3_2) {
        *sensorMarginW = ALIGN_DOWN((int)(*sensorMarginW / sensorMarginRatio), 2);
        *sensorMarginH = ALIGN_DOWN((int)(*sensorMarginH / sensorMarginRatio), 2);
    } else {
       int leftMargin = 0, rightMargin = 0, topMargin = 0, bottomMargin = 0;

       rightMargin = ALIGN_DOWN((int)(m_staticInfo->sensorMarginBase[WIDTH_BASE] / sensorMarginRatio), 2);
       leftMargin = m_staticInfo->sensorMarginBase[LEFT_BASE] + rightMargin;
       bottomMargin = ALIGN_DOWN((int)(m_staticInfo->sensorMarginBase[HEIGHT_BASE] / sensorMarginRatio), 2);
       topMargin = m_staticInfo->sensorMarginBase[TOP_BASE] + bottomMargin;

       *sensorMarginW = leftMargin + rightMargin;
       *sensorMarginH = topMargin + bottomMargin;
    }
}

void ExynosCamera3Parameters::getMaxPreviewSize(int *w, int *h)
{
    *w = m_staticInfo->maxPreviewW;
    *h = m_staticInfo->maxPreviewH;
}

int ExynosCamera3Parameters::getBayerFormat(int pipeId)
{
    int bayerFormat = V4L2_PIX_FMT_SBGGR16;

    switch (pipeId) {
    case PIPE_FLITE:
    case PIPE_3AA_REPROCESSING:
        bayerFormat = V4L2_PIX_FMT_SBGGR16;
        break;
    case PIPE_3AA:
    case PIPE_FLITE_REPROCESSING:
        bayerFormat = V4L2_PIX_FMT_SBGGR12;
        break;
    case PIPE_3AC:
    case PIPE_3AP:
    case PIPE_ISP:
    case PIPE_3AC_REPROCESSING:
    case PIPE_3AP_REPROCESSING:
    case PIPE_ISP_REPROCESSING:
        bayerFormat = V4L2_PIX_FMT_SBGGR10;
        break;
    default:
        CLOGW("WRN(%s[%d]):Invalid pipeId(%d)", __FUNCTION__, __LINE__, pipeId);
        break;
    }

#ifndef CAMERA_PACKED_BAYER_ENABLE
    bayerFormat = V4L2_PIX_FMT_SBGGR16;
#endif

    return bayerFormat;
}

void ExynosCamera3Parameters::m_setPreviewFormat(int fmt)
{
    m_cameraInfo.previewFormat = fmt;
}

void ExynosCamera3Parameters::m_setYuvFormat(const int format, const int index)
{
    m_cameraInfo.yuvFormat[index] = format;
}

int ExynosCamera3Parameters::getPreviewFormat(void)
{
    return m_cameraInfo.previewFormat;
}

int ExynosCamera3Parameters::getYuvFormat(const int index)
{
    return m_cameraInfo.yuvFormat[index];
}

void ExynosCamera3Parameters::m_setHwPreviewSize(int w, int h)
{
    m_cameraInfo.hwPreviewW = w;
    m_cameraInfo.hwPreviewH = h;
}

void ExynosCamera3Parameters::getHwPreviewSize(int *w, int *h)
{
    if (m_cameraInfo.scalableSensorMode != true) {
        *w = m_cameraInfo.hwPreviewW;
        *h = m_cameraInfo.hwPreviewH;
    } else {
        int newSensorW  = 0;
        int newSensorH = 0;
        m_getScalableSensorSize(&newSensorW, &newSensorH);

        *w = newSensorW;
        *h = newSensorH;
/*
 *    Should not use those value
 *    *w = 1024;
 *    *h = 768;
 *    *w = 1440;
 *    *h = 1080;
 */
        *w = m_cameraInfo.hwPreviewW;
        *h = m_cameraInfo.hwPreviewH;
    }
}

void ExynosCamera3Parameters::setHwPreviewStride(int stride)
{
    m_cameraInfo.previewStride = stride;
}

int ExynosCamera3Parameters::getHwPreviewStride(void)
{
    return m_cameraInfo.previewStride;
}

void ExynosCamera3Parameters::m_setHwPreviewFormat(int fmt)
{
    m_cameraInfo.hwPreviewFormat = fmt;
}

int ExynosCamera3Parameters::getHwPreviewFormat(void)
{
    return m_cameraInfo.hwPreviewFormat;
}

void ExynosCamera3Parameters::updateHwSensorSize(void)
{
    int curHwSensorW = 0;
    int curHwSensorH = 0;
    int newHwSensorW = 0;
    int newHwSensorH = 0;
    int maxHwSensorW = 0;
    int maxHwSensorH = 0;

    getHwSensorSize(&newHwSensorW, &newHwSensorH);
    getMaxSensorSize(&maxHwSensorW, &maxHwSensorH);

    if (newHwSensorW > maxHwSensorW || newHwSensorH > maxHwSensorH) {
        CLOGE2("Invalid sensor size (maxSize(%d/%d) size(%d/%d)",
                maxHwSensorW, maxHwSensorH, newHwSensorW, newHwSensorH);
    }

    if (getHighSpeedRecording() == true) {
#if 0
        int sizeList[SIZE_LUT_INDEX_END];
        m_getHighSpeedRecordingSize(sizeList);
        newHwSensorW = sizeList[SENSOR_W];
        newHwSensorH = sizeList[SENSOR_H];
#endif
    } else if (getScalableSensorMode() == true) {
        m_getScalableSensorSize(&newHwSensorW, &newHwSensorH);
    } else {
        getBnsSize(&newHwSensorW, &newHwSensorH);
    }

    getHwSensorSize(&curHwSensorW, &curHwSensorH);
    CLOGI2("curHwSensor size(%dx%d) newHwSensor size(%dx%d)", curHwSensorW, curHwSensorH, newHwSensorW, newHwSensorH);
    if (curHwSensorW != newHwSensorW || curHwSensorH != newHwSensorH) {
        m_setHwSensorSize(newHwSensorW, newHwSensorH);
        CLOGI2("newHwSensor size(%dx%d)", newHwSensorW, newHwSensorH);
    }
}

void ExynosCamera3Parameters::m_setHwSensorSize(int w, int h)
{
    m_cameraInfo.hwSensorW = w;
    m_cameraInfo.hwSensorH = h;
}

void ExynosCamera3Parameters::getHwSensorSize(int *w, int *h)
{
    CLOGV2("getScalableSensorMode()(%d)", getScalableSensorMode());
    int width  = 0;
    int height = 0;
    int sizeList[SIZE_LUT_INDEX_END];

    if (m_cameraInfo.scalableSensorMode != true) {
        /* matched ratio LUT is not existed, use equation */
        if (m_useSizeTable == true
            && m_staticInfo->previewSizeLut != NULL
            && m_cameraInfo.previewSizeRatioId < m_staticInfo->previewSizeLutMax
            && m_getPreviewSizeList(sizeList) == NO_ERROR) {

            width = sizeList[SENSOR_W];
            height = sizeList[SENSOR_H];

        } else {
            width  = m_cameraInfo.hwSensorW;
            height = m_cameraInfo.hwSensorH;
        }
    } else {
        m_getScalableSensorSize(&width, &height);
    }

    *w = width;
    *h = height;
}

void ExynosCamera3Parameters::updateBnsScaleRatio(void)
{
    int ret = 0;
    uint32_t bnsRatio = DEFAULT_BNS_RATIO * 1000;
    int curPreviewW = 0, curPreviewH = 0;

    if (m_staticInfo->bnsSupport == false)
        return;

    getPreviewSize(&curPreviewW, &curPreviewH);
    if (getDualMode() == true) {
#if defined(USE_BNS_DUAL_PREVIEW) || defined(USE_BNS_DUAL_RECORDING)
        bnsRatio = 2000;
#endif
    } else if ((getRecordingHint() == true)
/*    || (curPreviewW == curPreviewH)*/) {
#ifdef USE_BNS_RECORDING
        int videoW = 0, videoH = 0;
        getVideoSize(&videoW, &videoH);

        if ((getHighSpeedRecording() == true)) {
            bnsRatio = 1000;
        } else if (videoW == 1920 && videoH == 1080) {
            bnsRatio = 1500;
            CLOGI2("bnsRatio(%d), videoSize (%d, %d)", bnsRatio, videoW, videoH);
        } else
#endif
        {
            bnsRatio = 1000;
        }
        if (bnsRatio != getBnsScaleRatio()) {
            CLOGI2("restart set due to changing  bnsRatio(%d/%d)", bnsRatio, getBnsScaleRatio());
            m_setRestartPreview(true);
        }
    }
#ifdef USE_BINNING_MODE
    else if (getBinningMode() == true) {
        bnsRatio = 1000;
    }
#endif

    if (bnsRatio != getBnsScaleRatio())
        ret = m_setBnsScaleRatio(bnsRatio);

    if (ret < 0)
        CLOGE2("Cannot update BNS scale ratio(%d)", bnsRatio);
}

status_t ExynosCamera3Parameters::m_setBnsScaleRatio(int ratio)
{
#define MIN_BNS_RATIO 1000
#define MAX_BNS_RATIO 8000

    if (m_staticInfo->bnsSupport == false) {
        CLOGD2("This camera does not support BNS");
        ratio = MIN_BNS_RATIO;
    }

    if (ratio < MIN_BNS_RATIO || ratio > MAX_BNS_RATIO) {
        CLOGE2("Out of bound, ratio(%d), min:max(%d:%d)", ratio, MAX_BNS_RATIO, MAX_BNS_RATIO);
        return BAD_VALUE;
    }

    CLOGD2("update BNS ratio(%d -> %d)", m_cameraInfo.bnsScaleRatio, ratio);

    m_cameraInfo.bnsScaleRatio = ratio;

    /* When BNS scale ratio is changed, reset BNS size to MAX sensor size */
    getMaxSensorSize(&m_cameraInfo.bnsW, &m_cameraInfo.bnsH);

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::m_addHiddenResolutionList(String8 &string8Buf,
                                                            __unused struct ExynosSensorInfoBase *sensorInfo,
                                                            int w, int h, enum MODE mode, int cameraId)

{
    status_t ret = NO_ERROR;
    bool found = false;

    int (*sizeList)[SIZE_OF_RESOLUTION];
    int listSize = 0;

    switch (mode) {
    case MODE_PREVIEW:
        if (cameraId == CAMERA_ID_BACK) {
            sizeList = m_staticInfo->hiddenRearPreviewList;
            listSize = m_staticInfo->hiddenRearPreviewListMax;
        } else {
            sizeList = m_staticInfo->hiddenFrontPreviewList;
            listSize = m_staticInfo->hiddenFrontPreviewListMax;
        }
        break;
    case MODE_PICTURE:
        if (cameraId == CAMERA_ID_BACK) {
            sizeList = m_staticInfo->hiddenRearPictureList;
            listSize = m_staticInfo->hiddenRearPictureListMax;
        } else {
            sizeList = m_staticInfo->hiddenFrontPictureList;
            listSize = m_staticInfo->hiddenFrontPictureListMax;
        }
        break;
    case MODE_VIDEO:
        if (cameraId == CAMERA_ID_BACK) {
            sizeList = m_staticInfo->hiddenRearVideoList;
            listSize = m_staticInfo->hiddenRearVideoListMax;
        } else {
            sizeList = m_staticInfo->hiddenFrontVideoList;
            listSize = m_staticInfo->hiddenFrontVideoListMax;
        }
        break;
    default:
        CLOGE2("invalid mode(%d)", mode);
        return BAD_VALUE;
        break;
    }

    for (int i = 0; i < listSize; i++) {
        if (w == sizeList[i][0] && h == sizeList[i][1]) {
            found = true;
            break;
        }
    }

    if (found == true) {
        String8 uhdTempStr;
        char strBuf[32];

        snprintf(strBuf, sizeof(strBuf), "%dx%d,", w, h);

        /* append on head of string8Buf */
        uhdTempStr.setTo(strBuf);
        uhdTempStr.append(string8Buf);
        string8Buf.setTo(uhdTempStr);
    } else {
        ret = INVALID_OPERATION;
    }

    return ret;
}

uint32_t ExynosCamera3Parameters::getBnsScaleRatio(void)
{
    return m_cameraInfo.bnsScaleRatio;
}

void ExynosCamera3Parameters::setBnsSize(int w, int h)
{
    m_cameraInfo.bnsW = w;
    m_cameraInfo.bnsH = h;

    updateHwSensorSize();

#if 0
    int zoom = getZoomLevel();
    int previewW = 0, previewH = 0;
    getPreviewSize(&previewW, &previewH);
    if (m_setParamCropRegion(zoom, w, h, previewW, previewH) != NO_ERROR)
        CLOGE2("m_setParamCropRegion() fail");
#else
    ExynosRect srcRect, dstRect;
    getPreviewBayerCropSize(&srcRect, &dstRect);
#endif
}

void ExynosCamera3Parameters::getBnsSize(int *w, int *h)
{
    *w = m_cameraInfo.bnsW;
    *h = m_cameraInfo.bnsH;
}

void ExynosCamera3Parameters::updateBinningScaleRatio(void)
{
    int ret = 0;
    uint32_t binningRatio = DEFAULT_BINNING_RATIO * 1000;

    if ((getRecordingHint() == true)
        && (getHighSpeedRecording() == true)) {
        int fpsmode = 0;
        fpsmode = getFastFpsMode();
        switch (fpsmode) {
        case 1: /* 60 fps */
            binningRatio = 2000;
            break;
        case 2: /* 120 fps */
        case 3: /* 240 fps */
            binningRatio = 4000;
            break;
        default:
            CLOGE2("Invalide FastFpsMode(%d)", fpsmode);
        }
    }
#ifdef USE_BINNING_MODE
    else if (getBinningMode() == true) {
        binningRatio = 2000;
    }
#endif

    if (binningRatio != getBinningScaleRatio()) {
        CLOGI2("New sensor binning ratio(%d)", binningRatio);
        ret = m_setBinningScaleRatio(binningRatio);
    }

    if (ret < 0)
        CLOGE2("Cannot update BNS scale ratio(%d)", binningRatio);
}

status_t ExynosCamera3Parameters::m_setBinningScaleRatio(int ratio)
{
#define MIN_BINNING_RATIO 1000
#define MAX_BINNING_RATIO 6000

    if (ratio < MIN_BINNING_RATIO || ratio > MAX_BINNING_RATIO) {
        CLOGE2("Out of bound, ratio(%d), min:max(%d:%d)", ratio, MAX_BINNING_RATIO, MAX_BINNING_RATIO);
        return BAD_VALUE;
    }

    m_cameraInfo.binningScaleRatio = ratio;

    return NO_ERROR;
}

uint32_t ExynosCamera3Parameters::getBinningScaleRatio(void)
{
    return m_cameraInfo.binningScaleRatio;
}
#if 0
status_t ExynosCamera3Parameters::checkPictureSize(const CameraParameters& params)
{
    int curPictureW = 0;
    int curPictureH = 0;
    int newPictureW = 0;
    int newPictureH = 0;
    int curHwPictureW = 0;
    int curHwPictureH = 0;
    int newHwPictureW = 0;
    int newHwPictureH = 0;
    int right_ratio = 177;

    params.getPictureSize(&newPictureW, &newPictureH);

    if (newPictureW < 0 || newPictureH < 0) {
        return BAD_VALUE;
    }

    if (m_adjustPictureSize(&newPictureW, &newPictureH, &newHwPictureW, &newHwPictureH) != NO_ERROR) {
        return BAD_VALUE;
    }

    if (m_isSupportedPictureSize(newPictureW, newPictureH) == false) {
        int maxHwPictureW =0;
        int maxHwPictureH = 0;

        CLOGE2("Invalid picture size(%dx%d)", newPictureW, newPictureH);

        /* prevent wrong size setting */
        getMaxPictureSize(&maxHwPictureW, &maxHwPictureH);
        m_setPictureSize(maxHwPictureW, maxHwPictureH);
        m_setHwPictureSize(maxHwPictureW, maxHwPictureH);
        m_params.setPictureSize(maxHwPictureW, maxHwPictureH);
        CLOGE2("changed picture size to MAX(%dx%d)", maxHwPictureW, maxHwPictureH);

#ifdef FIXED_SENSOR_SIZE
        updateHwSensorSize();
#endif
        return INVALID_OPERATION;
    }
    CLOGI("INFO(%s):newPicture Size (%dx%d), ratioId(%d)",
        "setParameters", newPictureW, newPictureH, m_cameraInfo.pictureSizeRatioId);

    if ((int)(m_staticInfo->maxSensorW * 100 / m_staticInfo->maxSensorH) == right_ratio) {
        setHorizontalViewAngle(newPictureW, newPictureH);
    }
    m_params.setFloat(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, getHorizontalViewAngle());

    getPictureSize(&curPictureW, &curPictureH);
    getHwPictureSize(&curHwPictureW, &curHwPictureH);

    if (curPictureW != newPictureW || curPictureH != newPictureH ||
        curHwPictureW != newHwPictureW || curHwPictureH != newHwPictureH) {

        CLOGI("INFO(%s[%d]): Picture size changed: cur(%dx%d) -> new(%dx%d)",
                "setParameters", __LINE__, curPictureW, curPictureH, newPictureW, newPictureH);
        CLOGI("INFO(%s[%d]): HwPicture size changed: cur(%dx%d) -> new(%dx%d)",
                "setParameters", __LINE__, curHwPictureW, curHwPictureH, newHwPictureW, newHwPictureH);

        m_setPictureSize(newPictureW, newPictureH);
        m_setHwPictureSize(newHwPictureW, newHwPictureH);
        m_params.setPictureSize(newPictureW, newPictureH);

#ifdef FIXED_SENSOR_SIZE
        updateHwSensorSize();
#endif
    }

    return NO_ERROR;
}
#else
status_t ExynosCamera3Parameters::checkPictureSize(int newPictureW, int newPictureH)
{
    int curPictureW = 0;
    int curPictureH = 0;
    int curHwPictureW = 0;
    int curHwPictureH = 0;
    int newHwPictureW = 0;
    int newHwPictureH = 0;

//    params.getPictureSize(&newPictureW, &newPictureH);

    if (newPictureW < 0 || newPictureH < 0) {
        return BAD_VALUE;
    }

    if (m_adjustPictureSize(&newPictureW, &newPictureH, &newHwPictureW, &newHwPictureH) != NO_ERROR) {
        return BAD_VALUE;
    }

    if (m_isSupportedPictureSize(newPictureW, newPictureH) == false) {
        int maxHwPictureW =0;
        int maxHwPictureH = 0;

        CLOGE2("Invalid picture size(%dx%d)", newPictureW, newPictureH);

        /* prevent wrong size setting */
        getMaxPictureSize(&maxHwPictureW, &maxHwPictureH);
        m_setPictureSize(maxHwPictureW, maxHwPictureH);
        m_setHwPictureSize(maxHwPictureW, maxHwPictureH);

//        m_params.setPictureSize(maxHwPictureW, maxHwPictureH);

        CLOGE2("changed picture size to MAX(%dx%d)", maxHwPictureW, maxHwPictureH);

#ifdef FIXED_SENSOR_SIZE
        updateHwSensorSize();
#endif
        return INVALID_OPERATION;
    }
    CLOGI("INFO(%s):newPicture Size (%dx%d), ratioId(%d)",
        "setParameters", newPictureW, newPictureH, m_cameraInfo.pictureSizeRatioId);

    getPictureSize(&curPictureW, &curPictureH);
    getHwPictureSize(&curHwPictureW, &curHwPictureH);

    if (curPictureW != newPictureW || curPictureH != newPictureH ||
        curHwPictureW != newHwPictureW || curHwPictureH != newHwPictureH) {

        CLOGI("INFO(%s[%d]): Picture size changed: cur(%dx%d) -> new(%dx%d)",
                "setParameters", __LINE__, curPictureW, curPictureH, newPictureW, newPictureH);
        CLOGI("INFO(%s[%d]): HwPicture size changed: cur(%dx%d) -> new(%dx%d)",
                "setParameters", __LINE__, curHwPictureW, curHwPictureH, newHwPictureW, newHwPictureH);

        m_setPictureSize(newPictureW, newPictureH);
        m_setHwPictureSize(newHwPictureW, newHwPictureH);

//        m_params.setPictureSize(newPictureW, newPictureH);

#ifdef FIXED_SENSOR_SIZE
        updateHwSensorSize();
#endif
    }

    return NO_ERROR;
}
#endif

status_t ExynosCamera3Parameters::m_adjustPictureSize(int *newPictureW, int *newPictureH,
                                                 int *newHwPictureW, int *newHwPictureH)
{
    int ret = 0;
    int newX = 0, newY = 0, newW = 0, newH = 0;
    float zoomRatio = getZoomRatio(0) / 1000;

    if ((getRecordingHint() == true && getHighSpeedRecording() == true)
#ifdef USE_BINNING_MODE
        || getBinningMode()
#endif
       )
    {
       int sizeList[SIZE_LUT_INDEX_END];
       if (m_getPreviewSizeList(sizeList) == NO_ERROR) {
           *newPictureW = sizeList[TARGET_W];
           *newPictureH = sizeList[TARGET_H];
           *newHwPictureW = *newPictureW;
           *newHwPictureH = *newPictureH;

           return NO_ERROR;
       } else {
           CLOGE2("m_getPreviewSizeList() fail");
           return BAD_VALUE;
       }
    }

    getMaxPictureSize(newHwPictureW, newHwPictureH);

    if (getCameraId() == CAMERA_ID_BACK) {
        ret = getCropRectAlign(*newHwPictureW, *newHwPictureH,
                *newPictureW, *newPictureH,
                &newX, &newY, &newW, &newH,
                CAMERA_ISP_ALIGN, 2, 0, zoomRatio);
        if (ret < 0) {
            CLOGE2("getCropRectAlign(%d, %d, %d, %d) fail", *newHwPictureW, *newHwPictureH, *newPictureW, *newPictureH);
            return BAD_VALUE;
        }
        *newHwPictureW = newW;
        *newHwPictureH = newH;

#ifdef FIXED_SENSOR_SIZE
        /*
         * sensor crop size:
         * sensor crop is only used at 16:9 aspect ratio in picture size.
         */
        if (getSamsungCamera() == true) {
            if (((float)*newPictureW / (float)*newPictureH) == ((float)16 / (float)9)) {
                CLOGD2("Use sensor crop (ratio: %f)", ((float)*newPictureW / (float)*newPictureH));
                m_setHwSensorSize(newW, newH);
            }
        }
#endif
    }

    return NO_ERROR;
}

bool ExynosCamera3Parameters::m_isSupportedPictureSize(const int width,
                                                     const int height)
{
    int maxWidth, maxHeight = 0;
    int (*sizeList)[SIZE_OF_RESOLUTION];

    getMaxPictureSize(&maxWidth, &maxHeight);

    if (maxWidth < width || maxHeight < height) {
        CLOGE2("invalid picture Size(maxSize(%d/%d) size(%d/%d)",
            maxWidth, maxHeight, width, height);
        return false;
    }

    if (getCameraId() == CAMERA_ID_BACK) {
        sizeList = m_staticInfo->rearPictureList;
        for (int i = 0; i < m_staticInfo->rearPictureListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;
            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                m_cameraInfo.pictureSizeRatioId = sizeList[i][2];
                return true;
            }
        }
    } else {
        sizeList = m_staticInfo->frontPictureList;
        for (int i = 0; i < m_staticInfo->frontPictureListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;
            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                m_cameraInfo.pictureSizeRatioId = sizeList[i][2];
                return true;
            }
        }
    }

    if (getCameraId() == CAMERA_ID_BACK) {
        sizeList = m_staticInfo->hiddenRearPictureList;
        for (int i = 0; i < m_staticInfo->hiddenRearPictureListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;
            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                m_cameraInfo.pictureSizeRatioId = sizeList[i][2];
                return true;
            }
        }
    } else {
        sizeList = m_staticInfo->hiddenFrontPictureList;
        for (int i = 0; i < m_staticInfo->hiddenFrontPictureListMax; i++) {
            if (sizeList[i][0] > maxWidth || sizeList[i][1] > maxHeight)
                continue;
            if (sizeList[i][0] == width && sizeList[i][1] == height) {
                m_cameraInfo.pictureSizeRatioId = sizeList[i][2];
                return true;
            }
        }
    }

    CLOGE2("Invalid picture size(%dx%d)", width, height);

    return false;
}

void ExynosCamera3Parameters::m_setPictureSize(int w, int h)
{
    m_cameraInfo.pictureW = w;
    m_cameraInfo.pictureH = h;
}

void ExynosCamera3Parameters::getPictureSize(int *w, int *h)
{
    *w = m_cameraInfo.pictureW;
    *h = m_cameraInfo.pictureH;
}

void ExynosCamera3Parameters::getMaxPictureSize(int *w, int *h)
{
    *w = m_staticInfo->maxPictureW;
    *h = m_staticInfo->maxPictureH;
}

void ExynosCamera3Parameters::m_setHwPictureSize(int w, int h)
{
    m_cameraInfo.hwPictureW = w;
    m_cameraInfo.hwPictureH = h;
}

void ExynosCamera3Parameters::getHwPictureSize(int *w, int *h)
{
    *w = m_cameraInfo.hwPictureW;
    *h = m_cameraInfo.hwPictureH;
}

void ExynosCamera3Parameters::m_setHwBayerCropRegion(int w, int h, int x, int y)
{
    Mutex::Autolock lock(m_parameterLock);

    m_cameraInfo.hwBayerCropW = w;
    m_cameraInfo.hwBayerCropH = h;
    m_cameraInfo.hwBayerCropX = x;
    m_cameraInfo.hwBayerCropY = y;
}

void ExynosCamera3Parameters::getHwBayerCropRegion(int *w, int *h, int *x, int *y)
{
    Mutex::Autolock lock(m_parameterLock);

    *w = m_cameraInfo.hwBayerCropW;
    *h = m_cameraInfo.hwBayerCropH;
    *x = m_cameraInfo.hwBayerCropX;
    *y = m_cameraInfo.hwBayerCropY;
}

void ExynosCamera3Parameters::getHwVraInputSize(int *w, int *h)
{
#if defined(MAX_VRA_INPUT_SIZE_WIDTH) && defined(MAX_VRA_INPUT_SIZE_HEIGHT)
    int vraWidth = MAX_VRA_INPUT_WIDTH;
    int vraHeight = MAX_VRA_INPUT_HEIGHT;
#else
    int vraWidth = 640;
    int vraHeight = 480;
#endif
    float vraRatio = ROUND_OFF(((float)vraWidth / (float)vraHeight), 2);

    switch (m_cameraInfo.previewSizeRatioId) {
    case SIZE_RATIO_16_9:
        *w = vraWidth;
        *h = ALIGN_UP((vraWidth / 16) * 9, 2);
        break;
    case SIZE_RATIO_4_3:
        *w = ALIGN_UP((vraHeight / 3) * 4, CAMERA_16PX_ALIGN);
        *h = vraHeight;
        break;
    case SIZE_RATIO_1_1:
        *w = vraHeight;
        *h = vraHeight;
        break;
    case SIZE_RATIO_3_2:
        if (vraRatio == 1.33f) { /* 4:3 */
            *w = vraWidth;
            *h = ALIGN_UP((vraWidth / 3) * 2, 2);
        } else if (vraRatio == 1.77f) { /* 16:9 */
            *w = ALIGN_UP((vraHeight / 2) * 3, CAMERA_16PX_ALIGN);
            *h = vraHeight;
        } else {
            *w = vraWidth;
            *h = vraHeight;
        }
        break;
    case SIZE_RATIO_5_4:
        *w = ALIGN_UP((vraHeight / 4) * 5, CAMERA_16PX_ALIGN);
        *h = vraHeight;
        break;
    case SIZE_RATIO_5_3:
        if (vraRatio == 1.33f) { /* 4:3 */
            *w = vraWidth;
            *h = ALIGN_UP((vraWidth / 5) * 3, 2);
        } else if (vraRatio == 1.77f) { /* 16:9 */
            *w = ALIGN_UP((vraHeight / 3) * 5, CAMERA_16PX_ALIGN);
            *h = vraHeight;
        } else {
            *w = vraWidth;
            *h = vraHeight;
        }
        break;
    case SIZE_RATIO_11_9:
        *w = ALIGN_UP((vraHeight / 9) * 11, CAMERA_16PX_ALIGN);
        *h = vraHeight;
        break;
    default:
        CLOGW2("Invalid size ratio(%d)", m_cameraInfo.previewSizeRatioId);

        *w = vraWidth;
        *h = vraHeight;
        break;
    }
}

int ExynosCamera3Parameters::getHwVraInputFormat(void)
{
#if defined(CAMERA_VRA_INPUT_FORMAT)
    return CAMERA_VRA_INPUT_FORMAT;
#else
    return V4L2_PIX_FMT_NV21;
#endif
}

void ExynosCamera3Parameters::m_setHwPictureFormat(int fmt)
{
    m_cameraInfo.hwPictureFormat = fmt;
}

int ExynosCamera3Parameters::getHwPictureFormat(void)
{
    CLOGE("INFO(%s):m_cameraInfo.pictureFormat(%d)", __FUNCTION__, m_cameraInfo.hwPictureFormat);

    return m_cameraInfo.hwPictureFormat;
}
status_t ExynosCamera3Parameters::checkJpegQuality(int quality)
{
    int curJpegQuality = -1;
    if (quality < 0 || quality > 100) {
        CLOGE("ERR(%s[%d]):Invalid JPEG quality %d.",
                __FUNCTION__, __LINE__, quality);
        return BAD_VALUE;
    }
    curJpegQuality = getJpegQuality();
    if (curJpegQuality != quality) {
        CLOGI("INFO(%s[%d]):curJpegQuality %d newJpegQuality %d",
                __FUNCTION__, __LINE__, curJpegQuality, quality);
        m_setJpegQuality(quality);
    }
    return NO_ERROR;
}

void ExynosCamera3Parameters::m_setJpegQuality(int quality)
{
    m_cameraInfo.jpegQuality = quality;
}

int ExynosCamera3Parameters::getJpegQuality(void)
{
    return m_cameraInfo.jpegQuality;
}
status_t ExynosCamera3Parameters::checkThumbnailSize(int thumbnailW, int thumbnailH)
{
    int curThumbnailW = -1, curThumbnailH = -1;
    if (thumbnailW < 0 || thumbnailH < 0
        || thumbnailW > m_staticInfo->maxThumbnailW
        || thumbnailH > m_staticInfo->maxThumbnailH) {
        CLOGE("ERR(%s[%d]):Invalide thumbnail size %dx%d",
                __FUNCTION__, __LINE__, thumbnailW, thumbnailH);
        return BAD_VALUE;
    }
    getThumbnailSize(&curThumbnailW, &curThumbnailH);
    if (curThumbnailW != thumbnailW || curThumbnailH != thumbnailH) {
        CLOGI("INFO(%s[%d]):curThumbnailSize %dx%d newThumbnailSize %dx%d",
                __FUNCTION__, __LINE__,
                curThumbnailW, curThumbnailH, thumbnailW, thumbnailH);
        m_setThumbnailSize(thumbnailW, thumbnailH);
    }
    return NO_ERROR;
}

void ExynosCamera3Parameters::m_setThumbnailSize(int w, int h)
{
    m_cameraInfo.thumbnailW = w;
    m_cameraInfo.thumbnailH = h;
}

void ExynosCamera3Parameters::getThumbnailSize(int *w, int *h)
{
    *w = m_cameraInfo.thumbnailW;
    *h = m_cameraInfo.thumbnailH;
}

void ExynosCamera3Parameters::getMaxThumbnailSize(int *w, int *h)
{
    *w = m_staticInfo->maxThumbnailW;
    *h = m_staticInfo->maxThumbnailH;
}
status_t ExynosCamera3Parameters::checkThumbnailQuality(int quality)
{
    int curThumbnailQuality = -1;
    if (quality < 0 || quality > 100) {
        CLOGE("ERR(%s[%d]):Invalid thumbnail quality %d",
                __FUNCTION__, __LINE__, quality);
        return BAD_VALUE;
    }
    curThumbnailQuality = getThumbnailQuality();
    if (curThumbnailQuality != quality) {
        CLOGI("INFO(%s[%d]):curThumbnailQuality %d newThumbnailQuality %d",
                __FUNCTION__, __LINE__, curThumbnailQuality, quality);
        m_setThumbnailQuality(quality);
    }
    return NO_ERROR;
}

void ExynosCamera3Parameters::m_setThumbnailQuality(int quality)
{
    m_cameraInfo.thumbnailQuality = quality;
}

int ExynosCamera3Parameters::getThumbnailQuality(void)
{
    return m_cameraInfo.thumbnailQuality;
}

void ExynosCamera3Parameters::m_set3dnrMode(bool toggle)
{
    m_cameraInfo.is3dnrMode = toggle;
}

bool ExynosCamera3Parameters::get3dnrMode(void)
{
    return m_cameraInfo.is3dnrMode;
}

void ExynosCamera3Parameters::m_setDrcMode(bool toggle)
{
    m_cameraInfo.isDrcMode = toggle;
    if (setDrcEnable(toggle) < 0) {
        CLOGE2("set DRC fail, toggle(%d)", toggle);
    }
}

bool ExynosCamera3Parameters::getDrcMode(void)
{
    return m_cameraInfo.isDrcMode;
}

void ExynosCamera3Parameters::m_setOdcMode(bool toggle)
{
    m_cameraInfo.isOdcMode = toggle;
}

bool ExynosCamera3Parameters::getOdcMode(void)
{
    return m_cameraInfo.isOdcMode;
}

bool ExynosCamera3Parameters::getTpuEnabledMode(void)
{
    if (getHWVdisMode() == true)
        return true;

    if (get3dnrMode() == true)
        return true;

    if (getOdcMode() == true)
        return true;

    return false;
}

status_t ExynosCamera3Parameters::setZoomLevel(int zoom)
{
    int srcW = 0;
    int srcH = 0;
    int dstW = 0;
    int dstH = 0;

    m_cameraInfo.zoom = zoom;

    getHwSensorSize(&srcW, &srcH);
    getHwPreviewSize(&dstW, &dstH);

#if 0
    if (m_setParamCropRegion(zoom, srcW, srcH, dstW, dstH) != NO_ERROR) {
        return BAD_VALUE;
    }
#else
    ExynosRect srcRect, dstRect;
    getPreviewBayerCropSize(&srcRect, &dstRect);
#endif

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::setCropRegion(int x, int y, int w, int h)
{
    status_t ret = NO_ERROR;

    ret = setMetaCtlCropRegion(&m_metadata, x, y, w, h);
    if (ret != NO_ERROR) {
        CLOGE2("Failed to setMetaCtlCropRegion(%d, %d, %d, %d)", x, y, w, h);
    }

    return ret;
}

void ExynosCamera3Parameters::m_getCropRegion(int *x, int *y, int *w, int *h)
{
    getMetaCtlCropRegion(&m_metadata, x, y, w, h);
}

status_t ExynosCamera3Parameters::m_setParamCropRegion(
        int zoom,
        int srcW, int srcH,
        int dstW, int dstH)
{
    int newX = 0, newY = 0, newW = 0, newH = 0;
    float zoomRatio = getZoomRatio(zoom) / 1000;

    if (getCropRectAlign(srcW,  srcH,
                         dstW,  dstH,
                         &newX, &newY,
                         &newW, &newH,
                         CAMERA_MAGIC_ALIGN, 2,
                         zoom, zoomRatio) != NO_ERROR) {
        CLOGE2("getCropRectAlign(%d, %d, %d, %d) fail", srcW, srcH, dstW, dstH);
        return BAD_VALUE;
    }

    newX = ALIGN_UP(newX, 2);
    newY = ALIGN_UP(newY, 2);
    newW = srcW - (newX * 2);
    newH = srcH - (newY * 2);

    CLOGI2("size0(%d, %d, %d, %d)", srcW, srcH, dstW, dstH);
    CLOGI2("size(%d, %d, %d, %d), level(%d)", newX, newY, newW, newH, zoom);

    m_setHwBayerCropRegion(dstW, dstH, newX, newY);

    return NO_ERROR;
}

int ExynosCamera3Parameters::getZoomLevel(void)
{
    return m_cameraInfo.zoom;
}

void ExynosCamera3Parameters::m_setRotation(int rotation)
{
    m_cameraInfo.rotation = rotation;
}

int ExynosCamera3Parameters::getRotation(void)
{
    return m_cameraInfo.rotation;
}

void ExynosCamera3Parameters::m_setAutoExposureLock(bool lock)
{
    if (getHalVersion() != IS_HAL_VER_3_2) {
        m_cameraInfo.autoExposureLock = lock;
        setMetaCtlAeLock(&m_metadata, lock);
    }
}

bool ExynosCamera3Parameters::getAutoExposureLock(void)
{
    return m_cameraInfo.autoExposureLock;
}

void ExynosCamera3Parameters::m_adjustAeMode(enum aa_aemode curAeMode, enum aa_aemode *newAeMode)
{
    if (getHalVersion() != IS_HAL_VER_3_2) {
       int curMeteringMode = getMeteringMode();
           if (curAeMode == AA_AEMODE_OFF) {
           switch(curMeteringMode){
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
#define FW_CUSTOM_OFFSET (1)
/* F/W's middle value is 5, and step is -4, -3, -2, -1, 0, 1, 2, 3, 4 */
void ExynosCamera3Parameters::m_setExposureCompensation(int32_t value)
{
#if defined(USE_SUBDIVIDED_EV)
    setMetaCtlExposureCompensation(&m_metadata, value);
    setMetaCtlExposureCompensationStep(&m_metadata, m_staticInfo->exposureCompensationStep);
#else
    setMetaCtlExposureCompensation(&m_metadata, value + IS_EXPOSURE_DEFAULT + FW_CUSTOM_OFFSET);
#endif
}

int32_t ExynosCamera3Parameters::getExposureCompensation(void)
{
    int32_t expCompensation;
    getMetaCtlExposureCompensation(&m_metadata, &expCompensation);
#if defined(USE_SUBDIVIDED_EV)
    return expCompensation;
#else
    return expCompensation - IS_EXPOSURE_DEFAULT - FW_CUSTOM_OFFSET;
#endif
}

void ExynosCamera3Parameters::m_setMeteringAreas(uint32_t num, ExynosRect  *rects, int *weights)
{
    ExynosRect2 *rect2s = new ExynosRect2[num];

    for (uint32_t i = 0; i < num; i++)
        convertingRectToRect2(&rects[i], &rect2s[i]);

    m_setMeteringAreas(num, rect2s, weights);

    delete [] rect2s;
}

void ExynosCamera3Parameters::getMeteringAreas(__unused ExynosRect *rects)
{
    /* TODO */
}

void ExynosCamera3Parameters::getMeteringAreas(__unused ExynosRect2 *rect2s)
{
    /* TODO */
}

void ExynosCamera3Parameters::m_setMeteringMode(int meteringMode)
{
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t w = 0;
    uint32_t h = 0;
    uint32_t weight = 0;
    int hwSensorW = 0;
    int hwSensorH = 0;
    enum aa_aemode aeMode;

    if (getAutoExposureLock() == true) {
        CLOGD2("autoExposure is Locked");
        return;
    }

    m_cameraInfo.meteringMode = meteringMode;

    getHwSensorSize(&hwSensorW, &hwSensorH);

    switch (meteringMode) {
    case METERING_MODE_AVERAGE:
        aeMode = AA_AEMODE_AVERAGE;
        x = 0;
        y = 0;
        w = hwSensorW;
        h = hwSensorH;
        weight = 1000;
        break;
    case METERING_MODE_MATRIX:
        aeMode = AA_AEMODE_MATRIX;
        x = 0;
        y = 0;
        w = hwSensorW;
        h = hwSensorH;
        weight = 1000;
        break;
    case METERING_MODE_SPOT:
        /* In spot mode, default region setting is 100x100 rectangle on center */
        aeMode = AA_AEMODE_SPOT;
        x = hwSensorW / 2 - 50;
        y = hwSensorH / 2 - 50;
        w = hwSensorW / 2 + 50;
        h = hwSensorH / 2 + 50;
        weight = 50;
        break;
#ifdef TOUCH_AE
    case METERING_MODE_MATRIX_TOUCH:
        aeMode = AA_AEMODE_MATRIX_TOUCH;
        break;
    case METERING_MODE_SPOT_TOUCH:
        aeMode = AA_AEMODE_SPOT_TOUCH;
        break;
    case METERING_MODE_CENTER_TOUCH:
        aeMode = AA_AEMODE_CENTER_TOUCH;
        break;
    case METERING_MODE_AVERAGE_TOUCH:
        aeMode = AA_AEMODE_AVERAGE_TOUCH;
        break;
#endif
    case METERING_MODE_CENTER:
    default:
        aeMode = AA_AEMODE_CENTER;
        x = 0;
        y = 0;
        w = 0;
        h = 0;
        weight = 1000;
        break;
    }

    setMetaCtlAeMode(&m_metadata, aeMode);

    ExynosCameraActivityFlash *m_flashMgr = m_activityControl->getFlashMgr();
    m_flashMgr->setFlashExposure(aeMode);
}

int ExynosCamera3Parameters::getMeteringMode(void)
{
    return m_cameraInfo.meteringMode;
}

int ExynosCamera3Parameters::getSupportedMeteringMode(void)
{
    return m_staticInfo->meteringList;
}

void ExynosCamera3Parameters::m_setMeteringAreas(uint32_t num, ExynosRect2 *rect2s, int *weights)
{
    uint32_t maxNumMeteringAreas = getMaxNumMeteringAreas();

    if(getSamsungCamera()) {
        maxNumMeteringAreas = 1;
    }

    if (maxNumMeteringAreas == 0) {
        CLOGV2("maxNumMeteringAreas is 0. so, ignored");
        return;
    }

    if (maxNumMeteringAreas < num)
        num = maxNumMeteringAreas;

    if (getAutoExposureLock() == true) {
        CLOGD2("autoExposure is Locked");
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
            CLOGD2("(%d %d %d %d) %d", rect2s->x1, rect2s->y1, rect2s->x2, rect2s->y2, getMeteringMode());
            newRect2 = convertingAndroidArea2HWAreaBcropOut(&rect2s[i], &cropRegionRect);
            setMetaCtlAeRegion(&m_metadata, newRect2.x1, newRect2.y1,
                                newRect2.x2, newRect2.y2, weights[i]);
        }
    }
}

const char *ExynosCamera3Parameters::m_adjustAntibanding(const char *strAntibanding)
{
    const char *strAdjustedAntibanding = NULL;

    strAdjustedAntibanding = strAntibanding;

#if 0 /* fixed the flicker issue when highspeed recording(60fps or 120fps) */
    /* when high speed recording mode, off thre antibanding */
    if (getHighSpeedRecording())
        strAdjustedAntibanding = CameraParameters::ANTIBANDING_OFF;
#endif
    return strAdjustedAntibanding;
}


void ExynosCamera3Parameters::m_setAntibanding(int value)
{
    setMetaCtlAntibandingMode(&m_metadata, (enum aa_ae_antibanding_mode)value);
}

int ExynosCamera3Parameters::getAntibanding(void)
{
    enum aa_ae_antibanding_mode antibanding;
    getMetaCtlAntibandingMode(&m_metadata, &antibanding);
    return (int)antibanding;
}

int ExynosCamera3Parameters::getSupportedAntibanding(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return 0;
    } else {
        return m_staticInfo->antiBandingList;
    }
}

void ExynosCamera3Parameters::m_setSceneMode(int value)
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

int ExynosCamera3Parameters::getSceneMode(void)
{
    return m_cameraInfo.sceneMode;
}

int ExynosCamera3Parameters::getSupportedSceneModes(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return 0;
    } else {
        return m_staticInfo->sceneModeList;
    }
}

const char *ExynosCamera3Parameters::m_adjustFocusMode(const char *focusMode)
{
    int sceneMode = getSceneMode();
    const char *newFocusMode = NULL;

    /* TODO: vendor specific adjust */

    newFocusMode = focusMode;

    return newFocusMode;
}

void ExynosCamera3Parameters::m_setFocusMode(int focusMode)
{
    m_cameraInfo.focusMode = focusMode;

    if(getZoomActiveOn()) {
        CLOGD("DEBUG(%s):zoom moving..", "setParameters");
        return;
    }

    /* TODO: Notify auto focus activity */
    if(getPreviewRunning() == true) {
        CLOGD2("set Focus Mode(%s[%d]) !!!!");
        m_activityControl->setAutoFocusMode(focusMode);
    } else {
        m_setFocusmodeSetting = true;
    }
}

void ExynosCamera3Parameters::setFocusModeLock(bool enable) {
    int curFocusMode = getFocusMode();

    CLOGD2("FocusModeLock (%s)", enable? "true" : "false");

    if(enable) {
        m_activityControl->stopAutoFocus();
    } else {
        m_setFocusMode(curFocusMode);
    }
}

void ExynosCamera3Parameters::setFocusModeSetting(bool enable)
{
    m_setFocusmodeSetting = enable;
}

int ExynosCamera3Parameters::getFocusModeSetting(void)
{
    return m_setFocusmodeSetting;
}

int ExynosCamera3Parameters::getFocusMode(void)
{
    return m_cameraInfo.focusMode;
}

int ExynosCamera3Parameters::getSupportedFocusModes(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return 0;
    } else {
        return m_staticInfo->focusModeList;
    }
}

const char *ExynosCamera3Parameters::m_adjustFlashMode(const char *flashMode)
{
    int sceneMode = getSceneMode();
    const char *newFlashMode = NULL;

    /* TODO: vendor specific adjust */

    newFlashMode = flashMode;

    return newFlashMode;
}

void ExynosCamera3Parameters::m_setFlashMode(int flashMode)
{
    m_cameraInfo.flashMode = flashMode;

    /* TODO: Notity flash activity */
    m_activityControl->setFlashMode(flashMode);
}

int ExynosCamera3Parameters::getFlashMode(void)
{
    return m_cameraInfo.flashMode;
}

int ExynosCamera3Parameters::getSupportedFlashModes(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return 0;
    } else {
        return m_staticInfo->flashModeList;
    }
}

const char *ExynosCamera3Parameters::m_adjustWhiteBalanceMode(const char *whiteBalance)
{
    int sceneMode = getSceneMode();
    const char *newWhiteBalance = NULL;

    /* TODO: vendor specific adjust */

    /* TN' feautre can change whiteBalance even if Non SCENE_MODE_AUTO */

    newWhiteBalance = whiteBalance;

    return newWhiteBalance;
}

status_t ExynosCamera3Parameters::m_setWhiteBalanceMode(int whiteBalance)
{
    enum aa_awbmode awbMode;

    switch (whiteBalance) {
    case WHITE_BALANCE_AUTO:
        awbMode = AA_AWBMODE_WB_AUTO;
        break;
    case WHITE_BALANCE_INCANDESCENT:
        awbMode = AA_AWBMODE_WB_INCANDESCENT;
        break;
    case WHITE_BALANCE_FLUORESCENT:
        awbMode = AA_AWBMODE_WB_FLUORESCENT;
        break;
    case WHITE_BALANCE_DAYLIGHT:
        awbMode = AA_AWBMODE_WB_DAYLIGHT;
        break;
    case WHITE_BALANCE_CLOUDY_DAYLIGHT:
        awbMode = AA_AWBMODE_WB_CLOUDY_DAYLIGHT;
        break;
    case WHITE_BALANCE_WARM_FLUORESCENT:
        awbMode = AA_AWBMODE_WB_WARM_FLUORESCENT;
        break;
    case WHITE_BALANCE_TWILIGHT:
        awbMode = AA_AWBMODE_WB_TWILIGHT;
        break;
    case WHITE_BALANCE_SHADE:
        awbMode = AA_AWBMODE_WB_SHADE;
        break;
    default:
        CLOGE("ERR(%s):Unsupported value(%d)", __FUNCTION__, whiteBalance);
        return BAD_VALUE;
    }

    m_cameraInfo.whiteBalanceMode = whiteBalance;
    setMetaCtlAwbMode(&m_metadata, awbMode);

    ExynosCameraActivityFlash *m_flashMgr = m_activityControl->getFlashMgr();
    m_flashMgr->setFlashWhiteBalance(awbMode);

    return NO_ERROR;
}

int ExynosCamera3Parameters::m_convertMetaCtlAwbMode(struct camera2_shot_ext *shot_ext)
{
    int awbMode = WHITE_BALANCE_AUTO;

    switch (shot_ext->shot.ctl.aa.awbMode) {
        case AA_AWBMODE_WB_AUTO:
            awbMode = WHITE_BALANCE_AUTO;
            break;
        case AA_AWBMODE_WB_INCANDESCENT:
            awbMode = WHITE_BALANCE_INCANDESCENT;
            break;
        case AA_AWBMODE_WB_FLUORESCENT:
            awbMode = WHITE_BALANCE_FLUORESCENT;
            break;
        case AA_AWBMODE_WB_DAYLIGHT:
            awbMode = WHITE_BALANCE_DAYLIGHT;
            break;
        case AA_AWBMODE_WB_CLOUDY_DAYLIGHT:
            awbMode = WHITE_BALANCE_CLOUDY_DAYLIGHT;
            break;
        case AA_AWBMODE_WB_WARM_FLUORESCENT:
            awbMode = WHITE_BALANCE_WARM_FLUORESCENT;
            break;
        case AA_AWBMODE_WB_TWILIGHT:
            awbMode = WHITE_BALANCE_TWILIGHT;
            break;
        case AA_AWBMODE_WB_SHADE:
            awbMode = WHITE_BALANCE_SHADE;
            break;
        default:
            CLOGE2("Unsupported awbMode(%d)", shot_ext->shot.ctl.aa.awbMode);
            return BAD_VALUE;
    }

    return awbMode;
}

int ExynosCamera3Parameters::getWhiteBalanceMode(void)
{
    return m_cameraInfo.whiteBalanceMode;
}

int ExynosCamera3Parameters::getSupportedWhiteBalance(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return 0;
    } else {
        return m_staticInfo->whiteBalanceList;
    }
}

int ExynosCamera3Parameters::getSupportedISO(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return 0;
    } else {
        return m_staticInfo->isoValues;
    }
}

void ExynosCamera3Parameters::m_setAutoWhiteBalanceLock(bool value)
{
    if (getHalVersion() != IS_HAL_VER_3_2) {
        m_cameraInfo.autoWhiteBalanceLock = value;
        setMetaCtlAwbLock(&m_metadata, value);
    }
}

bool ExynosCamera3Parameters::getAutoWhiteBalanceLock(void)
{
    return m_cameraInfo.autoWhiteBalanceLock;
}

void ExynosCamera3Parameters::m_setFocusAreas(uint32_t numValid, ExynosRect *rects, int *weights)
{
    ExynosRect2 *rect2s = new ExynosRect2[numValid];

    for (uint32_t i = 0; i < numValid; i++)
        convertingRectToRect2(&rects[i], &rect2s[i]);

    m_setFocusAreas(numValid, rect2s, weights);

    delete [] rect2s;
}

void ExynosCamera3Parameters::m_setFocusAreas(uint32_t numValid, ExynosRect2 *rect2s, int *weights)
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

void ExynosCamera3Parameters::m_setColorEffectMode(int effect)
{
    aa_effect_mode_t newEffect;

    switch(effect) {
    case EFFECT_NONE:
        newEffect = AA_EFFECT_OFF;
        break;
    case EFFECT_MONO:
        newEffect = AA_EFFECT_MONO;
        break;
    case EFFECT_NEGATIVE:
        newEffect = AA_EFFECT_NEGATIVE;
        break;
    case EFFECT_SOLARIZE:
        newEffect = AA_EFFECT_SOLARIZE;
        break;
    case EFFECT_SEPIA:
        newEffect = AA_EFFECT_SEPIA;
        break;
    case EFFECT_POSTERIZE:
        newEffect = AA_EFFECT_POSTERIZE;
        break;
    case EFFECT_WHITEBOARD:
        newEffect = AA_EFFECT_WHITEBOARD;
        break;
    case EFFECT_BLACKBOARD:
        newEffect = AA_EFFECT_BLACKBOARD;
        break;
    case EFFECT_AQUA:
        newEffect = AA_EFFECT_AQUA;
        break;
    case EFFECT_RED_YELLOW:
        newEffect = AA_EFFECT_RED_YELLOW_POINT;
        break;
    case EFFECT_BLUE:
        newEffect = AA_EFFECT_BLUE_POINT;
        break;
    case EFFECT_WARM_VINTAGE:
        newEffect = AA_EFFECT_WARM_VINTAGE;
        break;
    case EFFECT_COLD_VINTAGE:
        newEffect = AA_EFFECT_COLD_VINTAGE;
        break;
    case EFFECT_BEAUTY_FACE:
        newEffect = AA_EFFECT_BEAUTY_FACE;
        break;
    default:
        newEffect = AA_EFFECT_OFF;
        CLOGE2("Color Effect mode(%d) is not supported", effect);
        break;
    }
    setMetaCtlAaEffect(&m_metadata, newEffect);
}

int ExynosCamera3Parameters::getColorEffectMode(void)
{
    aa_effect_mode_t curEffect;
    int effect;

    getMetaCtlAaEffect(&m_metadata, &curEffect);

    switch(curEffect) {
    case AA_EFFECT_OFF:
        effect = EFFECT_NONE;
        break;
    case AA_EFFECT_MONO:
        effect = EFFECT_MONO;
        break;
    case AA_EFFECT_NEGATIVE:
        effect = EFFECT_NEGATIVE;
        break;
    case AA_EFFECT_SOLARIZE:
        effect = EFFECT_SOLARIZE;
        break;
    case AA_EFFECT_SEPIA:
        effect = EFFECT_SEPIA;
        break;
    case AA_EFFECT_POSTERIZE:
        effect = EFFECT_POSTERIZE;
        break;
    case AA_EFFECT_WHITEBOARD:
        effect = EFFECT_WHITEBOARD;
        break;
    case AA_EFFECT_BLACKBOARD:
        effect = EFFECT_BLACKBOARD;
        break;
    case AA_EFFECT_AQUA:
        effect = EFFECT_AQUA;
        break;
    case AA_EFFECT_RED_YELLOW_POINT:
        effect = EFFECT_RED_YELLOW;
        break;
    case AA_EFFECT_BLUE_POINT:
        effect = EFFECT_BLUE;
        break;
    case AA_EFFECT_WARM_VINTAGE:
        effect = EFFECT_WARM_VINTAGE;
        break;
    case AA_EFFECT_COLD_VINTAGE:
        effect = EFFECT_COLD_VINTAGE;
        break;
    case AA_EFFECT_BEAUTY_FACE:
        effect = EFFECT_BEAUTY_FACE;
        break;
    default:
        effect = 0;
        CLOGE2("Color Effect mode(%d) is invalid value", curEffect);
        break;
    }

    return effect;
}

int ExynosCamera3Parameters::getSupportedColorEffects(void)
{
    return m_staticInfo->effectList;
}

bool ExynosCamera3Parameters::isSupportedColorEffects(int effectMode)
{
    int ret = false;

    if (effectMode & getSupportedColorEffects()) {
        return true;
    }

    if (effectMode & m_staticInfo->hiddenEffectList) {
        return true;
    }

    return ret;
}

void ExynosCamera3Parameters::m_setGpsAltitude(double altitude)
{
    m_cameraInfo.gpsAltitude = altitude;
}

double ExynosCamera3Parameters::getGpsAltitude(void)
{
    return m_cameraInfo.gpsAltitude;
}

void ExynosCamera3Parameters::m_setGpsLatitude(double latitude)
{
    m_cameraInfo.gpsLatitude = latitude;
}

double ExynosCamera3Parameters::getGpsLatitude(void)
{
    return m_cameraInfo.gpsLatitude;
}

void ExynosCamera3Parameters::m_setGpsLongitude(double longitude)
{
    m_cameraInfo.gpsLongitude = longitude;
}

double ExynosCamera3Parameters::getGpsLongitude(void)
{
    return m_cameraInfo.gpsLongitude;
}

void ExynosCamera3Parameters::m_setGpsProcessingMethod(const char *gpsProcessingMethod)
{
    memset(m_exifInfo.gps_processing_method, 0, sizeof(m_exifInfo.gps_processing_method));
    if (gpsProcessingMethod == NULL)
        return;

    size_t len = strlen(gpsProcessingMethod);

    if (len > sizeof(m_exifInfo.gps_processing_method)) {
        len = sizeof(m_exifInfo.gps_processing_method);
    }
    memcpy(m_exifInfo.gps_processing_method, gpsProcessingMethod, len);
}

const char *ExynosCamera3Parameters::getGpsProcessingMethod(void)
{
    return (const char *)m_exifInfo.gps_processing_method;
}

void ExynosCamera3Parameters::m_setExifFixedAttribute(void)
{
    char property[PROPERTY_VALUE_MAX];

    memset(&m_exifInfo, 0, sizeof(m_exifInfo));

    /* 2 0th IFD TIFF Tags */
    /* 3 Maker */
    strncpy((char *)m_exifInfo.maker, EXIF_DEF_MAKER,
                sizeof(m_exifInfo.maker) - 1);
    m_exifInfo.maker[sizeof(EXIF_DEF_MAKER) - 1] = '\0';

    /* 3 Model */
    property_get("ro.product.model", property, EXIF_DEF_MODEL);
    strncpy((char *)m_exifInfo.model, property,
                sizeof(m_exifInfo.model) - 1);
    m_exifInfo.model[sizeof(m_exifInfo.model) - 1] = '\0';
    /* 3 Software */
    property_get("ro.build.PDA", property, EXIF_DEF_SOFTWARE);
    strncpy((char *)m_exifInfo.software, property,
                sizeof(m_exifInfo.software) - 1);
    m_exifInfo.software[sizeof(m_exifInfo.software) - 1] = '\0';

    /* 3 YCbCr Positioning */
    m_exifInfo.ycbcr_positioning = EXIF_DEF_YCBCR_POSITIONING;

    /*2 0th IFD Exif Private Tags */
    /* 3 Exposure Program */
    m_exifInfo.exposure_program = EXIF_DEF_EXPOSURE_PROGRAM;
    /* 3 Exif Version */
    memcpy(m_exifInfo.exif_version, EXIF_DEF_EXIF_VERSION, sizeof(m_exifInfo.exif_version));

    if (getHalVersion() == IS_HAL_VER_3_2) {
        /* 3 Aperture */
        m_exifInfo.aperture.num = (int) m_staticInfo->aperture * COMMON_DENOMINATOR;
        m_exifInfo.aperture.den = COMMON_DENOMINATOR;
        /* 3 F Number */
        m_exifInfo.fnumber.num = m_staticInfo->fNumber * COMMON_DENOMINATOR;
        m_exifInfo.fnumber.den = COMMON_DENOMINATOR;
        /* 3 Maximum lens aperture */
        m_exifInfo.max_aperture.num = m_staticInfo->aperture * COMMON_DENOMINATOR;
        m_exifInfo.max_aperture.den = COMMON_DENOMINATOR;
        /* 3 Lens Focal Length */
        m_exifInfo.focal_length.num = m_staticInfo->focalLength * COMMON_DENOMINATOR;
        m_exifInfo.focal_length.den = COMMON_DENOMINATOR;
    } else {
        m_exifInfo.aperture.num = m_staticInfo->apertureNum;
        m_exifInfo.aperture.den = m_staticInfo->apertureDen;
        /* 3 F Number */
        m_exifInfo.fnumber.num = m_staticInfo->fNumberNum;
        m_exifInfo.fnumber.den = m_staticInfo->fNumberDen;
        /* 3 Maximum lens aperture */
        m_exifInfo.max_aperture.num = m_staticInfo->apertureNum;
        m_exifInfo.max_aperture.den = m_staticInfo->apertureDen;
        /* 3 Lens Focal Length */
        m_exifInfo.focal_length.num = m_staticInfo->focalLengthNum;
        m_exifInfo.focal_length.den = m_staticInfo->focalLengthDen;
    }

    /* 3 Maker note */
    if (m_exifInfo.maker_note)
        delete m_exifInfo.maker_note;

    m_exifInfo.maker_note_size = 98;
    m_exifInfo.maker_note = new unsigned char[m_exifInfo.maker_note_size];
    memset((void *)m_exifInfo.maker_note, 0, m_exifInfo.maker_note_size);
    /* 3 User Comments */
    if (m_exifInfo.user_comment)
        delete m_exifInfo.user_comment;

    m_exifInfo.user_comment_size = sizeof("user comment");
    m_exifInfo.user_comment = new unsigned char[m_exifInfo.user_comment_size + 8];
    memset((void *)m_exifInfo.user_comment, 0, m_exifInfo.user_comment_size + 8);

    /* 3 Color Space information */
    m_exifInfo.color_space = EXIF_DEF_COLOR_SPACE;
    /* 3 interoperability */
    m_exifInfo.interoperability_index = EXIF_DEF_INTEROPERABILITY;
    /* 3 Exposure Mode */
    m_exifInfo.exposure_mode = EXIF_DEF_EXPOSURE_MODE;

    /* 2 0th IFD GPS Info Tags */
    unsigned char gps_version[4] = { 0x02, 0x02, 0x00, 0x00 };
    memcpy(m_exifInfo.gps_version_id, gps_version, sizeof(gps_version));

    /* 2 1th IFD TIFF Tags */
    m_exifInfo.compression_scheme = EXIF_DEF_COMPRESSION;
    m_exifInfo.x_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    m_exifInfo.x_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    m_exifInfo.y_resolution.num = EXIF_DEF_RESOLUTION_NUM;
    m_exifInfo.y_resolution.den = EXIF_DEF_RESOLUTION_DEN;
    m_exifInfo.resolution_unit = EXIF_DEF_RESOLUTION_UNIT;
}

void ExynosCamera3Parameters::setExifChangedAttribute(exif_attribute_t   *exifInfo,
                                                     ExynosRect         *pictureRect,
                                                     ExynosRect         *thumbnailRect,
                                                     camera2_shot_t     *shot)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        m_setExifChangedAttribute(exifInfo, pictureRect, thumbnailRect, shot);
    } else {
        m_setExifChangedAttribute(exifInfo, pictureRect, thumbnailRect, &(shot->dm), &(shot->udm));
    }
}

void ExynosCamera3Parameters::m_setExifChangedAttribute(exif_attribute_t    *exifInfo,
                                                       ExynosRect           *pictureRect,
                                                       ExynosRect           *thumbnailRect,
                                                       __unused camera2_dm  *dm,
                                                       camera2_udm          *udm)
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

    /* 3 Date time */
    struct timeval rawtime;
    struct tm timeinfo;
    gettimeofday(&rawtime, NULL);
    localtime_r((time_t *)&rawtime.tv_sec, &timeinfo);
    strftime((char *)exifInfo->date_time, 20, "%Y:%m:%d %H:%M:%S", &timeinfo);
    sprintf((char *)exifInfo->sec_time, "%d", (int)(rawtime.tv_usec/1000));

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

    CLOGD2("udm->internal.vendorSpecific2[100](%d)", udm->internal.vendorSpecific2[100]);
    CLOGD2("udm->internal.vendorSpecific2[101](%d)", udm->internal.vendorSpecific2[101]);
    CLOGD2("udm->internal.vendorSpecific2[102](%d)", udm->internal.vendorSpecific2[102]);
    CLOGD2("udm->internal.vendorSpecific2[103](%d)", udm->internal.vendorSpecific2[103]);

    CLOGD2("iso_speed_rating(%d)", exifInfo->iso_speed_rating);
    CLOGD2("exposure_time(%d/%d)", exifInfo->exposure_time.num, exifInfo->exposure_time.den);
    CLOGD2("shutter_speed(%d/%d)", exifInfo->shutter_speed.num, exifInfo->shutter_speed.den);
    CLOGD2("aperture     (%d/%d)", exifInfo->aperture.num, exifInfo->aperture.den);
    CLOGD2("brightness   (%d/%d)", exifInfo->brightness.num, exifInfo->brightness.den);

    /* 3 Exposure Bias */
    exifInfo->exposure_bias.num = (int32_t)getExposureCompensation() * (m_staticInfo->exposureCompensationStep * 10);
    exifInfo->exposure_bias.den = 10;

    /* 3 Metering Mode */
    {
        switch (m_cameraInfo.meteringMode) {
        case METERING_MODE_CENTER:
            exifInfo->metering_mode = EXIF_METERING_CENTER;
            break;
        case METERING_MODE_MATRIX:
            exifInfo->metering_mode = EXIF_METERING_AVERAGE;
            break;
        case METERING_MODE_SPOT:
#ifdef TOUCH_AE
        case METERING_MODE_CENTER_TOUCH:
        case METERING_MODE_SPOT_TOUCH:
        case METERING_MODE_AVERAGE_TOUCH:
        case METERING_MODE_MATRIX_TOUCH:
#endif
            exifInfo->metering_mode = EXIF_METERING_SPOT;
            break;
        case METERING_MODE_AVERAGE:
        default:
            exifInfo->metering_mode = EXIF_METERING_AVERAGE;
            break;
        }
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
    if (m_cameraInfo.whiteBalanceMode == WHITE_BALANCE_AUTO)
        exifInfo->white_balance = EXIF_WB_AUTO;
    else
        exifInfo->white_balance = EXIF_WB_MANUAL;

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

    switch (this->getShotMode()) {
    case SHOT_MODE_BEAUTY_FACE:
    case SHOT_MODE_BEST_FACE:
        exifInfo->scene_capture_type = EXIF_SCENE_PORTRAIT;
        break;
    default:
        break;
    }

    /* 3 Image Unique ID */
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
#ifdef USE_CAMERA2_API_SUPPORT
void ExynosCamera3Parameters::m_setExifChangedAttribute(exif_attribute_t *exifInfo,
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
    memcpy((void *)mDebugInfo.debugData[APP_MARKER_4], (void *)&shot->udm, mDebugInfo.debugSize[APP_MARKER_4]);

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
#if 0 /* TODO: Must be same with the sensitivity in Result Metadata */
    exifInfo->iso_speed_rating = shot->udm.internal.vendorSpecific2[102];
#else
    exifInfo->iso_speed_rating = shot->dm.sensor.sensitivity;
#endif

    /* Exposure Time */
    exifInfo->exposure_time.num = 1;
#if 0 /* TODO: Must be same with the exposure time in Result Metadata */
    if (shot->udm.ae.vendorSpecific[0] == 0xAEAEAEAE) {
        exifInfo->exposure_time.den = (uint32_t) shot->udm.ae.vendorSpecific[64];
    } else
#endif
    {
        /* HACK : Sometimes, F/W does NOT send the exposureTime */
        if (shot->dm.sensor.exposureTime != 0)
            exifInfo->exposure_time.den = (uint32_t) 1e9 / shot->dm.sensor.exposureTime;
        else
            exifInfo->exposure_time.num = 0;
    }

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

    CLOGD2("udm->internal.vendorSpecific2[101](%d)", shot->udm.internal.vendorSpecific2[101]);
    CLOGD2("udm->internal.vendorSpecific2[102](%d)", shot->udm.internal.vendorSpecific2[102]);
    CLOGD2("udm->internal.vendorSpecific2[103](%d)", shot->udm.internal.vendorSpecific2[103]);
    CLOGD2("udm->internal.vendorSpecific2[104](%d)", shot->udm.internal.vendorSpecific2[104]);

    CLOGD2("iso_speed_rating(%d)", exifInfo->iso_speed_rating);
    CLOGD2("exposure_time(%d/%d)", exifInfo->exposure_time.num, exifInfo->exposure_time.den);
    CLOGD2("shutter_speed(%d/%d)", exifInfo->shutter_speed.num, exifInfo->shutter_speed.den);
    CLOGD2("aperture     (%d/%d)", exifInfo->aperture.num, exifInfo->aperture.den);
    CLOGD2("brightness   (%d/%d)", exifInfo->brightness.num, exifInfo->brightness.den);

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
#endif

debug_attribute_t *ExynosCamera3Parameters::getDebugAttribute(void)
{
    return &mDebugInfo;
}

status_t ExynosCamera3Parameters::getFixedExifInfo(exif_attribute_t *exifInfo)
{
    if (exifInfo == NULL) {
        CLOGE2("buffer is NULL");
        return BAD_VALUE;
    }

    memcpy(exifInfo, &m_exifInfo, sizeof(exif_attribute_t));

    return NO_ERROR;
}

void ExynosCamera3Parameters::m_setGpsTimeStamp(long timeStamp)
{
    m_cameraInfo.gpsTimeStamp = timeStamp;
}

long ExynosCamera3Parameters::getGpsTimeStamp(void)
{
    return m_cameraInfo.gpsTimeStamp;
}

/* TODO: Do not used yet */
#if 0
status_t ExynosCamera3Parameters::checkCityId(const CameraParameters& params)
{
    long long int newCityId = params.getInt64(CameraParameters::KEY_CITYID);
    long long int curCityId = -1;

    if (newCityId < 0)
        newCityId = 0;

    curCityId = getCityId();

    if (curCityId != newCityId) {
        m_setCityId(newCityId);
        m_params.set(CameraParameters::KEY_CITYID, newCityId);
    }

    return NO_ERROR;
}

void ExynosCamera3Parameters::m_setCityId(long long int cityId)
{
    m_cameraInfo.cityId = cityId;
}

long long int ExynosCamera3Parameters::getCityId(void)
{
    return m_cameraInfo.cityId;
}

status_t ExynosCamera3Parameters::checkWeatherId(const CameraParameters& params)
{
    int newWeatherId = params.getInt(CameraParameters::KEY_WEATHER);
    int curWeatherId = -1;

    if (newWeatherId < 0 || newWeatherId > 5) {
        return BAD_VALUE;
    }

    curWeatherId = (int)getWeatherId();

    if (curWeatherId != newWeatherId) {
        m_setWeatherId((unsigned char)newWeatherId);
        m_params.set(CameraParameters::KEY_WEATHER, newWeatherId);
    }

    return NO_ERROR;
}

void ExynosCamera3Parameters::m_setWeatherId(unsigned char weatherId)
{
    m_cameraInfo.weatherId = weatherId;
}

unsigned char ExynosCamera3Parameters::getWeatherId(void)
{
    return m_cameraInfo.weatherId;
}
#endif


/* F/W's middle value is 3, and step is -2, -1, 0, 1, 2 */
void ExynosCamera3Parameters::m_setBrightness(int brightness)
{
    setMetaCtlBrightness(&m_metadata, brightness + IS_BRIGHTNESS_DEFAULT + FW_CUSTOM_OFFSET);
}

int ExynosCamera3Parameters::getBrightness(void)
{
    int32_t brightness = 0;

    getMetaCtlBrightness(&m_metadata, &brightness);
    return brightness - IS_BRIGHTNESS_DEFAULT - FW_CUSTOM_OFFSET;
}

void ExynosCamera3Parameters::m_setSaturation(int saturation)
{
    setMetaCtlSaturation(&m_metadata, saturation + IS_SATURATION_DEFAULT + FW_CUSTOM_OFFSET);
}

int ExynosCamera3Parameters::getSaturation(void)
{
    int32_t saturation = 0;

    getMetaCtlSaturation(&m_metadata, &saturation);
    return saturation - IS_SATURATION_DEFAULT - FW_CUSTOM_OFFSET;
}

void ExynosCamera3Parameters::m_setSharpness(int sharpness)
{
    int newSharpness = sharpness + IS_SHARPNESS_DEFAULT;
    enum processing_mode edge_mode = PROCESSING_MODE_OFF;
    enum processing_mode noise_mode = PROCESSING_MODE_OFF;
    int edge_strength = 0;
    int noise_strength = 0;

    switch (newSharpness) {
    case IS_SHARPNESS_MINUS_2:
        edge_mode = PROCESSING_MODE_OFF;
        noise_mode = PROCESSING_MODE_HIGH_QUALITY;
        edge_strength = 0;
        noise_strength = 10;
        break;
    case IS_SHARPNESS_MINUS_1:
        edge_mode = PROCESSING_MODE_OFF;
        noise_mode = PROCESSING_MODE_HIGH_QUALITY;
        edge_strength = 0;
        noise_strength = 5;
        break;
    case IS_SHARPNESS_DEFAULT:
        edge_mode = PROCESSING_MODE_OFF;
        noise_mode = PROCESSING_MODE_OFF;
        edge_strength = 0;
        noise_strength = 0;
        break;
    case IS_SHARPNESS_PLUS_1:
        edge_mode = PROCESSING_MODE_HIGH_QUALITY;
        noise_mode = PROCESSING_MODE_OFF;
        edge_strength = 5;
        noise_strength = 0;
        break;
    case IS_SHARPNESS_PLUS_2:
        edge_mode = PROCESSING_MODE_HIGH_QUALITY;
        noise_mode = PROCESSING_MODE_OFF;
        edge_strength = 10;
        noise_strength = 0;
        break;
    default:
        break;
    }

    CLOGD2("newSharpness %d edge_mode(%d),st(%d), noise(%d),st(%d)",
         newSharpness, edge_mode, edge_strength, noise_mode, noise_strength);

    setMetaCtlSharpness(&m_metadata, edge_mode, edge_strength, noise_mode, noise_strength);
}

int ExynosCamera3Parameters::getSharpness(void)
{
    int32_t edge_sharpness = 0;
    int32_t noise_sharpness = 0;
    int32_t sharpness = 0;
    enum processing_mode edge_mode = PROCESSING_MODE_OFF;
    enum processing_mode noise_mode = PROCESSING_MODE_OFF;

    getMetaCtlSharpness(&m_metadata, &edge_mode, &edge_sharpness, &noise_mode, &noise_sharpness);

    if(noise_sharpness == 10 && edge_sharpness == 0) {
        sharpness = IS_SHARPNESS_MINUS_2;
    } else if(noise_sharpness == 5 && edge_sharpness == 0) {
        sharpness = IS_SHARPNESS_MINUS_1;
    } else if(noise_sharpness == 0 && edge_sharpness == 0) {
        sharpness = IS_SHARPNESS_DEFAULT;
    } else if(noise_sharpness == 0 && edge_sharpness == 5) {
        sharpness = IS_SHARPNESS_PLUS_1;
    } else if(noise_sharpness == 0 && edge_sharpness == 10) {
        sharpness = IS_SHARPNESS_PLUS_2;
    } else {
        sharpness = IS_SHARPNESS_DEFAULT;
    }

    return sharpness - IS_SHARPNESS_DEFAULT;
}

void ExynosCamera3Parameters::m_setHue(int hue)
{
    setMetaCtlHue(&m_metadata, hue + IS_HUE_DEFAULT + FW_CUSTOM_OFFSET);
}

int ExynosCamera3Parameters::getHue(void)
{
    int32_t hue = 0;

    getMetaCtlHue(&m_metadata, &hue);
    return hue - IS_HUE_DEFAULT - FW_CUSTOM_OFFSET;
}

void ExynosCamera3Parameters::m_setIso(uint32_t iso)
{
    enum aa_isomode mode = AA_ISOMODE_AUTO;

    if (iso == 0 )
        mode = AA_ISOMODE_AUTO;
    else
        mode = AA_ISOMODE_MANUAL;

    setMetaCtlIso(&m_metadata, mode, iso);
}

uint32_t ExynosCamera3Parameters::getIso(void)
{
    enum aa_isomode mode = AA_ISOMODE_AUTO;
    uint32_t iso = 0;

    getMetaCtlIso(&m_metadata, &mode, &iso);

    return iso;
}

uint64_t ExynosCamera3Parameters::getCaptureExposureTime(void)
{
    return m_exposureTimeCapture;
}

void ExynosCamera3Parameters::m_setContrast(uint32_t contrast)
{
    setMetaCtlContrast(&m_metadata, contrast);
}

uint32_t ExynosCamera3Parameters::getContrast(void)
{
    uint32_t contrast = 0;
    getMetaCtlContrast(&m_metadata, &contrast);
    return contrast;
}

void ExynosCamera3Parameters::m_setHdrMode(bool hdr)
{
    m_cameraInfo.hdrMode = hdr;

    if (hdr == true)
        m_setShotMode(SHOT_MODE_RICH_TONE);
    else
        m_setShotMode(SHOT_MODE_NORMAL);

    m_activityControl->setHdrMode(hdr);
}

bool ExynosCamera3Parameters::getHdrMode(void)
{
    return m_cameraInfo.hdrMode;
}

void ExynosCamera3Parameters::m_setWdrMode(bool wdr)
{
    m_cameraInfo.wdrMode = wdr;
}

bool ExynosCamera3Parameters::getWdrMode(void)
{
    return m_cameraInfo.wdrMode;
}

#ifdef USE_BINNING_MODE
int ExynosCamera3Parameters::getBinningMode(void)
{
    char cameraModeProperty[PROPERTY_VALUE_MAX];
    int ret = 0;

    if (m_staticInfo->vtcallSizeLutMax == 0 || m_staticInfo->vtcallSizeLut == NULL) {
        CLOGV2("vtCallSizeLut is NULL, can't support the binnig mode");
        return ret;
    }

    /* For VT Call with DualCamera Scenario */
    if (getDualMode()) {
        CLOGV2("DualMode can't support the binnig mode.(%d,%d)", getCameraId(), getDualMode());
        return ret;
    }

    if (getVtMode() != 3 && getVtMode() > 0 && getVtMode() < 5) {
        ret = 1;
    } else {
        if (m_binningProperty == true) {
            ret = 1;
        }
    }
    return ret;
}
#endif

void ExynosCamera3Parameters::m_setShotMode(int shotMode)
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
    case SHOT_MODE_DUAL:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_DUAL;
        break;
    case SHOT_MODE_AQUA:
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_AQUA;
        break;
    case SHOT_MODE_AUTO_PORTRAIT:
    case SHOT_MODE_PET:
    default:
        changeSceneMode = false;
        break;
    }

    m_cameraInfo.shotMode = shotMode;
    if (changeSceneMode == true)
        setMetaCtlSceneMode(&m_metadata, mode, sceneMode);
}

int ExynosCamera3Parameters::getPreviewBufferCount(void)
{
    CLOGV("DEBUG(%s):getPreviewBufferCount %d", "setParameters", m_previewBufferCount);

    return m_previewBufferCount;
}

void ExynosCamera3Parameters::setPreviewBufferCount(int previewBufferCount)
{
    m_previewBufferCount = previewBufferCount;

    CLOGV("DEBUG(%s):setPreviewBufferCount %d", "setParameters", m_previewBufferCount);

    return;
}

int ExynosCamera3Parameters::getShotMode(void)
{
    return m_cameraInfo.shotMode;
}

void ExynosCamera3Parameters::m_setAntiShake(bool toggle)
{
    enum aa_mode mode = AA_CONTROL_AUTO;
    enum aa_scene_mode sceneMode = AA_SCENE_MODE_FACE_PRIORITY;

    if (toggle == true) {
        mode = AA_CONTROL_USE_SCENE_MODE;
        sceneMode = AA_SCENE_MODE_ANTISHAKE;
    }

    setMetaCtlSceneMode(&m_metadata, mode, sceneMode);
    m_cameraInfo.antiShake = toggle;
}

bool ExynosCamera3Parameters::getAntiShake(void)
{
    return m_cameraInfo.antiShake;
}

void ExynosCamera3Parameters::m_setVtMode(int vtMode)
{
    m_cameraInfo.vtMode = vtMode;

    setMetaVtMode(&m_metadata, (enum camera_vt_mode)vtMode);
}

int ExynosCamera3Parameters::getVtMode(void)
{
    return m_cameraInfo.vtMode;
}

int ExynosCamera3Parameters::getVRMode(void)
{
    return m_cameraInfo.vrMode;
}

void ExynosCamera3Parameters::m_setGamma(bool gamma)
{
    m_cameraInfo.gamma = gamma;
}

bool ExynosCamera3Parameters::getGamma(void)
{
    return m_cameraInfo.gamma;
}

void ExynosCamera3Parameters::m_setSlowAe(bool slowAe)
{
    m_cameraInfo.slowAe = slowAe;
}

bool ExynosCamera3Parameters::getSlowAe(void)
{
    return m_cameraInfo.slowAe;
}

bool ExynosCamera3Parameters::isScalableSensorSupported(void)
{
    return m_staticInfo->scalableSensorSupport;
}

bool ExynosCamera3Parameters::m_adjustScalableSensorMode(const int scaleMode)
{
    bool adjustedScaleMode = false;
    int pictureW = 0;
    int pictureH = 0;
    float pictureRatio = 0;
    uint32_t minFps = 0;
    uint32_t maxFps = 0;

    /* If scale_mode is 1 or dual camera, scalable sensor turn on */
    if (scaleMode == 1)
        adjustedScaleMode = true;

    if (getDualMode() == true)
        adjustedScaleMode = true;

    /*
     * scalable sensor only support     24     fps for 4:3  - picture size
     * scalable sensor only support 15, 24, 30 fps for 16:9 - picture size
     */
    getPreviewFpsRange(&minFps, &maxFps);
    getPictureSize(&pictureW, &pictureH);

    pictureRatio = ROUND_OFF(((float)pictureW / (float)pictureH), 2);

    if (pictureRatio == 1.33f) { /* 4:3 */
        if (maxFps != 24)
            adjustedScaleMode = false;
    } else if (pictureRatio == 1.77f) { /* 16:9 */
        if ((maxFps != 15) && (maxFps != 24) && (maxFps != 30))
            adjustedScaleMode = false;
    } else {
        adjustedScaleMode = false;
    }

    if (scaleMode == 1 && adjustedScaleMode == false) {
        CLOGW2("pictureRatio(%f, %d, %d) fps(%d, %d) is not proper for scalable",
                pictureRatio, pictureW, pictureH, minFps, maxFps);
    }

    return adjustedScaleMode;
}

void ExynosCamera3Parameters::setScalableSensorMode(bool scaleMode)
{
    m_cameraInfo.scalableSensorMode = scaleMode;
}

bool ExynosCamera3Parameters::getScalableSensorMode(void)
{
    return m_cameraInfo.scalableSensorMode;
}

void ExynosCamera3Parameters::m_getScalableSensorSize(int *newSensorW, int *newSensorH)
{
    int previewW = 0;
    int previewH = 0;

    *newSensorW = 1920;
    *newSensorH = 1080;

    /* default scalable sensor size is 1920x1080(16:9) */
    getPreviewSize(&previewW, &previewH);

    /* when preview size is 1440x1080(4:3), return sensor size(1920x1440) */
    /* if (previewW == 1440 && previewH == 1080) { */
    if ((previewW * 3 / 4) == previewH) {
        *newSensorW  = 1920;
        *newSensorH = 1440;
    }
}

status_t ExynosCamera3Parameters::m_setImageUniqueId(const char *uniqueId)
{
    int uniqueIdLength;

    if (uniqueId == NULL) {
        return BAD_VALUE;
    }

    memset(m_cameraInfo.imageUniqueId, 0, sizeof(m_cameraInfo.imageUniqueId));

    uniqueIdLength = strlen(uniqueId);
    memcpy(m_cameraInfo.imageUniqueId, uniqueId, uniqueIdLength);

    return NO_ERROR;
}

const char *ExynosCamera3Parameters::getImageUniqueId(void)
{
    return m_cameraInfo.imageUniqueId;
}

#ifdef BURST_CAPTURE
int ExynosCamera3Parameters::getSeriesShotSaveLocation(void)
{
    int seriesShotSaveLocation = m_seriesShotSaveLocation;
    int shotMode = getShotMode();

    /* GED's series shot work as callback */
#ifdef CAMERA_GED_FEATURE
    seriesShotSaveLocation = BURST_SAVE_CALLBACK;
#else
    if (shotMode == SHOT_MODE_BEST_PHOTO) {
        seriesShotSaveLocation = BURST_SAVE_CALLBACK;
    } else {
        if (m_seriesShotSaveLocation == 0)
            seriesShotSaveLocation = BURST_SAVE_PHONE;
        else
            seriesShotSaveLocation = BURST_SAVE_SDCARD;
    }
#endif

    return seriesShotSaveLocation;
}

void ExynosCamera3Parameters::setSeriesShotSaveLocation(int ssaveLocation)
{
    m_seriesShotSaveLocation = ssaveLocation;
}

char *ExynosCamera3Parameters::getSeriesShotFilePath(void)
{
    return m_seriesShotFilePath;
}
#endif

int ExynosCamera3Parameters::getSeriesShotDuration(void)
{
    switch (m_cameraInfo.seriesShotMode) {
    case SERIES_SHOT_MODE_BURST:
        return NORMAL_BURST_DURATION;
    case SERIES_SHOT_MODE_BEST_FACE:
        return BEST_FACE_DURATION;
    case SERIES_SHOT_MODE_BEST_PHOTO:
        return BEST_PHOTO_DURATION;
    case SERIES_SHOT_MODE_ERASER:
        return ERASER_DURATION;
    case SERIES_SHOT_MODE_SELFIE_ALARM:
        return SELFIE_ALARM_DURATION;
    default:
        return 0;
    }
    return 0;
}

int ExynosCamera3Parameters::getSeriesShotMode(void)
{
    return m_cameraInfo.seriesShotMode;
}

void ExynosCamera3Parameters::setSeriesShotMode(int sshotMode, int count)
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
        }
        else if (shotMode == SHOT_MODE_SELFIE_ALARM) {
            sshotMode = SERIES_SHOT_MODE_SELFIE_ALARM;
            sshotCount = 3;
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

    CLOGD2("set shotmode(%d), shotCount(%d)", sshotMode, sshotCount);

    m_cameraInfo.seriesShotMode = sshotMode;
    m_setSeriesShotCount(sshotCount);
}

void ExynosCamera3Parameters::m_setSeriesShotCount(int seriesShotCount)
{
    m_cameraInfo.seriesShotCount = seriesShotCount;
}

int ExynosCamera3Parameters::getSeriesShotCount(void)
{
    return m_cameraInfo.seriesShotCount;
}

void ExynosCamera3Parameters::setSamsungCamera(bool value)
{
    String8 tempStr;
    ExynosCameraActivityAutofocus *autoFocusMgr = m_activityControl->getAutoFocusMgr();

    m_cameraInfo.samsungCamera = value;

#if 0
    autoFocusMgr->setSamsungCamera(value);

    /* zoom */
    if (getZoomSupported() == true) {
        int maxZoom = getMaxZoomLevel();
        CLOGI2("change MaxZoomLevel and ZoomRatio List.(%d)", maxZoom);

        if (0 < maxZoom) {
            m_params.set(CameraParameters::KEY_ZOOM_SUPPORTED, "true");

            if (getSmoothZoomSupported() == true)
                m_params.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "true");
            else
                m_params.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");

            m_params.set(CameraParameters::KEY_MAX_ZOOM, maxZoom - 1);
            m_params.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);

            int max_zoom_ratio = (int)getMaxZoomRatio();
            tempStr.setTo("");
            if (getZoomRatioList(tempStr, maxZoom, max_zoom_ratio, m_staticInfo->zoomRatioList) == NO_ERROR)
                m_params.set(CameraParameters::KEY_ZOOM_RATIOS, tempStr.string());
            else
                m_params.set(CameraParameters::KEY_ZOOM_RATIOS, "100");

            m_params.set("constant-growth-rate-zoom-supported", "true");

            CLOGV("INFO(%s):zoomRatioList=%s", "setDefaultParameter", tempStr.string());
        } else {
            m_params.set(CameraParameters::KEY_ZOOM_SUPPORTED, "false");
            m_params.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
            m_params.set(CameraParameters::KEY_MAX_ZOOM, ZOOM_LEVEL_0);
            m_params.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);
        }
    } else {
        m_params.set(CameraParameters::KEY_ZOOM_SUPPORTED, "false");
        m_params.set(CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED, "false");
        m_params.set(CameraParameters::KEY_MAX_ZOOM, ZOOM_LEVEL_0);
        m_params.set(CameraParameters::KEY_ZOOM, ZOOM_LEVEL_0);
    }
#endif
}

bool ExynosCamera3Parameters::getSamsungCamera(void)
{
    return m_cameraInfo.samsungCamera;
}

bool ExynosCamera3Parameters::getZoomSupported(void)
{
    return m_staticInfo->zoomSupport;
}

bool ExynosCamera3Parameters::getSmoothZoomSupported(void)
{
    return m_staticInfo->smoothZoomSupport;
}

void ExynosCamera3Parameters::setHorizontalViewAngle(int pictureW, int pictureH)
{
    double pi_camera = 3.1415926f;
    double distance;
    double ratio;
    double hViewAngle_half_rad = pi_camera / 180 * (double)m_staticInfo->horizontalViewAngle[SIZE_RATIO_16_9] / 2;

    distance = ((double)m_staticInfo->maxSensorW / (double)m_staticInfo->maxSensorH * 9 / 2)
                / tan(hViewAngle_half_rad);
    ratio = (double)pictureW / (double)pictureH;

    m_calculatedHorizontalViewAngle = (float)(atan(ratio * 9 / 2 / distance) * 2 * 180 / pi_camera);
}

float ExynosCamera3Parameters::getHorizontalViewAngle(void)
{
    int right_ratio = 177;

    if ((int)(m_staticInfo->maxSensorW * 100 / m_staticInfo->maxSensorH) == right_ratio) {
        return m_calculatedHorizontalViewAngle;
    } else {
        return m_staticInfo->horizontalViewAngle[m_cameraInfo.pictureSizeRatioId];
    }
}

float ExynosCamera3Parameters::getVerticalViewAngle(void)
{
    return m_staticInfo->verticalViewAngle;
}

void ExynosCamera3Parameters::getFnumber(int *num, int *den)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        *num = m_staticInfo->fNumber * COMMON_DENOMINATOR;
        *den = COMMON_DENOMINATOR;
    } else {
        *num = m_staticInfo->fNumberNum;
        *den = m_staticInfo->fNumberDen;
    }
}

void ExynosCamera3Parameters::getApertureValue(int *num, int *den)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        *num = m_staticInfo->aperture * COMMON_DENOMINATOR;
        *den = COMMON_DENOMINATOR;
    } else {
        *num = m_staticInfo->apertureNum;
        *den = m_staticInfo->apertureDen;
    }
}

int ExynosCamera3Parameters::getFocalLengthIn35mmFilm(void)
{
    return m_staticInfo->focalLengthIn35mmLength;
}

void ExynosCamera3Parameters::getFocalLength(int *num, int *den)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        *num = m_staticInfo->focalLength * COMMON_DENOMINATOR;
        *den = COMMON_DENOMINATOR;
    } else {
        *num = m_staticInfo->focalLengthNum;
        *den = m_staticInfo->focalLengthDen;
    }
}

void ExynosCamera3Parameters::getFocusDistances(int *num, int *den)
{
    *num = m_staticInfo->focusDistanceNum;
    *den = m_staticInfo->focusDistanceDen;
}

int ExynosCamera3Parameters::getMinExposureCompensation(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return m_staticInfo->exposureCompensationRange[MIN];
    } else {
        return m_staticInfo->minExposureCompensation;
    }
}

int ExynosCamera3Parameters::getMaxExposureCompensation(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return m_staticInfo->exposureCompensationRange[MAX];
    } else {
        return m_staticInfo->maxExposureCompensation;
    }
}

float ExynosCamera3Parameters::getExposureCompensationStep(void)
{
    return m_staticInfo->exposureCompensationStep;
}

int ExynosCamera3Parameters::getMaxNumDetectedFaces(void)
{
    return m_staticInfo->maxNumDetectedFaces;
}

uint32_t ExynosCamera3Parameters::getMaxNumFocusAreas(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return m_staticInfo->max3aRegions[AF];
    } else {
        return m_staticInfo->maxNumFocusAreas;
    }
}

uint32_t ExynosCamera3Parameters::getMaxNumMeteringAreas(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return m_staticInfo->max3aRegions[AE];
    } else {
        return m_staticInfo->maxNumMeteringAreas;
    }
}

int ExynosCamera3Parameters::getMaxZoomLevel(void)
{
    return m_staticInfo->maxZoomLevel;
}

float ExynosCamera3Parameters::getMaxZoomRatio(void)
{
    return (float)m_staticInfo->maxZoomRatio;
}

float ExynosCamera3Parameters::getZoomRatio(int zoomLevel)
{
    float zoomRatio = 1.00f;
    if (getZoomSupported() == true)
        zoomRatio = (float)m_staticInfo->zoomRatioList[zoomLevel];
    else
        zoomRatio = 1000.00f;

    return zoomRatio;
}

bool ExynosCamera3Parameters::getVideoSnapshotSupported(void)
{
    return m_staticInfo->videoSnapshotSupport;
}

bool ExynosCamera3Parameters::getVideoStabilizationSupported(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        bool supported = false;
        for (size_t i = 0; i < m_staticInfo->videoStabilizationModesLength; i++) {
            if (m_staticInfo->videoStabilizationModes[i]
                    == ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON) {
                supported = true;
                break;
            }
        }
        return supported;
    } else {
        return m_staticInfo->videoStabilizationSupport;
    }
}

bool ExynosCamera3Parameters::getAutoWhiteBalanceLockSupported(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return true;
    } else {
        return m_staticInfo->autoWhiteBalanceLockSupport;
    }
}

bool ExynosCamera3Parameters::getAutoExposureLockSupported(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return true;
    } else {
        return m_staticInfo->autoExposureLockSupport;
    }
}

void ExynosCamera3Parameters::enableMsgType(int32_t msgType)
{
    Mutex::Autolock lock(m_msgLock);
    m_enabledMsgType |= msgType;
}

void ExynosCamera3Parameters::disableMsgType(int32_t msgType)
{
    Mutex::Autolock lock(m_msgLock);
    m_enabledMsgType &= ~msgType;
}

bool ExynosCamera3Parameters::msgTypeEnabled(int32_t msgType)
{
    Mutex::Autolock lock(m_msgLock);
    return (m_enabledMsgType & msgType);
}

void ExynosCamera3Parameters::m_initMetadata(void)
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
#ifdef USE_CAMERA2_API_SUPPORT
    shot->ctl.lens.aperture = m_staticInfo->aperture;
    shot->ctl.lens.focalLength = m_staticInfo->focalLength;
#else
    shot->ctl.lens.aperture = (float)m_staticInfo->apertureNum / (float)m_staticInfo->apertureDen;
    shot->ctl.lens.focalLength = (float)m_staticInfo->focalLengthNum / (float)m_staticInfo->focalLengthDen;
#endif
    shot->ctl.lens.filterDensity = 0.0f;
    shot->ctl.lens.opticalStabilizationMode = ::OPTICAL_STABILIZATION_MODE_OFF;

    int minFps = (m_staticInfo->minFps == 0) ? 0 : (m_staticInfo->maxFps / 2);
    int maxFps = (m_staticInfo->maxFps == 0) ? 0 : m_staticInfo->maxFps;

    /* The min fps can not be '0'. Therefore it is set up default value '15'. */
    if (minFps == 0) {
        CLOGW2("Invalid min fps value(%d)", minFps);
        minFps = 15;
    }

    /*  The initial fps can not be '0' and bigger than '30'. Therefore it is set up default value '30'. */
    if (maxFps == 0 || 30 < maxFps) {
        CLOGW2("Invalid max fps value(%d)", maxFps);
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
    shot->ctl.noise.mode = ::PROCESSING_MODE_OFF;
    shot->ctl.noise.strength = 5;

    /* shading */
    shot->ctl.shading.mode = (enum processing_mode)0;

    /* color */
    shot->ctl.color.mode = ::COLORCORRECTION_MODE_FAST;
    static const float colorTransform[9] = {
        1.0f, 0.f, 0.f,
        0.f, 1.f, 0.f,
        0.f, 0.f, 1.f
    };

#ifdef USE_CAMERA2_API_SUPPORT
    for (size_t i = 0; i < sizeof(colorTransform)/sizeof(colorTransform[0]); i++) {
        shot->ctl.color.transform[i].num = colorTransform[i] * COMMON_DENOMINATOR;
        shot->ctl.color.transform[i].den = COMMON_DENOMINATOR;
    }
#else
    memcpy(shot->ctl.color.transform, colorTransform, sizeof(colorTransform));
#endif

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
    shot->ctl.edge.mode = ::PROCESSING_MODE_OFF;
    shot->ctl.edge.strength = 5;

    /* scaler
     * Max Picture Size == Max Sensor Size - Sensor Margin
     */
    if (m_setParamCropRegion(0,
                             m_staticInfo->maxPictureW, m_staticInfo->maxPictureH,
                             m_staticInfo->maxPreviewW, m_staticInfo->maxPreviewH
                             ) != NO_ERROR) {
        CLOGE2("m_setZoom() fail");
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
    shot->ctl.aa.videoStabilizationMode = (enum aa_videostabilization_mode)0;

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
    shot->ctl.aa.afTrigger = (enum aa_af_trigger)0;
    shot->ctl.aa.vendor_isoMode = AA_ISOMODE_AUTO;
    shot->ctl.aa.vendor_isoValue = 0;

    /* 2. dm */

    /* 3. utrl */
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

status_t ExynosCamera3Parameters::duplicateCtrlMetadata(void *buf)
{
    if (buf == NULL) {
        CLOGE2("buf is NULL");
        return BAD_VALUE;
    }

    struct camera2_shot_ext *meta_shot_ext = (struct camera2_shot_ext *)buf;
    memcpy(&meta_shot_ext->shot.ctl, &m_metadata.shot.ctl, sizeof(struct camera2_ctl));
    meta_shot_ext->shot.uctl.vtMode = m_metadata.shot.uctl.vtMode;

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::setFrameSkipCount(int count)
{
    m_frameSkipCounter.setCount(count);

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::getFrameSkipCount(int *count)
{
    *count = m_frameSkipCounter.getCount();
    m_frameSkipCounter.decCount();

    return NO_ERROR;
}

int ExynosCamera3Parameters::getFrameSkipCount(void)
{
    return m_frameSkipCounter.getCount();
}

void ExynosCamera3Parameters::setIsFirstStartFlag(bool flag)
{
    m_flagFirstStart = flag;
}

int ExynosCamera3Parameters::getIsFirstStartFlag(void)
{
    return m_flagFirstStart;
}

ExynosCameraActivityControl *ExynosCamera3Parameters::getActivityControl(void)
{
    return m_activityControl;
}

status_t ExynosCamera3Parameters::setAutoFocusMacroPosition(int autoFocusMacroPosition)
{
    int oldAutoFocusMacroPosition = m_cameraInfo.autoFocusMacroPosition;
    m_cameraInfo.autoFocusMacroPosition = autoFocusMacroPosition;

    m_activityControl->setAutoFocusMacroPosition(oldAutoFocusMacroPosition, autoFocusMacroPosition);

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::setDisEnable(bool enable)
{
    setMetaBypassDis(&m_metadata, enable == true ? 0 : 1);
    return NO_ERROR;
}

bool ExynosCamera3Parameters::getDisEnable(void)
{
    return m_metadata.dis_bypass;
}

status_t ExynosCamera3Parameters::setDrcEnable(bool enable)
{
    setMetaBypassDrc(&m_metadata, enable == true ? 0 : 1);
    return NO_ERROR;
}

bool ExynosCamera3Parameters::getDrcEnable(void)
{
    return m_metadata.drc_bypass;
}

status_t ExynosCamera3Parameters::setDnrEnable(bool enable)
{
    setMetaBypassDnr(&m_metadata, enable == true ? 0 : 1);
    return NO_ERROR;
}

bool ExynosCamera3Parameters::getDnrEnable(void)
{
    return m_metadata.dnr_bypass;
}

status_t ExynosCamera3Parameters::setFdEnable(bool enable)
{
    setMetaBypassFd(&m_metadata, enable == true ? 0 : 1);
    return NO_ERROR;
}

bool ExynosCamera3Parameters::getFdEnable(void)
{
    return m_metadata.fd_bypass;
}

status_t ExynosCamera3Parameters::setFdMode(enum facedetect_mode mode)
{
    setMetaCtlFdMode(&m_metadata, mode);
    return NO_ERROR;
}

status_t ExynosCamera3Parameters::getFdMeta(bool reprocessing, void *buf)
{
    if (buf == NULL) {
        CLOGE2("buf is NULL");
        return BAD_VALUE;
    }

    struct camera2_shot_ext *meta_shot_ext = (struct camera2_shot_ext *)buf;

    /* disable face detection for reprocessing frame */
    if (reprocessing) {
        meta_shot_ext->fd_bypass = 1;
        meta_shot_ext->shot.ctl.stats.faceDetectMode = ::FACEDETECT_MODE_OFF;
    }

    return NO_ERROR;
}

void ExynosCamera3Parameters::setFlipHorizontal(int val)
{
    if (val < 0) {
        CLOGE2("setFlipHorizontal ignored, invalid value(%d)", val);
        return;
    }

    m_cameraInfo.flipHorizontal = val;
}

int ExynosCamera3Parameters::getFlipHorizontal(void)
{
    if (m_cameraInfo.shotMode == SHOT_MODE_FRONT_PANORAMA) {
        return 0;
    } else {
        return m_cameraInfo.flipHorizontal;
    }
}

void ExynosCamera3Parameters::setFlipVertical(int val)
{
    if (val < 0) {
        CLOGE2("setFlipVertical ignored, invalid value(%d)", val);
        return;
    }

    m_cameraInfo.flipVertical = val;
}

int ExynosCamera3Parameters::getFlipVertical(void)
{
    return m_cameraInfo.flipVertical;
}

bool ExynosCamera3Parameters::getCallbackNeedCSC(void)
{
    bool ret = true;

    int previewW = 0, previewH = 0;
    int hwPreviewW = 0, hwPreviewH = 0;
    int previewFormat = getPreviewFormat();

    getPreviewSize(&previewW, &previewH);
    getHwPreviewSize(&hwPreviewW, &hwPreviewH);
    if ((previewW == hwPreviewW)&&
        (previewH == hwPreviewH)&&
        (previewFormat == V4L2_PIX_FMT_NV21)) {
        ret = false;
    }
    return ret;
}

bool ExynosCamera3Parameters::getCallbackNeedCopy2Rendering(void)
{
    bool ret = false;

    int previewW = 0, previewH = 0;

    getPreviewSize(&previewW, &previewH);
    if (previewW * previewH <= 1920*1080)
        ret = true;

    return ret;
}

bool ExynosCamera3Parameters::setDeviceOrientation(int orientation)
{
    if (orientation < 0 || orientation % 90 != 0) {
        CLOGE2("Invalid orientation (%d)", orientation);
        return false;
    }

    m_cameraInfo.deviceOrientation = orientation;

    /* fd orientation need to be calibrated, according to f/w spec */
    int hwRotation = BACK_ROTATION;

#if 0
    if (this->getCameraId() == CAMERA_ID_FRONT)
        hwRotation = FRONT_ROTATION;
#endif

    int fdOrientation = (orientation + hwRotation) % 360;

    CLOGD2("orientation(%d), hwRotation(%d), fdOrientation(%d)", orientation, hwRotation, fdOrientation);

    return true;
}

int ExynosCamera3Parameters::getDeviceOrientation(void)
{
    return m_cameraInfo.deviceOrientation;
}

int ExynosCamera3Parameters::getFdOrientation(void)
{
    /* HACK: Calibrate FRONT FD orientation */
    if (getCameraId() == CAMERA_ID_FRONT)
        return (m_cameraInfo.deviceOrientation + FRONT_ROTATION + 180) % 360;
    else
        return (m_cameraInfo.deviceOrientation + BACK_ROTATION) % 360;
}

void ExynosCamera3Parameters::getSetfileYuvRange(bool flagReprocessing, int *setfile, int *yuvRange)
{
    if (flagReprocessing == true) {
        *setfile = m_setfileReprocessing;
        *yuvRange = m_yuvRangeReprocessing;
    } else {
        *setfile = m_setfile;
        *yuvRange = m_yuvRange;
    }
}

void ExynosCamera3Parameters::setSetfileYuvRange(void)
{

    /* general */
    m_getSetfileYuvRange(false, &m_setfile, &m_yuvRange);

    /* reprocessing */
    m_getSetfileYuvRange(true, &m_setfileReprocessing, &m_yuvRangeReprocessing);

    CLOGD2("m_cameraId(%d) : general[setfile(%d) YUV range(%d)] : reprocesing[setfile(%d) YUV range(%d)]",
        m_cameraId,
        m_setfile, m_yuvRange,
        m_setfileReprocessing, m_yuvRangeReprocessing);

}

void ExynosCamera3Parameters::m_getSetfileYuvRange(bool flagReprocessing, int *setfile, int *yuvRange)
{
    uint32_t currentSetfile = 0;
    uint32_t stateReg = 0;
    int flagYUVRange = YUV_FULL_RANGE;

    unsigned int minFps = 0;
    unsigned int maxFps = 0;
    getPreviewFpsRange(&minFps, &maxFps);

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
                if(stateReg == STATE_STILL_CAPTURE)
                    currentSetfile = ISS_SUB_SCENARIO_FRONT_VT1_STILL_CAPTURE;
                break;
            case 2:
                currentSetfile = ISS_SUB_SCENARIO_FRONT_VT2;
                break;
            case 4:
                currentSetfile = ISS_SUB_SCENARIO_FRONT_VT4;
                break;
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
                        currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW;
                    break;

                case STATE_STILL_PREVIEW_WDR_ON:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_ON;
                    break;

                case STATE_STILL_PREVIEW_WDR_AUTO:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_AUTO;
                    break;

                case STATE_VIDEO:
                        currentSetfile = ISS_SUB_SCENARIO_VIDEO;
                    break;

                case STATE_VIDEO_WDR_ON:
                case STATE_UHD_VIDEO_WDR_ON:
                    currentSetfile = ISS_SUB_SCENARIO_VIDEO_WDR_ON;
                    break;

                case STATE_VIDEO_WDR_AUTO:
                case STATE_UHD_VIDEO_WDR_AUTO:
                    currentSetfile = ISS_SUB_SCENARIO_VIDEO_WDR_AUTO;
                    break;

                case STATE_STILL_CAPTURE:
                case STATE_VIDEO_CAPTURE:
                case STATE_UHD_PREVIEW_CAPTURE:
                case STATE_UHD_VIDEO_CAPTURE:
                        currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE;
                    break;

                case STATE_STILL_CAPTURE_WDR_ON:
                case STATE_VIDEO_CAPTURE_WDR_ON:
                case STATE_UHD_PREVIEW_CAPTURE_WDR_ON:
                case STATE_UHD_VIDEO_CAPTURE_WDR_ON:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON;
                    break;

                case STATE_STILL_CAPTURE_WDR_AUTO:
                case STATE_VIDEO_CAPTURE_WDR_AUTO:
                case STATE_UHD_PREVIEW_CAPTURE_WDR_AUTO:
                case STATE_UHD_VIDEO_CAPTURE_WDR_AUTO:
                    currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_AUTO;
                    break;

                case STATE_DUAL_STILL_PREVIEW:
                case STATE_DUAL_STILL_CAPTURE:
                case STATE_DUAL_VIDEO_CAPTURE:
                    currentSetfile = ISS_SUB_SCENARIO_DUAL_STILL;
                    break;

                case STATE_DUAL_VIDEO:
                    currentSetfile = ISS_SUB_SCENARIO_DUAL_VIDEO;
                    break;

                case STATE_UHD_PREVIEW:
                case STATE_UHD_VIDEO:
                    currentSetfile = ISS_SUB_SCENARIO_UHD_30FPS;
                    break;

                default:
                    CLOGD2("can't define senario of setfile.(0x%4x)", stateReg);
                    break;
            }
        }
    } else {
        switch(stateReg) {
            case STATE_STILL_PREVIEW:
                currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW;
                break;

            case STATE_STILL_PREVIEW_WDR_ON:
                currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_ON;
                break;

            case STATE_STILL_PREVIEW_WDR_AUTO:
                currentSetfile = ISS_SUB_SCENARIO_STILL_PREVIEW_WDR_AUTO;
                break;

            case STATE_STILL_CAPTURE:
            case STATE_VIDEO_CAPTURE:
            case STATE_DUAL_STILL_CAPTURE:
            case STATE_DUAL_VIDEO_CAPTURE:
            case STATE_UHD_PREVIEW_CAPTURE:
            case STATE_UHD_VIDEO_CAPTURE:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE;
                break;

            case STATE_STILL_CAPTURE_WDR_ON:
            case STATE_VIDEO_CAPTURE_WDR_ON:
            case STATE_UHD_PREVIEW_CAPTURE_WDR_ON:
            case STATE_UHD_VIDEO_CAPTURE_WDR_ON:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_ON;
                break;

            case STATE_STILL_CAPTURE_WDR_AUTO:
            case STATE_VIDEO_CAPTURE_WDR_AUTO:
            case STATE_UHD_PREVIEW_CAPTURE_WDR_AUTO:
            case STATE_UHD_VIDEO_CAPTURE_WDR_AUTO:
                currentSetfile = ISS_SUB_SCENARIO_STILL_CAPTURE_WDR_AUTO;
                break;

            case STATE_VIDEO:
                if (30 < minFps && 30 < maxFps) {
                    if (300 == minFps && 300 == maxFps) {
                        currentSetfile = ISS_SUB_SCENARIO_WVGA_300FPS;
                    } else if (60 == minFps && 60 == maxFps) {
                        currentSetfile = ISS_SUB_SCENARIO_FHD_60FPS;
                    } else if (240 == minFps && 240 == maxFps) {
                        currentSetfile = ISS_SUB_SCENARIO_FHD_240FPS;
                    } else {
                        currentSetfile = ISS_SUB_SCENARIO_VIDEO_HIGH_SPEED;
                    }
                } else {
                    currentSetfile = ISS_SUB_SCENARIO_VIDEO;
                }
                break;

            case STATE_VIDEO_WDR_ON:
                currentSetfile = ISS_SUB_SCENARIO_VIDEO_WDR_ON;
                break;

            case STATE_VIDEO_WDR_AUTO:
                currentSetfile = ISS_SUB_SCENARIO_VIDEO_WDR_AUTO;
                break;

            case STATE_DUAL_VIDEO:
                currentSetfile = ISS_SUB_SCENARIO_DUAL_VIDEO;
                break;

            case STATE_DUAL_STILL_PREVIEW:
                currentSetfile = ISS_SUB_SCENARIO_DUAL_STILL;
                break;

            case STATE_UHD_PREVIEW:
            case STATE_UHD_VIDEO:
                currentSetfile = ISS_SUB_SCENARIO_UHD_30FPS;
                break;

            case STATE_UHD_PREVIEW_WDR_ON:
            case STATE_UHD_VIDEO_WDR_ON:
                currentSetfile = ISS_SUB_SCENARIO_UHD_30FPS_WDR_ON;
                break;

            default:
                CLOGD2("can't define senario of setfile.(0x%4x)", stateReg);
                break;
        }
    }
#if 0
    CLOGD2("===============================================================================");
    CLOGD2("CurrentState(0x%4x)", stateReg);
    CLOGD2("getRTHdr()(%d)", getRTHdr());
    CLOGD2("getRecordingHint()(%d)", getRecordingHint());
    CLOGD2("m_isUHDRecordingMode()(%d)", m_isUHDRecordingMode());
    CLOGD2("getDualMode()(%d)", getDualMode());
    CLOGD2("getDualRecordingHint()(%d)", getDualRecordingHint());
    CLOGD2("flagReprocessing(%d)", flagReprocessing);
    CLOGD2(" ===============================================================================");
    CLOGD2("currentSetfile(%d)", currentSetfile);
    CLOGD2("flagYUVRange(%d)", flagYUVRange);
    CLOGD2("===============================================================================");
#else
    CLOGD2("CurrentState (0x%4x), currentSetfile(%d)", stateReg, currentSetfile);
#endif

done:
    *setfile = currentSetfile;
    *yuvRange = flagYUVRange;
}

void ExynosCamera3Parameters::setUseDynamicBayer(bool enable)
{
    m_useDynamicBayer = enable;
}

bool ExynosCamera3Parameters::getUseDynamicBayer(void)
{
    return m_useDynamicBayer;
}

void ExynosCamera3Parameters::setUseDynamicBayerVideoSnapShot(bool enable)
{
    m_useDynamicBayerVideoSnapShot = enable;
}

bool ExynosCamera3Parameters::getUseDynamicBayerVideoSnapShot(void)
{
    return m_useDynamicBayerVideoSnapShot;
}

void ExynosCamera3Parameters::setUseDynamicScc(bool enable)
{
    m_useDynamicScc = enable;
}

bool ExynosCamera3Parameters::getUseDynamicScc(void)
{
    bool dynamicScc = m_useDynamicScc;
    bool reprocessing = isReprocessing();

    if (getRecordingHint() == true && reprocessing == false)
        dynamicScc = false;

    return dynamicScc;
}

void ExynosCamera3Parameters::setUseFastenAeStable(bool enable)
{
    m_useFastenAeStable = enable;
}

bool ExynosCamera3Parameters::getUseFastenAeStable(void)
{
    return m_useFastenAeStable;
}

status_t ExynosCamera3Parameters::calcPreviewGSCRect(ExynosRect *srcRect, ExynosRect *dstRect)
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

    ret = getCropRectAlign(srcRect->w, srcRect->h,
                           previewW, previewH,
                           &srcRect->x, &srcRect->y,
                           &srcRect->w, &srcRect->h,
                           2, 2,
                           0, 1);

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

status_t ExynosCamera3Parameters::calcHighResolutionPreviewGSCRect(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;

    int previewW = 0, previewH = 0, previewFormat = 0;
    int pictureW = 0, pictureH = 0, pictureFormat = 0;
    previewFormat = getPreviewFormat();
    pictureFormat = getPictureFormat();

    if (isOwnScc(m_cameraId) == true)
    getPictureSize(&pictureW, &pictureH);
    else
        getHwPictureSize(&pictureW, &pictureH);
    getPreviewSize(&previewW, &previewH);

    srcRect->x = 0;
    srcRect->y = 0;
    srcRect->w = pictureW;
    srcRect->h = pictureH;
    srcRect->fullW = pictureW;
    srcRect->fullH = pictureH;
    srcRect->colorFormat = pictureFormat;

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = previewW;
    dstRect->h = previewH;
    dstRect->fullW = previewW;
    dstRect->fullH = previewH;
    dstRect->colorFormat = previewFormat;

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::calcRecordingGSCRect(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;

    int hwPreviewW = 0, hwPreviewH = 0, hwPreviewFormat = 0;
    int videoW = 0, videoH = 0, videoFormat = 0;

    hwPreviewFormat = getHwPreviewFormat();
    videoFormat     = getVideoFormat();

    getHwPreviewSize(&hwPreviewW, &hwPreviewH);
    getVideoSize(&videoW, &videoH);

    ret = getCropRectAlign(srcRect->w, srcRect->h,
                           videoW, videoH,
                           &srcRect->x, &srcRect->y,
                           &srcRect->w, &srcRect->h,
                           2, 2,
                           0, 1);

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

status_t ExynosCamera3Parameters::calcPictureRect(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int hwSensorW = 0, hwSensorH = 0;
    int hwPictureW = 0, hwPictureH = 0, hwPictureFormat = 0;
    int pictureW = 0, pictureH = 0, pictureFormat = 0;
    int previewW = 0, previewH = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;

    int zoomLevel = 0;
    int bayerFormat = CAMERA_BAYER_FORMAT;
    float zoomRatio = 1.0f;

    /* TODO: check state ready for start */
    pictureFormat = getPictureFormat();
    zoomLevel = getZoomLevel();
    getHwPictureSize(&hwPictureW, &hwPictureH);
    getPictureSize(&pictureW, &pictureH);

    getHwSensorSize(&hwSensorW, &hwSensorH);
    getPreviewSize(&previewW, &previewH);

    zoomRatio = getZoomRatio(zoomLevel) / 1000;
    /* TODO: get crop size from ctlMetadata */
    ret = getCropRectAlign(hwSensorW, hwSensorH,
                     previewW, previewH,
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

    ALIGN_UP(crop_crop_x, 2);
    ALIGN_UP(crop_crop_y, 2);

#if 0
    CLOGD2("hwSensorSize (%dx%d), previewSize (%dx%d)",
            hwSensorW, hwSensorH, previewW, previewH);
    CLOGD2("hwPictureSize (%dx%d), pictureSize (%dx%d)",
            hwPictureW, hwPictureH, pictureW, pictureH);
    CLOGD2("size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            cropX, cropY, cropW, cropH, zoomLevel);
    CLOGD2("size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    CLOGD2("size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            pictureFormat, JPEG_INPUT_COLOR_FMT);
#endif

    srcRect->x = crop_crop_x;
    srcRect->y = crop_crop_y;
    srcRect->w = crop_crop_w;
    srcRect->h = crop_crop_h;
    srcRect->fullW = cropW;
    srcRect->fullH = cropH;
    srcRect->colorFormat = pictureFormat;

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = pictureW;
    dstRect->h = pictureH;
    dstRect->fullW = pictureW;
    dstRect->fullH = pictureH;
    dstRect->colorFormat = JPEG_INPUT_COLOR_FMT;

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::calcPictureRect(int originW, int originH, ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;
    int pictureW = 0, pictureH = 0, pictureFormat = 0;

    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;
    float zoomRatio = getZoomRatio(0) / 1000;
#if 0
    int zoom = 0;
    int bayerFormat = CAMERA_BAYER_FORMAT;
#endif
    /* TODO: check state ready for start */
    pictureFormat = getPictureFormat();
    getPictureSize(&pictureW, &pictureH);

    /* TODO: get crop size from ctlMetadata */
    ret = getCropRectAlign(originW, originH,
                     pictureW, pictureH,
                     &crop_crop_x, &crop_crop_y,
                     &crop_crop_w, &crop_crop_h,
                     2, 2,
                     0, zoomRatio);

    ALIGN_UP(crop_crop_x, 2);
    ALIGN_UP(crop_crop_y, 2);

#if 0
    CLOGD2("originSize (%dx%d) pictureSize (%dx%d)",
            originW, originH, pictureW, pictureH);
    CLOGD2("size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoom);
    CLOGD2("size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            pictureFormat, JPEG_INPUT_COLOR_FMT);
#endif

    srcRect->x = crop_crop_x;
    srcRect->y = crop_crop_y;
    srcRect->w = crop_crop_w;
    srcRect->h = crop_crop_h;
    srcRect->fullW = originW;
    srcRect->fullH = originH;
    srcRect->colorFormat = pictureFormat;

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = pictureW;
    dstRect->h = pictureH;
    dstRect->fullW = pictureW;
    dstRect->fullH = pictureH;
    dstRect->colorFormat = JPEG_INPUT_COLOR_FMT;

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::getPreviewBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int hwBnsW   = 0;
    int hwBnsH   = 0;
    int hwBcropW = 0;
    int hwBcropH = 0;
    int zoomLevel = 0;
    float zoomRatio = 1.00f;
    int sizeList[SIZE_LUT_INDEX_END];
    int hwSensorMarginW = 0;
    int hwSensorMarginH = 0;
    float bnsRatio = 0;

    /* matched ratio LUT is not existed, use equation */
    if (m_useSizeTable == false
        || m_staticInfo->previewSizeLut == NULL
        || m_staticInfo->previewSizeLutMax <= m_cameraInfo.previewSizeRatioId
        || (getUsePureBayerReprocessing() == false &&
            m_cameraInfo.pictureSizeRatioId != m_cameraInfo.previewSizeRatioId)
        || m_getPreviewSizeList(sizeList) != NO_ERROR)
        return calcPreviewBayerCropSize(srcRect, dstRect);

    /* use LUT */
    hwBnsW = sizeList[BNS_W];
    hwBnsH = sizeList[BNS_H];
    hwBcropW = sizeList[BCROP_W];
    hwBcropH = sizeList[BCROP_H];

    if (getRecordingHint() == true) {
        if (m_cameraInfo.previewSizeRatioId != m_cameraInfo.videoSizeRatioId) {
            CLOGV2("preview ratioId(%d) != videoRatioId(%d), use previewRatioId",
                m_cameraInfo.previewSizeRatioId, m_cameraInfo.videoSizeRatioId);
        }
    }

    int curBnsW = 0, curBnsH = 0;
    getBnsSize(&curBnsW, &curBnsH);
    if (SIZE_RATIO(curBnsW, curBnsH) != SIZE_RATIO(hwBnsW, hwBnsH))
        CLOGW2("current BNS size(%dx%d) is NOT same with Hw BNS size(%dx%d)",
                curBnsW, curBnsH, hwBnsW, hwBnsH);

    zoomLevel = getZoomLevel();
    zoomRatio = getZoomRatio(zoomLevel) / 1000;

    /* Skip to calculate the crop size with zoom level
     * Condition 1 : High-speed camcording D-zoom with External Scaler
     * Condition 2 : HAL3 (Service calculates the crop size by itself
     */
    int fastFpsMode = getFastFpsMode();
    if ((fastFpsMode > CONFIG_MODE::HIGHSPEED_60 &&
        fastFpsMode < CONFIG_MODE::MAX &&
        getZoomPreviewWIthScaler() == true) ||
        getHalVersion() == IS_HAL_VER_3_2) {
        CLOGV2("hwBnsSize (%dx%d), hwBcropSize(%dx%d), fastFpsMode(%d)",
                hwBnsW, hwBnsH,
                hwBcropW, hwBcropH,
                fastFpsMode);
    } else {
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

    /* Re-calculate the BNS size for removing Sensor Margin */
    getSensorMargin(&hwSensorMarginW, &hwSensorMarginH);
    m_adjustSensorMargin(&hwSensorMarginW, &hwSensorMarginH);

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
            CLOGV2("hwBcrop (%d x %d)", hwBcropW, hwBcropH);
            ret = getCropRectAlign(cropRegionW, cropRegionH,
                    hwBcropW, hwBcropH,
                    &(dstRect->x), &(dstRect->y),
                    &(dstRect->w), &(dstRect->h),
                    CAMERA_BCROP_ALIGN, 2,
                    0, 0.0f);
            dstRect->x = ALIGN_DOWN((cropRegionX + dstRect->x), 2);
            dstRect->y = ALIGN_DOWN((cropRegionY + dstRect->y), 2);
        }
    } else {
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

    m_setHwBayerCropRegion(dstRect->w, dstRect->h, dstRect->x, dstRect->y);
#ifdef DEBUG_PERFRAME
    CLOGD2("zoomLevel=%d", zoomLevel);
    CLOGD2("hwBnsSize (%dx%d), hwBcropSize (%d, %d)(%dx%d)",
            srcRect->w, srcRect->h, dstRect->x, dstRect->y, dstRect->w, dstRect->h);
#endif

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::calcPreviewBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
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
    pictureFormat = getPictureFormat();
#endif
    zoomLevel = getZoomLevel();
    maxZoomRatio = getMaxZoomRatio() / 1000;
    getHwPictureSize(&hwPictureW, &hwPictureH);
    getPictureSize(&pictureW, &pictureH);

    getHwSensorSize(&hwSensorW, &hwSensorH);
    getPreviewSize(&previewW, &previewH);
    getSensorMargin(&hwSensorMarginW, &hwSensorMarginH);
    /* This function is disabled, because is not necessary.
       m_adjustSensorMargin function use sensorMarginBase[] size,
       but, here not using sensorMarginBase[] size.
    */
    //m_adjustSensorMargin(&hwSensorMarginW, &hwSensorMarginH);

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
        cropW = hwSensorW - (cropX * 2);
        cropH = hwSensorH - (cropY * 2);
    }

    if (getUsePureBayerReprocessing() == false
        && pictureW > 0 && pictureH > 0) {
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
            CLOGW2("zoom ratio is upto x%d, crop(%dx%d), picture(%dx%d)", maxZoomRatio, cropW, cropH, pictureW, pictureH);
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
    CLOGD2("hwSensorSize (%dx%d), previewSize (%dx%d)",
            hwSensorW, hwSensorH, previewW, previewH);
    CLOGD2("hwPictureSize (%dx%d), pictureSize (%dx%d)",
            hwPictureW, hwPictureH, pictureW, pictureH);
    CLOGD2("size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            cropX, cropY, cropW, cropH, zoomLevel);
    CLOGD2("size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    CLOGD2("size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            pictureFormat, JPEG_INPUT_COLOR_FMT);
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

status_t ExynosCamera3Parameters::calcPreviewDzoomCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
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

    CLOGV2("SRC cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d ratio = %f", srcRect->x, srcRect->y, srcRect->w, srcRect->h, zoomLevel, zoomRatio);
    CLOGV2("DST cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d ratio = %f", dstRect->x, dstRect->y, dstRect->w, dstRect->h, zoomLevel, zoomRatio);

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::getPictureBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
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
    CLOGD2("zoomRatio=%f", zoomRatio);
    CLOGD2("hwBnsSize (%dx%d), hwBcropSize (%d, %d)(%dx%d)",
            srcRect->w, srcRect->h, dstRect->x, dstRect->y, dstRect->w, dstRect->h);
#endif

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::calcPictureBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
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
    pictureFormat = getPictureFormat();
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
            CLOGW2("zoom ratio is upto x%d, crop(%dx%d), picture(%dx%d)", maxZoomRatio, cropW, cropH, pictureW, pictureH);
            cropX = ALIGN_DOWN(((hwSensorW - (pictureW / maxZoomRatio)) >> 1), 2);
            cropY = ALIGN_DOWN(((hwSensorH - (pictureH / maxZoomRatio)) >> 1), 2);
            cropW = hwSensorW - (cropX * 2);
            cropH = hwSensorH - (cropY * 2);
        }
    } else {
        zoomLevel = 0;
        if (getHalVersion() == IS_HAL_VER_3_2)
            zoomRatio = 0.0f;
        else
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
            CLOGW2("zoom ratio is upto x%d, crop(%dx%d), picture(%dx%d)",
                    maxZoomRatio, cropW, cropH, pictureW, pictureH);
            cropX = ALIGN_DOWN(((hwSensorCropW - (pictureW / maxZoomRatio)) >> 1), 2);
            cropY = ALIGN_DOWN(((hwSensorCropH - (pictureH / maxZoomRatio)) >> 1), 2);
            cropW = hwSensorCropW - (cropX * 2);
            cropH = hwSensorCropH - (cropY * 2);
        }
    }

#if 1
    CLOGD2("maxSensorSize (%dx%d), hwSensorSize (%dx%d), previewSize (%dx%d)",
            maxSensorW, maxSensorH, hwSensorW, hwSensorH, previewW, previewH);
    CLOGD2("hwPictureSize (%dx%d), pictureSize (%dx%d)",
            hwPictureW, hwPictureH, pictureW, pictureH);
    CLOGD2("size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            cropX, cropY, cropW, cropH, zoomLevel);
    CLOGD2("size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    CLOGD2("size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            pictureFormat, JPEG_INPUT_COLOR_FMT);
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

status_t ExynosCamera3Parameters::m_getPreviewBdsSize(ExynosRect *dstRect)
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
            CLOGV2("preview ratioId(%d) != videoRatioId(%d), use previewRatioId",
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
    CLOGD2("hwBdsSize (%dx%d)", dstRect->w, dstRect->h);
#endif

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::getPreviewBdsSize(ExynosRect *dstRect)
{
    status_t ret = NO_ERROR;

    ret = m_getPreviewBdsSize(dstRect);
    if (ret != NO_ERROR) {
        CLOGE2("calcPreviewBDSSize() fail");
        return ret;
    }

    if (this->getHWVdisMode() == true) {
        int disW = ALIGN_UP((int)(dstRect->w * HW_VDIS_W_RATIO), 2);
        int disH = ALIGN_UP((int)(dstRect->h * HW_VDIS_H_RATIO), 2);

        CLOGV2("HWVdis adjusted BDS Size (%d x %d) -> (%d x %d)", dstRect->w, dstRect->h, disW, disH);

        /*
         * check H/W VDIS size(BDS dst size) is too big than bayerCropSize(BDS out size).
         */
        ExynosRect bnsSize, bayerCropSize;

        if (getPreviewBayerCropSize(&bnsSize, &bayerCropSize) != NO_ERROR) {
            CLOGE2("getPreviewBayerCropSize() fail");
        } else {
            if (bayerCropSize.w < disW || bayerCropSize.h < disH) {
                CLOGV2("bayerCropSize (%d x %d) is smaller than (%d x %d). so force bayerCropSize",
                       bayerCropSize.w, bayerCropSize.h, disW, disH);

                disW = bayerCropSize.w;
                disH = bayerCropSize.h;
            }
        }

        dstRect->w = disW;
        dstRect->h = disH;
    }

#ifdef DEBUG_PERFRAME
    CLOGD2("hwBdsSize (%dx%d)", dstRect->w, dstRect->h);
#endif

    return ret;
}

status_t ExynosCamera3Parameters::calcPreviewBDSSize(ExynosRect *srcRect, ExynosRect *dstRect)
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
    pictureFormat = getPictureFormat();
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
    CLOGD2("hwSensorSize (%dx%d), previewSize (%dx%d)",
            hwSensorW, hwSensorH, previewW, previewH);
    CLOGD2("hwPictureSize (%dx%d), pictureSize (%dx%d)",
            hwPictureW, hwPictureH, pictureW, pictureH);
    CLOGD2("size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            cropX, cropY, cropW, cropH, zoomLevel);
    CLOGD2("size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    CLOGD2("size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            pictureFormat, JPEG_INPUT_COLOR_FMT);
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

status_t ExynosCamera3Parameters::getPictureBdsSize(ExynosRect *dstRect)
{
    int hwBdsW = 0;
    int hwBdsH = 0;

    /* matched ratio LUT is not existed, use equation */
    if (m_useSizeTable == false
        || m_staticInfo->pictureSizeLut == NULL
        || m_staticInfo->pictureSizeLutMax <= m_cameraInfo.pictureSizeRatioId) {
        ExynosRect rect;
        return calcPictureBDSSize(&rect, dstRect);
    }

    /* use LUT */
    hwBdsW = m_staticInfo->pictureSizeLut[m_cameraInfo.pictureSizeRatioId][BDS_W];
    hwBdsH = m_staticInfo->pictureSizeLut[m_cameraInfo.pictureSizeRatioId][BDS_H];

    dstRect->x = 0;
    dstRect->y = 0;
    dstRect->w = hwBdsW;
    dstRect->h = hwBdsH;

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::getFastenAeStableSensorSize(int *hwSensorW, int *hwSensorH)
{
    *hwSensorW = m_staticInfo->videoSizeLutHighSpeed[0][SENSOR_W];
    *hwSensorH = m_staticInfo->videoSizeLutHighSpeed[0][SENSOR_H];

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::getFastenAeStableBcropSize(int *hwBcropW, int *hwBcropH)
{
    *hwBcropW = m_staticInfo->videoSizeLutHighSpeed[0][BCROP_W];
    *hwBcropH = m_staticInfo->videoSizeLutHighSpeed[0][BCROP_H];

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::getFastenAeStableBdsSize(int *hwBdsW, int *hwBdsH)
{
    *hwBdsW = m_staticInfo->videoSizeLutHighSpeed[0][BDS_W];
    *hwBdsH = m_staticInfo->videoSizeLutHighSpeed[0][BDS_H];

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::calcPictureBDSSize(ExynosRect *srcRect, ExynosRect *dstRect)
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
    pictureFormat = getPictureFormat();
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
    CLOGD2("SensorSize (%dx%d), previewSize (%dx%d)",
            maxSensorW, maxSensorH, previewW, previewH);
    CLOGD2("hwPictureSize (%dx%d), pictureSize (%dx%d)",
            hwPictureW, hwPictureH, pictureW, pictureH);
    CLOGD2("size cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            cropX, cropY, cropW, cropH, zoomLevel);
    CLOGD2("size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoomLevel);
    CLOGD2("size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            pictureFormat, JPEG_INPUT_COLOR_FMT);
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

status_t ExynosCamera3Parameters::calcNormalToTpuSize(int srcW, int srcH, int *dstW, int *dstH)
{
    status_t ret = NO_ERROR;
    if (srcW < 0 || srcH < 0) {
        CLOGE2("src size is invalid(%d x %d)", srcW, srcH);
        return INVALID_OPERATION;
    }

    int disW = ALIGN_UP((int)(srcW * HW_VDIS_W_RATIO), 2);
    int disH = ALIGN_UP((int)(srcH * HW_VDIS_H_RATIO), 2);

    *dstW = disW;
    *dstH = disH;
    CLOGD2("HWVdis adjusted BDS Size (%d x %d) -> (%d x %d)", srcW, srcH, disW, disH);

    return NO_ERROR;
}

status_t ExynosCamera3Parameters::calcTpuToNormalSize(int srcW, int srcH, int *dstW, int *dstH)
{
    status_t ret = NO_ERROR;
    if (srcW < 0 || srcH < 0) {
        CLOGE2("src size is invalid(%d x %d)", srcW, srcH);
        return INVALID_OPERATION;
    }

    int disW = ALIGN_DOWN((int)(srcW / HW_VDIS_W_RATIO), 2);
    int disH = ALIGN_DOWN((int)(srcH / HW_VDIS_H_RATIO), 2);

    *dstW = disW;
    *dstH = disH;
    CLOGD2("HWVdis adjusted BDS Size (%d x %d) -> (%d x %d)", srcW, srcH, disW, disH);

    return ret;
}

void ExynosCamera3Parameters::setUsePureBayerReprocessing(bool enable)
{
    m_usePureBayerReprocessing = enable;
}

bool ExynosCamera3Parameters::getUsePureBayerReprocessing(void)
{
    int oldMode = m_usePureBayerReprocessing;

    if (getRecordingHint() == true) {
        if (getDualMode() == true)
            m_usePureBayerReprocessing = (getCameraId() == CAMERA_ID_BACK) ? USE_PURE_BAYER_REPROCESSING_ON_DUAL_RECORDING : USE_PURE_BAYER_REPROCESSING_FRONT_ON_DUAL_RECORDING;
        else
            m_usePureBayerReprocessing = (getCameraId() == CAMERA_ID_BACK) ? USE_PURE_BAYER_REPROCESSING_ON_RECORDING : USE_PURE_BAYER_REPROCESSING_FRONT_ON_RECORDING;
    } else {
        if (getDualMode() == true)
            m_usePureBayerReprocessing = (getCameraId() == CAMERA_ID_BACK) ? USE_PURE_BAYER_REPROCESSING_ON_DUAL : USE_PURE_BAYER_REPROCESSING_FRONT_ON_DUAL;
        else
            m_usePureBayerReprocessing = (getCameraId() == CAMERA_ID_BACK) ? USE_PURE_BAYER_REPROCESSING : USE_PURE_BAYER_REPROCESSING_FRONT;
    }

    if (oldMode != m_usePureBayerReprocessing) {
        CLOGD2("bayer usage is changed (%d -> %d)", oldMode, m_usePureBayerReprocessing);
    }

    return m_usePureBayerReprocessing;
}

bool ExynosCamera3Parameters::isUseYuvReprocessing(void)
{
    bool ret = false;

#ifdef USE_YUV_REPROCESSING
    ret = USE_YUV_REPROCESSING;
#endif

    return ret;
}

bool ExynosCamera3Parameters::isUseYuvReprocessingForThumbnail(void)
{
    bool ret = false;

#ifdef USE_YUV_REPROCESSING_FOR_THUMBNAIL
    if (isUseYuvReprocessing() == true)
        ret = USE_YUV_REPROCESSING_FOR_THUMBNAIL;
#endif

    return ret;
}

int32_t ExynosCamera3Parameters::getReprocessingBayerMode(void)
{
    int32_t mode = REPROCESSING_BAYER_MODE_NONE;
    bool useDynamicBayer = (getRecordingHint() == true || getDualRecordingHint() == true) ?
        getUseDynamicBayerVideoSnapShot() : getUseDynamicBayer();

    if (isReprocessing() == false)
        return mode;

    if (useDynamicBayer == true) {
        if (getUsePureBayerReprocessing() == true)
            mode = REPROCESSING_BAYER_MODE_PURE_DYNAMIC;
        else
            mode = REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC;
    } else {
        if (getUsePureBayerReprocessing() == true)
            mode = REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON;
        else
            mode = REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON;
    }

    return mode;
}

void ExynosCamera3Parameters::setAdaptiveCSCRecording(bool enable)
{
    m_useAdaptiveCSCRecording = enable;
}

bool ExynosCamera3Parameters::getAdaptiveCSCRecording(void)
{
    return m_useAdaptiveCSCRecording;
}

bool ExynosCamera3Parameters::doCscRecording(void)
{
    bool ret = true;
    int hwPreviewW = 0, hwPreviewH = 0;
    int videoW = 0, videoH = 0;

    getHwPreviewSize(&hwPreviewW, &hwPreviewH);
    getVideoSize(&videoW, &videoH);
    CLOGV2("hwPreviewSize = %d x %d", hwPreviewW, hwPreviewH);
    CLOGV2("VideoSize     = %d x %d", videoW, videoH);

    if (((videoW == 3840 && videoH == 2160) || (videoW == 1920 && videoH == 1080) || (videoW == 2560 && videoH == 1440))
        && m_useAdaptiveCSCRecording == true
        && videoW == hwPreviewW
        && videoH == hwPreviewH) {
        ret = false;
    }

    return ret;
}

int ExynosCamera3Parameters::getHalPixelFormat(void)
{
    int setfile = 0;
    int yuvRange = 0;
    int previewFormat = getHwPreviewFormat();
    int halFormat = 0;

    m_getSetfileYuvRange(false, &setfile, &yuvRange);

    halFormat = convertingHalPreviewFormat(previewFormat, yuvRange);

    return halFormat;
}

#if (TARGET_ANDROID_VER_MAJ >= 4 && TARGET_ANDROID_VER_MIN >= 4)
int ExynosCamera3Parameters::convertingHalPreviewFormat(int previewFormat, int yuvRange)
{
    int halFormat = 0;

    switch (previewFormat) {
    case V4L2_PIX_FMT_NV21:
        CLOGD2("preview format NV21");
        halFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        break;
    case V4L2_PIX_FMT_NV21M:
        CLOGD2("preview format NV21M");
        if (yuvRange == YUV_FULL_RANGE) {
            halFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL;
        } else if (yuvRange == YUV_LIMITED_RANGE) {
            halFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;
        } else {
            CLOGW2("invalid yuvRange, force set to full range");
            halFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL;
        }
        break;
    case V4L2_PIX_FMT_YVU420:
        CLOGD2("DEBUG(%s[%d]): preview format YVU420");
        halFormat = HAL_PIXEL_FORMAT_YV12;
        break;
    case V4L2_PIX_FMT_YVU420M:
        CLOGD2("preview format YVU420M");
        halFormat = HAL_PIXEL_FORMAT_EXYNOS_YV12_M;
        break;
    default:
        CLOGE2("unknown preview format(%d)", previewFormat);
        break;
    }

    return halFormat;
}
#else
int ExynosCamera3Parameters::convertingHalPreviewFormat(int previewFormat, int yuvRange)
{
    int halFormat = 0;

    switch (previewFormat) {
    case V4L2_PIX_FMT_NV21:
        halFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        break;
    case V4L2_PIX_FMT_NV21M:
        if (yuvRange == YUV_FULL_RANGE) {
            halFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_FULL;
        } else if (yuvRange == YUV_LIMITED_RANGE) {
            halFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP;
        } else {
            CLOGW2("invalid yuvRange, force set to full range");
            halFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_FULL;
        }
        break;
    case V4L2_PIX_FMT_YVU420:
        halFormat = HAL_PIXEL_FORMAT_YV12;
        break;
    case V4L2_PIX_FMT_YVU420M:
        halFormat = HAL_PIXEL_FORMAT_EXYNOS_YV12;
        break;
    default:
        CLOGE2("unknown preview format(%d)", previewFormat);
        break;
    }

    return halFormat;
}
#endif

void ExynosCamera3Parameters::setDvfsLock(bool lock) {
    m_dvfsLock = lock;
}

bool ExynosCamera3Parameters::getDvfsLock(void) {
    return m_dvfsLock;
}

#ifdef DEBUG_RAWDUMP
bool ExynosCamera3Parameters::checkBayerDumpEnable(void)
{
#ifndef RAWDUMP_CAPTURE
    char enableRawDump[PROPERTY_VALUE_MAX];
    property_get("ro.debug.rawdump", enableRawDump, "0");

    if (strcmp(enableRawDump, "1") == 0) {
        /*CLOGD("checkBayerDumpEnable : 1");*/
        return true;
    } else {
        /*CLOGD("checkBayerDumpEnable : 0");*/
        return false;
    }
#endif
    return true;
}
#endif  /* DEBUG_RAWDUMP */

bool ExynosCamera3Parameters::setConfig(struct ExynosConfigInfo* config)
{
    memcpy(m_exynosconfig, config, sizeof(struct ExynosConfigInfo));
    setConfigMode(m_exynosconfig->mode);
    return true;
}
struct ExynosConfigInfo* ExynosCamera3Parameters::getConfig()
{
    return m_exynosconfig;
}

bool ExynosCamera3Parameters::setConfigMode(uint32_t mode)
{
    bool ret = false;
    switch(mode){
    case CONFIG_MODE::NORMAL:
    case CONFIG_MODE::HIGHSPEED_60:
    case CONFIG_MODE::HIGHSPEED_120:
    case CONFIG_MODE::HIGHSPEED_240:
        m_exynosconfig->current = &m_exynosconfig->info[mode];
        m_exynosconfig->mode = mode;
        ret = true;
        break;
    default:
        CLOGE2("unknown config mode (%d)", mode);
    }
    return ret;
}

int ExynosCamera3Parameters::getConfigMode()
{
    int ret = -1;
    switch(m_exynosconfig->mode){
    case CONFIG_MODE::NORMAL:
    case CONFIG_MODE::HIGHSPEED_60:
    case CONFIG_MODE::HIGHSPEED_120:
    case CONFIG_MODE::HIGHSPEED_240:
        ret = m_exynosconfig->mode;
        break;
    default:
        CLOGE2("unknown config mode (%d)", m_exynosconfig->mode);
    }

    return ret;
}

void ExynosCamera3Parameters::setZoomActiveOn(bool enable)
{
    m_zoom_activated = enable;
}

bool ExynosCamera3Parameters::getZoomActiveOn(void)
{
    return m_zoom_activated;
}

status_t ExynosCamera3Parameters::setMarkingOfExifFlash(int flag)
{
    m_firing_flash_marking = flag;

    return NO_ERROR;
}

int ExynosCamera3Parameters::getMarkingOfExifFlash(void)
{
    return m_firing_flash_marking;
}

bool ExynosCamera3Parameters::increaseMaxBufferOfPreview(void)
{
    if((getShotMode() == SHOT_MODE_BEAUTY_FACE)||(getShotMode() == SHOT_MODE_FRONT_PANORAMA)
        ) {
        return true;
    } else {
        return false;
    }
}

bool ExynosCamera3Parameters::getSensorOTFSupported(void)
{
    return m_staticInfo->flite3aaOtfSupport;
}

bool ExynosCamera3Parameters::isReprocessing(void)
{
    bool reprocessing = false;
    int cameraId = getCameraId();
    bool flagDual = getDualMode();

    if (cameraId == CAMERA_ID_BACK) {
#if defined(MAIN_CAMERA_DUAL_REPROCESSING) && defined(MAIN_CAMERA_SINGLE_REPROCESSING)
        reprocessing = (flagDual == true) ? MAIN_CAMERA_DUAL_REPROCESSING : MAIN_CAMERA_SINGLE_REPROCESSING;
#else
        CLOGW2("MAIN_CAMERA_DUAL_REPROCESSING/MAIN_CAMERA_SINGLE_REPROCESSING is not defined");
#endif
    } else {
#if defined(FRONT_CAMERA_DUAL_REPROCESSING) && defined(FRONT_CAMERA_SINGLE_REPROCESSING)
        reprocessing = (flagDual == true) ? FRONT_CAMERA_DUAL_REPROCESSING : FRONT_CAMERA_SINGLE_REPROCESSING;
#else
        CLOGW2("FRONT_CAMERA_DUAL_REPROCESSING/FRONT_CAMERA_SINGLE_REPROCESSING is not defined");
#endif
    }

    return reprocessing;
}

bool ExynosCamera3Parameters::isSccCapture(void)
{
    bool sccCapture = false;
    int cameraId = getCameraId();
    bool flagDual = getDualMode();

    if (cameraId == CAMERA_ID_BACK) {
#if defined(MAIN_CAMERA_DUAL_SCC_CAPTURE) && defined(MAIN_CAMERA_SINGLE_SCC_CAPTURE)
        sccCapture = (flagDual == true) ? MAIN_CAMERA_DUAL_SCC_CAPTURE : MAIN_CAMERA_SINGLE_SCC_CAPTURE;
#else
        CLOGW2("MAIN_CAMERA_DUAL_SCC_CAPTURE/MAIN_CAMERA_SINGLE_SCC_CAPTUREis not defined");
#endif
    } else {
#if defined(FRONT_CAMERA_DUAL_SCC_CAPTURE) && defined(FRONT_CAMERA_SINGLE_SCC_CAPTURE)
        sccCapture = (flagDual == true) ? FRONT_CAMERA_DUAL_SCC_CAPTURE : FRONT_CAMERA_SINGLE_SCC_CAPTURE;
#else
        CLOGW2("FRONT_CAMERA_DUAL_SCC_CAPTURE/FRONT_CAMERA_SINGLE_SCC_CAPTURE is not defined");
#endif
    }

    return sccCapture;
}

bool ExynosCamera3Parameters::isFlite3aaOtf(void)
{
    bool flagOtfInput = false;

    int cameraId = getCameraId();
    bool flagDual = getDualMode();
    bool flagSensorOtf = getSensorOTFSupported();

    if (flagSensorOtf == false) {
        return flagOtfInput;
    }

    if (cameraId == CAMERA_ID_BACK) {
        /* for 52xx scenario */
        flagOtfInput = true;

        if (flagDual == true) {
#ifdef MAIN_CAMERA_DUAL_FLITE_3AA_OTF
            flagOtfInput = MAIN_CAMERA_DUAL_FLITE_3AA_OTF;
#else
            CLOGW2("MAIN_CAMERA_DUAL_FLITE_3AA_OTF is not defined");
#endif
        } else {
#ifdef MAIN_CAMERA_SINGLE_FLITE_3AA_OTF
            flagOtfInput = MAIN_CAMERA_SINGLE_FLITE_3AA_OTF;
#else
            CLOGW2("MAIN_CAMERA_SINGLE_FLITE_3AA_OTF is not defined");
#endif
        }
    } else {
        if (flagDual == true) {
#ifdef FRONT_CAMERA_DUAL_FLITE_3AA_OTF
            flagOtfInput = FRONT_CAMERA_DUAL_FLITE_3AA_OTF;
#else
            CLOGW2("FRONT_CAMERA_DUAL_FLITE_3AA_OTF is not defined");
#endif
        } else {
#ifdef FRONT_CAMERA_SINGLE_FLITE_3AA_OTF
            flagOtfInput = FRONT_CAMERA_SINGLE_FLITE_3AA_OTF;
#else
            CLOGW2("FRONT_CAMERA_SINGLE_FLITE_3AA_OTF is not defined");
#endif
        }
    }

    return flagOtfInput;
}

bool ExynosCamera3Parameters::is3aaIspOtf(void)
{
    bool ret = false;

    int cameraId = getCameraId();
    bool flagDual = getDualMode();

    if (cameraId == CAMERA_ID_BACK) {
        if (flagDual == true) {
#ifdef MAIN_CAMERA_DUAL_3AA_ISP_OTF
            ret = MAIN_CAMERA_DUAL_3AA_ISP_OTF;
#else
            CLOGW2("MAIN_CAMERA_DUAL_3AA_ISP_OTF is not defined");
#endif
        } else {
#ifdef MAIN_CAMERA_SINGLE_3AA_ISP_OTF
            ret = MAIN_CAMERA_SINGLE_3AA_ISP_OTF;
#else
            CLOGW2("MAIN_CAMERA_SINGLE_3AA_ISP_OTF is not defined");
#endif
        }
    } else {
        if (flagDual == true) {
#ifdef FRONT_CAMERA_DUAL_3AA_ISP_OTF
            ret = FRONT_CAMERA_DUAL_3AA_ISP_OTF;
#else
            CLOGW2("FRONT_CAMERA_DUAL_3AA_ISP_OTF is not defined");
#endif
        } else {
#ifdef FRONT_CAMERA_SINGLE_3AA_ISP_OTF
            ret = FRONT_CAMERA_SINGLE_3AA_ISP_OTF;
#else
            CLOGW2("FRONT_CAMERA_SINGLE_3AA_ISP_OTF is not defined");
#endif
        }
    }

    return ret;
}

bool ExynosCamera3Parameters::isIspMcscOtf(void)
{
    bool ret = true;

    int cameraId = getCameraId();
    bool flagDual = getDualMode();

    if (cameraId == CAMERA_ID_BACK) {
        if (flagDual == true) {
#ifdef MAIN_CAMERA_DUAL_ISP_MCSC_OTF
            ret = MAIN_CAMERA_DUAL_ISP_MCSC_OTF;
#endif
        } else {
#ifdef MAIN_CAMERA_SINGLE_ISP_MCSC_OTF
            ret = MAIN_CAMERA_SINGLE_ISP_MCSC_OTF;
#endif
        }
    } else {
        if (flagDual == true) {
#ifdef FRONT_CAMERA_DUAL_ISP_MCSC_OTF
            ret = FRONT_CAMERA_DUAL_ISP_MCSC_OTF;
#endif
        } else {
#ifdef FRONT_CAMERA_SINGLE_ISP_MCSC_OTF
            ret = FRONT_CAMERA_SINGLE_ISP_MCSC_OTF;
#endif
        }
    }

    return ret;
}

bool ExynosCamera3Parameters::isMcscVraOtf(void)
{
    bool ret = true;

    int cameraId = getCameraId();
    bool flagDual = getDualMode();

    if (cameraId == CAMERA_ID_BACK) {
        if (flagDual == true) {
#ifdef MAIN_CAMERA_DUAL_MCSC_VRA_OTF
            ret = MAIN_CAMERA_DUAL_MCSC_VRA_OTF;
#endif
        } else {
#ifdef MAIN_CAMERA_SINGLE_MCSC_VRA_OTF
            ret = MAIN_CAMERA_SINGLE_MCSC_VRA_OTF;
#endif
        }
    } else {
        if (flagDual == true) {
#ifdef FRONT_CAMERA_DUAL_MCSC_VRA_OTF
            ret = FRONT_CAMERA_DUAL_MCSC_VRA_OTF;
#endif
        } else {
#ifdef FRONT_CAMERA_SINGLE_MCSC_VRA_OTF
            ret = FRONT_CAMERA_SINGLE_MCSC_VRA_OTF;
#endif
        }
    }

    return ret;
}

bool ExynosCamera3Parameters::isReprocessing3aaIspOTF(void)
{
    bool otf = false;

    int cameraId = getCameraId();
    bool flagDual = getDualMode();

    if (cameraId == CAMERA_ID_BACK) {
        if (flagDual == true) {
#ifdef MAIN_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING
            otf = MAIN_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW2("MAIN_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        } else {
#ifdef MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING
            otf = MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW("MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        }
    } else {
        if (flagDual == true) {
#ifdef FRONT_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING
            otf = FRONT_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW2("FRONT_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        } else {
#ifdef FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING
            otf = FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING;
#else
            CLOGW2("FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING is not defined");
#endif
        }
    }

    if (otf == true) {
        bool flagDirtyBayer = false;

        int reprocessingBayerMode = this->getReprocessingBayerMode();
        switch(reprocessingBayerMode) {
        case REPROCESSING_BAYER_MODE_NONE:
        case REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON:
        case REPROCESSING_BAYER_MODE_PURE_DYNAMIC:
            flagDirtyBayer = false;
            break;
        case REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON:
        case REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC:
        default:
            flagDirtyBayer = true;
            break;
        }

        if (flagDirtyBayer == true) {
            CLOGW2("otf == true. but, flagDirtyBayer == true. so force false on 3aa_isp otf");

            otf = false;
        }
    }

    return otf;
}

bool ExynosCamera3Parameters::isReprocessingIspMcscOTF(void)
{
    bool otf = false;

    int cameraId = getCameraId();
    bool flagDual = getDualMode();

    if (isUseYuvReprocessing() == false) {
        if (cameraId == CAMERA_ID_BACK) {
            if (flagDual == true) {
#ifdef MAIN_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING
                otf = MAIN_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING;
#else
                ALOGW("WRN(%s[%d]): MAIN_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING is not defined", __FUNCTION__, __LINE__);
#endif
            } else {
#ifdef MAIN_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING
                otf = MAIN_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING;
#else
                ALOGW("WRN(%s[%d]): MAIN_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING is not defined", __FUNCTION__, __LINE__);
#endif
            }
        } else {
            if (flagDual == true) {
#ifdef FRONT_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING
                otf = FRONT_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING;
#else
                ALOGW("WRN(%s[%d]): FRONT_CAMERA_DUAL_ISP_MCSC_OTF_REPROCESSING is not defined", __FUNCTION__, __LINE__);
#endif
            } else {
#ifdef FRONT_CAMERA_SINGLE_3AA_OTF_MCSC_REPROCESSING
                otf = FRONT_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING;
#else
                ALOGW("WRN(%s[%d]): FRONT_CAMERA_SINGLE_ISP_MCSC_OTF_REPROCESSING is not defined", __FUNCTION__, __LINE__);
#endif
            }
        }
    }

    return otf;
}

bool ExynosCamera3Parameters::isHWFCEnabled(void)
{
#if defined(USE_JPEG_HWFC)
    return USE_JPEG_HWFC;
#else
    return false;
#endif
}

bool ExynosCamera3Parameters::isHWFCOnDemand(void)
{
#if defined(USE_JPEG_HWFC_ONDEMAND)
    return USE_JPEG_HWFC_ONDEMAND;
#else
    return false;
#endif
}

bool ExynosCamera3Parameters::isUseThumbnailHWFC(void)
{
#if defined(USE_THUMBNAIL_HWFC)
    return USE_JPEG_HWFC_ONDEMAND;
#else
    return false;
#endif
}

bool ExynosCamera3Parameters::getSupportedZoomPreviewWIthScaler(void)
{
	bool ret = false;
	int cameraId = getCameraId();
	bool flagDual = getDualMode();
	int fastFpsMode = getFastFpsMode();
    int vrMode = getVRMode();

	if (cameraId == CAMERA_ID_BACK) {
		if (fastFpsMode > CONFIG_MODE::HIGHSPEED_60 &&
            fastFpsMode < CONFIG_MODE::MAX &&
            vrMode != 1) {
			ret = true;
		}
	} else {
		if (flagDual == true) {
			ret = true;
		}
	}

	return ret;
}

void ExynosCamera3Parameters::setZoomPreviewWIthScaler(bool enable)
{
	m_zoomWithScaler = enable;
}

bool ExynosCamera3Parameters::getZoomPreviewWIthScaler(void)
{
	return m_zoomWithScaler;
}

bool ExynosCamera3Parameters::isOwnScc(int cameraId)
{
    bool ret = false;

    if (cameraId == CAMERA_ID_BACK) {
#ifdef MAIN_CAMERA_HAS_OWN_SCC
        ret = MAIN_CAMERA_HAS_OWN_SCC;
#else
        CLOGW2("MAIN_CAMERA_HAS_OWN_SCC is not defined");
#endif
    } else {
#ifdef FRONT_CAMERA_HAS_OWN_SCC
        ret = FRONT_CAMERA_HAS_OWN_SCC;
#else
        CLOGW2("FRONT_CAMERA_HAS_OWN_SCC is not defined");
#endif
    }

    return ret;
}

bool ExynosCamera3Parameters::isOwnMCSC(void)
{
    bool ret = false;

#ifdef OWN_MCSC_HW
    ret = OWN_MCSC_HW;
#endif

    return ret;
}

bool ExynosCamera3Parameters::isCompanion(int cameraId)
{
    bool ret = false;

    if (cameraId == CAMERA_ID_BACK) {
        CLOGI2("MAIN_CAMERA_USE_SAMSUNG_COMPANION is not defined");
    } else {
        CLOGI2("FRONT_CAMERA_USE_SAMSUNG_COMPANION is not defined");
    }

    return ret;
}

int ExynosCamera3Parameters::getHalVersion(void)
{
    return m_halVersion;
}

void ExynosCamera3Parameters::setHalVersion(int halVersion)
{
    m_halVersion = halVersion;
    m_activityControl->setHalVersion(m_halVersion);

    CLOGI2("m_halVersion(%d)", m_halVersion);

    return;
}

struct ExynosSensorInfoBase *ExynosCamera3Parameters::getSensorStaticInfo()
{
    return m_staticInfo;
}

bool ExynosCamera3Parameters::getSetFileCtlMode(void)
{
#ifdef SET_SETFILE_BY_SET_CTRL
    return true;
#else
    return false;
#endif
}

bool ExynosCamera3Parameters::getSetFileCtl3AA_ISP(void)
{
#ifdef SET_SETFILE_BY_SET_CTRL_3AA_ISP
    return SET_SETFILE_BY_SET_CTRL_3AA_ISP;
#else
    return false;
#endif
}

bool ExynosCamera3Parameters::getSetFileCtl3AA(void)
{
#ifdef SET_SETFILE_BY_SET_CTRL_3AA
    return SET_SETFILE_BY_SET_CTRL_3AA;
#else
    return false;
#endif
}

bool ExynosCamera3Parameters::getSetFileCtlISP(void)
{
#ifdef SET_SETFILE_BY_SET_CTRL_ISP
    return SET_SETFILE_BY_SET_CTRL_ISP;
#else
    return false;
#endif
}

bool ExynosCamera3Parameters::getSetFileCtlSCP(void)
{
#ifdef SET_SETFILE_BY_SET_CTRL_SCP
    return SET_SETFILE_BY_SET_CTRL_SCP;
#else
    return false;
#endif
}

bool ExynosCamera3Parameters::isUsing3acForIspc(void)
{
#if (defined(USE_3AC_FOR_ISPC) && (USE_3AC_FOR_ISPC))
    return true;
#else
    return false;
#endif
}

void ExynosCamera3Parameters::m_getV4l2Name(char* colorName, size_t length, int colorFormat)
{
    size_t index = 0;
    if (colorName == NULL) {
        CLOGE("ERR(%s[%d]):colorName is NULL", __FUNCTION__, __LINE__);
        return;
    }

    for (index = 0; index < length-1; index++) {
        colorName[index] = colorFormat & 0xff;
        colorFormat = colorFormat >> 8;
    }
    colorName[index] = '\0';
}

int32_t ExynosCamera3Parameters::getYuvStreamMaxNum(void)
{
    int32_t yuvStreamMaxNum = -1;

    if (m_staticInfo == NULL) {
        CLOGE("ERR(%s[%d]):m_staticInfo is NULL",
                __FUNCTION__, __LINE__);

        return INVALID_OPERATION;
    }

    yuvStreamMaxNum = m_staticInfo->maxNumOutputStreams[PROCESSED];
    if (yuvStreamMaxNum < 0) {
        CLOGE("ERR(%s[%d]):Invalid MaxNumOutputStreamsProcessed %d",
                __FUNCTION__, __LINE__, yuvStreamMaxNum);
        return BAD_VALUE;
    }

    return yuvStreamMaxNum;
}

status_t ExynosCamera3Parameters::setYuvBufferCount(const int count, const int index)
{
    if (count < 0 || count > VIDEO_MAX_FRAME
            || index < 0 || index > m_staticInfo->maxNumOutputStreams[PROCESSED]) {
        CLOGE("ERR(%s[%d]):Invalid argument. count %d index %d",
                __FUNCTION__, __LINE__, count, index);

        return BAD_VALUE;
    }

    m_yuvBufferCount[index] = count;

    return NO_ERROR;
}

int ExynosCamera3Parameters::getYuvBufferCount(const int index)
{
    if (index < 0 || index > m_staticInfo->maxNumOutputStreams[PROCESSED]) {
        CLOGE("ERR(%s[%d]):Invalid index %d",
                __FUNCTION__, __LINE__, index);
        return 0;
    }

    return m_yuvBufferCount[index];
}

}; /* namespace android */
