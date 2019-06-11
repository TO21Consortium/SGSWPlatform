/*
**
** Copyright 2012, Samsung Electronics Co. LTD
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
#define LOG_TAG "ExynosCameraActivitySpecialCapture"
#include <cutils/log.h>

#include "ExynosCameraActivitySpecialCapture.h"
//#include "ExynosCamera.h"

#define TIME_CHECK 1

namespace android {

class ExynosCamera;

ExynosCameraActivitySpecialCapture::ExynosCameraActivitySpecialCapture()
{
    t_isExclusiveReq = false;
    t_isActivated = false;
    t_reqNum = 0x1F;
    t_reqStatus = 0;

    m_hdrFcount = 0;
    m_currentInputFcount = 0;
    m_backupAeExpCompensation = 0;
    m_hdrStartFcount[0] = 0;
    m_hdrStartFcount[1] = 0;
    m_hdrStartFcount[2] = 0;
    m_hdrDropFcount[0] = 0;
    m_hdrDropFcount[1] = 0;
    m_hdrDropFcount[2] = 0;
    m_delay = 0;
    m_specialCaptureMode = SCAPTURE_MODE_NONE;
    m_check = false;
    m_specialCaptureStep = SCAPTURE_STEP_OFF;
    m_backupSceneMode = AA_SCENE_MODE_DISABLED;
    m_backupAaMode = AA_CONTROL_OFF;
    m_backupAeLock = AA_AE_LOCK_OFF;
    memset(m_backupAeTargetFpsRange, 0x00, sizeof(m_backupAeTargetFpsRange));
    m_backupFrameDuration = 0L;
#ifdef RAWDUMP_CAPTURE
    m_RawCaptureFcount = 0;
#endif
    memset(m_hdrBuffer, 0x00, sizeof(m_hdrBuffer));

}

ExynosCameraActivitySpecialCapture::~ExynosCameraActivitySpecialCapture()
{
    t_isExclusiveReq = false;
    t_isActivated = false;
    t_reqNum = 0x1F;
    t_reqStatus = 0;

    m_hdrFcount = 0;
    m_currentInputFcount = 0;
    m_backupAeExpCompensation = 0;
    m_hdrStartFcount[0] = 0;
    m_hdrStartFcount[1] = 0;
    m_hdrStartFcount[2] = 0;
    m_hdrDropFcount[0] = 0;
    m_hdrDropFcount[1] = 0;
    m_hdrDropFcount[2] = 0;
    m_delay = 0;
    m_specialCaptureMode = SCAPTURE_MODE_NONE;
    m_check = false;
}

int ExynosCameraActivitySpecialCapture::t_funcNull(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;

    return 1;
}

int ExynosCameraActivitySpecialCapture::t_funcSensorBefore(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);

    return 1;
}

int ExynosCameraActivitySpecialCapture::t_funcSensorAfter(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);
    int ret = 1;

    if (shot_ext != NULL && m_specialCaptureMode == SCAPTURE_MODE_HDR) {
        if (m_hdrDropFcount[2] + 1 == shot_ext->shot.dm.request.frameCount) {
            ret = 2;

            ALOGD("DEBUG(%s[%d]):(%d / B_LOCK %d)", __FUNCTION__, __LINE__, m_hdrStartFcount[0], shot_ext->shot.dm.request.frameCount);
        }

        if (m_hdrDropFcount[2] + 2 == shot_ext->shot.dm.request.frameCount) {
            ret = 3;

            ALOGD("DEBUG(%s[%d]):(%d / B_LOCK %d)", __FUNCTION__, __LINE__, m_hdrStartFcount[1], shot_ext->shot.dm.request.frameCount);
        }

        if (m_hdrDropFcount[2]  + 3 == shot_ext->shot.dm.request.frameCount) {
            ret = 4;

            ALOGD("DEBUG(%s[%d]):(%d / B_LOCK %d)", __FUNCTION__, __LINE__, m_hdrStartFcount[2], shot_ext->shot.dm.request.frameCount);
        }
    }

    return ret;
}

int ExynosCameraActivitySpecialCapture::t_funcISPBefore(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);

done:
    return 1;
}

int ExynosCameraActivitySpecialCapture::t_funcISPAfter(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);

    return 1;
}

int ExynosCameraActivitySpecialCapture::t_func3ABefore(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);

    if (shot_ext != NULL && m_specialCaptureMode == SCAPTURE_MODE_HDR) {
        m_currentInputFcount = shot_ext->shot.dm.request.frameCount;

        /* HACK UNLOCK AE */
