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
#define LOG_TAG "ExynosCameraFrameManager"
#include <cutils/log.h>

#include "ExynosCameraFrameManager.h"

namespace android {

FrameWorker::FrameWorker(const char* name, int cameraid, FRAMEMGR_OPER::MODE operMode)
{
    m_cameraId = cameraid;
    strncpy(m_name, name, EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_operMode = operMode;
    m_init();
}

FrameWorker::~FrameWorker()
{
    m_deinit();
}

status_t FrameWorker::m_init()
{
    m_key = 0;
    m_enable = false;
    return 0;
}

status_t FrameWorker::m_deinit()
{
    return 0;
}

uint32_t FrameWorker::getKey()
{
    return m_key;
}

status_t FrameWorker::setKey(uint32_t key)
{
    m_key = key;
    return 0;
}

status_t FrameWorker::m_setEnable(bool enable)
{
    Mutex::Autolock lock(m_enableLock);
    status_t ret = FRAMEMGR_ERRCODE::OK;
    m_enable = enable;
    return ret;
}

bool FrameWorker::m_getEnable()
{
    Mutex::Autolock lock(m_enableLock);
    return m_enable;
}

status_t FrameWorker::m_delete(ExynosCameraFrame *frame)
{
    int32_t ret = FRAMEMGR_ERRCODE::OK;
    delete frame;
    frame = NULL;

    return ret;
}

CreateWorker::CreateWorker(const char* name,
                            int cameraid,
                            FRAMEMGR_OPER::MODE operMode,
                            int32_t margin):FrameWorker(name, cameraid, operMode)
{
    CLOGD("DEBUG(%s): Worker CREATE(name(%s) cameraid(%d) mode(%d) margin(%d)  ",
            __FUNCTION__, name, cameraid, operMode, margin);
    m_init();
    m_setMargin(margin);
}
CreateWorker::~CreateWorker()
{
    CLOGD("DEBUG(%s): Worker DELETE(name(%s) cameraid(%d) mode(%d) ",
            __FUNCTION__, m_name, m_cameraId, m_operMode);
    m_deinit();
}

status_t CreateWorker::m_deinit()
{
    int32_t ret = FRAMEMGR_ERRCODE::OK;
    ExynosCameraFrame *frame = NULL;
    switch (m_operMode) {
    case FRAMEMGR_OPER::ONDEMAND:
        m_setEnable(false);
        if (m_worklist != NULL) {
            delete m_worklist;
            m_worklist = NULL;
        }

        if (m_lock != NULL) {
            delete m_lock;
            m_lock = NULL;
        }

        break;
    case FRAMEMGR_OPER::SLIENT:
        m_setEnable(false);
        m_thread->requestExitAndWait();
        m_setMargin(0);
        if (m_worklist != NULL) {
            while (m_worklist->getSizeOfProcessQ() > 0) {
                m_worklist->popProcessQ(&frame);
                m_delete(frame);
                frame = NULL;
            }

            delete m_worklist;
            m_worklist = NULL;
        }

        if (m_lock != NULL) {
            delete m_lock;
            m_lock = NULL;
        }

        break;
    case FRAMEMGR_OPER::NONE:
    default:
        CLOGE("ERR(%s[%d]): operMode is invalid operMode(%d)", __FUNCTION__, __LINE__, m_operMode);
        break;
    }

    return ret;
}

Mutex* CreateWorker::getLock()
{
    return m_lock;
}

status_t CreateWorker::execute(__unused ExynosCameraFrame* inframe, ExynosCameraFrame** outframe)
{
    int32_t ret = FRAMEMGR_ERRCODE::OK;
    if (m_getEnable() == false) {
        CLOGE("ERR(%s[%d]): invalid state, Need to start Worker before execute", __FUNCTION__, __LINE__);
        ret = FRAMEMGR_ERRCODE::ERR;
        return ret;
    }

    *outframe = m_execute();
    if (*outframe == NULL) {
        CLOGE("ERR(%s[%d]): m_execute is invalid (outframe = NULL)", __FUNCTION__, __LINE__);
        ret = FRAMEMGR_ERRCODE::ERR;
    }
    return ret;
}

status_t CreateWorker::start()
{
    if (m_worklist->getSizeOfProcessQ() > 0) {
        CLOGD("DEBUG(%s[%d]):previous worklist size(%d)",
                __FUNCTION__, __LINE__, m_worklist->getSizeOfProcessQ());
        m_worklist->release();
    }

    switch (m_operMode) {
    case FRAMEMGR_OPER::ONDEMAND:
        m_setEnable(true);
        break;
    case FRAMEMGR_OPER::SLIENT:
        m_setEnable(true);
        m_thread->run();

        if (m_thread->isRunning() == false)
            m_thread->run();
        break;
    case FRAMEMGR_OPER::NONE:
    default:
        CLOGE("ERR(%s[%d]): operMode is invalid operMode(%d)", __FUNCTION__, __LINE__, m_operMode);
        break;
    }
    return 0;
}

status_t CreateWorker::stop()
{
    ExynosCameraFrame *frame = NULL;
    switch (m_operMode) {
    case FRAMEMGR_OPER::ONDEMAND:
        m_setEnable(false);
        break;
    case FRAMEMGR_OPER::SLIENT:
        m_setEnable(false);
        m_thread->requestExitAndWait();
        CLOGD("DEBUG(%s[%d]): worker stopped remove ths remained frames(%d)",
                __FUNCTION__, __LINE__, m_worklist->getSizeOfProcessQ());

        while (m_worklist->getSizeOfProcessQ() > 0) {
            m_worklist->popProcessQ(&frame);
            m_delete(frame);
            frame = NULL;
        }
        break;
    case FRAMEMGR_OPER::NONE:
    default:
        CLOGE("ERR(%s[%d]): operMode is invalid operMode(%d)", __FUNCTION__, __LINE__, m_operMode);
        break;
    }

    return 0;
}

status_t CreateWorker::m_setMargin(int32_t numOfMargin)
{
    int32_t ret = FRAMEMGR_ERRCODE::OK;
    m_numOfMargin = numOfMargin;
    return ret;
}

int32_t CreateWorker::m_getMargin()
{
    return m_numOfMargin;
}


ExynosCameraFrame* CreateWorker::m_execute()
{
    ExynosCameraFrame *frame = NULL;
    int32_t ret = NO_ERROR;

    switch (m_operMode) {
    case FRAMEMGR_OPER::ONDEMAND:
        frame = new ExynosCameraFrame();
        break;
    case FRAMEMGR_OPER::SLIENT:
        m_worklist->popProcessQ(&frame);
        if (frame == NULL) {
            ret = m_thread->run();
            m_worklist->waitAndPopProcessQ(&frame);
        }

        if (frame == NULL) {
            CLOGE("ERR(%s[%d]): getframe failed, processQ size (%d)",
                __FUNCTION__, __LINE__, m_worklist->getSizeOfProcessQ());
            CLOGE("ERR(%s[%d]): Thread return (%d) Thread Run Status (%d)",
                __FUNCTION__, __LINE__, ret, m_thread->isRunning());
        }

        m_thread->run();
        break;
    case FRAMEMGR_OPER::NONE:
    default:
        CLOGE("ERR(%s[%d]): operMode is invalid operMode(%d)", __FUNCTION__, __LINE__, m_operMode);
        break;
    }
    return frame;
}

status_t CreateWorker::m_init()
{
    int32_t ret = FRAMEMGR_ERRCODE::OK;

    switch (m_operMode) {
    case FRAMEMGR_OPER::ONDEMAND:
        m_worklist = new frame_queue_t;
        m_lock = new Mutex();
        break;
    case FRAMEMGR_OPER::SLIENT:
        m_thread =  new FrameManagerThread(this,
                                        static_cast<func_ptr_t_>(&CreateWorker::workerMain),
                                        "Create Frame Thread",
                                        PRIORITY_URGENT_DISPLAY);
        m_worklist = new frame_queue_t;
        m_lock = new Mutex();
        break;
    case FRAMEMGR_OPER::NONE:
    default:
        m_worklist = NULL;
        m_lock = NULL;
        CLOGE("ERR(%s[%d]): operMode is invalid operMode(%d)", __FUNCTION__, __LINE__, m_operMode);
        ret = FRAMEMGR_ERRCODE::ERR;
        break;
    }

    return ret;
}

bool CreateWorker::workerMain()
{
#ifdef DEBUG
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif
    int32_t ret = FRAMEMGR_ERRCODE::OK;
    bool loop = true;
    ExynosCameraFrame *frame = NULL;

    if (m_getEnable() == false) {
        loop = false;
        CLOGD("DEBUG(%s[%d]): Create worker stopped delete current frame(%d)",
            __FUNCTION__, __LINE__, m_worklist->getSizeOfProcessQ());

        while (m_worklist->getSizeOfProcessQ() > 0) {
            frame = NULL;
            ret = m_worklist->waitAndPopProcessQ(&frame);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                continue;
            }

            if (frame == NULL) {
                CLOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
                continue;
            }
            m_delete(frame);
        }

        return loop;
    }

    while (m_worklist->getSizeOfProcessQ() < m_getMargin()) {
        frame = NULL;
        frame = new ExynosCameraFrame();
        m_worklist->pushProcessQ(&frame);
    }

    // HACK, 2014.10.04
    // This sleep is added to prevent high CPU laod as busy wating.
    // But this sleep must be deleted.
    usleep(1000);

    if (m_worklist->getSizeOfProcessQ() < m_getMargin() )
        loop = true;

    return loop;

}

DeleteWorker::DeleteWorker(const char* name,
                            int cameraid,
                            FRAMEMGR_OPER::MODE operMode):FrameWorker(name,
                            cameraid,
                            operMode)
{
    CLOGD("DEBUG(%s): Worker CREATE(name(%s) cameraid(%d) mode(%d) ",
            __FUNCTION__, name, cameraid, operMode);
    m_operMode = operMode;
    m_init();
}

DeleteWorker::~DeleteWorker()
{
    CLOGD("DEBUG(%s): Worker DELETE(name(%s) cameraid(%d) mode(%d) ",
            __FUNCTION__, m_name, m_cameraId, m_operMode);
    m_deinit();
}

status_t DeleteWorker::m_deinit()
{
    int32_t ret = FRAMEMGR_ERRCODE::OK;
    ExynosCameraFrame *frame = NULL;
    switch (m_operMode) {
    case FRAMEMGR_OPER::ONDEMAND:
        m_setEnable(false);
        if (m_worklist != NULL) {
            delete m_worklist;
            m_worklist = NULL;
        }

        if (m_lock != NULL) {
            delete m_lock;
            m_lock = NULL;
        }
        break;
    case FRAMEMGR_OPER::SLIENT:
        m_setEnable(false);
        m_thread->requestExitAndWait();

        if (m_worklist != NULL) {
            while (m_worklist->getSizeOfProcessQ() > 0) {
                m_worklist->popProcessQ(&frame);
                m_delete(frame);
            }

            delete m_worklist;
            m_worklist = NULL;
        }

        if (m_lock != NULL) {
            delete m_lock;
            m_lock = NULL;
        }
        break;
    case FRAMEMGR_OPER::NONE:
    default:
        CLOGE("ERR(%s[%d]): operMode is invalid operMode(%d)", __FUNCTION__, __LINE__, m_operMode);
        break;
    }

    return ret;
}

Mutex* DeleteWorker::getLock()
{
    return m_lock;
}

status_t DeleteWorker::execute(ExynosCameraFrame* inframe, __unused ExynosCameraFrame** outframe)
{
    status_t ret = FRAMEMGR_ERRCODE::OK;
    if (m_getEnable() == false) {
        CLOGE("ERR(%s[%d]): invalid state, Need to start Worker before execute", __FUNCTION__, __LINE__);
        ret = FRAMEMGR_ERRCODE::ERR;
        return ret;
    }

    ret = m_execute(inframe);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]): m_execute is invalid ret(%d)", __FUNCTION__, __LINE__, ret);
        ret = FRAMEMGR_ERRCODE::ERR;
    }

    return ret;
}

