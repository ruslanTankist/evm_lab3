#include "timer.h"

#include <assert.h>
#include <stdbool.h>
#include <LPC23xx.h>

#define TIMER0_INT_SRC 0x00000010

static bool timer0_initialized = false;

volatile static bool fiq_int_catched = false;

void
FIQ_int() __irq
{
	fiq_int_catched = true;
	T0MCR = 0x00000006;
	T0IR = 0x00000001;
	VICIntEnClr |= TIMER0_INT_SRC;
}

void
delay(time_t seconds)
{
	assert(timer0_initialized);
	
	T0MR0 = (time_t)(seconds * 1000);
	//Сбросить таймер
	T0TC = 0x00000000;
	//Запустить таймер
	T0TCR = 0x00000001;
	
	T0MCR = 0x00000003;
	
	T0IR = 0x00000001;
	
	VICVectAddr4 = (unsigned) FIQ_int;
	VICIntSelect |= TIMER0_INT_SRC;
	VICIntEnable |= TIMER0_INT_SRC;
	
	while (!fiq_int_catched);
	VICIntSelect = 0x00000000;
	VICVectAddr = 0;
	fiq_int_catched = false;
}

void
timer0_init(void)
{
	assert(!timer0_initialized);
	
	//Предделитель таймера = 6000
	T0PR = 6000;
	//Сбросить счетчик и делитель
	T0TCR = 0x00000002;
	
	timer0_initialized = true;
}
