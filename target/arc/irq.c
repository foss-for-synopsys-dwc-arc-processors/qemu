#include "qemu/osdep.h"
#include "qemu/log.h"
#include "accel/tcg/cpu-ldst.h"
#include "cpu.h"
#include "irq.h"

/* TODO: Set ECT.P=1 when exception is generated from IRQ's prologue */

target_ulong arc_get_irq_priority_pending(CPUARCState *env, bool enabled_irqs_only)
{
    target_ulong result = 0;

    for (target_ulong i = ARC_FIRST_INTERRUPT_NR; i <= ARC_LAST_INTERRUPT_NR; i++) {
        if (enabled_irqs_only && !env->aux_irq_bank[i].enable) {
            continue;
        }

        if (env->aux_irq_bank[i].pending) {
            g_assert(env->aux_irq_bank[i].priority <= 15);

            result |= 1 << env->aux_irq_bank[i].priority;
        }
    }

    return result;
}

target_ulong arc_load_vector_entry(CPUARCState *env, uint32_t vector)
{
    target_ulong addr = env->aux_int_vector_base +
                        ((target_ulong)vector << ARC_VECTOR_ENTRY_SHIFT);

#ifdef TARGET_ARCV3_64
    return cpu_ldq_data(env, addr);
#else
    return cpu_ldl_data(env, addr);
#endif
}

void arc_irq_push(CPUARCState *env, target_ulong value, const char *str)
{
    qemu_log_mask(CPU_LOG_INT, "arc: irq: push %s\n", str);
    env->gpr[ARC_REGNUM_SP] -= sizeof(target_ulong);
#ifdef TARGET_ARCV3_64
    cpu_stq_data(env, env->gpr[ARC_REGNUM_SP], value);
#else
    cpu_stl_data(env, env->gpr[ARC_REGNUM_SP], value);
#endif
}

target_ulong arc_irq_pop(CPUARCState *env, const char *str)
{
#ifdef TARGET_ARCV3_64
    target_ulong value = cpu_ldq_data(env, env->gpr[ARC_REGNUM_SP]);
#else
    target_ulong value = cpu_ldl_data(env, env->gpr[ARC_REGNUM_SP]);
#endif

    qemu_log_mask(CPU_LOG_INT, "arc: irq: pop %s\n", str);
    env->gpr[ARC_REGNUM_SP] += sizeof(target_ulong);

    return value;
}

void arc_enter_irq(ARCCPU *cpu)
{
    CPUARCState *env = &cpu->env;
    target_ulong tmp;
    arc_aux_status_t status32 = env->aux_status32;
    char regname[16];

    /* Switch to kernel stack if it's necessary */
    if (!env->aux_irq_ctrl.user_stack && env->aux_status32.user_mode) {
        tmp = env->aux_user_sp;
        env->aux_user_sp = env->gpr[ARC_REGNUM_R28];
        env->gpr[ARC_REGNUM_R28] = tmp;
        env->aux_status32.user_mode = 0;
    }

    env->gpr[ARC_REGNUM_ILINK] = env->pc;
    env->aux_eret = env->pc;

    arc_irq_push(env, arc_pack_status32(&status32), "STATUS32");
    arc_irq_push(env, env->pc, "PC");

    if (env->aux_irq_ctrl.code_density) {
        arc_irq_push(env, env->aux_jli_base, "JLI_BASE");
        arc_irq_push(env, env->aux_ldi_base, "LDI_BASE");
        arc_irq_push(env, env->aux_ei_base, "EI_BASE");
    }

#ifdef TARGET_ARCV2
    if (env->aux_irq_ctrl.loop) {
        arc_irq_push(env, env->gpr[ARC_REGNUM_LP_COUNT], "LP_COUNT");
        arc_irq_push(env, env->aux_lp_start, "LP_START");
        arc_irq_push(env, env->aux_lp_end, "LP_END");
    }
#endif

    /* Push GPR */
    g_assert(env->aux_irq_ctrl.nr <= 16);

    if (env->aux_irq_ctrl.blink && env->aux_irq_ctrl.nr != 16) {
        arc_irq_push(env, env->gpr[ARC_REGNUM_BLINK], "BLINK");
    }

    for (int i = 2 * env->aux_irq_ctrl.nr; i > 0; i--) {
        sprintf(regname, "R%d", i);
        arc_irq_push(env, env->gpr[i - 1], regname);
    }

    /* Late switch to kernel SP if in user thread */
    if (env->aux_irq_ctrl.user_stack && env->aux_status32.user_mode) {
        tmp = env->aux_user_sp;
        env->aux_user_sp = env->gpr[ARC_REGNUM_R28];
        env->gpr[ARC_REGNUM_R28] = tmp;
    }

    /* Set STATUS32 bits */
    env->aux_status32.zero_flag = status32.user_mode;
    env->aux_status32.user_mode = 0;
    env->aux_status32.disable_zol = 1;
    env->aux_status32.ei_pending = 0;
    env->aux_status32.enable_divzero_excp = 0;
    env->aux_status32.delay_slot_pending = 0;
    env->atomic_lf = 0;
}

