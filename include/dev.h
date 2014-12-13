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
 *   Device management data structure
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */

#ifndef __D_DEV_H__
#define __D_DEV_H__

#define DEVICE_NAME_LEN	8

struct device
{
	struct device *next;				/* next pointer */
	char name[DEVICE_NAME_LEN];			/* name of device */
	void *data;							/* user data */

	int (*create)();					/* create function */
	int (*destroy)();					/* destroy function */
	int (*open)();						/* open function */
	int (*close)();						/* close function */
	int (*ioctl)();						/* ioctl function */
	int (*read)();						/* read function */
	int (*write)();						/* write function */
};

typedef struct device DEVICE;

#endif /* __D_DEV_H__ */
