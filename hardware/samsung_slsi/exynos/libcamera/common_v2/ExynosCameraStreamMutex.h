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
 * \file      ExynosCameraStreamMutex.h
 * \brief     header file for ExynosCameraStreamMutex
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2016/01/29
 *
 * <b>Revision History: </b>
 * - 2016/01/29 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 *
 */

#ifndef EXYNOS_CAMERA_STREAM_MUTEX_H
#define EXYNOS_CAMERA_STREAM_MUTEX_H

#include <utils/threads.h>

#include "ExynosCameraSingleton.h"

using namespace android;

/* Class declaration */
//! ExynosCameraStreamMutex is global mutex for node open ~ V4L2_CID_IS_END_OF_STREAM.
/*!
 * \ingroup ExynosCamera
 */
class ExynosCameraStreamMutex: public ExynosCameraSingleton<ExynosCameraStreamMutex>
{
protected:
    friend class ExynosCameraSingleton<ExynosCameraStreamMutex>;

    //! Constructor
    ExynosCameraStreamMutex(){};

    //! Destructor
    virtual ~ExynosCameraStreamMutex(){};

public:
    //! setInfo
    /*!
    \remarks
        return streamMutex
    */
    Mutex *getStreamMutex(void)
    {
        return &m_streamMutex;
    }

private:
    Mutex m_streamMutex;
};

#endif //EXYNOS_CAMERA_STREAM_MUTEX_H
