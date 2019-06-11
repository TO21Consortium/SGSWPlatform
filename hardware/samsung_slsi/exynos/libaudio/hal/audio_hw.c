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

#define LOG_TAG "audio_hw_primary"
//#define LOG_NDEBUG 0

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/prctl.h>

#include <cutils/log.h>
#include <cutils/str_parms.h>
#include <cutils/sched_policy.h>
#include <cutils/properties.h>

#include <system/thread_defs.h>

#include <audio_utils/primitives.h>

#include "audio_hw.h"
#include "audio_hw_def.h"

/******************************************************************************/
/** Note: the following macro is used for extremely verbose logging message. **/
/** In order to run with ALOG_ASSERT turned on, we need to have LOG_NDEBUG   **/
/** set to 0; but one side effect of this is to turn all LOGV's as well. Some**/
/** messages are so verbose that we want to suppress them even when we have  **/
/** ALOG_ASSERT turned on.  Do not uncomment the #def below unless you       **/
/** really know what you are doing and want to see all of the extremely      **/
/**  verbose messages.                                                       **/
/******************************************************************************/
//#define VERY_VERY_VERBOSE_LOGGING
#ifdef VERY_VERY_VERBOSE_LOGGING
#define ALOGVV ALOGD
#else
#define ALOGVV(a...) do { } while(0)
#endif

//#define ROUTING_VERBOSE_LOGGING
#ifdef ROUTING_VERBOSE_LOGGING
#define ALOGRV ALOGD
#else
#define ALOGRV(a...) do { } while(0)
#endif

/******************************************************************************/
/**                                                                          **/
/** The Global Local Functions                                               **/
/**                                                                          **/
/******************************************************************************/
static audio_format_t audio_pcmformat_from_alsaformat(enum pcm_format pcmformat)
{
    audio_format_t format = AUDIO_FORMAT_PCM_16_BIT;

    switch (pcmformat) {
        case PCM_FORMAT_S16_LE:
            format = AUDIO_FORMAT_PCM_16_BIT;
            break;
        case PCM_FORMAT_S32_LE:
            format = AUDIO_FORMAT_PCM_32_BIT;
            break;
        case PCM_FORMAT_S8:
            format = AUDIO_FORMAT_PCM_8_BIT;
            break;
        case PCM_FORMAT_S24_LE:
        case PCM_FORMAT_S24_3LE:
            format = AUDIO_FORMAT_PCM_24_BIT_PACKED;
            break;
        case PCM_FORMAT_INVALID:
        case PCM_FORMAT_MAX:
            format = AUDIO_FORMAT_PCM_16_BIT;
            break;
    }

    return format;
}

static unsigned int get_sound_card(audio_usage_id_t usage_id)
{
    return (unsigned int)(sound_device_table[usage_id][0]);
}

static unsigned int get_sound_device(audio_usage_id_t usage_id)
{
    return (unsigned int)(sound_device_table[usage_id][1]);
}

/* Only Primary Output Stream can control Voice Call */
static bool output_drives_call(struct audio_device *adev, struct stream_out *out)
{
    return out == adev->primary_output;
}

/* Check Call Mode or not */
static bool isCallMode(audio_usage_mode_t mode)
{
    if (mode > AUSAGE_MODE_NORMAL && mode < AUSAGE_MODE_NONE)
        return true;
    else
        return false;
}

/* CP Centric Call Mode or not */
static bool isCPCallMode(audio_usage_mode_t mode)
{
    if (mode == AUSAGE_MODE_VOICE_CALL || mode == AUSAGE_MODE_LTE_CALL)
        return true;
    else
        return false;
}

/******************************************************************************/
/**                                                                          **/
/** The Local Functions for Audio Usage list                                 **/
/**                                                                          **/
/******************************************************************************/
static struct exynos_audio_usage *get_dangling_ausage_from_list(
        struct audio_device *adev,
        audio_usage_type_t usagetype,
        audio_io_handle_t handle)
{
    struct exynos_audio_usage *ausage = NULL;
    struct exynos_audio_usage *active_ausage = NULL;
    struct listnode *ausage_node;

    list_for_each(ausage_node, &adev->audio_usage_list) {
        ausage = node_to_item(ausage_node, struct exynos_audio_usage, node);
        if (ausage->usage_type == usagetype) {
            if ((usagetype == AUSAGE_PLAYBACK)) {
                if (ausage->stream.out != NULL &&
                    ausage->stream.out->ausage == AUSAGE_PLAYBACK_COMPR_OFFLOAD &&
                    ausage->stream.out->handle != handle) {
                    active_ausage = ausage;
                    ALOGV("device-%s: [PLAYBACK] usage-id (%d) -- Active Routed device(%s) Mode(%s)!",
                        __func__, ausage->usage_id, device_path_table[ausage->out_device_id],
                        mode_table[ausage->out_device_amode]);
                    break;
                }
            }

            if (usagetype == AUSAGE_CAPTURE) {
                if (ausage->stream.in != NULL &&
                    ausage->stream.in->handle != handle) {
                    active_ausage = ausage;
                    ALOGV("device-%s: [CAPTURE] usage-id (%d) -- Active Routed device(%s) Mode(%s)!",
                        __func__, ausage->usage_id, device_path_table[ausage->in_device_id],
                        mode_table[ausage->in_device_amode]);
                    break;
                }
            }
        }
    }

    return active_ausage;
}

static struct exynos_audio_usage *get_active_ausage_from_list(
        struct audio_device *adev,
        struct exynos_audio_usage *me,
        audio_usage_type_t usagetype)
{
    struct exynos_audio_usage *ausage = NULL;
    struct exynos_audio_usage *active_ausage = NULL;
    struct listnode *ausage_node;

    list_for_each(ausage_node, &adev->audio_usage_list) {
        ausage = node_to_item(ausage_node, struct exynos_audio_usage, node);
        if (ausage->usage_type == usagetype) {
            if ((usagetype == AUSAGE_PLAYBACK)) {
                if (ausage != me && ausage->stream.out != NULL &&
                    ausage->stream.out->sstate != STATE_STANDBY) {
                    active_ausage = ausage;
                    ALOGV("device-%s: [PLAYBACK] usage-id (%d) -- Active Routed device(%s) Mode(%s)!",
                        __func__, ausage->usage_id, device_path_table[ausage->out_device_id],
                        mode_table[ausage->out_device_amode]);
                    break;
                }
            }

            if (usagetype == AUSAGE_CAPTURE) {
                if (ausage != me && ausage->stream.in != NULL &&
                    ausage->stream.in->sstate != STATE_STANDBY) {
                    active_ausage = ausage;
                    ALOGV("device-%s: [CAPTURE] usage-id (%d) -- Active Routed device(%s) Mode(%s)!",
                        __func__, ausage->usage_id, device_path_table[ausage->in_device_id],
                        mode_table[ausage->in_device_amode]);
                    break;
                }
            }
        }
    }

    return active_ausage;
}

/*
 * Return Audio Usage Structure matched with Usage Type, Usage ID and IO_Handle
 */
static struct exynos_audio_usage *get_ausage_from_list(
        struct audio_device *adev,
        audio_usage_type_t usagetype,
        audio_usage_id_t usage_id,
        audio_io_handle_t handle)
{
    struct exynos_audio_usage *ausage = NULL;
    struct listnode *ausage_node;
    audio_io_handle_t ausage_handle = 0;

    list_for_each(ausage_node, &adev->audio_usage_list) {
        ausage = node_to_item(ausage_node, struct exynos_audio_usage, node);

        if (ausage->usage_type == AUSAGE_PLAYBACK)
            ausage_handle = ausage->stream.out->handle;
        else if (ausage->usage_type == AUSAGE_CAPTURE)
            ausage_handle = ausage->stream.in->handle;

        if (ausage->usage_type == usagetype &&
            ausage->usage_id == usage_id &&
            ausage_handle == handle)
            return ausage;
    }

    return NULL;
}

static void syncup_ausage_from_list(
        struct audio_device *adev,
        struct exynos_audio_usage *me,
        audio_usage_type_t usagetype,
        device_type_t cur_device,
        device_type_t new_device)
{
    struct exynos_audio_usage *ausage;
    struct listnode *ausage_node;

    list_for_each(ausage_node, &adev->audio_usage_list) {
        ausage = node_to_item(ausage_node, struct exynos_audio_usage, node);
        if (ausage->usage_type == usagetype) {
            ALOGRV("device-%s: Usage(%s) current-device(%s)'s new-device(%s) usage_type(%s)!",
                __func__, usage_table[ausage->usage_id], device_path_table[cur_device],
                device_path_table[new_device], (ausage->usage_type == AUSAGE_PLAYBACK ? "PLAYBACK" : "CAPTURE"));
            if ((ausage->usage_type == AUSAGE_PLAYBACK)) {
                if (cur_device != DEVICE_NONE && new_device == DEVICE_NONE) {
                    if (ausage == me || (ausage->stream.out != NULL &&
                        ausage->stream.out->sstate == STATE_STANDBY)) {
                        ausage->out_device_id = DEVICE_NONE;
                        ausage->out_device_amode = AUSAGE_MODE_NONE;
                        ALOGRV("device-%s: Usage(%s) device(%s) Disabled!", __func__,
                            usage_table[ausage->usage_id], device_path_table[cur_device]);
                    }
                } else if (cur_device == DEVICE_NONE && new_device != DEVICE_NONE) {
                    if (ausage->stream.out != NULL) {
                        ausage->out_device_id = new_device;
                        ausage->out_device_amode = adev->usage_amode;
                        ALOGRV("device-%s: Usage(%s) device(%s) Enabled!", __func__,
                            usage_table[ausage->usage_id], device_path_table[new_device]);
                    }
                }
            }

            if ((ausage->usage_type == AUSAGE_CAPTURE)) {
                if (cur_device != DEVICE_NONE && new_device == DEVICE_NONE) {
                    if (ausage == me || (ausage->stream.in != NULL &&
                        ausage->stream.in->sstate == STATE_STANDBY)) {
                        ausage->in_device_id = DEVICE_NONE;
                        ausage->in_device_amode = AUSAGE_MODE_NONE;
                        ALOGRV("device-%s: Usage(%s) device(%s) Disabled!", __func__,
                            usage_table[ausage->usage_id], device_path_table[cur_device]);
                    }
                } else if (cur_device == DEVICE_NONE && new_device != DEVICE_NONE) {
                    if (ausage->stream.in != NULL) {
                        ausage->in_device_id = new_device;
                        ausage->in_device_amode = adev->usage_amode;
                        ALOGRV("device-%s: Usage(%s) device(%s) Enabled!", __func__,
                            usage_table[ausage->usage_id],device_path_table[new_device]);
                    }
                }
            }
        }
    }

    return ;
}

static int add_audio_usage(
        struct audio_device *adev,
        audio_usage_type_t type,
        void *stream)
{
    struct exynos_audio_usage *ausage_node;
    struct stream_out *out = NULL;
    struct stream_in *in = NULL;
    int ret = 0;

    ausage_node = (struct exynos_audio_usage *)calloc(1, sizeof(struct exynos_audio_usage));
    if (ausage_node) {
        ausage_node->usage_type = type;
        if (type == AUSAGE_PLAYBACK) {
            out = (struct stream_out *)stream;

            ausage_node->usage_id = out->ausage;
            ausage_node->stream.out = out;
        } else if (type == AUSAGE_CAPTURE) {
            in = (struct stream_in *)stream;

            ausage_node->usage_id = in->ausage;
            ausage_node->stream.in = in;
        }

        if (ausage_node->usage_id == AUSAGE_PLAYBACK_PRIMARY) {
            /* In case of Primary Playback Usage, it is created at first. */
            ausage_node->out_device_id = DEVICE_NONE;
            ausage_node->in_device_id = DEVICE_NONE;
            ausage_node->out_device_amode = AUSAGE_MODE_NONE;
            ausage_node->in_device_amode = AUSAGE_MODE_NONE;
        } else {
            /* Syncup with Primary Usage's Status */
            struct stream_out *primary_out = adev->primary_output;
            struct exynos_audio_usage *primary_usage;

            primary_usage = get_ausage_from_list(adev, AUSAGE_PLAYBACK, primary_out->ausage, primary_out->handle);
            if (primary_usage) {
                ausage_node->out_device_id = primary_usage->out_device_id;
                ausage_node->in_device_id = primary_usage->in_device_id;
                ausage_node->out_device_amode = primary_usage->out_device_amode;
                ausage_node->in_device_amode = primary_usage->in_device_amode;
            } else
                ALOGE("device-%s: There is no Primary Playback Usage!", __func__);
        }

        list_add_tail(&adev->audio_usage_list, &ausage_node->node);
        ALOGV("%s-%s: Added Audio Stream into Audio Usage list!", usage_table[ausage_node->usage_id], __func__);
    } else {
        ALOGE("device-%s: Failed to allocate Memory!", __func__);
        ret = -ENOMEM;
    }

    return ret;
}

static int remove_audio_usage(
        struct audio_device *adev,
        audio_usage_type_t type,
        void *stream)
{
    struct exynos_audio_usage *ausage_node = NULL;
    struct stream_out *out = NULL;
    struct stream_in *in = NULL;
    audio_usage_id_t id = AUSAGE_DEFAULT;
    audio_io_handle_t handle = 0;
    int ret = 0;

    if (type == AUSAGE_PLAYBACK) {
        out = (struct stream_out *)stream;
        id = out->ausage;
        handle = out->handle;
    } else if (type == AUSAGE_CAPTURE) {
        in = (struct stream_in *)stream;
        id = in->ausage;
        handle = in->handle;
    }

    ausage_node = get_ausage_from_list(adev, type, id, handle);
    if (ausage_node) {
        list_remove(&ausage_node->node);
        free(ausage_node);
        ALOGV("%s-%s: Removed Audio Usage from Audio Usage list!", usage_table[id], __func__);
    } else {
        ALOGV("%s-%s: There is no Audio Usage in Audio Usage list!", usage_table[id], __func__);
    }

    return ret;
}

#ifdef ROUTING_VERBOSE_LOGGING
static void print_ausage(struct audio_device *adev)
{
    struct exynos_audio_usage *ausage;
    struct listnode *ausage_node;

    list_for_each(ausage_node, &adev->audio_usage_list) {
        ausage = node_to_item(ausage_node, struct exynos_audio_usage, node);
        if (ausage->usage_type == AUSAGE_PLAYBACK)
            ALOGRV("%s-%s: Audio Mode = %s, Audio Device = %s", usage_table[ausage->usage_id],
            __func__, mode_table[ausage->out_device_amode], device_path_table[ausage->out_device_id]);
        else
            ALOGRV("%s-%s: Audio Mode = %s, Audio Device = %s", usage_table[ausage->usage_id],
            __func__, mode_table[ausage->in_device_amode], device_path_table[ausage->in_device_id]);
    }

    return ;

}
#endif

static void clean_dangling_streams(
        struct audio_device *adev,
        audio_usage_type_t type,
        void *stream)
{
    struct exynos_audio_usage *dangling_ausage = NULL;
    struct stream_out *out = NULL;
    struct stream_in *in = NULL;
    audio_usage_id_t id = AUSAGE_DEFAULT;
    audio_io_handle_t handle = 0;

    if (type == AUSAGE_CAPTURE) {
        in = (struct stream_in *)stream;
        id = in->ausage;
        handle = in->handle;
    }

    do {
        dangling_ausage = get_dangling_ausage_from_list(adev, type, handle);
        if (dangling_ausage) {
            if (type == AUSAGE_CAPTURE) {
                struct stream_in *dangling_in = (struct stream_in *)dangling_ausage->stream.in;

                remove_audio_usage(adev, type, (void *)dangling_in);

                pthread_mutex_destroy(&dangling_in->lock);
                free(dangling_in);
            }
#if 0
            else if (type == AUSAGE_PLAYBACK && id == AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
                struct stream_out *dangling_out = (struct stream_out *)dangling_ausage->stream.out;

                remove_audio_usage(adev, type, (void *)dangling_out);

                if (dangling_out->nonblock_flag)
                    destroy_offload_callback_thread(dangling_out);

                if (dangling_out->comprconfig.codec != NULL) {
                    free(dangling_out->comprconfig.codec);
                    dangling_out->comprconfig.codec = NULL;
                }

                pthread_mutex_destroy(&dangling_out->lock);
                free(dangling_out);
            }
#endif
        }
    } while (dangling_ausage);

    return ;
}

