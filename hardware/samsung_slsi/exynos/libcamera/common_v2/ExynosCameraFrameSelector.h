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

#ifndef EXYNOS_CAMERA_BAYER_SELECTOR_H
#define EXYNOS_CAMERA_BAYER_SELECTOR_H

#include "ExynosCameraParameters.h"
#include "ExynosCameraBuffer.h"
#include "ExynosCameraBufferManager.h"
#include "ExynosCameraList.h"
#include "ExynosCameraActivityControl.h"
#include "ExynosCameraFrame.h"
#ifdef USE_FRAMEMANAGER
#include "ExynosCameraFrameManager.h"
#endif
#ifdef BOARD_CAMERA_USES_DUAL_CAMERA
#include "ExynosCameraDualFrameSelector.h"
#endif

namespace android{
class ExynosCameraFrameSelector {
public:
#ifdef USE_FRAMEMANAGER
    ExynosCameraFrameSelector (ExynosCameraParameters *param,
                            ExynosCameraBufferManager *bufMgr, ExynosCameraFrameManager *manager = NULL
                            );
#else
    ExynosCameraFrameSelector (ExynosCameraParameters *param,
                            ExynosCameraBufferManager *bufMgr);
#endif
    ~ExynosCameraFrameSelector();
    status_t release(void);
    status_t manageFrameHoldList(ExynosCameraFrame *frame, int pipeID, bool isSrc, int32_t dstPos = 0);
    status_t manageFrameHoldListForDynamicBayer(ExynosCameraFrame *frame);
    ExynosCameraFrame* selectFrames(int count, int pipeID, bool isSrc, int tryCount, int32_t dstPos = 0);
    ExynosCameraFrame* selectDynamicFrames(int count, int pipeID, bool isSrc, int tryCount, int32_t dstPos);
    ExynosCameraFrame* selectCaptureFrames(int count, uint32_t frameCount, int pipeID, bool isSrc, int tryCount, int32_t dstPos = 0);
    status_t clearList(int pipeID = -1 , bool isSrc = false, int32_t dstPos = 0);
    int getHoldCount(void) { return m_frameHoldList.getSizeOfProcessQ(); };
    status_t setFrameHoldCount(int32_t count);
    status_t cancelPicture(bool flagCancel = true);
    status_t wakeupQ(void);
    void setWaitTime(uint64_t waitTime);
    void setIsFirstFrame(bool isFirstFrame);
    bool getIsFirstFrame();

    void wakeselectDynamicFrames(void);
private:
    status_t m_manageNormalFrameHoldList(ExynosCameraFrame *frame, int pipeID, bool isSrc, int32_t dstPos);
    status_t m_manageHdrFrameHoldList(ExynosCameraFrame *frame, int pipeID, bool isSrc, int32_t dstPos);
#ifdef RAWDUMP_CAPTURE
    status_t m_manageRawFrameHoldList(ExynosCameraFrame *frame, int pipeID, bool isSrc, int32_t dstPos);
    ExynosCameraFrame* m_selectRawNormalFrame(int pipeID, bool isSrc, int tryCount);
    status_t m_list_release(ExynosCameraList<ExynosCameraFrame *> *list, int pipeID, bool isSrc, int32_t dstPos);
    int removeFlags;
#endif

    ExynosCameraFrame* m_selectNormalFrame(int pipeID, bool isSrc, int tryCount, int32_t dstPos);
    ExynosCameraFrame* m_selectFlashFrame(int pipeID, bool isSrc, int tryCount, int32_t dstPos);
    ExynosCameraFrame* m_selectFlashFrameV2(int pipeID, bool isSrc, int tryCount, int32_t dstPos);
    ExynosCameraFrame* m_selectFocusedFrame(int pipeID, bool isSrc, int tryCount, int32_t dstPos);
    ExynosCameraFrame* m_selectHdrFrame(int pipeID, bool isSrc, int tryCount, int32_t dstPos);
    ExynosCameraFrame* m_selectBurstFrame(int pipeID, bool isSrc, int tryCount, int32_t dstPos);
    ExynosCameraFrame* m_selectCaptureFrame(uint32_t frameCount, int pipeID, bool isSrc, int tryCount, int32_t dstPos);
    status_t m_getBufferFromFrame(ExynosCameraFrame *frame, int pipeID, bool isSrc, ExynosCameraBuffer *outBuffer, int32_t dstPos);
    status_t m_pushQ(ExynosCameraList<ExynosCameraFrame *> *list, ExynosCameraFrame* inframe, bool lockflag);
    status_t m_popQ(ExynosCameraList<ExynosCameraFrame *> *list, ExynosCameraFrame** outframe, bool unlockflag, int tryCount);
    status_t m_waitAndpopQ(ExynosCameraList<ExynosCameraFrame *> *list, ExynosCameraFrame** outframe, bool unlockflag, int tryCount);
    status_t m_frameComplete(ExynosCameraFrame *frame, bool isForcelyDelete = false,
                                int pipeID = 0, bool isSrc = false, int32_t dstPos = 0, bool flagReleaseBuf = false);
    status_t m_LockedFrameComplete(ExynosCameraFrame *frame, int pipeID, bool isSrc, int32_t dstPos);
    status_t m_clearList(ExynosCameraList<ExynosCameraFrame *> *list, int pipeID, bool isSrc, int32_t dstPos);
    status_t m_release(ExynosCameraList<ExynosCameraFrame *> *list);
    status_t m_releaseBuffer(ExynosCameraFrame *frame, int pipeID, bool isSrc, int32_t dstPos);

    bool m_isFrameMetaTypeShotExt(void);

private:
    ExynosCameraList<ExynosCameraFrame *> m_frameHoldList;
    ExynosCameraList<ExynosCameraFrame *> m_hdrFrameHoldList;
#ifdef RAWDUMP_CAPTURE
    ExynosCameraList<ExynosCameraFrame *> m_RawFrameHoldList;
#endif
#ifdef USE_FRAMEMANAGER
    ExynosCameraFrameManager *m_frameMgr;
#endif
    ExynosCameraParameters *m_parameters;
    ExynosCameraBufferManager *m_bufMgr;
    ExynosCameraActivityControl *m_activityControl;

    int m_reprocessingCount;

    mutable Mutex m_listLock;
    int32_t m_frameHoldCount;
    bool isCanceled;
    int32_t m_CaptureCount;
    bool m_isFirstFrame;
};
}

#endif

