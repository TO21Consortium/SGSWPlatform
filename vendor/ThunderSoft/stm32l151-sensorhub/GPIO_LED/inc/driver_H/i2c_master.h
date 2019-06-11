/** @defgroup I2C master peripheral
	@ingroup  Driver
	@{
*/
#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

/** @brief Configures I2C master peripheral
*/
void i2c_master_init(void);

/** @brief Desactivates the I2C master peripheral
*/
void i2c_master_deinit(void);

/** @brief Read a register through the control interface I2C 
* @param[in] address, I2c 7bit-address
* @param[in] register_addr, register address (location) to access
* @param[in] register_len, length value to read
* @param[in] register_value, pointer on byte value to read
* @retval 0 if correct communication, else wrong communication
*/
unsigned long i2c_master_read_register(unsigned char address, unsigned char register_addr, 
                                          unsigned short register_len, unsigned char *register_value);

/** @brief Write a register through the control interface I2C  
* @param[in] address, I2c 7bit-address
* @param[in] register_addr, register address (location) to access
* @param[in] register_len, length value to write
* @param[in] register_value, pointer on byte value to write
* @retval 0 if correct communication, else wrong communication
*/
unsigned long i2c_master_write_register(unsigned char address, unsigned char register_addr, 
                                           unsigned short register_len, const unsigned char *register_value);

#endif /* __I2C_MASTER_H__ */


