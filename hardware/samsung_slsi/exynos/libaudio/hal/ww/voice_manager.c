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

#define LOG_TAG "voice_manager"
//#define LOG_NDEBUG 0

#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <dlfcn.h>
#include <stdlib.h>

#include <cutils/log.h>

#include "voice_manager.h"
#include <cutils/properties.h>

#define VOLUME_STEPS_DEFAULT  "5"
#define VOLUME_STEPS_PROPERTY "ro.config.vc_call_vol_steps"

bool voice_is_in_call(struct voice_manager *voice)
{
    return voice->state_call;
}

int voice_set_call_mode(struct voice_manager *voice, enum voice_call_mode cmode)
{
    int ret = 0;

    if (voice) {
        voice->mode = cmode;
        ALOGD("%s: Set Call Mode = %d!", __func__, voice->mode);
    }

    return ret;
}

int voice_callback(void * handle, int event, const void *data, unsigned int datalen)
{
    struct voice_manager *voice = (struct voice_manager *)handle;
    int (*funcp)(int, const void *, unsigned int) = NULL;

    ALOGD("%s: Called Callback Function from RIL Audio Client!", __func__);
    if (voice) {
        switch (event) {
            case VOICE_AUDIO_EVENT_RINGBACK_STATE_CHANGED:
                ALOGD("%s: Received RINGBACK_STATE_CHANGED event!", __func__);
                break;

            case VOICE_AUDIO_EVENT_IMS_SRVCC_HANDOVER:
                ALOGD("%s: Received IMS_SRVCC_HANDOVER event!", __func__);
                break;

            default:
                ALOGD("%s: Received Unsupported event (%d)!", __func__, event);
                return 0;
        }

        funcp = voice->callback;
        funcp(event, data, datalen);
    }

    return 0;
}

int voice_set_mic_mute(struct voice_manager *voice, bool state)
{
    int ret = 0;

    voice->state_mic_mute = state;
    if (voice->state_call) {
        if (voice->rilc.ril_set_mute) {
            if (state)
                voice->rilc.ril_set_mute(VOICE_AUDIO_MUTE_ENABLED);
            else
                voice->rilc.ril_set_mute(VOICE_AUDIO_MUTE_DISABLED);
        }
        ALOGD("%s: MIC Mute = %d!", __func__, state);
    }

    return ret;
}

bool voice_get_mic_mute(struct voice_manager *voice)
{
    ALOGD("%s: MIC Mute = %d!", __func__, voice->state_mic_mute);
    return voice->state_mic_mute;
}

int voice_set_volume(struct voice_manager *voice, float volume)
{
    int vol, ret = 0;

    if (voice->state_call) {
        if (voice->rilc.ril_set_audio_volume)
            voice->rilc.ril_set_audio_volume((int)(volume * voice->volume_steps_max));

        ALOGD("%s: Volume = %d(%f)!", __func__, (int)(volume * voice->volume_steps_max), volume);
    }

    return ret;
}

int voice_set_audio_clock(struct voice_manager *voice, enum ril_audio_clockmode clockmode)
{
    int ret = 0;

    if (voice->state_call) {
        if (voice->rilc.ril_set_audio_clock)
            voice->rilc.ril_set_audio_clock((int)clockmode);

        ALOGD("%s: AudioClock Mode = %s!", __func__, (clockmode? "ON" : "OFF"));
    }

    return ret;
}

static enum ril_audio_path map_incall_device(struct voice_manager *voice, audio_devices_t devices)
{
    enum ril_audio_path device_type = VOICE_AUDIO_PATH_HANDSET;

    switch(devices) {
        case AUDIO_DEVICE_OUT_EARPIECE:
            if (voice->mode == VOICE_CALL_CS)
                device_type = VOICE_AUDIO_PATH_HANDSET;
            else
                device_type = VOICE_AUIDO_PATH_VOLTE_HANDSET;
            break;

        case AUDIO_DEVICE_OUT_SPEAKER:
            if (voice->mode == VOICE_CALL_CS)
                device_type = VOICE_AUIDO_PATH_SPEAKRERPHONE;
            else
                device_type = VOICE_AUIDO_PATH_VOLTE_SPEAKRERPHONE;
            break;

        case AUDIO_DEVICE_OUT_WIRED_HEADSET:
        case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
            if (voice->mode == VOICE_CALL_CS)
                device_type = VOICE_AUIDO_PATH_HEADSET;
            else
                device_type = VOICE_AUIDO_PATH_VOLTE_HEADSET;
            break;

        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_HEADSET:
        case AUDIO_DEVICE_OUT_BLUETOOTH_SCO_CARKIT:
            if (voice->mode == VOICE_CALL_CS)
                device_type = VOICE_AUIDO_PATH_STEREO_BLUETOOTH;
            else
                device_type = VOICE_AUIDO_PATH_VOLTE_STEREO_BLUETOOTH;
            break;

        default:
            if (voice->mode == VOICE_CALL_CS)
                device_type = VOICE_AUDIO_PATH_HANDSET;
            else
                device_type = VOICE_AUIDO_PATH_VOLTE_HANDSET;
            break;
    }

    return device_type;
}

