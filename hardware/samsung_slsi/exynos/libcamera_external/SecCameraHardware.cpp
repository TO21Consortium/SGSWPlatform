/*
 * Copyright 2008, The Android Open Source Project
 * Copyright 2013, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 /*!
 * \file      SecCameraHardware.cpp
 * \brief     source file for Android Camera Ext HAL
 * \author    teahyung kim (tkon.kim@samsung.com)
 * \date      2013/04/30
 *
 */

#ifndef ANDROID_HARDWARE_SECCAMERAHARDWARE_CPP
#define ANDROID_HARDWARE_SECCAMERAHARDWARE_CPP

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "SecCameraHardware"

#include "SecCameraHardware.h"

#define CLEAR(x)                memset(&(x), 0, sizeof(x))

#define CHECK_ERR(x, log)       if (CC_UNLIKELY(x < 0)) { \
                                    ALOGE log; \
                                    return false; \
                                }

#define CHECK_ERR_N(x, log)     if (CC_UNLIKELY(x < 0)) { \
                                    ALOGE log; \
                                    return x; \
                                }

#define CHECK_ERR_GOTO(out, x, log) if (CC_UNLIKELY(x < 0)) { \
                                        ALOGE log; \
                                        goto out; \
                                    }

#ifdef __GNUC__
#define __swap16gen(x) __statement({                    \
      __uint16_t __swap16gen_x = (x);                 \
                                      \
      (__uint16_t)((__swap16gen_x & 0xff) << 8 |          \
          (__swap16gen_x & 0xff00) >> 8);             \
})
#else /* __GNUC__ */
/* Note that these macros evaluate their arguments several times.  */
#define __swap16gen(x)                          \
      (__uint16_t)(((__uint16_t)(x) & 0xff) << 8 | ((__uint16_t)(x) & 0xff00) >> 8)
#endif 



#define MAX_THUMBNAIL_SIZE (60000)

#define EXYNOS_MEM_DEVICE_DEV_NAME "/dev/exynos-mem"

#ifdef SENSOR_NAME_GET_FROM_FILE
int g_rearSensorId = -1;
int g_frontSensorId = -1;
#endif

namespace android {

struct record_heap {
    uint32_t type;  // make sure that this is 4 byte.
    phyaddr_t y;
    phyaddr_t cbcr;
    uint32_t buf_index;
    uint32_t reserved;
};

gralloc_module_t const* SecCameraHardware::mGrallocHal;

SecCameraHardware::SecCameraHardware(int cameraId, camera_device_t *dev)
    : ISecCameraHardware(cameraId, dev)
{
    if (cameraId == CAMERA_ID_BACK)
        mFliteFormat = CAM_PIXEL_FORMAT_YUV422I;
    else
        mFliteFormat = CAM_PIXEL_FORMAT_YUV422I;

    mPreviewFormat        = CAM_PIXEL_FORMAT_YUV420SP;
    /* set suitably */
    mRecordingFormat      = CAM_PIXEL_FORMAT_YUV420;
    mPictureFormat        = CAM_PIXEL_FORMAT_JPEG;

    mZoomRatio            = 1000.00f;
    mFimc1CSC             = NULL;
    mFimc2CSC             = NULL;

    CLEAR(mWindowBuffer);

    if (!mGrallocHal) {
        int ret = 0;
        ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&mGrallocHal);
        if (CC_UNLIKELY(ret))
            ALOGE("SecCameraHardware: error, fail on loading gralloc HAL");
    }

    createInstance(cameraId);
}

SecCameraHardware::~SecCameraHardware()
{
}

SecCameraHardware::FLiteV4l2::FLiteV4l2()
{
    mCameraFd = -1;
    mBufferCount = 0;
    mStreamOn = false;
    mCmdStop = 0;
    mFastCapture = false;
}

SecCameraHardware::FLiteV4l2::~FLiteV4l2()
{
}

/* HACK */
#define SENSOR_SCENARIO_MASK    0xF0000000
#define SENSOR_SCENARIO_SHIFT    28
#define SENSOR_MODULE_MASK        0x0FFFFFFF
#define SENSOR_MODULE_SHIFT        0

int SecCameraHardware::FLiteV4l2::init(const char *devPath, const int cameraId)
{
    int err;
    int sensorId = 0;

    mCameraFd = open(devPath, O_RDWR);
    mCameraId = cameraId;
    CHECK_ERR_N(mCameraFd, ("FLiteV4l2 init: error %d, open %s (%d - %s)", mCameraFd, devPath, errno, strerror(errno)));
    CLOGV("DEBUG (%s) : %s, fd(%d) ", devPath, mCameraFd);

#if defined(USE_VIDIOC_QUERY_CAP)
    /* fimc_v4l2_querycap */
    struct v4l2_capability cap;
    CLEAR(cap);
    err = ioctl(mCameraFd, VIDIOC_QUERYCAP, &cap);
    CHECK_ERR_N(err, ("FLiteV4l2 init: error %d, VIDIOC_QUERYCAP", err));

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE_MPLANE)) {
        ALOGE("FLiteV4l2 init: error, no capture devices");
        return -1;
    }
#endif

    /* fimc_v4l2_enuminput */
    struct v4l2_input input;
    CLEAR(input);
    /* input.index = cameraId; */

    /* fimc_v4l2_s_input */
    sensorId = getSensorId(cameraId);
    CLOGV("DEBUG (FLiteV4l2::init) : sensor Id(%d) ", __func__, sensorId);
	
    input.index = (2 << SENSOR_SCENARIO_SHIFT) | sensorId; // external camera / sensor domule id

    err = ioctl(mCameraFd, VIDIOC_S_INPUT, &input);
    CHECK_ERR_N(err, ("FLiteV4l2 init: error %d, VIDIOC_S_INPUT", err));

#ifdef FAKE_SENSOR
    fakeIndex = 0;
    fakeByteData = 0;
#endif
    return 0;
}

void SecCameraHardware::FLiteV4l2::deinit()
{
    if (mCameraFd >= 0) {
        close(mCameraFd);
        mCameraFd = -1;
    }
    mBufferCount = 0;
    CLOGV("DEBUG (FLiteV4l2::deinit) : out ");
}

int SecCameraHardware::FLiteV4l2::startPreview(image_rect_type *fliteSize,
    cam_pixel_format format, int numBufs, int fps, bool movieMode, node_info_t *mFliteNode)
{
    /* fimc_v4l2_enum_fmt */
    int err;
    bool found = false;

#if defined(USE_VIDIOC_ENUM_FORMAT)
    struct v4l2_fmtdesc fmtdesc;
    CLEAR(fmtdesc);
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmtdesc.index = 0;

    while ((err = ioctl(mCameraFd, VIDIOC_ENUM_FMT, &fmtdesc)) == 0) {
        if (fmtdesc.pixelformat == (uint32_t)format) {
            ALOGV("FLiteV4l2 start: %s", fmtdesc.description);
            found = true;
            break;
        }
        fmtdesc.index++;
    }
    if (!found) {
        ALOGE("FLiteV4l2 start: error, unsupported pixel format (%c%c%c%c)"
            " fmtdesc.pixelformat = %d, %s, err=%d", format, format >> 8, format >> 16, format >> 24,
            fmtdesc.pixelformat, fmtdesc.description, err);
        return -1;
    }
#endif

#ifdef USE_CAPTURE_MODE
    /*
       capture_mode = oprmode
       oprmode = 0 (preview)
       oprmode = 1 (single capture)
       oprmode = 2 (HDR capture)
       */
    err = sctrl(CAM_CID_CAPTURE_MODE, false);
    CHECK_ERR_N(err, ("FLiteV4l2 sctrl: error %d, CAM_CID_CAPTURE_MODE", err));
#endif

    v4l2_field field;
    if (movieMode)
        field = (enum v4l2_field)IS_MODE_PREVIEW_VIDEO;
    else
        field = (enum v4l2_field)IS_MODE_PREVIEW_STILL;

    /* fimc_v4l2_s_fmt */
    struct v4l2_format v4l2_fmt;
    CLEAR(v4l2_fmt);

    CLOGD("DEBUG (FLiteV4l2::startPreview) : setting( size: %dx%d)", fliteSize->width, fliteSize->height);
    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_fmt.fmt.pix_mp.width = fliteSize->width;
    v4l2_fmt.fmt.pix_mp.height = fliteSize->height;
    v4l2_fmt.fmt.pix_mp.pixelformat = format;
    v4l2_fmt.fmt.pix_mp.field = field;
    err = ioctl(mCameraFd, VIDIOC_S_FMT, &v4l2_fmt);
    CHECK_ERR_N(err, ("FLiteV4l2 start: error %d, VIDIOC_S_FMT", err));

#ifdef CACHEABLE
    sctrl(V4L2_CID_CACHEABLE, 1);
#endif
    /* setting fps */
    CLOGD("DEBUG (FLiteV4l2::startPreview) : setting( fps: %d)", fps);

    struct v4l2_streamparm streamParam;
    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    streamParam.parm.capture.timeperframe.numerator   = 1;
    streamParam.parm.capture.timeperframe.denominator = fps;

    CLOGI("INFO (FLiteV4l2::startPreview) : set framerate (denominator=%d)", fps);
    err = sparm(&streamParam);

    CHECK_ERR_N(err, ("ERR(%s): sctrl V4L2_CID_CAM_FRAME_RATE(%d) value(%d)", __FUNCTION__, err, fps));

    /* sctrl(V4L2_CID_EMBEDDEDDATA_ENABLE, 0); */

    /* fimc_v4l2_reqbufs */
    struct v4l2_requestbuffers req;
    CLEAR(req);
    req.count = numBufs;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
#ifdef USE_USERPTR
    req.memory = V4L2_MEMORY_USERPTR;
#else
    req.memory = V4L2_MEMORY_MMAP;
#endif
    err = ioctl(mCameraFd, VIDIOC_REQBUFS, &req);
    CHECK_ERR_N(err, ("FLiteV4l2 start: error %d, VIDIOC_REQBUFS", err));

    mBufferCount = (int)req.count;

    /* setting mFliteNode for Q/DQ */
    mFliteNode->width  = v4l2_fmt.fmt.pix.width;
    mFliteNode->height = v4l2_fmt.fmt.pix.height;
    mFliteNode->format = v4l2_fmt.fmt.pix.pixelformat;
    mFliteNode->planes = NUM_FLITE_BUF_PLANE + SPARE_PLANE;
#ifdef SUPPORT_64BITS
    mFliteNode->memory = (enum v4l2_memory)req.memory;
    mFliteNode->type   = (enum v4l2_buf_type)req.type;
#else
    mFliteNode->memory = req.memory;
    mFliteNode->type   = req.type;
#endif
    mFliteNode->buffers= numBufs;

    return 0;
}

int SecCameraHardware::FLiteV4l2::startCapture(image_rect_type *img,
    cam_pixel_format format, int numBufs, int capKind)
{
    /* fimc_v4l2_enum_fmt */
    int err;
#if defined(USE_VIDIOC_ENUM_FORMAT)
    bool found = false;
    struct v4l2_fmtdesc fmtdesc;
    CLEAR(fmtdesc);
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmtdesc.index = 0;

    while ((err = ioctl(mCameraFd, VIDIOC_ENUM_FMT, &fmtdesc)) == 0) {
        if (fmtdesc.pixelformat == (uint32_t)format) {
            ALOGV("FLiteV4l2 start: %s", fmtdesc.description);
            found = true;
            break;
        }

        fmtdesc.index++;
    }

    if (!found) {
        ALOGE("FLiteV4l2 start: error, unsupported pixel format (%c%c%c%c)"
            " fmtdesc.pixelformat = %d, %s, err=%d", format, format >> 8, format >> 16, format >> 24,
            fmtdesc.pixelformat, fmtdesc.description, err);
        return -1;
    }
#endif

    /* fimc_v4l2_s_fmt */
    struct v4l2_format v4l2_fmt;
    CLEAR(v4l2_fmt);

#ifdef USE_CAPTURE_MODE
    /*
       capture_mode = oprmode
       oprmode = 0 (preview)
       oprmode = 1 (single capture)
       oprmode = 2 (HDR capture)
       */
    err = sctrl(CAM_CID_CAPTURE_MODE, true);
    CHECK_ERR_N(err, ("ERR(%s): sctrl V4L2_CID_CAMERA_CAPTURE(%d) enable failed", __FUNCTION__, err));
#endif
#ifdef USE_CAPTURE_FPS_CHANGE
    /* setting fps */
    struct v4l2_streamparm streamParam;
    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    streamParam.parm.capture.timeperframe.numerator   = 1;
    streamParam.parm.capture.timeperframe.denominator = CAPTURE_FRAMERATE;

    CLOGI("INFO (FLiteV4l2::startCapture) : set framerate (denominator=%d)", CAPTURE_FRAMERATE);
    err = sparm(&streamParam);

    CHECK_ERR_N(err, ("ERR(%s):  set framerate (denominator=%d) value(%d)", __FUNCTION__, CAPTURE_FRAMERATE, err));
#endif //#if 1

    CLOGD("DEBUG(FLiteV4l2::startCapture): requested capture size %dx%d %d", img->width, img->height, format);

    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_fmt.fmt.pix_mp.width = img->width;
    v4l2_fmt.fmt.pix_mp.height = img->height;
    v4l2_fmt.fmt.pix_mp.pixelformat = format;

    err = ioctl(mCameraFd, VIDIOC_S_FMT, &v4l2_fmt);
    CHECK_ERR_N(err, ("FLiteV4l2 start: error %d, VIDIOC_S_FMT", err));

    /* fimc_v4l2_reqbufs */
    struct v4l2_requestbuffers req;
    CLEAR(req);
    req.count = SKIP_CAPTURE_CNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
#ifdef USE_USERPTR
    req.memory = V4L2_MEMORY_USERPTR;
#else
    req.memory = V4L2_MEMORY_MMAP;
#endif

    err = ioctl(mCameraFd, VIDIOC_REQBUFS, &req);
    CHECK_ERR_N(err, ("FLiteV4l2 start: error %d, VIDIOC_REQBUFS", err));

    mBufferCount = (int)req.count;

    return 0;
}

int SecCameraHardware::FLiteV4l2::startRecord(image_rect_type *img, image_rect_type *videoSize,
    cam_pixel_format format, int numBufs)
{
    /* fimc_v4l2_enum_fmt */
     int err;
     bool found = false;
     struct v4l2_fmtdesc fmtdesc;
     CLEAR(fmtdesc);
     fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
     fmtdesc.index = 0;

     while ((err = ioctl(mCameraFd, VIDIOC_ENUM_FMT, &fmtdesc)) == 0) {
         if (fmtdesc.pixelformat == (uint32_t)format) {
             CLOGV("FLiteV4l2 start: %s", fmtdesc.description);
             found = true;
             break;
         }
         fmtdesc.index++;
     }

     if (!found) {
         CLOGE("FLiteV4l2 start: error, unsupported pixel format (%c%c%c%c)"
             " fmtdesc.pixelformat = %d, %s, err=%d", format, format >> 8, format >> 16, format >> 24,
             fmtdesc.pixelformat, fmtdesc.description, err);
         return -1;
     }

     /* fimc_v4l2_s_fmt */
     struct v4l2_format v4l2_fmt;
     CLEAR(v4l2_fmt);

     v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
     v4l2_fmt.fmt.pix.width = videoSize->width;
     v4l2_fmt.fmt.pix.height = videoSize->height;
     v4l2_fmt.fmt.pix.pixelformat = format;
     v4l2_fmt.fmt.pix.priv = V4L2_PIX_FMT_MODE_CAPTURE;

     err = ioctl(mCameraFd, VIDIOC_S_FMT, &v4l2_fmt);
     CHECK_ERR_N(err, ("FLiteV4l2 start: error %d, VIDIOC_S_FMT", err));

     v4l2_fmt.type = V4L2_BUF_TYPE_PRIVATE;
     v4l2_fmt.fmt.pix.width = videoSize->width;
     v4l2_fmt.fmt.pix.height = videoSize->height;
     v4l2_fmt.fmt.pix.pixelformat = format;
     v4l2_fmt.fmt.pix.field = (enum v4l2_field)IS_MODE_PREVIEW_STILL;

     CLOGD("Sensor FMT %dx%d", v4l2_fmt.fmt.pix.width, v4l2_fmt.fmt.pix.height);

     err = ioctl(mCameraFd, VIDIOC_S_FMT, &v4l2_fmt);
     CHECK_ERR_N(err, ("FLiteV4l2 start: error %d, VIDIOC_S_FMT", err));

     /* fimc_v4l2_reqbufs */
     struct v4l2_requestbuffers req;
     CLEAR(req);
     req.count = numBufs;
     req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
#ifdef USE_USERPTR
     req.memory = V4L2_MEMORY_USERPTR;
#else
     req.memory = V4L2_MEMORY_MMAP;
#endif

     err = ioctl(mCameraFd, VIDIOC_REQBUFS, &req);
     CHECK_ERR_N(err, ("FLiteV4l2 start: error %d, VIDIOC_REQBUFS", err));

     mBufferCount = (int)req.count;

     return 0;
}

int SecCameraHardware::FLiteV4l2::reqBufZero(node_info_t *mFliteNode)
{
    int err;
    /* fimc_v4l2_reqbufs */
    struct v4l2_requestbuffers req;
    CLEAR(req);
    req.count = 0;
    req.type = mFliteNode->type;
    req.memory = mFliteNode->memory;
    CLOGV("DEBUG(FLiteV4l2::reqBufZero): type[%d] memory[%d]", req.type, req.memory);
    err = ioctl(mCameraFd, VIDIOC_REQBUFS, &req);
    CHECK_ERR_N(err, ("FLiteV4l2 reqBufZero: error %d", err));
    return 1;
}

int SecCameraHardware::FLiteV4l2::querybuf2(unsigned int index, int planeCnt, ExynosBuffer *buf)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int err;

    CLEAR(v4l2_buf);
    CLEAR(planes);

    /* loop for buffer count */
    v4l2_buf.index      = index;
    v4l2_buf.type       = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_buf.memory     = V4L2_MEMORY_MMAP;
    v4l2_buf.length     = planeCnt;
    v4l2_buf.m.planes   = planes;

    err = ioctl(mCameraFd , VIDIOC_QUERYBUF, &v4l2_buf);
    if (err < 0) {
        CLOGE("ERR(FLiteV4l2::querybuf2)(%d): error %d, index %d", __LINE__, err, index);
        return 0;
    }

    /* loop for planes */
    for (int i = 0; i < planeCnt; i++) {
        unsigned int length = v4l2_buf.m.planes[i].length;
        unsigned int offset = v4l2_buf.m.planes[i].m.mem_offset;
        char *vAdr = (char *) mmap(0,
                    v4l2_buf.m.planes[i].length,
                    PROT_READ | PROT_WRITE, MAP_SHARED,
                    mCameraFd, offset);
        CLOGV("DEBUG(%s): [%d][%d] length %d, offset %d vadr %p", __FUNCTION__, index, i, length, offset, vAdr);
        if (vAdr == NULL) {
            CLOGE("ERR(FLiteV4l2::querybuf2)(%d): [%d][%d] vAdr is %p", __LINE__,  index, i, vAdr);
            return 0;
        } else {
            buf->virt.extP[i] = vAdr;
            buf->size.extS[i] = length;
            memset(buf->virt.extP[i], 0x0, buf->size.extS[i]);
        }
    }

    return 1;
}

int SecCameraHardware::FLiteV4l2::expBuf(unsigned int index, int planeCnt, ExynosBuffer *buf)
{
    struct v4l2_exportbuffer v4l2_expbuf;
    int err;

    for (int i = 0; i < planeCnt; i++) {
        memset(&v4l2_expbuf, 0, sizeof(v4l2_expbuf));
        v4l2_expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        v4l2_expbuf.index = index;
        err = ioctl(mCameraFd, VIDIOC_EXPBUF, &v4l2_expbuf);
        if (err < 0) {
            CLOGE("ERR(FLiteV4l2::expBuf)(%d): [%d][%d] failed %d", __LINE__, index, i, err);
            goto EXP_ERR;
        } else {
            CLOGV("DEBUG(%s): [%d][%d] fd %d", __FUNCTION__, index, i, v4l2_expbuf.fd);
            buf->fd.extFd[i] = v4l2_expbuf.fd;
        }
    }
    return 1;

EXP_ERR:
    for (int i = 0; i < planeCnt; i++) {
        if (buf->fd.extFd[i] > 0)
            ion_close(buf->fd.extFd[i]);
    }
    return 0;
}


sp<MemoryHeapBase> SecCameraHardware::FLiteV4l2::querybuf(uint32_t *frmsize)
{
    struct v4l2_buffer v4l2_buf;
    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.length = 0;

    for (int i = 0; i < mBufferCount; i++) {
        int err;
        v4l2_buf.index = i;
        err = ioctl(mCameraFd , VIDIOC_QUERYBUF, &v4l2_buf);
        if (err < 0) {
            CLOGE("FLiteV4l2 querybufs: error %d, index %d", err, i);
            *frmsize = 0;
            return NULL;
        }
    }

    *frmsize = v4l2_buf.length;

    return mBufferCount == 1 ?
        new MemoryHeapBase(mCameraFd, v4l2_buf.length, v4l2_buf.m.offset) : NULL;
}

int SecCameraHardware::FLiteV4l2::qbuf(uint32_t index)
{
    struct v4l2_buffer v4l2_buf;
    CLEAR(v4l2_buf);
    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = index;

    int err = ioctl(mCameraFd, VIDIOC_QBUF, &v4l2_buf);
    CHECK_ERR_N(err, ("FLiteV4l2 qbuf: error %d", err));

    return 0;
}

int SecCameraHardware::FLiteV4l2::dqbuf()
{
    struct v4l2_buffer v4l2_buf;
    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;

    int err = ioctl(mCameraFd, VIDIOC_DQBUF, &v4l2_buf);
    CHECK_ERR_N(err, ("FLiteV4l2 dqbuf: error %d", err));

    if (v4l2_buf.index > (uint32_t)mBufferCount) {
        CLOGE("FLiteV4l2 dqbuf: error %d, invalid index", v4l2_buf.index);
        return -1;
    }

    return v4l2_buf.index;
}

int SecCameraHardware::FLiteV4l2::dqbuf(uint32_t *index)
{
    struct v4l2_buffer v4l2_buf;
    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;

    int err = ioctl(mCameraFd, VIDIOC_DQBUF, &v4l2_buf);
    if (err >= 0)
        *index = v4l2_buf.index;

    return err;
}

int SecCameraHardware::FLiteV4l2::qbuf2(node_info_t *node, uint32_t index)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int i;
    int ret = 0;

    CLEAR(planes);
    CLEAR(v4l2_buf);

    v4l2_buf.m.planes   = planes;
    v4l2_buf.type       = node->type;
    v4l2_buf.memory     = node->memory;
    v4l2_buf.index      = index;
    v4l2_buf.length     = node->planes;

    for(i = 0; i < node->planes; i++){
        if (node->memory == V4L2_MEMORY_DMABUF)
            v4l2_buf.m.planes[i].m.fd = (int)(node->buffer[index].fd.extFd[i]);
        else if (node->memory == V4L2_MEMORY_USERPTR) {
            v4l2_buf.m.planes[i].m.userptr = (unsigned long)(node->buffer[index].virt.extP[i]);
        } else if (node->memory == V4L2_MEMORY_MMAP) {
            v4l2_buf.m.planes[i].m.userptr = (unsigned long)(node->buffer[index].virt.extP[i]);
        } else {
            CLOGE("ERR(%s):invalid node->memory(%d)", __func__, node->memory);
            return -1;
        }
        v4l2_buf.m.planes[i].length  = (unsigned long)(node->buffer[index].size.extS[i]);
    }

    ret = exynos_v4l2_qbuf(node->fd, &v4l2_buf);
    if (ret < 0) {
        CLOGE("ERR(FLiteV4l2::qbuf2):exynos_v4l2_qbuf failed (index:%d)(ret:%d)", index, ret);
        return -1;
    }else{
        CLOGV("DEBUG (FLiteV4l2::qbuf2) : exynos_v4l2_qbuf(index:%d)", index);
    }

    return ret;
}

int SecCameraHardware::FLiteV4l2::qbufForCapture(ExynosBuffer *buf, uint32_t index)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    unsigned int i;
    int ret = 0;

    CLEAR(planes);
    CLEAR(v4l2_buf);

    v4l2_buf.m.planes   = planes;
    v4l2_buf.type       = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
#ifdef USE_USERPTR
    v4l2_buf.memory     = V4L2_MEMORY_USERPTR;
#else
    v4l2_buf.memory     = V4L2_MEMORY_MMAP;
#endif
    v4l2_buf.index      = index;
    v4l2_buf.length     = 2;

    for(i = 0; i < v4l2_buf.length; i++){
        if (v4l2_buf.memory == V4L2_MEMORY_DMABUF)
            v4l2_buf.m.planes[i].m.fd = (int)(buf->fd.extFd[i]);
        else if (v4l2_buf.memory == V4L2_MEMORY_USERPTR) {
            v4l2_buf.m.planes[i].m.userptr = (unsigned long)(buf->virt.extP[i]);
        } else if (v4l2_buf.memory == V4L2_MEMORY_MMAP) {
            v4l2_buf.m.planes[i].m.userptr = (unsigned long)(buf->virt.extP[i]);
        } else {
            ALOGE("ERR(FLiteV4l2::qbufForCapture):invalid node->memory(%d)", v4l2_buf.memory);
            return -1;
        }
        v4l2_buf.m.planes[i].length  = (unsigned long)(buf->size.extS[i]);
    }

    ret = exynos_v4l2_qbuf(this->getFd(), &v4l2_buf);
    if (ret < 0) {
        CLOGE("ERR(FLiteV4l2::qbufForCapture):exynos_v4l2_qbuf failed (index:%d)(ret:%d)", index, ret);
        return -1;
    }

    return ret;
}


