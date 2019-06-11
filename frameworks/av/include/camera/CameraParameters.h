/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ANDROID_HARDWARE_CAMERA_PARAMETERS_H
#define ANDROID_HARDWARE_CAMERA_PARAMETERS_H

#include <utils/KeyedVector.h>
#include <utils/String8.h>

namespace android {

struct Size {
    int width;
    int height;

    Size() {
        width = 0;
        height = 0;
    }

    Size(int w, int h) {
        width = w;
        height = h;
    }
};

class CameraParameters
{
public:
    CameraParameters();
    CameraParameters(const String8 &params) { unflatten(params); }
    ~CameraParameters();

    String8 flatten() const;
    void unflatten(const String8 &params);

    void set(const char *key, const char *value);
    void set(const char *key, int value);
    void setFloat(const char *key, float value);
    const char *get(const char *key) const;
    int getInt(const char *key) const;
    long long int getInt64(const char *key) const;
    float getFloat(const char *key) const;

    void remove(const char *key);

    void setPreviewSize(int width, int height);
    void getPreviewSize(int *width, int *height) const;
    void getSupportedPreviewSizes(Vector<Size> &sizes) const;
    // Added for Factory in GalaxyCamera model [[[
    void setZoomRangeCheckData(int min, int max);
    void getZoomRangeCheckData(int *min, int *max) const;
    void setZoomSlopeCheckData(int min, int max);
    void getZoomSlopeCheckData(int *min, int *max) const;
    void setPuntRangeData(int min, int max, int num);
    void getPuntRangeData(int *min, int *max, int *num) const;
    void setOISRangeData(int x_min, int x_max, int y_min, int y_max, int x_gain, int peak_x, int peak_y);
    void getOISRangeData(int *x_min, int *x_max, int *y_min, int *y_max, int *x_gain, int *peak_x, int *peak_y) const;
    void setVibRangeData(int x_min, int x_max, int y_min, int y_max, int min, int max);
    void getVibRangeData(int *x_min, int *x_max, int *y_min, int *y_max, int *min, int *max) const;
    void setGyroRangeData (int x1, int y1, int x2, int y2);
    void getGyroRangeData (int *x1, int *y1, int *x2, int *y2) const;
    void setAFScanLimit(int min, int max);
    void getAFScanLimit(int *min, int *max) const;
    void setAFScanRange(int min, int max);
    void getAFScanRange(int *min, int *max) const;
    void setAFResolCapture(int min, int max, int af_unit_step);
    void getAFResolCapture(int *min, int *max, int *af_unit_step) const;
    void setRTCManuallyTime(int year, int month, int day, int hour, int minute);
    void getRTCManuallyTime(int *year, int *month, int *day, int *hour, int *minute) const;
    void setRTCManuallyLimit(int min, int max);
    void getRTCManuallyLimit(int *min, int *max) const;
    void setIRISRange(int min, int max);
    void getIRISRange(int *min, int *max) const;
    void setGainLiveviewRange(int min, int max);
    void getGainLiveviewRange(int *min, int *max) const;
    void setSCSpeedtime(int x, int y);
    void getSCSpeedtime(int *x, int *y) const;
    void setGainCaptureRange(int min, int max);
    void getGainCaptureRange(int *min, int *max) const;
    void setFlashRange(int x, int y);
    void getFlashRange(int *x, int *y) const;
    void setWhiteBalanceValue(int indoorRG, int indoorBG, int outdoorRG, int outdoorBG);
    void getWhiteBalanceValue(int *indoorRG, int *indoorBG, int *outdoorRG, int *outdoorBG) const;
    void setAFLEDRange(int startX, int endX, int startY, int endY);
    void getAFLEDRange(int *startX, int *endX, int *startY, int *endY) const;
    void setAFLEDLVLimit(int min, int max);
    void getAFLEDLVLimit(int *min, int *max) const;
    void setAFDiffCheck(int min, int max);
    void getAFDiffCheck(int *min, int *max) const;
    void setTiltScanLimit(int min, int max);
    void getTiltScanLimit(int *min, int *max) const;
    void setTiltAfRange(int min, int max);
    void getTiltAfRange(int *min, int *max) const;
    void setTiltDiffRange(int min, int max);
    void getTiltDiffRange(int * min, int * max) const;
    void setIrCheckRGain(int min, int max);
    void getIrCheckRGain(int *min, int *max) const;
    void setIrCheckBGain(int min, int max);
    void getIrCheckBGain(int *min, int *max) const;
    // Added for Factory in GalaxyCamera model ]]]

    // Set the dimensions in pixels to the given width and height
    // for video frames. The given width and height must be one
    // of the supported dimensions returned from
    // getSupportedVideoSizes(). Must not be called if
    // getSupportedVideoSizes() returns an empty Vector of Size.
    void setVideoSize(int width, int height);
    // Retrieve the current dimensions (width and height)
    // in pixels for video frames, which must be one of the
    // supported dimensions returned from getSupportedVideoSizes().
    // Must not be called if getSupportedVideoSizes() returns an
    // empty Vector of Size.
    void getVideoSize(int *width, int *height) const;
    // Retrieve a Vector of supported dimensions (width and height)
    // in pixels for video frames. If sizes returned from the method
    // is empty, the camera does not support calls to setVideoSize()
    // or getVideoSize(). In adddition, it also indicates that
    // the camera only has a single output, and does not have
    // separate output for video frames and preview frame.
    void getSupportedVideoSizes(Vector<Size> &sizes) const;
    // Retrieve the preferred preview size (width and height) in pixels
    // for video recording. The given width and height must be one of
    // supported preview sizes returned from getSupportedPreviewSizes().
    // Must not be called if getSupportedVideoSizes() returns an empty
    // Vector of Size. If getSupportedVideoSizes() returns an empty
    // Vector of Size, the width and height returned from this method
    // is invalid, and is "-1x-1".
    void getPreferredPreviewSizeForVideo(int *width, int *height) const;

    void setPreviewFrameRate(int fps);
    int getPreviewFrameRate() const;
    void getPreviewFpsRange(int *min_fps, int *max_fps) const;
    void setPreviewFormat(const char *format);
    const char *getPreviewFormat() const;
    void setPictureSize(int width, int height);
    void getPictureSize(int *width, int *height) const;
    void getSupportedPictureSizes(Vector<Size> &sizes) const;
    void setPictureFormat(const char *format);
    const char *getPictureFormat() const;

    void setAutoParameter(int now_av, int now_tv, int now_ev, int now_scenemode, int now_scenesubmode, int now_FDnum, int now_OTstatus, int now_Brightness, int now_iso);
    void getAutoParameter(int* now_av, int* now_tv, int* now_ev, int* now_scenemode, int* now_scenesubmode, int* now_FDnum, int* now_OTstatus, int* now_Brightness, int* now_iso) const;
    void dump() const;
    status_t dump(int fd, const Vector<String16>& args) const;

    /**
     * Returns a Vector containing the supported preview formats
     * as enums given in graphics.h.
     */
    void getSupportedPreviewFormats(Vector<int>& formats) const;

    // Returns true if no keys are present
    bool isEmpty() const;

    // Parameter keys to communicate between camera application and driver.
    // The access (read/write, read only, or write only) is viewed from the
    // perspective of applications, not driver.

    //////////////////////////////////////////////////////////////////////
    //Added For GalaxyCamera model
    // Current camera mode.
    // Example value : "Smart Auto, Program, A, S, M"
    static const char KEY_MODE[];
    static const char KEY_CAPTURE_MODE[];
    //////////////////////////////////////////////////////////////////////

