#ifndef _RTC_TIMER_H_
#define _RTC_TIMER_H_

#include <stdint.h>

enum rtc_timer_clock {
	RTC_TIMER_CLOCK_LSI = 0,
	RTC_TIMER_CLOCK_LSE
};

/**
  * @brief  Init system timer based on RTC clock  
  * @Note   Initialize clocks, PLL, RTC, adapt RTC if on LSI
  * @param  rtc_irq_handler RTC clock IRQ handler
  * @param  wake_period scheduler period (RTC period)
  * @param  useLSE use external clock LSE or internal clock LSI
  */
void rtc_timer_init(void (*rtc_irq_handler)(void), uint32_t wake_period, enum rtc_timer_clock use_clock);

/**
  * @brief  Reconfigure RTC clock IRQ handler
  * @param  rtc_irq_handler RTC clock IRQ handler
  */
void rtc_timer_update_irq_callback(void (*rtc_irq_handler)(void));

/**
  * @brief  Update RTC wake up period
  * @Note   Enable again the RTC if RTC was enabled
  * @param  wake_period scheduler period (RTC period)
  * @retval error (if negative)
  */
int32_t rtc_timer_update_wake_period(uint32_t wake_period);

/**
  * @brief  Get timestamps from RTC calendar counters in microsecond and prevent rollover
  * @retval timestamps in microsecond
  */
uint64_t rtc_timer_get_time_us(void);

#endif /* _RTC_TIMER_H_ */