int SecCameraHardware::FLiteV4l2::dqbuf2(node_info_t *node)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int ret;

    CLEAR(planes);
    CLEAR(v4l2_buf);

    v4l2_buf.type       = node->type;
    v4l2_buf.memory     = node->memory;
    v4l2_buf.m.planes   = planes;
    v4l2_buf.length     = node->planes;

    ret = exynos_v4l2_dqbuf(node->fd, &v4l2_buf);
    if (ret < 0) {
        if (ret != -EAGAIN)
            ALOGE("ERR(%s):VIDIOC_DQBUF failed (%d)", __func__, ret);
        return ret;
    }

    if (v4l2_buf.flags & V4L2_BUF_FLAG_ERROR) {
        ALOGE("ERR(FLiteV4l2::dqbuf2):VIDIOC_DQBUF returned with error (%d)", V4L2_BUF_FLAG_ERROR);
        return -1;
    }else{
        CLOGV("DEBUG [FLiteV4l2::dqbuf2(%d)] exynos_v4l2_dqbuf(index:%d)", __LINE__, v4l2_buf.index);
    }

    return v4l2_buf.index;
}

int SecCameraHardware::FLiteV4l2::dqbufForCapture(ExynosBuffer *buf)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[VIDEO_MAX_PLANES];
    int ret;

    CLEAR(planes);
    CLEAR(v4l2_buf);

    v4l2_buf.m.planes   = planes;
    v4l2_buf.type       = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
#ifdef USE_USERPTR
    v4l2_buf.memory     = V4L2_MEMORY_USERPTR;
#else
    v4l2_buf.memory     = V4L2_MEMORY_MMAP;
#endif
    v4l2_buf.length     = 2;

    ret = exynos_v4l2_dqbuf(this->getFd(), &v4l2_buf);
    if (ret < 0) {
        if (ret != -EAGAIN)
            ALOGE("ERR(FLiteV4l2::dqbufForCapture):VIDIOC_DQBUF failed (%d)", ret);
        return ret;
    }

    if (v4l2_buf.flags & V4L2_BUF_FLAG_ERROR) {
        ALOGE("ERR(FLiteV4l2::dqbufForCapture):VIDIOC_DQBUF returned with error (%d)", V4L2_BUF_FLAG_ERROR);
        return -1;
    }

    return v4l2_buf.index;
}

#ifdef FAKE_SENSOR
int SecCameraHardware::FLiteV4l2::fakeQbuf2(node_info_t *node, uint32_t index)
{
    fakeByteData++;
    fakeByteData = fakeByteData % 0xFF;

    for (int i = 0; i < node->planes; i++) {
        memset(node->buffer[index].virt.extP[i], fakeByteData,
                node->buffer[index].size.extS[i]);

    }
    fakeIndex = index;
    return fakeIndex;
}

int SecCameraHardware::FLiteV4l2::fakeDqbuf2(node_info_t *node)
{
    return fakeIndex;
}
#endif


int SecCameraHardware::FLiteV4l2::stream(bool on)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    int err = 0;

    CLOGV("DEBUG (FLiteV4l2::stream) : in ");

    err = sctrl(V4L2_CID_IS_S_STREAM, on ? IS_ENABLE_STREAM : IS_DISABLE_STREAM);
    CHECK_ERR_N(err, ("s_stream: error %d", err));

    err = ioctl(mCameraFd, on ? VIDIOC_STREAMON : VIDIOC_STREAMOFF, &type);
    CHECK_ERR_N(err, ("FLiteV4l2 stream: error %d", err));

    mStreamOn = on;

    CLOGD("DEBUG (FLiteV4l2::stream) : stream turning %s", on ? "on" : "off");

    return 0;
}

int SecCameraHardware::FLiteV4l2::polling(bool recordingMode)
{
    int err = 0;
    struct pollfd events;
    CLEAR(events);
    events.fd = mCameraFd;
    events.events = POLLIN | POLLERR;

    const long timeOut = 1500;

    if (recordingMode) {
        const long sliceTimeOut = 66;

        for (int i = 0; i < (timeOut / sliceTimeOut); i++) {
            if (!mStreamOn)
                return 0;
            err = poll(&events, 1, sliceTimeOut);
            if (err > 0)
                break;
        }
    } else {
#if 1 /* for test fast capture */
        const int count = 40;
        for (int i = 0; i < count; i++) {
            err = poll(&events, 1, timeOut / count);
            if (mFastCapture) {
                return 0;
            }
        }
#else
            err = poll(&events, 1, timeOut);
#endif
    }

    if (CC_UNLIKELY(err <= 0))
        CLOGE("FLiteV4l2 poll: error %d", err);

    return err;
}

int SecCameraHardware::FLiteV4l2::gctrl(uint32_t id, int *value)
{
    struct v4l2_control ctrl;
    CLEAR(ctrl);
    ctrl.id = id;

    int err = ioctl(mCameraFd, VIDIOC_G_CTRL, &ctrl);
    CHECK_ERR_N(err, ("FLiteV4l2 gctrl: error %d, id %#x", err, id));
    *value = ctrl.value;

    return 0;
}

int SecCameraHardware::FLiteV4l2::gctrl(uint32_t id, unsigned short *value)
{
    struct v4l2_control ctrl;
    CLEAR(ctrl);
    ctrl.id = id;

    int err = ioctl(mCameraFd, VIDIOC_G_CTRL, &ctrl);
    CHECK_ERR_N(err, ("FLiteV4l2 gctrl: error %d, id %#x", err, id));
    *value = (unsigned short)ctrl.value;

    return 0;
}

int SecCameraHardware::FLiteV4l2::gctrl(uint32_t id, char *value, int size)
{
    struct v4l2_ext_controls ctrls;
    struct v4l2_ext_control ctrl;

    CLEAR(ctrls);
    ctrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ctrls.count = 1;
    ctrls.controls = &ctrl;

    CLEAR(ctrl);
    ctrl.id = id;
    ctrl.string = value;
    ctrl.size=size;

    int err = ioctl(mCameraFd, VIDIOC_G_EXT_CTRLS, &ctrls);
    CHECK_ERR_N(err, ("FLiteV4l2 gctrl: error %d, id %#x", err, id));

    return 0;
}

int SecCameraHardware::FLiteV4l2::sctrl(uint32_t id, int value)
{
    struct v4l2_control ctrl;
    CLEAR(ctrl);
    ctrl.id = id;
    ctrl.value = value;

    int err = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    CHECK_ERR_N(err, ("FLiteV4l2 sctrl: error %d, id %#x value %d", err, id, value));

    return 0;
}

int SecCameraHardware::FLiteV4l2::sparm(struct v4l2_streamparm *stream_parm)
{
    int err = ioctl(mCameraFd, VIDIOC_S_PARM, stream_parm);
    CHECK_ERR_N(err, ("FLiteV4l2 sparm: error %d, value %d", err,
        stream_parm->parm.capture.timeperframe.denominator));

    return 0;
}


int SecCameraHardware::FLiteV4l2::getYuvPhyaddr(int index,
                                       phyaddr_t *y,
                                       phyaddr_t *cbcr)
{
    struct v4l2_control ctrl;
    int err;

    if (y) {
        CLEAR(ctrl);
        ctrl.id = V4L2_CID_PADDR_Y;
        ctrl.value = index;

        err = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        CHECK_ERR_N(err, ("FLiteV4l2 getYuvPhyaddr: error %d, V4L2_CID_PADDR_Y", err));

        *y = ctrl.value;
    }

    if (cbcr) {
        CLEAR(ctrl);
        ctrl.id = V4L2_CID_PADDR_CBCR;
        ctrl.value = index;

        err = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        CHECK_ERR_N(err, ("FLiteV4l2 getYuvPhyaddr: error %d, V4L2_CID_PADDR_CBCR", err));

        *cbcr = ctrl.value;
    }
    return 0;
}

int SecCameraHardware::FLiteV4l2::getYuvPhyaddr(int index,
                                       phyaddr_t *y,
                                       phyaddr_t *cb,
                                       phyaddr_t *cr)
{
    struct v4l2_control ctrl;
    int err;

    if (y) {
        CLEAR(ctrl);
        ctrl.id = V4L2_CID_PADDR_Y;
        ctrl.value = index;

        err = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        CHECK_ERR_N(err, ("FLiteV4l2 getYuvPhyaddr: error %d, V4L2_CID_PADDR_Y", err));

        *y = ctrl.value;
    }

    if (cb) {
        CLEAR(ctrl);
        ctrl.id = V4L2_CID_PADDR_CB;
        ctrl.value = index;

        err = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        CHECK_ERR_N(err, ("FLiteV4l2 getYuvPhyaddr: error %d, V4L2_CID_PADDR_CB", err));

        *cb = ctrl.value;
    }

    if (cr) {
        CLEAR(ctrl);
        ctrl.id = V4L2_CID_PADDR_CR;
        ctrl.value = index;

        err = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
        CHECK_ERR_N(err, ("FLiteV4l2 getYuvPhyaddr: error %d, V4L2_CID_PADDR_CR", err));

        *cr = ctrl.value;
    }

    return 0;
}

int SecCameraHardware::FLiteV4l2::setFastCaptureFimc(uint32_t IsFastCaptureCalled)
{
    mFastCapture = IsFastCaptureCalled;
    ALOGD("Set Fast capture in fimc : %d", mFastCapture);
    return 0;
}

int SecCameraHardware::FLiteV4l2::reset()
{
    struct v4l2_control ctrl;
    CLEAR(ctrl);
    ctrl.id = V4L2_CID_CAMERA_RESET;
    ctrl.value = 0;

    int err = ioctl(mCameraFd, VIDIOC_S_CTRL, &ctrl);
    CHECK_ERR_N(err, ("FLiteV4l2 reset: error %d", err));

    return 0;
}

void SecCameraHardware::FLiteV4l2::forceStop()
{
    mCmdStop = 1;
    return;
}

int SecCameraHardware::FLiteV4l2::getFd()
{
    return mCameraFd;
}

bool SecCameraHardware::init()
{
    CLOGD("DEBUG [%s(%d)] -in-", __FUNCTION__, __LINE__);

    LOG_PERFORMANCE_START(1);
    CLEAR(mFliteNode);
    CSC_METHOD cscMethod = CSC_METHOD_HW;
    int err;

    if (mCameraId == CAMERA_ID_BACK)
        err = mFlite.init(FLITE0_DEV_PATH, mCameraId);
    else
        err = mFlite.init(FLITE1_DEV_PATH, mCameraId);


    if (CC_UNLIKELY(err < 0)) {
        ALOGE("initCamera X: error(%d), %s", err,
            (mCameraId == CAMERA_ID_BACK) ? FLITE0_DEV_PATH : FLITE1_DEV_PATH);
        goto fimc_out;
    }
    if (mCameraId == CAMERA_ID_BACK) {
        char read_prop[92];
        int res_prop = 0;
        int value = 0;

        CLEAR(read_prop);
        res_prop = property_get("persist.sys.camera.fw_update", read_prop, "0");
        ALOGD("Lens Close Hold persist.sys.camera.fw_update [%s], res %d", read_prop, res_prop);

        // ISP boot option : "0" => Normal, "1" => fw_update
        if (!strcmp(read_prop, "1"))
            value = 0x1;
        else
        {
            CLEAR(read_prop);
            res_prop = property_get("persist.sys.camera.samsung", read_prop, "0");
            ALOGD("Lens Close Hold persist.sys.camera.samsung [%s], res %d", read_prop, res_prop);

            // samsung app : "0" => 3th party app, "1" => samsung app
            if (!strcmp(read_prop, "1"))
                value = 0x0;
            else
                value = 0x1;
        }

        CLEAR(read_prop);
        res_prop = property_get("persist.sys.camera.mem", read_prop, "0");
        ALOGD("ISP mem : property get [%s], res %d", read_prop, res_prop);

        // ISP target memory : "0" => NOR, "1" => EEPROM
        if (!strcmp(read_prop, "1"))
            value |= (0x1 << 8);
        else
            value |= 0x0;

        ALOGD("call camera init with value: 0x%02X", value);
        if (CC_UNLIKELY(err < 0)) {
            ALOGE("camera init error:%d", err);
        }
    }
    mFliteNode.fd = mFlite.getFd();
    ALOGV("mFlite fd %d", mFlite.getFd());

    setExifFixedAttribute();

    /* init CSC (fimc1, fimc2) */
    mFimc1CSC = csc_init(cscMethod);
    if (mFimc1CSC == NULL)
        ALOGE("ERR(%s): mFimc1CSC csc_init() fail", __func__);
    err = csc_set_hw_property(mFimc1CSC, CSC_HW_PROPERTY_FIXED_NODE, FIMC1_NODE_NUM);
    if (err != 0)
        ALOGE("ERR(%s): fimc0 open failed %d", __func__, err);

    mFimc2CSC = csc_init(cscMethod);
    if (mFimc2CSC == NULL)
        ALOGE("ERR(%s): mFimc2CSC csc_init() fail", __func__);
    err = csc_set_hw_property(mFimc2CSC, CSC_HW_PROPERTY_FIXED_NODE, FIMC2_NODE_NUM);
    if (err != 0)
        ALOGE("ERR(%s): fimc1 open failed %d", __func__, err);

    LOG_PERFORMANCE_END(1, "total");
    return ISecCameraHardware::init();

fimc_out:
    mFlite.deinit();
    return false;
}

void SecCameraHardware::initDefaultParameters()
{
    char str[16];
    CLEAR(str);
    if (mCameraId == CAMERA_FACING_BACK) {
        snprintf(str, sizeof(str), "%f", (double)Exif::DEFAULT_BACK_FOCAL_LEN_NUM/
            Exif::DEFAULT_BACK_FOCAL_LEN_DEN);
        mParameters.set(CameraParameters::KEY_FOCAL_LENGTH, str);
    } else {
        snprintf(str, sizeof(str), "%f", (double)Exif::DEFAULT_FRONT_FOCAL_LEN_NUM/
            Exif::DEFAULT_FRONT_FOCAL_LEN_DEN);
        mParameters.set(CameraParameters::KEY_FOCAL_LENGTH, str);
    }

    ISecCameraHardware::initDefaultParameters();
}

void SecCameraHardware::release()
{
    CLOGD("INFO(%s) : in ",__FUNCTION__);

    /* Forced wake up sound interrupt */
    mCameraPower = false;
    ExynosBuffer nullBuf;
    int i = 0;

    ISecCameraHardware::release();

    mFlite.deinit();

    /* flite buffer free */
    for (i = 0; i < FLITE_BUF_CNT; i++) {
        freeMem(&mFliteNode.buffer[i]);
        mFliteNode.buffer[i] = nullBuf;
    }

    /* capture buffer free */
    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        freeMem(&mPictureBufDummy[i]);
        mPictureBufDummy[i] = nullBuf;
    }
    freeMem(&mPictureBuf);
    mPictureBuf = nullBuf;

    mInitRecSrcQ();
    mInitRecDstBuf();

    /* deinit CSC (fimc0, fimc1) */
    if (mFimc1CSC)
        csc_deinit(mFimc1CSC);
    mFimc1CSC = NULL;

    if (mFimc2CSC)
        csc_deinit(mFimc2CSC);
    mFimc2CSC = NULL;
}

int SecCameraHardware::nativeSetFastCapture(bool onOff)
{
    mFastCaptureCalled = onOff;

    mFlite.setFastCaptureFimc(mFastCaptureCalled);
    return 0;
}

bool SecCameraHardware::nativeCreateSurface(uint32_t width, uint32_t height, uint32_t halPixelFormat)
{
    CLOGV("INFO(%s) : in ",__FUNCTION__);

    int min_bufs;

    if (CC_UNLIKELY(mFlagANWindowRegister == true)) {
        ALOGI("Surface already exist!!");
        return true;
    }

    status_t err;

    if (mPreviewWindow->get_min_undequeued_buffer_count(mPreviewWindow, &min_bufs)) {
        ALOGE("%s: could not retrieve min undequeued buffer count", __FUNCTION__);
        return INVALID_OPERATION;
    }

    if (min_bufs >= PREVIEW_BUF_CNT) {
        ALOGE("%s: warning, min undequeued buffer count %d is too high (expecting at most %d)", __FUNCTION__,
             min_bufs, PREVIEW_BUF_CNT - 1);
    }

    CLOGD("DEBUG [%s(%d)] setting buffer count to %d", __FUNCTION__, __LINE__,PREVIEW_BUF_CNT);
    if (mPreviewWindow->set_buffer_count(mPreviewWindow, PREVIEW_BUF_CNT)) {
        ALOGE("%s: could not set buffer count", __FUNCTION__);
        return INVALID_OPERATION;
    }

    if (mPreviewWindow->set_usage(mPreviewWindow, GRALLOC_SET_USAGE_FOR_CAMERA) != 0) {
        ALOGE("ERR(%s):could not set usage on gralloc buffer", __func__);
        return INVALID_OPERATION;
    }

    CLOGD("DEBUG [%s(%d)] (%dx%d)", __FUNCTION__, __LINE__, width, height);
    if (mPreviewWindow->set_buffers_geometry(mPreviewWindow,
                                width, height,
                                halPixelFormat)) {
        CLOGE("%s: could not set buffers geometry ", __FUNCTION__);
        return INVALID_OPERATION;
    }

    mFlagANWindowRegister = true;

    return true;
}

bool SecCameraHardware::nativeDestroySurface(void)
{
    CLOGV("DEBUG [%s(%d)] -in-", __FUNCTION__, __LINE__);
    mFlagANWindowRegister = false;

    return true;
}

int SecCameraHardware::save_dump_path( uint8_t *real_data, int data_size, const char* filePath)
{
    FILE *fp = NULL;
    char *buffer = NULL;

    CLOGE("save dump E");
    fp = fopen(filePath, "wb");

    if (fp == NULL) {
        CLOGE("Save dump image open error");
        return -1;
    }

    CLOGE("%s: real_data size ========>  %d", __func__, data_size);
    buffer = (char *) malloc(data_size);
    if (buffer == NULL) {
        CLOGE("Save buffer alloc failed");
        if (fp)
            fclose(fp);

        return -1;
    }

    memcpy(buffer, real_data, data_size);

    fflush(stdout);

    fwrite(buffer, 1, data_size, fp);

    fflush(fp);

    if (fp)
        fclose(fp);
    if (buffer)
        free(buffer);

    CLOGE("save dump X");
    return 0;
}

