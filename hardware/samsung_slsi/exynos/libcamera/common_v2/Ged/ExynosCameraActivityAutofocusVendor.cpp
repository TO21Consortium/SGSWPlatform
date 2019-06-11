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
#define LOG_TAG "ExynosCameraActivityAutofocusGed"
#include <cutils/log.h>

#include "ExynosCameraActivityAutofocus.h"

namespace android {

int ExynosCameraActivityAutofocus::t_func3ABefore(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;

    camera2_shot_ext *shot_ext;
    shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);

    if (shot_ext == NULL) {
        ALOGE("ERR(%s[%d]):shot_ext is null", __FUNCTION__, __LINE__);
        return false;
    }

    int currentState = this->getCurrentState();

    shot_ext->shot.ctl.aa.vendor_afState = (enum aa_afstate)m_aaAfState;

    switch (m_autofocusStep) {
        case AUTOFOCUS_STEP_STOP:
            /* m_interenalAutoFocusMode = m_autoFocusMode; */
            shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_OFF;
            shot_ext->shot.ctl.aa.vendor_afmode_option = 0x00;
            shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_IDLE;
            break;
        case AUTOFOCUS_STEP_TRIGGER_START:
            /* Autofocus lock for capture.
             * The START afTrigger make the AF state be locked on current state.
             */
            m_AUTOFOCUS_MODE2AA_AFMODE(m_interenalAutoFocusMode, shot_ext);
            shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_START;
            break;
        case AUTOFOCUS_STEP_REQUEST:
            /* Autofocus unlock for capture.
             * If the AF request is triggered by "unlockAutofocus()",
             * Send CANCEL afTrigger to F/W and start new AF scanning.
             */
            if (m_flagAutofocusLock == true) {
                m_AUTOFOCUS_MODE2AA_AFMODE(m_interenalAutoFocusMode, shot_ext);
                shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_CANCEL;
                m_flagAutofocusLock = false;
                m_autofocusStep = AUTOFOCUS_STEP_START;
            } else {
                shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_OFF;
                shot_ext->shot.ctl.aa.vendor_afmode_option = 0x00;
                shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_IDLE;

                /*
                 * assure triggering is valid
                 * case 0 : adjusted m_aaAFMode is AA_AFMODE_OFF
                 * case 1 : AUTOFOCUS_STEP_REQUESTs more than 3 times.
                 */
                if (m_aaAFMode == ::AA_AFMODE_OFF ||
                        AUTOFOCUS_WAIT_COUNT_STEP_REQUEST < m_stepRequestCount) {

                    if (AUTOFOCUS_WAIT_COUNT_STEP_REQUEST < m_stepRequestCount)
                        ALOGD("DEBUG(%s[%d]):m_stepRequestCount(%d), force AUTOFOCUS_STEP_START",
                                __FUNCTION__, __LINE__, m_stepRequestCount);

                    m_stepRequestCount = 0;

                    m_autofocusStep = AUTOFOCUS_STEP_START;
                } else {
                    m_stepRequestCount++;
                }
            }

            break;
        case AUTOFOCUS_STEP_START:
            m_interenalAutoFocusMode = m_autoFocusMode;
            m_AUTOFOCUS_MODE2AA_AFMODE(m_autoFocusMode, shot_ext);

            if (m_interenalAutoFocusMode == AUTOFOCUS_MODE_TOUCH) {
                shot_ext->shot.ctl.aa.afRegions[0] = m_focusArea.x1;
                shot_ext->shot.ctl.aa.afRegions[1] = m_focusArea.y1;
                shot_ext->shot.ctl.aa.afRegions[2] = m_focusArea.x2;
                shot_ext->shot.ctl.aa.afRegions[3] = m_focusArea.y2;
                shot_ext->shot.ctl.aa.afRegions[4] = m_focusWeight;
            } else {
                shot_ext->shot.ctl.aa.afRegions[0] = 0;
                shot_ext->shot.ctl.aa.afRegions[1] = 0;
                shot_ext->shot.ctl.aa.afRegions[2] = 0;
                shot_ext->shot.ctl.aa.afRegions[3] = 0;
                shot_ext->shot.ctl.aa.afRegions[4] = 1000;

                /* macro position */
                if (m_interenalAutoFocusMode == AUTOFOCUS_MODE_CONTINUOUS_PICTURE ||
                        m_interenalAutoFocusMode == AUTOFOCUS_MODE_MACRO ||
                        m_interenalAutoFocusMode == AUTOFOCUS_MODE_AUTO) {
                    if (m_macroPosition == AUTOFOCUS_MACRO_POSITION_CENTER)
                        shot_ext->shot.ctl.aa.afRegions[4] = AA_AFMODE_EXT_ADVANCED_MACRO_FOCUS;
                    else if(m_macroPosition == AUTOFOCUS_MACRO_POSITION_CENTER_UP)
                        shot_ext->shot.ctl.aa.afRegions[4] = AA_AFMODE_EXT_FOCUS_LOCATION;
                }
            }

            ALOGD("DEBUG(%s[%d]):AF-Mode(HAL/FW)=(%d/%d(%d)) AF-Region(x1,y1,x2,y2,weight)=(%d, %d, %d, %d, %d)",
                    __FUNCTION__, __LINE__, m_interenalAutoFocusMode, shot_ext->shot.ctl.aa.afMode, shot_ext->shot.ctl.aa.vendor_afmode_option,
                    shot_ext->shot.ctl.aa.afRegions[0],
                    shot_ext->shot.ctl.aa.afRegions[1],
                    shot_ext->shot.ctl.aa.afRegions[2],
                    shot_ext->shot.ctl.aa.afRegions[3],
                    shot_ext->shot.ctl.aa.afRegions[4]);

            switch (m_interenalAutoFocusMode) {
                /* these affect directly */
                case AUTOFOCUS_MODE_INFINITY:
                case AUTOFOCUS_MODE_FIXED:
                    /* These above mode may be considered like CAF. */
                    /*
                       m_autofocusStep = AUTOFOCUS_STEP_DONE;
                       break;
                     */
                    /* these start scanning directrly */
                case AUTOFOCUS_MODE_CONTINUOUS_VIDEO:
                case AUTOFOCUS_MODE_CONTINUOUS_PICTURE:
                case AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
                    m_autofocusStep = AUTOFOCUS_STEP_SCANNING;
                    break;
                    /* these need to wait starting af */
                default:
                    m_autofocusStep = AUTOFOCUS_STEP_START_SCANNING;
                    break;
            }

            break;
        case AUTOFOCUS_STEP_START_SCANNING:
            m_AUTOFOCUS_MODE2AA_AFMODE(m_interenalAutoFocusMode, shot_ext);

            /* set TAF regions */
            if (m_interenalAutoFocusMode == AUTOFOCUS_MODE_TOUCH) {
                shot_ext->shot.ctl.aa.afRegions[0] = m_focusArea.x1;
                shot_ext->shot.ctl.aa.afRegions[1] = m_focusArea.y1;
                shot_ext->shot.ctl.aa.afRegions[2] = m_focusArea.x2;
                shot_ext->shot.ctl.aa.afRegions[3] = m_focusArea.y2;
                shot_ext->shot.ctl.aa.afRegions[4] = m_focusWeight;
            }

            if (currentState == AUTOFOCUS_STATE_SCANNING) {
                m_autofocusStep = AUTOFOCUS_STEP_SCANNING;
                m_waitCountFailState = 0;
            }

            break;
        case AUTOFOCUS_STEP_SCANNING:
            m_AUTOFOCUS_MODE2AA_AFMODE(m_interenalAutoFocusMode, shot_ext);

            /* set TAF regions */
            if (m_interenalAutoFocusMode == AUTOFOCUS_MODE_TOUCH) {
                shot_ext->shot.ctl.aa.afRegions[0] = m_focusArea.x1;
                shot_ext->shot.ctl.aa.afRegions[1] = m_focusArea.y1;
                shot_ext->shot.ctl.aa.afRegions[2] = m_focusArea.x2;
                shot_ext->shot.ctl.aa.afRegions[3] = m_focusArea.y2;
                shot_ext->shot.ctl.aa.afRegions[4] = m_focusWeight;
            }

            switch (m_interenalAutoFocusMode) {
                case AUTOFOCUS_MODE_CONTINUOUS_VIDEO:
                case AUTOFOCUS_MODE_CONTINUOUS_PICTURE:
                case AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
                    break;
                default:
                    if (currentState == AUTOFOCUS_STATE_SUCCEESS ||
                            currentState == AUTOFOCUS_STATE_FAIL) {

                        /* some times fail is happen on 3, 4, 5 count while scanning */
                        if (currentState == AUTOFOCUS_STATE_FAIL &&
                                m_waitCountFailState < WAIT_COUNT_FAIL_STATE) {
                            m_waitCountFailState++;
                            break;
                        }

                        m_waitCountFailState = 0;
                        m_autofocusStep = AUTOFOCUS_STEP_DONE;
                    } else {
                        m_waitCountFailState++;
                    }
                    break;
            }
            break;
        case AUTOFOCUS_STEP_DONE:
            /* to assure next AUTOFOCUS_MODE_AUTO and AUTOFOCUS_MODE_TOUCH */
            shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_OFF;
            shot_ext->shot.ctl.aa.vendor_afmode_option = 0x00;
            shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_IDLE;
            break;
        default:
            break;
    }

