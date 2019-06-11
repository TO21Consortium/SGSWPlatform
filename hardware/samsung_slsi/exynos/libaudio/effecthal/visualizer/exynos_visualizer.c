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

#define LOG_TAG "Offload_Visualizer"
//#define LOG_NDEBUG 0

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/prctl.h>

#include "exynos_visualizer.h"


/* effect_handle_t interface implementation for visualizer effect */
extern const struct effect_interface_s effect_interface;


/* Exynos Offload Visualizer UUID: XX-XX-XX-XX-0002a5d5c51b */
const effect_descriptor_t visualizer_descriptor = {
        {0xe46b26a0, 0xdddd, 0x11db, 0x8afd, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // type
        {0x7a8044a0, 0x1a71, 0x11e3, 0xa184, {0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b}}, // uuid
        EFFECT_CONTROL_API_VERSION,
        (EFFECT_FLAG_TYPE_INSERT | EFFECT_FLAG_HW_ACC_TUNNEL),
        0, /* TODO */
        1,
        "Samsung Exynos Offload Visualizer",
        "Samsung SystemLSI",
};

/* The list for Offloaded Effect Descriptor by Exynos EffectHAL */
const effect_descriptor_t *descriptors[] = {
        &visualizer_descriptor,
        NULL,
};

pthread_once_t once = PTHREAD_ONCE_INIT;
int init_status;

/* list of created effects */
struct listnode created_effects_list;

/* list of active output streams */
struct listnode active_outputs_list;

/* visualizer capture pcm handle */
static struct pcm *pcm = NULL;

/* lock must be held when modifying or accessing created_effects_list or active_outputs_list */
pthread_mutex_t lock;

pthread_mutex_t thread_lock;
pthread_cond_t cond;
pthread_t capture_thread;
bool exit_thread;
int thread_status;


/*
 *  Local functions
 */
static void init_once()
{
    list_init(&created_effects_list);
    list_init(&active_outputs_list);

    pthread_mutex_init(&lock, NULL);
    pthread_mutex_init(&thread_lock, NULL);
    pthread_cond_init(&cond, NULL);
    exit_thread = false;
    thread_status = -1;

    init_status = 0;
}

int lib_init()
{
    pthread_once(&once, init_once);
    return init_status;
 }

bool effect_exists(effect_context_t *context)
{
    struct listnode *node;

    list_for_each(node, &created_effects_list) {
        effect_context_t *fx_ctxt = node_to_item(node, effect_context_t, effects_list_node);
        if (fx_ctxt == context) {
            return true;
        }
    }

    return false;
}

output_context_t *get_output(audio_io_handle_t output)
{
    struct listnode *node;

    list_for_each(node, &active_outputs_list) {
        output_context_t *out_ctxt = node_to_item(node, output_context_t, outputs_list_node);
        if (out_ctxt->handle == output) {
            return out_ctxt;
        }
    }

    return NULL;
}

void add_effect_to_output(output_context_t * output, effect_context_t *context)
{
    struct listnode *fx_node;

    list_for_each(fx_node, &output->effects_list) {
        effect_context_t *fx_ctxt = node_to_item(fx_node, effect_context_t, output_node);
        if (fx_ctxt == context)
            return;
    }

    list_add_tail(&output->effects_list, &context->output_node);
    if (context->ops.start)
        context->ops.start(context, output);

    return ;
}

void remove_effect_from_output(
        output_context_t * output,
        effect_context_t *context)
{
    struct listnode *fx_node;

    list_for_each(fx_node, &output->effects_list) {
        effect_context_t *fx_ctxt = node_to_item(fx_node, effect_context_t, output_node);
        if (fx_ctxt == context) {
            if (context->ops.stop)
                context->ops.stop(context, output);
            list_remove(&context->output_node);
            return;
        }
    }

    return ;
}

bool effects_enabled()
{
    struct listnode *out_node;

    list_for_each(out_node, &active_outputs_list) {
        struct listnode *fx_node;
        output_context_t *out_ctxt = node_to_item(out_node, output_context_t, outputs_list_node);

        list_for_each(fx_node, &out_ctxt->effects_list) {
            effect_context_t *fx_ctxt = node_to_item(fx_node, effect_context_t, output_node);
            if (fx_ctxt->state == EFFECT_STATE_ACTIVE && fx_ctxt->ops.process != NULL)
                return true;
        }
    }

    return false;
}

void *capture_thread_loop(void *arg __unused)
{
    int16_t data[AUDIO_CAPTURE_PERIOD_SIZE * AUDIO_CAPTURE_CHANNEL_COUNT * sizeof(int16_t)];
    audio_buffer_t buf;
    buf.frameCount = AUDIO_CAPTURE_PERIOD_SIZE;
    buf.s16 = data;
    bool capture_enabled = false;
    int ret;


    prctl(PR_SET_NAME, (unsigned long)"Visualizer Capture", 0, 0, 0);

    ALOGD("%s: Started running Visualizer Capture Thread", __func__);

    pthread_mutex_lock(&lock);
     while(!exit_thread) {
        if (effects_enabled()) {
            /* User start Visualizer, and compress offload playback is working */
            if (!capture_enabled) {
                pcm = pcm_open(SOUND_CARD, CAPTURE_DEVICE, PCM_IN, &pcm_config_capture);
                if (pcm && !pcm_is_ready(pcm)) {
                    ALOGE("%s: Failed to open PCM for Visualizer (%s)", __func__, pcm_get_error(pcm));
                    pcm_close(pcm);
                    pcm = NULL;
                } else {
                    ALOGD("%s: Opened PCM for Visualizer", __func__);
                    capture_enabled = true;
                }
            }
        } else {
            /* User stop Visualizer, but compress offload playback is working */
            if (capture_enabled) {
                if (pcm != NULL) {
                    pcm_close(pcm);
                    pcm = NULL;
                }
                capture_enabled = false;
            }
            ALOGD("%s: Compress Offload playback is working, but visualizer is not started yet. Wait!!!", __func__);
            pthread_cond_wait(&cond, &lock);
            ALOGD("%s: Compress Offload playback is working, and visualizer is started. Run!!!", __func__);
        }

        if (!capture_enabled)
            continue;

        pthread_mutex_unlock(&lock);
        ret = pcm_read(pcm, data, sizeof(data));
        pthread_mutex_lock(&lock);
        if (ret == 0) {
            struct listnode *out_node;

            list_for_each(out_node, &active_outputs_list) {
                output_context_t *out_ctxt = node_to_item(out_node, output_context_t, outputs_list_node);
                struct listnode *fx_node;

                list_for_each(fx_node, &out_ctxt->effects_list) {
                    effect_context_t *fx_ctxt = node_to_item(fx_node, effect_context_t, output_node);
                    if (fx_ctxt->ops.process != NULL)
                        fx_ctxt->ops.process(fx_ctxt, &buf, &buf);
                }
            }
        } else {
            if (pcm != NULL)
                ALOGW("%s: read status %d %s", __func__, ret, pcm_get_error(pcm));
            else
                ALOGW("%s: read status %d PCM Closed", __func__, ret);
        }
    }

    if (capture_enabled) {
        if (pcm != NULL) {
            pcm_close(pcm);
            pcm = NULL;
        }
        capture_enabled = false;
    }
    pthread_mutex_unlock(&lock);

    ALOGD("%s: Stopped Visualizer Capture Thread", __func__);
    return (void *)NULL;
}

/*
 * Interface from AudioHAL
 */
__attribute__ ((visibility ("default")))
int notify_start_output(audio_io_handle_t output)
{
    int ret = 0;
    struct listnode *node;
    output_context_t * out_ctxt = NULL;

    ALOGD("%s: called with Audio Output Handle (%u)", __func__, output);

    if (lib_init() != 0)
        return init_status;

    pthread_mutex_lock(&thread_lock);
    pthread_mutex_lock(&lock);
    if (get_output(output) != NULL) {
        ALOGW("%s output already started", __func__);
        ret = -ENOSYS;
    } else {
        out_ctxt = (output_context_t *) malloc(sizeof(output_context_t));
        if (out_ctxt) {
            ALOGD("%s: created Output Context for Audio Handle (%u)", __func__, output);
            out_ctxt->handle = output;

            list_init(&out_ctxt->effects_list);

            list_for_each(node, &created_effects_list) {
                effect_context_t *fx_ctxt = node_to_item(node, effect_context_t, effects_list_node);
                if (fx_ctxt->out_handle == output) {
                    ALOGD("%s: Start Effect Context for Audio Output Handle (%u)", __func__, output);
                    if (fx_ctxt->ops.start)
                        fx_ctxt->ops.start(fx_ctxt, out_ctxt);
                    list_add_tail(&out_ctxt->effects_list, &fx_ctxt->output_node);
                }
            }

            if (list_empty(&active_outputs_list)) {
                exit_thread = false;
                thread_status = pthread_create(&capture_thread, (const pthread_attr_t *) NULL, capture_thread_loop, NULL);
            }

            list_add_tail(&active_outputs_list, &out_ctxt->outputs_list_node);
            pthread_cond_signal(&cond);
        } else {
            ALOGE("%s: Failed to allocate memory for Output Context", __func__);
            ret = -ENOMEM;
        }
    }
    pthread_mutex_unlock(&lock);
    pthread_mutex_unlock(&thread_lock);

    return ret;
}

__attribute__ ((visibility ("default")))
int notify_stop_output(audio_io_handle_t output)
{
    int ret = 0;
    struct listnode *node;
    struct listnode *fx_node;
    output_context_t *out_ctxt;

    ALOGD("%s: called with Audio Output Handle (%u)", __func__, output);

    if (lib_init() != 0)
        return init_status;

    pthread_mutex_lock(&thread_lock);
    pthread_mutex_lock(&lock);
    out_ctxt = get_output(output);
    if (out_ctxt == NULL) {
        ALOGE("%s: This Audio Output Handle is not started", __func__);
        ret = -ENOSYS;
    } else {
        list_for_each(fx_node, &out_ctxt->effects_list) {
            effect_context_t *fx_ctxt = node_to_item(fx_node, effect_context_t, output_node);
            if (fx_ctxt->ops.stop)
                fx_ctxt->ops.stop(fx_ctxt, out_ctxt);
        }

        list_remove(&out_ctxt->outputs_list_node);
        pthread_cond_signal(&cond);

        if (list_empty(&active_outputs_list)) {
            if (thread_status == 0) {
                exit_thread = true;
                /* PCM should be closed here, since the active list is empty
                     pcm_read is returned immediately if it is wait in capture
                     thread */
                if (pcm != NULL) {
                    pcm_stop(pcm);
                    pcm_close(pcm);
                    pcm = NULL;
                    ALOGD("%s: Closed PCM for Visualizer", __func__);
                }
                pthread_mutex_unlock(&lock);
                pthread_cond_signal(&cond);
                pthread_join(capture_thread, (void **) NULL);
                pthread_mutex_lock(&lock);
                thread_status = -1;
            }
        }
        free(out_ctxt);
    }
    pthread_mutex_unlock(&lock);
    pthread_mutex_unlock(&thread_lock);

    return ret;
}

/*
** Effect operation functions
 */
int set_config(effect_context_t *context, effect_config_t *config)
{
    if (config->inputCfg.samplingRate != config->outputCfg.samplingRate) return -EINVAL;
    if (config->inputCfg.channels != config->outputCfg.channels) return -EINVAL;
    if (config->inputCfg.format != config->outputCfg.format) return -EINVAL;
    if (config->inputCfg.channels != AUDIO_CHANNEL_OUT_STEREO) return -EINVAL;
    if (config->outputCfg.accessMode != EFFECT_BUFFER_ACCESS_WRITE &&
            config->outputCfg.accessMode != EFFECT_BUFFER_ACCESS_ACCUMULATE) return -EINVAL;
    if (config->inputCfg.format != AUDIO_FORMAT_PCM_16_BIT) return -EINVAL;

    context->config = *config;

    if (context->ops.reset)
        context->ops.reset(context);

    return 0;
}

void get_config(effect_context_t *context, effect_config_t *config)
{
    *config = context->config;
}

uint32_t visualizer_get_delta_time_ms_from_updated_time(
        visualizer_context_t* visu_ctxt)
{
    uint32_t delta_ms = 0;
    if (visu_ctxt->buffer_update_time.tv_sec != 0) {
        struct timespec ts;

        if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
            time_t secs = ts.tv_sec - visu_ctxt->buffer_update_time.tv_sec;
            long nsec = ts.tv_nsec - visu_ctxt->buffer_update_time.tv_nsec;
            if (nsec < 0) {
                --secs;
                nsec += 1000000000;
            }
            delta_ms = secs * 1000 + nsec / 1000000;
        }
    }
    return delta_ms;
}

