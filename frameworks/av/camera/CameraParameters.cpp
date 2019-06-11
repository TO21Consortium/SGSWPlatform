/*
**
** Copyright 2008, The Android Open Source Project
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

#define LOG_TAG "CameraParams"
#include <utils/Log.h>

#include <string.h>
#include <stdlib.h>
#include <camera/CameraParameters.h>
#include <system/graphics.h>

namespace android {
// Parameter keys to communicate between camera application and driver.
const char CameraParameters::KEY_MODE[] = "mode";
const char CameraParameters::KEY_CAPTURE_MODE[] = "capture-mode"; //Added For GalaxyCamera model
const char CameraParameters::KEY_PREVIEW_SIZE[] = "preview-size";
const char CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES[] = "preview-size-values";
const char CameraParameters::KEY_PREVIEW_FORMAT[] = "preview-format";
const char CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS[] = "preview-format-values";
const char CameraParameters::KEY_PREVIEW_FRAME_RATE[] = "preview-frame-rate";
const char CameraParameters::KEY_SUPPORTED_PREVIEW_FRAME_RATES[] = "preview-frame-rate-values";
const char CameraParameters::KEY_PREVIEW_FPS_RANGE[] = "preview-fps-range";
const char CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE[] = "preview-fps-range-values";
const char CameraParameters::KEY_PICTURE_SIZE[] = "picture-size";
const char CameraParameters::KEY_SUPPORTED_PICTURE_SIZES[] = "picture-size-values";
const char CameraParameters::KEY_PICTURE_FORMAT[] = "picture-format";
const char CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS[] = "picture-format-values";
const char CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH[] = "jpeg-thumbnail-width";
const char CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT[] = "jpeg-thumbnail-height";
const char CameraParameters::KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES[] = "jpeg-thumbnail-size-values";
const char CameraParameters::KEY_JPEG_THUMBNAIL_QUALITY[] = "jpeg-thumbnail-quality";
const char CameraParameters::KEY_JPEG_QUALITY[] = "jpeg-quality";
const char CameraParameters::KEY_ROTATION[] = "rotation";
const char CameraParameters::KEY_GPS_LATITUDE[] = "gps-latitude";
const char CameraParameters::KEY_GPS_LONGITUDE[] = "gps-longitude";
const char CameraParameters::KEY_GPS_ALTITUDE[] = "gps-altitude";
const char CameraParameters::KEY_GPS_TIMESTAMP[] = "gps-timestamp";
const char CameraParameters::KEY_GPS_PROCESSING_METHOD[] = "gps-processing-method";
const char CameraParameters::KEY_WHITE_BALANCE[] = "whitebalance";
const char CameraParameters::KEY_SUPPORTED_WHITE_BALANCE[] = "whitebalance-values";
const char CameraParameters::KEY_EFFECT[] = "effect";
const char CameraParameters::KEY_SUPPORTED_EFFECTS[] = "effect-values";
const char CameraParameters::KEY_ANTIBANDING[] = "antibanding";
const char CameraParameters::KEY_SUPPORTED_ANTIBANDING[] = "antibanding-values";
const char CameraParameters::KEY_SCENE_MODE[] = "scene-mode";
const char CameraParameters::KEY_SUPPORTED_SCENE_MODES[] = "scene-mode-values";
const char CameraParameters::KEY_FLASH_MODE[] = "flash-mode";
const char CameraParameters::KEY_FLASH_STANDBY[] = "flash-standby";
const char CameraParameters::KEY_FLASH_CHARGING[] = "flash-charging";
const char CameraParameters::KEY_SUPPORTED_FLASH_MODES[] = "flash-mode-values";
const char CameraParameters::KEY_FOCUS_MODE[] = "focus-mode";
const char CameraParameters::KEY_SUPPORTED_FOCUS_MODES[] = "focus-mode-values";
const char CameraParameters::KEY_MAX_NUM_FOCUS_AREAS[] = "max-num-focus-areas";
const char CameraParameters::KEY_FOCUS_AREAS[] = "focus-areas";
const char CameraParameters::KEY_FOCAL_LENGTH[] = "focal-length";
const char CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE[] = "horizontal-view-angle";
const char CameraParameters::KEY_VERTICAL_VIEW_ANGLE[] = "vertical-view-angle";
const char CameraParameters::KEY_EXPOSURE_COMPENSATION[] = "exposure-compensation";
const char CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION[] = "max-exposure-compensation";
const char CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION[] = "min-exposure-compensation";
const char CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP[] = "exposure-compensation-step";
const char CameraParameters::KEY_AUTO_EXPOSURE_LOCK[] = "auto-exposure-lock";
const char CameraParameters::KEY_AUTO_EXPOSURE_LOCK_SUPPORTED[] = "auto-exposure-lock-supported";
const char CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK[] = "auto-whitebalance-lock";
const char CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK_SUPPORTED[] = "auto-whitebalance-lock-supported";
const char CameraParameters::KEY_MAX_NUM_METERING_AREAS[] = "max-num-metering-areas";
const char CameraParameters::KEY_METERING_AREAS[] = "metering-areas";
const char CameraParameters::KEY_ZOOM[] = "zoom";
const char CameraParameters::KEY_FOCUS_RANGE[] = "focus-range";
const char CameraParameters::KEY_RT_HDR[] = "rt-hdr";
const char CameraParameters::KEY_PHASE_AF[] = "phase-af";
const char CameraParameters::KEY_DYNAMIC_RANGE_CONTROL[] = "dynamic-range-control";

// Added for GalaxyCamera model
const char CameraParameters::KEY_ZOOM_ACTION[] = "zoom-action";
const char CameraParameters::KEY_ZOOM_LENS_STATUS[] = "zoom-lens-status";	// 1 : busy,  0: idle

const char CameraParameters::KEY_MAX_ZOOM[] = "max-zoom";
const char CameraParameters::KEY_ZOOM_RATIOS[] = "zoom-ratios";
const char CameraParameters::KEY_ZOOM_SUPPORTED[] = "zoom-supported";
const char CameraParameters::KEY_SMOOTH_ZOOM_SUPPORTED[] = "smooth-zoom-supported";
const char CameraParameters::KEY_FOCUS_DISTANCES[] = "focus-distances";
const char CameraParameters::KEY_VIDEO_FRAME_FORMAT[] = "video-frame-format";
const char CameraParameters::KEY_VIDEO_SIZE[] = "video-size";
const char CameraParameters::KEY_SUPPORTED_VIDEO_SIZES[] = "video-size-values";
const char CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO[] = "preferred-preview-size-for-video";
const char CameraParameters::KEY_MAX_NUM_DETECTED_FACES_HW[] = "max-num-detected-faces-hw";
const char CameraParameters::KEY_MAX_NUM_DETECTED_FACES_SW[] = "max-num-detected-faces-sw";
const char CameraParameters::KEY_RECORDING_HINT[] = "recording-hint";
const char CameraParameters::KEY_VIDEO_SNAPSHOT_SUPPORTED[] = "video-snapshot-supported";
const char CameraParameters::KEY_VIDEO_STABILIZATION[] = "video-stabilization";
const char CameraParameters::KEY_VIDEO_STABILIZATION_SUPPORTED[] = "video-stabilization-supported";
const char CameraParameters::KEY_LIGHTFX[] = "light-fx";

const char CameraParameters::KEY_ISO[] = "iso";
const char CameraParameters::KEY_FOCUS_AREA_MODE[] = "focus-area-mode";
const char CameraParameters::KEY_FACEDETECT[] = "face-detection";
const char CameraParameters::KEY_METERING[] = "metering";
const char CameraParameters::KEY_CONTRAST[] = "contrast";
const char CameraParameters::KEY_SHARPNESS[] = "sharpness";
const char CameraParameters::KEY_SATURATION[] = "saturation";
const char CameraParameters::KEY_BRACKET_AEB[] = "aeb-value";
const char CameraParameters::KEY_BRACKET_WBB[] = "wbb-value";
const char CameraParameters::KEY_IMAGE_STABILIZER[] = "image-stabilizer";
const char CameraParameters::KEY_SHUTTER_SPEED[] = "shutter-speed";
const char CameraParameters::KEY_APERTURE[] = "aperture";
const char CameraParameters::KEY_CONTINUOUS_MODE[] = "continuous-mode";
const char CameraParameters::KEY_OIS[] = "ois";
const char CameraParameters::KEY_AUTO_VALUE[] = "auto-value";
const char CameraParameters::KEY_RAW_SAVE[] = "raw-save";
const char CameraParameters::KEY_CAPTURE_BURST_FILEPATH[] = "capture-burst-filepath";
const char CameraParameters::KEY_CURRENT_ADDRESS[] = "current-address";

// Values for continuous mode settings.
const char CameraParameters::CONTINUOUS_OFF[] = "off";
const char CameraParameters::CONTINUOUS_ON[] = "on";

const char CameraParameters::KEY_WEATHER[] ="contextualtag-weather";
const char CameraParameters::KEY_CITYID[] ="contextualtag-cityid";

const char CameraParameters::TRUE[] = "true";
const char CameraParameters::FALSE[] = "false";
const char CameraParameters::FOCUS_DISTANCE_INFINITY[] = "Infinity";

// Values for white balance settings.
const char CameraParameters::WHITE_BALANCE_AUTO[] = "auto";
const char CameraParameters::WHITE_BALANCE_INCANDESCENT[] = "incandescent";
const char CameraParameters::WHITE_BALANCE_FLUORESCENT[] = "fluorescent";
const char CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT[] = "warm-fluorescent";
const char CameraParameters::WHITE_BALANCE_DAYLIGHT[] = "daylight";
const char CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT[] = "cloudy-daylight";
const char CameraParameters::WHITE_BALANCE_TWILIGHT[] = "twilight";
const char CameraParameters::WHITE_BALANCE_SHADE[] = "shade";
// added for GalaxyCamera model
const char CameraParameters::WHITE_BALANCE_FLUORESCENT_H[] = "fluorescent-h";
const char CameraParameters::WHITE_BALANCE_FLUORESCENT_L[] = "fluorescent-l";
const char CameraParameters::WHITE_BALANCE_TUNGSTEN[] = "incandescent";
const char CameraParameters::WHITE_BALANCE_CUSTOM[] = "custom";
const char CameraParameters::WHITE_BALANCE_K[] = "temperature";
const char CameraParameters::WHITE_BALANCE_PROHIBITION[] = "prohibition";
const char CameraParameters::WHITE_BALANCE_HORIZON[] = "horizon";
const char CameraParameters::WHITE_BALANCE_LEDLIGHT[] = "ledlight";

// Values for effect settings.
const char CameraParameters::EFFECT_NONE[] = "none";
const char CameraParameters::EFFECT_MONO[] = "mono";
const char CameraParameters::EFFECT_NEGATIVE[] = "negative";
const char CameraParameters::EFFECT_SOLARIZE[] = "solarize";
const char CameraParameters::EFFECT_SEPIA[] = "sepia";
const char CameraParameters::EFFECT_POSTERIZE[] = "posterize";
const char CameraParameters::EFFECT_WHITEBOARD[] = "whiteboard";
const char CameraParameters::EFFECT_BLACKBOARD[] = "blackboard";
const char CameraParameters::EFFECT_AQUA[] = "aqua";
const char CameraParameters::EFFECT_ANTIQUE[] = "antique";
const char CameraParameters::EFFECT_POINT_BLUE[] = "point-blue";
const char CameraParameters::EFFECT_POINT_RED[] = "point-red";
const char CameraParameters::EFFECT_POINT_YELLOW[] = "point-yellow";
const char CameraParameters::EFFECT_WARM[] = "warm";
const char CameraParameters::EFFECT_COLD[] = "cold";
const char CameraParameters::EFFECT_WASHED[] = "washed";

// Values for antibanding settings.
const char CameraParameters::ANTIBANDING_AUTO[] = "auto";
const char CameraParameters::ANTIBANDING_50HZ[] = "50hz";
const char CameraParameters::ANTIBANDING_60HZ[] = "60hz";
const char CameraParameters::ANTIBANDING_OFF[] = "off";

// Values for flash mode settings.
const char CameraParameters::FLASH_MODE_OFF[] = "off";
const char CameraParameters::FLASH_MODE_AUTO[] = "auto";
const char CameraParameters::FLASH_MODE_ON[] = "on";
const char CameraParameters::FLASH_MODE_RED_EYE[] = "red-eye";
const char CameraParameters::FLASH_MODE_TORCH[] = "torch";
const char CameraParameters::FLASH_MODE_FILLIN[] = "fillin";
const char CameraParameters::FLASH_MODE_SLOW_SYNC[] = "slow";
const char CameraParameters::FLASH_MODE_RED_EYE_FIX[] = "red-eye-fix";

const char CameraParameters::FLASH_VALUE_OF_ISP[] = "flash-value-of-isp";

// Values for flash stand by settings.
const char CameraParameters::FLASH_STANDBY_ON[] = "on";
const char CameraParameters::FLASH_STANDBY_OFF[] = "off";

// Values for scene mode settings.
const char CameraParameters::SCENE_MODE_AUTO[] = "auto";
const char CameraParameters::SCENE_MODE_ACTION[] = "action";
const char CameraParameters::SCENE_MODE_PORTRAIT[] = "portrait";
const char CameraParameters::SCENE_MODE_LANDSCAPE[] = "landscape";
const char CameraParameters::SCENE_MODE_NIGHT[] = "night";
const char CameraParameters::SCENE_MODE_NIGHT_PORTRAIT[] = "night-portrait";
const char CameraParameters::SCENE_MODE_THEATRE[] = "theatre";
const char CameraParameters::SCENE_MODE_BEACH[] = "beach";
const char CameraParameters::SCENE_MODE_SNOW[] = "snow";
const char CameraParameters::SCENE_MODE_SUNSET[] = "sunset";
const char CameraParameters::SCENE_MODE_STEADYPHOTO[] = "steadyphoto";
const char CameraParameters::SCENE_MODE_FIREWORKS[] = "fireworks";
const char CameraParameters::SCENE_MODE_SPORTS[] = "sports";
const char CameraParameters::SCENE_MODE_PARTY[] = "party";
const char CameraParameters::SCENE_MODE_CANDLELIGHT[] = "candlelight";
const char CameraParameters::SCENE_MODE_BARCODE[] = "barcode";
const char CameraParameters::SCENE_MODE_HDR[] = "hdr";
const char CameraParameters::SCENE_MODE_BEACH_SNOW[] = "beach-snow";
const char CameraParameters::SCENE_MODE_DUSK_DAWN[] = "dusk-dawn";
const char CameraParameters::SCENE_MODE_FALL_COLOR[] = "fall-color";
const char CameraParameters::SCENE_MODE_BACK_LIGHT[] = "back-light";
// added for GalaxyCamera model
const char CameraParameters::SCENE_MODE_BACK_LIGHT_PORTRAIT[] = "back-light-portrait";
const char CameraParameters::SCENE_MODE_WHITE[] = "white";
const char CameraParameters::SCENE_MODE_NATURAL_GREEN[] = "natural-green";
const char CameraParameters::SCENE_MODE_BLUESKY[] = "bluesky";
const char CameraParameters::SCENE_MODE_MACRO[] = "macro";
const char CameraParameters::SCENE_MODE_MACRO_TEXT[] = "macro-text";
const char CameraParameters::SCENE_MODE_MACRO_COLOR[] = "macro-color";
const char CameraParameters::SCENE_MODE_TRIPOD[] = "tripod";

const char CameraParameters::PIXEL_FORMAT_YUV422SP[] = "yuv422sp";
const char CameraParameters::PIXEL_FORMAT_YUV420SP[] = "yuv420sp";
const char CameraParameters::PIXEL_FORMAT_YUV420SP_NV21[] = "nv21";
const char CameraParameters::PIXEL_FORMAT_YUV422I[] = "yuv422i-yuyv";
const char CameraParameters::PIXEL_FORMAT_YUV420P[]  = "yuv420p";
const char CameraParameters::PIXEL_FORMAT_RGB565[] = "rgb565";
const char CameraParameters::PIXEL_FORMAT_RGBA8888[] = "rgba8888";
const char CameraParameters::PIXEL_FORMAT_JPEG[] = "jpeg";
const char CameraParameters::PIXEL_FORMAT_BAYER_RGGB[] = "bayer-rggb";
const char CameraParameters::PIXEL_FORMAT_ANDROID_OPAQUE[] = "android-opaque";

// Values for focus mode settings.
const char CameraParameters::FOCUS_MODE_AUTO[] = "auto";
const char CameraParameters::FOCUS_MODE_INFINITY[] = "infinity";
const char CameraParameters::FOCUS_MODE_MACRO[] = "macro";
const char CameraParameters::FOCUS_MODE_FIXED[] = "fixed";
const char CameraParameters::FOCUS_MODE_EDOF[] = "edof";
const char CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO[] = "continuous-video";
const char CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE[] = "continuous-picture";
const char CameraParameters::FOCUS_MODE_MANUAL[] = "manual";
// add for GalaxyCamera model
const char CameraParameters::FOCUS_MODE_CONTINUOUS[] = "continuous";

// Values for iso settings.
const char CameraParameters::ISO_AUTO[] = "auto";
const char CameraParameters::ISO_50[] = "50";
const char CameraParameters::ISO_80[] = "80";
const char CameraParameters::ISO_100[] = "100";
const char CameraParameters::ISO_200[] = "200";
const char CameraParameters::ISO_400[] = "400";
const char CameraParameters::ISO_800[] = "800";
const char CameraParameters::ISO_1600[] = "1600";
const char CameraParameters::ISO_3200[] = "3200";
const char CameraParameters::ISO_6400[] = "6400";
const char CameraParameters::ISO_SPORTS[] = "sports";
const char CameraParameters::ISO_NIGHT[] = "night";

// Values for focus area mode settings.
const char CameraParameters::FOCUS_AREA_CENTER[] = "center";
const char CameraParameters::FOCUS_AREA_MULTI[] = "multi";
const char CameraParameters::FOCUS_AREA_SMART_TOUCH[] = "smart-touch";

// Values for focus range mode settings.
const char CameraParameters::FOCUS_RANGE_AUTO[] = "auto";
const char CameraParameters::FOCUS_RANGE_MACRO[] = "macro";
const char CameraParameters::FOCUS_RANGE_AUTO_MACRO[] = "auto-macro";

// Values for focus mode settings.
const char CameraParameters::FOCUS_MODE_MULTI[] = "multi";
const char CameraParameters::FOCUS_MODE_TOUCH[] = "touch";
const char CameraParameters::FOCUS_MODE_OBJECT_TRACKING[] = "object-tracking";
const char CameraParameters::FOCUS_MODE_FACE_DETECTION[] = "face-detection";
const char CameraParameters::FOCUS_MODE_SMART_SELF[] = "self";

// Values for face detection settings.
const char CameraParameters::FACEDETECT_MODE_OFF[] = "off";
const char CameraParameters::FACEDETECT_MODE_NORMAL[] = "normal";
const char CameraParameters::FACEDETECT_MODE_SMILESHOT[] = "smilshot";
const char CameraParameters::FACEDETECT_MODE_BLINK[] = "blink";

// Values for metering settings.
const char CameraParameters::METERING_OFF[] = "off";
const char CameraParameters::METERING_CENTER[] = "center";
const char CameraParameters::METERING_MATRIX[] = "matrix";
const char CameraParameters::METERING_SPOT[] = "spot";

// Values for bracket settings.
const char CameraParameters::BRACKET_MODE_OFF[] = "off";
const char CameraParameters::BRACKET_MODE_AEB[] = "aeb";
const char CameraParameters::BRACKET_MODE_WBB[] = "wbb";

// Values for image stabilizer(ois) settings.
const char CameraParameters::IMAGE_STABILIZER_OFF[] = "off";
const char CameraParameters::IMAGE_STABILIZER_OIS[] = "ois";
const char CameraParameters::IMAGE_STABILIZER_DUALIS[] = "dual-is";

// Values for light fx settings
const char CameraParameters::LIGHTFX_LOWLIGHT[] = "low-light";
const char CameraParameters::LIGHTFX_HDR[] = "high-dynamic-range";

// Values for smart scene detection settings.
const char CameraParameters::SMART_SCENE_DETECT_OFF[] = "off";
const char CameraParameters::SMART_SCENE_DETECT_ON[] = "on";

///////////////////////////////////////////////////////////////////////////
// Added for GalaxyCamera model
// Values for camera modes
const char CameraParameters::MODE_SMART_AUTO[] = "smart-auto";
const char CameraParameters::MODE_PROGRAM[] = "program";
const char CameraParameters::MODE_A[] = "a";
const char CameraParameters::MODE_S[] = "s";
const char CameraParameters::MODE_M[] = "m";
const char CameraParameters::MODE_MAGIC_SHOT[] = "magic-shot";
const char CameraParameters::MODE_BEAUTY_FACE[] = "beauty-face";
const char CameraParameters::MODE_BEST_PHOTO[] = "best-photo";
const char CameraParameters::MODE_CONTINUOUS_SHOT[] = "continuous-shot";
const char CameraParameters::MODE_BEST_FACE[] = "best-face";
const char CameraParameters::MODE_LANDSCAPE[] = "landscape";
const char CameraParameters::MODE_DAWN[] = "dawn";
const char CameraParameters::MODE_SNOW[] = "snow";
const char CameraParameters::MODE_MACRO[] = "macro";
const char CameraParameters::MODE_FOOD[] = "food";
const char CameraParameters::MODE_PARTY_INDOOR[] = "party-indoor";
const char CameraParameters::MODE_ACTION_FREEZE[] = "action-freeze";
const char CameraParameters::MODE_RICH_TONE[] = "rich-tone";
const char CameraParameters::MODE_PANORAMA[] = "panorama";
const char CameraParameters::MODE_WATERFALL[] = "waterfall";
const char CameraParameters::MODE_ERASER[] = "eraser";
const char CameraParameters::MODE_SILHOUETTE[] = "silhouette";
const char CameraParameters::MODE_SUNSET[] = "sunset";
const char CameraParameters::MODE_FIREWORKS[] = "fireworks";
const char CameraParameters::MODE_LIGHT_TRACE[] = "light-trace";
const char CameraParameters::MODE_SMART_SELFSHOT[] = "smart-selfshot";
const char CameraParameters::MODE_SMART_SUGGEST[] = "smart-suggest";
const char CameraParameters::MODE_SMART_BUDDY[] = "smart-buddy";

// Values for camera capture modes
const char CameraParameters::CAPTURE_MODE_SINGLE[] = "single";
const char CameraParameters::CAPTURE_MODE_HDR[] = "hdr";
const char CameraParameters::CAPTURE_MODE_NIGHT[] = "night";
const char CameraParameters::CAPTURE_MODE_CONTINUOUS[] = "continuous";
const char CameraParameters::CAPTURE_MODE_AE_BKT[] = "ae-bkt";
const char CameraParameters::CAPTURE_MODE_RAW_SAVE[] = "raw-save";
const char CameraParameters::CAPTURE_MODE_LOG_SAVE[] = "log-save";
////////////////////////////////////////////////////////////////////////////

// Values for Zoom Action in GalaxyCamera model
const char CameraParameters::ZOOM_ACTION_KEY_START[] = "zoom-key-start";
const char CameraParameters::ZOOM_ACTION_PINCH_START[] = "zoom-pinch-start";
const char CameraParameters::ZOOM_ACTION_STOP[] = "zoom-stop";
const char CameraParameters::ZOOM_ACTION_SLOW_TELE_START[] ="slow-tele-start";
const char CameraParameters::ZOOM_ACTION_SLOW_WIDE_START[]="slow-wide-start";

//Self Shot ROI Box Info
const char CameraParameters::KEY_CURRENT_ROI_LEFT[] = "current_roi_left";
const char CameraParameters::KEY_CURRENT_ROI_TOP[] = "current_roi_top";
const char CameraParameters::KEY_CURRENT_ROI_WIDTH[] = "roi_width";
const char CameraParameters::KEY_CURRENT_ROI_HEIGHT[] = "roi_height";

// Added for Factory test in GalaxyCamera model
const char CameraParameters::KEY_FACTORY_COMMON[] = "factory-common";
const char CameraParameters::KEY_FACTORY_COMMON_END_RESULT[] = "factory-end-result";
const char CameraParameters::KEY_FACTORY_TESTNO[] = "factory-testno";
const char CameraParameters::KEY_FACTORY_COMMON_DOWN_RESULT[] = "factory-down-result";
const char CameraParameters::KEY_FACTORY_LDC[] = "factory-ldc";
const char CameraParameters::KEY_FACTORY_LSC[] = "factory-lsc";
const char CameraParameters::KEY_FACTORY_LSC_ZOOM_INTERRUPT[] = "factory-lsc-zoom-interrupt";
const char CameraParameters::KEY_FACTORY_FAIL_STOP[] = "factory-fail-stop";
const char CameraParameters::KEY_FACTORY_NODEFOCUSYES[] = "factory-nodefocusyes";
const char CameraParameters::KEY_FACTORY_OIS[] = "factory-ois";
const char CameraParameters::KEY_FACTORY_OIS_RANGE_DATA[] = "factory-ois-range-data";
const char CameraParameters::KEY_FACTORY_OIS_CENTERING_MOVE_SHIFT_DATA[] = "factory-ois-centering-move-shift-data";
const char CameraParameters::KEY_FACTORY_VIB[] = "factory-vib";
const char CameraParameters::KEY_FACTORY_VIB_RANGE_DATA[] = "factory-vib-range-data";
const char CameraParameters::KEY_FACTORY_GYRO[] = "factory-gyro";
const char CameraParameters::KEY_FACTORY_GYRO_RANGE_DATA[] = "factory-gyro-range-data";
const char CameraParameters::KEY_FACTORY_BACKLASH_COUNT[] = "factory-backlash-count";
const char CameraParameters::KEY_FACTORY_BACKLASH_MAX[] = "factory-bl-max";
const char CameraParameters::KEY_FACTORY_BACKLASH[] = "factory-backlash";
const char CameraParameters::KEY_FACTORY_INTERPOLATION[] = "factory-interpolation";
const char CameraParameters::KEY_FACTORY_ZOOM[] = "factory-zoom";
const char CameraParameters::KEY_FACTORY_ZOOM_STEP[] = "factory-zoom-step";
const char CameraParameters::KEY_FACTORY_PUNT[] = "factory-punt";
const char CameraParameters::KEY_FACTORY_ZOOM_RANGE_CHECK_DATA[] = "factory-zoom-range-check-data";
const char CameraParameters::KEY_FACTORY_ZOOM_SLOPE_CHECK_DATA[] = "factory-zoom-slope-check-data";
const char CameraParameters::KEY_FACTORY_PUNT_RANGE_DATA[] = "factory-punt-range-data";
const char CameraParameters::KEY_FACTORY_AF[] = "factory-af";
const char CameraParameters::KEY_FACTORY_AF_STEP_SET[] = "factory-af-step";
const char CameraParameters::KEY_FACTORY_AF_POSITION[] = "factory-af-pos";
const char CameraParameters::KEY_FACTORY_AF_SCAN_LIMIT[] = "factory-af-scan-limit";
const char CameraParameters::KEY_FACTORY_AF_SCAN_RANGE[] = "factory-af-scan-range";
const char CameraParameters::KEY_FACTORY_AF_RESOL_CAPTURE[] = "factory-af-resol-capture";
const char CameraParameters::KEY_FACTORY_DEFOCUS[] = "factory-defocus";
const char CameraParameters::KEY_FACTORY_DEFOCUS_WIDE[] = "factory-df-wide";
const char CameraParameters::KEY_FACTORY_DEFOCUS_TELE[] = "factory-df-tele";
const char CameraParameters::KEY_FACTORY_RESOL_CAP[] = "factory-resol";
const char CameraParameters::KEY_FACTORY_MODE[] = "factory-mode";
const char CameraParameters::KEY_FACTORY_AFLENS[] = "factory-aflens";
const char CameraParameters::KEY_FACTORY_RTC_MANUALLY_TIME[] = "factory-rtc-manually-time";
const char CameraParameters::KEY_FACTORY_RTC_MANUALLY_LIMIT[] = "factory-rtc-manually-limit";
const char CameraParameters::KEY_FACTORY_IRIS_RANGE[] = "factory-iris-range";
const char CameraParameters::KEY_FACTORY_GAIN_LIVEVIEW_RANGE[] = "factory-gain-liveview-range";
const char CameraParameters::KEY_FACTORY_AF_ZONE[] = "factory-af-zone";
const char CameraParameters::KEY_FACTORY_OIS_MOVE_SHIFT[] = "factory-shift";
const char CameraParameters::KEY_FACTORY_OIS_MOVE_SHIFT_X[] = "factory-shift-x";
const char CameraParameters::KEY_FACTORY_OIS_MOVE_SHIFT_Y[] = "factory-shift-y";
const char CameraParameters::KEY_FACTORY_PUNT_SHORT_SCAN_DATA[] = "factory-punt-short";
const char CameraParameters::KEY_FACTORY_PUNT_LONG_SCAN_DATA[] = "factory-punt-long";
const char CameraParameters::KEY_FACTORY_LV_TARGET[] = "factory-lv-target";
const char CameraParameters::KEY_FACTORY_ADJ_IRIS[] = "factory-adj-iris";
const char CameraParameters::KEY_FACTORY_ADJ_GAIN_LIVEVIEW[] = "factory-gain-live";
const char CameraParameters::KEY_FACTORY_SH_CLOSE_IRISNUM[] = "factory-sc-iris-num";
const char CameraParameters::KEY_FACTORY_SH_CLOSE_RANGE[] = "factory-sc-range";
const char CameraParameters::KEY_FACTORY_SH_CLOSE_SETIRIS[] = "factory-sc-set-iris";
const char CameraParameters::KEY_FACTORY_SH_CLOSE_ISO[] = "factory-sc-iso";
const char CameraParameters::KEY_FACTORY_SH_CLOSE[] = "factory-sc";
const char CameraParameters::KEY_FACTORY_SH_CLOSE_SPEEDTIME[] = "factory-sc-speedtime";
const char CameraParameters::KEY_FACTORY_FLICKER[] = "factory-flicker";
const char CameraParameters::KEY_FACTORY_GAIN_CAPTURE_RANGE[] = "factory-cap-range";
const char CameraParameters::KEY_FACTORY_CAPTURE_GAIN[] = "factory-cap-gain";
const char CameraParameters::KEY_FACTORY_CAPTURE_GAIN_OFFSET_VAL[] = "factory-cap-gain-offset-val";
const char CameraParameters::KEY_FACTORY_CAPTURE_GAIN_OFFSET_SIGN[] = "factory-cap-gain-offset-sign";
const char CameraParameters::KEY_FACTORY_CAPTURE_CTRL[] = "factory-cap-ctrl";
const char CameraParameters::KEY_FACTORY_AE_TARGET[] = "factory-ae-target";
const char CameraParameters::KEY_FACTORY_LSC_FSHD_TABLE[] = "factory-lsc-table";
const char CameraParameters::KEY_FACTORY_LSC_FSHD_REFERENCE[] = "factory-lsc-ref";
const char CameraParameters::KEY_FACTORY_FLASH_RANGE[] = "factory-flash-range";
const char CameraParameters::KEY_FACTORY_FLASH_CMD[] = "factory-flash-cmd";
const char CameraParameters::KEY_FACTORY_FLASH_MANUAL_CHARGE[] = "factory-flash-manual-charge";
const char CameraParameters::KEY_FACTORY_FLASH_MANUAL_EN[] = "factory-flash-manual-en";
const char CameraParameters::KEY_FACTORY_WB_VALUE[] = "factory-wb-value";
const char CameraParameters::KEY_FACTORY_WB_RANGE[] = "factory-wb-range";
const char CameraParameters::KEY_FACTORY_WB[] = "factory-wb";
const char CameraParameters::KEY_FACTORY_AF_LED_RANGE[] = "factory-af-led-range";
const char CameraParameters::KEY_FACTORY_AF_LED_LV_LIMIT[] = "factory-af-led-lv-limit";
const char CameraParameters::KEY_FACTORY_AF_LED_TIME[] = "factory-af-ledtime";
const char CameraParameters::KEY_FACTORY_AF_DIFF_CHECK[] = "factory-af-diff-check";
const char CameraParameters::KEY_FACTORY_DEFECTPIXEL_SET_NLV_CAPTURE[] = "factory-dfpx-nlvcap";
const char CameraParameters::KEY_FACTORY_DEFECTPIXEL_SET_NLV_DRAFT0[] = "factory-dfpx-dr0";
const char CameraParameters::KEY_FACTORY_DEFECTPIXEL_SET_NLV_DRAFT1[] = "factory-dfpx-dr1";
const char CameraParameters::KEY_FACTORY_DEFECTPIXEL_SET_NLV_DRAFT2[] = "factory-dfpx-dr2";
const char CameraParameters::KEY_FACTORY_DEFECTPIXEL_SET_NLV_DRAFT_HS[] = "factory-dfpx-hs";
const char CameraParameters::KEY_FACTORY_DEFECTPIXEL_SET_NLV_DRAFT1_HD[] = "factory-dfpx-dr1-hd";
const char CameraParameters::KEY_FACTORY_DEFECTPIXEL[] = "factory-dfpx";
const char CameraParameters::KEY_FACTORY_CAM_SYSMODE[] = "factory-sys-mode";
const char CameraParameters::KEY_FACTORY_FLASH_CHECK [] = "factory-flash-chg-chk";
const char CameraParameters::KEY_FACTORY_FW_VER_ISP[] = "factory-fw-ver-isp";
const char CameraParameters::KEY_FACTORY_FW_VER_OIS[] = "factory-fw-ver-ois";
const char CameraParameters::KEY_FACTORY_FW_VER_RESULT[] = "factory-fw-ver-result";
const char CameraParameters::KEY_FACTORY_LIVEVIEW_OFFSET_VAL[] = "factory-liveview-offset-val";
const char CameraParameters::KEY_FACTORY_LIVEVIEW_OFFSET_SIGN[] = "factory-liveview-offset-sign";
const char CameraParameters::KEY_FACTORY_TILT_SCAN_LIMIT[] = "factory-tilt-scan-limiit";
const char CameraParameters::KEY_FACTORY_TILT_FIELD[] = "factory-tilt-field";
const char CameraParameters::KEY_FACTORY_TILT_AF_RANGE[] = "factory-tilt-af-range";
const char CameraParameters::KEY_FACTORY_TILT_DIFF_RANGE[] = "factory-tilt-diff-range";
const char CameraParameters::KEY_FACTORY_TILT_1TIME_SCRIPT_RUN[] = "factory-tilt-1time-sciprt-run";
const char CameraParameters::KEY_FACTORY_IRCHECK_RGAIN[] = "factory-ircheck-rgain";
const char CameraParameters::KEY_FACTORY_IRCHECK_BGAIN[] = "factory-ircheck-bgain";
const char CameraParameters::KEY_FACTORY_IRCHECK_RUN[] = "ircheck-run";

const char CameraParameters::FACTORY_FIRMWARE_DOWNLOAD[] = "firm-dl";
const char CameraParameters::FACTORY_DOWNLOAD_CHECK[] = "dl-check";
const char CameraParameters::FACTORY_END_CHECK[] = "end-check";
const char CameraParameters::FACTORY_COMMON_SET_FOCUS_ZONE_MACRO[] = "macro";

const char CameraParameters::FACTORY_LDC_OFF[] = "off";
const char CameraParameters::FACTORY_LDC_ON[] = "on";

const char CameraParameters::FACTORY_LSC_OFF[] = "off";
const char CameraParameters::FACTORY_LSC_ON[] = "on";
const char CameraParameters::FACTORY_LSC_LOG[] = "lsc-log";

const char CameraParameters::FACTORY_LSC_ZOOM_INTERRUPT_ENABLE[] = "enable";
const char CameraParameters::FACTORY_LSC_ZOOM_INTERRUPT_DISABLE[] = "disable";

const char CameraParameters::FACTORY_FAIL_STOP_ON[] = "on";
const char CameraParameters::FACTORY_FAIL_STOP_OFF[] = "off";
const char CameraParameters::FACTORY_FAIL_STOP_RUN[] = "run";
const char CameraParameters::FACTORY_FAIL_STOP_STOP[] = "stop";

const char CameraParameters::FACTORY_NODEFOCUSYES_ON[] = "on";
const char CameraParameters::FACTORY_NODEFOCUSYES_OFF[] = "off";
const char CameraParameters::FACTORY_NODEFOCUSYES_RUN[] = "run";
const char CameraParameters::FACTORY_NODEFOCUSYES_STOP[] = "stop";

const char CameraParameters::FACTORY_OIS_RETURN_TO_CENTER[] = "return-center";
const char CameraParameters::FACTORY_OIS_MODE_ON[] = "on";
const char CameraParameters::FACTORY_OIS_MODE_OFF[] = "off";
const char CameraParameters::FACTORY_OIS_RUN[] = "run";
const char CameraParameters::FACTORY_OIS_START[] = "start";
const char CameraParameters::FACTORY_OIS_STOP[] = "stop";
const char CameraParameters::FACTORY_OIS_LOG[] = "log";

const char CameraParameters::FACTORY_VIB_START[] = "start";
const char CameraParameters::FACTORY_VIB_STOP[] = "stop";
const char CameraParameters::FACTORY_VIB_LOG[] = "log";

const char CameraParameters::FACTORY_GYRO_START[] = "start";
const char CameraParameters::FACTORY_GYRO_STOP[] = "stop";
const char CameraParameters::FACTORY_GYRO_LOG[] = "log";

const char CameraParameters::FACTORY_BACKLASH_INPUT[] = "input";
const char CameraParameters::FACTORY_BACKLASH_INPUT_MAX_THR[] = "max-thr";
const char CameraParameters::FACTORY_BACKLASH_WIDE_RUN[] = "wide-run";
const char CameraParameters::FACTORY_BACKLASH_LOG[] = "log";

const char CameraParameters::FACTORY_INTERPOLATION_USE[] = "use";
const char CameraParameters::FACTORY_INTERPOLATION_RELEASE[] = "release";

const char CameraParameters::FACTORY_ZOOM_MOVE_STEP[] = "move-step";
const char CameraParameters::FACTORY_ZOOM_RANGE_CHECK_START[] = "range-check-start";
const char CameraParameters::FACTORY_ZOOM_RANGE_CHECK_STOP[] = "range-check-stop";
const char CameraParameters::FACTORY_ZOOM_SLOPE_CHECK_START[] = "slope-check-start";
const char CameraParameters::FACTORY_ZOOM_SLOPE_CHECK_STOP[] = "slope-check-stop";
const char CameraParameters::FACTORY_ZOOM_STEP_TELE[] = "tele";
const char CameraParameters::FACTORY_ZOOM_STEP_WIDE[] = "wide";
const char CameraParameters::FACTORY_ZOOM_CLOSE[] = "close";
const char CameraParameters::FACTORY_ZOOM_MOVE_END_CHECK[] = "end-check";

const char CameraParameters::FACTORY_PUNT_RANGE_START[] = "range-start";
const char CameraParameters::FACTORY_PUNT_RANGE_STOP[] = "range-stop";
const char CameraParameters::FACTORY_PUNT_SHORT_SCAN_DATA[] = "short-scan-data";
const char CameraParameters::FACTORY_PUNT_SHORT_SCAN_START[] = "short-scan-start";
const char CameraParameters::FACTORY_PUNT_SHORT_SCAN_STOP[] = "short-scan-stop";
const char CameraParameters::FACTORY_PUNT_LONG_SCAN_DATA[] = "long-scan-data";
const char CameraParameters::FACTORY_PUNT_LONG_SCAN_START[] = "long-scan-start";
const char CameraParameters::FACTORY_PUNT_LONG_SCAN_STOP[] = "long-scan-stop";
const char CameraParameters::FACTORY_PUNT_LOG[] = "log";
const char CameraParameters::FACTORY_PUNT_EEP_WRITE[] = "eep-write";

const char CameraParameters::FACTORY_AF_LOCK_ON_SET[] = "lock-on";
const char CameraParameters::FACTORY_AF_LOCK_OFF_SET[] = "lock-off";
const char CameraParameters::FACTORY_AF_MOVE[] = "move";
const char CameraParameters::FACTORY_AF_MOVE_END_CHECK[] = "end-check";
const char CameraParameters::FACTORY_AF_STEP_LOG[] = "step-log";
const char CameraParameters::FACTORY_AF_LOCK_START[] = "lock-start";
const char CameraParameters::FACTORY_AF_LOCK_STOP[] = "lock-stop";
const char CameraParameters::FACTORY_AF_RANGE_LOG[] = "range-log";
const char CameraParameters::FACTORY_AF_FOCUS_LOG[] = "focus-log";
const char CameraParameters::FACTORY_AF_INTERRUPT_SET[] = "int-set";
const char CameraParameters::FACTORY_AF_STEP_SAVE[] = "step-save";
const char CameraParameters::FACTORY_AF_LED_END_CHECK[] = "led-end-check";
const char CameraParameters::FACTORY_AF_LED_LOG[] = "led-log";

const char CameraParameters::FACTORY_AF_POSITION_FAR[] = "0";
const char CameraParameters::FACTORY_AF_POSITION_NEAR[] = "1";
const char CameraParameters::FACTORY_AF_POSITION_POSITION[] = "3";

const char CameraParameters::FACTORY_DEFOCUS_RUN[] = "run";
const char CameraParameters::FACTORY_DEFOCUS_STOP[] = "stop";

const char CameraParameters::FACTORY_CAP_COMP_ON[] = "comp-on";
const char CameraParameters::FACTORY_CAP_COMP_OFF[] = "comp-off";
const char CameraParameters::FACTORY_CAP_COMP_START[] = "comp-start";
const char CameraParameters::FACTORY_CAP_COMP_STOP[] = "comp-stop";
const char CameraParameters::FACTORY_CAP_BARREL_ON[] = "barrel-on";
const char CameraParameters::FACTORY_CAP_BARREL_OFF[] = "barrel-off";
const char CameraParameters::FACTORY_CAP_BARREL_START[] = "barrel-start";
const char CameraParameters::FACTORY_CAP_BARREL_STOP[] = "barrel-stop";
const char CameraParameters::FACTORY_MODE_A[] = "a";

const char CameraParameters::FACTORY_AFLENS_OPEN[] = "open";
const char CameraParameters::FACTORY_AFLENS_CLOSE[] = "close";

const char CameraParameters::FACTORY_AF_ZONE_NORMAL[] = "normal";
const char CameraParameters::FACTORY_AF_ZONE_MACRO[] = "macro";
const char CameraParameters::FACTORY_AF_ZONE_AUTO_MACRO[] = "auto-macro";

const char CameraParameters::FACTORY_ADJ_IRIS_RUN[] = "run";
const char CameraParameters::FACTORY_ADJ_IRIS_STOP[] = "stop";
const char CameraParameters::FACTORY_ADJ_IRIS_LOG[] = "log";

const char CameraParameters::FACTORY_ADJ_GAIN_LIVEVIEW_RUN[] = "run";
const char CameraParameters::FACTORY_ADJ_GAIN_LIVEVIEW_STOP[] = "stop";
const char CameraParameters::FACTORY_ADJ_GAIN_LIVEVIEW_LOG[] = "log";

const char CameraParameters::FACTORY_SH_CLOSE_RUN[] = "run";
const char CameraParameters::FACTORY_SH_CLOSE_STOP[] = "stop";
const char CameraParameters::FACTORY_SH_CLOSE_LOG[] = "log";

const char CameraParameters::FACTORY_CAPTURE_GAIN_RUN[] = "run";
const char CameraParameters::FACTORY_CAPTURE_GAIN_STOP[] = "stop";
const char CameraParameters::FACTORY_STILL_CAP_NORMAL[] = "normal";
const char CameraParameters::FACTORY_STILL_CAP_DUALCAP[] = "dualcap";
const char CameraParameters::FACTORY_STILL_CAP_ON[] = "dual-on";
const char CameraParameters::FACTORY_STILL_CAP_OFF[] = "dual-off";

const char CameraParameters::FACTORY_FLASH_STOBE_CHECK_ON[] = "check-on";
const char CameraParameters::FACTORY_FLASH_STOBE_CHECK_OFF[] = "check-off";
const char CameraParameters::FACTORY_FLASH_CHARGE[] = "charge";
const char CameraParameters::FACTORY_FLASH_LOG[] = "log";

const char CameraParameters::FACTORY_WB_INDOOR_RUN[] = "indoor-run";
const char CameraParameters::FACTORY_WB_INDOOR_END_CHECK[] = "indoor-end-check";
const char CameraParameters::FACTORY_WB_OUTDOOR_RUN[] = "outdoor-run";
const char CameraParameters::FACTORY_WB_OUTDOOR_END_CHECK[] = "outdoor-end-check";
const char CameraParameters::FACTORY_WB_LOG[] = "log";

const char CameraParameters::FACTORY_DEFECTPIXEL_SCENARIO_6[] = "scenario-6";
const char CameraParameters::FACTORY_DEFECTPIXEL_RUN[] = "run";
const char CameraParameters::FACTORY_DEFECTPIXEL_END_CHECK[] = "end-check";
const char CameraParameters::FACTORY_DEFECTPIXEL_LOG[] = "log";
const char CameraParameters::FACTORY_SYSMODE_CAPTURE[] = "capture";
const char CameraParameters::FACTORY_SYSMODE_MONITOR[] = "monitor";
const char CameraParameters::FACTORY_SYSMODE_PARAM[] = "param";
const char CameraParameters::FACTORY_FLASH_CHARGE_END_CHECK[] = "charge-end";

CameraParameters::CameraParameters()
                : mMap()
{
}

CameraParameters::~CameraParameters()
{
}

String8 CameraParameters::flatten() const
{
    String8 flattened("");
    size_t size = mMap.size();

    for (size_t i = 0; i < size; i++) {
        String8 k, v;
        k = mMap.keyAt(i);
        v = mMap.valueAt(i);

        flattened += k;
        flattened += "=";
        flattened += v;
        if (i != size-1)
            flattened += ";";
    }

    return flattened;
}

void CameraParameters::unflatten(const String8 &params)
{
    const char *a = params.string();
    const char *b;

    mMap.clear();

    for (;;) {
        // Find the bounds of the key name.
        b = strchr(a, '=');
        if (b == 0)
            break;

        // Create the key string.
        String8 k(a, (size_t)(b-a));

        // Find the value.
        a = b+1;
        b = strchr(a, ';');
        if (b == 0) {
            // If there's no semicolon, this is the last item.
            String8 v(a);
            mMap.add(k, v);
            break;
        }

        String8 v(a, (size_t)(b-a));
        mMap.add(k, v);
        a = b+1;
    }
}


void CameraParameters::set(const char *key, const char *value)
{
    // XXX i think i can do this with strspn()
    if (strchr(key, '=') || strchr(key, ';')) {
        //XXX ALOGE("Key \"%s\"contains invalid character (= or ;)", key);
        return;
    }

    if (strchr(value, '=') || strchr(value, ';')) {
        //XXX ALOGE("Value \"%s\"contains invalid character (= or ;)", value);
        return;
    }

    mMap.replaceValueFor(String8(key), String8(value));
}

void CameraParameters::set(const char *key, int value)
{
    char str[16];
    sprintf(str, "%d", value);
    set(key, str);
}

void CameraParameters::setFloat(const char *key, float value)
{
    char str[16];  // 14 should be enough. We overestimate to be safe.
    snprintf(str, sizeof(str), "%g", value);
    set(key, str);
}

const char *CameraParameters::get(const char *key) const
{
    String8 v = mMap.valueFor(String8(key));
    if (v.length() == 0)
        return 0;
    return v.string();
}

int CameraParameters::getInt(const char *key) const
{
    const char *v = get(key);
    if (v == 0)
        return -1;
    return strtol(v, 0, 0);
}

long long int CameraParameters::getInt64(const char *key) const
{
    const char *v = get(key);
    if (v == 0)
    {
        return -1;
    }
    long long int value = strtoll(v, 0, 0);
    return value;
}

float CameraParameters::getFloat(const char *key) const
{
    const char *v = get(key);
    if (v == 0) return -1;
    return strtof(v, 0);
}

void CameraParameters::remove(const char *key)
{
    mMap.removeItem(String8(key));
}

// Parse string like "640x480" or "10000,20000"
static int parse_pair(const char *str, int *first, int *second, char delim,
                      char **endptr = NULL)
{
    // Find the first integer.
    char *end;
    int w = (int)strtol(str, &end, 10);
    // If a delimeter does not immediately follow, give up.
    if (*end != delim) {
        ALOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return -1;
    }

    // Find the second integer, immediately after the delimeter.
    int h = (int)strtol(end+1, &end, 10);

    *first = w;
    *second = h;

    if (endptr) {
        *endptr = end;
    }

    return 0;
}

// Added for Factory Test in GalaxyCamera model
static int parse_punt_range_data(const char *str, int *first, int *second, int *third, char delim,
                      char **endptr = NULL)
{
    // Find the first integer.
    char *end;
    int min = (int)strtol(str, &end, 10);
    // If a delimeter does not immediately follow, give up.
    if (*end != delim) {
        ALOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return -1;
    }

    // Find the second integer, immediately after the delimeter.
    int max = (int)strtol(end+1, &end, 10);

    int num = (int)strtol(end+1, &end, 10);

    *first = min;
    *second = max;
    *third = num;

    if (endptr) {
        *endptr = end;
    }

    return 0;
}

// Added for Factory Test in GalaxyCamera model
static int parse_rtc_manually_time_data(const char *str, int *first, int *second, int *third, int *fourth, int *fifth, char delim,
                      char **endptr = NULL)
{
    // Find the first integer.
    char *end;
    int year = (int)strtol(str, &end, 10);
    // If a delimeter does not immediately follow, give up.
    if (*end != delim) {
        ALOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return -1;
    }

    // Find the second integer, immediately after the delimeter.
    int month = (int)strtol(end+1, &end, 10);
    int day = (int)strtol(end+1, &end, 10);
    int hour = (int)strtol(end+1, &end, 10);
    int minute = (int)strtol(end+1, &end, 10);

    *first = year;
    *second = month;
    *third = day;
    *fourth = hour;
    *fifth = minute;

    if (endptr) {
        *endptr = end;
    }

    return 0;
}

// Added for Factory Test in GalaxyCamera model
static int parse_vibe_range_data(const char *str, int *first, int *second, int *third, int *fourth, int *fifth, int *sixth, char delim,
                      char **endptr = NULL)
{
    // Find the first integer.
    char *end;
    int x_min = (int)strtol(str, &end, 10);
    // If a delimeter does not immediately follow, give up.
    if (*end != delim) {
        ALOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return -1;
    }

    // Find the second integer, immediately after the delimeter.
    int x_max = (int)strtol(end+1, &end, 10);
    int y_min = (int)strtol(end+1, &end, 10);
    int y_max = (int)strtol(end+1, &end, 10);
    int min = (int)strtol(end+1, &end, 10);
    int max = (int)strtol(end+1, &end, 10);

    *first = x_min;
    *second = x_max;
    *third = y_min;
    *fourth = y_max;
    *fifth = min;
    *sixth = max;

    if (endptr) {
        *endptr = end;
    }

    return 0;
}

static int parse_gyro_range_data(const char *str, int *first, int *second, int *third, int *fourth, char delim,
                      char **endptr = NULL)
{
    // Find the first integer.
    char *end;
    int x1 = (int)strtol(str, &end, 10);
    // If a delimeter does not immediately follow, give up.
    if (*end != delim) {
        ALOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return -1;
    }

    // Find the second integer, immediately after the delimeter.
    int y1 = (int)strtol(end+1, &end, 10);
    int x2 = (int)strtol(end+1, &end, 10);
    int y2 = (int)strtol(end+1, &end, 10);

    *first = x1;
    *second = y1;
    *third = x2;
    *fourth = y2;

    if (endptr) {
        *endptr = end;
    }

    return 0;
}



// Added for Factory Test in GalaxyCamera model
static int parse_ois_range_data(const char *str, int *first, int *second, int *third, int *fourth, int *fifth, int *sixth, int *seventh, char delim,
                      char **endptr = NULL)
{
    // Find the first integer.
    char *end;
    int x_min = (int)strtol(str, &end, 10);
    // If a delimeter does not immediately follow, give up.
    if (*end != delim) {
        ALOGE("Cannot find delimeter (%c) in str=%s", delim, str);
        return -1;
    }

    // Find the second integer, immediately after the delimeter.
    int x_max = (int)strtol(end+1, &end, 10);
    int y_min = (int)strtol(end+1, &end, 10);
    int y_max = (int)strtol(end+1, &end, 10);
    int x_gain = (int)strtol(end+1, &end, 10);
    int peak_x = (int)strtol(end+1, &end, 10);
    int peak_y = (int)strtol(end+1, &end, 10);

    *first = x_min;
    *second = x_max;
    *third = y_min;
    *fourth = y_max;
    *fifth = x_gain;
    *sixth = peak_x;
    *seventh = peak_y;

    if (endptr) {
        *endptr = end;
    }

    return 0;
}

static void parseSizesList(const char *sizesStr, Vector<Size> &sizes)
{
    if (sizesStr == 0) {
        return;
    }

    char *sizeStartPtr = (char *)sizesStr;

    while (true) {
        int width, height;
        int success = parse_pair(sizeStartPtr, &width, &height, 'x',
                                 &sizeStartPtr);
        if (success == -1 || (*sizeStartPtr != ',' && *sizeStartPtr != '\0')) {
            ALOGE("Picture sizes string \"%s\" contains invalid character.", sizesStr);
            return;
        }
        sizes.push(Size(width, height));

        if (*sizeStartPtr == '\0') {
            return;
        }
        sizeStartPtr++;
    }
}

void CameraParameters::setPreviewSize(int width, int height)
{
    char str[32];
    sprintf(str, "%dx%d", width, height);
    set(KEY_PREVIEW_SIZE, str);
}

void CameraParameters::getPreviewSize(int *width, int *height) const
{
    *width = *height = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_PREVIEW_SIZE);
    if (p == 0)  return;
    parse_pair(p, width, height, 'x');
}

// Added for Factory in GalaxyCamera model
void CameraParameters::setZoomRangeCheckData(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_ZOOM_RANGE_CHECK_DATA, str);
}

// Added for Factory in GalaxyCamera model
void CameraParameters::getZoomRangeCheckData(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_ZOOM_RANGE_CHECK_DATA);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

// Added for Factory in GalaxyCamera model
void CameraParameters::setZoomSlopeCheckData(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_ZOOM_SLOPE_CHECK_DATA, str);
}

// Added for Factory in GalaxyCamera model
void CameraParameters::getZoomSlopeCheckData(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_ZOOM_SLOPE_CHECK_DATA);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

// Added for Factory in GalaxyCamera model
void CameraParameters::setPuntRangeData(int min, int max, int num)
{
    char str[32];
    sprintf(str, "%d,%d,%d", min, max, num);
    set(KEY_FACTORY_PUNT_RANGE_DATA, str);
}

// Added for Factgory in GalaxyCamera model
void CameraParameters::getPuntRangeData(int *min, int *max, int *num) const
{
    *min = *max = *num = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_PUNT_RANGE_DATA);
    if (p == 0)  return;
    parse_punt_range_data(p, min, max, num, ',');
}

// Added for Factory in GalaxyCamera model
void CameraParameters::setOISRangeData(int x_min, int x_max, int y_min, int y_max, int x_gain, int peak_x, int peak_y)
{
    char str[64];
    sprintf(str, "%d,%d,%d,%d,%d,%d,%d", x_min, x_max, y_min, y_max, x_gain, peak_x, peak_y);
    set(KEY_FACTORY_OIS_RANGE_DATA, str);
}

// Added for Factgory in GalaxyCamera model
void CameraParameters::getOISRangeData(int *x_min, int *x_max, int *y_min, int *y_max, int *x_gain, int *peak_x, int *peak_y) const
{
    *x_min = *x_max = *y_min = *y_max = *x_gain = *peak_x = *peak_y = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_OIS_RANGE_DATA);
    if (p == 0)  return;
    parse_ois_range_data(p, x_min, x_max, y_min, y_max, x_gain, peak_x, peak_y, ',');
}

// Added for Factory in GalaxyCamera model
void CameraParameters::setVibRangeData(int x_min, int x_max, int y_min, int y_max, int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d,%d,%d,%d,%d", x_min, x_max, y_min, y_max, min, max);
    set(KEY_FACTORY_VIB_RANGE_DATA, str);
}

// Added for Factory in GalaxyCamera model
void CameraParameters::getVibRangeData(int *x_min, int *x_max, int *y_min, int *y_max, int *min, int *max) const
{
    *x_min = *x_max = *y_min = *y_max = *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_VIB_RANGE_DATA);
    if (p == 0)  return;
    parse_vibe_range_data(p, x_min, x_max, y_min, y_max, min, max, ',');
}

// Added for Factory in GalaxyCamera model
void CameraParameters::setGyroRangeData(int x1, int y1, int x2, int y2)
{
    char str[32];
    sprintf(str, "%d,%d,%d,%d", x1, y1, x2, y2);
    set(KEY_FACTORY_GYRO_RANGE_DATA, str);
}

// Added for Factory in GalaxyCamera model
void CameraParameters::getGyroRangeData(int *x1, int *y1, int *x2, int *y2) const
{
    *x1 = *y1 = *x2 = *y2 = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_GYRO_RANGE_DATA);
    if (p == 0)  return;
    parse_gyro_range_data(p, x1, y1, x2, y2, ',');
}

void CameraParameters::setAFScanLimit(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_AF_SCAN_LIMIT, str);
}

void CameraParameters::getAFScanLimit(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_AF_SCAN_LIMIT);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

void CameraParameters::setAFScanRange(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_AF_SCAN_RANGE, str);
}

void CameraParameters::getAFScanRange(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_AF_SCAN_RANGE);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

void CameraParameters::setAFResolCapture(int min, int max, int af_unit_step)
{
    char str[32];
    sprintf(str, "%d,%d,%d", min, max, af_unit_step);
    set(KEY_FACTORY_AF_RESOL_CAPTURE, str);
}

// Added for Factgory in GalaxyCamera model
void CameraParameters::getAFResolCapture(int *min, int *max, int *af_unit_step) const
{
    *min = *max = *af_unit_step = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_AF_RESOL_CAPTURE);
    if (p == 0)  return;
    parse_punt_range_data(p, min, max, af_unit_step, ',');
}

void CameraParameters::setRTCManuallyTime(int year, int month, int day, int hour, int minute)
{
    char str[32];
    sprintf(str, "%d,%d,%d,%d,%d", year, month, day, hour, minute);
    set(KEY_FACTORY_RTC_MANUALLY_TIME, str);
}

// Added for Factgory in GalaxyCamera model
void CameraParameters::getRTCManuallyTime(int *year, int *month, int *day, int *hour, int *minute) const
{
    *year = *month = *day = *hour = *minute = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_RTC_MANUALLY_TIME);
    if (p == 0)  return;
    parse_rtc_manually_time_data(p, year, month, day, hour, minute, ',');
}

void CameraParameters::setRTCManuallyLimit(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_RTC_MANUALLY_LIMIT, str);
}

void CameraParameters::getRTCManuallyLimit(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_RTC_MANUALLY_LIMIT);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

void CameraParameters::setIRISRange(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_IRIS_RANGE, str);
}

void CameraParameters::getIRISRange(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_IRIS_RANGE);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

void CameraParameters::setGainLiveviewRange(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_GAIN_LIVEVIEW_RANGE, str);
}

void CameraParameters::getGainLiveviewRange(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_GAIN_LIVEVIEW_RANGE);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

void CameraParameters::setSCSpeedtime(int x, int y)
{
    char str[32];
    sprintf(str, "%d,%d", x, y);
    set(KEY_FACTORY_SH_CLOSE_SPEEDTIME, str);
}

void CameraParameters::getSCSpeedtime(int *x, int *y) const
{
    *x = *y = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_SH_CLOSE_SPEEDTIME);
    if (p == 0)  return;
    parse_pair(p, x, y, ',');
}

void CameraParameters::setGainCaptureRange(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_GAIN_CAPTURE_RANGE, str);
}

void CameraParameters::getGainCaptureRange(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_GAIN_CAPTURE_RANGE);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

void CameraParameters::setFlashRange(int x, int y)
{
    char str[32];
    sprintf(str, "%d,%d", x, y);
    set(KEY_FACTORY_FLASH_RANGE, str);
}

void CameraParameters::getFlashRange(int *x, int *y) const
{
    *x = *y = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_FLASH_RANGE);
    if (p == 0)  return;
    parse_pair(p, x, y, ',');
}

void CameraParameters::setWhiteBalanceValue(int indoorRG, int indoorBG, int outdoorRG, int outdoorBG)
{
    char str[32];
    sprintf(str, "%d,%d,%d,%d", indoorRG, indoorBG, outdoorRG, outdoorBG);
    set(KEY_FACTORY_WB_VALUE, str);
}

// Added for Factgory in GalaxyCamera model
void CameraParameters::getWhiteBalanceValue(int *indoorRG, int *indoorBG, int *outdoorRG, int *outdoorBG) const
{
    *indoorRG = *indoorBG = *outdoorRG = *outdoorBG = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_WB_VALUE);
    if (p == 0)  return;
    parse_gyro_range_data(p, indoorRG, indoorBG, outdoorRG, outdoorBG,  ',');
}

void CameraParameters::setAFLEDRange(int startX, int endX, int startY, int endY)
{
    char str[32];
    sprintf(str, "%d,%d,%d,%d", startX, endX, startY, endY);
    set(KEY_FACTORY_AF_LED_RANGE, str);
}

// Added for Factgory in GalaxyCamera model
void CameraParameters::getAFLEDRange(int *startX, int *endX, int *startY, int *endY) const
{
    *startX = *endX = *startY = *endY = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_AF_LED_RANGE);
    if (p == 0)  return;
    parse_gyro_range_data(p, startX, endX, startY, endY,  ',');
}

void CameraParameters::setAFLEDLVLimit(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_AF_LED_LV_LIMIT, str);
}

void CameraParameters::getAFLEDLVLimit(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_AF_LED_LV_LIMIT);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

void CameraParameters::setAFDiffCheck(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_AF_DIFF_CHECK, str);
}

void CameraParameters::getAFDiffCheck(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_AF_DIFF_CHECK);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

void CameraParameters::setTiltScanLimit(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_TILT_SCAN_LIMIT, str);
}

void CameraParameters::getTiltScanLimit(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_TILT_SCAN_LIMIT);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

void CameraParameters::setTiltAfRange(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_TILT_AF_RANGE, str);
}

void CameraParameters::getTiltAfRange(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_TILT_AF_RANGE);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

void CameraParameters::setTiltDiffRange(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_TILT_DIFF_RANGE, str);
}

void CameraParameters::getTiltDiffRange(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_TILT_DIFF_RANGE);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

void CameraParameters::setIrCheckRGain(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_IRCHECK_RGAIN, str);
}

void CameraParameters::getIrCheckRGain(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_IRCHECK_RGAIN);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

void CameraParameters::setIrCheckBGain(int min, int max)
{
    char str[32];
    sprintf(str, "%d,%d", min, max);
    set(KEY_FACTORY_IRCHECK_BGAIN, str);
}

void CameraParameters::getIrCheckBGain(int *min, int *max) const
{
    *min = *max = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_FACTORY_IRCHECK_BGAIN);
    if (p == 0)  return;
    parse_pair(p, min, max, ',');
}

void CameraParameters::getPreferredPreviewSizeForVideo(int *width, int *height) const
{
    *width = *height = -1;
    const char *p = get(KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO);
    if (p == 0)  return;
    parse_pair(p, width, height, 'x');
}

void CameraParameters::getSupportedPreviewSizes(Vector<Size> &sizes) const
{
    const char *previewSizesStr = get(KEY_SUPPORTED_PREVIEW_SIZES);
    parseSizesList(previewSizesStr, sizes);
}

void CameraParameters::setVideoSize(int width, int height)
{
    char str[32];
    sprintf(str, "%dx%d", width, height);
    set(KEY_VIDEO_SIZE, str);
}

void CameraParameters::getVideoSize(int *width, int *height) const
{
    *width = *height = -1;
    const char *p = get(KEY_VIDEO_SIZE);
    if (p == 0) return;
    parse_pair(p, width, height, 'x');
}

void CameraParameters::getSupportedVideoSizes(Vector<Size> &sizes) const
{
    const char *videoSizesStr = get(KEY_SUPPORTED_VIDEO_SIZES);
    parseSizesList(videoSizesStr, sizes);
}

void CameraParameters::setPreviewFrameRate(int fps)
{
    set(KEY_PREVIEW_FRAME_RATE, fps);
}

int CameraParameters::getPreviewFrameRate() const
{
    return getInt(KEY_PREVIEW_FRAME_RATE);
}

void CameraParameters::getPreviewFpsRange(int *min_fps, int *max_fps) const
{
    *min_fps = *max_fps = -1;
    const char *p = get(KEY_PREVIEW_FPS_RANGE);
    if (p == 0) return;
    parse_pair(p, min_fps, max_fps, ',');
}

void CameraParameters::setPreviewFormat(const char *format)
{
    set(KEY_PREVIEW_FORMAT, format);
}

const char *CameraParameters::getPreviewFormat() const
{
    return get(KEY_PREVIEW_FORMAT);
}

void CameraParameters::setPictureSize(int width, int height)
{
    char str[32];
    sprintf(str, "%dx%d", width, height);
    set(KEY_PICTURE_SIZE, str);
}

void CameraParameters::getPictureSize(int *width, int *height) const
{
    *width = *height = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_PICTURE_SIZE);
    if (p == 0) return;
    parse_pair(p, width, height, 'x');
}

void CameraParameters::getSupportedPictureSizes(Vector<Size> &sizes) const
{
    const char *pictureSizesStr = get(KEY_SUPPORTED_PICTURE_SIZES);
    parseSizesList(pictureSizesStr, sizes);
}

void CameraParameters::setPictureFormat(const char *format)
{
    set(KEY_PICTURE_FORMAT, format);
}

const char *CameraParameters::getPictureFormat() const
{
    return get(KEY_PICTURE_FORMAT);
}

void CameraParameters::setAutoParameter(int now_av, int now_tv, int now_ev, int now_scenemode, int now_scenesubmode, int now_FDnum, int now_OTstatus, int now_Brightness, int now_iso)
{
    char str[32];
    sprintf(str, "%d,%d,%d,%d,%d,%d,%d,%d,%d", now_av, now_tv, now_ev, now_scenemode, now_scenesubmode, now_FDnum, now_OTstatus, now_Brightness, now_iso);
    set(KEY_AUTO_VALUE, str);
}

void CameraParameters::getAutoParameter(int* now_av, int* now_tv, int* now_ev, int* now_scenemode, int* now_scenesubmode, int* now_FDnum, int* now_OTstatus, int* now_Brightness, int* now_iso) const
{
    *now_av = *now_tv = *now_ev = *now_scenemode = *now_scenesubmode = *now_FDnum = *now_OTstatus = *now_Brightness = *now_iso = -1;
    // Get the current string, if it doesn't exist, leave the -1x-1
    const char *p = get(KEY_AUTO_VALUE);
    if (p == 0)  return;
    parse_rtc_manually_time_data(p, now_av, now_tv, now_ev, now_scenemode, now_scenesubmode, ',');
}

void CameraParameters::dump() const
{
    ALOGD("dump: mMap.size = %zu", mMap.size());
    for (size_t i = 0; i < mMap.size(); i++) {
        String8 k, v;
        k = mMap.keyAt(i);
        v = mMap.valueAt(i);
        ALOGD("%s: %s\n", k.string(), v.string());
    }
}

status_t CameraParameters::dump(int fd, const Vector<String16>& /*args*/) const
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    snprintf(buffer, 255, "CameraParameters::dump: mMap.size = %zu\n", mMap.size());
    result.append(buffer);
    for (size_t i = 0; i < mMap.size(); i++) {
        String8 k, v;
        k = mMap.keyAt(i);
        v = mMap.valueAt(i);
        snprintf(buffer, 255, "\t%s: %s\n", k.string(), v.string());
        result.append(buffer);
    }
    write(fd, result.string(), result.size());
    return NO_ERROR;
}

