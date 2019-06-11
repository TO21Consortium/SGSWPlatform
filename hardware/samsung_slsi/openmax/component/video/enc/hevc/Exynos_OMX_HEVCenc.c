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
 * @file        Exynos_OMX_HEVCenc.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 *              Taehwan Kim (t_h.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2014.05.22 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exynos_OMX_Macros.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Venc.h"
#include "Exynos_OMX_VencControl.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OSAL_Thread.h"
#include "library_register.h"
#include "Exynos_OMX_HEVCenc.h"
#include "Exynos_OSAL_SharedMemory.h"
#include "Exynos_OSAL_Event.h"
#include "Exynos_OSAL_Queue.h"

#ifdef USE_ANDROID
#include "Exynos_OSAL_Android.h"
#endif

/* To use CSC_METHOD_HW in EXYNOS OMX, gralloc should allocate physical memory using FIMC */
/* It means GRALLOC_USAGE_HW_FIMC1 should be set on Native Window usage */
#include "csc.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_HEVC_ENC"
//#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

static OMX_ERRORTYPE SetProfileLevel(
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    OMX_ERRORTYPE                    ret            = OMX_ErrorNone;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc      = NULL;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc       = NULL;

    int nProfileCnt = 0;

    FunctionIn();

    if (pExynosComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    if (pVideoEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pHevcEnc = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pHevcEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pHevcEnc->hMFCHevcHandle.profiles[nProfileCnt++] = OMX_VIDEO_HEVCProfileMain;
    pHevcEnc->hMFCHevcHandle.nProfileCnt = nProfileCnt;

    switch (pHevcEnc->hMFCHevcHandle.videoInstInfo.HwVersion) {
    case MFC_100:
    case MFC_101:
        pHevcEnc->hMFCHevcHandle.maxLevel = OMX_VIDEO_HEVCHighTierLevel51;
        break;
    case MFC_90:
    case MFC_1010:
        pHevcEnc->hMFCHevcHandle.maxLevel = OMX_VIDEO_HEVCHighTierLevel5;
        break;
    case MFC_1011:
    case MFC_1020:
        pHevcEnc->hMFCHevcHandle.maxLevel = OMX_VIDEO_HEVCHighTierLevel41;
        break;
    case MFC_92:
    default:
        pHevcEnc->hMFCHevcHandle.maxLevel = OMX_VIDEO_HEVCHighTierLevel4;
        break;
    }

EXIT:
    return ret;
}

static OMX_ERRORTYPE GetIndexToProfileLevel(
    EXYNOS_OMX_BASECOMPONENT         *pExynosComponent,
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *pProfileLevelType)
{
    OMX_ERRORTYPE                    ret            = OMX_ErrorNone;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc      = NULL;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc       = NULL;

    int nLevelCnt = 0;
    OMX_U32 nMaxIndex = 0;

    FunctionIn();

    if ((pExynosComponent == NULL) ||
        (pProfileLevelType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    if (pVideoEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pHevcEnc = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pHevcEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    while ((pHevcEnc->hMFCHevcHandle.maxLevel >> nLevelCnt) > 0) {
        nLevelCnt++;
    }

    if ((pHevcEnc->hMFCHevcHandle.nProfileCnt == 0) ||
        (nLevelCnt == 0)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : there is no any profile/level", __FUNCTION__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    nMaxIndex = pHevcEnc->hMFCHevcHandle.nProfileCnt * nLevelCnt;
    if (nMaxIndex <= pProfileLevelType->nProfileIndex) {
        ret = OMX_ErrorNoMore;
        goto EXIT;
    }

    pProfileLevelType->eProfile = pHevcEnc->hMFCHevcHandle.profiles[pProfileLevelType->nProfileIndex / nLevelCnt];
    pProfileLevelType->eLevel = 0x1 << (pProfileLevelType->nProfileIndex % nLevelCnt);

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s : supported profile(%x), level(%x)", __FUNCTION__, pProfileLevelType->eProfile, pProfileLevelType->eLevel);

EXIT:
    return ret;
}

static OMX_BOOL CheckProfileLevelSupport(
    EXYNOS_OMX_BASECOMPONENT         *pExynosComponent,
    OMX_VIDEO_PARAM_PROFILELEVELTYPE *pProfileLevelType)
{
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc  = NULL;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc   = NULL;

    OMX_BOOL bProfileSupport = OMX_FALSE;
    OMX_BOOL bLevelSupport   = OMX_FALSE;

    int nLevelCnt = 0;
    int i;

    FunctionIn();

    if ((pExynosComponent == NULL) ||
        (pProfileLevelType == NULL)) {
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    if (pVideoEnc == NULL)
        goto EXIT;

    pHevcEnc = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pHevcEnc == NULL)
        goto EXIT;

    while ((pHevcEnc->hMFCHevcHandle.maxLevel >> nLevelCnt++) > 0);

    if ((pHevcEnc->hMFCHevcHandle.nProfileCnt == 0) ||
        (nLevelCnt == 0)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : there is no any profile/level", __FUNCTION__);
        goto EXIT;
    }

    for (i = 0; i < pHevcEnc->hMFCHevcHandle.nProfileCnt; i++) {
        if (pHevcEnc->hMFCHevcHandle.profiles[i] == pProfileLevelType->eProfile) {
            bProfileSupport = OMX_TRUE;
            break;
        }
    }

    if (bProfileSupport != OMX_TRUE)
        goto EXIT;

    while (nLevelCnt >= 0) {
        if ((int)pProfileLevelType->eLevel == (0x1 << nLevelCnt)) {
            bLevelSupport = OMX_TRUE;
            break;
        }

        nLevelCnt--;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s : profile(%x)/level(%x) is %ssupported", __FUNCTION__,
                                            pProfileLevelType->eProfile, pProfileLevelType->eLevel,
                                            (bProfileSupport && bLevelSupport)? "":"not ");

EXIT:
    return (bProfileSupport && bLevelSupport);
}

static OMX_U32 OMXHevcProfileToProfileIDC(OMX_VIDEO_HEVCPROFILETYPE eProfile)
{
    OMX_U32 ret = 0;

    switch (eProfile) {
    case OMX_VIDEO_HEVCProfileMain:
        ret = 0;
        break;
    default:
        ret = 0;
        break;
    }

    return ret;
}

static OMX_U32 OMXHevcLevelToTierIDC(OMX_VIDEO_HEVCLEVELTYPE eLevel)
{
    OMX_U32 ret = 0; //default Main tier

    switch (eLevel) {
    case OMX_VIDEO_HEVCMainTierLevel1:
    case OMX_VIDEO_HEVCMainTierLevel2:
    case OMX_VIDEO_HEVCMainTierLevel21:
    case OMX_VIDEO_HEVCMainTierLevel3:
    case OMX_VIDEO_HEVCMainTierLevel31:
    case OMX_VIDEO_HEVCMainTierLevel4:
    case OMX_VIDEO_HEVCMainTierLevel41:
    case OMX_VIDEO_HEVCMainTierLevel5:
        ret = 0;
        break;
    case OMX_VIDEO_HEVCHighTierLevel1:
    case OMX_VIDEO_HEVCHighTierLevel2:
    case OMX_VIDEO_HEVCHighTierLevel21:
    case OMX_VIDEO_HEVCHighTierLevel3:
    case OMX_VIDEO_HEVCHighTierLevel31:
    case OMX_VIDEO_HEVCHighTierLevel4:
    case OMX_VIDEO_HEVCHighTierLevel41:
    case OMX_VIDEO_HEVCHighTierLevel5:
        ret = 1;
        break;
    default:
        ret = 0;
        break;
    }

    return ret;
}

static OMX_U32 OMXHevcLevelToLevelIDC(OMX_VIDEO_HEVCLEVELTYPE eLevel)
{
    OMX_U32 ret = 40; //default OMX_VIDEO_HEVCLevel4

    switch (eLevel) {
    case OMX_VIDEO_HEVCMainTierLevel1:
    case OMX_VIDEO_HEVCHighTierLevel1:
        ret = 10;
        break;
    case OMX_VIDEO_HEVCMainTierLevel2:
    case OMX_VIDEO_HEVCHighTierLevel2:
        ret = 20;
        break;
    case OMX_VIDEO_HEVCMainTierLevel21:
    case OMX_VIDEO_HEVCHighTierLevel21:
        ret = 21;
        break;
    case OMX_VIDEO_HEVCMainTierLevel3:
    case OMX_VIDEO_HEVCHighTierLevel3:
        ret = 30;
        break;
    case OMX_VIDEO_HEVCMainTierLevel31:
    case OMX_VIDEO_HEVCHighTierLevel31:
        ret = 31;
        break;
    case OMX_VIDEO_HEVCMainTierLevel4:
    case OMX_VIDEO_HEVCHighTierLevel4:
        ret = 40;
        break;
    case OMX_VIDEO_HEVCMainTierLevel41:
    case OMX_VIDEO_HEVCHighTierLevel41:
        ret = 41;
        break;
    case OMX_VIDEO_HEVCMainTierLevel5:
    case OMX_VIDEO_HEVCHighTierLevel5:
        ret = 50;
        break;
    default:
        ret = 40;
        break;
    }

    return ret;
}

static void Print_HEVCEnc_Param(ExynosVideoEncParam *pEncParam)
{
    ExynosVideoEncCommonParam *pCommonParam = &pEncParam->commonParam;
    ExynosVideoEncHevcParam   *pHEVCParam   = &pEncParam->codecParam.hevc;

    /* common parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "SourceWidth             : %d", pCommonParam->SourceWidth);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "SourceHeight            : %d", pCommonParam->SourceHeight);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "IDRPeriod               : %d", pCommonParam->IDRPeriod);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "SliceMode               : %d", pCommonParam->SliceMode);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "RandomIntraMBRefresh    : %d", pCommonParam->RandomIntraMBRefresh);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "Bitrate                 : %d", pCommonParam->Bitrate);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "FrameQp                 : %d", pCommonParam->FrameQp);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "FrameQp_P               : %d", pCommonParam->FrameQp_P);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "QP(I) ranege            : %d / %d", pCommonParam->QpRange.QpMin_I, pCommonParam->QpRange.QpMax_I);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "QP(P) ranege            : %d / %d", pCommonParam->QpRange.QpMin_P, pCommonParam->QpRange.QpMax_P);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "QP(B) ranege            : %d / %d", pCommonParam->QpRange.QpMin_B, pCommonParam->QpRange.QpMax_B);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "PadControlOn            : %d", pCommonParam->PadControlOn);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LumaPadVal              : %d", pCommonParam->LumaPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "CbPadVal                : %d", pCommonParam->CbPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "CrPadVal                : %d", pCommonParam->CrPadVal);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "FrameMap                : %d", pCommonParam->FrameMap);

    /* HEVC specific parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "ProfileIDC              : %d", pHEVCParam->ProfileIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "TierIDC                 : %d", pHEVCParam->TierIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LevelIDC                : %d", pHEVCParam->LevelIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "FrameQp_B               : %d", pHEVCParam->FrameQp_B);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "FrameRate               : %d", pHEVCParam->FrameRate);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "MaxPartitionDepth       : %d", pHEVCParam->MaxPartitionDepth);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "NumberBFrames           : %d", pHEVCParam->NumberBFrames);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "NumberRefForPframes     : %d", pHEVCParam->NumberRefForPframes);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LoopFilterDisable       : %d", pHEVCParam->LoopFilterDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LoopFilterSliceFlag     : %d", pHEVCParam->LoopFilterSliceFlag);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LoopFilterTcOffset      : %d", pHEVCParam->LoopFilterTcOffset);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LoopFilterBetaOffset    : %d", pHEVCParam->LoopFilterBetaOffset);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LongtermRefEnable       : %d", pHEVCParam->LongtermRefEnable);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LongtermUserRef         : %d", pHEVCParam->LongtermUserRef);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LongtermStoreRef        : %d", pHEVCParam->LongtermStoreRef);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "DarkDisable             : %d", pHEVCParam->DarkDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "SmoothDisable           : %d", pHEVCParam->SmoothDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "StaticDisable           : %d", pHEVCParam->StaticDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "ActivityDisable         : %d", pHEVCParam->ActivityDisable);

    /* rate control related parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "EnableFRMRateControl    : %d", pCommonParam->EnableFRMRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "EnableMBRateControl     : %d", pCommonParam->EnableMBRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "CBRPeriodRf             : %d", pCommonParam->CBRPeriodRf);
}

static void Set_HEVCEnc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pInputPort       = NULL;
    EXYNOS_OMX_BASEPORT           *pOutputPort      = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = NULL;
    EXYNOS_HEVCENC_HANDLE         *pHevcEnc         = NULL;
    EXYNOS_MFC_HEVCENC_HANDLE     *pMFCHevcHandle   = NULL;
    OMX_COLOR_FORMATTYPE           eColorFormat     = OMX_COLOR_FormatUnused;

    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncHevcParam   *pHEVCParam   = NULL;

    int i;

    pVideoEnc           = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pHevcEnc            = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    pMFCHevcHandle      = &pHevcEnc->hMFCHevcHandle;
    pInputPort          = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pOutputPort         = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    pEncParam       = &pMFCHevcHandle->encParam;
    pCommonParam    = &pEncParam->commonParam;
    pHEVCParam      = &pEncParam->codecParam.hevc;

    pEncParam->eCompressionFormat = VIDEO_CODING_HEVC;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "eCompressionFormat: %d", pEncParam->eCompressionFormat);

    /* common parameters */
    if ((pVideoEnc->eRotationType == ROTATE_0) ||
        (pVideoEnc->eRotationType == ROTATE_180)) {
        pCommonParam->SourceWidth  = pOutputPort->portDefinition.format.video.nFrameWidth;
        pCommonParam->SourceHeight = pOutputPort->portDefinition.format.video.nFrameHeight;
    } else {
        pCommonParam->SourceWidth  = pOutputPort->portDefinition.format.video.nFrameHeight;
        pCommonParam->SourceHeight = pOutputPort->portDefinition.format.video.nFrameWidth;
    }
    pCommonParam->IDRPeriod    = pHevcEnc->nPFrames + 1;
    pCommonParam->SliceMode    = 0;
    pCommonParam->Bitrate      = pOutputPort->portDefinition.format.video.nBitrate;
    pCommonParam->FrameQp      = pVideoEnc->quantization.nQpI;
    pCommonParam->FrameQp_P    = pVideoEnc->quantization.nQpP;

    pCommonParam->QpRange.QpMin_I = pHevcEnc->qpRangeI.nMinQP;
    pCommonParam->QpRange.QpMax_I = pHevcEnc->qpRangeI.nMaxQP;
    pCommonParam->QpRange.QpMin_P = pHevcEnc->qpRangeP.nMinQP;
    pCommonParam->QpRange.QpMax_P = pHevcEnc->qpRangeP.nMaxQP;
    pCommonParam->QpRange.QpMin_B = pHevcEnc->qpRangeB.nMinQP;
    pCommonParam->QpRange.QpMax_B = pHevcEnc->qpRangeB.nMaxQP;

    pCommonParam->PadControlOn = 0;    /* 0: disable, 1: enable */
    pCommonParam->LumaPadVal   = 0;
    pCommonParam->CbPadVal     = 0;
    pCommonParam->CrPadVal     = 0;

    if (pVideoEnc->intraRefresh.eRefreshMode == OMX_VIDEO_IntraRefreshCyclic) {
        /* Cyclic Mode */
        pCommonParam->RandomIntraMBRefresh = pVideoEnc->intraRefresh.nCirMBs;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "RandomIntraMBRefresh: %d", pCommonParam->RandomIntraMBRefresh);
    } else {
        /* Don't support "Adaptive" and "Cyclic + Adaptive" */
        pCommonParam->RandomIntraMBRefresh = 0;
    }

    eColorFormat = Exynos_Input_GetActualColorFormat(pExynosComponent);
    pCommonParam->FrameMap = Exynos_OSAL_OMX2VideoFormat(eColorFormat, pInputPort->ePlaneType);

    /* HEVC specific parameters */
    pHEVCParam->ProfileIDC = OMXHevcProfileToProfileIDC(pHevcEnc->HevcComponent[OUTPUT_PORT_INDEX].eProfile);
    pHEVCParam->TierIDC    = OMXHevcLevelToTierIDC(pHevcEnc->HevcComponent[OUTPUT_PORT_INDEX].eLevel);
    pHEVCParam->LevelIDC   = OMXHevcLevelToLevelIDC(pHevcEnc->HevcComponent[OUTPUT_PORT_INDEX].eLevel);

    pHEVCParam->FrameQp_B  = pVideoEnc->quantization.nQpB;
    pHEVCParam->FrameRate  = (pInputPort->portDefinition.format.video.xFramerate) >> 16;

    /* there is no interface at OMX IL component */
    pHEVCParam->NumberBFrames         = 0;    /* 0 ~ 2 */
    pHEVCParam->NumberRefForPframes   = 1;    /* 1 ~ 2 */

    pHEVCParam->MaxPartitionDepth     = 1;
    pHEVCParam->LoopFilterDisable     = 0;    /* 1: Loop Filter Disable, 0: Filter Enable */
    pHEVCParam->LoopFilterSliceFlag   = 1;
    pHEVCParam->LoopFilterTcOffset    = 0;
    pHEVCParam->LoopFilterBetaOffset  = 0;

    pHEVCParam->LongtermRefEnable       = 0;
    pHEVCParam->LongtermUserRef         = 0;
    pHEVCParam->LongtermStoreRef        = 0;

    pHEVCParam->DarkDisable     = 1;    /* disable adaptive rate control on dark region */
    pHEVCParam->SmoothDisable   = 1;    /* disable adaptive rate control on smooth region */
    pHEVCParam->StaticDisable   = 1;    /* disable adaptive rate control on static region */
    pHEVCParam->ActivityDisable = 1;    /* disable adaptive rate control on high activity region */

    /* Temporal SVC */
    pHEVCParam->TemporalSVC.nTemporalLayerCount = (unsigned int)pHevcEnc->TemporalSVC.nTemporalLayerCount;
    for (i = 0; i < OMX_VIDEO_ANDROID_MAXHEVCTEMPORALLAYERS; i++)
        pHEVCParam->TemporalSVC.nTemporalLayerBitrateRatio[i] = (unsigned int)pHevcEnc->TemporalSVC.nTemporalLayerBitrateRatio[i];

    if (pMFCHevcHandle->bRoiInfo == OMX_TRUE)
        pHEVCParam->ROIEnable = 1;
    else
        pHEVCParam->ROIEnable = 0;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]: 0x%x", pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]);
    /* rate control related parameters */
    switch (pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]) {
    case OMX_Video_ControlRateDisable:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode DBR");
        pCommonParam->EnableFRMRateControl = 0;    /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 0;    /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 100;
        break;
    case OMX_Video_ControlRateConstant:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode CBR");
        pCommonParam->EnableFRMRateControl = 1;    /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1;    /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 9;
        break;
    case OMX_Video_ControlRateVariable:
    default: /*Android default */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pCommonParam->EnableFRMRateControl = 1;    /* 0: Disable, 1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1;    /* 0: Disable, 1:MB level RC */
        pCommonParam->CBRPeriodRf          = 100;
        break;
    }

//    Print_HEVCEnc_Param(pEncParam);
}

static void Change_HEVCEnc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_HEVCENC_HANDLE         *pHevcEnc          = NULL;
    EXYNOS_MFC_HEVCENC_HANDLE     *pMFCHevcHandle    = NULL;
    OMX_PTR                        pDynamicConfigCMD = NULL;
    OMX_PTR                        pConfigData       = NULL;
    OMX_S32                        nCmdIndex         = 0;
    ExynosVideoEncOps             *pEncOps           = NULL;
    int                            nValue            = 0;

    pVideoEnc       = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pHevcEnc        = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    pMFCHevcHandle  = &pHevcEnc->hMFCHevcHandle;
    pEncOps         = pMFCHevcHandle->pEncOps;

    pDynamicConfigCMD = (OMX_PTR)Exynos_OSAL_Dequeue(&pExynosComponent->dynamicConfigQ);
    if (pDynamicConfigCMD == NULL)
        goto EXIT;

    nCmdIndex   = *(OMX_S32 *)pDynamicConfigCMD;
    pConfigData = (OMX_PTR)((OMX_U8 *)pDynamicConfigCMD + sizeof(OMX_S32));

    switch ((int)nCmdIndex) {
    case OMX_IndexConfigVideoIntraVOPRefresh:
    {
        nValue = VIDEO_FRAME_I;
        pEncOps->Set_FrameType(pMFCHevcHandle->hMFCHandle, nValue);
        pVideoEnc->IntraRefreshVOP = OMX_FALSE;
    }
        break;
    case OMX_IndexConfigVideoIntraPeriod:
    {
        OMX_S32 nPFrames = (*((OMX_U32 *)pConfigData)) - 1;
        nValue = nPFrames + 1;
        pEncOps->Set_IDRPeriod(pMFCHevcHandle->hMFCHandle, nValue);
    }
        break;
    case OMX_IndexConfigVideoBitrate:
    {
        OMX_VIDEO_CONFIG_BITRATETYPE *pConfigBitrate = (OMX_VIDEO_CONFIG_BITRATETYPE *)pConfigData;
        if (pVideoEnc->eControlRate[OUTPUT_PORT_INDEX] != OMX_Video_ControlRateDisable) {
            nValue = pConfigBitrate->nEncodeBitrate;
            pEncOps->Set_BitRate(pMFCHevcHandle->hMFCHandle, nValue);
        }
    }
        break;
    case OMX_IndexConfigVideoFramerate:
    {
        OMX_CONFIG_FRAMERATETYPE *pConfigFramerate = (OMX_CONFIG_FRAMERATETYPE *)pConfigData;
        OMX_U32                   nPortIndex       = pConfigFramerate->nPortIndex;
        if (nPortIndex == INPUT_PORT_INDEX) {
            nValue = (pConfigFramerate->xEncodeFramerate) >> 16;
            pEncOps->Set_FrameRate(pMFCHevcHandle->hMFCHandle, nValue);
        }
    }
        break;
    case OMX_IndexConfigVideoQPRange:
    {
        OMX_VIDEO_QPRANGETYPE *pQpRange = (OMX_VIDEO_QPRANGETYPE *)pConfigData;
        ExynosVideoQPRange qpRange;

        qpRange.QpMin_I = pQpRange->qpRangeI.nMinQP;
        qpRange.QpMax_I = pQpRange->qpRangeI.nMaxQP;
        qpRange.QpMin_P = pQpRange->qpRangeP.nMinQP;
        qpRange.QpMax_P = pQpRange->qpRangeP.nMaxQP;
        qpRange.QpMin_B = pQpRange->qpRangeB.nMinQP;
        qpRange.QpMax_B = pQpRange->qpRangeB.nMaxQP;

        pEncOps->Set_QpRange(pMFCHevcHandle->hMFCHandle, qpRange);
    }
        break;
    case OMX_IndexConfigOperatingRate:
    {
        OMX_PARAM_U32TYPE *pConfigRate = (OMX_PARAM_U32TYPE *)pConfigData;
        OMX_U32            xFramerate  = pExynosComponent->pExynosPort[INPUT_PORT_INDEX].portDefinition.format.video.xFramerate;

        if (xFramerate == 0)
            nValue = 100;
        else
            nValue = (OMX_U32)((pConfigRate->nU32 / (double)xFramerate) * 100);

        pEncOps->Set_QosRatio(pMFCHevcHandle->hMFCHandle, nValue);
        pVideoEnc->bQosChanged = OMX_FALSE;
    }
        break;
#ifdef USE_QOS_CTRL
    case OMX_IndexVendorSetQosRatio:  /* MSRND */
    {
        EXYNOS_OMX_VIDEO_CONFIG_QOSINFO *pQosInfo = (EXYNOS_OMX_VIDEO_CONFIG_QOSINFO *)pConfigData;
        nValue = pQosInfo->nQosRatio;
        pEncOps->Set_QosRatio(pMFCHevcHandle->hMFCHandle, nValue);
        pVideoEnc->bQosChanged = OMX_FALSE;
    }
        break;
#endif
    case OMX_IndexConfigVideoTemporalSVC:
    {
        EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *pTemporalSVC = (EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *)pConfigData;
        ExynosVideoQPRange qpRange;
        int i;

        qpRange.QpMin_I = pTemporalSVC->nMinQuantizer;
        qpRange.QpMax_I = pTemporalSVC->nMaxQuantizer;
        qpRange.QpMin_P = pTemporalSVC->nMinQuantizer;
        qpRange.QpMax_P = pTemporalSVC->nMaxQuantizer;
        qpRange.QpMin_B = pTemporalSVC->nMinQuantizer;
        qpRange.QpMax_B = pTemporalSVC->nMaxQuantizer;

        pEncOps->Set_QpRange(pMFCHevcHandle->hMFCHandle, qpRange);

        pEncOps->Set_IDRPeriod(pMFCHevcHandle->hMFCHandle, pTemporalSVC->nKeyFrameInterval);

        /* Temporal SVC */
        TemporalLayerShareBuffer TemporalSVC;
        Exynos_OSAL_Memset(&TemporalSVC, 0, sizeof(TemporalLayerShareBuffer));
        TemporalSVC.nTemporalLayerCount = (unsigned int)pTemporalSVC->nTemporalLayerCount;
        for (i = 0; i < OMX_VIDEO_ANDROID_MAXHEVCTEMPORALLAYERS; i++)
            TemporalSVC.nTemporalLayerBitrateRatio[i] = (unsigned int)pTemporalSVC->nTemporalLayerBitrateRatio[i];

        if (pEncOps->Set_LayerChange(pMFCHevcHandle->hMFCHandle, TemporalSVC) != VIDEO_ERROR_NONE)
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "%s: Not supported control: Set_LayerChange", __FUNCTION__);
    }
        break;
     case OMX_IndexConfigVideoRoiInfo:
     {
        EXYNOS_OMX_VIDEO_CONFIG_ROIINFO *pRoiInfo = (EXYNOS_OMX_VIDEO_CONFIG_ROIINFO *)pConfigData;

        /* ROI INFO */
        RoiInfoShareBuffer RoiInfo;
        Exynos_OSAL_Memset(&RoiInfo, 0, sizeof(RoiInfo));
        RoiInfo.pRoiMBInfo     = (OMX_U64)(unsigned long)(((OMX_U8 *)pConfigData) + sizeof(EXYNOS_OMX_VIDEO_CONFIG_ROIINFO));
        RoiInfo.nRoiMBInfoSize = pRoiInfo->nRoiMBInfoSize;
        RoiInfo.nUpperQpOffset = pRoiInfo->nUpperQpOffset;
        RoiInfo.nLowerQpOffset = pRoiInfo->nLowerQpOffset;
        RoiInfo.bUseRoiInfo    = pRoiInfo->bUseRoiInfo;
        if (pEncOps->Set_RoiInfo != NULL)
            pEncOps->Set_RoiInfo(pMFCHevcHandle->hMFCHandle, &RoiInfo);
        else
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "%s: Not supported control: Set_RoiInfo", __func__);
    }
        break;
    default:
        break;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] nCmdIndex %x nValue %d ", pExynosComponent, __FUNCTION__, (int)nCmdIndex, nValue);
    Exynos_OSAL_Free(pDynamicConfigCMD);

    Set_HEVCEnc_Param(pExynosComponent);

EXIT:
    return;
}

#if 0  /* unused code */
OMX_ERRORTYPE GetCodecInputPrivateData(
    OMX_PTR pCodecBuffer,
    OMX_PTR pVirtAddr[],
    OMX_U32 nSize[])
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;

EXIT:
    return ret;
}
#endif

