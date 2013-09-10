#ifndef _RTL8139_H_
#define _RTL8139_H_
/*
 * rtl8139 driver for Dooloo
 * ffxz
 * 2002-8-17
 */
#define FLG_POLL	0x01

void rtl8139_init();
int  rtl8139_ioctl(int d, int request, void* p);
int  rtl8139_poll_start();
int  rtl8139_poll_stop();
int  rtl8139_poll_recv(int d, char *buf, size_t count);
int  rtl8139_poll_send(int d, const char *buf, size_t count);

void rtl8139_cleanup();
int rtl8139_open(const char *pathname, int flags);
int rtl8139_close(int d);
int rtl8139_recv(int d, void *buf, size_t count);
int rtl8139_send(int d, const void *buf, size_t count);

#endif
