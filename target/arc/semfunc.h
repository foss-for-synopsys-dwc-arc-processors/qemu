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

#ifndef __ARC_SEMFUNC_H__
#define __ARC_SEMFUNC_H__

#include "translate.h"
#include "semfunc-helper.h"

/* TODO (issue #62): these must be removed */
#define arc_false   (ctx->zero)
#define arc_true    (ctx->one)

#define LONG 0
#define BYTE 1
#define WORD 2
#define LONGLONG 3

int __not_implemented_semfunc_0(DisasCtxt *ctx);
int __not_implemented_semfunc_1(DisasCtxt *ctx, TCGv a);
int __not_implemented_semfunc_2(DisasCtxt *ctx, TCGv a, TCGv b);
int __not_implemented_semfunc_3(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c);
int __not_implemented_semfunc_4(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c, TCGv d);

#define SEMANTIC_FUNCTION_PROTOTYPE_0(NAME) \
    int arc_gen_##NAME(DisasCtxt *);
#define SEMANTIC_FUNCTION_PROTOTYPE_1(NAME) \
    int arc_gen_##NAME(DisasCtxt *, TCGv);
#define SEMANTIC_FUNCTION_PROTOTYPE_2(NAME) \
    int arc_gen_##NAME(DisasCtxt *, TCGv, TCGv);
#define SEMANTIC_FUNCTION_PROTOTYPE_3(NAME) \
    int arc_gen_##NAME(DisasCtxt *, TCGv, TCGv, TCGv);
#define SEMANTIC_FUNCTION_PROTOTYPE_4(NAME) \
    int arc_gen_##NAME(DisasCtxt *, TCGv, TCGv, TCGv, TCGv);

#define MAPPING(MNEMONIC, NAME, NOPS, ...)
#define CONSTANT(...)
#define SEMANTIC_FUNCTION(NAME, NOPS) \
  SEMANTIC_FUNCTION_PROTOTYPE_##NOPS(NAME)

#include "target/arc/semfunc-mapping.def"

#undef MAPPING
#undef CONSTANT
#undef SEMANTIC_FUNCTION_PROTOTYPE_0
#undef SEMANTIC_FUNCTION_PROTOTYPE_1
#undef SEMANTIC_FUNCTION_PROTOTYPE_2
#undef SEMANTIC_FUNCTION_PROTOTYPE_3
#undef SEMANTIC_FUNCTION_PROTOTYPE_4
#undef SEMANTIC_FUNCTION

void arc_gen_vec_add16_w0_i64(TCGv_i64 d, TCGv_i64 a, TCGv_i64 b);

void arc_gen_cmpl2_i64(TCGv_i64 ret, TCGv_i64 arg1,
                       unsigned int ofs, unsigned int len);

/*
 * @brief Verifies if a signed subtraction resulted in an overflow
 * @param overflow Set in case of overflow
 * @param result The result of the subtraction
 * @param op1 Operand of the subtraction
 * @param op2 Operand of the subtraction
 * @param operator_size Size of the subtraction operands
 */
void
arc_gen_sub_signed_overflow_tl(TCGv overflow, TCGv result,
                               TCGv op1, TCGv op2, uint8_t operand_size);

/*
 * @brief Verifies if an signed add resulted in an overflow. Only works with
 * numbers smaller than the architecture size.
 * @param overflow Set in case of overflow
 * @param result The result of the addition
 * @param op1 Operand of the add
 * @param op2 Operand of the add
 * @param operator_size Size of the add operands
 */
void arc_gen_add_signed_overflow_tl(TCGv overflow, TCGv result,
                                    TCGv op1, TCGv op2, uint8_t operator_size);

/*
 * @brief Verifies if a 64 bit signed add resulted in an overflow
 * This function is necessary because the _tl version cannot handle 64 bit
 * computations in a 32 bit architecture
 * @param overflow Set in case of overflow
 * @param result The result of the addition
 * @param op1 Operand of the add
 * @param op2 Operand of the add
 */
void arc_gen_add_signed_overflow_i64(TCGv_i64 overflow, TCGv_i64 result,
                                     TCGv_i64 op1, TCGv_i64 op2);

