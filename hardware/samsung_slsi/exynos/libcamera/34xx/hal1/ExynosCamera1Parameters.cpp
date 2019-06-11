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

ExynosCamera1Parameters::ExynosCamera1Parameters(int cameraId, __unused bool flagCompanion, int halVersion)
{
    m_cameraId = cameraId;
    m_halVersion = halVersion;

    const char *myName = (m_cameraId == CAMERA_ID_BACK) ? "ParametersBack" : "ParametersFront";
    strncpy(m_name, myName,  EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    m_staticInfo = createExynosCamera1SensorInfo(cameraId);
    m_useSizeTable = (m_staticInfo->sizeTableSupport) ? USE_CAMERA_SIZE_TABLE : false;
    m_useAdaptiveCSCRecording = (cameraId == CAMERA_ID_BACK) ? USE_ADAPTIVE_CSC_RECORDING : false;

    m_exynosconfig = NULL;
    m_activityControl = new ExynosCameraActivityControl(m_cameraId);

    memset(&m_cameraInfo, 0, sizeof(struct exynos_camera_info));
    memset(&m_exifInfo, 0, sizeof(m_exifInfo));

    m_initMetadata();

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
#ifdef USE_PREVIEW_CROP_FOR_ROATAION
    m_rotationProperty = checkRotationProperty();
#endif
    m_zoom_activated = false;
    m_exposureTimeCapture = 0;
    m_isFactoryMode = false;

    // Initial Values : END
    setDefaultCameraInfo();
    setDefaultParameter();
}

ExynosCamera1Parameters::~ExynosCamera1Parameters()
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

int ExynosCamera1Parameters::getCameraId(void)
{
    return m_cameraId;
}

CameraParameters ExynosCamera1Parameters::getParameters() const
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    return m_params;
}

void ExynosCamera1Parameters::setDefaultCameraInfo(void)
{
    CLOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);
    m_setHwSensorSize(m_staticInfo->maxSensorW, m_staticInfo->maxSensorH);
    m_setHwPreviewSize(m_staticInfo->maxPreviewW, m_staticInfo->maxPreviewH);
    m_setHwPictureSize(m_staticInfo->maxPictureW, m_staticInfo->maxPictureH);

    /* Initalize BNS scale ratio, step:500, ex)1500->x1.5 scale down */
    m_setBnsScaleRatio(1000);
    /* Initalize Binning scale ratio */
    m_setBinningScaleRatio(1000);
    /* Set Default VideoSize to FHD */
    m_setVideoSize(1920,1080);
}

