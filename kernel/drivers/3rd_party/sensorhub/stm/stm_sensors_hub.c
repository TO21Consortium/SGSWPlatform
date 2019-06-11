/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
*
* File Name		: stm_sensors_hub.c
* Authors		: MEMS Motion Sensors Products Div- Application Team
*			: Both authors are willing to be considered the contact
*			: and update points for the driver.
* Version		: V 1.2.2 sysfs
* Date			: 2013/08/09
* Description		: STM SENSOR HUB digital output stm sensor hub sensor API
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
********************************************************************************
* REVISON HISTORY
*
* VERSION	| DATE		| AUTHORS	  | DESCRIPTION
* 1.0		| 2010/May/02	| STM		  | Corrects ODR Bug
* 1.1.4		| 2011/Sep/02	| STM		  | SMB Bus Mng,
*		|		|		  | forces BDU setting
* 1.1.5		| 2011/Sep/24	| STM		  | Introduces FIFO Feat.
* 1.1.5.2	| 2011/Nov/11	| STM		  | enable gpio_int to be
*		|		|		  | passed as parameter at
*		|		|		  | module loading time;
*		|		|		  | corrects polling
*		|		|		  | bug at end of probing;
* 1.1.5.3	| 2011/Dec/20	| STM		  | corrects error in
*		|		|		  | I2C SADROOT; Modifies
*		|		|		  | resume suspend func.
* 1.1.5.4	| 2012/Jan/09	| STM		  | moved under input/misc;
* 1.1.5.5	| 2012/Mar/30	|		  | moved watermark, use_smbus,
*		|		|		  | fifomode @ struct foo_status
*		|		|		  | sysfs range input format
*		|		|		  | changed to decimal
* 1.2		| 2012/Jul/10	| STM		  | input_poll_dev removal
* 1.2.1		| 2012/Jul/10	| STM		  | added high resolution timers
* 1.2.2		| 2013/Jul/06	| STM		  | LSI mod, removed FIFO
*******************************************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/stat.h>
#include <linux/delay.h>
#ifdef CONFIG_OF
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#endif
#include <linux/interrupt.h>

#include "stm_sensors_hub.h"
#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>

//#define DEBUG

/* Maximum polled-device-reported rot speed value value in dps */
#define MS_TO_NS(x)		(x*1000000L)

#define HUB_MIN			0x0000
#define HUB_MAX			0xFFFF
#define FUZZ			0
#define FLAT			0

#define I2C_AUTO_INCREMENT	0x80

/* SENSOR HUB COMMAND */
#define HUB_READ_ALL_CMD	0x01

//#define HUB_AP_INT
static int read_flag = 0;
struct workqueue_struct *stm_sensors_hub_workqueue = 0;
struct stm_sensors_hub_status *gstat;
static void stm_sensors_hub_report_triple(struct stm_sensors_hub_status *stat);
int yesterdayStep = 0, todayStep = 0;
int step_index = 0;
int resume_flag = 0;
int enable_num = 0;
struct stm_sensors_hub_status {
	struct i2c_client *client;
	struct stm_sensors_hub_platform_data *pdata;

	struct mutex lock;
	//struct wake_lock wake_lock;

	wait_queue_head_t read_wait;

	struct input_dev *input_dev;

	int hw_initialized;
	atomic_t enabled;
	int use_smbus;

	bool polling_enabled;

	struct hrtimer hr_timer;
	ktime_t ktime;
	struct work_struct polling_task;

	u8 output_format;

	int hub_rst_n_gpio;
	int hub_boot0_gpio;
	int hub_boot1_gpio;
	int ap_int_hub_gpio;
	int hub_int_ap_gpio;
	struct notifier_block nb;
	u8 curr_time[4];


};


static int stm_sensors_hub_i2c_read(struct stm_sensors_hub_status *stat, u8 *buf,
									int len)
{
	int ret;
	ret = i2c_master_recv(stat->client, buf, len);
	return ret;
}

#if 1
static int stm_sensors_hub_i2c_write(struct stm_sensors_hub_status *stat, u8 *buf,
									int len)
{
	int ret;
#if 0
	u8 reg, value;

	if (len > 1)
		buf[0] = (I2C_AUTO_INCREMENT | buf[0]);

	reg = buf[0];
	value = buf[1];

	if (stat->use_smbus) {
		if (len == 1) {
			ret = i2c_smbus_write_byte_data(stat->client,
								reg, value);
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_write_byte_data: ret=%d, len:%d, "
				"command=0x%02x, value=0x%02x\n",
				ret, len, reg , value);
#endif
			return ret;
		} else if (len > 1) {
			ret = i2c_smbus_write_i2c_block_data(stat->client,
							reg, len, buf + 1);
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_write_i2c_block_data: ret=%d, "
				"len:%d, command=0x%02x, ",
				ret, len, reg);
			unsigned int ii;
			for (ii = 0; ii < (len + 1); ii++)
				printk(KERN_DEBUG "value[%d]=0x%02x\n",
								ii, buf[ii]);

			printk("\n");
#endif
			return ret;
		}
	}
#endif
	ret = i2c_master_send(stat->client, buf, len);
	return (ret == len) ? 0 : -1;
}
#endif


