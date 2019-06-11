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

/*!
 * \file      ExynosCameraDualFrameSelector.h
 * \brief     header file for ExynosCameraDualFrameSelector
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2014/10/08
 *
 * <b>Revision History: </b>
 * - 2014/10/08 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 *
 */

#ifndef EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_H
#define EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_H

#include "string.h"
#include <utils/Log.h>
#include <utils/threads.h>

#include "ExynosCameraSingleton.h"
#include "ExynosCameraFrameSelector.h"
#include "ExynosCameraFusionInclude.h"

using namespace android;

/* Class declaration */
//! ExynosCameraDualFrameSelector is sync logic to, get sync frame, among asynchronously coming frames from multiple camera.
/*!
 * \ingroup ExynosCamera
 */
class ExynosCameraDualFrameSelector
{
protected:
    /* Class declaration */
    //! SyncObj is the object to have frame
    /*!
     * \ingroup ExynosCamera
     */
    class SyncObj {
    public:
        //! Constructor
        SyncObj();
        //! Destructor
        ~SyncObj();

        //! create
        /*!
        \param cameraId
        \param frame
        \param pipeID
            caller's pipeID
        \param isSrc
            to check srcBuffer or dstBuffer
        \param dstPos
            when dstBuffer, it need dstPosition to get buffer
        \remarks
            create the initiate syncObj
        */
        status_t create(int cameraId,
                        ExynosCameraFrame *frame,
                        int pipeID,
                        bool isSrc,
                        int dstPos,
#ifdef USE_FRAMEMANAGER
                        ExynosCameraFrameManager *frameMgr,
#endif
                        ExynosCameraBufferManager *bufMgr);

        //! destroy
        /*!
        \remarks
            destroy obj (this leads frame's buffer release)
        */
        status_t destroy(void);

        //! getCameraId
        /*!
        \remarks
            get cameraId of frame
        */
        int      getCameraId(void);

        //! getFrame
        ExynosCameraFrame *getFrame(void);

        //! getPipeId
        /*!
        \remarks
            get pipeId of frame
        */
        int getPipeID(void);

        //! getBufferManager
        /*!
        \remarks
            get bufferManager of frame
        */
        ExynosCameraBufferManager *getBufferManager(void);

        //! getTimeStamp
        int      getTimeStamp(void);

        //! isSimilarTimeStamp
        /*!
        \param other
            other SyncObj to compare timeStamp
        \remarks
            compare my timeStamp and other's timeStamp
            if similar(sync) frames, return true;
            else, return false;
        */
        bool     isSimilarTimeStamp(SyncObj *other);

    private:
        status_t m_lockedFrameComplete(void);
        status_t m_getBufferFromFrame(ExynosCameraBuffer *outBuffer);

    public:
        SyncObj& operator =(const SyncObj &other) {
            m_cameraId   = other.m_cameraId;
            m_frame      = other.m_frame;
            m_pipeID     = other.m_pipeID;
            m_isSrc      = other.m_isSrc;
            m_dstPos     = other.m_dstPos;
#ifdef USE_FRAMEMANAGER
            m_frameMgr   = other.m_frameMgr;
#endif
            m_bufMgr     = other.m_bufMgr;

            m_timeStamp  = other.m_timeStamp;

            return *this;
        }

        bool operator ==(const SyncObj &other) const {
            bool ret = true;

            if (m_cameraId   != other.m_cameraId ||
                m_frame      != other.m_frame    ||
                m_pipeID     != other.m_pipeID   ||
                m_isSrc      != other.m_isSrc    ||
                m_dstPos     != other.m_dstPos   ||
#ifdef USE_FRAMEMANAGER
                m_frameMgr   != other.m_frameMgr ||
#endif
                m_bufMgr     != other.m_bufMgr   ||
                m_timeStamp  != other.m_timeStamp) {
                ret = false;
            }

            return ret;
        }

        bool operator !=(const SyncObj &other) const {
            return !(*this == other);
        }

