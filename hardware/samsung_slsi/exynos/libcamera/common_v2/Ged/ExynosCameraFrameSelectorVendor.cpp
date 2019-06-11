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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraFrameSelectorGed"

#include "ExynosCameraFrameSelector.h"

#define FLASHED_LLS_COUNT 4

namespace android {

status_t ExynosCameraFrameSelector::release(void)
{
    int ret = 0;
    ret = m_release(&m_frameHoldList);
    if (ret != NO_ERROR) {
        ALOGE("DEBUG(%s[%d]):m_frameHoldList release failed ", __FUNCTION__, __LINE__);
    }
    ret = m_release(&m_hdrFrameHoldList);
    if (ret != NO_ERROR) {
        ALOGE("DEBUG(%s[%d]):m_hdrFrameHoldList release failed ", __FUNCTION__, __LINE__);
    }

    isCanceled = false;

#ifdef BOARD_CAMERA_USES_DUAL_CAMERA
    if (m_parameters->getDualCameraMode() == true) {
        ExynosCameraDualFrameSelector *dualFrameSelector = ExynosCameraSingleton<ExynosCameraDualFrameSelector>::getInstance();
        dualFrameSelector->clear(m_parameters->getCameraId());
    }
#endif

    return NO_ERROR;
}

status_t ExynosCameraFrameSelector::manageFrameHoldList(ExynosCameraFrame *frame, int pipeID, bool isSrc, int32_t dstPos)
{
    int ret = 0;
#ifdef USE_FRAME_REFERENCE_COUNT
    frame->incRef();
#endif
    if (m_parameters->getHdrMode() == true ||
            m_parameters->getShotMode() == SHOT_MODE_RICH_TONE) {
        ret = m_manageHdrFrameHoldList(frame, pipeID, isSrc, dstPos);
    }
    else {
        ret = m_manageNormalFrameHoldList(frame, pipeID, isSrc, dstPos);
    }

    return ret;
}

status_t ExynosCameraFrameSelector::m_manageNormalFrameHoldList(ExynosCameraFrame *newFrame, int pipeID, bool isSrc, int32_t dstPos)
{
    int ret = 0;
    ExynosCameraFrame *oldFrame = NULL;
    ExynosCameraBuffer buffer;

#ifdef BOARD_CAMERA_USES_DUAL_CAMERA
    if (m_parameters->getDualCameraMode() == true) {
        /* handle dual sync frame */
        ExynosCameraDualFrameSelector *dualFrameSelector = ExynosCameraSingleton<ExynosCameraDualFrameSelector>::getInstance();

        ret = dualFrameSelector->setInfo(m_parameters->getCameraId(),
#ifdef USE_FRAMEMANAGER
                                         m_frameMgr,
#endif
                                         m_frameHoldCount);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):dualFrameSelector->setInfo(id(%d)",
                __FUNCTION__, __LINE__, m_parameters->getCameraId());
            return ret;
        }

        ret = dualFrameSelector->manageNormalFrameHoldList(m_parameters->getCameraId(),
                                                          &m_frameHoldList,
                                                           newFrame, pipeID, isSrc, dstPos, m_bufMgr);

        /* if dual frame sync is fail, just use original code. */
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):dualFrameSelector->manageFrameHoldList(id(%d), pipeID(%d), BufferType(%s), dstPos(%d)",
                __FUNCTION__, __LINE__, m_parameters->getCameraId(), pipeID, (isSrc)?"Src":"Dst", dstPos);
        } else {
            return ret;
        }
    }
