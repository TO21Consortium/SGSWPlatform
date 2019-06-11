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
 * @file        Exynos_OSAL_ETC.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <system/graphics.h>

#include "Exynos_OSAL_Memory.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OMX_Macros.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_VIDEO_OSAL"
//#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"


#ifdef PERFORMANCE_DEBUG
#include <time.h>
#include "Exynos_OSAL_Mutex.h"

#define INPUT_PORT_INDEX    0
#define OUTPUT_PORT_INDEX   1
#endif

#include "ExynosVideoApi.h"
#include "exynos_format.h"

static struct timeval perfStart[PERF_ID_MAX+1], perfStop[PERF_ID_MAX+1];
static unsigned long perfTime[PERF_ID_MAX+1], totalPerfTime[PERF_ID_MAX+1];
static unsigned int perfFrameCount[PERF_ID_MAX+1], perfOver30ms[PERF_ID_MAX+1];

size_t Exynos_OSAL_Strcpy(OMX_PTR dest, OMX_PTR src)
{
    return strlcpy(dest, src, (size_t)(strlen((const char *)src) + 1));
}

size_t Exynos_OSAL_Strncpy(OMX_PTR dest, OMX_PTR src, size_t num)
{
    return strlcpy(dest, src, (size_t)(num + 1));
}

OMX_S32 Exynos_OSAL_Strcmp(OMX_PTR str1, OMX_PTR str2)
{
    return strcmp(str1, str2);
}

OMX_S32 Exynos_OSAL_Strncmp(OMX_PTR str1, OMX_PTR str2, size_t num)
{
    return strncmp(str1, str2, num);
}

const char* Exynos_OSAL_Strstr(const char *str1, const char *str2)
{
    return strstr(str1, str2);
}

size_t Exynos_OSAL_Strcat(OMX_PTR dest, OMX_PTR src)
{
    return strlcat(dest, src, (size_t)(strlen((const char *)dest) + strlen((const char *)src) + 1));
}

size_t Exynos_OSAL_Strncat(OMX_PTR dest, OMX_PTR src, size_t num)
{
    size_t len = num;

    /* caution : num should be a size of dest buffer */
    return strlcat(dest, src, (size_t)(strlen((const char *)dest) + strlen((const char *)src) + 1));
}

size_t Exynos_OSAL_Strlen(const char *str)
{
    return strlen(str);
}

static OMX_S32 Exynos_OSAL_MeasureTime(struct timeval *start, struct timeval *stop)
{
    signed long sec, usec, time;

    sec = stop->tv_sec - start->tv_sec;
    if (stop->tv_usec >= start->tv_usec) {
        usec = (signed long)stop->tv_usec - (signed long)start->tv_usec;
    } else {
        usec = (signed long)stop->tv_usec + 1000000 - (signed long)start->tv_usec;
        sec--;
    }

    time = sec * 1000000 + (usec);

    return time;
}

void Exynos_OSAL_PerfInit(PERF_ID_TYPE id)
{
    Exynos_OSAL_Memset(&perfStart[id], 0, sizeof(perfStart[id]));
    Exynos_OSAL_Memset(&perfStop[id], 0, sizeof(perfStop[id]));
    perfTime[id] = 0;
    totalPerfTime[id] = 0;
    perfFrameCount[id] = 0;
    perfOver30ms[id] = 0;
}

void Exynos_OSAL_PerfStart(PERF_ID_TYPE id)
{
    gettimeofday(&perfStart[id], NULL);
}

void Exynos_OSAL_PerfStop(PERF_ID_TYPE id)
{
    gettimeofday(&perfStop[id], NULL);

    perfTime[id] = Exynos_OSAL_MeasureTime(&perfStart[id], &perfStop[id]);
    totalPerfTime[id] += perfTime[id];
    perfFrameCount[id]++;

    if (perfTime[id] > 30000)
        perfOver30ms[id]++;
}

OMX_U32 Exynos_OSAL_PerfFrame(PERF_ID_TYPE id)
{
    return perfTime[id];
}

OMX_U32 Exynos_OSAL_PerfTotal(PERF_ID_TYPE id)
{
    return totalPerfTime[id];
}

OMX_U32 Exynos_OSAL_PerfFrameCount(PERF_ID_TYPE id)
{
    return perfFrameCount[id];
}

int Exynos_OSAL_PerfOver30ms(PERF_ID_TYPE id)
{
    return perfOver30ms[id];
}

void Exynos_OSAL_PerfPrint(OMX_STRING prefix, PERF_ID_TYPE id)
{
    OMX_U32 perfTotal;
    int frameCount;

    frameCount = Exynos_OSAL_PerfFrameCount(id);
    perfTotal = Exynos_OSAL_PerfTotal(id);

    Exynos_OSAL_Log(EXYNOS_LOG_INFO, "%s Frame Count: %d", prefix, frameCount);
    Exynos_OSAL_Log(EXYNOS_LOG_INFO, "%s Avg Time: %.2f ms, Over 30ms: %d",
                prefix, (float)perfTotal / (float)(frameCount * 1000),
                Exynos_OSAL_PerfOver30ms(id));
}

