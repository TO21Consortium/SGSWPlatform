#include "led.h"




void
led_init(void)
{
	led_rcc_config();
	led_gpio_config();
}

void
led_rcc_config(void)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
}

void
led_gpio_config(void)
{	
	//gpio init
	GPIO_InitTypeDef GPIO_InitStructure;
	
	GPIO_InitStructure.GPIO_Pin =GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	/* Force a low level on LEDs*/ 	
	GPIO_ResetBits(GPIOB,GPIO_Pin_15);
}

void
led_delay(uint32_t ncount)
{
	for(; ncount != 0; ncount--);
}

void
led_clk(void)
{
	GPIO_ResetBits(GPIOB,GPIO_Pin_15);
	led_delay(0xfffff);	
	GPIO_SetBits(GPIOB,GPIO_Pin_15);
	led_delay(0xfffff);
}
