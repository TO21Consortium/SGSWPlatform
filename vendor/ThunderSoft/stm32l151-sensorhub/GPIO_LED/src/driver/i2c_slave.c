/*************************************************************************************************/
/** @file   i2c_slave.c
    @brief  Implementation of I2C slave

	@company Thundersoft\n
              Xi'an
	$Author: zhouyibo
	$Revision: 1
	$Date: 2016-07-21

*/
/*************************************************************************************************/

/**************************************************************************************************
 * Includes
**************************************************************************************************/
#include "i2c_slave.h"
#include "stm32l1xx_i2c.h"

/**************************************************************************************************
 * Private definition
**************************************************************************************************/
//#define I2CDRV_GPIO_PORT        GPIOB                      /**< I2C GPIO port*/
//#define I2CDRV_SCL_PIN          GPIO_Pin_8                 /**< I2C SCL GPIO Pin*/
//#define I2CDRV_SDA_PIN          GPIO_Pin_7                 /**< I2C SDA GPIO Pin*/
#define I2CDRV_GPIO_PORT        GPIOB                      /**< I2C GPIO port*/
#define I2CDRV_SCL_PIN          GPIO_Pin_10                 /**< I2C SCL GPIO Pin*/
#define I2CDRV_SDA_PIN          GPIO_Pin_11                /**< I2C SDA GPIO Pin*/

#define I2CDRV_SPEED            100000
#define I2CDRV_SLAVE_ADDR       0x40




//static i2c_buff_struct_t sI2CRBuffHandle;
static uint8_t               m_u8RxBuff[128] = {0};
static uint8_t               m_u8TxBuff[128] = {0};
static uint32_t              m_u32RxIndex;
static uint32_t              m_u32TxIndex;
//static uint32_t              m_u32TxMax;
int IntFlg = 0;
/**************************************************************************************************
 * Private declarations
**************************************************************************************************/
void I2C2_EV_IRQHandler(void);
void I2C2_ER_IRQHandler(void);
static void I2CDRV_InitHardware(void);
static void I2CDRV_Config(void);
int i2c_irq_flg = 0;
int i2c_reset = 0;


/*************************************************************************************************/
/** @brief I2C Event IRQ handler
*/
/*************************************************************************************************/
void I2C2_EV_IRQHandler(void)
{
    uint32_t u32Event = I2C_GetLastEvent(I2CDRV_Type);  /* the value of the last event */
    switch(u32Event)
    {
        //The address sent by the master matches the own address of the peripheral
        case I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED:
           IntFlg = (IntFlg << 4) + 1;
						i2c_reset = 1;
            m_u32RxIndex = 0;
            //The slave stretches SCL low until ADDR is
            //cleared and DR filled with the data to be sent
            //I2C_ClearFlag(I2CDRV_Type,I2C_FLAG_ADDR);
            I2C_ReadRegister(I2CDRV_Type,I2C_Register_SR1);
            I2C_ReadRegister(I2CDRV_Type,I2C_Register_SR2);
            break;
        //The application is expecting a data byte to be received
        case I2C_EVENT_SLAVE_BYTE_RECEIVED:
          IntFlg = (IntFlg << 4) + 2;
            m_u8RxBuff[m_u32RxIndex++] = I2C_ReceiveData(I2CDRV_Type);
            break;
        case I2C_EVENT_SLAVE_STOP_DETECTED:
          IntFlg = (IntFlg << 4) + 3;
            I2C_GetFlagStatus(I2CDRV_Type, I2C_FLAG_STOPF);
	    I2C_Cmd(I2CDRV_Type, ENABLE);
            break;
        case I2C_EVENT_SLAVE_TRANSMITTER_ADDRESS_MATCHED:
          m_u32TxIndex = 0;
            I2C_ReadRegister(I2CDRV_Type,I2C_Register_SR1);
            I2C_ReadRegister(I2CDRV_Type,I2C_Register_SR2);
        //  IntFlg = (IntFlg << 4) + 4;
            I2C_SendData(I2CDRV_Type, m_u8TxBuff[m_u32TxIndex++]);
            i2c_irq_flg = 1;
            break;
        case I2C_EVENT_SLAVE_BYTE_TRANSMITTED:
            IntFlg = (IntFlg << 4) + 5;
            //i2c_irq_flg = 1;
            /* #REDMINEID lili1201 Modified 2016-7-30 7:16 Start */
            #if 1 /*lili1201  comment out 2016-7-30 7:17*/
            //if (m_u32TxIndex < m_u32TxMax)
            I2C_SendData(I2CDRV_Type, m_u8TxBuff[m_u32TxIndex++]);
            //else
            //I2C_ClearFlag(I2CDRV_Type, I2C_FLAG_AF);
            #else
            //TsSensorHub_SendData(sI2CRBuffHandle.pTxBufHandle);
            #endif /* comment out 2016-7-30 7:17 */
            break;
        default:
            break;
    }
}

/*************************************************************************************************/
/** @brief I2C Error IRQ handler
*/
/*************************************************************************************************/
void I2C2_ER_IRQHandler(void)
{
    uint32_t u32Event = I2C_GetLastEvent(I2CDRV_Type); /* the error value of the last event */
    if( u32Event & I2C_EVENT_SLAVE_ACK_FAILURE)
    {
      IntFlg = (IntFlg << 4) + 6;
        I2C_ClearFlag(I2CDRV_Type,I2C_FLAG_AF);
    }
}

