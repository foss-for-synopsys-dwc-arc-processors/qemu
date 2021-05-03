/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2020 Synppsys Inc.
 * Contributed by Cupertino Miranda <cmiranda@synopsys.com>
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
#include "translate.h"
#include "qemu/bitops.h"
#include "tcg/tcg.h"
#include "semfunc-helper.h"
#include "translate.h"

void arc_gen_verifyCCFlag(const DisasCtxt *ctx, TCGv ret)
{
    TCGv c1 = tcg_temp_new();

    TCGv nZ = tcg_temp_new();
    TCGv nN = tcg_temp_new();
    TCGv nV = tcg_temp_new();
    TCGv nC = tcg_temp_new();

    switch (ctx->insn.cc) {
    /* AL, RA */
    case ARC_COND_AL:
        tcg_gen_movi_tl(ret, 1);
        break;
    /* EQ, Z */
    case ARC_COND_EQ:
        tcg_gen_mov_tl(ret, cpu_Zf);
        break;
    /* NE, NZ */
    case ARC_COND_NE:
        tcg_gen_xori_tl(ret, cpu_Zf, 1);
        break;
    /* PL, P */
    case ARC_COND_PL:
        tcg_gen_xori_tl(ret, cpu_Nf, 1);
        break;
    /* MI, N: */
    case ARC_COND_MI:
        tcg_gen_mov_tl(ret, cpu_Nf);
        break;
    /* CS, C, LO */
    case ARC_COND_CS:
        tcg_gen_mov_tl(ret, cpu_Cf);
        break;
    /* CC, NC, HS */
    case ARC_COND_CC:
        tcg_gen_xori_tl(ret, cpu_Cf, 1);
        break;
    /* VS, V */
    case ARC_COND_VS:
        tcg_gen_mov_tl(ret, cpu_Vf);
        break;
    /* VC, NV */
    case ARC_COND_VC:
        tcg_gen_xori_tl(ret, cpu_Vf, 1);
        break;
    /* GT */
    case ARC_COND_GT:
        /* (N & V & !Z) | (!N & !V & !Z) === XNOR(N, V) & !Z */
        tcg_gen_eqv_tl(ret, cpu_Nf, cpu_Vf);
        tcg_gen_xori_tl(nZ, cpu_Zf, 1);
        tcg_gen_and_tl(ret, ret, nZ);
        break;
    /* GE */
    case ARC_COND_GE:
        /* (N & V) | (!N & !V)  === XNOR(N, V) */
        tcg_gen_eqv_tl(ret, cpu_Nf, cpu_Vf);
        tcg_gen_andi_tl(ret, ret, 1);
        break;
    /* LT */
    case ARC_COND_LT:
        /* (N & !V) | (!N & V) === XOR(N, V) */
        tcg_gen_xor_tl(ret, cpu_Nf, cpu_Vf);
        break;
    /* LE */
    case ARC_COND_LE:
        /* Z | (N & !V) | (!N & V) === XOR(N, V) | Z */
        tcg_gen_xor_tl(ret, cpu_Nf, cpu_Vf);
        tcg_gen_or_tl(ret, ret, cpu_Zf);
        break;
    /* HI */
    case ARC_COND_HI:
        /* !C & !Z === !(C | Z) */
        tcg_gen_or_tl(ret, cpu_Cf, cpu_Zf);
        tcg_gen_xori_tl(ret, ret, 1);
        break;
    /* LS */
    case ARC_COND_LS:
        /* C & Z */
        tcg_gen_or_tl(ret, cpu_Cf, cpu_Zf);
        break;
    /* PNZ */
    case ARC_COND_PNZ:
        /* !N & !Z === !(N | Z) */
        tcg_gen_or_tl(ret, cpu_Nf, cpu_Zf);
        tcg_gen_xori_tl(ret, ret, 1);
        break;

    default:
        g_assert_not_reached();
    }

    tcg_temp_free(c1);
    tcg_temp_free(nZ);
    tcg_temp_free(nN);
    tcg_temp_free(nV);
    tcg_temp_free(nC);
}

#define MEMIDX (ctx->mem_idx)

#ifdef TARGET_ARCV2
const MemOp memop_for_size_sign[2][3] = {
    { MO_UL, MO_UB, MO_UW }, /* non sign-extended */
    { MO_UL, MO_SB, MO_SW } /* sign-extended */
};
#endif

#ifdef TARGET_ARCV3
const MemOp memop_for_size_sign[2][4] = {
    { MO_UL, MO_UB, MO_UW, MO_Q }, /* non sign-extended */
    { MO_SL, MO_SB, MO_SW, MO_Q } /* sign-extended */
};
#endif

void arc_gen_set_memory(const DisasCtxt *ctx, TCGv vaddr, int size,
        TCGv src, bool sign_extend)
{
#ifdef TARGET_ARCV2
    assert(size != 0x3);
#endif

    tcg_gen_qemu_st_tl(src, vaddr, MEMIDX,
                       memop_for_size_sign[sign_extend][size]);
}

void arc_gen_get_memory(const DisasCtxt *ctx, TCGv dest, TCGv vaddr,
        int size, bool sign_extend)
{
#ifdef TARGET_ARCV2
    assert(size != 0x3);
#endif

    tcg_gen_qemu_ld_tl(dest, vaddr, MEMIDX,
                       memop_for_size_sign[sign_extend][size]);
}

void arc_gen_no_further_loads_pending(const DisasCtxt *ctx, TCGv ret)
{
    /* TODO: To complete on SMP support. */
    tcg_gen_movi_tl(ret, 1);
}

void arc_gen_set_debug(const DisasCtxt *ctx, bool value)
{
    /* TODO: Could not find a reson to set this. */
}

