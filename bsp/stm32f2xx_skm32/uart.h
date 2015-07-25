/*
 * uart.h
 */
#ifndef __D_UARTR_H__
#define __D_UARTR_H__


#define SERIAL_UART_PORT 0
#define SERIAL_UART_BAUDRATE 115200

void uart_init(uint8_t port, uint32_t baudrate);
void uart_putc(uint8_t port, uint8_t c);
void uart_putc_ex(uint8_t port, uint8_t c);
uint16_t uart_getc(uint8_t port);

#endif /* __D_UARTR_H__ */
