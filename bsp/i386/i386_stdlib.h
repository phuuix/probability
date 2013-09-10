/* 
 * i386_stdlib.h
 */

#ifndef __D_I386_STDLIB_H__
#define __D_I386_STDLIB_H__

/* Flag bit settings */
#define RESPECT_WIDTH	1  /* Fixed width wanted 	*/
#define ADD_PLUS	2  /* Add + for positive/floats */
#define SPACE_PAD	4  /* Padding possibility	*/
#define ZERO_PAD	8
#define LEFT_PAD 	16

/* String conversion functions  	*/

long strtoi(char *s,int base,char **scan_end);
unsigned long strtou(char *s,int base,char **scan_end);
double strtod(char *s,char **scan_end);
long strtol(const char *nptr, char **endptr, int base);
unsigned long strtoul(const char *nptr, char **endptr, int base);


unsigned ecvt(double v,char *buffer,int width,int prec,int flag);
unsigned fcvt(double v,char *buffer,int width,int prec,int flag);
unsigned gcvt(double v,char *buffer,int width,int prec,int flag);
unsigned dcvt(long v,char *buffer,int base,int width,int flag);
unsigned ucvt(unsigned long v,char *buffer,int base,int width,int flag);


/* StdLib Macro */

#define atof(s)	strtod(s, NULL);
#define atoi(s)	strtoi(s, 10, NULL);
#define atou(s)	strtou(s, 10, NULL);
#define atol(s) strtol(s, 10, NULL);

/* Generic utility functions 	*/

void srand(long int seed);
long int rand(void);
unsigned abs(int x);


#if !defined(__max)
#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#endif
#if !defined(max) && !defined(__cplusplus)
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif
#if !defined(__min)
#define __min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#if !defined(min) && !defined(__cplusplus)
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif

#endif /* __D_I386_STDLIB_H__ */
