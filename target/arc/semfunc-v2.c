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

/**
 * Generates the code for setting up a 64 bit register from a 32 bit one
 * Either by concatenating a pair or 0 extending it directly
 */
#define ARC_GEN_SRC_PAIR_UNSIGNED(REGISTER) \
    arc_gen_next_register_i32_i64(ctx, r64_##REGISTER, REGISTER);

#define ARC_GEN_SRC_PAIR_SIGNED(REGISTER) \
    arc_gen_next_register_i32_i64(ctx, r64_##REGISTER, REGISTER);

#define ARC_GEN_SRC_NOT_PAIR_SIGNED(REGISTER) \
    tcg_gen_ext_i32_i64(r64_##REGISTER, REGISTER);

#define ARC_GEN_SRC_NOT_PAIR_UNSIGNED(REGISTER) \
    tcg_gen_extu_i32_i64(r64_##REGISTER, REGISTER);

#define ARC_GEN_DST_PAIR(REGISTER) \
    tcg_gen_extr_i64_i32(REGISTER, nextRegWithNull(REGISTER), r64_##REGISTER);

#define ARC_GEN_DST_NOT_PAIR(REGISTER) \
    tcg_gen_extrl_i64_i32(REGISTER, r64_##REGISTER);

#define VEC_SIGNED_PARAMS true, tcg_gen_sextract_i64, \
                                arc_gen_add_signed_overflow_i64

#define VEC_UNSIGNED_PARAMS false, tcg_gen_extract_i64, \
                                   arc_gen_add_unsigned_overflow_i64

/**
 * Generate the function call for signed/unsigned instructions
 */
#define ARC_GEN_BASE32_64_SIGNED(OPERATION) \
OPERATION(ctx, r64_a, r64_b, r64_c, acc, VEC_SIGNED_PARAMS)

#define ARC_GEN_BASE32_64_UNSIGNED(OPERATION) \
OPERATION(ctx, r64_a, r64_b, r64_c , acc, VEC_UNSIGNED_PARAMS)

/**
 * Generate a function to be used by 32 bit versions to interface with
 * their 64 bit counterparts.
 * It is assumed the accumulator is always a pair register and always updated
 */
#define ARC_GEN_32BIT_INTERFACE(NAME, A_REG_INFO, B_REG_INFO, C_REG_INFO,  \
                                IS_SIGNED, OPERATION)                      \
static inline void                                                         \
arc_autogen_base32_##NAME(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)          \
{                                                                          \
    TCGv_i64 r64_a = tcg_temp_new_i64();                                   \
    TCGv_i64 r64_b = tcg_temp_new_i64();                                   \
    TCGv_i64 r64_c = tcg_temp_new_i64();                                   \
    TCGv_i64 acc = tcg_temp_new_i64();                                     \
    ARC_GEN_SRC_ ## B_REG_INFO ## _ ## IS_SIGNED(b);                       \
    ARC_GEN_SRC_ ## C_REG_INFO ## _ ## IS_SIGNED(c);                       \
    tcg_gen_concat_i32_i64(acc, cpu_acclo, cpu_acchi);                     \
    ARC_GEN_BASE32_64_##IS_SIGNED(OPERATION);                              \
    tcg_gen_extr_i64_i32(cpu_acclo, cpu_acchi, acc);                       \
    ARC_GEN_DST_##A_REG_INFO(a);                                           \
    tcg_temp_free_i64(acc);                                                \
    tcg_temp_free_i64(r64_a);                                              \
    tcg_temp_free_i64(r64_b);                                              \
    tcg_temp_free_i64(r64_c);                                              \
}

/*
 * Generate a function to be used by 32 bit fpu instructions to interface
 * with the appropriate 'a = op(b, c)' 64 bit helpers
 */
#define ARC_GEN_32BIT_FLOAT_INTERFACE3(NAME, A_REG_INFO, B_REG_INFO,       \
                                       C_REG_INFO, HELPER)                 \
static inline void                                                         \
arc_autogen_base32_##NAME(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)          \
{                                                                          \
    TCGv_i64 r64_a = tcg_temp_new_i64();                                   \
    TCGv_i64 r64_b = tcg_temp_new_i64();                                   \
    TCGv_i64 r64_c = tcg_temp_new_i64();                                   \
    ARC_GEN_SRC_ ## B_REG_INFO ## _UNSIGNED(b);                            \
    ARC_GEN_SRC_ ## C_REG_INFO ## _UNSIGNED(c);                            \
    HELPER(r64_a, cpu_env, r64_b, r64_c);                                  \
    ARC_GEN_DST_##A_REG_INFO(a);                                           \
    tcg_temp_free_i64(r64_a);                                              \
    tcg_temp_free_i64(r64_b);                                              \
    tcg_temp_free_i64(r64_c);                                              \
}

/*
 * Generate a function to be used by 32 bit fpu instructions to interface
 * with the appropriate 'a = op(b)' 64 bit helpers
 */
#define ARC_GEN_32BIT_FLOAT_CMP_INTERFACE2(HELPER, A_REG_INFO, B_REG_INFO)   \
static inline void                                                           \
arc_autogen_base32_##HELPER(DisasCtxt *ctx, TCGv a, TCGv b)                  \
{                                                                            \
    TCGv_i64 r64_a = tcg_temp_new_i64();                                     \
    TCGv_i64 r64_b = tcg_temp_new_i64();                                     \
    ARC_GEN_SRC_ ## A_REG_INFO ## _UNSIGNED(a);                              \
    ARC_GEN_SRC_ ## B_REG_INFO ## _UNSIGNED(b);                              \
    gen_helper_ ## HELPER(cpu_env, r64_a, r64_b);                            \
    tcg_temp_free_i64(r64_a);                                                \
    tcg_temp_free_i64(r64_b);                                                \
}

/*
 * FLAG
 *    Variables: @src
 *    Functions: getCCFlag, arc_gen_get_register, getBit, hasInterrupts, Halt, ReplMask,
 *               targetHasOption, setRegister
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       status32 = arc_gen_get_register (R_STATUS32);
 *       if(((getBit (@src, 0) == 1) && (getBit (status32, 7) == 0)))
 *         {
 *           if((hasInterrupts () > 0))
 *             {
 *               status32 = (status32 | 1);
 *               Halt ();
 *             };
 *         }
 *       else
 *         {
 *           ReplMask (status32, @src, 3840);
 *           if(((getBit (status32, 7) == 0) && (hasInterrupts () > 0)))
 *             {
 *               ReplMask (status32, @src, 30);
 *               if(targetHasOption (DIV_REM_OPTION))
 *                 {
 *                   ReplMask (status32, @src, 8192);
 *                 };
 *               if(targetHasOption (STACK_CHECKING))
 *                 {
 *                   ReplMask (status32, @src, 16384);
 *                 };
 *               if(targetHasOption (LL64_OPTION))
 *                 {
 *                   ReplMask (status32, @src, 524288);
 *                 };
 *               ReplMask (status32, @src, 1048576);
 *             };
 *         };
 *       setRegister (R_STATUS32, status32);
 *     };
 * }
 */

int
arc_gen_FLAG(DisasCtxt *ctx, TCGv src)
{
    int ret = DISAS_UPDATE;
    TCGv temp_13 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_14 = tcg_temp_local_new();
    TCGv status32 = tcg_temp_local_new();
    TCGv temp_16 = tcg_temp_local_new();
    TCGv temp_15 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_18 = tcg_temp_local_new();
    TCGv temp_17 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_19 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_20 = tcg_temp_local_new();
    TCGv temp_22 = tcg_temp_local_new();
    TCGv temp_21 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_23 = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_11 = tcg_temp_local_new();
    TCGv temp_12 = tcg_temp_local_new();
    TCGv temp_24 = tcg_temp_local_new();
    TCGv temp_25 = tcg_temp_local_new();
    TCGv temp_26 = tcg_temp_local_new();
    TCGv temp_27 = tcg_temp_local_new();
    TCGv temp_28 = tcg_temp_local_new();
    getCCFlag(temp_13);
    tcg_gen_mov_tl(cc_flag, temp_13);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    gen_helper_get_status32(temp_14, cpu_env);
    tcg_gen_mov_tl(status32, temp_14);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_movi_tl(temp_16, 0);
    getBit(temp_15, src, temp_16);
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, temp_15, 1);
    tcg_gen_movi_tl(temp_18, 7);
    getBit(temp_17, status32, temp_18);
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_4, temp_17, 0);
    tcg_gen_and_tl(temp_5, temp_3, temp_4);
    tcg_gen_xori_tl(temp_6, temp_5, 1);
    tcg_gen_andi_tl(temp_6, temp_6, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_2);
    TCGLabel *done_3 = gen_new_label();
    hasInterrupts(temp_19);
    tcg_gen_setcondi_tl(TCG_COND_GT, temp_7, temp_19, 0);
    tcg_gen_xori_tl(temp_8, temp_7, 1);
    tcg_gen_andi_tl(temp_8, temp_8, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_8, arc_true, done_3);
    tcg_gen_ori_tl(status32, status32, 1);
    Halt();
    gen_set_label(done_3);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    tcg_gen_movi_tl(temp_20, 3840);
    ReplMask(status32, src, temp_20);
    TCGLabel *done_4 = gen_new_label();
    tcg_gen_movi_tl(temp_22, 7);
    getBit(temp_21, status32, temp_22);
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_9, temp_21, 0);
    hasInterrupts(temp_23);
    tcg_gen_setcondi_tl(TCG_COND_GT, temp_10, temp_23, 0);
    tcg_gen_and_tl(temp_11, temp_9, temp_10);
    tcg_gen_xori_tl(temp_12, temp_11, 1);
    tcg_gen_andi_tl(temp_12, temp_12, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_12, arc_true, done_4);
    tcg_gen_movi_tl(temp_24, 30);
    ReplMask(status32, src, temp_24);
    if (targetHasOption (DIV_REM_OPTION)) {
        tcg_gen_movi_tl(temp_25, 8192);
        ReplMask(status32, src, temp_25);
    }
    if (targetHasOption (STACK_CHECKING)) {
        tcg_gen_movi_tl(temp_26, 16384);
        ReplMask(status32, src, temp_26);
    }
    if (targetHasOption (LL64_OPTION)) {
        tcg_gen_movi_tl(temp_27, 524288);
        ReplMask(status32, src, temp_27);
    }
    tcg_gen_movi_tl(temp_28, 1048576);
    ReplMask(status32, src, temp_28);
    gen_set_label(done_4);
    gen_set_label(done_2);
    gen_helper_set_status32(cpu_env, status32);
    gen_set_label(done_1);
    tcg_temp_free(temp_13);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_14);
    tcg_temp_free(status32);
    tcg_temp_free(temp_16);
    tcg_temp_free(temp_15);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_18);
    tcg_temp_free(temp_17);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_19);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_20);
    tcg_temp_free(temp_22);
    tcg_temp_free(temp_21);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_23);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_11);
    tcg_temp_free(temp_12);
    tcg_temp_free(temp_24);
    tcg_temp_free(temp_25);
    tcg_temp_free(temp_26);
    tcg_temp_free(temp_27);
    tcg_temp_free(temp_28);

    return ret;
}


/*
 * KFLAG
 *    Variables: @src
 *    Functions: getCCFlag, arc_gen_get_register, getBit, hasInterrupts, Halt, ReplMask,
 *               targetHasOption, setRegister
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       status32 = arc_gen_get_register (R_STATUS32);
 *       if(((getBit (@src, 0) == 1) && (getBit (status32, 7) == 0)))
 *         {
 *           if((hasInterrupts () > 0))
 *             {
 *               status32 = (status32 | 1);
 *               Halt ();
 *             };
 *         }
 *       else
 *         {
 *           ReplMask (status32, @src, 3840);
 *           if(((getBit (status32, 7) == 0) && (hasInterrupts () > 0)))
 *             {
 *               ReplMask (status32, @src, 62);
 *               if(targetHasOption (DIV_REM_OPTION))
 *                 {
 *                   ReplMask (status32, @src, 8192);
 *                 };
 *               if(targetHasOption (STACK_CHECKING))
 *                 {
 *                   ReplMask (status32, @src, 16384);
 *                 };
 *               ReplMask (status32, @src, 65536);
 *               if(targetHasOption (LL64_OPTION))
 *                 {
 *                   ReplMask (status32, @src, 524288);
 *                 };
 *               ReplMask (status32, @src, 1048576);
 *               ReplMask (status32, @src, 2147483648);
 *             };
 *         };
 *       setRegister (R_STATUS32, status32);
 *     };
 * }
 */

int
arc_gen_KFLAG(DisasCtxt *ctx, TCGv src)
{
    int ret = DISAS_UPDATE;
    TCGv temp_13 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_14 = tcg_temp_local_new();
    TCGv status32 = tcg_temp_local_new();
    TCGv temp_16 = tcg_temp_local_new();
    TCGv temp_15 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_18 = tcg_temp_local_new();
    TCGv temp_17 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_19 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_20 = tcg_temp_local_new();
    TCGv temp_22 = tcg_temp_local_new();
    TCGv temp_21 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_23 = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_11 = tcg_temp_local_new();
    TCGv temp_12 = tcg_temp_local_new();
    TCGv temp_24 = tcg_temp_local_new();
    TCGv temp_25 = tcg_temp_local_new();
    TCGv temp_26 = tcg_temp_local_new();
    TCGv temp_27 = tcg_temp_local_new();
    TCGv temp_28 = tcg_temp_local_new();
    TCGv temp_29 = tcg_temp_local_new();
    TCGv temp_30 = tcg_temp_local_new();
    getCCFlag(temp_13);
    tcg_gen_mov_tl(cc_flag, temp_13);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    gen_helper_get_status32(temp_14, cpu_env);
    tcg_gen_mov_tl(status32, temp_14);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_movi_tl(temp_16, 0);
    getBit(temp_15, src, temp_16);
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, temp_15, 1);
    tcg_gen_movi_tl(temp_18, 7);
    getBit(temp_17, status32, temp_18);
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_4, temp_17, 0);
    tcg_gen_and_tl(temp_5, temp_3, temp_4);
    tcg_gen_xori_tl(temp_6, temp_5, 1);
    tcg_gen_andi_tl(temp_6, temp_6, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_2);
    TCGLabel *done_3 = gen_new_label();
    hasInterrupts(temp_19);
    tcg_gen_setcondi_tl(TCG_COND_GT, temp_7, temp_19, 0);
    tcg_gen_xori_tl(temp_8, temp_7, 1);
    tcg_gen_andi_tl(temp_8, temp_8, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_8, arc_true, done_3);
    tcg_gen_ori_tl(status32, status32, 1);
    Halt();
    gen_set_label(done_3);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    tcg_gen_movi_tl(temp_20, 3840);
    ReplMask(status32, src, temp_20);
    TCGLabel *done_4 = gen_new_label();
    tcg_gen_movi_tl(temp_22, 7);
    getBit(temp_21, status32, temp_22);
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_9, temp_21, 0);
    hasInterrupts(temp_23);
    tcg_gen_setcondi_tl(TCG_COND_GT, temp_10, temp_23, 0);
    tcg_gen_and_tl(temp_11, temp_9, temp_10);
    tcg_gen_xori_tl(temp_12, temp_11, 1);
    tcg_gen_andi_tl(temp_12, temp_12, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_12, arc_true, done_4);
    tcg_gen_movi_tl(temp_24, 62);
    ReplMask(status32, src, temp_24);
    if (targetHasOption (DIV_REM_OPTION)) {
        tcg_gen_movi_tl(temp_25, 8192);
        ReplMask(status32, src, temp_25);
    }
    if (targetHasOption (STACK_CHECKING)) {
        tcg_gen_movi_tl(temp_26, 16384);
        ReplMask(status32, src, temp_26);
    }
    tcg_gen_movi_tl(temp_27, 65536);
    ReplMask(status32, src, temp_27);
    if (targetHasOption (LL64_OPTION)) {
        tcg_gen_movi_tl(temp_28, 524288);
        ReplMask(status32, src, temp_28);
    }
    tcg_gen_movi_tl(temp_29, 1048576);
    ReplMask(status32, src, temp_29);
    tcg_gen_movi_tl(temp_30, 2147483648);
    ReplMask(status32, src, temp_30);
    gen_set_label(done_4);
    gen_set_label(done_2);
    gen_helper_set_status32(cpu_env, status32);
    gen_set_label(done_1);
    tcg_temp_free(temp_13);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_14);
    tcg_temp_free(status32);
    tcg_temp_free(temp_16);
    tcg_temp_free(temp_15);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_18);
    tcg_temp_free(temp_17);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_19);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_20);
    tcg_temp_free(temp_22);
    tcg_temp_free(temp_21);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_23);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_11);
    tcg_temp_free(temp_12);
    tcg_temp_free(temp_24);
    tcg_temp_free(temp_25);
    tcg_temp_free(temp_26);
    tcg_temp_free(temp_27);
    tcg_temp_free(temp_28);
    tcg_temp_free(temp_29);
    tcg_temp_free(temp_30);

    return ret;
}


/*
 * ADD
 * --- code ---
 * if (ctx->insn.cc)
 * {
 *   if (ctx->insn.ff)
 *   {
 *     msb_b = b >> (target_bits-1)
 *     msb_c = c >> (target_bits-1)
 *   }
 *
 *   a = b + c
 *
 *   if (ctx->insn.ff)
 *   {
 *     msb_a = a >> (target_bits-1)
 *
 *     cpu_z = (a == 0)
 *     cpu_n = msb_a
 *     cpu_c = (msb_b & msb_c) | (msb_b & ~msb_a) | (msb_c & ~msb_a)
 *     cpu_v = xnor(msb_b, msb_c) & xor(msb_b, msb_a)
 *   }
 * }
 */

int
arc_gen_ADD(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    TCGLabel *skip = gen_new_label();
    TCGv cc = tcg_temp_new();
    TCGv msb_a = tcg_temp_new();
    TCGv msb_b = tcg_temp_new();
    TCGv msb_c = tcg_temp_new();
    TCGv t = tcg_temp_new();

    if (ctx->insn.cc != ARC_COND_AL) {
        arc_gen_verifyCCFlag(ctx, cc);
        tcg_gen_brcondi_tl(TCG_COND_NE, cc, 1, skip);
    }

    /* Record the input operands MSB, if we have to update the flags. */
    if (ctx->insn.f) {
        tcg_gen_shri_tl(msb_b, b, TARGET_LONG_BITS - 1);
        tcg_gen_shri_tl(msb_c, c, TARGET_LONG_BITS - 1);
    }

    tcg_gen_add_tl(a, b, c);

    if (ctx->insn.f) {
        tcg_gen_shri_tl(msb_a, a, TARGET_LONG_BITS - 1);
        tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_Zf, a, 0);
        tcg_gen_mov_tl(cpu_Nf, msb_a);

        /* carry = (msb_b & msb_c) | (msb_b & ~msb_a) | (msb_c & ~msb_a) */
        tcg_gen_and_tl(cpu_Cf, msb_b, msb_c);
        tcg_gen_andc_tl(t, msb_b, msb_a);
        tcg_gen_or_tl(cpu_Cf, cpu_Cf, t);
        tcg_gen_andc_tl(t, msb_c, msb_a);
        tcg_gen_or_tl(cpu_Cf, cpu_Cf, t);

        /*
         * overflow = xnor(b,c) & xor(b,a)
         *
         * xnor(b,c) is 1 if and only if b and c are the same.
         * xor(b,a)  is 1 if and only if they are not the same.
         *
         * In other words, if both operands have the same sign,
         * but the result has a different one, that's an overflow.
         */
        tcg_gen_xor_tl(cpu_Vf, msb_b, msb_c);
        tcg_gen_not_tl(cpu_Vf, cpu_Vf);
        tcg_gen_andi_tl(cpu_Vf, cpu_Vf, 1);
        tcg_gen_xor_tl(t, msb_b, msb_a);
        tcg_gen_and_tl(cpu_Vf, cpu_Vf, t);
    }

    gen_set_label(skip);
    tcg_temp_free(t);
    tcg_temp_free(msb_c);
    tcg_temp_free(msb_b);
    tcg_temp_free(msb_a);
    tcg_temp_free(cc);

    return DISAS_NEXT;
}

