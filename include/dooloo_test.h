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
/*   dooloo_test.h                             0.3.0                     */
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

#ifndef __D_DOOLOO_TEST_H__
#define __D_DOOLOO_TEST_H__

int test_schedule(struct shell_session *ss, int argc, char **argv);
int test_event(struct shell_session *ss, int argc, char **argv);
int test_semaphore_pc(struct shell_session *ss, int argc, char **argv);
int test_mutex_inherit(struct shell_session *ss, int argc, char **argv);
int test_messageq(struct shell_session *ss, int argc, char **argv);
int test_page_alloc(struct shell_session *ss, int argc, char **argv);
int test_page_free(struct shell_session *ss, int argc, char **argv);
int test_page_show(struct shell_session *ss, int argc, char **argv);
int test_slab(struct shell_session *ss, int argc, char **argv);
int test_memory(struct shell_session *ss, int argc, char **argv);

#endif /* __D_DOOLOO_TEST_H__ */
