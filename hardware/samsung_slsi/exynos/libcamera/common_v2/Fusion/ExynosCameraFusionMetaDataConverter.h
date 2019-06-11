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
 * \file      ExynosCameraFusionMetaDataConverter.h
 * \brief     header file for ExynosCameraFusionMetaDataConverter
 * \author    Sangwoo, Park(sw5771.park@samsung.com)
 * \date      2015/06/24
 *
 * <b>Revision History: </b>
 * - 2014/10/08 : Sangwoo, Park(sw5771.park@samsung.com) \n
 *   Initial version
 *
 */

#ifndef EXYNOS_CAMERA_FUSION_META_CONVERTER_H
#define EXYNOS_CAMERA_FUSION_META_CONVERTER_H

#include "string.h"
#include <utils/Log.h>
#include <camera/CameraParameters.h>

#include "ExynosCameraFusionInclude.h"

//! ExynosCameraFusionMetaDataConverter
/*!
 * \ingroup ExynosCamera
 */
class ExynosCameraFusionMetaDataConverter
{
private:
    ExynosCameraFusionMetaDataConverter(){};
    virtual ~ExynosCameraFusionMetaDataConverter(){};

public:
    static void       translateFocusPos(int cameraId,
                        camera2_shot_ext *shot_ext,
                        DOF *dof,
                        float *nearFieldCm,
                        float *lensShiftUm,
                        float *farFieldCm);

    static ExynosRect translateFocusRoi(int cameraId,
                        camera2_shot_ext *shot_ext);

    static bool       translateAfStatus(int cameraId,
                        camera2_shot_ext *shot_ext);

    static float      translateAnalogGain(int cameraId,
                        camera2_shot_ext *shot_ext);

    static void       translateScalerSetting(int cameraId,
                        camera2_shot_ext *shot_ext,
                        int perFramePos,
                        ExynosRect *yRect,
                        ExynosRect *cbcrRect);

    static void       translateCropSetting(int cameraId,
                        camera2_shot_ext *shot_ext,
                        int perFramePos,
                        ExynosRect2 *yRect,
                        ExynosRect2 *cbcrRect);

    static float      translateZoomRatio(int cameraId,
                        camera2_shot_ext *shot_ext);

    static void       translate2Parameters(int cameraId,
                        CameraParameters *params,
                        struct camera2_shot_ext *shot_ext,
                        DOF *dof,
                        ExynosRect pictureRect);

private:
    static float      m_findLensField(int cameraId,
                                      DOF *dof,
                                      float currentLensShift,
                                      bool flagFar);
};

#endif //EXYNOS_CAMERA_FUSION_META_CONVERTER_H