/*
 * ADD1
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarryADD,
 *               setVFlag, OverflowADD
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   lc = @c << 1;
 *   if((cc_flag == true))
 *     {
 *       @a = (@b + lc);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setCFlag (CarryADD (@a, lb, lc));
 *           setVFlag (OverflowADD (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_ADD1(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_shli_tl(lc, c, 1);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_add_tl(a, b, lc);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        CarryADD(temp_5, a, lb, lc);
        tcg_gen_mov_tl(temp_4, temp_5);
        setCFlag(temp_4);
        OverflowADD(temp_7, a, lb, lc);
        tcg_gen_mov_tl(temp_6, temp_7);
        setVFlag(temp_6);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(lc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * ADD2
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarryADD,
 *               setVFlag, OverflowADD
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   lc = @c << 2;
 *   if((cc_flag == true))
 *     {
 *       @a = (@b + lc);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setCFlag (CarryADD (@a, lb, lc));
 *           setVFlag (OverflowADD (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_ADD2(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_shli_tl(lc, c, 2);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_add_tl(a, b, lc);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        CarryADD(temp_5, a, lb, lc);
        tcg_gen_mov_tl(temp_4, temp_5);
        setCFlag(temp_4);
        OverflowADD(temp_7, a, lb, lc);
        tcg_gen_mov_tl(temp_6, temp_7);
        setVFlag(temp_6);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(lc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * ADD3
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarryADD,
 *               setVFlag, OverflowADD
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   lc = @c << 3;
 *   if((cc_flag == true))
 *     {
 *       @a = (@b + lc);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setCFlag (CarryADD (@a, lb, lc));
 *           setVFlag (OverflowADD (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_ADD3(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_shli_tl(lc, c, 3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_add_tl(a, b, lc);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        CarryADD(temp_5, a, lb, lc);
        tcg_gen_mov_tl(temp_4, temp_5);
        setCFlag(temp_4);
        OverflowADD(temp_7, a, lb, lc);
        tcg_gen_mov_tl(temp_6, temp_7);
        setVFlag(temp_6);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(lc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * ADC
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getCFlag, getFFlag, setZFlag, setNFlag, setCFlag,
 *               CarryADD, setVFlag, OverflowADD
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   lc = @c;
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       lc = @c;
 *       @a = ((@b + @c) + getCFlag ());
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setCFlag (CarryADD (@a, lb, lc));
 *           setVFlag (OverflowADD (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_ADC(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_mov_tl(lc, c);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_mov_tl(lc, c);
    tcg_gen_add_tl(temp_4, b, c);
    getCFlag(temp_6);
    tcg_gen_mov_tl(temp_5, temp_6);
    tcg_gen_add_tl(a, temp_4, temp_5);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        CarryADD(temp_8, a, lb, lc);
        tcg_gen_mov_tl(temp_7, temp_8);
        setCFlag(temp_7);
        OverflowADD(temp_10, a, lb, lc);
        tcg_gen_mov_tl(temp_9, temp_10);
        setVFlag(temp_9);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(lc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_9);

    return ret;
}


/*
 * SBC
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getCFlag, getFFlag, setZFlag, setNFlag, setCFlag,
 *               CarrySUB, setVFlag, OverflowSUB
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   lc = @c;
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       lc = @c;
 *       @a = ((@b - @c) - getCFlag ());
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setCFlag (CarrySUB (@a, lb, lc));
 *           setVFlag (OverflowSUB (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_SBC(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_mov_tl(lc, c);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_mov_tl(lc, c);
    tcg_gen_sub_tl(temp_4, b, c);
    getCFlag(temp_6);
    tcg_gen_mov_tl(temp_5, temp_6);
    tcg_gen_sub_tl(a, temp_4, temp_5);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        CarrySUB(temp_8, a, lb, lc);
        tcg_gen_mov_tl(temp_7, temp_8);
        setCFlag(temp_7);
        OverflowSUB(temp_10, a, lb, lc);
        tcg_gen_mov_tl(temp_9, temp_10);
        setVFlag(temp_9);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(lc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_9);

    return ret;
}


/*
 * NEG
 *    Variables: @b, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB,
 *               setVFlag, OverflowSUB
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       @a = (0 - @b);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setCFlag (CarrySUB (@a, 0, lb));
 *           setVFlag (OverflowSUB (@a, 0, lb));
 *         };
 *     };
 * }
 */

int
arc_gen_NEG(DisasCtxt *ctx, TCGv b, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    tcg_gen_mov_tl(lb, b);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_subfi_tl(a, 0, b);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        tcg_gen_movi_tl(temp_6, 0);
        CarrySUB(temp_5, a, temp_6, lb);
        tcg_gen_mov_tl(temp_4, temp_5);
        setCFlag(temp_4);
        tcg_gen_movi_tl(temp_9, 0);
        OverflowSUB(temp_8, a, temp_9, lb);
        tcg_gen_mov_tl(temp_7, temp_8);
        setVFlag(temp_7);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_7);

    return ret;
}


/*
 * SUB
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB,
 *               setVFlag, OverflowSUB
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       lc = @c;
 *       @a = (@b - @c);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setCFlag (CarrySUB (@a, lb, lc));
 *           setVFlag (OverflowSUB (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_SUB(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    tcg_gen_mov_tl(lb, b);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_mov_tl(lc, c);
    tcg_gen_sub_tl(a, b, c);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        CarrySUB(temp_5, a, lb, lc);
        tcg_gen_mov_tl(temp_4, temp_5);
        setCFlag(temp_4);
        OverflowSUB(temp_7, a, lb, lc);
        tcg_gen_mov_tl(temp_6, temp_7);
        setVFlag(temp_6);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lc);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * SUB1
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB,
 *               setVFlag, OverflowSUB
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       lc = (@c << 1);
 *       @a = (@b - lc);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setCFlag (CarrySUB (@a, lb, lc));
 *           setVFlag (OverflowSUB (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_SUB1(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    tcg_gen_mov_tl(lb, b);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_shli_tl(lc, c, 1);
    tcg_gen_sub_tl(a, b, lc);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        CarrySUB(temp_5, a, lb, lc);
        tcg_gen_mov_tl(temp_4, temp_5);
        setCFlag(temp_4);
        OverflowSUB(temp_7, a, lb, lc);
        tcg_gen_mov_tl(temp_6, temp_7);
        setVFlag(temp_6);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lc);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * SUB2
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB,
 *               setVFlag, OverflowSUB
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       lc = (@c << 2);
 *       @a = (@b - lc);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setCFlag (CarrySUB (@a, lb, lc));
 *           setVFlag (OverflowSUB (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_SUB2(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    tcg_gen_mov_tl(lb, b);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_shli_tl(lc, c, 2);
    tcg_gen_sub_tl(a, b, lc);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        CarrySUB(temp_5, a, lb, lc);
        tcg_gen_mov_tl(temp_4, temp_5);
        setCFlag(temp_4);
        OverflowSUB(temp_7, a, lb, lc);
        tcg_gen_mov_tl(temp_6, temp_7);
        setVFlag(temp_6);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lc);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * SUB3
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB,
 *               setVFlag, OverflowSUB
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       lc = (@c << 3);
 *       @a = (@b - lc);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setCFlag (CarrySUB (@a, lb, lc));
 *           setVFlag (OverflowSUB (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_SUB3(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    tcg_gen_mov_tl(lb, b);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_shli_tl(lc, c, 3);
    tcg_gen_sub_tl(a, b, lc);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        CarrySUB(temp_5, a, lb, lc);
        tcg_gen_mov_tl(temp_4, temp_5);
        setCFlag(temp_4);
        OverflowSUB(temp_7, a, lb, lc);
        tcg_gen_mov_tl(temp_6, temp_7);
        setVFlag(temp_6);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lc);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * MAX
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB,
 *               setVFlag, OverflowSUB
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       lc = @c;
 *       alu = (lb - lc);
 *       if((lc >= lb))
 *         {
 *           @a = lc;
 *         }
 *       else
 *         {
 *           @a = lb;
 *         };
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (alu);
 *           setNFlag (alu);
 *           setCFlag (CarrySUB (@a, lb, lc));
 *           setVFlag (OverflowSUB (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_MAX(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_5 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv alu = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    getCCFlag(temp_5);
    tcg_gen_mov_tl(cc_flag, temp_5);
    tcg_gen_mov_tl(lb, b);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_mov_tl(lc, c);
    tcg_gen_sub_tl(alu, lb, lc);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_GE, temp_3, lc, lb);
    tcg_gen_xori_tl(temp_4, temp_3, 1);
    tcg_gen_andi_tl(temp_4, temp_4, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
    tcg_gen_mov_tl(a, lc);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    tcg_gen_mov_tl(a, lb);
    gen_set_label(done_2);
    if ((getFFlag () == true)) {
        setZFlag(alu);
        setNFlag(alu);
        CarrySUB(temp_7, a, lb, lc);
        tcg_gen_mov_tl(temp_6, temp_7);
        setCFlag(temp_6);
        OverflowSUB(temp_9, a, lb, lc);
        tcg_gen_mov_tl(temp_8, temp_9);
        setVFlag(temp_8);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_5);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lc);
    tcg_temp_free(alu);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_8);

    return ret;
}


/*
 * MIN
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB,
 *               setVFlag, OverflowSUB
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       lc = @c;
 *       alu = (lb - lc);
 *       if((lc <= lb))
 *         {
 *           @a = lc;
 *         }
 *       else
 *         {
 *           @a = lb;
 *         };
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (alu);
 *           setNFlag (alu);
 *           setCFlag (CarrySUB (@a, lb, lc));
 *           setVFlag (OverflowSUB (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_MIN(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_5 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv alu = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    getCCFlag(temp_5);
    tcg_gen_mov_tl(cc_flag, temp_5);
    tcg_gen_mov_tl(lb, b);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_mov_tl(lc, c);
    tcg_gen_sub_tl(alu, lb, lc);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_LE, temp_3, lc, lb);
    tcg_gen_xori_tl(temp_4, temp_3, 1);
    tcg_gen_andi_tl(temp_4, temp_4, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
    tcg_gen_mov_tl(a, lc);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    tcg_gen_mov_tl(a, lb);
    gen_set_label(done_2);
    if ((getFFlag () == true)) {
        setZFlag(alu);
        setNFlag(alu);
        CarrySUB(temp_7, a, lb, lc);
        tcg_gen_mov_tl(temp_6, temp_7);
        setCFlag(temp_6);
        OverflowSUB(temp_9, a, lb, lc);
        tcg_gen_mov_tl(temp_8, temp_9);
        setVFlag(temp_8);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_5);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lc);
    tcg_temp_free(alu);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_8);

    return ret;
}


/*
 * CMP
 *    Variables: @b, @c
 *    Functions: getCCFlag, setZFlag, setNFlag, setCFlag, CarrySUB, setVFlag,
 *               OverflowSUB
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       alu = (@b - @c);
 *       setZFlag (alu);
 *       setNFlag (alu);
 *       setCFlag (CarrySUB (alu, @b, @c));
 *       setVFlag (OverflowSUB (alu, @b, @c));
 *     };
 * }
 */

int
arc_gen_CMP(DisasCtxt *ctx, TCGv b, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv alu = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_sub_tl(alu, b, c);
    setZFlag(alu);
    setNFlag(alu);
    CarrySUB(temp_5, alu, b, c);
    tcg_gen_mov_tl(temp_4, temp_5);
    setCFlag(temp_4);
    OverflowSUB(temp_7, alu, b, c);
    tcg_gen_mov_tl(temp_6, temp_7);
    setVFlag(temp_6);
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(alu);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * AND
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       la = (@b & @c);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_AND(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_and_tl(la, b, c);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(la);

    return ret;
}


/*
 * OR
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       la = (@b | @c);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_OR(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_or_tl(la, b, c);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(la);

    return ret;
}


/*
 * XOR
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       la = (@b ^ @c);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_XOR(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_xor_tl(la, b, c);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(la);

    return ret;
}


/*
 * MOV
 *    Variables: @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       la = @b;
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_MOV(DisasCtxt *ctx, TCGv a, TCGv b)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(la, b);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(la);

    return ret;
}


/*
 * ASL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, getBit,
 *               setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       lc = (@c & 31);
 *       la = (lb << lc);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *           if((lc == 0))
 *             {
 *               setCFlag (0);
 *             }
 *           else
 *             {
 *               setCFlag (getBit (lb, (32 - lc)));
 *             };
 *           if((@c == 268435457))
 *             {
 *               t1 = getBit (la, 31);
 *               t2 = getBit (lb, 31);
 *               if((t1 == t2))
 *                 {
 *                   setVFlag (0);
 *                 }
 *               else
 *                 {
 *                   setVFlag (1);
 *                 };
 *             };
 *         };
 *     };
 * }
 */

int
arc_gen_ASL(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_9 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_13 = tcg_temp_local_new();
    TCGv temp_12 = tcg_temp_local_new();
    TCGv temp_11 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_15 = tcg_temp_local_new();
    TCGv temp_14 = tcg_temp_local_new();
    TCGv t1 = tcg_temp_local_new();
    TCGv temp_17 = tcg_temp_local_new();
    TCGv temp_16 = tcg_temp_local_new();
    TCGv t2 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_18 = tcg_temp_local_new();
    TCGv temp_19 = tcg_temp_local_new();
    getCCFlag(temp_9);
    tcg_gen_mov_tl(cc_flag, temp_9);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_andi_tl(lc, c, 31);
    tcg_gen_shl_tl(la, lb, lc);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
        TCGLabel *else_2 = gen_new_label();
        TCGLabel *done_2 = gen_new_label();
        tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, lc, 0);
        tcg_gen_xori_tl(temp_4, temp_3, 1);
        tcg_gen_andi_tl(temp_4, temp_4, 1);
        tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
        tcg_gen_movi_tl(temp_10, 0);
        setCFlag(temp_10);
        tcg_gen_br(done_2);
        gen_set_label(else_2);
        tcg_gen_subfi_tl(temp_13, 32, lc);
        getBit(temp_12, lb, temp_13);
        tcg_gen_mov_tl(temp_11, temp_12);
        setCFlag(temp_11);
        gen_set_label(done_2);
        TCGLabel *done_3 = gen_new_label();
        tcg_gen_setcondi_tl(TCG_COND_EQ, temp_5, c, 268435457);
        tcg_gen_xori_tl(temp_6, temp_5, 1);
        tcg_gen_andi_tl(temp_6, temp_6, 1);
        tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, done_3);
        tcg_gen_movi_tl(temp_15, 31);
        getBit(temp_14, la, temp_15);
        tcg_gen_mov_tl(t1, temp_14);
        tcg_gen_movi_tl(temp_17, 31);
        getBit(temp_16, lb, temp_17);
        tcg_gen_mov_tl(t2, temp_16);
        TCGLabel *else_4 = gen_new_label();
        TCGLabel *done_4 = gen_new_label();
        tcg_gen_setcond_tl(TCG_COND_EQ, temp_7, t1, t2);
        tcg_gen_xori_tl(temp_8, temp_7, 1);
        tcg_gen_andi_tl(temp_8, temp_8, 1);
        tcg_gen_brcond_tl(TCG_COND_EQ, temp_8, arc_true, else_4);
        tcg_gen_movi_tl(temp_18, 0);
        setVFlag(temp_18);
        tcg_gen_br(done_4);
        gen_set_label(else_4);
        tcg_gen_movi_tl(temp_19, 1);
        setVFlag(temp_19);
        gen_set_label(done_4);
        gen_set_label(done_3);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_9);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lb);
    tcg_temp_free(lc);
    tcg_temp_free(la);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_13);
    tcg_temp_free(temp_12);
    tcg_temp_free(temp_11);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_15);
    tcg_temp_free(temp_14);
    tcg_temp_free(t1);
    tcg_temp_free(temp_17);
    tcg_temp_free(temp_16);
    tcg_temp_free(t2);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_18);
    tcg_temp_free(temp_19);

    return ret;
}


/*
 * ASR
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, arithmeticShiftRight, getFFlag, setZFlag, setNFlag,
 *               setCFlag, getBit
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       lc = (@c & 31);
 *       la = arithmeticShiftRight (lb, lc);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *           if((lc == 0))
 *             {
 *               setCFlag (0);
 *             }
 *           else
 *             {
 *               setCFlag (getBit (lb, (lc - 1)));
 *             };
 *         };
 *     };
 * }
 */

int
arc_gen_ASR(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_5 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    getCCFlag(temp_5);
    tcg_gen_mov_tl(cc_flag, temp_5);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_andi_tl(lc, c, 31);
    arithmeticShiftRight(temp_6, lb, lc);
    tcg_gen_mov_tl(la, temp_6);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
        TCGLabel *else_2 = gen_new_label();
        TCGLabel *done_2 = gen_new_label();
        tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, lc, 0);
        tcg_gen_xori_tl(temp_4, temp_3, 1);
        tcg_gen_andi_tl(temp_4, temp_4, 1);
        tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
        tcg_gen_movi_tl(temp_7, 0);
        setCFlag(temp_7);
        tcg_gen_br(done_2);
        gen_set_label(else_2);
        tcg_gen_subi_tl(temp_10, lc, 1);
        getBit(temp_9, lb, temp_10);
        tcg_gen_mov_tl(temp_8, temp_9);
        setCFlag(temp_8);
        gen_set_label(done_2);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_5);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lb);
    tcg_temp_free(lc);
    tcg_temp_free(temp_6);
    tcg_temp_free(la);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_8);

    return ret;
}


/*
 * ASR8
 *    Variables: @b, @a
 *    Functions: getCCFlag, arithmeticShiftRight, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       la = arithmeticShiftRight (lb, 8);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_ASR8(DisasCtxt *ctx, TCGv b, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_movi_tl(temp_5, 8);
    arithmeticShiftRight(temp_4, lb, temp_5);
    tcg_gen_mov_tl(la, temp_4);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lb);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(la);

    return ret;
}


/*
 * ASR16
 *    Variables: @b, @a
 *    Functions: getCCFlag, arithmeticShiftRight, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       la = arithmeticShiftRight (lb, 16);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_ASR16(DisasCtxt *ctx, TCGv b, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_movi_tl(temp_5, 16);
    arithmeticShiftRight(temp_4, lb, temp_5);
    tcg_gen_mov_tl(la, temp_4);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lb);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(la);

    return ret;
}


/*
 * LSL16
 *    Variables: @b, @a
 *    Functions: getCCFlag, logicalShiftLeft, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       la = logicalShiftLeft (@b, 16);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_LSL16(DisasCtxt *ctx, TCGv b, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_movi_tl(temp_5, 16);
    logicalShiftLeft(temp_4, b, temp_5);
    tcg_gen_mov_tl(la, temp_4);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(la);

    return ret;
}


/*
 * LSL8
 *    Variables: @b, @a
 *    Functions: getCCFlag, logicalShiftLeft, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       la = logicalShiftLeft (@b, 8);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_LSL8(DisasCtxt *ctx, TCGv b, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_movi_tl(temp_5, 8);
    logicalShiftLeft(temp_4, b, temp_5);
    tcg_gen_mov_tl(la, temp_4);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(la);

    return ret;
}


/*
 * LSR
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, logicalShiftRight, getFFlag, setZFlag, setNFlag,
 *               setCFlag, getBit
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       lb = @b;
 *       lc = (@c & 31);
 *       la = logicalShiftRight (lb, lc);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *           if((lc == 0))
 *             {
 *               setCFlag (0);
 *             }
 *           else
 *             {
 *               setCFlag (getBit (lb, (lc - 1)));
 *             };
 *         };
 *     };
 * }
 */

int
arc_gen_LSR(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_5 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    getCCFlag(temp_5);
    tcg_gen_mov_tl(cc_flag, temp_5);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_andi_tl(lc, c, 31);
    logicalShiftRight(temp_6, lb, lc);
    tcg_gen_mov_tl(la, temp_6);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
        TCGLabel *else_2 = gen_new_label();
        TCGLabel *done_2 = gen_new_label();
        tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, lc, 0);
        tcg_gen_xori_tl(temp_4, temp_3, 1);
        tcg_gen_andi_tl(temp_4, temp_4, 1);
        tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
        tcg_gen_movi_tl(temp_7, 0);
        setCFlag(temp_7);
        tcg_gen_br(done_2);
        gen_set_label(else_2);
        tcg_gen_subi_tl(temp_10, lc, 1);
        getBit(temp_9, lb, temp_10);
        tcg_gen_mov_tl(temp_8, temp_9);
        setCFlag(temp_8);
        gen_set_label(done_2);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_5);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lb);
    tcg_temp_free(lc);
    tcg_temp_free(temp_6);
    tcg_temp_free(la);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_8);

    return ret;
}


