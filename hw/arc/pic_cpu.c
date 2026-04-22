#include "qemu/osdep.h"
#include "qemu/main-loop.h"
#include "cpu.h"
#include "irq.h"
#include "hw/hw.h"
#include "hw/irq.h"
#include "qemu/log.h"
#include "hw/arc/cpudevs.h"

static void arc_pic_cpu_handler(void *opaque, int irq, int level)
{
    ARCCPU *cpu = (ARCCPU *) opaque;
    CPUState *cs = CPU(cpu);
    CPUARCState *env = &cpu->env;

    g_assert(bql_locked());

    g_assert(irq >= ARC_FIRST_INTERRUPT_NR && irq <= ARC_LAST_INTERRUPT_NR);

    if (level) {
        env->aux_irq_bank[irq].pending = 1;
        cpu_interrupt(cs, CPU_INTERRUPT_HARD);
    } else {
        env->aux_irq_bank[irq].pending = 0;

        if (arc_get_irq_priority_pending(env, false) == 0) {
            cpu_reset_interrupt(cs, CPU_INTERRUPT_HARD);
        }
    }

    qemu_log_mask(CPU_LOG_INT, "arc: irq: IRQ line asserted: irq=%d level=%d pc=" TARGET_FMT_lx "\n", irq, level, env->pc);
}

void cpu_arc_pic_init(ARCCPU *cpu)
{
    CPUARCState *env = &cpu->env;
    qemu_irq *irqs;

    irqs = qemu_allocate_irqs(arc_pic_cpu_handler, cpu, ARC_MAX_NR_OF_VECTORS);

    for (int i = 0; i < ARC_MAX_NR_OF_VECTORS; i++) {
        env->irq[i] = irqs[i];
    }
}
