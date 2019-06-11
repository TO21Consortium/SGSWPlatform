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
 * @file        Exynos_OMX_Vdec.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              HyeYeon Chung (hyeon.chung@samsung.com)
 *              Yunji Kim (yunji.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Exynos_OMX_Macros.h"
#include "Exynos_OSAL_Event.h"
#include "Exynos_OMX_Vdec.h"
#include "Exynos_OMX_VdecControl.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OSAL_SharedMemory.h"
#include "Exynos_OSAL_Thread.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Mutex.h"
#include "Exynos_OSAL_ETC.h"

#ifdef USE_ANB
#include "Exynos_OSAL_Android.h"
#endif

#include "csc.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_VIDEO_DEC"
//#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

inline void Exynos_UpdateFrameSize(OMX_COMPONENTTYPE *pOMXComponent)
{
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *exynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *exynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    if ((exynosOutputPort->portDefinition.format.video.nFrameWidth !=
            exynosInputPort->portDefinition.format.video.nFrameWidth) ||
        (exynosOutputPort->portDefinition.format.video.nFrameHeight !=
            exynosInputPort->portDefinition.format.video.nFrameHeight)) {
        OMX_U32 width = 0, height = 0;

        exynosOutputPort->portDefinition.format.video.nFrameWidth =
            exynosInputPort->portDefinition.format.video.nFrameWidth;
        exynosOutputPort->portDefinition.format.video.nFrameHeight =
            exynosInputPort->portDefinition.format.video.nFrameHeight;
        width = exynosOutputPort->portDefinition.format.video.nStride =
            exynosInputPort->portDefinition.format.video.nStride;
        height = exynosOutputPort->portDefinition.format.video.nSliceHeight =
            exynosInputPort->portDefinition.format.video.nSliceHeight;

        switch((int)exynosOutputPort->portDefinition.format.video.eColorFormat) {
        case OMX_COLOR_FormatYUV420Planar:
        case OMX_COLOR_FormatYUV420SemiPlanar:
        case OMX_SEC_COLOR_FormatYUV420SemiPlanarInterlace:
        case OMX_SEC_COLOR_Format10bitYUV420SemiPlanar:
        case OMX_SEC_COLOR_FormatNV12Tiled:
        case OMX_SEC_COLOR_FormatYVU420Planar:
        case OMX_SEC_COLOR_FormatNV21Linear:
            if (width && height)
                exynosOutputPort->portDefinition.nBufferSize = (ALIGN(width, 16) * ALIGN(height, 16) * 3) / 2;
            break;
        default:
            if (width && height)
                exynosOutputPort->portDefinition.nBufferSize = ALIGN(width, 16) * ALIGN(height, 16) * 2;
            break;
        }
    }

    return;
}

void Exynos_Output_SetSupportFormat(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    OMX_COLOR_FORMATTYPE             ret         = OMX_COLOR_FormatUnused;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec   = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT             *pOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    if ((pVideoDec == NULL) || (pOutputPort == NULL))
        return ;

    if (pOutputPort->supportFormat != NULL) {
        OMX_BOOL ret = OMX_FALSE;
        int nLastIndex = OUTPUT_PORT_SUPPORTFORMAT_DEFAULT_NUM;
        int i;

        /* Default supported formats                                                                  */
        /* Customer wants OMX_COLOR_FormatYUV420SemiPlanar in the default colors format.              */
        /* But, Google wants OMX_COLOR_FormatYUV420Planar in the default colors format.               */
        /* Google's Videoeditor uses OMX_COLOR_FormatYUV420Planar(YV12) in the default colors format. */
        /* Therefore, only when you load the OpenMAX IL component by the customer name,               */
        /* to change the default OMX_COLOR_FormatYUV420SemiPlanar color format.                       */
        /* It is determined by case-sensitive.                                                        */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s, custom?:(%d)",pExynosComponent->componentName, IS_CUSTOM_COMPONENT(pExynosComponent->componentName));
        if (IS_CUSTOM_COMPONENT(pExynosComponent->componentName) != OMX_TRUE) {
            /* Google GED & S.LSI Component, for video editor*/
            pOutputPort->supportFormat[0] = OMX_COLOR_FormatYUV420Planar;
            pOutputPort->supportFormat[1] = OMX_COLOR_FormatYUV420SemiPlanar;
        } else {
            /* Customer Component*/
            pOutputPort->supportFormat[0] = OMX_COLOR_FormatYUV420SemiPlanar;
            pOutputPort->supportFormat[1] = OMX_COLOR_FormatYUV420Planar;
        }

        /* add extra formats, if It is supported by H/W. (CSC doesn't exist) */
        /* OMX_SEC_COLOR_FormatNV12Tiled */
        ret = pVideoDec->exynos_codec_checkFormatSupport(pExynosComponent,
                                            (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV12Tiled);
        if (ret == OMX_TRUE)
            pOutputPort->supportFormat[nLastIndex++] = (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV12Tiled;

        /* OMX_SEC_COLOR_FormatYVU420Planar */
        ret = pVideoDec->exynos_codec_checkFormatSupport(pExynosComponent,
                                            (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatYVU420Planar);
        if (ret == OMX_TRUE)
            pOutputPort->supportFormat[nLastIndex++] = (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatYVU420Planar;

        /* OMX_SEC_COLOR_FormatNV21Linear */
        ret = pVideoDec->exynos_codec_checkFormatSupport(pExynosComponent, OMX_SEC_COLOR_FormatNV21Linear);
        if (ret == OMX_TRUE)
            pOutputPort->supportFormat[nLastIndex++] = (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV21Linear;

        for (i = 0; i < nLastIndex; i++)
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Support Format[%d] : 0x%x", i, pOutputPort->supportFormat[i]);

        pOutputPort->supportFormat[nLastIndex] = OMX_COLOR_FormatUnused;
    }

    return ;
}

OMX_ERRORTYPE Exynos_ResolutionUpdate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT           *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pOutputPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    pOutputPort->cropRectangle.nTop     = pOutputPort->newCropRectangle.nTop;
    pOutputPort->cropRectangle.nLeft    = pOutputPort->newCropRectangle.nLeft;
    pOutputPort->cropRectangle.nWidth   = pOutputPort->newCropRectangle.nWidth;
    pOutputPort->cropRectangle.nHeight  = pOutputPort->newCropRectangle.nHeight;

    pInputPort->portDefinition.format.video.nFrameWidth     = pInputPort->newPortDefinition.format.video.nFrameWidth;
    pInputPort->portDefinition.format.video.nFrameHeight    = pInputPort->newPortDefinition.format.video.nFrameHeight;
    pInputPort->portDefinition.format.video.nStride         = pInputPort->newPortDefinition.format.video.nStride;
    pInputPort->portDefinition.format.video.nSliceHeight    = pInputPort->newPortDefinition.format.video.nSliceHeight;

    pOutputPort->portDefinition.nBufferCountActual  = pOutputPort->newPortDefinition.nBufferCountActual;
    pOutputPort->portDefinition.nBufferCountMin     = pOutputPort->newPortDefinition.nBufferCountMin;

    Exynos_UpdateFrameSize(pOMXComponent);

    /** Send crop info call back **/
    (*(pExynosComponent->pCallbacks->EventHandler))
        (pOMXComponent,
         pExynosComponent->callbackData,
         OMX_EventPortSettingsChanged, /* The command was completed */
         OMX_DirOutput, /* This is the port index */
         OMX_IndexConfigCommonOutputCrop,
         NULL);

    return ret;
}

void Exynos_Free_CodecBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    CODEC_DEC_BUFFER               **ppCodecBuffer      = NULL;

    int nBufferCnt = 0, nPlaneCnt = 0;
    int i, j;

    FunctionIn();

    if (nPortIndex == INPUT_PORT_INDEX) {
        ppCodecBuffer = &(pVideoDec->pMFCDecInputBuffer[0]);
        nBufferCnt = MFC_INPUT_BUFFER_NUM_MAX;
    } else {
        ppCodecBuffer = &(pVideoDec->pMFCDecOutputBuffer[0]);
        nBufferCnt = MFC_OUTPUT_BUFFER_NUM_MAX;
    }

    nPlaneCnt = Exynos_GetPlaneFromPort(&pExynosComponent->pExynosPort[nPortIndex]);
    for (i = 0; i < nBufferCnt; i++) {
        if (ppCodecBuffer[i] != NULL) {
            for (j = 0; j < nPlaneCnt; j++) {
                if (ppCodecBuffer[i]->pVirAddr[j] != NULL)
                    Exynos_OSAL_SharedMemory_Free(pVideoDec->hSharedMemory, ppCodecBuffer[i]->pVirAddr[j]);
            }

            Exynos_OSAL_Free(ppCodecBuffer[i]);
            ppCodecBuffer[i] = NULL;
        }
    }

    FunctionOut();
}

OMX_ERRORTYPE Exynos_Allocate_CodecBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex,
    int                  nBufferCnt,
    unsigned int         nAllocLen[MAX_BUFFER_PLANE])
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    MEMORY_TYPE                      eMemoryType        = CACHED_MEMORY;
    CODEC_DEC_BUFFER               **ppCodecBuffer      = NULL;

    int nPlaneCnt = 0;
    int i, j;

    FunctionIn();

    if (nPortIndex == INPUT_PORT_INDEX) {
        ppCodecBuffer = &(pVideoDec->pMFCDecInputBuffer[0]);
    } else {
        ppCodecBuffer = &(pVideoDec->pMFCDecOutputBuffer[0]);
#ifdef USE_CSC_HW
        eMemoryType = NORMAL_MEMORY;
#endif
    }
    nPlaneCnt = Exynos_GetPlaneFromPort(&pExynosComponent->pExynosPort[nPortIndex]);

    if (pExynosComponent->codecType == HW_VIDEO_DEC_SECURE_CODEC)
        eMemoryType = SECURE_MEMORY;

    for (i = 0; i < nBufferCnt; i++) {
        ppCodecBuffer[i] = (CODEC_DEC_BUFFER *)Exynos_OSAL_Malloc(sizeof(CODEC_DEC_BUFFER));
        if (ppCodecBuffer[i] == NULL) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to Alloc codec buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
        Exynos_OSAL_Memset(ppCodecBuffer[i], 0, sizeof(CODEC_DEC_BUFFER));

        for (j = 0; j < nPlaneCnt; j++) {
            ppCodecBuffer[i]->pVirAddr[j] =
                (void *)Exynos_OSAL_SharedMemory_Alloc(pVideoDec->hSharedMemory, nAllocLen[j], eMemoryType);
            if (ppCodecBuffer[i]->pVirAddr[j] == NULL) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to Alloc plane");
                ret = OMX_ErrorInsufficientResources;
                goto EXIT;
            }

            ppCodecBuffer[i]->fd[j] =
                Exynos_OSAL_SharedMemory_VirtToION(pVideoDec->hSharedMemory, ppCodecBuffer[i]->pVirAddr[j]);
            ppCodecBuffer[i]->bufferSize[j] = nAllocLen[j];
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PORT[%d]: pMFCCodecBuffer[%d]->pVirAddr[%d]: 0x%x", nPortIndex, i, j, ppCodecBuffer[i]->pVirAddr[j]);
        }

        ppCodecBuffer[i]->dataSize = 0;
    }

    return OMX_ErrorNone;

EXIT:
    Exynos_Free_CodecBuffers(pOMXComponent, nPortIndex);

    FunctionOut();

    return ret;
}

void Exynos_SetReorderTimestamp(
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent,
    OMX_U32                     *nIndex,
    OMX_TICKS                    timeStamp,
    OMX_U32                      nFlags) {

    int i;

    FunctionIn();

    if ((pExynosComponent == NULL) || (nIndex == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter : pExynosComponent(%p), nIndex(%p)", pExynosComponent, nIndex);
        return;
    }

    /* find a empty slot */
    for (i = 0; i < MAX_TIMESTAMP; i++) {
        if (pExynosComponent->bTimestampSlotUsed[*nIndex] == OMX_FALSE)
            break;

        (*nIndex)++;
        (*nIndex) %= MAX_TIMESTAMP;
    }

    if (i >= MAX_TIMESTAMP)
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not find empty slot of timestamp. Timestamp slot is full.");

    pExynosComponent->timeStamp[*nIndex]          = timeStamp;
    pExynosComponent->nFlags[*nIndex]             = nFlags;
    pExynosComponent->bTimestampSlotUsed[*nIndex] = OMX_TRUE;

    FunctionOut();
    return;
}

void Exynos_GetReorderTimestamp(
    EXYNOS_OMX_BASECOMPONENT            *pExynosComponent,
    EXYNOS_OMX_CURRENT_FRAME_TIMESTAMP  *sCurrentTimestamp,
    OMX_S32                              nFrameIndex,
    OMX_S32                              eFrameType) {

    EXYNOS_OMX_BASEPORT         *pExynosOutputPort  = NULL;
    int i = 0;

    FunctionIn();

    if ((pExynosComponent == NULL) || (sCurrentTimestamp == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter : pExynosComponent(%p), sCurrentTimestamp(%p)", pExynosComponent, sCurrentTimestamp);
        return;
    }

    pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    Exynos_OSAL_Memset(sCurrentTimestamp, 0, sizeof(EXYNOS_OMX_CURRENT_FRAME_TIMESTAMP));
    sCurrentTimestamp->timeStamp = DEFAULT_TIMESTAMP_VAL;

    for (i = 0; i < MAX_TIMESTAMP; i++) {
        /* NOTE: In case of CODECCONFIG, no return any frame */
        if ((pExynosComponent->bTimestampSlotUsed[i] == OMX_TRUE) &&
            (pExynosComponent->nFlags[i] != (OMX_BUFFERFLAG_CODECCONFIG | OMX_BUFFERFLAG_ENDOFFRAME))) {

            /* NOTE: In case of EOS, timestamp is not valid */
            if ((sCurrentTimestamp->timeStamp == DEFAULT_TIMESTAMP_VAL) ||
                ((sCurrentTimestamp->timeStamp > pExynosComponent->timeStamp[i]) &&
                    (((pExynosComponent->nFlags[i] & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS) ||
                     (pExynosComponent->bBehaviorEOS == OMX_TRUE))) ||
                ((sCurrentTimestamp->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
                sCurrentTimestamp->timeStamp = pExynosComponent->timeStamp[i];
                sCurrentTimestamp->nFlags    = pExynosComponent->nFlags[i];
                sCurrentTimestamp->nIndex    = i;
            }
        }
    }

    if (sCurrentTimestamp->timeStamp == DEFAULT_TIMESTAMP_VAL)
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] could not find a valid timestamp", pExynosComponent, __FUNCTION__);

    /* PTS : all index is same as tag */
    /* DTS : only in case of I-Frame, the index is same as tag */
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] disp_pic_frame_type: %d", pExynosComponent, __FUNCTION__, eFrameType);
    if ((ExynosVideoFrameType)eFrameType & VIDEO_FRAME_I) {
        /* Timestamp is weird */
        if (sCurrentTimestamp->nIndex != nFrameIndex) {
            Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] Timestamp is not same in spite of I-Frame", pExynosComponent, __FUNCTION__);

            /* trust a tag index returned from D/D */
            sCurrentTimestamp->timeStamp = pExynosComponent->timeStamp[nFrameIndex];
            sCurrentTimestamp->nFlags    = pExynosComponent->nFlags[nFrameIndex];
            sCurrentTimestamp->nIndex    = nFrameIndex;

            /* delete past timestamps */
            for(i = 0; i < MAX_TIMESTAMP; i++) {
                if ((pExynosComponent->bTimestampSlotUsed[i] == OMX_TRUE) &&
                    ((sCurrentTimestamp->timeStamp > pExynosComponent->timeStamp[i]) &&
                        ((pExynosComponent->nFlags[i] & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS))) {
                    pExynosComponent->nFlags[i]             = 0x00;
                    pExynosComponent->bTimestampSlotUsed[i] = OMX_FALSE;
                }

                if ((pExynosComponent->bTimestampSlotUsed[i] == OMX_FALSE) &&
                    (sCurrentTimestamp->timeStamp < pExynosComponent->timeStamp[i])) {
                    pExynosComponent->bTimestampSlotUsed[i] = OMX_TRUE;
                    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] revive an old timestamp index for I-frame sync", pExynosComponent, __FUNCTION__);
                }
            }
        }

        if (sCurrentTimestamp->timeStamp == DEFAULT_TIMESTAMP_VAL)
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "the index of frame(%d) about I-frame is wrong", nFrameIndex);

        sCurrentTimestamp->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
    }

    if (eFrameType & VIDEO_FRAME_CORRUPT)
        sCurrentTimestamp->nFlags |= OMX_BUFFERFLAG_DATACORRUPT;

    if (sCurrentTimestamp->timeStamp != DEFAULT_TIMESTAMP_VAL) {
        if (pExynosOutputPort->latestTimeStamp <= sCurrentTimestamp->timeStamp) {
            pExynosOutputPort->latestTimeStamp = sCurrentTimestamp->timeStamp;
        } else {
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "sCurrentTimestamp(%lld) is smaller than latestTimeStamp(%lld), uses latestTimeStamp",
                                sCurrentTimestamp->timeStamp, pExynosOutputPort->latestTimeStamp);
            sCurrentTimestamp->timeStamp = pExynosOutputPort->latestTimeStamp;
        }
    } else {
        Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "uses latestTimeStamp(%lld)", pExynosOutputPort->latestTimeStamp);
        sCurrentTimestamp->timeStamp = pExynosOutputPort->latestTimeStamp;
    }

    FunctionOut();
    return;
}

