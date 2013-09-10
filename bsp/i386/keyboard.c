/*
 * keyboard.c
 */
#include <bsp.h>
#include <assert.h>

#include "i386_hw.h"
#include "i386_keyboard.h"

#define SHIFT	-1
#define CTRL	-2
#define META	-3

static char keymap[128][2] = {
	{0},				/* 0 */
	{27,	27},		/* 1 - ESC */
	{'1',	'!'},		/* 2 */
	{'2',	'@'},
	{'3',	'#'},
	{'4',	'$'},
	{'5',	'%'},
	{'6',	'^'},
	{'7',	'&'},
	{'8',	'*'},
	{'9',	'('},
	{'0',	')'},
	{'-',	'_'},
	{'=',	'+'},
	{8,	8},				/* 14 - Backspace */
	{'\t',	'\t'},		/* 15 */
	{'q',	'Q'},
	{'w',	'W'},
	{'e',	'E'},
	{'r',	'R'},
	{'t',	'T'},
	{'y',	'Y'},
	{'u',	'U'},
	{'i',	'I'},
	{'o',	'O'},
	{'p',	'P'},
	{'[',	'{'},
	{']',	'}'},		/* 27 */
	{'\r',	'\r'},		/* 28 - Enter */
	{CTRL,	CTRL},		/* 29 - Ctrl */
	{'a',	'A'},		/* 30 */
	{'s',	'S'},
	{'d',	'D'},
	{'f',	'F'},
	{'g',	'G'},
	{'h',	'H'},
	{'j',	'J'},
	{'k',	'K'},
	{'l',	'L'},
	{';',	':'},
	{'\'',	'"'},		/* 40 */
	{'`',	'~'},		/* 41 */
	{SHIFT,	SHIFT},		/* 42 - Left Shift */
	{'\\',	'|'},		/* 43 */
	{'z',	'Z'},		/* 44 */
	{'x',	'X'},
	{'c',	'C'},
	{'v',	'V'},
	{'b',	'B'},
	{'n',	'N'},
	{'m',	'M'},
	{',',	'<'},
	{'.',	'>'},
	{'/',	'?'},		/* 53 */
	{SHIFT,	SHIFT},		/* 54 - Right Shift */
	{0,	0},				/* 55 - Print Screen */
	{META,	META},		/* 56 - Alt */
	{' ',	' '},		/* 57 - Space bar */
	{0,	0},				/* 58 - Caps Lock */
	{0,	0},				/* 59 - F1 */
	{0,	0},				/* 60 - F2 */
	{0,	0},				/* 61 - F3 */
	{0,	0},				/* 62 - F4 */
	{0,	0},				/* 63 - F5 */
	{0,	0},				/* 64 - F6 */
	{0,	0},				/* 65 - F7 */
	{0,	0},				/* 66 - F8 */
	{0,	0},				/* 67 - F9 */
	{0,	0},				/* 68 - F10 */
	{0,	0},				/* 69 - Num Lock */
	{0,	0},				/* 70 - Scroll Lock */
	{74, 74},		/* 71 - 7 HOME       (KEYPAD)	*/
    {82, 82},		/* 72 - 8 UP         (KEYPAD)	*/
    {75, 75},		/* 73 - 9 PGUP       (KEYPAD)	*/
    {'-', '-'},		/* 74 - -            (KEYPAD)	*/
    {80, 80},		/* 75 - 4 LEFT       (KEYPAD)	*/
    {'5', '5'},		/* 76 - 5            (KEYPAD)	*/
    {79, 79},		/* 77 - 6 RIGHT      (KEYPAD)	*/
    {'+', '+'},		/* 78 - +            (KEYPAD)	*/
    {77, 77},		/* 79 - 1 END        (KEYPAD)	*/
    {81, 81},		/* 80 - 2 DOWN       (KEYPAD)	*/
	{78, 78},		/* 81 - 3 PGDN       (KEYPAD)	*/
    {73, 73},		/* 82 - 0 INSERT     (KEYPAD)	*/
    {76, 76},		/* 83 - . DEL        (KEYPAD)	*/
    {0x7e, 0x7e},		/* 84		SYSRQ	*/
    {0x00, 0x00},		/* 85	*/
    {0x69, 0x69},		/* 86 	NOTE : This key code was not defined in the original table! (< >)	*/
    {0x0c, 0x0c},		/* 87 - F11	*/
    {0x0d, 0x0d},		/* 88 - F12	*/
#if 0
	{'7',	'7'},		/* 71 - Numeric keypad 7 */
	{'8',	'8'},		/* 72 - Numeric keypad 8 */
	{'9',	'9'},		/* 73 - Numeric keypad 9 */
	{'-',	'-'},		/* 74 - Numeric keypad '-' */
	{'4',	'4'},		/* 75 - Numeric keypad 4 */
	{'5',	'5'},		/* 76 - Numeric keypad 5 */
	{'6',	'6'},		/* 77 - Numeric keypad 6 */
	{'+',	'+'},		/* 78 - Numeric keypad '+' */
	{'1',	'1'},		/* 79 - Numeric keypad 1 */
	{'2',	'2'},		/* 80 - Numeric keypad 2 */
	{'3',	'3'},		/* 81 - Numeric keypad 3 */
	{'0',	'0'},		/* 82 - Numeric keypad 0 */
	{'.',	'.'},		/* 83 - Numeric keypad '.' */
#endif
};

