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

#ifdef USES_VPP
#ifdef USES_VPP_V2
#include <decon_8890.h>
#else
#include <decon-fb.h>
#endif
#include "ExynosDisplayResourceManagerModule.h"
#else
#include <s3c-fb.h>
#endif

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
#ifndef USES_FIMC
#include "exynos_gscaler.h"
#else
#include "exynos_fimc.h"
#endif
#include "exynos_format.h"
#include "exynos_v4l2.h"
#include "s5p_tvout_v4l2.h"
#include "ExynosHWCModule.h"
#include "ExynosRect.h"
#include "videodev2.h"

#ifdef USE_FB_PHY_LINEAR
const size_t NUM_HW_WIN_FB_PHY = 5;
#undef DUAL_VIDEO_OVERLAY_SUPPORT
#define G2D_COMPOSITION
#ifdef G2D_COMPOSITION
#define USE_FIMG2D_API
#endif
#endif

#if defined(DUAL_VIDEO_OVERLAY_SUPPORT)
#define MAX_VIDEO_LAYERS 2
#else
#define MAX_VIDEO_LAYERS 1
#endif

#ifndef FIMD_WORD_SIZE_BYTES
#define FIMD_WORD_SIZE_BYTES   8
#endif

#ifndef FIMD_BURSTLEN
#define FIMD_BURSTLEN   16
#endif

#ifndef DRM_FIMD_BURSTLEN
#define DRM_FIMD_BURSTLEN   8
#endif

#ifndef FIMD_ADDED_BURSTLEN_BYTES
#define FIMD_ADDED_BURSTLEN_BYTES     0
#endif

#ifndef FIMD_TOTAL_BW_LIMIT
#define FIMD_TOTAL_BW_LIMIT (2560 * 1600 * 5)
#endif

#ifndef WINUPDATE_X_ALIGNMENT
#define WINUPDATE_X_ALIGNMENT (8)
#endif

#ifndef WINUPDATE_W_ALIGNMENT
#define WINUPDATE_W_ALIGNMENT (8)
#endif

#ifndef WINUPDATE_DSC_H_SLICE_NUM
#define WINUPDATE_DSC_H_SLICE_NUM (4)
#endif

#ifndef WINUPDATE_DSC_Y_SLICE_SIZE
#define WINUPDATE_DSC_Y_SLICE_SIZE (64)
#endif

#ifndef WINUPDATE_THRESHOLD
#define WINUPDATE_THRESHOLD (75)
#endif

#ifndef WINUPDATE_MIN_HEIGHT
#define WINUPDATE_MIN_HEIGHT (1)
#endif

#define MEDIA_PROCESSOR_GSC  0
#define MEDIA_PROCESSOR_FIMC 1
#define MEDIA_PROCESSOR_G2D  2

#ifdef USES_FIMC
#define DEFAULT_MEDIA_PROCESSOR MEDIA_PROCESSOR_FIMC
#else
#define DEFAULT_MEDIA_PROCESSOR MEDIA_PROCESSOR_GSC
#endif

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

#ifndef HWC_VERSION
#define HWC_VERSION HWC_DEVICE_API_VERSION_1_3
#endif

#ifndef IDMA_SECURE
#define IDMA_SECURE IDMA_G2
#endif

const size_t NO_FB_NEEDED = NUM_HW_WINDOWS + 1;

#ifndef FIMD_BW_OVERLAP_CHECK
const size_t MAX_NUM_FIMD_DMA_CH = 2;
const int FIMD_DMA_CH_IDX[S3C_FB_MAX_WIN] = {0, 1, 1, 1, 0};
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

#ifdef USES_VPP
#ifndef MPP_VPP_G
#define MPP_VPP_G       10
const uint32_t VPP_ASSIGN_ORDER[] = {MPP_VG, MPP_VGR};
#else
const uint32_t VPP_ASSIGN_ORDER[] = {MPP_VPP_G, MPP_VG, MPP_VGR};
#endif
#endif

const size_t NUM_GSC_UNITS = sizeof(AVAILABLE_GSC_UNITS) /
        sizeof(AVAILABLE_GSC_UNITS[0]);

const size_t BURSTLEN_BYTES = FIMD_BURSTLEN * FIMD_WORD_SIZE_BYTES + FIMD_ADDED_BURSTLEN_BYTES;
const size_t DRM_BURSTLEN_BYTES = DRM_FIMD_BURSTLEN * FIMD_WORD_SIZE_BYTES + FIMD_ADDED_BURSTLEN_BYTES;
#if defined(NOT_USE_TRIPLE_BUFFER)
const size_t NUM_HDMI_BUFFERS = 4;
#else
const size_t NUM_HDMI_BUFFERS = 3;
#endif

#define NUM_VIRT_OVER   5

#define NUM_VIRT_OVER_HDMI 5

#define HWC_PAGE_MISS_TH  5

#define S3D_ERROR -1
#ifndef HDMI_INCAPABLE
#define HDMI_PRESET_DEFAULT V4L2_DV_1080P60
#else
#define HDMI_PRESET_DEFAULT 0
#endif
#define HDMI_PRESET_ERROR -1

#define HWC_FIMD_BW_TH  1   /* valid range 1 to 5 */
#define HWC_FPS_TH          5    /* valid range 1 to 60 */
#define VSYNC_INTERVAL (1000000000.0 / 60)
#define NUM_CONFIG_STABLE   10

#define OTF_SWITCH_THRESHOLD 2

#ifndef HLOG_CODE
#define HLOG_CODE 0
#endif

extern int hwcDebug;

inline bool hwcPrintDebugMessages()
{
    return hwcDebug & (1 << HLOG_CODE);
}

