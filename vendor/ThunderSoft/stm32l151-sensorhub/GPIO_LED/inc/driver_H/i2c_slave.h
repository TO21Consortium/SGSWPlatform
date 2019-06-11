#ifndef _I2C_SLAVE_H_
#define _I2C_SLAVE_H_

#include <stdint.h>
#include "stm32l1xx.h"
#include <string.h>
/**************************************************************************************************
 * global variables
**************************************************************************************************/

/**************************************************************************************************
 * global typedefs
**************************************************************************************************/
typedef enum
{
	EN_WORDADDR = 0,	    /**< 16 bits */
	EN_BYTEADDR = 1	        /**< 8 bit */
}EnI2CAddrType_t;

/**************************************************************************************************
 * global functions prototypes
**************************************************************************************************/

#define I2CDRV_OK               (0)
#define I2CDRV_FAIL             (1)

#define I2CDRV_Type             I2C2

/** define the operation direction of I2C */
#define I2CDRV_DIRECTION_TX           (0)
#define I2CDRV_DIRECTION_RX           (1)

#define INVALID_ADDR         ((uint32_t)0xFFFFFF)
#ifdef _cplusplus
extern "C" {
#endif
extern uint8_t  I2CDRV_Init(void);
extern uint8_t  I2CDRV_SLAVE_SendData(const uint8_t *p_pu8Buffer, uint16_t p_u16NumByte);
extern uint8_t  I2CDRV_SLAVE_ReadData(uint8_t *p_pu8Buffer, uint16_t *p_pu16NumByte);
#ifdef _cplusplus
}
#endif
#endif /* _I2C_SLAVE_H_ */

