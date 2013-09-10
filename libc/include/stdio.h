#ifndef _STDIO_H
#define _STDIO_H

#include <sys/cdefs.h>
#include <sys/types.h>


int printf(const char *format, ...) __THROW __attribute__((format(printf,1,2)));
int sprintf(char *str, const char *format, ...) __THROW __attribute__((format(printf,2,3)));
int snprintf(char *str, size_t size, const char *format, ...) __THROW __attribute__((format(printf,3,4)));
//int asprintf(char **ptr, const char* format, ...) __THROW __attribute_malloc__ __attribute__((format(printf,2,3)));

int scanf(const char *format, ...) __THROW __attribute__((format(scanf,1,2)));
int sscanf(const char *str, const char *format, ...) __THROW __attribute__((format(scanf,2,3)));

#include <stdarg.h>

int vprintf(const char *format, va_list ap) __THROW __attribute__((format(printf,1,0)));
int vsprintf(char *str, const char *format, va_list ap) __THROW __attribute__((format(printf,2,0)));
int vsnprintf(char *str, size_t size, const char *format, va_list ap) __THROW __attribute__((format(printf,3,0)));

int vscanf(const char *format, va_list ap) __THROW __attribute__((format(scanf,1,0)));
int vsscanf(const char *str, const char *format, va_list ap) __THROW __attribute__((format(scanf,2,0)));

int getchar(void) __THROW;
char *gets(char *s) __THROW;
int putchar(int c) __THROW;
int puts(const char *s) __THROW;

#define EOF (-1)

#define BUFSIZ 128

#define _IONBF 0
#define _IOLBF 1
#define _IOFBF 2

#endif
