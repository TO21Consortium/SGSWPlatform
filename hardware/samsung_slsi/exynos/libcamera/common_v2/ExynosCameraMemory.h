/*
 * Copyright 2013, Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed toggle an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*!
 * \file      ExynosCameraMemory.h
 * \brief     header file for ExynosCameraMemory
 * \author    Sunmi Lee(carrotsm.lee@samsung.com)
 * \date      2013/07/22
 *
 */

#ifndef EXYNOS_CAMERA_MEMORY_H__
#define EXYNOS_CAMERA_MEMORY_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <binder/MemoryHeapBase.h>
#include <hardware/camera.h>
#include <hardware/camera3.h>
#include <videodev2.h>
#include <videodev2_exynos_camera.h>
#include <ion/ion.h>
#include <ui/GraphicBuffer.h>

#include "gralloc_priv.h"
#include "exynos_format.h"

#include "ExynosCameraConfig.h"

#include "fimc-is-metadata.h"
#include "ExynosCameraAutoTimer.h"

namespace android {

/* #define EXYNOS_CAMERA_MEMORY_TRACE */

/* EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE define is log of gralloc function(xxxxx) duration.
 * If you want check function duration, enable this define.
 */
/* #define EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE */
#define GRALLOC_WARNING_DURATION_MSEC   (180)     /* 180ms */

class ExynosCameraGraphicBufferAllocator {
public:
    ExynosCameraGraphicBufferAllocator();
    virtual ~ExynosCameraGraphicBufferAllocator();

    status_t init(void);

    status_t setSize(int width, int height, int stride);
    status_t getSize(int *width, int *height, int *stride);

    status_t setHalPixelFormat(int halPixelFormat);
    int      getHalPixelFormat(void);

    status_t setGrallocUsage(int grallocUsage);
    int      getGrallocUsage(void);

    sp<GraphicBuffer> alloc(int index, int planeCount, int fdArr[], char *bufAddr[], unsigned int bufSize[]);
    status_t free(int index);

private:
    sp<GraphicBuffer> m_alloc(
            int index,
            int planeCount,
            int fdArr[],
            char *bufAddr[],
            unsigned int bufSize[],
            int width,
            int height,
            int halPixelFormat,
            int grallocUsage,
            int stride);

private:
    int                m_width;
    int                m_height;
    int                m_stride;
    int                m_halPixelFormat;
    int                m_grallocUsage;

    private_handle_t  *m_privateHandle[VIDEO_MAX_FRAME];
    sp<GraphicBuffer>  m_graphicBuffer[VIDEO_MAX_FRAME];
    bool               m_flagGraphicBufferAlloc[VIDEO_MAX_FRAME];
};

class ExynosCameraIonAllocator {
public:
    ExynosCameraIonAllocator();
    virtual ~ExynosCameraIonAllocator();

    status_t init(bool isCached);
    status_t alloc(
            int size,
            int *fd,
            char **addr,
            bool mapNeeded);
    status_t alloc(
            int size,
            int *fd,
            char **addr,
            int  mask,
            int  flags,
            bool mapNeeded);
    status_t free(
            int size,
            int *fd,
            char **addr,
            bool mapNeeded);
    status_t map(int size, int fd, char **addr);
    void     setIonHeapMask(int mask);
    void     setIonFlags(int flags);

private:
    int             m_ionClient;
    size_t          m_ionAlign;
    unsigned int    m_ionHeapMask;
    unsigned int    m_ionFlags;

    sp<GraphicBuffer>                m_graphicBuffer[VIDEO_MAX_FRAME];
    bool                             m_flagGraphicBufferAlloc[VIDEO_MAX_FRAME];
};

class ExynosCameraMHBAllocator {
/* do not use! deprecated class */
public:
    ExynosCameraMHBAllocator();
    virtual ~ExynosCameraMHBAllocator();

    status_t init(camera_request_memory allocator);
    status_t alloc(
            int size,
            int *fd,
            char **addr,
            int numBufs,
            camera_memory_t **heap);
    status_t free(
            int size,
            int *fd,
            char **addr,
            camera_memory_t **heap);

private:
    camera_request_memory   m_allocator;
};

class ExynosCameraGrallocAllocator {
public:
    ExynosCameraGrallocAllocator();
    virtual ~ExynosCameraGrallocAllocator();

    status_t init(
                preview_stream_ops *allocator,
                int bufCount,
                int minUndequeueBufferMargin = -1);
    status_t init(
                preview_stream_ops *allocator,
                int bufCount,
                int minUndequeueBufferMargin,
                int grallocUsage);
    status_t alloc(
                buffer_handle_t **bufHandle,
                int fd[],
                char *addr[],
                int  *bufStride,
                bool *isLocked);
    status_t free(buffer_handle_t *bufHandle, bool isLocked);

    status_t setBufferCount(int bufCount);
    status_t setBuffersGeometry(
                int width,
                int height,
                int halPixelFormat);

     /*
      * setGrallocUsage is happen on init() api.
      * default grallocUsage is GRALLOC_SET_USAGE_FOR_CAMERA
      */
    int      getGrallocUsage(void);

    status_t getAllocator(preview_stream_ops **allocator);
    int      getMinUndequeueBuffer();
    status_t dequeueBuffer(
                buffer_handle_t **bufHandle,
                int fd[],
                char *addr[],
                bool *isLocked);
    status_t enqueueBuffer(buffer_handle_t *bufHandle);
    status_t cancelBuffer(buffer_handle_t *bufHandle);

private:
    preview_stream_ops              *m_allocator;
    static gralloc_module_t const   *m_grallocHal;
    int32_t                          m_minUndequeueBufferMargin;

    int                              m_grallocUsage;
};

class ExynosCameraStreamAllocator {
public:
    ExynosCameraStreamAllocator();
    virtual ~ExynosCameraStreamAllocator();

    status_t init(camera3_stream_t *allocator);
    int      lock(
                buffer_handle_t **bufHandle,
                int fd[],
                char *addr[],
                bool *isLocked,
                int planeCount);

    int      unlock(buffer_handle_t *bufHandle);

private:
    camera3_stream_t                *m_allocator;
    static gralloc_module_t const   *m_grallocHal;
};
}
#endif
