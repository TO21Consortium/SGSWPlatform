#ifndef _LTR553_H_
#define _LTR553_H_

#include <stdint.h>
#include "stm32l1xx.h"
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define LTR553_I2C_ADDR 0x23

#define LTR559_ALS_CONTR	0x80
#define LTR559_PS_CONTR		0x81
#define LTR559_PS_LED		0x82
#define LTR559_PS_N_PULSES	0x83
#define LTR559_PS_MEAS_RATE	0x84
#define LTR559_ALS_MEAS_RATE	0x85
#define LTR559_MANUFACTURER_ID	0x87

#define LTR559_INTERRUPT	0x8F
#define LTR559_PS_THRES_UP_0	0x90
#define LTR559_PS_THRES_UP_1	0x91
#define LTR559_PS_THRES_LOW_0	0x92
#define LTR559_PS_THRES_LOW_1	0x93

#define LTR559_ALS_THRES_UP_0	0x97
#define LTR559_ALS_THRES_UP_1	0x98
#define LTR559_ALS_THRES_LOW_0	0x99
#define LTR559_ALS_THRES_LOW_1	0x9A

#define LTR559_INTERRUPT_PERSIST 0x9E

/* 559's Read Only Registers */
#define LTR559_ALS_DATA_CH1_0	0x88
#define LTR559_ALS_DATA_CH1_1	0x89
#define LTR559_ALS_DATA_CH0_0	0x8A
#define LTR559_ALS_DATA_CH0_1	0x8B
#define LTR559_ALS_PS_STATUS	0x8C
#define LTR559_PS_DATA_0	0x8D
#define LTR559_PS_DATA_1	0x8E


/* Basic Operating Modes */
#define MODE_ALS_Range1	0x00  ///for als gain x1
#define MODE_ALS_Range2	0x04  ///for als  gain x2
#define MODE_ALS_Range3	0x08  ///for als  gain x4
#define MODE_ALS_Range4	0x0C  ///for als gain x8
#define MODE_ALS_Range5	0x18  ///for als gain x48
#define MODE_ALS_Range6	0x1C  ///for als gain x96

#define MODE_ALS_StdBy		0x00

#define ALS_RANGE_64K	1
#define ALS_RANGE_32K 	2
#define ALS_RANGE_16K 	4
#define ALS_RANGE_8K 	8
#define ALS_RANGE_1300 48
#define ALS_RANGE_600 	96
#define SW_reset 0x02

#define MODE_PS_Gain16	0x00
#define MODE_PS_Gain32	0x08
#define MODE_PS_Gain64	0x0C

#define MODE_PS_StdBy		0x00

#define PS_RANGE16 	1
#define PS_RANGE32	4
#define PS_RANGE64 	8
#define PON_DELAY		100
#define WAKEUP_DELAY	10
static unsigned long ltr553_i2c_write(unsigned char register_addr,const unsigned char *register_value,unsigned short register_len);
static unsigned long ltr553_i2c_read(unsigned char register_addr,unsigned char *register_value,unsigned short register_len);
static void ltr553_ps_set_thres(void);
int8_t ltr553_dev_init(void);
void ltr559_ps_enable(uint8_t enable);
void ltr553_als_enable(uint8_t enable);
void ltr553_ps_read(uint16_t *data);
void ltr559_als_read(unsigned long *data);
uint8_t ltr553_get_ps_value(uint16_t ps);

#endif
