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
arc_gen_set_if_overflow(TCGv_i64 res, TCGv_i64 op1, TCGv_i64 op2,
                        TCGv_i64 overflow,
                        ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64)
{
    TCGv_i64 new_overflow = tcg_temp_new_i64();
    detect_overflow_i64(new_overflow, res, op1, op2);
    /*
     * By oring the new overflow into the provided overflow, it can only change
     * from 0 to 1
     */
    tcg_gen_or_i64(overflow, new_overflow, overflow);

    tcg_temp_free_i64(new_overflow);
}

void
arc_gen_set_vector_constant_operand(DisasCtxt *ctx, TCGv_i64 tcg_operand,
                                    operand_t* operand)
{
    uint64_t base_op_value;

    /*
     * Register, do nothing
     */
    if (operand->type & ARC_OPERAND_IR) {
        return;
    } else {
        /*
         * Word split limm
         */
        if (operand->type & ARC_OPERAND_LIMM &&
            operand->type & ARC_OPERAND_32_SPLIT) {

            base_op_value = ctx->insn.limm;
            base_op_value = base_op_value | base_op_value << 32;
            tcg_gen_movi_i64(tcg_operand, base_op_value);
        }
    }
}

static void
arc_gen_mac_check_fflags(DisasCtxt *ctx, TCGv_i64 result, TCGv_i64 sums,
                         TCGv_i64 acc, bool set_n_flag,
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

            tcg_gen_ext_i32_i64(overflow, getVFlag());
            arc_gen_set_if_overflow(result, acc, sums, overflow, detect_overflow_i64);
            tcg_gen_extrl_i64_i32(getVFlag(), overflow);

            tcg_temp_free_i64(overflow);
        #else
            if (set_n_flag) {
                tcg_gen_shri_i64(getNFlag(), result, 63);
            }
            arc_gen_set_if_overflow(result, acc, sums, getVFlag(), detect_overflow_i64);
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

    /*
     * Instruction code
     */

    arc_gen_set_vector_constant_operand(ctx, c, &(ctx->insn.operands[2]));

    extract_bits(b_h0, b, 0, 16);
    extract_bits(b_h1, b, 16, 16);
    extract_bits(b_h2, b, 32, 16);
    extract_bits(b_h3, b, 48, 16);

    extract_bits(c_h0, c, 0, 16);
    extract_bits(c_h1, c, 16, 16);
    extract_bits(c_h2, c, 32, 16);
    extract_bits(c_h3, c, 48, 16);

    /*
     * Multiply halfwords with words
     */
    tcg_gen_mul_i64(b_h0, b_h0, c_h0);
    tcg_gen_mul_i64(b_h1, b_h1, c_h1);
    tcg_gen_mul_i64(b_h2, b_h2, c_h2);
    tcg_gen_mul_i64(b_h3, b_h3, c_h3);

    /*
     * We dont need to truncate the multiplication results because
     * we know for a fact that it is a 16 bit number multiplication,
     * and we expect the result to be 32 bit (PRM) so there will never
     * be an overflow (0xffff * 0xffff = 0xfffe 0001 < 0x1 0000 0000)
     *
     * tcg_gen_andi_tl(b_hX, b_hX, 0xffffffff);
     */

    /*
     * Assemble final result via additions
     * As the operands are 32 bit, it is not possible for the sums to
     * overflow a 64 bit number either
     */
    tcg_gen_add_i64(b_h0, b_h0, b_h1);
    tcg_gen_add_i64(b_h2, b_h2, b_h3);

    tcg_gen_add_i64(b_h0, b_h0, b_h2);

    tcg_gen_add_i64(a, acc, b_h0);

    arc_gen_mac_check_fflags(ctx, a, b_h0, acc, set_n_flag, detect_overflow_i64);

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

    /*
     * Instruction code
     */

    arc_gen_set_vector_constant_operand(ctx, c, &(ctx->insn.operands[2]));

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
     * We dont need to truncate the multiplication results because
     * we know for a fact that it is a 16 bit number multiplication,
     * and we expect the result to be 32 bit (PRM) so there will never
     * be an overflow (0xffff * 0xffff = 0xfffe 0001 < 0x1 0000 0000)
     *
     * tcg_gen_andi_tl(b_hX, b_hX, 0xffffffff);
     */

    /*
     * Assemble final result via additions
     * As the operands are 32 bit, it is not possible for the sums to
     * overflow a 64 bit number either
     */
    tcg_gen_add_i64(b_w0, b_w0, b_w1);
    tcg_gen_add_i64(a, acc, b_w0);

    arc_gen_mac_check_fflags(ctx, a, b_w0, acc, set_n_flag, detect_overflow_i64);

    tcg_gen_add_i64(acc, acc, b_w0);


    tcg_temp_free_i64(b_w0);
    tcg_temp_free_i64(b_w1);

    tcg_temp_free_i64(c_h0);
    tcg_temp_free_i64(c_h1);
}
