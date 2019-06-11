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
 * \file      libg2d.cpp
 * \brief     source file for G2D library
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
#include "videodev2_exynos_media.h"
#include "exynos_v4l2.h"
#include "libg2d_obj.h"

void CFimg2d::Initialize(BL_DEVID devid, bool nonblock)
{
    m_iDeviceID = devid;
    int dev_num = m_iDeviceID - DEV_G2D0;

    snprintf(m_cszNode, BL_MAX_NODENAME, G2D_DEV_NODE "%d", G2D_NODE(dev_num));

    m_fdBlender = exynos_v4l2_open(m_cszNode,
            nonblock ? (O_RDWR | O_NONBLOCK) : (O_RDWR));
    if (m_fdBlender < 0) {
        BL_LOGERR("Failed to open '%s'", m_cszNode);
        return;
    }

    unsigned int cap = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_OUTPUT_MPLANE |
                       V4L2_CAP_VIDEO_CAPTURE_MPLANE;
    if (!exynos_v4l2_querycap(m_fdBlender, cap)) {
        BL_LOGERR("Failed to query capabilities on '%s'", m_cszNode);
        close(m_fdBlender);
        m_fdBlender = -1;
    } else {
        m_fdValidate = -m_fdBlender;
    }
}

void CFimg2d::ResetPort(BL_PORT port)
{
    BL_LOGD("Current m_Flags 0x%lx\n", m_Flags);

    if (IsFlagSet(F_SRC_QBUF + port))
        DQBuf(static_cast<BL_PORT>(port));

    if (IsFlagSet(F_SRC_STREAMON + port)) {
        if (exynos_v4l2_streamoff(m_fdBlender, m_Frame.port[port].type)) {
            BL_LOGERR("Failed STREAMOFF for the %s", m_cszPortName[port]);
	} else {
            BL_LOGD("VIDIC_STREAMOFF is successful for the %s", m_cszPortName[port]);
	}
    }

    if (IsFlagSet(F_SRC_REQBUFS + port)) {
        v4l2_requestbuffers reqbufs;
        memset(&reqbufs, 0, sizeof(reqbufs));
        reqbufs.type = m_Frame.port[port].type;
        reqbufs.memory = m_Frame.port[port].memory;
        if (exynos_v4l2_reqbufs(m_fdBlender, &reqbufs)) {
            BL_LOGERR("Failed REQBUFS(0) for the %s", m_cszPortName[port]);
	} else {
            BL_LOGD("VIDIC_REQBUFS(0) is successful for the %s", m_cszPortName[port]);
	}
    }

    if (port == SRC && IsFlagSet(F_FILL) && IsFlagSet(F_SRC_MEMORY)) {
        for (int i = 0; i < G2D_NUM_OF_PLANES; i++) {
            if (m_Frame.port[SRC].addr[i]) {
                free(m_Frame.port[SRC].addr[i]);
                BL_LOGD("Succeeded free for source buffer %d plane\n", port);
            }
        }
    }
}

CFimg2d::CFimg2d(BL_DEVID devid, bool nonblock)
{
    Initialize(devid, nonblock);
    if (Valid()) {
        BL_LOGD("Succeeded opened '%s'. fd %d", m_cszNode, m_fdBlender);
    }
}

CFimg2d::~CFimg2d()
{
    if (m_fdBlender >= 0)
        close(m_fdBlender);
    m_fdBlender = -1;
}