/* stm sensor hub data readout */
static int stm_sensors_hub_get_data(struct stm_sensors_hub_status *stat,
							u8 *data)
{
	int err;
#if 0
	int jj;
#endif

	data[0] = HUB_READ_ALL_CMD;
	err = stm_sensors_hub_i2c_read(stat, data, 28);
	if (err < 0)
		return err;

#if 0
	for (jj = 0; jj < 17; jj++) {
		printk("i2c: [%d] = 0x%02x\n", jj, data[jj]);
	}
#endif
	return err;
}

void get_current_time(struct stm_sensors_hub_status *stat)
{
	struct timex  txc;
	struct rtc_time tm;
	do_gettimeofday(&(txc.time));
	txc.time.tv_sec -= sys_tz.tz_minuteswest * 60;
	rtc_time_to_tm(txc.time.tv_sec,&tm);
	stat->curr_time[0] = tm.tm_hour;
	stat->curr_time[1] = tm.tm_min;
	stat->curr_time[2] = tm.tm_sec;
	printk("UTC time :%d-%d-%d %d:%d:%d \n",tm.tm_year+1900,tm.tm_mon+1, tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);
}


#define REPORT_QUAT_NUM 22
static void stm_sensors_hub_report_values(struct stm_sensors_hub_status *stat,
					u8 *data, int len)
{
	char kk;
	char i;
	unsigned short buf;
	//compass sensor
	for(kk = 0,i=3; kk < 3; kk++,i++)
	{
		buf = *(unsigned short *) & (data[2 * kk]);
		input_report_abs(stat->input_dev, i, buf);
		//printk("kk = %d,buf=%d\n",2 * kk, buf);
	}

	//press sensor
	for(kk = 3,i=18; kk < 5; kk++,i++)
	{
		buf = *(unsigned short *) & (data[2 * kk]);
		input_report_abs(stat->input_dev, i, buf);
		//printk("i = %d,kk = %d,buf=0x%x\n",i,2 * kk, buf);
		
	}

	//proximity sensor
	buf = (unsigned short)data[10];
	input_report_abs(stat->input_dev, 30, buf);
	//printk("proximity buf=0x%x\n",buf);

	//light sensor
	for(kk = 5,i=32; kk < 7; kk++,i++)
	{
		buf = *(unsigned short *) & (data[(2*kk) + 1]);
		input_report_abs(stat->input_dev, i, buf);
		//printk("i = %d,kk = %d,buf=0x%x\n",i,2 * kk, buf);
		
	}

	//acc sensor
	for(kk = 7,i=0; kk < 10; kk++,i++)
	{
		buf = *(unsigned short *) & (data[(2*kk) + 1]);
		input_report_abs(stat->input_dev, i, buf);
		//printk("acc sensor,i = %d,kk = %d,buf=0x%x\n",i,2 * kk, buf);
		
	}

	//Gysensor
	for(kk = 10,i=6; kk < 13; kk++,i++)
	{
		buf = *(unsigned short *) & (data[(2*kk) + 1]);
		input_report_abs(stat->input_dev, i, buf);
		//printk("Gysensor,i = %d,kk = %d,buf=0x%x\n",i,2 * kk, buf);
	}
	
	input_sync(stat->input_dev);
}

#if 0
static int stm_sensors_hub_hw_init(struct stm_sensors_hub_status *stat)
{
	int err = 0;

	dev_info(&stat->client->dev, "hw init start\n");


	gpio_direction_output(stat->hub_rst_n_gpio, STM_HUB_FW_ON);
	msleep(20);
	gpio_direction_output(stat->hub_rst_n_gpio, STM_HUB_FW_OFF);
	msleep(40);
	gpio_direction_output(stat->hub_rst_n_gpio, STM_HUB_FW_ON);
	msleep(40);


	stat->hw_initialized = 1;

	return err;
}
#endif

static void stm_sensors_hub_device_power_off(struct stm_sensors_hub_status *stat)
{
	//u8 data[3] = {0x05, 0x00, 0x00};

	if (stat->pdata->power_off) {
		stat->pdata->power_off();
		stat->hw_initialized = 0;
	}

	if (stat->hw_initialized) {
		stat->hw_initialized = 0;
	}
	printk("%s\n", __func__);
#if 0
	stm_sensors_hub_i2c_write(stat, data, 1);

	dev_info(&stat->client->dev, "%s\n", __func__);
#endif
}

static int stm_sensors_hub_device_power_on(struct stm_sensors_hub_status *stat)
{
#if 0
	int err;


	if (stat->pdata->power_on) {
		err = stat->pdata->power_on();
		if (err < 0)
			return err;
	}

	if (!stat->hw_initialized) {
		err = stm_sensors_hub_hw_init(stat);
		if (err < 0) {
			stm_sensors_hub_device_power_off(stat);
			return err;
		}
	}
#endif

	return 0;
}

