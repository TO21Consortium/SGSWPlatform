/*
 * Copyright 2008, The Android Open Source Project
 * Copyright 2013, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 /*!
 * \file      SecCameraInterface.h
 * \brief     source file for Android Camera Ext HAL
 * \author    teahyung kim (tkon.kim@samsung.com)
 * \date      2013/04/30
 *
 */

#ifndef ANDROID_HARDWARE_SECCAMERAINTERFACE_H
#define ANDROID_HARDWARE_SECCAMERAINTERFACE_H

#include <poll.h>
#include <fcntl.h>
#include <time.h>
#include <cutils/properties.h>
#include <camera/Camera.h>
#include <media/hardware/MetadataBufferType.h>
#include <utils/Endian.h>

#include "SecCameraHardware1MetadataConverter.h"
#include "SecCameraInterfaceState.h"

#ifndef CAMERA_MODULE_VERSION
#define CAMERA_MODULE_VERSION   CAMERA_MODULE_API_VERSION_2_4
#endif

#define SET_METHOD(m) m : HAL_camera_device_##m

#define MAX_NUM_OF_CAMERA 2

namespace android {

static CameraInfo sCameraInfo[] = {
#if !defined(BOARD_FRONT_CAMERA_ONLY_USE)
    {
        CAMERA_FACING_BACK,
        BACK_ROTATION  /* orientation */
    },
#endif
    {
        CAMERA_FACING_FRONT,
        FRONT_ROTATION  /* orientation */
    }
};

/* flashlight control */
#define TORCH_REAR_FILE_PATH "/sys/class/camera/flash/rear_torch_flash"
#define TORCH_FRONT_FILE_PATH "/sys/class/camera/flash/front_torch_flash"

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

static camera_metadata_t *g_cam_info[MAX_NUM_OF_CAMERA] = {NULL, NULL};
static const camera_module_callbacks_t *g_callbacks = NULL;

static camera_device_t *g_cam_device[MAX_NUM_OF_CAMERA];
static bool g_cam_torchEnabled[MAX_NUM_OF_CAMERA] = {false, false};

static inline SecCameraHardware *obj(struct camera_device *dev)
{
    return reinterpret_cast<SecCameraHardware *>(dev->priv);
}

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

/**
 * Callback functions for the camera HAL module to use to inform the framework
 * of changes to the camera subsystem. These are called only by HAL modules
 * implementing version CAMERA_MODULE_API_VERSION_2_1 or higher of the HAL
 * module API interface.
 */
static int HAL_set_callbacks(const camera_module_callbacks_t *callbacks);

/**
 * Retrun the camera hardware info
 */
static int HAL_getCameraInfo(int cameraId, struct camera_info *info);

/**
 * Return number of the camera hardware
 */
static int HAL_getNumberOfCameras();

static int HAL_open_legacy(const struct hw_module_t* module, const char* id, uint32_t halVersion, struct hw_device_t** device);

static void HAL_get_vendor_tag_ops(vendor_tag_ops_t* ops);
static int HAL_set_torch_mode(const char* camera_id, bool enabled);
static int HAL_init();

static int HAL_ext_camera_device_open(
    const struct hw_module_t* module,
    const char *id,
    struct hw_device_t **device);

/**
 * Open camera device
 */
int HAL_ext_camera_device_open_wrapper(
        const struct hw_module_t* module,
        const char *id,
        struct hw_device_t** device);

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

static hw_module_methods_t ext_camera_module_methods = {
            open : HAL_camera_device_open
};

extern "C" {
    struct camera_module HAL_MODULE_INFO_SYM = {
      common : {
          tag                : HARDWARE_MODULE_TAG,
          module_api_version : CAMERA_MODULE_VERSION,
          hal_api_version    : HARDWARE_HAL_API_VERSION,
          id                 : CAMERA_HARDWARE_MODULE_ID,
          name               : "Exynos Camera HAL1",
          author             : "Samsung Corporation",
          methods            : &ext_camera_module_methods,
          dso                : NULL,
          reserved           : {0},
      },
      get_number_of_cameras : HAL_getNumberOfCameras,
      get_camera_info       : HAL_getCameraInfo,
      set_callbacks         : HAL_set_callbacks,
#if (TARGET_ANDROID_VER_MAJ >= 4 && TARGET_ANDROID_VER_MIN >= 4)
      get_vendor_tag_ops    : HAL_get_vendor_tag_ops,
      open_legacy           : HAL_open_legacy,
      set_torch_mode        : HAL_set_torch_mode,
      init                  : HAL_init,
      reserved              : {0}
#endif
    };
}

}; /* namespace android */

#endif /* ANDROID_HARDWARE_SECCAMERAINTERFACE_H */
