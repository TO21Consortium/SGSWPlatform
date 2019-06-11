/*
 * Copyright 2013, Samsung Electronics Co. LTD
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

/*
 * file      ExynosCameraBufferLocker.h
 * brief     hearder file for ExynosCameraBufferLocker
 * author    Pilsun Jang(pilsun.jang@samsung.com)
 * date      2013/08/20
 *
 */

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraBufferLocker"

#include "ExynosCameraBufferLocker.h"

namespace android {
ExynosCameraBufferLocker::ExynosCameraBufferLocker()
{
    m_flagCreated = true;
    memset(m_bufferLockState, 0x00, sizeof(buffer_lock_state_t) * NUM_BAYER_BUFFERS);
    m_QNum = 0;
    printWhoAmI();
}

ExynosCameraBufferLocker::~ExynosCameraBufferLocker()
{
}

void ExynosCameraBufferLocker::init(void)
{
    EXYNOS_CAMERA_BUFFER_LOCKER_IN();

    buffer_lock_state_t initLockState;
    /* search bayer index */
    for (int i = 0; i < NUM_BAYER_BUFFERS; i++) {
        m_bufferLockState[i].bufferFcount = 0;
        m_bufferLockState[i].bufferLockState = EXYNOS_CAMERA_BUFFER_LOCKER_UNLOCKED;
    }
    m_QNum= 0;

    EXYNOS_CAMERA_BUFFER_LOCKER_OUT();
}

void ExynosCameraBufferLocker::deinit(void)
{
    EXYNOS_CAMERA_BUFFER_LOCKER_IN();

    m_indexQ.clear();
    m_QNum= 0;

    EXYNOS_CAMERA_BUFFER_LOCKER_OUT();
}


status_t ExynosCameraBufferLocker::setBufferLockByIndex(int index, bool setLock)
{
    ALOGD("[%s], (%d) index %d setLock %d", __FUNCTION__, __LINE__, index, setLock);

    if (setLock)
        m_bufferLockState[index].bufferLockState = EXYNOS_CAMERA_BUFFER_LOCKER_LOCKED;
    else
        m_bufferLockState[index].bufferLockState = EXYNOS_CAMERA_BUFFER_LOCKER_UNLOCKED;

    return NO_ERROR;
}

status_t ExynosCameraBufferLocker::setBufferLockByFcount(unsigned int fcount, bool setLock)
{
    ALOGV("[%s] (%d) fcount  %d", __FUNCTION__, __LINE__, fcount);

    for (int i = 0; i < NUM_BAYER_BUFFERS; i++) {
        if (m_bufferLockState[i].bufferFcount == fcount) {
            setBufferLockByIndex(i, setLock);

            return NO_ERROR;
        }
    }

    return INVALID_OPERATION;
}

status_t ExynosCameraBufferLocker::getBufferLockStateByIndex(int index, bool *lockState)
{
    EXYNOS_CAMERA_BUFFER_LOCKER_IN();

    if (m_bufferLockState[index].bufferLockState == EXYNOS_CAMERA_BUFFER_LOCKER_LOCKED)
        *lockState = true;
    else
        *lockState = false;


    EXYNOS_CAMERA_BUFFER_LOCKER_OUT();

    return NO_ERROR;
}

status_t ExynosCameraBufferLocker::setBufferFcount(int index, unsigned int fcount)
{
    m_bufferLockState[index].bufferFcount = fcount;

    return NO_ERROR;
}

status_t ExynosCameraBufferLocker::getBufferFcount(int index, unsigned int *fcount)
{
    *fcount = m_bufferLockState[index].bufferFcount;

    return NO_ERROR;
}

status_t ExynosCameraBufferLocker::putBufferToManageQ(int index)
{
    EXYNOS_CAMERA_BUFFER_LOCKER_IN();

    Mutex::Autolock lock(m_bufferStateLock);

    m_indexQ.push_back(index);

    EXYNOS_CAMERA_BUFFER_LOCKER_OUT();

    return NO_ERROR;
}

status_t ExynosCameraBufferLocker::getBufferToManageQ(int *index)
{
    List<int>::iterator token;

    EXYNOS_CAMERA_BUFFER_LOCKER_IN();

    Mutex::Autolock lock(m_bufferStateLock);

    if (m_indexQ.size() == 0)
        return INVALID_OPERATION;

    token = m_indexQ.begin()++;
    *index = *token;
    m_indexQ.erase(token);

    EXYNOS_CAMERA_BUFFER_LOCKER_OUT();

    return NO_ERROR;
}

status_t ExynosCameraBufferLocker::getBufferSizeQ(int *size)
{
    Mutex::Autolock lock(m_bufferStateLock);

    *size = m_indexQ.size();

    return NO_ERROR;
}

int ExynosCameraBufferLocker::getQnum(void)
{
    return m_QNum;
}

void ExynosCameraBufferLocker::incQnum(void)
{
    Mutex::Autolock lock(m_QNumLock);

    m_QNum ++;

    return;
}

void ExynosCameraBufferLocker::decQnum(void)
{
    Mutex::Autolock lock(m_QNumLock);

    m_QNum --;

    return;
}

void ExynosCameraBufferLocker::printWhoAmI()
{
    ALOGD("(%s, %d)", __FUNCTION__, __LINE__);

    return;
}

void ExynosCameraBufferLocker::printBufferState()
{
    for (int i = 0; i < NUM_BAYER_BUFFERS; i++) {
        ALOGD("(%s, %d): [%d].bufferLockState : %d" , __FUNCTION__, __LINE__, i, m_bufferLockState[i].bufferLockState);
        ALOGD("(%s, %d): [%d].bufferFcount : %d" , __FUNCTION__, __LINE__, i, m_bufferLockState[i].bufferFcount);
    }

    return;
}

void ExynosCameraBufferLocker::printBufferQ()
{
   List<int>::iterator token;
   int index;

    if (m_indexQ.size() == 0) {
        ALOGW("(%s, %d) no entry", __FUNCTION__, __LINE__);
        return;
    }

    ALOGD("(%s, %d) m_indexQ.size() = %zu", __FUNCTION__, __LINE__, m_indexQ.size());
    for (token = m_indexQ.begin(); token != m_indexQ.end(); token++) {
        index = *token;
        ALOGD("(%s, %d) index = %d", __FUNCTION__, __LINE__, index);
    }

    return;
}

}
