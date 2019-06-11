/* rm69080_mipi_lcd.c
 *
 * Samsung SoC MIPI LCD driver.
 *
 * Copyright (c) 2015 Samsung Electronics
 *
 * Haowei Li, <haowei.li@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/delay.h>
#include <linux/gpio.h>
#include <video/mipi_display.h>
#include <linux/platform_device.h>

#include "../../../video/fbdev/exynos/decon_7570/dsim.h"
#include "../../../video/fbdev/exynos/decon_7570/panels/lcd_ctrl.h"
#include "../../../video/fbdev/exynos/decon_7570/panels/decon_lcd.h"
#include "rm69080_param.h"

//#define GAMMA_PARAM_SIZE 26
#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 0
#define DEFAULT_BRIGHTNESS 72

struct backlight_device *bd;
#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend    rm69080_early_suspend;
#endif

static int rm69080_get_brightness(struct backlight_device *bd)
{
	return bd->props.brightness;
}

static int update_brightness(int brightness)
{
	lcd_brightness_set(brightness);
	return 0;
}

static int rm69080_set_brightness(struct backlight_device *bd)
{
	int brightness = bd->props.brightness;

	if (brightness < MIN_BRIGHTNESS || brightness > MAX_BRIGHTNESS) {
		printk(KERN_ALERT "Brightness should be in the range of 0 ~ 255\n");
		return -EINVAL;
	}

	update_brightness(brightness);

	return 0;
}

static const struct backlight_ops rm69080_backlight_ops = {
	.get_brightness = rm69080_get_brightness,
	.update_status = rm69080_set_brightness,
};

static int rm69080_probe(struct dsim_device *dsim)
{
	printk(KERN_ERR "rm69080_probe \n");
	bd = backlight_device_register("pwm-backlight.0", NULL,
		NULL, &rm69080_backlight_ops, NULL);
	if (IS_ERR(bd))
		printk(KERN_ALERT "failed to register backlight device!\n");

	bd->props.max_brightness = MAX_BRIGHTNESS;
	bd->props.brightness = DEFAULT_BRIGHTNESS;

	return 0;
}

static int rm69080_displayon(struct dsim_device *dsim)
{
	printk(KERN_ERR "rm69080_displayon \n");
	lcd_init(&dsim->lcd_info);
	return 0;
}

static int rm69080_suspend(struct dsim_device *dsim)
{
	printk(KERN_ERR "rm69080_suspend \n");
	lcd_disable();
	return 0;
}

static int rm69080_resume(struct dsim_device *dsim)
{
	printk(KERN_ERR "rm69080_resume\n");
	//lcd_init(&dsim->lcd_info);
	return 0;
}

struct mipi_dsim_lcd_driver rm69080_mipi_lcd_driver = {
	.probe		= rm69080_probe,
	.displayon	= rm69080_displayon,
	.suspend	= rm69080_suspend,
	.resume		= rm69080_resume,
};
