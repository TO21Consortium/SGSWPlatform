/*
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
 * @file        Exynos_OSAL_SkypeHD.cpp
 * @brief
 * @author      Seungbeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2015.04.27 : Create
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Exynos_OSAL_Mutex.h"
#include "Exynos_OSAL_Semaphore.h"
#include "Exynos_OMX_Baseport.h"
#include "Exynos_OMX_Basecomponent.h"
#include "Exynos_OMX_Macros.h"
#include "Exynos_OSAL_SkypeHD.h"
#include "Exynos_OSAL_ETC.h"
#include "Exynos_OMX_Def.h"
#include "exynos_format.h"

#include "ExynosVideoApi.h"

#include "OMX_Video_Extensions.h"

#undef  EXYNOS_LOG_TAG
#define EXYNOS_LOG_TAG    "Exynos_OSAL_SkypeHD"
//#define EXYNOS_LOG_OFF
#include "Exynos_OSAL_Log.h"

#include "Exynos_OSAL_SkypeHD.h"

/* ENCODE_ONLY */
#ifdef BUILD_ENC
#include "Exynos_OMX_Venc.h"
#include "Exynos_OMX_H264enc.h"
#endif

/* DECODE_ONLY */
#ifdef BUILD_DEC
#include "Exynos_OMX_Vdec.h"
#include "Exynos_OMX_H264dec.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Change CHECK_SIZE_VERSION Macro for SkypeHD */
OMX_ERRORTYPE Exynos_OMX_Check_SizeVersion_SkypeHD(OMX_PTR header, OMX_U32 size)
{
    OMX_ERRORTYPE    ret        = OMX_ErrorNone;
    OMX_VERSIONTYPE *version    = NULL;

    if (header == NULL) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    version = (OMX_VERSIONTYPE*)((char*)header + sizeof(OMX_U32));
    if (*((OMX_U32*)header) != size) {
        ret = OMX_ErrorBadParameter;
        goto EXIT;
    }

    Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "nVersionMajor:%d, nVersionMinor:%d", version->s.nVersionMajor, version->s.nVersionMinor);
    if (version->s.nVersionMajor != OMX_VIDEO_MajorVersion ||
        version->s.nVersionMinor > OMX_VIDEO_MinorVersion) {
        ret = OMX_ErrorVersionMismatch;
        goto EXIT;
    }

    ret = OMX_ErrorNone;

EXIT:
    return ret;
}



