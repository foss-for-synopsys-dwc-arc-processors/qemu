#include "qemu/osdep.h"
#include "qemu/main-loop.h"
#include "qemu/timer.h"
#include "qemu/host-utils.h"
#include "qemu/log.h"
#include "hw/irq.h"
#include "cpu.h"
#include "timer.h"

#define arc_timer_enabled(timer) FIELD_EX32(timer, AUX_TIMER_CONTROL, IE)
#define arc_timer_set_pending(timer) FIELD_DP32(timer, AUX_TIMER_CONTROL, IP, 1);

/*
 * An ARC core may be configured with up 2 timers: TIMER0 and TIMER1. Both of
 * them are connected to IRQ 16 and IRQ 17 lines respectively. Timers are
 * connected to a system clock signal which operates even in sleep state.
 *
 * These timers are configured and examined through these AUX registers:
 *
 *     COUNT0, 0x21, RW - the current count value of a timer
 *     CONTROL0, 0x22, RW - controls timer's behavior,
 *     LIMIT0, 0x23, RW - it's the value after which an interrupt
 *                        is generated, 0x00FFFFFF on reset
 *
 * There is the same set of registers for TIMER1:
 *
 *     COUNT1, 0x100, RW
 *     CONTROL1, 0x101, RW
 *     LIMIT1, 0x102, RW
 *
 * When COUNTn reaches LIMITn and CONTROLn.IE == 1, then an interrupt is
 * generated and CONTROLn.IP is set to 1. An interrupt handler must clear
 * CONTROLn.IP bit to reset a timer. Writing to CONTROLn leads to resetting
 * of COUNTn and deasserting of an IRQ line.
 *
 * How to program a timer n:
 *
 *     1. Write 0 to CONTROLn to disable interrupts.
 *     2. Write the limit value to LIMITn.
 *     3. Write 1 to CONTROLn to enable interrupts and reset COUNTn.
 *
 * Implementation details:
 *
 *     1. It's necessary to take a lock through BQL_LOCK_GUARD() because
 *        a QEMU timer callback is called from the different thread
 *        of the main loop with BQL lock.
 *     2. Support of NH, W and TD bits of CONTROLn is not implemented yet.
 *     3. The processor timers are driven by the virtual clock. The
 *        CPU frequency (cfg.freq_hz) is used to convert between counted
 *        clock cycles and nanoseconds.
 *     4. IP=1 only when IE=1 - this is not obvious from PRM
 *
 * TODO: Set IP = 1 immediately when IE == 1 && LIMITn == 0 in arc_timer_rearm()
 */

static uint64_t arc_timer_cycles_to_ns(ARCCPU *cpu, uint64_t cycles)
{
    return muldiv64(cycles, NANOSECONDS_PER_SECOND, cpu->cfg.freq_hz);
}

static uint64_t arc_timer_ns_to_cycles(ARCCPU *cpu, uint64_t ns)
{
    return muldiv64(ns, cpu->cfg.freq_hz, NANOSECONDS_PER_SECOND);
}

/* Current count value of a timer, derived from the elapsed virtual time */
static target_ulong arc_timer_count(CPUARCState *env, unsigned n)
{
    ARCCPU *cpu = env_archcpu(env);
    uint64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
    uint64_t cycles = arc_timer_ns_to_cycles(cpu, now - env->timer_last_clk[n]);

    /* Return limit value if a timer callback is not called yet */
    if (env->aux_timer_limit[n] && cycles > env->aux_timer_limit[n]) {
        cycles = env->aux_timer_limit[n];
    }

    return cycles;
}

/* (Re)arm the host timer to fire when COUNTn reaches LIMITn */
static void arc_timer_rearm(CPUARCState *env, unsigned n)
{
    ARCCPU *cpu = env_archcpu(env);
    uint64_t expire;

    /* The host timer is created by cpu_arc_clock_init() after a CPU reset */
    if (!env->cpu_timer[n]) {
        return;
    }

    if (env->aux_timer_limit[n] == 0) {
        timer_del(env->cpu_timer[n]);
        return;
    }

    expire = env->timer_last_clk[n] + arc_timer_cycles_to_ns(cpu, env->aux_timer_limit[n]);
    timer_mod(env->cpu_timer[n], expire);
}

static void arc_timer_expire(CPUARCState *env, unsigned n)
{
    g_assert(bql_locked());

    qemu_log_mask(CPU_LOG_INT, "arc: timer%u: limit reached\n", n);

    /* Restart counting from 0 */
    env->timer_last_clk[n] = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);

    if (arc_timer_enabled(env->aux_timer_control[n])) {
        /* IP=1 only when IE=1 - this is not obvious from PRM */
        env->aux_timer_control[n] = arc_timer_set_pending(env->aux_timer_control[n]);
        qemu_irq_raise((qemu_irq)env->irq[ARC_TIMER_IRQ(n)]);
    }

    arc_timer_rearm(env, n);
}

