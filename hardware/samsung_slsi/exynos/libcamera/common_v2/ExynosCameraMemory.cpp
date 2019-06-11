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

#define LOG_TAG "ExynosCameraMemoryAllocator"
#include "ExynosCameraMemory.h"

namespace android {


gralloc_module_t const *ExynosCameraGrallocAllocator::m_grallocHal;
gralloc_module_t const *ExynosCameraStreamAllocator::m_grallocHal;

ExynosCameraGraphicBufferAllocator::ExynosCameraGraphicBufferAllocator()
{
}

ExynosCameraGraphicBufferAllocator::~ExynosCameraGraphicBufferAllocator()
{
}

status_t ExynosCameraGraphicBufferAllocator::init(void)
{
    m_width = 0;
    m_height = 0;
    m_stride = 0;
    m_halPixelFormat = 0;
    m_grallocUsage = GRALLOC_SET_USAGE_FOR_CAMERA;

    for (int i = 0; i < VIDEO_MAX_FRAME; i++) {
        m_flagGraphicBufferAlloc[i] = false;
    }

    return NO_ERROR;
}

status_t ExynosCameraGraphicBufferAllocator::setSize(int width, int height, int stride)
{
    m_width  = width;
    m_height = height;
    m_stride = stride;

    return NO_ERROR;
}

status_t ExynosCameraGraphicBufferAllocator::getSize(int *width, int *height, int *stride)
{
    *width = m_width;
    *height = m_height;
    *stride = m_stride;

    return NO_ERROR;
}

status_t ExynosCameraGraphicBufferAllocator::setHalPixelFormat(int halPixelFormat)
{
    m_halPixelFormat = halPixelFormat;

    return NO_ERROR;
}

int ExynosCameraGraphicBufferAllocator::getHalPixelFormat(void)
{
    return m_halPixelFormat;
}

status_t ExynosCameraGraphicBufferAllocator::setGrallocUsage(int grallocUsage)
{
    m_grallocUsage = grallocUsage;

    return NO_ERROR;
}

int ExynosCameraGraphicBufferAllocator::getGrallocUsage(void)
{
    return m_grallocUsage;
}

sp<GraphicBuffer> ExynosCameraGraphicBufferAllocator::alloc(int index, int planeCount, int fdArr[], char *bufAddr[], unsigned int bufSize[])
{
    if ((index < 0) || (index >= VIDEO_MAX_FRAME)) {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):Buffer index error (%d/%d), assert!!!!",
            __FUNCTION__, __LINE__, index, VIDEO_MAX_FRAME);
    }

    sp<GraphicBuffer> graphicBuffer;

    if (m_flagGraphicBufferAlloc[index] == false) {
        graphicBuffer = m_alloc(index, planeCount, fdArr, bufAddr, bufSize, m_width, m_height, m_halPixelFormat, m_grallocUsage, m_stride);
        if (graphicBuffer == 0) {
            ALOGE("ERR(%s[%d]):m_alloc() fail", __FUNCTION__, __LINE__);
            goto done;
        }
    } else {
        graphicBuffer = m_graphicBuffer[index];
        if (graphicBuffer == 0) {
            ALOGE("ERR(%s[%d]):m_graphicBuffer[%d] == 0. so, fail", __FUNCTION__, __LINE__, index);
            goto done;
        }
    }

done:
    return graphicBuffer;
}

status_t ExynosCameraGraphicBufferAllocator::free(int index)
{
    int ret = 0;

    if (m_flagGraphicBufferAlloc[index] == false)
        return NO_ERROR;

    m_flagGraphicBufferAlloc[index] = false;

    delete m_privateHandle[index];
    m_privateHandle[index] = NULL;

    m_graphicBuffer[index] = 0;

    return NO_ERROR;
}

