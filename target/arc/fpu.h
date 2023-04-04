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
 * href="http://www.gnu.org/licenses/lgpl-2.1.html
 */

#ifndef ARC_FPU_H
#define ARC_FPU_H

#include "qemu/osdep.h"

/* FPU_CTRL Field Description */
#define FPU_CTRL_IVE   0  /* Enable Invalid Operation Exception */
#define FPU_CTRL_DZE   1  /* Enable Divide-by-Zero Exception */

/* FPU_STATUS Field Description */
#define FPU_STATUS_FWE     31   /* Flag Write Mode*/
#define FPU_STATUS_CIX     12   /* Clear Inexact Flag */
#define FPU_STATUS_CUF     11   /* Clear Underflow Flag */
#define FPU_STATUS_COF     10   /* Clear Overflow Flag */
#define FPU_STATUS_CDZ      9   /* Clear Divide By Zero Flag */
#define FPU_STATUS_CIV      8   /* Clear Invalid Operation Flag */
#define FPU_STATUS_CAL      7   /* Clear All Flags */
#define FPU_STATUS_IX       4   /* Inexact sticky flag */
#define FPU_STATUS_UF       3   /* Underflow sticky flag */
#define FPU_STATUS_OF       2   /* Overflow sticky flag */
#define FPU_STATUS_DZ       1   /* Divide By Zero sticky flag */
#define FPU_STATUS_IV       0   /* Invalid Operation sticky flag */

enum arc_float_rounding_modes {
    arc_round_to_zero = 0,
    arc_round_nearest_even = 1,
    arc_round_up = 2,
    arc_round_down = 3
};

extern uint8_t fpr_width;
extern uint8_t vfp_width;

/* Check if the current fpu status requires an exception to be raised */
void check_fpu_raise_exception(CPUARCState *env);

/* Initialize FPU initial values and flags */
void init_fpu(ARCCPU *cpu, bool fp_hp, bool fp_dp, bool fp_wide);

/*
 * Amount of elements in a vector is the configured vector width divided by the
 * operation size
 */
#define VLEN(SIZE) (vfp_width / SIZE)

/*
 * Return a mask of '1' from SIZE bits to 0. Special case for 64 bits is needed
 *  since 1 << 64 doesn't fit in a 64 bit calculation compiler throws warning.
 */
#define VEC_MASK(SIZE) (uint64_t) ((sizeof(uint64_t) == (SIZE >> 3)) ? \
                                   -1ull : ((1ull << SIZE) - 1))

/*                       Shuffle Instructions                       */

/* Register operands for shuffle operations */
enum shuffle_operand {
    B = 0,
    C
};

/*
 * First item is the "shuffle operand" and the second item is the
 * "shuffle index" based on a 16-bit element size
 */
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

/*
 * Available Shuffle patterns. Indicates how the final pattern looks like
 * The inputs are B3, B2, B1, B0 and C3, C2, C1, C0 if a second operand exists
 */
enum shuffle_pattern_enum {
    SHUFFLE_PAT_INV = -1,

    /* 32 bit vectors */
    V32_H_B0_B1,
    V32_H_C0_B0,
    V32_H_C1_B1,

    /* 64 bit vectors */
    V64_H_B2_B3_B0_B1,
    V64_S_B1B0_B3B2,

    V64_H_C2_C0_B2_B0,
    V64_H_C3_C1_B3_B1,
    V64_S_C1C0_B1B0,
    V64_S_C3C2_B3B2,

    V64_H_C1_B1_C0_B0,
    V64_H_C3_B3_C2_B2,

    V64_H_C2_B2_C0_B0,
    V64_H_C3_B3_C1_B1,

    /* 128bit wide vectors */
    V128_H_B6_B7_B4_B5_B2_B3_B0_B1,
    V128_S_B5B4_B7B6_B1B0_B3B2,
    V128_D_B3B2B1B0_B7B6B5B4,

    V128_H_C6_C4_C2_C0_B6_B4_B2_B0,
    V128_H_C7_C5_C3_C1_B7_B5_B3_B1,

    V128_S_C5C4_C1C0_B5B4_B1B0,
    V128_S_C7C6_C3C2_B7B6_B3B2,

    V128_D_C3C2C1C0_B3B2B1B0,
    V128_D_C7C6C5C4_B7B6B5B4,

    V128_H_C3_B3_C2_B2_C1_B1_C0_B0,
    V128_H_C7_B7_C6_B6_C5_B5_C4_B4,

    V128_S_C3C2_B3B2_C1C0_B1B0,
    V128_S_C7C6_B7B6_C5C4_B5B4,

    V128_H_C6_B6_C4_B4_C2_B2_C0_B0,
    V128_H_C7_B7_C5_B5_C3_B3_C1_B1,

    V128_S_C5C4_B5B4_C1C0_B1B0,
    V128_S_C7C6_B7B6_C3C2_B3B2,

