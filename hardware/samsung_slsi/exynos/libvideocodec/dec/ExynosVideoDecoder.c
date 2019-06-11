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
 * @file        ExynosVideoDecoder.c
 * @brief
 * @author      Jinsung Yang (jsgood.yang@samsung.com)
 * @version     1.0.0
 * @history
 *   2012.01.15: Initial Version
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <pthread.h>

#include <sys/poll.h>

#include "videodev2_exynos_media.h"
#ifdef USE_EXYNOS_MEDIA_EXT
#include "videodev2_exynos_media_ext.h"
#endif
#ifdef USE_MFC_MEDIA
#include "exynos_mfc_media.h"
#endif

#include <ion/ion.h>
#include "exynos_ion.h"


#include "ExynosVideoApi.h"
#include "ExynosVideoDec.h"
#include "OMX_Core.h"

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosVideoDecoder"
#include <utils/Log.h>

#define MAX_INPUTBUFFER_COUNT 32
#define MAX_OUTPUTBUFFER_COUNT 32

/*
 * [Common] __CodingType_To_V4L2PixelFormat
 */
static unsigned int __CodingType_To_V4L2PixelFormat(ExynosVideoCodingType codingType)
{
    unsigned int pixelformat = V4L2_PIX_FMT_H264;

    switch (codingType) {
    case VIDEO_CODING_AVC:
        pixelformat = V4L2_PIX_FMT_H264;
        break;
    case VIDEO_CODING_MPEG4:
        pixelformat = V4L2_PIX_FMT_MPEG4;
        break;
    case VIDEO_CODING_VP8:
        pixelformat = V4L2_PIX_FMT_VP8;
        break;
    case VIDEO_CODING_H263:
        pixelformat = V4L2_PIX_FMT_H263;
        break;
    case VIDEO_CODING_VC1:
        pixelformat = V4L2_PIX_FMT_VC1_ANNEX_G;
        break;
    case VIDEO_CODING_VC1_RCV:
        pixelformat = V4L2_PIX_FMT_VC1_ANNEX_L;
        break;
    case VIDEO_CODING_MPEG2:
        pixelformat = V4L2_PIX_FMT_MPEG2;
        break;
#ifdef USE_HEVCDEC_SUPPORT
    case VIDEO_CODING_HEVC:
        pixelformat = V4L2_PIX_FMT_HEVC;
        break;
#endif
#ifdef USE_VP9DEC_SUPPORT
    case VIDEO_CODING_VP9:
        pixelformat = V4L2_PIX_FMT_VP9;
        break;
#endif
    default:
        pixelformat = V4L2_PIX_FMT_H264;
        break;
    }

    return pixelformat;
}

/*
 * [Common] __ColorFormatType_To_V4L2PixelFormat
 */
static unsigned int __ColorFormatType_To_V4L2PixelFormat(
    ExynosVideoColorFormatType  colorFormatType,
    int                         nHwVersion)
{
    unsigned int pixelformat = V4L2_PIX_FMT_NV12M;

    switch (colorFormatType) {
    case VIDEO_COLORFORMAT_NV12M:
        pixelformat = V4L2_PIX_FMT_NV12M;
        break;
    case VIDEO_COLORFORMAT_NV21M:
        pixelformat = V4L2_PIX_FMT_NV21M;
        break;
    case VIDEO_COLORFORMAT_NV12M_TILED:
        if (nHwVersion == (int)MFC_51)
            pixelformat = V4L2_PIX_FMT_NV12MT;
        else
            pixelformat = V4L2_PIX_FMT_NV12MT_16X16;
        break;
    case VIDEO_COLORFORMAT_I420M:
        pixelformat = V4L2_PIX_FMT_YUV420M;
        break;
    case VIDEO_COLORFORMAT_YV12M:
        pixelformat = V4L2_PIX_FMT_YVU420M;
        break;
#ifdef USE_SINGLE_PALNE_SUPPORT
    case VIDEO_COLORFORMAT_NV12:
        pixelformat = V4L2_PIX_FMT_NV12N;
        break;
    case VIDEO_COLORFORMAT_I420:
        pixelformat = V4L2_PIX_FMT_YUV420N;
        break;
    case VIDEO_COLORFORMAT_NV12_TILED:
        pixelformat = V4L2_PIX_FMT_NV12NT;
        break;
#endif
    default:
        pixelformat = V4L2_PIX_FMT_NV12M;
        break;
    }

    return pixelformat;
}

/*
 * [Common] __Set_SupportFormat
 */
static void __Set_SupportFormat(ExynosVideoInstInfo *pVideoInstInfo)
{
    int nLastIndex = 0;

    if (pVideoInstInfo == NULL) {
        ALOGE("%s: ExynosVideoInstInfo must be supplied", __func__);
        return ;
    }

    memset(pVideoInstInfo->supportFormat, (int)VIDEO_COLORFORMAT_UNKNOWN, sizeof(pVideoInstInfo->supportFormat));

#ifdef USE_HEVC_HWIP
    if (pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC) {
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        goto EXIT;
    }
#endif

    switch (pVideoInstInfo->HwVersion) {
    case MFC_101:  /* NV12, NV21, I420, YV12 */
    case MFC_100:
    case MFC_1010:
    case MFC_1011:
    case MFC_90:
    case MFC_80:
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        break;
    case MFC_92:  /* NV12, NV21 */
    case MFC_78D:
    case MFC_1020:
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;
        break;
    case MFC_723:  /* NV12T, [NV12, NV21, I420, YV12] */
    case MFC_72:
    case MFC_77:
        if (pVideoInstInfo->specificInfo.dec.bDualDPBSupport == VIDEO_TRUE) {
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
            pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        }
    case MFC_78:  /* NV12T */
    case MFC_65:
    case MFC_61:
    case MFC_51:
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12_TILED;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M_TILED;
        break;
    default:
        break;
    }

EXIT:
    return ;
}

/*
 * [Common] __V4L2PixelFormat_To_ColorFormatType
 */
static ExynosVideoColorFormatType __V4L2PixelFormat_To_ColorFormatType(unsigned int pixelformat)
{
    ExynosVideoColorFormatType colorFormatType = VIDEO_COLORFORMAT_NV12_TILED;

    switch (pixelformat) {
    case V4L2_PIX_FMT_NV12M:
        colorFormatType = VIDEO_COLORFORMAT_NV12M;
        break;
    case V4L2_PIX_FMT_NV21M:
        colorFormatType = VIDEO_COLORFORMAT_NV21M;
        break;
    case V4L2_PIX_FMT_YUV420M:
        colorFormatType = VIDEO_COLORFORMAT_I420M;
        break;
    case V4L2_PIX_FMT_YVU420M:
        colorFormatType = VIDEO_COLORFORMAT_YV12M;
        break;
#ifdef USE_SINGLE_PALNE_SUPPORT
    case V4L2_PIX_FMT_NV12N:
    case V4L2_PIX_FMT_NV12N_10B:
        colorFormatType = VIDEO_COLORFORMAT_NV12;
        break;
    case V4L2_PIX_FMT_YUV420N:
        colorFormatType = VIDEO_COLORFORMAT_I420;
        break;
    case V4L2_PIX_FMT_NV12NT:
        colorFormatType = VIDEO_COLORFORMAT_NV12_TILED;
        break;
#endif
    case V4L2_PIX_FMT_NV12MT:
    case V4L2_PIX_FMT_NV12MT_16X16:
    default:
        colorFormatType = VIDEO_COLORFORMAT_NV12M_TILED;
        break;
    }

    return colorFormatType;
}

/*
 * [Decoder OPS] Init
 */
static void *MFC_Decoder_Init(ExynosVideoInstInfo *pVideoInfo)
{
    ExynosVideoDecContext *pCtx     = NULL;
    pthread_mutex_t       *pMutex   = NULL;
    int                    needCaps = (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING);

    int hIonClient = -1;

    if (pVideoInfo == NULL) {
        ALOGE("%s: bad parameter", __func__);
        goto EXIT_ALLOC_FAIL;
    }

    pCtx = (ExynosVideoDecContext *)malloc(sizeof(*pCtx));
    if (pCtx == NULL) {
        ALOGE("%s: Failed to allocate decoder context buffer", __func__);
        goto EXIT_ALLOC_FAIL;
    }

    memset(pCtx, 0, sizeof(*pCtx));

#ifdef USE_HEVC_HWIP
    if (pVideoInfo->eCodecType == VIDEO_CODING_HEVC) {
        if (pVideoInfo->eSecurityType == VIDEO_SECURE) {
            pCtx->hDec = exynos_v4l2_open_devname(VIDEO_HEVC_SECURE_DECODER_NAME, O_RDWR, 0);
        } else {
            pCtx->hDec = exynos_v4l2_open_devname(VIDEO_HEVC_DECODER_NAME, O_RDWR, 0);
        }
    } else
#endif
    {
        if (pVideoInfo->eSecurityType == VIDEO_SECURE) {
            pCtx->hDec = exynos_v4l2_open_devname(VIDEO_MFC_SECURE_DECODER_NAME, O_RDWR, 0);
        } else {
            pCtx->hDec = exynos_v4l2_open_devname(VIDEO_MFC_DECODER_NAME, O_RDWR, 0);
        }
    }

    if (pCtx->hDec < 0) {
        ALOGE("%s: Failed to open decoder device", __func__);
        goto EXIT_OPEN_FAIL;
    }

    memcpy(&pCtx->videoInstInfo, pVideoInfo, sizeof(pCtx->videoInstInfo));

    ALOGV("%s: HW version is %x", __func__, pCtx->videoInstInfo.HwVersion);

    if (!exynos_v4l2_querycap(pCtx->hDec, needCaps)) {
        ALOGE("%s: Failed to querycap", __func__);
        goto EXIT_QUERYCAP_FAIL;
    }

    pCtx->bStreamonInbuf = VIDEO_FALSE;
    pCtx->bStreamonOutbuf = VIDEO_FALSE;

    pMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (pMutex == NULL) {
        ALOGE("%s: Failed to allocate mutex about input buffer", __func__);
        goto EXIT_QUERYCAP_FAIL;
    }
    if (pthread_mutex_init(pMutex, NULL) != 0) {
        free(pMutex);
        goto EXIT_QUERYCAP_FAIL;
    }
    pCtx->pInMutex = (void*)pMutex;

    pMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    if (pMutex == NULL) {
        ALOGE("%s: Failed to allocate mutex about output buffer", __func__);
        goto EXIT_QUERYCAP_FAIL;
    }
    if (pthread_mutex_init(pMutex, NULL) != 0) {
        free(pMutex);
        goto EXIT_QUERYCAP_FAIL;
    }
    pCtx->pOutMutex = (void*)pMutex;

    hIonClient = ion_open();
    if (hIonClient < 0) {
        ALOGE("%s: Failed to create ion_client", __func__);
        goto EXIT_QUERYCAP_FAIL;
    }
    pCtx->hIONHandle = hIonClient;

    if (ion_alloc_fd(pCtx->hIONHandle, sizeof(PrivateDataShareBuffer) * VIDEO_BUFFER_MAX_NUM,
                     0, ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED, &(pCtx->nPrivateDataShareFD)) < 0) {
        ALOGE("%s: Failed to ion_alloc_fd for nPrivateDataShareFD", __func__);
        goto EXIT_QUERYCAP_FAIL;
    }

    pCtx->pPrivateDataShareAddress = mmap(NULL, (sizeof(PrivateDataShareBuffer) * VIDEO_BUFFER_MAX_NUM),
                                          PROT_READ | PROT_WRITE, MAP_SHARED, pCtx->nPrivateDataShareFD, 0);
    if (pCtx->pPrivateDataShareAddress == MAP_FAILED) {
        ALOGE("%s: Failed to mmap for nPrivateDataShareFD", __func__);
        goto EXIT_QUERYCAP_FAIL;
    }

    memset(pCtx->pPrivateDataShareAddress, -1, sizeof(PrivateDataShareBuffer) * VIDEO_BUFFER_MAX_NUM);

    return (void *)pCtx;

EXIT_QUERYCAP_FAIL:
    if (pCtx->pInMutex != NULL) {
        pthread_mutex_destroy(pCtx->pInMutex);
        free(pCtx->pInMutex);
    }

    if (pCtx->pOutMutex != NULL) {
        pthread_mutex_destroy(pCtx->pOutMutex);
        free(pCtx->pOutMutex);
    }

    /* free a ion_buffer */
    if (pCtx->nPrivateDataShareFD > 0) {
        close(pCtx->nPrivateDataShareFD);
        pCtx->nPrivateDataShareFD = -1;
    }

    /* free a ion_client */
    if (pCtx->hIONHandle > 0) {
        ion_close(pCtx->hIONHandle);
        pCtx->hIONHandle = -1;
    }

    exynos_v4l2_close(pCtx->hDec);

EXIT_OPEN_FAIL:
    free(pCtx);

EXIT_ALLOC_FAIL:
    return NULL;
}

