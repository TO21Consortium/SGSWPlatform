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
#define LOG_TAG "ExynosCameraFrameSelector"

#include "ExynosCameraFrameSelector.h"

#define FLASHED_LLS_COUNT 4

namespace android {

#ifdef USE_FRAMEMANAGER
ExynosCameraFrameSelector::ExynosCameraFrameSelector(ExynosCameraParameters *param,
                                                     ExynosCameraBufferManager *bufMgr,
                                                     ExynosCameraFrameManager *manager
#ifdef SUPPORT_DEPTH_MAP
                                                    , depth_callback_queue_t *depthCallbackQ
                                                    , ExynosCameraBufferManager *depthMapbufMgr
#endif
                                                     )
#else
    ExynosCameraFrameSelector::ExynosCameraFrameSelector(ExynosCameraParameters *param,
                                                         ExynosCameraBufferManager *bufMgr)
#endif
{
#ifdef USE_FRAMEMANAGER
    m_frameMgr = manager;
#endif
    m_parameters = param;
    m_bufMgr= bufMgr;
    m_activityControl = m_parameters->getActivityControl();

    m_frameHoldList.setWaitTime(2000000000);
#ifdef RAWDUMP_CAPTURE
    m_RawFrameHoldList.setWaitTime(2000000000);
#endif

    m_reprocessingCount = 0;
    m_frameHoldCount = 1;
    m_isFirstFrame = true;
    isCanceled = false;

    m_CaptureCount = 0;
#ifdef SUPPORT_DEPTH_MAP
    m_depthCallbackQ = depthCallbackQ;
    m_depthMapbufMgr = depthMapbufMgr;
#endif
}

ExynosCameraFrameSelector::~ExynosCameraFrameSelector()
{
}

status_t ExynosCameraFrameSelector::m_release(ExynosCameraList<ExynosCameraFrame *> *list)
{
    int ret = 0;
    ExynosCameraFrame *frame = NULL;
    while (list->getSizeOfProcessQ() > 0) {
        ret = m_popQ(list, &frame, true, 1);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):getBufferToManageQ fail", __FUNCTION__, __LINE__);
            m_bufMgr->printBufferState();
            m_bufMgr->printBufferQState();
        } else {
            m_frameComplete(frame, true);
        }
    }
    return ret;
}

