/*
 * Copyright (C) 2014, Samsung Electronics Co. LTD
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

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosCamera3Interface"
#include <cutils/log.h>

#include "ExynosCamera3Interface.h"
#include "ExynosCameraAutoTimer.h"

namespace android {

static int HAL3_camera_device_open(const struct hw_module_t* module,
                                    const char *id,
                                    struct hw_device_t** device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int cameraId = atoi(id);
    enum CAMERA_STATE state;
    FILE *fp = NULL;
    int ret = 0;

    CameraMetadata metadata;
    camera_metadata_entry flashAvailable;
    bool hasFlash = false;
    char flashFilePath[100] = {'\0',};

    /* Validation check */
    ALOGI("INFO(%s[%d]):camera(%d) in ======", __FUNCTION__, __LINE__, cameraId);
    if (cameraId < 0 || cameraId >= HAL_getNumberOfCameras()) {
        ALOGE("ERR(%s):Invalid camera ID %s", __FUNCTION__, id);
        return -EINVAL;
    }

    /* Check init thread state */
    if (g_thread) {
        ret = pthread_join(g_thread, NULL);
        if (ret != 0) {
            ALOGE("ERR(%s[%d]):pthread_join failed with error code %d",  __FUNCTION__, __LINE__, ret);
        }
        g_thread = 0;
    }

    /* Setting status and check current status */
    state = CAMERA_OPENED;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s[%d]):camera(%d) state(%d) is INVALID", __FUNCTION__, __LINE__, cameraId, state);
        return -EUSERS;
    }

    /* Create camera device */
    if (g_cam_device3[cameraId]) {
        ALOGE("ERR(%s[%d]):returning existing camera ID(%d)", __FUNCTION__, __LINE__, cameraId);
        *device = (hw_device_t *)g_cam_device3[cameraId];
        goto done;
    }

    g_cam_device3[cameraId] = (camera3_device_t *)malloc(sizeof(camera3_device_t));
    if (!g_cam_device3[cameraId])
        return -ENOMEM;

    g_cam_openLock[cameraId].lock();
    g_cam_device3[cameraId]->common.tag     = HARDWARE_DEVICE_TAG;
    g_cam_device3[cameraId]->common.version = CAMERA_DEVICE_API_VERSION_3_3;
    g_cam_device3[cameraId]->common.module  = const_cast<hw_module_t *>(module);
    g_cam_device3[cameraId]->common.close   = HAL3_camera_device_close;
    g_cam_device3[cameraId]->ops            = &camera_device3_ops;

    ALOGV("DEBUG(%s[%d]):open camera(%d)", __FUNCTION__, __LINE__, cameraId);
    g_cam_device3[cameraId]->priv = new ExynosCamera3(cameraId, &g_cam_info[cameraId]);
    *device = (hw_device_t *)g_cam_device3[cameraId];

    ALOGI("INFO(%s[%d]):camera(%d) out from new g_cam_device3[%d]->priv()",
        __FUNCTION__, __LINE__, cameraId, cameraId);

    g_cam_openLock[cameraId].unlock();

done:
    cam_stateLock[cameraId].lock();
    cam_state[cameraId] = state;
    cam_stateLock[cameraId].unlock();

    if (g_cam_info[cameraId]) {
        metadata = g_cam_info[cameraId];
        flashAvailable = metadata.find(ANDROID_FLASH_INFO_AVAILABLE);

        ALOGV("INFO(%s[%d]): cameraId(%d), flashAvailable.count(%d), flashAvailable.data.u8[0](%d)",
            __FUNCTION__, cameraId, flashAvailable.count, flashAvailable.data.u8[0]);

        if (flashAvailable.count == 1 && flashAvailable.data.u8[0] == 1) {
            hasFlash = true;
        } else {
            hasFlash = false;
        }
    }

    /* Turn off torch and update torch status */
    if(hasFlash && g_cam_torchEnabled[cameraId]) {
        if (cameraId == CAMERA_ID_BACK) {
            snprintf(flashFilePath, sizeof(flashFilePath), TORCH_REAR_FILE_PATH);
        } else {
            snprintf(flashFilePath, sizeof(flashFilePath), TORCH_FRONT_FILE_PATH);
        }

        fp = fopen(flashFilePath, "w+");
        if (fp == NULL) {
            ALOGE("ERR(%s[%d]):torch file open fail, ret(%d)", __FUNCTION__, __LINE__, fp);
        } else {
            fwrite("0", sizeof(char), 1, fp);
            fflush(fp);
            fclose(fp);

            g_cam_torchEnabled[cameraId] = false;
        }
    }

    g_callbacks->torch_mode_status_change(g_callbacks, id, TORCH_MODE_STATUS_NOT_AVAILABLE);

    ALOGI("INFO(%s[%d]):camera(%d) out =====", __FUNCTION__, __LINE__, cameraId);

    return 0;
}

