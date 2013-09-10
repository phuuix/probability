/* 
 * i386_limits.h
 */

/*	Some POSIX compliance stuff	*/

#ifndef __D_I386_LIMITS_H__
#define __D_I386_LIMITS_H__

/* Number limits */

#define CHAR_BIT	8
#define CHAR_MAX	255
#define CHAR_MIN	0
#define SCHAR_MAX	127
#define SCHAR_MIN	-128

#define INT_MAX    	2147483647
#define INT_MIN		(-INT_MAX - 1)
#define UINT_MAX  	4294967295U

#define LONG_MAX	2147483647L
#define LONG_MIN	(-LONG_MAX - 1)
#define ULONG_MAX	4294967295UL

#endif /* __D_I386_LIMITS_H__ */
