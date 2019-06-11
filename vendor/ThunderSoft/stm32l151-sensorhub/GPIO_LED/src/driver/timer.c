/* std */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "gpio.h"
#include "timer.h"

SYS_TIME sys_time;
unsigned int sys_ms = 0;
void timer_init()
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);	
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);	                            
	TIM_TimeBaseStructure.TIM_Period =  325;	//500;           
	TIM_TimeBaseStructure.TIM_Prescaler =  12000;//1199;	           
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1 ;	
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;               
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE );
	TIM_ARRPreloadConfig(TIM2, ENABLE);			 
	TIM_Cmd(TIM2, ENABLE); 
}
	
void NVIC_Configuration(void)  
{  
  NVIC_InitTypeDef NVIC_InitStructure;  
    
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);      
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;  
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  
  NVIC_Init(&NVIC_InitStructure);  
}

void TIM2_IRQHandler(void)
{
  TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

	sys_ms++;
  sys_time.ms++;
	if(sys_time.ms>=4){
		sys_time.ms = 0;
		sys_time.sec += 1;
		if(sys_time.sec >=60){
			sys_time.sec = 0;
			sys_time.min += 1;
			if(sys_time.min >=60){			   			
				sys_time.min = 0;  			
				sys_time.hours += 1;
			}
			if(sys_time.hours >=24){ 		
				sys_time.hours = 0;
			}
		}
	}
}

void initialTimer()
{
	timer_init();
	NVIC_Configuration();
}

