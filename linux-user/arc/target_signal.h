#ifndef ARC_TARGET_SIGNAL_H
#define ARC_TARGET_SIGNAL_H

#define TARGET_ARCH_HAS_SIGTRAMP_PAGE 0

typedef struct target_sigaltstack {
    abi_ulong ss_sp;
    abi_int ss_flags;
    abi_ulong ss_size;
} target_stack_t;

#define TARGET_SS_ONSTACK 1
#define TARGET_SS_DISABLE 2

#include "../generic/signal.h"

#endif /* ARC_TARGET_SIGNAL_H */
