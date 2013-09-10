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
/*   buffer.c                                  0.3.0                     */
/*                                                                       */
/* COMPONENT                                                             */
/*   Kernel                                                              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*                                                                       */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-20               Version 0.3.0                */
/*************************************************************************/ 

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <buffer.h>

/*
 * init string buffer
 * string buffer is consist of some strings
 *   sbuf: pointer to string buffer struct
 *   data: string buffer data
 *   len: length of data
 */
void str_buffer_init(struct str_buffer *sbuf, char *data, uint16_t len)
{
	if(sbuf)
	{
		sbuf->data = data;
		sbuf->len = len;
		sbuf->vlen = len;
		sbuf->head = 0;
		sbuf->tail = 0;

		memset(sbuf->data, 0, len);
	}
}


/*
 * put string into string buffer
 *   sbuf: pointer to string buffer struct
 *   str: string to be put
 *   return: 1 on success, 0 on failed
 */
int str_buffer_put(struct str_buffer *sbuf, char *str)
{
	int len;
	char *p;

	len = strlen(str);
	if(sbuf->vlen > len)
	{
		/* buffer is large enough */
		sbuf->vlen -= len + 1;
		for(p=sbuf->data+sbuf->tail; len>0; len--)
		{
			*p++ = *str++;
			if(p == sbuf->data + sbuf->len)
				p = sbuf->data;	/* wrap */
		}
		*p = '\0';
		sbuf->tail = p - sbuf->data + 1;
		if(sbuf->tail == sbuf->len)
			sbuf->tail = 0;	/* wrap */
#if 0		
		if((sbuf->len - sbuf->tail) > len)
		{
			/* don't need warp */
			strcpy(sbuf->data+sbuf->tail, str);
			sbuf->tail += len + 1;
		}else{
			/* need warp */
			int n = sbuf->len - sbuf->tail;
			memcpy(sbuf->data+sbuf->tail, str, n);
			strcpy(sbuf->data, str+n);
			sbuf->tail = len - n + 1;
		}
		
		/* check if tail need warp */
		if(sbuf->tail == sbuf->len)
			sbuf->tail = 0;
#endif /* 0 */	
		return 1;
	}
	return 0;
}


/*
 * get a string from string buffer
 *   sbuf: pointer to string buffer struct
 *   str: string to store
 *   len: len of str
 *   return: realy length of string gotten, or 0
 */
int str_buffer_get(struct str_buffer *sbuf, char *str, uint16_t len)
{
	int rlen;
	char *p;

	if(sbuf->vlen == sbuf->len)
		return 0;

	for(p=sbuf->data+sbuf->head, rlen=0; *p!=0 && len>0; rlen++, len --)
	{
		*str++ = *p++;
		if(p == sbuf->data + sbuf->len)
			p = sbuf->data;		/* wrap */
	}

	if(len == 0 || rlen == 0)		/* length is too short */
		return 0;
	else
		*str = '\0';

	sbuf->head = p - sbuf->data + 1;
	if(sbuf->head == sbuf->len)
		sbuf->head = 0;	/* wrap */
	sbuf->vlen += rlen + 1;

	return rlen;
}


/*
 * init memory buffer
 * Memory buffer is consist of some memory blocks, these memory blocks
 * is start with two bytes that is the length of current memory block
 *   mbuf: pointer to memory buffer struct
 *   data: memory buffer data
 *   len: length of data
 */
void mem_buffer_init(struct mem_buffer *mbuf, char *data, uint16_t len)
{
	/* data must align with WORD and len mod 2 must be 0 */ 
	if(mbuf)
	{
		if((uint32_t)data & 0x01)
			data ++;
		mbuf->data = data;
		mbuf->len = len & 0xfffe;
		mbuf->vlen = len & 0xfffe;
		mbuf->head = 0;
		mbuf->tail = 0;
	}
}


/*
 * put memory into memory buffer
 *   mbuf: pointer to memory buffer struct
 *   mem: memory to be put
 *   len: length of memory
 *   return: 1 on success, 0 on failed
 */
