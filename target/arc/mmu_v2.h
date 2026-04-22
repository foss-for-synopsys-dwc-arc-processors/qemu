#ifndef ARC_CPU_MMU_V2_H
#define ARC_CPU_MMU_V2_H

#include "cpu.h"

/* Fixed 8 KB normal page (super pages are not supported) */
#define ARC_MMU_PAGE_BITS 13
#define ARC_MMU_PAGE_SIZE (1u << ARC_MMU_PAGE_BITS)

/* A VPN mask for vaddr */
#define ARC_MMU_VPN_MASK (((~0u) << ARC_MMU_PAGE_BITS) & 0x7fffffffu)

/* A VPN mask for hwaddr */
#define ARC_MMU_PAGE_MASK ((~0u) << ARC_MMU_PAGE_BITS)

#define ARC_MMU_TRANSLATED_END 0x80000000u

target_ulong arc_aux_pid_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_pid_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_tlbindex_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_tlbindex_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
void arc_aux_tlbcommand_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_tlbpd0_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_tlbpd0_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_tlbpd1_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_tlbpd1_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_scratch_data0_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_scratch_data0_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);

#endif