#ifndef USE_LSI_3A
        shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;
#endif

        if (m_specialCaptureStep == SCAPTURE_STEP_START) {
            m_backupAeExpCompensation = shot_ext->shot.ctl.aa.aeExpCompensation;
            m_backupAaMode = shot_ext->shot.ctl.aa.mode;
            m_backupAeLock = shot_ext->shot.ctl.aa.aeLock;
            m_backupSceneMode = shot_ext->shot.ctl.aa.sceneMode;

            m_specialCaptureStep = SCAPTURE_STEP_MINUS_SET;
            ALOGD("DEBUG(%s[%d]):SCAPTURE_STEP_START", __FUNCTION__, __LINE__);
        } else if (m_specialCaptureStep == SCAPTURE_STEP_MINUS_SET) {
            shot_ext->shot.ctl.aa.mode = AA_CONTROL_USE_SCENE_MODE;
            shot_ext->shot.ctl.aa.sceneMode = AA_SCENE_MODE_HDR;
            shot_ext->shot.ctl.aa.aeLock = AA_AE_LOCK_OFF;
#ifdef USE_LSI_3A
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;
#endif
            m_specialCaptureStep = SCAPTURE_STEP_ZERO_SET;
            ALOGD("DEBUG(%s[%d]):SCAPTURE_STEP_MINUS_SET", __FUNCTION__, __LINE__);
        } else if (m_specialCaptureStep == SCAPTURE_STEP_ZERO_SET) {
            m_delay = 0;
            shot_ext->shot.ctl.aa.mode = AA_CONTROL_USE_SCENE_MODE;
            shot_ext->shot.ctl.aa.sceneMode = AA_SCENE_MODE_HDR;
            shot_ext->shot.ctl.aa.aeLock = AA_AE_LOCK_OFF;
#ifdef USE_LSI_3A
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;
#endif
            m_specialCaptureStep = SCAPTURE_STEP_PLUS_SET;
            ALOGD("DEBUG(%s[%d]):SCAPTURE_STEP_ZERO_SET", __FUNCTION__, __LINE__);
        } else if (m_specialCaptureStep == SCAPTURE_STEP_PLUS_SET) {
            shot_ext->shot.ctl.aa.mode = AA_CONTROL_USE_SCENE_MODE;
            shot_ext->shot.ctl.aa.sceneMode = AA_SCENE_MODE_HDR;
            shot_ext->shot.ctl.aa.aeLock = AA_AE_LOCK_OFF;
#ifdef USE_LSI_3A
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;
#endif

            m_specialCaptureStep = SCAPTURE_STEP_RESTORE;
            ALOGD("DEBUG(%s[%d]):SCAPTURE_STEP_PLUS_SET", __FUNCTION__, __LINE__);
        } else if (m_specialCaptureStep == SCAPTURE_STEP_RESTORE) {
            shot_ext->shot.ctl.aa.mode = AA_CONTROL_USE_SCENE_MODE;
            shot_ext->shot.ctl.aa.sceneMode = AA_SCENE_MODE_HDR;
            shot_ext->shot.ctl.aa.aeLock = m_backupAeLock;
            shot_ext->shot.ctl.aa.aeExpCompensation = m_backupAeExpCompensation;
#ifdef USE_LSI_3A
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;
#endif

            m_specialCaptureStep = SCAPTURE_STEP_WAIT_CAPTURE_DELAY;
            ALOGD("DEBUG(%s[%d]):SCAPTURE_STEP_RESTORE", __FUNCTION__, __LINE__);
        } else if (m_specialCaptureStep == SCAPTURE_STEP_WAIT_CAPTURE_DELAY) {
            shot_ext->shot.ctl.aa.sceneMode = m_backupSceneMode;
            shot_ext->shot.ctl.aa.mode = m_backupAaMode;
            shot_ext->shot.ctl.aa.aeLock = m_backupAeLock;
            shot_ext->shot.ctl.aa.aeExpCompensation = m_backupAeExpCompensation;
#ifdef USE_LSI_3A
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;
#endif

            m_specialCaptureStep = SCAPTURE_STEP_WAIT_CAPTURE;
            ALOGD("DEBUG(%s[%d]):SCAPTURE_STEP_WAIT_CAPTURE_DELAY", __FUNCTION__, __LINE__);
        } else if (m_specialCaptureStep == SCAPTURE_STEP_WAIT_CAPTURE) {
            m_specialCaptureStep = SCAPTURE_STEP_WAIT_CAPTURE;
            shot_ext->shot.ctl.aa.sceneMode = m_backupSceneMode;
            shot_ext->shot.ctl.aa.mode = m_backupAaMode;
            shot_ext->shot.ctl.aa.aeLock = m_backupAeLock;
            shot_ext->shot.ctl.aa.aeExpCompensation = m_backupAeExpCompensation;
#ifdef USE_LSI_3A
            shot_ext->shot.ctl.aa.aeMode = AA_AEMODE_CENTER;
#endif
        } else {
            m_specialCaptureStep = SCAPTURE_STEP_OFF;
            m_delay = 0;
            m_check = false;
        }
    }