void arc_enter_firq(ARCCPU *cpu)
{
    CPUARCState *env = &cpu->env;
    arc_aux_status_t status32 = env->aux_status32;
    target_ulong tmp;

    env->gpr[ARC_REGNUM_ILINK] = env->pc;
    env->aux_status32_p0 = status32;
    env->aux_status32_p0.halt = 0;

    if (FIELD_EX32(cpu->aux_rf_build, AUX_RF_BUILD, BANKS) > 0) {
        arc_cpu_switch_gpr_bank(env, 1);
    }

    if (status32.user_mode) {
        tmp = env->aux_user_sp;
        env->aux_user_sp = env->gpr[ARC_REGNUM_R28];
        env->gpr[ARC_REGNUM_R28] = tmp;
    }

    env->aux_status32.zero_flag = status32.user_mode;
    env->aux_status32.user_mode = 0;
    env->aux_status32.disable_zol = 1;
    env->aux_status32.ei_pending = 0;
    env->aux_status32.enable_divzero_excp = 0;
    env->aux_status32.delay_slot_pending = 0;
    env->atomic_lf = 0;
}

bool arc_cpu_exec_interrupt(CPUState *cs, int interrupt_request)
{
    ARCCPU *cpu = ARC_CPU(cs);
    CPUARCState *env = &cpu->env;
    target_ulong priorities_pending = arc_get_irq_priority_pending(env, true);

    /* It must be a hard interrupt */
    if (!(interrupt_request & CPU_INTERRUPT_HARD)) {
        return false;
    }

    /* CPU is not halted, interrupts are enabled and it's not handling an exception */
    if (env->aux_status32.halt || !env->aux_status32.enable_interrupt || env->aux_status32.exception_state || env->aux_status32.delay_slot_pending) {
        return false;
    }

    /* Priorities list must not be empty */
    if (!priorities_pending) {
        return false;
    }

    /* Hold this interrupt if it's priority is lower or the same than taken's one */
    if (ctz32(priorities_pending) >= ctz32(env->aux_irq_act.active)) {
        return false;
    }

    /* Find the first IRQ to serve */
    bool found = false;
    uint32_t irq_number = 0;
    arc_aux_irq_status_t *banked_irq = NULL;

    for (int priority = 0; !found && priority <= env->aux_status32.interrupt_priority; priority++) {
        for (irq_number = ARC_FIRST_INTERRUPT_NR; irq_number <= ARC_LAST_INTERRUPT_NR; irq_number++) {
            banked_irq = &env->aux_irq_bank[irq_number];
            if (banked_irq->priority == priority && banked_irq->enable && banked_irq->pending) {
                found = true;
                break;
            }
        }
    }

    if (!found) {
        return false;
    }

    qemu_log_mask(CPU_LOG_INT, "arc: irq: start handling irq=%u pc=" TARGET_FMT_lx "\n", irq_number, env->pc);

    /* Set U bit in AUX_IRQ_ACT if it's a first interrupt */
    if (env->aux_irq_act.active == 0) {
        env->aux_irq_act.from_user = env->aux_status32.user_mode;
    } else {
        /* We do not expect that STATUS32.U is set during IRQ handling */
        g_assert(!env->aux_status32.user_mode);
    }

    /* Set priority queue in AUX_IRQ_ACT */
    env->aux_irq_act.active |= 1 << banked_irq->priority;

    /* Set ICAUSE register */
    env->aux_icause_banked[banked_irq->priority] = irq_number;

    if (arc_cpu_has_firq(env) && banked_irq->priority == 0) {
        arc_enter_firq(cpu);
    } else {
        arc_enter_irq(cpu);
    }

    /* Calculate entry address */
    env->pc = arc_load_vector_entry(env, irq_number);
    env->gpr[ARC_REGNUM_PCL] = arc_pc_to_pcl(env->pc);

    qemu_log_mask(CPU_LOG_INT, "arc: irq: jumping to pc=" TARGET_FMT_lx "\n", env->pc);

    return true;
}

void arc_do_rtie_exception(CPUARCState *env)
{
    target_ulong tmp;

    qemu_log_mask(CPU_LOG_INT, "arc: exception: returning from pc=0x" TARGET_FMT_lx " to eret=0x" TARGET_FMT_lx "\n", env->pc, env->aux_eret);

    env->pc = env->aux_eret;
    env->gpr[ARC_REGNUM_PCL] = arc_pc_to_pcl(env->pc);
    env->aux_bta = env->aux_erbta;

    arc_cpu_switch_gpr_bank(env, env->aux_erstatus.register_bank);
    env->aux_status32 = env->aux_erstatus;

    if (arc_in_user_mode(env)) {
        tmp = env->aux_user_sp;
        env->aux_user_sp = env->gpr[ARC_REGNUM_SP];
        env->gpr[ARC_REGNUM_SP] = tmp;
    }
}