    private:
        int m_cameraId;
        ExynosCameraFrame *m_frame;
        int m_pipeID;
        bool m_isSrc;
        int m_dstPos;

#ifdef USE_FRAMEMANAGER
        ExynosCameraFrameManager *m_frameMgr;
#endif
        ExynosCameraBufferManager *m_bufMgr;

        int m_timeStamp;
    };

protected:
    friend class ExynosCameraSingleton<ExynosCameraDualFrameSelector>;

    //! Constructor
    ExynosCameraDualFrameSelector();

    //! Destructor
    virtual ~ExynosCameraDualFrameSelector();

public:
    //! setInfo
    /*!
    \param cameraId
    \param frameMgr
        frameManager to free frame.
    \param holdCount
        synced buffer count to hold on.
        ex) if hold Count is 1, it maintain only last 1 synced frame.
    \remarks
        setting information of cameraId's stream
    */
    status_t setInfo(int cameraId,
#ifdef USE_FRAMEMANAGER
                     ExynosCameraFrameManager *frameMgr,
#endif
                     int holdCount);

    //! manageNormalFrameHoldList
    /*!
    \param cameraId
    \param outputList
        when frame is synced. the target list that frame move.
    \param frame
    \param pipeID
        caller's pipeID
    \param isSrc
        to check srcBuffer or dstBuffer
    \param dstPos
        when dstBuffer, it need dstPosition to get buffer
    \param bufMgr
        bufferManager to get and free frame's buffer.
    \remarks
        trigger sync logic with a frame.
        when this function call, this class try to compare to find sync frame.
        If logic find synced frame, push frame to list which is from argument.
        This list is type of FIFO not stack.
        and, you can get sync frame by selectFrames();
    */
    status_t  manageNormalFrameHoldList(int cameraId,
                                        ExynosCameraList<ExynosCameraFrame *> *outputList,
                                        ExynosCameraFrame *frame,
                                        int pipeID, bool isSrc, int32_t dstPos, ExynosCameraBufferManager *bufMgr);

    //! selectFrames
    /*!
    \param cameraId
    \remarks
        get synced frame of cameraId
        this return one sync frame.
    */
    ExynosCameraFrame* selectFrames(int cameraId);

    //! releaseFrames
    /*!
    \param cameraId
    \remarks
        when error case, find frame on my and other's m_outputObjList() by frame's timeStamp.
        then, force release(== putBuffer() to bufferManger) sync frame of all cameras
    */
    status_t           releaseFrames(int cameraId, ExynosCameraFrame *frame);

    //! clear
    /*!
    \param cameraId
    \remarks
        This API make class reset.
        reset means
        release buffer to bufferManager.
        and, release frame in all sync and not-sync frame to frameManager.
    */
    status_t  clear(int cameraId);

protected:
    bool     m_checkSyncObjOnList   (SyncObj *syncObj,
                                     int otherCameraId,
                                     List<ExynosCameraDualFrameSelector::SyncObj> *list,
                                     SyncObj *resultSyncObj);
    status_t m_destroySyncObj       (SyncObj *syncObj, ExynosCameraList<ExynosCameraFrame *> *outlist);

    status_t m_pushList             (List<ExynosCameraDualFrameSelector::SyncObj> *list, SyncObj *syncObj);
    status_t m_popList              (List<ExynosCameraDualFrameSelector::SyncObj> *list, SyncObj *syncObj);
    status_t m_popListUntilTimeStamp(List<ExynosCameraDualFrameSelector::SyncObj> *list, SyncObj *syncObj, ExynosCameraList<ExynosCameraFrame *> *outputList = NULL);
    status_t m_popListUntilNumber   (List<ExynosCameraDualFrameSelector::SyncObj> *list, int minNum, SyncObj *syncObj = NULL, ExynosCameraList<ExynosCameraFrame *> *outputList = NULL);
    status_t m_clearList            (List<ExynosCameraDualFrameSelector::SyncObj> *list, ExynosCameraList<ExynosCameraFrame *> *outputList = NULL);
    void     m_printList            (List<ExynosCameraDualFrameSelector::SyncObj> *list, int cameraId);

