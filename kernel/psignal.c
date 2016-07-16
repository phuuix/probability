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
 *   Posix signal support
 *
 * @History
 *   AUTHOR         DATE                 NOTES
 *   
 */

#include <config.h>

#include <sys/types.h>
#include <assert.h>
#include <bsp.h>
#include <psignal.h>
#include <task.h>
#include <stdlib.h>
#include <ctype.h>

static uint32_t sigmask[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

/*******************************************************************
 * send signal:
 * 1. set signal to t_cursig
 * 2. if signal is blocked, return
 * 3. wakeup task or resume task
 * 4. schedule
 *
 * Here we don't treat SIGSTOP and SIGCONT separately
 *******************************************************************/

/*
 * send a signal to task
 * On success, zero is returned.  On error, -1  is  returned
 */
int sig_task(task_t task, int __sig)
{
	struct dtask *ptask;
	uint32_t f;

	if(task < 0 || task > MAX_TASK_NUMBER)
		return -1;

	if(__sig < 1 || __sig > _NSIG)
		return -1;

	ptask = &systask[task];

	ptask->sigctrl.t_cursig |= sigmask[__sig-1];
	if(ptask->sigctrl.t_cursig & ~(ptask->sigctrl.t_hold))
	{
		SYS_FSAVE(f);
		if(ptask->state == TASK_STATE_BLOCK)
		{
			systask[task].errcode = RSIGNAL;
			task_wakeup(task);
		}
		else if(ptask->state == TASK_STATE_SUSPEND)
		{
			systask[task].errcode = RSIGNAL;
			task_resume(task);
		}
		SYS_FRESTORE(f);
	}

	return 0;
}

/*******************************************************************
 * handle signal:
 * 1. if there aren't signals pending, return
 * 2. save t_hold
 * 3. clear the first found signal in t_cursig
 * 4. t_hold |= action's sa_mask
 * 5. t_hold |= signal
 * 6. call action's sa_handler function
 * 7. restore t_hold
 *
 * signal handling are called after task_schedule()
 *******************************************************************/

/*
 * judge if there is a signal is pending
 * this function is called 
 * return: the signal is pending, start from 1
 */
int sig_pending()
{
	uint32_t f;
	struct dtask *task;
	
	SYS_FSAVE(f);
	task = current;
	SYS_FRESTORE(f);

	if(task->sigctrl.t_cursig & ~task->sigctrl.t_hold)
	{
		int signum;
		sigset_t curset = task->sigctrl.t_cursig & ~task->sigctrl.t_hold;

		for(signum=0; signum<_NSIG; signum++)
		{
			if(curset & sigmask[signum])
			{
				/* clear signal */
				task->sigctrl.t_cursig &= ~sigmask[signum];
				if(task->sigctrl.t_action[signum].sa_handler)
				{
					return signum+1;
				}
			}
		}
	}
	return 0;
}


/* Get and/or change the set of blocked signals.  */
int sigprocmask (int __how, const sigset_t *__set, sigset_t *__oset)
{
	uint32_t f;
	struct dtask *task;
	sigset_t new_set, old_set;
	
	SYS_FSAVE(f);
	task = current;
	SYS_FRESTORE(f);
    
    old_set = task->sigctrl.t_hold;
	
    if ( NULL != __set ) {
		new_set = *__set;
			
		switch ( __how )
		{
			case SIG_BLOCK:
			task->sigctrl.t_hold |= new_set;
			break;

			case SIG_UNBLOCK:
			task->sigctrl.t_hold &= ~new_set;
			break;

			case SIG_SETMASK:
			task->sigctrl.t_hold = new_set;
			break;

			default:
			return( -1 );
		}
    }
	
    if ( NULL != __oset ) {
		*__oset = old_set;
    }
    return 0;
}

/* Change the set of blocked signals to SET,
   wait until a signal arrives, and restore the set of blocked signals.  */
int sigsuspend (const sigset_t *__set)
{
	uint32_t f;
	struct dtask *task;
	sigset_t old_set;
	
	SYS_FSAVE(f);
	task = current;
	SYS_FRESTORE(f);

    old_set = task->sigctrl.t_hold;

    task->sigctrl.t_hold = *__set;
 
    task_suspend(TASK_T(task));

    task->sigctrl.t_hold = old_set;
 
    return( 0 );
}

/* Get and/or set the action for signal SIG.  */
int sigaction (int __sig, const struct sigaction *__act, struct sigaction *__oact)
{
	uint32_t f;
	struct dtask *task;
	
	SYS_FSAVE(f);
	task = current;
	SYS_FRESTORE(f);
    
    if ( __sig < 1 || __sig > _NSIG ) {
		return( -1 );
    }

    if ( SIGKILL == __sig || SIGSTOP == __sig ) {
		return( -1 );
    }

	/* save old action */
    if ( NULL != __oact ) {
		*__oact = task->sigctrl.t_action[__sig - 1];
    }

	/* set new action */
    if ( NULL != __act ) {
		task->sigctrl.t_cursig &= ~(1L << (__sig - 1));
		task->sigctrl.t_action[__sig - 1] = *__act;
    }
    return( 0 );
}

/* Put in SET all signals that are blocked and waiting to be delivered.  */
int sigpending (sigset_t *__set)
{
	uint32_t f;
	struct dtask *task;
	
	SYS_FSAVE(f);
	task = current;
	SYS_FRESTORE(f);

	*__set = task->sigctrl.t_cursig & task->sigctrl.t_hold;
	
    return( 0 );
}

/* Clear all signals from SET.  */
int sigemptyset(sigset_t *__set)
{
	*__set = 0;
    return( 0 );
}

/* Set all signals in SET.  */
int sigfillset(sigset_t *__set)
{
	*__set = ~0;
    return( 0 );
}

/* Add SIGNO to SET.  */
int sigaddset(sigset_t *__set, int __signo)
{
	*__set |= 1L << (__signo - 1);
    return( 0 );
}

/* Remove SIGNO from SET.  */
int sigdelset(sigset_t *__set, int __signo)
{
	*__set &= ~(1L << (__signo - 1));
    return( 0 );
}

/* Return 1 if SIGNO is in SET, 0 if not.  */
int sigismember(const sigset_t *__set, int __signo)
{
	if ( *__set & (1L << (__signo - 1)) ) {
		return( 1 );
    } else {
		return( 0 );
    }
}

/* TODO: events like PSOS can be set up on signal */