static int stm_notifier_suspend(struct device *dev);
static int stm_notifier_resume(struct device *dev);
static int stm_pm_notifier(struct notifier_block *nb, unsigned long event, void *data)
{
	struct stm_sensors_hub_status *stat = container_of(nb, struct stm_sensors_hub_status, nb);
	struct i2c_client *clientbuf = stat->client;		
	printk("enter %s  \n",__func__);
	switch(event)
	{
		case PM_SUSPEND_PREPARE:
			stm_notifier_suspend(&clientbuf->dev);
			break;
		case PM_POST_SUSPEND:
			stm_notifier_resume(&clientbuf->dev);
			break;
	}
	return NOTIFY_DONE;
}

static int stm_sensors_hub_enable(struct stm_sensors_hub_status *stat)
{
	int err;

	dev_info(&stat->client->dev, "%s\n", __func__);
	printk("stm_sensors_hub_enable,stat->enabled=%d\n",atomic_read(&stat->enabled));
	if (!atomic_cmpxchg(&stat->enabled, 0, 1)) {
		err = stm_sensors_hub_device_power_on(stat);
		if (err < 0) {
			atomic_set(&stat->enabled, 0);
			return err;
		}
	printk("stm_sensors_hub_enable,stat->enabled=%d,stat->polling_enabled = %d\n",atomic_read(&stat->enabled),stat->polling_enabled);
		if (stat->polling_enabled) {
			hrtimer_start(&(stat->hr_timer), stat->ktime, HRTIMER_MODE_REL);
		}
	}

	return 0;
}

static int stm_sensors_hub_disable(struct stm_sensors_hub_status *stat)
{
	printk("%s: stat->enabled = %d\n", __func__,atomic_read(&stat->enabled));

	if (atomic_cmpxchg(&stat->enabled, 1, 0)) {
		hrtimer_cancel(&stat->hr_timer);
		dev_dbg(&stat->client->dev, "%s: cancel_hrtimer ", __func__);
		stm_sensors_hub_device_power_off(stat);
	}
	printk("%s: stat->enabled = %d\n", __func__,atomic_read(&stat->enabled));
	return 0;
}


static ssize_t attr_polling_rate_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int val;
	struct stm_sensors_hub_status *stat = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);

	mutex_lock(&stat->lock);
	val = stat->pdata->poll_interval;
	mutex_unlock(&stat->lock);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_polling_rate_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	struct stm_sensors_hub_status *stat = dev_get_drvdata(dev);
	unsigned long interval_ms;

	if (kstrtoul(buf, 10, &interval_ms))
		return -EINVAL;
	if (!interval_ms)
		return -EINVAL;

	mutex_lock(&stat->lock);
	stat->pdata->poll_interval = interval_ms;

	stat->ktime = ktime_set(0,
			MS_TO_NS(stat->pdata->poll_interval));

	mutex_unlock(&stat->lock);
	return size;
}

static ssize_t attr_enable_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct stm_sensors_hub_status *stat = dev_get_drvdata(dev);
	int val = atomic_read(&stat->enabled);
	printk("%s\n", __func__);

	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_enable_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{

	struct stm_sensors_hub_status *stat = dev_get_drvdata(dev);
	unsigned long val;
	dev_info(dev, "%s\n", __func__);

	if (kstrtoul(buf, 10, &val))
		return -EINVAL;

	printk("attr_enable_stor: val = %ld\n",val);
	if (val){
		stm_sensors_hub_enable(stat);
	}
	else{
		stm_sensors_hub_disable(stat);
	}
	return size;
}

static ssize_t attr_polling_mode_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int val = 0;
	struct stm_sensors_hub_status *stat = dev_get_drvdata(dev);

	mutex_lock(&stat->lock);
	if (stat->polling_enabled)
		val = 1;
	mutex_unlock(&stat->lock);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_polling_mode_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	struct stm_sensors_hub_status *stat = dev_get_drvdata(dev);
	unsigned long val;

	if (kstrtoul(buf, 10, &val))
		return -EINVAL;

	mutex_lock(&stat->lock);
	if (val) {
		stat->polling_enabled = true;
		printk("polling mode enabled,stat->enabled = %d\n",atomic_read(&stat->enabled));
		if (atomic_read(&stat->enabled)) {
			hrtimer_start(&(stat->hr_timer), stat->ktime, HRTIMER_MODE_REL);
		}
	} else {
		if (stat->polling_enabled) {
			hrtimer_cancel(&stat->hr_timer);
		}
		stat->polling_enabled = false;
		printk("polling mode disabled\n");
	}
	mutex_unlock(&stat->lock);
	return size;
}
static ssize_t boot0_state_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int ret;
	ret = gpio_get_value(gstat->hub_boot0_gpio);

	return sprintf(buf, "%d\n", ret);

}
static ssize_t boot0_state_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	unsigned long val;

	dev_info(dev, "%s\n", __func__);

	if (kstrtoul(buf, 10, &val))
		return -EINVAL;
	gpio_set_value(gstat->hub_boot0_gpio, val);

	return size;
}
static ssize_t boot1_state_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int ret;
	ret = gpio_get_value(gstat->hub_boot1_gpio);

	return sprintf(buf, "%d\n", ret);

}
static ssize_t boot1_state_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	unsigned long val;
	dev_info(dev, "%s\n", __func__);

	if (kstrtoul(buf, 10, &val))
		return -EINVAL;
	gpio_set_value(gstat->hub_boot1_gpio, val);

	return size;
}
static ssize_t reset_state_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int ret;
	ret = gpio_get_value(gstat->hub_rst_n_gpio);
	return sprintf(buf, "%d\n", ret);

}
static ssize_t reset_state_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	unsigned long val;

	dev_info(dev, "%s\n", __func__);
	if (kstrtoul(buf, 10, &val))
		return -EINVAL;
	gpio_set_value(gstat->hub_rst_n_gpio, val);

	return size;
}
static ssize_t stm_i2c_addr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	u8 data = 0x55;
	stm_sensors_hub_i2c_write(gstat, &data, 1);
	return sprintf(buf, "%d\n", gstat->client->addr);
}

