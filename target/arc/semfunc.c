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

/*
 *  Result = Op1 - Op2
 * In a signed subtraction, there is an overflow when all of these conditions
 *  are met:
 * (1) The operands have different signs
 * (2) The result does not have the same sign as the first operand
 */
void
arc_gen_sub_signed_overflow_tl(TCGv overflow, TCGv result,
                               TCGv op1, TCGv op2, uint8_t operand_size)
{
    TCGv diff_operand_sign = tcg_temp_new();
    TCGv diff_result_sign = tcg_temp_new();

    tcg_gen_xor_tl(diff_operand_sign, op1, op2);
    tcg_gen_xor_tl(diff_result_sign, result, op1);
    ///* if (diff_operand_sign AND diff_result_sign) */
    tcg_gen_and_tl(overflow, diff_operand_sign, diff_result_sign);

    tcg_gen_shri_tl(overflow, overflow, operand_size - 1);
    tcg_gen_andi_tl(overflow, overflow, 1);

    tcg_temp_free(diff_operand_sign);
    tcg_temp_free(diff_result_sign);
}

/*
 * In a signed addition, there is an overflow when these conditions are met:
 * (1) Both operands are of the same sign.
 * (2) The result's sign is different from the operand's.
 */
void
arc_gen_add_signed_overflow_tl(TCGv overflow, TCGv result,
                               TCGv op1, TCGv op2, uint8_t operand_size)
{
    TCGv diff_operand_sign = tcg_temp_new();
    TCGv diff_result_sign = tcg_temp_new();

    tcg_gen_xor_tl(diff_operand_sign, op1, op2);
    tcg_gen_xor_tl(diff_result_sign, op1, result);
    /* if (diff_result_sign AND !diff_operand_sign) */
    tcg_gen_andc_tl(overflow, diff_result_sign, diff_operand_sign);

    tcg_gen_shri_tl(overflow, overflow, operand_size - 1);
    tcg_gen_andi_tl(overflow, overflow, 1);

    tcg_temp_free(diff_operand_sign);
    tcg_temp_free(diff_result_sign);
}

/*
 * In a signed addition, there is an overflow when these conditions are met:
 * (1) Both operands are of the same sign.
 * (2) The result's sign is different from the operand's.
 */
void
arc_gen_add_signed_overflow_i64(TCGv_i64 overflow, TCGv_i64 result,
                                TCGv_i64 op1, TCGv_i64 op2)
{
    TCGv_i64 diff_operand_sign = tcg_temp_new_i64();
    TCGv_i64 diff_result_sign = tcg_temp_new_i64();

    tcg_gen_xor_i64(diff_operand_sign, op1, op2);
    tcg_gen_xor_i64(diff_result_sign, op1, result);
    /* if (diff_result_sign AND !diff_operand_sign) */
    tcg_gen_andc_i64(overflow, diff_result_sign, diff_operand_sign);

    tcg_gen_shri_i64(overflow, overflow, 63);
    tcg_gen_andi_i64(overflow, overflow, 1);

    tcg_temp_free_i64(diff_operand_sign);
    tcg_temp_free_i64(diff_result_sign);
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
arc_gen_rotate_right32_i64(TCGv_i64 destination, TCGv_i64 target_operand,
                           TCGv_i64 rotate_by)
{
    TCGv_i64 temp1 = tcg_temp_new_i64();
    TCGv_i64 temp2 = tcg_temp_new_i64();

    // Basic shift
    tcg_gen_shr_i64(temp1, target_operand, rotate_by);
    // Obtain rotated chunk
    tcg_gen_subfi_i64(temp2, 32, rotate_by);
    tcg_gen_shl_i64(temp2, target_operand, temp2);
    // Assemble
    tcg_gen_or_i64(destination, temp1, temp2);
    // Snip out what was shifted outside the 64 bits
    tcg_gen_andi_i64(destination, destination, 0xFFFFFFFF);

    tcg_temp_free_i64(temp1);
    tcg_temp_free_i64(temp2);
}

void
arc_gen_rotate_left32_i64(TCGv_i64 destination, TCGv_i64 target_operand,
                          TCGv_i64 rotate_by)
{
    TCGv_i64 temp1 = tcg_temp_new_i64();
    TCGv_i64 temp2 = tcg_temp_new_i64();

    // Basic shift
    tcg_gen_shl_i64(temp1, target_operand, rotate_by);
    // Obtain rotated chunk
    tcg_gen_subfi_i64(temp2, 32, rotate_by);
    tcg_gen_shr_i64(temp2, target_operand, temp2);
    // Assemble
    tcg_gen_or_i64(destination, temp1, temp2);
    // Snip out what was shifted outside the 64 bits
    tcg_gen_andi_i64(destination, destination, 0xFFFFFFFF);

    tcg_temp_free_i64(temp1);
    tcg_temp_free_i64(temp2);
}

void
arc_gen_arithmetic_shift_right32_i64(TCGv_i64 destination,
                                     TCGv_i64 target_operand,
                                     TCGv_i64 shift_by)
{
    TCGv_i32 temp = tcg_temp_new_i32();
    TCGv_i32 shift_by_i32 = tcg_temp_new_i32();

    // Extract 32 bit values
    tcg_gen_extrl_i64_i32(temp, target_operand);
    tcg_gen_extrl_i64_i32(shift_by_i32, shift_by);
    // Truncate size appropriately
    tcg_gen_andi_i32(shift_by_i32, shift_by_i32, 31);
    // Use built in arithmetic shift
    tcg_gen_sar_i32(temp, temp, shift_by_i32);
    // Unsigned extend into destination register
    tcg_gen_extu_i32_i64(destination, temp);

    tcg_temp_free_i32(shift_by_i32);
    tcg_temp_free_i32(temp);
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

    ARC_GEN_VEC_FIRST_OPERAND(operand_16bit, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, c);

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

    ARC_GEN_VEC_FIRST_OPERAND(operand_32bit, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, c);

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

    ARC_GEN_VEC_FIRST_OPERAND(operand_16bit, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, c);

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

    ARC_GEN_VEC_FIRST_OPERAND(operand_16bit, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, c);

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

    ARC_GEN_VEC_FIRST_OPERAND(operand_16bit, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, c);

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

    ARC_GEN_VEC_FIRST_OPERAND(operand_32bit, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, c);

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

    ARC_GEN_VEC_FIRST_OPERAND(operand_16bit, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, c);

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
    ARC_GEN_VEC_FIRST_OPERAND(operand_64bit, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_64bit, c);

    extract_bits(b, b, 0, 32);
    extract_bits(c, c, 0, 32);

    tcg_gen_mul_i64(acc, b, c);

    tcg_gen_mov_i64(a, acc);

    arc_gen_mpy_check_fflags(ctx, a, set_n_flag);

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