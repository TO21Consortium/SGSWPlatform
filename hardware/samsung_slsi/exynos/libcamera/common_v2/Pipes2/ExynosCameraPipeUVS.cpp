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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCameraPipeUVS"
#include <cutils/log.h>

#include "ExynosCameraPipeUVS.h"

namespace android {

ExynosCameraPipeUVS::~ExynosCameraPipeUVS()
{
    this->destroy();
}

status_t ExynosCameraPipeUVS::create(int32_t *sensorIds)
{
    if (bUVSInit == false) {
        CLOGE("ERR(%s):UVS_init() fail", __FUNCTION__);
        return INVALID_OPERATION;
    }

    m_mainThread = ExynosCameraThreadFactory::createThread(this, &ExynosCameraPipeUVS::m_mainThreadFunc, "UVSThread");

    m_inputFrameQ = new frame_queue_t(m_mainThread);

    CLOGI("INFO(%s[%d]):create() is succeed (%d)", __FUNCTION__, __LINE__, getPipeId());

    return NO_ERROR;
}

status_t ExynosCameraPipeUVS::destroy(void)
{

    int ret = 0;

    if ( bUVSInit == false) {
        return NO_ERROR;
    }

    if (end_uvs !=NULL) {
        ret = (*end_uvs)();

        if (ret < 0) {
                CLOGE("ERR(%s):UVS End fail", __FUNCTION__);
        }
        else {
            CLOGD("DEBUG(%s[%d]) UVS End Success!", __FUNCTION__, __LINE__);
        }
        end_uvs = NULL;
        init_uvs = NULL;
        run_uvs = NULL;
    }
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    if (uvsHandle !=NULL) {
        CLOGD("DEBUG(%s[%d]) uvsHandle : %08x", __FUNCTION__, __LINE__, uvsHandle);
        dlclose(uvsHandle);
        uvsHandle = NULL;
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

status_t ExynosCameraPipeUVS::start(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

status_t ExynosCameraPipeUVS::stop(void)
{
    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    int ret = 0;

    m_mainThread->requestExitAndWait();

    CLOGD("DEBUG(%s[%d]): thead exited", __FUNCTION__, __LINE__);

    m_inputFrameQ->release();

    return NO_ERROR;
}

status_t ExynosCameraPipeUVS::startThread(void)
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

status_t ExynosCameraPipeUVS::m_run(void)
{
    ExynosCameraFrame *newFrame = NULL;
    ExynosCameraBuffer yuv_UVS_in_Buffer;
    ExynosCameraBuffer yuv_UVS_out_Buffer;
    ExynosRect pictureRect;
    ExynosRect srcRect, dstRect;
    int hwSensorWidth = 0;
    int hwSensorHeight = 0;
    long long durationTime = 0;
    int ret = 0;

    m_parameters->getPictureSize(&pictureRect.w, &pictureRect.h);

    CLOGI("[ExynosCameraPipeUVS thread] waitFrameQ");
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
        CLOGI("[ExynosCameraPipeUVS thread] new frame is NULL");
        return NO_ERROR;
    }

    ret = newFrame->getSrcBuffer(getPipeId(), &yuv_UVS_in_Buffer);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):frame get src buffer fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return OK;
    }

    camera2_shot_ext shot_ext;

    newFrame->getUserDynamicMeta(&shot_ext);

    m_parameters->getHwSensorSize(&hwSensorWidth, &hwSensorHeight);
    m_parameters->getPictureBayerCropSize(&srcRect, &dstRect);

    m_uvsdynamicMeta.src_y = yuv_UVS_in_Buffer.addr[0];
    m_uvsdynamicMeta.width = pictureRect.w;
    m_uvsdynamicMeta.height = pictureRect.h;

    /* binning_x = (cropped_width * 1024) / capture_width
     * binning_y = (cropped_height * 1024) / capture_height
     */
    m_uvsdynamicMeta.binning_x = (dstRect.w * 1024) / pictureRect.w;
    m_uvsdynamicMeta.binning_y = (dstRect.h * 1024) / pictureRect.h;

    m_uvsdynamicMeta.radial_alpha_R = shot_ext.shot.udm.as.vendorSpecific[0];
    m_uvsdynamicMeta.radial_alpha_G = (shot_ext.shot.udm.as.vendorSpecific[1] + shot_ext.shot.udm.as.vendorSpecific[2])/2;
    m_uvsdynamicMeta.radial_alpha_B = shot_ext.shot.udm.as.vendorSpecific[3];

    CLOGD("DEBUG(%s[%d]):============= UVS Dynamic Params===================", __FUNCTION__, __LINE__);
    CLOGD("DEBUG(%s[%d]):= width                 : %d", __FUNCTION__, __LINE__, m_uvsdynamicMeta.width);
    CLOGD("DEBUG(%s[%d]):= height                : %d", __FUNCTION__, __LINE__, m_uvsdynamicMeta.height);
    CLOGD("DEBUG(%s[%d]):= buffersize            : %d", __FUNCTION__, __LINE__, yuv_UVS_in_Buffer.size[0]);
    CLOGD("DEBUG(%s[%d]):= BayerCropSize width   : %d", __FUNCTION__, __LINE__, dstRect.w);
    CLOGD("DEBUG(%s[%d]):= BayerCropSize height  : %d", __FUNCTION__, __LINE__, dstRect.h);
    CLOGD("DEBUG(%s[%d]):= binning_x             : %d", __FUNCTION__, __LINE__, m_uvsdynamicMeta.binning_x);
    CLOGD("DEBUG(%s[%d]):= binning_y             : %d", __FUNCTION__, __LINE__, m_uvsdynamicMeta.binning_y);
    CLOGD("DEBUG(%s[%d]):= radial_alpha_R        : %d", __FUNCTION__, __LINE__, m_uvsdynamicMeta.radial_alpha_R);
    CLOGD("DEBUG(%s[%d]):= radial_alpha_G        : %d", __FUNCTION__, __LINE__, m_uvsdynamicMeta.radial_alpha_G);
    CLOGD("DEBUG(%s[%d]):= radial_alpha_B        : %d", __FUNCTION__, __LINE__, m_uvsdynamicMeta.radial_alpha_B);
    CLOGD("DEBUG(%s[%d]):===================================================", __FUNCTION__, __LINE__);

    CLOGI("DEBUG(%s[%d]): UVS Processing call", __FUNCTION__, __LINE__);

    if (run_uvs !=NULL) {
        m_timer.start();
        ret = (*run_uvs)(&m_uvsdynamicMeta);
        m_timer.stop();
        durationTime = m_timer.durationMsecs();
        CLOGI("UVS Execution Time : (%5d msec)", (int)durationTime);

    if (ret < 0) {
            CLOGE("ERR(%s[%d]):UVS run fail, ret(%d)", __FUNCTION__, __LINE__, ret);
            return OK;
        }
    }

    CLOGI("DEBUG(%s[%d]): UVS Processing done", __FUNCTION__, __LINE__);

    ret = newFrame->setEntityState(getPipeId(), ENTITY_STATE_FRAME_DONE);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):set entity state fail, ret(%d)", __FUNCTION__, __LINE__, ret);
        return OK;
    }

    m_outputFrameQ->pushProcessQ(&newFrame);

    return NO_ERROR;
}

