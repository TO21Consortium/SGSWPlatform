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
 * \file      ExynosCameraBufferManager.h
 * \brief     header file for ExynosCameraBufferManager
 * \author    Sunmi Lee(carrotsm.lee@samsung.com)
 * \date      2013/07/17
 *
 */

#ifndef EXYNOS_CAMERA_BUFFER_MANAGER_H__
#define EXYNOS_CAMERA_BUFFER_MANAGER_H__


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <utils/List.h>
#include <utils/threads.h>
#include <cutils/properties.h>

#include <binder/MemoryHeapBase.h>
#include <ui/Fence.h>
#include <videodev2.h>
#include <videodev2_exynos_camera.h>
#include <ion/ion.h>

#include "gralloc_priv.h"

#include "ExynosCameraConfig.h"
#include "fimc-is-metadata.h"

#include "ExynosCameraList.h"
#include "ExynosCameraAutoTimer.h"
#include "ExynosCameraBuffer.h"
#include "ExynosCameraMemory.h"
#include "ExynosCameraThread.h"

namespace android {

/* #define DUMP_2_FILE */
/* #define EXYNOS_CAMERA_BUFFER_TRACE */

#ifdef EXYNOS_CAMERA_BUFFER_TRACE
#define EXYNOS_CAMERA_BUFFER_IN()   CLOGD("DEBUG(%s[%d]):IN.." , __FUNCTION__, __LINE__)
#define EXYNOS_CAMERA_BUFFER_OUT()  CLOGD("DEBUG(%s[%d]):OUT..", __FUNCTION__, __LINE__)
#else
#define EXYNOS_CAMERA_BUFFER_IN()   ((void *)0)
#define EXYNOS_CAMERA_BUFFER_OUT()  ((void *)0)
#endif

#ifdef USE_GRALLOC_BUFFER_COLLECTOR
#define MAX_BUFFER_COLLECT_COUNT        (6)
#define BUFFER_COLLECTOR_WAITING_TIME   (10000) /* 10 msec */
#endif

// Hack: Close Fence FD if the fd is larger than specified number
// Currently, Joon's fence FD is not closed properly
#define FORCE_CLOSE_ACQUIRE_FD
#define FORCE_CLOSE_ACQUIRE_FD_THRESHOLD    700


typedef enum buffer_manager_type {
    BUFFER_MANAGER_ION_TYPE                 = 0,
    BUFFER_MANAGER_HEAP_BASE_TYPE           = 1,
    BUFFER_MANAGER_GRALLOC_TYPE             = 2,
    BUFFER_MANAGER_SERVICE_GRALLOC_TYPE     = 3,
    BUFFER_MANAGER_INVALID_TYPE,
} buffer_manager_type_t;

typedef enum buffer_manager_allocation_mode {
    BUFFER_MANAGER_ALLOCATION_ATONCE   = 0,   /* alloc() : allocation all buffers */
    BUFFER_MANAGER_ALLOCATION_ONDEMAND = 1,   /* alloc() : allocation the number of reqCount buffers, getBuffer() : increase buffers within limits */
    BUFFER_MANAGER_ALLOCATION_SILENT   = 2,   /* alloc() : same as ONDEMAND, increase buffers in background */
    BUFFER_MANAGER_ALLOCATION_INVALID_MODE,
} buffer_manager_allocation_mode_t;

typedef ExynosCameraList<int32_t> reuseBufList_t;

class ExynosCameraBufferManager {
protected:
    ExynosCameraBufferManager();

public:
    static ExynosCameraBufferManager *createBufferManager(buffer_manager_type_t type);
    virtual ~ExynosCameraBufferManager();

    status_t create(const char *name, void *defaultAllocator);
    status_t create(const char *name, int cameraId, void *defaultAllocator);

    void     init(void);
    virtual void     deinit(void);
    virtual status_t resetBuffers(void);

    status_t setAllocator(void *allocator);

    status_t alloc(void);

    void     setContigBufCount(int reservedMemoryCount);
    int      getContigBufCount(void);
    status_t setInfo(
                int planeCount,
                unsigned int size[],
                unsigned int bytePerLine[],
                int reqBufCount,
                bool createMetaPlane,
                bool needMmap = false);
    status_t setInfo(
                int planeCount,
                unsigned int size[],
                unsigned int bytePerLine[],
                int reqBufCount,
                int allowedMaxBufCount,
                exynos_camera_buffer_type_t type,
                bool createMetaPlane,
                bool needMmap = false);
    status_t setInfo(
                int planeCount,
                unsigned int size[],
                unsigned int bytePerLine[],
                int startBufIndex,
                int reqBufCount,
                bool createMetaPlane,
                bool needMmap = false);
    status_t setInfo(
                int planeCount,
                unsigned int size[],
                unsigned int bytePerLine[],
                int startBufIndex,
                int reqBufCount,
                int allowedMaxBufCount,
                exynos_camera_buffer_type_t type,
                buffer_manager_allocation_mode_t allocMode,
                bool createMetaPlane,
                bool needMmap = false);

