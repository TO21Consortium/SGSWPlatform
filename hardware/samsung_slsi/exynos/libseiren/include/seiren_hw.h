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

#ifndef __SEIREN_HW_H__
#define __SEIREN_HW_H__

#include "seiren_ioctl.h"
#include "seiren_error.h"

#define ADEC_DEV_NAME                    "/dev/seiren"

typedef unsigned int u32;

typedef enum {
    ADEC_MP3 = 0x0,
    ADEC_AAC,
    ADEC_FLAC,
    SOUND_EQ = 0x9,
    SOUND_BASS,
    AENC_AMR,
    AENC_AAC,
} SEIREN_IPTYPE;

typedef enum {
    PORT_IN = 0x1,
    PORT_OUT,
} SEIREN_PORTTYPE;

typedef enum {
    /* PCM parameters */
    PCM_PARAM_MAX_SAMPLE_RATE = 0x0,
    PCM_PARAM_MAX_NUM_OF_CH,
    PCM_PARAM_MAX_BIT_PER_SAMPLE,

    PCM_PARAM_SAMPLE_RATE,
    PCM_PARAM_NUM_OF_CH,
    PCM_PARAM_BIT_PER_SAMPLE,

    PCM_MAX_CONFIG_INFO,
    PCM_CONFIG_INFO,

    /* EQ parameters */
    SEIREN_EQ_PARAM_NUM_OF_PRESETS = 0x10,
    SEIREN_EQ_PARAM_MAX_NUM_OF_BANDS ,
    SEIREN_EQ_PARAM_RANGE_OF_BANDLEVEL,
    SEIREN_EQ_PARAM_RANGE_OF_FREQ,

    SEIREN_EQ_PARAM_PRESET_ID,
    SEIREN_EQ_PARAM_NUM_OF_BANDS,
    SEIREN_EQ_PARAM_CENTER_FREQ,
    SEIREN_EQ_PARAM_BANDLEVEL,
    SEIREN_EQ_PARAM_BANDWIDTH,

    SEIREN_EQ_MAX_CONFIG_INFO,
    SEIREN_EQ_CONFIG_INFO,
    SEIREN_EQ_BAND_INFO,

    /* BASS parameters */

    /* Codec Dec parameters */
    ADEC_PARAM_SET_EOS = 0x30,
    ADEC_PARAM_GET_OUTPUT_STATUS,

    /* MP3 Dec parameters */

    /* AAC Dec parameters */

    /* FLAC Dec parameters */

    /* Codec Enc parameters */

    /* AMR Enc parameters */

    /* AAC Enc parameters */

    /* Buffer info */
    GET_IBUF_POOL_INFO = 0xA0,
    GET_OBUF_POOL_INFO,
    SET_IBUF_POOL_INFO,
    SET_OBUF_POOL_INFO,
} SEIREN_PARAMCMD;

typedef struct audio_mem_info_t {
    void *virt_addr;
    void *phy_addr;
    u32 mem_size;
    u32 data_size;
    u32 block_count;
} audio_mem_info_t;

typedef struct audio_mem_pool_info_t {
    u32 virt_addr;
    u32 phy_addr;
    u32 block_size;
    u32 block_count;
} audio_mem_pool_info_t;

typedef struct audio_pcm_config_info_t {
    u32 nDirection;    // 0: input, 1:output
    u32 nSamplingRate;
    u32 nBitPerSample;
    u32 nNumOfChannel;
} audio_pcm_config_info_t;

#ifdef __cplusplus
extern "C" {
#endif

int ADec_Create(u32 ulPlayerID, SEIREN_IPTYPE ipType, u32* pulHandle);
int ADec_Destroy(u32 ulHandle);
int ADec_SendStream(u32 ulHandle, audio_mem_info_t* pInputInfo, int* pulConsumedSize);
int ADec_DoEQ(u32 ulHandle, audio_mem_info_t* pMemInfo);
int ADec_RecvPCM(u32 ulHandle, audio_mem_info_t* pOutputInfo);
int ADec_SetParams(u32 ulHandle, SEIREN_PARAMCMD paramCmd, unsigned long pulValues);
int ADec_GetParams(u32 ulHandle, SEIREN_PARAMCMD paramCmd, unsigned long *pulValues);
int ADec_SendEOS(u32 ulHandle);
int ADec_Flush(u32 ulHandle, SEIREN_PORTTYPE portType);
int ADec_ConfigSignal(u32 ulHandle);
int ADec_GetPCMParams(u32 ulHandle, u32 *pulValues);
int ADec_GetIMemPoolInfo(u32 ulHandle, audio_mem_info_t* pIMemPoolInfo);
int ADec_GetOMemPoolInfo(u32 ulHandle, audio_mem_info_t* pOMemPoolInfo);

#ifdef __cplusplus
}
#endif

#endif /*__SEIREN_HW_H__ */