sp<GraphicBuffer> ExynosCameraGraphicBufferAllocator::m_alloc(int index,
                                                              int planeCount,
                                                              int fdArr[],
                                                              char *bufAddr[],
                                                              unsigned int bufSize[],
                                                              int width,
                                                              int height,
                                                              int halPixelFormat,
                                                              int grallocUsage,
                                                              int stride)
{
    if (m_flagGraphicBufferAlloc[index] == true) {
        ALOGE("ERR(%s[%d]):%d is already allocated. so, fail!!",
            __FUNCTION__, __LINE__, index);
        goto done;
    }

    if (planeCount <= 0) {
        ALOGE("ERR(%s[%d]):invalid value : planeCount(%d). so, fail!!",
            __FUNCTION__, __LINE__, planeCount);
        goto done;
    }

    if (width == 0 || height == 0 || halPixelFormat == 0 || grallocUsage <= 0 || stride <= 0) {
        ALOGE("ERR(%s[%d]):invalid value : width(%d), height(%d), halPixelFormat(%d), grallocUsage(%d), stride(%d). so, fail!!",
            __FUNCTION__, __LINE__, width, height, halPixelFormat, grallocUsage, stride);
        goto done;
    }

    if (planeCount == 1) {
        m_privateHandle[index] = new private_handle_t(fdArr[0], bufSize[0], grallocUsage, width, height,
            halPixelFormat, halPixelFormat, halPixelFormat, width, height, 0);

        m_privateHandle[index]->base = (uint64_t)bufAddr[0];
        m_privateHandle[index]->offset = 0;
    } else {
        android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):planeCount(%d) is not yet support, assert!!!!",
            __FUNCTION__, __LINE__, planeCount);
    }

    ALOGV("DEBUG(%s[%d]):new GraphicBuffer(bufAddr(%p), width(%d), height(%d), halPixelFormat(%d), grallocUsage(%d), stride(%d), m_privateHandle[%d](%p), false)",
            __FUNCTION__, __LINE__, bufAddr[0], width, height, halPixelFormat, grallocUsage, stride, index, m_privateHandle[index]);

    m_graphicBuffer[index] = new GraphicBuffer(width, height, halPixelFormat, grallocUsage, stride, (native_handle_t*)m_privateHandle[index], false);

    m_flagGraphicBufferAlloc[index] = true;

done:
    return m_graphicBuffer[index];
}

ExynosCameraIonAllocator::ExynosCameraIonAllocator()
{
    m_ionClient   = 0;
    m_ionAlign    = 0;
    m_ionHeapMask = 0;
    m_ionFlags    = 0;
}

ExynosCameraIonAllocator::~ExynosCameraIonAllocator()
{
    ion_close(m_ionClient);
}

status_t ExynosCameraIonAllocator::init(bool isCached)
{
    status_t ret = NO_ERROR;

    if (m_ionClient == 0) {
        m_ionClient = ion_open();

        if (m_ionClient < 0) {
            ALOGE("ERR(%s):ion_open failed", __FUNCTION__);
            ret = BAD_VALUE;
            goto func_exit;
        }
    }

    m_ionAlign    = 0;
    m_ionHeapMask = ION_HEAP_SYSTEM_MASK;
    m_ionFlags    = (isCached == true ?
        (ION_FLAG_CACHED | ION_FLAG_CACHED_NEEDS_SYNC ) : 0);

func_exit:

    return ret;
}