int visualizer_reset(effect_context_t *context)
{
    visualizer_context_t * visu_ctxt = (visualizer_context_t *)context;

    visu_ctxt->capture_idx = 0;
    visu_ctxt->last_capture_idx = 0;
    visu_ctxt->buffer_update_time.tv_sec = 0;
    visu_ctxt->latency = DSP_OUTPUT_LATENCY_MS;
    memset(visu_ctxt->capture_buf, 0x80, CAPTURE_BUF_SIZE);

    return 0;
}

int visualizer_enable(effect_context_t *context __unused)
{
    return 0;
}

int visualizer_disable(effect_context_t *context __unused)
{
    return 0;
}

int visualizer_start(
    effect_context_t *context __unused,
     output_context_t *output __unused)
{
    return 0;
}

int visualizer_stop(
    effect_context_t *context __unused,
    output_context_t *output __unused)
{
    return 0;
}

int visualizer_process(
        effect_context_t *context,
        audio_buffer_t *in_buf,
        audio_buffer_t *out_buf)
{
    visualizer_context_t *visu_ctxt = (visualizer_context_t *)context;

    if (!effect_exists(context))
        return -EINVAL;

    if (in_buf == NULL || in_buf->raw == NULL ||
        out_buf == NULL || out_buf->raw == NULL ||
        in_buf->frameCount != out_buf->frameCount ||
        in_buf->frameCount == 0) {
        return -EINVAL;
    }

    // perform measurements if needed
    if (visu_ctxt->meas_mode & MEASUREMENT_MODE_PEAK_RMS) {
        // find the peak and RMS squared for the new buffer
        uint32_t inIdx;
        int16_t max_sample = 0;
        float rms_squared_acc = 0;
        for (inIdx = 0 ; inIdx < in_buf->frameCount * visu_ctxt->channel_count ; inIdx++) {
            if (in_buf->s16[inIdx] > max_sample) {
                max_sample = in_buf->s16[inIdx];
            } else if (-in_buf->s16[inIdx] > max_sample) {
                max_sample = -in_buf->s16[inIdx];
            }
            rms_squared_acc += (in_buf->s16[inIdx] * in_buf->s16[inIdx]);
        }
        // store the measurement
        visu_ctxt->past_meas[visu_ctxt->meas_buffer_idx].peak_u16 = (uint16_t)max_sample;
        visu_ctxt->past_meas[visu_ctxt->meas_buffer_idx].rms_squared =
                rms_squared_acc / (in_buf->frameCount * visu_ctxt->channel_count);
        visu_ctxt->past_meas[visu_ctxt->meas_buffer_idx].is_valid = true;
        if (++visu_ctxt->meas_buffer_idx >= visu_ctxt->meas_wndw_size_in_buffers) {
            visu_ctxt->meas_buffer_idx = 0;
        }
    }

    /* all code below assumes stereo 16 bit PCM output and input */
    int32_t shift;

    if (visu_ctxt->scaling_mode == VISUALIZER_SCALING_MODE_NORMALIZED) {
        /* derive capture scaling factor from peak value in current buffer
                * this gives more interesting captures for display. */
        shift = 32;
        int len = in_buf->frameCount * 2;
        int i;
        for (i = 0; i < len; i++) {
            int32_t smp = in_buf->s16[i];
            if (smp < 0) smp = -smp - 1; /* take care to keep the max negative in range */
            int32_t clz = __builtin_clz(smp);
            if (shift > clz) shift = clz;
        }
        /* A maximum amplitude signal will have 17 leading zeros, which we want to
                * translate to a shift of 8 (for converting 16 bit to 8 bit) */
        shift = 25 - shift;
        /* Never scale by less than 8 to avoid returning unaltered PCM signal. */
        if (shift < 3) {
            shift = 3;
        }
        /* add one to combine the division by 2 needed after summing
                * left and right channels below */
        shift++;
    } else {
        assert(visu_ctxt->scaling_mode == VISUALIZER_SCALING_MODE_AS_PLAYED);
        shift = 9;
    }

    uint32_t capt_idx;
    uint32_t in_idx;
    uint8_t *buf = visu_ctxt->capture_buf;
    for (in_idx = 0, capt_idx = visu_ctxt->capture_idx;
         in_idx < in_buf->frameCount;
         in_idx++, capt_idx++) {
        if (capt_idx >= CAPTURE_BUF_SIZE) {
            /* wrap around */
            capt_idx = 0;
        }
        int32_t smp = in_buf->s16[2 * in_idx] + in_buf->s16[2 * in_idx + 1];
        smp = smp >> shift;
        buf[capt_idx] = ((uint8_t)smp)^0x80;
    }

    /* XXX the following two should really be atomic, though it probably doesn't
         * matter much for visualization purposes */
    visu_ctxt->capture_idx = capt_idx;
    /* update last buffer update time stamp */
    if (clock_gettime(CLOCK_MONOTONIC, &visu_ctxt->buffer_update_time) < 0) {
        visu_ctxt->buffer_update_time.tv_sec = 0;
    }

    if (context->state != EFFECT_STATE_ACTIVE) {
        ALOGV("%s DONE inactive", __func__);
        return -ENODATA;
    }

    return 0;
}