OMX_BOOL Exynos_Check_BufferProcess_State(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nPortIndex)
{
    OMX_BOOL ret = OMX_FALSE;

    if ((pExynosComponent == NULL) ||
        (pExynosComponent->pExynosPort == NULL))
        return OMX_FALSE;

    if ((pExynosComponent->currentState == OMX_StateExecuting) &&
        (pExynosComponent->pExynosPort[nPortIndex].portState == OMX_StateIdle) &&
        (pExynosComponent->transientState != EXYNOS_OMX_TransStateExecutingToIdle) &&
        (pExynosComponent->transientState != EXYNOS_OMX_TransStateIdleToExecuting)) {
        ret = OMX_TRUE;
    } else {
        ret = OMX_FALSE;
    }

    return ret;
}

OMX_ERRORTYPE Exynos_ResetAllPortConfig(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT           *pInputPort        = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pOutputPort       = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    /* Input port */
    pInputPort->portDefinition.format.video.nFrameWidth             = DEFAULT_FRAME_WIDTH;
    pInputPort->portDefinition.format.video.nFrameHeight            = DEFAULT_FRAME_HEIGHT;
    pInputPort->portDefinition.format.video.nStride                 = 0; /*DEFAULT_FRAME_WIDTH;*/
    pInputPort->portDefinition.format.video.nSliceHeight            = 0;
    pInputPort->portDefinition.format.video.pNativeRender           = 0;
    pInputPort->portDefinition.format.video.bFlagErrorConcealment   = OMX_FALSE;
    pInputPort->portDefinition.format.video.eColorFormat            = OMX_COLOR_FormatUnused;

    pInputPort->portDefinition.nBufferSize  = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pInputPort->portDefinition.bEnabled     = OMX_TRUE;

    pInputPort->bufferProcessType   = BUFFER_SHARE;
    pInputPort->portWayType         = WAY2_PORT;
    Exynos_SetPlaneToPort(pInputPort, MFC_DEFAULT_INPUT_BUFFER_PLANE);

    /* Output port */
    pOutputPort->portDefinition.format.video.nFrameWidth            = DEFAULT_FRAME_WIDTH;
    pOutputPort->portDefinition.format.video.nFrameHeight           = DEFAULT_FRAME_HEIGHT;
    pOutputPort->portDefinition.format.video.nStride                = 0; /*DEFAULT_FRAME_WIDTH;*/
    pOutputPort->portDefinition.format.video.nSliceHeight           = 0;
    pOutputPort->portDefinition.format.video.pNativeRender          = 0;
    pOutputPort->portDefinition.format.video.bFlagErrorConcealment  = OMX_FALSE;
    pOutputPort->portDefinition.format.video.eColorFormat           = OMX_COLOR_FormatYUV420Planar;

    pOutputPort->portDefinition.nBufferCountActual  = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pOutputPort->portDefinition.nBufferCountMin     = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pOutputPort->portDefinition.nBufferSize  = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pOutputPort->portDefinition.bEnabled     = OMX_TRUE;

    pOutputPort->bufferProcessType  = BUFFER_COPY | BUFFER_ANBSHARE;
#ifdef USE_ANB
    pOutputPort->bIsANBEnabled      = OMX_FALSE;
#ifdef USE_STOREMETADATA
    pOutputPort->bStoreMetaData      = OMX_FALSE;
#endif
#endif
    pOutputPort->portWayType        = WAY2_PORT;
    pOutputPort->latestTimeStamp    = DEFAULT_TIMESTAMP_VAL;
    Exynos_SetPlaneToPort(pOutputPort, Exynos_OSAL_GetPlaneCount(OMX_COLOR_FormatYUV420Planar, pOutputPort->ePlaneType));

    pOutputPort->cropRectangle.nTop     = 0;
    pOutputPort->cropRectangle.nLeft    = 0;
    pOutputPort->cropRectangle.nWidth   = DEFAULT_FRAME_WIDTH;
    pOutputPort->cropRectangle.nHeight  = DEFAULT_FRAME_HEIGHT;

    return ret;
}

