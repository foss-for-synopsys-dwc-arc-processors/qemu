#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/helper-proto.h"
#include "accel/tcg/cpu-ldst.h"
#include "accel/tcg/probe.h"
#include "mmu.h"

static void arc_check_atomic_align(CPUARCState *env, target_ulong addr,
                                   uint32_t align, uintptr_t ra)
{
    if (addr & (align - 1)) {
        env->excp_mem_addr = addr;
        arc_raise_exception(env, ARC_EXCP_MISALIGNED_DATA, 0, ra);
    }
}

static hwaddr arc_atomic_paddr(CPUARCState *env, target_ulong addr,
                               MMUAccessType type, int size, uintptr_t ra)
{
    int mmu_idx = env->aux_status32.user_mode;
    hwaddr paddr;
    bool ok;

    probe_access(env, addr, size, type, mmu_idx, ra);

    ok = arc_mmu_get_paddr(env, addr, type, &paddr);
    assert(ok);

    return paddr;
}

uint32_t HELPER(llock)(CPUARCState *env, target_ulong addr)
{
    hwaddr paddr;
    uint32_t value;

    arc_check_atomic_align(env, addr, 4, GETPC());
    paddr = arc_atomic_paddr(env, addr, MMU_DATA_LOAD, 4, GETPC());

    value = cpu_ldl_data_ra(env, addr, GETPC());

    env->atomic_lpa = paddr;
    env->atomic_lf = 1;

    return value;
}

/*
 * SCOND and SCONDD will write successfully only if LF == 1 and
 * paddr == LPA. This behavior is *undocumented* and may verified
 * in nSIM.
 */

void HELPER(scond)(CPUARCState *env, target_ulong addr, uint32_t value)
{
    hwaddr paddr;
    bool success;

    arc_check_atomic_align(env, addr, 4, GETPC());
    paddr = arc_atomic_paddr(env, addr, MMU_DATA_STORE, 4, GETPC());

    success = env->atomic_lf && env->atomic_lpa == paddr;
    if (success) {
        cpu_stl_data_ra(env, addr, value, GETPC());
    }

    env->aux_status32.zero_flag = success;
    env->atomic_lf = 0;
}

uint64_t HELPER(llockd)(CPUARCState *env, target_ulong addr)
{
    hwaddr paddr;
    uint64_t value;

    arc_check_atomic_align(env, addr, 8, GETPC());
    paddr = arc_atomic_paddr(env, addr, MMU_DATA_LOAD, 8, GETPC());

    value = cpu_ldq_data_ra(env, addr, GETPC());

    env->atomic_lpa = paddr;
    env->atomic_lf = 1;

    return value;
}

void HELPER(scondd)(CPUARCState *env, target_ulong addr, uint64_t value)
{
    hwaddr paddr;
    bool success;

    arc_check_atomic_align(env, addr, 8, GETPC());
    paddr = arc_atomic_paddr(env, addr, MMU_DATA_STORE, 8, GETPC());

    success = env->atomic_lf && env->atomic_lpa == paddr;
    if (success) {
        cpu_stq_data_ra(env, addr, value, GETPC());
    }
    env->aux_status32.zero_flag = success;
    env->atomic_lf = 0;
}
