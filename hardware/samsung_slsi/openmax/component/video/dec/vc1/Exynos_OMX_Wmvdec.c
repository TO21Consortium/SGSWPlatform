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
 * @file        Exynos_OMX_Wmvdec.c
 * @brief
 * @author      HyeYeon Chung (hyeon.chung@samsung.com)
 * @author      Satish Kumar Reddy (palli.satish@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.07.10 : Create
 *              : Support WMV3 (Vc-1 Simple/Main Profile)
 *              : Support WMvC1 (Vc-1 Advanced Profile)
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
#include "Exynos_OMX_Wmvdec.h"
#include "ExynosVideoApi.h"
#include "Exynos_OSAL_SharedMemory.h"
#include "Exynos_OSAL_Event.h"

/* To use CSC_METHOD_HW in EXYNOS OMX, gralloc should allocate physical memory using FIMC */
/* It means GRALLOC_USAGE_HW_FIMC1 should be set on Native Window usage */
#include "csc.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_WMV_DEC"
//#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

#define WMV_DEC_NUM_OF_EXTRA_BUFFERS 7

#define SL_META_LEN 36 /* sequence layer metadata length in bytes */

/* parser does not send start code on Stagefright */
#define WO_START_CODE

/* Enable or disable "WMV3_ADDITIONAL_START_CODE" based on MFC F/W's need */
//#define WMV3_ADDITIONAL_START_CODE

//#define FULL_FRAME_SEARCH

static OMX_ERRORTYPE SetProfileLevel(
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    OMX_ERRORTYPE                    ret            = OMX_ErrorNone;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec      = NULL;
    EXYNOS_WMVDEC_HANDLE            *pWmvDec        = NULL;

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

    pWmvDec = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pWmvDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pWmvDec->hMFCWmvHandle.profiles[nProfileCnt]   = OMX_VIDEO_VC1ProfileSimple;
    pWmvDec->hMFCWmvHandle.maxLevel[nProfileCnt++] = OMX_VIDEO_VC1LevelMedium;

    pWmvDec->hMFCWmvHandle.profiles[nProfileCnt]   = OMX_VIDEO_VC1ProfileMain;
    pWmvDec->hMFCWmvHandle.maxLevel[nProfileCnt++] = OMX_VIDEO_VC1LevelHigh;

    pWmvDec->hMFCWmvHandle.profiles[nProfileCnt]   = OMX_VIDEO_VC1ProfileAdvanced;
    pWmvDec->hMFCWmvHandle.maxLevel[nProfileCnt++] = OMX_VIDEO_VC1Level4;

    pWmvDec->hMFCWmvHandle.nProfileCnt = nProfileCnt;

EXIT:
    return ret;
}

static OMX_ERRORTYPE GetIndexToProfileLevel(
    EXYNOS_OMX_BASECOMPONENT         *pExynosComponent,
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *pProfileLevelType)
{
    OMX_ERRORTYPE                    ret            = OMX_ErrorNone;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec      = NULL;
    EXYNOS_WMVDEC_HANDLE            *pWmvDec        = NULL;

    OMX_U32 nLevelCnt = 0;
    OMX_U32 nMaxIndex = 0;
    int i;

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

    pWmvDec = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pWmvDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    for (i = 0; i < pWmvDec->hMFCWmvHandle.nProfileCnt; i++)
        nMaxIndex += pWmvDec->hMFCWmvHandle.maxLevel[i];

    if ((pWmvDec->hMFCWmvHandle.nProfileCnt == 0) ||
        (nMaxIndex == 0)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : there is no any profile/level", __FUNCTION__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    if (nMaxIndex <= pProfileLevelType->nProfileIndex) {
        ret = OMX_ErrorNoMore;
        goto EXIT;
    }

    for (i = 0; i < pWmvDec->hMFCWmvHandle.nProfileCnt; i++) {
        if (pProfileLevelType->nProfileIndex < (nLevelCnt + pWmvDec->hMFCWmvHandle.maxLevel[i])) {
            pProfileLevelType->eProfile = pWmvDec->hMFCWmvHandle.profiles[i];
            pProfileLevelType->eLevel   = (OMX_U32)(pProfileLevelType->nProfileIndex - nLevelCnt);
            break;
        }

        nLevelCnt += pWmvDec->hMFCWmvHandle.maxLevel[i];
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s : supported profile(%x), level(%x)", __FUNCTION__, pProfileLevelType->eProfile, pProfileLevelType->eLevel);

EXIT:
    return ret;
}

static OMX_BOOL CheckProfileLevelSupport(
    EXYNOS_OMX_BASECOMPONENT         *pExynosComponent,
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *pProfileLevelType)
{
    OMX_BOOL                         ret        = OMX_FALSE;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec  = NULL;
    EXYNOS_WMVDEC_HANDLE            *pWmvDec    = NULL;

    int i;

    FunctionIn();

    if ((pExynosComponent == NULL) ||
        (pProfileLevelType == NULL)) {
        goto EXIT;
    }

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    if (pVideoDec == NULL)
        goto EXIT;

    pWmvDec = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pWmvDec == NULL)
        goto EXIT;

    if (pWmvDec->hMFCWmvHandle.nProfileCnt == 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : there is no any profile/level", __FUNCTION__);
        goto EXIT;
    }

    for (i = 0; i < pWmvDec->hMFCWmvHandle.nProfileCnt; i++) {
        if ((pWmvDec->hMFCWmvHandle.profiles[i] == pProfileLevelType->eProfile) &&
            (pWmvDec->hMFCWmvHandle.maxLevel[i] >= pProfileLevelType->eLevel)) {
            ret = OMX_TRUE;
            break;
        }
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s : profile(%x)/level(%x) is %ssupported", __FUNCTION__,
                                            pProfileLevelType->eProfile, pProfileLevelType->eLevel,
                                            (ret == OMX_TRUE)? "":"not ");
EXIT:
    return ret;
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

static WMV_FORMAT getFormatType(
    OMX_U8 *pInputStream)
{
    WMV_FORMAT  ret          = WMV_FORMAT_UNKNOWN;
    OMX_BYTE    pCheckBuffer = (OMX_BYTE)pInputStream;

    if (pInputStream == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: pInputStream is null", __FUNCTION__);
        return ret;
    }

    if ((pCheckBuffer[3] == 0xC5) &&
        (pCheckBuffer[4] == 0x04)) {
        Exynos_OSAL_Log(EXYNOS_LOG_INFO, "WMV_FORMAT_WMV3");
        ret = WMV_FORMAT_WMV3;
    } else if (((pCheckBuffer[3] == 0x01) &&
                    (pCheckBuffer[4] == 0x0F)) ||
               ((pCheckBuffer[2] == 0x01) &&
                    (pCheckBuffer[3] == 0x0F))) {
        Exynos_OSAL_Log(EXYNOS_LOG_INFO, "WMV_FORMAT_VC1");
        ret = WMV_FORMAT_VC1;
    } else {
        Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "WMV_FORMAT_UNKNOWN");
    }

    return ret;
}

#if 0  /* unused code */
int Check_Wmv_Frame(
    OMX_U8   *pInputStream,
    OMX_U32   buffSize,
    OMX_U32   flag,
    OMX_BOOL  bPreviousFrameEOF,
    OMX_BOOL *pbEndOfFrame)
{
    OMX_U32  compressionID;
    OMX_BOOL bFrameStart;
    OMX_U32  len, readStream;
    OMX_U32  startCode;

    int retVal = (int)buffSize;
    static WMV_FORMAT wmvFormat;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "buffSize = %d", buffSize);

    len = 0;
    bFrameStart = OMX_FALSE;

    if (flag & OMX_BUFFERFLAG_CODECCONFIG)
        wmvFormat = getFormatType(pInputStream);

    if (wmvFormat == WMV_FORMAT_WMV3) {
        *pbEndOfFrame = OMX_TRUE;
        return retVal;
    }

#ifdef WO_START_CODE
    /* parser does not send start code on Stagefright */
    if (wmvFormat == WMV_FORMAT_VC1) {
        *pbEndOfFrame = OMX_TRUE;
        return retVal;
    }
#else
 /* TODO : for comformanc test based on common buffer scheme w/o parser */

    if (bPreviousFrameEOF == OMX_FALSE)
        bFrameStart = OMX_TRUE;

    startCode = 0xFFFFFFFF;
    if (bFrameStart == OMX_FALSE) {
        /* find Frame start code */
        while(startCode != 0x10D) {
            readStream = *(pInputStream + len);
            startCode = (startCode << 8) | readStream;
            len++;
            if (len > retVal)
                goto EXIT;
        }
    }

    /* find next Frame start code */
    startCode = 0xFFFFFFFF;
    while ((startCode != 0x10D)) {
        readStream = *(pInputStream + len);
        startCode = (startCode << 8) | readStream;
        len++;
        if (len > retVal)
            goto EXIT;
    }

    *pbEndOfFrame = OMX_TRUE;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "1. Check_Wmv_Frame returned EOF = %d, len = %d, buffSize = %d", *pbEndOfFrame, len - 4, buffSize);

    return len - 4;