int CFimg2d::SetCtrl()
{
    if (!IsFlagSet(F_CTRL_ANY)) {
        BL_LOGD("Skipped S_CTRL by No change");
        return 0;
    }

    int ret;

    if (IsFlagSet(F_FILL)) {
        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_COLOR_FILL, m_Ctrl.fill.enable);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_2D_COLOR_FILL");
            goto err;
        }
        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_SRC_COLOR, m_Ctrl.fill.color_argb8888);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_2D_SRC_COLOR");
            goto err;
        }
    }

    if (IsFlagSet(F_ROTATE)) {
        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_ROTATE, m_Ctrl.rot);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_ROTATE");
            goto err;
        }
        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_HFLIP, m_Ctrl.hflip);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_HFLIP");
            goto err;
        }
        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_VFLIP, m_Ctrl.vflip);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_VFLIP");
            goto err;
        }
    }

    if (IsFlagSet(F_BLEND)) {
        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_BLEND_OP, m_Ctrl.op);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_2D_BLEND_OP");
            goto err;
        }
        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_FMT_PREMULTI, m_Ctrl.premultiplied);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_2D_FMT_PREMULTI");
            goto err;
        }
    }

    if (IsFlagSet(F_GALPHA)) {
        if (m_Ctrl.global_alpha.enable) {
            ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_GLOBAL_ALPHA, m_Ctrl.global_alpha.val);
            if (ret) {
                BL_LOGERR("Failed S_CTRL V4L2_CID_GLOBAL_ALPHA");
                goto err;
            }
        } else {
            ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_GLOBAL_ALPHA, 0xff);
            if (ret) {
                BL_LOGERR("Failed S_CTRL V4L2_CID_GLOBAL_ALPHA 0xff");
                goto err;
            }
        }
    }

    if (IsFlagSet(F_DITHER)) {
        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_DITH, m_Ctrl.dither);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_2D_DITH");
            goto err;
        }
    }

    if (IsFlagSet(F_BLUSCR)) {
        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_BLUESCREEN, m_Ctrl.bluescreen.mode);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_2D_BLUESCREEN");
            goto err;
        }
        if (m_Ctrl.bluescreen.mode) {
            ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_BG_COLOR, m_Ctrl.bluescreen.bg_color);
            if (ret) {
                BL_LOGERR("Failed S_CTRL V4L2_CID_2D_BG_COLOR");
                goto err;
            }
        }
        if (m_Ctrl.bluescreen.mode == BLUSCR) {
            ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_BS_COLOR, m_Ctrl.bluescreen.bs_color);
            if (ret) {
                BL_LOGERR("Failed S_CTRL V4L2_CID_2D_BS_COLOR");
                goto err;
            }
        }
    }

    if (IsFlagSet(F_SCALE)) {
        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_SCALE_MODE, m_Ctrl.scale.mode);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_2D_SCALE_MODE");
            goto err;
        }

        int Wratio = m_Ctrl.scale.src_w << 16 | m_Ctrl.scale.dst_w;
        int Hratio = m_Ctrl.scale.src_h << 16 | m_Ctrl.scale.dst_h;

        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_SCALE_WIDTH, Wratio);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_2D_SCALE_WIDTH");
            goto err;
        }

        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_SCALE_HEIGHT, Hratio);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_2D_SCALE_HEIGHT");
            goto err;
        }
    }

    if (IsFlagSet(F_REPEAT)) {
        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_REPEAT, m_Ctrl.repeat);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_2D_REPEAT");
            goto err;
        }
    }

    if (IsFlagSet(F_CLIP)) {
        int val = 0;
        v4l2_rect clip_rect;

        if (m_Ctrl.clip.enable) {
            clip_rect.left = m_Ctrl.clip.x;
            clip_rect.top  = m_Ctrl.clip.y;
            clip_rect.width  = m_Ctrl.clip.width;
            clip_rect.height = m_Ctrl.clip.height;

            val = reinterpret_cast<int>(&clip_rect);
        }

        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_CLIP, val);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_2D_CLIP");
            goto err;
        }
    }

    if (IsFlagSet(F_CSC_SPEC)) {
        ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_CSC_EQ_MODE, m_Ctrl.csc_spec.enable);
        if (ret) {
            BL_LOGERR("Failed S_CTRL V4L2_CID_CSC_EQ_MODE");
            goto err;
        }

        if (m_Ctrl.csc_spec.enable) {
            bool is_bt709 = (m_Ctrl.csc_spec.space == V4L2_COLORSPACE_REC709)? true : false;
            ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_CSC_EQ, is_bt709);
            if (ret) {
                BL_LOGERR("Failed S_CTRL V4L2_CID_CSC_EQ");
                goto err;
            }
            ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_CSC_RANGE, m_Ctrl.csc_spec.wide);
            if (ret) {
                BL_LOGERR("Failed S_CTRL V4L2_CID_CSC_RANGE");
                goto err;
            }
        }
    }

    BL_LOGD("Succeeded S_CTRL flags(0x%lx)\n", m_Flags);
    return 0;

err:
    BL_LOGE("Failed S_CTRL flags(0x%lx)\n", m_Flags);
    return ret;
}