/*************************************************************************************************/
/** @brief Initializes I2C hardware
*/
/*************************************************************************************************/
static void I2CDRV_InitHardware(void)
{
    GPIO_InitTypeDef stGPIOInitStructure; /* GPIO config structure */
    /* Enable I2C clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);

    /* Enable GPIO clock */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);

    /* Reset I2C IP */
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, ENABLE);

    /* Release reset signal of I2C IP */
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C2, DISABLE);
    /* Connect PXx to I2C_SCL*/
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);

    /* Connect PXx to I2C_SDA*/
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_I2C2);

    /* GPIO configuration */
    /* Configure I2C pins: SCL */
    stGPIOInitStructure.GPIO_Pin = I2CDRV_SCL_PIN;
    stGPIOInitStructure.GPIO_Mode = GPIO_Mode_AF;
    stGPIOInitStructure.GPIO_Speed = GPIO_Speed_40MHz;
    stGPIOInitStructure.GPIO_OType = GPIO_OType_OD;
    stGPIOInitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &stGPIOInitStructure);

    /* Configure I2C pins: SDA */
    stGPIOInitStructure.GPIO_Pin = I2CDRV_SDA_PIN;
    GPIO_Init(GPIOB, &stGPIOInitStructure);
}

/*************************************************************************************************/
/** @brief Config I2C
 */
/*************************************************************************************************/
static void I2CDRV_Config(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    I2C_InitTypeDef stI2CInitStructure;      /* I2C config structure */
    //I2C_DeInit(I2CDRV_Type); 
    /* I2C configuration */
    stI2CInitStructure.I2C_Mode = I2C_Mode_SMBusDevice;
    //stI2CInitStructure.I2C_Mode = I2C_Mode_I2C;
    stI2CInitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    stI2CInitStructure.I2C_OwnAddress1 = I2CDRV_SLAVE_ADDR;
    stI2CInitStructure.I2C_Ack = I2C_Ack_Enable;
    stI2CInitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    stI2CInitStructure.I2C_ClockSpeed = I2CDRV_SPEED;

    /* I2C Peripheral Enable */
    I2C_Cmd(I2CDRV_Type, ENABLE);
	I2C_ARPCmd(I2CDRV_Type, ENABLE);
    /* Apply I2C configuration after enabling it */
    I2C_Init(I2CDRV_Type, &stI2CInitStructure);
    I2C_AcknowledgeConfig(I2CDRV_Type,ENABLE);
    /* Config the I2C Events Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel =  I2C2_EV_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    /* Config the I2C Errors Interrupt */
    NVIC_InitStructure.NVIC_IRQChannel = I2C2_ER_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    /* Enable I2c Interrupt */
    I2C_ITConfig(I2CDRV_Type, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR, ENABLE);
}

/*************************************************************************************************/
/** @brief Initializes I2C driver interface

    @retval I2CDRV_OK
    @retval I2CDRV_FAIL
*/
/*************************************************************************************************/
uint8_t I2CDRV_Init(void)
{
    uint8_t u8Status = I2CDRV_OK;
    static uint8_t u8FirstInit = 0;

    /* Ensure that this funciton is only called once */
    if(u8FirstInit > 0)
    {
        return I2CDRV_OK;
    }
    else
    {
        u8FirstInit = 1;
    }
    m_u32RxIndex = 0;
    m_u32TxIndex = 0;
   // m_u32TxMax = 0;
    /* #REDMINEID lili1201 Add. use ringbuffer for avoid data loss 2016-7-30 6:5 Start */
   // I2CDRV_SLAVE_init_buff();
    /* #REDMINEID lili1201 Add 2016-7-30 6:5 End */
    I2CDRV_InitHardware();
    I2CDRV_Config();
    return u8Status;
}

/*************************************************************************************************/
/** @brief send the data through I2C bus.
  
    @param p_pu8Buffer point to the data
    @param p_u16NumByte size of the data
    
    @retval I2CDRV_OK
    @retval I2CDRV_FAIL
*/ 
/*************************************************************************************************/
uint8_t I2CDRV_SLAVE_SendData(const uint8_t *p_pu8Buffer, uint16_t p_u16NumByte)
{
    uint8_t u8status = I2CDRV_OK;
    if((p_pu8Buffer != 0) && (p_u16NumByte != 0))
    {
        m_u32TxIndex = 0;
       // m_u32TxMax = p_u16NumByte;
        memcpy(m_u8TxBuff, p_pu8Buffer, p_u16NumByte);
    }
    return u8status;
}

/*************************************************************************************************/
/** @brief  read the data through I2C bus.

    @param p_pu8Buffer point to the data
    @param p_pu16NumByte Pointer to size of the data
    
    @retval I2CDRV_OK
    @retval I2CDRV_FAIL
*/
/*************************************************************************************************/
uint8_t I2CDRV_SLAVE_ReadData(uint8_t *p_pu8Buffer, uint16_t *p_pu16NumByte)
{
    uint8_t u8status = I2CDRV_OK;
    if((p_pu8Buffer != 0) && (m_u32RxIndex != 0))
    {
        memcpy(p_pu8Buffer, m_u8RxBuff, m_u32RxIndex);
        *p_pu16NumByte = m_u32RxIndex;
    }
    return u8status;
}