    return 1;
}

int ExynosCameraActivityAutofocus::t_func3AAfter(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;

    camera2_shot_ext *shot_ext;
    shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);

    if (shot_ext == NULL) {
        ALOGE("ERR(%s[%d]):shot_ext is null", __FUNCTION__, __LINE__);
        return false;
    }

    m_aaAfState = shot_ext->shot.dm.aa.afState;

    m_aaAFMode  = shot_ext->shot.ctl.aa.afMode;

    m_frameCount = shot_ext->shot.dm.request.frameCount;

    return true;
}

bool ExynosCameraActivityAutofocus::setAutofocusMode(int autoFocusMode)
{
    ALOGI("INFO(%s[%d]):autoFocusMode(%d)", __FUNCTION__, __LINE__, autoFocusMode);

    bool ret = true;

    switch(autoFocusMode) {
    case AUTOFOCUS_MODE_AUTO:
    case AUTOFOCUS_MODE_INFINITY:
    case AUTOFOCUS_MODE_MACRO:
    case AUTOFOCUS_MODE_FIXED:
    case AUTOFOCUS_MODE_EDOF:
    case AUTOFOCUS_MODE_CONTINUOUS_VIDEO:
    case AUTOFOCUS_MODE_CONTINUOUS_PICTURE:
    case AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
    case AUTOFOCUS_MODE_TOUCH:
        m_autoFocusMode = autoFocusMode;
        break;
    default:
        ALOGE("ERR(%s):invalid focus mode(%d) fail", __FUNCTION__, autoFocusMode);
        ret = false;
        break;
    }

    return ret;
}