#endif

EXIT :
    *pbEndOfFrame = OMX_FALSE;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "2. Check_Wmv_Frame returned EOF = %d, len = %d, buffSize = %d", *pbEndOfFrame, len - 1, buffSize);

    return --len;
}
#endif

static OMX_BOOL Check_Stream_PrefixCode(
    OMX_U8    *pInputStream,
    OMX_U32    streamSize,
    WMV_FORMAT wmvFormat)
{
    switch (wmvFormat) {
    case WMV_FORMAT_WMV3:
#ifdef WMV3_ADDITIONAL_START_CODE
        return OMX_FALSE;
#else
        if (streamSize > 0)
            return OMX_TRUE;
        else
            return OMX_FALSE;
#endif
        break;
    case WMV_FORMAT_VC1:
        /* TODO : for comformanc test based on common buffer scheme w/o parser */
        if (streamSize < 3) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: streamSize is too small (%d)", __FUNCTION__, streamSize);
            return OMX_FALSE;
        } else if ((pInputStream[0] == 0x00) &&
                   (pInputStream[1] == 0x00) &&
                   (pInputStream[2] == 0x01)) {
            return OMX_TRUE;
        } else {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s: Cannot find prefix", __FUNCTION__);
            return OMX_FALSE;
        }
        break;

    default:
        Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "%s: undefined wmvFormat (%d)", __FUNCTION__, wmvFormat);
        return OMX_FALSE;
        break;
    }
}

OMX_BOOL Make_Stream_MetaData(
     OMX_U8        *pInputStream,
     OMX_U32       *pStreamSize,
     WMV_FORMAT     wmvFormat)
{
    OMX_BOOL ret      = OMX_FALSE;
    OMX_BYTE pCurrBuf = pInputStream;
    switch ((int)wmvFormat) {
    case WMV_FORMAT_WMV3:
        if ((*pStreamSize) >= SL_META_LEN) {
            (*pStreamSize) = SL_META_LEN;
            ret = OMX_TRUE;
        } else {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: *pStreamSize is too small to contain metadata(%d)", __FUNCTION__, *pStreamSize);
            ret = OMX_FALSE;
        }
        break;
    case WMV_FORMAT_VC1:
        if ((pInputStream[1] == 0x00) &&
            (pInputStream[2] == 0x00) &&
            (pInputStream[3] == 0x01) &&
            (pInputStream[4] == 0x0F)) {
            Exynos_OSAL_Memmove(pCurrBuf, pInputStream + 1, (*pStreamSize) - 1);
            (*pStreamSize) -= 1;
            ret = OMX_TRUE;
        } else if ((pInputStream[0] == 0x00) &&
                   (pInputStream[1] == 0x00) &&
                   (pInputStream[2] == 0x01) &&
                   (pInputStream[3] == 0x0F) &&
                   ((*pStreamSize) >= 1)) {
            ret = OMX_TRUE;
        }
        break;
    default:
        Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "%s: It is not necessary to make bitstream metadata for wmvFormat (%d)", __FUNCTION__, (int)wmvFormat);
        ret = OMX_FALSE;
        break;
    }

    return ret;
}

static OMX_BOOL Make_Stream_StartCode(
    OMX_U8          *pInputStream,
    OMX_U32         *pStreamSize,
    WMV_FORMAT       wmvFormat)
{
    OMX_BOOL ret             = OMX_FALSE;
    OMX_U8   vc1StartCode[4] = {0x00, 0x00, 0x01, 0x0d};

#ifdef WMV3_ADDITIONAL_START_CODE
     /* first 4 bytes : size of Frame, second 4 bytes : present Time stamp */
    OMX_U8  wmvStartCode[8] = {0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00};
#endif

    switch ((int)wmvFormat) {
    case WMV_FORMAT_WMV3:
#ifdef WMV3_ADDITIONAL_START_CODE
        Exynos_OSAL_Memmove(pInputStream + 8, pInputStream, (*pStreamSize));
        Exynos_OSAL_Memcpy(pInputStream, wmvStartCode, 8);
        (*pStreamSize) += 8;
#endif
        ret = OMX_TRUE;
        break;
    case WMV_FORMAT_VC1:
        /* Should find better way to shift data */
        Exynos_OSAL_Memmove(pInputStream + 4, pInputStream, (*pStreamSize));
        Exynos_OSAL_Memcpy(pInputStream, vc1StartCode, 4);
        (*pStreamSize) += 4;
        ret = OMX_TRUE;
        break;
    default:
        Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "%s: undefined wmvFormat (%d)", __FUNCTION__, wmvFormat);
        ret = OMX_FALSE;
        break;
    }

EXIT:
    return ret;
}