/*
 * LSR16
 *    Variables: @b, @a
 *    Functions: getCCFlag, logicalShiftRight, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       la = logicalShiftRight (@b, 16);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_LSR16(DisasCtxt *ctx, TCGv b, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_movi_tl(temp_5, 16);
    logicalShiftRight(temp_4, b, temp_5);
    tcg_gen_mov_tl(la, temp_4);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(la);

    return ret;
}


/*
 * LSR8
 *    Variables: @b, @a
 *    Functions: getCCFlag, logicalShiftRight, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       la = logicalShiftRight (@b, 8);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_LSR8(DisasCtxt *ctx, TCGv b, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_movi_tl(temp_5, 8);
    logicalShiftRight(temp_4, b, temp_5);
    tcg_gen_mov_tl(la, temp_4);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(la);

    return ret;
}


/*
 * BIC
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       la = (@b & ~@c);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_BIC(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_not_tl(temp_4, c);
    tcg_gen_and_tl(la, b, temp_4);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);
    tcg_temp_free(la);

    return ret;
}


/*
 * BCLR
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       tmp = (1 << (@c & 31));
 *       la = (@b & ~tmp);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_BCLR(DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv tmp = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_andi_tl(temp_4, c, 31);
    tcg_gen_shlfi_tl(tmp, 1, temp_4);
    tcg_gen_not_tl(temp_5, tmp);
    tcg_gen_and_tl(la, b, temp_5);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);
    tcg_temp_free(tmp);
    tcg_temp_free(temp_5);
    tcg_temp_free(la);

    return ret;
}


/*
 * BMSK
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       tmp1 = ((@c & 31) + 1);
 *       if((tmp1 == 32))
 *         {
 *           tmp2 = 4294967295;
 *         }
 *       else
 *         {
 *           tmp2 = ((1 << tmp1) - 1);
 *         };
 *       la = (@b & tmp2);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_BMSK(DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
    int ret = DISAS_NEXT;
    TCGv temp_5 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv tmp1 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_5);
    tcg_gen_mov_tl(cc_flag, temp_5);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_andi_tl(temp_6, c, 31);
    tcg_gen_addi_tl(tmp1, temp_6, 1);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, tmp1, 32);
    tcg_gen_xori_tl(temp_4, temp_3, 1);
    tcg_gen_andi_tl(temp_4, temp_4, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
    tcg_gen_movi_tl(tmp2, 4294967295);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    tcg_gen_shlfi_tl(temp_7, 1, tmp1);
    tcg_gen_subi_tl(tmp2, temp_7, 1);
    gen_set_label(done_2);
    tcg_gen_and_tl(la, b, tmp2);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_5);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_6);
    tcg_temp_free(tmp1);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(tmp2);
    tcg_temp_free(temp_7);
    tcg_temp_free(la);

    return ret;
}


/*
 * BMSKN
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       tmp1 = ((@c & 31) + 1);
 *       if((tmp1 == 32))
 *         {
 *           tmp2 = 4294967295;
 *         }
 *       else
 *         {
 *           tmp2 = ((1 << tmp1) - 1);
 *         };
 *       la = (@b & ~tmp2);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_BMSKN(DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
    int ret = DISAS_NEXT;
    TCGv temp_5 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv tmp1 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_5);
    tcg_gen_mov_tl(cc_flag, temp_5);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_andi_tl(temp_6, c, 31);
    tcg_gen_addi_tl(tmp1, temp_6, 1);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, tmp1, 32);
    tcg_gen_xori_tl(temp_4, temp_3, 1);
    tcg_gen_andi_tl(temp_4, temp_4, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
    tcg_gen_movi_tl(tmp2, 4294967295);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    tcg_gen_shlfi_tl(temp_7, 1, tmp1);
    tcg_gen_subi_tl(tmp2, temp_7, 1);
    gen_set_label(done_2);
    tcg_gen_not_tl(temp_8, tmp2);
    tcg_gen_and_tl(la, b, temp_8);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_5);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_6);
    tcg_temp_free(tmp1);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(tmp2);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_8);
    tcg_temp_free(la);

    return ret;
}


/*
 * BSET
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       tmp = (1 << (@c & 31));
 *       la = (@b | tmp);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_BSET(DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv tmp = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_andi_tl(temp_4, c, 31);
    tcg_gen_shlfi_tl(tmp, 1, temp_4);
    tcg_gen_or_tl(la, b, tmp);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);
    tcg_temp_free(tmp);
    tcg_temp_free(la);

    return ret;
}


/*
 * BXOR
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       tmp = (1 << @c);
 *       la = (@b ^ tmp);
 *       @a = la;
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (la);
 *           setNFlag (la);
 *         };
 *     };
 * }
 */

int
arc_gen_BXOR(DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv tmp = tcg_temp_local_new();
    TCGv la = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_shlfi_tl(tmp, 1, c);
    tcg_gen_xor_tl(la, b, tmp);
    tcg_gen_mov_tl(a, la);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(la);
        setNFlag(la);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(tmp);
    tcg_temp_free(la);

    return ret;
}


/*
 * ROL
 *    Variables: @src, @dest, @n
 *    Functions: getCCFlag, rotateLeft, getFFlag, setZFlag, setNFlag, setCFlag,
 *               extractBits
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       lsrc = @src;
 *       @dest = rotateLeft (lsrc, 1);
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (@dest);
 *           setNFlag (@dest);
 *           setCFlag (extractBits (lsrc, 31, 31));
 *         };
 *     };
 * }
 */

int
arc_gen_ROL (DisasCtxt *ctx, TCGv src, TCGv n, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lsrc = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    int f_flag;
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lsrc, src);
    tcg_gen_andi_tl(temp_5, n, 31);
    rotateLeft(temp_4, lsrc, temp_5);
    tcg_gen_mov_tl(dest, temp_4);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(dest);
        setNFlag(dest);
        tcg_gen_movi_tl(temp_9, 31);
        tcg_gen_movi_tl(temp_8, 31);
        extractBits(temp_7, lsrc, temp_8, temp_9);
        tcg_gen_mov_tl(temp_6, temp_7);
        setCFlag(temp_6);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lsrc);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * ROL8
 *    Variables: @src, @dest
 *    Functions: getCCFlag, rotateLeft, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       lsrc = @src;
 *       @dest = rotateLeft (lsrc, 8);
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (@dest);
 *           setNFlag (@dest);
 *         };
 *     };
 * }
 */

int
arc_gen_ROL8(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lsrc = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lsrc, src);
    tcg_gen_movi_tl(temp_5, 8);
    rotateLeft(temp_4, lsrc, temp_5);
    tcg_gen_mov_tl(dest, temp_4);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(dest);
        setNFlag(dest);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lsrc);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);

    return ret;
}


/*
 * ROR
 *    Variables: @src, @n, @dest
 *    Functions: getCCFlag, rotateRight, getFFlag, setZFlag, setNFlag,
 *               setCFlag, extractBits
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       lsrc = @src;
 *       ln = (@n & 31);
 *       @dest = rotateRight (lsrc, ln);
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (@dest);
 *           setNFlag (@dest);
 *           setCFlag (extractBits (lsrc, (ln - 1), (ln - 1)));
 *         };
 *     };
 * }
 */

int
arc_gen_ROR(DisasCtxt *ctx, TCGv src, TCGv n, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lsrc = tcg_temp_local_new();
    TCGv ln = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    int f_flag;
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lsrc, src);
    tcg_gen_andi_tl(ln, n, 31);
    rotateRight(temp_4, lsrc, ln);
    tcg_gen_mov_tl(dest, temp_4);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(dest);
        setNFlag(dest);
        tcg_gen_subi_tl(temp_8, ln, 1);
        tcg_gen_subi_tl(temp_7, ln, 1);
        extractBits(temp_6, lsrc, temp_7, temp_8);
        tcg_gen_mov_tl(temp_5, temp_6);
        setCFlag(temp_5);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lsrc);
    tcg_temp_free(ln);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_5);

    return ret;
}


/*
 * ROR8
 *    Variables: @src, @dest
 *    Functions: getCCFlag, rotateRight, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       lsrc = @src;
 *       @dest = rotateRight (lsrc, 8);
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (@dest);
 *           setNFlag (@dest);
 *         };
 *     };
 * }
 */

int
arc_gen_ROR8(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lsrc = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lsrc, src);
    tcg_gen_movi_tl(temp_5, 8);
    rotateRight(temp_4, lsrc, temp_5);
    tcg_gen_mov_tl(dest, temp_4);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(dest);
        setNFlag(dest);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lsrc);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);

    return ret;
}


/*
 * RLC
 *    Variables: @src, @dest
 *    Functions: getCCFlag, getCFlag, getFFlag, setZFlag, setNFlag, setCFlag,
 *               extractBits
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       lsrc = @src;
 *       @dest = (lsrc << 1);
 *       @dest = (@dest | getCFlag ());
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (@dest);
 *           setNFlag (@dest);
 *           setCFlag (extractBits (lsrc, 31, 31));
 *         };
 *     };
 * }
 */

int
arc_gen_RLC(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lsrc = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    int f_flag;
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lsrc, src);
    tcg_gen_shli_tl(dest, lsrc, 1);
    getCFlag(temp_5);
    tcg_gen_mov_tl(temp_4, temp_5);
    tcg_gen_or_tl(dest, dest, temp_4);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(dest);
        setNFlag(dest);
        tcg_gen_movi_tl(temp_9, 31);
        tcg_gen_movi_tl(temp_8, 31);
        extractBits(temp_7, lsrc, temp_8, temp_9);
        tcg_gen_mov_tl(temp_6, temp_7);
        setCFlag(temp_6);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lsrc);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * RRC
 *    Variables: @src, @dest
 *    Functions: getCCFlag, getCFlag, getFFlag, setZFlag, setNFlag, setCFlag,
 *               extractBits
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       lsrc = @src;
 *       @dest = (lsrc >> 1);
 *       @dest = (@dest | (getCFlag () << 31));
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (@dest);
 *           setNFlag (@dest);
 *           setCFlag (extractBits (lsrc, 0, 0));
 *         };
 *     };
 * }
 */

int
arc_gen_RRC(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv lsrc = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    int f_flag;
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(lsrc, src);
    tcg_gen_shri_tl(dest, lsrc, 1);
    getCFlag(temp_6);
    tcg_gen_mov_tl(temp_5, temp_6);
    tcg_gen_shli_tl(temp_4, temp_5, 31);
    tcg_gen_or_tl(dest, dest, temp_4);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(dest);
        setNFlag(dest);
        tcg_gen_movi_tl(temp_10, 0);
        tcg_gen_movi_tl(temp_9, 0);
        extractBits(temp_8, lsrc, temp_9, temp_10);
        tcg_gen_mov_tl(temp_7, temp_8);
        setCFlag(temp_7);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(lsrc);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_7);

    return ret;
}


/*
 * SEXB
 *    Variables: @dest, @src
 *    Functions: getCCFlag, arithmeticShiftRight, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       @dest = arithmeticShiftRight ((@src << 24), 24);
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (@dest);
 *           setNFlag (@dest);
 *         };
 *     };
 * }
 */

int
arc_gen_SEXB(DisasCtxt *ctx, TCGv dest, TCGv src)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_movi_tl(temp_6, 24);
    tcg_gen_shli_tl(temp_5, src, 24);
    arithmeticShiftRight(temp_4, temp_5, temp_6);
    tcg_gen_mov_tl(dest, temp_4);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(dest);
        setNFlag(dest);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);

    return ret;
}


/*
 * SEXH
 *    Variables: @dest, @src
 *    Functions: getCCFlag, arithmeticShiftRight, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       @dest = arithmeticShiftRight ((@src << 16), 16);
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (@dest);
 *           setNFlag (@dest);
 *         };
 *     };
 * }
 */

int
arc_gen_SEXH(DisasCtxt *ctx, TCGv dest, TCGv src)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_movi_tl(temp_6, 16);
    tcg_gen_shli_tl(temp_5, src, 16);
    arithmeticShiftRight(temp_4, temp_5, temp_6);
    tcg_gen_mov_tl(dest, temp_4);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(dest);
        setNFlag(dest);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);

    return ret;
}


/*
 * EXTB
 *    Variables: @dest, @src
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       @dest = (@src & 255);
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (@dest);
 *           setNFlag (@dest);
 *         };
 *     };
 * }
 */

int
arc_gen_EXTB(DisasCtxt *ctx, TCGv dest, TCGv src)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_andi_tl(dest, src, 255);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(dest);
        setNFlag(dest);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);

    return ret;
}


/*
 * EXTH
 *    Variables: @dest, @src
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       @dest = (@src & 65535);
 *       f_flag = getFFlag ();
 *       if((f_flag == true))
 *         {
 *           setZFlag (@dest);
 *           setNFlag (@dest);
 *         };
 *     };
 * }
 */

int
arc_gen_EXTH(DisasCtxt *ctx, TCGv dest, TCGv src)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    int f_flag;
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_andi_tl(dest, src, 65535);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(dest);
        setNFlag(dest);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);

    return ret;
}


/*
 * BTST
 *    Variables: @c, @b
 *    Functions: getCCFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       tmp = (1 << (@c & 31));
 *       alu = (@b & tmp);
 *       setZFlag (alu);
 *       setNFlag (alu);
 *     };
 * }
 */

int
arc_gen_BTST(DisasCtxt *ctx, TCGv c, TCGv b)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv tmp = tcg_temp_local_new();
    TCGv alu = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_andi_tl(temp_4, c, 31);
    tcg_gen_shlfi_tl(tmp, 1, temp_4);
    tcg_gen_and_tl(alu, b, tmp);
    setZFlag(alu);
    setNFlag(alu);
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);
    tcg_temp_free(tmp);
    tcg_temp_free(alu);

    return ret;
}


/*
 * TST
 *    Variables: @b, @c
 *    Functions: getCCFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       alu = (@b & @c);
 *       setZFlag (alu);
 *       setNFlag (alu);
 *     };
 * }
 */

int
arc_gen_TST(DisasCtxt *ctx, TCGv b, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv alu = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_and_tl(alu, b, c);
    setZFlag(alu);
    setNFlag(alu);
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(alu);

    return ret;
}


/*
 * XBFU
 *    Variables: @src2, @src1, @dest
 *    Functions: getCCFlag, extractBits, getFFlag, setZFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       N = extractBits (@src2, 4, 0);
 *       M = (extractBits (@src2, 9, 5) + 1);
 *       tmp1 = (@src1 >> N);
 *       tmp2 = ((1 << M) - 1);
 *       @dest = (tmp1 & tmp2);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@dest);
 *         };
 *     };
 * }
 */

int
arc_gen_XBFU(DisasCtxt *ctx, TCGv src2, TCGv src1, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv N = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv M = tcg_temp_local_new();
    TCGv tmp1 = tcg_temp_local_new();
    TCGv temp_11 = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_movi_tl(temp_6, 0);
    tcg_gen_movi_tl(temp_5, 4);
    extractBits(temp_4, src2, temp_5, temp_6);
    tcg_gen_mov_tl(N, temp_4);
    tcg_gen_movi_tl(temp_10, 5);
    tcg_gen_movi_tl(temp_9, 9);
    extractBits(temp_8, src2, temp_9, temp_10);
    tcg_gen_mov_tl(temp_7, temp_8);
    tcg_gen_addi_tl(M, temp_7, 1);
    tcg_gen_shr_tl(tmp1, src1, N);
    tcg_gen_shlfi_tl(temp_11, 1, M);
    tcg_gen_subi_tl(tmp2, temp_11, 1);
    tcg_gen_and_tl(dest, tmp1, tmp2);
    if ((getFFlag () == true)) {
        setZFlag(dest);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(N);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_7);
    tcg_temp_free(M);
    tcg_temp_free(tmp1);
    tcg_temp_free(temp_11);
    tcg_temp_free(tmp2);

    return ret;
}


/*
 * AEX
 *    Variables: @src2, @b
 *    Functions: getCCFlag, readAuxReg, writeAuxReg
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       tmp = readAuxReg (@src2);
 *       writeAuxReg (@src2, @b);
 *       @b = tmp;
 *     };
 * }
 */

int
arc_gen_AEX(DisasCtxt *ctx, TCGv src2, TCGv b)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv tmp = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    readAuxReg(temp_4, src2);
    tcg_gen_mov_tl(tmp, temp_4);
    writeAuxReg(src2, b);
    tcg_gen_mov_tl(b, tmp);
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);
    tcg_temp_free(tmp);

    return ret;
}


/*
 * LR
 *    Variables: @dest, @src
 *    Functions: readAuxReg
 * --- code ---
 * {
 *   @dest = readAuxReg (@src);
 * }
 */

int
arc_gen_LR(DisasCtxt *ctx, TCGv dest, TCGv src)
{
    int ret = DISAS_NORETURN;

    if (tb_cflags(ctx->base.tb) & CF_USE_ICOUNT) {
        gen_io_start();
    }

    TCGv temp_1 = tcg_temp_local_new();
    readAuxReg(temp_1, src);
    tcg_gen_mov_tl(dest, temp_1);
    tcg_temp_free(temp_1);

    return ret;
}


/*
 * CLRI
 * --- code ---
 * if (status32.u)
 *   raise privilege violation
 *
 * if (ctx->insn.operands[0].type == reg)
 * {
 *   c[4] = status32 >> 31
 *   c[3:0] = ((status32 & STATUS32_E_MSK) >> 1) & 0xF
 *   c[5] = 1
 * }
 *
 * status32 &= STATUS32_IE
 */
int
arc_gen_CLRI(DisasCtxt *ctx, TCGv c)
{
    TCGv cond = tcg_temp_new();
    TCGLabel *cont = gen_new_label();

    assert(ctx->insn.n_ops == 0 || ctx->insn.n_ops == 1);

    /* If in user mode, raise a privilege violation. */
    tcg_gen_andi_tl(cond, cpu_pstate, STATUS32_U);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cond, 0, cont);
    tcg_temp_free(cond);
    arc_gen_excp(ctx, EXCP_PRIVILEGEV, 0, 0);

    gen_set_label(cont);
    if (ctx->insn.n_ops == 1 &&
        ctx->insn.operands[0].type == ARC_OPERAND_IR) {
        TCGv t = tcg_temp_new();

        /* c[4] = status32 >> 31 */
        tcg_gen_shri_tl(t, cpu_pstate, __builtin_ctz(STATUS32_IE));
        tcg_gen_andi_tl(t, t, 1);
        tcg_gen_shli_tl(c, t, 4);
        /* c[3:0] = ((status32 & STATUS32_E_MSK) >> 1) & 0xF */
        tcg_gen_andi_tl(t, cpu_pstate, STATUS32_E_MSK);
        tcg_gen_shri_tl(t, t, __builtin_ctz(STATUS32_E_MSK));
        tcg_gen_andi_tl(t, t, 0b1111);
        tcg_gen_or_tl(c, c, t);
        /* c[5] = 1 */
        tcg_gen_ori_tl(c, c, 0b100000);

        tcg_temp_free(t);
    }

    /* status32.ie = 0 */
    tcg_gen_andi_tl(cpu_pstate, cpu_pstate, ~STATUS32_IE);

    return DISAS_UPDATE;
}


/*
 * SETI
 * --- code ---
 * if (status32.u)
 *   raise privilege violation
 *
 * c_4   = (c & 0b010000) >> 4
 * c_5   = (c & 0b100000) >> 5
 * e_new = (c & 0b001111) << 1
 *
 * ie = ((c_5 == 1) ? c_4 : 1) << 31
 * status32 &= ~STATUS32_IE
 * status32 |= ie
 *
 * change_e = c_4 | c_5
 * e_old = status32 & STATUS32_E_MSK
 * e_new = ((change_e == 1) ? e_new : e_old)
 * status32 &= ~STATUS32_E_MSK
 * status32 |= e_new
 */
