/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2022 Synppsys Inc.
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
#include "exec/gdbstub.h"
#include "arc-common.h"
#include "target/arc/regs.h"
#include "irq.h"
#include "gdbstub.h"
#include "mpu.h"
#include "exec/helper-proto.h"

/* gets the register address for a particular processor */
#define REG_ADDR(reg, processor_type) \
    arc_aux_reg_address_for((reg), (processor_type))

/*
 * Select the correct bit length for handling data in common codes for
 * arcv3_64 and arcv3_32.
 */
#ifdef TARGET_ARC64
  #define ARCV3_LOAD_MEM(m)  ldq_p(m)
  #define ARCV3_GET_REG(m,r) gdb_get_reg64(m,r)
#else
  #define ARCV3_LOAD_MEM(m)  ldl_p(m)
  #define ARCV3_GET_REG(m,r) gdb_get_reg32(m,r)
#endif

int gdb_v2_core_read(CPUState *cs, GByteArray *mem_buf, int n)
{
    ARCCPU *cpu = ARC_CPU(cs);
    CPUARCState *env = &cpu->env;
    target_ulong regval = 0;

    switch (n) {
    case 0 ... 31:
       regval = env->r[n];
       break;
    case V2_CORE_R58:
       regval = env->r[58];
       break;
    case V2_CORE_R59:
       regval = env->r[59];
       break;
    case V2_CORE_R60:
       regval = env->r[60];
       break;
    case V2_CORE_R63:
       regval = env->r[63];
       break;
    default:
       assert(!"Unsupported register is being read.");
    }

    return gdb_get_reg32(mem_buf, regval);
}

int gdb_v2_core_write(CPUState *cs, uint8_t *mem_buf, int n)
{
    ARCCPU *cpu = ARC_CPU(cs);
    CPUARCState *env = &cpu->env;
    target_ulong regval = ldl_p(mem_buf);

    switch (n) {
    case 0 ... 31:
        env->r[n] = regval;
        break;
    case V2_CORE_R58:
        env->r[58] = regval;
        break;
    case V2_CORE_R59:
        env->r[59] = regval;
        break;
    case V2_CORE_R60:
        env->r[60] = regval;
        break;
    case V2_CORE_R63:
        env->r[63] = regval;
        break;
    default:
        assert(!"Unsupported register is being written.");
    }

    return sizeof(regval);
}

