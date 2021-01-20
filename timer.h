#ifndef TIMER_H_
#define TIMER_H_ 1

#include <LPC23xx.H>
#include <time.h>

void
delay(time_t seconds);

void
timer0_init(void);

#endif
