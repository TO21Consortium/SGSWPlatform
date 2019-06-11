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

/*!
 * \file      ExynosCameraFrameManager.h
 * \brief     header file for ExynosCameraFrameManager
 * \author    suyoung lee(suyoung80.lee@samsung.com)
 * \date      2014/05/08
*/

#ifndef EXYNOS_CAMERA_FRAME_MANAGER_H
#define EXYNOS_CAMERA_FRAME_MANAGER_H

#include <utils/threads.h>
#include <utils/RefBase.h>
#include <binder/MemoryBase.h>
#include <binder/MemoryHeapBase.h>
#include <hardware/camera.h>
#include <hardware/gralloc.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <media/hardware/MetadataBufferType.h>
#include <map>
#include <list>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include "csc.h"

#include "ExynosCameraParameters.h"
#include "ExynosCameraThread.h"
#include "ExynosCameraFrame.h"
#include "ExynosCameraList.h"

namespace android {

using namespace std;

class FrameWorker;

typedef ExynosCameraList<ExynosCameraFrame *> frame_queue_t;
typedef ExynosCameraThread<FrameWorker> FrameManagerThread;

namespace FRAMEMGR_ERRCODE {
    enum STATUS {
        OK = 0,
        ERR = -1,
        ERR_STATE_
    };
};

namespace FRAMEMGR_OPER {
    enum MODE {
        NONE      = 0,
        ONDEMAND  = 1,
        SLIENT    = 2
    };
};

namespace FRAMEMGR_WORKER {
    enum TYPE {
        NONE      = 0,
        CREATE    = 1,
        DELETE    = 2
    };
};

namespace DEBUG_LEVEL {
    enum STATUS {
        ERROR = 1,
        DEBUG = 2,
        TRACE = 3,
        DEFAULT = DEBUG
    };
};



class KeyBox : public virtual RefBase{
public:
    KeyBox(const char* name, int cameraid) {

        m_cameraId = cameraid;
        strncpy(m_name, name, EXYNOS_CAMERA_NAME_STR_SIZE - 1);

        init();
    }
    virtual ~KeyBox() { }

    status_t init() {
        m_uniqueKey = 0;
        return 0;
    }

    uint32_t createKey() {
        Mutex::Autolock lock(m_lock);

        return m_uniqueKey++;
    }

    uint32_t getKey() {
        Mutex::Autolock lock(m_lock);

        return m_uniqueKey;
    }
    status_t setKey(uint32_t key){
        Mutex::Autolock lock(m_lock);

        m_uniqueKey = key;
        return 0;
    }

private:
    int                              m_cameraId;
    char                             m_name[EXYNOS_CAMERA_NAME_STR_SIZE];
    uint32_t                         m_uniqueKey;
    Mutex                            m_lock;

};


class FrameWorker : public virtual RefBase {

public:
    FrameWorker() {};
    FrameWorker(const char* name, int cameraid, FRAMEMGR_OPER::MODE m_operMode);
    virtual ~FrameWorker();

    virtual uint32_t        getKey();
    virtual status_t        setKey(uint32_t key);
    virtual Mutex*          getLock() = 0;

    virtual status_t        execute(ExynosCameraFrame* inframe, ExynosCameraFrame** outframe) = 0;
    virtual status_t        start() = 0;
    virtual status_t        stop() = 0;


protected:
    virtual status_t        m_deinit();
    virtual status_t        m_init();

    virtual status_t        m_delete(ExynosCameraFrame *frame);

    virtual status_t        m_setEnable(bool enable);
    virtual bool            m_getEnable();
    virtual bool            workerMain() = 0;

private:


protected:
    int                     m_cameraId;
    char                    m_name[EXYNOS_CAMERA_NAME_STR_SIZE];
    sp<FrameManagerThread>  m_thread;
    FRAMEMGR_OPER::MODE     m_operMode;

private:
    uint32_t                m_key;
    bool                    m_enable;
    Mutex                   m_enableLock;

};

typedef bool (FrameWorker::*func_ptr_t_)();



class CreateWorker : public FrameWorker{

public:
    CreateWorker() {};
    CreateWorker(const char* name, int cameraid, FRAMEMGR_OPER::MODE operMode, int32_t margin);
    virtual ~CreateWorker();

