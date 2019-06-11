/*
 * Copyright (C) 2013 The Android Open Source Project
 * Copyright@ Samsung Electronics Co. LTD
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

/*!
 * \file      exynos_scaler.c
 * \brief     header file for Scaler HAL
 * \author    Sunyoung Kang (sy0816.kang@samsung.com)
 * \date      2013/02/01
 *
 * <b>Revision History: </b>
 * - 2013.02.01 : Sunyoung Kang (sy0816.kang@samsung.com) \n
 *   Create
 *
 * - 2013.04.26 : Cho KyongHo (pullip.cho@samsung.com \n
 *   Library rewrite
 *
 */

#ifndef _EXYNOS_SCALER_H_
#define _EXYNOS_SCALER_H_


#include <videodev2.h>
#include <videodev2_exynos_media.h>
#include <stdbool.h>

#include "exynos_format.h"
#include "exynos_v4l2.h"
#include "exynos_gscaler.h"

#define SC_DEV_NODE     "/dev/video"
#define SC_NODE(x)      (50 + x)

#define SC_NUM_OF_PLANES    (3)

#define V4L2_PIX_FMT_NV12_RGB32 v4l2_fourcc('N', 'V', '1', 'R')
#define V4L2_PIX_FMT_NV12N_RGB32 v4l2_fourcc('N', 'N', '1', 'R')
#define V4L2_PIX_FMT_NV12M_RGB32 v4l2_fourcc('N', 'V', 'R', 'G')
#define V4L2_PIX_FMT_NV12M_BGR32 v4l2_fourcc('N', 'V', 'B', 'G')
#define V4L2_PIX_FMT_NV12M_RGB565 v4l2_fourcc('N', 'V', 'R', '6')
#define V4L2_PIX_FMT_NV12M_RGB444 v4l2_fourcc('N', 'V', 'R', '4')
#define V4L2_PIX_FMT_NV12M_RGB555X v4l2_fourcc('N', 'V', 'R', '5')
#define V4L2_PIX_FMT_NV12MT_16X16_RGB32 v4l2_fourcc('V', 'M', 'R', 'G')
#define V4L2_PIX_FMT_NV21M_RGB32 v4l2_fourcc('V', 'N', 'R', 'G')
#define V4L2_PIX_FMT_NV21M_BGR32 v4l2_fourcc('V', 'N', 'B', 'G')
#define V4L2_PIX_FMT_NV21_RGB32 v4l2_fourcc('V', 'N', '1', 'R')
#define V4L2_PIX_FMT_YVU420_RGB32 v4l2_fourcc('Y', 'V', 'R', 'G')

#define V4L2_CID_2D_SRC_BLEND_SET_FMT (V4L2_CID_EXYNOS_BASE + 116)
#define V4L2_CID_2D_SRC_BLEND_SET_H_POS (V4L2_CID_EXYNOS_BASE + 117)
#define V4L2_CID_2D_SRC_BLEND_SET_V_POS (V4L2_CID_EXYNOS_BASE + 118)
#define V4L2_CID_2D_SRC_BLEND_FMT_PREMULTI (V4L2_CID_EXYNOS_BASE + 119)
#define V4L2_CID_2D_SRC_BLEND_SET_STRIDE (V4L2_CID_EXYNOS_BASE + 120)
#define V4L2_CID_2D_SRC_BLEND_SET_WIDTH (V4L2_CID_EXYNOS_BASE + 121)
#define V4L2_CID_2D_SRC_BLEND_SET_HEIGHT (V4L2_CID_EXYNOS_BASE + 122)

#ifdef SCALER_USE_LOCAL_CID
#define V4L2_CID_GLOBAL_ALPHA          (V4L2_CID_EXYNOS_BASE + 1)
#define V4L2_CID_2D_BLEND_OP           (V4L2_CID_EXYNOS_BASE + 103)
#define V4L2_CID_2D_COLOR_FILL         (V4L2_CID_EXYNOS_BASE + 104)
#define V4L2_CID_2D_DITH               (V4L2_CID_EXYNOS_BASE + 105)
#define V4L2_CID_2D_FMT_PREMULTI       (V4L2_CID_EXYNOS_BASE + 106)
#endif

