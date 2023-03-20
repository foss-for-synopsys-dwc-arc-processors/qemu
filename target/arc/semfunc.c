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
#include "target/arc/semfunc.h"
#include "exec/gen-icount.h"
#include "tcg/tcg-op-gvec.h"

void
arc_gen_vec_add16_w0_i64(TCGv_i64 d, TCGv_i64 a, TCGv_i64 b)
{
    TCGv_i64 t1 = tcg_temp_new_i64();

    tcg_gen_vec_add16_i64(t1, a, b);
    tcg_gen_deposit_i64(d, d, t1, 0, 32);

    tcg_temp_free_i64(t1);
}

void
arc_gen_cmpl2_i64(TCGv_i64 ret, TCGv_i64 arg1,
                  unsigned int ofs, unsigned int len)
{
    TCGv_i64 t1 = tcg_temp_new_i64();
    TCGv_i64 t2 = tcg_temp_new_i64();

    tcg_gen_mov_i64(t1, arg1);
    tcg_gen_extract_i64(t2, t1, ofs, len);
    tcg_gen_not_i64(t2, t2);
    tcg_gen_addi_i64(t2, t2, 1);
    tcg_gen_deposit_i64(t1, t1, t2, ofs, len);
    tcg_gen_mov_i64(ret, t1);

    tcg_temp_free_i64(t2);
	tcg_temp_free_i64(t1);
}

void
arc_gen_add_signed_overflow_i64(TCGv_i64 overflow, TCGv_i64 result,
                                TCGv_i64 op1, TCGv_i64 op2)
{
    TCGv_i64 t1 = tcg_temp_new_i64();
    TCGv_i64 t2 = tcg_temp_new_i64();

    /*
     * Check if the result has a different sign from one of the opperands:
     *   Last bit of t1 must be 1 (Different sign)
     *   Last bit of t2 must be 0 (Same sign)
     */
    tcg_gen_xor_i64(t1, op1, result);
    tcg_gen_xor_i64(t2, op1, op2);
    tcg_gen_andc_i64(t1, t1, t2);

    tcg_gen_shri_i64(overflow, t1, 63);

    tcg_temp_free_i64(t1);
    tcg_temp_free_i64(t2);
}

void
arc_gen_add_unsigned_overflow_i64(TCGv_i64 overflow, TCGv_i64 result,
                                  TCGv_i64 op1, TCGv_i64 op2)
{
    TCGv_i64 t1 = tcg_temp_new_i64();

    /*
     * If at least 1 of the operands had a 1 bit in position 63, and the result
     * has a 0, there was an overflow
     */
    tcg_gen_or_i64(t1, op1, op2);
    tcg_gen_andc_i64(t1, t1, result);

    tcg_gen_shri_i64(overflow, t1, 63);

    tcg_temp_free_i64(t1);
}

void
arc_gen_set_if_overflow(TCGv_i64 res, TCGv_i64 operand_1, TCGv_i64 operand_2,
                        TCGv_i64 overflow,
                        ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64)
{
    TCGv_i64 new_overflow = tcg_temp_new_i64();
    detect_overflow_i64(new_overflow, res, operand_1, operand_2);
    /*
     * By oring the new overflow into the provided overflow, it can only change
     * from 0 to 1
     */
    tcg_gen_or_i64(overflow, new_overflow, overflow);

    tcg_temp_free_i64(new_overflow);
}

void
arc_gen_set_operand_16bit_vec_const(DisasCtxt *ctx, operand_t *operand) {
    if (operand->type & ARC_OPERAND_LIMM) {
        operand->value  = ctx->insn.limm & 0x00000000ffffffff;
        operand->value |= (operand->value << 32);
    } else {
        operand->value &= 0x0000ffff;
        operand->value |= (operand->value << 16);
        operand->value |= (operand->value << 32);
    }
}

void
arc_gen_set_operand_32bit_vec_const(DisasCtxt *ctx, operand_t *operand) {
    if (operand->type & ARC_OPERAND_LIMM) {
        operand->value  = ctx->insn.limm & 0x00000000ffffffff;
        operand->value |= operand->value << 32;
    } else {
        operand->value &= 0x00000000ffffffff;
        operand->value |= (operand->value << 32);
    }
}

