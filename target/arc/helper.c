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

#include "cpu.h"
#include "hw/irq.h"
#include "include/hw/sysbus.h"
#include "include/sysemu/sysemu.h"
#include "qemu/qemu-print.h"
#include "exec/exec-all.h"
#include "exec/cpu_ldst.h"
#include "qemu/host-utils.h"
#include "exec/helper-proto.h"
#include "irq.h"

void arc_cpu_do_interrupt(CPUState *cs)
{
    ARCCPU      *cpu    = ARC_CPU(cs);
    CPUARCState *env    = &cpu->env;
    uint32_t     offset = 0;
    uint32_t     vectno;
    const char  *name;
    MemTxResult txres;

    /*
     * NOTE: Special LP_END exception. Immediately return code execution to
     * lp_start.
     * Now also used for delayslot MissI cases.
     * This special exception should not execute any of the exception
     * handling code. Instead it returns immediately after setting PC to the
     * address passed as exception parameter.
     */
    if (cs->exception_index == EXCP_LPEND_REACHED
        || cs->exception_index == EXCP_FAKE) {
        env->pc = env->param;
        CPU_PCL(env) = env->pc & 0xfffffffe;
        return;
    }

    /* If we take an exception within an exception => fatal Machine Check. */
    if (GET_STATUS_BIT(env->stat, AEf) == 1) {
        cs->exception_index = EXCP_MACHINE_CHECK;
        env->causecode = 0;
        env->param = 0;
        arc_mmu_disable(env);
        env->mpu.enabled = false;     /* no more MPU */
    }
    vectno = cs->exception_index & 0x0F;
    offset = OFFSET_FOR_VECTOR(vectno);

    /* Generic computation for exceptions. */
    switch (cs->exception_index) {
    case EXCP_RESET:
        name = "Reset";
        break;
    case EXCP_MEMORY_ERROR:
        name = "Memory Error";
        break;
    case EXCP_INST_ERROR:
        name = "Instruction Error";
        break;
    case EXCP_MACHINE_CHECK:
        name = "Machine Check";
        break;
#ifdef TARGET_ARCV2
    case EXCP_TLB_MISS_I:
        name = "TLB Miss Instruction";
        break;
    case EXCP_TLB_MISS_D:
        name = "TLB Miss Data";
        break;
#elif TARGET_ARCV3
    case EXCP_IMMU_FAULT:
        name = "Instruction MMU Fault";
        break;
    case EXCP_DMMU_FAULT:
        name = "Data MMU Fault";
        break;
#else
#error
#endif
    case EXCP_PROTV:
        name = "Protection Violation";
        break;
    case EXCP_PRIVILEGEV:
        name = "Privilege Violation";
        break;
    case EXCP_SWI:
        name = "SWI";
        break;
    case EXCP_TRAP:
        name = "Trap";
        break;
    case EXCP_EXTENSION:
        name = "Extension";
        break;
    case EXCP_DIVZERO:
        name = "DIV by Zero";
        break;
    case EXCP_DCERROR:
        name = "DCError";
        break;
    case EXCP_MISALIGNED:
        name = "Misaligned";
        break;
    case EXCP_IRQ:
    default:
        cpu_abort(cs, "unhandled exception/irq type=%d\n",
                  cs->exception_index);
        break;
    }

    qemu_log_mask(CPU_LOG_INT, "[EXCP] exception %d (%s) at pc=0x"
                  TARGET_FMT_lx "\n",
                  cs->exception_index, name, env->pc);

    /*
     * 3. exception status register is loaded with the contents
     * of STATUS32.
     */
    env->stat_er = env->stat;

    /* 4. exception return branch target address register. */
    env->erbta = env->bta;

    /*
     * 5. eception cause register is loaded with a code to indicate
     * the cause of the exception.
     */
    env->ecr = (vectno & 0xFF) << 16;
    env->ecr |= (env->causecode & 0xFF) << 8;
    env->ecr |= (env->param & 0xFF);

    /* 6. Set the EFA if available. */
    if (cpu->cfg.has_mmu || cpu->cfg.has_mpu) {
        switch (cs->exception_index) {
        case EXCP_DCERROR:
        case EXCP_DIVZERO:
        case EXCP_EXTENSION:
        case EXCP_TRAP:
        case EXCP_SWI:
        case EXCP_PRIVILEGEV:
        case EXCP_MACHINE_CHECK:
        case EXCP_INST_ERROR:
        case EXCP_RESET:
            /* TODO: this should move to the place raising the exception */
            env->efa  = env->pc;
            break;
        default:
            break;
        }
    }

    /* 7. CPU is switched to kernel mode. */
    SET_STATUS_BIT(env->stat, Uf, 0);

    if (GET_STATUS_BIT(env->stat_er, Uf)) {
        switchSP(env);
    }

    /* 8. Interrupts are disabled. */
    env->stat.IEf = 0;

    /* 9. The active exception flag is set. */
    SET_STATUS_BIT(env->stat, AEf, 1);

    /* 10-14. Other flags sets. */
    env->stat.Zf  = GET_STATUS_BIT(env->stat_er, Uf);
    SET_STATUS_BIT(env->stat, Lf, 1);
    env->stat.DEf = 0;
    SET_STATUS_BIT(env->stat, ESf, 0);
    SET_STATUS_BIT(env->stat, DZf, 0);
    SET_STATUS_BIT(env->stat, SCf, 0);

    /* 15. The PC is set with the appropriate exception vector. */
    env->pc = address_space_ldl(cs->as, env->intvec + offset,
                                MEMTXATTRS_UNSPECIFIED, &txres);
    assert(txres == MEMTX_OK);
    CPU_PCL(env) = env->pc & 0xfffffffe;

    qemu_log_mask(CPU_LOG_INT, "[EXCP] isr=0x" TARGET_FMT_lx
                  " vec=0x%x ecr=0x" TARGET_FMT_lx "\n",
                  env->pc, offset, env->ecr);

    /* Make sure that exception code decodes corectly */
    env->stat.is_delay_slot_instruction = 0;

    cs->exception_index = -1;
}

