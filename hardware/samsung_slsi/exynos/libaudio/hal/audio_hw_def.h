/*
 * Copyright (C) 2014 The Android Open Source Project
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

#ifndef __EXYNOS_AUDIOHAL_DEF_H__
#define __EXYNOS_AUDIOHAL_DEF_H__

#include <system/audio.h>

char * offload_msg_table[OFFLOAD_MSG_MAX] = {
    [OFFLOAD_MSG_INVALID]            = "Offload Message_Invalid",
    [OFFLOAD_MSG_WAIT_WRITE]         = "Offload Message_Wait to write",
    [OFFLOAD_MSG_WAIT_DRAIN]         = "Offload Message_Wait to drain",
    [OFFLOAD_MSG_WAIT_PARTIAL_DRAIN] = "Offload Message_Wait to drain partially",
    [OFFLOAD_MSG_EXIT]               = "Offload Message_Wait to exit",
};

/**
 ** Default PCM Configuration
 **
 ** start_threshold: PCM Device start automatically
 **                  when PCM data in ALSA Buffer it equal or greater than this value.
 ** stop_threshold: PCM Device stop automatically
 **                 when available room in ALSA Buffer it equal or greater than this value.
 **/
struct pcm_config pcm_config_primary = {
    .channels = DEFAULT_OUTPUT_CHANNELS,
    .rate = DEFAULT_OUTPUT_SAMPLING_RATE,
    .period_size = PRIMARY_OUTPUT_PERIOD_SIZE,
    .period_count = PRIMARY_OUTPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = PRIMARY_OUTPUT_PERIOD_SIZE,
    .stop_threshold = PRIMARY_OUTPUT_STOP_THREASHOLD,
//    .silence_threshold = 0,
//    .avail_min = PRIMARY_OUTPUT_PERIOD_SIZE,
};

struct pcm_config pcm_config_low_latency = {
    .channels = DEFAULT_OUTPUT_CHANNELS,
    .rate = DEFAULT_OUTPUT_SAMPLING_RATE,
    .period_size = LOW_LATENCY_OUTPUT_PERIOD_SIZE,
    .period_count = LOW_LATENCY_OUTPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = LOW_LATENCY_OUTPUT_PERIOD_SIZE,
    .stop_threshold = LOW_LATENCY_OUTPUT_STOP_THREASHOLD,
//    .silence_threshold = 0,
//    .avail_min = LOW_LATENCY_OUTPUT_PERIOD_SIZE,
};

struct pcm_config pcm_config_deep_buffer = {
    .channels = DEFAULT_OUTPUT_CHANNELS,
    .rate = DEFAULT_OUTPUT_SAMPLING_RATE,
    .period_size = DEEP_BUFFER_OUTPUT_PERIOD_SIZE,
    .period_count = DEEP_BUFFER_OUTPUT_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = DEEP_BUFFER_OUTPUT_PERIOD_SIZE,
    .stop_threshold = DEEP_BUFFER_OUTPUT_STOP_THREASHOLD,
//    .silence_threshold = 0,
//    .avail_min = DEEP_BUFFER_OUTPUT_PERIOD_SIZE,
};

