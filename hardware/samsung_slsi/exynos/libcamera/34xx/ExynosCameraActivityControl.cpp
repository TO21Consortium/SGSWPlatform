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
#define LOG_TAG "ExynosCameraActivityControl"

#include "ExynosCameraActivityControl.h"

namespace android {

ExynosCameraActivityControl::ExynosCameraActivityControl(__unused int cameraId)
{
    flagAutoFocusRunning = false;
    touchAFMode = false;
    touchAFModeForFlash = false;

    m_autofocusMgr = new ExynosCameraActivityAutofocus();
    m_flashMgr = new ExynosCameraActivityFlash();
    m_specialCaptureMgr = new ExynosCameraActivitySpecialCapture();
    m_uctlMgr = new ExynosCameraActivityUCTL();
    m_focusMode = FOCUS_MODE_AUTO;
    m_fpsValue = 1;
    m_halVersion = IS_HAL_VER_1_0;
}

ExynosCameraActivityControl::~ExynosCameraActivityControl()
{
    if (m_autofocusMgr != NULL) {
        delete m_autofocusMgr;
        m_autofocusMgr = NULL;
    }

    if (m_flashMgr != NULL) {
        delete m_flashMgr;
        m_flashMgr = NULL;
    }

    if (m_specialCaptureMgr != NULL) {
        delete m_specialCaptureMgr;
        m_specialCaptureMgr = NULL;
    }

    if (m_uctlMgr != NULL) {
        delete m_uctlMgr;
        m_uctlMgr = NULL;
    }
}

bool ExynosCameraActivityControl::autoFocus(int focusMode, int focusType)
{
    bool ret = true;
    int newfocusMode = 0;
    int currentAutofocusState = ExynosCameraActivityAutofocus::AUTOFOCUS_STATE_NONE;
    int newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_BASE;
    int oldMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_BASE;
    bool flagAutoFocusTringger = false;
    bool flagPreFlash = false;
    bool flagLockFocus = false;

    if (focusType == AUTO_FOCUS_SERVICE) {
        m_flashMgr->updateAeState();
    }
    ALOGD("DEBUG(%s[%d]):NeedCaptureFlash=%d", __FUNCTION__, __LINE__, m_flashMgr->getNeedCaptureFlash());
    if (m_flashMgr->getNeedCaptureFlash() == true) {
        /* start Pre-flash */
        if (startPreFlash(this->getAutoFocusMode()) != NO_ERROR)
            ALOGE("ERR(%s[%d]):start Pre-flash Fail", __FUNCTION__, __LINE__);
    }

    if (focusType == AUTO_FOCUS_HAL)
        newfocusMode = FOCUS_MODE_AUTO;
    else
        newfocusMode = this->getAutoFocusMode();

    oldMgrAutofocusMode = m_autofocusMgr->getAutofocusMode();
    ALOGD("DEBUG(%s[%d]):newfocusMode=%d", __FUNCTION__, __LINE__, newfocusMode);

    switch (newfocusMode) {
    case FOCUS_MODE_AUTO:
        if (touchAFMode == true)
            newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_TOUCH;
        else
            newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_AUTO;
        break;
    case FOCUS_MODE_INFINITY:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_INFINITY;
        break;
    case FOCUS_MODE_MACRO:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_MACRO;
        break;
    case FOCUS_MODE_CONTINUOUS_VIDEO:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_VIDEO;
        break;
    case FOCUS_MODE_CONTINUOUS_PICTURE:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE;
        break;
    case FOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO;
        break;
    case FOCUS_MODE_TOUCH:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_TOUCH;
        break;
    case FOCUS_MODE_FIXED:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_FIXED;
        break;
    case FOCUS_MODE_EDOF:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_EDOF;
        break;
    default:
        ALOGE("ERR(%s):Unsupported focusMode=%d", __FUNCTION__, newfocusMode);
        return false;
        break;
    }
    if (focusMode == newfocusMode) {
        switch (newMgrAutofocusMode) {
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_FIXED:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_EDOF:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_VIDEO:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
            flagLockFocus = true;
            break;
        default:
            break;
        }
    }

    /*
    * Applications can call autoFocus(AutoFocusCallback) in this mode.
    * If the autofocus is in the middle of scanning,
    * the focus callback will return when it completes
    * If the autofocus is not scanning,
    * the focus callback will immediately return with a boolean
    * that indicates whether the focus is sharp or not.
    */

    /*
     * But, When Continuous af is running,
     * auto focus api can be triggered,
     * and then, af will be lock. (af lock)
     */
    switch (newMgrAutofocusMode) {
    case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_FIXED:
    case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_EDOF:
    case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_VIDEO:
    case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE:
    case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
        flagAutoFocusTringger = false;
        break;
    default:
        switch(oldMgrAutofocusMode) {
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_FIXED:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_EDOF:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_VIDEO:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
            flagAutoFocusTringger = true;
            break;
        default:
            if (oldMgrAutofocusMode == newMgrAutofocusMode) {
                currentAutofocusState = m_autofocusMgr->getCurrentState();

                if (currentAutofocusState != ExynosCameraActivityAutofocus::AUTOFOCUS_STATE_SCANNING)
                    flagAutoFocusTringger = true;
                else
                    flagAutoFocusTringger = false;
            } else {
                flagAutoFocusTringger = true;
            }
            break;
        }
        break;
    }

    if (flagAutoFocusTringger == true) {
        if (m_autofocusMgr->flagAutofocusStart() == true)
            m_autofocusMgr->stopAutofocus();

        m_autofocusMgr->setAutofocusMode(newMgrAutofocusMode);

        m_autofocusMgr->startAutofocus();
    } else {
        m_autofocusMgr->setAutofocusMode(newMgrAutofocusMode);

        switch (newMgrAutofocusMode) {
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_FIXED:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_EDOF:
            ret = false;
            goto done;
            break;
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_VIDEO:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
            currentAutofocusState = m_autofocusMgr->getCurrentState();

            if (m_autofocusMgr->flagLockAutofocus() == true &&
                currentAutofocusState != ExynosCameraActivityAutofocus::AUTOFOCUS_STATE_SUCCEESS) {
                /* for make it fail */
                currentAutofocusState = ExynosCameraActivityAutofocus::AUTOFOCUS_STATE_FAIL;
            }

            if (currentAutofocusState == ExynosCameraActivityAutofocus::AUTOFOCUS_STATE_SUCCEESS) {
                ret = true;
                goto done;
            } else if (currentAutofocusState == ExynosCameraActivityAutofocus::AUTOFOCUS_STATE_FAIL) {
                ret = false;
                goto done;
            }

            break;
        default:
            break;
        }
    }

    ret = m_autofocusMgr->getAutofocusResult(flagLockFocus);

done :
    /* The focus position is locked after autoFocus call with CAF
    * But, the focus position is not locked when cancelAutoFocus after TAF is called
    * CF) If cancelAutoFocus is called when TAF, the focusMode is changed from TAF to CAF */

    if (flagLockFocus) {
            m_autofocusMgr->lockAutofocus();
    }

    /* Stop pre flash */
    stopPreFlash();

    return ret;
}

bool ExynosCameraActivityControl::cancelAutoFocus(void)
{
    /*
     * Cancels any auto-focus function in progress.
     * Whether or not auto-focus is currently in progress,
     * this function will return the focus position to the default.
     * If the camera does not support auto-focus, this is a no-op.
     */

    touchAFMode = false;
    touchAFModeForFlash = false;
    int mode = m_autofocusMgr->getAutofocusMode();

    switch (m_autofocusMgr->getAutofocusMode()) {
    /*  If applications want to resume the continuous focus, cancelAutoFocus must be called */
    case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_VIDEO:
    case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE:
    case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
        if (m_autofocusMgr->flagLockAutofocus() == true) {
            m_autofocusMgr->unlockAutofocus();

            if (m_autofocusMgr->flagAutofocusStart() == false)
                m_autofocusMgr->startAutofocus();
        }
        break;
    default:
        if (m_autofocusMgr->flagLockAutofocus() == true)
            m_autofocusMgr->unlockAutofocus();

        if (m_autofocusMgr->flagAutofocusStart() == true)
            m_autofocusMgr->stopAutofocus();

        break;
    }

    enum ExynosCameraActivityFlash::FLASH_TRIGGER triggerPath;
    m_flashMgr->getFlashTrigerPath(&triggerPath);

    if ((triggerPath == ExynosCameraActivityFlash::FLASH_TRIGGER_LONG_BUTTON) ||
         ((m_flashMgr->getNeedCaptureFlash() == true) && (m_flashMgr->getFlashStatus()!= AA_FLASHMODE_OFF)))
         this->cancelFlash();
     else
         m_flashMgr->setFlashStep(ExynosCameraActivityFlash::FLASH_STEP_OFF);
     m_flashMgr->setFlashTrigerPath(ExynosCameraActivityFlash::FLASH_TRIGGER_OFF);

    return true;
}

void ExynosCameraActivityControl::setAutoFocusMode(int focusMode)
{
    int newMgrAutofocusMode = 0;
    int oldMgrAutofocusMode = m_autofocusMgr->getAutofocusMode();

    m_focusMode = focusMode;

    ALOGD("DEBUG(%s[%d]):m_focusMode=%d", __FUNCTION__, __LINE__, m_focusMode);

    switch (m_focusMode) {
    case FOCUS_MODE_INFINITY:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_INFINITY;
        break;
    case FOCUS_MODE_FIXED:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_FIXED;
        break;
    case FOCUS_MODE_EDOF:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_EDOF;
        break;
    case FOCUS_MODE_CONTINUOUS_VIDEO:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_VIDEO;
        break;
    case FOCUS_MODE_CONTINUOUS_PICTURE:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE;
        break;
    case FOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
        newMgrAutofocusMode = ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO;
        break;
    default:
        break;
    }

    ALOGD("DEBUG(%s[%d]):newMgrAutofocusMode=%d, oldMgrAutofocusMode=%d",
           __FUNCTION__, __LINE__, newMgrAutofocusMode, oldMgrAutofocusMode);

    if (oldMgrAutofocusMode != newMgrAutofocusMode) {
        if (m_autofocusMgr->flagLockAutofocus() == true)
            m_autofocusMgr->unlockAutofocus();
    }

    if (newMgrAutofocusMode) { /* Continuous autofocus, infinity, fixed ... */
        /*
         * If applications want to resume the continuous focus,
         * cancelAutoFocus must be called.
         * Restarting the preview will not resume the continuous autofocus.
         * To stop continuous focus, applications should change the focus mode to other modes.
         */
        bool flagRestartAutofocus = false;

        if (oldMgrAutofocusMode == newMgrAutofocusMode) {
            if (m_autofocusMgr->flagAutofocusStart() == false &&
                m_autofocusMgr->flagLockAutofocus() == false)
                flagRestartAutofocus = true;
            else
                flagRestartAutofocus = false;
        } else {
            flagRestartAutofocus = true;
        }

        if (flagRestartAutofocus == true &&
            m_autofocusMgr->flagAutofocusStart() == true)
            m_autofocusMgr->stopAutofocus();

        if (oldMgrAutofocusMode != newMgrAutofocusMode)
            if (m_autofocusMgr->setAutofocusMode(newMgrAutofocusMode) == false)
                ALOGE("ERR(%s):setAutofocusMode fail", __FUNCTION__);

        if (flagRestartAutofocus == true)
            m_autofocusMgr->startAutofocus();

        enum ExynosCameraActivityFlash::FLASH_TRIGGER triggerPath;
        m_flashMgr->getFlashTrigerPath(&triggerPath);

        if ((triggerPath == ExynosCameraActivityFlash::FLASH_TRIGGER_LONG_BUTTON) ||
            ((m_flashMgr->getNeedCaptureFlash() == true) && (m_flashMgr->getFlashStatus() != AA_FLASHMODE_OFF))) {
            this->cancelFlash();
        } else {
            m_flashMgr->setFlashStep(ExynosCameraActivityFlash::FLASH_STEP_OFF);
        }

        m_flashMgr->setFlashTrigerPath(ExynosCameraActivityFlash::FLASH_TRIGGER_OFF);

    } else { /* single autofocus */
        switch (oldMgrAutofocusMode) {
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_INFINITY:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_FIXED:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_EDOF:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_VIDEO:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE:
        case ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO:
            if (m_autofocusMgr->flagAutofocusStart() == true)
                m_autofocusMgr->stopAutofocus();

            break;
        default:
            break;
        }
    }
}

int ExynosCameraActivityControl::getAutoFocusMode(void)
{
    return m_focusMode;
}

void ExynosCameraActivityControl::setAutoFcousArea(ExynosRect2 rect, int weight)
{
    m_autofocusMgr->setFocusAreas(rect, weight);
}

void ExynosCameraActivityControl::setAutoFocusMacroPosition(
        int oldAutoFocusMacroPosition,
        int autoFocusMacroPosition)
{
    int macroPosition = ExynosCameraActivityAutofocus::AUTOFOCUS_MACRO_POSITION_BASE;

    switch (autoFocusMacroPosition) {
    case 1:
        macroPosition = ExynosCameraActivityAutofocus::AUTOFOCUS_MACRO_POSITION_CENTER;
        break;
    case 2:
        macroPosition = ExynosCameraActivityAutofocus::AUTOFOCUS_MACRO_POSITION_CENTER_UP;
        break;
    default:
        break;
    }

    m_autofocusMgr->setMacroPosition(macroPosition);

    /* if macro option change, we need to restart CAF */
    if (oldAutoFocusMacroPosition != autoFocusMacroPosition) {
        if ((m_autofocusMgr->getAutofocusMode() == ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE ||
            m_autofocusMgr->getAutofocusMode() == ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_VIDEO ||
            m_autofocusMgr->getAutofocusMode() == ExynosCameraActivityAutofocus::AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO) &&
            m_autofocusMgr->flagAutofocusStart() == true &&
            m_autofocusMgr->flagLockAutofocus() == false) {
            m_autofocusMgr->stopAutofocus();
            m_autofocusMgr->startAutofocus();
        }
    }
}

int ExynosCameraActivityControl::getCAFResult(void)
{
    return m_autofocusMgr->getCAFResult();
}

void ExynosCameraActivityControl::stopAutoFocus(void)
{
    if (m_autofocusMgr->flagLockAutofocus() == true)
        m_autofocusMgr->unlockAutofocus();
    if (m_autofocusMgr->flagAutofocusStart() == true)
        m_autofocusMgr->stopAutofocus();
}

bool ExynosCameraActivityControl::setFlashMode(int flashMode)
{
    enum flash_mode m_flashMode;
    enum aa_ae_flashmode aeflashMode;

    switch (flashMode) {
    case FLASH_MODE_OFF:
        m_flashMode   = ::CAM2_FLASH_MODE_OFF;
        aeflashMode = ::AA_FLASHMODE_OFF;

        m_flashMgr->setFlashReq(ExynosCameraActivityFlash::FLASH_REQ_OFF);
        break;
    case FLASH_MODE_AUTO:
        m_flashMode   = ::CAM2_FLASH_MODE_SINGLE;
        /* aeflashMode = ::AA_FLASHMODE_AUTO; */
        aeflashMode = ::AA_FLASHMODE_CAPTURE;

        m_flashMgr->setFlashReq(ExynosCameraActivityFlash::FLASH_REQ_AUTO);
        break;
    case FLASH_MODE_ON:
        m_flashMode   = ::CAM2_FLASH_MODE_SINGLE;
        /* aeflashMode = ::AA_FLASHMODE_ON; */
        aeflashMode = ::AA_FLASHMODE_ON_ALWAYS;

        m_flashMgr->setFlashReq(ExynosCameraActivityFlash::FLASH_REQ_ON);
        break;
    case FLASH_MODE_TORCH:
        m_flashMode   = ::CAM2_FLASH_MODE_TORCH;
        aeflashMode = ::AA_FLASHMODE_ON_ALWAYS;

        m_flashMgr->setFlashReq(ExynosCameraActivityFlash::FLASH_REQ_TORCH);
        break;
    case FLASH_MODE_RED_EYE:
    default:
        ALOGE("ERR(%s):Unsupported value(%d)", __FUNCTION__, flashMode);
        return false;
        break;
    }
    return true;
}

status_t ExynosCameraActivityControl::startPreFlash(int focusMode)
{
    enum ExynosCameraActivityFlash::FLASH_TRIGGER triggerPath;
    bool flagPreFlash = false;
    int ret = NO_ERROR;

    if (m_flashMgr->getFlashWaitCancel() == true)
        m_flashMgr->setFlashWaitCancel(false);

    m_flashMgr->setCaptureStatus(true);
    m_flashMgr->setMainFlashFiring(false);
    m_flashMgr->getFlashTrigerPath(&triggerPath);

    if (touchAFModeForFlash == false && triggerPath != ExynosCameraActivityFlash::FLASH_TRIGGER_SHORT_BUTTON) {
        if (focusMode == FOCUS_MODE_AUTO) {
           m_flashMgr->setFlashTrigerPath(ExynosCameraActivityFlash::FLASH_TRIGGER_LONG_BUTTON);
           ALOGD("DEBUG(%s):FLASH_TRIGGER_LONG_BUTTON !!!", __FUNCTION__);
           flagPreFlash = true;
        } else {
           m_flashMgr->setFlashTrigerPath(ExynosCameraActivityFlash::FLASH_TRIGGER_SHORT_BUTTON);
           ALOGD("DEBUG(%s):FLASH_TRIGGER_SHORT_BUTTON !!!", __FUNCTION__);
           flagPreFlash = false;
        }
    } else {
        m_flashMgr->setFlashTrigerPath(ExynosCameraActivityFlash::FLASH_TRIGGER_TOUCH_DISPLAY);
        ALOGD("DEBUG(%s):FLASH_TRIGGER_TOUCH_DISPLAY !!!", __FUNCTION__);
        flagPreFlash = true;
    }

    if (flagPreFlash == true) {
        m_flashMgr->setFlashStep(ExynosCameraActivityFlash::FLASH_STEP_PRE_START);

        if (m_flashMgr->waitAeDone() == false) {
            ALOGE("ERR(%s):waitAeDone() fail", __FUNCTION__);
            ret = INVALID_OPERATION;
        }
    }

    return ret;
}

bool ExynosCameraActivityControl::flagFocusing(struct camera2_shot_ext *shot_ext, int focusMode)
{
    bool ret = false;

    if (shot_ext == NULL) {
        ALOGE("ERR(%s):shot_ext === NULL", __func__);
        return false;
    }

    switch (focusMode) {
    case FOCUS_MODE_INFINITY:
    case FOCUS_MODE_MACRO:
    case FOCUS_MODE_FIXED:
    case FOCUS_MODE_EDOF:
    case FOCUS_MODE_CONTINUOUS_VIDEO:
        return false;
    default:
        break;
    }

    ExynosCameraActivityAutofocus::AUTOFOCUS_STATE autoFocusState = m_autofocusMgr->afState2AUTOFOCUS_STATE(shot_ext->shot.dm.aa.afState);
    switch(autoFocusState) {
    case ExynosCameraActivityAutofocus::AUTOFOCUS_STATE_SCANNING:
        ret = true;
        break;
    case ExynosCameraActivityAutofocus::AUTOFOCUS_STATE_NONE:
    case ExynosCameraActivityAutofocus::AUTOFOCUS_STATE_SUCCEESS:
    case ExynosCameraActivityAutofocus::AUTOFOCUS_STATE_FAIL:
        ret = false;
        break;
    default:
        ALOGD("DEBUG(%s):invalid autoFocusState(%d)", __func__, autoFocusState);
        ret = false;
        break;
    }

    return ret;
}

void ExynosCameraActivityControl::stopPreFlash(void)
{
    enum ExynosCameraActivityFlash::FLASH_STEP flashStep;
    m_flashMgr->getFlashStep(&flashStep);

    if (flashStep == ExynosCameraActivityFlash::FLASH_STEP_PRE_START)
    m_flashMgr->setFlashStep(ExynosCameraActivityFlash::FLASH_STEP_PRE_DONE);
}

bool ExynosCameraActivityControl::waitFlashMainReady()
{
    int totalWaitingTime = 0;

    while ((totalWaitingTime <= FLASH_WAITING_SLEEP_TIME * 30) && (m_flashMgr->checkPreFlash() == false)) {
        usleep(FLASH_WAITING_SLEEP_TIME);
        totalWaitingTime += FLASH_WAITING_SLEEP_TIME;
    }

    if (FLASH_WAITING_SLEEP_TIME * 30 < totalWaitingTime) {
        ALOGE("ERR(%s):waiting too much (%d msec)", __FUNCTION__, totalWaitingTime);
    }

    return m_flashMgr->waitMainReady();
}

int ExynosCameraActivityControl::startMainFlash(void)
{
    int totalWaitingTime = 0;
    int waitCount = 0;
    unsigned int shotFcount = 0;
    enum ExynosCameraActivityFlash::FLASH_TRIGGER triggerPath;
    enum ExynosCameraActivityFlash::FLASH_STEP flashStep;

    m_flashMgr->getFlashTrigerPath(&triggerPath);

    /* check preflash to prevent FW lock-up */
    do {
        usleep(FLASH_WAITING_SLEEP_TIME);
        totalWaitingTime += FLASH_WAITING_SLEEP_TIME;
    } while ((totalWaitingTime <= FLASH_WAITING_SLEEP_TIME * 30) && (m_flashMgr->checkPreFlash() == false));

    if (m_flashMgr->checkPreFlash() == false) {
        ALOGD("DEBUG(%s): preflash is not done : m_flashMgr->checkPreFlash(%d)", __FUNCTION__, m_flashMgr->checkPreFlash());
        return -1;
    }

    if (m_flashMgr->waitMainReady() == false)
        ALOGW("WARN(%s):waitMainReady() timeout", __FUNCTION__);

    if (m_flashMgr->getFlashWaitCancel() == true) {
        ALOGD("DEBUG(%s): getFlashWaitCancel(%d)", __FUNCTION__, m_flashMgr->getFlashWaitCancel());
        return -1;
    }

    m_flashMgr->getFlashStep(&flashStep);
    if (flashStep == ExynosCameraActivityFlash::FLASH_STEP_OFF) {
        ALOGD("DEBUG(%s): getFlashStep is changed FLASH_STEP_OFF", __FUNCTION__);
        return -1;
    }
    m_flashMgr->setMainFlashFiring(true);
    m_flashMgr->setFlashStep(ExynosCameraActivityFlash::FLASH_STEP_MAIN_START);

    /* get best shot frame count */
    totalWaitingTime = 0;
    waitCount = 0;
    m_flashMgr->resetShotFcount();
    do {
        waitCount = m_flashMgr->getWaitingCount();
        if (0 < waitCount) {
            usleep(FLASH_WAITING_SLEEP_TIME);
            totalWaitingTime += FLASH_WAITING_SLEEP_TIME;
        }
    } while (0 < waitCount && totalWaitingTime <= FLASH_MAX_WAITING_TIME);

    if (0 < waitCount || FLASH_MAX_WAITING_TIME < totalWaitingTime)
        ALOGE("ERR(%s):waiting too much (%d msec)", __FUNCTION__, totalWaitingTime);

    shotFcount = m_flashMgr->getShotFcount();

    return shotFcount;
}

void ExynosCameraActivityControl::stopMainFlash(void)
{
    m_flashMgr->setCaptureStatus(false);
    m_flashMgr->setMainFlashFiring(false);

    m_flashMgr->setFlashTrigerPath(ExynosCameraActivityFlash::FLASH_TRIGGER_OFF);
    m_flashMgr->setFlashStep(ExynosCameraActivityFlash::FLASH_STEP_OFF);
}

void ExynosCameraActivityControl::cancelFlash(void)
{
    ALOGI("INFO(%s[%d])", __FUNCTION__, __LINE__);

    m_flashMgr->setFlashStep(ExynosCameraActivityFlash::FLASH_STEP_CANCEL);
    m_flashMgr->setFlashWaitCancel(true);
}

void ExynosCameraActivityControl::setHdrMode(bool hdrMode)
{
    if (hdrMode)
        m_specialCaptureMgr->setCaptureMode(ExynosCameraActivitySpecialCapture::SCAPTURE_MODE_HDR);
    else
        m_specialCaptureMgr->setCaptureMode(ExynosCameraActivitySpecialCapture::SCAPTURE_MODE_NONE);
}

int ExynosCameraActivityControl::getHdrFcount(int index)
{
    int startFcount = 0;
    int totalWaitingTime = 0;
    unsigned int shotFcount = 0;

    do {
        startFcount = m_specialCaptureMgr->getHdrStartFcount(index);
        if (startFcount == 0) {
            usleep(HDR_WAITING_SLEEP_TIME);
            totalWaitingTime += HDR_WAITING_SLEEP_TIME;
        }
    } while (startFcount == 0 && totalWaitingTime <= HDR_MAX_WAITING_TIME);

    if (startFcount == 0 || totalWaitingTime >= HDR_MAX_WAITING_TIME)
        ALOGE("ERR(%s):waiting too much (%d msec)", __FUNCTION__, totalWaitingTime);

    shotFcount = startFcount + m_specialCaptureMgr->getHdrWaitFcount();

    return shotFcount;
}

void ExynosCameraActivityControl::activityBeforeExecFunc(int pipeId, void *args)
{
    switch(pipeId) {
    case PIPE_FLITE:
        m_autofocusMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SENSOR_BEFORE, args);
        m_flashMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SENSOR_BEFORE, args);
        m_specialCaptureMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SENSOR_BEFORE, args);
        m_uctlMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SENSOR_BEFORE, args);
        break;
    case PIPE_3AA:
    case PIPE_3AA_ISP:
        if (m_halVersion != IS_HAL_VER_3_2) {
            m_autofocusMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_3A_BEFORE, args);
            m_flashMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_3A_BEFORE, args);
        } else {
            m_flashMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_3A_BEFORE_HAL3, args);
        }
        m_specialCaptureMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_3A_BEFORE, args);
        /* Added it for FD-AF. 2015/05/04*/
    case PIPE_VRA:
        m_uctlMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_3A_BEFORE, args);
        break;
    /* to set metadata of ISP buffer */
    case PIPE_POST_3AA_ISP:
        m_uctlMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_3A_BEFORE, args);
        break;
    /* TO DO : seperate 3aa & isp pipe */
