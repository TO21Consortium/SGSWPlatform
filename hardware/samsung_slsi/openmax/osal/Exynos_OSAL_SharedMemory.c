/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
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

/*
 * @file        Exynos_OSAL_SharedMemory.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              Jinsung Yang (jsgood.yang@samsung.com)
 *              Taehwan Kim (t_h.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <cutils/log.h>
#include <cutils/atomic.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <ion/ion.h>
#include "exynos_ion.h"

#include "Exynos_OSAL_Mutex.h"
#include "Exynos_OSAL_Memory.h"
#include "Exynos_OSAL_SharedMemory.h"

#include "Exynos_OSAL_ETC.h"

//#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"


static int mem_cnt = 0;
static int map_cnt = 0;

struct EXYNOS_SHAREDMEM_LIST;
typedef struct _EXYNOS_SHAREDMEM_LIST
{
    OMX_U32                        IONBuffer;
    OMX_PTR                        mapAddr;
    OMX_U32                        allocSize;
    OMX_BOOL                       owner;
    struct _EXYNOS_SHAREDMEM_LIST *pNextMemory;
} EXYNOS_SHAREDMEM_LIST;

typedef struct _EXYNOS_SHARED_MEMORY
{
    int                    hIONHandle;
    EXYNOS_SHAREDMEM_LIST *pAllocMemory;
    OMX_HANDLETYPE         hSMMutex;
} EXYNOS_SHARED_MEMORY;


OMX_HANDLETYPE Exynos_OSAL_SharedMemory_Open()
{
    EXYNOS_SHARED_MEMORY *pHandle   = NULL;
    int                   IONClient = -1;

    pHandle = (EXYNOS_SHARED_MEMORY *)Exynos_OSAL_Malloc(sizeof(EXYNOS_SHARED_MEMORY));
    if (pHandle == NULL)
        goto EXIT;
    Exynos_OSAL_Memset(pHandle, 0, sizeof(EXYNOS_SHARED_MEMORY));

    IONClient = ion_open();
    if (IONClient < 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "ion_open is failed: %d", IONClient);
        Exynos_OSAL_Free((void *)pHandle);
        pHandle = NULL;
        goto EXIT;
    }

    pHandle->hIONHandle = IONClient;

    if (OMX_ErrorNone != Exynos_OSAL_MutexCreate(&pHandle->hSMMutex)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Exynos_OSAL_MutexCreate(hSMMutex) is failed");
        /* free a ion_client */
        ion_close(pHandle->hIONHandle);
        pHandle->hIONHandle = -1;

        Exynos_OSAL_Free((void *)pHandle);
        pHandle = NULL;
    }

EXIT:
    return (OMX_HANDLETYPE)pHandle;
}

void Exynos_OSAL_SharedMemory_Close(OMX_HANDLETYPE handle)
{
    EXYNOS_SHARED_MEMORY  *pHandle = (EXYNOS_SHARED_MEMORY *)handle;
    EXYNOS_SHAREDMEM_LIST *pSMList = NULL;
    EXYNOS_SHAREDMEM_LIST *pCurrentElement = NULL;
    EXYNOS_SHAREDMEM_LIST *pDeleteElement = NULL;

    if (pHandle == NULL)
        goto EXIT;

    Exynos_OSAL_MutexLock(pHandle->hSMMutex);
    pCurrentElement = pSMList = pHandle->pAllocMemory;

    while (pCurrentElement != NULL) {
        pDeleteElement = pCurrentElement;
        pCurrentElement = pCurrentElement->pNextMemory;

        /* if mmap was not called, mapAddr is same as IONBuffer */
        if (pDeleteElement->mapAddr != INT_TO_PTR(pDeleteElement->IONBuffer)) {
            if (munmap(pDeleteElement->mapAddr, pDeleteElement->allocSize))
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "munmap is failed");
        }

        pDeleteElement->mapAddr = NULL;
        pDeleteElement->allocSize = 0;

        if (pDeleteElement->owner) {
            /* free a ion_buffer */
            ion_close(pDeleteElement->IONBuffer);
            mem_cnt--;
        }
        pDeleteElement->IONBuffer = 0;

        Exynos_OSAL_Free(pDeleteElement);

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SharedMemory mem count: %d", mem_cnt);
    }

    pHandle->pAllocMemory = pSMList = NULL;
    Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);

    Exynos_OSAL_MutexTerminate(pHandle->hSMMutex);
    pHandle->hSMMutex = NULL;

    /* free a ion_client */
    ion_close(pHandle->hIONHandle);
    pHandle->hIONHandle = -1;

    Exynos_OSAL_Free(pHandle);