static ssize_t stm_i2c_addr_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	int ret;
	unsigned int val;

	ret = kstrtouint(buf, 10, &val);
	if (ret) {
		printk("set i2c addr failed\n");
		return ret;
	}
	gstat->client->addr = (unsigned short)val;

	return size;
}
static ssize_t stm_i2c_test_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	int ret;
	unsigned int val;
	//u8 data[INPUT_BUFFER_LEN] = {0};
	//int ii;
	//u8 data[3] = {0x05, 0x00, 0x00};	
	ret = kstrtouint(buf, 10, &val);
	if (ret) {
		printk("set i2c addr failed\n");
		return ret;
	}
	if (val == 1) {
		stm_sensors_hub_report_triple(gstat);
		printk("stm i2c test start \n");
		if (ret < 0)
			printk("stm read i2c failed\n");
		else
			printk("stm i2c test end\n");

	}
	return size;
}
//add begin by zzz
static ssize_t yesterday_step_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", yesterdayStep);
}

static ssize_t yesterday_step_store(struct device *dev,
                              struct device_attribute *attr,
				const char *buf, size_t size)
{
	int step;
	sscanf(buf, "%d", &step);
	yesterdayStep = step;
	return size;
}

static ssize_t today_step_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
        return sprintf(buf, "%d\n", todayStep);
}

static ssize_t today_step_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	int step;
	sscanf(buf, "%d", &step);
	todayStep = step;
	return size;
}
//add end by zzz
static DEVICE_ATTR(pollrate_ms, S_IRUGO|S_IWUSR |S_IWGRP, attr_polling_rate_show, attr_polling_rate_store);
static DEVICE_ATTR(enable_device, S_IRUGO|S_IWUSR |S_IWGRP, attr_enable_show, attr_enable_store);
static DEVICE_ATTR(enable_polling, S_IRUGO|S_IWUSR |S_IWGRP, attr_polling_mode_show, attr_polling_mode_store);
static DEVICE_ATTR(boot0_state, S_IRUGO|S_IWUSR |S_IWGRP, boot0_state_show, boot0_state_store);
static DEVICE_ATTR(boot1_state, S_IRUGO|S_IWUSR |S_IWGRP, boot1_state_show, boot1_state_store);
static DEVICE_ATTR(reset_state, S_IRUGO|S_IWUSR |S_IWGRP, reset_state_show, reset_state_store);
static DEVICE_ATTR(stm_i2c_addr, S_IRUGO|S_IWUSR |S_IWGRP, stm_i2c_addr_show, stm_i2c_addr_store);
static DEVICE_ATTR(stm_i2c_test, (S_IRUGO | S_IWUSR | S_IWGRP), NULL, stm_i2c_test_store);
static DEVICE_ATTR(yesterday_step, S_IRUGO|S_IWUSR |S_IWGRP, yesterday_step_show, yesterday_step_store);
static DEVICE_ATTR(today_step, S_IRUGO|S_IWUSR |S_IWGRP, today_step_show, today_step_store);

static struct attribute *acc_sysfs_attrs[] = {
	&dev_attr_pollrate_ms.attr,
	&dev_attr_enable_device.attr,
	&dev_attr_enable_polling.attr,
	&dev_attr_boot0_state.attr,
	&dev_attr_boot1_state.attr,
	&dev_attr_reset_state.attr,
	&dev_attr_stm_i2c_addr.attr,
	&dev_attr_stm_i2c_test.attr,
	&dev_attr_yesterday_step.attr,
	&dev_attr_today_step.attr,
	NULL
};

static struct attribute_group acc_attribute_group = {
	.attrs = acc_sysfs_attrs,
};


#if 0
static int create_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(dev, attributes + i))
			goto error;
	return 0;

error:
	for (; i >= 0; i--)
		device_remove_file(dev, attributes + i);
	dev_err(dev, "%s:Unable to create interface\n", __func__);
	return -1;
}