/******************************************************************************/
/**                                                                          **/
/** The Local Functions for Audio Path Routing                               **/
/**                                                                          **/
/******************************************************************************/

/* Load mixer_path.xml, open Control Device and initialize Mixer structure */
static bool init_route(struct audio_device *adev)
{
    struct route_info *trinfo = NULL;
    struct audio_route *ar = NULL;
    const char *card_name = NULL;
    int i, ret_stat = 0;

    /* Open Mixer & Initialize Route Path */
    trinfo = (struct route_info *)calloc(1, sizeof(struct route_info));
    if (trinfo) {
        /* We are using only one Sound Card */
        adev->mixerinfo = mixer_open(SOUND_CARD0);
        if (adev->mixerinfo) {
            ar = audio_route_init(SOUND_CARD0, NULL);
            if (!ar) {
                /* Fail to open Mixer or init route */
                ALOGE("device-%s: Failed to init audio route with Mixer(%d)!", __func__, SOUND_CARD0);
                mixer_close(adev->mixerinfo);
                adev->mixerinfo = NULL;
                free(trinfo);
                return false;
            }
            trinfo->card_num = (unsigned int)SOUND_CARD0;
            trinfo->aroute = ar;
        } else {
            ALOGE("device-%s: Cannot open Mixer for %d!", __func__, SOUND_CARD0);
            free(trinfo);
            return false;
        }
    } else {
        ALOGE("device-%s: Failed to allocate memory for audio route!", __func__);
        return false;
    }

    /* Set Route Info */
    adev->rinfo = trinfo;

    return true;
}

/* Free Mixer structure and close Control Device */
static void deinit_route(struct audio_device *adev)
{
    struct route_info *trinfo = adev->rinfo;
    struct audio_route *ar = trinfo->aroute;

    if (trinfo && ar) {
        adev->vol_ctrl = NULL;

        audio_route_free(ar);
        if (adev->mixerinfo) {
            mixer_close(adev->mixerinfo);
            adev->mixerinfo = NULL;
        }

        free(trinfo);
        adev->rinfo = NULL;
    }

    return;
}

static audio_usage_mode_t get_usage_mode(
        struct audio_device *adev,
        audio_mode_t plat_amode)
{
    audio_usage_mode_t  usage_amode = AUSAGE_MODE_NONE;

    switch (plat_amode) {
        case AUDIO_MODE_NORMAL:
        case AUDIO_MODE_RINGTONE:
            usage_amode = AUSAGE_MODE_NORMAL;
            if (adev->call_state != CALL_OFF)
                ALOGE("device-%s: Abnormal Call State from Normal Mode!", __func__);
            break;
        case AUDIO_MODE_IN_CALL:
            switch (adev->call_state) {
                case CP_CALL:
                    usage_amode = AUSAGE_MODE_VOICE_CALL;
                    break;
                case LTE_CALL:
                    usage_amode = AUSAGE_MODE_LTE_CALL;
                    break;
                case WIFI_CALL:
                    usage_amode = AUSAGE_MODE_WIFI_CALL;
                    break;
                case CALL_OFF:
                    usage_amode = AUSAGE_MODE_NORMAL;
                    ALOGE("device-%s: Abnormal Call State from InCall Mode!", __func__);
                    break;
            }
            break;
        case AUDIO_MODE_IN_COMMUNICATION:
            usage_amode = AUSAGE_MODE_VOIP_CALL;
            if (adev->call_state != CALL_OFF)
                ALOGE("device-%s: Abnormal Call State from Communication Mode!", __func__);
            break;
        default:
            usage_amode = AUSAGE_MODE_NORMAL;
    }

    return usage_amode;
}

static void make_path_name(
        audio_usage_mode_t path_amode,
        device_type_t device,
        char *path_name)
{
    memset(path_name, 0, MAX_PATH_NAME_LEN);

    strlcpy(path_name, mode_path_table[path_amode], MAX_PATH_NAME_LEN);
    strlcat(path_name, "-", MAX_PATH_NAME_LEN);
    strlcat(path_name, device_path_table[device], MAX_PATH_NAME_LEN);

    return ;
}

static void do_set_route(
        struct audio_device *adev,
        audio_usage_id_t usage_id,
        device_type_t device,
        audio_usage_mode_t path_amode,
        bool set)
{
    struct audio_route *ar = adev->rinfo->aroute;
    char path_name[MAX_PATH_NAME_LEN];

    make_path_name(path_amode, device, path_name);
    if (set)
        if (audio_route_apply_and_update_path(ar, path_name) < 0)
            ALOGE("%s-%s: Failed to enable Audio Route(%s)", usage_table[usage_id], __func__, path_name);
        else
            ALOGD("%s-%s: Enabled Audio Route(%s)", usage_table[usage_id], __func__, path_name);
    else
        if (audio_route_reset_and_update_path(ar, path_name) < 0)
            ALOGE("%s-%s: Failed to disable Audio Route(%s)", usage_table[usage_id], __func__, path_name);
        else
            ALOGD("%s-%s: Disabled Audio Route(%s)", usage_table[usage_id], __func__, path_name);

    return ;
}

static int set_audio_route(
        void *stream,
        audio_usage_type_t usage_type,
        audio_usage_id_t usage_id,
        bool set)
{
    struct audio_device *adev = NULL;
    struct stream_out *stream_out = NULL;
    struct stream_in *stream_in = NULL;

    struct audio_route *ar = NULL;
    struct exynos_audio_usage *ausage = NULL;
    device_type_t cur_device = DEVICE_NONE;
    device_type_t new_device = DEVICE_NONE;
    audio_usage_mode_t cur_dev_amode = AUSAGE_MODE_NONE;
    char cur_pathname[MAX_PATH_NAME_LEN] = "media-none";
    char new_pathname[MAX_PATH_NAME_LEN] = "media-none";
    bool disable_cur_device = false;
    bool enable_new_device = false;
    audio_io_handle_t handle = 0;

    int ret = 0;

    if (usage_type == AUSAGE_PLAYBACK) {
        stream_out = (struct stream_out *)stream;
        adev = stream_out->adev;
        handle = stream_out->handle;
    }
    else if (usage_type == AUSAGE_CAPTURE) {
        stream_in = (struct stream_in *)stream;
        adev = stream_in->adev;
        handle = stream_in->handle;
    }
    ar = adev->rinfo->aroute;

    /* Get usage pointer from the list */
    ausage = get_ausage_from_list(adev, usage_type, usage_id, handle);
    if (ausage) {
        /* Get current Mode & routed device information based on usage type */
        // Usage Structure has current device information
        // Usage Audio Mode has current mode information
        // Stream Structure has new device information
        if (ausage->usage_type == AUSAGE_PLAYBACK) {
            cur_device = ausage->out_device_id;
            cur_dev_amode = ausage->out_device_amode;
            new_device = get_device_id(ausage->stream.out->devices);
        } else if (ausage->usage_type == AUSAGE_CAPTURE) {
            cur_device = ausage->in_device_id;
            cur_dev_amode = ausage->in_device_amode;
            new_device = get_device_id(ausage->stream.in->devices);
        }
        ALOGD("%s-%s: [%s] current-device(%s) new-device(%s) in Mode(%s)!", usage_table[usage_id], __func__,
            (ausage->usage_type == AUSAGE_PLAYBACK ? "PLAYBACK" : "CAPTURE"),
            device_path_table[cur_device], device_path_table[new_device], mode_table[cur_dev_amode]);

        /* get audio route pathname for both current & new devices*/
        if (cur_device != DEVICE_NONE) {
            make_path_name(cur_dev_amode, cur_device, cur_pathname);
            ALOGRV("%s-%s: Current routed pathname: %s", usage_table[usage_id], __func__, cur_pathname);
        }

        if (new_device != DEVICE_NONE) {
            make_path_name(adev->usage_amode, new_device, new_pathname);
            ALOGRV("%s-%s: New route pathname:  %s", usage_table[usage_id], __func__, new_pathname);
        }

        /* Handle request for disabling/enabling current/new routing */
        if (!set) {
            /* Case: Disable Audio Path */
            ALOGRV("%s-%s: Disable audio device(%s)!",
                    usage_table[usage_id], __func__, device_path_table[cur_device]);
            if (get_active_ausage_from_list(adev, ausage, ausage->usage_type)
                || isCallMode(adev->usage_amode)) {
                ALOGD("%s-%s: Current device(%s) is still in use by other usage!",
                    usage_table[usage_id], __func__, device_path_table[cur_device]);
            } else {
                if (cur_device != DEVICE_NONE)
                    disable_cur_device = true;
            }
        } else {
            /* 1: Requested route is already set up */
            if (new_device == cur_device &&
                !strcmp(new_pathname, cur_pathname)) {
                ALOGD("%s-%s: Request Audio Route [%s] is already setup", usage_table[usage_id], __func__, new_pathname);
            } else {
                /* need disable current and route new device */
                if (cur_device != DEVICE_NONE)
                    disable_cur_device = true;
                enable_new_device = true;
            }
        }

        /* Disable current device and if it is output corresponding input device
           should be disabled if active */
        if (disable_cur_device) {
            if (ausage->usage_type == AUSAGE_PLAYBACK) {
                struct exynos_audio_usage *active_ausage;
                if (cur_device != DEVICE_NONE) {
                    /* Disable current routed path */
                    do_set_route(adev, usage_id, cur_device, cur_dev_amode, false);
                    syncup_ausage_from_list(adev, ausage, ausage->usage_type, cur_device, DEVICE_NONE);

                    /* check whether input device is active */
                    active_ausage = get_active_ausage_from_list(adev, NULL, AUSAGE_CAPTURE);
                    if (active_ausage || isCallMode(adev->usage_amode)||
                                         isCallMode(cur_dev_amode)) {
                        device_type_t in_device = DEVICE_NONE;
                        device_type_t in_cur_device = DEVICE_NONE;  /* Input device matching with disabled output device */
                        audio_usage_mode_t in_dev_amode = AUSAGE_MODE_NONE;
                        audio_usage_id_t in_usage_id = AUSAGE_DEFAULT;
                        char in_active_pathname[MAX_PATH_NAME_LEN] = "media-none";
                        char in_cur_pathname[MAX_PATH_NAME_LEN] = "media-none";

                        in_cur_device = get_indevice_id_from_outdevice(cur_device);

                        /*get current routed in-device information */
                        if (active_ausage) {
                            in_device = active_ausage->in_device_id;  // Usage Structure has current device information
                            in_dev_amode = active_ausage->in_device_amode;
                            in_usage_id = active_ausage->usage_id;
                            if (in_device != DEVICE_NONE) {
                                make_path_name(in_dev_amode, in_device, in_active_pathname);
                                ALOGRV("%s-%s: Input active routed pathname: %s", usage_table[in_usage_id], __func__, in_active_pathname);
                            }
                            if (in_cur_device != DEVICE_NONE) {
                                make_path_name(adev->usage_amode, in_cur_device, in_cur_pathname);
                                ALOGRV("%s-%s: Input active routed pathname: %s", usage_table[in_usage_id], __func__, in_active_pathname);
                            }
                        } else {
                            in_device = in_cur_device;
                            in_dev_amode = cur_dev_amode;
                            in_usage_id = usage_id;
                        }

                        ALOGRV("%s-%s: Disable Active Input Device if required cur_in(%s) out-in(%s) cur-routed-mode(%s) new-mode(%s)",
                            usage_table[usage_id], __func__, device_path_table[in_device],
                            device_path_table[get_indevice_id_from_outdevice(cur_device)],
                            mode_table[in_dev_amode], mode_table[adev->usage_amode]);
                        if ((active_ausage && (active_ausage->stream.in->source == AUDIO_SOURCE_CAMCORDER ||
                                                            !strcmp(in_active_pathname, in_cur_pathname))) ) {
                            ALOGD("%s-%s: Skip disabling Current Active input Device(%s)",
                                usage_table[usage_id], __func__, device_path_table[in_device]);
                        } else {
                            ALOGD("%s-%s: Current Active input Device is Disabled (%s)",
                                usage_table[usage_id], __func__, device_path_table[in_device]);
                            if (in_device != DEVICE_NONE) {
                                /* Disable current routed in-device path */
                                do_set_route(adev, in_usage_id, in_device, in_dev_amode, false);
                                syncup_ausage_from_list(adev, active_ausage, AUSAGE_CAPTURE, in_device, DEVICE_NONE);
                            }
                        }
                    }
                }
            } else {
                if (cur_device != DEVICE_NONE) {
                    /* Disable current routed input path */
                    ALOGD("%s-%s: Capture Device is Disabled (%s)", usage_table[usage_id],
                        __func__, device_path_table[cur_device]);
                    do_set_route(adev, usage_id, cur_device, cur_dev_amode, false);
                    syncup_ausage_from_list(adev, ausage, ausage->usage_type, cur_device, DEVICE_NONE);
                }
            }
        }

        /* Enable new device and if it is output corresponding input device
             should be enabled if active */
        if (enable_new_device) {
            if (ausage->usage_type == AUSAGE_PLAYBACK) {
                struct exynos_audio_usage *active_ausage;
                device_type_t in_device = DEVICE_NONE;

                if (new_device != DEVICE_NONE) {
                    /* Enable new device routed path */
                    do_set_route(adev, usage_id, new_device, adev->usage_amode, true);
                    syncup_ausage_from_list(adev, ausage, ausage->usage_type, DEVICE_NONE, new_device);

                    /* check whether input device is active */
                    active_ausage = get_active_ausage_from_list(adev, NULL, AUSAGE_CAPTURE);
                    if (active_ausage || isCallMode(adev->usage_amode)) {
                        device_type_t in_device = DEVICE_NONE;
                        audio_usage_id_t in_usage_id = AUSAGE_DEFAULT;

                        ALOGD("%s-%s: ENABLE --Input Device for active usage ", usage_table[usage_id], __func__);
                        in_device = get_indevice_id_from_outdevice(new_device);

                        /*get current routed in-device information */
                        if (active_ausage) {
                            in_usage_id = active_ausage->usage_id;
                        } else {
                            in_usage_id = usage_id;
                        }

                        if ((active_ausage && active_ausage->in_device_id == DEVICE_NONE)
                                || isCallMode(adev->usage_amode)) {
                            ALOGD("%s-%s: Input Device Enabled (%s)", usage_table[usage_id],
                                __func__, device_path_table[in_device]);
                            /* Enable new in-device routed path */
                            do_set_route(adev, in_usage_id, in_device, adev->usage_amode, true);
                            syncup_ausage_from_list(adev, active_ausage, AUSAGE_CAPTURE, DEVICE_NONE, in_device);
                        }
                    }
                }
            } else {
                if (new_device != DEVICE_NONE) {
                    /* Enable new input device routed path */
                    ALOGD("%s-%s: Capture Device is Enabled (%s)", usage_table[usage_id],
                        __func__, device_path_table[new_device]);
                    do_set_route(adev, usage_id, new_device, adev->usage_amode, true);
                    syncup_ausage_from_list(adev, ausage, ausage->usage_type, DEVICE_NONE, new_device);
                }
            }

        }
    } else {
        ALOGW("%s-%s: Cannot find it from Audio Usage list!", usage_table[usage_id], __func__);
        ret = -EINVAL;
    }

#ifdef ROUTING_VERBOSE_LOGGING
    print_ausage(adev);
#endif

    return ret;
}