/* ENCODE_ONLY */
#ifdef BUILD_ENC
/* video enc */
OMX_ERRORTYPE Exynos_H264Enc_GetExtensionIndex_SkypeHD(
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

    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, OMX_MS_SKYPE_PARAM_DRIVERVER) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexSkypeParamDriverVersion;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, OMX_MS_SKYPE_PARAM_ENCODERSETTING) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexSkypeParamEncoderSetting;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, OMX_MS_SKYPE_PARAM_ENCODERCAP) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexSkypeParamEncoderCapability;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, OMX_MS_SKYPE_CONFIG_MARKLTRFRAME) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexSkypeConfigMarkLTRFrame;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, OMX_MS_SKYPE_CONFIG_USELTRFRAME) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexSkypeConfigUseLTRFrame;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, OMX_MS_SKYPE_CONFIG_QP) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexSkypeConfigQP;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, OMX_MS_SKYPE_CONFIG_TEMPORALLAYERCOUNT) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexSkypeConfigTemporalLayerCount;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, OMX_MS_SKYPE_CONFIG_BASELAYERPID) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexSkypeConfigBasePid;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    ret = OMX_ErrorUnsupportedIndex;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_GetParameter_SkypeHD(
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

    if ((hComponent == NULL) || (pComponentParameterStructure == NULL)) {
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
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc  = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;

    switch ((int)nParamIndex) {
    case OMX_IndexSkypeParamDriverVersion:
    {
        OMX_VIDEO_PARAM_DRIVERVER       *pDriverVer     = (OMX_VIDEO_PARAM_DRIVERVER *)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pDriverVer, sizeof(OMX_VIDEO_PARAM_DRIVERVER));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pDriverVer->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pDriverVer->nDriverVersion = (OMX_U64)(pH264Enc->hMFCH264Handle.videoInstInfo.SwVersion);
    }
        break;
    case OMX_IndexSkypeParamEncoderCapability:
    {
        OMX_VIDEO_PARAM_ENCODERCAP      *pEncoderCap    = (OMX_VIDEO_PARAM_ENCODERCAP *)pComponentParameterStructure;
        OMX_VIDEO_ENCODERCAP            *pstEncCap      = NULL;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pEncoderCap, sizeof(OMX_VIDEO_PARAM_ENCODERCAP));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pEncoderCap->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pstEncCap = &(pEncoderCap->stEncCap);

        pstEncCap->bLowLatency            = OMX_TRUE;
        pstEncCap->nMaxFrameWidth         = MAX_FRAME_WIDTH;
        pstEncCap->nMaxFrameHeight        = MAX_FRAME_HEIGHT;
        pstEncCap->nMaxInstances          = RESOURCE_VIDEO_ENC;
        pstEncCap->nMaxTemporaLayerCount  = OMX_VIDEO_MAX_TEMPORAL_LAYERS;
        pstEncCap->nMaxRefFrames          = ((OMX_VIDEO_MAX_TEMPORAL_LAYERS_WITH_LTR + 1) / 2) + OMX_VIDEO_MAX_LTR_FRAMES;
        pstEncCap->nMaxLTRFrames          = OMX_VIDEO_MAX_LTR_FRAMES;
        pstEncCap->nMaxLevel              = OMX_VIDEO_AVCLevel42;
        pstEncCap->nSliceControlModesBM   = (1 << (OMX_VIDEO_SliceControlModeMB - 1)) | (1 << (OMX_VIDEO_SliceControlModeByte - 1));
        pstEncCap->nMaxMacroblockProcessingRate = ENC_BLOCKS_PER_SECOND;
        pstEncCap->nResize                = 0;
        pstEncCap->xMinScaleFactor        = 0;
    }
        break;
    case OMX_IndexSkypeParamEncoderSetting:
    {
        OMX_VIDEO_PARAM_ENCODERSETTING  *pEncoderSetting    = (OMX_VIDEO_PARAM_ENCODERSETTING *)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pEncoderSetting, sizeof(OMX_VIDEO_PARAM_ENCODERSETTING));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pEncoderSetting->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        Exynos_OSAL_Memcpy(&(pEncoderSetting->stEncParam), &(pH264Enc->stEncParam), sizeof(OMX_VIDEO_ENCODERPARAMS));
    }
        break;
    default:
        ret = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_SetParameter_SkypeHD(
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

    if ((hComponent == NULL) || (pComponentParameterStructure == NULL)) {
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
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc  = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;

    switch ((int)nIndex) {
    case OMX_IndexSkypeParamEncoderSetting:
    {
        OMX_VIDEO_PARAM_ENCODERSETTING  *pEncoderSetting    = (OMX_VIDEO_PARAM_ENCODERSETTING *)pComponentParameterStructure;
        OMX_VIDEO_ENCODERPARAMS         *pstEncParam        = NULL;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pEncoderSetting, sizeof(OMX_VIDEO_PARAM_ENCODERSETTING));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pEncoderSetting->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pstEncParam = &(pEncoderSetting->stEncParam);

        pH264Enc->bLowLatency = pstEncParam->bLowLatency;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pstEncParam->bLowLatency: %s", (pstEncParam->bLowLatency == OMX_TRUE) ? "OMX_TRUE" : "OMX_FALSE");

        /* SetPrependSPSPPSToIDR */
        pH264Enc->hMFCH264Handle.bPrependSpsPpsToIdr = pstEncParam->bSequenceHeaderWithIDR;

        if (pstEncParam->bUseExtendedProfile == OMX_TRUE) {
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pstEncParam->eProfile: 0x%x", pstEncParam->eProfile);
            switch (pstEncParam->eProfile) {
            case OMX_VIDEO_EXT_AVCProfileConstrainedBaseline:
                pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eProfile = OMX_VIDEO_AVCProfileConstrainedBaseline;
                break;
            case OMX_VIDEO_EXT_AVCProfileConstrainedHigh:
                pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eProfile = OMX_VIDEO_AVCProfileConstrainedHigh;
                break;
            default:
                Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "default eProfile: 0x%x", pH264Enc->AVCComponent[OUTPUT_PORT_INDEX].eProfile);
                break;
            }
        }

        switch (pstEncParam->eHierarType) {
        case OMX_VIDEO_HierarType_P:
            pH264Enc->eHierarchicalType = EXYNOS_OMX_Hierarchical_P;
            break;
        case OMX_VIDEO_HierarType_B:
            pH264Enc->eHierarchicalType = EXYNOS_OMX_Hierarchical_B;
            break;
        default:
            Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "default eHierarType: 0x%x", pH264Enc->eHierarchicalType);
            break;
        }

        if (pstEncParam->nMaxTemporalLayerCount > OMX_VIDEO_ANDROID_MAXAVCTEMPORALLAYERS) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        } else {
            if (pstEncParam->nMaxTemporalLayerCount > 0) {
                pH264Enc->hMFCH264Handle.bTemporalSVC = OMX_TRUE;
                pH264Enc->TemporalSVC.nTemporalLayerCount = pstEncParam->nMaxTemporalLayerCount | GENERAL_TSVC_ENABLE;
            } else {
                pH264Enc->hMFCH264Handle.bTemporalSVC = OMX_FALSE;
            }
        }

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pstEncParam->nLTRFrames: %d", pstEncParam->nLTRFrames);
        if (((int)(pstEncParam->nLTRFrames) > ((int)OMX_VIDEO_MAX_LTR_FRAMES)) ||
            (((int)pstEncParam->nLTRFrames) < 0)) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        } else {
            if (pstEncParam->nLTRFrames > 0)
                pH264Enc->hMFCH264Handle.bLTREnable = OMX_TRUE;
            else
                pH264Enc->hMFCH264Handle.bLTREnable = OMX_FALSE;
        }

        switch (pstEncParam->eSliceControlMode) {
            case OMX_VIDEO_SliceControlModeMB:
                pH264Enc->AVCSliceFmo.eSliceMode = OMX_VIDEO_SLICEMODE_AVCMBSlice;
                break;
            case OMX_VIDEO_SliceControlModeByte:
                pH264Enc->AVCSliceFmo.eSliceMode = OMX_VIDEO_SLICEMODE_AVCByteSlice;
                break;
            case OMX_VIDEO_SliceControlModeNone:
            case OMX_VIDEO_SliceControlModMBRow:
            default:
                pH264Enc->AVCSliceFmo.eSliceMode = OMX_VIDEO_SLICEMODE_AVCDefault;
                break;
        }

        if (pstEncParam->nSarIndex > 0)
            pH264Enc->stSarParam.SarEnable = OMX_TRUE;

        pH264Enc->stSarParam.SarIndex  = pstEncParam->nSarIndex;
        pH264Enc->stSarParam.SarWidth  = pstEncParam->nSarWidth;
        pH264Enc->stSarParam.SarHeight = pstEncParam->nSarHeight;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "SarIndex:%d, SarWidth:%d, SarHeight:%d",
                            pstEncParam->nSarIndex, pstEncParam->nSarWidth, pstEncParam->nSarHeight);

        Exynos_OSAL_Memcpy(&(pH264Enc->stEncParam), &(pEncoderSetting->stEncParam), sizeof(OMX_VIDEO_ENCODERPARAMS));
    }
        break;
    default:
        ret = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_GetConfig_SkypeHD(
    OMX_HANDLETYPE  hComponent,
    OMX_INDEXTYPE   nIndex,
    OMX_PTR         pComponentConfigStructure)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc         = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (pComponentConfigStructure == NULL)) {
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
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc  = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;

    switch ((int)nIndex) {
    case OMX_IndexSkypeConfigTemporalLayerCount:
    {
        OMX_VIDEO_CONFIG_TEMPORALLAYERCOUNT *pTemporalLayerCount = (OMX_VIDEO_CONFIG_TEMPORALLAYERCOUNT *)pComponentConfigStructure;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pTemporalLayerCount, sizeof(OMX_VIDEO_CONFIG_TEMPORALLAYERCOUNT));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pTemporalLayerCount->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pTemporalLayerCount->nTemporalLayerCount = pH264Enc->TemporalSVC.nTemporalLayerCount & (~GENERAL_TSVC_ENABLE);
    }
        break;
    case OMX_IndexSkypeConfigBasePid:
    {
        OMX_VIDEO_CONFIG_BASELAYERPID *pBaseLayerPid = (OMX_VIDEO_CONFIG_BASELAYERPID *)pComponentConfigStructure;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pBaseLayerPid, sizeof(OMX_VIDEO_CONFIG_BASELAYERPID));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pBaseLayerPid->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pBaseLayerPid->nPID = pH264Enc->nBaseLayerPid;
    }
        break;
    default:
        ret = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Enc_SetConfig_SkypeHD(
    OMX_HANDLETYPE  hComponent,
    OMX_INDEXTYPE   nIndex,
    OMX_PTR         pComponentConfigStructure)
{
    OMX_ERRORTYPE                  ret              = OMX_ErrorNone;
    OMX_COMPONENTTYPE             *pOMXComponent    = NULL;
    EXYNOS_OMX_BASECOMPONENT      *pExynosComponent = NULL;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc        = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc         = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (pComponentConfigStructure == NULL)) {
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
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoEnc = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc  = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;

    switch ((int)nIndex) {
    case OMX_IndexSkypeConfigMarkLTRFrame:
    {
        OMX_VIDEO_CONFIG_MARKLTRFRAME   *pMarkLTRFrame  = (OMX_VIDEO_CONFIG_MARKLTRFRAME *)pComponentConfigStructure;
        EXYNOS_MFC_H264ENC_HANDLE       *pMFCH264Handle = NULL;
        ExynosVideoEncParam             *pEncParam      = NULL;
        ExynosVideoEncCommonParam       *pCommonParam   = NULL;
        ExynosVideoEncH264Param         *pH264Param     = NULL;
        ExynosVideoEncOps               *pEncOps        = NULL;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pMarkLTRFrame, sizeof(OMX_VIDEO_CONFIG_MARKLTRFRAME));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pMarkLTRFrame->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMFCH264Handle  = &pH264Enc->hMFCH264Handle;
        pEncParam       = &pMFCH264Handle->encParam;
        pCommonParam    = &pEncParam->commonParam;
        pH264Param      = &pEncParam->codecParam.h264;
        pEncOps         = pMFCH264Handle->pEncOps;

        /* TBD */
    }
        break;
    case OMX_IndexSkypeConfigUseLTRFrame:
    {
        OMX_VIDEO_CONFIG_USELTRFRAME    *pUseLTRFrame   = (OMX_VIDEO_CONFIG_USELTRFRAME *)pComponentConfigStructure;
        EXYNOS_MFC_H264ENC_HANDLE       *pMFCH264Handle = NULL;
        ExynosVideoEncParam             *pEncParam      = NULL;
        ExynosVideoEncCommonParam       *pCommonParam   = NULL;
        ExynosVideoEncH264Param         *pH264Param     = NULL;
        ExynosVideoEncOps               *pEncOps        = NULL;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pUseLTRFrame, sizeof(OMX_VIDEO_CONFIG_USELTRFRAME));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pUseLTRFrame->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMFCH264Handle  = &pH264Enc->hMFCH264Handle;
        pEncParam       = &pMFCH264Handle->encParam;
        pCommonParam    = &pEncParam->commonParam;
        pH264Param      = &pEncParam->codecParam.h264;
        pEncOps         = pMFCH264Handle->pEncOps;

        /* TBD */
    }
        break;
    case OMX_IndexSkypeConfigQP:
    {
        OMX_VIDEO_CONFIG_QP             *pConfigQp      = (OMX_VIDEO_CONFIG_QP *)pComponentConfigStructure;
        EXYNOS_MFC_H264ENC_HANDLE       *pMFCH264Handle = NULL;
        ExynosVideoEncParam             *pEncParam      = NULL;
        ExynosVideoEncCommonParam       *pCommonParam   = NULL;
        ExynosVideoEncH264Param         *pH264Param     = NULL;
        ExynosVideoEncOps               *pEncOps        = NULL;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pConfigQp, sizeof(OMX_VIDEO_CONFIG_QP));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pConfigQp->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMFCH264Handle  = &pH264Enc->hMFCH264Handle;
        pEncParam       = &pMFCH264Handle->encParam;
        pCommonParam    = &pEncParam->commonParam;
        pH264Param      = &pEncParam->codecParam.h264;
        pEncOps         = pMFCH264Handle->pEncOps;

        /* will remove */
        pVideoEnc->quantization.nQpI = pConfigQp->nQP;
        pVideoEnc->quantization.nQpP = pConfigQp->nQP;
        pVideoEnc->quantization.nQpB = pConfigQp->nQP;
