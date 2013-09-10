/*
 * This is a temp errno realization.
 */

int errno;

int *__errno_location(void) __attribute__((weak));
int *__errno_location() { return &errno; }

