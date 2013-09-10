/*
 * lock.c: provide atomic lock service
 */
#include <config.h>
#include <bsp.h>
#include <lock.h>

/*
 * lock
 *   l: lock_t pointer
 *   return: 1 on success; 0 on failed
 */
uint8_t atomic_lock(lock_t *l)
{
	if(test_and_set(l))
		return 0;
	else
		return 1;
}

/*
 * unlock
 *   l: lock_t pointer
 */
void atomic_unlock(lock_t *l)
{
	*l = 0;
}

