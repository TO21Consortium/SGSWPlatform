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

#ifndef EXYNOS_CAMERA_PIPE_UVS_H
#define EXYNOS_CAMERA_PIPE_UVS_H

#include <dlfcn.h>
#include "ExynosCameraPipe.h"
#include "ExynosCameraAutoTimer.h"

typedef struct UVSDynamicMeta {
    char    *src_y;
    char    *src_u;
    char    *src_v;
    int     bittage;
    int     width;
    int     height;
    int     binning_y;
    int     binning_x;
    int     radial_alpha_R;
    int     radial_alpha_G;
    int     radial_alpha_B;
    int     radial_biquad_A;
    int     radial_biquad_B;
    int     radial_biquad_shift_adder;
    int     radial_center_x;
    int     radial_center_y;
    int     radial_green;
    int     radial_refine_enable;
    int     radial_refine_luma_min;
    int     radial_refine_luma_max;
    int     pedestal_R;
    int     pedestal_G;
    int     pedestal_B;
    int     desat_low_U;
    int     desat_high_U;
    int     desat_low_V;
    int     desat_high_V;
    int     desat_shift;
    int     desat_luma_max;
    int     desat_singleside;
    int     desat_luma_offset;
    int     desat_gain_offset;
    int     out_offset_R;
    int     out_offset_G;
    int     out_offset_B;
} UVS_params;

#define UVS_LIBRARY_PATH "/vendor/lib/libexynosuvs.so"

namespace android {

typedef ExynosCameraList<ExynosCameraFrame *> frame_queue_t;

class ExynosCameraPipeUVS : protected virtual ExynosCameraPipe {
public:
    ExynosCameraPipeUVS()
    {
        m_init(NULL);
    }

    ExynosCameraPipeUVS(
        int cameraId,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums)  : ExynosCameraPipe(cameraId, obj_param, isReprocessing, nodeNums)
    {
        ALOGD("ExynosCameraPipeUVS Initialization Start!");
        if (m_init(nodeNums) < 0){

            bUVSInit = false;
            ALOGE("ExynosCameraPipeUVS Initialization failed!");

        }
        else {
            bUVSInit = true;
            ALOGD("ExynosCameraPipeUVS Initialization succeed!");

        }

    }

    virtual ~ExynosCameraPipeUVS();

    virtual status_t        create(int32_t *sensorIds = NULL);
    virtual status_t        destroy(void);

    virtual status_t        start(void);
    virtual status_t        stop(void);
    virtual status_t        startThread(void);

protected:
    virtual status_t        m_run(void);
    virtual bool            m_mainThreadFunc(void);

private:
    status_t                m_init(int32_t *nodeNums);

private:
    int                     m_uvsNum;
    void                   *m_uvs;
    void                    *uvsHandle;
    bool                    bUVSInit = false;
    void                    *hUVS_object;

    int (*init_uvs)(UVS_params *);
    int (*run_uvs)(UVS_params *);
    int (*end_uvs)();

    UVS_params             m_uvsdynamicMeta;
    ExynosCameraDurationTimer      m_timer;

};

}; /* namespace android */

#endif