OMX_ERRORTYPE Exynos_CodecBufferToData(
    CODEC_DEC_BUFFER    *pCodecBuffer,
    EXYNOS_OMX_DATA     *pData,
    OMX_U32              nPortIndex)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    int i;

    if (nPortIndex > OUTPUT_PORT_INDEX) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    pData->allocSize     = 0;
    pData->usedDataLen   = 0;
    pData->nFlags        = 0;
    pData->timeStamp     = 0;
    pData->pPrivate      = pCodecBuffer;
    pData->bufferHeader  = NULL;

    for (i = 0; i < MAX_BUFFER_PLANE; i++) {
        pData->multiPlaneBuffer.dataBuffer[i] = pCodecBuffer->pVirAddr[i];
        pData->multiPlaneBuffer.fd[i] = pCodecBuffer->fd[i];
        pData->allocSize += pCodecBuffer->bufferSize[i];
    }

    if (nPortIndex == INPUT_PORT_INDEX) {
        pData->dataLen       = pCodecBuffer->dataSize;
        pData->remainDataLen = pCodecBuffer->dataSize;
    } else {
        pData->dataLen       = 0;
        pData->remainDataLen = 0;
    }

EXIT:
    return ret;
}

void Exynos_Wait_ProcessPause(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nPortIndex)
{
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT *exynosOMXInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT *exynosOMXOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT *exynosOMXPort = NULL;

    FunctionIn();

    exynosOMXPort = &pExynosComponent->pExynosPort[nPortIndex];

    if (((pExynosComponent->currentState == OMX_StatePause) ||
        (pExynosComponent->currentState == OMX_StateIdle) ||
        (pExynosComponent->transientState == EXYNOS_OMX_TransStateLoadedToIdle) ||
        (pExynosComponent->transientState == EXYNOS_OMX_TransStateExecutingToIdle)) &&
        (pExynosComponent->transientState != EXYNOS_OMX_TransStateIdleToLoaded) &&
        (!CHECK_PORT_BEING_FLUSHED(exynosOMXPort))) {
        Exynos_OSAL_SignalWait(pExynosComponent->pExynosPort[nPortIndex].pauseEvent, DEF_MAX_WAIT_TIME);
        if (pVideoDec->bExitBufferProcessThread)
            goto EXIT;
        Exynos_OSAL_SignalReset(pExynosComponent->pExynosPort[nPortIndex].pauseEvent);
    }

EXIT:
    FunctionOut();

    return;
}

OMX_BOOL Exynos_CSC_OutputData(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *dstOutputData)
{
    OMX_BOOL                       ret              = OMX_FALSE;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec        = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT           *exynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER         *outputUseBuffer  = &exynosOutputPort->way.port2WayDataBuffer.outputDataBuffer;
    OMX_U32                        copySize         = 0;
    DECODE_CODEC_EXTRA_BUFFERINFO *pBufferInfo      = NULL;
    OMX_COLOR_FORMATTYPE           eColorFormat     = exynosOutputPort->portDefinition.format.video.eColorFormat;

    FunctionIn();

    void *pOutputBuf                = (void *)outputUseBuffer->bufferHeader->pBuffer;
    void *pSrcBuf[MAX_BUFFER_PLANE] = {NULL, };
    void *pYUVBuf[MAX_BUFFER_PLANE] = {NULL, };

    ExynosVideoPlane planes[MAX_BUFFER_PLANE];

    unsigned int nAllocLen[MAX_BUFFER_PLANE] = {0, 0, 0};
    unsigned int nDataLen[MAX_BUFFER_PLANE]  = {0, 0, 0};

    OMX_U32 nFrameWidth = 0, nFrameHeight = 0;
    OMX_U32 nImageWidth = 0, nImageHeight = 0, stride = 0;

    CSC_MEMTYPE     csc_memType = CSC_MEMORY_USERPTR;
    CSC_METHOD      csc_method  = CSC_METHOD_SW;
    CSC_ERRORCODE   cscRet      = CSC_ErrorNone;
    unsigned int srcCacheable = 1, dstCacheable = 1;

    pBufferInfo  = (DECODE_CODEC_EXTRA_BUFFERINFO *)dstOutputData->extInfo;

    nFrameWidth  = pBufferInfo->imageStride;
    nFrameHeight = exynosOutputPort->portDefinition.format.video.nSliceHeight;

    /* If crop info is used, width & height will be info about frame */
    nImageWidth  = pBufferInfo->imageWidth;
    nImageHeight = pBufferInfo->imageHeight;

    pSrcBuf[0] = dstOutputData->multiPlaneBuffer.dataBuffer[0];
    pSrcBuf[1] = dstOutputData->multiPlaneBuffer.dataBuffer[1];
    pSrcBuf[2] = dstOutputData->multiPlaneBuffer.dataBuffer[2];

    if (exynosOutputPort->ePlaneType == PLANE_SINGLE) {  /* from H/W. only Y addr is valid */
        /* get a count of color plane */
        int nPlaneCnt = Exynos_OSAL_GetPlaneCount(pBufferInfo->ColorFormat, PLANE_MULTIPLE);

        if (nPlaneCnt == 2) {  /* Semi-Planar : interleaved */
            pSrcBuf[1] = (void *)(((char *)pSrcBuf[0]) +
                         ((pVideoDec->b10bitData == OMX_TRUE)? GET_10B_UV_OFFSET(nImageWidth, nImageHeight):GET_UV_OFFSET(nImageWidth, nImageHeight)));
        } else if (nPlaneCnt == 3) {  /* Planar */
            pSrcBuf[1] = (void *)(((char *)pSrcBuf[0]) +
                         ((pVideoDec->b10bitData == OMX_TRUE)? GET_10B_CB_OFFSET(nImageWidth, nImageHeight):GET_CB_OFFSET(nImageWidth, nImageHeight)));
            pSrcBuf[2] = (void *)(((char *)pSrcBuf[0]) +
                         ((pVideoDec->b10bitData == OMX_TRUE)? GET_10B_CR_OFFSET(nImageWidth, nImageHeight):GET_CR_OFFSET(nImageWidth, nImageHeight)));
        }
    }

    /* calculate each plane info for the application */
    Exynos_OSAL_GetPlaneSize(eColorFormat, PLANE_MULTIPLE, nImageWidth, nImageHeight, nDataLen, nAllocLen);

    pYUVBuf[0]  = (void *)((char *)pOutputBuf);
    pYUVBuf[1]  = (void *)((char *)pOutputBuf + nDataLen[0]);
    pYUVBuf[2]  = (void *)((char *)pOutputBuf + nDataLen[0] + nDataLen[1]);

#ifdef USE_ANB
    if (exynosOutputPort->bIsANBEnabled == OMX_TRUE) {
        if (OMX_ErrorNone != Exynos_OSAL_LockANBHandle(pOutputBuf, nImageWidth, nImageHeight, eColorFormat, &stride, planes)) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OSAL_LockANBHandle() failed", __FUNCTION__);
            ret = OMX_FALSE;
            goto EXIT;
        }

        nImageWidth = stride;
        outputUseBuffer->dataLen = sizeof(void *);

        pYUVBuf[0]  = (void *)planes[0].addr;
        pYUVBuf[1]  = (void *)planes[1].addr;
        pYUVBuf[2]  = (void *)planes[2].addr;
    }
