/*
statemachine.h

state machine 
===========================
event: 
internal event (create by state machine itself, ie. timer event) or external event (create outside the state machine)
0 is reserved as invalid event;
This model doesn't specify how the event dispatcher/filter is implemented and leave it to designer.
---------------------------
state:
predefined state list start from 0 to N (for simple of implementation)
---------------------------
event processing function: 
one handler for each state; easy to extend
---------------------------
event queue: 
Every state machine has a FIFO event queue. Once a event is enqueued, the state machine will
be placed onto active list waiting for scheduling.
---------------------------
child state machine:
same as normal state machine; 
Child state machine could be created or destroy dynamically;
each state machine contains a pointer linked to its parent.

*/
#ifndef _STATEMACHINE_H_
#define _STATEMACHINE_H_

#include <stdio.h>

#include "list.h"
#include "uprintf.h"

/* several special event code reserved */
#define STATEMACHINE_EVENT_INVALID 0x0
#define STATEMACHINE_EVENT_TIMEOUT 0xFF
#define STATEMACHINE_EVENT_CHILDEND 0xFE

#define STATEMACHINE_EVENTQUEUE_SIZE 4 // must be power of 2, for optimization
#define STATEMACHINE_NUM_PARAM 2

typedef uint32_t statemachine_state_t;

typedef uint32_t (*statemachine_func_t)(struct statemachine *, uint32_t, struct statemachine_event *);
typedef uint32_t (*on_exit_func_t)(struct statemachine *, uint32_t);

typedef struct statemachine_event
{
	uint8_t event;
	uint8_t data8;
	uint16_t data16;
	uint32_t data32;
}statemachine_event_t;

typedef struct statemachine_scheduler
{
	// active state machine list
	dllist_node_t activelst;
}statemachine_scheduler_t;

typedef struct statemachine_transition
{
	uint32_t state;
	statemachine_func_t func;
}statemachine_eventhandler_t;

#define STATEMACHINE_FLAGS_VALID 1
#define STATEMACHINE_FLAGS_READY 2
typedef struct statemachine
{
    // node is for active list; must keep node at start of structure
	dllist_node_t node;
	
	uint32_t flags;
	uint8_t current_state;
	uint8_t num_handler;
	uint8_t event_queue_head;
	uint8_t event_queue_tail;
	statemachine_event_t event_queue[STATEMACHINE_EVENTQUEUE_SIZE];
	
    ptimer_t timer;                             // default timer node
	struct statemachine *parent;                // link to parent
	struct statemachine_scheduler *scheduler;   // link to state machine scheduler
	statemachine_eventhandler_t *handlers;      // event handlers
    on_exit_func_t on_exit_cb;                  // callback on exit of state machine
	
	void *parameter[STATEMACHINE_NUM_PARAM];
}statemachine_t;

void statemachine_init(statemachine_t *sm, statemachine_t *parent, statemachine_scheduler_t *scheduler, 
						statemachine_eventhandler_t *handlers, uint32_t num_handler, uint8_t state);

uint32_t statemachine_send_event(statemachine_t *sm, statemachine_event_t *event);

uint32_t statemachine_take_event(statemachine_t *sm, statemachine_event_t *event);

uint32_t statemachine_set_state(statemachine_t *sm, uint32_t new_state);

void statemachine_scheduling(statemachine_scheduler_t *scheduler);

#endif //_STATEMACHINE_H_
