/*
 * tty_pc_drv.c
 * pc (keyboard,vga) driver for tty
 */

#include <bsp.h>
#include <tty.h>
#include <ipc.h>
#include <irq.h>
#include <isr.h>

#include "i386_cons.h"
#include "i386_keyboard.h"
#include "i386_data.h"


extern struct tty_ldisc sys_tty_ldisc_default;

static int ttypc_driver_open(struct tty_struct *tty);
static void ttypc_driver_close(struct tty_struct *tty);
static ssize_t ttypc_driver_read(struct tty_struct *tty, unsigned char *buf, size_t nr);
static ssize_t ttypc_driver_write(struct tty_struct *tty, unsigned char *buf, size_t nr);
static int ttypc_driver_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg);

struct tty_driver tty_pc_driver = {
	ttypc_driver_open,
	ttypc_driver_close,
	ttypc_driver_read,
	ttypc_driver_write,
	ttypc_driver_ioctl
};

static int ttypc_driver_open(struct tty_struct *tty)
{
	return 0;
}

static void ttypc_driver_close(struct tty_struct *tty)
{
}

static ssize_t ttypc_driver_read(struct tty_struct *tty, unsigned char *buf, size_t nr)
{
	return nr;
}

static ssize_t ttypc_driver_write(struct tty_struct *tty, unsigned char *buf, size_t nr)
{
	int i;

	for(i=0; i<nr; i++)
	{
		cputc(buf[i]);
	}

	return nr;
}

static int ttypc_driver_ioctl(struct tty_struct *tty, unsigned int cmd, unsigned long arg)
{
	return 0;
}

#define STACKTESET

static void isr_keyboard(int n)
{
	char c;
	struct tty_evmail kbdmail;

	c = kbd_try_getchar();
	if(c != -1)
	{
		kbdmail.tty = 0;
		kbdmail.len = 1;
		kbdmail.data[0] = c;
#ifdef STACKTESET //test for interrupt stack
		kprintf("1: current task: %d esp=%d\n", current-systask, get_BP());
#endif
		tty_post_mail(&kbdmail);
#ifdef STACKTESET //test for interrupt stack
		kprintf("2: current task: %d esp=%d\n", current-systask, get_BP());
#endif

	}
}

/*
 * init PC tty
 * this function must run after tty_init()
 */
void tty_pc_init()
{
	bsp_isr_attach(KEYB_IRQ, isr_keyboard);
	bsp_irq_unmask(KEYB_IRQ);

	if(tty_register("ttypc", &sys_tty_ldisc_default, &tty_pc_driver) == 0)
		kprintf("tty 'ttypc' register failed\n");

	if(tty_fw_open("ttypc", 0) == NULL)
	{
		kprintf("open tty 'ttypc' failed\n");
		kprintf("tty 0 name: %s\n", systty[0].name);
	}
}