/*
    case PIPE_3AA:
    case PIPE_3AA_ISP:
        m_autofocusMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_ISP_BEFORE, args);
        m_flashMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_ISP_BEFORE, args);
        m_specialCaptureMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_ISP_BEFORE, args);
        break;
*/
    case PIPE_ISPC:
    case PIPE_SCC:
        if (m_halVersion != IS_HAL_VER_3_2)
        m_autofocusMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCC_BEFORE, args);
        m_flashMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCC_BEFORE, args);
        m_specialCaptureMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCC_BEFORE, args);
        m_uctlMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCC_BEFORE, args);
        break;
    case PIPE_SCP:
        if (m_halVersion != IS_HAL_VER_3_2)
        m_autofocusMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCP_BEFORE, args);
        m_flashMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCP_BEFORE, args);
        m_specialCaptureMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCP_BEFORE, args);
        m_uctlMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCP_BEFORE, args);
        break;
    /* HW FD Orientation is enabled 2014/04/28 */
    case PIPE_ISP:
        m_uctlMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_3A_BEFORE, args);
        break;
    default:
        break;
    }
}

void ExynosCameraActivityControl::activityAfterExecFunc(int pipeId, void *args)
{
    switch(pipeId) {
    case PIPE_FLITE:
        m_autofocusMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SENSOR_AFTER, args);
        m_flashMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SENSOR_AFTER, args);
        m_specialCaptureMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SENSOR_AFTER, args);
        m_uctlMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SENSOR_AFTER, args);
        break;
    case PIPE_3AA:
    case PIPE_3AA_ISP:
        if (m_halVersion != IS_HAL_VER_3_2) {
        m_autofocusMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_3A_AFTER, args);
        m_flashMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_3A_AFTER, args);
        } else {
            m_flashMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_3A_AFTER_HAL3, args);
        }
        m_specialCaptureMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_3A_AFTER, args);
        break;
    case PIPE_POST_3AA_ISP:
        m_uctlMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_3A_AFTER, args);
        break;
    /* TO DO : seperate 3aa & isp pipe
    case PIPE_3AA:
    case PIPE_3AA_ISP:
        m_autofocusMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_ISP_AFTER, args);
        m_flashMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_ISP_AFTER, args);
        m_specialCaptureMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_ISP_AFTER, args);
        break;
    */
    case PIPE_ISPC:
    case PIPE_SCC:
        m_autofocusMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCC_AFTER, args);
        m_flashMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCC_AFTER, args);
        m_specialCaptureMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCC_AFTER, args);
        m_uctlMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCC_AFTER, args);
        break;
    case PIPE_SCP:
        m_autofocusMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCP_AFTER, args);
        m_flashMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCP_AFTER, args);
        m_specialCaptureMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCP_AFTER, args);
        m_uctlMgr->execFunction(ExynosCameraActivityBase::CALLBACK_TYPE_SCP_AFTER, args);
        break;
    default:
        break;
    }
}

