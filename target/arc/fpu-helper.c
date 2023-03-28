/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2023 Synopsys Inc.
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
#include "fpu/softfloat-helpers.h"
#include "fpu.h"
#include "qemu/log.h"

static uint8_t fpr_width;
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
static inline void check_fpu_raise_exception(CPUARCState *env)
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

/* Soft fpu helper of type floatSIZE_operation */
#define FLOAT_INST3_HELPERS(NAME, HELPER, SIZE)                         \
uint64_t helper_##NAME(CPUARCState *env, uint64_t frs1, uint64_t frs2)  \
{                                                                       \
    uint64_t ret = float##SIZE##_##HELPER(frs1, frs2, &env->fp_status); \
    check_fpu_raise_exception(env);                                     \
    return ret;                                                         \
}

FLOAT_INST3_HELPERS(fdadd, add, 64)
FLOAT_INST3_HELPERS(fdsub, sub, 64)
FLOAT_INST3_HELPERS(fdmul, mul, 64)
FLOAT_INST3_HELPERS(fddiv, div, 64)
FLOAT_INST3_HELPERS(fdmin, minnum, 64)
FLOAT_INST3_HELPERS(fdmax, maxnum, 64)

FLOAT_INST3_HELPERS(fsadd, add, 32)
FLOAT_INST3_HELPERS(fssub, sub, 32)
FLOAT_INST3_HELPERS(fsmul, mul, 32)
FLOAT_INST3_HELPERS(fsdiv, div, 32)
FLOAT_INST3_HELPERS(fsmin, minnum, 32)
FLOAT_INST3_HELPERS(fsmax, maxnum, 32)

FLOAT_INST3_HELPERS(fhadd, add, 16)
FLOAT_INST3_HELPERS(fhsub, sub, 16)
FLOAT_INST3_HELPERS(fhmul, mul, 16)
FLOAT_INST3_HELPERS(fhdiv, div, 16)
FLOAT_INST3_HELPERS(fhmin, minnum, 16)
FLOAT_INST3_HELPERS(fhmax, maxnum, 16)

/* Soft fpu helper of type floatSIZE_muladd */
#define FLOAT_MULADD_HELPERS(NAME, SIZE, FLAGS)                              \
uint64_t helper_##NAME(CPUARCState *env, uint64_t b, uint64_t c, uint64_t d) \
{                                                                            \
    uint##SIZE##_t ret;                                                      \
    ret = float##SIZE##_muladd(b, c, d, FLAGS, &env->fp_status);             \
    check_fpu_raise_exception(env);                                          \
    return ret;                                                              \
}

FLOAT_MULADD_HELPERS(fdmadd,  64, 0)
FLOAT_MULADD_HELPERS(fdmsub,  64, float_muladd_negate_product)
FLOAT_MULADD_HELPERS(fdnmadd, 64, float_muladd_negate_result)
FLOAT_MULADD_HELPERS(fdnmsub, 64, float_muladd_negate_result | \
                                  float_muladd_negate_product)

FLOAT_MULADD_HELPERS(fsmadd,  32, 0)
FLOAT_MULADD_HELPERS(fsmsub,  32, float_muladd_negate_product)
FLOAT_MULADD_HELPERS(fsnmadd, 32, float_muladd_negate_result)
FLOAT_MULADD_HELPERS(fsnmsub, 32, float_muladd_negate_result | \
                                  float_muladd_negate_product)

FLOAT_MULADD_HELPERS(fhmadd,  16, 0)
FLOAT_MULADD_HELPERS(fhmsub,  16, float_muladd_negate_product)
FLOAT_MULADD_HELPERS(fhnmadd, 16, float_muladd_negate_result)
FLOAT_MULADD_HELPERS(fhnmsub, 16, float_muladd_negate_result | \
                                  float_muladd_negate_product)