    // Preview frame size in pixels (width x height).
    // Example value: "480x320". Read/Write.
    static const char KEY_PREVIEW_SIZE[];
    // Supported preview frame sizes in pixels.
    // Example value: "800x600,480x320". Read only.
    static const char KEY_SUPPORTED_PREVIEW_SIZES[];
    // The current minimum and maximum preview fps. This controls the rate of
    // preview frames received (CAMERA_MSG_PREVIEW_FRAME). The minimum and
    // maximum fps must be one of the elements from
    // KEY_SUPPORTED_PREVIEW_FPS_RANGE parameter.
    // Example value: "10500,26623"
    static const char KEY_PREVIEW_FPS_RANGE[];
    // The supported preview fps (frame-per-second) ranges. Each range contains
    // a minimum fps and maximum fps. If minimum fps equals to maximum fps, the
    // camera outputs frames in fixed frame rate. If not, the camera outputs
    // frames in auto frame rate. The actual frame rate fluctuates between the
    // minimum and the maximum. The list has at least one element. The list is
    // sorted from small to large (first by maximum fps and then minimum fps).
    // Example value: "(10500,26623),(15000,26623),(30000,30000)"
    static const char KEY_SUPPORTED_PREVIEW_FPS_RANGE[];
    // The image format for preview frames. See CAMERA_MSG_PREVIEW_FRAME in
    // frameworks/av/include/camera/Camera.h. The default is
    // PIXEL_FORMAT_YUV420SP. Example value: "yuv420sp" or PIXEL_FORMAT_XXX
    // constants. Read/write.
    static const char KEY_PREVIEW_FORMAT[];
    // Supported image formats for preview frames.
    // Example value: "yuv420sp,yuv422i-yuyv". Read only.
    static const char KEY_SUPPORTED_PREVIEW_FORMATS[];
    // Number of preview frames per second. This is the target frame rate. The
    // actual frame rate depends on the driver.
    // Example value: "15". Read/write.
    static const char KEY_PREVIEW_FRAME_RATE[];
    // Supported number of preview frames per second.
    // Example value: "24,15,10". Read.
    static const char KEY_SUPPORTED_PREVIEW_FRAME_RATES[];
    // The dimensions for captured pictures in pixels (width x height).
    // Example value: "1024x768". Read/write.
    static const char KEY_PICTURE_SIZE[];
    // Supported dimensions for captured pictures in pixels.
    // Example value: "2048x1536,1024x768". Read only.
    static const char KEY_SUPPORTED_PICTURE_SIZES[];
    // The image format for captured pictures. See CAMERA_MSG_COMPRESSED_IMAGE
    // in frameworks/base/include/camera/Camera.h.
    // Example value: "jpeg" or PIXEL_FORMAT_XXX constants. Read/write.
    static const char KEY_PICTURE_FORMAT[];
    // Supported image formats for captured pictures.
    // Example value: "jpeg,rgb565". Read only.
    static const char KEY_SUPPORTED_PICTURE_FORMATS[];
    // The width (in pixels) of EXIF thumbnail in Jpeg picture.
    // Example value: "512". Read/write.
    static const char KEY_JPEG_THUMBNAIL_WIDTH[];
    // The height (in pixels) of EXIF thumbnail in Jpeg picture.
    // Example value: "384". Read/write.
    static const char KEY_JPEG_THUMBNAIL_HEIGHT[];
    // Supported EXIF thumbnail sizes (width x height). 0x0 means not thumbnail
    // in EXIF.
    // Example value: "512x384,320x240,0x0". Read only.
    static const char KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES[];
    // The quality of the EXIF thumbnail in Jpeg picture. The range is 1 to 100,
    // with 100 being the best.
    // Example value: "90". Read/write.
    static const char KEY_JPEG_THUMBNAIL_QUALITY[];
    // Jpeg quality of captured picture. The range is 1 to 100, with 100 being
    // the best.
    // Example value: "90". Read/write.
    static const char KEY_JPEG_QUALITY[];
    // The rotation angle in degrees relative to the orientation of the camera.
    // This affects the pictures returned from CAMERA_MSG_COMPRESSED_IMAGE. The
    // camera driver may set orientation in the EXIF header without rotating the
    // picture. Or the driver may rotate the picture and the EXIF thumbnail. If
    // the Jpeg picture is rotated, the orientation in the EXIF header will be
    // missing or 1 (row #0 is top and column #0 is left side).
    //
    // Note that the JPEG pictures of front-facing cameras are not mirrored
    // as in preview display.
    //
    // For example, suppose the natural orientation of the device is portrait.
    // The device is rotated 270 degrees clockwise, so the device orientation is
    // 270. Suppose a back-facing camera sensor is mounted in landscape and the
    // top side of the camera sensor is aligned with the right edge of the
    // display in natural orientation. So the camera orientation is 90. The
    // rotation should be set to 0 (270 + 90).
    //
    // Example value: "0" or "90" or "180" or "270". Write only.
    static const char KEY_ROTATION[];
    // GPS latitude coordinate. GPSLatitude and GPSLatitudeRef will be stored in
    // JPEG EXIF header.
    // Example value: "25.032146" or "-33.462809". Write only.
    static const char KEY_GPS_LATITUDE[];
    // GPS longitude coordinate. GPSLongitude and GPSLongitudeRef will be stored
    // in JPEG EXIF header.
    // Example value: "121.564448" or "-70.660286". Write only.
    static const char KEY_GPS_LONGITUDE[];
    // GPS altitude. GPSAltitude and GPSAltitudeRef will be stored in JPEG EXIF
    // header.
    // Example value: "21.0" or "-5". Write only.
    static const char KEY_GPS_ALTITUDE[];
    // GPS timestamp (UTC in seconds since January 1, 1970). This should be
    // stored in JPEG EXIF header.
    // Example value: "1251192757". Write only.
    static const char KEY_GPS_TIMESTAMP[];
    // GPS Processing Method
    // Example value: "GPS" or "NETWORK". Write only.
    static const char KEY_GPS_PROCESSING_METHOD[];
    // Current white balance setting.
    // Example value: "auto" or WHITE_BALANCE_XXX constants. Read/write.
    static const char KEY_WHITE_BALANCE[];
    // Supported white balance settings.
    // Example value: "auto,incandescent,daylight". Read only.
    static const char KEY_SUPPORTED_WHITE_BALANCE[];
    // Current color effect setting.
    // Example value: "none" or EFFECT_XXX constants. Read/write.
    static const char KEY_EFFECT[];
    // Supported color effect settings.
    // Example value: "none,mono,sepia". Read only.
    static const char KEY_SUPPORTED_EFFECTS[];
    // Current antibanding setting.
    // Example value: "auto" or ANTIBANDING_XXX constants. Read/write.
    static const char KEY_ANTIBANDING[];
    // Supported antibanding settings.
    // Example value: "auto,50hz,60hz,off". Read only.
    static const char KEY_SUPPORTED_ANTIBANDING[];
    // Current scene mode.
    // Example value: "auto" or SCENE_MODE_XXX constants. Read/write.
    static const char KEY_SCENE_MODE[];
    // Supported scene mode settings.
    // Example value: "auto,night,fireworks". Read only.
    static const char KEY_SUPPORTED_SCENE_MODES[];
    // Current flash mode.
    // Example value: "auto" or FLASH_MODE_XXX constants. Read/write.
    static const char KEY_FLASH_MODE[];
    // Current flash stand by on /off.
    static const char KEY_FLASH_STANDBY[];
    // Current flash charging on /off.
    static const char KEY_FLASH_CHARGING[];
    // Supported flash modes.
    // Example value: "auto,on,off". Read only.
    static const char KEY_SUPPORTED_FLASH_MODES[];
    // Current focus mode. This will not be empty. Applications should call
    // CameraHardwareInterface.autoFocus to start the focus if focus mode is
    // FOCUS_MODE_AUTO or FOCUS_MODE_MACRO.
    // Example value: "auto" or FOCUS_MODE_XXX constants. Read/write.
    static const char KEY_FOCUS_MODE[];
    // Supported focus modes.
    // Example value: "auto,macro,fixed". Read only.
    static const char KEY_SUPPORTED_FOCUS_MODES[];
    // The maximum number of focus areas supported. This is the maximum length
    // of KEY_FOCUS_AREAS.
    // Example value: "0" or "2". Read only.
    static const char KEY_MAX_NUM_FOCUS_AREAS[];
    // Current focus areas.
    //
    // Before accessing this parameter, apps should check
    // KEY_MAX_NUM_FOCUS_AREAS first to know the maximum number of focus areas
    // first. If the value is 0, focus area is not supported.
    //
    // Each focus area is a five-element int array. The first four elements are
    // the rectangle of the area (left, top, right, bottom). The direction is
    // relative to the sensor orientation, that is, what the sensor sees. The
    // direction is not affected by the rotation or mirroring of
    // CAMERA_CMD_SET_DISPLAY_ORIENTATION. Coordinates range from -1000 to 1000.
    // (-1000,-1000) is the upper left point. (1000, 1000) is the lower right
    // point. The width and height of focus areas cannot be 0 or negative.
    //
    // The fifth element is the weight. Values for weight must range from 1 to
    // 1000.  The weight should be interpreted as a per-pixel weight - all
    // pixels in the area have the specified weight. This means a small area
    // with the same weight as a larger area will have less influence on the
    // focusing than the larger area. Focus areas can partially overlap and the
    // driver will add the weights in the overlap region.
    //
    // A special case of single focus area (0,0,0,0,0) means driver to decide
    // the focus area. For example, the driver may use more signals to decide
    // focus areas and change them dynamically. Apps can set (0,0,0,0,0) if they
    // want the driver to decide focus areas.
    //
    // Focus areas are relative to the current field of view (KEY_ZOOM). No
    // matter what the zoom level is, (-1000,-1000) represents the top of the
    // currently visible camera frame. The focus area cannot be set to be
    // outside the current field of view, even when using zoom.
    //
    // Focus area only has effect if the current focus mode is FOCUS_MODE_AUTO,
    // FOCUS_MODE_MACRO, FOCUS_MODE_CONTINUOUS_VIDEO, or
    // FOCUS_MODE_CONTINUOUS_PICTURE.
    // Example value: "(-10,-10,0,0,300),(0,0,10,10,700)". Read/write.
    static const char KEY_FOCUS_AREAS[];
    // Focal length in millimeter.
    // Example value: "4.31". Read only.
    static const char KEY_FOCAL_LENGTH[];
    // Horizontal angle of view in degrees.
    // Example value: "54.8". Read only.
    static const char KEY_HORIZONTAL_VIEW_ANGLE[];
    // Vertical angle of view in degrees.
    // Example value: "42.5". Read only.
    static const char KEY_VERTICAL_VIEW_ANGLE[];
    // Exposure compensation index. 0 means exposure is not adjusted.
    // Example value: "-5" or "5". Read/write.
    static const char KEY_EXPOSURE_COMPENSATION[];
    // The maximum exposure compensation index (>=0).
    // Example value: "6". Read only.
    static const char KEY_MAX_EXPOSURE_COMPENSATION[];
    // The minimum exposure compensation index (<=0).
    // Example value: "-6". Read only.
    static const char KEY_MIN_EXPOSURE_COMPENSATION[];
    // The exposure compensation step. Exposure compensation index multiply by
    // step eqals to EV. Ex: if exposure compensation index is -6 and step is
    // 0.3333, EV is -2.
    // Example value: "0.333333333" or "0.5". Read only.
    static const char KEY_EXPOSURE_COMPENSATION_STEP[];
    // The state of the auto-exposure lock. "true" means that
    // auto-exposure is locked to its current value and will not
    // change. "false" means the auto-exposure routine is free to
    // change exposure values. If auto-exposure is already locked,
    // setting this to true again has no effect (the driver will not
    // recalculate exposure values). Changing exposure compensation
    // settings will still affect the exposure settings while
    // auto-exposure is locked. Stopping preview or taking a still
    // image will not change the lock. In conjunction with
    // exposure compensation, this allows for capturing multi-exposure
    // brackets with known relative exposure values. Locking
    // auto-exposure after open but before the first call to
    // startPreview may result in severely over- or under-exposed
    // images.  The driver will not change the AE lock after
    // auto-focus completes.
    static const char KEY_AUTO_EXPOSURE_LOCK[];
    // Whether locking the auto-exposure is supported. "true" means it is, and
    // "false" or this key not existing means it is not supported.
    static const char KEY_AUTO_EXPOSURE_LOCK_SUPPORTED[];
    // The state of the auto-white balance lock. "true" means that
    // auto-white balance is locked to its current value and will not
    // change. "false" means the auto-white balance routine is free to
    // change white balance values. If auto-white balance is already
    // locked, setting this to true again has no effect (the driver
    // will not recalculate white balance values). Stopping preview or
    // taking a still image will not change the lock. In conjunction
    // with exposure compensation, this allows for capturing
    // multi-exposure brackets with fixed white balance. Locking
    // auto-white balance after open but before the first call to
    // startPreview may result in severely incorrect color.  The
    // driver will not change the AWB lock after auto-focus
    // completes.
    static const char KEY_AUTO_WHITEBALANCE_LOCK[];
    // Whether locking the auto-white balance is supported. "true"
    // means it is, and "false" or this key not existing means it is
    // not supported.
    static const char KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED[];