/*****************************************************************************/
/*                                                                           */
/* Local Functions for BT-SCO support                                        */
/*                                                                           */
/*****************************************************************************/
static int do_start_bt_sco(struct stream_out *out)
{
    struct audio_device *adev = out->adev;
    unsigned int sound_card;
    unsigned int sound_device;
    struct pcm_config pcmconfig;
    int ret = 0;

    // Initialize BT-Sco sound card and device information
    sound_card = BT_SCO_SOUND_CARD;
    sound_device = BT_SCO_SOUND_DEVICE;

    pcmconfig = pcm_config_bt_sco;

    if (adev->pcm_btsco_out == NULL) {
        /* Open BT-SCO Output  */
        adev->pcm_btsco_out = pcm_open(sound_card, sound_device, PCM_OUT, &pcmconfig);
        if (adev->pcm_btsco_out && !pcm_is_ready(adev->pcm_btsco_out)) {
            ALOGE("BT SCO-%s: Output Device is not ready(%s)!", __func__, pcm_get_error(adev->pcm_btsco_out));
            ret = -EBADF;
            goto err_out;
        }
        ALOGI("BT SCO-%s: PCM output device open Success!", __func__);
        /* Start PCM Device */
        pcm_start(adev->pcm_btsco_out);
        ALOGI("BT SCO-%s: PCM output device is started!", __func__);
    }

    if (adev->pcm_btsco_in == NULL) {
        /* Open BT-SCO Input */
        adev->pcm_btsco_in = pcm_open(sound_card, sound_device, PCM_IN, &pcmconfig);
        if (adev->pcm_btsco_in && !pcm_is_ready(adev->pcm_btsco_in)) {
            ALOGE("BT SCO-%s: Input Device is not ready(%s)!", __func__, pcm_get_error(adev->pcm_btsco_in));
            ret = -EBADF;
            goto err_in;
        }
        ALOGI("BT SCO-%s: PCM input device open Success!", __func__);

        /* Start PCM Device */
        pcm_start(adev->pcm_btsco_in);
        ALOGI("BT SCO-%s: PCM input device is started!", __func__);
    }

    return ret;

err_in:
    if (adev->pcm_btsco_in) {
        pcm_close(adev->pcm_btsco_in);
        adev->pcm_btsco_in = NULL;
        ALOGI("BT SCO-%s: PCM input device is closed!", __func__);
    }

err_out:
    if (adev->pcm_btsco_out) {
        pcm_close(adev->pcm_btsco_out);
        adev->pcm_btsco_out = NULL;
        ALOGI("BT SCO-%s: PCM output device is closed!", __func__);
    }

    return ret;
}

static int do_stop_bt_sco(struct audio_device *adev)
{
    if (adev->pcm_btsco_in) {
        pcm_stop(adev->pcm_btsco_in);
        pcm_close(adev->pcm_btsco_in);
        adev->pcm_btsco_in = NULL;
        ALOGI("BT SCO-%s: PCM input device is stopped & closed!", __func__);
    }

    if (adev->pcm_btsco_out) {
        pcm_stop(adev->pcm_btsco_out);
        pcm_close(adev->pcm_btsco_out);
        adev->pcm_btsco_out = NULL;
        ALOGI("BT SCO-%s: PCM output device is stopped & closed!", __func__);
    }

    return 0;
}

/*****************************************************************************/
/*                                                                           */
/* Local Functions for Voice Call                                            */
/*                                                                           */
/*****************************************************************************/
static int do_start_voice_call(struct stream_out *out)
{
    struct audio_device *adev = out->adev;
    unsigned int sound_card;
    unsigned int sound_device;
    struct pcm_config pcmconfig;
    int ret = -EBADF;

    // Hard coded for test
    sound_card = VOICE_CALL_SOUND_CARD;
    sound_device = VOICE_CALL_SOUND_DEVICE;

    pcmconfig = pcm_config_vc_wb;

    if (out->devices & AUDIO_DEVICE_OUT_ALL_SCO) {
        do_start_bt_sco(out);
    }

    if (isCPCallMode(adev->usage_amode) && voice_is_in_call(adev->voice)) {
        if (adev->pcm_voice_tx == NULL) {
            /* Open Voice TX(Output) */
            adev->pcm_voice_tx = pcm_open(sound_card, sound_device, PCM_OUT, &pcmconfig);
            if (adev->pcm_voice_tx && !pcm_is_ready(adev->pcm_voice_tx)) {
                ALOGE("Voice Call-%s: PCM TX Device is not ready(%s)!", __func__, pcm_get_error(adev->pcm_voice_tx));
                goto err_tx;
            }
            ALOGI("Voice Call-%s: Success to open PCM TX Device!", __func__);
        }

        if (adev->pcm_voice_rx == NULL) {
            /* Open Voice RX(Input) */
            adev->pcm_voice_rx = pcm_open(sound_card, sound_device, PCM_IN, &pcmconfig);
            if (adev->pcm_voice_rx && !pcm_is_ready(adev->pcm_voice_rx)) {
                ALOGE("Voice Call-%s: PCM RX Device is not ready(%s)!", __func__, pcm_get_error(adev->pcm_voice_rx));
                goto err_rx;
            }
            ALOGI("Voice Call-%s: Success to open PCM RX Device!", __func__);
        }

        /* Start All Devices */
        pcm_start(adev->pcm_voice_tx);
        pcm_start(adev->pcm_voice_rx);
        ALOGI("Voice Call-%s: Started PCM RX & TX Devices!", __func__);
        ret = 0;
    }

    return ret;

err_rx:
    if (adev->pcm_voice_rx) {
        pcm_close(adev->pcm_voice_rx);
        adev->pcm_voice_rx = NULL;
        ALOGI("Voice Call-%s: PCM RX Device is closed!", __func__);
    }

err_tx:
    if (adev->pcm_voice_tx) {
        pcm_close(adev->pcm_voice_tx);
        adev->pcm_voice_tx = NULL;
        ALOGI("Voice Call-%s: PCM TX Device is closed!", __func__);
    }

    return ret;
}

static int do_stop_voice_call(struct audio_device *adev)
{
    if (adev->pcm_btsco_in || adev->pcm_btsco_out) {
        /* close BT-SCO pcm first */
        do_stop_bt_sco(adev);
    }

    if (adev->pcm_voice_rx) {
        pcm_stop(adev->pcm_voice_rx);
        pcm_close(adev->pcm_voice_rx);
        adev->pcm_voice_rx = NULL;
        ALOGI("Voice Call-%s: Stopped & Closed PCM RX Devices!", __func__);
    }

    if (adev->pcm_voice_tx) {
        pcm_stop(adev->pcm_voice_tx);
        pcm_close(adev->pcm_voice_tx);
        adev->pcm_voice_tx = NULL;
        ALOGI("Voice Call-%s: Stopped & Closed PCM TX Devices!", __func__);
    }

    return 0;
}


/*****************************************************************************/
/* Local Functions for Playback Stream */

/* must be called with out->lock locked */
static int send_offload_msg(struct stream_out *out, offload_msg_type_t msg)
{
    struct exynos_offload_msg *msg_node = NULL;
    int ret = 0;

    msg_node = (struct exynos_offload_msg *)calloc(1, sizeof(struct exynos_offload_msg));
    if (msg_node) {
        msg_node->msg = msg;

        list_add_tail(&out->offload_msg_list, &msg_node->node);
        pthread_cond_signal(&out->offload_msg_cond);

        ALOGV("offload_out-%s: Sent Message = %s", __func__, offload_msg_table[msg]);
    } else {
        ALOGE("offload_out-%s: Failed to allocate memory for Offload MSG", __func__);
        ret = -ENOMEM;
    }

    return ret;
}

/* must be called with out->lock locked */
static offload_msg_type_t recv_offload_msg(struct stream_out *out)
{
    struct listnode *offload_msg_list = &(out->offload_msg_list);

    struct listnode *head = list_head(offload_msg_list);
    struct exynos_offload_msg *msg_node = node_to_item(head, struct exynos_offload_msg, node);
    offload_msg_type_t msg = msg_node->msg;

    list_remove(head);
    free(msg_node);

    ALOGV("offload_out-%s: Received Message = %s", __func__, offload_msg_table[msg]);
    return msg;
}

static int do_set_volume(struct stream_out *out, float left, float right)
{
    struct audio_device *adev = out->adev;
    struct mixer_ctl *ctrl;
    int ret = -ENAVAIL;
    int val[2];

    ctrl = mixer_get_ctl_by_name(adev->mixerinfo, OFFLOAD_VOLUME_CONTROL_NAME);
    if (ctrl) {
        val[0] = (int)(left * COMPRESS_PLAYBACK_VOLUME_MAX);
        val[1] = (int)(right * COMPRESS_PLAYBACK_VOLUME_MAX);
        ret = mixer_ctl_set_array(ctrl, val, sizeof(val)/sizeof(val[0]));
        if (ret != 0)
            ALOGE("%s-%s: Fail to set Volume", usage_table[out->ausage], __func__);
        else
            ALOGD("%s-%s: Set Volume(%f:%f) => (%d:%d)", usage_table[out->ausage], __func__, left, right, val[0], val[1]);
    } else {
        ALOGE("%s-%s: Cannot find volume controller", usage_table[out->ausage], __func__);
    }

    return ret;
}

static int do_close_output_stream(struct stream_out *out)
{
    struct audio_device *adev = out->adev;
    int ret = 0;

    /* Close PCM/Compress Device */
    if (out->ausage != AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
        /* In cases of low_latency, deep_buffer and Aux_Digital usage, It needs to pcm_close() */
        if (out->pcminfo) {
            ret = pcm_close(out->pcminfo);
            out->pcminfo = NULL;
        }
        ALOGI("%s-%s: Closed PCM Device", usage_table[out->ausage], __func__);
    } else {
        /* In cases of compress_offload usage, It needs to compress_close() */
        if (out->comprinfo) {
            compress_close(out->comprinfo);
            out->comprinfo = NULL;
        }
        ALOGI("%s-%s: Closed Compress Device", usage_table[out->ausage], __func__);
    }

    /* Reset Routing Path */
    /* Have to keep devices information to restart without calling set_parameter() */
    set_audio_route((void *)out, AUSAGE_PLAYBACK, out->ausage, false);

    return ret;
}

static int do_open_output_stream(struct stream_out *out)
{
    struct audio_device *adev = out->adev;
    unsigned int sound_card;
    unsigned int sound_device;
    unsigned int flags;
    int ret = 0;
    char fn[256];

    /* Set Routing Path */
    //set_audio_route((void *)out, AUSAGE_PLAYBACK, out->ausage, true);

    /* Open PCM/Compress Device */
    sound_card = get_sound_card(out->ausage);
    sound_device = get_sound_device(out->ausage);

    if (out->ausage != AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
        if (!out->pcminfo) {
            /* In cases of low_latency, deep_buffer and Aux_Digital usage, It needs to pcm_open() */
            flags = PCM_OUT | PCM_MONOTONIC;

            out->pcminfo = pcm_open(sound_card, sound_device, flags, &out->pcmconfig);
            if (out->pcminfo && !pcm_is_ready(out->pcminfo)) {
                /* pcm_open does always return pcm structure, not NULL */
                ALOGE("%s-%s: PCM Device is not ready(%s)!", usage_table[out->ausage], __func__, pcm_get_error(out->pcminfo));
                goto err_open;
            }

            //ALOGI("Refined PCM Period Size = %u, Period Count = %u", out->pcmconfig.period_size, out->pcmconfig.period_count);
            snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%u%c", sound_card, sound_device, flags & PCM_IN ? 'c' : 'p');
            ALOGI("%s-%s: Opened PCM Device is %s", usage_table[out->ausage], __func__, fn);

            out->comprinfo = NULL;
        }
    } else {
        if (!out->comprinfo) {
            /* In cases of compress_offload usage, It needs to compress_open() */
            flags = COMPRESS_IN;

            out->comprinfo = compress_open(sound_card, sound_device, flags, &out->comprconfig);
            if (out->comprinfo && !is_compress_ready(out->comprinfo)) {
                /* compress_open does always return compress structure, not NULL */
                ALOGE("%s-%s: Compress Device is not ready(%s)!", usage_table[out->ausage], __func__, compress_get_error(out->comprinfo));
                goto err_open;
            }

            //ALOGI("Refined Compress Fragment Size = %u, Fragments = %u", out->comprconfig.fragment_size, out->comprconfig.fragments);
            snprintf(fn, sizeof(fn), "/dev/snd/comprC%uD%u", sound_card, sound_device);
            ALOGI("%s-%s: Opened Compress Device is %s", usage_table[out->ausage], __func__, fn);

            out->pcminfo = NULL;

            /* Set Volume */
            do_set_volume(out, out->vol_left, out->vol_right);
        }
    }

    return ret;

err_open:
    do_close_output_stream(out);
    ret = -EINVAL;
    return ret;
}

static int do_start_output_stream(struct stream_out *out)
{
    struct audio_device *adev = out->adev;
    int ret = -ENOSYS;

    if (out->ausage != AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
#if 0
        if (out->pcminfo) {
            ret = pcm_start(out->pcminfo);
            if (ret == 0)
                ALOGI("%s-%s: Started PCM Device", usage_table[out->ausage], __func__);
            else
                ALOGE("%s-%s: Cannot start PCM(%s)", usage_table[out->ausage], __func__, pcm_get_error(out->pcminfo));
        }
#endif
        ret = 0;
    } else {
        if (out->comprinfo) {
            if (out->nonblock_flag && out->offload_callback) {
                compress_nonblock(out->comprinfo, out->nonblock_flag);
                ALOGD("%s-%s: Set Nonblock mode!", usage_table[out->ausage], __func__);
            } else {
                compress_nonblock(out->comprinfo, 0);
                ALOGD("%s-%s: Set Block mode!", usage_table[out->ausage], __func__);
            }

            ret = compress_start(out->comprinfo);
            if (ret == 0)
                ALOGI("%s-%s: Started Compress Device", usage_table[out->ausage], __func__);
            else
                ALOGE("%s-%s: Cannot start Compress Offload(%s)", usage_table[out->ausage], __func__, compress_get_error(out->comprinfo));

            /* Notify Offload playback started to VisualizerHAL */
            if (adev->notify_start_output_tovisualizer != NULL) {
                adev->notify_start_output_tovisualizer(out->handle);
                ALOGI("%s-%s: Notify Start to VisualizerHAL", usage_table[out->ausage], __func__);
            }
        }
    }

    return ret;
}

static int do_stop_output_stream(struct stream_out *out, bool force_stop)
{
    struct audio_device *adev = out->adev;
    int ret = -ENOSYS;

    if (out->ausage != AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
#if 0
        if (out->pcminfo) {
            ret = pcm_stop(out->pcminfo);
            if (ret == 0)
                ALOGD("%s-%s: Stopped PCM Device", usage_table[out->ausage], __func__);
            else
                ALOGE("%s-%s: Cannot stop PCM(%s)", usage_table[out->ausage], __func__, pcm_get_error(out->pcminfo));
        }
#endif
        ret = 0;
    } else {
        if (out->comprinfo) {
            /* Check Offload_Callback_Thread is blocked & wait to finish the action */
            if (!force_stop) {
                if (out->callback_thread_blocked) {
                    ALOGV("%s-%s: Waiting Offload Callback Thread is done", usage_table[out->ausage], __func__);
                    pthread_cond_wait(&out->offload_sync_cond, &out->lock);
                }
            }

            /* Notify Offload playback stopped to VisualizerHAL */
            if (adev->notify_stop_output_tovisualizer != NULL) {
                adev->notify_stop_output_tovisualizer(out->handle);
                ALOGI("%s-%s: Notify Stop to VisualizerHAL", usage_table[out->ausage], __func__);
            }

            ret = compress_stop(out->comprinfo);
            if (ret == 0)
                ALOGD("%s-%s: Stopped Compress Device", usage_table[out->ausage], __func__);
            else
                ALOGE("%s-%s: Cannot stop Compress Offload(%s)", usage_table[out->ausage], __func__, compress_get_error(out->comprinfo));

            out->ready_new_metadata = 1;
        }
    }

    return ret;
}

