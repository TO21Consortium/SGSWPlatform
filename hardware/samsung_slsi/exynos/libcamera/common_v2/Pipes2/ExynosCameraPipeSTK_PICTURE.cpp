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
#define LOG_TAG "ExynosCameraPipeSTK_PICTURE"
#include <cutils/log.h>

#include "ExynosCameraPipeSTK_PICTURE.h"

namespace android {

ExynosCameraPipeSTK_PICTURE::~ExynosCameraPipeSTK_PICTURE()
{
    this->destroy();
}

status_t ExynosCameraPipeSTK_PICTURE::create(__unused int32_t *sensorIds)
{
    if (bSTKInit == false) {
        CLOGE("ERR(%s):STK_init() fail", __FUNCTION__);
        return INVALID_OPERATION;
    }

    m_mainThread = ExynosCameraThreadFactory::createThread(this, &ExynosCameraPipeSTK_PICTURE::m_mainThreadFunc, "STK_PICTUREThread");

    m_inputFrameQ = new frame_queue_t(m_mainThread);

    CLOGI("INFO(%s[%d]):create() is succeed (%d)", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipeSTK_PICTURE::destroy(void)
{

    int ret = 0;

    if (bSTKInit == false) {
        return NO_ERROR;
    }

    if (end_stk !=NULL) {
        //ret = (*end_stk)();
        ret = (*end_stk)(m_stk_handle);

        if (ret < 0) {
            CLOGE("ERR(%s):STK_PICTURE End fail", __FUNCTION__);
        } else {
            CLOGD("DEBUG(%s[%d]) STK_PICTURE End Success!", __FUNCTION__, __LINE__);
        }
        end_stk = NULL;
        init_stk = NULL;
        run_stk = NULL;
    }
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (stkHandle !=NULL) {
        CLOGD("DEBUG(%s[%d]) STK_PICTURE Handle : %08x", __FUNCTION__, __LINE__, stkHandle);
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

status_t ExynosCameraPipeSTK_PICTURE::start(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    ExynosRect pictureRect;

    if (bSTKInit == false) {
        m_parameters->getPictureSize(&pictureRect.w, &pictureRect.h);

        CLOGD("DEBUG(%s[%d]) PictureSize (%d x %d), scenario(%d)",
                __FUNCTION__, __LINE__, pictureRect.w, pictureRect.h, STK_SCENARIO_CAPTURE);
        m_stk_handle = (*init_stk)(pictureRect.w, pictureRect.h, STK_SCENARIO_CAPTURE);

        bSTKInit = true;
    }

    return NO_ERROR;
}

status_t ExynosCameraPipeSTK_PICTURE::stop(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;

    m_mainThread->requestExitAndWait();

    CLOGD("DEBUG(%s[%d]): thead exited", __FUNCTION__, __LINE__);

    m_inputFrameQ->release();

    if (bSTKInit == false) {
        CLOGD("DEBUG(%s[%d]): STK_PICTURE already deinit", __FUNCTION__, __LINE__);
        return NO_ERROR;
    }

    if (end_stk != NULL) {
        ret = (*end_stk)(m_stk_handle);

        if (ret < 0)
            CLOGE("ERR(%s):STK_PICTURE End fail", __FUNCTION__);
        else
            CLOGD("DEBUG(%s[%d]) STK_PICTURE End Success!", __FUNCTION__, __LINE__);

        bSTKInit = false;
    }
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraPipeSTK_PICTURE::startThread(void)
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

status_t ExynosCameraPipeSTK_PICTURE::m_run(void)
{
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer stk_in_Buffer;
    ExynosRect pictureRect;
    ExynosRect srcRect, dstRect;
    int hwSensorWidth = 0;
    int hwSensorHeight = 0;
    long long durationTime = 0;
    int ret = 0;

    m_parameters->getPictureSize(&pictureRect.w, &pictureRect.h);

    CLOGV("[ExynosCameraPipeSTK_PICTURE thread] waitFrameQ");
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

    ret = newFrame->getSrcBuffer(getPipeId(), &stk_in_Buffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):frame get src buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return OK;
    }

    newFrame->getUserDynamicMeta(&m_shot_ext);

    m_parameters->getHwSensorSize(&hwSensorWidth, &hwSensorHeight);
    m_parameters->getPictureBayerCropSize(&srcRect, &dstRect);

    m_stkdynamicMeta.src_y = stk_in_Buffer.addr[0];
    m_stkdynamicMeta.width = pictureRect.w;
    m_stkdynamicMeta.height = pictureRect.h;

    /* binning_x = (cropped_width * 1024) / capture_width
     * binning_y = (cropped_height * 1024) / capture_height
     */
    m_stkdynamicMeta.binning_x = (dstRect.w * 1024) / pictureRect.w;
    m_stkdynamicMeta.binning_y = (dstRect.h * 1024) / pictureRect.h;

    m_stkdynamicMeta.radial_alpha_R = m_shot_ext.shot.udm.as.vendorSpecific[0];
    m_stkdynamicMeta.radial_alpha_G = (m_shot_ext.shot.udm.as.vendorSpecific[1] + m_shot_ext.shot.udm.as.vendorSpecific[2])/2;
    m_stkdynamicMeta.radial_alpha_B = m_shot_ext.shot.udm.as.vendorSpecific[3];

    CLOGV("DEBUG(%s[%d]):============= STK Dynamic Params===================", __FUNCTION__, __LINE__);
    CLOGV("DEBUG(%s[%d]):= width                 : %d", __FUNCTION__, __LINE__, m_stkdynamicMeta.width);
    CLOGV("DEBUG(%s[%d]):= height                : %d", __FUNCTION__, __LINE__, m_stkdynamicMeta.height);
    CLOGV("DEBUG(%s[%d]):= buffersize            : %d", __FUNCTION__, __LINE__, stk_in_Buffer.size[0]);
    CLOGV("DEBUG(%s[%d]):= BayerCropSize width   : %d", __FUNCTION__, __LINE__, dstRect.w);
    CLOGV("DEBUG(%s[%d]):= BayerCropSize height  : %d", __FUNCTION__, __LINE__, dstRect.h);
    CLOGV("DEBUG(%s[%d]):= binning_x             : %d", __FUNCTION__, __LINE__, m_stkdynamicMeta.binning_x);
    CLOGV("DEBUG(%s[%d]):= binning_y             : %d", __FUNCTION__, __LINE__, m_stkdynamicMeta.binning_y);
    CLOGV("DEBUG(%s[%d]):= radial_alpha_R        : %d", __FUNCTION__, __LINE__, m_stkdynamicMeta.radial_alpha_R);
    CLOGV("DEBUG(%s[%d]):= radial_alpha_G        : %d", __FUNCTION__, __LINE__, m_stkdynamicMeta.radial_alpha_G);
    CLOGV("DEBUG(%s[%d]):= radial_alpha_B        : %d", __FUNCTION__, __LINE__, m_stkdynamicMeta.radial_alpha_B);
    CLOGV("DEBUG(%s[%d]):===================================================", __FUNCTION__, __LINE__);

    CLOGI("DEBUG(%s[%d]): STK Processing call", __FUNCTION__, __LINE__);

#if 0
    char buff[128];
    snprintf(buff, sizeof(buff), "/data/media/0/CameraHAL_jpeginput_%d.yuv",
             m_shot_ext.shot.dm.request.frameCount);
    ret = dumpToFile(buff,
        stk_in_Buffer.addr[0],
        stk_in_Buffer.size[0]);
    if (ret != true) {
        //mflag_dumped = false;
        ALOGE("couldn't make a raw file");
    }
    else {
          //mflag_dumped = false;
          ALOGI("Raw Bayer dump Success!");
    }
#endif

    int pixelformat = STK_YUYV;
    int nv21Align = 0;

    if(m_parameters->getSeriesShotMode() == SERIES_SHOT_MODE_LLS
            || m_parameters->getShotMode() == SHOT_MODE_RICH_TONE
            || m_parameters->getShotMode() == SHOT_MODE_FRONT_PANORAMA
            || m_parameters->getPictureFormat() == V4L2_PIX_FMT_NV21
            || m_parameters->getShotMode() == SHOT_MODE_OUTFOCUS) {
        pixelformat = STK_NV21;
        nv21Align = pictureRect.w * pictureRect.h;
    } else {
        pixelformat = STK_YUYV;
    }

    if (run_stk !=NULL) {
        m_timer.start();

        if (pixelformat == STK_NV21)
            m_thread_id = (*run_stk)(m_stk_handle, stk_in_Buffer.addr[0], stk_in_Buffer.addr[0] + nv21Align, pixelformat);
        else
            m_thread_id = (*run_stk)(m_stk_handle, stk_in_Buffer.addr[0], NULL, pixelformat);

        ret = pthread_join(*m_thread_id, NULL);

        m_timer.stop();
        durationTime = m_timer.durationMsecs();
        m_totalCaptureCount++;
        m_totalProcessingTime += durationTime;
        CLOGI("STK PICTURE Execution Time : (%5d msec), Average(%5d msec), Count=%d",
            (int)durationTime, (int)(m_totalProcessingTime/m_totalCaptureCount), m_totalCaptureCount);

        if (ret < 0) {
            CLOGE("ERR(%s[%d]):STK run fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return OK;
        }
    }

    CLOGI("DEBUG(%s[%d]): STK Processing done", __FUNCTION__, __LINE__);

    ret = newFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):set entity state fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return OK;
    }

    m_outputFrameQ->pushProcessQ(&newFrame);

    return NO_ERROR;
}

bool ExynosCameraPipeSTK_PICTURE::m_mainThreadFunc(void)
{
    int ret = 0;
    bool loopFlag = false;

    CLOGI("[ExynosCameraPipeSTK_PICTURE] Enter m_mainThreadFunc");
    ret = m_run();
    if (ret < 0) {
        if (ret != TIMED_OUT)
            CLOGE("ERR(%s):m_putBuffer fail", __FUNCTION__);
    }

    if (m_inputFrameQ->getSizeOfProcessQ() > 0)
        loopFlag = true;

    return loopFlag;
}

status_t ExynosCameraPipeSTK_PICTURE::m_init(int32_t *nodeNums)
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

    memset(&m_shot_ext, 0x00, sizeof(struct camera2_shot_ext));

    m_totalCaptureCount = 0;
    m_totalProcessingTime = 0;

    char stk_lib_path[] = STK_LIBRARY_PATH;

    int ret = NO_ERROR;
    ExynosRect pictureRect;

    stkHandle = dlopen(stk_lib_path, RTLD_NOW);

    if (stkHandle == NULL) {
        ALOGE("ERR(%s[%d]): STK so handle is NULL : %s", __FUNCTION__, __LINE__, stk_lib_path);
        return INVALID_OPERATION;
    }

    //init_stk = (int(*)(STK_params *))dlsym(stkHandle, "stain_killer_init");
    init_stk = (void*(*)(int, int, enum stain_killer_scenario))dlsym(stkHandle, "stain_killer_init");

    if ((dlerror()!= NULL) && (init_stk == NULL)) {
        ALOGE("ERR(%s[%d]):  exn_stk_init dlsym error", __FUNCTION__, __LINE__);
        goto CLEAN;
    }

    run_stk = (pthread_t*(*)(void *, char *, char *, int))dlsym(stkHandle, "stain_killer_run");

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

    //ret = (*init_stk)(&m_stkdynamicMeta);

    m_parameters->getPictureSize(&pictureRect.w, &pictureRect.h);

    CLOGI("[ExynosCameraPipeSTK_PICTURE] PictureSize (%d x %d)", pictureRect.w, pictureRect.h);

    m_stk_handle = (*init_stk)(pictureRect.w, pictureRect.h, STK_SCENARIO_CAPTURE);

    CLOGI(" init_stk ret : %d", ret);

    return ret;

CLEAN:
    if (stkHandle != NULL) {
        dlclose(stkHandle);
    }
    return INVALID_OPERATION;
}

}; /* namespace android */