//        pCommonParam->FrameQp      = pVideoEnc->quantization.nQpI;
//        pCommonParam->FrameQp_P    = pVideoEnc->quantization.nQpP;
//        pH264Param->FrameQp_B      = pVideoEnc->quantization.nQpB;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "%s : pConfigQp->nQP:%d", __FUNCTION__, pConfigQp->nQP);

        /* TBD */
    }
        break;
    case OMX_IndexSkypeConfigTemporalLayerCount:
    {
        OMX_VIDEO_CONFIG_TEMPORALLAYERCOUNT *pTemporalLayerCount    = (OMX_VIDEO_CONFIG_TEMPORALLAYERCOUNT *)pComponentConfigStructure;
        EXYNOS_MFC_H264ENC_HANDLE           *pMFCH264Handle         = NULL;
        ExynosVideoEncOps                   *pEncOps                = NULL;

        int i = 0;
        TemporalLayerShareBuffer TemporalSVC;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pTemporalLayerCount, sizeof(OMX_VIDEO_CONFIG_TEMPORALLAYERCOUNT));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pTemporalLayerCount->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if ((pH264Enc->hMFCH264Handle.bTemporalSVC == OMX_FALSE) ||
            (pTemporalLayerCount->nTemporalLayerCount > OMX_VIDEO_ANDROID_MAXAVCTEMPORALLAYERS)) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        /* Temporal SVC */
        pMFCH264Handle  = &pH264Enc->hMFCH264Handle;
        pEncOps         = pMFCH264Handle->pEncOps;

        pH264Enc->TemporalSVC.nTemporalLayerCount = pTemporalLayerCount->nTemporalLayerCount | GENERAL_TSVC_ENABLE;
    }
        break;
    case OMX_IndexSkypeConfigBasePid:
    {
        OMX_VIDEO_CONFIG_BASELAYERPID   *pBaseLayerPid  = (OMX_VIDEO_CONFIG_BASELAYERPID *)pComponentConfigStructure;
        EXYNOS_MFC_H264ENC_HANDLE       *pMFCH264Handle = NULL;
        ExynosVideoEncParam             *pEncParam      = NULL;
        ExynosVideoEncCommonParam       *pCommonParam   = NULL;
        ExynosVideoEncH264Param         *pH264Param     = NULL;
        ExynosVideoEncOps               *pEncOps        = NULL;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pBaseLayerPid, sizeof(OMX_VIDEO_CONFIG_BASELAYERPID));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pBaseLayerPid->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMFCH264Handle  = &pH264Enc->hMFCH264Handle;
        pEncParam       = &pMFCH264Handle->encParam;
        pCommonParam    = &pEncParam->commonParam;
        pH264Param      = &pEncParam->codecParam.h264;
        pEncOps         = pMFCH264Handle->pEncOps;

        pH264Enc->nBaseLayerPid = pBaseLayerPid->nPID;
    }
        break;
    default:
        ret = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:

    FunctionOut();

    return ret;
}