/* Soft fpu helper for quiet compares */
#define FLOAT_COMPARE(NAME, HELPER)                                    \
void helper_##NAME(CPUARCState *env, uint64_t frs1, uint64_t frs2)     \
{                                                                      \
    switch (HELPER(frs1, frs2, &env->fp_status)) {                     \
    case float_relation_unordered:                                     \
        env->stat.Zf = 0;                                              \
        env->stat.Nf = 0;                                              \
        env->stat.Cf = 0;                                              \
        env->stat.Vf = 1;                                              \
    break;                                                             \
    case float_relation_less:                                          \
        env->stat.Zf = 0;                                              \
        env->stat.Nf = 1;                                              \
        env->stat.Cf = 1;                                              \
        env->stat.Vf = 0;                                              \
    break;                                                             \
    case float_relation_equal:                                         \
        env->stat.Zf = 1;                                              \
        env->stat.Nf = 0;                                              \
        env->stat.Cf = 0;                                              \
        env->stat.Vf = 0;                                              \
    break;                                                             \
    case float_relation_greater:                                       \
        env->stat.Zf = 0;                                              \
        env->stat.Nf = 0;                                              \
        env->stat.Cf = 0;                                              \
        env->stat.Vf = 0;                                              \
    break;                                                             \
    default:                                                           \
        g_assert("Unimplemented float comparison result. Check enum"); \
    }                                                                  \
                                                                       \
    check_fpu_raise_exception(env);                                    \
}

FLOAT_COMPARE(fdcmp, float64_compare_quiet)
FLOAT_COMPARE(fscmp, float32_compare_quiet)
FLOAT_COMPARE(fhcmp, float16_compare_quiet)

FLOAT_COMPARE(fdcmpf, float64_compare)
FLOAT_COMPARE(fscmpf, float32_compare)
FLOAT_COMPARE(fhcmpf, float16_compare)

/*
 * Soft fpu helper for 1 operand operations (conversions / roundings /
 * sqrt)
 */
#define CONVERSION_HELPERS(NAME, FLT_HELPER) \
uint64_t helper_##NAME(CPUARCState *env, uint64_t src) \
{                                                      \
    uint64_t ret = FLT_HELPER(src, &env->fp_status);   \
    check_fpu_raise_exception(env);                    \
    return ret;                                        \
}

CONVERSION_HELPERS(fs2d, float32_to_float64)
CONVERSION_HELPERS(fd2s, float64_to_float32)

CONVERSION_HELPERS(fl2d, int64_to_float64)
CONVERSION_HELPERS(fd2l, float64_to_int64)
CONVERSION_HELPERS(fd2l_rz, float64_to_int64_round_to_zero)
CONVERSION_HELPERS(ful2d, uint64_to_float64)
CONVERSION_HELPERS(fd2ul, float64_to_uint64)
CONVERSION_HELPERS(fd2ul_rz, float64_to_uint64_round_to_zero)

CONVERSION_HELPERS(fint2d, int32_to_float64)
CONVERSION_HELPERS(fd2int, float64_to_int32)
CONVERSION_HELPERS(fd2int_rz, float64_to_int32_round_to_zero)
CONVERSION_HELPERS(fuint2d, uint32_to_float64)
CONVERSION_HELPERS(fd2uint, float64_to_uint32)
CONVERSION_HELPERS(fd2uint_rz, float64_to_uint32_round_to_zero)

CONVERSION_HELPERS(fl2s, int64_to_float32)
CONVERSION_HELPERS(fs2l, float32_to_int64)
CONVERSION_HELPERS(fs2l_rz, float32_to_int64_round_to_zero)
CONVERSION_HELPERS(ful2s, uint64_to_float32)
CONVERSION_HELPERS(fs2ul, float32_to_uint64)
CONVERSION_HELPERS(fs2ul_rz, float32_to_uint64_round_to_zero)

