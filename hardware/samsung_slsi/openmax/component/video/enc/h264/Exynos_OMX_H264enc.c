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
 * @file        Exynos_OMX_H264enc.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
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
#include "Exynos_OMX_H264enc.h"
#include "Exynos_OSAL_SharedMemory.h"
#include "Exynos_OSAL_Event.h"
#include "Exynos_OSAL_Queue.h"

#ifdef USE_ANDROID
#include "Exynos_OSAL_Android.h"
#endif

#ifdef USE_SKYPE_HD
#include "Exynos_OSAL_SkypeHD.h"
#endif

/* To use CSC_METHOD_HW in EXYNOS OMX, gralloc should allocate physical memory using FIMC */
/* It means GRALLOC_USAGE_HW_FIMC1 should be set on Native Window usage */
#include "csc.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "EXYNOS_H264_ENC"
//#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

static OMX_ERRORTYPE SetProfileLevel(
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    OMX_ERRORTYPE                    ret            = OMX_ErrorNone;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc      = NULL;
    EXYNOS_H264ENC_HANDLE           *pH264Enc       = NULL;

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

    pH264Enc = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pH264Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pH264Enc->hMFCH264Handle.profiles[nProfileCnt++] = OMX_VIDEO_AVCProfileBaseline;
    pH264Enc->hMFCH264Handle.profiles[nProfileCnt++] = OMX_VIDEO_AVCProfileMain;
    pH264Enc->hMFCH264Handle.profiles[nProfileCnt++] = OMX_VIDEO_AVCProfileHigh;
    pH264Enc->hMFCH264Handle.profiles[nProfileCnt++] = (OMX_VIDEO_AVCPROFILETYPE)OMX_VIDEO_AVCProfileConstrainedBaseline;
    pH264Enc->hMFCH264Handle.profiles[nProfileCnt++] = (OMX_VIDEO_AVCPROFILETYPE)OMX_VIDEO_AVCProfileConstrainedHigh;
    pH264Enc->hMFCH264Handle.nProfileCnt = nProfileCnt;

    switch (pH264Enc->hMFCH264Handle.videoInstInfo.HwVersion) {
    case MFC_100:
    case MFC_101:
        pH264Enc->hMFCH264Handle.maxLevel = OMX_VIDEO_AVCLevel52;
        break;
    case MFC_80:
    case MFC_90:
    case MFC_1010:
        pH264Enc->hMFCH264Handle.maxLevel = OMX_VIDEO_AVCLevel51;
        break;
    case MFC_61:
    case MFC_65:
    case MFC_72:
    case MFC_723:
    case MFC_77:
    case MFC_1011:
        pH264Enc->hMFCH264Handle.maxLevel = OMX_VIDEO_AVCLevel42;
        break;
    case MFC_51:
    case MFC_78:
    case MFC_78D:
    case MFC_92:
    case MFC_1020:
    default:
        pH264Enc->hMFCH264Handle.maxLevel = OMX_VIDEO_AVCLevel4;
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
    EXYNOS_H264ENC_HANDLE           *pH264Enc       = NULL;

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

    pH264Enc = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pH264Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    while ((pH264Enc->hMFCH264Handle.maxLevel >> nLevelCnt) > 0) {
        nLevelCnt++;
    }

    if ((pH264Enc->hMFCH264Handle.nProfileCnt == 0) ||
        (nLevelCnt == 0)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : there is no any profile/level", __FUNCTION__);
        ret = OMX_ErrorUndefined;
        goto EXIT;
    }

    nMaxIndex = pH264Enc->hMFCH264Handle.nProfileCnt * nLevelCnt;
    if (nMaxIndex <= pProfileLevelType->nProfileIndex) {
        ret = OMX_ErrorNoMore;
        goto EXIT;
    }

    pProfileLevelType->eProfile = pH264Enc->hMFCH264Handle.profiles[pProfileLevelType->nProfileIndex / nLevelCnt];
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
    EXYNOS_H264ENC_HANDLE           *pH264Enc   = NULL;

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

    pH264Enc = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pH264Enc == NULL)
        goto EXIT;

    while ((pH264Enc->hMFCH264Handle.maxLevel >> nLevelCnt++) > 0);

    if ((pH264Enc->hMFCH264Handle.nProfileCnt == 0) ||
        (nLevelCnt == 0)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s : there is no any profile/level", __FUNCTION__);
        goto EXIT;
    }

    for (i = 0; i < pH264Enc->hMFCH264Handle.nProfileCnt; i++) {
        if (pH264Enc->hMFCH264Handle.profiles[i] == pProfileLevelType->eProfile) {
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

static OMX_U32 OMXAVCProfileToProfileIDC(OMX_VIDEO_AVCPROFILETYPE profile)
{
    OMX_U32 ret = 0;

    if (profile == OMX_VIDEO_AVCProfileBaseline)
        ret = 0;
    else if ((EXYNOS_OMX_VIDEO_AVCPROFILETYPE)profile == OMX_VIDEO_AVCProfileConstrainedBaseline)
        ret = 1;
    else if (profile == OMX_VIDEO_AVCProfileMain)
        ret = 2;
    else if (profile == OMX_VIDEO_AVCProfileHigh)
        ret = 4;
    else if ((EXYNOS_OMX_VIDEO_AVCPROFILETYPE)profile == OMX_VIDEO_AVCProfileConstrainedHigh)
        ret = 17;

    return ret;
}

static OMX_U32 OMXAVCLevelToLevelIDC(OMX_VIDEO_AVCLEVELTYPE level)
{
    OMX_U32 ret = 11; //default OMX_VIDEO_AVCLevel4

    if (level == OMX_VIDEO_AVCLevel1)
        ret = 0;
    else if (level == OMX_VIDEO_AVCLevel1b)
        ret = 1;
    else if (level == OMX_VIDEO_AVCLevel11)
        ret = 2;
    else if (level == OMX_VIDEO_AVCLevel12)
        ret = 3;
    else if (level == OMX_VIDEO_AVCLevel13)
        ret = 4;
    else if (level == OMX_VIDEO_AVCLevel2)
        ret = 5;
    else if (level == OMX_VIDEO_AVCLevel21)
        ret = 6;
    else if (level == OMX_VIDEO_AVCLevel22)
        ret = 7;
    else if (level == OMX_VIDEO_AVCLevel3)
        ret = 8;
    else if (level == OMX_VIDEO_AVCLevel31)
        ret = 9;
    else if (level == OMX_VIDEO_AVCLevel32)
        ret = 10;
    else if (level == OMX_VIDEO_AVCLevel4)
        ret = 11;
    else if (level == OMX_VIDEO_AVCLevel41)
        ret = 12;
    else if (level == OMX_VIDEO_AVCLevel42)
        ret = 13;

    return ret;
}

static OMX_U8 *FindDelimiter(OMX_U8 *pBuffer, OMX_U32 size)
{
    OMX_U32 i;

    for (i = 0; i < size - 3; i++) {
        if ((pBuffer[i] == 0x00)   &&
            (pBuffer[i + 1] == 0x00) &&
            (pBuffer[i + 2] == 0x00) &&
            (pBuffer[i + 3] == 0x01))
            return (pBuffer + i);
    }

    return NULL;
}

static void Print_H264Enc_Param(ExynosVideoEncParam *pEncParam)
{
    ExynosVideoEncCommonParam *pCommonParam = &pEncParam->commonParam;
    ExynosVideoEncH264Param   *pH264Param   = &pEncParam->codecParam.h264;

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

    /* H.264 specific parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "ProfileIDC              : %d", pH264Param->ProfileIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LevelIDC                : %d", pH264Param->LevelIDC);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "FrameQp_B               : %d", pH264Param->FrameQp_B);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "FrameRate               : %d", pH264Param->FrameRate);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "SliceArgument           : %d", pH264Param->SliceArgument);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "NumberBFrames           : %d", pH264Param->NumberBFrames);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "NumberReferenceFrames   : %d", pH264Param->NumberReferenceFrames);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "NumberRefForPframes     : %d", pH264Param->NumberRefForPframes);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LoopFilterDisable       : %d", pH264Param->LoopFilterDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LoopFilterAlphaC0Offset : %d", pH264Param->LoopFilterAlphaC0Offset);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LoopFilterBetaOffset    : %d", pH264Param->LoopFilterBetaOffset);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "SymbolMode              : %d", pH264Param->SymbolMode);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "PictureInterlace        : %d", pH264Param->PictureInterlace);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "Transform8x8Mode        : %d", pH264Param->Transform8x8Mode);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "DarkDisable             : %d", pH264Param->DarkDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "SmoothDisable           : %d", pH264Param->SmoothDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "StaticDisable           : %d", pH264Param->StaticDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "ActivityDisable         : %d", pH264Param->ActivityDisable);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "HierarType:             : %d", pH264Param->HierarType);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "VuiRestrictionEnable:   : %d", pH264Param->VuiRestrictionEnable);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "HeaderWithIFrame:       : %d", pH264Param->HeaderWithIFrame);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "SarEnable:              : %d", pH264Param->SarEnable);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "SarIndex:               : %d", pH264Param->SarIndex);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "SarWidth:               : %d", pH264Param->SarWidth);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "SarHeight:              : %d", pH264Param->SarHeight);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "LTREnable:              : %d", pH264Param->LTREnable);

    /* rate control related parameters */
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "EnableFRMRateControl    : %d", pCommonParam->EnableFRMRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "EnableMBRateControl     : %d", pCommonParam->EnableMBRateControl);
    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "CBRPeriodRf             : %d", pCommonParam->CBRPeriodRf);
}

static void Set_H264Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = NULL;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc          = NULL;
    EXYNOS_MFC_H264ENC_HANDLE     *pMFCH264Handle    = NULL;
    OMX_COLOR_FORMATTYPE           eColorFormat      = OMX_COLOR_FormatUnused;

    ExynosVideoEncParam       *pEncParam    = NULL;
    ExynosVideoEncCommonParam *pCommonParam = NULL;
    ExynosVideoEncH264Param   *pH264Param   = NULL;

    int i;

    pVideoEnc           = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc            = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    pMFCH264Handle      = &pH264Enc->hMFCH264Handle;
    pExynosInputPort    = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    pExynosOutputPort   = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    pEncParam       = &pMFCH264Handle->encParam;
    pCommonParam    = &pEncParam->commonParam;
    pH264Param      = &pEncParam->codecParam.h264;

    pEncParam->eCompressionFormat = VIDEO_CODING_AVC;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "eCompressionFormat: %d", pEncParam->eCompressionFormat);

    /* common parameters */
    if ((pVideoEnc->eRotationType == ROTATE_0) ||
        (pVideoEnc->eRotationType == ROTATE_180)) {
        pCommonParam->SourceWidth  = pExynosOutputPort->portDefinition.format.video.nFrameWidth;
        pCommonParam->SourceHeight = pExynosOutputPort->portDefinition.format.video.nFrameHeight;
    } else {
        pCommonParam->SourceWidth  = pExynosOutputPort->portDefinition.format.video.nFrameHeight;
        pCommonParam->SourceHeight = pExynosOutputPort->portDefinition.format.video.nFrameWidth;
    }
    pCommonParam->IDRPeriod    = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
    pCommonParam->SliceMode    = pH264Enc->AVCSliceFmo.eSliceMode;
    pCommonParam->Bitrate      = pExynosOutputPort->portDefinition.format.video.nBitrate;
    pCommonParam->FrameQp      = pVideoEnc->quantization.nQpI;
    pCommonParam->FrameQp_P    = pVideoEnc->quantization.nQpP;

    pCommonParam->QpRange.QpMin_I = pH264Enc->qpRangeI.nMinQP;
    pCommonParam->QpRange.QpMax_I = pH264Enc->qpRangeI.nMaxQP;
    pCommonParam->QpRange.QpMin_P = pH264Enc->qpRangeP.nMinQP;
    pCommonParam->QpRange.QpMax_P = pH264Enc->qpRangeP.nMaxQP;
    pCommonParam->QpRange.QpMin_B = pH264Enc->qpRangeB.nMinQP;
    pCommonParam->QpRange.QpMax_B = pH264Enc->qpRangeB.nMaxQP;

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
    pCommonParam->FrameMap = Exynos_OSAL_OMX2VideoFormat(eColorFormat, pExynosInputPort->ePlaneType);

    /* H.264 specific parameters */
    pH264Param->ProfileIDC   = OMXAVCProfileToProfileIDC(pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eProfile);    /*0: OMX_VIDEO_AVCProfileMain */
    pH264Param->LevelIDC     = OMXAVCLevelToLevelIDC(pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eLevel);    /*40: OMX_VIDEO_AVCLevel4 */
    pH264Param->FrameQp_B    = pVideoEnc->quantization.nQpB;
    pH264Param->FrameRate    = (pExynosInputPort->portDefinition.format.video.xFramerate) >> 16;
    if (pH264Enc->AVCSliceFmo.eSliceMode == OMX_VIDEO_SLICEMODE_AVCDefault)
        pH264Param->SliceArgument = 0;    /* Slice mb/byte size number */
    else
        pH264Param->SliceArgument = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nSliceHeaderSpacing;

    pH264Param->NumberBFrames           = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nBFrames;   /* 0 ~ 2 */
    pH264Param->NumberRefForPframes     = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nRefFrames; /* 1 ~ 2 */
    pH264Param->NumberReferenceFrames   = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nRefFrames;

    pH264Param->LoopFilterDisable     = 0;    /* 1: Loop Filter Disable, 0: Filter Enable */
    pH264Param->LoopFilterAlphaC0Offset = 0;
    pH264Param->LoopFilterBetaOffset    = 0;
    pH264Param->SymbolMode       = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].bEntropyCodingCABAC;    /* 0: CAVLC, 1: CABAC */
    pH264Param->PictureInterlace = 0;
    pH264Param->Transform8x8Mode = 1;    /* 0: 4x4, 1: allow 8x8 */
    pH264Param->DarkDisable     = 1;
    pH264Param->SmoothDisable   = 1;
    pH264Param->StaticDisable   = 1;
    pH264Param->ActivityDisable = 1;

    /* Temporal SVC */
    pH264Param->TemporalSVC.nTemporalLayerCount = (unsigned int)pH264Enc->TemporalSVC.nTemporalLayerCount;
    for (i = 0; i < OMX_VIDEO_ANDROID_MAXAVCTEMPORALLAYERS; i++)
        pH264Param->TemporalSVC.nTemporalLayerBitrateRatio[i] = (unsigned int)pH264Enc->TemporalSVC.nTemporalLayerBitrateRatio[i];

    /* Hierarchal P & B */
    pH264Param->HierarType = pH264Enc->eHierarchicalType;

    /* SPS VUI */
    if (pH264Enc->stSarParam.SarEnable == OMX_TRUE) {
        pH264Param->SarEnable = 1;
        /* TBD */
        /*need value check*/
        pH264Param->SarIndex  = pH264Enc->stSarParam.SarIndex;
        pH264Param->SarWidth  = pH264Enc->stSarParam.SarWidth;
        pH264Param->SarHeight = pH264Enc->stSarParam.SarHeight;

    } else {
        pH264Param->SarEnable = 0;
        pH264Param->SarIndex  = 0;
        pH264Param->SarWidth  = 0;
        pH264Param->SarHeight = 0;
    }

    if (pH264Enc->bLowLatency == OMX_TRUE) {
        pH264Param->HeaderWithIFrame = 0;           /* 1: header + first frame */
        pH264Param->LoopFilterDisable = 0;          /* 1: disable, 0: enable */
        pH264Param->VuiRestrictionEnable = (int)OMX_TRUE;
        pVideoEnc->eControlRate[OUTPUT_PORT_INDEX] = OMX_Video_ControlRateDisable;
        pCommonParam->EnableFRMQpControl = 1;       /* 0: Disable,  1: Per frame QP */
    } else {
        pH264Param->VuiRestrictionEnable = (int)OMX_FALSE;
    }

    if (pMFCH264Handle->bLTREnable == OMX_TRUE)
        pH264Param->LTREnable = 1;
    else
        pH264Param->LTREnable = 0;

    if (pMFCH264Handle->bRoiInfo == OMX_TRUE)
        pH264Param->ROIEnable = 1;
    else
        pH264Param->ROIEnable = 0;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]: 0x%x", pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]);
    /* rate control related parameters */
    switch ((int)pVideoEnc->eControlRate[OUTPUT_PORT_INDEX]) {
    case OMX_Video_ControlRateDisable:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode DBR");
        pCommonParam->EnableFRMRateControl = 0;    /* 0: Disable,  1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 0;    /* 0: Disable,  1: MB level RC */
        pCommonParam->CBRPeriodRf          = 100;
        break;
    case OMX_Video_ControlRateConstantVTCall:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode CBR VT Call");
        pCommonParam->EnableFRMRateControl = 1;    /* 0: Disable,  1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1;    /* 0: Disable,  1: MB level RC */
        pCommonParam->CBRPeriodRf          = 5;
        break;
    case OMX_Video_ControlRateConstant:
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode CBR");
        pCommonParam->EnableFRMRateControl = 1;    /* 0: Disable,  1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1;    /* 0: Disable,  1: MB level RC */
        pCommonParam->CBRPeriodRf          = 10;
        break;
    case OMX_Video_ControlRateVariable:
    default: /*Android default */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "Video Encode VBR");
        pCommonParam->EnableFRMRateControl = 1;    /* 0: Disable,  1: Frame level RC */
        pCommonParam->EnableMBRateControl  = 1;    /* 0: Disable,  1: MB level RC */
        pCommonParam->CBRPeriodRf          = 100;
        break;
    }