static int do_write_buffer(struct stream_out *out, const void* buffer, size_t bytes)
{
    int ret = 0, wrote = 0;

    if (out->ausage != AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
        /* In cases of low_latency, deep_buffer and Aux_Digital usage, It needs to pcm_write() */
        if (out->pcminfo) {
            ret = pcm_write(out->pcminfo, (void *)buffer, bytes);
            if (ret == 0) {
                ALOGVV("%s-%s: Write Success(%u bytes) to PCM Device", usage_table[out->ausage], __func__, (unsigned int)bytes);
                out->written += bytes / (out->pcmconfig.channels * sizeof(short)); // convert to frame unit
                ALOGVV("%s-%s: Written = %u frames", usage_table[out->ausage], __func__, (unsigned int)out->written);
                wrote = bytes;
            } else {
                wrote = ret;
                ALOGE_IF(out->err_count < MAX_ERR_COUNT, "%s-%s: Write Fail = %s",
                    usage_table[out->ausage], __func__, pcm_get_error(out->pcminfo));
                out->err_count++;
            }
        }
    } else {
        /* In case of compress_offload usage, It needs to compress_write() */
        if (out->comprinfo) {
            if (out->ready_new_metadata) {
                compress_set_gapless_metadata(out->comprinfo, &out->offload_metadata);
                ALOGD("%s-%s: Sent gapless metadata(delay = %u, padding = %u) to Compress Device",
                    usage_table[out->ausage], __func__, out->offload_metadata.encoder_delay,
                    out->offload_metadata.encoder_padding);
                out->ready_new_metadata = 0;
            }

            wrote = compress_write(out->comprinfo, buffer, bytes);
            ALOGVV("%s-%s: Write Request(%u bytes) to Compress Device, and Accepted (%u bytes)", usage_table[out->ausage], __func__, (unsigned int)bytes, wrote);
            if (wrote < 0) {
                ALOGE_IF(out->err_count < MAX_ERR_COUNT, "%s-%s: Error playing sample with Code(%s)", usage_table[out->ausage], __func__, compress_get_error(out->comprinfo));
                out->err_count++;
            } else if (wrote >= 0 && wrote < (ssize_t)bytes) {
                /* Compress Device has no available buffer, we have to wait */
                ALOGV("%s-%s: There are no available buffer in Compress Device, Need to wait", usage_table[out->ausage], __func__);
                ret = send_offload_msg(out, OFFLOAD_MSG_WAIT_WRITE);
            }
        }
    }

    return wrote;
}

/* Compress Offload Specific Functions */
static bool is_supported_compressed_format(audio_format_t format)
{
    switch (format & AUDIO_FORMAT_MAIN_MASK) {
    case AUDIO_FORMAT_MP3:
    case AUDIO_FORMAT_AAC:
        return true;
    default:
        break;
    }

    return false;
}

static int get_snd_codec_id(audio_format_t format)
{
    int id = 0;

    switch (format & AUDIO_FORMAT_MAIN_MASK) {
    case AUDIO_FORMAT_MP3:
        id = SND_AUDIOCODEC_MP3;
        break;
    case AUDIO_FORMAT_AAC:
        id = SND_AUDIOCODEC_AAC;
        break;
    default:
        ALOGE("%s: Unsupported audio format", __func__);
    }

    return id;
}

static void *offload_cbthread_loop(void *context)
{
    struct stream_out *out = (struct stream_out *) context;
    bool get_exit = false;
    int ret = 0;

    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_AUDIO);
    set_sched_policy(0, SP_FOREGROUND);
    prctl(PR_SET_NAME, (unsigned long)"Offload Callback", 0, 0, 0);

    ALOGD("%s-%s: Started running Offload Callback Thread", usage_table[out->ausage], __func__);

    pthread_mutex_lock(&out->lock);
    do {
        offload_msg_type_t msg = OFFLOAD_MSG_INVALID;
        stream_callback_event_t event;
        bool need_callback = false;

        if (list_empty(&out->offload_msg_list)) {
            ALOGV("%s-%s: transit to sleep", usage_table[out->ausage], __func__);
            pthread_cond_wait(&out->offload_msg_cond, &out->lock);
            ALOGV("%s-%s: transit to wake-up", usage_table[out->ausage], __func__);
        }

        if (!list_empty(&out->offload_msg_list))
            msg = recv_offload_msg(out);

        if (msg == OFFLOAD_MSG_EXIT) {
            get_exit = true;
            continue;
        }

        out->callback_thread_blocked = true;
        pthread_mutex_unlock(&out->lock);

        switch (msg) {
        case OFFLOAD_MSG_WAIT_WRITE:
            if (out->comprinfo) {
                ret = compress_wait(out->comprinfo, -1);
                if (ret) {
                    if (out->comprinfo)
                        ALOGE("Error - compress_wait return %s", compress_get_error(out->comprinfo));
                    else
                        ALOGE("Error - compress_wait return, but compress device already closed");
                }
            }

            need_callback = true;
            event = STREAM_CBK_EVENT_WRITE_READY;
            break;

        case OFFLOAD_MSG_WAIT_PARTIAL_DRAIN:
            if (out->comprinfo) {
                ret = compress_next_track(out->comprinfo);
                if (ret) {
                    if (out->comprinfo)
                        ALOGE("Error - compress_next_track return %s", compress_get_error(out->comprinfo));
                    else
                        ALOGE("Error - compress_next_track return, but compress device already closed");
                }
            }

            if (out->comprinfo) {
                ret = compress_partial_drain(out->comprinfo);
                if (ret) {
                    if (out->comprinfo)
                        ALOGE("Error - compress_partial_drain return %s", compress_get_error(out->comprinfo));
                    else
                        ALOGE("Error - compress_partial_drain return, but compress device already closed");
                }
            }

            need_callback = true;
            event = STREAM_CBK_EVENT_DRAIN_READY;

            /* Resend the metadata for next iteration */
            out->ready_new_metadata = 1;
            break;

        case OFFLOAD_MSG_WAIT_DRAIN:
            if (out->comprinfo) {
                ret = compress_drain(out->comprinfo);
                if (ret) {
                    if (out->comprinfo)
                        ALOGE("Error - compress_drain return %s", compress_get_error(out->comprinfo));
                    else
                        ALOGE("Error - compress_drain return, but compress device already closed");
                }
            }

            need_callback = true;
            event = STREAM_CBK_EVENT_DRAIN_READY;
            break;

        default:
            ALOGE("Invalid message = %u", msg);
            break;
        }

        pthread_mutex_lock(&out->lock);
        out->callback_thread_blocked = false;
        pthread_cond_signal(&out->offload_sync_cond);

        if (need_callback) {
            out->offload_callback(event, NULL, out->offload_cookie);
            if (event == STREAM_CBK_EVENT_DRAIN_READY)
                ALOGD("%s-%s: Callback to Platform with %d", usage_table[out->ausage], __func__, event);
        }
    } while(!get_exit);

    /* Clean the message list */
    pthread_cond_signal(&out->offload_sync_cond);
    while(!list_empty(&out->offload_msg_list))
        recv_offload_msg(out);
    pthread_mutex_unlock(&out->lock);

    ALOGD("%s-%s: Stopped running Offload Callback Thread", usage_table[out->ausage], __func__);
    return NULL;
}

static int create_offload_callback_thread(struct stream_out *out)
{
    pthread_cond_init(&out->offload_msg_cond, (const pthread_condattr_t *) NULL);
    pthread_cond_init(&out->offload_sync_cond, (const pthread_condattr_t *) NULL);

    pthread_create(&out->offload_callback_thread, (const pthread_attr_t *) NULL, offload_cbthread_loop, out);
    out->callback_thread_blocked = false;

    return 0;
}

static int destroy_offload_callback_thread(struct stream_out *out)
{
    int ret = 0;

    pthread_mutex_lock(&out->lock);
    if (out->sstate != STATE_IDLE) {
        /* Stop stream & transit to Idle */
        ALOGD("%s-%s: compress offload stream is running, will stop!", usage_table[out->ausage], __func__);
        do_stop_output_stream(out, false);
        out->sstate = STATE_IDLE;
        ALOGI("%s-%s: Transit to Idle", usage_table[out->ausage], __func__);
    }
    ret = send_offload_msg(out, OFFLOAD_MSG_EXIT);
    pthread_mutex_unlock(&out->lock);

    pthread_join(out->offload_callback_thread, (void **) NULL);
    ALOGD("%s-%s: Joined Offload Callback Thread!", usage_table[out->ausage], __func__);

    pthread_cond_destroy(&out->offload_sync_cond);
    pthread_cond_destroy(&out->offload_msg_cond);

    return 0;
}


static void check_and_set_pcm_config(
        struct pcm_config *pcmconfig,
        struct audio_config *config)
{
    /* Need to check which values are selected when requested configuration
         is different with default configuration */

    ALOGI("Check PCM Channel Count: Default(%d), Request(%d)",
                pcmconfig->channels, audio_channel_count_from_out_mask(config->channel_mask));
    pcmconfig->channels = audio_channel_count_from_out_mask(config->channel_mask);

    ALOGI("Check PCM Smapling Rate: Default(%d), Request(%d)", pcmconfig->rate, config->sample_rate);
    pcmconfig->rate = config->sample_rate;

    ALOGI("Check PCM Format: Default(%d), Request(%d)",
                pcmconfig->format, pcm_format_from_audio_format(config->format));
    pcmconfig->format = pcm_format_from_audio_format(config->format);

    return;
}

static void amplify(short *pcm_buf, size_t frames)
{
    char value[PROPERTY_VALUE_MAX];
    /* NOTICE
    * Beware of clipping.
    * Too much volume can cause unrecognizable sound.
    */
    if (property_get("ro.fm_record_volume", value, "1.0")) {
        float volume;
        if (1 == sscanf(value, "%f", &volume)) {
            while(frames--) {
                *pcm_buf = clamp16((int32_t)(*pcm_buf * volume));
                pcm_buf++;
                *pcm_buf = clamp16((int32_t)(*pcm_buf * volume));
                pcm_buf++;
            }
        }
    }
}

/*****************************************************************************/
/* Local Functions for Capture Stream */
static int do_close_input_stream(struct stream_in *in)
{
    struct audio_device *adev = in->adev;
    int ret = 0;

    /* Close PCM Device */
    if (in->ausage == AUSAGE_CAPTURE_LOW_LATENCY) {
        /* In cases of low_latency usage, It needs to pcm_close() */
        if (in->pcminfo) {
            ret = pcm_close(in->pcminfo);
            if (adev->pcm_capture == in->pcminfo)
                adev->pcm_capture = NULL;
            in->pcminfo = NULL;
        }
        ALOGI("%s-%s: Closed PCM Device", usage_table[in->ausage], __func__);
    } else {
        /* In cases of Error */
        ALOGE("%s-%s: Invalid Usage", usage_table[in->ausage], __func__);
        ret = -EINVAL;
    }

    /* Reset Routing Path */
    /* Have to keep devices information to restart without calling set_parameter() */
    set_audio_route((void *)in, AUSAGE_CAPTURE, in->ausage, false);

    return ret;
}

static int do_open_input_stream(struct stream_in *in)
{
    struct audio_device *adev = in->adev;
    unsigned int sound_card;
    unsigned int sound_device;
    unsigned int flags;
    int ret = 0;
    char fn[256];

    /* Set Routing Path */
    //set_audio_route((void *)in, AUSAGE_CAPTURE, in->ausage, true);

    /* Check Unique PCM Device */
    if (adev->pcm_capture) {
        ALOGW("%s-%s: PCM Device for Capture is already opened!!!", usage_table[in->ausage], __func__);
        clean_dangling_streams(adev, AUSAGE_CAPTURE, (void *)in);
        pcm_close(adev->pcm_capture);
        adev->pcm_capture = NULL;
    }

    /* Open PCM Device */
    sound_card = get_sound_card(in->ausage);
    sound_device = get_sound_device(in->ausage);

    if (in->ausage == AUSAGE_CAPTURE_LOW_LATENCY) {
        if (!in->pcminfo) {
            /* In cases of low_latency usage, It needs to pcm_open() */
            flags = PCM_IN;

            in->pcminfo = pcm_open(sound_card, sound_device, flags, &in->pcmconfig);
            if (in->pcminfo && !pcm_is_ready(in->pcminfo)) {
                ALOGE("%s-%s: PCM Device is not ready(%s)!", usage_table[in->ausage], __func__, pcm_get_error(in->pcminfo));
                goto err_open;
            }

            ALOGVV("%s: Refined PCM Period Size = %u, Period Count = %u", __func__, in->pcmconfig.period_size, in->pcmconfig.period_count);
            snprintf(fn, sizeof(fn), "/dev/snd/pcmC%uD%u%c", sound_card, sound_device, flags & PCM_IN ? 'c' : 'p');
            ALOGI("%s-%s: Opened PCM Device is %s", usage_table[in->ausage], __func__, fn);

            adev->pcm_capture = in->pcminfo;
        }
    } else {
        /* In cases of Error */
        ALOGE("%s-%s: Invalid Usage", usage_table[in->ausage], __func__);
        in->pcminfo = NULL;
        ret = -EINVAL;
    }

    return ret;

err_open:
    do_close_input_stream(in);
    ret = -EINVAL;
    return ret;
}

static int do_start_input_stream(struct stream_in *in)
{
    int ret = -ENOSYS;

    if (in->ausage == AUSAGE_CAPTURE_LOW_LATENCY) {
        if (in->pcminfo) {
            ret = pcm_start(in->pcminfo);
            if (ret == 0)
                ALOGI("%s-%s: Started PCM Device", usage_table[in->ausage], __func__);
            else
                ALOGE("%s-%s: Cannot start PCM(%s)", usage_table[in->ausage], __func__, pcm_get_error(in->pcminfo));
        }
    } else {
        /* In cases of Error */
        ALOGE("%s-%s: Invalid Usage", usage_table[in->ausage], __func__);
        ret = -EINVAL;
    }

    return ret;
}

static int do_stop_input_stream(struct stream_in *in)
{
    int ret = -ENOSYS;

    if (in->ausage == AUSAGE_CAPTURE_LOW_LATENCY) {
        if (in->pcminfo) {
            ret = pcm_stop(in->pcminfo);
            if (ret == 0)
                ALOGI("%s-%s: Stopped PCM Device", usage_table[in->ausage], __func__);
            else
                ALOGE("%s-%s: Cannot stop PCM(%s)", usage_table[in->ausage], __func__, pcm_get_error(in->pcminfo));
        }
    } else {
        /* In cases of Error */
        ALOGE("%s-%s: Invalid Usage", usage_table[in->ausage], __func__);
        ret = -EINVAL;
    }

    return ret;
}

static int do_read_buffer(struct stream_in *in, void* buffer, size_t bytes)
{
    int ret = 0, read = 0;

    if (in->ausage == AUSAGE_CAPTURE_LOW_LATENCY) {
        if (in->pcminfo) {
            ret = pcm_read(in->pcminfo, (void*)buffer, (unsigned int)bytes);
            if (ret == 0) {
                ALOGVV("%s-%s: Read Success(%u bytes) from PCM Device", usage_table[in->ausage], __func__, (unsigned int)bytes);
                read = bytes;
                if (in->devices == AUDIO_DEVICE_IN_FM_TUNER) {
                    amplify((short *) buffer, bytes / 4);
                }
            } else {
                read = ret;
                ALOGE_IF(in->err_count < MAX_ERR_COUNT, "%s-%s: Read Fail = %s",
                    usage_table[in->ausage], __func__, pcm_get_error(in->pcminfo));
                in->err_count++;
            }
        }
    }

    return read;
}

static void check_pcm_config(
        struct pcm_config *pcmconfig,
        struct audio_config *config)
{
    // Need to check which values are selected when requested configuration is different with default configuration
    ALOGI("%s: Check PCM Channel Count: Default(%d), Request(%d)", __func__, pcmconfig->channels, audio_channel_count_from_out_mask(config->channel_mask));
    ALOGI("%s: Check PCM Smapling Rate: Default(%d), Request(%d)", __func__, pcmconfig->rate, config->sample_rate);
    ALOGI("%s: Check PCM Format: Default(%d), Request(%d)", __func__, pcmconfig->format, pcm_format_from_audio_format(config->format));

    return ;
}

