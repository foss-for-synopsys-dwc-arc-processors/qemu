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

#ifndef ARC_TRANSLATE_H_
#define ARC_TRANSLATE_H_


#include "arc-common.h"

#include "tcg/tcg.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "disas/disas.h"
#include "tcg/tcg-op.h"
#include "exec/cpu_ldst.h"

#include "exec/helper-proto.h"
#include "exec/helper-gen.h"
#include "exec/log.h"

#include "exec/translator.h"

/* signaling the end of translation block */
#define DISAS_UPDATE        DISAS_TARGET_0
#define DISAS_BRANCH_IN_DELAYSLOT DISAS_TARGET_1

typedef struct DisasContext {
    DisasContextBase base;

    uint64_t cpc;   /*  current pc      */
    uint64_t npc;   /*  next pc         */
    uint64_t dpc;   /*  next next pc    */
    uint64_t pcl;
    uint64_t lpe;
    uint64_t lps;

    unsigned ds;    /*  we are within ds*/

    /* TODO (issue #62): these must be removed */
    TCGv     zero;  /*  0x00000000      */
    TCGv     one;   /*  0x00000001      */

    insn_t insn;

    CPUARCState *env;

    uint16_t buffer[2];
    uint8_t  mem_idx;

    TCGv     tmp_reg;
    TCGLabel *label;

} DisasContext;


#define cpu_gp     (cpu_r[26])
#define cpu_fp     (cpu_r[27])
#define cpu_sp     (cpu_r[28])
#define cpu_ilink1 (cpu_r[29])
#define cpu_ilink2 (cpu_r[30])
#define cpu_blink  (cpu_r[31])
#define cpu_acclo  (cpu_r[58])
#define cpu_acchi  (cpu_r[59])
#define cpu_lpc    (cpu_r[60])
#define cpu_pcl    (cpu_r[63])
#define cpu_limm   (cpu_r[62])

extern TCGv     cpu_S1f;
extern TCGv     cpu_S2f;
extern TCGv     cpu_CSf;

extern TCGv     cpu_pstate;
extern TCGv     cpu_Vf;
extern TCGv     cpu_Cf;
extern TCGv     cpu_Nf;
extern TCGv     cpu_Zf;

extern TCGv     cpu_er_pstate;
extern TCGv     cpu_er_Vf;
extern TCGv     cpu_er_Cf;
extern TCGv     cpu_er_Nf;
extern TCGv     cpu_er_Zf;

extern TCGv     cpu_eret;
extern TCGv     cpu_erbta;
extern TCGv     cpu_ecr;
extern TCGv     cpu_efa;

extern TCGv     cpu_pc;
extern TCGv     cpu_lps;
extern TCGv     cpu_lpe;

extern TCGv     cpu_npc;

extern TCGv     cpu_bta;

extern TCGv     cpu_r[64];

extern TCGv     cpu_intvec;

extern TCGv     cpu_lock_lf_var;

extern TCGv     cpu_exception_delay_slot_address;


/* TODO: Remove DisasCtxt.  */
typedef struct DisasContext DisasCtxt;

void gen_goto_tb(const DisasContext *ctx, int n, TCGv dest);

void decode_opc(CPUARCState *env, DisasContext *ctx);

/*
 * Helper function to glue "rasing an exception" in the generated TCGs.
 *
 * ctx:         Disassembling context
 * index:       ECR's index field
 * causecode:   ECR's cause code filed
 * param:       ECR's parameter field
 */
void arc_gen_excp(const DisasCtxt *ctx, uint32_t index,
                  uint32_t causecode, uint32_t param);

#endif


/*-*-indent-tabs-mode:nil;tab-width:4;indent-line-function:'insert-tab'-*-*/
/* vim: set ts=4 sw=4 et: */