int visualizer_set_parameter(
        effect_context_t *context,
        effect_param_t *p,
        uint32_t size __unused)
{
    visualizer_context_t *visu_ctxt = (visualizer_context_t *)context;

    if (p->psize != sizeof(uint32_t) || p->vsize != sizeof(uint32_t))
        return -EINVAL;

    switch (*(uint32_t *)p->data) {
    case VISUALIZER_PARAM_CAPTURE_SIZE:
        visu_ctxt->capture_size = *((uint32_t *)p->data + 1);
        ALOGV("%s set capture_size = %d", __func__, visu_ctxt->capture_size);
        break;

    case VISUALIZER_PARAM_SCALING_MODE:
        visu_ctxt->scaling_mode = *((uint32_t *)p->data + 1);
        ALOGV("%s set scaling_mode = %d", __func__, visu_ctxt->scaling_mode);
        break;

    case VISUALIZER_PARAM_LATENCY:
        /* Ignore latency as we capture at DSP output
                  * visu_ctxt->latency = *((uint32_t *)p->data + 1); */
        ALOGV("%s set latency = %d", __func__, visu_ctxt->latency);
        break;

    case VISUALIZER_PARAM_MEASUREMENT_MODE:
        visu_ctxt->meas_mode = *((uint32_t *)p->data + 1);
        ALOGV("%s set meas_mode = %d", __func__, visu_ctxt->meas_mode);
        break;

    default:
        return -EINVAL;
    }

    return 0;
}

