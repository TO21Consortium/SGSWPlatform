/*
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
 * @file        Exynos_OSAL_Android.cpp
 * @brief
 * @author      Seungbeom Kim (sbcrux.kim@samsung.com)
 * @author      Hyeyeon Chung (hyeon.chung@samsung.com)
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @author      Jinsung Yang (jsgood.yang@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>

#include <system/window.h>
#include <cutils/properties.h>
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>
#include <media/hardware/OMXPluginBase.h>
#include <media/hardware/HardwareAPI.h>
#include <hardware/hardware.h>
#include <media/hardware/MetadataBufferType.h>
#ifdef USE_DMA_BUF
#include <gralloc_priv.h>
#endif

#include <ion/ion.h>
#include "exynos_ion.h"

#include "Exynos_OSAL_Mutex.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OMX_Macros.h"
#include "Exynos_OSAL_Android.h"
#include "Exynos_OSAL_ETC.h"
#include "exynos_format.h"

#include "ExynosVideoApi.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "Exynos_OSAL_Android"
//#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

using namespace android;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _EXYNOS_OMX_SHARED_BUFFER {
    OMX_S32 bufferFd;
    OMX_S32 bufferFd1;
    OMX_S32 bufferFd2;

    ion_user_handle_t  ionHandle;
    ion_user_handle_t  ionHandle1;
    ion_user_handle_t  ionHandle2;

    OMX_U32 cnt;
} EXYNOS_OMX_SHARED_BUFFER;

typedef struct _EXYNOS_OMX_REF_HANDLE {
    OMX_HANDLETYPE           hMutex;
    OMX_PTR                  pGrallocModule;
    EXYNOS_OMX_SHARED_BUFFER SharedBuffer[MAX_BUFFER_REF];
} EXYNOS_OMX_REF_HANDLE;

static int lockCnt = 0;

int getIonFd(gralloc_module_t const *module)
{
    private_module_t* m = const_cast<private_module_t*>(reinterpret_cast<const private_module_t*>(module));
    return m->ionfd;
}

OMX_ERRORTYPE setBufferProcessTypeForDecoder(EXYNOS_OMX_BASEPORT *pExynosPort)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (pExynosPort == NULL) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

#ifdef USE_ANB_OUTBUF_SHARE
    if (pExynosPort->bufferProcessType & BUFFER_ANBSHARE) {
        int i;
        if (pExynosPort->supportFormat == NULL) {
            ret = OMX_ErrorUndefined;
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: supported format is empty", __FUNCTION__);
            goto EXIT;
        }

        pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        /* check to support NV12T format */
        for (i = 0; pExynosPort->supportFormat[i] != OMX_COLOR_FormatUnused; i++) {
            /* prefer to use NV12T */
            if (pExynosPort->supportFormat[i] == (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV12Tiled) {
                pExynosPort->portDefinition.format.video.eColorFormat = (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV12Tiled;
                break;
            }
        }
        pExynosPort->bufferProcessType = BUFFER_SHARE;

        Exynos_OSAL_Log(EXYNOS_LOG_INFO, "output buffer sharing mode is on (%x)", pExynosPort->portDefinition.format.video.eColorFormat);
    } else
#endif
           if (pExynosPort->bufferProcessType & BUFFER_COPY) {
        if (pExynosPort->bStoreMetaData == OMX_TRUE) {
            ret = OMX_ErrorUndefined;
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: bufferProcessType is invalid", __FUNCTION__);
            goto EXIT;
        }

        pExynosPort->bufferProcessType = BUFFER_COPY;
        pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    }

EXIT:
    return ret;
}

void resetBufferProcessTypeForDecoder(EXYNOS_OMX_BASEPORT *pExynosPort)
{
    int i;

    if (pExynosPort == NULL)
        return;

    pExynosPort->bufferProcessType = (EXYNOS_OMX_BUFFERPROCESS_TYPE)(BUFFER_COPY | BUFFER_ANBSHARE);
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;

    return;
}

OMX_ERRORTYPE Exynos_OSAL_LockANBHandle(
    OMX_IN OMX_PTR handle,
    OMX_IN OMX_U32 width,
    OMX_IN OMX_U32 height,
    OMX_IN OMX_COLOR_FORMATTYPE format,
    OMX_OUT OMX_U32 *pStride,
    OMX_OUT OMX_PTR planes)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;
#ifdef USE_DMA_BUF
    private_handle_t *priv_hnd = (private_handle_t *) bufferHandle;