static int HAL3_camera_device_close(struct hw_device_t* device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    uint32_t cameraId;
    int ret = OK;
    enum CAMERA_STATE state;
    char camid[10];

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    if (device) {
        camera3_device_t *cam_device = (camera3_device_t *)device;
        cameraId = obj(cam_device)->getCameraId();

        ALOGV("DEBUG(%s[%d]):close camera(%d)", __FUNCTION__, __LINE__, cameraId);

        ret = obj(cam_device)->releaseDevice();
        if (ret) {
            ALOGE("ERR(%s[%d]):initialize error!!", __FUNCTION__, __LINE__);
            ret = BAD_VALUE;
        }

        state = CAMERA_CLOSED;
        if (check_camera_state(state, cameraId) == false) {
            ALOGE("ERR(%s[%d]):camera(%d) state(%d) is INVALID",
                __FUNCTION__, __LINE__, cameraId, state);
            return -1;
        }

        g_cam_openLock[cameraId].lock();
        ALOGV("INFO(%s[%d]):camera(%d) open locked..", __FUNCTION__, __LINE__, cameraId);
        g_cam_device3[cameraId] = NULL;
        g_cam_openLock[cameraId].unlock();
        ALOGV("INFO(%s[%d]):camera(%d) open unlocked..", __FUNCTION__, __LINE__, cameraId);

        delete static_cast<ExynosCamera3 *>(cam_device->priv);
        free(cam_device);

        cam_stateLock[cameraId].lock();
        cam_state[cameraId] = state;
        cam_stateLock[cameraId].unlock();
        ALOGI("INFO(%s[%d]):close camera(%d)", __FUNCTION__, __LINE__, cameraId);
    }

    /* Update torch status */
    g_cam_torchEnabled[cameraId] = false;
    snprintf(camid, sizeof(camid), "%d\n", cameraId);
    g_callbacks->torch_mode_status_change(g_callbacks, camid,
        TORCH_MODE_STATUS_AVAILABLE_OFF);

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static int HAL3_camera_device_initialize(const struct camera3_device *dev,
                                        const camera3_callback_ops_t *callback_ops)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = OK;
    uint32_t cameraId = obj(dev)->getCameraId();
    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    g_cam_configLock[cameraId].lock();

    ALOGE("INFO(%s[%d]): dual cam_state[0](%d)", __FUNCTION__, __LINE__, cam_state[0]);

#ifdef DUAL_CAMERA_SUPPORTED
    if (cameraId != 0 && g_cam_device3[0] != NULL
        && cam_state[0] != CAMERA_NONE && cam_state[0] != CAMERA_CLOSED) {
        ret = obj(dev)->setDualMode(true);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):camera(%d) set dual mode fail, ret(%d)",
                __FUNCTION__, __LINE__, cameraId, ret);
        } else {
            ALOGI("INFO(%s[%d]):camera(%d) set dual mode)",
                __FUNCTION__, __LINE__, cameraId);
        }
    }
#endif

    ret = obj(dev)->initilizeDevice(callback_ops);
    if (ret) {
        ALOGE("ERR(%s[%d]):initialize error!!", __FUNCTION__, __LINE__);
        ret = BAD_VALUE;
    }
    g_cam_configLock[cameraId].unlock();

    ALOGV("DEBUG(%s):set callback ops - %p", __FUNCTION__, callback_ops);
    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static int HAL3_camera_device_configure_streams(const struct camera3_device *dev,
                                                camera3_stream_configuration_t *stream_list)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = OK;
    uint32_t cameraId = obj(dev)->getCameraId();
    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    g_cam_configLock[cameraId].lock();
    ret = obj(dev)->configureStreams(stream_list);
    if (ret) {
        ALOGE("ERR(%s[%d]):configure_streams error!!", __FUNCTION__, __LINE__);
        ret = BAD_VALUE;
    }
    g_cam_configLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static int HAL3_camera_device_register_stream_buffers(const struct camera3_device *dev,
                                                    const camera3_stream_buffer_set_t *buffer_set)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = OK;
    uint32_t cameraId = obj(dev)->getCameraId();
    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    g_cam_configLock[cameraId].lock();
    ret = obj(dev)->registerStreamBuffers(buffer_set);
    if (ret) {
        ALOGE("ERR(%s[%d]):register_stream_buffers error!!", __FUNCTION__, __LINE__);
        ret = BAD_VALUE;
    }
    g_cam_configLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static const camera_metadata_t* HAL3_camera_device_construct_default_request_settings(
                                                                const struct camera3_device *dev,
                                                                int type)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    camera_metadata_t *request = NULL;
    status_t res;
    uint32_t cameraId = obj(dev)->getCameraId();
    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    g_cam_configLock[cameraId].lock();
    res = obj(dev)->construct_default_request_settings(&request, type);
    if (res) {
        ALOGE("ERR(%s[%d]):constructDefaultRequestSettings error!!", __FUNCTION__, __LINE__);
        g_cam_configLock[cameraId].unlock();
        return NULL;
    }
    g_cam_configLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return request;
}