EXIT:
    return;
}

OMX_PTR Exynos_OSAL_SharedMemory_Alloc(OMX_HANDLETYPE handle, OMX_U32 size, MEMORY_TYPE memoryType)
{
    EXYNOS_SHARED_MEMORY  *pHandle         = (EXYNOS_SHARED_MEMORY *)handle;
    EXYNOS_SHAREDMEM_LIST *pSMList         = NULL;
    EXYNOS_SHAREDMEM_LIST *pElement        = NULL;
    EXYNOS_SHAREDMEM_LIST *pCurrentElement = NULL;
    OMX_S32                IONBuffer       = 0;
    OMX_PTR                pBuffer         = NULL;
    unsigned int mask;
    unsigned int flag;

    if (pHandle == NULL)
        goto EXIT;

    pElement = (EXYNOS_SHAREDMEM_LIST *)Exynos_OSAL_Malloc(sizeof(EXYNOS_SHAREDMEM_LIST));
    if (pElement == NULL)
        goto EXIT;
    Exynos_OSAL_Memset(pElement, 0, sizeof(EXYNOS_SHAREDMEM_LIST));
    pElement->owner = OMX_TRUE;

    /* priority is like as EXT > SECURE > CONTIG > CACHED > NORMAL */
    switch ((int)memoryType) {
    case (EXT_MEMORY | SECURE_MEMORY | CONTIG_MEMORY | CACHED_MEMORY):  /* EXTRA */
    case (EXT_MEMORY | SECURE_MEMORY | CONTIG_MEMORY):
    case (EXT_MEMORY | SECURE_MEMORY | CACHED_MEMORY):
    case (EXT_MEMORY | SECURE_MEMORY):
        mask = ION_HEAP_EXYNOS_CONTIG_MASK;
        flag = ION_EXYNOS_VIDEO_EXT_MASK | ION_FLAG_PROTECTED;
        break;
    case (EXT_MEMORY | CONTIG_MEMORY | CACHED_MEMORY):
    case (EXT_MEMORY | CONTIG_MEMORY):
    case (EXT_MEMORY | CACHED_MEMORY):
    case EXT_MEMORY:
        mask = ION_HEAP_EXYNOS_CONTIG_MASK;
        flag = ION_EXYNOS_VIDEO_EXT_MASK;
        break;
    case (SECURE_MEMORY | CONTIG_MEMORY | CACHED_MEMORY):  /* SECURE */
    case (SECURE_MEMORY | CONTIG_MEMORY):
    case (SECURE_MEMORY | CACHED_MEMORY):
    case SECURE_MEMORY:
        mask = ION_HEAP_EXYNOS_CONTIG_MASK;
        flag = ION_EXYNOS_MFC_INPUT_MASK | ION_FLAG_PROTECTED;
        break;
    case (CONTIG_MEMORY | CACHED_MEMORY):  /* CONTIG */
    case CONTIG_MEMORY:
        mask = ION_HEAP_EXYNOS_CONTIG_MASK;
        flag = ION_EXYNOS_MFC_INPUT_MASK;
        break;
    case CACHED_MEMORY:  /* CACHED */
        mask = ION_HEAP_SYSTEM_MASK;
        flag = ION_FLAG_CACHED;
        break;
    default:  /* NORMAL */
        mask = ION_HEAP_SYSTEM_MASK;
        flag = ION_FLAG_CACHED;
        break;
    }

#ifdef USE_IMPROVED_BUFFER
    if (flag & ION_FLAG_CACHED)  /* use improved cache oprs */
        flag |= ION_FLAG_CACHED_NEEDS_SYNC;
#endif

    if (ion_alloc_fd(pHandle->hIONHandle, size, 0, mask, flag, (int *)&IONBuffer) < 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "ion_alloc_fd is failed(mask:%x, flag:%x)", mask, flag);
        if (memoryType == CONTIG_MEMORY) {
            /* retry at normal area */
            flag = 0;
            if (ion_alloc_fd(pHandle->hIONHandle, size, 0, mask, flag, (int *)&IONBuffer) < 0)
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "retry, ion_alloc_fd is failed(mask:%x, flag:%x)", mask, flag);
        }
    }

    if (IONBuffer < 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "ion buffer is wrong: fd = %d", IONBuffer);
        Exynos_OSAL_Free((OMX_PTR)pElement);
        goto EXIT;
    }

    if (flag & ION_FLAG_PROTECTED) {
        /* in case of DRM, do not call mmap. so set a fd instead of vaddr */
        pBuffer = INT_TO_PTR(IONBuffer);
    } else {
        pBuffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, IONBuffer, 0);
        if (pBuffer == MAP_FAILED) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "mmap is failed(size:%d)", size);
            /* free a ion_buffer */
            ion_close(IONBuffer);
            Exynos_OSAL_Free((OMX_PTR)pElement);
            pBuffer = NULL;
            goto EXIT;
        }
    }

    pElement->IONBuffer = IONBuffer;
    pElement->mapAddr = pBuffer;
    pElement->allocSize = size;
    pElement->pNextMemory = NULL;

    Exynos_OSAL_MutexLock(pHandle->hSMMutex);
    pSMList = pHandle->pAllocMemory;
    if (pSMList == NULL) {
        pHandle->pAllocMemory = pSMList = pElement;
    } else {
        pCurrentElement = pSMList;
        while (pCurrentElement->pNextMemory != NULL) {
            pCurrentElement = pCurrentElement->pNextMemory;
        }
        pCurrentElement->pNextMemory = pElement;
    }
    Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);

    mem_cnt++;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SharedMemory mem count: %d", mem_cnt);

