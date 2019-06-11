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

#ifndef EXYNOS_CAMERA_COUNTER_H
#define EXYNOS_CAMERA_COUNTER_H

namespace android {

class ExynosCameraCounter {
public:
    /* Constructor */
    ExynosCameraCounter()
    {
        m_count = 0;
        m_compensation = 0;
    };

    void setCount(int cnt)
    {
        Mutex::Autolock lock(m_lock);
        m_count = cnt + m_compensation;
    };

    void setCompensation(int compensation)
    {
        Mutex::Autolock lock(m_lock);
        m_compensation = compensation;
    };

    int getCompensation(void)
    {
        Mutex::Autolock lock(m_lock);
        return m_compensation;
    };


    void clearCount(void)
    {
        Mutex::Autolock lock(m_lock);
        m_count = 0;
        m_compensation = 0;
    };

    int getCount()
    {
        Mutex::Autolock lock(m_lock);
        return m_count;
    };

    void decCount()
    {
        Mutex::Autolock lock(m_lock);
        if (m_count > 0)
            m_count--;
        else
            m_count = 0;
    };

private:
    mutable Mutex       m_lock;
    int                 m_count;
    int                 m_compensation;
};

}; /* namespace android */

#endif