status_t ExynosCamera1Parameters::checkRecordingHint(const CameraParameters& params)
{
    /* recording hint */
    bool recordingHint = false;
    const char *newRecordingHint = params.get(CameraParameters::KEY_RECORDING_HINT);

    if (newRecordingHint != NULL) {
        CLOGD("DEBUG(%s):newRecordingHint : %s", "setParameters", newRecordingHint);

        recordingHint = (strcmp(newRecordingHint, "true") == 0) ? true : false;

        m_setRecordingHint(recordingHint);

        m_params.set(CameraParameters::KEY_RECORDING_HINT, newRecordingHint);

    } else {
        /* to confirm that recordingHint value is checked up (whatever value is) */
        m_setRecordingHint(m_cameraInfo.recordingHint);

        recordingHint = getRecordingHint();
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setRecordingHint(bool hint)
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

bool ExynosCamera1Parameters::getRecordingHint(void)
{
    /*
     * Before setParameters, we cannot know recordingHint is valid or not
     * So, check and make assert for fast debugging
     */
    if (m_flagCheckRecordingHint == false)
        android_printAssert(NULL, LOG_TAG, "Cannot call getRecordingHint befor setRecordingHint, assert!!!!");

    return m_cameraInfo.recordingHint;
}


status_t ExynosCamera1Parameters::checkDualMode(const CameraParameters& params)
{
    /* dual_mode */
    bool flagDualMode = false;
    int newDualMode = params.getInt("dual_mode");

    if (newDualMode == 1) {
        CLOGD("DEBUG(%s):newDualMode : %d", "setParameters", newDualMode);
        flagDualMode = true;
    }

    m_setDualMode(flagDualMode);
    m_params.set("dual_mode", newDualMode);

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setDualMode(bool dual)
{
    m_cameraInfo.dualMode = dual;
    /* dualMode is confirmed */
    m_flagCheckDualMode = true;
}

bool ExynosCamera1Parameters::getDualMode(void)
{
    /*
    * Before setParameters, we cannot know dualMode is valid or not
    * So, check and make assert for fast debugging
    */
    if (m_flagCheckDualMode == false)
        android_printAssert(NULL, LOG_TAG, "Cannot call getDualMode befor checkDualMode, assert!!!!");

    return m_cameraInfo.dualMode;
}

status_t ExynosCamera1Parameters::checkDualRecordingHint(const CameraParameters& params)
{
    /* dual recording hint */
    bool flagDualRecordingHint = false;
    int newDualRecordingHint = params.getInt("dualrecording-hint");

    if (newDualRecordingHint == 1) {
        CLOGD("DEBUG(%s):newDualRecordingHint : %d", "setParameters", newDualRecordingHint);
        flagDualRecordingHint = true;
    }

    m_setDualRecordingHint(flagDualRecordingHint);
    m_params.set("dualrecording-hint", newDualRecordingHint);

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setDualRecordingHint(bool hint)
{
    m_cameraInfo.dualRecordingHint = hint;

    if (hint) {
        setMetaVideoMode(&m_metadata, AA_VIDEOMODE_ON);
    } else if (!hint && !getRecordingHint() && !getEffectRecordingHint()) {
        setMetaVideoMode(&m_metadata, AA_VIDEOMODE_OFF);
    }
}

bool ExynosCamera1Parameters::getDualRecordingHint(void)
{
    return m_cameraInfo.dualRecordingHint;
}

status_t ExynosCamera1Parameters::checkEffectHint(const CameraParameters& params)
{
    /* effect hint */
    bool flagEffectHint = false;
    int newEffectHint = params.getInt("effect_hint");

    if (newEffectHint < 0)
        return NO_ERROR;

    if (newEffectHint == 1) {
        CLOGD("DEBUG(%s[%d]):newEffectHint : %d", "setParameters", __LINE__, newEffectHint);
        flagEffectHint = true;
    }

    m_setEffectHint(newEffectHint);
    m_params.set("effect_hint", newEffectHint);

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setEffectHint(bool hint)
{
    m_cameraInfo.effectHint = hint;
}

bool ExynosCamera1Parameters::getEffectHint(void)
{
    return m_cameraInfo.effectHint;
}

bool ExynosCamera1Parameters::getEffectRecordingHint(void)
{
    return m_cameraInfo.effectRecordingHint;
}

status_t ExynosCamera1Parameters::checkPreviewFps(const CameraParameters& params)
{
    int ret = 0;

    ret = checkPreviewFpsRange(params);
    if (ret == BAD_VALUE) {
        CLOGE("ERR(%s): Inavalid value", "setParameters");
        return ret;
    } else if (ret != NO_ERROR) {
        ret = checkPreviewFrameRate(params);
    }

    return ret;
}

status_t ExynosCamera1Parameters::checkPreviewFpsRange(const CameraParameters& params)
{
    int newMinFps = 0;
    int newMaxFps = 0;
    int newFrameRate = params.getPreviewFrameRate();
    uint32_t curMinFps = 0;
    uint32_t curMaxFps = 0;

    params.getPreviewFpsRange(&newMinFps, &newMaxFps);
    if (newMinFps <= 0 || newMaxFps <= 0 || newMinFps > newMaxFps) {
        CLOGE("PreviewFpsRange is invalid, newMin(%d), newMax(%d)", newMinFps, newMaxFps);
        return BAD_VALUE;
    }

    ALOGI("INFO(%s):Original FpsRange[Min=%d, Max=%d]", __FUNCTION__, newMinFps, newMaxFps);

    if (m_adjustPreviewFpsRange(newMinFps, newMaxFps) != NO_ERROR) {
        CLOGE("Fail to adjust preview fps range");
        return INVALID_OPERATION;
    }

    newMinFps = newMinFps / 1000;
    newMaxFps = newMaxFps / 1000;
    if (FRAME_RATE_MAX < newMaxFps || newMaxFps < newMinFps) {
        CLOGE("PreviewFpsRange is out of bound");
        return INVALID_OPERATION;
    }

    getPreviewFpsRange(&curMinFps, &curMaxFps);
    CLOGI("INFO(%s):curFpsRange[Min=%d, Max=%d], newFpsRange[Min=%d, Max=%d], [curFrameRate=%d]",
        "checkPreviewFpsRange", curMinFps, curMaxFps, newMinFps, newMaxFps, m_params.getPreviewFrameRate());

    if (curMinFps != (uint32_t)newMinFps || curMaxFps != (uint32_t)newMaxFps) {
        m_setPreviewFpsRange((uint32_t)newMinFps, (uint32_t)newMaxFps);

        char newFpsRange[256];
        memset (newFpsRange, 0, 256);
        snprintf(newFpsRange, 256, "%d,%d", newMinFps * 1000, newMaxFps * 1000);

        CLOGI("DEBUG(%s):set PreviewFpsRange(%s)", __FUNCTION__, newFpsRange);
        CLOGI("DEBUG(%s):set PreviewFrameRate(curFps=%d->newFps=%d)", __FUNCTION__, m_params.getPreviewFrameRate(), newMaxFps);
        m_params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, newFpsRange);
        m_params.setPreviewFrameRate(newMaxFps);
    }

    getPreviewFpsRange(&curMinFps, &curMaxFps);
    m_activityControl->setFpsValue(curMinFps);

    /* For backword competivity */
    m_params.setPreviewFrameRate(newFrameRate);

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::m_adjustPreviewFpsRange(int &newMinFps, int &newMaxFps)
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
        ALOGD("DEBUG(%s[%d]):dualMode(true), newMaxFps=%d", __FUNCTION__, __LINE__, newMaxFps);
    }

    if (getDualRecordingHint() == true) {
        flagSpecialMode = true;

        /* when dual recording mode, fps is limited by 24fps */
        if (24000 < newMaxFps)
            newMaxFps = 24000;

        /* set fixed fps. */
        newMinFps = newMaxFps;
        ALOGD("DEBUG(%s[%d]):dualRecordingHint(true), newMaxFps=%d", __FUNCTION__, __LINE__, newMaxFps);
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
        ALOGD("DEBUG(%s[%d]):effectHint(true), newMaxFps=%d", __FUNCTION__, __LINE__, newMaxFps);
    }

    if (getRecordingHint() == true) {
        flagSpecialMode = true;
#if 0   /* Don't use to set fixed fps in the hal side. */
#ifdef USE_VARIABLE_FPS_OF_FRONT_RECORDING
        if (getCameraId() == CAMERA_ID_FRONT && getSamsungCamera() == true) {
            /* Supported the variable frame rate for Image Quality Performacne */
            ALOGD("DEBUG(%s[%d]):RecordingHint(true),newMinFps=%d,newMaxFps=%d", __FUNCTION__, __LINE__, newMinFps, newMaxFps);
        } else
#endif
        {
            /* set fixed fps. */
            newMinFps = newMaxFps;
        }
        ALOGD("DEBUG(%s[%d]):RecordingHint(true), newMaxFps=%d", __FUNCTION__, __LINE__, newMaxFps);
#endif
        ALOGD("DEBUG(%s[%d]):RecordingHint(true),newMinFps=%d,newMaxFps=%d", __FUNCTION__, __LINE__, newMinFps, newMaxFps);
    }

    if (flagSpecialMode == true) {
        CLOGD("DEBUG(%s[%d]):special mode enabled, newMaxFps=%d", __FUNCTION__, __LINE__, newMaxFps);
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
        ALOGI("INFO(%s): limit the maximum 30 fps range in THIRD_PARTY_BLACKBOX_MODE(%d,%d)", __FUNCTION__, newMinFps, newMaxFps);
        if (newMinFps > 30000) {
            newMinFps = 30000;
        }
        if (newMaxFps > 30000) {
            newMaxFps = 30000;
        }
        break;
    case THIRD_PARTY_VTCALL_MODE:
        ALOGI("INFO(%s): limit the maximum 15 fps range in THIRD_PARTY_VTCALL_MODE(%d,%d)", __FUNCTION__, newMinFps, newMaxFps);
        if (newMinFps > 15000) {
            newMinFps = 15000;
        }
        if (newMaxFps > 15000) {
            newMaxFps = 15000;
        }
        break;
    case THIRD_PARTY_HANGOUT_MODE:
        ALOGI("INFO(%s): change fps range 15000,15000 in THIRD_PARTY_HANGOUT_MODE", __FUNCTION__);
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

void ExynosCamera1Parameters::updatePreviewFpsRange(void)
{
    uint32_t curMinFps = 0;
    uint32_t curMaxFps = 0;
    int newMinFps = 0;
    int newMaxFps = 0;

    getPreviewFpsRange(&curMinFps, &curMaxFps);
    newMinFps = curMinFps * 1000;
    newMaxFps = curMaxFps * 1000;

    if (m_adjustPreviewFpsRange(newMinFps, newMaxFps) != NO_ERROR) {
        CLOGE("Fils to adjust preview fps range");
        return;
    }

    newMinFps = newMinFps / 1000;
    newMaxFps = newMaxFps / 1000;

    if (curMinFps != (uint32_t)newMinFps || curMaxFps != (uint32_t)newMaxFps) {
        m_setPreviewFpsRange((uint32_t)newMinFps, (uint32_t)newMaxFps);
    }
}

status_t ExynosCamera1Parameters::checkPreviewFrameRate(const CameraParameters& params)
{
    int newFrameRate = params.getPreviewFrameRate();
    int curFrameRate = m_params.getPreviewFrameRate();
    int newMinFps = 0;
    int newMaxFps = 0;
    int tempFps = 0;

    if (newFrameRate < 0) {
        return BAD_VALUE;
    }
    CLOGD("DEBUG(%s):curFrameRate=%d, newFrameRate=%d", __FUNCTION__, curFrameRate, newFrameRate);
    if (newFrameRate != curFrameRate) {
        tempFps = newFrameRate * 1000;

        if (m_getSupportedVariableFpsList(tempFps / 2, tempFps, &newMinFps, &newMaxFps) == false) {
            newMinFps = tempFps / 2;
            newMaxFps = tempFps;
        }

        char newFpsRange[256];
        memset (newFpsRange, 0, 256);
        snprintf(newFpsRange, 256, "%d,%d", newMinFps, newMaxFps);
        m_params.set(CameraParameters::KEY_PREVIEW_FPS_RANGE, newFpsRange);

        if (checkPreviewFpsRange(m_params) == true) {
            m_params.setPreviewFrameRate(newFrameRate);
            CLOGD("DEBUG(%s):setPreviewFrameRate(newFrameRate=%d)", __FUNCTION__, newFrameRate);
        }
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setPreviewFpsRange(uint32_t min, uint32_t max)
{
    setMetaCtlAeTargetFpsRange(&m_metadata, min, max);
    setMetaCtlSensorFrameDuration(&m_metadata, (uint64_t)((1000 * 1000 * 1000) / (uint64_t)max));

    ALOGI("INFO(%s):fps min(%d) max(%d)", __FUNCTION__, min, max);
}

void ExynosCamera1Parameters::getPreviewFpsRange(uint32_t *min, uint32_t *max)
{
    /* ex) min = 15 , max = 30 */
    getMetaCtlAeTargetFpsRange(&m_metadata, min, max);
}

bool ExynosCamera1Parameters::m_getSupportedVariableFpsList(int min, int max, int *newMin, int *newMax)
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

                CLOGW("WARN(%s):calibrate new fps(%d/%d -> %d/%d)", __FUNCTION__, min, max, *newMin, *newMax);

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

                CLOGW("WARN(%s):calibrate new fps(%d/%d -> %d/%d)", __FUNCTION__, min, max, *newMin, *newMax);

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

                CLOGW("WARN(%s):calibrate new fps(%d/%d -> %d/%d)", __FUNCTION__, min, max, *newMin, *newMax);

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

                CLOGW("WARN(%s):calibrate new fps(%d/%d -> %d/%d)", __FUNCTION__, min, max, *newMin, *newMax);

                return true;
            }
        }
    }

    return false;
}

status_t ExynosCamera1Parameters::checkVideoSize(const CameraParameters& params)
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

bool ExynosCamera1Parameters::m_isSupportedVideoSize(const int width,
                                                    const int height)
{
    int maxWidth = 0;
    int maxHeight = 0;
    int (*sizeList)[SIZE_OF_RESOLUTION];

    getMaxVideoSize(&maxWidth, &maxHeight);

    if (maxWidth < width || maxHeight < height) {
        CLOGE("ERR(%s):invalid video Size(maxSize(%d/%d) size(%d/%d)",
                __FUNCTION__, maxWidth, maxHeight, width, height);
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

    CLOGE("ERR(%s):Invalid video size(%dx%d)", __FUNCTION__, width, height);

    return false;
}

bool ExynosCamera1Parameters::m_isUHDRecordingMode(void)
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

void ExynosCamera1Parameters::m_setVideoSize(int w, int h)
{
    m_cameraInfo.videoW = w;
    m_cameraInfo.videoH = h;
}

bool ExynosCamera1Parameters::getUHDRecordingMode(void)
{
    return m_isUHDRecordingMode();
}

bool ExynosCamera1Parameters::getFaceDetectionMode(bool flagCheckingRecording)
{
    bool ret = true;

    /* turn off when dual mode back camera */
    if (getDualMode() == true &&
        getCameraId() == CAMERA_ID_BACK) {
        ret = false;
    }

    /* turn off when vt mode */
    if (getVtMode() != 0)
        ret = false;

    /* when stopRecording, ignore recording hint */
    if (flagCheckingRecording == true) {
        /* when recording mode*/
        if (getRecordingHint() == true) {
            ret = false;
        }
    }

    return ret;
}

void ExynosCamera1Parameters::getVideoSize(int *w, int *h)
{
    *w = m_cameraInfo.videoW;
    *h = m_cameraInfo.videoH;
}

void ExynosCamera1Parameters::getMaxVideoSize(int *w, int *h)
{
    *w = m_staticInfo->maxVideoW;
    *h = m_staticInfo->maxVideoH;
}

int ExynosCamera1Parameters::getVideoFormat(void)
{
    if (getAdaptiveCSCRecording() == true) {
        return V4L2_PIX_FMT_NV21M;
    } else {
        return V4L2_PIX_FMT_NV12M;
    }
}

bool ExynosCamera1Parameters::getReallocBuffer() {
    Mutex::Autolock lock(m_reallocLock);
    return m_reallocBuffer;
}

bool ExynosCamera1Parameters::setReallocBuffer(bool enable) {
    Mutex::Autolock lock(m_reallocLock);
    m_reallocBuffer = enable;
    return m_reallocBuffer;
}

status_t ExynosCamera1Parameters::checkFastFpsMode(const CameraParameters& params)
{
#ifdef TEST_GED_HIGH_SPEED_RECORDING
    int fastFpsMode  = getFastFpsMode();
#else
    int fastFpsMode  = params.getInt("fast-fps-mode");
#endif
    int tempShotMode = params.getInt("shot-mode");
    int prevFpsMode = getFastFpsMode();

    uint32_t curMinFps = 0;
    uint32_t curMaxFps = 0;
    uint32_t newMinFps = curMinFps;
    uint32_t newMaxFps = curMaxFps;

    bool recordingHint = getRecordingHint();
    bool isShotModeAnimated = false;
    bool flagHighSpeed = false;
    int newVideoW = 0;
    int newVideoH = 0;

    params.getVideoSize(&newVideoW, &newVideoH);
    getPreviewFpsRange(&curMinFps, &curMaxFps);

    // Workaround : Should be removed later once application fixes this.
    if( (curMinFps == 60 && curMaxFps == 60) && (newVideoW == 1920 && newVideoH == 1080) ) {
        fastFpsMode = 1;
    }

    CLOGD("DEBUG(%s):fast-fps-mode : %d", "setParameters", fastFpsMode);

#if (!USE_HIGHSPEED_RECORDING)
    fastFpsMode = -1;
    CLOGD("DEBUG(%s):fast-fps-mode not supported. set to (%d).", "setParameters", fastFpsMode);
#endif

    CLOGI("INFO(%s):curFpsRange[Min=%d, Max=%d], [curFrameRate=%d]",
        "checkPreviewFpsRange", curMinFps, curMaxFps, m_params.getPreviewFrameRate());


    if (fastFpsMode <= 0 || fastFpsMode > 3) {
        m_setHighSpeedRecording(false);
        setConfigMode(CONFIG_MODE::NORMAL);
        if( fastFpsMode != prevFpsMode) {
            setFastFpsMode(fastFpsMode);
            m_params.set("fast-fps-mode", fastFpsMode);
            setReallocBuffer(true);
            m_setRestartPreviewChecked(true);
        }
        return NO_ERROR;
    } else {
        if( fastFpsMode == prevFpsMode ) {
            CLOGE("INFO(%s):mode is not changed fastFpsMode(%d) prevFpsMode(%d)", "checkFastFpsMode", fastFpsMode, prevFpsMode);
            return NO_ERROR;
        }
    }

    if (tempShotMode == SHOT_MODE_ANIMATED_SCENE) {
        if (curMinFps == 15 && curMaxFps == 15)
            isShotModeAnimated = true;
    }

    if ((recordingHint == true) && !(isShotModeAnimated)) {

        CLOGD("DEBUG(%s):Set High Speed Recording", "setParameters");

        switch(fastFpsMode) {
            case 1:
                newMinFps = 60;
                newMaxFps = 60;
                setConfigMode(CONFIG_MODE::HIGHSPEED_60);
                break;
            case 2:
                newMinFps = 120;
                newMaxFps = 120;
                setConfigMode(CONFIG_MODE::HIGHSPEED_120);
                break;
            case 3:
                newMinFps = 240;
                newMaxFps = 240;
                setConfigMode(CONFIG_MODE::HIGHSPEED_240);
                break;
        }
        setFastFpsMode(fastFpsMode);
        m_params.set("fast-fps-mode", fastFpsMode);

        CLOGI("INFO(%s):fastFpsMode(%d) prevFpsMode(%d)", "checkFastFpsMode", fastFpsMode, prevFpsMode);
        setReallocBuffer(true);
        m_setRestartPreviewChecked(true);

        flagHighSpeed = m_adjustHighSpeedRecording(curMinFps, curMaxFps, newMinFps, newMaxFps);
        m_setHighSpeedRecording(flagHighSpeed);
        m_setPreviewFpsRange(newMinFps, newMaxFps);

        CLOGI("INFO(%s):m_setPreviewFpsRange(newFpsRange[Min=%d, Max=%d])", "checkFastFpsMode", newMinFps, newMaxFps);
#ifdef TEST_GED_HIGH_SPEED_RECORDING
        m_params.setPreviewFrameRate(newMaxFps);
        CLOGD("DEBUG(%s):setPreviewFrameRate (newMaxFps=%d)", "checkFastFpsMode", newMaxFps);
#endif
        updateHwSensorSize();
    }

    return NO_ERROR;
};

void ExynosCamera1Parameters::setFastFpsMode(int fpsMode)
{
    m_fastFpsMode = fpsMode;
}

int ExynosCamera1Parameters::getFastFpsMode(void)
{
    return m_fastFpsMode;
}

void ExynosCamera1Parameters::m_setHighSpeedRecording(bool highSpeed)
{
    m_cameraInfo.highSpeedRecording = highSpeed;
}

bool ExynosCamera1Parameters::getHighSpeedRecording(void)
{
    return m_cameraInfo.highSpeedRecording;
}

bool ExynosCamera1Parameters::m_adjustHighSpeedRecording(int curMinFps, int curMaxFps, __unused int newMinFps, int newMaxFps)
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
        CLOGD("DEBUG(%s[%d]):setRestartPreviewChecked true", __FUNCTION__, __LINE__);
        m_setRestartPreviewChecked(true);
    }

    return flagHighSpeedRecording;
}

void ExynosCamera1Parameters::m_setRestartPreviewChecked(bool restart)
{
    CLOGD("DEBUG(%s):setRestartPreviewChecked(during SetParameters) %s", __FUNCTION__, restart ? "true" : "false");
    Mutex::Autolock lock(m_parameterLock);

    m_flagRestartPreviewChecked = restart;
}

bool ExynosCamera1Parameters::m_getRestartPreviewChecked(void)
{
    Mutex::Autolock lock(m_parameterLock);

    return m_flagRestartPreviewChecked;
}

bool ExynosCamera1Parameters::getPreviewSizeChanged(void)
{
    return m_previewSizeChanged;
}

void ExynosCamera1Parameters::m_setRestartPreview(bool restart)
{
    CLOGD("DEBUG(%s):setRestartPreview %s", __FUNCTION__, restart ? "true" : "false");
    Mutex::Autolock lock(m_parameterLock);

    m_flagRestartPreview = restart;

}

void ExynosCamera1Parameters::setPreviewRunning(bool enable)
{
    Mutex::Autolock lock(m_parameterLock);

    m_previewRunning = enable;
    m_flagRestartPreviewChecked = false;
    m_flagRestartPreview = false;
    m_previewSizeChanged = false;
}

void ExynosCamera1Parameters::setPictureRunning(bool enable)
{
    Mutex::Autolock lock(m_parameterLock);

    m_pictureRunning = enable;
}

void ExynosCamera1Parameters::setRecordingRunning(bool enable)
{
    Mutex::Autolock lock(m_parameterLock);

    m_recordingRunning = enable;
}

bool ExynosCamera1Parameters::getPreviewRunning(void)
{
    Mutex::Autolock lock(m_parameterLock);

    return m_previewRunning;
}

bool ExynosCamera1Parameters::getPictureRunning(void)
{
    Mutex::Autolock lock(m_parameterLock);

    return m_pictureRunning;
}

bool ExynosCamera1Parameters::getRecordingRunning(void)
{
    Mutex::Autolock lock(m_parameterLock);

    return m_recordingRunning;
}

bool ExynosCamera1Parameters::getRestartPreview(void)
{
    Mutex::Autolock lock(m_parameterLock);

    return m_flagRestartPreview;
}

status_t ExynosCamera1Parameters::checkVideoStabilization(const CameraParameters& params)
{
    /* video stablization */
    const char *newVideoStabilization = params.get(CameraParameters::KEY_VIDEO_STABILIZATION);
    bool currVideoStabilization = m_flagVideoStabilization;
    bool isVideoStabilization = false;

    if (newVideoStabilization != NULL) {
        CLOGD("DEBUG(%s):newVideoStabilization %s", "setParameters", newVideoStabilization);

        if (!strcmp(newVideoStabilization, "true"))
            isVideoStabilization = true;

        if (currVideoStabilization != isVideoStabilization) {
            m_flagVideoStabilization = isVideoStabilization;
            m_setVideoStabilization(m_flagVideoStabilization);
            m_params.set(CameraParameters::KEY_VIDEO_STABILIZATION, newVideoStabilization);
        }
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setVideoStabilization(bool stabilization)
{
    m_cameraInfo.videoStabilization = stabilization;
}

bool ExynosCamera1Parameters::getVideoStabilization(void)
{
    return m_cameraInfo.videoStabilization;
}

bool ExynosCamera1Parameters::updateTpuParameters(void)
{
    status_t ret = NO_ERROR;

    /* 1. update data video stabilization state to actual*/
    CLOGD("%s(%d) video stabilization old(%d) new(%d)", __FUNCTION__, __LINE__, m_cameraInfo.videoStabilization, m_flagVideoStabilization);
    m_setVideoStabilization(m_flagVideoStabilization);

    bool hwVdisMode  = this->getHWVdisMode();

    if (setDisEnable(hwVdisMode) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):setDisEnable(%d) fail", __FUNCTION__, __LINE__, hwVdisMode);
    }

    /* 2. update data 3DNR state to actual*/
    CLOGD("%s(%d) 3DNR old(%d) new(%d)", __FUNCTION__, __LINE__, m_cameraInfo.is3dnrMode, m_flag3dnrMode);
    m_set3dnrMode(m_flag3dnrMode);
    if (setDnrEnable(m_flag3dnrMode) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):setDnrEnable(%d) fail", __FUNCTION__, __LINE__, m_flag3dnrMode);
    }

    return true;
}

status_t ExynosCamera1Parameters::checkPreviewSize(const CameraParameters& params)
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
        ALOGE("ERR(%s): adjustPreviewSize fail, newPreviewSize(%dx%d)", "Parameters", newPreviewW, newPreviewH);
        return BAD_VALUE;
    }

    if (m_isSupportedPreviewSize(newPreviewW, newPreviewH) == false) {
        ALOGE("ERR(%s): new preview size is invalid(%dx%d)", "Parameters", newPreviewW, newPreviewH);
        return BAD_VALUE;
    }

    ALOGI("INFO(%s):Cur Preview size(%dx%d)", "setParameters", curPreviewW, curPreviewH);
    ALOGI("INFO(%s):Cur HwPreview size(%dx%d)", "setParameters", curHwPreviewW, curHwPreviewH);
    ALOGI("INFO(%s):param.preview size(%dx%d)", "setParameters", previewW, previewH);
    ALOGI("INFO(%s):Adjust Preview size(%dx%d), ratioId(%d)", "setParameters", newPreviewW, newPreviewH, m_cameraInfo.previewSizeRatioId);
    ALOGI("INFO(%s):Calibrated HwPreview size(%dx%d)", "setParameters", newCalHwPreviewW, newCalHwPreviewH);

    if (curPreviewW != newPreviewW || curPreviewH != newPreviewH ||
        curHwPreviewW != newCalHwPreviewW || curHwPreviewH != newCalHwPreviewH ||
        getHighResolutionCallbackMode() == true) {
        m_setPreviewSize(newPreviewW, newPreviewH);
        m_setHwPreviewSize(newCalHwPreviewW, newCalHwPreviewH);

        if (getHighResolutionCallbackMode() == true) {
            m_previewSizeChanged = false;
        } else {
            ALOGD("DEBUG(%s):setRestartPreviewChecked true", __FUNCTION__);
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

bool ExynosCamera1Parameters::m_isSupportedPreviewSize(const int width,
                                                     const int height)
{
    int maxWidth, maxHeight = 0;
    int (*sizeList)[SIZE_OF_RESOLUTION];

    if (getHighResolutionCallbackMode() == true) {
        CLOGD("DEBUG(%s): Burst panorama mode start", __FUNCTION__);
#if defined(PANORAMA_RATIO)
        m_cameraInfo.previewSizeRatioId = PANORAMA_RATIO;
#else
        m_cameraInfo.previewSizeRatioId = SIZE_RATIO_16_9;
#endif
        return true;
    }

    getMaxPreviewSize(&maxWidth, &maxHeight);

    if (maxWidth*maxHeight < width*height) {
        CLOGE("ERR(%s):invalid PreviewSize(maxSize(%d/%d) size(%d/%d)",
            __FUNCTION__, maxWidth, maxHeight, width, height);
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

    CLOGE("ERR(%s):Invalid preview size(%dx%d)", __FUNCTION__, width, height);

    return false;
}

status_t ExynosCamera1Parameters::m_getPreviewSizeList(int *sizeList)
{
    int *tempSizeList = NULL;

    if (getHalVersion() == IS_HAL_VER_3_2) {
        /* CAMERA2_API use Video Scenario LUT as a default */
        if (m_staticInfo->videoSizeLut == NULL) {
            ALOGE("ERR(%s[%d]):videoSizeLut is NULL", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        } else if (m_staticInfo->videoSizeLutMax <= m_cameraInfo.previewSizeRatioId) {
            ALOGE("ERR(%s[%d]):unsupported video ratioId(%d)",
                    __FUNCTION__, __LINE__, m_cameraInfo.previewSizeRatioId);
            return BAD_VALUE;
        }
#if defined(ENABLE_FULL_FRAME)
        tempSizeList = m_staticInfo->videoSizeLut[m_cameraInfo.previewSizeRatioId];
#else
        tempSizeList = m_staticInfo->previewSizeLut[m_cameraInfo.previewSizeRatioId];
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
                    ALOGE("ERR(%s[%d]):previewSizeLut is NULL", __FUNCTION__, __LINE__);
                    return INVALID_OPERATION;
                } else if (m_staticInfo->previewSizeLutMax <= m_cameraInfo.previewSizeRatioId) {
                    ALOGE("ERR(%s[%d]):unsupported preview ratioId(%d)",
                            __FUNCTION__, __LINE__, m_cameraInfo.previewSizeRatioId);
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

                    fpsmode = getConfigMode();
                    tempSizeList = getHighSpeedSizeTable(fpsmode);
            }
#ifdef USE_BNS_RECORDING
                else if (m_staticInfo->videoSizeBnsLut != NULL
                         && videoW == 1920 && videoH == 1080) { /* Use BNS Recording only for FHD(16:9) */
                    if (m_staticInfo->videoSizeLutMax <= m_cameraInfo.previewSizeRatioId) {
                        ALOGE("ERR(%s[%d]):unsupported video ratioId(%d)",
                                __FUNCTION__, __LINE__, m_cameraInfo.previewSizeRatioId);
                        return BAD_VALUE;
                    }

                    tempSizeList = m_staticInfo->videoSizeBnsLut[m_cameraInfo.previewSizeRatioId];
                }
#endif
                else { /* Normal Recording Mode */
                    if (m_staticInfo->videoSizeLut == NULL) {
                        ALOGE("ERR(%s[%d]):videoSizeLut is NULL", __FUNCTION__, __LINE__);
                        return INVALID_OPERATION;
                    } else if (m_staticInfo->videoSizeLutMax <= m_cameraInfo.previewSizeRatioId) {
                        ALOGE("ERR(%s[%d]):unsupported video ratioId(%d)",
                                __FUNCTION__, __LINE__, m_cameraInfo.previewSizeRatioId);
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
                    ALOGE("ERR(%s[%d]):vtcallSizeLut is NULL", __FUNCTION__, __LINE__);
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
                    ALOGE("ERR(%s[%d]):previewSizeLut is NULL", __FUNCTION__, __LINE__);
                    return INVALID_OPERATION;
                } else if (m_staticInfo->previewSizeLutMax <= m_cameraInfo.previewSizeRatioId) {
                    ALOGE("ERR(%s[%d]):unsupported preview ratioId(%d)",
                            __FUNCTION__, __LINE__, m_cameraInfo.previewSizeRatioId);
                    return BAD_VALUE;
                }

                tempSizeList = m_staticInfo->previewSizeLut[m_cameraInfo.previewSizeRatioId];
            }
        }
    }

    if (tempSizeList == NULL) {
        ALOGE("ERR(%s[%d]):fail to get LUT", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    for (int i = 0; i < SIZE_LUT_INDEX_END; i++)
        sizeList[i] = tempSizeList[i];

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_getSWVdisPreviewSize(int w, int h, int *newW, int *newH)
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

bool ExynosCamera1Parameters::m_isHighResolutionCallbackSize(const int width, const int height)
{
#if defined(USE_LOW_RESOLUTION_PANORAMA)
    m_setHighResolutionCallbackMode(false);
    return false;
#else
    bool highResolutionCallbackMode;

    if (width == m_staticInfo->highResolutionCallbackW && height == m_staticInfo->highResolutionCallbackH)
        highResolutionCallbackMode = true;
    else
        highResolutionCallbackMode = false;

    CLOGD("DEBUG(%s):highResolutionCallSize:%s", "setParameters",
        highResolutionCallbackMode == true? "on":"off");

    m_setHighResolutionCallbackMode(highResolutionCallbackMode);

    return highResolutionCallbackMode;
#endif
}

void ExynosCamera1Parameters::m_isHighResolutionMode(const CameraParameters& params)
{
#if defined(USE_LOW_RESOLUTION_PANORAMA)
    m_setHighResolutionCallbackMode(false);
#else
    bool highResolutionCallbackMode;
    int shotmode = params.getInt("shot-mode");

    if ((getRecordingHint() == false) && (shotmode == SHOT_MODE_PANORAMA))
        highResolutionCallbackMode = true;
    else
        highResolutionCallbackMode = false;

    ALOGD("DEBUG(%s):highResolutionMode:%s", "setParameters",
        highResolutionCallbackMode == true? "on":"off");

    m_setHighResolutionCallbackMode(highResolutionCallbackMode);
#endif
}

void ExynosCamera1Parameters::m_setHighResolutionCallbackMode(bool enable)
{
    m_cameraInfo.highResolutionCallbackMode = enable;
}

bool ExynosCamera1Parameters::getHighResolutionCallbackMode(void)
{
    return m_cameraInfo.highResolutionCallbackMode;
}

status_t ExynosCamera1Parameters::checkPreviewFormat(const CameraParameters& params)
{
    const char *strNewPreviewFormat = params.getPreviewFormat();
    const char *strCurPreviewFormat = m_params.getPreviewFormat();
    int curHwPreviewFormat = getHwPreviewFormat();
    int newPreviewFormat = 0;
    int hwPreviewFormat = 0;

    CLOGD("DEBUG(%s):newPreviewFormat: %s", "setParameters", strNewPreviewFormat);

    if (!strcmp(strNewPreviewFormat, CameraParameters::PIXEL_FORMAT_RGB565))
        newPreviewFormat = V4L2_PIX_FMT_RGB565;
    else if (!strcmp(strNewPreviewFormat, CameraParameters::PIXEL_FORMAT_RGBA8888))
        newPreviewFormat = V4L2_PIX_FMT_RGB32;
    else if (!strcmp(strNewPreviewFormat, CameraParameters::PIXEL_FORMAT_YUV420SP))
        newPreviewFormat = V4L2_PIX_FMT_NV21;
    else if (!strcmp(strNewPreviewFormat, CameraParameters::PIXEL_FORMAT_YUV420P))
        newPreviewFormat = V4L2_PIX_FMT_YVU420;
    else if (!strcmp(strNewPreviewFormat, "yuv420sp_custom"))
        newPreviewFormat = V4L2_PIX_FMT_NV12T;
    else if (!strcmp(strNewPreviewFormat, "yuv422i"))
        newPreviewFormat = V4L2_PIX_FMT_YUYV;
    else if (!strcmp(strNewPreviewFormat, "yuv422p"))
        newPreviewFormat = V4L2_PIX_FMT_YUV422P;
    else
        newPreviewFormat = V4L2_PIX_FMT_NV21; /* for 3rd party */

    if (m_adjustPreviewFormat(newPreviewFormat, hwPreviewFormat) != NO_ERROR) {
        return BAD_VALUE;
    }

    m_setPreviewFormat(newPreviewFormat);
    m_params.setPreviewFormat(strNewPreviewFormat);
    if (curHwPreviewFormat != hwPreviewFormat) {
        m_setHwPreviewFormat(hwPreviewFormat);
        CLOGI("INFO(%s[%d]): preview format changed cur(%s) -> new(%s)", "Parameters", __LINE__, strCurPreviewFormat, strNewPreviewFormat);

        if (getPreviewRunning() == true) {
            CLOGD("DEBUG(%s[%d]):setRestartPreviewChecked true", __FUNCTION__, __LINE__);
            m_setRestartPreviewChecked(true);
        }
    }

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::m_adjustPreviewFormat(__unused int &previewFormat, int &hwPreviewFormat)
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

void ExynosCamera1Parameters::m_getCropRegion(int *x, int *y, int *w, int *h)
{
    getMetaCtlCropRegion(&m_metadata, x, y, w, h);
}

void ExynosCamera1Parameters::m_setPreviewSize(int w, int h)
{
    m_cameraInfo.previewW = w;
    m_cameraInfo.previewH = h;
}

void ExynosCamera1Parameters::getPreviewSize(int *w, int *h)
{
    *w = m_cameraInfo.previewW;
    *h = m_cameraInfo.previewH;
}

void ExynosCamera1Parameters::getYuvSize(int *width, int *height, const int index)
{
    *width = m_cameraInfo.yuvWidth[index];
    *height = m_cameraInfo.yuvHeight[index];
}

void ExynosCamera1Parameters::getMaxSensorSize(int *w, int *h)
{
    *w = m_staticInfo->maxSensorW;
    *h = m_staticInfo->maxSensorH;
}

void ExynosCamera1Parameters::getSensorMargin(int *w, int *h)
{
    *w = m_staticInfo->sensorMarginW;
    *h = m_staticInfo->sensorMarginH;
}

void ExynosCamera1Parameters::m_adjustSensorMargin(int *sensorMarginW, int *sensorMarginH)
{
    float bnsRatio = 1.00f;
    float binningRatio = 1.00f;
    float sensorMarginRatio = 1.00f;

    bnsRatio = (float)getBnsScaleRatio() / 1000.00f;
    binningRatio = (float)getBinningScaleRatio() / 1000.00f;
    sensorMarginRatio = bnsRatio * binningRatio;
    if ((int)sensorMarginRatio < 1) {
        ALOGW("WARN(%s[%d]):Invalid sensor margin ratio(%f), bnsRatio(%f), binningRatio(%f)",
                __FUNCTION__, __LINE__, sensorMarginRatio, bnsRatio, binningRatio);
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

void ExynosCamera1Parameters::getMaxPreviewSize(int *w, int *h)
{
    *w = m_staticInfo->maxPreviewW;
    *h = m_staticInfo->maxPreviewH;
}

void ExynosCamera1Parameters::m_setPreviewFormat(int fmt)
{
    m_cameraInfo.previewFormat = fmt;
}

int ExynosCamera1Parameters::getPreviewFormat(void)
{
    return m_cameraInfo.previewFormat;
}

void ExynosCamera1Parameters::m_setHwPreviewSize(int w, int h)
{
    m_cameraInfo.hwPreviewW = w;
    m_cameraInfo.hwPreviewH = h;
}

void ExynosCamera1Parameters::getHwPreviewSize(int *w, int *h)
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

void ExynosCamera1Parameters::setHwPreviewStride(int stride)
{
    m_cameraInfo.previewStride = stride;
}

int ExynosCamera1Parameters::getHwPreviewStride(void)
{
    return m_cameraInfo.previewStride;
}

void ExynosCamera1Parameters::m_setHwPreviewFormat(int fmt)
{
    m_cameraInfo.hwPreviewFormat = fmt;
}

int ExynosCamera1Parameters::getHwPreviewFormat(void)
{
    return m_cameraInfo.hwPreviewFormat;
}

void ExynosCamera1Parameters::updateHwSensorSize(void)
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
        CLOGE("ERR(%s):Invalid sensor size (maxSize(%d/%d) size(%d/%d)",
        __FUNCTION__, maxHwSensorW, maxHwSensorH, newHwSensorW, newHwSensorH);
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
    CLOGI("INFO(%s):curHwSensor size(%dx%d) newHwSensor size(%dx%d)", __FUNCTION__, curHwSensorW, curHwSensorH, newHwSensorW, newHwSensorH);
    if (curHwSensorW != newHwSensorW || curHwSensorH != newHwSensorH) {
        m_setHwSensorSize(newHwSensorW, newHwSensorH);
        CLOGI("INFO(%s):newHwSensor size(%dx%d)", __FUNCTION__, newHwSensorW, newHwSensorH);
    }
}

void ExynosCamera1Parameters::m_setHwSensorSize(int w, int h)
{
    m_cameraInfo.hwSensorW = w;
    m_cameraInfo.hwSensorH = h;
}

void ExynosCamera1Parameters::getHwSensorSize(int *w, int *h)
{
    ALOGV("INFO(%s[%d]) getScalableSensorMode()(%d)", __FUNCTION__, __LINE__, getScalableSensorMode());
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

void ExynosCamera1Parameters::updateBnsScaleRatio(void)
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
            ALOGI("INFO(%s[%d]):bnsRatio(%d), videoSize (%d, %d)",
                __FUNCTION__, __LINE__, bnsRatio, videoW, videoH);
        } else
#endif
        {
            bnsRatio = 1000;
        }
        if (bnsRatio != getBnsScaleRatio()) {
            CLOGI("INFO(%s[%d]): restart set due to changing  bnsRatio(%d/%d)",
                __FUNCTION__, __LINE__, bnsRatio, getBnsScaleRatio());
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
        CLOGE("ERR(%s[%d]): Cannot update BNS scale ratio(%d)", __FUNCTION__, __LINE__, bnsRatio);
}

status_t ExynosCamera1Parameters::m_setBnsScaleRatio(int ratio)
{
#define MIN_BNS_RATIO 1000
#define MAX_BNS_RATIO 8000

    if (m_staticInfo->bnsSupport == false) {
        CLOGD("DEBUG(%s[%d]): This camera does not support BNS", __FUNCTION__, __LINE__);
        ratio = MIN_BNS_RATIO;
    }

    if (ratio < MIN_BNS_RATIO || ratio > MAX_BNS_RATIO) {
        CLOGE("ERR(%s[%d]): Out of bound, ratio(%d), min:max(%d:%d)", __FUNCTION__, __LINE__, ratio, MAX_BNS_RATIO, MAX_BNS_RATIO);
        return BAD_VALUE;
    }

    CLOGD("DEBUG(%s[%d]): update BNS ratio(%d -> %d)", __FUNCTION__, __LINE__, m_cameraInfo.bnsScaleRatio, ratio);

    m_cameraInfo.bnsScaleRatio = ratio;

    /* When BNS scale ratio is changed, reset BNS size to MAX sensor size */
    getMaxSensorSize(&m_cameraInfo.bnsW, &m_cameraInfo.bnsH);

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::m_addHiddenResolutionList(String8 &string8Buf, __unused struct ExynosSensorInfoBase *sensorInfo,
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
        CLOGE("ERR(%s[%d]): invalid mode(%d)", __FUNCTION__, __LINE__, mode);
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

uint32_t ExynosCamera1Parameters::getBnsScaleRatio(void)
{
    return m_cameraInfo.bnsScaleRatio;
}

void ExynosCamera1Parameters::setBnsSize(int w, int h)
{
    m_cameraInfo.bnsW = w;
    m_cameraInfo.bnsH = h;

    updateHwSensorSize();

#if 0
    int zoom = getZoomLevel();
    int previewW = 0, previewH = 0;
    getPreviewSize(&previewW, &previewH);
    if (m_setParamCropRegion(zoom, w, h, previewW, previewH) != NO_ERROR)
        CLOGE("ERR(%s):m_setParamCropRegion() fail", __FUNCTION__);
#else
    ExynosRect srcRect, dstRect;
    getPreviewBayerCropSize(&srcRect, &dstRect);
#endif
}

void ExynosCamera1Parameters::getBnsSize(int *w, int *h)
{
    *w = m_cameraInfo.bnsW;
    *h = m_cameraInfo.bnsH;
}

void ExynosCamera1Parameters::updateBinningScaleRatio(void)
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
            ALOGE("ERR(%s[%d]): Invalide FastFpsMode(%d)", __FUNCTION__, __LINE__, fpsmode);
        }
    }
#ifdef USE_BINNING_MODE
    else if (getBinningMode() == true) {
        binningRatio = 2000;
    }
#endif

    if (binningRatio != getBinningScaleRatio()) {
        ALOGI("INFO(%s[%d]):New sensor binning ratio(%d)", __FUNCTION__, __LINE__, binningRatio);
        ret = m_setBinningScaleRatio(binningRatio);
    }

    if (ret < 0)
        ALOGE("ERR(%s[%d]): Cannot update BNS scale ratio(%d)", __FUNCTION__, __LINE__, binningRatio);
}

status_t ExynosCamera1Parameters::m_setBinningScaleRatio(int ratio)
{
#define MIN_BINNING_RATIO 1000
#define MAX_BINNING_RATIO 6000

    if (ratio < MIN_BINNING_RATIO || ratio > MAX_BINNING_RATIO) {
        ALOGE("ERR(%s[%d]): Out of bound, ratio(%d), min:max(%d:%d)",
                __FUNCTION__, __LINE__, ratio, MAX_BINNING_RATIO, MAX_BINNING_RATIO);
        return BAD_VALUE;
    }

    m_cameraInfo.binningScaleRatio = ratio;

    return NO_ERROR;
}

uint32_t ExynosCamera1Parameters::getBinningScaleRatio(void)
{
    return m_cameraInfo.binningScaleRatio;
}

status_t ExynosCamera1Parameters::checkPictureSize(const CameraParameters& params)
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

        CLOGE("ERR(%s):Invalid picture size(%dx%d)", __FUNCTION__, newPictureW, newPictureH);

        /* prevent wrong size setting */
        getMaxPictureSize(&maxHwPictureW, &maxHwPictureH);
        m_setPictureSize(maxHwPictureW, maxHwPictureH);
        m_setHwPictureSize(maxHwPictureW, maxHwPictureH);
        m_params.setPictureSize(maxHwPictureW, maxHwPictureH);
        CLOGE("ERR(%s):changed picture size to MAX(%dx%d)", __FUNCTION__, maxHwPictureW, maxHwPictureH);

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

status_t ExynosCamera1Parameters::m_adjustPictureSize(int *newPictureW, int *newPictureH,
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
           ALOGE("ERR(%s):m_getPreviewSizeList() fail", __FUNCTION__);
           return BAD_VALUE;
       }
    }

    getMaxPictureSize(newHwPictureW, newHwPictureH);

    //if (getCameraId() == CAMERA_ID_BACK) {
    // libcamera: 75xx: Fix size issue in preview and capture // Vijayakumar S N <vijay.sathenahallin@samsung.com>
    if (isReprocessing() == true
        && isUseYuvReprocessing() == false) {
        ret = getCropRectAlign(*newHwPictureW, *newHwPictureH,
                *newPictureW, *newPictureH,
                &newX, &newY, &newW, &newH,
                CAMERA_ISP_ALIGN, 2, 0, zoomRatio);
        if (ret < 0) {
            CLOGE("ERR(%s):getCropRectAlign(%d, %d, %d, %d) fail",
                    __FUNCTION__, *newHwPictureW, *newHwPictureH, *newPictureW, *newPictureH);
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
                CLOGD("(%s): Use sensor crop (ratio: %f)",
                        __FUNCTION__, ((float)*newPictureW / (float)*newPictureH));
                m_setHwSensorSize(newW, newH);
            }
        }
#endif
    // libcamera: 75xx: Fix size issue in preview and capture // Vijayakumar S N <vijay.sathenahallin@samsung.com>
    } else {
        /*
         * 15.05.29 <sw5771.park@samsung.com>
         * setFormat size set by hwPictureSize.
         * 3ac's per-frame size use getPreviewBayerCropSize.
         * if hwPictureSize(== m_getPreviewSizeList) is smaller than getPreviewBayerCropSize,
         * MMU fault is happen.
         * so, just comment out.
         * (originally, this code is for less memory.
         *  but, reserved is already allocated on system
         *       non-reverved memory is not much different size.)
         */
        /*
       int sizeList[SIZE_LUT_INDEX_END];
       if (m_getPreviewSizeList(sizeList) == NO_ERROR) {
           if (*newHwPictureW > sizeList[BCROP_W] || *newHwPictureH > sizeList[BCROP_H]) {
               *newHwPictureW = sizeList[BCROP_W];
               *newHwPictureH = sizeList[BCROP_H];
           }
       }
       */
    }

    return NO_ERROR;
}

bool ExynosCamera1Parameters::m_isSupportedPictureSize(const int width,
                                                     const int height)
{
    int maxWidth, maxHeight = 0;
    int (*sizeList)[SIZE_OF_RESOLUTION];

    getMaxPictureSize(&maxWidth, &maxHeight);

    if (maxWidth < width || maxHeight < height) {
        CLOGE("ERR(%s):invalid picture Size(maxSize(%d/%d) size(%d/%d)",
            __FUNCTION__, maxWidth, maxHeight, width, height);
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

    CLOGE("ERR(%s):Invalid picture size(%dx%d)", __FUNCTION__, width, height);

    return false;
}

void ExynosCamera1Parameters::m_setPictureSize(int w, int h)
{
    m_cameraInfo.pictureW = w;
    m_cameraInfo.pictureH = h;
}

void ExynosCamera1Parameters::getPictureSize(int *w, int *h)
{
    *w = m_cameraInfo.pictureW;
    *h = m_cameraInfo.pictureH;
}

void ExynosCamera1Parameters::getMaxPictureSize(int *w, int *h)
{
    *w = m_staticInfo->maxPictureW;
    *h = m_staticInfo->maxPictureH;
}

void ExynosCamera1Parameters::m_setHwPictureSize(int w, int h)
{
    m_cameraInfo.hwPictureW = w;
    m_cameraInfo.hwPictureH = h;
}

void ExynosCamera1Parameters::getHwPictureSize(int *w, int *h)
{
    *w = m_cameraInfo.hwPictureW;
    *h = m_cameraInfo.hwPictureH;
}

void ExynosCamera1Parameters::m_setHwBayerCropRegion(int w, int h, int x, int y)
{
    Mutex::Autolock lock(m_parameterLock);

    m_cameraInfo.hwBayerCropW = w;
    m_cameraInfo.hwBayerCropH = h;
    m_cameraInfo.hwBayerCropX = x;
    m_cameraInfo.hwBayerCropY = y;
}

void ExynosCamera1Parameters::getHwVraInputSize(int *w, int *h)
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
        CLOGW("WARN(%s[%d]):Invalid size ratio(%d)",
                __FUNCTION__, __LINE__, m_cameraInfo.previewSizeRatioId);

        *w = vraWidth;
        *h = vraHeight;
        break;
    }
}

int ExynosCamera1Parameters::getHwVraInputFormat(void)
{
#if defined(CAMERA_VRA_INPUT_FORMAT)
    return CAMERA_VRA_INPUT_FORMAT;
#else
    return V4L2_PIX_FMT_NV21;
#endif
}

void ExynosCamera1Parameters::getHwBayerCropRegion(int *w, int *h, int *x, int *y)
{
    Mutex::Autolock lock(m_parameterLock);

    *w = m_cameraInfo.hwBayerCropW;
    *h = m_cameraInfo.hwBayerCropH;
    *x = m_cameraInfo.hwBayerCropX;
    *y = m_cameraInfo.hwBayerCropY;
}

void ExynosCamera1Parameters::m_setPictureFormat(int fmt)
{
    m_cameraInfo.pictureFormat = fmt;
}

int ExynosCamera1Parameters::getPictureFormat(void)
{
    return m_cameraInfo.pictureFormat;
}

void ExynosCamera1Parameters::m_setHwPictureFormat(int fmt)
{
    m_cameraInfo.hwPictureFormat = fmt;
}

int ExynosCamera1Parameters::getHwPictureFormat(void)
{
    return m_cameraInfo.hwPictureFormat;
}

status_t ExynosCamera1Parameters::checkJpegQuality(const CameraParameters& params)
{
    int newJpegQuality = params.getInt(CameraParameters::KEY_JPEG_QUALITY);
    int curJpegQuality = getJpegQuality();

    CLOGD("DEBUG(%s):newJpegQuality %d", "setParameters", newJpegQuality);

    if (newJpegQuality < 1 || newJpegQuality > 100) {
        CLOGE("ERR(%s): Invalid Jpeg Quality (Min: %d, Max: %d, Value: %d)", __FUNCTION__, 1, 100, newJpegQuality);
        return BAD_VALUE;
    }

    if (curJpegQuality != newJpegQuality) {
        m_setJpegQuality(newJpegQuality);
        m_params.set(CameraParameters::KEY_JPEG_QUALITY, newJpegQuality);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setJpegQuality(int quality)
{
    m_cameraInfo.jpegQuality = quality;
}

int ExynosCamera1Parameters::getJpegQuality(void)
{
    return m_cameraInfo.jpegQuality;
}

status_t ExynosCamera1Parameters::checkThumbnailSize(const CameraParameters& params)
{
    int curThumbnailW = 0;
    int curThumbnailH = 0;
    int maxThumbnailW = 0;
    int maxThumbnailH = 0;
    int newJpegThumbnailW = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH);
    int newJpegThumbnailH = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT);

    CLOGD("DEBUG(%s):newJpegThumbnailW X newJpegThumbnailH: %d X %d", "setParameters", newJpegThumbnailW, newJpegThumbnailH);

    getMaxThumbnailSize(&maxThumbnailW, &maxThumbnailH);

    if (newJpegThumbnailW < 0 || newJpegThumbnailH < 0 ||
        newJpegThumbnailW > maxThumbnailW || newJpegThumbnailH > maxThumbnailH) {
        CLOGE("ERR(%s): Invalid Thumbnail Size (maxSize(%d/%d) size(%d/%d)", __FUNCTION__, maxThumbnailW, maxThumbnailH, newJpegThumbnailW, newJpegThumbnailH);
        return BAD_VALUE;
    }

    getThumbnailSize(&curThumbnailW, &curThumbnailH);

    if (curThumbnailW != newJpegThumbnailW || curThumbnailH != newJpegThumbnailH) {
        m_setThumbnailSize(newJpegThumbnailW, newJpegThumbnailH);
        m_params.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH,  newJpegThumbnailW);
        m_params.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT, newJpegThumbnailH);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setThumbnailSize(int w, int h)
{
    m_cameraInfo.thumbnailW = w;
    m_cameraInfo.thumbnailH = h;
}

void ExynosCamera1Parameters::getThumbnailSize(int *w, int *h)
{
    *w = m_cameraInfo.thumbnailW;
    *h = m_cameraInfo.thumbnailH;
}

void ExynosCamera1Parameters::getMaxThumbnailSize(int *w, int *h)
{
    *w = m_staticInfo->maxThumbnailW;
    *h = m_staticInfo->maxThumbnailH;
}

status_t ExynosCamera1Parameters::checkThumbnailQuality(const CameraParameters& params)
{
    int newJpegThumbnailQuality = params.getInt(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY);
    int curThumbnailQuality = getThumbnailQuality();

    CLOGD("DEBUG(%s):newJpegThumbnailQuality %d", "setParameters", newJpegThumbnailQuality);

    if (newJpegThumbnailQuality < 0 || newJpegThumbnailQuality > 100) {
        CLOGE("ERR(%s): Invalid Thumbnail Quality (Min: %d, Max: %d, Value: %d)", __FUNCTION__, 0, 100, newJpegThumbnailQuality);
        return BAD_VALUE;
    }

    if (curThumbnailQuality != newJpegThumbnailQuality) {
        m_setThumbnailQuality(newJpegThumbnailQuality);
        m_params.set(CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY, newJpegThumbnailQuality);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setThumbnailQuality(int quality)
{
    m_cameraInfo.thumbnailQuality = quality;
}

int ExynosCamera1Parameters::getThumbnailQuality(void)
{
    return m_cameraInfo.thumbnailQuality;
}

status_t ExynosCamera1Parameters::check3dnrMode(const CameraParameters& params)
{
    bool new3dnrMode = false;
    bool cur3dnrMode = false;
    const char *str3dnrMode = params.get("3dnr");

    if (str3dnrMode == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):new3dnrMode %s", "setParameters", str3dnrMode);

    if (!strcmp(str3dnrMode, "true"))
        new3dnrMode = true;

    if (m_flag3dnrMode != new3dnrMode) {
        m_flag3dnrMode = new3dnrMode;
        m_params.set("3dnr", str3dnrMode);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_set3dnrMode(bool toggle)
{
    m_cameraInfo.is3dnrMode = toggle;
}

bool ExynosCamera1Parameters::get3dnrMode(void)
{
    return m_cameraInfo.is3dnrMode;
}

status_t ExynosCamera1Parameters::checkDrcMode(const CameraParameters& params)
{
    bool newDrcMode = false;
    bool curDrcMode = false;
    const char *strDrcMode = params.get("drc");

    if (strDrcMode == NULL) {
#ifdef USE_FRONT_PREVIEW_DRC
        if (getCameraId() == CAMERA_ID_FRONT && m_staticInfo->drcSupport == true) {
            newDrcMode = !getRecordingHint();
            m_setDrcMode(newDrcMode);
            ALOGD("DEBUG(%s):Force DRC %s for front", "setParameters",
                    (newDrcMode == true)? "ON" : "OFF");
        }
#endif
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):newDrcMode %s", "setParameters", strDrcMode);

    if (!strcmp(strDrcMode, "true"))
        newDrcMode = true;

    curDrcMode = getDrcMode();

    if (curDrcMode != newDrcMode) {
        m_setDrcMode(newDrcMode);
        m_params.set("drc", strDrcMode);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setDrcMode(bool toggle)
{
    m_cameraInfo.isDrcMode = toggle;
    if (setDrcEnable(toggle) < 0) {
        CLOGE("ERR(%s[%d]): set DRC fail, toggle(%d)", __FUNCTION__, __LINE__, toggle);
    }
}

bool ExynosCamera1Parameters::getDrcMode(void)
{
    return m_cameraInfo.isDrcMode;
}

status_t ExynosCamera1Parameters::checkOdcMode(const CameraParameters& params)
{
    bool newOdcMode = false;
    bool curOdcMode = false;
    const char *strOdcMode = params.get("odc");

    if (strOdcMode == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):newOdcMode %s", "setParameters", strOdcMode);

    if (!strcmp(strOdcMode, "true"))
        newOdcMode = true;

    curOdcMode = getOdcMode();

    if (curOdcMode != newOdcMode) {
        m_setOdcMode(newOdcMode);
        m_params.set("odc", strOdcMode);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setOdcMode(bool toggle)
{
    m_cameraInfo.isOdcMode = toggle;
}

bool ExynosCamera1Parameters::getOdcMode(void)
{
    return m_cameraInfo.isOdcMode;
}

bool ExynosCamera1Parameters::getTpuEnabledMode(void)
{
    if (getHWVdisMode() == true)
        return true;

    if (get3dnrMode() == true)
        return true;

    if (getOdcMode() == true)
        return true;

    return false;
}

status_t ExynosCamera1Parameters::checkZoomLevel(const CameraParameters& params)
{
    int newZoom = params.getInt(CameraParameters::KEY_ZOOM);
    int curZoom = 0;

    CLOGD("DEBUG(%s):newZoom %d", "setParameters", newZoom);

    /* cannot support DZoom -> set Zoom Level 0 */
    if (getZoomSupported() == false) {
        if (newZoom != ZOOM_LEVEL_0) {
            CLOGE("ERR(%s):Invalid value (Zoom Should be %d, Value: %d)", __FUNCTION__, ZOOM_LEVEL_0, newZoom);
            return BAD_VALUE;
        }

        if (setZoomLevel(ZOOM_LEVEL_0) != NO_ERROR)
            return BAD_VALUE;

        return NO_ERROR;
    } else {
        if (newZoom < ZOOM_LEVEL_0 || getMaxZoomLevel() <= newZoom) {
            CLOGE("ERR(%s):Invalid value (Min: %d, Max: %d, Value: %d)", __FUNCTION__, ZOOM_LEVEL_0, getMaxZoomLevel(), newZoom);
            return BAD_VALUE;
        }

        if (setZoomLevel(newZoom) != NO_ERROR) {
            return BAD_VALUE;
        }
        m_params.set(CameraParameters::KEY_ZOOM, newZoom);

        m_flagMeteringRegionChanged = true;

        return NO_ERROR;
    }
    return NO_ERROR;
}

status_t ExynosCamera1Parameters::setZoomLevel(int zoom)
{
    int srcW = 0;
    int srcH = 0;
    int dstW = 0;
    int dstH = 0;
#ifdef USE_FW_ZOOMRATIO
    float zoomRatio = 1.00f;
#endif

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
#ifdef USE_FW_ZOOMRATIO
    zoomRatio = getZoomRatio(zoom) / 1000;
    setMetaCtlZoom(&m_metadata, zoomRatio);
#endif

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::m_setParamCropRegion(
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
        CLOGE("ERR(%s):getCropRectAlign(%d, %d, %d, %d) fail",
            __func__, srcW,  srcH, dstW,  dstH);
        return BAD_VALUE;
    }

    newX = ALIGN_UP(newX, 2);
    newY = ALIGN_UP(newY, 2);
    newW = srcW - (newX * 2);
    newH = srcH - (newY * 2);

    CLOGI("DEBUG(%s):size0(%d, %d, %d, %d)",
        __FUNCTION__, srcW, srcH, dstW, dstH);
    CLOGI("DEBUG(%s):size(%d, %d, %d, %d), level(%d)",
        __FUNCTION__, newX, newY, newW, newH, zoom);

    m_setHwBayerCropRegion(newW, newH, newX, newY);

    return NO_ERROR;
}

int ExynosCamera1Parameters::getZoomLevel(void)
{
    return m_cameraInfo.zoom;
}

status_t ExynosCamera1Parameters::checkRotation(const CameraParameters& params)
{
    int newRotation = params.getInt(CameraParameters::KEY_ROTATION);
    int curRotation = 0;

    if (newRotation < 0) {
        CLOGE("ERR(%s): Invalide Rotation value(%d)", __FUNCTION__, newRotation);
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):set orientation:%d", "setParameters", newRotation);

    curRotation = getRotation();

    if (curRotation != newRotation) {
        m_setRotation(newRotation);
        m_params.set(CameraParameters::KEY_ROTATION, newRotation);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setRotation(int rotation)
{
    m_cameraInfo.rotation = rotation;
}

int ExynosCamera1Parameters::getRotation(void)
{
    return m_cameraInfo.rotation;
}

status_t ExynosCamera1Parameters::checkAutoExposureLock(const CameraParameters& params)
{
    bool newAutoExposureLock = false;
    bool curAutoExposureLock = false;
    const char *strAutoExposureLock = params.get(CameraParameters::KEY_AUTO_EXPOSURE_LOCK);
    if (strAutoExposureLock == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):newAutoExposureLock %s", "setParameters", strAutoExposureLock);

    if (!strcmp(strAutoExposureLock, "true"))
        newAutoExposureLock = true;

    curAutoExposureLock = getAutoExposureLock();

    if (curAutoExposureLock != newAutoExposureLock) {
        ExynosCameraActivityFlash *m_flashMgr = m_activityControl->getFlashMgr();
        m_flashMgr->setAeLock(newAutoExposureLock);
        m_setAutoExposureLock(newAutoExposureLock);
        m_params.set(CameraParameters::KEY_AUTO_EXPOSURE_LOCK, strAutoExposureLock);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setAutoExposureLock(bool lock)
{
    if (getHalVersion() != IS_HAL_VER_3_2) {
        m_cameraInfo.autoExposureLock = lock;
        setMetaCtlAeLock(&m_metadata, lock);
    }
}

bool ExynosCamera1Parameters::getAutoExposureLock(void)
{
    return m_cameraInfo.autoExposureLock;
}

status_t ExynosCamera1Parameters::checkExposureCompensation(const CameraParameters& params)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return NO_ERROR;
    }

    int minExposureCompensation = params.getInt(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION);
    int maxExposureCompensation = params.getInt(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION);
    int newExposureCompensation = params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
    int curExposureCompensation = getExposureCompensation();

    CLOGD("DEBUG(%s):newExposureCompensation %d", "setParameters", newExposureCompensation);

    if ((newExposureCompensation < minExposureCompensation) ||
        (newExposureCompensation > maxExposureCompensation)) {
        CLOGE("ERR(%s): Invalide Exposurecompensation (Min: %d, Max: %d, Value: %d)", __FUNCTION__,
            minExposureCompensation, maxExposureCompensation, newExposureCompensation);
        return BAD_VALUE;
    }

    if (curExposureCompensation != newExposureCompensation) {
        m_setExposureCompensation(newExposureCompensation);
        m_params.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, newExposureCompensation);
    }

    return NO_ERROR;
}

int32_t ExynosCamera1Parameters::getLongExposureShotCount(void)
{
    bool getResult;
    int32_t count = 0;
#ifdef CAMERA_ADD_BAYER_ENABLE
    if (m_exposureTimeCapture <= CAMERA_EXPOSURE_TIME_MAX)
#endif
    {
        return 0;
    }

    if (m_exposureTimeCapture % CAMERA_EXPOSURE_TIME_MAX) {
        count = 2;
        getResult = false;
        while (!getResult) {
            if (m_exposureTimeCapture % count) {
                count++;
                continue;
            }
            if (CAMERA_EXPOSURE_TIME_MAX < (m_exposureTimeCapture / count)) {
                count++;
                continue;
            }
            getResult = true;
        }
        return count - 1;
    } else {
        return m_exposureTimeCapture / CAMERA_EXPOSURE_TIME_MAX - 1;
    }
}

status_t ExynosCamera1Parameters::checkMeteringAreas(const CameraParameters& params)
{
    int ret = NO_ERROR;
    const char *newMeteringAreas = params.get(CameraParameters::KEY_METERING_AREAS);
    const char *curMeteringAreas = m_params.get(CameraParameters::KEY_METERING_AREAS);
    const char meteringAreas[20] = "(0,0,0,0,0)";
    bool nullCheckflag = false;

    int newMeteringAreasSize = 0;
    bool isMeteringAreasSame = false;
    uint32_t maxNumMeteringAreas = getMaxNumMeteringAreas();

    if (newMeteringAreas == NULL || (newMeteringAreas != NULL && !strcmp(newMeteringAreas, "(0,0,0,0,0)"))) {
        if(getMeteringMode() == METERING_MODE_SPOT) {
            newMeteringAreas = meteringAreas;
            nullCheckflag = true;
        } else {
            setMetaCtlAeRegion(&m_metadata, 0, 0, 0, 0, 0);
            return NO_ERROR;
        }
    }

    if(getSamsungCamera()) {
        maxNumMeteringAreas = 1;
    }

    if (maxNumMeteringAreas <= 0) {
        CLOGD("DEBUG(%s): meterin area is not supported", "Parameters");
        return NO_ERROR;
    }

    ALOGD("DEBUG(%s):newMeteringAreas: %s ,maxNumMeteringAreas(%d)", "setParameters", newMeteringAreas, maxNumMeteringAreas);

    newMeteringAreasSize = strlen(newMeteringAreas);
    if (curMeteringAreas != NULL) {
        isMeteringAreasSame = !strncmp(newMeteringAreas, curMeteringAreas, newMeteringAreasSize);
    }

    if (curMeteringAreas == NULL || isMeteringAreasSame == false || m_flagMeteringRegionChanged == true) {
        /* ex : (-10,-10,0,0,300),(0,0,10,10,700) */
        ExynosRect2 *rect2s  = new ExynosRect2[maxNumMeteringAreas];
        int         *weights = new int[maxNumMeteringAreas];
        uint32_t validMeteringAreas = bracketsStr2Ints((char *)newMeteringAreas, maxNumMeteringAreas, rect2s, weights, 1);

        if (0 < validMeteringAreas && validMeteringAreas <= maxNumMeteringAreas) {
            m_setMeteringAreas((uint32_t)validMeteringAreas, rect2s, weights);
            if(!nullCheckflag) {
                m_params.set(CameraParameters::KEY_METERING_AREAS, newMeteringAreas);
            }
        } else {
            CLOGE("ERR(%s):MeteringAreas value is invalid", __FUNCTION__);
            ret = UNKNOWN_ERROR;
        }

        m_flagMeteringRegionChanged = false;
        delete [] rect2s;
        delete [] weights;
    }

    return ret;
}

void ExynosCamera1Parameters::m_setMeteringAreas(uint32_t num, ExynosRect  *rects, int *weights)
{
    ExynosRect2 *rect2s = new ExynosRect2[num];

    for (uint32_t i = 0; i < num; i++)
        convertingRectToRect2(&rects[i], &rect2s[i]);

    m_setMeteringAreas(num, rect2s, weights);

    delete [] rect2s;
}

void ExynosCamera1Parameters::getMeteringAreas(__unused ExynosRect *rects)
{
    /* TODO */
}

void ExynosCamera1Parameters::getMeteringAreas(__unused ExynosRect2 *rect2s)
{
    /* TODO */
}

void ExynosCamera1Parameters::m_setMeteringMode(int meteringMode)
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
        CLOGD("DEBUG(%s):autoExposure is Locked", __FUNCTION__);
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

int ExynosCamera1Parameters::getMeteringMode(void)
{
    return m_cameraInfo.meteringMode;
}

int ExynosCamera1Parameters::getSupportedMeteringMode(void)
{
    return m_staticInfo->meteringList;
}

status_t ExynosCamera1Parameters::checkAntibanding(const CameraParameters& params)
{
    int newAntibanding = -1;
    int curAntibanding = -1;

    const char *strKeyAntibanding = params.get(CameraParameters::KEY_ANTIBANDING);
    const char *strNewAntibanding = m_adjustAntibanding(strKeyAntibanding);

    if (strNewAntibanding == NULL) {
        return NO_ERROR;
    }
    CLOGD("DEBUG(%s):strNewAntibanding %s", "setParameters", strNewAntibanding);

    if (!strcmp(strNewAntibanding, CameraParameters::ANTIBANDING_AUTO))
        newAntibanding = AA_AE_ANTIBANDING_AUTO;
    else if (!strcmp(strNewAntibanding, CameraParameters::ANTIBANDING_50HZ))
        newAntibanding = AA_AE_ANTIBANDING_AUTO_50HZ;
    else if (!strcmp(strNewAntibanding, CameraParameters::ANTIBANDING_60HZ))
        newAntibanding = AA_AE_ANTIBANDING_AUTO_60HZ;
    else if (!strcmp(strNewAntibanding, CameraParameters::ANTIBANDING_OFF))
        newAntibanding = AA_AE_ANTIBANDING_OFF;
    else {
        CLOGE("ERR(%s):Invalid antibanding value(%s)", __FUNCTION__, strNewAntibanding);
        return BAD_VALUE;
    }

    curAntibanding = getAntibanding();

    if (curAntibanding != newAntibanding) {
        m_setAntibanding(newAntibanding);
    }

    if (strKeyAntibanding != NULL) {
        m_params.set(CameraParameters::KEY_ANTIBANDING, strKeyAntibanding);
    }

    return NO_ERROR;
}

const char *ExynosCamera1Parameters::m_adjustAntibanding(const char *strAntibanding)
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


void ExynosCamera1Parameters::m_setAntibanding(int value)
{
    setMetaCtlAntibandingMode(&m_metadata, (enum aa_ae_antibanding_mode)value);
}

int ExynosCamera1Parameters::getAntibanding(void)
{
    enum aa_ae_antibanding_mode antibanding;
    getMetaCtlAntibandingMode(&m_metadata, &antibanding);
    return (int)antibanding;
}

int ExynosCamera1Parameters::getSupportedAntibanding(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return 0;
    } else {
        return m_staticInfo->antiBandingList;
    }
}

int ExynosCamera1Parameters::getSceneMode(void)
{
    return m_cameraInfo.sceneMode;
}

int ExynosCamera1Parameters::getSupportedSceneModes(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return 0;
    } else {
        return m_staticInfo->sceneModeList;
    }
}

void ExynosCamera1Parameters::setFocusModeLock(bool enable) {
    int curFocusMode = getFocusMode();

    ALOGD("DEBUG(%s):FocusModeLock (%s)", __FUNCTION__, enable? "true" : "false");

    if(enable) {
        m_activityControl->stopAutoFocus();
    } else {
        m_setFocusMode(curFocusMode);
    }
}

void ExynosCamera1Parameters::setFocusModeSetting(bool enable)
{
    m_setFocusmodeSetting = enable;
}

int ExynosCamera1Parameters::getFocusModeSetting(void)
{
    return m_setFocusmodeSetting;
}

int ExynosCamera1Parameters::getFocusMode(void)
{
    return m_cameraInfo.focusMode;
}

int ExynosCamera1Parameters::getSupportedFocusModes(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return 0;
    } else {
        return m_staticInfo->focusModeList;
    }
}

status_t ExynosCamera1Parameters::checkFlashMode(const CameraParameters& params)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return NO_ERROR;
    } else {
        int  newFlashMode = -1;
        int  curFlashMode = -1;
        const char *strFlashMode = params.get(CameraParameters::KEY_FLASH_MODE);

        if (strFlashMode == NULL) {
            return NO_ERROR;
        }

        const char *strNewFlashMode = m_adjustFlashMode(strFlashMode);

        if (strNewFlashMode == NULL) {
            return NO_ERROR;
        }

        CLOGD("DEBUG(%s):strNewFlashMode %s", "setParameters", strNewFlashMode);

        if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_OFF))
            newFlashMode = FLASH_MODE_OFF;
        else if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_AUTO))
            newFlashMode = FLASH_MODE_AUTO;
        else if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_ON))
            newFlashMode = FLASH_MODE_ON;
        else if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_RED_EYE))
            newFlashMode = FLASH_MODE_RED_EYE;
        else if (!strcmp(strNewFlashMode, CameraParameters::FLASH_MODE_TORCH))
            newFlashMode = FLASH_MODE_TORCH;
        else {
            CLOGE("ERR(%s):unmatched flash_mode(%s), turn off flash", __FUNCTION__, strNewFlashMode);
            newFlashMode = FLASH_MODE_OFF;
            strNewFlashMode = CameraParameters::FLASH_MODE_OFF;
            return BAD_VALUE;
        }

#ifndef UNSUPPORT_FLASH
        if (!(newFlashMode & getSupportedFlashModes())) {
            CLOGE("ERR(%s[%d]): Flash mode(%s) is not supported!", __FUNCTION__, __LINE__, strNewFlashMode);
            return BAD_VALUE;
        }
#endif

        curFlashMode = getFlashMode();

        if (curFlashMode != newFlashMode) {
            m_setFlashMode(newFlashMode);
            m_params.set(CameraParameters::KEY_FLASH_MODE, strNewFlashMode);
        }

        return NO_ERROR;
    }
}