int mem_buffer_put(struct mem_buffer *mbuf, char *mem, uint16_t len)
{
	uint16_t ll;

	/* ll mod 2 must be 0 */
	ll = len + (len & 0x01);

	if(mbuf->vlen >= ll + 2)
	{	
		/* buffer is large enough */
		mbuf->vlen -= ll+2;

		/* put len */
		if(mbuf->tail < mbuf->len)
		{
			/* len is put unwraped */
			*(uint16_t *)(mbuf->data + mbuf->tail) = len;
			mbuf->tail += 2;
		}else{
			/* mbuf->tail == mbuf->len */
			*(uint16_t *)(mbuf->data) = len;
			mbuf->tail = 2;
		}

		/* put message */
		if(mbuf->len-mbuf->tail >= len)
		{
			/* don't need to warp data*/
			memcpy(mbuf->data+mbuf->tail, mem, len);
			mbuf->tail += ll;
		}else{
			/* need to warp data */
			uint16_t n = mbuf->len - mbuf->tail;
			memcpy(mbuf->data+mbuf->tail, mem, n);
			memcpy(mbuf->data, mem+n, len-n);
			mbuf->tail = ll-n;
		}

		return 1;
	}

	return 0;
}


/*
 * get a memory block from memory buffer
 *   mbuf: pointer to memory buffer struct
 *   mem: string to store
 *   mlen: length of memory block
 *   return: realy length of block gotten, or 0
 */
int mem_buffer_get(struct mem_buffer *mbuf, char *mem, uint16_t mlen)
{
	uint16_t len, ll;

	if(mbuf->vlen == mbuf->len)
		return 0;

	/* get len */
	if(mbuf->head == mbuf->len)
		mbuf->head = 0;
	len = *(uint16_t *)(mbuf->data + mbuf->head);
	if(len > mlen)
		return 0;
	
	ll = len + (len & 0x01);
	mbuf->vlen += ll + 2;
	mbuf->head += 2;

	if(mbuf->head + len > mbuf->len)
	{
		/* wrap */
		uint16_t n = mbuf->len - mbuf->head;
		memcpy(mem, mbuf->data + mbuf->head, n);
		memcpy(mem+n, mbuf->data, len-n);
		mbuf->head = ll - n;
	}else{
		memcpy(mem, mbuf->data + mbuf->head, len);
		mbuf->head += ll;
	}
	
	return len;
}


/*
 * init queue buffer
 * Queue buffer is consist of some queue unit
 *   qbuf: pointer to queue buffer struct
 *   data: memory buffer data
 *   unit_len: length of unit, in long words
 *   unit_num: number of unit
 */
void queue_buffer_init(struct queue_buffer *qbuf, char *data, uint16_t unit_len, uint16_t unit_num)
{
	if(qbuf)
	{
		qbuf->data = data;
		qbuf->unit_len = unit_len * sizeof(uint32_t);
		qbuf->len = unit_num * qbuf->unit_len;
		qbuf->vlen = qbuf->len;
		qbuf->head = 0;
		qbuf->tail = 0;
	}
}


/*
 * put unit into queue buffer
 *   qbuf: pointer to queue buffer struct
 *   unit: unit to be put
 *   return: 1 on success, 0 on failed
 */
int queue_buffer_put(struct queue_buffer *qbuf, uint32_t *unit)
{
	int i;
	uint32_t *pos;

	if(qbuf->vlen >= qbuf->unit_len)
	{
		pos = (uint32_t *)(qbuf->data + qbuf->tail);
		for(i=0; i<(qbuf->unit_len >> 2); i++)
		{
			*pos++ = *unit++;
		}

		qbuf->tail += qbuf->unit_len;
		if(qbuf->tail == qbuf->len)
			qbuf->tail = 0;		/* wrap */
		qbuf->vlen -= qbuf->unit_len;

		return 1;
	}

	return 0;
}


/*
 * get a unit from queue buffer
 *   qbuf: pointer to queue buffer struct
 *   unit: unit to store
 *   return: 1 on success, or 0
 */
int queue_buffer_get(struct queue_buffer *qbuf, uint32_t *unit)
{
	int i;
	uint32_t *pos;

	if(qbuf->vlen < qbuf->len)
	{
		pos = (uint32_t *)(qbuf->data + qbuf->head);
		for(i=0; i<(qbuf->unit_len >> 2); i++)
		{
			*unit++ = *pos++;
		}

		qbuf->head += qbuf->unit_len;
		if(qbuf->head == qbuf->len)
			qbuf->head = 0;		/* wrap */
		qbuf->vlen += qbuf->unit_len;

		return 1;
	}

	return 0;
}