#endif

    /* Skip INITIAL_SKIP_FRAME only FastenAeStable is disabled */
    /* This previous condition check is useless because almost framecount for capture is over than skip frame count */
    /*
     * if (m_parameters->getUseFastenAeStable() == true ||
     *     newFrame->getFrameCount() > INITIAL_SKIP_FRAME) {
     */
        m_pushQ(&m_frameHoldList, newFrame, true);

    /*
    } else {
        ret = m_getBufferFromFrame(newFrame, pipeID, isSrc, &buffer, dstPos);
        if( ret != NO_ERROR ) {
            ALOGE("ERR(%s[%d]):m_getBufferFromFrame fail pipeID(%d) BufferType(%s)", __FUNCTION__, __LINE__, pipeID, (isSrc)?"Src":"Dst");
        }
        if (m_bufMgr == NULL) {
            ALOGE("ERR(%s[%d]):m_bufMgr is NULL", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        } else {
            ret = m_bufMgr->putBuffer(buffer.index, EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL);
            if (ret < 0) {
                ALOGE("ERR(%s[%d]):putIndex is %d", __FUNCTION__, __LINE__, buffer.index);
                m_bufMgr->printBufferState();
                m_bufMgr->printBufferQState();
            }
            m_frameComplete(newFrame, false);
            newFrame = NULL;
        }
    }
    */

    if (m_frameHoldList.getSizeOfProcessQ() > m_frameHoldCount) {
        if( m_popQ(&m_frameHoldList, &oldFrame, true, 1) != NO_ERROR ) {
            ALOGE("ERR(%s[%d]):getBufferToManageQ fail", __FUNCTION__, __LINE__);

            m_bufMgr->printBufferState();
            m_bufMgr->printBufferQState();
        } else {
            ret = m_getBufferFromFrame(oldFrame, pipeID, isSrc, &buffer, dstPos);
            if( ret != NO_ERROR ) {
                ALOGE("ERR(%s[%d]):m_getBufferFromFrame fail pipeID(%d) BufferType(%s)", __FUNCTION__, __LINE__, pipeID, (isSrc)?"Src":"Dst");
            }
            if (m_bufMgr == NULL) {
                ALOGE("ERR(%s[%d]):m_bufMgr is NULL", __FUNCTION__, __LINE__);
                return INVALID_OPERATION;
            } else {
                /*
                 Frames in m_frameHoldList and m_hdrFrameHoldList are locked when they are inserted
                 on the list. So we need to use m_LockedFrameComplete() to remove those frames.
                 */
                m_LockedFrameComplete(oldFrame, pipeID, isSrc, dstPos);
                oldFrame = NULL;
            }
        }
    }

    return ret;
}