void arc_do_rtie_interrupt(CPUARCState *env)
{
    target_ulong tmp;
    uint32_t priority = ctz32(env->aux_irq_act.active);
    arc_aux_status_t new_status;
    char regname[16];

    qemu_log_mask(CPU_LOG_INT, "arc: irq: returning from irq with priority=%u\n", priority);

    /* Clear the currently-active interrupt */
    env->aux_irq_act.active &= ~(1 << priority);

    /*
     * Early switch back to the user stack when the outermost handler returns to
     * a user thread whose context was saved on the user stack.
     */
    if (env->aux_irq_act.active == 0 && env->aux_irq_act.from_user && env->aux_irq_ctrl.user_stack) {
        tmp = env->aux_user_sp;
        env->aux_user_sp = env->gpr[ARC_REGNUM_R28];
        env->gpr[ARC_REGNUM_R28] = tmp;

        /* Force all pops to be performed from User mode */
        env->aux_status32.user_mode = 1;
    }

    /* Pop GPR */
    g_assert(env->aux_irq_ctrl.nr <= 16);

    for (int i = 1; i <= 2 * env->aux_irq_ctrl.nr; i++) {
        sprintf(regname, "R%d", i - 1);
        if (i - 1 == ARC_REGNUM_SP) {
            tmp = arc_irq_pop(env, regname);
        } else {
            env->gpr[i - 1] = arc_irq_pop(env, regname);
        }
    }

    if (env->aux_irq_ctrl.blink && env->aux_irq_ctrl.nr != 16) {
        env->gpr[ARC_REGNUM_BLINK] = arc_irq_pop(env, "BLINK");
    }

#ifdef TARGET_ARCV2
    if (env->aux_irq_ctrl.loop) {
        env->aux_lp_end = arc_irq_pop(env, "LP_END");
        env->aux_lp_start = arc_irq_pop(env, "LP_START");
        env->gpr[ARC_REGNUM_LP_COUNT] = arc_irq_pop(env, "LP_COUNT");
    }
#endif

    if (env->aux_irq_ctrl.code_density) {
        env->aux_ei_base = arc_irq_pop(env, "EI_BASE");
        env->aux_ldi_base = arc_irq_pop(env, "LDI_BASE");
        env->aux_jli_base = arc_irq_pop(env, "JLI_BASE");
    }

    env->pc = arc_irq_pop(env, "PC");
    env->gpr[ARC_REGNUM_PCL] = arc_pc_to_pcl(env->pc);
    env->gpr[ARC_REGNUM_ILINK] = env->pc; /* Undocumented behavior! */

    tmp = arc_irq_pop(env, "STATUS32");
    arc_unpack_status32(&new_status, tmp);

    /* The popped value comes from memory, so RB may name a missing bank. */
    new_status.register_bank = arc_cpu_effective_gpr_bank(env, new_status.register_bank);
    arc_cpu_switch_gpr_bank(env, new_status.register_bank);
    env->aux_status32 = new_status;

    /*
     * Late switch back to the user stack when the outermost handler returns to
     * a user thread whose context was saved on the kernel stack.
     */
    if (env->aux_irq_act.active == 0 && arc_in_user_mode(env) && !env->aux_irq_ctrl.user_stack) {
        tmp = env->aux_user_sp;
        env->aux_user_sp = env->gpr[ARC_REGNUM_SP];
        env->gpr[ARC_REGNUM_SP] = tmp;
    }

    /* Reset AUX_IRQ_ACT.U when exiting to User mode */
    if (arc_in_user_mode(env)) {
        env->aux_irq_act.from_user = 0;
    }

    qemu_log_mask(CPU_LOG_INT, "arc: irq: returning to pc=0x" TARGET_FMT_lx "\n", env->pc);
}

void arc_do_rtie_firq(CPUARCState *env)
{
    target_ulong tmp;
    uint32_t rb;

    qemu_log_mask(CPU_LOG_INT, "arc: irq: returning from firq pc=0x" TARGET_FMT_lx "\n", env->pc);

    env->aux_irq_act.active &= ~1u;

    if (env->aux_irq_act.active == 0 && env->aux_irq_act.from_user) {
        tmp = env->aux_user_sp;
        env->aux_user_sp = env->gpr[ARC_REGNUM_R28];
        env->gpr[ARC_REGNUM_R28] = tmp;
        env->aux_irq_act.from_user = 0;
    }

    rb = arc_cpu_effective_gpr_bank(env, env->aux_status32_p0.register_bank);
    env->aux_status32_p0.register_bank = rb;
    arc_cpu_switch_gpr_bank(env, rb);
    env->aux_status32 = env->aux_status32_p0;

    env->pc = env->gpr[ARC_REGNUM_ILINK];
    env->gpr[ARC_REGNUM_PCL] = arc_pc_to_pcl(env->pc);

    qemu_log_mask(CPU_LOG_INT, "arc: irq: returning from firq to pc=0x" TARGET_FMT_lx "\n", env->pc);
}
