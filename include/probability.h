#ifndef _PROBABILITY_H_
#define _PROBABILITY_H_

/* please keep this file included as late as possible */

#define OS_VERSION "0.40"

#ifndef FALSE
#define FALSE 0L
#endif

#ifndef TRUE
#define TRUE 1L
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef offsetof
#define offsetof(type,member) ((size_t) &((type*)0)->member)
#endif

#ifndef container_of
#define container_of(node, type, member)            \
    ((type *)((char *)(node) - offsetof(type, member)))
#endif


#define TIME_WAIT_FOREVER	-1
#define TIME_WAIT_NONE		0

#define ROK			0
#define RERROR		-1
#define RTIMEOUT	2
#define RAGAIN		3
#define RSIGNAL		4

#endif // _PROBABILITY_H_
