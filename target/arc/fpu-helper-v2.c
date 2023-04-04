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
#include "fpu.h"
#include "qemu/log.h"

enum arc_fcvt32 {
    ARC_FCVT32_FS2INT =     0b000011,
    ARC_FCVT32_FS2INT_RZ =  0b001011,
    ARC_FCVT32_FINT2S =     0b000010,
    ARC_FCVT32_FS2UINT =    0b000001,
    ARC_FCVT32_FS2UINT_RZ = 0b001001,
    ARC_FCVT32_FUINT2S =    0b000000,
    ARC_FCVT32_FH2S =       0b010101,
    ARC_FCVT32_FS2H =       0b010100
};

enum arc_fcvt32_64 {
    ARC_FCVT32_64_FS2L     = 0b000011,
    ARC_FCVT32_64_FS2L_RZ  = 0b001011,
    ARC_FCVT32_64_FS2UL    = 0b000001,
    ARC_FCVT32_64_FS2UL_RZ = 0b001001,
    ARC_FCVT32_64_FINT2D   = 0b000010,
    ARC_FCVT32_64_FUINT2D  = 0b000000,
    ARC_FCVT32_64_FS2D     = 0b000100
};

enum arc_fcvt64 {
    ARC_FCVT64_FD2L     = 0b000011,
    ARC_FCVT64_FD2L_RZ  = 0b001011,
    ARC_FCVT64_FL2D     = 0b000010,
    ARC_FCVT64_FD2UL    = 0b000001,
    ARC_FCVT64_FD2UL_RZ = 0b001001,
    ARC_FCVT64_FUL2D    = 0b000000
};

enum arc_fcvt64_32 {
    ARC_FCVT64_32_FD2INT     = 0b000011,
    ARC_FCVT64_32_FD2INT_RZ  = 0b001011,
    ARC_FCVT64_32_FD2UINT    = 0b000001,
    ARC_FCVT64_32_FD2UINT_RZ = 0b001001,
    ARC_FCVT64_32_FL2S       = 0b000010,
    ARC_FCVT64_32_FUL2S      = 0b000000,
    ARC_FCVT64_32_FD2S       = 0b000100
};

/* Soft fpu helper of type floatSIZE_operation */
#define FLOAT_INST3_HELPERS(NAME, HELPER, SIZE)                         \
uint32_t helper_##NAME(CPUARCState *env, uint32_t frs1, uint32_t frs2)  \
{                                                                       \
    uint32_t ret = float##SIZE##_##HELPER(frs1, frs2, &env->fp_status); \
    check_fpu_raise_exception(env);                                     \
    return ret;                                                         \
}

FLOAT_INST3_HELPERS(fsadd32, add, 32)
FLOAT_INST3_HELPERS(fssub32, sub, 32)
FLOAT_INST3_HELPERS(fsmul32, mul, 32)
FLOAT_INST3_HELPERS(fsdiv32, div, 32)
FLOAT_INST3_HELPERS(fsmin32, minnum, 32)
FLOAT_INST3_HELPERS(fsmax32, maxnum, 32)

/* Helper for the 32 bit Single precision floating point square root */
uint32_t helper_fssqrt32(CPUARCState *env, uint32_t src) {
    uint32_t ret = float32_sqrt(src, &env->fp_status);
    check_fpu_raise_exception(env);
    return ret;
}

/* Helper for the 32 bit - 32 bit conversions */
uint32_t helper_fcvt32(CPUARCState *env, uint32_t src, uint32_t operation) {
    uint32_t ret = 0;

    switch (operation) {

    case ARC_FCVT32_FS2INT:
        ret = float32_to_int32(src, &env->fp_status);
        break;

    case ARC_FCVT32_FS2INT_RZ:
        ret = float32_to_int32_round_to_zero(src, &env->fp_status);
        break;

    case ARC_FCVT32_FINT2S:
        ret = int32_to_float32(src, &env->fp_status);
        break;

    case ARC_FCVT32_FS2UINT:
        ret = float32_to_uint32(src, &env->fp_status);
        break;

    case ARC_FCVT32_FS2UINT_RZ:
        ret = float32_to_uint32_round_to_zero(src, &env->fp_status);
        break;

    case ARC_FCVT32_FUINT2S:
        ret = uint32_to_float32(src, &env->fp_status);
        break;

    case ARC_FCVT32_FH2S:
        error_report("Half-precision (FH2S) not implemented for arcv2!");
        exit(EXIT_FAILURE);
        break;

    case ARC_FCVT32_FS2H:
        error_report("Half-precision (FS2H) not implemented for arcv2!");
        exit(EXIT_FAILURE);
        break;

    default:
        error_report("FCVT32 operation not supported!");
        exit(EXIT_FAILURE);
    }

    check_fpu_raise_exception(env);

    return ret;
}

