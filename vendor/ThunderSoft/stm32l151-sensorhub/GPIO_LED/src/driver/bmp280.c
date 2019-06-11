#include "bmp280.h"
#include "i2c_master.h"
#include <stdlib.h>
#include "delay.h"

#define dig_T1 g_bmp280->T1  
#define dig_T2 g_bmp280->T2  
#define dig_T3 g_bmp280->T3  
#define dig_P1 g_bmp280->P1  
#define dig_P2 g_bmp280->P2  
#define dig_P3 g_bmp280->P3  
#define dig_P4 g_bmp280->P4  
#define dig_P5 g_bmp280->P5  
#define dig_P6 g_bmp280->P6  
#define dig_P7 g_bmp280->P7  
#define dig_P8 g_bmp280->P8  
#define dig_P9 g_bmp280->P9 
struct bmp280 *g_bmp280; 

static unsigned long bmp280_write_register(unsigned char register_addr,const unsigned char *register_value,unsigned short register_len)
{  
  return i2c_master_write_register(BMP280_I2C_ADDR,register_addr,register_len, register_value);
}

static unsigned long bmp280_read_register(unsigned char register_addr,unsigned char *register_value,unsigned short register_len)
{
  return i2c_master_read_register(BMP280_I2C_ADDR,register_addr,register_len, register_value);
}

void bmp280_init()  
{  
    uint8_t reg_val[24] = {0};
    uint8_t ctrlmeas_reg, config_reg; 
    uint8_t bmp280_id; 
    g_bmp280 = malloc(sizeof(struct bmp280));
    bmp280_read_register(BMP280_CHIPID_REG,&bmp280_id,1);  
		delay_ms(10);
			g_bmp280->mode = BMP280_SLEEP_MODE;  
			//g_bmp280->mode = BMP280_NORMAL_MODE;
     g_bmp280->t_sb = BMP280_T_SB1;  
     g_bmp280->p_oversampling = BMP280_P_MODE_1;  
     g_bmp280->t_oversampling = BMP280_T_MODE_1;  
     g_bmp280->filter_coefficient = BMP280_FILTER_MODE_1;  
		
    bmp280_read_register(BMP280_DIG_T1_LSB_REG,reg_val,24);
		delay_ms(10);
    dig_T1 = reg_val[1] << 8 | reg_val[0];
    dig_T2 = reg_val[3] << 8 | reg_val[2];
    dig_T3 = reg_val[5] << 8 | reg_val[4];
    dig_P1 = reg_val[7] << 8 | reg_val[6];
    dig_P2 = reg_val[9] << 8 | reg_val[8];
    dig_P3 = reg_val[11] << 8 | reg_val[10];
    dig_P4 = reg_val[13] << 8 | reg_val[12];
    dig_P5 = reg_val[15] << 8 | reg_val[14];
    dig_P6 = reg_val[17] << 8 | reg_val[16];
    dig_P7 = reg_val[19] << 8 | reg_val[18];
    dig_P8 = reg_val[21] << 8 | reg_val[20];
    dig_P9 = reg_val[23] << 8 | reg_val[22];
    
#if 0
    uint8_t lsb, msb; 
    /* read the temperature calibration parameters */  
    bmp280_read_register(BMP280_DIG_T1_LSB_REG,&lsb,1);  
    bmp280_read_register(BMP280_DIG_T1_MSB_REG,&msb,1);  
    dig_T1 = msb << 8 | lsb;  
    bmp280_read_register(BMP280_DIG_T2_LSB_REG£¬&lsb£¬1);  
    bmp280_read_register(BMP280_DIG_T2_MSB_REG£¬&msb£¬1);  
    dig_T2 = msb << 8 | lsb;  
    bmp280_read_register(BMP280_DIG_T3_LSB_REG£¬&lsb£¬1);  
    bmp280_read_register(BMP280_DIG_T3_MSB_REG£¬&msb£¬1);  
    dig_T3 = msb << 8 | lsb;  
  
    /* read the pressure calibration parameters */  
    bmp280_read_register(BMP280_DIG_P1_LSB_REG£¬&lsb£¬1);  
    bmp280_read_register(BMP280_DIG_P1_MSB_REG£¬&msb£¬1);  
    dig_P1 = msb << 8 | lsb;  
    
    bmp280_read_register(BMP280_DIG_P2_LSB_REG£¬&lsb£¬1);  
    bmp280_read_register(BMP280_DIG_P2_MSB_REG£¬&msb£¬1);  
    dig_P2 = msb << 8 | lsb;  
  
    bmp280_read_register(BMP280_DIG_P3_LSB_REG£¬&lsb£¬1);  
    bmp280_read_register(BMP280_DIG_P3_MSB_REG£¬&msb£¬1);
    dig_P3 = msb << 8 | lsb;  
    
    bmp280_read_register(BMP280_DIG_P4_LSB_REG£¬&lsb£¬1);  
    bmp280_read_register(BMP280_DIG_P4_MSB_REG£¬&msb£¬1);
    dig_P4 = msb << 8 | lsb;  
 
    bmp280_read_register(BMP280_DIG_P5_LSB_REG£¬&lsb£¬1);  
    bmp280_read_register(BMP280_DIG_P5_MSB_REG£¬&msb£¬1);
    dig_P5 = msb << 8 | lsb;  

    bmp280_read_register(BMP280_DIG_P6_LSB_REG£¬&lsb£¬1);  
    bmp280_read_register(BMP280_DIG_P6_MSB_REG£¬&msb£¬1);
    dig_P6 = msb << 8 | lsb;  
  
    bmp280_read_register(BMP280_DIG_P7_LSB_REG£¬&lsb£¬1);  
    bmp280_read_register(BMP280_DIG_P7_MSB_REG£¬&msb£¬1);
    dig_P7 = msb << 8 | lsb;  

    bmp280_read_register(BMP280_DIG_P8_LSB_REG£¬&lsb£¬1);  
    bmp280_read_register(BMP280_DIG_P8_MSB_REG£¬&msb£¬1);
    dig_P8 = msb << 8 | lsb;  
  
    bmp280_read_register(BMP280_DIG_P9_LSB_REG£¬&lsb£¬1);  
    bmp280_read_register(BMP280_DIG_P9_MSB_REG£¬&msb£¬1);
    dig_P9 = msb << 8 | lsb;  
#endif
    
    
   
    bmp280_reset();  
		delay_ms(60);
		
    ctrlmeas_reg = g_bmp280->t_oversampling << 5 | g_bmp280->p_oversampling << 2 | g_bmp280->mode;  
    config_reg = g_bmp280->t_sb << 5 | g_bmp280->filter_coefficient << 2;  
  
    bmp280_write_register(BMP280_CTRLMEAS_REG,&ctrlmeas_reg,1);  
		delay_ms(50);
    bmp280_write_register(BMP280_CONFIG_REG, &config_reg,1);  
    delay_ms(50);
}  
  
