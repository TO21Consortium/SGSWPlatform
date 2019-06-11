/*
 * Copyright 2015, Samsung Electronics Co. LTD
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
 * \file      ExynosCameraConfig.h
 * \brief     hearder file for ExynosCameraConfig
 * \author    Pilsun Jang(pilsun.jang@samsung.com)
 * \date      2013/7/15
 *
 */

#ifndef EXYNOS_CAMERA_CONFIG_H__
#define EXYNOS_CAMERA_CONFIG_H__

#include "ExynosCameraCommonEnum.h"
#include "ExynosCameraCommonDefine.h"

/* It will be replaced by TN */
#ifdef CAMERA_GED_FEATURE
#else
#include "SecCameraConfig-common.h"
#endif /* CAMERA_GED_FEATURE */

#define SMDK7870

#ifndef USE_VENDOR_SPECIFIC_CONFIG_HEADER

#define MONITOR_LOG_SYNC
#define MONITOR_LOG_SYNC_INTERVAL 100

#define USE_HAL3_2_METADATA_INTERFACE
#define SUPPORT_HAL3_3_METADATA

#define USE_NEW_NOISE_REDUCTION_ALGORITHM

/* for universal GED */
#if UNIVERSAL_CAMERA
/* #define SAMSUNG_COMPANION */
#endif

#ifdef SAMSUNG_COMPANION
#define MAIN_CAMERA_USE_SAMSUNG_COMPANION   (false)
#define FRONT_CAMERA_USE_SAMSUNG_COMPANION  (false)
#endif

#include <math.h>

/************************************************/
#ifdef CAMERA_GED_FEATURE
#else
#define SUPPORT_X8_ZOOM
#endif

#define USE_SUBDIVIDED_EV

/* #define DEBUG_RAWDUMP */
/************************************************/


#define EFFECT_VALUE_VERSION_2_0

#define LCD_SIZE_DEFAULT                (0)
#define LCD_SIZE_800_480                (1)
#define LCD_SIZE_1280_720               (2)
#define LCD_SIZE_1920_1080              (3)
#define LCD_SIZE_2560_1440              (4)
#define CAMERA_LCD_SIZE                 LCD_SIZE_1280_720

/* #define LIMIT_SCP_SIZE_UNTIL_FHD_ON_CAPTURE */   /* Even if LCD is bigger than FHD, limit scp size until FHD on capture preview  */
#define LIMIT_SCP_SIZE_UNTIL_FHD_ON_RECORDING /* Even if LCD is bigger than FHD, limit scp size until FHD on recording preview*/

#define CAMERA_BCROP_ALIGN              (16)
#define CAMERA_ISP_ALIGN                (8)
#define CAMERA_MCSC_ALIGN               (2)
#define CAMERA_MAGIC_ALIGN              (16)
#define CAMERA_16PX_ALIGN               (16)
/* This value for GSC alignment refer to "csc.h" */
#define GSCALER_IMG_ALIGN               (16)

#define SCALER_MAX_SCALE_UP_RATIO       (8)

#define INITIAL_SKIP_FRAME              (8)
#define VISION_SKIP_FRAME               (4)
#define EFFECT_SKIP_FRAME               (1)
#define SMART_STAY_SKIP_COMPENSATION    (1)
#define USE_GRALLOC_REUSE_SUPPORT
#define USE_GRALLOC_BUFFER_COLLECTOR

/* Functions tobe enable */
#define START_PICTURE_THREAD
/* #define SET_SETFILE_BY_SHOT */
#define USE_FRAMEMANAGER
#ifdef USE_FRAMEMANAGER
#define USE_FRAME_REFERENCE_COUNT
#else
#endif

#define SET_SETFILE_BY_SET_CTRL
#ifdef SET_SETFILE_BY_SET_CTRL
#define SET_SETFILE_BY_SET_CTRL_3AA_ISP (false)
#define SET_SETFILE_BY_SET_CTRL_3AA     (false)
#define SET_SETFILE_BY_SET_CTRL_ISP     (true)
#define SET_SETFILE_BY_SET_CTRL_SCP     (false)
#endif

#ifdef CAMERA_GED_FEATURE
#else
#define RESERVED_MEMORY_ENABLE
#endif

#define RESERVED_BUFFER_COUNT_MAX       (0)

/* #define USE_BNS_PREVIEW */ /* use BNS on capture scenario. this change LUT size */

/* #define USE_BNS_RECORDING */
/* #define USE_BDS_RECORDING */ /* use bds on recording scenario. this change LUT size */

/* #define USE_BNS_DUAL_PREVIEW */ /* use bds on recording scenario. this change LUT size */
/* #define USE_BNS_DUAL_RECORDING */ /* use bds on recording scenario. this change LUT size */

/* #define USE_UHD_RECORDING */
/* #define USE_WQHD_RECORDING */
/* #define FPS_CHECK */
#define FIRST_PREVIEW_TIME_CHECK /* to get time startPreview() ~ first preview frame q to hwc */
#define USE_FD_AE
#define FD_ROTATION                     (true)
#define HDR_DELAY                       (1)
/* #define USE_FRONT_PREVIEW_DRC */