static int remove_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, attributes + i);
	return 0;
}
#endif
#define SENSOR_HUB_DATA_ITEM_LEN 16
static void stm_sensors_hub_report_triple(struct stm_sensors_hub_status *stat)
{
	int len;
	u8 data_out[INPUT_BUFFER_LEN];

	u8 *pdata = data_out;
	u8 data1 = 0x66;

	len = stm_sensors_hub_get_data(stat, data_out);

	if (len > 0 )
	{
		int recv_step_data_index = ((int)data_out[6]) << 8 | data_out[7];

		if((len == 28) && (data_out[0] == 0x5a ) && (data_out[3] == 0x5a))
		{
			if(recv_step_data_index != step_index){
				step_index = recv_step_data_index;
				printk("[before] yesterdaystep = %d, todaystep = %d, index = %d\n",yesterdayStep, todayStep, step_index);
				yesterdayStep += ((int)data_out[1]) << 8 | data_out[2];
				todayStep += ((int)data_out[4]) << 8 | data_out[5];
				printk("[after] yesterdaystep = %d, todaystep = %d\n",yesterdayStep, todayStep);
			}
			read_flag = 1;
			resume_flag = 0;
		}
		else
		{
			read_flag = 1;
			stm_sensors_hub_report_values(stat, pdata, SENSOR_HUB_DATA_ITEM_LEN);
		}
	}

	if(resume_flag)
		enable_num ++;
	else
		enable_num = 0;

	if(enable_num > 100){
		stm_sensors_hub_i2c_write(stat, &data1, 1);
		enable_num = 0;
		printk("attention:second enable firmware!\n");
	}

}

int stm_sensors_hub_input_open(struct input_dev *input)
{
	//struct stm_sensors_hub_status *stat = input_get_drvdata(input);
	printk( "%s\n", __func__);
	return 0;
}

void stm_sensors_hub_input_close(struct input_dev *dev)
{
//	struct stm_sensors_hub_status *stat = input_get_drvdata(dev);
	printk("%s\n", __func__);
	return;
}

static int stm_sensors_hub_validate_pdata(struct stm_sensors_hub_status *stat)
{
	/* checks for correctness of minimal polling period */
	stat->pdata->min_interval =
		max((unsigned int) STM_SENSORS_HUB_MIN_POLL_PERIOD_MS,
						stat->pdata->min_interval);

	stat->pdata->poll_interval = max(stat->pdata->poll_interval,
			stat->pdata->min_interval);

	return 0;
}

static int stm_sensors_hub_input_init(struct stm_sensors_hub_status *stat)
{
	char kk;
	int err = -1;

	dev_dbg(&stat->client->dev, "%s\n", __func__);

	stat->input_dev = input_allocate_device();
	if (!stat->input_dev) {
		err = -ENOMEM;
		dev_err(&stat->client->dev,
			"input device allocation failed\n");
		goto err0;
	}

	stat->input_dev->open = stm_sensors_hub_input_open;
	stat->input_dev->close = stm_sensors_hub_input_close;
	stat->input_dev->name = STM_SENSORS_HUB_DEV_NAME;
	stat->input_dev->id.bustype = BUS_I2C;
	stat->input_dev->dev.parent = &stat->client->dev;

	input_set_drvdata(stat->input_dev, stat);

	set_bit(EV_ABS, stat->input_dev->evbit);

#ifdef DEBUG
	set_bit(EV_KEY, stat->input_dev->keybit);
	set_bit(KEY_LEFT, stat->input_dev->keybit);
	input_set_abs_params(stat->input_dev, ABS_MISC, 0, 1, 0, 0);
#endif

	/* Driver Input parameters settings */
	for(kk = INPUT_SENSOR_HUB1; kk < INPUT_BUFFER_LEN/2; kk++)
		input_set_abs_params(stat->input_dev, kk, HUB_MIN, HUB_MAX, FUZZ, FLAT);

	stat->input_dev->name = STM_SENSORS_HUB_DEV_NAME;

	err = input_register_device(stat->input_dev);
	if (err) {
		dev_err(&stat->client->dev,
			"unable to register input device %s\n",
			stat->input_dev->name);
		goto err1;
	}

	return 0;

err1:
	input_free_device(stat->input_dev);
err0:
	return err;
}

static void stm_sensors_hub_input_cleanup(struct stm_sensors_hub_status *stat)
{
	input_unregister_device(stat->input_dev);
	input_free_device(stat->input_dev);
}

static void poll_function_work(struct work_struct *polling_task)
{
	struct stm_sensors_hub_status *stat;
	stat = container_of((struct work_struct *)polling_task,
					struct stm_sensors_hub_status, polling_task);

	/* HUB Data Report */
	stm_sensors_hub_report_triple(stat);
//	printk("poll_function_work,stat->enabled = %d\n",atomic_read(&stat->enabled));
	if(read_flag == 1)
	{
		if (atomic_read(&stat->enabled)) 
		{
			read_flag = 0;
			hrtimer_start(&stat->hr_timer, stat->ktime, HRTIMER_MODE_REL);
		}
	}
}

enum hrtimer_restart poll_function_read(struct hrtimer *timer)
{
	struct stm_sensors_hub_status *stat;

	stat = container_of((struct hrtimer *)timer,
				struct stm_sensors_hub_status, hr_timer);
//	printk("poll_function_read\n");
	queue_work(stm_sensors_hub_workqueue, &stat->polling_task);
	return HRTIMER_NORESTART;
}

#ifdef CONFIG_OF
static struct of_device_id stm_sensors_hub_dt_ids[] = {
	{ .compatible = "stm_sensor_hub" },
	{}
};
MODULE_DEVICE_TABLE(of, stm_sensors_hub_dt_ids);
#else
static struct stm_sensors_hub_platform_data default_stm_sensors_hub_pdata = {