//    Print_H264Enc_Param(pEncParam);
}

static void Change_H264Enc_Param(EXYNOS_OMX_BASECOMPONENT *pExynosComponent)
{
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc          = NULL;
    EXYNOS_MFC_H264ENC_HANDLE     *pMFCH264Handle    = NULL;
    OMX_PTR                        pDynamicConfigCMD = NULL;
    OMX_PTR                        pConfigData       = NULL;
    OMX_S32                        nCmdIndex         = 0;
    ExynosVideoEncOps             *pEncOps           = NULL;
    int                            nValue            = 0;

    int i;

    pVideoEnc       = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc        = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    pMFCH264Handle  = &pH264Enc->hMFCH264Handle;
    pEncOps         = pMFCH264Handle->pEncOps;

    pDynamicConfigCMD = (OMX_PTR)Exynos_OSAL_Dequeue(&pExynosComponent->dynamicConfigQ);
    if (pDynamicConfigCMD == NULL)
        goto EXIT;

    nCmdIndex   = *(OMX_S32 *)pDynamicConfigCMD;
    pConfigData = (OMX_PTR)((OMX_U8 *)pDynamicConfigCMD + sizeof(OMX_S32));

    switch ((int)nCmdIndex) {
    case OMX_IndexConfigVideoIntraVOPRefresh:
    {
        nValue = VIDEO_FRAME_I;
        pEncOps->Set_FrameType(pMFCH264Handle->hMFCHandle, nValue);
        pVideoEnc->IntraRefreshVOP = OMX_FALSE;
    }
        break;
    case OMX_IndexConfigVideoIntraPeriod:
    {
        OMX_S32 nPFrames = (*((OMX_S32 *)pConfigData)) - 1;
        nValue = nPFrames + 1;
        pEncOps->Set_IDRPeriod(pH264Enc->hMFCH264Handle.hMFCHandle, nValue);
    }
        break;
    case OMX_IndexConfigVideoAVCIntraPeriod:
    {
        OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pAVCIntraPeriod = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pConfigData;
        nValue = pAVCIntraPeriod->nIDRPeriod;
        pEncOps->Set_IDRPeriod(pMFCH264Handle->hMFCHandle, nValue);
    }
        break;
    case OMX_IndexConfigVideoBitrate:
    {
        OMX_VIDEO_CONFIG_BITRATETYPE *pConfigBitrate = (OMX_VIDEO_CONFIG_BITRATETYPE *)pConfigData;
        if (pVideoEnc->eControlRate[OUTPUT_PORT_INDEX] != OMX_Video_ControlRateDisable) {
            nValue = pConfigBitrate->nEncodeBitrate;
            pEncOps->Set_BitRate(pH264Enc->hMFCH264Handle.hMFCHandle, nValue);
        }
    }
        break;
    case OMX_IndexConfigVideoFramerate:
    {
        OMX_CONFIG_FRAMERATETYPE *pConfigFramerate = (OMX_CONFIG_FRAMERATETYPE *)pConfigData;
        OMX_U32                   nPortIndex       = pConfigFramerate->nPortIndex;
        if (nPortIndex == INPUT_PORT_INDEX) {
            nValue = (pConfigFramerate->xEncodeFramerate) >> 16;
            pEncOps->Set_FrameRate(pH264Enc->hMFCH264Handle.hMFCHandle, nValue);
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

        pEncOps->Set_QpRange(pMFCH264Handle->hMFCHandle, qpRange);
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

        pEncOps->Set_QosRatio(pMFCH264Handle->hMFCHandle, nValue);
        pVideoEnc->bQosChanged = OMX_FALSE;
    }
        break;
#ifdef USE_QOS_CTRL
    case OMX_IndexVendorSetQosRatio:  /* MSRND */
    {
        EXYNOS_OMX_VIDEO_CONFIG_QOSINFO *pQosInfo = (EXYNOS_OMX_VIDEO_CONFIG_QOSINFO *)pConfigData;
        nValue = pQosInfo->nQosRatio;
        pEncOps->Set_QosRatio(pMFCH264Handle->hMFCHandle, nValue);
        pVideoEnc->bQosChanged = OMX_FALSE;
    }
        break;
#endif
    case OMX_IndexConfigVideoTemporalSVC:
    {
        EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *pTemporalSVC = (EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *)pConfigData;
        ExynosVideoQPRange qpRange;

        qpRange.QpMin_I = pTemporalSVC->nMinQuantizer;
        qpRange.QpMax_I = pTemporalSVC->nMaxQuantizer;
        qpRange.QpMin_P = pTemporalSVC->nMinQuantizer;
        qpRange.QpMax_P = pTemporalSVC->nMaxQuantizer;
        qpRange.QpMin_B = pTemporalSVC->nMinQuantizer;
        qpRange.QpMax_B = pTemporalSVC->nMaxQuantizer;

        pEncOps->Set_QpRange(pMFCH264Handle->hMFCHandle, qpRange);
        pEncOps->Set_IDRPeriod(pMFCH264Handle->hMFCHandle, pTemporalSVC->nKeyFrameInterval);

        /* Temporal SVC */
        TemporalLayerShareBuffer TemporalSVC;
        Exynos_OSAL_Memset(&TemporalSVC, 0, sizeof(TemporalLayerShareBuffer));
        TemporalSVC.nTemporalLayerCount = (unsigned int)pTemporalSVC->nTemporalLayerCount;
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "nTemporalLayerCount     : %d", TemporalSVC.nTemporalLayerCount);
        for (i = 0; i < OMX_VIDEO_ANDROID_MAXAVCTEMPORALLAYERS; i++) {
            TemporalSVC.nTemporalLayerBitrateRatio[i] = (unsigned int)pTemporalSVC->nTemporalLayerBitrateRatio[i];
            Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "nTempBitrateRatio[%d]   : %d", i, TemporalSVC.nTemporalLayerBitrateRatio[i]);
        }
        pEncOps->Set_LayerChange(pMFCH264Handle->hMFCHandle, TemporalSVC);
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
            pEncOps->Set_RoiInfo(pMFCH264Handle->hMFCHandle, &RoiInfo);
        else
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "%s: Not supported control: Set_RoiInfo", __func__);
    }
        break;
    default:
#ifdef USE_SKYPE_HD
        if (pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.bSkypeSupport == VIDEO_TRUE)
            Change_H264Enc_SkypeHDParam(pExynosComponent, pDynamicConfigCMD);
#endif
        break;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] nCmdIndex %x nValue %d ", pExynosComponent, __FUNCTION__, (int)nCmdIndex, nValue);
    Exynos_OSAL_Free(pDynamicConfigCMD);

    Set_H264Enc_Param(pExynosComponent);

EXIT:
    return;
}

#if 0  /* unused code */
OMX_ERRORTYPE GetCodecInputPrivateData(OMX_PTR codecBuffer, OMX_PTR addr[], OMX_U32 size[])
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;

EXIT:
    return ret;
}
#endif

