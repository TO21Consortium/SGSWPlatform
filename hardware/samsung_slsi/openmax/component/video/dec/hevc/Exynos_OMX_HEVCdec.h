/*
 *
 * Copyright 2013 Samsung Electronics S.LSI Co. LTD
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
 * @file       Exynos_OMX_HEVCdec.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              Taehwan Kim (t_h.kim@samsung.com)
 * @version    2.0.0
 * @history
 *   2013.07.26 : Create
 */

#ifndef EXYNOS_OMX_HEVC_DEC_COMPONENT
#define EXYNOS_OMX_HEVC_DEC_COMPONENT

#include "Exynos_OMX_Def.h"
#include "OMX_Component.h"
#include "OMX_Video.h"
#include "ExynosVideoApi.h"


typedef struct _EXYNOS_MFC_HEVCDEC_HANDLE
{
    OMX_HANDLETYPE             hMFCHandle;
    OMX_U32                    indexTimestamp;
    OMX_U32                    outputIndexTimestamp;
    OMX_BOOL                   bConfiguredMFCSrc;
    OMX_BOOL                   bConfiguredMFCDst;
    OMX_S32                    maxDPBNum;

    /* for custom component(MSRND) */
    #define MAX_HEVC_DISPLAYDELAY_VALIDNUM 8
    OMX_U32                    nDisplayDelay;

#ifdef USE_S3D_SUPPORT
    EXYNOS_OMX_FPARGMT_TYPE    S3DFPArgmtType;
#endif

    ExynosVideoColorFormatType MFCOutputColorType;
    ExynosVideoDecOps         *pDecOps;
    ExynosVideoDecBufferOps   *pInbufOps;
    ExynosVideoDecBufferOps   *pOutbufOps;
    ExynosVideoGeometry        codecOutbufConf;
    ExynosVideoInstInfo        videoInstInfo;

    #define MAX_PROFILE_NUM 1
    OMX_VIDEO_HEVCPROFILETYPE   profiles[MAX_PROFILE_NUM];
    OMX_S32                     nProfileCnt;
    OMX_VIDEO_HEVCLEVELTYPE     maxLevel;
} EXYNOS_MFC_HEVCDEC_HANDLE;

typedef struct _EXYNOS_HEVCDEC_HANDLE
{
    /* OMX Codec specific */
    OMX_VIDEO_PARAM_HEVCTYPE HevcComponent[ALL_PORT_NUM];
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE errorCorrectionType[ALL_PORT_NUM];

    /* EXYNOS MFC Codec specific */
    EXYNOS_MFC_HEVCDEC_HANDLE            hMFCHevcHandle;

    OMX_BOOL bSourceStart;
    OMX_BOOL bDestinationStart;
    OMX_HANDLETYPE hSourceStartEvent;
    OMX_HANDLETYPE hDestinationStartEvent;

    EXYNOS_QUEUE bypassBufferInfoQ;
} EXYNOS_HEVCDEC_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName);
OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent);
OMX_ERRORTYPE HevcCodecDstSetup(OMX_COMPONENTTYPE *pOMXComponent);

#ifdef __cplusplus
};
#endif

#endif