    SHUFFLE_PATTERN_SIZE
};

/*
 * Defines a single pattern
 * Example: "shuffle_patterns_t pat = { false, 32, 16, 2, { C1, B1 } }"
 * "pat" is a 32 bit vector with 16 bit elements (2 elements)
 * Input order of operands is: " ... X4, X3, X2, X1, X0"
 * For "pat", this means the input operands {B1, B0} {C1, C0} would result in
 * an output of { C1, B1 }, C1 in position X1 and B1 in position X0
 */
struct shuffle_patterns_t {
    bool double_register;
    const unsigned char vector_size;
    const unsigned char elem_size;
    const unsigned char length;
    /*
     * Pattern of how input operands should be rearranged
     * index positions are 16 bit aligned (smallest possible element size),
     * element size is used to know where each element starts and ends for each
     * pattern
     */
    struct shuffle_replacement {
        const enum shuffle_operand operand;
        const unsigned char index;
    } index[8];
};

/* Definition of all patterns enumerated in shuffle_pattern_enum */
static const
struct shuffle_patterns_t shuffle_patterns[SHUFFLE_PATTERN_SIZE] = {
    [V32_H_B0_B1] = { false, 32, 16, 2, { B0, B1 } },
    [V32_H_C0_B0] = { false, 32, 16, 2, { C0, B0 } },
    [V32_H_C1_B1] = { false, 32, 16, 2, { C1, B1 } },

    /* 64 bit vectors */
    [V64_H_B2_B3_B0_B1] = { false, 64, 16, 4, { B2, B3, B0, B1 } },
    [V64_S_B1B0_B3B2]   = { false, 64, 32, 2, { B0, B2 } },

    [V64_H_C2_C0_B2_B0] = { false, 64, 16, 4, { C2, C0, B2, B0 } },
    [V64_H_C3_C1_B3_B1] = { false, 64, 16, 4, { C3, C1, B3, B1 } },
    [V64_S_C1C0_B1B0]       = { false, 64, 32, 2, { C0, B0 } },
    [V64_S_C3C2_B3B2]       = { false, 64, 32, 2, { C2, B2 } },

    [V64_H_C1_B1_C0_B0] = { false, 64, 16, 4, { C1, B1, C0, B0 } },
    [V64_H_C3_B3_C2_B2] = { false, 64, 16, 4, { C3, B3, C2, B2 } },

    [V64_H_C2_B2_C0_B0] = { false, 64, 16, 4, { C2, B2, C0, B0 } },
    [V64_H_C3_B3_C2_B2] = { false, 64, 16, 4, { C3, B3, C2, B2 } },

    /* 128bit wide vectors */
    [V128_H_B6_B7_B4_B5_B2_B3_B0_B1] = { true, 128, 16, 8, { B6, B7, B4, B5,
                                                             B2, B3, B0, B1 } },
    [V128_S_B5B4_B7B6_B1B0_B3B2]     = { true, 128, 32, 4, { B4, B6, B0, B2 } },
    [V128_D_B3B2B1B0_B7B6B5B4]       = { true, 128, 64, 2, { B0, B4 } },

    [V128_H_C6_C4_C2_C0_B6_B4_B2_B0] = { true, 128, 16, 8, { C6, C4, C2, C0,
                                                             B6, B4, B2, B0 } },
    [V128_H_C7_C5_C3_C1_B7_B5_B3_B1] = { true, 128, 16, 8, { C7, C5, C3, C1,
                                                             B7, B5, B3, B1 } },

    [V128_S_C5C4_C1C0_B5B4_B1B0] = { true, 128, 32, 4, { C4, C0, B4, B0 } },
    [V128_S_C7C6_C3C2_B7B6_B3B2] = { true, 128, 32, 4, { C6, C2, B6, B2 } },

    [V128_D_C3C2C1C0_B3B2B1B0] = { true, 128, 64, 2, { C0, B0 } },
    [V128_D_C7C6C5C4_B7B6B5B4] = { true, 128, 64, 2, { C4, B4 } },

    [V128_H_C3_B3_C2_B2_C1_B1_C0_B0] = { true, 128, 16, 8, { C3, B3, C2, B2,
                                                             C1, B1, C0, B0 } },
    [V128_H_C7_B7_C6_B6_C5_B5_C4_B4] = { true, 128, 16, 8, { C7, B7, C6, B6,
                                                             C5, B5, C4, B4 } },

    [V128_S_C3C2_B3B2_C1C0_B1B0] = { true, 128, 32, 4, { C2, B2, C0, B0 } },
    [V128_S_C7C6_B7B6_C5C4_B5B4] = { true, 128, 32, 4, { C6, B6, C4, B4 } },

    [V128_H_C6_B6_C4_B4_C2_B2_C0_B0] = { true, 128, 16, 8, { C6, B6, C4, B4,
                                                             C2, B2, C0, B0 } },
    [V128_H_C7_B7_C5_B5_C3_B3_C1_B1] = { true, 128, 16, 8, { C7, B7, C5, B5,
                                                             C3, B3, C1, B1 } },