/*
 * @brief Verifies if a 32 bit signed add generated a carry for 64 bit
 * architectures
 * @param overflow Set in case of carry
 * @param result The result of the addition
 * @param op1 Operand of the add
 * @param op2 Operand of the add
 */
void
arc_gen_add_signed_carry32_i64(TCGv_i64 carry, TCGv_i64 result,
                               TCGv_i64 op1, TCGv_i64 op2);

/*
 * @brief Verifies if a signed add generated a carry
 * @param overflow Set in case of carry
 * @param result The result of the addition
 * @param op1 Operand of the add
 * @param op2 Operand of the add
 */
void
arc_gen_add_signed_carry_tl(TCGv carry, TCGv result,
                            TCGv op1, TCGv op2);

/**
 * @brief Verifies if a 64 bit unsigned add resulted in an overflow
 * @param overflow Is set to 1 or 0 on no overflow, or overflow, respectively
 * @param result The result of the addition
 * @param op1 Operand of the add
 * @param op2 Operand of the add
 */
void arc_gen_add_unsigned_overflow_i64(TCGv_i64 overflow, TCGv_i64 result,
                                       TCGv_i64 op1, TCGv_i64 op2);

/**
 * Some macro definitions for using function pointers as arguments
 */
typedef void (*ARC_GEN_OVERFLOW_DETECT_FUNC)(TCGv_i64, TCGv_i64, \
                                             TCGv_i64, TCGv_i64);

typedef void (*ARC_GEN_EXTRACT_BITS_FUNC)(TCGv_i64, TCGv_i64, \
                                          unsigned int, unsigned int);

typedef void (*ARC_GEN_SPECIFIC_OPERATION_FUNC)(DisasCtxt*, TCGv_i64, TCGv_i64,\
                                                TCGv_i64, TCGv_i64, bool,      \
                                                ARC_GEN_EXTRACT_BITS_FUNC,     \
                                                ARC_GEN_OVERFLOW_DETECT_FUNC);

/**
 * @brief Runs the "detect_overflow_i64" function with res, op1 and op2 as
 * arguments, and ors the resulting overflow with the provided one.
 * This function effectively only sets the received overflow variable from
 * 0 to 1 and never the other way around
 * @param res The result to be passed to the overflow function
 * @param operand_1 The first operand to be passed to the overflow function
 * @param operand_2 The second operand to be passed to the overflow function
 * @param overflow The current overflow value that at most will change to 1
 * @param detect_overflow_i64 The function to call to detect overflow
 */
void arc_gen_set_if_overflow(TCGv_i64 res, TCGv_i64 operand_1,
                             TCGv_i64 operand_2, TCGv_i64 overflow,
                             ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64);

/**
 * @brief Analyzes operand flags and sets vector constant values accordingly
 * Operations:
 *  - Set limm as operand value
 * This should eventually be done by the decoder
 * @param ctx Current instruction context
 * @param operand Operand to analyze
 */
void
arc_gen_set_operand_64bit_vec_const(DisasCtxt *ctx, operand_t *operand);

/**
 * @brief Analyzes operand flags and sets vector constant values accordingly
 * Operations:
 *  - 32 bit duplication for limm
 *  - 32 bit duplication for s12 and u6
 * This function assumes "non limm" can only be u6 or s12, and thus should only
 * be used by functions aware of that fact and handle registers themselves
 * @param ctx Current instruction context
 * @param operand Operand to analyze
 */
void
arc_gen_set_operand_32bit_vec_const(DisasCtxt *ctx, operand_t *operand);

/**
 * @brief Analyzes operand flags and sets vector constant values accordingly
 * Operations:
 *  - 32 bit duplication for limm
 *  - 16 bit quadriplication for s12 and u6
 * This function assumes "non limm" can only be u6 or s12, and thus should only
 * be used by functions aware of that fact and handle registers themselves
 * @param ctx Current instruction context
 * @param operand Operand to analyze
 */
void
arc_gen_set_operand_16bit_vec_const(DisasCtxt *ctx, operand_t *operand);

/*
 * Generate code to handle the first operand (TCG_OPERAND) in a vector
 * operation to quadriplicate/duplicatie its' value based on the provided vector
 * operand size (OP_SIZE)
 */