static int HAL3_camera_device_process_capture_request(const struct camera3_device *dev,
                                                        camera3_capture_request_t *request)
{
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */

    int ret = OK;
    uint32_t cameraId = obj(dev)->getCameraId();
    ALOGV("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    g_cam_configLock[cameraId].lock();
    ret = obj(dev)->processCaptureRequest(request);
    if (ret) {
        ALOGE("ERR(%s[%d]):process_capture_request error(%d)!!", __FUNCTION__, __LINE__, ret);
        ret = BAD_VALUE;
    }
    g_cam_configLock[cameraId].unlock();
    ALOGV("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static int HAL3_camera_device_flush(const struct camera3_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = 0;
    uint32_t cameraId = obj(dev)->getCameraId();

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    g_cam_configLock[cameraId].lock();
    ret = obj(dev)->flush();
    if (ret) {
        ALOGE("ERR(%s[%d]):flush error(%d)!!", __FUNCTION__, __LINE__, ret);
        ret = BAD_VALUE;
    }
    g_cam_configLock[cameraId].unlock();

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
    return ret;
}

static void HAL3_camera_device_get_metadata_vendor_tag_ops(const struct camera3_device *dev,
                                                            vendor_tag_query_ops_t* ops)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    if (dev == NULL)
        ALOGE("ERR(%s[%d]):dev is NULL", __FUNCTION__, __LINE__);

    if (ops == NULL)
        ALOGE("ERR(%s[%d]):ops is NULL", __FUNCTION__, __LINE__);

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
}

static void HAL3_camera_device_dump(const struct camera3_device *dev, int fd)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    if (dev == NULL)
        ALOGE("ERR(%s[%d]):dev is NULL", __FUNCTION__, __LINE__);

    if (fd < 0)
        ALOGE("ERR(%s[%d]):fd is Negative Value", __FUNCTION__, __LINE__);

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);
}

/***************************************************************************
 * FUNCTION   : get_camera_info
 *
 * DESCRIPTION: static function to query the numner of cameras
 *
 * PARAMETERS : none
 *
 * RETURN     : the number of cameras pre-defined
 ***************************************************************************/
static int HAL_getNumberOfCameras()
{
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */
    int getNumOfCamera = sizeof(sCameraInfo) / sizeof(sCameraInfo[0]);
    ALOGV("DEBUG(%s[%d]):Number of cameras(%d)", __FUNCTION__, __LINE__, getNumOfCamera);
    return getNumOfCamera;
}

static int HAL_getCameraInfo(int cameraId, struct camera_info *info)
{
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */
    status_t ret = NO_ERROR;

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    if (cameraId < 0 || cameraId >= HAL_getNumberOfCameras()) {
        ALOGE("ERR(%s[%d]):Invalid camera ID %d", __FUNCTION__, __LINE__, cameraId);
        return -ENODEV;
    }

    /* set facing and orientation */
    memcpy(info, &sCameraInfo[cameraId], sizeof(CameraInfo));

    /* set device API version */
    info->device_version = CAMERA_DEVICE_API_VERSION_3_3;

    /* set camera_metadata_t if needed */
    if (info->device_version >= HARDWARE_DEVICE_API_VERSION(2, 0)) {
        if (g_cam_info[cameraId] == NULL) {
            ALOGV("DEBUG(%s[%d]):Return static information (%d)", __FUNCTION__, __LINE__, cameraId);
            ret = ExynosCamera3MetadataConverter::constructStaticInfo(cameraId, &g_cam_info[cameraId]);
            if (ret != 0) {
                ALOGE("ERR(%s[%d]): static information is NULL", __FUNCTION__, __LINE__);
                return -EINVAL;
            }
            info->static_camera_characteristics = g_cam_info[cameraId];
        } else {
            ALOGV("DEBUG(%s[%d]):Reuse Return static information (%d)", __FUNCTION__, __LINE__, cameraId);
            info->static_camera_characteristics = g_cam_info[cameraId];
        }
    }

    /* set service arbitration (resource_cost, conflicting_devices, conflicting_devices_length */
    info->resource_cost = sCameraConfigInfo[cameraId].resource_cost;
    info->conflicting_devices = sCameraConfigInfo[cameraId].conflicting_devices;
    info->conflicting_devices_length = sCameraConfigInfo[cameraId].conflicting_devices_length;
    ALOGV("INFO(%s info->resource_cost = %d ", __FUNCTION__, info->resource_cost);
    if (info->conflicting_devices_length) {
        for (size_t i = 0; i < info->conflicting_devices_length; i++) {
            ALOGV("INFO(%s info->conflicting_devices = %s ", __FUNCTION__, info->conflicting_devices[i]);
        }
    } else {
        ALOGV("INFO(%s info->conflicting_devices_length is zero ", __FUNCTION__);
    }

    return NO_ERROR;
}

