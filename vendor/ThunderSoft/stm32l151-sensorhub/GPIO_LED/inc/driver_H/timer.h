#ifndef _TIMER_H
#define _TIMER_H

typedef	struct{
	char hours;
	char min;
	char sec;
	char ms;
}SYS_TIME;

extern SYS_TIME    sys_time;

extern void initialTimer();

#endif