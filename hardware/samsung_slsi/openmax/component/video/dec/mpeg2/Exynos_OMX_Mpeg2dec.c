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
 * @file        Exynos_OMX_Mpeg2dec.c
 * @brief
 * @author      Satish Kumar Reddy (palli.satish@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.07.10 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exynos_OMX_Macros.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Vdec.h"
#include "Exynos_OMX_VdecControl.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Thread.h"
#include "library_register.h"
#include "Exynos_OMX_Mpeg2dec.h"
#include "ExynosVideoApi.h"
#include "Exynos_OSAL_SharedMemory.h"
#include "Exynos_OSAL_Event.h"

/* To use CSC_METHOD_HW in EXYNOS OMX, gralloc should allocate physical memory using FIMC */
/* It means GRALLOC_USAGE_HW_FIMC1 should be set on Native Window usage */
#include "csc.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_MPEG2_DEC"
//#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

#define MPEG2_DEC_NUM_OF_EXTRA_BUFFERS 7

//#define FULL_FRAME_SEARCH /* Full frame search not support*/

static OMX_ERRORTYPE SetProfileLevel(
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    OMX_ERRORTYPE                    ret            = OMX_ErrorNone;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec      = NULL;
    EXYNOS_MPEG2DEC_HANDLE          *pMpeg2Dec      = NULL;

    int nProfileCnt = 0;

    FunctionIn();

    if (pExynosComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    if (pVideoDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pMpeg2Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pMpeg2Dec->hMFCMpeg2Handle.profiles[nProfileCnt++] = OMX_VIDEO_MPEG2ProfileSimple;
    pMpeg2Dec->hMFCMpeg2Handle.profiles[nProfileCnt++] = OMX_VIDEO_MPEG2ProfileMain;
    pMpeg2Dec->hMFCMpeg2Handle.nProfileCnt = nProfileCnt;
    pMpeg2Dec->hMFCMpeg2Handle.maxLevel = OMX_VIDEO_MPEG2LevelHL;

EXIT:
    return ret;
}

static OMX_ERRORTYPE GetIndexToProfileLevel(
    EXYNOS_OMX_BASECOMPONENT         *pExynosComponent,
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *pProfileLevelType)
{
    OMX_ERRORTYPE                    ret            = OMX_ErrorNone;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec      = NULL;
    EXYNOS_MPEG2DEC_HANDLE           *pMpeg2Dec       = NULL;

    int nLevelCnt = 0;
    OMX_U32 nMaxIndex = 0;

    FunctionIn();

    if ((pExynosComponent == NULL) ||
        (pProfileLevelType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    if (pVideoDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pMpeg2Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    while ((pMpeg2Dec->hMFCMpeg2Handle.maxLevel >> nLevelCnt) > 0) {
        nLevelCnt++;
    }
    nLevelCnt += 1;  /* OMX_VIDEO_MPEG2LevelLL : 0 */

    if ((pMpeg2Dec->hMFCMpeg2Handle.nProfileCnt == 0) ||
        (nLevelCnt == 0)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : there is no any profile/level", __FUNCTION__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    nMaxIndex = pMpeg2Dec->hMFCMpeg2Handle.nProfileCnt * nLevelCnt;
    if (nMaxIndex <= pProfileLevelType->nProfileIndex) {
        ret = OMX_ErrorNoMore;
        goto EXIT;
    }

    pProfileLevelType->eProfile = pMpeg2Dec->hMFCMpeg2Handle.profiles[pProfileLevelType->nProfileIndex / nLevelCnt];
    pProfileLevelType->eLevel = 0x1 << (pProfileLevelType->nProfileIndex % nLevelCnt);

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s : supported profile(%x), level(%x)", __FUNCTION__, pProfileLevelType->eProfile, pProfileLevelType->eLevel);

EXIT:
    return ret;
}

static OMX_BOOL CheckProfileLevelSupport(
    EXYNOS_OMX_BASECOMPONENT         *pExynosComponent,
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *pProfileLevelType)
{
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec  = NULL;
    EXYNOS_MPEG2DEC_HANDLE           *pMpeg2Dec   = NULL;

    OMX_BOOL bProfileSupport = OMX_FALSE;
    OMX_BOOL bLevelSupport   = OMX_FALSE;

    int nLevelCnt = 0;
    int i;

    FunctionIn();

    if ((pExynosComponent == NULL) ||
        (pProfileLevelType == NULL)) {
        goto EXIT;
    }

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    if (pVideoDec == NULL)
        goto EXIT;

    pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pMpeg2Dec == NULL)
        goto EXIT;

    while ((pMpeg2Dec->hMFCMpeg2Handle.maxLevel >> nLevelCnt++) > 0);
    nLevelCnt += 1;  /* OMX_VIDEO_MPEG2LevelLL : 0 */

    if ((pMpeg2Dec->hMFCMpeg2Handle.nProfileCnt == 0) ||
        (nLevelCnt == 0)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : there is no any profile/level", __FUNCTION__);
        goto EXIT;
    }

    for (i = 0; i < pMpeg2Dec->hMFCMpeg2Handle.nProfileCnt; i++) {
        if (pMpeg2Dec->hMFCMpeg2Handle.profiles[i] == pProfileLevelType->eProfile) {
            bProfileSupport = OMX_TRUE;
            break;
        }
    }

    if (bProfileSupport != OMX_TRUE)
        goto EXIT;

    if (pProfileLevelType->eLevel == OMX_VIDEO_MPEG2LevelLL) {
        bLevelSupport = OMX_TRUE;
    } else {
        nLevelCnt--;
        while (nLevelCnt >= 0) {
            if ((int)pProfileLevelType->eLevel == (0x1 << nLevelCnt)) {
                bLevelSupport = OMX_TRUE;
                break;
            }

            nLevelCnt--;
        }
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s : profile(%x)/level(%x) is %ssupported", __FUNCTION__,
                                            pProfileLevelType->eProfile, pProfileLevelType->eLevel,
                                            (bProfileSupport && bLevelSupport)? "":"not ");

EXIT:
    return (bProfileSupport && bLevelSupport);
}

#if 0  /* unused code */
static OMX_ERRORTYPE GetCodecInputPrivateData(OMX_PTR codecBuffer, OMX_PTR *pVirtAddr, OMX_U32 *dataSize)
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;

EXIT:
    return ret;
}
#endif

static OMX_ERRORTYPE GetCodecOutputPrivateData(OMX_PTR codecBuffer, OMX_PTR addr[], OMX_U32 size[])
{
    OMX_ERRORTYPE       ret          = OMX_ErrorNone;
    ExynosVideoBuffer  *pCodecBuffer = NULL;

    if (codecBuffer == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pCodecBuffer = (ExynosVideoBuffer *)codecBuffer;

    if (addr != NULL) {
        addr[0] = pCodecBuffer->planes[0].addr;
        addr[1] = pCodecBuffer->planes[1].addr;
        addr[2] = pCodecBuffer->planes[2].addr;
    }

    if (size != NULL) {
        size[0] = pCodecBuffer->planes[0].allocSize;
        size[1] = pCodecBuffer->planes[1].allocSize;
        size[2] = pCodecBuffer->planes[2].allocSize;
    }

EXIT:
    return ret;
}

#if 0  /* unused code */
int Check_Mpeg2_Frame(
    OMX_U8   *pInputStream,
    OMX_U32   buffSize,
    OMX_U32   flag,
    OMX_BOOL  bPreviousFrameEOF,
    OMX_BOOL *pbEndOfFrame)
{
    FunctionIn();

    *pbEndOfFrame = OMX_TRUE;

    /* Frame  Start code*/
    if (pInputStream[0] != 0x00 || pInputStream[1] != 0x00 || pInputStream[2]!=0x01) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Mpeg2 Frame Start Code not Found");
        *pbEndOfFrame = OMX_FALSE;
    }

    FunctionOut();
    return buffSize;
}
#endif

static OMX_BOOL Check_Mpeg2_StartCode(
    OMX_U8     *pInputStream,
    OMX_U32     streamSize)
{
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "streamSize: %d",streamSize);

    if (streamSize < 3) {
        return OMX_FALSE;
    }

    /* Frame  Start code*/
    if (pInputStream[0] != 0x00 || pInputStream[1] != 0x00 || pInputStream[2]!=0x01) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Mpeg2 Frame Start Code not Found");
        return OMX_FALSE;
    }

    return OMX_TRUE;
}

OMX_BOOL CheckFormatHWSupport(
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent,
    OMX_COLOR_FORMATTYPE         eColorFormat)
{
    OMX_BOOL                         ret            = OMX_FALSE;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec      = NULL;
    EXYNOS_MPEG2DEC_HANDLE          *pMpeg2Dec      = NULL;
    EXYNOS_OMX_BASEPORT             *pOutputPort    = NULL;
    ExynosVideoColorFormatType       eVideoFormat   = VIDEO_CODING_UNKNOWN;
    int i;

    FunctionIn();

    if (pExynosComponent == NULL)
        goto EXIT;

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    if (pVideoDec == NULL)
        goto EXIT;

    pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pMpeg2Dec == NULL)
        goto EXIT;
    pOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    eVideoFormat = (ExynosVideoColorFormatType)Exynos_OSAL_OMX2VideoFormat(eColorFormat, pOutputPort->ePlaneType);

    for (i = 0; i < VIDEO_COLORFORMAT_MAX; i++) {
        if (pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo.supportFormat[i] == VIDEO_COLORFORMAT_UNKNOWN)
            break;

        if (pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo.supportFormat[i] == eVideoFormat) {
            ret = OMX_TRUE;
            break;
        }
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE Mpeg2CodecOpen(EXYNOS_MPEG2DEC_HANDLE *pMpeg2Dec, ExynosVideoInstInfo *pVideoInstInfo)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pMpeg2Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }

    /* alloc ops structure */
    pDecOps    = (ExynosVideoDecOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoDecOps));
    pInbufOps  = (ExynosVideoDecBufferOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoDecBufferOps));
    pOutbufOps = (ExynosVideoDecBufferOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoDecBufferOps));

    if ((pDecOps == NULL) || (pInbufOps == NULL) || (pOutbufOps == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate decoder ops buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pMpeg2Dec->hMFCMpeg2Handle.pDecOps    = pDecOps;
    pMpeg2Dec->hMFCMpeg2Handle.pInbufOps  = pInbufOps;
    pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps = pOutbufOps;

    /* function pointer mapping */
    pDecOps->nSize    = sizeof(ExynosVideoDecOps);
    pInbufOps->nSize  = sizeof(ExynosVideoDecBufferOps);
    pOutbufOps->nSize = sizeof(ExynosVideoDecBufferOps);

    Exynos_Video_Register_Decoder(pDecOps, pInbufOps, pOutbufOps);

    /* check mandatory functions for decoder ops */
    if ((pDecOps->Init == NULL) || (pDecOps->Finalize == NULL) ||
        (pDecOps->Get_ActualBufferCount == NULL) || (pDecOps->Set_FrameTag == NULL) ||
        (pDecOps->Get_FrameTag == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Mandatory functions must be supplied");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* check mandatory functions for buffer ops */
    if ((pInbufOps->Setup == NULL) || (pOutbufOps->Setup == NULL) ||
        (pInbufOps->Run == NULL) || (pOutbufOps->Run == NULL) ||
        (pInbufOps->Stop == NULL) || (pOutbufOps->Stop == NULL) ||
        (pInbufOps->Enqueue == NULL) || (pOutbufOps->Enqueue == NULL) ||
        (pInbufOps->Dequeue == NULL) || (pOutbufOps->Dequeue == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Mandatory functions must be supplied");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* alloc context, open, querycap */
#ifdef USE_DMA_BUF
    pVideoInstInfo->nMemoryType = V4L2_MEMORY_DMABUF;
#else
    pVideoInstInfo->nMemoryType = V4L2_MEMORY_USERPTR;
#endif
    pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle = pMpeg2Dec->hMFCMpeg2Handle.pDecOps->Init(pVideoInstInfo);
    if (pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate context buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    if (ret != OMX_ErrorNone) {
        if (pDecOps != NULL) {
            Exynos_OSAL_Free(pDecOps);
            pMpeg2Dec->hMFCMpeg2Handle.pDecOps = NULL;
        }
        if (pInbufOps != NULL) {
            Exynos_OSAL_Free(pInbufOps);
            pMpeg2Dec->hMFCMpeg2Handle.pInbufOps = NULL;
        }
        if (pOutbufOps != NULL) {
            Exynos_OSAL_Free(pOutbufOps);
            pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps = NULL;
        }
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg2CodecClose(EXYNOS_MPEG2DEC_HANDLE *pMpeg2Dec)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pMpeg2Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    pDecOps    = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    pInbufOps  = pMpeg2Dec->hMFCMpeg2Handle.pInbufOps;
    pOutbufOps = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;

    if (hMFCHandle != NULL) {
        pDecOps->Finalize(hMFCHandle);
        pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle = NULL;
    }

    /* Unregister function pointers */
    Exynos_Video_Unregister_Decoder(pDecOps, pInbufOps, pOutbufOps);

    if (pOutbufOps != NULL) {
        Exynos_OSAL_Free(pOutbufOps);
        pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps = NULL;
    }
    if (pInbufOps != NULL) {
        Exynos_OSAL_Free(pInbufOps);
        pMpeg2Dec->hMFCMpeg2Handle.pInbufOps = NULL;
    }
    if (pDecOps != NULL) {
        Exynos_OSAL_Free(pDecOps);
        pMpeg2Dec->hMFCMpeg2Handle.pDecOps = NULL;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg2CodecStart(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_MPEG2DEC_HANDLE   *pMpeg2Dec = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)((EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate)->hComponentHandle;
    if (pVideoDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pMpeg2Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    pDecOps    = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    pInbufOps  = pMpeg2Dec->hMFCMpeg2Handle.pInbufOps;
    pOutbufOps = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX)
        pInbufOps->Run(hMFCHandle);
    else if (nPortIndex == OUTPUT_PORT_INDEX)
        pOutbufOps->Run(hMFCHandle);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg2CodecStop(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = NULL;
    EXYNOS_MPEG2DEC_HANDLE          *pMpeg2Dec          = NULL;
    void                            *hMFCHandle         = NULL;
    ExynosVideoDecOps               *pDecOps            = NULL;
    ExynosVideoDecBufferOps         *pInbufOps          = NULL;
    ExynosVideoDecBufferOps         *pOutbufOps         = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    if (pVideoDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pMpeg2Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    pDecOps    = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    pInbufOps  = pMpeg2Dec->hMFCMpeg2Handle.pInbufOps;
    pOutbufOps = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;

    if ((nPortIndex == INPUT_PORT_INDEX) && (pInbufOps != NULL)) {
        pInbufOps->Stop(hMFCHandle);
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) && (pOutbufOps != NULL)) {
        EXYNOS_OMX_BASEPORT *pOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

        pOutbufOps->Stop(hMFCHandle);

        if ((pOutputPort->bufferProcessType & BUFFER_SHARE) &&
            (pOutputPort->bDynamicDPBMode == OMX_TRUE))
            pOutbufOps->Clear_RegisteredBuffer(hMFCHandle);
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg2CodecOutputBufferProcessRun(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_MPEG2DEC_HANDLE   *pMpeg2Dec = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)((EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate)->hComponentHandle;
    if (pVideoDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pMpeg2Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    pDecOps    = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    pInbufOps  = pMpeg2Dec->hMFCMpeg2Handle.pInbufOps;
    pOutbufOps = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX) {
        if (pMpeg2Dec->bSourceStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pMpeg2Dec->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    if (nPortIndex == OUTPUT_PORT_INDEX) {
        if (pMpeg2Dec->bDestinationStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pMpeg2Dec->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg2CodecRegistCodecBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex,
    int                  nBufferCnt)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG2DEC_HANDLE          *pMpeg2Dec          = (EXYNOS_MPEG2DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                            *hMFCHandle         = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    CODEC_DEC_BUFFER               **ppCodecBuffer      = NULL;
    ExynosVideoDecBufferOps         *pBufOps            = NULL;
    ExynosVideoPlane                *pPlanes            = NULL;

    int nPlaneCnt = 0;
    int i, j;

    FunctionIn();

    if (nPortIndex == INPUT_PORT_INDEX) {
        ppCodecBuffer   = &(pVideoDec->pMFCDecInputBuffer[0]);
        pBufOps         = pMpeg2Dec->hMFCMpeg2Handle.pInbufOps;
    } else {
        ppCodecBuffer   = &(pVideoDec->pMFCDecOutputBuffer[0]);
        pBufOps         = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;
    }
    nPlaneCnt = Exynos_GetPlaneFromPort(&pExynosComponent->pExynosPort[nPortIndex]);

    pPlanes = (ExynosVideoPlane *)Exynos_OSAL_Malloc(sizeof(ExynosVideoPlane) * nPlaneCnt);
    if (pPlanes == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* Register buffer */
    for (i = 0; i < nBufferCnt; i++) {
        for (j = 0; j < nPlaneCnt; j++) {
            pPlanes[j].addr         = ppCodecBuffer[i]->pVirAddr[j];
            pPlanes[j].fd           = ppCodecBuffer[i]->fd[j];
            pPlanes[j].allocSize    = ppCodecBuffer[i]->bufferSize[j];
        }

        if (pBufOps->Register(hMFCHandle, pPlanes, nPlaneCnt) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "PORT[%d]: Failed to Register buffer", nPortIndex);
            ret = OMX_ErrorInsufficientResources;
            Exynos_OSAL_Free(pPlanes);
            goto EXIT;
        }
    }

    Exynos_OSAL_Free(pPlanes);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg2CodecReconfigAllBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT             *pExynosPort        = &pExynosComponent->pExynosPort[nPortIndex];
    EXYNOS_MPEG2DEC_HANDLE          *pMpeg2Dec          = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                            *hMFCHandle         = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    ExynosVideoDecBufferOps         *pBufferOps         = NULL;

    FunctionIn();

    if ((nPortIndex == INPUT_PORT_INDEX) &&
        (pMpeg2Dec->bSourceStart == OMX_TRUE)) {
        ret = OMX_ErrorNotImplemented;
        goto EXIT;
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) &&
               (pMpeg2Dec->bDestinationStart == OMX_TRUE)) {
        pBufferOps = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;

        if (pExynosPort->bufferProcessType & BUFFER_COPY) {
            /**********************************/
            /* Codec Buffer Free & Unregister */
            /**********************************/
            Exynos_Free_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX);
            Exynos_CodecBufferReset(pExynosComponent, OUTPUT_PORT_INDEX);
            pBufferOps->Clear_RegisteredBuffer(hMFCHandle);
            pBufferOps->Cleanup_Buffer(hMFCHandle);

            /******************************************************/
            /* V4L2 Destnation Setup for DPB Buffer Number Change */
            /******************************************************/
            ret = Mpeg2CodecDstSetup(pOMXComponent);
            if (ret != OMX_ErrorNone) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed to Mpeg2CodecDstSetup(0x%x)", __func__, __LINE__, ret);
                goto EXIT;
            }

            pVideoDec->bReconfigDPB = OMX_FALSE;
        } else if (pExynosPort->bufferProcessType & BUFFER_SHARE) {
            /**********************************/
            /* Codec Buffer Unregister */
            /**********************************/
            pBufferOps->Clear_RegisteredBuffer(hMFCHandle);
            pBufferOps->Cleanup_Buffer(hMFCHandle);
        }

        Exynos_ResolutionUpdate(pOMXComponent);
    } else {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg2CodecEnQueueAllBuffer(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG2DEC_HANDLE         *pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    int i, nOutbufs;

    ExynosVideoDecOps       *pDecOps    = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pMpeg2Dec->hMFCMpeg2Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;

    FunctionIn();

    if ((nPortIndex != INPUT_PORT_INDEX) && (nPortIndex != OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((nPortIndex == INPUT_PORT_INDEX) &&
        (pMpeg2Dec->bSourceStart == OMX_TRUE)) {
        Exynos_CodecBufferReset(pExynosComponent, INPUT_PORT_INDEX);

        for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoDec->pMFCDecInputBuffer[%d]: 0x%x", i, pVideoDec->pMFCDecInputBuffer[i]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoDec->pMFCDecInputBuffer[%d]->pVirAddr[0]: 0x%x", i, pVideoDec->pMFCDecInputBuffer[i]->pVirAddr[0]);

            Exynos_CodecBufferEnQueue(pExynosComponent, INPUT_PORT_INDEX, pVideoDec->pMFCDecInputBuffer[i]);
        }

        pInbufOps->Clear_Queue(hMFCHandle);
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) &&
               (pMpeg2Dec->bDestinationStart == OMX_TRUE)) {
        Exynos_CodecBufferReset(pExynosComponent, OUTPUT_PORT_INDEX);

        for (i = 0; i < pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum; i++) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoDec->pMFCDecOutputBuffer[%d]: 0x%x", i, pVideoDec->pMFCDecOutputBuffer[i]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoDec->pMFCDecOutputBuffer[%d]->pVirAddr[0]: 0x%x", i, pVideoDec->pMFCDecOutputBuffer[i]->pVirAddr[0]);

            Exynos_CodecBufferEnQueue(pExynosComponent, OUTPUT_PORT_INDEX, pVideoDec->pMFCDecOutputBuffer[i]);
        }
        pOutbufOps->Clear_Queue(hMFCHandle);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg2CodecCheckResolution(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG2DEC_HANDLE        *pMpeg2Dec          = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle         = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pOutputPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_EXCEPTION_STATE     eOutputExcepState  = pOutputPort->exceptionFlag;

    ExynosVideoDecOps             *pDecOps            = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    ExynosVideoDecBufferOps       *pOutbufOps         = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;
    ExynosVideoGeometry            codecOutbufConf;

    OMX_CONFIG_RECTTYPE          *pCropRectangle        = &(pOutputPort->cropRectangle);
    OMX_PARAM_PORTDEFINITIONTYPE *pInputPortDefinition  = &(pInputPort->portDefinition);
    OMX_PARAM_PORTDEFINITIONTYPE *pOutputPortDefinition = &(pOutputPort->portDefinition);

    int maxDPBNum = 0;

    FunctionIn();

    /* get geometry */
    Exynos_OSAL_Memset(&codecOutbufConf, 0, sizeof(ExynosVideoGeometry));
    if (pOutbufOps->Get_Geometry(hMFCHandle, &codecOutbufConf) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to get geometry");
        ret = OMX_ErrorHardware;
        goto EXIT;
    }

    /* get dpb count */
    maxDPBNum = pDecOps->Get_ActualBufferCount(hMFCHandle);
    if (pVideoDec->bThumbnailMode == OMX_FALSE)
        maxDPBNum += EXTRA_DPB_NUM;

    if ((codecOutbufConf.nFrameWidth != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameWidth) ||
        (codecOutbufConf.nFrameHeight != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameHeight) ||
        (codecOutbufConf.nStride != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nStride) ||
#if 0  // TODO: check posibility
        (codecOutbufConf.eColorFormat != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.eColorFormat) ||
        (codecOutbufConf.eFilledDataType != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.eFilledDataType) ||
        (codecOutbufConf.bInterlaced != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.bInterlaced) ||
#endif
        (maxDPBNum != pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] DRC: W(%d), H(%d) -> W(%d), H(%d)",
                            pExynosComponent, __FUNCTION__,
                            codecOutbufConf.nFrameWidth,
                            codecOutbufConf.nFrameHeight,
                            pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameWidth,
                            pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameHeight);
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] DRC: DPB(%d), FORMAT(0x%x), TYPE(0x%x) -> DPB(%d), FORMAT(0x%x), TYPE(0x%x)",
                            pExynosComponent, __FUNCTION__,
                            maxDPBNum, codecOutbufConf.eColorFormat, codecOutbufConf.eFilledDataType,
                            pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum,
                            pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.eColorFormat,
                            pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.eFilledDataType);

        pInputPortDefinition->format.video.nFrameWidth     = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameWidth;
        pInputPortDefinition->format.video.nFrameHeight    = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameHeight;
        pInputPortDefinition->format.video.nStride         = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameWidth;
        pInputPortDefinition->format.video.nSliceHeight    = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameHeight;

        if (pOutputPort->bufferProcessType & BUFFER_SHARE) {
            pOutputPortDefinition->nBufferCountActual  = pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum;
            pOutputPortDefinition->nBufferCountMin     = pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum;
        }

        Exynos_UpdateFrameSize(pOMXComponent);

        if (eOutputExcepState == GENERAL_STATE) {
            pOutputPort->exceptionFlag = NEED_PORT_DISABLE;

            /** Send Port Settings changed call back **/
            (*(pExynosComponent->pCallbacks->EventHandler))
                (pOMXComponent,
                 pExynosComponent->callbackData,
                 OMX_EventPortSettingsChanged, /* The command was completed */
                 OMX_DirOutput, /* This is the port index */
                 0,
                 NULL);
        }
    }

    if ((codecOutbufConf.cropRect.nTop != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nTop) ||
        (codecOutbufConf.cropRect.nLeft != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nLeft) ||
        (codecOutbufConf.cropRect.nWidth != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nWidth) ||
        (codecOutbufConf.cropRect.nHeight != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nHeight)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] CROP: W(%d), H(%d) -> W(%d), H(%d)",
                            pExynosComponent, __FUNCTION__,
                            codecOutbufConf.cropRect.nWidth,
                            codecOutbufConf.cropRect.nHeight,
                            pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nWidth,
                            pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nHeight);

        pCropRectangle->nTop     = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nTop;
        pCropRectangle->nLeft    = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nLeft;
        pCropRectangle->nWidth   = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nWidth;
        pCropRectangle->nHeight  = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nHeight;

        /** Send crop info call back **/
        (*(pExynosComponent->pCallbacks->EventHandler))
            (pOMXComponent,
             pExynosComponent->callbackData,
             OMX_EventPortSettingsChanged, /* The command was completed */
             OMX_DirOutput, /* This is the port index */
             OMX_IndexConfigCommonOutputCrop,
             NULL);
    }

    Exynos_OSAL_Memcpy(&pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf, &codecOutbufConf, sizeof(codecOutbufConf));
    pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum = maxDPBNum;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg2CodecUpdateResolution(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG2DEC_HANDLE        *pMpeg2Dec          = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle         = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pOutputPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    ExynosVideoDecOps             *pDecOps            = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    ExynosVideoDecBufferOps       *pOutbufOps         = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;

    OMX_CONFIG_RECTTYPE             *pCropRectangle         = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE    *pInputPortDefinition   = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE    *pOutputPortDefinition  = NULL;

    FunctionIn();

    /* get geometry for output */
    Exynos_OSAL_Memset(&pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf, 0, sizeof(ExynosVideoGeometry));
    if (pOutbufOps->Get_Geometry(hMFCHandle, &pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to get geometry for parsed header info");
        ret = OMX_ErrorCorruptedHeader;
        goto EXIT;
    }

    /* get dpb count */
    pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum = pDecOps->Get_ActualBufferCount(hMFCHandle);
    if (pVideoDec->bThumbnailMode == OMX_FALSE)
        pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum += EXTRA_DPB_NUM;
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] Mpeg2CodecSetup maxDPBNum: %d", pExynosComponent, __FUNCTION__, pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum);

    /* get interlace info */
    if (pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.bInterlaced == VIDEO_TRUE)
        Exynos_OSAL_Log(EXYNOS_LOG_INFO, "detect an interlaced type");

    pMpeg2Dec->hMFCMpeg2Handle.bConfiguredMFCSrc = OMX_TRUE;

    if (pVideoDec->bReconfigDPB != OMX_TRUE) {
        pCropRectangle          = &(pOutputPort->cropRectangle);
        pInputPortDefinition    = &(pInputPort->portDefinition);
        pOutputPortDefinition   = &(pOutputPort->portDefinition);
    } else {
        pCropRectangle          = &(pOutputPort->newCropRectangle);
        pInputPortDefinition    = &(pInputPort->newPortDefinition);
        pOutputPortDefinition   = &(pOutputPort->newPortDefinition);
    }

    pCropRectangle->nTop     = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nTop;
    pCropRectangle->nLeft    = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nLeft;
    pCropRectangle->nWidth   = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nWidth;
    pCropRectangle->nHeight  = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nHeight;

    if (pOutputPort->bufferProcessType & BUFFER_COPY) {
        if ((pVideoDec->bReconfigDPB) ||
            (pInputPort->portDefinition.format.video.nFrameWidth != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameWidth) ||
            (pInputPort->portDefinition.format.video.nFrameHeight != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameHeight)) {
            pOutputPort->exceptionFlag = NEED_PORT_DISABLE;

            pInputPortDefinition->format.video.nFrameWidth  = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nFrameHeight = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameHeight;
            pInputPortDefinition->format.video.nStride      = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nSliceHeight = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameHeight;

            pOutputPortDefinition->nBufferCountActual       = pOutputPort->portDefinition.nBufferCountActual;
            pOutputPortDefinition->nBufferCountMin          = pOutputPort->portDefinition.nBufferCountMin;

            if (pVideoDec->bReconfigDPB != OMX_TRUE)
                Exynos_UpdateFrameSize(pOMXComponent);

            /** Send Port Settings changed call back **/
            (*(pExynosComponent->pCallbacks->EventHandler))
                (pOMXComponent,
                 pExynosComponent->callbackData,
                 OMX_EventPortSettingsChanged, /* The command was completed */
                 OMX_DirOutput, /* This is the port index */
                 0,
                 NULL);
        }
    } else if (pOutputPort->bufferProcessType & BUFFER_SHARE) {
        if ((pVideoDec->bReconfigDPB) ||
            (pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.bInterlaced == VIDEO_TRUE) ||
            (pInputPort->portDefinition.format.video.nFrameWidth != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameWidth) ||
            (pInputPort->portDefinition.format.video.nFrameHeight != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameHeight) ||
            ((OMX_S32)pOutputPort->portDefinition.nBufferCountActual != pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum)) {
            pOutputPort->exceptionFlag = NEED_PORT_DISABLE;

            pInputPortDefinition->format.video.nFrameWidth  = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nFrameHeight = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameHeight;
            pInputPortDefinition->format.video.nStride      = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nSliceHeight = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameHeight;

            pOutputPortDefinition->nBufferCountActual       = pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum;
            pOutputPortDefinition->nBufferCountMin          = pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum;

            if (pVideoDec->bReconfigDPB != OMX_TRUE)
                Exynos_UpdateFrameSize(pOMXComponent);

            /** Send Port Settings changed call back **/
            (*(pExynosComponent->pCallbacks->EventHandler))
                (pOMXComponent,
                 pExynosComponent->callbackData,
                 OMX_EventPortSettingsChanged, /* The command was completed */
                 OMX_DirOutput, /* This is the port index */
                 0,
                 NULL);
        }
    }

    if ((pVideoDec->bReconfigDPB != OMX_TRUE) &&
        ((pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameWidth != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nWidth) ||
         (pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameHeight != pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.cropRect.nHeight))) {
        /* Check Crop */
        pInputPortDefinition->format.video.nFrameWidth  = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameWidth;
        pInputPortDefinition->format.video.nFrameHeight = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameHeight;
        pInputPortDefinition->format.video.nStride      = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameWidth;
        pInputPortDefinition->format.video.nSliceHeight = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nFrameHeight;

        Exynos_UpdateFrameSize(pOMXComponent);

        /** Send crop info call back **/
        (*(pExynosComponent->pCallbacks->EventHandler))
            (pOMXComponent,
             pExynosComponent->callbackData,
             OMX_EventPortSettingsChanged, /* The command was completed */
             OMX_DirOutput, /* This is the port index */
             OMX_IndexConfigCommonOutputCrop,
             NULL);
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg2CodecSrcSetup(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec         = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG2DEC_HANDLE        *pMpeg2Dec         = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle        = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32                        oneFrameSize      = pSrcInputData->dataLen;
    OMX_COLOR_FORMATTYPE           eOutputFormat     = pExynosOutputPort->portDefinition.format.video.eColorFormat;

    ExynosVideoDecOps       *pDecOps    = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pMpeg2Dec->hMFCMpeg2Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;
    ExynosVideoGeometry      bufferConf;

    unsigned int nAllocLen[MAX_BUFFER_PLANE] = {0, 0, 0};
    unsigned int nDataLen[MAX_BUFFER_PLANE]  = {oneFrameSize, 0, 0};
    OMX_U32 nInBufferCnt = 0;
    OMX_BOOL bSupportFormat = OMX_FALSE;
    int i;

    FunctionIn();

    if ((oneFrameSize <= 0) && (pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
        BYPASS_BUFFER_INFO *pBufferInfo = (BYPASS_BUFFER_INFO *)Exynos_OSAL_Malloc(sizeof(BYPASS_BUFFER_INFO));
        if (pBufferInfo == NULL) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }

        pBufferInfo->nFlags     = pSrcInputData->nFlags;
        pBufferInfo->timeStamp  = pSrcInputData->timeStamp;
        ret = Exynos_OSAL_Queue(&pMpeg2Dec->bypassBufferInfoQ, (void *)pBufferInfo);
        Exynos_OSAL_SignalSet(pMpeg2Dec->hDestinationStartEvent);

        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pVideoDec->bThumbnailMode == OMX_TRUE)
        pDecOps->Set_IFrameDecoding(hMFCHandle);

    if ((pDecOps->Enable_DTSMode != NULL) &&
        (pVideoDec->bDTSMode == OMX_TRUE))
        pDecOps->Enable_DTSMode(hMFCHandle);

    /* input buffer info */
    Exynos_OSAL_Memset(&bufferConf, 0, sizeof(bufferConf));
    bufferConf.eCompressionFormat = VIDEO_CODING_MPEG2;
    pInbufOps->Set_Shareable(hMFCHandle);
    if (pExynosInputPort->bufferProcessType & BUFFER_SHARE) {
        bufferConf.nSizeImage = pExynosInputPort->portDefinition.nBufferSize;
    } else if (pExynosInputPort->bufferProcessType & BUFFER_COPY) {
        bufferConf.nSizeImage = DEFAULT_MFC_INPUT_BUFFER_SIZE;
    }
    bufferConf.nPlaneCnt = Exynos_GetPlaneFromPort(pExynosInputPort);
    nInBufferCnt = MAX_INPUTBUFFER_NUM_DYNAMIC;

    /* should be done before prepare input buffer */
    if (pInbufOps->Enable_Cacheable(hMFCHandle) != VIDEO_ERROR_NONE) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* set input buffer geometry */
    if (pInbufOps->Set_Geometry(hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* setup input buffer */
    if (pInbufOps->Setup(hMFCHandle, nInBufferCnt) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup input buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* set output geometry */
    Exynos_OSAL_Memset(&bufferConf, 0, sizeof(bufferConf));

    bSupportFormat = CheckFormatHWSupport(pExynosComponent, eOutputFormat);
    if (bSupportFormat == OMX_TRUE) {  /* supported by H/W */
        if ((pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo.specificInfo.dec.bDualDPBSupport == VIDEO_TRUE) &&
            (eOutputFormat != (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV12Tiled)) {
            /* Needs to eanble DualDPB feature */
            if (pDecOps->Enable_DualDPBMode(hMFCHandle) != VIDEO_ERROR_NONE) {
                ret = OMX_ErrorHardware;
                goto EXIT;
            }
        }
        bufferConf.eColorFormat = Exynos_OSAL_OMX2VideoFormat(eOutputFormat, pExynosOutputPort->ePlaneType);
        Exynos_SetPlaneToPort(pExynosOutputPort, Exynos_OSAL_GetPlaneCount(eOutputFormat, pExynosOutputPort->ePlaneType));
    } else {
        OMX_COLOR_FORMATTYPE eCheckFormat = OMX_SEC_COLOR_FormatNV12Tiled;
        bSupportFormat = CheckFormatHWSupport(pExynosComponent, eCheckFormat);
        if (bSupportFormat != OMX_TRUE) {
            eCheckFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            bSupportFormat = CheckFormatHWSupport(pExynosComponent, eCheckFormat);
        }
        if (bSupportFormat == OMX_TRUE) {  /* supported by CSC(NV12T/NV12 -> format) */
            bufferConf.eColorFormat = Exynos_OSAL_OMX2VideoFormat(eCheckFormat, pExynosOutputPort->ePlaneType);
            Exynos_SetPlaneToPort(pExynosOutputPort, Exynos_OSAL_GetPlaneCount(eCheckFormat, pExynosOutputPort->ePlaneType));
        } else {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not support this format (0x%x)", eOutputFormat);
            ret = OMX_ErrorNotImplemented;
            goto EXIT;
        }
    }

    pMpeg2Dec->hMFCMpeg2Handle.MFCOutputColorType = bufferConf.eColorFormat;
    bufferConf.nPlaneCnt = Exynos_GetPlaneFromPort(pExynosOutputPort);
    if (pOutbufOps->Set_Geometry(hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* input buffer enqueue for header parsing */
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] Header Size: %d", pExynosComponent, __FUNCTION__, oneFrameSize);
    if (pExynosInputPort->bufferProcessType & BUFFER_SHARE)
        nAllocLen[0] = pSrcInputData->bufferHeader->nAllocLen;
    else if (pExynosInputPort->bufferProcessType & BUFFER_COPY)
        nAllocLen[0] = DEFAULT_MFC_INPUT_BUFFER_SIZE;

    if (pInbufOps->ExtensionEnqueue(hMFCHandle,
                            (void **)pSrcInputData->multiPlaneBuffer.dataBuffer,
                            (int *)pSrcInputData->multiPlaneBuffer.fd,
                            nAllocLen,
                            nDataLen,
                            Exynos_GetPlaneFromPort(pExynosInputPort),
                            pSrcInputData->bufferHeader) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to enqueue input buffer for header parsing");
//        ret = OMX_ErrorInsufficientResources;
        ret = (OMX_ERRORTYPE)OMX_ErrorCodecInit;
        goto EXIT;
    }

    /* start header parsing */
    if (pInbufOps->Run(hMFCHandle) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run input buffer for header parsing");
        ret = OMX_ErrorCodecInit;
        goto EXIT;
    }

    ret = Mpeg2CodecUpdateResolution(pOMXComponent);
    if (((EXYNOS_OMX_ERRORTYPE)ret == OMX_ErrorCorruptedHeader) &&
        (pExynosComponent->codecType != HW_VIDEO_DEC_SECURE_CODEC) &&
        (oneFrameSize >= 8))
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "CorruptedHeader Info : %02x %02x %02x %02x %02x %02x %02x %02x ...",
                                                                    *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0])    , *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 1),
                                                                    *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 2), *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 3),
                                                                    *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 4), *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 5),
                                                                    *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 6), *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 7));
    if (ret != OMX_ErrorNone) {
        Mpeg2CodecStop(pOMXComponent, INPUT_PORT_INDEX);
        pInbufOps->Cleanup_Buffer(hMFCHandle);
        goto EXIT;
    }

    Exynos_OSAL_SleepMillisec(0);
    ret = OMX_ErrorInputDataDecodeYet;
    Mpeg2CodecStop(pOMXComponent, INPUT_PORT_INDEX);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Mpeg2CodecDstSetup(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG2DEC_HANDLE        *pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoDecOps       *pDecOps    = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pMpeg2Dec->hMFCMpeg2Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;

    unsigned int nAllocLen[MAX_BUFFER_PLANE] = {0, 0, 0};
    unsigned int nDataLen[MAX_BUFFER_PLANE]  = {0, 0, 0};
    int i, nOutbufs, nPlaneCnt;

    FunctionIn();

    nPlaneCnt = Exynos_GetPlaneFromPort(pExynosOutputPort);
    for (i = 0; i < nPlaneCnt; i++)
        nAllocLen[i] = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nAlignPlaneSize[i];

    if (pExynosOutputPort->bDynamicDPBMode == OMX_TRUE) {
        if (pDecOps->Enable_DynamicDPB(hMFCHandle) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to enable Dynamic DPB");
            ret = OMX_ErrorHardware;
            goto EXIT;
        }
    }

    pOutbufOps->Set_Shareable(hMFCHandle);

    if (pExynosOutputPort->bufferProcessType & BUFFER_COPY) {
        /* should be done before prepare output buffer */
        if (pOutbufOps->Enable_Cacheable(hMFCHandle) != VIDEO_ERROR_NONE) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }

        if (pExynosOutputPort->bDynamicDPBMode == OMX_FALSE) {
            /* get dpb count */
            nOutbufs = pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum;
            if (pOutbufOps->Setup(hMFCHandle, nOutbufs) != VIDEO_ERROR_NONE) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup output buffer");
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }

            ret = Exynos_Allocate_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX, nOutbufs, nAllocLen);
            if (ret != OMX_ErrorNone)
                goto EXIT;

            /* Register output buffer */
            ret = Mpeg2CodecRegistCodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX, nOutbufs);
            if (ret != OMX_ErrorNone)
                goto EXIT;

            /* Enqueue output buffer */
            for (i = 0; i < nOutbufs; i++)
                pOutbufOps->Enqueue(hMFCHandle,
                                    (void **)pVideoDec->pMFCDecOutputBuffer[i]->pVirAddr,
                                    nDataLen,
                                    nPlaneCnt,
                                    NULL);
        } else {
            if (pOutbufOps->Setup(hMFCHandle, MAX_OUTPUTBUFFER_NUM_DYNAMIC) != VIDEO_ERROR_NONE) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup output buffer");
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }

            /* get dpb count */
            nOutbufs = pMpeg2Dec->hMFCMpeg2Handle.maxDPBNum;
            ret = Exynos_Allocate_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX, nOutbufs, nAllocLen);
            if (ret != OMX_ErrorNone)
                goto EXIT;

            /* without Register output buffer */

            /* Enqueue output buffer */
            for (i = 0; i < nOutbufs; i++) {
                pOutbufOps->ExtensionEnqueue(hMFCHandle,
                                (void **)pVideoDec->pMFCDecOutputBuffer[i]->pVirAddr,
                                (int *)pVideoDec->pMFCDecOutputBuffer[i]->fd,
                                pVideoDec->pMFCDecOutputBuffer[i]->bufferSize,
                                nDataLen,
                                nPlaneCnt,
                                NULL);
            }
        }

        if (pOutbufOps->Run(hMFCHandle) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    } else if (pExynosOutputPort->bufferProcessType & BUFFER_SHARE) {
#ifdef USE_ANB
        if (pExynosOutputPort->bDynamicDPBMode == OMX_FALSE) {
            ExynosVideoPlane planes[MAX_BUFFER_PLANE];
            int plane;

            Exynos_OSAL_Memset((OMX_PTR)planes, 0, sizeof(ExynosVideoPlane) * MAX_BUFFER_PLANE);

            /* get dpb count */
            nOutbufs = pExynosOutputPort->portDefinition.nBufferCountActual;
            if (pOutbufOps->Setup(hMFCHandle, nOutbufs) != VIDEO_ERROR_NONE) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup output buffer");
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }

            if ((pExynosOutputPort->bIsANBEnabled == OMX_TRUE) &&
                (pExynosOutputPort->bStoreMetaData == OMX_FALSE)) {
                for (i = 0; i < pExynosOutputPort->assignedBufferNum; i++) {
                    for (plane = 0; plane < nPlaneCnt; plane++) {
                        planes[plane].fd = pExynosOutputPort->extendBufferHeader[i].buf_fd[plane];
                        planes[plane].addr = pExynosOutputPort->extendBufferHeader[i].pYUVBuf[plane];
                        planes[plane].allocSize = nAllocLen[plane];
                    }

                    if (pOutbufOps->Register(hMFCHandle, planes, nPlaneCnt) != VIDEO_ERROR_NONE) {
                        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to Register output buffer");
                        ret = OMX_ErrorInsufficientResources;
                        goto EXIT;
                    }
                    pOutbufOps->Enqueue(hMFCHandle,
                                        (void **)pExynosOutputPort->extendBufferHeader[i].pYUVBuf,
                                        nDataLen,
                                        nPlaneCnt,
                                        NULL);
                }

                if (pOutbufOps->Apply_RegisteredBuffer(hMFCHandle) != VIDEO_ERROR_NONE) {
                    Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to Apply output buffer");
                    ret = OMX_ErrorHardware;
                    goto EXIT;
                }
            } else {
                /*************/
                /*    TBD    */
                /*************/
                ret = OMX_ErrorNotImplemented;
                goto EXIT;
            }

        } else {
            /* get dpb count */
            nOutbufs = MAX_OUTPUTBUFFER_NUM_DYNAMIC;
            if (pOutbufOps->Setup(hMFCHandle, nOutbufs) != VIDEO_ERROR_NONE) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup output buffer");
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }

            if ((pExynosOutputPort->bIsANBEnabled == OMX_FALSE) &&
                (pExynosOutputPort->bStoreMetaData == OMX_FALSE)) {
                /*************/
                /*    TBD    */
                /*************/
                ret = OMX_ErrorNotImplemented;
                goto EXIT;
            }
        }
#else
        /*************/
        /*    TBD    */
        /*************/
        ret = OMX_ErrorNotImplemented;
        goto EXIT;
#endif
    }

    pMpeg2Dec->hMFCMpeg2Handle.bConfiguredMFCDst = OMX_TRUE;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     pComponentParameterStructure)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
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

    switch (nParamIndex) {
    case OMX_IndexParamVideoMpeg2:
    {
        OMX_VIDEO_PARAM_MPEG2TYPE *pDstMpeg2Param = (OMX_VIDEO_PARAM_MPEG2TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG2TYPE *pSrcMpeg2Param = NULL;
        EXYNOS_MPEG2DEC_HANDLE    *pMpeg2Dec      = NULL;
        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pDstMpeg2Param, sizeof(OMX_VIDEO_PARAM_MPEG2TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstMpeg2Param->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcMpeg2Param = &pMpeg2Dec->Mpeg2Component[pDstMpeg2Param->nPortIndex];

        Exynos_OSAL_Memcpy(((char *)pDstMpeg2Param) + nOffset,
                           ((char *)pSrcMpeg2Param) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_MPEG2TYPE) - nOffset);
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_MPEG2_DEC_ROLE);
    }
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        ret = GetIndexToProfileLevel(pExynosComponent, pDstProfileLevel);
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG2TYPE *pSrcMpeg2Component = NULL;
        EXYNOS_MPEG2DEC_HANDLE      *pMpeg2Dec = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcMpeg2Component = &pMpeg2Dec->Mpeg2Component[pDstProfileLevel->nPortIndex];

        pDstProfileLevel->eProfile = pSrcMpeg2Component->eProfile;
        pDstProfileLevel->eLevel = pSrcMpeg2Component->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        EXYNOS_MPEG2DEC_HANDLE              *pMpeg2Dec               = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pMpeg2Dec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE  *portDefinition     = (OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;
        OMX_U32                        portIndex          = portDefinition->nPortIndex;
        EXYNOS_OMX_BASECOMPONENT      *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
        EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
        EXYNOS_MPEG2DEC_HANDLE        *pMpeg2Dec          = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
        EXYNOS_OMX_BASEPORT           *pExynosPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

        ret = Exynos_OMX_VideoDecodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (((pExynosPort->bIsANBEnabled == OMX_TRUE) || (pExynosPort->bStoreMetaData == OMX_TRUE)) &&
            (pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.bInterlaced == VIDEO_TRUE) &&
            (portIndex == OUTPUT_PORT_INDEX)) {
            portDefinition->format.video.eColorFormat =
                (OMX_COLOR_FORMATTYPE)Exynos_OSAL_OMX2HALPixelFormat((OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatYUV420SemiPlanarInterlace, pExynosPort->ePlaneType);
        }

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "portDefinition->format.video.eColorFormat: 0x%x", portDefinition->format.video.eColorFormat);
    }
        break;
    default:
        ret = Exynos_OMX_VideoDecodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
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

    switch (nIndex) {
    case OMX_IndexParamVideoMpeg2:
    {
        OMX_VIDEO_PARAM_MPEG2TYPE *pDstMpeg2Param = NULL;
        OMX_VIDEO_PARAM_MPEG2TYPE *pSrcMpeg2Param = (OMX_VIDEO_PARAM_MPEG2TYPE *)pComponentParameterStructure;
        EXYNOS_MPEG2DEC_HANDLE    *pMpeg2Dec      = NULL;
        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pSrcMpeg2Param, sizeof(OMX_VIDEO_PARAM_MPEG2TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcMpeg2Param->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstMpeg2Param = &pMpeg2Dec->Mpeg2Component[pSrcMpeg2Param->nPortIndex];

        Exynos_OSAL_Memcpy(((char *)pDstMpeg2Param) + nOffset,
                           ((char *)pSrcMpeg2Param) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_MPEG2TYPE) - nOffset);
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pExynosComponent->currentState != OMX_StateLoaded) && (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_MPEG2_DEC_ROLE)) {
            pExynosComponent->pExynosPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG2;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_MPEG2TYPE *pDstMpeg2Component = NULL;
        EXYNOS_MPEG2DEC_HANDLE      *pMpeg2Dec = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

        pDstMpeg2Component = &pMpeg2Dec->Mpeg2Component[pSrcProfileLevel->nPortIndex];

        if (OMX_FALSE == CheckProfileLevelSupport(pExynosComponent, pSrcProfileLevel)) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        pDstMpeg2Component->eProfile = pSrcProfileLevel->eProfile;
        pDstMpeg2Component->eLevel = pSrcProfileLevel->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        EXYNOS_MPEG2DEC_HANDLE                *pMpeg2Dec                 = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pMpeg2Dec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    default:
        ret = Exynos_OMX_VideoDecodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE  nIndex,
    OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
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
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    case OMX_IndexConfigCommonOutputCrop:
    {
        EXYNOS_MPEG2DEC_HANDLE  *pMpeg2Dec     = NULL;
        OMX_CONFIG_RECTTYPE     *pSrcRectType  = NULL;
        OMX_CONFIG_RECTTYPE     *pDstRectType  = NULL;

        pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

        if (pMpeg2Dec->hMFCMpeg2Handle.bConfiguredMFCSrc == OMX_FALSE) {
            ret = OMX_ErrorNotReady;
            break;
        }

        pDstRectType = (OMX_CONFIG_RECTTYPE *)pComponentConfigStructure;

        if ((pDstRectType->nPortIndex != INPUT_PORT_INDEX) &&
            (pDstRectType->nPortIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        EXYNOS_OMX_BASEPORT *pExynosPort = &pExynosComponent->pExynosPort[pDstRectType->nPortIndex];

        pSrcRectType = &(pExynosPort->cropRectangle);

        pDstRectType->nTop    = pSrcRectType->nTop;
        pDstRectType->nLeft   = pSrcRectType->nLeft;
        pDstRectType->nHeight = pSrcRectType->nHeight;
        pDstRectType->nWidth  = pSrcRectType->nWidth;
    }
        break;
    default:
        ret = Exynos_OMX_VideoDecodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE  nIndex,
    OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
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
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    switch (nIndex) {
    default:
        ret = Exynos_OMX_VideoDecodeSetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_GetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE  hComponent,
    OMX_IN  OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE   *pIndexType)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
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
    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    ret = Exynos_OMX_VideoDecodeGetExtensionIndex(hComponent, cParameterName, pIndexType);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_ComponentRoleEnum(
    OMX_HANDLETYPE hComponent,
    OMX_U8        *cRole,
    OMX_U32        nIndex)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex == (MAX_COMPONENT_ROLE_NUM-1)) {
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_MPEG2_DEC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE Exynos_Mpeg2Dec_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_MPEG2DEC_HANDLE          *pMpeg2Dec           = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    OMX_PTR                hMFCHandle = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;

    ExynosVideoDecOps       *pDecOps        = NULL;
    ExynosVideoDecBufferOps *pInbufOps      = NULL;
    ExynosVideoDecBufferOps *pOutbufOps     = NULL;
    ExynosVideoInstInfo     *pVideoInstInfo = &(pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo);

    CSC_METHOD csc_method = CSC_METHOD_SW;
    int i, plane;

    FunctionIn();

    pMpeg2Dec->hMFCMpeg2Handle.bConfiguredMFCSrc = OMX_FALSE;
    pMpeg2Dec->hMFCMpeg2Handle.bConfiguredMFCDst = OMX_FALSE;
    pExynosComponent->bUseFlagEOF = OMX_TRUE;
    pExynosComponent->bSaveFlagEOS = OMX_FALSE;
    pExynosComponent->bBehaviorEOS = OMX_FALSE;
    pVideoDec->bDiscardCSDError = OMX_FALSE;

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] CodecOpen W: %d H:%d  Bitrate:%d FPS:%d", pExynosComponent, __FUNCTION__,
                                                                                              pExynosInputPort->portDefinition.format.video.nFrameWidth,
                                                                                              pExynosInputPort->portDefinition.format.video.nFrameHeight,
                                                                                              pExynosInputPort->portDefinition.format.video.nBitrate,
                                                                                              pExynosInputPort->portDefinition.format.video.xFramerate);

    pVideoInstInfo->nSize        = sizeof(ExynosVideoInstInfo);
    pVideoInstInfo->nWidth       = pExynosInputPort->portDefinition.format.video.nFrameWidth;
    pVideoInstInfo->nHeight      = pExynosInputPort->portDefinition.format.video.nFrameHeight;
    pVideoInstInfo->nBitrate     = pExynosInputPort->portDefinition.format.video.nBitrate;
    pVideoInstInfo->xFramerate   = pExynosInputPort->portDefinition.format.video.xFramerate;

    /* Mpeg2 Codec Open */
    ret = Mpeg2CodecOpen(pMpeg2Dec, pVideoInstInfo);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    pDecOps    = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    pInbufOps  = pMpeg2Dec->hMFCMpeg2Handle.pInbufOps;
    pOutbufOps = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;

    Exynos_SetPlaneToPort(pExynosInputPort, MFC_DEFAULT_INPUT_BUFFER_PLANE);
    if (pExynosInputPort->bufferProcessType & BUFFER_COPY) {
        unsigned int nAllocLen[MAX_BUFFER_PLANE] = {DEFAULT_MFC_INPUT_BUFFER_SIZE, 0, 0};
        Exynos_OSAL_SemaphoreCreate(&pExynosInputPort->codecSemID);
        Exynos_OSAL_QueueCreate(&pExynosInputPort->codecBufferQ, MAX_QUEUE_ELEMENTS);
        ret = Exynos_Allocate_CodecBuffers(pOMXComponent, INPUT_PORT_INDEX, MFC_INPUT_BUFFER_NUM_MAX, nAllocLen);
        if (ret != OMX_ErrorNone)
            goto EXIT;

        for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++)
            Exynos_CodecBufferEnQueue(pExynosComponent, INPUT_PORT_INDEX, pVideoDec->pMFCDecInputBuffer[i]);
    } else if (pExynosInputPort->bufferProcessType & BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    Exynos_SetPlaneToPort(pExynosOutputPort, MFC_DEFAULT_OUTPUT_BUFFER_PLANE);
    if (pExynosOutputPort->bufferProcessType & BUFFER_COPY) {
        Exynos_OSAL_SemaphoreCreate(&pExynosOutputPort->codecSemID);
        Exynos_OSAL_QueueCreate(&pExynosOutputPort->codecBufferQ, MAX_QUEUE_ELEMENTS);
    } else if (pExynosOutputPort->bufferProcessType & BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    pMpeg2Dec->bSourceStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pMpeg2Dec->hSourceStartEvent);
    pMpeg2Dec->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pMpeg2Dec->hDestinationStartEvent);

    INIT_ARRAY_TO_VAL(pExynosComponent->timeStamp, DEFAULT_TIMESTAMP_VAL, MAX_TIMESTAMP);
    Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pMpeg2Dec->hMFCMpeg2Handle.indexTimestamp = 0;
    pMpeg2Dec->hMFCMpeg2Handle.outputIndexTimestamp = 0;

    pExynosComponent->getAllDelayBuffer = OMX_FALSE;

    Exynos_OSAL_QueueCreate(&pMpeg2Dec->bypassBufferInfoQ, QUEUE_ELEMENTS);

#ifdef USE_CSC_HW
    csc_method = CSC_METHOD_HW;
#endif
    pVideoDec->csc_handle = csc_init(csc_method);
    if (pVideoDec->csc_handle == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }
    pVideoDec->csc_set_format = OMX_FALSE;

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Terminate */
OMX_ERRORTYPE Exynos_Mpeg2Dec_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_MPEG2DEC_HANDLE    *pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    OMX_PTR                hMFCHandle = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;

    ExynosVideoDecOps       *pDecOps    = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pMpeg2Dec->hMFCMpeg2Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;

    int i, plane;

    FunctionIn();

    if (pVideoDec->csc_handle != NULL) {
        csc_deinit(pVideoDec->csc_handle);
        pVideoDec->csc_handle = NULL;
    }

    Exynos_OSAL_QueueTerminate(&pMpeg2Dec->bypassBufferInfoQ);

    Exynos_OSAL_SignalTerminate(pMpeg2Dec->hDestinationStartEvent);
    pMpeg2Dec->hDestinationStartEvent = NULL;
    pMpeg2Dec->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalTerminate(pMpeg2Dec->hSourceStartEvent);
    pMpeg2Dec->hSourceStartEvent = NULL;
    pMpeg2Dec->bSourceStart = OMX_FALSE;

    if (pExynosOutputPort->bufferProcessType & BUFFER_COPY) {
        Exynos_Free_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX);
        Exynos_OSAL_QueueTerminate(&pExynosOutputPort->codecBufferQ);
        Exynos_OSAL_SemaphoreTerminate(pExynosOutputPort->codecSemID);
    } else if (pExynosOutputPort->bufferProcessType & BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    if (pExynosInputPort->bufferProcessType & BUFFER_COPY) {
        Exynos_Free_CodecBuffers(pOMXComponent, INPUT_PORT_INDEX);
        Exynos_OSAL_QueueTerminate(&pExynosInputPort->codecBufferQ);
        Exynos_OSAL_SemaphoreTerminate(pExynosInputPort->codecSemID);
    } else if (pExynosInputPort->bufferProcessType & BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }
    Mpeg2CodecClose(pMpeg2Dec);

    Exynos_ResetAllPortConfig(pOMXComponent);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_SrcIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec         = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG2DEC_HANDLE        *pMpeg2Dec         = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle        = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32                        oneFrameSize      = pSrcInputData->dataLen;

    ExynosVideoDecOps       *pDecOps     = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps   = pMpeg2Dec->hMFCMpeg2Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps  = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;
    ExynosVideoErrorType     codecReturn = VIDEO_ERROR_NONE;

    OMX_BUFFERHEADERTYPE tempBufferHeader;
    void *pPrivate = NULL;

    unsigned int nAllocLen[MAX_BUFFER_PLANE] = {0, 0, 0};
    unsigned int nDataLen[MAX_BUFFER_PLANE]  = {oneFrameSize, 0, 0};
    OMX_BOOL bInStartCode = OMX_FALSE;
    int i;

    FunctionIn();

    if (pMpeg2Dec->hMFCMpeg2Handle.bConfiguredMFCSrc == OMX_FALSE) {
        ret = Mpeg2CodecSrcSetup(pOMXComponent, pSrcInputData);
        goto EXIT;
    }

    if ((pMpeg2Dec->hMFCMpeg2Handle.bConfiguredMFCDst == OMX_FALSE) &&
        (pVideoDec->bForceHeaderParsing == OMX_FALSE)) {
        ret = Mpeg2CodecDstSetup(pOMXComponent);
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed to Mpeg2CodecDstSetup(0x%x)", __func__, __LINE__, ret);
            goto EXIT;
        }
    }

    if (((bInStartCode = Check_Mpeg2_StartCode(pSrcInputData->multiPlaneBuffer.dataBuffer[0], oneFrameSize)) == OMX_TRUE) ||
        ((pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
        pExynosComponent->timeStamp[pMpeg2Dec->hMFCMpeg2Handle.indexTimestamp] = pSrcInputData->timeStamp;
        pExynosComponent->nFlags[pMpeg2Dec->hMFCMpeg2Handle.indexTimestamp] = pSrcInputData->nFlags;
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] input timestamp %lld us (%.2f secs), Tag: %d, nFlags: 0x%x, oneFrameSize: %d", pExynosComponent, __FUNCTION__,
                                                        pSrcInputData->timeStamp, pSrcInputData->timeStamp / 1E6, pMpeg2Dec->hMFCMpeg2Handle.indexTimestamp, pSrcInputData->nFlags, oneFrameSize);
        pDecOps->Set_FrameTag(hMFCHandle, pMpeg2Dec->hMFCMpeg2Handle.indexTimestamp);
        pMpeg2Dec->hMFCMpeg2Handle.indexTimestamp++;
        pMpeg2Dec->hMFCMpeg2Handle.indexTimestamp %= MAX_TIMESTAMP;

        if ((pVideoDec->bQosChanged == OMX_TRUE) &&
            (pDecOps->Set_QosRatio != NULL)) {
            pDecOps->Set_QosRatio(hMFCHandle, pVideoDec->nQosRatio);
            pVideoDec->bQosChanged = OMX_FALSE;
        }

#ifdef PERFORMANCE_DEBUG
        Exynos_OSAL_V4L2CountIncrease(pExynosInputPort->hBufferCount, pSrcInputData->bufferHeader, INPUT_PORT_INDEX);
#endif

        /* queue work for input buffer */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] bufferHeader: 0x%x, dataBuffer: 0x%x", pExynosComponent, __FUNCTION__, pSrcInputData->bufferHeader, pSrcInputData->multiPlaneBuffer.dataBuffer[0]);

        if (pExynosInputPort->bufferProcessType & BUFFER_SHARE)
            nAllocLen[0] = pSrcInputData->bufferHeader->nAllocLen;
        else if (pExynosInputPort->bufferProcessType & BUFFER_COPY)
            nAllocLen[0] = DEFAULT_MFC_INPUT_BUFFER_SIZE;

        if (pExynosInputPort->bufferProcessType == BUFFER_COPY) {
            tempBufferHeader.nFlags     = pSrcInputData->nFlags;
            tempBufferHeader.nTimeStamp = pSrcInputData->timeStamp;
            pPrivate = (void *)&tempBufferHeader;
        } else {
            pPrivate = (void *)pSrcInputData->bufferHeader;
        }
        codecReturn = pInbufOps->ExtensionEnqueue(hMFCHandle,
                                (void **)pSrcInputData->multiPlaneBuffer.dataBuffer,
                                (int *)pSrcInputData->multiPlaneBuffer.fd,
                                nAllocLen,
                                nDataLen,
                                Exynos_GetPlaneFromPort(pExynosInputPort),
                                pPrivate);
        if (codecReturn != VIDEO_ERROR_NONE) {
            ret = (OMX_ERRORTYPE)OMX_ErrorCodecDecode;
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d", __FUNCTION__, __LINE__);
            goto EXIT;
        }
        Mpeg2CodecStart(pOMXComponent, INPUT_PORT_INDEX);
        if (pMpeg2Dec->bSourceStart == OMX_FALSE) {
            pMpeg2Dec->bSourceStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pMpeg2Dec->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
        if (pMpeg2Dec->bDestinationStart == OMX_FALSE) {
            pMpeg2Dec->bDestinationStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pMpeg2Dec->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    } else if (bInStartCode == OMX_FALSE) {
        ret = OMX_ErrorCorruptedFrame;
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_SrcOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG2DEC_HANDLE         *pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT     *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    ExynosVideoDecOps       *pDecOps    = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pMpeg2Dec->hMFCMpeg2Handle.pInbufOps;
    ExynosVideoBuffer       *pVideoBuffer;
    ExynosVideoBuffer        videoBuffer;

    FunctionIn();

    if (pInbufOps->ExtensionDequeue(hMFCHandle, &videoBuffer) == VIDEO_ERROR_NONE)
        pVideoBuffer = &videoBuffer;
    else
        pVideoBuffer = NULL;

    pSrcOutputData->dataLen       = 0;
    pSrcOutputData->usedDataLen   = 0;
    pSrcOutputData->remainDataLen = 0;
    pSrcOutputData->nFlags        = 0;
    pSrcOutputData->timeStamp     = 0;
    pSrcOutputData->bufferHeader  = NULL;

    if (pVideoBuffer == NULL) {
        pSrcOutputData->multiPlaneBuffer.dataBuffer[0] = NULL;
        pSrcOutputData->allocSize  = 0;
        pSrcOutputData->pPrivate = NULL;
    } else {
        pSrcOutputData->multiPlaneBuffer.dataBuffer[0] = pVideoBuffer->planes[0].addr;
        pSrcOutputData->multiPlaneBuffer.fd[0] = pVideoBuffer->planes[0].fd;
        pSrcOutputData->allocSize  = pVideoBuffer->planes[0].allocSize;

        if (pExynosInputPort->bufferProcessType & BUFFER_COPY) {
            int i;
            for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++) {
                if (pSrcOutputData->multiPlaneBuffer.dataBuffer[0] ==
                        pVideoDec->pMFCDecInputBuffer[i]->pVirAddr[0]) {
                    pVideoDec->pMFCDecInputBuffer[i]->dataSize = 0;
                    pSrcOutputData->pPrivate = pVideoDec->pMFCDecInputBuffer[i];
                    break;
                }
            }

            if (i >= MFC_INPUT_BUFFER_NUM_MAX) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not find buffer");
                ret = (OMX_ERRORTYPE)OMX_ErrorCodecDecode;
                goto EXIT;
            }
        }

        /* For Share Buffer */
        if (pExynosInputPort->bufferProcessType == BUFFER_SHARE)
            pSrcOutputData->bufferHeader = (OMX_BUFFERHEADERTYPE*)pVideoBuffer->pPrivate;

#ifdef PERFORMANCE_DEBUG
        Exynos_OSAL_V4L2CountDecrease(pExynosInputPort->hBufferCount, pSrcOutputData->bufferHeader, INPUT_PORT_INDEX);
#endif
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_DstIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG2DEC_HANDLE          *pMpeg2Dec          = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                            *hMFCHandle         = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT             *pExynosOutputPort  = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoDecOps       *pDecOps     = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    ExynosVideoDecBufferOps *pOutbufOps  = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;
    ExynosVideoErrorType     codecReturn = VIDEO_ERROR_NONE;

    unsigned int nAllocLen[MAX_BUFFER_PLANE] = {0, 0, 0};
    unsigned int nDataLen[MAX_BUFFER_PLANE]  = {0, 0, 0};
    int i, nPlaneCnt;

    FunctionIn();

    if (pDstInputData->multiPlaneBuffer.dataBuffer[0] == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to find input buffer");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    nPlaneCnt = Exynos_GetPlaneFromPort(pExynosOutputPort);
    for (i = 0; i < nPlaneCnt; i++) {
        nAllocLen[i] = pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.nAlignPlaneSize[i];

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] : %d => ADDR[%d]: 0x%x", pExynosComponent, __FUNCTION__, __LINE__, i,
                                        pDstInputData->multiPlaneBuffer.dataBuffer[i]);
    }

    if ((pVideoDec->bReconfigDPB == OMX_TRUE) &&
        (pExynosOutputPort->bufferProcessType & BUFFER_SHARE) &&
        (pExynosOutputPort->exceptionFlag == GENERAL_STATE)) {
        ret = Mpeg2CodecDstSetup(pOMXComponent);
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed to Mpeg2CodecDstSetup(0x%x)", __func__, __LINE__, ret);
            goto EXIT;
        }
        pVideoDec->bReconfigDPB = OMX_FALSE;
    }