void CameraParameters::getSupportedPreviewFormats(Vector<int>& formats) const {
    const char* supportedPreviewFormats =
          get(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS);

    if (supportedPreviewFormats == NULL) {
        ALOGW("%s: No supported preview formats.", __FUNCTION__);
        return;
    }

    String8 fmtStr(supportedPreviewFormats);
    char* prevFmts = fmtStr.lockBuffer(fmtStr.size());

    char* savePtr;
    char* fmt = strtok_r(prevFmts, ",", &savePtr);
    while (fmt) {
        int actual = previewFormatToEnum(fmt);
        if (actual != -1) {
            formats.add(actual);
        }
        fmt = strtok_r(NULL, ",", &savePtr);
    }
    fmtStr.unlockBuffer(fmtStr.size());
}


int CameraParameters::previewFormatToEnum(const char* format) {
    return
        !format ?
            HAL_PIXEL_FORMAT_YCrCb_420_SP :
        !strcmp(format, PIXEL_FORMAT_YUV422SP) ?
            HAL_PIXEL_FORMAT_YCbCr_422_SP : // NV16
        !strcmp(format, PIXEL_FORMAT_YUV420SP) ?
            HAL_PIXEL_FORMAT_YCrCb_420_SP : // NV21
        !strcmp(format, PIXEL_FORMAT_YUV422I) ?
            HAL_PIXEL_FORMAT_YCbCr_422_I :  // YUY2
        !strcmp(format, PIXEL_FORMAT_YUV420P) ?
            HAL_PIXEL_FORMAT_YV12 :         // YV12
        !strcmp(format, PIXEL_FORMAT_RGB565) ?
            HAL_PIXEL_FORMAT_RGB_565 :      // RGB565
        !strcmp(format, PIXEL_FORMAT_RGBA8888) ?
            HAL_PIXEL_FORMAT_RGBA_8888 :    // RGB8888
        !strcmp(format, PIXEL_FORMAT_BAYER_RGGB) ?
            HAL_PIXEL_FORMAT_RAW16 :   // Raw sensor data
        -1;
}

bool CameraParameters::isEmpty() const {
    return mMap.isEmpty();
}

}; // namespace android
