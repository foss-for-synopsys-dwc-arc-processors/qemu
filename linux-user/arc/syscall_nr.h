/*
 * Syscall numbers from asm-generic, common for most
 * of recently-added arches including ARC.
 */

#ifndef LINUX_USER_ARC_SYSCALL_NR_H
#define LINUX_USER_ARC_SYSCALL_NR_H

#ifdef TARGET_ARCV2
# include "syscall32_nr.h"
#elif TARGET_ARCV3
# include "syscall64_nr.h"
#else
# error "This should never happen !!!!"
#endif

#endif
