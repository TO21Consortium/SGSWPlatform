/*
**
** Copyright 2014, Samsung Electronics Co. LTD
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

#ifndef EXYNOS_CAMERA_ACTIVITY_CONTROL_H
#define EXYNOS_CAMERA_ACTIVITY_CONTROL_H

#include <cutils/log.h>
#include <utils/threads.h>
#include <utils/RefBase.h>
#include <utils/String8.h>
#include <cutils/properties.h>

#include "ExynosCameraActivityAutofocus.h"
#include "ExynosCameraActivityFlash.h"
#include "ExynosCameraActivitySpecialCapture.h"
#include "ExynosCameraActivityUCTL.h"
#include "ExynosCameraSensorInfo.h"
#include "ExynosRect.h"

namespace android{

enum auto_focus_type {
    AUTO_FOCUS_SERVICE      = 0,
    AUTO_FOCUS_HAL,
};

class ExynosCameraActivityControl {
public:

public:
    /* Constructor */
    ExynosCameraActivityControl(int cameraId);

    /* Destructor */
    virtual ~ExynosCameraActivityControl();
    /* Destroy the instance */
    bool            destroy(void);
    /* Check if the instance was created */
    bool            flagCreate(void);
    /* Starts camera auto-focus and registers a callback function to run when the camera is focused. */
    bool            autoFocus(int focusMode, int focusType);
    /* Cancel auto-focus operation */
    bool            cancelAutoFocus(void);
    /* Set auto-focus mode */
    void            setAutoFocusMode(int focusMode);
    int             getAutoFocusMode(void);
    /* Set auto-focus area */
    void            setAutoFcousArea(ExynosRect2 rect, int weight);
    /* Sets auto-focus macro position */
    void            setAutoFocusMacroPosition(
                        int oldautoFocusMacroPosition,
                        int autoFocusMacroPosition);
    int             getCAFResult(void);
    /* Check Whether auto-focus running */
    bool            flagFocusing(struct camera2_shot_ext *shot_ext, int focusMode);
    /* Stop auto-focus */
    void            stopAutoFocus(void);
    /* Sets flash mode */
    bool            setFlashMode(int flashMode);
    /* Start pre flash */
    status_t        startPreFlash(int focusMode);
    /* Stop pre flash */
    void            stopPreFlash(void);
    /* Start main flash */
    bool            waitFlashMainReady();

    /* Start main flash */
    int             startMainFlash(void);
    /* Stop main flash */
    void            stopMainFlash(void);
    /* Cancel flash */
    void            cancelFlash(void);
    /* Sets HDR mode */
    void            setHdrMode(bool hdrMode);
    int             getHdrFcount(int index);

    /* Sets FPS Value */
    void            setFpsValue(int fpsValue);
    int             getFpsValue();

    void setHalVersion(int halVersion);

    void            activityBeforeExecFunc(int pipeId, void *args);
    void            activityAfterExecFunc(int pipeId, void *args);
    ExynosCameraActivityFlash *getFlashMgr(void);
    ExynosCameraActivitySpecialCapture *getSpecialCaptureMgr(void);
    ExynosCameraActivityAutofocus *getAutoFocusMgr(void);
    ExynosCameraActivityUCTL *getUCTLMgr(void);

public:
    bool flagAutoFocusRunning;
    bool touchAFMode;
    bool touchAFModeForFlash;

private:
    ExynosCameraActivityAutofocus *m_autofocusMgr;
    ExynosCameraActivityFlash *m_flashMgr;
    ExynosCameraActivitySpecialCapture * m_specialCaptureMgr;
    ExynosCameraActivityUCTL * m_uctlMgr;

     unsigned int m_fpsValue;
    int m_focusMode;

    int  m_halVersion;
};

}

#endif