bool ExynosCameraActivityAutofocus::getAutofocusResult(bool flagLockFocus, bool flagStartFaceDetection, int numOfFace)
{
    ALOGI("INFO(%s[%d]):getAutofocusResult in m_autoFocusMode(%d)",
        __FUNCTION__, __LINE__, m_autoFocusMode);

    bool ret = false;
    bool af_over = false;
    bool flagCheckStep = false;
    int  currentState = AUTOFOCUS_STATE_NONE;
    bool flagScanningStarted = false;
    int flagtrigger = true;

    unsigned int i = 0;

    unsigned int waitTimeoutFpsValue = 0;

    if (m_samsungCamera) {
        if (getFpsValue() > 0) {
            waitTimeoutFpsValue = 30 / getFpsValue();
        }
        if (waitTimeoutFpsValue < 1)
            waitTimeoutFpsValue = 1;
    } else {
        waitTimeoutFpsValue = 1;
    }

    ALOGI("INFO(%s[%d]):waitTimeoutFpsValue(%d), getFpsValue(%d), flagStartFaceDetection(%d), numOfFace(%d)",
            __FUNCTION__, __LINE__, waitTimeoutFpsValue, getFpsValue(), flagStartFaceDetection, numOfFace);

    for (i = 0; i < AUTOFOCUS_TOTAL_WATING_TIME * waitTimeoutFpsValue; i += AUTOFOCUS_WATING_TIME) {
        currentState = this->getCurrentState();

        /*
           * TRIGGER_START means that lock the AF state.
           */
        if(flagtrigger && flagLockFocus && (m_interenalAutoFocusMode == m_autoFocusMode)) {
            m_autofocusStep = AUTOFOCUS_STEP_TRIGGER_START;
            flagtrigger = false;
            ALOGI("INFO(%s):m_aaAfState(%d) flagLockFocus(%d) m_interenalAutoFocusMode(%d) m_autoFocusMode(%d)",
                __FUNCTION__, m_aaAfState, flagLockFocus, m_interenalAutoFocusMode, m_autoFocusMode);
        }

        /* If stopAutofocus() called */
        if (m_autofocusStep == AUTOFOCUS_STEP_STOP && m_aaAfState == AA_AFSTATE_INACTIVE ) {
            m_afState = AUTOFOCUS_STATE_FAIL;
            af_over = true;

            if (currentState == AUTOFOCUS_STATE_SUCCEESS)
                ret = true;
            else
                ret = false;

            break; /* break for for() loop */
        }

        switch (m_interenalAutoFocusMode) {
        case AUTOFOCUS_MODE_INFINITY:
        case AUTOFOCUS_MODE_FIXED:
            /* These above mode may be considered like CAF. */
            /*
            af_over = true;
            ret = true;
            break;
            */
        case AUTOFOCUS_MODE_CONTINUOUS_VIDEO:
        case AUTOFOCUS_MODE_CONTINUOUS_PICTURE:
        case AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
            if (m_autofocusStep == AUTOFOCUS_STEP_SCANNING
                || m_autofocusStep == AUTOFOCUS_STEP_DONE
                || m_autofocusStep == AUTOFOCUS_STEP_TRIGGER_START) {
                flagCheckStep = true;
            }
            break;
        default:
            if (m_autofocusStep == AUTOFOCUS_STEP_DONE)
                flagCheckStep = true;
            break;
        }

        if (flagCheckStep == true) {
            switch (currentState) {
            case AUTOFOCUS_STATE_NONE:
                if (flagScanningStarted == true)
                    ALOGW("WARN(%s):AF restart is detected(%d)", __FUNCTION__, i / 1000);

                if (m_interenalAutoFocusMode == AUTOFOCUS_MODE_CONTINUOUS_PICTURE) {
                    ALOGD("DEBUG(%s):AF force-success on AUTOFOCUS_MODE_CONTINUOUS_PICTURE (%d)", __FUNCTION__, i / 1000);
                    af_over = true;
                    ret = true;
                }
                break;
            case AUTOFOCUS_STATE_SCANNING:
                flagScanningStarted = true;
                break;
            case AUTOFOCUS_STATE_SUCCEESS:
                if (flagStartFaceDetection && numOfFace > 0) {
                    if (m_aaAfState == AA_AFSTATE_FOCUSED_LOCKED) {
                        af_over = true;
                        ret = true;
                    }
                } else {
                    af_over = true;
                    ret = true;
                }
                break;
            case AUTOFOCUS_STATE_FAIL:
                if (flagStartFaceDetection && numOfFace > 0) {
                    if (m_aaAfState == AA_AFSTATE_NOT_FOCUSED_LOCKED) {
                        af_over = true;
                        ret = false;
                    }
                } else {
                    af_over = true;
                    ret = false;
                }
                break;
             default:
                ALOGV("ERR(%s):Invalid afState(%d)", __FUNCTION__, currentState);
                ret = false;
                break;
            }
        }

        if (af_over == true)
            break;

        usleep(AUTOFOCUS_WATING_TIME);
    }

    if (AUTOFOCUS_TOTAL_WATING_TIME * waitTimeoutFpsValue <= i) {
        ALOGW("WARN(%s):AF result time out(%d) msec", __FUNCTION__, i * waitTimeoutFpsValue / 1000);
        stopAutofocus(); /* Reset Previous AF */
        m_afState = AUTOFOCUS_STATE_FAIL;
    }

    ALOGI("INFO(%s[%d]):getAutofocusResult out m_autoFocusMode(%d) m_interenalAutoFocusMode(%d) result(%d) af_over(%d)",
        __FUNCTION__, __LINE__, m_autoFocusMode, m_interenalAutoFocusMode, ret, af_over);

    return ret;
}