const char *ExynosCamera1Parameters::m_adjustFlashMode(const char *flashMode)
{
    int sceneMode = getSceneMode();
    const char *newFlashMode = NULL;

    /* TODO: vendor specific adjust */

    newFlashMode = flashMode;

    return newFlashMode;
}

void ExynosCamera1Parameters::m_setFlashMode(int flashMode)
{
    m_cameraInfo.flashMode = flashMode;

    /* TODO: Notity flash activity */
    m_activityControl->setFlashMode(flashMode);
}

int ExynosCamera1Parameters::getFlashMode(void)
{
    return m_cameraInfo.flashMode;
}

int ExynosCamera1Parameters::getSupportedFlashModes(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return 0;
    } else {
        return m_staticInfo->flashModeList;
    }
}

status_t ExynosCamera1Parameters::checkWhiteBalanceMode(const CameraParameters& params)
{
    if (getHalVersion() != IS_HAL_VER_3_2) {
        int newWhiteBalance = -1;
        int curWhiteBalance = -1;
        const char *strWhiteBalance = params.get(CameraParameters::KEY_WHITE_BALANCE);
        const char *strNewWhiteBalance = m_adjustWhiteBalanceMode(strWhiteBalance);

        if (strNewWhiteBalance == NULL) {
            return NO_ERROR;
        }

        CLOGD("DEBUG(%s):newWhiteBalance %s", "setParameters", strNewWhiteBalance);

        if (!strcmp(strNewWhiteBalance, CameraParameters::WHITE_BALANCE_AUTO))
            newWhiteBalance = WHITE_BALANCE_AUTO;
        else if (!strcmp(strNewWhiteBalance, CameraParameters::WHITE_BALANCE_INCANDESCENT))
            newWhiteBalance = WHITE_BALANCE_INCANDESCENT;
        else if (!strcmp(strNewWhiteBalance, CameraParameters::WHITE_BALANCE_FLUORESCENT))
            newWhiteBalance = WHITE_BALANCE_FLUORESCENT;
        else if (!strcmp(strNewWhiteBalance, CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT))
            newWhiteBalance = WHITE_BALANCE_WARM_FLUORESCENT;
        else if (!strcmp(strNewWhiteBalance, CameraParameters::WHITE_BALANCE_DAYLIGHT))
            newWhiteBalance = WHITE_BALANCE_DAYLIGHT;
        else if (!strcmp(strNewWhiteBalance, CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT))
            newWhiteBalance = WHITE_BALANCE_CLOUDY_DAYLIGHT;
        else if (!strcmp(strNewWhiteBalance, CameraParameters::WHITE_BALANCE_TWILIGHT))
            newWhiteBalance = WHITE_BALANCE_TWILIGHT;
        else if (!strcmp(strNewWhiteBalance, CameraParameters::WHITE_BALANCE_SHADE))
            newWhiteBalance = WHITE_BALANCE_SHADE;
        else {
            CLOGE("ERR(%s):Invalid white balance(%s)", __FUNCTION__, strNewWhiteBalance);
            return BAD_VALUE;
        }

        if (!(newWhiteBalance & getSupportedWhiteBalance())) {
            CLOGE("ERR(%s[%d]): white balance mode(%s) is not supported", __FUNCTION__, __LINE__, strNewWhiteBalance);
            return BAD_VALUE;
        }

        curWhiteBalance = getWhiteBalanceMode();

        if (getSceneMode() == SCENE_MODE_AUTO) {
            enum aa_awbmode cur_awbMode;
            getMetaCtlAwbMode(&m_metadata, &cur_awbMode);

            if (m_setWhiteBalanceMode(newWhiteBalance) != NO_ERROR)
                return BAD_VALUE;

            m_params.set(CameraParameters::KEY_WHITE_BALANCE, strNewWhiteBalance);
        }
    }
    return NO_ERROR;
}

