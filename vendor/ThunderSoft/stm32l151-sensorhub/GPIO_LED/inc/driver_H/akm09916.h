#ifndef _AKM09916_H_
#define _AKM09916_H_
#include <stdint.h>
#include "stm32l1xx.h"
#include <string.h>
#include "i2c_master.h"

#define AKM09916_I2C_ADDR 0x0c
#define AKM09916_COMPANY_ID 0x48        //reg 0x01
#define AKM09916_Device_ID 0x09 //reg 0x01

/* Register definitions */
#define AK0991x_REG_WIA1					0x00		/* Company ID */
#define AK0991x_REG_WIA2					0x01		/* Device ID */

#define AK0991x_REG_INFO1					0x02		/* Information */
#define AK0991x_REG_INFO2					0x03		/* Information */

#define AK0991x_REG_ST1						0x10		/* Status 1 */
#define AK0991x_REG_HXL						0x11		/* Measurement data : X-axis data */
#define AK0991x_REG_HXH						0x12		/* Measurement data : X-axis data */
#define AK0991x_REG_HYL						0x13		/* Measurement data : Y-axis data */
#define AK0991x_REG_HYH						0x14		/* Measurement data : Y-axis data */
#define AK0991x_REG_HZL						0x15		/* Measurement data : Z-axis data */
#define AK0991x_REG_HZH						0x16		/* Measurement data : Z-axis data */
#define AK0991x_REG_TMPS					0x17		/*  */
#define AK0991x_REG_ST2						0x18		/* Status 2 : Data status */

#define AK0991x_REG_CNTL1					0x30        /* Control 1 : Function Control */
#define AK0991x_REG_CNTL2					0x31        /* Control 2 : Function Control */
#define AK0991x_REG_CNTL3					0x32        /* Control 3 : Function Control */

#define AK0991x_REG_TS1						0x33        /* Test 1 : DO NOT ACCESS,  */
#define AK0991x_REG_TS2						0x34        /* Test 2 : DO NOT ACCESS,  */


/* Register helper values */
#define AK0991x_COMPANY_ID					0x48    	/* Device ID of AKM */
#define AK0991x_DEVICE_ID					0x09     	/* Device ID of AKM */

#define AK0991x_CNTL2_MODE_POWER_DOWN   	0x00		/* Power-down mode */
#define AK0991x_CNTL2_MODE_SINGLE       	0x01		/* Single measurement mode */
#define AK0991x_CNTL2_MODE_CONTINUOUS1  	0x02 		/* Continuous measurement mode 1 */
#define AK0991x_CNTL2_MODE_CONTINUOUS2  	0x04		/* Continuous measurement mode 2 */
#define AK0991x_CNTL2_MODE_CONTINUOUS3     	0x06		/* Continuous measurement mode 3 */
#define AK0991x_CNTL2_MODE_CONTINUOUS4     	0x08		/* Continuous measurement mode 4 */
#define AK0991x_CNTL2_MODE_SELF_TEST	   	0x10		/* Self-test mode */
#define AK0991x_CNTL2_MODE_FUSE_ROM     	0x1F		/* Fuse ROM access mode */

#define AK0991x_DRDY						0x01

int ak0991x_enable_sensor(int en);
int ak09916_soft_reset();
int ak09916_compare_whoami();
int ak09916_run_selftest();
int ak09916_poll_data(int16_t *compass_data);

#endif
