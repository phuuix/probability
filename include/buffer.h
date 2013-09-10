/*************************************************************************/
/* The Dooloo kernel                                                     */
/* Copyright (C) 2004-2006 Xiong Puhui (Bearix)                          */
/* All Rights Reserved.                                                  */
/*                                                                       */
/* THIS WORK CONTAINS TRADE SECRET AND PROPRIETARY INFORMATION WHICH IS  */
/* THE PROPERTY OF DOOLOO RTOS DEVELOPMENT TEAM                          */
/*                                                                       */
/*************************************************************************/

/*************************************************************************/
/*                                                                       */
/* FILE                                       VERSION                    */
/*   buffer.h                                  0.3.0                     */
/*                                                                       */
/* COMPONENT                                                             */
/*   Kernel                                                              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*   Buffer routines                                                                    */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-20               Version 0.3.0                */
/*************************************************************************/ 

/*
 * buffer.h
 */

#ifndef __D_BUFFER_H__
#define __D_BUFFER_H__

#include <sys/types.h>


/* string buffer */
struct str_buffer
{
	uint16_t head;				/* buffer head */
	uint16_t tail;				/* buffer tail */

	char *data;					/* pointer to data */
	uint16_t len;				/* length of data */
	uint16_t vlen;				/* length of empty data */
};

/* mem buffer */
struct mem_buffer
{
	uint16_t head;				/* buffer head */
	uint16_t tail;				/* buffer tail */

	char *data;					/* pointer to data */
	uint16_t len;				/* length of data */
	uint16_t vlen;				/* length of empty data */
};

/* queue buffer */
struct queue_buffer
{
	uint16_t head;				/* buffer head */
	uint16_t tail;				/* buffer tail */

	char *data;					/* pointer to data */
	uint16_t unit_len;			/* length of data unit, in bytes */
	uint16_t len;				/* length of data, in bytes */
	uint16_t vlen;				/* length of empty data, in bytes */
};

void str_buffer_init(struct str_buffer *sbuf, char *data, uint16_t len);
int str_buffer_put(struct str_buffer *sbuf, char *str);
int str_buffer_get(struct str_buffer *sbuf, char *str, uint16_t len);

void mem_buffer_init(struct mem_buffer *mbuf, char *data, uint16_t len);
int mem_buffer_put(struct mem_buffer *mbuf, char *mem, uint16_t len);
int mem_buffer_get(struct mem_buffer *mbuf, char *mem, uint16_t mlen);

void queue_buffer_init(struct queue_buffer *qbuf, char *data, uint16_t unit_len, uint16_t unit_num);
int queue_buffer_put(struct queue_buffer *qbuf, uint32_t *unit);
int queue_buffer_get(struct queue_buffer *qbuf, uint32_t *unit);

#endif /* __D_BUFFER_H__ */
