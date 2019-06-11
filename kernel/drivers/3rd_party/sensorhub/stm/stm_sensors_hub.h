/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
*
* File Name		: stm_sensors_hub_sysfs.c
* Authors		: MEMS Motion Sensors Products Div- Application Team
*			: Both authors are willing to be considered the contact
*			: and update points for the driver.
* Version		: V 1.2.2 sysfs
* Date			: 2013/08/09
* Description		: GIORGIONE digital output gyroscope sensor API
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

#ifndef __STM_SENSORS_HUB_H__
#define __STM_SENSORS_HUB_H__

#define STM_SENSORS_HUB_DEV_NAME	"stm_sensor_hub"

#ifdef __KERNEL__

#define INPUT_BUFFER_LEN		70
#define INPUT_SENSOR_HUB1		0
#define INPUT_SENSOR_HUB70		(INPUT_BUFFER_LEN - 1)


/* STM32 Sensor HUB i2c address
*/
#define STM_SENSORS_HUB_I2C_SAD		0x40


/* to set gpios numb connected to gyro interrupt pins,
 * the unused ones havew to be set to -EINVAL
 */

#define STM_SENSORS_HUB_MIN_POLL_PERIOD_MS	20


/* STM32 Sensor HUB GPIO Status
*/
#define STM_HUB_FW_ON	1
#define STM_HUB_FW_OFF	0


struct stm_sensors_hub_platform_data {
	int (*init)(void);
	void (*exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);
	unsigned int poll_interval;
	unsigned int min_interval;

#ifdef CONFIG_OF
	struct device_node	*of_node;
#endif
};
#endif /* __KERNEL__ */

#endif  /* __STM_SENSORS_HUB_H__ */