status_t ExynosCameraIonAllocator::alloc(
        int size,
        int *fd,
        char **addr,
        bool mapNeeded)
{
    status_t ret = NO_ERROR;
    int ionFd = 0;
    char *ionAddr = NULL;

    if (m_ionClient == 0) {
        ALOGE("ERR(%s):allocator is not yet created", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (size == 0) {
        ALOGE("ERR(%s):size equals zero", __FUNCTION__);
        ret = BAD_VALUE;
        goto func_exit;
    }

    ret = ion_alloc_fd(m_ionClient, size, m_ionAlign, m_ionHeapMask, m_ionFlags, &ionFd);

    if (ret < 0) {
        ALOGE("ERR(%s):ion_alloc_fd(fd=%d) failed(%s)", __FUNCTION__, ionFd, strerror(errno));
        ionFd = -1;
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (mapNeeded == true) {
        if (map(size, ionFd, &ionAddr) != NO_ERROR) {
            ALOGE("ERR(%s):map failed", __FUNCTION__);
        }
    }

func_exit:

    *fd   = ionFd;
    *addr = ionAddr;

    return ret;
}

status_t ExynosCameraIonAllocator::alloc(
        int size,
        int *fd,
        char **addr,
        int  mask,
        int  flags,
        bool mapNeeded)
{
    status_t ret = NO_ERROR;
    int ionFd = 0;
    char *ionAddr = NULL;

    if (m_ionClient == 0) {
        ALOGE("ERR(%s):allocator is not yet created", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (size == 0) {
        ALOGE("ERR(%s):size equals zero", __FUNCTION__);
        ret = BAD_VALUE;
        goto func_exit;
    }

    ret  = ion_alloc_fd(m_ionClient, size, m_ionAlign, mask, flags, &ionFd);

    if (ret < 0) {
        ALOGE("ERR(%s):ion_alloc_fd(fd=%d) failed(%s)", __FUNCTION__, ionFd, strerror(errno));
        ionFd = -1;
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (mapNeeded == true) {
        if (map(size, ionFd, &ionAddr) != NO_ERROR) {
            ALOGE("ERR(%s):map failed", __FUNCTION__);
        }
    }

func_exit:

    *fd   = ionFd;
    *addr = ionAddr;

    return ret;
}

status_t ExynosCameraIonAllocator::free(
        __unused int size,
        int *fd,
        char **addr,
        bool mapNeeded)
{
    status_t ret = NO_ERROR;
    int ionFd = *fd;
    char *ionAddr = *addr;

    if (ionFd < 0) {
        ALOGE("ERR(%s):ion_fd is lower than zero", __FUNCTION__);
        ret = BAD_VALUE;
        goto func_exit;
    }

    if (mapNeeded == true) {
        if (ionAddr == NULL) {
            ALOGE("ERR(%s):ion_addr equals NULL", __FUNCTION__);
            ret = BAD_VALUE;
            goto func_exit;
        }

        if (munmap(ionAddr, size) < 0) {
            ALOGE("ERR(%s):munmap failed", __FUNCTION__);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

    ion_close(ionFd);

    ionFd   = -1;
    ionAddr = NULL;

func_exit:

    *fd   = ionFd;
    *addr = ionAddr;

    return ret;
}

status_t ExynosCameraIonAllocator::map(int size, int fd, char **addr)
{
    status_t ret = NO_ERROR;
    char *ionAddr = NULL;

    if (size == 0) {
        ALOGE("ERR(%s):size equals zero", __FUNCTION__);
        ret = BAD_VALUE;
        goto func_exit;
    }

    if (fd <= 0) {
        ALOGE("ERR(%s):fd=%d failed", __FUNCTION__, fd);
        ret = BAD_VALUE;
        goto func_exit;
    }

    ionAddr = (char *)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    if (ionAddr == (char *)MAP_FAILED || ionAddr == NULL) {
        ALOGE("ERR(%s):ion_map(size=%d) failed, (fd=%d), (%s)", __FUNCTION__, size, fd, strerror(errno));
        close(fd);
        ionAddr = NULL;
        ret = INVALID_OPERATION;
        goto func_exit;
    }

func_exit:

    *addr = ionAddr;

    return ret;
}

void ExynosCameraIonAllocator::setIonHeapMask(int mask)
{
    m_ionHeapMask |= mask;
}

void ExynosCameraIonAllocator::setIonFlags(int flags)
{
    m_ionFlags |= flags;
}

ExynosCameraMHBAllocator::ExynosCameraMHBAllocator()
{
    m_allocator = NULL;
}

ExynosCameraMHBAllocator::~ExynosCameraMHBAllocator()
{
}

status_t ExynosCameraMHBAllocator::init(camera_request_memory allocator)
{
    m_allocator = allocator;

    return NO_ERROR;
}

status_t ExynosCameraMHBAllocator::alloc(
        int size,
        int *fd,
        char **addr,
        int numBufs,
        camera_memory_t **heap)
{
    status_t ret = NO_ERROR;
    camera_memory_t *heap_ptr = NULL;
    int  heapFd    = 0;
    char *heapAddr = NULL;

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    heap_ptr = m_allocator(-1, size, numBufs, &heapFd);

    if (heap_ptr == NULL || heapFd < 0) {
        ALOGE("ERR(%s):heap_alloc(size=%d) failed", __FUNCTION__, size);
        heap_ptr = NULL;
        heapFd = -1;
        ret = BAD_VALUE;
        goto func_exit;
    }

    heapAddr = (char *)heap_ptr->data;

func_exit:

    *fd   = heapFd;
    *addr = heapAddr;
    *heap = heap_ptr;

#ifdef EXYNOS_CAMERA_MEMORY_TRACE
    ALOGI("INFO(%s[%d]):[heap.fd=%d] .addr=%p .heap=%p]",
        __FUNCTION__, __LINE__, heapFd, heapAddr, heap_ptr);
#endif

    return ret;
}

status_t ExynosCameraMHBAllocator::free(
        __unused int size,
        int *fd,
        char **addr,
        camera_memory_t **heap)
{
    status_t ret = NO_ERROR;
    camera_memory_t *heap_ptr = *heap;
    int heapFd     = *fd;
    char *heapAddr = *addr;

    if (heap_ptr == NULL) {
        ALOGE("ERR(%s):heap_ptr equals NULL", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    heap_ptr->release(heap_ptr);
    heapAddr = NULL;
    heapFd   = -1;
    heap_ptr = 0;

func_exit:

    *fd   = heapFd;
    *addr = heapAddr;
    *heap = heap_ptr;

    return ret;
}

ExynosCameraGrallocAllocator::ExynosCameraGrallocAllocator()
{
    m_allocator = NULL;
    m_minUndequeueBufferMargin = 0;
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&m_grallocHal))
        ALOGE("ERR(%s):Loading gralloc HAL failed", __FUNCTION__);

    m_grallocUsage = GRALLOC_SET_USAGE_FOR_CAMERA;
}

ExynosCameraGrallocAllocator::~ExynosCameraGrallocAllocator()
{
    m_minUndequeueBufferMargin = 0;
}

status_t ExynosCameraGrallocAllocator::init(
        preview_stream_ops *allocator,
        int bufCount,
        int minUndequeueBufferCount)
{
    status_t ret = NO_ERROR;

    ret = init(allocator, bufCount, minUndequeueBufferCount, GRALLOC_SET_USAGE_FOR_CAMERA);
    return ret;
}

status_t ExynosCameraGrallocAllocator::init(
        preview_stream_ops *allocator,
        int bufCount,
        int minUndequeueBufferCount,
        int grallocUsage)
{
    status_t ret = NO_ERROR;

    m_allocator = allocator;
    if( minUndequeueBufferCount < 0 ) {
        m_minUndequeueBufferMargin = 0;
    } else {
        m_minUndequeueBufferMargin = minUndequeueBufferCount;
    }

    if (setBufferCount(bufCount) != 0) {
        ALOGE("ERR(%s):setBufferCount failed", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_allocator->set_usage(m_allocator, grallocUsage) != 0) {
        ALOGE("ERR(%s):set_usage failed", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    m_grallocUsage = grallocUsage;

    if (m_grallocHal == NULL) {
        if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&m_grallocHal))
            ALOGE("ERR(%s):Loading gralloc HAL failed", __FUNCTION__);
    }

func_exit:

    return ret;
}

status_t ExynosCameraGrallocAllocator::alloc(
        buffer_handle_t **bufHandle,
        int fd[],
        char *addr[],
        int  *bufStride,
        bool *isLocked)
{
    status_t ret = NO_ERROR;
    int   width  = 0;
    int   height = 0;
    void  *grallocAddr[3] = {NULL};
    int   grallocFd[3] = {0};
    const private_handle_t *priv_handle = NULL;
    int   retryCount = 5;
    ExynosCameraDurationTimer   dequeuebufferTimer;
    ExynosCameraDurationTimer   lockbufferTimer;

    for (int retryCount = 5; retryCount > 0; retryCount--) {
#ifdef EXYNOS_CAMERA_MEMORY_TRACE
        ALOGI("INFO(%s[%d]):dequeue_buffer retryCount=%d",
            __FUNCTION__, __LINE__, retryCount);
#endif
        if (m_allocator == NULL) {
            ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
            ret = INVALID_OPERATION;
            goto func_exit;
        }

        dequeuebufferTimer.start();
        ret = m_allocator->dequeue_buffer(m_allocator, bufHandle, bufStride);
        dequeuebufferTimer.stop();

#if defined (EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE)
        ALOGD("DEBUG(%s[%d]):Check dequeue buffer performance, duration(%lld usec)",
                __FUNCTION__, __LINE__, dequeuebufferTimer.durationUsecs());
#else
        if (dequeuebufferTimer.durationMsecs() > GRALLOC_WARNING_DURATION_MSEC)
            ALOGW("WRN(%s[%d]):dequeue_buffer() duration(%lld msec)",
                    __FUNCTION__, __LINE__, dequeuebufferTimer.durationMsecs());
#endif

        if (ret == NO_INIT) {
            ALOGW("WARN(%s):BufferQueue is abandoned", __FUNCTION__);
            return ret;
        } else if (ret != 0) {
            ALOGE("ERR(%s):dequeue_buffer failed", __FUNCTION__);
            continue;
        }

        if (bufHandle == NULL) {
            ALOGE("ERR(%s):bufHandle == NULL failed, retry(%d)", __FUNCTION__, retryCount);
            continue;
        }

        lockbufferTimer.start();
        ret = m_allocator->lock_buffer(m_allocator, *bufHandle);
        lockbufferTimer.stop();
        if (ret != 0)
            ALOGE("ERR(%s):lock_buffer failed, but go on to the next step ...", __FUNCTION__);

#if defined (EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE)
        ALOGD("DEBUG(%s[%d]):Check lock buffer performance, duration(%lld usec)",
                __FUNCTION__, __LINE__, lockbufferTimer.durationUsecs());
#else
        if (lockbufferTimer.durationMsecs() > GRALLOC_WARNING_DURATION_MSEC)
            ALOGW("WRN(%s[%d]):lock_buffer() duration(%lld msec)",
                    __FUNCTION__, __LINE__, lockbufferTimer.durationMsecs());
#endif

        if (*isLocked == false) {
            lockbufferTimer.start();
            ret = m_grallocHal->lock(m_grallocHal, **bufHandle, GRALLOC_LOCK_FOR_CAMERA,
                                    0, 0,/* left, top */ width, height, grallocAddr);
            lockbufferTimer.stop();

#if defined (EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE)
            ALOGD("DEBUG(%s[%d]):Check grallocHAL lock performance, duration(%lld usec)",
                    __FUNCTION__, __LINE__, lockbufferTimer.durationUsecs());
#else
            if (lockbufferTimer.durationMsecs() > GRALLOC_WARNING_DURATION_MSEC)
                ALOGW("WRN(%s[%d]):grallocHAL->lock() duration(%lld msec)",
                        __FUNCTION__, __LINE__, lockbufferTimer.durationMsecs());
#endif

            if (ret != 0) {
                ALOGE("ERR(%s):grallocHal->lock failed.. retry", __FUNCTION__);

                if (m_allocator->cancel_buffer(m_allocator, *bufHandle) != 0)
                    ALOGE("ERR(%s):cancel_buffer failed", __FUNCTION__);
                ret = INVALID_OPERATION;
                goto func_exit;
            }
            break;
        }
    }

    if (bufHandle == NULL) {
        ALOGE("ERR(%s):bufHandle == NULL failed", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (*bufHandle == NULL) {
        ALOGE("@@@@ERR(%s):*bufHandle == NULL failed", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    priv_handle = private_handle_t::dynamicCast(**bufHandle);

    grallocFd[0] = priv_handle->fd;
    grallocFd[1] = priv_handle->fd1;
    *isLocked    = true;

func_exit:

    fd[0] = grallocFd[0];
    fd[1] = grallocFd[1];
    addr[0] = (char *)grallocAddr[0];
    addr[1] = (char *)grallocAddr[1];

    return ret;
}

status_t ExynosCameraGrallocAllocator::free(buffer_handle_t *bufHandle, bool isLocked)
{
    status_t ret = NO_ERROR;

    if (bufHandle == NULL) {
        ALOGE("ERR(%s):bufHandle equals NULL", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (isLocked == true) {
        if (m_grallocHal->unlock(m_grallocHal, *bufHandle) != 0) {
            ALOGE("ERR(%s):grallocHal->unlock failed", __FUNCTION__);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_allocator->cancel_buffer(m_allocator, bufHandle) != 0) {
        ALOGE("ERR(%s):cancel_buffer failed", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

func_exit:

    return ret;
}

status_t ExynosCameraGrallocAllocator::setBufferCount(int bufCount)
{
    status_t ret = NO_ERROR;

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        ret = INVALID_OPERATION;
    }

    if (m_allocator->set_buffer_count(m_allocator, bufCount) != 0) {
        ALOGE("ERR(%s):set_buffer_count failed [bufCount=%d]", __FUNCTION__, bufCount);
        ret = INVALID_OPERATION;
    }

    return ret;
}
status_t ExynosCameraGrallocAllocator::setBuffersGeometry(
        int width,
        int height,
        int halPixelFormat)
{
    status_t ret = NO_ERROR;

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        ret = INVALID_OPERATION;
    }

    if (m_allocator->set_buffers_geometry(
                    m_allocator,
                    width, height,
                    halPixelFormat) != 0) {
        ALOGE("ERR(%s):set_buffers_geometry failed", __FUNCTION__);
        ret = INVALID_OPERATION;
    }

    return ret;
}

int ExynosCameraGrallocAllocator::getGrallocUsage(void)
{
    return m_grallocUsage;
}

status_t ExynosCameraGrallocAllocator::getAllocator(preview_stream_ops **allocator)
{
    *allocator = m_allocator;

    return NO_ERROR;
}

int ExynosCameraGrallocAllocator::getMinUndequeueBuffer()
{
    int minUndeqBufCount = 0;

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        return INVALID_OPERATION;
    }

    if (m_allocator->get_min_undequeued_buffer_count(m_allocator, &minUndeqBufCount) != 0) {
        ALOGE("ERR(%s):enqueue_buffer failed", __FUNCTION__);
        return INVALID_OPERATION;
    }

    return minUndeqBufCount < 2 ? (minUndeqBufCount + m_minUndequeueBufferMargin) : minUndeqBufCount;
}

status_t ExynosCameraGrallocAllocator::dequeueBuffer(
        buffer_handle_t **bufHandle,
        int fd[],
        char *addr[],
        bool *isLocked)
{
    int bufStride = 0;
    status_t ret = NO_ERROR;

    ret = alloc(bufHandle, fd, addr, &bufStride, isLocked);
    if (ret == NO_INIT) {
        ALOGW("WARN(%s):BufferQueue is abandoned", __FUNCTION__);
        return ret;
    } else if (ret != NO_ERROR) {
        ALOGE("ERR(%s):alloc failed", __FUNCTION__);
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t ExynosCameraGrallocAllocator::enqueueBuffer(buffer_handle_t *handle)
{
    status_t ret = NO_ERROR;
    ExynosCameraDurationTimer   enqueuebufferTimer;

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        return INVALID_OPERATION;
    }

    enqueuebufferTimer.start();
    ret = m_allocator->enqueue_buffer(m_allocator, handle);
    enqueuebufferTimer.stop();

#if defined (EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE)
    ALOGD("DEBUG(%s[%d]):Check enqueue buffer performance, duration(%lld usec)",
            __FUNCTION__, __LINE__, enqueuebufferTimer.durationUsecs());
#else
    if (enqueuebufferTimer.durationMsecs() > GRALLOC_WARNING_DURATION_MSEC)
        ALOGW("WRN(%s[%d]):enqueue_buffer() duration(%lld msec)",
                __FUNCTION__, __LINE__, enqueuebufferTimer.durationMsecs());
#endif

    if (ret != 0) {
        ALOGE("ERR(%s):enqueue_buffer failed", __FUNCTION__);
        return INVALID_OPERATION;
    }

    return NO_ERROR;
}

status_t ExynosCameraGrallocAllocator::cancelBuffer(buffer_handle_t *handle)
{
    status_t ret = NO_ERROR;
    ExynosCameraDurationTimer   cancelbufferTimer;

    if (m_allocator == NULL) {
        ALOGE("ERR(%s):m_allocator equals NULL", __FUNCTION__);
        return INVALID_OPERATION;
    }

    if (m_grallocHal->unlock(m_grallocHal, *handle) != 0) {
        ALOGE("ERR(%s):grallocHal->unlock failed", __FUNCTION__);
        return INVALID_OPERATION;
    }

    cancelbufferTimer.start();
    ret = m_allocator->cancel_buffer(m_allocator, handle);
    cancelbufferTimer.stop();

#if defined (EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE)
    ALOGD("DEBUG(%s[%d]):Check cancel buffer performance, duration(%lld usec)",
            __FUNCTION__, __LINE__, cancelbufferTimer.durationUsecs());
#else
    if (cancelbufferTimer.durationMsecs() > GRALLOC_WARNING_DURATION_MSEC)
        ALOGW("WRN(%s[%d]):cancel_buffer() duration(%lld msec)",
                __FUNCTION__, __LINE__, cancelbufferTimer.durationMsecs());
#endif

    if (ret != 0) {
        ALOGE("ERR(%s):cancel_buffer failed", __FUNCTION__);
        return INVALID_OPERATION;
    }
    return NO_ERROR;
}

ExynosCameraStreamAllocator::ExynosCameraStreamAllocator()
{
    m_allocator = NULL;
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&m_grallocHal))
        ALOGE("ERR(%s):Loading gralloc HAL failed", __FUNCTION__);
}

ExynosCameraStreamAllocator::~ExynosCameraStreamAllocator()
{
}

status_t ExynosCameraStreamAllocator::init(camera3_stream_t *allocator)
{
    status_t ret = NO_ERROR;

    m_allocator = allocator;

    return ret;
}

int ExynosCameraStreamAllocator::lock(
        buffer_handle_t **bufHandle,
        int fd[],
        char *addr[],
        bool *isLocked,
        int planeCount)
{
    int ret = 0;
    uint32_t width  = 0;
    uint32_t height = 0;
    uint32_t usage  = 0;
    uint32_t format = 0;
    void  *grallocAddr[3] = {NULL};
    const private_handle_t *priv_handle = NULL;
    int   grallocFd[3] = {0};
    ExynosCameraDurationTimer   lockbufferTimer;

    if (bufHandle == NULL) {
        ALOGE("ERR(%s):bufHandle equals NULL, failed", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (*bufHandle == NULL) {
        ALOGE("ERR(%s):*bufHandle == NULL, failed", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    width  = m_allocator->width;
    height = m_allocator->height;
    usage  = m_allocator->usage;
    format = m_allocator->format;

    switch (format) {
    case HAL_PIXEL_FORMAT_YCbCr_420_888:
        android_ycbcr ycbcr;
        lockbufferTimer.start();
        ret = m_grallocHal->lock_ycbcr(
                m_grallocHal,
                **bufHandle,
                usage,
                0, 0, /* left, top */
                width, height,
                &ycbcr);
        lockbufferTimer.stop();
        grallocAddr[0] = ycbcr.y;
        break;
    default:
        lockbufferTimer.start();
        ret = m_grallocHal->lock(
                m_grallocHal,
                **bufHandle,
                usage,
                0, 0, /* left, top */
                width, height,
                grallocAddr);
        lockbufferTimer.stop();
        break;
    }

#if defined (EXYNOS_CAMERA_MEMORY_TRACE_GRALLOC_PERFORMANCE)
    ALOGD("DEBUG(%s[%d]):Check grallocHAL lock performance, duration(%lld usec)",
            __FUNCTION__, __LINE__, lockbufferTimer.durationUsecs());
#else
    if (lockbufferTimer.durationMsecs() > GRALLOC_WARNING_DURATION_MSEC)
        ALOGW("WRN(%s[%d]):grallocHAL->lock() duration(%lld msec)",
                __FUNCTION__, __LINE__, lockbufferTimer.durationMsecs());
#endif

    if (ret != 0) {
        ALOGE("ERR(%s):grallocHal->lock failed.. ", __FUNCTION__);
        goto func_exit;
    }

    priv_handle = private_handle_t::dynamicCast(**bufHandle);

    switch (planeCount) {
    case 3:
        grallocFd[2] = priv_handle->fd2;
    case 2:
        grallocFd[1] = priv_handle->fd1;
    case 1:
        grallocFd[0] = priv_handle->fd;
        break;
    default:
        break;
    }

    *isLocked    = true;

func_exit:
    switch (planeCount) {
    case 3:
        fd[2]   = grallocFd[2];
        addr[2] = (char *)grallocAddr[2];
    case 2:
        fd[1]   = grallocFd[1];
        addr[1] = (char *)grallocAddr[1];
    case 1:
        fd[0]   = grallocFd[0];
        addr[0] = (char *)grallocAddr[0];
        break;
    default:
        break;
    }

    return ret;
}

int ExynosCameraStreamAllocator::unlock(buffer_handle_t *bufHandle)
{
    int ret = 0;

    if (bufHandle == NULL) {
        ALOGE("ERR(%s):bufHandle equals NULL", __FUNCTION__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    ret = m_grallocHal->unlock(m_grallocHal, *bufHandle);

    if (ret != 0)
        ALOGE("ERR(%s):grallocHal->unlock failed", __FUNCTION__);

func_exit:
    return ret;
}
}
