/*
 * Copyright 2012, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraActivityAutofocus"
#include <cutils/log.h>

#include "ExynosCameraActivityAutofocus.h"

namespace android {

#define WAIT_COUNT_FAIL_STATE                (7)
#define AUTOFOCUS_WAIT_COUNT_STEP_REQUEST    (3)

#define AUTOFOCUS_WAIT_COUNT_FRAME_COUNT_NUM (3)       /* n + x frame count */
#define AUTOFOCUS_WATING_TIME_LOCK_AF        (10000)   /* 10msec */
#define AUTOFOCUS_TOTAL_WATING_TIME_LOCK_AF  (300000)  /* 300msec */
#define AUTOFOCUS_SKIP_FRAME_LOCK_AF         (6)       /* == NUM_BAYER_BUFFERS */

#define SET_BIT(x)      (1 << x)

ExynosCameraActivityAutofocus::ExynosCameraActivityAutofocus()
{
    m_flagAutofocusStart = false;
    m_flagAutofocusLock = false;

    /* first Lens position is infinity */
    /* m_autoFocusMode = AUTOFOCUS_MODE_BASE; */
    m_autoFocusMode = AUTOFOCUS_MODE_INFINITY;
    m_interenalAutoFocusMode = AUTOFOCUS_MODE_BASE;

    m_focusWeight = 0;
    /* first AF operation is trigger infinity mode */
    /* m_autofocusStep = AUTOFOCUS_STEP_STOP; */
    m_autofocusStep = AUTOFOCUS_STEP_REQUEST;
    m_aaAfState = ::AA_AFSTATE_INACTIVE;
    m_afState = AUTOFOCUS_STATE_NONE;
    m_aaAFMode = ::AA_AFMODE_OFF;
    m_metaCtlAFMode = -1;
    m_waitCountFailState = 0;
    m_stepRequestCount = 0;
    m_frameCount = 0;

    m_recordingHint = false;
    m_flagFaceDetection = false;
    m_macroPosition = AUTOFOCUS_MACRO_POSITION_BASE;
    m_fpsValue = 0;
    m_samsungCamera = false;
    m_afInMotionResult = false;

    m_af_mode_info = 0;
    m_af_pan_focus_info = 0;
    m_af_typical_macro_info = 0;
    m_af_module_version_info = 0;
    m_af_state_info = 0;
    m_af_cur_pos_info = 0;
    m_af_time_info = 0;
    m_af_factory_info = 0;
    m_paf_from_info = 0;
    m_paf_error_code = 0;
}

ExynosCameraActivityAutofocus::~ExynosCameraActivityAutofocus()
{
}

