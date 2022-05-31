/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2021 Synppsys Inc.
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

#define FWE	31
#define CIX 	12
#define CUF 	11
#define COF 	10
#define CDZ 	 9
#define CIV 	 8
#define CAL 	 7
#define IX  	 4
#define UF  	 3
#define OF  	 2
#define DZ  	 1
#define IV  	 0

#define BIT_READ(LOC, BIT)  ((LOC >> BIT) & 1)
#define BIT_WRITE(LOC, BIT, VAL)  LOC = (LOC & (~(1 << BIT))) | ((VAL & 1) << BIT)

#define READ_EXCP_FLAG(LOC, BIT) ((LOC & BIT) != 0)
#define WRITE_EXCP_FLAG(LOC, BIT, VALUE) LOC = ((LOC & (~BIT)) | ((VALUE & 1) * BIT))

static uint8_t fpr_width;
uint8_t vfp_width;
/* TODO: Maybe is not needed */
static uint8_t fpr_per_vector_operand;
static uint8_t vfp_max_length;

void
init_fpu(bool fp_dp, bool fp_wide, bool fp_hp)
{
  uint8_t index1 = ((fp_dp != 0) * 2) + (fp_wide != 0);
  const uint8_t fpr_widths [] = { 32, 32, 64, 64 };
  const uint8_t vfp_widths [] = { 32, 64, 64, 128 };

  fpr_width = fpr_widths[index1];
  vfp_width = vfp_widths[index1];

  fpr_per_vector_operand = (vfp_width / fpr_width);
  vfp_max_length = fpr_per_vector_operand * (vfp_width / 32) * (fp_hp != 0 ? 2 : 1);
}

static inline void check_fpu_raise_exception(CPUARCState *env)
{
    env->fp_status_persistent |= arc_pack_fpu_status(env);
    qemu_log_mask(LOG_UNIMP, "FLAG = 0x%08x\n", (uint32_t) env->fp_status_persistent);

    if((BIT_READ(env->fp_status_persistent, IV)
        && env->enable_invop_excp != 0)
       || (BIT_READ(env->fp_status_persistent, DZ)
	   && env->enable_divzero_excp != 0)) {
	arc_raise_exception(env, GETPC(), EXCP_EXTENSION);
    }
}

#define FLOAT_INST3_HELPERS(NAME, HELPER, SIZE) \
uint64_t helper_##NAME(CPUARCState *env, uint64_t frs1, uint64_t frs2) \
{ \
    uint64_t ret = float##SIZE##_##HELPER(frs1, frs2, &env->fp_status); \
    check_fpu_raise_exception(env); \
    return ret; \
} \

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

    /*
#define FLOAT_SGN_HELPERS(NAME, OPERATION, SIZE) \
uint64_t helper_##NAME(CPUARCState *env, uint64_t s1, uint64_t s2) \
{ \
    return float##SIZE##_set_sign(s1, OPERATION); \
} \

FLOAT_SGN_HELPERS(fdsgnj,  ( (s2 >> 63) & 1), 64)
FLOAT_SGN_HELPERS(fdsgnjn, (((s2 >> 63) & 1) == 0), 64)
FLOAT_SGN_HELPERS(fdsgnjx, ( (s2 >> 63) ^ (s1 >> 63)), 64)

FLOAT_SGN_HELPERS(fssgnj,  ( (s2 >> 31) & 1), 32)
FLOAT_SGN_HELPERS(fssgnjn, (((s2 >> 31) & 1) == 0), 32)
FLOAT_SGN_HELPERS(fssgnjx, ( (s2 >> 31) ^ (s1 >> 31)), 32)

FLOAT_SGN_HELPERS(fhsgnj,  ( (s2 >> 15) & 1), 16)
FLOAT_SGN_HELPERS(fhsgnjn, (((s2 >> 15) & 1) == 0), 16)
FLOAT_SGN_HELPERS(fhsgnjx, ( (s2 >> 15) ^ (s1 >> 15)), 16)
*/

#define FLOAT_MULADD_HELPERS(NAME, SIZE, FLAGS) \
uint64_t helper_##NAME(CPUARCState *env, uint64_t b, uint64_t c, uint64_t d) \
{ \
    uint##SIZE##_t tmp; \
    tmp = float##SIZE##_muladd(b, c, d, FLAGS, &env->fp_status); \
    check_fpu_raise_exception(env); \
    return tmp; \
}