int voice_set_path(struct voice_manager *voice, audio_devices_t devices)
{
    int ret = 0;
    enum ril_audio_path path;

    if (voice->state_call) {
        /* Mapping */
        path = map_incall_device(voice, devices);

        if (voice->rilc.ril_set_audio_path) {
            ret = voice->rilc.ril_set_audio_path(path);
            if (ret == 0) {
                ALOGD("%s: Set Audio Path to %d!", __func__, path);
            } else {
                ALOGE("%s: Failed to set path in RIL Client!", __func__);
                return ret;
            }
        } else {
            ALOGE("%s: ril_set_audio_path is not available.", __func__);
            ret = -1;
        }
    } else {
        ALOGE("%s: Voice is not IN_CALL", __func__);
        ret = -1;
    }

    return ret;
}


int voice_open(struct voice_manager *voice)
{
    int ret = 0;

    if (!voice->state_call) {
        if (voice->rilc.ril_open_client) {
            ret = voice->rilc.ril_open_client();
            if (ret == 0) {
                voice->state_call = true;
                ALOGD("%s: Opened RIL Client, Transit to IN_CALL!", __func__);
            } else {
                ALOGE("%s: Failed to open RIL Client!", __func__);
            }
        } else {
            ALOGE("%s: ril_open_client is not available.", __func__);
            ret = -1;
        }
    }

    return ret;
}

int voice_close(struct voice_manager *voice)
{
    int ret = 0;

    if (voice->state_call) {
        if (voice->rilc.ril_close_client) {
            ret = voice->rilc.ril_close_client();
            if (ret == 0) {
                voice->state_call = false;
                ALOGD("%s: Closed RIL Client, Transit to NOT_IN_CALL!", __func__);
            } else {
                ALOGE("%s: Failed to close RIL Client!", __func__);
            }
        } else {
            ALOGE("%s: ril_close_client is not available.", __func__);
            ret = -1;
        }
    }

    return ret;
}

int voice_set_callback(struct voice_manager * voice, void * callback_func)
{
    int ret = 0;

    if (voice->rilc.ril_register_callback) {
        ret = voice->rilc.ril_register_callback((void *)voice, (int *)voice_callback);
        if (ret == 0) {
            ALOGD("%s: Succeded to register Callback Function!", __func__);
            voice->callback = callback_func;
        }
        else
            ALOGE("%s: Failed to register Callback Function!", __func__);
    }
    else {
        ALOGE("%s: ril_register_callback is not available.", __func__);
        ret = -1;
    }

    return ret;
}

void voice_deinit(struct voice_manager *voice)
{
    if (voice) {
        if (voice->rilc.handle)
            dlclose(voice->rilc.handle);

        free(voice);
    }

    return ;
}

struct voice_manager* voice_init(void)
{
    struct voice_manager *voice = NULL;
    char property[PROPERTY_VALUE_MAX];

    voice = calloc(1, sizeof(struct voice_manager));
    if (voice) {
        if (access(RIL_CLIENT_LIBPATH, R_OK) == 0) {
            voice->rilc.handle = dlopen(RIL_CLIENT_LIBPATH, RTLD_NOW);
            if (voice->rilc.handle) {
                voice->rilc.ril_open_client = (int (*)(void))dlsym(voice->rilc.handle, "Open");
                voice->rilc.ril_close_client = (int (*)(void))dlsym(voice->rilc.handle, "Close");
                voice->rilc.ril_register_callback = (int (*)(void *, int *))dlsym(voice->rilc.handle, "RegisterEventCallback");
                voice->rilc.ril_set_audio_volume = (int (*)(int))dlsym(voice->rilc.handle, "SetAudioVolume");
                voice->rilc.ril_set_audio_path = (int (*)(int))dlsym(voice->rilc.handle, "SetAudioPath");
                voice->rilc.ril_set_multi_mic = (int (*)(int))dlsym(voice->rilc.handle, "SetMultiMic");
                voice->rilc.ril_set_mute = (int (*)(int))dlsym(voice->rilc.handle, "SetMute");
                voice->rilc.ril_set_audio_clock = (int (*)(int))dlsym(voice->rilc.handle, "SetAudioClock");
                voice->rilc.ril_set_audio_loopback = (int (*)(int, int))dlsym(voice->rilc.handle, "SetAudioLoopback");

                ALOGD("%s: Successed to open SMI RIL Client Interface!", __func__);
            } else {
                ALOGE("%s: Failed to open SMI RIL Client Interface(%s)!", __func__, RIL_CLIENT_LIBPATH);
                goto open_err;
            }
        } else {
            ALOGE("%s: Failed to access SMI RIL Client Interface(%s)!", __func__, RIL_CLIENT_LIBPATH);
            goto open_err;
        }

        voice->state_call = false;
        voice->mode = VOICE_CALL_NONE;
        voice->state_mic_mute = false;

        property_get(VOLUME_STEPS_PROPERTY, property, VOLUME_STEPS_DEFAULT);
        voice->volume_steps_max = atoi(property);
        /* this catches the case where VOLUME_STEPS_PROPERTY does not contain an integer */
        if (voice->volume_steps_max == 0)
            voice->volume_steps_max = atoi(VOLUME_STEPS_DEFAULT);

        voice->callback = NULL;
    }

    return voice;

open_err:
    if (voice) {
        free(voice);
        voice = NULL;
    }

    return voice;
}