void Change_H264Enc_SkypeHDParam(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_PTR pDynamicConfigCMD)
{
    OMX_ERRORTYPE                  ret               = OMX_ErrorNone;
    EXYNOS_OMX_VIDEOENC_COMPONENT *pVideoEnc         = NULL;
    EXYNOS_H264ENC_HANDLE         *pH264Enc          = NULL;
    EXYNOS_MFC_H264ENC_HANDLE     *pMFCH264Handle    = NULL;
    OMX_PTR                        pConfigData       = NULL;
    OMX_S32                        nCmdIndex         = 0;
    ExynosVideoEncOps             *pEncOps           = NULL;
    int                            nValue            = 0;

    int i;

    FunctionIn();

    pVideoEnc       = (EXYNOS_OMX_VIDEOENC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Enc        = (EXYNOS_H264ENC_HANDLE *)pVideoEnc->hCodecHandle;
    pMFCH264Handle  = &pH264Enc->hMFCH264Handle;
    pEncOps         = pMFCH264Handle->pEncOps;

    if (pDynamicConfigCMD == NULL)
        goto EXIT;

    nCmdIndex   = *(OMX_S32 *)pDynamicConfigCMD;
    pConfigData = (OMX_PTR)((OMX_U8 *)pDynamicConfigCMD + sizeof(OMX_S32));

    switch ((int)nCmdIndex) {
    case OMX_IndexSkypeConfigMarkLTRFrame:
    {
        OMX_VIDEO_CONFIG_MARKLTRFRAME   *pMarkLTRFrame  = (OMX_VIDEO_CONFIG_MARKLTRFRAME *)pConfigData;
        EXYNOS_MFC_H264ENC_HANDLE       *pMFCH264Handle = NULL;
        ExynosVideoEncParam             *pEncParam      = NULL;
        ExynosVideoEncCommonParam       *pCommonParam   = NULL;
        ExynosVideoEncH264Param         *pH264Param     = NULL;
        ExynosVideoEncOps               *pEncOps        = NULL;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pMarkLTRFrame, sizeof(OMX_VIDEO_CONFIG_MARKLTRFRAME));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pMarkLTRFrame->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMFCH264Handle  = &pH264Enc->hMFCH264Handle;
        pEncParam       = &pMFCH264Handle->encParam;
        pCommonParam    = &pEncParam->commonParam;
        pH264Param      = &pEncParam->codecParam.h264;
        pEncOps         = pMFCH264Handle->pEncOps;

        /* TBD */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pMarkLTRFrame->nLongTermFrmIdx: %d", pMarkLTRFrame->nLongTermFrmIdx + 1);
        pEncOps->Set_MarkLTRFrame(pMFCH264Handle->hMFCHandle, pMarkLTRFrame->nLongTermFrmIdx + 1);
    }
        break;
    case OMX_IndexSkypeConfigUseLTRFrame:
    {
        OMX_VIDEO_CONFIG_USELTRFRAME    *pUseLTRFrame   = (OMX_VIDEO_CONFIG_USELTRFRAME *)pConfigData;
        EXYNOS_MFC_H264ENC_HANDLE       *pMFCH264Handle = NULL;
        ExynosVideoEncParam             *pEncParam      = NULL;
        ExynosVideoEncCommonParam       *pCommonParam   = NULL;
        ExynosVideoEncH264Param         *pH264Param     = NULL;
        ExynosVideoEncOps               *pEncOps        = NULL;

        OMX_S32 nUsedLTRFrameNum = 0;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pUseLTRFrame, sizeof(OMX_VIDEO_CONFIG_USELTRFRAME));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pUseLTRFrame->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMFCH264Handle  = &pH264Enc->hMFCH264Handle;
        pEncParam       = &pMFCH264Handle->encParam;
        pCommonParam    = &pEncParam->commonParam;
        pH264Param      = &pEncParam->codecParam.h264;
        pEncOps         = pMFCH264Handle->pEncOps;

        nUsedLTRFrameNum = pUseLTRFrame->nUsedLTRFrameBM;
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pUseLTRFrame->nUsedLTRFrameBM: %d, nUsedLTRFrameNum = %d", pUseLTRFrame->nUsedLTRFrameBM, nUsedLTRFrameNum);
        if (nUsedLTRFrameNum < 0) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        pEncOps->Set_UsedLTRFrame(pMFCH264Handle->hMFCHandle, nUsedLTRFrameNum);
    }
        break;
    case OMX_IndexSkypeConfigQP:
    {
        OMX_VIDEO_CONFIG_QP             *pConfigQp      = (OMX_VIDEO_CONFIG_QP *)pConfigData;
        EXYNOS_MFC_H264ENC_HANDLE       *pMFCH264Handle = NULL;
        ExynosVideoEncParam             *pEncParam      = NULL;
        ExynosVideoEncCommonParam       *pCommonParam   = NULL;
        ExynosVideoEncH264Param         *pH264Param     = NULL;
        ExynosVideoEncOps               *pEncOps        = NULL;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pConfigQp, sizeof(OMX_VIDEO_CONFIG_QP));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pConfigQp->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMFCH264Handle  = &pH264Enc->hMFCH264Handle;
        pEncParam       = &pMFCH264Handle->encParam;
        pCommonParam    = &pEncParam->commonParam;
        pH264Param      = &pEncParam->codecParam.h264;
        pEncOps         = pMFCH264Handle->pEncOps;

        /* will remove */
        pVideoEnc->quantization.nQpI = pConfigQp->nQP;
        pVideoEnc->quantization.nQpP = pConfigQp->nQP;
        pVideoEnc->quantization.nQpB = pConfigQp->nQP;
//        pCommonParam->FrameQp      = pVideoEnc->quantization.nQpI;
//        pCommonParam->FrameQp_P    = pVideoEnc->quantization.nQpP;
//        pH264Param->FrameQp_B      = pVideoEnc->quantization.nQpB;

        /* TBD */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pConfigQp->nQP: %d", pConfigQp->nQP);
        pEncOps->Set_DynamicQpControl(pMFCH264Handle->hMFCHandle, pConfigQp->nQP);
    }
        break;
    case OMX_IndexSkypeConfigTemporalLayerCount:
    {
        OMX_VIDEO_CONFIG_TEMPORALLAYERCOUNT *pTemporalLayerCount    = (OMX_VIDEO_CONFIG_TEMPORALLAYERCOUNT *)pConfigData;
        EXYNOS_MFC_H264ENC_HANDLE           *pMFCH264Handle         = NULL;
        ExynosVideoEncOps                   *pEncOps                = NULL;

        int i = 0;
        TemporalLayerShareBuffer TemporalSVC;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pTemporalLayerCount, sizeof(OMX_VIDEO_CONFIG_TEMPORALLAYERCOUNT));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pTemporalLayerCount->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        if ((pH264Enc->hMFCH264Handle.bTemporalSVC == OMX_FALSE) ||
            (pTemporalLayerCount->nTemporalLayerCount > OMX_VIDEO_ANDROID_MAXAVCTEMPORALLAYERS)) {
            ret = OMX_ErrorBadParameter;
            goto EXIT;
        }

        /* Temporal SVC */
        pMFCH264Handle  = &pH264Enc->hMFCH264Handle;
        pEncOps         = pMFCH264Handle->pEncOps;

        Exynos_OSAL_Memset(&TemporalSVC, 0, sizeof(TemporalLayerShareBuffer));
        TemporalSVC.nTemporalLayerCount = (unsigned int)pTemporalLayerCount->nTemporalLayerCount | GENERAL_TSVC_ENABLE;
        for (i = 0; i < OMX_VIDEO_ANDROID_MAXAVCTEMPORALLAYERS; i++)
            TemporalSVC.nTemporalLayerBitrateRatio[i] = 0;

        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "TemporalSVC.nTemporalLayerCount: %d", TemporalSVC.nTemporalLayerCount & (~GENERAL_TSVC_ENABLE));
        pEncOps->Set_LayerChange(pMFCH264Handle->hMFCHandle, TemporalSVC);
    }
        break;
    case OMX_IndexSkypeConfigBasePid:
    {
        OMX_VIDEO_CONFIG_BASELAYERPID   *pBaseLayerPid  = (OMX_VIDEO_CONFIG_BASELAYERPID *)pConfigData;
        EXYNOS_MFC_H264ENC_HANDLE       *pMFCH264Handle = NULL;
        ExynosVideoEncParam             *pEncParam      = NULL;
        ExynosVideoEncCommonParam       *pCommonParam   = NULL;
        ExynosVideoEncH264Param         *pH264Param     = NULL;
        ExynosVideoEncOps               *pEncOps        = NULL;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pBaseLayerPid, sizeof(OMX_VIDEO_CONFIG_BASELAYERPID));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pBaseLayerPid->nPortIndex != OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pMFCH264Handle  = &pH264Enc->hMFCH264Handle;
        pEncParam       = &pMFCH264Handle->encParam;
        pCommonParam    = &pEncParam->commonParam;
        pH264Param      = &pEncParam->codecParam.h264;
        pEncOps         = pMFCH264Handle->pEncOps;

        /* TBD */
        Exynos_OSAL_Log(EXYNOS_LOG_TRACE, "pBaseLayerPid->nPID: %d", pBaseLayerPid->nPID);
        pEncOps->Set_BasePID(pMFCH264Handle->hMFCHandle, pBaseLayerPid->nPID);
    }
        break;
    default:
        break;
    }