#define CAMERA_PREVIEW_EXPOSURE_TIME_LIMIT (100000)
#define CAMERA_EXPOSURE_TIME_MAX           (500000)

#define BURST_CAPTURE                   (true)
#ifdef BURST_CAPTURE
enum {
    BURST_SAVE_PHONE = 0,
    BURST_SAVE_SDCARD,
    BURST_SAVE_CALLBACK,
};

#define DEFAULT_BURST_SAVE              (BURST_SAVE_CALLBACK)
#define BURST_SHUTTER_PREPICTURE        (0)
#define BURST_SHUTTER_PREPICTURE_DONE   (1)
#define BURST_SHUTTER_JPEGCB            (2)
#endif

#define SHOT_RECOVERY                   (true)
#define SHOT_RECOVERY_ONEBYONE          (true)
#ifdef SHOT_RECOVERY
#define SHOT_RECOVERY_COUNT             (0)
#else
#define SHOT_RECOVERY_COUNT             (0)
#endif

#define FRAME_SKIP_COUNT_RECORDING      (1)
#define FRAME_SKIP_COUNT_PREVIEW        (0)
#define FRAME_SKIP_COUNT_PREVIEW_FRONT  (3)

#ifdef CAMERA_GED_FEATURE
#define USE_ADAPTIVE_CSC_RECORDING      (true)
#else
#define USE_ADAPTIVE_CSC_RECORDING      (true)
#endif

#define SUPPORT_BACK_HW_VDIS            (false)
#define SUPPORT_FRONT_HW_VDIS           (false)

#define HW_VDIS_W_RATIO                 (1.2f)
#define HW_VDIS_H_RATIO                 (1.2f)

#ifdef CAMERA_GED_FEATURE
#define USE_HIGHSPEED_RECORDING         (false)
#else
#define USE_HIGHSPEED_RECORDING         (true)
#endif

#ifdef CAMERA_GED_FEATURE
#define USE_FASTEN_AE_STABLE            (false)
#else
#define USE_FASTEN_AE_STABLE            (true)
#endif

// fastenAE size will get by getFastenAeStableBcropSize()
/*
#define FASTEN_AE_WIDTH                 (1472)
#define FASTEN_AE_HEIGHT                (828)
*/
#define FASTEN_AE_FPS                   (120)
#define FASTEN_AE_FPS_FRONT             (112)
#define DEFAULT_BINNING_RATIO           (1)

#ifdef USE_BNS_PREVIEW
#define DEFAULT_BNS_RATIO               (1)
#else
#define DEFAULT_BNS_RATIO               (1)
#endif

#define USE_JPEG_HWFC                   (true)

#define USE_PURE_BAYER_REPROCESSING                    (false)
#define USE_PURE_BAYER_REPROCESSING_ON_RECORDING       (false)
#define USE_PURE_BAYER_REPROCESSING_ON_DUAL            (false)
#define USE_PURE_BAYER_REPROCESSING_ON_DUAL_RECORDING  (false)

#define USE_PURE_BAYER_REPROCESSING_FRONT              (false)
#define USE_PURE_BAYER_REPROCESSING_FRONT_ON_RECORDING (false)
#define USE_PURE_BAYER_REPROCESSING_FRONT_ON_DUAL      (false)
#define USE_PURE_BAYER_REPROCESSING_FRONT_ON_DUAL_RECORDING  (false)

#define USE_CAMERA_SIZE_TABLE           (true)

/* #define USE_VARIABLE_FPS_OF_FRONT_RECORDING */

/* This USE_DYNAMIC_BAYER define is for default scenario.
 * See <ExynosCameraParameter.cpp> for details of dynamic bayer setting
 */
#define USE_DYNAMIC_BAYER                       (false)
#define USE_DYNAMIC_BAYER_VIDEO_SNAP_SHOT       (false)

#define USE_DYNAMIC_BAYER_FRONT                 (false)
#define USE_DYNAMIC_BAYER_VIDEO_SNAP_SHOT_FRONT (false)

#define USE_YUV_REPROCESSING                    (true)
#define USE_YUV_REPROCESSING_FOR_THUMBNAIL      (true)

enum REPROCESSING_BAYER_MODE {
    REPROCESSING_BAYER_MODE_NONE            = 0, /* This means capture do not use reprocessing */
    REPROCESSING_BAYER_MODE_PURE_ALWAYS_ON,
    REPROCESSING_BAYER_MODE_DIRTY_ALWAYS_ON,
    REPROCESSING_BAYER_MODE_PURE_DYNAMIC,
    REPROCESSING_BAYER_MODE_DIRTY_DYNAMIC,
    REPROCESSING_BAYER_MODE_MAX,
};

#define SET_FPS_FRONTCAM                (1)

#define MAX_FACEDETECT_THREADQ_SIZE     (2)
#define MAX_FOCUSCONTINUS_THREADQ_SIZE  (2)

#define USE_DYNAMIC_SCC_REAR            (false)
#define USE_DYNAMIC_SCC_FRONT           (false)

