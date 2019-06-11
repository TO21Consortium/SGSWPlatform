/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_TAG "lights"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include <cutils/log.h>
#include <hardware/lights.h>

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

char const *const LCD_FILE = "/sys/class/backlight/pwm-backlight.0/brightness";

static int write_int(char const *path, int value)
{
	int fd;
	static int already_warned;

	ALOGE("write_int: path %s, value %d", path, value);
	fd = open(path, O_RDWR);

	if (fd >= 0) {
		char buffer[20];
		int bytes = snprintf(buffer, sizeof(buffer), "%d\n", value);
		int amt = write(fd, buffer, bytes);
		close(fd);
		return amt == -1 ? -errno : 0;
	} else {
		if (already_warned == 0) {
			ALOGE("write_int failed to open %s\n", path);
			already_warned = 1;
		}
		return -errno;
	}
}

static int rgb_to_brightness(struct light_state_t const *state)
{
	/* use max of the RGB components for brightness */
	int color = state->color & 0x00ffffff;
	int red = (color >> 16) & 0x000000ff;
	int green = (color >> 8) & 0x000000ff;
	int blue = color & 0x000000ff;

	int brightness = red;
	if (green > brightness)
		brightness = green;
	if (blue > brightness)
		brightness = blue;

	return brightness;
}

static int set_light_backlight(struct light_device_t *dev,
			struct light_state_t const *state)
{
	int err = 0;
	int brightness = rgb_to_brightness(state);

	pthread_mutex_lock(&g_lock);
	err = write_int(LCD_FILE, brightness);

	pthread_mutex_unlock(&g_lock);
	return err;
}

static int close_lights(struct hw_device_t *dev)
{
	ALOGV("close_light is called");
	free(dev);

	return 0;
}

static int open_lights(const struct hw_module_t *module, char const *name,
						struct hw_device_t **device)
{
	int (*set_light)(struct light_device_t *dev,
		struct light_state_t const *state);

	if (strcmp(LIGHT_ID_BACKLIGHT, name) == 0)
		set_light = set_light_backlight;
	else
		return -EINVAL;

	struct light_device_t *dev = malloc(sizeof(struct light_device_t));
	if (!dev)
		return -ENOMEM;
	memset(dev, 0, sizeof(*dev));

	dev->common.tag = HARDWARE_DEVICE_TAG;
	dev->common.version = 0;
	dev->common.module = (struct hw_module_t *)module;
	dev->common.close = close_lights;
	dev->set_light = set_light;

	*device = (struct hw_device_t *)dev;

	return 0;
}

static struct hw_module_methods_t lights_module_methods = {
	.open = open_lights,
};

struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = LIGHTS_HARDWARE_MODULE_ID,
	.name = "lights Module",
	.author = "Google, Inc.",
	.methods = &lights_module_methods,
};