int
arc_gen_SETI(DisasCtxt *ctx, TCGv c)
{
    TCGv cond = tcg_temp_new();
    TCGLabel *cont = gen_new_label();
    TCGv one;
    TCGv c_4;
    TCGv c_5;
    TCGv ie;
    TCGv e_old;
    TCGv e_new;
    TCGv change_e;

    /* If in user mode, raise a privilege violation. */
    tcg_gen_andi_tl(cond, cpu_pstate, STATUS32_U);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cond, 0, cont);
    tcg_temp_free(cond);
    arc_gen_excp(ctx, EXCP_PRIVILEGEV, 0, 0);

    gen_set_label(cont);
    one      = tcg_const_tl(1);
    c_4      = tcg_temp_new();
    c_5      = tcg_temp_new();
    ie       = tcg_temp_new();
    e_old    = tcg_temp_new();
    e_new    = tcg_temp_new();
    change_e = tcg_temp_new();

    /* extract bits */
    tcg_gen_andi_tl(c_4  , c, 0b010000);
    tcg_gen_andi_tl(c_5  , c, 0b100000);
    tcg_gen_andi_tl(e_new, c, 0b001111);
    /* align bits */
    tcg_gen_shri_tl(c_4, c_4, 4);
    tcg_gen_shri_tl(c_5, c_5, 5);
    tcg_gen_shli_tl(e_new, e_new, 1);
    /* ie = ((c_5 == 1) ? c_4 : 1) */
    tcg_gen_movcond_tl(TCG_COND_EQ, ie, c_5, one, c_4, one);
    tcg_gen_shli_tl(ie, ie, __builtin_ctz(STATUS32_IE));
    /* status32.ie = ie */
    tcg_gen_andi_tl(cpu_pstate, cpu_pstate, ~STATUS32_IE);
    tcg_gen_or_tl(cpu_pstate, cpu_pstate, ie);
    /* status32.e = e   (only if not both c_4 and c_5 are zero) */
    tcg_gen_or_tl(change_e, c_4, c_5);
    tcg_gen_andi_tl(e_old, cpu_pstate, STATUS32_E_MSK);
    /* e_new = ((change_e == 1) ? e_new : e_old) */
    tcg_gen_movcond_tl(TCG_COND_EQ, e_new, change_e, one, e_new, e_old);
    tcg_gen_andi_tl(cpu_pstate, cpu_pstate, ~STATUS32_E_MSK);
    tcg_gen_or_tl(cpu_pstate, cpu_pstate, e_new);

    tcg_temp_free(change_e);
    tcg_temp_free(e_new);
    tcg_temp_free(e_old);
    tcg_temp_free(ie);
    tcg_temp_free(c_5);
    tcg_temp_free(c_4);
    tcg_temp_free(one);

    return DISAS_UPDATE;
}


/*
 * NOP
 *    Variables:
 *    Functions: doNothing
 * --- code ---
 * {
 *   doNothing ();
 * }
 */

int
arc_gen_NOP(DisasCtxt *ctx)
{
    int ret = DISAS_NEXT;

    return ret;
}


/*
 * PREALLOC
 *    Variables:
 *    Functions: doNothing
 * --- code ---
 * {
 *   doNothing ();
 * }
 */

int
arc_gen_PREALLOC(DisasCtxt *ctx)
{
    int ret = DISAS_NEXT;

    return ret;
}


/*
 * PREFETCH
 *    Variables: @src1, @src2
 *    Functions: getAAFlag, doNothing
 * --- code ---
 * {
 *   AA = getAAFlag ();
 *   if(((AA == 1) || (AA == 2)))
 *     {
 *       @src1 = (@src1 + @src2);
 *     }
 *   else
 *     {
 *       doNothing ();
 *     };
 * }
 */

int
arc_gen_PREFETCH(DisasCtxt *ctx, TCGv src1, TCGv src2)
{
    int ret = DISAS_NEXT;
    int AA;
    AA = getAAFlag ();
    if (((AA == 1) || (AA == 2))) {
        tcg_gen_add_tl(src1, src1, src2);
    } else {
        doNothing();
    }

    return ret;
}


/*
 * MPY
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, HELPER, setZFlag, setNFlag, setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       _b = @b;
 *       _c = @c;
 *       @a = ((_b * _c) & 4294967295);
 *       if((getFFlag () == true))
 *         {
 *           high_part = HELPER (mpym, _b, _c);
 *           tmp1 = (high_part & 2147483648);
 *           tmp2 = @a >> 31;
 *           setZFlag (@a);
 *           setNFlag (high_part);
 *           setVFlag ((tmp1 != tmp2));
 *         };
 *     };
 * }
 */

int
arc_gen_MPY(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv _b = tcg_temp_local_new();
    TCGv _c = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv high_part = tcg_temp_local_new();
    TCGv tmp1 = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(_b, b);
    tcg_gen_mov_tl(_c, c);
    tcg_gen_mul_tl(temp_4, _b, _c);
    tcg_gen_andi_tl(a, temp_4, 4294967295);
    if ((getFFlag () == true)) {
        ARC_HELPER(mpym, high_part, _b, _c);
        tcg_gen_sari_tl(tmp2, a, 31);
        setZFlag(a);
        setNFlag(high_part);
        tcg_gen_setcond_tl(TCG_COND_NE, temp_5, high_part, tmp2);
        setVFlag(temp_5);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(_b);
    tcg_temp_free(_c);
    tcg_temp_free(temp_4);
    tcg_temp_free(high_part);
    tcg_temp_free(tmp1);
    tcg_temp_free(tmp2);
    tcg_temp_free(temp_5);

    return ret;
}


/*
 * MPYMU
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, HELPER, getFFlag, setZFlag, setNFlag, setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       @a = HELPER (mpymu, @b, @c);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (0);
 *           setVFlag (0);
 *         };
 *     };
 * }
 */

int
arc_gen_MPYMU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    ARC_HELPER(mpymu, a, b, c);
    if ((getFFlag () == true)) {
        setZFlag(a);
        tcg_gen_movi_tl(temp_4, 0);
        setNFlag(temp_4);
        tcg_gen_movi_tl(temp_5, 0);
        setVFlag(temp_5);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_5);

    return ret;
}


/*
 * MPYM
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, HELPER, getFFlag, setZFlag, setNFlag, setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       @a = HELPER (mpym, @b, @c);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setVFlag (0);
 *         };
 *     };
 * }
 */

int
arc_gen_MPYM(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    ARC_HELPER(mpym, a, b, c);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        tcg_gen_movi_tl(temp_4, 0);
        setVFlag(temp_4);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);

    return ret;
}


/*
 * MPYU
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, HELPER, setZFlag, setNFlag, setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       _b = @b;
 *       _c = @c;
 *       @a = ((_b * _c) & 4294967295);
 *       if((getFFlag () == true))
 *         {
 *           high_part = HELPER (mpymu, _b, _c);
 *           setZFlag (@a);
 *           setNFlag (0);
 *           setVFlag ((high_part != 0));
 *         };
 *     };
 * }
 */

int
arc_gen_MPYU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv _b = tcg_temp_local_new();
    TCGv _c = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv high_part = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(_b, b);
    tcg_gen_mov_tl(_c, c);
    tcg_gen_mul_tl(temp_4, _b, _c);
    tcg_gen_andi_tl(a, temp_4, 4294967295);
    if ((getFFlag () == true)) {
        ARC_HELPER(mpymu, high_part, _b, _c);
        setZFlag(a);
        tcg_gen_movi_tl(temp_5, 0);
        setNFlag(temp_5);
        tcg_gen_setcondi_tl(TCG_COND_NE, temp_6, high_part, 0);
        setVFlag(temp_6);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(_b);
    tcg_temp_free(_c);
    tcg_temp_free(temp_4);
    tcg_temp_free(high_part);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * MPYUW
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       @a = ((@b & 65535) * (@c & 65535));
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (0);
 *           setVFlag (0);
 *         };
 *     };
 * }
 */

int
arc_gen_MPYUW(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_andi_tl(temp_5, c, 65535);
    tcg_gen_andi_tl(temp_4, b, 65535);
    tcg_gen_mul_tl(a, temp_4, temp_5);
    if ((getFFlag () == true)) {
        setZFlag(a);
        tcg_gen_movi_tl(temp_6, 0);
        setNFlag(temp_6);
        tcg_gen_movi_tl(temp_7, 0);
        setVFlag(temp_7);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_7);

    return ret;
}


/*
 * MPYW
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, arithmeticShiftRight, getFFlag, setZFlag, setNFlag,
 *               setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       @a = (arithmeticShiftRight ((@b << 16), 16)
 *            * arithmeticShiftRight ((@c << 16), 16));
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setVFlag (0);
 *         };
 *     };
 * }
 */

int
arc_gen_MPYW(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_11 = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_12 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_movi_tl(temp_11, 16);
    tcg_gen_shli_tl(temp_10, c, 16);
    tcg_gen_movi_tl(temp_7, 16);
    tcg_gen_shli_tl(temp_6, b, 16);
    arithmeticShiftRight(temp_5, temp_6, temp_7);
    tcg_gen_mov_tl(temp_4, temp_5);
    arithmeticShiftRight(temp_9, temp_10, temp_11);
    tcg_gen_mov_tl(temp_8, temp_9);
    tcg_gen_mul_tl(a, temp_4, temp_8);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        tcg_gen_movi_tl(temp_12, 0);
        setVFlag(temp_12);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_11);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_12);

    return ret;
}


/*
 * DIV
 *    Variables: @src2, @src1, @dest
 *    Functions: getCCFlag, divSigned, getFFlag, setZFlag, setNFlag, setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       if(((@src2 != 0) && ((@src1 != 2147483648) || (@src2 != 4294967295))))
 *         {
 *           @dest = divSigned (@src1, @src2);
 *           if((getFFlag () == true))
 *             {
 *               setZFlag (@dest);
 *               setNFlag (@dest);
 *               setVFlag (0);
 *             };
 *         }
 *       else
 *         {
 *         };
 *     };
 * }
 */

int
arc_gen_DIV(DisasCtxt *ctx, TCGv src2, TCGv src1, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv temp_9 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_11 = tcg_temp_local_new();
    getCCFlag(temp_9);
    tcg_gen_mov_tl(cc_flag, temp_9);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcondi_tl(TCG_COND_NE, temp_3, src2, 0);
    tcg_gen_setcondi_tl(TCG_COND_NE, temp_4, src1, 2147483648);
    tcg_gen_setcondi_tl(TCG_COND_NE, temp_5, src2, 4294967295);
    tcg_gen_or_tl(temp_6, temp_4, temp_5);
    tcg_gen_and_tl(temp_7, temp_3, temp_6);
    tcg_gen_xori_tl(temp_8, temp_7, 1);
    tcg_gen_andi_tl(temp_8, temp_8, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_8, arc_true, else_2);
    divSigned(temp_10, src1, src2);
    tcg_gen_mov_tl(dest, temp_10);
    if ((getFFlag () == true)) {
        setZFlag(dest);
        setNFlag(dest);
        tcg_gen_movi_tl(temp_11, 0);
        setVFlag(temp_11);
    }
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    gen_set_label(done_2);
    gen_set_label(done_1);
    tcg_temp_free(temp_9);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_11);

    return ret;
}


/*
 * DIVU
 *    Variables: @src2, @dest, @src1
 *    Functions: getCCFlag, divUnsigned, getFFlag, setZFlag, setNFlag,
 *               setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       if((@src2 != 0))
 *         {
 *           @dest = divUnsigned (@src1, @src2);
 *           if((getFFlag () == true))
 *             {
 *               setZFlag (@dest);
 *               setNFlag (0);
 *               setVFlag (0);
 *             };
 *         }
 *       else
 *         {
 *         };
 *     };
 * }
 */

int
arc_gen_DIVU(DisasCtxt *ctx, TCGv src2, TCGv dest, TCGv src1)
{
    int ret = DISAS_NEXT;
    TCGv temp_5 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    getCCFlag(temp_5);
    tcg_gen_mov_tl(cc_flag, temp_5);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcondi_tl(TCG_COND_NE, temp_3, src2, 0);
    tcg_gen_xori_tl(temp_4, temp_3, 1);
    tcg_gen_andi_tl(temp_4, temp_4, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
    divUnsigned(temp_6, src1, src2);
    tcg_gen_mov_tl(dest, temp_6);
    if ((getFFlag () == true)) {
        setZFlag(dest);
        tcg_gen_movi_tl(temp_7, 0);
        setNFlag(temp_7);
        tcg_gen_movi_tl(temp_8, 0);
        setVFlag(temp_8);
    }
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    gen_set_label(done_2);
    gen_set_label(done_1);
    tcg_temp_free(temp_5);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_8);

    return ret;
}


/*
 * REM
 *    Variables: @src2, @src1, @dest
 *    Functions: getCCFlag, divRemainingSigned, getFFlag, setZFlag, setNFlag,
 *               setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       if(((@src2 != 0) && ((@src1 != 2147483648) || (@src2 != 4294967295))))
 *         {
 *           @dest = divRemainingSigned (@src1, @src2);
 *           if((getFFlag () == true))
 *             {
 *               setZFlag (@dest);
 *               setNFlag (@dest);
 *               setVFlag (0);
 *             };
 *         }
 *       else
 *         {
 *         };
 *     };
 * }
 */

int
arc_gen_REM(DisasCtxt *ctx, TCGv src2, TCGv src1, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv temp_9 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_11 = tcg_temp_local_new();
    getCCFlag(temp_9);
    tcg_gen_mov_tl(cc_flag, temp_9);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcondi_tl(TCG_COND_NE, temp_3, src2, 0);
    tcg_gen_setcondi_tl(TCG_COND_NE, temp_4, src1, 2147483648);
    tcg_gen_setcondi_tl(TCG_COND_NE, temp_5, src2, 4294967295);
    tcg_gen_or_tl(temp_6, temp_4, temp_5);
    tcg_gen_and_tl(temp_7, temp_3, temp_6);
    tcg_gen_xori_tl(temp_8, temp_7, 1);
    tcg_gen_andi_tl(temp_8, temp_8, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_8, arc_true, else_2);
    divRemainingSigned(temp_10, src1, src2);
    tcg_gen_mov_tl(dest, temp_10);
    if ((getFFlag () == true)) {
        setZFlag(dest);
        setNFlag(dest);
        tcg_gen_movi_tl(temp_11, 0);
        setVFlag(temp_11);
    }
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    gen_set_label(done_2);
    gen_set_label(done_1);
    tcg_temp_free(temp_9);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_11);

    return ret;
}


/*
 * REMU
 *    Variables: @src2, @dest, @src1
 *    Functions: getCCFlag, divRemainingUnsigned, getFFlag, setZFlag, setNFlag,
 *               setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       if((@src2 != 0))
 *         {
 *           @dest = divRemainingUnsigned (@src1, @src2);
 *           if((getFFlag () == true))
 *             {
 *               setZFlag (@dest);
 *               setNFlag (0);
 *               setVFlag (0);
 *             };
 *         }
 *       else
 *         {
 *         };
 *     };
 * }
 */

int
arc_gen_REMU(DisasCtxt *ctx, TCGv src2, TCGv dest, TCGv src1)
{
    int ret = DISAS_NEXT;
    TCGv temp_5 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    getCCFlag(temp_5);
    tcg_gen_mov_tl(cc_flag, temp_5);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcondi_tl(TCG_COND_NE, temp_3, src2, 0);
    tcg_gen_xori_tl(temp_4, temp_3, 1);
    tcg_gen_andi_tl(temp_4, temp_4, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
    divRemainingUnsigned(temp_6, src1, src2);
    tcg_gen_mov_tl(dest, temp_6);
    if ((getFFlag () == true)) {
        setZFlag(dest);
        tcg_gen_movi_tl(temp_7, 0);
        setNFlag(temp_7);
        tcg_gen_movi_tl(temp_8, 0);
        setVFlag(temp_8);
    }
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    gen_set_label(done_2);
    gen_set_label(done_1);
    tcg_temp_free(temp_5);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_8);

    return ret;
}


/*
 * MAC
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, arc_gen_get_register, MAC, getFFlag, setNFlag, OverflowADD,
 *               setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       old_acchi = arc_gen_get_register (R_ACCHI);
 *       high_mul = MAC (@b, @c);
 *       @a = arc_gen_get_register (R_ACCLO);
 *       if((getFFlag () == true))
 *         {
 *           new_acchi = arc_gen_get_register (R_ACCHI);
 *           setNFlag (new_acchi);
 *           if((OverflowADD (new_acchi, old_acchi, high_mul) == true))
 *             {
 *               setVFlag (1);
 *             };
 *         };
 *     };
 * }
 */

int
arc_gen_MAC(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_5 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv old_acchi = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv high_mul = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    TCGv new_acchi = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_11 = tcg_temp_local_new();
    getCCFlag(temp_5);
    tcg_gen_mov_tl(cc_flag, temp_5);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(temp_6, cpu_acchi);
    tcg_gen_mov_tl(old_acchi, temp_6);
    MAC(temp_7, b, c);
    tcg_gen_mov_tl(high_mul, temp_7);
    tcg_gen_mov_tl(temp_8, cpu_acclo);
    tcg_gen_mov_tl(a, temp_8);
    if ((getFFlag () == true)) {
        tcg_gen_mov_tl(temp_9, cpu_acchi);
        tcg_gen_mov_tl(new_acchi, temp_9);
        setNFlag(new_acchi);
        TCGLabel *done_2 = gen_new_label();
        OverflowADD(temp_10, new_acchi, old_acchi, high_mul);
        tcg_gen_setcond_tl(TCG_COND_EQ, temp_3, temp_10, arc_true);
        tcg_gen_xori_tl(temp_4, temp_3, 1);
        tcg_gen_andi_tl(temp_4, temp_4, 1);
        tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, done_2);
        tcg_gen_movi_tl(temp_11, 1);
        setVFlag(temp_11);
        gen_set_label(done_2);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_5);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_6);
    tcg_temp_free(old_acchi);
    tcg_temp_free(temp_7);
    tcg_temp_free(high_mul);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_9);
    tcg_temp_free(new_acchi);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_11);

    return ret;
}


/*
 * MACU
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, arc_gen_get_register, MACU, getFFlag, CarryADD, setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       old_acchi = arc_gen_get_register (R_ACCHI);
 *       high_mul = MACU (@b, @c);
 *       @a = arc_gen_get_register (R_ACCLO);
 *       if((getFFlag () == true))
 *         {
 *           new_acchi = arc_gen_get_register (R_ACCHI);
 *           if((CarryADD (new_acchi, old_acchi, high_mul) == true))
 *             {
 *               setVFlag (1);
 *             };
 *         };
 *     };
 * }
 */

int
arc_gen_MACU(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_5 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv old_acchi = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv high_mul = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    TCGv new_acchi = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_11 = tcg_temp_local_new();
    getCCFlag(temp_5);
    tcg_gen_mov_tl(cc_flag, temp_5);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(temp_6, cpu_acchi);
    tcg_gen_mov_tl(old_acchi, temp_6);
    MACU(temp_7, b, c);
    tcg_gen_mov_tl(high_mul, temp_7);
    tcg_gen_mov_tl(temp_8, cpu_acclo);
    tcg_gen_mov_tl(a, temp_8);
    if ((getFFlag () == true)) {
        tcg_gen_mov_tl(temp_9, cpu_acchi);
        tcg_gen_mov_tl(new_acchi, temp_9);
        TCGLabel *done_2 = gen_new_label();
        CarryADD(temp_10, new_acchi, old_acchi, high_mul);
        tcg_gen_setcond_tl(TCG_COND_EQ, temp_3, temp_10, arc_true);
        tcg_gen_xori_tl(temp_4, temp_3, 1);
        tcg_gen_andi_tl(temp_4, temp_4, 1);
        tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, done_2);
        tcg_gen_movi_tl(temp_11, 1);
        setVFlag(temp_11);
        gen_set_label(done_2);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_5);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_6);
    tcg_temp_free(old_acchi);
    tcg_temp_free(temp_7);
    tcg_temp_free(high_mul);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_9);
    tcg_temp_free(new_acchi);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_11);

    return ret;
}


/*
 * MACD
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, arc_gen_get_register, MAC, nextReg, getFFlag, setNFlag,
 *               OverflowADD, setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       old_acchi = arc_gen_get_register (R_ACCHI);
 *       high_mul = MAC (@b, @c);
 *       @a = arc_gen_get_register (R_ACCLO);
 *       pair = nextReg (a);
 *       pair = arc_gen_get_register (R_ACCHI);
 *       if((getFFlag () == true))
 *         {
 *           new_acchi = arc_gen_get_register (R_ACCHI);
 *           setNFlag (new_acchi);
 *           if((OverflowADD (new_acchi, old_acchi, high_mul) == true))
 *             {
 *               setVFlag (1);
 *             };
 *         };
 *     };
 * }
 */

int
arc_gen_MACD(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_5 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv old_acchi = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv high_mul = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv pair = NULL;
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv new_acchi = tcg_temp_local_new();
    TCGv temp_11 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_12 = tcg_temp_local_new();
    getCCFlag(temp_5);
    tcg_gen_mov_tl(cc_flag, temp_5);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(temp_6, cpu_acchi);
    tcg_gen_mov_tl(old_acchi, temp_6);
    MAC(temp_7, b, c);
    tcg_gen_mov_tl(high_mul, temp_7);
    tcg_gen_mov_tl(temp_8, cpu_acclo);
    tcg_gen_mov_tl(a, temp_8);
    pair = nextReg (a);
    tcg_gen_mov_tl(temp_9, cpu_acchi);
    tcg_gen_mov_tl(pair, temp_9);
    if ((getFFlag () == true)) {
        tcg_gen_mov_tl(temp_10, cpu_acchi);
        tcg_gen_mov_tl(new_acchi, temp_10);
        setNFlag(new_acchi);
        TCGLabel *done_2 = gen_new_label();
        OverflowADD(temp_11, new_acchi, old_acchi, high_mul);
        tcg_gen_setcond_tl(TCG_COND_EQ, temp_3, temp_11, arc_true);
        tcg_gen_xori_tl(temp_4, temp_3, 1);
        tcg_gen_andi_tl(temp_4, temp_4, 1);
        tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, done_2);
        tcg_gen_movi_tl(temp_12, 1);
        setVFlag(temp_12);
        gen_set_label(done_2);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_5);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_6);
    tcg_temp_free(old_acchi);
    tcg_temp_free(temp_7);
    tcg_temp_free(high_mul);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_10);
    tcg_temp_free(new_acchi);
    tcg_temp_free(temp_11);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_12);

    return ret;
}