status_t DeleteWorker::start()
{
    if (m_worklist->getSizeOfProcessQ() > 0) {
        CLOGD("DEBUG(%s[%d]):previous worklist size(%d), clear worklist",
                __FUNCTION__, __LINE__, m_worklist->getSizeOfProcessQ());
        m_worklist->release();
    }

    switch (m_operMode) {
    case FRAMEMGR_OPER::ONDEMAND:
        m_setEnable(true);
        break;
    case FRAMEMGR_OPER::SLIENT:
        m_setEnable(true);

        if (m_thread->isRunning() == false)
            m_thread->run();
        break;
    case FRAMEMGR_OPER::NONE:
    default:
        CLOGE("ERR(%s[%d]): operMode is invalid operMode(%d)", __FUNCTION__, __LINE__, m_operMode);
        break;
    }
    return 0;
}

status_t DeleteWorker::stop()
{
    ExynosCameraFrame *frame = NULL;
    switch (m_operMode) {
    case FRAMEMGR_OPER::ONDEMAND:
        m_setEnable(false);
        break;
    case FRAMEMGR_OPER::SLIENT:
        m_setEnable(false);
        m_thread->requestExitAndWait();
        CLOGD("DEBUG(%s[%d]): worker stopped remove ths remained frames(%d)",
            __FUNCTION__, __LINE__, m_worklist->getSizeOfProcessQ());

        while (m_worklist->getSizeOfProcessQ() > 0) {
            m_worklist->popProcessQ(&frame);
            m_delete(frame);
            frame = NULL;
        }
        break;
    case FRAMEMGR_OPER::NONE:
    default:
        CLOGE("ERR(%s[%d]): operMode is invalid operMode(%d)", __FUNCTION__, __LINE__, m_operMode);
        break;
    }

    return 0;
}