    status_t setInfo(
                int planeCount,
                unsigned int size[],
                unsigned int bytePerLine[],
                int startBufIndex,
                int reqBufCount,
                int allowedMaxBufCount,
                exynos_camera_buffer_type_t type,
                buffer_manager_allocation_mode_t allocMode,
                bool createMetaPlane,
                int width,
                int height,
                int stride,
                int pixelFormat,
                bool needMmap = false);

    status_t putBuffer(
                int bufIndex,
                enum EXYNOS_CAMERA_BUFFER_POSITION position);
    status_t getBuffer(
                int    *reqBufIndex,
                enum   EXYNOS_CAMERA_BUFFER_POSITION position,
                struct ExynosCameraBuffer *buffer);

    status_t updateStatus(
                int bufIndex,
                int driverValue,
                enum EXYNOS_CAMERA_BUFFER_POSITION   position,
                enum EXYNOS_CAMERA_BUFFER_PERMISSION permission);
    status_t getStatus(
                int bufIndex,
                struct ExynosCameraBufferStatus *bufStatus);

    virtual status_t getIndexByHandle(buffer_handle_t *handle, int *index);
    virtual status_t getHandleByIndex(buffer_handle_t **handle, int index);
    virtual sp<GraphicBuffer> getGraphicBuffer(int index);

    virtual status_t registerBuffer(
                int frameCount,
                buffer_handle_t *handle,
                int acquireFence,
                int releaseFence,
                enum EXYNOS_CAMERA_BUFFER_POSITION position);

    bool     isAllocated(void);
    bool     isAvaliable(int bufIndex);

    void     dump(void);
    void     dumpBufferInfo(void);
    int      getAllocatedBufferCount(void);
    int      getAvailableIncreaseBufferCount(void);
    int      getNumOfAvailableBuffer(void);
    int      getNumOfAvailableAndNoneBuffer(void);
    void     printBufferInfo(
                const char *funcName,
                const int lineNum,
                int bufIndex,
                int planeIndex);
    void     printBufferQState(void);
    virtual void printBufferState(void);
    virtual void printBufferState(int bufIndex, int planeIndex);

    virtual status_t increase(int increaseCount);
#ifdef USE_GRALLOC_REUSE_SUPPORT
    virtual status_t cancelBuffer(int bufIndex, bool isReuse = false);
#else
    virtual status_t cancelBuffer(int bufIndex);
#endif
    virtual status_t setBufferCount(int bufferCount);
    virtual int      getBufferCount(void);
    virtual int      getBufStride(void);

protected:
    status_t m_free(void);

    status_t m_setDefaultAllocator(void *allocator);
    status_t m_defaultAlloc(int bIndex, int eIndex, bool isMetaPlane);
    status_t m_defaultFree(int bIndex, int eIndex, bool isMetaPlane);

    bool     m_checkInfoForAlloc(void);
    status_t m_createDefaultAllocator(bool isCached = false);

    virtual void     m_resetSequenceQ(void);

    virtual status_t m_setAllocator(void *allocator) = 0;
    virtual status_t m_alloc(int bIndex, int eIndex) = 0;
    virtual status_t m_free(int bIndex, int eIndex)  = 0;

    virtual status_t m_increase(int increaseCount) = 0;
    virtual status_t m_decrease(void) = 0;

    virtual status_t m_putBuffer(int bufIndex) = 0;
    virtual status_t m_getBuffer(int *bufIndex, int *acquireFence, int *releaseFence) = 0;

protected:
    bool                        m_flagAllocated;
    int                         m_reservedMemoryCount;
    int                         m_reqBufCount;
    int                         m_allocatedBufCount;
    int                         m_allowedMaxBufCount;
    bool                        m_flagSkipAllocation;
    bool                        m_isDestructor;
    mutable Mutex               m_lock;
    bool                        m_flagNeedMmap;

    bool                        m_hasMetaPlane;
    /* using internal allocator (ION) for MetaData plane */
    ExynosCameraIonAllocator    *m_defaultAllocator;
    bool                        m_isCreateDefaultAllocator;
    struct ExynosCameraBuffer   m_buffer[VIDEO_MAX_FRAME];
    char                        m_name[EXYNOS_CAMERA_NAME_STR_SIZE];
    int                         m_cameraId;
    List<int>                   m_availableBufferIndexQ;
    mutable Mutex               m_availableBufferIndexQLock;

