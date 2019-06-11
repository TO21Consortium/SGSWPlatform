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
 * \file      exynos_blender.cpp
 * \brief     User's API for Exynos Blender library
 * \author    Eunseok Choi (es10.choi@samsung.com)
 * \date      2013/09/21
 *
 * <b>Revision History: </b>
 * - 2013.09.21 : Eunseok Choi (eunseok.choi@samsung.com) \n
 *   Create
 *
 */
#include "exynos_blender.h"
#include "exynos_blender_obj.h"
#include "libg2d_obj.h"

static CBlender *GetExynosBlender(bl_handle_t handle)
{
    if (handle == NULL) {
        BL_LOGE("Handle is null");
        return NULL;
    }

    CBlender *bl = reinterpret_cast<CBlender *>(handle);
    if (!bl->Valid()) {
        BL_LOGE("Handle is invalid %p", handle);
        return NULL;
    }

    return bl;
}

bl_handle_t exynos_bl_create(struct bl_property *prop)
{
    CBlender *bl;

    if (prop->devid == DEV_UNSPECIFIED) {
        BL_LOGE("Reserved device id %d\n", prop->devid);
        return NULL;

    } else if (prop->devid < DEVID_G2D_END) {
        bl = new CFimg2d(prop->devid, prop->nonblock);
        if (!bl) {
            BL_LOGE("Failed to create Fimg2d handle\n");
            return NULL;
        }

        if (!bl->Valid()) {
            BL_LOGE("Fimg2d handle %p is not valid\n", bl);
            delete bl;
            return NULL;
        }
        return reinterpret_cast<void *>(bl);

    } else {
        BL_LOGE("Uknown device id %d\n", prop->devid);
        return NULL;
    }
}

void exynos_bl_destroy(bl_handle_t handle)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return;

    if (bl->DoStop()) {
        BL_LOGE("Failed to stop Blender (handle %p)", handle);
        return;
    }

    delete bl;
}

int exynos_bl_deactivate(bl_handle_t handle, bool deact)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->Deactivate(deact);
}

int exynos_bl_set_color_fill(
        bl_handle_t handle,
        bool enable,
        uint32_t color_argb8888)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->SetColorFill(enable, color_argb8888);
}

int exynox_bl_set_rotate(
        bl_handle_t handle,
        enum BL_ROTATE rot,
        bool hflip,
        bool vflip)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->SetRotate(rot, hflip, vflip);
}

int exynos_bl_set_blend(
        bl_handle_t handle,
        enum BL_OP_TYPE op,
        bool premultiplied)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->SetBlend(op, premultiplied);
}

int exynos_bl_set_galpha(
        bl_handle_t handle,
        bool enable,
        unsigned char g_alpha)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->SetGlobalAlpha(enable, g_alpha);
}

int exynos_bl_set_dither(bl_handle_t handle, bool enable)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->SetDither(enable);
}

int exynos_bl_set_scale(
        bl_handle_t handle,
        enum BL_SCALE mode,
        uint32_t src_w,
        uint32_t dst_w,
        uint32_t src_h,
        uint32_t dst_h)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->SetScale(mode, src_w, dst_w, src_h, dst_h);
}

int exynos_bl_set_repeat(bl_handle_t handle, enum BL_REPEAT mode)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->SetRepeat(mode);
}

int exynos_bl_set_clip(
        bl_handle_t handle,
        bool enable,
        uint32_t x,
        uint32_t y,
        uint32_t width,
        uint32_t height)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->SetClipRect(enable, x, y, width, height);
}

int exynos_bl_set_csc_spec(
        bl_handle_t handle,
        bool enable,
        enum v4l2_colorspace space,
        bool wide)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->SetCscSpec(enable, space, wide);
}

int exynos_bl_set_src_format(
        bl_handle_t handle,
        unsigned int width,
        unsigned int height,
        unsigned int crop_x,
        unsigned int crop_y,
        unsigned int crop_width,
        unsigned int crop_height,
        unsigned int v4l2_colorformat)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->SetImageFormat(CBlender::SRC, width, height,
            crop_x, crop_y, crop_width, crop_height, v4l2_colorformat);
}

int exynos_bl_set_dst_format(
        bl_handle_t handle,
        unsigned int width,
        unsigned int height,
        unsigned int crop_x,
        unsigned int crop_y,
        unsigned int crop_width,
        unsigned int crop_height,
        unsigned int v4l2_colorformat)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->SetImageFormat(CBlender::DST, width, height,
            crop_x, crop_y, crop_width, crop_height, v4l2_colorformat);
}

int exynos_bl_set_src_addr(
        bl_handle_t handle,
        void *addr[BL_MAX_PLANES],
        enum v4l2_memory type)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->SetAddr(CBlender::SRC, addr, type);
}

int exynos_bl_set_dst_addr(
        bl_handle_t handle,
        void *addr[BL_MAX_PLANES],
        enum v4l2_memory type)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    return bl->SetAddr(CBlender::DST, addr, type);
}

int exynos_bl_do_blend(bl_handle_t handle)
{
    CBlender *bl = GetExynosBlender(handle);
    if (!bl)
        return -1;

    int ret = bl->DoStart();
    if (ret)
        return ret;

    bl->DoStop();
    return 0;
}

int exynos_bl_do_blend_fast(bl_handle_t handle)
{
    BL_LOGE("Unimplemented Operation (handle %p)", handle);
    return -1;
}