static int
gdb_v2_aux_read(CPUARCState *env, GByteArray *mem_buf, int regnum)
{
    ARCCPU *cpu = env_archcpu(env);
    target_ulong regval = 0;

    switch (regnum) {
    case V2_AUX_PC:
        regval = env->pc;
        break;
    case V2_AUX_LPS:
        regval = helper_lr(env, REG_ADDR(AUX_ID_lp_start, cpu->family));
        break;
    case V2_AUX_LPE:
        regval = helper_lr(env, REG_ADDR(AUX_ID_lp_end, cpu->family));
        break;
    case V2_AUX_STATUS:
        regval = pack_status32(&env->stat);
        break;
    case V2_AUX_TIMER_BUILD:
        regval = helper_lr(env, REG_ADDR(AUX_ID_timer_build, cpu->family));
        break;
    case V2_AUX_IRQ_BUILD:
        regval = helper_lr(env, REG_ADDR(AUX_ID_irq_build, cpu->family));
        break;
    case V2_AUX_MPY_BUILD:
        regval = helper_lr(env, REG_ADDR(AUX_ID_mpy_build, cpu->family));
        break;
    case V2_AUX_VECBASE_BUILD:
        regval = cpu->vecbase_build;
        break;
    case V2_AUX_ISA_CONFIG:
        regval = cpu->isa_config;
        break;
    case V2_AUX_TIMER_CNT0:
        regval = helper_lr(env, REG_ADDR(AUX_ID_count0, cpu->family));
        break;
    case V2_AUX_TIMER_CTRL0:
        regval = helper_lr(env, REG_ADDR(AUX_ID_control0, cpu->family));
        break;
    case V2_AUX_TIMER_LIM0:
        regval = helper_lr(env, REG_ADDR(AUX_ID_limit0, cpu->family));
        break;
    case V2_AUX_TIMER_CNT1:
        regval = helper_lr(env, REG_ADDR(AUX_ID_count1, cpu->family));
        break;
    case V2_AUX_TIMER_CTRL1:
        regval = helper_lr(env, REG_ADDR(AUX_ID_control1, cpu->family));
        break;
    case V2_AUX_TIMER_LIM1:
        regval = helper_lr(env, REG_ADDR(AUX_ID_limit1, cpu->family));
        break;
    /* MMUv4 */
    case V2_AUX_PID:
        regval = helper_lr(env, REG_ADDR(AUX_ID_pid, cpu->family));
        break;
    case V2_AUX_TLBPD0:
        regval = helper_lr(env, REG_ADDR(AUX_ID_tlbpd0, cpu->family));
        break;
    case V2_AUX_TLBPD1:
        regval = helper_lr(env, REG_ADDR(AUX_ID_tlbpd1, cpu->family));
        break;
    case V2_AUX_TLB_INDEX:
        regval = helper_lr(env, REG_ADDR(AUX_ID_tlbindex, cpu->family));
        break;
    case V2_AUX_TLB_CMD:
        regval = helper_lr(env, REG_ADDR(AUX_ID_tlbcommand, cpu->family));
        break;
    /* MPU */
    case V2_AUX_MPU_BUILD:
        regval = helper_lr(env, REG_ADDR(AUX_ID_mpu_build, cpu->family));
        break;
    case V2_AUX_MPU_EN:
        regval = helper_lr(env, REG_ADDR(AUX_ID_mpuen, cpu->family));
        break;
    case V2_AUX_MPU_ECR:
        regval = helper_lr(env, REG_ADDR(AUX_ID_mpuic, cpu->family));
        break;
    case V2_AUX_MPU_BASE0 ... V2_AUX_MPU_BASE15: {
        const uint8_t index = regnum - V2_AUX_MPU_BASE0;
        if (arc_mpu_is_rgn_reg_available(env, index)) {
            regval =
                helper_lr(env, REG_ADDR(AUX_ID_mpurdb0 + index, cpu->family));
        } else {
            regval = 0;
        }
        break;
    }
    case V2_AUX_MPU_PERM0 ... V2_AUX_MPU_PERM15: {
        const uint8_t index = regnum - V2_AUX_MPU_PERM0;
        if (arc_mpu_is_rgn_reg_available(env, index)) {
            regval =
                helper_lr(env, REG_ADDR(AUX_ID_mpurdp0 + index, cpu->family));
        } else {
            regval = 0;
        }
        break;
    }
    /* exceptions */
    case V2_AUX_ERSTATUS:
        regval = helper_lr(env, REG_ADDR(AUX_ID_erstatus, cpu->family));
        break;
    case V2_AUX_ERBTA:
        regval = helper_lr(env, REG_ADDR(AUX_ID_erbta, cpu->family));
        break;
    case V2_AUX_ECR:
        regval = helper_lr(env, REG_ADDR(AUX_ID_ecr, cpu->family));
        break;
    case V2_AUX_ERET:
        regval = helper_lr(env, REG_ADDR(AUX_ID_eret, cpu->family));
        break;
    case V2_AUX_EFA:
        regval = helper_lr(env, REG_ADDR(AUX_ID_efa, cpu->family));
        break;
    /* interrupt */
    case V2_AUX_ICAUSE:
        regval = helper_lr(env, REG_ADDR(AUX_ID_icause, cpu->family));
        break;
    case V2_AUX_IRQ_CTRL:
        regval = helper_lr(env, REG_ADDR(AUX_ID_aux_irq_ctrl, cpu->family));
        break;
    case V2_AUX_IRQ_ACT:
        regval = helper_lr(env, REG_ADDR(AUX_ID_aux_irq_act, cpu->family));
        break;
    case V2_AUX_IRQ_PRIO_PEND:
        regval = env->irq_priority_pending;
        break;
    case V2_AUX_IRQ_HINT:
        regval = helper_lr(env, REG_ADDR(AUX_ID_aux_irq_hint, cpu->family));
        break;
    case V2_AUX_IRQ_SELECT:
        regval = helper_lr(env, REG_ADDR(AUX_ID_irq_select, cpu->family));
        break;
    case V2_AUX_IRQ_ENABLE:
        regval = env->irq_bank[env->irq_select & 0xff].enable;
        break;
    case V2_AUX_IRQ_TRIGGER:
        regval = helper_lr(env, REG_ADDR(AUX_ID_irq_trigger, cpu->family));
        break;
    case V2_AUX_IRQ_STATUS:
        regval = helper_lr(env, REG_ADDR(AUX_ID_irq_status, cpu->family));
        break;
    case V2_AUX_IRQ_PULSE:
        regval = 0; /* write only for clearing the pulse triggered interrupt */
        break;
    case V2_AUX_IRQ_PENDING:
        regval = helper_lr(env, REG_ADDR(AUX_ID_irq_pending, cpu->family));
        break;
    case V2_AUX_IRQ_PRIO:
        regval = helper_lr(env, REG_ADDR(AUX_ID_irq_priority, cpu->family));
        break;
    case V2_AUX_BTA:
        regval = helper_lr(env, REG_ADDR(AUX_ID_bta, cpu->family));
        break;
    default:
        assert(!"Unsupported auxiliary register is being read.");
    }

    return gdb_get_reg32(mem_buf, regval);
}

