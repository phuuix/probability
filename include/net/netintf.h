/*
 * netintf.h
 */
#ifndef __D_NETINTF_H__
#define __D_NETINTF_H__

/*
 * for ioctl
 */

#define NIOCTL_SFLAGS		0x01
#define NIOCTL_GFLAGS		0x02
#define NIOCTL_SADDR		0x04
#define NIOCTL_GADDR		0x08
#define NIOCTL_POLLSTART	0x10
#define NIOCTL_POLLSTOP		0x20

struct ifnet
{
	void *data;
	void (*if_init)();
	void (*if_cleanup)();
	int (*if_open)();
	int (*if_close)();
	int (*if_ioctl)();
	int (*if_recv)();
	int (*if_send)();
	int (*if_poll_recv)();
	int (*if_poll_send)();
};

#endif /* __D_NETINTF_H__ */
