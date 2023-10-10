/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2020 Synppsys Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * http://www.gnu.org/licenses/lgpl-2.1.html
 */

#include "qemu/osdep.h"
#include "qemu/timer.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "hw/irq.h"
#include "hw/arc/cpudevs.h"
#include "timer.h"
#include "qemu/main-loop.h"
#include "qemu/log.h"

#define NANOSECONDS_PER_SECOND 1000000000LL
#define TIMER_PERIOD(hz) (1000000000LL / (hz))
#define TIMEOUT_LIMIT 1000000

#define FREQ_HZ (env_archcpu(env)->freq_hz)

#define CYCLES_TO_NS(VAL) (muldiv64(VAL, NANOSECONDS_PER_SECOND, FREQ_HZ))
#define NS_TO_CYCLE(VAL)  (muldiv64(VAL, FREQ_HZ, NANOSECONDS_PER_SECOND))

static uint64_t get_ns(CPUARCState *env)
{
#ifndef CONFIG_USER_ONLY
    return qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL);
#else
    return cpu_get_host_ticks();
#endif
}

uint64_t arc_get_cycles(CPUARCState *env)
{
    uint64_t diff = get_ns(env) - env->rtc.base_time_ns;

    /*
     * In user mode host's cycles are stored in base_time_ns and get_ns()
     * returns host cycles. Thus, return just a difference in this case
     * without converting from nanoseconds to cycles.
     */
#ifndef CONFIG_USER_ONLY
    return NS_TO_CYCLE(diff);
#else
    return diff;
#endif
}

uint64_t arc_get_global_cycles(void)
{
    return arc_get_cycles(&ARC_CPU(first_cpu)->env);
}

static uint32_t get_t_count(CPUARCState *env, uint32_t t)
{
#ifndef CONFIG_USER_ONLY
    return NS_TO_CYCLE(qemu_clock_get_ns(QEMU_CLOCK_VIRTUAL) - env->timer[t].last_clk);
#else
    return cpu_get_host_ticks() - env->timer[t].last_clk;
#endif
}

#define T_COUNT(T) (get_t_count(env, T))

/* Update the next timeout time as difference between Count and Limit */
static void cpu_arc_timer_update(CPUARCState *env, uint32_t timer)
{
    uint32_t delta;
    uint32_t t_count = T_COUNT(timer);
#ifndef CONFIG_USER_ONLY
    uint64_t now = get_ns(env);
#endif

    delta = env->timer[timer].T_Limit - t_count;

#ifndef CONFIG_USER_ONLY
    timer_mod_ns(env->cpu_timer[timer], now + CYCLES_TO_NS((uint64_t)delta));
#endif

    qemu_log_mask(LOG_UNIMP,
                  "[TMR%d] Timer update in 0x" TARGET_FMT_lx
                  " - 0x%08x = 0x%08x (ctrl:0x" TARGET_FMT_lx
                  " @ %d Hz)\n",
                  timer, env->timer[timer].T_Limit,
                  t_count, delta, env->timer[timer].T_Cntrl, FREQ_HZ);
}

#ifndef CONFIG_USER_ONLY
/* Expire the timer function. Rise an interrupt if required. */
static void cpu_arc_timer_expire(CPUARCState *env, uint32_t timer)
{
    assert(timer == 1 || timer == 0);
    qemu_log_mask(LOG_UNIMP, "[TMR%d] Timer expired\n", timer);

    uint32_t overflow = env->timer[timer].T_Cntrl & TMR_IP;
    /* Set the IP bit. */

    bool unlocked = !qemu_mutex_iothread_locked();
    if (unlocked) {
        qemu_mutex_lock_iothread();
    }
    env->timer[timer].T_Cntrl |= TMR_IP;
    env->timer[timer].last_clk = get_ns(env);
    if (unlocked) {
        qemu_mutex_unlock_iothread();
    }

    /* Raise an interrupt if enabled. */
    if ((env->timer[timer].T_Cntrl & TMR_IE) && !overflow) {
        qemu_log_mask(CPU_LOG_INT, "[TMR%d] Rising IRQ\n", timer);
        qemu_irq_raise(env->irq[TIMER0_IRQ + (timer & 0x01)]);
    }
}
#endif

