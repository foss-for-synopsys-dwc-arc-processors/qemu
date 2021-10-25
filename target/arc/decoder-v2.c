/*
 * QEMU Decoder for the ARC.
 * Copyright (C) 2020 Free Software Foundation, Inc.

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with GAS or GDB; see the file COPYING3. If not, write to
 * the Free Software Foundation, 51 Franklin Street - Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "qemu/osdep.h"
#include "target/arc/decoder.h"
#include "qemu/osdep.h"
#include "qemu/bswap.h"
#include "cpu.h"

/* Register names. */
static const char * const regnames[64] = {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
    "r24", "r25",
    "r26",
    "fp", "sp", "ilink", "r30", "blink",

    "r32", "r33", "r34", "r35", "r36", "r37", "r38", "r39",
    "r40", "r41", "r42", "r43", "r44", "r45", "r46", "r47",
    "r48", "r49", "r50", "r51", "r52", "r53", "r54", "r55",
    "r56", "r57", "r58", "r59", "lp_count", "rezerved", "LIMM", "pcl"
};

static const char *get_register_name(int value)
{
    return regnames[value];
}

/* Extract functions. */
static ATTRIBUTE_UNUSED int
extract_limm(unsigned long long insn ATTRIBUTE_UNUSED,
             bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  return value;
}

/* mask = 00000000000000000000111111000000. */
static long long int
extract_uimm6_20(unsigned long long insn ATTRIBUTE_UNUSED,
                 bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x003f) << 0;

  return value;
}

/* mask = 00000000000000000000111111222222. */
static long long int
extract_simm12_20(unsigned long long insn ATTRIBUTE_UNUSED,
                  bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 6) & 0x003f) << 0;
  value |= ((insn >> 0) & 0x003f) << 6;

  /* Extend the sign. */
  int signbit = 1 << (12 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 0000011100000000. */
static ATTRIBUTE_UNUSED int
extract_simm3_5_s(unsigned long long insn ATTRIBUTE_UNUSED,
                  bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 8) & 0x0007) << 0;

  /* Extend the sign. */
  int signbit = 1 << (3 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

static ATTRIBUTE_UNUSED int
extract_limm_s(unsigned long long insn ATTRIBUTE_UNUSED,
               bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  return value;
}

/* mask = 0000000000011111. */
static long long int
extract_uimm7_a32_11_s(unsigned long long insn ATTRIBUTE_UNUSED,
                       bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x001f) << 2;

  return value;
}

/* mask = 0000000001111111. */
static long long int
extract_uimm7_9_s(unsigned long long insn ATTRIBUTE_UNUSED,
                  bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x007f) << 0;

  return value;
}

/* mask = 0000000000000111. */
static long long int
extract_uimm3_13_s(unsigned long long insn ATTRIBUTE_UNUSED,
                   bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x0007) << 0;

  return value;
}

/* mask = 0000000111111111. */
static long long int
extract_simm11_a32_7_s(unsigned long long insn ATTRIBUTE_UNUSED,
                       bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x01ff) << 2;

  /* Extend the sign. */
  int signbit = 1 << (11 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 0000000002220111. */
static long long int
extract_uimm6_13_s(unsigned long long insn ATTRIBUTE_UNUSED,
                   bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x0007) << 0;
  value |= ((insn >> 4) & 0x0007) << 3;

  return value;
}

/* mask = 0000000000011111. */
static long long int
extract_uimm5_11_s(unsigned long long insn ATTRIBUTE_UNUSED,
                   bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x001f) << 0;

  return value;
}

/* mask = 00000000111111102000000000000000. */
static long long int
extract_simm9_a16_8(unsigned long long insn ATTRIBUTE_UNUSED,
                    bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 17) & 0x007f) << 1;
  value |= ((insn >> 15) & 0x0001) << 8;

  /* Extend the sign. */
  int signbit = 1 << (9 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 00000000000000000000111111000000. */
static long long int
extract_uimm6_8(unsigned long long insn ATTRIBUTE_UNUSED,
                bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x003f) << 0;

  return value;
}

/* mask = 00000111111111102222222222000000. */
static long long int
extract_simm21_a16_5(unsigned long long insn ATTRIBUTE_UNUSED,
                     bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 17) & 0x03ff) << 1;
  value |= ((insn >> 6) & 0x03ff) << 11;

  /* Extend the sign. */
  int signbit = 1 << (21 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 00000111111111102222222222003333. */
static long long int
extract_simm25_a16_5(unsigned long long insn ATTRIBUTE_UNUSED,
                     bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 17) & 0x03ff) << 1;
  value |= ((insn >> 6) & 0x03ff) << 11;
  value |= ((insn >> 0) & 0x000f) << 21;

  /* Extend the sign. */
  int signbit = 1 << (25 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 0000000111111111. */
static long long int
extract_simm10_a16_7_s(unsigned long long insn ATTRIBUTE_UNUSED,
                       bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x01ff) << 1;

  /* Extend the sign. */
  int signbit = 1 << (10 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 0000000000111111. */
static long long int
extract_simm7_a16_10_s(unsigned long long insn ATTRIBUTE_UNUSED,
                       bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x003f) << 1;

  /* Extend the sign. */
  int signbit = 1 << (7 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 00000111111111002222222222000000. */
static long long int
extract_simm21_a32_5(unsigned long long insn ATTRIBUTE_UNUSED,
                     bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 18) & 0x01ff) << 2;
  value |= ((insn >> 6) & 0x03ff) << 11;

  /* Extend the sign. */
  int signbit = 1 << (21 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 00000111111111002222222222003333. */
static long long int
extract_simm25_a32_5(unsigned long long insn ATTRIBUTE_UNUSED,
                     bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 18) & 0x01ff) << 2;
  value |= ((insn >> 6) & 0x03ff) << 11;
  value |= ((insn >> 0) & 0x000f) << 21;

  /* Extend the sign. */
  int signbit = 1 << (25 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 0000011111111111. */
static long long int
extract_simm13_a32_5_s(unsigned long long insn ATTRIBUTE_UNUSED,
                       bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x07ff) << 2;

  /* Extend the sign. */
  int signbit = 1 << (13 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 0000000001111111. */
static long long int
extract_simm8_a16_9_s(unsigned long long insn ATTRIBUTE_UNUSED,
                      bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x007f) << 1;

  /* Extend the sign. */
  int signbit = 1 << (8 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 00000000000000000000000111000000. */
static long long int
extract_uimm3_23(unsigned long long insn ATTRIBUTE_UNUSED,
                 bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x0007) << 0;

  return value;
}

/* mask = 0000001111111111. */
static long long int
extract_uimm10_6_s(unsigned long long insn ATTRIBUTE_UNUSED,
                   bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x03ff) << 0;

  return value;
}

/* mask = 0000002200011110. */
static long long int
extract_uimm6_11_s(unsigned long long insn ATTRIBUTE_UNUSED,
                   bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 1) & 0x000f) << 0;
  value |= ((insn >> 8) & 0x0003) << 4;

  return value;
}

/* mask = 00000000111111112000000000000000. */
static long long int
extract_simm9_8(unsigned long long insn ATTRIBUTE_UNUSED,
                bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 16) & 0x00ff) << 0;
  value |= ((insn >> 15) & 0x0001) << 8;

  /* Extend the sign. */
  int signbit = 1 << (9 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 0000000011111111. */
static long long int
extract_uimm10_a32_8_s(unsigned long long insn ATTRIBUTE_UNUSED,
                       bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x00ff) << 2;

  return value;
}

