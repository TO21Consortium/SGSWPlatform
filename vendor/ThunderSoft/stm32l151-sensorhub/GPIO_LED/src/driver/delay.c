/*
 * ________________________________________________________________________________________________________
 * Copyright (c) 2016-2016 InvenSense Inc. All rights reserved.
 *
 * This software, related documentation and any modifications thereto (collectively “Software”) is subject
 * to InvenSense and its licensors' intellectual property rights under U.S. and international copyright
 * and other intellectual property rights laws.
 *
 * InvenSense and its licensors retain all intellectual property and proprietary rights in and to the Software
 * and any use, reproduction, disclosure or distribution of the Software without an express license agreement
 * from InvenSense is strictly prohibited.
 *
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * EXCEPT AS OTHERWISE PROVIDED IN A LICENSE AGREEMENT BETWEEN THE PARTIES, IN NO EVENT SHALL
 * INVENSENSE BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THE SOFTWARE.
 * ________________________________________________________________________________________________________
 */

#include "delay.h"

#include "stm32l1xx.h"

#include <stdint.h>

// Divider for us based on 100MHz time clock
#define DELAY_TIMER_DIVIDER 100

static uint8_t setup_done = 0;

static void setup_timer(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

	/* Time base clock : 100MHz */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure); 
	TIM_TimeBaseStructure.TIM_Period = UINT16_MAX;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
}

static void start_timer(void) 
{
	if(!setup_done) {
		setup_timer();
		setup_done = 1;
	}

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
	TIM3->CNT = 0;
	TIM_Cmd(TIM3, ENABLE);
}

static void stop_timer(void) 
{
	TIM_Cmd(TIM3, DISABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, DISABLE);
}

static void internal_delay(uint16_t us)
{
	const uint16_t start = TIM3->CNT;

	uint32_t now, prev = 0;
	do{
		now = TIM3->CNT;

		/* handle rollover */
		if(now < prev)
			now = UINT16_MAX + now;
		prev = now;

	}while((now - start) <= us);
}

void delay_us(uint32_t us)
{
	uint32_t i;
	us = us * DELAY_TIMER_DIVIDER;

	start_timer();

	/* in case the delay is up to UINT16_MAX */
	if(us >= UINT16_MAX) {
		/* go to the loop as the internal_delay function only support uint16_t argument type */
		for(i = 0; i < (us / UINT16_MAX); i++)
			internal_delay(UINT16_MAX);
		internal_delay(us % UINT16_MAX);
	}
	else
		internal_delay(us);

	stop_timer();
}

void delay_ms(uint32_t ms)
{
	delay_us(ms * 1000);
}

void delay(int ms)
{
  while(ms--);
}
