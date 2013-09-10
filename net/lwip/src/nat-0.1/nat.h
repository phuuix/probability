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

#include "lwip/ip_addr.h"
#include "lwip/netif.h"

void nat_init(struct netif *external_netif, err_t (* tcpip_input)(struct pbuf *p, struct netif *inp));
void nat_tmr(void);
err_t nat_input(struct pbuf *p, struct netif *netif);
struct netif *nat_netif_add(void* state, err_t (* init)(struct netif *netif));

#if NAT_TCP_END_DETECT
enum nat_state {
  NAT_OPEN, NAT_FIN, NAT_CLOSING
};
#endif

struct nat_pcb {
  struct nat_pcb *next; /* For the linked list */

  struct ip_addr local_ip;

  u16_t local_port;
  u16_t ass_port; /* Uniquely Assigned port */

  u16_t tmr; /* Idle timer */

#if NAT_TCP_END_DETECT
  enum nat_state state;
  u8_t term_tmr;
#endif
};

#define NAT_EVENT_INPUT(nat_ctrl,p,netif,ret) \
                       if((nat_ctrl)->tcpip_input != NULL) { \
                       (ret = (nat_ctrl)->tcpip_input((p),(netif))); \
		       } else { \
		       (pbuf_free(p)); \
		       }

extern struct nat_pcb *nat_active_pcbs; /* List of all active NAT PCBs */
extern struct nat_pcb *nat_tmp_pcb; /* Only used for temporary storage. */

#define NAT_REG(pcbs, npcb) do { \
                            npcb->next = *pcbs; \
                            *pcbs = npcb; \
                            } while(0)
#define NAT_RMV(pcbs, npcb) do { \
                            if(*pcbs == npcb) { \
                               *pcbs = (*pcbs)->next; \
                            } else for(nat_tmp_pcb = *pcbs; nat_tmp_pcb != NULL; nat_tmp_pcb = nat_tmp_pcb->next) { \
                               if(nat_tmp_pcb->next != NULL && nat_tmp_pcb->next == npcb) { \
                                  nat_tmp_pcb->next = npcb->next; \
                                  break; \
                               } \
                            } \
                            npcb->next = NULL; \
                            } while(0)