EXIT:

    FunctionOut();

    return;
}
#endif


/* DECODE_ONLY */
#ifdef BUILD_DEC
/* video dec */
OMX_ERRORTYPE Exynos_H264Dec_GetExtensionIndex_SkypeHD(
    OMX_IN OMX_HANDLETYPE  hComponent,
    OMX_IN OMX_STRING      cParameterName,
    OMX_OUT OMX_INDEXTYPE *pIndexType)
{
    OMX_ERRORTYPE             ret               = OMX_ErrorNone;
    OMX_COMPONENTTYPE        *pOMXComponent     = NULL;
    EXYNOS_OMX_BASECOMPONENT *pExynosComponent  = NULL;

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
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, OMX_MS_SKYPE_PARAM_DRIVERVER) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexSkypeParamDriverVersion;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, OMX_MS_SKYPE_PARAM_DECODERSETTING) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexSkypeParamDecoderSetting;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    if (Exynos_OSAL_Strcmp(cParameterName, OMX_MS_SKYPE_PARAM_DECODERCAP) == 0) {
        *pIndexType = (OMX_INDEXTYPE) OMX_IndexSkypeParamDecoderCapability;
        ret = OMX_ErrorNone;
        goto EXIT;
    }

    ret = OMX_ErrorUnsupportedIndex;

