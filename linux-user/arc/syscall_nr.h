/*
 * Syscall numbers from asm-generic, common for most
 * of recently-added arches including ARC.
 */

#ifndef LINUX_USER_ARC_SYSCALL_NR_H
#define LINUX_USER_ARC_SYSCALL_NR_H

#if defined(TARGET_ARC32)
# include "syscall32_nr.h"
#elif defined(TARGET_ARC64)
# include "syscall64_nr.h"
#else
# error "This should never happen !!!!"
#endif

#endif