static int HAL_set_callbacks(const camera_module_callbacks_t *callbacks)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    if (callbacks == NULL)
        ALOGE("ERR(%s[%d]):dev is NULL", __FUNCTION__, __LINE__);

    g_callbacks = callbacks;

    return OK;
}

static int HAL_open_legacy(const struct hw_module_t* module, const char* id,
                          uint32_t halVersion, struct hw_device_t** device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    int ret = 0;

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    if (module == NULL) {
        ALOGE("ERR(%s[%d]):module is NULL", __FUNCTION__, __LINE__);
        ret = -EINVAL;
    }

    if (id == NULL) {
        ALOGE("ERR(%s[%d]):id is NULL", __FUNCTION__, __LINE__);
        ret = -EINVAL;
    }

    if (device == NULL) {
        ALOGE("ERR(%s[%d]):device is NULL", __FUNCTION__, __LINE__);
        ret = -EINVAL;
    }

    if (halVersion == 0)
        ALOGE("ERR(%s[%d]):halVersion is Zero", __FUNCTION__, __LINE__);

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);

#ifdef USE_ONE_INTERFACE_FILE
    if (!ret)
        return HAL_camera_device_open(module, id, device);
    else
        return ret;
#else
    return NO_ERROR;
#endif
}

static int HAL_set_torch_mode(const char* camera_id, bool enabled)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int cameraId = atoi(camera_id);
    FILE *fp = NULL;
    CameraMetadata metadata;
    camera_metadata_entry flashAvailable;
    int ret = 0;
    char flashFilePath[100] = {'\0',};

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);
    if (cameraId < 0 || cameraId >= HAL_getNumberOfCameras()) {
        ALOGE("ERR(%s[%d]):Invalid camera ID %d", __FUNCTION__, __LINE__, cameraId);
        return -EINVAL;
    }

    /* Check the android.flash.info.available */
    /* If this camera device does not support flash, It have to return -ENOSYS */
    metadata = g_cam_info[cameraId];
    flashAvailable = metadata.find(ANDROID_FLASH_INFO_AVAILABLE);

    if (flashAvailable.count == 1 && flashAvailable.data.u8[0] == 1) {
        ALOGV("DEBUG(%s[%d]): Flash metadata exist", __FUNCTION__, __LINE__);
    } else {
        ALOGE("ERR(%s[%d]): Can not find flash metadata", __FUNCTION__, __LINE__);
        return -ENOSYS;
    }

    ALOGI("INFO(%s[%d]): Current Camera State (state = %d)", __FUNCTION__, __LINE__, cam_state[cameraId]);

    /* Add the check the camera state that camera in use or not */
    if (cam_state[cameraId] > CAMERA_CLOSED) {
        ALOGE("ERR(%s[%d]): Camera Device is busy (state = %d)", __FUNCTION__, __LINE__, cam_state[cameraId]);
        g_callbacks->torch_mode_status_change(g_callbacks, camera_id, TORCH_MODE_STATUS_AVAILABLE_OFF);
        return -EBUSY;
    }

    /* Add the sysfs file read (sys/class/camera/flash/torch_flash) then set 0 or 1 */
    if (cameraId == CAMERA_ID_BACK) {
        snprintf(flashFilePath, sizeof(flashFilePath), TORCH_REAR_FILE_PATH);
    } else {
        snprintf(flashFilePath, sizeof(flashFilePath), TORCH_FRONT_FILE_PATH);
    }

    fp = fopen(flashFilePath, "w+");
    if (fp == NULL) {
        ALOGE("ERR(%s[%d]):torch file open(%s) fail, ret(%d)",
            __FUNCTION__, __LINE__, flashFilePath, fp);
        return -ENOSYS;
    }

    if (enabled) {
        fwrite("1", sizeof(char), 1, fp);
    } else {
        fwrite("0", sizeof(char), 1, fp);
    }

    fflush(fp);

    ret = fclose(fp);
    if (ret != 0) {
        ALOGE("ERR(%s[%d]): file close failed(%d)", __FUNCTION__, __LINE__, ret);
    }

    if (enabled) {
        g_cam_torchEnabled[cameraId] = true;
        g_callbacks->torch_mode_status_change(g_callbacks,
            camera_id, TORCH_MODE_STATUS_AVAILABLE_ON);
    } else {
        g_cam_torchEnabled[cameraId] = false;
        g_callbacks->torch_mode_status_change(g_callbacks,
            camera_id, TORCH_MODE_STATUS_AVAILABLE_OFF);
    }

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

