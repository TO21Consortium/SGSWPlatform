#ifndef EXYNOS_CAMERA_COMMON_CONFIG_H__
#define EXYNOS_CAMERA_COMMON_CONFIG_H__

#include <math.h>

#include <cutils/log.h>

#define BUILD_DATE()   ALOGE("Build Date is (%s) (%s) ", __DATE__, __TIME__)
#define WHERE_AM_I()   ALOGE("[(%s)%d] ", __FUNCTION__, __LINE__)
#define LOG_DELAY()    usleep(100000)

#define TARGET_ANDROID_VER_MAJ 4
#define TARGET_ANDROID_VER_MIN 4

/* ---------------------------------------------------------- */
/* log */
#define XPaste(s) s
#define Paste2(a, b) XPaste(a)b
#define ID "[CAM_ID(%d)][%s]-"
#define ID_PARM m_cameraId, m_name
#define LOCATION_ID         "(%s[%d]):"
#define LOCATION_ID_PARM    __FUNCTION__, __LINE__

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

#define CLOGD2(fmt, ...) \
        CLOGD(Paste2("DEBUG" LOCATION_ID, fmt), LOCATION_ID_PARM, ##__VA_ARGS__)

#define CLOGV2(fmt, ...) \
        CLOGV(Paste2("VERB" LOCATION_ID, fmt), LOCATION_ID_PARM, ##__VA_ARGS__)

#define CLOGW2(fmt, ...) \
        CLOGW(Paste2("WARN" LOCATION_ID, fmt), LOCATION_ID_PARM, ##__VA_ARGS__)

#define CLOGE2(fmt, ...) \
        CLOGE(Paste2("ERR" LOCATION_ID, fmt), LOCATION_ID_PARM, ##__VA_ARGS__)

#define CLOGI2(fmt, ...) \
        CLOGI(Paste2("INFO" LOCATION_ID, fmt), LOCATION_ID_PARM, ##__VA_ARGS__)

/* ---------------------------------------------------------- */
/* Debug Timer */
#define DEBUG_TIMER_INIT \
    ExynosCameraDurationTimer debugPPPTimer;
#define DEBUG_TIMER_START \
    debugPPPTimer.start();
#define DEBUG_TIMER_STOP \
    debugPPPTimer.stop(); CLOGD("DEBUG(%s[%d]): DurationTimer #0 (%lld usec)", __FUNCTION__, __LINE__, debugPPPTimer.durationUsecs());

/* ---------------------------------------------------------- */
/* Align */
#define ROUND_UP(x, a)              (((x) + ((a)-1)) / (a) * (a))
#define ROUND_OFF_HALF(x, dig)      ((float)(floor((x) * pow(10.0f, dig) + 0.5) / pow(10.0f, dig)))
#define ROUND_OFF_DIGIT(x, dig)     ((uint32_t)(floor(((double)x)/((double)dig) + 0.5f) * dig))

/* Image processing */
#define SATURATING_ADD(a, b)  (((a) > (0x3FF - (b))) ? 0x3FF : ((a) + (b)))
#define COMBINE(a, b) ((((a<<20)&0xFFF00000)|((b<<8)&0x000FFF00)))
#define COMBINE_P0(a, b) ((((a)&0x00FF)|((b<<8)&0x0F00)))
#define COMBINE_P1(a, b) ((((a>>4)&0x000F)|((b<<4)&0x0FF0)))
#define COMBINE_P3(a, b) ((((a>>8)&0x000F)|((b<<4)&0x00F0)))
#define COMBINE_P4(a, b) ((((a<<8)&0x0F00)|((b)&0x00FF)))

/* ---------------------------------------------------------- */
/* Node Prefix */
#define NODE_PREFIX "/dev/video"

/* ---------------------------------------------------------- */
/* Max Camera Name Size */
#define EXYNOS_CAMERA_NAME_STR_SIZE (256)

/* ---------------------------------------------------------- */
#define SIZE_RATIO(w, h)         ((w) * 10 / (h))

/* ---------------------------------------------------------- */

#endif /* EXYNOS_CAMERA_COMMON_CONFIG_H__ */