status_t DeleteWorker::m_execute(ExynosCameraFrame* frame)
{
    int32_t ret = FRAMEMGR_ERRCODE::OK;

    switch (m_operMode) {
    case FRAMEMGR_OPER::ONDEMAND:
        m_delete(frame);
        break;
    case FRAMEMGR_OPER::SLIENT:
        m_worklist->pushProcessQ(&frame);
        m_thread->run();
        break;
    case FRAMEMGR_OPER::NONE:
    default:
        ret = FRAMEMGR_ERRCODE::ERR;
        CLOGE("ERR(%s[%d]): operMode is invalid operMode(%d)", __FUNCTION__, __LINE__, m_operMode);
        break;
    }
    return ret;
}

status_t DeleteWorker::m_init()
{
    int32_t ret = FRAMEMGR_ERRCODE::OK;

    switch (m_operMode) {
    case FRAMEMGR_OPER::ONDEMAND:
        m_worklist = new frame_queue_t;
        m_lock = new Mutex();
        break;
    case FRAMEMGR_OPER::SLIENT:
        m_thread =  new FrameManagerThread(this,
                            static_cast<func_ptr_t_>(&DeleteWorker::workerMain),
                            "Delete Frame Thread",
                            PRIORITY_URGENT_DISPLAY);
        m_worklist = new frame_queue_t;
        m_lock = new Mutex();
        break;
    case FRAMEMGR_OPER::NONE:
    default:
        m_worklist = NULL;
        m_lock = NULL;
        ret = FRAMEMGR_ERRCODE::ERR;
        CLOGE("ERR(%s[%d]): operMode is invalid operMode(%d)", __FUNCTION__, __LINE__, m_operMode);
        break;
    }

    return ret;
}

