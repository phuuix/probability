/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 *
 * Copyright (c) Puhui Xiong <phuuix@163.com>
 * @file
 *   A portable timer lib
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   
 */

#include <stdlib.h>
#include <assert.h>

#include <ptimer.h>

/***********************************************************************************/
/* Function : ptimer_start                                                         */
/***********************************************************************************/
/* Description : - Start a timer                                                   */
/*                                                                                 */
/*                                                                                 */
/* Interface :                                                                     */
/*      Name            | io |       Description                                   */
/* ---------------------|----|-----------------------------------------------------*/
/*   table              | i  | pointer to timer table                              */
/*   timer              | i  | timer                                               */
/*   timeval            | i  | expires time = now + timeval                        */
/*   Return             |    | N/A                                                 */
/***********************************************************************************/
int ptimer_start(ptimer_table_t *table, ptimer_t *timer, uint32_t timeval)
{
	uint32_t slot;
	
	if(table == NULL || timer == NULL)
		return PTIMER_RET_ERROR;

	/* In current design, even a timer is running, it can be restarted again.
	 * The only requirement is the timer node isn't in the list.
	 */
	assert(timer->node.next == NULL);
	assert(timer->node.prev == NULL);
	
	timer->flags |= PTIMER_FLAG_RUNNING;
	timer->duration = timeval;
	if(timeval >= table->allslots)
	{
		timer->remainder = timeval - table->allslots + 1;
		slot = (table->curslot + table->allslots - 1) & (table->allslots - 1);
	}
	else
	{
		timer->remainder = 0;
	
		/* find register slot */
		slot = (table->curslot + timeval) & (table->allslots - 1);
	}
	
	dllist_append(&table->table[slot], (dllist_node_t *)timer);
	
//	ZLOG_DEBUG("start timer: 0x%p timeval=%u, curslot=%u target_slot=%u\n", timer, timeval, table->curslot, slot);
	return PTIMER_RET_OK;
}


/***********************************************************************************/
/* Function : ptimer_start_remainder                                               */
/***********************************************************************************/
/* Description : - internal function                                               */
/*               - in case of remainder timer                                      */
/*                                                                                 */
/* Interface :                                                                     */
/*      Name            | io |       Description                                   */
/* ---------------------|----|-----------------------------------------------------*/
/*   table              | i  | pointer to timer table                              */
/*   timer              | i  | timer                                               */
/*   timeval            | i  | expires time = now + timeval                        */
/*   Return             |    | N/A                                                 */
/***********************************************************************************/
void ptimer_start_remainder(ptimer_table_t *table, ptimer_t *timer, uint32_t timeval)
{
	uint32_t slot;
			
	if(timeval >= table->allslots)
	{
		timer->remainder = timeval - table->allslots + 1;
		slot = (table->curslot + table->allslots - 1) & (table->allslots - 1);
	}
	else
	{
		timer->remainder = 0;
		
		/* find register slot */
		slot = (table->curslot + timeval) & (table->allslots - 1);
	}
	
    // in case of new slot equals current slot, we have to shift it by one
    if(slot == table->curslot)
        slot = (table->curslot + 1) & (table->allslots - 1);

	dllist_append(&table->table[slot], (dllist_node_t *)timer);
	
//	ZLOG_DEBUG("start timer remainder: 0x%p timeval=%u, curslot=%u target_slot=%u\n", timer, timeval, table->curslot, slot);
}


/***********************************************************************************/
/* Function : ptimer_cancel                                                        */
/***********************************************************************************/
/* Description : - Stop a timer                                                    */
/*                                                                                 */
/* Interface :                                                                     */
/*      Name            | io |       Description                                   */
/* ---------------------|----|-----------------------------------------------------*/
/*   table              | i  | pointer to timer table                              */
/*   timer              | i  | timer                                               */
/*   Return             |    | N/A                                                 */
/***********************************************************************************/
void ptimer_cancel(ptimer_table_t *table, ptimer_t *timer)
{
	if(table == NULL || timer == NULL)
		return;
		
//	ZLOG_DEBUG("cancel timer: 0x%p\n", timer);

	if(timer->flags & PTIMER_FLAG_RUNNING)
	{
		timer->flags &= ~PTIMER_FLAG_RUNNING;
		dllist_remove(NULL, (dllist_node_t *)timer);
		timer->node.next = NULL;
		timer->node.prev = NULL;
	}
}

