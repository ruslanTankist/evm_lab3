#include "uart.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned char byte;

#define UART0_INT_MASK    0x00000040
#define UART0_READ_READY  (U0LSR&0x01)
#define UART0_WRITE_READY (U0LSR&0x20)

static bool UART_initialized  = false;

static
byte
UART0_read_byte(void)
{
	byte b = '\0';
	
	while(!UART0_READ_READY);
	b = U0RBR;
	
	return b;
}

static
void
UART0_write_byte(byte b)
{
	while(!UART0_WRITE_READY);
	U0RBR = b;
}

int
UART0_read_line(char *buf, size_t buf_len)
{
	assert(buf);
	assert(buf_len > 0);
	
	int retval = 0;
	size_t i   = 0;
	char input = '\0';
	
	do {
		input = UART0_read_byte();
		UART0_write_byte(input); //echo
		
		switch (input) {
			case '\r':
				goto done;
			default:
				if (i == buf_len-1)
					goto overflow;
				buf[i] = input;
		}
		i++;
	} while (1);

overflow:
	retval = UART_ERR_BUFFER_OVERFLOW;
done:
	buf[i] = '\0';
	UART0_write_byte('\n');
	return retval;
}

void
UART0_write_line(const char *msg)
{
	assert(msg);
	
	for (size_t i=0; i<strlen(msg); i++) {
		UART0_write_byte(msg[i]);
	}
	
	UART0_write_byte('\n');
}

void
UART0_reg_int_handler(UART_interrupt_handler h)
{
	if (VICIntEnable & UART0_INT_MASK) {
		//setting NULL interrupt handler when interrapts are enabled may lead to undefined behaviour
		assert(h);
	}
	VICVectAddr6 = (unsigned) h;
}

void
UART0_int_enable(void)
{
	VICIntEnable |= UART0_INT_MASK;
}

void
UART0_int_disable(void)
{
	VICIntEnable &= !UART0_INT_MASK;
}

int UART_init (void) {
	if (UART_initialized)
		return UART_ERR_ALREADY_INITIALIZED;
		
	UART_initialized = true;
	
	//Разрешить альтернативные UART0 функции входов/выходов P0.2 и P0.3: TxD и RxD
	PINSEL0 = 0x00000050;
	//Установить параметры передачи: 8 бит, без контроля четности, 1 стоповый бит
	//+Разрешить запись делителя частоты CLK_UART0
	U0LCR   = 0x00000083;
	//Установить делитель частоты на скорость 4800 при частоте CLK_UART0 = 12MHz
	U0DLL   = 0x00000068;
	//Дополнительный делитель частоты (DivAddVal/MulVal + 1) = 0.5 + 1 = 1.5
	U0FDR   = 0x00000021;
	//Фиксировать делитель частоты
	U0LCR   = 0x00000003;
	//Программировать FIFO буфер на прием 8-ми байт.
	U0FCR   = 0x00000081;
	//Разрешить прерывание по приему
	U0IER   = 0x00000001;
	
	return 0;
}