    // The maximum number of metering areas supported. This is the maximum
    // length of KEY_METERING_AREAS.
    // Example value: "0" or "2". Read only.
    static const char KEY_MAX_NUM_METERING_AREAS[];
    // Current metering areas. Camera driver uses these areas to decide
    // exposure.
    //
    // Before accessing this parameter, apps should check
    // KEY_MAX_NUM_METERING_AREAS first to know the maximum number of metering
    // areas first. If the value is 0, metering area is not supported.
    //
    // Each metering area is a rectangle with specified weight. The direction is
    // relative to the sensor orientation, that is, what the sensor sees. The
    // direction is not affected by the rotation or mirroring of
    // CAMERA_CMD_SET_DISPLAY_ORIENTATION. Coordinates of the rectangle range
    // from -1000 to 1000. (-1000, -1000) is the upper left point. (1000, 1000)
    // is the lower right point. The width and height of metering areas cannot
    // be 0 or negative.
    //
    // The fifth element is the weight. Values for weight must range from 1 to
    // 1000.  The weight should be interpreted as a per-pixel weight - all
    // pixels in the area have the specified weight. This means a small area
    // with the same weight as a larger area will have less influence on the
    // metering than the larger area. Metering areas can partially overlap and
    // the driver will add the weights in the overlap region.
    //
    // A special case of all-zero single metering area means driver to decide
    // the metering area. For example, the driver may use more signals to decide
    // metering areas and change them dynamically. Apps can set all-zero if they
    // want the driver to decide metering areas.
    //
    // Metering areas are relative to the current field of view (KEY_ZOOM).
    // No matter what the zoom level is, (-1000,-1000) represents the top of the
    // currently visible camera frame. The metering area cannot be set to be
    // outside the current field of view, even when using zoom.
    //
    // No matter what metering areas are, the final exposure are compensated
    // by KEY_EXPOSURE_COMPENSATION.
    // Example value: "(-10,-10,0,0,300),(0,0,10,10,700)". Read/write.
    static const char KEY_METERING_AREAS[];
    // Current zoom value.
    // Example value: "0" or "6". Read/write.
    static const char KEY_ZOOM[];
    static const char KEY_FOCUS_RANGE[];