const char *ExynosCamera1Parameters::m_adjustWhiteBalanceMode(const char *whiteBalance)
{
    int sceneMode = getSceneMode();
    const char *newWhiteBalance = NULL;

    /* TODO: vendor specific adjust */

    /* TN' feautre can change whiteBalance even if Non SCENE_MODE_AUTO */

    newWhiteBalance = whiteBalance;

    return newWhiteBalance;
}

status_t ExynosCamera1Parameters::m_setWhiteBalanceMode(int whiteBalance)
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

int ExynosCamera1Parameters::m_convertMetaCtlAwbMode(struct camera2_shot_ext *shot_ext)
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
            ALOGE("ERR(%s):Unsupported awbMode(%d)", __FUNCTION__, shot_ext->shot.ctl.aa.awbMode);
            return BAD_VALUE;
    }

    return awbMode;
}

int ExynosCamera1Parameters::getWhiteBalanceMode(void)
{
    return m_cameraInfo.whiteBalanceMode;
}

int ExynosCamera1Parameters::getSupportedWhiteBalance(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return 0;
    } else {
        return m_staticInfo->whiteBalanceList;
    }
}

int ExynosCamera1Parameters::getSupportedISO(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return 0;
    } else {
        return m_staticInfo->isoValues;
    }
}

