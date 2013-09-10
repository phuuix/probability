/**
 * Copyright (c) 2011-2012 Phuuix Xiong <phuuix@163.com>
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
 * @file
 * A portable timer lib
 *
 * @History
 * Phuuix Xiong, Create, 01-25-2011
 */

#ifndef __PTIMER_H__
#define __PTIMER_H__

#include <list.h>

#define PTIMER_FLAG_RUNNING 0x01
#define PTIMER_FLAG_PERIODIC 0x02


typedef void (*onexpired_func_t)(void *, uint32_t, uint32_t);

/* timer */
typedef struct ptimer
{
	dllist_node_t node;		//keep it at first byte
	uint32_t flags;				//bit0: 0 -- not running, 1 -- running
	uint32_t duration;
	uint32_t remainder;
	onexpired_func_t onexpired_func;
	uint32_t param[2];
}ptimer_t;


/* timer table */
typedef struct ptimer_table
{
	dllist_node_t *table;
	uint16_t allslots;
	uint16_t curslot;
}ptimer_table_t;

int ptimer_is_running(ptimer_t *timer);
int ptimer_init(ptimer_table_t *table, uint16_t allslots);
void ptimer_consume_time(ptimer_table_t *table, uint32_t time);
void ptimer_cancel(ptimer_table_t *table, ptimer_t *timer);
int ptimer_start(ptimer_table_t *table, ptimer_t *timer, uint32_t timeval);
int ptimer_init_nomalloc(ptimer_table_t *table, uint16_t allslots, dllist_node_t *slot);

#endif //__PTIMER_H__