	.poll_interval = 10, /* 10 ms <-> 100Hz */
	.min_interval = STM_SENSORS_HUB_MIN_POLL_PERIOD_MS, /* 2ms */
};
#endif

#ifdef CONFIG_OF
static int stm_sensors_hub_dt_parse(struct i2c_client *client,
					struct stm_sensors_hub_status *stat)
{
	int err = -1;
	u32 val;
	struct device_node *dn;

	if (of_match_device(stm_sensors_hub_dt_ids, &client->dev)) {
		dn = client->dev.of_node;
		stat->pdata->of_node = dn;
		printk("stm_sensors_hub_dt_parse start!\n");
		if (!of_property_read_u32(dn, "poll_interval", &val))
			stat->pdata->poll_interval = val;
		printk("stm stat->pdata->poll_interval=%d\n",stat->pdata->poll_interval);

		if (!of_property_read_u32(dn, "min_interval", &val))
			stat->pdata->min_interval = val;
		printk("stm stat->pdata->min_interval=%d\n",stat->pdata->min_interval);

		/*******f***********************/
		/* Sensor  HUB Reset */
		/******************************/
		stat->hub_rst_n_gpio = of_get_named_gpio(dn, "hub_rst_n_gpio", 0);
		printk("stm stat->hub_rst_n_gpio=%d\n",stat->hub_rst_n_gpio);

		err = gpio_request_one(stat->hub_rst_n_gpio,
			GPIOF_OPEN_DRAIN|GPIOF_OUT_INIT_HIGH,"hub_rst_n_gpio");
		if (err) {
			printk("gpio [%d] request failed\n", err);
			return err;
		}
		if (!gpio_is_valid(stat->hub_rst_n_gpio)) {
			printk("failed to get reset gpio\n");
			return err;
		}

		/******************************/
		/* Sensor  HUB Boot0 */
		/******************************/
		stat->hub_boot0_gpio = of_get_named_gpio(dn, "hub_boot0_gpio", 0);
		printk("stm stat->hub_boot0_gpio=%d\n",stat->hub_boot0_gpio);

		err = gpio_request(stat->hub_boot0_gpio, "hub_boot0_gpio");
		if (err) {
			printk("stat->hub_boot0_gpio gpio [%d] request failed\n", err);
			return err;
		}

		if (!gpio_is_valid(stat->hub_boot0_gpio)) {
			printk("failed to get reset gpio\n");
			return err;
		}


		/******************************/
		/* Sensor  HUB Boot1 */
		/******************************/
		stat->hub_boot1_gpio = of_get_named_gpio(dn, "hub_boot1_gpio", 0);
		printk("stm stat->hub_boot1_gpio=%d\n",stat->hub_boot1_gpio);

		err = gpio_request(stat->hub_boot1_gpio, "hub_boot1_gpio");
		if (err) {
			printk("gpio [%d] request failed\n", err);
			return err;
		}
		
		if (!gpio_is_valid(stat->hub_boot1_gpio)) {
			printk("failed to get reset gpio\n");
			return err;
		}

	
		/******************************/
		/* Sensor HUB AP IRQ */
		/******************************/	
		stat->ap_int_hub_gpio = of_get_named_gpio(dn, "ap_int_hub_gpio", 0);
		printk("stm stat->ap_int_hub_gpio=%d\n",stat->ap_int_hub_gpio);

		err = gpio_request(stat->ap_int_hub_gpio, "ap_int_hub_gpio");
		if (err) {
			printk("gpio [%d] request failed\n", err);
			return err;
		}

		if (!gpio_is_valid(stat->ap_int_hub_gpio)) {
			printk("failed to get ap irq gpio\n");
			return err;
		}


		/******************************/
		/* Sensor HUB IRQ */
		/******************************/
		stat->hub_int_ap_gpio = of_get_named_gpio(dn, "hub_int_ap_gpio", 0);
		printk("stm stat->hub_int_ap_gpio=%d\n",stat->hub_int_ap_gpio);

		err = gpio_request(stat->hub_int_ap_gpio, "hub_int_ap_gpio");
		if (err) {
			printk("gpio [%d] request failed\n", err);
			return err;
		}

		if (!gpio_is_valid(stat->hub_int_ap_gpio)) {
			printk("failed to get hub irq gpio\n");
			return err;
		}

		printk("%s poll_interval %d, min_interval %d, gpios %d %d %d %d %d\n",
			STM_SENSORS_HUB_DEV_NAME,
			stat->pdata->poll_interval,
			stat->pdata->min_interval,
			stat->hub_rst_n_gpio,
			stat->hub_boot0_gpio,
			stat->hub_boot1_gpio,
			stat->ap_int_hub_gpio,
			stat->hub_int_ap_gpio);

	}

	return 0;
}
#endif