status_t ExynosCamera1Parameters::checkAutoWhiteBalanceLock(const CameraParameters& params)
{
    if (getHalVersion() != IS_HAL_VER_3_2) {
        bool newAutoWhiteBalanceLock = false;
        bool curAutoWhiteBalanceLock = false;
        const char *strNewAutoWhiteBalanceLock = params.get(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK);

        if (strNewAutoWhiteBalanceLock == NULL) {
            return NO_ERROR;
        }

        CLOGD("DEBUG(%s):strNewAutoWhiteBalanceLock %s", "setParameters", strNewAutoWhiteBalanceLock);

        if (!strcmp(strNewAutoWhiteBalanceLock, "true"))
            newAutoWhiteBalanceLock = true;

        curAutoWhiteBalanceLock = getAutoWhiteBalanceLock();

        if (curAutoWhiteBalanceLock != newAutoWhiteBalanceLock) {
            ExynosCameraActivityFlash *m_flashMgr = m_activityControl->getFlashMgr();
            m_flashMgr->setAwbLock(newAutoWhiteBalanceLock);
            m_setAutoWhiteBalanceLock(newAutoWhiteBalanceLock);
            m_params.set(CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK, strNewAutoWhiteBalanceLock);
        }
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setAutoWhiteBalanceLock(bool value)
{
    if (getHalVersion() != IS_HAL_VER_3_2) {
        m_cameraInfo.autoWhiteBalanceLock = value;
        setMetaCtlAwbLock(&m_metadata, value);
    }
}

bool ExynosCamera1Parameters::getAutoWhiteBalanceLock(void)
{
    return m_cameraInfo.autoWhiteBalanceLock;
}

void ExynosCamera1Parameters::m_setFocusAreas(uint32_t numValid, ExynosRect *rects, int *weights)
{
    ExynosRect2 *rect2s = new ExynosRect2[numValid];

    for (uint32_t i = 0; i < numValid; i++)
        convertingRectToRect2(&rects[i], &rect2s[i]);

    m_setFocusAreas(numValid, rect2s, weights);

    delete [] rect2s;
}

void ExynosCamera1Parameters::getFocusAreas(int *validFocusArea, ExynosRect2 *rect2s, int *weights)
{
    *validFocusArea = m_cameraInfo.numValidFocusArea;

    if (*validFocusArea != 0) {
        /* Currently only supported 1 region */
        getMetaCtlAfRegion(&m_metadata, &rect2s->x1, &rect2s->y1,
                            &rect2s->x2, &rect2s->y2, weights);
    }
}

status_t ExynosCamera1Parameters::checkColorEffectMode(const CameraParameters& params)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return NO_ERROR;
    } else {
        int newEffectMode = EFFECT_NONE;
        int curEffectMode = EFFECT_NONE;
        const char *strNewEffectMode = params.get(CameraParameters::KEY_EFFECT);

        if (strNewEffectMode == NULL) {
            return NO_ERROR;
        }

        CLOGD("DEBUG(%s):strNewEffectMode %s", "setParameters", strNewEffectMode);

        if (!strcmp(strNewEffectMode, CameraParameters::EFFECT_NONE)) {
            newEffectMode = EFFECT_NONE;
        } else if (!strcmp(strNewEffectMode, CameraParameters::EFFECT_MONO)) {
            newEffectMode = EFFECT_MONO;
        } else if (!strcmp(strNewEffectMode, CameraParameters::EFFECT_NEGATIVE)) {
            newEffectMode = EFFECT_NEGATIVE;
        } else if (!strcmp(strNewEffectMode, CameraParameters::EFFECT_SOLARIZE)) {
            newEffectMode = EFFECT_SOLARIZE;
        } else if (!strcmp(strNewEffectMode, CameraParameters::EFFECT_SEPIA)) {
            newEffectMode = EFFECT_SEPIA;
        } else if (!strcmp(strNewEffectMode, CameraParameters::EFFECT_POSTERIZE)) {
            newEffectMode = EFFECT_POSTERIZE;
        } else if (!strcmp(strNewEffectMode, CameraParameters::EFFECT_WHITEBOARD)) {
            newEffectMode = EFFECT_WHITEBOARD;
        } else if (!strcmp(strNewEffectMode, CameraParameters::EFFECT_BLACKBOARD)) {
            newEffectMode = EFFECT_BLACKBOARD;
        } else if (!strcmp(strNewEffectMode, CameraParameters::EFFECT_AQUA)) {
            newEffectMode = EFFECT_AQUA;
        } else if (!strcmp(strNewEffectMode, CameraParameters::EFFECT_POINT_BLUE)) {
            newEffectMode = EFFECT_BLUE;
        } else if (!strcmp(strNewEffectMode, "point-red-yellow")) {
            newEffectMode = EFFECT_RED_YELLOW;
        } else if (!strcmp(strNewEffectMode, "vintage-cold")) {
            newEffectMode = EFFECT_COLD_VINTAGE;
        } else if (!strcmp(strNewEffectMode, "beauty" )) {
            newEffectMode = EFFECT_BEAUTY_FACE;
        } else {
            CLOGE("ERR(%s):Invalid effect(%s)", __FUNCTION__, strNewEffectMode);
            return BAD_VALUE;
        }

        if (!isSupportedColorEffects(newEffectMode)) {
            CLOGE("ERR(%s[%d]): Effect mode(%s) is not supported!", __FUNCTION__, __LINE__, strNewEffectMode);
            return BAD_VALUE;
        }

        curEffectMode = getColorEffectMode();

        if (curEffectMode != newEffectMode) {
            m_setColorEffectMode(newEffectMode);
            m_params.set(CameraParameters::KEY_EFFECT, strNewEffectMode);

            m_frameSkipCounter.setCount(EFFECT_SKIP_FRAME);
        }
        return NO_ERROR;
    }
}

void ExynosCamera1Parameters::m_setColorEffectMode(int effect)
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
        CLOGE("ERR(%s[%d]):Color Effect mode(%d) is not supported", __FUNCTION__, __LINE__, effect);
        break;
    }
    setMetaCtlAaEffect(&m_metadata, newEffect);
}

int ExynosCamera1Parameters::getColorEffectMode(void)
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
        CLOGE("ERR(%s[%d]):Color Effect mode(%d) is invalid value", __FUNCTION__, __LINE__, curEffect);
        break;
    }

    return effect;
}

int ExynosCamera1Parameters::getSupportedColorEffects(void)
{
    return m_staticInfo->effectList;
}

bool ExynosCamera1Parameters::isSupportedColorEffects(int effectMode)
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

