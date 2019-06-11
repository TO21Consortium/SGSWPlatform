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
 * \file      ExynosCameraActivityAutofocus.h
 * \brief     hearder file for ExynosCameraActivityAutofocus
 * \author    Sangowoo Park(sw5771.park@samsung.com)
 * \date      2012/03/07
 *
 */

#ifndef EXYNOS_CAMERA_ACTIVITY_AUTOFOCUS_H__
#define EXYNOS_CAMERA_ACTIVITY_AUTOFOCUS_H__

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

#include <videodev2.h>
#include <videodev2_exynos_camera.h>
#include <linux/vt.h>

#include <utils/RefBase.h>
#include <utils/String8.h>
#include <utils/List.h>
#include "cutils/properties.h"

#include "exynos_format.h"
#include "ExynosBuffer.h"
#include "ExynosRect.h"
#include "exynos_v4l2.h"
#include "ExynosCameraActivityBase.h"

#include "fimc-is-metadata.h"

#define UniPluginFocusData_t int

#define AUTOFOCUS_WATING_TIME        (10000)   /* 10msec */
#define AUTOFOCUS_TOTAL_WATING_TIME  (3000000) /* 3000msec */

#define MAX_FRAME_AF_DEBUG              50
#define WAIT_COUNT_FAIL_STATE                (7)
#define AUTOFOCUS_WAIT_COUNT_STEP_REQUEST    (3)

#define AUTOFOCUS_WAIT_COUNT_FRAME_COUNT_NUM (3)       /* n + x frame count */
#define AUTOFOCUS_WATING_TIME_LOCK_AF        (10000)   /* 10msec */
#define AUTOFOCUS_TOTAL_WATING_TIME_LOCK_AF  (300000)  /* 300msec */
#define AUTOFOCUS_SKIP_FRAME_LOCK_AF         (6)       /* == NUM_BAYER_BUFFERS */

#define SET_BIT(x)      (1 << x)

struct camera2_af_debug_info {
    uint16_t		CurrPos;
    uint64_t		FilterValue;
};

struct camera2_af_exif_info {
    uint16_t        INDICATOR1;   // AFAF
    uint16_t        INDICATOR2;   // AFAF

    uint16_t        AF_MODE;
    uint16_t        AF_PAN_FOCUS;
    uint16_t        AF_TYPICAL_MACRO;
    uint16_t        AF_MODULE_VERSION;
    uint16_t        AF_STATE;
    uint16_t        AF_CUR_POS;
    uint16_t        AF_TIME; // unit: (ms)
    uint16_t        FACTORY_INFO;
    uint32_t        PAF_FROM_INFO; // first 4 bytes of PDAF cal region(date)
    int32_t         APEX_BV;
    float           GYRO_INFO_X;
    float           GYRO_INFO_Y;
    float           GYRO_INFO_Z;

    uint16_t        INDICATOR3;   // AFAF
    uint16_t        INDICATOR4;   // AFAF

    uint16_t        TotalDebugInfo;
    camera2_af_debug_info    DebugInfo[MAX_FRAME_AF_DEBUG];

    uint16_t        INDICATOR5;   // AFAF
    uint16_t        INDICATOR6;   // AFAF
};

namespace android {
    /* Moved from SecCameraParameters.h */
    enum {
        FOCUS_RESULT_FAIL = 0,
        FOCUS_RESULT_SUCCESS,
        FOCUS_RESULT_CANCEL,
        FOCUS_RESULT_FOCUSING,
        FOCUS_RESULT_RESTART,
    };

class ExynosCameraActivityAutofocus : public ExynosCameraActivityBase {
public:
    enum AUTOFOCUS_MODE {
        AUTOFOCUS_MODE_BASE                     = (0),
        AUTOFOCUS_MODE_AUTO                     = (1 << 0),
        AUTOFOCUS_MODE_INFINITY                 = (1 << 1),
        AUTOFOCUS_MODE_MACRO                    = (1 << 2),
        AUTOFOCUS_MODE_FIXED                    = (1 << 3),
        AUTOFOCUS_MODE_EDOF                     = (1 << 4),
        AUTOFOCUS_MODE_CONTINUOUS_VIDEO         = (1 << 5),
        AUTOFOCUS_MODE_CONTINUOUS_PICTURE       = (1 << 6),
        AUTOFOCUS_MODE_TOUCH                    = (1 << 7),
        AUTOFOCUS_MODE_CONTINUOUS_PICTURE_MACRO = (1 << 8),
    };

    enum AUTOFOCUS_MACRO_POSITION {
        AUTOFOCUS_MACRO_POSITION_BASE           = (0),
        AUTOFOCUS_MACRO_POSITION_CENTER         = (1 << 0),
        AUTOFOCUS_MACRO_POSITION_CENTER_UP      = (1 << 1),
    };