status_t ExynosCameraFrameSelector::manageFrameHoldListForDynamicBayer(ExynosCameraFrame *frame)
{
    int ret = 0;

    if (frame == NULL) {
        ALOGE("ERR(%s[%d]): frame is NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }
#ifdef USE_FRAME_REFERENCE_COUNT
    frame->incRef();
#endif
    m_pushQ(&m_frameHoldList, frame, true);
    ALOGI("INFO(%s[%d]): frameCount(%d) m_frameHoldList size(%d)",
            __FUNCTION__, __LINE__, frame->getFrameCount(), m_frameHoldList.getSizeOfProcessQ());

    return ret;
}

status_t ExynosCameraFrameSelector::m_manageHdrFrameHoldList(ExynosCameraFrame *frame,
                                                                int pipeID,
                                                                bool isSrc,
                                                                int32_t dstPos)
{
    int ret = 0;
    ExynosCameraBuffer buffer;
    ExynosCameraFrame *newFrame  = NULL;
    ExynosCameraActivitySpecialCapture *m_sCaptureMgr = NULL;
    unsigned int hdrFcount = 0;
    unsigned int fliteFcount = 0;
    newFrame = frame;

    m_sCaptureMgr = m_activityControl->getSpecialCaptureMgr();
    hdrFcount = m_sCaptureMgr->getHdrDropFcount();
    hdrFcount += m_parameters->getHDRDelay();

    ret = m_getBufferFromFrame(newFrame, pipeID, isSrc, &buffer, dstPos);
    if( ret != NO_ERROR ) {
        ALOGE("ERR(%s[%d]):m_getBufferFromFrame fail pipeID(%d) BufferType(%s)",
                __FUNCTION__, __LINE__, pipeID, (isSrc)?"Src":"Dst");
    }

    if (m_parameters->getUsePureBayerReprocessing() == true) {
        camera2_shot_ext *shot_ext = NULL;
        shot_ext = (camera2_shot_ext *)(buffer.addr[1]);
        if (shot_ext != NULL)
            fliteFcount = shot_ext->shot.dm.request.frameCount;
        else
            ALOGE("ERR(%s[%d]):fliteReprocessingBuffer is null", __FUNCTION__, __LINE__);
    } else {
        camera2_stream *shot_stream = NULL;
        shot_stream = (camera2_stream *)(buffer.addr[1]);
        if (shot_stream != NULL)
            fliteFcount = shot_stream->fcount;
        else
            ALOGE("ERR(%s[%d]):fliteReprocessingBuffer is null", __FUNCTION__, __LINE__);
    }

    if (hdrFcount + 1 == fliteFcount || hdrFcount + 2 == fliteFcount || hdrFcount + 3 == fliteFcount) {
        ALOGI("INFO(%s[%d]):hdrFcount %d, fliteFcount %d", __FUNCTION__, __LINE__, hdrFcount, fliteFcount);
        m_pushQ(&m_hdrFrameHoldList, newFrame, true);
    } else {
        m_frameComplete(newFrame, false, pipeID, isSrc, dstPos, true);
        newFrame = NULL;
    }

    return ret;
}

/* It's for dynamic bayer */
ExynosCameraFrame* ExynosCameraFrameSelector::selectDynamicFrames(__unused int count,
                                                                  int pipeID,
                                                                  bool isSrc,
                                                                  int tryCount,
                                                                  int32_t dstPos)
{
        return m_selectNormalFrame(pipeID, isSrc, tryCount, dstPos);
}

ExynosCameraFrame* ExynosCameraFrameSelector::selectCaptureFrames(int count,
                                                                  uint32_t frameCount,
                                                                  int pipeID,
                                                                  bool isSrc,
                                                                  int tryCount,
                                                                  int32_t dstPos)
{
    ExynosCameraFrame* selectedFrame = NULL;
    ExynosCameraActivityFlash *m_flashMgr = NULL;

    m_reprocessingCount = count;
    m_flashMgr = m_activityControl->getFlashMgr();

    if (m_flashMgr->getNeedCaptureFlash() == true) {
        selectedFrame = m_selectFlashFrameV2(pipeID, isSrc, tryCount, dstPos);
        if (selectedFrame == NULL) {
            ALOGE("ERR(%s[%d]):Failed to selectFlashFrame", __FUNCTION__, __LINE__);
            selectedFrame = m_selectNormalFrame(pipeID, isSrc, tryCount, dstPos);
        }
    } else {
        selectedFrame = m_selectCaptureFrame(frameCount, pipeID, isSrc, tryCount, dstPos);
        if (selectedFrame == NULL) {
            ALOGE("ERR(%s[%d]):Failed to selectCaptureFrame", __FUNCTION__, __LINE__);
            selectedFrame = m_selectNormalFrame(pipeID, isSrc, tryCount, dstPos);
        }
    }

    return selectedFrame;
}

ExynosCameraFrame* ExynosCameraFrameSelector::m_selectFocusedFrame(int pipeID, bool isSrc, int tryCount, int32_t dstPos)
{
    int ret = 0;
    ExynosCameraFrame* selectedFrame = NULL;
    struct camera2_shot_ext shot_ext;
    memset(&shot_ext, 0x00, sizeof(struct camera2_shot_ext));

    for (int i = 0; i < CAPTURE_WAITING_COUNT; i++) {
        selectedFrame = m_selectNormalFrame(pipeID, isSrc, tryCount, dstPos);
        if (selectedFrame == NULL) {
            ALOGE("ERR(%s[%d]):selectedFrame is NULL", __FUNCTION__, __LINE__);
            break;
        }

        /* get dynamic metadata for af state */
        if (selectedFrame->getMetaDataEnable() == false)
            ALOGD("DEBUG(%s[%d]):Waiting for update metadata failed (%d) ", __FUNCTION__, __LINE__, ret);
        selectedFrame->getDynamicMeta(&shot_ext);

        /* Skip focusing frame */
        if (m_activityControl->flagFocusing(&shot_ext, m_parameters->getFocusMode()) == true) {
            ALOGD("DEBUG(%s[%d]):skip focusing frame(count %d)",
                    __FUNCTION__, __LINE__, selectedFrame->getFrameCount());
            if (m_bufMgr == NULL) {
                ALOGE("ERR(%s[%d]):m_bufMgr is NULL", __FUNCTION__, __LINE__);
                return NULL;
            } else {
                m_frameComplete(selectedFrame, false, pipeID, isSrc, dstPos, true);
                selectedFrame = NULL;
            }
        } else {
            ALOGD("DEBUG(%s[%d]):focusing complete (count %d)",
                    __FUNCTION__, __LINE__, selectedFrame->getFrameCount());
            break;
        }

        usleep(DM_WAITING_TIME);
    }

    return selectedFrame;
}

ExynosCameraFrame* ExynosCameraFrameSelector::m_selectFlashFrame(int pipeID, bool isSrc, int tryCount, int32_t dstPos)
{
    int ret = 0;
    ExynosCameraFrame* selectedFrame = NULL;
    ExynosCameraBuffer selectedBuffer;
    int bufferFcount = 0;
    int waitFcount = 0;
    int totalWaitingCount = 0;

    /* Choose bayerBuffer to process reprocessing */
    while (totalWaitingCount <= (FLASH_MAIN_TIMEOUT_COUNT + m_parameters->getReprocessingBayerHoldCount())) {
        /* Start main flash & Get best frame count for flash */
        if (waitFcount == 0) {
            waitFcount = m_activityControl->startMainFlash() + 1;
            ALOGD("DEBUG(%s):best frame count for flash capture : %d", __FUNCTION__, waitFcount);
        }

        ret = m_waitAndpopQ(&m_frameHoldList, &selectedFrame, false, tryCount);
        if (ret < 0 ||  selectedFrame == NULL) {
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

        /* Handling exception cases : like preflash is not processed */
        if (waitFcount < 0) {
            ALOGW("WARN(%s[%d]):waitFcount is negative : preflash is not processed", __FUNCTION__, __LINE__);

            return NULL;
        }

        if (isCanceled == true) {
            ALOGD("DEBUG(%s[%d]):isCanceled", __FUNCTION__, __LINE__);

            return NULL;
        }

        ret = m_getBufferFromFrame(selectedFrame, pipeID, isSrc, &selectedBuffer, dstPos);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):m_getBufferFromFrame fail pipeID(%d) BufferType(%s) bufferPtr(%p)",
                    __FUNCTION__, __LINE__, pipeID, (isSrc)?"Src":"Dst", selectedBuffer);
        }

        if (m_isFrameMetaTypeShotExt() == true) {
            camera2_shot_ext *shot_ext = NULL;
            shot_ext = (camera2_shot_ext *)(selectedBuffer.addr[1]);
            if (shot_ext != NULL)
                bufferFcount = shot_ext->shot.dm.request.frameCount;
            else
                ALOGE("ERR(%s[%d]):selectedBuffer is null", __FUNCTION__, __LINE__);
        } else {
            camera2_stream *shot_stream = NULL;
            shot_stream = (camera2_stream *)(selectedBuffer.addr[1]);
            if (shot_stream != NULL)
                bufferFcount = shot_stream->fcount;
            else
                ALOGE("ERR(%s[%d]):selectedBuffer is null", __FUNCTION__, __LINE__);
        }

        /* Put mismatched buffer */
        if (waitFcount != bufferFcount) {
            if (m_bufMgr == NULL) {
                ALOGE("ERR(%s[%d]):m_bufMgr is NULL", __FUNCTION__, __LINE__);
                return NULL;
            } else {
                m_frameComplete(selectedFrame, false, pipeID, isSrc, dstPos, true);
                selectedFrame = NULL;
            }
        }

        if (waitFcount <= bufferFcount) {
            break;
        }

        totalWaitingCount++;
        ALOGD("DEBUG(%s[%d]) (totalWaitingCount %d)", __FUNCTION__, __LINE__, totalWaitingCount);
    }

    if (totalWaitingCount > FLASH_MAIN_TIMEOUT_COUNT) {
        ALOGW("WARN(%s[%d]):fail to get bayer frame count for flash capture (totalWaitingCount %d)",
                __FUNCTION__, __LINE__, totalWaitingCount);
    }

    ALOGD("DEBUG(%s[%d]):waitFcount : %d, bufferFcount : %d",
            __FUNCTION__, __LINE__, waitFcount, bufferFcount);

    /* Stop main flash */
    m_activityControl->stopMainFlash();

    return selectedFrame;

}