void bmp280_reset()  
{  
    uint8_t reg_val;
    reg_val = BMP280_RESET_VALUE;
    bmp280_write_register(BMP280_RESET_REG, &reg_val,1);  
}  
  
void bmp280_set_standby_time(BMP280_T_SB t_standby)  
{  
    uint8_t config_reg;  
  
    g_bmp280->t_sb = t_standby;  
    config_reg = g_bmp280->t_sb << 5 | g_bmp280->filter_coefficient << 2;  
  
    bmp280_write_register(BMP280_CONFIG_REG, &config_reg,1);  
}  
  
void bmp280_set_work_mode(BMP280_WORK_MODE mode)  
{  
    uint8_t ctrlmeas_reg;
    
    g_bmp280->mode = mode;  
    ctrlmeas_reg = g_bmp280->t_oversampling << 5 | g_bmp280->p_oversampling << 2 | g_bmp280->mode;  
  
    bmp280_write_register(BMP280_CTRLMEAS_REG, &ctrlmeas_reg,1);  
}  
  
void bmp280_set_temperature_oversampling_mode(BMP280_T_OVERSAMPLING t_osl)  
{  
    uint8_t ctrlmeas_reg;  
  
    g_bmp280->t_oversampling = t_osl;  
    ctrlmeas_reg = g_bmp280->t_oversampling << 5 | g_bmp280->p_oversampling << 2 | g_bmp280->mode;  
  
    bmp280_write_register(BMP280_CTRLMEAS_REG, &ctrlmeas_reg,1);  
}  
  
void bmp280_set_pressure_oversampling_mode(BMP280_P_OVERSAMPLING p_osl)  
{  
    uint8_t ctrlmeas_reg;  
  
    g_bmp280->t_oversampling = p_osl;  
    ctrlmeas_reg = g_bmp280->t_oversampling << 5 | g_bmp280->p_oversampling << 2 | g_bmp280->mode;  
  
    bmp280_write_register(BMP280_CTRLMEAS_REG, &ctrlmeas_reg, 1);  
}  
  
void bmp280_set_filter_mode(BMP280_FILTER_COEFFICIENT f_coefficient)  
{  
    uint8_t config_reg;  
  
    g_bmp280->filter_coefficient = f_coefficient;  
    config_reg = g_bmp280->t_sb << 5 | g_bmp280->filter_coefficient << 2;  
  
    bmp280_write_register(BMP280_CONFIG_REG, &config_reg, 1);  
}  
  