    // Companion chip stats mode control
    static const char KEY_RT_HDR[];
    static const char KEY_PHASE_AF[];
    static const char KEY_DYNAMIC_RANGE_CONTROL[];

    static const char KEY_ZOOM_ACTION[];
    static const char KEY_ZOOM_LENS_STATUS[];

    // Maximum zoom value.
    // Example value: "6". Read only.
    static const char KEY_MAX_ZOOM[];
    // The zoom ratios of all zoom values. The zoom ratio is in 1/100
    // increments. Ex: a zoom of 3.2x is returned as 320. The number of list
    // elements is KEY_MAX_ZOOM + 1. The first element is always 100. The last
    // element is the zoom ratio of zoom value KEY_MAX_ZOOM.
    // Example value: "100,150,200,250,300,350,400". Read only.
    static const char KEY_ZOOM_RATIOS[];
    // Whether zoom is supported. Zoom is supported if the value is "true". Zoom
    // is not supported if the value is not "true" or the key does not exist.
    // Example value: "true". Read only.
    static const char KEY_ZOOM_SUPPORTED[];
    // Whether if smooth zoom is supported. Smooth zoom is supported if the
    // value is "true". It is not supported if the value is not "true" or the
    // key does not exist.
    // See CAMERA_CMD_START_SMOOTH_ZOOM, CAMERA_CMD_STOP_SMOOTH_ZOOM, and
    // CAMERA_MSG_ZOOM in frameworks/base/include/camera/Camera.h.
    // Example value: "true". Read only.
    static const char KEY_SMOOTH_ZOOM_SUPPORTED[];

    // The distances (in meters) from the camera to where an object appears to
    // be in focus. The object is sharpest at the optimal focus distance. The
    // depth of field is the far focus distance minus near focus distance.
    //
    // Focus distances may change after starting auto focus, canceling auto
    // focus, or starting the preview. Applications can read this anytime to get
    // the latest focus distances. If the focus mode is FOCUS_MODE_CONTINUOUS,
    // focus distances may change from time to time.
    //
    // This is intended to estimate the distance between the camera and the
    // subject. After autofocus, the subject distance may be within near and far
    // focus distance. However, the precision depends on the camera hardware,
    // autofocus algorithm, the focus area, and the scene. The error can be
    // large and it should be only used as a reference.
    //
    // Far focus distance > optimal focus distance > near focus distance. If
    // the far focus distance is infinity, the value should be "Infinity" (case
    // sensitive). The format is three float values separated by commas. The
    // first is near focus distance. The second is optimal focus distance. The
    // third is far focus distance.
    // Example value: "0.95,1.9,Infinity" or "0.049,0.05,0.051". Read only.
    static const char KEY_FOCUS_DISTANCES[];

    // The current dimensions in pixels (width x height) for video frames.
    // The width and height must be one of the supported sizes retrieved
    // via KEY_SUPPORTED_VIDEO_SIZES.
    // Example value: "1280x720". Read/write.
    static const char KEY_VIDEO_SIZE[];
    // A list of the supported dimensions in pixels (width x height)
    // for video frames. See CAMERA_MSG_VIDEO_FRAME for details in
    // frameworks/base/include/camera/Camera.h.
    // Example: "176x144,1280x720". Read only.
    static const char KEY_SUPPORTED_VIDEO_SIZES[];

    // The maximum number of detected faces supported by hardware face
    // detection. If the value is 0, hardware face detection is not supported.
    // Example: "5". Read only
    static const char KEY_MAX_NUM_DETECTED_FACES_HW[];

    // The maximum number of detected faces supported by software face
    // detection. If the value is 0, software face detection is not supported.
    // Example: "5". Read only
    static const char KEY_MAX_NUM_DETECTED_FACES_SW[];

    // Preferred preview frame size in pixels for video recording.
    // The width and height must be one of the supported sizes retrieved
    // via KEY_SUPPORTED_PREVIEW_SIZES. This key can be used only when
    // getSupportedVideoSizes() does not return an empty Vector of Size.
    // Camcorder applications are recommended to set the preview size
    // to a value that is not larger than the preferred preview size.
    // In other words, the product of the width and height of the
    // preview size should not be larger than that of the preferred
    // preview size. In addition, we recommend to choos a preview size
    // that has the same aspect ratio as the resolution of video to be
    // recorded.
    // Example value: "800x600". Read only.
    static const char KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO[];

    // The image format for video frames. See CAMERA_MSG_VIDEO_FRAME in
    // frameworks/base/include/camera/Camera.h.
    // Example value: "yuv420sp" or PIXEL_FORMAT_XXX constants. Read only.
    static const char KEY_VIDEO_FRAME_FORMAT[];

    // Sets the hint of the recording mode. If this is true, MediaRecorder.start
    // may be faster or has less glitches. This should be called before starting
    // the preview for the best result. But it is allowed to change the hint
    // while the preview is active. The default value is false.
    //
    // The apps can still call Camera.takePicture when the hint is true. The
    // apps can call MediaRecorder.start when the hint is false. But the
    // performance may be worse.
    // Example value: "true" or "false". Read/write.
    static const char KEY_RECORDING_HINT[];

    // Returns true if video snapshot is supported. That is, applications
    // can call Camera.takePicture during recording. Applications do not need to
    // call Camera.startPreview after taking a picture. The preview will be
    // still active. Other than that, taking a picture during recording is
    // identical to taking a picture normally. All settings and methods related
    // to takePicture work identically. Ex: KEY_PICTURE_SIZE,
    // KEY_SUPPORTED_PICTURE_SIZES, KEY_JPEG_QUALITY, KEY_ROTATION, and etc.
    // The picture will have an EXIF header. FLASH_MODE_AUTO and FLASH_MODE_ON
    // also still work, but the video will record the flash.
    //
    // Applications can set shutter callback as null to avoid the shutter
    // sound. It is also recommended to set raw picture and post view callbacks
    // to null to avoid the interrupt of preview display.
    //
    // Field-of-view of the recorded video may be different from that of the
    // captured pictures.
    // Example value: "true" or "false". Read only.
    static const char KEY_VIDEO_SNAPSHOT_SUPPORTED[];

    // The state of the video stabilization. If set to true, both the
    // preview stream and the recorded video stream are stabilized by
    // the camera. Only valid to set if KEY_VIDEO_STABILIZATION_SUPPORTED is
    // set to true.
    //
    // The value of this key can be changed any time the camera is
    // open. If preview or recording is active, it is acceptable for
    // there to be a slight video glitch when video stabilization is
    // toggled on and off.
    //
    // This only stabilizes video streams (between-frames stabilization), and
    // has no effect on still image capture.
    static const char KEY_VIDEO_STABILIZATION[];

    // Returns true if video stabilization is supported. That is, applications
    // can set KEY_VIDEO_STABILIZATION to true and have a stabilized preview
    // stream and record stabilized videos.
    static const char KEY_VIDEO_STABILIZATION_SUPPORTED[];

    // Supported modes for special effects with light.
    // Example values: "lowlight,hdr".
    static const char KEY_LIGHTFX[];

    // Value for KEY_ZOOM_SUPPORTED or KEY_SMOOTH_ZOOM_SUPPORTED.
    static const char TRUE[];
    static const char FALSE[];

    // Value for KEY_FOCUS_DISTANCES.
    static const char FOCUS_DISTANCE_INFINITY[];

    // Current ISO setting.
    static const char KEY_ISO[];

    // Current Focus Area Mode setting.
    static const char KEY_FOCUS_AREA_MODE[];

    // Current Face Detection setting.
    static const char KEY_FACEDETECT[];