#define USE_GSC_FOR_CAPTURE_BACK        (false)
#define USE_GSC_FOR_CAPTURE_FRONT       (false)

#define MAX_SERIES_SHOT_COUNT           (1000)

#define SENSOR_FW_PATH_BACK "/sys/class/camera/rear/rear_camfw"
#define SENSOR_FW_PATH_FRONT "/sys/class/camera/front/front_camfw"

#define TORCH_REAR_FILE_PATH "/sys/devices/14400000.fimc_is/torch/flash_torch_control"
#define TORCH_FRONT_FILE_PATH "/sys/devices/14400000.fimc_is/torch/flash_torch_control"

#define SUPPORT_64BITS

#ifndef DEBUG_RAWDUMP
#define CAMERA_PACKED_BAYER_ENABLE
#endif
/* #define USE_BUFFER_WITH_STRIDE */

#define OWN_MCSC_HW                 (true)

/* back */
#define MAIN_CAMERA_FLITE_NUM                       FIMC_IS_VIDEO_SS0_NUM
#define MAIN_CAMERA_HAS_OWN_SCC     (false)

#define MAIN_CAMERA_SINGLE_FLITE_3AA_OTF (true)
#define MAIN_CAMERA_DUAL_FLITE_3AA_OTF   (true)

#define MAIN_CAMERA_SINGLE_3AA_ISP_OTF (true)
#define MAIN_CAMERA_DUAL_3AA_ISP_OTF   (false)

#define MAIN_CAMERA_SINGLE_MCSC_VRA_OTF (false)
#define MAIN_CAMERA_DUAL_MCSC_VRA_OTF   (false)

#define MAIN_CAMERA_SINGLE_REPROCESSING  (true)
#define MAIN_CAMERA_SINGLE_SCC_CAPTURE   (false)

#define MAIN_CAMERA_DUAL_REPROCESSING    (false)
#define MAIN_CAMERA_DUAL_SCC_CAPTURE     (false)

#define MAIN_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING    (false)
#define MAIN_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING      (false) /* need to be same with FRONT_CAMERA_DUAL_3AA_ISP_OTF on dual case */

/* front */
#define FRONT_CAMERA_FLITE_NUM                       FIMC_IS_VIDEO_SS1_NUM
#define FRONT_CAMERA_HAS_OWN_SCC    (false)

#define FRONT_CAMERA_SINGLE_FLITE_3AA_OTF (true)
#define FRONT_CAMERA_DUAL_FLITE_3AA_OTF   (false)

#define FRONT_CAMERA_SINGLE_3AA_ISP_OTF (true)
#define FRONT_CAMERA_DUAL_3AA_ISP_OTF   (true)

#define FRONT_CAMERA_SINGLE_MCSC_VRA_OTF (false)
#define FRONT_CAMERA_DUAL_MCSC_VRA_OTF   (false)

#define FRONT_CAMERA_SINGLE_REPROCESSING  (true)
#define FRONT_CAMERA_SINGLE_SCC_CAPTURE   (false)

#define FRONT_CAMERA_DUAL_REPROCESSING    (false)
#define FRONT_CAMERA_DUAL_SCC_CAPTURE     (true)

#define FRONT_CAMERA_SINGLE_3AA_ISP_OTF_REPROCESSING    (false)
#define FRONT_CAMERA_DUAL_3AA_ISP_OTF_REPROCESSING      (false)

#define USE_3AC_FOR_ISPC            (true)

#define CAMERA_HAS_OWN_BDS          (false)

#define SCENARIO_SHIFT              (28)
#define REPROCESSING_SHIFT          (28)
#define OTF_3AA_SHIFT               (24)
#define SSX_VINDEX_SHIFT            (16)
#define TAX_VINDEX_SHIFT            (8)

#define SENSOR_NOBLOCK_SHIFT        (28)
#define SENSOR_INSTANT_SHIFT        (16)
#define SENSOR_SSTREAM_SHIFT        (0)

#define INPUT_STREAM_MASK   0xFF000000 /* stream type : 1 (reprocessing) */
#define INPUT_STREAM_SHIFT   24
#define INPUT_MODULE_MASK   0x00FF0000 /* module id : unique sensor id */
#define INPUT_MODULE_SHIFT   16
#define INPUT_VINDEX_MASK   0x0000FF00 /* video node index : connected node */
#define INPUT_VINDEX_SHIFT   8
#define INPUT_MEMORY_MASK   0x000000F0 /* memory interface : 0 (on-the-fly) */
#define INPUT_MEMORY_SHIFT   4
#define INPUT_LEADER_MASK   0x0000000F /* leader : 1 (leader video node) */
#define INPUT_LEADER_SHIFT   0

#define PREVIEW_GSC_NODE_NUM            (4)  /* 4 = MSC from Exynos5420 */
#define PICTURE_GSC_NODE_NUM            (4)  /* 0,1,2 = GSC */
#define VIDEO_GSC_NODE_NUM              (4)

#if (SUPPORT_BACK_HW_VDIS || SUPPORT_FRONT_HW_VDIS)
/*
 * HACK
 * When enable hw vdis, one more M2M case is happen(isp -> tpu).
 * we meet frame drop(2,3 per sec), because of memory lack.
 * so, just increase buffer to avoid frame drop.
 */