CONVERSION_HELPERS(fint2s, int32_to_float32)
CONVERSION_HELPERS(fs2int, float32_to_int32)
CONVERSION_HELPERS(fs2int_rz, float32_to_int32_round_to_zero)
CONVERSION_HELPERS(fuint2s, uint32_to_float32)
CONVERSION_HELPERS(fs2uint, float32_to_uint32)
CONVERSION_HELPERS(fs2uint_rz, float32_to_uint32_round_to_zero)

CONVERSION_HELPERS(fdrnd, float64_round_to_int)
CONVERSION_HELPERS(fsrnd, float32_round_to_int)

CONVERSION_HELPERS(fdsqrt, float64_sqrt)
CONVERSION_HELPERS(fssqrt, float32_sqrt)
CONVERSION_HELPERS(fhsqrt, float16_sqrt)

/* Soft fpu helper for round_to_int with rounding to zero mode */
#define CONVERSION_HELPERS_RZ(NAME, FLT_HELPER)                       \
uint64_t helper_##NAME(CPUARCState *env, uint64_t src)                \
{                                                                     \
    FloatRoundMode saved_rounding_mode;                               \
    bool saved_flush_zero;                                            \
                                                                      \
    saved_flush_zero = get_flush_to_zero(&env->fp_status);            \
    saved_rounding_mode = get_float_rounding_mode(&env->fp_status);   \
                                                                      \
    set_flush_to_zero(true, &env->fp_status);                         \
    set_float_rounding_mode(float_round_to_zero, &env->fp_status);    \
                                                                      \
    uint64_t ret = FLT_HELPER(src, &env->fp_status);                  \
                                                                      \
    set_flush_to_zero(saved_flush_zero, &env->fp_status);             \
    set_float_rounding_mode(saved_rounding_mode, &env->fp_status);    \
                                                                      \
    check_fpu_raise_exception(env);                                   \
    return ret;                                                       \
}

CONVERSION_HELPERS_RZ(fdrnd_rz, float64_round_to_int)
CONVERSION_HELPERS_RZ(fsrnd_rz, float32_round_to_int)

/* Soft fpu helper for float32_to_float16 (single to half precision) */
uint64_t helper_fs2h(CPUARCState *env, uint64_t src)
{
    uint64_t ret = float32_to_float16(src, true, &env->fp_status);
    check_fpu_raise_exception(env);
    return ret;
}

/*
 * Soft fpu helper for float32_to_float16 (single to half precision)
 * rounding to zero mode
 */
uint64_t helper_fs2h_rz(CPUARCState *env, uint64_t src)
{
    FloatRoundMode saved_rounding_mode;
    bool saved_flush_zero;

    saved_flush_zero = get_flush_to_zero(&env->fp_status);
    saved_rounding_mode = get_float_rounding_mode(&env->fp_status);

    set_flush_to_zero(true, &env->fp_status);
    set_float_rounding_mode(float_round_to_zero, &env->fp_status);

    uint64_t ret = float32_to_float16(src, true, &env->fp_status);

    set_flush_to_zero(saved_flush_zero, &env->fp_status);
    set_float_rounding_mode(saved_rounding_mode, &env->fp_status);

    check_fpu_raise_exception(env);
    return ret;
}

/* Soft fpu helper for float16_to_float32 (single to half precision) */
uint64_t helper_fh2s(CPUARCState *env, uint64_t src)
{
    uint64_t ret = float16_to_float32(src, true, &env->fp_status);
    check_fpu_raise_exception(env);
    return ret;
}

/*
 * Soft fpu helper for vector insert (vfXins) instructions
 * ((size*)dest)[index] = ((size*)dest)orig[index]
 * Returns dest
 */
uint64_t
helper_vfins(CPUARCState *env, uint64_t dest, uint64_t index, uint64_t orig,
             uint64_t size)
{
    g_assert(index < VLEN(size));

    uint64_t mask = ((1ull << size) - 1ull) << (index * size);
    if (size == 64) {
        mask = (uint64_t) -1ull;
    }
    uint64_t ret = dest;

    ret = ret & (~mask);
    ret |= (orig << (index * size)) & mask;

    return ret;
}

