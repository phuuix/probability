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
/*   dev.h                                     0.3.0                     */
/*                                                                       */
/* COMPONENT                                                             */
/*   Kernel                                                              */
/*                                                                       */
/* DESCRIPTION                                                           */
/*   Device management data structure                                    */
/*                                                                       */
/* CHANGELOG                                                             */
/*   AUTHOR         DATE                    NOTES                        */
/*   Bearix         2006-8-20               Version 0.3.0                */
/*************************************************************************/ 


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