void *init_func(__unused void *data)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    FILE *fp = NULL;
    char name[64];

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    fp = fopen(INIT_MODULE_PATH, "r");
    if (fp == NULL) {
        ALOGI("INFO(%s[%d]):module init file open fail, ret(%d)", __FUNCTION__, __LINE__, fp);
        return NULL;
    }

    if (fgets(name, sizeof(name), fp) == NULL) {
        ALOGI("INFO(%s[%d]):failed to read init sysfs", __FUNCTION__, __LINE__);
    }

    fclose(fp);

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);

    return NULL;
}

static int HAL_init()
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int ret = 0;

    ALOGI("INFO(%s[%d]):in =====", __FUNCTION__, __LINE__);

    ret = pthread_create(&g_thread, NULL, init_func, NULL);
    if (ret) {
        ALOGE("ERR(%s[%d]):pthread_create failed with error code %d", __FUNCTION__, __LINE__, ret);
    }

    ALOGI("INFO(%s[%d]):out =====", __FUNCTION__, __LINE__);

    return OK;
}

#ifdef USE_ONE_INTERFACE_FILE
static int HAL_camera_device_open(
        const struct hw_module_t* module,
        const char *id,
        struct hw_device_t** device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    int cameraId = atoi(id);
    FILE *fp = NULL;

    CameraMetadata metadata;
    camera_metadata_entry flashAvailable;
    bool hasFlash;
    char flashFilePath[100] = {'\0',};

#ifdef BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA
    if (cameraId == 0) {
        return HAL_ext_camera_device_open_wrapper(module, id, device);
    }
#endif

#ifdef BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA
    if (cameraId == 1) {
        return HAL_ext_camera_device_open_wrapper(module, id, device);
    }
#endif

#if (defined BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA) && (defined BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA)
#else
    enum CAMERA_STATE state;
    int ret = 0;

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);
    if (cameraId < 0 || cameraId >= HAL_getNumberOfCameras()) {
        ALOGE("ERR(%s):Invalid camera ID %s", __FUNCTION__, id);
        return -EINVAL;
    }

    /* Check init thread state */
    if (g_thread) {
        ret = pthread_join(g_thread, NULL);
        if (ret != 0) {
            ALOGE("ERR(%s[%d]):pthread_join failed with error code %d",  __FUNCTION__, __LINE__, ret);
        }
        g_thread = 0;
    }

    state = CAMERA_OPENED;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s):camera(%d) state(%d) is INVALID", __FUNCTION__, cameraId, state);
        return -1;
    }

    if ((unsigned int)cameraId < (sizeof(sCameraInfo) / sizeof(sCameraInfo[0]))) {
        if (g_cam_device[cameraId]) {
            ALOGE("DEBUG(%s):returning existing camera ID %s", __FUNCTION__, id);
            *device = (hw_device_t *)g_cam_device[cameraId];
            goto done;
        }

        g_cam_device[cameraId] = (camera_device_t *)malloc(sizeof(camera_device_t));
        if (!g_cam_device[cameraId])
            return -ENOMEM;

        g_cam_openLock[cameraId].lock();
        g_cam_device[cameraId]->common.tag     = HARDWARE_DEVICE_TAG;
        g_cam_device[cameraId]->common.version = 1;
        g_cam_device[cameraId]->common.module  = const_cast<hw_module_t *>(module);
        g_cam_device[cameraId]->common.close   = HAL_camera_device_close;

        g_cam_device[cameraId]->ops = &camera_device_ops;

        ALOGD("DEBUG(%s):open camera %s", __FUNCTION__, id);
        g_cam_device[cameraId]->priv = new ExynosCamera(cameraId, g_cam_device[cameraId]);
        *device = (hw_device_t *)g_cam_device[cameraId];
        ALOGI("INFO(%s[%d]):camera(%d) out from new g_cam_device[%d]->priv()",
            __FUNCTION__, __LINE__, cameraId, cameraId);

        g_cam_openLock[cameraId].unlock();
        ALOGI("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);
    } else {
        ALOGE("DEBUG(%s):camera(%s) open fail - must front camera open first",
            __FUNCTION__, id);
        return -EINVAL;
    }

done:
    cam_stateLock[cameraId].lock();
    cam_state[cameraId] = state;
    cam_stateLock[cameraId].unlock();

    if (g_cam_info[cameraId]) {
        metadata = g_cam_info[cameraId];
        flashAvailable = metadata.find(ANDROID_FLASH_INFO_AVAILABLE);

        ALOGV("INFO(%s[%d]): cameraId(%d), flashAvailable.count(%d), flashAvailable.data.u8[0](%d)",
            __FUNCTION__, cameraId, flashAvailable.count, flashAvailable.data.u8[0]);

        if (flashAvailable.count == 1 && flashAvailable.data.u8[0] == 1) {
            hasFlash = true;
        } else {
            hasFlash = false;
        }
    }

    if(hasFlash && g_cam_torchEnabled[cameraId]) {
        if (cameraId == CAMERA_ID_BACK) {
            snprintf(flashFilePath, sizeof(flashFilePath), TORCH_REAR_FILE_PATH);
        } else {
            snprintf(flashFilePath, sizeof(flashFilePath), TORCH_FRONT_FILE_PATH);
        }

        fp = fopen(flashFilePath, "w+");
        if (fp == NULL) {
            ALOGE("ERR(%s[%d]):torch file open fail, ret(%d)", __FUNCTION__, __LINE__, fp);
        } else {
            fwrite("0", sizeof(char), 1, fp);
            fflush(fp);
            fclose(fp);

            g_cam_torchEnabled[cameraId] = false;
            g_callbacks->torch_mode_status_change(g_callbacks, id, TORCH_MODE_STATUS_AVAILABLE_OFF);
        }
    }

    g_callbacks->torch_mode_status_change(g_callbacks, id, TORCH_MODE_STATUS_NOT_AVAILABLE);

    ALOGI("INFO(%s[%d]):camera(%d) out", __FUNCTION__, __LINE__, cameraId);
#endif /* (defined BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA) && (defined BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA) */

    return 0;
}