static int check_input_parameters(
        uint32_t sample_rate,
        audio_format_t format,
        int channel_count)
{
    if (format != AUDIO_FORMAT_PCM_16_BIT)
        return -EINVAL;

    if (channel_count != 2)
        return -EINVAL;

    if (sample_rate != 48000)
        return -EINVAL;

    return 0;
}

static size_t get_input_buffer_size(uint32_t sample_rate, int channel_count)
{
    size_t size = 0;

    size = (sample_rate * AUDIO_CAPTURE_PERIOD_DURATION_MSEC) / 1000;// Number of Sample during duration
    size *= sizeof(short) * channel_count; // Always 16bit PCM, so 2Bytes data(Short Size)

    /* make sure the size is multiple of 32 bytes at 48 kHz mono 16-bit PCM:
     *  5.000 ms => 240 samples => 15*16*1*2 = 480 Bytes, a whole multiple of 32 (15)
     *  3.333 ms => 160 samples => 10*16*1*2 = 320 Bytes, a whole multiple of 32 (10)
     */
    size += 0x1f;
    size &= ~0x1f;

    return size;
}

/****************************************************************************/

/****************************************************************************/
/**                                                                        **/
/** The Stream_Out Function Implementation                                 **/
/**                                                                        **/
/****************************************************************************/
static uint32_t out_get_sample_rate(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGV("%s-%s: exit with sample rate = %u", usage_table[out->ausage], __func__, out->sample_rate);
    return out->sample_rate;
}

static int out_set_sample_rate(
        struct audio_stream *stream __unused,
        uint32_t rate __unused)
{
    return -ENOSYS;
}

static size_t out_get_buffer_size(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    if (out->ausage != AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
        /* Total Buffer Size in Kernel = period_size * period_count * number of bytes per sample(frame) */
        ALOGV("%s-%s: Period Size = %u, Frame Size = %u", usage_table[out->ausage], __func__,
            out->pcmconfig.period_size, (unsigned int)audio_stream_out_frame_size((const struct audio_stream_out *)stream));
        return out->pcmconfig.period_size * audio_stream_out_frame_size((const struct audio_stream_out *)stream);
    } else {
        /* Total Buffer Size in Kernel is fixed 4K * 5ea */
        ALOGV("%s-%s: Fragment Size = %u", usage_table[out->ausage], __func__, COMPRESS_PLAYBACK_BUFFER_SIZE);
        return COMPRESS_PLAYBACK_BUFFER_SIZE;
    }

    return 4096;
}

static audio_channel_mask_t out_get_channels(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGV("%s-%s: exit with channel mask = 0x%x", usage_table[out->ausage], __func__, out->channel_mask);
    return out->channel_mask;
}

static audio_format_t out_get_format(const struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGV("%s-%s: exit with audio format = 0x%x", usage_table[out->ausage], __func__, out->format);
    return out->format;
}

static int out_set_format(
        struct audio_stream *stream __unused,
        audio_format_t format __unused)
{
    return -ENOSYS;
}

static int out_standby(struct audio_stream *stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->adev;

    ALOGV("%s-%s: enter", usage_table[out->ausage], __func__);

    pthread_mutex_lock(&out->lock);
    if (out->sstate != STATE_STANDBY) {
        /* Stop stream & transit to Idle */
        if (out->sstate != STATE_IDLE) {
            ALOGV("%s-%s: stream is running, will stop!", usage_table[out->ausage], __func__);
            do_stop_output_stream(out, false);
            out->sstate = STATE_IDLE;
            ALOGI("%s-%s: Transit to Idle", usage_table[out->ausage], __func__);
        }

        /* Close device & transit to Standby */
        pthread_mutex_lock(&adev->lock);
        do_close_output_stream(out);
        pthread_mutex_unlock(&adev->lock);
        out->sstate = STATE_STANDBY;
        ALOGI("%s-%s: Transit to Standby", usage_table[out->ausage], __func__);
    }
    out->err_count = 0;
    pthread_mutex_unlock(&out->lock);

    ALOGV("%s-%s: exit", usage_table[out->ausage], __func__);
    return 0;
}

static int out_dump(const struct audio_stream *stream, int fd)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGV("%s-%s: enit with fd(%d)", usage_table[out->ausage], __func__, fd);
    return 0;
}

static int out_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->adev;
    struct str_parms *parms;
    int ret = 0, process_count = 0;
    char value[32];

    ALOGD("%s-%s: enter with param = %s", usage_table[out->ausage], __func__, kvpairs);

    parms = str_parms_create_str(kvpairs);

    pthread_mutex_lock(&out->lock);
    if (out->ausage == AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
        struct compr_gapless_mdata tmp_mdata;
        bool need_to_set_metadata = false;

        tmp_mdata.encoder_delay = 0;
        tmp_mdata.encoder_padding = 0;

        /* These parameters are sended from sendMetaDataToHal() in Util.cpp when openAudioSink() is called */
        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_AVG_BIT_RATE, value, sizeof(value));
        if (ret >= 0) {
            unsigned int bit_rate = atoi(value);
            if (out->comprconfig.codec->bit_rate == bit_rate)
                ALOGI("%s: Requested same bit rate(%u)", __func__, bit_rate);

            process_count++;
        }

        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_SAMPLE_RATE, value, sizeof(value));
        if (ret >= 0) {
            unsigned int sample_rate = atoi(value);
            if (out->sample_rate == sample_rate)
                ALOGI("%s: Requested same sample rate(%u)", __func__, sample_rate);

            process_count++;
        }

        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_NUM_CHANNEL, value, sizeof(value));
        if (ret >= 0) {
            unsigned int num_ch = atoi(value);
            if (out->channel_mask== num_ch)
                ALOGI("%s: Requested same Number of Channel(%u)", __func__, num_ch);

            process_count++;
        }

        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_DELAY_SAMPLES, value, sizeof(value));
        if (ret >= 0) {
            tmp_mdata.encoder_delay = atoi(value);
            ALOGI("%s: Codec Delay Samples(%u)", __func__, tmp_mdata.encoder_delay);
            need_to_set_metadata = true;

            process_count++;
        }

        ret = str_parms_get_str(parms, AUDIO_OFFLOAD_CODEC_PADDING_SAMPLES, value, sizeof(value));
        if (ret >= 0) {
            tmp_mdata.encoder_padding = atoi(value);
            ALOGI("%s: Codec Padding Samples(%u)", __func__, tmp_mdata.encoder_padding);
            need_to_set_metadata = true;

            process_count++;
        }

        if (need_to_set_metadata) {
            out->offload_metadata = tmp_mdata;
            out->ready_new_metadata = 1;
        }
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        audio_devices_t req_device;

        /* AudioFlinger wants to change Audio Routing to some device */
        req_device = atoi(value);
        if (req_device != AUDIO_DEVICE_NONE) {
            /* Start/stop the BT SCO PCM nodes */
            if ((req_device & AUDIO_DEVICE_OUT_ALL_SCO) ^
                (out->devices & AUDIO_DEVICE_OUT_ALL_SCO)) {
                if (req_device & AUDIO_DEVICE_OUT_ALL_SCO)
                    do_start_bt_sco(out);
                else
                    do_stop_bt_sco(adev);
            }

            ALOGD("%s-%s: Requested to change route from 0x%X to 0x%X",
                usage_table[out->ausage], __func__, out->devices, req_device);
            out->devices = req_device;

            /* Route the new device as requested by framework */
            pthread_mutex_lock(&adev->lock);
            set_audio_route((void *)out, AUSAGE_PLAYBACK, out->ausage, true);
            pthread_mutex_unlock(&adev->lock);

            if (output_drives_call(adev, out)) {
                /* This output stream can be handled routing for voice call */
                if (adev->voice) {
                    if (!voice_is_in_call(adev->voice)) {
                        /* Check Audio Mode. If it is In-Call Mode, have to start voice call */
                        if (isCPCallMode(adev->usage_amode)) {
                            /* Start Call */
                            ret = voice_open(adev->voice);
                            if (ret == 0) {
                                adev->usage_amode = get_usage_mode(adev, adev->amode);
                                ALOGD("device-%s: Platform mode(%d) configured HAL mode(%s)", __func__, adev->amode, mode_table[adev->usage_amode]);
                                if (adev->usage_amode == AUSAGE_MODE_VOICE_CALL)
                                    voice_set_call_mode(adev->voice, VOICE_CALL_CS);
                                else if (adev->usage_amode == AUSAGE_MODE_LTE_CALL)
                                    voice_set_call_mode(adev->voice, VOICE_CALL_PS);
                                voice_set_path(adev->voice, out->devices);

                                do_start_voice_call(out);
                                voice_set_volume(adev->voice, adev->voice_volume);
                                ALOGD("%s-%s: *** Started CP Voice Call with voice_volume:%f ***",
                                    usage_table[out->ausage],__func__, adev->voice_volume);
                            } else {
                                ALOGE("%s-%s: Failed to open Voice Client", usage_table[out->ausage], __func__);
                            }
                        }
                    } else {
                        if (isCPCallMode(adev->usage_amode)) {
                            /* Now, In-Call State. Let's update RIL regarding output change */
                            adev->usage_amode = get_usage_mode(adev, adev->amode);
                            ALOGD("device-%s: Platform mode(%d) configured HAL mode(%s)", __func__, adev->amode, mode_table[adev->usage_amode]);
                            if (adev->usage_amode == AUSAGE_MODE_VOICE_CALL)
                                voice_set_call_mode(adev->voice, VOICE_CALL_CS);
                            else if (adev->usage_amode == AUSAGE_MODE_LTE_CALL)
                                voice_set_call_mode(adev->voice, VOICE_CALL_PS);

                            voice_set_path(adev->voice, out->devices);
                            ALOGD("%s-%s: RIL Route Updated for 0x%X", usage_table[out->ausage], __func__, out->devices);
                        }
                    }
                }
            }
        } else {
            /* When audio device will be changed, AudioFlinger requests to route with AUDIO_DEVICE_NONE */
            ALOGV("%s-%s: Requested to change route to AUDIO_DEVICE_NONE", usage_table[out->ausage], __func__);
        }

        process_count++;
    }
    pthread_mutex_unlock(&out->lock);

    if (process_count == 0)
        ALOGW("%s-%s: Not Supported param!", usage_table[out->ausage], __func__);

    str_parms_destroy(parms);
    ALOGD("%s-%s: exit", usage_table[out->ausage], __func__);
    return 0;
}

static char * out_get_parameters(const struct audio_stream *stream, const char *keys)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGD("%s-%s: enter with param = %s", usage_table[out->ausage], __func__, keys);
    return strdup("");
}

static uint32_t out_get_latency(const struct audio_stream_out *stream)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGV("%s-%s: enter", usage_table[out->ausage], __func__);

    if (out->ausage != AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
        /* Basic latency(ms unit)  is the delay in kernel buffer = (period * count * 1,000) / sample_rate */
        /* Do we need to add platform latency from DMA to real device(speaker) */
        return (out->pcmconfig.period_count * out->pcmconfig.period_size * 1000) / (out->pcmconfig.rate);
    } else {
        /* In case of Compress Offload, need to check it */
        return 100;
    }

    return 0;
}

static int out_set_volume(
        struct audio_stream_out *stream,
        float left,
        float right)
{
    struct stream_out *out = (struct stream_out *)stream;
    int ret = -ENOSYS;

    ALOGV("%s-%s: enter", usage_table[out->ausage], __func__);

    if (out->ausage == AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
        /* Save requested volume level to use at start */
        out->vol_left = left;
        out->vol_right = right;

        ret = do_set_volume(out, left, right);
    } else {
        ALOGE("%s-%s: Don't support volume control for this stream", usage_table[out->ausage], __func__);
    }

    ALOGV("%s-%s: exit", usage_table[out->ausage], __func__);
    return ret;
}

static ssize_t out_write(
        struct audio_stream_out *stream,
        const void* buffer,
        size_t bytes)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->adev;
    int ret = 0, wrote = 0;

    ALOGVV("%s-%s: enter", usage_table[out->ausage], __func__);

    pthread_mutex_lock(&out->lock);
    /* Check Device is opened */
    if (out->sstate == STATE_STANDBY) {
        pthread_mutex_lock(&adev->lock);
        ret = do_open_output_stream(out);
        if (ret != 0) {
            ALOGE("%s-%s: Fail to open Output Stream!", usage_table[out->ausage], __func__);
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_unlock(&out->lock);
            return wrote;
        } else {
            out->sstate = STATE_IDLE;
            ALOGI("%s-%s: Transit to Idle", usage_table[out->ausage], __func__);
        }
        pthread_mutex_unlock(&adev->lock);
    }

    /* Transfer data before start */
    if (out->sstate > STATE_STANDBY) {
        wrote = do_write_buffer(out, buffer, bytes);
        if (wrote >= 0) {
            if (out->sstate == STATE_IDLE) {
                /* Start stream & Transit to Playing */
                ret = do_start_output_stream(out);
                if (ret != 0) {
                    ALOGE("%s-%s: Fail to start Output Stream!", usage_table[out->ausage], __func__);
                } else {
                    out->sstate = STATE_PLAYING;
                    ALOGI("%s-%s: Transit to Playing", usage_table[out->ausage], __func__);
                }
            }
        }
    }
    pthread_mutex_unlock(&out->lock);

    ALOGVV("Playback Stream(%d) %s: exit", out->ausage, __func__);
    return wrote;
}

static int out_get_render_position(
        const struct audio_stream_out *stream,
        uint32_t *dsp_frames)
{
    struct stream_out *out = (struct stream_out *)stream;
    unsigned int sample_rate = 0;
    int ret = -ENAVAIL;

    ALOGVV("Playback Stream(%d) %s: enter", out->ausage, __func__);

    pthread_mutex_lock(&out->lock);
    if ((out->ausage == AUSAGE_PLAYBACK_COMPR_OFFLOAD) && (dsp_frames != NULL)) {
        *dsp_frames = 0;
        if (out->comprinfo) {
            ret = compress_get_tstamp(out->comprinfo, (unsigned long *)dsp_frames, &sample_rate);
            if (ret < 0) {
                ALOGV("%s-%s: Error is %s", usage_table[out->ausage], __func__, compress_get_error(out->comprinfo));
            } else {
                ALOGVV("%s-%s: rendered frames %u with sample_rate %u", usage_table[out->ausage], __func__, *dsp_frames, sample_rate);
                ret = 0;
            }
        }
    } else {
        ALOGE("%s-%s: PCM is not support yet!", usage_table[out->ausage], __func__);
    }
    pthread_mutex_unlock(&out->lock);

    ALOGVV("Playback Stream(%d) %s: exit", out->ausage, __func__);
    return ret;
}

static int out_add_audio_effect(
        const struct audio_stream *stream,
        effect_handle_t effect)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGD("%s: exit with effect(%p)", __func__, effect);
    return 0;
}

static int out_remove_audio_effect(
        const struct audio_stream *stream,
        effect_handle_t effect)
{
    struct stream_out *out = (struct stream_out *)stream;

    ALOGD("%s: exit with effect(%p)", __func__, effect);
    return 0;
}

static int out_get_next_write_timestamp(
        const struct audio_stream_out *stream,
        int64_t *timestamp)
{
    struct stream_out *out = (struct stream_out *)stream;

    *timestamp = 0;

//    ALOGV("%s: exit", __func__);
    return -EINVAL;
}

static int out_get_presentation_position(
        const struct audio_stream_out *stream,
        uint64_t *frames,
        struct timespec *timestamp)
{
    struct stream_out *out = (struct stream_out *)stream;
    unsigned int sample_rate = 0;
    unsigned int avail = 0;
    unsigned long hw_frames;
    int ret = -ENAVAIL;

    ALOGVV("%s-%s: entered", usage_table[out->ausage], __func__);