#ifdef USE_STOREMETADATA
    else if (exynosOutputPort->bStoreMetaData == OMX_TRUE) {
        if (OMX_ErrorNone != Exynos_OSAL_LockMetaData(pOutputBuf, nImageWidth, nImageHeight, eColorFormat, &stride, planes)) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: Exynos_OSAL_LockMetaData() failed", __FUNCTION__);
            ret = OMX_FALSE;
            goto EXIT;
        }

        nImageWidth = stride;
        outputUseBuffer->dataLen = sizeof(void *);

        pYUVBuf[0]  = (void *)planes[0].addr;
        pYUVBuf[1]  = (void *)planes[1].addr;
        pYUVBuf[2]  = (void *)planes[2].addr;
    }
#endif
#endif

    if (pVideoDec->exynos_codec_checkFormatSupport(pExynosComponent, eColorFormat) == OMX_TRUE) {
        csc_memType = CSC_MEMORY_MFC;  /* to remove stride value */
        if (pVideoDec->csc_set_format == OMX_FALSE) {
            csc_set_method(pVideoDec->csc_handle, CSC_METHOD_SW);
            csc_set_src_format(
                pVideoDec->csc_handle,                                              /* handle */
                nFrameWidth,                                                        /* width */
                nFrameHeight,                                                       /* height */
                0,                                                                  /* crop_left */
                0,                                                                  /* crop_right */
                nImageWidth,                                                        /* crop_width */
                nImageHeight,                                                       /* crop_height */
                Exynos_OSAL_OMX2HALPixelFormat(
                        pBufferInfo->ColorFormat, exynosOutputPort->ePlaneType),    /* color_format */
                srcCacheable);                                                      /* cacheable */
        }
    } else {
        csc_get_method(pVideoDec->csc_handle, &csc_method);
        if (csc_method == CSC_METHOD_HW) {
            srcCacheable = 0;
            dstCacheable = 0;
        }

#ifdef USE_DMA_BUF
        if (csc_method == CSC_METHOD_HW) {
            csc_memType = CSC_MEMORY_DMABUF;

            pSrcBuf[0] = INT_TO_PTR(dstOutputData->multiPlaneBuffer.fd[0]);
            pSrcBuf[1] = INT_TO_PTR(dstOutputData->multiPlaneBuffer.fd[1]);
            pSrcBuf[2] = INT_TO_PTR(dstOutputData->multiPlaneBuffer.fd[2]);

#ifdef USE_ANB
            if ((exynosOutputPort->bIsANBEnabled == OMX_TRUE) ||
                (exynosOutputPort->bStoreMetaData == OMX_TRUE)) {
                pYUVBuf[0]  = INT_TO_PTR(planes[0].fd);
                pYUVBuf[1]  = INT_TO_PTR(planes[1].fd);
                pYUVBuf[2]  = INT_TO_PTR(planes[2].fd);
            } else {
                pYUVBuf[0] = INT_TO_PTR(Exynos_OSAL_SharedMemory_VirtToION(pVideoDec->hSharedMemory, pOutputBuf));
                pYUVBuf[1] = NULL;
                pYUVBuf[2] = NULL;
            }
#else
            pYUVBuf[0] = INT_TO_PTR(Exynos_OSAL_SharedMemory_VirtToION(pVideoDec->hSharedMemory, pOutputBuf));
            pYUVBuf[1] = NULL;
            pYUVBuf[2] = NULL;
#endif
        }
#endif

        if (pVideoDec->csc_set_format == OMX_FALSE) {
            csc_set_src_format(
                pVideoDec->csc_handle,                                              /* handle */
                nFrameWidth,                                                        /* width */
                nFrameHeight,                                                       /* height */
                0,                                                                  /* crop_left */
                0,                                                                  /* crop_right */
                nImageWidth,                                                        /* crop_width */
                nImageHeight,                                                       /* crop_height */
                Exynos_OSAL_OMX2HALPixelFormat(
                        pBufferInfo->ColorFormat, exynosOutputPort->ePlaneType),    /* color_format */
                srcCacheable);                                                      /* cacheable */
        }
    }

    if (pVideoDec->csc_set_format == OMX_FALSE) {
        unsigned int pixelFormat = Exynos_OSAL_OMX2HALPixelFormat(eColorFormat, PLANE_SINGLE_USER);

        if ((IS_CUSTOM_COMPONENT(pExynosComponent->componentName) == OMX_TRUE) &&
            (nFrameWidth != nImageWidth) &&
            (csc_method == CSC_METHOD_HW)) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "H/W CSC isn't supported by the constraint (w:%d/h:%d)", nImageWidth, nImageHeight);
            ret = OMX_FALSE;
            goto EXIT;
        }

#ifdef USE_ANB
        if (exynosOutputPort->bIsANBEnabled == OMX_TRUE)
            pixelFormat = Exynos_OSAL_OMX2HALPixelFormat(eColorFormat, exynosOutputPort->ePlaneType);
#ifdef USE_STOREMETADATA
        else if (exynosOutputPort->bStoreMetaData == OMX_TRUE)
            pixelFormat = Exynos_OSAL_OMX2HALPixelFormat(eColorFormat, exynosOutputPort->ePlaneType);
#endif
#endif

        csc_set_dst_format(
            pVideoDec->csc_handle,                              /* handle */
            nImageWidth,                                        /* width */
            nImageHeight,                                       /* height */
            0,                                                  /* crop_left */
            0,                                                  /* crop_right */
            nImageWidth,                                        /* crop_width */
            nImageHeight,                                       /* crop_height */
            pixelFormat,                                        /* color_format */
            dstCacheable);                                      /* cacheable */

        csc_set_eq_property(
            pVideoDec->csc_handle,          /* handle */
            CSC_EQ_MODE_USER,               /* user select */
            CSC_EQ_RANGE_NARROW,            /* narrow */
            CSC_EQ_COLORSPACE_SMPTE170M);   /* bt.601 */

        pVideoDec->csc_set_format = OMX_TRUE;
    }

    csc_set_src_buffer(
        pVideoDec->csc_handle,  /* handle */
        pSrcBuf,
        csc_memType);           /* YUV Addr or FD */
    csc_set_dst_buffer(
        pVideoDec->csc_handle,  /* handle */
        pYUVBuf,
        csc_memType);           /* YUV Addr or FD */
    cscRet = csc_convert(pVideoDec->csc_handle);
    if (cscRet != CSC_ErrorNone)
        ret = OMX_FALSE;
    else
        ret = OMX_TRUE;

#ifdef USE_ANB
    if (exynosOutputPort->bIsANBEnabled == OMX_TRUE)
        Exynos_OSAL_UnlockANBHandle(pOutputBuf);
#ifdef USE_STOREMETADATA
    else if (exynosOutputPort->bStoreMetaData == OMX_TRUE)
        Exynos_OSAL_UnlockMetaData(pOutputBuf);
#endif
#endif

EXIT:
    FunctionOut();

    return ret;
}