int CFimg2d::SetFormat()
{
    if (!IsFlagSet(F_DST_FMT)) {
        BL_LOGE("No found destination foramt\n");
        return -1;
    }

    if (!IsFlagSet(F_FILL) && !IsFlagSet(F_SRC_FMT)) {
        BL_LOGE("No found source foramt\n");
        return -1;
    }

    //! dummy min buffer(16x16) for src color fill
    if (IsFlagSet(F_FILL)) {
        m_Frame.port[SRC].type = SRC_BUFTYPE;
        m_Frame.port[SRC].color_format = m_Frame.port[DST].color_format;
        m_Frame.port[SRC].width  = 16;
        m_Frame.port[SRC].height = 16;
        m_Frame.port[SRC].crop_x = 0;
        m_Frame.port[SRC].crop_y = 0;
        m_Frame.port[SRC].crop_width  = m_Frame.port[SRC].width;
        m_Frame.port[SRC].crop_height = m_Frame.port[SRC].height;
        SetFlag(F_SRC_FMT);

        for (int i = 0; i < G2D_NUM_OF_PLANES; i++) {
            m_Frame.port[SRC].addr[i] = calloc(1, 16 * 16 * 4);
            if (m_Frame.port[SRC].addr[i])
                BL_LOGD("Succeeded alloc for source color %d plane\n", i);
            else {
                BL_LOGE("Failed alloc for source color %d plane\n", i);
                return -1;
            }
        }

        m_Frame.port[SRC].memory = V4L2_MEMORY_USERPTR;

        SetFlag(F_SRC_MEMORY);
    }

    for (int port = 0; port < NUM_PORTS; port++) {
        v4l2_format fmt;
        fmt.type = m_Frame.port[port].type;
        fmt.fmt.pix_mp.pixelformat = m_Frame.port[port].color_format;
        fmt.fmt.pix_mp.width  = m_Frame.port[port].width;
        fmt.fmt.pix_mp.height = m_Frame.port[port].height;

        if (exynos_v4l2_s_fmt(m_fdBlender, &fmt) < 0) {
            BL_LOGE("Failed S_FMT(fmt: %d, w:%d, h:%d) for the %s",
                    fmt.fmt.pix_mp.pixelformat, fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
                    m_cszPortName[port]);
            return -1;
        }

        // returned fmt.fmt.pix_mp.num_planes and fmt.fmt.pix_mp.plane_fmt[i].sizeimage
        m_Frame.port[port].out_num_planes = fmt.fmt.pix_mp.num_planes;

        for (int i = 0; i < m_Frame.port[port].out_num_planes; i++)
            m_Frame.port[port].out_plane_size[i] = fmt.fmt.pix_mp.plane_fmt[i].sizeimage;

        v4l2_crop crop;
        crop.type = m_Frame.port[port].type;
        crop.c.left = m_Frame.port[port].crop_x;
        crop.c.top  = m_Frame.port[port].crop_y;
        crop.c.width  = m_Frame.port[port].crop_width;
        crop.c.height = m_Frame.port[port].crop_height;

        if (exynos_v4l2_s_crop(m_fdBlender, &crop) < 0) {
            BL_LOGE("Failed S_CROP(fmt: %d, l:%d, t:%d, w:%d, h:%d) for the %s",
                    crop.type, crop.c.left, crop.c.top, crop.c.width, crop.c.height,
                    m_cszPortName[port]);
            return -1;
        }

        if (m_Frame.port[port].out_num_planes > G2D_NUM_OF_PLANES) {
            BL_LOGE("Number of planes exceeds %d", m_Frame.port[port].out_num_planes);
            return -1;
        }

        BL_LOGD("Succeeded S_FMT and S_CROP for the %s", m_cszPortName[port]);
    }

    return 0;
}

int CFimg2d::ReqBufs()
{
    v4l2_requestbuffers reqbufs;

    for (int port = 0; port < NUM_PORTS; port++) {
        if (!IsFlagSet(F_SRC_FMT + port)) {
            BL_LOGE("No found format for the %s", m_cszPortName[port]);
            return -1;
        }
        if (!IsFlagSet(F_SRC_MEMORY + port)) {
            BL_LOGE("No found buffer for the %s", m_cszPortName[port]);
            return -1;
        }

        memset(&reqbufs, 0, sizeof(reqbufs));

        reqbufs.type    = m_Frame.port[port].type;
        reqbufs.memory  = m_Frame.port[port].memory;
        reqbufs.count   = 1;

        if (exynos_v4l2_reqbufs(m_fdBlender, &reqbufs) < 0) {
            BL_LOGE("Failed REQBUFS for the %s", m_cszPortName[port]);
            return -1;
        }

        BL_LOGD("Succeeded REQBUFS for the %s", m_cszPortName[port]);
        SetFlag(F_SRC_REQBUFS + port);
    }

    return 0;
}