OMX_ERRORTYPE GetCodecOutputPrivateData(OMX_PTR codecBuffer, OMX_PTR *pVirtAddr, OMX_U32 *dataSize)
{
    OMX_ERRORTYPE       ret = OMX_ErrorNone;
    ExynosVideoBuffer  *pCodecBuffer;

    if (codecBuffer == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pCodecBuffer = (ExynosVideoBuffer *)codecBuffer;

    if (pVirtAddr != NULL)
        *pVirtAddr = pCodecBuffer->planes[0].addr;

    if (dataSize != NULL)
        *dataSize = pCodecBuffer->planes[0].allocSize;

    pCodecBuffer = (ExynosVideoBuffer *)codecBuffer;

EXIT:
    return ret;
}

OMX_BOOL CheckFormatHWSupport(
    EXYNOS_OMX_BASECOMPONENT    *pExynosComponent,
    OMX_COLOR_FORMATTYPE         eColorFormat)
{
    OMX_BOOL                         ret            = OMX_FALSE;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc      = NULL;
    EXYNOS_H264ENC_HANDLE           *pH264Enc       = NULL;
    EXYNOS_OMX_BASEPORT             *pInputPort     = NULL;
    ExynosVideoColorFormatType       eVideoFormat   = VIDEO_CODING_UNKNOWN;
    int i;

    FunctionIn();

    if (pExynosComponent == NULL)
        goto EXIT;

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    if (pVideoEnc == NULL)
        goto EXIT;

    pH264Enc = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pH264Enc == NULL)
        goto EXIT;
    pInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    eVideoFormat = (ExynosVideoColorFormatType)Exynos_OSAL_OMX2VideoFormat(eColorFormat, pInputPort->ePlaneType);

    for (i = 0; i < VIDEO_COLORFORMAT_MAX; i++) {
        if (pH264Enc->hMFCH264Handle.videoInstInfo.supportFormat[i] == VIDEO_COLORFORMAT_UNKNOWN)
            break;

        if (pH264Enc->hMFCH264Handle.videoInstInfo.supportFormat[i] == eVideoFormat) {
            ret = OMX_TRUE;
            break;
        }
    }

EXIT:
    return ret;
}

