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

/*!
 * \file      ExynosCameraAutoTimer.h
 * \brief     hearder file for ExynosCameraAutoTimer
 * \author    Sangwoo.Park(sw5771.park@samsung.com)
 * \date      2013/06/18
 *
 * <b>Revision History: </b>
 * - 2013/06/18 : Sangwoo.Park(sw5771.park@samsung.com) \n
 *   Initial version
 *
 */

#ifndef EXYNOS_CAMERA_AUTO_TIMER_H
#define EXYNOS_CAMERA_AUTO_TIMER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <utils/Timers.h>
#include <utils/threads.h>

namespace android {

class ExynosCameraDurationTimer {
public:
    ExynosCameraDurationTimer()
    {
        memset(&m_startTime, 0x00, sizeof(struct timeval));
        memset(&m_stopTime, 0x00, sizeof(struct timeval));
    }
    ~ExynosCameraDurationTimer() {}

    void start()
    {
        gettimeofday(&m_startTime, NULL);
    };

    void stop()
    {
        gettimeofday(&m_stopTime, NULL);
    };

    uint64_t durationMsecs() const
    {
        nsecs_t stop  = ((nsecs_t)m_stopTime.tv_sec) * 1000LL + ((nsecs_t)m_stopTime.tv_usec) / 1000LL;
        nsecs_t start = ((nsecs_t)m_startTime.tv_sec) * 1000LL + ((nsecs_t)m_startTime.tv_usec) / 1000LL;

        return stop - start;
    };

    uint64_t durationUsecs() const
    {
        nsecs_t stop  = ((nsecs_t)m_stopTime.tv_sec) * 1000000LL + ((nsecs_t)m_stopTime.tv_usec);
        nsecs_t start = ((nsecs_t)m_startTime.tv_sec) * 1000000LL + ((nsecs_t)m_startTime.tv_usec);

        return stop - start;
    };

private:
    struct timeval  m_startTime;
    struct timeval  m_stopTime;
};

class ExynosCameraAutoTimer {
private:
    ExynosCameraAutoTimer(void)
    {}

public:
    inline ExynosCameraAutoTimer(char *strLog)
    {
        if (m_create(strLog) == false)
            ALOGE("ERR(%s):m_create() fail", __func__);
    }

    inline ExynosCameraAutoTimer(const char *strLog)
    {
        char *strTemp = (char*)strLog;

        if (m_create(strTemp) == false)
            ALOGE("ERR(%s):m_create() fail", __func__);
    }

    inline virtual ~ExynosCameraAutoTimer()
    {
        uint64_t durationTime;

        m_timer.stop();

        durationTime = m_timer.durationMsecs();

        if (m_logStr) {
            ALOGD("DEBUG:duration time(%5d msec):(%s)",
                (int)durationTime, m_logStr);
        } else {
            ALOGD("DEBUG:duration time(%5d msec):(NULL)",
                (int)durationTime);
        }
    }

private:
    bool m_create(char *strLog)
    {
        m_logStr = strLog;

        m_timer.start();

        return true;
    }

private:
    ExynosCameraDurationTimer m_timer;
    char         *m_logStr;
};

}; /* namespace android */

#endif /* EXYNOS_CAMERA_AUTO_TIMER_H */