OMX_BOOL Exynos_Preprocessor_InputData(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *srcInputData)
{
    OMX_BOOL               ret = OMX_FALSE;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *exynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *inputUseBuffer = &exynosInputPort->way.port2WayDataBuffer.inputDataBuffer;
    OMX_U32                copySize = 0;
    OMX_BYTE               checkInputStream = NULL;
    OMX_U32                checkInputStreamLen = 0;

    FunctionIn();

    if (exynosInputPort->bufferProcessType & BUFFER_COPY) {
        if ((srcInputData->multiPlaneBuffer.dataBuffer[0] == NULL) ||
            (srcInputData->pPrivate == NULL)) {
            ret = OMX_FALSE;
            goto EXIT;
        }
    }

    if (inputUseBuffer->dataValid == OMX_TRUE) {
        if (exynosInputPort->bufferProcessType & BUFFER_SHARE) {
            Exynos_Shared_BufferToData(inputUseBuffer, srcInputData, ONE_PLANE);

            if (pExynosComponent->codecType == HW_VIDEO_DEC_SECURE_CODEC) {
                int ionFD = -1;
                OMX_PTR dataBuffer = NULL;

                /* caution : data loss */
                ionFD = PTR_TO_INT(srcInputData->multiPlaneBuffer.dataBuffer[0]);

                dataBuffer = Exynos_OSAL_SharedMemory_IONToVirt(pVideoDec->hSharedMemory, ionFD);
                if (dataBuffer == NULL) {
                    ret = OMX_FALSE;
                    goto EXIT;
                }

                srcInputData->multiPlaneBuffer.fd[0]         = ionFD;
                srcInputData->multiPlaneBuffer.dataBuffer[0] = dataBuffer;
            } else {
                srcInputData->multiPlaneBuffer.fd[0] =
                    Exynos_OSAL_SharedMemory_VirtToION(pVideoDec->hSharedMemory,
                                srcInputData->multiPlaneBuffer.dataBuffer[0]);
            }

            /* reset dataBuffer */
            Exynos_ResetDataBuffer(inputUseBuffer);
        } else if (exynosInputPort->bufferProcessType & BUFFER_COPY) {
            checkInputStream = inputUseBuffer->bufferHeader->pBuffer + inputUseBuffer->usedDataLen;
            checkInputStreamLen = inputUseBuffer->remainDataLen;

            pExynosComponent->bUseFlagEOF = OMX_TRUE;

            copySize = checkInputStreamLen;

            if (((srcInputData->allocSize) - (srcInputData->dataLen)) >= copySize) {
                if (copySize > 0) {
                    Exynos_OSAL_Memcpy((OMX_PTR)((char *)srcInputData->multiPlaneBuffer.dataBuffer[0] + srcInputData->dataLen),
                                       checkInputStream, copySize);
                }

                inputUseBuffer->dataLen -= copySize;
                inputUseBuffer->remainDataLen -= copySize;
                inputUseBuffer->usedDataLen += copySize;

                srcInputData->dataLen += copySize;
                srcInputData->remainDataLen += copySize;

                srcInputData->timeStamp = inputUseBuffer->timeStamp;
                srcInputData->nFlags = inputUseBuffer->nFlags;
                srcInputData->bufferHeader = inputUseBuffer->bufferHeader;
            } else {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "input codec buffer is smaller than decoded input data size Out Length");
                pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                        pExynosComponent->callbackData,
                                                        OMX_EventError, OMX_ErrorUndefined, 0, NULL);
                ret = OMX_FALSE;
            }

            Exynos_InputBufferReturn(pOMXComponent, inputUseBuffer);
        }

        if ((srcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) {
            Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] bSaveFlagEOS : OMX_TRUE", pExynosComponent, __FUNCTION__);
            if (srcInputData->dataLen != 0)
                pExynosComponent->bBehaviorEOS = OMX_TRUE;
        }

        if ((pExynosComponent->checkTimeStamp.needSetStartTimeStamp == OMX_TRUE) &&
            ((srcInputData->nFlags & OMX_BUFFERFLAG_CODECCONFIG) != OMX_BUFFERFLAG_CODECCONFIG)) {
            EXYNOS_OMX_BASEPORT *pOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

            pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_TRUE;
            pExynosComponent->checkTimeStamp.startTimeStamp = srcInputData->timeStamp;
            pOutputPort->latestTimeStamp = srcInputData->timeStamp;  // for reordering timestamp mode
            pExynosComponent->checkTimeStamp.nStartFlags = srcInputData->nFlags;
            pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
            Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] first frame timestamp after seeking %lld us (%.2f secs)",
                            pExynosComponent, __FUNCTION__, srcInputData->timeStamp, srcInputData->timeStamp / 1E6);
        }

        ret = OMX_TRUE;
    }

EXIT:

    FunctionOut();

    return ret;
}

