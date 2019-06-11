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
 * @file        Exynos_OSAL_ETC.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */

#ifndef Exynos_OSAL_ETC
#define Exynos_OSAL_ETC

#include "OMX_Types.h"
#include "Exynos_OMX_Def.h"

#ifdef PERFORMANCE_DEBUG
#include <sys/time.h>
#endif

#define INT_TO_PTR(var) ((void *)(unsigned long)var)
#define PTR_TO_INT(var) ((int)(unsigned long)var)

#ifdef __cplusplus
extern "C" {
#endif

size_t Exynos_OSAL_Strcpy(OMX_PTR dest, OMX_PTR src);
OMX_S32 Exynos_OSAL_Strncmp(OMX_PTR str1, OMX_PTR str2, size_t num);
OMX_S32 Exynos_OSAL_Strcmp(OMX_PTR str1, OMX_PTR str2);
const char* Exynos_OSAL_Strstr(const char *str1, const char *str2);
size_t Exynos_OSAL_Strcat(OMX_PTR dest, OMX_PTR src);
size_t Exynos_OSAL_Strlen(const char *str);
ssize_t getline(char **ppLine, size_t *len, FILE *stream);

/* perf */
typedef enum _PERF_ID_TYPE {
    PERF_ID_CSC = 0,
    PERF_ID_DEC,
    PERF_ID_ENC,
    PERF_ID_USER,
    PERF_ID_MAX,
} PERF_ID_TYPE;

void Exynos_OSAL_PerfInit(PERF_ID_TYPE id);
void Exynos_OSAL_PerfStart(PERF_ID_TYPE id);
void Exynos_OSAL_PerfStop(PERF_ID_TYPE id);
OMX_U32 Exynos_OSAL_PerfFrame(PERF_ID_TYPE id);
OMX_U32 Exynos_OSAL_PerfTotal(PERF_ID_TYPE id);
OMX_U32 Exynos_OSAL_PerfFrameCount(PERF_ID_TYPE id);
int Exynos_OSAL_PerfOver30ms(PERF_ID_TYPE id);
void Exynos_OSAL_PerfPrint(OMX_STRING prefix, PERF_ID_TYPE id);

unsigned int Exynos_OSAL_GetPlaneCount(OMX_COLOR_FORMATTYPE eOMXFormat, PLANE_TYPE ePlaneType);
void Exynos_OSAL_GetPlaneSize(OMX_COLOR_FORMATTYPE eColorFormat, PLANE_TYPE ePlaneType, OMX_U32 nWidth, OMX_U32 nHeight, unsigned int nDataLen[MAX_BUFFER_PLANE], unsigned int nAllocLen[MAX_BUFFER_PLANE]);

int Exynos_OSAL_OMX2VideoFormat(OMX_COLOR_FORMATTYPE eColorFormat, PLANE_TYPE ePlaneType);
OMX_COLOR_FORMATTYPE Exynos_OSAL_Video2OMXFormat(int nVideoFormat);

OMX_COLOR_FORMATTYPE Exynos_OSAL_HAL2OMXColorFormat(unsigned int nHALFormat);
unsigned int Exynos_OSAL_OMX2HALPixelFormat(OMX_COLOR_FORMATTYPE eOMXFormat, PLANE_TYPE ePlaneType);

inline static const char *stateString(OMX_STATETYPE i) {
     switch (i) {
         case OMX_StateInvalid:                 return "OMX_StateInvaild";
         case OMX_StateLoaded:                  return "OMX_StateLoaded";
         case OMX_StateIdle:                    return "OMX_StateIdle";
         case OMX_StateExecuting:               return "OMX_StateExecuting";
         case OMX_StatePause:                   return "OMX_StatePause";
         case OMX_StateWaitForResources:        return "OMX_StateWaitForResources";
         default:                               return "??";
     }
}

#ifdef PERFORMANCE_DEBUG
typedef struct _BUFFER_TIME
{
    OMX_TICKS             previousTimeStamp;
    OMX_TICKS             currentTimeStamp;
    struct timeval        previousIncomingTime;
    struct timeval        currentIncomingTime;
    OMX_S32               OMXQBufferCount;
    OMX_BUFFERHEADERTYPE *pBufferHeader;

    OMX_S32               V4L2QBufferCount;
    struct timeval        V4L2QBufferTime;
    struct timeval        V4L2deQBufferTime;
} BUFFER_TIME;

OMX_ERRORTYPE Exynos_OSAL_CountCreate(OMX_HANDLETYPE *phCountHandle);
void Exynos_OSAL_CountTerminate(OMX_HANDLETYPE *phCountHandle);
OMX_S32 Exynos_OSAL_CountIncrease(OMX_HANDLETYPE hCountHandle, OMX_BUFFERHEADERTYPE *OMXBufferHeader, OMX_U32 nPortIndex);
OMX_S32 Exynos_OSAL_CountDecrease(OMX_HANDLETYPE hCountHandle, OMX_BUFFERHEADERTYPE *OMXBufferHeader, OMX_U32 nPortIndex);
OMX_ERRORTYPE Exynos_OSAL_GetCountInfoUseOMXBuffer(OMX_HANDLETYPE hCountHandle, OMX_BUFFERHEADERTYPE *OMXBufferHeader, BUFFER_TIME *pBufferInfo);
OMX_ERRORTYPE Exynos_OSAL_GetCountInfoUseTimestamp(OMX_HANDLETYPE hCountHandle, OMX_TICKS Timestamp, BUFFER_TIME *pBufferInfo);
OMX_S32 Exynos_OSAL_V4L2CountIncrease(OMX_HANDLETYPE hCountHandle, OMX_BUFFERHEADERTYPE *OMXBufferHeader, OMX_U32 nPortIndex);
OMX_S32 Exynos_OSAL_V4L2CountDecrease(OMX_HANDLETYPE hCountHandle, OMX_BUFFERHEADERTYPE *OMXBufferHeader, OMX_U32 nPortIndex);
OMX_ERRORTYPE Exynos_OSAL_CountReset(OMX_HANDLETYPE hCountHandle);
void Exynos_OSAL_PrintCountInfo(BUFFER_TIME srcBufferInfo, BUFFER_TIME dstBufferInfo);
#endif

#ifdef __cplusplus
}
#endif

#endif