#ifdef RAWDUMP_CAPTURE
    else if (shot_ext != NULL && m_specialCaptureMode == SCAPTURE_MODE_RAW) {
        switch (m_specialCaptureStep) {
            case SCAPTURE_STEP_START:
                shot_ext->shot.ctl.aa.captureIntent = AA_CAPTURE_INTENT_PREVIEW;
                m_specialCaptureStep = SCAPTURE_STEP_OFF;
                ALOGD("DEBUG(%s[%d]):SCAPTURE_STEP_START", __FUNCTION__, __LINE__);
                break;
            case SCAPTURE_STEP_OFF:
                shot_ext->shot.ctl.aa.captureIntent = AA_CAPTURE_INTENT_PREVIEW;
                ALOGD("DEBUG(%s[%d]):SCAPTURE_STEP_OFF", __FUNCTION__, __LINE__);
                break;
            default:
                m_specialCaptureStep = SCAPTURE_STEP_OFF;
                m_specialCaptureMode = SCAPTURE_MODE_NONE;
                break;
        }
    }
#endif

    return 1;
}

int ExynosCameraActivitySpecialCapture::t_func3AAfter(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_shot_ext *shot_ext = (struct camera2_shot_ext *)(buf->addr[1]);

    ALOGV("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    if (shot_ext != NULL && m_specialCaptureMode == SCAPTURE_MODE_HDR) {
        if (shot_ext->shot.dm.flash.vendor_firingStable == 2) {
            m_hdrStartFcount[0] = shot_ext->shot.dm.request.frameCount;
            ALOGD("DEBUG(%s[%d]):m_hdrStartFcount[0] (%d / %d)",
                __FUNCTION__, __LINE__, m_hdrStartFcount[0], shot_ext->shot.dm.request.frameCount);
        }

        if (shot_ext->shot.dm.flash.vendor_firingStable == 3) {
            m_hdrStartFcount[1] = shot_ext->shot.dm.request.frameCount;
            m_check = true;
            ALOGD("DEBUG(%s[%d]):m_hdrStartFcount[1] (%d / %d)",
                __FUNCTION__, __LINE__, m_hdrStartFcount[1], shot_ext->shot.dm.request.frameCount);
        }

        if (shot_ext->shot.dm.flash.vendor_firingStable == 4) {
            m_hdrStartFcount[2] = shot_ext->shot.dm.request.frameCount;
            ALOGD("DEBUG(%s[%d]):m_hdrStartFcount[2] (%d / %d)",
                __FUNCTION__, __LINE__, m_hdrStartFcount[2], shot_ext->shot.dm.request.frameCount);
        }

        if (shot_ext->shot.dm.flash.vendor_firingStable == CAPTURE_STATE_HDR_DARK) {
            m_hdrDropFcount[0] = shot_ext->shot.dm.request.frameCount;
            ALOGD("DEBUG(%s[%d]):m_hdrDropFcount[0] (%d / %d)",
                __FUNCTION__, __LINE__, m_hdrDropFcount[0], shot_ext->shot.dm.request.frameCount);
        }

        if (shot_ext->shot.dm.flash.vendor_firingStable == CAPTURE_STATE_HDR_NORMAL) {
            m_hdrDropFcount[1] = shot_ext->shot.dm.request.frameCount;
            ALOGD("DEBUG(%s[%d]):m_hdrDropFcount[1] (%d / %d)",
                __FUNCTION__, __LINE__, m_hdrDropFcount[1], shot_ext->shot.dm.request.frameCount);
        }

        if (shot_ext->shot.dm.flash.vendor_firingStable == CAPTURE_STATE_HDR_BRIGHT) {
            m_hdrDropFcount[2] = shot_ext->shot.dm.request.frameCount;
            ALOGD("DEBUG(%s[%d]):m_hdrDropFcount[2] (%d / %d)",
                __FUNCTION__, __LINE__, m_hdrDropFcount[2], shot_ext->shot.dm.request.frameCount);
        }
    }
#ifdef RAWDUMP_CAPTURE
    else if (shot_ext != NULL && m_specialCaptureMode == SCAPTURE_MODE_RAW) {
        if (m_RawCaptureFcount == 0 && shot_ext->shot.dm.flash.vendor_firingStable == CAPTURE_STATE_RAW_CAPTURE)
        {
            m_RawCaptureFcount = shot_ext->shot.dm.request.frameCount;
            ALOGD("DEBUG(%s[%d]):m_RawCaptureFcount (%d / %d)",
                __FUNCTION__, __LINE__, m_RawCaptureFcount, shot_ext->shot.dm.request.frameCount);
            m_specialCaptureMode = SCAPTURE_MODE_NONE;
        }
        ALOGV("DEBUG(%s[%d]):m_RawCaptureFcount (%d / %d) firingStable(%d)",
                __FUNCTION__, __LINE__, m_RawCaptureFcount,
                shot_ext->shot.dm.request.frameCount, shot_ext->shot.dm.flash.vendor_firingStable);
    }
#endif

    return 1;
}