bool SecCameraHardware::nativeFlushSurfaceYUV420(uint32_t width, uint32_t height, uint32_t size, uint32_t index, int type)
{
    //ALOGV("%s: width=%d, height=%d, size=0x%x, index=%d", __FUNCTION__, width, height, size, index);
    ExynosBuffer dstBuf;
    buffer_handle_t *buf_handle = NULL;

    if (CC_UNLIKELY(!mPreviewWindowSize.width)) {
        ALOGE("nativeFlushSurfacePostview: error, Preview window %dx%d", mPreviewWindowSize.width, mPreviewWindowSize.height);
        return false;
    }

    if (CC_UNLIKELY(mFlagANWindowRegister == false)) {
        ALOGE("%s::mFlagANWindowRegister == false fail", __FUNCTION__);
        return false;
    }

    if (mPreviewWindow && mGrallocHal) {
        int stride;
        if (0 != mPreviewWindow->dequeue_buffer(mPreviewWindow, &buf_handle, &stride)) {
            ALOGE("Could not dequeue gralloc buffer!\n");
            return false;
        } else {
            if (mPreviewWindow->lock_buffer(mPreviewWindow, buf_handle) != 0)
                ALOGE("ERR(%s):Could not lock gralloc buffer !!", __func__ );
        }

#ifdef SUPPORT_64BITS
        void *vaddr[3] = {NULL};
        if (!mGrallocHal->lock(mGrallocHal,
                               *buf_handle,
                               GRALLOC_LOCK_FOR_CAMERA,
                               0, 0, width, height, vaddr)) {
#else
        unsigned int vaddr[3];
        if (!mGrallocHal->lock(mGrallocHal,
                               *buf_handle,
                               GRALLOC_LOCK_FOR_CAMERA,
                               0, 0, width, height, (void **)vaddr)) {
#endif

            char *src;
            char *ptr = (char *)vaddr[0];
            src = (char *)mPostviewHeap->data;

            memcpy(ptr, src, width * height);
            src += width * height;
            ptr = (char *)vaddr[1];
            memcpy(ptr, src, (width * height ) >> 1);

            mGrallocHal->unlock(mGrallocHal, *buf_handle);
        }

        if (0 != mPreviewWindow->enqueue_buffer(mPreviewWindow, buf_handle)) {
            ALOGE("Could not enqueue gralloc buffer!\n");
            return false;
        }
    } else if (!mGrallocHal) {
        ALOGE("nativeFlushSurfaceYUV420: gralloc HAL is not loaded");
        return false;
    }

    return true;

CANCEL:
    if (mPreviewWindow->cancel_buffer(mPreviewWindow, buf_handle) != 0)
        ALOGE("ERR(%s):Fail to cancel buffer", __func__);

    return false;
}

bool SecCameraHardware::nativeFlushSurface(
    uint32_t width, uint32_t height, uint32_t size, uint32_t index, int type)
{
    //ALOGV("%s: width=%d, height=%d, size=0x%x, index=%d", __FUNCTION__, width, height, size, index);
    ExynosBuffer dstBuf;
    buffer_handle_t *buf_handle = NULL;

    if (CC_UNLIKELY(!mPreviewWindowSize.width)) {
        CLOGE("nativeFlushSurface: error, Preview window %dx%d",
            mPreviewWindowSize.width, mPreviewWindowSize.height);
        return false;
    }

    if (CC_UNLIKELY(mFlagANWindowRegister == false)) {
        CLOGE("%s::mFlagANWindowRegister == false fail", __FUNCTION__);
        return false;
    }

    if (mPreviewWindow && mGrallocHal) {
        int stride;
        if (0 != mPreviewWindow->dequeue_buffer(mPreviewWindow, &buf_handle, &stride)) {
            CLOGE("Could not dequeue gralloc buffer!\n");
            return false;
        } else {
            if (mPreviewWindow->lock_buffer(mPreviewWindow, buf_handle) != 0)
                CLOGE("ERR(%s):Could not lock gralloc buffer !!", __func__ );
        }
#ifdef SUPPORT_64BITS
        void *vaddr[3] = {NULL};
        if (!mGrallocHal->lock(mGrallocHal,
                               *buf_handle,
                               GRALLOC_LOCK_FOR_CAMERA,
                               0, 0, width, height, vaddr)) {
#else
        unsigned int vaddr[3];
        if (!mGrallocHal->lock(mGrallocHal,
                               *buf_handle,
                               GRALLOC_LOCK_FOR_CAMERA,
                               0, 0, width, height, (void **)vaddr)) {
#endif
            /* csc start flite(s) -> fimc0 -> gralloc(d) */
            if (mFimc1CSC) {
                /* set zoom info */
                bool flag;
                flag = getCropRect(mFLiteSize.width, mFLiteSize.height, mPreviewSize.width,
                                   mPreviewSize.height, &mPreviewZoomRect.x, &mPreviewZoomRect.y,
                                   &mPreviewZoomRect.w, &mPreviewZoomRect.h, 2, 2, 2, 2, mZoomRatio);
                if(false == flag) {
                    CLOGE("ERR(%s):mFLiteSize.width = %u mFLiteSize.height = %u "
                        "mPreviewSize.width = %u mPreviewSize.height = %u ", __func__,
                        mFLiteSize.width, mFLiteSize.height, mPreviewSize.width, mPreviewSize.height);
                    CLOGE("ERR(%s):Preview CropRect failed X = %u Y = %u W = %u H = %u ", __func__,
                        mPreviewZoomRect.x, mPreviewZoomRect.y, mPreviewZoomRect.w, mPreviewZoomRect.h);
                }

#ifdef DEBUG_PREVIEW
                CLOGD("DEBUG PREVIEW(%s:%d): src(%d,%d,%d,%d,%d,%d), dst(%d,%d), fmt(%s)"
                    , __FUNCTION__, __LINE__
                    , mFLiteSize.width
                    , mFLiteSize.height
                    , mPreviewZoomRect.x
                    , mPreviewZoomRect.y
                    , mPreviewZoomRect.w
                    , mPreviewZoomRect.h
                    , mPreviewSize.width
                    , mPreviewSize.height
                    , mFliteNode.format == CAM_PIXEL_FORMAT_YVU420P ? "YV12" :
                    mFliteNode.format == CAM_PIXEL_FORMAT_YUV420SP ? "NV21" :
                    mFliteNode.format == CAM_PIXEL_FORMAT_YUV422I ? "YUYV" :
                     "Others");

#endif

                /* src : FLite */
                csc_set_src_format(mFimc1CSC,
                        mFLiteSize.width, mFLiteSize.height,
                        mPreviewZoomRect.x, mPreviewZoomRect.y,
                        mPreviewZoomRect.w, mPreviewZoomRect.h,
                        V4L2_PIX_2_HAL_PIXEL_FORMAT(mFliteNode.format),
                        0);

                if (type == CAMERA_HEAP_POSTVIEW) {
                    csc_set_src_buffer(mFimc1CSC,
                            (void **)mPictureBufDummy[0].fd.extFd, CSC_MEMORY_DMABUF);
                } else {
                    csc_set_src_buffer(mFimc1CSC,
                            (void **)mFliteNode.buffer[index].fd.extFd, CSC_MEMORY_DMABUF);
                }

                //mSaveDump("/data/camera_source%d.yuv", &mFliteNode.buffer[index], index);

                int halPixelFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_FULL;

                /* when recording, narrow color range will be applied */
                if (mMovieMode)
                    halPixelFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP;

                /* dst : GRALLOC */
                csc_set_dst_format(mFimc1CSC,
                        mPreviewSize.width, mPreviewSize.height,
                        0, 0,
                        mPreviewSize.width, mPreviewSize.height,
                        halPixelFormat,
                        0);

                const private_handle_t *priv_handle = private_handle_t::dynamicCast(*buf_handle);
                dstBuf.fd.extFd[0]  = priv_handle->fd;
                dstBuf.fd.extFd[1]  = priv_handle->fd1;
                dstBuf.virt.extP[0] = (char *)vaddr[0];
                dstBuf.virt.extP[1] = (char *)vaddr[1];
                dstBuf.size.extS[0] = mPreviewSize.width * mPreviewSize.height;
                dstBuf.size.extS[1] = mPreviewSize.width * mPreviewSize.height / 2;

                csc_set_dst_buffer(mFimc1CSC,
                                   (void **)dstBuf.fd.extFd, CSC_MEMORY_DMABUF);

                mFimc1CSCLock.lock();
                if (csc_convert(mFimc1CSC) != 0) {
                    ALOGE("ERR(%s):csc_convert(mFimc1CSC) fail", __func__);
                    mFimc1CSCLock.unlock();
                    goto CANCEL;
                }
                mFimc1CSCLock.unlock();
            }

            mGrallocHal->unlock(mGrallocHal, *buf_handle);
        }

#ifdef USE_DEDICATED_PREVIEW_ENQUEUE_THREAD
        m_previewFrameQ.pushProcessQ(&buf_handle);
#else
        if (0 != mPreviewWindow->enqueue_buffer(mPreviewWindow, buf_handle)) {
            ALOGE("Could not enqueue gralloc buffer!\n");
            return false;
        }
#endif
    } else if (!mGrallocHal) {
        ALOGE("nativeFlushSurface: gralloc HAL is not loaded");
        return false;
    }

#if 0
    static int count=0;
    mSaveDump("/data/camera_flush%d.yuv", &dstBuf, count);
    count++;
    if(count > 3) count = 0;
#endif
    /* if CAMERA_MSG_PREVIEW_METADATA, prepare callback buffer */
    if (mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME) {
        if (nativePreviewCallback(index, &dstBuf) < 0)
            ALOGE("ERROR(%s): nativePreviewCallback failed", __func__);
    }


    return true;

CANCEL:
    if (mPreviewWindow->cancel_buffer(mPreviewWindow, buf_handle) != 0)
        ALOGE("ERR(%s):Fail to cancel buffer", __func__);

    return false;
}

bool SecCameraHardware::beautyLiveFlushSurface(uint32_t width, uint32_t height, uint32_t size, uint32_t index, int type)
{
    //ALOGV("%s: width=%d, height=%d, size=0x%x, index=%d", __FUNCTION__, width, height, size, index);
    ExynosBuffer dstBuf;
    buffer_handle_t *buf_handle = NULL;

    if (CC_UNLIKELY(!mPreviewWindowSize.width)) {
        ALOGE("beautyLiveFlushSurface: error, Preview window %dx%d", mPreviewWindowSize.width, mPreviewWindowSize.height);
        return false;
    }

    if (CC_UNLIKELY(mFlagANWindowRegister == false)) {
        ALOGE("%s::mFlagANWindowRegister == false fail", __FUNCTION__);
        return false;
    }

    if (mPreviewWindow && mGrallocHal) {
        int stride;
        if (0 != mPreviewWindow->dequeue_buffer(mPreviewWindow, &buf_handle, &stride)) {
            ALOGE("Could not dequeue gralloc buffer!\n");
            return false;
        } else {
            if (mPreviewWindow->lock_buffer(mPreviewWindow, buf_handle) != 0)
                ALOGE("ERR(%s):Could not lock gralloc buffer !!", __func__ );
        }

#ifdef SUPPORT_64BITS
        void *vaddr[3] = {NULL};
        if (!mGrallocHal->lock(mGrallocHal,
                               *buf_handle,
                               GRALLOC_LOCK_FOR_CAMERA,
                               0, 0, width, height, vaddr)) {
#else
        unsigned int vaddr[3];
        if (!mGrallocHal->lock(mGrallocHal,
                               *buf_handle,
                               GRALLOC_LOCK_FOR_CAMERA,
                               0, 0, width, height, (void **)vaddr)) {
#endif
///////////////////////////////////////////////////////////////////////
            char *frame = NULL;
            if ( type==CAMERA_HEAP_PREVIEW) {
                frame =  ((char *)mPreviewHeap->data) + (mPreviewFrameSize * index);
            } else {
                frame =  ((char *)mPostviewHeap->data);
            }
            char *src = frame;
            char *ptr = (char *)vaddr[0];

            if (src == NULL || ptr == NULL)
                return false;

            // Y
            memcpy(ptr, src, width * height);
            src += width * height;
            if (mPreviewFormat == CAM_PIXEL_FORMAT_YVU420P) {
                /* YV12 */
                //ALOGV("%s: yuv420p YV12", __func__);
                // V
                ptr = (char *)vaddr[1];
                if (src == NULL || ptr == NULL)
                    return false;

                memcpy(ptr, src, width * height / 4);
                src += width * height / 4;
                // U
                ptr = (char *)vaddr[2];
                if (src == NULL || ptr == NULL)
                    return false;

                memcpy(ptr, src, width * height / 4);
            } else if (mPreviewFormat == CAM_PIXEL_FORMAT_YUV420SP) {
                /* NV21 */
                //ALOGV("%s: yuv420sp NV21", __func__);
                ptr = (char *)vaddr[1];
                if (src == NULL || ptr == NULL)
                    return false;

                memcpy(ptr, src, (width * height) >> 1);
            }
///////////////////////////////////////////////////////////////////////
            mGrallocHal->unlock(mGrallocHal, *buf_handle);
        }

#ifdef USE_DEDICATED_PREVIEW_ENQUEUE_THREAD
        m_previewFrameQ.pushProcessQ(&buf_handle);
#else
        if (0 != mPreviewWindow->enqueue_buffer(mPreviewWindow, buf_handle)) {
            ALOGE("Could not enqueue gralloc buffer!\n");
            return false;
        }
#endif
    } else if (!mGrallocHal) {
        ALOGE("nativeFlushSurface: gralloc HAL is not loaded");
        return false;
    }

    /* if CAMERA_MSG_PREVIEW_METADATA, prepare callback buffer */
    if (mMsgEnabled & CAMERA_MSG_PREVIEW_FRAME) {
        if (nativePreviewCallback(index, &dstBuf) < 0)
            ALOGE("ERROR(%s): nativePreviewCallback failed", __func__);
    }

    return true;

CANCEL:
    if (mPreviewWindow->cancel_buffer(mPreviewWindow, buf_handle) != 0)
        ALOGE("ERR(%s):Fail to cancel buffer", __func__);

    return false;
}



image_rect_type SecCameraHardware::nativeGetWindowSize()
{
    image_rect_type window;
    if (!mMovieMode) {
        window.width = mPreviewSize.width;
        window.height = mPreviewSize.height;
        return window;
    }

    switch (FRM_RATIO(mPreviewSize)) {
    case CAM_FRMRATIO_QCIF:
        window.width = 528;
        window.height = 432;
        break;
    case CAM_FRMRATIO_VGA:
        window.width = 640;
        window.height = 480;
        break;
    case CAM_FRMRATIO_WVGA:
        window.width = 800;
        window.height = 480;
        break;
    case CAM_FRMRATIO_D1:
        window.width = 720;
        window.height = 480;
        break;
    case CAM_FRMRATIO_HD:
        window.width = 800;
        window.height = 450;
        break;
    default:
        ALOGW("nativeGetWindowSize: invalid frame ratio %d", FRM_RATIO(mPreviewSize));
        window.width = mPreviewSize.width;
        window.height = mPreviewSize.height;
        break;
    }
    return window;
}

bool SecCameraHardware::allocatePreviewHeap()
{
    if (mPreviewHeap) {
        mPreviewHeap->release(mPreviewHeap);
        mPreviewHeap = 0;
        mPreviewHeapFd = -1;
    }

    /* not need to alloc MHB by mM2MExyMemFd for ion */
#ifdef BOARD_USE_MHB_ION
    if (mEnableDZoom) {
        mPreviewHeap = mGetMemoryCb(-1, mPreviewFrameSize, PREVIEW_BUF_CNT, &mPreviewHeapFd);
        if (!mPreviewHeap || mPreviewHeapFd < 0) {
            ALOGE("allocatePreviewHeap: error, fail to create preview heap(%d)", mPreviewHeap);
            return false;
        }
    } else {
        mPreviewHeap = mGetMemoryCb((int)mFlite.getfd(), mPreviewFrameSize, PREVIEW_BUF_CNT, 0);
        if (!mPreviewHeap) {
            ALOGE("allocatePreviewHeap: error, fail to create preview heap(%d %d)", mPreviewHeap, mPreviewHeapFd);
            return false;
        }
    }
#else
    if (mEnableDZoom) {
        mPreviewHeap = mGetMemoryCb(-1, mPreviewFrameSize, PREVIEW_BUF_CNT, 0);
        if (!mPreviewHeap) {
            ALOGE("allocatePreviewHeap: error, fail to create preview heap(%d)", mPreviewHeap);
            return false;
        }
    } else {
        mPreviewHeap = mGetMemoryCb((int)mFlite.getfd(), mPreviewFrameSize, PREVIEW_BUF_CNT, 0);
        if (!mPreviewHeap) {
            ALOGE("allocatePreviewHeap: error, fail to create preview heap(%d)", mPreviewHeap);
            return false;
        }
    }
#endif

    ALOGD("allocatePreviewHeap: %dx%d, frame %dx%d",
        mOrgPreviewSize.width, mOrgPreviewSize.height, mPreviewFrameSize, PREVIEW_BUF_CNT);

    return true;
}

status_t SecCameraHardware::nativeStartPreview()
{
    CLOGV("INFO(%s) : in ",__FUNCTION__);

    int err;
    int formatMode;

    /* YV12 */
   CLOGD("DEBUG[%s(%d)]Preview Format = %s",
        __FUNCTION__, __LINE__,
        mFliteFormat == CAM_PIXEL_FORMAT_YVU420P ? "YV12" :
        mFliteFormat == CAM_PIXEL_FORMAT_YUV420SP ? "NV21" :
        mFliteFormat == CAM_PIXEL_FORMAT_YUV422I ? "YUYV" :
        "Others");

    err = mFlite.startPreview(&mFLiteSize, mFliteFormat, FLITE_BUF_CNT, mFps, mMovieMode, &mFliteNode);
    CHECK_ERR_N(err, ("nativeStartPreview: error, mFlite.start"));

    mFlite.querybuf(&mPreviewFrameSize);
    if (mPreviewFrameSize == 0) {
        CLOGE("nativeStartPreview: error, mFlite.querybuf");
        return UNKNOWN_ERROR;
    }

    if (!allocatePreviewHeap()) {
        CLOGE("nativeStartPreview: error, allocatePreviewHeap");
        return NO_MEMORY;
    }

    for (int i = 0; i < FLITE_BUF_CNT; i++) {
        err = mFlite.qbuf(i);
        CHECK_ERR_N(err, ("nativeStartPreview: error %d, mFlite.qbuf(%d)", err, i));
    }

    err = mFlite.stream(true);
    CHECK_ERR_N(err, ("nativeStartPreview: error %d, mFlite.stream", err));

    CLOGV("DEBUG[%s(%d)]-out -", __FUNCTION__, __LINE__);
    return NO_ERROR;
}

status_t SecCameraHardware::nativeStartPreviewZoom()
{
    CLOGV("INFO(%s) : in ",__FUNCTION__);

    int err;
    int formatMode;
    int i = 0;
    ExynosBuffer nullBuf;

    /* YV12 */
    CLOGD("DEBUG[%s(%d)] : Preview Format = %s",
        __FUNCTION__, __LINE__,
        mFliteFormat == CAM_PIXEL_FORMAT_YVU420P ? "YV12" :
        mFliteFormat == CAM_PIXEL_FORMAT_YUV420SP ? "NV21" :
        mFliteFormat == CAM_PIXEL_FORMAT_YUV422I ? "YUYV" :
        "Others");

   CLOGD("DEBUG[%s(%d)] : size:%dx%d/  fps: %d ",
        __FUNCTION__, __LINE__, mFLiteSize.width, mFLiteSize.height, mFps);

    err = mFlite.startPreview(&mFLiteSize, mFliteFormat, FLITE_BUF_CNT, mFps, mMovieMode, &mFliteNode);
    CHECK_ERR_N(err, ("nativeStartPreviewZoom: error, mFlite.start"));
    mFliteNode.ionClient = mIonCameraClient;

    CLOGI("INFO(%s) : mFliteNode.fd     [%d]"   , __FUNCTION__, mFliteNode.fd);
    CLOGI("INFO(%s) : mFliteNode.width  [%d]"   , __FUNCTION__, mFliteNode.width);
    CLOGI("INFO(%s) : mFliteNode.height [%d]"   , __FUNCTION__, mFliteNode.height);
    CLOGI("INFO(%s) : mFliteNode.format [%c%c%c%c]" , __FUNCTION__, mFliteNode.format,
                mFliteNode.format >> 8, mFliteNode.format >> 16, mFliteNode.format >> 24);
    CLOGI("INFO(%s) : mFliteNode.planes [%d]"   , __FUNCTION__, mFliteNode.planes);
    CLOGI("INFO(%s) : mFliteNode.buffers[%d]"   , __FUNCTION__, mFliteNode.buffers);
    CLOGI("INFO(%s) : mFliteNode.memory [%d]"   , __FUNCTION__, mFliteNode.memory);
    CLOGI("INFO(%s) : mFliteNode.type   [%d]"   , __FUNCTION__, mFliteNode.type);
    CLOGI("INFO(%s) : mFliteNode.ionClient [%d]", __FUNCTION__, mFliteNode.ionClient);

#ifdef USE_USERPTR
    /* For FLITE buffer */
    for (i = 0; i < FLITE_BUF_CNT; i++) {
        mFliteNode.buffer[i] = nullBuf;
        /* YUV size */
        getAlignedYUVSize(mFliteFormat, mFLiteSize.width, mFLiteSize.height, &mFliteNode.buffer[i]);
        if (allocMem(mIonCameraClient, &mFliteNode.buffer[i], 0) == false) {
            CLOGE("ERR(%s):mFliteNode allocMem() fail", __func__);
            return UNKNOWN_ERROR;
        } else {
            CLOGD("DEBUG(%s): mFliteNode allocMem(%d) adr(%p), size(%d), ion(%d)", __FUNCTION__,
                i, mFliteNode.buffer[i].virt.extP[0], mFliteNode.buffer[i].size.extS[0], mIonCameraClient);
            memset(mFliteNode.buffer[i].virt.extP[0], 0, mFliteNode.buffer[i].size.extS[0]);
        }
    }
#else
    /* loop for buffer count */
    for (int i = 0; i < mFliteNode.buffers; i++) {
        err = mFlite.querybuf2(i, mFliteNode.planes, &mFliteNode.buffer[i]);
        CHECK_ERR_N(err, ("nativeStartPreviewZoom: error, mFlite.querybuf2"));
    }
#endif
    CLOGV("DEBUG(%s) : mMsgEnabled(%d)", __FUNCTION__, mMsgEnabled);

    /* allocate preview callback buffer */
    mPreviewFrameSize = getAlignedYUVSize(mPreviewFormat, mOrgPreviewSize.width, mOrgPreviewSize.height, NULL);
    CLOGD("DEBUG(%s) : mPreviewFrameSize(%d)(%dx%d) for callback", __FUNCTION__,
        mPreviewFrameSize, mOrgPreviewSize.width, mOrgPreviewSize.height);

    if (!allocatePreviewHeap()) {
        CLOGE("ERR(%s)(%d) : allocatePreviewHeap() fail", __FUNCTION__, __LINE__);
        goto DESTROYMEM;
    }

    mPreviewZoomRect.x = 0;
    mPreviewZoomRect.y = 0;
    mPreviewZoomRect.w = mFLiteSize.width;
    mPreviewZoomRect.h = mFLiteSize.height;

    for (int i = 0; i < FLITE_BUF_CNT; i++) {
        err = mFlite.qbuf2(&(mFliteNode), i);
        CHECK_ERR_GOTO(DESTROYMEM, err, ("nativeStartPreviewZoom: error %d, mFlite.qbuf(%d)", err, i));
    }

#if !defined(USE_USERPTR)
    /* export FD */
    for (int i = 0; i < mFliteNode.buffers; i++) {
        err = mFlite.expBuf(i, mFliteNode.planes, &mFliteNode.buffer[i]);
        CHECK_ERR_N(err, ("nativeStartPreviewZoom: error, mFlite.expBuf"));
    }
#endif
    err = mFlite.stream(true);
    CHECK_ERR_GOTO(DESTROYMEM, err, ("nativeStartPreviewZoom: error %d, mFlite.stream", err));

    CLOGV("INFO(%s) : out ",__FUNCTION__);
    return NO_ERROR;

DESTROYMEM:
#ifdef USE_USERPTR
    /* flite buffer free */
    for (i = 0; i < FLITE_BUF_CNT; i++) {
        freeMem(&mFliteNode.buffer[i]);
        mFliteNode.buffer[i] = nullBuf;
    }
#else
    if (mFlite.reqBufZero(&mFliteNode) < 0)
        ALOGE("ERR(%s): mFlite.reqBufZero() fail", __func__);
    for (i = 0; i < FLITE_BUF_CNT; i++)
        mFliteNode.buffer[i] = nullBuf;
#endif
    return UNKNOWN_ERROR;
}

int SecCameraHardware::nativeGetPreview()
{
    int index = -1;
    int retries = 10;
    int ret = -1;

    CLOGV("INFO(%s) : in ",__FUNCTION__);

    /* All buffers are empty. Request a frame to the device */
retry:
    if (mFlite.polling() == 0) {
        if (mFastCaptureCalled) {
            CLOGD("DEBUG[%s(%d)] : mFastCaptureCalled!!-",__FUNCTION__, __LINE__);
            return -1;
        }
         CLOGE("ERR[%s(%d)] : no frame, RESET!!!", __FUNCTION__, __LINE__);
        if (retries-- <= 0)
            return -1;

        mFlite.stream(false);
        mFlite.deinit();
        if (mCameraId == CAMERA_ID_BACK)
            mFlite.init(FLITE0_DEV_PATH, mCameraId);
        else
            mFlite.init(FLITE1_DEV_PATH, mCameraId);

        if (mEnableDZoom)
            ret = nativeStartPreviewZoom();
        else
            ret = nativeStartPreview();

        goto retry;
     } else {
        ret = mFlite.dqbuf2(&mFliteNode);
        index = ret;
        CHECK_ERR_N(index, ("nativeGetPreview: error, mFlite.dqbuf"));
    }

    if (mEnableDZoom)
        mRecordTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);

    if (!mPreviewInitialized) {
        mPreviewInitialized = true;
        CLOGD("DEBUG(%s) : preview started",__FUNCTION__);
    }

    return index;
}

status_t SecCameraHardware::nativePreviewCallback(int index, ExynosBuffer *grallocBuf)
{
    /* If orginal size and format(defined by app) is different to
     * changed size and format(defined by hal), do CSC again.
     * But orginal and changed size and format(defined by app) is same,
     * just copy to callback buffer from gralloc buf
     */

    ExynosBuffer dstBuf;
    dstBuf.fd.extFd[0] = mPreviewHeapFd;
    getAlignedYUVSize(mPreviewFormat, mOrgPreviewSize.width, mOrgPreviewSize.height, &dstBuf);
    char *srcAdr = NULL;
    srcAdr = (char *)mPreviewHeap->data;
    srcAdr += (index * mPreviewFrameSize);

    /* YV12 */
    if (mPreviewFormat == V4L2_PIX_FMT_NV21 ||
        mPreviewFormat == V4L2_PIX_FMT_NV21M) {
        dstBuf.virt.extP[0] = srcAdr;
        dstBuf.virt.extP[1] = dstBuf.virt.extP[0] + dstBuf.size.extS[0];
    } else if (mPreviewFormat == V4L2_PIX_FMT_YVU420 ||
                mPreviewFormat == V4L2_PIX_FMT_YVU420M) {
        dstBuf.virt.extP[0] = srcAdr;
        dstBuf.virt.extP[1] = dstBuf.virt.extP[0] + dstBuf.size.extS[0];
        dstBuf.virt.extP[2] = dstBuf.virt.extP[1] + dstBuf.size.extS[1];
    }

    if (grallocBuf == NULL ||
        mOrgPreviewSize.width != mPreviewSize.width ||
        mOrgPreviewSize.height != mPreviewSize.height ||
        mOrgPreviewSize.width != mFLiteSize.width ||
        mOrgPreviewSize.height != mFLiteSize.height ||
        HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP != V4L2_PIX_2_HAL_PIXEL_FORMAT(mPreviewFormat)) {

        /* csc start flite(s) -> fimc0 -> callback(d) */
        if (mFimc1CSC) {
            /* set zoom info */
            bool flag;
            flag = getCropRect(mFLiteSize.width, mFLiteSize.height, mPreviewSize.width,
                               mPreviewSize.height, &mPreviewZoomRect.x, &mPreviewZoomRect.y,
                               &mPreviewZoomRect.w, &mPreviewZoomRect.h, 2, 2, 2, 2, mZoomRatio);
            if(false == flag) {
                CLOGE("ERR(%s):mFLiteSize.width = %u mFLiteSize.height = %u "
                    "mPreviewSize.width = %u mPreviewSize.height = %u ", __func__,
                    mFLiteSize.width, mFLiteSize.height, mPreviewSize.width, mPreviewSize.height);
                CLOGE("ERR(%s):Preview CropRect failed X = %u Y = %u W = %u H = %u ", __func__,
                    mPreviewZoomRect.x, mPreviewZoomRect.y, mPreviewZoomRect.w, mPreviewZoomRect.h);
            }
            /* src : FLite */
            CLOGV("DEBUG(%s):SRC size(%u x %u / %u, %u, %u, %u) ,Format(%s)", __func__,
                    mFLiteSize.width, mFLiteSize.height, mPreviewZoomRect.x, mPreviewZoomRect.y,mPreviewZoomRect.w, mPreviewZoomRect.h,
                    mFliteNode.format == CAM_PIXEL_FORMAT_YVU420P ? "YV12" :
                    mFliteNode.format == CAM_PIXEL_FORMAT_YUV420SP ? "NV21" :
                    mFliteNode.format == CAM_PIXEL_FORMAT_YUV422I ? "YUYV" :
                     "Others");


            csc_set_src_format(mFimc1CSC,
                    mFLiteSize.width, mFLiteSize.height,
                    mPreviewZoomRect.x, mPreviewZoomRect.y,
                    mPreviewZoomRect.w, mPreviewZoomRect.h,
                    V4L2_PIX_2_HAL_PIXEL_FORMAT(mFliteNode.format),
                    0);

#ifdef USE_USERPTR
            csc_set_src_buffer(mFimc1CSC, (void **)mFliteNode.buffer[index].virt.extP, CSC_MEMORY_USERPTR);
#else
            csc_set_dst_buffer(mFimc1CSC, (void **)mFliteNode.buffer[index].fd.extFd, CSC_MEMORY_DMABUF);
#endif

            /* dst : callback buffer(MHB) */
            CLOGV("DEBUG(%s):DST size(%u x %u), Format(%s)", __func__,
                    mOrgPreviewSize.width, mOrgPreviewSize.height,
                    mPreviewFormat == CAM_PIXEL_FORMAT_YVU420P ? "YV12" :
                    mPreviewFormat == CAM_PIXEL_FORMAT_YUV420SP ? "NV21" :
                    mPreviewFormat == CAM_PIXEL_FORMAT_YUV422I ? "YUYV" :
                     "Others");

            csc_set_dst_format(mFimc1CSC,
                    mOrgPreviewSize.width, mOrgPreviewSize.height,
                    0, 0,
                    mOrgPreviewSize.width, mOrgPreviewSize.height,
                    V4L2_PIX_2_HAL_PIXEL_FORMAT(mPreviewFormat),
                    0);

            csc_set_dst_buffer(mFimc1CSC,
                               (void **)dstBuf.virt.extP, CSC_MEMORY_USERPTR);

            mFimc1CSCLock.lock();
            if (csc_convert(mFimc1CSC) != 0) {
                CLOGE("ERR(%s):csc_convert(mFimc1CSC) fail", __func__);
                mFimc1CSCLock.unlock();
                return false;
            }
            mFimc1CSCLock.unlock();
        } else {
            CLOGE("ERR(%s): mFimc1CSC == NULL", __func__);
            return false;
        }
    } else {
        for (int plane = 0; plane < 2; plane++) {
            /* just memcpy only */
            char *srcAddr = NULL;
            char *dstAddr = NULL;
            srcAddr = grallocBuf->virt.extP[plane];
            dstAddr = dstBuf.virt.extP[plane];
            memcpy(dstAddr, srcAddr, dstBuf.size.extS[plane]);
        }
    }
    /* mSaveDump("/data/camera_preview%d.yuv", &dstBuf, index); */

    return NO_ERROR;
}


int SecCameraHardware::nativeReleasePreviewFrame(int index)
{
#ifdef FAKE_SENSOR
    return mFlite.fakeQbuf2(&mFliteNode, index);
#else
    return mFlite.qbuf2(&mFliteNode, index);
#endif
}

void SecCameraHardware::nativeStopPreview(void)
{
    int err = 0;
    int i = 0;
    ExynosBuffer nullBuf;

    err = mFlite.stream(false);
    if (CC_UNLIKELY(err < 0))
        CLOGE("nativeStopPreview: error, mFlite.stream");

#ifdef USE_USERPTR
    /* flite buffer free */
    for (i = 0; i < FLITE_BUF_CNT; i++) {
        freeMem(&mFliteNode.buffer[i]);
        mFliteNode.buffer[i] = nullBuf;
    }
#else
    for (i = 0; i < FLITE_BUF_CNT; i++) {
        for (int j = 0; j < mFliteNode.planes; j++) {
            munmap((void *)mFliteNode.buffer[i].virt.extP[j],
                mFliteNode.buffer[i].size.extS[j]);
            ion_free(mFliteNode.buffer[i].fd.extFd[j]);
        }
        mFliteNode.buffer[i] = nullBuf;
    }

    if (mFlite.reqBufZero(&mFliteNode) < 0)
        ALOGE("ERR(%s): mFlite.reqBufZero() fail", __func__);
#endif
    CLOGV("INFO(%s) : out ",__FUNCTION__);
}

bool SecCameraHardware::allocateRecordingHeap()
{
    int framePlaneSize1 = ALIGN(mVideoSize.width, 16) * ALIGN(mVideoSize.height, 16) + MFC_7X_BUFFER_OFFSET;
    int framePlaneSize2 = ALIGN(mVideoSize.width, 16) * ALIGN(mVideoSize.height / 2, 16) + MFC_7X_BUFFER_OFFSET;;

    if (mRecordingHeap != NULL) {
        mRecordingHeap->release(mRecordingHeap);
        mRecordingHeap = 0;
        mRecordHeapFd = -1;
    }

#ifdef BOARD_USE_MHB_ION
    mRecordingHeap = mGetMemoryCb(-1, sizeof(struct addrs), REC_BUF_CNT, &mRecordHeapFd);
    if (mRecordingHeap == NULL || mRecordHeapFd < 0) {
        ALOGE("ERR(%s): mGetMemoryCb(mRecordingHeap(%d), size(%d) fail [Heap %p, Fd %d]",\
            __func__, REC_BUF_CNT, sizeof(struct addrs), mRecordingHeap, mRecordHeapFd);
        return false;
    }
#else
    mRecordingHeap = mGetMemoryCb(-1, sizeof(struct addrs), REC_BUF_CNT, 0);
    if (mRecordingHeap == NULL) {
        ALOGE("ERR(%s): mGetMemoryCb(mRecordingHeap(%d), size(%d) fail [Heap %p]",\
            __func__, REC_BUF_CNT, sizeof(struct addrs), mRecordingHeap);
        return false;
    }
#endif

    for (int i = 0; i < REC_BUF_CNT; i++) {
#ifdef BOARD_USE_MHB_ION
        for (int j = 0; j < REC_PLANE_CNT; j++) {
            if (mRecordDstHeap[i][j] != NULL) {
                mRecordDstHeap[i][j]->release(mRecordDstHeap[i][j]);
                mRecordDstHeap[i][j] = 0;
                mRecordDstHeapFd[i][j] = -1;
            }
        }

        mRecordDstHeap[i][0] = mGetMemoryCb(-1, framePlaneSize1, 1, &(mRecordDstHeapFd[i][0]));
        mRecordDstHeap[i][1] = mGetMemoryCb(-1, framePlaneSize2, 1, &(mRecordDstHeapFd[i][1]));
        ALOGV("DEBUG(%s): mRecordDstHeap[%d][0] adr(%p), fd(%d)", __func__, i, mRecordDstHeap[i][0]->data, mRecordDstHeapFd[i][0]);
        ALOGV("DEBUG(%s): mRecordDstHeap[%d][1] adr(%p), fd(%d)", __func__, i, mRecordDstHeap[i][1]->data, mRecordDstHeapFd[i][1]);

        if (mRecordDstHeap[i][0] == NULL || mRecordDstHeapFd[i][0] <= 0 ||
            mRecordDstHeap[i][1] == NULL || mRecordDstHeapFd[i][1] <= 0) {
            ALOGE("ERR(%s): mGetMemoryCb(mRecordDstHeap[%d] size(%d/%d) fail",\
                __func__, i, framePlaneSize1, framePlaneSize2);
            goto error;
        }

#ifdef NOTDEFINED
        if (mRecordDstHeap[i][j] == NULL) {
            ALOGE("ERR(%s): mGetMemoryCb(mRecordDstHeap[%d][%d], size(%d) fail",\
                __func__, i, j, framePlaneSize);
            goto error;
        }
#endif
#else
        freeMem(&mRecordingDstBuf[i]);

        mRecordingDstBuf[i].size.extS[0] = framePlaneSize1;
        mRecordingDstBuf[i].size.extS[1] = framePlaneSize2;

        if (allocMem(mIonCameraClient, &mRecordingDstBuf[i], ((1 << 1) | 1)) == false) {
            ALOGE("ERR(%s):allocMem(mRecordingDstBuf() fail", __func__);
            goto error;
        }
#endif
    }

    ALOGD("DEBUG(%s): %dx%d, frame plane (%d/%d)x%d", __func__,
        ALIGN(mVideoSize.width, 16), ALIGN(mVideoSize.height, 2), framePlaneSize1, framePlaneSize2, REC_BUF_CNT);

    return true;

error:
    if (mRecordingHeap == NULL) {
        mRecordingHeap->release(mRecordingHeap);
        mRecordingHeap = NULL;
    }

    for (int i = 0; i < REC_BUF_CNT; i++) {
        for (int j = 0; j < REC_PLANE_CNT; j++) {
            if (mRecordDstHeap[i][j] != NULL) {
                mRecordDstHeap[i][j]->release(mRecordDstHeap[i][j]);
                mRecordDstHeap[i][j] = 0;
            }
        }

        freeMem(&mRecordingDstBuf[i]);
    }

    return false;
}

#ifdef RECORDING_CAPTURE
bool SecCameraHardware::allocateRecordingSnapshotHeap()
{
    /* init jpeg heap */
    if (mJpegHeap) {
        mJpegHeap->release(mJpegHeap);
        mJpegHeap = NULL;
    }

#ifdef BOARD_USE_MHB_ION
    int jpegHeapFd = -1;
    mJpegHeap = mGetMemoryCb(-1, mRecordingPictureFrameSize, 1, jpegHeapFd);
    if (mJpegHeap == NULL || jpegHeapFd < 0) {
        ALOGE("allocateRecordingSnapshotHeap: error, fail to create jpeg heap");
        return UNKNOWN_ERROR;
    }
#else
    mJpegHeap = mGetMemoryCb(-1, mRecordingPictureFrameSize, 1, 0);
    if (mJpegHeap == NULL) {
        ALOGE("allocateRecordingSnapshotHeap: error, fail to create jpeg heap");
        return UNKNOWN_ERROR;
    }
#endif

    ALOGD("allocateRecordingSnapshotHeap: jpeg %dx%d, size %d",
        mPictureSize.width, mPictureSize.height, mRecordingPictureFrameSize);

    return true;
}
#endif

status_t SecCameraHardware::nativeStartRecording(void)
{
    CLOGI("INFO(%s) : in ", __FUNCTION__);
#ifdef NOTDEFINED
    if (mMirror) {
        nativeSetParameters(CAM_CID_HFLIP, 1, 0);
        nativeSetParameters(CAM_CID_HFLIP, 1, 1);
    } else {
        nativeSetParameters(CAM_CID_HFLIP, 0, 0);
        nativeSetParameters(CAM_CID_HFLIP, 0, 1);
    }

    uint32_t recordingTotalFrameSize;
    mFimc1.querybuf(&recordingTotalFrameSize);
    if (recordingTotalFrameSize == 0) {
        ALOGE("nativeStartPreview: error, mFimc1.querybuf");
        return UNKNOWN_ERROR;
    }

    if (!allocateRecordingHeap()) {
        ALOGE("nativeStartRecording: error, allocateRecordingHeap");
        return NO_MEMORY;
    }

    for (int i = 0; i < REC_BUF_CNT; i++) {
        err = mFimc1.qbuf(i);
        CHECK_ERR_N(err, ("nativeStartRecording: error, mFimc1.qbuf(%d)", i));
    }

    err = mFimc1.stream(true);
    CHECK_ERR_N(err, ("nativeStartRecording: error, mFimc1.stream"));
#endif
    CLOGI("INFO(%s) : out ",__FUNCTION__);
    return NO_ERROR;
}

/* for zoom recording */
status_t SecCameraHardware::nativeStartRecordingZoom(void)
{
    int err;
    Mutex::Autolock lock(mNativeRecordLock);

    CLOGI("INFO(%s) : in - (%d/%d)", __FUNCTION__, mVideoSize.width, mVideoSize.height);

    /* 1. init zoom size info */
    mRecordZoomRect.x = 0;
    mRecordZoomRect.y = 0;
    mRecordZoomRect.w = mVideoSize.width;
    mRecordZoomRect.h = mVideoSize.height;

    /* 2. init buffer var src and dst */
    mInitRecSrcQ();
    mInitRecDstBuf();

    /* 3. alloc MHB for recording dst */
    if (!allocateRecordingHeap()) {
        ALOGE("nativeStartPostRecording: error, allocateRecordingHeap");
        goto destroyMem;
    }

    CLOGI("INFO(%s) : out ",__FUNCTION__);
    return NO_ERROR;

destroyMem:
    mInitRecSrcQ();
    mInitRecDstBuf();
    return UNKNOWN_ERROR;
}

#ifdef NOTDEFINED
int SecCameraHardware::nativeGetRecording()
{
    int index;
    phyaddr_t y, cbcr;
    int err, retries = 3;

retry:
    err = mFimc1.polling(true);
    if (CC_UNLIKELY(err <= 0)) {
        if (mFimc1.getStreamStatus()) {
            if (!err && (retries > 0)) {
                ALOGW("nativeGetRecording: warning, wait for input (%d)", retries);
                usleep(300000);
                retries--;
                goto retry;
            }
            ALOGE("nativeGetRecording: error, mFimc1.polling err=%d cnt=%d", err, retries);
        } else {
            ALOGV("nativeGetRecording: stop getting a frame");
        }
        return UNKNOWN_ERROR;
    }

    index = mFimc1.dqbuf();
    CHECK_ERR_N(index, ("nativeGetRecording: error, mFimc1.dqbuf"));

    /* get fimc capture physical addr */
    err = mFimc1.getYuvPhyaddr(index, &y, &cbcr);
    CHECK_ERR_N(err, ("nativeGetRecording: error, mFimc1.getYuvPhyaddr"));

    Mutex::Autolock lock(mNativeRecordLock);

    if (!mRecordingHeap)
        return NO_MEMORY;

    struct record_heap *heap = (struct record_heap *)mRecordingHeap->data;
    heap[index].type = kMetadataBufferTypeCameraSource;
    heap[index].y = y;
    heap[index].cbcr = cbcr;
    heap[index].buf_index = index;

    return index;
}

int SecCameraHardware::nativeReleaseRecordingFrame(int index)
{
    return mFimc1.qbuf(index);
}

int SecCameraHardware::nativeReleasePostRecordingFrame(int index)
{
    return NO_ERROR;
}
void SecCameraHardware::nativeStopPostRecording()
{
    Mutex::Autolock lock(mNativeRecordLock);

    ALOGD("nativeStopPostRecording EX");
}
#endif

void SecCameraHardware::nativeStopRecording()
{
    Mutex::Autolock lock(mNativeRecordLock);

    mInitRecSrcQ();
    mInitRecDstBuf();

    ALOGD("nativeStopRecording EX");
}

bool SecCameraHardware::getCropRect(unsigned int src_w, unsigned int src_h,
    unsigned int dst_w, unsigned int dst_h, unsigned int *crop_x, unsigned int *crop_y,
    unsigned int *crop_w, unsigned int *crop_h,
    int align_x, int align_y, int align_w, int align_h, float zoomRatio)
{
    bool flag = true;
    *crop_w = src_w;
    *crop_h = src_h;

    if (src_w == 0 || src_h == 0 || dst_w == 0 || dst_h == 0) {
        ALOGE("ERR(%s):width or height valuse is 0, src(%dx%d), dst(%dx%d)",
                 __func__, src_w, src_h, dst_w, dst_h);
        return false;
    }

    /* Calculation aspect ratio */
    if (src_w != dst_w
        || src_h != dst_h) {
        float src_ratio = 1.0f;
        float dst_ratio = 1.0f;

        /* ex : 1024 / 768 */
        src_ratio = (float)src_w / (float)src_h;

        /* ex : 352  / 288 */
        dst_ratio = (float)dst_w / (float)dst_h;

        if (dst_ratio <= src_ratio) {
            /* shrink w */
            *crop_w = src_h * dst_ratio;
            *crop_h = src_h;
        } else {
            /* shrink h */
            *crop_w = src_w;
            *crop_h = src_w / dst_ratio;
        }
    }

    flag = getRectZoomAlign(src_w, src_h, dst_w, dst_h, crop_x, crop_y,
        crop_w, crop_h, align_x, align_y, align_w, align_h, zoomRatio);
    if(false == flag) {
        ALOGE("ERR(%s):src_w = %u src_h = %u dst_w = %u dst_h = %u "
            "crop_x = %u crop_y = %u crop_w = %u crop_h = %u "
            "align_w = %d align_h = %d zoom = %f", __func__,
            src_w, src_h, dst_w, dst_h, *crop_x, *crop_y,
            *crop_w, *crop_h, align_w, align_h, zoomRatio);
    }

    return true;
}

bool SecCameraHardware::getRectZoomAlign(unsigned int src_w, unsigned int src_h,
    unsigned int dst_w, unsigned int dst_h, unsigned int *crop_x, unsigned int *crop_y,
    unsigned int *crop_w, unsigned int *crop_h,
    int align_x, int align_y, int align_w, int align_h, float zoomRatio)
{
    int x = 0;
    int y = 0;

    if (zoomRatio != 0) {
        /* calculation zoom */
        *crop_w = (unsigned int)((float)*crop_w * 1000 / zoomRatio);
        *crop_h = (unsigned int)((float)*crop_h * 1000 / zoomRatio);
    }

    /* Alignment by desired size */
    unsigned int w_remain = (*crop_w & (align_w - 1));
    if (w_remain != 0) {
        if (  (unsigned int)(align_w >> 1) <= w_remain
            && (unsigned int)(*crop_w + (align_w - w_remain)) <= src_w) {
            *crop_w += (align_w - w_remain);
        }
        else
            *crop_w -= w_remain;
    }

    unsigned int h_remain = (*crop_h & (align_h - 1));
    if (h_remain != 0) {
        if (  (unsigned int)(align_h >> 1) <= h_remain
            && (unsigned int)(*crop_h + (align_h - h_remain)) <= src_h) {
            *crop_h += (align_h - h_remain);
        }
        else
            *crop_h -= h_remain;
    }

    x = ((int)src_w - (int)*crop_w) >> 1;
    y = ((int)src_h - (int)*crop_h) >> 1;

    if (x < 0 || y < 0) {
        ALOGE("ERR(%s):crop size too big (%u, %u, %u, %u)",
                 __func__, *crop_x, *crop_y, *crop_w, *crop_h);
        return false;
    }

    *crop_x = ALIGN_DOWN(x, align_x);
    *crop_y = ALIGN_DOWN(y, align_y);

    return true;
}

status_t SecCameraHardware::nativeCSCPreview(int index, int type)
{
    ExynosBuffer dstBuf;
    char *dstAdr = NULL;
    dstAdr = (char *)mPreviewHeap->data;
    dstAdr += (index * mPreviewFrameSize);

    if (mFimc1CSC) {
        /* set zoom info */
        mPreviewZoomRect.w = ALIGN_DOWN((uint32_t)((float)mFLiteSize.width * 1000 / mZoomRatio), 2);
        mPreviewZoomRect.h = ALIGN_DOWN((uint32_t)((float)mFLiteSize.height * 1000 / mZoomRatio), 2);

        mPreviewZoomRect.x = ALIGN_DOWN(((mFLiteSize.width - mPreviewZoomRect.w) / 2), 2);
        mPreviewZoomRect.y = ALIGN_DOWN(((mFLiteSize.height - mPreviewZoomRect.h) / 2), 2);

#ifdef DEBUG_PREVIEW
        CLOGD("DEBUG PREVIEW(%s) (%d): src(%d,%d,%d,%d,%d,%d), dst(%d,%d), fmt(%d)"
            , __FUNCTION__, __LINE__
            , mFLiteSize.width
            , mFLiteSize.height
            , mPreviewZoomRect.x
            , mPreviewZoomRect.y
            , mPreviewZoomRect.w
            , mPreviewZoomRect.h
            , mPreviewSize.width
            , mPreviewSize.height
            , V4L2_PIX_2_HAL_PIXEL_FORMAT(mFliteNode.format)
        );
#endif

        /* src : FLite */
        csc_set_src_format(mFimc1CSC,
            mFLiteSize.width, mFLiteSize.height,
            mPreviewZoomRect.x, mPreviewZoomRect.y,
            mPreviewZoomRect.w, mPreviewZoomRect.h,
            V4L2_PIX_2_HAL_PIXEL_FORMAT(mFliteNode.format),
            0);

        if (type == CAMERA_HEAP_POSTVIEW) {
            csc_set_src_buffer(mFimc1CSC, (void **)mPictureBufDummy[0].virt.extP, CSC_MEMORY_USERPTR);
        } else {
            csc_set_src_buffer(mFimc1CSC, (void **)mFliteNode.buffer[index].virt.extP, CSC_MEMORY_USERPTR);
        }

        /* mSaveDump("/data/camera_preview%d.yuv", &mFliteNode.buffer[index], index); */

        int halPixelFormat = HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M_FULL;

        /* dst : GRALLOC */
        csc_set_dst_format(mFimc1CSC,
            mPreviewSize.width, mPreviewSize.height,
            0, 0,
            mPreviewSize.width, mPreviewSize.height,
            halPixelFormat,
            0);

        dstBuf.fd.extFd[0]  = mPreviewHeapFd;
        dstBuf.virt.extP[0] = dstAdr;
        dstBuf.size.extS[0] = mPreviewSize.width * mPreviewSize.height;
        dstBuf.virt.extP[1] = dstBuf.virt.extP[0] + dstBuf.size.extS[0];
        dstBuf.size.extS[1] = mPreviewSize.width * mPreviewSize.height / 2;

        csc_set_dst_buffer(mFimc1CSC, (void **) dstBuf.virt.extP, CSC_MEMORY_USERPTR);

        mFimc1CSCLock.lock();
        if (csc_convert(mFimc1CSC) != 0) {
            ALOGE("ERR(%s):csc_convert(mFimc1CSC) fail", __func__);
            mFimc1CSCLock.unlock();
            return false;
        }
        mFimc1CSCLock.unlock();
    }
    return true;
}

status_t SecCameraHardware::nativeCSCRecording(rec_src_buf_t *srcBuf, int dstIdx)
{
    Mutex::Autolock lock(mNativeRecordLock);

    /* csc start flite(s) -> fimc1 -> callback(d) */
    if (mFimc2CSC) {
        struct addrs *addrs;
        bool flag;
        /* set zoom info */
        flag = getCropRect(mFLiteSize.width, mFLiteSize.height, mVideoSize.width,
            mVideoSize.height, &mRecordZoomRect.x, &mRecordZoomRect.y,
            &mRecordZoomRect.w, &mRecordZoomRect.h, 2, 2, 2, 2, mZoomRatio);
        if(false == flag) {
            ALOGE("ERR(%s):mFLiteSize.width = %u mFLiteSize.height = %u "
                "mVideoSize.width = %u mVideoSize.height = %u ", __func__,
                mFLiteSize.width, mFLiteSize.height, mVideoSize.width, mVideoSize.height);
            ALOGE("ERR(%s):Recording CropRect failed X = %u Y = %u W = %u H = %u ", __func__,
                mRecordZoomRect.x, mRecordZoomRect.y, mRecordZoomRect.w, mRecordZoomRect.h);
        }

#ifdef DEBUG_RECORDING
        ALOGD("DEBUG RECORDING(%s) (%d): src(%d,%d,%d,%d,%d,%d), dst(%d,%d) %d", __func__, __LINE__
            , mFLiteSize.width
            , mFLiteSize.height
            , mRecordZoomRect.x
            , mRecordZoomRect.y
            , mRecordZoomRect.w
            , mRecordZoomRect.h
            , mVideoSize.width
            , mVideoSize.height
            , dstIdx
        );
#endif
        /* src : FLite */
        csc_set_src_format(mFimc2CSC,
                mFLiteSize.width, mFLiteSize.height,
                mRecordZoomRect.x, mRecordZoomRect.y,
                mRecordZoomRect.w, mRecordZoomRect.h,
                V4L2_PIX_2_HAL_PIXEL_FORMAT(mFliteNode.format),
                0);

        csc_set_src_buffer(mFimc2CSC, (void **)srcBuf->buf->fd.extFd, CSC_MEMORY_DMABUF);
        //csc_set_src_buffer(mFimc2CSC, (void **)srcBuf->buf->virt.extP, CSC_MEMORY_USERPTR);

        /* dst : MHB(callback */
        csc_set_dst_format(mFimc2CSC,
                mVideoSize.width, mVideoSize.height,
                0, 0, mVideoSize.width, mVideoSize.height,
                V4L2_PIX_2_HAL_PIXEL_FORMAT(mRecordingFormat),
                /* HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP, */
                0);

        ExynosBuffer dstBuf;
        getAlignedYUVSize(mRecordingFormat, mVideoSize.width, mVideoSize.height, &dstBuf);
        for (int i = 0; i < REC_PLANE_CNT; i++) {
#if defined(ALLOCATION_REC_BUF_BY_MEM_CB)
            dstBuf.virt.extP[i] = (char *)mRecordDstHeap[dstIdx][i]->data;
            dstBuf.fd.extFd[i] = mRecordDstHeapFd[dstIdx][i];
#else
            dstBuf.virt.extP[i] = (char *)mRecordingDstBuf[dstIdx].virt.extP[i];
            dstBuf.fd.extFd[i] = mRecordingDstBuf[dstIdx].fd.extFd[i];
#endif
        }

#ifdef DEBUG_RECORDING
        ALOGD("DEBUG(%s) (%d): dst(%d,%d,%p,%p,%d,%d) index(%d)", __func__, __LINE__
            , dstBuf.fd.extFd[0]
            , dstBuf.fd.extFd[1]
            , dstBuf.virt.extP[0]
            , dstBuf.virt.extP[1]
            , dstBuf.size.extS[0]
            , dstBuf.size.extS[1]
            , dstIdx
        );
#endif
        csc_set_dst_buffer(mFimc2CSC, (void **)dstBuf.fd.extFd, CSC_MEMORY_DMABUF);

        mFimc2CSCLock.lock();
        if (csc_convert_with_rotation(mFimc2CSC, 0, mflipHorizontal, mflipVertical) != 0) {
            ALOGE("ERR(%s):csc_convert(mFimc2CSC) fail", __func__);
            mFimc2CSCLock.unlock();
            return false;
        }
        mFimc2CSCLock.unlock();

        addrs = (struct addrs *)mRecordingHeap->data;
        addrs[dstIdx].type      = kMetadataBufferTypeCameraSource;
        addrs[dstIdx].fd_y      = (unsigned int)dstBuf.fd.extFd[0];
        addrs[dstIdx].fd_cbcr   = (unsigned int)dstBuf.fd.extFd[1];
        addrs[dstIdx].buf_index = dstIdx;
        /* ALOGV("DEBUG(%s): After CSC Camera Meta index %d fd(%d, %d)", __func__, dstIdx, addrs[dstIdx].fd_y, addrs[dstIdx].fd_cbcr); */
        /* mSaveDump("/data/camera_recording%d.yuv", &dstBuf, dstIdx); */
      } else {
        ALOGE("ERR(%s): mFimc2CSC == NULL", __func__);
        return false;
    }
    return true;
}

status_t SecCameraHardware::nativeCSCCapture(ExynosBuffer *srcBuf, ExynosBuffer *dstBuf)
{
    bool flag;

    if (mFimc2CSC) {
        /* set zoom info */
        flag = getCropRect(mFLiteCaptureSize.width, mFLiteCaptureSize.height, mPictureSize.width,
            mPictureSize.height, &mPictureZoomRect.x, &mPictureZoomRect.y,
            &mPictureZoomRect.w, &mPictureZoomRect.h, 2, 2, 2, 2, mZoomRatio);

        if(false == flag) {
            ALOGE("ERR(%s):mFLiteCaptureSize.width = %u mFLiteCaptureSize.height = %u "
                "mPictureSize.width = %u mPictureSize.height = %u ", __func__,
                mFLiteCaptureSize.width, mFLiteCaptureSize.height, mPictureSize.width, mPictureSize.height);
            ALOGE("ERR(%s):Capture CropRect failed X = %u Y = %u W = %u H = %u ", __func__,
                mPictureZoomRect.x, mPictureZoomRect.y, mPictureZoomRect.w, mPictureZoomRect.h);
        }

#ifdef DEBUG_CAPTURE
        ALOGD("DEBUG(%s) (%d): (%d, %d), (%d, %d), (%d, %d, %d, %d)", __func__, __LINE__
            , mFLiteCaptureSize.width
            , mFLiteCaptureSize.height
            , mPictureSize.width
            , mPictureSize.height
            , mPictureZoomRect.x
            , mPictureZoomRect.y
            , mPictureZoomRect.w
            , mPictureZoomRect.h
        );
#endif
        /* src : FLite */
        csc_set_src_format(mFimc2CSC,
                mFLiteCaptureSize.width, mFLiteCaptureSize.height,
                mPictureZoomRect.x, mPictureZoomRect.y,
                mPictureZoomRect.w, mPictureZoomRect.h,
                V4L2_PIX_2_HAL_PIXEL_FORMAT(mFliteNode.format),
                0);

        csc_set_src_buffer(mFimc2CSC, (void **)srcBuf->virt.extP, CSC_MEMORY_USERPTR);

        /* dst : buffer */
#ifdef USE_NV21_CALLBACK
        if (mPictureFormat == CAM_PIXEL_FORMAT_YUV420SP) {
            csc_set_dst_format(mFimc2CSC,
                    mPictureSize.width, mPictureSize.height,
                    0, 0, mPictureSize.width, mPictureSize.height,
                    V4L2_PIX_2_HAL_PIXEL_FORMAT(mPictureFormat),
                    0);

           csc_set_dst_buffer(mFimc2CSC, (void **)dstBuf->virt.extP, CSC_MEMORY_USERPTR);
        }
        else
#endif
        {
            csc_set_dst_format(mFimc2CSC,
                    mPictureSize.width, mPictureSize.height,
                    0, 0, mPictureSize.width, mPictureSize.height,
                    V4L2_PIX_2_HAL_PIXEL_FORMAT(mFliteNode.format),
                    0);

            csc_set_dst_buffer(mFimc2CSC, (void **)dstBuf->fd.extFd, CSC_MEMORY_DMABUF);
        }

        mFimc2CSCLock.lock();
        if (mCameraId == CAMERA_ID_FRONT) {
            if (csc_convert_with_rotation(mFimc2CSC, 0, mflipHorizontal, mflipVertical) != 0) {
                ALOGE("ERR(%s):csc_convert_with_rotation(mFimc2CSC) fail", __func__);
                mFimc2CSCLock.unlock();
                return false;
            }
        } else {
            if (csc_convert(mFimc2CSC) != 0) {
                ALOGE("ERR(%s):csc_convert(mFimc2CSC) fail", __func__);
                mFimc2CSCLock.unlock();
                return false;
            }
        }
        mFimc2CSCLock.unlock();
        /* mSaveDump("/data/camera_recording%d.yuv", &dstBuf, dstIdx); */
    } else {
        ALOGE("ERR(%s): mFimc2CSC == NULL", __func__);
        return false;
    }
    return true;
}

status_t SecCameraHardware::nativeCSCRecordingCapture(ExynosBuffer *srcBuf, ExynosBuffer *dstBuf)
{
    if (mFimc2CSC) {
        ALOGD("DEBUG(%s) (%d) src : mFLiteSize(%d x %d), mPreviewZoomRect(%d, %d, %d, %d)", __FUNCTION__, __LINE__,
            mFLiteSize.width,
            mFLiteSize.height,
            mPreviewZoomRect.x,
            mPreviewZoomRect.y,
            mPreviewZoomRect.w,
            mPreviewZoomRect.h);

        ALOGD("DEBUG(%s) (%d) dst : (%d x %d)", __FUNCTION__, __LINE__,
            mPreviewSize.width, mPreviewSize.height);

        int dstW = mPictureSize.width;
        int dstH = mPictureSize.height;

        /* src : FLite */
        csc_set_src_format(mFimc2CSC,
                mFLiteSize.width, mFLiteSize.height,
                mPreviewZoomRect.x, mPreviewZoomRect.y,
                mPreviewZoomRect.w, mPreviewZoomRect.h,
                V4L2_PIX_2_HAL_PIXEL_FORMAT(mFliteNode.format),
                0);

        csc_set_src_buffer(mFimc2CSC, (void **)srcBuf->virt.extP, CSC_MEMORY_USERPTR);

        /* dst : buffer */
        csc_set_dst_format(mFimc2CSC,
                dstW, dstH,
                0, 0, dstW, dstH,
                V4L2_PIX_2_HAL_PIXEL_FORMAT(mFliteNode.format),
                0);

        csc_set_dst_buffer(mFimc2CSC, (void **)dstBuf->fd.extFd, CSC_MEMORY_DMABUF);

        mFimc2CSCLock.lock();
        if (csc_convert(mFimc2CSC) != 0) {
            ALOGE("ERR(%s):csc_convert(mFimc2CSC) fail", __func__);
            mFimc2CSCLock.unlock();
            return INVALID_OPERATION;
        }
        mFimc2CSCLock.unlock();
        /* mSaveDump("/data/camera_recording%d.yuv", &dstBuf, dstIdx); */
    } else {
        ALOGE("ERR(%s): mFimc1CSC == NULL", __func__);
        return INVALID_OPERATION;
    }
    return NO_ERROR;
}

int SecCameraHardware::nativegetWBcustomX()
{
    int x_value, err;

    err = mFlite.gctrl(V4L2_CID_CAMERA_WB_CUSTOM_X, &x_value);
    CHECK_ERR(err, ("nativegetWBcustomX: error [%d]", x_value));

    ALOGV("%s res[%d]", __func__, x_value);

    return x_value;
}

int SecCameraHardware::nativegetWBcustomY()
{
    int y_value, err;

    err = mFlite.gctrl(V4L2_CID_CAMERA_WB_CUSTOM_Y, &y_value);
    CHECK_ERR(err, ("nativegetWBcustomY: error [%d]", y_value));

    ALOGV("%s res[%d]", __func__, y_value);

    return y_value;
}



status_t SecCameraHardware::nativeSetZoomRatio(int value)
{
    /* Calculation of the crop information */
    mZoomRatio = getZoomRatio(value);

    ALOGD("%s: Zoomlevel = %d, mZoomRatio = %f", __FUNCTION__, value, mZoomRatio);

    return NO_ERROR;
}

#ifdef RECORDING_CAPTURE
bool SecCameraHardware::nativeGetRecordingJpeg(ExynosBuffer *yuvBuf, uint32_t width, uint32_t height)
{
    bool ret = false;

    Exif exif(mCameraId);

    uint8_t *outBuf;
    int jpegSize = 0;
    int thumbSize = 0;
    uint32_t exifSize = 0;

    ExynosBuffer jpegBuf;
    jpegBuf.size.extS[0] = width * height * 2;

    ExynosBuffer exifBuf;
    exifBuf.size.extS[0] = EXIF_MAX_LEN;

    ExynosBuffer thumbnailYuvBuf;
    thumbnailYuvBuf.size.extS[0] = mThumbnailSize.width * mThumbnailSize.height * 2;

    bool thumbnail = false;

    /* Thumbnail */
    LOG_PERFORMANCE_START(1);

    /* use for both thumbnail and main jpeg */
    if (allocMem(mIonCameraClient, &jpegBuf, 1 << 1) == false) {
        ALOGE("ERR(%s):(%d)allocMem(jpegBuf) fail", __func__, __LINE__);
        goto jpeg_encode_done;
    }

    if (mThumbnailSize.width == 0 || mThumbnailSize.height == 0)
        goto encode_jpeg;

    if (allocMem(mIonCameraClient, &thumbnailYuvBuf, 1 << 1) == false) {
        ALOGE("ERR(%s):(%d)allocMem(thumbnailYuvBuf) fail", __func__, __LINE__);
        goto encode_jpeg;
    }

    LOG_PERFORMANCE_START(3);

    scaleDownYuv422((uint8_t *)yuvBuf->virt.extP[0], (int)width, (int)height,
                    (uint8_t *)thumbnailYuvBuf.virt.extP[0], (int)mThumbnailSize.width, (int)mThumbnailSize.height);

    LOG_PERFORMANCE_END(3, "scaleDownYuv422");

    if (this->EncodeToJpeg(&thumbnailYuvBuf, &jpegBuf,
            mThumbnailSize.width, mThumbnailSize.height,
            CAM_PIXEL_FORMAT_YUV422I,
            &thumbSize,
            JPEG_THUMBNAIL_QUALITY) != NO_ERROR) {
        ALOGE("ERR(%s):(%d)EncodeToJpeg", __func__, __LINE__);
        goto encode_jpeg;
    }

    outBuf = (uint8_t *)jpegBuf.virt.extP[0];

    thumbnail = true;

    LOG_PERFORMANCE_END(1, "encode thumbnail");

encode_jpeg:
    /* EXIF */
    setExifChangedAttribute();

    if (allocMem(mIonCameraClient, &exifBuf, 1 << 1) == false) {
        ALOGE("ERR(%s):(%d)allocMem(exifBuf) fail", __func__, __LINE__);
        goto jpeg_encode_done;
    }

#if 0 //##mmkim for test
    if (CC_LIKELY(thumbnail))
        exifSize = exif.make(exifBuf.virt.extP[0], &mExifInfo, exifBuf.size.extS[0], outBuf, thumbSize);
    else
        exifSize = exif.make(exifBuf.virt.extP[0], &mExifInfo);

    if (CC_UNLIKELY(!exifSize)) {
        ALOGE("getJpeg: error, fail to make EXIF");
        goto jpeg_encode_done;
    }
#endif
    /* Jpeg */
    LOG_PERFORMANCE_START(2);

#ifdef SAMSUNG_JPEG_QUALITY_ADJUST_TARGET
    adjustJpegQuality();
#endif

    if (this->EncodeToJpeg(yuvBuf, &jpegBuf,
            width, height,
            CAM_PIXEL_FORMAT_YUV422I,
            &jpegSize,
            mJpegQuality) != NO_ERROR) {
        ALOGE("ERR(%s):(%d)EncodeToJpeg", __func__, __LINE__);
        goto jpeg_encode_done;
    }

    outBuf = (uint8_t *)jpegBuf.virt.extP[0];

    LOG_PERFORMANCE_END(2, "encode jpeg");

    LOG_PERFORMANCE_START(4);
    mRecordingPictureFrameSize = jpegSize + exifSize;
    /* picture frame size is should be calculated before call allocateSnapshotHeap */
    if (!allocateRecordingSnapshotHeap()) {
        ALOGE("getJpeg: error, allocateSnapshotHeap");
        return UNKNOWN_ERROR;
    }

    memcpy(mJpegHeap->data, outBuf, 2);
    memcpy((uint8_t *)mJpegHeap->data + 2, exifBuf.virt.extP[0], exifSize);
    memcpy((uint8_t *)mJpegHeap->data + 2 + exifSize, outBuf + 2, jpegSize - 2);
    LOG_PERFORMANCE_END(4, "jpeg + exif");

    ret = true;

jpeg_encode_done:

    freeMem(&thumbnailYuvBuf);
    freeMem(&exifBuf);
    freeMem(&jpegBuf);

    return ret;
}
#endif

#if FRONT_ZSL
bool SecCameraHardware::allocateFullPreviewHeap()
{
    if (mFullPreviewHeap) {
        mFullPreviewHeap->release(mFullPreviewHeap);
        mFullPreviewHeap = NULL;
    }

    mFullPreviewHeap = mGetMemoryCb((int)mFimc1.getfd(),
                            mFullPreviewFrameSize, kBufferZSLCount, 0);
    if (!mFullPreviewHeap || mFullPreviewHeap->data == MAP_FAILED) {
        CLOGE("ERR(%s): heap creation fail", __func__);
        return false;
    }

    CLOGD("allocateFullPreviewHeap: %dx%d, frame %dx%d",
        mPictureSize.width, mPictureSize.height, mFullPreviewFrameSize, kBufferZSLCount);

    return true;
}

status_t SecCameraHardware::nativeStartFullPreview(void)
{
    CLOGD("INFO(%s) : in ",__FUNCTION__);

    int err;
    cam_pixel_format captureFormat = CAM_PIXEL_FORMAT_YUV422I;

    err = mFimc1.startCapture(&mPictureSize, captureFormat, kBufferZSLCount, 0);
    CHECK_ERR_N(err, ("nativeStartFullPreview: error, mFimc1.start"));

    mFimc1.querybuf(&mFullPreviewFrameSize);
    if (mFullPreviewFrameSize == 0) {
        CLOGE("nativeStartFullPreview: error, mFimc1.querybuf");
        return UNKNOWN_ERROR;
    }

    if (!allocateFullPreviewHeap()) {
        ALOGE("nativeStartFullPreview: error, allocateFullPreviewHeap");
        return NO_MEMORY;
    }

    for (int i = 0; i < kBufferZSLCount; i++) {
        err = mFimc1.qbuf(i);
        CHECK_ERR_N(err, ("nativeStartFullPreview: error, mFimc1.qbuf(%d)", i));
    }

    rawImageMem = new MemoryHeapBase(mFullPreviewFrameSize);

    err = mFimc1.stream(true);
    CHECK_ERR_N(err, ("nativeStartFullPreview: error, mFimc1.stream"));

    CLOGD("INFO(%s) : out ",__FUNCTION__);
    return NO_ERROR;
}

int SecCameraHardware::nativeGetFullPreview()
{
    int index;
    phyaddr_t y, cbcr;
    int err;

    err = mFimc1.polling();
    CHECK_ERR_N(err, ("nativeGetFullPreview: error, mFimc1.polling"));

    index = mFimc1.dqbuf();
    CHECK_ERR_N(index, ("nativeGetFullPreview: error, mFimc1.dqbuf"));

    mJpegIndex = index;

    return index;
}

int SecCameraHardware::nativeReleaseFullPreviewFrame(int index)
{
    return mFimc1.qbuf(index);
}

void SecCameraHardware::nativeStopFullPreview()
{
    if (mFimc1.stream(false) < 0)
        CLOGE("nativeStopFullPreview X: error, mFimc1.stream(0)");

    if (mFullPreviewHeap) {
        mFullPreviewHeap->release(mFullPreviewHeap);
        mFullPreviewHeap = NULL;
    }

    rawImageMem.clear();

    CLOGD("INFO(%s) : out ",__FUNCTION__);
}

void SecCameraHardware::nativeForceStopFullPreview()
{
    mFimc1.forceStop();
}

bool SecCameraHardware::getZSLJpeg()
{
    int ret;

    ALOGE("%s:: mJpegIndex : %d", __func__, mJpegIndex);

#ifdef SUPPORT_64BITS
    memcpy( (unsigned char *)rawImageMem->base(),
            (unsigned char *)((unsigned long)mFullPreviewHeap->data + mJpegIndex * mFullPreviewFrameSize),
            mFullPreviewFrameSize );
#else
    memcpy( (unsigned char *)rawImageMem->base(),
            (unsigned char *)((unsigned int)mFullPreviewHeap->data + mJpegIndex * mFullPreviewFrameSize),
            mFullPreviewFrameSize );
#endif

    sp<MemoryHeapBase> thumbnailJpeg;
    sp<MemoryHeapBase> rawThumbnail;

    unsigned char *thumb;
    int thumbSize = 0;

    bool thumbnail = false;
    /* Thumbnail */
    LOG_PERFORMANCE_START(1);

    if (mThumbnailSize.width == 0 || mThumbnailSize.height == 0)
        goto encode_jpeg;

    rawThumbnail = new MemoryHeapBase(mThumbnailSize.width * mThumbnailSize.height * 2);

    LOG_PERFORMANCE_START(3);
#ifdef USE_HW_SCALER
    ret = scaleDownYUVByFIMC((unsigned char *)rawImageMem->base(),
                        (int)mPictureSize.width,
                        (int)mPictureSize.height,
                        (unsigned char *)rawThumbnail->base(),
                        (int)mThumbnailSize.width,
                        (int)mThumbnailSize.height,
                        CAM_PIXEL_FORMAT_YUV422I);

    if (!ret) {
        CLOGE("Fail to scale down YUV data for thumbnail!\n");
        goto encode_jpeg;
    }
#else
    ret = scaleDownYuv422((unsigned char *)rawImageMem->base(),
                        (int)mPictureSize.width,
                        (int)mPictureSize.height,
                        (unsigned char *)rawThumbnail->base(),
                        (int)mThumbnailSize.width,
                        (int)mThumbnailSize.height);

    if (!ret) {
        CLOGE("Fail to scale down YUV data for thumbnail!\n");
        goto encode_jpeg;
    }
#endif
    LOG_PERFORMANCE_END(3, "scaleDownYuv422");

    thumbnailJpeg = new MemoryHeapBase(mThumbnailSize.width * mThumbnailSize.height * 2);

#ifdef CHG_ENCODE_JPEG
    ret = EncodeToJpeg((unsigned char*)rawThumbnail->base(),
                        mThumbnailSize.width,
                        mThumbnailSize.height,
                        CAM_PIXEL_FORMAT_YUV422I,
                        (unsigned char*)thumbnailJpeg->base(),
                        &thumbSize,
                        JPEG_THUMBNAIL_QUALITY);

    if (ret != NO_ERROR) {
        ALOGE("thumbnail:EncodeToJpeg failed\n");
        goto encode_jpeg;
    }
#endif
    if (thumbSize > MAX_THUMBNAIL_SIZE) {
        ALOGE("thumbnail size is over limit\n");
        goto encode_jpeg;
    }

    thumb = (unsigned char *)thumbnailJpeg->base();
    thumbnail = true;

    LOG_PERFORMANCE_END(1, "encode thumbnail");

encode_jpeg:

    /* EXIF */
    setExifChangedAttribute();

    Exif exif(mCameraId);
    uint32_t exifSize;

    unsigned char *jpeg;
    int jpegSize = 0;
    int jpegQuality = mParameters.getInt(CameraParameters::KEY_JPEG_QUALITY);

    sp<MemoryHeapBase> JpegHeap = new MemoryHeapBase(mPictureSize.width * mPictureSize.height * 2);
    sp<MemoryHeapBase> exifHeap = new MemoryHeapBase(EXIF_MAX_LEN);
    if (!initialized(exifHeap)) {
        ALOGE("getJpeg: error, could not initialize Camera exif heap");
        return false;
    }

    if (!thumbnail)
        exifSize = exif.make(exifHeap->base(), &mExifInfo);
    else
        exifSize = exif.make(exifHeap->base(), &mExifInfo, exifHeap->getSize(), thumb, thumbSize);

#ifdef CHG_ENCODE_JPEG
#ifdef SAMSUNG_JPEG_QUALITY_ADJUST_TARGET
    adjustJpegQuality();
#endif

    ret = EncodeToJpeg((unsigned char*)rawImageMem->base(),
                        mPictureSize.width,
                        mPictureSize.height,
                        CAM_PIXEL_FORMAT_YUV422I,
                        (unsigned char*)JpegHeap->base(),
                        &jpegSize,
                        mJpegQuality);

    if (ret != NO_ERROR) {
        ALOGE("EncodeToJpeg failed\n");
        return false;
    }
#endif
    mPictureFrameSize = jpegSize + exifSize;

    /* picture frame size is should be calculated before call allocateSnapshotHeap */
    if (!allocateSnapshotHeap()) {
        CLOGE("getJpeg: error, allocateSnapshotHeap");
        return false;
    }

    jpeg = (unsigned char *)JpegHeap->base();
    memcpy((unsigned char *)mJpegHeap->data, jpeg, 2);
    memcpy((unsigned char *)mJpegHeap->data + 2, exifHeap->base(), exifSize);
    memcpy((unsigned char *)mJpegHeap->data + 2 + exifSize, jpeg + 2, jpegSize - 2);

    return true;
}
#endif

bool SecCameraHardware::allocatePostviewHeap()
{
    cam_pixel_format postviewFmt;

    CLOGD("INFO(%s) : in ",__FUNCTION__);
    if ( mPostviewHeap && mPostviewHeap->size == mPostviewFrameSize )
        return true;

    if (mPostviewHeap) {
        mPostviewHeap->release(mPostviewHeap);
        mPostviewHeap = 0;
    }

    if (mPostviewHeapTmp) {
        mPostviewHeapTmp->release(mPostviewHeapTmp);
        mPostviewHeapTmp = NULL;
    }

    postviewFmt = CAM_PIXEL_FORMAT_YUV422I;

    mPostviewFrameSize = getAlignedYUVSize(postviewFmt, mPostviewSize.width, mPostviewSize.height, NULL);
    mPostviewHeap = mGetMemoryCb(-1, mPostviewFrameSize, 1, &mPostviewHeapFd);
    if (!mPostviewHeap || mPostviewHeap->data == MAP_FAILED) {
        CLOGE("ERR(%s): Virtual postview heap creation fail", __func__);
        return false;
    }

    mPostviewHeapTmp = mGetMemoryCb(-1, mPostviewFrameSize, 1, &mPostviewHeapTmpFd);
    if (!mPostviewHeapTmp || mPostviewHeapTmp->data == MAP_FAILED) {
        CLOGE("ERR(%s): Virtual postview heap creation fail", __func__);
        return false;
    }

    CLOGD("allocatePostviewHeap: postview %dx%d, frame %d",
        mPostviewSize.width, mPostviewSize.height, mPostviewFrameSize);

    return true;
}

bool SecCameraHardware::allocateSnapshotHeap()
{
    /* init jpeg heap */
    if (mJpegHeap) {
        mJpegHeap->release(mJpegHeap);
        mJpegHeap = 0;
    }

    mJpegHeap = mGetMemoryCb(-1, mPictureFrameSize, 1, &mJpegHeapFd);
    if (mJpegHeap == NULL || mJpegHeap->data == MAP_FAILED) {
        CLOGE("ERR(%s): Jpeg heap creation fail", __func__);
        if (mJpegHeap) {
            mJpegHeap->release(mJpegHeap);
            mJpegHeap = NULL;
        }
        return false;
    }

    CLOGD("allocateSnapshotHeap: jpeg %dx%d, size %d",
         mPictureSize.width, mPictureSize.height, mPictureFrameSize);

#if 0
    /* RAW_IMAGE or POSTVIEW_FRAME heap */
    if ((mMsgEnabled & CAMERA_MSG_RAW_IMAGE) || (mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME)) {
        mRawFrameSize = getAlignedYUVSize(mFliteFormat, mRawSize.width, mRawSize.height, NULL);
        mRawHeap = mGetMemoryCb(-1, mRawFrameSize, 1, &mRawHeapFd);

        ALOGD("allocateSnapshotHeap: postview %dx%d, frame %d",
             mRawSize.width, mRawSize.height, mRawFrameSize);
    }
#endif

    return true;
}

bool SecCameraHardware::allocateHDRHeap()
{
    /* init postview heap */
    if (mHDRHeap) {
        mHDRHeap->release(mHDRHeap);
        mHDRHeap = NULL;
    }

    mRawSize = mPictureSize;
    mHDRFrameSize = mRawSize.width*mRawSize.height*2;
    mHDRHeap = mGetMemoryCb(-1, mHDRFrameSize, 1, &mHDRHeapFd);
    if (!mHDRHeap || mHDRHeap->data == MAP_FAILED) {
        ALOGE("ERR(%s): HDR heap creation fail", __func__);
        goto out;
    }

    return true;

    out:

    if (mHDRHeap) {
        mHDRHeap->release(mHDRHeap);
        mHDRHeap = NULL;
    }

    return false;
}

#ifndef RCJUNG
bool SecCameraHardware::allocateYUVHeap()
{
    /* init YUV main image heap */
    if (mYUVHeap) {
        mYUVHeap->release(mYUVHeap);
        mYUVHeap = 0;
    }

    mYUVHeap = mGetMemoryCb(-1, mRawFrameSize, 1, 0);
    if (!mYUVHeap || mYUVHeap->data == MAP_FAILED) {
        ALOGE("ERR(%s): YUV heap creation fail", __func__);
        goto out;
    }

    ALOGD("allocateYUVHeap: YUV %dx%d, frame %d",
    mOrgPreviewSize.width, mOrgPreviewSize.height, mRawFrameSize);

    return true;

    out:

    if (mYUVHeap) {
        mYUVHeap->release(mYUVHeap);
        mYUVHeap = NULL;
    }

    return false;
}
#endif

void SecCameraHardware::nativeMakeJpegDump()
{
    int postviewOffset;
    CLOGV("DEBUG(%s): (%d)", __FUNCTION__, __LINE__);
    ExynosBuffer jpegBuf;
    CLOGV("DEBUG(%s): (%d)", __FUNCTION__, __LINE__);
    if (getJpegOnBack(&postviewOffset) >= 0) {
        CLOGV("DEBUG(%s): (%d)", __FUNCTION__, __LINE__);
        jpegBuf.size.extS[0] = mPictureFrameSize;
        jpegBuf.virt.extP[0] = (char *)mJpegHeap->data;
        CLOGV("DEBUG(%s): (%d)", __FUNCTION__, __LINE__);
        mSaveDump("/data/camera_%d.jpeg", &jpegBuf, 0);
       CLOGV("DEBUG(%s): (%d)", __FUNCTION__, __LINE__);
    } else {
        CLOGV("DEBUG(%s): (%d) fail!!!!", __FUNCTION__, __LINE__);
    }
}

bool SecCameraHardware::nativeStartPostview()
{
    ALOGD("nativeStartPostview E");

    int err;
    int nBufs = 1;
    int i;

    /* For YUV postview */
    cam_pixel_format captureFormat = CAM_PIXEL_FORMAT_YUV422I;
 
    if ( captureFormat==CAM_PIXEL_FORMAT_YUV422I )
        mPostviewFrameSize = mPostviewSize.width * mPostviewSize.height * 2; /* yuv422I */
    else if ( captureFormat == CAM_PIXEL_FORMAT_YUV420SP )
        mPostviewFrameSize = mPostviewSize.width * mPostviewSize.height * 1.5; /* yuv420sp */

    ALOGD("Postview size : width = %d, height = %d, frame size = %d",
    mPostviewSize.width, mPostviewSize.height, mPreviewFrameSize);
    err = mFlite.startCapture(&mPostviewSize, captureFormat, nBufs, START_CAPTURE_POSTVIEW);
    CHECK_ERR(err, ("nativeStartPostview: error, mFlite.start"));

    ALOGD("nativeStartPostview GC");

    getAlignedYUVSize(captureFormat, mPostviewSize.width, mPostviewSize.height, &mPictureBuf);
    if (allocMem(mIonCameraClient, &mPictureBuf, 1 << 1) == false) {
        ALOGE("ERR(%s):mPictureBuf allocMem() fail", __func__);
        return UNKNOWN_ERROR;
    } else {
        ALOGV("DEBUG(%s): mPictureBuf allocMem adr(%p), size(%d), ion(%d) w/h(%d/%d)", __FUNCTION__,
            mPictureBuf.virt.extP[0], mPictureBuf.size.extS[0], mIonCameraClient,
            mPostviewSize.width, mPostviewSize.height);
        memset(mPictureBuf.virt.extP[0], 0, mPictureBuf.size.extS[0]);
    }

#ifdef USE_USERPTR
    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        getAlignedYUVSize(captureFormat, mPostviewSize.width, mPostviewSize.height, &mPictureBufDummy[i]);
        if (allocMem(mIonCameraClient, &mPictureBufDummy[i], 1 << 1) == false) {
            ALOGE("ERR(%s):mPictureBuf dummy allocMem() fail", __func__);
            return UNKNOWN_ERROR;
        } else {
            ALOGV("DEBUG(%s): mPictureBuf dummy allocMem adr(%p), size(%d), ion(%d) w/h(%d/%d)", __FUNCTION__,
                mPictureBufDummy[i].virt.extP[0], mPictureBufDummy[i].size.extS[0], mIonCameraClient,
                mPostviewSize.width, mPostviewSize.height);
            memset(mPictureBufDummy[i].virt.extP[0], 0, mPictureBufDummy[i].size.extS[0]);
        }
    }
#else
    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        err = mFlite.querybuf2(i, mFliteNode.planes, &mPictureBufDummy[i]);
        CHECK_ERR_N(err, ("nativeStartPreviewZoom: error, mFlite.querybuf2"));
    }
#endif

    /* qbuf dummy buffer for skip */
    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        err = mFlite.qbufForCapture(&mPictureBufDummy[i], i);
        CHECK_ERR(err, ("nativeStartPostview: error, mFlite.qbuf(%d)", i));
    }

    ALOGD("Normal Capture Stream on");
    err = mFlite.stream(true);
    CHECK_ERR(err, ("nativeStartPostview: error, mFlite.stream"));

    if ( mCaptureMode == RUNNING_MODE_SINGLE ) {
   //     PlayShutterSound();
//        Fimc_stream_true_part2();
    }

    ALOGD("nativeStartPostview X");
    return true;
}