#define MAX_BUFFERS                     (32)
#define NUM_SENSOR_BUFFERS              (4 + REPROCESSING_BAYER_HOLD_COUNT)
#define NUM_BAYER_BUFFERS               (4 + REPROCESSING_BAYER_HOLD_COUNT + SHOT_RECOVERY_COUNT)
#define INIT_BAYER_BUFFERS              (4 + SHOT_RECOVERY_COUNT)
#define NUM_3AA_BUFFERS                 NUM_BAYER_BUFFERS + 3
#define NUM_HW_DIS_BUFFERS              (NUM_3AA_BUFFERS)
#define NUM_VRA_BUFFERS                 (2)
#define NUM_PREVIEW_BUFFERS             (9 + SHOT_RECOVERY_COUNT + 3)
#define NUM_PREVIEW_SPARE_BUFFERS       (3)
#define INIT_SCP_BUFFERS                (5)
#define NUM_PICTURE_BUFFERS             (5)
#define NUM_REPROCESSING_BUFFERS        (1)
#define NUM_RECORDING_BUFFERS           (8)
#define NUM_FASTAESTABLE_BUFFER         (10)
#define NUM_PREVIEW_BUFFERS_MARGIN      (1)
#else
#define MAX_BUFFERS                     (32)
#define NUM_SENSOR_BUFFERS              (4 + REPROCESSING_BAYER_HOLD_COUNT)
#define NUM_BAYER_BUFFERS               (4 + REPROCESSING_BAYER_HOLD_COUNT + SHOT_RECOVERY_COUNT)
#define INIT_BAYER_BUFFERS              (4 + SHOT_RECOVERY_COUNT)
#define NUM_3AA_BUFFERS                 NUM_BAYER_BUFFERS
#define NUM_HW_DIS_BUFFERS              (NUM_3AA_BUFFERS + 3)
#define NUM_VRA_BUFFERS                 (2)
#define NUM_PREVIEW_BUFFERS             (9 + SHOT_RECOVERY_COUNT + 3)
#define NUM_PREVIEW_SPARE_BUFFERS       (3)
#define INIT_SCP_BUFFERS                (5)
#define NUM_PICTURE_BUFFERS             (5)
#define NUM_REPROCESSING_BUFFERS        (1)
#define NUM_RECORDING_BUFFERS           (8)
#define NUM_FASTAESTABLE_BUFFER         (10)
#define NUM_PREVIEW_BUFFERS_MARGIN      (1)
#endif

#ifdef CAMERA_GED_FEATURE
#define RESERVED_NUM_BAYER_BUFFERS      (NUM_BAYER_BUFFERS)
#define RESERVED_NUM_ISP_BUFFERS        (0)
#else
#ifdef DEBUG_RAWDUMP
#define RESERVED_NUM_BAYER_BUFFERS      (0)
#define RESERVED_NUM_ISP_BUFFERS        (0)
#else
#define RESERVED_NUM_BAYER_BUFFERS      (5)
#define RESERVED_NUM_ISP_BUFFERS        (3)
#endif /* DEBUG_RAWDUMP */
#endif

#define PIPE_FLITE_PREPARE_COUNT            (3)
#define PIPE_3AC_PREPARE_COUNT              (3)
#define PIPE_3AA_ISP_PREPARE_COUNT          (3)
#define PIPE_ISP_PREPARE_COUNT              (0)
#define PIPE_SCC_PREPARE_COUNT              (1)
#define PIPE_SCP_PREPARE_COUNT              (3)

#define PIPE_FLITE_FRONT_PREPARE_COUNT      (3)
#define PIPE_SCC_FRONT_PREPARE_COUNT        (1)
#define PIPE_SCP_FRONT_PREPARE_COUNT        (3)
#define PIPE_SCP_REPROCESSING_PREPARE_COUNT (3)
#define PIPE_SCC_REPROCESSING_PREPARE_COUNT (1)