/* dest = src1 - src2. Compute C, N, V and Z flags */
void arc_gen_sub_Cf(TCGv ret, TCGv dest, TCGv src1, TCGv src2)
{
    TCGv t1 = tcg_temp_new();
    TCGv t2 = tcg_temp_new();
    TCGv t3 = tcg_temp_new();

    tcg_gen_not_tl(t1, src1);       /* t1 = ~src1                 */
    tcg_gen_and_tl(t2, t1, src2);   /* t2 = ~src1 & src2          */
    tcg_gen_or_tl(t3, t1, src2);    /* t3 = (~src1 | src2) & dest */
    tcg_gen_and_tl(t3, t3, dest);
    /* t2 = ~src1 & src2 | ~src1 & dest | dest & src2 */
    tcg_gen_or_tl(t2, t2, t3);
    tcg_gen_shri_tl(ret, t2, TARGET_LONG_BITS - 1);   /* Cf = t2[31/63] */

    tcg_temp_free(t3);
    tcg_temp_free(t2);
    tcg_temp_free(t1);
}


void arc_gen_get_bit(TCGv ret, TCGv a, TCGv pos)
{
    tcg_gen_rotr_tl(ret, a, pos);
    tcg_gen_andi_tl(ret, ret, 1);
}

/* accumulator += b32 * c32 */
void arc_gen_mac(TCGv phi, TCGv b32, TCGv c32)
{
    TCGv plo = tcg_temp_new();
    tcg_gen_muls2_tl(plo, phi, b32, c32);

    /* Adding the product to the accumulator */
    tcg_gen_add2_tl(cpu_acclo, cpu_acchi, cpu_acclo, cpu_acchi, plo, phi);
    tcg_temp_free(plo);
}

/* Unsigned version of mac */
void arc_gen_macu(TCGv phi, TCGv b32, TCGv c32)
{
    TCGv plo = tcg_temp_new();
    tcg_gen_mulu2_tl(plo, phi, b32, c32);

    /* Adding the product to the accumulator */
    tcg_gen_add2_tl(cpu_acclo, cpu_acchi, cpu_acclo, cpu_acchi, plo, phi);
    tcg_temp_free(plo);
}

void tcg_gen_shlfi_tl(TCGv a, int b, TCGv c)
{
    TCGv tmp = tcg_temp_new();
    tcg_gen_movi_tl(tmp, b);
    tcg_gen_shl_tl(a, tmp, c);
    tcg_temp_free(tmp);
}

void arc_gen_extract_bits(TCGv ret, TCGv a, TCGv start, TCGv end)
{
    TCGv tmp1 = tcg_temp_new();

    tcg_gen_shr_tl(ret, a, end);

    tcg_gen_sub_tl(tmp1, start, end);
    tcg_gen_addi_tl(tmp1, tmp1, 1);
    tcg_gen_shlfi_tl(tmp1, 1, tmp1);
    tcg_gen_subi_tl(tmp1, tmp1, 1);

    tcg_gen_and_tl(ret, ret, tmp1);

    tcg_temp_free(tmp1);
}

void arc_gen_get_register(TCGv ret, enum arc_registers reg)
{
    switch (reg) {
    case R_SP:
        tcg_gen_mov_tl(ret, cpu_sp);
        break;
    case R_STATUS32:
        gen_helper_get_status32(ret, cpu_env);
        break;
    case R_ACCLO:
        tcg_gen_mov_tl(ret, cpu_acclo);
        break;
    case R_ACCHI:
        tcg_gen_mov_tl(ret, cpu_acchi);
        break;
    default:
        g_assert_not_reached();
    }
}


void arc_gen_set_register(enum arc_registers reg, TCGv value)
{
    switch (reg) {
    case R_SP:
        tcg_gen_mov_tl(cpu_sp, value);
        break;
    case R_STATUS32:
        gen_helper_set_status32(cpu_env, value);
        break;
    case R_ACCLO:
        tcg_gen_mov_tl(cpu_acclo, value);
        break;
    case R_ACCHI:
        tcg_gen_mov_tl(cpu_acchi, value);
        break;
    default:
        g_assert_not_reached();
    }
}


/* TODO: Get this from props ... */
void arc_has_interrupts(const DisasCtxt *ctx, TCGv ret)
{
    tcg_gen_movi_tl(ret, 1);
}

/*
 ***************************************
 * Statically inferred return function *
 ***************************************
 */

TCGv arc_gen_next_reg(const DisasCtxt *ctx, TCGv reg)
{
    int i;
    for (i = 0; i < 64; i += 2) {
        if (reg == cpu_r[i]) {
            return cpu_r[i + 1];
        }
    }
    /* Check if REG is an odd register. */
    for (i = 1; i < 64; i += 2) {
        /* If so, that is unsanctioned. */
        if (reg == cpu_r[i]) {
            arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
            return NULL;
        }
    }
    /* REG was not a register after all. */
    g_assert_not_reached();
}

bool arc_target_has_option(enum target_options option)
{
    /* TODO: Fill with meaningful cases. */
    switch (option) {
    case LL64_OPTION:
        return true;
        break;
    default:
        break;
    }
    return false;
}


bool arc_is_instruction_operand_a_register(const DisasCtxt *ctx, int nop)
{
    assert(nop < ctx->insn.n_ops);
    operand_t operand = ctx->insn.operands[nop];

    return (operand.type & ARC_OPERAND_IR) != 0;
}


/*-*-indent-tabs-mode:nil;tab-width:4;indent-line-function:'insert-tab'-*-*/
/* vim: set ts=4 sw=4 et: */