/*
 * Soft fpu helper for vector extract (vfXext) instruction
 * Returns ((size*)orig)[index]
 */
uint64_t
helper_vfext(CPUARCState *env, uint64_t orig, uint64_t index, uint64_t size)
{
    g_assert(index < VLEN(size));
    uint64_t mask = ((1ull << size) - 1ull) << (index * size);
    if (size == 64) {
        mask = (uint64_t) -1ull;
    }
    uint64_t ret = (orig & mask) >> (index * size);

    return ret;
}

/*
 * Soft fpu helper for vector replicate (vfXrep) instruction
 * Inserts value at every index of the vector
 */
uint64_t
helper_vfrep(CPUARCState *env, uint64_t value, uint64_t size)
{
    uint64_t mask = ((1ull << size) - 1ull);
    if (size == 64) {
        mask = (uint64_t) -1ull;
    }
    uint64_t ret = value & mask;

    while (size < 64) {
        ret = ret | (ret << size);
        size = size << 1;
    }

    return ret;
}

/* Single vector, operate on each element (sqrt) */
#define VFLOAT2(NAME, SIZE, OP)                           \
uint64_t                                                  \
HELPER(NAME)(CPUARCState *env, uint64_t b)                \
{                                                         \
    g_assert(SIZE <= 64 && SIZE > 0);                     \
    uint64_t ret = 0;                                     \
    uint64_t mask = VEC_MASK(SIZE);                       \
    for (int i = 0; i <= VLEN(SIZE); i++) {               \
        uint64_t b_elem_i = (b >> (i * SIZE)) & mask;     \
        uint64_t ret_elem_i = HELPER(OP)(env, b_elem_i);  \
        ret |= (ret_elem_i & mask) << (i * SIZE);         \
    }                                                     \
                                                          \
    return ret;                                           \
}

VFLOAT2(vfhsqrt, 16, fhsqrt)
VFLOAT2(vfssqrt, 32, fssqrt)
VFLOAT2(vfdsqrt, 64, fdsqrt)

/* Two vectors, operate between each element (add/sub/div/mul) */
#define VFLOAT3(NAME, SIZE, OP) \
uint64_t \
HELPER(NAME)(CPUARCState *env, uint64_t b, uint64_t c)              \
{                                                                   \
    g_assert(SIZE <= 64 && SIZE > 0);                               \
    uint64_t ret = 0;                                               \
    uint64_t mask = VEC_MASK(SIZE);                                 \
    for (int i = 0; i <= VLEN(SIZE); i++) {                         \
        uint64_t b_elem_i = (b >> (i * SIZE)) & mask;               \
        uint64_t c_elem_i = (c >> (i * SIZE)) & mask;               \
        uint64_t ret_elem_i = HELPER(OP)(env, b_elem_i, c_elem_i);  \
        ret |= (ret_elem_i & mask) << (i * SIZE);                   \
    }                                                               \
                                                                    \
  return ret;                                                       \
}

VFLOAT3(vfhmul, 16, fhmul)
VFLOAT3(vfsmul, 32, fsmul)
VFLOAT3(vfdmul, 64, fdmul)

VFLOAT3(vfhdiv, 16, fhdiv)
VFLOAT3(vfsdiv, 32, fsdiv)
VFLOAT3(vfddiv, 64, fddiv)

VFLOAT3(vfhadd, 16, fhadd)
VFLOAT3(vfsadd, 32, fsadd)
VFLOAT3(vfdadd, 64, fdadd)

VFLOAT3(vfhsub, 16, fhsub)
VFLOAT3(vfssub, 32, fssub)
VFLOAT3(vfdsub, 64, fdsub)

/*
 * Two vectors, alternating operations between each element (add/sub or
 * sub/add)
 */