bool SecCameraHardware::nativeStartYUVSnapshot()
{
    ALOGD("nativeStartYUVSnapshot E");

    int err;
    int nBufs = 1;
    int i = 0;
    ExynosBuffer nullBuf;

    cam_pixel_format captureFormat = CAM_PIXEL_FORMAT_YUV422I;

    err = mFlite.startCapture(&mFLiteCaptureSize, captureFormat, nBufs, START_CAPTURE_YUV_MAIN);
    CHECK_ERR(err, ("nativeStartYUVSnapshot: error, mFlite.start"));

#ifdef USE_USERPTR
    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        getAlignedYUVSize(captureFormat, mFLiteCaptureSize.width, mFLiteCaptureSize.height, &mPictureBufDummy[i]);
        if (allocMem(mIonCameraClient, &mPictureBufDummy[i], 1 << 1) == false) {
            ALOGE("ERR(%s):mPictureBuf dummy allocMem() fail", __func__);
            return UNKNOWN_ERROR;
        } else {
            ALOGV("DEBUG(%s): mPictureBuf dummy allocMem adr(%p), size(%d), ion(%d) w/h(%d/%d)", __FUNCTION__,
                mPictureBufDummy[i].virt.extP[0], mPictureBufDummy[i].size.extS[0], mIonCameraClient,
                mFLiteCaptureSize.width, mFLiteCaptureSize.height);
            memset(mPictureBufDummy[i].virt.extP[0], 0, mPictureBufDummy[i].size.extS[0]);
        }
    }