ExynosCameraFrame* ExynosCameraFrameSelector::m_selectHdrFrame(__unused int pipeID, __unused bool isSrc, int tryCount, __unused int32_t dstPos)
{
    int ret = 0;
    ExynosCameraFrame* selectedFrame = NULL;

    ret = m_waitAndpopQ(&m_hdrFrameHoldList, &selectedFrame, false, tryCount);
    if( ret < 0 ||  selectedFrame == NULL ) {
        ALOGD("DEBUG(%s[%d]):getFrame Fail ret(%d)", __FUNCTION__, __LINE__, ret);
        return NULL;
    }

    return selectedFrame;

}

ExynosCameraFrame* ExynosCameraFrameSelector::m_selectBurstFrame(int pipeID, bool isSrc, int tryCount, int32_t dstPos)
{
    int ret = 0;
    ExynosCameraFrame* selectedFrame = NULL;

    ExynosCameraActivityFlash *m_flashMgr = m_activityControl->getFlashMgr();

    for (int i = 0; i < TOTAL_WAITING_TIME; i += DM_WAITING_TIME) {
        if (m_isFirstFrame == true) {
            selectedFrame = m_selectFocusedFrame(pipeID, isSrc, tryCount, dstPos);
            if (selectedFrame == NULL) {
                ALOGE("ERR(%s[%d]):selectedFrame is NULL", __FUNCTION__, __LINE__);
                selectedFrame = m_selectNormalFrame(pipeID, isSrc, tryCount, dstPos);
            }
        } else {
            selectedFrame = m_selectNormalFrame(pipeID, isSrc, tryCount, dstPos);
        }

        /* Skip flash frame */
        if (m_flashMgr->checkPreFlash() == true) {
            if (m_flashMgr->checkFlashOff() == false) {
                ALOGD("DEBUG(%s[%d]):skip flash frame(count %d)",
                        __FUNCTION__, __LINE__, selectedFrame->getFrameCount());

                if (m_bufMgr == NULL) {
                    ALOGE("ERR(%s[%d]):m_bufMgr is NULL", __FUNCTION__, __LINE__);
                    return NULL;
                } else {
                    m_frameComplete(selectedFrame, false, pipeID, isSrc, dstPos, true);
                    selectedFrame = NULL;
                }
            } else {
                ALOGD("DEBUG(%s[%d]):flash off done (count %d)",
                        __FUNCTION__, __LINE__, selectedFrame->getFrameCount());
                break;
            }
        } else {
            ALOGD("DEBUG(%s[%d]):pre-flash off", __FUNCTION__, __LINE__);
            break;
        }
        usleep(DM_WAITING_TIME);
    }

    return selectedFrame;
}