void
arc_gen_set_operand_64bit_vec_const(DisasCtxt *ctx, operand_t *operand) {
    if (operand->type & ARC_OPERAND_LIMM) {
        operand->value  = ctx->insn.limm;
    }
}

/**
 * @brief Depending on the "f" flag, takes care of N/V flags for mac functions
 * Overflow flag is only set (0 to 1), never unset (1 to 0)
 * @param ctx Current instruction context
 * @param result The result of the operation
 * @param operand_1 First operation operand
 * @param operand_2 Second operation operand
 * @param set_n_flag Whether to set the negative flag (N) or leave it alone
 * @param detect_overflow_i64 Function used to detect overflow
 */
static void
arc_gen_mac_check_fflags(DisasCtxt *ctx, TCGv_i64 result, TCGv_i64 operand_1,
                         TCGv_i64 operand_2, bool set_n_flag,
                         ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64)
{
    /*
     * F flag is set, affect the flags
     */
    if (getFFlag()) {
        #if TARGET_LONG_BITS == 32
            TCGv_i64 N_flag;
            TCGv_i64 overflow;

            if (set_n_flag) {
                N_flag = tcg_temp_new_i64();

                tcg_gen_shri_i64(N_flag, result, 63);
                tcg_gen_extrl_i64_i32(getNFlag(), N_flag);

                tcg_temp_free_i64(N_flag);
            }

            overflow = tcg_temp_new_i64();

            tcg_gen_ext_i32_i64(overflow, cpu_Vf);
            arc_gen_set_if_overflow(result, operand_1, operand_2, overflow, \
                                    detect_overflow_i64);
            tcg_gen_extrl_i64_i32(cpu_Vf, overflow);

            tcg_temp_free_i64(overflow);
        #else
            if (set_n_flag) {
                tcg_gen_shri_i64(getNFlag(), result, 63);
            }
            arc_gen_set_if_overflow(result, operand_1, operand_2, cpu_Vf, \
                                    detect_overflow_i64);
        #endif
    }
}


void
arc_gen_qmach_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                       TCGv_i64 acc, bool set_n_flag,
                       ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                       ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64)
{
    TCGv_i64 b_h0 = tcg_temp_new_i64();
    TCGv_i64 b_h1 = tcg_temp_new_i64();
    TCGv_i64 b_h2 = tcg_temp_new_i64();
    TCGv_i64 b_h3 = tcg_temp_new_i64();

    TCGv_i64 c_h0 = tcg_temp_new_i64();
    TCGv_i64 c_h1 = tcg_temp_new_i64();
    TCGv_i64 c_h2 = tcg_temp_new_i64();
    TCGv_i64 c_h3 = tcg_temp_new_i64();

    /* Instruction code */

    ARC_GEN_VEC_FIRST_OPERAND(operand_16bit, i64, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, i64, c);

    extract_bits(b_h0, b, 0, 16);
    extract_bits(b_h1, b, 16, 16);
    extract_bits(b_h2, b, 32, 16);
    extract_bits(b_h3, b, 48, 16);

    extract_bits(c_h0, c, 0, 16);
    extract_bits(c_h1, c, 16, 16);
    extract_bits(c_h2, c, 32, 16);
    extract_bits(c_h3, c, 48, 16);

    /*
     * Multiply halfwords
     * The 16 bit operands cannot overflow the expected 32 bit result
     */
    tcg_gen_mul_i64(b_h0, b_h0, c_h0);
    tcg_gen_mul_i64(b_h1, b_h1, c_h1);
    tcg_gen_mul_i64(b_h2, b_h2, c_h2);
    tcg_gen_mul_i64(b_h3, b_h3, c_h3);

    /*
     * Assemble final result via additions
     * As the operands are 32 bit, it is not possible for the sums to
     * overflow a 64 bit number
     */
    tcg_gen_add_i64(b_h0, b_h0, b_h1);
    tcg_gen_add_i64(b_h2, b_h2, b_h3);

    tcg_gen_add_i64(b_h0, b_h0, b_h2);

    tcg_gen_add_i64(a, acc, b_h0);

    arc_gen_mac_check_fflags(ctx, a, b_h0, acc, set_n_flag, \
                             detect_overflow_i64);

    tcg_gen_add_i64(acc, acc, b_h0);


    tcg_temp_free_i64(b_h0);
    tcg_temp_free_i64(b_h1);
    tcg_temp_free_i64(b_h2);
    tcg_temp_free_i64(b_h3);

    tcg_temp_free_i64(c_h0);
    tcg_temp_free_i64(c_h1);
    tcg_temp_free_i64(c_h2);
    tcg_temp_free_i64(c_h3);
}

