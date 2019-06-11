/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef ANDROID_EXYNOS_HWC_MODULE_H_
#define ANDROID_EXYNOS_HWC_MODULE_H_
#include <hardware/hwcomposer.h>

#define VSYNC_DEV_PREFIX "/sys/devices/"
#define VSYNC_DEV_MIDDLE "14850000.sysmmu/14850000.sysmmu/"
#define VSYNC_DEV_NAME  "14830000.decon_fb/vsync"

#define FIMD_WORD_SIZE_BYTES   16
#define FIMD_BURSTLEN   16
#define FIMD_BW_OVERLAP_CHECK

#define TRY_SECOND_VSYNC_DEV
#ifdef TRY_SECOND_VSYNC_DEV
#define VSYNC_DEV_NAME2  "exynos5-fb.1/vsync"
#define VSYNC_DEV_MIDDLE2  "platform/exynos-sysmmu.30/exynos-sysmmu.11/"
#endif

#define HDMI_RESERVE_MEM_DEV_NAME "/sys/class/ion_cma/ion_video_ext/isolated"
#define SMEM_PATH "/dev/s5p-smem"
#define SECMEM_IOC_SET_VIDEO_EXT_PROC   _IOWR('S', 13, int)

#define HWC_VERSION HWC_DEVICE_API_VERSION_1_5

#define DUAL_VIDEO_OVERLAY_SUPPORT

/* Max number windows available in Exynos7570 is 3. */
#define NUM_AVAILABLE_HW_WINDOWS	3

#ifdef FIMD_BW_OVERLAP_CHECK
const size_t MAX_NUM_FIMD_DMA_CH = 3;
const uint32_t FIMD_DMA_CH_IDX[] = {0, 1, 2};
const uint32_t FIMD_DMA_CH_BW_SET1[MAX_NUM_FIMD_DMA_CH] = {1920 * 1080, 1920 * 1080, 1920 * 1080};
const uint32_t FIMD_DMA_CH_BW_SET2[MAX_NUM_FIMD_DMA_CH] = {1920 * 1200, 1920 * 1200, 1920 * 1200};
const uint32_t FIMD_DMA_CH_OVERLAP_CNT_SET1[MAX_NUM_FIMD_DMA_CH] = {1, 1, 1};
const uint32_t FIMD_DMA_CH_OVERLAP_CNT_SET2[MAX_NUM_FIMD_DMA_CH] = {1, 1, 1};

/*
 * TODO: All channels are enabled for WUXGA channels. Need to check
 * if this is supported. If yes disable BW CHK else fine tune.
 */

inline void fimd_bw_overlap_limits_init(int xres, int yres,
            uint32_t *fimd_dma_chan_max_bw, uint32_t *fimd_dma_chan_max_overlap_cnt)
{
    if (xres * yres > 1920 * 1080) {
        for (size_t i = 0; i < MAX_NUM_FIMD_DMA_CH; i++) {
            fimd_dma_chan_max_bw[i] = FIMD_DMA_CH_BW_SET2[i];
            fimd_dma_chan_max_overlap_cnt[i] = FIMD_DMA_CH_OVERLAP_CNT_SET2[i];
        }
    } else {
        for (size_t i = 0; i < MAX_NUM_FIMD_DMA_CH; i++) {
            fimd_dma_chan_max_bw[i] = FIMD_DMA_CH_BW_SET1[i];
            fimd_dma_chan_max_overlap_cnt[i] = FIMD_DMA_CH_OVERLAP_CNT_SET1[i];
        }
    }
}
#endif

const size_t GSC_DST_W_ALIGNMENT_RGB888 = 1;
const size_t GSC_DST_CROP_W_ALIGNMENT_RGB888 = 1;
const size_t GSC_W_ALIGNMENT = 16;
const size_t GSC_H_ALIGNMENT = 16;
const size_t GSC_DST_H_ALIGNMENT_RGB888 = 1;

const size_t FIMD_GSC_IDX = 0;
const size_t FIMD_EXT_MPP_IDX = 0;
/* HDMI_GSC_IDX is not used but added for build issue */
const size_t HDMI_GSC_IDX = 2;
const size_t HDMI_EXT_MPP_IDX = 2;
const size_t WFD_GSC_IDX = 1;
const size_t WFD_EXT_MPP_IDX = 1;

const int FIMD_GSC_USAGE_IDX[] = {FIMD_GSC_IDX};
const int AVAILABLE_GSC_UNITS[] = { 0, 1};

#define MPP_VG          0
#define MPP_VGR         2
#define MPP_MSC         4
#define MPP_MSC_1	5

#define EXTERNAL_MPPS   2

struct exynos_mpp_t {
    int type;
    unsigned int index;
};

const exynos_mpp_t AVAILABLE_EXTERNAL_MPP_UNITS[] = {{MPP_MSC, 0}, {MPP_MSC_1, 0} };

#endif
