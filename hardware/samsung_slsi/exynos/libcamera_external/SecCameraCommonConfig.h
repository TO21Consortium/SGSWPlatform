#ifndef EXYNOS_CAMERA_COMMON_CONFIG_H__
#define EXYNOS_CAMERA_COMMON_CONFIG_H__

#include <math.h>

#include <cutils/log.h>
#include "ISecCameraHardware.h"


#define BUILD_DATE()   ALOGE("Build Date is (%s) (%s) ", __DATE__, __TIME__)
#define WHERE_AM_I()   ALOGE("[(%s)%d] ", __FUNCTION__, __LINE__)
#define LOG_DELAY()    usleep(100000)

#define TARGET_ANDROID_VER_MAJ 4
#define TARGET_ANDROID_VER_MIN 4

/* ---------------------------------------------------------- */
/* log */
#define XPaste(s) s
#define Paste2(a, b) XPaste(a)b
#define ID "[CAM_ID(%d)] - "
#define ID_PARM mCameraId

#define CLOGD(fmt, ...) \
    ALOGD(Paste2(ID, fmt), ID_PARM, ##__VA_ARGS__)

#define CLOGV(fmt, ...) \
    ALOGV(Paste2(ID, fmt), ID_PARM, ##__VA_ARGS__)

#define CLOGW(fmt, ...) \
    ALOGW(Paste2(ID, fmt), ID_PARM, ##__VA_ARGS__)

#define CLOGE(fmt, ...) \
    ALOGE(Paste2(ID, fmt), ID_PARM, ##__VA_ARGS__)

#define CLOGI(fmt, ...) \
    ALOGI(Paste2(ID, fmt), ID_PARM, ##__VA_ARGS__)

#define CLOGT(cnt, fmt, ...) \
    if (cnt != 0) CLOGI(Paste2("#TRACE#", fmt), ##__VA_ARGS__) \

#define CLOG_ASSERT(fmt, ...) \
    android_printAssert(NULL, LOG_TAG, Paste2(ID, fmt), ID_PARM, ##__VA_ARGS__);

/* ---------------------------------------------------------- */
/* Align */
#define ROUND_UP(x, a)              (((x) + ((a)-1)) / (a) * (a))
#define ROUND_OFF_HALF(x, dig)      ((float)(floor((x) * pow(10.0f, dig) + 0.5) / pow(10.0f, dig)))

/* ---------------------------------------------------------- */
/* Node Prefix */
#define NODE_PREFIX "/dev/video"

/* ---------------------------------------------------------- */
/* Max Camera Name Size */
#define EXYNOS_CAMERA_NAME_STR_SIZE (256)

/* ---------------------------------------------------------- */
/* Linux type */
#ifndef _LINUX_TYPES_H
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
/*typedef unsigned long long uint64_t;*/
#endif

/* ---------------------------------------------------------- */
/*  INCLUDE */
/* ---------------------------------------------------------- */

/* ---------------------------------------------------------- */
/*   SENSOR ENUM  */
/* ---------------------------------------------------------- */

typedef enum
{
	SENSOR_NAME_NOTHING		 = 0,
	SENSOR_NAME_S5K3H2		 = 1,
	SENSOR_NAME_S5K6A3		 = 2,
	SENSOR_NAME_S5K3H5		 = 3,
	SENSOR_NAME_S5K3H7		 = 4,
	SENSOR_NAME_S5K3H7_SUNNY	 = 5,
	SENSOR_NAME_S5K3H7_SUNNY_2M	 = 6,
	SENSOR_NAME_S5K6B2		 = 7,
	SENSOR_NAME_S5K3L2		 = 8,
	SENSOR_NAME_S5K4E5		 = 9,
	SENSOR_NAME_S5K2P2		 = 10,
	SENSOR_NAME_S5K8B1		 = 11,
	SENSOR_NAME_S5K1P2		 = 12,
	SENSOR_NAME_S5K4H5		 = 13,
	SENSOR_NAME_S5K3M2		 = 14,
	SENSOR_NAME_S5K2P2_12M		 = 15,
	SENSOR_NAME_S5K6D1		 = 16,
	SENSOR_NAME_S5K5E3		 = 17,
	SENSOR_NAME_S5K2T2		 = 18,
	SENSOR_NAME_S5K2P3		 = 19,
	SENSOR_NAME_S5K2P8		 = 20,
	SENSOR_NAME_S5K4E6		 = 21,
	SENSOR_NAME_S5K5E2		 = 22,
	SENSOR_NAME_S5K3P3		 = 23,
	SENSOR_NAME_S5K4H5YC		 = 24,
	SENSOR_NAME_S5K2X8		 = 28,
	SENSOR_NAME_S5K2L1		 = 29,
	SENSOR_NAME_S5K4EC		 = 57,

	SENSOR_NAME_IMX135		 = 101, /* 101 ~ 200 Sony sensors */
	SENSOR_NAME_IMX134		 = 102,
	SENSOR_NAME_IMX175		 = 103,
	SENSOR_NAME_IMX240		 = 104,
	SENSOR_NAME_IMX220		 = 105,
	SENSOR_NAME_IMX228		 = 106,
	SENSOR_NAME_IMX219		 = 107,
	SENSOR_NAME_IMX230		 = 108,
	SENSOR_NAME_IMX260		 = 109,

	SENSOR_NAME_SR261		 = 201, /* 201 ~ 300 Other vendor sensors */
	SENSOR_NAME_OV5693		 = 202,
	SENSOR_NAME_SR544		 = 203,
	SENSOR_NAME_OV5670		 = 204,
	SENSOR_NAME_DSIM		 = 205,
	SENSOR_NAME_VIRTUAL		 = 206,

	SENSOR_NAME_CUSTOM		 = 301,
	SENSOR_NAME_SR200		 = 302, // SoC Module
	SENSOR_NAME_SR352		 = 303,
	SENSOR_NAME_SR130PC20	 = 304,
	SENSOR_NAME_S5K5E6		 = 305,
	SENSOR_NAME_VIRTUAL_ZEBU = 901,
	SENSOR_NAME_END,
}IS_SensorNameEnum;

enum CAMERA_ID {
    CAMERA_ID_BACK  = 0,
    CAMERA_ID_FRONT = 1,
    CAMERA_ID_MAX,
};

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
    PIPE_SCP,
    PIPE_3AA_ISP,
    PIPE_POST_3AA_ISP,
    PIPE_DIS,
    PIPE_SCC,
    PIPE_GSC,
    PIPE_GSC_VIDEO,
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
    PIPE_SCC_REPROCESSING,
    PIPE_SCP_REPROCESSING,
    PIPE_GSC_REPROCESSING,
    PIPE_JPEG_REPROCESSING,
    MAX_PIPE_NUM_REPROCESSING
};


/* ---------------------------------------------------------- */
/* From Parameter Header */
namespace CONFIG_MODE {
    enum MODE {
        NORMAL        = 0x00,
        HIGHSPEED_60,
        HIGHSPEED_120,
        HIGHSPEED_240,
        MAX
    };
};

/* camera errors */
enum {
    SEC_CAMERA_ERROR_PREVIEWFRAME_TIMEOUT = 1001,
    SEC_CAMERA_ERROR_DATALINE_FAIL = 2000
};

struct CONFIG_PIPE {
    uint32_t prepare[MAX_PIPE_NUM_REPROCESSING];
};

struct CONFIG_BUFFER {
    uint32_t num_bayer_buffers;
    uint32_t init_bayer_buffers;
    uint32_t num_3aa_buffers;
    uint32_t num_hwdis_buffers;
    uint32_t num_preview_buffers;
    uint32_t num_preview_cb_buffers;
    uint32_t num_picture_buffers;
    uint32_t num_reprocessing_buffers;
    uint32_t num_recording_buffers;
    uint32_t num_fastaestable_buffer;
    uint32_t reprocessing_bayer_hold_count;
    uint32_t front_num_bayer_buffers;
    uint32_t front_num_picture_buffers;
    uint32_t preview_buffer_margin;
#ifdef USE_CAMERA2_API_SUPPORT
    uint32_t num_min_block_request;
    uint32_t num_max_block_request;
#endif
};

struct CONFIG_BUFFER_PIPE {
    struct CONFIG_PIPE pipeInfo;
    struct CONFIG_BUFFER bufInfo;
};

struct ExynosConfigInfo {
    struct CONFIG_BUFFER_PIPE *current;
    struct CONFIG_BUFFER_PIPE info[CONFIG_MODE::MAX];
    uint32_t mode;
};

/* ---------------------------------------------------------- */
/* Activity Controller */
enum auto_focus_type {
    AUTO_FOCUS_SERVICE      = 0,
    AUTO_FOCUS_HAL,
};

#ifdef SENSOR_NAME_GET_FROM_FILE
#define SENSOR_NAME_PATH_BACK "/sys/class/camera/rear/rear_sensorid"
#define SENSOR_NAME_PATH_FRONT "/sys/class/camera/front/front_sensorid"
#endif

#ifdef SENSOR_FW_GET_FROM_FILE
#define SENSOR_FW_PATH_BACK "/sys/class/camera/rear/rear_camfw"
#define SENSOR_FW_PATH_FRONT "/sys/class/camera/front/front_camfw"
#endif

#if defined(SUPPORT_X8_ZOOM)
#define MAX_ZOOM_LEVEL ZOOM_LEVEL_X8_MAX
#define MAX_ZOOM_RATIO (8000)
#define MAX_ZOOM_LEVEL_FRONT ZOOM_LEVEL_MAX
#define MAX_ZOOM_RATIO_FRONT (4000)
#define MAX_BASIC_ZOOM_LEVEL ZOOM_LEVEL_X8_MAX  /* CTS and 3rd-Party */
#elif defined(SUPPORT_X8_ZOOM_AND_800STEP)
#define MAX_ZOOM_LEVEL ZOOM_LEVEL_X8_800STEP_MAX
#define MAX_ZOOM_RATIO (8000)
#define MAX_ZOOM_LEVEL_FRONT ZOOM_LEVEL_MAX
#define MAX_ZOOM_RATIO_FRONT (4000)
#define MAX_BASIC_ZOOM_LEVEL ZOOM_LEVEL_X8_MAX  /* CTS and 3rd-Party */
#elif defined(SUPPORT_X4_ZOOM_AND_400STEP)
#define MAX_ZOOM_LEVEL ZOOM_LEVEL_X4_400STEP_MAX
#define MAX_ZOOM_RATIO (4000)
#define MAX_ZOOM_LEVEL_FRONT ZOOM_LEVEL_MAX
#define MAX_ZOOM_RATIO_FRONT (4000)
#define MAX_BASIC_ZOOM_LEVEL ZOOM_LEVEL_MAX  /* CTS and 3rd-Party */
#else
#define MAX_ZOOM_LEVEL ZOOM_LEVEL_MAX
#define MAX_ZOOM_RATIO (4000)
#define MAX_ZOOM_LEVEL_FRONT ZOOM_LEVEL_MAX
#define MAX_ZOOM_RATIO_FRONT (4000)
#define MAX_BASIC_ZOOM_LEVEL ZOOM_LEVEL_MAX  /* CTS and 3rd-Party */
#endif
#endif /* EXYNOS_CAMERA_COMMON_CONFIG_H__ */