/*
 * Quick poll for a pending input character.
 * Returns a character if available, -1 otherwise.  This routine can return
 * false negatives in the following cases:
 *
 *	- a valid character is in transit from the keyboard when called
 *	- a key release is received (from a previous key press)
 *	- a SHIFT key press is received (shift state is recorded however)
 *	- a key press for a multi-character sequence is received
 *
 * Yes, this is horrible.
 */
int kbd_try_getchar(void)
{
	static unsigned shift_state, ctrl_state, meta_state;
	unsigned scan_code, ch;

//	base_critical_enter();

	/* See if a scan code is ready, returning if none. */
	if ((inp(K_STATUS) & K_OBUF_FUL) == 0) {
//		base_critical_leave();
		return -1;
	}
	scan_code = inp(K_RDWR);

	/* Handle key releases - only release of SHIFT is important. */
	if (scan_code & 0x80) {
		scan_code &= 0x7f;
		if (keymap[scan_code][0] == SHIFT)
			shift_state = 0;
		else if (keymap[scan_code][0] == CTRL)
			ctrl_state = 0;
		else if (keymap[scan_code][0] == META)
			meta_state = 0;
		ch = -1;
	} else {
		/* Translate the character through the keymap. */
		ch = keymap[scan_code][shift_state] | meta_state;
		if (ch == SHIFT) {
			shift_state = 1;
			ch = -1;
		} else if (ch == CTRL) {
			ctrl_state = 1;
			ch = -1;
		} else if (ch == META) {
			meta_state = 0200;
			ch = -1;
		} else if (ch == 0)
			ch = -1;
		else if (ctrl_state)
			ch = (keymap[scan_code][1] - '@') | meta_state;
	}

//	base_critical_leave();

	return ch;
}

/*
 * delay for a while
 *   i: delay time
 */
void kbd_delay(int i)
{
	while (--i >= 0)
		;
}

/*
 * wait while keyboard busy
 *   i: wait time
 */
void kbd_wait_while_busy(int i)
{
    while (i--) {
        if ((inp(K_STATUS) & K_IBUF_FUL) == 0)
            break;
        kbd_delay(KBD_DELAY);
    }
}

/*
 * write keyboard command
 *   cb: command
 */
void kbd_write_cmd_byte(int cb)
{
	kbd_wait_while_busy(1000);
	outp(K_CMD, KC_CMD_WRITE);	
	kbd_wait_while_busy(1000);
	outp(K_RDWR, cb);
}

/*
 * init keyboard interrupt
 */
int keyboard_init()
{
	int c, i, retries = 10;

	/* Write the command byte to make sure that IRQ generation is off. */
	kbd_write_cmd_byte((K_CB_SCAN|K_CB_INHBOVR|K_CB_SETSYSF));

	/* empty keyboard buffer. */
	while (inp(K_STATUS) & K_OBUF_FUL) {
		kbd_delay(KBD_DELAY);
		(void) inp(K_RDWR);
	}

	/* reset keyboard */
	while (retries--) {
		kbd_wait_while_busy(1000);
		outp(K_RDWR, KC_CMD_PULSE);
		i = 10000;
		while (!(inp(K_STATUS) & K_OBUF_FUL) && (--i > 0)) 
			kbd_delay(KBD_DELAY);
		if ((c = inp(K_RDWR)) == K_RET_ACK)
			break;
	}

	if (retries > 0) {	
		retries = 10;

		while (retries--) {
			/* Need to wait a long time... */
			i = 10000;
			while (!(inp(K_STATUS) & K_OBUF_FUL) && (--i > 0)) 
				kbd_delay(KBD_DELAY * 10);
			if ((c = inp(K_RDWR)) == K_RET_RESET_DONE)
				break;
		}
	}

	/* XXX should we do something more drastic? (like panic?) */
	assert(retries);

	/* Now turn on IRQ generation. */
	kbd_write_cmd_byte((K_CB_SCAN|K_CB_INHBOVR|K_CB_SETSYSF|K_CB_ENBLIRQ));

	return 1;
}
