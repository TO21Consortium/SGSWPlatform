#ifndef _ICM20608_H_
#define _ICM20608_H_

#include <stdint.h>
#include "stm32l1xx.h"
#include <string.h>
#include <stdint.h>
#include <assert.h>

#define HW_ICM20608 0x19

/* icm20608 I2C address */
#define ICM20608_I2C_ADDR 0x68

#define INV_X_GYRO      (0x40)
#define INV_Y_GYRO      (0x20)
#define INV_Z_GYRO      (0x10)
#define INV_XYZ_GYRO    (INV_X_GYRO | INV_Y_GYRO | INV_Z_GYRO)
#define INV_XYZ_ACCEL   (0x08)
#define INV_XYZ_COMPASS (0x01)

/* Gyro Filter configurations. */
enum lpf_e {
    INV_GYRO_FILTER_250Hz = 0,
    INV_GYRO_FILTER_176HZ,
    INV_GYRO_FILTER_92HZ,
    INV_GYRO_FILTER_41HZ,
    INV_GYRO_FILTER_20HZ,
    INV_GYRO_FILTER_10HZ,
    INV_GYRO_FILTER_5HZ,
    INV_GYRO_FILTER_3200HZ,
    NUM_GYRO_FILTER
};

/* Gyro Averaging Filters. */
enum gyro_avgf_e {
    INV_GYRO_1X_AVG = 0,
    INV_GYRO_2X_AVG,
    INV_GYRO_4X_AVG,
    INV_GYRO_8X_AVG,
    INV_GYRO_16X_AVG,
    INV_GYRO_32X_AVG,
    INV_GYRO_64X_AVG,
    INV_GYRO_128X_AVG,
    NUM_GYRO_AVG
};

/* Accel Filter configurations. */
enum lpf_a_e {
    INV_ACCEL_FILTER_218Hz = 0,
    INV_ACCEL_FILTER_218HZ,
    INV_ACCEL_FILTER_99HZ,
    INV_ACCEL_FILTER_45HZ,
    INV_ACCEL_FILTER_21HZ,
    INV_ACCEL_FILTER_10HZ,
    INV_ACCEL_FILTER_5HZ,
    INV_ACCEL_FILTER_420HZ,
    NUM_ACCEL_FILTER
};

/* Full scale ranges. */
enum gyro_fsr_e {
    INV_FSR_250DPS = 0,
    INV_FSR_500DPS,
    INV_FSR_1000DPS,
    INV_FSR_2000DPS,
    NUM_GYRO_FSR
};

/* Full scale ranges. */
enum accel_fsr_e {
    INV_FSR_2G = 0,
    INV_FSR_4G,
    INV_FSR_8G,
    INV_FSR_16G,
    NUM_ACCEL_FSR
};

/* Accel Averaging Filters. */
enum accel_avgf_e {
    INV_ACCEL_4X_AVG = 0,
    INV_ACCEL_8X_AVG,
    INV_ACCEL_16X_AVG,
    INV_ACCEL_32X_AVG,
    NUM_ACCEL_AVG
};

/* Clock sources. */
enum clock_sel_e {
    INV_CLK_INTERNAL = 0,
    INV_CLK_PLL,
    NUM_CLK
};

/* Low-power accel wakeup rates. */
enum lp_accel_rate_e {
    INV_LPA_0_24HZ,
    INV_LPA_0_49HZ,
    INV_LPA_0_98HZ,
    INV_LPA_1_95HZ,
    INV_LPA_3_91HZ,
    INV_LPA_7_81HZ,
    INV_LPA_15_63HZ,
    INV_LPA_31_25HZ,
    INV_LPA_62_5HZ,
    INV_LPA_125HZ,
    INV_LPA_250HZ,
    INV_LPA_500HZ
};