bool DeleteWorker::workerMain()
{
#ifdef DEBUG
        ExynosCameraAutoTimer autoTimer(__FUNCTION__);
#endif
    int32_t ret = FRAMEMGR_ERRCODE::OK;
    bool loop = true;
    ExynosCameraFrame *frame = NULL;

    if (m_getEnable() == false) {
        loop = false;
        CLOGD("DEBUG(%s[%d]): Delete worker stopped delete current frame(%d)",
                __FUNCTION__, __LINE__, m_worklist->getSizeOfProcessQ());

        while (m_worklist->getSizeOfProcessQ() > 0) {
            frame = NULL;
            ret = m_worklist->waitAndPopProcessQ(&frame);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                goto func_exit;
            }

            if (frame == NULL) {
                CLOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
                goto func_exit;
            }
            m_delete(frame);
        }
        return loop;
    }

    while (m_worklist->getSizeOfProcessQ() > 0) {
        frame = NULL;
        ret = m_worklist->waitAndPopProcessQ(&frame);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):wait and pop fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            goto func_exit;
        }

        if (frame == NULL) {
            CLOGE("ERR(%s[%d]):frame is NULL", __FUNCTION__, __LINE__);
            goto func_exit;
        }

        m_delete(frame);
    }
func_exit:

    // HACK, 2014.10.04
    // This sleep is added to prevent high CPU laod as busy wating.
    // But this sleep must be deleted.
    usleep(2000);

    if (m_worklist->getSizeOfProcessQ() > 0)
        loop = true;

    return loop;

}