/*
 * MACDU
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, arc_gen_get_register, MACU, nextReg, getFFlag, CarryADD,
 *               setVFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       old_acchi = arc_gen_get_register (R_ACCHI);
 *       high_mul = MACU (@b, @c);
 *       @a = arc_gen_get_register (R_ACCLO);
 *       pair = nextReg (a);
 *       pair = arc_gen_get_register (R_ACCHI);
 *       if((getFFlag () == true))
 *         {
 *           new_acchi = arc_gen_get_register (R_ACCHI);
 *           if((CarryADD (new_acchi, old_acchi, high_mul) == true))
 *             {
 *               setVFlag (1);
 *             };
 *         };
 *     };
 * }
 */

int
arc_gen_MACDU(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_5 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv old_acchi = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv high_mul = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv pair = NULL;
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv new_acchi = tcg_temp_local_new();
    TCGv temp_11 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_12 = tcg_temp_local_new();
    getCCFlag(temp_5);
    tcg_gen_mov_tl(cc_flag, temp_5);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(temp_6, cpu_acchi);
    tcg_gen_mov_tl(old_acchi, temp_6);
    MACU(temp_7, b, c);
    tcg_gen_mov_tl(high_mul, temp_7);
    tcg_gen_mov_tl(temp_8, cpu_acclo);
    tcg_gen_mov_tl(a, temp_8);
    pair = nextReg (a);
    tcg_gen_mov_tl(temp_9, cpu_acchi);
    tcg_gen_mov_tl(pair, temp_9);
    if ((getFFlag () == true)) {
        tcg_gen_mov_tl(temp_10, cpu_acchi);
        tcg_gen_mov_tl(new_acchi, temp_10);
        TCGLabel *done_2 = gen_new_label();
        CarryADD(temp_11, new_acchi, old_acchi, high_mul);
        tcg_gen_setcond_tl(TCG_COND_EQ, temp_3, temp_11, arc_true);
        tcg_gen_xori_tl(temp_4, temp_3, 1);
        tcg_gen_andi_tl(temp_4, temp_4, 1);
        tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, done_2);
        tcg_gen_movi_tl(temp_12, 1);
        setVFlag(temp_12);
        gen_set_label(done_2);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_5);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_6);
    tcg_temp_free(old_acchi);
    tcg_temp_free(temp_7);
    tcg_temp_free(high_mul);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_10);
    tcg_temp_free(new_acchi);
    tcg_temp_free(temp_11);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_12);

    return ret;
}


/*
 * ABS
 *    Variables: @src, @dest
 *    Functions: Carry, getFFlag, setZFlag, setNFlag, setCFlag, Zero, setVFlag,
 *               getNFlag
 * --- code ---
 * {
 *   lsrc = @src;
 *   alu = (0 - lsrc);
 *   if((Carry (lsrc) == 1))
 *     {
 *       @dest = alu;
 *     }
 *   else
 *     {
 *       @dest = lsrc;
 *     };
 *   if((getFFlag () == true))
 *     {
 *       setZFlag (@dest);
 *       setNFlag (@dest);
 *       setCFlag (Zero ());
 *       setVFlag (getNFlag ());
 *     };
 * }
 */

int
arc_gen_ABS(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv lsrc = tcg_temp_local_new();
    TCGv alu = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    tcg_gen_mov_tl(lsrc, src);
    tcg_gen_subfi_tl(alu, 0, lsrc);
    TCGLabel *else_1 = gen_new_label();
    TCGLabel *done_1 = gen_new_label();
    Carry(temp_3, lsrc);
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_1, temp_3, 1);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_1);
    tcg_gen_mov_tl(dest, alu);
    tcg_gen_br(done_1);
    gen_set_label(else_1);
    tcg_gen_mov_tl(dest, lsrc);
    gen_set_label(done_1);
    if ((getFFlag () == true)) {
        setZFlag(dest);
        setNFlag(dest);
        tcg_gen_mov_tl(temp_4, Zero());
        setCFlag(temp_4);
        tcg_gen_mov_tl(temp_5, getNFlag());
        setVFlag(temp_5);
    }
    tcg_temp_free(lsrc);
    tcg_temp_free(alu);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_5);

    return ret;
}


/*
 * SWAP
 *    Variables: @src, @dest
 *    Functions: getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   tmp1 = (@src << 16);
 *   tmp2 = ((@src >> 16) & 65535);
 *   @dest = (tmp1 | tmp2);
 *   f_flag = getFFlag ();
 *   if((f_flag == true))
 *     {
 *       setZFlag (@dest);
 *       setNFlag (@dest);
 *     };
 * }
 */

int
arc_gen_SWAP(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv tmp1 = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    int f_flag;
    tcg_gen_shli_tl(tmp1, src, 16);
    tcg_gen_shri_tl(temp_1, src, 16);
    tcg_gen_andi_tl(tmp2, temp_1, 65535);
    tcg_gen_or_tl(dest, tmp1, tmp2);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(dest);
        setNFlag(dest);
    }
    tcg_temp_free(tmp1);
    tcg_temp_free(temp_1);
    tcg_temp_free(tmp2);

    return ret;
}


/*
 * SWAPE
 *    Variables: @src, @dest
 *    Functions: getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   tmp1 = ((@src << 24) & 4278190080);
 *   tmp2 = ((@src << 8) & 16711680);
 *   tmp3 = ((@src >> 8) & 65280);
 *   tmp4 = ((@src >> 24) & 255);
 *   @dest = (((tmp1 | tmp2) | tmp3) | tmp4);
 *   f_flag = getFFlag ();
 *   if((f_flag == true))
 *     {
 *       setZFlag (@dest);
 *       setNFlag (@dest);
 *     };
 * }
 */

int
arc_gen_SWAPE(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv temp_1 = tcg_temp_local_new();
    TCGv tmp1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv tmp3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv tmp4 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    int f_flag;
    tcg_gen_shli_tl(temp_1, src, 24);
    tcg_gen_andi_tl(tmp1, temp_1, 4278190080);
    tcg_gen_shli_tl(temp_2, src, 8);
    tcg_gen_andi_tl(tmp2, temp_2, 16711680);
    tcg_gen_shri_tl(temp_3, src, 8);
    tcg_gen_andi_tl(tmp3, temp_3, 65280);
    tcg_gen_shri_tl(temp_4, src, 24);
    tcg_gen_andi_tl(tmp4, temp_4, 255);
    tcg_gen_or_tl(temp_6, tmp1, tmp2);
    tcg_gen_or_tl(temp_5, temp_6, tmp3);
    tcg_gen_or_tl(dest, temp_5, tmp4);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(dest);
        setNFlag(dest);
    }
    tcg_temp_free(temp_1);
    tcg_temp_free(tmp1);
    tcg_temp_free(temp_2);
    tcg_temp_free(tmp2);
    tcg_temp_free(temp_3);
    tcg_temp_free(tmp3);
    tcg_temp_free(temp_4);
    tcg_temp_free(tmp4);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_5);

    return ret;
}


/*
 * NOT
 *    Variables: @dest, @src
 *    Functions: getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   @dest = ~@src;
 *   f_flag = getFFlag ();
 *   if((f_flag == true))
 *     {
 *       setZFlag (@dest);
 *       setNFlag (@dest);
 *     };
 * }
 */

int
arc_gen_NOT(DisasCtxt *ctx, TCGv dest, TCGv src)
{
    int ret = DISAS_NEXT;
    int f_flag;
    tcg_gen_not_tl(dest, src);
    f_flag = getFFlag ();
    if ((f_flag == true)) {
        setZFlag(dest);
        setNFlag(dest);
    }
    return ret;
}


/*
 * BI
 *    Variables: @c
 *    Functions: setPC, getPCL
 * --- code ---
 * {
 *   setPC ((nextInsnAddress () + (@c << 2)));
 * }
 */

int
arc_gen_BI(DisasCtxt *ctx, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    tcg_gen_shli_tl(temp_4, c, 2);
    nextInsnAddress(temp_3);
    tcg_gen_mov_tl(temp_2, temp_3);
    tcg_gen_add_tl(temp_1, temp_2, temp_4);
    setPC(temp_1);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_1);

    return ret;
}


/*
 * BIH
 * --- code ---
 * target = ctx->npc + (c << 1);
 */
int
arc_gen_BIH(DisasCtxt *ctx, TCGv c)
{
    TCGv target = tcg_temp_local_new();
    TCGv addendum = tcg_temp_local_new();

    tcg_gen_movi_tl(target, ctx->npc);
    tcg_gen_shli_tl(addendum, c, 1);
    tcg_gen_add_tl(target, target, addendum);
    gen_goto_tb(ctx, target);

    tcg_temp_free(addendum);
    tcg_temp_free(target);

    return DISAS_NORETURN;
}


/*
 * B
 * --- code ---
 * target = cpl + offset
 *
 * if (cc_flag == true)
 *   gen_branchi(target)
 */
int
arc_gen_B(DisasCtxt *ctx, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[0].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    arc_gen_verifyCCFlag(ctx, cond);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cond, 0, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}

/*
 * DBNZ
 * --- code ---
 * target = cpl + offset
 *
 * @a = @a - 1
 * if (@a != 0)
 *   gen_branchi(target)
 */
int
arc_gen_DBNZ(DisasCtxt *ctx, TCGv a, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[1].value;
    TCGLabel *do_not_branch = gen_new_label();

    update_delay_flag(ctx);

    /* if (--a != 0) */
    tcg_gen_subi_tl(a, a, 1);
    tcg_gen_brcondi_tl(TCG_COND_EQ, a, 0, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);

    return DISAS_NORETURN;
}

/*
 * BBIT0
 * --- code ---
 * target = cpl + offset
 *
 * _c = @c & 31
 * msk = 1 << _c
 * bit = @b & msk
 * if (bit == 0)
 *   gen_branchi(target)
 */
int
arc_gen_BBIT0(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv _c = tcg_temp_new();
    TCGv msk = tcg_const_tl(1);
    TCGv bit = tcg_temp_new();

    update_delay_flag(ctx);

    /* if ((b & (1 << (c & 31))) == 0) */
    tcg_gen_andi_tl(_c, c, 31);
    tcg_gen_shl_tl(msk, msk, _c);
    tcg_gen_and_tl(bit, b, msk);
    tcg_gen_brcondi_tl(TCG_COND_NE, bit, 0, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(bit);
    tcg_temp_free(msk);
    tcg_temp_free(_c);

    return DISAS_NORETURN;
}


/*
 * BBIT1
 * --- code ---
 * target = cpl + offset
 *
 * _c = @c & 31
 * msk = 1 << _c
 * bit = @b & msk
 * if (bit != 0)
 *   gen_branchi(target)
 */
int
arc_gen_BBIT1(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv _c = tcg_temp_new();
    TCGv msk = tcg_const_tl(1);
    TCGv bit = tcg_temp_new();

    update_delay_flag(ctx);

    /* if ((b & (1 << (c & 31))) != 0) */
    tcg_gen_andi_tl(_c, c, 31);
    tcg_gen_shl_tl(msk, msk, _c);
    tcg_gen_and_tl(bit, b, msk);
    tcg_gen_brcondi_tl(TCG_COND_EQ, bit, 0, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(bit);
    tcg_temp_free(msk);
    tcg_temp_free(_c);

    return DISAS_NORETURN;
}


/*
 * BL
 * --- code ---
 * target = cpl + offset
 *
 * save_addr = ctx->npc
 * if (ctx->insn.d)
 *   save_addr = ctx->npc + insn_len(ctx->npc)
 *
 * if (cc_flag == true)
 * {
 *   blink = save_addr
 *   gen_branchi(target)
 * }
 */
int
arc_gen_BL(DisasCtxt *ctx, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[0].value;
    target_ulong save_addr = ctx->npc;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    /*
     * According to hardware team, fetching the delay slot, if any, happens
     * irrespective of the CC flag.
     */
    if (ctx->insn.d) {
        const ARCCPU *cpu = env_archcpu(ctx->env);
        uint16_t ds_insn;
        uint8_t ds_len;

        /* Save the PC, in case cpu_lduw_code() rasises an exception. */
        updatei_pcs(ctx->cpc);
        ds_insn = (uint16_t) cpu_lduw_code(ctx->env, ctx->npc);
        ds_len = arc_insn_length(ds_insn, cpu->family);
        save_addr = ctx->npc + ds_len;
    }

    arc_gen_verifyCCFlag(ctx, cond);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cond, 0, do_not_branch);

    tcg_gen_movi_tl(cpu_blink, save_addr);
    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/*
 * J
 * --- code ---
 * if (cc_flag == true)
 *   gen_branch(target)
 */
int
arc_gen_J(DisasCtxt *ctx, TCGv target)
{
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    arc_gen_verifyCCFlag(ctx, cond);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cond, 0, do_not_branch);

    gen_branch(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/*
 * JL
 * --- code ---
 * save_addr = ctx->npc
 * if (ctx->insn.d)
 *   save_addr = ctx->npc + insn_len(ctx->npc)
 *
 * if (cc_flag == true)
 * {
 *   _target = target
 *   blink = save_addr
 *   gen_branch(_target)
 * }
 */
int
arc_gen_JL(DisasCtxt *ctx, TCGv target)
{
    target_ulong save_addr = ctx->npc;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv _target = tcg_temp_new();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    /*
     * According to hardware team, fetching the delay slot, if any, happens
     * irrespective of the CC flag.
     */
    if (ctx->insn.d) {
        const ARCCPU *cpu = env_archcpu(ctx->env);
        uint16_t ds_insn;
        uint8_t ds_len;

        /* Save the PC, in case cpu_lduw_code() rasises an exception. */
        updatei_pcs(ctx->cpc);
        ds_insn = (uint16_t) cpu_lduw_code(ctx->env, ctx->npc);
        ds_len = arc_insn_length(ds_insn, cpu->family);
        save_addr = ctx->npc + ds_len;
    }

    arc_gen_verifyCCFlag(ctx, cond);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cond, 0, do_not_branch);

    /*
     * In case blink and target registers are the same, i.e.:
     *   jl   [blink]
     * We have to use the blink's value before it is overwritten.
     */
    tcg_gen_mov_tl(_target, target);
    tcg_gen_movi_tl(cpu_blink, save_addr);
    gen_branch(ctx, _target);

    gen_set_label(do_not_branch);

    tcg_temp_free(cond);
    tcg_temp_free(_target);

    return DISAS_NORETURN;
}


/*
 * SETEQ
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       p_b = @b;
 *       p_c = @c;
 *       take_branch = false;
 *       if((p_b == p_c))
 *         {
 *         }
 *       else
 *         {
 *         };
 *       if((p_b == p_c))
 *         {
 *           @a = true;
 *         }
 *       else
 *         {
 *           @a = false;
 *         };
 *     };
 * }
 */

int
arc_gen_SETEQ(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_7 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv p_b = tcg_temp_local_new();
    TCGv p_c = tcg_temp_local_new();
    TCGv take_branch = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_7);
    tcg_gen_mov_tl(cc_flag, temp_7);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(p_b, b);
    tcg_gen_mov_tl(p_c, c);
    tcg_gen_mov_tl(take_branch, arc_false);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_3, p_b, p_c);
    tcg_gen_xori_tl(temp_4, temp_3, 1);
    tcg_gen_andi_tl(temp_4, temp_4, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    gen_set_label(done_2);
    TCGLabel *else_3 = gen_new_label();
    TCGLabel *done_3 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_5, p_b, p_c);
    tcg_gen_xori_tl(temp_6, temp_5, 1);
    tcg_gen_andi_tl(temp_6, temp_6, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_3);
    tcg_gen_mov_tl(a, arc_true);
    tcg_gen_br(done_3);
    gen_set_label(else_3);
    tcg_gen_mov_tl(a, arc_false);
    gen_set_label(done_3);
    gen_set_label(done_1);
    tcg_temp_free(temp_7);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(p_b);
    tcg_temp_free(p_c);
    tcg_temp_free(take_branch);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * BREQ
 * --- code ---
 * target = cpl + offset
 *
 * if (b == c)
 *   gen_branchi(target)
 */
int
arc_gen_BREQ(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_brcond_tl(TCG_COND_NE, b, c, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/*
 * SETNE
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       p_b = @b;
 *       p_c = @c;
 *       take_branch = false;
 *       if((p_b != p_c))
 *         {
 *         }
 *       else
 *         {
 *         };
 *       if((p_b != p_c))
 *         {
 *           @a = true;
 *         }
 *       else
 *         {
 *           @a = false;
 *         };
 *     };
 * }
 */

int
arc_gen_SETNE(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_7 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv p_b = tcg_temp_local_new();
    TCGv p_c = tcg_temp_local_new();
    TCGv take_branch = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_7);
    tcg_gen_mov_tl(cc_flag, temp_7);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(p_b, b);
    tcg_gen_mov_tl(p_c, c);
    tcg_gen_mov_tl(take_branch, arc_false);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_NE, temp_3, p_b, p_c);
    tcg_gen_xori_tl(temp_4, temp_3, 1);
    tcg_gen_andi_tl(temp_4, temp_4, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    gen_set_label(done_2);
    TCGLabel *else_3 = gen_new_label();
    TCGLabel *done_3 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_NE, temp_5, p_b, p_c);
    tcg_gen_xori_tl(temp_6, temp_5, 1);
    tcg_gen_andi_tl(temp_6, temp_6, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_3);
    tcg_gen_mov_tl(a, arc_true);
    tcg_gen_br(done_3);
    gen_set_label(else_3);
    tcg_gen_mov_tl(a, arc_false);
    gen_set_label(done_3);
    gen_set_label(done_1);
    tcg_temp_free(temp_7);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(p_b);
    tcg_temp_free(p_c);
    tcg_temp_free(take_branch);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * BRNE
 * --- code ---
 * target = cpl + offset
 *
 * if (b != c)
 *   gen_branchi(target)
 */
int
arc_gen_BRNE(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_brcond_tl(TCG_COND_EQ, b, c, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/*
 * SETLT
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       p_b = @b;
 *       p_c = @c;
 *       take_branch = false;
 *       if((p_b < p_c))
 *         {
 *         }
 *       else
 *         {
 *         };
 *       if((p_b < p_c))
 *         {
 *           @a = true;
 *         }
 *       else
 *         {
 *           @a = false;
 *         };
 *     };
 * }
 */

int
arc_gen_SETLT(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_7 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv p_b = tcg_temp_local_new();
    TCGv p_c = tcg_temp_local_new();
    TCGv take_branch = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_7);
    tcg_gen_mov_tl(cc_flag, temp_7);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(p_b, b);
    tcg_gen_mov_tl(p_c, c);
    tcg_gen_mov_tl(take_branch, arc_false);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_LT, temp_3, p_b, p_c);
    tcg_gen_xori_tl(temp_4, temp_3, 1);
    tcg_gen_andi_tl(temp_4, temp_4, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    gen_set_label(done_2);
    TCGLabel *else_3 = gen_new_label();
    TCGLabel *done_3 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_LT, temp_5, p_b, p_c);
    tcg_gen_xori_tl(temp_6, temp_5, 1);
    tcg_gen_andi_tl(temp_6, temp_6, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_3);
    tcg_gen_mov_tl(a, arc_true);
    tcg_gen_br(done_3);
    gen_set_label(else_3);
    tcg_gen_mov_tl(a, arc_false);
    gen_set_label(done_3);
    gen_set_label(done_1);
    tcg_temp_free(temp_7);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(p_b);
    tcg_temp_free(p_c);
    tcg_temp_free(take_branch);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * BRLT
 * --- code ---
 * target = cpl + offset
 *
 * if (b s< c)
 *   gen_branchi(target)
 * }
 */
int
arc_gen_BRLT(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_brcond_tl(TCG_COND_GE, b, c, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/*
 * SETGE
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       p_b = @b;
 *       p_c = @c;
 *       take_branch = false;
 *       if((p_b >= p_c))
 *         {
 *         }
 *       else
 *         {
 *         };
 *       if((p_b >= p_c))
 *         {
 *           @a = true;
 *         }
 *       else
 *         {
 *           @a = false;
 *         };
 *     };
 * }
 */

int
arc_gen_SETGE(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_7 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv p_b = tcg_temp_local_new();
    TCGv p_c = tcg_temp_local_new();
    TCGv take_branch = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_7);
    tcg_gen_mov_tl(cc_flag, temp_7);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(p_b, b);
    tcg_gen_mov_tl(p_c, c);
    tcg_gen_mov_tl(take_branch, arc_false);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_GE, temp_3, p_b, p_c);
    tcg_gen_xori_tl(temp_4, temp_3, 1);
    tcg_gen_andi_tl(temp_4, temp_4, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    gen_set_label(done_2);
    TCGLabel *else_3 = gen_new_label();
    TCGLabel *done_3 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_GE, temp_5, p_b, p_c);
    tcg_gen_xori_tl(temp_6, temp_5, 1);
    tcg_gen_andi_tl(temp_6, temp_6, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_3);
    tcg_gen_mov_tl(a, arc_true);
    tcg_gen_br(done_3);
    gen_set_label(else_3);
    tcg_gen_mov_tl(a, arc_false);
    gen_set_label(done_3);
    gen_set_label(done_1);
    tcg_temp_free(temp_7);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(p_b);
    tcg_temp_free(p_c);
    tcg_temp_free(take_branch);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * BRGE
 * --- code ---
 * target = cpl + offset
 *
 * if (b s>= c)
 *   gen_branchi(target)
 */
int
arc_gen_BRGE(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_brcond_tl(TCG_COND_LT, b, c, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/*
 * SETLE
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       p_b = @b;
 *       p_c = @c;
 *       take_branch = false;
 *       if((p_b <= p_c))
 *         {
 *         }
 *       else
 *         {
 *         };
 *       if((p_b <= p_c))
 *         {
 *           @a = true;
 *         }
 *       else
 *         {
 *           @a = false;
 *         };
 *     };
 * }
 */

int
arc_gen_SETLE(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_7 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv p_b = tcg_temp_local_new();
    TCGv p_c = tcg_temp_local_new();
    TCGv take_branch = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_7);
    tcg_gen_mov_tl(cc_flag, temp_7);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(p_b, b);
    tcg_gen_mov_tl(p_c, c);
    tcg_gen_mov_tl(take_branch, arc_false);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_LE, temp_3, p_b, p_c);
    tcg_gen_xori_tl(temp_4, temp_3, 1);
    tcg_gen_andi_tl(temp_4, temp_4, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    gen_set_label(done_2);
    TCGLabel *else_3 = gen_new_label();
    TCGLabel *done_3 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_LE, temp_5, p_b, p_c);
    tcg_gen_xori_tl(temp_6, temp_5, 1);
    tcg_gen_andi_tl(temp_6, temp_6, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_3);
    tcg_gen_mov_tl(a, arc_true);
    tcg_gen_br(done_3);
    gen_set_label(else_3);
    tcg_gen_mov_tl(a, arc_false);
    gen_set_label(done_3);
    gen_set_label(done_1);
    tcg_temp_free(temp_7);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(p_b);
    tcg_temp_free(p_c);
    tcg_temp_free(take_branch);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * SETGT
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       p_b = @b;
 *       p_c = @c;
 *       take_branch = false;
 *       if((p_b > p_c))
 *         {
 *         }
 *       else
 *         {
 *         };
 *       if((p_b > p_c))
 *         {
 *           @a = true;
 *         }
 *       else
 *         {
 *           @a = false;
 *         };
 *     };
 * }
 */

int
arc_gen_SETGT(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_7 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv p_b = tcg_temp_local_new();
    TCGv p_c = tcg_temp_local_new();
    TCGv take_branch = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_7);
    tcg_gen_mov_tl(cc_flag, temp_7);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_mov_tl(p_b, b);
    tcg_gen_mov_tl(p_c, c);
    tcg_gen_mov_tl(take_branch, arc_false);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_GT, temp_3, p_b, p_c);
    tcg_gen_xori_tl(temp_4, temp_3, 1);
    tcg_gen_andi_tl(temp_4, temp_4, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    gen_set_label(done_2);
    TCGLabel *else_3 = gen_new_label();
    TCGLabel *done_3 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_GT, temp_5, p_b, p_c);
    tcg_gen_xori_tl(temp_6, temp_5, 1);
    tcg_gen_andi_tl(temp_6, temp_6, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_3);
    tcg_gen_mov_tl(a, arc_true);
    tcg_gen_br(done_3);
    gen_set_label(else_3);
    tcg_gen_mov_tl(a, arc_false);
    gen_set_label(done_3);
    gen_set_label(done_1);
    tcg_temp_free(temp_7);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(p_b);
    tcg_temp_free(p_c);
    tcg_temp_free(take_branch);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);

    return ret;
}