int ExynosCameraActivityAutofocus::getCAFResult(void)
{
    int ret = 0;

     /*
      * 0: fail
      * 1: success
      * 2: canceled
      * 3: focusing
      * 4: restart
      */

     static int  oldRet = AUTOFOCUS_RESULT_CANCEL;
     static bool flagCAFScannigStarted = false;

     switch (m_aaAFMode) {
     case AA_AFMODE_CONTINUOUS_VIDEO:
     case AA_AFMODE_CONTINUOUS_PICTURE:
     /* case AA_AFMODE_CONTINUOUS_VIDEO_FACE: */

         switch(m_aaAfState) {
         case AA_AFSTATE_INACTIVE:
            ret = AUTOFOCUS_RESULT_CANCEL;
            break;
         case AA_AFSTATE_PASSIVE_SCAN:
         case AA_AFSTATE_ACTIVE_SCAN:
            ret = AUTOFOCUS_RESULT_FOCUSING;
            break;
         case AA_AFSTATE_PASSIVE_FOCUSED:
         case AA_AFSTATE_FOCUSED_LOCKED:
            ret = AUTOFOCUS_RESULT_SUCCESS;
            break;
         case AA_AFSTATE_PASSIVE_UNFOCUSED:
            if (flagCAFScannigStarted == true)
                ret = AUTOFOCUS_RESULT_FAIL;
            else
                ret = oldRet;
            break;
         case AA_AFSTATE_NOT_FOCUSED_LOCKED:
             ret = AUTOFOCUS_RESULT_FAIL;
             break;
         default:
            ALOGE("(%s[%d]):invalid m_aaAfState", __FUNCTION__, __LINE__);
            ret = AUTOFOCUS_RESULT_CANCEL;
            break;
         }

         if (m_aaAfState == AA_AFSTATE_ACTIVE_SCAN)
             flagCAFScannigStarted = true;
         else
             flagCAFScannigStarted = false;

         oldRet = ret;
         break;
     default:
         flagCAFScannigStarted = false;

         ret = oldRet;
         break;
     }

     return ret;
}

