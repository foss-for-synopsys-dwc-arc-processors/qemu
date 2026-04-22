#include "qemu/osdep.h"
#include "exec/cputlb.h"
#include "cpu.h"
#include "cpu_bits.h"
#include "cache.h"

static void arc_invalidate_cache(CPUARCState *env)
{
    tlb_flush(env_cpu(env));
}

target_ulong arc_aux_i_cache_build_read(CPUARCState *env, uintptr_t retaddr)
{
    return env_archcpu(env)->aux_i_cache_build;
}

target_ulong arc_aux_d_cache_build_read(CPUARCState *env, uintptr_t retaddr)
{
    return env_archcpu(env)->aux_d_cache_build;
}

/* Invalidate instruction cache */
void arc_aux_ic_ivic_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_invalidate_cache(env);
}

/* Invalidate data cache */
void arc_aux_dc_ivdc_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_invalidate_cache(env);
}

/* Invalidate instruction cache line */
void arc_aux_ic_ivil_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_invalidate_cache(env);
}

/* Invalidate data cache line */
void arc_aux_dc_ivdl_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_invalidate_cache(env);
}

/* Invalidate instruction cache start region */
void arc_aux_ic_ivir_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_invalidate_cache(env);
}

/* Data cache region start */
void arc_aux_dc_startr_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_invalidate_cache(env);
}

/* Invalidate instruction cache end region */
void arc_aux_ic_endr_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_invalidate_cache(env);
    env->aux_ic_endr = value & 0xFFFFFF00;
}

target_ulong arc_aux_ic_endr_read(CPUARCState *env, uintptr_t retaddr)
{
    return env->aux_ic_endr;
}

/* Data cache region end address */
void arc_aux_dc_endr_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    env->aux_dc_endr = value & 0xFFFFFF00;
}

target_ulong arc_aux_dc_endr_read(CPUARCState *env, uintptr_t retaddr)
{
    return env->aux_dc_endr;
}

/* PTAG and PTAG_HI registers */
void arc_aux_ic_ptag_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    env->aux_ic_ptag = value;
}

target_ulong arc_aux_ic_ptag_read(CPUARCState *env, uintptr_t retaddr)
{
    return env->aux_ic_ptag;
}

void arc_aux_ic_ptag_hi_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    env->aux_ic_ptag_hi = value & 0xFF;
}

target_ulong arc_aux_ic_ptag_hi_read(CPUARCState *env, uintptr_t retaddr)
{
    return env->aux_ic_ptag_hi;
}

void arc_aux_dc_ptag_hi_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    env->aux_dc_ptag_hi = value & 0xFF;
}

target_ulong arc_aux_dc_ptag_hi_read(CPUARCState *env, uintptr_t retaddr)
{
    return env->aux_dc_ptag_hi;
}

/* Instruction cache control register */
void arc_aux_ic_ctrl_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_invalidate_cache(env);
    arc_unpack_aux_ic_ctrl(&env->aux_ic_ctrl, value);
}

target_ulong arc_aux_ic_ctrl_read(CPUARCState *env, uintptr_t retaddr)
{
    return arc_pack_aux_ic_ctrl(&env->aux_ic_ctrl);
}

/* Data cache control register */
void arc_aux_dc_ctrl_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_invalidate_cache(env);
    arc_unpack_aux_dc_ctrl(&env->aux_dc_ctrl, value);
}

target_ulong arc_aux_dc_ctrl_read(CPUARCState *env, uintptr_t retaddr)
{
    return arc_pack_aux_dc_ctrl(&env->aux_dc_ctrl);
}

/* Flush data cache */
void arc_aux_dc_flsh_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_invalidate_cache(env);
}

/* Flush data line */
void arc_aux_dc_fldl_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_invalidate_cache(env);
}

/* Volatile configuration register */
target_ulong arc_aux_volatile_read(CPUARCState *env, uintptr_t retaddr)
{
    /* TODO: This is copied from old QEMU as-is */
    return 0xC0000000;
}
