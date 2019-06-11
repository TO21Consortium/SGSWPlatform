#include "ltr553.h"
#include "delay.h"
#include "i2c_master.h"

//static int8_t ps_gainrange;
static int8_t als_gainrange;
static uint16_t ps_trigger_high = 800;
static uint16_t ps_trigger_low = 700;

static unsigned long ltr553_i2c_write(unsigned char register_addr,const unsigned char *register_value,unsigned short register_len)
{  
  return i2c_master_write_register(LTR553_I2C_ADDR,register_addr,register_len, register_value);
}

static unsigned long ltr553_i2c_read(unsigned char register_addr,unsigned char *register_value,unsigned short register_len)
{
  return i2c_master_read_register(LTR553_I2C_ADDR,register_addr,register_len, register_value);
}

static void ltr553_ps_set_thres(void)
{
	uint8_t databuf[2];
        //writer low_thres
	databuf[0] = (uint8_t)(ps_trigger_low & 0x00FF);
	ltr553_i2c_write(LTR559_PS_THRES_LOW_0, databuf, 1);
	
	databuf[0] = (uint8_t)((ps_trigger_low & 0xFF00) >> 8);
        ltr553_i2c_write(LTR559_PS_THRES_LOW_1, databuf, 1);
        
        //write high thres	
	databuf[0] = (uint8_t)(ps_trigger_high & 0x00FF);
	ltr553_i2c_write(LTR559_PS_THRES_UP_0, databuf, 1);
		
	databuf[0] = (uint8_t)((ps_trigger_high & 0xFF00) >> 8);;
	ltr553_i2c_write(LTR559_PS_THRES_UP_1, databuf, 0x2);
}


int8_t ltr553_dev_init(void)
{
        int8_t res;
       // uint8_t init_ps_gain;
        
	uint8_t databuf[2];	 
	databuf[0] = SW_reset;
	ltr553_i2c_write(LTR559_ALS_CONTR, databuf, 1);//sw_reset
	
	delay_ms(PON_DELAY);
        
	//init_ps_gain = MODE_PS_Gain16;
        databuf[0] = 0x7F;
	ltr553_i2c_write(LTR559_PS_LED, databuf, 1);
        databuf[0] = 0x6;
        ltr553_i2c_write(LTR559_PS_N_PULSES, databuf, 1); 
        databuf[0] = 0x0;
	ltr553_i2c_write(LTR559_PS_MEAS_RATE, databuf, 1); 
        databuf[0] = 0x01;
	ltr553_i2c_write(LTR559_ALS_MEAS_RATE, databuf, 1); 

	als_gainrange = ALS_RANGE_8K;

	/*for interrup work mode support */
	//ltr553_ps_set_thres();
		
	databuf[0] = 0x00;
	ltr553_i2c_write(LTR559_INTERRUPT, databuf, 1);
	
	databuf[0] = 0x40;
	ltr553_i2c_write(LTR559_INTERRUPT_PERSIST, databuf, 1);
   delay_ms(WAKEUP_DELAY*2);
        
   databuf[0] = 0;
	ltr553_i2c_read(LTR559_ALS_PS_STATUS, databuf, 1);
        
   res = -1;
  if(0 != (databuf[0] & 0x02))
	{
            res = 0;
	}
	return res;
}

void ltr559_ps_enable(uint8_t enable)
{
	uint8_t regdata;
	ltr553_i2c_read(LTR559_PS_CONTR,&regdata,1);
	
	if (enable == 1)
		regdata = 0x2b;
	else 
		regdata &= 0xfc;

	ltr553_i2c_write(LTR559_PS_CONTR,&regdata,1);

	delay_ms(WAKEUP_DELAY);
}

void ltr553_als_enable(uint8_t enable)
{	
	uint8_t regdata=0;	

	ltr553_i2c_read(LTR559_ALS_CONTR,&regdata,1);

	if (enable == 1) 
		regdata = 0x0D;
	else 
		regdata &= 0xfe;

	ltr553_i2c_write(LTR559_ALS_CONTR, &regdata,1);
	delay_ms(WAKEUP_DELAY);
}


void ltr553_ps_read(uint16_t *data)
{
	uint8_t psval_lo,psval_hi;
  uint16_t psdata;

	ltr553_i2c_read(LTR559_PS_DATA_0,&psval_lo,1);
	ltr553_i2c_read(LTR559_PS_DATA_1,&psval_hi,1);
	psdata = ((psval_hi & 7)* 256) + psval_lo;
	*data = psdata;
}



void ltr559_als_read(unsigned long *data)
{
	uint8_t alsval_ch0_lo, alsval_ch0_hi,alsval_ch1_lo,alsval_ch1_hi;
	uint16_t alsval_ch0,alsval_ch1;
	int  luxdata_int;
	int ratio;
        uint8_t first_flag = 1;

als_data_try:
	ltr553_i2c_read(LTR559_ALS_DATA_CH0_0,&alsval_ch0_lo,1);
	ltr553_i2c_read(LTR559_ALS_DATA_CH0_1,&alsval_ch0_hi,1);
	alsval_ch0 = (alsval_ch0_hi * 256) + alsval_ch0_lo;
        
        ltr553_i2c_read(LTR559_ALS_DATA_CH1_0,&alsval_ch1_lo,1);
	ltr553_i2c_read(LTR559_ALS_DATA_CH1_1,&alsval_ch1_hi,1);
	alsval_ch1 = (alsval_ch1_hi * 256) + alsval_ch1_lo;

    	if((alsval_ch1==0)||(alsval_ch0==0))
    	{
       	 ratio = 100;
    	}else{
		ratio = (alsval_ch1*100) /(alsval_ch0+alsval_ch1);
        }
		
	if (ratio < 45){
		luxdata_int = (((17743 * alsval_ch0)+(11059 * alsval_ch1))/als_gainrange)/1000;
	}
	else if ((ratio < 64) && (ratio >= 45)){
		luxdata_int = (((42785 * alsval_ch0)-(19548 * alsval_ch1))/als_gainrange)/1000;
	}
	else if ((ratio < 85) && (ratio >= 64)) {
		luxdata_int = (((5926 * alsval_ch0)+(1185 * alsval_ch1))/als_gainrange)/1000;
	}
	else {
                if(first_flag == 1)
                {       
                    first_flag = 0;
                    delay_ms(2);
                    goto als_data_try;
                }
                else
                {
                  luxdata_int = 0;
                }
	}
        
	*data = luxdata_int;
}

uint8_t ltr553_get_ps_value(uint16_t ps)
{
	uint8_t val = 0;

	if(ps > 800)
	{
		val = 5;  /*close*/
	}
	else if(ps < 700)
	{
		val = 0;  /*far away*/
	}
        else
          val = 0;
        return val;
}