    // Current Metering setting.
    static const char KEY_METERING[];

    // Current Contrast setting.
    static const char KEY_CONTRAST[];

    // Current Sharpness setting.
    static const char KEY_SHARPNESS[];

    // Current Saturation setting.
    static const char KEY_SATURATION[];

    // Current bracket aeb setting.
    static const char KEY_BRACKET_AEB[];

    // Current bracket wbb setting.
    static const char KEY_BRACKET_WBB[];

    // Current Image Stabilizer(OIS) setting.
    static const char KEY_IMAGE_STABILIZER[];

    // Current video frame rate setting.
    static const char KEY_VIDEO_FRAME_RATE[];

    // Current shutter speed setting.
    static const char KEY_SHUTTER_SPEED[];

    // Current aperture setting.
    static const char KEY_APERTURE[];

    // Current Continuous settings.
    static const char KEY_CONTINUOUS_MODE[];

    // Current OIS settings.
    // Example value: "still", "movie", "continuous", "vss".
    static const char KEY_OIS[];
    // Current ato values.
    // Example values "aperture,shutter-speed,evc,scene,sub-scene"
    static const char KEY_AUTO_VALUE[];

    // Current RawDataSave values.
    // Example value : 0 means off, 1 means on.
    static const char KEY_RAW_SAVE[];

    // Current Burst shot save folder.
    static const char KEY_CAPTURE_BURST_FILEPATH[];

    // Current Address.
    static const char KEY_CURRENT_ADDRESS[];

    // Values for continuous mode settings.
    static const char CONTINUOUS_OFF[];
    static const char CONTINUOUS_ON[];

    static const char KEY_WEATHER[];
    static const char KEY_CITYID[];

    // Values for white balance settings.
    static const char WHITE_BALANCE_AUTO[];
    static const char WHITE_BALANCE_INCANDESCENT[];
    static const char WHITE_BALANCE_FLUORESCENT[];
    static const char WHITE_BALANCE_WARM_FLUORESCENT[];
    static const char WHITE_BALANCE_DAYLIGHT[];
    static const char WHITE_BALANCE_CLOUDY_DAYLIGHT[];
    static const char WHITE_BALANCE_TWILIGHT[];
    static const char WHITE_BALANCE_SHADE[];
    // added for GalaxyCamera model
    static const char WHITE_BALANCE_FLUORESCENT_H[];
    static const char WHITE_BALANCE_FLUORESCENT_L[];
    static const char WHITE_BALANCE_TUNGSTEN[];
    static const char WHITE_BALANCE_CUSTOM[];
    static const char WHITE_BALANCE_K[];
    static const char WHITE_BALANCE_PROHIBITION[];
    static const char WHITE_BALANCE_HORIZON[];
    static const char WHITE_BALANCE_LEDLIGHT[];

    // Values for effect settings.
    static const char EFFECT_NONE[];
    static const char EFFECT_MONO[];
    static const char EFFECT_NEGATIVE[];
    static const char EFFECT_SOLARIZE[];
    static const char EFFECT_SEPIA[];
    static const char EFFECT_POSTERIZE[];
    static const char EFFECT_WHITEBOARD[];
    static const char EFFECT_BLACKBOARD[];
    static const char EFFECT_AQUA[];
    static const char EFFECT_ANTIQUE[];
    static const char EFFECT_POINT_BLUE[];
    static const char EFFECT_POINT_RED[];
    static const char EFFECT_POINT_YELLOW[];
    static const char EFFECT_WARM[];
    static const char EFFECT_COLD[];
    static const char EFFECT_WASHED[];

    // Values for antibanding settings.
    static const char ANTIBANDING_AUTO[];
    static const char ANTIBANDING_50HZ[];
    static const char ANTIBANDING_60HZ[];
    static const char ANTIBANDING_OFF[];

    // Values for flash mode settings.
    // Flash will not be fired.
    static const char FLASH_MODE_OFF[];
    // Flash will be fired automatically when required. The flash may be fired
    // during preview, auto-focus, or snapshot depending on the driver.
    static const char FLASH_MODE_AUTO[];
    // Flash will always be fired during snapshot. The flash may also be
    // fired during preview or auto-focus depending on the driver.
    static const char FLASH_MODE_ON[];
    // Flash will be fired in red-eye reduction mode.
    static const char FLASH_MODE_RED_EYE[];
    // Constant emission of light during preview, auto-focus and snapshot.
    // This can also be used for video recording.
    static const char FLASH_MODE_TORCH[];
    static const char FLASH_MODE_FILLIN[];
    static const char FLASH_MODE_SLOW_SYNC[];
    static const char FLASH_MODE_RED_EYE_FIX[];

    static const char FLASH_VALUE_OF_ISP[];

    // Values for flash stand by settings.
    static const char FLASH_STANDBY_ON[];
    static const char FLASH_STANDBY_OFF[];

    // Values for scene mode settings.
    static const char SCENE_MODE_AUTO[];
    static const char SCENE_MODE_ACTION[];
    static const char SCENE_MODE_PORTRAIT[];
    static const char SCENE_MODE_LANDSCAPE[];
    static const char SCENE_MODE_NIGHT[];
    static const char SCENE_MODE_NIGHT_PORTRAIT[];
    static const char SCENE_MODE_THEATRE[];
    static const char SCENE_MODE_BEACH[];
    static const char SCENE_MODE_SNOW[];
    static const char SCENE_MODE_SUNSET[];
    static const char SCENE_MODE_STEADYPHOTO[];
    static const char SCENE_MODE_FIREWORKS[];
    static const char SCENE_MODE_SPORTS[];
    static const char SCENE_MODE_PARTY[];
    static const char SCENE_MODE_CANDLELIGHT[];
	static const char SCENE_MODE_BEACH_SNOW[];
	static const char SCENE_MODE_DUSK_DAWN[];
	static const char SCENE_MODE_FALL_COLOR[];
	static const char SCENE_MODE_BACK_LIGHT[];
    // Applications are looking for a barcode. Camera driver will be optimized
    // for barcode reading.
    static const char SCENE_MODE_BARCODE[];
    // A high-dynamic range mode. In this mode, the HAL module will use a
    // capture strategy that extends the dynamic range of the captured
    // image in some fashion. Only the final image is returned.
    static const char SCENE_MODE_HDR[];
    // added for GalaxyCamera model
    static const char SCENE_MODE_BACK_LIGHT_PORTRAIT[];
    static const char SCENE_MODE_WHITE[];
    static const char SCENE_MODE_NATURAL_GREEN[];
    static const char SCENE_MODE_BLUESKY[];
    static const char SCENE_MODE_MACRO[];
    static const char SCENE_MODE_MACRO_TEXT[];
    static const char SCENE_MODE_MACRO_COLOR[];
    static const char SCENE_MODE_TRIPOD[];

    // Pixel color formats for KEY_PREVIEW_FORMAT, KEY_PICTURE_FORMAT,
    // and KEY_VIDEO_FRAME_FORMAT
    static const char PIXEL_FORMAT_YUV422SP[];
    static const char PIXEL_FORMAT_YUV420SP[]; // NV21
    static const char PIXEL_FORMAT_YUV420SP_NV21[]; // NV21
    static const char PIXEL_FORMAT_YUV422I[]; // YUY2
    static const char PIXEL_FORMAT_YUV420P[]; // YV12
    static const char PIXEL_FORMAT_RGB565[];
    static const char PIXEL_FORMAT_RGBA8888[];
    static const char PIXEL_FORMAT_JPEG[];
    // Raw bayer format used for images, which is 10 bit precision samples
    // stored in 16 bit words. The filter pattern is RGGB.
    static const char PIXEL_FORMAT_BAYER_RGGB[];
    // Pixel format is not known to the framework
    static const char PIXEL_FORMAT_ANDROID_OPAQUE[];