    enum AUTOFOCUS_STATE {
        AUTOFOCUS_STATE_NONE = 0,
        AUTOFOCUS_STATE_SCANNING,
        AUTOFOCUS_STATE_SUCCEESS,
        AUTOFOCUS_STATE_FAIL,
    };

    enum {
        AUTOFOCUS_RESULT_FAIL = 0,
        AUTOFOCUS_RESULT_SUCCESS,
        AUTOFOCUS_RESULT_CANCEL,
        AUTOFOCUS_RESULT_FOCUSING,
        AUTOFOCUS_RESULT_RESTART,
    };

public:
    ExynosCameraActivityAutofocus();
    virtual ~ExynosCameraActivityAutofocus();

protected:
    int t_funcNull(void *args);
    int t_funcSensorBefore(void *args);
    int t_funcSensorAfter(void *args);
    int t_func3ABefore(void *args);
    int t_func3AAfter(void *args);
    int t_func3ABeforeHAL3(void *args);
    int t_func3AAfterHAL3(void *args);
    int t_funcISPBefore(void *args);
    int t_funcISPAfter(void *args);
    int t_funcSCPBefore(void *args);
    int t_funcSCPAfter(void *args);
    int t_funcSCCBefore(void *args);
    int t_funcSCCAfter(void *args);

public:
    bool setAutofocusMode(int autoFocusMode);
    int  getAutofocusMode(void);

    bool setFocusAreas(ExynosRect2 rect, int weight);
    bool getFocusAreas(ExynosRect2 *rect, int *weight);

    bool startAutofocus(void);
    bool stopAutofocus(void);
    bool flagAutofocusStart(void);

    bool lockAutofocus(void);
    bool unlockAutofocus(void);
    bool flagLockAutofocus(void);

    bool getAutofocusResult(bool flagLockFocus = false, bool flagStartFaceDetection = false, int numOfFace = 0);
    int  getCAFResult(void);
    int  getCurrentState(void);

    bool setRecordingHint(bool hint);
    bool getRecordingHint(void);

    bool setFaceDetection(bool toggle);
    bool setMacroPosition(int macroPosition);

    void setAfInMotionResult(bool afInMotion);
    bool getAfInMotionResult(void);

    void displayAFInfo(void);
    void displayAFStatus(void);

    static AUTOFOCUS_STATE afState2AUTOFOCUS_STATE(enum aa_afstate aaAfState);

    /* Sets FPS Value */
    void setFpsValue(int fpsValue);
    void setSamsungCamera(int flags);
    int getFpsValue();

private:
    enum AUTOFOCUS_STEP {
        AUTOFOCUS_STEP_BASE = 0,
        AUTOFOCUS_STEP_STOP,
        AUTOFOCUS_STEP_DELAYED_STOP,
        AUTOFOCUS_STEP_REQUEST,
        AUTOFOCUS_STEP_START,
        AUTOFOCUS_STEP_START_SCANNING,
        AUTOFOCUS_STEP_SCANNING,
        AUTOFOCUS_STEP_DONE,
        AUTOFOCUS_STEP_TRIGGER_START,
    };

    bool m_flagAutofocusStart;
    bool m_flagAutofocusLock;

    int  m_autoFocusMode;           /* set by user */
    int  m_interenalAutoFocusMode;  /* set by this */

    ExynosRect2 m_focusArea;
    int  m_focusWeight;

    int  m_autofocusStep;
    int  m_aaAfState;
    int  m_afState;
    int  m_aaAFMode;                /* set on h/w */
    int  m_metaCtlAFMode;
    int  m_waitCountFailState;
    int  m_stepRequestCount;
    int  m_frameCount;

    bool m_recordingHint;
    bool m_flagFaceDetection;
    int  m_macroPosition;

    uint16_t m_af_mode_info;
    uint16_t m_af_pan_focus_info;
    uint16_t m_af_typical_macro_info;
    uint16_t m_af_module_version_info;
    uint16_t m_af_state_info;
    uint16_t m_af_cur_pos_info;
    uint16_t m_af_time_info;
    uint16_t m_af_factory_info;
    uint32_t m_paf_from_info;
    uint32_t m_paf_error_code;

    unsigned int m_fpsValue;
    bool m_samsungCamera;
    bool m_afInMotionResult;

    void  m_AUTOFOCUS_MODE2AA_AFMODE(int autoFocusMode, camera2_shot_ext *shot_ext);
};
}

#endif /* EXYNOS_CAMERA_ACTIVITY_AUTOFOCUS_H__ */