    virtual Mutex*          getLock();

    virtual status_t        execute(ExynosCameraFrame* inframe, ExynosCameraFrame** outframe);
    virtual status_t        start();
    virtual status_t        stop();

protected:
    virtual bool            workerMain();
    virtual status_t        m_init();
    virtual status_t        m_deinit();

private:
    virtual status_t        m_setMargin(int32_t numOfMargin);
    virtual int32_t         m_getMargin();
    virtual ExynosCameraFrame* m_execute();


private:
    frame_queue_t           *m_worklist;
    Mutex                   *m_lock;
    int32_t                 m_numOfMargin;

};

class DeleteWorker : public FrameWorker{

public:
    DeleteWorker() {};
    DeleteWorker(const char* name, int cameraid, FRAMEMGR_OPER::MODE operMode);
    virtual ~DeleteWorker();

    virtual Mutex*          getLock();

    virtual status_t        execute(ExynosCameraFrame* inframe, ExynosCameraFrame** outframe);
    virtual status_t        start();
    virtual status_t        stop();

protected:
    virtual bool            workerMain();
    virtual status_t        m_deinit();
    virtual status_t        m_init();

private:
    virtual status_t        m_execute(ExynosCameraFrame* frame);

private:
    frame_queue_t           *m_worklist;
    Mutex                   *m_lock;

};

class ExynosCameraFrameManager {

public:
    ExynosCameraFrameManager() {};
    ExynosCameraFrameManager(const char* name, int cameraid, FRAMEMGR_OPER::MODE operMode, uint32_t dumpmargin, uint32_t dumpmarginCount);
    virtual ~ExynosCameraFrameManager();

    ExynosCameraFrame*  createFrame(ExynosCameraParameters *param, uint32_t framecnt, uint32_t frametype = 0);

    status_t            deleteFrame(ExynosCameraFrame* frame);
    status_t            setKeybox(sp<KeyBox> keybox);
    sp<KeyBox>          getKeybox();
    status_t            setWorker(int key, sp<FrameWorker> worker);
    sp<FrameWorker>     getWorker(int key);
    sp<FrameWorker>     eraseWorker(int key);

    status_t            setOperMode(FRAMEMGR_OPER::MODE mode);
    int                 getOperMode();

    status_t            start();
    status_t            stop();

    int                 deleteAllFrame();

    status_t            dump();

private:
    status_t            m_init();

    status_t            m_deinit();
    status_t            m_setupWorkerInfo(int key, sp<FrameWorker> worker);

    ExynosCameraFrame*  m_createFrame(ExynosCameraParameters *param, uint32_t framecnt, uint32_t frametype = 0);
    status_t            m_deleteFrame(ExynosCameraFrame* frame);


    status_t            m_insertFrame(ExynosCameraFrame* frame, map<uint32_t, ExynosCameraFrame*> *list, Mutex *lock);
    ExynosCameraFrame*  m_removeFrame(ExynosCameraFrame* frame, map<uint32_t, ExynosCameraFrame*> *list, Mutex *lock);
    status_t            m_delete(ExynosCameraFrame* frame);

    status_t            m_setOperMode(FRAMEMGR_OPER::MODE mode);
    int                 m_getOperMode();

    status_t            m_setEnable(bool enable);
    bool                m_getEnable();

    status_t            m_incRef(ExynosCameraFrame* frame);
    status_t            m_decRef(ExynosCameraFrame* frame);

    void                m_dumpFrameMargin();
    status_t            m_dumpFrame();
    status_t            m_dumpRef();


public:


private:
    int                               m_cameraId;
    char                              m_name[EXYNOS_CAMERA_NAME_STR_SIZE];
    map<uint32_t, ExynosCameraFrame*> m_runningFrameList;
    Mutex                             *m_lock;
    sp<KeyBox>                        m_keybox;
    map<uint32_t, sp<FrameWorker> >   m_workerList;

    int                                m_operMode;
    bool                               m_enable;
    Mutex                              m_enableLock;
    int32_t                            m_dumpMargin;
    int32_t                            m_dumpMarginCount;
    Mutex                              m_stateLock;



};




}; /* namespace android */
#endif