/* mask = 0000000111111111. */
static long long int
extract_simm9_7_s(unsigned long long insn ATTRIBUTE_UNUSED,
                  bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x01ff) << 0;

  /* Extend the sign. */
  int signbit = 1 << (9 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 0000000000011111. */
static long long int
extract_uimm6_a16_11_s(unsigned long long insn ATTRIBUTE_UNUSED,
                       bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x001f) << 1;

  return value;
}

/* mask = 0000020000011000. */
static long long int
extract_uimm5_a32_11_s(unsigned long long insn ATTRIBUTE_UNUSED,
                       bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 3) & 0x0003) << 2;
  value |= ((insn >> 10) & 0x0001) << 4;

  return value;
}

/* mask = 0000022222200111. */
static long long int
extract_simm11_a32_13_s(unsigned long long insn ATTRIBUTE_UNUSED,
                        bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x0007) << 2;
  value |= ((insn >> 5) & 0x003f) << 5;

  /* Extend the sign. */
  int signbit = 1 << (11 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 0000000022220111. */
static long long int
extract_uimm7_13_s(unsigned long long insn ATTRIBUTE_UNUSED,
                   bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x0007) << 0;
  value |= ((insn >> 4) & 0x000f) << 3;

  return value;
}

/* mask = 00000000000000000000011111000000. */
static long long int
extract_uimm6_a16_21(unsigned long long insn ATTRIBUTE_UNUSED,
                     bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x001f) << 1;

  return value;
}

/* mask = 0000022200011110. */
static long long int
extract_uimm7_11_s(unsigned long long insn ATTRIBUTE_UNUSED,
                   bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 1) & 0x000f) << 0;
  value |= ((insn >> 8) & 0x0007) << 4;

  return value;
}

/* mask = 00000000000000000000111111000000. */
static long long int
extract_uimm7_a16_20(unsigned long long insn ATTRIBUTE_UNUSED,
                     bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x003f) << 1;

  return value;
}

/* mask = 00000000000000000000111111222222. */
static long long int
extract_simm13_a16_20(unsigned long long insn ATTRIBUTE_UNUSED,
                      bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 6) & 0x003f) << 1;
  value |= ((insn >> 0) & 0x003f) << 7;

  /* Extend the sign. */
  int signbit = 1 << (13 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

/* mask = 0000000011111111. */
static long long int
extract_uimm8_8_s(unsigned long long insn ATTRIBUTE_UNUSED,
                  bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x00ff) << 0;

  return value;
}

/* mask = 0000011111100000. */
static long long int
extract_uimm6_5_s(unsigned long long insn ATTRIBUTE_UNUSED,
                  bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 5) & 0x003f) << 0;

  return value;
}

/* mask = 00000000000000000000000000000000. */
static ATTRIBUTE_UNUSED int
extract_uimm6_axx_(unsigned long long insn ATTRIBUTE_UNUSED,
                   bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  return value;
}

static long long int extract_rb(unsigned long long insn ATTRIBUTE_UNUSED,
                                bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    int value = (((insn >> 12) & 0x07) << 3) | ((insn >> 24) & 0x07);

    if (value == 0x3e && invalid) {
        *invalid = TRUE;
    }

    return value;
}

static long long int extract_rhv1(unsigned long long insn ATTRIBUTE_UNUSED,
                                  bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    int value = ((insn & 0x7) << 3) | ((insn >> 5) & 0x7);

    return value;
}

static long long int extract_rhv2(unsigned long long insn ATTRIBUTE_UNUSED,
                                  bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    int value = ((insn >> 5) & 0x07) | ((insn & 0x03) << 3);

    return value;
}

static long long int extract_r0(unsigned long long insn ATTRIBUTE_UNUSED,
                                bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return 0;
}

static long long int extract_r1(unsigned long long insn ATTRIBUTE_UNUSED,
                                bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return 1;
}

static long long int extract_r2(unsigned long long insn ATTRIBUTE_UNUSED,
                                bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return 2;
}

static long long int extract_r3(unsigned long long insn ATTRIBUTE_UNUSED,
                                bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return 3;
}

static long long int extract_sp(unsigned long long insn ATTRIBUTE_UNUSED,
                                bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return 28;
}

static long long int extract_gp(unsigned long long insn ATTRIBUTE_UNUSED,
                                bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return 26;
}

static long long int extract_pcl(unsigned long long insn ATTRIBUTE_UNUSED,
                                 bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return 63;
}

static long long int extract_blink(unsigned long long insn ATTRIBUTE_UNUSED,
                                   bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return 31;
}

static long long int extract_ilink1(unsigned long long insn ATTRIBUTE_UNUSED,
                                    bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return 29;
}

static long long int extract_ilink2(unsigned long long insn ATTRIBUTE_UNUSED,
                                    bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return 30;
}

static long long int extract_ras(unsigned long long insn ATTRIBUTE_UNUSED,
                                 bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    int value = insn & 0x07;
    if (value > 3) {
        return value + 8;
    } else {
        return value;
    }
}

static long long int extract_rbs(unsigned long long insn ATTRIBUTE_UNUSED,
                                 bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    int value = (insn >> 8) & 0x07;
    if (value > 3) {
        return value + 8;
    } else {
        return value;
    }
}