    // Values for focus mode settings.
    // Auto-focus mode. Applications should call
    // CameraHardwareInterface.autoFocus to start the focus in this mode.
    static const char FOCUS_MODE_AUTO[];
    // Focus is set at infinity. Applications should not call
    // CameraHardwareInterface.autoFocus in this mode.
    static const char FOCUS_MODE_INFINITY[];
    // Macro (close-up) focus mode. Applications should call
    // CameraHardwareInterface.autoFocus to start the focus in this mode.
    static const char FOCUS_MODE_MACRO[];
    // Focus is fixed. The camera is always in this mode if the focus is not
    // adjustable. If the camera has auto-focus, this mode can fix the
    // focus, which is usually at hyperfocal distance. Applications should
    // not call CameraHardwareInterface.autoFocus in this mode.
    static const char FOCUS_MODE_FIXED[];
    // Extended depth of field (EDOF). Focusing is done digitally and
    // continuously. Applications should not call
    // CameraHardwareInterface.autoFocus in this mode.
    static const char FOCUS_MODE_EDOF[];
    // Continuous auto focus mode intended for video recording. The camera
    // continuously tries to focus. This is the best choice for video
    // recording because the focus changes smoothly . Applications still can
    // call CameraHardwareInterface.takePicture in this mode but the subject may
    // not be in focus. Auto focus starts when the parameter is set.
    //
    // Applications can call CameraHardwareInterface.autoFocus in this mode. The
    // focus callback will immediately return with a boolean that indicates
    // whether the focus is sharp or not. The focus position is locked after
    // autoFocus call. If applications want to resume the continuous focus,
    // cancelAutoFocus must be called. Restarting the preview will not resume
    // the continuous autofocus. To stop continuous focus, applications should
    // change the focus mode to other modes.
    static const char FOCUS_MODE_CONTINUOUS_VIDEO[];
    // Continuous auto focus mode intended for taking pictures. The camera
    // continuously tries to focus. The speed of focus change is more aggressive
    // than FOCUS_MODE_CONTINUOUS_VIDEO. Auto focus starts when the parameter is
    // set.
    //
    // Applications can call CameraHardwareInterface.autoFocus in this mode. If
    // the autofocus is in the middle of scanning, the focus callback will
    // return when it completes. If the autofocus is not scanning, focus
    // callback will immediately return with a boolean that indicates whether
    // the focus is sharp or not. The apps can then decide if they want to take
    // a picture immediately or to change the focus mode to auto, and run a full
    // autofocus cycle. The focus position is locked after autoFocus call. If
    // applications want to resume the continuous focus, cancelAutoFocus must be
    // called. Restarting the preview will not resume the continuous autofocus.
    // To stop continuous focus, applications should change the focus mode to
    // other modes.
    static const char FOCUS_MODE_CONTINUOUS_PICTURE[];
    static const char FOCUS_MODE_MANUAL[];

    // Values for light special effects
    // Low-light enhancement mode
    static const char LIGHTFX_LOWLIGHT[];
    // High-dynamic range mode
    static const char LIGHTFX_HDR[];

    // add for GalaxyCamera model
    static const char FOCUS_MODE_CONTINUOUS[];

    // Values for iso settings.
    static const char ISO_AUTO[];
    static const char ISO_50[];
    static const char ISO_80[];
    static const char ISO_100[];
    static const char ISO_200[];
    static const char ISO_400[];
    static const char ISO_800[];
    static const char ISO_1600[];
    static const char ISO_3200[];
    static const char ISO_6400[];
    static const char ISO_SPORTS[];
    static const char ISO_NIGHT[];

    // Values for focus area mode settings.
    static const char FOCUS_AREA_CENTER[];
    static const char FOCUS_AREA_MULTI[];
    static const char FOCUS_AREA_SMART_TOUCH[];

    // Values for focus range mode settings.
    static const char FOCUS_RANGE_AUTO[];
    static const char FOCUS_RANGE_MACRO[];
    static const char FOCUS_RANGE_AUTO_MACRO[];

    // Values for focus mode settings.
    static const char FOCUS_MODE_MULTI[];
    static const char FOCUS_MODE_TOUCH[];
    static const char FOCUS_MODE_OBJECT_TRACKING[];
    static const char FOCUS_MODE_FACE_DETECTION[];
    static const char FOCUS_MODE_SMART_SELF[];

    // Values for face detection settings.
    static const char FACEDETECT_MODE_OFF[];
    static const char FACEDETECT_MODE_NORMAL[];
    static const char FACEDETECT_MODE_SMILESHOT[];
    static const char FACEDETECT_MODE_BLINK[];

    // Values for metering settings.
    static const char METERING_OFF[];
    static const char METERING_CENTER[];
    static const char METERING_MATRIX[];
    static const char METERING_SPOT[];

    // Values for bracket settings.
    static const char BRACKET_MODE_OFF[];
    static const char BRACKET_MODE_AEB[];
    static const char BRACKET_MODE_WBB[];

    // Values for image stabilizer(ois) settings.
    static const char IMAGE_STABILIZER_OFF[];
    static const char IMAGE_STABILIZER_OIS[];
    static const char IMAGE_STABILIZER_DUALIS[];

    // Values for smart scene detection settings.
    static const char SMART_SCENE_DETECT_OFF[];
    static const char SMART_SCENE_DETECT_ON[];

///////////////////////////////////////////////////////////////////////////
    // Added for GalaxyCamera model
    // Values for camera modes
    static const char MODE_SMART_AUTO[];
    static const char MODE_PROGRAM[];
    static const char MODE_A[];
    static const char MODE_S[];
    static const char MODE_M[];
    static const char MODE_MAGIC_SHOT[];
    static const char MODE_BEAUTY_FACE[];
    static const char MODE_BEST_PHOTO[];
    static const char MODE_CONTINUOUS_SHOT[];
    static const char MODE_BEST_FACE[];
    static const char MODE_LANDSCAPE[];
    static const char MODE_DAWN[];
    static const char MODE_SNOW[];
    static const char MODE_MACRO[];
    static const char MODE_FOOD[];
    static const char MODE_PARTY_INDOOR[];
    static const char MODE_ACTION_FREEZE[];
    static const char MODE_RICH_TONE[];
    static const char MODE_PANORAMA[];
    static const char MODE_WATERFALL[];
    static const char MODE_ERASER[];
    static const char MODE_SILHOUETTE[];
    static const char MODE_SUNSET[];
    static const char MODE_FIREWORKS[];
    static const char MODE_LIGHT_TRACE[];
    static const char MODE_SMART_SELFSHOT[];
    static const char MODE_SMART_SUGGEST[];
    static const char MODE_SMART_BUDDY[];

    // Values for camera capture modes
    static const char CAPTURE_MODE_SINGLE[];
    static const char CAPTURE_MODE_HDR[];
    static const char CAPTURE_MODE_NIGHT[];
    static const char CAPTURE_MODE_CONTINUOUS[];
    static const char CAPTURE_MODE_AE_BKT[];
    static const char CAPTURE_MODE_RAW_SAVE[];
    static const char CAPTURE_MODE_LOG_SAVE[];
////////////////////////////////////////////////////////////////////////////

    // Values for Zoom Action in GalaxyCamera model
    static const char ZOOM_ACTION_KEY_START[];
    static const char ZOOM_ACTION_PINCH_START[];
    static const char ZOOM_ACTION_STOP[];
    static const char ZOOM_ACTION_SLOW_TELE_START[];
    static const char ZOOM_ACTION_SLOW_WIDE_START[];