#define LIBSC_V4L2_CID_DNOISE_FT        (V4L2_CID_EXYNOS_BASE + 150)
#define LIBSC_M2M1SHOT_OP_FILTER_SHIFT  (28)
#define LIBSC_M2M1SHOT_OP_FILTER_MASK   (0xf << 28)

// libgscaler's internal use only
typedef enum _HW_SCAL_ID {
    HW_SCAL0 = 4,
    HW_SCAL1,
    HW_SCAL2,
    HW_SCAL_MAX,
} HW_SCAL_ID;

// argument of non-blocking api
typedef exynos_mpp_img exynos_sc_img;

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Create libscaler handle
 *
 * \ingroup exynos_scaler
 *
 * \param dev_num
 *  scaler dev_num[in]
 *
 * \return
 * libscaler handle
 */
void *exynos_sc_create(int dev_num);

/*!
 * Destroy libscaler handle
 *
 * \ingroup exynos_scaler
 *
 * \param handle
 *   libscaler handle[in]
 */
int exynos_sc_destroy(void *handle);

/*!
 * Convert color space with presetup color format
 *
 * \ingroup exynos_scaler
 *
 * \param handle
 *   libscaler handle[in]
 *
 * \return
 *   error code
 */
int exynos_sc_convert(void *handle);

/*!
 * Convert color space with presetup color format
 *
 * \ingroup exynos_scaler
 *
 * \param handle
 *   libscaler handle
 *
 * \param csc_range
 *   csc narrow/wide property
 *
 * \param v4l2_colorspace
 *   csc equation property
 *
 * \param filter
 *   denoise filter info
 *
 * \return
 *   error code
 */
int exynos_sc_set_csc_property(
    void        *handle,
    unsigned int csc_range,
    unsigned int v4l2_colorspace,
    unsigned int filter);

/*!
 * Set source format.
 *
 * \ingroup exynos_scaler
 *
 * \param handle
 *   libscaler handle[in]
 *
 * \param width
 *   image width[in]
 *
 * \param height
 *   image height[in]
 *
 * \param crop_left
 *   image left crop size[in]
 *
 * \param crop_top
 *   image top crop size[in]
 *
 * \param crop_width
 *   cropped image width[in]
 *
 * \param crop_height
 *   cropped image height[in]
 *
 * \param v4l2_colorformat
 *   color format[in]
 *
 * \param cacheable
 *   ccacheable[in]
 *
 * \param mode_drm
 *   mode_drm[in]
 *
 * \param premultiplied
 *   pre-multiplied format[in]
 *
 * \return
 *   error code
 */
int exynos_sc_set_src_format(
    void        *handle,
    unsigned int width,
    unsigned int height,
    unsigned int crop_left,
    unsigned int crop_top,
    unsigned int crop_width,
    unsigned int crop_height,
    unsigned int v4l2_colorformat,
    unsigned int cacheable,
    unsigned int mode_drm,
    unsigned int premultiplied);

/*!
 * Set destination format.
 *
 * \ingroup exynos_scaler
 *
 * \param handle
 *   libscaler handle[in]
 *
 * \param width
 *   image width[in]
 *
 * \param height
 *   image height[in]
 *
 * \param crop_left
 *   image left crop size[in]
 *
 * \param crop_top
 *   image top crop size[in]
 *
 * \param crop_width
 *   cropped image width[in]
 *
 * \param crop_height
 *   cropped image height[in]
 *
 * \param v4l2_colorformat
 *   color format[in]
 *
 * \param cacheable
 *   ccacheable[in]
 *
 * \param mode_drm
 *   mode_drm[in]
 *
 * \param premultiplied
 *   pre-multiplied format[in]
 *
 * \return
 *   error code
 */
int exynos_sc_set_dst_format(
    void        *handle,
    unsigned int width,
    unsigned int height,
    unsigned int crop_left,
    unsigned int crop_top,
    unsigned int crop_width,
    unsigned int crop_height,
    unsigned int v4l2_colorformat,
    unsigned int cacheable,
    unsigned int mode_drm,
    unsigned int premultiplied);

/*!
 * Set source buffer
 *
 * \ingroup exynos_scaler
 *
 * \param handle
 *   libscaler handle[in]
 *
 * \param addr
 *   buffer pointer array[in]
 *
 * \param mem_type
 *   memory type[in]
 *
 * \param acquireFenceFd
 *   acquire fence fd for the buffer or -1[in]
 *
 * \return
 *   error code
 */