bool ExynosCameraPipeUVS::m_mainThreadFunc(void)
{
    int ret = 0;
    bool loopFlag = false;

    CLOGI("[ExynosCameraPipeUVS] Enter m_mainThreadFunc");
    ret = m_run();
    if (ret < 0) {
        if (ret != TIMED_OUT)
            CLOGE("ERR(%s):m_putBuffer fail", __FUNCTION__);
    }

    if (m_inputFrameQ->getSizeOfProcessQ() > 0)
        loopFlag = true;

    return loopFlag;
}

status_t ExynosCameraPipeUVS::m_init(int32_t *nodeNums)
{
    if (nodeNums == NULL)
        m_uvsNum = -1;
    else
        m_uvsNum = nodeNums[0];

    m_uvs = NULL;

    /*
     * Load the UVSuppression libarry
     * Initialize the UVSuppression library
     */
    bUVSInit = false;
    hUVS_object = NULL;
    uvsHandle = NULL;
    init_uvs = NULL;
    run_uvs = NULL;
    end_uvs = NULL;

    char uvs_lib_path[] = UVS_LIBRARY_PATH;

    int ret = NO_ERROR;

    uvsHandle = dlopen(uvs_lib_path, RTLD_NOW);

    if (uvsHandle == NULL) {
        ALOGE("ERR(%s[%d]): UVS so handle is NULL : %s", __FUNCTION__, __LINE__, uvs_lib_path);
        return INVALID_OPERATION;
    }

    init_uvs = (int(*)(UVS_params *))dlsym(uvsHandle, "exn_uvs_init");

    if ((dlerror()!= NULL) && (init_uvs == NULL)) {
        ALOGE("ERR(%s[%d]):  exn_uvs_init dlsym error", __FUNCTION__, __LINE__);
        goto CLEAN;
    }

    run_uvs = (int(*)(UVS_params *))dlsym(uvsHandle, "exn_uvs_run");

    if ((dlerror()!= NULL) && (run_uvs == NULL)) {
        ALOGE("ERR(%s[%d]): exn_uvs_run dlsym error", __FUNCTION__, __LINE__);
        goto CLEAN;
    }

    end_uvs = (int(*)())dlsym(uvsHandle, "exn_uvs_end");

    if ((dlerror()!= NULL) && (end_uvs == NULL)) {
        ALOGE("ERR(%s[%d]): exn_uvs_end dlsym error", __FUNCTION__, __LINE__);
        goto CLEAN;
    }

    /*
     * Call the UVSuppression library initialization function.
     *
     */

    ret = (*init_uvs)(&m_uvsdynamicMeta);

    CLOGI(" init_uvs ret : %d", ret);

    return ret;

CLEAN:
    if (uvsHandle != NULL) {
        dlclose(uvsHandle);
    }
    return INVALID_OPERATION;
}

}; /* namespace android */