static int HAL_camera_device_close(struct hw_device_t* device)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    uint32_t cameraId;
    enum CAMERA_STATE state;

#if (defined BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA) && (defined BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA)
#else
    ALOGI("INFO(%s[%d]): in", __FUNCTION__, __LINE__);

    if (device) {
        camera_device_t *cam_device = (camera_device_t *)device;
        cameraId = obj(cam_device)->getCameraId();

        ALOGI("INFO(%s[%d]):camera(%d)", __FUNCTION__, __LINE__, cameraId);

        state = CAMERA_CLOSED;
        if (check_camera_state(state, cameraId) == false) {
            ALOGE("ERR(%s):camera(%d) state(%d) is INVALID",
                __FUNCTION__, cameraId, state);
            return -1;
        }

        g_cam_openLock[cameraId].lock();
        ALOGI("INFO(%s[%d]):camera(%d) locked..", __FUNCTION__, __LINE__, cameraId);
        g_cam_device[cameraId] = NULL;
        g_cam_openLock[cameraId].unlock();
        ALOGI("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

        delete static_cast<ExynosCamera *>(cam_device->priv);
        free(cam_device);

        cam_stateLock[cameraId].lock();
        cam_state[cameraId] = state;
        cam_stateLock[cameraId].unlock();
        ALOGI("INFO(%s[%d]):camera(%d)", __FUNCTION__, __LINE__, cameraId);
    }

    ALOGI("INFO(%s[%d]): out", __FUNCTION__, __LINE__);
#endif /* (defined BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA) && (defined BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA) */

    return 0;
}

static int HAL_camera_device_set_preview_window(
        struct camera_device *dev,
        struct preview_stream_ops *buf)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    static int ret;
    uint32_t cameraId = obj(dev)->getCameraId();

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);
    ret = obj(dev)->setPreviewWindow(buf);
    ALOGI("INFO(%s[%d]):camera(%d) out", __FUNCTION__, __LINE__, cameraId);
    return ret;
}

static void HAL_camera_device_set_callbacks(struct camera_device *dev,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void* user)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    obj(dev)->setCallbacks(notify_cb, data_cb, data_cb_timestamp,
                           get_memory,
                           user);
}

static void HAL_camera_device_enable_msg_type(
        struct camera_device *dev,
        int32_t msg_type)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    obj(dev)->enableMsgType(msg_type);
}

static void HAL_camera_device_disable_msg_type(
        struct camera_device *dev,
        int32_t msg_type)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    obj(dev)->disableMsgType(msg_type);
}

static int HAL_camera_device_msg_type_enabled(
        struct camera_device *dev,
        int32_t msg_type)
{
    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->msgTypeEnabled(msg_type);
}

