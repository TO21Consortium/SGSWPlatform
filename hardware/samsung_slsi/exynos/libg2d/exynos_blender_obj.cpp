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
 * \file      exynos_blender_obj.cpp
 * \brief     source file for Exynos Blender library
 * \author    Eunseok Choi (es10.choi@samsung.com)
 * \date      2013/09/21
 *
 * <b>Revision History: </b>
 * - 2013.09.21 : Eunseok Choi (eunseok.choi@samsung.com) \n
 *   Create
 *
 */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>

#include "exynos_blender.h"
#include "exynos_blender_obj.h"

const char *CBlender::m_cszPortName[CBlender::NUM_PORTS] = { "source", "destination" };

int CBlender::SetColorFill(bool enable, uint32_t color_argb8888)
{
    m_Ctrl.fill.enable = enable;
    m_Ctrl.fill.color_argb8888 = color_argb8888;
    SetFlag(F_FILL);
    return 0;
}

int CBlender::SetRotate(BL_ROTATE rot, bool hflip, bool vflip)
{
    m_Ctrl.rot = rot;
    m_Ctrl.hflip = hflip;
    m_Ctrl.vflip = vflip;
    SetFlag(F_ROTATE);
    return 0;
}

int CBlender::SetBlend(BL_OP_TYPE op, bool premultiplied)
{
    m_Ctrl.op = op;
    m_Ctrl.premultiplied = premultiplied;
    SetFlag(F_BLEND);
    return 0;
}

int CBlender::SetGlobalAlpha(bool enable, unsigned char g_alpha)
{
    m_Ctrl.global_alpha.enable = enable;
    m_Ctrl.global_alpha.val = g_alpha;
    SetFlag(F_GALPHA);
    return 0;
}

int CBlender::SetDither(bool enable)
{
    m_Ctrl.dither = enable;
    SetFlag(F_DITHER);
    return 0;
}

int CBlender::SetBluescreen(BL_BLUESCREEN mode, uint32_t bg_color, uint32_t bs_color)
{
    m_Ctrl.bluescreen.mode = mode;
    m_Ctrl.bluescreen.bg_color = bg_color;
    m_Ctrl.bluescreen.bs_color = bs_color;
    SetFlag(F_BLUSCR);
    return 0;
}

int CBlender::SetScale(BL_SCALE mode,
        uint32_t src_w, uint32_t dst_w, uint32_t src_h, uint32_t dst_h)
{
    if (mode && (!src_w || !dst_w || !src_h || !dst_h)) {
        BL_LOGE("Invalid zero scale ratio: %d %d %d %d",
                src_w, dst_w, src_h, dst_h);
            return -1;
    }

    m_Ctrl.scale.mode = mode;
    m_Ctrl.scale.src_w = src_w;
    m_Ctrl.scale.dst_w = dst_w;
    m_Ctrl.scale.src_h = src_h;
    m_Ctrl.scale.dst_h = dst_h;

    SetFlag(F_SCALE);
    return 0;
}

int CBlender::SetRepeat(BL_REPEAT mode)
{
    m_Ctrl.repeat = mode;
    SetFlag(F_REPEAT);
    return 0;
}

int CBlender::SetClipRect(
        bool enable,
        uint32_t x,
        uint32_t y,
        uint32_t width,
        uint32_t height)
{
    m_Ctrl.clip.enable = enable;
    m_Ctrl.clip.x = x;
    m_Ctrl.clip.y = y;
    m_Ctrl.clip.width  = width;
    m_Ctrl.clip.height = height;

    SetFlag(F_CLIP);
    return 0;
}

int CBlender::SetCscSpec(bool enable, enum v4l2_colorspace space, bool wide)
{
    if (enable) {
        BL_LOGD("Following user-defined csc mode (enable=true)\n");
    } else {
        BL_LOGD("Following driver default csc mode (enable=false)\n");
    }

    if (space != V4L2_COLORSPACE_SMPTE170M && space != V4L2_COLORSPACE_REC709) {
        BL_LOGE("Invalid colorspace %d", space);
        return -1;
    }

    m_Ctrl.csc_spec.enable = enable;
    m_Ctrl.csc_spec.space = space;
    m_Ctrl.csc_spec.wide = wide;

    SetFlag(F_CSC_SPEC);
    return 0;
}

int CBlender::SetImageFormat(
        BL_PORT port,
        unsigned int width,unsigned int height,
        unsigned int crop_x, unsigned int crop_y,
        unsigned int crop_width, unsigned int crop_height,
        unsigned int v4l2_colorformat)
{
    m_Frame.port[port].type = (port == SRC) ? SRC_BUFTYPE : DST_BUFTYPE;
    m_Frame.port[port].color_format = v4l2_colorformat;
    m_Frame.port[port].width = width;
    m_Frame.port[port].height = height;
    m_Frame.port[port].crop_x = crop_x;
    m_Frame.port[port].crop_y = crop_y;
    m_Frame.port[port].crop_width = crop_width;
    m_Frame.port[port].crop_height = crop_height;

    SetFlag(F_SRC_FMT + port);
    return 0;
}

int CBlender::SetAddr(
        BL_PORT port,
        void *addr[BL_MAX_PLANES],
        v4l2_memory type)
{
    for (int i = 0; i < BL_MAX_PLANES; i++)
        m_Frame.port[port].addr[i] = addr[i];

    m_Frame.port[port].memory = type;

    SetFlag(F_SRC_MEMORY + port);
    return 0;
}