int visualizer_get_parameter(
        effect_context_t *context,
        effect_param_t *p,
        uint32_t *size)
{
    visualizer_context_t *visu_ctxt = (visualizer_context_t *)context;

    p->status = 0;
    *size = sizeof(effect_param_t) + sizeof(uint32_t);
    if (p->psize != sizeof(uint32_t)) {
        p->status = -EINVAL;
        return 0;
    }

    switch (*(uint32_t *)p->data) {
    case VISUALIZER_PARAM_CAPTURE_SIZE:
        ALOGV("%s get capture_size = %d", __func__, visu_ctxt->capture_size);
        *((uint32_t *)p->data + 1) = visu_ctxt->capture_size;
        p->vsize = sizeof(uint32_t);
        *size += sizeof(uint32_t);
        break;

    case VISUALIZER_PARAM_SCALING_MODE:
        ALOGV("%s get scaling_mode = %d", __func__, visu_ctxt->scaling_mode);
        *((uint32_t *)p->data + 1) = visu_ctxt->scaling_mode;
        p->vsize = sizeof(uint32_t);
        *size += sizeof(uint32_t);
        break;

    case VISUALIZER_PARAM_MEASUREMENT_MODE:
        ALOGV("%s get meas_mode = %d", __func__, visu_ctxt->meas_mode);
        *((uint32_t *)p->data + 1) = visu_ctxt->meas_mode;
        p->vsize = sizeof(uint32_t);
        *size += sizeof(uint32_t);
        break;

    default:
        p->status = -EINVAL;
    }

    return 0;
}