ExynosCameraFrameManager::ExynosCameraFrameManager(const char* name,
                                                            int cameraid,
                                                            FRAMEMGR_OPER::MODE operMode,
                                                            uint32_t dumpmargin,
                                                            uint32_t dumpmarginCount)
{
    ALOGD("DEBUG(%s): FrameManager CREATE(name(%s) cameraid(%d) mode(%d)",
            __FUNCTION__, name, cameraid, operMode);

    m_cameraId = cameraid;
    m_dumpMargin = dumpmargin;
    m_dumpMarginCount = dumpmarginCount;
    strncpy(m_name, name, EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_setOperMode(operMode);
    m_init();
}

ExynosCameraFrameManager::~ExynosCameraFrameManager()
{
    CLOGD("DEBUG(%s): FrameManager DELETE(name(%s) cameraid(%d) mode(%d)",
            __FUNCTION__, m_name, m_cameraId, m_operMode);
    m_deinit();
}

ExynosCameraFrame* ExynosCameraFrameManager::createFrame(ExynosCameraParameters *param, uint32_t framecnt, uint32_t frametype)
{
    return m_createFrame(param, framecnt, frametype);
}

status_t ExynosCameraFrameManager::deleteFrame(ExynosCameraFrame* frame)
{
    return m_deleteFrame(frame);
}

status_t ExynosCameraFrameManager::setKeybox(sp<KeyBox> keybox)
{
    Mutex::Autolock lock(m_stateLock);
    m_keybox = keybox;
    return 0;
}

sp<KeyBox> ExynosCameraFrameManager::getKeybox()
{
    Mutex::Autolock lock(m_stateLock);
    return m_keybox;
}

status_t ExynosCameraFrameManager::setWorker(int key, sp<FrameWorker> worker)
{
    Mutex::Autolock lock(m_stateLock);
    map<uint32_t, sp<FrameWorker> >::iterator iter;
    pair<map<uint32_t, sp<FrameWorker> >::iterator,bool> workerRet;
    pair<map<uint32_t, Mutex*>::iterator,bool> mutexRet;

    Mutex* locker = NULL;

    iter = m_workerList.find(key);
    if (iter != m_workerList.end()) {
        CLOGE("ERR(%s[%d]): already worker is EXIST(%d)", __FUNCTION__, __LINE__, key);
    } else {
        m_setupWorkerInfo(key, worker);
    }

    return 0;
}

status_t ExynosCameraFrameManager::setOperMode(FRAMEMGR_OPER::MODE mode)
{
   return m_setOperMode(mode);
}

int ExynosCameraFrameManager::getOperMode()
{
    return m_getOperMode();
}

status_t ExynosCameraFrameManager::m_init()
{
    int ret = FRAMEMGR_ERRCODE::OK;

    m_keybox = NULL;

    m_lock = new Mutex();
    m_setEnable(false);
    return ret;
}

status_t ExynosCameraFrameManager::m_setupWorkerInfo(int key, sp<FrameWorker> worker)
{
    int ret = FRAMEMGR_ERRCODE::OK;
    pair<map<uint32_t, sp<FrameWorker> >::iterator,bool> workerRet;
    pair<map<uint32_t, Mutex*>::iterator,bool> mutexRet;
    Mutex* locker = NULL;

    workerRet = m_workerList.insert( pair<uint32_t, sp<FrameWorker> >(key, worker));
    if (workerRet.second == false) {
        ret = FRAMEMGR_ERRCODE::ERR;
        CLOGE("ERR(%s[%d]): work insert failed(%d)", __FUNCTION__, __LINE__, key);
    }
    return ret;
}

sp<FrameWorker> ExynosCameraFrameManager::getWorker(int key)
{
    Mutex::Autolock lock(m_stateLock);
    sp<FrameWorker> ret;
    map<uint32_t, sp<FrameWorker> >::iterator iter;

    iter = m_workerList.find(key);
    if (iter != m_workerList.end()) {
        ret = iter->second;
    } else {
        CLOGE("ERR(%s[%d]): worker is not EXIST(%d)", __FUNCTION__, __LINE__, key);
        ret = NULL;
    }

    return ret;
}

sp<FrameWorker> ExynosCameraFrameManager::eraseWorker(int key)
{
    Mutex::Autolock lock(m_stateLock);
    sp<FrameWorker> ret;
    map<uint32_t, sp<FrameWorker> >::iterator iter;

    iter = m_workerList.find(key);
    if (iter != m_workerList.end()) {
        ret = iter->second;
        m_workerList.erase(iter);
    } else {
        CLOGE("ERR(%s[%d]): worker is not EXIST(%d)", __FUNCTION__, __LINE__, key);
        ret = NULL;
    }
    return ret;
}

status_t ExynosCameraFrameManager::start()
{
    Mutex::Autolock lock(m_stateLock);
    status_t ret = FRAMEMGR_ERRCODE::OK;
    sp<FrameWorker> worker = NULL;


    if (m_getEnable() == true) {
        CLOGD("DEBUG(%s[%d]):frameManager already start!!", __FUNCTION__, __LINE__);
        return ret;
    }

    {
        Mutex::Autolock _l(m_lock);
        if (m_runningFrameList.size() > 0) {
            CLOGD("DEBUG(%s[%d]):previous runningFrameList size(%d), clear list",
                    __FUNCTION__, __LINE__, m_runningFrameList.size());
            m_runningFrameList.clear();
        }
    }

    map<uint32_t, sp<FrameWorker> >::iterator iter;
    for (iter = m_workerList.begin() ; iter != m_workerList.end() ; ++iter) {
        worker = iter->second;
        ret = worker->start();
    }

    m_setEnable(true);

    return ret;
}

status_t ExynosCameraFrameManager::stop()
{
    Mutex::Autolock lock(m_stateLock);
    status_t ret = FRAMEMGR_ERRCODE::OK;
    sp<FrameWorker> worker;

    if (m_getEnable() == false) {
        CLOGD("DEBUG(%s[%d]):frameManager already stop!!", __FUNCTION__, __LINE__);
        return ret;
    }

    m_setEnable(false);

    map<uint32_t, sp<FrameWorker> >::iterator iter;
    for (iter = m_workerList.begin() ; iter != m_workerList.end() ; ++iter) {
        worker = iter->second;
        ret = worker->stop();
    }

    return ret;
}

int ExynosCameraFrameManager::deleteAllFrame()
{
    Mutex::Autolock lock(m_stateLock);
    status_t ret = NO_ERROR;
    ExynosCameraFrame *frame = NULL;

    if (m_getEnable() == true) {
        CLOGE("ERR(%s[%d]): invalid state, module state enabled, state must disabled", __FUNCTION__, __LINE__);
        return ret;
    }

    map<uint32_t, ExynosCameraFrame*>::iterator frameIter;

    m_lock->lock();

    int runningFrameListSize = m_runningFrameList.size();

    if (runningFrameListSize == 0) {
        CLOGD("DEBUG(%s[%d]):No memory leak detected m_runningFrameList.size()(%d)",
                __FUNCTION__, __LINE__, runningFrameListSize);
    } else {
        CLOGW("WARN(%s[%d]):%d memory leak detected m_runningFrameList.size()(%d)",
                __FUNCTION__, __LINE__, runningFrameListSize, runningFrameListSize);
    }

    for (frameIter = m_runningFrameList.begin() ; frameIter != m_runningFrameList.end() ;) {
        frame = frameIter->second;

        CLOGW("WARN(%s[%d]):delete memory leak detected FrameKey(%d) HAL-FrameCnt(%d) FrameType(%u)",
            __FUNCTION__, __LINE__, frame->getUniqueKey(), frame->getFrameCount(), frame->getFrameType());

        m_runningFrameList.erase(frameIter++);
        SAFE_DELETE(frame);
    }

    m_runningFrameList.clear();

    m_lock->unlock();

    return ret;
}

ExynosCameraFrame* ExynosCameraFrameManager::m_createFrame(ExynosCameraParameters *param,
                                                                uint32_t framecnt,
                                                                uint32_t frametype)
{
    int ret = FRAMEMGR_ERRCODE::OK;
    uint32_t key = 0;
    ExynosCameraFrame *frame = NULL;
    map<uint32_t, sp<FrameWorker> >::iterator iter;
    sp<FrameWorker> worker = NULL;
    if (m_getEnable() == false) {
        CLOGE("ERR(%s[%d]): module state disabled", __FUNCTION__, __LINE__);
        return frame;
    }

    iter = m_workerList.find(FRAMEMGR_WORKER::CREATE);
    worker = iter->second;

    switch (m_operMode) {
    case FRAMEMGR_OPER::ONDEMAND:
    case FRAMEMGR_OPER::SLIENT:
        worker->execute(NULL, &frame);
        if (frame == NULL) {
            CLOGE("ERR(%s[%d]): Frame is NULL", __FUNCTION__, __LINE__);
            return frame;
        }
        frame->setUniqueKey(m_keybox->createKey());
        frame->setFrameInfo(param ,framecnt, frametype);
        ret = m_insertFrame(frame, &m_runningFrameList, m_lock);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]): m_insertFrame is Failed ", __FUNCTION__, __LINE__);
            return NULL;
        }
        m_dumpFrameMargin();
        break;
    }

    return frame;
}