#define VFLOAT3_2OPS_SPLIT(NAME, SIZE, OP1, OP2)                \
uint64_t                                                        \
HELPER(NAME)(CPUARCState *env, uint64_t b, uint64_t c)          \
{                                                               \
    g_assert(SIZE <= 64 && SIZE > 0);                           \
    uint64_t ret = 0;                                           \
    uint64_t mask = VEC_MASK(SIZE);                             \
    for (int i = 0; i <= VLEN(SIZE); i++) {                     \
        uint64_t b_elem_i = (b >> (i * SIZE)) & mask;           \
        uint64_t c_elem_i = (c >> (i * SIZE)) & mask;           \
        uint64_t ret_elem_i;                                    \
        if ((i & 0x01) == 0) {                                  \
            ret_elem_i = HELPER(OP1)(env, b_elem_i, c_elem_i);  \
        } else {                                                \
            ret_elem_i = HELPER(OP2)(env, b_elem_i, c_elem_i);  \
        }                                                       \
        ret |= (ret_elem_i & mask) << (i * SIZE);               \
    }                                                           \
                                                                \
    return ret;                                                 \
}

VFLOAT3_2OPS_SPLIT(vfhaddsub, 16, fhadd, fhsub)
VFLOAT3_2OPS_SPLIT(vfsaddsub, 32, fsadd, fssub)
VFLOAT3_2OPS_SPLIT(vfdaddsub, 64, fdadd, fdsub)

VFLOAT3_2OPS_SPLIT(vfhsubadd, 16, fhsub, fhadd)
VFLOAT3_2OPS_SPLIT(vfssubadd, 32, fssub, fsadd)
VFLOAT3_2OPS_SPLIT(vfdsubadd, 64, fdsub, fdadd)

/*
 * One vector, operate each element with provided scalar
 * (add/sub/div/mul)
 */
#define VFLOAT3_SCALARC(NAME, SIZE, OP)                       \
uint64_t                                                      \
HELPER(NAME)(CPUARCState *env, uint64_t b, uint64_t c)        \
{                                                             \
    g_assert(SIZE <= 64 && SIZE > 0);                         \
    uint64_t ret = 0;                                         \
    uint64_t mask = VEC_MASK(SIZE);                           \
    for (int i = 0; i <= VLEN(SIZE); i++) {                   \
        uint64_t b_elem_i = (b >> (i * SIZE)) & mask;         \
        uint64_t ret_elem_i = HELPER(OP)(env, b_elem_i, c);   \
        ret |= (ret_elem_i & mask) << (i * SIZE);             \
    }                                                         \
                                                              \
  return ret;                                                 \
}

VFLOAT3_SCALARC(vfhmuls, 16, fhmul)
VFLOAT3_SCALARC(vfsmuls, 32, fsmul)
VFLOAT3_SCALARC(vfdmuls, 64, fdmul)

VFLOAT3_SCALARC(vfhdivs, 16, fhdiv)
VFLOAT3_SCALARC(vfsdivs, 32, fsdiv)
VFLOAT3_SCALARC(vfddivs, 64, fddiv)

VFLOAT3_SCALARC(vfhadds, 16, fhadd)
VFLOAT3_SCALARC(vfsadds, 32, fsadd)
VFLOAT3_SCALARC(vfdadds, 64, fdadd)

VFLOAT3_SCALARC(vfhsubs, 16, fhsub)
VFLOAT3_SCALARC(vfssubs, 32, fssub)
VFLOAT3_SCALARC(vfdsubs, 64, fdsub)

/* Two vectors, multiplicate, add/sub between each element */
#define VFLOAT4(NAME, SIZE, OP)                                               \
uint64_t                                                                      \
HELPER(NAME)(CPUARCState *env, uint64_t b, uint64_t c, uint64_t d)            \
{                                                                             \
    g_assert(SIZE <= 64 && SIZE > 0);                                         \
    uint64_t ret = 0;                                                         \
    uint64_t mask = VEC_MASK(SIZE);                                           \
    for (int i = 0; i <= VLEN(SIZE); i++) {                                   \
        uint64_t b_elem_i = (b >> (i * SIZE)) & mask;                         \
        uint64_t c_elem_i = (c >> (i * SIZE)) & mask;                         \
        uint64_t d_index_i = (d >> (i * SIZE)) & mask;                        \
        uint64_t ret_elem_i = HELPER(OP)(env, b_elem_i, c_elem_i, d_index_i); \
        ret |= (ret_elem_i & mask) << (i * SIZE);                             \
    }                                                                         \
                                                                              \
    return ret;                                                               \
}

