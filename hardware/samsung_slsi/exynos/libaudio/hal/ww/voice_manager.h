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

#define RIL_CLIENT_LIBPATH "/system/lib/libsmiril-audio.so"


/* Syncup with RIL Audio Client */

/* Voice Audio Path */
enum ril_audio_path {
    VOICE_AUIDO_PATH_NONE                   = 0,

    VOICE_AUDIO_PATH_HANDSET                = 1,
    VOICE_AUIDO_PATH_HEADSET                = 2,
    VOICE_AUIDO_PATH_HANDSFREE              = 3,
    VOICE_AUIDO_PATH_BLUETOOTH              = 4,
    VOICE_AUIDO_PATH_STEREO_BLUETOOTH       = 5,
    VOICE_AUIDO_PATH_SPEAKRERPHONE          = 6,
    VOICE_AUIDO_PATH_35PI_HEADSET           = 7,
    VOICE_AUIDO_PATH_BT_NS_EC_OFF           = 8,
    VOICE_AUIDO_PATH_WB_BLUETOOTH           = 9,
    VOICE_AUIDO_PATH_WB_BT_NS_EC_OFF        = 10,
    VOICE_AUIDO_PATH_HANDSET_HAC            = 11,

    VOICE_AUIDO_PATH_VOLTE_HANDSET          = 65,
    VOICE_AUIDO_PATH_VOLTE_HEADSET          = 66,
    VOICE_AUIDO_PATH_VOLTE_HFK              = 67,
    VOICE_AUIDO_PATH_VOLTE_BLUETOOTH        = 68,
    VOICE_AUIDO_PATH_VOLTE_STEREO_BLUETOOTH = 69,
    VOICE_AUIDO_PATH_VOLTE_SPEAKRERPHONE    = 70,
    VOICE_AUIDO_PATH_VOLTE_35PI_HEADSET     = 71,
    VOICE_AUIDO_PATH_VOLTE_BT_NS_EC_OFF     = 72,
    VOICE_AUIDO_PATH_VOLTE_WB_BLUETOOTH     = 73,
    VOICE_AUIDO_PATH_VOLTE_WB_BT_NS_EC_OFF  = 74,
    VOICE_AUIDO_PATH_MAX
};

/* Voice Audio Multi-MIC */
enum ril_audio_multimic {
    VOICE_MULTI_MIC_OFF,
    VOICE_MULTI_MIC_ON,
};

/* Voice Audio Volume */
enum ril_audio_volume {
    VOICE_AUDIO_VOLUME_INVALID   = -1,
    VOICE_AUDIO_VOLUME_LEVEL0    = 0,
    VOICE_AUDIO_VOLUME_LEVEL1,
    VOICE_AUDIO_VOLUME_LEVEL2,
    VOICE_AUDIO_VOLUME_LEVEL3,
    VOICE_AUDIO_VOLUME_LEVEL4,
    VOICE_AUDIO_VOLUME_LEVEL5,
    VOICE_AUDIO_VOLUME_LEVEL_MAX = VOICE_AUDIO_VOLUME_LEVEL5,
};

/* Voice Audio Mute */
enum ril_audio_mute {
    VOICE_AUDIO_MUTE_DISABLED,
    VOICE_AUDIO_MUTE_ENABLED,
};

/* Voice Audio Clock */
enum ril_audio_clockmode {
    VOICE_AUDIO_TURN_OFF_I2S,
    VOICE_AUDIO_TURN_ON_I2S,
};

/* Voice Loopback */
enum ril_audio_loopback {
    VOICE_AUDIO_LOOPBACK_STOP,
    VOICE_AUDIO_LOOPBACK_START,
};

enum ril_audio_loopback_path {
    VOICE_AUDIO_LOOPBACK_PATH_NA                    = 0,    //0: N/A

    VOICE_AUDIO_LOOPBACK_PATH_HANDSET               = 1,    //1: handset
    VOICE_AUDIO_LOOPBACK_PATH_HEADSET               = 2,    //2: headset
    VOICE_AUDIO_LOOPBACK_PATH_HANDSFREE             = 3,    //3: handsfree
    VOICE_AUDIO_LOOPBACK_PATH_BT                    = 4,    //4: Bluetooth
    VOICE_AUDIO_LOOPBACK_PATH_STEREO_BT             = 5,    //5: stereo Bluetooth
    VOICE_AUDIO_LOOPBACK_PATH_SPK                   = 6,    //6: speaker phone
    VOICE_AUDIO_LOOPBACK_PATH_35PI_HEADSET          = 7,    //7: 3.5pi headset
    VOICE_AUDIO_LOOPBACK_PATH_BT_NS_EC_OFF          = 8,    //8: BT NS/EC off
    VOICE_AUDIO_LOOPBACK_PATH_WB_BT                 = 9,    //9: WB Bluetooth
    VOICE_AUDIO_LOOPBACK_PATH_WB_BT_NS_EC_OFF       = 10,   //10: WB BT NS/EC
    VOICE_AUDIO_LOOPBACK_PATH_HANDSET_HAC           = 11,   //11: handset HAC

    VOICE_AUDIO_LOOPBACK_PATH_VOLTE_HANDSET         = 65,   //65: VOLTE handset
    VOICE_AUDIO_LOOPBACK_PATH_VOLTE_HEADSET         = 66,   //66: VOLTE headset
    VOICE_AUDIO_LOOPBACK_PATH_VOLTE_HANDSFREE       = 67,   //67: VOLTE hands
    VOICE_AUDIO_LOOPBACK_PATH_VOLTE_BT              = 68,   //68: VOLTE Bluetooth
    VOICE_AUDIO_LOOPBACK_PATH_VOLTE_STEREO_BT       = 69,   //69: VOLTE stere
    VOICE_AUDIO_LOOPBACK_PATH_VOLTE_SPK             = 70,   //70: VOLTE speaker phone
    VOICE_AUDIO_LOOPBACK_PATH_VOLTE_35PI_HEADSET    = 71,   //71: VOLTE 3.5pi
    VOICE_AUDIO_LOOPBACK_PATH_VOLTE_BT_NS_EC_OFF    = 72,   //72: VOLTE BT NS
    VOICE_AUDIO_LOOPBACK_PATH_VOLTE_WB_BT           = 73,   //73: VOLTE WB Blueto
    VOICE_AUDIO_LOOPBACK_PATH_VOLTE_WB_BT_NS_EC_OFF = 74,   //74: VOLTE W

    VOICE_AUDIO_LOOPBACK_PATH_HEADSET_MIC1          = 129,  //129: Headset ? MIC1
    VOICE_AUDIO_LOOPBACK_PATH_HEADSET_MIC2          = 130,  //130: Headset ? MIC2
    VOICE_AUDIO_LOOPBACK_PATH_HEADSET_MIC3          = 131,  //131: Headset ? MIC3
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

/* RIL Audio Client Interface Structure */
struct rilclient_intf {
    /* The pointer of interface library for RIL Client*/
    void *handle;

    /* Function pointers */
    int (*ril_open_client)(void);
    int (*ril_close_client)(void);
    int (*ril_register_callback)(void *, int *);
    int (*ril_set_audio_volume)(int);
    int (*ril_set_audio_path)(int);
    int (*ril_set_multi_mic)(int);
    int (*ril_set_mute)(int);
    int (*ril_set_audio_clock)(int);
    int (*ril_set_audio_loopback)(int, int);
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
