/* diet stdio */

#include <sys/cdefs.h>
#include <sys/types.h>
#include "dietfeatures.h"
#include <stdarg.h>

#ifdef WANT_SMALL_STDIO_BUFS
#define BUFSIZE 128
#else
#define BUFSIZE 2048
#endif

#define ERRORINDICATOR 1
#define EOFINDICATOR 2
#define BUFINPUT 4
#define BUFLINEWISE 8
#define NOBUF 16
#define STATICBUF 32
#define FDPIPE 64
#define CANREAD 128
#define CANWRITE 256
#define CHECKLINEWISE 512

#define _IONBF 0
#define _IOLBF 1
#define _IOFBF 2

#include <stdio.h>

/* ..scanf */
struct arg_scanf {
  void *data;
  int (*getch)(void*);
  int (*putch)(int,void*);
};

int __v_scanf(struct arg_scanf* fn, const char *format, va_list arg_ptr);

struct arg_printf {
  void *data;
  int (*put)(void*,size_t,void*);
};

int __v_printf(struct arg_printf* fn, const char *format, va_list arg_ptr);