int exynos_sc_set_src_addr(
    void *handle,
    void *addr[SC_NUM_OF_PLANES],
    int mem_type,
    int acquireFenceFd);

/*!
 * Set destination buffer
 *
 * \param handle
 *   libscaler handle[in]
 *
 * \param addr
 *   buffer pointer array[in]
 *
 * \param mem_type
 *   memory type[in]
 *
 * \param acquireFenceFd
 *   acquire fence fd for the buffer or -1[in]
 *
 * \return
 *   error code
 */
int exynos_sc_set_dst_addr(
    void *handle,
    void *addr[SC_NUM_OF_PLANES],
    int mem_type,
    int acquireFenceFd);

/*!
 * Set rotation.
 *
 * \ingroup exynos_scaler
 *
 * \param handle
 *   libscaler handle[in]
 *
 * \param rot
 *   image rotation. It should be multiple of 90[in]
 *
 * \param flip_h
 *   image flip_horizontal[in]
 *
 * \param flip_v
 *   image flip_vertical[in]
 *
 * \return
 *   error code
 */
int exynos_sc_set_rotation(
    void *handle,
    int rot,
    int flip_h,
    int flip_v);

////// non-blocking /////

void *exynos_sc_create_exclusive(
    int dev_num,
    int allow_drm);

int exynos_sc_csc_exclusive(void *handle,
    unsigned int range_full,
    unsigned int v4l2_colorspace);

int exynos_sc_config_exclusive(
    void *handle,
    exynos_sc_img *src_img,
    exynos_sc_img *dst_img);

int exynos_sc_run_exclusive(
    void *handle,
    exynos_sc_img *src_img,
    exynos_sc_img *dst_img);

void *exynos_sc_create_blend_exclusive(
        int dev_num,
        int allow_drm);

int exynos_sc_config_blend_exclusive(
    void *handle,
    exynos_sc_img *src_img,
    exynos_sc_img *dst_img,
    struct SrcBlendInfo  *srcblendinfo);

int exynos_sc_wait_frame_done_exclusive
(void *handle);

int exynos_sc_stop_exclusive
(void *handle);

int exynos_sc_free_and_close
(void *handle);


/******************************************************************************
 ******** API for Copy Pixels between RGB data ********************************
 ******************************************************************************/

/*!
 * Description of an image for both of the source and the destination.
 *
 * \ingroup exynos_scaler
 */
struct exynos_sc_pxinfo_img
{
    void *addr;
    unsigned int width;
    unsigned int height;
    unsigned int crop_left;
    unsigned int crop_top;
    unsigned int crop_width;
    unsigned int crop_height;
    unsigned int pxfmt;  // enum EXYNOS_SC_FMT_PXINFO
};

/*!
 * Description of a pixel copy
 *
 * \ingroup exynos_scaler
 */
struct exynos_sc_pxinfo {
    struct exynos_sc_pxinfo_img src;
    struct exynos_sc_pxinfo_img dst;
    unsigned short rotate; // 0 ~ 360
    char hflip;  // non-zero value for hflip
    char vflip;  // non-zero value for vflip
};

/*!
 * Pixel format definition for pixel copy
 *
 * \ingroup exynos_scaler
 */
enum SC_FMT_PXINFO {
    EXYNOS_SC_FMT_RGB32 = 0x10,
    EXYNOS_SC_FMT_BGR32,
    EXYNOS_SC_FMT_RGB565,
    EXYNOS_SC_FMT_RGB555X,
    EXYNOS_SC_FMT_RGB444,
};

/*!
 * Copy pixel data from RGB to RGB
 *
 * \ingroup exynos_scaler
 *
 * \param pxinfo
 *   information for pixel data copy [in]
 *
 * \param dev_num
 *   Scaler H/W instance number. Starts from 0 [in]
 *
 * \return
 *   true on success in copying pixel data.
 *   false on failure.
 */
bool exynos_sc_copy_pixels(
    struct exynos_sc_pxinfo *pxinfo,
    int dev_num);

#ifdef __cplusplus
}
#endif

#endif /* _EXYNOS_SCALER_H_ */