#else
    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        err = mFlite.querybuf2(i, mFliteNode.planes, &mPictureBufDummy[i]);
        CHECK_ERR_N(err, ("nativeStartYUVSnapshot: error, mFlite.querybuf2"));
    }
#endif

    /* qbuf dummy buffer for skip */
    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        err = mFlite.qbufForCapture(&mPictureBufDummy[i], i);
        CHECK_ERR(err, ("nativeStartYUVSnapshot: error, mFlite.qbuf(%d)", i));
    }

    err = mFlite.stream(true);
    CHECK_ERR(err, ("nativeStartYUVSnapshot: error, mFlite.stream"));

    if ( mCaptureMode == RUNNING_MODE_SINGLE ) {
   //     PlayShutterSound();
//        Fimc_stream_true_part2();
    }

    ALOGD("nativeStartYUVSnapshot X");
    return true;
}

bool SecCameraHardware::nativeGetYUVSnapshot(int numF, int *postviewOffset)
{
    ALOGD("nativeGetYUVSnapshot E");

    int err;
    int i = 0;

retry:

//    err = mFlite.sctrl(CAM_CID_TRANSFER, numF);
    CHECK_ERR(err, ("nativeGetYUVSnapshot: error, capture start"))
    /*
     * Put here if Capture Start Command code is additionally needed
     * in case of ISP.
     */

    /*
     * Waiting for frame(stream) to be input.
     * ex) poll()
     */
    err = mFlite.polling();
    if (CC_UNLIKELY(err <= 0)) {
        LOG_FATAL("nativeGetYUVSnapshot: fail to get a frame!");
        return false;
    }
    ALOGV("DEBUG(%s): (%d) nativeGetYUVSnapshot dq start", __FUNCTION__, __LINE__);

    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        int ret = mFlite.dqbufForCapture(&mPictureBufDummy[i]);
        ALOGV("DEBUG(%s) (%d): dqbufForCapture dq(%d), ret = %d", __FUNCTION__, __LINE__, i, ret);
    }

    /* Stop capturing stream(or frame) data. */
    err = mFlite.stream(false);
    CHECK_ERR(err, ("nativeGetYUVSnapshot: error, mFlite.stream"));

