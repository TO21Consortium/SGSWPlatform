#include "misc.h"
#include "usart_1.h"
#include "led.h"
#include "i2c_slave.h"
#include "i2c_master.h"
#include "delay.h"
#include "akm09916.h"
#include "icm20608.h"
#include "ltr553.h"
#include "bmp280.h"
#include "gpio.h"
#include "icm20608.h"

/* std */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "timer.h"

int ischangeflag = 0;
int count = 0;
int PITCH = 0;
int Is_power_off = 0;

int cnt = 0;
unsigned char flag = 0;
int16_t acc_data[3] = {0};
int16_t ecompass_data[3]={0};
int16_t gysensor_data[3] = {0};
unsigned long als_data = 0;

void Delay (uint32_t nCount);
extern int i2c_irq_flg;
extern int i2c_reset;
extern uint8_t ps_value;
extern uint8_t ps_int;
unsigned char test_data[30] = {0x11, 0x22, 0x33, 0x44,0x88,0x99,0x12,0x23,0x56};
unsigned char tmpRV[30] = {0};
unsigned char tmpwrite[30] = {0};
unsigned short recv_len = 0;

unsigned int step ;
unsigned int yesterday_step = 0;
unsigned int today_step = 0;
float tempValue[4] = {0};
int tempCount = 0;
int isDirectionUp = 0;
int continueUpCount = 0;
int continueUpFormerCount = 0;
int lastStatus = 0;
float peakOfWave = 0;
float valleyOfWave = 0;
unsigned int timeOfThisPeak = 0;
unsigned int timeOfLastPeak = 0;
unsigned int timeOfNow = 0;
float gravityNew = 0;
float gravityOld = 0;
static float initialValue = 1.3;
float ThreadValue = 2.0;

extern unsigned int sys_ms;
int stepSendCount = 1;
int Calculate_Pitch(int16_t *acc_data)
{
		float gravity=0.0;
		int pitch = 0;
		signed short acc_Xvalue = 0, acc_Yvalue = 0, acc_Zvalue = 0;
		acc_Xvalue = (signed short)acc_data[0];
		acc_Yvalue = (signed short)acc_data[1];
		acc_Zvalue = (signed short)acc_data[2];

		gravity =sqrt(acc_Xvalue * acc_Xvalue +acc_Yvalue * acc_Yvalue +acc_Zvalue * acc_Zvalue);
	
		pitch  = (int)(asin(acc_Zvalue / gravity)*573.25);	//573.25 = 1800/3.14			
		return pitch;
}


void stm_gpio_config(void)
{
	//gpio init
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA,GPIO_Pin_0);
}

float get_gravity(int16_t *acc_data)
{
		float gravity=0.0;
	  float x = 0.0,y = 0.0, z = 0.0;
		x = (float)acc_data[0];
		y = (float)acc_data[1];
		z = (float)acc_data[2];
		
		x = (-1) * 39.2 * x / 65536;
		y = (-1) * 39.2 * y / 65536;
		z = 39.2 * z / 65536;
		gravity =sqrt(x * x + y * y + z * z);
	
		return gravity;
}


float averageValue(float value[], int n)
{
    float ave = 0;
		int i = 0;
    for (i = 0; i < n; i++)
	  {
        ave += value[i];
    }
    ave = ave / 4;
    if (ave >= 8)
        ave = (float) 4.3;
    else if (ave >= 7 && ave < 8)
        ave = (float) 3.3;
    else if (ave >= 4 && ave < 7)
        ave = (float) 2.3;
    else if (ave >= 3 && ave < 4)
        ave = (float) 2.0;
    else
		{
        ave = (float) 1.3;
    }
    return ave;
}

float Peak_Valley_Thread(float value)
{
		int i = 0;
    float tempThread = ThreadValue;
    if (tempCount < 4)
		{
        tempValue[tempCount] = value;
        tempCount++;
    }
		else
		{
        tempThread = averageValue(tempValue, 4);
        for (i = 1; i < 4; i++)
			  {
            tempValue[i - 1] = tempValue[i];
        }
            tempValue[3] = value;
    }
    return tempThread;
}

int DetectorPeak(float newValue, float oldValue)
{
    if(newValue >= 15)
		{
        return 0;
    }
    lastStatus = isDirectionUp;
    if (newValue >= oldValue)
		{
        isDirectionUp = 1;
        continueUpCount++;
    }
		else
		{
        continueUpFormerCount = continueUpCount;
        continueUpCount = 0;
        isDirectionUp = 0;
    }

    if (!isDirectionUp && lastStatus && (continueUpFormerCount >= 2 || oldValue >= 20))
		{
        peakOfWave = oldValue;
        return 1;
    }
		else if (!lastStatus && isDirectionUp)
		{
        valleyOfWave = oldValue;
        return 0;
    }
		else
		{
        return 0;
    }
}

void DetectorNewStep(float values)
{
		if (gravityOld == 0)
		{
				gravityOld = values;
    }
		else
		{
				if (DetectorPeak(values, gravityOld))
				{
						timeOfLastPeak = timeOfThisPeak;
						timeOfNow = sys_ms;
            if (timeOfNow - timeOfLastPeak >= 2 && (peakOfWave - valleyOfWave >= ThreadValue))
						{
								timeOfThisPeak = timeOfNow;
                step++;
            }
            if (timeOfNow - timeOfLastPeak >= 2 && (peakOfWave - valleyOfWave >= initialValue))
						{
                timeOfThisPeak = timeOfNow;
                ThreadValue = Peak_Valley_Thread(peakOfWave - valleyOfWave);
            }
        }
    }
    gravityOld = values;
}