    if (frames != NULL) {
        *frames = 0;

        pthread_mutex_lock(&out->lock);
        if (out->ausage == AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
            if (out->comprinfo) {
                ret = compress_get_tstamp(out->comprinfo, &hw_frames, &sample_rate);
                if (ret < 0) {
                    ALOGV_IF(out->err_count < MAX_ERR_COUNT, "%s-%s: Error is %s", usage_table[out->ausage], __func__, compress_get_error(out->comprinfo));
                    out->err_count++;
                } else {
                    ALOGV("%s-%s: rendered frames %lu with sample_rate %u", usage_table[out->ausage], __func__, hw_frames, sample_rate);
                    *frames = hw_frames;

                    clock_gettime(CLOCK_MONOTONIC, timestamp);
                    ret = 0;
                }
            } else {
                ALOGE("%s-%s: Compress Device is not opended yet", usage_table[out->ausage], __func__);
            }
        } else {
            if (out->pcminfo) {
                // Need to check again
                ret = pcm_get_htimestamp(out->pcminfo, &avail, timestamp);
                if (ret < 0) {
                    ALOGV_IF(out->err_count < MAX_ERR_COUNT, "%s-%s: Error is %d", usage_table[out->ausage], __func__, ret);
                    out->err_count++;
                } else {
                    uint64_t kernel_buffer_size = (uint64_t)out->pcmconfig.period_size * (uint64_t)out->pcmconfig.period_count;  // Total Frame Count in kernel Buffer
                    int64_t signed_frames = out->written - kernel_buffer_size + avail;

                    ALOGVV("%s-%s: %lld frames are rendered", usage_table[out->ausage], __func__, (long long)signed_frames);

                    if (signed_frames >= 0) {
                        *frames = (uint64_t)signed_frames;
                        ret = 0;
                    }
                }
            } else {
                ALOGE("%s-%s: PCM Device is not opended yet", usage_table[out->ausage], __func__);
            }
        }
        pthread_mutex_unlock(&out->lock);
    } else {
        ALOGE("%s-%s: Invalid Parameter with Null pointer parameter", usage_table[out->ausage], __func__);
    }

    ALOGVV("%s-%s: exit", usage_table[out->ausage], __func__);
    return ret;
}

static int out_set_callback(
        struct audio_stream_out *stream,
        stream_callback_t callback,
        void *cookie)
{
    struct stream_out *out = (struct stream_out *)stream;
    int ret = -EINVAL;

    ALOGD("%s-%s: entered", usage_table[out->ausage], __func__);

    pthread_mutex_lock(&out->lock);
    if (out->ausage == AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
        if (callback && cookie) {
            out->offload_callback = callback;
            out->offload_cookie = cookie;
            ret = 0;

            ALOGD("%s-%s: set callback function & cookie", usage_table[out->ausage], __func__);
        }
    }
    pthread_mutex_unlock(&out->lock);

    ALOGD("%s-%s: exit", usage_table[out->ausage], __func__);
    return ret;
}

static int out_pause(struct audio_stream_out* stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->adev;
    int ret = -ENOSYS;

    ALOGD("%s-%s: entered", usage_table[out->ausage], __func__);

    pthread_mutex_lock(&out->lock);
    if (out->ausage == AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
        if (out->comprinfo) {
            if (out->sstate == STATE_PLAYING) {
                if (adev->notify_stop_output_tovisualizer != NULL) {
                    adev->notify_stop_output_tovisualizer(out->handle);
                    ALOGI("%s: Notify Stop to VisualizerHAL", __func__);
                }

                ret = compress_pause(out->comprinfo);
                if (ret == 0) {
                    out->sstate = STATE_PAUSED;
                    ALOGI("%s-%s: Transit to Paused", usage_table[out->ausage], __func__);
                } else {
                    ALOGD("%s-%s: Failed to pause(%s)", usage_table[out->ausage], __func__, compress_get_error(out->comprinfo));
                }
            } else {
                ALOGD("%s-%s: Abnormal State(%u) for pausing", usage_table[out->ausage], __func__, out->sstate);
            }
        } else {
            ALOGE("%s-%s: Invalid for pausing", usage_table[out->ausage], __func__);
        }
    }
    pthread_mutex_unlock(&out->lock);

    ALOGD("%s-%s: exit", usage_table[out->ausage], __func__);
    return ret;
}

static int out_resume(struct audio_stream_out* stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    struct audio_device *adev = out->adev;
    int ret = -ENOSYS;

    ALOGD("%s-%s: entered", usage_table[out->ausage], __func__);

    pthread_mutex_lock(&out->lock);
    if (out->ausage == AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
        if (out->comprinfo) {
            if (out->sstate == STATE_PAUSED) {
                ret = compress_resume(out->comprinfo);
                if (ret == 0) {
                    out->sstate = STATE_PLAYING;
                    ALOGI("%s-%s: Transit to Playing", usage_table[out->ausage], __func__);

                    if (adev->notify_start_output_tovisualizer != NULL) {
                        adev->notify_start_output_tovisualizer(out->handle);
                        ALOGI("%s: Notify Start to VisualizerHAL", __func__);
                    }
                } else {
                    ALOGD("%s-%s: Failed to resume(%s)", usage_table[out->ausage], __func__, compress_get_error(out->comprinfo));
                }
            } else {
                ALOGD("%s-%s: Abnormal State(%u) for resuming", usage_table[out->ausage], __func__, out->sstate);
            }
        } else {
            ALOGE("%s-%s: Invalid for resuming", usage_table[out->ausage], __func__);
        }
    }
    pthread_mutex_unlock(&out->lock);

    ALOGD("%s-%s: exit", usage_table[out->ausage], __func__);
    return ret;
}

static int out_drain(struct audio_stream_out* stream, audio_drain_type_t type )
{
    struct stream_out *out = (struct stream_out *)stream;
    int ret = -ENOSYS;

    ALOGD("%s-%s: entered with type = %d", usage_table[out->ausage], __func__, type);

    pthread_mutex_lock(&out->lock);
    if (out->ausage == AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
        if (out->comprinfo) {
            if (out->sstate > STATE_IDLE) {
                if (type == AUDIO_DRAIN_EARLY_NOTIFY)
                    ret = send_offload_msg(out, OFFLOAD_MSG_WAIT_PARTIAL_DRAIN);
                else
                    ret = send_offload_msg(out, OFFLOAD_MSG_WAIT_DRAIN);
            } else {
                out->offload_callback(STREAM_CBK_EVENT_DRAIN_READY, NULL, out->offload_cookie);
                ALOGD("%s-%s: State is IDLE. Return callback with drain_ready", usage_table[out->ausage], __func__);
            }
        } else {
            ALOGE("%s-%s: Invalid for draining", usage_table[out->ausage], __func__);
        }
    }
    pthread_mutex_unlock(&out->lock);

    ALOGD("%s-%s: exit", usage_table[out->ausage], __func__);
    return ret;
}

static int out_flush(struct audio_stream_out* stream)
{
    struct stream_out *out = (struct stream_out *)stream;
    int ret = -ENOSYS;

    ALOGD("%s-%s: entered", usage_table[out->ausage], __func__);

    pthread_mutex_lock(&out->lock);
    if (out->ausage == AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
        if (out->comprinfo) {
            if (out->sstate != STATE_IDLE) {
                ret = do_stop_output_stream(out, true);
                out->sstate = STATE_IDLE;
                ALOGI("%s-%s: Transit to idle due to flush", usage_table[out->ausage], __func__);
            } else {
                ret = 0;
                ALOGD("%s-%s: This stream is already stopped", usage_table[out->ausage], __func__);
            }
        } else {
            ALOGE("%s-%s: Invalid for flushing", usage_table[out->ausage], __func__);
        }
    }
    pthread_mutex_unlock(&out->lock);

    ALOGD("%s-%s: exit", usage_table[out->ausage], __func__);
    return ret;
}


/****************************************************************************/
/**                                                                      **/
/** The Stream_In Function Implementation                                 **/
/**                                                                       **/
/****************************************************************************/
static uint32_t in_get_sample_rate(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGVV("%s-%s: exit with sample rate = %u", usage_table[in->ausage], __func__, in->sample_rate);
    return in->sample_rate;
}

static int in_set_sample_rate(struct audio_stream *stream, uint32_t rate)
{
    struct stream_in *in = (struct stream_in *)stream;
    (void)rate;

    ALOGVV("%s-%s: exit with %u", usage_table[in->ausage], __func__, rate);
    return -ENOSYS;
}

static size_t in_get_buffer_size(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;
    size_t size = 0;
    int channel_count = audio_channel_count_from_in_mask(in->channel_mask);

    size = get_input_buffer_size(in->sample_rate, channel_count);

    ALOGVV("%s-%s: exit with %d", usage_table[in->ausage], __func__, (int)size);
    return size;
}

static audio_channel_mask_t in_get_channels(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGVV("%s-%s: exit with channel mask = 0x%x", usage_table[in->ausage], __func__, in->channel_mask);
    return in->channel_mask;
}

static audio_format_t in_get_format(const struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGVV("%s-%s: exit with audio format = 0x%x", usage_table[in->ausage], __func__, in->format);
    return in->format;
}

static int in_set_format(struct audio_stream *stream, audio_format_t format)
{
    struct stream_in *in = (struct stream_in *)stream;
    (void)format;

    ALOGVV("%s-%s: enter with %d", usage_table[in->ausage], __func__, format);

    ALOGVV("%s-%s: exit", usage_table[in->ausage], __func__);
    return -ENOSYS;
}

static int in_standby(struct audio_stream *stream)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->adev;

    ALOGD("%s-%s: enter", usage_table[in->ausage], __func__);

    pthread_mutex_lock(&in->lock);
    if (in->sstate != STATE_STANDBY) {
        /* Stop stream & transit to Idle */
        if (in->sstate != STATE_IDLE) {
            ALOGV("%s-%s: stream is running, will stop!", usage_table[in->ausage], __func__);
            do_stop_input_stream(in);
            in->sstate = STATE_IDLE;
            ALOGI("%s-%s: Transit to Idle", usage_table[in->ausage], __func__);
        }

        /* Close device & transit to Standby */
        pthread_mutex_lock(&adev->lock);
        do_close_input_stream(in);
        pthread_mutex_unlock(&adev->lock);
        in->sstate = STATE_STANDBY;
        ALOGI("%s-%s: Transit to Standby", usage_table[in->ausage], __func__);
    }
    in->err_count = 0;
    pthread_mutex_unlock(&in->lock);

    ALOGD("%s-%s: exit", usage_table[in->ausage], __func__);
    return 0;
}

static int in_dump(const struct audio_stream *stream, int fd)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGV("%s-%s: enter with fd(%d)", usage_table[in->ausage], __func__, fd);

    ALOGV("%s-%s: exit", usage_table[in->ausage], __func__);
    return 0;
}

static int in_set_parameters(struct audio_stream *stream, const char *kvpairs)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->adev;
    struct str_parms *parms;
    int ret = 0, process_count = 0;
    char value[32];

    ALOGD("%s-%s: enter with param = %s", usage_table[in->ausage], __func__, kvpairs);

    parms = str_parms_create_str(kvpairs);

    pthread_mutex_lock(&in->lock);
    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_INPUT_SOURCE, value, sizeof(value));
    if (ret >= 0) {
        unsigned int val;

        val = (unsigned int)atoi(value);
        if ((in->source != val) && (val != 0)) {
            ALOGD("%s-%s: Changing source from %d to %d", usage_table[in->ausage], __func__, in->source, val);
            in->source = val;
        }

        process_count++;
    }

    ret = str_parms_get_str(parms, AUDIO_PARAMETER_STREAM_ROUTING, value, sizeof(value));
    if (ret >= 0) {
        audio_devices_t req_device;

        /* AudioFlinger wants to change Audio Routing to some device */
        req_device = atoi(value);
        if (req_device != 0) {
            ALOGD("%s-%s: Requested to change route from 0x%X to 0x%X", usage_table[in->ausage], __func__, in->devices, req_device);
            in->devices = req_device;

            pthread_mutex_lock(&adev->lock);
            set_audio_route((void *)in, AUSAGE_CAPTURE, in->ausage, true);
            pthread_mutex_unlock(&adev->lock);
        }

        process_count++;
    }
    pthread_mutex_unlock(&in->lock);

    if (process_count == 0)
        ALOGW("%s-%s: Not Supported param!", usage_table[in->ausage], __func__);

    str_parms_destroy(parms);
    ALOGD("%s-%s: exit", usage_table[in->ausage], __func__);
    return 0;
}

static char * in_get_parameters(
        const struct audio_stream *stream,
        const char *keys)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGD("%s-%s: enter with keys(%s)", usage_table[in->ausage], __func__, keys);

    return strdup("");
}

static int in_set_gain(struct audio_stream_in *stream, float gain)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGV("%s-%s: enter with gain(%f)", usage_table[in->ausage], __func__, gain);

    return 0;
}

static ssize_t in_read(
        struct audio_stream_in *stream,
        void* buffer,
        size_t bytes)
{
    struct stream_in *in = (struct stream_in *)stream;
    struct audio_device *adev = in->adev;
    size_t frames_rq = bytes / audio_stream_in_frame_size(stream);
    int ret = 0;

    ALOGVV("%s-%s: enter", usage_table[in->ausage], __func__);

    pthread_mutex_lock(&in->lock);
    /* Check Device is opened */
    if (in->sstate == STATE_STANDBY) {
        pthread_mutex_lock(&adev->lock);
        ret = do_open_input_stream(in);
        if (ret != 0) {
            ALOGE("%s-%s: Fail to open Output Stream!", usage_table[in->ausage], __func__);
            pthread_mutex_unlock(&adev->lock);
            pthread_mutex_unlock(&in->lock);
            return ret;
        } else {
            in->sstate = STATE_IDLE;
            ALOGI("%s-%s: Transit to Idle", usage_table[in->ausage], __func__);
        }
        pthread_mutex_unlock(&adev->lock);
    }

    /* Transfer data before start */
    if (in->sstate == STATE_IDLE) {
        /* Start stream & transit to Playing */
        ret = do_start_input_stream(in);
        if (ret != 0) {
            ALOGE("%s-%s: Fail to start Output Stream!", usage_table[in->ausage], __func__);
        } else {
            in->sstate = STATE_PLAYING;
            ALOGI("%s-%s: Transit to Capturing", usage_table[in->ausage], __func__);
        }
    }

    if (in->sstate == STATE_PLAYING)
        do_read_buffer(in, buffer, bytes);
    pthread_mutex_unlock(&in->lock);

    ALOGVV("%s-%s: exit with read data(%d Bytes)", usage_table[in->ausage], __func__, (int)bytes);
    return bytes;
}

static uint32_t in_get_input_frames_lost(struct audio_stream_in *stream)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGV("%s-%s: exit", usage_table[in->ausage], __func__);
    return 0;
}

static int in_add_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGD("%s: enter with effect(%p)", __func__, effect);

    ALOGD("%s: exit", __func__);
    return 0;
}

static int in_remove_audio_effect(const struct audio_stream *stream, effect_handle_t effect)
{
    struct stream_in *in = (struct stream_in *)stream;

    ALOGD("%s: enter with effect(%p)", __func__, effect);

    ALOGD("%s: exit", __func__);
    return 0;
}

