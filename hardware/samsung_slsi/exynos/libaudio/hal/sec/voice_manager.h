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

#ifndef __EXYNOS_VOICE_SERVICE_H__
#define __EXYNOS_VOICE_SERVICE_H__

#include <system/audio.h>

#define RIL_CLIENT_LIBPATH "/system/lib/libsecril-client.so"


typedef enum _SoundType {
    SOUND_TYPE_VOICE,
    SOUND_TYPE_SPEAKER,
    SOUND_TYPE_HEADSET,
    SOUND_TYPE_BTVOICE
} SoundType;

typedef enum _AudioPath {
    SOUND_AUDIO_PATH_HANDSET,
    SOUND_AUDIO_PATH_HEADSET,
    SOUND_AUDIO_PATH_SPEAKER,
    SOUND_AUDIO_PATH_BLUETOOTH,
    SOUND_AUDIO_PATH_STEREO_BT,
    SOUND_AUDIO_PATH_HEADPHONE,
    SOUND_AUDIO_PATH_BLUETOOTH_NO_NR,
    SOUND_AUDIO_PATH_MIC1,
    SOUND_AUDIO_PATH_MIC2,
    SOUND_AUDIO_PATH_BLUETOOTH_WB,
    SOUND_AUDIO_PATH_BLUETOOTH_WB_NO_NR,
    SOUND_AUDIO_PATH_HANDSET_HAC,
    SOUND_AUDIO_PATH_SLD,
    SOUND_AUDIO_PATH_VOLTE_HANDSET = 30,
    SOUND_AUDIO_PATH_VOLTE_HEADSET,
    SOUND_AUDIO_PATH_VOLTE_SPEAKER,
    SOUND_AUDIO_PATH_VOLTE_BLUETOOTH,
    SOUND_AUDIO_PATH_VOLTE_STEREO_BT,
    SOUND_AUDIO_PATH_VOLTE_HEADPHONE,
    SOUND_AUDIO_PATH_VOLTE_BLUETOOTH_NO_NR,
    SOUND_AUDIO_PATH_VOLTE_MIC1,
    SOUND_AUDIO_PATH_VOLTE_MIC2,
    SOUND_AUDIO_PATH_VOLTE_BLUETOOTH_WB,
    SOUND_AUDIO_PATH_VOLTE_BLUETOOTH_WB_NO_NR,
    SOUND_AUDIO_PATH_VOLTE_HANDSET_HAC,
    SOUND_AUDIO_PATH_VOLTE_SLD,
    SOUND_AUDIO_PATH_CALL_FWD = 50,
    SOUND_AUDIO_PATH_HEADSET_MIC1,
    SOUND_AUDIO_PATH_HEADSET_MIC2,
    SOUND_AUDIO_PATH_HEADSET_MIC3
} AudioPath;

/* Voice Audio Multi-MIC */
enum ril_audio_multimic {
    VOICE_MULTI_MIC_OFF,
    VOICE_MULTI_MIC_ON,
};

typedef enum _ExtraVolume {
    ORIGINAL_PATH,
    EXTRA_VOLUME_PATH,
    EMERGENCY_PATH
} ExtraVolume;

typedef enum _SoundClockCondition {
    SOUND_CLOCK_STOP,
    SOUND_CLOCK_START
} SoundClockCondition;

typedef enum _MuteCondition {
      TX_UNMUTE, /* 0x00: TX UnMute */
      TX_MUTE,   /* 0x01: TX Mute */
      RX_UNMUTE, /* 0x02: RX UnMute */
      RX_MUTE,   /* 0x03: RX Mute */
      RXTX_UNMUTE, /* 0x04: RXTX UnMute */
      RXTX_MUTE,   /* 0x05: RXTX Mute */
} MuteCondition;

enum ril_audio_clockmode {
    VOICE_AUDIO_TURN_OFF_I2S,
    VOICE_AUDIO_TURN_ON_I2S
};

/* Voice Call Mode */
enum voice_call_mode {
    VOICE_CALL_NONE = 0,
    VOICE_CALL_CS,              // CS(Circit Switched) Call
    VOICE_CALL_PS,              // PS(Packet Switched) Call
    VOICE_CALL_MAX,
};

/* Event from RIL Audio Client */
#define VOICE_AUDIO_EVENT_BASE                     10000
#define VOICE_AUDIO_EVENT_RINGBACK_STATE_CHANGED   (VOICE_AUDIO_EVENT_BASE + 1)
#define VOICE_AUDIO_EVENT_IMS_SRVCC_HANDOVER       (VOICE_AUDIO_EVENT_BASE + 2)


struct rilclient_intf {
    /* The pointer of interface library for RIL Client*/
    void *handle;

    /* The SIPC RIL Client Handle */
    /* This will be used as parameter of RIL Client Functions */
    void *client;

    SoundType sound_type;

    /* Function pointers */
    void *(*ril_open_client)(void);
    int (*ril_close_client)(void *);
    int (*ril_connect)(void *);
    int (*ril_is_connected)(void *);
    int (*ril_disconnect)(void *);
    int (*ril_set_volume)(void *, SoundType, int);
    int (*ril_set_audio_path)(void *, AudioPath, ExtraVolume);
    int (*ril_set_clock_sync)(void *, SoundClockCondition);
    int (*ril_set_mute)(void *, MuteCondition);
};


struct voice_manager {
    struct rilclient_intf rilc;

    bool state_call;           // Current Call Status
    enum voice_call_mode mode; // Current Call Mode
    bool state_mic_mute;       // Current Main MIC Mute Status

    int volume_steps_max;      // Voice Volume maximum steps

    int (*callback)(int, const void *, unsigned int); // Callback Function Pointer
};


/* General Functiuons */
bool voice_is_in_call(struct voice_manager *voice);
int voice_set_call_mode(struct voice_manager *voice, enum voice_call_mode cmode);

/* RIL Audio Client related Functions */
int voice_open(struct voice_manager * voice);
int voice_close(struct voice_manager * voice);
int voice_set_callback(struct voice_manager * voice, void * callback_func);

int voice_set_volume(struct voice_manager *voice, float volume);
int voice_set_path(struct voice_manager * voice, audio_devices_t devices);
int voice_set_multimic(struct voice_manager *voice, enum ril_audio_multimic mmic);
int voice_set_mic_mute(struct voice_manager *voice, bool state);
bool voice_get_mic_mute(struct voice_manager *voice);
int voice_set_audio_clock(struct voice_manager *voice, enum ril_audio_clockmode clockmode);

/* Voice Manager related Functiuons */
void voice_deinit(struct voice_manager *voice);
struct voice_manager * voice_init(void);

#endif  // __EXYNOS_VOICE_SERVICE_H__