ExynosCameraFrame* ExynosCameraFrameSelector::m_selectCaptureFrame(uint32_t frameCount, int pipeID, bool isSrc, int tryCount, int32_t dstPos)
{
    int ret = 0;
    ExynosCameraFrame *selectedFrame = NULL;

    for (int i = 0; i < CAPTURE_WAITING_COUNT; i++) {
        selectedFrame = m_selectNormalFrame(pipeID, isSrc, tryCount, dstPos);
        if (selectedFrame == NULL) {
            ALOGE("ERR(%s[%d]):selectedFrame is NULL", __FUNCTION__, __LINE__);
            break;
        }

        if (selectedFrame->getFrameCount() < frameCount) {
            ALOGD("DEBUG(%s[%d]):skip capture frame(count %d), waiting frame(count %d)",
                    __FUNCTION__, __LINE__, selectedFrame->getFrameCount(), frameCount);

            if (m_bufMgr == NULL) {
                ALOGE("ERR(%s[%d]):m_bufMgr is NULL", __FUNCTION__, __LINE__);
                return NULL;
            } else {
                m_frameComplete(selectedFrame, false, pipeID, isSrc, dstPos, true);
                selectedFrame = NULL;
            }
        } else {
            ALOGD("DEBUG(%s[%d]):capture frame (count %d)",
                    __FUNCTION__, __LINE__, selectedFrame->getFrameCount());
            break;
        }
    }

    return selectedFrame;
}

