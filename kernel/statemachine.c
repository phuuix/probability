/* statemachine.c */

#include "statemachine.h"

static int32_t statemachine_default_timeout(ptimer_t *timer, uint32_t param1, uint32_t param2)
{
    statemachine_t * sm = (statemachine_t *)param1;
    statemachine_event_t event;

    event.event = STATEMACHINE_EVENT_TIMEOUT;
    statemachine_send_event(sm, &event);
}

void statemachine_timer_init(statemachine_t *sm)
{
    sm->timer.onexpired_func = statemachine_default_timeout;
    sm->timer.param[0] = (uint32_t)sm;
}

void statemachine_init(statemachine_t *sm, statemachine_t *parent, 
                        statemachine_scheduler_t *scheduler, statemachine_eventhandler_t *handlers, 
                        uint32_t num_handler, on_exit_func_t on_exit_cb, uint8_t state)
{
	assert(sm);
	assert(scheduler);
	assert(handlers);
	assert(num_handler > 0);
	
	memset(sm, 0, sizeof(statemachine_t);
	sm->parent = parent;
	sm->scheduler = scheduler;
	sm->handlers = handlers;
	sm->num_handler = num_handler;
    sm->on_exit_cb = on_exit_cb;
	sm->current_state = state;
	sm->flags = STATEMACHINE_FLAGS_VALID;

    /* setup the default timer, you can override it to your own */
    statemachine_timer_init(sm);
}

uint32_t statemachine_send_event(statemachine_t *sm, statemachine_event_t *event)
{
	assert(sm);
	assert(sm->scheduler);
	assert(event);
	assert(event->event != STATEMACHINE_EVENT_INVALID);
	
	if(sm->event_queue_tail.event == STATEMACHINE_EVENT_INVALID)
	{
		sm->event_queue[sm->event_queue_tail] = *event;
		sm->event_queue_tail ++;
		sm->event_queue_tail &= (STATEMACHINE_EVENTQUEUE_SIZE-1);
		
		if(sm->flags & STATEMACHINE_FLAGS_READY == 0)
		{
			// once a event is enqueued, add the state machine to active list
			dllist_append(&sm->scheduler->activelst, &sm->node);
			sm->flags |= STATEMACHINE_FLAGS_READY;
		}
		return 0;
	}
	
	// event queue is full
	return -1;
}

uint32_t statemachine_take_event(statemachine_t *sm, statemachine_event_t *event)
{
	assert(sm);
	assert(sm->flags);
	assert(event);
	
	if(sm->event_queue_head.event != STATEMACHINE_EVENT_INVALID)
	{
		*event = sm->event_queue[sm->event_queue_head];
		// set the current event invalid
		sm->event_queue[sm->event_queue_head].event = STATEMACHINE_EVENT_INVALID;
		sm->event_queue_head ++;
		sm->event_queue_head &= (STATEMACHINE_EVENTQUEUE_SIZE-1);
		
		return 0;
	}
	
	// empty
	return -1;
}

uint32_t statemachine_set_state(statemachine_t *sm, uint32_t new_state)
{
	assert(sm);
	assert(new_state < sm->num_handler);
	
	sm->current_state = new_state;
}

void statemachine_scheduling(statemachine_scheduler_t *scheduler)
{
	dllist_node_t *list;
	statemachine_t *sm;
	statemachine_event_t event;
	uint32_t ret = 0;
	
	assert(scheduler);
	
	list = &scheduler->activelst;
	while(!dllist_isempty(list))
	{
		sm = (statemachine_t *)list->next;
		assert(sm->transition_table);
		
		/* consume all events in a state machine */
		while((statemachine_take_event(sm, event) == 0))
		{
			/* call the handler function to process event */
			ret = sm->transition_table[sm->current_state].func(sm, event);
            if(ret != 0)
            {
                sm->on_exit_cb(sm, ret);
                break;
            }
		}
		
		/* remove state machine from the active list */
		dllist_remove(list, sm);
		sm->flags &= (~STATEMACHINE_FLAGS_READY);
	}
}

