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
 * @file      Exynos_OMX_Flacdec.h
 * @brief
 * @author    Sungyeon Kim (sy85.kim@samsung.com)
 * @version   1.0.0
 * @history
 *   2012.12.22 : Create
 */

#ifndef EXYNOS_OMX_FLAC_DEC_COMPONENT
#define EXYNOS_OMX_FLAC_DEC_COMPONENT

#include "Exynos_OMX_Def.h"
#include "OMX_Component.h"
#include "seiren_hw.h"

static const uint32_t flac_sample_rates[] =
{
    0,     88200, 176400, 192000, 8000, 16000, 22050, 24000,
    32000,  44100, 48000, 96000, 0,      0,      0,     0
};
static const uint32_t flac_channels[] =
{
    1, 2, 3, 4, 5, 6, 7, 8,
    2, 2, 2, 0, 0, 0, 0, 0
};

typedef struct _EXYNOS_Seiren_FLAC_HANDLE
{
    OMX_U32          hSeirenHandle;
    OMX_BOOL         bConfiguredSeiren;
    OMX_BOOL         bSeirenLoaded;
    OMX_BOOL         bSeirenSendEOS;
    OMX_S32          returnCodec;
    audio_mem_info_t input_mem_pool;
    audio_mem_info_t output_mem_pool;
} EXYNOS_Seiren_FLAC_HANDLE;

typedef struct _EXYNOS_FLAC_HANDLE
{
    /* OMX Codec specific */
    OMX_AUDIO_PARAM_FLACTYPE flacParam;
    OMX_AUDIO_PARAM_PCMMODETYPE    pcmParam;

    /* SEC Seiren Codec specific */
    EXYNOS_Seiren_FLAC_HANDLE       hSeirenFlacHandle;
} EXYNOS_FLAC_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

OSCL_EXPORT_REF OMX_ERRORTYPE Exynos_OMX_ComponentInit(OMX_HANDLETYPE hComponent, OMX_STRING componentName);
                OMX_ERRORTYPE Exynos_OMX_ComponentDeinit(OMX_HANDLETYPE hComponent);

#ifdef __cplusplus
};
#endif

#endif /* EXYNOS_OMX_FLAC_DEC_COMPONENT */
