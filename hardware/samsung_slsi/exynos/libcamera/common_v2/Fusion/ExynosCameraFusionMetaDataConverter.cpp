/*
 * Copyright@ Samsung Electronics Co. LTD
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

#define LOG_TAG "ExynosCameraFusionMetaDataConverter"

#include "ExynosCameraFusionMetaDataConverter.h"

//#define EXYNOS_CAMERA_FUSION_META_DATA_CONVERTER_DEBUG

#ifdef EXYNOS_CAMERA_FUSION_META_DATA_CONVERTER_DEBUG
#define META_CONVERTER_LOG CLOGD
#else
#define META_CONVERTER_LOG CLOGV
#endif

void ExynosCameraFusionMetaDataConverter::translateFocusPos(int cameraId,
                                                            camera2_shot_ext *shot_ext,
                                                            DOF *dof,
                                                            float *nearFieldCm,
                                                            float *lensShiftUm,
                                                            float *farFieldCm)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    if (shot_ext == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):shot_ext == NULL on cameraId(%d), assert!!!!", __FUNCTION__, __LINE__, m_cameraId);
    }

    // FW's meta
    float currentPos = (float)shot_ext->shot.udm.af.lensPositionCurrent;
    float calibCurrentPos = currentPos;

    // FW's meta
    float macroPos = (float)shot_ext->shot.udm.af.lensPositionMacro;

    // FW's meta
    float infinityPos = (float)shot_ext->shot.udm.af.lensPositionInfinity;

    if (macroPos < calibCurrentPos)
        calibCurrentPos = macroPos;

    if (calibCurrentPos < infinityPos)
        calibCurrentPos = infinityPos;

    // convert pos to lens shift
    *lensShiftUm = 0;
    float tempPos = (calibCurrentPos - infinityPos) * (dof->lensShiftOn01M - dof->lensShiftOn12M);
    float fullPos = (macroPos - infinityPos);

    if (tempPos != 0.0f && fullPos != 0.0f) {
        *lensShiftUm = tempPos / fullPos;
    }

    *lensShiftUm += dof->lensShiftOn12M;

    if (*lensShiftUm < 0.0f) {
        META_CONVERTER_LOG("DEBUG(%s[%d]):*lensShiftUm(%f) <= 0.0f. so, set 0.0f",__FUNCTION__, __LINE__, *lensShiftUm);
        *lensShiftUm = 0.0f;
    }

    // DOF table + lens_shift_um
    *nearFieldCm = m_findLensField(m_cameraId, dof, *lensShiftUm, false);

    // DOF table + lens_shift_um
    *farFieldCm  = m_findLensField(m_cameraId, dof, *lensShiftUm, true);

     // mm -> cm
    if (*nearFieldCm != 0.0f)
        *nearFieldCm /= 10.0f;

    // mm -> cm
    if (*farFieldCm != 0.0f)
        *farFieldCm /= 10.0f;

    // mm -> um
    *lensShiftUm *= 1000.0f;

    META_CONVERTER_LOG("DEBUG(%s[%d]):macroPos(%f), currentPos(%f), calibCurrentPos(%f), infinityPos(%f), lensShiftOn01M(%f), lensShiftOn12M(%f) -> nearFieldCm(%f), lensShiftUm(%f), farFieldCm(%f)",
        __FUNCTION__, __LINE__,
        macroPos,
        currentPos,
        calibCurrentPos,
        infinityPos,
        dof->lensShiftOn01M,
        dof->lensShiftOn12M,
        *nearFieldCm,
        *lensShiftUm,
        *farFieldCm);
}

ExynosRect ExynosCameraFusionMetaDataConverter::translateFocusRoi(int cameraId,
                                                                  struct camera2_shot_ext *shot_ext)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    if (shot_ext == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):shot_ext == NULL on cameraId(%d), assert!!!!", __FUNCTION__, __LINE__, m_cameraId);
    }

    ExynosRect rect;

    // Focus ROI
    rect.x = shot_ext->shot.dm.aa.afRegions[0];
    rect.y = shot_ext->shot.dm.aa.afRegions[1];
    rect.w = shot_ext->shot.dm.aa.afRegions[2] - shot_ext->shot.dm.aa.afRegions[0];
    rect.h = shot_ext->shot.dm.aa.afRegions[3] - shot_ext->shot.dm.aa.afRegions[1];
    rect.fullW = rect.w;
    rect.fullH = rect.h;

    META_CONVERTER_LOG("DEBUG(%s[%d]):afRegions:(%d, %d, %d, %d)",
        __FUNCTION__, __LINE__,
        rect.x,
        rect.y,
        rect.w,
        rect.h);

    return rect;
}

bool ExynosCameraFusionMetaDataConverter::translateAfStatus(int cameraId,
                                                            struct camera2_shot_ext *shot_ext)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    bool ret = false;

    if (shot_ext == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):shot_ext == NULL on cameraId(%d), assert!!!!", __FUNCTION__, __LINE__, m_cameraId);
    }

    ExynosCameraActivityAutofocus::AUTOFOCUS_STATE afState =  ExynosCameraActivityAutofocus::afState2AUTOFOCUS_STATE(shot_ext->shot.dm.aa.afState);

    switch (afState) {
    case ExynosCameraActivityAutofocus::AUTOFOCUS_STATE_SUCCEESS:
        ret = true;
        break;
    case ExynosCameraActivityAutofocus::AUTOFOCUS_STATE_FAIL:
    case ExynosCameraActivityAutofocus::AUTOFOCUS_STATE_SCANNING:
    default:
        ret = false;
        break;
    }

    META_CONVERTER_LOG("DEBUG(%s[%d]):afState:(%d) ExynosCameraActivityAutofocus::AUTOFOCUS_STATE(%d), ret(%d)",
        __FUNCTION__, __LINE__,
        shot_ext->shot.dm.aa.afState, afState, ret);

    return ret;
}

float ExynosCameraFusionMetaDataConverter::translateAnalogGain(int cameraId,
                                                               struct camera2_shot_ext *shot_ext)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    if (shot_ext == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):shot_ext == NULL on cameraId(%d), assert!!!!", __FUNCTION__, __LINE__, m_cameraId);
    }

    // AE gain
    META_CONVERTER_LOG("DEBUG(%s[%d]):analogGain:(%d)",
        __FUNCTION__, __LINE__,
        shot_ext->shot.udm.sensor.analogGain);

    return (float)shot_ext->shot.udm.sensor.analogGain;
}

void ExynosCameraFusionMetaDataConverter::translateScalerSetting(int cameraId,
                                                                 struct camera2_shot_ext *shot_ext,
                                                                 int perFramePos,
                                                                 ExynosRect *yRect,
                                                                 ExynosRect *cbcrRect)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    if (shot_ext == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):shot_ext == NULL on cameraId(%d), assert!!!!", __FUNCTION__, __LINE__, m_cameraId);
    }

    yRect->w = shot_ext->node_group.capture[perFramePos].output.cropRegion[2] -
               shot_ext->node_group.capture[perFramePos].output.cropRegion[0];

    yRect->h = shot_ext->node_group.capture[perFramePos].output.cropRegion[3] -
               shot_ext->node_group.capture[perFramePos].output.cropRegion[1];

    yRect->fullW = yRect->w;
    yRect->fullH = yRect->h;

    cbcrRect->w = yRect->w / 2;
    cbcrRect->h = yRect->h;

    cbcrRect->fullW = cbcrRect->w;
    cbcrRect->fullH = cbcrRect->h;

    META_CONVERTER_LOG("DEBUG(%s[%d]):scalerSetting:perFramePos(%d) y(%d x %d) cbcr(%d x %d)",
        __FUNCTION__, __LINE__,
        perFramePos,
        yRect->w,
        yRect->h,
        cbcrRect->w,
        cbcrRect->h);
}

void ExynosCameraFusionMetaDataConverter::translateCropSetting(int cameraId,
                                                               struct camera2_shot_ext *shot_ext,
                                                               int perFramePos,
                                                               ExynosRect2 *yRect,
                                                               ExynosRect2 *cbcrRect)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    if (shot_ext == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):shot_ext == NULL on cameraId(%d), assert!!!!", __FUNCTION__, __LINE__, m_cameraId);
    }

    ExynosRect scalerYRect;
    ExynosRect scalerCbRect;

    translateScalerSetting(cameraId, shot_ext, perFramePos, &scalerYRect, &scalerCbRect);

    yRect->x1 = 0;
    yRect->x2 = scalerYRect.w;

    yRect->y1 = 0;
    yRect->y2 = scalerYRect.h;

    cbcrRect->x1 = 0;
    cbcrRect->x2 = scalerCbRect.w;

    cbcrRect->y1 = 0;
    cbcrRect->y2 = scalerCbRect.h;

    META_CONVERTER_LOG("DEBUG(%s[%d]):cropSetting:perFramePos(%d) y(%d<->%d x %d<->%d) cbcr(%d<->%d x %d<->%d)",
        __FUNCTION__, __LINE__,
        perFramePos,
        yRect->x1,    yRect->x2,    yRect->y1,    yRect->y2,
        cbcrRect->x1, cbcrRect->x2, cbcrRect->y1, cbcrRect->y2);
}

float ExynosCameraFusionMetaDataConverter::translateZoomRatio(int cameraId,
                                                              struct camera2_shot_ext *shot_ext)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    if (shot_ext == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):shot_ext == NULL on cameraId(%d), assert!!!!", __FUNCTION__, __LINE__, m_cameraId);
    }

    // zoomRatio
    float zoomRatio = shot_ext->shot.udm.zoomRatio;

    //getMetaCtlZoom(shot_ext, &zoomRatio);

    META_CONVERTER_LOG("DEBUG(%s[%d]):zoomRatio:(%f)",
        __FUNCTION__, __LINE__,
        zoomRatio);

    return zoomRatio;
}

void ExynosCameraFusionMetaDataConverter::translate2Parameters(int cameraId,
                                                               CameraParameters *params,
                                                               struct camera2_shot_ext *shot_ext,
                                                               DOF *dof,
                                                               ExynosRect pictureRect)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    status_t ret = NO_ERROR;

    if (shot_ext == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):shot_ext == NULL on cameraId(%d), assert!!!!", __FUNCTION__, __LINE__, m_cameraId);
    }

    ///////////////////////////////////////////////
    // Focus distances
    float nearFieldCm = 0.0f;
    float lensShiftUm = 0.0f;
    float farFieldCm = 0.0f;

    translateFocusPos(cameraId, shot_ext, dof, &nearFieldCm, &lensShiftUm, &farFieldCm);

    CLOGD("DEBUG(%s[%d]):focus-distances: nearFiledCm(%f), lensShiftUm(%f), farFiledCm(%f)",
        __FUNCTION__, __LINE__,
        nearFieldCm,
        lensShiftUm,
        farFieldCm);

    char tempStr[EXYNOS_CAMERA_NAME_STR_SIZE];

    sprintf(tempStr, "%.1f,%.1f,%.1f", nearFieldCm, lensShiftUm, farFieldCm);
    params->set(CameraParameters::KEY_FOCUS_DISTANCES, tempStr);

    ///////////////////////////////////////////////
    // Focus ROI
    ExynosRect bayerRoiRect;
    bayerRoiRect = translateFocusRoi(cameraId, shot_ext);

    // calibrate to picture size.
    ExynosRect pictureRoiRect;
    float wRatio = (float)pictureRect.w / (float)((bayerRoiRect.x * 2) + bayerRoiRect.w);
    float hRatio = (float)pictureRect.h / (float)((bayerRoiRect.y * 2) + bayerRoiRect.h);

    pictureRoiRect.x = (int)((float)bayerRoiRect.x * wRatio);
    pictureRoiRect.y = (int)((float)bayerRoiRect.y * hRatio);
    pictureRoiRect.w = (int)((float)bayerRoiRect.w * wRatio);
    pictureRoiRect.h = (int)((float)bayerRoiRect.h * hRatio);

    params->set("roi_startx", pictureRoiRect.x);
    params->set("roi_starty", pictureRoiRect.y);
    params->set("roi_width",  pictureRoiRect.w);
    params->set("roi_height", pictureRoiRect.h);

    CLOGD("DEBUG(%s[%d]):Roi(afRegion):bayerRoiRect(%d, %d, %d, %d) -> pictureRoiRect(%d, %d, %d, %d) in pictureSize(%d x %d)",
        __FUNCTION__, __LINE__,
        bayerRoiRect.x, bayerRoiRect.y, bayerRoiRect.w, bayerRoiRect.h,
        pictureRoiRect.x, pictureRoiRect.y, pictureRoiRect.w, pictureRoiRect.h,
        pictureRect.w, pictureRect.h);

    ///////////////////////////////////////////////
    // AE gain
    float analogGain = 0.0f;

    analogGain = translateAnalogGain(cameraId, shot_ext);

    // min : 100
    float analogGainRatio = (float)(shot_ext->shot.udm.sensor.analogGain) / 100.0f;

    CLOGD("DEBUG(%s[%d]):ae_info_gain(analogGain):(%f), analogGainRatio(%f)", __FUNCTION__, __LINE__, analogGain, analogGainRatio);

    params->setFloat("ae_info_gain", analogGainRatio);
}

float ExynosCameraFusionMetaDataConverter::m_findLensField(int cameraId,
                                                           DOF *dof,
                                                           float currentLensShift,
                                                           bool flagFar)
{
    // hack for CLOG
    int         m_cameraId = cameraId;
    const char *m_name = "";

    if (dof == NULL) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):dof == NULL on cameraId(%d), assert!!!!", __FUNCTION__, __LINE__, m_cameraId);
    }

    bool  found = false;
    int   foundIndex = 0;

    float targetShift = 0.0;
    float targetField = 0.0f;

    // variables for interpolation
    float weight = 0.0f;
    float leftLensShift = 0.0f;
    float leftField = 0.0f;
    float rightLensShift = 0.0f;
    float rightField = 0.0f;

    if (flagFar == true)
        targetField = DEFAULT_DISTANCE_FAR;
    else
        targetField = DEFAULT_DISTANCE_NEAR;

    for (foundIndex = 0; foundIndex < dof->lutCnt; foundIndex++) {
        if (currentLensShift == dof->lut[foundIndex].lensShift) {
            if (flagFar == true)
                targetField = dof->lut[foundIndex].farField;
            else
                targetField = dof->lut[foundIndex].nearField;

            found = true;
            break;
        } else if (currentLensShift < dof->lut[foundIndex].lensShift) {
            break;
        }
    }

    if (found == true) {
        targetShift = dof->lut[foundIndex].lensShift;
    } else {
        if (dof->lutCnt == 0) {
            CLOGW("WARN(%s[%d]):use targetField(%f), by currentLensShift(%f), dof->lutCnt(%d)",
                __FUNCTION__, __LINE__, targetField, currentLensShift, dof->lutCnt);
        } else {
            // clipping in the end of table
            if (foundIndex == 0 || dof->lutCnt - 1 < foundIndex) {
                if (dof->lutCnt - 1 < foundIndex)
                    foundIndex = dof->lutCnt - 1;

                if (flagFar == true)
                    targetField = dof->lut[foundIndex].farField;
                else
                    targetField = dof->lut[foundIndex].nearField;

                META_CONVERTER_LOG("DEBUG(%s[%d]):clip on foundIndex(%d)", __FUNCTION__, __LINE__, foundIndex);

            } else { // calibrate between two value.
                leftLensShift = dof->lut[foundIndex-1].lensShift;
                rightLensShift = dof->lut[foundIndex].lensShift;

                if (flagFar == true) {
                    leftField = dof->lut[foundIndex-1].farField;
                    rightField = dof->lut[foundIndex].farField;
                } else {
                    leftField = dof->lut[foundIndex-1].nearField;
                    rightField = dof->lut[foundIndex].nearField;
                }

                weight = (currentLensShift - leftLensShift) / (rightLensShift - leftLensShift);

                targetField = ((rightField - leftField) * weight) + leftField;

                CLOGD("DEBUG(%s[%d]):calibrated on foundIndex(%d), weight(%f) = currentLensShift(%f) - leftLensShift(%f)) / (rightLensShift(%f) - leftLensShift(%f))",
                    __FUNCTION__, __LINE__, foundIndex, weight, currentLensShift, leftLensShift, rightLensShift, leftLensShift);
            }

            targetShift = dof->lut[foundIndex].lensShift;
        }
    }

done:
    META_CONVERTER_LOG("DEBUG(%s[%d]):use foundIndex(%d) targetShift(%f)'s targetField(%f), by currentLensShift(%f), dof->lutCnt(%d)",
            __FUNCTION__, __LINE__, foundIndex, targetShift, targetField, currentLensShift, dof->lutCnt);

    return targetField;
}
