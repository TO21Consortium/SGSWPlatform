/*
 * Copyright (C) 2014 The Android Open Source Project
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
 * \file      libscaler-m2m1shot.cpp
 * \brief     source file for Scaler HAL
 * \author    Cho KyongHo <pullip.cho@samsung.com>
 * \date      2014/05/08
 *
 * <b>Revision History: </b>
 * - 2014.05.08 : Cho KyongHo (pullip.cho@samsung.com) \n
 *   Create
 */
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <videodev2.h>

#include <exynos_scaler.h>

#include "libscaler-common.h"
#include "libscaler-m2m1shot.h"

using namespace std;

const char dev_base_name[] = "/dev/m2m1shot_scaler";
#define DEVBASE_NAME_LEN 20

struct PixFormat {
    unsigned int pixfmt;
    char planes;
    char bit_pp[3];
};

const static PixFormat g_pixfmt_table[] = {
    {V4L2_PIX_FMT_RGB32,        1, {32, 0, 0}, },
    {V4L2_PIX_FMT_BGR32,        1, {32, 0, 0}, },
    {V4L2_PIX_FMT_RGB565,       1, {16, 0, 0}, },
    {V4L2_PIX_FMT_RGB555X,      1, {16, 0, 0}, },
    {V4L2_PIX_FMT_RGB444,       1, {16, 0, 0}, },
    {V4L2_PIX_FMT_YUYV,         1, {16, 0, 0}, },
    {V4L2_PIX_FMT_YVYU,         1, {16, 0, 0}, },
    {V4L2_PIX_FMT_UYVY,         1, {16, 0, 0}, },
    {V4L2_PIX_FMT_NV16,         1, {16, 0, 0}, },
    {V4L2_PIX_FMT_NV61,         1, {16, 0, 0}, },
    {V4L2_PIX_FMT_YUV420,       1, {12, 0, 0}, },
    {V4L2_PIX_FMT_YVU420,       1, {12, 0, 0}, },
    {V4L2_PIX_FMT_NV12M,        2, {8, 4, 0}, },
    {V4L2_PIX_FMT_NV21M,        2, {8, 4, 0}, },
    {v4l2_fourcc('V', 'M', '1', '2'), 2, {8, 4, 0}, },
    {V4L2_PIX_FMT_NV12,         1, {12, 0, 0}, },
    {V4L2_PIX_FMT_NV21,         1, {12, 0, 0}, },
    {v4l2_fourcc('N', 'M', '2', '1'), 2, {8, 4, 0}, },
    {V4L2_PIX_FMT_YUV420M,      3, {8, 2, 2}, },
    {V4L2_PIX_FMT_YVU420M,      3, {8, 2, 2}, },
    {V4L2_PIX_FMT_NV24,         1, {24, 0, 0}, },
    {V4L2_PIX_FMT_NV42,         1, {24, 0, 0}, },
};


CScalerM2M1SHOT::CScalerM2M1SHOT(int devid, int drm) : m_iFD(-1)
{
    char devname[DEVBASE_NAME_LEN + 2]; // basenamelen + id + null

    if ((devid < 0) || (devid > 4)) { // instance number must be between 0 ~ 3
        SC_LOGE("Invalid device instance ID %d", devid);
        return;
    }

    strncpy(devname, dev_base_name, DEVBASE_NAME_LEN);
    devname[DEVBASE_NAME_LEN] = devid + '0';
    devname[DEVBASE_NAME_LEN + 1] = '\0';

    memset(&m_task, 0, sizeof(m_task));

    m_iFD = open(devname, O_RDWR);
    if (m_iFD < 0) {
        SC_LOGERR("Failed to open '%s'", devname);
    } else {
        // default 3 planes not to miss any buffer address
        m_task.buf_out.num_planes = 3;
        m_task.buf_cap.num_planes = 3;
    }
}

CScalerM2M1SHOT::~CScalerM2M1SHOT()
{
    if (m_iFD >= 0)
        close(m_iFD);
}