static gint arc_cpu_list_compare(gconstpointer a, gconstpointer b)
{
    ObjectClass *class_a = (ObjectClass *)a;
    ObjectClass *class_b = (ObjectClass *)b;
    const char *name_a;
    const char *name_b;

    name_a = object_class_get_name(class_a);
    name_b = object_class_get_name(class_b);
    if (strcmp(name_a, "any-" TYPE_ARC_CPU) == 0) {
        return 1;
    } else if (strcmp(name_b, "any-" TYPE_ARC_CPU) == 0) {
        return -1;
    } else {
        return strcmp(name_a, name_b);
    }
}

static void arc_cpu_list_entry(gpointer data, gpointer user_data)
{
    ObjectClass *oc = data;
    const char *typename;
    char *name;

    typename = object_class_get_name(oc);
    name = g_strndup(typename, strlen(typename) - strlen("-" TYPE_ARC_CPU));
    qemu_printf("  %s\n", name);
    g_free(name);
}

void arc_cpu_list(void)
{
    GSList *list;

    list = object_class_get_list(TYPE_ARC_CPU, false);
    list = g_slist_sort(list, arc_cpu_list_compare);
    qemu_printf("Available CPUs:\n");
    g_slist_foreach(list, arc_cpu_list_entry, NULL);
    g_slist_free(list);
}

int arc_cpu_memory_rw_debug(CPUState *cs, vaddr addr, uint8_t *buf,
                            int len, bool is_write)
{
    return cpu_memory_rw_debug(cs, addr, buf, len, is_write);
}

hwaddr arc_cpu_get_phys_page_debug(CPUState *cs, vaddr addr)
{
    ARCCPU *cpu = ARC_CPU(cs);
    CPUARCState *env = &cpu->env;

    return arc_mmu_debug_translate(env, addr);
}

void helper_debug(CPUARCState *env)
{
   CPUState *cs = env_cpu(env);

   cs->exception_index = EXCP_DEBUG;
   cpu_loop_exit(cs);
}

/*
 * raises a simple exception with causecode and parameter set to 0.
 * it also considers "pc" as the exception return address. this is
 * not true for a software trap.
 * it is very important that "env->host_pc" holds the recent value,
 * else the cpu_restore_state() will not be helpful and we end up
 * with incorrect registers in env.
 */
void QEMU_NORETURN arc_raise_exception(CPUARCState *env, int32_t excp_idx)
{
    CPUState *cs = env_cpu(env);
    cpu_restore_state(cs, env->host_pc, true);
    cs->exception_index = excp_idx;
    env->causecode = env->param = 0x0;
    env->eret  = env->pc;
    env->erbta = env->bta;
    cpu_loop_exit(cs);
}


/* vim: set ts=4 sw=4 et: */