#endif
    Rect bounds((uint32_t)width, (uint32_t)height);
    ExynosVideoPlane *vplanes = (ExynosVideoPlane *) planes;
    void *vaddr[MAX_BUFFER_PLANE] = {NULL, };

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: handle: 0x%x, format = %x", __FUNCTION__, handle, priv_hnd->format);

    int usage = 0;
    switch ((int)format) {
    case OMX_COLOR_FormatYUV420Planar:
    case OMX_COLOR_FormatYUV420SemiPlanar:
    case OMX_SEC_COLOR_FormatYUV420SemiPlanarInterlace:
    case OMX_SEC_COLOR_Format10bitYUV420SemiPlanar:
    case OMX_SEC_COLOR_FormatNV12Tiled:
        usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
        break;
#ifdef USE_ANDROIDOPAQUE
    case OMX_COLOR_FormatAndroidOpaque:
    {
        OMX_COLOR_FORMATTYPE formatType;
        formatType = Exynos_OSAL_GetANBColorFormat(priv_hnd);
        if ((formatType == OMX_COLOR_FormatYUV420SemiPlanar) ||
            (formatType == (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV12Tiled))
            usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
        else
            usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN | GRALLOC_USAGE_HW_VIDEO_ENCODER;
    }
        break;
#endif
    default:
        usage = GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN;
        break;
    }

    if (mapper.lock(bufferHandle, usage, bounds, vaddr) != 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: mapper.lock() fail", __func__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    lockCnt++;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: lockCnt:%d", __func__, lockCnt);

#ifdef USE_DMA_BUF
    vplanes[0].fd = priv_hnd->fd;
    vplanes[0].offset = 0;
    vplanes[1].fd = priv_hnd->fd1;
    vplanes[1].offset = 0;
    vplanes[2].fd = priv_hnd->fd2;
    vplanes[2].offset = 0;
#endif

    if (priv_hnd->flags & GRALLOC_USAGE_PROTECTED) {
        /* in case of DRM,  vaddrs are invalid */
        vplanes[0].addr = INT_TO_PTR(priv_hnd->fd);
        vplanes[1].addr = INT_TO_PTR(priv_hnd->fd1);
        vplanes[2].addr = INT_TO_PTR(priv_hnd->fd2);
    } else {
        vplanes[0].addr = vaddr[0];
        vplanes[1].addr = vaddr[1];
        vplanes[2].addr = vaddr[2];
    }

    if ((priv_hnd->format == HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV) &&
        (vaddr[2] != NULL))
        vplanes[2].addr = vaddr[2];

    *pStride = (OMX_U32)priv_hnd->stride;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: buffer locked: 0x%x", __func__, *vaddr);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_UnlockANBHandle(OMX_IN OMX_PTR handle)
{
    FunctionIn();

    OMX_ERRORTYPE ret = OMX_ErrorNone;
    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
    buffer_handle_t bufferHandle = (buffer_handle_t) handle;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: handle: 0x%x", __func__, handle);

    if (mapper.unlock(bufferHandle) != 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: mapper.unlock() fail", __func__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }
    lockCnt--;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: lockCnt:%d", __func__, lockCnt);

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: buffer unlocked: 0x%x", __func__, handle);

EXIT:
    FunctionOut();

    return ret;
}

OMX_COLOR_FORMATTYPE Exynos_OSAL_GetANBColorFormat(OMX_IN OMX_PTR handle)
{
    FunctionIn();

    OMX_COLOR_FORMATTYPE ret = OMX_COLOR_FormatUnused;
    private_handle_t *priv_hnd = (private_handle_t *) handle;

    ret = Exynos_OSAL_HAL2OMXColorFormat(priv_hnd->format);
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "ColorFormat: 0x%x", ret);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_LockMetaData(
    OMX_IN OMX_PTR pBuffer,
    OMX_IN OMX_U32 width,
    OMX_IN OMX_U32 height,
    OMX_IN OMX_COLOR_FORMATTYPE format,
    OMX_OUT OMX_U32 *pStride,
    OMX_OUT OMX_PTR planes)
{
    FunctionIn();

    OMX_ERRORTYPE   ret     = OMX_ErrorNone;
    OMX_PTR         pBuf    = NULL;

    ret = Exynos_OSAL_GetInfoFromMetaData((OMX_BYTE)pBuffer, &pBuf);
    if (ret == OMX_ErrorNone)
        ret = Exynos_OSAL_LockANBHandle(pBuf, width, height, format, pStride, planes);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_UnlockMetaData(OMX_IN OMX_PTR pBuffer)
{
    FunctionIn();

    OMX_ERRORTYPE   ret     = OMX_ErrorNone;
    OMX_PTR         pBuf    = NULL;

    ret = Exynos_OSAL_GetInfoFromMetaData((OMX_BYTE)pBuffer, &pBuf);
    if (ret == OMX_ErrorNone)
        ret = Exynos_OSAL_UnlockANBHandle(pBuf);

EXIT:
    FunctionOut();

    return ret;
}

OMX_HANDLETYPE Exynos_OSAL_RefANB_Create()
{
    OMX_ERRORTYPE            ret    = OMX_ErrorNone;
    EXYNOS_OMX_REF_HANDLE   *phREF  = NULL;
    gralloc_module_t        *module = NULL;

    int i = 0;

    FunctionIn();

    phREF = (EXYNOS_OMX_REF_HANDLE *)Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_REF_HANDLE));
    if (phREF == NULL)
        goto EXIT;

    Exynos_OSAL_Memset(phREF, 0, sizeof(EXYNOS_OMX_REF_HANDLE));
    for (i = 0; i < MAX_BUFFER_REF; i++) {
        phREF->SharedBuffer[i].bufferFd  = -1;
        phREF->SharedBuffer[i].bufferFd1 = -1;
        phREF->SharedBuffer[i].bufferFd2 = -1;
    }
    if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **)&module) != 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: hw_get_module(GRALLOC_HARDWARE_MODULE_ID) fail", __func__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    phREF->pGrallocModule = (OMX_PTR)module;

    ret = Exynos_OSAL_MutexCreate(&phREF->hMutex);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Free(phREF);
        phREF = NULL;
    }

EXIT:
    FunctionOut();

    return ((OMX_HANDLETYPE)phREF);
}