static int HAL_camera_device_start_preview(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    static int ret;
    uint32_t cameraId = obj(dev)->getCameraId();
    enum CAMERA_STATE state;

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);

    state = CAMERA_PREVIEW;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s):camera(%d) state(%d) is INVALID",
            __FUNCTION__, cameraId, state);
        return -1;
    }

    g_cam_previewLock[cameraId].lock();

#ifdef DUAL_CAMERA_SUPPORTED
    if (cameraId != 0 && g_cam_device[0] != NULL
        && cam_state[0] != CAMERA_NONE && cam_state[0] != CAMERA_CLOSED) {
        ret = obj(dev)->setDualMode(true);
        if (ret != NO_ERROR) {
            ALOGE("ERR(%s[%d]):camera(%d) set dual mode fail, ret(%d)",
                __FUNCTION__, __LINE__, cameraId, ret);
        } else {
            ALOGI("INFO(%s[%d]):camera(%d) set dual mode)",
                __FUNCTION__, __LINE__, cameraId);
        }
    }
#endif

    ret = obj(dev)->startPreview();
    ALOGV("INFO(%s[%d]):camera(%d) out from startPreview()",
        __FUNCTION__, __LINE__, cameraId);

    g_cam_previewLock[cameraId].unlock();

    ALOGV("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

    if (ret == OK) {
        cam_stateLock[cameraId].lock();
        cam_state[cameraId] = state;
        cam_stateLock[cameraId].unlock();
        ALOGI("INFO(%s[%d]):camera(%d) out (startPreview succeeded)",
            __FUNCTION__, __LINE__, cameraId);
    } else {
        ALOGI("INFO(%s[%d]):camera(%d) out (startPreview FAILED)",
            __FUNCTION__, __LINE__, cameraId);
    }
    return ret;
}

static void HAL_camera_device_stop_preview(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    uint32_t cameraId = obj(dev)->getCameraId();
    enum CAMERA_STATE state;

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);
/* HACK : If camera in recording state, */
/*        CameraService have to call the stop_recording before the stop_preview */
#if 1
    if (cam_state[cameraId] == CAMERA_RECORDING) {
        ALOGE("ERR(%s[%d]):camera(%d) in RECORDING RUNNING state ---- INVALID ----",
            __FUNCTION__, __LINE__, cameraId);
        ALOGE("ERR(%s[%d]):camera(%d) The stop_recording must be called "
            "before the stop_preview  ---- INVALID ----",
            __FUNCTION__, __LINE__,  cameraId);
        HAL_camera_device_stop_recording(dev);
        ALOGE("ERR(%s[%d]):cameraId=%d out from stop_recording  ---- INVALID ----",
            __FUNCTION__, __LINE__,  cameraId);

        for (int i=0; i<30; i++) {
            ALOGE("ERR(%s[%d]):camera(%d) The stop_recording must be called "
                "before the stop_preview  ---- INVALID ----",
                __FUNCTION__, __LINE__,  cameraId);
        }
        ALOGE("ERR(%s[%d]):camera(%d) sleep 500ms for ---- INVALID ---- state",
            __FUNCTION__, __LINE__,  cameraId);
        usleep(500000); /* to notify, sleep 500ms */
    }
#endif
    state = CAMERA_PREVIEWSTOPPED;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s):camera(%d) state(%d) is INVALID", __FUNCTION__, cameraId, state);
        return;
    }

    g_cam_previewLock[cameraId].lock();

    obj(dev)->stopPreview();
    ALOGI("INFO(%s[%d]):camera(%d) out from stopPreview()",
        __FUNCTION__, __LINE__, cameraId);

    g_cam_previewLock[cameraId].unlock();

    ALOGI("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

    cam_stateLock[cameraId].lock();
    cam_state[cameraId] = state;
    cam_stateLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):camera(%d) out", __FUNCTION__, __LINE__, cameraId);
}

static int HAL_camera_device_preview_enabled(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->previewEnabled();
}

static int HAL_camera_device_store_meta_data_in_buffers(
        struct camera_device *dev,
        int enable)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->storeMetaDataInBuffers(enable);
}

static int HAL_camera_device_start_recording(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    static int ret;
    uint32_t cameraId = obj(dev)->getCameraId();
    enum CAMERA_STATE state;

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);

    state = CAMERA_RECORDING;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s):camera(%d) state(%d) is INVALID",
            __FUNCTION__, cameraId, state);
        return -1;
    }

    g_cam_recordingLock[cameraId].lock();

    ret = obj(dev)->startRecording();
    ALOGI("INFO(%s[%d]):camera(%d) out from startRecording()",
        __FUNCTION__, __LINE__, cameraId);

    g_cam_recordingLock[cameraId].unlock();

    ALOGI("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

    if (ret == OK) {
        cam_stateLock[cameraId].lock();
        cam_state[cameraId] = state;
        cam_stateLock[cameraId].unlock();
        ALOGI("INFO(%s[%d]):camera(%d) out (startRecording succeeded)",
            __FUNCTION__, __LINE__, cameraId);
    } else {
        ALOGI("INFO(%s[%d]):camera(%d) out (startRecording FAILED)",
            __FUNCTION__, __LINE__, cameraId);
    }
    return ret;
}

