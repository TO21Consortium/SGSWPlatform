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
 * @file        Exynos_OMX_Vdec.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              HyeYeon Chung (hyeon.chung@samsung.com)
 *              Yunji Kim (yunji.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#ifndef EXYNOS_OMX_VIDEO_DECODE
#define EXYNOS_OMX_VIDEO_DECODE

#include "OMX_Component.h"
#include "Exynos_OMX_Def.h"
#include "Exynos_OSAL_Queue.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Basecomponent.h"
#include "ExynosVideoApi.h"

#define MAX_VIDEO_INPUTBUFFER_NUM           5
#define MIN_VIDEO_INPUTBUFFER_NUM           1
#define MAX_VIDEO_OUTPUTBUFFER_NUM          2

#define DEFAULT_FRAME_WIDTH                 176
#define DEFAULT_FRAME_HEIGHT                144

#define MAX_FRAME_WIDTH                 1920
#define MAX_FRAME_HEIGHT                1080

#define DEFAULT_VIDEO_INPUT_BUFFER_SIZE    (DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT) * 2
#define DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE   (DEFAULT_FRAME_WIDTH * DEFAULT_FRAME_HEIGHT * 3) / 2

#define MFC_INPUT_BUFFER_NUM_MAX            3
#define DEFAULT_MFC_INPUT_BUFFER_SIZE       MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT * 3 / 2

/* input buffer size for custom component */
#define CUSTOM_DEFAULT_VIDEO_INPUT_BUFFER_SIZE     (1024 * 1024 * 3 / 2)  /* 1.5MB */
#define CUSTOM_FHD_VIDEO_INPUT_BUFFER_SIZE         (1920 * 1080 * 3 / 2)
#define CUSTOM_UHD_VIDEO_INPUT_BUFFER_SIZE         (1024 * 1024 * 6)      /* 6MB */

#define MFC_OUTPUT_BUFFER_NUM_MAX           16 * 2
#define DEFAULT_MFC_OUTPUT_YBUFFER_SIZE     MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT
#define DEFAULT_MFC_OUTPUT_CBUFFER_SIZE     MAX_FRAME_WIDTH * MAX_FRAME_HEIGHT / 2

#define INPUT_PORT_SUPPORTFORMAT_NUM_MAX         1
#define OUTPUT_PORT_SUPPORTFORMAT_DEFAULT_NUM    2  /* I420P, NV12 */
#define OUTPUT_PORT_SUPPORTFORMAT_NUM_MAX        (OUTPUT_PORT_SUPPORTFORMAT_DEFAULT_NUM + 4)  /* NV12T, YV12, NV21, UNUSED */

#define EXTRA_DPB_NUM                       5

#define MFC_DEFAULT_INPUT_BUFFER_PLANE      1
#define MFC_DEFAULT_OUTPUT_BUFFER_PLANE     2

#define MAX_INPUTBUFFER_NUM_DYNAMIC         0 /* Dynamic number of metadata buffer */
#define MAX_OUTPUTBUFFER_NUM_DYNAMIC        0 /* Dynamic number of metadata buffer */

#define MAX_SECURE_INPUT_BUFFER_SIZE        ((3 * 1024 * 1024) / 2) /* 1.5MB */

#define DEC_BLOCKS_PER_SECOND               979200 /* remove it and have to read a capability at media_codecs.xml */

typedef struct
{
    void *pAddrY;
    void *pAddrC;
} CODEC_DEC_ADDR_INFO;

typedef struct _BYPASS_BUFFER_INFO
{
    OMX_U32   nFlags;
    OMX_TICKS timeStamp;
} BYPASS_BUFFER_INFO;

typedef struct _CODEC_DEC_BUFFER
{
    void            *pVirAddr[MAX_BUFFER_PLANE];   /* virtual address   */
    unsigned int     bufferSize[MAX_BUFFER_PLANE]; /* buffer alloc size */
    int              fd[MAX_BUFFER_PLANE];         /* buffer FD */
    int              dataSize;                     /* total data length */
} CODEC_DEC_BUFFER;

typedef struct _DECODE_CODEC_EXTRA_BUFFERINFO
{
    /* For Decode Output */
    OMX_U32                imageWidth;
    OMX_U32                imageHeight;
    OMX_U32                imageStride;
    OMX_COLOR_FORMATTYPE   ColorFormat;
    PrivateDataShareBuffer PDSB;
} DECODE_CODEC_EXTRA_BUFFERINFO;

typedef struct _EXYNOS_OMX_CURRENT_FRAME_TIMESTAMP
{
    OMX_S32   nIndex;
    OMX_TICKS timeStamp;
    OMX_U32   nFlags;
} EXYNOS_OMX_CURRENT_FRAME_TIMESTAMP;

typedef struct _EXYNOS_OMX_VIDEODEC_COMPONENT
{
    OMX_HANDLETYPE hCodecHandle;
    OMX_BOOL bDiscardCSDError;          /* if it is true, discard a error event in CorruptedHeader case */
    OMX_BOOL bForceHeaderParsing;
    OMX_BOOL bThumbnailMode;
    OMX_BOOL bDTSMode;                  /* true:Decoding Time Stamp, false:Presentation Time Stamp */
    OMX_BOOL bReorderMode;              /* true:use Time Stamp reordering, don't care about a mode like as PTS or DTS */
    OMX_BOOL b10bitData;                /* true: 10bit decoding, false: 8bit decoding */
    OMX_BOOL bQosChanged;
    OMX_U32  nQosRatio;
    CODEC_DEC_BUFFER *pMFCDecInputBuffer[MFC_INPUT_BUFFER_NUM_MAX];
    CODEC_DEC_BUFFER *pMFCDecOutputBuffer[MFC_OUTPUT_BUFFER_NUM_MAX];

    /* Buffer Process */
    OMX_BOOL       bExitBufferProcessThread;
    OMX_HANDLETYPE hSrcInputThread;
    OMX_HANDLETYPE hSrcOutputThread;
    OMX_HANDLETYPE hDstInputThread;
    OMX_HANDLETYPE hDstOutputThread;

    /* Shared Memory Handle */
    OMX_HANDLETYPE hSharedMemory;

    /* For Reconfiguration DPB */
    OMX_BOOL bReconfigDPB;

    /* For Ref Cnt handling about graphic buffer */
    OMX_HANDLETYPE hRefHandle;

    /* CSC handle */
    OMX_PTR csc_handle;
    OMX_U32 csc_set_format;

    OMX_ERRORTYPE (*exynos_codec_srcInputProcess) (OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pInputData);
    OMX_ERRORTYPE (*exynos_codec_srcOutputProcess) (OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pInputData);
    OMX_ERRORTYPE (*exynos_codec_dstInputProcess) (OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pOutputData);
    OMX_ERRORTYPE (*exynos_codec_dstOutputProcess) (OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pOutputData);

    OMX_ERRORTYPE (*exynos_codec_start)(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*exynos_codec_stop)(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*exynos_codec_bufferProcessRun)(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);
    OMX_ERRORTYPE (*exynos_codec_enqueueAllBuffer)(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);

#if 0  /* unused code */
    int (*exynos_checkInputFrame) (OMX_U8 *pInputStream, OMX_U32 buffSize, OMX_U32 flag,
                                   OMX_BOOL bPreviousFrameEOF, OMX_BOOL *pbEndOfFrame);
    OMX_ERRORTYPE (*exynos_codec_getCodecInputPrivateData) (OMX_PTR codecBuffer, OMX_PTR *addr, OMX_U32 *size);
#endif
    OMX_ERRORTYPE (*exynos_codec_getCodecOutputPrivateData) (OMX_PTR codecBuffer, OMX_PTR addr[], OMX_U32 size[]);

    OMX_ERRORTYPE (*exynos_codec_reconfigAllBuffers) (OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);
    OMX_BOOL      (*exynos_codec_checkFormatSupport)(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_COLOR_FORMATTYPE eColorFormat);
    OMX_ERRORTYPE (*exynos_codec_checkResolutionChange)(OMX_COMPONENTTYPE *pOMXComponent);
} EXYNOS_OMX_VIDEODEC_COMPONENT;

#ifdef __cplusplus
extern "C" {
#endif

inline void Exynos_UpdateFrameSize(OMX_COMPONENTTYPE *pOMXComponent);
void Exynos_Output_SetSupportFormat(EXYNOS_OMX_BASECOMPONENT *pExynosComponent);
OMX_ERRORTYPE Exynos_ResolutionUpdate(OMX_COMPONENTTYPE *pOMXComponent);
void Exynos_SetReorderTimestamp(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 *nIndex, OMX_TICKS timeStamp, OMX_U32 nFlags);
void Exynos_GetReorderTimestamp(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, EXYNOS_OMX_CURRENT_FRAME_TIMESTAMP *sCurrentTimestamp, OMX_S32 nFrameIndex, OMX_S32 eFrameType);
OMX_BOOL Exynos_Check_BufferProcess_State(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_U32 nPortIndex);
OMX_ERRORTYPE Exynos_CodecBufferToData(CODEC_DEC_BUFFER *codecBuffer, EXYNOS_OMX_DATA *pData, OMX_U32 nPortIndex);
OMX_BOOL Exynos_Preprocessor_InputData(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *srcInputData);
OMX_ERRORTYPE Exynos_OMX_SrcInputBufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE Exynos_OMX_SrcOutputBufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE Exynos_OMX_DstInputBufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE Exynos_OMX_DstOutputBufferProcess(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE Exynos_OMX_VideoDecodeComponentInit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE Exynos_OMX_VideoDecodeComponentDeinit(OMX_IN OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE Exynos_Allocate_CodecBuffers(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex, int nBufferCnt, unsigned int nAllocSize[MAX_BUFFER_PLANE]);
void Exynos_Free_CodecBuffers(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex);
OMX_ERRORTYPE Exynos_ResetAllPortConfig(OMX_COMPONENTTYPE *pOMXComponent);

#ifdef __cplusplus
}
#endif

#endif
