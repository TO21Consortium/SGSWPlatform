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

#ifndef EXYNOS_CAMERA3_SERVICE_INTERFACE_H
#define EXYNOS_CAMERA3_SERVICE_INTERFACE_H


#define USE_ONE_INTERFACE_FILE

#include <utils/RefBase.h>

#include "hardware/camera3.h"
#include "system/camera_metadata.h"

#ifdef USE_ONE_INTERFACE_FILE
#include "ExynosCameraFrameFactory.h"
#include "ExynosCamera.h"
#endif
#include "ExynosCamera3.h"
#include "ExynosCameraInterfaceState.h"
#include "pthread.h"

#ifdef USE_ONE_INTERFACE_FILE
#define SET_METHOD(m) m : HAL_camera_device_##m
#endif
#define SET_METHOD3(m) m : HAL3_camera_device_##m

#define MAX_NUM_OF_CAMERA 2

/* init camera module */
#define INIT_MODULE_PATH "/sys/class/camera/rear/fw_update"

namespace android {

static CameraInfo sCameraInfo[] = {
#if !defined(BOARD_FRONT_CAMERA_ONLY_USE)
    {
        CAMERA_FACING_BACK,
        BACK_ROTATION,  /* orientation */
    },
#endif
    {
        CAMERA_FACING_FRONT,
        FRONT_ROTATION,  /* orientation */
    }
};

/* This struct used in device3.3 service arbitration */
struct CameraConfigInfo {
    int resource_cost;
    char** conflicting_devices;
    size_t conflicting_devices_length;
};

const CameraConfigInfo sCameraConfigInfo[] = {
#if !defined(BOARD_FRONT_CAMERA_ONLY_USE)
    {
        51,      /* resoruce_cost               : [0 , 100] */
        NULL,    /* conflicting_devices         : NULL, (char *[]){"1"}, (char *[]){"0", "1"} */
        0,       /* conflicting_devices_lenght  : The length of the array in the conflicting_devices field */
    },
#endif
    {
        51,      /* resoruce_cost               : [0, 100] */
        NULL,    /* conflicting_devices         : NULL, (char *[]){"0"}, (char *[]){"0", "1"} */
        0,       /* conflicting_devices_lenght  : The length of the array in the conflicting_devices field */
    }
};

#ifdef USE_ONE_INTERFACE_FILE
static camera_device_t *g_cam_device[MAX_NUM_OF_CAMERA];
#endif
static camera3_device_t *g_cam_device3[MAX_NUM_OF_CAMERA];
static camera_metadata_t *g_cam_info[MAX_NUM_OF_CAMERA] = {NULL, NULL};
static const camera_module_callbacks_t *g_callbacks = NULL;

//ExynosCameraRequestManager *m_exynosCameraRequestManager[MAX_NUM_OF_CAMERA];
//const   camera3_callback_ops_t *callbackOps;

static Mutex            g_cam_openLock[MAX_NUM_OF_CAMERA];
static Mutex            g_cam_configLock[MAX_NUM_OF_CAMERA];
#ifdef USE_ONE_INTERFACE_FILE
static Mutex            g_cam_previewLock[MAX_NUM_OF_CAMERA];
static Mutex            g_cam_recordingLock[MAX_NUM_OF_CAMERA];
#endif
static bool             g_cam_torchEnabled[MAX_NUM_OF_CAMERA] = {false, false};
pthread_t		g_thread;

static inline ExynosCamera3 *obj(const struct camera3_device *dev)
{
    return reinterpret_cast<ExynosCamera3 *>(dev->priv);
};

/**
 * Open camera device
 */
static int HAL3_camera_device_open(const struct hw_module_t* module,
                                    const char *id,
                                    struct hw_device_t** device);

/**
 * Close camera device
 */
static int HAL3_camera_device_close(struct hw_device_t* device);

/**
 * initialize
 * One-time initialization to pass framework callback function pointers to the HAL.
 */
static int HAL3_camera_device_initialize(const struct camera3_device *dev,
                                        const camera3_callback_ops_t *callback_ops);

/**
 * configure_streams
 * Reset the HAL camera device processing pipeline and set up new input and output streams.
 */
static int HAL3_camera_device_configure_streams(const struct camera3_device *dev,
                                                camera3_stream_configuration_t *stream_list);

/**
 * register_stream_buffers
 * Register buffers for a given stream with the HAL device.
 */
static int HAL3_camera_device_register_stream_buffers(const struct camera3_device *dev,
                                                    const camera3_stream_buffer_set_t *buffer_set);

/**
 * construct_default_request_settings
 * Create capture settings for standard camera use cases.
 */
static const camera_metadata_t* HAL3_camera_device_construct_default_request_settings(
                                                                const struct camera3_device *dev,
                                                                int type);

/**
 * process_capture_request
 * Send a new capture request to the HAL.
 */
static int HAL3_camera_device_process_capture_request(const struct camera3_device *dev,
                                                        camera3_capture_request_t *request);

/**
 * flush
 * Flush all currently in-process captures and all buffers in the pipeline on the given device.
 */
static int HAL3_camera_device_flush(const struct camera3_device *dev);

/**
 * get_metadata_vendor_tag_ops
 * Get methods to query for vendor extension metadata tag information.
 */
static void HAL3_camera_device_get_metadata_vendor_tag_ops(const struct camera3_device *dev,
                                                            vendor_tag_query_ops_t* ops);

/**
 * dump
 * Print out debugging state for the camera device.
 */
static void HAL3_camera_device_dump(const struct camera3_device *dev, int fd);

/**
 * Retrun the camera hardware info
 */
static int HAL_getCameraInfo(int cameraId, struct camera_info *info);

/**
 * Provide callback function pointers to the HAL module to inform framework
 * of asynchronous camera module events.
 */
static int HAL_set_callbacks(const camera_module_callbacks_t *callbacks);

/**
 * Return number of the camera hardware
 */
static int HAL_getNumberOfCameras();

#ifdef USE_ONE_INTERFACE_FILE
static inline ExynosCamera *obj(struct camera_device *dev)
{
    return reinterpret_cast<ExynosCamera *>(dev->priv);
};

/**
 * Open camera device
 */
static int HAL_camera_device_open(
        const struct hw_module_t* module,
        const char *id,
        struct hw_device_t** device);

/**
 * Close camera device
 */
static int HAL_camera_device_close(struct hw_device_t* device);

/**
 * Set the preview_stream_ops to which preview frames are sent
 */
static int HAL_camera_device_set_preview_window(
        struct camera_device *dev,
        struct preview_stream_ops *buf);

/**
 * Set the notification and data callbacks
 */
static void HAL_camera_device_set_callbacks(
        struct camera_device *dev,
        camera_notify_callback notify_cb,
        camera_data_callback data_cb,
        camera_data_timestamp_callback data_cb_timestamp,
        camera_request_memory get_memory,
        void* user);

/**
 * The following three functions all take a msg_type, which is a bitmask of
 * the messages defined in include/ui/Camera.h
 */

/**
 * Enable a message, or set of messages.
 */
static void HAL_camera_device_enable_msg_type(
        struct camera_device *dev,
        int32_t msg_type);

/**
 * Disable a message, or a set of messages.
 *
 * Once received a call to disableMsgType(CAMERA_MSG_VIDEO_FRAME), camera
 * HAL should not rely on its client to call releaseRecordingFrame() to
 * release video recording frames sent out by the cameral HAL before and
 * after the disableMsgType(CAMERA_MSG_VIDEO_FRAME) call. Camera HAL
 * clients must not modify/access any video recording frame after calling
 * disableMsgType(CAMERA_MSG_VIDEO_FRAME).
 */
static void HAL_camera_device_disable_msg_type(
        struct camera_device *dev,
        int32_t msg_type);

/**
 * Query whether a message, or a set of messages, is enabled.  Note that
 * this is operates as an AND, if any of the messages queried are off, this
 * will return false.
 */
static int HAL_camera_device_msg_type_enabled(
        struct camera_device *dev,
        int32_t msg_type);

/**
 * Start preview mode.
 */
static int HAL_camera_device_start_preview(struct camera_device *dev);

/**
 * Stop a previously started preview.
 */
static void HAL_camera_device_stop_preview(struct camera_device *dev);

/**
 * Returns true if preview is enabled.
 */
static int HAL_camera_device_preview_enabled(struct camera_device *dev);

/**
 * Request the camera HAL to store meta data or real YUV data in the video
 * buffers sent out via CAMERA_MSG_VIDEO_FRAME for a recording session. If
 * it is not called, the default camera HAL behavior is to store real YUV
 * data in the video buffers.
 *
 * This method should be called before startRecording() in order to be
 * effective.
 *
 * If meta data is stored in the video buffers, it is up to the receiver of
 * the video buffers to interpret the contents and to find the actual frame
 * data with the help of the meta data in the buffer. How this is done is
 * outside of the scope of this method.
 *
 * Some camera HALs may not support storing meta data in the video buffers,
 * but all camera HALs should support storing real YUV data in the video
 * buffers. If the camera HAL does not support storing the meta data in the
 * video buffers when it is requested to do do, INVALID_OPERATION must be
 * returned. It is very useful for the camera HAL to pass meta data rather
 * than the actual frame data directly to the video encoder, since the
 * amount of the uncompressed frame data can be very large if video size is
 * large.
 *
 * @param enable if true to instruct the camera HAL to store
 *      meta data in the video buffers; false to instruct
 *      the camera HAL to store real YUV data in the video
 *      buffers.
 *
 * @return OK on success.
 */
static int HAL_camera_device_store_meta_data_in_buffers(
        struct camera_device *dev,
        int enable);

/**
 * Start record mode. When a record image is available, a
 * CAMERA_MSG_VIDEO_FRAME message is sent with the corresponding
 * frame. Every record frame must be released by a camera HAL client via
 * releaseRecordingFrame() before the client calls
 * disableMsgType(CAMERA_MSG_VIDEO_FRAME). After the client calls
 * disableMsgType(CAMERA_MSG_VIDEO_FRAME), it is the camera HAL's
 * responsibility to manage the life-cycle of the video recording frames,
 * and the client must not modify/access any video recording frames.
 */
static int HAL_camera_device_start_recording(struct camera_device *dev);

/**
 * Stop a previously started recording.
 */
static void HAL_camera_device_stop_recording(struct camera_device *dev);

/**
 * Returns true if recording is enabled.
 */
static int HAL_camera_device_recording_enabled(struct camera_device *dev);

/**
 * Release a record frame previously returned by CAMERA_MSG_VIDEO_FRAME.
 *
 * It is camera HAL client's responsibility to release video recording
 * frames sent out by the camera HAL before the camera HAL receives a call
 * to disableMsgType(CAMERA_MSG_VIDEO_FRAME). After it receives the call to
 * disableMsgType(CAMERA_MSG_VIDEO_FRAME), it is the camera HAL's
 * responsibility to manage the life-cycle of the video recording frames.
 */
static void HAL_camera_device_release_recording_frame(
        struct camera_device *dev,
        const void *opaque);

/**
 * Start auto focus, the notification callback routine is called with
 * CAMERA_MSG_FOCUS once when focusing is complete. autoFocus() will be
 * called again if another auto focus is needed.
 */
static int HAL_camera_device_auto_focus(struct camera_device *dev);

/**
 * Cancels auto-focus function. If the auto-focus is still in progress,
 * this function will cancel it. Whether the auto-focus is in progress or
 * not, this function will return the focus position to the default.  If
 * the camera does not support auto-focus, this is a no-op.
 */
static int HAL_camera_device_cancel_auto_focus(struct camera_device *dev);

/**
 * Take a picture.
 */
static int HAL_camera_device_take_picture(struct camera_device *dev);

/**
 * Cancel a picture that was started with takePicture. Calling this method
 * when no picture is being taken is a no-op.
 */
static int HAL_camera_device_cancel_picture(struct camera_device *dev);

/**
 * Set the camera parameters. This returns BAD_VALUE if any parameter is
 * invalid or not supported.
 */
static int HAL_camera_device_set_parameters(
        struct camera_device *dev,
        const char *parms);

/**
 * Return the camera parameters.
 */
char *HAL_camera_device_get_parameters(struct camera_device *dev);

/**
 * Release buffer that used by the camera parameters.
 */
static void HAL_camera_device_put_parameters(
        struct camera_device *dev,
        char *parms);

/**
 * Send command to camera driver.
 */
static int HAL_camera_device_send_command(
        struct camera_device *dev,
        int32_t cmd,
        int32_t arg1,
        int32_t arg2);

/**
 * Release the hardware resources owned by this object.  Note that this is
 * *not* done in the destructor.
 */
static void HAL_camera_device_release(struct camera_device *dev);

/**
 * Dump state of the camera hardware
 */
static int HAL_camera_device_dump(struct camera_device *dev, int fd);
#endif

static void HAL_get_vendor_tag_ops(vendor_tag_ops_t* ops);

static int HAL_open_legacy(const struct hw_module_t* module, const char* id, uint32_t halVersion, struct hw_device_t** device);

static int HAL_set_torch_mode(const char* camera_id, bool enabled);

static int HAL_init(void);

#ifdef USE_ONE_INTERFACE_FILE
static camera_device_ops_t camera_device_ops = {
        SET_METHOD(set_preview_window),
        SET_METHOD(set_callbacks),
        SET_METHOD(enable_msg_type),
        SET_METHOD(disable_msg_type),
        SET_METHOD(msg_type_enabled),
        SET_METHOD(start_preview),
        SET_METHOD(stop_preview),
        SET_METHOD(preview_enabled),
        SET_METHOD(store_meta_data_in_buffers),
        SET_METHOD(start_recording),
        SET_METHOD(stop_recording),
        SET_METHOD(recording_enabled),
        SET_METHOD(release_recording_frame),
        SET_METHOD(auto_focus),
        SET_METHOD(cancel_auto_focus),
        SET_METHOD(take_picture),
        SET_METHOD(cancel_picture),
        SET_METHOD(set_parameters),
        SET_METHOD(get_parameters),
        SET_METHOD(put_parameters),
        SET_METHOD(send_command),
        SET_METHOD(release),
        SET_METHOD(dump),
};
#endif

static camera3_device_ops_t camera_device3_ops = {
        SET_METHOD3(initialize),
        SET_METHOD3(configure_streams),
        NULL,
        SET_METHOD3(construct_default_request_settings),
        SET_METHOD3(process_capture_request),
        SET_METHOD3(get_metadata_vendor_tag_ops),
        SET_METHOD3(dump),
        SET_METHOD3(flush),
        {0} /* reserved for future use */
};

static hw_module_methods_t mCameraHwModuleMethods = {
            open : HAL3_camera_device_open
};

/*
 * Required HAL header.
 */
extern "C" {
    camera_module_t HAL_MODULE_INFO_SYM = {
      common : {
          tag                : HARDWARE_MODULE_TAG,
          module_api_version : CAMERA_MODULE_API_VERSION_2_4,
          hal_api_version    : HARDWARE_HAL_API_VERSION,
          id                 : CAMERA_HARDWARE_MODULE_ID,
          name               : "Exynos Camera HAL3",
          author             : "Samsung Electronics Inc",
          methods            : &mCameraHwModuleMethods,
          dso                : NULL,
          reserved           : {0},
      },
      get_number_of_cameras : HAL_getNumberOfCameras,
      get_camera_info       : HAL_getCameraInfo,
      set_callbacks         : HAL_set_callbacks,
      get_vendor_tag_ops    : HAL_get_vendor_tag_ops,
      open_legacy           : HAL_open_legacy,
      set_torch_mode        : HAL_set_torch_mode,
      init                  : HAL_init,
      reserved              : {0},
    };
}

}; // namespace android
#endif