#ifdef SAVE_DUMP
#if 0
    save_dump_path((uint8_t*)mPictureBufDummy[0].virt.extP[0],
            mPictureBufDummy[0].size.extS[0], "/data/dump_jpeg_only.jpg");
#endif
#endif

        if (!allocateHDRHeap()) {
            ALOGE("getEncodedJpeg: error, allocateSnapshotHeap");
            return false;
        }
        memcpy((char *)mHDRHeap->data, (uint8_t *)mPictureBufDummy[0].virt.extP[0], mHDRFrameSize);
        ALOGD("%s: mHDRHeap memcpy end size (mHDRFrameSize) = %d", __func__, mHDRFrameSize);
        //mGetMemoryCb(mPictureBufDummy[0].fd.extFd[0], mHDRFrameSize, 1, mCallbackCookie);

#if 0
    err = getYUV(numF);
    if (mCaptureMode == RUNNING_MODE_HDR) {
        if (numF == 1) {
            struct record_heap *scrab_heap = (struct record_heap *)mHDRHeap->data;

            scrab_heap[3].type = kMetadataBufferTypeCameraSource;
            scrab_heap[3].buf_index = 3;
            scrab_heap[3].reserved = (uint32_t)mPictureBufDummy[0].virt.extP[0];
            ALOGE("Scrab memory set to ION. scrab_heap[3].reserved = %08x", scrab_heap[3].reserved);
        }
    }
    CHECK_ERR(err, ("nativeGetYUVSnapshot: error, getYUV"));
#endif
    ALOGD("nativeGetYUVSnapshot X");
    return true;
}

/* --3 */
bool SecCameraHardware::nativeStartSnapshot()
{
    CLOGV("DEBUG (%s) : in ",__FUNCTION__);

    int err;
    int nBufs = 1;
    int i = 0;
    ExynosBuffer nullBuf;

    cam_pixel_format captureFormat = CAM_PIXEL_FORMAT_YUV422I;

#if FRONT_ZSL
    if (mCameraId == CAMERA_ID_FRONT && ISecCameraHardware::mFullPreviewRunning)
        return true;
#endif

    err = mFlite.startCapture(&mFLiteCaptureSize, captureFormat, nBufs, START_CAPTURE_YUV_MAIN);
    CHECK_ERR(err, ("nativeStartSnapshot: error, mFlite.start"));

    /*
       TODO : The only one buffer should be used
       between mPictureBuf and mPictureBufDummy in case of jpeg capture
     */
    /* For picture buffer */
#ifdef USE_NV21_CALLBACK
    if(mPictureFormat == CAM_PIXEL_FORMAT_YUV420SP) {
        getAlignedYUVSize(mPictureFormat, mPictureSize.width, mPictureSize.height, &mPictureBuf);
        mPictureBuf.size.extS[0] = mPictureBuf.size.extS[0] + mPictureBuf.size.extS[1];
        mPictureBuf.size.extS[1] = 0;
    }
    else
#endif
    {
        getAlignedYUVSize(captureFormat, mPictureSize.width, mPictureSize.height, &mPictureBuf);
    }

    if (allocMem(mIonCameraClient, &mPictureBuf, 1 << 1) == false) {
        CLOGE("ERR(%s):mPictureBuf allocMem() fail", __func__);
        return UNKNOWN_ERROR;
    } else {
        CLOGV("DEBUG(%s): mPictureBuf allocMem adr(%p), size(%d), ion(%d) w/h(%d/%d)", __FUNCTION__,
            mPictureBuf.virt.extP[0], mPictureBuf.size.extS[0], mIonCameraClient,
            mPictureSize.width, mPictureSize.height);
        memset(mPictureBuf.virt.extP[0], 0, mPictureBuf.size.extS[0]);
    }

#ifdef USE_USERPTR
    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        getAlignedYUVSize(captureFormat, mFLiteCaptureSize.width, mFLiteCaptureSize.height, &mPictureBufDummy[i]);
        if (allocMem(mIonCameraClient, &mPictureBufDummy[i], 1 << 1) == false) {
            CLOGE("ERR(%s):mPictureBuf dummy allocMem() fail", __func__);
            return UNKNOWN_ERROR;
        } else {
            CLOGD("DEBUG(%s): mPictureBuf dummy allocMem adr(%p), size(%d), ion(%d) w/h(%d/%d)", __FUNCTION__,
                mPictureBufDummy[i].virt.extP[0], mPictureBufDummy[i].size.extS[0], mIonCameraClient,
                mPictureSize.width, mPictureSize.height);
            memset(mPictureBufDummy[i].virt.extP[0], 0, mPictureBufDummy[i].size.extS[0]);
        }
    }
#else
    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        err = mFlite.querybuf2(i, mFliteNode.planes, &mPictureBufDummy[i]);
        CHECK_ERR_N(err, ("nativeStartPreviewZoom: error, mFlite.querybuf2"));
    }
#endif

    /* qbuf dummy buffer for skip */
    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        err = mFlite.qbufForCapture(&mPictureBufDummy[i], i);
        CHECK_ERR(err, ("nativeStartSnapshot: error, mFlite.qbuf(%d)", i));
    }

#if !defined(USE_USERPTR)
    /* export FD */
    for (int i = 0; i < SKIP_CAPTURE_CNT; i++) {
        err = mFlite.expBuf(i, mFliteNode.planes, &mPictureBufDummy[i]);
        CHECK_ERR_N(err, ("nativeStartSnapshot: error, mFlite.expBuf"));
    }
#endif

    err = mFlite.stream(true);
    CHECK_ERR(err, ("nativeStartSnapshot: error, mFlite.stream"));

    CLOGV("DEBUG (%s) : out ",__FUNCTION__);
    return true;
}

bool SecCameraHardware::nativeGetPostview(int numF)
{
    int err;
    int i = 0;

    ALOGD("nativeGetPostview E");
retry:

    /*
    * Put here if Capture Start Command code is needed in addition */
    #if 0
    if (mCameraId == CAMERA_ID_BACK) {
        err = mFlite.sctrl(CAM_CID_POSTVIEW_TRANSFER, numF);
        CHECK_ERR(err, ("nativeGetPostview: error, capture start"));
    }
    #endif
    /*
    * Waiting for frame(stream) to be input.
    * ex) poll()
    */
    err = mFlite.polling();
    if (CC_UNLIKELY(err <= 0)) {
#ifdef ISP_LOGWRITE
        ALOGE("polling error - SEC_ISP_DBG_logwrite = %s", __func__);
        SEC_ISP_DBG_logwrite();
#endif
        ALOGE("nativeGetPostview: error, mFlite.polling");
        return false;
    }

    /*
    * Get out a filled buffer from driver's queue. */
    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        int ret = mFlite.dqbufForCapture(&mPictureBufDummy[i]);
        ALOGV("DEBUG(%s) (%d): dqbufForCapture dq(%d), ret = %d", __FUNCTION__, __LINE__, i, ret);
    }

    /*
    * Stop capturing stream(or frame) data. */
    err = mFlite.stream(false);
    CHECK_ERR(err, ("nativeGetPostview: error, mFlite.stream"));

    if (mCameraId == CAMERA_ID_BACK) {
        err = getPostview(numF);
        CHECK_ERR(err, ("nativeGetPostview: error, getPostview"));
    }

    ALOGD("nativeGetPostview X");
    return true;
}

/* --4 */
bool SecCameraHardware::nativeGetSnapshot(int numF, int *postviewOffset)
{
    CLOGV("DEBUG (%s) : in ",__FUNCTION__);

    int err;
    int i = 0;
    bool retryDone = false;

#if FRONT_ZSL
    if (mCameraId == CAMERA_ID_FRONT && ISecCameraHardware::mFullPreviewRunning)
        return getZSLJpeg();
#endif

retry:

    /*
     * Waiting for frame(stream) to be input.
     * ex) poll()
     */
    err = mFlite.polling();
    if (CC_UNLIKELY(err <= 0)) {
#ifdef DEBUG_CAPTURE_RETRY
        LOG_FATAL("nativeGetSnapshot: fail to get a frame!");
#else
        if (!retryDone) {
            CLOGW("nativeGetSnapshot: warning. Reset the camera device");
            mFlite.stream(false);
            nativeStopSnapshot();
            mFlite.reset();
            nativeStartSnapshot();
            retryDone = true;
            goto retry;
        }
        CLOGE("nativeGetSnapshot: error, mFlite.polling");
#endif
        return false;
    }
    CLOGV("DEBUG(%s): (%d) nativeGetSnapshot dq start", __FUNCTION__, __LINE__);

    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        if(i > 0) {
            err = mFlite.polling();
        }
        int ret = mFlite.dqbufForCapture(&mPictureBufDummy[i]);
        CLOGD("DEBUG(%s) (%d): dqbufForCapture dq(%d), ret = %d", __FUNCTION__, __LINE__, i, ret);
    }

#ifdef SAVE_DUMP
    save_dump_path((uint8_t*)mPictureBufDummy[0].virt.extP[0],
            mPictureBufDummy[0].size.extS[0], "/data/dump_raw.yuv");
#endif

    /* Stop capturing stream(or frame) data. */
    err = mFlite.stream(false);
    CHECK_ERR(err, ("nativeGetSnapshot: error, mFlite.stream"));

    /* last capture was stored dummy buffer due to zoom.
     * and zoom applied to capture image by fimc */
    err = nativeCSCCapture(&mPictureBufDummy[i-1], &mPictureBuf);
    CHECK_ERR_N(err, ("nativeGetSnapshot: error, nativeCSCCapture"));

#ifdef SAVE_DUMP
    save_dump_path((uint8_t*)mPictureBufDummy[0].virt.extP[0],
            mPictureBufDummy[0].size.extS[0], "/data/dump_jpeg_only.jpg");
#endif
#ifdef USE_NV21_CALLBACK
    if (mPictureFormat == CAM_PIXEL_FORMAT_YUV420SP) {
        mPictureFrameSize = mPictureBuf.size.extS[0];
        if (!allocateSnapshotHeap()) {
            ALOGE("%s: error , allocateSnapshotHeap", __func__);
            return false;
        }
        memcpy((unsigned char *)mJpegHeap->data, mPictureBuf.virt.extP[0], mPictureBuf.size.extS[0]);
    }
    else
#endif
    {
        if (numF == 0 && mCaptureMode == RUNNING_MODE_RAW ) {    //(mCaptureMode == RUNNING_MODE_RAW ) {
            int jpegSize;
            nativeGetParameters(CAM_CID_JPEG_MAIN_SIZE, &jpegSize );
            mPictureFrameSize = (uint32_t)jpegSize;
        } else {
            /* Get Jpeg image including EXIF. */
            if (mCameraId == CAMERA_ID_BACK)
                err = getJpegOnBack(postviewOffset);
            else
                err = getJpegOnFront(postviewOffset);

            CHECK_ERR(err, ("nativeGetSnapshot: error, getJpeg"));
        }

#if 0
        if ((mMsgEnabled & CAMERA_MSG_RAW_IMAGE) || (mMsgEnabled & CAMERA_MSG_POSTVIEW_FRAME)) {
            int copySize = mRawFrameSize;
            if (mPictureBuf.size.extS[0] < copySize)
                copySize = mPictureBuf.size.extS[0];

            memcpy((char *)mRawHeap->data, mPictureBuf.virt.extP[0], copySize);
            ALOGV("DEBUG(%s): (%d) copied mRawHeap", __FUNCTION__, __LINE__);
        }
#endif

#ifdef DUMP_JPEG_FILE
        ExynosBuffer jpegBuf;
        CLOGV("DEBUG(%s): (%d) %d", __FUNCTION__, __LINE__, mPictureFrameSize);
        jpegBuf.size.extS[0] = mPictureFrameSize;
        jpegBuf.virt.extP[0] = (char *)mJpegHeap->data;
        CLOGV("DEBUG(%s): (%d)", __FUNCTION__, __LINE__);
        mSaveDump("/data/camera_capture%d.jpg", &jpegBuf, 1);
        CLOGV("DEBUG(%s): (%d)", __FUNCTION__, __LINE__);
#endif
    }
    CLOGV("DEBUG (%s) : out ",__FUNCTION__);
    return true;
}

bool SecCameraHardware::nativeStartDualCapture(int numF)
{
    int err;

    ALOGD("nativeStartDualCapture E - frame: %d", numF);
    err = mFlite.sctrl(V4L2_CID_CAMERA_SET_DUAL_CAPTURE, numF);
    CHECK_ERR(err, ("nativeStartDualCapture: error, capture start"));

    ALOGD("nativeStartDualCapture X");

    return true;
}

int SecCameraHardware::getYUV(int fnum)
{
    ALOGE("%s: start, fnum = %d", __func__, fnum);

    struct record_heap *heap = (struct record_heap *)mHDRHeap->data;
    int nAddr;

    nAddr = mPictureBufDummy[0].phys.extP[0];

    heap[fnum - 1].type = kMetadataBufferTypeCameraSource;
    heap[fnum - 1].y = nAddr;
    heap[fnum - 1].cbcr = nAddr + (mRawSize.width * mRawSize.height);
    heap[fnum - 1].buf_index = fnum - 1;
    /*
    Just 2 buffer of ION memories are allocated for HDR.
    Fimc0 memory is used for last one buffer to reduce
    the number of ION memory to 2 from 3.
    In case of 16M camera, 64M HDR memory is needed instead of 96M.
    */

    ALOGE("Fnum = %d, ION memory using", fnum);
#ifdef SUPPORT_64BITS
    heap[fnum - 1].reserved = (unsigned long)mPictureBufDummy[0].virt.extP[0];
#else
    heap[fnum - 1].reserved = (uint32_t)mPictureBufDummy[0].virt.extP[0];
#endif
#if 0
    if (fnum <= 2) {
        ALOGE("Fnum = %d, ION memory using", fnum);
        heap[fnum - 1].reserved = (uint32_t*)mPictureBufDummy[0].virt.extP[0];
    } else {
        uint32_t last_frame = (uint32_t)mRawHeap->base();
        heap[fnum - 1].reserved = last_frame;
        ALOGE("Fnum = %d, Fimc0 memory using", fnum);
    }
#endif

    ALOGE("getYUV hdrheappointer(ion) : %x, mHDRFrameSize = %d", heap[fnum - 1].reserved, mHDRFrameSize);

    /* Note : Sensor return main image size, not only JPEG but also YUV. */
    //err = mFlite.gctrl(V4L2_CID_CAM_JPEG_MAIN_SIZE, &yuvSize);
    //CHECK_ERR(err, ("getYUV: error %d, jpeg size", err));
    //mRawFrameSize = yuvSize;
    mRawFrameSize = mRawSize.width*mRawSize.height*2; /* yuv422I */

    /* picture frame size is should be calculated before call allocatePostviewHeap */
#ifdef SUPPORT_64BITS
    char *postview = (char *)mPictureBufDummy[0].virt.extP[0];
    memcpy((char *)(unsigned long)heap[fnum - 1].reserved , postview, mRawFrameSize);
#else
    uint8_t *postview = (uint8_t *)mPictureBufDummy[0].virt.extP[0];
    memcpy((void*)heap[fnum - 1].reserved , postview, mRawFrameSize);
#endif
#if 0    /* FIMC output data */
        if (fnum == 1)
            save_dump_path(postview, mRawFrameSize, "/data/dump_HDR1.yuv");
        if (fnum == 2)
            save_dump_path(postview, mRawFrameSize, "/data/dump_HDR2.yuv");
        if (fnum == 3)
            save_dump_path(postview, mRawFrameSize, "/data/dump_HDR3.yuv");
#endif
#if 0    /* ION memory data */
        if (fnum == 1)
            save_dump_path((uint8_t*)mPictureBufDummy[0].virt.extP[0], mHDRFrameSize, "/data/dump_HDR1.yuv");
        if (fnum == 2)
            save_dump_path((uint8_t*)mPictureBufDummy[0].virt.extP[0], mHDRFrameSize, "/data/dump_HDR2.yuv");
        if (fnum == 3)
            save_dump_path((uint8_t*)mPictureBufDummy[0].virt.extP[0], mHDRFrameSize, "/data/dump_HDR3.yuv");
#endif
#if 0    /* ION memory data through mHDRHeap pointer */
        if (fnum == 1)
            save_dump_path((uint8_t*)(((struct record_heap *)mHDRHeap->data)[fnum - 1].reserved), mHDRFrameSize, "/data/dump_HDR1.yuv");
        if (fnum == 2)
            save_dump_path((uint8_t*)(((struct record_heap *)mHDRHeap->data)[fnum - 1].reserved), mHDRFrameSize, "/data/dump_HDR2.yuv");
        if (fnum == 3)
            save_dump_path((uint8_t*)(((struct record_heap *)mHDRHeap->data)[fnum - 1].reserved), mHDRFrameSize, "/data/dump_HDR3.yuv");
#endif

    ALOGD("%s: mPostviewHeap memcpy end size (mRawFrameSize) = %d", __func__, mRawFrameSize);

    return 0;
}

#ifndef RCJUNG
int SecCameraHardware::getOneYUV()
{
    ALOGE("%s: start", __func__);

    int yuvSize = 0;
    int err;

    /* Note : Sensor return main image size, not only JPEG but also YUV. */
    err = mFlite.gctrl(V4L2_CID_CAM_JPEG_MAIN_SIZE, &yuvSize);
    CHECK_ERR(err, ("getYUV: error %d, jpeg size", err));

    /* picture frame size is should be calculated before call allocateYUVHeap */
    mRawSize = mPictureSize;
    mRawFrameSize = mRawSize.width * mRawSize.height * 2; /* yuv422I */

    setExifChangedAttribute();

    uint8_t *YUVmain = (uint8_t *)mPictureBufDummy[0].virt.extP[0];

    if (!allocateYUVHeap()) {
        ALOGE("getYUV: error, allocateYUVHeap");
        return false;
    }

    memcpy(mYUVHeap->data, YUVmain, mRawFrameSize);

    ALOGD("%s: mYUVHeap memcpy end size (mRawFrameSize) = %d", __func__, mRawFrameSize);

    return true;
}
#endif