#define NUM_FRAME_PREPARE_COUNT         (6)
#define NUM_BURST_GSC_JPEG_INIT_BUFFER  (4) /* Number of pre-allicated buffer for burst shot
                                               Increasing this number will increase takePicture()'s
                                               response time. Currently it is defined as
                                               (# of JPEG save thread) + 1 */

/* TO DO : will remove */
#define REPROCESSING_BAYER_HOLD_COUNT   (1)
#define FRONT_NUM_SENSOR_BUFFERS        (NUM_SENSOR_BUFFERS)
#define FRONT_NUM_BAYER_BUFFERS         (NUM_BAYER_BUFFERS)
#define FRONT_NUM_3AA_BUFFERS           (NUM_3AA_BUFFERS)
#define FRONT_NUM_PICTURE_BUFFERS       (NUM_PICTURE_BUFFERS)

#ifdef DEBUG_RAWDUMP
#define FRONT_RESERVED_NUM_BAYER_BUFFERS        (0)
#define FRONT_RESERVED_NUM_ISP_BUFFERS          (0)
#else
#define FRONT_RESERVED_NUM_BAYER_BUFFERS        (6)
#define FRONT_RESERVED_NUM_ISP_BUFFERS          (0)
#endif /* DEBUG_RAWDUMP */

#define MFC_7X_BUFFER_OFFSET            (256)

#if (USE_HIGHSPEED_RECORDING)
#define FPS60_NUM_NUM_SENSOR_BUFFERS         (NUM_SENSOR_BUFFERS + 5)
#define FPS60_NUM_NUM_BAYER_BUFFERS          (NUM_BAYER_BUFFERS + 5)
#define FPS60_INIT_BAYER_BUFFERS             (INIT_BAYER_BUFFERS * 2)
#define FPS60_NUM_HW_DIS_BUFFERS             (NUM_HW_DIS_BUFFERS + 5)
#define FPS60_NUM_VRA_BUFFERS                (2)
#define FPS60_NUM_PREVIEW_BUFFERS            (NUM_PREVIEW_BUFFERS + 7)
#define FPS60_NUM_PICTURE_BUFFERS            (NUM_PICTURE_BUFFERS)
#define FPS60_NUM_REPROCESSING_BUFFERS       (NUM_REPROCESSING_BUFFERS)
#define FPS60_NUM_RECORDING_BUFFERS          (NUM_RECORDING_BUFFERS + 5)
#define FPS60_INITIAL_SKIP_FRAME             (INITIAL_SKIP_FRAME)
#define FPS60_REPROCESSING_BAYER_HOLD_COUNT  (REPROCESSING_BAYER_HOLD_COUNT)
#define FPS60_FRONT_NUM_SENSOR_BUFFERS       (FRONT_NUM_SENSOR_BUFFERS)
#define FPS60_FRONT_NUM_BAYER_BUFFERS        (FRONT_NUM_BAYER_BUFFERS)
#define FPS60_FRONT_NUM_PICTURE_BUFFERS      (FRONT_NUM_PICTURE_BUFFERS)
#define FPS60_NUM_PREVIEW_BUFFERS_MARGIN     (NUM_PREVIEW_BUFFERS_MARGIN + 1)

#define FPS60_PIPE_FLITE_PREPARE_COUNT       (FPS60_INIT_BAYER_BUFFERS)
#define FPS60_PIPE_3AC_PREPARE_COUNT         (FPS60_INIT_BAYER_BUFFERS)
#define FPS60_PIPE_3AA_ISP_PREPARE_COUNT     (FPS60_INIT_BAYER_BUFFERS)
#define FPS60_PIPE_SCP_PREPARE_COUNT         (FPS60_INIT_BAYER_BUFFERS)
#define FPS60_PIPE_SCP_REPROCESSING_PREPARE_COUNT (3)

#define FPS120_NUM_NUM_SENSOR_BUFFERS        (NUM_SENSOR_BUFFERS * 3)
#define FPS120_NUM_NUM_BAYER_BUFFERS         (NUM_BAYER_BUFFERS * 3)
#define FPS120_INIT_BAYER_BUFFERS            (INIT_BAYER_BUFFERS * 3)
#define FPS120_NUM_HW_DIS_BUFFERS            (NUM_HW_DIS_BUFFERS * 3)
#define FPS120_NUM_VRA_BUFFERS               (2)
#define FPS120_NUM_PREVIEW_BUFFERS           (NUM_PREVIEW_BUFFERS * 3)
#define FPS120_NUM_PICTURE_BUFFERS           (NUM_PICTURE_BUFFERS)
#define FPS120_NUM_REPROCESSING_BUFFERS      (NUM_REPROCESSING_BUFFERS)
#define FPS120_NUM_RECORDING_BUFFERS         (NUM_RECORDING_BUFFERS * 3)
#define FPS120_INITIAL_SKIP_FRAME            (INITIAL_SKIP_FRAME)
#define FPS120_REPROCESSING_BAYER_HOLD_COUNT (REPROCESSING_BAYER_HOLD_COUNT)
#define FPS120_FRONT_NUM_SENSOR_BUFFERS      (FRONT_NUM_SENSOR_BUFFERS)
#define FPS120_FRONT_NUM_BAYER_BUFFERS       (FRONT_NUM_BAYER_BUFFERS)
#define FPS120_FRONT_NUM_PICTURE_BUFFERS     (FRONT_NUM_PICTURE_BUFFERS)
#define FPS120_NUM_PREVIEW_BUFFERS_MARGIN    (NUM_PREVIEW_BUFFERS_MARGIN + 1)

#define FPS120_PIPE_FLITE_PREPARE_COUNT       (FPS120_INIT_BAYER_BUFFERS)
#define FPS120_PIPE_3AC_PREPARE_COUNT         (FPS120_INIT_BAYER_BUFFERS)
#define FPS120_PIPE_3AA_ISP_PREPARE_COUNT     (FPS120_INIT_BAYER_BUFFERS)
#define FPS120_PIPE_SCP_PREPARE_COUNT         (FPS120_INIT_BAYER_BUFFERS)
#define FPS120_PIPE_SCP_REPROCESSING_PREPARE_COUNT (3)
#endif

#define V4L2_CAMERA_MEMORY_TYPE         (V4L2_MEMORY_DMABUF) /* (V4L2_MEMORY_USERPTR) */
#define SCC_OUTPUT_COLOR_FMT            (V4L2_PIX_FMT_YUYV)
#define JPEG_INPUT_COLOR_FMT            (SCC_OUTPUT_COLOR_FMT)
#define CAMERA_BAYER_FORMAT             (V4L2_PIX_FMT_SBGGR12)
#define CAMERA_DUMP_BAYER_FORMAT        (V4L2_PIX_FMT_SBGGR16)

#define ERROR_POLLING_DETECTED          (-1001)
#define ERROR_DQ_BLOCKED_DETECTED       (-1002)
#define ERROR_DQ_BLOCKED_COUNT          (20)
#define WARNING_3AA_THREAD_INTERVAL     (100000)
#define WARNING_SCP_THREAD_INTERVAL     (100000)
#define MONITOR_THREAD_INTERVAL         (200000)

#define FRAME_MIN_NUM (3)

#define EXYNOS_CAMERA_BUFFER_MAX_PLANES (5)     /* img buffer 4 + metadata 1 */
#define EXYNOS_CAMERA_META_PLANE_SIZE   (32 * 1024)
#define GRALLOC_LOCK_FOR_CAMERA         (GRALLOC_SET_USAGE_FOR_CAMERA)

#define EXYNOS_CAMERA_PREVIEW_FPS_REFERENCE  (60)

#define  NUM_OF_DETECTED_FACES           (16)
#define  NUM_OF_DETECTED_FACES_THRESHOLD (0)

#define PERFRAME_NODE_GROUP_MAX          (5)
#define PERFRAME_NODE_GROUP_0            (0)
#define PERFRAME_NODE_GROUP_1            (1)
#define PERFRAME_NODE_GROUP_2            (2)
#define PERFRAME_NODE_GROUP_3            (3)
#define PERFRAME_NODE_GROUP_4            (4)

#define PERFRAME_INFO_INDEX_MAX          (5)
#define PERFRAME_INFO_INDEX_0            (0)
#define PERFRAME_INFO_INDEX_1            (1)
#define PERFRAME_INFO_INDEX_2            (2)
#define PERFRAME_INFO_INDEX_3            (3)
#define PERFRAME_INFO_INDEX_4            (4)

#define PERFRAME_CONTROL_PIPE                   PIPE_3AA
#define PERFRAME_CONTROL_REPROCESSING_PIPE      PIPE_3AA_REPROCESSING
#define PERFRAME_CONTROL_NODE_3AA
/* #define PERFRAME_CONTROL_NODE_ISP */
#define PERFRAME_INFO_3AA                       PERFRAME_INFO_INDEX_0
#define PERFRAME_INFO_ISP                       PERFRAME_INFO_INDEX_1
#define PERFRAME_INFO_DIS                       PERFRAME_INFO_INDEX_2
#define PERFRAME_INFO_MCSC                      PERFRAME_INFO_INDEX_3
#define PERFRAME_INFO_VRA                       PERFRAME_INFO_INDEX_4

#define PERFRAME_INFO_PURE_REPROCESSING_3AA     PERFRAME_INFO_INDEX_0
#define PERFRAME_INFO_PURE_REPROCESSING_ISP     PERFRAME_INFO_INDEX_1
#define PERFRAME_INFO_PURE_REPROCESSING_MCSC    PERFRAME_INFO_INDEX_2

#define PERFRAME_INFO_DIRTY_REPROCESSING_ISP    PERFRAME_INFO_INDEX_0
#define PERFRAME_INFO_DIRTY_REPROCESSING_MCSC   PERFRAME_INFO_INDEX_1

#define PERFRAME_INFO_YUV_REPROCESSING_MCSC     PERFRAME_INFO_INDEX_0

#define PERFRAME_3AA_POS                        PER_FRAME_GROUP_0
#define PERFRAME_ISP_POS                        PER_FRAME_GROUP_1

#define PERFRAME_BACK_3AC_POS           (0)
#define PERFRAME_BACK_3AP_POS           (1)
#define PERFRAME_BACK_ISPC_POS          (0)
#define PERFRAME_BACK_ISPP_POS          (2)
#define PERFRAME_BACK_SCC_POS           (3)
#define PERFRAME_BACK_SCP_POS           (3)
#define PERFRAME_BACK_MCSC0_POS         (0)
#define PERFRAME_BACK_MCSC1_POS         (0)
#define PERFRAME_BACK_MCSC2_POS         (0)

/*
#define PERFRAME_FRONT_3AC_POS          (1)
#define PERFRAME_FRONT_3AP_POS          (0)
#define PERFRAME_FRONT_SCC_POS          (0)
#define PERFRAME_FRONT_SCP_POS          (1)
*/
#define PERFRAME_FRONT_3AC_POS          (PERFRAME_BACK_3AC_POS)
#define PERFRAME_FRONT_3AP_POS          (PERFRAME_BACK_3AP_POS)
#define PERFRAME_FRONT_ISPC_POS         (PERFRAME_BACK_ISPC_POS)
#define PERFRAME_FRONT_ISPP_POS         (PERFRAME_BACK_ISPP_POS)
#define PERFRAME_FRONT_SCC_POS          (PERFRAME_BACK_SCC_POS)
#define PERFRAME_FRONT_SCP_POS          (PERFRAME_BACK_SCP_POS)

#define PERFRAME_REPROCESSING_3AP_POS   (0)
#define PERFRAME_REPROCESSING_3AC_POS   (1)
#define PERFRAME_REPROCESSING_SCC_POS   (2) /* be care of source code, when you change. */
#define PERFRAME_REPROCESSING_ISPC_POS    (0)
#define PERFRAME_REPROCESSING_ISPP_POS    (0)
#define PERFRAME_REPROCESSING_MCSC0_POS   (PERFRAME_REPROCESSING_SCC_POS)
#define PERFRAME_REPROCESSING_MCSC2_POS   (0)
#define PERFRAME_REPROCESSING_MCSC3_POS   (0)
#define PERFRAME_REPROCESSING_MCSC4_POS   (0)

#define WAITING_TIME                     (5000)   /* 5msec */
#define TOTAL_WAITING_TIME               (3000 * 1000)  /* 3000msec */
#define TOTAL_WAITING_COUNT              (3)
#define TOTAL_FLASH_WATING_COUNT         (10)
#define CAPTURE_WAITING_COUNT            (15)

#define DM_WAITING_TIME                  (30 * 1000) /* 30msec */
#define DM_WAITING_COUNT                 (10)

/*vision */
/* #define VISION_DUMP */
#define VISION_WIDTH                     (320)
#define VISION_HEIGHT                    (180)

/* callback state */
#define CALLBACK_STATE_PREVIEW_META     (1)
#define CALLBACK_STATE_PREVIEW_FRAME    (2)
#define CALLBACK_STATE_RAW_IMAGE        (3)
#define CALLBACK_STATE_POSTVIEW_FRAME   (4)
#define CALLBACK_STATE_COMPRESSED_IMAGE (5)

#define MAGIC_SHOT_DURATION         125000  /* 8fps */
#define BEST_PHOTO_DURATION         195000  /* about 5fps */
#define BEST_FACE_DURATION          400000  /* 2.5fps */
#define ERASER_DURATION             800000  /* 1.25fps */
#define NORMAL_BURST_DURATION       90000   /* about 11fps */
#define SELFIE_ALARM_DURATION       330000  /* about 3fps */

#define USE_MEM2MEM_GSC

#ifdef USE_MEM2MEM_GSC
#define GRALLOC_SET_USAGE_FOR_CAMERA \
    (GRALLOC_USAGE_SW_READ_OFTEN | \
     GRALLOC_USAGE_SW_WRITE_OFTEN | \
     GRALLOC_USAGE_HW_TEXTURE | \
     GRALLOC_USAGE_HW_COMPOSER | \
     GRALLOC_USAGE_EXTERNAL_DISP | \
     GRALLOC_USAGE_HW_CAMERA_MASK)
#else
#define GRALLOC_SET_USAGE_FOR_CAMERA \
    (GRALLOC_USAGE_SW_READ_OFTEN | \
     GRALLOC_USAGE_SW_WRITE_OFTEN | \
     GRALLOC_USAGE_HW_TEXTURE | \
     GRALLOC_USAGE_HW_COMPOSER | \
     GRALLOC_USAGE_EXTERNAL_DISP)
#endif

enum YUV_RANGE {
    YUV_FULL_RANGE = 0,
    YUV_LIMITED_RANGE = 1,
};

enum pipeline {
    PIPE_FLITE                  = 0,
    PIPE_3AA,
    PIPE_3AC,
    PIPE_3AP,
    PIPE_ISP,
    PIPE_ISPC,
    PIPE_ISPP,
    PIPE_DIS,
    PIPE_MCSC,
    PIPE_SCP,
    PIPE_MCSC0 = PIPE_SCP,
    PIPE_MCSC1,
    PIPE_MCSC2,
    PIPE_MCSC3,
    PIPE_MCSC4,
    PIPE_VRA,
    PIPE_3AA_ISP,
    PIPE_POST_3AA_ISP,
    PIPE_SCC,
    PIPE_GSC,
    PIPE_GSC_VIDEO,
    PIPE_GSC_VRA,
    PIPE_GSC_PICTURE,
    PIPE_JPEG,
    MAX_PIPE_NUM,

    /*
     * PIPE_XXX_FRONT are deprecated define.
     * Don't use this. (just let for common code compile)
     */
    PIPE_FLITE_FRONT = 100,
    PIPE_3AA_FRONT,
    PIPE_3AC_FRONT,
    PIPE_3AP_FRONT,
    PIPE_ISP_FRONT,
    PIPE_ISPC_FRONT,
    PIPE_ISPP_FRONT,
    PIPE_SCP_FRONT,
    PIPE_3AA_ISP_FRONT,
    PIPE_POST_3AA_ISP_FRONT,
    PIPE_DIS_FRONT,
    PIPE_SCC_FRONT,
    PIPE_GSC_FRONT,
    PIPE_GSC_VIDEO_FRONT,
    PIPE_GSC_PICTURE_FRONT,
    PIPE_JPEG_FRONT,
    MAX_PIPE_NUM_FRONT,

    PIPE_FLITE_REPROCESSING     = 200,
    PIPE_3AA_REPROCESSING,
    PIPE_3AC_REPROCESSING,
    PIPE_3AP_REPROCESSING,
    PIPE_ISP_REPROCESSING,
    PIPE_ISPC_REPROCESSING,
    PIPE_ISPP_REPROCESSING,
    PIPE_MCSC_REPROCESSING,
    PIPE_MCSC0_REPROCESSING,
    PIPE_MCSC1_REPROCESSING,
    PIPE_MCSC2_REPROCESSING,
    PIPE_MCSC3_REPROCESSING,
    PIPE_MCSC4_REPROCESSING,
    PIPE_SCC_REPROCESSING,
    PIPE_SCP_REPROCESSING,
    PIPE_GSC_REPROCESSING,
    PIPE_GSC_REPROCESSING2,
    PIPE_GSC_REPROCESSING3,
#ifdef UVS
    PIPE_UVS_REPROCESSING,
#endif
    PIPE_JPEG_REPROCESSING,
    PIPE_HWFC_JPEG_SRC_REPROCESSING,
    PIPE_HWFC_THUMB_SRC_REPROCESSING,
    PIPE_HWFC_JPEG_DST_REPROCESSING,
    PIPE_HWFC_THUMB_DST_REPROCESSING,
    MAX_PIPE_NUM_REPROCESSING
};

enum fimc_is_video_dev_num {
    FIMC_IS_VIDEO_BAS_NUM = 100,
    FIMC_IS_VIDEO_SS0_NUM = 101,
    FIMC_IS_VIDEO_SS1_NUM,
    FIMC_IS_VIDEO_SS2_NUM,
    FIMC_IS_VIDEO_SS3_NUM,
    FIMC_IS_VIDEO_30S_NUM = 110,
    FIMC_IS_VIDEO_30C_NUM,
    FIMC_IS_VIDEO_30P_NUM,
    FIMC_IS_VIDEO_31S_NUM = 120,
    FIMC_IS_VIDEO_31C_NUM,
    FIMC_IS_VIDEO_31P_NUM,
    FIMC_IS_VIDEO_I0S_NUM = 130,
    FIMC_IS_VIDEO_I0C_NUM,
    FIMC_IS_VIDEO_I0P_NUM,
    FIMC_IS_VIDEO_I1S_NUM = 140,
    FIMC_IS_VIDEO_I1C_NUM,
    FIMC_IS_VIDEO_I1P_NUM,
    FIMC_IS_VIDEO_TPU_NUM = 150,
    FIMC_IS_VIDEO_SCC_NUM,
    FIMC_IS_VIDEO_SCP_NUM,
    FIMC_IS_VIDEO_M0S_NUM = 160,
    FIMC_IS_VIDEO_M1S_NUM,
    FIMC_IS_VIDEO_M0P_NUM = 170,
    FIMC_IS_VIDEO_M1P_NUM,
    FIMC_IS_VIDEO_M2P_NUM,
    FIMC_IS_VIDEO_M3P_NUM,
    FIMC_IS_VIDEO_M4P_NUM,
    FIMC_IS_VIDEO_VRA_NUM = 180,
    FIMC_IS_VIDEO_HWFC_JPEG_NUM = 200,
    FIMC_IS_VIDEO_HWFC_THUMB_NUM = 201,
    FIMC_IS_VIDEO_MAX_NUM
};

#define MAIN_CAMERA_COMPANION_NUM   (109)
#define FRONT_CAMERA_COMPANION_NUM  (109)

enum frame_generation_mode_enum {
    EXYNOS_GENERATE_SKIP_FRAME = 0,
    EXYNOS_GENERATE_FAST_AE_STABLE_FRAME = 1,
    EXYNOS_GENERATE_DYNAMIC_BAYER_FRAME = 2,
    EXYNOS_GENERATE_NORMAL_FRAME = 3,
    EXYNOS_GENERATE_FRAME_MAX
};

/* This struct used in recording callback */
/* This struct passes to OMX */
struct addrs {
    uint32_t        type;       /* make sure that this is 4 byte. */
    unsigned int    fdPlaneY;
    unsigned int    fdPlaneCbcr;
    unsigned int    bufIndex;
    unsigned int    reserved;
};

/* overflow handling */
#define SENSOR_OVERFLOW_CHECK

/* for test */
/* #define SCALABLE_ON */
/* #define TEST_GED_HIGH_SPEED_RECORDING */
/* #define TEST_APP_HIGH_SPEED_RECORDING */ /* for ArtCamera */
/* #define FORCE_RESET_MULTI_FRAME_FACTORY */
#endif /*USE_VENDOR_SPECIFIC_CONFIG_HEADER*/

#endif /* EXYNOS_CAMERA_CONFIG_H__ */