FLOAT_MULADD_HELPERS(fdmadd, 64, 0)
FLOAT_MULADD_HELPERS(fdmsub, 64, float_muladd_negate_product)
FLOAT_MULADD_HELPERS(fdnmadd, 64, float_muladd_negate_result)
FLOAT_MULADD_HELPERS(fdnmsub, 64, float_muladd_negate_result | float_muladd_negate_product)

FLOAT_MULADD_HELPERS(fsmadd, 32, 0)
FLOAT_MULADD_HELPERS(fsmsub, 32, float_muladd_negate_product)
FLOAT_MULADD_HELPERS(fsnmadd, 32, float_muladd_negate_result)
FLOAT_MULADD_HELPERS(fsnmsub, 32, float_muladd_negate_result | float_muladd_negate_product)

FLOAT_MULADD_HELPERS(fhmadd, 16, 0)
FLOAT_MULADD_HELPERS(fhmsub, 16, float_muladd_negate_product)
FLOAT_MULADD_HELPERS(fhnmadd, 16, float_muladd_negate_result)
FLOAT_MULADD_HELPERS(fhnmsub, 16, float_muladd_negate_result | float_muladd_negate_product)

#define FLOAT_COMPARE(NAME, HELPER) \
void helper_##NAME(CPUARCState *env, uint64_t frs1, uint64_t frs2) \
{ \
    FloatRelation tmp = HELPER(frs1, frs2, &env->fp_status); \
    bool is_equal = (tmp == float_relation_equal); \
    bool is_smaller = (tmp == float_relation_less); \
 \
    if(tmp == float_relation_unordered) { \
        env->stat.Zf = 0; \
        env->stat.Nf = 0; \
        env->stat.Cf = 0; \
        env->stat.Vf = 1; \
    } else { \
        env->stat.Zf = is_equal; \
        env->stat.Nf = is_smaller; \
        env->stat.Cf = is_smaller; \
        env->stat.Vf = 0; \
    } \
    check_fpu_raise_exception(env); \
}

FLOAT_COMPARE(fdcmp, float64_compare_quiet)
FLOAT_COMPARE(fscmp, float32_compare_quiet)
FLOAT_COMPARE(fhcmp, float16_compare_quiet)

#define FLOAT_COMPARE_F(NAME, HELPER) \
void helper_##NAME(CPUARCState *env, uint64_t frs1, uint64_t frs2) \
{ \
    FloatRelation tmp = HELPER(frs1, frs2, &env->fp_status); \
    bool is_equal = (tmp == float_relation_equal); \
    bool is_smaller = (tmp == float_relation_less); \
    /* bool unordered = float64_is_any_nan(frs1) || float64_is_any_nan(frs2); */ \
 \
    if(tmp == float_relation_unordered) { \
        env->stat.Zf = 0; \
        env->stat.Nf = 0; \
        env->stat.Cf = 0; \
        env->stat.Vf = 1; \
    } else { \
        env->stat.Zf = is_equal; \
        env->stat.Nf = is_smaller; \
        env->stat.Cf = is_smaller; \
        env->stat.Vf = 0; \
    } \
    check_fpu_raise_exception(env); \
}

FLOAT_COMPARE_F(fdcmpf, float64_compare)
FLOAT_COMPARE_F(fscmpf, float32_compare)
FLOAT_COMPARE_F(fhcmpf, float16_compare)

#define CONVERTION_HELPERS(NAME, FLT_HELPER) \
uint64_t helper_##NAME(CPUARCState *env, uint64_t src) \
{ \
    uint64_t ret = FLT_HELPER(src, &env->fp_status); \
    check_fpu_raise_exception(env); \
    return ret; \
}

CONVERTION_HELPERS(fs2d, float32_to_float64)
CONVERTION_HELPERS(fd2s, float64_to_float32)

CONVERTION_HELPERS(fl2d, int64_to_float64)
CONVERTION_HELPERS(fd2l, float64_to_int64)
CONVERTION_HELPERS(fd2l_rz, float64_to_int64_round_to_zero)
CONVERTION_HELPERS(ful2d, uint64_to_float64)
CONVERTION_HELPERS(fd2ul, float64_to_uint64)
CONVERTION_HELPERS(fd2ul_rz, float64_to_uint64_round_to_zero)

CONVERTION_HELPERS(fint2d, int32_to_float64)
CONVERTION_HELPERS(fd2int, float64_to_int32)
CONVERTION_HELPERS(fd2int_rz, float64_to_int32_round_to_zero)
CONVERTION_HELPERS(fuint2d, uint32_to_float64)
CONVERTION_HELPERS(fd2uint, float64_to_uint32)
CONVERTION_HELPERS(fd2uint_rz, float64_to_uint32_round_to_zero)