static int
gdb_v2_aux_write(CPUARCState *env, uint8_t *mem_buf, int regnum)
{
    ARCCPU *cpu = env_archcpu(env);
    target_ulong regval = ldl_p(mem_buf);

    switch (regnum) {
    case V2_AUX_PC:
        env->pc = regval;
        break;
    case V2_AUX_LPS:
        helper_sr(env, regval, REG_ADDR(AUX_ID_lp_start, cpu->family));
        break;
    case V2_AUX_LPE:
        helper_sr(env, regval, REG_ADDR(AUX_ID_lp_end, cpu->family));
        break;
    case V2_AUX_STATUS:
        unpack_status32(&env->stat, regval);
        break;
    case V2_AUX_TIMER_BUILD:
    case V2_AUX_IRQ_BUILD:
    case V2_AUX_MPY_BUILD:
    case V2_AUX_MPU_BUILD:
    case V2_AUX_MPU_ECR:
    case V2_AUX_IRQ_PENDING:
    case V2_AUX_VECBASE_BUILD:
    case V2_AUX_ISA_CONFIG:
    case V2_AUX_ICAUSE:
    case V2_AUX_IRQ_PRIO_PEND:
    case V2_AUX_IRQ_STATUS:
        /* builds/configs/exceptions/irqs cannot be changed */
        break;
    case V2_AUX_TIMER_CNT0:
        helper_sr(env, regval, REG_ADDR(AUX_ID_count0, cpu->family));
        break;
    case V2_AUX_TIMER_CTRL0:
        helper_sr(env, regval, REG_ADDR(AUX_ID_control0, cpu->family));
        break;
    case V2_AUX_TIMER_LIM0:
        helper_sr(env, regval, REG_ADDR(AUX_ID_limit0, cpu->family));
        break;
    case V2_AUX_TIMER_CNT1:
        helper_sr(env, regval, REG_ADDR(AUX_ID_count1, cpu->family));
        break;
    case V2_AUX_TIMER_CTRL1:
        helper_sr(env, regval, REG_ADDR(AUX_ID_control1, cpu->family));
        break;
    case V2_AUX_TIMER_LIM1:
        helper_sr(env, regval, REG_ADDR(AUX_ID_limit1, cpu->family));
        break;
    /* MMUv4 */
    case V2_AUX_PID:
        helper_sr(env, regval, REG_ADDR(AUX_ID_pid, cpu->family));
        break;
    case V2_AUX_TLBPD0:
        helper_sr(env, regval, REG_ADDR(AUX_ID_tlbpd0, cpu->family));
        break;
    case V2_AUX_TLBPD1:
        helper_sr(env, regval, REG_ADDR(AUX_ID_tlbpd1, cpu->family));
        break;
    case V2_AUX_TLB_INDEX:
        helper_sr(env, regval, REG_ADDR(AUX_ID_tlbindex, cpu->family));
        break;
    case V2_AUX_TLB_CMD:
        helper_sr(env, regval, REG_ADDR(AUX_ID_tlbcommand, cpu->family));
        break;
    /* MPU */
    case V2_AUX_MPU_EN:
        helper_sr(env, regval, REG_ADDR(AUX_ID_mpuen, cpu->family));
        break;
    case V2_AUX_MPU_BASE0 ... V2_AUX_MPU_BASE15: {
        const uint8_t index = regnum - V2_AUX_MPU_BASE0;
        if (arc_mpu_is_rgn_reg_available(env, index)) {
            helper_sr(env, regval, REG_ADDR(AUX_ID_mpurdb0 + index, cpu->family));
        } else {
            return 0;
        }
        break;
    }
    case V2_AUX_MPU_PERM0 ... V2_AUX_MPU_PERM15: {
        const uint8_t index = regnum - V2_AUX_MPU_PERM0;
        if (arc_mpu_is_rgn_reg_available(env, index)) {
            helper_sr(env, regval, REG_ADDR(AUX_ID_mpurdp0 + index, cpu->family));
        } else {
            return 0;
        }
        break;
    }
    /* exceptions */
    case V2_AUX_ERSTATUS:
        helper_sr(env, regval, REG_ADDR(AUX_ID_erstatus, cpu->family));
        break;
    case V2_AUX_ERBTA:
        helper_sr(env, regval, REG_ADDR(AUX_ID_erbta, cpu->family));
        break;
    case V2_AUX_ECR:
        helper_sr(env, regval, REG_ADDR(AUX_ID_ecr, cpu->family));
        break;
    case V2_AUX_ERET:
        helper_sr(env, regval, REG_ADDR(AUX_ID_eret, cpu->family));
        break;
    case V2_AUX_EFA:
        helper_sr(env, regval, REG_ADDR(AUX_ID_efa, cpu->family));
        break;
    /* interrupt */
    case V2_AUX_IRQ_CTRL:
        helper_sr(env, regval, REG_ADDR(AUX_ID_aux_irq_ctrl, cpu->family));
        break;
    case V2_AUX_IRQ_ACT:
        helper_sr(env, regval, REG_ADDR(AUX_ID_aux_irq_act, cpu->family));
        break;
    case V2_AUX_IRQ_HINT:
        helper_sr(env, regval, REG_ADDR(AUX_ID_aux_irq_hint, cpu->family));
        break;
    case V2_AUX_IRQ_SELECT:
        helper_sr(env, regval, REG_ADDR(AUX_ID_irq_select, cpu->family));
        break;
    case V2_AUX_IRQ_ENABLE:
        helper_sr(env, regval, REG_ADDR(AUX_ID_irq_enable, cpu->family));
        break;
    case V2_AUX_IRQ_TRIGGER:
        helper_sr(env, regval, REG_ADDR(AUX_ID_irq_trigger, cpu->family));
        break;
    case V2_AUX_IRQ_PULSE:
        helper_sr(env, regval, REG_ADDR(AUX_ID_irq_pulse_cancel, cpu->family));
        break;
    case V2_AUX_IRQ_PRIO:
        helper_sr(env, regval, REG_ADDR(AUX_ID_irq_priority, cpu->family));
        break;
    case V2_AUX_BTA:
        helper_sr(env, regval, REG_ADDR(AUX_ID_bta, cpu->family));
        break;
    default:
        assert(!"Unsupported auxiliary register is being written.");
    }

    return sizeof(regval);
}