int visualizer_command(
        effect_context_t *context,
        uint32_t cmd_code,
        uint32_t cmd_size __unused,
        void *cmd_data __unused,
        uint32_t *reply_size,
        void *reply_data)
{
    visualizer_context_t * visu_ctxt = (visualizer_context_t *)context;
    int ret = 0;

    switch (cmd_code) {
    case VISUALIZER_CMD_CAPTURE: {
            if (reply_data == NULL || *reply_size != visu_ctxt->capture_size) {
                ret = -EINVAL;
                ALOGE("%s: Command(%u) has Invalid Parameter", __func__, cmd_code);
            } else {
                if (context->state == EFFECT_STATE_ACTIVE) {
                    int32_t latency_ms = visu_ctxt->latency;
                    const uint32_t delta_ms = visualizer_get_delta_time_ms_from_updated_time(visu_ctxt);
                    latency_ms -= delta_ms;

                    if (latency_ms < 0)
                        latency_ms = 0;

                    const uint32_t delta_smp = context->config.inputCfg.samplingRate * latency_ms / 1000;

                    int32_t capture_point = visu_ctxt->capture_idx - visu_ctxt->capture_size - delta_smp;
                    int32_t capture_size = visu_ctxt->capture_size;
                    if (capture_point < 0) {
                        int32_t size = -capture_point;
                        if (size > capture_size)
                            size = capture_size;

                        memcpy(reply_data, visu_ctxt->capture_buf + CAPTURE_BUF_SIZE + capture_point, size);
                        reply_data = (void *)((size_t)reply_data + size);
                        capture_size -= size;
                        capture_point = 0;
                    }
                    memcpy(reply_data, visu_ctxt->capture_buf + capture_point, capture_size);


                    /* if audio framework has stopped playing audio although the effect is still
                     * active we must clear the capture buffer to return silence */
                    if ((visu_ctxt->last_capture_idx == visu_ctxt->capture_idx)
                                && (visu_ctxt->buffer_update_time.tv_sec != 0)) {
                        if (delta_ms > MAX_STALL_TIME_MS) {
                            ALOGV("%s capture going to idle", __func__);
                            visu_ctxt->buffer_update_time.tv_sec = 0;
                            memset(reply_data, 0x80, visu_ctxt->capture_size);
                        }
                    }
                    visu_ctxt->last_capture_idx = visu_ctxt->capture_idx;
                } else {
                    memset(reply_data, 0x80, visu_ctxt->capture_size);
                }
            }
        }
        break;

    case VISUALIZER_CMD_MEASURE: {
            uint16_t peak_u16 = 0;
            float sum_rms_squared = 0.0f;
            uint8_t nb_valid_meas = 0;

            /* reset measurements if last measurement was too long ago (which implies stored
                          * measurements aren't relevant anymore and shouldn't bias the new one) */
            const int32_t delay_ms = visualizer_get_delta_time_ms_from_updated_time(visu_ctxt);
            if (delay_ms > DISCARD_MEASUREMENTS_TIME_MS) {
                uint32_t i;
                ALOGV("Discarding measurements, last measurement is %dms old", delay_ms);
                for (i=0 ; i<visu_ctxt->meas_wndw_size_in_buffers ; i++) {
                    visu_ctxt->past_meas[i].is_valid = false;
                    visu_ctxt->past_meas[i].peak_u16 = 0;
                    visu_ctxt->past_meas[i].rms_squared = 0;
                }
                visu_ctxt->meas_buffer_idx = 0;
            } else {
                /* only use actual measurements, otherwise the first RMS measure happening before
                                 * MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS have been played will always be artificially low */
                uint32_t i;
                for (i=0 ; i < visu_ctxt->meas_wndw_size_in_buffers ; i++) {
                    if (visu_ctxt->past_meas[i].is_valid) {
                        if (visu_ctxt->past_meas[i].peak_u16 > peak_u16) {
                            peak_u16 = visu_ctxt->past_meas[i].peak_u16;
                        }
                        sum_rms_squared += visu_ctxt->past_meas[i].rms_squared;
                        nb_valid_meas++;
                    }
                }
            }

            float rms = nb_valid_meas == 0 ? 0.0f : sqrtf(sum_rms_squared / nb_valid_meas);
            int32_t* p_int_reply_data = (int32_t*)reply_data;

            /* convert from I16 sample values to mB and write results */
            if (rms < 0.000016f) {
                p_int_reply_data[MEASUREMENT_IDX_RMS] = -9600; //-96dB
            } else {
                p_int_reply_data[MEASUREMENT_IDX_RMS] = (int32_t) (2000 * log10(rms / 32767.0f));
            }

            if (peak_u16 == 0) {
                p_int_reply_data[MEASUREMENT_IDX_PEAK] = -9600; //-96dB
            } else {
                p_int_reply_data[MEASUREMENT_IDX_PEAK] = (int32_t) (2000 * log10(peak_u16 / 32767.0f));
            }
            ALOGV("VISUALIZER_CMD_MEASURE peak=%d (%dmB), rms=%.1f (%dmB)", peak_u16, p_int_reply_data[MEASUREMENT_IDX_PEAK],rms, p_int_reply_data[MEASUREMENT_IDX_RMS]);
        }
        break;

    default:
        ALOGW("%s: Not supported Command %u", __func__, cmd_code);
        ret = -EINVAL;
        break;
    }

    return ret;
}

int visualizer_init(effect_context_t *context)
{
    visualizer_context_t * visu_ctxt = (visualizer_context_t *)context;
    int i;

    context->config.inputCfg.accessMode = EFFECT_BUFFER_ACCESS_READ;
    context->config.inputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    context->config.inputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    context->config.inputCfg.samplingRate = 44100;
    context->config.inputCfg.bufferProvider.getBuffer = NULL;
    context->config.inputCfg.bufferProvider.releaseBuffer = NULL;
    context->config.inputCfg.bufferProvider.cookie = NULL;
    context->config.inputCfg.mask = EFFECT_CONFIG_ALL;
    context->config.outputCfg.accessMode = EFFECT_BUFFER_ACCESS_ACCUMULATE;
    context->config.outputCfg.channels = AUDIO_CHANNEL_OUT_STEREO;
    context->config.outputCfg.format = AUDIO_FORMAT_PCM_16_BIT;
    context->config.outputCfg.samplingRate = 44100;
    context->config.outputCfg.bufferProvider.getBuffer = NULL;
    context->config.outputCfg.bufferProvider.releaseBuffer = NULL;
    context->config.outputCfg.bufferProvider.cookie = NULL;
    context->config.outputCfg.mask = EFFECT_CONFIG_ALL;

    visu_ctxt->capture_size = VISUALIZER_CAPTURE_SIZE_MAX;
    visu_ctxt->scaling_mode = VISUALIZER_SCALING_MODE_NORMALIZED;

    // measurement initialization
    visu_ctxt->channel_count = audio_channel_count_from_out_mask(context->config.inputCfg.channels);
    visu_ctxt->meas_mode = MEASUREMENT_MODE_NONE;
    visu_ctxt->meas_wndw_size_in_buffers = MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS;
    visu_ctxt->meas_buffer_idx = 0;

    for (i = 0; i < visu_ctxt->meas_wndw_size_in_buffers; i++) {
        visu_ctxt->past_meas[i].is_valid = false;
        visu_ctxt->past_meas[i].peak_u16 = 0;
        visu_ctxt->past_meas[i].rms_squared = 0;
    }

    set_config(context, &context->config);

    return 0;
}

