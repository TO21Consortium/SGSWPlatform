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
 * \file      exynos_blender_obj.h
 * \brief     Class definition for Exynos Blender library
 * \author    Eunseok Choi (es10.choi@samsung.com)
 * \date      2013/09/21
 *
 * <b>Revision History: </b>
 * - 2013.09.21 : Eunseok Choi (eunseok.choi@samsung.com) \n
 *   Create
 *
 */
#ifndef __EXYNOS_BLENDER_OBJ_H__
#define __EXYNOS_BLENDER_OBJ_H__

#include <cstring>
#include <cstdlib>
#include <system/graphics.h>
#include <cutils/log.h>
#include "exynos_blender.h"

#define BL_LOGERR(fmt, args...) \
    ((void)ALOG(LOG_ERROR, LOG_TAG, "%s: " fmt " [%s]", __func__, ##args, strerror(errno)))
#define BL_LOGE(fmt, args...)   \
    ((void)ALOG(LOG_ERROR, LOG_TAG, "%s: " fmt, __func__, ##args))

//#define BL_DEBUG

#ifdef BL_DEBUG
#define BL_LOGD(args...)        ((void)ALOG(LOG_INFO, LOG_TAG, ##args))
#else
#define BL_LOGD(args...)
#endif

#define UNIMPL          { BL_LOGE("Unimplemented Operation %p\n", this); return -1; }

#define SRC_BUFTYPE     V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE
#define DST_BUFTYPE     V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE

class CBlender {
public:
    enum BL_PORT { SRC, DST, NUM_PORTS };

protected:
    enum { BL_MAX_NODENAME = 14 };
    enum BL_FLAGS {
        F_FILL,
        F_ROTATE,
        F_BLEND,
        F_GALPHA,
        F_DITHER,
        F_BLUSCR,
        F_SCALE,
        F_REPEAT,
        F_CLIP,
        F_CSC_SPEC,
        F_CTRL_ANY = 10,

        F_SRC_FMT,
        F_DST_FMT,
        F_SRC_MEMORY,
        F_DST_MEMORY,

        F_SRC_REQBUFS,
        F_DST_REQBUFS,
        F_SRC_QBUF,
        F_DST_QBUF,
        F_SRC_STREAMON,
        F_DST_STREAMON,

        F_FLAGS_END = 32
    };

    struct BL_FrameInfo {
        struct {
            v4l2_buf_type type;
            uint32_t width, height;
            uint32_t crop_x, crop_y, crop_width, crop_height;
            uint32_t color_format;
            void *addr[BL_MAX_PLANES];
            v4l2_memory memory;
            int out_num_planes;
            unsigned long out_plane_size[BL_MAX_PLANES];
        } port[NUM_PORTS];
    } m_Frame;

    struct BL_Control {
        struct {
            bool enable;
            uint32_t color_argb8888;
        } fill;
        BL_ROTATE rot;
        bool hflip;
        bool vflip;
        BL_OP_TYPE op;
        bool premultiplied;
        struct {
            bool enable;
            unsigned char val;
        } global_alpha;
        bool dither;
        struct {
            BL_BLUESCREEN mode;
            uint32_t bg_color;
            uint32_t bs_color;
        } bluescreen;
        struct {
            BL_SCALE mode;
            uint32_t src_w;
            uint32_t dst_w;
            uint32_t src_h;
            uint32_t dst_h;
        } scale;
        BL_REPEAT repeat;
        struct {
            bool enable;
            uint32_t x;
            uint32_t y;
            uint32_t width;
            uint32_t height;
        } clip;
        struct {
            bool enable;    // set 'true' for user-defined
            enum v4l2_colorspace space;
            bool wide;
        } csc_spec;
    } m_Ctrl;
    unsigned long m_Flags;

    int m_fdBlender;
    int m_iDeviceID;
    int m_fdValidate;

    char m_cszNode[BL_MAX_NODENAME];    // /dev/videoXX
    static const char *m_cszPortName[NUM_PORTS];

    inline void SetFlag(int f) { m_Flags |= (1 << f); }
    inline void ResetFlag(void) { m_Flags = 0; }

    inline bool IsFlagSet(int f)
    {
        if (f == F_CTRL_ANY)
            return m_Flags & ((1 << f) - 1);
        else
            return m_Flags & (1 << f);
    }

    inline void ClearFlag(int f)
    {
        if (f == F_CTRL_ANY) {
            m_Flags &= ~((1 << f) - 1);
        } else {
            m_Flags &= ~(1 << f);
        }
    }

public:
    CBlender()
    {
        m_fdBlender = -1;
        m_iDeviceID = -1;
        memset(&m_Frame, 0, sizeof(m_Frame));
        memset(&m_Ctrl, 0, sizeof(m_Ctrl));
        m_Flags = 0;
        m_fdValidate = 0;
        memset(m_cszNode, 0, sizeof(m_cszNode));
    }
    virtual ~CBlender() {};

    bool Valid() { return (m_fdBlender >= 0) && (m_fdBlender == -m_fdValidate); }
    int GetDeviceID() { return m_iDeviceID; }

    int SetColorFill(bool enable, uint32_t color_argb8888);
    int SetRotate(BL_ROTATE rot, bool hflip, bool vflip);
    int SetBlend(BL_OP_TYPE op, bool premultiplied);
    int SetGlobalAlpha(bool enable, unsigned char g_alpha);
    int SetDither(bool enable);
    int SetBluescreen(BL_BLUESCREEN mode, uint32_t bg_color, uint32_t bs_color = 0);
    int SetScale(BL_SCALE mode, uint32_t src_w, uint32_t dst_w, uint32_t src_h, uint32_t dst_h);
    int SetRepeat(BL_REPEAT mode);
    int SetClipRect(bool enable, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    int SetCscSpec(bool enable, enum v4l2_colorspace space, bool wide);

    int SetAddr(BL_PORT port, void *addr[BL_MAX_PLANES], v4l2_memory type);
    int SetImageFormat(BL_PORT port, unsigned int width, unsigned int height,
                       unsigned int crop_x, unsigned int crop_y,
                       unsigned int crop_width, unsigned int crop_height,
                       unsigned int v4l2_colorformat);

    virtual int DoStart() = 0;
    virtual int DoStop() = 0;
    virtual int Deactivate(bool deact) UNIMPL;
};

#endif // __EXYNOS_BLENER_OBJ_H__
