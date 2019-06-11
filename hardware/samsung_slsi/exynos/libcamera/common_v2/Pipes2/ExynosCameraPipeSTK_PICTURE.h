/*
**
** Copyright 2015, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_PIPE_STK_PICTURE_H
#define EXYNOS_CAMERA_PIPE_STK_PICTURE_H

#include <dlfcn.h>
#include "ExynosCameraPipe.h"
#include "ExynosCameraAutoTimer.h"

/* YUV Format Info : http://www.fourcc.org/yuv.php */
enum stain_killer_image_format {
    STK_NV21 = 0x3132564E,
    STK_YUV2 = 0x32595559,
    STK_YUYV = 0x56595559,
};

enum stain_killer_scenario {
    STK_SCENARIO_CAPTURE = 0,
    STK_SCENARIO_PREVIEW,
    STK_SCENARIO_MAX_NUM
};

typedef struct STKDynamicMeta {
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
} STK_params;

#define STK_LIBRARY_PATH "/system/lib/libstainkiller.so"

namespace android {

typedef ExynosCameraList<ExynosCameraFrame *> frame_queue_t;

class ExynosCameraPipeSTK_PICTURE : protected virtual ExynosCameraPipe {
public:
    ExynosCameraPipeSTK_PICTURE()
    {
        m_init(NULL);
    }

    ExynosCameraPipeSTK_PICTURE(
        int cameraId,
        ExynosCameraParameters *obj_param,
        bool isReprocessing,
        int32_t *nodeNums)  : ExynosCameraPipe(cameraId, obj_param, isReprocessing, nodeNums)
    {
        ALOGD("ExynosCameraPipeSTK_PICTURE Initialization Start!");
        if (m_init(nodeNums) < 0){

            bSTKInit = false;
            ALOGE("ExynosCameraPipeSTK_PICTURE Initialization failed!");

        }
        else {
            bSTKInit = true;
            ALOGD("ExynosCameraPipeSTK_PICTURE Initialization succeed!");

        }

    }

    virtual ~ExynosCameraPipeSTK_PICTURE();

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
    int                     m_stkNum;
    void                    *m_stk;
    void                    *stkHandle;
    void                    *m_stk_handle;
    bool                    bSTKInit = false;
    void                    *hSTK_object;
    pthread_t               *m_thread_id;
    camera2_shot_ext        m_shot_ext;

    void* (*init_stk)(int, int, enum stain_killer_scenario);
    pthread_t* (*run_stk)(void*, char*, char*, int);
    int (*end_stk)(void*);

    STK_params             m_stkdynamicMeta;
    ExynosCameraDurationTimer      m_timer;

    int                     m_totalCaptureCount;
    long long               m_totalProcessingTime;
};

}; /* namespace android */

#endif
