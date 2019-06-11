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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraPipeSTK_PREVIEW"
#include <cutils/log.h>

#include "ExynosCameraPipeSTK_PREVIEW.h"

namespace android {

ExynosCameraPipeSTK_PREVIEW::~ExynosCameraPipeSTK_PREVIEW()
{
    this->destroy();
}

status_t ExynosCameraPipeSTK_PREVIEW::create(__unused int32_t *sensorIds)
{
    if (bSTKInit == false) {
        CLOGE("ERR(%s):STK_PREVIEW_init() fail", __FUNCTION__);
        return INVALID_OPERATION;
    }

    m_mainThread = ExynosCameraThreadFactory::createThread(this, &ExynosCameraPipeSTK_PREVIEW::m_mainThreadFunc, "STK_PREVIEWThread");

    m_inputFrameQ = new frame_queue_t(m_mainThread);

    CLOGI("INFO(%s[%d]):create() is succeed (%d)", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipeSTK_PREVIEW::destroy(void)
{

    int ret = 0;

    if (bSTKInit == false) {
        return NO_ERROR;
    }

    if (end_stk !=NULL) {
        //ret = (*end_stk)();
        ret = (*end_stk)(m_stk_handle);

        if (ret < 0) {
            CLOGE("ERR(%s):STK_PREVIEW End fail", __FUNCTION__);
        } else {
            CLOGD("DEBUG(%s[%d]) STK_PREVIEW End Success!", __FUNCTION__, __LINE__);
        }
        end_stk = NULL;
        init_stk = NULL;
        run_stk = NULL;
    }
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (stkHandle !=NULL) {
        CLOGD("DEBUG(%s[%d]) STK_PREVIEW Handle : %08x", __FUNCTION__, __LINE__, stkHandle);
        dlclose(stkHandle);
        stkHandle = NULL;
    }

    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (m_inputFrameQ != NULL) {
        m_inputFrameQ->release();
        delete m_inputFrameQ;
        m_inputFrameQ = NULL;
    }
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    CLOGI("INFO(%s[%d]):destroy() is succeed (%d)", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipeSTK_PREVIEW::start(void)
{
    ExynosRect previewRect;

    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (bSTKInit == false) {
        m_parameters->getHwPreviewSize(&previewRect.w, &previewRect.h);

        CLOGI("DEBUG(%s[%d]) PreviewSize (%d x %d), scenario(%d)",
                __FUNCTION__, __LINE__, previewRect.w, previewRect.h, STK_SCENARIO_PREVIEW);
        m_stk_handle = (*init_stk)(previewRect.w, previewRect.h, STK_SCENARIO_PREVIEW);
        bSTKInit = true;
    }

    return NO_ERROR;
}

status_t ExynosCameraPipeSTK_PREVIEW::stop(void)
{
    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;

    m_mainThread->requestExitAndWait();

    CLOGD("DEBUG(%s[%d]): thead exited", __FUNCTION__, __LINE__);

    m_inputFrameQ->release();

    if (bSTKInit == false) {
        return NO_ERROR;
    }

    if (end_stk != NULL) {
        //ret = (*end_stk)();
        ret = (*end_stk)(m_stk_handle);

        if (ret < 0)
            CLOGE("ERR(%s):STK_PREVIEW End fail", __FUNCTION__);
        else
            CLOGD("DEBUG(%s[%d]) STK_PREVIEW End Success!", __FUNCTION__, __LINE__);

        bSTKInit = false;
    }
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraPipeSTK_PREVIEW::startThread(void)
{
    CLOGV("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (m_outputFrameQ == NULL) {
        CLOGE("ERR(%s):outputFrameQ is NULL, cannot start", __FUNCTION__);
        return INVALID_OPERATION;
    }

    m_mainThread->run();

    CLOGI("INFO(%s[%d]):startThread is succeed (%d)", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipeSTK_PREVIEW::m_run(void)
{
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer nv21_STK_in_Buffer;
    ExynosRect previewRect;
    ExynosRect srcRect, dstRect;
    int hwSensorWidth = 0;
    int hwSensorHeight = 0;
    long long durationTime = 0;
    int ret = 0;

    m_parameters->getHwPreviewSize(&previewRect.w, &previewRect.h);

    CLOGV("[ExynosCameraPipeSTK_PREVIEW thread] waitFrameQ");
    ret = m_inputFrameQ->waitAndPopProcessQ(&newFrame);
    if (ret < 0) {
        if (ret == TIMED_OUT) {
            CLOGW("WARN(%s):wait timeout", __FUNCTION__);
        } else {
            CLOGE("ERR(%s):wait and pop fail, ret(%d)", __FUNCTION__, ret);
        }
        return ret;
    }

    if (newFrame == NULL) {
        CLOGE("ERR(%s):new frame is NULL", __FUNCTION__);
        return NO_ERROR;
    }

    ret = newFrame->getSrcBuffer(getPipeId(), &nv21_STK_in_Buffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):frame get src buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return OK;
    }

#if 0
    camera2_shot_ext shot_ext;
    // I could not check still that 'shot_ext.shot.dm.request.frameCount' is updated.
    newFrame->getUserDynamicMeta(&shot_ext);

    char buff[128];
    snprintf(buff, sizeof(buff), "/data/stk/CameraHAL_jpeginput_%d.nv1",
             shot_ext.shot.dm.request.frameCount);
    ret = dumpToFile2plane(buff,
        nv21_STK_in_Buffer.addr[0],
        nv21_STK_in_Buffer.addr[1],
        nv21_STK_in_Buffer.size[0],
        nv21_STK_in_Buffer.size[1]);
    if (ret != true) {
        //mflag_dumped = false;
        ALOGE("couldn't make a raw file");
    }
    else {
          //mflag_dumped = false;
          ALOGI("Raw Bayer dump Success!");
    }
#endif

    int pixelformat = STK_NV21;
    int stkPreviewQ;
    int seriesShotMode;
    int availableBufferCountLimit = 4;

    stkPreviewQ = m_inputFrameQ->getSizeOfProcessQ();
    seriesShotMode = m_parameters->getSeriesShotMode();

    if (run_stk != NULL) {
        if (stkPreviewQ <= availableBufferCountLimit) {
            CLOGI("INFO(%s[%d]):Start STK_Preview frameCount(%d), stkPreviewQ(%d), SeriesShotMode(%d)",
                __FUNCTION__, __LINE__, newFrame->getFrameCount(), stkPreviewQ, seriesShotMode);

            m_timer.start();

            m_thread_id = (*run_stk)(m_stk_handle, nv21_STK_in_Buffer.addr[0], nv21_STK_in_Buffer.addr[1], pixelformat);

            ret = pthread_join(*m_thread_id, NULL);

            m_timer.stop();
            durationTime = m_timer.durationMsecs();
            CLOGI("STK Preview Execution Time : (%5d msec)", (int)durationTime);

            if (ret < 0) {
                CLOGE("ERR(%s[%d]):STK run fail, ret(%d)", __FUNCTION__, __LINE__, ret);
                return ret;
            }
        } else {
            CLOGW("WARN(%s[%d]):Skip STK_Preview frameCount(%d), stkPreviewQ(%d), SeriesShotMode(%d)",
                __FUNCTION__, __LINE__, newFrame->getFrameCount(), stkPreviewQ, seriesShotMode);
        }
    }

    CLOGV("DEBUG(%s[%d]): STK Processing done", __FUNCTION__, __LINE__);

    ret = newFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):set entity state fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return ret;
    }

    ret = newFrame->setDstBufferState(getPipeId(), ENTITY_BUFFER_STATE_COMPLETE);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):setdst Buffer failed(%d) frame(%d)", __FUNCTION__, __LINE__, ret, newFrame->getFrameCount());
        return ret;
    }

    m_outputFrameQ->pushProcessQ(&newFrame);

    return NO_ERROR;
}

bool ExynosCameraPipeSTK_PREVIEW::m_mainThreadFunc(void)
{
    int ret = 0;
    bool loopFlag = false;

    CLOGV("[ExynosCameraPipeSTK] Enter m_mainThreadFunc");
    ret = m_run();
    if (ret < 0) {
        if (ret != TIMED_OUT)
            CLOGE("ERR(%s):m_putBuffer fail", __FUNCTION__);
    }

    if (m_inputFrameQ->getSizeOfProcessQ() > 0)
        loopFlag = true;

    return loopFlag;
}

status_t ExynosCameraPipeSTK_PREVIEW::m_init(int32_t *nodeNums)
{
    if (nodeNums == NULL)
        m_stkNum = -1;
    else
        m_stkNum = nodeNums[0];

    m_stk = NULL;

    /*
     * Load the Stain-Killer libarry
     * Initialize the Stain-Killer library
     */
    bSTKInit = false;
    hSTK_object = NULL;
    stkHandle = NULL;
    init_stk = NULL;
    run_stk = NULL;
    end_stk = NULL;

    char stk_lib_path[] = STK_PREVIEW_LIBRARY_PATH;

    int ret = NO_ERROR;
    ExynosRect previewRect;

    stkHandle = dlopen(stk_lib_path, RTLD_NOW);

    if (stkHandle == NULL) {
        ALOGE("ERR(%s[%d]): STK so handle is NULL : %s", __FUNCTION__, __LINE__, stk_lib_path);
        return INVALID_OPERATION;
    }

    //init_stk = (int(*)(STK_params *))dlsym(stkHandle, "stain_killer_init");
    init_stk = (void*(*)(int, int, enum stain_killer_scenario))dlsym(stkHandle, "stain_killer_init");

    if ((dlerror()!= NULL) && (init_stk == NULL)) {
        ALOGE("ERR(%s[%d]): exn_stk_init dlsym error", __FUNCTION__, __LINE__);
        goto CLEAN;
    }

    run_stk = (pthread_t*(*)(void *, char *, char*, int))dlsym(stkHandle, "stain_killer_run");

    if ((dlerror()!= NULL) && (run_stk == NULL)) {
        ALOGE("ERR(%s[%d]): exn_stk_run dlsym error", __FUNCTION__, __LINE__);
        goto CLEAN;
    }

    end_stk = (int(*)(void *))dlsym(stkHandle, "stain_killer_deinit");

    if ((dlerror()!= NULL) && (end_stk == NULL)) {
        ALOGE("ERR(%s[%d]): exn_stk_end dlsym error", __FUNCTION__, __LINE__);
        goto CLEAN;
    }

    /*
     * Call the Stain-Killer library initialization function.
     *
     */

    m_parameters->getHwPreviewSize(&previewRect.w, &previewRect.h);
    CLOGV("[ExynosCameraPipeSTK_PREVIEW] PreviewSize (%d x %d)", previewRect.w, previewRect.h);

    m_stk_handle = (*init_stk)(previewRect.w, previewRect.h, STK_SCENARIO_PREVIEW);
    CLOGV(" init_stk ret : %d", ret);

    return ret;

CLEAN:
    if (stkHandle != NULL) {
        dlclose(stkHandle);
    }
    return INVALID_OPERATION;
}

}; /* namespace android */