int SecCameraHardware::getPostview(int num)
{
    ALOGD("%s: start", __func__);

    /* picture frame size is should be calculated before call allocatePostviewHeap */
    if (!allocatePostviewHeap()) {
        ALOGE("getPostview: error, allocatePostviewHeap");
        return UNKNOWN_ERROR;
    }

    uint8_t *postview = (uint8_t *)mPictureBufDummy[0].virt.extP[0];

    memcpy((char *)mPostviewHeap->data, postview, mPostviewFrameSize);

#ifdef SAVE_DUMP
#if 0
    char *fileName = NULL;
    sprintf(fileName, "%s_%d.yuv%c", "/data/dump_postview_yuv", num, NULL);
    ALOGD("getPostview: dump postview image = %s", fileName);
    save_dump_path((uint8_t *)mPictureBufDummy[0].virt.extP[0], mPictureBufDummy[0].size.extS[0], fileName);
#endif
#endif

    ALOGE("%s: Postview memcpy end size = %d", __func__, mPostviewFrameSize);

    return 0;
}

inline int SecCameraHardware::getJpegOnBack(int *postviewOffset)
{
    status_t ret;

#if !defined(REAR_USE_YUV_CAPTURE)
    if (mCaptureMode == RUNNING_MODE_RAW)
        return internalGetJpegForRawWithZoom(postviewOffset);
    else
        return internalGetJpegForSocYuvWithZoom(postviewOffset);
#else
    if (mEnableDZoom)
        ret = internalGetJpegForSocYuvWithZoom(postviewOffset);
    else
        ret = internalGetJpegForSocYuv(postviewOffset);

    return ret;
#endif
}

inline int SecCameraHardware::getJpegOnFront(int *postviewOffset)
{
    status_t ret;
    ret = internalGetJpegForSocYuvWithZoom(postviewOffset);
    return ret;
}

void SecCameraHardware::nativeStopSnapshot()
{
    ExynosBuffer nullBuf;
    int i = 0;

    if (mRawHeap != NULL) {
        mRawHeap->release(mRawHeap);
        mRawHeap = 0;
    }

    freeMem(&mPictureBuf);

#ifdef USE_USERPTR
    /* capture buffer free */
    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        freeMem(&mPictureBufDummy[i]);
        mPictureBufDummy[i] = nullBuf;
    }
#else
    for (i = 0; i < SKIP_CAPTURE_CNT; i++) {
        for (int j = 0; j < mFliteNode.planes; j++) {
            munmap((void *)mPictureBufDummy[i].virt.extP[j],
                mPictureBufDummy[i].size.extS[j]);
            ion_free(mPictureBufDummy[i].fd.extFd[j]);
        }
        mPictureBufDummy[i] = nullBuf;
    }

    if (mFlite.reqBufZero(&mFliteNode) < 0)
        ALOGE("ERR(%s): mFlite.reqBufZero() fail", __func__);
#endif


    mPictureBuf = nullBuf;
    CLOGD("DEBUG (%s) : out ",__FUNCTION__);
}

bool SecCameraHardware::nativeSetAutoFocus()
{
    ALOGV("nativeSetAutofocus E");

    int i, waitUs = 10000, tryCount = (900 * 1000) / waitUs;
    for (i = 1; i <= tryCount; i++) {
        if (mPreviewInitialized)
            break;
        else
            usleep(waitUs);

        if (!(i % 40))
            ALOGD("AF: waiting for preview\n");
    }

    if (CC_UNLIKELY(i > tryCount))
        ALOGI("cancelAutoFocus: cancel timeout");

    if (!IsAutofocusRunning()) {
        ALOGW("nativeSetAutofocus X: AF Cancel is called");
        return true;
    }

    int err = mFlite.sctrl(V4L2_CID_CAM_SINGLE_AUTO_FOCUS, AUTO_FOCUS_ON);
    CHECK_ERR(err, ("nativeSetAutofocus X: error, mFlite.sctrl"))

    ALOGV("nativeSetAutofocus X");

    return true;
}

int SecCameraHardware::nativeGetPreAutoFocus()
{
    ALOGV("nativeGetPreAutofocus E");
    int status, i;

    const int tryCount = 500;

    usleep(150000);

    for (i = 0 ; i < tryCount ; i ++) {
        int err;
        err = mFlite.gctrl(V4L2_CID_CAM_AUTO_FOCUS_RESULT, &status);
        CHECK_ERR_N(err, ("nativeGetPreAutoFocus: error %d", err));
        if (status != 0x0)
            break;
        usleep(10000);
    }

    ALOGV("nativeGetPreAutofocus X %d", status);
    return status;
}

int SecCameraHardware::nativeGetAutoFocus()
{
    ALOGV("nativeGetAutofocus E");
    int status, i;
    /* AF completion takes more much time in case of night mode.
     So be careful if you modify tryCount. */
    const int tryCount = 300;

    for (i = 0; i < tryCount; i ++) {
        int err;
        usleep(20000);
        err = mFlite.gctrl(V4L2_CID_CAMERA_AUTO_FOCUS_DONE, &status);
        CHECK_ERR_N(err, ("nativeGetAutofocus: error %d", err));
        if ((status != 0x0) && (status != 0x08)) {
            break;
        }
    }

    if (i == tryCount)
        ALOGE("nativeGetAutoFocus: error, AF hasn't been finished yet.");

    ALOGV("nativeGetAutofocus X");
    return status;
}

status_t SecCameraHardware::nativeCancelAutoFocus()
{
    ALOGV("nativeCancelAutofocus E1");
//#if NOTDEFINED
//    int err = mFlite.sctrl(V4L2_CID_CAMERA_CANCEL_AUTO_FOCUS, 0);
    int err = mFlite.sctrl(V4L2_CID_CAM_SINGLE_AUTO_FOCUS, AUTO_FOCUS_OFF);
    CHECK_ERR(err, ("nativeCancelAutofocus: error, mFlite.sctrl"))
//#endif
    ALOGV("nativeCancelAutofocus X2");
    return NO_ERROR;
}

inline status_t SecCameraHardware::nativeSetParameters(cam_control_id id, int value, bool recordingMode)
{
    int err = NO_ERROR;

    if (CC_LIKELY(!recordingMode))
        err = mFlite.sctrl(id, value);
    else {
        if (mCameraId == CAMERA_ID_FRONT)
            err = mFlite.sctrl(id, value);
    }

    return NO_ERROR;
}

inline status_t SecCameraHardware::nativeGetParameters(cam_control_id id, int *value, bool recordingMode)
{
    int err = NO_ERROR;

    if (CC_LIKELY(!recordingMode))
        err = mFlite.gctrl(id, value);

    CHECK_ERR_N(err, ("nativeGetParameters X: error %d", err))

    return NO_ERROR;
}

void SecCameraHardware::setExifFixedAttribute()
{
    char property[PROPERTY_VALUE_MAX];

    CLEAR(mExifInfo);
    /* 0th IFD TIFF Tags */
    /* Maker */
    property_get("ro.product.manufacturer", property, Exif::DEFAULT_MAKER);
    strncpy((char *)mExifInfo.maker, property,
                sizeof(mExifInfo.maker) - 1);
    mExifInfo.maker[sizeof(mExifInfo.maker) - 1] = '\0';

    /* Model */
    property_get("ro.product.model", property, Exif::DEFAULT_MODEL);
    strncpy((char *)mExifInfo.model, property,
                sizeof(mExifInfo.model) - 1);
    mExifInfo.model[sizeof(mExifInfo.model) - 1] = '\0';

    /* Software */
    property_get("ro.build.PDA", property, Exif::DEFAULT_SOFTWARE);
    strncpy((char *)mExifInfo.software, property,
                sizeof(mExifInfo.software) - 1);
    mExifInfo.software[sizeof(mExifInfo.software) - 1] = '\0';

    /* YCbCr Positioning */
    mExifInfo.ycbcr_positioning = Exif::DEFAULT_YCBCR_POSITIONING;

    /* 0th IFD Exif Private Tags */
    /* F Number */
    if (mCameraId == CAMERA_ID_BACK) {
        mExifInfo.fnumber.num = Exif::DEFAULT_BACK_FNUMBER_NUM;
        mExifInfo.fnumber.den = Exif::DEFAULT_BACK_FNUMBER_DEN;
    } else {
        mExifInfo.fnumber.num = Exif::DEFAULT_FRONT_FNUMBER_NUM;
        mExifInfo.fnumber.den = Exif::DEFAULT_FRONT_FNUMBER_DEN;
    }

    /* Exposure Program */
    mExifInfo.exposure_program = Exif::DEFAULT_EXPOSURE_PROGRAM;

    /* Exif Version */
    memcpy(mExifInfo.exif_version, Exif::DEFAULT_EXIF_VERSION, sizeof(mExifInfo.exif_version));

    /* Aperture */
    double av = APEX_FNUM_TO_APERTURE((double)mExifInfo.fnumber.num/mExifInfo.fnumber.den);
    mExifInfo.aperture.num = av*Exif::DEFAULT_APEX_DEN;
    mExifInfo.aperture.den = Exif::DEFAULT_APEX_DEN;

    /* Maximum lens aperture */
    mExifInfo.max_aperture.num = mExifInfo.aperture.num;
    mExifInfo.max_aperture.den = mExifInfo.aperture.den;

    /* Lens Focal Length */
    if (mCameraId == CAMERA_ID_BACK) {
        mExifInfo.focal_length.num = Exif::DEFAULT_BACK_FOCAL_LEN_NUM;
        mExifInfo.focal_length.den = Exif::DEFAULT_BACK_FOCAL_LEN_DEN;
    } else {
        mExifInfo.focal_length.num = Exif::DEFAULT_FRONT_FOCAL_LEN_NUM;
        mExifInfo.focal_length.den = Exif::DEFAULT_FRONT_FOCAL_LEN_DEN;
    }

    /* Lens Focal Length in 35mm film*/
    if (mCameraId == CAMERA_ID_BACK) {
        mExifInfo.focal_35mm_length = Exif::DEFAULT_BACK_FOCAL_LEN_35mm;
    } else {
        mExifInfo.focal_35mm_length = Exif::DEFAULT_FRONT_FOCAL_LEN_35mm;
    }

    /* Color Space information */
    mExifInfo.color_space = Exif::DEFAULT_COLOR_SPACE;

    /* Exposure Mode */
    mExifInfo.exposure_mode = Exif::DEFAULT_EXPOSURE_MODE;

    /* Sensing Method */
    mExifInfo.sensing_method = Exif::DEFAULT_SENSING_METHOD;

    /* 0th IFD GPS Info Tags */
    unsigned char gps_version[4] = {0x02, 0x02, 0x00, 0x00};
    memcpy(mExifInfo.gps_version_id, gps_version, sizeof(gps_version));

    /* 1th IFD TIFF Tags */
    mExifInfo.compression_scheme = Exif::DEFAULT_COMPRESSION;
    mExifInfo.x_resolution.num = Exif::DEFAULT_RESOLUTION_NUM;
    mExifInfo.x_resolution.den = Exif::DEFAULT_RESOLUTION_DEN;
    mExifInfo.y_resolution.num = Exif::DEFAULT_RESOLUTION_NUM;
    mExifInfo.y_resolution.den = Exif::DEFAULT_RESOLUTION_DEN;
    mExifInfo.resolution_unit = Exif::DEFAULT_RESOLUTION_UNIT;
}

void SecCameraHardware::setExifChangedAttribute()
{
    /* 0th IFD TIFF Tags */
    /* Width, Height */
    if (!mMovieMode) {
        mExifInfo.width = mPictureSize.width;
        mExifInfo.height = mPictureSize.height;
    } else {
        mExifInfo.width = mVideoSize.width;
        mExifInfo.height = mVideoSize.height;
    }

    /* ISP firmware version */
#ifdef SENSOR_FW_GET_FROM_FILE
    char *camera_fw = NULL;
    char *savePtr = NULL;
    char sensor_fw[12];
    size_t sizes = sizeof(sensor_fw) / sizeof(sensor_fw[0]);
    camera_fw = strtok_r((char *)getSensorFWFromFile(sensor_fw, sizes, mCameraId), " ", &savePtr);
    strncpy((char *)mExifInfo.unique_id, camera_fw, sizeof(mExifInfo.unique_id) - 1);
    ALOGD("Exif: unique_id = %s", mExifInfo.unique_id);
#else
    char unique_id[12] = {'\0',};
    mFlite.gctrl(V4L2_CID_CAM_SENSOR_FW_VER, unique_id, 12);
    ALOGD("Exif: unique_id = %s", unique_id);
    strncpy((char *)mExifInfo.unique_id, unique_id, sizeof(mExifInfo.unique_id) - 1);
    mExifInfo.unique_id[sizeof(mExifInfo.unique_id) - 1] = '\0';
#endif

    /* Orientation */
    switch (mParameters.getInt(CameraParameters::KEY_ROTATION)) {
    case 90:
        mExifInfo.orientation = EXIF_ORIENTATION_90;
        break;
    case 180:
        mExifInfo.orientation = EXIF_ORIENTATION_180;
        break;
    case 270:
        mExifInfo.orientation = EXIF_ORIENTATION_270;
        break;
    case 0:
    default:
        mExifInfo.orientation = EXIF_ORIENTATION_UP;
        break;
    }
    ALOGD("Exif: setRotation = %d, orientation = %d", mParameters.getInt(CameraParameters::KEY_ROTATION), mExifInfo.orientation);

    /* Date time */
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime((char *)mExifInfo.date_time, 20, "%Y:%m:%d %H:%M:%S", timeinfo);

    /* 0th IFD Exif Private Tags */
    /* Exposure Time */
    int err;
    int exposureTimeNum;
    int exposureTimeDen;
    int rt_num, rt_den;

    err = mFlite.gctrl(V4L2_CID_EXIF_EXPOSURE_TIME_NUM, &exposureTimeNum);
    if (err < 0)
        ALOGE("setExifChangedAttribute: exposure time num err = %d", err);
    err = mFlite.gctrl(V4L2_CID_EXIF_EXPOSURE_TIME_DEN, &exposureTimeDen);
    if (err < 0)
        ALOGE("setExifChangedAttribute: exposure time den err = %d", err);
    if (exposureTimeNum > 0) {
        mExifInfo.exposure_time.num = exposureTimeNum;
    } else {
        ALOGE("exposureTimeNum is negative. set to 1");
        mExifInfo.exposure_time.num = 1;
    }
    mExifInfo.exposure_time.den = exposureTimeDen;
    ALOGD("Exif: exposure time num = %d,  den = %d", exposureTimeNum, exposureTimeDen);

    /* Shutter Speed */
    double exposure = (double)mExifInfo.exposure_time.den / (double)mExifInfo.exposure_time.num;
    mExifInfo.shutter_speed.num = APEX_EXPOSURE_TO_SHUTTER(exposure) * Exif::DEFAULT_APEX_DEN;
    if(mExifInfo.shutter_speed.num < 0)
        mExifInfo.shutter_speed.num = 0;
    mExifInfo.shutter_speed.den = Exif::DEFAULT_APEX_DEN;
    ALOGD("Exif: shutter speed num = %d, den = %d", mExifInfo.shutter_speed.num, mExifInfo.shutter_speed.den);

    /* Flash */
    if (mCameraId == CAMERA_ID_BACK) {
        err = mFlite.gctrl(V4L2_CID_CAMERA_EXIF_FLASH, (int *)&mExifInfo.flash);
        if (err < 0)
            ALOGE("setExifChangedAttribute: Flash value err = %d", err);
        ALOGD("mEixfInfo.flash = %x", mExifInfo.flash);
    }

    /* Color Space information */
    mExifInfo.color_space = Exif::DEFAULT_COLOR_SPACE;

    /* User Comments */
    strncpy((char *)mExifInfo.user_comment, Exif::DEFAULT_USERCOMMENTS, sizeof(mExifInfo.user_comment) - 1);
    /* ISO Speed Rating */
    err = mFlite.gctrl(V4L2_CID_CAMERA_EXIF_ISO, (int *)&mExifInfo.iso_speed_rating);
    if (err < 0)
        ALOGE("setExifChangedAttribute: ISO Speed Rating err = %d", err);

    /* Brightness */
    int bv;
    err = mFlite.gctrl(V4L2_CID_CAMERA_EXIF_BV, &bv);
    if (err < 0)
        ALOGE("setExifChangedAttribute: Brightness value err = %d", err);
    mExifInfo.brightness.num = bv;
    mExifInfo.brightness.den = Exif::DEFAULT_APEX_DEN;

    if (mCameraId == CAMERA_ID_BACK) {
        /* Exposure Bias */
        float exposure = mParameters.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION) *
            mParameters.getFloat(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP);
        mExifInfo.exposure_bias.num = (exposure * Exif::DEFAULT_APEX_DEN);
        mExifInfo.exposure_bias.den = Exif::DEFAULT_APEX_DEN;
    }

    /* Metering Mode */
    const char *metering = mParameters.get("metering");
    if (!metering || !strcmp(metering, "center"))
        mExifInfo.metering_mode = EXIF_METERING_CENTER;
    else if (!strcmp(metering, "spot"))
        mExifInfo.metering_mode = EXIF_METERING_SPOT;
    else if (!strcmp(metering, "matrix"))
        mExifInfo.metering_mode = EXIF_METERING_AVERAGE;

    /* White Balance */
    const char *wb = mParameters.get(CameraParameters::KEY_WHITE_BALANCE);
    if (!wb || !strcmp(wb, CameraParameters::WHITE_BALANCE_AUTO))
        mExifInfo.white_balance = EXIF_WB_AUTO;
    else
        mExifInfo.white_balance = EXIF_WB_MANUAL;

    /* Scene Capture Type */
    switch (mSceneMode) {
    case SCENE_MODE_PORTRAIT:
        mExifInfo.scene_capture_type = EXIF_SCENE_PORTRAIT;
        break;
    case SCENE_MODE_LANDSCAPE:
        mExifInfo.scene_capture_type = EXIF_SCENE_LANDSCAPE;
        break;
    case SCENE_MODE_NIGHTSHOT:
        mExifInfo.scene_capture_type = EXIF_SCENE_NIGHT;
        break;
    default:
        mExifInfo.scene_capture_type = EXIF_SCENE_STANDARD;
        break;
    }

    /* 0th IFD GPS Info Tags */
    const char *strLatitude = mParameters.get(CameraParameters::KEY_GPS_LATITUDE);
    const char *strLogitude = mParameters.get(CameraParameters::KEY_GPS_LONGITUDE);
    const char *strAltitude = mParameters.get(CameraParameters::KEY_GPS_ALTITUDE);

    if (strLatitude != NULL && strLogitude != NULL && strAltitude != NULL) {
        if (atof(strLatitude) > 0) {
            strncpy((char *)mExifInfo.gps_latitude_ref, "N", sizeof(mExifInfo.gps_latitude_ref) - 1);
         } else {
            strncpy((char *)mExifInfo.gps_latitude_ref, "S", sizeof(mExifInfo.gps_latitude_ref) - 1);
         }
         mExifInfo.gps_latitude_ref[sizeof(mExifInfo.gps_latitude_ref) - 1] = '\0';

        if (atof(strLogitude) > 0){
            strncpy((char *)mExifInfo.gps_longitude_ref, "E", sizeof(mExifInfo.gps_longitude_ref) - 1);
        } else {
            strncpy((char *)mExifInfo.gps_longitude_ref, "W", sizeof(mExifInfo.gps_longitude_ref) - 1);
        }
        mExifInfo.gps_longitude_ref[sizeof(mExifInfo.gps_longitude_ref) - 1] = '\0';

        if (atof(strAltitude) > 0)
            mExifInfo.gps_altitude_ref = 0;
        else
            mExifInfo.gps_altitude_ref = 1;

        double latitude = fabs(atof(strLatitude));
        double longitude = fabs(atof(strLogitude));
        double altitude = fabs(atof(strAltitude));

        mExifInfo.gps_latitude[0].num = (uint32_t)latitude;
        mExifInfo.gps_latitude[0].den = 1;
        mExifInfo.gps_latitude[1].num = (uint32_t)((latitude - mExifInfo.gps_latitude[0].num) * 60);
        mExifInfo.gps_latitude[1].den = 1;
        mExifInfo.gps_latitude[2].num = (uint32_t)(round((((latitude - mExifInfo.gps_latitude[0].num) * 60) -
                    mExifInfo.gps_latitude[1].num) * 60));
        mExifInfo.gps_latitude[2].den = 1;

        mExifInfo.gps_longitude[0].num = (uint32_t)longitude;
        mExifInfo.gps_longitude[0].den = 1;
        mExifInfo.gps_longitude[1].num = (uint32_t)((longitude - mExifInfo.gps_longitude[0].num) * 60);
        mExifInfo.gps_longitude[1].den = 1;
        mExifInfo.gps_longitude[2].num = (uint32_t)(round((((longitude - mExifInfo.gps_longitude[0].num) * 60) -
                    mExifInfo.gps_longitude[1].num) * 60));
        mExifInfo.gps_longitude[2].den = 1;

        mExifInfo.gps_altitude.num = (uint32_t)altitude;
        mExifInfo.gps_altitude.den = 1;

        const char *strTimestamp = mParameters.get(CameraParameters::KEY_GPS_TIMESTAMP);
        long timestamp = 0;
        if (strTimestamp)
            timestamp = atol(strTimestamp);

        struct tm tm_data;
        gmtime_r(&timestamp, &tm_data);
        mExifInfo.gps_timestamp[0].num = tm_data.tm_hour;
        mExifInfo.gps_timestamp[0].den = 1;
        mExifInfo.gps_timestamp[1].num = tm_data.tm_min;
        mExifInfo.gps_timestamp[1].den = 1;
        mExifInfo.gps_timestamp[2].num = tm_data.tm_sec;
        mExifInfo.gps_timestamp[2].den = 1;
        strftime((char *)mExifInfo.gps_datestamp, sizeof(mExifInfo.gps_datestamp), "%Y:%m:%d", &tm_data);

        const char *progressingMethod = mParameters.get(CameraParameters::KEY_GPS_PROCESSING_METHOD);
        if (progressingMethod) {
            size_t len = strlen(progressingMethod);
            if (len >= sizeof(mExifInfo.gps_processing_method))
                len = sizeof(mExifInfo.gps_processing_method) - 1;

            CLEAR(mExifInfo.gps_processing_method);
            strncpy((char *)mExifInfo.gps_processing_method, progressingMethod, len);
        }

        mExifInfo.enableGps = true;
    } else {
        mExifInfo.enableGps = false;
    }

    /* 1th IFD TIFF Tags */
    mExifInfo.widthThumb = mThumbnailSize.width;
    mExifInfo.heightThumb = mThumbnailSize.height;

}

