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

#include <cutils/list.h>
#include <cutils/log.h>
#include <system/thread_defs.h>

#include <hardware/audio_effect.h>
#include <tinyalsa/asoundlib.h>
#include <audio_effects/effect_visualizer.h>


enum {
	EFFECT_STATE_UNINITIALIZED,
	EFFECT_STATE_INITIALIZED,
	EFFECT_STATE_ACTIVE,
};

typedef struct effect_context_s effect_context_t;
typedef struct output_context_s output_context_t;


/* effect specific operations. Only the init() and process() operations must be defined.
 * Others are optional.
 */
typedef struct effect_ops_s {
    int (*init)(effect_context_t *context);
    int (*release)(effect_context_t *context);
    int (*reset)(effect_context_t *context);
    int (*enable)(effect_context_t *context);
    int (*disable)(effect_context_t *context);
    int (*start)(effect_context_t *context, output_context_t *output);
    int (*stop)(effect_context_t *context, output_context_t *output);
    int (*process)(effect_context_t *context, audio_buffer_t *in, audio_buffer_t *out);
    int (*set_parameter)(effect_context_t *context, effect_param_t *param, uint32_t size);
    int (*get_parameter)(effect_context_t *context, effect_param_t *param, uint32_t *size);
    int (*command)(effect_context_t *context, uint32_t cmd_code, uint32_t cmd_size, void *cmd_data, uint32_t *reply_size, void *reply_data);
} effect_ops_t;

struct effect_context_s {
    const struct effect_interface_s *itfe;

    struct listnode effects_list_node;  /* node in created_effects_list */
    struct listnode output_node;  /* node in output_context_t.effects_list */
    effect_config_t config;
    const effect_descriptor_t *desc;
    audio_io_handle_t out_handle;  /* io handle of the output the effect is attached to */
    uint32_t state;
    bool offload_enabled;  /* when offload is enabled we process VISUALIZER_CMD_CAPTURE command.
                              Otherwise non offloaded visualizer has already processed the command
                              and we must not overwrite the reply. */
    effect_ops_t ops;
};

typedef struct output_context_s {
    struct listnode outputs_list_node;  /* node in active_outputs_list */
    audio_io_handle_t handle; /* io handle */
    struct listnode effects_list; /* list of effects attached to this output */
} output_context_t;



/* maximum time since last capture buffer update before resetting capture buffer. This means
   that the framework has stopped playing audio and we must start returning silence */
#define MAX_STALL_TIME_MS 1000

#define CAPTURE_BUF_SIZE 65536 /* "64k should be enough for everyone" */

#define DISCARD_MEASUREMENTS_TIME_MS 2000 /* discard measurements older than this number of ms */

/* maximum number of buffers for which we keep track of the measurements */
#define MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS 25 /* note: buffer index is stored in uint8_t */

typedef struct buffer_stats_s {
    bool is_valid;
    uint16_t peak_u16; /* the positive peak of the absolute value of the samples in a buffer */
    float rms_squared; /* the average square of the samples in a buffer */
} buffer_stats_t;

typedef struct visualizer_context_s {
    effect_context_t common;

    uint32_t capture_idx;
    uint32_t capture_size;
    uint32_t scaling_mode;
    uint32_t last_capture_idx;
    uint32_t latency;
    struct timespec buffer_update_time;
    uint8_t capture_buf[CAPTURE_BUF_SIZE];
    /* for measurements */
    uint8_t channel_count; /* to avoid recomputing it every time a buffer is processed */
    uint32_t meas_mode;
    uint8_t meas_wndw_size_in_buffers;
    uint8_t meas_buffer_idx;
    buffer_stats_t past_meas[MEASUREMENT_WINDOW_MAX_SIZE_IN_BUFFERS];
} visualizer_context_t;


#define DSP_OUTPUT_LATENCY_MS 0 /* Fudge factor for latency after capture point in audio DSP */

#define SOUND_CARD 0
#define CAPTURE_DEVICE 8

/* Proxy port supports only MMAP read and those fixed parameters*/
#define AUDIO_CAPTURE_CHANNEL_COUNT 2
#define AUDIO_CAPTURE_SMP_RATE 48000
#define AUDIO_CAPTURE_PERIOD_SIZE 1024
#define AUDIO_CAPTURE_PERIOD_COUNT 4

struct pcm_config pcm_config_capture = {
    .channels = AUDIO_CAPTURE_CHANNEL_COUNT,
    .rate = AUDIO_CAPTURE_SMP_RATE,
    .period_size = AUDIO_CAPTURE_PERIOD_SIZE,
    .period_count = AUDIO_CAPTURE_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .start_threshold = AUDIO_CAPTURE_PERIOD_SIZE / 4,
    .stop_threshold = INT_MAX,
    .avail_min = AUDIO_CAPTURE_PERIOD_SIZE / 4,
};
