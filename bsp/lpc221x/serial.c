/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * Copyright (C) 1999 2000 2001 Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <bsp.h>
#include "lpc2xxx.h"
#include "serial.h"


/*
 * Initialise the serial port with the given baudrate. The settings
 * are always 8 data bits, no parity, 1 stop bit, no start bits.
 *
 */
void serial_init(void)
{
    lpc221x_uart_init(SERIAL_UART_PORT, 115200, PCLK);
}


/*
 * Output a single byte to the serial port.
 */
void serial_putc(const char c)
{
    while(!lpc221x_uart_tx_ready(SERIAL_UART_PORT))
		;

	/* \n ==> \r\n */
	if(c == '\n')
		lpc221x_uart_tx_byte(SERIAL_UART_PORT, '\r');
    lpc221x_uart_tx_byte(SERIAL_UART_PORT, c);
}

int serial_tstc(void)
{
	return lpc221x_uart_rx_ready(SERIAL_UART_PORT);
}

/*
 * Read a single byte from the serial port. Returns 1 on success, 0
 * otherwise. When the function is succesfull, the character read is
 * written into its argument c.
 */
int serial_getc(void)
{
    while (!lpc221x_uart_rx_ready(SERIAL_UART_PORT))
      ;

    return lpc221x_uart_rx_byte(SERIAL_UART_PORT);
}

void serial_puts(char *str)
{
	while(*str)
		serial_putc(*str++);
}

