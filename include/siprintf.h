#ifndef __SIPRINTF_H__
#define __SIPRINTF_H__

#include <stdarg.h>
#include <stddef.h>

typedef struct printdev
{
	void *internal;
	unsigned short limit;
	unsigned short offset;
	int (*outchar)(struct printdev *dev, char c);
}printdev_t;

int doiprintf(printdev_t *dev, const char *format, va_list args );
int siprintf(char *buf, size_t size, char *fmt, ...);
int string_outchar(printdev_t *dev, char c);

#endif /* __SIPRINTF_H__ */
