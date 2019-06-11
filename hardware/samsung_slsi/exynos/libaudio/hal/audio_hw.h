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

#ifndef __EXYNOS_AUDIOHAL_H__
#define __EXYNOS_AUDIOHAL_H__

#include <cutils/list.h>

#include <audio_route/audio_route.h>
#include <audio_utils/resampler.h>

/* Definition of AudioHAL */
#include <hardware/hardware.h>
#include <hardware/audio.h>
#include <hardware/audio_alsaops.h>

#include <system/audio.h>


/* PCM Interface of ALSA & Compress Offload Interface */
#include <tinyalsa/asoundlib.h>
#include <tinycompress/tinycompress.h>
#include <compress_params.h>

/* SoC specific header */
#include <audio_conf.h>
/* Voice call - RIL interface */
#include "voice_manager.h"



/* Mixer Path configuration file for working AudioHAL */
#define VENDOR_MIXER_XML_PATH "/vendor/etc/mixer_paths.xml"
/* Effect HAL and Visualizer library path for offload scenario */
#define OFFLOAD_VISUALIZERHAL_PATH "/system/lib/soundfx/libexynosvisualizer.so"




/**
 ** Audio Usages For AudioHAL
 **/
typedef enum {
    AUSAGE_PLAYBACK,
    AUSAGE_CAPTURE,
} audio_usage_type_t;

/* Audio Usages */
typedef enum {
    AUSAGE_DEFAULT               = -1,
    AUSAGE_MIN                   = 0,

    AUSAGE_PLAYBACK_PRIMARY      = 0,  // For Primary Output Profile
    AUSAGE_PLAYBACK_LOW_LATENCY,       // For Fast Output Profile
    AUSAGE_PLAYBACK_DEEP_BUFFER,       // For Deep Buffer Profile
    AUSAGE_PLAYBACK_COMPR_OFFLOAD,     // For Compress Offload Profile
    AUSAGE_PLAYBACK_AUX_DIGITAL,       // For HDMI Profile


    AUSAGE_CAPTURE_LOW_LATENCY,        // For Primary Input Profile

    AUSAGE_MAX,
    AUSAGE_CNT                   = AUSAGE_MAX
} audio_usage_id_t;

/* usage mode definitions */
typedef enum {
    AUSAGE_MODE_NORMAL               = 0,
    AUSAGE_MODE_VOICE_CALL,
    AUSAGE_MODE_VOIP_CALL,
    AUSAGE_MODE_LTE_CALL,
    AUSAGE_MODE_WIFI_CALL,

    AUSAGE_MODE_NONE,
    AUSAGE_MODE_MAX,
    AUSAGE_MODE_CNT            = AUSAGE_MODE_MAX
} audio_usage_mode_t;

/**
 ** Stream Status
 **/
typedef enum {
    STATE_STANDBY   = 0,  // Stream is opened, but Device(PCM or Compress) is not opened yet.
    STATE_IDLE,           // Stream is opened & Device(PCM or Compress) is opened.
    STATE_PLAYING,        // Stream is opened & Device(PCM or Compress) is opened & Device is working.
    STATE_PAUSED,         // Stream is opened & Device(Compress) is opened & Device is pausing.(only available for Compress Offload Stream)
} stream_state_type_t;

/**
 ** Exynos Offload Message List
 **/
typedef enum {
    OFFLOAD_MSG_INVALID              = 0,

    OFFLOAD_MSG_WAIT_WRITE,
    OFFLOAD_MSG_WAIT_DRAIN,
    OFFLOAD_MSG_WAIT_PARTIAL_DRAIN,
    OFFLOAD_MSG_EXIT,

    OFFLOAD_MSG_MAX,
} offload_msg_type_t;

/**
 ** Call State
 **/
typedef enum {
    CALL_OFF   = 0,
    CP_CALL,
    LTE_CALL,
    WIFI_CALL,
} call_state_type_t;

struct exynos_offload_msg {
    struct listnode node;
    offload_msg_type_t msg;
};