/* End of ARCv2 */

int gdb_v3_core_read(CPUState *cs, GByteArray *mem_buf, int n)
{
    ARCCPU *cpu = ARC_CPU(cs);
    CPUARCState *env = &cpu->env;
    target_ulong regval = 0;

    switch (n) {
    case 0 ... 31:
       regval = env->r[n];
       break;
    case V3_CORE_R58:
       regval = env->r[58];
       break;
    case V3_CORE_R63:
       regval = env->r[63];
       break;
    default:
       assert(!"Unsupported register is being read.");
    }

    return ARCV3_GET_REG(mem_buf, regval);
}

int gdb_v3_core_write(CPUState *cs, uint8_t *mem_buf, int n)
{
    ARCCPU *cpu = ARC_CPU(cs);
    CPUARCState *env = &cpu->env;
    target_ulong regval = ARCV3_LOAD_MEM(mem_buf);

    switch (n) {
    case 0 ... 31:
        env->r[n] = regval;
        break;
    case V3_CORE_R58:
        env->r[58] = regval;
        break;
    case V3_CORE_R63:
        env->r[63] = regval;
        break;
    default:
        assert(!"Unsupported register is being written.");
    }

    return sizeof(regval);
}

static int
gdb_v3_aux_read(CPUARCState *env, GByteArray *mem_buf, int regnum)
{
    ARCCPU *cpu = env_archcpu(env);
    target_ulong regval = 0;

    switch (regnum) {
    case V3_AUX_PC:
        regval = env->pc;
        break;
    case V3_AUX_STATUS:
        regval = pack_status32(&env->stat);
        break;
    case V3_AUX_TIMER_BUILD:
        regval = helper_lr(env, REG_ADDR(AUX_ID_timer_build, cpu->family));
        break;
    case V3_AUX_IRQ_BUILD:
        regval = helper_lr(env, REG_ADDR(AUX_ID_irq_build, cpu->family));
        break;
    case V3_AUX_VECBASE_BUILD:
        regval = cpu->vecbase_build;
        break;
    case V3_AUX_ISA_CONFIG:
        regval = cpu->isa_config;
        break;
    case V3_AUX_TIMER_CNT0:
        regval = helper_lr(env, REG_ADDR(AUX_ID_count0, cpu->family));
        break;
    case V3_AUX_TIMER_CTRL0:
        regval = helper_lr(env, REG_ADDR(AUX_ID_control0, cpu->family));
        break;
    case V3_AUX_TIMER_LIM0:
        regval = helper_lr(env, REG_ADDR(AUX_ID_limit0, cpu->family));
        break;
    case V3_AUX_TIMER_CNT1:
        regval = helper_lr(env, REG_ADDR(AUX_ID_count1, cpu->family));
        break;
    case V3_AUX_TIMER_CTRL1:
        regval = helper_lr(env, REG_ADDR(AUX_ID_control1, cpu->family));
        break;
    case V3_AUX_TIMER_LIM1:
        regval = helper_lr(env, REG_ADDR(AUX_ID_limit1, cpu->family));
        break;
    /* exceptions */
    case V3_AUX_ERSTATUS:
        regval = helper_lr(env, REG_ADDR(AUX_ID_erstatus, cpu->family));
        break;
    case V3_AUX_ERBTA:
        regval = helper_lr(env, REG_ADDR(AUX_ID_erbta, cpu->family));
        break;
    case V3_AUX_ECR:
        regval = helper_lr(env, REG_ADDR(AUX_ID_ecr, cpu->family));
        break;
    case V3_AUX_ERET:
        regval = helper_lr(env, REG_ADDR(AUX_ID_eret, cpu->family));
        break;
    case V3_AUX_EFA:
        regval = helper_lr(env, REG_ADDR(AUX_ID_efa, cpu->family));
        break;
    /* interrupt */
    case V3_AUX_ICAUSE:
        regval = helper_lr(env, REG_ADDR(AUX_ID_icause, cpu->family));
        break;
    case V3_AUX_IRQ_CTRL:
        regval = helper_lr(env, REG_ADDR(AUX_ID_aux_irq_ctrl, cpu->family));
        break;
    case V3_AUX_IRQ_ACT:
        regval = helper_lr(env, REG_ADDR(AUX_ID_aux_irq_act, cpu->family));
        break;
    case V3_AUX_IRQ_PRIO_PEND:
        regval = env->irq_priority_pending;
        break;
    case V3_AUX_IRQ_HINT:
        regval = helper_lr(env, REG_ADDR(AUX_ID_aux_irq_hint, cpu->family));
        break;
    case V3_AUX_IRQ_SELECT:
        regval = helper_lr(env, REG_ADDR(AUX_ID_irq_select, cpu->family));
        break;
    case V3_AUX_IRQ_ENABLE:
        regval = env->irq_bank[env->irq_select & 0xff].enable;
        break;
    case V3_AUX_IRQ_TRIGGER:
        regval = helper_lr(env, REG_ADDR(AUX_ID_irq_trigger, cpu->family));
        break;
    case V3_AUX_IRQ_STATUS:
        regval = helper_lr(env, REG_ADDR(AUX_ID_irq_status, cpu->family));
        break;
    case V3_AUX_IRQ_PULSE:
        regval = 0; /* write only for clearing the pulse triggered interrupt */
        break;
    case V3_AUX_IRQ_PRIO:
        regval = helper_lr(env, REG_ADDR(AUX_ID_irq_priority, cpu->family));
        break;
    case V3_AUX_BTA:
        regval = helper_lr(env, REG_ADDR(AUX_ID_bta, cpu->family));
        break;
    /* MMUv6 */
    case V3_AUX_MMU_CTRL:
        regval = helper_lr(env, REG_ADDR(AUX_ID_mmu_ctrl, cpu->family));
        break;
    case V3_AUX_RTP0:
        regval = helper_lr(env, REG_ADDR(AUX_ID_mmu_rtp0, cpu->family));
        break;
    case V3_AUX_RTP1:
        regval = helper_lr(env, REG_ADDR(AUX_ID_mmu_rtp1, cpu->family));
        break;
    default:
        assert(!"Unsupported auxiliary register is being read.");
    }

    return ARCV3_GET_REG(mem_buf, regval);
}