/*****************************************************************************/
/**                                                                       **/
/** The Audio Device Function Implementation                              **/
/**                                                                       **/
/*****************************************************************************/
static int adev_open_output_stream(
        struct audio_hw_device *dev,
        audio_io_handle_t handle,
        audio_devices_t devices,
        audio_output_flags_t flags,
        struct audio_config *config,
        struct audio_stream_out **stream_out,
        const char *address __unused)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_out *out;
    struct exynos_audio_usage *active_ausage;
    int ret;

    ALOGD("device-%s: enter: io_handle (%d), sample_rate(%d) channel_mask(%#x) devices(%#x) flags(%#x)",
          __func__, handle, config->sample_rate, config->channel_mask, devices, flags);

    *stream_out = NULL;

    /* Allocate memory for Structure audio_stream_out */
    out = (struct stream_out *)calloc(1, sizeof(struct stream_out));
    if (!out) {
        ALOGE("device-%s: Fail to allocate memory for stream_out", __func__);
        return -ENOMEM;
    }
    out->adev = adev;

    if (devices == AUDIO_DEVICE_NONE)
        devices = AUDIO_DEVICE_OUT_SPEAKER;

    /* Save common parameters from Android Platform */
    out->handle = handle;
    out->devices = devices;
    out->flags = flags;
    out->sample_rate = config->sample_rate;
    out->channel_mask = config->channel_mask;
    out->format = config->format;

    /* Set basic configuration from Flags */
    if ((out->flags & AUDIO_OUTPUT_FLAG_PRIMARY) != 0) {
        /* Case: Normal Playback */
        ALOGD("device-%s: Requested open Primary output", __func__);

        out->ausage = AUSAGE_PLAYBACK_PRIMARY;

        /* Check previous primary output is exist */
        if (adev->primary_output == NULL) {
            adev->primary_output = out;
        } else {
            ALOGE("%s-%s: Primary output is already opened", usage_table[out->ausage], __func__);
            ret = -EEXIST;
            goto err_open;
        }

        /* Set PCM Configuration */
        out->pcmconfig = pcm_config_primary;
        check_and_set_pcm_config(&out->pcmconfig, config);
    } else if ((out->flags & AUDIO_OUTPUT_FLAG_FAST) != 0) {
        /* Case: Playback with Low Latency */
        ALOGD("device-%s: Requested open fast output", __func__);

        out->ausage = AUSAGE_PLAYBACK_LOW_LATENCY;

        /* Set PCM Configuration */
        out->pcmconfig = pcm_config_low_latency;
        check_and_set_pcm_config(&out->pcmconfig, config);
    } else if ((out->flags & AUDIO_OUTPUT_FLAG_DEEP_BUFFER) != 0) {
        /* Case: Playback with Deep Buffer */
        ALOGD("device-%s: Requested open Deep Buffer output", __func__);

        out->ausage = AUSAGE_PLAYBACK_DEEP_BUFFER;

        /* Set PCM Configuration */
        out->pcmconfig = pcm_config_deep_buffer;
        check_and_set_pcm_config(&out->pcmconfig, config);
    } else if ((out->flags & AUDIO_OUTPUT_FLAG_DIRECT) != 0) {
        /* Case: Payback with Direct */
        if ((out->devices & AUDIO_DEVICE_OUT_AUX_DIGITAL) != 0) {
            /* Sub-Case: Playback with Aux Digital */
            ALOGD("device-%s: Requested open Aux output", __func__);

            out->ausage = AUSAGE_PLAYBACK_AUX_DIGITAL;
        } else if ((out->flags & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) != 0) {
            /* Sub-Case: Playback with Offload */
            ALOGD("device-%s: Requested open Compress Offload output", __func__);

            out->ausage = AUSAGE_PLAYBACK_COMPR_OFFLOAD;

            if (is_supported_compressed_format(config->offload_info.format)) {
                out->comprconfig.codec = (struct snd_codec *)calloc(1, sizeof(struct snd_codec));
                if (out->comprconfig.codec == NULL) {
                    ALOGE("%s-%s: Fail to allocate memory for Sound Codec", usage_table[out->ausage], __func__);

                    ret = -ENOMEM;
                    goto err_open;
                }

                /* Mapping function pointers in Structure audio_stream_in as real function */
                out->stream.set_callback = out_set_callback;
                out->stream.pause = out_pause;
                out->stream.resume = out_resume;
                out->stream.drain = out_drain;
                out->stream.flush = out_flush;

                if (config->offload_info.channel_mask
                        && (config->offload_info.channel_mask != config->channel_mask)) {
                    ALOGV("%s-%s: Channel Mask = Config(%u), Offload_Info(%u)",
                        usage_table[out->ausage], __func__, config->channel_mask, config->offload_info.channel_mask);
                    config->channel_mask = config->offload_info.channel_mask;
                }

                if (config->offload_info.sample_rate && (config->offload_info.sample_rate != config->sample_rate)) {
                    ALOGV("%s-%s: Sampling Rate = Config(%u), Offload_Info(%u)",
                        usage_table[out->ausage], __func__, config->sample_rate, config->offload_info.sample_rate);
                    config->sample_rate = config->offload_info.sample_rate;
                }

                /* Set Compress Offload Configuration */
                out->comprconfig.fragment_size = COMPRESS_OFFLOAD_FRAGMENT_SIZE;
                out->comprconfig.fragments = COMPRESS_OFFLOAD_NUM_FRAGMENTS;
                out->comprconfig.codec->id = get_snd_codec_id(config->offload_info.format);
//                out->comprconfig.codec->ch_in = audio_channel_count_from_out_mask(config->channel_mask);
                out->comprconfig.codec->ch_in = config->channel_mask;
//                out->comprconfig.codec->ch_out = audio_channel_count_from_out_mask(config->channel_mask);
                out->comprconfig.codec->ch_out = config->channel_mask;
//                out->comprconfig.codec->sample_rate = compress_get_alsa_rate(config->sample_rate);
                out->comprconfig.codec->sample_rate = config->sample_rate;
                out->comprconfig.codec->bit_rate = config->offload_info.bit_rate;
                out->comprconfig.codec->format = config->format;

                ALOGV("%s-%s: Sound Codec = ID(%u), Channel(%u), Sample Rate(%u), Bit Rate(%u)",
                      usage_table[out->ausage], __func__, out->comprconfig.codec->id, out->comprconfig.codec->ch_in,
                      out->comprconfig.codec->sample_rate, out->comprconfig.codec->bit_rate);

                if (flags & AUDIO_OUTPUT_FLAG_NON_BLOCKING) {
                    ALOGV("%s-%s: Need to work as Nonblock Mode!", usage_table[out->ausage], __func__);
                    out->nonblock_flag = 1;

                    create_offload_callback_thread(out);
                    list_init(&out->offload_msg_list);
                }

                out->ready_new_metadata = 1;
            } else {
                ALOGE("%s-%s: Unsupported Compressed Format(%x)", usage_table[out->ausage],
                    __func__, config->offload_info.format);

                ret = -EINVAL;
                goto err_open;
            }
        }
    } else {
        /* Error Case: Not Supported usage */
        ALOGE("device-%s: Requested open un-supported output", __func__);

        ret = -EINVAL;
        goto err_open;
    }

    /* Mapping function pointers in Structure audio_stream_in as real function */
    out->stream.common.get_sample_rate = out_get_sample_rate;
    out->stream.common.set_sample_rate = out_set_sample_rate;
    out->stream.common.get_buffer_size = out_get_buffer_size;
    out->stream.common.get_channels = out_get_channels;
    out->stream.common.get_format = out_get_format;
    out->stream.common.set_format = out_set_format;
    out->stream.common.standby = out_standby;
    out->stream.common.dump = out_dump;
    out->stream.common.set_parameters = out_set_parameters;
    out->stream.common.get_parameters = out_get_parameters;
    out->stream.common.add_audio_effect = out_add_audio_effect;
    out->stream.common.remove_audio_effect = out_remove_audio_effect;
    out->stream.get_latency = out_get_latency;
    out->stream.set_volume = out_set_volume;
    out->stream.write = out_write;
    out->stream.get_render_position = out_get_render_position;
    out->stream.get_next_write_timestamp = out_get_next_write_timestamp;
    out->stream.get_presentation_position = out_get_presentation_position;

    /* Set Platform-specific information */
    pthread_mutex_init(&out->lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_lock(&out->lock);

    out->sstate = STATE_STANDBY;   // Not open Device
    ALOGI("%s-%s: Transit to Standby", usage_table[out->ausage], __func__);

    pthread_mutex_lock(&adev->lock);
    if (out->ausage == AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
        /* Defense code to clear any dangling ausage, if media server is restarted */
        active_ausage = get_dangling_ausage_from_list(adev, AUSAGE_PLAYBACK, out->handle);
        if (active_ausage) {
            /* Clear dangling ausage from list, one ausage can exist at a time*/
            remove_audio_usage(adev, AUSAGE_PLAYBACK, active_ausage->stream.out);
            ALOGD("%s-%s: Remove Dangling ausage from list!!", usage_table[out->ausage], __func__);
        }
    }

    /* Add this audio usage into Audio Usage List */
    add_audio_usage(adev, AUSAGE_PLAYBACK, (void *)out);
    pthread_mutex_unlock(&adev->lock);

    pthread_mutex_unlock(&out->lock);

    /* Set Structure audio_stream_in for return */
    *stream_out = &out->stream;

    ALOGD("device-%s: Opened %s stream", __func__, usage_table[out->ausage]);
    return 0;

err_open:
    free(out);
    *stream_out = NULL;

    ALOGE("device-%s: Cannot open this stream as error(%d)", __func__, ret);
    return ret;
}

static void adev_close_output_stream(
            struct audio_hw_device *dev,
            struct audio_stream_out *stream)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_out *out = (struct stream_out *)stream;
    audio_usage_id_t id = out->ausage;

    ALOGD("device-%s: enter with Audio Usage(%s)", __func__, usage_table[id]);

    if (out) {
        out_standby(&stream->common);

        /* Clean up Platform-specific information */
        /* Remove this audio usage from Audio Usage List */
        pthread_mutex_lock(&out->lock);
        pthread_mutex_lock(&adev->lock);
        remove_audio_usage(adev, AUSAGE_PLAYBACK, (void *)out);
        pthread_mutex_unlock(&adev->lock);
        pthread_mutex_unlock(&out->lock);

        /* Check close Primary Output */
        if (out->ausage == AUSAGE_PLAYBACK_PRIMARY) {
            adev->primary_output = NULL;
        } else if (out->ausage == AUSAGE_PLAYBACK_COMPR_OFFLOAD) {
            if (out->nonblock_flag)
                destroy_offload_callback_thread(out);

            pthread_mutex_lock(&out->lock);
            if (out->comprconfig.codec != NULL) {
                free(out->comprconfig.codec);
                out->comprconfig.codec = NULL;
            }
            pthread_mutex_unlock(&out->lock);
        }

        pthread_mutex_destroy(&out->lock);

        free(out);
    }

    ALOGD("device-%s: Closed %s stream", __func__, usage_table[id]);
    return;
}

static int adev_set_parameters(
            struct audio_hw_device *dev,
            const char *kvpairs)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct str_parms *parms;
    char value[32];
    int val;
    int ret = 0;

    ALOGD("device-%s: enter with key(%s)", __func__, kvpairs);

    pthread_mutex_lock(&adev->lock);

    parms = str_parms_create_str(kvpairs);

    ret = str_parms_get_str(parms, "screen_state", value, sizeof(value));
    if (ret >= 0) {
        if (strcmp(value, AUDIO_PARAMETER_VALUE_ON) == 0)
            adev->screen_off = false;
        else
            adev->screen_off = true;
    }

    ret = str_parms_get_int(parms, "rotation", &val);
    if (ret >= 0) {
        switch (val) {
        case 0:
        case 90:
        case 180:
        case 270:
            ALOGD("device-%s: Set is rotated with %d", __func__, val);
            break;
        default:
            ALOGE("device-%s: unexpected rotation of %d", __func__, val);
        }
    }

    /* LTE Based Communication - CP Centric */
    ret = str_parms_get_str(parms, "VoLTEstate", value, sizeof(value));
    if (ret >= 0) {
        if (!strcmp(value, "voice")) {
            /* FIXME: Need to check Handover & control Voice PCM */
            adev->call_state = LTE_CALL;
            ALOGD("device-%s: VoLTE Voice Call Start!!", __func__);
        } else if (!strcmp(value, "end")) {
            /* Need to check Handover & control Voice PCM */
            adev->call_state = CP_CALL;
            ALOGD("device-%s: VoLTE Voice Call End!!", __func__);
        } else
            ALOGD("device-%s: Unknown VoLTE parameters = %s!!", __func__, value);

        adev->usage_amode = get_usage_mode(adev, adev->amode);
        ALOGD("device-%s: Platform mode(%d) configured HAL mode(%s)", __func__, adev->amode, mode_table[adev->usage_amode]);
        if (adev->usage_amode == AUSAGE_MODE_VOICE_CALL)
            voice_set_call_mode(adev->voice, VOICE_CALL_CS);
        else if (adev->usage_amode == AUSAGE_MODE_LTE_CALL)
            voice_set_call_mode(adev->voice, VOICE_CALL_PS);

        voice_set_path(adev->voice, adev->primary_output->devices);
    }

    /* WiFi Based Communication - AP Centric */
    ret = str_parms_get_str(parms, "VoWiFistate", value, sizeof(value));
    if (ret >= 0) {
        if (!strcmp(value, "voice")) {
            /* FIXME: Need to check Handover & control Voice PCM */
            adev->call_state = WIFI_CALL;
            ALOGD("device-%s: VoWiFI Voice Call Start!!", __func__);
        } else if (!strcmp(value, "end")) {
            /* FIXME: Need to check Handover & control Voice PCM */
            adev->call_state = CP_CALL;
            ALOGD("device-%s: VoWiFI Voice Call End!!", __func__);
        } else
            ALOGD("device-%s: Unknown VoWiFI parameters = %s!!", __func__, value);

        adev->usage_amode = get_usage_mode(adev, adev->amode);
        ALOGD("device-%s: Platform mode(%d) configured HAL mode(%s)", __func__, adev->amode, mode_table[adev->usage_amode]);

        /* Need to re-routing */
        set_audio_route((void *)adev->primary_output, AUSAGE_PLAYBACK, adev->primary_output->ausage, true);
    }

    str_parms_destroy(parms);
    pthread_mutex_unlock(&adev->lock);

    ALOGD("device-%s: exit", __func__);
    return ret;
}

static char * adev_get_parameters(
        const struct audio_hw_device *dev,
        const char *keys)
{
    struct audio_device *adev = (struct audio_device *)dev;
    char *str = NULL;

    ALOGD("device-%s: enter with key(%s)", __func__, keys);

    pthread_mutex_lock(&adev->lock);
    pthread_mutex_unlock(&adev->lock);

    ALOGD("device-%s: exit with %s", __func__, str);
    return str;
}

static int adev_init_check(const struct audio_hw_device *dev)
{
    struct audio_device *adev = (struct audio_device *)dev;
    int ret = 0;

    if (adev) {
        if (adev->rinfo == NULL) {
            ALOGE("device-%s: Audio Primary HW Device is not initialized", __func__);
            ret = -EINVAL;
        } else {
            ALOGV("device-%s: Audio Primary HW Device is already opened & initialized", __func__);
        }
    } else {
        ALOGE("device-%s: Audio Primary HW Device is not opened", __func__);
        ret = -EINVAL;
    }

    return ret;
}

static int adev_set_voice_volume(struct audio_hw_device *dev, float volume)
{
    struct audio_device *adev = (struct audio_device *)dev;
    int ret = 0;

    ALOGD("device-%s: enter with volume level(%f)", __func__, volume);

    pthread_mutex_lock(&adev->lock);
    adev->voice_volume = volume;

    if (adev->voice)
        ret = voice_set_volume(adev->voice, volume);
    pthread_mutex_unlock(&adev->lock);

    ALOGD("device-%s: exit", __func__);
    return ret;
}

static int adev_set_master_volume(struct audio_hw_device *dev, float volume)
{
    struct audio_device *adev = (struct audio_device *)dev;
    int ret = 0;

    ALOGD("device-%s: enter with volume level(%f)", __func__, volume);

    pthread_mutex_lock(&adev->lock);
    ret = -ENOSYS;
    pthread_mutex_unlock(&adev->lock);

    ALOGD("device-%s: exit, Not supported Master Volume by AudioHAL", __func__);
    return ret;
}

static int adev_get_master_volume(struct audio_hw_device *dev, float *volume)
{
    struct audio_device *adev = (struct audio_device *)dev;
    int ret = 0;

    ALOGD("device-%s: enter", __func__);

    pthread_mutex_lock(&adev->lock);
    *volume = 0;
    ret = -ENOSYS;
    pthread_mutex_unlock(&adev->lock);

    ALOGD("device-%s: exit, Not supported Master Volume by AudioHAL", __func__);
    return ret;
}

static int adev_set_master_mute(struct audio_hw_device *dev, bool muted)
{
    struct audio_device *adev = (struct audio_device *)dev;
    int ret = 0;

    ALOGD("device-%s: enter with mute statue(%d)", __func__, muted);

    pthread_mutex_lock(&adev->lock);
    ret = -ENOSYS;
    pthread_mutex_unlock(&adev->lock);

    ALOGD("device-%s: exit, Not supported Master Mute by AudioHAL", __func__);
    return ret;
}

