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
 * @file        Exynos_OSAL_SkypeHD.h
 * @brief
 * @author      Seungbeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2015.04.27 : Create
 */

#ifndef Exynos_OSAL_SKYPEHD
#define Exynos_OSAL_SKYPEHD

#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_Index.h"

#ifdef __cplusplus
extern "C" {
#endif

/* COMMON_ONLY */
OMX_ERRORTYPE Exynos_OMX_Check_SizeVersion_SkypeHD(OMX_PTR header, OMX_U32 size);

#ifdef BUILD_ENC
/* ENCODE_ONLY */
OMX_ERRORTYPE Exynos_H264Enc_GetExtensionIndex_SkypeHD(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_STRING cParameterName, OMX_OUT OMX_INDEXTYPE *pIndexType);
OMX_ERRORTYPE Exynos_H264Enc_GetParameter_SkypeHD(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex, OMX_INOUT OMX_PTR pComponentParameterStructure);
OMX_ERRORTYPE Exynos_H264Enc_SetParameter_SkypeHD(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex, OMX_IN OMX_PTR pComponentParameterStructure);
OMX_ERRORTYPE Exynos_H264Enc_GetConfig_SkypeHD(OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure);
OMX_ERRORTYPE Exynos_H264Enc_SetConfig_SkypeHD(OMX_HANDLETYPE hComponent,
    OMX_INDEXTYPE nIndex, OMX_PTR pComponentConfigStructure);
void Change_H264Enc_SkypeHDParam(EXYNOS_OMX_BASECOMPONENT *pExynosComponent, OMX_PTR pDynamicConfigCMD);
#endif

#ifdef BUILD_DEC
/* DECODE_ONLY */
OMX_ERRORTYPE Exynos_H264Dec_GetExtensionIndex_SkypeHD(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_STRING cParameterName, OMX_OUT OMX_INDEXTYPE *pIndexType);
OMX_ERRORTYPE Exynos_H264Dec_GetParameter_SkypeHD(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nParamIndex, OMX_INOUT OMX_PTR pComponentParameterStructure);
OMX_ERRORTYPE Exynos_H264Dec_SetParameter_SkypeHD(OMX_IN OMX_HANDLETYPE hComponent,
    OMX_IN OMX_INDEXTYPE  nIndex, OMX_IN OMX_PTR pComponentParameterStructure);
#endif

#ifdef __cplusplus
}
#endif

#endif