OMX_BOOL CheckFormatHWSupport(
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent,
    OMX_COLOR_FORMATTYPE         eColorFormat)
{
    OMX_BOOL                         ret            = OMX_FALSE;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec      = NULL;
    EXYNOS_WMVDEC_HANDLE            *pWmvDec        = NULL;
    EXYNOS_OMX_BASEPORT             *pOutputPort    = NULL;
    ExynosVideoColorFormatType       eVideoFormat   = VIDEO_CODING_UNKNOWN;
    int i;

    FunctionIn();

    if (pExynosComponent == NULL)
        goto EXIT;

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    if (pVideoDec == NULL)
        goto EXIT;

    pWmvDec = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pWmvDec == NULL)
        goto EXIT;
    pOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    eVideoFormat = (ExynosVideoColorFormatType)Exynos_OSAL_OMX2VideoFormat(eColorFormat, pOutputPort->ePlaneType);

    for (i = 0; i < VIDEO_COLORFORMAT_MAX; i++) {
        if (pWmvDec->hMFCWmvHandle.videoInstInfo.supportFormat[i] == VIDEO_COLORFORMAT_UNKNOWN)
            break;

        if (pWmvDec->hMFCWmvHandle.videoInstInfo.supportFormat[i] == eVideoFormat) {
            ret = OMX_TRUE;
            break;
        }
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE WmvCodecOpen(EXYNOS_WMVDEC_HANDLE *pWmvDec, ExynosVideoInstInfo *pVideoInstInfo)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pWmvDec == NULL) {
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

    pWmvDec->hMFCWmvHandle.pDecOps    = pDecOps;
    pWmvDec->hMFCWmvHandle.pInbufOps  = pInbufOps;
    pWmvDec->hMFCWmvHandle.pOutbufOps = pOutbufOps;

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
    pWmvDec->hMFCWmvHandle.hMFCHandle = pWmvDec->hMFCWmvHandle.pDecOps->Init(pVideoInstInfo);
    if (pWmvDec->hMFCWmvHandle.hMFCHandle == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate context buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    if (ret != OMX_ErrorNone) {
        if (pDecOps != NULL) {
            Exynos_OSAL_Free(pDecOps);
            pWmvDec->hMFCWmvHandle.pDecOps = NULL;
        }
        if (pInbufOps != NULL) {
            Exynos_OSAL_Free(pInbufOps);
            pWmvDec->hMFCWmvHandle.pInbufOps = NULL;
        }
        if (pOutbufOps != NULL) {
            Exynos_OSAL_Free(pOutbufOps);
            pWmvDec->hMFCWmvHandle.pOutbufOps = NULL;
        }
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE WmvCodecClose(EXYNOS_WMVDEC_HANDLE *pWmvDec)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pWmvDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pWmvDec->hMFCWmvHandle.hMFCHandle;
    pDecOps    = pWmvDec->hMFCWmvHandle.pDecOps;
    pInbufOps  = pWmvDec->hMFCWmvHandle.pInbufOps;
    pOutbufOps = pWmvDec->hMFCWmvHandle.pOutbufOps;

    if (hMFCHandle != NULL) {
        pDecOps->Finalize(hMFCHandle);
        pWmvDec->hMFCWmvHandle.hMFCHandle = NULL;
    }

    /* Unregister function pointers */
    Exynos_Video_Unregister_Decoder(pDecOps, pInbufOps, pOutbufOps);

    if (pOutbufOps != NULL) {
        Exynos_OSAL_Free(pOutbufOps);
        pWmvDec->hMFCWmvHandle.pOutbufOps = NULL;
    }
    if (pInbufOps != NULL) {
        Exynos_OSAL_Free(pInbufOps);
        pWmvDec->hMFCWmvHandle.pInbufOps = NULL;
    }
    if (pDecOps != NULL) {
        Exynos_OSAL_Free(pDecOps);
        pWmvDec->hMFCWmvHandle.pDecOps = NULL;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE WmvCodecStart(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_WMVDEC_HANDLE   *pWmvDec = NULL;

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

    pWmvDec = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pWmvDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pWmvDec->hMFCWmvHandle.hMFCHandle;
    pDecOps    = pWmvDec->hMFCWmvHandle.pDecOps;
    pInbufOps  = pWmvDec->hMFCWmvHandle.pInbufOps;
    pOutbufOps = pWmvDec->hMFCWmvHandle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX)
        pInbufOps->Run(hMFCHandle);
    else if (nPortIndex == OUTPUT_PORT_INDEX)
        pOutbufOps->Run(hMFCHandle);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE WmvCodecStop(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = NULL;
    EXYNOS_WMVDEC_HANDLE            *pWmvDec            = NULL;
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

    pWmvDec = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pWmvDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pWmvDec->hMFCWmvHandle.hMFCHandle;
    pDecOps    = pWmvDec->hMFCWmvHandle.pDecOps;
    pInbufOps  = pWmvDec->hMFCWmvHandle.pInbufOps;
    pOutbufOps = pWmvDec->hMFCWmvHandle.pOutbufOps;

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

OMX_ERRORTYPE WmvCodecOutputBufferProcessRun(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoDecOps       *pDecOps    = NULL;
    ExynosVideoDecBufferOps *pInbufOps  = NULL;
    ExynosVideoDecBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    EXYNOS_WMVDEC_HANDLE   *pWmvDec = NULL;

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
    pWmvDec = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pWmvDec == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pWmvDec->hMFCWmvHandle.hMFCHandle;
    pDecOps    = pWmvDec->hMFCWmvHandle.pDecOps;
    pInbufOps  = pWmvDec->hMFCWmvHandle.pInbufOps;
    pOutbufOps = pWmvDec->hMFCWmvHandle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX) {
        if (pWmvDec->bSourceStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pWmvDec->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    if (nPortIndex == OUTPUT_PORT_INDEX) {
        if (pWmvDec->bDestinationStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pWmvDec->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE WMVCodecRegistCodecBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex,
    int                  nBufferCnt)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_WMVDEC_HANDLE            *pWmvDec            = (EXYNOS_WMVDEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                            *hMFCHandle         = pWmvDec->hMFCWmvHandle.hMFCHandle;
    CODEC_DEC_BUFFER               **ppCodecBuffer      = NULL;
    ExynosVideoDecBufferOps         *pBufOps            = NULL;
    ExynosVideoPlane                *pPlanes            = NULL;

    int nPlaneCnt = 0;
    int i, j;

    FunctionIn();

    if (nPortIndex == INPUT_PORT_INDEX) {
        ppCodecBuffer   = &(pVideoDec->pMFCDecInputBuffer[0]);
        pBufOps         = pWmvDec->hMFCWmvHandle.pInbufOps;
    } else {
        ppCodecBuffer   = &(pVideoDec->pMFCDecOutputBuffer[0]);
        pBufOps         = pWmvDec->hMFCWmvHandle.pOutbufOps;
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

OMX_ERRORTYPE WmvCodecReconfigAllBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT             *pExynosPort        = &pExynosComponent->pExynosPort[nPortIndex];
    EXYNOS_WMVDEC_HANDLE            *pWmvDec            = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    void                            *hMFCHandle         = pWmvDec->hMFCWmvHandle.hMFCHandle;
    ExynosVideoDecBufferOps         *pBufferOps         = NULL;

    FunctionIn();

    if ((nPortIndex == INPUT_PORT_INDEX) &&
        (pWmvDec->bSourceStart == OMX_TRUE)) {
        ret = OMX_ErrorNotImplemented;
        goto EXIT;
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) &&
               (pWmvDec->bDestinationStart == OMX_TRUE)) {
        pBufferOps = pWmvDec->hMFCWmvHandle.pOutbufOps;

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
            ret = WmvCodecDstSetup(pOMXComponent);
            if (ret != OMX_ErrorNone) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed to WmvCodecDstSetup(0x%x)", __func__, __LINE__, ret);
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

OMX_ERRORTYPE WmvCodecEnQueueAllBuffer(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_WMVDEC_HANDLE         *pWmvDec = (EXYNOS_WMVDEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pWmvDec->hMFCWmvHandle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    int i, nOutbufs;

    ExynosVideoDecOps       *pDecOps    = pWmvDec->hMFCWmvHandle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pWmvDec->hMFCWmvHandle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pWmvDec->hMFCWmvHandle.pOutbufOps;

    FunctionIn();

    if ((nPortIndex != INPUT_PORT_INDEX) && (nPortIndex != OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if (nPortIndex == INPUT_PORT_INDEX) {
        Exynos_CodecBufferReset(pExynosComponent, INPUT_PORT_INDEX);

        for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoDec->pMFCDecInputBuffer[%d]: 0x%x", i, pVideoDec->pMFCDecInputBuffer[i]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoDec->pMFCDecInputBuffer[%d]->pVirAddr[0]: 0x%x", i, pVideoDec->pMFCDecInputBuffer[i]->pVirAddr[0]);

            Exynos_CodecBufferEnQueue(pExynosComponent, INPUT_PORT_INDEX, pVideoDec->pMFCDecInputBuffer[i]);
        }

        pInbufOps->Clear_Queue(hMFCHandle);
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) &&
               (pWmvDec->bDestinationStart == OMX_TRUE)) {
        Exynos_CodecBufferReset(pExynosComponent, OUTPUT_PORT_INDEX);

        for (i = 0; i < pWmvDec->hMFCWmvHandle.maxDPBNum; i++) {
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

OMX_ERRORTYPE WmvCodecCheckResolution(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_WMVDEC_HANDLE          *pWmvDec            = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle         = pWmvDec->hMFCWmvHandle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pOutputPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_EXCEPTION_STATE     eOutputExcepState  = pOutputPort->exceptionFlag;

    ExynosVideoDecOps             *pDecOps            = pWmvDec->hMFCWmvHandle.pDecOps;
    ExynosVideoDecBufferOps       *pOutbufOps         = pWmvDec->hMFCWmvHandle.pOutbufOps;
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

    if ((codecOutbufConf.nFrameWidth != pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameWidth) ||
        (codecOutbufConf.nFrameHeight != pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameHeight) ||
        (codecOutbufConf.nStride != pWmvDec->hMFCWmvHandle.codecOutbufConf.nStride) ||
#if 0  // TODO: check posibility
        (codecOutbufConf.eColorFormat != pWmvDec->hMFCWmvHandle.codecOutbufConf.eColorFormat) ||
        (codecOutbufConf.eFilledDataType != pWmvDec->hMFCWmvHandle.codecOutbufConf.eFilledDataType) ||
        (codecOutbufConf.bInterlaced != pWmvDec->hMFCWmvHandle.codecOutbufConf.bInterlaced) ||
#endif
        (maxDPBNum != pWmvDec->hMFCWmvHandle.maxDPBNum)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] DRC: W(%d), H(%d) -> W(%d), H(%d)",
                            pExynosComponent, __FUNCTION__,
                            codecOutbufConf.nFrameWidth,
                            codecOutbufConf.nFrameHeight,
                            pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameWidth,
                            pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameHeight);
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] DRC: DPB(%d), FORMAT(0x%x), TYPE(0x%x) -> DPB(%d), FORMAT(0x%x), TYPE(0x%x)",
                            pExynosComponent, __FUNCTION__,
                            maxDPBNum, codecOutbufConf.eColorFormat, codecOutbufConf.eFilledDataType,
                            pWmvDec->hMFCWmvHandle.maxDPBNum,
                            pWmvDec->hMFCWmvHandle.codecOutbufConf.eColorFormat,
                            pWmvDec->hMFCWmvHandle.codecOutbufConf.eFilledDataType);

        pInputPortDefinition->format.video.nFrameWidth     = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameWidth;
        pInputPortDefinition->format.video.nFrameHeight    = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameHeight;
        pInputPortDefinition->format.video.nStride         = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameWidth;
        pInputPortDefinition->format.video.nSliceHeight    = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameHeight;

        if (pOutputPort->bufferProcessType & BUFFER_SHARE) {
            pOutputPortDefinition->nBufferCountActual  = pWmvDec->hMFCWmvHandle.maxDPBNum;
            pOutputPortDefinition->nBufferCountMin     = pWmvDec->hMFCWmvHandle.maxDPBNum;
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

    if ((codecOutbufConf.cropRect.nTop != pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nTop) ||
        (codecOutbufConf.cropRect.nLeft != pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nLeft) ||
        (codecOutbufConf.cropRect.nWidth != pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nWidth) ||
        (codecOutbufConf.cropRect.nHeight != pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nHeight)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] CROP: W(%d), H(%d) -> W(%d), H(%d)",
                            pExynosComponent, __FUNCTION__,
                            codecOutbufConf.cropRect.nWidth,
                            codecOutbufConf.cropRect.nHeight,
                            pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nWidth,
                            pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nHeight);

        pCropRectangle->nTop     = pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nTop;
        pCropRectangle->nLeft    = pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nLeft;
        pCropRectangle->nWidth   = pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nWidth;
        pCropRectangle->nHeight  = pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nHeight;

        /** Send crop info call back **/
        (*(pExynosComponent->pCallbacks->EventHandler))
            (pOMXComponent,
             pExynosComponent->callbackData,
             OMX_EventPortSettingsChanged, /* The command was completed */
             OMX_DirOutput, /* This is the port index */
             OMX_IndexConfigCommonOutputCrop,
             NULL);
    }

    Exynos_OSAL_Memcpy(&pWmvDec->hMFCWmvHandle.codecOutbufConf, &codecOutbufConf, sizeof(codecOutbufConf));
    pWmvDec->hMFCWmvHandle.maxDPBNum = maxDPBNum;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE WmvCodecUpdateResolution(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_WMVDEC_HANDLE          *pWmvDec            = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle         = pWmvDec->hMFCWmvHandle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pOutputPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    ExynosVideoDecOps             *pDecOps            = pWmvDec->hMFCWmvHandle.pDecOps;
    ExynosVideoDecBufferOps       *pOutbufOps         = pWmvDec->hMFCWmvHandle.pOutbufOps;

    OMX_CONFIG_RECTTYPE             *pCropRectangle         = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE    *pInputPortDefinition   = NULL;
    OMX_PARAM_PORTDEFINITIONTYPE    *pOutputPortDefinition  = NULL;

    FunctionIn();

    /* get geometry for output */
    Exynos_OSAL_Memset(&pWmvDec->hMFCWmvHandle.codecOutbufConf, 0, sizeof(ExynosVideoGeometry));
    if (pOutbufOps->Get_Geometry(hMFCHandle, &pWmvDec->hMFCWmvHandle.codecOutbufConf) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to get geometry for parsed header info");
        ret = OMX_ErrorCorruptedHeader;
        goto EXIT;
    }

    /* get dpb count */
    pWmvDec->hMFCWmvHandle.maxDPBNum = pDecOps->Get_ActualBufferCount(hMFCHandle);
    if (pVideoDec->bThumbnailMode == OMX_FALSE)
        pWmvDec->hMFCWmvHandle.maxDPBNum += EXTRA_DPB_NUM;
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] WmvCodecSetup maxDPBNum: %d", pExynosComponent, __FUNCTION__, pWmvDec->hMFCWmvHandle.maxDPBNum);

    /* get interlace info */
    if (pWmvDec->hMFCWmvHandle.codecOutbufConf.bInterlaced == VIDEO_TRUE)
        Exynos_OSAL_Log(EXYNOS_LOG_INFO, "detect an interlaced type");

    pWmvDec->hMFCWmvHandle.bConfiguredMFCSrc = OMX_TRUE;

    if (pVideoDec->bReconfigDPB != OMX_TRUE) {
        pCropRectangle          = &(pOutputPort->cropRectangle);
        pInputPortDefinition    = &(pInputPort->portDefinition);
        pOutputPortDefinition   = &(pOutputPort->portDefinition);
    } else {
        pCropRectangle          = &(pOutputPort->newCropRectangle);
        pInputPortDefinition    = &(pInputPort->newPortDefinition);
        pOutputPortDefinition   = &(pOutputPort->newPortDefinition);
    }

    pCropRectangle->nTop     = pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nTop;
    pCropRectangle->nLeft    = pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nLeft;
    pCropRectangle->nWidth   = pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nWidth;
    pCropRectangle->nHeight  = pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nHeight;

    if (pOutputPort->bufferProcessType & BUFFER_COPY) {
        if ((pVideoDec->bReconfigDPB) ||
            (pInputPort->portDefinition.format.video.nFrameWidth != pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameWidth) ||
            (pInputPort->portDefinition.format.video.nFrameHeight != pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameHeight)) {
            pOutputPort->exceptionFlag = NEED_PORT_DISABLE;

            pInputPortDefinition->format.video.nFrameWidth  = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nFrameHeight = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameHeight;
            pInputPortDefinition->format.video.nStride      = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nSliceHeight = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameHeight;

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
            (pWmvDec->hMFCWmvHandle.codecOutbufConf.bInterlaced == VIDEO_TRUE) ||
            (pInputPort->portDefinition.format.video.nFrameWidth != pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameWidth) ||
            (pInputPort->portDefinition.format.video.nFrameHeight != pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameHeight) ||
            ((OMX_S32)pOutputPort->portDefinition.nBufferCountActual != pWmvDec->hMFCWmvHandle.maxDPBNum)) {
            pOutputPort->exceptionFlag = NEED_PORT_DISABLE;

            pInputPortDefinition->format.video.nFrameWidth  = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nFrameHeight = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameHeight;
            pInputPortDefinition->format.video.nStride      = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameWidth;
            pInputPortDefinition->format.video.nSliceHeight = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameHeight;

            pOutputPortDefinition->nBufferCountActual       = pWmvDec->hMFCWmvHandle.maxDPBNum;
            pOutputPortDefinition->nBufferCountMin          = pWmvDec->hMFCWmvHandle.maxDPBNum;

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
        ((pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameWidth != pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nWidth) ||
         (pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameHeight != pWmvDec->hMFCWmvHandle.codecOutbufConf.cropRect.nHeight))) {
        /* Check Crop */
        pInputPortDefinition->format.video.nFrameWidth  = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameWidth;
        pInputPortDefinition->format.video.nFrameHeight = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameHeight;
        pInputPortDefinition->format.video.nStride      = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameWidth;
        pInputPortDefinition->format.video.nSliceHeight = pWmvDec->hMFCWmvHandle.codecOutbufConf.nFrameHeight;

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

OMX_ERRORTYPE WmvCodecSrcSetup(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec         = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_WMVDEC_HANDLE          *pWmvDec           = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle        = pWmvDec->hMFCWmvHandle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32                        oneFrameSize      = pSrcInputData->dataLen;
    OMX_COLOR_FORMATTYPE           eOutputFormat     = pExynosOutputPort->portDefinition.format.video.eColorFormat;
    OMX_BOOL                       bMetaData         = OMX_FALSE;

    ExynosVideoDecOps       *pDecOps    = pWmvDec->hMFCWmvHandle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pWmvDec->hMFCWmvHandle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pWmvDec->hMFCWmvHandle.pOutbufOps;
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
        ret = Exynos_OSAL_Queue(&pWmvDec->bypassBufferInfoQ, (void *)pBufferInfo);
        Exynos_OSAL_SignalSet(pWmvDec->hDestinationStartEvent);

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

    if (pSrcInputData->nFlags & OMX_BUFFERFLAG_CODECCONFIG)
        pWmvDec->hMFCWmvHandle.wmvFormat = getFormatType(pSrcInputData->multiPlaneBuffer.dataBuffer[0]);

    if (pWmvDec->hMFCWmvHandle.wmvFormat == WMV_FORMAT_WMV3) {
        bufferConf.eCompressionFormat = VIDEO_CODING_VC1_RCV;
    } else if (pWmvDec->hMFCWmvHandle.wmvFormat == WMV_FORMAT_VC1) {
        bufferConf.eCompressionFormat = VIDEO_CODING_VC1;
    } else {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Unsupported WMV Codec Format Type");
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

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
        if ((pWmvDec->hMFCWmvHandle.videoInstInfo.specificInfo.dec.bDualDPBSupport == VIDEO_TRUE) &&
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

    pWmvDec->hMFCWmvHandle.MFCOutputColorType = bufferConf.eColorFormat;
    bufferConf.nPlaneCnt = Exynos_GetPlaneFromPort(pExynosOutputPort);
    if (pOutbufOps->Set_Geometry(hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    bMetaData = Make_Stream_MetaData(pSrcInputData->multiPlaneBuffer.dataBuffer[0], &oneFrameSize, pWmvDec->hMFCWmvHandle.wmvFormat);
    if (bMetaData == OMX_FALSE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Fail to Make Stream MetaData");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    /* input buffer enqueue for header parsing */
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] Header Size: %d", pExynosComponent, __FUNCTION__, oneFrameSize);
    if (pExynosInputPort->bufferProcessType & BUFFER_SHARE)
        nAllocLen[0] = pSrcInputData->bufferHeader->nAllocLen;
    else if (pExynosInputPort->bufferProcessType & BUFFER_COPY)
        nAllocLen[0] = DEFAULT_MFC_INPUT_BUFFER_SIZE;

    nDataLen[0] = oneFrameSize;  /* it is possible to be changed at Make_Stream_MetaData() */

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

    ret = WmvCodecUpdateResolution(pOMXComponent);
    if (((EXYNOS_OMX_ERRORTYPE)ret == OMX_ErrorCorruptedHeader) &&
        (pExynosComponent->codecType != HW_VIDEO_DEC_SECURE_CODEC) &&
        (oneFrameSize >= 8))
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "CorruptedHeader Info : %02x %02x %02x %02x %02x %02x %02x %02x ...",
                                                                    *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0])    , *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 1),
                                                                    *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 2), *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 3),
                                                                    *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 4), *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 5),
                                                                    *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 6), *((OMX_U8 *)pSrcInputData->multiPlaneBuffer.dataBuffer[0] + 7));
    if (ret != OMX_ErrorNone) {
        WmvCodecStop(pOMXComponent, INPUT_PORT_INDEX);
        pInbufOps->Cleanup_Buffer(hMFCHandle);
        goto EXIT;
    }

    Exynos_OSAL_SleepMillisec(0);
    /*  disable header info re-input scheme
      ret = OMX_ErrorInputDataDecodeYet;
      WmvCodecStop(pOMXComponent, INPUT_PORT_INDEX);
    */
    ret = OMX_ErrorNoneSrcSetupFinish;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE WmvCodecDstSetup(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_WMVDEC_HANDLE          *pWmvDec = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle = pWmvDec->hMFCWmvHandle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoDecOps       *pDecOps    = pWmvDec->hMFCWmvHandle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pWmvDec->hMFCWmvHandle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pWmvDec->hMFCWmvHandle.pOutbufOps;

    unsigned int nAllocLen[MAX_BUFFER_PLANE] = {0, 0, 0};
    unsigned int nDataLen[MAX_BUFFER_PLANE]  = {0, 0, 0};
    int i, nOutbufs, nPlaneCnt;

    FunctionIn();

    nPlaneCnt = Exynos_GetPlaneFromPort(pExynosOutputPort);
    for (i = 0; i < nPlaneCnt; i++)
        nAllocLen[i] = pWmvDec->hMFCWmvHandle.codecOutbufConf.nAlignPlaneSize[i];

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
            nOutbufs = pWmvDec->hMFCWmvHandle.maxDPBNum;
            if (pOutbufOps->Setup(hMFCHandle, nOutbufs) != VIDEO_ERROR_NONE) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup output buffer");
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }

            ret = Exynos_Allocate_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX, nOutbufs, nAllocLen);
            if (ret != OMX_ErrorNone)
                goto EXIT;

            /* Register output buffer */
            ret = WMVCodecRegistCodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX, nOutbufs);
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
            nOutbufs = pWmvDec->hMFCWmvHandle.maxDPBNum;
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

    pWmvDec->hMFCWmvHandle.bConfiguredMFCDst = OMX_TRUE;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_WmvDec_GetParameter(
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
    case OMX_IndexParamVideoWmv:
    {
        OMX_VIDEO_PARAM_WMVTYPE *pDstWmvParam = (OMX_VIDEO_PARAM_WMVTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_WMVTYPE *pSrcWmvParam = NULL;
        EXYNOS_WMVDEC_HANDLE    *pWmvDec      = NULL;
        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pDstWmvParam, sizeof(OMX_VIDEO_PARAM_WMVTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstWmvParam->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pWmvDec = (EXYNOS_WMVDEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcWmvParam = &pWmvDec->WmvComponent[pDstWmvParam->nPortIndex];

        Exynos_OSAL_Memcpy(((char *)pDstWmvParam) + nOffset,
                           ((char *)pSrcWmvParam) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_WMVTYPE) - nOffset);
    }
        break;
    case OMX_IndexParamVideoVC1:
    {
        OMX_VIDEO_PARAM_VC1TYPE *pDstVc1Param = (OMX_VIDEO_PARAM_VC1TYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_VC1TYPE *pSrcVc1Param = NULL;
        EXYNOS_WMVDEC_HANDLE    *pWmvDec      = NULL;
        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pDstVc1Param, sizeof(OMX_VIDEO_PARAM_VC1TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstVc1Param->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pWmvDec = (EXYNOS_WMVDEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcVc1Param = &pWmvDec->Vc1Component[pDstVc1Param->nPortIndex];

        Exynos_OSAL_Memcpy(((char *)pDstVc1Param) + nOffset,
                           ((char *)pSrcVc1Param) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_VC1TYPE) - nOffset);
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_WMV_DEC_ROLE);
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
        OMX_VIDEO_PARAM_VC1TYPE          *pSrcVc1Component = NULL;
        EXYNOS_WMVDEC_HANDLE             *pWmvDec          = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pWmvDec = (EXYNOS_WMVDEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcVc1Component = &pWmvDec->Vc1Component[pDstProfileLevel->nPortIndex];

        pDstProfileLevel->eProfile = pSrcVc1Component->eProfile;
        pDstProfileLevel->eLevel   = pSrcVc1Component->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;
        EXYNOS_WMVDEC_HANDLE                *pWmvDec                 = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pWmvDec = (EXYNOS_WMVDEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pSrcErrorCorrectionType = &pWmvDec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
#ifdef USE_TIMESTAMP_REORDER_SUPPORT
    case OMX_IndexExynosParamReorderMode:  /* MSRND */
    {
        EXYNOS_OMX_VIDEODEC_COMPONENT       *pVideoDec      = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
        EXYNOS_OMX_VIDEO_PARAM_REORDERMODE  *pReorderParam  = (EXYNOS_OMX_VIDEO_PARAM_REORDERMODE *)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pReorderParam, sizeof(EXYNOS_OMX_VIDEO_PARAM_REORDERMODE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        pReorderParam->bReorderMode = pVideoDec->bReorderMode;
    }
        break;
#endif
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE  *portDefinition     = (OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;
        OMX_U32                        portIndex          = portDefinition->nPortIndex;
        EXYNOS_OMX_BASECOMPONENT      *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
        EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
        EXYNOS_WMVDEC_HANDLE          *pWmvDec            = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
        EXYNOS_OMX_BASEPORT           *pExynosPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

        ret = Exynos_OMX_VideoDecodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (((pExynosPort->bIsANBEnabled == OMX_TRUE) || (pExynosPort->bStoreMetaData == OMX_TRUE)) &&
            (pWmvDec->hMFCWmvHandle.codecOutbufConf.bInterlaced == VIDEO_TRUE) &&
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

OMX_ERRORTYPE Exynos_WmvDec_SetParameter(
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
    case OMX_IndexParamVideoWmv:
    {
        OMX_VIDEO_PARAM_WMVTYPE *pDstWmvParam = NULL;
        OMX_VIDEO_PARAM_WMVTYPE *pSrcWmvParam = (OMX_VIDEO_PARAM_WMVTYPE *)pComponentParameterStructure;
        EXYNOS_WMVDEC_HANDLE    *pWmvDec      = NULL;
        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pSrcWmvParam, sizeof(OMX_VIDEO_PARAM_WMVTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcWmvParam->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pWmvDec = (EXYNOS_WMVDEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstWmvParam = &pWmvDec->WmvComponent[pSrcWmvParam->nPortIndex];

        Exynos_OSAL_Memcpy(((char *)pDstWmvParam) + nOffset,
                           ((char *)pSrcWmvParam) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_WMVTYPE) - nOffset);
    }
        break;
    case OMX_IndexParamVideoVC1:
    {
        OMX_VIDEO_PARAM_VC1TYPE *pDstVc1Param = NULL;
        OMX_VIDEO_PARAM_VC1TYPE *pSrcVc1Param = (OMX_VIDEO_PARAM_VC1TYPE *)pComponentParameterStructure;
        EXYNOS_WMVDEC_HANDLE    *pWmvDec      = NULL;
        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pSrcVc1Param, sizeof(OMX_VIDEO_PARAM_VC1TYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcVc1Param->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pWmvDec = (EXYNOS_WMVDEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstVc1Param = &pWmvDec->Vc1Component[pSrcVc1Param->nPortIndex];

        Exynos_OSAL_Memcpy(((char *)pDstVc1Param) + nOffset,
                           ((char *)pSrcVc1Param) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_VC1TYPE) - nOffset);
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

        if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_WMV_DEC_ROLE)) {
            pExynosComponent->pExynosPort[INPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_VC1TYPE          *pDstVc1Component = NULL;
        EXYNOS_WMVDEC_HANDLE             *pVc1Dec          = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pVc1Dec = (EXYNOS_WMVDEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

        pDstVc1Component = &pVc1Dec->Vc1Component[pSrcProfileLevel->nPortIndex];

        if (OMX_FALSE == CheckProfileLevelSupport(pExynosComponent, pSrcProfileLevel)) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        pDstVc1Component->eProfile  = pSrcProfileLevel->eProfile;
        pDstVc1Component->eLevel    = pSrcProfileLevel->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;
        EXYNOS_WMVDEC_HANDLE                *pWmvDec                 = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pWmvDec = (EXYNOS_WMVDEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
        pDstErrorCorrectionType = &pWmvDec->errorCorrectionType[INPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
#ifdef USE_TIMESTAMP_REORDER_SUPPORT
    case OMX_IndexExynosParamReorderMode:  /* MSRND */
    {
        EXYNOS_OMX_VIDEODEC_COMPONENT       *pVideoDec      = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
        EXYNOS_OMX_VIDEO_PARAM_REORDERMODE  *pReorderParam  = (EXYNOS_OMX_VIDEO_PARAM_REORDERMODE *)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pReorderParam, sizeof(EXYNOS_OMX_VIDEO_PARAM_REORDERMODE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        pVideoDec->bReorderMode = pReorderParam->bReorderMode;
    }
        break;
#endif
    default:
        ret = Exynos_OMX_VideoDecodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_WmvDec_GetConfig(
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
        EXYNOS_WMVDEC_HANDLE    *pWmvDec       = NULL;
        OMX_CONFIG_RECTTYPE     *pSrcRectType  = NULL;
        OMX_CONFIG_RECTTYPE     *pDstRectType  = NULL;

        pWmvDec = (EXYNOS_WMVDEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;

        if (pWmvDec->hMFCWmvHandle.bConfiguredMFCSrc == OMX_FALSE) {
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

OMX_ERRORTYPE Exynos_WmvDec_SetConfig(
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

OMX_ERRORTYPE Exynos_WmvDec_GetExtensionIndex(
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

    if (IS_CUSTOM_COMPONENT(pExynosComponent->componentName) == OMX_TRUE) {
#ifdef USE_TIMESTAMP_REORDER_SUPPORT
        if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_CUSTOM_INDEX_PARAM_REORDER_MODE) == 0) {
            *pIndexType = (OMX_INDEXTYPE) OMX_IndexExynosParamReorderMode;
            ret = OMX_ErrorNone;
            goto EXIT;
        }
#endif
    }

    ret = Exynos_OMX_VideoDecodeGetExtensionIndex(hComponent, cParameterName, pIndexType);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_WmvDec_ComponentRoleEnum(
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
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_WMV_DEC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE Exynos_WmvDec_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_WMVDEC_HANDLE          *pWmvDec           = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    OMX_PTR                hMFCHandle = pWmvDec->hMFCWmvHandle.hMFCHandle;

    ExynosVideoDecOps       *pDecOps        = NULL;
    ExynosVideoDecBufferOps *pInbufOps      = NULL;
    ExynosVideoDecBufferOps *pOutbufOps     = NULL;
    ExynosVideoInstInfo     *pVideoInstInfo = &(pWmvDec->hMFCWmvHandle.videoInstInfo);

    CSC_METHOD csc_method = CSC_METHOD_SW;
    int i, plane;

    FunctionIn();

    pWmvDec->hMFCWmvHandle.bConfiguredMFCSrc = OMX_FALSE;
    pWmvDec->hMFCWmvHandle.bConfiguredMFCDst = OMX_FALSE;
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

    /* WMV Codec Open */
    ret = WmvCodecOpen(pWmvDec, pVideoInstInfo);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    pDecOps    = pWmvDec->hMFCWmvHandle.pDecOps;
    pInbufOps  = pWmvDec->hMFCWmvHandle.pInbufOps;
    pOutbufOps = pWmvDec->hMFCWmvHandle.pOutbufOps;

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

    pWmvDec->bSourceStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pWmvDec->hSourceStartEvent);
    pWmvDec->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pWmvDec->hDestinationStartEvent);

    Exynos_OSAL_Memset(pExynosComponent->bTimestampSlotUsed, 0, sizeof(OMX_BOOL) * MAX_TIMESTAMP);
    INIT_ARRAY_TO_VAL(pExynosComponent->timeStamp, DEFAULT_TIMESTAMP_VAL, MAX_TIMESTAMP);
    Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pWmvDec->hMFCWmvHandle.indexTimestamp = 0;
    pWmvDec->hMFCWmvHandle.outputIndexTimestamp = 0;
    /* Default WMV codec format is set as VC1*/
    pWmvDec->hMFCWmvHandle.wmvFormat = WMV_FORMAT_VC1;

    pExynosComponent->getAllDelayBuffer = OMX_FALSE;

    Exynos_OSAL_QueueCreate(&pWmvDec->bypassBufferInfoQ, QUEUE_ELEMENTS);

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
OMX_ERRORTYPE Exynos_WmvDec_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_WMVDEC_HANDLE    *pWmvDec = (EXYNOS_WMVDEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    OMX_PTR                hMFCHandle = pWmvDec->hMFCWmvHandle.hMFCHandle;

    ExynosVideoDecOps       *pDecOps    = pWmvDec->hMFCWmvHandle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pWmvDec->hMFCWmvHandle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps = pWmvDec->hMFCWmvHandle.pOutbufOps;

    int i, plane;

    FunctionIn();

    if (pVideoDec->csc_handle != NULL) {
        csc_deinit(pVideoDec->csc_handle);
        pVideoDec->csc_handle = NULL;
    }

    Exynos_OSAL_QueueTerminate(&pWmvDec->bypassBufferInfoQ);

    Exynos_OSAL_SignalTerminate(pWmvDec->hDestinationStartEvent);
    pWmvDec->hDestinationStartEvent = NULL;
    pWmvDec->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalTerminate(pWmvDec->hSourceStartEvent);
    pWmvDec->hSourceStartEvent = NULL;
    pWmvDec->bSourceStart = OMX_FALSE;

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
    WmvCodecClose(pWmvDec);

    Exynos_ResetAllPortConfig(pOMXComponent);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_WmvDec_SrcIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec         = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_WMVDEC_HANDLE          *pWmvDec           = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    void                          *hMFCHandle        = pWmvDec->hMFCWmvHandle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32                        oneFrameSize      = pSrcInputData->dataLen;

    ExynosVideoDecOps       *pDecOps     = pWmvDec->hMFCWmvHandle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps   = pWmvDec->hMFCWmvHandle.pInbufOps;
    ExynosVideoDecBufferOps *pOutbufOps  = pWmvDec->hMFCWmvHandle.pOutbufOps;
    ExynosVideoErrorType     codecReturn = VIDEO_ERROR_NONE;

    OMX_BUFFERHEADERTYPE tempBufferHeader;
    void *pPrivate = NULL;

    unsigned int nAllocLen[MAX_BUFFER_PLANE] = {0, 0, 0};
    unsigned int nDataLen[MAX_BUFFER_PLANE]  = {oneFrameSize, 0, 0};
    OMX_BOOL bStartCode   = OMX_FALSE;
    int i;

    FunctionIn();

    if (pWmvDec->hMFCWmvHandle.bConfiguredMFCSrc == OMX_FALSE) {
        ret = WmvCodecSrcSetup(pOMXComponent, pSrcInputData);
        goto EXIT;
    }

    if ((pWmvDec->hMFCWmvHandle.bConfiguredMFCDst == OMX_FALSE) &&
        (pVideoDec->bForceHeaderParsing == OMX_FALSE)) {
        ret = WmvCodecDstSetup(pOMXComponent);
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed to WmvCodecDstSetup(0x%x)", __func__, __LINE__, ret);
            goto EXIT;
        }
    }

    bStartCode = Check_Stream_PrefixCode(pSrcInputData->multiPlaneBuffer.dataBuffer[0], oneFrameSize, pWmvDec->hMFCWmvHandle.wmvFormat);
    if ((bStartCode == OMX_FALSE) &&
        ((pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS)) {
        OMX_U32 bufferSizeWithHeader;
        if (pWmvDec->hMFCWmvHandle.wmvFormat == WMV_FORMAT_WMV3)
            bufferSizeWithHeader = oneFrameSize + 8;
        else if (pWmvDec->hMFCWmvHandle.wmvFormat == WMV_FORMAT_VC1)
            bufferSizeWithHeader = oneFrameSize + 4;
        else
            bufferSizeWithHeader = 0;

        if (pSrcInputData->allocSize < bufferSizeWithHeader) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can't attach startcode due to lack of buffer space");
            ret = (OMX_ERRORTYPE)OMX_ErrorCodecDecode;
            goto EXIT;
        }

        bStartCode = Make_Stream_StartCode(pSrcInputData->multiPlaneBuffer.dataBuffer[0], &oneFrameSize, pWmvDec->hMFCWmvHandle.wmvFormat);
        if (bStartCode == OMX_FALSE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Fail to Make Stream Start Code");
            ret = (OMX_ERRORTYPE)OMX_ErrorCodecDecode;
            goto EXIT;
        }
    }

    if ((bStartCode == OMX_TRUE) ||
        ((pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
        if (pVideoDec->bReorderMode == OMX_FALSE) {
            /* next slot will be used like as circular queue */
            pExynosComponent->timeStamp[pWmvDec->hMFCWmvHandle.indexTimestamp] = pSrcInputData->timeStamp;
            pExynosComponent->nFlags[pWmvDec->hMFCWmvHandle.indexTimestamp]    = pSrcInputData->nFlags;
        } else {  /* MSRND */
            Exynos_SetReorderTimestamp(pExynosComponent, &(pWmvDec->hMFCWmvHandle.indexTimestamp), pSrcInputData->timeStamp, pSrcInputData->nFlags);
        }

        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] input timestamp %lld us (%.2f secs), Tag: %d, nFlags: 0x%x, oneFrameSize: %d", pExynosComponent, __FUNCTION__,
                                                        pSrcInputData->timeStamp, pSrcInputData->timeStamp / 1E6, pWmvDec->hMFCWmvHandle.indexTimestamp, pSrcInputData->nFlags, oneFrameSize);

        pDecOps->Set_FrameTag(hMFCHandle, pWmvDec->hMFCWmvHandle.indexTimestamp);
        pWmvDec->hMFCWmvHandle.indexTimestamp++;
        pWmvDec->hMFCWmvHandle.indexTimestamp %= MAX_TIMESTAMP;

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

        nDataLen[0] = oneFrameSize;  /* it is possible to be changed at Make_Stream_MetaData() */

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
        WmvCodecStart(pOMXComponent, INPUT_PORT_INDEX);
        if (pWmvDec->bSourceStart == OMX_FALSE) {
            pWmvDec->bSourceStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pWmvDec->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
        if (pWmvDec->bDestinationStart == OMX_FALSE) {
            pWmvDec->bDestinationStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pWmvDec->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    } else if (bStartCode == OMX_FALSE) {
        ret = OMX_ErrorCorruptedFrame;
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_WmvDec_SrcOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_WMVDEC_HANDLE         *pWmvDec = (EXYNOS_WMVDEC_HANDLE *)((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pWmvDec->hMFCWmvHandle.hMFCHandle;
    EXYNOS_OMX_BASEPORT     *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    ExynosVideoDecOps       *pDecOps    = pWmvDec->hMFCWmvHandle.pDecOps;
    ExynosVideoDecBufferOps *pInbufOps  = pWmvDec->hMFCWmvHandle.pInbufOps;
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
        pSrcOutputData->bufferHeader = NULL;
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

OMX_ERRORTYPE Exynos_WmvDec_DstIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_WMVDEC_HANDLE            *pWmvDec            = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    void                            *hMFCHandle         = pWmvDec->hMFCWmvHandle.hMFCHandle;
    EXYNOS_OMX_BASEPORT             *pExynosOutputPort  = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoDecOps       *pDecOps     = pWmvDec->hMFCWmvHandle.pDecOps;
    ExynosVideoDecBufferOps *pOutbufOps  = pWmvDec->hMFCWmvHandle.pOutbufOps;
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
        nAllocLen[i] = pWmvDec->hMFCWmvHandle.codecOutbufConf.nAlignPlaneSize[i];

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] : %d => ADDR[%d]: 0x%x", pExynosComponent, __FUNCTION__, __LINE__, i,
                                        pDstInputData->multiPlaneBuffer.dataBuffer[i]);
    }

    if ((pVideoDec->bReconfigDPB == OMX_TRUE) &&
        (pExynosOutputPort->bufferProcessType & BUFFER_SHARE) &&
        (pExynosOutputPort->exceptionFlag == GENERAL_STATE)) {
        ret = WmvCodecDstSetup(pOMXComponent);
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed to WmvCodecDstSetup(0x%x)", __func__, __LINE__, ret);
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
    WmvCodecStart(pOMXComponent, OUTPUT_PORT_INDEX);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_WmvDec_DstOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_WMVDEC_HANDLE            *pWmvDec            = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    void                            *hMFCHandle         = pWmvDec->hMFCWmvHandle.hMFCHandle;
    EXYNOS_OMX_BASEPORT             *pExynosInputPort   = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT             *pExynosOutputPort  = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    DECODE_CODEC_EXTRA_BUFFERINFO   *pBufferInfo        = NULL;

    ExynosVideoDecOps           *pDecOps        = pWmvDec->hMFCWmvHandle.pDecOps;
    ExynosVideoDecBufferOps     *pOutbufOps     = pWmvDec->hMFCWmvHandle.pOutbufOps;
    ExynosVideoBuffer           *pVideoBuffer   = NULL;
    ExynosVideoBuffer            videoBuffer;
    ExynosVideoFrameStatusType   displayStatus  = VIDEO_FRAME_STATUS_UNKNOWN;
    ExynosVideoGeometry         *bufferGeometry = NULL;
    ExynosVideoErrorType         codecReturn    = VIDEO_ERROR_NONE;

    OMX_S32 indexTimestamp = 0;
    int plane, nPlaneCnt;

    FunctionIn();

    if (pWmvDec->bDestinationStart == OMX_FALSE) {
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
            WmvCodecUpdateResolution(pOMXComponent);
            pVideoDec->csc_set_format = OMX_FALSE;
        }
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    pWmvDec->hMFCWmvHandle.outputIndexTimestamp++;
    pWmvDec->hMFCWmvHandle.outputIndexTimestamp %= MAX_TIMESTAMP;

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
        (pWmvDec->hMFCWmvHandle.codecOutbufConf.bInterlaced == VIDEO_TRUE) &&
        (pVideoBuffer->planes[2].addr != NULL)) {
        /* only NV12 case */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] interlace type = %x", pExynosComponent, __FUNCTION__, pVideoBuffer->interlacedType);
        *(int *)(pVideoBuffer->planes[2].addr) = pVideoBuffer->interlacedType;
    }

    pBufferInfo = (DECODE_CODEC_EXTRA_BUFFERINFO *)pDstOutputData->extInfo;
    bufferGeometry = &pWmvDec->hMFCWmvHandle.codecOutbufConf;
    pBufferInfo->imageWidth = bufferGeometry->nFrameWidth;
    pBufferInfo->imageHeight = bufferGeometry->nFrameHeight;
    pBufferInfo->imageStride = bufferGeometry->nStride;
    pBufferInfo->ColorFormat = Exynos_OSAL_Video2OMXFormat((int)bufferGeometry->eColorFormat);
    Exynos_OSAL_Memcpy(&pBufferInfo->PDSB, &pVideoBuffer->PDSB, sizeof(PrivateDataShareBuffer));

    indexTimestamp = pDecOps->Get_FrameTag(hMFCHandle);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] out indexTimestamp: %d", pExynosComponent, __FUNCTION__, indexTimestamp);

    if (pVideoDec->bReorderMode == OMX_FALSE) {
        if ((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)) {
            if ((pExynosComponent->checkTimeStamp.needSetStartTimeStamp != OMX_TRUE) &&
                (pExynosComponent->checkTimeStamp.needCheckStartTimeStamp != OMX_TRUE)) {
                if (indexTimestamp == INDEX_AFTER_EOS) {
                    pDstOutputData->timeStamp = 0x00;
                    pDstOutputData->nFlags = 0x00;
                } else {
                    pDstOutputData->timeStamp = pExynosComponent->timeStamp[pWmvDec->hMFCWmvHandle.outputIndexTimestamp];
                    pDstOutputData->nFlags = pExynosComponent->nFlags[pWmvDec->hMFCWmvHandle.outputIndexTimestamp];
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
                    pWmvDec->hMFCWmvHandle.outputIndexTimestamp = indexTimestamp;
                else
                    indexTimestamp = pWmvDec->hMFCWmvHandle.outputIndexTimestamp;
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
    } else {  /* MSRND */
        EXYNOS_OMX_CURRENT_FRAME_TIMESTAMP sCurrentTimestamp;

        Exynos_GetReorderTimestamp(pExynosComponent, &sCurrentTimestamp, indexTimestamp, pVideoBuffer->frameType);

        pDstOutputData->timeStamp   = sCurrentTimestamp.timeStamp;
        pDstOutputData->nFlags      = sCurrentTimestamp.nFlags | OMX_BUFFERFLAG_ENDOFFRAME;

        pExynosComponent->nFlags[sCurrentTimestamp.nIndex]               = 0x00;
        pExynosComponent->bTimestampSlotUsed[sCurrentTimestamp.nIndex]   = OMX_FALSE;

        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] output reorder timestamp %lld us (%.2f secs), sCurrentTimestamp.nIndex: %d, nFlags: 0x%x", pExynosComponent, __FUNCTION__,
                                                        pDstOutputData->timeStamp, pDstOutputData->timeStamp / 1E6, sCurrentTimestamp.nIndex, pDstOutputData->nFlags);
    }

#ifdef PERFORMANCE_DEBUG
    if (pDstOutputData->bufferHeader != NULL) {
        pDstOutputData->bufferHeader->nTimeStamp = pDstOutputData->timeStamp;
        Exynos_OSAL_V4L2CountDecrease(pExynosOutputPort->hBufferCount, pDstOutputData->bufferHeader, OUTPUT_PORT_INDEX);
    }
#endif

    if (pWmvDec->hMFCWmvHandle.videoInstInfo.specificInfo.dec.bLastFrameSupport == VIDEO_FALSE) {
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

OMX_ERRORTYPE Exynos_WmvDec_srcInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
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

    ret = Exynos_WmvDec_SrcIn(pOMXComponent, pSrcInputData);
    if ((ret != OMX_ErrorNone) &&
        ((EXYNOS_OMX_ERRORTYPE)ret != OMX_ErrorInputDataDecodeYet) &&
        ((EXYNOS_OMX_ERRORTYPE)ret != OMX_ErrorNoneSrcSetupFinish) &&
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

OMX_ERRORTYPE Exynos_WmvDec_srcOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_WMVDEC_HANDLE            *pWmvDec            = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
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
    if ((pWmvDec->bSourceStart == OMX_FALSE) &&
       (!CHECK_PORT_BEING_FLUSHED(pExynosInputPort))) {
        Exynos_OSAL_SignalWait(pWmvDec->hSourceStartEvent, DEF_MAX_WAIT_TIME);
        if (pVideoDec->bExitBufferProcessThread)
            goto EXIT;

        Exynos_OSAL_SignalReset(pWmvDec->hSourceStartEvent);
    }

    ret = Exynos_WmvDec_SrcOut(pOMXComponent, pSrcOutputData);
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

OMX_ERRORTYPE Exynos_WmvDec_dstInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_WMVDEC_HANDLE            *pWmvDec            = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
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
        if ((pWmvDec->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pWmvDec->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            if (pVideoDec->bExitBufferProcessThread)
                goto EXIT;

            Exynos_OSAL_SignalReset(pWmvDec->hDestinationStartEvent);
        }
        if (Exynos_OSAL_GetElemNum(&pWmvDec->bypassBufferInfoQ) > 0) {
            BYPASS_BUFFER_INFO *pBufferInfo = (BYPASS_BUFFER_INFO *)Exynos_OSAL_Dequeue(&pWmvDec->bypassBufferInfoQ);
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
    if (pWmvDec->hMFCWmvHandle.bConfiguredMFCDst == OMX_TRUE) {
        ret = Exynos_WmvDec_DstIn(pOMXComponent, pDstInputData);
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

OMX_ERRORTYPE Exynos_WmvDec_dstOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_WMVDEC_HANDLE            *pWmvDec            = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
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
        if ((pWmvDec->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pWmvDec->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            if (pVideoDec->bExitBufferProcessThread)
                goto EXIT;

            Exynos_OSAL_SignalReset(pWmvDec->hDestinationStartEvent);
        }
        if (Exynos_OSAL_GetElemNum(&pWmvDec->bypassBufferInfoQ) > 0) {
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

            pBufferInfo = Exynos_OSAL_Dequeue(&pWmvDec->bypassBufferInfoQ);
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
    ret = Exynos_WmvDec_DstOut(pOMXComponent, pDstOutputData);
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
    EXYNOS_WMVDEC_HANDLE            *pWmvDec            = NULL;
    OMX_S32                          wmvFormat          = WMV_FORMAT_UNKNOWN;
    int i = 0;

    Exynos_OSAL_Get_Log_Property(); // For debuging
    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }
    if ((Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_WMV_DEC, componentName) != 0) &&
        (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_WMV_CUSTOM_DEC, componentName) != 0)) {
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

    pWmvDec = Exynos_OSAL_Malloc(sizeof(EXYNOS_WMVDEC_HANDLE));
    if (pWmvDec == NULL) {
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pWmvDec, 0, sizeof(EXYNOS_WMVDEC_HANDLE));
    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    pVideoDec->hCodecHandle = (OMX_HANDLETYPE)pWmvDec;
    pWmvDec->hMFCWmvHandle.wmvFormat = wmvFormat;
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

    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingWMV;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/wmv");
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
        INIT_SET_SIZE_VERSION(&pWmvDec->WmvComponent[i], OMX_VIDEO_PARAM_WMVTYPE);
        pWmvDec->WmvComponent[i].nPortIndex = i;
        pWmvDec->WmvComponent[i].eFormat    = OMX_VIDEO_WMVFormat9;
    }

    for(i = 0; i < ALL_PORT_NUM; i++) {
        INIT_SET_SIZE_VERSION(&pWmvDec->Vc1Component[i], OMX_VIDEO_PARAM_VC1TYPE);
        pWmvDec->Vc1Component[i].nPortIndex = i;
        pWmvDec->Vc1Component[i].eProfile   = OMX_VIDEO_VC1ProfileMain;
        pWmvDec->Vc1Component[i].eLevel     = OMX_VIDEO_VC1LevelHigh;
    }

    pOMXComponent->GetParameter      = &Exynos_WmvDec_GetParameter;
    pOMXComponent->SetParameter      = &Exynos_WmvDec_SetParameter;
    pOMXComponent->GetConfig         = &Exynos_WmvDec_GetConfig;
    pOMXComponent->SetConfig         = &Exynos_WmvDec_SetConfig;
    pOMXComponent->GetExtensionIndex = &Exynos_WmvDec_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &Exynos_WmvDec_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &Exynos_OMX_ComponentDeinit;

    pExynosComponent->exynos_codec_componentInit      = &Exynos_WmvDec_Init;
    pExynosComponent->exynos_codec_componentTerminate = &Exynos_WmvDec_Terminate;

    pVideoDec->exynos_codec_srcInputProcess  = &Exynos_WmvDec_srcInputBufferProcess;
    pVideoDec->exynos_codec_srcOutputProcess = &Exynos_WmvDec_srcOutputBufferProcess;
    pVideoDec->exynos_codec_dstInputProcess  = &Exynos_WmvDec_dstInputBufferProcess;
    pVideoDec->exynos_codec_dstOutputProcess = &Exynos_WmvDec_dstOutputBufferProcess;

    pVideoDec->exynos_codec_start         = &WmvCodecStart;
    pVideoDec->exynos_codec_stop          = &WmvCodecStop;
    pVideoDec->exynos_codec_bufferProcessRun = &WmvCodecOutputBufferProcessRun;
    pVideoDec->exynos_codec_enqueueAllBuffer = &WmvCodecEnQueueAllBuffer;

#if 0  /* unused code */
    pVideoDec->exynos_checkInputFrame                 = &Check_Wmv_Frame;
    pVideoDec->exynos_codec_getCodecInputPrivateData  = &GetCodecInputPrivateData;
#endif

    pVideoDec->exynos_codec_getCodecOutputPrivateData = &GetCodecOutputPrivateData;
    pVideoDec->exynos_codec_reconfigAllBuffers        = &WmvCodecReconfigAllBuffers;

    pVideoDec->exynos_codec_checkFormatSupport      = &CheckFormatHWSupport;
    pVideoDec->exynos_codec_checkResolutionChange   = &WmvCodecCheckResolution;

    pVideoDec->hSharedMemory = Exynos_OSAL_SharedMemory_Open();
    if (pVideoDec->hSharedMemory == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        Exynos_OSAL_Free(pWmvDec);
        pWmvDec = ((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pWmvDec->hMFCWmvHandle.videoInstInfo.eCodecType = VIDEO_CODING_VC1;
    if (pExynosComponent->codecType == HW_VIDEO_DEC_SECURE_CODEC)
        pWmvDec->hMFCWmvHandle.videoInstInfo.eSecurityType = VIDEO_SECURE;
    else
        pWmvDec->hMFCWmvHandle.videoInstInfo.eSecurityType = VIDEO_NORMAL;

    if (Exynos_Video_GetInstInfo(&(pWmvDec->hMFCWmvHandle.videoInstInfo), VIDEO_TRUE /* dec */) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%p][%s]: Exynos_Video_GetInstInfo is failed", pExynosComponent, __FUNCTION__);
        Exynos_OSAL_Free(pWmvDec);
        pWmvDec = ((EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] GetInstInfo for dec %d %d %d %d", pExynosComponent, __FUNCTION__,
            (pWmvDec->hMFCWmvHandle.videoInstInfo.specificInfo.dec.bDualDPBSupport),
            (pWmvDec->hMFCWmvHandle.videoInstInfo.specificInfo.dec.bDynamicDPBSupport),
            (pWmvDec->hMFCWmvHandle.videoInstInfo.specificInfo.dec.bLastFrameSupport),
            (pWmvDec->hMFCWmvHandle.videoInstInfo.specificInfo.dec.bSkypeSupport));

    if (pWmvDec->hMFCWmvHandle.videoInstInfo.specificInfo.dec.bDynamicDPBSupport == VIDEO_TRUE)
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
    EXYNOS_WMVDEC_HANDLE        *pWmvDec            = NULL;

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

    pWmvDec = (EXYNOS_WMVDEC_HANDLE *)pVideoDec->hCodecHandle;
    if (pWmvDec != NULL) {
        Exynos_OSAL_Free(pWmvDec);
        pWmvDec = pVideoDec->hCodecHandle = NULL;
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