ExynosCameraFrame* ExynosCameraFrameSelector::selectFrames(int count, int pipeID, bool isSrc, int tryCount, int32_t dstPos)
{
    ExynosCameraFrame* selectedFrame = NULL;
    ExynosCameraActivityFlash *m_flashMgr = NULL;
    ExynosCameraActivityAutofocus *afMgr = m_activityControl->getAutoFocusMgr();     // shoud not be a NULL
    ExynosCameraActivitySpecialCapture *m_sCaptureMgr = m_activityControl->getSpecialCaptureMgr();

    m_reprocessingCount = count;

    m_flashMgr = m_activityControl->getFlashMgr();

#ifdef BOARD_CAMERA_USES_DUAL_CAMERA
    if (m_parameters->getDualCameraMode() == true) {
        /* handle dual sync frame */
        ExynosCameraDualFrameSelector *dualFrameSelector = ExynosCameraSingleton<ExynosCameraDualFrameSelector>::getInstance();

        selectedFrame = dualFrameSelector->selectFrames(m_parameters->getCameraId());
    } else
#endif

    if (m_flashMgr->getNeedCaptureFlash() == true && m_parameters->getSeriesShotCount() == 0) {
        selectedFrame = m_selectFlashFrame(pipeID, isSrc, tryCount, dstPos);

        if (selectedFrame == NULL) {
            ALOGE("ERR(%s[%d]):select Flash Frame Fail!", __FUNCTION__, __LINE__);
            selectedFrame = m_selectNormalFrame(pipeID, isSrc, tryCount, dstPos);
        }
    } else if (m_parameters->getHdrMode() == true ||
            m_parameters->getShotMode() == SHOT_MODE_RICH_TONE) {
        selectedFrame = m_selectHdrFrame(pipeID, isSrc, tryCount, dstPos);

        if (selectedFrame == NULL) {
            ALOGE("ERR(%s[%d]):select HDR Frame Fail!", __FUNCTION__, __LINE__);
            selectedFrame = m_selectNormalFrame(pipeID, isSrc, tryCount, dstPos);
        }
    } else if (afMgr->getRecordingHint() == true
               || m_parameters->getHighResolutionCallbackMode() == true
               || (m_parameters->getShotMode() > SHOT_MODE_AUTO
                   && m_parameters->getShotMode() != SHOT_MODE_NIGHT
                   && m_parameters->getShotMode() != SHOT_MODE_PRO_MODE)) {
        /*
         On recording mode, do not try to find focused frame but just use normal frame.
         ExynosCameraActivityAutofocus::setRecordingHint() is called
         with true argument on startRecording(), and called with false on
         stopRecording(). So it is used to determine whether the recording
         is currently progressing or not on codes below.
         */

        if(afMgr->getRecordingHint() == true && m_parameters->getRecordingHint() == false) {
            ALOGD("DEBUG(%s[%d]):HACK: Applying AFManager recordingHint(true)", __FUNCTION__, __LINE__);
        }

        selectedFrame = m_selectNormalFrame(pipeID, isSrc, tryCount, dstPos);
        if (selectedFrame == NULL)
            ALOGE("ERR(%s[%d]):select Frame Fail!", __FUNCTION__, __LINE__);
    } else if (m_parameters->getSeriesShotCount() > 0) {
        selectedFrame = m_selectBurstFrame(pipeID, isSrc, tryCount, dstPos);
        if (selectedFrame == NULL) {
            ALOGE("ERR(%s[%d]:select focused frame Faile!", __FUNCTION__, __LINE__);
            selectedFrame = m_selectNormalFrame(pipeID, isSrc, tryCount, dstPos);
        }
    } else if (m_parameters->getCaptureExposureTime() > CAMERA_PREVIEW_EXPOSURE_TIME_LIMIT) {
                selectedFrame = m_selectNormalFrame(pipeID, isSrc, tryCount, dstPos);
    } else {
        selectedFrame = m_selectFocusedFrame(pipeID, isSrc, tryCount, dstPos);

        if (selectedFrame == NULL) {
            ALOGE("ERR(%s[%d]:select focused frame Faile!", __FUNCTION__, __LINE__);
            selectedFrame = m_selectNormalFrame(pipeID, isSrc, tryCount, dstPos);
        }
    }

    m_isFirstFrame = false;

    return selectedFrame;
}

ExynosCameraFrame* ExynosCameraFrameSelector::m_selectNormalFrame(__unused int pipeID, __unused bool isSrc, int tryCount, __unused int32_t dstPos)
{
    int ret = 0;
    ExynosCameraFrame *selectedFrame = NULL;

    ret = m_waitAndpopQ(&m_frameHoldList, &selectedFrame, false, tryCount);
    if (ret < 0 || selectedFrame == NULL) {
        ALOGD("DEBUG(%s[%d]):getFrame Fail ret(%d)", __FUNCTION__, __LINE__, ret);
        return NULL;
    } else if (isCanceled == true) {
        ALOGD("DEBUG(%s[%d]):isCanceled", __FUNCTION__, __LINE__);
        if (selectedFrame != NULL) {
            m_LockedFrameComplete(selectedFrame, pipeID, isSrc, dstPos);
        }
        return NULL;
    }
    ALOGD("DEBUG(%s[%d]):Frame Count(%d)", __FUNCTION__, __LINE__, selectedFrame->getFrameCount());

    return selectedFrame;
}

ExynosCameraFrame* ExynosCameraFrameSelector::m_selectFlashFrameV2(int pipeID, bool isSrc, int tryCount, int32_t dstPos)
{
    /* m_selectFlashFrameV2 is implemented by MCD */
    return m_selectFlashFrame(pipeID, isSrc, tryCount, dstPos);
}

