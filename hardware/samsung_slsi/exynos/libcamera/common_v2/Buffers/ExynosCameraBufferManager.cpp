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

#define LOG_TAG "ExynosCameraBufferManager"
#include "ExynosCameraBufferManager.h"

namespace android {

ExynosCameraBufferManager::ExynosCameraBufferManager()
{
    m_isDestructor = false;
    m_cameraId = 0;

    init();

    m_allocationThread = new allocThread(this, &ExynosCameraBufferManager::m_allocationThreadFunc, "allocationThreadFunc");
}

ExynosCameraBufferManager::~ExynosCameraBufferManager()
{
    m_isDestructor = true;
}

ExynosCameraBufferManager *ExynosCameraBufferManager::createBufferManager(buffer_manager_type_t type)
{
    switch (type) {
    case BUFFER_MANAGER_ION_TYPE:
        return (ExynosCameraBufferManager *)new InternalExynosCameraBufferManager();
        break;
    case BUFFER_MANAGER_HEAP_BASE_TYPE:
        return (ExynosCameraBufferManager *)new MHBExynosCameraBufferManager();
        break;
    case BUFFER_MANAGER_GRALLOC_TYPE:
        return (ExynosCameraBufferManager *)new GrallocExynosCameraBufferManager();
        break;
    case BUFFER_MANAGER_SERVICE_GRALLOC_TYPE:
        return (ExynosCameraBufferManager *)new ServiceExynosCameraBufferManager();
        break;
    case BUFFER_MANAGER_INVALID_TYPE:
        ALOGE("ERR(%s[%d]):Unknown bufferManager type(%d)", __FUNCTION__, __LINE__, (int)type);
    default:
        break;
    }

    return NULL;
}

status_t ExynosCameraBufferManager::create(const char *name, int cameraId, void *defaultAllocator)
{
    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;

    m_cameraId = cameraId;
    strncpy(m_name, name, EXYNOS_CAMERA_NAME_STR_SIZE - 1);

    if (defaultAllocator == NULL) {
        if (m_createDefaultAllocator(false) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_createDefaultAllocator failed", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }
    } else {
        if (m_setDefaultAllocator(defaultAllocator) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_setDefaultAllocator failed", __FUNCTION__, __LINE__);
            return INVALID_OPERATION;
        }
    }

    return ret;
}

status_t ExynosCameraBufferManager::create(const char *name, void *defaultAllocator)
{
    return create(name, 0, defaultAllocator);
}

void ExynosCameraBufferManager::init(void)
{
    EXYNOS_CAMERA_BUFFER_IN();

    m_flagAllocated = false;
    m_reservedMemoryCount = 0;
    m_reqBufCount  = 0;
    m_allocatedBufCount  = 0;
    m_allowedMaxBufCount = 0;
    m_defaultAllocator = NULL;
    m_isCreateDefaultAllocator = false;
    memset((void *)m_buffer, 0, (VIDEO_MAX_FRAME) * sizeof(struct ExynosCameraBuffer));
    for (int bufIndex = 0; bufIndex < VIDEO_MAX_FRAME; bufIndex++) {
        for (int planeIndex = 0; planeIndex < EXYNOS_CAMERA_BUFFER_MAX_PLANES; planeIndex++) {
            m_buffer[bufIndex].fd[planeIndex] = -1;
        }
    }
    m_hasMetaPlane = false;
    memset(m_name, 0x00, sizeof(m_name));
    strncpy(m_name, "none", EXYNOS_CAMERA_NAME_STR_SIZE - 1);
    m_flagSkipAllocation = false;
    m_flagNeedMmap = false;
    m_allocMode = BUFFER_MANAGER_ALLOCATION_ATONCE;
    m_indexOffset = 0;

    m_graphicBufferAllocator.init();

    EXYNOS_CAMERA_BUFFER_OUT();
}

void ExynosCameraBufferManager::deinit(void)
{
    CLOGD("DEBUG(%s[%d]):IN.." , __FUNCTION__, __LINE__);

    if (m_flagAllocated == false) {
        CLOGI("INFO(%s[%d]:OUT.. Buffer is not allocated", __FUNCTION__, __LINE__);
        return;
    }

    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_SILENT) {
        m_allocationThread->join();
        CLOGI("INFO(%s[%d]):allocationThread is finished", __FUNCTION__, __LINE__);
    }

    for (int bufIndex = 0; bufIndex < m_allocatedBufCount; bufIndex++)
        cancelBuffer(bufIndex);

    if (m_free() != NO_ERROR)
        CLOGE("ERR(%s[%d])::free failed", __FUNCTION__, __LINE__);

    if (m_defaultAllocator != NULL && m_isCreateDefaultAllocator == true) {
        delete m_defaultAllocator;
        m_defaultAllocator = NULL;
    }

    m_reservedMemoryCount = 0;
    m_flagSkipAllocation = false;
    CLOGD("DEBUG(%s[%d]):OUT..", __FUNCTION__, __LINE__);
}

status_t ExynosCameraBufferManager::resetBuffers(void)
{
    /* same as deinit */
    /* clear buffers except releasing the memory */
    CLOGD("DEBUG(%s[%d]):IN.." , __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    if (m_flagAllocated == false) {
        CLOGI("INFO(%s[%d]:OUT.. Buffer is not allocated", __FUNCTION__, __LINE__);
        return ret;
    }

    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_SILENT) {
        m_allocationThread->join();
        CLOGI("INFO(%s[%d]):allocationThread is finished", __FUNCTION__, __LINE__);
    }

    for (int bufIndex = m_indexOffset; bufIndex < m_allocatedBufCount + m_indexOffset; bufIndex++)
        cancelBuffer(bufIndex);

    m_resetSequenceQ();
    m_flagSkipAllocation = true;

    return ret;
}

status_t ExynosCameraBufferManager::setAllocator(void *allocator)
{
    Mutex::Autolock lock(m_lock);

    if (allocator == NULL) {
        CLOGE("ERR(%s[%d]):m_allocator equals NULL", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    return m_setAllocator(allocator);
}

status_t ExynosCameraBufferManager::alloc(void)
{
    EXYNOS_CAMERA_BUFFER_IN();
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;

    if (m_flagSkipAllocation == true) {
        CLOGI("INFO(%s[%d]):skip to allocate memory (m_flagSkipAllocation=%d)",
            __FUNCTION__, __LINE__, (int)m_flagSkipAllocation);
        goto func_exit;
    }

    if (m_checkInfoForAlloc() == false) {
        CLOGE("ERR(%s[%d]):m_checkInfoForAlloc failed", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_hasMetaPlane == true) {
        if (m_defaultAlloc(m_indexOffset, m_reqBufCount + m_indexOffset, m_hasMetaPlane) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_defaultAlloc failed", __FUNCTION__, __LINE__);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

    /* allocate image buffer */
    if (m_alloc(m_indexOffset, m_reqBufCount + m_indexOffset) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_alloc failed", __FUNCTION__, __LINE__);

        if (m_hasMetaPlane == true) {
            CLOGD("DEBUG(%s[%d]):Free metadata plane. bufferCount %d",
                    __FUNCTION__, __LINE__, m_reqBufCount);
            if (m_defaultFree(m_indexOffset, m_reqBufCount + m_indexOffset, m_hasMetaPlane) != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_defaultFree failed", __FUNCTION__, __LINE__);
            }
        }

        ret = INVALID_OPERATION;
        goto func_exit;
    }

    m_allocatedBufCount = m_reqBufCount;
    m_resetSequenceQ();
    m_flagAllocated = true;

    CLOGD("DEBUG(%s[%d]):Allocate the buffer succeeded "
          "(m_allocatedBufCount=%d, m_reqBufCount=%d, m_allowedMaxBufCount=%d) --- dumpBufferInfo ---",
        __FUNCTION__, __LINE__,
        m_allocatedBufCount, m_reqBufCount, m_allowedMaxBufCount);
    dumpBufferInfo();
    CLOGD("DEBUG(%s[%d]):------------------------------------------------------------------",
        __FUNCTION__, __LINE__);

    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_SILENT) {
        /* run the allocationThread */
        m_allocationThread->run(PRIORITY_DEFAULT);
        CLOGI("INFO(%s[%d]):allocationThread is started", __FUNCTION__, __LINE__);
    }

func_exit:

    m_flagSkipAllocation = false;
    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ExynosCameraBufferManager::m_free(void)
{
    EXYNOS_CAMERA_BUFFER_IN();
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    Mutex::Autolock lock(m_lock);

    CLOGD("DEBUG(%s[%d]):Free the buffer (m_allocatedBufCount=%d) --- dumpBufferInfo ---",
        __FUNCTION__, __LINE__, m_allocatedBufCount);
    dumpBufferInfo();
    CLOGD("DEBUG(%s[%d]):------------------------------------------------------",
        __FUNCTION__, __LINE__);

    status_t ret = NO_ERROR;

    if (m_flagAllocated != false) {
        if (m_free(m_indexOffset, m_allocatedBufCount + m_indexOffset) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_free failed", __FUNCTION__, __LINE__);
            ret = INVALID_OPERATION;
            goto func_exit;
        }

        if (m_hasMetaPlane == true) {
            if (m_defaultFree(m_indexOffset, m_allocatedBufCount + m_indexOffset, m_hasMetaPlane) != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_defaultFree failed", __FUNCTION__, __LINE__);
                ret = INVALID_OPERATION;
                goto func_exit;
            }
        }
        m_availableBufferIndexQLock.lock();
        m_availableBufferIndexQ.clear();
        m_availableBufferIndexQLock.unlock();
        m_allocatedBufCount  = 0;
        m_allowedMaxBufCount = 0;
        m_flagAllocated = false;
    }

    CLOGD("DEBUG(%s[%d]):Free the buffer succeeded (m_allocatedBufCount=%d)",
        __FUNCTION__, __LINE__, m_allocatedBufCount);

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

void ExynosCameraBufferManager::m_resetSequenceQ()
{
    Mutex::Autolock lock(m_availableBufferIndexQLock);
    m_availableBufferIndexQ.clear();

    for (int bufIndex = m_indexOffset; bufIndex < m_allocatedBufCount + m_indexOffset; bufIndex++)
        m_availableBufferIndexQ.push_back(m_buffer[bufIndex].index);

    return;
}

void ExynosCameraBufferManager::setContigBufCount(int reservedMemoryCount)
{
    ALOGI("INFO(%s[%d]):reservedMemoryCount(%d)", __FUNCTION__, __LINE__, reservedMemoryCount);
    m_reservedMemoryCount = reservedMemoryCount;
    return;
}

int ExynosCameraBufferManager::getContigBufCount(void)
{
    return m_reservedMemoryCount;
}

/*  If Image buffer color format equals YV12, and buffer has MetaDataPlane..

    planeCount = 4      (set by user)
    size[0] : Image buffer plane Y size
    size[1] : Image buffer plane Cr size
    size[2] : Image buffer plane Cb size

    if (createMetaPlane == true)
        size[3] = EXYNOS_CAMERA_META_PLANE_SIZE;    (set by BufferManager, internally)
*/
status_t ExynosCameraBufferManager::setInfo(
        int planeCount,
        unsigned int size[],
        unsigned int bytePerLine[],
        int reqBufCount,
        bool createMetaPlane,
        bool needMmap)
{
    status_t ret = NO_ERROR;

    ret = setInfo(
            planeCount,
            size,
            bytePerLine,
            0,
            reqBufCount,
            reqBufCount,
            EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE,
            BUFFER_MANAGER_ALLOCATION_ATONCE,
            createMetaPlane,
            needMmap);
    if (ret < 0)
        ALOGE("ERR(%s[%d]):setInfo fail", __FUNCTION__, __LINE__);

    return ret;
}

status_t ExynosCameraBufferManager::setInfo(
        int planeCount,
        unsigned int size[],
        unsigned int bytePerLine[],
        int reqBufCount,
        int allowedMaxBufCount,
        exynos_camera_buffer_type_t type,
        bool createMetaPlane,
        bool needMmap)
{
    status_t ret = NO_ERROR;

    ret = setInfo(
            planeCount,
            size,
            bytePerLine,
            0,
            reqBufCount,
            allowedMaxBufCount,
            type,
            BUFFER_MANAGER_ALLOCATION_ONDEMAND,
            createMetaPlane,
            needMmap);
    if (ret < 0)
        ALOGE("ERR(%s[%d]):setInfo fail", __FUNCTION__, __LINE__);

    return ret;
}

status_t ExynosCameraBufferManager::setInfo(
        int planeCount,
        unsigned int size[],
        unsigned int bytePerLine[],
        int startBufIndex,
        int reqBufCount,
        bool createMetaPlane,
        bool needMmap)
{
    status_t ret = NO_ERROR;

    ret = setInfo(
            planeCount,
            size,
            bytePerLine,
            startBufIndex,
            reqBufCount,
            reqBufCount,
            EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE,
            BUFFER_MANAGER_ALLOCATION_ATONCE,
            createMetaPlane,
            needMmap);
    if (ret < 0)
        ALOGE("ERR(%s[%d]):setInfo fail", __FUNCTION__, __LINE__);

    return ret;
}

status_t ExynosCameraBufferManager::setInfo(
        int planeCount,
        unsigned int size[],
        unsigned int bytePerLine[],
        int startBufIndex,
        int reqBufCount,
        int allowedMaxBufCount,
        exynos_camera_buffer_type_t type,
        buffer_manager_allocation_mode_t allocMode,
        bool createMetaPlane,
        bool needMmap)
{
    EXYNOS_CAMERA_BUFFER_IN();
    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;

    if (m_indexOffset > 0) {
        CLOGD("DEBUG(%s[%d]):buffer indexOffset(%d), Index[0 - %d] not used",
            __FUNCTION__, __LINE__, m_indexOffset, m_indexOffset);
    }
    m_indexOffset = startBufIndex;

    if (createMetaPlane == true) {
        size[planeCount-1] = EXYNOS_CAMERA_META_PLANE_SIZE;
        m_hasMetaPlane = true;
    }

    if (allowedMaxBufCount < reqBufCount) {
        CLOGW("WARN(%s[%d]):abnormal value [reqBufCount=%d, allowedMaxBufCount=%d]",
            __FUNCTION__, __LINE__, reqBufCount, allowedMaxBufCount);
        allowedMaxBufCount = reqBufCount;
    }

    if (reqBufCount < 0 || VIDEO_MAX_FRAME < reqBufCount) {
        CLOGE("ERR(%s[%d]):abnormal value [reqBufCount=%d]",
            __FUNCTION__, __LINE__, reqBufCount);
        ret = BAD_VALUE;
        goto func_exit;
    }

    if (planeCount < 0 || EXYNOS_CAMERA_BUFFER_MAX_PLANES <= planeCount) {
        CLOGE("ERR(%s[%d]):abnormal value [planeCount=%d]",
            __FUNCTION__, __LINE__, planeCount);
        ret = BAD_VALUE;
        goto func_exit;
    }

    for (int bufIndex = m_indexOffset; bufIndex < allowedMaxBufCount + m_indexOffset; bufIndex++) {
        for (int planeIndex = 0; planeIndex < planeCount; planeIndex++) {
            if (size[planeIndex] == 0) {
                CLOGE("ERR(%s[%d]):abnormal value [size=%d]",
                    __FUNCTION__, __LINE__, size[planeIndex]);
                ret = BAD_VALUE;
                goto func_exit;
            }
            m_buffer[bufIndex].size[planeIndex]         = size[planeIndex];
            m_buffer[bufIndex].bytesPerLine[planeIndex] = bytePerLine[planeIndex];
        }
        m_buffer[bufIndex].planeCount = planeCount;
        m_buffer[bufIndex].type       = type;
    }
    m_allowedMaxBufCount = allowedMaxBufCount + startBufIndex;
    m_reqBufCount  = reqBufCount;
    m_flagNeedMmap = needMmap;
    m_allocMode    = allocMode;
func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ExynosCameraBufferManager::setInfo(
        int planeCount,
        unsigned int size[],
        unsigned int bytePerLine[],
        int startBufIndex,
        int reqBufCount,
        __unused int allowedMaxBufCount,
        __unused exynos_camera_buffer_type_t type,
        __unused buffer_manager_allocation_mode_t allocMode,
        bool createMetaPlane,
        int width,
        int height,
        int stride,
        int pixelFormat,
        bool needMmap)
{
    status_t ret = NO_ERROR;

    ret = setInfo(
            planeCount,
            size,
            bytePerLine,
            startBufIndex,
            reqBufCount,
            reqBufCount,
            type,
            allocMode,
            createMetaPlane,
            needMmap);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):setInfo fail", __FUNCTION__, __LINE__);
        return ret;
    }

    ret = m_graphicBufferAllocator.setSize(width, height, stride);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):m_graphicBufferAllocator.setSiz(%d, %d, %d) fail",
            __FUNCTION__, __LINE__, width, height, stride);
        return ret;
    }

    ret = m_graphicBufferAllocator.setHalPixelFormat(pixelFormat);
    if (ret != NO_ERROR) {
        ALOGE("ERR(%s[%d]):m_graphicBufferAllocator.setHalPixelFormat(%d) fail",
            __FUNCTION__, __LINE__, pixelFormat);
        return ret;
    }

    return ret;
}

bool ExynosCameraBufferManager::m_allocationThreadFunc(void)
{
    status_t ret = NO_ERROR;
    int increaseCount = 1;

    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    CLOGI("INFO(%s[%d]:increase buffer silently - start - "
          "(m_allowedMaxBufCount=%d, m_allocatedBufCount=%d, m_reqBufCount=%d)",
        __FUNCTION__, __LINE__,
        m_allowedMaxBufCount, m_allocatedBufCount, m_reqBufCount);

    increaseCount = m_allowedMaxBufCount - m_reqBufCount;

    /* increase buffer*/
    for (int count = 0; count < increaseCount; count++) {
        ret = m_increase(1);
        if (ret < 0) {
            CLOGE("ERR(%s[%d]):increase the buffer failed", __FUNCTION__, __LINE__);
        } else {
            m_lock.lock();
            m_availableBufferIndexQ.push_back(m_buffer[m_allocatedBufCount + m_indexOffset].index);
            m_allocatedBufCount++;
            m_lock.unlock();
        }

    }
    dumpBufferInfo();
    CLOGI("INFO(%s[%d]:increase buffer silently - end - (increaseCount=%d)"
          "(m_allowedMaxBufCount=%d, m_allocatedBufCount=%d, m_reqBufCount=%d)",
        __FUNCTION__, __LINE__, increaseCount,
        m_allowedMaxBufCount, m_allocatedBufCount, m_reqBufCount);

    /* false : Thread run once */
    return false;
}

status_t ExynosCameraBufferManager::registerBuffer(
        __unused int frameCount,
        __unused buffer_handle_t *handle,
        __unused int acquireFence,
        __unused int releaseFence,
        __unused enum EXYNOS_CAMERA_BUFFER_POSITION position)
{
    return NO_ERROR;
}

status_t ExynosCameraBufferManager::putBuffer(
        int bufIndex,
        enum EXYNOS_CAMERA_BUFFER_POSITION position)
{
    EXYNOS_CAMERA_BUFFER_IN();
    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;
    List<int>::iterator r;
    bool found = false;
    enum EXYNOS_CAMERA_BUFFER_PERMISSION permission;

    permission = EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE;

    if (bufIndex < 0 || m_allocatedBufCount + m_indexOffset <= bufIndex) {
        CLOGE("ERR(%s[%d]):buffer Index in out of bound [bufIndex=%d], allocatedBufCount(%d)",
            __FUNCTION__, __LINE__, bufIndex, m_allocatedBufCount);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    m_availableBufferIndexQLock.lock();
    for (r = m_availableBufferIndexQ.begin(); r != m_availableBufferIndexQ.end(); r++) {
        if (bufIndex == *r) {
            found = true;
            break;
        }
    }
    m_availableBufferIndexQLock.unlock();

    if (found == true) {
        CLOGI("INFO(%s[%d]):bufIndex=%d is already in (available state)",
            __FUNCTION__, __LINE__, bufIndex);
        goto func_exit;
    }

    if (m_putBuffer(bufIndex) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_putBuffer failed [bufIndex=%d, position=%d, permission=%d]",
            __FUNCTION__, __LINE__, bufIndex, (int)position, (int)permission);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (updateStatus(bufIndex, 0, position, permission) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):setStatus failed [bufIndex=%d, position=%d, permission=%d]",
            __FUNCTION__, __LINE__, bufIndex, (int)position, (int)permission);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    m_availableBufferIndexQLock.lock();
    m_availableBufferIndexQ.push_back(m_buffer[bufIndex].index);
    m_availableBufferIndexQLock.unlock();

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

/* User Process need to check the index of buffer returned from "getBuffer()" */
status_t ExynosCameraBufferManager::getBuffer(
        int  *reqBufIndex,
        enum EXYNOS_CAMERA_BUFFER_POSITION position,
        struct ExynosCameraBuffer *buffer)
{
    EXYNOS_CAMERA_BUFFER_IN();
    Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;
    List<int>::iterator r;

    int  bufferIndex;
    enum EXYNOS_CAMERA_BUFFER_PERMISSION permission;
    int  acquireFence = -1;
    int  releaseFence = -1;

    bufferIndex = *reqBufIndex;
    permission = EXYNOS_CAMERA_BUFFER_PERMISSION_NONE;

    if (m_allocatedBufCount == 0) {
        CLOGE("ERR(%s[%d]):m_allocatedBufCount equals zero", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_getBuffer(&bufferIndex, &acquireFence, &releaseFence) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_getBuffer failed [bufferIndex=%d, position=%d, permission=%d]",
            __FUNCTION__, __LINE__, bufferIndex, (int)position, (int)permission);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

reDo:

    if (bufferIndex < 0 || m_allocatedBufCount + m_indexOffset <= bufferIndex) {
        /* find availableBuffer */
        m_availableBufferIndexQLock.lock();
        if (m_availableBufferIndexQ.empty() == false) {
            r = m_availableBufferIndexQ.begin();
            bufferIndex = *r;
            m_availableBufferIndexQ.erase(r);
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
            CLOGI("INFO(%s[%d]):available buffer [index=%d]...",
                __FUNCTION__, __LINE__, bufferIndex);
#endif
        }
        m_availableBufferIndexQLock.unlock();
    } else {
        m_availableBufferIndexQLock.lock();
        /* get the Buffer of requested */
        for (r = m_availableBufferIndexQ.begin(); r != m_availableBufferIndexQ.end(); r++) {
            if (bufferIndex == *r) {
                m_availableBufferIndexQ.erase(r);
                break;
            }
        }
        m_availableBufferIndexQLock.unlock();
    }

    if (0 <= bufferIndex && bufferIndex < m_allocatedBufCount + m_indexOffset) {
        /* found buffer */
        if (isAvaliable(bufferIndex) == false) {
            CLOGE("ERR(%s[%d]):isAvaliable failed [bufferIndex=%d]",
                __FUNCTION__, __LINE__, bufferIndex);
            ret = BAD_VALUE;
            goto func_exit;
        }

        permission = EXYNOS_CAMERA_BUFFER_PERMISSION_IN_PROCESS;

        if (updateStatus(bufferIndex, 0, position, permission) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setStatus failed [bIndex=%d, position=%d, permission=%d]",
                __FUNCTION__, __LINE__, bufferIndex, (int)position, (int)permission);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    } else {
        /* do not find buffer */
        if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND) {
            /* increase buffer*/
            ret = m_increase(1);
            if (ret < 0) {
                CLOGE("ERR(%s[%d]):increase the buffer failed, m_allocatedBufCount %d, bufferIndex %d",
                    __FUNCTION__, __LINE__,  m_allocatedBufCount, bufferIndex);
            } else {
                m_availableBufferIndexQLock.lock();
                m_availableBufferIndexQ.push_back(m_allocatedBufCount + m_indexOffset);
                m_availableBufferIndexQLock.unlock();
                bufferIndex = m_allocatedBufCount + m_indexOffset;
                m_allocatedBufCount++;

                dumpBufferInfo();
                CLOGI("INFO(%s[%d]):increase the buffer succeeded (bufferIndex=%d)",
                    __FUNCTION__, __LINE__, bufferIndex);
                goto reDo;
            }
        } else {
            if (m_allocatedBufCount == 1)
                bufferIndex = 0;
            else
                ret = INVALID_OPERATION;
        }

        if (ret < 0) {
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
            CLOGD("DEBUG(%s[%d]):find free buffer... failed --- dump ---",
                __FUNCTION__, __LINE__);
            dump();
            CLOGD("DEBUG(%s[%d]):----------------------------------------",
                __FUNCTION__, __LINE__);
            CLOGD("DEBUG(%s[%d]):buffer Index in out of bound [bufferIndex=%d]",
                __FUNCTION__, __LINE__, bufferIndex);
#endif
            ret = BAD_VALUE;
            goto func_exit;
        }
    }

    m_buffer[bufferIndex].index = bufferIndex;

    m_buffer[bufferIndex].acquireFence = acquireFence;
    m_buffer[bufferIndex].releaseFence = releaseFence;

    *reqBufIndex = bufferIndex;
    *buffer      = m_buffer[bufferIndex];

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ExynosCameraBufferManager::increase(int increaseCount)
{
    CLOGD("DEBUG(%s[%d]):increaseCount(%d) function invalid. Do nothing", __FUNCTION__, __LINE__, increaseCount);
    return NO_ERROR;
}

#ifdef USE_GRALLOC_REUSE_SUPPORT
status_t ExynosCameraBufferManager::cancelBuffer(int bufIndex, bool isReuse)
#else
status_t ExynosCameraBufferManager::cancelBuffer(int bufIndex)
#endif
{
    int ret = NO_ERROR;
#ifdef USE_GRALLOC_REUSE_SUPPORT
    if (isReuse == true) {
        CLOGD("DEBUG(%s[%d]):cancelBuffer bufIndex(%d) isReuse(true) function invalid, put buffer", __FUNCTION__, __LINE__, bufIndex);
    }
#endif
    {
        ret = putBuffer(bufIndex, EXYNOS_CAMERA_BUFFER_POSITION_NONE);
    }
    return ret;
}

int ExynosCameraBufferManager::getBufStride(void)
{
    return 0;
}

status_t ExynosCameraBufferManager::updateStatus(
        int bufIndex,
        int driverValue,
        enum EXYNOS_CAMERA_BUFFER_POSITION   position,
        enum EXYNOS_CAMERA_BUFFER_PERMISSION permission)
{
    if (bufIndex < 0) {
        CLOGE("ERR(%s[%d]):Invalid buffer index %d",
                __FUNCTION__, __LINE__, bufIndex);
        return BAD_VALUE;
    }

    m_buffer[bufIndex].index = bufIndex;
    m_buffer[bufIndex].status.driverReturnValue = driverValue;
    m_buffer[bufIndex].status.position          = position;
    m_buffer[bufIndex].status.permission        = permission;

    return NO_ERROR;
}

status_t ExynosCameraBufferManager::getStatus(
        int bufIndex,
        struct ExynosCameraBufferStatus *bufStatus)
{
    *bufStatus = m_buffer[bufIndex].status;

    return NO_ERROR;
}

status_t ExynosCameraBufferManager::getIndexByHandle(__unused buffer_handle_t *handle, __unused int *index)
{
    return NO_ERROR;
}

status_t ExynosCameraBufferManager::getHandleByIndex(__unused buffer_handle_t **handle, __unused int index)
{
    return NO_ERROR;
}

sp<GraphicBuffer> ExynosCameraBufferManager::getGraphicBuffer(int index)
{
    EXYNOS_CAMERA_BUFFER_IN();

    sp<GraphicBuffer> graphicBuffer;
    int planeCount = 0;

    if ((index < 0) || (index >= m_allowedMaxBufCount)) {
        CLOGE("ERR(%s[%d]):Buffer index error (%d/%d)", __FUNCTION__, __LINE__, index, m_allowedMaxBufCount);
        goto done;
    }

    planeCount = m_buffer[index].planeCount;

    if (m_hasMetaPlane == true) {
        planeCount--;
    }

    graphicBuffer = m_graphicBufferAllocator.alloc(index, planeCount, m_buffer[index].fd, m_buffer[index].addr, m_buffer[index].size);
    if (graphicBuffer == 0) {
        CLOGE("ERR(%s[%d]):m_graphicBufferAllocator.alloc(%d) fail", __FUNCTION__, __LINE__, index);
        goto done;
    }

done:
    EXYNOS_CAMERA_BUFFER_OUT();

    return graphicBuffer;
}

bool ExynosCameraBufferManager::isAllocated(void)
{
    return m_flagAllocated;
}

bool ExynosCameraBufferManager::isAvaliable(int bufIndex)
{
    bool ret = false;

    switch (m_buffer[bufIndex].status.permission) {
    case EXYNOS_CAMERA_BUFFER_PERMISSION_NONE:
    case EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE:
        ret = true;
        break;

    case EXYNOS_CAMERA_BUFFER_PERMISSION_IN_PROCESS:
    default:
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
        CLOGD("DEBUG(%s[%d]):buffer is not available", __FUNCTION__, __LINE__);
        dump();
#endif
        ret = false;
        break;
    }

    return ret;
}

status_t ExynosCameraBufferManager::m_setDefaultAllocator(void *allocator)
{
    m_defaultAllocator = (ExynosCameraIonAllocator *)allocator;

    return NO_ERROR;
}

status_t ExynosCameraBufferManager::m_defaultAlloc(int bIndex, int eIndex, bool isMetaPlane)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;
    int planeIndexStart = 0;
    int planeIndexEnd   = 0;
    bool mapNeeded      = false;
#ifdef DEBUG_RAWDUMP
    char enableRawDump[PROP_VALUE_MAX];
#endif /* DEBUG_RAWDUMP */

    int mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_NONCACHED;
    int flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_NONCACHED;

    ExynosCameraDurationTimer m_timer;
    long long    durationTime = 0;
    long long    durationTimeSum = 0;
    unsigned int estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_NONCACHED;
    unsigned int estimatedTime = 0;
    unsigned int bufferSize = 0;
    int          reservedMaxCount = 0;
    int          bufIndex = 0;
    if (m_defaultAllocator == NULL) {
        CLOGE("ERR(%s[%d]):m_defaultAllocator equals NULL", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (bIndex < 0 || eIndex < 0) {
        CLOGE("ERR(%s[%d]):Invalid index parameters. bIndex %d eIndex %d",
                __FUNCTION__, __LINE__,
                bIndex, eIndex);
        ret = BAD_VALUE;
        goto func_exit;
    }

    if (isMetaPlane == true) {
        mapNeeded = true;
    } else {
#ifdef DEBUG_RAWDUMP
        mapNeeded = true;
#else
        mapNeeded = m_flagNeedMmap;
#endif
    }

    for (bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        if (isMetaPlane == true) {
            planeIndexStart = m_buffer[bufIndex].planeCount-1;
            planeIndexEnd   = m_buffer[bufIndex].planeCount;
            mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_NONCACHED;
            flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_NONCACHED;
            estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_NONCACHED;
        } else {
            planeIndexStart = 0;
            planeIndexEnd   = (m_hasMetaPlane ?
                m_buffer[bufIndex].planeCount-1 : m_buffer[bufIndex].planeCount);
            switch (m_buffer[bufIndex].type) {
            case EXYNOS_CAMERA_BUFFER_ION_NONCACHED_TYPE:
                mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_NONCACHED;
                flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_NONCACHED;
                estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_NONCACHED;
                break;
            case EXYNOS_CAMERA_BUFFER_ION_CACHED_TYPE:
                mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_CACHED;
                flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED;
                estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_CACHED;
                break;
            case EXYNOS_CAMERA_BUFFER_ION_RESERVED_TYPE:
            /* case EXYNOS_CAMERA_BUFFER_ION_NONCACHED_RESERVED_TYPE: */
#ifdef RESERVED_MEMORY_ENABLE
                reservedMaxCount = (m_reservedMemoryCount > 0 ? m_reservedMemoryCount : RESERVED_BUFFER_COUNT_MAX);
#else
                reservedMaxCount = 0;
#endif
                if (bufIndex < reservedMaxCount) {
                    CLOGI("INFO(%s[%d]):bufIndex(%d) < reservedMaxCount(%d) , m_reservedMemoryCount(%d), non-cached",
                        __FUNCTION__, __LINE__, bufIndex, reservedMaxCount, m_reservedMemoryCount);
                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_RESERVED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_RESERVED;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_RESERVED;
                } else {
                    CLOGI("INFO(%s[%d]):bufIndex(%d) >= reservedMaxCount(%d) , m_reservedMemoryCount(%d), non-cached. so, alloc ion memory instead of reserved memory",
                        __FUNCTION__, __LINE__, bufIndex, reservedMaxCount, m_reservedMemoryCount);
                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_NONCACHED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_NONCACHED;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_NONCACHED;
                }
                break;
            case EXYNOS_CAMERA_BUFFER_ION_CACHED_RESERVED_TYPE:
#ifdef RESERVED_MEMORY_ENABLE
                reservedMaxCount = (m_reservedMemoryCount > 0 ? m_reservedMemoryCount : RESERVED_BUFFER_COUNT_MAX);
#else
                reservedMaxCount = 0;
#endif
                if (bufIndex < reservedMaxCount) {
                    CLOGI("INFO(%s[%d]):bufIndex(%d) < reservedMaxCount(%d) , m_reservedMemoryCount(%d), cached",
                        __FUNCTION__, __LINE__, bufIndex, reservedMaxCount, m_reservedMemoryCount);

                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_RESERVED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_RESERVED | EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_RESERVED;
                } else {
                    CLOGI("INFO(%s[%d]):bufIndex(%d) >= reservedMaxCount(%d) , m_reservedMemoryCount(%d), cached. so, alloc ion memory instead of reserved memory",
                        __FUNCTION__, __LINE__, bufIndex, reservedMaxCount, m_reservedMemoryCount);

                    mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_CACHED;
                    flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED;
                    estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_CACHED;
                }
                break;
            case EXYNOS_CAMERA_BUFFER_ION_CACHED_SYNC_FORCE_TYPE:
                ALOGD("DEBUG(%s[%d]):SYNC_FORCE_CACHED", __FUNCTION__, __LINE__);
                mask  = EXYNOS_CAMERA_BUFFER_ION_MASK_CACHED;
                flags = EXYNOS_CAMERA_BUFFER_ION_FLAG_CACHED_SYNC_FORCE;
                estimatedBase = EXYNOS_CAMERA_BUFFER_ION_WARNING_TIME_CACHED;
                break;
            case EXYNOS_CAMERA_BUFFER_INVALID_TYPE:
            default:
                CLOGE("ERR(%s[%d]):buffer type is invaild (%d)", __FUNCTION__, __LINE__, (int)m_buffer[bufIndex].type);
                break;
            }
        }

        if (isMetaPlane == false) {
            m_timer.start();
            bufferSize = 0;
        }

        for (int planeIndex = planeIndexStart; planeIndex < planeIndexEnd; planeIndex++) {
            if (m_buffer[bufIndex].fd[planeIndex] >= 0) {
                CLOGE("ERR(%s[%d]):buffer[%d].fd[%d] = %d already allocated",
                        __FUNCTION__, __LINE__, bufIndex, planeIndex, m_buffer[bufIndex].fd[planeIndex]);
                continue;
            }

            if (m_defaultAllocator->alloc(
                    m_buffer[bufIndex].size[planeIndex],
                    &(m_buffer[bufIndex].fd[planeIndex]),
                    &(m_buffer[bufIndex].addr[planeIndex]),
                    mask,
                    flags,
                    mapNeeded) != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_defaultAllocator->alloc(bufIndex=%d, planeIndex=%d, planeIndex=%d) failed",
                    __FUNCTION__, __LINE__, bufIndex, planeIndex, m_buffer[bufIndex].size[planeIndex]);
                ret = INVALID_OPERATION;
                goto func_exit;
            }
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
            printBufferInfo(__FUNCTION__, __LINE__, bufIndex, planeIndex);
#endif
            if (isMetaPlane == false)
                bufferSize = bufferSize + m_buffer[bufIndex].size[planeIndex];
        }
        if (isMetaPlane == false) {
            m_timer.stop();
            durationTime = m_timer.durationMsecs();
            durationTimeSum += durationTime;
            CLOGD("DEBUG(%s[%d]):duration time(%5d msec):(type=%d, bufIndex=%d, size=%.2f)",
                __FUNCTION__, __LINE__, (int)durationTime, m_buffer[bufIndex].type, bufIndex, (float)bufferSize / (float)(1024 * 1024));

            estimatedTime = estimatedBase * bufferSize / EXYNOS_CAMERA_BUFFER_1MB;
            if (estimatedTime < durationTime) {
                CLOGW("WARN(%s[%d]):estimated time(%5d msec):(type=%d, bufIndex=%d, size=%d)",
                    __FUNCTION__, __LINE__, (int)estimatedTime, m_buffer[bufIndex].type, bufIndex, (int)bufferSize);
            }
        }

        if (updateStatus(
                bufIndex,
                0,
                EXYNOS_CAMERA_BUFFER_POSITION_NONE,
                EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setStatus failed [bIndex=%d, position=NONE, permission=NONE]",
                __FUNCTION__, __LINE__, bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }
    CLOGD("DEBUG(%s[%d]):Duration time of buffer allocation(%5d msec)", __FUNCTION__, __LINE__, (int)durationTimeSum);

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;

func_exit:
    EXYNOS_CAMERA_BUFFER_OUT();

    if (bufIndex < eIndex) {
        if (m_defaultFree(0, bufIndex, isMetaPlane) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_defaultFree failed", __FUNCTION__, __LINE__);
        }
    }
    return ret;
}

status_t ExynosCameraBufferManager::m_defaultFree(int bIndex, int eIndex, bool isMetaPlane)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;
    int planeIndexStart = 0;
    int planeIndexEnd   = 0;
    bool mapNeeded      = false;
#ifdef DEBUG_RAWDUMP
    char enableRawDump[PROP_VALUE_MAX];
#endif /* DEBUG_RAWDUMP */

    if (isMetaPlane == true) {
        mapNeeded = true;
    } else {
#ifdef DEBUG_RAWDUMP
        mapNeeded = true;
#else
        mapNeeded = m_flagNeedMmap;
#endif
    }

    for (int bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        if (isAvaliable(bufIndex) == false) {
            CLOGE("ERR(%s[%d]):buffer [bufIndex=%d] in InProcess state",
                __FUNCTION__, __LINE__, bufIndex);
            if (m_isDestructor == false) {
                ret = BAD_VALUE;
                continue;
            } else {
                CLOGE("ERR(%s[%d]):buffer [bufIndex=%d] in InProcess state, but try to forcedly free",
                    __FUNCTION__, __LINE__, bufIndex);
            }
        }

        if (isMetaPlane == true) {
            planeIndexStart = m_buffer[bufIndex].planeCount-1;
            planeIndexEnd   = m_buffer[bufIndex].planeCount;
        } else {
            planeIndexStart = 0;
            planeIndexEnd   = (m_hasMetaPlane ?
                m_buffer[bufIndex].planeCount-1 : m_buffer[bufIndex].planeCount);
        }

        for (int planeIndex = planeIndexStart; planeIndex < planeIndexEnd; planeIndex++) {
            if (m_defaultAllocator->free(
                    m_buffer[bufIndex].size[planeIndex],
                    &(m_buffer[bufIndex].fd[planeIndex]),
                    &(m_buffer[bufIndex].addr[planeIndex]),
                    mapNeeded) != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_defaultAllocator->free for Imagedata Plane failed",
                    __FUNCTION__, __LINE__);
                ret = INVALID_OPERATION;
                goto func_exit;
            }
        }

        if (updateStatus(
                bufIndex,
                0,
                EXYNOS_CAMERA_BUFFER_POSITION_NONE,
                EXYNOS_CAMERA_BUFFER_PERMISSION_NONE) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setStatus failed [bIndex=%d, position=NONE, permission=NONE]",
                __FUNCTION__, __LINE__, bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }

        if (m_graphicBufferAllocator.free(bufIndex) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_graphicBufferAllocator.free(%d) fail",
                __FUNCTION__, __LINE__, bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

bool ExynosCameraBufferManager::m_checkInfoForAlloc(void)
{
    EXYNOS_CAMERA_BUFFER_IN();

    bool ret = true;

    if (m_reqBufCount < 0 || VIDEO_MAX_FRAME < m_reqBufCount) {
        CLOGE("ERR(%s[%d]):buffer Count in out of bound [m_reqBufCount=%d]",
            __FUNCTION__, __LINE__, m_reqBufCount);
        ret = false;
        goto func_exit;
    }

    for (int bufIndex = m_indexOffset; bufIndex < m_reqBufCount + m_indexOffset; bufIndex++) {
        if (m_buffer[bufIndex].planeCount < 0
         || VIDEO_MAX_PLANES <= m_buffer[bufIndex].planeCount) {
            CLOGE("ERR(%s[%d]):plane Count in out of bound [m_buffer[bIndex].planeCount=%d]",
                __FUNCTION__, __LINE__, m_buffer[bufIndex].planeCount);
            ret = false;
            goto func_exit;
        }

        for (int planeIndex = 0; planeIndex < m_buffer[bufIndex].planeCount; planeIndex++) {
            if (m_buffer[bufIndex].size[planeIndex] == 0) {
                CLOGE("ERR(%s[%d]):size is empty [m_buffer[%d].size[%d]=%d]",
                    __FUNCTION__, __LINE__, bufIndex, planeIndex, m_buffer[bufIndex].size[planeIndex]);
                ret = false;
                goto func_exit;
            }
        }
    }

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ExynosCameraBufferManager::m_createDefaultAllocator(bool isCached)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;

    m_defaultAllocator = new ExynosCameraIonAllocator();
    m_isCreateDefaultAllocator = true;
    if (m_defaultAllocator->init(isCached) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_defaultAllocator->init failed", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

int ExynosCameraBufferManager::getAllocatedBufferCount(void)
{
    return m_allocatedBufCount;
}

int ExynosCameraBufferManager::getAvailableIncreaseBufferCount(void)
{
    CLOGI("INFO(%s[%d]):this function only applied to ONDEMAND mode (%d)", __FUNCTION__, __LINE__, m_allocMode);
    int numAvailable = 0;

    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND)
        numAvailable += (m_allowedMaxBufCount - m_allocatedBufCount);

    CLOGI("INFO(%s[%d]):m_allowedMaxBufCount(%d), m_allocatedBufCount(%d), ret(%d)",
        __FUNCTION__, __LINE__, m_allowedMaxBufCount, m_allocatedBufCount, numAvailable);
    return numAvailable;
}

int ExynosCameraBufferManager::getNumOfAvailableBuffer(void)
{
    int numAvailable = 0;

    for (int i = m_indexOffset; i < m_allocatedBufCount + m_indexOffset; i++) {
        if (m_buffer[i].status.permission == EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE)
            numAvailable++;
    }

    if (m_allocMode == BUFFER_MANAGER_ALLOCATION_ONDEMAND)
        numAvailable += (m_allowedMaxBufCount - m_allocatedBufCount);

    return numAvailable;
}

int ExynosCameraBufferManager::getNumOfAvailableAndNoneBuffer(void)
{
    int numAvailable = 0;

    for (int i = m_indexOffset; i < m_allocatedBufCount + m_indexOffset; i++) {
        if (m_buffer[i].status.permission == EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE ||
            m_buffer[i].status.permission == EXYNOS_CAMERA_BUFFER_PERMISSION_NONE)
            numAvailable++;
    }

    return numAvailable;
}

void ExynosCameraBufferManager::printBufferState(void)
{
    for (int i = m_indexOffset; i < m_allocatedBufCount + m_indexOffset; i++) {
        CLOGI("INFO(%s[%d]):m_buffer[%d].fd[0]=%d, position=%d, permission=%d]",
            __FUNCTION__, __LINE__, i, m_buffer[i].fd[0],
            m_buffer[i].status.position, m_buffer[i].status.permission);
    }

    return;
}

void ExynosCameraBufferManager::printBufferState(int bufIndex, int planeIndex)
{
    CLOGI("INFO(%s[%d]):m_buffer[%d].fd[%d]=%d, .status.permission=%d]",
        __FUNCTION__, __LINE__, bufIndex, planeIndex, m_buffer[bufIndex].fd[planeIndex],
        m_buffer[bufIndex].status.permission);

    return;
}

void ExynosCameraBufferManager::printBufferQState()
{
    List<int>::iterator r;
    int  bufferIndex;

    Mutex::Autolock lock(m_availableBufferIndexQLock);

    for (r = m_availableBufferIndexQ.begin(); r != m_availableBufferIndexQ.end(); r++) {
        bufferIndex = *r;
        CLOGV("DEBUG(%s[%d]):bufferIndex=%d", __FUNCTION__, __LINE__, bufferIndex);
    }

    return;
}

void ExynosCameraBufferManager::printBufferInfo(
        const char *funcName,
        const int lineNum,
        int bufIndex,
        int planeIndex)
{
    CLOGI("INFO(%s[%d]):[m_buffer[%d].fd[%d]=%d] .addr=%p .size=%d]",
        funcName, lineNum, bufIndex, planeIndex,
        m_buffer[bufIndex].fd[planeIndex],
        m_buffer[bufIndex].addr[planeIndex],
        m_buffer[bufIndex].size[planeIndex]);

    return;
}

void ExynosCameraBufferManager::dump(void)
{
    printBufferState();
    printBufferQState();

    return;
}

void ExynosCameraBufferManager::dumpBufferInfo(void)
{
    for (int bufIndex = m_indexOffset; bufIndex < m_allocatedBufCount + m_indexOffset; bufIndex++)
        for (int planeIndex = 0; planeIndex < m_buffer[bufIndex].planeCount; planeIndex++) {
            CLOGI("INFO(%s[%d]):[m_buffer[%d].fd[%d]=%d] .addr=%p .size=%d .position=%d .permission=%d]",
                __FUNCTION__, __LINE__, m_buffer[bufIndex].index, planeIndex,
                m_buffer[bufIndex].fd[planeIndex],
                m_buffer[bufIndex].addr[planeIndex],
                m_buffer[bufIndex].size[planeIndex],
                m_buffer[bufIndex].status.position,
                m_buffer[bufIndex].status.permission);
    }
    printBufferQState();

    return;
}

status_t ExynosCameraBufferManager::setBufferCount(__unused int bufferCount)
{
    CLOGD("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    return NO_ERROR;
}

int ExynosCameraBufferManager::getBufferCount(void)
{
    CLOGD("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    return 0;
}

InternalExynosCameraBufferManager::InternalExynosCameraBufferManager()
{
    ExynosCameraBufferManager::init();
}

InternalExynosCameraBufferManager::~InternalExynosCameraBufferManager()
{
    ExynosCameraBufferManager::deinit();
}

status_t InternalExynosCameraBufferManager::m_setAllocator(void *allocator)
{
    return m_setDefaultAllocator(allocator);
}

status_t InternalExynosCameraBufferManager::m_alloc(int bIndex, int eIndex)
{
    return m_defaultAlloc(bIndex, eIndex, false);
}

status_t InternalExynosCameraBufferManager::m_free(int bIndex, int eIndex)
{
    return m_defaultFree(bIndex, eIndex, false);
}

status_t InternalExynosCameraBufferManager::m_increase(int increaseCount)
{
    CLOGD("DEBUG(%s[%d]):IN.." , __FUNCTION__, __LINE__);
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = NO_ERROR;

    if (m_allowedMaxBufCount <= m_allocatedBufCount) {
        CLOGD("DEBUG(%s[%d]):BufferManager can't increase the buffer "
              "(m_reqBufCount=%d, m_allowedMaxBufCount=%d <= m_allocatedBufCount=%d)",
            __FUNCTION__, __LINE__,
            m_reqBufCount, m_allowedMaxBufCount, m_allocatedBufCount);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_allowedMaxBufCount < m_allocatedBufCount + increaseCount) {
        CLOGI("INFO(%s[%d]):change the increaseCount (%d->%d) --- "
              "(m_reqBufCount=%d, m_allowedMaxBufCount=%d <= m_allocatedBufCount=%d + increaseCount=%d)",
            __FUNCTION__, __LINE__, increaseCount, m_allowedMaxBufCount - m_allocatedBufCount,
            m_reqBufCount, m_allowedMaxBufCount, m_allocatedBufCount, increaseCount);
        increaseCount = m_allowedMaxBufCount - m_allocatedBufCount;
    }

    /* set the buffer information */
    for (int bufIndex = m_allocatedBufCount + m_indexOffset; bufIndex < m_allocatedBufCount + m_indexOffset + increaseCount; bufIndex++) {
        for (int planeIndex = 0; planeIndex < m_buffer[0].planeCount; planeIndex++) {
            if (m_buffer[0].size[planeIndex] == 0) {
                CLOGE("ERR(%s[%d]):abnormal value [size=%d]",
                    __FUNCTION__, __LINE__, m_buffer[0].size[planeIndex]);
                ret = BAD_VALUE;
                goto func_exit;
            }
            m_buffer[bufIndex].size[planeIndex]         = m_buffer[0].size[planeIndex];
            m_buffer[bufIndex].bytesPerLine[planeIndex] = m_buffer[0].bytesPerLine[planeIndex];
        }
        m_buffer[bufIndex].planeCount = m_buffer[0].planeCount;
        m_buffer[bufIndex].type       = m_buffer[0].type;
    }

    if (m_alloc(m_allocatedBufCount + m_indexOffset, m_allocatedBufCount + m_indexOffset + increaseCount) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_alloc failed", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_hasMetaPlane == true) {
        if (m_defaultAlloc(m_allocatedBufCount + m_indexOffset, m_allocatedBufCount + m_indexOffset + increaseCount, m_hasMetaPlane) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_defaultAlloc failed", __FUNCTION__, __LINE__);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

    CLOGD("DEBUG(%s[%d]):Increase the buffer succeeded (m_allocatedBufCount=%d, increaseCount=%d)",
        __FUNCTION__, __LINE__, m_allocatedBufCount + m_indexOffset, increaseCount);

func_exit:

    CLOGD("DEBUG(%s[%d]):OUT.." , __FUNCTION__, __LINE__);

    return ret;
}

status_t InternalExynosCameraBufferManager::m_decrease(void)
{
    CLOGD("DEBUG(%s[%d]):IN.." , __FUNCTION__, __LINE__);
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);

    status_t ret = true;
    List<int>::iterator r;

    int  bufferIndex = -1;

    if (m_allocatedBufCount <= m_reqBufCount) {
        CLOGD("DEBUG(%s[%d]):BufferManager can't decrease the buffer "
              "(m_allowedMaxBufCount=%d, m_allocatedBufCount=%d <= m_reqBufCount=%d)",
            __FUNCTION__, __LINE__,
            m_allowedMaxBufCount, m_allocatedBufCount, m_reqBufCount);
        ret = INVALID_OPERATION;
        goto func_exit;
    }
    bufferIndex = m_allocatedBufCount;

    if (m_free(bufferIndex-1 + m_indexOffset, bufferIndex + m_indexOffset) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_free failed", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_hasMetaPlane == true) {
        if (m_defaultFree(bufferIndex-1 + m_indexOffset, bufferIndex + m_indexOffset, m_hasMetaPlane) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_defaultFree failed", __FUNCTION__, __LINE__);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

    m_availableBufferIndexQLock.lock();
    for (r = m_availableBufferIndexQ.begin(); r != m_availableBufferIndexQ.end(); r++) {
        if ((bufferIndex + m_indexOffset) == *r) {
            m_availableBufferIndexQ.erase(r);
            break;
        }
    }
    m_availableBufferIndexQLock.unlock();
    m_allocatedBufCount--;

    CLOGD("DEBUG(%s[%d]):Decrease the buffer succeeded (m_allocatedBufCount=%d)" ,
        __FUNCTION__, __LINE__, m_allocatedBufCount);

func_exit:

    CLOGD("DEBUG(%s[%d]):OUT.." , __FUNCTION__, __LINE__);

    return ret;
}

status_t InternalExynosCameraBufferManager::m_putBuffer(__unused int bufIndex)
{
    return NO_ERROR;
}

status_t InternalExynosCameraBufferManager::m_getBuffer(__unused int *bufIndex, __unused int *acquireFence, __unused int *releaseFence)
{
    return NO_ERROR;
}

status_t InternalExynosCameraBufferManager::increase(int increaseCount)
{
    ExynosCameraAutoTimer autoTimer(__FUNCTION__);
    Mutex::Autolock lock(m_lock);
    status_t ret = NO_ERROR;

    CLOGI("INFO(%s[%d]):m_allocatedBufCount(%d), m_allowedMaxBufCount(%d), increaseCount(%d)",
        __FUNCTION__, __LINE__, m_allocatedBufCount, m_allowedMaxBufCount, increaseCount);

    /* increase buffer*/
    ret = m_increase(increaseCount);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):increase the buffer failed, m_allocatedBufCount(%d), m_allowedMaxBufCount(%d), increaseCount(%d)",
            __FUNCTION__, __LINE__,  m_allocatedBufCount, m_allowedMaxBufCount, increaseCount);
    } else {
        for (int bufferIndex = m_allocatedBufCount + m_indexOffset; bufferIndex < m_allocatedBufCount + m_indexOffset + increaseCount; bufferIndex++) {
            m_availableBufferIndexQLock.lock();
            m_availableBufferIndexQ.push_back(bufferIndex);
            m_availableBufferIndexQLock.unlock();
        }
        m_allocatedBufCount += increaseCount;

        dumpBufferInfo();
        CLOGI("INFO(%s[%d]):increase the buffer succeeded (increaseCount(%d))",
            __FUNCTION__, __LINE__, increaseCount);
    }

func_exit:

    return ret;
}

MHBExynosCameraBufferManager::MHBExynosCameraBufferManager()
{
    ExynosCameraBufferManager::init();

    m_allocator   = NULL;
    m_numBufsHeap = 1;

    for (int bufIndex = m_indexOffset; bufIndex < VIDEO_MAX_FRAME; bufIndex++) {
        for (int planeIndex = 0; planeIndex < EXYNOS_CAMERA_BUFFER_MAX_PLANES; planeIndex++) {
            m_heap[bufIndex][planeIndex] = NULL;
        }
    }
}

MHBExynosCameraBufferManager::~MHBExynosCameraBufferManager()
{
    ExynosCameraBufferManager::deinit();
}

status_t MHBExynosCameraBufferManager::m_setAllocator(void *allocator)
{
    m_allocator = (ExynosCameraMHBAllocator *)allocator;

    return NO_ERROR;
}

status_t MHBExynosCameraBufferManager::m_alloc(int bIndex, int eIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    int planeCount = 0;

    if (m_allocator == NULL) {
        CLOGE("ERR(%s[%d]):m_allocator equals NULL", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    for (int bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        planeCount = (m_hasMetaPlane ?
            m_buffer[bufIndex].planeCount-1 : m_buffer[bufIndex].planeCount);

        for (int planeIndex = 0; planeIndex < planeCount; planeIndex++) {
            if (m_allocator->alloc(
                    m_buffer[bufIndex].size[planeIndex],
                    &(m_buffer[bufIndex].fd[planeIndex]),
                    &(m_buffer[bufIndex].addr[planeIndex]),
                    m_numBufsHeap,
                    &(m_heap[bufIndex][planeIndex])) != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_allocator->alloc(bufIndex=%d, planeIndex=%d, planeIndex=%d) failed",
                    __FUNCTION__, __LINE__, bufIndex, planeIndex, m_buffer[bufIndex].size[planeIndex]);
                return INVALID_OPERATION;
            }
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
            printBufferInfo(__FUNCTION__, __LINE__, bufIndex, planeIndex);
            CLOGI("INFO(%s[%d]):[m_buffer[%d][%d].heap=%p]",
                __FUNCTION__, __LINE__, bufIndex, planeIndex, m_heap[bufIndex][planeIndex]);
#endif
        }

        if (updateStatus(
                bufIndex,
                0,
                EXYNOS_CAMERA_BUFFER_POSITION_NONE,
                EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setStatus failed [bIndex=%d, position=NONE, permission=NONE]",
                __FUNCTION__, __LINE__, bufIndex);
            return INVALID_OPERATION;
        }
    }

    EXYNOS_CAMERA_BUFFER_OUT();

    return NO_ERROR;
}

status_t MHBExynosCameraBufferManager::m_free(int bIndex, int eIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;
    int planeCount = 0;

    for (int bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        if (isAvaliable(bufIndex) == false) {
            CLOGE("ERR(%s[%d]):buffer [bufIndex=%d] in InProcess state",
                __FUNCTION__, __LINE__, bufIndex);
            if (m_isDestructor == false) {
                ret = BAD_VALUE;
                continue;
            } else {
                CLOGE("ERR(%s[%d]):buffer [bufIndex=%d] in InProcess state, but try to forcedly free",
                    __FUNCTION__, __LINE__, bufIndex);
            }
        }

        planeCount = (m_hasMetaPlane ?
            m_buffer[bufIndex].planeCount-1 : m_buffer[bufIndex].planeCount);

        for (int planeIndex = 0; planeIndex < planeCount; planeIndex++) {
            if (m_allocator->free(
                    m_buffer[bufIndex].size[planeIndex],
                    &(m_buffer[bufIndex].fd[planeIndex]),
                    &(m_buffer[bufIndex].addr[planeIndex]),
                    &(m_heap[bufIndex][planeIndex])) != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_defaultAllocator->free for Imagedata Plane failed",
                    __FUNCTION__, __LINE__);
                ret = INVALID_OPERATION;
                goto func_exit;
            }
            m_heap[bufIndex][planeIndex] = 0;
        }

        if (updateStatus(
                bufIndex,
                0,
                EXYNOS_CAMERA_BUFFER_POSITION_NONE,
                EXYNOS_CAMERA_BUFFER_PERMISSION_NONE) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setStatus failed [bIndex=%d, position=NONE, permission=NONE]",
                __FUNCTION__, __LINE__, bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }

        if (m_graphicBufferAllocator.free(bufIndex) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_graphicBufferAllocator.free(%d) fail",
                __FUNCTION__, __LINE__, bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t MHBExynosCameraBufferManager::m_increase(__unused int increaseCount)
{
    CLOGD("DEBUG(%s[%d]):allocMode(%d) is invalid. Do nothing", __FUNCTION__, __LINE__, m_allocMode);
    return INVALID_OPERATION;
}

status_t MHBExynosCameraBufferManager::m_decrease(void)
{
    return INVALID_OPERATION;
}

status_t MHBExynosCameraBufferManager::m_putBuffer(__unused int bufIndex)
{
    return NO_ERROR;
}

status_t MHBExynosCameraBufferManager::m_getBuffer(__unused int *bufIndex, __unused int *acquireFence, __unused int *releaseFence)
{
    return NO_ERROR;
}

status_t MHBExynosCameraBufferManager::allocMulti()
{
    m_numBufsHeap = m_reqBufCount;
    m_reqBufCount = 1;

    return alloc();
}

status_t MHBExynosCameraBufferManager::getHeapMemory(
        int bufIndex,
        int planeIndex,
        camera_memory_t **heap)
{
    EXYNOS_CAMERA_BUFFER_IN();

    if (m_buffer[bufIndex].status.position != EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL) {
        CLOGE("ERR(%s[%d]):buffer position not in IN_HAL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    if (m_heap[bufIndex][planeIndex] == NULL) {
        CLOGE("ERR(%s[%d]):m_heap equals NULL", __FUNCTION__, __LINE__);
        return BAD_VALUE;
    }

    *heap = m_heap[bufIndex][planeIndex];
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
    CLOGI("INFO(%s[%d]):heap=%p", __FUNCTION__, __LINE__, *heap);
#endif

    EXYNOS_CAMERA_BUFFER_OUT();

    return NO_ERROR;
}

GrallocExynosCameraBufferManager::GrallocExynosCameraBufferManager()
{
    ExynosCameraBufferManager::init();

    m_allocator             = NULL;
    m_dequeuedBufCount      = 0;
    m_minUndequeuedBufCount = 0;
    m_bufStride             = 0;
    m_bufferCount = 0;

#ifdef USE_GRALLOC_BUFFER_COLLECTOR
    m_collectedBufferCount = 0;

    m_stopBufferCollector = false;
    m_bufferCollector = new grallocBufferThread(this, &GrallocExynosCameraBufferManager::m_bufferCollectorThreadFunc, "GrallocBufferCollector", PRIORITY_DEFAULT);
#endif

    for (int bufIndex = 0; bufIndex < VIDEO_MAX_FRAME; bufIndex++) {
        m_handle[bufIndex]         = NULL;
        m_handleIsLocked[bufIndex] = false;
    }
}

GrallocExynosCameraBufferManager::~GrallocExynosCameraBufferManager()
{
    ExynosCameraBufferManager::deinit();
}

status_t GrallocExynosCameraBufferManager::m_setAllocator(void *allocator)
{
    m_allocator = (ExynosCameraGrallocAllocator *)allocator;

    return NO_ERROR;
}

status_t GrallocExynosCameraBufferManager::m_alloc(int bIndex, int eIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;

    ExynosCameraDurationTimer m_timer;
    long long    durationTime = 0;
    long long    durationTimeSum = 0;
    unsigned int estimatedBase = EXYNOS_CAMERA_BUFFER_GRALLOC_WARNING_TIME;
    unsigned int estimatedTime = 0;
    unsigned int bufferSize = 0;
    int planeIndexEnd   = 0;

    if (m_allocator == NULL) {
        CLOGE("ERR(%s[%d]):m_allocator equals NULL", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (getBufferCount() == 0) {
        CLOGE("ERR(%s[%d]):m_reqBufCount(%d)", __FUNCTION__, __LINE__, m_reqBufCount);
        setBufferCount(m_reqBufCount);
    }

    m_minUndequeuedBufCount = m_allocator->getMinUndequeueBuffer();
    if (m_minUndequeuedBufCount < 0 ) {
        CLOGE("ERR(%s[%d]):m_minUndequeuedBufCount=%d..",
            __FUNCTION__, __LINE__, m_minUndequeuedBufCount);
        ret = INVALID_OPERATION;
        goto func_exit;
    }
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
    CLOGI("INFO(%s[%d]):before dequeueBuffer m_reqBufCount=%d, m_minUndequeuedBufCount=%d",
        __FUNCTION__, __LINE__, m_reqBufCount, m_minUndequeuedBufCount);
#endif
    for (int bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        m_timer.start();
        if (m_allocator->alloc(
                &m_handle[bufIndex],
                m_buffer[bufIndex].fd,
                m_buffer[bufIndex].addr,
                &m_bufStride,
                &m_handleIsLocked[bufIndex]) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):alloc failed [bufIndex=%d]", __FUNCTION__, __LINE__, bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }

        planeIndexEnd = m_buffer[bufIndex].planeCount;

        if (m_hasMetaPlane == true)
            planeIndexEnd--;

        bufferSize = 0;
        for (int planeIndex = 0; planeIndex < planeIndexEnd; planeIndex++)
            bufferSize = bufferSize + m_buffer[bufIndex].size[planeIndex];

        m_timer.stop();
        durationTime = m_timer.durationMsecs();
        durationTimeSum += durationTime;
        CLOGD("DEBUG(%s[%d]):duration time(%5d msec):(type=%d, bufIndex=%d, size=%.2f)",
            __FUNCTION__, __LINE__, (int)durationTime, m_buffer[bufIndex].type, bufIndex, (float)bufferSize / (float)(1024 * 1024));

        estimatedTime = estimatedBase * bufferSize / EXYNOS_CAMERA_BUFFER_1MB;
        if (estimatedTime < durationTime) {
            CLOGW("WARN(%s[%d]):estimated time(%5d msec):(type=%d, bufIndex=%d, size=%d)",
                __FUNCTION__, __LINE__, (int)estimatedTime, m_buffer[bufIndex].type, bufIndex, (int)bufferSize);
        }

#ifdef EXYNOS_CAMERA_BUFFER_TRACE
        CLOGD("DEBUG(%s[%d]):-- dump buffer status --", __FUNCTION__, __LINE__);
        dump();
#endif
        m_dequeuedBufCount++;

        if (updateStatus(
                bufIndex,
                0,
                EXYNOS_CAMERA_BUFFER_POSITION_NONE,
                EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setStatus failed [bufIndex=%d, position=NONE, permission=NONE]",
                __FUNCTION__, __LINE__, bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }
    CLOGD("DEBUG(%s[%d]):Duration time of buffer allocation(%5d msec)", __FUNCTION__, __LINE__, (int)durationTimeSum);

    for (int bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
#ifdef USE_GRALLOC_REUSE_SUPPORT
        m_cancelReuseBuffer(bufIndex, true);
#else
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
        CLOGD("DEBUG(%s[%d]):-- dump buffer status --", __FUNCTION__, __LINE__);
        dump();
#endif
        if (m_allocator->cancelBuffer(m_handle[bufIndex]) != 0) {
            CLOGE("ERR(%s[%d]):could not free [bufIndex=%d]", __FUNCTION__, __LINE__, bufIndex);
            goto func_exit;
        }
        m_dequeuedBufCount--;
        m_handleIsLocked[bufIndex] = false;

        if (updateStatus(
                bufIndex,
                0,
                EXYNOS_CAMERA_BUFFER_POSITION_NONE,
                EXYNOS_CAMERA_BUFFER_PERMISSION_NONE) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setStatus failed [bufIndex=%d, position=NONE, permission=NONE]",
                __FUNCTION__, __LINE__, bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
#endif
    }

#ifdef USE_GRALLOC_BUFFER_COLLECTOR
    m_bufferCollector->run();
#endif
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
    CLOGI("INFO(%s[%d]):before exit m_alloc m_dequeuedBufCount=%d, m_minUndequeuedBufCount=%d",
        __FUNCTION__, __LINE__, m_dequeuedBufCount, m_minUndequeuedBufCount);
#endif

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t GrallocExynosCameraBufferManager::m_free(int bIndex, int eIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;

    CLOGD("DEBUG(%s[%d]):IN -- dump buffer status --", __FUNCTION__, __LINE__);
    dump();

    for (int bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        if (m_handleIsLocked[bufIndex] == false) {
            CLOGD("DEBUG(%s[%d]):buffer [bufIndex=%d] already free", __FUNCTION__, __LINE__, bufIndex);
            continue;
        }

        if (m_allocator->free(m_handle[bufIndex], m_handleIsLocked[bufIndex]) != 0) {
            CLOGE("ERR(%s[%d]):could not free [bufIndex=%d]", __FUNCTION__, __LINE__, bufIndex);
            goto func_exit;
        }
        m_dequeuedBufCount--;
        m_handle[bufIndex] = NULL;
        m_handleIsLocked[bufIndex] = false;

        if (updateStatus(
                bufIndex,
                0,
                EXYNOS_CAMERA_BUFFER_POSITION_NONE,
                EXYNOS_CAMERA_BUFFER_PERMISSION_NONE) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setStatus failed [bIndex=%d, position=NONE, permission=NONE]",
                __FUNCTION__, __LINE__, bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }

        if (m_graphicBufferAllocator.free(bufIndex) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_graphicBufferAllocator.free(%d) fail",
                __FUNCTION__, __LINE__, bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t GrallocExynosCameraBufferManager::m_increase(__unused int increaseCount)
{
    CLOGD("DEBUG(%s[%d]):allocMode(%d) is invalid. Do nothing", __FUNCTION__, __LINE__, m_allocMode);
    return INVALID_OPERATION;
}

status_t GrallocExynosCameraBufferManager::m_decrease(void)
{
    return INVALID_OPERATION;
}

status_t GrallocExynosCameraBufferManager::m_putBuffer(int bufIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;

    if (m_handle[bufIndex] != NULL &&
        m_buffer[bufIndex].status.position == EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL) {
        if (m_allocator->enqueueBuffer(m_handle[bufIndex]) != 0) {
            CLOGE("ERR(%s[%d]):could not enqueue_buffer [bufIndex=%d]",
                __FUNCTION__, __LINE__, bufIndex);
            CLOGD("DEBUG(%s[%d]):dump buffer status", __FUNCTION__, __LINE__);
            dump();
            goto func_exit;
        }
        m_dequeuedBufCount--;
        m_handleIsLocked[bufIndex] = false;
    }
    m_buffer[bufIndex].status.position = EXYNOS_CAMERA_BUFFER_POSITION_IN_SERVICE;

#ifdef EXYNOS_CAMERA_BUFFER_TRACE
    CLOGD("DEBUG(%s[%d]):dump buffer status", __FUNCTION__, __LINE__);
    dump();
#endif

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t GrallocExynosCameraBufferManager::m_getBuffer(int *bufIndex, __unused int *acquireFence, __unused int *releaseFence)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;
    buffer_handle_t *bufHandle = NULL;
    int  bufferFd[3] = {0};
    void *bufferAddr[3] = {NULL};

    int   stride = 0;
    int   bufferIndex = -1;

    const private_handle_t *priv_handle;
    bool  isExistedBuffer = false;
    bool  isLocked = false;

#ifdef USE_GRALLOC_BUFFER_COLLECTOR
    /* Error return check is done by callee */
    ret = m_getCollectedBuffer(bufIndex);
#else /* USE_GRALLOC_BUFFER_COLLECTOR */
    m_minUndequeuedBufCount = m_allocator->getMinUndequeueBuffer();

    if (m_minUndequeuedBufCount < 0 ) {
        CLOGE("ERR(%s[%d]):m_minUndequeuedBufCount=%d..",
            __FUNCTION__, __LINE__, m_minUndequeuedBufCount);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

   if (m_dequeuedBufCount == m_reqBufCount - m_minUndequeuedBufCount) {
        CLOGI("INFO(%s[%d]):skip allocation... ", __FUNCTION__, __LINE__);
        CLOGI("INFO(%s[%d]):m_dequeuedBufCount(%d) == m_reqBufCount(%d) - m_minUndequeuedBufCount(%d)",
                __FUNCTION__, __LINE__, m_dequeuedBufCount, m_reqBufCount, m_minUndequeuedBufCount);
        CLOGD("DEBUG(%s[%d]):-- dump buffer status --", __FUNCTION__, __LINE__);
        dump();
        ret = INVALID_OPERATION;

        goto func_exit;
    }
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
    CLOGD("DEBUG(%s[%d]):before dequeueBuffer() "
          "m_reqBufCount=%d, m_dequeuedBufCount=%d, m_minUndequeuedBufCount=%d",
        __FUNCTION__, __LINE__,
        m_reqBufCount, m_dequeuedBufCount, m_minUndequeuedBufCount);
#endif

    if (m_allocator->dequeueBuffer(
            &bufHandle,
            bufferFd,
            (char **)bufferAddr,
            &isLocked) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):dequeueBuffer failed", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_indexOffset < 0 || VIDEO_MAX_FRAME < (m_reqBufCount + m_indexOffset)) {
        CLOGE("ERR(%s[%d]):abnormal value [m_indexOffset=%d, m_reqBufCount=%d]",
                __FUNCTION__, __LINE__, m_indexOffset, m_reqBufCount);
        ret = BAD_VALUE;
        goto func_exit;
    }

    for (int index = m_indexOffset; index < m_reqBufCount + m_indexOffset; index++) {
        if (m_buffer[index].addr[0] != bufferAddr[0]) {
            continue;
        } else {
            bufferIndex = index;
            isExistedBuffer = true;
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
            CLOGI("INFO(%s[%d]):bufferIndex(%d) found!", __FUNCTION__, __LINE__, bufferIndex);
#endif
            break;
        }
    }

    if (isExistedBuffer == false) {
        CLOGI("INFO(%s[%d]):not existedBuffer!", __FUNCTION__, __LINE__);
        if (m_allocator->cancelBuffer(bufHandle) != 0) {
            CLOGE("ERR(%s[%d]):could not cancelBuffer [bufferIndex=%d]",
                __FUNCTION__, __LINE__, bufferIndex);
        }
        ret = BAD_VALUE;
        goto func_exit;
    }

    if (bufferIndex < 0 || VIDEO_MAX_FRAME <= bufferIndex) {
        CLOGE("ERR(%s[%d]):abnormal value [bufferIndex=%d]",
            __FUNCTION__, __LINE__, bufferIndex);
        ret = BAD_VALUE;
        goto func_exit;
    }

    priv_handle = private_handle_t::dynamicCast(*bufHandle);

    if (m_hasMetaPlane == true) {
        switch (m_buffer[bufferIndex].planeCount) {
        case 3:
            m_buffer[bufferIndex].fd[1]   = priv_handle->fd1;
            m_buffer[bufferIndex].addr[1] = (char *)bufferAddr[1];
            /* no break; */
        case 2:
            m_buffer[bufferIndex].fd[0]   = priv_handle->fd;
            m_buffer[bufferIndex].addr[0] = (char *)bufferAddr[0];
            break;
        default:
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):invalid m_buffer[%d].planeCount(%d) with metaPlane, assert!!!!",
                __FUNCTION__, __LINE__, bufferIndex, m_buffer[bufferIndex].planeCount);
            break;
        }
    } else {
        switch (m_buffer[bufferIndex].planeCount) {
        case 3:
            m_buffer[bufferIndex].fd[2]   = priv_handle->fd2;
            m_buffer[bufferIndex].addr[2] = (char *)bufferAddr[2];
            /* no break; */
        case 2:
            m_buffer[bufferIndex].fd[1]   = priv_handle->fd1;
            m_buffer[bufferIndex].addr[1] = (char *)bufferAddr[1];
            /* no break; */
        case 1:
            m_buffer[bufferIndex].fd[0]   = priv_handle->fd;
            m_buffer[bufferIndex].addr[0] = (char *)bufferAddr[0];
            break;
        default:
            android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):invalid m_buffer[%d].planeCount(%d) without metaPlane, assert!!!!",
                __FUNCTION__, __LINE__, bufferIndex, m_buffer[bufferIndex].planeCount);
        }
    }

    m_handleIsLocked[bufferIndex] = isLocked;

    *bufIndex = bufferIndex;
    m_handle[bufferIndex] = bufHandle;
    m_dequeuedBufCount++;
    m_buffer[bufferIndex].status.position   = EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL;
    m_buffer[bufferIndex].status.permission = EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE;
#endif /* NOT USE_GRALLOC_BUFFER_COLLECTOR */

#ifdef EXYNOS_CAMERA_BUFFER_TRACE
    CLOGD("DEBUG(%s[%d]):-- dump buffer status --", __FUNCTION__, __LINE__);
    dump();
    CLOGI("INFO(%s[%d]):-- OUT -- m_dequeuedBufCount=%d, m_minUndequeuedBufCount=%d",
        __FUNCTION__, __LINE__, m_dequeuedBufCount, m_minUndequeuedBufCount);
#endif

func_exit:
    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

#ifdef USE_GRALLOC_REUSE_SUPPORT
bool GrallocExynosCameraBufferManager::m_cancelReuseBuffer(int bufIndex, bool isReuse)
{
    bool ret = false;
    bool found = false;
    List<int>::iterator r;
    int item = -1;
    int maxCount = -1;

    if (isReuse == true) {
        m_availableBufferIndexQLock.lock();
        for (r = m_availableBufferIndexQ.begin(); r != m_availableBufferIndexQ.end(); r++) {
            if (bufIndex == *r) {
                found = true;
                break;
            }
        }

        if (found == true) {
            CLOGI("INFO(%s[%d]):bufIndex=%d is already in (available state)",
                __FUNCTION__, __LINE__, bufIndex);
            m_availableBufferIndexQLock.unlock();
            CLOGI("INFO(%s[%d]):cancelReuse not available buffer is founded [bufIndex=%d]", __FUNCTION__, __LINE__, bufIndex);
            ret = true;
            return ret;
        } else {
            m_availableBufferIndexQ.push_back(m_buffer[bufIndex].index);
        }
        m_availableBufferIndexQLock.unlock();

        m_handleIsLocked[bufIndex] = true;
        m_buffer[bufIndex].status.position   = EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL;
        m_buffer[bufIndex].status.permission = EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE;

#ifdef USE_GRALLOC_BUFFER_COLLECTOR
        m_collectedBufferCount++;
#endif
        ret = true;
    }

    return ret;
}
#endif

#ifdef USE_GRALLOC_REUSE_SUPPORT
status_t GrallocExynosCameraBufferManager::cancelBuffer(int bufIndex, bool isReuse)
#else
status_t GrallocExynosCameraBufferManager::cancelBuffer(int bufIndex)
#endif
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;
    Mutex::Autolock lock(m_lock);

    List<int>::iterator r;
    bool found = false;
#ifdef USE_GRALLOC_REUSE_SUPPORT
    bool reuseRet = false;
#endif

    if (bufIndex < 0 || m_reqBufCount + m_indexOffset <= bufIndex) {
        CLOGE("ERR(%s[%d]):buffer Index in out of bound [bufIndex=%d]",
            __FUNCTION__, __LINE__, bufIndex);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    if (m_handleIsLocked[bufIndex] == false) {
        CLOGD("DEBUG(%s[%d]):buffer [bufIndex=%d] already free", __FUNCTION__, __LINE__, bufIndex);
        return ret;
    }

#ifdef USE_GRALLOC_REUSE_SUPPORT
    reuseRet = m_cancelReuseBuffer(bufIndex, isReuse);
    if (reuseRet == true) {
        goto func_exit;
    }
#endif

    if (m_allocator->cancelBuffer(m_handle[bufIndex]) != 0) {
        CLOGE("ERR(%s[%d]):could not cancel buffer [bufIndex=%d]", __FUNCTION__, __LINE__, bufIndex);
        goto func_exit;
    }
    m_dequeuedBufCount--;
    m_handle[bufIndex] = NULL;
    m_handleIsLocked[bufIndex] = false;

    if (updateStatus(
            bufIndex,
            0,
            EXYNOS_CAMERA_BUFFER_POSITION_NONE,
            EXYNOS_CAMERA_BUFFER_PERMISSION_NONE) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):setStatus failed [bIndex=%d, position=NONE, permission=NONE]",
            __FUNCTION__, __LINE__, bufIndex);
        ret = INVALID_OPERATION;
        goto func_exit;
    }

    m_availableBufferIndexQLock.lock();
    for (r = m_availableBufferIndexQ.begin(); r != m_availableBufferIndexQ.end(); r++) {
        if (bufIndex == *r) {
            found = true;
            break;
        }
    }

    if (found == true) {
        CLOGI("INFO(%s[%d]):bufIndex=%d is already in (available state)",
            __FUNCTION__, __LINE__, bufIndex);
        m_availableBufferIndexQLock.unlock();
        goto func_exit;
    }
    m_availableBufferIndexQ.push_back(m_buffer[bufIndex].index);
    m_availableBufferIndexQLock.unlock();

#ifdef EXYNOS_CAMERA_BUFFER_TRACE
    CLOGD("DEBUG(%s[%d]):-- dump buffer status --", __FUNCTION__, __LINE__);
    dump();
#endif

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

void GrallocExynosCameraBufferManager::deinit(void)
{
    CLOGD("DEBUG(%s[%d]):IN.." , __FUNCTION__, __LINE__);

#ifdef USE_GRALLOC_BUFFER_COLLECTOR
    /* declare thread will stop */
    m_stopBufferCollector = true;
#endif

    ExynosCameraBufferManager::deinit();

#ifdef USE_GRALLOC_BUFFER_COLLECTOR
    /* thread stop */
    m_bufferCollector->requestExitAndWait();

    /* after thread end, reset m_stopBufferCollector as default */
    m_stopBufferCollector = false;
#endif

    CLOGD("DEBUG(%s[%d]):OUT..", __FUNCTION__, __LINE__);
}

status_t GrallocExynosCameraBufferManager::resetBuffers(void)
{
    CLOGD("DEBUG(%s[%d]):IN.." , __FUNCTION__, __LINE__);

    status_t ret = NO_ERROR;

#ifdef USE_GRALLOC_BUFFER_COLLECTOR
    /* declare thread will stop */
    m_stopBufferCollector = true;
#endif

    ret = ExynosCameraBufferManager::resetBuffers();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):ExynosCameraBufferManager::resetBuffers()", __FUNCTION__, __LINE__);
    }

#ifdef USE_GRALLOC_BUFFER_COLLECTOR
    /* thread stop */
    m_bufferCollector->requestExitAndWait();

    /* after thread end, reset m_stopBufferCollector as default */
    m_stopBufferCollector = false;
#endif

    CLOGD("DEBUG(%s[%d]):OUT..", __FUNCTION__, __LINE__);

    return ret;
}

status_t GrallocExynosCameraBufferManager::setBufferCount(int bufferCount)
{
    CLOGD("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);
    status_t ret = NO_ERROR;

    ret = resetBuffers();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):resetBuffers() failed", __FUNCTION__, __LINE__);
        goto func_exit;
    }

    ret = m_allocator->setBufferCount(bufferCount);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_allocator->setBufferCount(m_bufferCount(%d) -> %d)",
            __FUNCTION__, __LINE__, m_bufferCount, bufferCount);
        goto func_exit;
    }

    m_bufferCount = bufferCount;

func_exit:

    return ret;
}

int GrallocExynosCameraBufferManager::getBufferCount(void)
{
    CLOGD("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    return m_bufferCount;
}

int GrallocExynosCameraBufferManager::getBufStride(void)
{
    ALOGI("INFO(%s):bufStride=%d", __FUNCTION__, m_bufStride);
    return m_bufStride;
}

void GrallocExynosCameraBufferManager::printBufferState(void)
{
    for (int i = 0; i < m_allocatedBufCount; i++) {
        CLOGI("INFO(%s[%d]):m_buffer[%d].fd[0]=%d, position=%d, permission=%d, lock=%d]",
            __FUNCTION__, __LINE__, i, m_buffer[i].fd[0],
            m_buffer[i].status.position, m_buffer[i].status.permission, m_handleIsLocked[i]);
    }

    return;
}

void GrallocExynosCameraBufferManager::printBufferState(int bufIndex, int planeIndex)
{
    CLOGI("INFO(%s[%d]):m_buffer[%d].fd[%d]=%d, .status.permission=%d, lock=%d]",
        __FUNCTION__, __LINE__, bufIndex, planeIndex, m_buffer[bufIndex].fd[planeIndex],
        m_buffer[bufIndex].status.permission, m_handleIsLocked[bufIndex]);

    return;
}

sp<GraphicBuffer> GrallocExynosCameraBufferManager::getGraphicBuffer(int index)
{
    EXYNOS_CAMERA_BUFFER_IN();

    sp<GraphicBuffer> graphicBuffer;

    int width, height, stride;

    int planeCount = 0;

    if ((index < 0) || (index >= m_allowedMaxBufCount)) {
        CLOGE("ERR(%s[%d]):Buffer index error (%d/%d)", __FUNCTION__, __LINE__, index, m_allowedMaxBufCount);
        goto done;
    }

    if (m_graphicBufferAllocator.getSize(&width, &height, &stride) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_graphicBufferAllocator.getSize(%d) fail", __FUNCTION__, __LINE__, index);
        goto done;
    }

    if (m_graphicBufferAllocator.setSize(width, height, m_bufStride) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_graphicBufferAllocator.setSize(%d) fail", __FUNCTION__, __LINE__, index);
        goto done;
    }

    if (m_graphicBufferAllocator.setGrallocUsage(m_allocator->getGrallocUsage()) != NO_ERROR) {
        CLOGE("ERR(%s[%d]):m_graphicBufferAllocator.setGrallocUsage(%d) fail", __FUNCTION__, __LINE__, index);
        goto done;
    }

    planeCount = m_buffer[index].planeCount;

    if (m_hasMetaPlane == true) {
        planeCount--;
    }

    graphicBuffer = m_graphicBufferAllocator.alloc(index, planeCount, m_buffer[index].fd, m_buffer[index].addr, m_buffer[index].size);
    if (graphicBuffer == 0) {
        CLOGE("ERR(%s[%d]):m_graphicBufferAllocator.alloc(%d) fail", __FUNCTION__, __LINE__, index);
        goto done;
    }

done:
    EXYNOS_CAMERA_BUFFER_OUT();

    return graphicBuffer;
}

#ifdef USE_GRALLOC_BUFFER_COLLECTOR
bool GrallocExynosCameraBufferManager::m_bufferCollectorThreadFunc(void)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;
    buffer_handle_t *bufHandle = NULL;
    int  bufferFd[3] = {0};
    void *bufferAddr[3] = {NULL};

    int   stride = 0;
    int   bufferIndex = -1;

    const private_handle_t *priv_handle;
    bool  isExistedBuffer = false;
    bool  isLocked = false;
    uint8_t tryCount = 0;

    if (m_stopBufferCollector == true) {
        CLOGD("DEBUG(%s[%d]):m_stopBufferCollector == true. so, just return. m_collectedBufferCount(%d)",
                __FUNCTION__, __LINE__, m_collectedBufferCount);
        return false;
    }

    if (m_collectedBufferCount >= MAX_BUFFER_COLLECT_COUNT ||
        m_dequeuedBufCount >= (m_allowedMaxBufCount - m_indexOffset - m_minUndequeuedBufCount)
    ) {
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
        CLOGD("DEBUG(%s[%d]): bufferCollector just return. m_collectedBufferCount(%d) m_dequeuedBufCount(%d)",
                __FUNCTION__, __LINE__, m_collectedBufferCount, m_dequeuedBufCount);
#endif
        goto EXIT;
    }

    /* Blocking Function :
       If the Gralloc buffer queue can not give the available buffer,
       it will be blocked until the buffer which is being rendered
       is released.
     */
    ret = m_allocator->dequeueBuffer(&bufHandle,
            bufferFd,
            (char **) bufferAddr,
            &isLocked);
    if (ret == NO_INIT) {
        CLOGW("WARN(%s[%d]):BufferQueue is abandoned!", __FUNCTION__, __LINE__);
        return false;
    } else if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):dequeueBuffer failed, dequeue(%d), collected(%d)",
                __FUNCTION__, __LINE__, m_dequeuedBufCount, m_collectedBufferCount);
        goto EXIT;
    } else if (bufHandle == NULL) {
        CLOGE("ERR(%s[%d]):Buffer handle is NULL, dequeue(%d), collected(%d)",
                __FUNCTION__, __LINE__, m_dequeuedBufCount, m_collectedBufferCount);
        goto EXIT;
    }

    if (m_indexOffset < 0
            || VIDEO_MAX_FRAME < (m_reqBufCount + m_indexOffset)) {
        CLOGE("ERR(%s[%d]):abnormal value [m_indexOffset=%d, m_reqBufCount=%d]",
                __FUNCTION__, __LINE__, m_indexOffset, m_reqBufCount);
        goto EXIT;
    }

    for (int index = m_indexOffset; index < m_reqBufCount + m_indexOffset; index++) {
        if (m_buffer[index].addr[0] != bufferAddr[0]) {
            continue;
        } else {
            bufferIndex = index;
            isExistedBuffer = true;
#ifdef EXYNOS_CAMERA_BUFFER_TRACE
            CLOGI("INFO(%s[%d]):bufferIndex(%d) found!", __FUNCTION__, __LINE__, bufferIndex);
#endif
            break;
        }
    }

    if (isExistedBuffer == false) {
        CLOGI("INFO(%s[%d]):not existedBuffer!", __FUNCTION__, __LINE__);
        if (m_allocator->cancelBuffer(bufHandle) != 0) {
            CLOGE("ERR(%s[%d]):could not cancelBuffer [bufferIndex=%d]",
                    __FUNCTION__, __LINE__, bufferIndex);
        }
        goto EXIT;
    }

    if (m_stopBufferCollector == true) {
        CLOGD("DEBUG(%s[%d]):m_stopBufferCollector == true. so, just cancel. m_collectedBufferCount(%d)",
                __FUNCTION__, __LINE__, m_collectedBufferCount);

        if (m_allocator->cancelBuffer(bufHandle) != 0) {
            CLOGE("ERR(%s[%d]):could not cancelBuffer [bufferIndex=%d]",
                    __FUNCTION__, __LINE__, bufferIndex);
        }
        return false;
    }

    if (bufferIndex < 0 || VIDEO_MAX_FRAME <= bufferIndex) {
        CLOGE("ERR(%s[%d]):abnormal value [bufferIndex=%d]",
                __FUNCTION__, __LINE__, bufferIndex);
        goto EXIT;
    }

    priv_handle = private_handle_t::dynamicCast(*bufHandle);

    {
        /*
         * this is mutex for race-condition with cancelBuffer()
         * problem scenario.
         *   a. cancelBuffer() : cancelBuffer done -> context switch ->
         *   b. m_preDequeueThreadFunc(): dequeueBuffer done -> m_handleIsLocked[bufferIndex] = true -> context switch ->
         *   c. cancelBuffer() : m_handleIsLocked[bufferIndex] = false
         *   d. and next cancelBuffer() is fail, and lost that buffer forever.
         */

        m_lock.lock();

        if (m_hasMetaPlane == true) {
            switch (m_buffer[bufferIndex].planeCount) {
            case 3:
                m_buffer[bufferIndex].fd[1]   = priv_handle->fd1;
                m_buffer[bufferIndex].addr[1] = (char *)bufferAddr[1];
                /* no break; */
            case 2:
                m_buffer[bufferIndex].fd[0]   = priv_handle->fd;
                m_buffer[bufferIndex].addr[0] = (char *)bufferAddr[0];
                break;
            default:
                android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):invalid m_buffer[%d].planeCount(%d) with metaPlane, assert!!!!",
                    __FUNCTION__, __LINE__, bufferIndex, m_buffer[bufferIndex].planeCount);
                break;
            }
        } else {
            switch (m_buffer[bufferIndex].planeCount) {
            case 3:
                m_buffer[bufferIndex].fd[2]   = priv_handle->fd2;
                m_buffer[bufferIndex].addr[2] = (char *)bufferAddr[2];
                /* no break; */
            case 2:
                m_buffer[bufferIndex].fd[1]   = priv_handle->fd1;
                m_buffer[bufferIndex].addr[1] = (char *)bufferAddr[1];
                /* no break; */
            case 1:
                m_buffer[bufferIndex].fd[0]   = priv_handle->fd;
                m_buffer[bufferIndex].addr[0] = (char *)bufferAddr[0];
                break;
            default:
                android_printAssert(NULL, LOG_TAG, "ASSERT(%s[%d]):invalid m_buffer[%d].planeCount(%d) without metaPlane, assert!!!!",
                    __FUNCTION__, __LINE__, bufferIndex, m_buffer[bufferIndex].planeCount);
            }
        }

        m_handleIsLocked[bufferIndex] = isLocked;

        m_handle[bufferIndex] = bufHandle;
        m_dequeuedBufCount++;
        m_buffer[bufferIndex].status.position   = EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL;
        m_buffer[bufferIndex].status.permission = EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE;

#ifdef EXYNOS_CAMERA_BUFFER_TRACE
        CLOGD("DEBUG(%s[%d]):-- dump buffer status --", __FUNCTION__, __LINE__);
        dump();
        CLOGI("INFO(%s[%d]):-- OUT -- m_dequeuedBufCount=%d, m_minUndequeuedBufCount=%d",
                __FUNCTION__, __LINE__, m_dequeuedBufCount, m_minUndequeuedBufCount);
        CLOGD("DEBUG(%s[%d]):Pre-dequeue buffer(%d)",
                __FUNCTION__, __LINE__, bufferIndex);
#endif

        m_collectedBufferCount ++;

        m_lock.unlock();
    }

EXIT:
    while ((m_collectedBufferCount >= MAX_BUFFER_COLLECT_COUNT
           || m_dequeuedBufCount >= (m_allowedMaxBufCount - m_indexOffset - m_minUndequeuedBufCount))
           && m_stopBufferCollector == false) {
        usleep(BUFFER_COLLECTOR_WAITING_TIME);
    }

    return true;
}

status_t GrallocExynosCameraBufferManager::m_getCollectedBuffer(int *bufIndex)
{
    status_t ret = NO_ERROR;
    List<int>::iterator r;
    int currentBufferIndex = -1;

    if (m_collectedBufferCount < 1) {
        CLOGE("ERR(%s[%d]):Gralloc buffer collector has no Buffer", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
    } else {
        m_availableBufferIndexQLock.lock();

        for (r = m_availableBufferIndexQ.begin(); r != m_availableBufferIndexQ.end(); r++) {
            /* Found the collected buffer */
            if (m_isCollectedBuffer(*r) == true) {
                currentBufferIndex  = *r;
                break;
            }
        }

        m_availableBufferIndexQLock.unlock();
    }

    if (currentBufferIndex > -1) {
        *bufIndex = currentBufferIndex;
        m_collectedBufferCount --;
        CLOGV("INFO(%s[%d]):Get buffer(%d) from gralloc buffer collector, available count(%d)",
                __FUNCTION__, __LINE__, currentBufferIndex, m_collectedBufferCount);
    } else {
        ret = INVALID_OPERATION;
        CLOGE("ERR(%s[%d]):Failed to get available gralloc buffer from buffer collector, available count(%d)",
                __FUNCTION__, __LINE__, m_collectedBufferCount);
    }

    return ret;
}

bool GrallocExynosCameraBufferManager::m_isCollectedBuffer(int bufferIndex)
{
    bool ret = false;
    bool isValidPosition = false;
    bool isValidPermission = false;

    switch (m_buffer[bufferIndex].status.position) {
    case EXYNOS_CAMERA_BUFFER_POSITION_IN_HAL:
        isValidPosition = true;
        break;
    case EXYNOS_CAMERA_BUFFER_POSITION_NONE:
    case EXYNOS_CAMERA_BUFFER_POSITION_IN_DRIVER:
    case EXYNOS_CAMERA_BUFFER_POSITION_IN_SERVICE:
    default:
        isValidPosition = false;
        break;
    }

    switch (m_buffer[bufferIndex].status.permission) {
    case EXYNOS_CAMERA_BUFFER_PERMISSION_AVAILABLE:
        isValidPermission = true;
        break;
    case EXYNOS_CAMERA_BUFFER_PERMISSION_NONE:
    case EXYNOS_CAMERA_BUFFER_PERMISSION_IN_PROCESS:
    default:
        isValidPermission = false;
        break;
    }

    ret = isValidPosition && isValidPermission;
    return ret;
}
#endif /* USE_GRALLOC_BUFFER_COLLECTOR */
ExynosCameraFence::ExynosCameraFence()
{
    m_fenceType = EXYNOS_CAMERA_FENCE_TYPE_BASE;

    m_frameCount = -1;
    m_index = -1;

    m_acquireFence = -1;
    m_releaseFence = -1;

    m_fence = 0;

    m_flagSwfence = false;
}

ExynosCameraFence::ExynosCameraFence(
        enum EXYNOS_CAMERA_FENCE_TYPE fenceType,
        int frameCount,
        int index,
        int acquireFence,
        int releaseFence)
{
    /* default setting */
    m_fenceType = EXYNOS_CAMERA_FENCE_TYPE_BASE;

    m_frameCount = -1;
    m_index = -1;

    m_acquireFence = -1;
    m_releaseFence = -1;

    m_fence = 0;
    m_flagSwfence = false;

    /* we will set from here */
    if (fenceType <= EXYNOS_CAMERA_FENCE_TYPE_BASE ||
        EXYNOS_CAMERA_FENCE_TYPE_MAX <= fenceType) {
        ALOGE("ERR(%s[%d]):invalid fenceType(%d), frameCount(%d), index(%d)",
            __FUNCTION__, __LINE__, fenceType, frameCount, index);
        return;
    }

    m_fenceType = fenceType;

    m_frameCount = frameCount;
    m_index = index;

    m_acquireFence = acquireFence;
    m_releaseFence = releaseFence;

    if (0 <= m_acquireFence || 0 <= m_releaseFence) {
        ALOGV("DEBUG(%s[%d]):fence(%d):m_acquireFence(%d), m_releaseFence(%d)",
            __FUNCTION__, __LINE__, m_frameCount, m_acquireFence, m_releaseFence);
    }

#ifdef USE_SW_FENCE
    m_flagSwfence = true;
#endif

    if (m_flagSwfence == true) {
        switch (m_fenceType) {
        case EXYNOS_CAMERA_FENCE_TYPE_ACQUIRE:
            m_fence = new Fence(acquireFence);
            break;
        case EXYNOS_CAMERA_FENCE_TYPE_RELEASE:
            m_fence = new Fence(releaseFence);
            break;
        default:
            ALOGE("ERR(%s[%d]):invalid m_fenceType(%d), m_frameCount(%d), m_index(%d)",
                __FUNCTION__, __LINE__, m_fenceType, m_frameCount, m_index);
            break;
        }
    }
}

ExynosCameraFence::~ExynosCameraFence()
{
    /* delete sp<Fence> addr */
    m_fence = 0;
#ifdef FORCE_CLOSE_ACQUIRE_FD
    static uint64_t closeCnt = 0;
    if(m_acquireFence >= FORCE_CLOSE_ACQUIRE_FD_THRESHOLD) {
        if(closeCnt++ % 1000 == 0) {
            ALOGW("ERR(%s[%d]):Attempt to close acquireFence[%d], %d th close.",
                    __FUNCTION__, __LINE__, m_acquireFence, closeCnt);
        }
        ::close(m_acquireFence);
    }
#endif
}

int ExynosCameraFence::getFenceType(void)
{
    return m_fenceType;
}

int ExynosCameraFence::getFrameCount(void)
{
    return m_frameCount;
}

int ExynosCameraFence::getIndex(void)
{
    return m_index;
}

int ExynosCameraFence::getAcquireFence(void)
{
    return m_acquireFence;
}

int ExynosCameraFence::getReleaseFence(void)
{
    return m_releaseFence;
}

bool ExynosCameraFence::isValid(void)
{
    bool ret = false;

    if (m_flagSwfence == true) {
        if (m_fence == NULL) {
            ALOGE("ERR(%s[%d]):m_fence == NULL. so, fail", __FUNCTION__, __LINE__);
            ret = false;
        } else {
            ret = m_fence->isValid();
        }
    } else {
        switch (m_fenceType) {
        case EXYNOS_CAMERA_FENCE_TYPE_ACQUIRE:
            if (0 <= m_acquireFence)
              ret = true;
            break;
        case EXYNOS_CAMERA_FENCE_TYPE_RELEASE:
            if (0 <= m_releaseFence)
              ret = true;
            break;
        default:
            ALOGE("ERR(%s[%d]):invalid m_fenceType(%d), m_frameCount(%d), m_index(%d)",
                __FUNCTION__, __LINE__, m_fenceType, m_frameCount, m_index);
            break;
        }
    }

    return ret;
}

status_t ExynosCameraFence::wait(int time)
{
    status_t ret = NO_ERROR;

    if (this->isValid() == false) {
        ALOGE("ERR(%s[%d]):this->isValid() == false. so, fail!! frameCount(%d), index(%d), fencType(%d)",
            __FUNCTION__, __LINE__, m_frameCount, m_index, m_fenceType);
        return INVALID_OPERATION;
    }

    if (m_flagSwfence == false) {
        ALOGW("WARN(%s[%d]):m_flagSwfence == false. so, fail!! frameCount(%d), index(%d), fencType(%d)",
            __FUNCTION__, __LINE__, m_frameCount, m_index, m_fenceType);

        return INVALID_OPERATION;
    }

    int waitTime = time;
    if (waitTime < 0)
        waitTime = 1000; /* wait 1 sec */

    int fenceFd = -1;

    switch (m_fenceType) {
    case EXYNOS_CAMERA_FENCE_TYPE_ACQUIRE:
        fenceFd = m_acquireFence;
        break;
    case EXYNOS_CAMERA_FENCE_TYPE_RELEASE:
        fenceFd = m_releaseFence;
        break;
    default:
        ALOGE("ERR(%s[%d]):invalid m_fenceType(%d), m_frameCount(%d), m_index(%d)",
            __FUNCTION__, __LINE__, m_fenceType, m_frameCount, m_index);
        break;
    }

    ret = m_fence->wait(waitTime);
    if (ret == TIMED_OUT) {
        ALOGE("ERR(%s[%d]):Fence timeout. so, fail!! fenceFd(%d), frameCount(%d), index(%d), fencType(%d)",
            __FUNCTION__, __LINE__, fenceFd, m_frameCount, m_index, m_fenceType);

        return INVALID_OPERATION;
    } else if (ret != OK) {
        ALOGE("ERR(%s[%d]):Fence wait error. so, fail!! fenceFd(%d), frameCount(%d), index(%d), fencType(%d)",
            __FUNCTION__, __LINE__, fenceFd, m_frameCount, m_index, m_fenceType);

        return INVALID_OPERATION;
    }

    return ret;
}

ServiceExynosCameraBufferManager::ServiceExynosCameraBufferManager()
{
    ExynosCameraBufferManager::init();

    m_allocator = new ExynosCameraStreamAllocator();

    CLOGD("DEBUG(%s[%d])", __FUNCTION__, __LINE__);
    for (int bufIndex = 0; bufIndex < VIDEO_MAX_FRAME; bufIndex++) {
        m_handle[bufIndex]         = NULL;
        m_handleIsLocked[bufIndex] = false;
    }
}

ServiceExynosCameraBufferManager::~ServiceExynosCameraBufferManager()
{
    ExynosCameraBufferManager::deinit();
}

status_t ServiceExynosCameraBufferManager::registerBuffer(
        int frameCount,
        buffer_handle_t *handle,
        int acquireFence,
        int releaseFence,
        enum EXYNOS_CAMERA_BUFFER_POSITION position)
{
    EXYNOS_CAMERA_BUFFER_IN();
    /*
     * this->putBuffer(index, position) has same lock.
     * so, don't lock here
     */
    //Mutex::Autolock lock(m_lock);

    status_t ret = NO_ERROR;
    ExynosCameraFence *fence = NULL;

    int index = -1;

    ret = getIndexByHandle(handle, &index);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):getIndexByHandle error (%d)", __FUNCTION__, __LINE__, ret);
        goto func_exit;
    }

    ret = putBuffer(index, position);
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):putBuffer(%d) error (%d)", __FUNCTION__, __LINE__, index, ret);
        goto func_exit;
    }

    m_lock.lock();
    ret = m_registerBuffer(&handle, index);
    if (ret < 0) {
        CLOGE("ERR(%s[%d]):m_setBuffer fail", __FUNCTION__, __LINE__);
    }

    /*
     * Wait release fence, before give buffer to h/w.
     * Save acquire fence, this will give to h/w.
     */
    fence = new ExynosCameraFence(
                    ExynosCameraFence::EXYNOS_CAMERA_FENCE_TYPE_ACQUIRE,
                    frameCount,
                    index,
                    acquireFence,
                    releaseFence);

    m_pushFence(&m_fenceList, fence);
    m_lock.unlock();

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ServiceExynosCameraBufferManager::getIndexByHandle(buffer_handle_t *handle, int *index)
{
    EXYNOS_CAMERA_BUFFER_IN();
    status_t ret = OK;
    int  emptyIndex = -2;
    int  bufIndex = -2;
    bool flag = false;

    if (handle == NULL)
        return BAD_VALUE;

    if (m_indexOffset < 0 || VIDEO_MAX_FRAME < (m_reqBufCount + m_indexOffset)) {
        CLOGE("ERR(%s[%d]):abnormal value [m_indexOffset=%d, m_reqBufCount=%d]",
                __FUNCTION__, __LINE__, m_indexOffset, m_reqBufCount);
        return BAD_VALUE;
    }

    for (int i = m_indexOffset; i < m_reqBufCount + m_indexOffset; i++) {
        if (handle == m_handle[i]) {
            emptyIndex = i;
            CLOGV("DEBUG(%s[%d]):index(%d), (%p/%p)", __FUNCTION__, __LINE__, emptyIndex, handle , m_handle[i]);
            flag = true;
            break;
        }
        if (m_handle[i] == NULL) {
            emptyIndex = i;
        }

    }
    bufIndex = emptyIndex;

    if (flag == false && bufIndex >= 0) {
        CLOGD("DEBUG(%s[%d]): assigned new buffer handle(%p) Index(%d)", __FUNCTION__, __LINE__, m_handle[bufIndex], bufIndex);
    }

    if ((bufIndex < 0) || (bufIndex >= m_allowedMaxBufCount + m_indexOffset)) {
        CLOGE("ERR(%s[%d]):Buffer index error (%d/%d)", __FUNCTION__, __LINE__, bufIndex, m_allowedMaxBufCount);
        return BAD_VALUE;
    }

    *index = bufIndex;

    EXYNOS_CAMERA_BUFFER_OUT();
    return ret;
}

status_t ServiceExynosCameraBufferManager::getHandleByIndex(buffer_handle_t **handle, int index)
{
    EXYNOS_CAMERA_BUFFER_IN();
    if ((index < 0) || (index >= m_allowedMaxBufCount)) {
        CLOGE("ERR(%s[%d]):Buffer index error (%d/%d)", __FUNCTION__, __LINE__, index, m_allowedMaxBufCount);
        return BAD_VALUE;
    }

    if (m_handle[index] == NULL)
        CLOGE("ERR(%s):m_handle[%d] is NULL!!", __FUNCTION__, index);

    *handle = m_handle[index];

    EXYNOS_CAMERA_BUFFER_OUT();
    return OK;
}

void ServiceExynosCameraBufferManager::m_resetSequenceQ()
{
    Mutex::Autolock lock(m_availableBufferIndexQLock);
    m_availableBufferIndexQ.clear();

    /*
     * service buffer is given by service.
     * so, initial state is all un-available.
     */
    /*
    for (int bufIndex = 0; bufIndex < m_allocatedBufCount; bufIndex++)
        m_availableBufferIndexQ.push_back(m_buffer[bufIndex].index);
    */
}

status_t ServiceExynosCameraBufferManager::m_setAllocator(void *allocator)
{
    if (m_allocator == NULL) {
        ALOGE("ERR(%s[%d]):m_allocator equals NULL", __FUNCTION__, __LINE__);
        goto func_exit;
    }

    m_allocator->init((camera3_stream_t *)allocator);

func_exit:
    return NO_ERROR;
}

status_t ServiceExynosCameraBufferManager::m_compareFdOfBufferHandle(const buffer_handle_t* handle, const ExynosCameraBuffer* exynosBuf) {
    bool fdCmp = true;
    const private_handle_t * privHandle = NULL;
    int fdCmpPlaneNum = 0;

    if (handle == NULL)
        return BAD_VALUE;
    if (exynosBuf == NULL)
        return BAD_VALUE;

    privHandle = private_handle_t::dynamicCast(*handle);
    if (privHandle == NULL)
        return BAD_VALUE;

    fdCmpPlaneNum = (m_hasMetaPlane) ? exynosBuf->planeCount- 1 : exynosBuf->planeCount;
    switch(fdCmpPlaneNum) {
        /* Compare each plane's DMA fd */
        case 3:
            fdCmp = fdCmp && (exynosBuf->fd[2] == privHandle->fd2);
        case 2:
            fdCmp = fdCmp && (exynosBuf->fd[1] == privHandle->fd1);
        case 1:
            fdCmp = fdCmp && (exynosBuf->fd[0] == privHandle->fd);
            break;
        default:
            CLOGE("ERR(%s[%d]):Invalid plane count [m_buffer.planeCount=%d, m_hasMetaPlane=%d]",
                    __FUNCTION__, __LINE__, exynosBuf->planeCount, m_hasMetaPlane);
            return INVALID_OPERATION;
    }

    if(fdCmp == true) {
        return NO_ERROR;
    } else {
        CLOGI("INFO(%s[%d]): same handle but different FD : index[%d] handleFd[%d/%d/%d] - bufFd[%d/%d/%d]"
            , __FUNCTION__, __LINE__
            , exynosBuf->index
            , privHandle->fd, privHandle->fd1, privHandle->fd2
            , exynosBuf->fd[0], exynosBuf->fd[1], exynosBuf->fd[2]);
        return NAME_NOT_FOUND;
    }
}

status_t ServiceExynosCameraBufferManager::m_registerBuffer(buffer_handle_t **handle, int index)
{
    status_t ret = OK;
    int planeCount = 0;

    if (handle == NULL)
        return BAD_VALUE;

    const private_handle_t * privHandle = private_handle_t::dynamicCast(**handle);
    CLOGV("DEBUG(%s[%d]): register handle[%d/%d/%d] - buf[index:%d][%d/%d/%d]"
        , __FUNCTION__, __LINE__
            , privHandle->fd, privHandle->fd1, privHandle->fd2
            , index, m_buffer[index].fd[0], m_buffer[index].fd[1], m_buffer[index].fd[2]);

    if (m_handleIsLocked[index] == true) {
        /* Check the contents of buffer_handle_t */
        if(m_compareFdOfBufferHandle(*handle, &m_buffer[index]) == NO_ERROR) {
            return NO_ERROR;
        }
        /* Otherwise, DMA fd shoud be updated on following codes. */
    }

    m_handle[index] = *handle;

    planeCount = m_buffer[index].planeCount;

    if (m_hasMetaPlane == true)
        planeCount--;

    ret = m_allocator->lock(
                        handle,
                        m_buffer[index].fd,
                        m_buffer[index].addr,
                        &m_handleIsLocked[index], planeCount);
    if (ret != 0)
        CLOGE("ERR(%s[%d]):m_allocator->lock failed.. ", __FUNCTION__, __LINE__);

    return ret;
}

status_t ServiceExynosCameraBufferManager::m_alloc(int bIndex, int eIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = OK;
    CLOGD("DEBUG(%s[%d]):", __FUNCTION__, __LINE__);

    for (int bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        if (updateStatus(
                bufIndex,
                0,
                EXYNOS_CAMERA_BUFFER_POSITION_IN_SERVICE,
                EXYNOS_CAMERA_BUFFER_PERMISSION_NONE) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setStatus failed [bufIndex=%d, position=SERVICE, permission=NONE]",
                __FUNCTION__, __LINE__, bufIndex);
            ret = INVALID_OPERATION;
            break;
        }
    }

    /*
     * service buffer is given by service.
     * so, initial state is all un-available.
     */
    m_availableBufferIndexQLock.lock();
    m_availableBufferIndexQ.clear();
    m_availableBufferIndexQLock.unlock();

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ServiceExynosCameraBufferManager::m_free(int bIndex, int eIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;

    CLOGD("DEBUG(%s[%d]):IN -- dump buffer status --", __FUNCTION__, __LINE__);
    dump();

    for (int bufIndex = bIndex; bufIndex < eIndex; bufIndex++) {
        if (m_handleIsLocked[bufIndex] == false) {
            CLOGD("DEBUG(%s[%d]):buffer [bufIndex=%d] already free", __FUNCTION__, __LINE__, bufIndex);
            continue;
        }

        m_removeFence(&m_fenceList, bufIndex);

        m_handle[bufIndex] = NULL;
        m_handleIsLocked[bufIndex] = false;

        if (updateStatus(
                bufIndex,
                0,
                EXYNOS_CAMERA_BUFFER_POSITION_NONE,
                EXYNOS_CAMERA_BUFFER_PERMISSION_NONE) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):setStatus failed [bIndex=%d, position=NONE, permission=NONE]",
                __FUNCTION__, __LINE__, bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }

        if (m_graphicBufferAllocator.free(bufIndex) != NO_ERROR) {
            CLOGE("ERR(%s[%d]):m_graphicBufferAllocator.free(%d) fail",
                __FUNCTION__, __LINE__, bufIndex);
            ret = INVALID_OPERATION;
            goto func_exit;
        }
    }

func_exit:

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ServiceExynosCameraBufferManager::m_increase(__unused int increaseCount)
{
    CLOGD("DEBUG(%s[%d]):allocMode(%d) is invalid. Do nothing", __FUNCTION__, __LINE__, m_allocMode);
    return INVALID_OPERATION;
}

status_t ServiceExynosCameraBufferManager::m_decrease(void)
{
    return INVALID_OPERATION;
}

status_t ServiceExynosCameraBufferManager::m_putBuffer(__unused int bufIndex)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;

    CLOGV("DEBUG(%s[%d]):no effect" , __FUNCTION__, __LINE__);

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ServiceExynosCameraBufferManager::m_getBuffer(int *bufIndex, int *acquireFence, int *releaseFence)
{
    EXYNOS_CAMERA_BUFFER_IN();

    status_t ret = NO_ERROR;

    ExynosCameraFence *ptrFence = NULL;
    int frameCount = -1;

    /* this is default value */
    *acquireFence = -1;
    *releaseFence = -1;

    ptrFence = m_popFence(&m_fenceList);
    if (ptrFence == NULL) {
        CLOGE("DEBUG(%s[%d]):m_popFence() fail", __FUNCTION__, __LINE__);
        ret = INVALID_OPERATION;
        goto done;
    }

    *bufIndex = ptrFence->getIndex();
    if (*bufIndex < 0) {
        CLOGE("ERR(%s[%d]):*bufIndex(%d) < 0", __FUNCTION__, __LINE__, *bufIndex);
        ret = BAD_VALUE;
        goto done;
    }

    frameCount = ptrFence->getFrameCount();
    if (frameCount < 0) {
        CLOGE("ERR(%s[%d]):frameCount(%d) < 0", __FUNCTION__, __LINE__, frameCount);
        ret = BAD_VALUE;
        goto done;
    }

#ifdef USE_CAMERA2_USE_FENCE
    if (ptrFence != NULL && ret == NO_ERROR) {
#ifdef USE_SW_FENCE
        /* wait before give buffer to hardware */
        if (ptrFence->isValid() == true) {
            ret = m_waitFence(ptrFence);
            if (ret != NO_ERROR) {
                CLOGE("ERR(%s[%d]):m_waitFence() fail", __FUNCTION__, __LINE__);
                goto done;
            }
        } else {
            CLOGV("DEBUG(%s[%d]):Fence is invalid. framecount=%d", __FUNCTION__, __LINE__, frameCount);
        }
#else
        /* give fence to H/W */
        *acquireFence = ptrFence->getAcquireFence();
        *releaseFence = ptrFence->getReleaseFence();
#endif
    }
#endif

    if (m_allocator->unlock(m_handle[*bufIndex]) != 0) {
        ALOGE("ERR(%s):grallocHal->unlock failed", __FUNCTION__);
        return INVALID_OPERATION;
    }

done:
    if (ptrFence != NULL) {
        delete ptrFence;
        ptrFence = NULL;
    }

    EXYNOS_CAMERA_BUFFER_OUT();

    return ret;
}

status_t ServiceExynosCameraBufferManager::m_waitFence(ExynosCameraFence *fence)
{
    status_t ret = NO_ERROR;

#if 0
    /* reference code */
    sp<Fence> bufferAcquireFence = new Fence(buffer->acquire_fence);
    ret = bufferAcquireFence->wait(1000); /* 1 sec */
    if (ret == TIMED_OUT) {
        ALOGE("ERR(%s[%d]):Fence timeout(%d)!!", __FUNCTION__, __LINE__, request->frame_number);
        return INVALID_OPERATION;
    } else if (ret != OK) {
        CLOGE("ERR(%s[%d]):Waiting on Fence error(%d)!!", __FUNCTION__, __LINE__, request->frame_number);
        return INVALID_OPERATION;
    }
#endif

    if (fence == NULL) {
        CLOGE("DEBUG(%s[%d]):fence == NULL. so, fail", __FUNCTION__, __LINE__);
        return INVALID_OPERATION;
    }

    if (fence->isValid() == false) {
        CLOGE("ERR(%s[%d]):fence(%d)->isValid() == false. so, fail",
            __FUNCTION__, __LINE__, fence->getFrameCount());
        return INVALID_OPERATION;
    }

    CLOGV("DEBUG(%s[%d]):Valid fence on frameCount(%d)",
        __FUNCTION__, __LINE__, fence->getFrameCount());


    ret = fence->wait();
    if (ret != NO_ERROR) {
        CLOGE("ERR(%s[%d]):fence(frameCount : %d)->wait() fail",
            __FUNCTION__, __LINE__, fence->getFrameCount());
        return INVALID_OPERATION;
    } else {
        CLOGV("DEBUG(%s[%d]):fence(frameCount : %d)->wait() succeed",
            __FUNCTION__, __LINE__, fence->getFrameCount());
    }

    return ret;
}

void ServiceExynosCameraBufferManager::m_pushFence(List<ExynosCameraFence *> *list, ExynosCameraFence *fence)
{
    Mutex::Autolock l(m_fenceListLock);

    list->push_back(fence);
}

ExynosCameraFence *ServiceExynosCameraBufferManager::m_popFence(List<ExynosCameraFence *> *list)
{
    ExynosCameraFence *curFence = NULL;
    List<ExynosCameraFence *>::iterator r;

    Mutex::Autolock l(m_fenceListLock);

    if (list->empty()) {
        CLOGE("ERR(%s[%d]):list is empty", __FUNCTION__, __LINE__);
        return NULL;
    }

    r = list->begin()++;
    curFence = *r;
    list->erase(r);

    return curFence;
}

status_t ServiceExynosCameraBufferManager::m_removeFence(List<ExynosCameraFence *> *list, int index)
{
    ExynosCameraFence *curFence = NULL;
    List<ExynosCameraFence *>::iterator r;

    Mutex::Autolock l(m_fenceListLock);

    if (list->empty()) {
        CLOGD("DEBUG(%s[%d]):list is empty", __FUNCTION__, __LINE__);
        return NO_ERROR;
    }

    r = list->begin()++;

    do {
        curFence = *r;
        if (curFence == NULL) {
            ALOGE("ERR(%s):curFence is empty", __FUNCTION__);
            return INVALID_OPERATION;
        }

        if (curFence->getIndex() == index) {
            CLOGV("DEBUG(%s[%d]):remove Fence(%d), frameCount(%d)",
                __FUNCTION__,  __LINE__, index, curFence->getFrameCount());

            list->erase(r);
            delete curFence;
            curFence = NULL;
            return NO_ERROR;
        }
        r++;
    } while (r != list->end());

    CLOGD("DEBUG(%s[%d]):Cannot find index(%d)", __FUNCTION__, __LINE__, index);

    return INVALID_OPERATION;
}
} // namespace android