    int      m_getTimeStamp(ExynosCameraFrame *frame);

    status_t m_pushQ(ExynosCameraList<ExynosCameraFrame *> *list, ExynosCameraFrame* frame, int cameraId);
    status_t m_popQ (ExynosCameraList<ExynosCameraFrame *> *list, ExynosCameraFrame** outframe, int cameraId);

protected:
    // not yet sync obj list
    List<ExynosCameraDualFrameSelector::SyncObj>  m_noSyncObjList[CAMERA_ID_MAX];

    // sync obj list
    List<ExynosCameraDualFrameSelector::SyncObj>  m_syncObjList[CAMERA_ID_MAX];

    // output candidate obj list
    ExynosCameraList<ExynosCameraFrame *>        *m_outputList[CAMERA_ID_MAX];

#ifdef USE_FRAMEMANAGER
    ExynosCameraFrameManager  *m_frameMgr[CAMERA_ID_MAX];
#endif

    int                        m_holdCount[CAMERA_ID_MAX];
    bool                       m_flagValidCameraId[CAMERA_ID_MAX];

    Mutex                      m_lock;
};

//! ExynosCameraDualFrameSelector
/*!
 * \ingroup ExynosCamera
 */
class ExynosCameraDualPreviewFrameSelector : public ExynosCameraDualFrameSelector
{
protected:
    friend class ExynosCameraSingleton<ExynosCameraDualPreviewFrameSelector>;

    //! Constructor
    ExynosCameraDualPreviewFrameSelector();

    //! Destructor
    virtual ~ExynosCameraDualPreviewFrameSelector();

public:
    /*!
    \param cameraId
    \param holdCount
        synced buffer count to hold on.
        ex) if hold Count is 1, it maintain only last 1 synced frame.
    \remarks
        setting information of cameraId's stream
    */
    status_t setInfo(int cameraId,
                     int holdCount,
                     DOF *dof);

    /*!
    \param cameraId
    \remarks
        return cameraId's DOF
    */
    DOF *getDOF(int cameraId);

    //! managePreviewFrameHoldList
    /*!
    \param cameraId
    \param outputList
        when frame is not synced. the frame is move to outputList.
        the owner of outputList can detect like this.
        if (entity->getDstBufState() == ENTITY_BUFFER_STATE_ERROR)
    \param frame
    \param pipeID
        caller's pipeID
    \param isSrc
        to check srcBuffer or dstBuffer
    \param dstPos
        when dstBuffer, it need dstPosition to get buffer
    \param bufMgr
        bufferManager to get and free frame's buffer.
    \remarks
        trigger sync logic with a frame.
        when this function call, this class try to compare to find sync frame.
        then, maintain sync frame.
        and, you can get sync frame by selectFrames();
    */
    status_t  managePreviewFrameHoldList(int cameraId,
                                         ExynosCameraList<ExynosCameraFrame *> *outputList,
                                         ExynosCameraFrame *frame,
                                         int pipeID, bool isSrc, int32_t dstPos, ExynosCameraBufferManager *bufMgr);

    //! selectFrames
    /*!
    \param cameraId
    \param frame0
        synced wide's frame
    \param frame1
        synced tele's frame
    \remarks
        get synced frame of cameraId
        if synced       : return true and give two frames.
        else not synced : return false.
    */
    bool selectFrames(int cameraId,
                      ExynosCameraFrame **frame0, ExynosCameraList<ExynosCameraFrame *> **outputList0, ExynosCameraBufferManager **bufManager0,
                      ExynosCameraFrame **frame1, ExynosCameraList<ExynosCameraFrame *> **outputList1, ExynosCameraBufferManager **bufManager1);

    //! clear
    /*!
    \param cameraId
    \remarks
        This API make class reset.
        reset means
        send the all sync and non-sync frames to caller.
        then, caller will free the frames.
    */
    status_t  clear(int cameraId);

protected:
    DOF* m_dof[CAMERA_ID_MAX];
};

#endif //EXYNOS_CAMERA_DUAL_FRAME_SELECTOR_H