int SecCameraHardware::internalGetJpegForSocYuvWithZoom(int *postviewOffset)
{
    ExynosBuffer thumbnailJpeg;
    ExynosBuffer rawThumbnail;
    ExynosBuffer jpegOutBuf;
    ExynosBuffer exifOutBuf;
    ExynosBuffer nullBuf;
    Exif exif(mCameraId, CAMERA_TYPE_SOC);
    int i;

    uint8_t *thumb;
    int32_t thumbSize;
    bool thumbnail = false;
    int err = -1;
    int ret = UNKNOWN_ERROR;

    s5p_rect zoomRect;
    CLOGD("DEBUG(%s)(%d): picture size(%d/%d)", __FUNCTION__, __LINE__, mPictureSize.width, mPictureSize.height);
    CLOGD("DEBUG(%s)(%d): thumbnail size(%d/%d)", __FUNCTION__, __LINE__, mThumbnailSize.width, mThumbnailSize.height);

    zoomRect.w = (uint32_t)((float)mPictureSize.width * 1000 / mZoomRatio);
    zoomRect.h = (uint32_t)((float)mPictureSize.height * 1000 / mZoomRatio);

    if (zoomRect.w % 2)
        zoomRect.w -= 1;

    if (zoomRect.h % 2)
        zoomRect.h -= 1;

    zoomRect.x = (mPictureSize.width - zoomRect.w) / 2;
    zoomRect.y = (mPictureSize.height - zoomRect.h) / 2;

    if (zoomRect.x % 2)
        zoomRect.x -= 1;

    if (zoomRect.y % 2)
        zoomRect.y -= 1;

    /* Making thumbnail image */
    if (mThumbnailSize.width == 0 || mThumbnailSize.height == 0)
        goto encodeJpeg;

    /* alloc rawThumbnail */
    rawThumbnail.size.extS[0] = mThumbnailSize.width * mThumbnailSize.height * 2;
    if (allocMem(mIonCameraClient, &rawThumbnail, 1 << 1) == false) {
        CLOGE("ERR(%s): rawThumbnail allocMem() fail", __func__);
        goto destroyMem;
    } else {
        CLOGV("DEBUG(%s): rawThumbnail allocMem adr(%p), size(%d), ion(%d)", __FUNCTION__,
            rawThumbnail.virt.extP[0], rawThumbnail.size.extS[0], mIonCameraClient);
        memset(rawThumbnail.virt.extP[0], 0, rawThumbnail.size.extS[0]);
    }

    if (mPictureBuf.size.extS[0] <= 0) {
        goto destroyMem;
    }

    LOG_PERFORMANCE_START(3);

    scaleDownYuv422((unsigned char *)mPictureBuf.virt.extP[0],
                    (int)mPictureSize.width,
                    (int)mPictureSize.height,
                    (unsigned char *)rawThumbnail.virt.extP[0],
                    (int)mThumbnailSize.width,
                    (int)mThumbnailSize.height);
    LOG_PERFORMANCE_END(3, "scaleDownYuv422");

    /* alloc thumbnailJpeg */
    thumbnailJpeg.size.extS[0] = mThumbnailSize.width * mThumbnailSize.height * 2;
    if (allocMem(mIonCameraClient, &thumbnailJpeg, 1 << 1) == false) {
        CLOGE("ERR(%s): thumbnailJpeg allocMem() fail", __func__);
        goto destroyMem;
    } else {
        CLOGV("DEBUG(%s): thumbnailJpeg allocMem adr(%p), size(%d), ion(%d)", __FUNCTION__,
            thumbnailJpeg.virt.extP[0], thumbnailJpeg.size.extS[0], mIonCameraClient);
        memset(thumbnailJpeg.virt.extP[0], 0, thumbnailJpeg.size.extS[0]);
    }

    err = EncodeToJpeg(&rawThumbnail,
                       &thumbnailJpeg,
                        mThumbnailSize.width,
                        mThumbnailSize.height,
                        CAM_PIXEL_FORMAT_YUV422I,
                        &thumbSize,
                        JPEG_THUMBNAIL_QUALITY);
    CHECK_ERR_GOTO(encodeJpeg, err, ("getJpeg: error, EncodeToJpeg(thumbnail)"));

    thumb = (uint8_t *)thumbnailJpeg.virt.extP[0];
    thumbnail = true;

    LOG_PERFORMANCE_END(1, "encode thumbnail");

encodeJpeg:
    /* Making EXIF header */

    setExifChangedAttribute();

    uint32_t exifSize;
    int32_t jpegSize;
    uint8_t *jpeg;

    /* alloc exifOutBuf */
    exifOutBuf.size.extS[0] = EXIF_MAX_LEN;
    if (allocMem(mIonCameraClient, &exifOutBuf, 1 << 1) == false) {
        CLOGE("ERR(%s): exifTmpBuf allocMem() fail", __func__);
        goto destroyMem;
    } else {
        CLOGV("DEBUG(%s): exifTmpBuf allocMem adr(%p), size(%d), ion(%d)", __FUNCTION__,
            exifOutBuf.virt.extP[0], exifOutBuf.size.extS[0], mIonCameraClient);
        memset(exifOutBuf.virt.extP[0], 0, exifOutBuf.size.extS[0]);
    }

    if (!thumbnail)
        exifSize = exif.make((void *)exifOutBuf.virt.extP[0], &mExifInfo);
    else
        exifSize = exif.make((void *)exifOutBuf.virt.extP[0], &mExifInfo, exifOutBuf.size.extS[0], thumb, thumbSize);
    if (CC_UNLIKELY(!exifSize)) {
        CLOGE("ERR(%s): getJpeg: error, fail to make EXIF", __FUNCTION__);
        goto destroyMem;
    }

    /* alloc jpegOutBuf */
    jpegOutBuf.size.extS[0] = mPictureSize.width * mPictureSize.height * 2;
    if (allocMem(mIonCameraClient, &jpegOutBuf, 1 << 1) == false) {
        CLOGE("ERR(%s): jpegOutBuf allocMem() fail", __func__);
        goto destroyMem;
    } else {
        CLOGV("DEBUG(%s): jpegOutBuf allocMem adr(%p), size(%d), ion(%d)", __FUNCTION__,
            jpegOutBuf.virt.extP[0], jpegOutBuf.size.extS[0], mIonCameraClient);
        memset(jpegOutBuf.virt.extP[0], 0, jpegOutBuf.size.extS[0]);
    }

    /* Making main jpeg image */
    err = EncodeToJpeg(&mPictureBuf,
                       &jpegOutBuf,
                        mPictureSize.width,
                        mPictureSize.height,
                        CAM_PIXEL_FORMAT_YUV422I,
                        &jpegSize,
                        mJpegQuality);

    CHECK_ERR_GOTO(destroyMem, err, ("getJpeg: error, EncodeToJpeg(Main)"));

    /* Finally, Creating Jpeg image file including EXIF */
    mPictureFrameSize = jpegSize + exifSize;

    /*note that picture frame size is should be calculated before call allocateSnapshotHeap */
    if (!allocateSnapshotHeap()) {
        ALOGE("getEncodedJpeg: error, allocateSnapshotHeap");
        goto destroyMem;
    }

    jpeg = (unsigned char *)jpegOutBuf.virt.extP[0];
    memcpy((unsigned char *)mJpegHeap->data, jpeg, 2);
    memcpy((unsigned char *)mJpegHeap->data + 2, exifOutBuf.virt.extP[0], exifSize);
    memcpy((unsigned char *)mJpegHeap->data + 2 + exifSize, jpeg + 2, jpegSize - 2);
    CLOGD("DEBUG(%s): success making jpeg(%d) and exif(%d)", __FUNCTION__, jpegSize, exifSize);

    ret = NO_ERROR;

destroyMem:
    freeMem(&thumbnailJpeg);
    freeMem(&rawThumbnail);
    freeMem(&jpegOutBuf);
    freeMem(&exifOutBuf);

    return ret;
}

int SecCameraHardware::internalGetJpegForRawWithZoom(int *postviewOffset)
{
    ExynosBuffer thumbnailJpeg;
    ExynosBuffer rawThumbnail;
    ExynosBuffer jpegOutBuf;
    ExynosBuffer exifOutBuf;
    ExynosBuffer nullBuf;
    Exif exif(mCameraId);
    int i;

    uint8_t *thumb;
    bool thumbnail = false;
    int err;
    int ret = UNKNOWN_ERROR;

    int32_t jpegSize = -1;
    int32_t jpegOffset;
    int32_t thumbSize = 0;
    int32_t thumbOffset;

    err = mFlite.gctrl(V4L2_CID_CAM_JPEG_MAIN_SIZE, &jpegSize);
    CHECK_ERR(err, ("getJpeg: error %d, jpeg size", err));

    ALOGD("internalgetJpeg: jpegSize = %d", jpegSize);

    err = mFlite.gctrl(V4L2_CID_CAM_JPEG_MAIN_OFFSET, &jpegOffset);
    CHECK_ERR(err, ("getJpeg: error %d, jpeg offset", err));


    s5p_rect zoomRect;
    ALOGV("DEBUG(%s)(%d): picture size(%d/%d)", __FUNCTION__, __LINE__, mPictureSize.width, mPictureSize.height);

    zoomRect.w = (uint32_t)((float)mPictureSize.width * 1000 / mZoomRatio);
    zoomRect.h = (uint32_t)((float)mPictureSize.height * 1000 / mZoomRatio);

    if (zoomRect.w % 2)
        zoomRect.w -= 1;

    if (zoomRect.h % 2)
        zoomRect.h -= 1;

    zoomRect.x = (mPictureSize.width - zoomRect.w) / 2;
    zoomRect.y = (mPictureSize.height - zoomRect.h) / 2;

    if (zoomRect.x % 2)
        zoomRect.x -= 1;

    if (zoomRect.y % 2)
        zoomRect.y -= 1;


    if (mPictureBufDummy[0].size.extS[0] <= 0)
        return UNKNOWN_ERROR;

    LOG_PERFORMANCE_START(3);

#if 0
    ALOGD("mPostviewFrameSize = %d", mPostviewFrameSize);
    save_dump_path((uint8_t*)mPostviewHeap->data,
            mPostviewFrameSize, "/data/dump_postview_yuv.yuv");
#endif

    thumbnail = false;

    LOG_PERFORMANCE_END(1, "encode thumbnail");
#ifdef SAVE_DUMP
#if 0
    save_dump_path((uint8_t*)thumbnailJpeg.virt.extP[0],
            thumbSize, "/data/dump_thumbnail_jpeg.jpg");
#endif
#endif

encodeJpeg:
    /* Making EXIF header */

    setExifChangedAttribute();

    uint32_t exifSize;
    uint8_t *jpeg;

    /* alloc exifOutBuf */
    exifOutBuf.size.extS[0] = EXIF_MAX_LEN;
    if (allocMem(mIonCameraClient, &exifOutBuf, 1 << 1) == false) {
        ALOGE("ERR(%s): exifTmpBuf allocMem() fail", __func__);
        goto destroyMem;
    } else {
        ALOGV("DEBUG(%s): exifTmpBuf allocMem adr(%p), size(%d), ion(%d)", __FUNCTION__,
                exifOutBuf.virt.extP[0], exifOutBuf.size.extS[0], mIonCameraClient);
        memset(exifOutBuf.virt.extP[0], 0, exifOutBuf.size.extS[0]);
    }

    if (!thumbnail)
        exifSize = exif.make((void *)exifOutBuf.virt.extP[0], &mExifInfo);
    else
        exifSize = exif.make((void *)exifOutBuf.virt.extP[0], &mExifInfo, exifOutBuf.size.extS[0], thumb, thumbSize);
    if (CC_UNLIKELY(!exifSize)) {
        ALOGE("ERR(%s): getJpeg: error, fail to make EXIF", __FUNCTION__);
        goto destroyMem;
    }

    /* Finally, Creating Jpeg image file including EXIF */
    mPictureFrameSize = jpegSize + exifSize;

    {
        ALOGD("jpegSize = %d, exifSize = %d", jpegSize, exifSize);
        /*note that picture frame size is should be calculated before call allocateSnapshotHeap */
        if (!allocateSnapshotHeap()) {
            ALOGE("getEncodedJpeg: error, allocateSnapshotHeap");
            goto destroyMem;
        }

        //    jpeg = (unsigned char *)jpegOutBuf.virt.extP[0];
        jpeg = (unsigned char *)mPictureBufDummy[0].virt.extP[0];
        memcpy((unsigned char *)mJpegHeap->data, jpeg, 2);
        memcpy((unsigned char *)mJpegHeap->data + 2, exifOutBuf.virt.extP[0], exifSize);
        memcpy((unsigned char *)mJpegHeap->data + 2 + exifSize, jpeg + 2, jpegSize - 2);
        ALOGD("DEBUG(%s): success making jpeg(%d) and exif(%d)", __FUNCTION__, jpegSize, exifSize);

#ifdef SAVE_DUMP
#if 0
        save_dump_path((uint8_t*)mJpegHeap->data,
                mPictureFrameSize, "/data/dump_full_img.jpg");
#endif
#endif
    }

    ret = NO_ERROR;

destroyMem:
    freeMemSinglePlane(&jpegOutBuf, 0);
    freeMemSinglePlane(&exifOutBuf, 0);

    return ret;
}

int SecCameraHardware::EncodeToJpeg(ExynosBuffer *yuvBuf, ExynosBuffer *jpegBuf,
    int width, int height, cam_pixel_format srcformat, int* jpegSize, int quality)
{
    ExynosJpegEncoder *jpegEnc = NULL;
    int ret = UNKNOWN_ERROR;

    /* 1. ExynosJpegEncoder create */
    jpegEnc = new ExynosJpegEncoder();
    if (jpegEnc == NULL) {
        ALOGE("ERR(%s) (%d):  jpegEnc is null", __FUNCTION__, __LINE__);
        return ret;
    }

    ret = jpegEnc->create();
    if (ret < 0) {
        ALOGE("ERR(%s):jpegEnc.create(%d) fail", __func__, ret);
        goto jpeg_encode_done;
    }

    /* 2. cache on */
    ret = jpegEnc->setCache(JPEG_CACHE_ON);
    if (ret < 0) {
        ALOGE("ERR(%s):jpegEnc.setCache(%d) fail", __func__, ret);
        goto jpeg_encode_done;
    }

    /* 3. set quality */
    ret = jpegEnc->setQuality(quality);
    if (ret < 0) {
        ALOGE("ERR(%s):jpegEnc.setQuality(%d) fail", __func__, ret);
        goto jpeg_encode_done;
    }

    ret = jpegEnc->setSize(width, height);
    if (ret < 0) {
        ALOGE("ERR(%s):jpegEnc.setSize() fail", __func__);
        goto jpeg_encode_done;
    }

    /* 4. set yuv format */
    ret = jpegEnc->setColorFormat(srcformat);
    if (ret < 0) {
        ALOGE("ERR(%s):jpegEnc.setColorFormat(%d) fail", __func__, ret);
        goto jpeg_encode_done;
    }

    /* 5. set jpeg format */
    ret = jpegEnc->setJpegFormat(V4L2_PIX_FMT_JPEG_422);
    if (ret < 0) {
        ALOGE("ERR(%s):jpegEnc.setJpegFormat(%d) fail", __func__, ret);
        goto jpeg_encode_done;
    }

    /* src and dst buffer alloc */
    /* 6. set yuv format(src) from FLITE */
    ret = jpegEnc->setInBuf(((char **)&(yuvBuf->virt.extP[0])), (int *)yuvBuf->size.extS);
    if (ret < 0) {
        ALOGE("ERR(%s):jpegEnc.setInBuf(%d) fail", __func__, ret);
        goto jpeg_encode_done;
    }

    /* 6. set jpeg format(dest) from FLITE */
    ret = jpegEnc->setOutBuf((char *)jpegBuf->virt.extP[0], jpegBuf->size.extS[0]);
    if (ret < 0) {
        ALOGE("ERR(%s):jpegEnc.setOutBuf(%d) fail", __func__, ret);
        goto jpeg_encode_done;
    }

    ret = jpegEnc->updateConfig();
    if (ret < 0) {
        ALOGE("ERR(%s):jpegEnc.updateConfig(%d) fail", __func__, ret);
        goto jpeg_encode_done;
    }

    /* start encoding */
    ret = jpegEnc->encode();
    if (ret < 0) {
        ALOGE("ERR(%s):jpegEnc.encode(%d) fail", __func__, ret);
        goto jpeg_encode_done;
    }

    /* start encoding */
    *jpegSize = jpegEnc->getJpegSize();
    if ((*jpegSize) <= 0) {
        ALOGE("ERR(%s):jpegEnc.getJpegSize(%d) is too small", __func__, *jpegSize);
        goto jpeg_encode_done;
    }

    ALOGD("DEBUG(%s): jpegEnc success!! size(%d)", __func__, *jpegSize);
    ret = NO_ERROR;

jpeg_encode_done:
    jpegEnc->destroy();
    delete jpegEnc;

    if (ret != NO_ERROR) {
        ALOGE("ERR(%s): (%d) [yuvBuf->fd.extFd[0] %d][yuvSize[0] %d]", __FUNCTION__, __LINE__,
            yuvBuf->fd.extFd[0], yuvBuf->size.extS[0]);
        ALOGE("ERR(%s): (%d) [jpegBuf->fd.extFd %d][jpegBuf->size.extS %d]", __FUNCTION__, __LINE__,
            jpegBuf->fd.extFd[0], jpegBuf->size.extS[0]);
        ALOGE("ERR(%s): (%d) [w %d][h %d][colorFormat %d]", __FUNCTION__, __LINE__,
            width, height, srcformat);
    }

    return ret;
}

bool SecCameraHardware::scaleDownYuv422(uint8_t *srcBuf, int srcW, int srcH,
                                        uint8_t *dstBuf, int dstW, int dstH)

{
    int step_x, step_y;
    int dst_pos;
    char *src_buf = (char *)srcBuf;
    char *dst_buf = (char *)dstBuf;

    if (dstW & 0x01 || dstH & 0x01) {
        ALOGE("ERROR(%s): (%d) width or height invalid(%d/%d)", __FUNCTION__, __LINE__,
                dstW & 0x01, dstH & 0x01);
        return false;
    }

    step_x = srcW / dstW;
    step_y = srcH / dstH;

    unsigned int srcWStride = srcW * 2;
    unsigned int stepXStride = step_x * 2;

    dst_pos = 0;
    for (int y = 0; y < dstH; y++) {
        int src_y_start_pos;
        src_y_start_pos = srcWStride * step_y * y;
        for (int x = 0; x < dstW; x += 2) {
            int src_pos;
            src_pos = src_y_start_pos + (stepXStride * x);
            dst_buf[dst_pos++] = src_buf[src_pos];
            dst_buf[dst_pos++] = src_buf[src_pos + 1];
            dst_buf[dst_pos++] = src_buf[src_pos + 2];
            dst_buf[dst_pos++] = src_buf[src_pos + 3];
        }
    }

    return true;
}

bool SecCameraHardware::conversion420to422(uint8_t *srcBuf, uint8_t *dstBuf, int width, int height)
{
    int i, j, k;
    int vPos1, vPos2, uPos1, uPos2;

    /* copy y table */
    for (i = 0; i < width * height; i++) {
        j = i * 2;
        dstBuf[j] = srcBuf[i];
    }

    for (i = 0; i < width * height / 2; i += 2) {
        j = width * height;
        k = width * 2;
        uPos1 = (i / width) * k * 2 + (i % width) * 2 + 1;
        uPos2 = uPos1 + k;
        vPos1 = uPos1 + 2;
        vPos2 = uPos2 + 2;

        if (uPos1 >= 0) {
            dstBuf[vPos1] = srcBuf[j + i]; /* V table */
            dstBuf[vPos2] = srcBuf[j + i]; /* V table */
            dstBuf[uPos1] = srcBuf[j + i + 1]; /* U table */
            dstBuf[uPos2] = srcBuf[j + i + 1]; /* U table */
        }
    }

    return true;
}

bool SecCameraHardware::conversion420Tto422(uint8_t *srcBuf, uint8_t *dstBuf, int width, int height)
{
    int i, j, k;
    int vPos1, vPos2, uPos1, uPos2;

    /* copy y table */
    for (i = 0; i < width * height; i++) {
        j = i * 2;
        dstBuf[j] = srcBuf[i];
    }
    for (i = 0; i < width * height / 2; i+=2) {
        j = width * height;
        k = width * 2;
        uPos1 = (i / width) * k * 2 + (i % width) * 2 + 1;
        uPos2 = uPos1 + k;
        vPos1 = uPos1 + 2;
        vPos2 = uPos2 + 2;

        if (uPos1 >= 0) {
            dstBuf[uPos1] = srcBuf[j + i]; /* V table */
            dstBuf[uPos2] = srcBuf[j + i]; /* V table */
            dstBuf[vPos1] = srcBuf[j + i + 1]; /* U table */
            dstBuf[vPos2] = srcBuf[j + i + 1]; /* U table */
        }
    }

    return true;
}

int SecCameraHardware::mSaveDump(const char *filepath, ExynosBuffer *dstBuf, int index)
{
    FILE *yuv_fp = NULL;
    char filename[100];
    char *buffer = NULL;
    CLEAR(filename);

    /* file create/open, note to "wb" */
    snprintf(filename, 100, filepath, index);
    yuv_fp = fopen(filename, "wb");
    if (yuv_fp == NULL) {
        CLOGE("Save jpeg file open error[%s]", filename);
        return -1;
    }

    int yuv_size = dstBuf->size.extS[0] + dstBuf->size.extS[1] + dstBuf->size.extS[2];

    buffer = (char *) malloc(yuv_size);
    if (buffer == NULL) {
        CLOGE("Save YUV] buffer alloc failed");
        if (yuv_fp)
            fclose(yuv_fp);

        return -1;
    }

    memcpy(buffer, dstBuf->virt.extP[0], dstBuf->size.extS[0]);
    if (dstBuf->size.extS[1] > 0) {
        memcpy(buffer + dstBuf->size.extS[0],
                dstBuf->virt.extP[1], dstBuf->size.extS[1]);
        if (dstBuf->size.extS[2] > 0) {
            memcpy(buffer + dstBuf->size.extS[0] + dstBuf->size.extS[1],
                    dstBuf->virt.extP[2], dstBuf->size.extS[2]);
        }
    }

    fflush(stdout);

    fwrite(buffer, 1, yuv_size, yuv_fp);

    fflush(yuv_fp);

    if (yuv_fp)
            fclose(yuv_fp);
    if (buffer)
            free(buffer);

    return 0;
}


int SecCameraHardware::createInstance(int cameraId)
{
    if (!init()) {
        ALOGE("createInstance: error, camera cannot be initialiezed");
        mInitialized = false;
        return NO_INIT;
    }

    initDefaultParameters();
    CLOGD("DEBUG[%s(%d)]-%s camera created", __FUNCTION__, __LINE__,
        cameraId == CAMERA_ID_BACK ? "Back" : "Front");

    mInitialized = true;
    return NO_ERROR;
}

int SecCameraHardware::getMaxZoomLevel(void)
{
    int zoomLevel = 0;

    zoomLevel = MAX_BASIC_ZOOM_LEVEL;

    return zoomLevel;
}

int SecCameraHardware::getMaxZoomRatio(void)
{
    int maxZoomRatio = 0;

    if (mCameraId == CAMERA_ID_BACK) {
        maxZoomRatio = MAX_ZOOM_RATIO;
    } else {
        maxZoomRatio = MAX_ZOOM_RATIO_FRONT;
    }

    return maxZoomRatio;
}

float SecCameraHardware::getZoomRatio(int zoomLevel)
{
    float zoomRatio = 1.00f;
    if (IsZoomSupported() == true)
        zoomRatio = (float)zoomRatioList[zoomLevel];
    else
        zoomRatio = 1000.00f;

    return zoomRatio;
}

#ifdef SAMSUNG_JPEG_QUALITY_ADJUST_TARGET
void SecCameraHardware::adjustJpegQuality(void)
{
    mJpegQuality = JPEG_QUALITY_ADJUST_TARGET;
    ALOGD("DEBUG(%s):adjusted JpegQuality %d", __func__, mJpegQuality);
}
#endif

int getSensorId(int camId)
{
    int sensorId = -1;

#ifdef SENSOR_NAME_GET_FROM_FILE
    int &curSensorId = (camId == CAMERA_ID_BACK) ? g_rearSensorId : g_frontSensorId;

    if (curSensorId < 0) {
        curSensorId = getSensorIdFromFile(camId);
        if (curSensorId < 0) {
            ALOGE("ERR(%s): invalid sensor ID %d", __FUNCTION__, sensorId);
        }
    }

    sensorId = curSensorId;
#else
    if (camId == CAMERA_ID_BACK) {
        sensorId = BACK_CAMERA_SENSOR_NAME;
    } else if (camId == CAMERA_ID_FRONT) {
        sensorId = FRONT_CAMERA_SENSOR_NAME;
    } else {
        ALOGE("ERR(%s):Unknown camera ID(%d)", __FUNCTION__, camId);
    }
#endif

    return sensorId;
}

#ifdef SENSOR_NAME_GET_FROM_FILE
int getSensorIdFromFile(int camId)
{
    FILE *fp = NULL;
    int numread = -1;
    char sensor_name[50];
    int sensorName = -1;
    bool ret = true;

    if (camId == CAMERA_ID_BACK) {
        fp = fopen(SENSOR_NAME_PATH_BACK, "r");
        if (fp == NULL) {
            ALOGE("ERR(%s[%d]):failed to open sysfs entry", __FUNCTION__, __LINE__);
            goto err;
        }
    } else {
        fp = fopen(SENSOR_NAME_PATH_FRONT, "r");
        if (fp == NULL) {
            ALOGE("ERR(%s[%d]):failed to open sysfs entry", __FUNCTION__, __LINE__);
            goto err;
        }
    }

    if (fgets(sensor_name, sizeof(sensor_name), fp) == NULL) {
        ALOGE("ERR(%s[%d]):failed to read sysfs entry", __FUNCTION__, __LINE__);
	    goto err;
    }

    numread = strlen(sensor_name);
    ALOGD("DEBUG(%s[%d]):Sensor name is %s(%d)", __FUNCTION__, __LINE__, sensor_name, numread);

    /* TODO: strncmp for check sensor name, str is vendor specific sensor name
     * ex)
     *    if (strncmp((const char*)sensor_name, "str", numread - 1) == 0) {
     *        sensorName = SENSOR_NAME_IMX135;
     *    }
     */
    sensorName = atoi(sensor_name);

err:
    if (fp != NULL)
        fclose(fp);

    return sensorName;
}
#endif

#ifdef SENSOR_FW_GET_FROM_FILE
const char *getSensorFWFromFile(char *sensor_fw, size_t size, int camId)
{
    FILE *fp = NULL;
    int numread = -1;

    if (camId == CAMERA_ID_BACK) {
        fp = fopen(SENSOR_FW_PATH_BACK, "r");
        if (fp == NULL) {
            ALOGE("ERR(%s[%d]):failed to open SENSOR_FW_PATH_BACK", __FUNCTION__, __LINE__);
            goto err;
        }
    } else {
        fp = fopen(SENSOR_FW_PATH_FRONT, "r");
        if (fp == NULL) {
            ALOGE("ERR(%s[%d]):failed to open SENSOR_FW_PATH_FRONT", __FUNCTION__, __LINE__);
            goto err;
        }
    }
    if (fgets(sensor_fw, size, fp) == NULL) {
        ALOGE("ERR(%s[%d]):camId(%d)failed to read sysfs entry", __FUNCTION__, __LINE__, camId);
        goto err;
    }

    numread = strlen(sensor_fw);
    ALOGD("DEBUG(%s[%d]):Sensor fw is %s(%d)", __FUNCTION__, __LINE__, sensor_fw, numread);

err:
    if (fp != NULL)
        fclose(fp);

    return (const char*)sensor_fw;
}
#endif

}; /* namespace android */

#endif /* ANDROID_HARDWARE_SECCAMERAHARDWARE_CPP */