void
arc_gen_dmacwh_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                        TCGv_i64 acc, bool set_n_flag,
                        ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                        ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64)
{
    TCGv_i64 b_w0 = tcg_temp_new_i64();
    TCGv_i64 b_w1 = tcg_temp_new_i64();

    TCGv_i64 c_h0 = tcg_temp_new_i64();
    TCGv_i64 c_h1 = tcg_temp_new_i64();

    /* Instruction code */

    ARC_GEN_VEC_FIRST_OPERAND(operand_32bit, i64, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, i64, c);

    extract_bits(b_w0, b, 0, 32);
    extract_bits(b_w1, b, 32, 32);

    extract_bits(c_h0, c, 0, 16);
    extract_bits(c_h1, c, 16, 16);

    /*
     * Multiply halfwords with words
     */
    tcg_gen_mul_i64(b_w0, b_w0, c_h0);
    tcg_gen_mul_i64(b_w1, b_w1, c_h1);

    /*
     * Assemble final result via additions
     * As the operands are 32 bit, it is not possible for the sums to
     * overflow a 64 bit number
     */
    tcg_gen_add_i64(b_w0, b_w0, b_w1);
    tcg_gen_add_i64(a, acc, b_w0);

    arc_gen_mac_check_fflags(ctx, a, b_w0, acc, set_n_flag, \
                             detect_overflow_i64);

    tcg_gen_add_i64(acc, acc, b_w0);


    tcg_temp_free_i64(b_w0);
    tcg_temp_free_i64(b_w1);

    tcg_temp_free_i64(c_h0);
    tcg_temp_free_i64(c_h1);
}

void
arc_gen_dmach_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                   TCGv_i64 acc, bool set_n_flag,
                   ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                   ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64)
{
    TCGv_i64 b_h0 = tcg_temp_new_i64();
    TCGv_i64 b_h1 = tcg_temp_new_i64();

    TCGv_i64 c_h0 = tcg_temp_new_i64();
    TCGv_i64 c_h1 = tcg_temp_new_i64();

    /* Instruction code */

    ARC_GEN_VEC_FIRST_OPERAND(operand_16bit, i64, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, i64, c);

    extract_bits(b_h0, b, 0, 16);
    extract_bits(b_h1, b, 16, 16);

    extract_bits(c_h0, c, 0, 16);
    extract_bits(c_h1, c, 16, 16);

    /*
     * Multiply halfwords
     */
    tcg_gen_mul_i64(b_h0, b_h0, c_h0);
    tcg_gen_mul_i64(b_h1, b_h1, c_h1);

    /*
     * Assemble final result via additions
     * As the operands are 32 bit, it is not possible for the sums to
     * overflow a 64 bit number either
     */
    tcg_gen_add_i64(b_h0, b_h0, b_h1);

    tcg_gen_add_i64(a, acc, b_h0);

    arc_gen_mac_check_fflags(ctx, a, b_h0, acc, set_n_flag, \
                             detect_overflow_i64);

    tcg_gen_add_i64(acc, acc, b_h0);
    tcg_gen_andi_i64(a, a, 0x0ffffffff);

    tcg_temp_free_i64(b_h0);
    tcg_temp_free_i64(b_h1);

    tcg_temp_free_i64(c_h0);
    tcg_temp_free_i64(c_h1);
}

/**
 * @brief Depending on the "f" flag, takes care of N/V flags for mpy functions
 * Overflow flag is only set (0 to 1), never unset (1 to 0)
 * @param ctx Current instruction context
 * @param result The result of the operation
 * @param operand_1 First operation operand
 * @param operand_2 Second operation operand
 * @param set_n_flag Whether to set the negative flag (N) or leave it alone
 */
