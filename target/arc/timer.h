#ifndef ARC_CPU_TIMER_H
#define ARC_CPU_TIMER_H

#include "qemu/osdep.h"
#include "hw/registerfields.h"
#include "cpu.h"

#define ARC_TIMER_IRQ(n) (16 + (n))
#define ARC_TIMER_LIMIT_RESET 0x00ffffff

void cpu_arc_clock_init(ARCCPU *cpu);
void arc_timers_reset(ARCCPU *cpu);

target_ulong arc_aux_timer0_count_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_timer0_count_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_timer0_control_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_timer0_control_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_timer0_limit_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_timer0_limit_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);

target_ulong arc_aux_timer1_count_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_timer1_count_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_timer1_control_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_timer1_control_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);
target_ulong arc_aux_timer1_limit_read(CPUARCState *env, uintptr_t retaddr);
void arc_aux_timer1_limit_write(CPUARCState *env, target_ulong value, uintptr_t retaddr);

target_ulong arc_aux_timer_build_read(CPUARCState *env, uintptr_t retaddr);

#endif /* ARC_TIMER_H */
