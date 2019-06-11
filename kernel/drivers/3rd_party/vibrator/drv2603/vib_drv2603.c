/* Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/hrtimer.h>
#include <linux/of_device.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include "../../../staging/android/timed_output.h"

#define DRV2603_VIB_DEFAULT_TIMEOUT	15000

struct drv2603_vib {
	struct platform_device *pdev;
	struct hrtimer vib_timer;
	struct timed_output_dev timed_dev;
	struct work_struct work;
	struct regulator *drv2603_reg;
	int state;
	int timeout;
	struct mutex lock;
};

static int drv2603_vib_set(struct drv2603_vib *vib, int on)
{
	int rc = 0;

	if (!vib) {
		return -EINVAL;
	}

	if (on) {
		if (!regulator_is_enabled(vib->drv2603_reg)) {
			rc = regulator_enable(vib->drv2603_reg);
		}
	} else {
		if (regulator_is_enabled(vib->drv2603_reg)) {
			rc = regulator_disable(vib->drv2603_reg);
		}
	}

	if (rc) {
		printk(KERN_ERR "vib: drv2603_vib_set failed.\n");
		return rc;
	}

	return 0;
}

static enum hrtimer_restart drv2603_vib_timer_func(struct hrtimer *timer)
{
	struct drv2603_vib *vib = container_of(timer, struct drv2603_vib, vib_timer);

	if (vib) {
		vib->state = 0;
		schedule_work(&vib->work);
	}

	return HRTIMER_NORESTART;
}

static void drv2603_vib_enable(struct timed_output_dev *dev, int value)
{
	struct drv2603_vib *vib = container_of(dev, struct drv2603_vib, timed_dev);

	if (!vib) {
		return;
	}

	mutex_lock(&vib->lock);
	hrtimer_cancel(&vib->vib_timer);

	if (value == 0) {
		vib->state = 0;
	} else {
		vib->state = 1;
		value = (value > vib->timeout ? vib->timeout : value);
		hrtimer_start(&vib->vib_timer,
				ktime_set(value / 1000, (value % 1000) * 1000000),
				HRTIMER_MODE_REL);
	}
	mutex_unlock(&vib->lock);
	schedule_work(&vib->work);
}

static void drv2603_vib_update(struct work_struct *work)
{
	struct drv2603_vib *vib = container_of(work, struct drv2603_vib, work);

	if (vib) {
		drv2603_vib_set(vib, vib->state);
	}
}

static int drv2603_vib_get_time(struct timed_output_dev *dev)
{
	struct drv2603_vib *vib = container_of(dev, struct drv2603_vib, timed_dev);

	if (!vib) {
		return -EINVAL;
	}

	if (hrtimer_active(&vib->vib_timer)) {
		ktime_t r = hrtimer_get_remaining(&vib->vib_timer);

		return (int)ktime_to_us(r);
	} else {
		return 0;
	}
}

#ifdef CONFIG_PM
static int drv2603_vibrator_suspend(struct device *dev)
{
	struct drv2603_vib *vib = dev_get_drvdata(dev);

	if (!vib) {
		return -EINVAL;
	}

	hrtimer_cancel(&vib->vib_timer);
	cancel_work_sync(&vib->work);
	/* turn-off vibrator */
	drv2603_vib_set(vib, 0);

	return 0;
}
#endif
static SIMPLE_DEV_PM_OPS(drv2603_vibrator_pm_ops, drv2603_vibrator_suspend, NULL);

static int drv2603_vib_parse_dt(struct drv2603_vib *vib)
{
	struct platform_device *pdev = NULL;
	int rc = 0;
	u32 temp_val;

	if (!vib) {
		return -EINVAL;
	}

	pdev = vib->pdev;
	vib->timeout = DRV2603_VIB_DEFAULT_TIMEOUT;
	rc = of_property_read_u32(pdev->dev.of_node,
			"ti,vib-timeout-ms", &temp_val);
	if (!rc) {
		vib->timeout = temp_val;
	} else if (rc != -EINVAL) {
		printk(KERN_ERR "vib: drv2603_vib_parse_dt failed.\n");
		return rc;
	}

	return 0;
}

static int drv2603_vibrator_probe(struct platform_device *pdev)
{
	struct drv2603_vib *vib;
	int rc = 0;

	if (!pdev) {
		return -EINVAL;
	}

	vib = devm_kzalloc(&pdev->dev, sizeof(*vib), GFP_KERNEL);
	if (!vib) {
		return -ENOMEM;
	}
	vib->pdev = pdev;

	rc = drv2603_vib_parse_dt(vib);
	if (rc) {
		printk(KERN_ERR "vib: drv2603_vib_parse_dt failed.\n");
		return rc;
	}

	vib->state = 0;
	mutex_init(&vib->lock);
	INIT_WORK(&vib->work, drv2603_vib_update);

	vib->drv2603_reg = devm_regulator_get(&pdev->dev, "drv2603");

	hrtimer_init(&vib->vib_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	vib->vib_timer.function = drv2603_vib_timer_func;

	vib->timed_dev.name = "vibrator";
	vib->timed_dev.get_time = drv2603_vib_get_time;
	vib->timed_dev.enable = drv2603_vib_enable;

	dev_set_drvdata(&pdev->dev, vib);

	rc = timed_output_dev_register(&vib->timed_dev);
	if (rc < 0) {
		printk(KERN_ERR "vib: timed_output_dev_register failed");
		return rc;
	}

	return rc;
}

static int drv2603_vibrator_remove(struct platform_device *pdev)
{
	struct drv2603_vib *vib = NULL;

	if (!pdev) {
		return -EINVAL;
	}
	vib = dev_get_drvdata(&pdev->dev);
	cancel_work_sync(&vib->work);
	hrtimer_cancel(&vib->vib_timer);
	timed_output_dev_unregister(&vib->timed_dev);
	mutex_destroy(&vib->lock);
	return 0;
}

static struct of_device_id drv2603_of_match[] = {
	{	.compatible = "ti,vib-drv2603",	},
	{}
};
MODULE_DEVICE_TABLE(of, drv2603_of_match);

static struct platform_driver drv2603_vibrator_driver = {
	.driver		= {
		.name	= "vib-drv2603",
		.of_match_table = of_match_ptr(drv2603_of_match),
		.pm	= &drv2603_vibrator_pm_ops,
	},
	.probe		= drv2603_vibrator_probe,
	.remove		= drv2603_vibrator_remove,
};

static int __init drv2603_vibrator_init(void)
{
	return platform_driver_register(&drv2603_vibrator_driver);
}

static void __exit drv2603_vibrator_exit(void)
{
	return platform_driver_unregister(&drv2603_vibrator_driver);
}

device_initcall(drv2603_vibrator_init);
module_exit(drv2603_vibrator_exit);
