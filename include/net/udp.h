/*
 * udp.h
 * Modified from Linux.
 */
#ifndef __D_UDP_H__
#define __D_UDP_H__


struct udphdr {
	__u16	source;
	__u16	dest;
	__u16	len;
	__u16	check;
};


#endif	/* __D_UDP_H__ */