int ExynosCameraActivitySpecialCapture::t_func3ABeforeHAL3(__unused void *args)
{
    return 1;
}

int ExynosCameraActivitySpecialCapture::t_func3AAfterHAL3(__unused void *args)
{
    return 1;
}

int ExynosCameraActivitySpecialCapture::t_funcSCPBefore(__unused void *args)
{
    ALOGV("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    return 1;
}


int ExynosCameraActivitySpecialCapture::t_funcSCPAfter(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;
    camera2_stream *shot_stream = (struct camera2_stream *)(buf->addr[2]);

    ALOGV("INFO(%s[%d]):(%d)(%d)(%d)", __FUNCTION__, __LINE__, shot_stream->fvalid, shot_stream->fcount, m_hdrDropFcount[0]);
    ALOGV("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    if (shot_stream != NULL && m_specialCaptureMode == SCAPTURE_MODE_HDR) {
#if 0
        if ((m_hdrDropFcount[2] == shot_stream->fcount) ||
            (m_hdrDropFcount[2] + 1 == shot_stream->fcount) ||
            (m_hdrStartFcount[0] == shot_stream->fcount) ||
            (m_hdrStartFcount[1] == shot_stream->fcount) ||
            (m_hdrStartFcount[2] == shot_stream->fcount) ||
            (m_hdrStartFcount[2] + 1 == shot_stream->fcount)) {
            shot_stream->fvalid = false;

            ALOGV("DEBUG(%s[%d]):drop fcount %d [%d %d %d][%d %d %d]", __FUNCTION__, __LINE__, shot_stream->fcount,
                m_hdrStartFcount[0] , m_hdrStartFcount[1] , m_hdrStartFcount[2],
                m_hdrDropFcount[0] , m_hdrDropFcount[1] , m_hdrDropFcount[2]);
        }
#else

        if (m_hdrDropFcount[0] + 3 == shot_stream->fcount) {
            shot_stream->fvalid = false;

            ALOGV("DEBUG(%s[%d]):drop fcount %d [%d %d %d][%d %d %d]", __FUNCTION__, __LINE__, shot_stream->fcount,
                m_hdrStartFcount[0] , m_hdrStartFcount[1] , m_hdrStartFcount[2],
                m_hdrDropFcount[0] , m_hdrDropFcount[1] , m_hdrDropFcount[2]);
        }
#endif
    }

    return 1;
}

int ExynosCameraActivitySpecialCapture::t_funcSCCBefore(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;

    ALOGV("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    return 1;
}

int ExynosCameraActivitySpecialCapture::t_funcSCCAfter(void *args)
{
    ExynosCameraBuffer *buf = (ExynosCameraBuffer *)args;

    ALOGV("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    return 1;
}

int ExynosCameraActivitySpecialCapture::setCaptureMode(enum SCAPTURE_MODE sCaptureModeVal)
{
    m_specialCaptureMode = sCaptureModeVal;

    ALOGD("DEBUG(%s[%d]):(%d)", __FUNCTION__, __LINE__, m_specialCaptureMode);

    return 1;
}

int ExynosCameraActivitySpecialCapture::getIsHdr()
{
    if (m_specialCaptureMode == SCAPTURE_MODE_HDR)
        return true;
    else
        return false;
}

int ExynosCameraActivitySpecialCapture::setCaptureStep(enum SCAPTURE_STEP sCaptureStepVal)
{
    m_specialCaptureStep = sCaptureStepVal;

    if (m_specialCaptureStep == SCAPTURE_STEP_OFF) {
        m_hdrFcount = 0;
        m_currentInputFcount = 0;
        m_backupAeExpCompensation = 0;
        m_hdrStartFcount[0] = 0;
        m_check = false;

        m_hdrStartFcount[0] = 0;
        m_hdrStartFcount[1] = 0;
        m_hdrStartFcount[2] = 0;
        m_hdrDropFcount[0] = 0;
        m_hdrDropFcount[1] = 0;
        m_hdrDropFcount[2] = 0;

        /* dealloc buffers */
    }

    if (m_specialCaptureStep == SCAPTURE_STEP_START) {
        /* alloc buffers */
    }

    ALOGD("DEBUG(%s[%d]):(%d)", __FUNCTION__, __LINE__, m_specialCaptureStep);

    return 1;
}

unsigned int ExynosCameraActivitySpecialCapture::getHdrStartFcount(int index)
{
    return m_hdrStartFcount[index];
}

unsigned int ExynosCameraActivitySpecialCapture::getHdrDropFcount(void)
{
    return m_hdrDropFcount[0];
}

int ExynosCameraActivitySpecialCapture::resetHdrStartFcount()
{
    ALOGD("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    m_hdrStartFcount[0] = m_hdrStartFcount[1] = m_hdrStartFcount[2] = 0;

    return 1;
}

int ExynosCameraActivitySpecialCapture::getHdrWaitFcount()
{
    return HDR_WAIT_COUNT;
}

void ExynosCameraActivitySpecialCapture::setHdrBuffer(ExynosCameraBuffer *secondBuffer, ExynosCameraBuffer *thirdBuffer)
{
    m_hdrBuffer[0] = secondBuffer;
    m_hdrBuffer[1] = thirdBuffer;

    ALOGD("DEBUG(%s[%d]):(%p / %p)", __FUNCTION__, __LINE__, m_hdrBuffer[0], secondBuffer);
    ALOGD("DEBUG(%s[%d]):(%p / %p)", __FUNCTION__, __LINE__, m_hdrBuffer[1], thirdBuffer);
    ALOGD("DEBUG(%s[%d]):(%d) (%d)", __FUNCTION__, __LINE__, m_hdrBuffer[0]->size[0], m_hdrBuffer[0]->size[1]);
    ALOGD("DEBUG(%s[%d]):(%d) (%d)", __FUNCTION__, __LINE__, m_hdrBuffer[1]->size[0], m_hdrBuffer[1]->size[1]);

    return;
}

ExynosCameraBuffer *ExynosCameraActivitySpecialCapture::getHdrBuffer(int index)
{
    ALOGD("DEBUG(%s[%d]):(%d)", __FUNCTION__, __LINE__, index);

    return (m_hdrBuffer[index]);
}

#ifdef RAWDUMP_CAPTURE
unsigned int ExynosCameraActivitySpecialCapture::getRawCaptureFcount(void)
{
    return m_RawCaptureFcount;
}

void ExynosCameraActivitySpecialCapture::resetRawCaptureFcount()
{
    ALOGD("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    m_RawCaptureFcount = 0;
}
#endif
} /* namespace android */