/***********************************************************************************/
/* Function : ptimer_consume_time                                                  */
/***********************************************************************************/
/* Description : - consume time, it is up to user to call this function            */
/*                                                                                 */
/* Interface :                                                                     */
/*      Name            | io |       Description                                   */
/* ---------------------|----|-----------------------------------------------------*/
/*   table              | i  | pointer to timer table                              */
/*   time               | i  | passed time                                         */
/*   Return             |    | N/A                                                 */
/***********************************************************************************/
void ptimer_consume_time(ptimer_table_t *table, uint32_t time)
{
	ptimer_t *timer;
	uint32_t i, ret=PTIMER_RET_OK;
	
	if(table == NULL) return;
	
	for(i=0; i<time; i++)
	{
		while(!DLLIST_EMPTY(&table->table[table->curslot]))
		{
			timer = (ptimer_t *)DLLIST_HEAD(&table->table[table->curslot]);

			assert(ptimer_is_running(timer));

			/* remove all timers in current slot */
			dllist_remove(NULL, (dllist_node_t *)timer);
			timer->node.next = NULL;
			timer->node.prev = NULL;
			
			/* handle remainder */
			if(timer->remainder)
			{
				ptimer_start_remainder(table, timer, timer->remainder);
				continue;
			}
			
			/* call onexpired_func: the expire function can call ptimer_start again, so timer has to be removed firstly */
//			ZLOG_DEBUG("timer expired: 0x%p at slot %u\n", timer, table->curslot);
			if(timer->onexpired_func)
			{
				ret = timer->onexpired_func(timer, timer->param[0], timer->param[1]);
			}

			/* be careful: PTIMER_FLAG_RUNNING flag race here !!! */

			if(PTIMER_RET_OK == ret)
			{
				/* if periodic timer */
				if(timer->flags & PTIMER_FLAG_PERIODIC)
				{
					ptimer_start(table, timer, timer->duration);
				}
				else
				{
					/* timer normal ends: mark timer as not running */
					timer->flags &= ~PTIMER_FLAG_RUNNING;
				}
			}
			else if (ret < 0)
			{
				/* error occures: mark timer as not running */
				timer->flags &= ~PTIMER_FLAG_RUNNING;
			}
			/* else: timer is restarted, do nothing */
		}
		
		table->curslot = (table->curslot + 1) & (table->allslots - 1);
	}
}

/***********************************************************************************/
/* Function : ptimer_init                                                          */
/***********************************************************************************/
/* Description : - init timer per table mechanism                                  */
/*                                                                                 */
/* Interface :                                                                     */
/*      Name            | io |       Description                                   */
/* ---------------------|----|-----------------------------------------------------*/
/*   table              | o  | pointer to timer table                              */
/*   allslots           | i  | the number of time slot of table                    */
/*   Return             |    | 0 is success                                        */
/***********************************************************************************/
int ptimer_init(ptimer_table_t *table, uint16_t allslots)
{
	uint16_t vpower = 1;
	
	if(table == NULL)
		return PTIMER_RET_ERROR;
	
	if(allslots > (1<<15))	/* 32768 */
		vpower = (1<<15);
	else if(allslots < 64)
		vpower = 64;
	else
	{
		while(vpower < allslots)
			vpower = vpower << 1;
	}
	
	table->table = malloc(sizeof(dllist_node_t) * vpower);
	table->allslots = vpower;
	table->curslot = 0;
	
	if(table->table)
	{
		uint32_t i;
		for(i=0; i<table->allslots; i++)
		{
			dllist_init(&table->table[i]);
		}
	}
	
	return table->table ? PTIMER_RET_OK : PTIMER_RET_ERROR;
}

/* allslots must be power of 2 */
int ptimer_init_nomalloc(ptimer_table_t *table, uint16_t allslots, dllist_node_t *slot)
{
	if(table == NULL)
		return PTIMER_RET_ERROR;
	
	table->table = slot;
	table->allslots = allslots;
	table->curslot = 0;
	
	if(table->table)
	{
		uint32_t i;
		for(i=0; i<table->allslots; i++)
		{
			dllist_init(&table->table[i]);
		}
	}
	
	return table->table ? PTIMER_RET_OK : PTIMER_RET_ERROR;
}


/***********************************************************************************/
/* Function : ptimer_is_running                                                    */
/***********************************************************************************/
/* Description : - judge if timer is running                                       */
/*                                                                                 */
/* Interface :                                                                     */
/*      Name            | io |       Description                                   */
/* ---------------------|----|-----------------------------------------------------*/
/*   timer              | i  |                                                     */
/*   Return             |    | 1 is running                                        */
/***********************************************************************************/
int ptimer_is_running(ptimer_t *timer)
{
	return (timer->flags & PTIMER_FLAG_RUNNING);
}