int ExynosCameraActivityAutofocus::t_funcNull(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityAutofocus::t_funcSensorBefore(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityAutofocus::t_funcSensorAfter(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityAutofocus::t_funcISPBefore(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityAutofocus::t_funcISPAfter(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityAutofocus::t_func3ABeforeHAL3(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityAutofocus::t_func3AAfterHAL3(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityAutofocus::t_funcSCPBefore(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityAutofocus::t_funcSCPAfter(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityAutofocus::t_funcSCCBefore(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityAutofocus::t_funcSCCAfter(__unused void *args)
{
    return 1;
}

int ExynosCameraActivityAutofocus::getAutofocusMode(void)
{
    return m_autoFocusMode;
}

bool ExynosCameraActivityAutofocus::getRecordingHint(void)
{
    return m_recordingHint;
}

bool ExynosCameraActivityAutofocus::setFocusAreas(ExynosRect2 rect, int weight)
{
    m_focusArea = rect;
    m_focusWeight = weight;

    return true;
}

bool ExynosCameraActivityAutofocus::getFocusAreas(ExynosRect2 *rect, int *weight)
{
    *rect = m_focusArea;
    *weight = m_focusWeight;

    return true;
}

bool ExynosCameraActivityAutofocus::startAutofocus(void)
{
    ALOGI("INFO(%s[%d]):m_autoFocusMode(%d)", __FUNCTION__, __LINE__, m_autoFocusMode);

    m_autofocusStep = AUTOFOCUS_STEP_REQUEST;
    m_flagAutofocusStart = true;

    return true;
}

bool ExynosCameraActivityAutofocus::stopAutofocus(void)
{
    ALOGI("INFO(%s[%d]):m_autoFocusMode(%d)", __FUNCTION__, __LINE__, m_autoFocusMode);

    m_autofocusStep = AUTOFOCUS_STEP_STOP;
    m_flagAutofocusStart = false;

    return true;
}

bool ExynosCameraActivityAutofocus::flagAutofocusStart(void)
{
    return m_flagAutofocusStart;
}

bool ExynosCameraActivityAutofocus::lockAutofocus()
{
    ALOGI("INFO(%s[%d]):m_autoFocusMode(%d)", __FUNCTION__, __LINE__, m_autoFocusMode);

    if(m_autofocusStep != AUTOFOCUS_STEP_TRIGGER_START) {
        m_autofocusStep = AUTOFOCUS_STEP_TRIGGER_START;

        ALOGI("INFO(%s): request locked state of Focus. : m_autofocusStep(%d), m_aaAfState(%d)",
            __FUNCTION__, m_autofocusStep, m_aaAfState);
    }

    m_flagAutofocusStart = false;

    if (m_aaAfState == AA_AFSTATE_INACTIVE ||
        m_aaAfState == AA_AFSTATE_PASSIVE_SCAN ||
        m_aaAfState == AA_AFSTATE_ACTIVE_SCAN) {
        /*
         * hold, until + 3 Frame
         * n (lockFrameCount) : n - 1's state
         * n + 1              : adjust on f/w
         * n + 2              : adjust on sensor
         * n + 3              : result
         */
        int lockFrameCount = m_frameCount;
        unsigned int i = 0;
        bool flagScanningDetected = false;
        int  scanningDetectedFrameCount = 0;

        for (i = 0; i < AUTOFOCUS_TOTAL_WATING_TIME_LOCK_AF; i += AUTOFOCUS_WATING_TIME_LOCK_AF) {
            if (lockFrameCount + AUTOFOCUS_WAIT_COUNT_FRAME_COUNT_NUM <= m_frameCount) {
                ALOGD("DEBUG(%s):find lockFrameCount(%d) + %d, m_frameCount(%d), m_aaAfState(%d)",
                    __FUNCTION__, lockFrameCount, AUTOFOCUS_WAIT_COUNT_FRAME_COUNT_NUM, m_frameCount, m_aaAfState);
                break;
            }

            if (flagScanningDetected == false) {
                if (m_aaAfState == AA_AFSTATE_PASSIVE_SCAN ||
                    m_aaAfState == AA_AFSTATE_ACTIVE_SCAN) {
                    flagScanningDetected = true;
                    scanningDetectedFrameCount = m_frameCount;
                }
            }

            usleep(AUTOFOCUS_WATING_TIME_LOCK_AF);
        }

        if (AUTOFOCUS_TOTAL_WATING_TIME_LOCK_AF <= i) {
            ALOGW("WARN(%s):AF lock time out (%d)msec", __FUNCTION__, i / 1000);
        } else {
            /* skip bayer frame when scanning detected */
            if (flagScanningDetected == true) {
                for (i = 0; i < AUTOFOCUS_TOTAL_WATING_TIME_LOCK_AF; i += AUTOFOCUS_WATING_TIME_LOCK_AF) {
                    if (scanningDetectedFrameCount + AUTOFOCUS_SKIP_FRAME_LOCK_AF <= m_frameCount) {
                        ALOGD("DEBUG(%s):kcoolsw find scanningDetectedFrameCount(%d) + %d, m_frameCount(%d), m_aaAfState(%d)",
                            __FUNCTION__, scanningDetectedFrameCount, AUTOFOCUS_SKIP_FRAME_LOCK_AF, m_frameCount, m_aaAfState);
                        break;
                    }

                    usleep(AUTOFOCUS_WATING_TIME_LOCK_AF);
                }

                if (AUTOFOCUS_TOTAL_WATING_TIME_LOCK_AF <= i)
                    ALOGW("WARN(%s):kcoolsw scanningDectected skip time out (%d)msec", __FUNCTION__, i / 1000);
            }
        }
    }

    m_flagAutofocusLock = true;

    return true;
}

bool ExynosCameraActivityAutofocus::unlockAutofocus()
{
    ALOGI("INFO(%s[%d]):m_autoFocusMode(%d)", __FUNCTION__, __LINE__, m_autoFocusMode);

    /*
     * With the 3.2 metadata interface,
     * unlockAutofocus() triggers the new AF scanning.
     */
    m_flagAutofocusStart = true;
    m_autofocusStep = AUTOFOCUS_STEP_REQUEST;

    return true;
}

bool ExynosCameraActivityAutofocus::flagLockAutofocus(void)
{
    return m_flagAutofocusLock;
}

int ExynosCameraActivityAutofocus::getCurrentState(void)
{
    int state = AUTOFOCUS_STATE_NONE;

    if (m_flagAutofocusStart == false) {
        state = m_afState;
        goto done;
    }

    switch (m_aaAfState) {
    case ::AA_AFSTATE_INACTIVE:
        state = AUTOFOCUS_STATE_NONE;
        break;
    case ::AA_AFSTATE_PASSIVE_SCAN:
    case ::AA_AFSTATE_ACTIVE_SCAN:
        state = AUTOFOCUS_STATE_SCANNING;
        break;
    case ::AA_AFSTATE_PASSIVE_FOCUSED:
    case ::AA_AFSTATE_FOCUSED_LOCKED:
        state = AUTOFOCUS_STATE_SUCCEESS;
        break;
    case ::AA_AFSTATE_NOT_FOCUSED_LOCKED:
    case ::AA_AFSTATE_PASSIVE_UNFOCUSED:
        state = AUTOFOCUS_STATE_FAIL;
        break;
    default:
        state = AUTOFOCUS_STATE_NONE;
        break;
    }

done:
    m_afState = state;

    return state;
}

bool ExynosCameraActivityAutofocus::setRecordingHint(bool hint)
{
    ALOGI("INFO(%s[%d]):hint(%d)", __FUNCTION__, __LINE__, hint);

    m_recordingHint = hint;
    return true;
}

bool ExynosCameraActivityAutofocus::setFaceDetection(bool toggle)
{
    ALOGI("INFO(%s[%d]):toggle(%d)", __FUNCTION__, __LINE__, toggle);

    m_flagFaceDetection = toggle;
    return true;
}

bool ExynosCameraActivityAutofocus::setMacroPosition(int macroPosition)
{
    ALOGI("INFO(%s[%d]):macroPosition(%d)", __FUNCTION__, __LINE__, macroPosition);

    m_macroPosition = macroPosition;
    return true;
}

void ExynosCameraActivityAutofocus::setFpsValue(int fpsValue)
{
    m_fpsValue = fpsValue;
}

int ExynosCameraActivityAutofocus::getFpsValue()
{
    return m_fpsValue;
}

void ExynosCameraActivityAutofocus::setSamsungCamera(int flags)
{
    m_samsungCamera = flags;
}

void ExynosCameraActivityAutofocus::setAfInMotionResult(bool afInMotion)
{
    m_afInMotionResult = afInMotion;
}

bool ExynosCameraActivityAutofocus::getAfInMotionResult(void)
{
    return m_afInMotionResult;
}

void ExynosCameraActivityAutofocus::displayAFInfo(void)
{
    ALOGD("(%s):==================================================", "CMGEFL");
    ALOGD("(%s):0x%x", "CMGEFL", m_af_mode_info);
    ALOGD("(%s):0x%x", "CMGEFL", m_af_pan_focus_info);
    ALOGD("(%s):0x%x", "CMGEFL", m_af_typical_macro_info);
    ALOGD("(%s):0x%x", "CMGEFL", m_af_module_version_info);
    ALOGD("(%s):0x%x", "CMGEFL", m_af_state_info);
    ALOGD("(%s):0x%x", "CMGEFL", m_af_cur_pos_info);
    ALOGD("(%s):0x%x", "CMGEFL", m_af_time_info);
    ALOGD("(%s):0x%x", "CMGEFL", m_af_factory_info);
    ALOGD("(%s):0x%x", "CMGEFL", m_paf_from_info);
    ALOGD("(%s):0x%x", "CMGEFL", m_paf_error_code);
    ALOGD("(%s):==================================================", "CMGEFL");
    return ;
}

void ExynosCameraActivityAutofocus::displayAFStatus(void)
{
    ALOGD("(%s):0x%x / 0x%x / 0x%x", "CMGEFL",
           m_af_state_info, m_af_cur_pos_info, m_af_time_info);
    return ;
}
} /* namespace android */