EXIT:
    return pBuffer;
}

void Exynos_OSAL_SharedMemory_Free(OMX_HANDLETYPE handle, OMX_PTR pBuffer)
{
    EXYNOS_SHARED_MEMORY  *pHandle         = (EXYNOS_SHARED_MEMORY *)handle;
    EXYNOS_SHAREDMEM_LIST *pSMList         = NULL;
    EXYNOS_SHAREDMEM_LIST *pCurrentElement = NULL;
    EXYNOS_SHAREDMEM_LIST *pDeleteElement  = NULL;

    if (pHandle == NULL)
        goto EXIT;

    Exynos_OSAL_MutexLock(pHandle->hSMMutex);
    pSMList = pHandle->pAllocMemory;
    if (pSMList == NULL) {
        Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);
        goto EXIT;
    }

    pCurrentElement = pSMList;
    if (pSMList->mapAddr == pBuffer) {
        pDeleteElement = pSMList;
        pHandle->pAllocMemory = pSMList = pSMList->pNextMemory;
    } else {
        while ((pCurrentElement != NULL) && (((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
               (((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->mapAddr != pBuffer))
            pCurrentElement = pCurrentElement->pNextMemory;

        if ((((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
            (((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->mapAddr == pBuffer)) {
            pDeleteElement = pCurrentElement->pNextMemory;
            pCurrentElement->pNextMemory = pDeleteElement->pNextMemory;
        } else {
            Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not find SharedMemory");
            goto EXIT;
        }
    }
    Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);

    if (pDeleteElement->mapAddr != INT_TO_PTR(pDeleteElement->IONBuffer)) {
        if (munmap(pDeleteElement->mapAddr, pDeleteElement->allocSize)) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "munmap is failed");
            goto EXIT;
        }
    }

    pDeleteElement->mapAddr = NULL;
    pDeleteElement->allocSize = 0;

    if (pDeleteElement->owner) {
        /* free a ion_buffer */
        ion_close(pDeleteElement->IONBuffer);
        mem_cnt--;
    }
    pDeleteElement->IONBuffer = 0;

    Exynos_OSAL_Free(pDeleteElement);

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SharedMemory mem count: %d", mem_cnt);

EXIT:
    return;
}

#ifdef USE_DMA_BUF
OMX_PTR Exynos_OSAL_SharedMemory_Map(OMX_HANDLETYPE handle, OMX_U32 size, unsigned int ionfd)
{
    EXYNOS_SHARED_MEMORY  *pHandle = (EXYNOS_SHARED_MEMORY *)handle;
    EXYNOS_SHAREDMEM_LIST *pSMList = NULL;
    EXYNOS_SHAREDMEM_LIST *pElement = NULL;
    EXYNOS_SHAREDMEM_LIST *pCurrentElement = NULL;
    OMX_S32 IONBuffer = 0;
    OMX_PTR pBuffer = NULL;

    if (pHandle == NULL)
        goto EXIT;

    pElement = (EXYNOS_SHAREDMEM_LIST *)Exynos_OSAL_Malloc(sizeof(EXYNOS_SHAREDMEM_LIST));
    Exynos_OSAL_Memset(pElement, 0, sizeof(EXYNOS_SHAREDMEM_LIST));

    IONBuffer = (OMX_S32)ionfd;

    if (IONBuffer <= 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "ionFD(%d) is wrong", IONBuffer);
        Exynos_OSAL_Free((void*)pElement);
        goto EXIT;
    }

    pBuffer = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, IONBuffer, 0);
    if (pBuffer == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "mmap is failed(size:%d)", size);
        /* free a ion_buffer */
        ion_close(IONBuffer);
        Exynos_OSAL_Free((void*)pElement);
        goto EXIT;
    }

    pElement->IONBuffer = IONBuffer;
    pElement->mapAddr = pBuffer;
    pElement->allocSize = size;
    pElement->pNextMemory = NULL;

    Exynos_OSAL_MutexLock(pHandle->hSMMutex);
    pSMList = pHandle->pAllocMemory;
    if (pSMList == NULL) {
        pHandle->pAllocMemory = pSMList = pElement;
    } else {
        pCurrentElement = pSMList;
        while (pCurrentElement->pNextMemory != NULL) {
            pCurrentElement = pCurrentElement->pNextMemory;
        }
        pCurrentElement->pNextMemory = pElement;
    }
    Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);

    map_cnt++;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SharedMemory map count: %d", map_cnt);

EXIT:
    return pBuffer;
}

void Exynos_OSAL_SharedMemory_Unmap(OMX_HANDLETYPE handle, unsigned int ionfd)
{
    EXYNOS_SHARED_MEMORY  *pHandle = (EXYNOS_SHARED_MEMORY *)handle;
    EXYNOS_SHAREDMEM_LIST *pSMList = NULL;
    EXYNOS_SHAREDMEM_LIST *pCurrentElement = NULL;
    EXYNOS_SHAREDMEM_LIST *pDeleteElement = NULL;

    if (pHandle == NULL)
        goto EXIT;

    Exynos_OSAL_MutexLock(pHandle->hSMMutex);
    pSMList = pHandle->pAllocMemory;
    if (pSMList == NULL) {
        Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);
        goto EXIT;
    }

    pCurrentElement = pSMList;
    if (pSMList->IONBuffer == ionfd) {
        pDeleteElement = pSMList;
        pHandle->pAllocMemory = pSMList = pSMList->pNextMemory;
    } else {
        while ((pCurrentElement != NULL) && (((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
               (((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->IONBuffer != ionfd))
            pCurrentElement = pCurrentElement->pNextMemory;

        if ((((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
            (((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->IONBuffer == ionfd)) {
            pDeleteElement = pCurrentElement->pNextMemory;
            pCurrentElement->pNextMemory = pDeleteElement->pNextMemory;
        } else {
            Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not find SharedMemory");
            goto EXIT;
        }
    }
    Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);

    if (munmap(pDeleteElement->mapAddr, pDeleteElement->allocSize)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "munmap is failed");
        goto EXIT;
    }
    pDeleteElement->mapAddr = NULL;
    pDeleteElement->allocSize = 0;
    pDeleteElement->IONBuffer = 0;

    Exynos_OSAL_Free(pDeleteElement);

    map_cnt--;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SharedMemory map count: %d", map_cnt);

EXIT:
    return;
}
#endif

int Exynos_OSAL_SharedMemory_VirtToION(OMX_HANDLETYPE handle, OMX_PTR pBuffer)
{
    EXYNOS_SHARED_MEMORY  *pHandle         = (EXYNOS_SHARED_MEMORY *)handle;
    EXYNOS_SHAREDMEM_LIST *pSMList         = NULL;
    EXYNOS_SHAREDMEM_LIST *pCurrentElement = NULL;
    EXYNOS_SHAREDMEM_LIST *pFindElement    = NULL;
    int ion_addr = 0;
    if (pHandle == NULL || pBuffer == NULL)
        goto EXIT;

    Exynos_OSAL_MutexLock(pHandle->hSMMutex);
    pSMList = pHandle->pAllocMemory;
    if (pSMList == NULL) {
        Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);
        goto EXIT;
    }

    pCurrentElement = pSMList;
    if (pSMList->mapAddr == pBuffer) {
        pFindElement = pSMList;
    } else {
        while ((pCurrentElement != NULL) && (((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
               (((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->mapAddr != pBuffer))
            pCurrentElement = pCurrentElement->pNextMemory;

        if ((((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
            (((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->mapAddr == pBuffer)) {
            pFindElement = pCurrentElement->pNextMemory;
        } else {
            Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "Can not find SharedMemory");
            goto EXIT;
        }
    }
    Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);

    ion_addr = pFindElement->IONBuffer;

EXIT:
    return ion_addr;
}

OMX_PTR Exynos_OSAL_SharedMemory_IONToVirt(OMX_HANDLETYPE handle, int ion_addr)
{
    EXYNOS_SHARED_MEMORY  *pHandle         = (EXYNOS_SHARED_MEMORY *)handle;
    EXYNOS_SHAREDMEM_LIST *pSMList         = NULL;
    EXYNOS_SHAREDMEM_LIST *pCurrentElement = NULL;
    EXYNOS_SHAREDMEM_LIST *pFindElement    = NULL;
    OMX_PTR pBuffer = NULL;
    if (pHandle == NULL || ion_addr == 0)
        goto EXIT;

    Exynos_OSAL_MutexLock(pHandle->hSMMutex);
    pSMList = pHandle->pAllocMemory;
    if (pSMList == NULL) {
        Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);
        goto EXIT;
    }

    pCurrentElement = pSMList;
    if (pSMList->IONBuffer == (OMX_U32)ion_addr) {
        pFindElement = pSMList;
    } else {
        while ((pCurrentElement != NULL) && (((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
               (((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->IONBuffer != (OMX_U32)ion_addr))
            pCurrentElement = pCurrentElement->pNextMemory;

        if ((((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory)) != NULL) &&
            (((EXYNOS_SHAREDMEM_LIST *)(pCurrentElement->pNextMemory))->IONBuffer == (OMX_U32)ion_addr)) {
            pFindElement = pCurrentElement->pNextMemory;
        } else {
            Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "Can not find SharedMemory");
            goto EXIT;
        }
    }
    Exynos_OSAL_MutexUnlock(pHandle->hSMMutex);

    pBuffer = pFindElement->mapAddr;

EXIT:
    return pBuffer;
}
