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
 * \file      libscaler-v4l2.cpp
 * \brief     source file for Scaler HAL
 * \author    Cho KyongHo <pullip.cho@samsung.com>
 * \date      2014/05/12
 *
 * <b>Revision History: </b>
 * - 2014.05.12 : Cho KyongHo (pullip.cho@samsung.com) \n
 *   Create
 */

#include <cstring>
#include <cstdlib>

#include "libscaler-v4l2.h"

void CScalerV4L2::Initialize(int instance)
{
    snprintf(m_cszNode, SC_MAX_NODENAME, SC_DEV_NODE "%d", SC_NODE(instance));

    m_fdScaler = exynos_v4l2_open(m_cszNode, O_RDWR);
    if (m_fdScaler < 0) {
        SC_LOGERR("Failed to open '%s'", m_cszNode);
        return;
    }

    unsigned int cap = V4L2_CAP_STREAMING | V4L2_CAP_VIDEO_OUTPUT_MPLANE |
                        V4L2_CAP_VIDEO_CAPTURE_MPLANE;
    if (!exynos_v4l2_querycap(m_fdScaler, cap)) {
        SC_LOGERR("Failed to query capture on '%s'", m_cszNode);
        close(m_fdScaler);
        m_fdScaler = -1;
    } else {
        m_fdValidate = -m_fdScaler;
    }
}

CScalerV4L2::CScalerV4L2(int instance, int allow_drm)
{
    m_fdScaler = -1;
    m_iInstance = instance;
    m_nRotDegree = 0;
    m_fStatus = 0;

    memset(&m_frmSrc, 0, sizeof(m_frmSrc));
    memset(&m_frmDst, 0, sizeof(m_frmDst));

    m_frmSrc.fdAcquireFence = -1;
    m_frmDst.fdAcquireFence = -1;

    m_frmSrc.name = "output";
    m_frmDst.name = "capture";

    m_frmSrc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    m_frmDst.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;

    Initialize(instance);

    if(Valid()) {
        if (allow_drm)
            SetFlag(m_fStatus, SCF_ALLOW_DRM);
        SC_LOGD("Successfully opened '%s'; returned fd %d; drmmode %s",
                m_cszNode, m_fdScaler, allow_drm ? "enabled" : "disabled");
    }
}

CScalerV4L2::~CScalerV4L2()
{
    if (m_fdScaler >= 0)
        close(m_fdScaler);

    m_fdScaler = -1;
}

bool CScalerV4L2::Stop()
{
    if (!ResetDevice(m_frmSrc)) {
        SC_LOGE("Failed to stop Scaler for the output frame");
        return false;
    }

    if (!ResetDevice(m_frmDst)) {
        SC_LOGE("Failed to stop Scaler for the cature frame");
        return false;
    }

    return true;
}

bool CScalerV4L2::Run()
{
    if (!DevSetCtrl())
        return false;

    if (!DevSetFormat())
        return false;

    if (!ReqBufs())
        return false;

    if (!StreamOn())
        return false;

    if (!QBuf())
        return false;

    return DQBuf();
}

bool CScalerV4L2::DevSetCtrl()
{
    if (!TestFlag(m_fStatus, SCF_ROTATION_FRESH)) {
        SC_LOGD("Skipping rotation and flip setting due to no change");
        return true;
    }

    if (!Stop())
        return false;

    if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_ROTATE, m_nRotDegree) < 0) {
        SC_LOGERR("Failed V4L2_CID_ROTATE with degree %d", m_nRotDegree);
        return false;
    }

    if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_VFLIP, TestFlag(m_fStatus, SCF_HFLIP)) < 0) {
        SC_LOGERR("Failed V4L2_CID_VFLIP - %d", TestFlag(m_fStatus, SCF_VFLIP));
        return false;
    }

    if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_HFLIP, TestFlag(m_fStatus, SCF_VFLIP)) < 0) {
        SC_LOGERR("Failed V4L2_CID_HFLIP - %d", TestFlag(m_fStatus, SCF_HFLIP));
        return false;
    }

    SC_LOGD("Successfully set CID_ROTATE(%d), CID_VFLIP(%d) and CID_HFLIP(%d)",
            m_nRotDegree, TestFlag(m_fStatus, SCF_VFLIP), TestFlag(m_fStatus, SCF_HFLIP));

    ClearFlag(m_fStatus, SCF_ROTATION_FRESH);

    if (TestFlag(m_fStatus, SCF_CSC_FRESH) && TestFlag(m_fStatus, SCF_CSC_WIDE)) {
        if (exynos_v4l2_s_ctrl(m_fdScaler, V4L2_CID_CSC_RANGE,
                            TestFlag(m_fStatus, SCF_CSC_WIDE) ? 1 : 0) < 0) {
            SC_LOGERR("Failed V4L2_CID_CSC_RANGE to %d", TestFlag(m_fStatus, SCF_CSC_WIDE));
            return false;
        }

        ClearFlag(m_fStatus, SCF_CSC_FRESH);
    }

    return true;
}

