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
 * @file        ExynosVideoEncoder.c
 * @brief
 * @author      Hyeyeon Chung (hyeon.chung@samsung.com)
 * @author      Jinsung Yang (jsgood.yang@samsung.com)
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     1.0.0
 * @history
 *   2012.02.09: Initial Version
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
#include "ExynosVideoEnc.h"
#include "OMX_Core.h"

/* #define LOG_NDEBUG 0 */
#define LOG_TAG "ExynosVideoEncoder"
#include <utils/Log.h>

#define MAX_CTRL_NUM          107
#define H264_CTRL_NUM         91  /* +7(svc), +4(skype), +1(roi), +4(qp range) */
#define MPEG4_CTRL_NUM        26  /* +4(qp range) */
#define H263_CTRL_NUM         18  /* +2(qp range) */
#define VP8_CTRL_NUM          30  /* +3(svc), +2(qp range) */
#define HEVC_CTRL_NUM         53  /* +7(svc), +1(roi), +4(qp range)*/
#define VP9_CTRL_NUM          29  /* +3(svc), +4(qp range) */

#define MAX_INPUTBUFFER_COUNT 32
#define MAX_OUTPUTBUFFER_COUNT 32

#ifdef USE_DEFINE_H264_SEI_TYPE
enum v4l2_mpeg_video_h264_sei_fp_type {
    V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_CHEKERBOARD    = 0,
    V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_COLUMN         = 1,
    V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_ROW            = 2,
    V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_SIDE_BY_SIDE   = 3,
    V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_TOP_BOTTOM     = 4,
    V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_TEMPORAL       = 5,
};
#endif

static const int vp8_qp_trans[] =
{
    0,   1,  2,  3,  4,  5,  7,  8,
    9,  10, 12, 13, 15, 17, 18, 19,
    20,  21, 23, 24, 25, 26, 27, 28,
    29,  30, 31, 33, 35, 37, 39, 41,
    43,  45, 47, 49, 51, 53, 55, 57,
    59,  61, 64, 67, 70, 73, 76, 79,
    82,  85, 88, 91, 94, 97, 100, 103,
    106, 109, 112, 115, 118, 121, 124, 127,
};

const int vp9_qp_trans[] = {
  0,    4,   8,  12,  16,  20,  24,  28,
  32,   36,  40,  44,  48,  52,  56,  60,
  64,   68,  72,  76,  80,  84,  88,  92,
  96,  100, 104, 108, 112, 116, 120, 124,
  128, 132, 136, 140, 144, 148, 152, 156,
  160, 164, 168, 172, 176, 180, 184, 188,
  192, 196, 200, 204, 208, 212, 216, 220,
  224, 228, 232, 236, 240, 244, 249, 255,
};

#define GET_VALUE(value, min, max) ((value < min)? min:(value > max)? max:value)
#define GET_H264_QP_VALUE(value) GET_VALUE(value, 0, 51)
#define GET_MPEG4_QP_VALUE(value) GET_VALUE(value, 1, 31)
#define GET_H263_QP_VALUE(value) GET_VALUE(value, 1, 31)
#define GET_VP8_QP_VALUE(value) (vp8_qp_trans[GET_VALUE(value, 0, ((int)(sizeof(vp8_qp_trans)/sizeof(int)) - 1))])
#define GET_HEVC_QP_VALUE(value) GET_VALUE(value, 0, 51)
#define GET_VP9_QP_VALUE(value) (vp9_qp_trans[GET_VALUE(value, 1, ((int)(sizeof(vp9_qp_trans)/sizeof(int)) - 1))])

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
#ifdef USE_HEVCENC_SUPPORT
    case VIDEO_CODING_HEVC:
        pixelformat = V4L2_PIX_FMT_HEVC;
        break;