OMX_ERRORTYPE H264CodecOpen(EXYNOS_H264ENC_HANDLE *pH264Enc, ExynosVideoInstInfo *pVideoInstInfo)
{
    OMX_ERRORTYPE           ret = OMX_ErrorNone;

    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pH264Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }

    /* alloc ops structure */
    pEncOps = (ExynosVideoEncOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoEncOps));
    pInbufOps = (ExynosVideoEncBufferOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoEncBufferOps));
    pOutbufOps = (ExynosVideoEncBufferOps *)Exynos_OSAL_Malloc(sizeof(ExynosVideoEncBufferOps));

    if ((pEncOps == NULL) || (pInbufOps == NULL) || (pOutbufOps == NULL)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate encoder ops buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pH264Enc->hMFCH264Handle.pEncOps = pEncOps;
    pH264Enc->hMFCH264Handle.pInbufOps = pInbufOps;
    pH264Enc->hMFCH264Handle.pOutbufOps = pOutbufOps;

    /* function pointer mapping */
    pEncOps->nSize = sizeof(ExynosVideoEncOps);
    pInbufOps->nSize = sizeof(ExynosVideoEncBufferOps);
    pOutbufOps->nSize = sizeof(ExynosVideoEncBufferOps);

    Exynos_Video_Register_Encoder(pEncOps, pInbufOps, pOutbufOps);

    /* check mandatory functions for encoder ops */
    if ((pEncOps->Init == NULL) || (pEncOps->Finalize == NULL) ||
        (pEncOps->Set_FrameTag == NULL) || (pEncOps->Get_FrameTag == NULL)) {
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
    pH264Enc->hMFCH264Handle.hMFCHandle = pH264Enc->hMFCH264Handle.pEncOps->Init(pVideoInstInfo);
    if (pH264Enc->hMFCH264Handle.hMFCHandle == NULL) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to allocate context buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    if (ret != OMX_ErrorNone) {
        if (pEncOps != NULL) {
            Exynos_OSAL_Free(pEncOps);
            pH264Enc->hMFCH264Handle.pEncOps = NULL;
        }
        if (pInbufOps != NULL) {
            Exynos_OSAL_Free(pInbufOps);
            pH264Enc->hMFCH264Handle.pInbufOps = NULL;
        }
        if (pOutbufOps != NULL) {
            Exynos_OSAL_Free(pOutbufOps);
            pH264Enc->hMFCH264Handle.pOutbufOps = NULL;
        }
    }

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecClose(EXYNOS_H264ENC_HANDLE *pH264Enc)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;

    FunctionIn();

    if (pH264Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;

    if (hMFCHandle != NULL) {
        pEncOps->Finalize(hMFCHandle);
        hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle = NULL;
    }

    /* Unregister function pointers */
    Exynos_Video_Unregister_Encoder(pEncOps, pInbufOps, pOutbufOps);

    if (pOutbufOps != NULL) {
        Exynos_OSAL_Free(pOutbufOps);
        pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps = NULL;
    }
    if (pInbufOps != NULL) {
        Exynos_OSAL_Free(pInbufOps);
        pInbufOps = pH264Enc->hMFCH264Handle.pInbufOps = NULL;
    }
    if (pEncOps != NULL) {
        Exynos_OSAL_Free(pEncOps);
        pEncOps = pH264Enc->hMFCH264Handle.pEncOps = NULL;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecStart(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_H264ENC_HANDLE   *pH264Enc = NULL;

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

    pH264Enc = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pH264Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX)
        pInbufOps->Run(hMFCHandle);
    else if (nPortIndex == OUTPUT_PORT_INDEX)
        pOutbufOps->Run(hMFCHandle);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecStop(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_H264ENC_HANDLE   *pH264Enc = NULL;

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
    pH264Enc = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pH264Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;

    if ((nPortIndex == INPUT_PORT_INDEX) && (pInbufOps != NULL))
        pInbufOps->Stop(hMFCHandle);
    else if ((nPortIndex == OUTPUT_PORT_INDEX) && (pOutbufOps != NULL))
        pOutbufOps->Stop(hMFCHandle);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecOutputBufferProcessRun(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    void                    *hMFCHandle = NULL;
    ExynosVideoEncOps       *pEncOps    = NULL;
    ExynosVideoEncBufferOps *pInbufOps  = NULL;
    ExynosVideoEncBufferOps *pOutbufOps = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_H264ENC_HANDLE   *pH264Enc = NULL;

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
    pH264Enc = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pH264Enc == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;

    if (nPortIndex == INPUT_PORT_INDEX) {
        if (pH264Enc->bSourceStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pH264Enc->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    if (nPortIndex == OUTPUT_PORT_INDEX) {
        if (pH264Enc->bDestinationStart == OMX_FALSE) {
            Exynos_OSAL_SignalSet(pH264Enc->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecRegistCodecBuffers(
    OMX_COMPONENTTYPE   *pOMXComponent,
    OMX_U32              nPortIndex,
    int                  nBufferCnt)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE           *pH264Enc           = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    void                            *hMFCHandle         = pH264Enc->hMFCH264Handle.hMFCHandle;
    CODEC_ENC_BUFFER               **ppCodecBuffer      = NULL;
    ExynosVideoEncBufferOps         *pBufOps            = NULL;
    ExynosVideoPlane                *pPlanes            = NULL;

    int nPlaneCnt = 0;
    int i, j;

    FunctionIn();

    if (nPortIndex == INPUT_PORT_INDEX) {
        ppCodecBuffer   = &(pVideoEnc->pMFCEncInputBuffer[0]);
        pBufOps         = pH264Enc->hMFCH264Handle.pInbufOps;
    } else {
        ppCodecBuffer   = &(pVideoEnc->pMFCEncOutputBuffer[0]);
        pBufOps         = pH264Enc->hMFCH264Handle.pOutbufOps;
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

OMX_ERRORTYPE H264CodecEnqueueAllBuffer(OMX_COMPONENTTYPE *pOMXComponent, OMX_U32 nPortIndex)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    void                          *hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    int i, nOutbufs;

    ExynosVideoEncOps       *pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;

    FunctionIn();

    if ((nPortIndex != INPUT_PORT_INDEX) && (nPortIndex != OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorBadPortIndex;
        goto EXIT;
    }

    if ((nPortIndex == INPUT_PORT_INDEX) &&
        (pH264Enc->bSourceStart == OMX_TRUE)) {
        Exynos_CodecBufferReset(pExynosComponent, INPUT_PORT_INDEX);

        for (i = 0; i < MFC_INPUT_BUFFER_NUM_MAX; i++)  {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]->pVirAddr[0]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]->pVirAddr[0]);
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pVideoEnc->pMFCEncInputBuffer[%d]->pVirAddr[1]: 0x%x", i, pVideoEnc->pMFCEncInputBuffer[i]->pVirAddr[1]);

            Exynos_CodecBufferEnqueue(pExynosComponent, INPUT_PORT_INDEX, pVideoEnc->pMFCEncInputBuffer[i]);
        }

        pInbufOps->Clear_Queue(hMFCHandle);
    } else if ((nPortIndex == OUTPUT_PORT_INDEX) &&
               (pH264Enc->bDestinationStart == OMX_TRUE)) {
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

OMX_ERRORTYPE H264CodecSrcSetup(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE                  ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_MFC_H264ENC_HANDLE     *pMFCH264Handle    = &pH264Enc->hMFCH264Handle;
    void                          *hMFCHandle = pMFCH264Handle->hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32                        oneFrameSize = pSrcInputData->dataLen;

    ExynosVideoEncOps       *pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;
    ExynosVideoEncParam     *pEncParam    = NULL;

    ExynosVideoGeometry      bufferConf;
    OMX_U32                  inputBufferNumber = 0;
    int i, nOutbufs;

    FunctionIn();

    if ((oneFrameSize <= 0) && (pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] FBD with EOS will be processed through bypassBufferInfoQ",
                                                pExynosComponent, __FUNCTION__);

        BYPASS_BUFFER_INFO *pBufferInfo = (BYPASS_BUFFER_INFO *)Exynos_OSAL_Malloc(sizeof(BYPASS_BUFFER_INFO));
        if (pBufferInfo == NULL) {
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }

        pBufferInfo->nFlags     = pSrcInputData->nFlags;
        pBufferInfo->timeStamp  = pSrcInputData->timeStamp;

        ret = Exynos_OSAL_Queue(&pH264Enc->bypassBufferInfoQ, (void *)pBufferInfo);
        Exynos_OSAL_SignalSet(pH264Enc->hDestinationStartEvent);  /* awake dstInput thread */

        ret = OMX_ErrorNone;
        goto EXIT;
    }

    Set_H264Enc_Param(pExynosComponent);

    pEncParam = &pMFCH264Handle->encParam;
    if (pEncOps->Set_EncParam) {
        if(pEncOps->Set_EncParam(pH264Enc->hMFCH264Handle.hMFCHandle, pEncParam) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to set geometry for input buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    }
    Print_H264Enc_Param(pEncParam);

    if (pMFCH264Handle->bPrependSpsPpsToIdr == OMX_TRUE) {
        if (pEncOps->Enable_PrependSpsPpsToIdr)
            pEncOps->Enable_PrependSpsPpsToIdr(pH264Enc->hMFCH264Handle.hMFCHandle);
        else
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "%s: Not supported control: Enable_PrependSpsPpsToIdr", __func__);
    }

    /* input buffer info: only 3 config values needed */
    Exynos_OSAL_Memset(&bufferConf, 0, sizeof(bufferConf));
    bufferConf.eColorFormat = pEncParam->commonParam.FrameMap;
    if ((pVideoEnc->eRotationType == ROTATE_0) ||
        (pVideoEnc->eRotationType == ROTATE_180)) {
        bufferConf.nFrameWidth = pExynosInputPort->portDefinition.format.video.nFrameWidth;
        bufferConf.nFrameHeight = pExynosInputPort->portDefinition.format.video.nFrameHeight;
        bufferConf.nStride = ALIGN(pExynosInputPort->portDefinition.format.video.nFrameWidth, 16);
    } else {
        bufferConf.nFrameWidth = pExynosInputPort->portDefinition.format.video.nFrameHeight;
        bufferConf.nFrameHeight = pExynosInputPort->portDefinition.format.video.nFrameWidth;
        bufferConf.nStride = ALIGN(pExynosInputPort->portDefinition.format.video.nFrameHeight, 16);
    }
    bufferConf.nPlaneCnt = Exynos_GetPlaneFromPort(pExynosInputPort);
    pInbufOps->Set_Shareable(hMFCHandle);
    inputBufferNumber = MAX_INPUTBUFFER_NUM_DYNAMIC;

    if (pExynosInputPort->bufferProcessType & BUFFER_COPY) {
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
    if (pInbufOps->Setup(hMFCHandle, inputBufferNumber) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup input buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if ((pExynosInputPort->bufferProcessType & BUFFER_SHARE)
#ifdef USE_METADATABUFFERTYPE
        && (pExynosInputPort->bStoreMetaData != OMX_TRUE)
#endif
        ) {
        ret = OMX_ErrorNotImplemented;
        goto EXIT;
    }

    pH264Enc->hMFCH264Handle.bConfiguredMFCSrc = OMX_TRUE;
    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE H264CodecDstSetup(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret                  = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent     = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc            = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc             = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_MFC_H264ENC_HANDLE     *pMFCH264Handle       = &pH264Enc->hMFCH264Handle;
    void                          *hMFCHandle           = pMFCH264Handle->hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort     = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort    = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoEncOps       *pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;
    ExynosVideoGeometry      bufferConf;

    unsigned int nAllocLen[VIDEO_BUFFER_MAX_PLANES] = {0, 0, 0};
    unsigned int nDataLen[VIDEO_BUFFER_MAX_PLANES]  = {0, 0 ,0};
    int i, nOutbufs;

    FunctionIn();

    int nOutBufSize = pExynosOutputPort->portDefinition.nBufferSize;
    if (pExynosOutputPort->bStoreMetaData == OMX_TRUE) {
        nOutBufSize = ALIGN(pExynosOutputPort->portDefinition.format.video.nFrameWidth *
                            pExynosOutputPort->portDefinition.format.video.nFrameHeight * 3 / 2, 512);
    }

    /* set geometry for output (dst) */
    if (pOutbufOps->Set_Geometry) {
        /* output buffer info: only 2 config values needed */
        bufferConf.eCompressionFormat = VIDEO_CODING_AVC;
        bufferConf.nSizeImage = nOutBufSize;
        bufferConf.nPlaneCnt = Exynos_GetPlaneFromPort(pExynosOutputPort);

        if (pOutbufOps->Set_Geometry(pH264Enc->hMFCH264Handle.hMFCHandle, &bufferConf) != VIDEO_ERROR_NONE) {
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
    int SetupBufferNumber = 0;
    if (pExynosOutputPort->bufferProcessType & BUFFER_COPY)
        SetupBufferNumber = MFC_OUTPUT_BUFFER_NUM_MAX;
    else
        SetupBufferNumber = pExynosOutputPort->portDefinition.nBufferCountActual;
    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SetupBufferNumber:%d", SetupBufferNumber);

    if (pOutbufOps->Setup(pH264Enc->hMFCH264Handle.hMFCHandle, SetupBufferNumber) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to setup output buffer");
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    if (pExynosOutputPort->bufferProcessType & BUFFER_COPY) {
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
                                Exynos_GetPlaneFromPort(pExynosOutputPort),
                                NULL);
        }

        if (pOutbufOps->Run(hMFCHandle) != VIDEO_ERROR_NONE) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "Failed to run output buffer");
            ret = OMX_ErrorInsufficientResources;
            goto EXIT;
        }
    } else if (pExynosOutputPort->bufferProcessType & BUFFER_SHARE) {
        /* Register output buffer */
        /*************/
        /*    TBD    */
        /*************/
    }
    pH264Enc->hMFCH264Handle.bConfiguredMFCDst = OMX_TRUE;

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_GetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     pComponentParameterStructure)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = NULL;
    EXYNOS_H264ENC_HANDLE           *pH264Enc           = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
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
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoEnc   = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc    = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] nParamIndex %x", pExynosComponent, __FUNCTION__, (int)nParamIndex);
    switch ((int)nParamIndex) {
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = NULL;

        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pDstAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSrcAVCComponent = &pH264Enc->AVCComponent[pDstAVCComponent->nPortIndex];

        Exynos_OSAL_Memcpy(((char *)pDstAVCComponent) + nOffset,
                           ((char *)pSrcAVCComponent) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_AVCTYPE) - nOffset);
    }
        break;
    case OMX_IndexParamVideoSliceFMO:
    {
        OMX_VIDEO_PARAM_AVCSLICEFMO *pDstSliceFmo = (OMX_VIDEO_PARAM_AVCSLICEFMO *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCSLICEFMO *pSrcSliceFmo = NULL;

        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pDstSliceFmo, sizeof(OMX_VIDEO_PARAM_AVCSLICEFMO));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pSrcSliceFmo = &pH264Enc->AVCSliceFmo;

        Exynos_OSAL_Memcpy(((char *)pDstSliceFmo) + nOffset,
                           ((char *)pSrcSliceFmo) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_AVCSLICEFMO) - nOffset);
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;
        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        Exynos_OSAL_Strcpy((char *)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_H264_ENC_ROLE);
    }
        break;
    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        ret = GetIndexToProfileLevel(pExynosComponent, pDstProfileLevel);
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pDstProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE*)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSrcAVCComponent = &pH264Enc->AVCComponent[pDstProfileLevel->nPortIndex];

        pDstProfileLevel->eProfile = pSrcAVCComponent->eProfile;
        pDstProfileLevel->eLevel = pSrcAVCComponent->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pDstErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSrcErrorCorrectionType = &pH264Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
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

        pQpRange->qpRangeI.nMinQP = pH264Enc->qpRangeI.nMinQP;
        pQpRange->qpRangeI.nMaxQP = pH264Enc->qpRangeI.nMaxQP;
        pQpRange->qpRangeP.nMinQP = pH264Enc->qpRangeP.nMinQP;
        pQpRange->qpRangeP.nMaxQP = pH264Enc->qpRangeP.nMaxQP;
        pQpRange->qpRangeB.nMinQP = pH264Enc->qpRangeB.nMinQP;
        pQpRange->qpRangeB.nMaxQP = pH264Enc->qpRangeB.nMaxQP;
    }
        break;
    case OMX_IndexParamVideoAVCEnableTemporalSVC:
    {
        EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC *pDstEnableTemporalSVC = (EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC *)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pDstEnableTemporalSVC, sizeof(EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pDstEnableTemporalSVC->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pDstEnableTemporalSVC->bEnableTemporalSVC = pH264Enc->hMFCH264Handle.bTemporalSVC;
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

        pDstEnableRoiInfo->bEnableRoiInfo = pH264Enc->hMFCH264Handle.bRoiInfo;
    }
        break;

    default:
#ifdef USE_SKYPE_HD
        if (pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.bSkypeSupport == VIDEO_TRUE)
            ret = Exynos_H264Enc_GetParameter_SkypeHD(hComponent, nParamIndex, pComponentParameterStructure);
        else
            ret = OMX_ErrorUnsupportedIndex;

        if (OMX_ErrorNone != ret)
#endif
            ret = Exynos_OMX_VideoEncodeGetParameter(hComponent, nParamIndex, pComponentParameterStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_SetParameter(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = NULL;
    EXYNOS_H264ENC_HANDLE           *pH264Enc           = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentParameterStructure == NULL) {
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
    if (pExynosComponent->currentState == OMX_StateInvalid ) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoEnc   = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc    = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] nIndex %x", pExynosComponent, __FUNCTION__, (int)nIndex);
    switch ((int)nIndex) {
    case OMX_IndexParamVideoAvc:
    {
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;
        OMX_VIDEO_PARAM_AVCTYPE *pSrcAVCComponent = (OMX_VIDEO_PARAM_AVCTYPE *)pComponentParameterStructure;

        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pSrcAVCComponent, sizeof(OMX_VIDEO_PARAM_AVCTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcAVCComponent->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pDstAVCComponent = &pH264Enc->AVCComponent[pSrcAVCComponent->nPortIndex];

        Exynos_OSAL_Memcpy(((char *)pDstAVCComponent) + nOffset,
                           ((char *)pSrcAVCComponent) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_AVCTYPE) - nOffset);

        if (pDstAVCComponent->nBFrames > 2) {  /* 0 ~ 2 */
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "[%p][%s] nBFrames(%d) is over a maximum value(2). it is limited to max",
                                                    pExynosComponent, __FUNCTION__, pDstAVCComponent->nBFrames);
            pDstAVCComponent->nBFrames = 2;
        }

        if (pDstAVCComponent->nRefFrames > 2) {  /* 1 ~ 2 */
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "[%p][%s] nRefFrames(%d) is over a maximum value(2). it is limited to max",
                                                    pExynosComponent, __FUNCTION__, pDstAVCComponent->nRefFrames);
            pDstAVCComponent->nRefFrames = 2;
        } else if (pDstAVCComponent->nRefFrames == 0) {
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "[%p][%s] nRefFrames(%d) is smaller than minimum value(1). it is limited to min",
                                                    pExynosComponent, __FUNCTION__, pDstAVCComponent->nRefFrames);
            pDstAVCComponent->nRefFrames = 1;
        }
    }
        break;
    case OMX_IndexParamVideoSliceFMO:
    {
        OMX_VIDEO_PARAM_AVCSLICEFMO *pSrcSliceFmo = (OMX_VIDEO_PARAM_AVCSLICEFMO *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCSLICEFMO *pDstSliceFmo = NULL;

        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pSrcSliceFmo, sizeof(OMX_VIDEO_PARAM_AVCSLICEFMO));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        pDstSliceFmo = &pH264Enc->AVCSliceFmo;

        Exynos_OSAL_Memcpy(((char *)pDstSliceFmo) + nOffset,
                           ((char *)pSrcSliceFmo) + nOffset,
                           sizeof(OMX_VIDEO_PARAM_AVCSLICEFMO) - nOffset);
    }
        break;
    case OMX_IndexParamStandardComponentRole:
    {
        OMX_PARAM_COMPONENTROLETYPE *pComponentRole = (OMX_PARAM_COMPONENTROLETYPE*)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion(pComponentRole, sizeof(OMX_PARAM_COMPONENTROLETYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pExynosComponent->currentState != OMX_StateLoaded) && (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            ret = OMX_ErrorIncorrectStateOperation;
            goto EXIT;
        }

        if (!Exynos_OSAL_Strcmp((char*)pComponentRole->cRole, EXYNOS_OMX_COMPONENT_H264_ENC_ROLE)) {
            pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
        } else {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }
    }
        break;
    case OMX_IndexParamVideoProfileLevelCurrent:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *pSrcProfileLevel = (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_AVCTYPE *pDstAVCComponent = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcProfileLevel, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pSrcProfileLevel->nPortIndex >= ALL_PORT_NUM) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pDstAVCComponent = &pH264Enc->AVCComponent[pSrcProfileLevel->nPortIndex];
        if (OMX_FALSE == CheckProfileLevelSupport(pExynosComponent, pSrcProfileLevel)) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        pDstAVCComponent->eProfile = pSrcProfileLevel->eProfile;
        pDstAVCComponent->eLevel = pSrcProfileLevel->eLevel;
    }
        break;
    case OMX_IndexParamVideoErrorCorrection:
    {
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pSrcErrorCorrectionType = (OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *)pComponentParameterStructure;
        OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE *pDstErrorCorrectionType = NULL;

        ret = Exynos_OMX_Check_SizeVersion(pSrcErrorCorrectionType, sizeof(OMX_VIDEO_PARAM_ERRORCORRECTIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcErrorCorrectionType->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pDstErrorCorrectionType = &pH264Enc->errorCorrectionType[OUTPUT_PORT_INDEX];

        pDstErrorCorrectionType->bEnableHEC = pSrcErrorCorrectionType->bEnableHEC;
        pDstErrorCorrectionType->bEnableResync = pSrcErrorCorrectionType->bEnableResync;
        pDstErrorCorrectionType->nResynchMarkerSpacing = pSrcErrorCorrectionType->nResynchMarkerSpacing;
        pDstErrorCorrectionType->bEnableDataPartitioning = pSrcErrorCorrectionType->bEnableDataPartitioning;
        pDstErrorCorrectionType->bEnableRVLC = pSrcErrorCorrectionType->bEnableRVLC;
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

        pH264Enc->qpRangeI.nMinQP = pQpRange->qpRangeI.nMinQP;
        pH264Enc->qpRangeI.nMaxQP = pQpRange->qpRangeI.nMaxQP;
        pH264Enc->qpRangeP.nMinQP = pQpRange->qpRangeP.nMinQP;
        pH264Enc->qpRangeP.nMaxQP = pQpRange->qpRangeP.nMaxQP;
        pH264Enc->qpRangeB.nMinQP = pQpRange->qpRangeB.nMinQP;
        pH264Enc->qpRangeB.nMaxQP = pQpRange->qpRangeB.nMaxQP;
    }
        break;
    case OMX_IndexParamPrependSPSPPSToIDR:
    {
#ifdef USE_ANDROID
        ret = Exynos_OSAL_SetPrependSPSPPSToIDR(pComponentParameterStructure, &(pH264Enc->hMFCH264Handle.bPrependSpsPpsToIdr));
#else
        ret = OMX_ErrorNotImplemented;
#endif
    }
        break;
    case OMX_IndexParamVideoAVCEnableTemporalSVC:
    {
        EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC   *pSrcEnableTemporalSVC  = (EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC *)pComponentParameterStructure;
        OMX_PARAM_PORTDEFINITIONTYPE                *pPortDef               = &(pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition);
        int i;

        ret = Exynos_OMX_Check_SizeVersion(pSrcEnableTemporalSVC, sizeof(EXYNOS_OMX_VIDEO_PARAM_ENABLE_TEMPORALSVC));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcEnableTemporalSVC->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if ((pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.bTemporalSvcSupport == VIDEO_FALSE) &&
            (pSrcEnableTemporalSVC->bEnableTemporalSVC == OMX_TRUE)) {
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "%s: MFC D/D doesn't support Temporal SVC", __func__);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        pH264Enc->hMFCH264Handle.bTemporalSVC = pSrcEnableTemporalSVC->bEnableTemporalSVC;
        if ((pH264Enc->hMFCH264Handle.bTemporalSVC == OMX_TRUE) &&
            (pH264Enc->TemporalSVC.nTemporalLayerCount == 0)) {  /* not initialized yet */
            pH264Enc->TemporalSVC.nTemporalLayerCount           = 1;
            pH264Enc->TemporalSVC.nTemporalLayerBitrateRatio[0] = pPortDef->format.video.nBitrate;
        } else if (pH264Enc->hMFCH264Handle.bTemporalSVC == OMX_FALSE) {  /* set default value */
            pH264Enc->TemporalSVC.nTemporalLayerCount = 0;
            for (i = 0; i < OMX_VIDEO_ANDROID_MAXAVCTEMPORALLAYERS; i++)
                pH264Enc->TemporalSVC.nTemporalLayerBitrateRatio[i] = pPortDef->format.video.nBitrate;
        }
    }
        break;
    case OMX_IndexParamVideoEnableRoiInfo:
    {
        EXYNOS_OMX_VIDEO_PARAM_ENABLE_ROIINFO   *pSrcEnableRoiInfo  = (EXYNOS_OMX_VIDEO_PARAM_ENABLE_ROIINFO *)pComponentParameterStructure;
        int i;

        ret = Exynos_OMX_Check_SizeVersion(pSrcEnableRoiInfo, sizeof(EXYNOS_OMX_VIDEO_PARAM_ENABLE_ROIINFO));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if (pSrcEnableRoiInfo->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if ((pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.bRoiInfoSupport == VIDEO_FALSE) &&
            (pSrcEnableRoiInfo->bEnableRoiInfo == OMX_TRUE)) {
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "%s: MFC D/D doesn't support Roi Info", __func__);
            ret = OMX_ErrorUndefined;
            goto EXIT;
        }

        pH264Enc->hMFCH264Handle.bRoiInfo = pSrcEnableRoiInfo->bEnableRoiInfo;
    }
        break;
    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE    *pPortDef       = (OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;
        OMX_U32                          nPortIndex     = pPortDef->nPortIndex;
        EXYNOS_OMX_BASEPORT             *pExynosPort    = &pExynosComponent->pExynosPort[nPortIndex];;
        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        if (nPortIndex >= pExynosComponent->portParam.nPorts) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        ret = Exynos_OMX_Check_SizeVersion(pPortDef, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
        if (ret != OMX_ErrorNone) {
            goto EXIT;
        }

        if ((pExynosComponent->currentState != OMX_StateLoaded) &&
            (pExynosComponent->currentState != OMX_StateWaitForResources)) {
            if (pExynosPort->portDefinition.bEnabled == OMX_TRUE) {
                ret = OMX_ErrorIncorrectStateOperation;
                goto EXIT;
            }
        }

        if (pPortDef->nBufferCountActual < pExynosPort->portDefinition.nBufferCountMin) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        if ((nPortIndex == INPUT_PORT_INDEX) &&
            ((pPortDef->format.video.xFramerate >> 16) <= 0)) {
            Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s[%x] xFramerate is invalid(%d)",
                                              __FUNCTION__, OMX_IndexParamPortDefinition, pPortDef->format.video.xFramerate >> 16);
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        Exynos_OSAL_Memcpy(((char *)&pExynosPort->portDefinition) + nOffset,
                           ((char *)pPortDef) + nOffset,
                           pPortDef->nSize - nOffset);
        if (nPortIndex == INPUT_PORT_INDEX) {
            pExynosPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
            Exynos_UpdateFrameSize(pOMXComponent);

            if (pVideoEnc->bFirstInput == OMX_FALSE)
                pVideoEnc->bEncDRC = OMX_TRUE;

            pVideoEnc->bFirstInput = OMX_TRUE;
            pH264Enc->hMFCH264Handle.bConfiguredMFCSrc = OMX_FALSE;

            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pExynosOutputPort->portDefinition.nBufferSize: %d", pExynosPort->portDefinition.nBufferSize);
        } else if (nPortIndex == OUTPUT_PORT_INDEX) {
            pH264Enc->hMFCH264Handle.bConfiguredMFCDst = OMX_FALSE;
        }

        ret = OMX_ErrorNone;
    }
        break;

    default:
#ifdef USE_SKYPE_HD
        if (pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.bSkypeSupport == VIDEO_TRUE)
            ret = Exynos_H264Enc_SetParameter_SkypeHD(hComponent, nIndex, pComponentParameterStructure);
        else
            ret = OMX_ErrorUnsupportedIndex;

        if (OMX_ErrorNone != ret)
#endif
            ret = Exynos_OMX_VideoEncodeSetParameter(hComponent, nIndex, pComponentParameterStructure);
        break;
    }
EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_GetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = NULL;
    EXYNOS_H264ENC_HANDLE           *pH264Enc           = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
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
    pH264Enc  = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] nIndex %x", pExynosComponent, __FUNCTION__, (int)nIndex);
    switch ((int)nIndex) {
    case OMX_IndexConfigVideoAVCIntraPeriod:
    {
        OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pAVCIntraPeriod = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pComponentConfigStructure;
        OMX_U32           portIndex = pAVCIntraPeriod->nPortIndex;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            pAVCIntraPeriod->nIDRPeriod = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
            pAVCIntraPeriod->nPFrames = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames;
        }
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

        pQpRange->qpRangeI.nMinQP = pH264Enc->qpRangeI.nMinQP;
        pQpRange->qpRangeI.nMaxQP = pH264Enc->qpRangeI.nMaxQP;
        pQpRange->qpRangeP.nMinQP = pH264Enc->qpRangeP.nMinQP;
        pQpRange->qpRangeP.nMaxQP = pH264Enc->qpRangeP.nMaxQP;
        pQpRange->qpRangeB.nMinQP = pH264Enc->qpRangeB.nMinQP;
        pQpRange->qpRangeB.nMaxQP = pH264Enc->qpRangeB.nMaxQP;
    }
        break;
    case OMX_IndexConfigVideoTemporalSVC:
    {
        EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *pDstTemporalSVC = (EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *)pComponentConfigStructure;
        EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *pSrcTemporalSVC = &pH264Enc->TemporalSVC;

        /* except nSize, nVersion and nPortIndex */
        int nOffset = sizeof(OMX_U32) + sizeof(OMX_VERSIONTYPE) + sizeof(OMX_U32);

        ret = Exynos_OMX_Check_SizeVersion(pDstTemporalSVC, sizeof(EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pDstTemporalSVC->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pSrcTemporalSVC->nKeyFrameInterval = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;

        pSrcTemporalSVC->nMinQuantizer = pH264Enc->qpRangeI.nMinQP;
        pSrcTemporalSVC->nMaxQuantizer = pH264Enc->qpRangeI.nMaxQP;

        Exynos_OSAL_Memcpy(((char *)pDstTemporalSVC) + nOffset,
                           ((char *)pSrcTemporalSVC) + nOffset,
                           sizeof(EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC) - nOffset);
    }
        break;
    default:
#ifdef USE_SKYPE_HD
        if (pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.bSkypeSupport == VIDEO_TRUE)
            ret = Exynos_H264Enc_GetConfig_SkypeHD(hComponent, nIndex, pComponentConfigStructure);
        else
            ret = OMX_ErrorUnsupportedIndex;

        if (OMX_ErrorNone != ret)
#endif
            ret = Exynos_OMX_VideoEncodeGetConfig(hComponent, nIndex, pComponentConfigStructure);
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_SetConfig(
    OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex,
    OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc         = NULL;

    FunctionIn();

    if (hComponent == NULL || pComponentConfigStructure == NULL) {
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
    pH264Enc  = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] nIndex %x", pExynosComponent, __FUNCTION__, (int)nIndex);
    switch ((int)nIndex) {
    case OMX_IndexConfigVideoIntraPeriod:
    {
        EXYNOS_OMX_VIDEOENC_COMPONENT *pVEncBase = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
        OMX_U32 nPFrames = (*((OMX_U32 *)pComponentConfigStructure)) - 1;

        pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames = nPFrames;

        ret = OMX_ErrorNone;
    }
        break;
    case OMX_IndexConfigVideoAVCIntraPeriod:
    {
        OMX_VIDEO_CONFIG_AVCINTRAPERIOD *pAVCIntraPeriod = (OMX_VIDEO_CONFIG_AVCINTRAPERIOD *)pComponentConfigStructure;
        OMX_U32           portIndex = pAVCIntraPeriod->nPortIndex;

        if ((portIndex != OUTPUT_PORT_INDEX)) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        } else {
            if (pAVCIntraPeriod->nIDRPeriod == (pAVCIntraPeriod->nPFrames + 1))
                pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames = pAVCIntraPeriod->nPFrames;
            else {
                ret = OMX_ErrorBadParameter;
                goto EXIT;
            }
        }
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

        pH264Enc->qpRangeI.nMinQP = pQpRange->qpRangeI.nMinQP;
        pH264Enc->qpRangeI.nMaxQP = pQpRange->qpRangeI.nMaxQP;
        pH264Enc->qpRangeP.nMinQP = pQpRange->qpRangeP.nMinQP;
        pH264Enc->qpRangeP.nMaxQP = pQpRange->qpRangeP.nMaxQP;
        pH264Enc->qpRangeB.nMinQP = pQpRange->qpRangeB.nMinQP;
        pH264Enc->qpRangeB.nMaxQP = pQpRange->qpRangeB.nMaxQP;
    }
        break;
    case OMX_IndexConfigVideoTemporalSVC:
    {
        EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC *pDstTemporalSVC = &pH264Enc->TemporalSVC;
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

        if ((pH264Enc->hMFCH264Handle.bTemporalSVC == OMX_FALSE) ||
            (pSrcTemporalSVC->nTemporalLayerCount > OMX_VIDEO_ANDROID_MAXAVCTEMPORALLAYERS)) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames = pSrcTemporalSVC->nKeyFrameInterval - 1;

        pH264Enc->qpRangeI.nMinQP = pSrcTemporalSVC->nMinQuantizer;
        pH264Enc->qpRangeI.nMaxQP = pSrcTemporalSVC->nMaxQuantizer;
        pH264Enc->qpRangeP.nMinQP = pSrcTemporalSVC->nMinQuantizer;
        pH264Enc->qpRangeP.nMaxQP = pSrcTemporalSVC->nMaxQuantizer;
        pH264Enc->qpRangeB.nMinQP = pSrcTemporalSVC->nMinQuantizer;
        pH264Enc->qpRangeB.nMaxQP = pSrcTemporalSVC->nMaxQuantizer;

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

        if ((pH264Enc->hMFCH264Handle.bRoiInfo == OMX_FALSE) ||
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
#ifdef USE_SKYPE_HD
        if (pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.bSkypeSupport == VIDEO_TRUE)
            ret = Exynos_H264Enc_SetConfig_SkypeHD(hComponent, nIndex, pComponentConfigStructure);
        else
            ret = OMX_ErrorUnsupportedIndex;

        if (OMX_ErrorNone != ret)
#endif
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

OMX_ERRORTYPE Exynos_H264Enc_GetExtensionIndex(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pOMXComponent      = NULL;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = NULL;
    EXYNOS_H264ENC_HANDLE           *pH264Enc           = NULL;

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
    if ((cParameterName == NULL) || (pIndexType == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc  = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;

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

    if (Exynos_OSAL_Strcmp(cParameterName, EXYNOS_INDEX_PARAM_VIDEO_AVC_ENABLE_TEMPORALSVC) == 0) {
        *pIndexType = OMX_IndexParamVideoAVCEnableTemporalSVC;
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

#ifdef USE_SKYPE_HD
    if (pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.bSkypeSupport == VIDEO_TRUE)
        ret = Exynos_H264Enc_GetExtensionIndex_SkypeHD(hComponent, cParameterName, pIndexType);
    else
        ret = OMX_ErrorUnsupportedIndex;

    if (ret != OMX_ErrorNone)
#endif
        ret = Exynos_OMX_VideoEncodeGetExtensionIndex(hComponent, cParameterName, pIndexType);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_ComponentRoleEnum(OMX_HANDLETYPE hComponent, OMX_U8 *cRole, OMX_U32 nIndex)
{
    OMX_ERRORTYPE               ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (cRole == NULL)) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }
    if (nIndex == (MAX_COMPONENT_ROLE_NUM-1)) {
        Exynos_OSAL_Strcpy((char *)cRole, EXYNOS_OMX_COMPONENT_H264_ENC_ROLE);
        ret = OMX_ErrorNone;
    } else {
        ret = OMX_ErrorNoMore;
    }

EXIT:
    FunctionOut();

    return ret;
}

/* MFC Init */
OMX_ERRORTYPE Exynos_H264Enc_Init(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_OMX_BASEPORT             *pExynosInputPort   = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT             *pExynosOutputPort  = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_H264ENC_HANDLE           *pH264Enc           = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;;
    EXYNOS_MFC_H264ENC_HANDLE       *pMFCH264Handle     = &pH264Enc->hMFCH264Handle;
    OMX_PTR                          hMFCHandle         = NULL;
    OMX_COLOR_FORMATTYPE             eColorFormat       = OMX_COLOR_FormatUnused;

    ExynosVideoEncOps       *pEncOps        = NULL;
    ExynosVideoEncBufferOps *pInbufOps      = NULL;
    ExynosVideoEncBufferOps *pOutbufOps     = NULL;
    ExynosVideoInstInfo     *pVideoInstInfo = &(pH264Enc->hMFCH264Handle.videoInstInfo);

    CSC_METHOD csc_method = CSC_METHOD_SW;
    int i = 0;

    FunctionIn();

    pH264Enc->hMFCH264Handle.bConfiguredMFCSrc = OMX_FALSE;
    pH264Enc->hMFCH264Handle.bConfiguredMFCDst = OMX_FALSE;
    pVideoEnc->bFirstInput  = OMX_TRUE;
    pVideoEnc->bFirstOutput = OMX_FALSE;
    pExynosComponent->bUseFlagEOF = OMX_TRUE;
    pExynosComponent->bSaveFlagEOS = OMX_FALSE;
    pExynosComponent->bBehaviorEOS = OMX_FALSE;

    eColorFormat = pExynosInputPort->portDefinition.format.video.eColorFormat;
#ifdef USE_METADATABUFFERTYPE
    if (pExynosInputPort->bStoreMetaData == OMX_TRUE) {
#ifdef USE_ANDROIDOPAQUE
        if (eColorFormat == (OMX_COLOR_FORMATTYPE)OMX_COLOR_FormatAndroidOpaque) {
            pExynosInputPort->bufferProcessType = BUFFER_COPY;
        } else {
            pExynosInputPort->bufferProcessType = BUFFER_SHARE;
        }
#else
        pExynosInputPort->bufferProcessType = BUFFER_SHARE;
#endif
    } else {
        pExynosInputPort->bufferProcessType = BUFFER_COPY;
    }
#else
    pExynosInputPort->bufferProcessType = BUFFER_COPY;
#endif

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] CodecOpen W: %d H:%d  Bitrate:%d FPS:%d", pExynosComponent, __FUNCTION__,
                                                                                              pExynosInputPort->portDefinition.format.video.nFrameWidth,
                                                                                              pExynosInputPort->portDefinition.format.video.nFrameHeight,
                                                                                              pExynosInputPort->portDefinition.format.video.nBitrate,
                                                                                              pExynosInputPort->portDefinition.format.video.xFramerate);
    pVideoInstInfo->nSize        = sizeof(ExynosVideoInstInfo);
    pVideoInstInfo->nWidth       = pExynosInputPort->portDefinition.format.video.nFrameWidth;
    pVideoInstInfo->nHeight      = pExynosInputPort->portDefinition.format.video.nFrameHeight;
    pVideoInstInfo->nBitrate     = pExynosInputPort->portDefinition.format.video.nBitrate;
    pVideoInstInfo->xFramerate   = pExynosInputPort->portDefinition.format.video.xFramerate;

    /* H.264 Codec Open */
    ret = H264CodecOpen(pH264Enc, pVideoInstInfo);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;
    hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;

    Exynos_SetPlaneToPort(pExynosInputPort, MFC_DEFAULT_INPUT_BUFFER_PLANE);
    Exynos_SetPlaneToPort(pExynosOutputPort, MFC_DEFAULT_OUTPUT_BUFFER_PLANE);

    Exynos_OSAL_SemaphoreCreate(&pExynosInputPort->codecSemID);
    Exynos_OSAL_QueueCreate(&pExynosInputPort->codecBufferQ, MAX_QUEUE_ELEMENTS);

    if (pExynosOutputPort->bufferProcessType & BUFFER_COPY) {
        Exynos_OSAL_SemaphoreCreate(&pExynosOutputPort->codecSemID);
        Exynos_OSAL_QueueCreate(&pExynosOutputPort->codecBufferQ, MAX_QUEUE_ELEMENTS);
    } else if (pExynosOutputPort->bufferProcessType & BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    pH264Enc->bSourceStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pH264Enc->hSourceStartEvent);
    pH264Enc->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalCreate(&pH264Enc->hDestinationStartEvent);

    Exynos_OSAL_Memset(pExynosComponent->bTimestampSlotUsed, 0, sizeof(OMX_BOOL) * MAX_TIMESTAMP);
    INIT_ARRAY_TO_VAL(pExynosComponent->timeStamp, DEFAULT_TIMESTAMP_VAL, MAX_TIMESTAMP);
    Exynos_OSAL_Memset(pExynosComponent->nFlags, 0, sizeof(OMX_U32) * MAX_FLAGS);
    pH264Enc->hMFCH264Handle.indexTimestamp = 0;
    pH264Enc->hMFCH264Handle.outputIndexTimestamp = 0;

    pExynosComponent->getAllDelayBuffer = OMX_FALSE;

    Exynos_OSAL_QueueCreate(&pH264Enc->bypassBufferInfoQ, QUEUE_ELEMENTS);

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
OMX_ERRORTYPE Exynos_H264Enc_Terminate(OMX_COMPONENTTYPE *pOMXComponent)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle);
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT      *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    EXYNOS_H264ENC_HANDLE    *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    OMX_PTR                hMFCHandle = pH264Enc->hMFCH264Handle.hMFCHandle;

    ExynosVideoEncOps       *pEncOps    = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps  = pH264Enc->hMFCH264Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps = pH264Enc->hMFCH264Handle.pOutbufOps;

    int i = 0, plane = 0;

    FunctionIn();

    if (pVideoEnc->csc_handle != NULL) {
        csc_deinit(pVideoEnc->csc_handle);
        pVideoEnc->csc_handle = NULL;
    }

    Exynos_OSAL_QueueTerminate(&pH264Enc->bypassBufferInfoQ);

    Exynos_OSAL_SignalTerminate(pH264Enc->hDestinationStartEvent);
    pH264Enc->hDestinationStartEvent = NULL;
    pH264Enc->bDestinationStart = OMX_FALSE;
    Exynos_OSAL_SignalTerminate(pH264Enc->hSourceStartEvent);
    pH264Enc->hSourceStartEvent = NULL;
    pH264Enc->bSourceStart = OMX_FALSE;

    if (pExynosOutputPort->bufferProcessType & BUFFER_COPY) {
        Exynos_Free_CodecBuffers(pOMXComponent, OUTPUT_PORT_INDEX);
        Exynos_OSAL_QueueTerminate(&pExynosOutputPort->codecBufferQ);
        Exynos_OSAL_SemaphoreTerminate(pExynosOutputPort->codecSemID);
    } else if (pExynosOutputPort->bufferProcessType & BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    if (pExynosInputPort->bufferProcessType & BUFFER_COPY) {
        Exynos_Free_CodecBuffers(pOMXComponent, INPUT_PORT_INDEX);
    } else if (pExynosInputPort->bufferProcessType & BUFFER_SHARE) {
        /*************/
        /*    TBD    */
        /*************/
        /* Does not require any actions. */
    }

    Exynos_OSAL_QueueTerminate(&pExynosInputPort->codecBufferQ);
    Exynos_OSAL_SemaphoreTerminate(pExynosInputPort->codecSemID);

    H264CodecClose(pH264Enc);

    Exynos_ResetAllPortConfig(pOMXComponent);

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_SrcIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc          = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    void                          *hMFCHandle        = pH264Enc->hMFCH264Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosInputPort  = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    OMX_U32                        oneFrameSize      = pSrcInputData->dataLen;
    OMX_COLOR_FORMATTYPE           inputColorFormat  = OMX_COLOR_FormatUnused;

    ExynosVideoEncOps       *pEncOps     = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps   = pH264Enc->hMFCH264Handle.pInbufOps;
    ExynosVideoEncBufferOps *pOutbufOps  = pH264Enc->hMFCH264Handle.pOutbufOps;
    ExynosVideoErrorType     codecReturn = VIDEO_ERROR_NONE;

    OMX_BUFFERHEADERTYPE tempBufferHeader;
    void *pPrivate = NULL;

    unsigned int nAllocLen[MAX_BUFFER_PLANE]  = {0, 0, 0};
    unsigned int nDataLen[MAX_BUFFER_PLANE]   = {0, 0, 0};
    int i, nPlaneCnt;

    FunctionIn();

    if (pH264Enc->hMFCH264Handle.bConfiguredMFCSrc == OMX_FALSE) {
        ret = H264CodecSrcSetup(pOMXComponent, pSrcInputData);
        if ((ret != OMX_ErrorNone) ||
            ((pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {
            goto EXIT;
        }
    }

    if (pH264Enc->hMFCH264Handle.bConfiguredMFCDst == OMX_FALSE) {
        ret = H264CodecDstSetup(pOMXComponent);
        if (ret != OMX_ErrorNone)
            goto EXIT;
    }

    while (Exynos_OSAL_GetElemNum(&pExynosComponent->dynamicConfigQ) > 0) {
        Change_H264Enc_Param(pExynosComponent);
    }

    if ((pSrcInputData->dataLen > 0) ||
        ((pSrcInputData->nFlags & OMX_BUFFERFLAG_EOS) == OMX_BUFFERFLAG_EOS)) {


        pExynosComponent->timeStamp[pH264Enc->hMFCH264Handle.indexTimestamp]            = pSrcInputData->timeStamp;
        pExynosComponent->bTimestampSlotUsed[pH264Enc->hMFCH264Handle.indexTimestamp]   = OMX_TRUE;
        pExynosComponent->nFlags[pH264Enc->hMFCH264Handle.indexTimestamp]               = pSrcInputData->nFlags;
        Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] input timestamp %lld us (%.2f secs), Tag: %d, nFlags: 0x%x", pExynosComponent, __FUNCTION__,
                                                        pSrcInputData->timeStamp, pSrcInputData->timeStamp / 1E6, pH264Enc->hMFCH264Handle.indexTimestamp, pSrcInputData->nFlags);
        pEncOps->Set_FrameTag(hMFCHandle, pH264Enc->hMFCH264Handle.indexTimestamp);
        pH264Enc->hMFCH264Handle.indexTimestamp++;
        pH264Enc->hMFCH264Handle.indexTimestamp %= MAX_TIMESTAMP;

#ifdef PERFORMANCE_DEBUG
        Exynos_OSAL_V4L2CountIncrease(pExynosInputPort->hBufferCount, pSrcInputData->bufferHeader, INPUT_PORT_INDEX);
#endif

        /* queue work for input buffer */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "[%p][%s] oneFrameSize: %ld, bufferHeader: 0x%p", pExynosComponent, __FUNCTION__, oneFrameSize, pSrcInputData->bufferHeader);

        inputColorFormat = Exynos_Input_GetActualColorFormat(pExynosComponent);

        if ((pVideoEnc->eRotationType == ROTATE_0) ||
            (pVideoEnc->eRotationType == ROTATE_180)) {
            Exynos_OSAL_GetPlaneSize(inputColorFormat,
                                     pExynosInputPort->ePlaneType,
                                     pExynosInputPort->portDefinition.format.video.nFrameWidth,
                                     pExynosInputPort->portDefinition.format.video.nFrameHeight,
                                     nDataLen,
                                     nAllocLen);
        } else {
            Exynos_OSAL_GetPlaneSize(inputColorFormat,
                                     pExynosInputPort->ePlaneType,
                                     pExynosInputPort->portDefinition.format.video.nFrameHeight,
                                     pExynosInputPort->portDefinition.format.video.nFrameWidth,
                                     nDataLen,
                                     nAllocLen);
        }

        if (pExynosInputPort->bufferProcessType == BUFFER_COPY) {
            tempBufferHeader.nFlags     = pSrcInputData->nFlags;
            tempBufferHeader.nTimeStamp = pSrcInputData->timeStamp;
            pPrivate = (void *)&tempBufferHeader;
        } else {
            pPrivate = (void *)pSrcInputData->bufferHeader;
        }

        nPlaneCnt = Exynos_GetPlaneFromPort(pExynosInputPort);
        if (pVideoEnc->nInbufSpareSize > 0) {
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
        H264CodecStart(pOMXComponent, INPUT_PORT_INDEX);
        if (pH264Enc->bSourceStart == OMX_FALSE) {
            pH264Enc->bSourceStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pH264Enc->hSourceStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
        if (pH264Enc->bDestinationStart == OMX_FALSE) {
            pH264Enc->bDestinationStart = OMX_TRUE;
            Exynos_OSAL_SignalSet(pH264Enc->hDestinationStartEvent);
            Exynos_OSAL_SleepMillisec(0);
        }
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_SrcOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE           *pH264Enc           = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    void                            *hMFCHandle         = pH264Enc->hMFCH264Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT             *pExynosInputPort   = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    ExynosVideoEncOps       *pEncOps        = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pInbufOps      = pH264Enc->hMFCH264Handle.pInbufOps;
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
        nPlaneCnt = Exynos_GetPlaneFromPort(pExynosInputPort);
        for (plane = 0; plane < nPlaneCnt; plane++) {
            pSrcOutputData->multiPlaneBuffer.dataBuffer[plane] = pVideoBuffer->planes[plane].addr;
            pSrcOutputData->multiPlaneBuffer.fd[plane] = pVideoBuffer->planes[plane].fd;

            pSrcOutputData->allocSize += pVideoBuffer->planes[plane].allocSize;
        }

        if (pExynosInputPort->bufferProcessType & BUFFER_COPY) {
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
        if (pExynosInputPort->bufferProcessType == BUFFER_SHARE)
            pSrcOutputData->bufferHeader = (OMX_BUFFERHEADERTYPE*)pVideoBuffer->pPrivate;

#ifdef PERFORMANCE_DEBUG
        Exynos_OSAL_V4L2CountDecrease(pExynosInputPort->hBufferCount, pSrcOutputData->bufferHeader, INPUT_PORT_INDEX);
#endif
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_DstIn(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE                  ret                  = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent     = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc            = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc             = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    void                          *hMFCHandle           = pH264Enc->hMFCH264Handle.hMFCHandle;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort    = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    ExynosVideoEncOps       *pEncOps     = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps *pOutbufOps  = pH264Enc->hMFCH264Handle.pOutbufOps;
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
    Exynos_OSAL_V4L2CountIncrease(pExynosOutputPort->hBufferCount, pDstInputData->bufferHeader, OUTPUT_PORT_INDEX);
#endif

    nAllocLen[0] = pExynosOutputPort->portDefinition.nBufferSize;
    if (pExynosOutputPort->bStoreMetaData == OMX_TRUE)
        nAllocLen[0] = ALIGN(pExynosOutputPort->portDefinition.format.video.nFrameWidth * pExynosOutputPort->portDefinition.format.video.nFrameHeight * 3 / 2, 512);

    codecReturn = pOutbufOps->ExtensionEnqueue(hMFCHandle,
                                (void **)pDstInputData->multiPlaneBuffer.dataBuffer,
                                (int *)pDstInputData->multiPlaneBuffer.fd,
                                nAllocLen,
                                nDataLen,
                                Exynos_GetPlaneFromPort(pExynosOutputPort),
                                pDstInputData->bufferHeader);
    if (codecReturn != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - pOutbufOps->Enqueue", __FUNCTION__, __LINE__);
        ret = (OMX_ERRORTYPE)OMX_ErrorCodecEncode;
        goto EXIT;
    }
    H264CodecStart(pOMXComponent, OUTPUT_PORT_INDEX);

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_DstOut(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent  = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE         *pH264Enc          = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_OMX_BASEPORT           *pExynosOutputPort = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];
    void                          *hMFCHandle        = pH264Enc->hMFCH264Handle.hMFCHandle;

    ExynosVideoEncOps          *pEncOps        = pH264Enc->hMFCH264Handle.pEncOps;
    ExynosVideoEncBufferOps    *pOutbufOps     = pH264Enc->hMFCH264Handle.pOutbufOps;
    ExynosVideoBuffer          *pVideoBuffer   = NULL;
    ExynosVideoBuffer           videoBuffer;
    ExynosVideoFrameStatusType  displayStatus  = VIDEO_FRAME_STATUS_UNKNOWN;
    ExynosVideoErrorType        codecReturn    = VIDEO_ERROR_NONE;

    OMX_S32 indexTimestamp = 0;

    FunctionIn();

    if (pH264Enc->bDestinationStart == OMX_FALSE) {
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

    pH264Enc->hMFCH264Handle.outputIndexTimestamp++;
    pH264Enc->hMFCH264Handle.outputIndexTimestamp %= MAX_TIMESTAMP;

    pDstOutputData->multiPlaneBuffer.dataBuffer[0] = pVideoBuffer->planes[0].addr;
    pDstOutputData->multiPlaneBuffer.fd[0] = pVideoBuffer->planes[0].fd;
    pDstOutputData->allocSize   = pVideoBuffer->planes[0].allocSize;
    pDstOutputData->dataLen     = pVideoBuffer->planes[0].dataSize;
    pDstOutputData->remainDataLen = pVideoBuffer->planes[0].dataSize;
    pDstOutputData->usedDataLen = 0;
    pDstOutputData->pPrivate = pVideoBuffer;
    if (pExynosOutputPort->bufferProcessType & BUFFER_COPY) {
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
        if (pExynosComponent->codecType != HW_VIDEO_ENC_SECURE_CODEC) {
            OMX_U8 *p = NULL;
            OMX_U32 iSpsSize = 0;
            OMX_U32 iPpsSize = 0;

            /* start header return */
            /* Calculate sps/pps size if needed */
            p = FindDelimiter((OMX_U8 *)((char *)pDstOutputData->multiPlaneBuffer.dataBuffer[0] + 4),
                                pDstOutputData->dataLen - 4);

            iSpsSize = PTR_TO_INT((p - (OMX_U8 *)pDstOutputData->multiPlaneBuffer.dataBuffer[0]));
            pH264Enc->hMFCH264Handle.headerData.pHeaderSPS =
                (OMX_PTR)pDstOutputData->multiPlaneBuffer.dataBuffer[0];
            pH264Enc->hMFCH264Handle.headerData.SPSLen = iSpsSize;

            iPpsSize = pDstOutputData->dataLen - iSpsSize;
            pH264Enc->hMFCH264Handle.headerData.pHeaderPPS =
                (OMX_U8 *)pDstOutputData->multiPlaneBuffer.dataBuffer[0] + iSpsSize;
            pH264Enc->hMFCH264Handle.headerData.PPSLen = iPpsSize;
        }

        pDstOutputData->timeStamp = 0;
        pDstOutputData->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
        pDstOutputData->nFlags |= OMX_BUFFERFLAG_ENDOFFRAME;
        pVideoEnc->bFirstOutput = OMX_TRUE;
    } else {
        indexTimestamp = pEncOps->Get_FrameTag(pH264Enc->hMFCH264Handle.hMFCHandle);

        if ((indexTimestamp < 0) || (indexTimestamp >= MAX_TIMESTAMP)) {
            Exynos_OSAL_Log(EXYNOS_LOG_WARNING, "[%p][%s] Tag(%d) is invalid. changes to use outputIndexTimestamp(%d)",
                                                  pExynosComponent, __FUNCTION__,
                                                  indexTimestamp, pH264Enc->hMFCH264Handle.outputIndexTimestamp);
            indexTimestamp = pH264Enc->hMFCH264Handle.outputIndexTimestamp;
        }

        if (pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nBFrames > 0) {
            if ((pExynosComponent->nFlags[indexTimestamp] & OMX_BUFFERFLAG_EOS) &&
                (pVideoBuffer->frameType == VIDEO_FRAME_P)) {
                /* move an EOS flag to previous slot
                 * B1 B2 P(EOS) -> P B1 B2(EOS)
                 * B1 P(EOS) -> P B1(EOS)
                 */
                int index = ((indexTimestamp - 1) < 0)? (MAX_TIMESTAMP - 1):(indexTimestamp - 1);

                if (pExynosComponent->bTimestampSlotUsed[index] == OMX_TRUE) {
                    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] EOS flag is moved to %d from %d",
                                                            pExynosComponent, __FUNCTION__,
                                                            index, indexTimestamp);
                    pExynosComponent->nFlags[indexTimestamp] &= (~OMX_BUFFERFLAG_EOS);
                    pExynosComponent->nFlags[index] |= OMX_BUFFERFLAG_EOS;
                }
            }
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
                                            pDstOutputData->timeStamp, pDstOutputData->timeStamp / 1E6, indexTimestamp,
                                            pVideoBuffer->frameType, pDstOutputData->dataLen);

#ifdef PERFORMANCE_DEBUG
    if (pDstOutputData->bufferHeader != NULL) {
        pDstOutputData->bufferHeader->nTimeStamp = pDstOutputData->timeStamp;
        Exynos_OSAL_V4L2CountDecrease(pExynosOutputPort->hBufferCount, pDstOutputData->bufferHeader, OUTPUT_PORT_INDEX);
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

OMX_ERRORTYPE Exynos_H264Enc_srcInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcInputData)
{
    OMX_ERRORTYPE             ret = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_H264ENC_HANDLE    *pH264Enc = (EXYNOS_H264ENC_HANDLE *)((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle;
    EXYNOS_OMX_BASEPORT      *pExynosInputPort = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosInputPort)) || (!CHECK_PORT_POPULATED(pExynosInputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    ret = Exynos_H264Enc_SrcIn(pOMXComponent, pSrcInputData);
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

OMX_ERRORTYPE Exynos_H264Enc_srcOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pSrcOutputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE           *pH264Enc           = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_OMX_BASEPORT             *pExynosInputPort   = &pExynosComponent->pExynosPort[INPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosInputPort)) || (!CHECK_PORT_POPULATED(pExynosInputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pExynosInputPort->bufferProcessType & BUFFER_COPY) {
        if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, INPUT_PORT_INDEX)) {
            ret = OMX_ErrorNone;
            goto EXIT;
        }
    }
    if ((pH264Enc->bSourceStart == OMX_FALSE) &&
       (!CHECK_PORT_BEING_FLUSHED(pExynosInputPort))) {
        Exynos_OSAL_SignalWait(pH264Enc->hSourceStartEvent, DEF_MAX_WAIT_TIME);
        if (pVideoEnc->bExitBufferProcessThread)
            goto EXIT;

        Exynos_OSAL_SignalReset(pH264Enc->hSourceStartEvent);
    }

    ret = Exynos_H264Enc_SrcOut(pOMXComponent, pSrcOutputData);
    if ((ret != OMX_ErrorNone) && (pExynosComponent->currentState == OMX_StateExecuting)) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "%s: %d: Failed - SrcOut -> event is thrown to client", __FUNCTION__, __LINE__);
        pExynosComponent->pCallbacks->EventHandler((OMX_HANDLETYPE)pOMXComponent,
                                                pExynosComponent->callbackData,
                                                OMX_EventError, ret, 0, NULL);
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_dstInputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstInputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE           *pH264Enc           = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_OMX_BASEPORT             *pExynosOutputPort  = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosOutputPort)) || (!CHECK_PORT_POPULATED(pExynosOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (pExynosOutputPort->bufferProcessType & BUFFER_SHARE) {
        if ((pH264Enc->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pH264Enc->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            if (pVideoEnc->bExitBufferProcessThread)
                goto EXIT;

            Exynos_OSAL_SignalReset(pH264Enc->hDestinationStartEvent);
        }

        if (Exynos_OSAL_GetElemNum(&pH264Enc->bypassBufferInfoQ) > 0) {
            Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] buffer with EOS will be returned by bypassBufferInfoQ",
                                                    pExynosComponent, __FUNCTION__);

            BYPASS_BUFFER_INFO *pBufferInfo = (BYPASS_BUFFER_INFO *)Exynos_OSAL_Dequeue(&pH264Enc->bypassBufferInfoQ);
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
    if (pH264Enc->hMFCH264Handle.bConfiguredMFCDst == OMX_TRUE) {
        ret = Exynos_H264Enc_DstIn(pOMXComponent, pDstInputData);
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

OMX_ERRORTYPE Exynos_H264Enc_dstOutputBufferProcess(OMX_COMPONENTTYPE *pOMXComponent, EXYNOS_OMX_DATA *pDstOutputData)
{
    OMX_ERRORTYPE                    ret                = OMX_ErrorNone;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent   = (EXYNOS_OMX_BASECOMPONENT *)pOMXComponent->pComponentPrivate;
    EXYNOS_OMX_VIDEOENC_COMPONENT   *pVideoEnc          = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    EXYNOS_H264ENC_HANDLE           *pH264Enc           = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    EXYNOS_OMX_BASEPORT             *pExynosOutputPort  = &pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX];

    FunctionIn();

    if ((!CHECK_PORT_ENABLED(pExynosOutputPort)) || (!CHECK_PORT_POPULATED(pExynosOutputPort))) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }
    if (OMX_FALSE == Exynos_Check_BufferProcess_State(pExynosComponent, OUTPUT_PORT_INDEX)) {
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (pExynosOutputPort->bufferProcessType & BUFFER_COPY) {
        if ((pH264Enc->bDestinationStart == OMX_FALSE) &&
           (!CHECK_PORT_BEING_FLUSHED(pExynosOutputPort))) {
            Exynos_OSAL_SignalWait(pH264Enc->hDestinationStartEvent, DEF_MAX_WAIT_TIME);
            if (pVideoEnc->bExitBufferProcessThread)
                goto EXIT;

            Exynos_OSAL_SignalReset(pH264Enc->hDestinationStartEvent);
        }

        if (Exynos_OSAL_GetElemNum(&pH264Enc->bypassBufferInfoQ) > 0) {
            EXYNOS_OMX_DATABUFFER *dstOutputUseBuffer   = &pExynosOutputPort->way.port2WayDataBuffer.outputDataBuffer;
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

            pBufferInfo = Exynos_OSAL_Dequeue(&pH264Enc->bypassBufferInfoQ);
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
    ret = Exynos_H264Enc_DstOut(pOMXComponent, pDstOutputData);
    if ((ret != OMX_ErrorNone) && (pExynosComponent->currentState == OMX_StateExecuting)) {
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
    EXYNOS_H264ENC_HANDLE         *pH264Enc         = NULL;
    OMX_BOOL                       bSecureMode      = OMX_FALSE;
    int i = 0;

    Exynos_OSAL_Get_Log_Property(); // For debuging
    FunctionIn();

    if ((hComponent == NULL) || (componentName == NULL)) {
        ret = OMX_ErrorBadParameter;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorBadParameter, Line:%d", __LINE__);
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_H264_ENC, componentName) == 0) {
        bSecureMode = OMX_FALSE;
    } else if (Exynos_OSAL_Strcmp(EXYNOS_OMX_COMPONENT_H264_DRM_ENC, componentName) == 0) {
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

    pH264Enc = Exynos_OSAL_Malloc(sizeof(EXYNOS_H264ENC_HANDLE));
    if (pH264Enc == NULL) {
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "OMX_ErrorInsufficientResources, Line:%d", __LINE__);
        goto EXIT;
    }
    Exynos_OSAL_Memset(pH264Enc, 0, sizeof(EXYNOS_H264ENC_HANDLE));
    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pVideoEnc->hCodecHandle = (OMX_HANDLETYPE)pH264Enc;
    pH264Enc->qpRangeI.nMinQP = 5;
    pH264Enc->qpRangeI.nMaxQP = 50;
    pH264Enc->qpRangeP.nMinQP = 5;
    pH264Enc->qpRangeP.nMaxQP = 50;
    pH264Enc->qpRangeB.nMinQP = 5;
    pH264Enc->qpRangeB.nMaxQP = 50;

    pVideoEnc->quantization.nQpI = 29;
    pVideoEnc->quantization.nQpP = 30;
    pVideoEnc->quantization.nQpB = 32;

    if (pExynosComponent->codecType == HW_VIDEO_ENC_SECURE_CODEC)
        Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_H264_DRM_ENC);
    else
        Exynos_OSAL_Strcpy(pExynosComponent->componentName, EXYNOS_OMX_COMPONENT_H264_ENC);

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
    pExynosPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
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
    pExynosPort->portDefinition.format.video.nStride = 0; /*DEFAULT_FRAME_WIDTH;*/
    pExynosPort->portDefinition.nBufferSize = DEFAULT_VIDEO_OUTPUT_BUFFER_SIZE;
    pExynosPort->portDefinition.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    Exynos_OSAL_Memset(pExynosPort->portDefinition.format.video.cMIMEType, 0, MAX_OMX_MIMETYPE_SIZE);
    Exynos_OSAL_Strcpy(pExynosPort->portDefinition.format.video.cMIMEType, "video/avc");
    pExynosPort->portDefinition.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    pExynosPort->portDefinition.bEnabled = OMX_TRUE;
    pExynosPort->bufferProcessType = BUFFER_SHARE;
    pExynosPort->portWayType = WAY2_PORT;
    pExynosPort->ePlaneType = PLANE_SINGLE;

    for(i = 0; i < ALL_PORT_NUM; i++) {
        INIT_SET_SIZE_VERSION(&pH264Enc->AVCComponent[i], OMX_VIDEO_PARAM_AVCTYPE);
        pH264Enc->AVCComponent[i].nPortIndex = i;
        pH264Enc->AVCComponent[i].eProfile   = OMX_VIDEO_AVCProfileBaseline;
        pH264Enc->AVCComponent[i].eLevel     = OMX_VIDEO_AVCLevel31;

        pH264Enc->AVCComponent[i].nPFrames      = 29;
        pH264Enc->AVCComponent[i].nBFrames      = 0;
        pH264Enc->AVCComponent[i].nRefFrames    = 1;
    }

    Exynos_OSAL_Memset(&pH264Enc->TemporalSVC, 0, sizeof(EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC));
    INIT_SET_SIZE_VERSION(&pH264Enc->TemporalSVC, EXYNOS_OMX_VIDEO_CONFIG_TEMPORALSVC);
    pH264Enc->TemporalSVC.nKeyFrameInterval = pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].nPFrames + 1;
    pH264Enc->TemporalSVC.nTemporalLayerCount   = 0;
    pH264Enc->hMFCH264Handle.bTemporalSVC       = OMX_FALSE;
    for (i = 0; i < OMX_VIDEO_ANDROID_MAXAVCTEMPORALLAYERS; i++) {
        pH264Enc->TemporalSVC.nTemporalLayerBitrateRatio[i] =
                                pExynosComponent->pExynosPort[OUTPUT_PORT_INDEX].portDefinition.format.video.nBitrate;
    }
    pH264Enc->TemporalSVC.nMinQuantizer = pH264Enc->qpRangeI.nMinQP;
    pH264Enc->TemporalSVC.nMaxQuantizer = pH264Enc->qpRangeI.nMaxQP;

    pOMXComponent->GetParameter      = &Exynos_H264Enc_GetParameter;
    pOMXComponent->SetParameter      = &Exynos_H264Enc_SetParameter;
    pOMXComponent->GetConfig         = &Exynos_H264Enc_GetConfig;
    pOMXComponent->SetConfig         = &Exynos_H264Enc_SetConfig;
    pOMXComponent->GetExtensionIndex = &Exynos_H264Enc_GetExtensionIndex;
    pOMXComponent->ComponentRoleEnum = &Exynos_H264Enc_ComponentRoleEnum;
    pOMXComponent->ComponentDeInit   = &Exynos_OMX_ComponentDeinit;

    pExynosComponent->exynos_codec_componentInit      = &Exynos_H264Enc_Init;
    pExynosComponent->exynos_codec_componentTerminate = &Exynos_H264Enc_Terminate;

    pVideoEnc->exynos_codec_srcInputProcess  = &Exynos_H264Enc_srcInputBufferProcess;
    pVideoEnc->exynos_codec_srcOutputProcess = &Exynos_H264Enc_srcOutputBufferProcess;
    pVideoEnc->exynos_codec_dstInputProcess  = &Exynos_H264Enc_dstInputBufferProcess;
    pVideoEnc->exynos_codec_dstOutputProcess = &Exynos_H264Enc_dstOutputBufferProcess;

    pVideoEnc->exynos_codec_start         = &H264CodecStart;
    pVideoEnc->exynos_codec_stop          = &H264CodecStop;
    pVideoEnc->exynos_codec_bufferProcessRun = &H264CodecOutputBufferProcessRun;
    pVideoEnc->exynos_codec_enqueueAllBuffer = &H264CodecEnqueueAllBuffer;

#if 0  /* unused code */
    pVideoEnc->exynos_checkInputFrame        = NULL;
    pVideoEnc->exynos_codec_getCodecInputPrivateData  = &GetCodecInputPrivateData;
#endif

    pVideoEnc->exynos_codec_getCodecOutputPrivateData = &GetCodecOutputPrivateData;

    pVideoEnc->exynos_codec_checkFormatSupport = &CheckFormatHWSupport;

    pVideoEnc->hSharedMemory = Exynos_OSAL_SharedMemory_Open();
    if (pVideoEnc->hSharedMemory == NULL) {
        Exynos_OSAL_Free(pH264Enc);
        pH264Enc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    pH264Enc->hMFCH264Handle.videoInstInfo.eCodecType = VIDEO_CODING_AVC;
    if (pExynosComponent->codecType == HW_VIDEO_ENC_SECURE_CODEC)
        pH264Enc->hMFCH264Handle.videoInstInfo.eSecurityType = VIDEO_SECURE;
    else
        pH264Enc->hMFCH264Handle.videoInstInfo.eSecurityType = VIDEO_NORMAL;

    if (Exynos_Video_GetInstInfo(&(pH264Enc->hMFCH264Handle.videoInstInfo), VIDEO_FALSE /* enc */) != VIDEO_ERROR_NONE) {
        Exynos_OSAL_Log(EXYNOS_LOG_ERROR, "[%p][%s]: Exynos_Video_GetInstInfo is failed", pExynosComponent, __FUNCTION__);
        Exynos_OSAL_Free(pH264Enc);
        pH264Enc = ((EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle)->hCodecHandle = NULL;
        Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
        ret = OMX_ErrorInsufficientResources;
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_ESSENTIAL, "[%p][%s] GetInstInfo for enc %d %d %d %d %d %d", pExynosComponent, __FUNCTION__,
            (pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.bRGBSupport),
            (pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.nSpareSize),
            (pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.bTemporalSvcSupport),
            (pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.bSkypeSupport),
            (pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.bRoiInfoSupport),
            (pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.bQpRangePBSupport));

    if (pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.nSpareSize > 0)
        pVideoEnc->nInbufSpareSize = pH264Enc->hMFCH264Handle.videoInstInfo.specificInfo.enc.nSpareSize;

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
    OMX_ERRORTYPE            ret = OMX_ErrorNone;
    OMX_COMPONENTTYPE          *pOMXComponent = NULL;
    EXYNOS_OMX_BASECOMPONENT   *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc = NULL;
    EXYNOS_H264ENC_HANDLE      *pH264Enc         = NULL;

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

    pH264Enc = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    if (pH264Enc != NULL) {
        Exynos_OSAL_Free(pH264Enc);
        pH264Enc = pVideoEnc->hCodecHandle = NULL;
    }

    ret = Exynos_OMX_VideoEncodeComponentDeinit(pOMXComponent);
    if (ret != OMX_ErrorNone) {
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    FunctionOut();

    return ret;
}