static void
arc_gen_mpy_check_fflags(DisasCtxt *ctx, TCGv_i64 result, bool set_n_flag)
{
    /*
     * F flag is set, affect the flags
     */
    if (getFFlag()) {
        tcg_gen_movi_tl(cpu_Vf, 0);

        #if TARGET_LONG_BITS == 32
            TCGv_i64 N_flag;

            if (set_n_flag) {
                N_flag = tcg_temp_new_i64();

                tcg_gen_shri_i64(N_flag, result, 63);
                tcg_gen_extrl_i64_i32(getNFlag(), N_flag);

                tcg_temp_free_i64(N_flag);
            }
        #else
            if (set_n_flag) {
                tcg_gen_shri_i64(getNFlag(), result, 63);
            }
        #endif
    }
}

void
arc_gen_dmpyh_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                        TCGv_i64 acc, bool set_n_flag,
                        ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                        ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64)
{
    TCGv_i64 b_h0 = tcg_temp_new_i64();
    TCGv_i64 b_h1 = tcg_temp_new_i64();

    TCGv_i64 c_h0 = tcg_temp_new_i64();
    TCGv_i64 c_h1 = tcg_temp_new_i64();

    /* Instruction code */

    ARC_GEN_VEC_FIRST_OPERAND(operand_16bit, i64, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, i64, c);

    extract_bits(b_h0, b, 0, 16);
    extract_bits(b_h1, b, 16, 16);

    extract_bits(c_h0, c, 0, 16);
    extract_bits(c_h1, c, 16, 16);

    /* Multiply halfwords with words */
    tcg_gen_mul_i64(b_h0, b_h0, c_h0);
    tcg_gen_mul_i64(b_h1, b_h1, c_h1);

    /*
     * Assemble final result via additions
     * As the operands are 32 bit, it is not possible for the sums to
     * overflow a 64 bit number either
     */
    tcg_gen_add_i64(acc, b_h0, b_h1);

    arc_gen_mpy_check_fflags(ctx, acc, set_n_flag);

    tcg_gen_andi_i64(a, acc, 0xffffffff);

    tcg_temp_free_i64(b_h0);
    tcg_temp_free_i64(b_h1);

    tcg_temp_free_i64(c_h0);
    tcg_temp_free_i64(c_h1);
}

void
arc_gen_qmpyh_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                       TCGv_i64 acc, bool set_n_flag,
                       ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                       ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64)
{
    TCGv_i64 b_h0 = tcg_temp_new_i64();
    TCGv_i64 b_h1 = tcg_temp_new_i64();
    TCGv_i64 b_h2 = tcg_temp_new_i64();
    TCGv_i64 b_h3 = tcg_temp_new_i64();

    TCGv_i64 c_h0 = tcg_temp_new_i64();
    TCGv_i64 c_h1 = tcg_temp_new_i64();
    TCGv_i64 c_h2 = tcg_temp_new_i64();
    TCGv_i64 c_h3 = tcg_temp_new_i64();

    /* Instruction code */

    ARC_GEN_VEC_FIRST_OPERAND(operand_16bit, i64, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, i64, c);

    extract_bits(b_h0, b, 0, 16);
    extract_bits(b_h1, b, 16, 16);
    extract_bits(b_h2, b, 32, 16);
    extract_bits(b_h3, b, 48, 16);

    extract_bits(c_h0, c, 0, 16);
    extract_bits(c_h1, c, 16, 16);
    extract_bits(c_h2, c, 32, 16);
    extract_bits(c_h3, c, 48, 16);

    /*
     * Multiply halfwords
     * The 16 bit operands cannot overflow the expected 32 bit result
     */
    tcg_gen_mul_i64(b_h0, b_h0, c_h0);
    tcg_gen_mul_i64(b_h1, b_h1, c_h1);
    tcg_gen_mul_i64(b_h2, b_h2, c_h2);
    tcg_gen_mul_i64(b_h3, b_h3, c_h3);

    /*
     * Assemble final result via additions
     * As the operands are 32 bit, it is not possible for the sums to
     * overflow a 64 bit number
     */
    tcg_gen_add_i64(b_h0, b_h0, b_h1);
    tcg_gen_add_i64(b_h2, b_h2, b_h3);

    tcg_gen_add_i64(a, b_h0, b_h2);

    arc_gen_mpy_check_fflags(ctx, a, set_n_flag);

    tcg_gen_mov_i64(acc, a);


    tcg_temp_free_i64(b_h0);
    tcg_temp_free_i64(b_h1);
    tcg_temp_free_i64(b_h2);
    tcg_temp_free_i64(b_h3);

    tcg_temp_free_i64(c_h0);
    tcg_temp_free_i64(c_h1);
    tcg_temp_free_i64(c_h2);
    tcg_temp_free_i64(c_h3);
}

