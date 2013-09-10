
/*
 * i386_string.h
 */

#ifndef __D_I386_STRING_H__
#define __D_I386_STRING_H__

char *strcpy(char *dst,const char *src);
char *strncpy(char *dst,const char *src,int n);
int strcmp(const char *s1,const char *s2);
int strncmp(const char *s1,const char *s2,int n);
int strlen(const char *s);
char *strscn(char *s,char *pattern);
char *strchr(char *s,int c);
char *strrchr(const char *s, int c);
char *strupr(char *s);
char *strlwr(char *s);
char *strcat(char *dst,char *src);
char *strncat(char *dest, const char *src, int n);
char *strstr(const char *haystack, const char *needle);

#endif /* __D_I386_STRING_H__ */
