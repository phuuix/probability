/*
 * serial.h
 */
#ifndef __D_SERIAL_H__
#define __D_SERIAL_H__

#define SERIAL_UART_PORT 0

void serial_setbrg(int baudrate);
void serial_init(void);
void serial_putc(const char c);
int serial_tstc(void);
int serial_getc(void);
void serial_puts(char *str);

#endif /* __D_SERIAL_H__ */