/* Returns temperature in DegC, double precision. Output value of â€?1.23â€?equals 51.23 DegC. */  
double bmp280_compensate_temperature_double(int32_t adc_T)  
{  
    double var1, var2, temperature;  
  
    var1 = (((double) adc_T) / 16384.0 - ((double) dig_T1) / 1024.0)  
            * ((double) dig_T2);  
    var2 = ((((double) adc_T) / 131072.0 - ((double) dig_T1) / 8192.0)  
            * (((double) adc_T) / 131072.0 - ((double) dig_T1) / 8192.0))  
            * ((double) dig_T3);  
    g_bmp280->t_fine = (int32_t) (var1 + var2);  
    temperature = (var1 + var2) / 5120.0;  
  
    return temperature;  
}  
  
  
/* Returns pressure in Pa as double. Output value of â€?6386.2â€?equals 96386.2 Pa = 963.862 hPa */  
double bmp280_compensate_pressure_double(int32_t adc_P)  
{  
    double var1, var2, pressure;  
  
    var1 = ((double) g_bmp280->t_fine / 2.0) - 64000.0;  
    var2 = var1 * var1 * ((double) dig_P6) / 32768.0;  
    var2 = var2 + var1 * ((double) dig_P5) * 2.0;  
    var2 = (var2 / 4.0) + (((double) dig_P4) * 65536.0);  
    var1 = (((double) dig_P3) * var1 * var1 / 524288.0  
            + ((double) dig_P2) * var1) / 524288.0;  
    var1 = (1.0 + var1 / 32768.0) * ((double) dig_P1);  
  
    if (var1 == 0.0) {  
        return 0; // avoid exception caused by division by zero  
    }  
  
    pressure = 1048576.0 - (double) adc_P;  
    pressure = (pressure - (var2 / 4096.0)) * 6250.0 / var1;  
    var1 = ((double) dig_P9) * pressure * pressure / 2147483648.0;  
    var2 = pressure * ((double) dig_P8) / 32768.0;  
    pressure = pressure + (var1 + var2 + ((double) dig_P7)) / 16.0;  
  
    return pressure;  
}  
  
  
/* Returns temperature in DegC, double precision. Output value of â€?1.23â€?equals 51.23 DegC. */  
double bmp280_get_temperature()  
{  
    uint8_t lsb, msb, xlsb;  
    int32_t adc_T;  
    double temperature;  
  
    bmp280_read_register(BMP280_TEMPERATURE_XLSB_REG,&xlsb, 1);  
    bmp280_read_register(BMP280_TEMPERATURE_LSB_REG,&lsb,1);  
    bmp280_read_register(BMP280_TEMPERATURE_MSB_REG,&msb,1);  
  
    adc_T = (msb << 12) | (lsb << 4) | (xlsb >> 4);  
    temperature = bmp280_compensate_temperature_double(adc_T); 
    return temperature;  
}  
  
/* Returns pressure in Pa as double. Output value of â€?6386.2â€?equals 96386.2 Pa = 963.862 hPa */  
double bmp280_get_pressure()  
{  
    uint8_t lsb, msb, xlsb;  
    int32_t adc_P;  
    double pressure;  
  
    bmp280_read_register(BMP280_PRESSURE_XLSB_REG,&xlsb, 1); 
		delay_ms(8);	
    bmp280_read_register(BMP280_PRESSURE_LSB_REG,&lsb,1); 
		delay_ms(8);	
    bmp280_read_register(BMP280_PRESSURE_MSB_REG,&msb,1); 
		delay_ms(8);	
  
    adc_P = (msb << 12) | (lsb << 4) | (xlsb >> 4);  
    pressure = bmp280_compensate_pressure_double(adc_P);  
  
    return pressure;  
}  
  

void bmp280_get_temperature_and_pressure(double *temperature, double *pressure)  
{  
    *temperature = bmp280_get_temperature();  
    *pressure = bmp280_get_pressure();  
}  
  

void bmp280_forced_mode_get_temperature_and_pressure(double *temperature, double *pressure)  
{  		
		bmp280_get_temperature_and_pressure(temperature, pressure);
		delay_ms(2);
		bmp280_set_work_mode(BMP280_FORCED_MODE);  
}  
  

void bmp280_demo(double *temperature, double *pressure)  
{  
    bmp280_init();  
  
    if(g_bmp280 != NULL) {  
        while(1) {  
            bmp280_forced_mode_get_temperature_and_pressure(temperature, pressure);  
  
            delay_ms(100);  
      } 
}  



}