#if 0
int Is_power_off(void)
{
		if(0 == i2c_irq_flg)
			cnt++;
		else
			cnt = 0;
		if(cnt > 5)
			return 1;
		else
			return 0;

}

#endif

void send_step_to_kernel()
{
			memset(tmpRV, 0, sizeof(tmpRV));
			tmpRV[0] = 0x5a;
			tmpRV[1] = (unsigned char)(yesterday_step >> 8);
			tmpRV[2] = (unsigned char)yesterday_step;
			tmpRV[3] = 0x5a;
			tmpRV[4] = (unsigned char)(today_step >> 8);
			tmpRV[5] = (unsigned char)today_step;
	    	tmpRV[6] = (unsigned char)(stepSendCount >> 8);
			tmpRV[7] = (unsigned char)stepSendCount;
	    	stepSendCount++;
			I2CDRV_SLAVE_SendData(tmpRV,8);
			step = 0;
			today_step = 0;
			yesterday_step = 0;	
}
void wake_up_system()
{
		GPIO_ResetBits(GPIOA,GPIO_Pin_0);
		delay_ms(30);
		GPIO_SetBits(GPIOA,GPIO_Pin_0);
}

int adjust()
{
	PITCH = Calculate_Pitch(acc_data);
	if((abs(PITCH) > 800) || (abs(PITCH) < 100 && flag))
	{
		if(Is_power_off && ischangeflag)
		{
			if(count > 5)
			{
				if(flag == 1){
					flag = 0;
				}else{
					flag = 1;
				}
				wake_up_system();
				count = 0;
				ischangeflag = 0;
				return 1;
			}
			else
			{
				count++;
			}
		}
	}
	else
	{
		ischangeflag = 1;
	}
	return 0;
}

int main(void) 
{   
	unsigned char accel_fsr,  new_temp = 0;
  unsigned short gyro_rate, gyro_fsr;
	unsigned long timestamp = 0;
	unsigned char sensors, more;
	
	double temperature = 0;
  double pressure = 0;
	float press_val = 0; 
	uint16_t ps_real_val = 0;
	//led_init();
	//usart1_init();
	I2CDRV_Init();
	
	//delay_ms(10500);//delay for wait other sensor power on
	
	
	i2c_master_init();
	//press sensor
	bmp280_init();
	//compass msensor 
	ak09916_compare_whoami();
  ak0991x_enable_sensor(1);
	ak09916_run_selftest();
	// p+l sensor
  ltr553_dev_init();
  ltr559_ps_enable(1);
  ltr553_als_enable(1);
//gpio_init_p_sensor_int();
	
	icm_init();
	 /* Get/set hardware configuration. Start gyro. */
  /* Wake up all sensors. */
  icm_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
  /* Push both gyro and accel data into the FIFO. */
  icm_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
  icm_set_sample_rate(DEFAULT_ICM_HZ);
  /* Read back configuration in case it was set improperly. */
  icm_get_sample_rate(&gyro_rate);
  icm_get_gyro_fsr(&gyro_fsr);
  icm_get_accel_fsr(&accel_fsr);
	
	delay_ms(100);
	//stm_gpiob_config();
	/*add by tong 20161208 add timer fun*/
	initialTimer();
	/*end by tong 20161208*/
	stm_gpio_config();
	while (1)
	{
		if(i2c_reset == 1)
		{
			memset(tmpwrite, 0, sizeof(tmpwrite));
			I2CDRV_SLAVE_ReadData(tmpwrite,&recv_len);
			i2c_reset = 0;

			switch(tmpwrite[0])
			{

				case 0x66:
						Is_power_off= 0;
						send_step_to_kernel();
						break;
				case 0x77:
						Is_power_off= 1;
						break;
				case 0x55:
						NVIC_SystemReset();
						break;
				default:
						sys_time.hours = tmpwrite[0];
						sys_time.min = tmpwrite[1];
						sys_time.sec = tmpwrite[2];
						break;
		    }
		}

		if(!Is_power_off)
		{
				memset(tmpRV, 0, sizeof(tmpRV));
				ak09916_poll_data(ecompass_data);
				memcpy(tmpRV, ecompass_data, 6);
				bmp280_forced_mode_get_temperature_and_pressure(&temperature, &pressure);
				press_val=(float)pressure;
				memcpy(tmpRV+6,&press_val,4);
				ltr553_ps_read(&ps_real_val);
				if(ps_real_val > 750)
				{
						ps_value = 0;  /*close*/
				}
				else
				{
						ps_value = 5;  /*far away*/
				}
				memcpy(tmpRV+10,&ps_value,1);
				ltr559_als_read(&als_data);
				memcpy(tmpRV+11,&als_data,4);
				icm_get_gyro_reg(gysensor_data, &timestamp);
				icm_get_accel_reg(acc_data, &timestamp);
				//icm_read_fifo(gysensor_data, acc_data, &timestamp,&sensors, &more);	
				memcpy(tmpRV+15,&acc_data,6);
				memcpy(tmpRV+21,&gysensor_data,6);
				delay_ms(1);
				
				if(i2c_irq_flg)
				{
						I2CDRV_SLAVE_SendData(tmpRV,28);
						i2c_irq_flg = 0;
				}
		}
		else
		{
			icm_get_accel_reg(acc_data, &timestamp);
			DetectorNewStep(get_gravity(acc_data));
			if(sys_time.hours == 23 && sys_time.min == 59 && sys_time.sec == 59)  
			{
				yesterday_step = step;
				today_step = 0;
				step = 0;
				stepSendCount = 0;
			}
			else
			{
				today_step = step;
			}

			adjust();
		}
	}
	
}
void Delay (uint32_t nCount)
{
  for(; nCount != 0; nCount--);
}