OMX_BOOL Exynos_Postprocess_OutputData(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *dstOutputData)
{
    OMX_BOOL                   ret = OMX_FALSE;
    EXYNOS_OMX_BASECOMPONENT  *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT       *exynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER     *outputUseBuffer = &exynosOutputPort->way.port2WayDataBuffer.outputDataBuffer;
    OMX_U32                    copySize = 0;
    DECODE_CODEC_EXTRA_BUFFERINFO *pBufferInfo = NULL;

    FunctionIn();

    if (exynosOutputPort->bufferProcessType & BUFFER_SHARE) {
#ifdef USE_ANB
        if ((exynosOutputPort->bIsANBEnabled == OMX_FALSE) &&
            (exynosOutputPort->bStoreMetaData == OMX_FALSE)) {
            if (Exynos_Shared_DataToBuffer(dstOutputData, outputUseBuffer) == OMX_ErrorNone)
                outputUseBuffer->dataValid = OMX_TRUE;
        } else {
            if (Exynos_Shared_DataToANBBuffer(dstOutputData, outputUseBuffer, exynosOutputPort) == OMX_ErrorNone) {
                outputUseBuffer->dataValid = OMX_TRUE;
            } else {
                ret = OMX_FALSE;
                goto EXIT;
            }
        }
#else
        if (Exynos_Shared_DataToBuffer(dstOutputData, outputUseBuffer) == OMX_ErrorNone)
            outputUseBuffer->dataValid = OMX_TRUE;
#endif
    }

    if (outputUseBuffer->dataValid == OMX_TRUE) {
        if ((pExynosComponent->checkTimeStamp.needCheckStartTimeStamp == OMX_TRUE) &&
            ((dstOutputData->nFlags & OMX_BUFFERFLAG_EOS) != OMX_BUFFERFLAG_EOS)) {
            if ((pExynosComponent->checkTimeStamp.startTimeStamp == dstOutputData->timeStamp) &&
                (pExynosComponent->checkTimeStamp.nStartFlags ==
                 (dstOutputData->nFlags & ~(OMX_BUFFERFLAG_SYNCFRAME | OMX_BUFFERFLAG_DATACORRUPT)))) {
                pExynosComponent->checkTimeStamp.startTimeStamp = RESET_TIMESTAMP_VAL;
                pExynosComponent->checkTimeStamp.nStartFlags = 0x0;
                pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
                pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
            } else {
                if (pExynosComponent->checkTimeStamp.startTimeStamp < dstOutputData->timeStamp) {
                    pExynosComponent->checkTimeStamp.startTimeStamp = RESET_TIMESTAMP_VAL;
                    pExynosComponent->checkTimeStamp.nStartFlags = 0x0;
                    pExynosComponent->checkTimeStamp.needSetStartTimeStamp = OMX_FALSE;
                    pExynosComponent->checkTimeStamp.needCheckStartTimeStamp = OMX_FALSE;
                } else {
                    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] drop frame after seeking", pExynosComponent, __FUNCTION__);
                    if (exynosOutputPort->bufferProcessType & BUFFER_SHARE)
                        Exynos_OMX_FillThisBufferAgain(pOMXComponent, outputUseBuffer->bufferHeader);

                    ret = OMX_TRUE;
                    goto EXIT;
                }
            }
        } else if ((pExynosComponent->checkTimeStamp.needSetStartTimeStamp == OMX_TRUE)) {
            ret = OMX_TRUE;
            Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] not set check timestame after seeking", pExynosComponent, __FUNCTION__);
            goto EXIT;
        }

        if (exynosOutputPort->bufferProcessType & BUFFER_COPY) {
            if ((dstOutputData->remainDataLen <= (outputUseBuffer->allocSize - outputUseBuffer->dataLen)) &&
                (!CHECK_PORT_BEING_FLUSHED(exynosOutputPort))) {
                copySize = dstOutputData->remainDataLen;
                Exynos_OSAL_Log(EXYNOS_LOG_TRACE,"copySize: %d", copySize);

                outputUseBuffer->dataLen += copySize;
                outputUseBuffer->remainDataLen += copySize;
                outputUseBuffer->nFlags = dstOutputData->nFlags;
                outputUseBuffer->timeStamp = dstOutputData->timeStamp;

                if (outputUseBuffer->remainDataLen > 0) {
                    ret = Exynos_CSC_OutputData(pOMXComponent, dstOutputData);
                } else {
                    ret = OMX_TRUE;
                }

                if (ret == OMX_TRUE) {
                    if ((outputUseBuffer->remainDataLen > 0) ||
                        ((outputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) ||
                        (CHECK_PORT_BEING_FLUSHED(exynosOutputPort))) {
                        Exynos_OutputBufferReturn(pOMXComponent, outputUseBuffer);
                    }
                } else {
                    Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "csc_convert Error");
                    pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                            pExynosComponent->callbackData,
                                                            OMX_EventError, OMX_ErrorUndefined, 0, NULL);
                    ret = OMX_FALSE;
                }
            } else if (CHECK_PORT_BEING_FLUSHED(exynosOutputPort)) {
                outputUseBuffer->dataLen = 0;
                outputUseBuffer->remainDataLen = 0;
                outputUseBuffer->nFlags = dstOutputData->nFlags;
                outputUseBuffer->timeStamp = dstOutputData->timeStamp;
                Exynos_OutputBufferReturn(pOMXComponent, outputUseBuffer);
            } else {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "output buffer is smaller than decoded data size Out Length");
                pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                        pExynosComponent->callbackData,
                                                        OMX_EventError, OMX_ErrorUndefined, 0, NULL);
                ret = OMX_FALSE;
            }
        } else if (exynosOutputPort->bufferProcessType & BUFFER_SHARE) {
            if ((outputUseBuffer->remainDataLen > 0) ||
                ((outputUseBuffer->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) ||
                (CHECK_PORT_BEING_FLUSHED(exynosOutputPort))) {
                Exynos_OutputBufferReturn(pOMXComponent, outputUseBuffer);
            } else {
                Exynos_OMX_FillThisBufferAgain(pOMXComponent, outputUseBuffer->bufferHeader);
                Exynos_ResetDataBuffer(outputUseBuffer);
            }
        }
    } else {
        ret = OMX_FALSE;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_SrcInputBufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *exynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *exynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *srcInputUseBuffer = &exynosInputPort->way.port2WayDataBuffer.inputDataBuffer;
    EXYNOS_OMX_DATA          *pSrcInputData = &exynosInputPort->processData;
    OMX_BOOL               bCheckInputData = OMX_FALSE;
    OMX_BOOL               bValidCodecData = OMX_FALSE;

    FunctionIn();

    while (!pVideoDec->bExitBufferProcessThread) {
        Exynos_OSAL_SleepMillisec(0);
        Exynos_Wait_ProcessPause(pExynosComponent, INPUT_PORT_INDEX);
        if ((exynosInputPort->semWaitPortEnable[INPUT_WAY_INDEX] != NULL) &&
            (!CHECK_PORT_ENABLED(exynosInputPort))) {
            /* sema will be posted at PortEnable */
            Exynos_OSAL_SemaphoreWait(exynosInputPort->semWaitPortEnable[INPUT_WAY_INDEX]);
            continue;
        }

        while ((Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX)) &&
               (!pVideoDec->bExitBufferProcessThread)) {
            Exynos_OSAL_SleepMillisec(0);

            if ((CHECK_PORT_BEING_FLUSHED(exynosInputPort)) ||
                ((exynosOutputPort->exceptionFlag != GENERAL_STATE) &&
                    (((EXYNOS_OMX_ERRORTYPE)ret == OMX_ErrorInputDataDecodeYet) ||
                    ((EXYNOS_OMX_ERRORTYPE)ret == OMX_ErrorNoneSrcSetupFinish))) ||
                (exynosOutputPort->exceptionFlag == INVALID_STATE))
                break;
            if (exynosInputPort->portState != OMX_StateIdle)
                break;

            Exynos_OSAL_MutexLock(srcInputUseBuffer->bufferMutex);
            if ((EXYNOS_OMX_ERRORTYPE)ret != OMX_ErrorInputDataDecodeYet) {
                if (exynosInputPort->bufferProcessType & BUFFER_COPY) {
                    OMX_PTR codecBuffer;
                    if ((pSrcInputData->multiPlaneBuffer.dataBuffer[0] == NULL) || (pSrcInputData->pPrivate == NULL)) {
                        Exynos_CodecBufferDeQueue(pExynosComponent, INPUT_PORT_INDEX, &codecBuffer);
                        if (pVideoDec->bExitBufferProcessThread) {
                            Exynos_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
                            goto EXIT;
                        }

                        if (codecBuffer != NULL) {
                            Exynos_CodecBufferToData(codecBuffer, pSrcInputData, INPUT_PORT_INDEX);
                        }
                        Exynos_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
                        break;
                    }
                }

                if (srcInputUseBuffer->dataValid == OMX_TRUE) {
                    bCheckInputData = Exynos_Preprocessor_InputData(pOMXComponent, pSrcInputData);
                } else {
                    bCheckInputData = OMX_FALSE;
                }

                if ((bCheckInputData == OMX_FALSE) &&
                    (!CHECK_PORT_BEING_FLUSHED(exynosInputPort))) {
                    ret = Exynos_InputBufferGetQueue(pExynosComponent);
                    if (pVideoDec->bExitBufferProcessThread) {
                        Exynos_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
                        goto EXIT;
                    }

                    Exynos_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
                    break;
                }

                if (CHECK_PORT_BEING_FLUSHED(exynosInputPort)) {
                    Exynos_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
                    break;
                }
            }

            /* if input flush is occured before obtaining bufferMutex,
             * bufferHeader can be NULL.
             */
            if (pSrcInputData->bufferHeader == NULL) {
                ret = OMX_ErrorNone;
                Exynos_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
                break;
            }

            ret = pVideoDec->exynos_codec_srcInputProcess(pOMXComponent, pSrcInputData);
            if (((EXYNOS_OMX_ERRORTYPE)ret == OMX_ErrorCorruptedFrame) ||
                ((EXYNOS_OMX_ERRORTYPE)ret == OMX_ErrorCorruptedHeader)) {
                if (exynosInputPort->bufferProcessType & BUFFER_COPY) {
                    OMX_PTR codecBuffer;
                    codecBuffer = pSrcInputData->pPrivate;
                    if (codecBuffer != NULL)
                        Exynos_CodecBufferEnQueue(pExynosComponent, INPUT_PORT_INDEX, codecBuffer);
                }

                if (exynosInputPort->bufferProcessType & BUFFER_SHARE) {
                    Exynos_OMX_InputBufferReturn(pOMXComponent, pSrcInputData->bufferHeader);
                }
            }

            if ((EXYNOS_OMX_ERRORTYPE)ret != OMX_ErrorInputDataDecodeYet) {
                Exynos_ResetCodecData(pSrcInputData);
            }
            Exynos_OSAL_MutexUnlock(srcInputUseBuffer->bufferMutex);
            if ((EXYNOS_OMX_ERRORTYPE)ret == OMX_ErrorCodecInit)
                pVideoDec->bExitBufferProcessThread = OMX_TRUE;
        }
    }

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_SrcOutputBufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *exynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *srcOutputUseBuffer = &exynosInputPort->way.port2WayDataBuffer.outputDataBuffer;
    EXYNOS_OMX_DATA           srcOutputData;

    FunctionIn();

    Exynos_ResetCodecData(&srcOutputData);

    while (!pVideoDec->bExitBufferProcessThread) {
        Exynos_OSAL_SleepMillisec(0);
        if ((exynosInputPort->semWaitPortEnable[OUTPUT_WAY_INDEX] != NULL) &&
            (!CHECK_PORT_ENABLED(exynosInputPort))) {
            /* sema will be posted at PortEnable */
            Exynos_OSAL_SemaphoreWait(exynosInputPort->semWaitPortEnable[OUTPUT_WAY_INDEX]);
            continue;
        }

        while (!pVideoDec->bExitBufferProcessThread) {
            if (exynosInputPort->bufferProcessType & BUFFER_COPY) {
                if (Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX) == OMX_FALSE)
                    break;
            }
            Exynos_OSAL_SleepMillisec(0);

            if (CHECK_PORT_BEING_FLUSHED(exynosInputPort))
                break;

            Exynos_OSAL_MutexLock(srcOutputUseBuffer->bufferMutex);
            ret = pVideoDec->exynos_codec_srcOutputProcess(pOMXComponent, &srcOutputData);

            if (ret == OMX_ErrorNone) {
                if (exynosInputPort->bufferProcessType & BUFFER_COPY) {
                    OMX_PTR codecBuffer;
                    codecBuffer = srcOutputData.pPrivate;
                    if (codecBuffer != NULL)
                        Exynos_CodecBufferEnQueue(pExynosComponent, INPUT_PORT_INDEX, codecBuffer);
                }
                if (exynosInputPort->bufferProcessType & BUFFER_SHARE) {
                    Exynos_Shared_DataToBuffer(&srcOutputData, srcOutputUseBuffer);
                    Exynos_InputBufferReturn(pOMXComponent, srcOutputUseBuffer);
                }
                Exynos_ResetCodecData(&srcOutputData);
            }
            Exynos_OSAL_MutexUnlock(srcOutputUseBuffer->bufferMutex);
        }
    }

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_DstInputBufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *exynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *dstInputUseBuffer = &exynosOutputPort->way.port2WayDataBuffer.inputDataBuffer;
    EXYNOS_OMX_DATA           dstInputData;

    FunctionIn();

    Exynos_ResetCodecData(&dstInputData);

    while (!pVideoDec->bExitBufferProcessThread) {
        Exynos_OSAL_SleepMillisec(0);
        if ((exynosOutputPort->semWaitPortEnable[INPUT_WAY_INDEX] != NULL) &&
            (!CHECK_PORT_ENABLED(exynosOutputPort))) {
            /* sema will be posted at PortEnable */
            Exynos_OSAL_SemaphoreWait(exynosOutputPort->semWaitPortEnable[INPUT_WAY_INDEX]);
            continue;
        }

        while ((Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) &&
               (!pVideoDec->bExitBufferProcessThread)) {
            Exynos_OSAL_SleepMillisec(0);

            if ((CHECK_PORT_BEING_FLUSHED(exynosOutputPort)) ||
                (!CHECK_PORT_POPULATED(exynosOutputPort)) ||
                (exynosOutputPort->exceptionFlag != GENERAL_STATE))
                break;
            if (exynosOutputPort->portState != OMX_StateIdle)
                break;

            Exynos_OSAL_MutexLock(dstInputUseBuffer->bufferMutex);
            if ((EXYNOS_OMX_ERRORTYPE)ret != OMX_ErrorOutputBufferUseYet) {
                if (exynosOutputPort->bufferProcessType & BUFFER_COPY) {
                    CODEC_DEC_BUFFER *pCodecBuffer = NULL;
                    ret = Exynos_CodecBufferDeQueue(pExynosComponent, OUTPUT_PORT_INDEX, (OMX_PTR *)&pCodecBuffer);
                    if (pVideoDec->bExitBufferProcessThread) {
                        Exynos_OSAL_MutexUnlock(dstInputUseBuffer->bufferMutex);
                        goto EXIT;
                    }

                    if (ret != OMX_ErrorNone) {
                        Exynos_OSAL_MutexUnlock(dstInputUseBuffer->bufferMutex);
                        break;
                    }
                    Exynos_CodecBufferToData(pCodecBuffer, &dstInputData, OUTPUT_PORT_INDEX);
                }

                if (exynosOutputPort->bufferProcessType & BUFFER_SHARE) {
                    if ((dstInputUseBuffer->dataValid != OMX_TRUE) &&
                        (!CHECK_PORT_BEING_FLUSHED(exynosOutputPort))) {
                        ret = Exynos_OutputBufferGetQueue(pExynosComponent);
                        if (pVideoDec->bExitBufferProcessThread) {
                            Exynos_OSAL_MutexUnlock(dstInputUseBuffer->bufferMutex);
                            goto EXIT;
                        }

                        if (ret != OMX_ErrorNone) {
                            Exynos_OSAL_MutexUnlock(dstInputUseBuffer->bufferMutex);
                            break;
                        }
#ifdef USE_ANB
                        if ((exynosOutputPort->bIsANBEnabled == OMX_FALSE) &&
                            (exynosOutputPort->bStoreMetaData == OMX_FALSE)) {
                            Exynos_Shared_BufferToData(dstInputUseBuffer, &dstInputData, TWO_PLANE);
                        } else {
                            ret = Exynos_Shared_ANBBufferToData(dstInputUseBuffer, &dstInputData, exynosOutputPort);
                            if (ret != OMX_ErrorNone) {
                                dstInputUseBuffer->dataValid = OMX_FALSE;
                                Exynos_OSAL_MutexUnlock(dstInputUseBuffer->bufferMutex);
                                break;
                            }

                            OMX_PTR pANBHandle = NULL;
#ifdef USE_STOREMETADATA
                            if (exynosOutputPort->bStoreMetaData == OMX_TRUE) {
                                OMX_PTR ppBuf[MAX_BUFFER_PLANE] = {NULL, NULL, NULL};

                                ret = Exynos_OSAL_GetInfoFromMetaData(dstInputData.bufferHeader->pBuffer, ppBuf);
                                if (ret != OMX_ErrorNone) {
                                    /* actually, the error will be happen at ANBBufferToData, if metadata is invalid. */
                                    dstInputUseBuffer->dataValid = OMX_FALSE;
                                    Exynos_OSAL_MutexUnlock(dstInputUseBuffer->bufferMutex);
                                    break;
                                }

                                pANBHandle = ppBuf[0];
                            } else
#endif
                            {
                                pANBHandle = dstInputData.bufferHeader->pBuffer;
                            }

                            Exynos_OSAL_RefANB_Increase(pVideoDec->hRefHandle, pANBHandle, exynosOutputPort->ePlaneType);
                        }
#else
                        Exynos_Shared_BufferToData(dstInputUseBuffer, &dstInputData, TWO_PLANE);
#endif
                        Exynos_ResetDataBuffer(dstInputUseBuffer);
                    }
                }

                if (CHECK_PORT_BEING_FLUSHED(exynosOutputPort)) {
                    Exynos_OSAL_MutexUnlock(dstInputUseBuffer->bufferMutex);
                    break;
                }
            }

            ret = pVideoDec->exynos_codec_dstInputProcess(pOMXComponent, &dstInputData);
            if ((EXYNOS_OMX_ERRORTYPE)ret != OMX_ErrorOutputBufferUseYet) {
                Exynos_ResetCodecData(&dstInputData);
            }
            Exynos_OSAL_MutexUnlock(dstInputUseBuffer->bufferMutex);
        }
    }

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_DstOutputBufferProcess(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT      *exynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_OMX_DATABUFFER    *dstOutputUseBuffer = &exynosOutputPort->way.port2WayDataBuffer.outputDataBuffer;
    EXYNOS_OMX_DATA          *pDstOutputData = &exynosOutputPort->processData;

    FunctionIn();

    while (!pVideoDec->bExitBufferProcessThread) {
        Exynos_OSAL_SleepMillisec(0);
        Exynos_Wait_ProcessPause(pExynosComponent, OUTPUT_PORT_INDEX);
        if ((exynosOutputPort->semWaitPortEnable[OUTPUT_WAY_INDEX] != NULL) &&
            (!CHECK_PORT_ENABLED(exynosOutputPort))) {
            /* sema will be posted at PortEnable */
            Exynos_OSAL_SemaphoreWait(exynosOutputPort->semWaitPortEnable[OUTPUT_WAY_INDEX]);
            continue;
        }

        while ((Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) &&
               (!pVideoDec->bExitBufferProcessThread)) {
            Exynos_OSAL_SleepMillisec(0);

            if (CHECK_PORT_BEING_FLUSHED(exynosOutputPort))
                break;

            Exynos_OSAL_MutexLock(dstOutputUseBuffer->bufferMutex);
            if (exynosOutputPort->bufferProcessType & BUFFER_COPY) {
                if ((dstOutputUseBuffer->dataValid != OMX_TRUE) &&
                    (!CHECK_PORT_BEING_FLUSHED(exynosOutputPort))) {
                    ret = Exynos_OutputBufferGetQueue(pExynosComponent);
                    if (pVideoDec->bExitBufferProcessThread) {
                        Exynos_OSAL_MutexUnlock(dstOutputUseBuffer->bufferMutex);
                        goto EXIT;
                    }

                    if (ret != OMX_ErrorNone) {
                        Exynos_OSAL_MutexUnlock(dstOutputUseBuffer->bufferMutex);
                        break;
                    }
                }
            }

            if ((dstOutputUseBuffer->dataValid == OMX_TRUE) ||
                (exynosOutputPort->bufferProcessType & BUFFER_SHARE))
                ret = pVideoDec->exynos_codec_dstOutputProcess(pOMXComponent, pDstOutputData);

            if (((ret == OMX_ErrorNone) && (dstOutputUseBuffer->dataValid == OMX_TRUE)) ||
                (exynosOutputPort->bufferProcessType & BUFFER_SHARE)) {
#ifdef USE_ANB
                if (exynosOutputPort->bufferProcessType == BUFFER_SHARE) {
                    DECODE_CODEC_EXTRA_BUFFERINFO *pBufferInfo = NULL;
                    int i;

                    pBufferInfo = (DECODE_CODEC_EXTRA_BUFFERINFO *)pDstOutputData->extInfo;
                    for (i = 0; i < VIDEO_BUFFER_MAX_NUM; i++) {
                        if (pBufferInfo->PDSB.dpbFD[i].fd > -1) {
                            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "decRefCnt-FD:%d", pBufferInfo->PDSB.dpbFD[i].fd);
                            Exynos_OSAL_RefANB_Decrease(pVideoDec->hRefHandle, pBufferInfo->PDSB.dpbFD[i].fd);
                        } else {
                            break;
                        }
                    }
                }
#endif
                Exynos_Postprocess_OutputData(pOMXComponent, pDstOutputData);
            }

            if (exynosOutputPort->bufferProcessType & BUFFER_COPY) {
                if (pDstOutputData->pPrivate != NULL) {
                    Exynos_CodecBufferEnQueue(pExynosComponent, OUTPUT_PORT_INDEX, pDstOutputData->pPrivate);
                    pDstOutputData->pPrivate = NULL;
                }
            }

            /* reset outputData */
            Exynos_ResetCodecData(pDstOutputData);
            Exynos_OSAL_MutexUnlock(dstOutputUseBuffer->bufferMutex);
        }
    }