bool CScalerM2M1SHOT::CScalerM2M1SHOT::Run()
{
    int ret;

    ret = ioctl(m_iFD, M2M1SHOT_IOC_PROCESS, &m_task);
    if (ret < 0) {
        SC_LOGERR("Failed to process the given M2M1SHOT task");
        return false;
    }

    return true;
}

bool CScalerM2M1SHOT::SetFormat(m2m1shot_pix_format &fmt, m2m1shot_buffer &buf,
        unsigned int width, unsigned int height, unsigned int v4l2_fmt) {
    const PixFormat *pixfmt = NULL;

    fmt.width = width;
    fmt.height = height;
    fmt.fmt = v4l2_fmt;

    for (size_t i = 0; i < ARRSIZE(g_pixfmt_table); i++) {
        if (g_pixfmt_table[i].pixfmt == v4l2_fmt) {
            pixfmt = &g_pixfmt_table[i];
            break;
        }
    }

    if (!pixfmt) {
        SC_LOGE("Format %#x is not supported", v4l2_fmt);
        return false;
    }

    for (int i = 0; i < pixfmt->planes; i++) {
        if (((pixfmt->bit_pp[i] * width) % 8) != 0) {
            SC_LOGE("Plane %d of format %#x must have even width", i, v4l2_fmt);
            return false;
        }
        buf.plane[i].len = (pixfmt->bit_pp[i] * width * height) / 8;
    }

    buf.num_planes = pixfmt->planes;

    return true;
}

bool CScalerM2M1SHOT::SetCrop(m2m1shot_pix_format &fmt,
        unsigned int l, unsigned int t, unsigned int w, unsigned int h) {
    if (fmt.width <= l) {
        SC_LOGE("crop left %d is larger than image width %d", l, fmt.width);
        return false;
    }
    if (fmt.height <= t) {
        SC_LOGE("crop top %d is larger than image height %d", t, fmt.height);
        return false;
    }
    if (fmt.width < (l + w)) {
        SC_LOGE("crop width %d@%d  exceeds image width %d", w, l, fmt.width);
        return false;
    }
    if (fmt.height < (t + h)) {
        SC_LOGE("crop height %d@%d  exceeds image height %d", h, t, fmt.height);
        return false;
    }

    fmt.crop.left = l;
    fmt.crop.top = t;
    fmt.crop.width = w;
    fmt.crop.height = h;

    return true;
}

bool CScalerM2M1SHOT::SetAddr(
                m2m1shot_buffer &buf, void *addr[SC_NUM_OF_PLANES], int mem_type) {
    if (mem_type == V4L2_MEMORY_DMABUF) {
        buf.type = M2M1SHOT_BUFFER_DMABUF;
        for (int i = 0; i < buf.num_planes; i++)
            buf.plane[i].fd = reinterpret_cast<int>(addr[i]);
    } else if (mem_type == V4L2_MEMORY_USERPTR) {
        buf.type = M2M1SHOT_BUFFER_USERPTR;
        for (int i = 0; i < buf.num_planes; i++)
            buf.plane[i].userptr = reinterpret_cast<unsigned long>(addr[i]);
    } else {
        SC_LOGE("Unknown buffer type %d", mem_type);
        return false;
    }

    return true;
}

bool CScalerM2M1SHOT::SetRotate(int rot, int hflip, int vflip) {
    if ((rot % 90) != 0) {
        SC_LOGE("Rotation degree %d must be multiple of 90", rot);
        return false;
    }

    rot = rot % 360;
    if (rot < 0)
        rot = 360 + rot;

    m_task.op.rotate = rot;
    m_task.op.op &= ~(M2M1SHOT_OP_FLIP_HORI | M2M1SHOT_OP_FLIP_VIRT);
    if (hflip)
        m_task.op.op |= M2M1SHOT_OP_FLIP_HORI;
    if (vflip)
        m_task.op.op |= M2M1SHOT_OP_FLIP_VIRT;

    return true;
}
