/* 
 * i386_cons.c
 */
#include <bsp.h>
#include "i386_cons.h"
#include "i386_hw.h"

#define SAFE_CPUTC

typedef void *LIN_ADDR;

/* Store bios settings */
static unsigned char bios_x=0,bios_y=0;


static void place(int x,int y)
{
    unsigned short cursor_word = x + y*80;
    /* Set cursor position				  */
    /* CGA is programmed writing first the Index register */
    /* to specify what internal register we are accessing */
    /* Then we load the Data register with the wanted val */
	outp(CGA_INDEX_REG,CURSOR_POS_MSB);
    outp(CGA_DATA_REG,(cursor_word >> 8) & 0xFF);
    outp(CGA_INDEX_REG,CURSOR_POS_LSB);
    outp(CGA_DATA_REG,cursor_word & 0xFF);
    
    /* Adjust temporary cursor bios position */
    bios_x = x;
    bios_y = y;
}

void cursor(int start,int end)
{
    /* Same thing as above; Set cursor scan line */
    outp(CGA_INDEX_REG,CURSOR_START);
    outp(CGA_DATA_REG,start);
    outp(CGA_INDEX_REG,CURSOR_END);
    outp(CGA_DATA_REG,end);
}


void _clear(char c,char attr,int x1,int y1,int x2,int y2)
{
    register int i,j;
    WORD w = attr;
    w <<= 8; w |= c;
    for (i = x1; i <= x2; i++)
    	for (j = y1; j <= y2; j++) 
            lmempokew((LIN_ADDR)(0xB8000 + 2*i+160*j),w);
    place(x1,y1);
    bios_y = y1;
    bios_x = x1;
}

void clear()
{
//    _clear(' ',BLACK,0,0,79,24);
	_clear(' ',WHITE,0,0,79,24);
}

void _scroll(char attr,int x1,int y1,int x2,int y2)
{
    register int x,y;
    WORD xattr = attr << 8,w;
    LIN_ADDR v = (LIN_ADDR)(0xB8000);
      
    for (y = y1+1; y <= y2; y++)
       for (x = x1; x <= x2; x++) {
	   w = lmempeekw((LIN_ADDR)(v + 2*(y*80+x)));
	   lmempokew((LIN_ADDR)(v + 2*((y-1)*80+x)),w);
    }
    for (x = x1; x <= x2; x++)
   	lmempokew((LIN_ADDR)(v + 2*((y-1)*80+x)),xattr|' ');
}

void scroll(void)
{
    _scroll(WHITE,0,0,79,24);
}

void cputc(char c)
{
    unsigned short scan_x,x,y;
    LIN_ADDR v = (LIN_ADDR)(0xB8000);

#ifdef SAFE_CPUTC
	unsigned long f;
	f = bsp_fsave();
#endif
    x = bios_x;
    y = bios_y;
    switch (c) {
	case '\t' : x += 8;		
    	if (x >= 80) {
			x = 0;
			if (y == 24) scroll();
			else y++;
    		    } else {
			scan_x = 0;
			while ((scan_x+8) < x) scan_x += 8;
			x = scan_x;
		}
		break;
	case '\n' : x = 0;
		if (y == 24) scroll();
		else y++;
		break;
	case '\b' : 
		if(x>0)
		{
			x--;
			lmempokew((LIN_ADDR)(v + 2*(x + y*80)),0x0f00|' ');
		}
		break;
	default   : 
		lmempokew((LIN_ADDR)(v + 2*(x + y*80)),0x0700|c);
		x++;
		if (x > 80) {
			x = 0;
			if (y == 24) scroll();
			else y++;
	    }
    }
    place(x,y);
#ifdef SAFE_CPUTC
	bsp_frestore(f);
#endif
}

void cputs(char *s)
{
    char c;
    while (*s != '\0') {
	c = *s++;
	cputc(c);
    }
}

void cputs_xy(int x,int y,char attr,char *s)
{
    LIN_ADDR v = (LIN_ADDR)(0xB8000 + (80*y+x)*2);
    while (*s != 0) {
	/* REMEMBER! This is a macro! v++ is out to prevent side-effects */
	lmempokeb(v,*s); s++; v++;
	lmempokeb(v,attr); v++;
    }
}

void cputc_xy(int x,int y,char attr,char c)
{
    LIN_ADDR v = (LIN_ADDR)(0xB8000 + (80*y+x)*2);
    /* REMEMBER! This is a macro! v++ is out to prevent side-effects */
    lmempokeb(v,c); v++;
    lmempokeb(v,attr);
}

char cgetc_xy(int x,int y,char *attr,char *c)
{
    LIN_ADDR v = (LIN_ADDR)(0xB8000 + (80*y+x)*2);
    char r;
    r = lmempeekb(v); v++;
    if (c != NULL) *c = r;
    r = lmempeekb(v);
    if (attr != NULL) *attr = r;
    return(r);
}