CONVERTION_HELPERS(fl2s, int64_to_float32)
CONVERTION_HELPERS(fs2l, float32_to_int64)
CONVERTION_HELPERS(fs2l_rz, float32_to_int64_round_to_zero)
CONVERTION_HELPERS(ful2s, uint64_to_float32)
CONVERTION_HELPERS(fs2ul, float32_to_uint64)
CONVERTION_HELPERS(fs2ul_rz, float32_to_uint64_round_to_zero)

CONVERTION_HELPERS(fint2s, int32_to_float32)
CONVERTION_HELPERS(fs2int, float32_to_int32)
CONVERTION_HELPERS(fs2int_rz, float32_to_int32_round_to_zero)
CONVERTION_HELPERS(fuint2s, uint32_to_float32)
CONVERTION_HELPERS(fs2uint, float32_to_uint32)
CONVERTION_HELPERS(fs2uint_rz, float32_to_uint32_round_to_zero)

CONVERTION_HELPERS(fdrnd, float64_round_to_int)
CONVERTION_HELPERS(fsrnd, float32_round_to_int)


#define CONVERTION_HELPERS_RZ(NAME, FLT_HELPER) \
uint64_t helper_##NAME(CPUARCState *env, uint64_t src) \
{ \
    bool save = get_flush_to_zero(&env->fp_status); \
    set_flush_to_zero(true, &env->fp_status); \
    uint64_t ret = FLT_HELPER(src, &env->fp_status); \
    set_flush_to_zero(save, &env->fp_status); \
    check_fpu_raise_exception(env); \
    return ret; \
}

CONVERTION_HELPERS_RZ(fdrnd_rz, float64_round_to_int)
CONVERTION_HELPERS_RZ(fsrnd_rz, float32_round_to_int)

uint64_t helper_fs2h(CPUARCState *env, uint64_t src)
{
    uint64_t ret = float32_to_float16(src, true, &env->fp_status);
    check_fpu_raise_exception(env);
    return ret;
}

uint64_t helper_fs2h_rz(CPUARCState *env, uint64_t src)
{
    bool save = get_flush_to_zero(&env->fp_status);
    set_flush_to_zero(true, &env->fp_status);
    uint64_t ret =  float32_to_float16(src, true, &env->fp_status);
    set_flush_to_zero(save, &env->fp_status);
    check_fpu_raise_exception(env);
    return ret;
}

uint64_t helper_fh2s(CPUARCState *env, uint64_t src)
{
    uint64_t ret = float16_to_float32(src, true, &env->fp_status);
    check_fpu_raise_exception(env);
    return ret;
}

CONVERTION_HELPERS(fdsqrt, float64_sqrt)
CONVERTION_HELPERS(fssqrt, float32_sqrt)
CONVERTION_HELPERS(fhsqrt, float16_sqrt)

uint64_t
helper_vfins(CPUARCState *env, uint64_t dest, uint64_t index, uint64_t orig, uint64_t size)
{
  uint64_t mask = ((1ull << size) - 1ull) << (index * size);
  if(size == 64) {
      mask = (uint64_t) -1ull;
  }
  uint64_t ret = dest;

  ret = ret & (~mask);
  ret |= (orig << (index * size)) & mask;

  return ret;
}

uint64_t
helper_vfext(CPUARCState *env, uint64_t orig, uint64_t index, uint64_t size)
{
  uint64_t mask = ((1ull << size) - 1ull) << (index * size);
  if(size == (sizeof(uint64_t) << 3)) {
      mask = (uint64_t) -1ull;
  }
  uint64_t ret = (orig & mask) >> (index * size);

  return ret;
}

uint64_t
helper_vfrep(CPUARCState *env, uint64_t value, uint64_t size)
{
  uint64_t mask = ((1ull << size) - 1ull);
  if(size == (sizeof(uint64_t) << 3)) {
      mask = (uint64_t) -1ull;
  }
  uint64_t ret = value & mask;

  while(size < (sizeof(uint64_t) << 3))
    {
      ret = ret | (ret << size);
      size = size << 1;
    }

  return ret;
}

#define VLEN(SIZE) ((vfp_width > 64 ? 16 : vfp_width) >> (SIZE >> 3))

#define VEC_MASK(SIZE) (uint64_t) ((sizeof(uint64_t) == (SIZE >> 3)) ? \
		       -1ull : \
		       ((1ull << SIZE) - 1))

