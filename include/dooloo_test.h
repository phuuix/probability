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
 *   test routines
 *
 * @History
 *   AUTHOR         DATE           NOTES
 */

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
