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

/*-*-indent-tabs-mode:nil;tab-width:4;indent-line-function:'insert-tab'-*-*/
/* vim: set ts=4 sw=4 et: */
