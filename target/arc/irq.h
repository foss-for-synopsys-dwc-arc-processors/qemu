#ifndef ARC_CPU_IRQ_H
#define ARC_CPU_IRQ_H

#include "qemu/osdep.h"
#include "cpu.h"

target_ulong arc_get_irq_priority_pending(CPUARCState *env, bool enabled_irqs_only);
target_ulong arc_load_vector_entry(CPUARCState *env, uint32_t vector);
bool arc_cpu_exec_interrupt(CPUState *cs, int interrupt_request);
void arc_do_rtie_exception(CPUARCState *env);
void arc_irq_push(CPUARCState *env, target_ulong value, const char *str);
target_ulong arc_irq_pop(CPUARCState *env, const char *str);
void arc_do_rtie_interrupt(CPUARCState *env);
void arc_do_rtie_firq(CPUARCState *env);
void arc_enter_irq(ARCCPU *cpu);
void arc_enter_firq(ARCCPU *cpu);

#endif