EXIT:
    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Dec_GetParameter_SkypeHD(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex,
    OMX_INOUT OMX_PTR     pComponentParameterStructure)
{
    OMX_ERRORTYPE                    ret               = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pOMXComponent     = NULL;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent  = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec         = NULL;
    EXYNOS_H264DEC_HANDLE           *pH264Dec          = NULL;

    FunctionIn();

    if ((hComponent == NULL) || (pComponentParameterStructure == NULL)) {
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
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoDec   = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Dec    = (EXYNOS_H264DEC_HANDLE *)pVideoDec->hCodecHandle;

    switch ((int)nParamIndex) {
    case OMX_IndexSkypeParamDriverVersion:
    {
        OMX_VIDEO_PARAM_DRIVERVER *pDriverVer = (OMX_VIDEO_PARAM_DRIVERVER *)pComponentParameterStructure;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pDriverVer, sizeof(OMX_VIDEO_PARAM_DRIVERVER));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pDriverVer->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pDriverVer->nDriverVersion = (OMX_U64)(pH264Dec->hMFCH264Handle.videoInstInfo.SwVersion);
    }
        break;
    case OMX_IndexSkypeParamDecoderCapability:
    {
        OMX_VIDEO_PARAM_DECODERCAP  *pDecoderCap    = (OMX_VIDEO_PARAM_DECODERCAP *)pComponentParameterStructure;
        OMX_VIDEO_DECODERCAP        *pstDecCap      = NULL;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pDecoderCap, sizeof(OMX_VIDEO_PARAM_DECODERCAP));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pDecoderCap->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pstDecCap = &(pDecoderCap->stDecoderCap);

        /* TBD */
        pstDecCap->bLowLatency     = OMX_TRUE;
        pstDecCap->nMaxFrameWidth  = MAX_FRAME_WIDTH;
        pstDecCap->nMaxFrameHeight = MAX_FRAME_HEIGHT;
        pstDecCap->nMaxInstances   = RESOURCE_VIDEO_DEC;
        pstDecCap->nMaxLevel       = OMX_VIDEO_AVCLevel42;
        pstDecCap->nMaxMacroblockProcessingRate = DEC_BLOCKS_PER_SECOND;
    }
        break;
    case OMX_IndexSkypeParamDecoderSetting:
    {
        OMX_VIDEO_PARAM_DECODERSETTING  *pDecoderSetting    = (OMX_VIDEO_PARAM_DECODERSETTING *)pComponentParameterStructure;
        EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec          = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
        EXYNOS_H264DEC_HANDLE           *pH264Dec           = (EXYNOS_H264DEC_HANDLE *)pVideoDec->hCodecHandle;
        OMX_VIDEO_DECODERPARAMS         *pstDecParam        = NULL;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pDecoderSetting, sizeof(OMX_VIDEO_PARAM_DECODERSETTING));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pDecoderSetting->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pstDecParam = &(pDecoderSetting->stDecParam);
        pstDecParam->bLowLatency = pH264Dec->bLowLatency;
    }
        break;
    default:
        ret = OMX_ErrorUnsupportedIndex;
        break;
    }