unsigned int Exynos_OSAL_GetPlaneCount(
    OMX_COLOR_FORMATTYPE eOMXFormat,
    PLANE_TYPE           ePlaneType)
{
    unsigned int nPlaneCnt = 0;

    switch ((int)eOMXFormat) {
    case OMX_COLOR_FormatYCbYCr:
    case OMX_COLOR_FormatYUV420Planar:
    case OMX_SEC_COLOR_FormatYVU420Planar:
        nPlaneCnt = 3;
        break;
    case OMX_COLOR_FormatYUV420SemiPlanar:
    case OMX_SEC_COLOR_FormatYUV420SemiPlanarInterlace:
    case OMX_SEC_COLOR_Format10bitYUV420SemiPlanar:
    case OMX_SEC_COLOR_FormatNV21Linear:
    case OMX_SEC_COLOR_FormatNV12Tiled:
        nPlaneCnt = 2;
        break;
    case OMX_COLOR_Format32bitARGB8888:
    case OMX_COLOR_Format32bitBGRA8888:
    case OMX_COLOR_Format32BitRGBA8888:
        nPlaneCnt = 1;
        break;
    default:
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: unsupported color format(%x).", __FUNCTION__, eOMXFormat);
        nPlaneCnt = 0;
        break;
    }

    if ((ePlaneType & PLANE_SINGLE) &&
        (nPlaneCnt > 0)) {
        nPlaneCnt = 1;
    }

    return nPlaneCnt;
}