#endif
#ifdef USE_VP9ENC_SUPPORT
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
    case VIDEO_COLORFORMAT_ARGB8888:
        pixelformat = V4L2_PIX_FMT_ARGB32;
        break;
    case VIDEO_COLORFORMAT_BGRA8888:
        pixelformat = V4L2_PIX_FMT_BGR32;
        break;
    case VIDEO_COLORFORMAT_RGBA8888:
        pixelformat = V4L2_PIX_FMT_RGB32X;
        break;
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

    pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12;
    pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV12M;
    pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_NV21M;

    switch ((int)pVideoInstInfo->HwVersion) {
    case MFC_101:  /* NV12, NV21, I420, YV12 */
    case MFC_100:
    case MFC_1010:
    case MFC_1011:
    case MFC_1020:
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        break;
    case MFC_90:  /* NV12, NV21, BGRA, RGBA, I420, YV12, ARGB */
    case MFC_80:
#ifdef USE_HEVC_HWIP
    case HEVC_10:
#endif
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_BGRA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_RGBA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_ARGB8888;
        break;
    case MFC_723:  /* NV12, NV21, BGRA, RGBA, I420, YV12, ARGB, NV12T */
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_BGRA8888;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_RGBA8888;
    case MFC_72:  /* NV12, NV21, I420, YV12, ARGB, NV12T */
    case MFC_77:
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_I420M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_YV12M;
        pVideoInstInfo->supportFormat[nLastIndex++] = VIDEO_COLORFORMAT_ARGB8888;
    case MFC_78:  /* NV12, NV21, NV12T */
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
 * [Encoder OPS] Init
 */
static void *MFC_Encoder_Init(ExynosVideoInstInfo *pVideoInfo)
{
    ExynosVideoEncContext *pCtx     = NULL;
    pthread_mutex_t       *pMutex   = NULL;
    int needCaps = (V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_STREAMING);

    int hIonClient = -1;

    if (pVideoInfo == NULL) {
        ALOGE("%s: bad parameter", __func__);
        goto EXIT_ALLOC_FAIL;
    }

    pCtx = (ExynosVideoEncContext *)malloc(sizeof(*pCtx));
    if (pCtx == NULL) {
        ALOGE("%s: Failed to allocate encoder context buffer", __func__);
        goto EXIT_ALLOC_FAIL;
    }

    memset(pCtx, 0, sizeof(*pCtx));

#ifdef USE_HEVC_HWIP
    if (pVideoInfo->eCodecType == VIDEO_CODING_HEVC) {
        if (pVideoInfo->eSecurityType == VIDEO_SECURE) {
            pCtx->hEnc = exynos_v4l2_open_devname(VIDEO_HEVC_SECURE_ENCODER_NAME, O_RDWR, 0);
        } else {
            pCtx->hEnc = exynos_v4l2_open_devname(VIDEO_HEVC_ENCODER_NAME, O_RDWR, 0);
        }
    } else
#endif
    {
        if (pVideoInfo->eSecurityType == VIDEO_SECURE) {
            pCtx->hEnc = exynos_v4l2_open_devname(VIDEO_SECURE_ENCODER_NAME, O_RDWR, 0);
        } else {
            pCtx->hEnc = exynos_v4l2_open_devname(VIDEO_ENCODER_NAME, O_RDWR, 0);
        }
    }

    if (pCtx->hEnc < 0) {
        ALOGE("%s: Failed to open encoder device", __func__);
        goto EXIT_OPEN_FAIL;
    }

    memcpy(&pCtx->videoInstInfo, pVideoInfo, sizeof(pCtx->videoInstInfo));

    ALOGV("%s: MFC version is %x", __func__, pCtx->videoInstInfo.HwVersion);

    if (!exynos_v4l2_querycap(pCtx->hEnc, needCaps)) {
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

    if (pCtx->videoInstInfo.specificInfo.enc.bTemporalSvcSupport == VIDEO_TRUE) {
        if (ion_alloc_fd(pCtx->hIONHandle, sizeof(TemporalLayerShareBuffer),
                         0, ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED, &(pCtx->nTemporalLayerShareBufferFD)) < 0 ) {
            ALOGE("%s: Failed to ion_alloc_fd for nTemporalLayerShareBufferFD", __func__);
            goto EXIT_QUERYCAP_FAIL;
        }

        pCtx->pTemporalLayerShareBufferAddr = mmap(NULL, sizeof(TemporalLayerShareBuffer),
                                                   PROT_READ | PROT_WRITE, MAP_SHARED,  pCtx->nTemporalLayerShareBufferFD, 0);
        if (pCtx->pTemporalLayerShareBufferAddr == MAP_FAILED) {
            ALOGE("%s: Failed to mmap for nTemporalLayerShareBufferFD", __func__);
            goto EXIT_QUERYCAP_FAIL;
        }

        memset(pCtx->pTemporalLayerShareBufferAddr, 0, sizeof(TemporalLayerShareBuffer));
    }

    if (pCtx->videoInstInfo.specificInfo.enc.bRoiInfoSupport == VIDEO_TRUE) {
        if (ion_alloc_fd(pCtx->hIONHandle, sizeof(RoiInfoShareBuffer),
                         0, ION_HEAP_SYSTEM_MASK, ION_FLAG_CACHED, &(pCtx->nRoiShareBufferFD)) < 0 ) {
            ALOGE("%s: Failed to ion_alloc_fd for nRoiShareBufferFD", __func__);
            goto EXIT_QUERYCAP_FAIL;
        }

        pCtx->pRoiShareBufferAddr = mmap(NULL, sizeof(RoiInfoShareBuffer),
                                                   PROT_READ | PROT_WRITE, MAP_SHARED,  pCtx->nRoiShareBufferFD, 0);
        if (pCtx->pRoiShareBufferAddr == MAP_FAILED) {
            ALOGE("%s: Failed to mmap for nRoiShareBufferFD", __func__);
            goto EXIT_QUERYCAP_FAIL;
        }

        memset(pCtx->pRoiShareBufferAddr, 0, sizeof(RoiInfoShareBuffer));
    }

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

    if (pCtx->videoInstInfo.specificInfo.enc.bTemporalSvcSupport == VIDEO_TRUE) {

        if (pCtx->pTemporalLayerShareBufferAddr != NULL) {
            munmap(pCtx->pTemporalLayerShareBufferAddr, sizeof(TemporalLayerShareBuffer));
            pCtx->pTemporalLayerShareBufferAddr = NULL;
        }

        /* free a ion_buffer */
        if (pCtx->nTemporalLayerShareBufferFD > 0) {
            close(pCtx->nTemporalLayerShareBufferFD);
            pCtx->nTemporalLayerShareBufferFD = -1;
        }
    }

    if (pCtx->videoInstInfo.specificInfo.enc.bRoiInfoSupport == VIDEO_TRUE) {

        if (pCtx->pRoiShareBufferAddr != NULL) {
            munmap(pCtx->pRoiShareBufferAddr, sizeof(RoiInfoShareBuffer));
            pCtx->pRoiShareBufferAddr = NULL;
        }

        /* free a ion_buffer */
        if (pCtx->nRoiShareBufferFD > 0) {
            close(pCtx->nRoiShareBufferFD);
            pCtx->nRoiShareBufferFD = -1;
        }
    }

    /* free a ion_client */
    if (pCtx->hIONHandle > 0) {
        ion_close(pCtx->hIONHandle);
        pCtx->hIONHandle = -1;
    }

    exynos_v4l2_close(pCtx->hEnc);

EXIT_OPEN_FAIL:
    free(pCtx);

EXIT_ALLOC_FAIL:
    return NULL;
}

/*
 * [Encoder OPS] Finalize
 */
static ExynosVideoErrorType MFC_Encoder_Finalize(void *pHandle)
{
    ExynosVideoEncContext *pCtx         = (ExynosVideoEncContext *)pHandle;
    ExynosVideoPlane      *pVideoPlane  = NULL;
    pthread_mutex_t       *pMutex       = NULL;
    ExynosVideoErrorType   ret          = VIDEO_ERROR_NONE;
    int i, j;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->videoInstInfo.specificInfo.enc.bTemporalSvcSupport == VIDEO_TRUE) {
        if (pCtx->pTemporalLayerShareBufferAddr != NULL) {
            munmap(pCtx->pTemporalLayerShareBufferAddr, sizeof(TemporalLayerShareBuffer));
            pCtx->pTemporalLayerShareBufferAddr = NULL;
        }

        /* free a ion_buffer */
        if (pCtx->nTemporalLayerShareBufferFD > 0) {
            close(pCtx->nTemporalLayerShareBufferFD);
            pCtx->nTemporalLayerShareBufferFD = -1;
        }
    }

    if (pCtx->videoInstInfo.specificInfo.enc.bRoiInfoSupport == VIDEO_TRUE) {
        if (pCtx->pRoiShareBufferAddr != NULL) {
            munmap(pCtx->pRoiShareBufferAddr, sizeof(RoiInfoShareBuffer));
            pCtx->pRoiShareBufferAddr = NULL;
        }

        /* free a ion_buffer */
        if (pCtx->nRoiShareBufferFD > 0) {
            close(pCtx->nRoiShareBufferFD);
            pCtx->nRoiShareBufferFD = -1;
        }
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

    if (pCtx->pInbuf != NULL) {
        free(pCtx->pInbuf);
        pCtx->pInbuf = NULL;
    }

    if (pCtx->pOutbuf != NULL) {
        free(pCtx->pOutbuf);
        pCtx->pOutbuf = NULL;
    }

    if (pCtx->hEnc >= 0)
        exynos_v4l2_close(pCtx->hEnc);

    free(pCtx);

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Extended Control
 */
static ExynosVideoErrorType MFC_Encoder_Set_EncParam (
    void                *pHandle,
    ExynosVideoEncParam *pEncParam)
{
    ExynosVideoEncContext     *pCtx         = (ExynosVideoEncContext *)pHandle;
    ExynosVideoEncInitParam   *pInitParam   = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;

    ExynosVideoErrorType       ret          = VIDEO_ERROR_NONE;

    int i;
    struct v4l2_ext_control  ext_ctrl[MAX_CTRL_NUM];
    struct v4l2_ext_controls ext_ctrls;

    if ((pCtx == NULL) || (pEncParam == NULL)) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pInitParam = &pEncParam->initParam;
    pCommonParam = &pEncParam->commonParam;

    /* common parameters */
    ext_ctrl[0].id = V4L2_CID_MPEG_VIDEO_GOP_SIZE;
    ext_ctrl[0].value = pCommonParam->IDRPeriod;
    ext_ctrl[1].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MODE;
    ext_ctrl[1].value = pCommonParam->SliceMode;  /* 0: one, 1: fixed #mb, 2: fixed #bytes */
    ext_ctrl[2].id = V4L2_CID_MPEG_VIDEO_CYCLIC_INTRA_REFRESH_MB;
    ext_ctrl[2].value = pCommonParam->RandomIntraMBRefresh;
    ext_ctrl[3].id = V4L2_CID_MPEG_MFC51_VIDEO_PADDING;
    ext_ctrl[3].value = pCommonParam->PadControlOn;
    ext_ctrl[4].id = V4L2_CID_MPEG_MFC51_VIDEO_PADDING_YUV;
    ext_ctrl[4].value = pCommonParam->CrPadVal;
    ext_ctrl[4].value |= (pCommonParam->CbPadVal << 8);
    ext_ctrl[4].value |= (pCommonParam->LumaPadVal << 16);
    ext_ctrl[5].id = V4L2_CID_MPEG_VIDEO_FRAME_RC_ENABLE;
    ext_ctrl[5].value = pCommonParam->EnableFRMRateControl;
    ext_ctrl[6].id = V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE;
    ext_ctrl[6].value = pCommonParam->EnableMBRateControl;
    ext_ctrl[7].id = V4L2_CID_MPEG_VIDEO_BITRATE;

    /* FIXME temporary fix */
    if (pCommonParam->Bitrate)
        ext_ctrl[7].value = pCommonParam->Bitrate;
    else
        ext_ctrl[7].value = 1; /* just for testing Movie studio */

    /* codec specific parameters */
    switch (pEncParam->eCompressionFormat) {
    case VIDEO_CODING_AVC:
    {
        ExynosVideoEncH264Param *pH264Param = &pEncParam->codecParam.h264;

        /* common parameters but id is depends on codec */
        ext_ctrl[8].id = V4L2_CID_MPEG_VIDEO_H264_I_FRAME_QP;
        ext_ctrl[8].value = pCommonParam->FrameQp;
        ext_ctrl[9].id =  V4L2_CID_MPEG_VIDEO_H264_P_FRAME_QP;
        ext_ctrl[9].value = pCommonParam->FrameQp_P;
        ext_ctrl[10].id =  V4L2_CID_MPEG_VIDEO_H264_MAX_QP;  /* QP range : I frame */
        ext_ctrl[10].value = GET_H264_QP_VALUE(pCommonParam->QpRange.QpMax_I);
        ext_ctrl[11].id = V4L2_CID_MPEG_VIDEO_H264_MIN_QP;
        ext_ctrl[11].value = GET_H264_QP_VALUE(pCommonParam->QpRange.QpMin_I);
        ext_ctrl[12].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[12].value = pCommonParam->CBRPeriodRf;

        /* H.264 specific parameters */
        switch (pCommonParam->SliceMode) {
        case 0:
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[13].value = 1;  /* default */
            ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[14].value = 2800; /* based on MFC6.x */
            break;
        case 1:
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[13].value = pH264Param->SliceArgument;
            ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[14].value = 2800; /* based on MFC6.x */
            break;
        case 2:
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[13].value = 1; /* default */
            ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[14].value = pH264Param->SliceArgument;
            break;
        default:
            break;
        }

        ext_ctrl[15].id = V4L2_CID_MPEG_VIDEO_H264_PROFILE;
        ext_ctrl[15].value = pH264Param->ProfileIDC;
        ext_ctrl[16].id = V4L2_CID_MPEG_VIDEO_H264_LEVEL;
        ext_ctrl[16].value = pH264Param->LevelIDC;
        ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_H264_NUM_REF_PIC_FOR_P;
        ext_ctrl[17].value = pH264Param->NumberRefForPframes;
        /*
         * It should be set using h264Param->NumberBFrames after being handled by appl.
         */
        ext_ctrl[18].id =  V4L2_CID_MPEG_VIDEO_B_FRAMES;
        ext_ctrl[18].value = pH264Param->NumberBFrames;
        ext_ctrl[19].id = V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_MODE;
        ext_ctrl[19].value = pH264Param->LoopFilterDisable;
        ext_ctrl[20].id = V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_ALPHA;
        ext_ctrl[20].value = pH264Param->LoopFilterAlphaC0Offset;
        ext_ctrl[21].id = V4L2_CID_MPEG_VIDEO_H264_LOOP_FILTER_BETA;
        ext_ctrl[21].value = pH264Param->LoopFilterBetaOffset;
        ext_ctrl[22].id = V4L2_CID_MPEG_VIDEO_H264_ENTROPY_MODE;
        ext_ctrl[22].value = pH264Param->SymbolMode;
        ext_ctrl[23].id = V4L2_CID_MPEG_MFC51_VIDEO_H264_INTERLACE;
        ext_ctrl[23].value = pH264Param->PictureInterlace;
        ext_ctrl[24].id = V4L2_CID_MPEG_VIDEO_H264_8X8_TRANSFORM;
        ext_ctrl[24].value = pH264Param->Transform8x8Mode;
        ext_ctrl[25].id = V4L2_CID_MPEG_MFC51_VIDEO_H264_RC_FRAME_RATE;
        ext_ctrl[25].value = pH264Param->FrameRate;
        ext_ctrl[26].id =  V4L2_CID_MPEG_VIDEO_H264_B_FRAME_QP;
        ext_ctrl[26].value = pH264Param->FrameQp_B;
        ext_ctrl[27].id =  V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_DARK;
        ext_ctrl[27].value = pH264Param->DarkDisable;
        ext_ctrl[28].id =  V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_SMOOTH;
        ext_ctrl[28].value = pH264Param->SmoothDisable;
        ext_ctrl[29].id =  V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_STATIC;
        ext_ctrl[29].value = pH264Param->StaticDisable;
        ext_ctrl[30].id =  V4L2_CID_MPEG_MFC51_VIDEO_H264_ADAPTIVE_RC_ACTIVITY;
        ext_ctrl[30].value = pH264Param->ActivityDisable;

        /* doesn't have to be set */
        ext_ctrl[31].id = V4L2_CID_MPEG_VIDEO_GOP_CLOSURE;
        ext_ctrl[31].value = 1;
        ext_ctrl[32].id = V4L2_CID_MPEG_VIDEO_H264_I_PERIOD;
        ext_ctrl[32].value = 0;
        ext_ctrl[33].id = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[33].value = 0;
        ext_ctrl[34].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;

        if (pH264Param->HeaderWithIFrame == 0) {
            /* default */
            ext_ctrl[34].value = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE;              /* 0: seperated header */
        } else {
            ext_ctrl[34].value = V4L2_MPEG_VIDEO_HEADER_MODE_JOINED_WITH_1ST_FRAME; /* 1: header + first frame */
        }
        ext_ctrl[35].id =  V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_ENABLE;
        ext_ctrl[35].value = pH264Param->SarEnable;
        ext_ctrl[36].id = V4L2_CID_MPEG_VIDEO_H264_VUI_SAR_IDC;
        ext_ctrl[36].value = pH264Param->SarIndex;
        ext_ctrl[37].id = V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_WIDTH;
        ext_ctrl[37].value = pH264Param->SarWidth;
        ext_ctrl[38].id =  V4L2_CID_MPEG_VIDEO_H264_VUI_EXT_SAR_HEIGHT;
        ext_ctrl[38].value = pH264Param->SarHeight;

        /* Initial parameters : Frame Skip */
        switch (pInitParam->FrameSkip) {
        case VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT:
            ext_ctrl[39].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[39].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
            break;
        case VIDEO_FRAME_SKIP_MODE_BUF_LIMIT:
            ext_ctrl[39].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[39].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
            break;
        default:
            /* VIDEO_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[39].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[39].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
            break;
        }

        /* SVC is not supported yet */
        ext_ctrl[40].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING;
        ext_ctrl[40].value = 0;
        switch (pH264Param->HierarType) {
        case 1:
            ext_ctrl[41].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_TYPE;
            ext_ctrl[41].value = V4L2_MPEG_VIDEO_H264_HIERARCHICAL_CODING_B;
            break;
        case 0:
        default:
            ext_ctrl[41].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_TYPE;
            ext_ctrl[41].value = V4L2_MPEG_VIDEO_H264_HIERARCHICAL_CODING_P;
            break;
        }
        ext_ctrl[42].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER;
        ext_ctrl[42].value = pH264Param->TemporalSVC.nTemporalLayerCount;
        ext_ctrl[43].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[43].value = (0 << 16 | 29);
        ext_ctrl[44].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[44].value = (1 << 16 | 29);
        ext_ctrl[45].id =  V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[45].value = (2 << 16 | 29);

        ext_ctrl[46].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FRAME_PACKING;
        ext_ctrl[46].value = 0;
        ext_ctrl[47].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_CURRENT_FRAME_0;
        ext_ctrl[47].value = 0;
        ext_ctrl[48].id =  V4L2_CID_MPEG_VIDEO_H264_SEI_FP_ARRANGEMENT_TYPE;
        ext_ctrl[48].value = V4L2_MPEG_VIDEO_H264_SEI_FP_TYPE_SIDE_BY_SIDE;

        /* FMO is not supported yet */
        ext_ctrl[49].id =  V4L2_CID_MPEG_VIDEO_H264_FMO;
        ext_ctrl[49].value = 0;
        ext_ctrl[50].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_MAP_TYPE;
        ext_ctrl[50].value = V4L2_MPEG_VIDEO_H264_FMO_MAP_TYPE_INTERLEAVED_SLICES;
        ext_ctrl[51].id = V4L2_CID_MPEG_VIDEO_H264_FMO_SLICE_GROUP;
        ext_ctrl[51].value = 4;
        ext_ctrl[52].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH;
        ext_ctrl[52].value = (0 << 30 | 0);
        ext_ctrl[53].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH;
        ext_ctrl[53].value = (1 << 30 | 0);
        ext_ctrl[54].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH;
        ext_ctrl[54].value = (2 << 30 | 0);
        ext_ctrl[55].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_RUN_LENGTH;
        ext_ctrl[55].value = (3 << 30 | 0);
        ext_ctrl[56].id =  V4L2_CID_MPEG_VIDEO_H264_FMO_CHANGE_DIRECTION;
        ext_ctrl[56].value = V4L2_MPEG_VIDEO_H264_FMO_CHANGE_DIR_RIGHT;
        ext_ctrl[57].id = V4L2_CID_MPEG_VIDEO_H264_FMO_CHANGE_RATE;
        ext_ctrl[57].value = 0;

        /* ASO is not supported yet */
        ext_ctrl[58].id =  V4L2_CID_MPEG_VIDEO_H264_ASO;
        ext_ctrl[58].value = 0;
        for (i = 0; i < 32; i++) {
            ext_ctrl[59 + i].id =  V4L2_CID_MPEG_VIDEO_H264_ASO_SLICE_ORDER;
            ext_ctrl[59 + i].value = (i << 16 | 0);
        }
        ext_ctrls.count = H264_CTRL_NUM;

        if (pCtx->videoInstInfo.specificInfo.enc.bTemporalSvcSupport == VIDEO_TRUE) {
            i = ext_ctrls.count;
            ext_ctrl[i].id          = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT0;
            ext_ctrl[i].value       = pH264Param->TemporalSVC.nTemporalLayerBitrateRatio[0];
            ext_ctrl[i + 1].id      = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT1;
            ext_ctrl[i + 1].value   = pH264Param->TemporalSVC.nTemporalLayerBitrateRatio[1];
            ext_ctrl[i + 2].id      = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT2;
            ext_ctrl[i + 2].value   = pH264Param->TemporalSVC.nTemporalLayerBitrateRatio[2];
            ext_ctrl[i + 3].id      = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT3;
            ext_ctrl[i + 3].value   = pH264Param->TemporalSVC.nTemporalLayerBitrateRatio[3];
            ext_ctrl[i + 4].id      = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT4;
            ext_ctrl[i + 4].value   = pH264Param->TemporalSVC.nTemporalLayerBitrateRatio[4];
            ext_ctrl[i + 5].id     = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT5;
            ext_ctrl[i + 5].value  = pH264Param->TemporalSVC.nTemporalLayerBitrateRatio[5];
            ext_ctrl[i + 6].id     = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_BIT6;
            ext_ctrl[i + 6].value  = pH264Param->TemporalSVC.nTemporalLayerBitrateRatio[6];
            ext_ctrls.count += 7;
        }

#ifdef CID_SUPPORT
        if (pCtx->videoInstInfo.specificInfo.enc.bSkypeSupport == VIDEO_TRUE) {
            i = ext_ctrls.count;

            /* VUI RESRICTION ENABLE */
            ext_ctrl[i].id          = V4L2_CID_MPEG_MFC_H264_VUI_RESTRICTION_ENABLE;
            ext_ctrl[i].value       = pH264Param->VuiRestrictionEnable;

            /* H264 ENABLE LTR */
            ext_ctrl[i + 1].id      = V4L2_CID_MPEG_MFC_H264_ENABLE_LTR;
            ext_ctrl[i + 1].value   = pH264Param->LTREnable;

            /* FRAME LEVEL QP ENABLE */
            ext_ctrl[i + 2].id      = V4L2_CID_MPEG_MFC_CONFIG_QP_ENABLE;
            ext_ctrl[i + 2].value   = pCommonParam->EnableFRMQpControl;

            /* CONFIG QP VALUE */
            ext_ctrl[i + 3].id      = V4L2_CID_MPEG_MFC_CONFIG_QP;
            ext_ctrl[i + 3].value   = pCommonParam->FrameQp;

            ext_ctrls.count += 4;
        }
#endif
#ifdef USE_MFC_MEDIA
        if (pCtx->videoInstInfo.specificInfo.enc.bRoiInfoSupport == VIDEO_TRUE) {
            i = ext_ctrls.count;
            ext_ctrl[i].id      =  V4L2_CID_MPEG_VIDEO_ROI_ENABLE;
            ext_ctrl[i].value   = pH264Param->ROIEnable;

            ext_ctrls.count += 1;
        }
#endif
        /* optional : if these are not set, set value are same as I frame */
        if (pCtx->videoInstInfo.specificInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            i = ext_ctrls.count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_H264_MIN_QP_P;  /* P frame */
            ext_ctrl[i].value = GET_H264_QP_VALUE(pCommonParam->QpRange.QpMin_P);
            ext_ctrl[i + 1].id =  V4L2_CID_MPEG_VIDEO_H264_MAX_QP_P;
            ext_ctrl[i + 1].value = GET_H264_QP_VALUE(pCommonParam->QpRange.QpMax_P);
            ext_ctrl[i + 2].id = V4L2_CID_MPEG_VIDEO_H264_MIN_QP_B;  /* B frame */
            ext_ctrl[i + 2].value = GET_H264_QP_VALUE(pCommonParam->QpRange.QpMin_B);
            ext_ctrl[i + 3].id =  V4L2_CID_MPEG_VIDEO_H264_MAX_QP_B;
            ext_ctrl[i + 3].value = GET_H264_QP_VALUE(pCommonParam->QpRange.QpMax_B);

            ext_ctrls.count += 4;
        }
        break;
    }

    case VIDEO_CODING_MPEG4:
    {
        ExynosVideoEncMpeg4Param *pMpeg4Param = &pEncParam->codecParam.mpeg4;

        /* common parameters but id is depends on codec */
        ext_ctrl[8].id = V4L2_CID_MPEG_VIDEO_MPEG4_I_FRAME_QP;
        ext_ctrl[8].value = pCommonParam->FrameQp;
        ext_ctrl[9].id =  V4L2_CID_MPEG_VIDEO_MPEG4_P_FRAME_QP;
        ext_ctrl[9].value = pCommonParam->FrameQp_P;
        ext_ctrl[10].id =  V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP;  /* I frame */
        ext_ctrl[10].value = GET_MPEG4_QP_VALUE(pCommonParam->QpRange.QpMax_I);
        ext_ctrl[11].id = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP;
        ext_ctrl[11].value = GET_MPEG4_QP_VALUE(pCommonParam->QpRange.QpMin_I);
        ext_ctrl[12].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[12].value = pCommonParam->CBRPeriodRf;

        /* MPEG4 specific parameters */
        switch (pCommonParam->SliceMode) {
        case 0:
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[13].value = 1;  /* default */
            ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[14].value = 2800; /* based on MFC6.x */
            break;
        case 1:
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[13].value = pMpeg4Param->SliceArgument;
            ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[14].value = 2800; /* based on MFC6.x */
            break;
        case 2:
            ext_ctrl[13].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_MB;
            ext_ctrl[13].value = 1; /* default */
            ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_MULTI_SLICE_MAX_BYTES;
            ext_ctrl[14].value = pMpeg4Param->SliceArgument;
            break;
        default:
            break;
        }

        ext_ctrl[15].id = V4L2_CID_MPEG_VIDEO_MPEG4_PROFILE;
        ext_ctrl[15].value = pMpeg4Param->ProfileIDC;
        ext_ctrl[16].id = V4L2_CID_MPEG_VIDEO_MPEG4_LEVEL;
        ext_ctrl[16].value = pMpeg4Param->LevelIDC;
        ext_ctrl[17].id = V4L2_CID_MPEG_VIDEO_MPEG4_QPEL;
        ext_ctrl[17].value = pMpeg4Param->DisableQpelME;

        /*
         * It should be set using mpeg4Param->NumberBFrames after being handled by appl.
         */
        ext_ctrl[18].id =  V4L2_CID_MPEG_VIDEO_B_FRAMES;
        ext_ctrl[18].value = pMpeg4Param->NumberBFrames;

        ext_ctrl[19].id = V4L2_CID_MPEG_MFC51_VIDEO_MPEG4_VOP_TIME_RES;
        ext_ctrl[19].value = pMpeg4Param->TimeIncreamentRes;
        ext_ctrl[20].id = V4L2_CID_MPEG_MFC51_VIDEO_MPEG4_VOP_FRM_DELTA;
        ext_ctrl[20].value = pMpeg4Param->VopTimeIncreament;
        ext_ctrl[21].id =  V4L2_CID_MPEG_VIDEO_MPEG4_B_FRAME_QP;
        ext_ctrl[21].value = pMpeg4Param->FrameQp_B;
        ext_ctrl[22].id = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[22].value = 0;
        ext_ctrl[23].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;
        ext_ctrl[23].value = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE;
        ext_ctrl[24].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_FIXED_TARGET_BIT;
        ext_ctrl[24].value = 1;

        /* Initial parameters : Frame Skip */
        switch (pInitParam->FrameSkip) {
        case VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT:
            ext_ctrl[25].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[25].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
            break;
        case VIDEO_FRAME_SKIP_MODE_BUF_LIMIT:
            ext_ctrl[25].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[25].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
            break;
        default:
            /* VIDEO_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[25].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[25].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
            break;
        }
        ext_ctrls.count = MPEG4_CTRL_NUM;

        /* optional : if these are not set, set value are same as I frame */
        if (pCtx->videoInstInfo.specificInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            i = ext_ctrls.count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_P;  /* P frame */
            ext_ctrl[i].value = GET_MPEG4_QP_VALUE(pCommonParam->QpRange.QpMin_P);
            ext_ctrl[i + 1].id =  V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_P;
            ext_ctrl[i + 1].value = GET_MPEG4_QP_VALUE(pCommonParam->QpRange.QpMax_P);
            ext_ctrl[i + 2].id = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_B;  /* B frame */
            ext_ctrl[i + 2].value = GET_MPEG4_QP_VALUE(pCommonParam->QpRange.QpMin_B);
            ext_ctrl[i + 3].id =  V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_B;
            ext_ctrl[i + 3].value = GET_MPEG4_QP_VALUE(pCommonParam->QpRange.QpMax_B);

            ext_ctrls.count += 4;
        }
        break;
    }

    case VIDEO_CODING_H263:
    {
        ExynosVideoEncH263Param *pH263Param = &pEncParam->codecParam.h263;

        /* common parameters but id is depends on codec */
        ext_ctrl[8].id = V4L2_CID_MPEG_VIDEO_H263_I_FRAME_QP;
        ext_ctrl[8].value = pCommonParam->FrameQp;
        ext_ctrl[9].id =  V4L2_CID_MPEG_VIDEO_H263_P_FRAME_QP;
        ext_ctrl[9].value = pCommonParam->FrameQp_P;
        ext_ctrl[10].id =  V4L2_CID_MPEG_VIDEO_H263_MAX_QP;  /* I frame */
        ext_ctrl[10].value = GET_H263_QP_VALUE(pCommonParam->QpRange.QpMax_I);
        ext_ctrl[11].id = V4L2_CID_MPEG_VIDEO_H263_MIN_QP;
        ext_ctrl[11].value = GET_H263_QP_VALUE(pCommonParam->QpRange.QpMin_I);
        ext_ctrl[12].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[12].value = pCommonParam->CBRPeriodRf;

        /* H263 specific parameters */
        ext_ctrl[13].id = V4L2_CID_MPEG_MFC51_VIDEO_H263_RC_FRAME_RATE;
        ext_ctrl[13].value = pH263Param->FrameRate;
        ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[14].value = 0;
        ext_ctrl[15].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;
        ext_ctrl[15].value = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE;
        ext_ctrl[16].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_FIXED_TARGET_BIT;
        ext_ctrl[16].value = 1;

        /* Initial parameters : Frame Skip */
        switch (pInitParam->FrameSkip) {
        case VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT:
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
            break;
        case VIDEO_FRAME_SKIP_MODE_BUF_LIMIT:
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
            break;
        default:
            /* VIDEO_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
            break;
        }
        ext_ctrls.count = H263_CTRL_NUM;

        /* optional : if these are not set, set value are same as I frame */
        if (pCtx->videoInstInfo.specificInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            i = ext_ctrls.count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_H263_MIN_QP_P;  /* P frame */
            ext_ctrl[i].value = GET_H263_QP_VALUE(pCommonParam->QpRange.QpMin_P);
            ext_ctrl[i + 1].id =  V4L2_CID_MPEG_VIDEO_H263_MAX_QP_P;
            ext_ctrl[i + 1].value = GET_H263_QP_VALUE(pCommonParam->QpRange.QpMax_P);

            ext_ctrls.count += 2;
        }
        break;
    }

    case VIDEO_CODING_VP8:
    {
        ExynosVideoEncVp8Param *pVp8Param = &pEncParam->codecParam.vp8;

        /* common parameters but id is depends on codec */
        ext_ctrl[8].id = V4L2_CID_MPEG_VIDEO_VP8_I_FRAME_QP;
        ext_ctrl[8].value = pCommonParam->FrameQp;
        ext_ctrl[9].id =  V4L2_CID_MPEG_VIDEO_VP8_P_FRAME_QP;
        ext_ctrl[9].value = pCommonParam->FrameQp_P;
        ext_ctrl[10].id =  V4L2_CID_MPEG_VIDEO_VP8_MAX_QP;  /* I frame */
        ext_ctrl[10].value = GET_VP8_QP_VALUE(pCommonParam->QpRange.QpMax_I);
        ext_ctrl[11].id = V4L2_CID_MPEG_VIDEO_VP8_MIN_QP;
        ext_ctrl[11].value = GET_VP8_QP_VALUE(pCommonParam->QpRange.QpMin_I);
        ext_ctrl[12].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[12].value = pCommonParam->CBRPeriodRf;

        /* VP8 specific parameters */
        ext_ctrl[13].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_RC_FRAME_RATE;
        ext_ctrl[13].value = pVp8Param->FrameRate;
        ext_ctrl[14].id = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[14].value = 0;
        ext_ctrl[15].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;
        ext_ctrl[15].value = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE;
        ext_ctrl[16].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_FIXED_TARGET_BIT;
        ext_ctrl[16].value = 1;

        /* Initial parameters : Frame Skip */
        switch (pInitParam->FrameSkip) {
        case VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT:
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
            break;
        case VIDEO_FRAME_SKIP_MODE_BUF_LIMIT:
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
            break;
        default:
            /* VIDEO_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[17].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
            break;
        }

        ext_ctrl[18].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_VERSION;
        ext_ctrl[18].value = pVp8Param->Vp8Version;

        ext_ctrl[19].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_NUM_OF_PARTITIONS;
        ext_ctrl[19].value = pVp8Param->Vp8NumberOfPartitions;

        ext_ctrl[20].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_FILTER_LEVEL;
        ext_ctrl[20].value = pVp8Param->Vp8FilterLevel;

        ext_ctrl[21].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_FILTER_SHARPNESS;
        ext_ctrl[21].value = pVp8Param->Vp8FilterSharpness;

        ext_ctrl[22].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_GOLDEN_FRAMESEL;
        ext_ctrl[22].value = pVp8Param->Vp8GoldenFrameSel;

        ext_ctrl[23].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_HIERARCHY_QP_ENABLE;
        ext_ctrl[23].value = 0;

        ext_ctrl[24].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_HIERARCHY_QP_LAYER0;
        ext_ctrl[24].value = (0 << 16 | 37);

        ext_ctrl[25].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_HIERARCHY_QP_LAYER1;
        ext_ctrl[25].value = (1 << 16 | 37);

        ext_ctrl[26].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_HIERARCHY_QP_LAYER2;
        ext_ctrl[26].value = (2 << 16 | 37);

        ext_ctrl[27].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_REF_NUMBER_FOR_PFRAMES;
        ext_ctrl[27].value = pVp8Param->RefNumberForPFrame;

        ext_ctrl[28].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_DISABLE_INTRA_MD4X4;
        ext_ctrl[28].value = pVp8Param->DisableIntraMd4x4;

        ext_ctrl[29].id = V4L2_CID_MPEG_MFC70_VIDEO_VP8_NUM_TEMPORAL_LAYER;
        ext_ctrl[29].value = pVp8Param->TemporalSVC.nTemporalLayerCount;
        ext_ctrls.count = VP8_CTRL_NUM;

        if (pCtx->videoInstInfo.specificInfo.enc.bTemporalSvcSupport == VIDEO_TRUE) {
            i = ext_ctrls.count;
            ext_ctrl[i].id         = V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_BIT0;
            ext_ctrl[i].value      = pVp8Param->TemporalSVC.nTemporalLayerBitrateRatio[0];
            ext_ctrl[i + 1].id     = V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_BIT1;
            ext_ctrl[i + 1].value  = pVp8Param->TemporalSVC.nTemporalLayerBitrateRatio[1];
            ext_ctrl[i + 2].id     = V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_BIT2;
            ext_ctrl[i + 2].value  = pVp8Param->TemporalSVC.nTemporalLayerBitrateRatio[2];

            ext_ctrls.count += 3;
        }

        /* optional : if these are not set, set value are same as I frame */
        if (pCtx->videoInstInfo.specificInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            i = ext_ctrls.count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_VP8_MIN_QP_P;  /* P frame */
            ext_ctrl[i].value = GET_VP8_QP_VALUE(pCommonParam->QpRange.QpMin_P);
            ext_ctrl[i + 1].id =  V4L2_CID_MPEG_VIDEO_VP8_MAX_QP_P;
            ext_ctrl[i + 1].value = GET_VP8_QP_VALUE(pCommonParam->QpRange.QpMax_P);

            ext_ctrls.count += 2;
        }
        break;
    }
#ifdef USE_HEVCENC_SUPPORT
    case VIDEO_CODING_HEVC:
    {
        ExynosVideoEncHevcParam *pHevcParam = &pEncParam->codecParam.hevc;

        /* common parameters but id is depends on codec */
        ext_ctrl[8].id = V4L2_CID_MPEG_VIDEO_HEVC_I_FRAME_QP;
        ext_ctrl[8].value = pCommonParam->FrameQp;
        ext_ctrl[9].id =  V4L2_CID_MPEG_VIDEO_HEVC_P_FRAME_QP;
        ext_ctrl[9].value = pCommonParam->FrameQp_P;
        ext_ctrl[10].id =  V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP;  /* I frame */
        ext_ctrl[10].value = GET_HEVC_QP_VALUE(pCommonParam->QpRange.QpMax_I);
        ext_ctrl[11].id = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP;
        ext_ctrl[11].value = GET_HEVC_QP_VALUE(pCommonParam->QpRange.QpMin_I);
        ext_ctrl[12].id = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[12].value = pCommonParam->CBRPeriodRf;

        /* HEVC specific parameters */
        ext_ctrl[13].id =  V4L2_CID_MPEG_VIDEO_HEVC_B_FRAME_QP;
        ext_ctrl[13].value = pHevcParam->FrameQp_B;
        ext_ctrl[14].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_RC_FRAME_RATE;
        ext_ctrl[14].value = pHevcParam->FrameRate;

        /* Initial parameters : Frame Skip */
        switch (pInitParam->FrameSkip) {
        case VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT:
            ext_ctrl[15].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[15].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
            break;
        case VIDEO_FRAME_SKIP_MODE_BUF_LIMIT:
            ext_ctrl[15].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[15].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
            break;
        default:
            /* VIDEO_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[15].id = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[15].value = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
            break;
        }

        ext_ctrl[16].id = V4L2_CID_MPEG_VIDEO_HEVC_PROFILE;
        ext_ctrl[16].value = pHevcParam->ProfileIDC;

        ext_ctrl[17].id = V4L2_CID_MPEG_VIDEO_HEVC_LEVEL;
        ext_ctrl[17].value = pHevcParam->LevelIDC;

        ext_ctrl[18].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_TIER_FLAG;
        ext_ctrl[18].value = pHevcParam->TierIDC;

        ext_ctrl[19].id =  V4L2_CID_MPEG_VIDEO_B_FRAMES;
        ext_ctrl[19].value = pHevcParam->NumberBFrames;

        ext_ctrl[20].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_MAX_PARTITION_DEPTH;
        ext_ctrl[20].value = pHevcParam->MaxPartitionDepth;

        ext_ctrl[21].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_REF_NUMBER_FOR_PFRAMES;
        ext_ctrl[21].value = pHevcParam->NumberRefForPframes;

        ext_ctrl[22].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LF_DISABLE;
        ext_ctrl[22].value = pHevcParam->LoopFilterDisable;

        ext_ctrl[23].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LF_SLICE_BOUNDARY;
        ext_ctrl[23].value = pHevcParam->LoopFilterSliceFlag;

        ext_ctrl[24].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LF_TC_OFFSET_DIV2;
        ext_ctrl[24].value = pHevcParam->LoopFilterTcOffset;

        ext_ctrl[25].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LF_BETA_OFFSET_DIV2;
        ext_ctrl[25].value = pHevcParam->LoopFilterBetaOffset;

        ext_ctrl[26].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LTR_ENABLE;
        ext_ctrl[26].value = pHevcParam->LongtermRefEnable;

        ext_ctrl[27].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_USER_REF;
        ext_ctrl[27].value = pHevcParam->LongtermUserRef;

        ext_ctrl[28].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_STORE_REF;
        ext_ctrl[28].value = pHevcParam->LongtermStoreRef;

        /* should be set V4L2_CID_MPEG_VIDEO_MB_RC_ENABLE first */
        ext_ctrl[29].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_ADAPTIVE_RC_DARK;
        ext_ctrl[29].value = pHevcParam->DarkDisable;

        ext_ctrl[30].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_ADAPTIVE_RC_SMOOTH;
        ext_ctrl[30].value = pHevcParam->SmoothDisable;

        ext_ctrl[31].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_ADAPTIVE_RC_STATIC;
        ext_ctrl[31].value = pHevcParam->StaticDisable;

        ext_ctrl[32].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_ADAPTIVE_RC_ACTIVITY;
        ext_ctrl[32].value = pHevcParam->ActivityDisable;

        /* doesn't have to be set */
        ext_ctrl[33].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_REFRESH_TYPE;
        ext_ctrl[33].value = 2;
        ext_ctrl[34].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_REFRESH_PERIOD;
        ext_ctrl[34].value = 0;
        ext_ctrl[35].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_LOSSLESS_CU_ENABLE;
        ext_ctrl[35].value = 0;
        ext_ctrl[36].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_CONST_INTRA_PRED_ENABLE;
        ext_ctrl[36].value = 0;
        ext_ctrl[37].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_WAVEFRONT_ENABLE;
        ext_ctrl[37].value = 0;
        ext_ctrl[38].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_SIGN_DATA_HIDING;
        ext_ctrl[38].value = 1;
        ext_ctrl[39].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_GENERAL_PB_ENABLE;
        ext_ctrl[39].value = 1;
        ext_ctrl[40].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_TEMPORAL_ID_ENABLE;
        ext_ctrl[40].value = 1;
        ext_ctrl[41].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_STRONG_SMOTHING_FLAG;
        ext_ctrl[41].value = 1;
        ext_ctrl[42].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_MAX_NUM_MERGE_MV_MINUS1;
        ext_ctrl[42].value = 4;
        ext_ctrl[43].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_DISABLE_INTRA_PU_SPLIT;
        ext_ctrl[43].value = 0;
        ext_ctrl[44].id = V4L2_CID_MPEG_MFC90_VIDEO_HEVC_DISABLE_TMV_PREDICTION;
        ext_ctrl[44].value = 0;
        ext_ctrl[45].id = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[45].value = 0;
        ext_ctrl[46].id = V4L2_CID_MPEG_VIDEO_HEADER_MODE;
        ext_ctrl[46].value = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE;

        /* SVC is not supported yet */
        ext_ctrl[47].id =  V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_QP_ENABLE;
        ext_ctrl[47].value = 0;
        ext_ctrl[48].id =  V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_TYPE;
        ext_ctrl[48].value = V4L2_MPEG_VIDEO_H264_HIERARCHICAL_CODING_P;
        ext_ctrl[49].id =  V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER;
        ext_ctrl[49].value = pHevcParam->TemporalSVC.nTemporalLayerCount;
        ext_ctrl[50].id =  V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[50].value = (0 << 16 | 29);
        ext_ctrl[51].id =  V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[51].value = (1 << 16 | 29);
        ext_ctrl[52].id =  V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[52].value = (2 << 16 | 29);
        ext_ctrls.count = HEVC_CTRL_NUM;

#ifdef CID_SUPPORT
        if (pCtx->videoInstInfo.specificInfo.enc.bTemporalSvcSupport == VIDEO_TRUE) {
            i = ext_ctrls.count;
            ext_ctrl[i].id         = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT0;
            ext_ctrl[i].value      = pHevcParam->TemporalSVC.nTemporalLayerBitrateRatio[0];
            ext_ctrl[i + 1].id     = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT1;
            ext_ctrl[i + 1].value  = pHevcParam->TemporalSVC.nTemporalLayerBitrateRatio[1];
            ext_ctrl[i + 2].id     = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT2;
            ext_ctrl[i + 2].value  = pHevcParam->TemporalSVC.nTemporalLayerBitrateRatio[2];
            ext_ctrl[i + 3].id     = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT3;
            ext_ctrl[i + 3].value  = pHevcParam->TemporalSVC.nTemporalLayerBitrateRatio[3];
            ext_ctrl[i + 4].id     = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT4;
            ext_ctrl[i + 4].value  = pHevcParam->TemporalSVC.nTemporalLayerBitrateRatio[4];
            ext_ctrl[i + 5].id     = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT5;
            ext_ctrl[i + 5].value  = pHevcParam->TemporalSVC.nTemporalLayerBitrateRatio[5];
            ext_ctrl[i + 6].id     = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_BIT6;
            ext_ctrl[i + 6].value  = pHevcParam->TemporalSVC.nTemporalLayerBitrateRatio[6];
            ext_ctrls.count += 7;
        }
#endif
#ifdef USE_MFC_MEDIA
        if (pCtx->videoInstInfo.specificInfo.enc.bRoiInfoSupport == VIDEO_TRUE) {
            i = ext_ctrls.count;
            ext_ctrl[i].id     =  V4L2_CID_MPEG_VIDEO_ROI_ENABLE;
            ext_ctrl[i].value  = pHevcParam->ROIEnable;

            ext_ctrls.count += 1;
        }
#endif

        /* optional : if these are not set, set value are same as I frame */
        if (pCtx->videoInstInfo.specificInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            i = ext_ctrls.count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_P;  /* P frame */
            ext_ctrl[i].value = GET_HEVC_QP_VALUE(pCommonParam->QpRange.QpMin_P);
            ext_ctrl[i + 1].id =  V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_P;
            ext_ctrl[i + 1].value = GET_HEVC_QP_VALUE(pCommonParam->QpRange.QpMax_P);
            ext_ctrl[i + 2].id = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_B;  /* B frame */
            ext_ctrl[i + 2].value = GET_HEVC_QP_VALUE(pCommonParam->QpRange.QpMin_B);
            ext_ctrl[i + 3].id =  V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_B;
            ext_ctrl[i + 3].value = GET_HEVC_QP_VALUE(pCommonParam->QpRange.QpMax_B);

            ext_ctrls.count += 4;
        }
        break;
    }
#endif
#ifdef USE_VP9ENC_SUPPORT
    case VIDEO_CODING_VP9:
    {
        ExynosVideoEncVp9Param *pVp9Param = &pEncParam->codecParam.vp9;

        /* VP9 specific parameters */
        ext_ctrl[8].id      = V4L2_CID_MPEG_VIDEO_VP9_I_FRAME_QP;
        ext_ctrl[8].value   = pCommonParam->FrameQp;

        ext_ctrl[9].id      = V4L2_CID_MPEG_VIDEO_VP9_P_FRAME_QP;
        ext_ctrl[9].value   = pCommonParam->FrameQp_P;

        ext_ctrl[10].id     = V4L2_CID_MPEG_VIDEO_VP9_MAX_QP;  /* I frame */
        ext_ctrl[10].value  = GET_VP9_QP_VALUE(pCommonParam->QpRange.QpMax_I);

        ext_ctrl[11].id     = V4L2_CID_MPEG_VIDEO_VP9_MIN_QP;
        ext_ctrl[11].value  = GET_VP9_QP_VALUE(pCommonParam->QpRange.QpMin_I);

        ext_ctrl[12].id     = V4L2_CID_MPEG_MFC51_VIDEO_RC_REACTION_COEFF;
        ext_ctrl[12].value  = pCommonParam->CBRPeriodRf;

        ext_ctrl[13].id     = V4L2_CID_MPEG_VIDEO_VP9_RC_FRAME_RATE;
        ext_ctrl[13].value  = pVp9Param->FrameRate;

        ext_ctrl[14].id     = V4L2_CID_MPEG_VIDEO_VBV_SIZE;
        ext_ctrl[14].value  = 0;

        ext_ctrl[15].id     = V4L2_CID_MPEG_VIDEO_HEADER_MODE;
        ext_ctrl[15].value  = V4L2_MPEG_VIDEO_HEADER_MODE_SEPARATE;

        ext_ctrl[16].id     = V4L2_CID_MPEG_VIDEO_VP9_DISABLE_INTRA_PU_SPLIT;
        ext_ctrl[16].value  = 0;

        /* Initial parameters : Frame Skip */
        switch (pInitParam->FrameSkip) {
        case VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT:
            ext_ctrl[17].id     = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value  = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_LEVEL_LIMIT;
            break;
        case VIDEO_FRAME_SKIP_MODE_BUF_LIMIT:
            ext_ctrl[17].id     = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value  = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_BUF_LIMIT;
            break;
        default:
            /* VIDEO_FRAME_SKIP_MODE_DISABLE (default) */
            ext_ctrl[17].id     = V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE;
            ext_ctrl[17].value  = V4L2_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE_DISABLED;
            break;
        }

        ext_ctrl[18].id     = V4L2_CID_MPEG_VIDEO_VP9_VERSION;
        ext_ctrl[18].value  = pVp9Param->Vp9Version;

        ext_ctrl[19].id     = V4L2_CID_MPEG_VIDEO_VP9_MAX_PARTITION_DEPTH;
        ext_ctrl[19].value  = pVp9Param->MaxPartitionDepth;

        ext_ctrl[20].id     = V4L2_CID_MPEG_VIDEO_VP9_GOLDEN_FRAMESEL;
        ext_ctrl[20].value  = pVp9Param->Vp9GoldenFrameSel;

        ext_ctrl[21].id     = V4L2_CID_MPEG_VIDEO_VP9_GF_REFRESH_PERIOD;
        ext_ctrl[21].value  = pVp9Param->Vp9GFRefreshPeriod;

        ext_ctrl[22].id     = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHY_QP_ENABLE;
        ext_ctrl[22].value  = 0;

        ext_ctrl[23].id     = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[23].value  = (0 << 16 | 90);

        ext_ctrl[24].id     = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[24].value  = (1 << 16 | 90);

        ext_ctrl[25].id     = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_QP;
        ext_ctrl[25].value  = (2 << 16 | 90);

        ext_ctrl[26].id     = V4L2_CID_MPEG_VIDEO_VP9_REF_NUMBER_FOR_PFRAMES;
        ext_ctrl[26].value  = pVp9Param->RefNumberForPFrame;

        ext_ctrl[27].id     = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER;
        ext_ctrl[27].value  = pVp9Param->TemporalSVC.nTemporalLayerCount;

        ext_ctrl[28].id     = V4L2_CID_MPEG_VIDEO_DISABLE_IVF_HEADER;
        ext_ctrl[28].value  = 1;
        ext_ctrls.count = VP9_CTRL_NUM;

        if (pCtx->videoInstInfo.specificInfo.enc.bTemporalSvcSupport == VIDEO_TRUE) {
            i = ext_ctrls.count;
            ext_ctrl[i].id          = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_BIT0;
            ext_ctrl[i].value       = pVp9Param->TemporalSVC.nTemporalLayerBitrateRatio[0];
            ext_ctrl[i + 1].id      = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_BIT1;
            ext_ctrl[i + 1].value   = pVp9Param->TemporalSVC.nTemporalLayerBitrateRatio[1];
            ext_ctrl[i + 2].id      = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_BIT2;
            ext_ctrl[i + 2].value   = pVp9Param->TemporalSVC.nTemporalLayerBitrateRatio[2];

            ext_ctrls.count += 3;
        }

        /* optional : if these are not set, set value are same as I frame */
        if (pCtx->videoInstInfo.specificInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            i = ext_ctrls.count;
            ext_ctrl[i].id = V4L2_CID_MPEG_VIDEO_VP9_MIN_QP_P;  /* P frame */
            ext_ctrl[i].value = GET_VP9_QP_VALUE(pCommonParam->QpRange.QpMin_P);
            ext_ctrl[i + 1].id =  V4L2_CID_MPEG_VIDEO_VP9_MAX_QP_P;
            ext_ctrl[i + 1].value = GET_VP9_QP_VALUE(pCommonParam->QpRange.QpMax_P);

            ext_ctrls.count += 2;
        }
        break;
    }
#endif
    default:
        ALOGE("[%s] Undefined codec type",__func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    ext_ctrls.ctrl_class = V4L2_CTRL_CLASS_MPEG;
    ext_ctrls.controls = ext_ctrl;

    if (exynos_v4l2_s_ext_ctrl(pCtx->hEnc, &ext_ctrls) != 0) {
        ALOGE("%s: Failed to s_ext_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Frame Tag
 */
static ExynosVideoErrorType MFC_Encoder_Set_FrameTag(
    void   *pHandle,
    int     frameTag)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG, frameTag) != 0) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Get Frame Tag
 */
static int MFC_Encoder_Get_FrameTag(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    int frameTag = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    if (exynos_v4l2_g_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_TAG, &frameTag) != 0) {
        ALOGE("%s: Failed to g_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return frameTag;
}

/*
 * [Encoder OPS] Set Frame Type
 */
static ExynosVideoErrorType MFC_Encoder_Set_FrameType(
    void                    *pHandle,
    ExynosVideoFrameType     frameType)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_FORCE_FRAME_TYPE, frameType) != 0) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Frame Rate
 */
static ExynosVideoErrorType MFC_Encoder_Set_FrameRate(
    void   *pHandle,
    int     frameRate)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_RATE_CH, frameRate) != 0) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Bit Rate
 */
static ExynosVideoErrorType MFC_Encoder_Set_BitRate(
    void   *pHandle,
    int     bitRate)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_BIT_RATE_CH, bitRate) != 0) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Quantization Min/Max
 */
static ExynosVideoErrorType MFC_Encoder_Set_QuantizationRange(
    void *pHandle, ExynosVideoQPRange qpRange)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    int cids[3][2], values[3][2];  /* I, P, B : min & max */
    int i;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __FUNCTION__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (qpRange.QpMin_I > qpRange.QpMax_I) {
        ALOGE("%s: QP(I) range(%d, %d) is wrong", __FUNCTION__, qpRange.QpMin_I, qpRange.QpMax_I);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (qpRange.QpMin_P > qpRange.QpMax_P) {
        ALOGE("%s: QP(P) range(%d, %d) is wrong", __FUNCTION__, qpRange.QpMin_P, qpRange.QpMax_P);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    memset(cids, 0, sizeof(cids));
    memset(values, 0, sizeof(values));

    /* codec specific parameters */
    /* common parameters but id is depends on codec */
    switch (pCtx->videoInstInfo.eCodecType) {
    case VIDEO_CODING_AVC:
    {
        if (qpRange.QpMin_B > qpRange.QpMax_B) {
            ALOGE("%s: QP(B) range(%d, %d) is wrong", __FUNCTION__, qpRange.QpMin_B, qpRange.QpMax_B);
            ret = VIDEO_ERROR_BADPARAM;
            goto EXIT;
        }

        cids[0][0] = V4L2_CID_MPEG_VIDEO_H264_MIN_QP;
        cids[0][1] = V4L2_CID_MPEG_VIDEO_H264_MAX_QP;

        values[0][0] = GET_H264_QP_VALUE(qpRange.QpMin_I);
        values[0][1] = GET_H264_QP_VALUE(qpRange.QpMax_I);

        if (pCtx->videoInstInfo.specificInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            cids[1][0] = V4L2_CID_MPEG_VIDEO_H264_MIN_QP_P;
            cids[1][1] = V4L2_CID_MPEG_VIDEO_H264_MAX_QP_P;
            cids[2][0] = V4L2_CID_MPEG_VIDEO_H264_MIN_QP_B;
            cids[2][1] = V4L2_CID_MPEG_VIDEO_H264_MAX_QP_B;

            values[1][0] = GET_H264_QP_VALUE(qpRange.QpMin_P);
            values[1][1] = GET_H264_QP_VALUE(qpRange.QpMax_P);
            values[2][0] = GET_H264_QP_VALUE(qpRange.QpMin_B);
            values[2][1] = GET_H264_QP_VALUE(qpRange.QpMax_B);
        }
        break;
    }
    case VIDEO_CODING_MPEG4:
    {
        if (qpRange.QpMin_B > qpRange.QpMax_B) {
            ALOGE("%s: QP(B) range(%d, %d) is wrong", __FUNCTION__, qpRange.QpMin_B, qpRange.QpMax_B);
            ret = VIDEO_ERROR_BADPARAM;
            goto EXIT;
        }

        cids[0][0] = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP;
        cids[0][1] = V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP;

        values[0][0] = GET_MPEG4_QP_VALUE(qpRange.QpMin_I);
        values[0][1] = GET_MPEG4_QP_VALUE(qpRange.QpMax_I);

        if (pCtx->videoInstInfo.specificInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            cids[1][0] = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_P;
            cids[1][1] = V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_P;
            cids[2][0] = V4L2_CID_MPEG_VIDEO_MPEG4_MIN_QP_B;
            cids[2][1] = V4L2_CID_MPEG_VIDEO_MPEG4_MAX_QP_B;

            values[1][0] = GET_MPEG4_QP_VALUE(qpRange.QpMin_P);
            values[1][1] = GET_MPEG4_QP_VALUE(qpRange.QpMax_P);
            values[2][0] = GET_MPEG4_QP_VALUE(qpRange.QpMin_B);
            values[2][1] = GET_MPEG4_QP_VALUE(qpRange.QpMax_B);
        }
        break;
    }
    case VIDEO_CODING_H263:
    {
        cids[0][0] = V4L2_CID_MPEG_VIDEO_H263_MIN_QP;
        cids[0][1] = V4L2_CID_MPEG_VIDEO_H263_MAX_QP;

        values[0][0] = GET_H263_QP_VALUE(qpRange.QpMin_I);
        values[0][1] = GET_H263_QP_VALUE(qpRange.QpMax_I);

        if (pCtx->videoInstInfo.specificInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            cids[1][0] = V4L2_CID_MPEG_VIDEO_H263_MIN_QP_P;
            cids[1][1] = V4L2_CID_MPEG_VIDEO_H263_MAX_QP_P;

            values[1][0] = GET_H263_QP_VALUE(qpRange.QpMin_P);
            values[1][1] = GET_H263_QP_VALUE(qpRange.QpMax_P);
        }
        break;
    }
    case VIDEO_CODING_VP8:
    {
        cids[0][0] = V4L2_CID_MPEG_VIDEO_VP8_MIN_QP;
        cids[0][1] = V4L2_CID_MPEG_VIDEO_VP8_MAX_QP;

        values[0][0] = GET_VP8_QP_VALUE(qpRange.QpMin_I);
        values[0][1] = GET_VP8_QP_VALUE(qpRange.QpMax_I);

        if (pCtx->videoInstInfo.specificInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            cids[1][0] = V4L2_CID_MPEG_VIDEO_VP8_MIN_QP_P;
            cids[1][1] = V4L2_CID_MPEG_VIDEO_VP8_MAX_QP_P;

            values[1][0] = GET_VP8_QP_VALUE(qpRange.QpMin_P);
            values[1][1] = GET_VP8_QP_VALUE(qpRange.QpMax_P);
        }
        break;
    }
#ifdef USE_HEVCENC_SUPPORT
    case VIDEO_CODING_HEVC:
    {
        if (qpRange.QpMin_B > qpRange.QpMax_B) {
            ALOGE("%s: QP(B) range(%d, %d) is wrong", __FUNCTION__, qpRange.QpMin_B, qpRange.QpMax_B);
            ret = VIDEO_ERROR_BADPARAM;
            goto EXIT;
        }

        cids[0][0] = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP;
        cids[0][1] = V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP;

        values[0][0] = GET_HEVC_QP_VALUE(qpRange.QpMin_I);
        values[0][1] = GET_HEVC_QP_VALUE(qpRange.QpMax_I);

        if (pCtx->videoInstInfo.specificInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            cids[1][0] = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_P;
            cids[1][1] = V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_P;
            cids[2][0] = V4L2_CID_MPEG_VIDEO_HEVC_MIN_QP_B;
            cids[2][1] = V4L2_CID_MPEG_VIDEO_HEVC_MAX_QP_B;

            values[1][0] = GET_HEVC_QP_VALUE(qpRange.QpMin_P);
            values[1][1] = GET_HEVC_QP_VALUE(qpRange.QpMax_P);
            values[2][0] = GET_HEVC_QP_VALUE(qpRange.QpMin_B);
            values[2][1] = GET_HEVC_QP_VALUE(qpRange.QpMax_B);
        }
        break;
    }
#endif
#ifdef USE_VP9ENC_SUPPORT
    case VIDEO_CODING_VP9:
    {
        cids[0][0] = V4L2_CID_MPEG_VIDEO_VP9_MIN_QP;
        cids[0][1] = V4L2_CID_MPEG_VIDEO_VP9_MAX_QP;

        values[0][0] = GET_VP9_QP_VALUE(qpRange.QpMin_I);
        values[0][1] = GET_VP9_QP_VALUE(qpRange.QpMax_I);

        if (pCtx->videoInstInfo.specificInfo.enc.bQpRangePBSupport == VIDEO_TRUE) {
            cids[1][0] = V4L2_CID_MPEG_VIDEO_VP9_MIN_QP_P;
            cids[1][1] = V4L2_CID_MPEG_VIDEO_VP9_MAX_QP_P;

            values[1][0] = GET_VP9_QP_VALUE(qpRange.QpMin_P);
            values[1][1] = GET_VP9_QP_VALUE(qpRange.QpMax_P);
        }
        break;
    }
#endif
    default:
    {
        ALOGE("[%s] Undefined codec type", __FUNCTION__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }
        break;
    }

    for (i = 0; i < (int)(sizeof(cids)/sizeof(cids[0])); i++) {
        if (cids[i][0] == 0)
            break;

        ALOGV("%s: QP[%d] range (%d / %d)", __FUNCTION__, i, values[i][0], values[i][1]);

        /* keep a calling sequence as Max->Min because dirver has a restriction */
        if (exynos_v4l2_s_ctrl(pCtx->hEnc, cids[i][1], values[i][1]) != 0) {
            ALOGE("%s: Failed to s_ctrl for max value", __FUNCTION__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }

        if (exynos_v4l2_s_ctrl(pCtx->hEnc, cids[i][0], values[i][0]) != 0) {
            ALOGE("%s: Failed to s_ctrl for min value", __FUNCTION__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Frame Skip
 */
static ExynosVideoErrorType MFC_Encoder_Set_FrameSkip(
    void   *pHandle,
    int     frameSkip)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_SKIP_MODE, frameSkip) != 0) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set IDR Period
 */
static ExynosVideoErrorType MFC_Encoder_Set_IDRPeriod(
    void   *pHandle,
    int     IDRPeriod)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_I_PERIOD_CH, IDRPeriod) != 0) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Enable Prepend SPS and PPS to every IDR Frames
 */
static ExynosVideoErrorType MFC_Encoder_Enable_PrependSpsPpsToIdr(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    switch ((int)pCtx->videoInstInfo.eCodecType) {
    case VIDEO_CODING_AVC:
        if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_VIDEO_H264_PREPEND_SPSPPS_TO_IDR, 1) != 0) {
            ALOGE("%s: Failed to s_ctrl", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
        break;
#if defined(USE_HEVCENC_SUPPORT) && defined(CID_SUPPORT)
    case VIDEO_CODING_HEVC:
        if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_VIDEO_HEVC_PREPEND_SPSPPS_TO_IDR, 1) != 0) {
            ALOGE("%s: Failed to s_ctrl", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
        break;
#endif
    default:
        ALOGE("%s: codec(%x) can't support PrependSpsPpsToIdr", __func__, pCtx->videoInstInfo.eCodecType);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Qos Ratio
 */
static ExynosVideoErrorType MFC_Encoder_Set_QosRatio(
    void *pHandle,
    int   ratio)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_VIDEO_QOS_RATIO, ratio) != 0) {
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Layer Change
 */
static ExynosVideoErrorType MFC_Encoder_Set_LayerChange(
    void                        *pHandle,
    TemporalLayerShareBuffer     TemporalSVC)
{
    ExynosVideoEncContext      *pCtx  = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType        ret   = VIDEO_ERROR_NONE;
    TemporalLayerShareBuffer   *pTLSB = NULL;
    unsigned int CID = 0;
    int i = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pTLSB = (TemporalLayerShareBuffer *)pCtx->pTemporalLayerShareBufferAddr;
    if (pCtx->videoInstInfo.specificInfo.enc.bTemporalSvcSupport == VIDEO_FALSE) {
        ALOGE("%s: Layer Change is not supported :: codec type(%x), F/W ver(%x)", __func__, pCtx->videoInstInfo.eCodecType, pCtx->videoInstInfo.HwVersion);
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    switch ((int)pCtx->videoInstInfo.eCodecType) {
    case VIDEO_CODING_AVC:
        CID = V4L2_CID_MPEG_VIDEO_H264_HIERARCHICAL_CODING_LAYER_CH;
        break;
#if defined(USE_HEVCENC_SUPPORT) && defined(CID_SUPPORT)
    case VIDEO_CODING_HEVC:
        CID = V4L2_CID_MPEG_VIDEO_HEVC_HIERARCHICAL_CODING_LAYER_CH;
        break;
#endif
    case VIDEO_CODING_VP8:
        CID = V4L2_CID_MPEG_VIDEO_VP8_HIERARCHICAL_CODING_LAYER_CH;
        break;
#ifdef USE_VP9ENC_SUPPORT
    case VIDEO_CODING_VP9:
        CID = V4L2_CID_MPEG_VIDEO_VP9_HIERARCHICAL_CODING_LAYER_CH;
        break;
#endif
    default:
        ALOGE("%s: this codec type is not supported(%x), F/W ver(%x)", __func__, pCtx->videoInstInfo.eCodecType, pCtx->videoInstInfo.HwVersion);
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
        break;
    }

    memcpy(pTLSB, &TemporalSVC, sizeof(TemporalLayerShareBuffer));
    if (exynos_v4l2_s_ctrl(pCtx->hEnc, CID, pCtx->nTemporalLayerShareBufferFD) != 0) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

#ifdef CID_SUPPORT
/*
 * [Encoder OPS] Set Dynamic QP Control
 */
static ExynosVideoErrorType MFC_Encoder_Set_DynamicQpControl(void *pHandle, int nQp)
{
    ExynosVideoEncContext *pCtx    = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret     = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC_CONFIG_QP, nQp) != 0) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Mark LTR-frame
 */
static ExynosVideoErrorType MFC_Encoder_Set_MarkLTRFrame(void *pHandle, int nLongTermFrmIdx)
{
    ExynosVideoEncContext *pCtx    = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret     = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC_H264_MARK_LTR, nLongTermFrmIdx) != 0) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Used LTR-frame
 */
static ExynosVideoErrorType MFC_Encoder_Set_UsedLTRFrame(void *pHandle, int nUsedLTRFrameNum)
{
    ExynosVideoEncContext *pCtx    = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret     = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC_H264_USE_LTR, nUsedLTRFrameNum) != 0) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Set Base PID
 */
static ExynosVideoErrorType MFC_Encoder_Set_BasePID(void *pHandle, int nPID)
{
    ExynosVideoEncContext *pCtx    = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret     = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC_H264_BASE_PRIORITY, nPID) != 0) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}
#endif // CID_SUPPORT

#ifdef USE_MFC_MEDIA
/*
 * [Encoder OPS] Set Roi Information
 */
static ExynosVideoErrorType MFC_Encoder_Set_RoiInfo(
    void                   *pHandle,
    RoiInfoShareBuffer     *pRoiInfo)
{
    ExynosVideoEncContext   *pCtx  = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType     ret   = VIDEO_ERROR_NONE;
    RoiInfoShareBuffer      *pRISB = NULL;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    pRISB = (RoiInfoShareBuffer *)pCtx->pRoiShareBufferAddr;
    if (pCtx->videoInstInfo.specificInfo.enc.bRoiInfoSupport == VIDEO_FALSE) {
        ALOGE("%s: ROI Info setting is not supported :: codec type(%x), F/W ver(%x)", __func__, pCtx->videoInstInfo.eCodecType, pCtx->videoInstInfo.HwVersion);
        ret = VIDEO_ERROR_NOSUPPORT;
        goto EXIT;
    }

    memcpy(pRISB, pRoiInfo, sizeof(RoiInfoShareBuffer));
    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_VIDEO_ROI_CONTROL, pCtx->nRoiShareBufferFD) != 0) {
        ALOGE("%s: Failed to s_ctrl", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}
#endif // USE_MFC_MEDIA

/*
 * [Encoder Buffer OPS] Enable Cacheable (Input)
 */
static ExynosVideoErrorType MFC_Encoder_Enable_Cacheable_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_CACHEABLE, 2) != 0) {
        ALOGE("%s: Failed V4L2_CID_CACHEABLE", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Enable Cacheable (Output)
 */
static ExynosVideoErrorType MFC_Encoder_Enable_Cacheable_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_CACHEABLE, 1) != 0) {
        ALOGE("%s: Failed V4L2_CID_CACHEABLE", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Set Shareable Buffer (Input)
 */
static ExynosVideoErrorType MFC_Encoder_Set_Shareable_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
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
 * [Encoder Buffer OPS] Set Shareable Buffer (Output)
 */
static ExynosVideoErrorType MFC_Encoder_Set_Shareable_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
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
 * [Encoder Buffer OPS] Get Buffer (Input)
 */
static ExynosVideoErrorType MFC_Encoder_Get_Buffer_Inbuf(
    void               *pHandle,
    int                 nIndex,
    ExynosVideoBuffer **pBuffer)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        *pBuffer = NULL;
        ret = VIDEO_ERROR_NOBUFFERS;
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
 * [Encoder Buffer OPS] Get Buffer (Output)
 */
static ExynosVideoErrorType MFC_Encoder_Get_Buffer_Outbuf(
    void               *pHandle,
    int                 nIndex,
    ExynosVideoBuffer **pBuffer)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        *pBuffer = NULL;
        ret = VIDEO_ERROR_NOBUFFERS;
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
 * [Encoder Buffer OPS] Set Geometry (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Set_Geometry_Inbuf(
    void                *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
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
    fmt.fmt.pix_mp.pixelformat = __ColorFormatType_To_V4L2PixelFormat(bufferConf->eColorFormat, pCtx->videoInstInfo.HwVersion);
    fmt.fmt.pix_mp.width = bufferConf->nFrameWidth;
    fmt.fmt.pix_mp.height = bufferConf->nFrameHeight;
    fmt.fmt.pix_mp.plane_fmt[0].bytesperline = bufferConf->nStride;
    fmt.fmt.pix_mp.num_planes = bufferConf->nPlaneCnt;

    if (exynos_v4l2_s_fmt(pCtx->hEnc, &fmt) != 0) {
        ALOGE("%s: Failed to s_fmt", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&pCtx->inbufGeometry, bufferConf, sizeof(pCtx->inbufGeometry));
    pCtx->nInbufPlanes = bufferConf->nPlaneCnt;

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Get Geometry (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Get_Geometry_Inbuf(
    void                *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
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
    if (exynos_v4l2_g_fmt(pCtx->hEnc, &fmt) != 0) {
        ALOGE("%s: Failed to g_fmt", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    bufferConf->nFrameHeight = fmt.fmt.pix_mp.width;
    bufferConf->nFrameHeight = fmt.fmt.pix_mp.height;
    bufferConf->nSizeImage = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Set Geometry (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Set_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
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
    fmt.fmt.pix_mp.pixelformat = __CodingType_To_V4L2PixelFormat(bufferConf->eCompressionFormat);
    fmt.fmt.pix_mp.plane_fmt[0].sizeimage = bufferConf->nSizeImage;

    if (exynos_v4l2_s_fmt(pCtx->hEnc, &fmt) != 0) {
        ALOGE("%s: Failed to s_fmt", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    memcpy(&pCtx->outbufGeometry, bufferConf, sizeof(pCtx->outbufGeometry));
    pCtx->nOutbufPlanes = bufferConf->nPlaneCnt;

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Get Geometry (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Get_Geometry_Outbuf(
    void                *pHandle,
    ExynosVideoGeometry *bufferConf)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
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
    if (exynos_v4l2_g_fmt(pCtx->hEnc, &fmt) != 0) {
        ALOGE("%s: Failed to g_fmt", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    bufferConf->nSizeImage = fmt.fmt.pix_mp.plane_fmt[0].sizeimage;

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Setup (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Setup_Inbuf(
    void           *pHandle,
    unsigned int    nBufferCount)
{
    ExynosVideoEncContext *pCtx         = (ExynosVideoEncContext *)pHandle;
    ExynosVideoPlane      *pVideoPlane  = NULL;
    ExynosVideoErrorType   ret          = VIDEO_ERROR_NONE;

    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_BUFFER_MAX_PLANES];
    int i, j;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (nBufferCount == 0) {
        nBufferCount = MAX_INPUTBUFFER_COUNT;
        ALOGV("%s: Change buffer count %d", __func__, nBufferCount);
    }

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareInbuf == VIDEO_TRUE)
        req.memory = pCtx->videoInstInfo.nMemoryType;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hEnc, &req) != 0) {
        ALOGE("Failed to require buffer");
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->nInbufs = (int)req.count;

    pCtx->pInbuf = malloc(sizeof(*pCtx->pInbuf) * pCtx->nInbufs);
    if (pCtx->pInbuf == NULL) {
        ALOGE("%s: Failed to allocate input buffer context", __func__);
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
            if (exynos_v4l2_querybuf(pCtx->hEnc, &buf) != 0) {
                ALOGE("%s: Failed to querybuf", __func__);
                ret = VIDEO_ERROR_APIFAIL;
                goto EXIT;
            }

            for (j = 0; j < pCtx->nInbufPlanes; j++) {
                pVideoPlane = &pCtx->pInbuf[i].planes[j];
                pVideoPlane->addr = mmap(NULL,
                        buf.m.planes[j].length, PROT_READ | PROT_WRITE,
                        MAP_SHARED, pCtx->hEnc, buf.m.planes[j].m.mem_offset);

                if (pVideoPlane->addr == MAP_FAILED) {
                    ALOGE("%s: Failed to map", __func__);
                    ret = VIDEO_ERROR_MAPFAIL;
                    goto EXIT;
                }

                pVideoPlane->allocSize = buf.m.planes[j].length;
                pVideoPlane->dataSize = 0;
            }

            pCtx->pInbuf[i].pGeometry = &pCtx->inbufGeometry;
            pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
            pCtx->pInbuf[i].bRegistered = VIDEO_TRUE;

        }
    } else {
        for (i = 0; i < pCtx->nInbufs; i++) {
            pCtx->pInbuf[i].pGeometry = &pCtx->inbufGeometry;
            pCtx->pInbuf[i].bQueued = VIDEO_FALSE;
            pCtx->pInbuf[i].bRegistered = VIDEO_FALSE;
        }
    }

    return ret;

EXIT:
    if ((pCtx != NULL) && (pCtx->pInbuf != NULL)) {
        if (pCtx->bShareInbuf == VIDEO_FALSE) {
            for (i = 0; i < pCtx->nInbufs; i++) {
                for (j = 0; j < pCtx->nInbufPlanes; j++) {
                    pVideoPlane = &pCtx->pInbuf[i].planes[j];
                    if (pVideoPlane->addr == MAP_FAILED) {
                        pVideoPlane->addr = NULL;
                        break;
                    }

                    munmap(pVideoPlane->addr, pVideoPlane->allocSize);
                }
            }
        }

        free(pCtx->pInbuf);
        pCtx->pInbuf = NULL;
    }

    return ret;
}

/*
 * [Encoder Buffer OPS] Setup (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Setup_Outbuf(
    void           *pHandle,
    unsigned int    nBufferCount)
{
    ExynosVideoEncContext *pCtx         = (ExynosVideoEncContext *)pHandle;
    ExynosVideoPlane      *pVideoPlane  = NULL;
    ExynosVideoErrorType   ret          = VIDEO_ERROR_NONE;

    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    struct v4l2_plane planes[VIDEO_BUFFER_MAX_PLANES];
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

    memset(&req, 0, sizeof(req));

    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    req.count = nBufferCount;

    if (pCtx->bShareOutbuf == VIDEO_TRUE)
        req.memory = pCtx->videoInstInfo.nMemoryType;
    else
        req.memory = V4L2_MEMORY_MMAP;

    if (exynos_v4l2_reqbufs(pCtx->hEnc, &req) != 0) {
        ALOGE("%s: Failed to reqbuf", __func__);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pCtx->nOutbufs = req.count;

    pCtx->pOutbuf = malloc(sizeof(*pCtx->pOutbuf) * pCtx->nOutbufs);
    if (pCtx->pOutbuf == NULL) {
        ALOGE("%s: Failed to allocate output buffer context", __func__);
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
            if (exynos_v4l2_querybuf(pCtx->hEnc, &buf) != 0) {
                ALOGE("%s: Failed to querybuf", __func__);
                ret = VIDEO_ERROR_APIFAIL;
                goto EXIT;
            }

            for (j = 0; j < pCtx->nOutbufPlanes; j++) {
                pVideoPlane = &pCtx->pOutbuf[i].planes[j];
                pVideoPlane->addr = mmap(NULL,
                        buf.m.planes[j].length, PROT_READ | PROT_WRITE,
                        MAP_SHARED, pCtx->hEnc, buf.m.planes[j].m.mem_offset);

                if (pVideoPlane->addr == MAP_FAILED) {
                    ALOGE("%s: Failed to map", __func__);
                    ret = VIDEO_ERROR_MAPFAIL;
                    goto EXIT;
                }

                pVideoPlane->allocSize = buf.m.planes[j].length;
                pVideoPlane->dataSize = 0;
            }

            pCtx->pOutbuf[i].pGeometry = &pCtx->outbufGeometry;
            pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
            pCtx->pOutbuf[i].bRegistered = VIDEO_TRUE;
        }
    } else {
        for (i = 0; i < pCtx->nOutbufs; i++ ) {
            pCtx->pOutbuf[i].pGeometry = &pCtx->outbufGeometry;
            pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
            pCtx->pOutbuf[i].bRegistered = VIDEO_FALSE;
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
 * [Encoder Buffer OPS] Run (src)
 */
static ExynosVideoErrorType MFC_Encoder_Run_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonInbuf == VIDEO_FALSE) {
        if (exynos_v4l2_streamon(pCtx->hEnc, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) != 0) {
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
 * [Encoder Buffer OPS] Run (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Run_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonOutbuf == VIDEO_FALSE) {
        if (exynos_v4l2_streamon(pCtx->hEnc, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) != 0) {
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
 * [Encoder Buffer OPS] Stop (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Stop_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int i = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonInbuf == VIDEO_TRUE) {
        if (exynos_v4l2_streamoff(pCtx->hEnc, V4L2_BUF_TYPE_VIDEO_OUTPUT_MPLANE) != 0) {
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
 * [Encoder Buffer OPS] Stop (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Stop_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int i = 0;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    if (pCtx->bStreamonOutbuf == VIDEO_TRUE) {
        if (exynos_v4l2_streamoff(pCtx->hEnc, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) != 0) {
            ALOGE("%s: Failed to streamoff for output buffer", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
        pCtx->bStreamonOutbuf = VIDEO_FALSE;
    }

    for (i = 0; i < pCtx->nOutbufs; i++) {
        pCtx->pOutbuf[i].bQueued = VIDEO_FALSE;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Wait (Src)
 */
static ExynosVideoErrorType MFC_Encoder_Wait_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct pollfd poll_events;
    int poll_state;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    poll_events.fd = pCtx->hEnc;
    poll_events.events = POLLOUT | POLLERR;
    poll_events.revents = 0;

    do {
        poll_state = poll((struct pollfd*)&poll_events, 1, VIDEO_ENCODER_POLL_TIMEOUT);
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

/*
 * [Encoder Buffer OPS] Wait (Dst)
 */
static ExynosVideoErrorType MFC_Encoder_Wait_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    struct pollfd poll_events;
    int poll_state;
    int bframe_count = 0; // FIXME

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    poll_events.fd = pCtx->hEnc;
    poll_events.events = POLLIN | POLLERR;
    poll_events.revents = 0;

    do {
        poll_state = poll((struct pollfd*)&poll_events, 1, VIDEO_ENCODER_POLL_TIMEOUT);
        if (poll_state > 0) {
            if (poll_events.revents & POLLIN) {
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
        } else {
            bframe_count++; // FIXME
        }
    } while (poll_state == 0 && bframe_count < 5); // FIXME

EXIT:
    return ret;
}

static ExynosVideoErrorType MFC_Encoder_Register_Inbuf(
    void             *pHandle,
    ExynosVideoPlane *planes,
    int               nPlanes)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int nIndex;

    if ((pCtx == NULL) || (planes == NULL) || (nPlanes != pCtx->nInbufPlanes)) {
        ALOGE("%s: input params must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
        if (pCtx->pInbuf[nIndex].bRegistered == VIDEO_FALSE) {
            int plane;
            for (plane = 0; plane < nPlanes; plane++) {
                pCtx->pInbuf[nIndex].planes[plane].addr = planes[plane].addr;
                pCtx->pInbuf[nIndex].planes[plane].allocSize = planes[plane].allocSize;
                pCtx->pInbuf[nIndex].planes[plane].fd = planes[plane].fd;
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

static ExynosVideoErrorType MFC_Encoder_Register_Outbuf(
    void             *pHandle,
    ExynosVideoPlane *planes,
    int               nPlanes)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int nIndex;

    if ((pCtx == NULL) || (planes == NULL) || (nPlanes != pCtx->nOutbufPlanes)) {
        ALOGE("%s: params must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
        if (pCtx->pOutbuf[nIndex].bRegistered == VIDEO_FALSE) {
            int plane;
            for (plane = 0; plane < nPlanes; plane++) {
                pCtx->pOutbuf[nIndex].planes[plane].addr = planes[plane].addr;
                pCtx->pOutbuf[nIndex].planes[plane].allocSize = planes[plane].allocSize;
                pCtx->pOutbuf[nIndex].planes[plane].fd = planes[plane].fd;
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

static ExynosVideoErrorType MFC_Encoder_Clear_RegisteredBuffer_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int nIndex = -1, plane;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
        for (plane = 0; plane < pCtx->nInbufPlanes; plane++)
            pCtx->pInbuf[nIndex].planes[plane].addr = NULL;
        pCtx->pInbuf[nIndex].bRegistered = VIDEO_FALSE;
    }

EXIT:
    return ret;
}

static ExynosVideoErrorType MFC_Encoder_Clear_RegisteredBuffer_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int nIndex = -1, plane;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
        for (plane = 0; plane < pCtx->nOutbufPlanes; plane++)
            pCtx->pOutbuf[nIndex].planes[plane].addr = NULL;
        pCtx->pOutbuf[nIndex].bRegistered = VIDEO_FALSE;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Find (Input)
 */
static int MFC_Encoder_Find_Inbuf(
    void          *pHandle,
    unsigned char *pBuffer)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
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
 * [Encoder Buffer OPS] Find (Output)
 */
static int MFC_Encoder_Find_Outbuf(
    void          *pHandle,
    unsigned char *pBuffer)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
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
 * [Encoder Buffer OPS] Enqueue (Input)
 */
static ExynosVideoErrorType MFC_Encoder_Enqueue_Inbuf(
    void          *pHandle,
    void          *pBuffer[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_plane planes[VIDEO_BUFFER_MAX_PLANES];
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
    index = MFC_Encoder_Find_Inbuf(pCtx, pBuffer[0]);
    if (index == -1) {
        pthread_mutex_unlock(pMutex);
        ALOGW("%s: Matching Buffer index not found", __func__);
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
                buf.m.planes[i].m.fd = pCtx->pInbuf[buf.index].planes[i].fd;

            buf.m.planes[i].length = pCtx->pInbuf[index].planes[i].allocSize;
            buf.m.planes[i].bytesused = dataSize[i];
            buf.m.planes[i].data_offset = 0;
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

        if (flags & LAST_FRAME)
            ALOGD("%s: DATA with flags(0x%x)", __FUNCTION__, flags);
    }
#ifdef USE_ORIGINAL_HEADER
    buf.reserved2 = flags;
#else
    buf.input = flags;
#endif

    signed long long sec = (((OMX_BUFFERHEADERTYPE *)pPrivate)->nTimeStamp / 1E6);
    signed long long usec = (((OMX_BUFFERHEADERTYPE *)pPrivate)->nTimeStamp) - (sec * 1E6);
    buf.timestamp.tv_sec = (long)sec;
    buf.timestamp.tv_usec = (long)usec;

    pCtx->pInbuf[buf.index].pPrivate = pPrivate;
    pCtx->pInbuf[buf.index].bQueued = VIDEO_TRUE;

    if (pCtx->videoInstInfo.eCodecType == VIDEO_CODING_VP8) {
        int     oldFrameRate   = 0;
        int     curFrameRate   = 0;
        int64_t curDuration    = 0;

        curDuration = ((int64_t)((OMX_BUFFERHEADERTYPE *)pPrivate)->nTimeStamp - pCtx->oldTimeStamp);
        if ((curDuration > 0) && (pCtx->oldDuration > 0)) {
            oldFrameRate = (1E6 / pCtx->oldDuration);
            curFrameRate = (1E6 / curDuration);

            if (((curFrameRate - oldFrameRate) >= FRAME_RATE_CHANGE_THRESH_HOLD) ||
                ((oldFrameRate - curFrameRate) >= FRAME_RATE_CHANGE_THRESH_HOLD)) {
                if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_RATE_CH, curFrameRate) != 0) {
                    ALOGE("%s: Failed to s_ctrl", __func__);
                    pthread_mutex_unlock(pMutex);
                    ret = VIDEO_ERROR_APIFAIL;
                    goto EXIT;
                }
                pCtx->oldFrameRate = curFrameRate;
            }
        }

        if (curDuration > 0)
            pCtx->oldDuration = curDuration;
        pCtx->oldTimeStamp = (int64_t)((OMX_BUFFERHEADERTYPE *)pPrivate)->nTimeStamp;
    }

    pthread_mutex_unlock(pMutex);

    if (exynos_v4l2_qbuf(pCtx->hEnc, &buf) != 0) {
        ALOGE("%s: Failed to enqueue input buffer", __func__);
        pthread_mutex_lock(pMutex);
        pCtx->pInbuf[buf.index].pPrivate = NULL;
        pCtx->pInbuf[buf.index].bQueued = VIDEO_FALSE;
        pthread_mutex_unlock(pMutex);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Enqueue (Output)
 */
static ExynosVideoErrorType MFC_Encoder_Enqueue_Outbuf(
    void          *pHandle,
    void          *pBuffer[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_plane planes[VIDEO_BUFFER_MAX_PLANES];
    struct v4l2_buffer buf;
    int i, index;

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
    index = MFC_Encoder_Find_Outbuf(pCtx, pBuffer[0]);
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
        }
    } else {
        buf.memory = V4L2_MEMORY_MMAP;
    }

    pCtx->pOutbuf[buf.index].pPrivate = pPrivate;
    pCtx->pOutbuf[buf.index].bQueued = VIDEO_TRUE;
    pthread_mutex_unlock(pMutex);

    if (exynos_v4l2_qbuf(pCtx->hEnc, &buf) != 0) {
        ALOGE("%s: Failed to enqueue output buffer", __func__);
        pthread_mutex_lock(pMutex);
        pCtx->pOutbuf[buf.index].pPrivate = NULL;
        pCtx->pOutbuf[buf.index].bQueued = VIDEO_FALSE;
        pthread_mutex_unlock(pMutex);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Enqueue All (Output)
 */
static ExynosVideoErrorType MFC_Encoder_Enqueue_All_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;

    void           *pBuffer[VIDEO_BUFFER_MAX_PLANES]  = {NULL, };
    unsigned int    dataSize[VIDEO_BUFFER_MAX_PLANES] = {0, };

    int i;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    for (i = 0; i < pCtx->nOutbufs; i++) {
        ret = MFC_Encoder_Enqueue_Outbuf(pCtx, pBuffer, dataSize, 1, NULL);
        if (ret != VIDEO_ERROR_NONE)
            goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] Dequeue (Input)
 */
static ExynosVideoBuffer *MFC_Encoder_Dequeue_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx     = (ExynosVideoEncContext *)pHandle;
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

    if (exynos_v4l2_dqbuf(pCtx->hEnc, &buf) != 0) {
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
    pthread_mutex_unlock(pMutex);

EXIT:
    return pInbuf;
}

/*
 * [Encoder Buffer OPS] Dequeue (Output)
 */
static ExynosVideoBuffer *MFC_Encoder_Dequeue_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx    = (ExynosVideoEncContext *)pHandle;
    ExynosVideoBuffer     *pOutbuf = NULL;
    pthread_mutex_t       *pMutex  = NULL;

    struct v4l2_buffer buf;
    struct v4l2_plane  planes[VIDEO_BUFFER_MAX_PLANES];
    int value, plane;
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

    /* returning DQBUF_EIO means MFC H/W status is invalid */
    ret = exynos_v4l2_dqbuf(pCtx->hEnc, &buf);
    if (ret != 0) {
        if (errno == EIO)
            pOutbuf = (ExynosVideoBuffer *)VIDEO_ERROR_DQBUF_EIO;
        else
            pOutbuf = NULL;
        goto EXIT;
    }

    pMutex = (pthread_mutex_t*)pCtx->pOutMutex;
    pthread_mutex_lock(pMutex);

    pOutbuf = &pCtx->pOutbuf[buf.index];
    if (pOutbuf->bQueued == VIDEO_FALSE) {
        pOutbuf = NULL;
        pthread_mutex_unlock(pMutex);
        goto EXIT;
    }

    for (plane = 0; plane < pCtx->nOutbufPlanes; plane++)
        pOutbuf->planes[plane].dataSize = buf.m.planes[plane].bytesused;

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
        ALOGI("%s: encoded frame type is = %d",__func__, (buf.flags & (0x7 << 3)));
        pOutbuf->frameType = VIDEO_FRAME_OTHERS;
        break;
    };

    pOutbuf->bQueued = VIDEO_FALSE;
    pthread_mutex_unlock(pMutex);

EXIT:
    return pOutbuf;
}

static ExynosVideoErrorType MFC_Encoder_Clear_Queued_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int i = 0;

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

static ExynosVideoErrorType MFC_Encoder_Clear_Queued_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    int i = 0;

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
 * [Encoder Buffer OPS] FindIndex (Input)
 */
static int MFC_Encoder_FindEmpty_Inbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    int nIndex = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nInbufs; nIndex++) {
        if (pCtx->pInbuf[nIndex].bQueued == VIDEO_FALSE) {
            break;
        }
    }

    if (nIndex == pCtx->nInbufs)
        nIndex = -1;

EXIT:
    return nIndex;
}

/*
 * [Encoder Buffer OPS] FindIndex (Output)
 */
static int MFC_Encoder_FindEmpty_Outbuf(void *pHandle)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    int nIndex = -1;

    if (pCtx == NULL) {
        ALOGE("%s: Video context info must be supplied", __func__);
        goto EXIT;
    }

    for (nIndex = 0; nIndex < pCtx->nOutbufs; nIndex++) {
        if (pCtx->pOutbuf[nIndex].bQueued == VIDEO_FALSE)
            break;
    }

    if (nIndex == pCtx->nInbufs)
        nIndex = -1;

EXIT:
    return nIndex;
}

/*
 * [Encoder Buffer OPS] ExtensionEnqueue (Input)
 */
static ExynosVideoErrorType MFC_Encoder_ExtensionEnqueue_Inbuf(
    void          *pHandle,
    void          *pBuffer[],
    int            pFd[],
    unsigned int   allocLen[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoEncContext *pCtx = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret  = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_plane planes[VIDEO_BUFFER_MAX_PLANES];
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
    index = MFC_Encoder_FindEmpty_Inbuf(pCtx);
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

        if (flags & LAST_FRAME)
            ALOGD("%s: DATA with flags(0x%x)", __FUNCTION__, flags);
    }
#ifdef USE_ORIGINAL_HEADER
    buf.reserved2 = flags;
#else
    buf.input = flags;
#endif

    signed long long sec = (((OMX_BUFFERHEADERTYPE *)pPrivate)->nTimeStamp / 1E6);
    signed long long usec = (((OMX_BUFFERHEADERTYPE *)pPrivate)->nTimeStamp) - (sec * 1E6);
    buf.timestamp.tv_sec = (long)sec;
    buf.timestamp.tv_usec = (long)usec;

    pCtx->pInbuf[buf.index].pPrivate = pPrivate;
    pCtx->pInbuf[buf.index].bQueued = VIDEO_TRUE;

    if (pCtx->videoInstInfo.eCodecType == VIDEO_CODING_VP8) {
        int     oldFrameRate   = 0;
        int     curFrameRate   = 0;
        int64_t curDuration    = 0;

        curDuration = ((int64_t)((OMX_BUFFERHEADERTYPE *)pPrivate)->nTimeStamp - pCtx->oldTimeStamp);
        if ((curDuration > 0) && (pCtx->oldDuration > 0)) {
            oldFrameRate = (1E6 / pCtx->oldDuration);
            curFrameRate = (1E6 / curDuration);

            if (((curFrameRate - oldFrameRate) >= FRAME_RATE_CHANGE_THRESH_HOLD) ||
                ((oldFrameRate - curFrameRate) >= FRAME_RATE_CHANGE_THRESH_HOLD)) {
                if (exynos_v4l2_s_ctrl(pCtx->hEnc, V4L2_CID_MPEG_MFC51_VIDEO_FRAME_RATE_CH, curFrameRate) != 0) {
                    ALOGE("%s: Failed to s_ctrl", __func__);
                    pthread_mutex_unlock(pMutex);
                    ret = VIDEO_ERROR_APIFAIL;
                    goto EXIT;
                }
                pCtx->oldFrameRate = curFrameRate;
            }
        }

        if (curDuration > 0)
            pCtx->oldDuration = curDuration;
        pCtx->oldTimeStamp = (int64_t)((OMX_BUFFERHEADERTYPE *)pPrivate)->nTimeStamp;
    }

    pthread_mutex_unlock(pMutex);

    if (exynos_v4l2_qbuf(pCtx->hEnc, &buf) != 0) {
        ALOGE("%s: Failed to enqueue input buffer", __func__);
        pthread_mutex_lock(pMutex);
        pCtx->pInbuf[buf.index].pPrivate = NULL;
        pCtx->pInbuf[buf.index].bQueued = VIDEO_FALSE;
        pthread_mutex_unlock(pMutex);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}

/*
 * [Encoder Buffer OPS] ExtensionDequeue (Input)
 */
static ExynosVideoErrorType MFC_Encoder_ExtensionDequeue_Inbuf(
    void              *pHandle,
    ExynosVideoBuffer *pVideoBuffer)
{
    ExynosVideoEncContext *pCtx     = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret      = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex   = NULL;

    struct v4l2_buffer buf;
    struct v4l2_plane  planes[VIDEO_BUFFER_MAX_PLANES];

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
    buf.length = pCtx->nInbufPlanes;;
    buf.memory = pCtx->videoInstInfo.nMemoryType;
    if (exynos_v4l2_dqbuf(pCtx->hEnc, &buf) != 0) {
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
 * [Encoder Buffer OPS] ExtensionEnqueue (Output)
 */
static ExynosVideoErrorType MFC_Encoder_ExtensionEnqueue_Outbuf(
    void          *pHandle,
    void          *pBuffer[],
    int            pFd[],
    unsigned int   allocLen[],
    unsigned int   dataSize[],
    int            nPlanes,
    void          *pPrivate)
{
    ExynosVideoEncContext *pCtx   = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret    = VIDEO_ERROR_NONE;
    pthread_mutex_t       *pMutex = NULL;

    struct v4l2_plane planes[VIDEO_BUFFER_MAX_PLANES];
    struct v4l2_buffer buf;
    int index, i;

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
    index = MFC_Encoder_FindEmpty_Outbuf(pCtx);
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
        pCtx->pOutbuf[buf.index].planes[i].addr = pBuffer[i];
        pCtx->pOutbuf[buf.index].planes[i].fd = pFd[i];
        pCtx->pOutbuf[buf.index].planes[i].allocSize = allocLen[i];
    }

    pCtx->pOutbuf[buf.index].pPrivate = pPrivate;
    pCtx->pOutbuf[buf.index].bQueued = VIDEO_TRUE;
    pthread_mutex_unlock(pMutex);

    if (exynos_v4l2_qbuf(pCtx->hEnc, &buf) != 0) {
        ALOGE("%s: Failed to enqueue output buffer", __func__);
        pthread_mutex_lock(pMutex);
        pCtx->pOutbuf[buf.index].pPrivate = NULL;
        pCtx->pOutbuf[buf.index].bQueued = VIDEO_FALSE;
        pthread_mutex_unlock(pMutex);
        ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

EXIT:
    return ret;
}


/*
 * [Encoder Buffer OPS] ExtensionDequeue (Output)
 */
static ExynosVideoErrorType MFC_Encoder_ExtensionDequeue_Outbuf(
    void              *pHandle,
    ExynosVideoBuffer *pVideoBuffer)
{
    ExynosVideoEncContext *pCtx    = (ExynosVideoEncContext *)pHandle;
    ExynosVideoErrorType   ret     = VIDEO_ERROR_NONE;
    ExynosVideoBuffer     *pOutbuf = NULL;
    pthread_mutex_t       *pMutex = NULL;
    struct v4l2_buffer buf;
    struct v4l2_plane  planes[VIDEO_BUFFER_MAX_PLANES];
    int value, plane;

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

    if (pCtx->bShareOutbuf == VIDEO_TRUE)
        buf.memory = pCtx->videoInstInfo.nMemoryType;
    else
        buf.memory = V4L2_MEMORY_MMAP;

    /* returning DQBUF_EIO means MFC H/W status is invalid */
    if (exynos_v4l2_dqbuf(pCtx->hEnc, &buf) != 0) {
        if (errno == EIO)
            ret = VIDEO_ERROR_DQBUF_EIO;
        else
            ret = VIDEO_ERROR_APIFAIL;
        goto EXIT;
    }

    pMutex = (pthread_mutex_t*)pCtx->pOutMutex;
    pthread_mutex_lock(pMutex);

    pOutbuf = &pCtx->pOutbuf[buf.index];
    for (plane = 0; plane < pCtx->nOutbufPlanes; plane++)
        pOutbuf->planes[plane].dataSize = buf.m.planes[plane].bytesused;

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
        ALOGI("%s: encoded frame type is = %d",__func__, (buf.flags & (0x7 << 3)));
        pOutbuf->frameType = VIDEO_FRAME_OTHERS;
        break;
    };

    if (pCtx->pOutbuf[buf.index].bQueued == VIDEO_TRUE)
        memcpy(pVideoBuffer, pOutbuf, sizeof(ExynosVideoBuffer));
    else
        ret = VIDEO_ERROR_NOBUFFERS;
    memset(pOutbuf, 0, sizeof(ExynosVideoBuffer));

    pCtx->pOutbuf[buf.index].bQueued = VIDEO_FALSE;
    pthread_mutex_unlock(pMutex);

EXIT:
    return ret;
}

/*
 * [Encoder OPS] Common
 */
static ExynosVideoEncOps defEncOps = {
    .nSize                      = 0,
    .Init                       = MFC_Encoder_Init,
    .Finalize                   = MFC_Encoder_Finalize,
    .Set_EncParam               = MFC_Encoder_Set_EncParam,
    .Set_FrameType              = MFC_Encoder_Set_FrameType,
    .Set_FrameRate              = MFC_Encoder_Set_FrameRate,
    .Set_BitRate                = MFC_Encoder_Set_BitRate,
    .Set_QpRange                = MFC_Encoder_Set_QuantizationRange,
    .Set_FrameSkip              = MFC_Encoder_Set_FrameSkip,
    .Set_IDRPeriod              = MFC_Encoder_Set_IDRPeriod,
    .Set_FrameTag               = MFC_Encoder_Set_FrameTag,
    .Get_FrameTag               = MFC_Encoder_Get_FrameTag,
    .Enable_PrependSpsPpsToIdr  = MFC_Encoder_Enable_PrependSpsPpsToIdr,
    .Set_QosRatio               = MFC_Encoder_Set_QosRatio,
    .Set_LayerChange            = MFC_Encoder_Set_LayerChange,
#ifdef CID_SUPPORT
    .Set_DynamicQpControl       = MFC_Encoder_Set_DynamicQpControl,
    .Set_MarkLTRFrame           = MFC_Encoder_Set_MarkLTRFrame,
    .Set_UsedLTRFrame           = MFC_Encoder_Set_UsedLTRFrame,
    .Set_BasePID                = MFC_Encoder_Set_BasePID,
#endif
#ifdef USE_MFC_MEDIA
    .Set_RoiInfo                = MFC_Encoder_Set_RoiInfo,
#endif
};

/*
 * [Encoder Buffer OPS] Input
 */
static ExynosVideoEncBufferOps defInbufOps = {
    .nSize                  = 0,
    .Enable_Cacheable       = MFC_Encoder_Enable_Cacheable_Inbuf,
    .Set_Shareable          = MFC_Encoder_Set_Shareable_Inbuf,
    .Get_Buffer             = NULL,
    .Set_Geometry           = MFC_Encoder_Set_Geometry_Inbuf,
    .Get_Geometry           = MFC_Encoder_Get_Geometry_Inbuf,
    .Setup                  = MFC_Encoder_Setup_Inbuf,
    .Run                    = MFC_Encoder_Run_Inbuf,
    .Stop                   = MFC_Encoder_Stop_Inbuf,
    .Enqueue                = MFC_Encoder_Enqueue_Inbuf,
    .Enqueue_All            = NULL,
    .Dequeue                = MFC_Encoder_Dequeue_Inbuf,
    .Register               = MFC_Encoder_Register_Inbuf,
    .Clear_RegisteredBuffer = MFC_Encoder_Clear_RegisteredBuffer_Inbuf,
    .Clear_Queue            = MFC_Encoder_Clear_Queued_Inbuf,
    .ExtensionEnqueue       = MFC_Encoder_ExtensionEnqueue_Inbuf,
    .ExtensionDequeue       = MFC_Encoder_ExtensionDequeue_Inbuf,
};

/*
 * [Encoder Buffer OPS] Output
 */
static ExynosVideoEncBufferOps defOutbufOps = {
    .nSize                  = 0,
    .Enable_Cacheable       = MFC_Encoder_Enable_Cacheable_Outbuf,
    .Set_Shareable          = MFC_Encoder_Set_Shareable_Outbuf,
    .Get_Buffer             = MFC_Encoder_Get_Buffer_Outbuf,
    .Set_Geometry           = MFC_Encoder_Set_Geometry_Outbuf,
    .Get_Geometry           = MFC_Encoder_Get_Geometry_Outbuf,
    .Setup                  = MFC_Encoder_Setup_Outbuf,
    .Run                    = MFC_Encoder_Run_Outbuf,
    .Stop                   = MFC_Encoder_Stop_Outbuf,
    .Enqueue                = MFC_Encoder_Enqueue_Outbuf,
    .Enqueue_All            = NULL,
    .Dequeue                = MFC_Encoder_Dequeue_Outbuf,
    .Register               = MFC_Encoder_Register_Outbuf,
    .Clear_RegisteredBuffer = MFC_Encoder_Clear_RegisteredBuffer_Outbuf,
    .Clear_Queue            = MFC_Encoder_Clear_Queued_Outbuf,
    .ExtensionEnqueue       = MFC_Encoder_ExtensionEnqueue_Outbuf,
    .ExtensionDequeue       = MFC_Encoder_ExtensionDequeue_Outbuf,
};

ExynosVideoErrorType MFC_Exynos_Video_GetInstInfo_Encoder(
    ExynosVideoInstInfo *pVideoInstInfo)
{
    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;
    int hEnc = -1;
    int mode = 0, version = 0;

    if (pVideoInstInfo == NULL) {
        ALOGE("%s: bad parameter", __func__);
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

#ifdef USE_HEVC_HWIP
    if (pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC)
        hEnc = exynos_v4l2_open_devname(VIDEO_HEVC_ENCODER_NAME, O_RDWR, 0);
    else
#endif
        hEnc = exynos_v4l2_open_devname(VIDEO_ENCODER_NAME, O_RDWR, 0);

    if (hEnc < 0) {
        ALOGE("%s: Failed to open decoder device", __func__);
        ret = VIDEO_ERROR_OPENFAIL;
        goto EXIT;
    }

    if (exynos_v4l2_g_ctrl(hEnc, V4L2_CID_MPEG_MFC_GET_VERSION_INFO, &version) != 0) {
        ALOGW("%s: MFC version information is not available", __func__);
#ifdef USE_HEVC_HWIP
        if (pVideoInstInfo->eCodecType == VIDEO_CODING_HEVC)
            pVideoInstInfo->HwVersion = (int)HEVC_10;
        else
#endif
            pVideoInstInfo->HwVersion = (int)MFC_65;
    } else {
        pVideoInstInfo->HwVersion = version;
    }

    if (exynos_v4l2_g_ctrl(hEnc, V4L2_CID_MPEG_MFC_GET_EXT_INFO, &mode) != 0) {
        pVideoInstInfo->specificInfo.enc.bRGBSupport = VIDEO_FALSE;
        pVideoInstInfo->specificInfo.enc.nSpareSize = 0;
        pVideoInstInfo->specificInfo.enc.bTemporalSvcSupport = VIDEO_FALSE;
        pVideoInstInfo->specificInfo.enc.bRoiInfoSupport = VIDEO_FALSE;
        pVideoInstInfo->specificInfo.enc.bQpRangePBSupport = VIDEO_FALSE;
        goto EXIT;
    }

    pVideoInstInfo->specificInfo.enc.bQpRangePBSupport = (mode & (0x1 << 5))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->specificInfo.enc.bRoiInfoSupport = (mode & (0x1 << 4))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->specificInfo.enc.bSkypeSupport = (mode & (0x1 << 3))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->specificInfo.enc.bTemporalSvcSupport = (mode & (0x1 << 2))? VIDEO_TRUE:VIDEO_FALSE;
    pVideoInstInfo->specificInfo.enc.bRGBSupport = (mode & (0x1 << 0))? VIDEO_TRUE:VIDEO_FALSE;
    if (mode & (0x1 << 1)) {
        if (exynos_v4l2_g_ctrl(hEnc, V4L2_CID_MPEG_MFC_GET_EXTRA_BUFFER_SIZE, &(pVideoInstInfo->specificInfo.enc.nSpareSize)) != 0) {
            ALOGE("%s: g_ctrl is failed(V4L2_CID_MPEG_MFC_GET_EXTRA_BUFFER_SIZE)", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }
    }

    pVideoInstInfo->SwVersion = 0;
#ifdef CID_SUPPORT
    if (pVideoInstInfo->specificInfo.enc.bSkypeSupport == VIDEO_TRUE) {
        int swVersion = 0;

        if (exynos_v4l2_g_ctrl(hEnc, V4L2_CID_MPEG_MFC_GET_DRIVER_INFO, &swVersion) != 0) {
            ALOGE("%s: g_ctrl is failed(V4L2_CID_MPEG_MFC_GET_EXTRA_BUFFER_SIZE)", __func__);
            ret = VIDEO_ERROR_APIFAIL;
            goto EXIT;
        }

        pVideoInstInfo->SwVersion = (unsigned long long)swVersion;
    }
#endif

    __Set_SupportFormat(pVideoInstInfo);

EXIT:
    if (hEnc >= 0)
        exynos_v4l2_close(hEnc);

    return ret;
}

int MFC_Exynos_Video_Register_Encoder(
    ExynosVideoEncOps *pEncOps,
    ExynosVideoEncBufferOps *pInbufOps,
    ExynosVideoEncBufferOps *pOutbufOps)
{
    ExynosVideoErrorType ret = VIDEO_ERROR_NONE;

    if ((pEncOps == NULL) || (pInbufOps == NULL) || (pOutbufOps == NULL)) {
        ret = VIDEO_ERROR_BADPARAM;
        goto EXIT;
    }

    defEncOps.nSize = sizeof(defEncOps);
    defInbufOps.nSize = sizeof(defInbufOps);
    defOutbufOps.nSize = sizeof(defOutbufOps);

    memcpy((char *)pEncOps + sizeof(pEncOps->nSize), (char *)&defEncOps + sizeof(defEncOps.nSize),
            pEncOps->nSize - sizeof(pEncOps->nSize));

    memcpy((char *)pInbufOps + sizeof(pInbufOps->nSize), (char *)&defInbufOps + sizeof(defInbufOps.nSize),
            pInbufOps->nSize - sizeof(pInbufOps->nSize));

    memcpy((char *)pOutbufOps + sizeof(pOutbufOps->nSize), (char *)&defOutbufOps + sizeof(defOutbufOps.nSize),
            pOutbufOps->nSize - sizeof(pOutbufOps->nSize));

EXIT:
    return ret;
}