int visualizer_release(effect_context_t *context __unused)
{
    return 0;
}

/*
 * Effect Control Interface Implementation
 */
/* Effect Control Interface Implementation: process */
int effect_process(
        effect_handle_t self,
        audio_buffer_t *in_buffer,
        audio_buffer_t *out_buffer)
{
    effect_context_t *context = (effect_context_t *)self;

    ALOGD("%s: called", __func__);

    if (context == NULL) {
        return -EINVAL;
    }

    if (in_buffer == NULL || in_buffer->raw == NULL ||
        out_buffer == NULL || out_buffer->raw == NULL ||
        in_buffer->frameCount != out_buffer->frameCount ||
        in_buffer->frameCount == 0) {
        return -EINVAL;
    }


    if (context->state != EFFECT_STATE_ACTIVE) {
        return -ENODATA;
    }

    return 0;
}

/* Effect Control Interface Implementation: command */
int effect_command(
        effect_handle_t self,
        uint32_t cmd_code,
        uint32_t cmd_size,
        void *cmd_data,
        uint32_t *reply_size,
        void *reply_data)
{
    effect_context_t *context = (effect_context_t *)self;
    int ret = 0;
    int retsize;

    ALOGV("%s: Effect(%s), Audio Output Handle (%d)", __func__, context->desc->name, context->out_handle);
    ALOGV("%s: command (%u)", __func__, cmd_code);

    if (context == NULL || context->state == EFFECT_STATE_UNINITIALIZED) {
        ALOGE("%s: Invalid Parameter", __func__);
        return -EINVAL;
    }

    switch (cmd_code) {
    case EFFECT_CMD_INIT:  /* o: cmd_size = 0, cmd_data = NULL */
        if (reply_data == NULL || *reply_size != sizeof(uint32_t)) {
            ret = -EINVAL;
            ALOGE("%s: Command(%u) has Invalid Parameter", __func__, cmd_code);
        } else {
            *(int *)reply_data = context->ops.init(context);
            context->state = EFFECT_STATE_INITIALIZED;
        }
        break;

    case EFFECT_CMD_SET_CONFIG: /* 1 */
        if (cmd_data == NULL || cmd_size != sizeof(effect_config_t)
                || reply_data == NULL || *reply_size != sizeof(uint32_t)) {
            ret = -EINVAL;
            ALOGE("%s: Command(%u) has Invalid Parameter", __func__, cmd_code);
        } else {
            *(int *) reply_data = set_config(context, (effect_config_t *)cmd_data);
        }
        break;

    case EFFECT_CMD_RESET: /* 2: cmd_size = 0, cmd_data = NULL, reply_size = 0, reply_data = NULL */
        if (context->ops.reset)
            context->ops.reset(context);
        break;

    case EFFECT_CMD_ENABLE: /* 3: cmd_size = 0, cmd_data = NULL */
        if (reply_data == NULL || *reply_size != sizeof(uint32_t)) {
            ret = -EINVAL;
            ALOGE("%s: Command(%u) has Invalid Parameter", __func__, cmd_code);
        } else if (context->state != EFFECT_STATE_INITIALIZED) {
            ret = -ENOSYS;
            ALOGE("%s: Command(%u) has Invalid State", __func__, context->state);
        } else {
            if (context->offload_enabled && context->ops.enable)
                context->ops.enable(context);

            pthread_cond_signal(&cond);
            context->state = EFFECT_STATE_ACTIVE;
            *(int *)reply_data = 0;
        }
        break;

    case EFFECT_CMD_DISABLE: /* 4: cmd_size = 0, cmd_data = NULL */
        if (reply_data == NULL || *reply_size != sizeof(uint32_t)) {
            ret = -EINVAL;
            ALOGE("%s: Command(%u) has Invalid Parameter", __func__, cmd_code);
        } else if (context->state != EFFECT_STATE_ACTIVE) {
            ret = -ENOSYS;
            ALOGE("%s: Command(%u) has Invalid State", __func__, context->state);
        } else {
            if (context->offload_enabled && context->ops.disable)
                context->ops.disable(context);

            pthread_cond_signal(&cond);
            context->state = EFFECT_STATE_INITIALIZED;
            *(int *)reply_data = 0;
        }
        break;

    case EFFECT_CMD_SET_PARAM: /* 5 */
        if (cmd_data == NULL ||
            cmd_size != (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(uint32_t)) ||
            reply_data == NULL || *reply_size != sizeof(uint32_t)) {
            ret = -EINVAL;
            ALOGE("%s: Command(%u) has Invalid Parameter", __func__, cmd_code);
        } else {
            *(int *)reply_data = 0;

            effect_param_t *p = (effect_param_t *)cmd_data;
            if (p->psize != sizeof(uint32_t) || p->vsize != sizeof(uint32_t)) {
                *(int *)reply_data = -EINVAL;
                ALOGE("%s: Parameter in Command(%u) has Invalid Size", __func__, cmd_code);
                break;
            } else {
                if (context->ops.set_parameter)
                    *(int32_t *)reply_data = context->ops.set_parameter(context, p, *reply_size);
            }
        }
        break;

    case EFFECT_CMD_SET_PARAM_DEFERRED: /* 6 */
    case EFFECT_CMD_SET_PARAM_COMMIT: /* 7 */
        break;

    case EFFECT_CMD_GET_PARAM: /* 8 */
        if (cmd_data == NULL ||
            cmd_size != (int)(sizeof(effect_param_t) + sizeof(uint32_t)) ||
            reply_data == NULL || *reply_size < (int)(sizeof(effect_param_t) + sizeof(uint32_t) + sizeof(uint32_t))) {
            ret = -EINVAL;
            ALOGE("%s: Command(%u) has Invalid Parameter", __func__, cmd_code);
        } else if (!context->offload_enabled) {
            ret = -EINVAL;
            ALOGE("%s: Command(%u) cannot applied because Offload not enabled yet", __func__, cmd_code);
        } else {
            memcpy(reply_data, cmd_data, sizeof(effect_param_t) + sizeof(uint32_t));

            effect_param_t *p = (effect_param_t *)reply_data;
            p->status = 0;
            *reply_size = sizeof(effect_param_t) + sizeof(uint32_t);
            if (p->psize != sizeof(uint32_t)) {
                p->status = -EINVAL;
                ALOGE("%s: Parameter in Command(%u) has Invalid Size", __func__, cmd_code);
                break;
            } else {
                if (context->ops.get_parameter)
                    context->ops.get_parameter(context, p, reply_size);
            }
        }
        break;

    case EFFECT_CMD_SET_DEVICE: /* 9 */
    case EFFECT_CMD_SET_VOLUME: /* 10 */
    case EFFECT_CMD_SET_AUDIO_MODE: /* 11 */
    case EFFECT_CMD_SET_CONFIG_REVERSE: /* 12 */
    case EFFECT_CMD_SET_INPUT_DEVICE: /* 13 */
        break;

    case EFFECT_CMD_GET_CONFIG: /* 14: cmd_size = 0, cmd_data = NULL */
        if (reply_data == NULL || *reply_size != sizeof(effect_config_t)) {
            ret = -EINVAL;
            ALOGE("%s: Command(%u) has Invalid Parameter", __func__, cmd_code);
        } else {
            get_config(context, (effect_config_t *)reply_data);
        }
        break;

    case EFFECT_CMD_GET_CONFIG_REVERSE: /* 15 */
    case EFFECT_CMD_GET_FEATURE_SUPPORTED_CONFIGS: /* 16 */
    case EFFECT_CMD_GET_FEATURE_CONFIG: /* 17 */
    case EFFECT_CMD_SET_FEATURE_CONFIG: /* 18 */
    case EFFECT_CMD_SET_AUDIO_SOURCE: /* 19 */
        break;

    case EFFECT_CMD_OFFLOAD: /* 20 */
        if (cmd_size != sizeof(effect_offload_param_t) || cmd_data == NULL
                    || reply_data == NULL || *reply_size != sizeof(int)) {
            ret = -EINVAL;
            ALOGE("%s: Command(%u) has Invalid Parameter", __func__, cmd_code);
        } else {
            effect_offload_param_t* offload_param = (effect_offload_param_t*)cmd_data;

            ALOGD("%s: Command(%u)= offload %d, output %d", __func__, cmd_code, offload_param->isOffload, offload_param->ioHandle);

            context->offload_enabled = offload_param->isOffload;
            if (context->out_handle == offload_param->ioHandle) {
                ALOGD("%s: Requested same Audio output", __func__);
            } else {
                ALOGD("%s: Requested to change Audio output from %d to %d", __func__, context->out_handle, offload_param->ioHandle);
                context->out_handle = offload_param->ioHandle;
            }
            *(int *)reply_data = 0;
        }
        break;

    default:
        if (cmd_code >= EFFECT_CMD_FIRST_PROPRIETARY && context->ops.command) {
            if (context->offload_enabled)
                ret = context->ops.command(context, cmd_code, cmd_size, cmd_data, reply_size, reply_data);
        } else {
            ALOGW("%s: Not supported Command %u", __func__, cmd_code);
            ret = -EINVAL;
        }
        break;
    }

    return ret;
}

