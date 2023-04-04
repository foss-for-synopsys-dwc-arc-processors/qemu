/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2021 Synopsys Inc.
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
#include "cpu.h"
#include "exec/exec-all.h"
#include "exec/helper-proto.h"
#include "fpu/softfloat.h"
#include "fpu.h"
#include "qemu/log.h"

uint8_t fpr_width;
/* Width of vector floating point operations */
uint8_t vfp_width;

/* Turn the internal soft fpu flags into an ARC FPU status value */
static uint32_t arc_pack_fpu_status(CPUARCState *env)
{
    uint32_t r = 0;
    int e = get_float_exception_flags(&env->fp_status);

    r |= !!(e & float_flag_invalid) << FPU_STATUS_IV;
    r |= !!(e & float_flag_divbyzero) << FPU_STATUS_DZ;
    r |= !!(e & float_flag_overflow) << FPU_STATUS_OF;
    r |= !!(e & float_flag_underflow) << FPU_STATUS_UF;
    r |= !!(e & float_flag_inexact) << FPU_STATUS_IX;

    return r;
}

/* Turn the ARC FPU status value VAL into the internal soft fpu flags */
static void arc_unpack_fpu_status(CPUARCState *env, uint32_t val)
{
    int tmp = 0;

    tmp |= ((val >> FPU_STATUS_IV) & 1) ? float_flag_invalid   : 0;
    tmp |= ((val >> FPU_STATUS_DZ) & 1) ? float_flag_divbyzero : 0;
    tmp |= ((val >> FPU_STATUS_OF) & 1) ? float_flag_overflow  : 0;
    tmp |= ((val >> FPU_STATUS_UF) & 1) ? float_flag_underflow : 0;
    tmp |= ((val >> FPU_STATUS_IX) & 1) ? float_flag_inexact   : 0;

    set_float_exception_flags(tmp, &env->fp_status);
}

void init_fpu(CPUARCState *env, bool fp_hp, bool fp_dp, bool fp_wide)
{
    fpr_width = fp_dp ? 64 : 32;
    vfp_width = fp_wide ? 2 * fpr_width : fpr_width;

    /*
     * Disable default NaN mode (no platform default)
     * NaN propagation is always done according to IEEE 754-2008
     */
    set_default_nan_mode(0, &env->fp_status);
    /* As per ARCv3 PRM PU_CTRL Field Description */
    set_float_rounding_mode(float_round_nearest_even, &env->fp_status);
    /* Ensure exception flags start cleared */
    set_float_exception_flags(0, &env->fp_status);

    env->fp_status_persistent = arc_pack_fpu_status(env);
}

/*
 * The _get and _set functions have their headers automatically generated in
 * regs.c via the tempaltes AUX_REG_SETTER and AUX_REG_GETTER, thus the unused
 * aux_reg_detail
 */
target_ulong arc_fpu_status_get(const struct arc_aux_reg_detail *aux_reg_detail,
                                void *data)
{
    CPUARCState *env = (CPUARCState *) data;
    return (uint32_t)(env->fp_status_persistent);
}

void arc_fpu_status_set(const struct arc_aux_reg_detail *aux_reg_detail,
                        target_ulong val, void *data)
{
    CPUARCState *env = (CPUARCState *) data;
    /*
     * As per the PRM, the FWE bit (which is not written into FPU_STATUS) acts
     *  as a control signal to determine whether individual flags can be set
     *  directly or only cleared via writting to the 'Clear X' bits
     */
    bool fwe_flag = !!(val & (1 << FPU_STATUS_FWE));
    uint32_t flags = env->fp_status_persistent;

    if (fwe_flag == 0) {
        /*
         * fwe_flag == 0, Direct writes are disabled, flags can only be cleared
         * via their respective 'Clear X' bits.
         */
        if (((val >> FPU_STATUS_CAL) & 1) == 1) {
            /* Clear all (CAL is set) */
            flags = 0;
        } else {
            /* Clear individually */
            flags = (flags & 0x1f) & (((~(val >> 8)) & 0x1f));
        }
    } else {
        /*
         * fwe_flag == 1, Direct writes are enabled, and data is written directly
         * into IV, DZ, OF, UF and IX fields (bits 0 through 5) from val
         */
        flags = (flags & (~0x1f)) | (val & 0x1f);
    }
    env->fp_status_persistent = flags;
    arc_unpack_fpu_status(env, flags);
    qemu_log_mask(LOG_UNIMP,
                  "Writing 0x%08x to FP_STATUS register resulted in 0x%08x\n",
                  (uint32_t)val, flags);
}

target_ulong arc_fpu_ctrl_get(const struct arc_aux_reg_detail *aux_reg_detail,
                              void *data)
{
    CPUARCState *env = (CPUARCState *) data;

    uint32_t ret = 0;
    uint32_t rmode;

    switch (get_float_rounding_mode(&env->fp_status)) {
    case float_round_nearest_even:
        rmode = arc_round_nearest_even;
        break;
    case float_round_down:
        rmode = arc_round_down;
        break;
    case float_round_up:
        rmode = arc_round_up;
        break;
    case float_round_to_zero:
        rmode = arc_round_to_zero;
        break;
    default:
        g_assert_not_reached();
        break;
    }

    ret |= (rmode << 8);

    ret |= env->enable_invop_excp   << FPU_CTRL_IVE;
    ret |= env->enable_divzero_excp << FPU_CTRL_DZE;

    return ret;
}

void arc_fpu_ctrl_set(const struct arc_aux_reg_detail *aux_reg_detail,
                      target_ulong val, void *data)
{
    CPUARCState *env = (CPUARCState *) data;

    const FloatRoundMode conversion[] = {
        [arc_round_to_zero]      = float_round_to_zero,
        [arc_round_nearest_even] = float_round_nearest_even,
        [arc_round_up]           = float_round_up,
        [arc_round_down]         = float_round_down
    };
    const int arc_round = (val >> 8) & 3;
    g_assert(arc_round >= 0 && arc_round < sizeof(conversion));

    set_float_rounding_mode(conversion[arc_round], &env->fp_status);
    env->enable_invop_excp   = !!(val & (1 << FPU_CTRL_IVE));
    env->enable_divzero_excp = !!(val & (1 << FPU_CTRL_DZE));
}

/* Check if the current fpu status requires an exception to be raised */
inline void check_fpu_raise_exception(CPUARCState *env)
{
    env->fp_status_persistent |= arc_pack_fpu_status(env);
    qemu_log_mask(LOG_UNIMP,
                  "FPU_STATUS = 0x%08x\n",
                  (uint32_t) env->fp_status_persistent);

    if (((env->fp_status_persistent >> FPU_STATUS_IV) & 1
          && env->enable_invop_excp != 0)
        || ((env->fp_status_persistent >> FPU_STATUS_DZ) & 1
          && env->enable_divzero_excp != 0)) {
        arc_raise_exception(env, GETPC(), EXCP_EXTENSION);
    }
}