ExynosCameraActivityFlash *ExynosCameraActivityControl::getFlashMgr(void)
{
    return m_flashMgr;
}

ExynosCameraActivitySpecialCapture *ExynosCameraActivityControl::getSpecialCaptureMgr(void)
{
    return m_specialCaptureMgr;
}

ExynosCameraActivityAutofocus*ExynosCameraActivityControl::getAutoFocusMgr(void)
{
    return m_autofocusMgr;
}

ExynosCameraActivityUCTL*ExynosCameraActivityControl::getUCTLMgr(void)
{
    return m_uctlMgr;
}

void ExynosCameraActivityControl::setFpsValue(int fpsValue)
{
    m_fpsValue = fpsValue;

    m_autofocusMgr->setFpsValue(m_fpsValue);
    m_flashMgr->setFpsValue(m_fpsValue);

    ALOGI("INFO(%s[%d]): m_fpsValue(%d)", __FUNCTION__, __LINE__, m_fpsValue);
}

int ExynosCameraActivityControl::getFpsValue()
{
    return m_fpsValue;
}

void ExynosCameraActivityControl::setHalVersion(int halVersion)
{
    m_halVersion = halVersion;
    ALOGI("INFO(%s[%d]): m_halVersion(%d)", __FUNCTION__, __LINE__, m_halVersion);

    return;
}
}; /* namespace android */

