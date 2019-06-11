#ifndef _DELAY_H_
#define _DELAY_H_

#include <stdint.h>

/**
  * @brief  Busy wait based on a 100MHz clock timer
  * The timer is start and stop for each call to this function to avoid power consumption
  * @warning Maximum timing value supported is 40s
  * @param  Timing in us 
  */
void delay_us(uint32_t us);

/**
  * @brief  Busy wait based on a 100MHz clock timer
  * The timer is start and stop for each call to this function to avoid power consumption
  * @warning Maximum timing value supported is 40s
  * @param  Timing in ms 
  */
void delay_ms(uint32_t ms);
void delay(int ms);

#endif