static long long int extract_rcs(unsigned long long insn ATTRIBUTE_UNUSED,
                                 bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    int value = (insn >> 5) & 0x07;
    if (value > 3) {
        return value + 8;
    } else {
        return value;
    }
}

static long long int extract_simm3s(unsigned long long insn ATTRIBUTE_UNUSED,
                                    bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    int value = (insn >> 8) & 0x07;
    if (value == 7) {
        return -1;
    } else {
        return value;
    }
}

static long long int extract_rrange(unsigned long long insn  ATTRIBUTE_UNUSED,
                                    bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return (insn >> 1) & 0x0F;
}

static long long int extract_fpel(unsigned long long insn ATTRIBUTE_UNUSED,
                                  bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return (insn & 0x0100) ? 27 : -1;
}

static long long int extract_blinkel(unsigned long long insn ATTRIBUTE_UNUSED,
                                     bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return (insn & 0x0200) ? 31 : -1;
}

static long long int extract_pclel(unsigned long long insn ATTRIBUTE_UNUSED,
                                   bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    return (insn & 0x0400) ? 63 : -1;
}

static long long int extract_w6(unsigned long long insn ATTRIBUTE_UNUSED,
                                bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    signed value = 0;

    value |= ((insn >> 6) & 0x003f) << 0;

    int signbit = 1 << 5;
    value = (value ^ signbit) - signbit;

    return value;
}

static long long int extract_g_s(unsigned long long insn ATTRIBUTE_UNUSED,
                                 bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    int value = 0;

    value |= ((insn >> 8) & 0x0007) << 0;
    value |= ((insn >> 3) & 0x0003) << 3;

    /* Extend the sign. */
    int signbit = 1 << (6 - 1);
    value = (value ^ signbit) - signbit;

    return value;
}

static long long int extract_uimm12_20(unsigned long long insn ATTRIBUTE_UNUSED,
                                       bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
    int value = 0;

    value |= ((insn >> 6) & 0x003f) << 0;
    value |= ((insn >> 0) & 0x003f) << 6;

    return value;
}

/*
 * The operands table.
 *
 * The format of the operands table is:
 *
 * BITS SHIFT FLAGS EXTRACT_FUN.
 */
const struct arc_operand arc_operands[] = {
    { 0, 0, 0, 0 },
#define ARC_OPERAND(NAME, BITS, SHIFT, RELO, FLAGS, FUN)       \
    { BITS, SHIFT, FLAGS, FUN },
#include "target/arc/operands.def"
#undef ARC_OPERAND
    { 0, 0, 0, 0}
};

enum arc_operands_map {
    OPERAND_UNUSED = 0,
#define ARC_OPERAND(NAME, BITS, SHIFT, RELO, FLAGS, FUN) OPERAND_##NAME,
#include "target/arc/operands.def"
#undef ARC_OPERAND
    OPERAND_LAST
};

/*
 * The flag operands table.
 *
 * The format of the table is
 * NAME CODE BITS SHIFT FAVAIL.
 */
const struct arc_flag_operand arc_flag_operands[] = {
    { 0, 0, 0, 0, 0},
#define ARC_FLAG(NAME, MNEMONIC, CODE, BITS, SHIFT, AVAIL)      \
    { MNEMONIC, CODE, BITS, SHIFT, AVAIL },
#include "target/arc/flags.def"
#undef ARC_FLAG
    { 0, 0, 0, 0, 0}
};

enum arc_flags_map {
    F_NULL = 0,
#define ARC_FLAG(NAME, MNEMONIC, CODE, BITS, SHIFT, AVAIL) F_##NAME,
#include "target/arc/flags.def"
#undef ARC_FLAG
    F_LAST
};

/*
 * Table of the flag classes.
 *
 * The format of the table is
 * CLASS {FLAG_CODE}.
 */