static int stm_sensors_hub_probe(struct i2c_client *client,
					const struct i2c_device_id *devid)
{
	int i,err = -1;
	u8 data = 0x55;
	struct stm_sensors_hub_status *stat;

	u32 smbus_func = I2C_FUNC_SMBUS_BYTE_DATA |
			I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_I2C_BLOCK ;

	dev_info(&client->dev, "%s probe start.\n", STM_SENSORS_HUB_DEV_NAME);
	printk("stm_sensors_hub_probe\n");
	stat = kzalloc(sizeof(*stat), GFP_KERNEL);
	if (stat == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto err0;
	}

	/* Support for both I2C and SMBUS adapter interfaces. */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_warn(&client->dev, "client not i2c capable\n");
		if (i2c_check_functionality(client->adapter, smbus_func)) {
			stat->use_smbus = 1;
			dev_warn(&client->dev, "client using SMBUS\n");
		} else {
			err = -ENODEV;
			dev_err(&client->dev, "client nor SMBUS capable\n");
			stat->use_smbus = 0;
			goto err0;
		}
	}

	if(stm_sensors_hub_workqueue == 0)
		stm_sensors_hub_workqueue = create_workqueue("stm_sensors_hub_workqueue");
	
	
  	INIT_WORK(&stat->polling_task, poll_function_work);

	hrtimer_init(&stat->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	stat->hr_timer.function = &poll_function_read;

	mutex_init(&stat->lock);
	mutex_lock(&stat->lock);
	stat->client = client;

	stat->pdata = kzalloc(sizeof(*stat->pdata), GFP_KERNEL);
	if (stat->pdata == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for pdata: %d\n", err);
		goto err1;
	}

#ifdef CONFIG_OF
	err = stm_sensors_hub_dt_parse(client, stat);
	if (err < 0) {
		dev_err(&client->dev, "failed to parse device tree info\n");
		goto err1;
	}

	/******************************/
	/* Sensor HW HUB Enable */
	/******************************/
	gpio_direction_output(stat->hub_boot0_gpio, STM_HUB_FW_OFF);
	gpio_direction_output(stat->hub_boot1_gpio, STM_HUB_FW_OFF);
	gpio_direction_output(stat->ap_int_hub_gpio, STM_HUB_FW_OFF);

	gpio_direction_output(stat->hub_rst_n_gpio, STM_HUB_FW_ON);
	msleep(50);
	gpio_direction_output(stat->hub_rst_n_gpio, STM_HUB_FW_OFF);
	msleep(50);
	gpio_direction_output(stat->hub_rst_n_gpio, STM_HUB_FW_ON);

#else
	if (client->dev.platform_data == NULL) {
		memcpy(stat->pdata, &default_stm_sensors_hub_pdata,
							sizeof(*stat->pdata));
		dev_info(&client->dev, "using default plaform_data\n");
	} else {
		memcpy(stat->pdata, client->dev.platform_data,
						sizeof(*stat->pdata));
	}
#endif

	err = stm_sensors_hub_validate_pdata(stat);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto err1_1;
	}

	i2c_set_clientdata(client, stat);

	if (stat->pdata->init) {
		err = stat->pdata->init();
		if (err < 0) {
			dev_err(&client->dev, "init failed: %d\n", err);
			goto err1_1;
		}
	}

	stat->polling_enabled = true;
	dev_info(&client->dev, "polling mode enabled\n");

	err = stm_sensors_hub_device_power_on(stat);
	if (err < 0) {
		dev_err(&client->dev, "power on failed: %d\n", err);
		goto err2;
	}

	atomic_set(&stat->enabled, 1);

	stat->ktime = ktime_set(0,
			MS_TO_NS(stat->pdata->poll_interval));

	err = stm_sensors_hub_input_init(stat);
	if (err < 0)
		goto err3;

	err = sysfs_create_group(&stat->input_dev->dev.kobj, &acc_attribute_group);
	if (err < 0) {
		dev_err(&client->dev,
		   "device sysfs register failed\n");
		goto err4;
	}

	//stm_sensors_hub_device_power_off(stat);

	/* As default, do not report information */
	atomic_set(&stat->enabled, 0);
	device_init_wakeup(&client->dev, 1);
	stat->nb.notifier_call = stm_pm_notifier;
	register_pm_notifier(&stat->nb);

	mutex_unlock(&stat->lock);

	printk("%s probed: device created successfully\n",STM_SENSORS_HUB_DEV_NAME);

	gstat = stat;
	for(i = 1; i < 6; i++) {
	   err = stm_sensors_hub_i2c_write(gstat, &data, 1);
	   if(err == 0) break;
	   printk("send reset try times %d\n",i);
	   msleep(2);
	}

	msleep(2000);

	get_current_time(stat);
	for(i = 1; i < 6; i++) {
	   err = stm_sensors_hub_i2c_write(gstat, stat->curr_time, sizeof(stat->curr_time));
	   if(err == 0) break;
	   printk("send time try times %d\n",i);
	   msleep(100);
	}

	//hrtimer_start(&stat->hr_timer, stat->ktime, HRTIMER_MODE_REL);
	printk("stm32 sensorhub probe end!\n");
	return 0;

err4:
	stm_sensors_hub_input_cleanup(stat);
err3:
	stm_sensors_hub_device_power_off(stat);
err2:
	if (stat->pdata->exit)
		stat->pdata->exit();
err1_1:
	mutex_unlock(&stat->lock);
	kfree(stat->pdata);