void
arc_gen_dmpywh_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                        TCGv_i64 acc, bool set_n_flag,
                        ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                        ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64)
{
    TCGv_i64 b_w0 = tcg_temp_new_i64();
    TCGv_i64 b_w1 = tcg_temp_new_i64();

    TCGv_i64 c_h0 = tcg_temp_new_i64();
    TCGv_i64 c_h1 = tcg_temp_new_i64();

    /* Instruction code */

    ARC_GEN_VEC_FIRST_OPERAND(operand_32bit, i64, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, i64, c);

    extract_bits(b_w0, b, 0, 32);
    extract_bits(b_w1, b, 32, 32);

    extract_bits(c_h0, c, 0, 16);
    extract_bits(c_h1, c, 16, 16);

    /*
     * Multiply halfwords with words
     */
    tcg_gen_mul_i64(b_w0, b_w0, c_h0);
    tcg_gen_mul_i64(b_w1, b_w1, c_h1);

    /*
     * Assemble final result via additions
     * As the operands are 32 bit, it is not possible for the sums to
     * overflow a 64 bit number
     */
    tcg_gen_add_i64(a, b_w0, b_w1);

    arc_gen_mpy_check_fflags(ctx, a, set_n_flag);

    tcg_gen_mov_i64(acc, a);

    tcg_temp_free_i64(b_w0);
    tcg_temp_free_i64(b_w1);

    tcg_temp_free_i64(c_h0);
    tcg_temp_free_i64(c_h1);

}

void
arc_gen_vmpy2h_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                        TCGv_i64 acc, bool set_n_flag,
                        ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                        ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64)
{
    TCGv_i64 b_h0 = tcg_temp_new_i64();
    TCGv_i64 b_h1 = tcg_temp_new_i64();

    TCGv_i64 c_h0 = tcg_temp_new_i64();
    TCGv_i64 c_h1 = tcg_temp_new_i64();

    /* Instruction code */

    ARC_GEN_VEC_FIRST_OPERAND(operand_16bit, i64, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, i64, c);

    extract_bits(b_h0, b, 0, 16);
    extract_bits(b_h1, b, 16, 16);

    extract_bits(c_h0, c, 0, 16);
    extract_bits(c_h1, c, 16, 16);

    /* Multiply halfwords with words */
    tcg_gen_mul_i64(b_h0, b_h0, c_h0);
    tcg_gen_mul_i64(b_h1, b_h1, c_h1);

    /*
     * Assemble final result
     */
    tcg_gen_extract_i64(a, b_h0, 0, 32);
    tcg_gen_shli_i64(b_h1, b_h1, 32);
    tcg_gen_or_i64(a, a, b_h1);

    /* Does not set/change any flag */

    tcg_gen_mov_i64(acc, a);

    tcg_temp_free_i64(b_h0);
    tcg_temp_free_i64(b_h1);

    tcg_temp_free_i64(c_h0);
    tcg_temp_free_i64(c_h1);
}

void
arc_gen_mpyd_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                        TCGv_i64 acc, bool set_n_flag,
                        ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                        ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64)
{
    /* Instruction code */
    ARC_GEN_VEC_FIRST_OPERAND(operand_64bit, i64, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_64bit, i64, c);

    extract_bits(b, b, 0, 32);
    extract_bits(c, c, 0, 32);

    tcg_gen_mul_i64(acc, b, c);

    tcg_gen_mov_i64(a, acc);

    arc_gen_mpy_check_fflags(ctx, a, set_n_flag);

}

