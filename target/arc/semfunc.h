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

void arc_gen_add_signed_overflow_i64(TCGv_i64 overflow, TCGv_i64 result,
                                     TCGv_i64 op1, TCGv_i64 op2);

void arc_gen_add_unsigned_overflow_i64(TCGv_i64 overflow, TCGv_i64 result,
                                       TCGv_i64 op1, TCGv_i64 op2);

typedef void (*ARC_GEN_EXTRACT_BITS_FUNC)(TCGv_i64, TCGv_i64, unsigned int, unsigned int);
typedef void (*ARC_GEN_OVERFLOW_DETECT_FUNC)(TCGv_i64, TCGv_i64, TCGv_i64, TCGv_i64);

void arc_gen_set_if_overflow(TCGv_i64 res, TCGv_i64 op1, TCGv_i64 op2,
                             TCGv_i64 overflow,
                             ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow_i64);

void arc_gen_qmach_base_i64(DisasCtxt *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                       TCGv_i64 acc, TCGv_i64 overflow,
                       ARC_GEN_EXTRACT_BITS_FUNC extract_bits,
                       ARC_GEN_OVERFLOW_DETECT_FUNC detect_overflow);


#define ARC_GEN_QMACHU_I64(ctx, a, b, c, acc, overflow) arc_gen_qmach_base_i64(ctx, a, b, c, acc, overflow, tcg_gen_extract_i64, arc_gen_add_unsigned_overflow_i64)

#define ARC_GEN_QMACH_I64(ctx, a, b, c, acc, overflow) arc_gen_qmach_base_i64(ctx, a, b, c, acc, overflow, tcg_gen_sextract_i64, arc_gen_add_signed_overflow_i64)

void
arc_gen_set_vector_constant_operand(DisasCtxt *ctx, TCGv_i64 tcg_operand, operand_t* operand);


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
