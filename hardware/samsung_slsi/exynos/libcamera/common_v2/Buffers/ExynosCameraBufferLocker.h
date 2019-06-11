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

#ifndef EXYNOS_CAMERA_BUFFER_LOCKER_H__
#define EXYNOS_CAMERA_BUFFER_LOCKER_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utils/List.h>
#include <utils/threads.h>

#include <hardware/camera.h>
#include <videodev2.h>
#include <videodev2_exynos_camera.h>

#include "fimc-is-metadata.h"
#include "ExynosCameraBuffer.h"

 /* #define EXYNOS_CAMERA_BUFFER_LOCKER_TRACE  */

namespace android {

#ifdef EXYNOS_CAMERA_BUFFER_LOCKER_TRACE
#define EXYNOS_CAMERA_BUFFER_LOCKER_IN()   ALOGD("(%s, %d):IN.." , __FUNCTION__, __LINE__)
#define EXYNOS_CAMERA_BUFFER_LOCKER_OUT()  ALOGD("(%s, %d):OUT..", __FUNCTION__, __LINE__)
#else
#define EXYNOS_CAMERA_BUFFER_LOCKER_IN()   ((void *)0)
#define EXYNOS_CAMERA_BUFFER_LOCKER_OUT()  ((void *)0)
#endif

#define EXYNOS_CAMERA_BUFFER_LOCKER_LOCKED 1
#define EXYNOS_CAMERA_BUFFER_LOCKER_UNLOCKED 2

typedef struct exynos_camera_buffer_lock_state {
    int bufferLockState;
    unsigned int bufferFcount;
} buffer_lock_state_t;

class ExynosCameraBufferLocker {
    public:
        ExynosCameraBufferLocker();
        virtual ~ExynosCameraBufferLocker();
        void     init(void);
        void     deinit(void);

        status_t setBufferLockByIndex(int index, bool setLock);
        status_t setBufferLockByFcount(unsigned int fcount, bool setLock);
        status_t getBufferLockStateByIndex(int index, bool *lockState);
        status_t setBufferFcount(int index, unsigned int fcount);
        status_t getBufferFcount(int index, unsigned int *fcount);
        status_t putBufferToManageQ(int index);
        status_t getBufferToManageQ(int *index);
        status_t getBufferSizeQ(int *size);
        int getQnum(void);
        void incQnum(void);
        void decQnum(void);

        void printWhoAmI(void);
        void printBufferState(void);
        void printBufferQ(void);

    private:
        mutable Mutex m_bufferStateLock;
        List<int> m_indexQ;
        bool m_flagCreated;
        buffer_lock_state_t m_bufferLockState[NUM_BAYER_BUFFERS];
        int m_QNum;
        mutable Mutex m_QNumLock;
};
}
#endif