const struct arc_flag_class arc_flag_classes[] = {
#define C_EMPTY             0
    { F_CLASS_NONE, { F_NULL }, 0},

#define C_CC_EQ             (C_EMPTY + 1)
    {F_CLASS_IMPLICIT | F_CLASS_COND, {F_EQUAL, F_NULL}, 0},

#define C_CC_GE             (C_CC_EQ + 1)
    {F_CLASS_IMPLICIT | F_CLASS_COND, {F_GE, F_NULL}, 0},

#define C_CC_GT             (C_CC_GE + 1)
    {F_CLASS_IMPLICIT | F_CLASS_COND, {F_GT, F_NULL}, 0},

#define C_CC_HI             (C_CC_GT + 1)
    {F_CLASS_IMPLICIT | F_CLASS_COND, {F_HI, F_NULL}, 0},

#define C_CC_HS             (C_CC_HI + 1)
    {F_CLASS_IMPLICIT | F_CLASS_COND, {F_NOTCARRY, F_NULL}, 0},

#define C_CC_LE             (C_CC_HS + 1)
    {F_CLASS_IMPLICIT | F_CLASS_COND, {F_LE, F_NULL}, 0},

#define C_CC_LO             (C_CC_LE + 1)
    {F_CLASS_IMPLICIT | F_CLASS_COND, {F_CARRY, F_NULL}, 0},

#define C_CC_LS             (C_CC_LO + 1)
    {F_CLASS_IMPLICIT | F_CLASS_COND, {F_LS, F_NULL}, 0},

#define C_CC_LT             (C_CC_LS + 1)
    {F_CLASS_IMPLICIT | F_CLASS_COND, {F_LT, F_NULL}, 0},

#define C_CC_NE             (C_CC_LT + 1)
    {F_CLASS_IMPLICIT | F_CLASS_COND, {F_NOTEQUAL, F_NULL}, 0},

#define C_AA_AB             (C_CC_NE + 1)
    {F_CLASS_IMPLICIT | F_CLASS_WB, {F_AB3, F_NULL}, 0},

#define C_AA_AW             (C_AA_AB + 1)
    {F_CLASS_IMPLICIT | F_CLASS_WB, {F_AW3, F_NULL}, 0},

#define C_ZZ_D              (C_AA_AW + 1)
    {F_CLASS_IMPLICIT | F_CLASS_ZZ, {F_SIZED, F_NULL}, 0},

#define C_ZZ_H              (C_ZZ_D + 1)
    {F_CLASS_IMPLICIT | F_CLASS_ZZ, {F_H1, F_NULL}, 0},

#define C_ZZ_B              (C_ZZ_H + 1)
    {F_CLASS_IMPLICIT | F_CLASS_ZZ, {F_SIZEB1, F_NULL}, 0},

#define C_CC                (C_ZZ_B + 1)
    { F_CLASS_OPTIONAL | F_CLASS_EXTEND | F_CLASS_COND,
        { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL,
          F_NOTZERO, F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS,
          F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
          F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW,
          F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT,
          F_LE, F_HI, F_LS, F_PNZ, F_NULL
        },
      0
    },

#define C_AA_ADDR3          (C_CC + 1)
#define C_AA27              (C_CC + 1)
    { F_CLASS_OPTIONAL | F_CLASS_WB, { F_A3, F_AW3, F_AB3, F_AS3, F_NULL }, 0},
#define C_AA_ADDR9          (C_AA_ADDR3 + 1)
#define C_AA21              (C_AA_ADDR3 + 1)
    { F_CLASS_OPTIONAL | F_CLASS_WB, { F_A9, F_AW9, F_AB9, F_AS9, F_NULL }, 0},
#define C_AA_ADDR22         (C_AA_ADDR9 + 1)
#define C_AA8               (C_AA_ADDR9 + 1)
    { F_CLASS_OPTIONAL | F_CLASS_WB,
      { F_A22, F_AW22, F_AB22, F_AS22, F_NULL },
      0
    },

#define C_F                 (C_AA_ADDR22 + 1)
    { F_CLASS_OPTIONAL | F_CLASS_F, { F_FLAG, F_NULL }, 0},
#define C_FHARD             (C_F + 1)
    { F_CLASS_OPTIONAL | F_CLASS_F, { F_FFAKE, F_NULL }, 0},

#define C_T                 (C_FHARD + 1)
    { F_CLASS_OPTIONAL, { F_NT, F_T, F_NULL }, 0},
#define C_D                 (C_T + 1)
    { F_CLASS_OPTIONAL | F_CLASS_D, { F_ND, F_D, F_NULL }, 0},
#define C_DNZ_D             (C_D + 1)
    { F_CLASS_OPTIONAL | F_CLASS_D, { F_DNZ_ND, F_DNZ_D, F_NULL }, 0},

#define C_DHARD             (C_DNZ_D + 1)
    { F_CLASS_OPTIONAL | F_CLASS_D, { F_DFAKE, F_NULL }, 0},

#define C_DI20              (C_DHARD + 1)
    { F_CLASS_OPTIONAL | F_CLASS_DI, { F_DI11, F_NULL }, 0},
#define C_DI14              (C_DI20 + 1)
    { F_CLASS_OPTIONAL | F_CLASS_DI, { F_DI14, F_NULL }, 0},
#define C_DI16              (C_DI14 + 1)
    { F_CLASS_OPTIONAL | F_CLASS_DI, { F_DI15, F_NULL }, 0},
#define C_DI26              (C_DI16 + 1)
    { F_CLASS_OPTIONAL | F_CLASS_DI, { F_DI5, F_NULL }, 0},

#define C_X25               (C_DI26 + 1)
    { F_CLASS_OPTIONAL | F_CLASS_X, { F_SIGN6, F_NULL }, 0},
#define C_X15               (C_X25 + 1)
    { F_CLASS_OPTIONAL | F_CLASS_X, { F_SIGN16, F_NULL }, 0},
#define C_XHARD             (C_X15 + 1)
#define C_X                 (C_X15 + 1)
    { F_CLASS_OPTIONAL | F_CLASS_X, { F_SIGNX, F_NULL }, 0},

#define C_ZZ13              (C_X + 1)
    { F_CLASS_OPTIONAL | F_CLASS_ZZ, { F_SIZEB17, F_SIZEW17, F_H17, F_NULL}, 0},
#define C_ZZ23              (C_ZZ13 + 1)
    { F_CLASS_OPTIONAL | F_CLASS_ZZ, { F_SIZEB7, F_SIZEW7, F_H7, F_NULL}, 0},
#define C_ZZ29              (C_ZZ23 + 1)
    { F_CLASS_OPTIONAL | F_CLASS_ZZ, { F_SIZEB1, F_SIZEW1, F_H1, F_NULL}, 0},

#define C_AS                (C_ZZ29 + 1)
    { F_CLASS_IMPLICIT | F_CLASS_OPTIONAL | F_CLASS_WB, { F_ASFAKE, F_NULL}, 0},

#define C_NE                (C_AS + 1)
    { F_CLASS_OPTIONAL | F_CLASS_COND, { F_NE, F_NULL}, 0},
};

/* List with special cases instructions and the applicable flags. */
const struct arc_flag_special arc_flag_special_cases[] = {
    { "b",  { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO,
              F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET,
              F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET,
              F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT,
              F_LE, F_HI, F_LS, F_PNZ, F_NULL
            }
    },
    { "bl", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO,
              F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET,
              F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET,
              F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT,
              F_LE, F_HI, F_LS, F_PNZ, F_NULL
            }
    },
    { "br", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO,
              F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET,
              F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET,
              F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT,
              F_LE, F_HI, F_LS, F_PNZ, F_NULL
            }
    },
    { "j",  { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO,
              F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET,
              F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET,
              F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT,
              F_LE, F_HI, F_LS, F_PNZ, F_NULL
            }
    },
    { "jl", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO,
              F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET,
              F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET,
              F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT,
              F_LE, F_HI, F_LS, F_PNZ, F_NULL
            }
    },
    { "lp", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO,
              F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET,
              F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET,
              F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT,
              F_LE, F_HI, F_LS, F_PNZ, F_NULL
            }
    },
    { "set", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO,
               F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET,
               F_LOWER, F_CARRYCLR, F_NOTCARRY, F_HIGHER, F_OVERFLOWSET,
               F_OVERFLOW, F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT,
               F_LE, F_HI, F_LS, F_PNZ, F_NULL
             }
    },
    { "ld", { F_SIZEB17, F_SIZEW17, F_H17, F_NULL } },
    { "st", { F_SIZEB1, F_SIZEW1, F_H1, F_NULL } }
};

const unsigned arc_num_flag_special = ARRAY_SIZE(arc_flag_special_cases);

