#ifndef ARC_TARGET_CPU_H
#define ARC_TARGET_CPU_H

static inline void cpu_clone_regs_child(CPUARCState *env, target_ulong newsp,
                                        unsigned flags)
{
    if (newsp) {
        CPU_SP(env) = newsp;
    }

    //env->gpr[xA0] = 0;
}

static inline void cpu_clone_regs_parent(CPUARCState *env, unsigned flags)
{
}

static inline void cpu_set_tls(CPUARCState *env, target_ulong newtls)
{
  // TODO
    //env->gpr[xTP] = newtls;
}

static inline abi_ulong get_sp_from_cpustate(CPUARCState *env)
{
   return CPU_SP(env);
}
#endif
