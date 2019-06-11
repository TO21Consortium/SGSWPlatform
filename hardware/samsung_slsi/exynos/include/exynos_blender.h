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
 * \file      exynos_blender.h
 * \brief     User's API for Exynos Blender library
 * \author    Eunseok Choi (es10.choi@samsung.com)
 * \date      2013/09/21
 *
 * <b>Revision History: </b>
 * - 2013.09.21 : Eunseok Choi (eunseok.choi@samsung.com) \n
 *   Create
 *
 */
#ifndef __EXYNOS_BLENDER_H__
#define __EXYNOS_BLENDER_H__

#include "videodev2.h"

#define BL_MAX_PLANES   3

enum BL_DEVID {
    DEV_UNSPECIFIED = 0,
    DEV_G2D0, DEVID_G2D_END,
};

enum BL_OP_TYPE {
    OP_SRC = 2,
    OP_SRC_OVER = 4
};

enum BL_ROTATE {
    ORIGIN,
    ROT90  = 90,
    ROT180 = 180,
    ROT270 = 270
};

enum BL_SCALE {
    NOSCALE,
    NEAREST,
    BILINEAR,
    POLYPHASE
};

enum BL_REPEAT {
    NOREPEAT,
    NORMAL,
    MIRROR = 3,
    REFLECT = MIRROR,
    CLAMP
};

enum BL_BLUESCREEN {
    OPAQUE,
    //! transparent mode. need bgcolor
    TRANSP,
    //! bluescreen mode. need bgcolor and bscolor
    BLUSCR
};

struct bl_property {
    enum BL_DEVID devid;
    //! true: device open with NONBLOCK flag.
    bool nonblock;
};

typedef void *bl_handle_t;

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * Create exynos_blender handle
 *
 * \ingroup exynos_blender
 *
 * \param prop
 *   blender property[in]
 *
 * \return
 *   exynos_blender handle
 */
bl_handle_t exynos_bl_create(struct bl_property *prop);

/*!
 * Destroy exynos_blender handle
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 */
void exynos_bl_destroy(bl_handle_t handle);

/*!
 * Deactivate exynos_blender: g2d-specific for drm
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param deact
 *   true: deactivate, false: activated (default)
 *
 * \return
 *   error code
 */
int exynos_bl_deactivate(bl_handle_t handle, bool deact);

/*!
 * Set color fill mode and src color
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param enable
 *   true: color fill mode.
 *   if true, src_addr & src_format is ignored.
 *
 * \param color_argb8888
 *   32-bit color value, 'a' is msb.
 *
 * \return
 *   error code
 */
int exynos_bl_set_color_fill(
        bl_handle_t handle,
        bool enable,
        uint32_t color_argb8888);

/*!
 * Set rotate and flip
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param rot
 *   90/180/270: clockwise degree
 *
 * \param hflip
 *   true: hoizontal(y-axis) flip
 *
 * \param vflip
 *   true: vertical(x-axis) flip
 *
 * \return
 *   error code
 */
int exynox_bl_set_rotate(
        bl_handle_t handle,
        enum BL_ROTATE rot,
        bool hflip,
        bool vflip);

/*!
 * Set blend op mode
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param op
 *   2: SRC COPY (default), 4: SRC OVER
 *
 * \param premultiplied
 *   true: alpha premultiplied mode for src and dst
 *
 * \return
 *   error code
 */
int exynos_bl_set_blend(
        bl_handle_t handle,
        enum BL_OP_TYPE op,
        bool premultiplied);

/*!
 * Set global alpha value
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param enable
 *
 * \param g_alpha
 *   range: 0x0~0xff.
 *   0x0 for transpranet, 0xff for opaque. (default '0xff')
 *
 * \return
 *   error code
 */
int exynos_bl_set_galpha(
        bl_handle_t handle,
        bool enable,
        unsigned char g_alpha);

/*!
 * Set dither
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param enable
 *   true: enable dithering effect
 *
 * \return
 *   error code
 */
int exynos_bl_set_dither(bl_handle_t handle, bool enable);

/*!
 * Set scaling ratio
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param mode
 *   1: nearest, 2: bilinear, 3: polyphase
 *
 * \param src_w
 *   src width in pixels of horizontal scale ratio
 *
 * \param dst_w
 *   dst width in pixels of horizontal scale ratio
 *
 * \param src_h
 *   src height in pixels of vertical scale ratio
 *
 * \param dst_h
 *   dst height in pixels of vertical scale ratio
 *
 * \return
 *   error code
 */