/**
 ** Real Audio In/Output Device based on Target Device
 **/
typedef enum {
    DEVICE_EARPIECE               = 0,   // handset or receiver
    DEVICE_SPEAKER,
    DEVICE_HEADSET,                      // headphone + mic
    DEVICE_HEADPHONE,                    // headphone or earphone
    DEVICE_SPEAKER_AND_HEADSET,
    DEVICE_SPEAKER_AND_HEADPHONE,
    DEVICE_BT_HEADSET,

    DEVICE_MAIN_MIC,
    DEVICE_HEADSET_MIC,
    DEVICE_BT_HEADSET_MIC,

    DEVICE_NONE,
    DEVICE_MAX,
    DEVICE_CNT                   = DEVICE_MAX
} device_type_t;


/**
 **  Mapping Audio In/Output Device in Audio.h into Real In/Output Device of Set/Board
 **/
static device_type_t get_device_id(audio_devices_t devices)
{
    if (devices > AUDIO_DEVICE_BIT_IN) {
        /* Input Devices */
        if (popcount(devices) == 2) {
            switch (devices) {
                case AUDIO_DEVICE_IN_BUILTIN_MIC:   return DEVICE_MAIN_MIC;
                case AUDIO_DEVICE_IN_WIRED_HEADSET: return DEVICE_HEADSET_MIC;
                case AUDIO_DEVICE_IN_BLUETOOTH_SCO_HEADSET: return DEVICE_BT_HEADSET_MIC;

                default:                            return DEVICE_NONE;
            }
        }
    } else {
        /* Output Devices */
        if (popcount(devices) == 1) {
            /* Single Device */
            switch (devices) {
                case AUDIO_DEVICE_OUT_EARPIECE:        return DEVICE_EARPIECE;
                case AUDIO_DEVICE_OUT_SPEAKER:         return DEVICE_SPEAKER;
                case AUDIO_DEVICE_OUT_WIRED_HEADSET:   return DEVICE_HEADSET;
                case AUDIO_DEVICE_OUT_WIRED_HEADPHONE: return DEVICE_HEADPHONE;
                case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:
                case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
                case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
                    return DEVICE_BT_HEADSET;

                case AUDIO_DEVICE_NONE:
                default:                               return DEVICE_NONE;
            }
        } else if (popcount(devices) == 2) {
            /* Dual Device */
            if (devices == (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_WIRED_HEADSET))
                return DEVICE_SPEAKER_AND_HEADSET;
            if (devices == (AUDIO_DEVICE_OUT_SPEAKER | AUDIO_DEVICE_OUT_WIRED_HEADPHONE))
                return DEVICE_SPEAKER_AND_HEADPHONE;
        }
    }

    return DEVICE_NONE;
}

static device_type_t get_indevice_id_from_outdevice(device_type_t devices)
{
    switch (devices) {
        case DEVICE_EARPIECE:
        case DEVICE_SPEAKER:
        case DEVICE_HEADPHONE:
        case DEVICE_SPEAKER_AND_HEADPHONE:
            return DEVICE_MAIN_MIC;

        case DEVICE_HEADSET:
        case DEVICE_SPEAKER_AND_HEADSET:
            return DEVICE_HEADSET_MIC;

        case DEVICE_BT_HEADSET:
            return DEVICE_BT_HEADSET_MIC;

        case DEVICE_MAIN_MIC:
        case DEVICE_HEADSET_MIC:
            return devices;

        case DEVICE_NONE:
        default:
            return DEVICE_NONE;
    }

    return DEVICE_NONE;
}


/**
 ** Structure for Audio Output Stream
 ** Implement audio_stream_out structure
 **/
struct stream_out {
    struct audio_stream_out stream;
    pthread_mutex_t lock;

    /* These variables are needed to save Android Request becuase pcm_config
         and audio_config are different */
    audio_io_handle_t handle;
    audio_devices_t devices;
    audio_output_flags_t flags;
    unsigned int sample_rate;
    audio_channel_mask_t channel_mask;
    audio_format_t format;