ExynosCameraActivityAutofocus::AUTOFOCUS_STATE ExynosCameraActivityAutofocus::afState2AUTOFOCUS_STATE(enum aa_afstate aaAfState)
{
    AUTOFOCUS_STATE autoFocusState;

    switch (aaAfState) {
    case AA_AFSTATE_INACTIVE:
        autoFocusState = AUTOFOCUS_STATE_NONE;
        break;
    case AA_AFSTATE_PASSIVE_SCAN:
    case AA_AFSTATE_ACTIVE_SCAN:
        autoFocusState = AUTOFOCUS_STATE_SCANNING;
        break;
    case AA_AFSTATE_PASSIVE_FOCUSED:
    case AA_AFSTATE_FOCUSED_LOCKED:
        autoFocusState = AUTOFOCUS_STATE_SUCCEESS;
        break;
    case AA_AFSTATE_NOT_FOCUSED_LOCKED:
    case AA_AFSTATE_PASSIVE_UNFOCUSED:
        autoFocusState = AUTOFOCUS_STATE_FAIL;
        break;
    default:
        autoFocusState = AUTOFOCUS_STATE_NONE;
        break;
    }

    return autoFocusState;
}

void ExynosCameraActivityAutofocus::m_AUTOFOCUS_MODE2AA_AFMODE(int autoFocusMode, camera2_shot_ext *shot_ext)
{
    switch (autoFocusMode) {
    case AUTOFOCUS_MODE_AUTO:

        if (m_recordingHint == true) {
            /* CONTINUOUS_VIDEO */
            shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_CONTINUOUS_VIDEO;
            shot_ext->shot.ctl.aa.vendor_afmode_option = 0x00
                | SET_BIT(AA_AFMODE_OPTION_BIT_VIDEO);
            shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_IDLE;
        } else if (m_flagFaceDetection == true) {
            /* AUTO_FACE */
            shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_AUTO;
            shot_ext->shot.ctl.aa.vendor_afmode_option = 0x00
                | SET_BIT(AA_AFMODE_OPTION_BIT_FACE);
            shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_START;
        } else {
            /* AUTO */
            shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_AUTO;
            shot_ext->shot.ctl.aa.vendor_afmode_option = 0x00;
            shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_START;
        }
        break;
    case AUTOFOCUS_MODE_INFINITY:
        /* INFINITY */
        shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_AUTO;
        shot_ext->shot.ctl.aa.vendor_afmode_option = 0x00;
        shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_CANCEL;
        break;
    case AUTOFOCUS_MODE_MACRO:
        /* MACRO */
        shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_MACRO;
        shot_ext->shot.ctl.aa.vendor_afmode_option = 0x00
            | SET_BIT(AA_AFMODE_OPTION_BIT_MACRO);
        shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_START;
        break;
    case AUTOFOCUS_MODE_EDOF:
        /* EDOF */
        shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_EDOF;
        break;
    case AUTOFOCUS_MODE_CONTINUOUS_VIDEO:
        /* CONTINUOUS_VIDEO */
        shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_CONTINUOUS_VIDEO;
        shot_ext->shot.ctl.aa.vendor_afmode_option = 0x00
            | SET_BIT(AA_AFMODE_OPTION_BIT_VIDEO);
        shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_IDLE;
        break;
    case AUTOFOCUS_MODE_CONTINUOUS_PICTURE:
    case AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
        if (m_flagFaceDetection == true) {
            /* CONTINUOUS_PICTURE_FACE */
            shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_CONTINUOUS_PICTURE;
            shot_ext->shot.ctl.aa.vendor_afmode_option = 0x00
                | SET_BIT(AA_AFMODE_OPTION_BIT_FACE);
            shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_IDLE;
        } else {
            /* CONTINUOUS_PICTURE */
            shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_CONTINUOUS_PICTURE;
            shot_ext->shot.ctl.aa.vendor_afmode_option = 0x00;
            shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_IDLE;
        }

        break;
    case AUTOFOCUS_MODE_TOUCH:
        if (m_recordingHint == true) {
            /* CONTINUOUS_VIDEO */
            shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_CONTINUOUS_VIDEO;
            shot_ext->shot.ctl.aa.vendor_afmode_option = 0x00
                | SET_BIT(AA_AFMODE_OPTION_BIT_VIDEO);
            shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_IDLE;
        } else {
            /* AUTO */
            shot_ext->shot.ctl.aa.afMode = ::AA_AFMODE_AUTO;
            shot_ext->shot.ctl.aa.vendor_afmode_option = 0x00;
            shot_ext->shot.ctl.aa.afTrigger = AA_AF_TRIGGER_START;
        }

        break;
    case AUTOFOCUS_MODE_FIXED:
        break;
    default:
        ALOGE("ERR(%s):invalid focus mode (%d)", __FUNCTION__, autoFocusMode);
        break;
    }
}

} /* namespace android */