    //Self Shot ROI Box Info
    static const char KEY_CURRENT_ROI_LEFT[];
    static const char KEY_CURRENT_ROI_TOP[];
    static const char KEY_CURRENT_ROI_WIDTH[];
    static const char KEY_CURRENT_ROI_HEIGHT[];

    // Added for Factory test in GalaxyCamera model
    static const char KEY_FACTORY_COMMON[];
    static const char KEY_FACTORY_COMMON_END_RESULT[];
    static const char KEY_FACTORY_TESTNO[];
    static const char KEY_FACTORY_COMMON_DOWN_RESULT[];
    static const char KEY_FACTORY_LDC[];
    static const char KEY_FACTORY_LSC[];
    static const char KEY_FACTORY_LSC_ZOOM_INTERRUPT[];
    static const char KEY_FACTORY_FAIL_STOP[];
    static const char KEY_FACTORY_NODEFOCUSYES[];
    static const char KEY_FACTORY_OIS[];
    static const char KEY_FACTORY_OIS_RANGE_DATA[];
    static const char KEY_FACTORY_OIS_CENTERING_MOVE_SHIFT_DATA[];
    static const char KEY_FACTORY_VIB[];
    static const char KEY_FACTORY_VIB_RANGE_DATA[];
    static const char KEY_FACTORY_GYRO[];
    static const char KEY_FACTORY_GYRO_RANGE_DATA[];
    static const char KEY_FACTORY_BACKLASH_COUNT[];
    static const char KEY_FACTORY_BACKLASH_MAX[];
    static const char KEY_FACTORY_BACKLASH[];
    static const char KEY_FACTORY_INTERPOLATION[];
    static const char KEY_FACTORY_ZOOM[];
    static const char KEY_FACTORY_ZOOM_STEP[];
    static const char KEY_FACTORY_PUNT[];
    static const char KEY_FACTORY_ZOOM_RANGE_CHECK_DATA[];
    static const char KEY_FACTORY_ZOOM_SLOPE_CHECK_DATA[];
    static const char KEY_FACTORY_PUNT_RANGE_DATA[];
    static const char KEY_FACTORY_AF[];
    static const char KEY_FACTORY_AF_STEP_SET[];
    static const char KEY_FACTORY_AF_POSITION[];
    static const char KEY_FACTORY_AF_SCAN_LIMIT[];
    static const char KEY_FACTORY_AF_SCAN_RANGE[];
    static const char KEY_FACTORY_AF_RESOL_CAPTURE[];
    static const char KEY_FACTORY_DEFOCUS[];
    static const char KEY_FACTORY_DEFOCUS_WIDE[];
    static const char KEY_FACTORY_DEFOCUS_TELE[];
    static const char KEY_FACTORY_RESOL_CAP[];
    static const char KEY_FACTORY_MODE[];
    static const char KEY_FACTORY_AFLENS[];
    static const char KEY_FACTORY_RTC_MANUALLY_TIME[];
    static const char KEY_FACTORY_RTC_MANUALLY_LIMIT[];
    static const char KEY_FACTORY_IRIS_RANGE[];
    static const char KEY_FACTORY_GAIN_LIVEVIEW_RANGE[];
    static const char KEY_FACTORY_AF_ZONE[];
    static const char KEY_FACTORY_OIS_MOVE_SHIFT[];
    static const char KEY_FACTORY_OIS_MOVE_SHIFT_X[];
    static const char KEY_FACTORY_OIS_MOVE_SHIFT_Y[];
    static const char KEY_FACTORY_PUNT_SHORT_SCAN_DATA[];
    static const char KEY_FACTORY_PUNT_LONG_SCAN_DATA[];
    static const char KEY_FACTORY_LV_TARGET[];
    static const char KEY_FACTORY_ADJ_IRIS[];
    static const char KEY_FACTORY_ADJ_GAIN_LIVEVIEW[];
    static const char KEY_FACTORY_SH_CLOSE_IRISNUM[];
    static const char KEY_FACTORY_SH_CLOSE_RANGE[];
    static const char KEY_FACTORY_SH_CLOSE_SETIRIS[];
    static const char KEY_FACTORY_SH_CLOSE_ISO[];
    static const char KEY_FACTORY_SH_CLOSE[];
    static const char KEY_FACTORY_SH_CLOSE_SPEEDTIME[];
    static const char KEY_FACTORY_FLICKER[];
    static const char KEY_FACTORY_GAIN_CAPTURE_RANGE[];
    static const char KEY_FACTORY_CAPTURE_GAIN[];
    static const char KEY_FACTORY_CAPTURE_GAIN_OFFSET_VAL[];
    static const char KEY_FACTORY_CAPTURE_GAIN_OFFSET_SIGN[];
    static const char KEY_FACTORY_CAPTURE_CTRL[];
    static const char KEY_FACTORY_AE_TARGET[];
    static const char KEY_FACTORY_LSC_FSHD_TABLE[];
    static const char KEY_FACTORY_LSC_FSHD_REFERENCE[];
    static const char KEY_FACTORY_FLASH_RANGE[];
    static const char KEY_FACTORY_FLASH_CMD[];
    static const char KEY_FACTORY_FLASH_MANUAL_CHARGE[];
    static const char KEY_FACTORY_FLASH_MANUAL_EN[];
    static const char KEY_FACTORY_WB_VALUE[];
    static const char KEY_FACTORY_WB_RANGE[];
    static const char KEY_FACTORY_WB[];
    static const char KEY_FACTORY_AF_LED_RANGE[];
    static const char KEY_FACTORY_AF_LED_LV_LIMIT[];
    static const char KEY_FACTORY_AF_LED_TIME[];
    static const char KEY_FACTORY_AF_DIFF_CHECK[];
    static const char KEY_FACTORY_DEFECTPIXEL_SET_NLV_CAPTURE[];
    static const char KEY_FACTORY_DEFECTPIXEL_SET_NLV_DRAFT0[];
    static const char KEY_FACTORY_DEFECTPIXEL_SET_NLV_DRAFT1[];
    static const char KEY_FACTORY_DEFECTPIXEL_SET_NLV_DRAFT2[];
    static const char KEY_FACTORY_DEFECTPIXEL_SET_NLV_DRAFT_HS[];
    static const char KEY_FACTORY_DEFECTPIXEL_SET_NLV_DRAFT1_HD[];
    static const char KEY_FACTORY_DEFECTPIXEL[];
    static const char KEY_FACTORY_CAM_SYSMODE[];
    static const char KEY_FACTORY_FLASH_CHECK [];
    static const char KEY_FACTORY_FW_VER_ISP[];
    static const char KEY_FACTORY_FW_VER_OIS[];
    static const char KEY_FACTORY_FW_VER_RESULT[];
    static const char KEY_FACTORY_LIVEVIEW_OFFSET_SIGN[];
    static const char KEY_FACTORY_LIVEVIEW_OFFSET_VAL[];
    static const char KEY_FACTORY_TILT_SCAN_LIMIT[];
    static const char KEY_FACTORY_TILT_FIELD[];
    static const char KEY_FACTORY_TILT_AF_RANGE[];
    static const char KEY_FACTORY_TILT_DIFF_RANGE[];
    static const char KEY_FACTORY_TILT_1TIME_SCRIPT_RUN[];
    static const char KEY_FACTORY_IRCHECK_RGAIN[];
    static const char KEY_FACTORY_IRCHECK_BGAIN[];
    static const char KEY_FACTORY_IRCHECK_RUN[];

    static const char FACTORY_FIRMWARE_DOWNLOAD[];
    static const char FACTORY_DOWNLOAD_CHECK[];
    static const char FACTORY_END_CHECK[];
    static const char FACTORY_COMMON_SET_FOCUS_ZONE_MACRO[];