int CFimg2d::QBuf()
{
    v4l2_buffer buffer;
    v4l2_plane  planes[G2D_NUM_OF_PLANES];

    for (int port = 0; port < NUM_PORTS; port++) {
        if (!IsFlagSet(F_SRC_REQBUFS + port)) {
            BL_LOGE("No found reqbufs for the %s", m_cszPortName[port]);
            return -1;
        }

        memset(&buffer, 0, sizeof(buffer));
        memset(&planes, 0, sizeof(planes));

        buffer.type   = m_Frame.port[port].type;
        buffer.memory = m_Frame.port[port].memory;
        buffer.index  = 0;
        buffer.length = m_Frame.port[port].out_num_planes;

        buffer.m.planes = planes;
        for (unsigned long i = 0; i < buffer.length; i++) {
            planes[i].length = m_Frame.port[port].out_plane_size[i];
            if (V4L2_TYPE_IS_OUTPUT(buffer.type))
                planes[i].bytesused = planes[i].length;
            if (buffer.memory == V4L2_MEMORY_DMABUF)
                planes[i].m.fd = reinterpret_cast<int>(m_Frame.port[port].addr[i]);
            else
                planes[i].m.userptr = reinterpret_cast<unsigned long>(m_Frame.port[port].addr[i]);
        }

        if (exynos_v4l2_qbuf(m_fdBlender, &buffer) < 0) {
            BL_LOGE("Failed QBUF for the %s", m_cszPortName[port]);
            return -1;
        }

        BL_LOGD("Succeeded QBUF for the %s", m_cszPortName[port]);
        SetFlag(F_SRC_QBUF + port);
    }

    return 0;
}

int CFimg2d::DQBuf(BL_PORT port)
{
    v4l2_buffer buffer;
    v4l2_plane  plane[G2D_NUM_OF_PLANES];

    if (!IsFlagSet(F_SRC_QBUF + port)) {
        BL_LOGE("No found queued buffer for the %s", m_cszPortName[port]);
        return -1;
    }

    memset(&buffer, 0, sizeof(buffer));

    buffer.type   = m_Frame.port[port].type;
    buffer.memory = m_Frame.port[port].memory;

    if (V4L2_TYPE_IS_MULTIPLANAR(buffer.type)) {
        memset(plane, 0, sizeof(plane));

        buffer.length   = m_Frame.port[port].out_num_planes;
        buffer.m.planes = plane;
    }

    if (exynos_v4l2_dqbuf(m_fdBlender, &buffer) < 0 ) {
        BL_LOGE("Failed DQBuf the %s", m_cszPortName[port]);
        return -1;
    }

    if (buffer.flags & V4L2_BUF_FLAG_ERROR) {
        BL_LOGE("Error occurred while processing streaming data");
        return -1;
    }

    ClearFlag(F_SRC_QBUF + port);
    BL_LOGD("Succeeded VIDIOC_DQBUF for the %s", m_cszPortName[port]);
    return 0;
}

int CFimg2d::DQBuf()
{
    for (int port = 0; port < NUM_PORTS; port++) {
        if (DQBuf(static_cast<BL_PORT>(port)))
            return -1;
    }
    return 0;
}

int CFimg2d::StreamOn()
{
    for (int port = 0; port < NUM_PORTS; port++) {
        if (exynos_v4l2_streamon(m_fdBlender, m_Frame.port[port].type)) {
            BL_LOGE("Failed StreamOn for the %s", m_cszPortName[port]);
            return errno;
        }

        SetFlag(F_SRC_STREAMON + port);
        BL_LOGD("Succeeded VIDIOC_STREAMON for the %s", m_cszPortName[port]);
    }
    return 0;
}