/*
 * [Decoder OPS] Finalize
 */
static ExynosVideoErrorType MFC_Decoder_Finalize(void *pHandle)
{
    ExynosVideoDecContext *pCtx         = (ExynosVideoDecContext *)pHandle;
    ExynosVideoPlane      *pVideoPlane  = NULL;
    pthread_mutex_t       *pMutex       = NULL;
    ExynosVideoErrorType   ret          = VIDEO_ERROR_NONE;
    int i, j;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->pPrivateDataShareAddress != NULL) {
        munmap(pCtx->pPrivateDataShareAddress, sizeof(PrivateDataShareBuffer) * VIDEO_BUFFER_MAX_NUM);
        pCtx->pPrivateDataShareAddress = NULL;
    }

    /* free a ion_buffer */
    if (pCtx->nPrivateDataShareFD > 0) {
        close(pCtx->nPrivateDataShareFD);
        pCtx->nPrivateDataShareFD = -1;
    }

    /* free a ion_client */
    if (pCtx->hIONHandle > 0) {
        ion_close(pCtx->hIONHandle);
        pCtx->hIONHandle = -1;
    }

    if (pCtx->pOutMutex != NULL) {
        pMutex = (pthread_mutex_t*)pCtx->pOutMutex;
        pthread_mutex_destroy(pMutex);
        free(pMutex);
        pCtx->pOutMutex = NULL;
    }

    if (pCtx->pInMutex != NULL) {
        pMutex = (pthread_mutex_t*)pCtx->pInMutex;
        pthread_mutex_destroy(pMutex);
        free(pMutex);
        pCtx->pInMutex = NULL;
    }

    if (pCtx->bShareInbuf == VIDEO_FALSE) {
        for (i = 0; i < pCtx->nInbufs; i++) {
            for (j = 0; j < pCtx->nInbufPlanes; j++) {
                pVideoPlane = &pCtx->pInbuf[i].planes[j];
                if (pVideoPlane->addr != NULL) {
                    munmap(pVideoPlane->addr, pVideoPlane->allocSize);
                    pVideoPlane->addr = NULL;
                    pVideoPlane->allocSize = 0;
                    pVideoPlane->dataSize = 0;
                }

                pCtx->pInbuf[i].pGeometry = NULL;
                pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
                pCtx->pInbuf[i].bRegistered = VIDEO_FALSE;
            }
        }
    }

    if (pCtx->bShareOutbuf == VIDEO_FALSE) {
        for (i = 0; i < pCtx->nOutbufs; i++) {
            for (j = 0; j < pCtx->nOutbufPlanes; j++) {
                pVideoPlane = &pCtx->pOutbuf[i].planes[j];
                if (pVideoPlane->addr != NULL) {
                    munmap(pVideoPlane->addr, pVideoPlane->allocSize);
                    pVideoPlane->addr = NULL;
                    pVideoPlane->allocSize = 0;
                    pVideoPlane->dataSize = 0;
                }

                pCtx->pOutbuf[i].pGeometry = NULL;
                pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
                pCtx->pOutbuf[i].bRegistered = VIDEO_FALSE;
            }
        }
    }

    if (pCtx->pInbuf != NULL)
        free(pCtx->pInbuf);

    if (pCtx->pOutbuf != NULL)
        free(pCtx->pOutbuf);

    if (pCtx->hDec >= 0)
        exynos_v4l2_close(pCtx->hDec);

    free(pCtx);

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set Frame Tag
 */
static ExynosVideoErrorType MFC_Decoder_Set_FrameTag(
    void *pHandle,
    int   frameTag)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG, frameTag) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Get Frame Tag
 */
static int MFC_Decoder_Get_FrameTag(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int frameTag = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG, &frameTag);

EXIT:
    return frameTag;
}

/*
 * [Decoder OPS] Get Buffer Count
 */
static int MFC_Decoder_Get_ActualBufferCount(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int bufferCount = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MIN_BUFFERS_FOR_CAPTURE, &bufferCount);

EXIT:
    return bufferCount;
}

/*
 * [Decoder OPS] Set Display Delay
 */
static ExynosVideoErrorType MFC_Decoder_Set_DisplayDelay(
    void *pHandle,
    int   delay)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_DECODER_H264_DISPLAY_DELAY, delay) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set Immediate Display
 */