bool CScalerV4L2::ResetDevice(FrameInfo &frm)
{
    if (!DQBuf(frm))
        return false;

    if (TestFlag(frm.flags, SCFF_STREAMING)) {
        if (exynos_v4l2_streamoff(m_fdScaler, frm.type) < 0 ) {
            SC_LOGERR("Failed STREAMOFF for the %s", frm.name);
            return false;
        }
        ClearFlag(frm.flags, SCFF_STREAMING);
    }

    SC_LOGD("VIDIC_STREAMOFF is successful for the %s", frm.name);

    if (TestFlag(frm.flags, SCFF_REQBUFS)) {
        v4l2_requestbuffers reqbufs;
        memset(&reqbufs, 0, sizeof(reqbufs));
        reqbufs.type = frm.type;
        reqbufs.memory = frm.memory;
        if (exynos_v4l2_reqbufs(m_fdScaler, &reqbufs) < 0 ) {
            SC_LOGERR("Failed to REQBUFS(0) for the %s", frm.name);
            return false;
        }

        ClearFlag(frm.flags, SCFF_REQBUFS);
    }

    SC_LOGD("VIDIC_REQBUFS(0) is successful for the %s", frm.name);

    return true;
}

bool CScalerV4L2::DevSetFormat(FrameInfo &frm)
{

    if (!TestFlag(frm.flags, SCFF_BUF_FRESH)) {
        SC_LOGD("Skipping S_FMT for the %s since it is already done", frm.name);
        return true;
    }

    if (!ResetDevice(frm)) {
        SC_LOGE("Failed to VIDIOC_S_FMT for the %s", frm.name);
        return false;
    }

    v4l2_format fmt;
    fmt.type = frm.type;
    fmt.fmt.pix_mp.pixelformat = frm.color_format;
    fmt.fmt.pix_mp.width  = frm.width;
    fmt.fmt.pix_mp.height = frm.height;

    if (exynos_v4l2_s_fmt(m_fdScaler, &fmt) < 0) {
        SC_LOGERR("Failed S_FMT(fmt: %d, w:%d, h:%d) for the %s",
                fmt.fmt.pix_mp.pixelformat, fmt.fmt.pix_mp.width, fmt.fmt.pix_mp.height,
                frm.name);
        return false;
    }

    // returned fmt.fmt.pix_mp.num_planes and fmt.fmt.pix_mp.plane_fmt[i].sizeimage
    frm.out_num_planes = fmt.fmt.pix_mp.num_planes;

    for (int i = 0; i < frm.out_num_planes; i++)
        frm.out_plane_size[i] = fmt.fmt.pix_mp.plane_fmt[i].sizeimage;

    v4l2_crop crop;
    crop.type = frm.type;
    crop.c = frm.crop;

    if (exynos_v4l2_s_crop(m_fdScaler, &crop) < 0) {
        SC_LOGERR("Failed S_CROP(fmt: %d, l:%d, t:%d, w:%d, h:%d) for the %s",
                crop.type, crop.c.left, crop.c.top, crop.c.width, crop.c.height,
                frm.name);
        return false;
    }

    if (frm.out_num_planes > SC_MAX_PLANES) {
        SC_LOGE("Number of planes exceeds %d of %s", frm.out_num_planes, frm.name);
        return false;
    }

    ClearFlag(frm.flags, SCFF_BUF_FRESH);

    SC_LOGD("Successfully S_FMT and S_CROP for the %s", frm.name);

    return true;
}

bool CScalerV4L2::DevSetFormat()
{
    if (!DevSetFormat(m_frmSrc))
        return false;

    return DevSetFormat(m_frmDst);
}