status_t ExynosCameraFrameManager::m_deleteFrame(ExynosCameraFrame* frame)
{
    int ret = FRAMEMGR_ERRCODE::OK;
    ExynosCameraFrame* removeFrame = NULL;
    map<uint32_t, sp<FrameWorker> >::iterator iter;
    sp<FrameWorker> worker;

    if (m_getEnable() == false) {
        CLOGE("ERR(%s[%d]): module state disabled", __FUNCTION__, __LINE__);
        ret = FRAMEMGR_ERRCODE::ERR;
        return ret;
    }

    iter = m_workerList.find(FRAMEMGR_WORKER::DELETE);
    worker = iter->second;

    switch (m_operMode) {
    case FRAMEMGR_OPER::ONDEMAND:
    case FRAMEMGR_OPER::SLIENT:
        removeFrame = m_removeFrame(frame, &m_runningFrameList, m_lock);
        if (removeFrame == NULL) {
#ifndef USE_FRAME_REFERENCE_COUNT
            CLOGE("ERR(%s[%d]): Frame is NULL", __FUNCTION__, __LINE__);
#else
            /* in case of ths USE_FRAME_REFERENCE_COUNT function */
            /* The frame must delete that refcount is ZERO */
#endif
            ret = FRAMEMGR_ERRCODE::ERR;

            return ret;
        }
        worker->execute(frame, NULL);
        m_dumpFrameMargin();
        break;
    case FRAMEMGR_OPER::NONE:
    default:
        ret = FRAMEMGR_ERRCODE::ERR;
        break;
    }

    return ret;
}