/*
 * BRLO
 * --- code ---
 * target = cpl + offset
 *
 * if (b u< c)
 *   gen_branchi(target)
 */
int
arc_gen_BRLO(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_brcond_tl(TCG_COND_GEU, b, c, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/*
 * SETLO
 *    Variables: @b, @c, @a
 *    Functions: unsignedLT
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       p_b = @b;
 *       p_c = @c;
 *       take_branch = false;
 *       if(unsignedLT (p_b, p_c))
 *         {
 *         }
 *       else
 *         {
 *         };
 *       if(unsignedLT (p_b, p_c))
 *         {
 *           @a = true;
 *         }
 *       else
 *         {
 *           @a = false;
 *         };
 *     }
 * }
 */

int
arc_gen_SETLO(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv p_b = tcg_temp_local_new();
    TCGv p_c = tcg_temp_local_new();
    TCGv take_branch = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv cc_temp_1 = tcg_temp_local_new();
    getCCFlag(cc_flag);
    TCGLabel *done_cc = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, cc_temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(cc_temp_1, cc_temp_1, 1); tcg_gen_andi_tl(cc_temp_1, cc_temp_1, 1);;
    tcg_gen_brcond_tl(TCG_COND_EQ, cc_temp_1, arc_true, done_cc);;
    tcg_gen_mov_tl(p_b, b);
    tcg_gen_mov_tl(p_c, c);
    tcg_gen_mov_tl(take_branch, arc_false);
    TCGLabel *else_1 = gen_new_label();
    TCGLabel *done_1 = gen_new_label();
    unsignedLT(temp_3, p_b, p_c);
    tcg_gen_xori_tl(temp_1, temp_3, 1);
    tcg_gen_andi_tl(temp_1, temp_1, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_1, arc_true, else_1);
    tcg_gen_br(done_1);
    gen_set_label(else_1);
    gen_set_label(done_1);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    unsignedLT(temp_4, p_b, p_c);
    tcg_gen_xori_tl(temp_2, temp_4, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_2);
    tcg_gen_mov_tl(a, arc_true);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    tcg_gen_mov_tl(a, arc_false);
    gen_set_label(done_2);
    gen_set_label(done_cc);
    tcg_temp_free(p_b);
    tcg_temp_free(p_c);
    tcg_temp_free(take_branch);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_2);
    tcg_temp_free(cc_temp_1);
    tcg_temp_free(cc_flag);

    return ret;
}


/*
 * BRHS
 * --- code ---
 * target = cpl + offset
 *
 * if (b u>= c)
 *   gen_branchi(target)
 */
int
arc_gen_BRHS(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_brcond_tl(TCG_COND_LTU, b, c, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/*
 * SETHS
 *    Variables: @b, @c, @a
 *    Functions: unsignedGE
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       p_b = @b;
 *       p_c = @c;
 *       take_branch = false;
 *       if(unsignedGE (p_b, p_c))
 *         {
 *         }
 *       else
 *         {
 *         };
 *       if(unsignedGE (p_b, p_c))
 *         {
 *           @a = true;
 *         }
 *       else
 *         {
 *           @a = false;
 *         };
 *     }
 * }
 */

int
arc_gen_SETHS(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv p_b = tcg_temp_local_new();
    TCGv p_c = tcg_temp_local_new();
    TCGv take_branch = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv cc_temp_1 = tcg_temp_local_new();
    getCCFlag(cc_flag);
    TCGLabel *done_cc = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, cc_temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(cc_temp_1, cc_temp_1, 1); tcg_gen_andi_tl(cc_temp_1, cc_temp_1, 1);;
    tcg_gen_brcond_tl(TCG_COND_EQ, cc_temp_1, arc_true, done_cc);;
    tcg_gen_mov_tl(p_b, b);
    tcg_gen_mov_tl(p_c, c);
    tcg_gen_mov_tl(take_branch, arc_false);
    TCGLabel *else_1 = gen_new_label();
    TCGLabel *done_1 = gen_new_label();
    unsignedGE(temp_3, p_b, p_c);
    tcg_gen_xori_tl(temp_1, temp_3, 1);
    tcg_gen_andi_tl(temp_1, temp_1, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_1, arc_true, else_1);
    tcg_gen_br(done_1);
    gen_set_label(else_1);
    gen_set_label(done_1);
    TCGLabel *else_2 = gen_new_label();
    TCGLabel *done_2 = gen_new_label();
    unsignedGE(temp_4, p_b, p_c);
    tcg_gen_xori_tl(temp_2, temp_4, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_2);
    tcg_gen_mov_tl(a, arc_true);
    tcg_gen_br(done_2);
    gen_set_label(else_2);
    tcg_gen_mov_tl(a, arc_false);
    gen_set_label(done_2);
    gen_set_label(done_cc);
    tcg_temp_free(p_b);
    tcg_temp_free(p_c);
    tcg_temp_free(take_branch);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_2);
    tcg_temp_free(cc_temp_1);
    tcg_temp_free(cc_flag);

    return ret;
}


/*
 * EX - CODED BY HAND
 */

int
arc_gen_EX (DisasCtxt *ctx, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp = tcg_temp_local_new();
  tcg_gen_mov_tl(temp, b);
  tcg_gen_atomic_xchg_tl(b, c, temp, ctx->mem_idx, MO_UL);
  tcg_temp_free(temp);

  return ret;
}


//#define ARM_LIKE_LLOCK_SCOND

extern TCGv cpu_exclusive_addr;
extern TCGv cpu_exclusive_val;
extern TCGv cpu_exclusive_val_hi;

/*
 * LLOCK -- CODED BY HAND
 */

int
arc_gen_LLOCK(DisasCtxt *ctx, TCGv dest, TCGv src)
{
    int ret = DISAS_NEXT;
#ifndef ARM_LIKE_LLOCK_SCOND
    gen_helper_llock(dest, cpu_env, src);
#else
    tcg_gen_qemu_ld_tl(cpu_exclusive_val, src, ctx->mem_idx, MO_UL);
    tcg_gen_mov_tl(dest, cpu_exclusive_val);
    tcg_gen_mov_tl(cpu_exclusive_addr, src);
#endif

    return ret;
}


/*
 * LLOCKD -- CODED BY HAND
 */

int
arc_gen_LLOCKD(DisasCtxt *ctx, TCGv dest, TCGv src)
{
    int ret = DISAS_NEXT;
    TCGv pair = nextReg (dest);

    TCGv_i64 temp_1 = tcg_temp_local_new_i64();
    TCGv_i64 temp_2 = tcg_temp_local_new_i64();

#ifndef ARM_LIKE_LLOCK_SCOND
    gen_helper_llockd(temp_1, cpu_env, src);
#else
    tcg_gen_qemu_ld_i64(temp_1, src, ctx->mem_idx, MO_UQ);
    tcg_gen_mov_tl(cpu_exclusive_addr, src);

    tcg_gen_shri_i64(temp_2, temp_1, 32);
    tcg_gen_trunc_i64_tl(cpu_exclusive_val_hi, temp_2);
    tcg_gen_trunc_i64_tl(cpu_exclusive_val, temp_1);
#endif

    tcg_gen_shri_i64(temp_2, temp_1, 32);
    tcg_gen_trunc_i64_tl(pair, temp_2);
    tcg_gen_trunc_i64_tl(dest, temp_1);

    tcg_temp_free_i64(temp_1);
    tcg_temp_free_i64(temp_2);


    return ret;
}


/*
 * SCOND -- CODED BY HAND
 */

int
arc_gen_SCOND(DisasCtxt *ctx, TCGv addr, TCGv value)
{
    int ret = DISAS_NEXT;
#ifndef ARM_LIKE_LLOCK_SCOND
    TCGv temp_4 = tcg_temp_local_new();
    gen_helper_scond(temp_4, cpu_env, addr, value);
    setZFlag(temp_4);
    tcg_temp_free(temp_4);
#else
    TCGLabel *fail_label = gen_new_label();
    TCGLabel *done_label = gen_new_label();
    TCGv tmp;

    tcg_gen_brcond_tl(TCG_COND_NE, addr, cpu_exclusive_addr, fail_label);
    tmp = tcg_temp_new();

    tcg_gen_atomic_cmpxchg_tl(tmp, cpu_exclusive_addr, cpu_exclusive_val,
                               value, ctx->mem_idx,
                               MO_UL | MO_ALIGN);
    tcg_gen_setcond_tl(TCG_COND_NE, tmp, tmp, cpu_exclusive_val);

    setZFlag(tmp);

    tcg_temp_free(tmp);
    tcg_gen_br(done_label);

    gen_set_label(fail_label);
    tcg_gen_movi_tl(cpu_Zf, 1);
    gen_set_label(done_label);
    tcg_gen_movi_tl(cpu_exclusive_addr, -1);
#endif

    return ret;
}


/*
 * SCONDD -- CODED BY HAND
 */

int
arc_gen_SCONDD(DisasCtxt *ctx, TCGv addr, TCGv value)
{
    int ret = DISAS_NEXT;
    TCGv pair = NULL;
    pair = nextReg (value);

    TCGv_i64 temp_1 = tcg_temp_local_new_i64();
    TCGv_i64 temp_2 = tcg_temp_local_new_i64();

    TCGv_i64 temp_3 = tcg_temp_local_new_i64();
    TCGv_i64 temp_4 = tcg_temp_local_new_i64();
    TCGv_i64 exclusive_val = tcg_temp_local_new_i64();

    tcg_gen_ext_i32_i64(temp_1, pair);
    tcg_gen_extu_i32_i64(temp_2, value);
    tcg_gen_shli_i64(temp_1, temp_1, 32);
    tcg_gen_or_i64(temp_1, temp_1, temp_2);

#ifndef ARM_LIKE_LLOCK_SCOND
    TCGv temp_5 = tcg_temp_local_new();
    gen_helper_scondd(temp_5, cpu_env, addr, temp_1);
    setZFlag(temp_5);
    tcg_temp_free(temp_5);
#else
    TCGLabel *fail_label = gen_new_label();
    TCGLabel *done_label = gen_new_label();

    tcg_gen_ext_i32_i64(temp_3, cpu_exclusive_val_hi);
    tcg_gen_ext_i32_i64(temp_4, cpu_exclusive_val);
    tcg_gen_shli_i64(temp_3, temp_3, 32);

    tcg_gen_brcond_tl(TCG_COND_NE, addr, cpu_exclusive_addr, fail_label);

    TCGv_i64 tmp = tcg_temp_new_i64();
    TCGv tmp1 = tcg_temp_new();

    tcg_gen_or_i64(exclusive_val, temp_3, temp_4);

    tcg_gen_atomic_cmpxchg_i64(tmp, cpu_exclusive_addr, exclusive_val,
                               temp_1, ctx->mem_idx,
                               MO_UL | MO_ALIGN);
    tcg_gen_setcond_i64(TCG_COND_NE, tmp, tmp, exclusive_val);
    tcg_gen_trunc_i64_tl(tmp1, tmp);
    setZFlag(tmp1);

    tcg_temp_free_i64(tmp);
    tcg_temp_free(tmp1);
    tcg_gen_br(done_label);

    gen_set_label(fail_label);
    tcg_gen_movi_tl(cpu_Zf, 1);
    gen_set_label(done_label);
    tcg_gen_movi_tl(cpu_exclusive_addr, -1);
#endif

    tcg_temp_free_i64(temp_1);
    tcg_temp_free_i64(temp_2);
    tcg_temp_free_i64(temp_3);
    tcg_temp_free_i64(temp_4);
    tcg_temp_free_i64(exclusive_val);

    return ret;
}


/* DMB - HAND MADE
 */

int
arc_gen_DMB (DisasCtxt *ctx, TCGv a)
{
  TCGBar bar = 0;
  switch(ctx->insn.operands[0].value & 7) {
    case 1:
      bar |= TCG_BAR_SC | TCG_MO_LD_LD | TCG_MO_LD_ST;
      break;
    case 2:
      bar |= TCG_BAR_SC | TCG_MO_ST_ST;
      break;
    default:
      bar |= TCG_BAR_SC | TCG_MO_ALL;
      break;
  }
  tcg_gen_mb(bar);

  return DISAS_NORETURN;
}


/*
 * LD
 * --- code ---
 * {
 *   if(aa == 0 || aa == 1)
 *       address = src1 + src2;
 *   if(aa == 2)
 *       address = src1;
 *   if(aa == 3 && (zz == 0 || zz == 3))
 *       address = src1 + (src2 << 2);
 *   if(aa == 3 && zz == 2)
 *       address = src1 + (src2 << 1);
 *   setDebugLD (1);
 *   new_dest = getMemory (address, zz);
 *   if(aa == 1 || aa == 2)
 *       src1 = src1 + src2;
 *   if(x == 1)
 *       new_dest = SignExtend (new_dest, zz);
 *   if(arc_gen_no_further_loads_pending ())
 *       setDebugLD (0);
 *   dest = new_dest;
 * }
 */

int
arc_gen_LD(DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
    int aa = ctx->insn.aa;
    int zz = ctx->insn.zz;

    TCGv address = tcg_temp_new();
    TCGv loads_pending = tcg_temp_new();
    TCGv scaled_disp = tcg_temp_new();
    TCGv temp = tcg_temp_new();

    /* compute source address */
    tcg_gen_mov_tl(address, src1);
    if (aa == 0 || aa == 1) {
        tcg_gen_add_tl(address, address, src2);
    } else if (aa == 3) {
        if (zz == 0 || zz == 3) {
            tcg_gen_shli_tl(scaled_disp, src2, 2);
        } else if (zz == 2) {
            tcg_gen_shli_tl(scaled_disp, src2, 1);
        }
        tcg_gen_add_tl(address, src1, scaled_disp);
    }

    /* indicate load pending */
    setDebugLD(tcg_constant_tl(1));

    /* perform the load */
    tcg_gen_qemu_ld_tl(temp, address, ctx->mem_idx,
        memop_for_size_sign[ctx->insn.x][zz]);

    /* update source register if needed */
    if (aa == 1 || aa == 2) {
        tcg_gen_add_tl(src1, src1, src2);
    }

    /* if no loads are pending, clear DEBUG.LD */
    TCGLabel *done_1 = gen_new_label();
    arc_gen_no_further_loads_pending(ctx, loads_pending);
    tcg_gen_andi_tl(loads_pending, loads_pending, 1);
    tcg_gen_brcondi_tl(TCG_COND_NE, loads_pending, 1, done_1);
    setDebugLD(tcg_constant_tl(0));
    gen_set_label(done_1);

    /* publish the load */
    tcg_gen_mov_tl(dest, temp);

    tcg_temp_free(temp);
    tcg_temp_free(scaled_disp);
    tcg_temp_free(loads_pending);
    tcg_temp_free(address);

    return DISAS_NEXT;
}


/*
 * LDD
 * --- code ---
 * {
 *   if(aa == 0 || aa == 1)
 *       address = src1 + src2;
 *   if(aa == 2)
 *       address = src1;
 *   if(aa == 3 && (zz == 0 || zz == 3))
 *       address = src1 + (src2 << 2);
 *   if(aa == 3 && zz == 2)
 *       address = src1 + (src2 << 1);
 *   setDebugLD (1);
 *   new_dest = getMemory (address, LONG);
 *   pair = nextReg (dest);
 *   pair = getMemory (address + 4, LONG);
 *   if(aa == 1 || aa == 2)
 *       src1 = src1 + src2;
 *   if(arc_gen_no_further_loads_pending ())
 *       setDebugLD (0);
 *   dest = new_dest;
 * }
 */

int
arc_gen_LDD(DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
    int aa = ctx->insn.aa;
    int zz = ctx->insn.zz;

    TCGv address = tcg_temp_new();
    TCGv scaled_disp = tcg_temp_new();
    TCGv loads_pending = tcg_temp_new();
    TCGv temp_lo = tcg_temp_new();
    TCGv temp_hi = tcg_temp_new();

    /* compute source address */
    tcg_gen_mov_tl(address, src1);
    if (aa == 0 || aa == 1) {
        tcg_gen_add_tl(address, address, src2);
    } else if (aa == 3) {
        if (zz == 0 || zz == 3) {
            tcg_gen_shli_tl(scaled_disp, src2, 2);
        } else if (zz == 2) {
            tcg_gen_shli_tl(scaled_disp, src2, 1);
        }
        tcg_gen_add_tl(address, src1, scaled_disp);
    }

    /* indicate load pending */
    setDebugLD(tcg_constant_tl(1));

    /* perform both loads */
    tcg_gen_qemu_ld_tl(temp_lo, address, ctx->mem_idx, MO_UL);
    tcg_gen_addi_tl(address, address, 4);
    tcg_gen_qemu_ld_tl(temp_hi, address, ctx->mem_idx, MO_UL);

    /* update source register if needed */
    if (aa == 1 || aa == 2) {
        tcg_gen_add_tl(src1, src1, src2);
    }

    /* if no loads are pending, clear DEBUG.LD */
    TCGLabel *done_1 = gen_new_label();
    arc_gen_no_further_loads_pending(ctx, loads_pending);
    tcg_gen_andi_tl(loads_pending, loads_pending, 1);
    tcg_gen_brcondi_tl(TCG_COND_NE, loads_pending, 1, done_1);
    setDebugLD(tcg_constant_tl(0));
    gen_set_label(done_1);

    /* publish the loads */
    tcg_gen_mov_tl(dest, temp_lo);
    tcg_gen_mov_tl(nextReg(dest), temp_hi);

    tcg_temp_free(temp_hi);
    tcg_temp_free(temp_lo);
    tcg_temp_free(loads_pending);
    tcg_temp_free(scaled_disp);
    tcg_temp_free(address);

    return DISAS_NEXT;
}


/*
 * ST
 * --- code ---
 * {
 *   if(aa == 0 || aa == 1)
 *       address = src1 + src2;
 *   if(aa == 2)
 *       address = src1;
 *   if(aa == 3 && (zz == 0 || zz == 3))
 *       address = src1 + (src2 << 2);
 *   if(aa == 3 && (zz == 2)
 *       address = src1 + (src2 << 1);
 *   setMemory (address, zz, dest);
 *   if(aa == 1 || aa == 2)
 *       src1 = src1 + src2;
 * }
 */

int
arc_gen_ST(DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
    int aa = ctx->insn.aa;
    int zz = ctx->insn.zz;
    TCGv address = tcg_temp_new();
    TCGv scaled_disp = tcg_temp_new();

    /* compute destination address */
    tcg_gen_mov_tl(address, src1);
    if (aa == 0 || aa == 1) {
        tcg_gen_add_tl(address, address, src2);
    } else if (aa == 3) {
        if (zz == 0 || zz == 3) {
            tcg_gen_shli_tl(scaled_disp, src2, 2);
        } else if (zz == 2) {
            tcg_gen_shli_tl(scaled_disp, src2, 1);
        }
        tcg_gen_add_tl(address, src1, scaled_disp);
    }

    /* perform the store */
    tcg_gen_qemu_st_tl(dest, address, ctx->mem_idx,
        memop_for_size_sign[ctx->insn.x][zz]);

    /* update destination register if needed */
    if (aa == 1 || aa == 2) {
        tcg_gen_add_tl(src1, src1, src2);
    }

    tcg_temp_free(scaled_disp);
    tcg_temp_free(address);

    return DISAS_NEXT;
}


/*
 * STD
 * --- code ---
 * {
 *   if(aa == 0 || aa == 1)
 *       address = src1 + src2;
 *   if(aa == 2)
 *       address = src1;
 *   if(aa == 3 && (zz == 0 || zz == 3))
 *       address = src1 + (src2 << 2);
 *   if(aa == 3 && zz == 2)
 *       address = src1 + (src2 << 1);
 *   setMemory (address, LONG, dest);
 *   if(instructionHasRegisterOperandIn (0))
 *     {
 *       pair = nextReg (dest);
 *       setMemory (address + 4, LONG, pair);
 *     }
 *   else
 *     {
 *       tmp = 0;
 *       if(getBit (@dest, 31) == 1)
 *           tmp = 4294967295;
 *       setMemory (address + 4, LONG, tmp);
 *     }
 *   if(aa == 1 || aa == 2)
 *       src1 = src1 + src2;
 * }
 */

int
arc_gen_STD(DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
    int aa = ctx->insn.aa;
    int zz = ctx->insn.zz;

    TCGv address = tcg_temp_new();
    TCGv scaled_disp = tcg_temp_new();

    /* compute destination address */
    tcg_gen_mov_tl(address, src1);
    if (aa == 0 || aa == 1) {
        tcg_gen_add_tl(address, address, src2);
    } else if (aa == 3) {
        if (zz == 0 || zz == 3) {
            tcg_gen_shli_tl(scaled_disp, src2, 2);
        } else if (zz == 2) {
            tcg_gen_shli_tl(scaled_disp, src2, 1);
        }
        tcg_gen_add_tl(address, src1, scaled_disp);
    }

    /* write the low 32 bits */
    tcg_gen_qemu_st_tl(dest, address, ctx->mem_idx, MO_UL);

    /* depending on the type of the destination operand, either ... */
    if (ctx->insn.operands[0].type & ARC_OPERAND_IR) {
        /* write the contents of the high 32 bits if it's a register, or ... */
        tcg_gen_addi_tl(address, address, 4);
        tcg_gen_qemu_st_tl(nextReg(dest), address, ctx->mem_idx, MO_UL);
    } else {
        /* sign-extend to 64 bits if it's an immediate */
        TCGv value_hi = tcg_temp_new();
        tcg_gen_sari_tl(value_hi, dest, TARGET_LONG_BITS - 1);

        tcg_gen_addi_tl(address, address, 4);
        tcg_gen_qemu_st_tl(value_hi, address, ctx->mem_idx, MO_UL);

        tcg_temp_free(value_hi);
    }

    /* update destination register if needed */
    if (aa == 1 || aa == 2) {
        tcg_gen_add_tl(src1, src1, src2);
    }

    tcg_temp_free(scaled_disp);
    tcg_temp_free(address);

    return DISAS_NEXT;
}


/*
 * POP
 * --- code ---
 * {
 *   new_dest = getMemory (arc_gen_get_register (R_SP), LONG);
 *   setRegister (R_SP, (arc_gen_get_register (R_SP) + 4));
 *   dest = new_dest;
 * }
 */

int
arc_gen_POP(DisasCtxt *ctx, TCGv dest)
{
    tcg_gen_qemu_ld_tl(dest, cpu_sp, ctx->mem_idx, MO_UL);
    if (dest != cpu_sp) {
        tcg_gen_addi_tl(cpu_sp, cpu_sp, 4);
    }

    return DISAS_NEXT;
}


/*
 * PUSH
 * --- code ---
 * {
 *   setMemory ((arc_gen_get_register (R_SP) - 4), LONG, src);
 *   setRegister (R_SP, (arc_gen_get_register (R_SP) - 4));
 * }
 */

int
arc_gen_PUSH(DisasCtxt *ctx, TCGv src)
{
    TCGv sp = tcg_temp_new();

    tcg_gen_mov_tl(sp, cpu_sp);
    tcg_gen_subi_tl(sp, sp, 4);
    tcg_gen_qemu_st_tl(src, sp, ctx->mem_idx, MO_UL);
    tcg_gen_mov_tl(cpu_sp, sp);

    tcg_temp_free(sp);

    return DISAS_NEXT;
}


/*
 * LP
 *    Variables: @rd
 *    Functions: getCCFlag, getRegIndex, writeAuxReg, nextInsnAddress, getPCL,
 *               setPC
 * --- code ---
 * {
 *   if((getCCFlag () == true))
 *     {
 *       lp_start_index = getRegIndex (LP_START);
 *       lp_end_index = getRegIndex (LP_END);
 *       writeAuxReg (lp_start_index, nextInsnAddress ());
 *       writeAuxReg (lp_end_index, (getPCL () + @rd));
 *     }
 *   else
 *     {
 *       setPC ((getPCL () + @rd));
 *     };
 * }
 */

int
arc_gen_LP(DisasCtxt *ctx, TCGv rd)
{
    int ret = DISAS_NORETURN;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv lp_start_index = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv lp_end_index = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_10 = tcg_temp_local_new();
    TCGv temp_9 = tcg_temp_local_new();
    TCGv temp_8 = tcg_temp_local_new();
    TCGv temp_13 = tcg_temp_local_new();
    TCGv temp_12 = tcg_temp_local_new();
    TCGv temp_11 = tcg_temp_local_new();
    TCGLabel *else_1 = gen_new_label();
    TCGLabel *done_1 = gen_new_label();
    getCCFlag(temp_3);
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, temp_3, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_1);
    getRegIndex(temp_4, LP_START);
    tcg_gen_mov_tl(lp_start_index, temp_4);
    getRegIndex(temp_5, LP_END);
    tcg_gen_mov_tl(lp_end_index, temp_5);
    nextInsnAddress(temp_7);
    tcg_gen_mov_tl(temp_6, temp_7);
    writeAuxReg(lp_start_index, temp_6);
    getPCL(temp_10);
    tcg_gen_mov_tl(temp_9, temp_10);
    tcg_gen_add_tl(temp_8, temp_9, rd);
    writeAuxReg(lp_end_index, temp_8);
    tcg_gen_br(done_1);
    gen_set_label(else_1);
    getPCL(temp_13);
    tcg_gen_mov_tl(temp_12, temp_13);
    tcg_gen_add_tl(temp_11, temp_12, rd);
    setPC(temp_11);
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);
    tcg_temp_free(lp_start_index);
    tcg_temp_free(temp_5);
    tcg_temp_free(lp_end_index);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_10);
    tcg_temp_free(temp_9);
    tcg_temp_free(temp_8);
    tcg_temp_free(temp_13);
    tcg_temp_free(temp_12);
    tcg_temp_free(temp_11);

    return ret;
}