static int adev_get_master_mute(struct audio_hw_device *dev, bool *muted)
{
    struct audio_device *adev = (struct audio_device *)dev;
    int ret = 0;

    ALOGD("device-%s: enter", __func__);

    pthread_mutex_lock(&adev->lock);
    *muted = false;
    ret = -ENOSYS;
    pthread_mutex_unlock(&adev->lock);

    ALOGD("device-%s: exit, Not supported Master Mute by AudioHAL", __func__);
    return ret;
}

static int adev_set_mode(struct audio_hw_device *dev, audio_mode_t mode)
{
    struct audio_device *adev = (struct audio_device *)dev;

    ALOGD("device-%s: enter with mode = %d", __func__, mode);
    ALOGD("device-%s: previous mode = %d", __func__, adev->amode);

    pthread_mutex_lock(&adev->lock);
    if (adev->amode != mode) {
        if (adev->voice) {
            if ((mode == AUDIO_MODE_NORMAL || mode == AUDIO_MODE_IN_COMMUNICATION)
                    && voice_is_in_call(adev->voice)) {
                /* Change from Voice Call Mode to Normal Mode */
                /* Stop Voice Call */
                do_stop_voice_call(adev);
                voice_set_audio_clock(adev->voice, VOICE_AUDIO_TURN_OFF_I2S);
                voice_close(adev->voice);
                ALOGD("device-%s: *** Stopped CP Voice Call ***", __func__);

                /* Changing Call State */
                if (adev->call_state == CP_CALL) adev->call_state = CALL_OFF;
            } else if (mode == AUDIO_MODE_IN_CALL) {
                /* Change from Normal Mode to Voice Call Mode */
                /* We cannot start Voice Call right now because we don't know which device will be used.
                   So, we need to delay Voice Call start when get the routing information for Voice Call */

                /* Changing Call State */
                if (adev->call_state == CALL_OFF) adev->call_state = CP_CALL;
            }
        } else if (mode == AUDIO_MODE_IN_CALL) {
                /* Request to change to Voice Call Mode, But Primary AudioHAL cannot support this */
                ALOGW("device-%s: CP RIL interface is NOT supported!!", __func__);
                pthread_mutex_unlock(&adev->lock);
                return -EINVAL;
        }

        /* Changing Android Audio Mode */
        ALOGD("device-%s: changed mode from %d to %d", __func__, adev->amode, mode);
        adev->amode = mode;

        /* Get Audio Usage Mode */
        adev->usage_amode = get_usage_mode(adev, adev->amode);
        ALOGD("device-%s: Platform mode(%d) configured HAL mode(%s)", __func__, mode, mode_table[adev->usage_amode]);
    }
    pthread_mutex_unlock(&adev->lock);

    ALOGD("device-%s: exit", __func__);
    return 0;
}

static int adev_set_mic_mute(struct audio_hw_device *dev, bool state)
{
    struct audio_device *adev = (struct audio_device *)dev;
    int ret = 0;

    ALOGD("device-%s: enter with mic mute(%d)", __func__, state);

    pthread_mutex_lock(&adev->lock);
    if (adev->voice)
        ret = voice_set_mic_mute(adev->voice, state);
    pthread_mutex_unlock(&adev->lock);

    ALOGD("device-%s: exit", __func__);
    return ret;
}

static int adev_get_mic_mute(const struct audio_hw_device *dev, bool *state)
{
    struct audio_device *adev = (struct audio_device *)dev;
    int ret = 0;

    ALOGD("device-%s: enter", __func__);

    pthread_mutex_lock(&adev->lock);
    if (adev->voice)
        *state = voice_get_mic_mute(adev->voice);
    else
        *state = false;
    pthread_mutex_unlock(&adev->lock);

    ALOGD("device-%s: exit", __func__);
    return ret;
}

static size_t adev_get_input_buffer_size(
            const struct audio_hw_device *dev __unused,
            const struct audio_config *config)
{
    size_t size = 0;
    int channel_count = audio_channel_count_from_in_mask(config->channel_mask);

    size = get_input_buffer_size(config->sample_rate, channel_count);

    ALOGD("device-%s: exited with %d Bytes", __func__, (int)size);
    return size;
}

static int adev_open_input_stream(
        struct audio_hw_device *dev,
        audio_io_handle_t handle,
        audio_devices_t devices,
        struct audio_config *config,
        struct audio_stream_in **stream_in,
        audio_input_flags_t flags,
        const char *address __unused,
        audio_source_t source)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_in *in;
    struct exynos_audio_usage *active_ausage;
    int channel_count = audio_channel_count_from_in_mask(config->channel_mask);
    int ret = 0;

    ALOGD("device-%s: enter: io_handle (%d), sample_rate(%d) channel_mask(%#x) devices(%#x) flags(%#x) source(%d)",
          __func__, handle, config->sample_rate, config->channel_mask, devices, flags, source);

    *stream_in = NULL;

    if (check_input_parameters(config->sample_rate, config->format, channel_count) != 0) {
        ALOGE("device-%s: Request has unsupported configuration!", __func__);

        config->format = audio_pcmformat_from_alsaformat(pcm_config_audio_capture.format);
        config->sample_rate = (uint32_t)(pcm_config_audio_capture.rate);
        config->channel_mask = audio_channel_in_mask_from_count(pcm_config_audio_capture.channels);
        ALOGD("device-%s: Proposed configuration!", __func__);
        return -EINVAL;
    }

    /* Allocate memory for Structure audio_stream_in */
    in = (struct stream_in *)calloc(1, sizeof(struct stream_in));
    if (!in) {
        ALOGE("device-%s: Fail to allocate memory for stream_in", __func__);
        return -ENOMEM;
    }
    in->adev = adev;

    if (devices == AUDIO_DEVICE_NONE)
        devices = AUDIO_DEVICE_IN_BUILTIN_MIC;

    /* Save common parameters from Android Platform */
    /* Mapping function pointers in Structure audio_stream_in as real function */
    in->stream.common.get_sample_rate = in_get_sample_rate;
    in->stream.common.set_sample_rate = in_set_sample_rate;
    in->stream.common.get_buffer_size = in_get_buffer_size;
    in->stream.common.get_channels = in_get_channels;
    in->stream.common.get_format = in_get_format;
    in->stream.common.set_format = in_set_format;
    in->stream.common.standby = in_standby;
    in->stream.common.dump = in_dump;
    in->stream.common.set_parameters = in_set_parameters;
    in->stream.common.get_parameters = in_get_parameters;
    in->stream.common.add_audio_effect = in_add_audio_effect;
    in->stream.common.remove_audio_effect = in_remove_audio_effect;
    in->stream.set_gain = in_set_gain;
    in->stream.read = in_read;
    in->stream.get_input_frames_lost = in_get_input_frames_lost;

    in->handle = handle;
    in->devices = devices;
    in->source = source;
    in->flags = flags;
    in->sample_rate = config->sample_rate;
    in->channel_mask = config->channel_mask;
    in->format = config->format;

    /* Set basic configuration from devices */
    {
        /* Case: Capture For Recording */
        ALOGD("device-%s: Requested open Low Latency input", __func__);

        in->ausage = AUSAGE_CAPTURE_LOW_LATENCY;

        /* Set PCM Configuration */
        in->pcmconfig = pcm_config_audio_capture;// Default PCM Configuration
        check_pcm_config(&in->pcmconfig, config);
    }

    /* Set Platform-specific information */
    pthread_mutex_init(&in->lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_lock(&in->lock);

    in->sstate = STATE_STANDBY;
    ALOGI("%s-%s: Transit to Standby", usage_table[in->ausage], __func__);

    /* Add this audio usage into Audio Usage List */
    pthread_mutex_lock(&adev->lock);
    add_audio_usage(adev, AUSAGE_CAPTURE, (void *)in);
    pthread_mutex_unlock(&adev->lock);

    pthread_mutex_unlock(&in->lock);

    /* Set Structure audio_stream_in for return */
    *stream_in = &in->stream;

    ALOGD("device-%s: Opened %s stream", __func__, usage_table[in->ausage]);
    return 0;

err_open:
    free(in);
    *stream_in = NULL;

    ALOGD("device-%s: Cannot open this stream as error(%d)", __func__, ret);
    return ret;
}

static void adev_close_input_stream(
        struct audio_hw_device *dev,
        struct audio_stream_in *stream)
{
    struct audio_device *adev = (struct audio_device *)dev;
    struct stream_in *in = (struct stream_in *)stream;
    audio_usage_id_t id = in->ausage;

    ALOGD("device-%s: enter with Audio Usage(%s)", __func__, usage_table[id]);

    if (in) {
        in_standby(&stream->common);

        pthread_mutex_lock(&in->lock);

        /* Remove this audio usage from Audio Usage List */
        pthread_mutex_lock(&adev->lock);
        remove_audio_usage(adev, AUSAGE_CAPTURE, (void *)in);
        pthread_mutex_unlock(&adev->lock);

        pthread_mutex_unlock(&in->lock);
        pthread_mutex_destroy(&in->lock);
        free(in);
    }

    ALOGD("device-%s: Closed %s stream", __func__, usage_table[id]);
    return;
}

static int adev_dump(const audio_hw_device_t *device, int fd)
{
    struct audio_device *adev = (struct audio_device *)device;

    ALOGV("device-%s: enter with file descriptor(%d)", __func__, fd);

    ALOGV("device-%s: exit - This function is not implemented yet!", __func__);
    return 0;
}

static int adev_close(hw_device_t *device)
{
    struct audio_device *adev = (struct audio_device *)device;

    ALOGV("device-%s: enter", __func__);

    if (adev) {
        /* Clean up Platform-specific information */
        pthread_mutex_lock(&adev->lock);
        if(adev->offload_visualizer_lib)
            dlclose(adev->offload_visualizer_lib);

        if(adev->rinfo)
            deinit_route(adev);

        if (adev->voice) {
            voice_deinit(adev->voice);
            adev->voice = NULL;
        }

        pthread_mutex_unlock(&adev->lock);
        pthread_mutex_destroy(&adev->lock);

        free(adev);
    }

    ALOGV("device-%s: Closed Audio Primary HW Device", __func__);
    return 0;
}

static int onCallbackFromRILAudio(int event, const void *data, unsigned int datalen)
{
    switch (event) {
        case VOICE_AUDIO_EVENT_RINGBACK_STATE_CHANGED:
            ALOGD("device-%s: On RINGBACK_STATE_CHANGED event! Ignored", __func__);
            if (data && datalen > 0)
                ALOGD("device-%s: Data Length(4 expected) = %d", __func__, datalen);
            break;

        case VOICE_AUDIO_EVENT_IMS_SRVCC_HANDOVER:
            ALOGD("device-%s: On IMS_SRVCC_HANDOVER event!", __func__);
            break;

        default:
            ALOGD("device-%s: On Unsupported event (%d)!", __func__, event);
            break;
    }

    return 0;
}

static int adev_open(
        const hw_module_t* module,
        const char* name,
        hw_device_t** device)
{
    struct audio_device *adev;

    ALOGV("device-%s: enter", __func__);

    /* Check Interface Name. It must be AUDIO_HARDWARE_INTERFACE */
    if (strcmp(name, AUDIO_HARDWARE_INTERFACE) != 0) {
        ALOGE("device-%s: Invalid request: Interface Name = %s", __func__, name);
        return -EINVAL;
    }

    /* Allocate memory for Structure audio_device */
    adev = calloc(1, sizeof(struct audio_device));
    if (!adev) {
        ALOGE("device-%s: Fail to allocate memory for audio_device", __func__);
        return -ENOMEM;
    }

    /* Mapping function pointers in Structure audio_hw_device as real function */
    adev->hw_device.common.tag = HARDWARE_DEVICE_TAG;
    adev->hw_device.common.version = AUDIO_DEVICE_API_VERSION_2_0;// Now, Must be Version 2.0
    adev->hw_device.common.module = (struct hw_module_t *) module;
    adev->hw_device.common.close = adev_close;

    adev->hw_device.init_check = adev_init_check;
    adev->hw_device.set_voice_volume = adev_set_voice_volume;
    adev->hw_device.set_master_volume = adev_set_master_volume;
    adev->hw_device.get_master_volume = adev_get_master_volume;
    adev->hw_device.set_master_mute = adev_set_master_mute;
    adev->hw_device.get_master_mute = adev_get_master_mute;
    adev->hw_device.set_mode = adev_set_mode;
    adev->hw_device.set_mic_mute = adev_set_mic_mute;
    adev->hw_device.get_mic_mute = adev_get_mic_mute;
    adev->hw_device.set_parameters = adev_set_parameters;
    adev->hw_device.get_parameters = adev_get_parameters;
    adev->hw_device.get_input_buffer_size = adev_get_input_buffer_size;
    adev->hw_device.open_output_stream = adev_open_output_stream;
    adev->hw_device.close_output_stream = adev_close_output_stream;
    adev->hw_device.open_input_stream = adev_open_input_stream;
    adev->hw_device.close_input_stream = adev_close_input_stream;
    adev->hw_device.dump = adev_dump;

    /* Set Platform-specific information */
    pthread_mutex_init(&adev->lock, (const pthread_mutexattr_t *) NULL);
    pthread_mutex_lock(&adev->lock);

    adev->amode = AUDIO_MODE_NORMAL;
    adev->usage_amode = AUSAGE_MODE_NORMAL;
    adev->call_state = CALL_OFF;
    adev->primary_output = NULL;
    adev->pcm_capture = NULL;
    adev->voice_volume = 0;

    /* Initialize Voice related service */
    adev->voice = voice_init();
    if (!adev->voice)
        ALOGE("device-%s: Failed to init Voice Manager!", __func__);
    else {
        ALOGD("device-%s: Successed to init Voice Manager!", __func__);

        voice_set_callback(adev->voice, (void *)onCallbackFromRILAudio);
    }

    /* Initialize Audio Route */
    if (!init_route(adev)) {
        ALOGE("device-%s: Failed to init Route!", __func__);

        if (adev->voice)
            voice_deinit(adev->voice);
        adev->voice = NULL;
        pthread_mutex_unlock(&adev->lock);
        pthread_mutex_destroy(&adev->lock);
        free(adev);
        *device = NULL;
        return -EINVAL;
    }
    else
        ALOGD("device-%s: Successed to init Route!", __func__);

    /* Initialize Audio Usage List */
    list_init(&adev->audio_usage_list);
    pthread_mutex_unlock(&adev->lock);

    /* Set Structure audio_hw_device for return */
    *device = &adev->hw_device.common;

    /* Setup the Link to communicate with VisualizerHAL */
    if (access(OFFLOAD_VISUALIZERHAL_PATH, R_OK) == 0) {
        adev->offload_visualizer_lib = dlopen(OFFLOAD_VISUALIZERHAL_PATH, RTLD_NOW);
        if (adev->offload_visualizer_lib) {
            ALOGV("device-%s: DLOPEN is successful for %s", __func__, OFFLOAD_VISUALIZERHAL_PATH);
            adev->notify_start_output_tovisualizer = (int (*)(audio_io_handle_t))dlsym(adev->offload_visualizer_lib, "notify_start_output");
            adev->notify_stop_output_tovisualizer = (int (*)(audio_io_handle_t))dlsym(adev->offload_visualizer_lib, "notify_stop_output");
        } else {
            ALOGE("device-%s: DLOPEN is failed for %s", __func__, OFFLOAD_VISUALIZERHAL_PATH);
        }
    }

    ALOGD("device-%s: Opened Audio Primary HW Device", __func__);
    return 0;
}

/* Entry Point for AudioHAL (Primary Audio HW Module for Android) */
static struct hw_module_methods_t hal_module_methods = {
    .open = adev_open,
};

struct audio_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = AUDIO_MODULE_API_VERSION_0_1,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = AUDIO_HARDWARE_MODULE_ID,
        .name = "Exynos Primary AudioHAL",
        .author = "Samsung",
        .methods = &hal_module_methods,
    },
};