int exynos_bl_set_scale(
        bl_handle_t handle,
        enum BL_SCALE mode,
        uint32_t src_w,
        uint32_t dst_w,
        uint32_t src_h,
        uint32_t dst_h);

/*!
 * Set repeat mode
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param mode
 *   1: normal, 3: mirror(reflect), 4: clamp
 *
 * \return
 *   error code
 */
int exynos_bl_set_repeat(bl_handle_t handle, enum BL_REPEAT mode);

/*!
 * Set dst clip rect
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param enable
 *   true:  clip rect is inside of dst crop rect.
 *
 * \param x
 *   clip left
 *
 * \param y
 *   clip top
 *
 * \param width
 *   clip width
 *
 * \param height
 *   clip height
 *
 * \return
 *   error code
 */
int exynos_bl_set_clip(
        bl_handle_t handle,
        bool enable,
        uint32_t x,
        uint32_t y,
        uint32_t width,
        uint32_t height);

/*!
 * Set colorspace conversion spec
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param enable
 *   true: user-defined, false: auto
 *
 * \param space
 *   V4L2_COLORSPACE_SMPTE170M: 601, V4L2_COLORSPACE_REC709: 709
 *
 * \param wide
 *   true: wide, false: narrow
 *
 * \return
 *   error code
 */
int exynos_bl_set_csc_spec(
        bl_handle_t handle,
        bool enable,
        enum v4l2_colorspace space,
        bool wide);

/*!
 * Set src image format
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param width
 *   full width in pixels
 *
 * \param height
 *   full height in pixels
 *
 * \param crop_x
 *   left of src rect
 *
 * \param crop_y
 *   top of src rect
 *
 * \param crop_width
 *   width of src rect
 *
 * \param crop_height
 *   height of src rect
 *
 * \return
 *   error code
 */
int exynos_bl_set_src_format(
        bl_handle_t handle,
        unsigned int width,
        unsigned int height,
        unsigned int crop_x,
        unsigned int crop_y,
        unsigned int crop_width,
        unsigned int crop_height,
        unsigned int v4l2_colorformat);

/*!
 * Set dst image format
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param width
 *   full width in pixels
 *
 * \param height
 *   full height in pixels
 *
 * \param crop_x
 *   left of dst rect
 *
 * \param crop_y
 *   top of dst rect
 *
 * \param crop_width
 *   width of dst rect
 *
 * \param crop_height
 *   height of dst rect
 *
 * \return
 *   error code
 */
int exynos_bl_set_dst_format(
        bl_handle_t handle,
        unsigned int width,
        unsigned int height,
        unsigned int crop_x,
        unsigned int crop_y,
        unsigned int crop_width,
        unsigned int crop_height,
        unsigned int v4l2_colorformat);

/*!
 * Set src buffer
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param addr[]
 *   base address: userptr(ion or malloc), ion_fd: dmabuf
 *
 * \param type
 *   V4L2_MEMORY_USERPTR: 2, V4L2_MEMORY_DMABUF: 4
 *
 * \return
 *   error code
 */
int exynos_bl_set_src_addr(
        bl_handle_t handle,
        void *addr[BL_MAX_PLANES],
        enum v4l2_memory type);

/*!
 * Set dst buffer
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \param addr[]
 *   base address: userptr(ion or malloc), fd: dmabuf(ion)
 *
 * \param type
 *   V4L2_MEMORY_USERPTR: 2, V4L2_MEMORY_DMABUF: 4
 *
 * \return
 *   error code
 */
int exynos_bl_set_dst_addr(
        bl_handle_t handle,
        void *addr[BL_MAX_PLANES],
        enum v4l2_memory type);

/*!
 * Start blending single frame
 *
 * \ingroup exynos_blender
 *
 * \param handle
 *   exynos_blender handle[in]
 *
 * \return
 *   error code
 */
int exynos_bl_do_blend(bl_handle_t handle);

/*!
 * Start 2-step(scaling & rotation) blending single frame
 *
 * \ingroup exynos_blender
 *
 * \return
 *   error code
 */
int exynos_bl_do_blend_fast(bl_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // __EXYNOS_BLENER_H__
