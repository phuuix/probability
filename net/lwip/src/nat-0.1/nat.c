/*
 * Copyright (c) 2003 EISLAB, Lulea University of Technology.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the NAT contribution for lwIP.
 * 
 * Author: Conny Ohult <conny@sm.luth.se>
 *
 */

/*-----------------------------------------------------------------------------------*/
/* 
 * nat.c  
 *
 *  
 */
/*-----------------------------------------------------------------------------------*/

#include "natopts.h"
#include "nat_memp.h"
#include "nat.h"

#include "lwip/inet.h"
#include "lwip/ip.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/icmp.h"

#include "lwip/debug.h"

struct nat_pcb *nat_active_pcbs; /* List of all active NAT PCBs */
struct nat_pcb *nat_tmp_pcb; /* Only used for temporary storage. */

u16_t port_nxt; /* Next unique transport identifier to allocate */
u32_t ip_nxt; /* Next private IP address to allocate for a remote host */

struct nat_ctrl {
  struct netif *nat_external_netif;
  err_t (* tcpip_input)(struct pbuf *p, struct netif *inp); /* Used to pass a packet up to the 
							       TCP/IP stack */
} nat_ctrl;


/*-----------------------------------------------------------------------------------*/
/* 
 * nat_init():
 *
 * Initializes the NAT layer. Takes the lwIP external network interface and a 
 * function used to pass a packet up to lwIP as arguments.
 *
 */
/*-----------------------------------------------------------------------------------*/
void 
nat_init(struct netif *external_netif, err_t (* tcpip_input)(struct pbuf *p, struct netif *inp))
{
  /* Clear globals */
  nat_active_pcbs = NULL;
  nat_tmp_pcb = NULL;

  /* Initialize the IP address pool */
  ip_nxt = NAT_MIN_IP;

  /* Initialize the unique port number */
  port_nxt = NAT_MIN_PORT;

  /* Set network interface to the external network */
  nat_ctrl.nat_external_netif = external_netif;

  /* Set lwIP input function to be called when a packet arrives from a network interface */
  nat_ctrl.tcpip_input = tcpip_input;
}
/*-----------------------------------------------------------------------------------*/
/* 
 * nat_tmr():
 *
 * Called every 1s and updates the idle timer attached to each session.
 * 
 */
/*-----------------------------------------------------------------------------------*/
void
nat_tmr(void) 
{
  struct nat_pcb *pcb;
  
  /* Step through all of the active pcbs */
  for(pcb = nat_active_pcbs; pcb != NULL; pcb = pcb->next) {
    ++pcb->tmr;
#if NAT_IDLE_TIMEOUT
    if(pcb->tmr >= NAT_IDLE_TIMEOUT) {
      LWIP_DEBUGF(NAT_DEBUG, ("nat_tmr: Idle timeout for session has expired and will be removed\n"));
      nat_close(pcb);
    }
#endif
#if NAT_TCP_END_DETECT
    if(pcb->state == NAT_CLOSING) {
      ++pcb->term_tmr;
      if(pcb->term_tmr >= 240) {
	LWIP_DEBUGF(NAT_DEBUG, ("nat_tmr: Session can be assumed to have been terminated\n"));
	nat_close(pcb);
      }
    }
#endif
  }
}
/*-----------------------------------------------------------------------------------*/
/* 
 * nat_next_port():
 *
 * Return a port number that can be used to form a globally unique address.
 * 
 */
/*-----------------------------------------------------------------------------------*/
u16_t
nat_next_port(void)
{
  if(port_nxt == NAT_MAX_PORT) {
    return (port_nxt = NAT_MIN_PORT);
  }
  return ++port_nxt; 
}
/*-----------------------------------------------------------------------------------*/
/* 
 * nat_new():
 *
 * 
 * 
 */