err1:
	destroy_workqueue(stm_sensors_hub_workqueue);
	kfree(stat);
err0:
		pr_err("%s: Driver Initialization failed\n",
							STM_SENSORS_HUB_DEV_NAME);
		return err;
}

static int stm_sensors_hub_remove(struct i2c_client *client)
{
	struct stm_sensors_hub_status *stat = i2c_get_clientdata(client);

	dev_info(&stat->client->dev, "driver removing\n");

	cancel_work_sync(&stat->polling_task);
	if(!stm_sensors_hub_workqueue) {
		flush_workqueue(stm_sensors_hub_workqueue);
		destroy_workqueue(stm_sensors_hub_workqueue);
	}

	stm_sensors_hub_disable(stat);
	stm_sensors_hub_input_cleanup(stat);

	//remove_sysfs_interfaces(&client->dev);

	kfree(stat->pdata);
	kfree(stat);
	return 0;
}

#ifdef CONFIG_PM
static int stm_sensors_hub_suspend(struct device *dev)
{
	printk("enter %s  \n",__func__);
	return 0;
}
static int stm_sensors_hub_resume(struct device *dev)
{
	printk("enter %s  \n",__func__);
	return 0;
}
#else
#define stm_sensors_hub_suspend	NULL
#define stm_sensors_hub_resume	NULL
#endif /*CONFIG_PM*/


static int stm_notifier_suspend(struct device *dev)
{
	int i,err = 0;
	u8 data2 = 0x77;
	struct i2c_client *client = to_i2c_client(dev);
	struct stm_sensors_hub_status *stat = i2c_get_clientdata(client);

	printk("%s,stat->enable = %d\n",__func__,atomic_read(&stat->enabled));	
	
	if (atomic_read(&stat->enabled)) {
		mutex_lock(&stat->lock);
		if (stat->polling_enabled) {
			printk("stm_sensors_hub_suspend,polling mode enabled\n");
			hrtimer_cancel(&stat->hr_timer);
		}

		mutex_unlock(&stat->lock);
	}

	for(i = 1 ;i < 6; i++) {
	   err = stm_sensors_hub_i2c_write(stat, &data2, 1);
	   if(err == 0) break;
	   printk("disable try times %d\n",i);
	   mdelay(5);
	}	
	resume_flag = 0;	
	return err;
}

static int stm_notifier_resume(struct device *dev)
{
	int i,err = 0;
	u8 data1 = 0x66;
	
	struct i2c_client *client = to_i2c_client(dev);
	struct stm_sensors_hub_status *stat = i2c_get_clientdata(client);	

	printk("%s,stat->enable = %d,stat->polling_enable = %d\n",__func__,atomic_read(&stat->enabled),stat->polling_enabled);

	if (atomic_read(&stat->enabled)) {
		mutex_lock(&stat->lock);
		if (stat->polling_enabled) {
			printk("stm_sensors_hub_resume,polling mode enabled\n");
			//hrtimer_init(&stat->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
			hrtimer_start(&(stat->hr_timer), stat->ktime, HRTIMER_MODE_REL);
		}

		mutex_unlock(&stat->lock);

	}
	
	for(i = 1 ;i < 6; i++) {
	   err = stm_sensors_hub_i2c_write(stat, &data1, 1);
	   if(err == 0) break;
	   printk("disable try times %d\n",i);
	   mdelay(5);
	}	
	resume_flag = 1;	

	return err;
}

static SIMPLE_DEV_PM_OPS(stm_sensors_hub_pm, stm_sensors_hub_suspend,
			 stm_sensors_hub_resume);

static const struct i2c_device_id stm_sensors_hub_id[] = {
	{ STM_SENSORS_HUB_DEV_NAME , 0 },
	{},
};

MODULE_DEVICE_TABLE(i2c, stm_sensors_hub_id);

static struct i2c_driver stm_sensors_hub_driver = {
	.driver = {
			.owner = THIS_MODULE,
			.name = STM_SENSORS_HUB_DEV_NAME,			
			.pm = &stm_sensors_hub_pm,
			
#ifdef CONFIG_OF
			.of_match_table = of_match_ptr(stm_sensors_hub_dt_ids),
#endif
	},

	.probe = stm_sensors_hub_probe,
	.remove = stm_sensors_hub_remove,
	.id_table = stm_sensors_hub_id,
};

static int __init stm_sensors_hub_init(void)
{
	int rc;
	printk("stm_sensors_hub_init start\n");
	rc = i2c_add_driver(&stm_sensors_hub_driver);
	if (rc) {
		pr_err("%s FAILED: i2c_add_driver rc=%d\n", __func__, rc);
		goto init_exit;
	}
	printk("stm_sensors_hub_init end\n");
	return 0;

init_exit:
	return rc;
}

static void __exit stm_sensors_hub_exit(void)
{
	i2c_del_driver(&stm_sensors_hub_driver);
}

module_init(stm_sensors_hub_init);
module_exit(stm_sensors_hub_exit);


//module_i2c_driver(stm_sensors_hub_driver);

MODULE_DESCRIPTION("stm_sensors_hub driver");
MODULE_AUTHOR("STMicroelectronics");
MODULE_LICENSE("GPL");
