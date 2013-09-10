# Makefile for lwip
DOOLOO_PATH = ../../..
include $(DOOLOO_PATH)/config.mk

LWIPDIR=$(DOOLOO_PATH)/net/lwip/src
LWIPDIR=.
ARFLAGS=rs
LWIPARCH=dooloo

CFLAGS:=$(CFLAGS) -DIPv4 -DLWIP_DEBUG \
	-I$(LWIPDIR)/include -I$(LWIPDIR)/include/ipv4 -I$(LWIPDIR)/arch/$(LWIPARCH)/include -I.

# COREFILES, CORE4FILES: The minimum set of files needed for lwIP.
COREFILES=$(LWIPDIR)/core/mem.c $(LWIPDIR)/core/memp.c $(LWIPDIR)/core/netif.c \
	$(LWIPDIR)/core/pbuf.c $(LWIPDIR)/core/raw.c $(LWIPDIR)/core/stats.c \
	$(LWIPDIR)/core/sys.c $(LWIPDIR)/core/tcp.c $(LWIPDIR)/core/tcp_in.c \
	$(LWIPDIR)/core/tcp_out.c $(LWIPDIR)/core/udp.c $(LWIPDIR)/core/dhcp.c \
	$(LWIPDIR)/core/init.c $(LWIPDIR)/core/timers.c $(LWIPDIR)/core/def.c
CORE4FILES=$(LWIPDIR)/core/ipv4/icmp.c $(LWIPDIR)/core/ipv4/ip.c \
	$(LWIPDIR)/core/ipv4/inet.c $(LWIPDIR)/core/ipv4/ip_addr.c \
	$(LWIPDIR)/core/ipv4/inet_chksum.c $(LWIPDIR)/core/ipv4/ip_frag.c \
	$(LWIPDIR)/core/ipv4/igmp.c

# SNMPFILES: Extra SNMPv1 agent
SNMPFILES=$(LWIPDIR)/core/snmp/asn1_dec.c $(LWIPDIR)/core/snmp/asn1_enc.c \
	$(LWIPDIR)/core/snmp/mib2.c $(LWIPDIR)/core/snmp/mib_structs.c \
	$(LWIPDIR)/core/snmp/msg_in.c $(LWIPDIR)/core/snmp/msg_out.c

# APIFILES: The files which implement the sequential and socket APIs.
APIFILES=$(LWIPDIR)/api/api_lib.c $(LWIPDIR)/api/api_msg.c $(LWIPDIR)/api/tcpip.c \
	$(LWIPDIR)/api/err.c $(LWIPDIR)/api/sockets.c $(LWIPDIR)/api/netbuf.c $(LWIPDIR)/api/netdb.c

# NETIFFILES: Files implementing various generic network interface functions.'
NETIFFILES=$(LWIPDIR)/netif/etharp.c $(LWIPDIR)/netif/slipif.c

# NETIFFILES: Add PPP netif
NETIFFILES+=$(LWIPDIR)/netif/ppp/auth.c $(LWIPDIR)/netif/ppp/chap.c \
	$(LWIPDIR)/netif/ppp/chpms.c $(LWIPDIR)/netif/ppp/fsm.c \
	$(LWIPDIR)/netif/ppp/ipcp.c $(LWIPDIR)/netif/ppp/lcp.c \
	$(LWIPDIR)/netif/ppp/magic.c $(LWIPDIR)/netif/ppp/md5.c \
	$(LWIPDIR)/netif/ppp/pap.c $(LWIPDIR)/netif/ppp/ppp.c \
	$(LWIPDIR)/netif/ppp/randm.c $(LWIPDIR)/netif/ppp/vj.c \
	arch/$(LWIPARCH)/netif/sio.c

# ARCHFILES: Archiecture specific files.
ARCHFILES=$(LWIPDIR)/arch/$(LWIPARCH)/sys_arch.c \
	$(LWIPDIR)/arch/$(LWIPARCH)/sys_arch_init.c

#ARCHFILES=$(wildcard $(LWIPARCH)/*.c $(LWIPARCH)/netif/tapif.c $(LWIPARCH)/netif/tunif.c $(LWIPARCH)/netif/unixif.c $(LWIPARCH)/netif/list.c $(LWIPARCH)/netif/tcpdump.c)

# APPFILES: Applications.
APPFILES=

# LWIPFILES: All the above.
LWIPFILES=$(COREFILES) $(CORE4FILES) $(SNMPFILES) $(APIFILES) $(NETIFFILES) $(ARCHFILES)
LWIPFILESW=$(wildcard $(LWIPFILES))
LWIPOBJS=$(notdir $(LWIPFILESW:.c=.o))

LWIPLIB=$(LIB_PATH)/liblwip4.a
APPLIB=$(LIB_PATH)/liblwipapps.a
APPOBJS=$(notdir $(APPFILES:.c=.o))

%.o:
	$(CC) $(CFLAGS) -c $(<:.o=.c)

all: .depend $(LWIPLIB) $(APPLIB)
.PHONY: all

clean:
	$(RM) *.o $(LWIPLIB) $(APPLIB) .depend*

depend dep: .depend

include .depend

$(APPLIB): $(APPOBJS)
	$(AR) $(ARFLAGS) $(APPLIB) $?

$(LWIPLIB): $(LWIPOBJS)
	$(AR) $(ARFLAGS) $(LWIPLIB) $?

.depend: $(LWIPFILES) $(APPFILES)
	$(CC) $(CFLAGS) -MM $^ > .depend

