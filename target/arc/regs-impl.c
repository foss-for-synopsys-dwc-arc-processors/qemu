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
#include "qemu/log.h"
#include "qemu/error-report.h"
#include "target/arc/regs.h"
#include "target/arc/mmu.h"
#include "target/arc/mpu.h"
#include "target/arc/irq.h"
#include "target/arc/timer.h"
#include "target/arc/cache.h"

static target_ulong get_identity(CPUARCState *env)
{
    target_ulong chipid = 0xffff, arcnum = 0, arcver, res;
    ARCCPU *cpu = env_archcpu(env);

    switch (cpu->family) {
    case ARC_OPCODE_ARC700:
        arcver = 0x34;
        break;

    case ARC_OPCODE_ARCv2EM:
        arcver = 0x44;
        break;

    case ARC_OPCODE_ARCv2HS:
        arcver = 0x54;
        break;

    /* TODO: Add V3/ARC32. */
    case ARC_OPCODE_V3_ARC64:
        arcver = 0x70;
        break;

    default:
        arcver = 0;

    }

    /* TODO: in SMP, arcnum depends on the cpu instance. */
    res = ((chipid & 0xFFFF) << 16) | ((arcnum & 0xFF) << 8) | (arcver & 0xFF);
    return res;
}



/* TODO: Move this to thread safe data structure. */
target_ulong jli_base;

target_ulong
arc_jli_regs_get(const struct arc_aux_reg_detail *aux_reg_detail,
             void *data)
{
    switch (aux_reg_detail->id) {
    case AUX_ID_jli_base:
        return jli_base;
        break;
    default:
        assert(0);
        break;
    }
}

void
arc_jli_regs_set(const struct arc_aux_reg_detail *aux_reg_detail,
             target_ulong val, void *data)
{
    switch (aux_reg_detail->id) {
    case AUX_ID_jli_base:
        jli_base = val & (~3);
        break;
    default:
        assert(0);
        break;
    }
}


target_ulong
arc_general_regs_get(const struct arc_aux_reg_detail *aux_reg_detail,
                          void *data)
{
    CPUARCState *env = (CPUARCState *) data;
    ARCCPU *cpu = env_archcpu(env);
    target_ulong reg = 0;

    switch (aux_reg_detail->id) {
    case AUX_ID_aux_volatile:
        reg = 0xc0000000;
        break;

    case AUX_ID_lp_start:
        reg = env->lps;
        break;

    case AUX_ID_lp_end:
        reg = env->lpe;
        break;

    case AUX_ID_identity:
        reg = get_identity(env);
        break;

    case AUX_ID_exec_ctrl:
        reg = 0;
        break;

    case AUX_ID_debug:
        reg = 0;
        break;

    case AUX_ID_pc:
        reg = env->pc & 0xfffffffe;
        break;

    case AUX_ID_mpy_build:
        reg = cpu->mpy_build;
        break;

    case AUX_ID_isa_config:
        reg = cpu->isa_config;
        break;

    case AUX_ID_eret:
        reg = env->eret;
        break;

    case AUX_ID_erbta:
        reg = env->erbta;
        break;

    case AUX_ID_ecr:
        reg = env->ecr;
        break;

    case AUX_ID_efa:
        reg = env->efa;
        break;

    case AUX_ID_bta:
        reg = env->bta;
        break;

    case AUX_ID_bta_l1:
        reg = env->bta_l1;
        break;

    case AUX_ID_bta_l2:
        reg = env->bta_l2;
        break;

    case AUX_ID_hw_pf_ctrl:
        reg = (0)       /* EN */
            | (1 << 1)  /* RD_ST */
            | (1 << 3)  /* WR_ST */
            | (3 << 5)  /* OUTS */
            | (0 << 7); /* AG */

        break;

    case AUX_ID_unimp_bcr:
        /* TODO: raise instruction error here */
        reg = 0;
        break;

    default:
        break;
    }

    return reg;
}

void
arc_general_regs_set(const struct arc_aux_reg_detail *aux_reg_detail,
                     target_ulong val, void *data)
{
    CPUARCState *env = (CPUARCState *) data;

    switch (aux_reg_detail->id) {
    case AUX_ID_lp_start:
        env->lps = val;
        break;

    case AUX_ID_lp_end:
        env->lpe = val;
        break;

    case AUX_ID_eret:
        env->eret = val;
        break;

    case AUX_ID_erbta:
        env->erbta = val;
        break;

    case AUX_ID_bta:
        env->bta = val;
        break;

    case AUX_ID_ecr:
        env->ecr = val;
        break;

    case AUX_ID_efa:
        env->efa = val;
        break;

    default:
        break;
    }
}
