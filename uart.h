#ifndef UART_H_
#define UART_H_ 1

#include <LPC23xx.H>
#include <stdio.h>
#include <string.h>

#define UART_ERR_ALREADY_INITIALIZED -1
#define UART_ERR_BUFFER_OVERFLOW     -2

#define UART_MAX_WRITE_LEN 256

typedef void (*UART_interrupt_handler)(void) __irq;

void
UART0_reg_int_handler(UART_interrupt_handler h);
void
UART0_int_enable(void);
void
UART0_int_disable(void);

int
UART0_read_line(char *buf, size_t buf_len);
void
UART0_write_line(const char *msg);
#define UART0_write_line_fmt(_fmt, ...)                  \
do {                                                     \
	char _msg[UART_MAX_WRITE_LEN] = "\0";                  \
	assert(_fmt);                                          \
	snprintf(_msg, UART_MAX_WRITE_LEN, _fmt, __VA_ARGS__); \
	UART0_write_line(_msg);                                \
} while(0)

int 
UART_init(void);

#endif
