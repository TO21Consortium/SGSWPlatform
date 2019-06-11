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
 * distributed under the License is distributed toggle an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * \file      ExynosCameraList.h
 * \brief     hearder file for CAMERA HAL MODULE
 * \author    Hyeonmyeong Choi(hyeon.choi@samsung.com)
 * \date      2013/03/05
 *
 */

#ifndef EXYNOS_CAMERA_LIST_H__
#define EXYNOS_CAMERA_LIST_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utils/threads.h>

#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/List.h>
#include "cutils/properties.h"

#define WAIT_TIME (150 * 1000000)

using namespace android;

enum LIST_CMD {
    WAKE_UP = 1,
};

template<typename T>
class ExynosCameraList {
public:
    typedef List<T> listType;
    typedef typename listType::iterator iterator;

    ExynosCameraList()
    {
        m_statusException = NO_ERROR;
        m_waitProcessQ = false;
        m_waitTime = WAIT_TIME;
        m_thread = NULL;
    }

    ExynosCameraList(sp<Thread> thread)
    {
        m_statusException = NO_ERROR;
        m_waitProcessQ = false;
        m_waitTime = WAIT_TIME;
        m_thread = NULL;

        m_thread = thread;
    }

    ~ExynosCameraList()
    {
        release();
    }

    void setup(sp<Thread> thread)
    {
        m_processQMutex.lock();
        m_thread = thread;
        m_processQMutex.unlock();
    }

    void wakeupAll(void)
    {
        setStatusException(TIMED_OUT);
        if (m_waitProcessQ)
            m_processQCondition.signal();
    }

    void sendCmd(uint32_t cmd)
    {
        switch (cmd) {
        case WAKE_UP:
            wakeupAll();
            break;
        default:
            ALOGE("ERR(%s):unknown cmd(%d)", __FUNCTION__, cmd);
            break;
        }
    }

    void setStatusException(status_t exception)
    {
        Mutex::Autolock lock(m_flagMutex);
        m_statusException = exception;
    }

    status_t getStatusException(void)
    {
        Mutex::Autolock lock(m_flagMutex);
        return m_statusException;
    }

    /* Process Queue */
    void pushProcessQ(T *buf)
    {
        Mutex::Autolock lock(m_processQMutex);
        m_processQ.push_back(*buf);

        if (m_waitProcessQ)
            m_processQCondition.signal();
        else if (m_thread != NULL && m_thread->isRunning() == false)
            m_thread->run();
    };

    status_t popProcessQ(T *buf)
    {
        iterator r;

        Mutex::Autolock lock(m_processQMutex);
        if (m_processQ.empty())
            return TIMED_OUT;

        r = m_processQ.begin()++;
        *buf = *r;
        m_processQ.erase(r);

        return OK;
    };

    status_t waitAndPopProcessQ(T *buf)
    {
        iterator r;

        status_t ret;
        m_processQMutex.lock();
        if (m_processQ.empty()) {
            m_waitProcessQ = true;

            setStatusException(NO_ERROR);
            ret = m_processQCondition.waitRelative(m_processQMutex, m_waitTime);
            m_waitProcessQ = false;

            if (ret < 0) {
                if (ret == TIMED_OUT)
                    ALOGV("DEBUG(%s):Time out, Skip to pop process Q", __FUNCTION__);
                else
                    ALOGE("ERR(%s):Fail to pop processQ", __FUNCTION__);

                m_processQMutex.unlock();
                return ret;
            }

            ret = getStatusException();
            if (ret != NO_ERROR) {
                if (ret == TIMED_OUT) {
                    ALOGV("DEBUG(%s):return CAM_ECANCELED.(%d).", __FUNCTION__, ret);
                } else {
                    ALOGW("WARN(%s[%d]): Exception status(%d)", __FUNCTION__, __LINE__, ret);
                }
                m_processQMutex.unlock();
                return ret;
            }
        }

        if (m_processQ.empty()) {
            ALOGE("ERR(%s[%d]): processQ is empty, invalid state", __FUNCTION__, __LINE__);
            m_processQMutex.unlock();
            return INVALID_OPERATION;
        }

        r = m_processQ.begin();
        *buf = *r;
        m_processQ.erase(r);

        m_processQMutex.unlock();
        return OK;
    };

    int getSizeOfProcessQ(void)
    {
        Mutex::Autolock lock(m_processQMutex);
        return m_processQ.size();
    };

    /* release both Queue */
    void release(void)
    {
        setStatusException(TIMED_OUT);

        m_processQMutex.lock();
        if (m_waitProcessQ)
            m_processQCondition.signal();

        m_processQ.clear();
        m_processQMutex.unlock();
    };

    void setWaitTime(uint64_t waitTime)
    {
        m_waitTime = waitTime;
        ALOGV("DEBUG(%s):m_waitTime : %llu", __FUNCTION__, m_waitTime);
    }

    bool isWaiting(void) {
        Mutex::Autolock lock(m_processQMutex);
        return m_waitProcessQ;
    }

private:
    List<T>             m_processQ;
    Mutex               m_processQMutex;
    Mutex               m_flagMutex;
    mutable Condition   m_processQCondition;
    bool                m_waitProcessQ;
    status_t            m_statusException;
    uint64_t            m_waitTime;

    sp<Thread>          m_thread;
};
#endif