void
arc_gen_except_no_wait_instructions(DisasCtxt *ctx)
{
    TCGLabel *done = gen_new_label();
    TCGv in_kernel_mode = tcg_temp_local_new();
    TCGv usermode_sleep_enabled = tcg_temp_local_new();

    inKernelMode(in_kernel_mode);
    getUsermodeSleep(usermode_sleep_enabled);

    /* If either in kernel mode or usermode sleep is toggled, we don't throw */
    tcg_gen_or_tl(in_kernel_mode, in_kernel_mode, usermode_sleep_enabled);
    tcg_gen_brcondi_tl(TCG_COND_EQ, in_kernel_mode, 1, done);

    throwExcpPriviledgeV();

    gen_set_label(done);

    tcg_temp_free(usermode_sleep_enabled);
    tcg_temp_free(in_kernel_mode);
}

/*
 * VPACK2HL -- CODED BY HAND
 */
int
arc_gen_VPACK2HL(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  TCGv b_h0 = tcg_temp_new();
  TCGv c_h0 = tcg_temp_new();

  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  /* Conditional execution */
  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  /* Instruction code */

  tcg_gen_sextract_tl(b_h0, b, 0, 16);
  tcg_gen_sextract_tl(c_h0, c, 0, 16);

  tcg_gen_mov_tl(a, c_h0);
  tcg_gen_deposit_tl(a, a, b_h0, 16, 16);

  /* Conditional execution end. */
  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  tcg_temp_free(b_h0);
  tcg_temp_free(c_h0);

  return DISAS_NEXT;
}

/*
 * VPACK2HM -- CODED BY HAND
 */
int
arc_gen_VPACK2HM(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  TCGv b_h1 = tcg_temp_new();
  TCGv c_h1 = tcg_temp_new();

  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  /* Conditional execution */
  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  /* Instruction code */

  tcg_gen_sextract_tl(b_h1, b, 16, 16);
  tcg_gen_sextract_tl(c_h1, c, 16, 16);

  tcg_gen_mov_tl(a, c_h1);
  tcg_gen_deposit_tl(a, a, b_h1, 16, 16);

  /* Conditional execution end. */
  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  tcg_temp_free(b_h1);
  tcg_temp_free(c_h1);

  return DISAS_NEXT;
}

MemOp
arc_gen_atld_op(DisasCtxt *ctx, TCGv_i32 b, TCGv c)
{
    void (*atomic_fn)(TCGv_i32, TCGv, TCGv_i32, TCGArg, MemOp);

    MemOp mop = MO_32 | MO_ALIGN;
    atomic_fn = NULL;

    switch(ctx->insn.op) {
    case ATO_ADD:
        atomic_fn = tcg_gen_atomic_fetch_add_i32;
        break;

    case ATO_OR:
        atomic_fn = tcg_gen_atomic_fetch_or_i32;
        break;

    case ATO_AND:
        atomic_fn = tcg_gen_atomic_fetch_and_i32;
        break;

    case ATO_XOR:
        atomic_fn = tcg_gen_atomic_fetch_xor_i32;
        break;

    case ATO_MINU:
        atomic_fn = tcg_gen_atomic_fetch_umin_i32;
        break;

    case ATO_MAXU:
        atomic_fn = tcg_gen_atomic_fetch_umax_i32;
        break;

    case ATO_MIN:
        atomic_fn = tcg_gen_atomic_fetch_smin_i32;
        mop |= MO_SIGN;
        break;

    case ATO_MAX:
        atomic_fn = tcg_gen_atomic_fetch_smax_i32;
        mop |= MO_SIGN;
        break;

    default:
        assert("Invalid atld operation");
        break;
    }

    if (ctx->insn.aq) {
        tcg_gen_mb(TCG_BAR_SC | TCG_MO_ALL);
    }

    atomic_fn(b, c, b, ctx->mem_idx, mop);

    if (ctx->insn.rl) {
        tcg_gen_mb(TCG_BAR_SC | TCG_MO_ALL);
    }

    return mop;
}