#if defined(DISABLE_HWC_DEBUG)
#define HLOGD(...)
#define HLOGV(...)
#define HLOGE(...)
#else
#define HLOGD(...) \
    if (hwcPrintDebugMessages()) \
        ALOGD(__VA_ARGS__);
#define HLOGV(...) \
    if (hwcPrintDebugMessages()) \
        ALOGV(__VA_ARGS__);
#define HLOGE(...) \
    if (hwcPrintDebugMessages()) \
        ALOGE(__VA_ARGS__);
#endif

typedef enum _COMPOS_MODE_SWITCH {
    NO_MODE_SWITCH,
    HWC_2_GLES = 1,
    GLES_2_HWC,
} HWC_COMPOS_MODE_SWITCH;

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

struct exynos5_gsc_map_t {
    enum {
        GSC_NONE = 0,
        GSC_M2M,
        // TODO: GSC_LOCAL_PATH
        GSC_LOCAL,
    } mode;
    int idx;
};

struct exynos5_hwc_post_data_t {
    int                 overlay_map[NUM_HW_WINDOWS];
    exynos5_gsc_map_t   gsc_map[NUM_HW_WINDOWS];
    size_t              fb_window;
};

const size_t NUM_GSC_DST_BUFS = 3;
const size_t NUM_DRM_GSC_DST_BUFS = 2;
struct exynos5_gsc_data_t {
    void            *gsc;
    exynos_mpp_img  src_cfg;
    exynos_mpp_img  mid_cfg;
    exynos_mpp_img  dst_cfg;
    buffer_handle_t dst_buf[NUM_GSC_DST_BUFS];
    buffer_handle_t mid_buf[NUM_GSC_DST_BUFS];
    int             dst_buf_fence[NUM_GSC_DST_BUFS];
    int             mid_buf_fence[NUM_GSC_DST_BUFS];
    size_t          current_buf;
    int             gsc_mode;
    uint32_t    last_gsc_lay_hnd;
};

struct hdmi_layer_t {
    int     id;
    int     fd;
    bool    enabled;
    exynos_mpp_img  cfg;

    bool    streaming;
    size_t  current_buf;
    size_t  queued_buf;
};

struct hwc_ctrl_t {
    int     max_num_ovly;
    int     num_of_video_ovly;
    int     dynamic_recomp_mode;
    int     skip_static_layer_mode;
    int     dma_bw_balance_mode;
};

#if defined(G2D_COMPOSITION) || defined(USE_GRALLOC_FLAG_FOR_HDMI)
#include "FimgApi.h"
#endif

#ifdef G2D_COMPOSITION
struct exynos5_g2d_data_t {
     int    ovly_lay_idx[NUM_HW_WIN_FB_PHY];
     int    win_used[NUM_HW_WINDOWS];
};
#endif

class ExynosPrimaryDisplay;
class ExynosExternalDisplay;
class ExynosVirtualDisplay;
#if defined(USES_DUAL_DISPLAY)
class ExynosSecondaryDisplayModule;
#endif
#ifdef USES_VPP
class ExynosDisplayResourceManagerModule;
#endif

struct exynos5_hwc_composer_device_1_t {
    hwc_composer_device_1_t base;

    ExynosPrimaryDisplay    *primaryDisplay;
#if defined(USES_DUAL_DISPLAY)
    ExynosSecondaryDisplayModule    *secondaryDisplay;
#endif
    ExynosExternalDisplay    *externalDisplay;
    ExynosVirtualDisplay    *virtualDisplay;
    struct v4l2_rect        mVirtualDisplayRect;
#ifdef USES_VPP
    ExynosDisplayResourceManagerModule *mDisplayResourceManager;
#endif

    int                     vsync_fd;
    FILE                    *psrInfoFd;
    int                     psrMode;
    int                     panelType;

    const hwc_procs_t       *procs;
    pthread_t               vsync_thread;
    int                     force_gpu;

    bool hdmi_hpd;

    int mHdmiPreset;
    int mHdmiCurrentPreset;
    bool mHdmiResolutionChanged;
    bool mHdmiResolutionHandled;
    int mS3DMode;
    bool mUseSubtitles;
    int video_playback_status;

    int VsyncInterruptStatus;
    int CompModeSwitch;
    uint64_t LastUpdateTimeStamp;
    uint64_t LastModeSwitchTimeStamp;
    int totPixels;
    int incomingPixels;
    int updateCallCnt;
    pthread_t   update_stat_thread;
    int update_event_cnt;
    volatile bool update_stat_thread_flag;

    struct hwc_ctrl_t    hwc_ctrl;

    int mCecFd;
    int mCecPaddr;
    int mCecLaddr;

    bool                    force_mirror_mode;
    int                     ext_fbt_transform;                  /* HAL_TRANSFORM_ROT_XXX */
    bool                    external_display_pause;
    bool                    local_external_display_pause;

    bool notifyPSRExit;
    bool allowOTF;
    int setCount;
    bool debugMessages;
};

void exynos5_create_update_stat_thread(struct exynos5_hwc_composer_device_1_t *dev);

enum {
    OTF_OFF = 0,
    OTF_RUNNING,
    OTF_TO_M2M,
    SEC_M2M,
};

enum {
    S3D_MODE_DISABLED = 0,
    S3D_MODE_READY,
    S3D_MODE_RUNNING,
    S3D_MODE_STOPPING,
};

enum {
    S3D_FB = 0,
    S3D_SBS,
    S3D_TB,
    S3D_NONE,
};

enum {
    NO_DRM = 0,
    NORMAL_DRM,
    SECURE_DRM,
};

enum {
    PSR_NONE = 0,
    PSR_DP,
    PSR_MIPI,
};

enum {
    PANEL_LEGACY = 0,
    PANEL_DSC,
};
#endif
