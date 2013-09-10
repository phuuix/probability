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
 * This file is part of the lwBT Bluetooth stack.
 * 
 * Author: Conny Ohult <conny@sm.luth.se>
 *
 */

#ifndef __NATOPTS_H__
#define __NATOPTS_H__

/* ---------- NAT options ---------- */
/* NAT_LOCAL_IP: Control which the local IP address will be. */
#define NAT_LOCAL_IP htonl(((u32_t)(192 & 0xff) << 24) | ((u32_t)(168 & 0xff) << 16) | \
		     ((u32_t)(99 & 0xff) << 8) | (u32_t)(1 & 0xff))
/* NAT_MIN_IP and NAT_MAX_IP specify the range used to assign addresses to remote hosts in a private
   network. Default is the private address range 
   192.168.99.2--192.168.255.254 */
/* NAT_MIN_IP: Control which the first IP address of the pool range will be. */
#define NAT_MIN_IP htonl(((u32_t)(192 & 0xff) << 24) | ((u32_t)(168 & 0xff) << 16) | \
		     ((u32_t)(99 & 0xff) << 8) | (u32_t)(2 & 0xff))
/* NAT_MAX_IP: Control which the last IP address of the pool range will be */
#define NAT_MAX_IP htonl(((u32_t)(192 & 0xff) << 24) | ((u32_t)(168 & 0xff) << 16) | \
                         ((u32_t)(99 & 0xff) << 8) | (u32_t)(254 & 0xff))

/* NAT_MIN_PORT and NAT_MAX_PORT specify the range used to assign port numbers that can be used 
   to form globally unique address */
#define NAT_MIN_PORT 49152
#define NAT_MAX_PORT 65535
/* NAT_RETIRE_LONGEST_IDLE: Control if the longest idle session should be retired when it 
   becomes necessary */
#define NAT_RETIRE_LONGEST_IDLE 1 /* Default 1 */
/* NAT_IDLE_TIMEOUT: Control how long a session can remain idle before it 
   is removed */
#define NAT_IDLE_TIMEOUT 0 /* Default 0 (No timeout). RFC2663 informs that TCP sessions 
			      that have not been used for say, 24 hours, and non-TCP 
			      sessions that have not been used for a couple of minutes, are 
			      terminated, but that this is application specific */
/* NAT_TCP_END_DETECT: Control if a TCP session should be closed after 2 * MSL (Maximum 
   Segment Lifetime) or 4 minutes when a RST or second FIN is sent or received */
#define NAT_TCP_END_DETECT 0 /* Default 1 */

#endif /* __NATOPTS_H__ */