#ifdef PERFORMANCE_DEBUG
    Exynos_OSAL_V4L2CountIncrease(pExynosOutputPort->hBufferCount, pDstInputData->bufferHeader, OUTPUT_PORT_INDEX);
#endif

    if (pExynosOutputPort->bDynamicDPBMode == OMX_FALSE) {
        codecReturn = pOutbufOps->Enqueue(hMFCHandle,
                                          (void **)pDstInputData->multiPlaneBuffer.dataBuffer,
                                          nDataLen,
                                          nPlaneCnt,
                                          pDstInputData->bufferHeader);
    } else {
        codecReturn = pOutbufOps->ExtensionEnqueue(hMFCHandle,
                                    (void **)pDstInputData->multiPlaneBuffer.dataBuffer,
                                    (int *)pDstInputData->multiPlaneBuffer.fd,
                                    nAllocLen,
                                    nDataLen,
                                    nPlaneCnt,
                                    pDstInputData->bufferHeader);
    }

    if (codecReturn != VIDEO_ERROR_NONE) {
        if (codecReturn != VIDEO_ERROR_WRONGBUFFERSIZE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : %d", __FUNCTION__, __LINE__);
            ret = (OMX_ERRORTYPE)OMX_ErrorCodecDecode;
        }
        goto EXIT;
    }
    Mpeg2CodecStart(pOMXComponent, OUTPUT_PORT_INDEX);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_DstOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG2DEC_HANDLE          *pMpeg2Dec          = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                            *hMFCHandle         = pMpeg2Dec->hMFCMpeg2Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT             *pExynosInputPort   = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT             *pExynosOutputPort  = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    DECODE_CODEC_EXTRA_BUFFERINFO   *pBufferInfo        = NULL;

    ExynosVideoDecOps           *pDecOps        = pMpeg2Dec->hMFCMpeg2Handle.pDecOps;
    ExynosVideoDecBufferOps     *pOutbufOps     = pMpeg2Dec->hMFCMpeg2Handle.pOutbufOps;
    ExynosVideoBuffer           *pVideoBuffer   = NULL;
    ExynosVideoBuffer            videoBuffer;
    ExynosVideoFrameStatusType   displayStatus  = VIDEO_FRAME_STATUS_UNKNOWN;
    ExynosVideoGeometry         *bufferGeometry = NULL;
    ExynosVideoErrorType         codecReturn    = VIDEO_ERROR_NONE;

    OMX_S32 indexTimestamp = 0;
    int plane, nPlaneCnt;

    FunctionIn();

    if (pMpeg2Dec->bDestinationStart == OMX_FALSE) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    while (1) {
        if (pExynosOutputPort->bDynamicDPBMode == OMX_FALSE) {
            pVideoBuffer = pOutbufOps->Dequeue(hMFCHandle);
            if (pVideoBuffer == (ExynosVideoBuffer *)VIDEO_ERROR_DQBUF_EIO) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "HW is not available");
                ret = OMX_ErrorHardware;
                goto EXIT;
            }

            if (pVideoBuffer == NULL) {
                ret = OMX_ErrorNone;
                goto EXIT;
            }
        } else {
            Exynos_OSAL_Memset(&videoBuffer, 0, sizeof(ExynosVideoBuffer));

            codecReturn = pOutbufOps->ExtensionDequeue(hMFCHandle, &videoBuffer);
            if (codecReturn == VIDEO_ERROR_NONE) {
                pVideoBuffer = &videoBuffer;
            } else if (codecReturn == VIDEO_ERROR_DQBUF_EIO) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "HW is not available");
                pVideoBuffer = NULL;
                ret = OMX_ErrorHardware;
                goto EXIT;
            } else {
                pVideoBuffer = NULL;
                ret = OMX_ErrorNone;
                goto EXIT;
            }
        }

        displayStatus = pVideoBuffer->displayStatus;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] displayStatus: 0x%x", pExynosComponent, __FUNCTION__, displayStatus);

        if ((displayStatus == VIDEO_FRAME_STATUS_DISPLAY_DECODING) ||
            (displayStatus == VIDEO_FRAME_STATUS_DISPLAY_ONLY) ||
            (displayStatus == VIDEO_FRAME_STATUS_CHANGE_RESOL) ||
            (displayStatus == VIDEO_FRAME_STATUS_DECODING_FINISHED) ||
            (displayStatus == VIDEO_FRAME_STATUS_LAST_FRAME) ||
            (CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            ret = OMX_ErrorNone;
            break;
        }
    }

   if ((pVideoDec->bThumbnailMode == OMX_FALSE) &&
       (displayStatus == VIDEO_FRAME_STATUS_CHANGE_RESOL)) {
        if (pVideoDec->bReconfigDPB != OMX_TRUE) {
            pExynosOutputPort->exceptionFlag = NEED_PORT_FLUSH;
            pVideoDec->bReconfigDPB = OMX_TRUE;
            Mpeg2CodecUpdateResolution(pOMXComponent);
            pVideoDec->csc_set_format = OMX_FALSE;
        }
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    pMpeg2Dec->hMFCMpeg2Handle.outputIndexTimestamp++;
    pMpeg2Dec->hMFCMpeg2Handle.outputIndexTimestamp %= MAX_TIMESTAMP;

    pDstOutputData->allocSize = pDstOutputData->dataLen = 0;
    nPlaneCnt = Exynos_GetPlaneFromPort(pExynosOutputPort);
    for (plane = 0; plane < nPlaneCnt; plane++) {
        pDstOutputData->multiPlaneBuffer.dataBuffer[plane] = pVideoBuffer->planes[plane].addr;
        pDstOutputData->multiPlaneBuffer.fd[plane] = pVideoBuffer->planes[plane].fd;
        pDstOutputData->allocSize += pVideoBuffer->planes[plane].allocSize;
        pDstOutputData->dataLen +=  pVideoBuffer->planes[plane].dataSize;
    }
    pDstOutputData->usedDataLen = 0;
    pDstOutputData->pPrivate = pVideoBuffer;
    if (pExynosOutputPort->bufferProcessType & BUFFER_COPY) {
        int i = 0;
        pDstOutputData->pPrivate = NULL;
        for (i = 0; i < MFC_OUTPUT_BUFFER_NUM_MAX; i++) {
            if (pDstOutputData->multiPlaneBuffer.dataBuffer[0] ==
                pVideoDec->pMFCDecOutputBuffer[i]->pVirAddr[0]) {
                pDstOutputData->pPrivate = pVideoDec->pMFCDecOutputBuffer[i];
                break;
            }
        }

        if (pDstOutputData->pPrivate == NULL) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not find buffer");
            ret = (OMX_ERRORTYPE)OMX_ErrorCodecDecode;
            goto EXIT;
        }
    }

    /* For Share Buffer */
    pDstOutputData->bufferHeader = (OMX_BUFFERHEADERTYPE *)pVideoBuffer->pPrivate;

    /* get interlace frame info */
    if ((pExynosOutputPort->bufferProcessType & BUFFER_SHARE) &&
        (pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf.bInterlaced == VIDEO_TRUE) &&
        (pVideoBuffer->planes[2].addr != NULL)) {
        /* only NV12 case */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] interlace type = %x", pExynosComponent, __FUNCTION__, pVideoBuffer->interlacedType);
        *(int *)(pVideoBuffer->planes[2].addr) = pVideoBuffer->interlacedType;
    }

    pBufferInfo = (DECODE_CODEC_EXTRA_BUFFERINFO *)pDstOutputData->extInfo;
    bufferGeometry = &pMpeg2Dec->hMFCMpeg2Handle.codecOutbufConf;
    pBufferInfo->imageWidth = bufferGeometry->nFrameWidth;
    pBufferInfo->imageHeight = bufferGeometry->nFrameHeight;
    pBufferInfo->imageStride = bufferGeometry->nStride;
    pBufferInfo->ColorFormat = Exynos_OSAL_Video2OMXFormat((int)bufferGeometry->eColorFormat);
    Exynos_OSAL_Memcpy(&pBufferInfo->PDSB, &pVideoBuffer->PDSB, sizeof(PrivateDataShareBuffer));

    indexTimestamp = pDecOps->Get_FrameTag(hMFCHandle);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] out indexTimestamp: %d", pExynosComponent, __FUNCTION__, indexTimestamp);
    if ((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)) {
        if ((pExynosComponent->checkTimeStamp.needSetStartTimeStamp != OMX_TRUE) &&
            (pExynosComponent->checkTimeStamp.needCheckStartTimeStamp != OMX_TRUE)) {
            if (indexTimestamp == INDEX_AFTER_EOS) {
                pDstOutputData->timeStamp = 0x00;
                pDstOutputData->nFlags = 0x00;
            } else {
                pDstOutputData->timeStamp = pExynosComponent->timeStamp[pMpeg2Dec->hMFCMpeg2Handle.outputIndexTimestamp];
                pDstOutputData->nFlags = pExynosComponent->nFlags[pMpeg2Dec->hMFCMpeg2Handle.outputIndexTimestamp];
                Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] missing out indexTimestamp: %d", pExynosComponent, __FUNCTION__, indexTimestamp);
            }
        } else {
            pDstOutputData->timeStamp = 0x00;
            pDstOutputData->nFlags = 0x00;
        }
    } else {
        /* For timestamp correction. if mfc support frametype detect */
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] disp_pic_frame_type: %d", pExynosComponent, __FUNCTION__, pVideoBuffer->frameType);

        /* NEED TIMESTAMP REORDER */
        if (pVideoDec->bDTSMode == OMX_TRUE) {
            if ((pVideoBuffer->frameType & VIDEO_FRAME_I) ||
                ((pVideoBuffer->frameType & VIDEO_FRAME_OTHERS) &&
                    ((pExynosComponent->nFlags[indexTimestamp] & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) ||
                (pExynosComponent->checkTimeStamp.needCheckStartTimeStamp == OMX_TRUE))
                pMpeg2Dec->hMFCMpeg2Handle.outputIndexTimestamp = indexTimestamp;
            else
                indexTimestamp = pMpeg2Dec->hMFCMpeg2Handle.outputIndexTimestamp;
        }

        pDstOutputData->timeStamp   = pExynosComponent->timeStamp[indexTimestamp];
        pDstOutputData->nFlags      = pExynosComponent->nFlags[indexTimestamp] | OMX_BUFFERFLAG_ENDOFFRAME;

        if (pVideoBuffer->frameType & VIDEO_FRAME_I)
            pDstOutputData->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

        if (pVideoBuffer->frameType & VIDEO_FRAME_CORRUPT)
            pDstOutputData->nFlags |= OMX_BUFFERFLAG_DATACORRUPT;

        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] output timestamp %lld us (%.2f secs), indexTimestamp: %d, nFlags: 0x%x", pExynosComponent, __FUNCTION__,
                                                            pDstOutputData->timeStamp, pDstOutputData->timeStamp / 1E6, indexTimestamp, pDstOutputData->nFlags);
    }

