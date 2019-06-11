/*
 *
 * Copyright 2014 Samsung Electronics S.LSI Co. LTD
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
 * @file       Exynos_OMX_Vp9dec.h
 * @brief
 * @author     Taehwan Kim (t_h.kim@samsung.com)
 *             SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version    2.0.0
 * @history
 *   2014.07.24 : Create
 */

#ifndef EXYNOS_OMX_VP9_DEC_COMPONENT
#define EXYNOS_OMX_VP9_DEC_COMPONENT

#include "Exynos_OMX_Def.h"
#include "OMX_Component.h"
#include "OMX_Video.h"
#include "ExynosVideoApi.h"


typedef struct _EXYNOS_MFC_VP9DEC_HANDLE
{
    OMX_HANDLETYPE             hMFCHandle;
    OMX_U32                    indexTimestamp;
    OMX_U32                    outputIndexTimestamp;
    OMX_BOOL                   bConfiguredMFCSrc;
    OMX_BOOL                   bConfiguredMFCDst;
    OMX_S32                    maxDPBNum;

    ExynosVideoColorFormatType MFCOutputColorType;
    ExynosVideoDecOps         *pDecOps;
    ExynosVideoDecBufferOps   *pInbufOps;
    ExynosVideoDecBufferOps   *pOutbufOps;
    ExynosVideoGeometry        codecOutbufConf;
    ExynosVideoInstInfo        videoInstInfo;
} EXYNOS_MFC_VP9DEC_HANDLE;

typedef struct _EXYNOS_VP9DEC_HANDLE
{
    /* OMX Codec specific */
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE errorCorrectionType[ALL_PORT_NUM];

    /* EXYNOS MFC Codec specific */
    EXYNOS_MFC_VP9DEC_HANDLE            hMFCVp9Handle;

    OMX_BOOL bSourceStart;
    OMX_BOOL bDestinationStart;
    OMX_HANDLETYPE hSourceStartEvent;
    OMX_HANDLETYPE hDestinationStartEvent;

    EXYNOS_QUEUE bypassBufferInfoQ;
} EXYNOS_VP9DEC_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(
    OMX_HANDLETYPE hComponent,
    OMX_STRING componentName);
OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(
    OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE VP9CodecDstSetup(
    OMX_COMPONENTTYPE *pOMXComponent);

#ifdef __cplusplus
};
#endif

#endif