    buffer_manager_allocation_mode_t m_allocMode;
    int                         m_indexOffset;

    ExynosCameraGraphicBufferAllocator m_graphicBufferAllocator;

private:
    typedef ExynosCameraThread<ExynosCameraBufferManager> allocThread;

    sp<allocThread>             m_allocationThread;
    bool                        m_allocationThreadFunc(void);
};

class InternalExynosCameraBufferManager : public ExynosCameraBufferManager {
public:
    InternalExynosCameraBufferManager();
    virtual ~InternalExynosCameraBufferManager();

    status_t increase(int increaseCount);

protected:
    status_t m_setAllocator(void *allocator);

    status_t m_alloc(int bIndex, int eIndex);
    status_t m_free(int bIndex, int eIndex);

    status_t m_increase(int increaseCount);
    status_t m_decrease(void);

    status_t m_putBuffer(int bufIndex);
    status_t m_getBuffer(int *bufIndex, int *acquireFence, int *releaseFence);
};

class MHBExynosCameraBufferManager : public ExynosCameraBufferManager {
/* do not use! deprecated class */
public:
    MHBExynosCameraBufferManager();
    virtual ~MHBExynosCameraBufferManager();

    status_t allocMulti();
    status_t getHeapMemory(
                int bufIndex,
                int planeIndex,
                camera_memory_t **heap);

protected:
    status_t m_setAllocator(void *allocator);

    status_t m_alloc(int bIndex, int eIndex);
    status_t m_free(int bIndex, int eIndex);

    status_t m_increase(int increaseCount);
    status_t m_decrease(void);

    status_t m_putBuffer(int bufIndex);
    status_t m_getBuffer(int *bufIndex, int *acquireFence, int *releaseFence);

private:
    ExynosCameraMHBAllocator *m_allocator;
    camera_memory_t          *m_heap[VIDEO_MAX_FRAME][EXYNOS_CAMERA_BUFFER_MAX_PLANES];
    int                      m_numBufsHeap;
};

class GrallocExynosCameraBufferManager : public ExynosCameraBufferManager {
public:
    GrallocExynosCameraBufferManager();
    virtual ~GrallocExynosCameraBufferManager();

    void     deinit(void);
    status_t resetBuffers(void);
#ifdef USE_GRALLOC_REUSE_SUPPORT
    status_t cancelBuffer(int bufIndex, bool isReuse = false);
#else
    status_t cancelBuffer(int bufIndex);
#endif
    status_t setBufferCount(int bufferCount);
    int      getBufferCount(void);
    int      getBufStride(void);
    void     printBufferState(void);
    void     printBufferState(int bufIndex, int planeIndex);

    sp<GraphicBuffer> getGraphicBuffer(int index);

protected:
    status_t m_setAllocator(void *allocator);

    status_t m_alloc(int bIndex, int eIndex);
    status_t m_free(int bIndex, int eIndex);

    status_t m_increase(int increaseCount);
    status_t m_decrease(void);

    status_t m_putBuffer(int bufIndex);
    status_t m_getBuffer(int *bufIndex, int *acquireFence, int *releaseFence);

#ifdef USE_GRALLOC_REUSE_SUPPORT
    bool m_cancelReuseBuffer(int bufIndex, bool isReuse);
#endif

private:
    ExynosCameraGrallocAllocator *m_allocator;
    buffer_handle_t              *m_handle[VIDEO_MAX_FRAME];
    bool                         m_handleIsLocked[VIDEO_MAX_FRAME];
    int                          m_dequeuedBufCount;
    int                          m_minUndequeuedBufCount;
    int                          m_bufferCount;
    int                          m_bufStride;
#ifdef USE_GRALLOC_BUFFER_COLLECTOR
    typedef ExynosCameraThread<GrallocExynosCameraBufferManager> grallocBufferThread;

    int                         m_collectedBufferCount;
    bool                        m_stopBufferCollector;

    sp<grallocBufferThread>     m_bufferCollector;
    bool                        m_bufferCollectorThreadFunc(void);