status_t ExynosCamera1Parameters::checkGpsAltitude(const CameraParameters& params)
{
    double newAltitude = 0;
    double curAltitude = 0;
    const char *strNewGpsAltitude = params.get(CameraParameters::KEY_GPS_ALTITUDE);

    if (strNewGpsAltitude == NULL) {
        m_params.remove(CameraParameters::KEY_GPS_ALTITUDE);
        m_setGpsAltitude(0);
        return NO_ERROR;
    }

    CLOGV("DEBUG(%s):strNewGpsAltitude %s", "setParameters", strNewGpsAltitude);

    newAltitude = atof(strNewGpsAltitude);
    curAltitude = getGpsAltitude();

    if (curAltitude != newAltitude) {
        m_setGpsAltitude(newAltitude);
        m_params.set(CameraParameters::KEY_GPS_ALTITUDE, strNewGpsAltitude);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setGpsAltitude(double altitude)
{
    m_cameraInfo.gpsAltitude = altitude;
}

double ExynosCamera1Parameters::getGpsAltitude(void)
{
    return m_cameraInfo.gpsAltitude;
}

status_t ExynosCamera1Parameters::checkGpsLatitude(const CameraParameters& params)
{
    double newLatitude = 0;
    double curLatitude = 0;
    const char *strNewGpsLatitude = params.get(CameraParameters::KEY_GPS_LATITUDE);

    if (strNewGpsLatitude == NULL) {
        m_params.remove(CameraParameters::KEY_GPS_LATITUDE);
        m_setGpsLatitude(0);
        return NO_ERROR;
    }

    CLOGV("DEBUG(%s):strNewGpsLatitude %s", "setParameters", strNewGpsLatitude);

    newLatitude = atof(strNewGpsLatitude);
    curLatitude = getGpsLatitude();

    if (curLatitude != newLatitude) {
        m_setGpsLatitude(newLatitude);
        m_params.set(CameraParameters::KEY_GPS_LATITUDE, strNewGpsLatitude);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setGpsLatitude(double latitude)
{
    m_cameraInfo.gpsLatitude = latitude;
}

double ExynosCamera1Parameters::getGpsLatitude(void)
{
    return m_cameraInfo.gpsLatitude;
}

status_t ExynosCamera1Parameters::checkGpsLongitude(const CameraParameters& params)
{
    double newLongitude = 0;
    double curLongitude = 0;
    const char *strNewGpsLongitude = params.get(CameraParameters::KEY_GPS_LONGITUDE);

    if (strNewGpsLongitude == NULL) {
        m_params.remove(CameraParameters::KEY_GPS_LONGITUDE);
        m_setGpsLongitude(0);
        return NO_ERROR;
    }

    CLOGV("DEBUG(%s):strNewGpsLongitude %s", "setParameters", strNewGpsLongitude);

    newLongitude = atof(strNewGpsLongitude);
    curLongitude = getGpsLongitude();

    if (curLongitude != newLongitude) {
        m_setGpsLongitude(newLongitude);
        m_params.set(CameraParameters::KEY_GPS_LONGITUDE, strNewGpsLongitude);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setGpsLongitude(double longitude)
{
    m_cameraInfo.gpsLongitude = longitude;
}

double ExynosCamera1Parameters::getGpsLongitude(void)
{
    return m_cameraInfo.gpsLongitude;
}

status_t ExynosCamera1Parameters::checkGpsProcessingMethod(const CameraParameters& params)
{
    // gps processing method
    const char *strNewGpsProcessingMethod = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);
    const char *strCurGpsProcessingMethod = NULL;
    bool changeMethod = false;

    if (strNewGpsProcessingMethod == NULL) {
        m_params.remove(CameraParameters::KEY_GPS_PROCESSING_METHOD);
        m_setGpsProcessingMethod(NULL);
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):strNewGpsProcessingMethod %s", "setParameters", strNewGpsProcessingMethod);

    strCurGpsProcessingMethod = getGpsProcessingMethod();

    if (strCurGpsProcessingMethod != NULL) {
        int newLen = strlen(strNewGpsProcessingMethod);
        int curLen = strlen(strCurGpsProcessingMethod);

        if (newLen != curLen)
            changeMethod = true;
        else
            changeMethod = strncmp(strNewGpsProcessingMethod, strCurGpsProcessingMethod, newLen);
    }

    if (changeMethod == true) {
        m_setGpsProcessingMethod(strNewGpsProcessingMethod);
        m_params.set(CameraParameters::KEY_GPS_PROCESSING_METHOD, strNewGpsProcessingMethod);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setGpsProcessingMethod(const char *gpsProcessingMethod)
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

const char *ExynosCamera1Parameters::getGpsProcessingMethod(void)
{
    return (const char *)m_exifInfo.gps_processing_method;
}

void ExynosCamera1Parameters::m_setExifFixedAttribute(void)
{
    char property[PROPERTY_VALUE_MAX];

    memset(&m_exifInfo, 0, sizeof(m_exifInfo));

    /* 2 0th IFD TIFF Tags */
    /* 3 Maker */
    property_get("ro.product.manufacturer", property, EXIF_DEF_MAKER);
    strncpy((char *)m_exifInfo.maker, property,
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

void ExynosCamera1Parameters::setExifChangedAttribute(exif_attribute_t   *exifInfo,
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

debug_attribute_t *ExynosCamera1Parameters::getDebugAttribute(void)
{
    return &mDebugInfo;
}

status_t ExynosCamera1Parameters::getFixedExifInfo(exif_attribute_t *exifInfo)
{
    if (exifInfo == NULL) {
        CLOGE("ERR(%s[%d]): buffer is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    memcpy(exifInfo, &m_exifInfo, sizeof(exif_attribute_t));

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::checkGpsTimeStamp(const CameraParameters& params)
{
    long newGpsTimeStamp = -1;
    long curGpsTimeStamp = -1;
    const char *strNewGpsTimeStamp = params.get(CameraParameters::KEY_GPS_TIMESTAMP);

    if (strNewGpsTimeStamp == NULL) {
        m_params.remove(CameraParameters::KEY_GPS_TIMESTAMP);
        m_setGpsTimeStamp(0);
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):strNewGpsTimeStamp %s", "setParameters", strNewGpsTimeStamp);

    newGpsTimeStamp = atol(strNewGpsTimeStamp);

    curGpsTimeStamp = getGpsTimeStamp();

    if (curGpsTimeStamp != newGpsTimeStamp) {
        m_setGpsTimeStamp(newGpsTimeStamp);
        m_params.set(CameraParameters::KEY_GPS_TIMESTAMP, strNewGpsTimeStamp);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setGpsTimeStamp(long timeStamp)
{
    m_cameraInfo.gpsTimeStamp = timeStamp;
}

long ExynosCamera1Parameters::getGpsTimeStamp(void)
{
    return m_cameraInfo.gpsTimeStamp;
}

status_t ExynosCamera1Parameters::checkBrightness(const CameraParameters& params)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return NO_ERROR;
    } else {
        int maxBrightness = params.getInt("brightness-max");
        int minBrightness = params.getInt("brightness-min");
        int newBrightness = params.getInt("brightness");
        int curBrightness = -1;
        int curEffectMode = EFFECT_NONE;

        CLOGD("DEBUG(%s):newBrightness %d", "setParameters", newBrightness);

        if (newBrightness < minBrightness || newBrightness > maxBrightness) {
            CLOGE("ERR(%s): Invalid Brightness (Min: %d, Max: %d, Value: %d)", __FUNCTION__, minBrightness, maxBrightness, newBrightness);
            return BAD_VALUE;
        }

        curEffectMode = getColorEffectMode();
        if(curEffectMode == EFFECT_BEAUTY_FACE) {
            return NO_ERROR;
        }

        curBrightness = getBrightness();

        if (curBrightness != newBrightness) {
            m_setBrightness(newBrightness);
            m_params.set("brightness", newBrightness);
        }
        return NO_ERROR;
    }
}

/* F/W's middle value is 3, and step is -2, -1, 0, 1, 2 */
void ExynosCamera1Parameters::m_setBrightness(int brightness)
{
    setMetaCtlBrightness(&m_metadata, brightness + IS_BRIGHTNESS_DEFAULT + FW_CUSTOM_OFFSET);
}

int ExynosCamera1Parameters::getBrightness(void)
{
    int32_t brightness = 0;

    getMetaCtlBrightness(&m_metadata, &brightness);
    return brightness - IS_BRIGHTNESS_DEFAULT - FW_CUSTOM_OFFSET;
}

status_t ExynosCamera1Parameters::checkSaturation(const CameraParameters& params)
{
    int maxSaturation = params.getInt("saturation-max");
    int minSaturation = params.getInt("saturation-min");
    int newSaturation = params.getInt("saturation");
    int curSaturation = -1;

    CLOGD("DEBUG(%s):newSaturation %d", "setParameters", newSaturation);

    if (newSaturation < minSaturation || newSaturation > maxSaturation) {
        CLOGE("ERR(%s): Invalid Saturation (Min: %d, Max: %d, Value: %d)", __FUNCTION__, minSaturation, maxSaturation, newSaturation);
        return BAD_VALUE;
    }

    curSaturation = getSaturation();
#ifdef CAMERA_GED_FEATURE
    if(getSceneMode() == SCENE_MODE_AUTO) {
       if (curSaturation != newSaturation) {
           m_setSaturation(newSaturation);
           m_params.set("saturation", newSaturation);
       }
    } else {
        m_params.set("saturation", "auto");
    }
#else
    if (curSaturation != newSaturation) {
        m_setSaturation(newSaturation);
        m_params.set("saturation", newSaturation);
    }
#endif
    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setSaturation(int saturation)
{
    setMetaCtlSaturation(&m_metadata, saturation + IS_SATURATION_DEFAULT + FW_CUSTOM_OFFSET);
}

int ExynosCamera1Parameters::getSaturation(void)
{
    int32_t saturation = 0;

    getMetaCtlSaturation(&m_metadata, &saturation);
    return saturation - IS_SATURATION_DEFAULT - FW_CUSTOM_OFFSET;
}

status_t ExynosCamera1Parameters::checkSharpness(const CameraParameters& params)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return NO_ERROR;
    } else {
        int maxSharpness = params.getInt("sharpness-max");
        int minSharpness = params.getInt("sharpness-min");
        int newSharpness = params.getInt("sharpness");
        int curSharpness = -1;
        int curEffectMode = EFFECT_NONE;

        CLOGD("DEBUG(%s):newSharpness %d", "setParameters", newSharpness);

        if (newSharpness < minSharpness || newSharpness > maxSharpness) {
            CLOGE("ERR(%s): Invalid Sharpness (Min: %d, Max: %d, Value: %d)", __FUNCTION__, minSharpness, maxSharpness, newSharpness);
            return BAD_VALUE;
        }

        curEffectMode = getColorEffectMode();
        if(curEffectMode == EFFECT_BEAUTY_FACE) {
            return NO_ERROR;
        }
        curSharpness = getSharpness();

        if (curSharpness != newSharpness) {
            m_setSharpness(newSharpness);
            m_params.set("sharpness", newSharpness);
        }
        return NO_ERROR;
    }
}

void ExynosCamera1Parameters::m_setSharpness(int sharpness)
{
#ifdef USE_NEW_NOISE_REDUCTION_ALGORITHM
    enum processing_mode default_edge_mode = PROCESSING_MODE_FAST;
    enum processing_mode default_noise_mode = PROCESSING_MODE_FAST;
    int default_edge_strength = 5;
    int default_noise_strength = 5;
#else
    enum processing_mode default_edge_mode = PROCESSING_MODE_OFF;
    enum processing_mode default_noise_mode = PROCESSING_MODE_OFF;
    int default_edge_strength = 0;
    int default_noise_strength = 0;
#endif

    int newSharpness = sharpness + IS_SHARPNESS_DEFAULT;
    enum processing_mode edge_mode = default_edge_mode;
    enum processing_mode noise_mode = default_noise_mode;
    int edge_strength = default_edge_strength;
    int noise_strength = default_noise_strength;

    switch (newSharpness) {
    case IS_SHARPNESS_MINUS_2:
        edge_mode = default_edge_mode;
        noise_mode = default_noise_mode;
        edge_strength = default_edge_strength;
        noise_strength = 10;
        break;
    case IS_SHARPNESS_MINUS_1:
        edge_mode = default_edge_mode;
        noise_mode = default_noise_mode;
        edge_strength = default_edge_strength;
        noise_strength = (10 + default_noise_strength + 1) / 2;
        break;
    case IS_SHARPNESS_DEFAULT:
        edge_mode = default_edge_mode;
        noise_mode = default_noise_mode;
        edge_strength = default_edge_strength;
        noise_strength = default_noise_strength;
        break;
    case IS_SHARPNESS_PLUS_1:
        edge_mode = default_edge_mode;
        noise_mode = default_noise_mode;
        edge_strength = (10 + default_edge_strength + 1) / 2;
        noise_strength = default_noise_strength;
        break;
    case IS_SHARPNESS_PLUS_2:
        edge_mode = default_edge_mode;
        noise_mode = default_noise_mode;
        edge_strength = 10;
        noise_strength = default_noise_strength;
        break;
    default:
        break;
    }

    CLOGD("DEBUG(%s):newSharpness %d edge_mode(%d),st(%d), noise(%d),st(%d)",
         __FUNCTION__, newSharpness, edge_mode, edge_strength, noise_mode, noise_strength);

    setMetaCtlSharpness(&m_metadata, edge_mode, edge_strength, noise_mode, noise_strength);
}

int ExynosCamera1Parameters::getSharpness(void)
{
#ifdef USE_NEW_NOISE_REDUCTION_ALGORITHM
    enum processing_mode default_edge_mode = PROCESSING_MODE_FAST;
    enum processing_mode default_noise_mode = PROCESSING_MODE_FAST;
    int default_edge_sharpness = 5;
    int default_noise_sharpness = 5;
#else
    enum processing_mode default_edge_mode = PROCESSING_MODE_OFF;
    enum processing_mode default_noise_mode = PROCESSING_MODE_OFF;
    int default_edge_sharpness = 0;
    int default_noise_sharpness = 0;
#endif

    int32_t edge_sharpness = default_edge_sharpness;
    int32_t noise_sharpness = default_noise_sharpness;
    int32_t sharpness = 0;
    enum processing_mode edge_mode = default_edge_mode;
    enum processing_mode noise_mode = default_noise_mode;

    getMetaCtlSharpness(&m_metadata, &edge_mode, &edge_sharpness, &noise_mode, &noise_sharpness);

    if(noise_sharpness == 10 && edge_sharpness == default_edge_sharpness) {
        sharpness = IS_SHARPNESS_MINUS_2;
    } else if(noise_sharpness == (10 + default_noise_sharpness + 1) / 2
            && edge_sharpness == default_edge_sharpness) {
        sharpness = IS_SHARPNESS_MINUS_1;
    } else if(noise_sharpness == default_noise_sharpness
            && edge_sharpness == default_edge_sharpness) {
        sharpness = IS_SHARPNESS_DEFAULT;
    } else if(noise_sharpness == default_noise_sharpness
            && edge_sharpness == (10 + default_edge_sharpness + 1) / 2) {
        sharpness = IS_SHARPNESS_PLUS_1;
    } else if(noise_sharpness == default_noise_sharpness
            && edge_sharpness == 10) {
        sharpness = IS_SHARPNESS_PLUS_2;
    } else {
        sharpness = IS_SHARPNESS_DEFAULT;
    }

    return sharpness - IS_SHARPNESS_DEFAULT;
}

status_t ExynosCamera1Parameters::checkHue(const CameraParameters& params)
{
    int maxHue = params.getInt("hue-max");
    int minHue = params.getInt("hue-min");
    int newHue = params.getInt("hue");
    int curHue = -1;

    CLOGD("DEBUG(%s):newHue %d", "setParameters", newHue);

    if (newHue < minHue || newHue > maxHue) {
        CLOGE("ERR(%s): Invalid Hue (Min: %d, Max: %d, Value: %d)", __FUNCTION__, minHue, maxHue, newHue);
        return BAD_VALUE;
    }

    curHue = getHue();

    if (curHue != newHue) {
        m_setHue(newHue);
        m_params.set("hue", newHue);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setHue(int hue)
{
    setMetaCtlHue(&m_metadata, hue + IS_HUE_DEFAULT + FW_CUSTOM_OFFSET);
}

int ExynosCamera1Parameters::getHue(void)
{
    int32_t hue = 0;

    getMetaCtlHue(&m_metadata, &hue);
    return hue - IS_HUE_DEFAULT - FW_CUSTOM_OFFSET;
}

status_t ExynosCamera1Parameters::checkIso(const CameraParameters& params)
{
    uint32_t newISO = 0;
    uint32_t curISO = 0;
    const char *strNewISO = params.get("iso");

    if (strNewISO == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):strNewISO %s", "setParameters", strNewISO);

    if (!strcmp(strNewISO, "auto")) {
        newISO = 0;
    } else {
        newISO = (uint32_t)atoi(strNewISO);
        if (newISO == 0) {
            CLOGE("ERR(%s):Invalid iso value(%s)", __FUNCTION__, strNewISO);
            return BAD_VALUE;
        }
    }

    curISO = getIso();
#ifdef CAMERA_GED_FEATURE
    if(getSceneMode() == SCENE_MODE_AUTO) {
        if (curISO != newISO) {
            m_setIso(newISO);
            m_params.set("iso", strNewISO);
        }
    } else {
        m_params.set("iso", "auto");
    }
#else
    if (curISO != newISO) {
        m_setIso(newISO);
        m_params.set("iso", strNewISO);
    }
#endif
    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setIso(uint32_t iso)
{
    enum aa_isomode mode = AA_ISOMODE_AUTO;

    if (iso == 0 )
        mode = AA_ISOMODE_AUTO;
    else
        mode = AA_ISOMODE_MANUAL;

    setMetaCtlIso(&m_metadata, mode, iso);
}

uint32_t ExynosCamera1Parameters::getIso(void)
{
    enum aa_isomode mode = AA_ISOMODE_AUTO;
    uint32_t iso = 0;

    getMetaCtlIso(&m_metadata, &mode, &iso);

    return iso;
}

uint64_t ExynosCamera1Parameters::getCaptureExposureTime(void)
{
    return m_exposureTimeCapture;
}

status_t ExynosCamera1Parameters::checkContrast(const CameraParameters& params)
{
    uint32_t newContrast = 0;
    uint32_t curContrast = 0;
    const char *strNewContrast = params.get("contrast");

    if (strNewContrast == NULL) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):strNewContrast %s", "setParameters", strNewContrast);

    if (!strcmp(strNewContrast, "auto"))
        newContrast = IS_CONTRAST_DEFAULT;
    else if (!strcmp(strNewContrast, "-2"))
        newContrast = IS_CONTRAST_MINUS_2;
    else if (!strcmp(strNewContrast, "-1"))
        newContrast = IS_CONTRAST_MINUS_1;
    else if (!strcmp(strNewContrast, "0"))
        newContrast = IS_CONTRAST_DEFAULT;
    else if (!strcmp(strNewContrast, "1"))
        newContrast = IS_CONTRAST_PLUS_1;
    else if (!strcmp(strNewContrast, "2"))
        newContrast = IS_CONTRAST_PLUS_2;
    else {
        CLOGE("ERR(%s):Invalid contrast value(%s)", __FUNCTION__, strNewContrast);
        return BAD_VALUE;
    }

    curContrast = getContrast();

    if (curContrast != newContrast) {
        m_setContrast(newContrast);
        m_params.set("contrast", strNewContrast);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setContrast(uint32_t contrast)
{
    setMetaCtlContrast(&m_metadata, contrast);
}

uint32_t ExynosCamera1Parameters::getContrast(void)
{
    uint32_t contrast = 0;
    getMetaCtlContrast(&m_metadata, &contrast);
    return contrast;
}

status_t ExynosCamera1Parameters::checkHdrMode(const CameraParameters& params)
{
    int newHDR = params.getInt("hdr-mode");
    bool curHDR = -1;

    if (newHDR < 0) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):newHDR %d", "setParameters", newHDR);

    curHDR = getHdrMode();

    if (curHDR != (bool)newHDR) {
        m_setHdrMode((bool)newHDR);
        m_params.set("hdr-mode", newHDR);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setHdrMode(bool hdr)
{
    m_cameraInfo.hdrMode = hdr;

#ifdef CAMERA_GED_FEATURE
    if (hdr == true)
        m_setShotMode(SHOT_MODE_RICH_TONE);
    else
        m_setShotMode(SHOT_MODE_NORMAL);
#endif

    m_activityControl->setHdrMode(hdr);
}

bool ExynosCamera1Parameters::getHdrMode(void)
{
    return m_cameraInfo.hdrMode;
}

int ExynosCamera1Parameters::getPreviewBufferCount(void)
{
    CLOGV("DEBUG(%s):getPreviewBufferCount %d", "setParameters", m_previewBufferCount);

    return m_previewBufferCount;
}

void ExynosCamera1Parameters::setPreviewBufferCount(int previewBufferCount)
{
    m_previewBufferCount = previewBufferCount;

    CLOGV("DEBUG(%s):setPreviewBufferCount %d", "setParameters", m_previewBufferCount);

    return;
}

status_t ExynosCamera1Parameters::checkAntiShake(const CameraParameters& params)
{
    int newAntiShake = params.getInt("anti-shake");
    bool curAntiShake = false;
    bool toggle = false;
    int curShotMode = getShotMode();

    if (curShotMode != SHOT_MODE_AUTO)
        return NO_ERROR;

    if (newAntiShake < 0) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):newAntiShake %d", "setParameters", newAntiShake);

    if (newAntiShake == 1)
        toggle = true;

    curAntiShake = getAntiShake();

    if (curAntiShake != toggle) {
        m_setAntiShake(toggle);
        m_params.set("anti-shake", newAntiShake);
    }

    return NO_ERROR;
}

void ExynosCamera1Parameters::m_setAntiShake(bool toggle)
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

bool ExynosCamera1Parameters::getAntiShake(void)
{
    return m_cameraInfo.antiShake;
}

status_t ExynosCamera1Parameters::checkScalableSensorMode(const CameraParameters& params)
{
    bool needScaleMode = false;
    bool curScaleMode = false;
    int newScaleMode = params.getInt("scale_mode");

    if (newScaleMode < 0) {
        return NO_ERROR;
    }

    CLOGD("DEBUG(%s):newScaleMode %d", "setParameters", newScaleMode);

    if (isScalableSensorSupported() == true) {
        needScaleMode = m_adjustScalableSensorMode(newScaleMode);
        curScaleMode = getScalableSensorMode();

        if (curScaleMode != needScaleMode) {
            setScalableSensorMode(needScaleMode);
            m_params.set("scale_mode", newScaleMode);
        }

//        updateHwSensorSize();
    }

    return NO_ERROR;
}

bool ExynosCamera1Parameters::isScalableSensorSupported(void)
{
    return m_staticInfo->scalableSensorSupport;
}

bool ExynosCamera1Parameters::m_adjustScalableSensorMode(const int scaleMode)
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
        CLOGW("WARN(%s):pictureRatio(%f, %d, %d) fps(%d, %d) is not proper for scalable",
                __FUNCTION__, pictureRatio, pictureW, pictureH, minFps, maxFps);
    }

    return adjustedScaleMode;
}

void ExynosCamera1Parameters::setScalableSensorMode(bool scaleMode)
{
    m_cameraInfo.scalableSensorMode = scaleMode;
}

bool ExynosCamera1Parameters::getScalableSensorMode(void)
{
    return m_cameraInfo.scalableSensorMode;
}

void ExynosCamera1Parameters::m_getScalableSensorSize(int *newSensorW, int *newSensorH)
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

bool ExynosCamera1Parameters::getZoomSupported(void)
{
    return m_staticInfo->zoomSupport;
}

bool ExynosCamera1Parameters::getSmoothZoomSupported(void)
{
    return m_staticInfo->smoothZoomSupport;
}

void ExynosCamera1Parameters::checkHorizontalViewAngle(void)
{
    m_params.setFloat(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, getHorizontalViewAngle());
}

void ExynosCamera1Parameters::setHorizontalViewAngle(int pictureW, int pictureH)
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

float ExynosCamera1Parameters::getHorizontalViewAngle(void)
{
    int right_ratio = 177;

    if ((int)(m_staticInfo->maxSensorW * 100 / m_staticInfo->maxSensorH) == right_ratio) {
        return m_calculatedHorizontalViewAngle;
    } else {
        return m_staticInfo->horizontalViewAngle[m_cameraInfo.pictureSizeRatioId];
    }
}

float ExynosCamera1Parameters::getVerticalViewAngle(void)
{
    return m_staticInfo->verticalViewAngle;
}

void ExynosCamera1Parameters::getFnumber(int *num, int *den)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        *num = m_staticInfo->fNumber * COMMON_DENOMINATOR;
        *den = COMMON_DENOMINATOR;
    } else {
        *num = m_staticInfo->fNumberNum;
        *den = m_staticInfo->fNumberDen;
    }
}

void ExynosCamera1Parameters::getApertureValue(int *num, int *den)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        *num = m_staticInfo->aperture * COMMON_DENOMINATOR;
        *den = COMMON_DENOMINATOR;
    } else {
        *num = m_staticInfo->apertureNum;
        *den = m_staticInfo->apertureDen;
    }
}

int ExynosCamera1Parameters::getFocalLengthIn35mmFilm(void)
{
    return m_staticInfo->focalLengthIn35mmLength;
}

void ExynosCamera1Parameters::getFocalLength(int *num, int *den)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        *num = m_staticInfo->focalLength * COMMON_DENOMINATOR;
        *den = COMMON_DENOMINATOR;
    } else {
        *num = m_staticInfo->focalLengthNum;
        *den = m_staticInfo->focalLengthDen;
    }
}

void ExynosCamera1Parameters::getFocusDistances(int *num, int *den)
{
    *num = m_staticInfo->focusDistanceNum;
    *den = m_staticInfo->focusDistanceDen;
}

int ExynosCamera1Parameters::getMinExposureCompensation(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return m_staticInfo->exposureCompensationRange[MIN];
    } else {
        return m_staticInfo->minExposureCompensation;
    }
}

int ExynosCamera1Parameters::getMaxExposureCompensation(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return m_staticInfo->exposureCompensationRange[MAX];
    } else {
        return m_staticInfo->maxExposureCompensation;
    }
}

float ExynosCamera1Parameters::getExposureCompensationStep(void)
{
    return m_staticInfo->exposureCompensationStep;
}

int ExynosCamera1Parameters::getMaxNumDetectedFaces(void)
{
    return m_staticInfo->maxNumDetectedFaces;
}

uint32_t ExynosCamera1Parameters::getMaxNumFocusAreas(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return m_staticInfo->max3aRegions[AF];
    } else {
        return m_staticInfo->maxNumFocusAreas;
    }
}

uint32_t ExynosCamera1Parameters::getMaxNumMeteringAreas(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return m_staticInfo->max3aRegions[AE];
    } else {
        return m_staticInfo->maxNumMeteringAreas;
    }
}

int ExynosCamera1Parameters::getMaxZoomLevel(void)
{
    int zoomLevel = 0;
    int samsungCamera = getSamsungCamera();

    if (samsungCamera || m_cameraId == CAMERA_ID_FRONT) {
        zoomLevel = m_staticInfo->maxZoomLevel;
    } else {
        zoomLevel = m_staticInfo->maxBasicZoomLevel;
    }
    return zoomLevel;
}

float ExynosCamera1Parameters::getMaxZoomRatio(void)
{
    return (float)m_staticInfo->maxZoomRatio;
}

float ExynosCamera1Parameters::getZoomRatio(int zoomLevel)
{
    float zoomRatio = 1.00f;
    if (getZoomSupported() == true)
        zoomRatio = (float)m_staticInfo->zoomRatioList[zoomLevel];
    else
        zoomRatio = 1000.00f;

    return zoomRatio;
}

bool ExynosCamera1Parameters::getVideoSnapshotSupported(void)
{
    return m_staticInfo->videoSnapshotSupport;
}

bool ExynosCamera1Parameters::getVideoStabilizationSupported(void)
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

bool ExynosCamera1Parameters::getAutoWhiteBalanceLockSupported(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return true;
    } else {
        return m_staticInfo->autoWhiteBalanceLockSupport;
    }
}

bool ExynosCamera1Parameters::getAutoExposureLockSupported(void)
{
    if (getHalVersion() == IS_HAL_VER_3_2) {
        return true;
    } else {
        return m_staticInfo->autoExposureLockSupport;
    }
}

void ExynosCamera1Parameters::enableMsgType(int32_t msgType)
{
    Mutex::Autolock lock(m_msgLock);
    m_enabledMsgType |= msgType;
}

void ExynosCamera1Parameters::disableMsgType(int32_t msgType)
{
    Mutex::Autolock lock(m_msgLock);
    m_enabledMsgType &= ~msgType;
}

bool ExynosCamera1Parameters::msgTypeEnabled(int32_t msgType)
{
    Mutex::Autolock lock(m_msgLock);
    return (m_enabledMsgType & msgType);
}

status_t ExynosCamera1Parameters::setFrameSkipCount(int count)
{
    m_frameSkipCounter.setCount(count);

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::getFrameSkipCount(int *count)
{
    *count = m_frameSkipCounter.getCount();
    m_frameSkipCounter.decCount();

    return NO_ERROR;
}

int ExynosCamera1Parameters::getFrameSkipCount(void)
{
    return m_frameSkipCounter.getCount();
}

void ExynosCamera1Parameters::setIsFirstStartFlag(bool flag)
{
    m_flagFirstStart = flag;
}

int ExynosCamera1Parameters::getIsFirstStartFlag(void)
{
    return m_flagFirstStart;
}

ExynosCameraActivityControl *ExynosCamera1Parameters::getActivityControl(void)
{
    return m_activityControl;
}

status_t ExynosCamera1Parameters::setAutoFocusMacroPosition(int autoFocusMacroPosition)
{
    int oldAutoFocusMacroPosition = m_cameraInfo.autoFocusMacroPosition;
    m_cameraInfo.autoFocusMacroPosition = autoFocusMacroPosition;

    m_activityControl->setAutoFocusMacroPosition(oldAutoFocusMacroPosition, autoFocusMacroPosition);

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::setDisEnable(bool enable)
{
    setMetaBypassDis(&m_metadata, enable == true ? 0 : 1);
    return NO_ERROR;
}

bool ExynosCamera1Parameters::getDisEnable(void)
{
    return m_metadata.dis_bypass;
}

status_t ExynosCamera1Parameters::setDrcEnable(bool enable)
{
    setMetaBypassDrc(&m_metadata, enable == true ? 0 : 1);
    return NO_ERROR;
}

bool ExynosCamera1Parameters::getDrcEnable(void)
{
    return m_metadata.drc_bypass;
}

status_t ExynosCamera1Parameters::setDnrEnable(bool enable)
{
    setMetaBypassDnr(&m_metadata, enable == true ? 0 : 1);
    return NO_ERROR;
}

bool ExynosCamera1Parameters::getDnrEnable(void)
{
    return m_metadata.dnr_bypass;
}

status_t ExynosCamera1Parameters::setFdEnable(bool enable)
{
    setMetaBypassFd(&m_metadata, enable == true ? 0 : 1);
    return NO_ERROR;
}

bool ExynosCamera1Parameters::getFdEnable(void)
{
    return m_metadata.fd_bypass;
}

status_t ExynosCamera1Parameters::setFdMode(enum facedetect_mode mode)
{
    setMetaCtlFdMode(&m_metadata, mode);
    return NO_ERROR;
}

status_t ExynosCamera1Parameters::getFdMeta(bool reprocessing, void *buf)
{
    if (buf == NULL) {
        CLOGE("ERR: buf is NULL");
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

void ExynosCamera1Parameters::setFlipHorizontal(int val)
{
    if (val < 0) {
        CLOGE("ERR(%s[%d]): setFlipHorizontal ignored, invalid value(%d)",
                __FUNCTION__, __LINE__, val);
        return;
    }

    m_cameraInfo.flipHorizontal = val;
}

int ExynosCamera1Parameters::getFlipHorizontal(void)
{
#if defined(USE_CAPTURE_FRAME_FRONT_WIDESELFIE)
    return m_cameraInfo.flipHorizontal;
#else
    if (m_cameraInfo.shotMode == SHOT_MODE_FRONT_PANORAMA) {
        return 0;
    } else {
        return m_cameraInfo.flipHorizontal;
    }
#endif
}

void ExynosCamera1Parameters::setFlipVertical(int val)
{
    if (val < 0) {
        CLOGE("ERR(%s[%d]): setFlipVertical ignored, invalid value(%d)",
                __FUNCTION__, __LINE__, val);
        return;
    }

    m_cameraInfo.flipVertical = val;
}

int ExynosCamera1Parameters::getFlipVertical(void)
{
    return m_cameraInfo.flipVertical;
}

bool ExynosCamera1Parameters::getCallbackNeedCSC(void)
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

bool ExynosCamera1Parameters::getCallbackNeedCopy2Rendering(void)
{
    bool ret = false;
    int previewW = 0, previewH = 0;

    getPreviewSize(&previewW, &previewH);
    if (previewW * previewH <= 1920*1080) {
        ret = true;
    }
    return ret;
}

#ifdef RAWDUMP_CAPTURE
void ExynosCamera1Parameters::setRawCaptureModeOn(bool enable)
{
    m_flagRawCaptureOn = enable;
}

bool ExynosCamera1Parameters::getRawCaptureModeOn(void)
{
    return m_flagRawCaptureOn;
}
#endif

bool ExynosCamera1Parameters::setDeviceOrientation(int orientation)
{
    if (orientation < 0 || orientation % 90 != 0) {
        CLOGE("ERR(%s[%d]):Invalid orientation (%d)",
                __FUNCTION__, __LINE__, orientation);
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

    CLOGD("DEBUG(%s[%d]):orientation(%d), hwRotation(%d), fdOrientation(%d)",
                __FUNCTION__, __LINE__, orientation, hwRotation, fdOrientation);

    return true;
}

int ExynosCamera1Parameters::getDeviceOrientation(void)
{
    return m_cameraInfo.deviceOrientation;
}

int ExynosCamera1Parameters::getFdOrientation(void)
{
    /* HACK: Calibrate FRONT FD orientation */
    if (getCameraId() == CAMERA_ID_FRONT)
        return (m_cameraInfo.deviceOrientation + FRONT_ROTATION + 180) % 360;
    else
        return (m_cameraInfo.deviceOrientation + BACK_ROTATION) % 360;
}

void ExynosCamera1Parameters::getSetfileYuvRange(bool flagReprocessing, int *setfile, int *yuvRange)
{
    if (flagReprocessing == true) {
        *setfile = m_setfileReprocessing;
        *yuvRange = m_yuvRangeReprocessing;
    } else {
        *setfile = m_setfile;
        *yuvRange = m_yuvRange;
    }
}

status_t ExynosCamera1Parameters::checkSetfileYuvRange(void)
{
    int oldSetFile = m_setfile;
    int oldYUVRange = m_yuvRange;

    /* general */
    m_getSetfileYuvRange(false, &m_setfile, &m_yuvRange);

    /* reprocessing */
    m_getSetfileYuvRange(true, &m_setfileReprocessing, &m_yuvRangeReprocessing);

    CLOGD("DEBUG(%s[%d]):m_cameraId(%d) : general[setfile(%d) YUV range(%d)] : reprocesing[setfile(%d) YUV range(%d)]",
        __FUNCTION__, __LINE__,
        m_cameraId,
        m_setfile, m_yuvRange,
        m_setfileReprocessing, m_yuvRangeReprocessing);

    return NO_ERROR;
}

void ExynosCamera1Parameters::setSetfileYuvRange(void)
{
    /* reprocessing */
    m_getSetfileYuvRange(true, &m_setfileReprocessing, &m_yuvRangeReprocessing);

    ALOGD("DEBUG(%s[%d]):m_cameraId(%d) : general[setfile(%d) YUV range(%d)] : reprocesing[setfile(%d) YUV range(%d)]",
        __FUNCTION__, __LINE__,
        m_cameraId,
        m_setfile, m_yuvRange,
        m_setfileReprocessing, m_yuvRangeReprocessing);

}

void ExynosCamera1Parameters::setUseDynamicBayer(bool enable)
{
    m_useDynamicBayer = enable;
}

bool ExynosCamera1Parameters::getUseDynamicBayer(void)
{
    return m_useDynamicBayer;
}

void ExynosCamera1Parameters::setUseDynamicBayerVideoSnapShot(bool enable)
{
    m_useDynamicBayerVideoSnapShot = enable;
}

bool ExynosCamera1Parameters::getUseDynamicBayerVideoSnapShot(void)
{
    return m_useDynamicBayerVideoSnapShot;
}

void ExynosCamera1Parameters::setUseDynamicScc(bool enable)
{
    m_useDynamicScc = enable;
}

bool ExynosCamera1Parameters::getUseDynamicScc(void)
{
    bool dynamicScc = m_useDynamicScc;
    bool reprocessing = isReprocessing();

    if (getRecordingHint() == true && reprocessing == false)
        dynamicScc = false;

    return dynamicScc;
}

status_t ExynosCamera1Parameters::calcHighResolutionPreviewGSCRect(ExynosRect *srcRect, ExynosRect *dstRect)
{
    int ret = 0;

    int cropX = 0, cropY = 0;
    int cropW = 0, cropH = 0;
    int crop_crop_x = 0, crop_crop_y = 0;
    int crop_crop_w = 0, crop_crop_h = 0;

    int previewW = 0, previewH = 0, previewFormat = 0;
    int pictureW = 0, pictureH = 0, pictureFormat = 0;
    previewFormat = getPreviewFormat();
    pictureFormat = getHwPictureFormat();

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

status_t ExynosCamera1Parameters::calcPictureRect(ExynosRect *srcRect, ExynosRect *dstRect)
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
    pictureFormat = getHwPictureFormat();
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

status_t ExynosCamera1Parameters::calcPictureRect(int originW, int originH, ExynosRect *srcRect, ExynosRect *dstRect)
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
    pictureFormat = getHwPictureFormat();
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
    CLOGD("DEBUG(%s):originSize (%dx%d) pictureSize (%dx%d)",
            __FUNCTION__, originW, originH, pictureW, pictureH);
    CLOGD("DEBUG(%s):size2 cropX = %d, cropY = %d, cropW = %d, cropH = %d, zoom = %d",
            __FUNCTION__, crop_crop_x, crop_crop_y, crop_crop_w, crop_crop_h, zoom);
    CLOGD("DEBUG(%s):size pictureFormat = 0x%x, JPEG_INPUT_COLOR_FMT = 0x%x",
            __FUNCTION__, pictureFormat, JPEG_INPUT_COLOR_FMT);
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

status_t ExynosCamera1Parameters::getPreviewBayerCropSize(ExynosRect *srcRect, ExynosRect *dstRect)
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
            ALOGV("WARN(%s):preview ratioId(%d) != videoRatioId(%d), use previewRatioId",
                __FUNCTION__, m_cameraInfo.previewSizeRatioId, m_cameraInfo.videoSizeRatioId);
        }
    }

    int curBnsW = 0, curBnsH = 0;
    getBnsSize(&curBnsW, &curBnsH);
    if (SIZE_RATIO(curBnsW, curBnsH) != SIZE_RATIO(hwBnsW, hwBnsH))
        ALOGW("ERROR(%s[%d]): current BNS size(%dx%d) is NOT same with Hw BNS size(%dx%d)",
                __FUNCTION__, __LINE__, curBnsW, curBnsH, hwBnsW, hwBnsH);

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
        ALOGV("DEBUG(%s[%d]):hwBnsSize (%dx%d), hwBcropSize(%dx%d), fastFpsMode(%d)",
                __FUNCTION__, __LINE__,
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
    ALOGD("DEBUG(%s):zoomLevel=%d", __FUNCTION__, zoomLevel);
    ALOGD("DEBUG(%s):hwBnsSize (%dx%d), hwBcropSize (%d, %d)(%dx%d)",
            __FUNCTION__, srcRect->w, srcRect->h, dstRect->x, dstRect->y, dstRect->w, dstRect->h);
#endif

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::getPreviewBdsSize(ExynosRect *dstRect)
{
    status_t ret = NO_ERROR;

    ret = m_getPreviewBdsSize(dstRect);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):calcPreviewBDSSize() fail", __FUNCTION__, __LINE__);
        return ret;
    }

    if (this->getHWVdisMode() == true) {
        int disW = ALIGN_UP((int)(dstRect->w * HW_VDIS_W_RATIO), 2);
        int disH = ALIGN_UP((int)(dstRect->h * HW_VDIS_H_RATIO), 2);

        CLOGV("DEBUG(%s[%d]):HWVdis adjusted BDS Size (%d x %d) -> (%d x %d)",
            __FUNCTION__, __LINE__, dstRect->w, dstRect->h, disW, disH);

        /*
         * check H/W VDIS size(BDS dst size) is too big than bayerCropSize(BDS out size).
         */
        ExynosRect bnsSize, bayerCropSize;

        if (getPreviewBayerCropSize(&bnsSize, &bayerCropSize) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):getPreviewBayerCropSize() fail", __FUNCTION__, __LINE__);
        } else {
            if (bayerCropSize.w < disW || bayerCropSize.h < disH) {
                CLOGV("DEBUG(%s[%d]):bayerCropSize (%d x %d) is smaller than (%d x %d). so force bayerCropSize",
                    __FUNCTION__, __LINE__, bayerCropSize.w, bayerCropSize.h, disW, disH);

                disW = bayerCropSize.w;
                disH = bayerCropSize.h;
            }
        }

        dstRect->w = disW;
        dstRect->h = disH;
    }

#ifdef DEBUG_PERFRAME
    CLOGD("DEBUG(%s):hwBdsSize (%dx%d)", __FUNCTION__, dstRect->w, dstRect->h);
#endif

    return ret;
}

status_t ExynosCamera1Parameters::getPictureBdsSize(ExynosRect *dstRect)
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

status_t ExynosCamera1Parameters::calcNormalToTpuSize(int srcW, int srcH, int *dstW, int *dstH)
{
    status_t ret = NO_ERROR;
    if (srcW < 0 || srcH < 0) {
        CLOGE("ERR(%s[%d]):src size is invalid(%d x %d)", __FUNCTION__, __LINE__, srcW, srcH);
        return INVALID_OPERATION;
    }

    int disW = ALIGN_UP((int)(srcW * HW_VDIS_W_RATIO), 2);
    int disH = ALIGN_UP((int)(srcH * HW_VDIS_H_RATIO), 2);

    *dstW = disW;
    *dstH = disH;
    CLOGD("DEBUG(%s[%d]):HWVdis adjusted BDS Size (%d x %d) -> (%d x %d)", __FUNCTION__, __LINE__, srcW, srcH, disW, disH);

    return NO_ERROR;
}

status_t ExynosCamera1Parameters::calcTpuToNormalSize(int srcW, int srcH, int *dstW, int *dstH)
{
    status_t ret = NO_ERROR;
    if (srcW < 0 || srcH < 0) {
        CLOGE("ERR(%s[%d]):src size is invalid(%d x %d)", __FUNCTION__, __LINE__, srcW, srcH);
        return INVALID_OPERATION;
    }

    int disW = ALIGN_DOWN((int)(srcW / HW_VDIS_W_RATIO), 2);
    int disH = ALIGN_DOWN((int)(srcH / HW_VDIS_H_RATIO), 2);

    *dstW = disW;
    *dstH = disH;
    CLOGD("DEBUG(%s[%d]):HWVdis adjusted BDS Size (%d x %d) -> (%d x %d)", __FUNCTION__, __LINE__, srcW, srcH, disW, disH);

    return ret;
}

void ExynosCamera1Parameters::setUsePureBayerReprocessing(bool enable)
{
    m_usePureBayerReprocessing = enable;
}

bool ExynosCamera1Parameters::getUsePureBayerReprocessing(void)
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
        CLOGD("DEBUG(%s[%d]):bayer usage is changed (%d -> %d)", __FUNCTION__, __LINE__, oldMode, m_usePureBayerReprocessing);
    }

    return m_usePureBayerReprocessing;
}

bool ExynosCamera1Parameters::isUseYuvReprocessing(void)
{
    bool ret = false;

#ifdef USE_YUV_REPROCESSING
    ret = USE_YUV_REPROCESSING;
#endif

    return ret;
}

bool ExynosCamera1Parameters::isUseYuvReprocessingForThumbnail(void)
{
    bool ret = false;

#ifdef USE_YUV_REPROCESSING_FOR_THUMBNAIL
    if (isUseYuvReprocessing() == true)
        ret = USE_YUV_REPROCESSING_FOR_THUMBNAIL;
#endif

    return ret;
}

int32_t ExynosCamera1Parameters::getReprocessingBayerMode(void)
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

int ExynosCamera1Parameters::getBayerFormat(int pipeId)
{
    int bayerFormat = V4L2_PIX_FMT_SBGGR16;

    switch (pipeId) {
    case PIPE_FLITE:
    case PIPE_3AA:
    case PIPE_FLITE_REPROCESSING:
    case PIPE_3AA_REPROCESSING:
        bayerFormat = CAMERA_BAYER_FORMAT;
        break;
    case PIPE_3AC:
    case PIPE_3AP:
    case PIPE_ISP:
    case PIPE_3AC_REPROCESSING:
    case PIPE_3AP_REPROCESSING:
    case PIPE_ISP_REPROCESSING:
    default:
        CLOGW("WRN(%s[%d]):Invalid pipeId(%d)", __FUNCTION__, __LINE__, pipeId);
        break;
    }

    return bayerFormat;
}

void ExynosCamera1Parameters::setAdaptiveCSCRecording(bool enable)
{
    m_useAdaptiveCSCRecording = enable;
}

bool ExynosCamera1Parameters::getAdaptiveCSCRecording(void)
{
    return m_useAdaptiveCSCRecording;
}

int ExynosCamera1Parameters::getHalPixelFormat(void)
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
int ExynosCamera1Parameters::convertingHalPreviewFormat(int previewFormat, int yuvRange)
{
    int halFormat = 0;

    switch (previewFormat) {
    case V4L2_PIX_FMT_NV21:
        CLOGD("DEBUG(%s[%d]): preview format NV21", __FUNCTION__, __LINE__);
        halFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        break;
    case V4L2_PIX_FMT_NV21M:
        CLOGD("DEBUG(%s[%d]): preview format NV21M", __FUNCTION__, __LINE__);
        if (yuvRange == YUV_FULL_RANGE) {
            halFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL;
        } else if (yuvRange == YUV_LIMITED_RANGE) {
            halFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M;
        } else {
            CLOGW("WRN(%s[%d]): invalid yuvRange, force set to full range", __FUNCTION__, __LINE__);
            halFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL;
        }
        break;
    case V4L2_PIX_FMT_YVU420:
        CLOGD("DEBUG(%s[%d]): preview format YVU420", __FUNCTION__, __LINE__);
        halFormat = HAL_PIXEL_FORMAT_YV12;
        break;
    case V4L2_PIX_FMT_YVU420M:
        CLOGD("DEBUG(%s[%d]): preview format YVU420M", __FUNCTION__, __LINE__);
        halFormat = HAL_PIXEL_FORMAT_EXYNOS_YV12_M;
        break;
    default:
        CLOGE("ERR(%s[%d]): unknown preview format(%d)", __FUNCTION__, __LINE__, previewFormat);
        break;
    }

    return halFormat;
}
#else
int ExynosCamera1Parameters::convertingHalPreviewFormat(int previewFormat, int yuvRange)
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
            CLOGW("WRN(%s[%d]): invalid yuvRange, force set to full range", __FUNCTION__, __LINE__);
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
        CLOGE("ERR(%s[%d]): unknown preview format(%d)", __FUNCTION__, __LINE__, previewFormat);
        break;
    }

    return halFormat;
}
#endif

void ExynosCamera1Parameters::setDvfsLock(bool lock) {
    m_dvfsLock = lock;
}

bool ExynosCamera1Parameters::getDvfsLock(void) {
    return m_dvfsLock;
}
bool ExynosCamera1Parameters::setConfig(struct ExynosConfigInfo* config)
{
    memcpy(m_exynosconfig, config, sizeof(struct ExynosConfigInfo));
    setConfigMode(m_exynosconfig->mode);
    return true;
}
struct ExynosConfigInfo* ExynosCamera1Parameters::getConfig()
{
    return m_exynosconfig;
}

bool ExynosCamera1Parameters::setConfigMode(uint32_t mode)
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
        CLOGE("ERR(%s[%d]): unknown config mode (%d)", __FUNCTION__, __LINE__, mode);
    }
    return ret;
}

