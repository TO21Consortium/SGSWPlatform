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
 * \file      ExynosCameraActivityFlash.h
 * \brief     hearder file for CAMERA HAL MODULE
 * \author    Pilsun Jang(pilsun.jang@samsung.com)
 * \date      2012/12/19
 *
 */

#ifndef EXYNOS_CAMERA_ACTIVITY_FLASH_H__
#define EXYNOS_CAMERA_ACTIVITY_FLASH_H__

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
#include "ExynosJpegEncoderForCamera.h"
#include "ExynosExif.h"
#include "exynos_v4l2.h"
#include "ExynosCameraActivityBase.h"

#include "fimc-is-metadata.h"

#define CAPTURE_SKIP_COUNT (1)

#define FLASH_WAITING_SLEEP_TIME            (15000)     /* 15 msec */
#define FLASH_MAX_WAITING_TIME              (2000000)   /* 2 sec */
#define FLASH_MAX_AEDONE_WAITING_TIME       (3000000)   /* 3 sec */
#define FLASH_MAX_PRE_DONE_WAITING_TIME     (5000000)   /* 5 sec */
#define FLASH_OFF_MAX_WATING_TIME           (500000)    /* 500 ms */
#define FLASH_CAPTURE_WAITING_TIME          (200000)    /* 200 ms */
#define FLASH_TIMEOUT_COUNT                 (30)        /* 30 fps * 1 sec */
#define FLASH_AF_TIMEOUT_COUNT              (60)        /*33ms * 60 */
#define FLASH_AE_TIMEOUT_COUNT              (60)        /*33ms * 60 */
#define FLASH_MAIN_TIMEOUT_COUNT            (15)        /* 33ms * 15 */
#define FLASH_SHOT_MAX_WAITING_TIME         (100000)    /* 100 msec */
#define FLASH_MAIN_WAIT_COUNT               (2)

namespace android {

class ExynosCameraActivityFlash : public ExynosCameraActivityBase {
public:
    enum FLASH_REQ {
        FLASH_REQ_OFF,
        FLASH_REQ_AUTO,
        FLASH_REQ_ON,
        FLASH_REQ_RED_EYE,
        FLASH_REQ_SINGLE,
        FLASH_REQ_TORCH,
        FLASH_REQ_END
    };

    enum FLASH_STATUS {
        FLASH_STATUS_OFF,
        FLASH_STATUS_NEED_FLASH,
        FLASH_STATUS_PRE_CHECK,
        FLASH_STATUS_PRE_READY,
        FLASH_STATUS_PRE_ON,
        FLASH_STATUS_PRE_AE_DONE, /* 5 */
        FLASH_STATUS_PRE_AF,
        FLASH_STATUS_PRE_AF_DONE,
        FLASH_STATUS_PRE_DONE,
        FLASH_STATUS_MAIN_READY,
        FLASH_STATUS_MAIN_ON,
        FLASH_STATUS_MAIN_WAIT,
        FLASH_STATUS_MAIN_DONE,
        FLASH_STATUS_END
    };

    enum FLASH_STEP {
        FLASH_STEP_OFF,
        FLASH_STEP_PRE_AF_START,
        FLASH_STEP_PRE_AF_DONE,
        FLASH_STEP_PRE_START,
        FLASH_STEP_PRE_DONE,
        FLASH_STEP_MAIN_START,
        FLASH_STEP_MAIN_DONE,
        FLASH_STEP_CANCEL,
        FLASH_STEP_PRE_LCD_ON,
        FLASH_STEP_LCD_ON,
        FLASH_STEP_LCD_OFF,
        FLASH_STEP_END
    };

    enum FLASH_TRIGGER {
        FLASH_TRIGGER_OFF,
        FLASH_TRIGGER_TOUCH_DISPLAY,
        FLASH_TRIGGER_SHORT_BUTTON,
        FLASH_TRIGGER_LONG_BUTTON,
        FLASH_TRIGGER_END
    };

public:
    ExynosCameraActivityFlash();
    virtual ~ExynosCameraActivityFlash();

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
    bool setFlashReq(enum FLASH_REQ flashReqVal);
    bool setFlashReq(enum FLASH_REQ flashReqVal, bool overrideFlashControl);
    enum ExynosCameraActivityFlash::FLASH_REQ getFlashReq(void);

