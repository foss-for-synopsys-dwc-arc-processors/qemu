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
        /* C | Z */
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

/*
 * This functions is only used when dealing with branch instructions.
 * It's called to emit TCGs at the point that it's decided that the
 * branch _is_ taken. Therefore, the PC is going to change. In the absence
 * of a delay slot, the PC is changed immediately. Whereas in the other
 * case, the change of PC is delayed by setting the BTA (Branch Target
 * Address) register and the "DE" flag of "status32".
 *
 * if (ctx->insn.d)      // has a delay slot
 * {
 *     status32.de = 1
 *     reg_bta = target
 * } else {               // without any delay slot
 *     cpu_pc = target
 *     [ chain tb to "target" address ]
 * }
 *
 * There is no use of gen_gotoi_tb() to fall through to a delay slot.
 * The caller can put it at the most appropriate place among the emitted
 * TCG instructions. Consider DBNZ as an example:
 *
 *                          if (--counter)
 *                                {
 *  ------------------------gen_branchi()-----------------------
 *                          /           \
 *       gen_gotoi(target, 1)            status32.de = 1
 *                                       BTA = target
 *  -------------------------End of call------------------------
 *                                 }
 *                       return DISAS_NORETURN
 *                                 .
 *                                 .
 *                                 .
 *                       if (ret == DISAS_NORETURN)
 *                           gen_gotoi(next_pc, 0)
 *                           ^^^^^^^^^
 * this will "goto" the next linear insn, which either may be the delay
 * slot of dbnz or the fall-thru instruction for cases that the counter
 * has reached 0.
 */
void
gen_branchi(DisasCtxt *ctx, target_ulong target)
{
    assert(ctx->insn.class == BRANCH ||
           ctx->insn.class == BRCC   ||
           ctx->insn.class == BBIT0  ||
           ctx->insn.class == BBIT1);

    if (ctx->insn.d) {
        tcg_gen_ori_tl(cpu_pstate, cpu_pstate, STATUS32_DE);
        tcg_gen_movi_tl(cpu_bta, target);
    } else {
        /* Use slot 1, because slot 0 is used for next pc. */
        gen_gotoi_tb(ctx, 1, target);
    }
}

/* The TCGv version of gen_branchi(). */
void
gen_branch(DisasCtxt *ctx, TCGv target)
{
    assert(ctx->insn.class == JUMP);

    if (ctx->insn.d) {
        tcg_gen_ori_tl(cpu_pstate, cpu_pstate, STATUS32_DE);
        tcg_gen_mov_tl(cpu_bta, target);
    } else {
        gen_goto_tb(ctx, target);
    }
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

/* TODO: Get this from props ... */
void arc_has_interrupts(const DisasCtxt *ctx, TCGv ret)
{
    tcg_gen_movi_tl(ret, 1);
}

#ifdef TARGET_ARC32
const MemOp memop_for_size_sign[2][3] = {
    { MO_UL, MO_UB, MO_UW }, /* non sign-extended */
    { MO_UL, MO_SB, MO_SW } /* sign-extended */
};
#endif

#ifdef TARGET_ARC64
const MemOp memop_for_size_sign[2][4] = {
    { MO_UL, MO_UB, MO_UW, MO_UQ }, /* non sign-extended */
    { MO_SL, MO_SB, MO_SW, MO_SQ } /* sign-extended */
};
#endif



/*
 ***************************************
 * Statically inferred return function *
 ***************************************
 */
#if defined (TARGET_ARC32)
  #define arc_tcgv_tl_temp tcgv_i32_temp
#elif defined (TARGET_ARC64)
  #define arc_tcgv_tl_temp tcgv_i64_temp
#else
    #error "Should not happen"
#endif

TCGv arc_gen_next_reg(const DisasCtxt *ctx, TCGv reg, bool fail)
{
    ptrdiff_t n = arc_tcgv_tl_temp(reg) - arc_tcgv_tl_temp(cpu_r[0]);
    if (n >= 0 && n < 64) {
        /* Check if REG is an even register. */
        if (n % 2 == 0)
            return cpu_r[n + 1];

        /* REG is an odd register. */
        arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
        return reg;
    }

    /* REG was not a register after all. */
    if (fail)
        g_assert_not_reached();

    return reg;
}

TCGv arc_gen_next_fpu_reg(const DisasCtxt *ctx, TCGv reg, bool fail)
{
    ptrdiff_t n = arc_tcgv_tl_temp(reg) - arc_tcgv_tl_temp(cpu_fpr[0]);
    if (n >= 0 && n < 32) {
        /* Check if REG is an even register. */
        if (n % 2 == 0) {
            return cpu_fpr[n + 1];
        }

        /* REG is an odd register. */
        arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
        return reg;
    }

    /* REG was not a register after all. Do we accept NULL reg? fail == false*/
    if (fail) {
        g_assert_not_reached();
    }

    return reg;
}

bool arc_target_has_option(enum target_options option)
{
    /* TODO: Fill with meaningful cases. */
    switch (option) {
    case LL64_OPTION:
        return true;
        break;
    case DIV_REM_OPTION:
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