int ExynosCamera1Parameters::getConfigMode()
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
        CLOGE("ERR(%s[%d]): unknown config mode (%d)", __FUNCTION__, __LINE__, m_exynosconfig->mode);
    }

    return ret;
}

void ExynosCamera1Parameters::setZoomActiveOn(bool enable)
{
    m_zoom_activated = enable;
}

bool ExynosCamera1Parameters::getZoomActiveOn(void)
{
    return m_zoom_activated;
}

status_t ExynosCamera1Parameters::setMarkingOfExifFlash(int flag)
{
    m_firing_flash_marking = flag;

    return NO_ERROR;
}

int ExynosCamera1Parameters::getMarkingOfExifFlash(void)
{
    return m_firing_flash_marking;
}

bool ExynosCamera1Parameters::getSensorOTFSupported(void)
{
    return m_staticInfo->flite3aaOtfSupport;
}

bool ExynosCamera1Parameters::isReprocessing(void)
{
    bool reprocessing = false;
    int cameraId = getCameraId();
    bool flagDual = getDualMode();

    if (cameraId == CAMERA_ID_BACK) {
#if defined(MAIN_CAMERA_DUAL_REPROCESSING) && defined(MAIN_CAMERA_SINGLE_REPROCESSING)
        reprocessing = (flagDual == true) ? MAIN_CAMERA_DUAL_REPROCESSING : MAIN_CAMERA_SINGLE_REPROCESSING;
#else
        ALOGW("WRN(%s[%d]): MAIN_CAMERA_DUAL_REPROCESSING/MAIN_CAMERA_SINGLE_REPROCESSING is not defined", __FUNCTION__, __LINE__);
#endif
    } else {
#if defined(FRONT_CAMERA_DUAL_REPROCESSING) && defined(FRONT_CAMERA_SINGLE_REPROCESSING)
        reprocessing = (flagDual == true) ? FRONT_CAMERA_DUAL_REPROCESSING : FRONT_CAMERA_SINGLE_REPROCESSING;
#else
        ALOGW("WRN(%s[%d]): FRONT_CAMERA_DUAL_REPROCESSING/FRONT_CAMERA_SINGLE_REPROCESSING is not defined", __FUNCTION__, __LINE__);
#endif
    }

    return reprocessing;
}