#ifdef PERFORMANCE_DEBUG
    if (pDstOutputData->bufferHeader != NULL) {
        pDstOutputData->bufferHeader->nTimeStamp = pDstOutputData->timeStamp;
        Exynos_OSAL_V4L2CountDecrease(pExynosOutputPort->hBufferCount, pDstOutputData->bufferHeader, OUTPUT_PORT_INDEX);
    }
#endif

    if (pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo.specificInfo.dec.bLastFrameSupport == VIDEO_FALSE) {
        if ((!(pVideoBuffer->frameType & VIDEO_FRAME_B)) &&
            (pExynosComponent->bSaveFlagEOS == OMX_TRUE)) {
            pDstOutputData->nFlags |= OMX_BUFFERFLAG_EOS;
        }

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] displayStatus:%d, nFlags0x%x", pExynosComponent, __FUNCTION__, displayStatus, pDstOutputData->nFlags);
        if (displayStatus == VIDEO_FRAME_STATUS_DECODING_FINISHED) {
            pDstOutputData->remainDataLen = 0;

            if ((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)) {
                if (indexTimestamp != INDEX_AFTER_EOS)
                    Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "[%p][%s] indexTimestamp(%d) is wrong", pExynosComponent, __FUNCTION__, indexTimestamp);
                pDstOutputData->timeStamp   = 0x00;
                pDstOutputData->nFlags      = 0x00;
                goto EXIT;
            }

            if ((pExynosComponent->nFlags[indexTimestamp] & OMX_BUFFERFLAG_EOS) ||
                (pExynosComponent->bSaveFlagEOS == OMX_TRUE)) {
                pDstOutputData->nFlags |= OMX_BUFFERFLAG_EOS;
                pExynosComponent->nFlags[indexTimestamp] &= (~OMX_BUFFERFLAG_EOS);
            }
        } else if ((pDstOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            pDstOutputData->remainDataLen = 0;

            if (pExynosComponent->bBehaviorEOS == OMX_TRUE) {
                pDstOutputData->remainDataLen = bufferGeometry->nFrameWidth * bufferGeometry->nFrameHeight * 3 / 2;

                if (!(pVideoBuffer->frameType & VIDEO_FRAME_B)) {
                    pExynosComponent->bBehaviorEOS = OMX_FALSE;
                } else {
                    pExynosComponent->bSaveFlagEOS = OMX_TRUE;
                    pDstOutputData->nFlags &= (~OMX_BUFFERFLAG_EOS);
                }
            }
        } else {
            pDstOutputData->remainDataLen = bufferGeometry->nFrameWidth * bufferGeometry->nFrameHeight * 3 / 2;
        }
    } else {
        if ((displayStatus == VIDEO_FRAME_STATUS_DECODING_FINISHED) ||
            (displayStatus == VIDEO_FRAME_STATUS_LAST_FRAME) ||
            ((pDstOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
            Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] displayStatus:%d, nFlags0x%x", pExynosComponent, __FUNCTION__, displayStatus, pDstOutputData->nFlags);
            pDstOutputData->remainDataLen = 0;

            if ((pExynosComponent->bBehaviorEOS == OMX_TRUE) ||
                (displayStatus == VIDEO_FRAME_STATUS_LAST_FRAME)) {
                pDstOutputData->remainDataLen = bufferGeometry->nFrameWidth * bufferGeometry->nFrameHeight * 3 / 2;

                if (displayStatus != VIDEO_FRAME_STATUS_LAST_FRAME) {
                    pDstOutputData->nFlags &= (~OMX_BUFFERFLAG_EOS);
                } else {
                    pDstOutputData->nFlags |= OMX_BUFFERFLAG_EOS;
                    pExynosComponent->bBehaviorEOS = OMX_FALSE;
                }
            }
        } else {
            pDstOutputData->remainDataLen = bufferGeometry->nFrameWidth * bufferGeometry->nFrameHeight * 3 / 2;
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_srcInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT             *pExynosInputPort   = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosInputPort)) || (!CHECK_PORT_POPULATED(pExynosInputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    ret = Exynos_Mpeg2Dec_SrcIn(pOMXComponent, pSrcInputData);
    if ((ret != OMX_ErrorNone) &&
        ((EXYNOS_OMX_ERRORTYPE)ret != OMX_ErrorInputDataDecodeYet) &&
        ((EXYNOS_OMX_ERRORTYPE)ret != OMX_ErrorCorruptedFrame)) {

        if (((EXYNOS_OMX_ERRORTYPE)ret == OMX_ErrorCorruptedHeader) &&
            (pVideoDec->bDiscardCSDError == OMX_TRUE)) {
            goto EXIT;
        }

        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_srcOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG2DEC_HANDLE          *pMpeg2Dec          = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    EXYNOS_OMX_BASEPORT             *pExynosInputPort   = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosInputPort)) || (!CHECK_PORT_POPULATED(pExynosInputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pExynosInputPort->bufferProcessType & BUFFER_COPY) {
        if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX)) {
            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }
    if ((pMpeg2Dec->bSourceStart == OMX_FALSE) &&
       (!CHECK_PORT_BEING_FLUSHED(pExynosInputPort))) {
        Exynos_OSAL_SignalWait(pMpeg2Dec->hSourceStartEvent, DEF_MAX_WAIT_TIME);
        if (pVideoDec->bExitBufferProcessThread)
            goto EXIT;

        Exynos_OSAL_SignalReset(pMpeg2Dec->hSourceStartEvent);
    }

    ret = Exynos_Mpeg2Dec_SrcOut(pOMXComponent, pSrcOutputData);
    if ((ret != OMX_ErrorNone) &&
        (pExynosComponent->currentState == OMX_StateExecuting)) {
        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_dstInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG2DEC_HANDLE          *pMpeg2Dec          = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    EXYNOS_OMX_BASEPORT             *pExynosOutputPort  = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosOutputPort)) || (!CHECK_PORT_POPULATED(pExynosOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) {
        if (pExynosComponent->currentState == OMX_StatePause)
            ret = OMX_ErrorOutputBufferUseYet;
        else
            ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (pExynosOutputPort->bufferProcessType & BUFFER_SHARE) {
        if ((pMpeg2Dec->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pMpeg2Dec->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            if (pVideoDec->bExitBufferProcessThread)
                goto EXIT;

            Exynos_OSAL_SignalReset(pMpeg2Dec->hDestinationStartEvent);
        }
        if (Exynos_OSAL_GetElemNum(&pMpeg2Dec->bypassBufferInfoQ) > 0) {
            BYPASS_BUFFER_INFO *pBufferInfo = (BYPASS_BUFFER_INFO *)Exynos_OSAL_Dequeue(&pMpeg2Dec->bypassBufferInfoQ);
            if (pBufferInfo == NULL) {
                ret = OMX_ErrorUndefined;
                goto EXIT;
            }

            pDstInputData->bufferHeader->nFlags     = pBufferInfo->nFlags;
            pDstInputData->bufferHeader->nTimeStamp = pBufferInfo->timeStamp;
            Exynos_OMX_OutputBufferReturn(pOMXComponent, pDstInputData->bufferHeader);
            Exynos_OSAL_Free(pBufferInfo);

            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }
    if (pMpeg2Dec->hMFCMpeg2Handle.bConfiguredMFCDst == OMX_TRUE) {
        ret = Exynos_Mpeg2Dec_DstIn(pOMXComponent, pDstInputData);
        if (ret != OMX_ErrorNone) {
            pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
        }
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_Mpeg2Dec_dstOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_MPEG2DEC_HANDLE          *pMpeg2Dec          = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    EXYNOS_OMX_BASEPORT             *pExynosOutputPort  = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosOutputPort)) || (!CHECK_PORT_POPULATED(pExynosOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pExynosOutputPort->bufferProcessType & BUFFER_COPY) {
        if ((pMpeg2Dec->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pMpeg2Dec->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            if (pVideoDec->bExitBufferProcessThread)
                goto EXIT;

            Exynos_OSAL_SignalReset(pMpeg2Dec->hDestinationStartEvent);
        }
        if (Exynos_OSAL_GetElemNum(&pMpeg2Dec->bypassBufferInfoQ) > 0) {
            EXYNOS_OMX_DATABUFFER *dstOutputUseBuffer   = &pExynosOutputPort->way.port2WayDataBuffer.outputDataBuffer;
            OMX_BUFFERHEADERTYPE  *pOMXBuffer           = NULL;
            BYPASS_BUFFER_INFO    *pBufferInfo          = NULL;

            if (dstOutputUseBuffer->dataValid == OMX_FALSE) {
                pOMXBuffer = Exynos_OutputBufferGetQueue_Direct(pExynosComponent);
                if (pOMXBuffer == NULL) {
                    ret = OMX_ErrorUndefined;
                    goto EXIT;
                }
            } else {
                pOMXBuffer = dstOutputUseBuffer->bufferHeader;
            }

            pBufferInfo = Exynos_OSAL_Dequeue(&pMpeg2Dec->bypassBufferInfoQ);
            if (pBufferInfo == NULL) {
                ret = OMX_ErrorUndefined;
                goto EXIT;
            }

            pOMXBuffer->nFlags      = pBufferInfo->nFlags;
            pOMXBuffer->nTimeStamp  = pBufferInfo->timeStamp;
            Exynos_OMX_OutputBufferReturn(pOMXComponent, pOMXBuffer);
            Exynos_OSAL_Free(pBufferInfo);

            dstOutputUseBuffer->dataValid = OMX_FALSE;

            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }
    ret = Exynos_Mpeg2Dec_DstOut(pOMXComponent, pDstOutputData);
    if ((ret != OMX_ErrorNone) &&
        (pExynosComponent->currentState == OMX_StateExecuting)) {
        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
    }

EXIT:
    FunctionOut();

    return ret;
}

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(
    OMX_HANDLETYPE  hComponent,
    OMX_STRING      componentName)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = NULL;
    EXYNOS_OMX_BASEPORT             *pExynosPort        = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = NULL;
    EXYNOS_MPEG2DEC_HANDLE            *pMpeg2Dec            = NULL;
    int i = 0;

    Exynos_OSAL_Get_Log_Property(); // For debuging
    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    if ((Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_MPEG2_DEC, componentName) != 0) &&
        (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_MPEG2_CUSTOM_DEC, componentName) != 0)) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, componentName:%s, Line:%d", componentName, __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_VideoDecodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pExynosComponent->codecType = HW_VIDEO_DEC_CODEC;

    pExynosComponent->componentName = (OMX_STRING)Exynos_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pExynosComponent->componentName == NULL) {
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pMpeg2Dec = Exynos_OSAL_Malloc(sizeof(EXYNOS_MPEG2DEC_HANDLE));
    if (pMpeg2Dec == NULL) {
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pMpeg2Dec, 0, sizeof(EXYNOS_MPEG2DEC_HANDLE));
    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    pVideoDec->hCodecHandle = (OMX_HANDLETYPE)pMpeg2Dec;

    Exynos_OSAL_Strcpy(pExynosComponent->componentName, componentName);

    /* Set componentVersion */
    pExynosComponent->componentVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pExynosComponent->componentVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pExynosComponent->componentVersion.s.nRevision     = REVISION_NUMBER;
    pExynosComponent->componentVersion.s.nStep         = STEP_NUMBER;
    /* Set specVersion */
    pExynosComponent->specVersion.s.nVersionMajor = VERSIONMAJOR_NUMBER;
    pExynosComponent->specVersion.s.nVersionMinor = VERSIONMINOR_NUMBER;
    pExynosComponent->specVersion.s.nRevision     = REVISION_NUMBER;
    pExynosComponent->specVersion.s.nStep         = STEP_NUMBER;

    /* Input port */
    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pExynosPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pExynosPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pExynosPort->portDefinition.format.video.nSliceHeight = 0;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    if (IS_CUSTOM_COMPONENT(pExynosComponent->componentName) == OMX_TRUE)
        pExynosPort->portDefinition.nBufferSize = CUSTOM_DEFAULT_VIDEO_INPUT_BUFFER_SIZE;

    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingMPEG2;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/mpeg2");
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;
    //pExynosPort->bufferProcessType = BUFFER_SHARE;
    pExynosPort->bufferProcessType = BUFFER_COPY;
    pExynosPort->portWayType = WAY2_PORT;
    pExynosPort->ePlaneType = PLANE_SINGLE;

    /* Output port */
    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pExynosPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pExynosPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pExynosPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pExynosPort->portDefinition.format.video.nSliceHeight = 0;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "raw/video");
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;
    pExynosPort->bufferProcessType = BUFFER_COPY | BUFFER_ANBSHARE;
    pExynosPort->portWayType = WAY2_PORT;
    pExynosPort->ePlaneType = PLANE_MULTIPLE;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        INIT_SET_SIZE_VERSION(&pMpeg2Dec->Mpeg2Component[i], OMX_VIDEO_PARAM_MPEG2TYPE);
        pMpeg2Dec->Mpeg2Component[i].nPortIndex = i;
        pMpeg2Dec->Mpeg2Component[i].eProfile = OMX_VIDEO_MPEG2ProfileMain;
        pMpeg2Dec->Mpeg2Component[i].eLevel = OMX_VIDEO_MPEG2LevelML; /* Check again**** */
    }

    pOMXComponent->GetParameter      = &Exynos_Mpeg2Dec_GetParameter;
    pOMXComponent->SetParameter      = &Exynos_Mpeg2Dec_SetParameter;
    pOMXComponent->GetConfig         = &Exynos_Mpeg2Dec_GetConfig;
    pOMXComponent->SetConfig         = &Exynos_Mpeg2Dec_SetConfig;
    pOMXComponent->GetExtensionIndex = &Exynos_Mpeg2Dec_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &Exynos_Mpeg2Dec_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &Exynos_OMX_ComponentDeinit;

    pExynosComponent->exynos_codec_componentInit      = &Exynos_Mpeg2Dec_Init;
    pExynosComponent->exynos_codec_componentTerminate = &Exynos_Mpeg2Dec_Terminate;

    pVideoDec->exynos_codec_srcInputProcess  = &Exynos_Mpeg2Dec_srcInputBufferProcess;
    pVideoDec->exynos_codec_srcOutputProcess = &Exynos_Mpeg2Dec_srcOutputBufferProcess;
    pVideoDec->exynos_codec_dstInputProcess  = &Exynos_Mpeg2Dec_dstInputBufferProcess;
    pVideoDec->exynos_codec_dstOutputProcess = &Exynos_Mpeg2Dec_dstOutputBufferProcess;

    pVideoDec->exynos_codec_start         = &Mpeg2CodecStart;
    pVideoDec->exynos_codec_stop          = &Mpeg2CodecStop;
    pVideoDec->exynos_codec_bufferProcessRun = &Mpeg2CodecOutputBufferProcessRun;
    pVideoDec->exynos_codec_enqueueAllBuffer = &Mpeg2CodecEnQueueAllBuffer;

#if 0  /* unused code */
    pVideoDec->exynos_checkInputFrame                 = &Check_Mpeg2_Frame;
    pVideoDec->exynos_codec_getCodecInputPrivateData  = &GetCodecInputPrivateData;
#endif

    pVideoDec->exynos_codec_getCodecOutputPrivateData = &GetCodecOutputPrivateData;
    pVideoDec->exynos_codec_reconfigAllBuffers        = &Mpeg2CodecReconfigAllBuffers;

    pVideoDec->exynos_codec_checkFormatSupport      = &CheckFormatHWSupport;
    pVideoDec->exynos_codec_checkResolutionChange   = &Mpeg2CodecCheckResolution;

    pVideoDec->hSharedMemory = Exynos_OSAL_SharedMemory_Open();
    if (pVideoDec->hSharedMemory == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        Exynos_OSAL_Free(pMpeg2Dec);
        pMpeg2Dec = ((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo.eCodecType = VIDEO_CODING_MPEG2;
    if (pExynosComponent->codecType == HW_VIDEO_DEC_SECURE_CODEC)
        pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo.eSecurityType = VIDEO_SECURE;
    else
        pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo.eSecurityType = VIDEO_NORMAL;

    if (Exynos_Video_GetInstInfo(&(pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo), VIDEO_TRUE /* dec */) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%p][%s]: Exynos_Video_GetInstInfo is failed", pExynosComponent, __FUNCTION__);
        Exynos_OSAL_Free(pMpeg2Dec);
        pMpeg2Dec = ((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] GetInstInfo for dec %d %d %d %d", pExynosComponent, __FUNCTION__,
            (pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo.specificInfo.dec.bDualDPBSupport),
            (pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo.specificInfo.dec.bDynamicDPBSupport),
            (pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo.specificInfo.dec.bLastFrameSupport),
            (pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo.specificInfo.dec.bSkypeSupport));

    if (pMpeg2Dec->hMFCMpeg2Handle.videoInstInfo.specificInfo.dec.bDynamicDPBSupport == VIDEO_TRUE)
        pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].bDynamicDPBMode = OMX_TRUE;

    Exynos_Output_SetSupportFormat(pExynosComponent);
    SetProfileLevel(pExynosComponent);

    pExynosComponent->currentState = OMX_StateLoaded;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(
    OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE           *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent   = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_MPEG2DEC_HANDLE        *pMpeg2Dec            = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;

    Exynos_OSAL_SharedMemory_Close(pVideoDec->hSharedMemory);

    Exynos_OSAL_Free(pExynosComponent->componentName);
    pExynosComponent->componentName = NULL;

    pMpeg2Dec = (EXYNOS_MPEG2DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pMpeg2Dec != NULL) {
        Exynos_OSAL_Free(pMpeg2Dec);
        pMpeg2Dec = pVideoDec->hCodecHandle = NULL;
    }

    ret = Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}