/*
 * This callback should occur when the counter is exactly equal to the
 * limit value. Offset the count by one to avoid immediately
 * retriggering the callback before any virtual time has passed.
 */

#ifndef CONFIG_USER_ONLY
static void arc_timer0_cb(void *opaque)
{
    CPUARCState *env = (CPUARCState *) opaque;

    if (!(env_archcpu(env)->timer_build & TB_T0)) {
        return;
    }

    cpu_arc_timer_expire(env, 0);
    cpu_arc_timer_update(env, 0);
}

/* Like the above function but for TIMER1. */
static void arc_timer1_cb(void *opaque)
{
    CPUARCState *env = (CPUARCState *) opaque;

    if (!(env_archcpu(env)->timer_build & TB_T1)) {
        return;
    }

    cpu_arc_timer_expire(env, 1);
    cpu_arc_timer_update(env, 1);
}
#endif

/* Helper used when resetting the system. */
static void cpu_arc_count_reset(CPUARCState *env, uint32_t timer)
{
    assert(timer == 0 || timer == 1);
    env->timer[timer].T_Cntrl = 0;
    env->timer[timer].T_Limit = 0x00ffffff;
}

/* Get the counter value. */
static uint32_t cpu_arc_count_get(CPUARCState *env, uint32_t timer)
{
    uint32_t count = T_COUNT(timer);
    qemu_log_mask(LOG_UNIMP, "[TMR%d] Timer count %d.\n", timer, count);
    return count;
}

/* Set the counter value. */
static void cpu_arc_count_set(CPUARCState *env, uint32_t timer, uint32_t val)
{
    assert(timer == 0 || timer == 1);
    bool unlocked = !qemu_mutex_iothread_locked();
    if (unlocked) {
        qemu_mutex_lock_iothread();
    }
    env->timer[timer].last_clk = get_ns(env) - CYCLES_TO_NS(val);
    cpu_arc_timer_update(env, timer);
    if (unlocked) {
        qemu_mutex_unlock_iothread();
    }
}

/* Store the counter limit. */
static void cpu_arc_store_limit(CPUARCState *env,
                                uint32_t timer, uint32_t value)
{
    switch (timer) {
    case 0:
        if (!(env_archcpu(env)->timer_build & TB_T0)) {
            return;
        }
        break;
    case 1:
        if (!(env_archcpu(env)->timer_build & TB_T1)) {
            return;
        }
        break;
    default:
        break;
    }
    env->timer[timer].T_Limit = value;
    cpu_arc_timer_update(env, timer);
}

/* Set the timer control bits. */
static void cpu_arc_control_set(CPUARCState *env,
                                uint32_t timer, uint32_t value)
{
    assert(timer == 1 || timer == 0);
    bool unlocked = !qemu_mutex_iothread_locked();
    if (unlocked) {
        qemu_mutex_lock_iothread();
    }
    if ((env->timer[timer].T_Cntrl & TMR_IP) && !(value & TMR_IP)) {
        qemu_irq_lower(env->irq[TIMER0_IRQ + (timer)]);
    }
    env->timer[timer].T_Cntrl = value & 0x1f;
    if (unlocked) {
        qemu_mutex_unlock_iothread();
    }
}

