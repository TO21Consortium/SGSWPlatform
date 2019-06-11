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
 * @file        Exynos_OMX_HEVCenc.h
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              Taehwan Kim (t_h.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2014.05.22 : Create
 */

#ifndef EXYNOS_OMX_HEVC_ENC_COMPONENT
#define EXYNOS_OMX_HEVC_ENC_COMPONENT

#include "Exynos_OMX_Def.h"
#include "OMX_Component.h"
#include "OMX_Video.h"

#include "ExynosVideoApi.h"

typedef struct _EXYNOS_MFC_HEVCENC_HANDLE
{
    OMX_HANDLETYPE hMFCHandle;

    OMX_U32 indexTimestamp;
    OMX_U32 outputIndexTimestamp;
    OMX_BOOL bConfiguredMFCSrc;
    OMX_BOOL bConfiguredMFCDst;
    OMX_BOOL bPrependSpsPpsToIdr;
    OMX_BOOL bTemporalSVC;
    OMX_BOOL bRoiInfo;

    ExynosVideoEncOps       *pEncOps;
    ExynosVideoEncBufferOps *pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps;
    ExynosVideoEncParam      encParam;
    ExynosVideoInstInfo      videoInstInfo;

    #define MAX_PROFILE_NUM 1
    OMX_VIDEO_HEVCPROFILETYPE   profiles[MAX_PROFILE_NUM];
    OMX_S32                     nProfileCnt;
    OMX_VIDEO_HEVCLEVELTYPE     maxLevel;
} EXYNOS_MFC_HEVCENC_HANDLE;

typedef struct _EXYNOS_HEVCENC_HANDLE
{
    /* OMX Codec specific */
    OMX_VIDEO_PARAM_HEVCTYPE            HevcComponent[ALL_PORT_NUM];
    OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE errorCorrectionType[ALL_PORT_NUM];
    OMX_U32                             nPFrames;  /* IDR period control */
    OMX_VIDEO_QPRANGE                   qpRangeI;
    OMX_VIDEO_QPRANGE                   qpRangeP;
    OMX_VIDEO_QPRANGE                   qpRangeB;
    EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC TemporalSVC;    /* Temporal SVC */

    /* SEC MFC Codec specific */
    EXYNOS_MFC_HEVCENC_HANDLE hMFCHevcHandle;

    OMX_BOOL bSourceStart;
    OMX_BOOL bDestinationStart;
    OMX_HANDLETYPE hSourceStartEvent;
    OMX_HANDLETYPE hDestinationStartEvent;

    EXYNOS_QUEUE bypassBufferInfoQ;
} EXYNOS_HEVCENC_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName);
OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
};
#endif

#endif