OMX_ERRORTYPE Exynos_OSAL_RefANB_Reset(OMX_HANDLETYPE hREF)
{
    OMX_ERRORTYPE            ret    = OMX_ErrorNone;
    EXYNOS_OMX_REF_HANDLE   *phREF  = (EXYNOS_OMX_REF_HANDLE *)hREF;
    gralloc_module_t        *module = NULL;

    int i = 0;

    FunctionIn();

    if (phREF == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    module = (gralloc_module_t *)phREF->pGrallocModule;

    Exynos_OSAL_MutexLock(phREF->hMutex);

    for (i = 0; i < MAX_BUFFER_REF; i++) {
        if (phREF->SharedBuffer[i].bufferFd > -1) {
            while(phREF->SharedBuffer[i].cnt > 0) {
                if (phREF->SharedBuffer[i].ionHandle != -1)
                    ion_free(getIonFd(module), phREF->SharedBuffer[i].ionHandle);
                if (phREF->SharedBuffer[i].ionHandle1 != -1)
                    ion_free(getIonFd(module), phREF->SharedBuffer[i].ionHandle1);
                if (phREF->SharedBuffer[i].ionHandle2 != -1)
                    ion_free(getIonFd(module), phREF->SharedBuffer[i].ionHandle2);
                phREF->SharedBuffer[i].cnt--;
            }
            phREF->SharedBuffer[i].bufferFd    = -1;
            phREF->SharedBuffer[i].bufferFd1   = -1;
            phREF->SharedBuffer[i].bufferFd2   = -1;
            phREF->SharedBuffer[i].ionHandle   = -1;
            phREF->SharedBuffer[i].ionHandle1  = -1;
            phREF->SharedBuffer[i].ionHandle2  = -1;
        }
    }
    Exynos_OSAL_MutexUnlock(phREF->hMutex);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_RefANB_Terminate(OMX_HANDLETYPE hREF)
{
    OMX_ERRORTYPE            ret    = OMX_ErrorNone;
    EXYNOS_OMX_REF_HANDLE   *phREF  = (EXYNOS_OMX_REF_HANDLE *)hREF;

    FunctionIn();

    if (phREF == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    Exynos_OSAL_RefANB_Reset(phREF);

    phREF->pGrallocModule = NULL;

    ret = Exynos_OSAL_MutexTerminate(phREF->hMutex);
    if (ret != OMX_ErrorNone)
        goto EXIT;

    Exynos_OSAL_Free(phREF);
    phREF = NULL;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_RefANB_Increase(
    OMX_HANDLETYPE  hREF,
    OMX_PTR         pBuffer,
    PLANE_TYPE      ePlaneType)
{
    OMX_ERRORTYPE            ret            = OMX_ErrorNone;
    EXYNOS_OMX_REF_HANDLE   *phREF          = (EXYNOS_OMX_REF_HANDLE *)hREF;
    OMX_COLOR_FORMATTYPE     eColorFormat   = OMX_COLOR_FormatUnused;

    buffer_handle_t      bufferHandle   = (buffer_handle_t)pBuffer;
    private_handle_t    *priv_hnd       = (private_handle_t *)bufferHandle;
    gralloc_module_t    *module         = NULL;

    ion_user_handle_t ionHandle  = -1;
    ion_user_handle_t ionHandle1 = -1;
    ion_user_handle_t ionHandle2 = -1;
    int i, nPlaneCnt;

    FunctionIn();

    if (phREF == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    eColorFormat = Exynos_OSAL_HAL2OMXColorFormat(priv_hnd->format);
    if (eColorFormat == OMX_COLOR_FormatUnused) {
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    nPlaneCnt = Exynos_OSAL_GetPlaneCount(eColorFormat, ePlaneType);
    module = (gralloc_module_t *)phREF->pGrallocModule;

    Exynos_OSAL_MutexLock(phREF->hMutex);

    if ((priv_hnd->fd >= 0) &&
        (nPlaneCnt >= 1)) {
        if (ion_import(getIonFd(module), priv_hnd->fd, &ionHandle) < 0) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "ion_import is failed(client:%d, fd:%d)", getIonFd(module), priv_hnd->fd);
            ionHandle = -1;
        }
    }

    if ((priv_hnd->fd1 >= 0) &&
        (nPlaneCnt >= 2)) {
        if (ion_import(getIonFd(module), priv_hnd->fd1, &ionHandle1) < 0) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "ion_import is failed(client:%d, fd1:%d)", getIonFd(module), priv_hnd->fd1);
            ionHandle1 = -1;
        }
    }

    if ((priv_hnd->fd2 >= 0) &&
        (nPlaneCnt == 3)) {
        if (ion_import(getIonFd(module), priv_hnd->fd2, &ionHandle2) < 0) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "ion_import is failed(client:%d, fd2:%d)", getIonFd(module), priv_hnd->fd2);
            ionHandle2 = -1;
        }
    }

    for (i = 0; i < MAX_BUFFER_REF; i++) {
        if (phREF->SharedBuffer[i].bufferFd == priv_hnd->fd) {
            phREF->SharedBuffer[i].cnt++;
            break;
        }
    }

    if (i >=  MAX_BUFFER_REF) {
        for (i = 0; i < MAX_BUFFER_REF; i++) {
            if (phREF->SharedBuffer[i].bufferFd == -1) {
                phREF->SharedBuffer[i].bufferFd    = priv_hnd->fd;
                phREF->SharedBuffer[i].bufferFd1   = priv_hnd->fd1;
                phREF->SharedBuffer[i].bufferFd2   = priv_hnd->fd2;
                phREF->SharedBuffer[i].ionHandle  = ionHandle;
                phREF->SharedBuffer[i].ionHandle1 = ionHandle1;
                phREF->SharedBuffer[i].ionHandle2 = ionHandle2;
                phREF->SharedBuffer[i].cnt++;
                break;
            }
        }
    }

    if (i >=  MAX_BUFFER_REF) {
        ret = OMX_ErrorUndefined;
        Exynos_OSAL_MutexUnlock(phREF->hMutex);
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "inc fd:%d cnt:%d", phREF->SharedBuffer[i].bufferFd, phREF->SharedBuffer[i].cnt);
    Exynos_OSAL_MutexUnlock(phREF->hMutex);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_RefANB_Decrease(OMX_HANDLETYPE hREF, OMX_S32 bufferFd)
{

    OMX_ERRORTYPE            ret    = OMX_ErrorNone;
    EXYNOS_OMX_REF_HANDLE   *phREF  = (EXYNOS_OMX_REF_HANDLE *)hREF;
    gralloc_module_t        *module = NULL;

    int i;

    FunctionIn();

    if ((phREF == NULL) || (bufferFd < 0)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    module = (gralloc_module_t *)phREF->pGrallocModule;

    Exynos_OSAL_MutexLock(phREF->hMutex);

    for (i = 0; i < MAX_BUFFER_REF; i++) {
        if (phREF->SharedBuffer[i].bufferFd == bufferFd) {
            if (phREF->SharedBuffer[i].ionHandle != -1)
                ion_free(getIonFd(module), phREF->SharedBuffer[i].ionHandle);
            if (phREF->SharedBuffer[i].ionHandle1 != -1)
                ion_free(getIonFd(module), phREF->SharedBuffer[i].ionHandle1);
            if (phREF->SharedBuffer[i].ionHandle2 != -1)
                ion_free(getIonFd(module), phREF->SharedBuffer[i].ionHandle2);
            phREF->SharedBuffer[i].cnt--;

            if (phREF->SharedBuffer[i].cnt == 0) {
                phREF->SharedBuffer[i].bufferFd    = -1;
                phREF->SharedBuffer[i].bufferFd1   = -1;
                phREF->SharedBuffer[i].bufferFd2   = -1;
                phREF->SharedBuffer[i].ionHandle   = -1;
                phREF->SharedBuffer[i].ionHandle1  = -1;
                phREF->SharedBuffer[i].ionHandle2  = -1;
            }
            break;
        }
    }

    if (i >=  MAX_BUFFER_REF) {
        ret = OMX_ErrorUndefined;
        Exynos_OSAL_MutexUnlock(phREF->hMutex);
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "dec fd:%d cnt:%d", phREF->SharedBuffer[i].bufferFd, phREF->SharedBuffer[i].cnt);
    Exynos_OSAL_MutexUnlock(phREF->hMutex);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE useAndroidNativeBuffer(
    EXYNOS_OMX_BASEPORT   *pExynosPort,
    OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_U32                nPortIndex,
    OMX_PTR                pAppPrivate,
    OMX_U32                nSizeBytes,
    OMX_U8                *pBuffer)
{
    OMX_ERRORTYPE         ret = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *temp_bufferHeader = NULL;
    unsigned int          i = 0;
    OMX_U32               width, height;
    OMX_U32               stride;
    ExynosVideoPlane      planes[MAX_BUFFER_PLANE];

    FunctionIn();

    if (pExynosPort == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosPort->portState != OMX_StateIdle) {
        ret = OMX_ErrorIncorrectStateOperation;
        goto EXIT;
    }
    if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    temp_bufferHeader = (OMX_BUFFERHEADERTYPE *)Exynos_OSAL_Malloc(sizeof(OMX_BUFFERHEADERTYPE));
    if (temp_bufferHeader == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    Exynos_OSAL_Memset(temp_bufferHeader, 0, sizeof(OMX_BUFFERHEADERTYPE));

    for (i = 0; i < pExynosPort->portDefinition.nBufferCountActual; i++) {
        if (pExynosPort->bufferStateAllocate[i] == BUFFER_STATE_FREE) {
            pExynosPort->extendBufferHeader[i].OMXBufferHeader = temp_bufferHeader;
            pExynosPort->bufferStateAllocate[i] = (BUFFER_STATE_ASSIGNED | HEADER_STATE_ALLOCATED);
            INIT_SET_SIZE_VERSION(temp_bufferHeader, OMX_BUFFERHEADERTYPE);
            if (pExynosPort->eANBType == NATIVE_GRAPHIC_BUFFER1) {
                android_native_buffer_t *pANB = (android_native_buffer_t *)pBuffer;
                temp_bufferHeader->pBuffer = (OMX_U8 *)pANB->handle;
            } else {
                temp_bufferHeader->pBuffer = (OMX_U8 *)pBuffer;
            }
            temp_bufferHeader->nAllocLen      = nSizeBytes;
            temp_bufferHeader->pAppPrivate    = pAppPrivate;
            if (nPortIndex == INPUT_PORT_INDEX)
                temp_bufferHeader->nInputPortIndex = INPUT_PORT_INDEX;
            else
                temp_bufferHeader->nOutputPortIndex = OUTPUT_PORT_INDEX;

            width = pExynosPort->portDefinition.format.video.nFrameWidth;
            height = pExynosPort->portDefinition.format.video.nFrameHeight;
            Exynos_OSAL_LockANBHandle(temp_bufferHeader->pBuffer, width, height,
                                pExynosPort->portDefinition.format.video.eColorFormat,
                                &stride, planes);
#ifdef USE_DMA_BUF
            pExynosPort->extendBufferHeader[i].buf_fd[0] = planes[0].fd;
            pExynosPort->extendBufferHeader[i].buf_fd[1] = planes[1].fd;
            pExynosPort->extendBufferHeader[i].buf_fd[2] = planes[2].fd;
#endif
            pExynosPort->extendBufferHeader[i].pYUVBuf[0] = planes[0].addr;
            pExynosPort->extendBufferHeader[i].pYUVBuf[1] = planes[1].addr;
            pExynosPort->extendBufferHeader[i].pYUVBuf[2] = planes[2].addr;
            Exynos_OSAL_UnlockANBHandle(temp_bufferHeader->pBuffer);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "useAndroidNativeBuffer: buf %d pYUVBuf[0]:0x%x , pYUVBuf[1]:0x%x ",
                            i, pExynosPort->extendBufferHeader[i].pYUVBuf[0],
                            pExynosPort->extendBufferHeader[i].pYUVBuf[1]);

            pExynosPort->assignedBufferNum++;
            if (pExynosPort->assignedBufferNum == (OMX_S32)pExynosPort->portDefinition.nBufferCountActual) {
                pExynosPort->portDefinition.bPopulated = OMX_TRUE;
                /* Exynos_OSAL_MutexLock(pExynosComponent->compMutex); */
                Exynos_OSAL_SemaphorePost(pExynosPort->loadedResource);
                /* Exynos_OSAL_MutexUnlock(pExynosComponent->compMutex); */
            }
            *ppBufferHdr = temp_bufferHeader;
            ret = OMX_ErrorNone;

            goto EXIT;
        }
    }

    Exynos_OSAL_Free(temp_bufferHeader);
    ret = OMX_ErrorInsufficientResources;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_GetAndroidParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_INOUT OMX_PTR     ComponentParameterStructure)
{
    OMX_ERRORTYPE                ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE           *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent   = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone)
        goto EXIT;

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch ((int)nIndex) {
#ifdef USE_ANB
    case OMX_IndexParamGetAndroidNativeBuffer:
    {
        GetAndroidNativeBufferUsageParams *pANBParams = (GetAndroidNativeBufferUsageParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: OMX_IndexParamGetAndroidNativeBuffer", __func__);

        ret = Exynos_OMX_Check_SizeVersion(pANBParams, sizeof(GetAndroidNativeBufferUsageParams));
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OMX_Check_SizeVersion(GetAndroidNativeBufferUsageParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        /* NOTE: OMX_IndexParamGetAndroidNativeBuffer returns original 'nUsage' without any
         * modifications since currently not defined what the 'nUsage' is for.
         */
        pANBParams->nUsage |= (GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP);
#if defined(USE_IMPROVED_BUFFER) && !defined(USE_CSC_HW) && !defined(USE_NON_CACHED_GRAPHICBUFFER)
        pANBParams->nUsage |= (GRALLOC_USAGE_SW_READ_OFTEN | GRALLOC_USAGE_SW_WRITE_OFTEN);
#endif
#if defined(USE_MFC5X_ALIGNMENT)
        if ((pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].bufferProcessType & BUFFER_SHARE) &&
            (pExynosComponent->pExynosPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat == OMX_VIDEO_CodingAVC)) {
            pANBParams->nUsage |= GRALLOC_USAGE_PRIVATE_0;
        }
#endif
        if (pExynosComponent->codecType == HW_VIDEO_DEC_SECURE_CODEC) {  // secure decoder component
            /* never knows actual image size before parsing a header info
             * could not guarantee DRC(Dynamic Resolution Chnage) case */
            EXYNOS_OMX_BASEPORT *pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
            unsigned int HD_SIZE = 1280 * 720 * 3 / 2;  /* 720p */

            if ((pExynosPort->portDefinition.format.video.nFrameWidth *
                 pExynosPort->portDefinition.format.video.nFrameHeight * 3 / 2) > HD_SIZE) {  /* over than 720p */
                pANBParams->nUsage |= GRALLOC_USAGE_PROTECTED_DPB;  /* try to use a CMA area */
            }
        }
    }
        break;
    case OMX_IndexParamConsumerUsageBits:
    {
        OMX_U32 *pUsageBits = (OMX_U32 *)ComponentParameterStructure;

        switch ((int)pExynosComponent->codecType) {
        case HW_VIDEO_ENC_CODEC:
            (*pUsageBits) = GRALLOC_USAGE_HW_VIDEO_ENCODER;
            break;
        case HW_VIDEO_ENC_SECURE_CODEC:
            (*pUsageBits) = GRALLOC_USAGE_HW_VIDEO_ENCODER | GRALLOC_USAGE_PROTECTED;
            break;
        default:
            (*pUsageBits) = 0;
            ret = OMX_ErrorUnsupportedIndex;
            break;
        }
    }
        break;
#endif
    default:
    {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Unsupported index (%d)", __func__, nIndex);
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_SetAndroidParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        ComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (ComponentParameterStructure == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    switch ((int)nIndex) {
#ifdef USE_ANB
    case OMX_IndexParamEnableAndroidBuffers:
    {
        EnableAndroidNativeBuffersParams *pANBParams = (EnableAndroidNativeBuffersParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;
        EXYNOS_OMX_BASEPORT *pExynosPort = NULL;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: OMX_IndexParamEnableAndroidNativeBuffers", __func__);

        ret = Exynos_OMX_Check_SizeVersion(pANBParams, sizeof(EnableAndroidNativeBuffersParams));
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OMX_Check_SizeVersion(EnableAndroidNativeBuffersParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pExynosPort = &pExynosComponent->pExynosPort[portIndex];
        if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if (pExynosPort->bStoreMetaData != OMX_TRUE) {
            if (portIndex == OUTPUT_PORT_INDEX) {
                if (pANBParams->enable == OMX_TRUE) {
                    ret = setBufferProcessTypeForDecoder(pExynosPort);
                    if (ret != OMX_ErrorNone)
                        goto EXIT;
                } else if ((pANBParams->enable == OMX_FALSE) &&
                           (pExynosPort->bIsANBEnabled == OMX_TRUE)) {
                    /* reset to initialized value */
                    resetBufferProcessTypeForDecoder(pExynosPort);
                }
            }

            pExynosPort->bIsANBEnabled = pANBParams->enable;
        }
    }
        break;

    case OMX_IndexParamUseAndroidNativeBuffer:
    {
        UseAndroidNativeBufferParams *pANBParams = (UseAndroidNativeBufferParams *) ComponentParameterStructure;
        OMX_U32 portIndex = pANBParams->nPortIndex;
        EXYNOS_OMX_BASEPORT *pExynosPort = NULL;
        android_native_buffer_t *pANB;
        OMX_U32 nSizeBytes;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: OMX_IndexParamUseAndroidNativeBuffer, portIndex: %d", __func__, portIndex);

        ret = Exynos_OMX_Check_SizeVersion(pANBParams, sizeof(UseAndroidNativeBufferParams));
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OMX_Check_SizeVersion(UseAndroidNativeBufferParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pExynosPort = &pExynosComponent->pExynosPort[portIndex];
        if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if (pExynosPort->portState != OMX_StateIdle) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Port state should be IDLE", __func__);
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        pANB = pANBParams->nativeBuffer.get();

        /* MALI alignment restriction */
        nSizeBytes = ALIGN(pANB->width, 16) * ALIGN(pANB->height, 16);
        nSizeBytes += ALIGN(pANB->width / 2, 16) * ALIGN(pANB->height / 2, 16) * 2;

        ret = useAndroidNativeBuffer(pExynosPort,
                                     pANBParams->bufferHeader,
                                     pANBParams->nPortIndex,
                                     pANBParams->pAppPrivate,
                                     nSizeBytes,
                                     (OMX_U8 *) pANB);
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: useAndroidNativeBuffer is failed: err=0x%x", __func__,ret);
            goto EXIT;
        }
    }
        break;
#endif

#ifdef USE_STOREMETADATA
    case OMX_IndexParamStoreMetaDataBuffer:
    {
        StoreMetaDataInBuffersParams *pMetaParams = (StoreMetaDataInBuffersParams *)ComponentParameterStructure;
        OMX_U32 portIndex = pMetaParams->nPortIndex;
        EXYNOS_OMX_BASEPORT *pExynosPort = NULL;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: OMX_IndexParamStoreMetaDataBuffer", __func__);

        ret = Exynos_OMX_Check_SizeVersion(pMetaParams, sizeof(StoreMetaDataInBuffersParams));
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OMX_Check_SizeVersion(StoreMetaDataInBuffersParams) is failed", __func__);
            goto EXIT;
        }

        if (portIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if ((pExynosComponent->codecType == HW_VIDEO_DEC_CODEC) ||
            (pExynosComponent->codecType == HW_VIDEO_DEC_SECURE_CODEC)) {
            EXYNOS_OMX_BASEPORT *pOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
            if ((portIndex == INPUT_PORT_INDEX) ||
                (pOutputPort->bDynamicDPBMode == OMX_FALSE)) {
                ret = OMX_ErrorUndefined;
                goto EXIT;
            }
        }

        pExynosPort = &pExynosComponent->pExynosPort[portIndex];
        if (CHECK_PORT_TUNNELED(pExynosPort) && CHECK_PORT_BUFFER_SUPPLIER(pExynosPort)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if (((pExynosComponent->codecType == HW_VIDEO_DEC_CODEC) ||
             (pExynosComponent->codecType == HW_VIDEO_DEC_SECURE_CODEC)) &&
            (portIndex == OUTPUT_PORT_INDEX)) {
            if (pMetaParams->bStoreMetaData == OMX_TRUE) {
                pExynosPort->bStoreMetaData = pMetaParams->bStoreMetaData;
                ret = setBufferProcessTypeForDecoder(pExynosPort);
            } else if ((pMetaParams->bStoreMetaData == OMX_FALSE) &&
                       (pExynosPort->bStoreMetaData == OMX_TRUE)) {
                /* reset to initialized value */
                resetBufferProcessTypeForDecoder(pExynosPort);
            }
        }

        pExynosPort->bStoreMetaData = pMetaParams->bStoreMetaData;
    }
        break;
#endif

    default:
    {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Unsupported index (%d)", __func__, nIndex);
        ret = OMX_ErrorUnsupportedIndex;
        goto EXIT;
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_GetInfoFromMetaData(OMX_IN OMX_BYTE pBuffer,
                                           OMX_OUT OMX_PTR *ppBuf)
{
    OMX_ERRORTYPE      ret = OMX_ErrorNone;
    MetadataBufferType type;

    FunctionIn();

/*
 * meta data contains the following data format.
 * payload depends on the MetadataBufferType
 * ---------------------------------------------------------------
 * | MetadataBufferType               |         payload          |
 * ---------------------------------------------------------------
 *
 * If MetadataBufferType is kMetadataBufferTypeCameraSource, then
 * ---------------------------------------------------------------
 * | kMetadataBufferTypeCameraSource  | addr. of Y | addr. of UV |
 * ---------------------------------------------------------------
 *
 * If MetadataBufferType is kMetadataBufferTypeGrallocSource, then
 * ---------------------------------------------------------------
 * | kMetadataBufferTypeGrallocSource |     buffer_handle_t      |
 * ---------------------------------------------------------------
 */

    /* MetadataBufferType */
    Exynos_OSAL_Memcpy(&type, (MetadataBufferType *)pBuffer, sizeof(MetadataBufferType));

    switch ((int)type) {
    case kMetadataBufferTypeCameraSource:
    {
        void *pAddress = NULL;

        /* Address. of Y */
        Exynos_OSAL_Memcpy(&pAddress, pBuffer + sizeof(MetadataBufferType), sizeof(void *));
        ppBuf[0] = (void *)pAddress;

        /* Address. of CbCr */
        Exynos_OSAL_Memcpy(&pAddress, pBuffer + sizeof(MetadataBufferType) + sizeof(void *), sizeof(void *));
        ppBuf[1] = (void *)pAddress;

        if ((ppBuf[0] == NULL) || (ppBuf[1] == NULL))
            ret = OMX_ErrorBadParameter;
    }
        break;
    case kMetadataBufferTypeGrallocSource:
    {
        buffer_handle_t    pBufHandle;

        /* buffer_handle_t */
        Exynos_OSAL_Memcpy(&pBufHandle, pBuffer + sizeof(MetadataBufferType), sizeof(buffer_handle_t));
        ppBuf[0] = (OMX_PTR)pBufHandle;

        if (ppBuf[0] == NULL) {
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "GrallocBuffer's pHandle is NULL");
            ret = OMX_ErrorBadParameter;
        }
    }
        break;
    default:
    {
        ret = OMX_ErrorBadParameter;
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_GetBufferFdFromMetaData(
    OMX_IN  OMX_BYTE pBuffer,
    OMX_OUT OMX_PTR *ppBuf)
{
    OMX_ERRORTYPE      ret  = OMX_ErrorNone;
    MetadataBufferType type = kMetadataBufferTypeInvalid;

    FunctionIn();

    /* MetadataBufferType */
    Exynos_OSAL_Memcpy(&type, (MetadataBufferType *)pBuffer, sizeof(MetadataBufferType));

    switch ((int)type) {
    case kMetadataBufferTypeCameraSource:
    {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d - Unsupport Type of MetadataBuffer", __FUNCTION__, __LINE__);
    }
        break;
    case kMetadataBufferTypeGrallocSource:
    {
        VideoGrallocMetadata    *pMetaData      = (VideoGrallocMetadata *)pBuffer;
        buffer_handle_t          bufferHandle   = (buffer_handle_t)pMetaData->pHandle;
        native_handle_t         *pNativeHandle  = (native_handle_t *)bufferHandle;

        /* ION FD. */
        ppBuf[0] = INT_TO_PTR(pNativeHandle->data[0]);
    }
        break;
    default:
    {
        ret = OMX_ErrorBadParameter;
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_SetDataLengthToMetaData(
    OMX_IN OMX_BYTE pBuffer,
    OMX_IN OMX_U32  dataLength)
{
    OMX_ERRORTYPE      ret  = OMX_ErrorNone;
    MetadataBufferType type = kMetadataBufferTypeInvalid;

    FunctionIn();

    /* MetadataBufferType */
    Exynos_OSAL_Memcpy(&type, (MetadataBufferType *)pBuffer, sizeof(MetadataBufferType));

    switch ((int)type) {
    case kMetadataBufferTypeCameraSource:
    {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d - Unsupport Type of MetadataBuffer", __FUNCTION__, __LINE__);
    }
        break;
    case kMetadataBufferTypeGrallocSource:
    {
        VideoGrallocMetadata    *pMetaData      = (VideoGrallocMetadata *)pBuffer;
        buffer_handle_t          bufferHandle   = (buffer_handle_t)pMetaData->pHandle;
        native_handle_t         *pNativeHandle  = (native_handle_t *)bufferHandle;

        /* size of stream */
        pNativeHandle->data[3] = dataLength;
    }
        break;
    default:
    {
        ret = OMX_ErrorBadParameter;
    }
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_PTR Exynos_OSAL_AllocMetaDataBuffer(
    OMX_HANDLETYPE      hSharedMemory,
    EXYNOS_CODEC_TYPE   codecType,
    OMX_U32             nPortIndex,
    OMX_U32             nSizeBytes,
    MEMORY_TYPE         eMemoryType)
{
    /*
     * meta data contains the following data format.
     * payload depends on the MetadataBufferType
     * ---------------------------------------------------------------
     * | MetadataBufferType               |         payload          |
     * ---------------------------------------------------------------
     * If MetadataBufferType is kMetadataBufferTypeGrallocSource, then
     * ---------------------------------------------------------------
     * | kMetadataBufferTypeGrallocSource |     buffer_handle_t      |
     * ---------------------------------------------------------------
     */

#define ENC_OUT_FD_NUM 1
#define EXTRA_DATA_NUM 3

    VideoGrallocMetadata *pMetaData = NULL;
    buffer_handle_t  bufferHandle  = NULL;
    native_handle_t *pNativeHandle = NULL;

    OMX_PTR pTempBuffer = NULL;
    OMX_PTR pTempVirAdd = NULL;
    OMX_U32 nTempFD     = 0;

    if (((codecType == HW_VIDEO_ENC_CODEC) ||
         (codecType == HW_VIDEO_ENC_SECURE_CODEC)) &&
        (nPortIndex == OUTPUT_PORT_INDEX)) {
        pTempBuffer = Exynos_OSAL_Malloc(MAX_METADATA_BUFFER_SIZE);
        if (pTempBuffer == NULL) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d - Error InsufficientResources", __FUNCTION__, __LINE__);
            goto EXIT;
        }

        pNativeHandle = native_handle_create(ENC_OUT_FD_NUM, EXTRA_DATA_NUM);
        if (pNativeHandle == NULL) {
            Exynos_OSAL_Free(pTempBuffer);
            pTempBuffer = NULL;
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d - Error InsufficientResources", __FUNCTION__, __LINE__);
            goto EXIT;
        }

        pTempVirAdd = Exynos_OSAL_SharedMemory_Alloc(hSharedMemory, nSizeBytes, eMemoryType);
        if (pTempVirAdd == NULL) {
            native_handle_delete(pNativeHandle);
            pNativeHandle = NULL;
            Exynos_OSAL_Free(pTempBuffer);
            pTempBuffer = NULL;
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d - Error InsufficientResources", __FUNCTION__, __LINE__);
            goto EXIT;
        }

        nTempFD = Exynos_OSAL_SharedMemory_VirtToION(hSharedMemory, pTempVirAdd);

        pNativeHandle->data[0] = (int)nTempFD;
        pNativeHandle->data[1] = PTR_TO_INT(pTempVirAdd); /* caution : data loss & FIXME : remove  */
        pNativeHandle->data[2] = (int)nSizeBytes;
        pNativeHandle->data[3] = (int)0;

        pMetaData       = (VideoGrallocMetadata *)pTempBuffer;
        bufferHandle    = (buffer_handle_t)pNativeHandle;

        pMetaData->eType   = kMetadataBufferTypeGrallocSource;
        pMetaData->pHandle = bufferHandle;
    } else {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d - Unsupport MetadataBuffer", __FUNCTION__, __LINE__);
        pTempBuffer = NULL;
    }

EXIT:
    return pTempBuffer;
}

OMX_ERRORTYPE Exynos_OSAL_FreeMetaDataBuffer(
    OMX_HANDLETYPE      hSharedMemory,
    EXYNOS_CODEC_TYPE   codecType,
    OMX_U32             nPortIndex,
    OMX_PTR             pTempBuffer)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    OMX_U32 nTempFD     = 0;
    OMX_PTR pTempVirAdd = NULL;

    VideoGrallocMetadata    *pMetaData      = NULL;
    buffer_handle_t          bufferHandle   = NULL;
    native_handle_t         *pNativeHandle  = NULL;

    if (((codecType == HW_VIDEO_ENC_CODEC) ||
         (codecType == HW_VIDEO_ENC_SECURE_CODEC)) &&
        (nPortIndex == OUTPUT_PORT_INDEX)) {
        if (*(MetadataBufferType *)(pTempBuffer) != (MetadataBufferType)kMetadataBufferTypeGrallocSource) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d - Invalid MetaDataBuffer", __FUNCTION__, __LINE__);
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        pMetaData       = (VideoGrallocMetadata *)pTempBuffer;
        bufferHandle    = (buffer_handle_t)pMetaData->pHandle;
        pNativeHandle   = (native_handle_t *)bufferHandle;

        nTempFD     = (OMX_U32)pNativeHandle->data[0];
        pTempVirAdd = Exynos_OSAL_SharedMemory_IONToVirt(hSharedMemory, nTempFD);

        Exynos_OSAL_SharedMemory_Free(hSharedMemory, pTempVirAdd);

        native_handle_delete(pNativeHandle);

        Exynos_OSAL_Free(pTempBuffer);

        ret = OMX_ErrorNone;
    } else {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d - Unsupport MetadataBuffer", __FUNCTION__, __LINE__);
        ret = OMX_ErrorNotImplemented;
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_SetPrependSPSPPSToIDR(
    OMX_PTR pComponentParameterStructure,
    OMX_PTR pbPrependSpsPpsToIdr)
{
    OMX_ERRORTYPE                    ret        = OMX_ErrorNone;
    PrependSPSPPSToIDRFramesParams  *pParams = (PrependSPSPPSToIDRFramesParams *)pComponentParameterStructure;

    ret = Exynos_OMX_Check_SizeVersion(pParams, sizeof(PrependSPSPPSToIDRFramesParams));
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OMX_Check_SizeVersion(PrependSPSPPSToIDRFramesParams) is failed", __func__);
        goto EXIT;
    }

    (*((OMX_BOOL *)pbPrependSpsPpsToIdr)) = pParams->bEnable;

EXIT:
    return ret;
}

OMX_U32 Exynos_OSAL_GetDisplayExtraBufferCount(void)
{
    char value[PROPERTY_VALUE_MAX] = {0, };

    if (property_get("debug.hwc.otf", value, NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "DisplayExtraBufferCount is 3. The OTF exist");
        return 3;  /* Display System has OTF */
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "DisplayExtraBufferCount is 2. The OTF not exist");
    return 2;  /* min count */
}

#ifdef __cplusplus
}
#endif
