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
#define BLOCK_TASK 0
#define BLOCK_IPC 1
#define BLOCK_MEM 2
#define BLOCK_


int uprintf(uint8_t level, uint8_t block_id, char *fmt,...);
void uprintf_set_enable(uint8_t level,uint8_t block_id,uint8_t enable);


#endif //__UPRINTF_H__