status_t ExynosCameraFrameSelector::m_waitAndpopQ(ExynosCameraList<ExynosCameraFrame *> *list, ExynosCameraFrame** outframe, bool unlockflag, int tryCount)
{
    status_t ret = NO_ERROR;
    int iter = 0;

    do {
        ret = list->waitAndPopProcessQ(outframe);

        if (isCanceled == true) {
            ALOGD("DEBUG(%s[%d]):isCanceled", __FUNCTION__, __LINE__);

            return NO_ERROR;
        }

        if( ret < 0 ) {
            if( ret == TIMED_OUT ) {
                ALOGD("DEBUG(%s[%d]):waitAndPopQ Time out -> retry[max cur](%d %d)", __FUNCTION__, __LINE__, tryCount, iter);
                iter++;
                continue;
            }
        }
        if(*outframe != NULL) {
            ALOGD("DEBUG(%s[%d]):Frame Count(%d)", __FUNCTION__, __LINE__, (*outframe)->getFrameCount());
        }
    } while (ret != OK && tryCount > iter);

    if(ret != OK) {
        ALOGE("ERR(%s[%d]):wait for popQ fail(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    if(*outframe == NULL) {
        ALOGE("ERR(%s[%d]):wait for popQ frame = NULL frame(%p)", __FUNCTION__, __LINE__, *outframe);
        return ret;
    }

    if(unlockflag) {
        (*outframe)->frameUnlock();
    }
    return ret;
}

status_t ExynosCameraFrameSelector::clearList(int pipeID, bool isSrc, int32_t dstPos)
{
    int ret = 0;
    ExynosCameraFrame *frame = NULL;
    ExynosCameraBuffer *buffer = NULL;
    if (m_frameHoldList.isWaiting() == false) {
        ret = m_clearList(&m_frameHoldList, pipeID, isSrc, dstPos);
        if( ret < 0 ) {
            ALOGE("DEBUG(%s[%d]):m_frameHoldList clear failed, pipeID(%d)", __FUNCTION__, __LINE__, pipeID);
        }
    } else {
        ALOGE("ERR(%s[%d]):Cannot clear frameHoldList cause waiting for pop frame", __FUNCTION__, __LINE__);
    }

    if (m_hdrFrameHoldList.isWaiting() == false) {
        ret = m_clearList(&m_hdrFrameHoldList, pipeID, isSrc, dstPos);
        if( ret < 0 ) {
            ALOGE("DEBUG(%s[%d]):m_hdrFrameHoldList clear failed, pipeID(%d)", __FUNCTION__, __LINE__, pipeID);
        }
    } else {
        ALOGE("ERR(%s[%d]):Cannot clear hdrFrameHoldList cause waiting for pop frame", __FUNCTION__, __LINE__);
    }

    isCanceled = false;

    return NO_ERROR;
}

status_t ExynosCameraFrameSelector::m_releaseBuffer(ExynosCameraFrame *frame, int pipeID, bool isSrc, int32_t dstPos)
{
    int ret = 0;
    ExynosCameraBuffer bayerBuffer;

    ret = m_getBufferFromFrame(frame, pipeID, isSrc, &bayerBuffer, dstPos);
    if( ret != NO_ERROR ) {
        ALOGE("ERR(%s[%d]):m_getBufferFromFrame fail pipeID(%d) BufferType(%s) bufferPtr(%p)",
                __FUNCTION__, __LINE__, pipeID, (isSrc)?"Src":"Dst", &bayerBuffer);
    }
    if (m_bufMgr == NULL) {
        ALOGE("ERR(%s[%d]):m_bufMgr is NULL", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    } else {
        ret = m_bufMgr->putBuffer(bayerBuffer.index, EXYNOS_CAMERA_BUFFER_POSITION_NONE);
        if (ret < 0) {
            ALOGE("ERR(%s[%d]):putIndex is %d", __FUNCTION__, __LINE__, bayerBuffer.index);
            m_bufMgr->printBufferState();
            m_bufMgr->printBufferQState();
        }
    }
    return NO_ERROR;
}
}