OMX_ERRORTYPE GetCodecOutputPrivateData(
    OMX_PTR  pCodecBuffer,
    OMX_PTR *pVirtAddr,
    OMX_U32 *pDataSize)
{
    OMX_ERRORTYPE      ret          = OMX_ErrorNone;
    ExynosVideoBuffer *pVideoBuffer = NULL;

    if (pCodecBuffer == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoBuffer = (ExynosVideoBuffer *)pCodecBuffer;

    if (pVirtAddr != NULL)
        *pVirtAddr = pVideoBuffer->planes[0].addr;

    if (pDataSize != NULL)
        *pDataSize = pVideoBuffer->planes[0].allocSize;

EXIT:
    return ret;
}

OMX_BOOL CheckFormatHWSupport(
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent,
    OMX_COLOR_FORMATTYPE         eColorFormat)
{
    OMX_BOOL                         ret            = OMX_FALSE;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc      = NULL;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc       = NULL;
    EXYNOS_OMX_BASEPORT             *pInputPort     = NULL;
    ExynosVideoColorFormatType       eVideoFormat   = VIDEO_CODING_UNKNOWN;
    int i;

    FunctionIn();

    if (pExynosComponent == NULL)
        goto EXIT;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    if (pVideoEnc == NULL)
        goto EXIT;

    pHevcEnc = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pHevcEnc == NULL)
        goto EXIT;
    pInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    eVideoFormat = (ExynosVideoColorFormatType)Exynos_OSAL_OMX2VideoFormat(eColorFormat, pInputPort->ePlaneType);

    for (i = 0; i < VIDEO_COLORFORMAT_MAX; i++) {
        if (pHevcEnc->hMFCHevcHandle.videoInstInfo.supportFormat[i] == VIDEO_COLORFORMAT_UNKNOWN)
            break;

        if (pHevcEnc->hMFCHevcHandle.videoInstInfo.supportFormat[i] == eVideoFormat) {
            ret = OMX_TRUE;
            break;
        }
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE HEVCCodecOpen(EXYNOS_HEVCENC_HANDLE *pHevcEnc, ExynosVideoInstInfo *pVideoInstInfo)
{
    OMX_ERRORTYPE            ret        = OMX_ErrorNone;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pHevcEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }

    /* alloc ops structure */
    pEncOps = (ExynosVideoEncOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoEncOps));
    pInbufOps = (ExynosVideoEncBufferOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoEncBufferOps));
    pOutbufOps = (ExynosVideoEncBufferOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoEncBufferOps));

    if ((pEncOps == NULL) ||
        (pInbufOps == NULL) ||
        (pOutbufOps == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate encoder ops buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pHevcEnc->hMFCHevcHandle.pEncOps    = pEncOps;
    pHevcEnc->hMFCHevcHandle.pInbufOps  = pInbufOps;
    pHevcEnc->hMFCHevcHandle.pOutbufOps = pOutbufOps;

    /* function pointer mapping */
    pEncOps->nSize     = sizeof(ExynosVideoEncOps);
    pInbufOps->nSize   = sizeof(ExynosVideoEncBufferOps);
    pOutbufOps->nSize  = sizeof(ExynosVideoEncBufferOps);

    Exynos_Video_Register_Encoder(pEncOps, pInbufOps, pOutbufOps);

    /* check mandatory functions for encoder ops */
    if ((pEncOps->Init == NULL) ||
        (pEncOps->Finalize == NULL) ||
        (pEncOps->Set_FrameTag == NULL) ||
        (pEncOps->Get_FrameTag == NULL)) {
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
    pHevcEnc->hMFCHevcHandle.hMFCHandle = pHevcEnc->hMFCHevcHandle.pEncOps->Init(pVideoInstInfo);
    if (pHevcEnc->hMFCHevcHandle.hMFCHandle == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate context buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    if (ret != OMX_ErrorNone) {
        if (pEncOps != NULL) {
            Exynos_OSAL_Free(pEncOps);
            pHevcEnc->hMFCHevcHandle.pEncOps = NULL;
        }

        if (pInbufOps != NULL) {
            Exynos_OSAL_Free(pInbufOps);
            pHevcEnc->hMFCHevcHandle.pInbufOps = NULL;
        }

        if (pOutbufOps != NULL) {
            Exynos_OSAL_Free(pOutbufOps);
            pHevcEnc->hMFCHevcHandle.pOutbufOps = NULL;
        }
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE HEVCCodecClose(EXYNOS_HEVCENC_HANDLE *pHevcEnc)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pHevcEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pHevcEnc->hMFCHevcHandle.hMFCHandle;
    pEncOps    = pHevcEnc->hMFCHevcHandle.pEncOps;
    pInbufOps  = pHevcEnc->hMFCHevcHandle.pInbufOps;
    pOutbufOps = pHevcEnc->hMFCHevcHandle.pOutbufOps;

    if (hMFCHandle != NULL) {
        pEncOps->Finalize(hMFCHandle);
        hMFCHandle = pHevcEnc->hMFCHevcHandle.hMFCHandle = NULL;
    }

    /* Unregister function pointers */
    Exynos_Video_Unregister_Encoder(pEncOps, pInbufOps, pOutbufOps);

    if (pOutbufOps != NULL) {
        Exynos_OSAL_Free(pOutbufOps);
        pOutbufOps = pHevcEnc->hMFCHevcHandle.pOutbufOps = NULL;
    }

    if (pInbufOps != NULL) {
        Exynos_OSAL_Free(pInbufOps);
        pInbufOps = pHevcEnc->hMFCHevcHandle.pInbufOps = NULL;
    }

    if (pEncOps != NULL) {
        Exynos_OSAL_Free(pEncOps);
        pEncOps = pHevcEnc->hMFCHevcHandle.pEncOps = NULL;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE HEVCCodecStart(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex)
{
    OMX_ERRORTYPE                    ret        = OMX_ErrorNone;
    void                            *hMFCHandle = NULL;
    ExynosVideoEncBufferOps         *pInbufOps  = NULL;
    ExynosVideoEncBufferOps         *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc  = NULL;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc   = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)((EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate)->hComponentHandle;
    if (pVideoEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pHevcEnc = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pHevcEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pHevcEnc->hMFCHevcHandle.hMFCHandle;
    pInbufOps  = pHevcEnc->hMFCHevcHandle.pInbufOps;
    pOutbufOps = pHevcEnc->hMFCHevcHandle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX)
        pInbufOps->Run(hMFCHandle);
    else if (nPortIndex == OUTPUT_PORT_INDEX)
        pOutbufOps->Run(hMFCHandle);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE HEVCCodecStop(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex)
{
    OMX_ERRORTYPE                    ret        = OMX_ErrorNone;
    void                            *hMFCHandle = NULL;
    ExynosVideoEncBufferOps         *pInbufOps  = NULL;
    ExynosVideoEncBufferOps         *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc  = NULL;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc   = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)((EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate)->hComponentHandle;
    if (pVideoEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pHevcEnc = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pHevcEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pHevcEnc->hMFCHevcHandle.hMFCHandle;
    pInbufOps  = pHevcEnc->hMFCHevcHandle.pInbufOps;
    pOutbufOps = pHevcEnc->hMFCHevcHandle.pOutbufOps;

    if ((nPortIndex == INPUT_PORT_INDEX) && (pInbufOps != NULL))
        pInbufOps->Stop(hMFCHandle);
    else if ((nPortIndex == OUTPUT_PORT_INDEX) && (pOutbufOps != NULL))
        pOutbufOps->Stop(hMFCHandle);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE HEVCCodecOutputBufferProcessRun(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex)
{
    OMX_ERRORTYPE                    ret        = OMX_ErrorNone;
    void                            *hMFCHandle = NULL;
    ExynosVideoEncBufferOps         *pInbufOps  = NULL;
    ExynosVideoEncBufferOps         *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc  = NULL;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc   = NULL;

    FunctionIn();

    if (pOMXComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)((EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate)->hComponentHandle;
    if (pVideoEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pHevcEnc = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pHevcEnc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pHevcEnc->hMFCHevcHandle.hMFCHandle;
    pInbufOps  = pHevcEnc->hMFCHevcHandle.pInbufOps;
    pOutbufOps = pHevcEnc->hMFCHevcHandle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX) {
        if (pHevcEnc->bSourceStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pHevcEnc->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    if (nPortIndex == OUTPUT_PORT_INDEX) {
        if (pHevcEnc->bDestinationStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pHevcEnc->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE HEVCCodecRegistCodecBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex,
    int                  nBufferCnt)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc           = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    void                            *hMFCHandle         = pHevcEnc->hMFCHevcHandle.hMFCHandle;
    CODEC_ENC_BUFFER               **ppCodecBuffer      = NULL;
    ExynosVideoEncBufferOps         *pBufOps            = NULL;
    ExynosVideoPlane                *pPlanes            = NULL;

    int nPlaneCnt = 0;
    int i, j;

    FunctionIn();

    if (nPortIndex == INPUT_PORT_INDEX) {
        ppCodecBuffer   = &(pVideoEnc->pMFCEncInputBuffer[0]);
        pBufOps         = pHevcEnc->hMFCHevcHandle.pInbufOps;
    } else {
        ppCodecBuffer   = &(pVideoEnc->pMFCEncOutputBuffer[0]);
        pBufOps         = pHevcEnc->hMFCHevcHandle.pOutbufOps;
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

OMX_ERRORTYPE HEVCCodecEnqueueAllBuffer(
    OMX_COMPONENTTYPE *pOMXComponent,
    OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc           = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    void                            *hMFCHandle         = pHevcEnc->hMFCHevcHandle.hMFCHandle;
    EXYNOS_OMX_BASEPORT             *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT             *pOutputPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    int i, nOutbufs;

    ExynosVideoEncOps       *pEncOps    = pHevcEnc->hMFCHevcHandle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pHevcEnc->hMFCHevcHandle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pHevcEnc->hMFCHevcHandle.pOutbufOps;

    FunctionIn();

    if ((nPortIndex != INPUT_PORT_INDEX) &&
        (nPortIndex != OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((nPortIndex == INPUT_PORT_INDEX) &&
        (pHevcEnc->bSourceStart == OMX_TRUE)) {
        Exynos_CodecBufferReset(pExynosComponent, INPUT_PORT_INDEX);

        for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++)  {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]->pVirAddr[0]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]->pVirAddr[0]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]->pVirAddr[1]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]->pVirAddr[1]);

            Exynos_CodecBufferEnqueue(pExynosComponent, INPUT_PORT_INDEX, pVideoEnc->pMFCEncInputBuffer[i]);
        }

        pInbufOps->Clear_Queue(hMFCHandle);
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) &&
               (pHevcEnc->bDestinationStart == OMX_TRUE)) {
        Exynos_CodecBufferReset(pExynosComponent, OUTPUT_PORT_INDEX);

        for (i = 0; i < MFC_OUTPUT_BUFFER_NUM_MAX; i++) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncOutputBuffer[%d]: 0x%x", i, pVideoEnc->pMFCEncOutputBuffer[i]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncOutputBuffer[%d]->pVirAddr[0]: 0x%x", i, pVideoEnc->pMFCEncOutputBuffer[i]->pVirAddr[0]);

            Exynos_CodecBufferEnqueue(pExynosComponent, OUTPUT_PORT_INDEX, pVideoEnc->pMFCEncOutputBuffer[i]);
        }

        pOutbufOps->Clear_Queue(hMFCHandle);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE HEVCCodecSrcSetup(
    OMX_COMPONENTTYPE   *pOMXComponent,
    EXYNOS_OMX_DATA     *pSrcInputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc           = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_MFC_HEVCENC_HANDLE       *pMFCHevcHandle     = &pHevcEnc->hMFCHevcHandle;
    void                            *hMFCHandle         = pMFCHevcHandle->hMFCHandle;
    EXYNOS_OMX_BASEPORT             *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT             *pOutputPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoEncOps       *pEncOps    = pHevcEnc->hMFCHevcHandle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pHevcEnc->hMFCHevcHandle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pHevcEnc->hMFCHevcHandle.pOutbufOps;
    ExynosVideoEncParam     *pEncParam    = NULL;

    ExynosVideoGeometry bufferConf;
    OMX_U32             nInputBufferCnt = 0;
    int i, nOutbufs;

    FunctionIn();

    if ((pSrcInputData->dataLen <= 0) && (pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] FBD with EOS will be processed through bypassBufferInfoQ",
                                                pExynosComponent, __FUNCTION__);

        BYPASS_BUFFER_INFO *pBufferInfo = (BYPASS_BUFFER_INFO *)Exynos_OSAL_Malloc(sizeof(BYPASS_BUFFER_INFO));
        if (pBufferInfo == NULL) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }

        pBufferInfo->nFlags     = pSrcInputData->nFlags;
        pBufferInfo->timeStamp  = pSrcInputData->timeStamp;
        ret = Exynos_OSAL_Queue(&pHevcEnc->bypassBufferInfoQ, (void *)pBufferInfo);
        Exynos_OSAL_SignalSet(pHevcEnc->hDestinationStartEvent);  /* awake dstInput thread */

        ret = OMX_ErrorNone;
        goto EXIT;
    }

    Set_HEVCEnc_Param(pExynosComponent);

    pEncParam = &pMFCHevcHandle->encParam;
    if (pEncOps->Set_EncParam) {
        if(pEncOps->Set_EncParam(pHevcEnc->hMFCHevcHandle.hMFCHandle, pEncParam) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }
    Print_HEVCEnc_Param(pEncParam);

    if ((pMFCHevcHandle->bPrependSpsPpsToIdr == OMX_TRUE) &&
        (pEncOps->Enable_PrependSpsPpsToIdr)) {
        if (pEncOps->Enable_PrependSpsPpsToIdr(pHevcEnc->hMFCHevcHandle.hMFCHandle) != VIDEO_ERROR_NONE)
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "%s: Not supported control: Enable_PrependSpsPpsToIdr", __func__);
    }

    /* input buffer info: only 3 config values needed */
    Exynos_OSAL_Memset(&bufferConf, 0, sizeof(bufferConf));
    bufferConf.eColorFormat = pEncParam->commonParam.FrameMap;
    if ((pVideoEnc->eRotationType == ROTATE_0) ||
        (pVideoEnc->eRotationType == ROTATE_180)) {
        bufferConf.nFrameWidth = pInputPort->portDefinition.format.video.nFrameWidth;
        bufferConf.nFrameHeight = pInputPort->portDefinition.format.video.nFrameHeight;
        bufferConf.nStride = ALIGN(pInputPort->portDefinition.format.video.nFrameWidth, 16);
    } else {
        bufferConf.nFrameWidth = pInputPort->portDefinition.format.video.nFrameHeight;
        bufferConf.nFrameHeight = pInputPort->portDefinition.format.video.nFrameWidth;
        bufferConf.nStride = ALIGN(pInputPort->portDefinition.format.video.nFrameHeight, 16);
    }
    bufferConf.nPlaneCnt    = Exynos_GetPlaneFromPort(pInputPort);
    pInbufOps->Set_Shareable(hMFCHandle);
    nInputBufferCnt = MAX_INPUTBUFFER_NUM_DYNAMIC;


    if (pInputPort->bufferProcessType & BUFFER_COPY) {
        /* should be done before prepare input buffer */
        if (pInbufOps->Enable_Cacheable(hMFCHandle) != VIDEO_ERROR_NONE) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* set input buffer geometry */
    if (pInbufOps->Set_Geometry) {
        if (pInbufOps->Set_Geometry(hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* setup input buffer */
    if (pInbufOps->Setup(hMFCHandle, nInputBufferCnt) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup input buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if ((pInputPort->bufferProcessType & BUFFER_SHARE)
#ifdef USE_METADATABUFFERTYPE
        && (pInputPort->bStoreMetaData != OMX_TRUE)
#endif
        ) {
        ret = OMX_ErrorNotImplemented;
        goto EXIT;
    }

    pHevcEnc->hMFCHevcHandle.bConfiguredMFCSrc = OMX_TRUE;
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE HEVCCodecDstSetup(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc           = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_MFC_HEVCENC_HANDLE       *pMFCHevcHandle     = &pHevcEnc->hMFCHevcHandle;
    void                            *hMFCHandle         = pMFCHevcHandle->hMFCHandle;
    EXYNOS_OMX_BASEPORT             *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT             *pOutputPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoEncOps       *pEncOps    = pHevcEnc->hMFCHevcHandle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pHevcEnc->hMFCHevcHandle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pHevcEnc->hMFCHevcHandle.pOutbufOps;
    ExynosVideoGeometry      bufferConf;

    unsigned int nAllocLen[VIDEO_BUFFER_MAX_PLANES] = {0, 0, 0};
    unsigned int nDataLen[VIDEO_BUFFER_MAX_PLANES]  = {0, 0, 0};
    int i, nOutBufSize = 0, nOutputBufferCnt = 0;

    FunctionIn();

    nOutBufSize = pOutputPort->portDefinition.nBufferSize;
    if (pOutputPort->bStoreMetaData == OMX_TRUE) {
        nOutBufSize = ALIGN(pOutputPort->portDefinition.format.video.nFrameWidth *
                            pOutputPort->portDefinition.format.video.nFrameHeight * 3 / 2, 512);
    }

    /* set geometry for output (dst) */
    if (pOutbufOps->Set_Geometry) {
        /* only 2 config values needed */
        bufferConf.eCompressionFormat   = VIDEO_CODING_HEVC;
        bufferConf.nSizeImage           = nOutBufSize;
        bufferConf.nPlaneCnt            = Exynos_GetPlaneFromPort(pOutputPort);

        if (pOutbufOps->Set_Geometry(pHevcEnc->hMFCHevcHandle.hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }

    /* should be done before prepare output buffer */
    if (pOutbufOps->Enable_Cacheable(hMFCHandle) != VIDEO_ERROR_NONE) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pOutbufOps->Set_Shareable(hMFCHandle);

    if (pOutputPort->bufferProcessType & BUFFER_COPY)
        nOutputBufferCnt = MFC_OUTPUT_BUFFER_NUM_MAX;
    else
        nOutputBufferCnt = pOutputPort->portDefinition.nBufferCountActual;

    if (pOutbufOps->Setup(pHevcEnc->hMFCHevcHandle.hMFCHandle, nOutputBufferCnt) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (pOutputPort->bufferProcessType & BUFFER_COPY) {
        nAllocLen[0] = nOutBufSize;
        ret = Exynos_Allocate_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX, MFC_OUTPUT_BUFFER_NUM_MAX, nAllocLen);
        if (ret != OMX_ErrorNone)
            goto EXIT;

        /* Enqueue output buffer */
        for (i = 0; i < MFC_OUTPUT_BUFFER_NUM_MAX; i++) {
            pOutbufOps->ExtensionEnqueue(hMFCHandle,
                                (void **)pVideoEnc->pMFCEncOutputBuffer[i]->pVirAddr,
                                (int *)pVideoEnc->pMFCEncOutputBuffer[i]->fd,
                                pVideoEnc->pMFCEncOutputBuffer[i]->bufferSize,
                                nDataLen,
                                Exynos_GetPlaneFromPort(pOutputPort),
                                NULL);
        }

        if (pOutbufOps->Run(hMFCHandle) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    } else if (pOutputPort->bufferProcessType & BUFFER_SHARE) {
        /* Register output buffer */
        /*************/
        /*    TBD    */
        /*************/
    }

    pHevcEnc->hMFCHevcHandle.bConfiguredMFCDst = OMX_TRUE;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     pComponentParameterStructure)
{
    OMX_ERRORTYPE                    ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc        = NULL;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc         = NULL;

    FunctionIn();

    if ((hComponent == NULL) ||
        (pComponentParameterStructure == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;

    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone)
        goto EXIT;

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pHevcEnc = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] nParamIndex %x", pExynosComponent, __FUNCTION__, (int)nParamIndex);
    switch ((int)nParamIndex) {
    case OMX_IndexParamVideoHevc:
    {
        OMX_VIDEO_PARAM_HEVCTYPE *pDstHevcComponent = (OMX_VIDEO_PARAM_HEVCTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_HEVCTYPE *pSrcHevcComponent = NULL;
        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pDstHevcComponent, sizeof(OMX_VIDEO_PARAM_HEVCTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pDstHevcComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSrcHevcComponent = &pHevcEnc->HevcComponent[pDstHevcComponent->nPortIndex];

        Exynos_OSAL_Memcpy(((char *)pDstHevcComponent) + nOffset,
                           ((char *)pSrcHevcComponent) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_HEVCTYPE) - nOffset);
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_HEVC_ENC_ROLE);
    }
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        ret = GetIndexToProfileLevel(pExynosComponent, pDstProfileLevel);
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel  = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        OMX_VIDEO_PARAM_HEVCTYPE         *pSrcHevcComponent = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSrcHevcComponent = &pHevcEnc->HevcComponent[pDstProfileLevel->nPortIndex];

        pDstProfileLevel->eProfile = pSrcHevcComponent->eProfile;
        pDstProfileLevel->eLevel   = pSrcHevcComponent->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pDstErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSrcErrorCorrectionType = &pHevcEnc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC              = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync           = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing   = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC             = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    case OMX_IndexParamVideoQPRange:
    {
        OMX_VIDEO_QPRANGETYPE *pQpRange   = (OMX_VIDEO_QPRANGETYPE *)pComponentParameterStructure;
        OMX_U32                nPortIndex = pQpRange->nPortIndex;

        ret = Exynos_OMX_Check_SizeVersion(pQpRange, sizeof(OMX_VIDEO_QPRANGETYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pQpRange->qpRangeI.nMinQP = pHevcEnc->qpRangeI.nMinQP;
        pQpRange->qpRangeI.nMaxQP = pHevcEnc->qpRangeI.nMaxQP;
        pQpRange->qpRangeP.nMinQP = pHevcEnc->qpRangeP.nMinQP;
        pQpRange->qpRangeP.nMaxQP = pHevcEnc->qpRangeP.nMaxQP;
        pQpRange->qpRangeB.nMinQP = pHevcEnc->qpRangeB.nMinQP;
        pQpRange->qpRangeB.nMaxQP = pHevcEnc->qpRangeB.nMaxQP;
    }
        break;
    case OMX_IndexParamVideoHevcEnableTemporalSVC:
    {
        EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC *pDstEnableTemporalSVC = (EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC *)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pDstEnableTemporalSVC, sizeof(EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pDstEnableTemporalSVC->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pDstEnableTemporalSVC->bEnableTemporalSVC = pHevcEnc->hMFCHevcHandle.bTemporalSVC;
    }
        break;
    case OMX_IndexParamVideoEnableRoiInfo:
    {
        EXYNOS_OMX_VIDEO_PARAM_ENABLE_ROIINFO *pDstEnableRoiInfo = (EXYNOS_OMX_VIDEO_PARAM_ENABLE_ROIINFO *)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pDstEnableRoiInfo, sizeof(EXYNOS_OMX_VIDEO_PARAM_ENABLE_ROIINFO));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstEnableRoiInfo->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pDstEnableRoiInfo->bEnableRoiInfo = pHevcEnc->hMFCHevcHandle.bRoiInfo;
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE                    ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc        = NULL;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc         = NULL;

    FunctionIn();

    if ((hComponent == NULL) ||
        (pComponentParameterStructure == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;

    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone)
        goto EXIT;

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pHevcEnc = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] nIndex %x", pExynosComponent, __FUNCTION__, (int)nIndex);
    switch ((int)nIndex) {
    case OMX_IndexParamVideoHevc:
    {
        OMX_VIDEO_PARAM_HEVCTYPE *pDstHevcComponent = NULL;
        OMX_VIDEO_PARAM_HEVCTYPE *pSrcHevcComponent = (OMX_VIDEO_PARAM_HEVCTYPE *)pComponentParameterStructure;
        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pSrcHevcComponent, sizeof(OMX_VIDEO_PARAM_HEVCTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcHevcComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pDstHevcComponent = &pHevcEnc->HevcComponent[pSrcHevcComponent->nPortIndex];

        Exynos_OSAL_Memcpy(((char *)pDstHevcComponent) + nOffset,
                           ((char *)pSrcHevcComponent) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_HEVCTYPE) - nOffset);
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if ((pExynosComponent->currentState != OMX_StateLoaded) &&
            (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_HEVC_ENC_ROLE)) {
            pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingHEVC;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel  = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_HEVCTYPE         *pDstHevcComponent = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pDstHevcComponent = &pHevcEnc->HevcComponent[pSrcProfileLevel->nPortIndex];

        if (OMX_FALSE == CheckProfileLevelSupport(pExynosComponent, pSrcProfileLevel)) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        pDstHevcComponent->eProfile = pSrcProfileLevel->eProfile;
        pDstHevcComponent->eLevel   = pSrcProfileLevel->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pDstErrorCorrectionType = &pHevcEnc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC              = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync           = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing   = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC             = pSrcErrorCorrectionType->bEnableRVLC;
    }
        break;
    case OMX_IndexParamVideoQPRange:
    {
        OMX_VIDEO_QPRANGETYPE *pQpRange   = (OMX_VIDEO_QPRANGETYPE *)pComponentParameterStructure;
        OMX_U32                nPortIndex = pQpRange->nPortIndex;

        ret = Exynos_OMX_Check_SizeVersion(pQpRange, sizeof(OMX_VIDEO_QPRANGETYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if ((pQpRange->qpRangeI.nMinQP > pQpRange->qpRangeI.nMaxQP) ||
            (pQpRange->qpRangeP.nMinQP > pQpRange->qpRangeP.nMaxQP) ||
            (pQpRange->qpRangeB.nMinQP > pQpRange->qpRangeB.nMaxQP)) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: QP value is invalid(I[min:%d, max:%d], P[min:%d, max:%d], B[min:%d, max:%d])", __FUNCTION__,
                            pQpRange->qpRangeI.nMinQP, pQpRange->qpRangeI.nMaxQP,
                            pQpRange->qpRangeP.nMinQP, pQpRange->qpRangeP.nMaxQP,
                            pQpRange->qpRangeB.nMinQP, pQpRange->qpRangeB.nMaxQP);
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        pHevcEnc->qpRangeI.nMinQP = pQpRange->qpRangeI.nMinQP;
        pHevcEnc->qpRangeI.nMaxQP = pQpRange->qpRangeI.nMaxQP;
        pHevcEnc->qpRangeP.nMinQP = pQpRange->qpRangeP.nMinQP;
        pHevcEnc->qpRangeP.nMaxQP = pQpRange->qpRangeP.nMaxQP;
        pHevcEnc->qpRangeB.nMinQP = pQpRange->qpRangeB.nMinQP;
        pHevcEnc->qpRangeB.nMaxQP = pQpRange->qpRangeB.nMaxQP;
    }
        break;
    case OMX_IndexParamPrependSPSPPSToIDR:
    {
#ifdef USE_ANDROID
        ret = Exynos_OSAL_SetPrependSPSPPSToIDR(pComponentParameterStructure, &(pHevcEnc->hMFCHevcHandle.bPrependSpsPpsToIdr));
#else
        ret = OMX_ErrorNotImplemented;
#endif
    }
        break;
    case OMX_IndexParamVideoHevcEnableTemporalSVC:
    {
        EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC   *pSrcEnableTemporalSVC  = (EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC *)pComponentParameterStructure;
        OMX_PARAM_PORTDEFINITIONTYPE                *pPortDef               = &(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition);
        int i;

        ret = Exynos_OMX_Check_SizeVersion(pSrcEnableTemporalSVC, sizeof(EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcEnableTemporalSVC->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if ((pHevcEnc->hMFCHevcHandle.videoInstInfo.specificInfo.enc.bTemporalSvcSupport == VIDEO_FALSE) &&
            (pSrcEnableTemporalSVC->bEnableTemporalSVC == OMX_TRUE)) {
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "%s: MFC D/D doesn't support Temporal SVC", __func__);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        pHevcEnc->hMFCHevcHandle.bTemporalSVC = pSrcEnableTemporalSVC->bEnableTemporalSVC;
        if ((pHevcEnc->hMFCHevcHandle.bTemporalSVC == OMX_TRUE) &&
            (pHevcEnc->TemporalSVC.nTemporalLayerCount == 0)) {  /* not initialized yet */
            pHevcEnc->TemporalSVC.nTemporalLayerCount           = 1;
            pHevcEnc->TemporalSVC.nTemporalLayerBitrateRatio[0] = pPortDef->format.video.nBitrate;
        } else if (pHevcEnc->hMFCHevcHandle.bTemporalSVC == OMX_FALSE) {  /* set default value */
            pHevcEnc->TemporalSVC.nTemporalLayerCount = 0;
            for (i = 0; i < OMX_VIDEO_ANDROID_MAXHEVCTEMPORALLAYERS; i++)
                pHevcEnc->TemporalSVC.nTemporalLayerBitrateRatio[i] = pPortDef->format.video.nBitrate;
        }
    }
        break;
    case OMX_IndexParamVideoEnableRoiInfo:
    {
        EXYNOS_OMX_VIDEO_PARAM_ENABLE_ROIINFO   *pSrcEnableRoiInfo  = (EXYNOS_OMX_VIDEO_PARAM_ENABLE_ROIINFO *)pComponentParameterStructure;
        OMX_PARAM_PORTDEFINITIONTYPE            *pPortDef           = &(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition);
        int i;

        ret = Exynos_OMX_Check_SizeVersion(pSrcEnableRoiInfo, sizeof(EXYNOS_OMX_VIDEO_PARAM_ENABLE_ROIINFO));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcEnableRoiInfo->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if ((pHevcEnc->hMFCHevcHandle.videoInstInfo.specificInfo.enc.bRoiInfoSupport == VIDEO_FALSE) &&
            (pSrcEnableRoiInfo->bEnableRoiInfo == OMX_TRUE)) {
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "%s: MFC D/D doesn't support Roi Info", __func__);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }
        pHevcEnc->hMFCHevcHandle.bRoiInfo = pSrcEnableRoiInfo->bEnableRoiInfo;
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE                    ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc        = NULL;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc         = NULL;

    FunctionIn();

    if ((hComponent == NULL) ||
        (pComponentConfigStructure == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;

    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone)
        goto EXIT;

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pHevcEnc = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] nIndex %x", pExynosComponent, __FUNCTION__, (int)nIndex);
    switch ((int)nIndex) {
    case OMX_IndexConfigVideoQPRange:
    {
        OMX_VIDEO_QPRANGETYPE *pQpRange   = (OMX_VIDEO_QPRANGETYPE *)pComponentConfigStructure;
        OMX_U32                nPortIndex = pQpRange->nPortIndex;

        ret = Exynos_OMX_Check_SizeVersion(pQpRange, sizeof(OMX_VIDEO_QPRANGETYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pQpRange->qpRangeI.nMinQP = pHevcEnc->qpRangeI.nMinQP;
        pQpRange->qpRangeI.nMaxQP = pHevcEnc->qpRangeI.nMaxQP;
        pQpRange->qpRangeP.nMinQP = pHevcEnc->qpRangeP.nMinQP;
        pQpRange->qpRangeP.nMaxQP = pHevcEnc->qpRangeP.nMaxQP;
        pQpRange->qpRangeB.nMinQP = pHevcEnc->qpRangeB.nMinQP;
        pQpRange->qpRangeB.nMaxQP = pHevcEnc->qpRangeB.nMaxQP;
    }
        break;
    case OMX_IndexConfigVideoTemporalSVC:
    {
        EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *pDstTemporalSVC = (EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *)pComponentConfigStructure;
        EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *pSrcTemporalSVC = &pHevcEnc->TemporalSVC;

        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pDstTemporalSVC, sizeof(EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pDstTemporalSVC->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSrcTemporalSVC->nKeyFrameInterval = pHevcEnc->nPFrames + 1;

        pSrcTemporalSVC->nMinQuantizer = pHevcEnc->qpRangeI.nMinQP;
        pSrcTemporalSVC->nMaxQuantizer = pHevcEnc->qpRangeI.nMaxQP;

        Exynos_OSAL_Memcpy(((char *)pDstTemporalSVC) + nOffset,
                           ((char *)pSrcTemporalSVC) + nOffset,
                           sizeof(EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC) - nOffset);
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = NULL;
    EXYNOS_HEVCENC_HANDLE         *pHevcEnc         = NULL;

    FunctionIn();

    if ((hComponent == NULL) ||
        (pComponentConfigStructure == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;

    ret = Exynos_OMX_Check_SizeVersion(pOMXComponent, sizeof(OMX_COMPONENTTYPE));
    if (ret != OMX_ErrorNone)
        goto EXIT;

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pHevcEnc = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] nIndex %x", pExynosComponent, __FUNCTION__, (int)nIndex);
    switch ((int)nIndex) {
    case OMX_IndexConfigVideoIntraPeriod:
    {
        OMX_U32 nPFrames = (*((OMX_U32 *)pComponentConfigStructure)) - 1;

        pHevcEnc->nPFrames = nPFrames;

        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexConfigVideoQPRange:
    {
        OMX_VIDEO_QPRANGETYPE *pQpRange   = (OMX_VIDEO_QPRANGETYPE *)pComponentConfigStructure;
        OMX_U32                nPortIndex = pQpRange->nPortIndex;

        ret = Exynos_OMX_Check_SizeVersion(pQpRange, sizeof(OMX_VIDEO_QPRANGETYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if ((pQpRange->qpRangeI.nMinQP > pQpRange->qpRangeI.nMaxQP) ||
            (pQpRange->qpRangeP.nMinQP > pQpRange->qpRangeP.nMaxQP) ||
            (pQpRange->qpRangeB.nMinQP > pQpRange->qpRangeB.nMaxQP)) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: QP value is invalid(I[min:%d, max:%d], P[min:%d, max:%d], B[min:%d, max:%d])", __FUNCTION__,
                            pQpRange->qpRangeI.nMinQP, pQpRange->qpRangeI.nMaxQP,
                            pQpRange->qpRangeP.nMinQP, pQpRange->qpRangeP.nMaxQP,
                            pQpRange->qpRangeB.nMinQP, pQpRange->qpRangeB.nMaxQP);
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        pHevcEnc->qpRangeI.nMinQP = pQpRange->qpRangeI.nMinQP;
        pHevcEnc->qpRangeI.nMaxQP = pQpRange->qpRangeI.nMaxQP;
        pHevcEnc->qpRangeP.nMinQP = pQpRange->qpRangeP.nMinQP;
        pHevcEnc->qpRangeP.nMaxQP = pQpRange->qpRangeP.nMaxQP;
        pHevcEnc->qpRangeB.nMinQP = pQpRange->qpRangeB.nMinQP;
        pHevcEnc->qpRangeB.nMaxQP = pQpRange->qpRangeB.nMaxQP;
    }
        break;
    case OMX_IndexConfigVideoTemporalSVC:
    {
        EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *pDstTemporalSVC = &pHevcEnc->TemporalSVC;
        EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *pSrcTemporalSVC = (EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *)pComponentConfigStructure;

        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pSrcTemporalSVC, sizeof(EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcTemporalSVC->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if ((pHevcEnc->hMFCHevcHandle.bTemporalSVC == OMX_FALSE) ||
            (pSrcTemporalSVC->nTemporalLayerCount > OMX_VIDEO_ANDROID_MAXHEVCTEMPORALLAYERS)) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        pHevcEnc->nPFrames = pSrcTemporalSVC->nKeyFrameInterval - 1;

        pHevcEnc->qpRangeI.nMinQP = pSrcTemporalSVC->nMinQuantizer;
        pHevcEnc->qpRangeI.nMaxQP = pSrcTemporalSVC->nMaxQuantizer;

        Exynos_OSAL_Memcpy(((char *)pDstTemporalSVC) + nOffset,
                           ((char *)pSrcTemporalSVC) + nOffset,
                           sizeof(EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC) - nOffset);
    }
        break;
    case OMX_IndexConfigVideoRoiInfo:
    {
        EXYNOS_OMX_VIDEO_CONFIG_ROIINFO *pRoiInfo = (EXYNOS_OMX_VIDEO_CONFIG_ROIINFO *)pComponentConfigStructure;

        if (pRoiInfo->nPortIndex != INPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }
        if ((pHevcEnc->hMFCHevcHandle.bRoiInfo == OMX_FALSE) ||
            ((pRoiInfo->bUseRoiInfo == OMX_TRUE) &&
            ((pRoiInfo->nRoiMBInfoSize <= 0) ||
            (pRoiInfo->pRoiMBInfo == NULL)))) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: bUseRoiInfo %d nRoiMBInfoSize %d pRoiMBInfo %p", __FUNCTION__, __LINE__,
                                                    pRoiInfo->bUseRoiInfo, pRoiInfo->nRoiMBInfoSize, pRoiInfo->pRoiMBInfo);
                ret = OMX_ErrorBadParameter;
                goto EXIT;
        }
    }
        break;
    default:
        ret = Exynos_OMX_VideoEncodeSetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    if (ret == OMX_ErrorNone) {
        OMX_PTR pDynamicConfigCMD = NULL;
        pDynamicConfigCMD = Exynos_OMX_MakeDynamicConfigCMD(nIndex, pComponentConfigStructure);
        Exynos_OSAL_Queue(&pExynosComponent->dynamicConfigQ, (void *)pDynamicConfigCMD);
    }

    if (ret == (OMX_ERRORTYPE)OMX_ErrorNoneExpiration)
        ret = OMX_ErrorNone;

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
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
    if (ret != OMX_ErrorNone)
        goto EXIT;

    if (pOMXComponent->pComponentPrivate == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;

    if ((cParameterName == NULL) ||
        (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_PARAM_PREPEND_SPSPPS_TO_IDR) == 0) {
        *pIndexType = OMX_IndexParamPrependSPSPPSToIDR;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_CONFIG_VIDEO_TEMPORALSVC) == 0) {
        *pIndexType = OMX_IndexConfigVideoTemporalSVC;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_PARAM_VIDEO_HEVC_ENABLE_TEMPORALSVC) == 0) {
        *pIndexType = OMX_IndexParamVideoHevcEnableTemporalSVC;
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_CONFIG_VIDEO_ROIINFO) == 0) {
        *pIndexType = OMX_IndexConfigVideoRoiInfo;
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_PARAM_VIDEO_ENABLE_ROIINFO) == 0) {
        *pIndexType = OMX_IndexParamVideoEnableRoiInfo;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    ret = Exynos_OMX_VideoEncodeGetExtensionIndex(hComponent, cParameterName, pIndexType);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_ComponentRoleEnum(
    OMX_HANDLETYPE   hComponent,
    OMX_U8          *cRole,
    OMX_U32          nIndex)
{
    OMX_ERRORTYPE               ret              = OMX_ErrorNone;

    FunctionIn();

    if ((hComponent == NULL) ||
        (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    if (nIndex == (MAX_COMPONENT_ROLE_NUM-1)) {
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_HEVC_ENC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT             *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT             *pOutputPort        = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc           = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    OMX_PTR                          hMFCHandle         = NULL;
    OMX_COLOR_FORMATTYPE             eColorFormat       = OMX_COLOR_FormatUnused;

    ExynosVideoEncOps       *pEncOps        = NULL;
    ExynosVideoEncBufferOps *pInbufOps      = NULL;
    ExynosVideoEncBufferOps *pOutbufOps     = NULL;
    ExynosVideoInstInfo     *pVideoInstInfo = &(pHevcEnc->hMFCHevcHandle.videoInstInfo);

    CSC_METHOD csc_method = CSC_METHOD_SW;

    int i = 0, nPlaneCnt;

    FunctionIn();

    pHevcEnc->hMFCHevcHandle.bConfiguredMFCSrc = OMX_FALSE;
    pHevcEnc->hMFCHevcHandle.bConfiguredMFCDst = OMX_FALSE;
    pVideoEnc->bFirstInput         = OMX_TRUE;
    pVideoEnc->bFirstOutput        = OMX_FALSE;
    pExynosComponent->bUseFlagEOF  = OMX_TRUE;
    pExynosComponent->bSaveFlagEOS = OMX_FALSE;
    pExynosComponent->bBehaviorEOS = OMX_FALSE;

    eColorFormat = pInputPort->portDefinition.format.video.eColorFormat;
#ifdef USE_METADATABUFFERTYPE
    if (pInputPort->bStoreMetaData == OMX_TRUE) {
#ifdef USE_ANDROIDOPAQUE
        if (eColorFormat == (OMX_COLOR_FORMATTYPE)OMX_COLOR_FormatAndroidOpaque)
            pInputPort->bufferProcessType = BUFFER_COPY;
        else
            pInputPort->bufferProcessType = BUFFER_SHARE;
#else
        pInputPort->bufferProcessType = BUFFER_COPY;
#endif
    } else {
        pInputPort->bufferProcessType = BUFFER_COPY;
    }
#else
    pInputPort->bufferProcessType = BUFFER_COPY;
#endif

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] CodecOpen W: %d H:%d  Bitrate:%d FPS:%d", pExynosComponent, __FUNCTION__,
                                                                                              pInputPort->portDefinition.format.video.nFrameWidth,
                                                                                              pInputPort->portDefinition.format.video.nFrameHeight,
                                                                                              pInputPort->portDefinition.format.video.nBitrate,
                                                                                              pInputPort->portDefinition.format.video.xFramerate);
    pVideoInstInfo->nSize        = sizeof(ExynosVideoInstInfo);
    pVideoInstInfo->nWidth       = pInputPort->portDefinition.format.video.nFrameWidth;
    pVideoInstInfo->nHeight      = pInputPort->portDefinition.format.video.nFrameHeight;
    pVideoInstInfo->nBitrate     = pInputPort->portDefinition.format.video.nBitrate;
    pVideoInstInfo->xFramerate   = pInputPort->portDefinition.format.video.xFramerate;

    /* HEVC Codec Open */
    ret = HEVCCodecOpen(pHevcEnc, pVideoInstInfo);
    if (ret != OMX_ErrorNone)
        goto EXIT;

    pEncOps    = pHevcEnc->hMFCHevcHandle.pEncOps;
    pInbufOps  = pHevcEnc->hMFCHevcHandle.pInbufOps;
    pOutbufOps = pHevcEnc->hMFCHevcHandle.pOutbufOps;
    hMFCHandle = pHevcEnc->hMFCHevcHandle.hMFCHandle;

    Exynos_SetPlaneToPort(pInputPort, MFC_DEFAULT_INPUT_BUFFER_PLANE);
    Exynos_SetPlaneToPort(pOutputPort, MFC_DEFAULT_OUTPUT_BUFFER_PLANE);

    Exynos_OSAL_SemaphoreCreate(&pInputPort->codecSemID);
    Exynos_OSAL_QueueCreate(&pInputPort->codecBufferQ, MAX_QUEUE_ELEMENTS);

    if (pOutputPort->bufferProcessType & BUFFER_COPY) {
        Exynos_OSAL_SemaphoreCreate(&pOutputPort->codecSemID);
        Exynos_OSAL_QueueCreate(&pOutputPort->codecBufferQ, MAX_QUEUE_ELEMENTS);
    } else if (pOutputPort->bufferProcessType & BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    pHevcEnc->bSourceStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pHevcEnc->hSourceStartEvent);
    pHevcEnc->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pHevcEnc->hDestinationStartEvent);

    Exynos_OSAL_Memset(pExynosComponent->bTimestampSlotUsed, 0, sizeof(OMX_BOOL) * MAX_TIMESTAMP);
    INIT_ARRAY_TO_VAL(pExynosComponent->timeStamp, DEFAULT_TIMESTAMP_VAL, MAX_TIMESTAMP);
    Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pHevcEnc->hMFCHevcHandle.indexTimestamp = 0;
    pHevcEnc->hMFCHevcHandle.outputIndexTimestamp = 0;

    pExynosComponent->getAllDelayBuffer = OMX_FALSE;

    Exynos_OSAL_QueueCreate(&pHevcEnc->bypassBufferInfoQ, QUEUE_ELEMENTS);

#ifdef USE_CSC_HW
    csc_method = CSC_METHOD_HW;
#endif
    if (pExynosComponent->codecType == HW_VIDEO_ENC_SECURE_CODEC) {
        pVideoEnc->csc_handle = csc_init(CSC_METHOD_HW);
        csc_set_hw_property(pVideoEnc->csc_handle, CSC_HW_PROPERTY_FIXED_NODE, 2);
        csc_set_hw_property(pVideoEnc->csc_handle, CSC_HW_PROPERTY_MODE_DRM, 1);
    } else {
        pVideoEnc->csc_handle = csc_init(csc_method);
    }
    if (pVideoEnc->csc_handle == NULL) {
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pVideoEnc->csc_set_format = OMX_FALSE;

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Terminate */
OMX_ERRORTYPE Exynos_HEVCEnc_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
    EXYNOS_OMX_BASEPORT           *pInputPort       = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pOutputPort      = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_HEVCENC_HANDLE         *pHevcEnc         = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;

    int i = 0, plane = 0;

    FunctionIn();

    if (pVideoEnc->csc_handle != NULL) {
        csc_deinit(pVideoEnc->csc_handle);
        pVideoEnc->csc_handle = NULL;
    }

    Exynos_OSAL_QueueTerminate(&pHevcEnc->bypassBufferInfoQ);

    Exynos_OSAL_SignalTerminate(pHevcEnc->hDestinationStartEvent);
    pHevcEnc->hDestinationStartEvent = NULL;
    pHevcEnc->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalTerminate(pHevcEnc->hSourceStartEvent);
    pHevcEnc->hSourceStartEvent = NULL;
    pHevcEnc->bSourceStart = OMX_FALSE;

    if (pOutputPort->bufferProcessType & BUFFER_COPY) {
        Exynos_Free_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX);
        Exynos_OSAL_QueueTerminate(&pOutputPort->codecBufferQ);
        Exynos_OSAL_SemaphoreTerminate(pOutputPort->codecSemID);
    } else if (pOutputPort->bufferProcessType & BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    if (pInputPort->bufferProcessType & BUFFER_COPY) {
        Exynos_Free_CodecBuffers(pOMXComponent, INPUT_PORT_INDEX);
    } else if (pInputPort->bufferProcessType & BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    Exynos_OSAL_QueueTerminate(&pInputPort->codecBufferQ);
    Exynos_OSAL_SemaphoreTerminate(pInputPort->codecSemID);

    HEVCCodecClose(pHevcEnc);

    Exynos_ResetAllPortConfig(pOMXComponent);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_SrcIn(
    OMX_COMPONENTTYPE   *pOMXComponent,
    EXYNOS_OMX_DATA     *pSrcInputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_HEVCENC_HANDLE         *pHevcEnc          = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    void                          *hMFCHandle        = pHevcEnc->hMFCHevcHandle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pInputPort        = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    OMX_COLOR_FORMATTYPE           inputColorFormat  = OMX_COLOR_FormatUnused;

    ExynosVideoEncOps       *pEncOps     = pHevcEnc->hMFCHevcHandle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps   = pHevcEnc->hMFCHevcHandle.pInbufOps;
    ExynosVideoErrorType     codecReturn = VIDEO_ERROR_NONE;

    OMX_BUFFERHEADERTYPE tempBufferHeader;
    void *pPrivate = NULL;

    unsigned int nAllocLen[MAX_BUFFER_PLANE]  = {0, 0, 0};
    unsigned int nDataLen[MAX_BUFFER_PLANE]   = {0, 0, 0};
    int i, nPlaneCnt;

    FunctionIn();

    if (pHevcEnc->hMFCHevcHandle.bConfiguredMFCSrc == OMX_FALSE) {
        ret = HEVCCodecSrcSetup(pOMXComponent, pSrcInputData);
        if ((ret != OMX_ErrorNone) ||
            ((pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
            goto EXIT;
        }
    }

    if (pHevcEnc->hMFCHevcHandle.bConfiguredMFCDst == OMX_FALSE) {
        ret = HEVCCodecDstSetup(pOMXComponent);
        if (ret != OMX_ErrorNone)
            goto EXIT;
    }

    while (Exynos_OSAL_GetElemNum(&pExynosComponent->dynamicConfigQ) > 0) {
        Change_HEVCEnc_Param(pExynosComponent);
    }

    if ((pSrcInputData->dataLen > 0) ||
        ((pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
        pExynosComponent->timeStamp[pHevcEnc->hMFCHevcHandle.indexTimestamp]            = pSrcInputData->timeStamp;
        pExynosComponent->bTimestampSlotUsed[pHevcEnc->hMFCHevcHandle.indexTimestamp]   = OMX_TRUE;
        pExynosComponent->nFlags[pHevcEnc->hMFCHevcHandle.indexTimestamp]               = pSrcInputData->nFlags;
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] input timestamp %lld us (%.2f secs), Tag: %d, nFlags: 0x%x", pExynosComponent, __FUNCTION__,
                                                        pSrcInputData->timeStamp, pSrcInputData->timeStamp / 1E6, pHevcEnc->hMFCHevcHandle.indexTimestamp, pSrcInputData->nFlags);

        pEncOps->Set_FrameTag(hMFCHandle, pHevcEnc->hMFCHevcHandle.indexTimestamp);
        pHevcEnc->hMFCHevcHandle.indexTimestamp++;
        pHevcEnc->hMFCHevcHandle.indexTimestamp %= MAX_TIMESTAMP;

#ifdef PERFORMANCE_DEBUG
        Exynos_OSAL_V4L2CountIncrease(pInputPort->hBufferCount, pSrcInputData->bufferHeader, INPUT_PORT_INDEX);
#endif

        /* queue work for input buffer */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] oneFrameSize: %ld, bufferHeader: 0x%p", pExynosComponent, __FUNCTION__, pSrcInputData->dataLen, pSrcInputData->bufferHeader);

        inputColorFormat = Exynos_Input_GetActualColorFormat(pExynosComponent);
        if ((pVideoEnc->eRotationType == ROTATE_0) ||
            (pVideoEnc->eRotationType == ROTATE_180)) {
            Exynos_OSAL_GetPlaneSize(inputColorFormat,
                                     pInputPort->ePlaneType,
                                     pInputPort->portDefinition.format.video.nFrameWidth,
                                     pInputPort->portDefinition.format.video.nFrameHeight,
                                     nDataLen,
                                     nAllocLen);
        } else {
            Exynos_OSAL_GetPlaneSize(inputColorFormat,
                                     pInputPort->ePlaneType,
                                     pInputPort->portDefinition.format.video.nFrameHeight,
                                     pInputPort->portDefinition.format.video.nFrameWidth,
                                     nDataLen,
                                     nAllocLen);
        }

        if (pInputPort->bufferProcessType == BUFFER_COPY) {
            tempBufferHeader.nFlags     = pSrcInputData->nFlags;
            tempBufferHeader.nTimeStamp = pSrcInputData->timeStamp;
            pPrivate = (void *)&tempBufferHeader;
        } else {
            pPrivate = (void *)pSrcInputData->bufferHeader;
        }

        nPlaneCnt = Exynos_GetPlaneFromPort(pInputPort);
        if (pVideoEnc->nInbufSpareSize> 0) {
            for (i = 0; i < nPlaneCnt; i++)
                nAllocLen[i] = nAllocLen[i] + pVideoEnc->nInbufSpareSize;
        }

        if (pSrcInputData->dataLen == 0) {
            for (i = 0; i < nPlaneCnt; i++)
                nDataLen[i] = 0;
        }

        codecReturn = pInbufOps->ExtensionEnqueue(hMFCHandle,
                                    (void **)pSrcInputData->multiPlaneBuffer.dataBuffer,
                                    (int *)pSrcInputData->multiPlaneBuffer.fd,
                                    nAllocLen,
                                    nDataLen,
                                    nPlaneCnt,
                                    pPrivate);
        if (codecReturn != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - pInbufOps->Enqueue", __FUNCTION__, __LINE__);
            ret = (OMX_ERRORTYPE)OMX_ErrorCodecEncode;
            goto EXIT;
        }

        HEVCCodecStart(pOMXComponent, INPUT_PORT_INDEX);

        if (pHevcEnc->bSourceStart == OMX_FALSE) {
            pHevcEnc->bSourceStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pHevcEnc->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }

        if (pHevcEnc->bDestinationStart == OMX_FALSE) {
            pHevcEnc->bDestinationStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pHevcEnc->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_SrcOut(
    OMX_COMPONENTTYPE   *pOMXComponent,
    EXYNOS_OMX_DATA     *pSrcOutputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc           = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    void                            *hMFCHandle         = pHevcEnc->hMFCHevcHandle.hMFCHandle;
    EXYNOS_OMX_BASEPORT             *pInputPort         = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    ExynosVideoEncBufferOps *pInbufOps      = pHevcEnc->hMFCHevcHandle.pInbufOps;
    ExynosVideoBuffer       *pVideoBuffer   = NULL;
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
    pSrcOutputData->allocSize     = 0;
    pSrcOutputData->bufferHeader  = NULL;

    if (pVideoBuffer == NULL) {
        pSrcOutputData->multiPlaneBuffer.dataBuffer[0] = NULL;
        pSrcOutputData->pPrivate = NULL;
    } else {
        int plane = 0, nPlaneCnt;
        nPlaneCnt = Exynos_GetPlaneFromPort(pInputPort);
        for (plane = 0; plane < nPlaneCnt; plane++) {
            pSrcOutputData->multiPlaneBuffer.dataBuffer[plane] = pVideoBuffer->planes[plane].addr;
            pSrcOutputData->multiPlaneBuffer.fd[plane] = pVideoBuffer->planes[plane].fd;

            pSrcOutputData->allocSize += pVideoBuffer->planes[plane].allocSize;
        }

        if (pInputPort->bufferProcessType & BUFFER_COPY) {
            int i;
            for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++) {
                if (pSrcOutputData->multiPlaneBuffer.dataBuffer[0] ==
                        pVideoEnc->pMFCEncInputBuffer[i]->pVirAddr[0]) {
                    pVideoEnc->pMFCEncInputBuffer[i]->dataSize = 0;
                    pSrcOutputData->pPrivate = pVideoEnc->pMFCEncInputBuffer[i];
                    break;
                }
            }

            if (i >= MFC_INPUT_BUFFER_NUM_MAX) {
                Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - Lost buffer", __FUNCTION__, __LINE__);
                ret = (OMX_ERRORTYPE)OMX_ErrorCodecEncode;
                goto EXIT;
            }
        }

        /* For Share Buffer */
        if (pInputPort->bufferProcessType == BUFFER_SHARE)
            pSrcOutputData->bufferHeader = (OMX_BUFFERHEADERTYPE*)pVideoBuffer->pPrivate;

#ifdef PERFORMANCE_DEBUG
        Exynos_OSAL_V4L2CountDecrease(pInputPort->hBufferCount, pSrcOutputData->bufferHeader, INPUT_PORT_INDEX);
#endif
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_DstIn(
    OMX_COMPONENTTYPE   *pOMXComponent,
    EXYNOS_OMX_DATA     *pDstInputData)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_HEVCENC_HANDLE         *pHevcEnc         = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    void                          *hMFCHandle       = pHevcEnc->hMFCHevcHandle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pOutputPort      = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoEncOps       *pEncOps     = pHevcEnc->hMFCHevcHandle.pEncOps;
    ExynosVideoEncBufferOps *pOutbufOps  = pHevcEnc->hMFCHevcHandle.pOutbufOps;
    ExynosVideoErrorType     codecReturn = VIDEO_ERROR_NONE;

    unsigned int nAllocLen[VIDEO_BUFFER_MAX_PLANES] = {0, 0, 0};
    unsigned int nDataLen[VIDEO_BUFFER_MAX_PLANES]  = {0, 0, 0};

    FunctionIn();

    if (pDstInputData->multiPlaneBuffer.dataBuffer[0] == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to find input buffer");
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

#ifdef PERFORMANCE_DEBUG
    Exynos_OSAL_V4L2CountIncrease(pOutputPort->hBufferCount, pDstInputData->bufferHeader, OUTPUT_PORT_INDEX);
#endif

    nAllocLen[0] = pOutputPort->portDefinition.nBufferSize;
    if (pOutputPort->bStoreMetaData == OMX_TRUE)
        nAllocLen[0] = ALIGN(pOutputPort->portDefinition.format.video.nFrameWidth * pOutputPort->portDefinition.format.video.nFrameHeight * 3 / 2, 512);

    codecReturn = pOutbufOps->ExtensionEnqueue(hMFCHandle,
                                (void **)pDstInputData->multiPlaneBuffer.dataBuffer,
                                (int *)pDstInputData->multiPlaneBuffer.fd,
                                nAllocLen,
                                nDataLen,
                                Exynos_GetPlaneFromPort(pOutputPort),
                                pDstInputData->bufferHeader);
    if (codecReturn != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - pOutbufOps->Enqueue", __FUNCTION__, __LINE__);
        ret = (OMX_ERRORTYPE)OMX_ErrorCodecEncode;
        goto EXIT;
    }

    HEVCCodecStart(pOMXComponent, OUTPUT_PORT_INDEX);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_DstOut(
    OMX_COMPONENTTYPE   *pOMXComponent,
    EXYNOS_OMX_DATA     *pDstOutputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_HEVCENC_HANDLE         *pHevcEnc          = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_OMX_BASEPORT           *pOutputPort       = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    void                          *hMFCHandle        = pHevcEnc->hMFCHevcHandle.hMFCHandle;

    ExynosVideoEncOps          *pEncOps        = pHevcEnc->hMFCHevcHandle.pEncOps;
    ExynosVideoEncBufferOps    *pOutbufOps     = pHevcEnc->hMFCHevcHandle.pOutbufOps;
    ExynosVideoBuffer          *pVideoBuffer   = NULL;
    ExynosVideoBuffer           videoBuffer;
    ExynosVideoFrameStatusType  displayStatus  = VIDEO_FRAME_STATUS_UNKNOWN;
    ExynosVideoErrorType        codecReturn    = VIDEO_ERROR_NONE;

    OMX_S32 indexTimestamp = 0;

    FunctionIn();

    if (pHevcEnc->bDestinationStart == OMX_FALSE) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

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

    pHevcEnc->hMFCHevcHandle.outputIndexTimestamp++;
    pHevcEnc->hMFCHevcHandle.outputIndexTimestamp %= MAX_TIMESTAMP;

    pDstOutputData->multiPlaneBuffer.dataBuffer[0] = pVideoBuffer->planes[0].addr;
    pDstOutputData->multiPlaneBuffer.fd[0] = pVideoBuffer->planes[0].fd;
    pDstOutputData->allocSize   = pVideoBuffer->planes[0].allocSize;
    pDstOutputData->dataLen     = pVideoBuffer->planes[0].dataSize;
    pDstOutputData->remainDataLen = pVideoBuffer->planes[0].dataSize;
    pDstOutputData->usedDataLen = 0;
    pDstOutputData->pPrivate = pVideoBuffer;

    if (pOutputPort->bufferProcessType & BUFFER_COPY) {
        int i = 0;
        pDstOutputData->pPrivate = NULL;

        for (i = 0; i < MFC_OUTPUT_BUFFER_NUM_MAX; i++) {
            if (pDstOutputData->multiPlaneBuffer.dataBuffer[0] ==
                pVideoEnc->pMFCEncOutputBuffer[i]->pVirAddr[0]) {
                pDstOutputData->pPrivate = pVideoEnc->pMFCEncOutputBuffer[i];
                break;
            }
        }

        if (pDstOutputData->pPrivate == NULL) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Can not find buffer");
            ret = (OMX_ERRORTYPE)OMX_ErrorCodecEncode;
            goto EXIT;
        }
    }

    /* For Share Buffer */
    pDstOutputData->bufferHeader = (OMX_BUFFERHEADERTYPE *)pVideoBuffer->pPrivate;

    if (pVideoEnc->bFirstOutput == OMX_FALSE) {
        pDstOutputData->timeStamp = 0;
        pDstOutputData->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
        pDstOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
        pVideoEnc->bFirstOutput = OMX_TRUE;
    } else {
        indexTimestamp = pEncOps->Get_FrameTag(pHevcEnc->hMFCHevcHandle.hMFCHandle);

        if ((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)) {
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "[%p][%s] Tag(%d) is invalid. changes to use outputIndexTimestamp(%d)",
                                                pExynosComponent, __FUNCTION__,
                                                indexTimestamp, pHevcEnc->hMFCHevcHandle.outputIndexTimestamp);
            indexTimestamp = pHevcEnc->hMFCHevcHandle.outputIndexTimestamp;
        }

        pDstOutputData->timeStamp                               = pExynosComponent->timeStamp[indexTimestamp];
        pExynosComponent->bTimestampSlotUsed[indexTimestamp]    = OMX_FALSE;
        pDstOutputData->nFlags                                  = pExynosComponent->nFlags[indexTimestamp];
        pDstOutputData->nFlags                                 |= OMX_BUFFERFLAG_ENDOFFRAME;
    }

    if (pVideoBuffer->frameType == VIDEO_FRAME_I)
        pDstOutputData->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] output timestamp %lld us (%.2f secs), Tag: %d, frameType: %d dataLen: %d",
                                          pExynosComponent, __FUNCTION__,
                                          pDstOutputData->timeStamp, pDstOutputData->timeStamp / 1E6,
                                          indexTimestamp, pVideoBuffer->frameType, pDstOutputData->dataLen);

#ifdef PERFORMANCE_DEBUG
    if (pDstOutputData->bufferHeader != NULL) {
        pDstOutputData->bufferHeader->nTimeStamp = pDstOutputData->timeStamp;
        Exynos_OSAL_V4L2CountDecrease(pOutputPort->hBufferCount, pDstOutputData->bufferHeader, OUTPUT_PORT_INDEX);
    }
#endif

    if ((displayStatus == VIDEO_FRAME_STATUS_CHANGE_RESOL) ||
        (((pDstOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) &&
         (pExynosComponent->bBehaviorEOS == OMX_FALSE))) {
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] displayStatus:%d, nFlags0x%x", pExynosComponent, __FUNCTION__, displayStatus, pDstOutputData->nFlags);
        pDstOutputData->remainDataLen = 0;
    }

    if (((pDstOutputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS) &&
        (pExynosComponent->bBehaviorEOS == OMX_TRUE))
        pExynosComponent->bBehaviorEOS = OMX_FALSE;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_srcInputBufferProcess(
    OMX_COMPONENTTYPE   *pOMXComponent,
    EXYNOS_OMX_DATA     *pSrcInputData)
{
    OMX_ERRORTYPE             ret              = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_BASEPORT      *pInputPort       = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pInputPort)) ||
        (!CHECK_PORT_POPULATED(pInputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    ret = Exynos_HEVCEnc_SrcIn(pOMXComponent, pSrcInputData);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - SrcIn -> event is thrown to client", __FUNCTION__, __LINE__);
        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_srcOutputBufferProcess(
    OMX_COMPONENTTYPE   *pOMXComponent,
    EXYNOS_OMX_DATA     *pSrcOutputData)
{
    OMX_ERRORTYPE                    ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc         = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc          = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_OMX_BASEPORT             *pInputPort        = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pInputPort)) ||
        (!CHECK_PORT_POPULATED(pInputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pInputPort->bufferProcessType & BUFFER_COPY) {
        if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX)) {
            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }

    if ((pHevcEnc->bSourceStart == OMX_FALSE) &&
       (!CHECK_PORT_BEING_FLUSHED(pInputPort))) {
        Exynos_OSAL_SignalWait(pHevcEnc->hSourceStartEvent, DEF_MAX_WAIT_TIME);
        if (pVideoEnc->bExitBufferProcessThread)
            goto EXIT;

        Exynos_OSAL_SignalReset(pHevcEnc->hSourceStartEvent);
    }

    ret = Exynos_HEVCEnc_SrcOut(pOMXComponent, pSrcOutputData);
    if ((ret != OMX_ErrorNone) &&
        (pExynosComponent->currentState == OMX_StateExecuting)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - SrcOut -> event is thrown to client", __FUNCTION__, __LINE__);
        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_dstInputBufferProcess(
    OMX_COMPONENTTYPE   *pOMXComponent,
    EXYNOS_OMX_DATA     *pDstInputData)
{
    OMX_ERRORTYPE                    ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc         = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc          = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_OMX_BASEPORT             *pOutputPort       = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pOutputPort)) ||
        (!CHECK_PORT_POPULATED(pOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pOutputPort->bufferProcessType & BUFFER_SHARE) {
        if ((pHevcEnc->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pOutputPort))) {
            Exynos_OSAL_SignalWait(pHevcEnc->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            if (pVideoEnc->bExitBufferProcessThread)
                goto EXIT;

            Exynos_OSAL_SignalReset(pHevcEnc->hDestinationStartEvent);
        }

        if (Exynos_OSAL_GetElemNum(&pHevcEnc->bypassBufferInfoQ) > 0) {
            Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] buffer with EOS will be returned by bypassBufferInfoQ",
                                                    pExynosComponent, __FUNCTION__);

            BYPASS_BUFFER_INFO *pBufferInfo = (BYPASS_BUFFER_INFO *)Exynos_OSAL_Dequeue(&pHevcEnc->bypassBufferInfoQ);
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

    if (pHevcEnc->hMFCHevcHandle.bConfiguredMFCDst == OMX_TRUE) {
        ret = Exynos_HEVCEnc_DstIn(pOMXComponent, pDstInputData);
        if (ret != OMX_ErrorNone) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - DstIn -> event is thrown to client", __FUNCTION__, __LINE__);
            pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
        }
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_HEVCEnc_dstOutputBufferProcess(
    OMX_COMPONENTTYPE   *pOMXComponent,
    EXYNOS_OMX_DATA     *pDstOutputData)
{
    OMX_ERRORTYPE                    ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc         = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc          = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_OMX_BASEPORT             *pOutputPort       = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pOutputPort)) ||
        (!CHECK_PORT_POPULATED(pOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pOutputPort->bufferProcessType & BUFFER_COPY) {
        if ((pHevcEnc->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pOutputPort))) {
            Exynos_OSAL_SignalWait(pHevcEnc->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            if (pVideoEnc->bExitBufferProcessThread)
                goto EXIT;

            Exynos_OSAL_SignalReset(pHevcEnc->hDestinationStartEvent);
        }

        if (Exynos_OSAL_GetElemNum(&pHevcEnc->bypassBufferInfoQ) > 0) {
            EXYNOS_OMX_DATABUFFER *dstOutputUseBuffer   = &pOutputPort->way.port2WayDataBuffer.outputDataBuffer;
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

            pBufferInfo = Exynos_OSAL_Dequeue(&pHevcEnc->bypassBufferInfoQ);
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

    ret = Exynos_HEVCEnc_DstOut(pOMXComponent, pDstOutputData);
    if ((ret != OMX_ErrorNone) &&
        (pExynosComponent->currentState == OMX_StateExecuting)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - DstOut -> event is thrown to client", __FUNCTION__, __LINE__);
        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
    }

EXIT:
    FunctionOut();

    return ret;
}

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(
    OMX_HANDLETYPE hComponent,
    OMX_STRING     componentName)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosPort      = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = NULL;
    EXYNOS_HEVCENC_HANDLE         *pHevcEnc         = NULL;
    OMX_BOOL                       bSecureMode      = OMX_FALSE;
    int i = 0;

    Exynos_OSAL_Get_Log_Property(); // For debuging
    FunctionIn();

    if ((hComponent == NULL) ||
        (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_HEVC_ENC, componentName) == 0) {
        bSecureMode = OMX_FALSE;
    } else if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_HEVC_DRM_ENC, componentName) == 0) {
        bSecureMode = OMX_TRUE;
    } else {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, componentName:%s, Line:%d", componentName, __LINE__);
        goto EXIT;
    }

    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    ret = Exynos_OMX_VideoEncodeComponentInit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_Error, Line:%d", __LINE__);
        goto EXIT;
    }

    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pExynosComponent->codecType = (bSecureMode == OMX_TRUE)? HW_VIDEO_ENC_SECURE_CODEC:HW_VIDEO_ENC_CODEC;

    pExynosComponent->componentName = (OMX_STRING)Exynos_OSAL_Malloc(MAX_OMX_COMPONENT_NAME_SIZE);
    if (pExynosComponent->componentName == NULL) {
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pExynosComponent->componentName, 0, MAX_OMX_COMPONENT_NAME_SIZE);

    pHevcEnc = Exynos_OSAL_Malloc(sizeof(EXYNOS_HEVCENC_HANDLE));
    if (pHevcEnc == NULL) {
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pHevcEnc, 0, sizeof(EXYNOS_HEVCENC_HANDLE));

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pVideoEnc->hCodecHandle = (OMX_HANDLETYPE)pHevcEnc;
    pHevcEnc->qpRangeI.nMinQP = 0;
    pHevcEnc->qpRangeI.nMaxQP = 50;
    pHevcEnc->qpRangeP.nMinQP = 0;
    pHevcEnc->qpRangeP.nMaxQP = 50;
    pHevcEnc->qpRangeB.nMinQP = 0;
    pHevcEnc->qpRangeB.nMaxQP = 50;

    pVideoEnc->quantization.nQpI = 29;
    pVideoEnc->quantization.nQpP = 30;
    pVideoEnc->quantization.nQpB = 32;

    if (pExynosComponent->codecType == HW_VIDEO_ENC_SECURE_CODEC)
        Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_HEVC_DRM_ENC);
    else
        Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_HEVC_ENC);

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
    pExynosPort->portDefinition.format.video.nStride = 0;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_INPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "raw/video");
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;
    pExynosPort->bufferProcessType = BUFFER_COPY;
    pExynosPort->portWayType = WAY2_PORT;
    pExynosPort->ePlaneType = PLANE_MULTIPLE;

#ifdef USE_SINGLE_PLANE_IN_DRM
    if (pExynosComponent->codecType == HW_VIDEO_ENC_SECURE_CODEC)
        pExynosPort->ePlaneType = PLANE_SINGLE;
#endif

    /* Output port */
    pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    pExynosPort->portDefinition.format.video.nFrameWidth = DEFAULT_FRAME_WIDTH;
    pExynosPort->portDefinition.format.video.nFrameHeight= DEFAULT_FRAME_HEIGHT;
    pExynosPort->portDefinition.format.video.nStride = 0;
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingHEVC;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/avc");
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;
    pExynosPort->bufferProcessType = BUFFER_SHARE;
    pExynosPort->portWayType = WAY2_PORT;
    pExynosPort->ePlaneType = PLANE_SINGLE;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        INIT_SET_SIZE_VERSION(&pHevcEnc->HevcComponent[i], OMX_VIDEO_PARAM_HEVCTYPE);
        pHevcEnc->HevcComponent[i].nPortIndex = i;
        pHevcEnc->HevcComponent[i].eProfile   = OMX_VIDEO_HEVCProfileMain;
        pHevcEnc->HevcComponent[i].eLevel     = OMX_VIDEO_HEVCMainTierLevel4;
    }
    pHevcEnc->nPFrames = 29;

    Exynos_OSAL_Memset(&pHevcEnc->TemporalSVC, 0, sizeof(EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC));
    INIT_SET_SIZE_VERSION(&pHevcEnc->TemporalSVC, EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC);
    pHevcEnc->TemporalSVC.nKeyFrameInterval = pHevcEnc->nPFrames + 1;
    pHevcEnc->TemporalSVC.nTemporalLayerCount   = 0;
    pHevcEnc->hMFCHevcHandle.bTemporalSVC       = OMX_FALSE;
    for (i = 0; i < OMX_VIDEO_ANDROID_MAXHEVCTEMPORALLAYERS; i++) {
        pHevcEnc->TemporalSVC.nTemporalLayerBitrateRatio[i] =
                                pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.nBitrate;
    }
    pHevcEnc->TemporalSVC.nMinQuantizer = pHevcEnc->qpRangeI.nMinQP;
    pHevcEnc->TemporalSVC.nMaxQuantizer = pHevcEnc->qpRangeI.nMaxQP;

    pOMXComponent->GetParameter      = &Exynos_HEVCEnc_GetParameter;
    pOMXComponent->SetParameter      = &Exynos_HEVCEnc_SetParameter;
    pOMXComponent->GetConfig         = &Exynos_HEVCEnc_GetConfig;
    pOMXComponent->SetConfig         = &Exynos_HEVCEnc_SetConfig;
    pOMXComponent->GetExtensionIndex = &Exynos_HEVCEnc_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &Exynos_HEVCEnc_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &Exynos_OMX_ComponentDeinit;

    pExynosComponent->exynos_codec_componentInit      = &Exynos_HEVCEnc_Init;
    pExynosComponent->exynos_codec_componentTerminate = &Exynos_HEVCEnc_Terminate;

    pVideoEnc->exynos_codec_srcInputProcess  = &Exynos_HEVCEnc_srcInputBufferProcess;
    pVideoEnc->exynos_codec_srcOutputProcess = &Exynos_HEVCEnc_srcOutputBufferProcess;
    pVideoEnc->exynos_codec_dstInputProcess  = &Exynos_HEVCEnc_dstInputBufferProcess;
    pVideoEnc->exynos_codec_dstOutputProcess = &Exynos_HEVCEnc_dstOutputBufferProcess;

    pVideoEnc->exynos_codec_start            = &HEVCCodecStart;
    pVideoEnc->exynos_codec_stop             = &HEVCCodecStop;
    pVideoEnc->exynos_codec_bufferProcessRun = &HEVCCodecOutputBufferProcessRun;
    pVideoEnc->exynos_codec_enqueueAllBuffer = &HEVCCodecEnqueueAllBuffer;

#if 0  /* unused code */
    pVideoEnc->exynos_checkInputFrame        = NULL;
    pVideoEnc->exynos_codec_getCodecInputPrivateData  = &GetCodecInputPrivateData;
#endif

    pVideoEnc->exynos_codec_getCodecOutputPrivateData = &GetCodecOutputPrivateData;

    pVideoEnc->exynos_codec_checkFormatSupport = &CheckFormatHWSupport;

    pVideoEnc->hSharedMemory = Exynos_OSAL_SharedMemory_Open();
    if (pVideoEnc->hSharedMemory == NULL) {
        Exynos_OSAL_Free(pHevcEnc);
        pHevcEnc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pHevcEnc->hMFCHevcHandle.videoInstInfo.eCodecType = VIDEO_CODING_HEVC;
    if (pExynosComponent->codecType == HW_VIDEO_ENC_SECURE_CODEC)
        pHevcEnc->hMFCHevcHandle.videoInstInfo.eSecurityType = VIDEO_SECURE;
    else
        pHevcEnc->hMFCHevcHandle.videoInstInfo.eSecurityType = VIDEO_NORMAL;

    if (Exynos_Video_GetInstInfo(&(pHevcEnc->hMFCHevcHandle.videoInstInfo), VIDEO_FALSE /* enc */) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%p][%s]: Exynos_Video_GetInstInfo is failed", pExynosComponent, __FUNCTION__);
        Exynos_OSAL_Free(pHevcEnc);
        pHevcEnc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] GetInstInfo for enc %d %d %d %d %d %d", pExynosComponent, __FUNCTION__,
            (pHevcEnc->hMFCHevcHandle.videoInstInfo.specificInfo.enc.bRGBSupport),
            (pHevcEnc->hMFCHevcHandle.videoInstInfo.specificInfo.enc.nSpareSize),
            (pHevcEnc->hMFCHevcHandle.videoInstInfo.specificInfo.enc.bTemporalSvcSupport),
            (pHevcEnc->hMFCHevcHandle.videoInstInfo.specificInfo.enc.bSkypeSupport),
            (pHevcEnc->hMFCHevcHandle.videoInstInfo.specificInfo.enc.bRoiInfoSupport),
            (pHevcEnc->hMFCHevcHandle.videoInstInfo.specificInfo.enc.bQpRangePBSupport));

    if (pHevcEnc->hMFCHevcHandle.videoInstInfo.specificInfo.enc.nSpareSize > 0)
        pVideoEnc->nInbufSpareSize = pHevcEnc->hMFCHevcHandle.videoInstInfo.specificInfo.enc.nSpareSize;

    Exynos_Input_SetSupportFormat(pExynosComponent);
    SetProfileLevel(pExynosComponent);

    pExynosComponent->currentState = OMX_StateLoaded;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = NULL;
    EXYNOS_HEVCENC_HANDLE           *pHevcEnc           = NULL;

    FunctionIn();

    if (hComponent == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    pOMXComponent = (OMX_COMPONENTTYPE *)hComponent;
    pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;

    Exynos_OSAL_SharedMemory_Close(pVideoEnc->hSharedMemory);

    Exynos_OSAL_Free(pExynosComponent->componentName);
    pExynosComponent->componentName = NULL;

    pHevcEnc = (EXYNOS_HEVCENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pHevcEnc != NULL) {
        Exynos_OSAL_Free(pHevcEnc);
        pHevcEnc = pVideoEnc->hCodecHandle = NULL;
    }

    ret = Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
    if (ret != OMX_ErrorNone)
        goto EXIT;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}
