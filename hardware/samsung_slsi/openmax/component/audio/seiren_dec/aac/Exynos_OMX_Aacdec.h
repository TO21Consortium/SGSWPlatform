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
 * @file      Exynos_OMX_Aacdec.h
 * @brief
 * @author    Sungyeon Kim (sy85.kim@samsung.com)
 * @version   1.0.0
 * @history
 *   2012.12.22 : Create
 */

#ifndef EXYNOS_OMX_AAC_DEC_COMPONENT
#define EXYNOS_OMX_AAC_DEC_COMPONENT

#include "Exynos_OMX_Def.h"
#include "OMX_Component.h"
#include "seiren_hw.h"

static const uint32_t aac_sample_rates[] =
{
    96000, 88200, 64000, 48000, 44100, 32000,
    24000, 22050, 16000, 12000, 11025, 8000
};

typedef struct _EXYNOS_Seiren_AAC_HANDLE
{
    OMX_U32          hSeirenHandle;
    OMX_BOOL         bConfiguredSeiren;
    OMX_BOOL         bSeirenLoaded;
    OMX_BOOL         bSeirenSendEOS;
    OMX_S32          returnCodec;
    audio_mem_info_t input_mem_pool;
    audio_mem_info_t output_mem_pool;
} EXYNOS_Seiren_AAC_HANDLE;

typedef struct _EXYNOS_AAC_HANDLE
{
    /* OMX Codec specific */
    OMX_AUDIO_PARAM_AACPROFILETYPE aacParam;
    OMX_AUDIO_PARAM_PCMMODETYPE    pcmParam;

    /* SEC Seiren Codec specific */
    EXYNOS_Seiren_AAC_HANDLE       hSeirenAacHandle;
} EXYNOS_AAC_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName);
                OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
};
#endif

#endif /* EXYNOS_OMX_AAC_DEC_COMPONENT */