    status_t                    m_getCollectedBuffer(int *bufIndex);
    bool                        m_isCollectedBuffer(int bufferIndex);
#endif
};

class ExynosCameraFence {
public:
    enum EXYNOS_CAMERA_FENCE_TYPE {
        EXYNOS_CAMERA_FENCE_TYPE_BASE = 0,
        EXYNOS_CAMERA_FENCE_TYPE_ACQUIRE,
        EXYNOS_CAMERA_FENCE_TYPE_RELEASE,
        EXYNOS_CAMERA_FENCE_TYPE_MAX,
    };

private:
    ExynosCameraFence();

public:
    ExynosCameraFence(
                enum EXYNOS_CAMERA_FENCE_TYPE fenceType,
                int frameCount,
                int index,
                int acquireFence,
                int releaseFence);

    virtual ~ExynosCameraFence();

    int  getFenceType(void);
    int  getFrameCount(void);
    int  getIndex(void);
    int  getAcquireFence(void);
    int  getReleaseFence(void);

    bool isValid(void);
    status_t wait(int time = -1);

private:
    enum EXYNOS_CAMERA_FENCE_TYPE m_fenceType;

    int       m_frameCount;
    int       m_index;

    int       m_acquireFence;
    int       m_releaseFence;

    sp<Fence> m_fence;

    bool      m_flagSwfence;
};

class ServiceExynosCameraBufferManager : public ExynosCameraBufferManager {
public:
    ServiceExynosCameraBufferManager();
    virtual ~ServiceExynosCameraBufferManager();

    status_t getIndexByHandle(buffer_handle_t *handle, int *index);
    status_t getHandleByIndex(buffer_handle_t **handle, int index);

    /*
     * The H/W fence sequence is
     * 1. On putBuffer (call by processCaptureRequest()),
     *    And, save acquire_fence, release_fence value.
     *    S/W fence : Make Fence class with acquire_fence on output_buffer.
     *
     * 2. when getBuffer,
     *    H/W fence : save acquire_fence, release_fence to ExynosCameraBuffer.
     *    S/W fence : wait Fence class that allocated at step 1.
     *
     *    (step 3 ~ step 4 is on ExynosCameraNode::m_qBuf())
     * 3. During H/W qBuf,
     *    give ExynosCameraBuffer.acquireFence (gotten step2) to v4l2
     *    v4l2_buffer.flags = V4L2_BUF_FLAG_USE_SYNC;
     *    v4l2_buffer.reserved = ExynosCameraBuffer.acquireFence;
     * 4. after H/W qBuf
     *    v4l2_buffer.reserved is changed to release_fence value.
     *    So,
     *    ExynosCameraBuffer.releaseFence = static_cast<int>(v4l2_buffer.reserved)
     *
     * 5. (step5 is on ExynosCamera3::m_setResultBufferInfo())
     *    after H/W dqBuf, we meet handlePreview().
     *    we can set final value.
     *    result_buffer_info_t.acquire_fence = -1. (NOT original ExynosCameraBuffer.acquireFence)
     *    result_buffer_info_t.release_fence = ExynosCameraBuffer.releaseFence. (gotten by driver at step3)
     *    (service will look this release_fence.)
     *
     * 6  skip bufferManger::putBuffer().
     *     (we don't need to call putBuffer. because, buffer came from service.)
     *
     * 7. repeat from 1.
     */
    status_t registerBuffer(
                int frameCount,
                buffer_handle_t *handle,
                int acquireFence,
                int releaseFence,
                enum EXYNOS_CAMERA_BUFFER_POSITION position);

protected:
    virtual void     m_resetSequenceQ(void);

    status_t m_setAllocator(void *allocator);
    status_t m_alloc(int bIndex, int eIndex);
    status_t m_free(int bIndex, int eIndex);

    status_t m_increase(int increaseCount);
    status_t m_decrease(void);

    status_t m_registerBuffer(buffer_handle_t **handle, int index);

    status_t m_putBuffer(int bufIndex);
    status_t m_getBuffer(int *bufIndex, int *acquireFence, int *releaseFence);

    virtual status_t m_waitFence(ExynosCameraFence *fence);

    virtual void     m_pushFence(List<ExynosCameraFence *> *list, ExynosCameraFence *fence);
    virtual ExynosCameraFence *m_popFence(List<ExynosCameraFence *> *list);
    virtual status_t m_removeFence(List<ExynosCameraFence *> *list, int index);
    virtual status_t m_compareFdOfBufferHandle(const buffer_handle_t* handle, const ExynosCameraBuffer* exynosBuf);

private:
    ExynosCameraStreamAllocator     *m_allocator;
    buffer_handle_t                 *m_handle[VIDEO_MAX_FRAME];
    bool                            m_handleIsLocked[VIDEO_MAX_FRAME];

    List<ExynosCameraFence *>       m_fenceList;
    mutable Mutex                   m_fenceListLock;
};
}
#endif