/*
 * The opcode table.
 *
 * The format of the opcode table is:
 *
 * NAME OPCODE MASK CPU CLASS SUBCLASS { OPERANDS } { FLAGS }.
 *
 * The table is organised such that, where possible, all instructions with
 * the same mnemonic are together in a block. When the assembler searches
 * for a suitable instruction the entries are checked in table order, so
 * more specific, or specialised cases should appear earlier in the table.
 *
 * As an example, consider two instructions 'add a,b,u6' and 'add
 * a,b,limm'. The first takes a 6-bit immediate that is encoded within the
 * 32-bit instruction, while the second takes a 32-bit immediate that is
 * encoded in a follow-on 32-bit, making the total instruction length
 * 64-bits. In this case the u6 variant must appear first in the table, as
 * all u6 immediates could also be encoded using the 'limm' extension,
 * however, we want to use the shorter instruction wherever possible.
 *
 * It is possible though to split instructions with the same mnemonic into
 * multiple groups. However, the instructions are still checked in table
 * order, even across groups. The only time that instructions with the
 * same mnemonic should be split into different groups is when different
 * variants of the instruction appear in different architectures, in which
 * case, grouping all instructions from a particular architecture together
 * might be preferable to merging the instruction into the main instruction
 * table.
 *
 * An example of this split instruction groups can be found with the 'sync'
 * instruction. The core arc architecture provides a 'sync' instruction,
 * while the nps instruction set extension provides 'sync.rd' and
 * 'sync.wr'. The rd/wr flags are instruction flags, not part of the
 * mnemonic, so we end up with two groups for the sync instruction, the
 * first within the core arc instruction table, and the second within the
 * nps extension instructions.
 */

#define OPCODE(...)
#define OPERANDS_LIST(NAME, COUNT, ...) \
  { __VA_ARGS__ },
#define FLAGS_LIST(...)
#define MNEMONIC(...)
static unsigned char operands[OPERANDS_LIST_SIZE+1][MAX_INSN_ARGS] = {
#include "target/arc/opcodes.def"
    { 0 } /* NO OPERANDS */
};
#undef OPCODE
#undef OPERANDS_LIST
#undef FLAGS_LIST
#undef MNEMONIC

#define OPCODE(NAME, INSN, MASK, ARCH, CLASS, SUBCLASS, OPERANDS, FLAGS, TEMPLATE, ASM_TEMPLATE) \
    ASM_TEMPLATE,
#define OPERANDS_LIST(...)
#define FLAGS_LIST(...)
#define MNEMONIC(...)
const char *opcode_name_str[OPCODE_SIZE] = {
    #include "target/arc/opcodes.def"
};
#undef OPCODE
#undef OPERANDS_LIST
#undef FLAGS_LIST
#undef MNEMONIC

#define OPCODE(...)
#define OPERANDS_LIST(...)
#define FLAGS_LIST(NAME, COUNT, ...) \
  { __VA_ARGS__ },
#define MNEMONIC(...)
static unsigned char flags_arc[FLAGS_SIZE+1][MAX_INSN_FLGS] = {
#include "target/arc/opcodes.def"
    { 0 }
};
#undef OPCODE
#undef OPERANDS_LIST
#undef FLAGS_LIST
#undef MNEMONIC

/* Opcode debug strings */
#define OPCODE(NAME, INSN, MASK, ARCH, CLASS, SUBCLASS, OPERANDS, FLAGS, ...) \
    "OPCODE_" #ARCH "_" #NAME "_" #INSN "_" #MASK "_" #CLASS "_" #OPERANDS,
#define OPERANDS_LIST(...)
#define FLAGS_LIST(...)
#define MNEMONIC(...)
const char *opcode_enum_strs[OPCODE_SIZE] = {
    #include "opcodes.def"
};
#undef OPCODE
#undef OPERANDS_LIST
#undef FLAGS_LIST
#undef MNEMONIC

