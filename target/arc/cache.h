#ifndef ARC_CPU_CACHE_H
#define ARC_CPU_CACHE_H

#include "qemu/osdep.h"
#include "cpu.h"

target_ulong arc_aux_i_cache_build_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_ic_ivic_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
void arc_aux_ic_ivil_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
void arc_aux_ic_ivir_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
void arc_aux_ic_endr_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_ic_endr_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_ic_ptag_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_ic_ptag_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_ic_ptag_hi_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_ic_ptag_hi_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_ic_ctrl_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_ic_ctrl_read(CPUARCState *env, uintptr_t retaddr);

target_ulong arc_aux_d_cache_build_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_dc_ivdc_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
void arc_aux_dc_ivdl_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
void arc_aux_dc_startr_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
void arc_aux_dc_endr_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_dc_endr_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_dc_ptag_hi_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_dc_ptag_hi_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_dc_ctrl_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_dc_ctrl_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_dc_flsh_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
void arc_aux_dc_fldl_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_volatile_read(CPUARCState *env, uintptr_t retaddr);

#endif