static int
gdb_v3_aux_write(CPUARCState *env, uint8_t *mem_buf, int regnum)
{
    ARCCPU *cpu = env_archcpu(env);
    target_ulong regval = ARCV3_LOAD_MEM(mem_buf);

    switch (regnum) {
    case V3_AUX_PC:
        env->pc = regval;
        break;
    case V3_AUX_STATUS:
        unpack_status32(&env->stat, regval);
        break;
    case V3_AUX_TIMER_BUILD:
    case V3_AUX_IRQ_BUILD:
    case V3_AUX_VECBASE_BUILD:
    case V3_AUX_ISA_CONFIG:
    case V3_AUX_ICAUSE:
    case V3_AUX_IRQ_PRIO_PEND:
    case V3_AUX_IRQ_STATUS:
        /* builds/configs/exceptions/irqs cannot be changed */
        break;
    case V3_AUX_TIMER_CNT0:
        helper_sr(env, regval, REG_ADDR(AUX_ID_count0, cpu->family));
        break;
    case V3_AUX_TIMER_CTRL0:
        helper_sr(env, regval, REG_ADDR(AUX_ID_control0, cpu->family));
        break;
    case V3_AUX_TIMER_LIM0:
        helper_sr(env, regval, REG_ADDR(AUX_ID_limit0, cpu->family));
        break;
    case V3_AUX_TIMER_CNT1:
        helper_sr(env, regval, REG_ADDR(AUX_ID_count1, cpu->family));
        break;
    case V3_AUX_TIMER_CTRL1:
        helper_sr(env, regval, REG_ADDR(AUX_ID_control1, cpu->family));
        break;
    case V3_AUX_TIMER_LIM1:
        helper_sr(env, regval, REG_ADDR(AUX_ID_limit1, cpu->family));
        break;
    /* exceptions */
    case V3_AUX_ERSTATUS:
        helper_sr(env, regval, REG_ADDR(AUX_ID_erstatus, cpu->family));
        break;
    case V3_AUX_ERBTA:
        helper_sr(env, regval, REG_ADDR(AUX_ID_erbta, cpu->family));
        break;
    case V3_AUX_ECR:
        helper_sr(env, regval, REG_ADDR(AUX_ID_ecr, cpu->family));
        break;
    case V3_AUX_ERET:
        helper_sr(env, regval, REG_ADDR(AUX_ID_eret, cpu->family));
        break;
    case V3_AUX_EFA:
        helper_sr(env, regval, REG_ADDR(AUX_ID_efa, cpu->family));
        break;
    /* interrupt */
    case V3_AUX_IRQ_CTRL:
        helper_sr(env, regval, REG_ADDR(AUX_ID_aux_irq_ctrl, cpu->family));
        break;
    case V3_AUX_IRQ_ACT:
        helper_sr(env, regval, REG_ADDR(AUX_ID_aux_irq_act, cpu->family));
        break;
    case V3_AUX_IRQ_HINT:
        helper_sr(env, regval, REG_ADDR(AUX_ID_aux_irq_hint, cpu->family));
        break;
    case V3_AUX_IRQ_SELECT:
        helper_sr(env, regval, REG_ADDR(AUX_ID_irq_select, cpu->family));
        break;
    case V3_AUX_IRQ_ENABLE:
        helper_sr(env, regval, REG_ADDR(AUX_ID_irq_enable, cpu->family));
        break;
    case V3_AUX_IRQ_TRIGGER:
        helper_sr(env, regval, REG_ADDR(AUX_ID_irq_trigger, cpu->family));
        break;
    case V3_AUX_IRQ_PULSE:
        helper_sr(env, regval, REG_ADDR(AUX_ID_irq_pulse_cancel, cpu->family));
        break;
    case V3_AUX_IRQ_PRIO:
        helper_sr(env, regval, REG_ADDR(AUX_ID_irq_priority, cpu->family));
        break;
    case V3_AUX_BTA:
        helper_sr(env, regval, REG_ADDR(AUX_ID_bta, cpu->family));
        break;
    default:
        assert(!"Unsupported minimal auxiliary register is being written.");
    }
    return sizeof(regval);
}