#define OPCODE(NAME, OPCODE, MASK, ARCH, CLASS, SUBCLASS, OPS, FLGS, ...) { \
  .name = #NAME, \
  .mnemonic = MNEMONIC_##NAME, \
  .opcode = OPCODE, \
  .mask = MASK, \
  .cpu = ARC_OPCODE_##ARCH, \
  .insn_class = CLASS, \
  .subclass = SUBCLASS, \
  .operands = operands[OPERANDS_##OPS], \
  .flags = flags_arc[FLAGS_##FLGS] \
},
#define OPERANDS_LIST(...)
#define FLAGS_LIST(...)
#define MNEMONIC(...)
const struct arc_opcode arc_opcodes[OPCODE_SIZE+1] = {
#include "target/arc/opcodes.def"
    { NULL, 0, 0, 0, 0, 0, 0, 0 }
};
#undef OPCODE
#undef OPERANDS_LIST
#undef FLAGS_LIST
#undef MNEMONIC

#include "decode_tree.h"

static insn_t
load_insninfo_if_valid(uint64_t insn,
                       uint8_t insn_len,
                       uint32_t isa_mask,
                       bool *invalid,
                       const struct arc_opcode *opcode)
{
    const uint8_t *opidx;
    const uint8_t *flgidx;
    bool has_limm = false;
    insn_t ret;

    *invalid = false;
    uint32_t noperands = 0;

    has_limm = false;
    noperands = 0;
    memset(&ret, 0, sizeof(insn_t));

    /* Possible candidate, check the operands. */
    for (opidx = opcode->operands; *opidx; ++opidx) {
        int value, limmind;
        const struct arc_operand *operand = &arc_operands[*opidx];

        if (operand->flags & ARC_OPERAND_FAKE) {
            continue;
        }

        if (operand->extract) {
            value = (*operand->extract)(insn, invalid);
        } else {
            value = (insn >> operand->shift) & ((1 << operand->bits) - 1);
        }

        /*
         * Check for LIMM indicator. If it is there, then make sure
         * we pick the right format.
         */
        limmind = (isa_mask & ARC_OPCODE_ARCV2) ? 0x1E : 0x3E;
        if (operand->flags & ARC_OPERAND_IR &&
            !(operand->flags & ARC_OPERAND_LIMM)) {
            if ((value == 0x3E && insn_len == 4) ||
                (value == limmind && insn_len == 2)) {
                *invalid = TRUE;
                break;
            }
        }

        if (operand->flags & ARC_OPERAND_LIMM &&
            !(operand->flags & ARC_OPERAND_DUPLICATE)) {
            has_limm = true;
            if (operand->flags & ARC_OPERAND_16_SPLIT) {
                ret.limm_split_16_p = true;
            }
        } else {
            if (operand->flags & ARC_OPERAND_16_SPLIT) {
                value &= 0x0000ffff;
                value |= (value << 16);
            }
        }

        ret.operands[noperands].value = value;
        ret.operands[noperands].type = operand->flags;
        noperands += 1;
        ret.n_ops = noperands;
    }

    /* Check the flags. */
    for (flgidx = opcode->flags; *flgidx; ++flgidx) {
        /* Get a valid flag class. */
        const struct arc_flag_class *cl_flags = &arc_flag_classes[*flgidx];
        const unsigned *flgopridx;
        bool foundA = false, foundB = false;
        unsigned int value;

        /* FIXME! Add check for EXTENSION flags. */

        for (flgopridx = cl_flags->flags; *flgopridx; ++flgopridx) {
            const struct arc_flag_operand *flg_operand =
            &arc_flag_operands[*flgopridx];

            /* Check for the implicit flags. */
            if (cl_flags->flag_class & F_CLASS_IMPLICIT) {
                if (cl_flags->flag_class & F_CLASS_COND) {
                    ret.cc = flg_operand->code;
                } else if (cl_flags->flag_class & F_CLASS_WB) {
                    ret.aa = flg_operand->code;
                } else if (cl_flags->flag_class & F_CLASS_ZZ) {
                    ret.zz = flg_operand->code;
                }
                continue;
            }

            if (cl_flags->extract) {
                value = (*cl_flags->extract)(insn);
            } else {
                value = (insn >> flg_operand->shift) &
                        ((1 << flg_operand->bits) - 1);
            }

            if (value == flg_operand->code) {
                if (cl_flags->flag_class & F_CLASS_ZZ) {
                    switch (flg_operand->name[0]) {
                    case 'b':
                        ret.zz = 1;
                        break;
                    case 'h':
                    case 'w':
                        ret.zz = 2;
                        break;
                    default:
                        ret.zz = 4;
                        break;
                    }
                }

                /*
                 * TODO: This has a problem: instruction "b label"
                 * sets this to true.
                 */
                if (cl_flags->flag_class & F_CLASS_D) {
                    ret.d = value ? true : false;
                    if (cl_flags->flags[0] == F_DFAKE) {
                        ret.d = true;
                    }
                }

                if (cl_flags->flag_class & F_CLASS_COND) {
                    ret.cc = value;
                }

                if (cl_flags->flag_class & F_CLASS_WB) {
                    ret.aa = value;
                }

                if (cl_flags->flag_class & F_CLASS_F) {
                    ret.f = true;
                }

                if (cl_flags->flag_class & F_CLASS_DI) {
                    ret.di = true;
                }

                if (cl_flags->flag_class & F_CLASS_X) {
                    ret.x = true;
                }

                foundA = true;
            }
            if (value) {
                foundB = true;
            }
        }

        if (!foundA && foundB) {
            *invalid = TRUE;
            break;
        }
    }

    if (*invalid == TRUE) {
        return ret;
    }

    /* The instruction is valid. */
    ret.limm_p = has_limm;
    ret.class = (uint32_t) opcode->insn_class;

    /*
     * FIXME: here add extra info about the instruction
     * e.g. delay slot, data size, write back, etc.
     */
    return ret;
}

/* Main entry point for this file. */
const struct arc_opcode *arc_find_format(insn_t *insnd,
                                         uint64_t insn,
                                         uint8_t insn_len,
                                         uint32_t isa_mask)
{
    enum opcode multi_match[10];
    const struct arc_opcode *ret = NULL;

    unsigned char mcount = find_insn_for_opcode(insn, isa_mask, insn_len, multi_match);
    for(unsigned char i = 0; i < mcount; i++) {
        bool invalid = FALSE;
        insn_t tmp = load_insninfo_if_valid(insn, insn_len, isa_mask, &invalid, &arc_opcodes[multi_match[i]]);

        if(invalid == FALSE) {
            *insnd = tmp;
            ret = &arc_opcodes[multi_match[i]];
        }
    }

    return ret;
}

/*
 * Calculate the instruction length for an instruction starting with
 * MSB and LSB, the most and least significant byte. The ISA_MASK is
 * used to filter the instructions considered to only those that are
 * part of the current architecture.
 *
 * The instruction lengths are calculated from the ARC_OPCODE table,
 * and cached for later use.
 */
unsigned int arc_insn_length(uint16_t insn, uint16_t cpu_type)
{
    uint8_t major_opcode;
    uint8_t msb, lsb;

    msb = (uint8_t)(insn >> 8);
    lsb = (uint8_t)(insn & 0xFF);
    major_opcode = msb >> 3;

    switch (cpu_type) {
    case ARC_OPCODE_ARC700:
        if (major_opcode == 0xb) {
            uint8_t minor_opcode = lsb & 0x1f;

            if (minor_opcode < 4) {
                return 6;
            } else if (minor_opcode == 0x10 || minor_opcode == 0x11) {
                return 8;
            }
        }
        if (major_opcode == 0xa) {
            return 8;
        }
        /* Fall through. */
    case ARC_OPCODE_ARC600:
        return (major_opcode > 0xb) ? 2 : 4;
        break;

    case ARC_OPCODE_ARCv2EM:
    case ARC_OPCODE_ARCv2HS:
        return (major_opcode > 0x7) ? 2 : 4;
        break;

    default:
        g_assert_not_reached();
    }
}

#define ARRANGE_ENDIAN(info, buf)                                       \
    (info->endian == BFD_ENDIAN_LITTLE ? bfd_getm32(bfd_getl32(buf))    \
     : bfd_getb32(buf))

/*
 * Helper function to convert middle-endian data to something more
 * meaningful.
 */

static bfd_vma bfd_getm32(unsigned int data)
{
    bfd_vma value = 0;

    value  = (data & 0x0000ffff) << 16;
    value |= (data & 0xffff0000) >> 16;
    return value;
}

/* Helper for printing instruction flags. */

bool special_flag_p(const char *opname, const char *flgname);
bool special_flag_p(const char *opname, const char *flgname)
{
    const struct arc_flag_special *flg_spec;
    unsigned i, j, flgidx;

    for (i = 0; i < arc_num_flag_special; ++i) {
        flg_spec = &arc_flag_special_cases[i];

        if (strcmp(opname, flg_spec->name) != 0) {
            continue;
        }

        /* Found potential special case instruction. */
        for (j = 0; ; ++j) {
            flgidx = flg_spec->flags[j];
            if (flgidx == 0) {
                break; /* End of the array. */
            }

            if (strcmp(flgname, arc_flag_operands[flgidx].name) == 0) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/* Print instruction flags. */

static void print_flags(const struct arc_opcode *opcode,
                        uint64_t insn,
                        struct disassemble_info *info)
{
    const unsigned char *flgidx;
    unsigned int value;

    /* Now extract and print the flags. */
    for (flgidx = opcode->flags; *flgidx; flgidx++) {
        /* Get a valid flag class. */
        const struct arc_flag_class *cl_flags = &arc_flag_classes[*flgidx];
        const unsigned *flgopridx;

        /* Check first the extensions. Not supported yet. */
        if (cl_flags->flag_class & F_CLASS_EXTEND) {
            value = insn & 0x1F;
        }

        for (flgopridx = cl_flags->flags; *flgopridx; ++flgopridx) {
            const struct arc_flag_operand *flg_operand =
                &arc_flag_operands[*flgopridx];

            /* Implicit flags are only used for the insn decoder. */
            if (cl_flags->flag_class & F_CLASS_IMPLICIT) {
                continue;
            }

            if (!flg_operand->favail) {
                continue;
            }

            if (cl_flags->extract) {
                value = (*cl_flags->extract)(insn);
            } else {
                value = (insn >> flg_operand->shift) &
                        ((1 << flg_operand->bits) - 1);
            }

            if (value == flg_operand->code) {
                /* FIXME!: print correctly nt/t flag. */
                if (!special_flag_p(opcode->name, flg_operand->name)) {
                    (*info->fprintf_func)(info->stream, ".");
                }
                (*info->fprintf_func)(info->stream, "%s", flg_operand->name);
            }
        }
    }
}

static const char *get_auxreg(const struct arc_opcode *opcode,
                       int value,
                       unsigned isa_mask)
{
    unsigned int i;
    const struct arc_aux_reg_detail *auxr = &arc_aux_regs_detail[0];

    if (opcode->insn_class != AUXREG) {
        return NULL;
    }

    for (i = 0; i < ARRAY_SIZE(arc_aux_regs); i++, auxr++) {
        if (!(auxr->cpu & isa_mask)) {
            continue;
        }

        if (auxr->subclass != NONE) {
            return NULL;
        }

        if (auxr->address == value) {
            return auxr->name;
        }
    }
    return NULL;
}

/* Print the operands of an instruction. */

static void print_operands(const struct arc_opcode *opcode,
                           bfd_vma memaddr,
                           uint64_t insn,
                           uint32_t isa_mask,
                           insn_t *pinsn,
                           struct disassemble_info *info)
{
    bfd_boolean need_comma  = FALSE;
    bfd_boolean open_braket = FALSE;
    int value, vpcl = 0;
    bfd_boolean rpcl = FALSE, rset = FALSE;
    const unsigned char *opidx;
    int i;

    for (i = 0, opidx = opcode->operands; *opidx; opidx++) {
        const struct arc_operand *operand = &arc_operands[*opidx];

        if (open_braket && (operand->flags & ARC_OPERAND_BRAKET)) {
            (*info->fprintf_func)(info->stream, "]");
            open_braket = FALSE;
            continue;
        }

        /* Only take input from real operands. */
        if (ARC_OPERAND_IS_FAKE(operand)) {
            continue;
        }

        if (need_comma) {
            (*info->fprintf_func)(info->stream, ",");
        }

        if (!open_braket && (operand->flags & ARC_OPERAND_BRAKET)) {
            (*info->fprintf_func)(info->stream, "[");
            open_braket = TRUE;
            need_comma  = FALSE;
            continue;
        }

        need_comma = TRUE;

        /* Get the decoded */
        value = pinsn->operands[i++].value;

        if ((operand->flags & ARC_OPERAND_IGNORE) &&
            (operand->flags & ARC_OPERAND_IR) &&
            value == -1) {
            need_comma = FALSE;
            continue;
        }

        if (operand->flags & ARC_OPERAND_PCREL) {
            rpcl = TRUE;
            vpcl = value;
            rset = TRUE;

            info->target = (bfd_vma) (memaddr & ~3) + value;
        } else if (!(operand->flags & ARC_OPERAND_IR)) {
            vpcl = value;
            rset = TRUE;
        }

        /* Print the operand as directed by the flags. */
        if (operand->flags & ARC_OPERAND_IR) {
            const char *rname;

            assert(value >= 0 && value < 64);
            rname = get_register_name(value);
            /* A single register. */
            if ((operand->flags & ARC_OPERAND_TRUNCATE) == 0) {
                (*info->fprintf_func)(info->stream, "%s", rname);
            } else { /* A register pair, e.g. "r1r0". */
                /* Make sure we print only legal register pairs. */
                if ((value & 0x01) == 0) {
                    const char *rpair = get_register_name(value+1);
                    (*info->fprintf_func)(info->stream, "%s", rpair);
                }
                (*info->fprintf_func)(info->stream, "%s", rname);
            }
            if (value == 63) {
                rpcl = TRUE;
            } else {
                rpcl = FALSE;
            }
        } else if (operand->flags & ARC_OPERAND_LIMM) {
            value = pinsn->limm;
            const char *rname = get_auxreg(opcode, value, isa_mask);

            if (rname && open_braket) {
                (*info->fprintf_func)(info->stream, "%s", rname);
            } else {
                (*info->fprintf_func)(info->stream, "%#x", value);
            }
        } else if (operand->flags & ARC_OPERAND_SIGNED) {
            const char *rname = get_auxreg(opcode, value, isa_mask);
            if (rname && open_braket) {
                (*info->fprintf_func)(info->stream, "%s", rname);
            } else {
                (*info->fprintf_func)(info->stream, "%d", value);
            }
        } else {
            if (operand->flags & ARC_OPERAND_TRUNCATE   &&
                !(operand->flags & ARC_OPERAND_ALIGNED32) &&
                !(operand->flags & ARC_OPERAND_ALIGNED16) &&
                 value >= 0 && value <= 14) {
                /* Leave/Enter mnemonics. */
                switch (value) {
                case 0:
                    need_comma = FALSE;
                    break;
                case 1:
                    (*info->fprintf_func)(info->stream, "r13");
                    break;
                default:
                    (*info->fprintf_func)(info->stream, "r13-%s",
                            get_register_name(13 + value - 1));
                    break;
                }
                rpcl = FALSE;
                rset = FALSE;
            } else {
                const char *rname = get_auxreg(opcode, value, isa_mask);
                if (rname && open_braket) {
                    (*info->fprintf_func)(info->stream, "%s", rname);
                } else {
                    (*info->fprintf_func)(info->stream, "%#x", value);
                }
            }
        }
    }

    /* Pretty print extra info for pc-relative operands. */
    if (rpcl && rset) {
        if (info->flags & INSN_HAS_RELOC) {
            /*
             * If the instruction has a reloc associated with it, then
             * the offset field in the instruction will actually be
             * the addend for the reloc.  (We are using REL type
             * relocs).  In such cases, we can ignore the pc when
             * computing addresses, since the addend is not currently
             * pc-relative.
             */
            memaddr = 0;
        }

        (*info->fprintf_func)(info->stream, "\t;");
        (*info->print_address_func)((memaddr & ~3) + vpcl, info);
    }
}

/* Select the proper instructions set for the given architecture. */

static int arc_read_mem(bfd_vma memaddr,
                        uint64_t *insn,
                        uint32_t *isa_mask,
                        struct disassemble_info *info)
{
    bfd_byte buffer[8];
    unsigned int highbyte, lowbyte;
    int status;
    int insn_len = 0;

    highbyte = ((info->endian == BFD_ENDIAN_LITTLE) ? 1 : 0);
    lowbyte  = ((info->endian == BFD_ENDIAN_LITTLE) ? 0 : 1);

    switch (info->mach) {
    case bfd_mach_arc_arc700:
        *isa_mask = ARC_OPCODE_ARC700;
        break;
    case bfd_mach_arc_arc601:
    case bfd_mach_arc_arc600:
        *isa_mask = ARC_OPCODE_ARC600;
        break;
    case bfd_mach_arc_arcv2em:
    case bfd_mach_arc_arcv2:
        *isa_mask = ARC_OPCODE_ARCv2EM;
        break;
    case bfd_mach_arc_arcv2hs:
        *isa_mask = ARC_OPCODE_ARCv2HS;
        break;

    default:
        *isa_mask = ARC_OPCODE_NONE;
        break;
    }

    info->bytes_per_line  = 8;
    info->bytes_per_chunk = 2;
    info->display_endian = info->endian;

    /* Read the insn into a host word. */
    status = (*info->read_memory_func)(memaddr, buffer, 2, info);

    if (status != 0) {
        (*info->memory_error_func)(status, memaddr, info);
        return -1;
    }

    insn_len = arc_insn_length((buffer[highbyte] << 8 |
                buffer[lowbyte]), *isa_mask);

    switch (insn_len) {
    case 2:
        *insn = (buffer[highbyte] << 8) | buffer[lowbyte];
        break;

    case 4:
        /* This is a long instruction: Read the remaning 2 bytes. */
        status = (*info->read_memory_func)(memaddr + 2, &buffer[2], 2, info);
        if (status != 0) {
            (*info->memory_error_func)(status, memaddr + 2, info);
            return -1;
        }
        *insn = (uint64_t) ARRANGE_ENDIAN(info, buffer);
        break;

    case 6:
        status = (*info->read_memory_func)(memaddr + 2, &buffer[2], 4, info);
        if (status != 0) {
            (*info->memory_error_func)(status, memaddr + 2, info);
            return -1;
        }
        *insn  = (uint64_t) ARRANGE_ENDIAN(info, &buffer[2]);
        *insn |= ((uint64_t) buffer[highbyte] << 40) |
                 ((uint64_t) buffer[lowbyte]  << 32);
        break;

    case 8:
        status = (*info->read_memory_func)(memaddr + 2, &buffer[2], 6, info);
        if (status != 0) {
            (*info->memory_error_func)(status, memaddr + 2, info);
            return -1;
        }
        *insn = ((((uint64_t) ARRANGE_ENDIAN(info, buffer)) << 32) |
                  ((uint64_t) ARRANGE_ENDIAN(info, &buffer[4])));
        break;

    default:
        /* There is no instruction whose length is not 2, 4, 6, or 8. */
        g_assert_not_reached();
    }
    return insn_len;
}

/* Disassembler main entry function. */

int print_insn_arc(bfd_vma memaddr, struct disassemble_info *info)
{
    const struct arc_opcode *opcode = NULL;
    int insn_len = -1;
    uint64_t insn;
    uint32_t isa_mask;
    insn_t dis_insn;

    insn_len = arc_read_mem(memaddr, &insn, &isa_mask, info);

    if (insn_len < 2) {
        return -1;
    }

    opcode = arc_find_format(&dis_insn, insn, insn_len, isa_mask);

    if (!opcode) {
        switch (insn_len) {
        case 2:
            (*info->fprintf_func)(info->stream, ".shor\t%#04lx",
                   insn & 0xffff);
            break;

        case 4:
            (*info->fprintf_func)(info->stream, ".word\t%#08lx",
                                  insn & 0xffffffff);
            break;

        case 6:
            (*info->fprintf_func)(info->stream, ".long\t%#08lx",
                                  insn & 0xffffffff);
            (*info->fprintf_func)(info->stream, ".long\t%#04lx",
                                  (insn >> 32) & 0xffff);
            break;

        case 8:
            (*info->fprintf_func)(info->stream, ".long\t%#08lx",
                                  insn & 0xffffffff);
            (*info->fprintf_func)(info->stream, ".long\t%#08lx",
                                  insn >> 32);
            break;

        default:
            return -1;
        }

        info->insn_type = dis_noninsn;
        return insn_len;
    }

    if (dis_insn.limm_p) {
        bfd_byte buffer[4];
        int status = (*info->read_memory_func)(memaddr + insn_len, buffer,
                                               4, info);
        if (status != 0) {
            return -1;
        }
        dis_insn.limm = ARRANGE_ENDIAN(info, buffer);
        insn_len += 4;
    }

    /* Print the mnemonic. */
    (*info->fprintf_func)(info->stream, "%s", opcode->name);

    print_flags(opcode, insn, info);

    if (opcode->operands[0] != 0) {
        (*info->fprintf_func)(info->stream, "\t");
    }

    /* Now extract and print the operands. */
    print_operands(opcode, memaddr, insn, isa_mask, &dis_insn, info);

    /* Say how many bytes we consumed */
    return insn_len;
}

/*-*-indent-tabs-mode:nil;tab-width:4;indent-line-function:'insert-tab'-*-*/
/* vim: set ts=4 sw=4 et: */
