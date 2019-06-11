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
 * @file        Exynos_OMX_Vp8dec.c
 * @brief
 * @author      Satish Kumar Reddy (palli.satish@samsung.com)
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
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
#include "Exynos_OMX_Vp8dec.h"
#include "ExynosVideoApi.h"
#include "Exynos_OSAL_SharedMemory.h"
#include "Exynos_OSAL_Event.h"

/* To use CSC_METHOD_HW in EXYNOS OMX, gralloc should allocate physical memory using FIMC */
/* It means GRALLOC_USAGE_HW_FIMC1 should be set on Native Window usage */
#include "csc.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_VP8_DEC"
//#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

#define VP8_DEC_NUM_OF_EXTRA_BUFFERS 7

//#define FULL_FRAME_SEARCH /* Full frame search not support*/

static OMX_ERRORTYPE SetProfileLevel(
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    OMX_ERRORTYPE                    ret            = OMX_ErrorNone;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec      = NULL;
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec        = NULL;

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

    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pVp8Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVp8Dec->hMFCVp8Handle.profiles[nProfileCnt++] = OMX_VIDEO_VP8ProfileMain;
    pVp8Dec->hMFCVp8Handle.nProfileCnt = nProfileCnt;
    pVp8Dec->hMFCVp8Handle.maxLevel = OMX_VIDEO_VP8Level_Version3;

EXIT:
    return ret;
}

static OMX_ERRORTYPE GetIndexToProfileLevel(
    EXYNOS_OMX_BASECOMPONENT         *pExynosComponent,
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *pProfileLevelType)
{
    OMX_ERRORTYPE                    ret           = OMX_ErrorNone;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec     = NULL;
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec       = NULL;

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

    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pVp8Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    while ((pVp8Dec->hMFCVp8Handle.maxLevel >> nLevelCnt) > 0) {
        nLevelCnt++;
    }

    if ((pVp8Dec->hMFCVp8Handle.nProfileCnt == 0) ||
        (nLevelCnt == 0)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : there is no any profile/level", __FUNCTION__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    nMaxIndex = pVp8Dec->hMFCVp8Handle.nProfileCnt * nLevelCnt;
    if (nMaxIndex <= pProfileLevelType->nProfileIndex) {
        ret = OMX_ErrorNoMore;
        goto EXIT;
    }

    pProfileLevelType->eProfile = pVp8Dec->hMFCVp8Handle.profiles[pProfileLevelType->nProfileIndex / nLevelCnt];
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
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec    = NULL;

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

    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pVp8Dec == NULL)
        goto EXIT;

    while ((pVp8Dec->hMFCVp8Handle.maxLevel >> nLevelCnt++) > 0);

    if ((pVp8Dec->hMFCVp8Handle.nProfileCnt == 0) ||
        (nLevelCnt == 0)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : there is no any profile/level", __FUNCTION__);
        goto EXIT;
    }

    for (i = 0; i < pVp8Dec->hMFCVp8Handle.nProfileCnt; i++) {
        if (pVp8Dec->hMFCVp8Handle.profiles[i] == pProfileLevelType->eProfile) {
            bProfileSupport = OMX_TRUE;
            break;
        }
    }

    if (bProfileSupport != OMX_TRUE)
        goto EXIT;

    while (nLevelCnt >= 0) {
        if ((int)pProfileLevelType->eLevel == (0x1 << nLevelCnt)) {
            bLevelSupport = OMX_TRUE;
            break;
        }

        nLevelCnt--;
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
static int Check_VP8_Frame(
    OMX_U8   *pInputStream,
    OMX_U32   buffSize,
    OMX_U32   flag,
    OMX_BOOL  bPreviousFrameEOF,
    OMX_BOOL *pbEndOfFrame)
{
    /* Uncompressed data Chunk comprises a common
    (for key frames and interframes) 3-byte frame tag that
    contains four fields
    - 1-bit frame type (0 - key frame, 1 - inter frame)
    - 3-bit version number (0 - 3 are defined as four different
                                      profiles with different decoding complexity)
    - 1-bit show_frame flag ( 0 - current frame not for display,
                                          1 - current frame is for dispaly)
    - 19-bit field - size of the first data partition in bytes

    Key Frames : frame tag followed by 7 bytes of uncompressed
    data
    3-bytes : Start code (byte 0: 0x9d,byte 1: 0x01,byte 2: 0x2a)
    Next 4-bytes: Width & height, Horizontal and vertical scale information
    16 bits      :     (2 bits Horizontal Scale << 14) | Width (14 bits)
    16 bits      :     (2 bits Vertical Scale << 14) | Height (14 bits)
    */
    int width, height;
    int horizSscale, vertScale;

    FunctionIn();

    *pbEndOfFrame = OMX_TRUE;

    /*Check for Key frame*/
    if (!(pInputStream[0] & 0x01)){
        /* Key Frame  Start code*/
        if (pInputStream[3] != 0x9d || pInputStream[4] != 0x01 || pInputStream[5]!=0x2a) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, " VP8 Key Frame Start Code not Found");
            *pbEndOfFrame = OMX_FALSE;
        }
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, " VP8 Found Key Frame Start Code");
        width = (pInputStream[6] | (pInputStream[7] << 8)) & 0x3fff;
        horizSscale = pInputStream[7] >> 6;
        height = (pInputStream[8] | (pInputStream[9] << 8)) & 0x3fff;
        vertScale = pInputStream[9] >> 6;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "width = %d, height = %d, horizSscale = %d, vertScale = %d", width, height, horizSscale, vertScale);
    }

    FunctionOut();
    return (int)buffSize;
}
#endif

