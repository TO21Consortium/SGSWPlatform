/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ANDROID_EXYNOS_HWC_H_
#define ANDROID_EXYNOS_HWC_H_
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>


#include <decon-fb.h>


//#include <s3c-fb.h>


#include <EGL/egl.h>

#define HWC_REMOVE_DEPRECATED_VERSIONS 1

#include <cutils/compiler.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <hardware_legacy/uevent.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/Timers.h>

#include <sync/sync.h>

#include "gralloc_priv.h"
#include "exynos_format.h"
#include "exynos_v4l2.h"
#include "ExynosHWCModule.h"
#include "s5p_tvout_v4l2.h"
#include "ExynosRect.h"
#include "videodev2.h"


#ifdef __GNUC__
#define __UNUSED__ __attribute__((__unused__))
#else
#define __UNUSED__
#endif

#


#ifdef NUM_AVAILABLE_HW_WINDOWS
/*
 * NUM_AVAILABLE_HW_WINDOWS can be optionally provided by
 * soc specific header file which is generally present at
 * $SoC\libhwcmodule\ExynosHWCModule.h. This is useful when
 * same display controller driver is used by SoCs having
 * different number of windows.
 * S3C_FB_MAX_WIN: max number of hardware windows supported
 * by the display controller driver.
 * NUM_AVAILABLE_HW_WINDOWS: max windows in the given SoC.
 */
const size_t NUM_HW_WINDOWS = NUM_AVAILABLE_HW_WINDOWS;
#else
const size_t NUM_HW_WINDOWS = S3C_FB_MAX_WIN;
#endif
const size_t NO_FB_NEEDED = NUM_HW_WINDOWS + 1;
#ifndef FIMD_BW_OVERLAP_CHECK
const size_t MAX_NUM_FIMD_DMA_CH = 2;
const int FIMD_DMA_CH_IDX[NUM_HW_WINDOWS] = {0, 1, 1, 1, 0};
#endif

#define MAX_DEV_NAME 128
#ifndef VSYNC_DEV_PREFIX
#define VSYNC_DEV_PREFIX ""
#endif
#ifndef VSYNC_DEV_MIDDLE
#define VSYNC_DEV_MIDDLE ""
#endif

#ifdef TRY_SECOND_VSYNC_DEV
#ifndef VSYNC_DEV_NAME2
#define VSYNC_DEV_NAME2 ""
#endif
#ifndef VSYNC_DEV_MIDDLE2
#define VSYNC_DEV_MIDDLE2 ""
#endif
#endif

const size_t NUM_GSC_UNITS = 0;

#define NUM_VIRT_OVER   5

#define NUM_VIRT_OVER_HDMI 5

#define HWC_PAGE_MISS_TH  5

#define S3D_ERROR -1
#define HDMI_PRESET_DEFAULT V4L2_DV_1080P60
#define HDMI_PRESET_ERROR -1

#define HWC_FIMD_BW_TH  1   /* valid range 1 to 5 */
#define HWC_FPS_TH          5    /* valid range 1 to 60 */
#define VSYNC_INTERVAL (1000000000.0 / 60)
#define NUM_CONFIG_STABLE   10

#define OTF_SWITCH_THRESHOLD 2

#ifndef HLOG_CODE
#define HLOG_CODE 0
#endif

#define HLOGD(...)
#define HLOGV(...)
#define HLOGE(...)

struct exynos5_hwc_composer_device_1_t;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
    uint32_t fw;
    uint32_t fh;
    uint32_t format;
    uint32_t rot;
    uint32_t cacheable;
    uint32_t drmMode;
    uint32_t index;
} video_layer_config;


struct exynos5_hwc_post_data_t {
    int                 overlay_map[NUM_HW_WINDOWS];
    size_t              fb_window;
};

struct hwc_ctrl_t {
    int     max_num_ovly;
    int     num_of_video_ovly;
    int     dynamic_recomp_mode;
    int     skip_static_layer_mode;
    int     dma_bw_balance_mode;
};



class ExynosPrimaryDisplay;


struct exynos5_hwc_composer_device_1_t {
    hwc_composer_device_1_t base;

    ExynosPrimaryDisplay    *primaryDisplay;

    int                     vsync_fd;

    const hwc_procs_t       *procs;
    pthread_t               vsync_thread;



    int VsyncInterruptStatus;
    int CompModeSwitch;
    uint64_t LastUpdateTimeStamp;
    uint64_t LastModeSwitchTimeStamp;
    int updateCallCnt;
    pthread_t   update_stat_thread;
    int update_event_cnt;
    volatile bool update_stat_thread_flag;

    struct hwc_ctrl_t    hwc_ctrl;

    int setCount;
};


#endif