EXIT:

    FunctionOut();

    return ret;
}

OMX_ERRORTYPE Exynos_H264Dec_SetParameter_SkypeHD(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex,
    OMX_IN OMX_PTR        pComponentParameterStructure)
{
    OMX_ERRORTYPE                    ret               = OMX_ErrorNone;
    OMX_COMPONENTTYPE               *pOMXComponent     = NULL;
    EXYNOS_OMX_BASECOMPONENT        *pExynosComponent  = NULL;
    EXYNOS_OMX_VIDEODEC_COMPONENT   *pVideoDec         = NULL;
    EXYNOS_H264DEC_HANDLE           *pH264Dec          = NULL;

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
    if (pExynosComponent->currentState == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto EXIT;
    }

    pVideoDec   = (EXYNOS_OMX_VIDEODEC_COMPONENT *)pExynosComponent->hComponentHandle;
    pH264Dec    = (EXYNOS_H264DEC_HANDLE *)pVideoDec->hCodecHandle;

    switch ((int)nIndex) {
    case OMX_IndexSkypeParamDecoderSetting:
    {
        OMX_VIDEO_PARAM_DECODERSETTING  *pDecoderSetting = (OMX_VIDEO_PARAM_DECODERSETTING *)pComponentParameterStructure;
        OMX_VIDEO_DECODERPARAMS         *pstDecParam     = NULL;

        ret = Exynos_OMX_Check_SizeVersion_SkypeHD(pDecoderSetting, sizeof(OMX_VIDEO_PARAM_DECODERSETTING));
        if (ret != OMX_ErrorNone)
            goto EXIT;

        if (pDecoderSetting->nPortIndex > OUTPUT_PORT_INDEX) {
            ret = OMX_ErrorBadPortIndex;
            goto EXIT;
        }

        pstDecParam = &(pDecoderSetting->stDecParam);
        pH264Dec->bLowLatency = pstDecParam->bLowLatency;
    }
        break;
    default:
        ret = OMX_ErrorUnsupportedIndex;
        break;

    }

EXIT:

    FunctionOut();

    return ret;
}
#endif

#ifdef __cplusplus
}
#endif