/* Effect Control Interface Implementation: get_descriptor */
int effect_get_descriptor(effect_handle_t self, effect_descriptor_t *descriptor)
{
    effect_context_t *context = (effect_context_t *)self;

    ALOGD("%s: called", __func__);

    if (context == NULL || descriptor == NULL) {
        ALOGV("%s: invalid param", __func__);
        return -EINVAL;
    }

    if (!effect_exists(context))
        return -EINVAL;

    *descriptor = *context->desc;

    return 0;
}

/* effect_handle_t interface implementation for offload effects */
const struct effect_interface_s effect_interface = {
    effect_process,
    effect_command,
    effect_get_descriptor,
    NULL,
};

/**
 ** AudioEffectHAL Interface Implementation
**/
int effect_lib_create(
        const effect_uuid_t *uuid,
        int32_t session_id __unused,
        int32_t io_id,
        effect_handle_t *handle)
{
    effect_context_t *context = 0;
    int ret = 0, i;

    ALOGD("%s: called", __func__);

    if (lib_init() != 0)
        return init_status;

    if (handle == NULL || uuid == NULL) {
        ALOGE("%s: Invalid Parameters", __func__);
        return -EINVAL;
    }

    /* Check UUID from Descriptor List which this effect lib is supporting */
    for (i = 0; descriptors[i] != NULL; i++) {
        if (memcmp(uuid, &descriptors[i]->uuid, sizeof(effect_uuid_t)) == 0) {
            ALOGV("%s: got supported descriptor", __func__);
            break;
        }
    }
    if (descriptors[i] == NULL) {
        ALOGE("%s: Not supported UUID", __func__);
        return -EINVAL;
    }

    /* According to UUID,, make Effect Context */
    if (memcmp(uuid, &visualizer_descriptor.uuid, sizeof(effect_uuid_t)) == 0) {
        ALOGD("%s: Requested Visualizer for Audio IO(%d)", __func__, io_id);
        visualizer_context_t *visu_ctxt = (visualizer_context_t *)calloc(1, sizeof(visualizer_context_t));
        if (visu_ctxt) {
            context = (effect_context_t *)visu_ctxt;
            context->ops.init = visualizer_init;
            context->ops.release = visualizer_release;
            context->ops.reset = visualizer_reset;
            context->ops.enable = visualizer_enable;
            context->ops.disable = visualizer_disable;
            context->ops.start = visualizer_start;
            context->ops.stop = visualizer_stop;
            context->ops.process = visualizer_process;
            context->ops.set_parameter = visualizer_set_parameter;
            context->ops.get_parameter = visualizer_get_parameter;
            context->ops.command = visualizer_command;
            context->desc = &visualizer_descriptor;
        } else {
            ALOGE("%s: Failed to allocate memory for conxtext", __func__);
            ret = -ENOMEM;
        }
    } else {
        ALOGE("%s: Not supported Visualizer UUID", __func__);
        ret = -EINVAL;
    }

    if (ret == 0) {
        context->itfe = &effect_interface;
        context->state = EFFECT_STATE_UNINITIALIZED;
        context->out_handle = (audio_io_handle_t)io_id;

        ret = context->ops.init(context);
        if (ret < 0) {
            ALOGW("%s init failed", __func__);
            free(context);
            return ret;
        }

        context->state = EFFECT_STATE_INITIALIZED;

        pthread_mutex_lock(&lock);
        list_add_tail(&created_effects_list, &context->effects_list_node);
        output_context_t *out_ctxt = get_output(io_id);
        if (out_ctxt != NULL) {
            ALOGD("%s: Got Output Context for Audio Output Handle (%d)", __func__, io_id);
            add_effect_to_output(out_ctxt, context);
        }
        pthread_mutex_unlock(&lock);

        *handle = (effect_handle_t)context;
    }

    ALOGD("%s created context %p", __func__, context);
    return ret;
}