int CFimg2d::DoStart()
{
    int ret;

#ifdef BL_DEBUG
    DebugParam();
#endif

    ret = SetCtrl();
    if (ret)
        goto err;

    ret = SetFormat();
    if (ret)
        goto err;

    ret = ReqBufs();
    if (ret)
        goto err;

    ret = QBuf();
    if (ret)
        goto err;

    ret = StreamOn();
    if (ret)
        goto err;

    ret = DQBuf();
    if (ret)
        goto err;

    return 0;

err:
    DebugParam();
    DoStop();
    return ret;
}

int CFimg2d::DoStop()
{
    for (int port = 0; port < NUM_PORTS; port++)
        ResetPort(static_cast<BL_PORT>(port));

    ResetFlag();
    return 0;
}

int CFimg2d::Deactivate(bool deact)
{
    int ret = exynos_v4l2_s_ctrl(m_fdBlender, V4L2_CID_2D_DEACTIVATE, deact);
    if (ret) {
        BL_LOGERR("Failed S_CTRL V4L2_CID_2D_DEACTIVATE");
        return ret;
    }

    BL_LOGERR("Succeeded S_CTRL V4L2_CID_2D_DEACTIVATE");
    return 0;
}

void CFimg2d::DebugParam()
{
    const char *port[2] = { "SRC", "DST" };

    BL_LOGE("G2D HAL Parameter Debug!!\n");
    ALOGE("Handle %p DeviceId %d\n", this, GetDeviceID());
    for (int i = 0; i < NUM_PORTS; i++) {
        ALOGE("[%s] V4L2 Buf Type %d\n", port[i], m_Frame.port[i].type);
        ALOGE("[%s] Full WxH %d x %d\n", port[i],
                m_Frame.port[i].width, m_Frame.port[i].height);
        ALOGE("[%s] Crop(XYWH) %d, %d, %d, %d\n", port[i],
                m_Frame.port[i].crop_x,
                m_Frame.port[i].crop_y,
                m_Frame.port[i].crop_width,
                m_Frame.port[i].crop_height);
        ALOGE("[%s] Format %d\n", port[i], m_Frame.port[i].color_format);
        ALOGE("[%s] Addr [0] %p [1] %p\n", port[i],
                m_Frame.port[i].addr[0],
                m_Frame.port[i].addr[1]);
        ALOGE("[%s] Addr Type %d\n", port[i], m_Frame.port[i].memory);
        ALOGE("[%s] out_num_planes %d\n", port[i], m_Frame.port[i].out_num_planes);
        ALOGE("[%s] out_planes_size [0] %ld [1] %ld\n", port[i],
                m_Frame.port[i].out_plane_size[0],
                m_Frame.port[i].out_plane_size[1]);
    }
    ALOGE("Flags 0x%lx\n", m_Flags);
    ALOGE("fill: enable %d color 0x%x\n",
            m_Ctrl.fill.enable, m_Ctrl.fill.color_argb8888);
    ALOGE("rotate: %d hflip: %d vflip: %d\n",
            m_Ctrl.rot, m_Ctrl.hflip, m_Ctrl.vflip);
    ALOGE("blend: %d premultiplied: %d\n",
            m_Ctrl.op, m_Ctrl.premultiplied);
    ALOGE("galpha: enable %d val 0x%x\n",
            m_Ctrl.global_alpha.enable, m_Ctrl.global_alpha.val);
    ALOGE("dither: enable %d\n", m_Ctrl.dither);
    ALOGE("bluescreen: mode %d bgcolor 0x%x bscolor 0x%x\n",
            m_Ctrl.bluescreen.mode,
            m_Ctrl.bluescreen.bg_color,
            m_Ctrl.bluescreen.bs_color);
    ALOGE("scale: mode %d Width(src:dst %d, %d) Height(src:dst %d, %d)\n",
            m_Ctrl.scale.mode,
            m_Ctrl.scale.src_w, m_Ctrl.scale.dst_w,
            m_Ctrl.scale.src_h, m_Ctrl.scale.dst_h);
    ALOGE("repeat: mode %d\n", m_Ctrl.repeat);
    ALOGE("clip: enable %d Clip(XYWH) %d, %d, %d %d\n", m_Ctrl.clip.enable,
            m_Ctrl.clip.x, m_Ctrl.clip.y, m_Ctrl.clip.width, m_Ctrl.clip.height);
    ALOGE("csc spec: enable %d space %d wide %d\n",
            m_Ctrl.csc_spec.enable,
            m_Ctrl.csc_spec.space,
            m_Ctrl.csc_spec.wide);
}