OMX_BOOL Check_VP8_StartCode(
    OMX_U8     *pInputStream,
    OMX_U32     streamSize)
{
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "streamSize: %d",streamSize);
    if (streamSize < 3) {
        return OMX_FALSE;
    }

    if (!(pInputStream[0] & 0x01)){
        /* Key Frame  Start code*/
        if (pInputStream[3] != 0x9d || pInputStream[4] != 0x01 || pInputStream[5]!=0x2a) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, " VP8 Key Frame Start Code not Found");
            return OMX_FALSE;
        }
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, " VP8 Found Key Frame Start Code");
    }

    return OMX_TRUE;
}

OMX_BOOL CheckFormatHWSupport(
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent,
    OMX_COLOR_FORMATTYPE         eColorFormat)
{
    OMX_BOOL                         ret            = OMX_FALSE;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec      = NULL;
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec        = NULL;
    EXYNOS_OMX_BASEPORT             *pOutputPort    = NULL;
    ExynosVideoColorFormatType       eVideoFormat   = VIDEO_CODING_UNKNOWN;
    int i;

    FunctionIn();

    if (pExynosComponent == NULL)
        goto EXIT;

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    if (pVideoDec == NULL)
        goto EXIT;

    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pVp8Dec == NULL)
        goto EXIT;
    pOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    eVideoFormat = (ExynosVideoColorFormatType)Exynos_OSAL_OMX2VideoFormat(eColorFormat, pOutputPort->ePlaneType);

    for (i = 0; i < VIDEO_COLORFORMAT_MAX; i++) {
        if (pVp8Dec->hMFCVp8Handle.videoInstInfo.supportFormat[i] == VIDEO_COLORFORMAT_UNKNOWN)
            break;

        if (pVp8Dec->hMFCVp8Handle.videoInstInfo.supportFormat[i] == eVideoFormat) {
            ret = OMX_TRUE;
            break;
        }
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE VP8CodecOpen(EXYNOS_VP8DEC_HANDLE *pVp8Dec, ExynosVideoInstInfo *pVideoInstInfo)
{
    OMX_ERRORTYPE            ret        = OMX_ErrorNone;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pVp8Dec == NULL) {
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

    pVp8Dec->hMFCVp8Handle.pDecOps    = pDecOps;
    pVp8Dec->hMFCVp8Handle.pInbufOps  = pInbufOps;
    pVp8Dec->hMFCVp8Handle.pOutbufOps = pOutbufOps;

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
    pVp8Dec->hMFCVp8Handle.hMFCHandle = pVp8Dec->hMFCVp8Handle.pDecOps->Init(pVideoInstInfo);
    if (pVp8Dec->hMFCVp8Handle.hMFCHandle == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate context buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    if (ret != OMX_ErrorNone) {
        if (pDecOps != NULL) {
            Exynos_OSAL_Free(pDecOps);
            pVp8Dec->hMFCVp8Handle.pDecOps = NULL;
        }
        if (pInbufOps != NULL) {
            Exynos_OSAL_Free(pInbufOps);
            pVp8Dec->hMFCVp8Handle.pInbufOps = NULL;
        }
        if (pOutbufOps != NULL) {
            Exynos_OSAL_Free(pOutbufOps);
            pVp8Dec->hMFCVp8Handle.pOutbufOps = NULL;
        }
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecClose(EXYNOS_VP8DEC_HANDLE *pVp8Dec)
{
    OMX_ERRORTYPE            ret        = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pVp8Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    if (hMFCHandle != NULL) {
        pDecOps->Finalize(hMFCHandle);
        pVp8Dec->hMFCVp8Handle.hMFCHandle = NULL;
    }

    /* Unregister function pointers */
    Exynos_Video_Unregister_Decoder(pDecOps, pInbufOps, pOutbufOps);

    if (pOutbufOps != NULL) {
        Exynos_OSAL_Free(pOutbufOps);
        pVp8Dec->hMFCVp8Handle.pOutbufOps = NULL;
    }
    if (pInbufOps != NULL) {
        Exynos_OSAL_Free(pInbufOps);
        pVp8Dec->hMFCVp8Handle.pInbufOps = NULL;
    }
    if (pDecOps != NULL) {
        Exynos_OSAL_Free(pDecOps);
        pVp8Dec->hMFCVp8Handle.pDecOps = NULL;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecStart(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_VP8DEC_HANDLE   *pVp8Dec = NULL;

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

    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pVp8Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX)
        pInbufOps->Run(hMFCHandle);
    else if (nPortIndex == OUTPUT_PORT_INDEX)
        pOutbufOps->Run(hMFCHandle);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecStop(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = NULL;
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec            = NULL;
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

    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pVp8Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

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

OMX_ERRORTYPE VP8CodecOutputBufferProcessRun(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_VP8DEC_HANDLE   *pVp8Dec = NULL;

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
    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pVp8Dec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX) {
        if (pVp8Dec->bSourceStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pVp8Dec->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    if (nPortIndex == OUTPUT_PORT_INDEX) {
        if (pVp8Dec->bDestinationStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pVp8Dec->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecRegistCodecBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex,
    int                  nBufferCnt)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec            = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                            *hMFCHandle         = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    CODEC_DEC_BUFFER               **ppCodecBuffer      = NULL;
    ExynosVideoDecBufferOps         *pBufOps            = NULL;
    ExynosVideoPlane                *pPlanes            = NULL;

    int nPlaneCnt = 0;
    int i, j;

    FunctionIn();

    if (nPortIndex == INPUT_PORT_INDEX) {
        ppCodecBuffer   = &(pVideoDec->pMFCDecInputBuffer[0]);
        pBufOps         = pVp8Dec->hMFCVp8Handle.pInbufOps;
    } else {
        ppCodecBuffer   = &(pVideoDec->pMFCDecOutputBuffer[0]);
        pBufOps         = pVp8Dec->hMFCVp8Handle.pOutbufOps;
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

OMX_ERRORTYPE Vp8CodecReconfigAllBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT             *pExynosPort        = &pExynosComponent->pExynosPort[nPortIndex];
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec            = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                            *hMFCHandle         = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    ExynosVideoDecBufferOps         *pBufferOps         = NULL;

    FunctionIn();

    if ((nPortIndex == INPUT_PORT_INDEX) &&
        (pVp8Dec->bSourceStart == OMX_TRUE)) {
        ret = OMX_ErrorNotImplemented;
        goto EXIT;
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) &&
               (pVp8Dec->bDestinationStart == OMX_TRUE)) {
        pBufferOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

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
            ret = VP8CodecDstSetup(pOMXComponent);
            if (ret != OMX_ErrorNone) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed to VP8CodecDstSetup(0x%x)", __func__, __LINE__, ret);
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

OMX_ERRORTYPE VP8CodecEnQueueAllBuffer(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE         *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    int i, nOutbufs;

    ExynosVideoDecOps       *pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    FunctionIn();

    if ((nPortIndex != INPUT_PORT_INDEX) && (nPortIndex != OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((nPortIndex == INPUT_PORT_INDEX) &&
        (pVp8Dec->bSourceStart == OMX_TRUE)) {
        Exynos_CodecBufferReset(pExynosComponent, INPUT_PORT_INDEX);

        for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoDec->pMFCDecInputBuffer[%d]: 0x%x", i, pVideoDec->pMFCDecInputBuffer[i]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoDec->pMFCDecInputBuffer[%d]->pVirAddr[0]: 0x%x", i, pVideoDec->pMFCDecInputBuffer[i]->pVirAddr[0]);

            Exynos_CodecBufferEnQueue(pExynosComponent, INPUT_PORT_INDEX, pVideoDec->pMFCDecInputBuffer[i]);
        }

        pInbufOps->Clear_Queue(hMFCHandle);
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) &&
               (pVp8Dec->bDestinationStart == OMX_TRUE)) {
        Exynos_CodecBufferReset(pExynosComponent, OUTPUT_PORT_INDEX);

        for (i = 0; i < pVp8Dec->hMFCVp8Handle.maxDPBNum; i++) {
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

OMX_ERRORTYPE Vp8CodecCheckResolution(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec            = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle         = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pOutputPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_EXCEPTION_STATE     eOutputExcepState  = pOutputPort->exceptionFlag;

    ExynosVideoDecOps             *pDecOps            = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps       *pOutbufOps         = pVp8Dec->hMFCVp8Handle.pOutbufOps;
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

    if ((codecOutbufConf.nFrameWidth != pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth) ||
        (codecOutbufConf.nFrameHeight != pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight) ||
        (codecOutbufConf.nStride != pVp8Dec->hMFCVp8Handle.codecOutbufConf.nStride) ||
#if 0  // TODO: check posibility
        (codecOutbufConf.eColorFormat != pVp8Dec->hMFCVp8Handle.codecOutbufConf.eColorFormat) ||
        (codecOutbufConf.eFilledDataType != pVp8Dec->hMFCVp8Handle.codecOutbufConf.eFilledDataType) ||
        (codecOutbufConf.bInterlaced != pVp8Dec->hMFCVp8Handle.codecOutbufConf.bInterlaced) ||
#endif
        (maxDPBNum != pVp8Dec->hMFCVp8Handle.maxDPBNum)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] DRC: W(%d), H(%d) -> W(%d), H(%d)",
                            pExynosComponent, __FUNCTION__,
                            codecOutbufConf.nFrameWidth,
                            codecOutbufConf.nFrameHeight,
                            pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth,
                            pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight);
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] DRC: DPB(%d), FORMAT(0x%x), TYPE(0x%x) -> DPB(%d), FORMAT(0x%x), TYPE(0x%x)",
                            pExynosComponent, __FUNCTION__,
                            maxDPBNum, codecOutbufConf.eColorFormat, codecOutbufConf.eFilledDataType,
                            pVp8Dec->hMFCVp8Handle.maxDPBNum,
                            pVp8Dec->hMFCVp8Handle.codecOutbufConf.eColorFormat,
                            pVp8Dec->hMFCVp8Handle.codecOutbufConf.eFilledDataType);

        pInputPortDefinition->format.video.nFrameWidth     = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth;
        pInputPortDefinition->format.video.nFrameHeight    = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight;
        pInputPortDefinition->format.video.nStride         = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth;
        pInputPortDefinition->format.video.nSliceHeight    = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight;

        if (pOutputPort->bufferProcessType & BUFFER_SHARE) {
            pOutputPortDefinition->nBufferCountActual  = pVp8Dec->hMFCVp8Handle.maxDPBNum;
            pOutputPortDefinition->nBufferCountMin     = pVp8Dec->hMFCVp8Handle.maxDPBNum;
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

    if ((codecOutbufConf.cropRect.nTop != pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nTop) ||
        (codecOutbufConf.cropRect.nLeft != pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nLeft) ||
        (codecOutbufConf.cropRect.nWidth != pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nWidth) ||
        (codecOutbufConf.cropRect.nHeight != pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nHeight)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] CROP: W(%d), H(%d) -> W(%d), H(%d)",
                            pExynosComponent, __FUNCTION__,
                            codecOutbufConf.cropRect.nWidth,
                            codecOutbufConf.cropRect.nHeight,
                            pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nWidth,
                            pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nHeight);

        pCropRectangle->nTop     = pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nTop;
        pCropRectangle->nLeft    = pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nLeft;
        pCropRectangle->nWidth   = pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nWidth;
        pCropRectangle->nHeight  = pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nHeight;

        /** Send crop info call back **/
        (*(pExynosComponent->pCallbacks->EventHandler))
            (pOMXComponent,
             pExynosComponent->callbackData,
             OMX_EventPortSettingsChanged, /* The command was completed */
             OMX_DirOutput, /* This is the port index */
             OMX_IndexConfigCommonOutputCrop,
             NULL);
    }

    Exynos_OSAL_Memcpy(&pVp8Dec->hMFCVp8Handle.codecOutbufConf, &codecOutbufConf, sizeof(codecOutbufConf));
    pVp8Dec->hMFCVp8Handle.maxDPBNum = maxDPBNum;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Vp8CodecUpdateResolution(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec            = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle         = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pOutputPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    ExynosVideoDecOps             *pDecOps            = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps       *pOutbufOps         = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    OMX_CONFIG_RECTTYPE             *pCropRectangle         = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE    *pInputPortDefinition   = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE    *pOutputPortDefinition  = NULL;

    FunctionIn();

    /* get geometry for output */
    Exynos_OSAL_Memset(&pVp8Dec->hMFCVp8Handle.codecOutbufConf, 0, sizeof(ExynosVideoGeometry));
    if (pOutbufOps->Get_Geometry(hMFCHandle, &pVp8Dec->hMFCVp8Handle.codecOutbufConf) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to get geometry for parsed header info");
        ret = OMX_ErrorCorruptedHeader;
        goto EXIT;
    }

    /* get dpb count */
    pVp8Dec->hMFCVp8Handle.maxDPBNum = pDecOps->Get_ActualBufferCount(hMFCHandle);
    if (pVideoDec->bThumbnailMode == OMX_FALSE)
        pVp8Dec->hMFCVp8Handle.maxDPBNum += EXTRA_DPB_NUM;
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] VP8CodecSetup maxDPBNum: %d", pExynosComponent, __FUNCTION__, pVp8Dec->hMFCVp8Handle.maxDPBNum);

    pVp8Dec->hMFCVp8Handle.bConfiguredMFCSrc = OMX_TRUE;

    if (pVideoDec->bReconfigDPB != OMX_TRUE) {
        pCropRectangle          = &(pOutputPort->cropRectangle);
        pInputPortDefinition    = &(pInputPort->portDefinition);
        pOutputPortDefinition   = &(pOutputPort->portDefinition);
    } else {
        pCropRectangle          = &(pOutputPort->newCropRectangle);
        pInputPortDefinition    = &(pInputPort->newPortDefinition);
        pOutputPortDefinition   = &(pOutputPort->newPortDefinition);
    }

    pCropRectangle->nTop     = pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nTop;
    pCropRectangle->nLeft    = pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nLeft;
    pCropRectangle->nWidth   = pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nWidth;
    pCropRectangle->nHeight  = pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nHeight;

    if (pOutputPort->bufferProcessType & BUFFER_COPY) {
        if ((pVideoDec->bReconfigDPB) ||
            (pInputPort->portDefinition.format.video.nFrameWidth != pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth) ||
            (pInputPort->portDefinition.format.video.nFrameHeight != pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight)) {
            pOutputPort->exceptionFlag = NEED_PORT_DISABLE;

            pInputPortDefinition->format.video.nFrameWidth  = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nFrameHeight = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight;
            pInputPortDefinition->format.video.nStride      = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nSliceHeight = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight;

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
            (pInputPort->portDefinition.format.video.nFrameWidth != pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth) ||
            (pInputPort->portDefinition.format.video.nFrameHeight != pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight) ||
            ((OMX_S32)pOutputPort->portDefinition.nBufferCountActual != pVp8Dec->hMFCVp8Handle.maxDPBNum)) {
            pOutputPort->exceptionFlag = NEED_PORT_DISABLE;

            pInputPortDefinition->format.video.nFrameWidth  = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nFrameHeight = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight;
            pInputPortDefinition->format.video.nStride      = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nSliceHeight = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight;

            pOutputPortDefinition->nBufferCountActual       = pVp8Dec->hMFCVp8Handle.maxDPBNum;
            pOutputPortDefinition->nBufferCountMin          = pVp8Dec->hMFCVp8Handle.maxDPBNum;

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
        ((pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth != pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nWidth) ||
         (pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight != pVp8Dec->hMFCVp8Handle.codecOutbufConf.cropRect.nHeight))) {
        /* Check Crop */
        pInputPortDefinition->format.video.nFrameWidth  = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth;
        pInputPortDefinition->format.video.nFrameHeight = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight;
        pInputPortDefinition->format.video.nStride      = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameWidth;
        pInputPortDefinition->format.video.nSliceHeight = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nFrameHeight;

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

OMX_ERRORTYPE VP8CodecSrcSetup(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec         = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec           = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle        = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_COLOR_FORMATTYPE           eOutputFormat     = pExynosOutputPort->portDefinition.format.video.eColorFormat;
    OMX_U32                        oneFrameSize      = pSrcInputData->dataLen;

    ExynosVideoDecOps       *pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;
    ExynosVideoGeometry      bufferConf;

    unsigned int nAllocLen[MAX_BUFFER_PLANE] = {0, 0, 0};
    unsigned int nDataLen[MAX_BUFFER_PLANE]  = {oneFrameSize, 0, 0};
    OMX_U32  nInBufferCnt   = 0;
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
        ret = Exynos_OSAL_Queue(&pVp8Dec->bypassBufferInfoQ, (void *)pBufferInfo);
        Exynos_OSAL_SignalSet(pVp8Dec->hDestinationStartEvent);

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
    bufferConf.eCompressionFormat = VIDEO_CODING_VP8;
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
        if ((pVp8Dec->hMFCVp8Handle.videoInstInfo.specificInfo.dec.bDualDPBSupport == VIDEO_TRUE) &&
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

    pVp8Dec->hMFCVp8Handle.MFCOutputColorType = bufferConf.eColorFormat;
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

    /* set buffer process type */
    if (pDecOps->Set_BufferProcessType(hMFCHandle, pExynosOutputPort->bufferProcessType) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set buffer process type(not supported)");
    }

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

    ret = Vp8CodecUpdateResolution(pOMXComponent);
    if (((EXYNOS_OMX_ERRORTYPE)ret == OMX_ErrorCorruptedHeader) &&
        (pExynosComponent->codecType != HW_VIDEO_DEC_SECURE_CODEC) &&
        (oneFrameSize >= 8))
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "CorruptedHeader Info : %02x %02x %02x %02x %02x %02x %02x %02x ...",
                                                                    *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0])    , *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 1),
                                                                    *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 2), *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 3),
                                                                    *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 4), *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 5),
                                                                    *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 6), *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 7));
    if (ret != OMX_ErrorNone) {
        VP8CodecStop(pOMXComponent, INPUT_PORT_INDEX);
        pInbufOps->Cleanup_Buffer(hMFCHandle);
        goto EXIT;
    }

    Exynos_OSAL_SleepMillisec(0);
    ret = OMX_ErrorInputDataDecodeYet;
    VP8CodecStop(pOMXComponent, INPUT_PORT_INDEX);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE VP8CodecDstSetup(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoDecOps       *pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    unsigned int nAllocLen[MAX_BUFFER_PLANE] = {0, 0, 0};
    unsigned int nDataLen[MAX_BUFFER_PLANE]  = {0, 0, 0};
    int i, nOutbufs, nPlaneCnt;

    FunctionIn();

    nPlaneCnt = Exynos_GetPlaneFromPort(pExynosOutputPort);
    for (i = 0; i < nPlaneCnt; i++)
        nAllocLen[i] = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nAlignPlaneSize[i];

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
            nOutbufs = pVp8Dec->hMFCVp8Handle.maxDPBNum;
            if (pOutbufOps->Setup(hMFCHandle, nOutbufs) != VIDEO_ERROR_NONE) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup output buffer");
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }

            ret = Exynos_Allocate_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX, nOutbufs, nAllocLen);
            if (ret != OMX_ErrorNone)
                goto EXIT;

            /* Register output buffer */
            ret = VP8CodecRegistCodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX, nOutbufs);
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
            nOutbufs = pVp8Dec->hMFCVp8Handle.maxDPBNum;
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

    pVp8Dec->hMFCVp8Handle.bConfiguredMFCDst = OMX_TRUE;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_GetParameter(
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

    switch ((int)nParamIndex) {
    case OMX_IndexParamVideoVp8:
    {
        OMX_VIDEO_PARAM_VP8TYPE *pDstVP8Component = (OMX_VIDEO_PARAM_VP8TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_VP8TYPE *pSrcVP8Component = NULL;
        EXYNOS_VP8DEC_HANDLE    *pVp8Dec          = NULL;

        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pDstVP8Component, sizeof(OMX_VIDEO_PARAM_VP8TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstVP8Component->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcVP8Component = &pVp8Dec->VP8Component[pDstVP8Component->nPortIndex];

        Exynos_OSAL_Memcpy(((char *)pDstVP8Component) + nOffset,
                           ((char *)pSrcVP8Component) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_VP8TYPE) - nOffset);
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_VP8_DEC_ROLE);
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
        OMX_VIDEO_PARAM_PROFILELEVELTYPE    *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        OMX_VIDEO_PARAM_VP8TYPE             *pSrcVP8Component = NULL;
        EXYNOS_VP8DEC_HANDLE                *pVp8Dec          = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcVP8Component = &pVp8Dec->VP8Component[pDstProfileLevel->nPortIndex];

        pDstProfileLevel->eProfile = pSrcVP8Component->eProfile;
        pDstProfileLevel->eLevel = pSrcVP8Component->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        EXYNOS_VP8DEC_HANDLE                *pVp8Dec                 = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pVp8Dec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
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

OMX_ERRORTYPE Exynos_VP8Dec_SetParameter(
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

    switch ((int)nIndex) {
    case OMX_IndexParamVideoVp8:
    {
        OMX_VIDEO_PARAM_VP8TYPE *pDstVP8Component = NULL;
        OMX_VIDEO_PARAM_VP8TYPE *pSrcVP8Component = (OMX_VIDEO_PARAM_VP8TYPE *)pComponentParameterStructure;
        EXYNOS_VP8DEC_HANDLE    *pVp8Dec          = NULL;
        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pSrcVP8Component, sizeof(OMX_VIDEO_PARAM_VP8TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcVP8Component->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstVP8Component = &pVp8Dec->VP8Component[pSrcVP8Component->nPortIndex];

        Exynos_OSAL_Memcpy(((char *)pDstVP8Component) + nOffset,
                           ((char *)pSrcVP8Component) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_VP8TYPE) - nOffset);
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

        if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_VP8_DEC_ROLE)) {
            pExynosComponent->pExynosPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingVP8;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel  = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_VP8TYPE          *pDstVP8Component  = NULL;
        EXYNOS_VP8DEC_HANDLE             *pVp8Dec           = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

        pDstVP8Component = &pVp8Dec->VP8Component[pSrcProfileLevel->nPortIndex];

        if (OMX_FALSE == CheckProfileLevelSupport(pExynosComponent, pSrcProfileLevel)) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        pDstVP8Component->eProfile  = pSrcProfileLevel->eProfile;
        pDstVP8Component->eLevel    = pSrcProfileLevel->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        EXYNOS_VP8DEC_HANDLE                *pVp8Dec                 = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pVp8Dec->errorCorrectionType[INPUT_PORT_INDEX];

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

OMX_ERRORTYPE Exynos_VP8Dec_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE  nIndex,
    OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
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
        EXYNOS_VP8DEC_HANDLE    *pVp8Dec       = NULL;
        OMX_CONFIG_RECTTYPE     *pSrcRectType  = NULL;
        OMX_CONFIG_RECTTYPE     *pDstRectType  = NULL;

        pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

        if (pVp8Dec->hMFCVp8Handle.bConfiguredMFCSrc == OMX_FALSE) {
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

OMX_ERRORTYPE Exynos_VP8Dec_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE  nIndex,
    OMX_PTR        pComponentConfigStructure)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
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

OMX_ERRORTYPE Exynos_VP8Dec_GetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE  hComponent,
    OMX_IN  OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE  *pIndexType)
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

OMX_ERRORTYPE Exynos_VP8Dec_ComponentRoleEnum(
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
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_VP8_DEC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE Exynos_VP8Dec_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec         = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec           = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    OMX_PTR                        hMFCHandle        = pVp8Dec->hMFCVp8Handle.hMFCHandle;

    ExynosVideoDecOps       *pDecOps        = NULL;
    ExynosVideoDecBufferOps *pInbufOps      = NULL;
    ExynosVideoDecBufferOps *pOutbufOps     = NULL;
    ExynosVideoInstInfo     *pVideoInstInfo = &(pVp8Dec->hMFCVp8Handle.videoInstInfo);

    CSC_METHOD csc_method = CSC_METHOD_SW;
    int i, plane;

    FunctionIn();

    pVp8Dec->hMFCVp8Handle.bConfiguredMFCSrc = OMX_FALSE;
    pVp8Dec->hMFCVp8Handle.bConfiguredMFCDst = OMX_FALSE;
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

    /* VP8 Codec Open */
    ret = VP8CodecOpen(pVp8Dec, pVideoInstInfo);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

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

    pVp8Dec->bSourceStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pVp8Dec->hSourceStartEvent);
    pVp8Dec->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pVp8Dec->hDestinationStartEvent);

    INIT_ARRAY_TO_VAL(pExynosComponent->timeStamp, DEFAULT_TIMESTAMP_VAL, MAX_TIMESTAMP);
    Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pVp8Dec->hMFCVp8Handle.indexTimestamp = 0;
    pVp8Dec->hMFCVp8Handle.outputIndexTimestamp = 0;

    pExynosComponent->getAllDelayBuffer = OMX_FALSE;

    Exynos_OSAL_QueueCreate(&pVp8Dec->bypassBufferInfoQ, QUEUE_ELEMENTS);

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
OMX_ERRORTYPE Exynos_VP8Dec_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_VP8DEC_HANDLE    *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    OMX_PTR                hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;

    ExynosVideoDecOps       *pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pVp8Dec->hMFCVp8Handle.pOutbufOps;

    int i, plane;

    FunctionIn();

    if (pVideoDec->csc_handle != NULL) {
        csc_deinit(pVideoDec->csc_handle);
        pVideoDec->csc_handle = NULL;
    }

    Exynos_OSAL_QueueTerminate(&pVp8Dec->bypassBufferInfoQ);

    Exynos_OSAL_SignalTerminate(pVp8Dec->hDestinationStartEvent);
    pVp8Dec->hDestinationStartEvent = NULL;
    pVp8Dec->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalTerminate(pVp8Dec->hSourceStartEvent);
    pVp8Dec->hSourceStartEvent = NULL;
    pVp8Dec->bSourceStart = OMX_FALSE;

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
    VP8CodecClose(pVp8Dec);

    Exynos_ResetAllPortConfig(pOMXComponent);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_SrcIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec         = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE          *pVp8Dec           = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle        = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32                        oneFrameSize      = pSrcInputData->dataLen;

    ExynosVideoDecOps       *pDecOps     = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps   = pVp8Dec->hMFCVp8Handle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps  = pVp8Dec->hMFCVp8Handle.pOutbufOps;
    ExynosVideoErrorType     codecReturn = VIDEO_ERROR_NONE;

    OMX_BUFFERHEADERTYPE tempBufferHeader;
    void *pPrivate = NULL;

    unsigned int nAllocLen[MAX_BUFFER_PLANE] = {0, 0, 0};
    unsigned int nDataLen[MAX_BUFFER_PLANE]  = {oneFrameSize, 0, 0};
    OMX_BOOL bInStartCode = OMX_FALSE;
    int i;

    FunctionIn();

    if (pVp8Dec->hMFCVp8Handle.bConfiguredMFCSrc == OMX_FALSE) {
        ret = VP8CodecSrcSetup(pOMXComponent, pSrcInputData);
        goto EXIT;
    }

    if ((pVp8Dec->hMFCVp8Handle.bConfiguredMFCDst == OMX_FALSE) &&
        (pVideoDec->bForceHeaderParsing == OMX_FALSE)) {
        ret = VP8CodecDstSetup(pOMXComponent);
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed to VP8CodecDstSetup(0x%x)", __func__, __LINE__, ret);
            goto EXIT;
        }
    }

    if (((bInStartCode = Check_VP8_StartCode(pSrcInputData->multiPlaneBuffer.dataBuffer[0], oneFrameSize)) == OMX_TRUE) ||
        ((pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
        pExynosComponent->timeStamp[pVp8Dec->hMFCVp8Handle.indexTimestamp] = pSrcInputData->timeStamp;
        pExynosComponent->nFlags[pVp8Dec->hMFCVp8Handle.indexTimestamp] = pSrcInputData->nFlags;
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] input timestamp %lld us (%.2f secs), Tag: %d, nFlags: 0x%x, oneFrameSize: %d", pExynosComponent, __FUNCTION__,
                                                        pSrcInputData->timeStamp, pSrcInputData->timeStamp / 1E6, pVp8Dec->hMFCVp8Handle.indexTimestamp, pSrcInputData->nFlags, oneFrameSize);
        pDecOps->Set_FrameTag(hMFCHandle, pVp8Dec->hMFCVp8Handle.indexTimestamp);
        pVp8Dec->hMFCVp8Handle.indexTimestamp++;
        pVp8Dec->hMFCVp8Handle.indexTimestamp %= MAX_TIMESTAMP;

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
        VP8CodecStart(pOMXComponent, INPUT_PORT_INDEX);
        if (pVp8Dec->bSourceStart == OMX_FALSE) {
            pVp8Dec->bSourceStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pVp8Dec->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
        if (pVp8Dec->bDestinationStart == OMX_FALSE) {
            pVp8Dec->bDestinationStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pVp8Dec->hDestinationStartEvent);
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

OMX_ERRORTYPE Exynos_VP8Dec_SrcOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE         *pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT     *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    ExynosVideoDecOps       *pDecOps    = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pVp8Dec->hMFCVp8Handle.pInbufOps;
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

OMX_ERRORTYPE Exynos_VP8Dec_DstIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec            = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                            *hMFCHandle         = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT             *pExynosOutputPort  = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoDecOps       *pDecOps     = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps *pOutbufOps  = pVp8Dec->hMFCVp8Handle.pOutbufOps;
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
        nAllocLen[i] = pVp8Dec->hMFCVp8Handle.codecOutbufConf.nAlignPlaneSize[i];

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] : %d => ADDR[%d]: 0x%x", pExynosComponent, __FUNCTION__, __LINE__, i,
                                        pDstInputData->multiPlaneBuffer.dataBuffer[i]);
    }

    if ((pVideoDec->bReconfigDPB == OMX_TRUE) &&
        (pExynosOutputPort->bufferProcessType & BUFFER_SHARE) &&
        (pExynosOutputPort->exceptionFlag == GENERAL_STATE)) {
        ret = VP8CodecDstSetup(pOMXComponent);
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed to VP8CodecDstSetup(0x%x)", __func__, __LINE__, ret);
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
    VP8CodecStart(pOMXComponent, OUTPUT_PORT_INDEX);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_VP8Dec_DstOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec            = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    void                            *hMFCHandle         = pVp8Dec->hMFCVp8Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT             *pExynosInputPort   = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT             *pExynosOutputPort  = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    DECODE_CODEC_EXTRA_BUFFERINFO   *pBufferInfo        = NULL;

    ExynosVideoDecOps           *pDecOps        = pVp8Dec->hMFCVp8Handle.pDecOps;
    ExynosVideoDecBufferOps     *pOutbufOps     = pVp8Dec->hMFCVp8Handle.pOutbufOps;
    ExynosVideoBuffer           *pVideoBuffer   = NULL;
    ExynosVideoBuffer            videoBuffer;
    ExynosVideoFrameStatusType   displayStatus  = VIDEO_FRAME_STATUS_UNKNOWN;
    ExynosVideoGeometry         *bufferGeometry = NULL;
    ExynosVideoErrorType         codecReturn    = VIDEO_ERROR_NONE;

    OMX_S32 indexTimestamp = 0;
    int plane, nPlaneCnt;

    FunctionIn();

    if (pVp8Dec->bDestinationStart == OMX_FALSE) {
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
            Vp8CodecUpdateResolution(pOMXComponent);
            pVideoDec->csc_set_format = OMX_FALSE;
        }
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    pVp8Dec->hMFCVp8Handle.outputIndexTimestamp++;
    pVp8Dec->hMFCVp8Handle.outputIndexTimestamp %= MAX_TIMESTAMP;

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

    pBufferInfo = (DECODE_CODEC_EXTRA_BUFFERINFO *)pDstOutputData->extInfo;
    bufferGeometry = &pVp8Dec->hMFCVp8Handle.codecOutbufConf;
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
                pDstOutputData->timeStamp = pExynosComponent->timeStamp[pVp8Dec->hMFCVp8Handle.outputIndexTimestamp];
                pDstOutputData->nFlags = pExynosComponent->nFlags[pVp8Dec->hMFCVp8Handle.outputIndexTimestamp];
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
               pVp8Dec->hMFCVp8Handle.outputIndexTimestamp = indexTimestamp;
            else
               indexTimestamp = pVp8Dec->hMFCVp8Handle.outputIndexTimestamp;
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

    if (pVp8Dec->hMFCVp8Handle.videoInstInfo.specificInfo.dec.bLastFrameSupport == VIDEO_FALSE) {
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

            if (pExynosComponent->bBehaviorEOS == OMX_TRUE) {
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

OMX_ERRORTYPE Exynos_VP8Dec_srcInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
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

    ret = Exynos_VP8Dec_SrcIn(pOMXComponent, pSrcInputData);
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

OMX_ERRORTYPE Exynos_VP8Dec_srcOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec            = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
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
    if ((pVp8Dec->bSourceStart == OMX_FALSE) &&
       (!CHECK_PORT_BEING_FLUSHED(pExynosInputPort))) {
        Exynos_OSAL_SignalWait(pVp8Dec->hSourceStartEvent, DEF_MAX_WAIT_TIME);
        if (pVideoDec->bExitBufferProcessThread)
            goto EXIT;

        Exynos_OSAL_SignalReset(pVp8Dec->hSourceStartEvent);
    }

    ret = Exynos_VP8Dec_SrcOut(pOMXComponent, pSrcOutputData);
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

OMX_ERRORTYPE Exynos_VP8Dec_dstInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec            = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
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
        if ((pVp8Dec->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pVp8Dec->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            if (pVideoDec->bExitBufferProcessThread)
                goto EXIT;

            Exynos_OSAL_SignalReset(pVp8Dec->hDestinationStartEvent);
        }
        if (Exynos_OSAL_GetElemNum(&pVp8Dec->bypassBufferInfoQ) > 0) {
            BYPASS_BUFFER_INFO *pBufferInfo = (BYPASS_BUFFER_INFO *)Exynos_OSAL_Dequeue(&pVp8Dec->bypassBufferInfoQ);
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
    if (pVp8Dec->hMFCVp8Handle.bConfiguredMFCDst == OMX_TRUE) {
        ret = Exynos_VP8Dec_DstIn(pOMXComponent, pDstInputData);
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

OMX_ERRORTYPE Exynos_VP8Dec_dstOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec            = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
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
        if ((pVp8Dec->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pVp8Dec->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            if (pVideoDec->bExitBufferProcessThread)
                goto EXIT;

            Exynos_OSAL_SignalReset(pVp8Dec->hDestinationStartEvent);
        }
        if (Exynos_OSAL_GetElemNum(&pVp8Dec->bypassBufferInfoQ) > 0) {
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

            pBufferInfo = Exynos_OSAL_Dequeue(&pVp8Dec->bypassBufferInfoQ);
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
    ret = Exynos_VP8Dec_DstOut(pOMXComponent, pDstOutputData);
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
    EXYNOS_VP8DEC_HANDLE            *pVp8Dec            = NULL;
    int i = 0;

    Exynos_OSAL_Get_Log_Property(); // For debuging
    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    if ((Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_VP8_DEC, componentName) != 0) &&
        (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_VP8_CUSTOM_DEC, componentName) != 0)) {
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

    pVp8Dec = Exynos_OSAL_Malloc(sizeof(EXYNOS_VP8DEC_HANDLE));
    if (pVp8Dec == NULL) {
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pVp8Dec, 0, sizeof(EXYNOS_VP8DEC_HANDLE));
    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    pVideoDec->hCodecHandle = (OMX_HANDLETYPE)pVp8Dec;
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

    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingVP8;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/x-vnd.on2.vp8");
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;
    pExynosPort->bufferProcessType = BUFFER_SHARE;
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
        INIT_SET_SIZE_VERSION(&pVp8Dec->VP8Component[i], OMX_VIDEO_PARAM_VP8TYPE);
        pVp8Dec->VP8Component[i].nPortIndex = i;
        pVp8Dec->VP8Component[i].eProfile   = OMX_VIDEO_VP8ProfileMain;
        pVp8Dec->VP8Component[i].eLevel     = OMX_VIDEO_VP8Level_Version3;
    }

    pOMXComponent->GetParameter      = &Exynos_VP8Dec_GetParameter;
    pOMXComponent->SetParameter      = &Exynos_VP8Dec_SetParameter;
    pOMXComponent->GetConfig         = &Exynos_VP8Dec_GetConfig;
    pOMXComponent->SetConfig         = &Exynos_VP8Dec_SetConfig;
    pOMXComponent->GetExtensionIndex = &Exynos_VP8Dec_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &Exynos_VP8Dec_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &Exynos_OMX_ComponentDeinit;

    pExynosComponent->exynos_codec_componentInit      = &Exynos_VP8Dec_Init;
    pExynosComponent->exynos_codec_componentTerminate = &Exynos_VP8Dec_Terminate;

    pVideoDec->exynos_codec_srcInputProcess  = &Exynos_VP8Dec_srcInputBufferProcess;
    pVideoDec->exynos_codec_srcOutputProcess = &Exynos_VP8Dec_srcOutputBufferProcess;
    pVideoDec->exynos_codec_dstInputProcess  = &Exynos_VP8Dec_dstInputBufferProcess;
    pVideoDec->exynos_codec_dstOutputProcess = &Exynos_VP8Dec_dstOutputBufferProcess;

    pVideoDec->exynos_codec_start            = &VP8CodecStart;
    pVideoDec->exynos_codec_stop             = &VP8CodecStop;
    pVideoDec->exynos_codec_bufferProcessRun = &VP8CodecOutputBufferProcessRun;
    pVideoDec->exynos_codec_enqueueAllBuffer = &VP8CodecEnQueueAllBuffer;

#if 0  /* unused code */
    pVideoDec->exynos_checkInputFrame                 = &Check_VP8_Frame;
    pVideoDec->exynos_codec_getCodecInputPrivateData  = &GetCodecInputPrivateData;
#endif

    pVideoDec->exynos_codec_getCodecOutputPrivateData = &GetCodecOutputPrivateData;
    pVideoDec->exynos_codec_reconfigAllBuffers        = &Vp8CodecReconfigAllBuffers;

    pVideoDec->exynos_codec_checkFormatSupport      = &CheckFormatHWSupport;
    pVideoDec->exynos_codec_checkResolutionChange   = &Vp8CodecCheckResolution;

    pVideoDec->hSharedMemory = Exynos_OSAL_SharedMemory_Open();
    if (pVideoDec->hSharedMemory == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        Exynos_OSAL_Free(pVp8Dec);
        pVp8Dec = ((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pVp8Dec->hMFCVp8Handle.videoInstInfo.eCodecType = VIDEO_CODING_VP8;
    if (pExynosComponent->codecType == HW_VIDEO_DEC_SECURE_CODEC)
        pVp8Dec->hMFCVp8Handle.videoInstInfo.eSecurityType = VIDEO_SECURE;
    else
        pVp8Dec->hMFCVp8Handle.videoInstInfo.eSecurityType = VIDEO_NORMAL;

    if (Exynos_Video_GetInstInfo(&(pVp8Dec->hMFCVp8Handle.videoInstInfo), VIDEO_TRUE /* dec */) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%p][%s]: Exynos_Video_GetInstInfo is failed", pExynosComponent, __FUNCTION__);
        Exynos_OSAL_Free(pVp8Dec);
        pVp8Dec = ((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] GetInstInfo for dec %d %d %d %d", pExynosComponent, __FUNCTION__,
            (pVp8Dec->hMFCVp8Handle.videoInstInfo.specificInfo.dec.bDualDPBSupport),
            (pVp8Dec->hMFCVp8Handle.videoInstInfo.specificInfo.dec.bDynamicDPBSupport),
            (pVp8Dec->hMFCVp8Handle.videoInstInfo.specificInfo.dec.bLastFrameSupport),
            (pVp8Dec->hMFCVp8Handle.videoInstInfo.specificInfo.dec.bSkypeSupport));

    if (pVp8Dec->hMFCVp8Handle.videoInstInfo.specificInfo.dec.bDynamicDPBSupport == VIDEO_TRUE)
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
    EXYNOS_VP8DEC_HANDLE        *pVp8Dec            = NULL;

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

    pVp8Dec = (EXYNOS_VP8DEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pVp8Dec != NULL) {
        Exynos_OSAL_Free(pVp8Dec);
        pVp8Dec = pVideoDec->hCodecHandle = NULL;
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