static ExynosVideoErrorType MFC_Decoder_Set_ImmediateDisplay( void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_VIDEO_DECODER_IMMEDIATE_DISPLAY, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set I-Frame Decoding
 */
static ExynosVideoErrorType MFC_Decoder_Set_IFrameDecoding(
    void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

#ifdef USE_HEVC_HWIP
    if ((pCtx->videoInstInfo.eCodecType != VIDEO_CODING_HEVC) &&
        (pCtx->videoInstInfo.HwVersion == (int)MFC_51))
#else
    if (pCtx->videoInstInfo.HwVersion == (int)MFC_51)
#endif
        return MFC_Decoder_Set_DisplayDelay(pHandle, 0);

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_I_FRAME_DECODING, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Packed PB
 */
static ExynosVideoErrorType MFC_Decoder_Enable_PackedPB(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_PACKED_PB, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Loop Filter
 */
static ExynosVideoErrorType MFC_Decoder_Enable_LoopFilter(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_VIDEO_DECODER_MPEG4_DEBLOCK_FILTER, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Slice Mode
 */
static ExynosVideoErrorType MFC_Decoder_Enable_SliceMode(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_VIDEO_DECODER_SLICE_INTERFACE, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable SEI Parsing
 */
static ExynosVideoErrorType MFC_Decoder_Enable_SEIParsing(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_VIDEO_H264_SEI_FRAME_PACKING, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Get Frame Packing information
 */
static ExynosVideoErrorType MFC_Decoder_Get_FramePackingInfo(
    void                    *pHandle,
    ExynosVideoFramePacking *pFramePacking)
{
    ExynosVideoDecContext   *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType     ret  = VIDEO_ERROR_NONE;

    struct v4l2_ext_control  ext_ctrl[FRAME_PACK_SEI_INFO_NUM];
    struct v4l2_ext_controls ext_ctrls;

    int seiAvailable, seiInfo, seiGridPos, i;
    unsigned int seiArgmtId;


    if ((pCtx == NULL) || (pFramePacking == NULL)) {
        ALOGE("%s: Video context info or FramePacking pointer must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(pFramePacking, 0, sizeof(*pFramePacking));
    memset(ext_ctrl, 0, (sizeof(struct v4l2_ext_control) * FRAME_PACK_SEI_INFO_NUM));

    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
    ext_ctrls.count = FRAME_PACK_SEI_INFO_NUM;
    ext_ctrls.controls = ext_ctrl;
    ext_ctrl[0].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_AVAIL;
    ext_ctrl[1].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_ARRGMENT_ID;
    ext_ctrl[2].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_INFO;
    ext_ctrl[3].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_GRID_POS;

    if (exynos_v4l2_g_ext_ctrl(pCtx->hDec, &ext_ctrls) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    seiAvailable = ext_ctrl[0].value;
    seiArgmtId = ext_ctrl[1].value;
    seiInfo = ext_ctrl[2].value;
    seiGridPos = ext_ctrl[3].value;

    pFramePacking->available = seiAvailable;
    pFramePacking->arrangement_id = seiArgmtId;

    pFramePacking->arrangement_cancel_flag = OPERATE_BIT(seiInfo, 0x1, 0);
    pFramePacking->arrangement_type = OPERATE_BIT(seiInfo, 0x3f, 1);
    pFramePacking->quincunx_sampling_flag = OPERATE_BIT(seiInfo, 0x1, 8);
    pFramePacking->content_interpretation_type = OPERATE_BIT(seiInfo, 0x3f, 9);
    pFramePacking->spatial_flipping_flag = OPERATE_BIT(seiInfo, 0x1, 15);
    pFramePacking->frame0_flipped_flag = OPERATE_BIT(seiInfo, 0x1, 16);
    pFramePacking->field_views_flag = OPERATE_BIT(seiInfo, 0x1, 17);
    pFramePacking->current_frame_is_frame0_flag = OPERATE_BIT(seiInfo, 0x1, 18);

    pFramePacking->frame0_grid_pos_x = OPERATE_BIT(seiGridPos, 0xf, 0);
    pFramePacking->frame0_grid_pos_y = OPERATE_BIT(seiGridPos, 0xf, 4);
    pFramePacking->frame1_grid_pos_x = OPERATE_BIT(seiGridPos, 0xf, 8);
    pFramePacking->frame1_grid_pos_y = OPERATE_BIT(seiGridPos, 0xf, 12);

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Decoding Timestamp Mode
 */
static ExynosVideoErrorType MFC_Decoder_Enable_DTSMode(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_VIDEO_DECODER_DECODING_TIMESTAMP_MODE, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set Qos Ratio
 */
static ExynosVideoErrorType MFC_Decoder_Set_QosRatio(
    void *pHandle,
    int   ratio)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_VIDEO_QOS_RATIO, ratio) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Dual DPB Mode
 */
static ExynosVideoErrorType MFC_Decoder_Enable_DualDPBMode(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC_SET_DUAL_DPB_MODE, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Enable Dynamic DPB
 */
static ExynosVideoErrorType MFC_Decoder_Enable_DynamicDPB(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC_SET_DYNAMIC_DPB_MODE, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC_SET_USER_SHARED_HANDLE, pCtx->nPrivateDataShareFD) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder OPS] Set Buffer Process Type
 */
static ExynosVideoErrorType MFC_Decoder_Set_BufferProcessType(
    void *pHandle,
    int   bufferProcessType)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC_SET_BUF_PROCESS_TYPE, bufferProcessType) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Enable Cacheable (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Enable_Cacheable_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_CACHEABLE, 2) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Enable Cacheable (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Enable_Cacheable_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_CACHEABLE, 1) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Set Shareable Buffer (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Shareable_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pCtx->bShareInbuf = VIDEO_TRUE;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Set Shareable Buffer (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Shareable_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pCtx->bShareOutbuf = VIDEO_TRUE;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Get Buffer (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Get_Buffer_Inbuf(
    void               *pHandle,
    int                 nIndex,
    ExynosVideoBuffer **pBuffer)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        *pBuffer = NULL;
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->nInbufs <= nIndex) {
        *pBuffer = NULL;
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    *pBuffer = (ExynosVideoBuffer *)&pCtx->pInbuf[nIndex];

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Get Buffer (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Get_Buffer_Outbuf(
    void               *pHandle,
    int                 nIndex,
    ExynosVideoBuffer **pBuffer)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        *pBuffer = NULL;
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->nOutbufs <= nIndex) {
        *pBuffer = NULL;
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    *pBuffer = (ExynosVideoBuffer *)&pCtx->pOutbuf[nIndex];

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Set Geometry (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Geometry_Inbuf(
    void                *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_format fmt;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (bufferConf == NULL) {
        ALOGE("%s: Buffer geometry must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    fmt.fmt.pix_mp.pixelformat = __CodingType_To_V4L2PixelFormat(bufferConf->eCompressionFormat);
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = bufferConf->nSizeImage;

    if (exynos_v4l2_s_fmt(pCtx->hDec, &fmt) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&pCtx->inbufGeometry, bufferConf, sizeof(pCtx->inbufGeometry));
    pCtx->nInbufPlanes = bufferConf->nPlaneCnt;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Set Geometry (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Set_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_format fmt;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (bufferConf == NULL) {
        ALOGE("%s: Buffer geometry must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    fmt.fmt.pix_mp.pixelformat = __ColorFormatType_To_V4L2PixelFormat(bufferConf->eColorFormat, pCtx->videoInstInfo.HwVersion);

    if (exynos_v4l2_s_fmt(pCtx->hDec, &fmt) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&pCtx->outbufGeometry, bufferConf, sizeof(pCtx->outbufGeometry));
    pCtx->nOutbufPlanes = bufferConf->nPlaneCnt;

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Get Geometry (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Get_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_format fmt;
    struct v4l2_crop   crop;
    int i, value;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (bufferConf == NULL) {
        ALOGE("%s: Buffer geometry must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&fmt, 0, sizeof(fmt));
    memset(&crop, 0, sizeof(crop));

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (exynos_v4l2_g_fmt(pCtx->hDec, &fmt) != 0) {
        if (errno == EAGAIN)
            ret = VIDEO_ERROR_HEADERINFO;
        else
            ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    if (exynos_v4l2_g_crop(pCtx->hDec, &crop) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    bufferConf->nFrameWidth = fmt.fmt.pix_mp.width;
    bufferConf->nFrameHeight = fmt.fmt.pix_mp.height;
    bufferConf->eColorFormat = __V4L2PixelFormat_To_ColorFormatType(fmt.fmt.pix_mp.pixelformat);
    bufferConf->nStride = fmt.fmt.pix_mp.plane_fmt[0].bytesperline;

#ifdef USE_DEINTERLACING_SUPPORT
    if ((fmt.fmt.pix_mp.field == V4L2_FIELD_INTERLACED) ||
        (fmt.fmt.pix_mp.field == V4L2_FIELD_INTERLACED_TB) ||
        (fmt.fmt.pix_mp.field == V4L2_FIELD_INTERLACED_BT))
        bufferConf->bInterlaced = VIDEO_TRUE;
    else
#endif
        bufferConf->bInterlaced = VIDEO_FALSE;

    exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC_GET_10BIT_INFO, &value);
    if (value == 1) {
        bufferConf->eFilledDataType = DATA_8BIT_WITH_2BIT;
        if (pCtx->outbufGeometry.eColorFormat != bufferConf->eColorFormat) {
            /* format is internally changed, because requested format is not supported by MFC H/W
             * In this case, NV12 will be used.
             */
            pCtx->nOutbufPlanes = 2;

#ifdef USE_SINGLE_PALNE_SUPPORT
            if (pCtx->videoInstInfo.eSecurityType == VIDEO_SECURE)
                pCtx->nOutbufPlanes = 1;
#endif
        }
    }

    /* Get planes aligned buffer size */
    for (i = 0; i < pCtx->nOutbufPlanes; i++)
        bufferConf->nAlignPlaneSize[i] = fmt.fmt.pix_mp.plane_fmt[i].sizeimage;

    bufferConf->cropRect.nTop = crop.c.top;
    bufferConf->cropRect.nLeft = crop.c.left;
    bufferConf->cropRect.nWidth = crop.c.width;
    bufferConf->cropRect.nHeight = crop.c.height;

    ALOGV("%s: %s contents", __FUNCTION__, (bufferConf->eFilledDataType & DATA_10BIT)? "10bit":"8bit");

    memcpy(&pCtx->outbufGeometry, bufferConf, sizeof(pCtx->outbufGeometry));

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Setup (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Setup_Inbuf(
    void         *pHandle,
    unsigned int  nBufferCount)
{
    ExynosVideoDecContext *pCtx         = (ExynosVideoDecContext *)pHandle;
    ExynosVideoPlane      *pVideoPlane  = NULL;
    ExynosVideoErrorType   ret          = VIDEO_ERROR_NONE;

    struct v4l2_requestbuffers req;
    struct v4l2_buffer         buf;
    struct v4l2_plane          planes[VIDEO_BUFFER_MAX_PLANES];
    int i;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (nBufferCount == 0) {
        nBufferCount = MAX_INPUTBUFFER_COUNT;
        ALOGV("%s: Change buffer count %d", __func__, nBufferCount);
    }

    ALOGV("%s: setting up inbufs (%d) shared=%s\n", __func__, nBufferCount,
          pCtx->bShareInbuf ? "true" : "false");

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareInbuf == VIDEO_TRUE)
        req.memory = pCtx->videoInstInfo.nMemoryType;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hDec, &req) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    if (req.count != nBufferCount) {
        ALOGE("%s: asked for %d, got %d\n", __func__, nBufferCount, req.count);
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }

    pCtx->nInbufs = (int)req.count;

    pCtx->pInbuf = malloc(sizeof(*pCtx->pInbuf) * pCtx->nInbufs);
    if (pCtx->pInbuf == NULL) {
        ALOGE("Failed to allocate input buffer context");
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }
    memset(pCtx->pInbuf, 0, sizeof(*pCtx->pInbuf) * pCtx->nInbufs);

    memset(&buf, 0, sizeof(buf));

    if (pCtx->bShareInbuf == VIDEO_FALSE) {
        buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = planes;
        buf.length = pCtx->nInbufPlanes;

        for (i = 0; i < pCtx->nInbufs; i++) {
            buf.index = i;
            if (exynos_v4l2_querybuf(pCtx->hDec, &buf) != 0) {
                ret = VIDEO_ERROR_APIFAIL;
                goto EXIT;
            }

            pVideoPlane = &pCtx->pInbuf[i].planes[0];

            pVideoPlane->addr = mmap(NULL,
                    buf.m.planes[0].length, PROT_READ | PROT_WRITE,
                    MAP_SHARED, pCtx->hDec, buf.m.planes[0].m.mem_offset);

            if (pVideoPlane->addr == MAP_FAILED) {
                ret = VIDEO_ERROR_MAPFAIL;
                goto EXIT;
            }

            pVideoPlane->allocSize = buf.m.planes[0].length;
            pVideoPlane->dataSize = 0;

            pCtx->pInbuf[i].pGeometry = &pCtx->inbufGeometry;
            pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
            pCtx->pInbuf[i].bRegistered = VIDEO_TRUE;
        }
    }

    return ret;

EXIT:
    if ((pCtx != NULL) && (pCtx->pInbuf != NULL)) {
        if (pCtx->bShareInbuf == VIDEO_FALSE) {
            for (i = 0; i < pCtx->nInbufs; i++) {
                pVideoPlane = &pCtx->pInbuf[i].planes[0];
                if (pVideoPlane->addr == MAP_FAILED) {
                    pVideoPlane->addr = NULL;
                    break;
                }

                munmap(pVideoPlane->addr, pVideoPlane->allocSize);
            }
        }

        free(pCtx->pInbuf);
        pCtx->pInbuf = NULL;
    }

    return ret;
}

/*
 * [Decoder Buffer OPS] Setup (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Setup_Outbuf(
    void         *pHandle,
    unsigned int  nBufferCount)
{
    ExynosVideoDecContext *pCtx         = (ExynosVideoDecContext *)pHandle;
    ExynosVideoPlane      *pVideoPlane  = NULL;
    ExynosVideoErrorType   ret          = VIDEO_ERROR_NONE;

    struct v4l2_requestbuffers req;
    struct v4l2_buffer         buf;
    struct v4l2_plane          planes[VIDEO_BUFFER_MAX_PLANES];
    int i, j;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (nBufferCount == 0) {
        nBufferCount = MAX_OUTPUTBUFFER_COUNT;
        ALOGV("%s: Change buffer count %d", __func__, nBufferCount);
    }

    ALOGV("%s: setting up outbufs (%d) shared=%s\n", __func__, nBufferCount,
          pCtx->bShareOutbuf ? "true" : "false");

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareOutbuf == VIDEO_TRUE)
        req.memory = pCtx->videoInstInfo.nMemoryType;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hDec, &req) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    if (req.count != nBufferCount) {
        ALOGE("%s: asked for %d, got %d\n", __func__, nBufferCount, req.count);
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }

    pCtx->nOutbufs = req.count;

    pCtx->pOutbuf = malloc(sizeof(*pCtx->pOutbuf) * pCtx->nOutbufs);
    if (pCtx->pOutbuf == NULL) {
        ALOGE("Failed to allocate output buffer context");
        ret = VIDEO_ERROR_NOMEM;
        goto EXIT;
    }
    memset(pCtx->pOutbuf, 0, sizeof(*pCtx->pOutbuf) * pCtx->nOutbufs);

    memset(&buf, 0, sizeof(buf));

    if (pCtx->bShareOutbuf == VIDEO_FALSE) {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.m.planes = planes;
        buf.length = pCtx->nOutbufPlanes;

        for (i = 0; i < pCtx->nOutbufs; i++) {
            buf.index = i;
            if (exynos_v4l2_querybuf(pCtx->hDec, &buf) != 0) {
                ret = VIDEO_ERROR_APIFAIL;
                goto EXIT;
            }

            for (j = 0; j < pCtx->nOutbufPlanes; j++) {
                pVideoPlane = &pCtx->pOutbuf[i].planes[j];
                pVideoPlane->addr = mmap(NULL,
                        buf.m.planes[j].length, PROT_READ | PROT_WRITE,
                        MAP_SHARED, pCtx->hDec, buf.m.planes[j].m.mem_offset);

                if (pVideoPlane->addr == MAP_FAILED) {
                    ret = VIDEO_ERROR_MAPFAIL;
                    goto EXIT;
                }

                pVideoPlane->allocSize = buf.m.planes[j].length;
                pVideoPlane->dataSize = 0;
            }

            pCtx->pOutbuf[i].pGeometry = &pCtx->outbufGeometry;
            pCtx->pOutbuf[i].bQueued      = VIDEO_FALSE;
            pCtx->pOutbuf[i].bSlotUsed    = VIDEO_FALSE;
            pCtx->pOutbuf[i].nIndexUseCnt = 0;
            pCtx->pOutbuf[i].bRegistered  = VIDEO_TRUE;
        }
    }

    return ret;

EXIT:
    if ((pCtx != NULL) && (pCtx->pOutbuf != NULL)) {
        if (pCtx->bShareOutbuf == VIDEO_FALSE) {
            for (i = 0; i < pCtx->nOutbufs; i++) {
                for (j = 0; j < pCtx->nOutbufPlanes; j++) {
                    pVideoPlane = &pCtx->pOutbuf[i].planes[j];
                    if (pVideoPlane->addr == MAP_FAILED) {
                        pVideoPlane->addr = NULL;
                        break;
                    }

                    munmap(pVideoPlane->addr, pVideoPlane->allocSize);
                }
            }
        }

        free(pCtx->pOutbuf);
        pCtx->pOutbuf = NULL;
    }

    return ret;
}

/*
 * [Decoder Buffer OPS] Run (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Run_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonInbuf == VIDEO_FALSE) {
        if (exynos_v4l2_streamon(pCtx->hDec, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) != 0) {
            ALOGE("%s: Failed to streamon for input buffer", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
        pCtx->bStreamonInbuf = VIDEO_TRUE;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Run (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Run_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonOutbuf == VIDEO_FALSE) {
        if (exynos_v4l2_streamon(pCtx->hDec, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) != 0) {
            ALOGE("%s: Failed to streamon for output buffer", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
        pCtx->bStreamonOutbuf = VIDEO_TRUE;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Stop (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Stop_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int i = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonInbuf == VIDEO_TRUE) {
        if (exynos_v4l2_streamoff(pCtx->hDec, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) != 0) {
            ALOGE("%s: Failed to streamoff for input buffer", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
        pCtx->bStreamonInbuf = VIDEO_FALSE;
    }

    for (i = 0; i <  pCtx->nInbufs; i++) {
        pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Stop (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Stop_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int i = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonOutbuf == VIDEO_TRUE) {
        if (exynos_v4l2_streamoff(pCtx->hDec, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) != 0) {
            ALOGE("%s: Failed to streamoff for output buffer", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
        pCtx->bStreamonOutbuf = VIDEO_FALSE;
    }

    for (i = 0; i < pCtx->nOutbufs; i++) {
        pCtx->pOutbuf[i].bQueued      = VIDEO_FALSE;
        pCtx->pOutbuf[i].bSlotUsed    = VIDEO_FALSE;
        pCtx->pOutbuf[i].nIndexUseCnt = 0;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Wait (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Wait_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct pollfd poll_events;
    int poll_state;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    poll_events.fd = pCtx->hDec;
    poll_events.events = POLLOUT | POLLERR;
    poll_events.revents = 0;

    do {
        poll_state = poll((struct pollfd*)&poll_events, 1, VIDEO_DECODER_POLL_TIMEOUT);
        if (poll_state > 0) {
            if (poll_events.revents & POLLOUT) {
                break;
            } else {
                ALOGE("%s: Poll return error", __func__);
                ret = VIDEO_ERROR_POLL;
                break;
            }
        } else if (poll_state < 0) {
            ALOGE("%s: Poll state error", __func__);
            ret = VIDEO_ERROR_POLL;
            break;
        }
    } while (poll_state == 0);

EXIT:
    return ret;
}

static ExynosVideoErrorType MFC_Decoder_Register_Inbuf(
    void             *pHandle,
    ExynosVideoPlane *planes,
    int               nPlanes)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int nIndex, plane;

    if ((pCtx == NULL) || (planes == NULL) || (nPlanes != pCtx->nInbufPlanes)) {
        ALOGE("%s: params must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
        if (pCtx->pInbuf[nIndex].bRegistered == VIDEO_FALSE) {
            for (plane = 0; plane < nPlanes; plane++) {
                pCtx->pInbuf[nIndex].planes[plane].addr = planes[plane].addr;
                pCtx->pInbuf[nIndex].planes[plane].allocSize = planes[plane].allocSize;
                pCtx->pInbuf[nIndex].planes[plane].fd = planes[plane].fd;
                ALOGV("%s: registered buf %d (addr=%p alloc_sz=%u fd=%d)\n", __func__, nIndex,
                  planes[plane].addr, planes[plane].allocSize, planes[plane].fd);
            }
            pCtx->pInbuf[nIndex].bRegistered = VIDEO_TRUE;
            break;
        }
    }

    if (nIndex == pCtx->nInbufs) {
        ALOGE("%s: can not find non-registered input buffer", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
    }

EXIT:
    return ret;
}

static ExynosVideoErrorType MFC_Decoder_Register_Outbuf(
    void             *pHandle,
    ExynosVideoPlane *planes,
    int               nPlanes)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int nIndex, plane;

    if ((pCtx == NULL) || (planes == NULL) || (nPlanes != pCtx->nOutbufPlanes)) {
        ALOGE("%s: params must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
        if (pCtx->pOutbuf[nIndex].bRegistered == VIDEO_FALSE) {
            for (plane = 0; plane < nPlanes; plane++) {
                pCtx->pOutbuf[nIndex].planes[plane].addr = planes[plane].addr;
                pCtx->pOutbuf[nIndex].planes[plane].allocSize = planes[plane].allocSize;
                pCtx->pOutbuf[nIndex].planes[plane].fd = planes[plane].fd;
                ALOGV("%s: registered buf %d[%d]:(addr=%p alloc_sz=%d fd=%d)\n",
                      __func__, nIndex, plane, planes[plane].addr, planes[plane].allocSize, planes[plane].fd);
            }

            /* this is for saving interlaced type */
            if (pCtx->outbufGeometry.bInterlaced == VIDEO_TRUE) {
                pCtx->pOutbuf[nIndex].planes[2].addr    = planes[2].addr;
                pCtx->pOutbuf[nIndex].planes[2].fd      = planes[2].fd;
            }

            pCtx->pOutbuf[nIndex].bRegistered = VIDEO_TRUE;

            break;
        }
    }

    if (nIndex == pCtx->nOutbufs) {
        ALOGE("%s: can not find non-registered output buffer", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
    }

EXIT:
    return ret;
}

static ExynosVideoErrorType MFC_Decoder_Clear_RegisteredBuffer_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int nIndex, plane;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
        for (plane = 0; plane < pCtx->nInbufPlanes; plane++) {
            pCtx->pInbuf[nIndex].planes[plane].addr = NULL;
            pCtx->pInbuf[nIndex].planes[plane].fd = -1;
        }

        pCtx->pInbuf[nIndex].bRegistered = VIDEO_FALSE;
    }

EXIT:
    return ret;
}

static ExynosVideoErrorType MFC_Decoder_Clear_RegisteredBuffer_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int nIndex, plane;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
        for (plane = 0; plane < pCtx->nOutbufPlanes; plane++) {
            pCtx->pOutbuf[nIndex].planes[plane].addr = NULL;
            pCtx->pOutbuf[nIndex].planes[plane].fd = -1;
        }

        /* this is for saving interlaced type */
        if (pCtx->outbufGeometry.bInterlaced == VIDEO_TRUE) {
            pCtx->pOutbuf[nIndex].planes[2].addr    = NULL;
            pCtx->pOutbuf[nIndex].planes[2].fd      = -1;
        }
        pCtx->pOutbuf[nIndex].bRegistered = VIDEO_FALSE;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Find (Input)
 */
static int MFC_Decoder_Find_Inbuf(
    void          *pHandle,
    unsigned char *pBuffer)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int nIndex = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
        if (pCtx->pInbuf[nIndex].bQueued == VIDEO_FALSE) {
            if ((pBuffer == NULL) ||
                (pCtx->pInbuf[nIndex].planes[0].addr == pBuffer))
                break;
        }
    }

    if (nIndex == pCtx->nInbufs)
        nIndex = -1;

EXIT:
    return nIndex;
}

/*
 * [Decoder Buffer OPS] Find (Outnput)
 */
static int MFC_Decoder_Find_Outbuf(
    void          *pHandle,
    unsigned char *pBuffer)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int nIndex = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
        if (pCtx->pOutbuf[nIndex].bQueued == VIDEO_FALSE) {
            if ((pBuffer == NULL) ||
                (pCtx->pOutbuf[nIndex].planes[0].addr == pBuffer))
                break;
        }
    }

    if (nIndex == pCtx->nOutbufs)
        nIndex = -1;

EXIT:
    return nIndex;
}

/*
 * [Decoder Buffer OPS] Enqueue (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Enqueue_Inbuf(
    void          *pHandle,
    void          *pBuffer[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_plane  planes[VIDEO_BUFFER_MAX_PLANES];
    struct v4l2_buffer buf;
    int index, i, flags = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->nInbufPlanes < nPlanes) {
        ALOGE("%s: Number of max planes : %d, nPlanes : %d", __func__,
                                    pCtx->nInbufPlanes, nPlanes);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buf.m.planes = planes;
    buf.length = pCtx->nInbufPlanes;

    pMutex = (pthread_mutex_t*)pCtx->pInMutex;
    pthread_mutex_lock(pMutex);
    index = MFC_Decoder_Find_Inbuf(pCtx, pBuffer[0]);
    if (index == -1) {
        pthread_mutex_unlock(pMutex);
        ALOGE("%s: Failed to get index", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }

    buf.index = index;
    if (pCtx->bShareInbuf == VIDEO_TRUE) {
        buf.memory = pCtx->videoInstInfo.nMemoryType;
        for (i = 0; i < nPlanes; i++) {
            if (buf.memory == V4L2_MEMORY_USERPTR)
                buf.m.planes[i].m.userptr = (unsigned long)pBuffer[i];
            else
                buf.m.planes[i].m.fd = pCtx->pInbuf[index].planes[i].fd;

            buf.m.planes[i].length = pCtx->pInbuf[index].planes[i].allocSize;
            buf.m.planes[i].bytesused = dataSize[i];
            buf.m.planes[i].data_offset = 0;
            ALOGV("%s: shared inbuf(%d) plane(%d) addr=%lx len=%d used=%d\n", __func__,
                  index, i,
                  buf.m.planes[i].m.userptr,
                  buf.m.planes[i].length,
                  buf.m.planes[i].bytesused);
        }
    } else {
        buf.memory = V4L2_MEMORY_MMAP;
        for (i = 0; i < nPlanes; i++) {
            buf.m.planes[i].bytesused = dataSize[i];
            buf.m.planes[i].data_offset = 0;
        }
    }

    if (dataSize[0] <= 0) {
        flags = EMPTY_DATA | LAST_FRAME;
        ALOGD("%s: EMPTY DATA", __FUNCTION__);
    } else {
        if ((((OMX_BUFFERHEADERTYPE *)pPrivate)->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)
            flags = LAST_FRAME;

        if ((((OMX_BUFFERHEADERTYPE *)pPrivate)->nFlags & OMX_BUFFERFLAG_CODECCONFIG) == OMX_BUFFERFLAG_CODECCONFIG)
            flags |= CSD_FRAME;

        if (flags & (CSD_FRAME | LAST_FRAME))
            ALOGD("%s: DATA with flags(0x%x)", __FUNCTION__, flags);
    }
#ifdef USE_ORIGINAL_HEADER
    buf.reserved2 = flags;
#else
    buf.input = flags;
#endif

    signed long long sec  = (((OMX_BUFFERHEADERTYPE *)pPrivate)->nTimeStamp / 1E6);
    signed long long usec = (((OMX_BUFFERHEADERTYPE *)pPrivate)->nTimeStamp) - (sec * 1E6);
    buf.timestamp.tv_sec  = (long)sec;
    buf.timestamp.tv_usec = (long)usec;

    pCtx->pInbuf[buf.index].pPrivate = pPrivate;
    pCtx->pInbuf[buf.index].bQueued = VIDEO_TRUE;
    pthread_mutex_unlock(pMutex);

    if (exynos_v4l2_qbuf(pCtx->hDec, &buf) != 0) {
        ALOGE("%s: Failed to enqueue input buffer", __func__);
        pthread_mutex_lock(pMutex);
        pCtx->pInbuf[buf.index].pPrivate = NULL;
        pCtx->pInbuf[buf.index].bQueued  = VIDEO_FALSE;
        pthread_mutex_unlock(pMutex);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Enqueue (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Enqueue_Outbuf(
    void          *pHandle,
    void          *pBuffer[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_plane  planes[VIDEO_BUFFER_MAX_PLANES];
    struct v4l2_buffer buf;
    int i, index, state = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->nOutbufPlanes < nPlanes) {
        ALOGE("%s: Number of max planes : %d, nPlanes : %d", __func__,
                                    pCtx->nOutbufPlanes, nPlanes);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.m.planes = planes;
    buf.length = pCtx->nOutbufPlanes;

    pMutex = (pthread_mutex_t*)pCtx->pOutMutex;
    pthread_mutex_lock(pMutex);
    index = MFC_Decoder_Find_Outbuf(pCtx, pBuffer[0]);
    if (index == -1) {
        pthread_mutex_unlock(pMutex);
        ALOGE("%s: Failed to get index", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }
    buf.index = index;


    if (pCtx->bShareOutbuf == VIDEO_TRUE) {
        buf.memory = pCtx->videoInstInfo.nMemoryType;
        for (i = 0; i < nPlanes; i++) {
            if (buf.memory == V4L2_MEMORY_USERPTR)
                buf.m.planes[i].m.userptr = (unsigned long)pBuffer[i];
            else
                buf.m.planes[i].m.fd = pCtx->pOutbuf[index].planes[i].fd;

            buf.m.planes[i].length = pCtx->pOutbuf[index].planes[i].allocSize;
            buf.m.planes[i].bytesused = dataSize[i];
            buf.m.planes[i].data_offset = 0;
            ALOGV("%s: shared outbuf(%d) plane=%d addr=0x%lx len=%d used=%d\n", __func__,
                  index, i,
                  buf.m.planes[i].m.userptr,
                  buf.m.planes[i].length,
                  buf.m.planes[i].bytesused);
        }
    } else {
        ALOGV("%s: non-shared outbuf(%d)\n", __func__, index);
        buf.memory = V4L2_MEMORY_MMAP;
    }

    pCtx->pOutbuf[buf.index].pPrivate = pPrivate;
    pCtx->pOutbuf[buf.index].bQueued = VIDEO_TRUE;
    pthread_mutex_unlock(pMutex);

    if (exynos_v4l2_qbuf(pCtx->hDec, &buf) != 0) {
        pthread_mutex_lock(pMutex);
        pCtx->pOutbuf[buf.index].pPrivate = NULL;
        pCtx->pOutbuf[buf.index].bQueued = VIDEO_FALSE;
        exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_CHECK_STATE, &state);
        if (state == 1) {
            /* The case of Resolution is changed */
            ret = VIDEO_ERROR_WRONGBUFFERSIZE;
        } else {
            ALOGE("%s: Failed to enqueue output buffer", __func__);
            ret = VIDEO_ERROR_APIFAIL;
        }
        pthread_mutex_unlock(pMutex);
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Dequeue (Input)
 */
static ExynosVideoBuffer *MFC_Decoder_Dequeue_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx     = (ExynosVideoDecContext *)pHandle;
    ExynosVideoBuffer     *pInbuf   = NULL;
    pthread_mutex_t       *pMutex   = NULL;

    struct v4l2_buffer buf;
    struct v4l2_plane  planes[VIDEO_BUFFER_MAX_PLANES];

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    if (pCtx->bStreamonInbuf == VIDEO_FALSE) {
        pInbuf = NULL;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buf.m.planes = planes;
    buf.length = pCtx->nInbufPlanes;

    if (pCtx->bShareInbuf == VIDEO_TRUE)
        buf.memory = pCtx->videoInstInfo.nMemoryType;
    else
        buf.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_dqbuf(pCtx->hDec, &buf) != 0) {
        pInbuf = NULL;
        goto EXIT;
    }

    pMutex = (pthread_mutex_t*)pCtx->pInMutex;
    pthread_mutex_lock(pMutex);

    pInbuf = &pCtx->pInbuf[buf.index];
    if (pInbuf->bQueued == VIDEO_FALSE) {
        pInbuf = NULL;
        pthread_mutex_unlock(pMutex);
        goto EXIT;
    }

    pCtx->pInbuf[buf.index].bQueued = VIDEO_FALSE;

    if (pCtx->bStreamonInbuf == VIDEO_FALSE)
        pInbuf = NULL;

    pthread_mutex_unlock(pMutex);

EXIT:
    return pInbuf;
}

/*
 * [Decoder Buffer OPS] Dequeue (Output)
 */
static ExynosVideoBuffer *MFC_Decoder_Dequeue_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx    = (ExynosVideoDecContext *)pHandle;
    ExynosVideoBuffer     *pOutbuf = NULL;
    pthread_mutex_t       *pMutex  = NULL;

    struct v4l2_buffer buf;
    struct v4l2_plane  planes[VIDEO_BUFFER_MAX_PLANES];

    int value = 0, state = 0;
    int ret = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    if (pCtx->bStreamonOutbuf == VIDEO_FALSE) {
        pOutbuf = NULL;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.m.planes = planes;
    buf.length = pCtx->nOutbufPlanes;

    if (pCtx->bShareOutbuf == VIDEO_TRUE)
        buf.memory = pCtx->videoInstInfo.nMemoryType;
    else
        buf.memory = V4L2_MEMORY_MMAP;

    /* HACK: pOutbuf return -1 means DECODING_ONLY for almost cases */
    ret = exynos_v4l2_dqbuf(pCtx->hDec, &buf);
    if (ret != 0) {
        if (errno == EIO)
            pOutbuf = (ExynosVideoBuffer *)VIDEO_ERROR_DQBUF_EIO;
        else
            pOutbuf = NULL;
        goto EXIT;
    }

    if (pCtx->bStreamonOutbuf == VIDEO_FALSE) {
        pOutbuf = NULL;
        goto EXIT;
    }

    pMutex = (pthread_mutex_t*)pCtx->pOutMutex;
    pthread_mutex_lock(pMutex);

    pOutbuf = &pCtx->pOutbuf[buf.index];
    if (pOutbuf->bQueued == VIDEO_FALSE) {
        pOutbuf = NULL;
        ret = VIDEO_ERROR_NOBUFFERS;
        pthread_mutex_unlock(pMutex);
        goto EXIT;
    }

    exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_DISPLAY_STATUS, &value);

    switch (value) {
    case 0:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DECODING_ONLY;
#ifdef USE_HEVC_HWIP
        if ((pCtx->videoInstInfo.eCodecType == VIDEO_CODING_HEVC) ||
            (pCtx->videoInstInfo.HwVersion != (int)MFC_51)) {
#else
        if (pCtx->videoInstInfo.HwVersion != (int)MFC_51) {
#endif
            exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_CHECK_STATE, &state);
            if (state == 4) /* DPB realloc for S3D SEI */
                pOutbuf->displayStatus = VIDEO_FRAME_STATUS_ENABLED_S3D;
        }
        break;
    case 1:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DISPLAY_DECODING;
        break;
    case 2:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DISPLAY_ONLY;
        break;
    case 3:
        exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_CHECK_STATE, &state);
        if (state == 1) /* Resolution is changed */
            pOutbuf->displayStatus = VIDEO_FRAME_STATUS_CHANGE_RESOL;
        else            /* Decoding is finished */
            pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DECODING_FINISHED;
        break;
    case 4:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_LAST_FRAME;
        break;
    default:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_UNKNOWN;
        break;
    }

    switch (buf.flags & (0x7 << 3)) {
    case V4L2_BUF_FLAG_KEYFRAME:
        pOutbuf->frameType = VIDEO_FRAME_I;
        break;
    case V4L2_BUF_FLAG_PFRAME:
        pOutbuf->frameType = VIDEO_FRAME_P;
        break;
    case V4L2_BUF_FLAG_BFRAME:
        pOutbuf->frameType = VIDEO_FRAME_B;
        break;
    default:
        pOutbuf->frameType = VIDEO_FRAME_OTHERS;
        break;
    };

    if (buf.flags & V4L2_BUF_FLAG_ERROR)
        pOutbuf->frameType |= VIDEO_FRAME_CORRUPT;

    if (pCtx->outbufGeometry.bInterlaced == VIDEO_TRUE) {
        if ((buf.field == V4L2_FIELD_INTERLACED_TB) ||
            (buf.field == V4L2_FIELD_INTERLACED_BT)) {
            pOutbuf->interlacedType = buf.field;
        } else {
            ALOGV("%s: buf.field's value is invald(%d)", __FUNCTION__, buf.field);
            pOutbuf->interlacedType = V4L2_FIELD_NONE;
        }
    }

    pOutbuf->bQueued = VIDEO_FALSE;

    pthread_mutex_unlock(pMutex);

EXIT:
    return pOutbuf;
}

static ExynosVideoErrorType MFC_Decoder_Clear_Queued_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int i;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (i = 0; i < pCtx->nInbufs; i++) {
        pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
    }

EXIT:
    return ret;
}

static ExynosVideoErrorType MFC_Decoder_Clear_Queued_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int i;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (i = 0; i < pCtx->nOutbufs; i++) {
        pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Cleanup Buffer (Input)
 */
static ExynosVideoErrorType MFC_Decoder_Cleanup_Buffer_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_requestbuffers req;
    int nBufferCount = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    nBufferCount = 0; /* for clean-up */

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareInbuf == VIDEO_TRUE)
        req.memory = pCtx->videoInstInfo.nMemoryType;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hDec, &req) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->nInbufs = (int)req.count;

    if (pCtx->pInbuf != NULL) {
        free(pCtx->pInbuf);
        pCtx->pInbuf = NULL;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Cleanup Buffer (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Cleanup_Buffer_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct v4l2_requestbuffers req;
    int nBufferCount = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    nBufferCount = 0; /* for clean-up */

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareOutbuf == VIDEO_TRUE)
        req.memory = pCtx->videoInstInfo.nMemoryType;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hDec, &req) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->nOutbufs = (int)req.count;

    if (pCtx->pOutbuf != NULL) {
        free(pCtx->pOutbuf);
        pCtx->pOutbuf = NULL;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] Apply Registered Buffer (Output)
 */
static ExynosVideoErrorType MFC_Decoder_Apply_RegisteredBuffer_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hDec, V4L2_CID_MPEG_VIDEO_DECODER_WAIT_DECODING_START, 1) != 0) {
        ALOGW("%s: The requested function is not implemented", __func__);
        //ret = VIDEO_ERROR_APIFAIL;
        //goto EXIT;    /* For Backward compatibility */
    }

    ret = MFC_Decoder_Run_Outbuf(pHandle);
    if (VIDEO_ERROR_NONE != ret)
        goto EXIT;

    ret = MFC_Decoder_Stop_Outbuf(pHandle);

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] FindIndex (Input)
 */
static int MFC_Decoder_FindEmpty_Inbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int nIndex = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
        if (pCtx->pInbuf[nIndex].bQueued == VIDEO_FALSE)
            break;
    }

    if (nIndex == pCtx->nInbufs)
        nIndex = -1;

EXIT:
    return nIndex;
}

/*
 * [Decoder Buffer OPS] ExtensionEnqueue (Input)
 */
static ExynosVideoErrorType MFC_Decoder_ExtensionEnqueue_Inbuf(
    void          *pHandle,
    void          *pBuffer[],
    int            pFd[],
    unsigned int   allocLen[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoDecContext *pCtx   = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret    = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_plane  planes[VIDEO_BUFFER_MAX_PLANES];
    struct v4l2_buffer buf;
    int index, i, flags = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->nInbufPlanes < nPlanes) {
        ALOGE("%s: Number of max planes : %d, nPlanes : %d", __func__,
                                    pCtx->nInbufPlanes, nPlanes);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));

    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buf.m.planes = planes;
    buf.length = pCtx->nInbufPlanes;

    pMutex = (pthread_mutex_t*)pCtx->pInMutex;
    pthread_mutex_lock(pMutex);
    index = MFC_Decoder_FindEmpty_Inbuf(pCtx);
    if (index == -1) {
        pthread_mutex_unlock(pMutex);
        ALOGE("%s: Failed to get index", __func__);
        ret = VIDEO_ERROR_NOBUFFERS;
        goto EXIT;
    }

    buf.index = index;
    buf.memory = pCtx->videoInstInfo.nMemoryType;
    for (i = 0; i < nPlanes; i++) {
        if (buf.memory == V4L2_MEMORY_USERPTR)
            buf.m.planes[i].m.userptr = (unsigned long)pBuffer[i];
        else
            buf.m.planes[i].m.fd = pFd[i];

        buf.m.planes[i].length = allocLen[i];
        buf.m.planes[i].bytesused = dataSize[i];
        buf.m.planes[i].data_offset = 0;

        /* Temporary storage for Dequeue */
        pCtx->pInbuf[buf.index].planes[i].addr = pBuffer[i];
        pCtx->pInbuf[buf.index].planes[i].fd = pFd[i];
        pCtx->pInbuf[buf.index].planes[i].allocSize = allocLen[i];
    }

    if (dataSize[0] <= 0) {
        flags = EMPTY_DATA | LAST_FRAME;
        ALOGD("%s: EMPTY DATA", __FUNCTION__);
    } else {
        if ((((OMX_BUFFERHEADERTYPE *)pPrivate)->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)
            flags = LAST_FRAME;

        if ((((OMX_BUFFERHEADERTYPE *)pPrivate)->nFlags & OMX_BUFFERFLAG_CODECCONFIG) == OMX_BUFFERFLAG_CODECCONFIG)
            flags |= CSD_FRAME;

        if (flags & (CSD_FRAME | LAST_FRAME))
            ALOGD("%s: DATA with flags(0x%x)", __FUNCTION__, flags);
    }
#ifdef USE_ORIGINAL_HEADER
    buf.reserved2 = flags;
#else
    buf.input = flags;
#endif

    signed long long sec  = (((OMX_BUFFERHEADERTYPE *)pPrivate)->nTimeStamp / 1E6);
    signed long long usec = (((OMX_BUFFERHEADERTYPE *)pPrivate)->nTimeStamp) - (sec * 1E6);
    buf.timestamp.tv_sec  = (long)sec;
    buf.timestamp.tv_usec = (long)usec;

    pCtx->pInbuf[buf.index].pPrivate = pPrivate;
    pCtx->pInbuf[buf.index].bQueued = VIDEO_TRUE;
    pthread_mutex_unlock(pMutex);

    if (exynos_v4l2_qbuf(pCtx->hDec, &buf) != 0) {
        ALOGE("%s: Failed to enqueue input buffer", __func__);
        pthread_mutex_lock(pMutex);
        pCtx->pInbuf[buf.index].pPrivate = NULL;
        pCtx->pInbuf[buf.index].bQueued  = VIDEO_FALSE;
        pthread_mutex_unlock(pMutex);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] ExtensionDequeue (Input)
 */
static ExynosVideoErrorType MFC_Decoder_ExtensionDequeue_Inbuf(
    void              *pHandle,
    ExynosVideoBuffer *pVideoBuffer)
{
    ExynosVideoDecContext *pCtx   = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret    = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_plane  planes[VIDEO_BUFFER_MAX_PLANES];
    struct v4l2_buffer buf;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonInbuf == VIDEO_FALSE) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    buf.m.planes = planes;
    buf.length = pCtx->nInbufPlanes;
    buf.memory = pCtx->videoInstInfo.nMemoryType;
    if (exynos_v4l2_dqbuf(pCtx->hDec, &buf) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pMutex = (pthread_mutex_t*)pCtx->pInMutex;
    pthread_mutex_lock(pMutex);

    if (pCtx->pInbuf[buf.index].bQueued == VIDEO_TRUE)
        memcpy(pVideoBuffer, &pCtx->pInbuf[buf.index], sizeof(ExynosVideoBuffer));
    else
        ret = VIDEO_ERROR_NOBUFFERS;
    memset(&pCtx->pInbuf[buf.index], 0, sizeof(ExynosVideoBuffer));

    pCtx->pInbuf[buf.index].bQueued = VIDEO_FALSE;
    pthread_mutex_unlock(pMutex);

EXIT:
    return ret;
}


/*
 * [Decoder Buffer OPS] FindIndex (Output)
 */
static int MFC_Decoder_FindEmpty_Outbuf(void *pHandle)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int nIndex = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
        if ((pCtx->pOutbuf[nIndex].bQueued == VIDEO_FALSE) &&
            (pCtx->pOutbuf[nIndex].bSlotUsed == VIDEO_FALSE))
            break;
    }

    if (nIndex == pCtx->nOutbufs)
        nIndex = -1;

EXIT:
    return nIndex;
}

/*
 * [Decoder Buffer OPS] BufferIndexFree (Output)
 */
void MFC_Decoder_BufferIndexFree_Outbuf(
    void                   *pHandle,
    PrivateDataShareBuffer *pPDSB,
    int                     index)
{
    ExynosVideoDecContext *pCtx = (ExynosVideoDecContext *)pHandle;
    int i, j;

    ALOGV("De-queue buf.index:%d, fd:%d", index, pCtx->pOutbuf[index].planes[0].fd);

    if (pCtx->pOutbuf[index].nIndexUseCnt == 0)
        pCtx->pOutbuf[index].bSlotUsed = VIDEO_FALSE;

    for (i = 0; i < VIDEO_BUFFER_MAX_NUM; i++) {
        if (pPDSB->dpbFD[i].fd < 0)
            break;

        ALOGV("pPDSB->dpbFD[%d].fd:%d", i, pPDSB->dpbFD[i].fd);
        for (j = 0; j < pCtx->nOutbufs; j++) {
            if (pPDSB->dpbFD[i].fd == pCtx->pOutbuf[j].planes[0].fd) {
                if (pCtx->pOutbuf[j].bQueued == VIDEO_FALSE) {
                    if (pCtx->pOutbuf[j].nIndexUseCnt > 0)
                        pCtx->pOutbuf[j].nIndexUseCnt--;
                } else if(pCtx->pOutbuf[j].bQueued == VIDEO_TRUE) {
                    if (pCtx->pOutbuf[j].nIndexUseCnt > 1) {
                        /* The buffer being used as the reference buffer came again. */
                        pCtx->pOutbuf[j].nIndexUseCnt--;
                    } else {
                        /* Reference DPB buffer is internally reused. */
                    }
                }
                ALOGV("dec Cnt : FD:%d, pCtx->pOutbuf[%d].nIndexUseCnt:%d", pPDSB->dpbFD[i].fd, j, pCtx->pOutbuf[j].nIndexUseCnt);
                if ((pCtx->pOutbuf[j].nIndexUseCnt == 0) &&
                    (pCtx->pOutbuf[j].bQueued == VIDEO_FALSE))
                    pCtx->pOutbuf[j].bSlotUsed = VIDEO_FALSE;
            }
        }
    }
    memset((char *)pPDSB, -1, sizeof(PrivateDataShareBuffer));

    return;
}

/*
 * [Decoder Buffer OPS] ExtensionEnqueue (Output)
 */
static ExynosVideoErrorType MFC_Decoder_ExtensionEnqueue_Outbuf(
    void          *pHandle,
    void          *pBuffer[],
    int            pFd[],
    unsigned int   allocLen[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoDecContext *pCtx   = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType   ret    = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_plane  planes[VIDEO_BUFFER_MAX_PLANES];
    struct v4l2_buffer buf;
    int i, index, state = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->nOutbufPlanes < nPlanes) {
        ALOGE("%s: Number of max planes : %d, nPlanes : %d", __func__,
                                    pCtx->nOutbufPlanes, nPlanes);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.m.planes = planes;
    buf.length = pCtx->nOutbufPlanes;

    pMutex = (pthread_mutex_t*)pCtx->pOutMutex;
    pthread_mutex_lock(pMutex);

    index = MFC_Decoder_Find_Outbuf(pCtx, pBuffer[0]);
    if (index == -1) {
        ALOGV("%s: Failed to find index", __func__);
        index = MFC_Decoder_FindEmpty_Outbuf(pCtx);
        if (index == -1) {
            pthread_mutex_unlock(pMutex);
            ALOGE("%s: Failed to get index", __func__);
            ret = VIDEO_ERROR_NOBUFFERS;
            goto EXIT;
        }
    }

    buf.index = index;
    ALOGV("En-queue index:%d pCtx->pOutbuf[buf.index].bQueued:%d, pFd[0]:%d",
           index, pCtx->pOutbuf[buf.index].bQueued, pFd[0]);

    buf.memory = pCtx->videoInstInfo.nMemoryType;
    for (i = 0; i < nPlanes; i++) {
        if (buf.memory == V4L2_MEMORY_USERPTR)
            buf.m.planes[i].m.userptr = (unsigned long)pBuffer[i];
        else
            buf.m.planes[i].m.fd = pFd[i];

        buf.m.planes[i].length = allocLen[i];
        buf.m.planes[i].bytesused = dataSize[i];
        buf.m.planes[i].data_offset = 0;

        /* Temporary storage for Dequeue */
        pCtx->pOutbuf[buf.index].planes[i].addr = pBuffer[i];
        pCtx->pOutbuf[buf.index].planes[i].fd = pFd[i];
        pCtx->pOutbuf[buf.index].planes[i].allocSize = allocLen[i];

        ALOGV("%s: shared outbuf(%d) plane=%d addr=0x%p fd=%d len=%d used=%d\n",
              __func__, index, i,
              (void*)buf.m.planes[i].m.userptr, buf.m.planes[i].m.fd,
              buf.m.planes[i].length, buf.m.planes[i].bytesused);
    }

    /* this is for saving interlaced type */
    if (pCtx->outbufGeometry.bInterlaced == VIDEO_TRUE) {
        pCtx->pOutbuf[buf.index].planes[2].addr = pBuffer[2];
        pCtx->pOutbuf[buf.index].planes[2].fd   = pFd[2];
    }

    pCtx->pOutbuf[buf.index].pPrivate = pPrivate;
    pCtx->pOutbuf[buf.index].bQueued = VIDEO_TRUE;
    pCtx->pOutbuf[buf.index].bSlotUsed = VIDEO_TRUE;
    pCtx->pOutbuf[buf.index].nIndexUseCnt++;
    pthread_mutex_unlock(pMutex);

    if (exynos_v4l2_qbuf(pCtx->hDec, &buf) != 0) {
        pthread_mutex_lock(pMutex);
        pCtx->pOutbuf[buf.index].nIndexUseCnt--;
        pCtx->pOutbuf[buf.index].pPrivate = NULL;
        pCtx->pOutbuf[buf.index].bQueued = VIDEO_FALSE;
        if (pCtx->pOutbuf[buf.index].nIndexUseCnt == 0)
            pCtx->pOutbuf[buf.index].bSlotUsed = VIDEO_FALSE;
        exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_CHECK_STATE, &state);
        if (state == 1) {
            /* The case of Resolution is changed */
            ret = VIDEO_ERROR_WRONGBUFFERSIZE;
        } else {
            ALOGE("%s: Failed to enqueue output buffer", __func__);
            ret = VIDEO_ERROR_APIFAIL;
        }
        pthread_mutex_unlock(pMutex);
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Decoder Buffer OPS] ExtensionDequeue (Output)
 */
static ExynosVideoErrorType MFC_Decoder_ExtensionDequeue_Outbuf(
    void              *pHandle,
    ExynosVideoBuffer *pVideoBuffer)
{
    ExynosVideoDecContext  *pCtx    = (ExynosVideoDecContext *)pHandle;
    ExynosVideoErrorType    ret     = VIDEO_ERROR_NONE;
    pthread_mutex_t        *pMutex  = NULL;
    ExynosVideoBuffer      *pOutbuf = NULL;
    PrivateDataShareBuffer *pPDSB   = NULL;

    struct v4l2_plane  planes[VIDEO_BUFFER_MAX_PLANES];
    struct v4l2_buffer buf;
    int value = 0, state = 0;
    int i, j;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonOutbuf == VIDEO_FALSE) {
        pOutbuf = NULL;
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    buf.m.planes = planes;
    buf.length = pCtx->nOutbufPlanes;
    buf.memory = pCtx->videoInstInfo.nMemoryType;

    /* HACK: pOutbuf return -1 means DECODING_ONLY for almost cases */
    if (exynos_v4l2_dqbuf(pCtx->hDec, &buf) != 0) {
        if (errno == EIO)
            ret = VIDEO_ERROR_DQBUF_EIO;
        else
            ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pMutex = (pthread_mutex_t*)pCtx->pOutMutex;
    pthread_mutex_lock(pMutex);

    pOutbuf = &pCtx->pOutbuf[buf.index];
    if (pOutbuf->bQueued == VIDEO_FALSE) {
        pOutbuf = NULL;
        ret = VIDEO_ERROR_NOBUFFERS;
        pthread_mutex_unlock(pMutex);
        goto EXIT;
    }

    exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_DISPLAY_STATUS, &value);

    switch (value) {
    case 0:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DECODING_ONLY;
#ifdef USE_HEVC_HWIP
        if ((pCtx->videoInstInfo.eCodecType == VIDEO_CODING_HEVC) ||
            (pCtx->videoInstInfo.HwVersion != (int)MFC_51)) {
#else
        if (pCtx->videoInstInfo.HwVersion != (int)MFC_51) {
#endif
            exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_CHECK_STATE, &state);
            if (state == 4) /* DPB realloc for S3D SEI */
                pOutbuf->displayStatus = VIDEO_FRAME_STATUS_ENABLED_S3D;
        }
        break;
    case 1:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DISPLAY_DECODING;
        break;
    case 2:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DISPLAY_ONLY;
        break;
    case 3:
        exynos_v4l2_g_ctrl(pCtx->hDec, V4L2_CID_MPEG_MFC51_VIDEO_CHECK_STATE, &state);
        if (state == 1) /* Resolution is changed */
            pOutbuf->displayStatus = VIDEO_FRAME_STATUS_CHANGE_RESOL;
        else            /* Decoding is finished */
            pOutbuf->displayStatus = VIDEO_FRAME_STATUS_DECODING_FINISHED;
        break;
    case 4:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_LAST_FRAME;
        break;
    default:
        pOutbuf->displayStatus = VIDEO_FRAME_STATUS_UNKNOWN;
        break;
    }

    switch (buf.flags & (0x7 << 3)) {
    case V4L2_BUF_FLAG_KEYFRAME:
        pOutbuf->frameType = VIDEO_FRAME_I;
        break;
    case V4L2_BUF_FLAG_PFRAME:
        pOutbuf->frameType = VIDEO_FRAME_P;
        break;
    case V4L2_BUF_FLAG_BFRAME:
        pOutbuf->frameType = VIDEO_FRAME_B;
        break;
    default:
        pOutbuf->frameType = VIDEO_FRAME_OTHERS;
        break;
    };

    if (buf.flags & V4L2_BUF_FLAG_ERROR)
        pOutbuf->frameType |= VIDEO_FRAME_CORRUPT;

    if (pCtx->outbufGeometry.bInterlaced == VIDEO_TRUE) {
        if ((buf.field == V4L2_FIELD_INTERLACED_TB) ||
            (buf.field == V4L2_FIELD_INTERLACED_BT)) {
            pOutbuf->interlacedType = buf.field;
        } else {
            ALOGV("%s: buf.field's value is invald(%d)", __FUNCTION__, buf.field);
            pOutbuf->interlacedType = V4L2_FIELD_NONE;
        }
    }

    pPDSB = ((PrivateDataShareBuffer *)pCtx->pPrivateDataShareAddress) + buf.index;
    if (pCtx->pOutbuf[buf.index].bQueued == VIDEO_TRUE) {
        memcpy(pVideoBuffer, pOutbuf, sizeof(ExynosVideoBuffer));
        memcpy((char *)(&(pVideoBuffer->PDSB)), (char *)pPDSB, sizeof(PrivateDataShareBuffer));
    } else {
        ret = VIDEO_ERROR_NOBUFFERS;
        ALOGV("%s :: %d", __FUNCTION__, __LINE__);
    }

    MFC_Decoder_BufferIndexFree_Outbuf(pHandle, pPDSB, buf.index);
    pCtx->pOutbuf[buf.index].bQueued = VIDEO_FALSE;
    pthread_mutex_unlock(pMutex);

EXIT:
    return ret;
}


/*
 * [Decoder OPS] Common
 */
static ExynosVideoDecOps defDecOps = {
    .nSize                  = 0,
    .Init                   = MFC_Decoder_Init,
    .Finalize               = MFC_Decoder_Finalize,
    .Set_DisplayDelay       = MFC_Decoder_Set_DisplayDelay,
    .Set_IFrameDecoding     = MFC_Decoder_Set_IFrameDecoding,
    .Enable_PackedPB        = MFC_Decoder_Enable_PackedPB,
    .Enable_LoopFilter      = MFC_Decoder_Enable_LoopFilter,
    .Enable_SliceMode       = MFC_Decoder_Enable_SliceMode,
    .Get_ActualBufferCount  = MFC_Decoder_Get_ActualBufferCount,
    .Set_FrameTag           = MFC_Decoder_Set_FrameTag,
    .Get_FrameTag           = MFC_Decoder_Get_FrameTag,
    .Enable_SEIParsing      = MFC_Decoder_Enable_SEIParsing,
    .Get_FramePackingInfo   = MFC_Decoder_Get_FramePackingInfo,
    .Set_ImmediateDisplay   = MFC_Decoder_Set_ImmediateDisplay,
    .Enable_DTSMode         = MFC_Decoder_Enable_DTSMode,
    .Set_QosRatio           = MFC_Decoder_Set_QosRatio,
    .Enable_DualDPBMode     = MFC_Decoder_Enable_DualDPBMode,
    .Enable_DynamicDPB      = MFC_Decoder_Enable_DynamicDPB,
    .Set_BufferProcessType  = MFC_Decoder_Set_BufferProcessType,
};

/*
 * [Decoder Buffer OPS] Input
 */
static ExynosVideoDecBufferOps defInbufOps = {
    .nSize                  = 0,
    .Enable_Cacheable       = MFC_Decoder_Enable_Cacheable_Inbuf,
    .Set_Shareable          = MFC_Decoder_Set_Shareable_Inbuf,
    .Get_Buffer             = NULL,
    .Set_Geometry           = MFC_Decoder_Set_Geometry_Inbuf,
    .Get_Geometry           = NULL,
    .Setup                  = MFC_Decoder_Setup_Inbuf,
    .Run                    = MFC_Decoder_Run_Inbuf,
    .Stop                   = MFC_Decoder_Stop_Inbuf,
    .Enqueue                = MFC_Decoder_Enqueue_Inbuf,
    .Enqueue_All            = NULL,
    .Dequeue                = MFC_Decoder_Dequeue_Inbuf,
    .Register               = MFC_Decoder_Register_Inbuf,
    .Clear_RegisteredBuffer = MFC_Decoder_Clear_RegisteredBuffer_Inbuf,
    .Clear_Queue            = MFC_Decoder_Clear_Queued_Inbuf,
    .Cleanup_Buffer         = MFC_Decoder_Cleanup_Buffer_Inbuf,
    .Apply_RegisteredBuffer = NULL,
    .ExtensionEnqueue       = MFC_Decoder_ExtensionEnqueue_Inbuf,
    .ExtensionDequeue       = MFC_Decoder_ExtensionDequeue_Inbuf,
};

/*
 * [Decoder Buffer OPS] Output
 */
static ExynosVideoDecBufferOps defOutbufOps = {
    .nSize                  = 0,
    .Enable_Cacheable       = MFC_Decoder_Enable_Cacheable_Outbuf,
    .Set_Shareable          = MFC_Decoder_Set_Shareable_Outbuf,
    .Get_Buffer             = MFC_Decoder_Get_Buffer_Outbuf,
    .Set_Geometry           = MFC_Decoder_Set_Geometry_Outbuf,
    .Get_Geometry           = MFC_Decoder_Get_Geometry_Outbuf,
    .Setup                  = MFC_Decoder_Setup_Outbuf,
    .Run                    = MFC_Decoder_Run_Outbuf,
    .Stop                   = MFC_Decoder_Stop_Outbuf,
    .Enqueue                = MFC_Decoder_Enqueue_Outbuf,
    .Enqueue_All            = NULL,
    .Dequeue                = MFC_Decoder_Dequeue_Outbuf,
    .Register               = MFC_Decoder_Register_Outbuf,
    .Clear_RegisteredBuffer = MFC_Decoder_Clear_RegisteredBuffer_Outbuf,
    .Clear_Queue            = MFC_Decoder_Clear_Queued_Outbuf,
    .Cleanup_Buffer         = MFC_Decoder_Cleanup_Buffer_Outbuf,
    .Apply_RegisteredBuffer = MFC_Decoder_Apply_RegisteredBuffer_Outbuf,
    .ExtensionEnqueue       = MFC_Decoder_ExtensionEnqueue_Outbuf,
    .ExtensionDequeue       = MFC_Decoder_ExtensionDequeue_Outbuf,
};

ExynosVideoErrorType MFC_Exynos_Video_GetInstInfo_Decoder(
    ExynosVideoInstInfo *pVideoInstInfo)
{
    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;
    int hDec = -1;
    int mode = 0, version = 0;

    if (pVideoInstInfo == NULL) {
        ALOGE("%s: bad parameter", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

#ifdef USE_HEVC_HWIP
    if (pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC)
        hDec = exynos_v4l2_open_devname(VIDEO_HEVC_DECODER_NAME, O_RDWR, 0);
    else
#endif
        hDec = exynos_v4l2_open_devname(VIDEO_MFC_DECODER_NAME, O_RDWR, 0);

    if (hDec < 0) {
        ALOGE("%s: Failed to open decoder device", __func__);
        ret = VIDEO_ERROR_OPENFAIL;
        goto EXIT;
    }

    if (exynos_v4l2_g_ctrl(hDec, V4L2_CID_MPEG_MFC_GET_VERSION_INFO, &version) != 0) {
        ALOGW("%s: HW version information is not available", __func__);
#ifdef USE_HEVC_HWIP
        if (pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC)
            pVideoInstInfo->HwVersion = (int)HEVC_10;
        else
#endif
            pVideoInstInfo->HwVersion = (int)MFC_65;
    } else {
        pVideoInstInfo->HwVersion = version;
    }

    if (exynos_v4l2_g_ctrl(hDec, V4L2_CID_MPEG_MFC_GET_EXT_INFO, &mode) != 0) {
        pVideoInstInfo->specificInfo.dec.bDualDPBSupport = VIDEO_FALSE;
        pVideoInstInfo->specificInfo.dec.bDynamicDPBSupport = VIDEO_FALSE;
        pVideoInstInfo->specificInfo.dec.bLastFrameSupport = VIDEO_FALSE;
        goto EXIT;
    }

    pVideoInstInfo->specificInfo.dec.bSkypeSupport      = (mode & (0x1 << 3))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->specificInfo.dec.bLastFrameSupport  = (mode & (0x1 << 2))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->specificInfo.dec.bDynamicDPBSupport = (mode & (0x1 << 1))? VIDEO_TRUE:VIDEO_FALSE;
#ifndef USE_FORCEFULLY_DISABLE_DUALDPB
    pVideoInstInfo->specificInfo.dec.bDualDPBSupport    = (mode & (0x1 << 0))? VIDEO_TRUE:VIDEO_FALSE;
#endif

    pVideoInstInfo->SwVersion = 0;
#ifdef CID_SUPPORT
    if (pVideoInstInfo->specificInfo.dec.bSkypeSupport == VIDEO_TRUE) {
        int swVersion = 0;

        if (exynos_v4l2_g_ctrl(hDec, V4L2_CID_MPEG_MFC_GET_DRIVER_INFO, &swVersion) != 0) {
            ALOGE("%s: g_ctrl is failed(V4L2_CID_MPEG_MFC_GET_EXTRA_BUFFER_SIZE)", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }

        pVideoInstInfo->SwVersion = (unsigned long long)swVersion;
    }
#endif

    __Set_SupportFormat(pVideoInstInfo);

EXIT:
    if (hDec >= 0)
        exynos_v4l2_close(hDec);

    return ret;
}

int MFC_Exynos_Video_Register_Decoder(
    ExynosVideoDecOps       *pDecOps,
    ExynosVideoDecBufferOps *pInbufOps,
    ExynosVideoDecBufferOps *pOutbufOps)
{
    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;

    if ((pDecOps == NULL) || (pInbufOps == NULL) || (pOutbufOps == NULL)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    defDecOps.nSize = sizeof(defDecOps);
    defInbufOps.nSize = sizeof(defInbufOps);
    defOutbufOps.nSize = sizeof(defOutbufOps);

    memcpy((char *)pDecOps + sizeof(pDecOps->nSize), (char *)&defDecOps + sizeof(defDecOps.nSize),
            pDecOps->nSize - sizeof(pDecOps->nSize));

    memcpy((char *)pInbufOps + sizeof(pInbufOps->nSize), (char *)&defInbufOps + sizeof(defInbufOps.nSize),
            pInbufOps->nSize - sizeof(pInbufOps->nSize));

    memcpy((char *)pOutbufOps + sizeof(pOutbufOps->nSize), (char *)&defOutbufOps + sizeof(defOutbufOps.nSize),
            pOutbufOps->nSize - sizeof(pOutbufOps->nSize));

EXIT:
    return ret;
}