/*-----------------------------------------------------------------------------------*/
struct nat_pcb *
nat_new(struct ip_addr *ipaddr, u16_t port)
{
  struct nat_pcb *pcb, *oldest;

  pcb = nat_memp_malloc(MEMP_NAT_PCB);
  if(pcb == NULL) {
    /* Out of PCBs */
#if NAT_RETIRE_LONGEST_IDLE
    /* Retire the session that has been idle the longest time */
    LWIP_DEBUGF(NAT_DEBUG, ("nat_process: Out of PCBs. Remove the session that has been idle the longest time\n"));
    oldest = nat_active_pcbs;
    for(pcb = nat_active_pcbs->next; pcb->next != NULL; pcb = pcb->next) {
      if(pcb->tmr > oldest->tmr) {
	oldest = pcb;
      }
    }
    NAT_RMV(&(nat_active_pcbs), oldest);
    pcb = oldest;
#else
    return NULL;
#endif
  }
  pcb->local_ip.addr = ipaddr->addr;
  pcb->local_port = port;
  pcb->ass_port = htons(nat_next_port());
  pcb->tmr = 0;
#if NAT_TCP_END_DETECT
  pcb->state = NAT_OPEN;
  pcb->term_tmr = 0;
#endif
  NAT_REG(&nat_active_pcbs, pcb);

  return pcb;
}
/*-----------------------------------------------------------------------------------*/
/* 
 * nat_close():
 *
 * 
 * 
 */
/*-----------------------------------------------------------------------------------*/
void
nat_close(struct nat_pcb *pcb)
{
  if(pcb != NULL) {
    /* Remove the NAT PCB that contain the PPP PCB from the list of active connections */
    NAT_RMV(&(nat_active_pcbs), pcb);
    nat_memp_free(MEMP_NAT_PCB, pcb);
  }
}
/*-----------------------------------------------------------------------------------*/
/* 
 * nat_chksum_adjust():
 *
 * 
 * 
 */
/*-----------------------------------------------------------------------------------*/
void 
nat_chksum_adjust(u8_t *chksum, u8_t *optr, s16_t olen, u8_t *nptr, s16_t nlen)
   /* assuming: unsigned char is 8 bits, long is 32 bits.
     - chksum points to the chksum in the packet
     - optr points to the old data in the packet
     - nptr points to the new data in the packet
   */
{
  s32_t x, old, new;
  x=chksum[0]*256+chksum[1];
  x=~x & 0xFFFF;
  while (olen) {
    old=optr[0]*256+optr[1]; optr+=2;
    x-=old & 0xffff;
    if (x<=0) {
      x--; x&=0xffff; 
    }
    olen-=2;
  }
  while (nlen) {
    new=nptr[0]*256+nptr[1]; nptr+=2;
    x+=new & 0xffff;
    if (x & 0x10000) { 
      x++; x&=0xffff; 
    }
    nlen-=2;
  }
  x=~x & 0xFFFF;
  chksum[0]=x/256; chksum[1]=x & 0xff;
  //LWIP_DEBUGF(NAT_DEBUG, ("nat_chksum_adjust: chksum = 0x%x\n", *((u16_t *)chksum)));
}
/*-----------------------------------------------------------------------------------*/
/* 
 * nat_input():
 *
 * Called by the network interface to pass a packet up to the TCP/IP stack
 * 
 */
