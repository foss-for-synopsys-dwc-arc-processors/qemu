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
#if defined(TARGET_ARC32)
    ARCCPU *cpu = env_archcpu(env);
    switch (cpu->family) {
    case ARC_OPCODE_ARC32:
	env->r[30] = newtls;
	break;
    default:
	env->r[25] = newtls;
	break;
    }
#elif defined(TARGET_ARC64)
    env->r[30] = newtls;
#else
    #error "TARGET macro not defined!"
#endif
}

static inline abi_ulong get_sp_from_cpustate(CPUARCState *env)
{
   return CPU_SP(env);
}
#endif