static int
gdb_v3_fpu_read(CPUARCState *env, GByteArray *mem_buf, int regnum)
{
    ARCCPU *cpu = env_archcpu(env);
    switch (regnum) {
    case 0 ... 31:
        return gdb_get_reg64(mem_buf, env->fpr[regnum]);
    case V3_FPU_BUILD: {
        /*
         * TODO FPU: when fpu module is implemented, this logic should
         * move there and here we should just call the fpu_getter.
         */
        uint32_t reg_bld = 0;
        if (cpu->cfg.has_fpu) {
            reg_bld = (4 << 0)  |   /* version: ARCv3 floating point */
                      (1 << 8)  |   /* hp: half precision */
                      (1 << 9)  |   /* sp: single precision */
                      (1 << 10) |   /* dp: double precision */
                      (1 << 11) |   /* ds: divide and square root */
                      (1 << 12) |   /* vf: vector floating point */
                      (1 << 13) |   /* wv: wide vector */
                      (5 << 16) |   /* fp_regs: 32 */
                      (0 << 24);    /* dd: no demand driven floating point */
        }
        return gdb_get_reg32(mem_buf, reg_bld);
    }
    case V3_FPU_CTRL:
        return gdb_get_reg32(mem_buf, env->fp_ctrl);
    case V3_FPU_STATUS:
        return gdb_get_reg32(mem_buf, env->fp_status);
    default:
        return 0;
    }
}