static void HAL_camera_device_stop_recording(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    uint32_t cameraId = obj(dev)->getCameraId();
    enum CAMERA_STATE state;

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);

    state = CAMERA_RECORDINGSTOPPED;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s):camera(%d) state(%d) is INVALID", __FUNCTION__, cameraId, state);
        return;
    }

    g_cam_recordingLock[cameraId].lock();

    obj(dev)->stopRecording();
    ALOGI("INFO(%s[%d]):camera(%d) out from stopRecording()",
        __FUNCTION__, __LINE__, cameraId);

    g_cam_recordingLock[cameraId].unlock();

    ALOGI("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

    cam_stateLock[cameraId].lock();
    cam_state[cameraId] = state;
    cam_stateLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):camera(%d) out", __FUNCTION__, __LINE__, cameraId);
}

static int HAL_camera_device_recording_enabled(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->recordingEnabled();
}

static void HAL_camera_device_release_recording_frame(struct camera_device *dev,
                                const void *opaque)
{
    /* ExynosCameraAutoTimer autoTimer(__FUNCTION__); */

    ALOGV("DEBUG(%s):", __FUNCTION__);
    obj(dev)->releaseRecordingFrame(opaque);
}

static int HAL_camera_device_auto_focus(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->autoFocus();
}

static int HAL_camera_device_cancel_auto_focus(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->cancelAutoFocus();
}

static int HAL_camera_device_take_picture(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->takePicture();
}

static int HAL_camera_device_cancel_picture(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->cancelPicture();
}

static int HAL_camera_device_set_parameters(
        struct camera_device *dev,
        const char *parms)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    String8 str(parms);
    CameraParameters p(str);
    return obj(dev)->setParameters(p);
}

char *HAL_camera_device_get_parameters(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    String8 str;

/* HACK : to avoid compile error */
#if (defined BOARD_BACK_CAMERA_USES_EXTERNAL_CAMERA) && (defined BOARD_FRONT_CAMERA_USES_EXTERNAL_CAMERA)
    ALOGE("ERR(%s[%d]):invalid opertion on external camera", __FUNCTION__, __LINE__);
#else
    CameraParameters parms = obj(dev)->getParameters();
    str = parms.flatten();
#endif
    return strdup(str.string());
}

static void HAL_camera_device_put_parameters(
        struct camera_device *dev,
        char *parms)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    free(parms);
}

static int HAL_camera_device_send_command(
        struct camera_device *dev,
        int32_t cmd,
        int32_t arg1,
        int32_t arg2)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->sendCommand(cmd, arg1, arg2);
}

static void HAL_camera_device_release(struct camera_device *dev)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    uint32_t cameraId = obj(dev)->getCameraId();
    enum CAMERA_STATE state;

    ALOGI("INFO(%s[%d]):camera(%d) in", __FUNCTION__, __LINE__, cameraId);

    state = CAMERA_RELEASED;
    if (check_camera_state(state, cameraId) == false) {
        ALOGE("ERR(%s):camera(%d) state(%d) is INVALID",
            __FUNCTION__, cameraId, state);
        return;
    }

    g_cam_openLock[cameraId].lock();

    obj(dev)->release();
    ALOGV("INFO(%s[%d]):camera(%d) out from release()",
        __FUNCTION__, __LINE__, cameraId);

    g_cam_openLock[cameraId].unlock();

    ALOGV("INFO(%s[%d]):camera(%d) unlocked..", __FUNCTION__, __LINE__, cameraId);

    cam_stateLock[cameraId].lock();
    cam_state[cameraId] = state;
    cam_stateLock[cameraId].unlock();
    ALOGI("INFO(%s[%d]):camera(%d) out", __FUNCTION__, __LINE__, cameraId);
}

static int HAL_camera_device_dump(struct camera_device *dev, int fd)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    ALOGV("DEBUG(%s):", __FUNCTION__);
    return obj(dev)->dump(fd);
}
#endif

static void HAL_get_vendor_tag_ops(__unused vendor_tag_ops_t* ops)
{
    ALOGW("WARN(%s[%d]):empty operation", __FUNCTION__, __LINE__);
}

}; /* namespace android */
