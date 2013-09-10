/*********************************************************************/
/* Dooloo OS                                                          */
/*********************************************************************/

/* config.h
  * configure file for hostsim
 */

#ifndef __D_SYSCONFIG_H__
#define __D_SYSCONFIG_H__


/* user_app_init entry */
//#define INCLUDE_USER_APP_INIT

/*********************************************************************/
/* gdb stub module configure                                         */
/*********************************************************************/
/* include gdb stub module */
#define INCLUDE_GDB_STUB
/* the approch to connect to gdb stub: udp or serial port */
#define GDB_REMOTE_UDP
//#define GDB_REMOTE_SERIAL

/* gdb net interface struct */
#define GDB_NET_INTERFACE NULL
/* gdb net interface's ip address */
#define GDB_NET_IPADDRESS 0xc0585864 /* 192.88.88.100 */

/*********************************************************************/
/* task module configure                                             */
/*********************************************************************/
/* maximal priority levles */
#define MAX_PRIORITY_LEVEL		8
/* clock interrupt number */
#define INT_SYSTICK	15
/* ticks per second */
#define TICKS_PER_SECOND	100
/* system's maximal task number */
#define MAX_TASK_NUMBER		8
/* system's maximal task_delay number */
#define MAX_TASK_DELAY_NUMBER	8
/* idle task's stack size */
#define TILDE_STACK_SIZE	0x400
/* kservice task's stack size */
#define TKSERV_STACK_SIZE	0x400       // no kservice task
/* kservice task's priority */
#define TKSERV_PRIORITY		128
#define TPRINT_STACK_SIZE   0x500
#define TPRINT_PRIORITY     210
/* block queue is size sensitive */
#define BLOCKQ_IS_SIZE_SENSTIVE 1

/*********************************************************************/
/* memory module configure                                           */
/*********************************************************************/
#define MEMORY_DEBUG
/* system's max s_cache number */
#define MAX_S_CACHE_NUMBER 4
/* advance memory policy */
#define MEMORY_ADVANCE 0
/* size used in none advance memory policy */
#define MEMORY_SIZE64_N     80
#define MEMORY_SIZE256_N    20
#define MEMORY_SIZE768_N    8
#define MEMORY_SIZEBIG_N    4

#define MEMORY_SIZE64       64
#define MEMORY_SIZE256      256
#define MEMORY_SIZE768      1024
#define MEMORY_SIZEBIG      1600

/*********************************************************************/
/* page module configure                                             */
/*********************************************************************/
/* zone number */
#define SYS_ZONE_NUMBER 0x01
/* max pages in zone */
#define MAX_PAGES_IN_ZONE 1024		/* max memory in zone is 4M */
/* max areas in zone */
#define MAX_AREAS_IN_ZONE 10		/* max allcatation unit is 2M */

/* definition for zone0 */
#define ZONE0_MEMORY_LOW_ADDR	0x400000
#define ZONE0_PAGE_COUNT		1024


/*********************************************************************/
/* memory slab module configure                                      */
/*********************************************************************/
/* system's max kmem_cache number */
#define MAX_KMEM_CACHE_NUMBER 16


/*********************************************************************/
/* signal configure                                                   */
/*********************************************************************/
/* include signal module */
//#define INCLUDE_SIGNAL
/* system's maximal event number */
#define _NSIG	8

/*********************************************************************/
/* tty module configure                                              */
/*********************************************************************/
/* MAX tty number */
#define MAX_TTY_NUMBER	1
/* tty_input task's stack size */
#define TTTY_INPUT_STACK_SIZE	0x300
/* tty_input task's priority */
#define TTTY_INPUT_PRIORITY	0x2
/* tty_output task's stack size */
#define TTTY_OUTPUT_STACK_SIZE	0x300
/* tty_output task's priority */
#define TTTY_OUTPUT_PRIORIY 0x7
/* tty input mbox number */
#define TTY_INPUT_MBOX_NUM 0x1
/* tty output buffer size */
#define TTY_OUTPUT_BUFFER_SIZE	0x4

/*********************************************************************/
/* command module configure                                          */
/*********************************************************************/
/* command task's stack size */
#define TCOMMAND_STACK_SIZE	0x300
/* command task's priority */
#define TCOMMAND_PRIORITY	0x5
/* if support shell command */
#define SHELL_COMMAND

/*********************************************************************/
/* net module configure                                              */
/*********************************************************************/
/* lwip tcp/ip stack */
#define IP_STACK_LWIP

/* max hareware address length */
#define NET_MAX_HWADDR_LEN		6

/* ipv4's max interface number */
#define IPV4_MAX_INTF_NUMBER	4
/* ipv4 udp's max connect number */
#define IPV4_UDP_MAX_CONNECT	4

/* ipv4 mblk number */
#define IPV4_MBLK_NUMBER	100
/* ipv4 cblk number */
#define IPV4_CBLK_NUMBER	50
/* ipv4 cpool number */
#define IPV4_CPOOL_NUMBER	7
/* ipv4 cluster64 number */
#define IPV4_CLUSTER64_NUMBER	32
/* ipv4 cluster128 number */
#define IPV4_CLUSTER128_NUMBER	64
/* ipv4 cluster256 number */
#define IPV4_CLUSTER256_NUMBER	64
/* ipv4 cluster512 number */
#define IPV4_CLUSTER512_NUMBER  64
/* ipv4 cluster1024 number */
#define IPV4_CLUSTER1024_NUMBER 64
/* ipv4 cluster2048 number */
#define IPV4_CLUSTER2048_NUMBER 4
/* ipv4 cluster4096 number */
#define IPV4_CLUSTER4096_NUMBER 2

#endif /* __D_SYSCONFIG_H__ */