status_t ExynosCameraFrameManager::dump()
{
    status_t ret = FRAMEMGR_ERRCODE::OK;
    m_dumpFrame();
    return ret;
}

void ExynosCameraFrameManager::m_dumpFrameMargin()
{
    int32_t frameCount = 0;
    static uint32_t count = 0; ;


    frameCount = m_runningFrameList.size();

    if (m_dumpMargin < frameCount)
    {
        count++;
        if (count % m_dumpMarginCount == 0) {
            CLOGW("WARN(%s[%d]):Suspect memory leak of Frame. m_dumpMargin(%d) m_dumpMarginCount(%d) m_runningFrameList.size()(%zu)",
                __FUNCTION__, __LINE__, m_dumpMargin, m_dumpMarginCount, m_runningFrameList.size());

            m_dumpFrame();
        }
    }

}

status_t ExynosCameraFrameManager::m_dumpFrame()
{
    status_t ret = FRAMEMGR_ERRCODE::OK;
    map<uint32_t, ExynosCameraFrame*>::iterator frameIter;
    ExynosCameraFrame *frame = NULL;

    m_lock->lock();
    CLOGD("DEBUG(%s[%d]):(%s) ++++++++++++++++++++", __FUNCTION__, __LINE__, m_name);

    CLOGD("DEBUG(%s[%d]):m_dumpMargin(%d) m_dumpMarginCount(%d) m_runningFrameList.size()(%zu)",
        __FUNCTION__, __LINE__, m_dumpMargin, m_dumpMarginCount, m_runningFrameList.size());
    int testCount = 0;
    for (frameIter = m_runningFrameList.begin() ; frameIter != m_runningFrameList.end() ; ++frameIter) {
        frame = frameIter->second;
        CLOGD("DEBUG(%s[%d]): RunningFrame UniqueKey(%d)", __FUNCTION__, __LINE__, frame->getUniqueKey());
        if (++testCount >= 5)
            break;
    }

    CLOGD("DEBUG(%s[%d]):(%s) ------------------------------", __FUNCTION__, __LINE__, m_name);
    m_lock->unlock();

    /* HACK FOR DEBUGGING : ANR DETECTION */
    if (m_runningFrameList.size() > 300) {
#ifdef AVOID_ASSERT_FRAME
        CLOGE("ERR(%s[%d]): too many frames - m_runningFrameList.size(%d)",
                __FUNCTION__, __LINE__, m_runningFrameList.size());
#else
        android_printAssert(NULL, LOG_TAG, "HACK For ANR DEBUGGING");
#endif
    }

    return ret;
}