int effect_lib_release(effect_handle_t handle)
{
    effect_context_t *context = (effect_context_t *)handle;
    int ret = 0;

    ALOGD("%s: called", __func__);

    if (lib_init() != 0)
        return init_status;

    if (handle == NULL) {
        ALOGE("%s: Invalid Parameters", __func__);
        ret = -EINVAL;
    } else {
        pthread_mutex_lock(&lock);
        if (effect_exists(context)) {
            output_context_t *out_ctxt = get_output(context->out_handle);
            if (out_ctxt != NULL)
                remove_effect_from_output(out_ctxt, context);
            list_remove(&context->effects_list_node);
            ALOGD("%s: Remove effect context from Effect List", __func__);

            if (context->ops.release)
                context->ops.release(context);
            context->state = EFFECT_STATE_UNINITIALIZED;
            free(context);
        }
        pthread_mutex_unlock(&lock);
    }

    return ret;
}

int effect_lib_get_descriptor(
        const effect_uuid_t *uuid,
        effect_descriptor_t *descriptor)
{
    int ret = 0, i;

    ALOGD("%s: called", __func__);

    if (lib_init() != 0)
        return init_status;

    if (descriptor == NULL || uuid == NULL) {
        ALOGV("%s: called with NULL pointer", __func__);
        ret = -EINVAL;
    } else {
        for (i = 0; descriptors[i] != NULL; i++) {
            if (memcmp(uuid, &descriptors[i]->uuid, sizeof(effect_uuid_t)) == 0) {
                ALOGV("%s: got supported descriptor", __func__);
                memcpy((void *)descriptor, (const void *)descriptors[i], sizeof(effect_descriptor_t));
                //*descriptor = *descriptors[i];
                break;
            }
        }
        if (descriptors[i] == NULL) {
            ALOGE("%s: Not supported UUID", __func__);
            ret = -EINVAL;
        }
    }

    return ret;
}


/* This is the only symbol that needs to be exported */
__attribute__ ((visibility ("default")))
audio_effect_library_t AUDIO_EFFECT_LIBRARY_INFO_SYM = {
    .tag = AUDIO_EFFECT_LIBRARY_TAG,
    .version = EFFECT_LIBRARY_API_VERSION,
    .name = "Exynos Offload Visualizer HAL",
    .implementor = "Samsung SystemLSI",
    .create_effect = effect_lib_create,
    .release_effect = effect_lib_release,
    .get_descriptor = effect_lib_get_descriptor,
};
