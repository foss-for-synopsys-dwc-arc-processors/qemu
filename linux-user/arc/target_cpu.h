#ifndef ARC_TARGET_CPU_H
#define ARC_TARGET_CPU_H

static inline void cpu_clone_regs_child(CPUARCState *env, target_ulong newsp,
                                        unsigned flags)
{
    if (newsp) {
        CPU_SP(env) = newsp;
    }

    /* Clone return 0 in child */
    env->r[0] = 0;
}

static inline void cpu_clone_regs_parent(CPUARCState *env, unsigned flags)
{
}

static inline void cpu_set_tls(CPUARCState *env, target_ulong newtls)
{
#if defined(TARGET_ARCv2)
    env->r[25] = newtls;
#else
    env->r[30] = newtls;
#endif
}

static inline abi_ulong get_sp_from_cpustate(CPUARCState *env)
{
   return CPU_SP(env);
}
#endif