#if defined(TARGET_ARC64)
static uint64_t arc_rtc_count_get_low(CPUARCState *env)
{
    uint8_t enabled = FIELD_EX32(env->rtc.ctrl, ARC_RTC_CTRL, ENABLE);

    if (enabled) {
        return arc_get_cycles(env);
    } else {
        return env->rtc.stop_cycles;
    }
}
#else
static uint32_t arc_rtc_count_get_low(CPUARCState *env)
{
    uint8_t enabled = FIELD_EX32(env->rtc.ctrl, ARC_RTC_CTRL, ENABLE);
    uint64_t cycles;

    if (enabled) {
        cycles = arc_get_cycles(env);
    } else {
        cycles = env->rtc.stop_cycles;
    }

    /*
     * If RTC is enabled then update (high,low) pair with the latest cycles
     * count. Otherwise, don't update it and use the old value.
     */
    env->rtc.low = cycles & 0xFFFFFFFF;
    env->rtc.high = (cycles >> 32) & 0xFFFFFFFF;

    return env->rtc.low;
}

static uint32_t arc_rtc_count_get_high(CPUARCState *env)
{
    return env->rtc.high;
}
#endif

/* Set the RTC control bits. */
static void arc_rtc_ctrl_set(CPUARCState *env, uint32_t val)
{
#ifndef CONFIG_USER_ONLY
    uint8_t enable = FIELD_EX32(val, ARC_RTC_CTRL, ENABLE);
    uint8_t enabled = FIELD_EX32(env->rtc.ctrl, ARC_RTC_CTRL, ENABLE);
    uint8_t clear = FIELD_EX32(val, ARC_RTC_CTRL, CLEAR);

    assert(GET_STATUS_BIT(env->stat, Uf) == 0);

    /* Case: RTC is enabled and it's going to be disabled.
     * Remember stop time and save the latest cycles count for further using.
     */
    if (enabled && !enable) {
        env->rtc.stop_time_ns = get_ns(env);
        env->rtc.stop_cycles = arc_get_cycles(env);
    }

    if (clear) {
        env->rtc.base_time_ns = get_ns(env);

        /*
         * If RTC is stopped then remember stop time - it will allow to reset
         * the counter after reactivating the RTC.
         */
        if (!enabled) {
            env->rtc.stop_time_ns = env->rtc.base_time_ns;
            env->rtc.stop_cycles = 0;
        }
    }

    /*
     * Case: RTC is disabled and it's going to be enabled.
     * Increase base time to fill the gap between stop and now.
     */
    if (!enabled && enable) {
        env->rtc.base_time_ns += get_ns(env) - env->rtc.stop_time_ns;
    }

    /* Always set atomicity bits since. */
    env->rtc.ctrl = FIELD_DP32(env->rtc.ctrl, ARC_RTC_CTRL, ENABLE, enable);
    env->rtc.ctrl = FIELD_DP32(env->rtc.ctrl, ARC_RTC_CTRL, A0, 1);
    env->rtc.ctrl = FIELD_DP32(env->rtc.ctrl, ARC_RTC_CTRL, A1, 1);
#endif
}

/* Init procedure, called in platform. */

void
cpu_arc_clock_init(ARCCPU *cpu)
{
    CPUARCState *env = &cpu->env;

#ifndef CONFIG_USER_ONLY
    if (FIELD_EX32(cpu->timer_build, ARC_TIMER_BUILD, T0)) {
        env->cpu_timer[0] = timer_new_ns(QEMU_CLOCK_VIRTUAL, &arc_timer0_cb, env);
    }

    if (FIELD_EX32(cpu->timer_build, ARC_TIMER_BUILD, T1)) {
        env->cpu_timer[1] = timer_new_ns(QEMU_CLOCK_VIRTUAL, &arc_timer1_cb, env);
    }
#endif

    env->timer[0].last_clk = get_ns(env);
    env->timer[1].last_clk = get_ns(env);
}

/*
 * TODO: Implement setting default interrupt priorities for Timer 0 and Timer 1.
 */