static target_ulong arc_timer_count_get(CPUARCState *env, unsigned n)
{
    BQL_LOCK_GUARD();

    return arc_timer_count(env, n);
}

static void arc_timer_count_set(CPUARCState *env, unsigned n, target_ulong value)
{
    ARCCPU *cpu = env_archcpu(env);
    uint64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);

    BQL_LOCK_GUARD();

    /* Anchor the count's zero point so that the current count equals value */
    env->timer_last_clk[n] = now - arc_timer_cycles_to_ns(cpu, value);
    arc_timer_rearm(env, n);
}

static uint32_t arc_timer_control_get(CPUARCState *env, unsigned n)
{
    BQL_LOCK_GUARD();

    return env->aux_timer_control[n];
}

static void arc_timer_control_set(CPUARCState *env, unsigned n, uint32_t value)
{
    BQL_LOCK_GUARD();

    /* Any write to CONTROLn de-asserts the timer interrupt */
    qemu_irq_lower((qemu_irq)env->irq[ARC_TIMER_IRQ(n)]);

    env->aux_timer_control[n] = arc_filter_aux_timer_control(value);

    /* Rearm in case IE changed (check TODO in top comments) */
    arc_timer_rearm(env, n);
}

static target_ulong arc_timer_limit_get(CPUARCState *env, unsigned n)
{
    BQL_LOCK_GUARD();

    return env->aux_timer_limit[n];
}

static void arc_timer_limit_set(CPUARCState *env, unsigned n, target_ulong value)
{
    BQL_LOCK_GUARD();

    env->aux_timer_limit[n] = value;
    arc_timer_rearm(env, n);
}

static void arc_timer0_cb(void *opaque)
{
    arc_timer_expire(&((ARCCPU *)opaque)->env, 0);
}

static void arc_timer1_cb(void *opaque)
{
    arc_timer_expire(&((ARCCPU *)opaque)->env, 1);
}

target_ulong arc_aux_timer0_count_read(CPUARCState *env, uintptr_t retaddr)
{
    return arc_timer_count_get(env, 0);
}

void arc_aux_timer0_count_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_timer_count_set(env, 0, value);
}

target_ulong arc_aux_timer0_control_read(CPUARCState *env, uintptr_t retaddr)
{
    return arc_timer_control_get(env, 0);
}

void arc_aux_timer0_control_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_timer_control_set(env, 0, value);
}

target_ulong arc_aux_timer0_limit_read(CPUARCState *env, uintptr_t retaddr)
{
    return arc_timer_limit_get(env, 0);
}

void arc_aux_timer0_limit_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_timer_limit_set(env, 0, value);
}

target_ulong arc_aux_timer1_count_read(CPUARCState *env, uintptr_t retaddr)
{
    return arc_timer_count_get(env, 1);
}

void arc_aux_timer1_count_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_timer_count_set(env, 1, value);
}

target_ulong arc_aux_timer1_control_read(CPUARCState *env, uintptr_t retaddr)
{
    return arc_timer_control_get(env, 1);
}

void arc_aux_timer1_control_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_timer_control_set(env, 1, value);
}

target_ulong arc_aux_timer1_limit_read(CPUARCState *env, uintptr_t retaddr)
{
    return arc_timer_limit_get(env, 1);
}

void arc_aux_timer1_limit_write(CPUARCState *env, target_ulong value, uintptr_t retaddr)
{
    arc_timer_limit_set(env, 1, value);
}

target_ulong arc_aux_timer_build_read(CPUARCState *env, uintptr_t retaddr)
{
    return env_archcpu(env)->aux_timer_build;
}

void arc_timers_reset(ARCCPU *cpu)
{
    CPUARCState *env = &cpu->env;
    uint64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);

    for (int i = 0; i < ARC_NR_OF_TIMERS; i++) {
        env->aux_timer_control[i] = 0;
        env->aux_timer_limit[i] = ARC_TIMER_LIMIT_RESET;
        env->timer_last_clk[i] = now;
        arc_timer_rearm(env, i);
    }
}

void cpu_arc_clock_init(ARCCPU *cpu)
{
    CPUARCState *env = &cpu->env;
    uint64_t now = qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);

    if (cpu->cfg.has_timer0) {
        env->cpu_timer[0] = timer_new_ns(QEMU_CLOCK_VIRTUAL, arc_timer0_cb, cpu);
        env->timer_last_clk[0] = now;
        arc_timer_rearm(env, 0);
    }

    if (cpu->cfg.has_timer1) {
        env->cpu_timer[1] = timer_new_ns(QEMU_CLOCK_VIRTUAL, arc_timer1_cb, cpu);
        env->timer_last_clk[1] = now;
        arc_timer_rearm(env, 1);
    }
}