void Exynos_OSAL_GetPlaneSize(
    OMX_COLOR_FORMATTYPE    eColorFormat,
    PLANE_TYPE              ePlaneType,
    OMX_U32                 nWidth,
    OMX_U32                 nHeight,
    unsigned int            nDataLen[MAX_BUFFER_PLANE],
    unsigned int            nAllocLen[MAX_BUFFER_PLANE])
{
    switch ((int)eColorFormat) {
    case OMX_COLOR_FormatYUV420Planar:
    case (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatYVU420Planar:
        if (ePlaneType == PLANE_SINGLE) {
            nDataLen[0] = (nWidth * nHeight) * 3 / 2;

            nAllocLen[0] = GET_Y_SIZE(nWidth, nHeight) + GET_CB_SIZE(nWidth, nHeight) + GET_CR_SIZE(nWidth, nHeight);
        } else {
            nDataLen[0] = nWidth * nHeight;
            nDataLen[1] = nDataLen[0] >> 2;
            nDataLen[2] = nDataLen[1];

            nAllocLen[0] = ALIGN(ALIGN(nWidth, 16) * ALIGN(nHeight, 16), 256);
            nAllocLen[1] = ALIGN(ALIGN(nWidth >> 1, 16) * (ALIGN(nHeight, 16) >> 1), 256);
            nAllocLen[2] = nAllocLen[1];
        }
        break;
    case OMX_COLOR_FormatYUV420SemiPlanar:
    case (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV21Linear:
    case (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV12Tiled:
        if (ePlaneType == PLANE_SINGLE) {
            nDataLen[0] = (nWidth * nHeight) * 3 / 2;

            nAllocLen[0] = GET_Y_SIZE(nWidth, nHeight) + GET_UV_SIZE(nWidth, nHeight);
        } else {
            nDataLen[0] = nWidth * nHeight;
            nDataLen[1] = nDataLen[0] >> 1;

            nAllocLen[0] = ALIGN(ALIGN(nWidth, 16) * ALIGN(nHeight, 16), 256);
            nAllocLen[1] = ALIGN(ALIGN(nWidth, 16) * ALIGN(nHeight, 16) >> 1, 256);
        }
        break;
    case OMX_SEC_COLOR_Format10bitYUV420SemiPlanar:

        if (ePlaneType == PLANE_SINGLE) {
            nDataLen[0] = (nWidth * nHeight) * 3 / 2;

            nAllocLen[0] = GET_10B_Y_SIZE(nWidth, nHeight) + GET_10B_UV_SIZE(nWidth, nHeight);
        } else {
            nDataLen[0] = nWidth * nHeight;
            nDataLen[1] = nDataLen[0] >> 1;

            nAllocLen[0] = ALIGN((ALIGN(nWidth, 16) * ALIGN(nHeight, 16) + 64) + (ALIGN(nWidth / 4, 16) * nHeight + 64), 256);
            nAllocLen[1] = ALIGN((ALIGN(nWidth, 16) * ALIGN(nHeight, 16) + 64) + (ALIGN(nWidth / 4, 16) * (nHeight / 2) + 64), 256);
        }
        break;
    case OMX_COLOR_Format32bitARGB8888:
    case OMX_COLOR_Format32bitBGRA8888:
    case OMX_COLOR_Format32BitRGBA8888:
        nDataLen[0] = nWidth * nHeight * 4;

        nAllocLen[0] = ALIGN(ALIGN(nWidth, 16) * ALIGN(nHeight, 16) * 4, 256);
        break;
    default:
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: unsupported color format(%x).", __func__, eColorFormat);
        break;
    }
}

static struct {
    ExynosVideoColorFormatType  eVideoFormat[2];  /* multi-FD, single-FD(H/W) */
    OMX_COLOR_FORMATTYPE        eOMXFormat;
} VIDEO_COLORFORMAT_MAP[] = {
{{VIDEO_COLORFORMAT_NV12M, VIDEO_COLORFORMAT_NV12}, OMX_COLOR_FormatYUV420SemiPlanar},
{{VIDEO_COLORFORMAT_NV12M, VIDEO_COLORFORMAT_NV12}, (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatYUV420SemiPlanarInterlace},
{{VIDEO_COLORFORMAT_NV12M, VIDEO_COLORFORMAT_NV12}, (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_Format10bitYUV420SemiPlanar},
{{VIDEO_COLORFORMAT_NV12M_TILED, VIDEO_COLORFORMAT_NV12_TILED}, (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV12Tiled},
{{VIDEO_COLORFORMAT_NV21M, 0 /* not supported */}, (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV21Linear},
{{VIDEO_COLORFORMAT_I420M, VIDEO_COLORFORMAT_I420}, OMX_COLOR_FormatYUV420Planar},
{{VIDEO_COLORFORMAT_YV12M, 0 /* not supported */}, (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatYVU420Planar},
{{VIDEO_COLORFORMAT_ARGB8888, VIDEO_COLORFORMAT_ARGB8888}, OMX_COLOR_Format32bitBGRA8888},
{{VIDEO_COLORFORMAT_BGRA8888, VIDEO_COLORFORMAT_BGRA8888}, OMX_COLOR_Format32bitARGB8888},
{{VIDEO_COLORFORMAT_RGBA8888, VIDEO_COLORFORMAT_RGBA8888}, (OMX_COLOR_FORMATTYPE)OMX_COLOR_Format32BitRGBA8888},
};

int Exynos_OSAL_OMX2VideoFormat(
    OMX_COLOR_FORMATTYPE    eColorFormat,
    PLANE_TYPE              ePlaneType)
{
    ExynosVideoColorFormatType nVideoFormat = VIDEO_COLORFORMAT_UNKNOWN;
    int nVideoFormats = (int)(sizeof(VIDEO_COLORFORMAT_MAP)/sizeof(VIDEO_COLORFORMAT_MAP[0]));
    int i;

    for (i = 0; i < nVideoFormats; i++) {
        if (VIDEO_COLORFORMAT_MAP[i].eOMXFormat == eColorFormat) {
            nVideoFormat = VIDEO_COLORFORMAT_MAP[i].eVideoFormat[(ePlaneType & 0x10)? 1:0];
            break;
        }
    }

    if (nVideoFormat == VIDEO_COLORFORMAT_UNKNOWN) {
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "OMX(%x)/Plane type(%x) is not supported", eColorFormat, ePlaneType);
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "OMX(%x)/Plane type(%x) -> VIDEO(%x) is changed", eColorFormat, ePlaneType, nVideoFormat);

EXIT:

    return (int)nVideoFormat;
}

OMX_COLOR_FORMATTYPE Exynos_OSAL_Video2OMXFormat(
    int nVideoFormat)
{
    OMX_COLOR_FORMATTYPE eOMXFormat = OMX_COLOR_FormatUnused;
    int nVideoFormats = (int)(sizeof(VIDEO_COLORFORMAT_MAP)/sizeof(VIDEO_COLORFORMAT_MAP[0]));
    int i;

    for (i = 0; i < nVideoFormats; i++) {
        if (((int)VIDEO_COLORFORMAT_MAP[i].eVideoFormat[0] == nVideoFormat) ||
            ((int)VIDEO_COLORFORMAT_MAP[i].eVideoFormat[1] == nVideoFormat)) {
            eOMXFormat = VIDEO_COLORFORMAT_MAP[i].eOMXFormat;
            break;
        }
    }

    if (eOMXFormat == OMX_COLOR_FormatUnused) {
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "VIDEO(%x) is not supported", nVideoFormat);
        goto EXIT;
    }

#if 0
    if ((eOMXFormat == OMX_SEC_COLOR_FormatYUV420SemiPlanarInterlace) ||
        (eOMXFormat == OMX_SEC_COLOR_Format10bitYUV420SemiPlanar)) {
        eOMXFormat = OMX_COLOR_FormatYUV420SemiPlanar;  /* hiding */
    }
#endif

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "VIDEO(%x) -> OMX(%x) is changed", nVideoFormat, eOMXFormat);

EXIT:


    return eOMXFormat;
}

static struct {
    unsigned int         nHALFormat[PLANE_MAX_NUM];  /* multi-FD, single-FD(H/W), sigle-FD(general) */
    OMX_COLOR_FORMATTYPE eOMXFormat;
} HAL_COLORFORMAT_MAP[] = {
/* NV12 format */
{{HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP},
     OMX_COLOR_FormatYUV420SemiPlanar},

{{HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_PRIV, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP},
 (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatYUV420SemiPlanarInterlace},  /* deinterlacing at single-FD(H/W) is not supported */

{{HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_S10B, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_S10B, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP},
 (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_Format10bitYUV420SemiPlanar},

/* NV12T format */
{{HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SP_M_TILED, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_SPN_TILED, 0 /* not implemented */},
 (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV12Tiled},

/* NV21 format */
{{HAL_PIXEL_FORMAT_EXYNOS_YCrCb_420_SP_M, 0 /* not implemented */, HAL_PIXEL_FORMAT_YCrCb_420_SP},
 (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatNV21Linear},

/* I420P format */
{{HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P_M, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_PN, HAL_PIXEL_FORMAT_EXYNOS_YCbCr_420_P},
 OMX_COLOR_FormatYUV420Planar},

/* YV12 format */
{{HAL_PIXEL_FORMAT_EXYNOS_YV12_M, 0 /* not implemented */, HAL_PIXEL_FORMAT_YV12},
 (OMX_COLOR_FORMATTYPE)OMX_SEC_COLOR_FormatYVU420Planar},

/* RGB format */
{{HAL_PIXEL_FORMAT_RGBA_8888, HAL_PIXEL_FORMAT_RGBA_8888, HAL_PIXEL_FORMAT_RGBA_8888},
 (OMX_COLOR_FORMATTYPE)OMX_COLOR_Format32BitRGBA8888},

{{HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888, HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888, HAL_PIXEL_FORMAT_EXYNOS_ARGB_8888},
 OMX_COLOR_Format32bitBGRA8888},

{{HAL_PIXEL_FORMAT_BGRA_8888, HAL_PIXEL_FORMAT_BGRA_8888, HAL_PIXEL_FORMAT_BGRA_8888},
 OMX_COLOR_Format32bitARGB8888},
};

OMX_COLOR_FORMATTYPE Exynos_OSAL_HAL2OMXColorFormat(
    unsigned int nHALFormat)
{
    OMX_COLOR_FORMATTYPE eOMXFormat = OMX_COLOR_FormatUnused;
    int nHALFormats = (int)(sizeof(HAL_COLORFORMAT_MAP)/sizeof(HAL_COLORFORMAT_MAP[0]));
    int i;

    for (i = 0; i < nHALFormats; i++) {
        if ((HAL_COLORFORMAT_MAP[i].nHALFormat[0] == nHALFormat) ||
            (HAL_COLORFORMAT_MAP[i].nHALFormat[1] == nHALFormat) ||
            (HAL_COLORFORMAT_MAP[i].nHALFormat[2] == nHALFormat)) {
            eOMXFormat = HAL_COLORFORMAT_MAP[i].eOMXFormat;
            break;
        }
    }

    if (eOMXFormat == OMX_COLOR_FormatUnused) {
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "HAL(%x) is not supported", nHALFormat);
        goto EXIT;
    }

#if 0
    if ((eOMXFormat == OMX_SEC_COLOR_FormatYUV420SemiPlanarInterlace) ||
        (eOMXFormat == OMX_SEC_COLOR_Format10bitYUV420SemiPlanar)) {
        eOMXFormat = OMX_COLOR_FormatYUV420SemiPlanar;  /* hiding */
    }
#endif

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "HAL(%x) -> OMX(%x) is changed", nHALFormat, eOMXFormat);

EXIT:

    return eOMXFormat;
}

unsigned int Exynos_OSAL_OMX2HALPixelFormat(
    OMX_COLOR_FORMATTYPE eOMXFormat,
    PLANE_TYPE           ePlaneType)
{
    unsigned int nHALFormat = 0;
    int nHALFormats = (int)(sizeof(HAL_COLORFORMAT_MAP)/sizeof(HAL_COLORFORMAT_MAP[0]));
    int i;

    for (i = 0; i < nHALFormats; i++) {
        if (HAL_COLORFORMAT_MAP[i].eOMXFormat == eOMXFormat) {
            nHALFormat = HAL_COLORFORMAT_MAP[i].nHALFormat[ePlaneType & 0xF];
            break;
        }
    }

    if (nHALFormat == 0) {
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "OMX(%x)/Plane type(%x) is not supported", eOMXFormat, ePlaneType);
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "OMX(%x) -> HAL(%x) is changed", eOMXFormat, nHALFormat);

EXIT:

    return nHALFormat;
}

#ifdef PERFORMANCE_DEBUG
typedef struct _EXYNOS_OMX_COUNT
{
    OMX_HANDLETYPE mutex;
    OMX_S32        OMXQCount;
    OMX_S32        V4L2QCount;
    BUFFER_TIME    sBufferTime[MAX_TIMESTAMP];
    OMX_U32        bufferTimeIndex;
    OMX_TICKS      dstOutPreviousTimeStamp;
} EXYNOS_OMX_COUNT;

OMX_ERRORTYPE Exynos_OSAL_CountCreate(OMX_HANDLETYPE *hCountHandle)
{
    OMX_ERRORTYPE       ret         = OMX_ErrorNone;
    EXYNOS_OMX_COUNT   *countHandle = NULL;

    countHandle = (EXYNOS_OMX_COUNT *)Exynos_OSAL_Malloc(sizeof(EXYNOS_OMX_COUNT));
    if (countHandle == NULL) {
        *hCountHandle = NULL;
        return OMX_ErrorInsufficientResources;
    }
    Exynos_OSAL_Memset((OMX_PTR)countHandle, 0, sizeof(EXYNOS_OMX_COUNT));

    ret = Exynos_OSAL_MutexCreate(&(countHandle->mutex));
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Free(countHandle);
        *hCountHandle = NULL;
        goto EXIT;
    }

    countHandle->OMXQCount = 0;
    *hCountHandle = countHandle;

EXIT:
    return ret;
}

void Exynos_OSAL_CountTerminate(OMX_HANDLETYPE *hCountHandle)
{
    EXYNOS_OMX_COUNT *countHandle = (EXYNOS_OMX_COUNT *)*hCountHandle;

    if (countHandle == NULL)
        return;

    Exynos_OSAL_MutexTerminate(countHandle->mutex);
    Exynos_OSAL_Free(countHandle);
    *hCountHandle = NULL;

    return;
}

OMX_S32 Exynos_OSAL_CountIncrease(OMX_HANDLETYPE hCountHandle, OMX_BUFFERHEADERTYPE *OMXBufferHeader, OMX_U32 nPortIndex)
{
    EXYNOS_OMX_COUNT *countHandle = (EXYNOS_OMX_COUNT *)hCountHandle;
    OMX_U32 nextIndex = 0;
    OMX_S32 ret = -1;

    struct timeval currentTime;
    struct tm* ptm;
    char time_string[40];
    long milliseconds;

    if (countHandle == NULL)
        return ret;

    if (OMX_ErrorNone == Exynos_OSAL_MutexLock(countHandle->mutex)) {
        countHandle->OMXQCount++;
        nextIndex = (countHandle->bufferTimeIndex + 1) % MAX_TIMESTAMP;
        //Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "nextIndex:%d, countHandle->bufferTimeIndex:%d", nextIndex, countHandle->bufferTimeIndex);
        gettimeofday(&(currentTime), NULL);
        countHandle->sBufferTime[countHandle->bufferTimeIndex].currentTimeStamp = OMXBufferHeader->nTimeStamp;
        Exynos_OSAL_Memcpy(&countHandle->sBufferTime[countHandle->bufferTimeIndex].currentIncomingTime, &currentTime, sizeof(currentTime));
        countHandle->sBufferTime[countHandle->bufferTimeIndex].OMXQBufferCount  = countHandle->OMXQCount;
        countHandle->sBufferTime[countHandle->bufferTimeIndex].pBufferHeader    = OMXBufferHeader;
        countHandle->sBufferTime[nextIndex].previousTimeStamp       = OMXBufferHeader->nTimeStamp;
        Exynos_OSAL_Memcpy(&countHandle->sBufferTime[nextIndex].previousIncomingTime, &currentTime, sizeof(currentTime));

        countHandle->bufferTimeIndex = nextIndex;
        ret = countHandle->OMXQCount;
        Exynos_OSAL_MutexUnlock(countHandle->mutex);

        ptm = localtime (&currentTime.tv_sec);
        strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
        milliseconds = currentTime.tv_usec / 1000;
/*
        if (nPortIndex == INPUT_PORT_INDEX)
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "ETB time = %s.%03ld, header:0x%x, OMXQCount:%d", time_string, milliseconds, OMXBufferHeader, ret);
        else
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FTB time = %s.%03ld, header:0x%x, OMXQCount:%d", time_string, milliseconds, OMXBufferHeader, ret);
*/
    }

    return ret;
}

OMX_S32 Exynos_OSAL_CountDecrease(OMX_HANDLETYPE hCountHandle, OMX_BUFFERHEADERTYPE *OMXBufferHeader, OMX_U32 nPortIndex)
{
    EXYNOS_OMX_COUNT *countHandle = (EXYNOS_OMX_COUNT *)hCountHandle;
    OMX_U32 i = 0;
    OMX_S32 ret = -1;

    struct timeval currentTime;
    struct tm* ptm;
    char time_string[40];
    long milliseconds;

    if (countHandle == NULL)
        return ret;

    if (OMX_ErrorNone == Exynos_OSAL_MutexLock(countHandle->mutex)) {
        for (i = 0; i < MAX_TIMESTAMP; i++) {
            if (countHandle->sBufferTime[i].pBufferHeader == OMXBufferHeader) {
                countHandle->sBufferTime[i].pBufferHeader = NULL;
                if (nPortIndex == OUTPUT_PORT_INDEX)
                    countHandle->dstOutPreviousTimeStamp = countHandle->sBufferTime[i].currentTimeStamp;
                break;
            }
        }

        countHandle->OMXQCount--;
        ret = countHandle->OMXQCount;
        Exynos_OSAL_MutexUnlock(countHandle->mutex);

        if (i >= MAX_TIMESTAMP) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "can not find a bufferHeader !!");
            ret = OMX_ErrorNoMore;
        }

        gettimeofday(&(currentTime), NULL);
        ptm = localtime (&currentTime.tv_sec);
        strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
        milliseconds = currentTime.tv_usec / 1000;
/*
        if (nPortIndex == INPUT_PORT_INDEX)
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "EBD time = %s.%03ld, header:0x%x, OMXQCount:%d", time_string, milliseconds, OMXBufferHeader, ret);
        else
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FBD time = %s.%03ld, header:0x%x, OMXQCount:%d", time_string, milliseconds, OMXBufferHeader, ret);
*/
    }

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_GetCountInfoUseOMXBuffer(OMX_HANDLETYPE hCountHandle, OMX_BUFFERHEADERTYPE *OMXBufferHeader, BUFFER_TIME *pBufferInfo)
{
    OMX_ERRORTYPE     ret         = OMX_ErrorNone;
    EXYNOS_OMX_COUNT *countHandle = (EXYNOS_OMX_COUNT *)hCountHandle;
    OMX_U32 i = 0;

    if ((countHandle == NULL) ||
        (OMXBufferHeader == NULL) ||
        (pBufferInfo == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Error : OMX_ErrorBadParameter");
        return OMX_ErrorBadParameter;
    }

    if (OMX_ErrorNone == Exynos_OSAL_MutexLock(countHandle->mutex)) {
        for (i = 0; i < MAX_TIMESTAMP; i++) {
            if (countHandle->sBufferTime[i].pBufferHeader == OMXBufferHeader) {
                Exynos_OSAL_Memcpy(pBufferInfo, &(countHandle->sBufferTime[i]), sizeof(BUFFER_TIME));
                break;
            }
        }
        Exynos_OSAL_MutexUnlock(countHandle->mutex);
        if (i >= MAX_TIMESTAMP) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "can not find a bufferHeader !!");
            ret = OMX_ErrorNoMore;
        }
    }
    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_GetCountInfoUseTimestamp(OMX_HANDLETYPE hCountHandle, OMX_TICKS Timestamp, BUFFER_TIME *pBufferInfo)
{
    EXYNOS_OMX_COUNT *countHandle = (EXYNOS_OMX_COUNT *)hCountHandle;
    OMX_U32 i = 0;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if ((countHandle == NULL) ||
        (pBufferInfo == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Error : OMX_ErrorBadParameter");
        return OMX_ErrorBadParameter;
    }

    if (OMX_ErrorNone == Exynos_OSAL_MutexLock(countHandle->mutex)) {
        for (i = 0; i < MAX_TIMESTAMP; i++) {
            if (countHandle->sBufferTime[i].currentTimeStamp == Timestamp) {
                Exynos_OSAL_Memcpy(pBufferInfo, &(countHandle->sBufferTime[i]), sizeof(BUFFER_TIME));
                break;
            }
        }
        Exynos_OSAL_MutexUnlock(countHandle->mutex);
        if (i >= MAX_TIMESTAMP) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "can not find a timestamp !!");
            ret = OMX_ErrorNoMore;
        }
    }
    return ret;
}

OMX_S32 Exynos_OSAL_V4L2CountIncrease(OMX_HANDLETYPE hCountHandle, OMX_BUFFERHEADERTYPE *OMXBufferHeader, OMX_U32 nPortIndex)
{
    EXYNOS_OMX_COUNT *countHandle = (EXYNOS_OMX_COUNT *)hCountHandle;
    OMX_U32 i = 0, index = 0;
    OMX_S32 ret = -1;

    struct tm* ptm;
    char time_string[40];
    long milliseconds;

    if (countHandle == NULL)
        return ret;

    if (OMX_ErrorNone == Exynos_OSAL_MutexLock(countHandle->mutex)) {
        for (i = 0; i < MAX_TIMESTAMP; i++) {
            if (countHandle->sBufferTime[i].pBufferHeader == OMXBufferHeader) {
                index = i;
                break;
            }
        }

        if (i >= MAX_TIMESTAMP) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "can not find a bufferHeader !!");
            Exynos_OSAL_MutexUnlock(countHandle->mutex);
            return OMX_ErrorNoMore;
        }

        countHandle->V4L2QCount++;
        gettimeofday(&(countHandle->sBufferTime[index].V4L2QBufferTime), NULL);
        countHandle->sBufferTime[index].V4L2QBufferCount = countHandle->V4L2QCount;
        ret = countHandle->V4L2QCount;
        Exynos_OSAL_MutexUnlock(countHandle->mutex);

        ptm = localtime (&countHandle->sBufferTime[index].V4L2QBufferTime.tv_sec);
        strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
        milliseconds = countHandle->sBufferTime[index].V4L2deQBufferTime.tv_usec / 1000;
/*
        if (nPortIndex == INPUT_PORT_INDEX)
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "V4L2 Src Q time = %s.%03ld, header:0x%x, index:%d", time_string, milliseconds, OMXBufferHeader, index);
        else
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "V4L2 Dst Q time = %s.%03ld, header:0x%x", time_string, milliseconds, OMXBufferHeader);
*/
    }

    return ret;
}

OMX_S32 Exynos_OSAL_V4L2CountDecrease(OMX_HANDLETYPE hCountHandle, OMX_BUFFERHEADERTYPE *OMXBufferHeader, OMX_U32 nPortIndex)
{
    EXYNOS_OMX_COUNT *countHandle = (EXYNOS_OMX_COUNT *)hCountHandle;
    OMX_U32 i = 0, index = 0;
    OMX_S32 ret = -1;

    struct tm* ptm;
    char time_string[40];
    long milliseconds;

    if (countHandle == NULL)
        return ret;

    if (OMX_ErrorNone == Exynos_OSAL_MutexLock(countHandle->mutex)) {
        for (i = 0; i < MAX_TIMESTAMP; i++) {
            if (countHandle->sBufferTime[i].pBufferHeader == OMXBufferHeader) {
                index = i;
                break;
            }
        }

        if (i >= MAX_TIMESTAMP) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "can not find a bufferHeader !!");
            Exynos_OSAL_MutexUnlock(countHandle->mutex);
            return OMX_ErrorNoMore;
        }

        countHandle->V4L2QCount--;
        gettimeofday(&(countHandle->sBufferTime[index].V4L2deQBufferTime), NULL);
        if (nPortIndex == OUTPUT_PORT_INDEX) {
            countHandle->sBufferTime[index].currentTimeStamp  = OMXBufferHeader->nTimeStamp;
            countHandle->sBufferTime[index].previousTimeStamp = countHandle->dstOutPreviousTimeStamp;
        }
        ret = countHandle->V4L2QCount;
        Exynos_OSAL_MutexUnlock(countHandle->mutex);

        ptm = localtime (&countHandle->sBufferTime[index].V4L2deQBufferTime.tv_sec);
        strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
        milliseconds = countHandle->sBufferTime[index].V4L2deQBufferTime.tv_usec / 1000;
/*
        if (nPortIndex == INPUT_PORT_INDEX)
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "V4L2 Src deQ time = %s.%03ld, header:0x%x", time_string, milliseconds, OMXBufferHeader);
        else
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "V4L2 Dst deQ time = %s.%03ld, header:0x%x", time_string, milliseconds, OMXBufferHeader);
*/
    }

    return ret;
}

