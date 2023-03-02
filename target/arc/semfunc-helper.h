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

#ifndef SEMFUNC_HELPER_H_
#define SEMFUNC_HELPER_H_

#include "translate.h"
#include "qemu/bitops.h"
#include "tcg/tcg.h"
#include "target/arc/regs.h"

void arc_gen_mac(TCGv phi, TCGv b, TCGv c);
void arc_gen_macu(TCGv phi, TCGv b, TCGv c);

void arc_gen_extract_bits(TCGv ret, TCGv a, TCGv start, TCGv end);
void arc_gen_get_bit(TCGv ret, TCGv a, TCGv pos);

void tcg_gen_shlfi_tl(TCGv a, int b, TCGv c);

#define getBit(R, A, POS)   arc_gen_get_bit(R, A, POS)
#define getBiti(R, A, POS)   arc_gen_get_bit(R, A, tcg_constant_tl(POS))

#define ReplMask(DEST, SRC, MASK) \
    gen_helper_repl_mask(DEST, DEST, SRC, MASK)

#define ReplMaski(DEST, SRC, MASK) \
    gen_helper_repl_mask(DEST, DEST, SRC, tcg_constant_i64(MASK))

#define SignExtend(VALUE, SIZE) VALUE

/*       Flag calculation operations       */
#ifdef TARGET_ARC64

#define Carry32(R, A) \
                                  tcg_gen_shri_tl(R, A, 31); \
                                  tcg_gen_andi_tl(R, R, 0x1);

#define OverflowADD32(R, A, B, C) arc_gen_add_signed_overflow_tl(R, A, B, C, 32)

#define CarryADD32(R, A, B, C)    arc_gen_add_signed_carry32_i64(R, A, B, C)

#define CarrySUB32(R, A, B, C)    arc_gen_sub_signed_carry_tl(R, A, B, C, 32)

#define OverflowSUB32(R, A, B, C) arc_gen_sub_signed_overflow_tl(R, A, B, C, 32)

#endif

#define Carry(R, A)               tcg_gen_shri_tl(R, A, TARGET_LONG_BITS - 1);

#define OverflowADD(R, A, B, C)   arc_gen_add_signed_overflow_tl(R, A, B, C, TARGET_LONG_BITS);

#define CarryADD(R, A, B, C)    arc_gen_add_signed_carry_tl(R, A, B, C)

#define CarrySUB(R, A, B, C)    arc_gen_sub_signed_carry_tl(R, A, B, C, TARGET_LONG_BITS); \
                                tcg_gen_setcondi_tl(TCG_COND_NE, R, R, 0)
#define OverflowSUB(R, A, B, C) arc_gen_sub_signed_overflow_tl(R, A, B, C, TARGET_LONG_BITS)

/*       Flag operations       */
#define setCFlag(ELEM)  tcg_gen_andi_tl(cpu_Cf, ELEM, 1)
#define getCFlag(R)     tcg_gen_mov_tl(R, cpu_Cf)
#define clearCFlag()    tcg_gen_movi_tl(cpu_Cf, 0)

#define setVFlag(ELEM)  tcg_gen_andi_tl(cpu_Vf, ELEM, 1)
#define setVFlagTo1()   tcg_gen_movi_tl(cpu_Vf, 1)
#define clearVFlag()    tcg_gen_movi_tl(cpu_Vf, 0)

#define getNFlag(R)     cpu_Nf
#define setNFlag(ELEM)  tcg_gen_shri_tl(cpu_Nf, ELEM, (TARGET_LONG_BITS - 1))
#define clearNFlag()    tcg_gen_movi_tl(cpu_Cf, 0)
#ifdef TARGET_ARC64
#define setNFlag32(ELEM)  tcg_gen_shri_tl(cpu_Nf, ELEM, 31)
#endif
/* Sets N flag based on received number */
#define setNFlagByNum(ELEM, N) { \
    TCGv _tmp = tcg_temp_local_new(); \
    tcg_gen_shri_tl(_tmp, ELEM, (N - 1)); \
    tcg_gen_andi_tl(cpu_Nf, _tmp, 1); \
    tcg_temp_free(_tmp); \
}

#define setZFlag(ELEM) tcg_gen_setcondi_tl(TCG_COND_EQ, cpu_Zf, ELEM, 0);
/* Sets Z flag based on received number */
#define setZFlagByNum(ELEM, N) { \
    TCGv _tmp = tcg_temp_local_new(); \
    tcg_gen_andi_tl(_tmp, cpu_Zf, (1 << N) - 1); \
    tcg_gen_setcondi_tl(TCG_COND_EQ, _tmp, ELEM, 0); \
    tcg_temp_free(_tmp); \
}

#define setLF(VALUE)    tcg_gen_mov_tl(cpu_lock_lf_var, VALUE)
#define getLF(R)        tcg_gen_mov_tl(R, cpu_lock_lf_var)

#define DO_IN_32BIT_SIGNED(OP, R, SRC1, SRC2) \
    TCGv_i32 lr = tcg_temp_new_i32(); \
    TCGv_i32 lsrc1 = tcg_temp_new_i32(); \
    TCGv_i32 lsrc2 = tcg_temp_new_i32(); \
    tcg_gen_trunc_tl_i32(lsrc1, SRC1); \
    tcg_gen_trunc_tl_i32(lsrc2, SRC2); \
    OP(lr, lsrc1, lsrc2); \
    tcg_gen_ext_i32_tl(R, lr); \
    tcg_temp_free_i32(lr); \
    tcg_temp_free_i32(lsrc1); \
    tcg_temp_free_i32(lsrc2);

#define DO_IN_32BIT_UNSIGNED(OP, R, SRC1, SRC2) \
    TCGv_i32 lr = tcg_temp_new_i32(); \
    TCGv_i32 lsrc1 = tcg_temp_new_i32(); \
    TCGv_i32 lsrc2 = tcg_temp_new_i32(); \
    tcg_gen_trunc_tl_i32(lsrc1, SRC1); \
    tcg_gen_trunc_tl_i32(lsrc2, SRC2); \
    OP(lr, lsrc1, lsrc2); \
    tcg_gen_extu_i32_tl(R, lr); \
    tcg_temp_free_i32(lr); \
    tcg_temp_free_i32(lsrc1); \
    tcg_temp_free_i32(lsrc2);

#ifdef TARGET_ARC64


#define ARC64_ADDRESS_ADD(A, B, C) { \
  ARCCPU *cpu = env_archcpu(ctx->env); \
  if((cpu->family & ARC_OPCODE_ARC64) != 0) { \
    tcg_gen_add_tl(A, B, C); \
  } else if((cpu->family & ARC_OPCODE_ARC32) != 0) { \
    /* TODO: TO REMOVE */ \
    assert(0); \
    TCGv_i32 tA, tB, tC; \
    tA = tcg_temp_new_i32(); \
    tB = tcg_temp_new_i32(); \
    tC = tcg_temp_new_i32(); \
    \
    tcg_gen_extrl_i64_i32(tB, B); \
    tcg_gen_extrl_i64_i32(tC, C); \
    tcg_gen_add_i32(tA, tB, tC); \
    tcg_gen_extu_i32_i64(A, tA); \
    \
    tcg_temp_free_i32(tA); \
    tcg_temp_free_i32(tB); \
    tcg_temp_free_i32(tC); \
  } else { \
    assert(0); \
  } \
}

#endif

#endif /* SEMFUNC_HELPER_H_ */


/*-*-indent-tabs-mode:nil;tab-width:4;indent-line-function:'insert-tab'-*-*/
/* vim: set ts=4 sw=4 et: */
