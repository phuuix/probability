/*
 * i386_cons.h
 */

/*	Console output functions	*/

#ifndef __D_I386_CONS_H__
#define __D_I386_CONS_H__

/* CGA compatible registers value */

#define CURSOR_START		0x0A
#define CURSOR_END		0x0B
#define VIDEO_ADDRESS_MSB	0x0C
#define VIDEO_ADDRESS_LSB	0x0D
#define CURSOR_POS_MSB		0x0E
#define CURSOR_POS_LSB		0x0F

/* CGA compatible registers */

#define CGA_INDEX_REG	0x3D4
#define CGA_DATA_REG	0x3D5

void cursor(int start,int end);
void _clear(char c,char attr,int x1,int y1,int x2,int y2);
void clear(void);
void _scroll(char attr,int x1,int y1,int x2,int y2);
void scroll(void);
void cputc(char c);
void cputs(char *s);
void cputc_xy(int x,int y,char attr,char c);
char cgetc_xy(int x,int y,char *attr,char *c);
void cputs_xy(int x,int y,char attr,char *s);


/* Text mode color definitions */

#define BLACK		0
#define BLUE		1
#define GREEN		2
#define CYAN		3
#define RED		4
#define MAGENTA		5
#define BROWN		6
#define LIGHTGRAY	7
#define DARKGRAY	8
#define LIGHTBLUE	9
#define LIGHTGREEN	10
#define LIGHTCYAN	11
#define LIGHTRED	12
#define LIGHTMAGENTA	13
#define YELLOW 		14
#define WHITE		15


#endif /* __D_I386_CONS_H__ */