/*-----------------------------------------------------------------------------------*/
err_t 
nat_input(struct pbuf *p, struct netif *netif)
{
  struct ip_hdr *iphdr = p->payload;
  struct tcp_hdr *tcphdr = NULL;
  struct udp_hdr *udphdr = NULL;
  struct icmp_echo_hdr *icmphdr = NULL;
  u32_t oaddr;
  u16_t dest, src;
  struct nat_pcb *pcb = NULL;
  err_t ret = ERR_OK;
  static u16_t iphdrlen;

  LWIP_DEBUGF(NAT_DEBUG, ("nat_input: iphdr->src %d.%d.%d.%d\n", (u8_t)(ntohl(iphdr->src.addr) >> 24) & 0xff, (u8_t)(ntohl(iphdr->src.addr) >> 16) & 0xff, (u8_t)(ntohl(iphdr->src.addr) >> 8) & 0xff, (u8_t)ntohl(iphdr->src.addr) & 0xff));
  LWIP_DEBUGF(NAT_DEBUG, ("nat_input: iphdr->dest %d.%d.%d.%d\n", (u8_t)(ntohl(iphdr->dest.addr) >> 24) & 0xff, (u8_t)(ntohl(iphdr->dest.addr) >> 16) & 0xff, (u8_t)(ntohl(iphdr->dest.addr) >> 8) & 0xff, (u8_t)ntohl(iphdr->dest.addr) & 0xff));

  /* Obtain IP header length in number of 32-bit words */
  iphdrlen = IPH_HL(iphdr);
  /* Calculate IP header length in bytes */
  iphdrlen *= 4;

  switch(IPH_PROTO(iphdr)) {
  case IP_PROTO_TCP:
    tcphdr = (struct tcp_hdr *)(((u8_t *)p->payload) + iphdrlen);
    dest = tcphdr->dest;
    src = tcphdr->src;
    break;
  case IP_PROTO_UDP:
    udphdr = (struct udp_hdr *)(((u8_t *)p->payload) + iphdrlen);
    dest = udphdr->dest;
    src = udphdr->src;
    break;
  case IP_PROTO_ICMP:
    icmphdr = (struct icmp_echo_hdr *)(((u8_t *)p->payload) + iphdrlen);
    src = dest = icmphdr->id;
    break;
  default:
    //TODO:
    break;
  }

  if(ntohs(dest) >= NAT_MIN_PORT && ntohs(dest) <= NAT_MAX_PORT) {
    for(pcb = nat_active_pcbs; pcb != NULL; pcb = pcb->next) {
      if(dest == pcb->ass_port) {
	break; /* Found matching connection */
      }
    }
  }
  if(pcb != NULL) {
    /* IP packet is for translated local connection */
    LWIP_DEBUGF(NAT_DEBUG, ("nat_input: IP packet is for translated local connection\n"));

    pcb->tmr = 0;/* Reset idle timer */

    /* Calculate new checksum */
    oaddr = iphdr->dest.addr;
    ((struct ip_addr *)&(iphdr->dest))->addr = pcb->local_ip.addr; 
    nat_chksum_adjust((u8_t *)&IPH_CHKSUM(iphdr),(u8_t *)&oaddr, 4, (u8_t *)&iphdr->dest.addr, 4);

    if(IPH_PROTO(iphdr) == IP_PROTO_TCP) {
      nat_chksum_adjust((u8_t *)&tcphdr->chksum,(u8_t *)&oaddr, 4, (u8_t *)&iphdr->dest.addr, 4);
      tcphdr->dest = pcb->local_port;
      nat_chksum_adjust((u8_t *)&tcphdr->chksum,(u8_t *)&dest, 2, (u8_t *)&tcphdr->dest, 2);
    } else if(IPH_PROTO(iphdr) == IP_PROTO_UDP) {
      if(udphdr->chksum != 0) {
	nat_chksum_adjust((u8_t *)&udphdr->chksum,(u8_t *)&oaddr, 4, (u8_t *)&iphdr->dest.addr, 4);
	udphdr->dest = pcb->local_port;
	nat_chksum_adjust((u8_t *)&udphdr->chksum,(u8_t *)&dest, 2, (u8_t *)&udphdr->dest, 2);
      }
    } else if(IPH_PROTO(iphdr) == IP_PROTO_ICMP && (ICMPH_CODE(icmphdr) == ICMP_ECHO || ICMPH_CODE(icmphdr) == ICMP_ER)) {
      icmphdr->id = pcb->local_port;
      nat_chksum_adjust((u8_t *)&icmphdr->chksum,(u8_t *)&dest, 2, (u8_t *)&icmphdr->id, 2);
    } else {
      return ERR_RTE;
    }   
  } else {
    /* IP packet is for AP network connection or the local TCP/IP stack */
    LWIP_DEBUGF(NAT_DEBUG, ("nat_input: IP packet is for AP network connection or the local TCP/IP stack\n"));
    for(pcb = nat_active_pcbs; pcb != NULL; pcb = pcb->next) {
      if(ip_addr_cmp(&pcb->local_ip, &iphdr->src) && pcb->local_port == src) {
	break;
      }
    }
    if(pcb == NULL) {
      /* A new session should be created if the packet came from the private network and is destined for 
	 the external network */
      if(iphdr->dest.addr != NAT_LOCAL_IP) {
	if(iphdr->src.addr >= NAT_MIN_IP && iphdr->src.addr < ip_nxt) {
	  LWIP_DEBUGF(NAT_DEBUG, ("nat_input: Create a new session\n"));
	  pcb = nat_new(&iphdr->src, src);
	}
      }
    }
    if(pcb != NULL) {
      /* IP packet is for AP network connection */
      LWIP_DEBUGF(NAT_DEBUG, ("nat_input: IP packet is for AP network connection\n"));

      if(nat_ctrl.nat_external_netif == NULL) {
	pbuf_free(p);
	return ret;
      }

      pcb->tmr = 0; /* Reset idle timer */

      /* Calculate new checksum */
      oaddr = iphdr->src.addr;
      ((struct ip_addr *)&(iphdr->src))->addr = nat_ctrl.nat_external_netif->ip_addr.addr;
      nat_chksum_adjust((u8_t *)&IPH_CHKSUM(iphdr),(u8_t *)&oaddr, 4, (u8_t *)&iphdr->src.addr, 4);
      
      switch(IPH_PROTO(iphdr)) {
      case IP_PROTO_TCP:
	nat_chksum_adjust((u8_t *)&tcphdr->chksum,(u8_t *)&oaddr, 4, (u8_t *)&iphdr->src.addr, 4);
	tcphdr->src = pcb->ass_port;
	nat_chksum_adjust((u8_t *)&tcphdr->chksum,(u8_t *)&src, 2, (u8_t *)&tcphdr->src, 2);
	break;
      case IP_PROTO_UDP:
	if(udphdr->chksum != 0) {
	  nat_chksum_adjust((u8_t *)&udphdr->chksum,(u8_t *)&oaddr, 4, (u8_t *)&iphdr->src.addr, 4);
	  udphdr->src = pcb->ass_port;
	  nat_chksum_adjust((u8_t *)&udphdr->chksum,(u8_t *)&src, 2, (u8_t *)&udphdr->src, 2);
	}
	break;
      case IP_PROTO_ICMP: //TODO: NOT AS ABOVE?
	icmphdr->id = pcb->ass_port;
	nat_chksum_adjust((u8_t *)&icmphdr->chksum,(u8_t *)&src, 2, (u8_t *)&icmphdr->id, 2);
	break;
      default:
	return ERR_RTE;
	break;
      }
    }
  }
  
  
#if NAT_TCP_END_DETECT
  if(pcb != NULL) {
    if(IPH_PROTO(iphdr) == IP_PROTO_TCP) {
      if(TCPH_FLAGS(tcphdr) & TCP_RST) {
	pcb->state = NAT_CLOSING;
      } else if(TCPH_FLAGS(tcphdr) & TCP_FIN) {
	if(pcb->state == NAT_OPEN) {
	  pcb->state = NAT_FIN;
	} else {
	  pcb->state = NAT_CLOSING;
	}
      }
    }
  }
#endif
   LWIP_DEBUGF(NAT_DEBUG, ("nat_input: NAT_EVENT_INPUT\n")); 
    NAT_EVENT_INPUT(&nat_ctrl, p, netif, ret);
 
  return ret;
}
/*-----------------------------------------------------------------------------------*/
/* 
 * nat_netif_add():
 *
 * Add a network interface to a list of lwIP netifs. The network interface will be 
 * part of a private network that access the external network through a single shared 
 * network interface.
 *  
 */
