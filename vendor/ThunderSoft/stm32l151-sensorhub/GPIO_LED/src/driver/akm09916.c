#include "akm09916.h"
#include "delay.h"

static unsigned long akm09916_i2c_write(unsigned char register_addr,const unsigned char *register_value,unsigned short register_len)
{  
  return i2c_master_write_register(AKM09916_I2C_ADDR,register_addr,register_len, register_value);
}

static unsigned long akm09916_i2c_read(unsigned char register_addr,unsigned char *register_value,unsigned short register_len)
{
  return i2c_master_read_register(AKM09916_I2C_ADDR,register_addr,register_len, register_value);
}

int ak09916_poll_data(int16_t *compass_data)
{
	unsigned long rc;
	uint8_t data[8] = {0};
	uint8_t mode;
        uint8_t counter = 10;        

	while (counter > 0) {
          rc = akm09916_i2c_read(AK0991x_REG_ST1, data, 8);
          if(rc != 0)
              return rc;
          /* get measurment values if drdy */
          if((data[0] & AK0991x_DRDY) != 0) {
			compass_data[0] = (((unsigned short)data[2]) << 8) | data[1];
			compass_data[1] = (((unsigned short)data[4]) << 8) | data[3];
			compass_data[2] = (((unsigned short)data[6]) << 8) | data[5];
                        counter = 0;
          }else{
                  counter--;
          }
	/* reload single measurment mode */
	mode = AK0991x_CNTL2_MODE_SINGLE;
	rc = akm09916_i2c_write(AK0991x_REG_CNTL2, &mode, 1);
	if(rc != 0)
		return rc;
			
          delay_ms(1);
	}

	return 0;
}

int ak09916_run_selftest()
{
	int result;
	uint8_t data[6], mode;
	uint8_t counter;
	int16_t x, y, z;
	//int shift;        
        const short AK09916_ST_Lower[3] = {-200, -200, -1000};
	const short AK09916_ST_Upper[3] = {200, 200, -200};
        /* set to power down mode */
	mode = AK0991x_CNTL2_MODE_POWER_DOWN;
	result = akm09916_i2c_write(AK0991x_REG_CNTL2, &mode, 1);
	if (result)
		goto AKM_fail;
        //delay(10000);
        delay_ms(100);
	/* set self test mode */
	mode = AK0991x_CNTL2_MODE_SELF_TEST;
	result = akm09916_i2c_write(AK0991x_REG_CNTL2, &mode, 1);
	if (result)
		goto AKM_fail;

	counter = 10;        

	while (counter > 0) {
		result = akm09916_i2c_read(AK0991x_REG_ST1, data, 1);
		if (result)
			goto AKM_fail;
		if ((data[0] & AK0991x_DRDY) == 0)
			counter--;
		else
			counter = 0;
                delay_ms(10);
	}
	if ((data[0] & AK0991x_DRDY) == 0) {
		result = -1;
		goto AKM_fail;
	}

	result = akm09916_i2c_read(AK0991x_REG_HXL, data, 6);
	if (result)
		goto AKM_fail;

	x = ((uint16_t)data[1]) <<8 | data[0];
	y = ((uint16_t)data[3]) <<8 | data[2];
	z = ((uint16_t)data[5]) <<8 | data[4];
        
	result = -1;
     
	if (x > AK09916_ST_Upper[0] || x <  AK09916_ST_Lower[0])
			goto AKM_fail;
	if (y >  AK09916_ST_Upper[1] || y < AK09916_ST_Lower[1])
			goto AKM_fail;
	if (z >  AK09916_ST_Upper[2] || z < AK09916_ST_Lower[2])
			goto AKM_fail;

	result = 0;
        //delay(10000);
        delay_ms(100);
AKM_fail:
	/* set to power down mode */        
	mode = AK0991x_CNTL2_MODE_POWER_DOWN;
	akm09916_i2c_write(AK0991x_REG_CNTL2, &mode, 1);

	return result;
}
//return 1 means success, return 0 means fail
int ak09916_compare_whoami()
{
        uint8_t whoami[2]={0};
	int rc;
        rc = akm09916_i2c_read(AK0991x_REG_WIA1, whoami, 2);
	if(rc != 0)
		goto err;

	if(whoami[0] == AKM09916_COMPANY_ID && whoami[1] == AKM09916_Device_ID)
        {
          return 1;
        }
err:
	return 0;
}

int ak09916_soft_reset()
{
	uint8_t soft_reset = 1;
	return akm09916_i2c_write(AK0991x_REG_CNTL3, &soft_reset, 1);
}

int ak0991x_enable_sensor(int en)
{
	int rc = 0;       
        
	if(en) {
		uint8_t mode = AK0991x_CNTL2_MODE_SINGLE;
                //rc = ak09916_soft_reset();
		rc = akm09916_i2c_write(AK0991x_REG_CNTL2, &mode, 1);
	}
	else {
		rc = ak09916_soft_reset();
	}
	return rc;
}