bool ExynosCamera1Parameters::isSccCapture(void)
{
    bool sccCapture = false;
    int cameraId = getCameraId();
    bool flagDual = getDualMode();

    if (cameraId == CAMERA_ID_BACK) {
#if defined(MAIN_CAMERA_DUAL_SCC_CAPTURE) && defined(MAIN_CAMERA_SINGLE_SCC_CAPTURE)
        sccCapture = (flagDual == true) ? MAIN_CAMERA_DUAL_SCC_CAPTURE : MAIN_CAMERA_SINGLE_SCC_CAPTURE;
#else
        ALOGW("WRN(%s[%d]): MAIN_CAMERA_DUAL_SCC_CAPTURE/MAIN_CAMERA_SINGLE_SCC_CAPTUREis not defined", __FUNCTION__, __LINE__);
#endif
    } else {
#if defined(FRONT_CAMERA_DUAL_SCC_CAPTURE) && defined(FRONT_CAMERA_SINGLE_SCC_CAPTURE)
        sccCapture = (flagDual == true) ? FRONT_CAMERA_DUAL_SCC_CAPTURE : FRONT_CAMERA_SINGLE_SCC_CAPTURE;
#else
        ALOGW("WRN(%s[%d]): FRONT_CAMERA_DUAL_SCC_CAPTURE/FRONT_CAMERA_SINGLE_SCC_CAPTURE is not defined", __FUNCTION__, __LINE__);
#endif
    }

    return sccCapture;
}

bool ExynosCamera1Parameters::isFlite3aaOtf(void)
{
    bool flagOtfInput = false;

    int cameraId = getCameraId();
    bool flagDual = getDualMode();
    bool flagSensorOtf = getSensorOTFSupported();

    if (flagSensorOtf == false)
        return flagOtfInput;

    if (cameraId == CAMERA_ID_BACK) {
        /* for 52xx scenario */
        flagOtfInput = true;

        if (flagDual == true) {
#ifdef MAIN_CAMERA_DUAL_FLITE_3AA_OTF
            flagOtfInput = MAIN_CAMERA_DUAL_FLITE_3AA_OTF;
#else
            ALOGW("WRN(%s[%d]): MAIN_CAMERA_DUAL_FLITE_3AA_OTF is not defined", __FUNCTION__, __LINE__);
#endif
        } else {
#ifdef MAIN_CAMERA_SINGLE_FLITE_3AA_OTF
            flagOtfInput = MAIN_CAMERA_SINGLE_FLITE_3AA_OTF;
#else
            ALOGW("WRN(%s[%d]): MAIN_CAMERA_SINGLE_FLITE_3AA_OTF is not defined", __FUNCTION__, __LINE__);
#endif
        }
    } else {
        if (flagDual == true) {
#ifdef FRONT_CAMERA_DUAL_FLITE_3AA_OTF
            flagOtfInput = FRONT_CAMERA_DUAL_FLITE_3AA_OTF;
#else
            ALOGW("WRN(%s[%d]): FRONT_CAMERA_DUAL_FLITE_3AA_OTF is not defined", __FUNCTION__, __LINE__);
#endif
        } else {
#ifdef FRONT_CAMERA_SINGLE_FLITE_3AA_OTF
            flagOtfInput = FRONT_CAMERA_SINGLE_FLITE_3AA_OTF;
#else
            ALOGW("WRN(%s[%d]): FRONT_CAMERA_SINGLE_FLITE_3AA_OTF is not defined", __FUNCTION__, __LINE__);
#endif
        }
    }

    return flagOtfInput;
}

bool ExynosCamera1Parameters::is3aaIspOtf(void)
{
    bool ret = false;

    int cameraId = getCameraId();
    bool flagDual = getDualMode();

    if (cameraId == CAMERA_ID_BACK) {
        if (flagDual == true) {
#ifdef MAIN_CAMERA_DUAL_3AA_ISP_OTF
            ret = MAIN_CAMERA_DUAL_3AA_ISP_OTF;
#else
            ALOGW("WRN(%s[%d]): MAIN_CAMERA_DUAL_3AA_ISP_OTF is not defined", __FUNCTION__, __LINE__);
#endif
        } else {
#ifdef MAIN_CAMERA_SINGLE_3AA_ISP_OTF
            ret = MAIN_CAMERA_SINGLE_3AA_ISP_OTF;
#else
            ALOGW("WRN(%s[%d]): MAIN_CAMERA_SINGLE_3AA_ISP_OTF is not defined", __FUNCTION__, __LINE__);
#endif
        }
    } else {
        if (flagDual == true) {
#ifdef FRONT_CAMERA_DUAL_3AA_ISP_OTF
            ret = FRONT_CAMERA_DUAL_3AA_ISP_OTF;
#else
            ALOGW("WRN(%s[%d]): FRONT_CAMERA_DUAL_3AA_ISP_OTF is not defined", __FUNCTION__, __LINE__);
#endif
        } else {
#ifdef FRONT_CAMERA_SINGLE_3AA_ISP_OTF
            ret = FRONT_CAMERA_SINGLE_3AA_ISP_OTF;
#else
            ALOGW("WRN(%s[%d]): FRONT_CAMERA_SINGLE_3AA_ISP_OTF is not defined", __FUNCTION__, __LINE__);
#endif
        }
    }

    return ret;
}

bool ExynosCamera1Parameters::isIspMcscOtf(void)
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

bool ExynosCamera1Parameters::isMcscVraOtf(void)
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

bool ExynosCamera1Parameters::isReprocessing3aaIspOTF(void)
{
    bool otf = false;

    int cameraId = getCameraId();
    bool flagDual = getDualMode();

    if (cameraId == CAMERA_ID_BACK) {
        if (flagDual == true) {
#ifdef MAIN_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING
            otf = MAIN_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING;
#else
            ALOGW("WRN(%s[%d]): MAIN_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING is not defined", __FUNCTION__, __LINE__);
#endif
        } else {
#ifdef MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING
            otf = MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING;
#else
            ALOGW("WRN(%s[%d]): MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING is not defined", __FUNCTION__, __LINE__);
#endif
        }
    } else {
        if (flagDual == true) {
#ifdef FRONT_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING
            otf = FRONT_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING;
#else
            ALOGW("WRN(%s[%d]): FRONT_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING is not defined", __FUNCTION__, __LINE__);
#endif
        } else {
#ifdef FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING
            otf = FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING;
#else
            ALOGW("WRN(%s[%d]): FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING is not defined", __FUNCTION__, __LINE__);
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
            ALOGW("WRN(%s[%d]): otf == true. but, flagDirtyBayer == true. so force false on 3aa_isp otf",
                __FUNCTION__, __LINE__);

            otf = false;
        }
    }

    return otf;
}

bool ExynosCamera1Parameters::isReprocessingIspMcscOTF(void)
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

bool ExynosCamera1Parameters::isHWFCEnabled(void)
{
#if defined(USE_JPEG_HWFC)
    return USE_JPEG_HWFC;
#else
    return false;
#endif
}

bool ExynosCamera1Parameters::isHWFCOnDemand(void)
{
#if defined(USE_JPEG_HWFC_ONDEMAND)
    return USE_JPEG_HWFC_ONDEMAND;
#else
    return false;
#endif
}

bool ExynosCamera1Parameters::isUseThumbnailHWFC(void)
{
#if defined(USE_THUMBNAIL_HWFC)
    return USE_JPEG_HWFC_ONDEMAND;
#else
    return false;
#endif
}

void ExynosCamera1Parameters::setZoomPreviewWIthScaler(bool enable)
{
	m_zoomWithScaler = enable;
}

bool ExynosCamera1Parameters::getZoomPreviewWIthScaler(void)
{
	return m_zoomWithScaler;
}

bool ExynosCamera1Parameters::isUsing3acForIspc(void)
{
#if (defined(USE_3AC_FOR_ISPC) && (USE_3AC_FOR_ISPC))
    return true;
#else
    return false;
#endif
}

bool ExynosCamera1Parameters::isOwnScc(int cameraId)
{
    bool ret = false;

    if (cameraId == CAMERA_ID_BACK) {
#ifdef MAIN_CAMERA_HAS_OWN_SCC
        ret = MAIN_CAMERA_HAS_OWN_SCC;
#else
        ALOGW("WRN(%s[%d]): MAIN_CAMERA_HAS_OWN_SCC is not defined", __FUNCTION__, __LINE__);
#endif
    } else {
#ifdef FRONT_CAMERA_HAS_OWN_SCC
        ret = FRONT_CAMERA_HAS_OWN_SCC;
#else
        ALOGW("WRN(%s[%d]): FRONT_CAMERA_HAS_OWN_SCC is not defined", __FUNCTION__, __LINE__);
#endif
    }

    return ret;
}

bool ExynosCamera1Parameters::isOwnMCSC(void)
{
    bool ret = false;

#ifdef OWN_MCSC_HW
    ret = OWN_MCSC_HW;
#endif

    return ret;
}

bool ExynosCamera1Parameters::isCompanion(int cameraId)
{
    bool ret = false;

    if (cameraId == CAMERA_ID_BACK) {
        ALOGI("INFO(%s[%d]): MAIN_CAMERA_USE_SAMSUNG_COMPANION is not defined", __FUNCTION__, __LINE__);
    } else {
        ALOGI("INFO(%s[%d]): FRONT_CAMERA_USE_SAMSUNG_COMPANION is not defined", __FUNCTION__, __LINE__);
    }

    return ret;
}

int ExynosCamera1Parameters::getHalVersion(void)
{
    return m_halVersion;
}

void ExynosCamera1Parameters::setHalVersion(int halVersion)
{
    m_halVersion = halVersion;
    m_activityControl->setHalVersion(m_halVersion);

    ALOGI("INFO(%s[%d]): m_halVersion(%d)", __FUNCTION__, __LINE__, m_halVersion);

    return;
}

struct ExynosSensorInfoBase *ExynosCamera1Parameters::getSensorStaticInfo()
{
    ALOGE("ERR(%s[%d]): halVersion(%d) does not support this function!!!!", __FUNCTION__, __LINE__, m_halVersion);
    return NULL;
}

bool ExynosCamera1Parameters::getSetFileCtlMode(void)
{
#ifdef SET_SETFILE_BY_SET_CTRL
    return true;
#else
    return false;
#endif
}

bool ExynosCamera1Parameters::getSetFileCtl3AA_ISP(void)
{
#ifdef SET_SETFILE_BY_SET_CTRL_3AA_ISP
    return SET_SETFILE_BY_SET_CTRL_3AA_ISP;
#else
    return false;
#endif
}

bool ExynosCamera1Parameters::getSetFileCtl3AA(void)
{
#ifdef SET_SETFILE_BY_SET_CTRL_3AA
    return SET_SETFILE_BY_SET_CTRL_3AA;
#else
    return false;
#endif
}

bool ExynosCamera1Parameters::getSetFileCtlISP(void)
{
#ifdef SET_SETFILE_BY_SET_CTRL_ISP
    return SET_SETFILE_BY_SET_CTRL_ISP;
#else
    return false;
#endif
}

bool ExynosCamera1Parameters::getSetFileCtlSCP(void)
{
#ifdef SET_SETFILE_BY_SET_CTRL_SCP
    return SET_SETFILE_BY_SET_CTRL_SCP;
#else
    return false;
#endif
}

int ExynosCamera1Parameters::getMaxNumCPUCluster(void)
{
#ifdef CLUSTER_MAX_NUM
    return CLUSTER_MAX_NUM;
#else
    return 1;
#endif
}

int ExynosCamera1Parameters::getNumOfReprocessingFactory(void)
{
#ifdef NUM_OF_REPROCESSING_FACTORY
    return NUM_OF_REPROCESSING_FACTORY;
#else
    return 1;
#endif
}

}; /* namespace android */