#define BIT_I2C_MST_VDDIO   (0x80)
#define BIT_FIFO_EN         (0x40)
#define BIT_DMP_EN          (0x80)
#define BIT_FIFO_RST        (0x04)
#define BIT_DMP_RST         (0x08)
#define BIT_FIFO_OVERFLOW   (0x10)
#define BIT_DATA_RDY_EN     (0x01)
#define BIT_DMP_INT_EN      (0x02)
#define BIT_MOT_INT_EN      (0x40)
#define BITS_FSR            (0x18)
#define BITS_LPF            (0x07)
#define BITS_HPF            (0x07)
#define BITS_CLK            (0x07)
#define BIT_FIFO_SIZE_1024  (0x40)
#define BIT_FIFO_SIZE_2048  (0x80)
#define BIT_FIFO_SIZE_4096  (0xC0)
#define BIT_RESET           (0x80)
#define BIT_SLEEP           (0x40)
#define BIT_S0_DELAY_EN     (0x01)
#define BIT_S2_DELAY_EN     (0x04)
#define BITS_SLAVE_LENGTH   (0x0F)
#define BIT_SLAVE_BYTE_SW   (0x40)
#define BIT_SLAVE_GROUP     (0x10)
#define BIT_SLAVE_EN        (0x80)
#define BIT_I2C_READ        (0x80)
#define BITS_I2C_MASTER_DLY (0x1F)
#define BIT_AUX_IF_EN       (0x20)
#define BIT_ACTL            (0x80)
#define BIT_LATCH_EN        (0x20)
#define BIT_ANY_RD_CLR      (0x10)
#define BIT_BYPASS_EN       (0x02)
#define BITS_WOM_EN         (0xC0)
#define BIT_LPA_CYCLE       (0x20)
#define BIT_STBY_XA         (0x20)
#define BIT_STBY_YA         (0x10)
#define BIT_STBY_ZA         (0x08)
#define BIT_STBY_XG         (0x04)
#define BIT_STBY_YG         (0x02)
#define BIT_STBY_ZG         (0x01)
#define BIT_STBY_XYZA       (BIT_STBY_XA | BIT_STBY_YA | BIT_STBY_ZA)
#define BIT_STBY_XYZG       (BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG)
#define BIT_ACCL_FC_B		(0x08)
#define BIT_ACCL_DEC2		(0x30)
#define BIT_GYRO_FC_B		(0x01)
#define BIT_G_AVGCFG            (0x70)
#define WHOAMI_20608            (0xAF)
#define BIT_GYRO_CYCLE          (0x80)

#define DEFAULT_ICM_HZ  (20)

/* Set up APIs */
int icm_init(void);
int icm_init_slave(void);
int icm_set_bypass(unsigned char bypass_on);
int icm_check_whoami(void);

/* Configuration APIs */
int icm_lp_accel_mode(unsigned short rate);
int icm_lp_motion_interrupt(unsigned short thresh, unsigned char time,
    unsigned short lpa_freq);
int icm_lp_6axis_mode(int gyro_avg,unsigned char enable);
int icm_set_int_level(unsigned char active_low);
int icm_set_int_latched(unsigned char enable);

int icm_set_dmp_state(unsigned char enable);
int icm_get_dmp_state(unsigned char *enabled);

int icm_get_gyro_lpf(unsigned short *lpf);
int icm_set_gyro_lpf(unsigned short lpf);

int icm_get_accel_lpf(unsigned short *lpf);
int icm_set_accel_lpf(unsigned short lpf);

int icm_get_gyro_avgf(unsigned short *avgf);
int icm_set_gyro_avgf(unsigned short avgf);

int icm_get_accel_avgf(unsigned short *avgf);
int icm_set_accel_avgf(unsigned short avgf);

int icm_get_gyro_fsr(unsigned short *fsr);
int icm_set_gyro_fsr(unsigned short fsr);

int icm_get_accel_fsr(unsigned char *fsr);
int icm_set_accel_fsr(unsigned char fsr);

int icm_get_watermark(unsigned char *threshold);
int icm_set_watermark(unsigned char threshold);

int icm_get_compass_fsr(unsigned short *fsr);

int icm_get_gyro_sens(float *sens);
int icm_get_accel_sens(unsigned short *sens);

int icm_get_sample_rate(unsigned short *rate);
int icm_set_sample_rate(unsigned short rate);
int icm_get_compass_sample_rate(unsigned short *rate);
int icm_set_compass_sample_rate(unsigned short rate);

int icm_get_fifo_config(unsigned char *sensors);
int icm_configure_fifo(unsigned char sensors);

int icm_get_power_state(unsigned char *power_on);
int icm_set_sensors(unsigned char sensors);

int icm_read_accel_bias(long *accel_bias);
int icm_set_gyro_bias_reg(long * gyro_bias);
int icm_set_accel_bias_reg(const long *accel_bias);

/* Data getter/setter APIs */
int icm_get_gyro_reg(short *data, unsigned long *timestamp);
int icm_get_accel_reg(short *data, unsigned long *timestamp);
int icm_get_compass_reg(short *data, unsigned long *timestamp);
int icm_get_temperature(long *data, unsigned long *timestamp);

int icm_get_int_status(short *status);
int icm_read_fifo(short *gyro, short *accel, unsigned long *timestamp,
    unsigned char *sensors, unsigned char *more);
int icm_read_fifo_stream(unsigned short length, unsigned char *data,
    unsigned char *more);
int icm_reset_fifo(void);

int icm_write_mem(unsigned short mem_addr, unsigned short length,
    unsigned char *data);
int icm_read_mem(unsigned short mem_addr, unsigned short length,
    unsigned char *data);
int icm_load_firmware(unsigned short length, const unsigned char *firmware,
    unsigned short start_addr, unsigned short sample_rate);

int icm_reg_dump(void);
int icm_read_reg(unsigned char reg, unsigned char *data);
int icm_run_self_test(long *gyro, long *accel, unsigned char debug);
int icm_register_tap_cb(void (*func)(unsigned char, unsigned char));

#endif