/* Helper for the 32 bit - 64 bit conversions */
uint64_t helper_fcvt32_64(CPUARCState *env, uint32_t src, uint32_t operation) {
    uint64_t ret = 0;

    switch (operation) {

    case ARC_FCVT32_64_FS2L:
        ret = float32_to_int64(src, &env->fp_status);
        break;

    case ARC_FCVT32_64_FS2L_RZ:
        ret = float32_to_int64_round_to_zero(src, &env->fp_status);
        break;

    case ARC_FCVT32_64_FS2UL:
        ret = float32_to_uint64(src, &env->fp_status);
        break;

    case ARC_FCVT32_64_FS2UL_RZ:
        ret = float32_to_uint64_round_to_zero(src, &env->fp_status);
        break;

    case ARC_FCVT32_64_FINT2D:
        ret = int32_to_float64(src, &env->fp_status);
        break;

    case ARC_FCVT32_64_FUINT2D:
        ret = uint32_to_float64(src, &env->fp_status);
        break;

    case ARC_FCVT32_64_FS2D:
        ret = float32_to_float64(src, &env->fp_status);
        break;

    default:

        error_report("FCVT32_64 operation not supported!");
        exit(EXIT_FAILURE);
    }

    check_fpu_raise_exception(env);

    return ret;
}

/* Helper for the 64 bit - 64 bit conversions */
uint64_t helper_fcvt64(CPUARCState *env, uint64_t src, uint32_t operation) {
    uint64_t ret = 0;

    switch (operation) {

    case ARC_FCVT64_FD2L:
        ret = float64_to_int64(src, &env->fp_status);
        break;

    case ARC_FCVT64_FD2L_RZ:
        ret = float64_to_int64_round_to_zero(src, &env->fp_status);
        break;

    case ARC_FCVT64_FL2D:
        ret = int64_to_float64(src, &env->fp_status);
        break;

    case ARC_FCVT64_FD2UL:
        ret = float64_to_uint64(src, &env->fp_status);
        break;

    case ARC_FCVT64_FD2UL_RZ:
        ret = float64_to_uint64_round_to_zero(src, &env->fp_status);
        break;

    case ARC_FCVT64_FUL2D:
        ret = uint64_to_float64(src, &env->fp_status);
        break;

    default:
        error_report("FCVT64 operation not supported!");
        exit(EXIT_FAILURE);
    }

    check_fpu_raise_exception(env);

    return ret;
}

/* Helper for the 64 bit - 32 bit conversions */
uint32_t helper_fcvt64_32(CPUARCState *env, uint64_t src, uint32_t operation) {
    uint32_t ret = 0;

    switch (operation) {

    case ARC_FCVT64_32_FD2INT:
        ret = float64_to_int32(src, &env->fp_status);
        break;

    case ARC_FCVT64_32_FD2INT_RZ:
        ret = float64_to_int32_round_to_zero(src, &env->fp_status);
        break;

    case ARC_FCVT64_32_FD2UINT:
        ret = float64_to_uint32(src, &env->fp_status);
        break;

    case ARC_FCVT64_32_FD2UINT_RZ:
        ret = float64_to_uint32_round_to_zero(src, &env->fp_status);
        break;

    case ARC_FCVT64_32_FL2S:
        ret = int64_to_float32(src, &env->fp_status);
        break;

    case ARC_FCVT64_32_FUL2S:
        ret = uint64_to_float32(src, &env->fp_status);
        break;

    case ARC_FCVT64_32_FD2S:
        ret = float64_to_float32(src, &env->fp_status);
        break;

    default:
        error_report("FCVT64_32 operation not supported!");
        exit(EXIT_FAILURE);
    }

    check_fpu_raise_exception(env);

    return ret;
}