    audio_usage_id_t ausage;
    stream_state_type_t sstate;
    bool mixer_path_setup;

    /* PCM specific */
    struct pcm *pcminfo;
    struct pcm_config pcmconfig;
    uint64_t written; /* total frames written, not cleared when entering standby */

    /* Offload specific */
    struct compress *comprinfo;
    struct compr_config comprconfig;
    int nonblock_flag;
    float vol_left, vol_right;

    stream_callback_t offload_callback;
    void *offload_cookie;

    pthread_t offload_callback_thread;

    pthread_cond_t offload_msg_cond;
    struct listnode offload_msg_list;

    pthread_cond_t offload_sync_cond;
    bool callback_thread_blocked;

    struct compr_gapless_mdata offload_metadata;
    int ready_new_metadata;

    unsigned long err_count;
    struct audio_device *adev;
};

/**
 ** Structure for Audio Input Stream
 ** Implement audio_stream_in structure
 **/
struct stream_in {
    struct audio_stream_in stream;
    pthread_mutex_t lock;

    /* These variables are needed to save Android Request becuase pcm_config
         and audio_config are different */
    audio_io_handle_t handle;
    audio_devices_t devices;
    audio_source_t source;
    audio_input_flags_t flags;
    unsigned int sample_rate;
    audio_channel_mask_t channel_mask;
    audio_format_t format;

    audio_usage_id_t ausage;
    stream_state_type_t sstate;
    bool mixer_path_setup;

    /* PCM specific */
    struct pcm *pcminfo;
    struct pcm_config pcmconfig;

    unsigned long err_count;
    struct audio_device *adev;
};

/**
 ** Exynos AudioHAL Usage List
 **/
union stream_ptr {
    struct stream_in *in;
    struct stream_out *out;
};

struct exynos_audio_usage {
    struct listnode node;

    audio_usage_type_t usage_type;  /* Audio Usage Type */
    audio_usage_id_t usage_id;      /* Audio Usage ID */

    device_type_t out_device_id;    /* related out_device */
    device_type_t in_device_id;     /* related in_device */
    audio_usage_mode_t out_device_amode; /*related out device usage mode */
    audio_usage_mode_t in_device_amode; /*related in device usage mode */

    union stream_ptr stream;        /* related stream_in/stream_out structure */
};

/**
 ** Routing Information
 **/
struct route_info {
    unsigned int card_num;
    struct audio_route *aroute;
};

/**
 ** Structure for Audio Primary HW Module
 ** Implement audio_hw_device structure
 **/
struct audio_device {
    struct audio_hw_device hw_device;
    pthread_mutex_t lock; /* see note below on mutex acquisition order */

    audio_mode_t amode;
    audio_usage_mode_t usage_amode;
    call_state_type_t call_state;
    bool screen_off;

    struct stream_out *primary_output;   // Need to know which stream is primary
    struct pcm *pcm_capture;             // Capture PCM Device is unique

    struct route_info *rinfo;
    struct mixer *mixerinfo;             // For Volume control & Mute
    struct mixer_ctl * vol_ctrl;

    struct listnode audio_usage_list;

    /* Voice */
    struct voice_manager *voice;
    struct pcm *pcm_voice_rx;      // PCM Device for Voice Capture
    struct pcm *pcm_voice_tx;      // PCM Device for Voice Playback
    float voice_volume;

    /* BT-SCO */
    struct pcm *pcm_btsco_in;      // PCM Device for bt-sco Capture
    struct pcm *pcm_btsco_out;     // PCM Device for bt-sco Playback

    /* Visualizer Library Link */
    void *offload_visualizer_lib;
    int (*notify_start_output_tovisualizer)(audio_io_handle_t);
    int (*notify_stop_output_tovisualizer)(audio_io_handle_t);
};

#define MAX_PATH_NAME_LEN 50
#define MAX_ERR_COUNT 10

#endif  // __EXYNOS_AUDIOHAL_H__