#define ARC_GEN_VEC_FIRST_OPERAND(OP_SIZE, TCG_OPERAND)                 \
    do {                                                                \
        operand_t *operand_1 = &(ctx->insn.operands[1]);                \
        if (!(operand_1->type & ARC_OPERAND_IR)) {                      \
            arc_gen_set_ ## OP_SIZE ## _vec_const(ctx, operand_1);      \
            tcg_gen_movi_i64(TCG_OPERAND, operand_1->value);            \
        }                                                               \
    } while(0)

/*
 * Generate code to handle the second operand (TCG_OPERAND) in a vector
 * operation to quadriplicate/duplicatie its' value based on the provided vector
 * operand size (OP_SIZE).
 */
#define ARC_GEN_VEC_SECOND_OPERAND(OP_SIZE, TCG_OPERAND)                \
    do {                                                                \
        operand_t *operand_2 = &(ctx->insn.operands[2]);                \
        if (!(operand_2->type & ARC_OPERAND_IR)) {                      \
            if (operand_2->type & ARC_OPERAND_LIMM &&                   \
                operand_2->type & ARC_OPERAND_DUPLICATE){               \
                operand_2->value = ctx->insn.operands[1].value;         \
            } else {                                                    \
                arc_gen_set_ ## OP_SIZE ## _vec_const(ctx, operand_2);  \
            }                                                           \
            tcg_gen_movi_i64(TCG_OPERAND, operand_2->value);            \
        }                                                               \
    } while(0)

/**
 * @brief Base for the 64 bit QMACH operation.
 * @param ctx Current instruction context
 * @param a First (dest) operand of the instruction
 * @param b Second operand of the instruction
 * @param c Third operand of the instruction
 * @param acc The current accumulator value
 * @param set_n_flag Whether to set the N flag or not
 * @param extract_bits The function to be used to extract the bits
 * @param detect_overflow The function to use to detect 64 bit overflow
 */
void arc_gen_qmach_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                            TCGv_i64 acc, bool set_n_flag,
                            ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                            ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow);

/**
 * @brief Base for the 64 bit QMPYH operation.
 * @param ctx Current instruction context
 * @param a First (dest) operand of the instruction
 * @param b Second operand of the instruction
 * @param c Third operand of the instruction
 * @param acc The current accumulator value
 * @param set_n_flag Whether to set the N flag or not
 * @param extract_bits The function to be used to extract the bits
 * @param detect_overflow The function to use to detect 64 bit overflow
 */
void arc_gen_qmpyh_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                       TCGv_i64 acc, bool set_n_flag,
                       ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                       ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64);

/**
 * @brief Base for the 64 bit DMACWH operation.
 * @param ctx Current instruction context
 * @param a First (dest) operand of the instruction
 * @param b Second operand of the instruction
 * @param c Third operand of the instruction
 * @param acc The current accumulator value
 * @param set_n_flag Whether to set the N flag or not
 * @param extract_bits The function to be used to extract the bits
 * @param detect_overflow The function to use to detect 64 bit overflow
 */
void arc_gen_dmacwh_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                             TCGv_i64 acc, bool set_n_flag,
                             ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                             ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64);

/**
 * @brief Base for the 64 bit DMACH operation.
 * @param ctx Current instruction context
 * @param a First (dest) operand of the instruction
 * @param b Second operand of the instruction
 * @param c Third operand of the instruction
 * @param acc The current accumulator value
 * @param set_n_flag Whether to set the N flag or not
 * @param extract_bits The function to be used to extract the bits
 * @param detect_overflow The function to use to detect 64 bit overflow
 */
void arc_gen_dmach_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                            TCGv_i64 acc, bool set_n_flag,
                            ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                            ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64);

/**
 * @brief Base for the 64 bit DMPYH operation.
 * @param ctx Current instruction context
 * @param a First (dest) operand of the instruction
 * @param b Second operand of the instruction
 * @param c Third operand of the instruction
 * @param acc The current accumulator value
 * @param set_n_flag Whether to set the N flag or not
 * @param extract_bits The function to be used to extract the bits
 * @param detect_overflow The function to use to detect 64 bit overflow
 */