VFLOAT4(vfhmadd, 16, fhmadd)
VFLOAT4(vfsmadd, 32, fsmadd)
VFLOAT4(vfdmadd, 64, fdmadd)

VFLOAT4(vfhmsub, 16, fhnmsub)
VFLOAT4(vfsmsub, 32, fsnmsub)
VFLOAT4(vfdmsub, 64, fdnmsub)

VFLOAT4(vfhnmadd, 16, fhnmadd)
VFLOAT4(vfsnmadd, 32, fsnmadd)
VFLOAT4(vfdnmadd, 64, fdnmadd)

VFLOAT4(vfhnmsub, 16, fhnmsub)
VFLOAT4(vfsnmsub, 32, fsnmsub)
VFLOAT4(vfdnmsub, 64, fdnmsub)

/* Two vectors, multiplicate, add/sub between each element */
#define VFLOAT4_SCALARD(NAME, SIZE, OP) \
uint64_t \
HELPER(NAME)(CPUARCState *env, uint64_t b, uint64_t c, uint64_t d)     \
{                                                                      \
    g_assert(SIZE <= 64 && SIZE > 0);                                  \
    uint64_t ret = 0;                                                  \
    uint64_t mask = VEC_MASK(SIZE);                                    \
    for (int i = 0; i <= VLEN(SIZE); i++) {                            \
        uint64_t b_elem_i = (b >> (i * SIZE)) & mask;                  \
        uint64_t c_elem_i = (c >> (i * SIZE)) & mask;                  \
        uint64_t ret_elem_i = HELPER(OP)(env, b_elem_i, c_elem_i, d);  \
        ret |= (ret_elem_i & mask) << (i * SIZE);                      \
    }                                                                  \
                                                                       \
  return ret;                                                          \
}

VFLOAT4_SCALARD(vfhmadds, 16, fhmadd)
VFLOAT4_SCALARD(vfsmadds, 32, fsmadd)
VFLOAT4_SCALARD(vfdmadds, 64, fdmadd)

VFLOAT4_SCALARD(vfhmsubs, 16, fhnmsub)
VFLOAT4_SCALARD(vfsmsubs, 32, fsnmsub)
VFLOAT4_SCALARD(vfdmsubs, 64, fdnmsub)

VFLOAT4_SCALARD(vfhnmadds, 16, fhnmadd)
VFLOAT4_SCALARD(vfsnmadds, 32, fsnmadd)
VFLOAT4_SCALARD(vfdnmadds, 64, fdnmadd)

VFLOAT4_SCALARD(vfhnmsubs, 16, fhnmsub)
VFLOAT4_SCALARD(vfsnmsubs, 32, fsnmsub)
VFLOAT4_SCALARD(vfdnmsubs, 64, fdnmsub)

/*
 * Vector exchange permutation operation, swap elements of size SIZE in b
 */
#define VFEXCH(NAME, SIZE)                          \
uint64_t                                            \
HELPER(NAME)(CPUARCState *env, uint64_t b)          \
{                                                   \
    g_assert(SIZE <= 64 && SIZE > 0);               \
    uint64_t ret = 0;                               \
    uint64_t b_elem_i = b;                          \
    uint64_t mask = VEC_MASK(SIZE);                 \
    for (int i = 0; i <= VLEN(SIZE) / 2; i++) {     \
        uint64_t tmp1 = (b_elem_i & mask) << SIZE;  \
        b_elem_i = b_elem_i >> SIZE;                \
        uint64_t tmp2 = (b_elem_i & mask);          \
        b_elem_i = b_elem_i >> SIZE;                \
        ret |= (tmp1  | tmp2) << (i * (SIZE * 2));  \
    }                                               \
                                                    \
  return ret;                                       \
}

