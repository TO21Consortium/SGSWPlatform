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

int voice_set_mic_mute(struct voice_manager *voice, bool state)
{
    int ret = 0;

    voice->state_mic_mute = state;
    if (voice->state_call) {
        if (voice->rilc.ril_set_mute)
            voice->rilc.ril_set_mute(voice->rilc.client, (int)state);

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
    int ret = 0;

    if (voice->state_call) {
        if (voice->rilc.ril_set_volume)
            voice->rilc.ril_set_volume(voice->rilc.client, voice->rilc.sound_type, (int)(volume * voice->volume_steps_max));

        ALOGD("%s: Volume = %d(%f)!", __func__, (int)(volume * voice->volume_steps_max), volume);
    }

    return ret;
}

int voice_set_audio_clock(struct voice_manager *voice, enum ril_audio_clockmode clockmode)
{
    int ret = 0;

    if (voice->state_call) {
        if (voice->rilc.ril_set_clock_sync) {
            if (clockmode == VOICE_AUDIO_TURN_OFF_I2S) {
                voice->rilc.ril_set_clock_sync(voice->rilc.client, SOUND_CLOCK_STOP);
                ALOGD("%s: Sound Clock Stopped", __func__);
            }
            else if (clockmode == VOICE_AUDIO_TURN_ON_I2S) {
                voice->rilc.ril_set_clock_sync(voice->rilc.client, SOUND_CLOCK_START);
                ALOGD("%s: Sound Clock Started", __func__);
            }
        }
    }

    return ret;
}

static AudioPath map_incall_device(audio_devices_t devices)
{
    AudioPath device_type = SOUND_AUDIO_PATH_SPEAKER;

    switch(devices) {
    case AUDIO_DEVICE_OUT_EARPIECE:
        device_type = SOUND_AUDIO_PATH_HANDSET;
        break;
    case AUDIO_DEVICE_OUT_SPEAKER:
        device_type = SOUND_AUDIO_PATH_SPEAKER;
        break;
    case AUDIO_DEVICE_OUT_WIRED_HEADSET:
        device_type = SOUND_AUDIO_PATH_HEADSET;
        break;
    case AUDIO_DEVICE_OUT_WIRED_HEADPHONE:
        device_type = SOUND_AUDIO_PATH_HEADSET;
        break;
    default:
        device_type = SOUND_AUDIO_PATH_HANDSET;
        break;
    }

    return device_type;
}

static SoundType map_sound_fromdevice(AudioPath path)
{
    SoundType sound_type = SOUND_TYPE_VOICE;

    switch(path) {
    case SOUND_AUDIO_PATH_SPEAKER:
        sound_type = SOUND_TYPE_SPEAKER;
        break;
    case SOUND_AUDIO_PATH_HEADSET:
        sound_type = SOUND_TYPE_HEADSET;
        break;
    case SOUND_AUDIO_PATH_HANDSET:
    default:
        sound_type = SOUND_TYPE_VOICE;
        break;
    }

    return sound_type;
}


int voice_set_path(struct voice_manager *voice, audio_devices_t devices)
{
    int ret = 0;
    AudioPath path;

    if (voice->state_call) {
        /* Mapping */
        path = map_incall_device(devices);
        voice->rilc.sound_type = map_sound_fromdevice(path);

        if (voice->rilc.ril_set_audio_path) {
            ret = voice->rilc.ril_set_audio_path(voice->rilc.client, path, ORIGINAL_PATH);
            if (ret == 0)
                ALOGD("%s: Set Audio Path to %d!", __func__, path);
            else {
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
    int ret = -1;
    void *client = NULL;

    if (!voice->state_call) {
        if (voice->rilc.ril_open_client) {
            client = voice->rilc.ril_open_client();
            if (client) {
                if (voice->rilc.ril_is_connected(client))
                    ALOGD("%s: RIL Client is already connected with RIL", __func__);
                else if (voice->rilc.ril_connect(client)) {
                    ALOGD("%s: RIL Client cannot connect with RIL", __func__);
                    voice->rilc.ril_close_client(client);
                    return ret;
                }

                voice->rilc.client = client;
                voice->state_call = true;
                ALOGD("%s: Opened RIL Client, Transit to IN_CALL!", __func__);

                ret = 0;
            } else {
                ALOGE("%s: Failed to open RIL Client!", __func__);
            }
        } else {
            ALOGE("%s: ril_open_client is not available.", __func__);
        }
    }

    return ret;
}

int voice_close(struct voice_manager *voice)
{
    int ret = 0;

    if (voice->state_call) {
        if (voice->rilc.ril_close_client && voice->rilc.ril_disconnect) {
            if (voice->rilc.ril_is_connected(voice->rilc.client)) {
                voice->rilc.ril_disconnect(voice->rilc.client);
                ALOGD("%s: RIL Client dis-connect with RIL", __func__);
            }

            ret = voice->rilc.ril_close_client(voice->rilc.client);
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
                voice->rilc.ril_open_client = (int (*)(void))dlsym(voice->rilc.handle, "OpenClient_RILD");
                voice->rilc.ril_close_client = (int (*)(void))dlsym(voice->rilc.handle, "CloseClient_RILD");
                voice->rilc.ril_connect = (int (*)(void))dlsym(voice->rilc.handle, "Connect_RILD");
                voice->rilc.ril_is_connected = (int (*)(void))dlsym(voice->rilc.handle, "isConnected_RILD");
                voice->rilc.ril_disconnect = (int (*)(void))dlsym(voice->rilc.handle, "Disconnect_RILD");
                voice->rilc.ril_set_volume = (int (*)(int))dlsym(voice->rilc.handle, "SetCallVolume");
                voice->rilc.ril_set_audio_path = (int (*)(int))dlsym(voice->rilc.handle, "SetCallAudioPath");
                voice->rilc.ril_set_clock_sync = (int (*)(int))dlsym(voice->rilc.handle, "SetCallClockSync");
                voice->rilc.ril_set_mute = (int (*)(int))dlsym(voice->rilc.handle, "SetMute");

                ALOGD("%s: Successed to open SIPC RIL Client Interface!", __func__);
            } else {
                ALOGE("%s: Failed to open SIPC RIL Client Interface(%s)!", __func__, RIL_CLIENT_LIBPATH);
                goto open_err;
            }
        } else {
            ALOGE("%s: Failed to access SIPC RIL Client Interface(%s)!", __func__, RIL_CLIENT_LIBPATH);
            goto open_err;
        }

        voice->state_mic_mute = false;
        voice->mode = VOICE_CALL_NONE;
        voice->state_call = false;

        property_get(VOLUME_STEPS_PROPERTY, property, VOLUME_STEPS_DEFAULT);
        voice->volume_steps_max = atoi(property);
        /* this catches the case where VOLUME_STEPS_PROPERTY does not contain an integer */
        if (voice->volume_steps_max == 0)
            voice->volume_steps_max = atoi(VOLUME_STEPS_DEFAULT);
    }

    return voice;

open_err:
    if (voice) {
        free(voice);
        voice = NULL;
    }

    return voice;
}
