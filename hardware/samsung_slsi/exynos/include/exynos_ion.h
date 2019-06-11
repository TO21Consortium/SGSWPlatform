/*
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
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

#ifndef _LIB_ION_H_
#define _LIB_ION_H_

#define ION_HEAP_SYSTEM_MASK            (1 << 0)
#define ION_HEAP_EXYNOS_CONTIG_MASK     (1 << 4)
#define ION_EXYNOS_VIDEO_EXT_MASK       (1 << 31)
#define ION_EXYNOS_VIDEO_EXT2_MASK      (1 << 29)
#define ION_EXYNOS_FIMD_VIDEO_MASK      (1 << 28)
#define ION_EXYNOS_GSC_MASK             (1 << 27)
#define ION_EXYNOS_MFC_OUTPUT_MASK      (1 << 26)
#define ION_EXYNOS_MFC_INPUT_MASK       (1 << 25)
#define ION_EXYNOS_G2D_WFD_MASK         (1 << 22)
#define ION_EXYNOS_VIDEO_MASK           (1 << 21)

enum {
	ION_EXYNOS_HEAP_ID_CRYPTO           = 1,
	ION_EXYNOS_HEAP_ID_VIDEO_FW         = 2,
	ION_EXYNOS_HEAP_ID_VIDEO_STREAM     = 3,
	ION_EXYNOS_HEAP_ID_RESERVED         = 4,
	ION_EXYNOS_HEAP_ID_VIDEO_FRAME      = 5,
	ION_EXYNOS_HEAP_ID_VIDEO_SCALER     = 6,
	ION_EXYNOS_HEAP_ID_VIDEO_NFW        = 7,
	ION_EXYNOS_HEAP_ID_GPU_CRC          = 8,
	ION_EXYNOS_HEAP_ID_GPU_BUFFER       = 9,
	ION_EXYNOS_HEAP_ID_CAMERA           = 10,
};

#define EXYNOS_ION_HEAP_CRYPTO_MASK         (1 << ION_EXYNOS_HEAP_ID_CRYPTO)
#define EXYNOS_ION_HEAP_VIDEO_FW_MASK       (1 << ION_EXYNOS_HEAP_ID_VIDEO_FW)
#define EXYNOS_ION_HEAP_VIDEO_STREAM_MASK   (1 << ION_EXYNOS_HEAP_ID_VIDEO_STREAM)
#define EXYNOS_ION_HEAP_VIDEO_FRAME_MASK    (1 << ION_EXYNOS_HEAP_ID_VIDEO_FRAME)
#define EXYNOS_ION_HEAP_VIDEO_SCALER_MASK   (1 << ION_EXYNOS_HEAP_ID_VIDEO_SCALER)
#define EXYNOS_ION_HEAP_VIDEO_NFW_MASK      (1 << ION_EXYNOS_HEAP_ID_VIDEO_NFW)
#define EXYNOS_ION_HEAP_GPU_CRC             (1 << ION_EXYNOS_HEAP_ID_GPU_CRC)
#define EXYNOS_ION_HEAP_GPU_BUFFER          (1 << ION_EXYNOS_HEAP_ID_GPU_BUFFER)
#define EXYNOS_ION_HEAP_CAMERA              (1 << ION_EXYNOS_HEAP_ID_CAMERA)

#endif /* _LIB_ION_H_ */