/*
 * NORM
 *    Variables: @src, @dest
 *    Functions: CRLSB, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   psrc = @src;
 *   @dest = CRLSB (psrc);
 *   if((getFFlag () == true))
 *     {
 *       setZFlag (psrc);
 *       setNFlag (psrc);
 *     };
 * }
 */

int
arc_gen_NORM(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv psrc = tcg_temp_local_new();
    tcg_gen_mov_tl(psrc, src);
    tcg_gen_clrsb_tl(dest, psrc);
    if ((getFFlag () == true)) {
        setZFlag(psrc);
        setNFlag(psrc);
    }
    tcg_temp_free(psrc);

    return ret;
}


/*
 * NORMH
 *    Variables: @src, @dest
 *    Functions: SignExtend16to32, CRLSB, getFFlag, setZFlagByNum, setNFlagByNum
 * --- code ---
 * {
 *   psrc = (@src & 65535);
 *   psrc = SignExtend16to32 (psrc);
 *   @dest = CRLSB (psrc);
 *   @dest = (@dest - 16);
 *   if((getFFlag () == true))
 *     {
 *       setZFlagByNum (psrc, 16);
 *       setNFlagByNum (psrc, 16);
 *     };
 * }
 */

int
arc_gen_NORMH(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv psrc = tcg_temp_local_new();
    tcg_gen_andi_tl(psrc, src, 65535);
    tcg_gen_ext16s_tl(psrc, psrc);
    tcg_gen_clrsb_tl(dest, psrc);
    tcg_gen_subi_tl(dest, dest, 16);
    if ((getFFlag () == true)) {
        setZFlagByNum(psrc, 16);
        setNFlagByNum(psrc, 16);
    }
    tcg_temp_free(psrc);

    return ret;
}


/*
 * FLS
 *    Variables: @src, @dest
 *    Functions: CLZ, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   psrc = @src;
 *   if((psrc == 0))
 *     {
 *       @dest = 0;
 *     }
 *   else
 *     {
 *       @dest = 31 - CLZ (psrc, 32);
 *     };
 *   if((getFFlag () == true))
 *     {
 *       setZFlag (psrc);
 *       setNFlag (psrc);
 *     };
 * }
 */

int
arc_gen_FLS(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv psrc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    tcg_gen_mov_tl(psrc, src);
    TCGLabel *else_1 = gen_new_label();
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_1, psrc, 0);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_1);
    tcg_gen_movi_tl(dest, 0);
    tcg_gen_br(done_1);
    gen_set_label(else_1);
    tcg_gen_movi_tl(temp_5, 32);
    tcg_gen_clz_tl(temp_4, psrc, temp_5);
    tcg_gen_mov_tl(temp_3, temp_4);
    tcg_gen_subfi_tl(dest, 31, temp_3);
    gen_set_label(done_1);
    if ((getFFlag () == true)) {
        setZFlag(psrc);
        setNFlag(psrc);
    }
    tcg_temp_free(psrc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_3);

    return ret;
}


/*
 * FFS
 *    Variables: @src, @dest
 *    Functions: CTZ, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   psrc = @src;
 *   if((psrc == 0))
 *     {
 *       @dest = 31;
 *     }
 *   else
 *     {
 *       @dest = CTZ (psrc, 32);
 *     };
 *   if((getFFlag () == true))
 *     {
 *       setZFlag (psrc);
 *       setNFlag (psrc);
 *     };
 * }
 */

int
arc_gen_FFS(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv psrc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    tcg_gen_mov_tl(psrc, src);
    TCGLabel *else_1 = gen_new_label();
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_1, psrc, 0);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_1);
    tcg_gen_movi_tl(dest, 31);
    tcg_gen_br(done_1);
    gen_set_label(else_1);
    tcg_gen_movi_tl(temp_4, 32);
    tcg_gen_ctz_tl(temp_3, psrc, temp_4);
    tcg_gen_mov_tl(dest, temp_3);
    gen_set_label(done_1);
    if ((getFFlag () == true)) {
        setZFlag(psrc);
        setNFlag(psrc);
    }
    tcg_temp_free(psrc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_3);

    return ret;
}

static void
arc_check_dest_reg_is_even_or_null(DisasCtxt *ctx, TCGv reg)
{
  ptrdiff_t n = tcgv_i32_temp(reg) - tcgv_i32_temp(cpu_r[0]);
  if (n >= 0 && n < 64) {
    /* REG is an odd register. */
    if (n % 2 != 0)
      arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
  }
}

static void
arc_gen_next_register_i32_i64(DisasCtxt *ctx,
                              TCGv_i64 dest, TCGv_i32 reg)
{
  ptrdiff_t n = tcgv_i32_temp(reg) - tcgv_i32_temp(cpu_r[0]);
  if (n >= 0 && n < 64) {
    /* Check if REG is an even register. */
    if (n % 2 == 0) {
      if (n == 62) { /* limm */
        tcg_gen_concat_i32_i64(dest, reg, reg);
        tcg_gen_andi_i64(dest, dest, 0xffffffff);
      } else { /* normal register */
        tcg_gen_concat_i32_i64(dest, reg, cpu_r[n + 1]);
      }
    } else { /* if REG is an odd register, thows an exception */
      arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
    }
  } else { /* u6 or s12 */
    tcg_gen_concat_i32_i64(dest, reg, reg);
  }
}

static void
arc_gen_vec_pair_i32(DisasCtxt *ctx,
                     TCGv_i32 dest, TCGv_i32 b, TCGv_i32 c,
                     void (*OP)(TCGv_i64, TCGv_i64, TCGv_i64))
{
  TCGv_i64 t1 = tcg_temp_new_i64();
  TCGv_i64 t2 = tcg_temp_new_i64();
  TCGv_i64 t3 = tcg_temp_new_i64();

  /* check if dest is an even or a null register */
  arc_check_dest_reg_is_even_or_null(ctx, dest);

  /* t2 = [next(b):b] */
  arc_gen_next_register_i32_i64(ctx, t2, b);
  /* t3 = [next(c):c] */
  arc_gen_next_register_i32_i64(ctx, t3, c);

  /* execute the instruction operation */
  OP(t1, t2, t3);

  /* save the result on [next(dest):dest] */
  tcg_gen_extrl_i64_i32(dest, t1);
  tcg_gen_extrh_i64_i32(nextRegWithNull(dest), t1);

  tcg_temp_free_i64(t3);
  tcg_temp_free_i64(t2);
  tcg_temp_free_i64(t1);
}

/*
 * VMAC2H and VMAC2HU
 */

static void
arc_gen_vmac2h_i32(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c,
                   void (*OP)(TCGv, TCGv, unsigned int, unsigned int))
{
  TCGv b_h0, b_h1, c_h0, c_h1;

  arc_check_dest_reg_is_even_or_null(ctx, dest);

  b_h0 = tcg_temp_new();
  b_h1 = tcg_temp_new();
  c_h0 = tcg_temp_new();
  c_h1 = tcg_temp_new();

  OP(b_h0, b, 0, 16);
  OP(c_h0, c, 0, 16);
  OP(b_h1, b, 16, 16);
  OP(c_h1, c, 16, 16);

  tcg_gen_mul_tl(b_h0, b_h0, c_h0);
  tcg_gen_mul_tl(b_h1, b_h1, c_h1);

  tcg_gen_add_tl(cpu_acclo, cpu_acclo, b_h0);
  tcg_gen_add_tl(cpu_acchi, cpu_acchi, b_h1);
  tcg_gen_mov_tl(dest, cpu_acclo);
  tcg_gen_mov_tl(nextRegWithNull(dest), cpu_acchi);

  tcg_temp_free(c_h1);
  tcg_temp_free(c_h0);
  tcg_temp_free(b_h1);
  tcg_temp_free(b_h0);
}

int
arc_gen_VMAC2H(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  arc_gen_vmac2h_i32(ctx, dest, b, c, tcg_gen_sextract_i32);

  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  return DISAS_NEXT;
}

int
arc_gen_VMAC2HU(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  arc_gen_vmac2h_i32(ctx, dest, b, c, tcg_gen_extract_i32);

  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  return DISAS_NEXT;
}

/*
 * VADD: VADD2, VADD2H, VADD4H
 */