bool CScalerV4L2::QBuf(FrameInfo &frm, int *pfdReleaseFence)
{
    v4l2_buffer buffer;
    v4l2_plane planes[SC_MAX_PLANES];

    if (!TestFlag(frm.flags, SCFF_REQBUFS)) {
        SC_LOGE("Trying to QBUF without REQBUFS for %s is not allowed",
                frm.name);
        return false;
    }

    if (!DQBuf(frm))
        return false;

    memset(&buffer, 0, sizeof(buffer));
    memset(&planes, 0, sizeof(planes));

    buffer.type   = frm.type;
    buffer.memory = frm.memory;
    buffer.index  = 0;
    buffer.length = frm.out_num_planes;

    if (pfdReleaseFence) {
        buffer.flags    = V4L2_BUF_FLAG_USE_SYNC;
        buffer.reserved = frm.fdAcquireFence;
    }

    buffer.m.planes = planes;
    for (unsigned long i = 0; i < buffer.length; i++) {
        planes[i].length = frm.out_plane_size[i];
        if (V4L2_TYPE_IS_OUTPUT(buffer.type))
            planes[i].bytesused = planes[i].length;
        if (buffer.memory == V4L2_MEMORY_DMABUF)
            planes[i].m.fd = reinterpret_cast<int>(frm.addr[i]);
        else
            planes[i].m.userptr = reinterpret_cast<unsigned long>(frm.addr[i]);
    }


    if (exynos_v4l2_qbuf(m_fdScaler, &buffer) < 0) {
        SC_LOGERR("Failed to QBUF for the %s", frm.name);
        return false;
    }

    SetFlag(frm.flags, SCFF_QBUF);

    if (pfdReleaseFence) {
        if (frm.fdAcquireFence >= 0)
            close(frm.fdAcquireFence);
        frm.fdAcquireFence = -1;

        *pfdReleaseFence = static_cast<int>(buffer.reserved);
    }

    SC_LOGD("Successfully QBUF for the %s", frm.name);

    return true;
}

bool CScalerV4L2::ReqBufs(FrameInfo &frm)
{
    v4l2_requestbuffers reqbufs;

    if (TestFlag(frm.flags, SCFF_REQBUFS)) {
        SC_LOGD("Skipping REQBUFS for the %s since it is already done", frm.name);
        return true;
    }

    memset(&reqbufs, 0, sizeof(reqbufs));

    reqbufs.type    = frm.type;
    reqbufs.memory  = frm.memory;
    reqbufs.count   = 1;

    if (exynos_v4l2_reqbufs(m_fdScaler, &reqbufs) < 0) {
        SC_LOGERR("Failed to REQBUFS for the %s", frm.name);
        return false;
    }

    SetFlag(frm.flags, SCFF_REQBUFS);

    SC_LOGD("Successfully REQBUFS for the %s", frm.name);

    return true;
}

bool CScalerV4L2::SetRotate(int rot, int flip_h, int flip_v)
{
    if ((rot % 90) != 0) {
        SC_LOGE("Rotation of %d degree is not supported", rot);
        return false;
    }

    SetRotDegree(rot);

    if (flip_h)
        SetFlag(m_fStatus, SCF_VFLIP);
    else
        ClearFlag(m_fStatus, SCF_VFLIP);

    if (flip_v)
        SetFlag(m_fStatus, SCF_HFLIP);
    else
        ClearFlag(m_fStatus, SCF_HFLIP);

    SetFlag(m_fStatus, SCF_ROTATION_FRESH);

    return true;
}

bool CScalerV4L2::StreamOn(FrameInfo &frm)
{
    if (!TestFlag(frm.flags, SCFF_REQBUFS)) {
        SC_LOGE("Trying to STREAMON without REQBUFS for %s is not allowed",
                frm.name);
        return false;
    }

    if (!TestFlag(frm.flags, SCFF_STREAMING)) {
        if (exynos_v4l2_streamon(m_fdScaler, frm.type) < 0 ) {
            SC_LOGERR("Failed StreamOn for the %s", frm.name);
            return false;
        }

        SetFlag(frm.flags, SCFF_STREAMING);

        SC_LOGD("Successfully VIDIOC_STREAMON for the %s", frm.name);
    }

    return true;
}

bool CScalerV4L2::DQBuf(FrameInfo &frm)
{
    if (!TestFlag(frm.flags, SCFF_QBUF))
        return true;

    v4l2_buffer buffer;
    v4l2_plane plane[SC_NUM_OF_PLANES];

    memset(&buffer, 0, sizeof(buffer));

    buffer.type = frm.type;
    buffer.memory = frm.memory;

    if (V4L2_TYPE_IS_MULTIPLANAR(buffer.type)) {
        memset(plane, 0, sizeof(plane));

        buffer.length = frm.out_num_planes;
        buffer.m.planes = plane;
    }

    ClearFlag(frm.flags, SCFF_QBUF);

    if (exynos_v4l2_dqbuf(m_fdScaler, &buffer) < 0 ) {
        SC_LOGERR("Failed to DQBuf the %s", frm.name);
        return false;
    }

    if (buffer.flags & V4L2_BUF_FLAG_ERROR) {
        SC_LOGE("Error occurred while processing streaming data");
        return false;
    }

    SC_LOGD("Successfully VIDIOC_DQBUF for the %s", frm.name);

    return true;
}