VFEXCH(vfhexch, 16)
VFEXCH(vfsexch, 32)


/*                       Shuffle Helpers                       */

/* @return int Returns the configuredFPU vfp width */
static int shuffle_type_entry_for_vfp_width(void)
{
    switch (vfp_width) {
    case 128:
        return 0;
    case 64:
        return 1;
    case 32:
        return 2;
    default:
        g_assert("Invalid vfp width for vector shuffle" == 0);
        return -1;
    }
}

/*
 * Performs a shuffle operation according to the requested
 * pattern type (i.e. HEXCH, SEXCH, DEXCH, etc)
 * high_part determines whether we are working on the lower or upper 64 bits (1
 *  is only used in 128 bit operations)
 * bh/bl and ch/cl are the higher and lower 64 bits of the respective operands
 * bh and ch are only different from 0 for 128 bit operations
 * Returns the shuffled result
 */
uint64_t helper_vector_shuffle(CPUARCState *env, target_ulong type,
                               target_ulong high_part, uint64_t bh, uint64_t bl,
                               uint64_t ch, uint64_t cl)
{
    bool input_in_high_part;
    uint64_t operand;
    uint64_t index;
    int p;

    char vfp_width_entry = shuffle_type_entry_for_vfp_width();

    g_assert(type < SHUFFLE_TYPE_SIZE && vfp_width_entry < 3);

    /* Pattern enum value for the instruction "type", width "vfp_width_entry" */
    enum shuffle_pattern_enum shuffle_pattern =
        shuffle_types[type][(int) vfp_width_entry];

    g_assert(shuffle_pattern != SHUFFLE_PAT_INV);

    /* Actual pattern */
    const struct shuffle_patterns_t *pat = &shuffle_patterns[shuffle_pattern];
    uint64_t ret = 0;

    char length = pat->length;

    /*
     * For double register operations, only half of the pattern operands are
     * used per call. helper_vector_shuffle is called separately for the high
     * and low parts of the pattern.
     * Therefore in those cases we only work with half the length of the pattern
     */
    if (pat->double_register == true) {
        length /= 2;
    }

    char elem_size = pat->elem_size;

    ret = 0;
    /*
     * i is the destination index, relative to the current operation and result
     * p is the destination index, absolute to the entire vector.
     * p starts at the end and i at the start of their respective ranges
     * operand is the register to source the operand using "mask" and "index"
     * index is the source index of the source operand, and is derived from p,
     *  which is derived from i
     *
     * Therefore, it starts at the end of the entire vector and is the index
     * inside the source operand, from where to retrieve the value that must be
     * placed in the relative destination indexed by i
     *
     * For each destination index i, do
     *    tmp = operand[index]
     *    ret |= tmp << (elem_size * i)
     */
    for (int i = 0; i < length; i++) {
        p = pat->length - (high_part == true ? length : 0) - i - 1;
        index = pat->index[p].index;

        input_in_high_part = (pat->double_register == true && index > 3) ?
                                  true : false;

        switch (pat->index[p].operand) {
        case B:
            operand = input_in_high_part ? bh : bl;
        break;
        case C:
            operand = input_in_high_part ? ch : cl;
        break;
        default:
            g_assert("Invalid operand for vector shuffle!" == 0);
        }

        /* index > 3 means double register (4*16 = 64 = single register size) */
        index = index > 3 ? index - 4 : index;

        uint64_t mask = -1;
        mask >>= (64 - elem_size);
        /*
         * Vectors are 16 bit indexed. With shifts and a mask we get the element
         * that must be placed in position "i"
         */
        uint64_t tmp = (operand >> (16 * index)) & mask;
        ret |= tmp << (elem_size * i);
    }

    return ret;
}