/*-----------------------------------------------------------------------------------*/
struct netif *
nat_netif_add(void* state, err_t (* init)(struct netif *netif))
{
  struct ip_addr ipaddr, netmask, gw;

  /* Get a private IP address that must be used by the host in the private network */
  if(ip_nxt > NAT_MAX_IP) {
    ip_nxt = NAT_MIN_IP;
  }

  /* Avoid setting a private IP address that matches the external interface IP address */
  if(nat_ctrl.nat_external_netif != NULL) {
    if(nat_ctrl.nat_external_netif->ip_addr.addr == ip_nxt) {
      /* Set next private address to be used */
      ip_nxt = ntohl(ip_nxt);
      ++ip_nxt;			     
      ip_nxt = htonl(ip_nxt);
    }
  }

  gw.addr = ip_nxt;
  ipaddr.addr = NAT_LOCAL_IP;
  netmask.addr = htonl(~(ntohl(gw.addr) ^ ntohl(ipaddr.addr))); /* Only packets with an IP address 
								   matching the one assigned for the 
								   private network must be forwarded 
								   on the network interface */
  
  /* Set next private address to be used */
  ip_nxt = ntohl(ip_nxt);
  ++ip_nxt;			     
  ip_nxt = htonl(ip_nxt);

  return netif_add(&ipaddr, &netmask, &gw, state, init, nat_input);
}
/*-----------------------------------------------------------------------------------*/