EXIT:

    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_OMX_SrcInputProcessThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    Exynos_OMX_SrcInputBufferProcess(pOMXComponent);

    Exynos_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_OMX_SrcOutputProcessThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    Exynos_OMX_SrcOutputBufferProcess(pOMXComponent);

    Exynos_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_OMX_DstInputProcessThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    Exynos_OMX_DstInputBufferProcess(pOMXComponent);

    Exynos_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

static OMX_ERRORTYPE Exynos_OMX_DstOutputProcessThread(OMX_PTR threadData)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_MESSAGE       *message = NULL;

    FunctionIn();

    if (threadData == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)threadData;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    Exynos_OMX_DstOutputBufferProcess(pOMXComponent);

    Exynos_OSAL_ThreadExit(NULL);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BufferProcess_Create(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;

    FunctionIn();

    pVideoDec->bExitBufferProcessThread = OMX_FALSE;

    ret = Exynos_OSAL_ThreadCreate(&pVideoDec->hDstOutputThread,
                 Exynos_OMX_DstOutputProcessThread,
                 pOMXComponent);
    if (ret == OMX_ErrorNone)
        ret = Exynos_OSAL_ThreadCreate(&pVideoDec->hSrcOutputThread,
                     Exynos_OMX_SrcOutputProcessThread,
                     pOMXComponent);
    if (ret == OMX_ErrorNone)
        ret = Exynos_OSAL_ThreadCreate(&pVideoDec->hDstInputThread,
                     Exynos_OMX_DstInputProcessThread,
                     pOMXComponent);
    if (ret == OMX_ErrorNone)
        ret = Exynos_OSAL_ThreadCreate(&pVideoDec->hSrcInputThread,
                     Exynos_OMX_SrcInputProcessThread,
                     pOMXComponent);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_BufferProcess_Terminate(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    OMX_S32                countValue = 0;
    unsigned int           i = 0;

    FunctionIn();

    pVideoDec->bExitBufferProcessThread = OMX_TRUE;

    Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].bufferSemID, &countValue);
    if (countValue == 0)
        Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].bufferSemID);
    Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].codecSemID, &countValue);
    if (countValue == 0)
        Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].codecSemID);

    /* srcInput */
    Exynos_OSAL_SignalSet(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].pauseEvent);
    Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].semWaitPortEnable[INPUT_WAY_INDEX]);
    Exynos_OSAL_ThreadTerminate(pVideoDec->hSrcInputThread);
    pVideoDec->hSrcInputThread = NULL;
    Exynos_OSAL_Set_SemaphoreCount(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].semWaitPortEnable[INPUT_WAY_INDEX], 0);

    Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].bufferSemID, &countValue);
    if (countValue == 0)
        Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].bufferSemID);
    Exynos_OSAL_Get_SemaphoreCount(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].codecSemID, &countValue);
    if (countValue == 0)
        Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].codecSemID);
    pVideoDec->exynos_codec_bufferProcessRun(pOMXComponent, OUTPUT_PORT_INDEX);

    /* dstInput */
    Exynos_OSAL_SignalSet(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].pauseEvent);
    Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].semWaitPortEnable[INPUT_WAY_INDEX]);
    Exynos_OSAL_ThreadTerminate(pVideoDec->hDstInputThread);
    pVideoDec->hDstInputThread = NULL;
    Exynos_OSAL_Set_SemaphoreCount(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].semWaitPortEnable[INPUT_WAY_INDEX], 0);

    pVideoDec->exynos_codec_stop(pOMXComponent, INPUT_PORT_INDEX);
    pVideoDec->exynos_codec_bufferProcessRun(pOMXComponent, INPUT_PORT_INDEX);

    /* srcOutput */
    Exynos_OSAL_SignalSet(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].pauseEvent);
    Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].semWaitPortEnable[OUTPUT_WAY_INDEX]);
    Exynos_OSAL_ThreadTerminate(pVideoDec->hSrcOutputThread);
    pVideoDec->hSrcOutputThread = NULL;
    Exynos_OSAL_Set_SemaphoreCount(pExynosComponent->pExynosPort[INPUT_PORT_INDEX].semWaitPortEnable[OUTPUT_WAY_INDEX], 0);

    pVideoDec->exynos_codec_stop(pOMXComponent, OUTPUT_PORT_INDEX);
    pVideoDec->exynos_codec_bufferProcessRun(pOMXComponent, OUTPUT_PORT_INDEX);

    /* dstOutput */
    Exynos_OSAL_SignalSet(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].pauseEvent);
    Exynos_OSAL_SemaphorePost(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].semWaitPortEnable[OUTPUT_WAY_INDEX]);
    Exynos_OSAL_ThreadTerminate(pVideoDec->hDstOutputThread);
    pVideoDec->hDstOutputThread = NULL;
    Exynos_OSAL_Set_SemaphoreCount(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].semWaitPortEnable[OUTPUT_WAY_INDEX], 0);

    pExynosComponent->checkTimeStamp.needSetStartTimeStamp      = OMX_FALSE;
    pExynosComponent->checkTimeStamp.needCheckStartTimeStamp    = OMX_FALSE;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_VideoDecodeComponentInit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = Exynos_OMX_BaseComponent_Constructor(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    ret = Exynos_OMX_Port_Constructor(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OMX_BaseComponent_Destructor(pOMXComponent);
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    pVideoDec = Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_VIDEODEC_COMPONENT));
    if (pVideoDec == NULL) {
        Exynos_OMX_Port_Destructor(pOMXComponent);
        Exynos_OMX_BaseComponent_Destructor(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }

    Exynos_OSAL_Memset(pVideoDec, 0, sizeof(EXYNOS_OMX_VIDEODEC_COMPONENT));
    pVideoDec->bForceHeaderParsing  = OMX_FALSE;
    pVideoDec->bReconfigDPB         = OMX_FALSE;
    pVideoDec->bDTSMode             = OMX_FALSE;
    pVideoDec->bReorderMode         = OMX_FALSE;
    pVideoDec->b10bitData           = OMX_FALSE;
    pVideoDec->bQosChanged          = OMX_FALSE;
    pVideoDec->nQosRatio            = 0;
    pExynosComponent->hComponentHandle = (OMX_HANDLETYPE)pVideoDec;

    pExynosComponent->bSaveFlagEOS = OMX_FALSE;
    pExynosComponent->bBehaviorEOS = OMX_FALSE;
    pExynosComponent->bMultiThreadProcess = OMX_TRUE;

    /* Input port */
    pExynosPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosPort->supportFormat = Exynos_OSAL_Malloc(sizeof(OMX_COLOR_FORMATTYPE) * INPUT_PORT_SUPPORTFORMAT_NUM_MAX);
    if (pExynosPort->supportFormat == NULL) {
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosPort->supportFormat, 0, (sizeof(OMX_COLOR_FORMATTYPE) * INPUT_PORT_SUPPORTFORMAT_NUM_MAX));

    pExynosPort->portDefinition.nBufferCountActual = MAX_VIDEO_INPUTBUFFER_NUM;
    pExynosPort->portDefinition.nBufferCountMin = MIN_VIDEO_INPUTBUFFER_NUM;
    pExynosPort->portDefinition.nBufferSize = 0;
    pExynosPort->portDefinition.eDomain = OMX_PortDomainVideo;

    pExynosPort->portDefinition.format.video.cMIMEType = Exynos_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    if (pExynosPort->portDefinition.format.video.cMIMEType == NULL) {
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "raw/video");
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

    pExynosPort->portDefinition.format.video.nFrameWidth = 0;
    pExynosPort->portDefinition.format.video.nFrameHeight= 0;
    pExynosPort->portDefinition.format.video.nStride = 0;
    pExynosPort->portDefinition.format.video.nSliceHeight = 0;
    pExynosPort->portDefinition.format.video.nBitrate = 64000;
    pExynosPort->portDefinition.format.video.xFramerate = (15 << 16);
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.format.video.pNativeWindow = NULL;

    /* Output port */
    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pExynosPort->supportFormat = Exynos_OSAL_Malloc(sizeof(OMX_COLOR_FORMATTYPE) * OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX);
    if (pExynosPort->supportFormat == NULL) {
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosPort->supportFormat, 0, (sizeof(OMX_COLOR_FORMATTYPE) * OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX));

    pExynosPort->portDefinition.nBufferCountActual = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pExynosPort->portDefinition.nBufferCountMin = MAX_VIDEO_OUTPUTBUFFER_NUM;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.eDomain = OMX_PortDomainVideo;

    pExynosPort->portDefinition.format.video.cMIMEType = Exynos_OSAL_Malloc(MAX_OMX_MIMETYPE_SIZE);
    if (pExynosPort->portDefinition.format.video.cMIMEType == NULL) {
        Exynos_OMX_VideoDecodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "raw/video");
    pExynosPort->portDefinition.format.video.pNativeRender = 0;
    pExynosPort->portDefinition.format.video.bFlagErrorConcealment = OMX_FALSE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;

    pExynosPort->portDefinition.format.video.nFrameWidth = 0;
    pExynosPort->portDefinition.format.video.nFrameHeight= 0;
    pExynosPort->portDefinition.format.video.nStride = 0;
    pExynosPort->portDefinition.format.video.nSliceHeight = 0;
    pExynosPort->portDefinition.format.video.nBitrate = 64000;
    pExynosPort->portDefinition.format.video.xFramerate = (15 << 16);
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.format.video.pNativeWindow = NULL;

    pExynosPort->processData.extInfo = (OMX_PTR)Exynos_OSAL_Malloc(sizeof(DECODE_CODEC_EXTRA_BUFFERINFO));
    Exynos_OSAL_Memset(((char *)pExynosPort->processData.extInfo), 0, sizeof(DECODE_CODEC_EXTRA_BUFFERINFO));

    int i = 0;
    DECODE_CODEC_EXTRA_BUFFERINFO *pBufferInfo = NULL;
    pBufferInfo = (DECODE_CODEC_EXTRA_BUFFERINFO *)(pExynosPort->processData.extInfo);
    for (i = 0; i < VIDEO_BUFFER_MAX_NUM; i++) {
        pBufferInfo->PDSB.dpbFD[i].fd  = -1;
        pBufferInfo->PDSB.dpbFD[i].fd1 = -1;
        pBufferInfo->PDSB.dpbFD[i].fd2 = -1;
    }

    pOMXComponent->UseBuffer              = &Exynos_OMX_UseBuffer;
    pOMXComponent->AllocateBuffer         = &Exynos_OMX_AllocateBuffer;
    pOMXComponent->FreeBuffer             = &Exynos_OMX_FreeBuffer;

#ifdef TUNNELING_SUPPORT
    pOMXComponent->ComponentTunnelRequest           = &Exynos_OMX_ComponentTunnelRequest;
    pExynosComponent->exynos_AllocateTunnelBuffer   = &Exynos_OMX_AllocateTunnelBuffer;
    pExynosComponent->exynos_FreeTunnelBuffer       = &Exynos_OMX_FreeTunnelBuffer;
#endif

    pExynosComponent->exynos_BufferProcessCreate    = &Exynos_OMX_BufferProcess_Create;
    pExynosComponent->exynos_BufferProcessTerminate = &Exynos_OMX_BufferProcess_Terminate;
    pExynosComponent->exynos_BufferFlush          = &Exynos_OMX_BufferFlush;

#ifdef USE_ANB
    pVideoDec->hRefHandle = Exynos_OSAL_RefANB_Create();
#endif

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_VideoDecodeComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE          ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE     *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT      *pExynosPort = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT *pVideoDec = NULL;
    int                    i = 0;

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

    pVideoDec = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;

#ifdef USE_ANB
    Exynos_OSAL_RefANB_Terminate(pVideoDec->hRefHandle);
#endif

    Exynos_OSAL_Free(pVideoDec);
    pExynosComponent->hComponentHandle = pVideoDec = NULL;

    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    if (pExynosPort->processData.extInfo != NULL) {
        Exynos_OSAL_Free(pExynosPort->processData.extInfo);
        pExynosPort->processData.extInfo = NULL;
    }

    for(i = 0; i < ALL_PORT_NUM; i++) {
        pExynosPort = &pExynosComponent->pExynosPort[i];
        Exynos_OSAL_Free(pExynosPort->portDefinition.format.video.cMIMEType);
        pExynosPort->portDefinition.format.video.cMIMEType = NULL;

        Exynos_OSAL_Free(pExynosPort->supportFormat);
        pExynosPort->supportFormat = NULL;
    }

    ret = Exynos_OMX_Port_Destructor(pOMXComponent);

    ret = Exynos_OMX_BaseComponent_Destructor(hComponent);

EXIT:
    FunctionOut();

    return ret;
}
