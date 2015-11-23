/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 *
 * Copyright (c) Puhui Xiong <phuuix@163.com>
 * @file
 *   privde kernel service such as CPU load statisstic
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */

#ifndef __KSERVICE_H__
#define __KSERVICE_H__

typedef uint32_t (*kserv_func_t)(void *timer, uint32_t param1, uint32_t param2);

typedef struct kserv_mail
{
    uint32_t priority:8;
    uint32_t periodic:1; 
    uint32_t timeunit:7;         /* tick, s, minute, hour */
    uint32_t interval:16;
	kserv_func_t func;
	uint32_t param[2];
}kserv_mail_t;

void kservice_init();

#endif