status_t ExynosCameraFrameManager::m_deinit()
{
    status_t ret = FRAMEMGR_ERRCODE::OK;
    ExynosCameraFrame *frame = NULL;
    sp<FrameWorker> worker;

    map<uint32_t, sp<FrameWorker> >::iterator iter;
    for (iter = m_workerList.begin() ; iter != m_workerList.end() ; ++iter) {
        worker = iter->second;
        m_workerList.erase(iter++);
        worker = NULL;
    }

    m_workerList.clear();

    map<uint32_t, ExynosCameraFrame*>::iterator frameIter;

    if (m_lock == NULL) {
        CLOGE("ERR(%s[%d]): lock is NULL", __FUNCTION__, __LINE__);
        ret = FRAMEMGR_ERRCODE::ERR;
        return ret;
    }

    m_lock->lock();
    for (frameIter = m_runningFrameList.begin() ; frameIter != m_runningFrameList.end() ;) {
        frame = frameIter->second;
        m_runningFrameList.erase(frameIter++);
        delete frame;
        frame = NULL;
    }
    m_runningFrameList.clear();
    m_lock->unlock();

    CLOGD("DEBUG(%s[%d]): delete m_lock", __FUNCTION__, __LINE__);
    delete m_lock;
    m_lock = NULL;

    if (m_keybox != NULL) {
        CLOGD("DEBUG(%s[%d]): delete m_keybox", __FUNCTION__, __LINE__);
        m_keybox = NULL;
    }

    return ret;
}

status_t ExynosCameraFrameManager::m_insertFrame(ExynosCameraFrame* frame,
                                                    map<uint32_t,
                                                    ExynosCameraFrame*> *list,
                                                    Mutex *lock)
{
    lock->lock();
    pair<map<uint32_t, ExynosCameraFrame*>::iterator,bool> listRet;
    int32_t ret = FRAMEMGR_ERRCODE::OK;

    listRet = list->insert( pair<uint32_t, ExynosCameraFrame*>(frame->getUniqueKey(), frame));
    if (listRet.second == false) {
        ret = FRAMEMGR_ERRCODE::ERR;
        CLOGE("ERR(%s[%d]): insertFrame fail frame already exist!! HAL frameCnt( %d ) UniqueKey ( %u ) ",
                __FUNCTION__, __LINE__, frame->getFrameCount(), frame->getUniqueKey());
    }
    lock->unlock();
    return ret;
}

ExynosCameraFrame* ExynosCameraFrameManager::m_removeFrame(ExynosCameraFrame* frame,
                                                                map<uint32_t,
                                                                ExynosCameraFrame*> *list,
                                                                Mutex *lock)
{
    map<uint32_t, ExynosCameraFrame*>::iterator iter;
    pair<map<uint32_t, ExynosCameraFrame*>::iterator,bool> listRet;
    ExynosCameraFrame *ret = NULL;

    lock->lock();
    iter = list->find(frame->getUniqueKey());
    if (iter != list->end()) {
        ret = iter->second;
#ifdef USE_FRAME_REFERENCE_COUNT
        if (ret->getRef() != 0) {
            lock->unlock();
            return NULL;
        }
#endif
        list->erase( iter );
    } else {
        CLOGE("ERR(%s[%d]): frame is not EXIST HAL-FrameCnt(%d) UniqueKey(%d)",
                __FUNCTION__, __LINE__, frame->getFrameCount(), frame->getUniqueKey());
    }
    lock->unlock();

    return ret;
}

status_t ExynosCameraFrameManager::m_setOperMode(FRAMEMGR_OPER::MODE mode)
{
    int32_t ret = FRAMEMGR_ERRCODE::OK;
    switch (mode) {
    case FRAMEMGR_OPER::ONDEMAND:
    case FRAMEMGR_OPER::SLIENT:
        m_operMode = mode;
        break;
    default:
        m_operMode = -1;
        ret = FRAMEMGR_ERRCODE::ERR;
        CLOGE("ERR(%s[%d]): operMode is invalid operMode(%d)", __FUNCTION__, __LINE__, m_operMode);
        break;
    }

    return ret;
}

int ExynosCameraFrameManager::m_getOperMode()
{
    return m_operMode;
}

status_t ExynosCameraFrameManager::m_setEnable(bool enable)
{
    Mutex::Autolock lock(m_enableLock);
    status_t ret = FRAMEMGR_ERRCODE::OK;
    m_enable = enable;
    return ret;
}

bool ExynosCameraFrameManager::m_getEnable()
{
    Mutex::Autolock lock(m_enableLock);
    return m_enable;
}




//************FrameManager END *************************//




}; /* namespace android */
