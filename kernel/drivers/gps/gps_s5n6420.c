/*
 * Driver for GPS-S5N6420
 *
 * Copyright (c) 2015 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/module.h>
#include <linux/rfkill.h>

static unsigned int gps_pwr_on;
static unsigned int gps_reset;
static struct rfkill *gps_rfkill;
static struct rfkill *gps_reset_rfkill;

static int s5n6420_gps_rfkill_set_power(void *data, bool blocked)
{
	if (!blocked) {
		gpio_set_value(gps_pwr_on, 1);
	} else {
		gpio_set_value(gps_pwr_on, 0);
	}
	return 0;
}

static int s5n6420_gps_reset_rfkill_set_power(void *data, bool blocked)
{
	if (!blocked) {
		gpio_set_value(gps_reset, 1);
	} else {
		gpio_set_value(gps_reset, 0);
	}
	return 0;
}

static const struct rfkill_ops s5n6420_gps_rfkill_ops = {
	.set_block = s5n6420_gps_rfkill_set_power,
};

static const struct rfkill_ops s5n6420_gps_reset_rfkill_ops = {
	.set_block = s5n6420_gps_reset_rfkill_set_power,
};

static int gps_s5n6420_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node;
	int ret;

	printk(KERN_ERR "gps_s5n6420_probe\n");
	node = dev->of_node;
	if (!node)
		return -ENODEV;

	gps_pwr_on = of_get_gpio(node, 0);
	if (gpio_is_valid(gps_pwr_on))
	{
		ret = gpio_request(gps_pwr_on, "GPS_PWR_EN");
		if (ret) {
			dev_err(dev, "cannot get GPS_PWR_EN GPIO\n");
			return ret;
		}
		gpio_direction_output(gps_pwr_on, 0);
		gpio_export(gps_pwr_on, 1);
		gpio_export_link(dev, "GPS_PWR_EN", gps_pwr_on);
	}
	else
	{
		dev_warn(dev, "GPIO not specified in DT (of_get_gpio returned %d)\n", gps_pwr_on);
		return -ENOENT;
	}

	gps_reset = of_get_gpio(node, 1);
	if (gpio_is_valid(gps_reset))
	{
		ret = gpio_request(gps_reset, "GPS_RESET");
		if (ret) {
			dev_err(dev, "cannot get GPS_RESET GPIO\n");
			return ret;
		}
		gpio_direction_output(gps_reset, 0);
		gpio_export(gps_reset, 1);
		gpio_export_link(dev, "GPS_RESET", gps_reset);
	}
	else
	{
		gpio_unexport(gps_pwr_on);
		gpio_free(gps_pwr_on);

		dev_warn(dev, "GPIO not specified in DT (of_get_gpio returned %d)\n", gps_reset);
		return -ENOENT;
	}

	gps_rfkill = rfkill_alloc("s5n6420 gps", &pdev->dev, RFKILL_TYPE_GPS, &s5n6420_gps_rfkill_ops, NULL);
	if (!gps_rfkill) {
		gpio_unexport(gps_reset);
		gpio_free(gps_reset);

		gpio_unexport(gps_pwr_on);
		gpio_free(gps_pwr_on);

		dev_warn(dev, "gps_rfkill alloc failed\n");
		return -ENOMEM;
	}
	rfkill_init_sw_state(gps_rfkill, 0);
	ret = rfkill_register(gps_rfkill);
	if (ret) {
		rfkill_destroy(gps_rfkill);

		gpio_unexport(gps_reset);
		gpio_free(gps_reset);

		gpio_unexport(gps_pwr_on);
		gpio_free(gps_pwr_on);

		dev_warn(dev, "gps_rfkill register failed\n");

		return -1;
	}
	//rfkill_set_sw_state(gps_rfkill, true);

	gps_reset_rfkill = rfkill_alloc("s5n6420 gps reset", &pdev->dev, RFKILL_TYPE_GPS, &s5n6420_gps_reset_rfkill_ops, NULL);
	if (!gps_reset_rfkill) {
		rfkill_unregister(gps_rfkill);
		rfkill_destroy(gps_rfkill);

		gpio_unexport(gps_reset);
		gpio_free(gps_reset);

		gpio_unexport(gps_pwr_on);
		gpio_free(gps_pwr_on);

		dev_warn(dev, "gps_rfkill alloc failed\n");
		return -ENOMEM;
	}
	rfkill_init_sw_state(gps_reset_rfkill, 0);
	ret = rfkill_register(gps_reset_rfkill);
	if (ret) {
		rfkill_destroy(gps_reset_rfkill);

		rfkill_unregister(gps_rfkill);
		rfkill_destroy(gps_rfkill);

		gpio_unexport(gps_reset);
		gpio_free(gps_reset);

		gpio_unexport(gps_pwr_on);
		gpio_free(gps_pwr_on);

		dev_warn(dev, "gps_rfkill register failed\n");

		return -1;
	}
	//rfkill_set_sw_state(gps_reset_rfkill, true);
	return 0;
}

static int gps_s5n6420_remove(struct platform_device *pdev)
{
	rfkill_unregister(gps_rfkill);
	rfkill_destroy(gps_rfkill);

	rfkill_unregister(gps_reset_rfkill);
	rfkill_destroy(gps_reset_rfkill);

	gpio_unexport(gps_reset);
	gpio_free(gps_reset);

	gpio_unexport(gps_pwr_on);
	gpio_free(gps_pwr_on);

	return 0;
}

static const struct of_device_id gps_s5n6420_of_match[] = {
	{ .compatible = "samsung,gps-s5n6420", },
	{ },
};
MODULE_DEVICE_TABLE(of, gps_s5n6420_of_match);

static struct platform_driver gps_s5n6420_driver = {
	.driver		= {
		.name	= "gps-s5n6420",
		.of_match_table = of_match_ptr(gps_s5n6420_of_match),
	},
	.probe		= gps_s5n6420_probe,
	.remove		= gps_s5n6420_remove,
};

static int __init gps_s5n6420_init(void)
{
	return platform_driver_register(&gps_s5n6420_driver);
}

static void __exit gps_s5n6420_exit(void)
{
	platform_driver_unregister(&gps_s5n6420_driver);
}

device_initcall(gps_s5n6420_init);
module_exit(gps_s5n6420_exit);