void
arc_gen_dmpyh_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                        TCGv_i64 acc, bool set_n_flag,
                        ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                        ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64);

/**
 * @brief Base for the 64 bit DMPYWH operation.
 * @param ctx Current instruction context
 * @param a First (dest) operand of the instruction
 * @param b Second operand of the instruction
 * @param c Third operand of the instruction
 * @param acc The current accumulator value
 * @param set_n_flag Whether to set the N flag or not
 * @param extract_bits The function to be used to extract the bits
 * @param detect_overflow The function to use to detect 64 bit overflow
 */
void
arc_gen_dmpywh_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                        TCGv_i64 acc, bool set_n_flag,
                        ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                        ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64);

/**
 * @brief Base for the 64 bit VMPY2H operation.
 * @param ctx Current instruction context
 * @param a First (dest) operand of the instruction
 * @param b Second operand of the instruction
 * @param c Third operand of the instruction
 * @param acc The current accumulator value
 * @param set_n_flag Whether to set the N flag or not
 * @param extract_bits The function to be used to extract the bits
 * @param detect_overflow The function to use to detect 64 bit overflow
 */
void
arc_gen_vmpy2h_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                        TCGv_i64 acc, bool set_n_flag,
                        ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                        ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64);

/**
 * @brief Base for the 64 bit MPYD operation.
 * @param ctx Current instruction context
 * @param a First (dest) operand of the instruction
 * @param b Second operand of the instruction
 * @param c Third operand of the instruction
 * @param acc The current accumulator value
 * @param set_n_flag Whether to set the N flag or not
 * @param extract_bits The function to be used to extract the bits
 * @param detect_overflow The function to use to detect 64 bit overflow
 */
void
arc_gen_mpyd_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                        TCGv_i64 acc, bool set_n_flag,
                        ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                        ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64);

/**
 * @brief Prologue for the CC flag handling and jump
 */
#define ARC_GEN_CC_PROLOGUE()                                           \
    TCGv cc_temp;                                                       \
    TCGLabel *cc_done;                                                  \
    if (ctx->insn.cc != ARC_COND_AL && ctx->insn.cc != ARC_COND_RA) {   \
        cc_temp = tcg_temp_local_new();                                 \
        cc_done = gen_new_label();                                      \
        getCCFlag(cc_temp);                                             \
        tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);           \
    }

/**
 * @brief Epilogue for the CC flag handling and jump
 */
#define ARC_GEN_CC_EPILOGUE()                                           \
    if (ctx->insn.cc != ARC_COND_AL && ctx->insn.cc != ARC_COND_RA) {   \
        gen_set_label(cc_done);                                         \
        tcg_temp_free(cc_temp);                                         \
    }

#define ARC_GEN_CMPL2_H0_I64(RET, ARG1)     arc_gen_cmpl2_i64(RET, ARG1, 0, 16)
#define ARC_GEN_CMPL2_H1_I64(RET, ARG1)     arc_gen_cmpl2_i64(RET, ARG1, 16, 16)
#define ARC_GEN_CMPL2_H2_I64(RET, ARG1)     arc_gen_cmpl2_i64(RET, ARG1, 32, 16)
#define ARC_GEN_CMPL2_H3_I64(RET, ARG1)     arc_gen_cmpl2_i64(RET, ARG1, 48, 16)
#define ARC_GEN_CMPL2_H0_H2_I64(RET, ARG1)  \
    ARC_GEN_CMPL2_H0_I64(RET, ARG1);        \
    ARC_GEN_CMPL2_H2_I64(RET, RET)
#define ARC_GEN_CMPL2_H1_H3_I64(RET, ARG1)  \
    ARC_GEN_CMPL2_H1_I64(RET, ARG1);        \
    ARC_GEN_CMPL2_H3_I64(RET, RET)
#define ARC_GEN_CMPL2_W0_I64(RET, ARG1)     arc_gen_cmpl2_i64(RET, ARG1, 0, 32)
#define ARC_GEN_CMPL2_W1_I64(RET, ARG1)     arc_gen_cmpl2_i64(RET, ARG1, 32, 32)

#endif /* __ARC_SEMFUNC_H__ */
