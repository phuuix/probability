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
 *   user level printf
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */

#ifndef __UPRINTF_H__
#define __UPRINTF_H__

/* print level */
#define UPRINT_EMERG       0       /* system is unusable */
#define UPRINT_ALERT       1       /* action must be taken immediately */
#define UPRINT_CRITICAL    2       /* critical conditions */
#define UPRINT_ERROR       3       /* error conditions */
#define UPRINT_WARNING     4       /* warning conditions */
#define UPRINT_NOTICE      5       /* normal but significant condition */
#define UPRINT_INFO        6       /* informational */
#define UPRINT_DEBUG       7       /* debug-level messages */
#define UPRINT_MLEVEL      8       /* maximum level */

/* block id */
#define UPRINT_BLK_DEF 0           /* default block */
#define UPRINT_BLK_TSK 1
#define UPRINT_BLK_IPC 2
#define UPRINT_BLK_MEM 3
#define UPRINT_BLK_CLI 4
#define UPRINT_BLK_NET 5

#define UPRINT_MAX_BLOCK 16
#define UPRINTF_BUFSIZE    256
#define UPRINTF_BUFNUM     64						/* must be power of 2 */

int uprintf(uint8_t level, uint8_t block_id, char *fmt,...);
void uprintf_set_enable(uint8_t level,uint8_t block_id,uint8_t enable);
void uprintf_init();
int uprintf_enabled(uint8_t level, uint8_t block_id);
void uprintf_set_enable(uint8_t level, uint8_t block_id, uint8_t enable);
uint32_t uprintf_get_flags();
uint32_t uprintf_set_flags(uint32_t flags);
int uprintf_default(char *fmt,...);

#endif //__UPRINTF_H__