void arc_initializeTIMER(ARCCPU *cpu)
{
    uint32_t build = 0;
    uint8_t version;

    switch (cpu->family) {
    case ARC_OPCODE_ARC64:
        version = 0x7;
        break;
    case ARC_OPCODE_ARC32:
        version = 0x6;
        break;
    default:
        version = 0x4;
    }

    build = FIELD_DP32(build, ARC_TIMER_BUILD, VERSION, version);

    if (cpu->cfg.has_timer_0) {
        build = FIELD_DP32(build, ARC_TIMER_BUILD, T0, 1);
    }

    if (cpu->cfg.has_timer_1) {
        build = FIELD_DP32(build, ARC_TIMER_BUILD, T1, 1);
    }

    if (cpu->cfg.rtc_option) {
        build = FIELD_DP32(build, ARC_TIMER_BUILD, RTC, 1);
    }

    cpu->timer_build = build;
}

void arc_resetTIMER(ARCCPU *cpu)
{
    CPUARCState *env = &cpu->env;

    if (FIELD_EX32(cpu->timer_build, ARC_TIMER_BUILD, T0)) {
        cpu_arc_count_reset(env, 0);
    }

    if (FIELD_EX32(cpu->timer_build, ARC_TIMER_BUILD, T1)) {
        cpu_arc_count_reset(env, 1);
    }
}

/* Function implementation for reading/writing aux regs. */
target_ulong
aux_timer_get(const struct arc_aux_reg_detail *aux_reg_detail, void *data)
{
    CPUARCState *env = (CPUARCState *) data;

    switch (aux_reg_detail->id) {
    case AUX_ID_control0:
        return env->timer[0].T_Cntrl;
        break;

    case AUX_ID_control1:
        return env->timer[1].T_Cntrl;
        break;

    case AUX_ID_count0:
        return cpu_arc_count_get(env, 0);
        break;

    case AUX_ID_count1:
        return cpu_arc_count_get(env, 1);
        break;

    case AUX_ID_limit0:
        return env->timer[0].T_Limit;
        break;

    case AUX_ID_limit1:
        return env->timer[1].T_Limit;
        break;

    case AUX_ID_timer_build:
        return env_archcpu(env)->timer_build;
        break;

    case AUX_ID_aux_rtc_low:
        return arc_rtc_count_get_low(env);
        break;

#if !defined(TARGET_ARC64)
    /* AUX_RTC_HIGH register is not presented on ARC HS6x processors. */
    case AUX_ID_aux_rtc_high:
        return arc_rtc_count_get_high(env);
        break;
#endif

    case AUX_ID_aux_rtc_ctrl:
        return env->rtc.ctrl;
        break;

    default:
        break;
    }
    return 0;
}

void aux_timer_set(const struct arc_aux_reg_detail *aux_reg_detail,
                   target_ulong val, void *data)
{
    CPUARCState *env = (CPUARCState *) data;

    qemu_log_mask(LOG_UNIMP, "[TMRx] AUX[%s] <= 0x" TARGET_FMT_lx "\n",
                  aux_reg_detail->name, val);

    qemu_mutex_lock_iothread();
    switch (aux_reg_detail->id) {
    case AUX_ID_control0:
        if (env_archcpu(env)->timer_build & TB_T0) {
            cpu_arc_control_set(env, 0, val);
        }
        break;

    case AUX_ID_control1:
        if (env_archcpu(env)->timer_build & TB_T1) {
            cpu_arc_control_set(env, 1, val);
        }
        break;

    case AUX_ID_count0:
        if (env_archcpu(env)->timer_build & TB_T0) {
            cpu_arc_count_set(env, 0, val);
        }
        break;

    case AUX_ID_count1:
        if (env_archcpu(env)->timer_build & TB_T1) {
            cpu_arc_count_set(env, 1, val);
        }
        break;

    case AUX_ID_limit0:
        cpu_arc_store_limit(env, 0, val);
        break;

    case AUX_ID_limit1:
        cpu_arc_store_limit(env, 1, val);
        break;

    case AUX_ID_aux_rtc_ctrl:
        arc_rtc_ctrl_set(env, val);
        break;

    default:
        break;
    }
    qemu_mutex_unlock_iothread();
}


/*-*-indent-tabs-mode:nil;tab-width:4;indent-line-function:'insert-tab'-*-*/
/* vim: set ts=4 sw=4 et: */