#define ARC_GEN_BASE_I64(NAME, OP, SIZE)                                    \
static void                                                                 \
arc_gen_##NAME##_base_i64(DisasCtxt *ctx,                                   \
                        TCGv_i64 dest, TCGv_i64 b, TCGv_i64 c,              \
                        TCGv_i64 acc, bool set_n_flag,                      \
                        ARC_GEN_EXTRACT_BITS_FUNC extract_bits,             \
                        ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64)   \
{                                                                           \
  ARC_GEN_VEC_FIRST_OPERAND(operand_##SIZE, i64, b);                        \
  ARC_GEN_VEC_SECOND_OPERAND(operand_##SIZE, i64, c);                       \
                                                                            \
  OP(dest, b, c);                                                           \
}

ARC_GEN_BASE_I64(vadd2, tcg_gen_vec_add32_i64,    32bit);
ARC_GEN_BASE_I64(vadd4h, tcg_gen_vec_add16_i64,   16bit);
ARC_GEN_BASE_I64(vsub2, tcg_gen_vec_sub32_i64,    32bit);
ARC_GEN_BASE_I64(vsub4h, tcg_gen_vec_sub16_i64,   16bit);



ARC_GEN_32BIT_INTERFACE(VADD2, PAIR, PAIR, PAIR, UNSIGNED, \
                        arc_gen_vadd2_base_i64);

int
arc_gen_VADD2(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  ARC_GEN_SEMFUNC_INIT();

  arc_autogen_base32_VADD2(ctx, dest, b, c);

  ARC_GEN_SEMFUNC_DEINIT();

  return DISAS_NEXT;
}


#define VEC_ADD16_SUB16_I32_W0(NAME, OP)                 \
static void                                              \
arc_gen_vec_##NAME##16_i32_w0(TCGv dest, TCGv b, TCGv c) \
{                                                        \
  TCGv t1 = tcg_temp_new();                              \
                                                         \
  OP(t1, b, c);                                          \
  tcg_gen_deposit_i32(dest, dest, t1, 0, 32);            \
                                                         \
  tcg_temp_free(t1);                                     \
}

VEC_ADD16_SUB16_I32_W0(add, tcg_gen_vec_add16_i32)
VEC_ADD16_SUB16_I32_W0(sub, tcg_gen_vec_sub16_i32)

int
arc_gen_VADD2H(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  ARC_GEN_SEMFUNC_INIT();

  ARC_GEN_VEC_FIRST_OPERAND(operand_16bit, i32, b);
  ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, i32, c);

  arc_gen_vec_add16_i32_w0(dest, b, c);

  ARC_GEN_SEMFUNC_DEINIT();

  return DISAS_NEXT;
}


ARC_GEN_32BIT_INTERFACE(VADD4H, PAIR, PAIR, PAIR, UNSIGNED, \
                        arc_gen_vadd4h_base_i64);

int
arc_gen_VADD4H(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  ARC_GEN_SEMFUNC_INIT();

  arc_autogen_base32_VADD4H(ctx, dest, b, c);

  ARC_GEN_SEMFUNC_DEINIT();

  return DISAS_NEXT;
}

/*
 * VSUB: VSUB2, VSUB2H, VSUB4H
 */

ARC_GEN_32BIT_INTERFACE(VSUB2, PAIR, PAIR, PAIR, UNSIGNED, \
                        arc_gen_vsub2_base_i64);

int
arc_gen_VSUB2(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  ARC_GEN_SEMFUNC_INIT();

  arc_autogen_base32_VSUB2(ctx, dest, b, c);

  ARC_GEN_SEMFUNC_DEINIT();

  return DISAS_NEXT;
}

int
arc_gen_VSUB2H(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  ARC_GEN_SEMFUNC_INIT();

  ARC_GEN_VEC_FIRST_OPERAND(operand_16bit, i32, b);
  ARC_GEN_VEC_SECOND_OPERAND(operand_16bit, i32, c);

  arc_gen_vec_sub16_i32_w0(dest, b, c);

  ARC_GEN_SEMFUNC_DEINIT();

  return DISAS_NEXT;
}


ARC_GEN_32BIT_INTERFACE(VSUB4H, PAIR, PAIR, PAIR, UNSIGNED, \
                        arc_gen_vsub4h_base_i64);

int
arc_gen_VSUB4H(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  ARC_GEN_SEMFUNC_INIT();

  arc_autogen_base32_VSUB4H(ctx, dest, b, c);

  ARC_GEN_SEMFUNC_DEINIT();

  return DISAS_NEXT;
}

/*
 * VADDSUB and VSUBADD operations
 */

static void
arc_gen_cmpl2_i32(TCGv_i32 ret, TCGv_i32 arg1,
                  unsigned int ofs, unsigned int len)
{
    TCGv_i32 t1 = tcg_temp_new_i32();
    TCGv_i32 t2 = tcg_temp_new_i32();

    tcg_gen_mov_i32(t1, arg1);
    tcg_gen_extract_i32(t2, t1, ofs, len);
    tcg_gen_not_i32(t2, t2);
    tcg_gen_addi_i32(t2, t2, 1);
    tcg_gen_deposit_i32(t1, t1, t2, ofs, len);
    tcg_gen_mov_i32(ret, t1);

    tcg_temp_free_i32(t2);
    tcg_temp_free_i32(t1);
}

#define ARC_GEN_CMPL2_H0_I32(RET, ARG1)     arc_gen_cmpl2_i32(RET, ARG1, 0, 16)
#define ARC_GEN_CMPL2_H1_I32(RET, ARG1)     arc_gen_cmpl2_i32(RET, ARG1, 16, 16)

#define VEC_VADDSUB_VSUBADD_OP(NAME, FIELD, OP, TL)             \
static void                                                     \
arc_gen_##NAME##_op(TCGv_##TL dest, TCGv_##TL b, TCGv_##TL c)   \
{                                                               \
    TCGv_##TL t1 = tcg_temp_new_##TL();                         \
                                                                \
    ARC_GEN_CMPL2_##FIELD(t1, c);                               \
    tcg_gen_vec_##OP##_##TL(dest, b, t1);                       \
                                                                \
    tcg_temp_free_##TL(t1);                                     \
}

VEC_VADDSUB_VSUBADD_OP(vaddsub, W1_I64, add32, i64)
VEC_VADDSUB_VSUBADD_OP(vaddsub2h, H1_I32, add16, i32)
VEC_VADDSUB_VSUBADD_OP(vaddsub4h, H1_H3_I64, add16, i64)
VEC_VADDSUB_VSUBADD_OP(vsubadd, W0_I64, add32, i64)
VEC_VADDSUB_VSUBADD_OP(vsubadd2h, H0_I32, add16, i32)
VEC_VADDSUB_VSUBADD_OP(vsubadd4h, H0_H2_I64, add16, i64)

/*
 * VADDSUB: VADDSUB, VADDSUB2H, VADDSUB4H
 */

int
arc_gen_VADDSUB(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  arc_gen_vec_pair_i32(ctx, dest, b, c,
                       arc_gen_vaddsub_op);

  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  return DISAS_NEXT;
}

int
arc_gen_VADDSUB2H(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  arc_gen_vaddsub2h_op(dest, b, c);

  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  return DISAS_NEXT;
}

int
arc_gen_VADDSUB4H(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  arc_gen_vec_pair_i32(ctx, dest, b, c,
                       arc_gen_vaddsub4h_op);

  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  return DISAS_NEXT;
}

/*
 * VSUBADD: VSUBADD, VSUBADD2H, VSUBADD4H
 */

int
arc_gen_VSUBADD(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  arc_gen_vec_pair_i32(ctx, dest, b, c,
                       arc_gen_vsubadd_op);

  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  return DISAS_NEXT;
}

int
arc_gen_VSUBADD2H(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  arc_gen_vsubadd2h_op(dest, b, c);

  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  return DISAS_NEXT;
}

int
arc_gen_VSUBADD4H(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)
{
  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  arc_gen_vec_pair_i32(ctx, dest, b, c,
                       arc_gen_vsubadd4h_op);

  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(QMACH, PAIR, PAIR, PAIR, SIGNED, \
                        arc_gen_qmach_base_i64);

int
arc_gen_QMACH(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_QMACH(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(QMACHU, PAIR, PAIR, PAIR, UNSIGNED, \
                        arc_gen_qmach_base_i64);

int
arc_gen_QMACHU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_QMACHU(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(DMACWH, PAIR, PAIR, NOT_PAIR, SIGNED, \
                        arc_gen_dmacwh_base_i64);

int
arc_gen_DMACWH(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_DMACWH(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(DMACWHU, PAIR, PAIR, NOT_PAIR, UNSIGNED, \
                        arc_gen_dmacwh_base_i64);

int
arc_gen_DMACWHU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_DMACWHU(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(DMACH, NOT_PAIR, NOT_PAIR, NOT_PAIR, SIGNED, \
                        arc_gen_dmach_base_i64);

int
arc_gen_DMACH(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  ARC_GEN_SEMFUNC_INIT();

  arc_autogen_base32_DMACH(ctx, a, b, c);

  ARC_GEN_SEMFUNC_DEINIT();

  return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(DMACHU, NOT_PAIR, NOT_PAIR, NOT_PAIR, UNSIGNED, \
                        arc_gen_dmach_base_i64);

int
arc_gen_DMACHU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  ARC_GEN_SEMFUNC_INIT();

  arc_autogen_base32_DMACHU(ctx, a, b, c);

  ARC_GEN_SEMFUNC_DEINIT();

  return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(DMPYH, NOT_PAIR, NOT_PAIR, NOT_PAIR, SIGNED, \
                        arc_gen_dmpyh_base_i64);

int
arc_gen_DMPYH(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_DMPYH(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(DMPYHU, NOT_PAIR, NOT_PAIR, NOT_PAIR, UNSIGNED, \
                        arc_gen_dmpyh_base_i64);

int
arc_gen_DMPYHU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_DMPYHU(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(QMPYH, PAIR, PAIR, PAIR, SIGNED, \
                        arc_gen_qmpyh_base_i64);

int
arc_gen_QMPYH(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_QMPYH(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(QMPYHU, PAIR, PAIR, PAIR, UNSIGNED, \
                        arc_gen_qmpyh_base_i64);

int
arc_gen_QMPYHU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_QMPYHU(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(DMPYWH, PAIR, PAIR, NOT_PAIR, SIGNED, \
                        arc_gen_dmpywh_base_i64);

int
arc_gen_DMPYWH(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_DMPYWH(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(DMPYWHU, PAIR, PAIR, NOT_PAIR, UNSIGNED, \
                        arc_gen_dmpywh_base_i64);

int
arc_gen_DMPYWHU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_DMPYWHU(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(VMPY2H, PAIR, NOT_PAIR, NOT_PAIR, SIGNED, \
                        arc_gen_vmpy2h_base_i64);

int
arc_gen_VMPY2H(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_VMPY2H(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(VMPY2HU, PAIR, NOT_PAIR, NOT_PAIR, UNSIGNED, \
                        arc_gen_vmpy2h_base_i64);

int
arc_gen_VMPY2HU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_VMPY2HU(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(MPYD, PAIR, NOT_PAIR, NOT_PAIR, SIGNED, \
                        arc_gen_mpyd_base_i64);

int
arc_gen_MPYD(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_MPYD(ctx, a, b, c);


    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_INTERFACE(MPYDU, PAIR, NOT_PAIR, NOT_PAIR, UNSIGNED, \
                        arc_gen_mpyd_base_i64);

int
arc_gen_MPYDU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_MPYDU(ctx, a, b, c);


    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * VMAX2
 * Compare the 32-bit signed vector elements of the input register pairs b and c
 * The maximum value of the two elements is stored in the corresponding
 * output register pair a.
 * This instruction does not update any STATUS32 flags.
 *    Inputs:  [@b, @(b+1)], [@c, @(c+1)]
 *    Outputs: [@a, @(a+1)]
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       a      = signed_max(b, c);
 *       @(a+1) = signed_max(@(b+1), @(c+1));
 *     };
 * }
 */
int
arc_gen_VMAX2(DisasCtxt *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c)
{
    ARC_GEN_SEMFUNC_INIT();

    tcg_gen_smax_i32(nextRegWithNull(a), nextReg(b), nextReg(c));
    tcg_gen_smax_i32(a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * VMIN2
 * Compare the 32-bit signed vector elements of the input register pairs b and c
 * The minimum value of the two elements is stored in the corresponding
 * output register pair a.
 * This instruction does not update any STATUS32 flags.
 *    Inputs:  [@b, @(b+1)], [@c, @(c+1)]
 *    Outputs: [@a, @(a+1)]
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   if((cc_flag == true))
 *     {
 *       a      = signed_min(b, c);
 *       @(a+1) = signed_min(@(b+1), @(c+1));
 *     };
 * }
 */
int
arc_gen_VMIN2(DisasCtxt *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c)
{
    ARC_GEN_SEMFUNC_INIT();

    tcg_gen_smin_i32(nextRegWithNull(a), nextReg(b), nextReg(c));
    tcg_gen_smin_i32(a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * ATLD
 *    Variables: @b, @c
 *    Functions: arc_gen_atld_op
 * --- code ---
 * {
 *   arc_gen_atld_op(b, c)
 * }
 */
int
arc_gen_ATLD(DisasCtxt *ctx, TCGv b, TCGv c)
{
    arc_gen_atld_op(ctx, b, c);
    return DISAS_NEXT;
}

/* Floating point instructions */

/*
 * FCVT32
 * switch(C) {
 *    : A = (int)   B;
 *    : A = (float) B;
 *    : A = (uint)  B;
 * }
 */
int
arc_gen_FCVT32(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    gen_helper_fcvt32(a, cpu_env, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * FCVT32_64
 * switch(C) {
 *    : A = (long)   B;
 *    : A = (ulong)  B;
 *    : A = (double) B;
 * }
 */
int
arc_gen_FCVT32_64(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    TCGv_i64 r64_a = tcg_temp_new_i64();

    gen_helper_fcvt32_64(r64_a, cpu_env, b, c);
    tcg_gen_extr_i64_i32(a, nextRegWithNull(a), r64_a);

    tcg_temp_free_i64(r64_a);


    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * FCVT64
 * switch(C) {
 *    : A = (long)   B;
 *    : A = (ulong)  B;
 *    : A = (double) B;
 * }
 */
int
arc_gen_FCVT64(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    TCGv_i64 r64_a = tcg_temp_new_i64();
    TCGv_i64 r64_b = tcg_temp_new_i64();

    arc_gen_next_register_i32_i64(ctx, r64_b, b);
    gen_helper_fcvt64(r64_a, cpu_env, r64_b, c);
    tcg_gen_extr_i64_i32(a, nextRegWithNull(a), r64_a);

    tcg_temp_free_i64(r64_a);
    tcg_temp_free_i64(r64_b);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * FCVT64_32
 * switch(C) {
 *    : A = (int)   B;
 *    : A = (uint)  B;
 *    : A = (float) B;
 * }
 */
int
arc_gen_FCVT64_32(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    TCGv_i64 r64_b = tcg_temp_new_i64();
    TCGv_i64 r64_c = tcg_temp_new_i64();

    arc_gen_next_register_i32_i64(ctx, r64_b, b);
    gen_helper_fcvt64_32(a, cpu_env, r64_b, c);

    tcg_temp_free_i64(r64_b);
    tcg_temp_free_i64(r64_c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * FSADD
 * if (cc) {
 *    a = (float) (b + c);
 * }
 */
int arc_gen_FSADD(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    gen_helper_fsadd32(a, cpu_env, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_FLOAT_INTERFACE3(FDADD, PAIR, PAIR, PAIR, gen_helper_fdadd);
/*
 * FDADD
 * if (cc) {
 *    a = (double) (b + c);
 * }
 */
int arc_gen_FDADD(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_FDADD(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}


/*
 * FSSUB
 * if (cc) {
 *    a = (float) (b - c);
 * }
 */
int arc_gen_FSSUB(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    gen_helper_fssub32(a, cpu_env, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_FLOAT_INTERFACE3(FDSUB, PAIR, PAIR, PAIR, gen_helper_fdsub);
/*
 * FDSUB
 * if (cc) {
 *    a = (double) (b - c);
 * }
 */
int arc_gen_FDSUB(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_FDSUB(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * FSDIV
 * if (cc) {
 *    a = (float) (b / c);
 * }
 */
int arc_gen_FSDIV(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    gen_helper_fsdiv32(a, cpu_env, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_FLOAT_INTERFACE3(FDDIV, PAIR, PAIR, PAIR, gen_helper_fddiv);
/*
 * FDDIV
 * if (cc) {
 *    a = (double) (b / c);
 * }
 */
int arc_gen_FDDIV(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_FDDIV(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * FSMUL
 * if (cc) {
 *    a = (float) (b * c);
 * }
 */
int arc_gen_FSMUL(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    gen_helper_fsmul32(a, cpu_env, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_FLOAT_INTERFACE3(FDMUL, PAIR, PAIR, PAIR, gen_helper_fdmul);
/*
 * FDMUL
 * if (cc) {
 *    a = (double) (b * c);
 * }
 */
int arc_gen_FDMUL(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_FDMUL(ctx, a, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * FSSQRT
 * if (cc) {
 *    a = (float) sqrt(b);
 * }
 */
int arc_gen_FSSQRT(DisasCtxt *ctx, TCGv a, TCGv b)
{
    ARC_GEN_SEMFUNC_INIT();

    gen_helper_fssqrt32(a, cpu_env, b);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * FDSQRT
 * if (cc) {
 *    a = (double) sqrt(b);
 * }
 */
int arc_gen_FDSQRT(DisasCtxt *ctx, TCGv a, TCGv b)
{
    ARC_GEN_SEMFUNC_INIT();

    TCGv_i64 r64_a = tcg_temp_new_i64();
    TCGv_i64 r64_b = tcg_temp_new_i64();
    arc_gen_next_register_i32_i64(ctx, r64_b, b);

    gen_helper_fdsqrt(r64_a, cpu_env, r64_b);
    tcg_gen_extr_i64_i32(a, nextRegWithNull(a), r64_a);

    tcg_temp_free_i64(r64_a);
    tcg_temp_free_i64(r64_b);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * FSMADDv2
 * if (cc) {
 *    a = (float) (acc + (b * c));
 * }
 */
int arc_gen_FSMADDv2(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    TCGv_i64 r64_a = tcg_temp_new_i64();
    TCGv_i64 r64_b = tcg_temp_new_i64();
    TCGv_i64 r64_c = tcg_temp_new_i64();
    TCGv_i64 acc = tcg_temp_new_i64();

    tcg_gen_extu_i32_i64(r64_b, b);
    tcg_gen_extu_i32_i64(r64_c, c);
    tcg_gen_extu_i32_i64(acc, cpu_acclo);

    gen_helper_fsmadd(r64_a, cpu_env, r64_b, r64_c, acc);

    tcg_gen_extrl_i64_i32(a, r64_a);
    /* Unlike v3 accumulator usage, v2 does not */

    tcg_temp_free_i64(r64_a);
    tcg_temp_free_i64(r64_b);
    tcg_temp_free_i64(r64_c);
    tcg_temp_free_i64(acc);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * FDMADDv2
 * if (cc) {
 *    a = (double) (acc + (b * c));
 * }
 */
int arc_gen_FDMADDv2(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    TCGv_i64 r64_a = tcg_temp_new_i64();
    TCGv_i64 r64_b = tcg_temp_new_i64();
    TCGv_i64 r64_c = tcg_temp_new_i64();
    TCGv_i64 acc = tcg_temp_new_i64();

    arc_gen_next_register_i32_i64(ctx, r64_b, b);
    arc_gen_next_register_i32_i64(ctx, r64_c, c);
    tcg_gen_concat_i32_i64(acc, cpu_acclo, cpu_acchi);

    gen_helper_fdmadd(r64_a, cpu_env, r64_b, r64_c, acc);

    tcg_gen_extr_i64_i32(a, nextRegWithNull(a), r64_a);

    tcg_temp_free_i64(r64_a);
    tcg_temp_free_i64(r64_b);
    tcg_temp_free_i64(r64_c);
    tcg_temp_free_i64(acc);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * FSMSUBv2
 * if (cc) {
 *    a = (float) (acc - (b * c));
 * }
 */
int arc_gen_FSMSUBv2(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    TCGv_i64 r64_a = tcg_temp_new_i64();
    TCGv_i64 r64_b = tcg_temp_new_i64();
    TCGv_i64 r64_c = tcg_temp_new_i64();
    TCGv_i64 acc = tcg_temp_new_i64();

    tcg_gen_extu_i32_i64(r64_b, b);
    tcg_gen_extu_i32_i64(r64_c, c);
    tcg_gen_extu_i32_i64(acc, cpu_acclo);

    gen_helper_fsmsub(r64_a, cpu_env, r64_b, r64_c, acc);

    tcg_gen_extrl_i64_i32(a, r64_a);

    tcg_temp_free_i64(r64_a);
    tcg_temp_free_i64(r64_b);
    tcg_temp_free_i64(r64_c);
    tcg_temp_free_i64(acc);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * FDMSUBv2
 * if (cc) {
 *    a = (double) (acc - (b * c));
 * }
 */
int arc_gen_FDMSUBv2(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    TCGv_i64 r64_a = tcg_temp_new_i64();
    TCGv_i64 r64_b = tcg_temp_new_i64();
    TCGv_i64 r64_c = tcg_temp_new_i64();
    TCGv_i64 acc = tcg_temp_new_i64();

    arc_gen_next_register_i32_i64(ctx, r64_b, b);
    arc_gen_next_register_i32_i64(ctx, r64_c, c);
    tcg_gen_concat_i32_i64(acc, cpu_acclo, cpu_acchi);

    gen_helper_fdmsub(r64_a, cpu_env, r64_b, r64_c, acc);

    tcg_gen_extr_i64_i32(a, nextRegWithNull(a), r64_a);

    tcg_temp_free_i64(r64_a);
    tcg_temp_free_i64(r64_b);
    tcg_temp_free_i64(r64_c);
    tcg_temp_free_i64(acc);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_FLOAT_CMP_INTERFACE2(fscmp, NOT_PAIR, NOT_PAIR)

/*
 * FSCMP
 * if (cc) {
 *    compare(b, c)
 * }
 */
int
arc_gen_FSCMP(DisasCtxt *ctx, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_fscmp(ctx, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_FLOAT_CMP_INTERFACE2(fscmpf, NOT_PAIR, NOT_PAIR)
/*
 * FSCMPF
 * if (cc) {
 *    compare_ieee754_flag(b, c)
 * }
 */
int
arc_gen_FSCMPF(DisasCtxt *ctx, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_fscmpf(ctx, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_FLOAT_CMP_INTERFACE2(fdcmp, PAIR, PAIR)
/*
 * FDCMP
 * if (cc) {
 *    compare(b, c)
 * }
 */
int
arc_gen_FDCMP(DisasCtxt *ctx, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_fdcmp(ctx, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

ARC_GEN_32BIT_FLOAT_CMP_INTERFACE2(fdcmpf, PAIR, PAIR)
/*
 * FDCMPF
 * if (cc) {
 *    compare_ieee754_flag(b, c)
 * }
 */
int
arc_gen_FDCMPF(DisasCtxt *ctx, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_autogen_base32_fdcmpf(ctx, b, c);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}
