/*************************************************************************/
/* The Dooloo kernel                                                     */
/* Copyright (C) 2004-2006 Xiong Puhui (Bearix)                          */
/* All Rights Reserved.                                                  */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF DOOLOO RTOS DEVELOPMENT TEAM                          */
/*                                                                       */
/*************************************************************************/

/*************************************************************************/
/*                                                                       */
/* FILE                                       VERSION                    */
/*   psignal.h                                 0.3.0                     */
/*                                                                       */
/* COMPONENT                                                             */
/*   Kernel                                                              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*   Posix signal support                                                */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-20               Version 0.3.0                */
/*************************************************************************/ 

/*
 * psignal.h
 * Posix signal support
 */

#ifndef __D_PSIGNAL_H__
#define __D_PSIGNAL_H__

#include <config.h>
#include <sys/types.h>

/* some of following definations are copyed from Linux */

/* Fake signal functions.  */
#define SIG_ERR ((__sighandler_t) -1)       /* Error return.  */
#define SIG_DFL ((__sighandler_t) 0)        /* Default action.  */
#define SIG_IGN ((__sighandler_t) 1)        /* Ignore signal.  */

#ifdef __USE_UNIX98
# define SIG_HOLD   ((__sighandler_t) 2)    /* Add signal to hold mask.  */
#endif

/* Signals.  */
#define SIGHUP      1   /* Hangup (POSIX).  */
#define SIGINT      2   /* Interrupt (ANSI).  */
#define SIGQUIT     3   /* Quit (POSIX).  */
#define SIGILL      4   /* Illegal instruction (ANSI).  */
#define SIGTRAP     5   /* Trace trap (POSIX).  */
#define SIGABRT     6   /* Abort (ANSI).  */
#define SIGIOT      6   /* IOT trap (4.2 BSD).  */
#define SIGBUS      7   /* BUS error (4.2 BSD).  */
#define SIGFPE      8   /* Floating-point exception (ANSI).  */
#define SIGKILL     9   /* Kill, unblockable (POSIX).  */
#define SIGUSR1     10  /* User-defined signal 1 (POSIX).  */
#define SIGSEGV     11  /* Segmentation violation (ANSI).  */
#define SIGUSR2     12  /* User-defined signal 2 (POSIX).  */
#define SIGPIPE     13  /* Broken pipe (POSIX).  */
#define SIGALRM     14  /* Alarm clock (POSIX).  */
#define SIGTERM     15  /* Termination (ANSI).  */
#define SIGSTKFLT   16  /* Stack fault.  */
#define SIGCLD      SIGCHLD /* Same as SIGCHLD (System V).  */
#define SIGCHLD     17  /* Child status has changed (POSIX).  */
#define SIGCONT     18  /* Continue (POSIX).  */
#define SIGSTOP     19  /* Stop, unblockable (POSIX).  */
#define SIGTSTP     20  /* Keyboard stop (POSIX).  */
#define SIGTTIN     21  /* Background read from tty (POSIX).  */
#define SIGTTOU     22  /* Background write to tty (POSIX).  */
#define SIGURG      23  /* Urgent condition on socket (4.2 BSD).  */
#define SIGXCPU     24  /* CPU limit exceeded (4.2 BSD).  */
#define SIGXFSZ     25  /* File size limit exceeded (4.2 BSD).  */
#define SIGVTALRM   26  /* Virtual alarm clock (4.2 BSD).  */
#define SIGPROF     27  /* Profiling alarm clock (4.2 BSD).  */
#define SIGWINCH    28  /* Window size change (4.3 BSD, Sun).  */
#define SIGPOLL     SIGIO   /* Pollable event occurred (System V).  */
#define SIGIO       29  /* I/O now possible (4.2 BSD).  */
#define SIGPWR      30  /* Power failure restart (System V).  */
#define SIGSYS      31  /* Bad system call.  */
#define SIGUNUSED   31

#ifndef _NSIG
#define _NSIG       32  /* Biggest signal number + 1 */
#endif

/* Bits in `sa_flags'.  */
#define SA_NOCLDSTOP  1      /* Don't send SIGCHLD when children stop.  */
#define SA_NOCLDWAIT  2      /* Don't create zombie on child death.  */
#define SA_SIGINFO    4      /* Invoke signal-catching function with
                    three arguments instead of one.  */
#if defined __USE_UNIX98 || defined __USE_MISC
# define SA_ONSTACK   0x08000000 /* Use signal stack by using `sa_restorer'. */
# define SA_RESTART   0x10000000 /* Restart syscall on signal return.  */
# define SA_NODEFER   0x40000000 /* Don't automatically block the signal when
                    its handler is being executed.  */
# define SA_RESETHAND 0x80000000 /* Reset to SIG_DFL on entry to handler.  */
#endif
#ifdef __USE_MISC
# define SA_INTERRUPT 0x20000000 /* Historical no-op.  */

/* Some aliases for the SA_ constants.  */
# define SA_NOMASK    SA_NODEFER
# define SA_ONESHOT   SA_RESETHAND
# define SA_STACK     SA_ONSTACK
#endif

/* Values for the HOW argument to `sigprocmask'.  */
#define SIG_BLOCK     0      /* Block signals.  */
#define SIG_UNBLOCK   1      /* Unblock signals.  */
#define SIG_SETMASK   2      /* Set the set of blocked signals.  */

#ifndef _SIGSET_T_
#define	_SIGSET_T_
typedef unsigned int sigset_t;
#endif

struct sigaction 
{
	void (*sa_handler)(int);
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_restorer)(void);
};

struct signal_ctrl
{
	sigset_t t_cursig;
	sigset_t t_sig;
	sigset_t t_hold;
//	sigset_t t_ignore;
	struct sigaction t_action[_NSIG];
};

/* Get and/or change the set of blocked signals.  */
int sigprocmask (int __how, const sigset_t *__set, sigset_t *__oset);

/* Change the set of blocked signals to SET,
   wait until a signal arrives, and restore the set of blocked signals.  */
int sigsuspend (const sigset_t *__set);

/* Get and/or set the action for signal SIG.  */
int sigaction (int __sig, const struct sigaction *__act, struct sigaction *__oact);

/* Put in SET all signals that are blocked and waiting to be delivered.  */
int sigpending (sigset_t *__set);

/* Clear all signals from SET.  */
int sigemptyset(sigset_t *__set);

/* Set all signals in SET.  */
int sigfillset(sigset_t *__set);

/* Add SIGNO to SET.  */
int sigaddset(sigset_t *__set, int __signo);

/* Remove SIGNO from SET.  */
int sigdelset(sigset_t *__set, int __signo);

/* Return 1 if SIGNO is in SET, 0 if not.  */
int sigismember(const sigset_t *__set, int __signo);

int sig_task(int task, int sig);
int sig_pending();


#endif /* __D_PSIGNAL_H__ */
