
#ifndef __D_LINKAGE_H__
#define __D_LINKAGE_H__

/* This file derives from linux/linkage.h. It allows a transparent naming
 * between COFF and ELF executable models.
 */

#ifdef __linux__ /*__LINUX__*/
#define SYMBOL_NAME_STR(X) #X
#define SYMBOL_NAME(X) X
#ifdef __STDC__
#define SYMBOL_NAME_LABEL(X) X##:
#else
#define SYMBOL_NAME_LABEL(X) X/**/:
#endif
#else
#define SYMBOL_NAME_STR(X) "_"#X
#ifdef __STDC__
#define SYMBOL_NAME(X) _##X
#define SYMBOL_NAME_LABEL(X) _##X##:
#else
#define SYMBOL_NAME(X) _/**/X
#define SYMBOL_NAME_LABEL(X) _/**/X/**/:
#endif
#endif

#endif /* __D_LINKAGE_H__ */