    static const char FACTORY_LDC_OFF[];
    static const char FACTORY_LDC_ON[];

    static const char FACTORY_LSC_OFF[];
    static const char FACTORY_LSC_ON[];
    static const char FACTORY_LSC_LOG[];

    static const char FACTORY_LSC_ZOOM_INTERRUPT_ENABLE[];
    static const char FACTORY_LSC_ZOOM_INTERRUPT_DISABLE[];

    static const char FACTORY_FAIL_STOP_ON[];
    static const char FACTORY_FAIL_STOP_OFF[];
    static const char FACTORY_FAIL_STOP_RUN[];
    static const char FACTORY_FAIL_STOP_STOP[];

    static const char FACTORY_NODEFOCUSYES_ON[];
    static const char FACTORY_NODEFOCUSYES_OFF[];
    static const char FACTORY_NODEFOCUSYES_RUN[];
    static const char FACTORY_NODEFOCUSYES_STOP[];

    static const char FACTORY_OIS_RETURN_TO_CENTER[];
    static const char FACTORY_OIS_MODE_ON[];
    static const char FACTORY_OIS_MODE_OFF[];
    static const char FACTORY_OIS_RUN[];
    static const char FACTORY_OIS_START[];
    static const char FACTORY_OIS_STOP[];
    static const char FACTORY_OIS_LOG[];

    static const char FACTORY_VIB_START[];
    static const char FACTORY_VIB_STOP[];
    static const char FACTORY_VIB_LOG[];

    static const char FACTORY_GYRO_START[];
    static const char FACTORY_GYRO_STOP[];
    static const char FACTORY_GYRO_LOG[];

    static const char FACTORY_BACKLASH_INPUT[];
    static const char FACTORY_BACKLASH_INPUT_MAX_THR[];
    static const char FACTORY_BACKLASH_WIDE_RUN[];
    static const char FACTORY_BACKLASH_LOG[];

    static const char FACTORY_INTERPOLATION_USE[];
    static const char FACTORY_INTERPOLATION_RELEASE[];

    static const char FACTORY_ZOOM_MOVE_STEP[];
    static const char FACTORY_ZOOM_RANGE_CHECK_START[];
    static const char FACTORY_ZOOM_RANGE_CHECK_STOP[];
    static const char FACTORY_ZOOM_SLOPE_CHECK_START[];
    static const char FACTORY_ZOOM_SLOPE_CHECK_STOP[];
    static const char FACTORY_ZOOM_STEP_TELE[];
    static const char FACTORY_ZOOM_STEP_WIDE[];
    static const char FACTORY_ZOOM_CLOSE[];
    static const char FACTORY_ZOOM_MOVE_END_CHECK[];

    static const char FACTORY_PUNT_RANGE_START[];
    static const char FACTORY_PUNT_RANGE_STOP[];
    static const char FACTORY_PUNT_SHORT_SCAN_DATA[];
    static const char FACTORY_PUNT_SHORT_SCAN_START[];
    static const char FACTORY_PUNT_SHORT_SCAN_STOP[];
    static const char FACTORY_PUNT_LONG_SCAN_DATA[];
    static const char FACTORY_PUNT_LONG_SCAN_START[];
    static const char FACTORY_PUNT_LONG_SCAN_STOP[];
    static const char FACTORY_PUNT_LOG[];

    static const char FACTORY_PUNT_EEP_WRITE[];

    static const char FACTORY_AF_LOCK_ON_SET[];
    static const char FACTORY_AF_LOCK_OFF_SET[];
    static const char FACTORY_AF_MOVE[];
    static const char FACTORY_AF_MOVE_END_CHECK[];
    static const char FACTORY_AF_STEP_LOG[];
    static const char FACTORY_AF_LOCK_START[];
    static const char FACTORY_AF_LOCK_STOP[];
    static const char FACTORY_AF_RANGE_LOG[];
    static const char FACTORY_AF_FOCUS_LOG[];
    static const char FACTORY_AF_INTERRUPT_SET[];
    static const char FACTORY_AF_STEP_SAVE[];
    static const char FACTORY_AF_LED_END_CHECK[];
    static const char FACTORY_AF_LED_LOG[];

    static const char FACTORY_AF_POSITION_FAR[];
    static const char FACTORY_AF_POSITION_NEAR[];
    static const char FACTORY_AF_POSITION_POSITION[];

    static const char FACTORY_DEFOCUS_RUN[];
    static const char FACTORY_DEFOCUS_STOP[];

    static const char FACTORY_CAP_COMP_ON[];
    static const char FACTORY_CAP_COMP_OFF[];
    static const char FACTORY_CAP_COMP_START[];
    static const char FACTORY_CAP_COMP_STOP[];
    static const char FACTORY_CAP_BARREL_ON[];
    static const char FACTORY_CAP_BARREL_OFF[];
    static const char FACTORY_CAP_BARREL_START[];
    static const char FACTORY_CAP_BARREL_STOP[];
    static const char FACTORY_MODE_A[];
    static const char FACTORY_AFLENS_OPEN[];
    static const char FACTORY_AFLENS_CLOSE[];
    static const char FACTORY_AF_ZONE_NORMAL[];
    static const char FACTORY_AF_ZONE_MACRO[];
    static const char FACTORY_AF_ZONE_AUTO_MACRO[];
    static const char FACTORY_ADJ_IRIS_RUN[];
    static const char FACTORY_ADJ_IRIS_STOP[];
    static const char FACTORY_ADJ_IRIS_LOG[];

    static const char FACTORY_ADJ_GAIN_LIVEVIEW_RUN[];
    static const char FACTORY_ADJ_GAIN_LIVEVIEW_STOP[];
    static const char FACTORY_ADJ_GAIN_LIVEVIEW_LOG[];
    static const char FACTORY_SH_CLOSE_LOG[];
    static const char FACTORY_SH_CLOSE_RUN[];
    static const char FACTORY_SH_CLOSE_STOP[];
    static const char FACTORY_CAPTURE_GAIN_RUN[];
    static const char FACTORY_CAPTURE_GAIN_STOP[];

    static const char FACTORY_STILL_CAP_NORMAL[];
    static const char FACTORY_STILL_CAP_DUALCAP[];
    static const char FACTORY_STILL_CAP_ON[];
    static const char FACTORY_STILL_CAP_OFF[];

    static const char FACTORY_FLASH_STOBE_CHECK_ON[];
    static const char FACTORY_FLASH_STOBE_CHECK_OFF[];
    static const char FACTORY_FLASH_CHARGE[];
    static const char FACTORY_FLASH_LOG[];

    static const char FACTORY_WB_INDOOR_RUN[];
    static const char FACTORY_WB_INDOOR_END_CHECK[];
    static const char FACTORY_WB_OUTDOOR_RUN[];
    static const char FACTORY_WB_OUTDOOR_END_CHECK[];
    static const char FACTORY_WB_LOG[];

    static const char FACTORY_DEFECTPIXEL_SCENARIO_6[];
    static const char FACTORY_DEFECTPIXEL_RUN[];
    static const char FACTORY_DEFECTPIXEL_END_CHECK[];
    static const char FACTORY_DEFECTPIXEL_LOG[];
    static const char FACTORY_SYSMODE_CAPTURE[];
    static const char FACTORY_SYSMODE_MONITOR[];
    static const char FACTORY_SYSMODE_PARAM[];

    static const char FACTORY_FLASH_CHARGE_END_CHECK[];

    /**
     * Returns the the supported preview formats as an enum given in graphics.h
     * corrsponding to the format given in the input string or -1 if no such
     * conversion exists.
     */
    static int previewFormatToEnum(const char* format);

private:
    DefaultKeyedVector<String8,String8>    mMap;
};

}; // namespace android

#endif