    [V128_S_C5C4_B5B4_C1C0_B1B0] = { true, 128, 32, 4, { C4, B4, C0, B0 } },
    [V128_S_C7C6_B7B6_C3C2_B3B2] = { true, 128, 32, 4, { C6, B6, C2, B2 } }
};

/* Shuffle instructions */
enum shuffle_type_enum {
    HEXCH = 0,
    SEXCH,
    DEXCH,
    HUNPKL,
    HUNPKM,
    SUNPKL,
    SUNPKM,
    DUNPKL,
    DUNPKM,
    HPACKL,
    HPACKM,
    SPACKL,
    SPACKM,
    DPACKL,
    DPACKM,
    HBFLYL,
    HBFLYM,
    SBFLYL,
    SBFLYM,
    DBFLYL,
    DBFLYM,
    SHUFFLE_TYPE_SIZE
};

/*
 * Maps the specific instuction into 3 patterns, for 128, 64 and 32 bit
 * operations
 */
static const enum shuffle_pattern_enum shuffle_types[SHUFFLE_TYPE_SIZE][3] = {
    [HEXCH]  = { V128_H_B6_B7_B4_B5_B2_B3_B0_B1, V64_H_B2_B3_B0_B1, V32_H_B0_B1 },
    [SEXCH]  = { V128_S_B5B4_B7B6_B1B0_B3B2,     V64_S_B1B0_B3B2,   SHUFFLE_PAT_INV },
    [DEXCH]  = { V128_D_B3B2B1B0_B7B6B5B4,       SHUFFLE_PAT_INV,   SHUFFLE_PAT_INV },
    [HUNPKL] = { V128_H_C6_C4_C2_C0_B6_B4_B2_B0, V64_H_C2_C0_B2_B0, V32_H_C0_B0 },
    [HUNPKM] = { V128_H_C7_C5_C3_C1_B7_B5_B3_B1, V64_H_C3_C1_B3_B1, V32_H_C1_B1 },
    [SUNPKL] = { V128_S_C5C4_C1C0_B5B4_B1B0,     V64_S_C1C0_B1B0,   SHUFFLE_PAT_INV },
    [SUNPKM] = { V128_S_C7C6_C3C2_B7B6_B3B2,     V64_S_C3C2_B3B2,   SHUFFLE_PAT_INV },
    [DUNPKL] = { V128_D_C3C2C1C0_B3B2B1B0,       SHUFFLE_PAT_INV,   SHUFFLE_PAT_INV },
    [DUNPKM] = { V128_D_C7C6C5C4_B7B6B5B4,       SHUFFLE_PAT_INV,   SHUFFLE_PAT_INV },
    [HPACKL] = { V128_H_C3_B3_C2_B2_C1_B1_C0_B0, V64_H_C1_B1_C0_B0, V32_H_C0_B0 },
    [HPACKM] = { V128_H_C7_B7_C6_B6_C5_B5_C4_B4, V64_H_C3_B3_C2_B2, V32_H_C1_B1 },
    [SPACKL] = { V128_S_C3C2_B3B2_C1C0_B1B0,     V64_S_C1C0_B1B0,   SHUFFLE_PAT_INV },
    [SPACKM] = { V128_S_C7C6_B7B6_C5C4_B5B4,     V64_S_C3C2_B3B2,   SHUFFLE_PAT_INV },
    [DPACKL] = { V128_D_C3C2C1C0_B3B2B1B0,       SHUFFLE_PAT_INV,   SHUFFLE_PAT_INV },
    [DPACKM] = { V128_D_C7C6C5C4_B7B6B5B4,       SHUFFLE_PAT_INV,   SHUFFLE_PAT_INV },
    [HBFLYL] = { V128_H_C6_B6_C4_B4_C2_B2_C0_B0, V64_H_C2_B2_C0_B0, V32_H_C0_B0 },
    [HBFLYM] = { V128_H_C7_B7_C5_B5_C3_B3_C1_B1, V64_H_C3_B3_C1_B1, V32_H_C1_B1 },
    [SBFLYL] = { V128_S_C5C4_B5B4_C1C0_B1B0,     V64_S_C1C0_B1B0,   SHUFFLE_PAT_INV },
    [SBFLYM] = { V128_S_C7C6_B7B6_C3C2_B3B2,     V64_S_C3C2_B3B2,   SHUFFLE_PAT_INV },
    [DBFLYL] = { V128_D_C3C2C1C0_B3B2B1B0,       SHUFFLE_PAT_INV,   SHUFFLE_PAT_INV },
    [DBFLYM] = { V128_D_C7C6C5C4_B7B6B5B4,       SHUFFLE_PAT_INV,   SHUFFLE_PAT_INV },
};

#endif
