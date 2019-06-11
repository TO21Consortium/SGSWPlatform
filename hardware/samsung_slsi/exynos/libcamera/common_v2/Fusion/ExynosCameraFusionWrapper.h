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
 * \file      ExynosCameraFusionWrapper.h
 * \brief     header file for ExynosCameraFusionWrapper
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2014/10/08
 *
 * <b>Revision History: </b>
 * - 2014/10/08 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 *
 */

#ifndef EXYNOS_CAMERA_FUSION_WRAPPER_H
#define EXYNOS_CAMERA_FUSION_WRAPPER_H

#include "string.h"
#include <dlfcn.h>
#include <utils/Log.h>
#include <utils/threads.h>

#include "ExynosCameraFusionInclude.h"

using namespace android;

//#define EXYNOS_CAMERA_FUSION_WRAPPER_DEBUG

#ifdef EXYNOS_CAMERA_FUSION_WRAPPER_DEBUG
#define EXYNOS_CAMERA_FUSION_WRAPPER_DEBUG_LOG CLOGD
#else
#define EXYNOS_CAMERA_FUSION_WRAPPER_DEBUG_LOG CLOGV
#endif

#define FUSION_PROCESSTIME_STANDARD (34000)

//! ExynosCameraFusionWrapper
/*!
 * \ingroup ExynosCamera
 */
class ExynosCameraFusionWrapper
{
protected:
    friend class ExynosCameraSingleton<ExynosCameraFusionWrapper>;

    //! Constructor
    ExynosCameraFusionWrapper();
    //! Destructor
    virtual ~ExynosCameraFusionWrapper();

public:
    //! create
    virtual status_t create(int cameraId,
                            int srcWidth, int srcHeight,
                            int dstWidth, int dstHeight,
                            char *calData = NULL, int calDataSize = 0);

    //! destroy
    virtual status_t  destroy(int cameraId);

    //! flagCreate
    virtual bool      flagCreate(int cameraId);

    //! flagReady to run execute
    virtual bool      flagReady(int cameraId);

    //! execute
    virtual status_t  execute(int cameraId,
                              struct camera2_shot_ext *shot_ext[], DOF *dof[],
                              ExynosCameraBuffer srcBuffer[], ExynosRect srcRect[], ExynosCameraBufferManager *srcBufferManager[],
                              ExynosCameraBuffer dstBuffer,   ExynosRect dstRect,   ExynosCameraBufferManager *dstBufferManager);

protected:
    void      m_init(int cameraId);

    status_t  m_execute(int cameraId,
                        ExynosCameraBuffer srcBuffer[], ExynosRect srcRect[],
                        ExynosCameraBuffer dstBuffer,   ExynosRect dstRect);

protected:
    bool      m_flagCreated[CAMERA_ID_MAX];
    Mutex     m_createLock;

    int       m_mainCameraId;
    int       m_subCameraId;
    bool      m_flagValidCameraId[CAMERA_ID_MAX];

    int       m_width      [CAMERA_ID_MAX];
    int       m_height     [CAMERA_ID_MAX];
    int       m_stride     [CAMERA_ID_MAX];

    ExynosCameraDurationTimer m_emulationProcessTimer;
    int                       m_emulationProcessTime;
    float                     m_emulationCopyRatio;
};

#endif //EXYNOS_CAMERA_FUSION_WRAPPER_H
