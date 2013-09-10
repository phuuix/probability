/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
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

#include "nat_memp.h"
#include "nat.h"
#include "lwip/mem.h"

struct memp {
  struct memp *next;
};

static struct memp *memp_tab[MEMP_NAT_MAX];

static const u16_t memp_sizes[MEMP_NAT_MAX] = {
  sizeof(struct nat_pcb),
};

static const u16_t memp_num[MEMP_NAT_MAX] = {
  MEMP_NUM_NAT_PCB
};

static u8_t memp_memory[(MEMP_NUM_NAT_PCB *
			 MEM_ALIGN_SIZE(sizeof(struct nat_pcb) +
					sizeof(struct memp)))];

/*-----------------------------------------------------------------------------------*/
void
nat_memp_init(void)
{
  struct memp *m, *memp;
  u16_t i, j;
  u16_t size;

  memp = (struct memp *)&memp_memory[0];
  for(i = 0; i < MEMP_NAT_MAX; ++i) {
    size = MEM_ALIGN_SIZE(memp_sizes[i] + sizeof(struct memp));
    if(memp_num[i] > 0) {
      memp_tab[i] = memp;
      m = memp;
      
      for(j = 0; j < memp_num[i]; ++j) {
	m->next = (struct memp *)MEM_ALIGN((u8_t *)m + size);
	memp = m;
	m = m->next;
      }
      memp->next = NULL;
      memp = m;
    } else {
      memp_tab[i] = NULL;
    }
  }
}
/*-----------------------------------------------------------------------------------*/
void *
nat_memp_malloc(nat_memp_t type)
{
  struct memp *memp;
  void *mem;

  memp = memp_tab[type];
  
  if(memp != NULL) {    
    memp_tab[type] = memp->next;    
    memp->next = NULL;

    mem = MEM_ALIGN((u8_t *)memp + sizeof(struct memp));
    /* initialize memp memory with zeroes */
    memset(mem, 0, memp_sizes[type]);	
    return mem;
  } else {
    return NULL;
  }
}
/*-----------------------------------------------------------------------------------*/
void
nat_memp_free(nat_memp_t type, void *mem)
{
  struct memp *memp;

  if(mem == NULL) {
    return;
  }
  memp = (struct memp *)((u8_t *)mem - sizeof(struct memp));
  
  memp->next = memp_tab[type]; 
  memp_tab[type] = memp;

  return;
}
/*-----------------------------------------------------------------------------------*/