static int
gdb_v3_fpu_write(CPUARCState *env, uint8_t *mem_buf, int regnum)
{
    switch (regnum) {
    case 0 ... 31:
        env->fpr[regnum] = ldq_p(mem_buf);
        return sizeof(uint64_t);
    case V3_FPU_BUILD:
        /* build register cannot be changed. */
        return 0;
    case V3_FPU_CTRL:
        env->fp_ctrl = ldl_p(mem_buf);
        return sizeof(uint32_t);
    case V3_FPU_STATUS:
        env->fp_status = ldl_p(mem_buf);
        return sizeof(uint32_t);
    default:
        return 0;
    }
}

void arc_cpu_register_gdb_regs_for_features(ARCCPU *cpu)
{
    CPUState *cs = CPU(cpu);

    if (cpu->family & ARC_OPCODE_ARCV2) {
        gdb_register_coprocessor(cs,
          /* getter */           gdb_v2_aux_read,
          /* setter */           gdb_v2_aux_write,
          /* number of regs */   GDB_ARCV2_AUX_LAST,
          /* feature file */     GDB_ARCV2_AUX_XML,
          /* pos. in g packet */ 0);
    } else if (cpu->family & ARC_OPCODE_ARC32) {
        gdb_register_coprocessor(cs,
                                 gdb_v3_aux_read,
                                 gdb_v3_aux_write,
                                 GDB_ARCV3_AUX_LAST,
                                 GDB_ARCV3_32_AUX_XML,
                                 0);
    } else if (cpu->family & ARC_OPCODE_ARC64) {
        gdb_register_coprocessor(cs,
                                 gdb_v3_aux_read,
                                 gdb_v3_aux_write,
                                 GDB_ARCV3_AUX_LAST,
                                 GDB_ARCV3_64_AUX_XML,
                                 0);
        gdb_register_coprocessor(cs,
                                 gdb_v3_fpu_read,
                                 gdb_v3_fpu_write,
                                 GDB_ARCV3_FPU_LAST,
                                 GDB_ARCV3_64_FPU_XML,
                                 0);
    } else {
        g_assert_not_reached();
    }
}

/*-*-indent-tabs-mode:nil;tab-width:4;indent-line-function:'insert-tab'-*-*/
/* vim: set ts=4 sw=4 et: */