    bool setFlashStep(enum FLASH_STEP flashStepVal);

    bool setFlashStatus(enum FLASH_STATUS flashStatusVal);

    status_t setNeedFlashOffDelay(bool delay);
    bool setFlashTrigerPath(enum FLASH_TRIGGER flashTriggerVal);
    bool setFlashWhiteBalance(enum aa_awbmode wbModeVal);
    bool setFlashExposure(enum aa_aemode aeModeVal);
    bool setShouldCheckedRcount(int rcount);
    int  checkMainCaptureRcount(int rcount);

    bool setShouldCheckedFcount(int rcount);
    int  checkMainCaptureFcount(int rcount);

    int  getWaitingCount(void);
    bool getNeedFlash(void);

    bool waitAeDone(void);
    bool waitMainReady(void);
    void setCaptureStatus(bool isCapture);
    unsigned int getShotFcount();
    void resetShotFcount(void);
    bool getFlashTrigerPath(enum FLASH_TRIGGER *flashTriggerVal);
    bool getFlashStep(enum FLASH_STEP *flashStepVal);
    int  getFlashStatus(void);
    bool getNeedCaptureFlash(void);
    bool getNeedFlashOffDelay(void);
    bool setRecordingHint(bool hint);
    bool checkPreFlash(void);
    bool checkFlashOff(void);
    bool updateAeState(void);
    void setAeLock(bool aeLock);
    void setAwbLock(bool awbLock);
    void setFlashWaitCancel(bool cancel);
    bool getFlashWaitCancel(void);
    void setFpsValue(int fpsValue);
    int getFpsValue();
    void notifyAfResult(void);
    void notifyAfResultHAL3(void);
    void notifyAeResult(void);
    void setMainFlashFiring(bool isMainFlashFiring);
    void setManualExposureTime(uint64_t exposureTime);

private:
    bool m_isNeedFlash;
    bool m_isNeedCaptureFlash;
    /* TODO: If need another delay, add variables */
    bool m_isNeedFlashOffDelay;
    int  m_flashTriggerStep;

    int  m_flashStepErrorCount;

    int  m_mainCaptureRcount;
    bool m_checkMainCaptureRcount;
    int  m_mainCaptureFcount;
    int  m_currentIspInputFcount;
    bool m_checkMainCaptureFcount;
    int  m_waitingCount;

    bool m_isMainFlashFiring;
    bool m_isCapture;
    bool m_isRecording;
    bool m_isFlashOff;
    bool m_needAfTrigger;

    int  m_timeoutCount;
    int  m_aeWaitMaxCount;
    int  m_mainWaitCount;

    bool m_checkFlashStepCancel;
    bool m_checkFlashWaitCancel;
    bool m_isPreFlash;
    bool m_aeLock;
    bool m_awbLock;
    unsigned int m_fpsValue;
    uint64_t m_manualExposureTime;

    enum flash_mode         m_flashMode;
    enum aa_ae_flashmode    m_aeflashMode;
    enum aa_aemode          m_aeMode;
    enum aa_awbmode         m_awbMode;
    enum ae_state           m_aeState;

    enum FLASH_STATUS       m_flashStatus;
    enum FLASH_STEP         m_flashStep;
    enum FLASH_TRIGGER      m_flashTrigger;
    enum FLASH_REQ          m_flashReq;
    bool                    m_overrideFlashControl;

    ExynosCameraBuffer      m_reqBuf;
    unsigned int            m_ShotFcount;
    enum FLASH_STATUS       m_flashPreStatus;
    enum ae_state           m_aePreState;
    enum ae_state           m_curAeState;
};
}

#endif /* EXYNOS_CAMERA_ACTIVITY_FLASH_H__ */