struct pcm_config pcm_config_audio_capture = {
    .channels = DEFAULT_INPUT_CHANNELS,
    .rate = DEFAULT_INPUT_SAMPLING_RATE,
    .period_size = AUDIO_CAPTURE_PERIOD_SIZE,
    .period_count = AUDIO_CAPTURE_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_vc_nb = {
    .channels = DEFAULT_VOICE_CHANNELS,
    .rate = NB_VOICE_SAMPLING_RATE,
    .period_size = WB_VOICE_PERIOD_SIZE,
    .period_count = WB_VOICE_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_vc_wb = {
    .channels = DEFAULT_VOICE_CHANNELS,
    .rate = WB_VOICE_SAMPLING_RATE,
    .period_size = WB_VOICE_PERIOD_SIZE,
    .period_count = WB_VOICE_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

struct pcm_config pcm_config_bt_sco = {
    .channels = DEFAULT_BT_SCO_CHANNELS,
    .rate = WB_VOICE_SAMPLING_RATE,
    .period_size = BT_SCO_PERIOD_SIZE,
    .period_count = BT_SCO_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
};

/**
 ** Sound Card and Sound Device for specific Audio usage
 **/
int sound_device_table[AUSAGE_MAX][2] = {
    [AUSAGE_PLAYBACK_PRIMARY]       = {PRIMARY_SOUND_CARD, PRIMARY_PLAYBACK_DEVICE},
    [AUSAGE_PLAYBACK_LOW_LATENCY]   = {LOW_LATENCY_SOUND_CARD, LOW_LATENCY_PLAYBACK_DEVICE},
    [AUSAGE_PLAYBACK_DEEP_BUFFER]   = {DEEP_BUFFER_SOUND_CARD, DEEP_BUFFER_PLAYBACK_DEVICE},
    [AUSAGE_PLAYBACK_COMPR_OFFLOAD] = {COMPR_OFFLOAD_SOUND_CARD, COMPR_OFFLOAD_PLAYBACK_DEVICE},
    [AUSAGE_PLAYBACK_AUX_DIGITAL]   = {AUX_DIGITAL_SOUND_CARD, AUX_DIGITAL_PLAYBACK_DEVICE},
    [AUSAGE_CAPTURE_LOW_LATENCY]    = {LOW_LATENCY_SOUND_CARD, LOW_LATENCY_CAPTURE_DEVICE},
};

/**
 ** Audio Usage & Mode Table for readable log messages
 **/
char * usage_table[AUSAGE_CNT] = {
    [AUSAGE_PLAYBACK_PRIMARY]       = "primary_out",
    [AUSAGE_PLAYBACK_LOW_LATENCY]   = "fast_out",
    [AUSAGE_PLAYBACK_DEEP_BUFFER]   = "deep_out",
    [AUSAGE_PLAYBACK_COMPR_OFFLOAD] = "offload_out",
    [AUSAGE_PLAYBACK_AUX_DIGITAL]   = "aux_out",
    [AUSAGE_CAPTURE_LOW_LATENCY]    = "primary_in",
};

char * mode_table[AUSAGE_CNT] = {
    [AUSAGE_MODE_NORMAL]            = "normal",
    [AUSAGE_MODE_VOICE_CALL]        = "voice_call",
    [AUSAGE_MODE_VOIP_CALL]         = "voip_call",
    [AUSAGE_MODE_LTE_CALL]          = "LTE_call",
    [AUSAGE_MODE_WIFI_CALL]         = "WiFi_call",
    [AUSAGE_MODE_NONE]              = "none",
};

/**
 ** Device Path(Codec to Device) Configuration based on Audio Input/Output Device
 **/
char * device_path_table[DEVICE_CNT] = {
    [DEVICE_EARPIECE]              = "handset",
    [DEVICE_SPEAKER]               = "speaker",
    [DEVICE_HEADSET]               = "headset",
    [DEVICE_HEADPHONE]             = "headset",
    [DEVICE_SPEAKER_AND_HEADSET]   = "speaker-headset",
    [DEVICE_SPEAKER_AND_HEADPHONE] = "speaker-headset",
    [DEVICE_BT_HEADSET]            = "bt-sco-headset",
    [DEVICE_MAIN_MIC]              = "mic",
    [DEVICE_HEADSET_MIC]           = "headset-mic",
    [DEVICE_BT_HEADSET_MIC]        = "bt-sco-mic",
    [DEVICE_NONE]                  = "none",
};

/* Audio Routing Path = Ausage_Mode or Service Name + -(Hyphen) + Device Name */

/**
 ** Service Path(AP/CP to Codec) Configuration based on Audio Usage
 **/
 char * mode_path_table[AUSAGE_CNT] = {
    [AUSAGE_MODE_NORMAL]                = "media",
    [AUSAGE_MODE_VOICE_CALL]            = "incall",
    [AUSAGE_MODE_VOIP_CALL]             = "communication",
    [AUSAGE_MODE_LTE_CALL]              = "incall",
    [AUSAGE_MODE_WIFI_CALL]             = "media",
    [AUSAGE_MODE_NONE]                  = "none",
};

#endif  // __EXYNOS_AUDIOHAL_DEF_H__