#define VFLOAT2(NAME, SIZE, OP) \
uint64_t \
HELPER(NAME)(CPUARCState *env, uint64_t b) \
{ \
  uint64_t ret = 0; \
  uint64_t mask = VEC_MASK(SIZE); \
  for(int i = 0; i <= VLEN(SIZE); i++) { \
    uint64_t _b = (b >> (i * SIZE)) & mask; \
    uint64_t _ret = HELPER(OP)(env, _b); \
    ret |= (_ret & mask) << (i * SIZE); \
  } \
 \
  return ret; \
}

VFLOAT2(vfhsqrt, 16, fhsqrt)
VFLOAT2(vfssqrt, 32, fssqrt)
VFLOAT2(vfdsqrt, 64, fdsqrt)


#define VFLOAT3(NAME, SIZE, OP) \
uint64_t \
HELPER(NAME)(CPUARCState *env, uint64_t b, uint64_t c) \
{ \
  uint64_t ret = 0; \
  uint64_t mask = VEC_MASK(SIZE); \
  for(int i = 0; i <= VLEN(SIZE); i++) { \
    uint64_t _b = (b >> (i * SIZE)) & mask; \
    uint64_t _c = (c >> (i * SIZE)) & mask; \
    uint64_t _ret = HELPER(OP)(env, _b, _c); \
    ret |= (_ret & mask) << (i * SIZE); \
  } \
 \
  return ret; \
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

#define VFLOAT3_2OPS_SPLIT(NAME, SIZE, OP1, OP2) \
uint64_t \
HELPER(NAME)(CPUARCState *env, uint64_t b, uint64_t c) \
{ \
  uint64_t ret = 0; \
  uint64_t mask = VEC_MASK(SIZE); \
  for(int i = 0; i <= VLEN(SIZE); i++) { \
    uint64_t _b = (b >> (i * SIZE)) & mask; \
    uint64_t _c = (c >> (i * SIZE)) & mask; \
    uint64_t _ret; \
    if((i % 2) == 0) \
      _ret = HELPER(OP1)(env, _b, _c); \
    else \
      _ret = HELPER(OP2)(env, _b, _c); \
    ret |= (_ret & mask) << (i * SIZE); \
  } \
 \
  return ret; \
}

VFLOAT3_2OPS_SPLIT(vfhaddsub, 16, fhadd, fhsub)
VFLOAT3_2OPS_SPLIT(vfsaddsub, 32, fsadd, fssub)
VFLOAT3_2OPS_SPLIT(vfdaddsub, 64, fdadd, fdsub)

VFLOAT3_2OPS_SPLIT(vfhsubadd, 16, fhsub, fhadd)
VFLOAT3_2OPS_SPLIT(vfssubadd, 32, fssub, fsadd)
VFLOAT3_2OPS_SPLIT(vfdsubadd, 64, fdsub, fdadd)

#define VFLOAT3_SCALARC(NAME, SIZE, OP) \
uint64_t \
HELPER(NAME)(CPUARCState *env, uint64_t b, uint64_t c) \
{ \
  uint64_t ret = 0; \
  uint64_t mask = VEC_MASK(SIZE); \
  for(int i = 0; i <= VLEN(SIZE); i++) { \
    uint64_t _b = (b >> (i * SIZE)) & mask; \
    uint64_t _ret = HELPER(OP)(env, _b, c); \
    ret |= (_ret & mask) << (i * SIZE); \
  } \
 \
  return ret; \
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

#define VFLOAT4(NAME, SIZE, OP) \
uint64_t \
HELPER(NAME)(CPUARCState *env, uint64_t b, uint64_t c, uint64_t d) \
{ \
  uint64_t ret = 0; \
  uint64_t mask = VEC_MASK(SIZE); \
  for(int i = 0; i <= VLEN(SIZE); i++) { \
    uint64_t _b = (b >> (i * SIZE)) & mask; \
    uint64_t _c = (c >> (i * SIZE)) & mask; \
    uint64_t _d = (c >> (i * SIZE)) & mask; \
    uint64_t _ret = HELPER(OP)(env, _b, _c, _d); \
    ret |= (_ret & mask) << (i * SIZE); \
  } \
 \
  return ret; \
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


#define VFLOAT4_SCALARD(NAME, SIZE, OP) \
uint64_t \
HELPER(NAME)(CPUARCState *env, uint64_t b, uint64_t c, uint64_t d) \
{ \
  uint64_t ret = 0; \
  uint64_t mask = VEC_MASK(SIZE); \
  for(int i = 0; i <= VLEN(SIZE); i++) { \
    uint64_t _b = (b >> (i * SIZE)) & mask; \
    uint64_t _c = (c >> (i * SIZE)) & mask; \
    uint64_t _ret = HELPER(OP)(env, _b, _c, d); \
    ret |= (_ret & mask) << (i * SIZE); \
  } \
 \
  return ret; \
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

#define VFEXCH(NAME, SIZE) \
uint64_t \
HELPER(NAME)(CPUARCState *env, uint64_t b) \
{ \
  uint64_t ret = 0; \
  uint64_t _b = b; \
  uint64_t mask = VEC_MASK(SIZE); \
  for(int i = 0; i <= VLEN(SIZE)/2; i++) { \
    uint64_t tmp1 = (_b & mask) << SIZE; \
    _b = _b >> SIZE; \
    uint64_t tmp2 = (_b & mask); \
    _b = _b >> SIZE; \
    ret |= (tmp1  | tmp2) << (i * (SIZE*2)); \
  } \
 \
  return ret; \
}
VFEXCH(vfhexch, 16)
VFEXCH(vfsexch, 32)

#define VFUNPK(NAME, SIZE, SHIFT) \
uint64_t \
HELPER(NAME)(CPUARCState *env, uint64_t b, uint64_t c, ) \
{ \
  uint64_t ret = 0; \
  uint64_t _b = b >> SHIFT; \
  uint64_t _c = c >> SHIFT; \
  uint64_t mask = VEC_MASK(SIZE); \
  for(int i = 0; i <= VLEN(SIZE)/2; i++) { \
    uint64_t tmp1 = (_b & mask) << SIZE; \
    _b = _b >> SIZE; \
    uint64_t tmp2 = (_b & mask); \
    _b = _b >> SIZE; \
    ret |= (tmp1  | tmp2) << (i * (SIZE*2)); \
  } \
 \
  return ret; \
}

enum shuffle_pattern_enum {
  SHUFFLE_PAT_INV = -1,

  /* 32 bit vectors */
  V32_H_B0_B1,
  V32_H_C0_B0,
  V32_H_C1_B1,

  /* 64 bit vectors */
  V64_H_B2_B3_B0_B1,
  V64_S_B0_B2,

  V64_H_C2_C0_B2_B0,
  V64_H_C3_C1_B3_B1,
  V64_S_C0_B0,
  V64_S_C2_B2,

  V64_H_C1_B1_C0_B0,
  V64_H_C3_B3_C2_B2,

  V64_H_C2_B2_C0_B0,
  V64_H_C3_B3_C1_B1,

  /* 128bit wide vectors */
  V128_H_B6_B7_B4_B5_B2_B3_B0_B1,
  V128_S_B4_B6_B0_B2,
  V128_D_B0_B4,

  V128_H_C6_C4_C2_C0_B6_B4_B2_B0,
  V128_H_C7_C5_C3_C1_B7_B5_B3_B1,

  V128_S_C4_C0_B4_B0,
  V128_S_C6_C2_B6_B2,

  V128_D_C0_B0,
  V128_D_C4_B4,

  V128_H_C3_B3_C2_B2_C1_B1_C0_B0,
  V128_H_C7_B7_C6_B6_C5_B5_C4_B4,

  V128_S_C2_B2_C0_B0,
  V128_S_C6_B6_C4_B4,

  V128_H_C6_B6_C4_B4_C2_B2_C0_B0,
  V128_H_C7_B7_C5_B5_C3_B3_C1_B1,

  V128_S_C4_B4_C0_B0,
  V128_S_C6_B6_C2_B2,
  SHUFFLE_PATTERN_SIZE,
};

enum shuffle_operand {
  B = 0,
  C
};


struct shuffle_patterns_t {
  bool double_register;
  const char vector_size;
  const char elem_size;
  const char length;
  struct shuffle_replacement {
    const enum shuffle_operand operand;
    const char index; /* Considering one position is always 16bits */
  } index[8];
};

#define B0 {B, 0}
#define B1 {B, 1}
#define B2 {B, 2}
#define B3 {B, 3}
#define B4 {B, 4}
#define B5 {B, 5}
#define B6 {B, 6}
#define B7 {B, 7}
#define C0 {C, 0}
#define C1 {C, 1}
#define C2 {C, 2}
#define C3 {C, 3}
#define C4 {C, 4}
#define C5 {C, 5}
#define C6 {C, 6}
#define C7 {C, 7}

const struct shuffle_patterns_t shuffle_patterns[SHUFFLE_PATTERN_SIZE] = {
  [V32_H_B0_B1] = { false, 32, 16, 2, { B0, B1 } },
  [V32_H_C0_B0] = { false, 32, 16, 2, { C0, B0 } },
  [V32_H_C1_B1] = { false, 32, 16, 2, { C1, B1 } },

  /* 64 bit vectors */
  [V64_H_B2_B3_B0_B1] = { false, 64, 16, 4, { B2, B3, B0, B1 } },
  [V64_S_B0_B2]	      = { false, 64, 32, 2, { B0, B2 } },

  [V64_H_C2_C0_B2_B0] = { false, 64, 16, 4, { C2, C0, B2, B0 } },
  [V64_H_C3_C1_B3_B1] = { false, 64, 16, 4, { C2, C0, B2, B0 } },
  [V64_S_C0_B0]	      = { false, 64, 32, 2, { C2, C0 } },
  [V64_S_C2_B2]	      = { false, 64, 32, 2, { C2, B2 } },

  [V64_H_C1_B1_C0_B0] = { false, 64, 16, 4, { C1, B1, C0, B0 } },
  [V64_H_C3_B3_C2_B2] = { false, 64, 16, 4, { C3, B3, C2, B2 } },

  [V64_H_C2_B2_C0_B0] = { false, 64, 16, 4, { C2, B2, C0, B0 } },
  [V64_H_C3_B3_C2_B2] = { false, 64, 16, 4, { C3, B3, C2, B2 } },

  /* 128bit wide vectors */
  [V128_H_B6_B7_B4_B5_B2_B3_B0_B1] = { true, 128, 16, 8, { B6, B7, B4, B5, B2, B3, B0, B1 } },
  [V128_S_B4_B6_B0_B2]		   = { true, 128, 32, 4, { B4, B6, B0, B2 } },
  [V128_D_B0_B4]		   = { true, 128, 64, 2, { B0, B4 } },

  [V128_H_C6_C4_C2_C0_B6_B4_B2_B0] = { true, 128, 16, 8, { C6, C4, C2, C0, B6, B4, B2, B0 } },
  [V128_H_C7_C5_C3_C1_B7_B5_B3_B1] = { true, 128, 16, 8, { C7, C5, C3, C1, B7, B5, B3, B1 } },

  [V128_S_C4_C0_B4_B0] = { true, 128, 32, 4, { C4, C0, B4, B0 } },
  [V128_S_C6_C2_B6_B2] = { true, 128, 32, 4, { C6, C2, B6, B2 } },

  [V128_D_C0_B0] = { true, 128, 64, 2, { C0, B0 } },
  [V128_D_C4_B4] = { true, 128, 64, 2, { C4, B4 } },

  [V128_H_C3_B3_C2_B2_C1_B1_C0_B0] = { true, 128, 16, 8, { C3, B3, C2, B2, C1, B1, C0, B0 } },
  [V128_H_C7_B7_C6_B6_C5_B5_C4_B4] = { true, 128, 16, 8, { C7, B7, C6, B6, C5, B5, C4, B4 } },

  [V128_S_C2_B2_C0_B0] = { true, 128, 32, 4, { C2, B2, C0, B0 } },
  [V128_S_C6_B6_C4_B4] = { true, 128, 32, 4, { C6, B6, C4, B4 } },

  [V128_H_C6_B6_C4_B4_C2_B2_C0_B0] = { true, 128, 16, 8, { C6, B6, C4, B4, C2, B2, C0, B0 } },
  [V128_H_C7_B7_C5_B5_C3_B3_C1_B1] = { true, 128, 16, 8, { C7, B7, C5, B5, C3, B3, C1, B1 } },

  [V128_S_C4_B4_C0_B0] = { true, 128, 32, 4, { C4, B4, C0, B0 } },
  [V128_S_C6_B6_C2_B2] = { true, 128, 32, 4, { C6, B6, C2, B2 } }
};


const enum shuffle_pattern_enum shuffle_types[SHUFFLE_TYPE_SIZE][3] = {
  [HEXCH]  = { V128_H_B6_B7_B4_B5_B2_B3_B0_B1,	V64_H_B2_B3_B0_B1,  V32_H_B0_B1 },
  [SEXCH]  = { V128_S_B4_B6_B0_B2,	       	V64_S_B0_B2,	    SHUFFLE_PAT_INV },
  [DEXCH]  = { V128_D_B0_B4,		       	SHUFFLE_PAT_INV,    SHUFFLE_PAT_INV },
  [HUNPKL] = { V128_H_C6_C4_C2_C0_B6_B4_B2_B0, 	V64_H_C2_C0_B2_B0,  V32_H_C0_B0 },
  [HUNPKM] = { V128_H_C7_C5_C3_C1_B7_B5_B3_B1, 	V64_H_C3_C1_B3_B1,  V32_H_C1_B1 },
  [SUNPKL] = { V128_S_C4_C0_B4_B0,	       	V64_S_C0_B0,	    SHUFFLE_PAT_INV },
  [SUNPKM] = { V128_S_C6_C2_B6_B2,	       	V64_S_C2_B2,	    SHUFFLE_PAT_INV },
  [DUNPKL] = { V128_D_C0_B0,		       	SHUFFLE_PAT_INV,    SHUFFLE_PAT_INV },
  [DUNPKM] = { V128_D_C4_B4,		       	SHUFFLE_PAT_INV,    SHUFFLE_PAT_INV },
  [HPACKL] = { V128_H_C3_B3_C2_B2_C1_B1_C0_B0, 	V64_H_C1_B1_C0_B0,  V32_H_C0_B0 },
  [HPACKM] = { V128_H_C7_B7_C6_B6_C5_B5_C4_B4, 	V64_H_C3_B3_C2_B2,  V32_H_C1_B1 },
  [SPACKL] = { V128_S_C2_B2_C0_B0,	       	V64_S_C0_B0,	    SHUFFLE_PAT_INV },
  [SPACKM] = { V128_S_C6_B6_C4_B4,	       	V64_S_C2_B2,	    SHUFFLE_PAT_INV },
  [DPACKL] = { V128_D_C0_B0,		       	SHUFFLE_PAT_INV,    SHUFFLE_PAT_INV },
  [DPACKM] = { V128_D_C4_B4,		       	SHUFFLE_PAT_INV,    SHUFFLE_PAT_INV },
  [HBFLYL] = { V128_H_C6_B6_C4_B4_C2_B2_C0_B0, 	V64_H_C2_B2_C0_B0,  SHUFFLE_PAT_INV },
  [HBFLYM] = { V128_H_C7_B7_C5_B5_C3_B3_C1_B1, 	V64_H_C3_B3_C1_B1,  SHUFFLE_PAT_INV },
  [SBFLYL] = { V128_S_C4_B4_C0_B0,	       	V64_S_C0_B0,	    SHUFFLE_PAT_INV },
  [SBFLYM] = { V128_S_C6_B6_C2_B2,	       	V64_S_C2_B2,	    SHUFFLE_PAT_INV },
  [DBFLYL] = { V128_D_C0_B0,		       	SHUFFLE_PAT_INV,    SHUFFLE_PAT_INV },
  [DBFLYM] = { V128_D_C4_B4,		       	SHUFFLE_PAT_INV,    SHUFFLE_PAT_INV },
};

static int shuffle_type_entry_for_vfp_width(void) {
  switch(vfp_width) {
    case 128:
      return 0;
    case 64:
      return 1;
    case 32:
      return 2;
    default:
      assert("Should never happen" == 0);
  }
}



uint64_t
helper_vector_shuffle(CPUARCState *env, target_ulong type, target_ulong high_part,
		      uint64_t bh, uint64_t bl, uint64_t ch, uint64_t cl)
{
  char vfp_width_entry = shuffle_type_entry_for_vfp_width();
  enum shuffle_pattern_enum shuffle_pattern =
	  shuffle_types[type][(int) vfp_width_entry];
  assert(shuffle_pattern != SHUFFLE_PAT_INV);
  const struct shuffle_patterns_t *pat = &shuffle_patterns[shuffle_pattern];
  uint64_t ret = 0;

  char length = pat->length;

  if(pat->double_register == true)
    length /= 2;

  char elem_size = pat->elem_size;

  ret = 0;
  for(int i = 0; i < length; i++) {
    uint64_t *elem = NULL;
    int p = pat->length - (high_part == true ? length : 0) - i - 1;
    uint64_t index = pat->index[p].index;

    bool input_in_high_part = pat->double_register == true && index >= length ? true : false;

    switch(pat->index[p].operand) {
      case B:
	elem = input_in_high_part ? &bh : &bl;
	break;
      case C:
	elem = input_in_high_part ? &ch : &cl;
	break;
      default:
	assert("This cannot happen!" == 0);
    }

    index = index > length ? index - length : index;

    uint64_t mask = -1;
    mask >>= (64 - elem_size);

    uint64_t tmp = ((*elem) >> (16 * index)) & mask;

    ret |= tmp << (elem_size * i);
  }

  return ret;
}



uint32_t
arc_pack_fpu_status(CPUARCState *env)
{    uint32_t ret = 0;
    uint8_t exp = get_float_exception_flags(&env->fp_status);

    BIT_WRITE(ret, IV, READ_EXCP_FLAG(exp, float_flag_invalid));
    BIT_WRITE(ret, DZ, READ_EXCP_FLAG(exp, float_flag_divbyzero));
    BIT_WRITE(ret, OF, READ_EXCP_FLAG(exp, float_flag_overflow));
    BIT_WRITE(ret, UF, READ_EXCP_FLAG(exp, float_flag_underflow));
    BIT_WRITE(ret, IX, READ_EXCP_FLAG(exp, float_flag_inexact));
    return ret;
}

static void
arc_unpack_fpu_status(CPUARCState *env, uint32_t val)
{
    uint32_t tmp = 0;
    WRITE_EXCP_FLAG(tmp, float_flag_invalid,	BIT_READ(val, IV));
    WRITE_EXCP_FLAG(tmp, float_flag_divbyzero,	BIT_READ(val, DZ));
    WRITE_EXCP_FLAG(tmp, float_flag_overflow,	BIT_READ(val, OF));
    WRITE_EXCP_FLAG(tmp, float_flag_underflow,	BIT_READ(val, UF));
    WRITE_EXCP_FLAG(tmp, float_flag_inexact,  	BIT_READ(val, IX));
    set_float_exception_flags(tmp, &env->fp_status);
}

target_ulong
arc_fpu_status_get_internal(CPUARCState *env)
{
    uint32_t ret = env->fp_status_persistent;
    return ret;
}

target_ulong
arc_fpu_status_get(const struct arc_aux_reg_detail *aux_reg_detail,
                   void *data)
{
    CPUARCState *env = (CPUARCState *) data;
    return arc_fpu_status_get_internal(env);
}

void
arc_fpu_status_set_interval(CPUARCState *env, target_ulong val)
{
    bool fwe_flag = BIT_READ(val, FWE);
    uint32_t flags = env->fp_status_persistent;

    if(fwe_flag == 0) {
        if(BIT_READ(val, CAL) == 1) {   /* CAL (clear all) is set */
	    flags = 0;
        } else {
	    flags = (flags & 0x1f) & (((~(val >> 8)) & 0x1f));
        }
    } else {
	flags = (flags & (~0x1f)) | (val & 0x1f);
    }
    env->fp_status_persistent = flags;
    arc_unpack_fpu_status(env, flags);
    qemu_log_mask(LOG_UNIMP, "FLAG IS SET TO 0x%08x after VAL = 0x%08x\n", (uint32_t) flags, (uint32_t) val);
}

void
arc_fpu_status_set(const struct arc_aux_reg_detail *aux_reg_detail,
		   target_ulong val, void *data)
{
    CPUARCState *env = (CPUARCState *) data;
    arc_fpu_status_set_interval(env, val);
}

enum arc_float_rounding_modes {
  arc_round_to_zero = 0,
  arc_round_nearest_even = 1,
  arc_round_up = 2,
  arc_round_down = 3
};

#define IVE   0
#define DZE   1


target_ulong
arc_fpu_ctrl_get_internal(CPUARCState *env)
{
    const uint32_t convertion[] = {
      1,    /* float_round_nearest_even = 0, */
      3,    /* float_round_down         = 1, */
      2,    /* float_round_up           = 2, */
      0,    /* float_round_to_zero      = 3, */
      42,   /* float_round_ties_away    = 4, */
      42,   /* float_round_to_odd       = 5, */
    };

    uint32_t ret = 0;
    uint32_t rmode = convertion[get_float_rounding_mode(&env->fp_status)];

    assert(rmode != 42);
    ret |= (rmode << 8);
    BIT_WRITE(ret, IVE, env->enable_invop_excp);
    BIT_WRITE(ret, DZE, env->enable_divzero_excp);
    return ret;
}
target_ulong
arc_fpu_ctrl_get(const struct arc_aux_reg_detail *aux_reg_detail,
		 void *data)
{
    CPUARCState *env = (CPUARCState *) data;
    return arc_fpu_ctrl_get_internal(env);
}

void
arc_fpu_ctrl_set_internal(CPUARCState *env, target_ulong val)
{
    const FloatRoundMode convertion[] = {
      float_round_to_zero,
      float_round_nearest_even,
      float_round_up,
      float_round_down
    };

    set_float_rounding_mode(convertion[(val >> 8) & 0x3], &env->fp_status);
    env->enable_invop_excp = BIT_READ(val, IVE);
    env->enable_divzero_excp = BIT_READ(val, DZE);
}
void
arc_fpu_ctrl_set(const struct arc_aux_reg_detail *aux_reg_detail,
		 target_ulong val, void *data)
{
    CPUARCState *env = (CPUARCState *) data;
    arc_fpu_ctrl_set_internal(env, val);
}
