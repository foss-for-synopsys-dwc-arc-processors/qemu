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