status_t ExynosCameraFrameSelector::m_getBufferFromFrame(ExynosCameraFrame *frame,
                                                            int pipeID,
                                                            bool isSrc,
                                                            ExynosCameraBuffer *outBuffer,
                                                            int32_t dstPos)
{
    status_t ret = NO_ERROR;
    ExynosCameraBuffer selectedBuffer;

    if (frame == NULL) {
        ALOGE("ERR(%s[%d]):frame == NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (outBuffer == NULL) {
        ALOGE("ERR(%s[%d]):outBuffer == NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (isSrc) {
        ret = frame->getSrcBuffer(pipeID, &selectedBuffer);
        if (ret != NO_ERROR)
            ALOGE("ERR(%s[%d]):getSrcBuffer(pipeID %d) fail", __FUNCTION__, __LINE__, pipeID);
    } else {
        if (dstPos < 0) {
            ret = frame->getDstBuffer(pipeID, &selectedBuffer);
            if (ret != NO_ERROR)
                ALOGE("ERR(%s[%d]):getDstBuffer(pipeID %d) fail", __FUNCTION__, __LINE__, pipeID);
        } else {
            ret = frame->getDstBuffer(pipeID, &selectedBuffer, dstPos);
            if (ret != NO_ERROR)
                ALOGE("ERR(%s[%d]):getDstBuffer(pipeID %d, dstPos %d) fail", __FUNCTION__, __LINE__, pipeID, dstPos);
        }
    }

    *outBuffer = selectedBuffer;
    return ret;
}

status_t ExynosCameraFrameSelector::m_pushQ(ExynosCameraList<ExynosCameraFrame *> *list,
                                            ExynosCameraFrame* inframe,
                                            bool lockflag)
{
    status_t ret = NO_ERROR;
    if( lockflag ) {
        inframe->frameLock();
    }
    list->pushProcessQ(&inframe);
    return ret;
}
status_t ExynosCameraFrameSelector::m_popQ(ExynosCameraList<ExynosCameraFrame *> *list,
                                            ExynosCameraFrame** outframe,
                                            bool unlockflag,
                                            int tryCount)
{
    status_t ret = NO_ERROR;
    int iter = 0;

    do {
        ret = list->popProcessQ(outframe);
        if( ret < 0 ) {
            if( ret == TIMED_OUT ) {
                ALOGD("DEBUG(%s[%d]):PopQ Time out -> retry[max cur](%d %d)",
                        __FUNCTION__, __LINE__, tryCount, iter);

                iter++;
                continue;
            }
        }
    } while (ret != OK && tryCount > iter);

    if( ret != OK ) {
        ALOGE("ERR(%s[%d]):popQ fail(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    if( *outframe == NULL ) {
        ALOGE("ERR(%s[%d]):popQ frame = NULL frame(%p)", __FUNCTION__, __LINE__, *outframe);
        return ret;
    }

    if( unlockflag ) {
        (*outframe)->frameUnlock();
    }
    return ret;
}

#ifdef USE_FRAMEMANAGER
status_t ExynosCameraFrameSelector::m_frameComplete(ExynosCameraFrame *frame, bool isForcelyDelete,
                                                        int pipeID, bool isSrc, int32_t dstPos, bool flagReleaseBuf)
{
    int ret = OK;

    if(flagReleaseBuf) {
        m_releaseBuffer(frame, pipeID, isSrc, dstPos);
    }

    if (isForcelyDelete == true) {
        ALOGD("DEBUG(%s[%d]):frame deleted forcely : isComplete(%d) count(%d) LockState(%d)",
            __FUNCTION__, __LINE__,
            frame->isComplete(),
            frame->getFrameCount(),
            frame->getFrameLockState());

        if (m_frameMgr != NULL) {
#ifdef USE_FRAME_REFERENCE_COUNT
            frame->decRef();
#endif
            m_frameMgr->deleteFrame(frame);
        } else {
            ALOGE("ERR(%s[%d]):m_frameMgr is NULL (%d)", __FUNCTION__, __LINE__, frame->getFrameCount());
        }
        frame = NULL;
    } else {
        ALOGV("DEBUG(%s[%d]):frame complete, count(%d)", __FUNCTION__, __LINE__, frame->getFrameCount());
#ifndef USE_FRAME_REFERENCE_COUNT
        if (frame->isComplete() == true) {
            if (frame->getFrameLockState() == false) {
                ALOGV("DEBUG(%s[%d]):frame complete, count(%d)",
                        __FUNCTION__, __LINE__, frame->getFrameCount());

                if (m_frameMgr != NULL) {
#ifdef USE_FRAME_REFERENCE_COUNT
                    frame->decRef();
#endif
                    m_frameMgr->deleteFrame(frame);

                } else {
                    ALOGE("ERR(%s[%d]):m_frameMgr is NULL (%d)",
                            __FUNCTION__, __LINE__, frame->getFrameCount());
                }
                frame = NULL;
            } else {
                ALOGV("DEBUG(%s[%d]):frame is locked : isComplete(%d) count(%d) LockState(%d)",
                    __FUNCTION__, __LINE__,
                    frame->isComplete(),
                    frame->getFrameCount(),
                    frame->getFrameLockState());
            }
        } else {
            if (m_frameMgr != NULL) {
#ifdef USE_FRAME_REFERENCE_COUNT
                frame->decRef();
                ALOGE("ERR(%s[%d]):frame deleted : key(%d) refCnt(%d)",
                        __FUNCTION__, __LINE__, frame->getUniqueKey(), frame->getRef());
#endif
                m_frameMgr->deleteFrame(frame);
            } else {
                ALOGE("ERR(%s[%d]):m_frameMgr is NULL (%d)",
                        __FUNCTION__, __LINE__, frame->getFrameCount());
            }
            frame = NULL;
        }
#else /* USE_FRAME_REFERENCE_COUNT */
        if (m_frameMgr != NULL) {
#ifdef USE_FRAME_REFERENCE_COUNT
            frame->decRef();
#endif
            m_frameMgr->deleteFrame(frame);
        } else {
            ALOGE("ERR(%s[%d]):m_frameMgr is NULL (%d)", __FUNCTION__, __LINE__, frame->getFrameCount());
        }
#endif
        frame = NULL;
    }

    return ret;
}
#else /* USE_FRAMEMANAGER */
status_t ExynosCameraFrameSelector::m_frameComplete(ExynosCameraFrame *frame, bool isForcelyDelete,                                                  int pipeID,
                                                        int pipeID, bool isSrc, int32_t dstPos, bool flagReleaseBuf)
{
    int ret = OK;

    if(flagReleaseBuf) {
        m_releaseBuffer(frame, pipeID, isSrc, dstPos);
    }

    if (isForcelyDelete == true) {
        ALOGD("DEBUG(%s[%d]):frame deleted forcely : isComplete(%d) count(%d) LockState(%d)",
            __FUNCTION__, __LINE__,
            frame->isComplete(),
            frame->getFrameCount(),
            frame->getFrameLockState());
        delete frame;
        frame = NULL;
    } else {
        if (frame->isComplete() == true) {
            if (frame->getFrameLockState() == false) {
                ALOGV("DEBUG(%s[%d]):frame complete, count(%d)",
                        __FUNCTION__, __LINE__, frame->getFrameCount());

                delete frame;
                frame = NULL;
            } else {
                ALOGV("DEBUG(%s[%d]):frame is locked : isComplete(%d) count(%d) LockState(%d)",
                    __FUNCTION__, __LINE__,
                    frame->isComplete(),
                    frame->getFrameCount(),
                    frame->getFrameLockState());
            }
        } else {
            ALOGV("DEBUG(%s[%d]):frame is not completed : isComplete(%d) count(%d) LockState(%d)",
                __FUNCTION__, __LINE__,
                frame->isComplete(),
                frame->getFrameCount(),
                frame->getFrameLockState());
        }
    }
    return ret;
}
#endif

/*
 * Check complete flag of the Frame and deallocate it if it is completed.
 * This function ignores lock flag of the frame(Lock flag is usually set to protect
 * the frame from deallocation), so please use with caution.
 * This function is required to remove a frame from frameHoldingList.
 */
#ifdef USE_FRAMEMANAGER
status_t ExynosCameraFrameSelector::m_LockedFrameComplete(ExynosCameraFrame *frame, int pipeID,
                                                                bool isSrc, int32_t dstPos)
{
    int ret = OK;

    m_releaseBuffer(frame, pipeID, isSrc, dstPos);

#ifndef USE_FRAME_REFERENCE_COUNT
    if (frame->isComplete() == true) {
        if (frame->getFrameLockState() == true)
        {
            ALOGV("DEBUG(%s[%d]):Deallocating locked frame, count(%d)",
                    __FUNCTION__, __LINE__, frame->getFrameCount());
        }

        if (m_frameMgr != NULL) {
#ifdef USE_FRAME_REFERENCE_COUNT
            frame->decRef();
#endif
            m_frameMgr->deleteFrame(frame);
        }
        frame = NULL;
    }
#else
	if (m_frameMgr != NULL) {
#ifdef USE_FRAME_REFERENCE_COUNT
		frame->decRef();
#endif
		m_frameMgr->deleteFrame(frame);
	} else {
		ALOGE("ERR(%s[%d]):m_frameMgr is NULL (%d)", __FUNCTION__, __LINE__, frame->getFrameCount());
	}
#endif

    return ret;
}
#else
status_t ExynosCameraFrameSelector::m_LockedFrameComplete(ExynosCameraFrame *frame, int pipeID,
                                                                bool isSrc, int32_t dstPos)
{
    int ret = OK;

    m_releaseBuffer(frame, pipeID, isSrc, dstPos);

    if (frame->isComplete() == true) {
        if (frame->getFrameLockState() == true)
        {
            ALOGV("DEBUG(%s[%d]):Deallocating locked frame, count(%d)",
                __FUNCTION__, __LINE__, frame->getFrameCount());
            ALOGV("DEBUG(%s[%d]):frame is locked : isComplete(%d) count(%d) LockState(%d)",
                __FUNCTION__, __LINE__,
                frame->isComplete(),
                frame->getFrameCount(),
                frame->getFrameLockState());
        }
        delete frame;
        frame = NULL;
    }
    return ret;
}
#endif

status_t ExynosCameraFrameSelector::wakeupQ(void)
{
    m_frameHoldList.sendCmd(WAKE_UP);

    return NO_ERROR;
}

status_t ExynosCameraFrameSelector::cancelPicture(bool flagCancel)
{
    isCanceled = flagCancel;

    return NO_ERROR;
}

status_t ExynosCameraFrameSelector::m_clearList(ExynosCameraList<ExynosCameraFrame *> *list,int pipeID, bool isSrc, int32_t dstPos)
{
    int ret = 0;
    ExynosCameraFrame *frame = NULL;
    ExynosCameraBuffer buffer;

    while (list->getSizeOfProcessQ() > 0) {
        if( m_popQ(list, &frame, false, 1) != NO_ERROR ) {
            ALOGE("ERR(%s[%d]):getBufferToManageQ fail", __FUNCTION__, __LINE__);

            m_bufMgr->printBufferState();
            m_bufMgr->printBufferQState();
        } else {
            ret = m_getBufferFromFrame(frame, pipeID, isSrc, &buffer, dstPos);
            if( ret != NO_ERROR ) {
                ALOGE("ERR(%s[%d]):m_getBufferFromFrame fail pipeID(%d) BufferType(%s)",
                        __FUNCTION__, __LINE__, pipeID, (isSrc)?"Src":"Dst");
            }
            if (m_bufMgr == NULL) {
                ALOGE("ERR(%s[%d]):m_bufMgr is NULL", __FUNCTION__, __LINE__);
                return INVALID_OPERATION;
            } else {
                if (buffer.index >= 0)
                    ret = m_bufMgr->putBuffer(buffer.index, EXYNOS_CAMERA_BUFFER_POSITION_NONE);
                if (ret < 0) {
                    ALOGE("ERR(%s[%d]):putIndex is %d", __FUNCTION__, __LINE__, buffer.index);
                    m_bufMgr->printBufferState();
                    m_bufMgr->printBufferQState();
                }
                /*
                 Frames in m_frameHoldList and m_hdrFrameHoldList are locked when they are inserted
                 on the list. So we need to use m_LockedFrameComplete() to remove those frames.
                 Please beware that the frame might be deleted in elsewhere, epically on erroneous
                 conditions. So if the program encounters memory fault here, please check the other
                 frame deallocation(delete) routines.
                 */

                /* Rather than blindly deleting frame in m_LockedFrameComplete(), we do following:
                 * 1. Check whether frame is complete. Delete the frame if it is complete.
                 * 2. If the frame is not complete, unlock it. mainThread will delete this frame.
                 */

                //m_LockedFrameComplete(frame);

#ifdef USE_FRAMEMANAGER
#ifndef USE_FRAME_REFERENCE_COUNT
                if (frame->isComplete() == true) {
                    if (frame->getFrameLockState() == true)
                        ALOGV("DEBUG(%s[%d]):Deallocating locked frame, count(%d)",
                                __FUNCTION__, __LINE__, frame->getFrameCount());
#else
                {
#endif
                    if (m_frameMgr != NULL) {
#ifdef USE_FRAME_REFERENCE_COUNT
                        frame->decRef();
#endif
                        m_frameMgr->deleteFrame(frame);
                    } else {
                        ALOGE("ERR(%s[%d]):m_frameMgr is NULL (%d)", __FUNCTION__, __LINE__, frame->getFrameCount());
                    }
                }
#else
                if (frame->isComplete() == true) {
                    delete frame;
                    frame = NULL;
                } else {
                    if (frame->getFrameLockState() == true)
                        frame->frameUnlock();
                }
#endif
            }
        }
    }
    return ret;
}

status_t ExynosCameraFrameSelector::setFrameHoldCount(int32_t count)
{
    if (count < 0) {
        ALOGE("ERR(%s[%d]):frame hold count cannot be negative value, current value(%d)",
                __FUNCTION__, __LINE__, count);
        return BAD_VALUE;
    }

    m_frameHoldCount = count;

    return NO_ERROR;
}

bool ExynosCameraFrameSelector::m_isFrameMetaTypeShotExt(void)
{
    bool isShotExt = true;

    if (m_parameters->isSccCapture() == true) {
        if (m_parameters->isReprocessing() == true)
            isShotExt = true;
        else
            isShotExt = false;
    } else {
        if (m_parameters->getUsePureBayerReprocessing() == false)
            isShotExt = false;
    }

    return isShotExt;
}

void ExynosCameraFrameSelector::setWaitTime(uint64_t waitTime)
{
    m_frameHoldList.setWaitTime(waitTime);
}

void ExynosCameraFrameSelector::setIsFirstFrame(bool isFirstFrame)
{
    m_isFirstFrame = isFirstFrame;
}

bool ExynosCameraFrameSelector::getIsFirstFrame()
{
    return m_isFirstFrame;
}

void ExynosCameraFrameSelector::wakeselectDynamicFrames(void)
{
    isCanceled = true;
    m_frameHoldList.wakeupAll();
}
}