OMX_ERRORTYPE Exynos_OSAL_CountReset(OMX_HANDLETYPE hCountHandle)
{
    EXYNOS_OMX_COUNT *countHandle = (EXYNOS_OMX_COUNT *)hCountHandle;

    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (countHandle == NULL)
        return OMX_ErrorBadParameter;

    if (OMX_ErrorNone == Exynos_OSAL_MutexLock(countHandle->mutex)) {
        countHandle->bufferTimeIndex = 0;
        countHandle->OMXQCount = 0;
        countHandle->V4L2QCount = 0;
        Exynos_OSAL_Memset(&countHandle->sBufferTime, 0, sizeof(BUFFER_TIME) * MAX_TIMESTAMP);
        Exynos_OSAL_MutexUnlock(countHandle->mutex);
    }

    return ret;
}

void Exynos_OSAL_PrintCountInfo(BUFFER_TIME srcBufferInfo, BUFFER_TIME dstBufferInfo)
{
    struct timeval FBDTime;
    struct tm* ptm;
    char time_string[40];
    long milliseconds;

    OMX_S32 srcOMXtoV4L2 = 0;
    OMX_S32 srcV4L2toOMX = 0;
    OMX_S32 dstOMXtoV4L2 = 0;
    OMX_S32 dstV4L2toOMX = 0;

    OMX_S32 ETBFTB = 0;
    OMX_S32 ETBFBD = 0;
    OMX_S32 FTBFBD = 0;

    OMX_S32 srcQdstQ = 0;
    OMX_S32 srcQdstDQ = 0;
    OMX_S32 dstQdstDQ = 0;

    {
        gettimeofday(&FBDTime, NULL);

        srcOMXtoV4L2 = Exynos_OSAL_MeasureTime(&(srcBufferInfo.currentIncomingTime), &(srcBufferInfo.V4L2QBufferTime)) / 1000;
        //srcV4L2toOMX = Exynos_OSAL_MeasureTime(&(srcBufferInfo.V4L2deQBufferTime), &(srcBufferInfo.?????????????????)) / 1000;
        dstOMXtoV4L2 = Exynos_OSAL_MeasureTime(&(dstBufferInfo.currentIncomingTime), &(dstBufferInfo.V4L2QBufferTime)) / 1000;
        dstV4L2toOMX = Exynos_OSAL_MeasureTime(&(dstBufferInfo.V4L2deQBufferTime), &FBDTime) / 1000;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PERFORM:: INTERNAL: srcOMXtoV4L2:%d, srcV4L2toOMX:%d, dstOMXtoV4L2:%d, dstV4L2toOMX:%d", srcOMXtoV4L2, srcV4L2toOMX, dstOMXtoV4L2, dstV4L2toOMX);
    }

    {
        srcQdstQ = Exynos_OSAL_MeasureTime(&(srcBufferInfo.V4L2QBufferTime), &(dstBufferInfo.V4L2QBufferTime)) / 1000;
        srcQdstDQ = Exynos_OSAL_MeasureTime(&(srcBufferInfo.V4L2QBufferTime), &(dstBufferInfo.V4L2deQBufferTime)) / 1000;
        dstQdstDQ = Exynos_OSAL_MeasureTime(&(dstBufferInfo.V4L2QBufferTime), &(dstBufferInfo.V4L2deQBufferTime)) / 1000;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PERFORM:: V4L2: srcQdstQ:%d, srcQdstDQ:%d, dstQdstDQ:%d, src-V4L2QBufferCount:%d, dst-V4L2QBufferCount:%d",
                                            srcQdstQ, srcQdstDQ, dstQdstDQ, srcBufferInfo.V4L2QBufferCount, dstBufferInfo.V4L2QBufferCount);
    }

    {
        ETBFTB = Exynos_OSAL_MeasureTime(&(srcBufferInfo.currentIncomingTime), &(dstBufferInfo.currentIncomingTime)) / 1000;
        ETBFBD = Exynos_OSAL_MeasureTime(&(srcBufferInfo.currentIncomingTime), &FBDTime) / 1000;
        FTBFBD = Exynos_OSAL_MeasureTime(&(dstBufferInfo.currentIncomingTime), &FBDTime) / 1000;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PERFORM:: BUFFER: ETBFTB:%d, ETBFBD:%d, FTBFBD:%d, src-OMXQBufferCount:%d, dst-OMXQBufferCount:%d",
                                            ETBFTB, ETBFBD, FTBFBD, srcBufferInfo.OMXQBufferCount, dstBufferInfo.OMXQBufferCount);

        if (ETBFTB > 0)
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PERFORM:: BUFFER: Output port buffer slow. real decode time:%d", FTBFBD / dstBufferInfo.OMXQBufferCount);
        else if (ETBFTB < 0)
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PERFORM:: BUFFER: Input port buffer slow. real decode time:%d", ETBFBD / srcBufferInfo.OMXQBufferCount);
        else
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PERFORM:: BUFFER: real decode time:%d or %d", ETBFBD / srcBufferInfo.OMXQBufferCount, FTBFBD / dstBufferInfo.OMXQBufferCount);
    }

    {
        OMX_S32 srcBufferInterval;
        OMX_S32 dstBufferInterval;

        srcBufferInterval = Exynos_OSAL_MeasureTime(&(srcBufferInfo.previousIncomingTime), &(srcBufferInfo.currentIncomingTime)) / 1000;
        dstBufferInterval = Exynos_OSAL_MeasureTime(&(dstBufferInfo.previousIncomingTime), &(dstBufferInfo.currentIncomingTime)) / 1000;

        if ((srcBufferInterval / (srcBufferInfo.OMXQBufferCount - 1)) > ((dstBufferInfo.currentTimeStamp - dstBufferInfo.previousTimeStamp) / 1000))
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PERFORM:: SLOW: Warning!! src buffer slow. srcBuffer Interval:%d, correct interval:%d, OMXQBufferCount:%d",
                            srcBufferInterval,
                            (srcBufferInfo.OMXQBufferCount == 1) ? srcBufferInterval : (srcBufferInterval / (srcBufferInfo.OMXQBufferCount - 1)),
                            srcBufferInfo.OMXQBufferCount);
        if ((dstBufferInterval / (dstBufferInfo.OMXQBufferCount - 1)) > ((dstBufferInfo.currentTimeStamp - dstBufferInfo.previousTimeStamp) / 1000))
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PERFORM:: SLOW: Warning!! dst buffer slow. dstBuffer Interval:%d, correct interval:%d, OMXQBufferCount:%d",
                            dstBufferInterval,
                            (dstBufferInfo.OMXQBufferCount == 1) ? dstBufferInterval : (dstBufferInterval / (dstBufferInfo.OMXQBufferCount - 1)),
                            dstBufferInfo.OMXQBufferCount);
    }

    {
        OMX_S32 srcTimestampInterval;
        OMX_S32 dstTimestampInterval;

        srcTimestampInterval = ((OMX_S32)(srcBufferInfo.currentTimeStamp - srcBufferInfo.previousTimeStamp)) / 1000;
        dstTimestampInterval = ((OMX_S32)(dstBufferInfo.currentTimeStamp - dstBufferInfo.previousTimeStamp)) / 1000;

        if ((srcTimestampInterval > 0) && (dstTimestampInterval > 0))
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "PERFORM:: TYPE: Normal timestamp contents");
        else if ((srcTimestampInterval < 0) && (dstTimestampInterval > 0))
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "PERFORM:: TYPE: PTS timestamp contents");
        else if ((srcTimestampInterval > 0) && (dstTimestampInterval < 0))
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "PERFORM:: TYPE: DTS timestamp contents");
        else
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "PERFORM:: TYPE: Timestamp is strange!!");
    }

/*
    ptm = localtime (&srcBufferInfo.currentIncomingTime.tv_sec);
    strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
    milliseconds = srcBufferInfo.currentIncomingTime.tv_usec / 1000;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "ETB time = %s.%03ld\n", time_string, milliseconds);

    ptm = localtime (&dstBufferInfo.currentIncomingTime.tv_sec);
    strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
    milliseconds = dstBufferInfo.currentIncomingTime.tv_usec / 1000;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "FTB time = %s.%03ld\n", time_string, milliseconds);

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "return time = %d ms",
                            Exynos_OSAL_MeasureTime(&(srcBufferInfo.currentIncomingTime), &(dstBufferInfo.currentIncomingTime)) / 1000);
*/

    return;
}
#endif
