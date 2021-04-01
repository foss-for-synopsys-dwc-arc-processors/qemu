/*
 * Decoder for the ARC.
 * Copyright (C) 2017 Free Software Foundation, Inc.

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with GAS or GDB; see the file COPYING3.  If not, write to
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
    "gp",
    "fp", "sp", "ilink", "r30", "blink",

    "r32", "r33", "r34", "r35", "r36", "r37", "r38", "r39",
    "r40", "r41", "r42", "r43", "r44", "r45", "r46", "r47",
    "r48", "r49", "r50", "r51", "r52", "r53", "r54", "r55",
    "r56", "r57", "r58", "r59", "lp_count", "rezerved", "LIMM", "pcl"
};
const char *get_register_name(int value)
{
    return regnames[value];
}

extern bool special_flag_p(const char *opname, const char *flgname);

static long long int
extract_rb (unsigned long long insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = (((insn >> 12) & 0x07) << 3) | ((insn >> 24) & 0x07);

  if (value == 0x3e && invalid)
    *invalid = TRUE; /* A limm operand, it should be extracted in a
			different way.  */

  return value;
}

static long long int
extract_rhv1 (unsigned long long insn ATTRIBUTE_UNUSED,
	      bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = ((insn & 0x7) << 3) | ((insn >> 5) & 0x7);

  return value;
}

static long long int
extract_rhv2 (unsigned long long insn ATTRIBUTE_UNUSED,
	      bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = ((insn >> 5) & 0x07) | ((insn & 0x03) << 3);

  return value;
}

static long long int
extract_r0 (unsigned long long insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 0;
}

static long long int
extract_r1 (unsigned long long insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 1;
}

static long long int
extract_r2 (unsigned long long insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 2;
}

static long long int
extract_r3 (unsigned long long insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 3;
}

static long long int
extract_sp (unsigned long long insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 28;
}

static long long int
extract_gp (unsigned long long insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 26;
}

static long long int
extract_pcl (unsigned long long insn ATTRIBUTE_UNUSED,
	     bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 63;
}

static long long int
extract_blink (unsigned long long insn ATTRIBUTE_UNUSED,
	       bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 31;
}

static long long int
extract_ilink1 (unsigned long long insn ATTRIBUTE_UNUSED,
		bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 29;
}

static long long int
extract_ilink2 (unsigned long long insn ATTRIBUTE_UNUSED,
		bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  return 30;
}

static long long int
extract_ras (unsigned long long insn ATTRIBUTE_UNUSED,
	     bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = insn & 0x07;
  if (value > 3)
    return (value + 8);
  else
    return value;
}

static long long int
extract_rbs (unsigned long long insn ATTRIBUTE_UNUSED,
	     bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = (insn >> 8) & 0x07;
  if (value > 3)
    return (value + 8);
  else
    return value;
}

static long long int
extract_rcs (unsigned long long insn ATTRIBUTE_UNUSED,
	     bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = (insn >> 5) & 0x07;
  if (value > 3)
    return (value + 8);
  else
    return value;
}

static long long int
extract_simm3s (unsigned long long insn ATTRIBUTE_UNUSED,
		bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = (insn >> 8) & 0x07;
  if (value == 7)
    return -1;
  else
    return value;
}

static long long int
extract_rrange (unsigned long long insn  ATTRIBUTE_UNUSED,
		bfd_boolean * invalid  ATTRIBUTE_UNUSED)
{
  return (insn >> 1) & 0x0F;
}

static long long int
extract_fpel (unsigned long long insn  ATTRIBUTE_UNUSED,
	      bfd_boolean * invalid  ATTRIBUTE_UNUSED)
{
  return (insn & 0x0100) ? 27 : -1;
}

static long long int
extract_blinkel (unsigned long long insn  ATTRIBUTE_UNUSED,
		 bfd_boolean * invalid  ATTRIBUTE_UNUSED)
{
  return (insn & 0x0200) ? 31 : -1;
}

static long long int
extract_pclel (unsigned long long insn  ATTRIBUTE_UNUSED,
	       bfd_boolean * invalid  ATTRIBUTE_UNUSED)
{
  return (insn & 0x0400) ? 63 : -1;
}

#define EXTRACT_W6
/* mask = 00000000000000000000111111000000.  */
static long long int
extract_w6 (unsigned long long insn ATTRIBUTE_UNUSED,
	    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  signed value = 0;

  value |= ((insn >> 6) & 0x003f) << 0;

  int signbit = 1 << 5;
  value = (value ^ signbit) - signbit;

  return value;
}

#define EXTRACT_G_S
/* mask = 0000011100022000.  */
static long long int
extract_g_s (unsigned long long insn ATTRIBUTE_UNUSED,
	     bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 8) & 0x0007) << 0;
  value |= ((insn >> 3) & 0x0003) << 3;

  /* Extend the sign.  */
  int signbit = 1 << (6 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}

static long long int
extract_uimm12_20 (unsigned long long insn ATTRIBUTE_UNUSED,
		   bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 6) & 0x003f) << 0;
  value |= ((insn >> 0) & 0x003f) << 6;

  return value;
}

#ifndef EXTRACT_LIMM
#define EXTRACT_LIMM
/* mask = 00000000000000000000000000000000.  */
static ATTRIBUTE_UNUSED int
extract_limm (unsigned long long insn ATTRIBUTE_UNUSED, bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  return value;
}
#endif /* EXTRACT_LIMM */

#ifndef EXTRACT_UIMM6_20
#define EXTRACT_UIMM6_20
/* mask = 00000000000000000000111111000000.  */
static long long int
extract_uimm6_20 (unsigned long long insn ATTRIBUTE_UNUSED,
		  bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x003f) << 0;

  return value;
}
#endif /* EXTRACT_UIMM6_20 */

#ifndef EXTRACT_SIMM12_20
#define EXTRACT_SIMM12_20
/* mask = 00000000000000000000111111222222.  */
static long long int
extract_simm12_20 (unsigned long long insn ATTRIBUTE_UNUSED,
		   bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 6) & 0x003f) << 0;
  value |= ((insn >> 0) & 0x003f) << 6;

  /* Extend the sign.  */
  int signbit = 1 << (12 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM12_20 */

#ifndef EXTRACT_SIMM3_5_S
#define EXTRACT_SIMM3_5_S
/* mask = 0000011100000000.  */
static ATTRIBUTE_UNUSED int
extract_simm3_5_s (unsigned long long insn ATTRIBUTE_UNUSED,
		   bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 8) & 0x0007) << 0;

  /* Extend the sign.  */
  int signbit = 1 << (3 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM3_5_S */

#ifndef EXTRACT_LIMM_S
#define EXTRACT_LIMM_S
/* mask = 0000000000000000.  */
static ATTRIBUTE_UNUSED int
extract_limm_s (unsigned long long insn ATTRIBUTE_UNUSED, bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  return value;
}
#endif /* EXTRACT_LIMM_S */

#ifndef EXTRACT_UIMM7_A32_11_S
#define EXTRACT_UIMM7_A32_11_S
/* mask = 0000000000011111.  */
static long long int
extract_uimm7_a32_11_s (unsigned long long insn ATTRIBUTE_UNUSED,
			bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x001f) << 2;

  return value;
}
#endif /* EXTRACT_UIMM7_A32_11_S */

#ifndef EXTRACT_UIMM7_9_S
#define EXTRACT_UIMM7_9_S
/* mask = 0000000001111111.  */
static long long int
extract_uimm7_9_s (unsigned long long insn ATTRIBUTE_UNUSED,
		   bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x007f) << 0;

  return value;
}
#endif /* EXTRACT_UIMM7_9_S */

#ifndef EXTRACT_UIMM3_13_S
#define EXTRACT_UIMM3_13_S
/* mask = 0000000000000111.  */
static long long int
extract_uimm3_13_s (unsigned long long insn ATTRIBUTE_UNUSED,
		    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x0007) << 0;

  return value;
}
#endif /* EXTRACT_UIMM3_13_S */

#ifndef EXTRACT_SIMM11_A32_7_S
#define EXTRACT_SIMM11_A32_7_S
/* mask = 0000000111111111.  */
static long long int
extract_simm11_a32_7_s (unsigned long long insn ATTRIBUTE_UNUSED,
			bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x01ff) << 2;

  /* Extend the sign.  */
  int signbit = 1 << (11 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM11_A32_7_S */

#ifndef EXTRACT_UIMM6_13_S
#define EXTRACT_UIMM6_13_S
/* mask = 0000000002220111.  */
static long long int
extract_uimm6_13_s (unsigned long long insn ATTRIBUTE_UNUSED,
		    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x0007) << 0;
  value |= ((insn >> 4) & 0x0007) << 3;

  return value;
}
#endif /* EXTRACT_UIMM6_13_S */

#ifndef EXTRACT_UIMM5_11_S
#define EXTRACT_UIMM5_11_S
/* mask = 0000000000011111.  */
static long long int
extract_uimm5_11_s (unsigned long long insn ATTRIBUTE_UNUSED,
		    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x001f) << 0;

  return value;
}
#endif /* EXTRACT_UIMM5_11_S */

#ifndef EXTRACT_SIMM9_A16_8
#define EXTRACT_SIMM9_A16_8
/* mask = 00000000111111102000000000000000.  */
static long long int
extract_simm9_a16_8 (unsigned long long insn ATTRIBUTE_UNUSED,
		     bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 17) & 0x007f) << 1;
  value |= ((insn >> 15) & 0x0001) << 8;

  /* Extend the sign.  */
  int signbit = 1 << (9 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM9_A16_8 */

#ifndef EXTRACT_UIMM6_8
#define EXTRACT_UIMM6_8
/* mask = 00000000000000000000111111000000.  */
static long long int
extract_uimm6_8 (unsigned long long insn ATTRIBUTE_UNUSED,
		 bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x003f) << 0;

  return value;
}
#endif /* EXTRACT_UIMM6_8 */

#ifndef EXTRACT_SIMM21_A16_5
#define EXTRACT_SIMM21_A16_5
/* mask = 00000111111111102222222222000000.  */
static long long int
extract_simm21_a16_5 (unsigned long long insn ATTRIBUTE_UNUSED,
		      bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 17) & 0x03ff) << 1;
  value |= ((insn >> 6) & 0x03ff) << 11;

  /* Extend the sign.  */
  int signbit = 1 << (21 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM21_A16_5 */

#ifndef EXTRACT_SIMM25_A16_5
#define EXTRACT_SIMM25_A16_5
/* mask = 00000111111111102222222222003333.  */
static long long int
extract_simm25_a16_5 (unsigned long long insn ATTRIBUTE_UNUSED,
		      bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 17) & 0x03ff) << 1;
  value |= ((insn >> 6) & 0x03ff) << 11;
  value |= ((insn >> 0) & 0x000f) << 21;

  /* Extend the sign.  */
  int signbit = 1 << (25 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM25_A16_5 */

#ifndef EXTRACT_SIMM10_A16_7_S
#define EXTRACT_SIMM10_A16_7_S
/* mask = 0000000111111111.  */
static long long int
extract_simm10_a16_7_s (unsigned long long insn ATTRIBUTE_UNUSED,
			bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x01ff) << 1;

  /* Extend the sign.  */
  int signbit = 1 << (10 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM10_A16_7_S */

#ifndef EXTRACT_SIMM7_A16_10_S
#define EXTRACT_SIMM7_A16_10_S
/* mask = 0000000000111111.  */
static long long int
extract_simm7_a16_10_s (unsigned long long insn ATTRIBUTE_UNUSED,
			bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x003f) << 1;

  /* Extend the sign.  */
  int signbit = 1 << (7 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM7_A16_10_S */

#ifndef EXTRACT_SIMM21_A32_5
#define EXTRACT_SIMM21_A32_5
/* mask = 00000111111111002222222222000000.  */
static long long int
extract_simm21_a32_5 (unsigned long long insn ATTRIBUTE_UNUSED,
		      bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 18) & 0x01ff) << 2;
  value |= ((insn >> 6) & 0x03ff) << 11;

  /* Extend the sign.  */
  int signbit = 1 << (21 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM21_A32_5 */

#ifndef EXTRACT_SIMM25_A32_5
#define EXTRACT_SIMM25_A32_5
/* mask = 00000111111111002222222222003333.  */
static long long int
extract_simm25_a32_5 (unsigned long long insn ATTRIBUTE_UNUSED,
		      bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 18) & 0x01ff) << 2;
  value |= ((insn >> 6) & 0x03ff) << 11;
  value |= ((insn >> 0) & 0x000f) << 21;

  /* Extend the sign.  */
  int signbit = 1 << (25 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM25_A32_5 */

#ifndef EXTRACT_SIMM13_A32_5_S
#define EXTRACT_SIMM13_A32_5_S
/* mask = 0000011111111111.  */
static long long int
extract_simm13_a32_5_s (unsigned long long insn ATTRIBUTE_UNUSED,
			bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x07ff) << 2;

  /* Extend the sign.  */
  int signbit = 1 << (13 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM13_A32_5_S */

#ifndef EXTRACT_SIMM8_A16_9_S
#define EXTRACT_SIMM8_A16_9_S
/* mask = 0000000001111111.  */
static long long int
extract_simm8_a16_9_s (unsigned long long insn ATTRIBUTE_UNUSED,
		       bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x007f) << 1;

  /* Extend the sign.  */
  int signbit = 1 << (8 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM8_A16_9_S */

#ifndef EXTRACT_UIMM3_23
#define EXTRACT_UIMM3_23
/* mask = 00000000000000000000000111000000.  */
static long long int
extract_uimm3_23 (unsigned long long insn ATTRIBUTE_UNUSED,
		  bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x0007) << 0;

  return value;
}
#endif /* EXTRACT_UIMM3_23 */

#ifndef EXTRACT_UIMM10_6_S
#define EXTRACT_UIMM10_6_S
/* mask = 0000001111111111.  */
static long long int
extract_uimm10_6_s (unsigned long long insn ATTRIBUTE_UNUSED,
		    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x03ff) << 0;

  return value;
}
#endif /* EXTRACT_UIMM10_6_S */

#ifndef EXTRACT_UIMM6_11_S
#define EXTRACT_UIMM6_11_S
/* mask = 0000002200011110.  */
static long long int
extract_uimm6_11_s (unsigned long long insn ATTRIBUTE_UNUSED,
		    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 1) & 0x000f) << 0;
  value |= ((insn >> 8) & 0x0003) << 4;

  return value;
}
#endif /* EXTRACT_UIMM6_11_S */

#ifndef EXTRACT_SIMM9_8
#define EXTRACT_SIMM9_8
/* mask = 00000000111111112000000000000000.  */
static long long int
extract_simm9_8 (unsigned long long insn ATTRIBUTE_UNUSED,
		 bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 16) & 0x00ff) << 0;
  value |= ((insn >> 15) & 0x0001) << 8;

  /* Extend the sign.  */
  int signbit = 1 << (9 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM9_8 */

#ifndef EXTRACT_UIMM10_A32_8_S
#define EXTRACT_UIMM10_A32_8_S
/* mask = 0000000011111111.  */
static long long int
extract_uimm10_a32_8_s (unsigned long long insn ATTRIBUTE_UNUSED,
			bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x00ff) << 2;

  return value;
}
#endif /* EXTRACT_UIMM10_A32_8_S */

#ifndef EXTRACT_SIMM9_7_S
#define EXTRACT_SIMM9_7_S
/* mask = 0000000111111111.  */
static long long int
extract_simm9_7_s (unsigned long long insn ATTRIBUTE_UNUSED,
		   bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x01ff) << 0;

  /* Extend the sign.  */
  int signbit = 1 << (9 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM9_7_S */

#ifndef EXTRACT_UIMM6_A16_11_S
#define EXTRACT_UIMM6_A16_11_S
/* mask = 0000000000011111.  */
static long long int
extract_uimm6_a16_11_s (unsigned long long insn ATTRIBUTE_UNUSED,
			bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x001f) << 1;

  return value;
}
#endif /* EXTRACT_UIMM6_A16_11_S */


#ifndef EXTRACT_UIMM5_A32_11_S
#define EXTRACT_UIMM5_A32_11_S
/* mask = 0000020000011000.  */
static long long int
extract_uimm5_a32_11_s (unsigned long long insn ATTRIBUTE_UNUSED,
			bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 3) & 0x0003) << 2;
  value |= ((insn >> 10) & 0x0001) << 4;

  return value;
}
#endif /* EXTRACT_UIMM5_A32_11_S */

#ifndef EXTRACT_SIMM11_A32_13_S
#define EXTRACT_SIMM11_A32_13_S
/* mask = 0000022222200111.  */
static long long int
extract_simm11_a32_13_s (unsigned long long insn ATTRIBUTE_UNUSED,
			 bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 0) & 0x0007) << 2;
  value |= ((insn >> 5) & 0x003f) << 5;

  /* Extend the sign.  */
  int signbit = 1 << (11 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM11_A32_13_S */

#ifndef EXTRACT_UIMM7_13_S
#define EXTRACT_UIMM7_13_S
/* mask = 0000000022220111.  */
static long long int
extract_uimm7_13_s (unsigned long long insn ATTRIBUTE_UNUSED,
		    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x0007) << 0;
  value |= ((insn >> 4) & 0x000f) << 3;

  return value;
}
#endif /* EXTRACT_UIMM7_13_S */

#ifndef EXTRACT_UIMM6_A16_21
#define EXTRACT_UIMM6_A16_21
/* mask = 00000000000000000000011111000000.  */
static long long int
extract_uimm6_a16_21 (unsigned long long insn ATTRIBUTE_UNUSED,
		      bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x001f) << 1;

  return value;
}
#endif /* EXTRACT_UIMM6_A16_21 */

#ifndef EXTRACT_UIMM7_11_S
#define EXTRACT_UIMM7_11_S
/* mask = 0000022200011110.  */
static long long int
extract_uimm7_11_s (unsigned long long insn ATTRIBUTE_UNUSED,
		    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 1) & 0x000f) << 0;
  value |= ((insn >> 8) & 0x0007) << 4;

  return value;
}
#endif /* EXTRACT_UIMM7_11_S */

#ifndef EXTRACT_UIMM7_A16_20
#define EXTRACT_UIMM7_A16_20
/* mask = 00000000000000000000111111000000.  */
static long long int
extract_uimm7_a16_20 (unsigned long long insn ATTRIBUTE_UNUSED,
		      bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 6) & 0x003f) << 1;

  return value;
}
#endif /* EXTRACT_UIMM7_A16_20 */

#ifndef EXTRACT_SIMM13_A16_20
#define EXTRACT_SIMM13_A16_20
/* mask = 00000000000000000000111111222222.  */
static long long int
extract_simm13_a16_20 (unsigned long long insn ATTRIBUTE_UNUSED,
		       bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  int value = 0;

  value |= ((insn >> 6) & 0x003f) << 1;
  value |= ((insn >> 0) & 0x003f) << 7;

  /* Extend the sign.  */
  int signbit = 1 << (13 - 1);
  value = (value ^ signbit) - signbit;

  return value;
}
#endif /* EXTRACT_SIMM13_A16_20 */


#ifndef EXTRACT_UIMM8_8_S
#define EXTRACT_UIMM8_8_S
/* mask = 0000000011111111.  */
static long long int
extract_uimm8_8_s (unsigned long long insn ATTRIBUTE_UNUSED,
		   bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 0) & 0x00ff) << 0;

  return value;
}
#endif /* EXTRACT_UIMM8_8_S */

#ifndef EXTRACT_UIMM6_5_S
#define EXTRACT_UIMM6_5_S
/* mask = 0000011111100000.  */
static long long int
extract_uimm6_5_s (unsigned long long insn ATTRIBUTE_UNUSED,
		   bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  value |= ((insn >> 5) & 0x003f) << 0;

  return value;
}
#endif /* EXTRACT_UIMM6_5_S */

#ifndef EXTRACT_UIMM6_AXX_
#define EXTRACT_UIMM6_AXX_
/* mask = 00000000000000000000000000000000.  */
static ATTRIBUTE_UNUSED int
extract_uimm6_axx_ (unsigned long long insn ATTRIBUTE_UNUSED,
		    bfd_boolean * invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;

  return value;
}
#endif /* EXTRACT_UIMM6_AXX_ */

/* mask  = 0000022000011111.  */
#ifndef EXTRACT_UIMM9_A32_11_S
#define EXTRACT_UIMM9_A32_11_S
ATTRIBUTE_UNUSED static long long int
extract_uimm9_a32_11_s (unsigned long long insn ATTRIBUTE_UNUSED,
                        bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;
  value |= ((insn >> 0) & 0x001f) << 2;
  value |= ((insn >> 9) & 0x0003) << 7;

  return value;
}
#endif /* EXTRACT_UIMM9_A32_11_S */

/* mask  = 0000022222220111.  */
#ifndef EXTRACT_UIMM10_13_S
#define EXTRACT_UIMM10_13_S
ATTRIBUTE_UNUSED static long long int
extract_uimm10_13_s (unsigned long long insn ATTRIBUTE_UNUSED,
                     bfd_boolean *invalid ATTRIBUTE_UNUSED)
{
  unsigned value = 0;
  value |= ((insn >> 0) & 0x0007) << 0;
  value |= ((insn >> 4) & 0x007f) << 3;

  return value;
}
#endif /* EXTRACT_UIMM10_13_S */

static long long
extract_rbb (unsigned long long  insn,
	    bfd_boolean *       invalid)
{
  int value = (((insn >> 1) & 0x07) << 3) | ((insn >> 8) & 0x07);

  if (value == 0x3e && invalid)
    *invalid = TRUE; /* A limm operand, it should be extracted in a
			different way.  */

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
#include "target/arc/operands-v3.def"
#undef ARC_OPERAND
    { 0, 0, 0, 0}
};

enum arc_operands_map {
    OPERAND_UNUSED = 0,
#define ARC_OPERAND(NAME, BITS, SHIFT, RELO, FLAGS, FUN) OPERAND_##NAME,
#include "target/arc/operands-v3.def"
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
#include "target/arc/flags-v3.def"
#undef ARC_FLAG
    { 0, 0, 0, 0, 0}
};

enum arc_flags_map {
    F_NULL = 0,
#define ARC_FLAG(NAME, MNEMONIC, CODE, BITS, SHIFT, AVAIL) F_##NAME,
#include "target/arc/flags-v3.def"
#undef ARC_FLAG
    F_LAST
};

/*
 * Table of the flag classes.
 *
 * The format of the table is
 * CLASS {FLAG_CODE}.
 */
const struct arc_flag_class arc_flag_classes[] =
{
#define C_EMPTY     0
  { F_CLASS_NONE, { F_NULL } },

#define C_CC_EQ     (C_EMPTY + 1)
  {F_CLASS_IMPLICIT | F_CLASS_COND, {F_EQUAL, F_NULL} },

#define C_CC_GE     (C_CC_EQ + 1)
  {F_CLASS_IMPLICIT | F_CLASS_COND, {F_GE, F_NULL} },

#define C_CC_GT     (C_CC_GE + 1)
  {F_CLASS_IMPLICIT | F_CLASS_COND, {F_GT, F_NULL} },

#define C_CC_HI     (C_CC_GT + 1)
  {F_CLASS_IMPLICIT | F_CLASS_COND, {F_HI, F_NULL} },

#define C_CC_HS     (C_CC_HI + 1)
  {F_CLASS_IMPLICIT | F_CLASS_COND, {F_NOTCARRY, F_NULL} },

#define C_CC_LE     (C_CC_HS + 1)
  {F_CLASS_IMPLICIT | F_CLASS_COND, {F_LE, F_NULL} },

#define C_CC_LO     (C_CC_LE + 1)
  {F_CLASS_IMPLICIT | F_CLASS_COND, {F_CARRY, F_NULL} },

#define C_CC_LS     (C_CC_LO + 1)
  {F_CLASS_IMPLICIT | F_CLASS_COND, {F_LS, F_NULL} },

#define C_CC_LT     (C_CC_LS + 1)
  {F_CLASS_IMPLICIT | F_CLASS_COND, {F_LT, F_NULL} },

#define C_CC_NE     (C_CC_LT + 1)
  {F_CLASS_IMPLICIT | F_CLASS_COND, {F_NOTEQUAL, F_NULL} },

#define C_AA_AB     (C_CC_NE + 1)
  {F_CLASS_IMPLICIT | F_CLASS_WB, {F_AB3, F_NULL} },

#define C_AA_AW     (C_AA_AB + 1)
  {F_CLASS_IMPLICIT | F_CLASS_WB, {F_AW3, F_NULL} },

#define C_ZZ_D      (C_AA_AW + 1)
  {F_CLASS_IMPLICIT | F_CLASS_ZZ, {F_SIZED, F_NULL} },

#define C_ZZ_L      (C_ZZ_D + 1)
  {F_CLASS_IMPLICIT | F_CLASS_ZZ, {F_SIZEL, F_NULL} },

#define C_ZZ_W      (C_ZZ_L + 1)
  {F_CLASS_IMPLICIT | F_CLASS_ZZ, {F_SIZEW, F_NULL} },

#define C_ZZ_H      (C_ZZ_W + 1)
  {F_CLASS_IMPLICIT | F_CLASS_ZZ, {F_H1, F_NULL} },

#define C_ZZ_B      (C_ZZ_H + 1)
  {F_CLASS_IMPLICIT | F_CLASS_ZZ, {F_SIZEB1, F_NULL} },

#define C_CC	    (C_ZZ_B + 1)
  { F_CLASS_OPTIONAL | F_CLASS_EXTEND | F_CLASS_COND,
    { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL,
      F_NOTZERO, F_POZITIVE, F_PL, F_NEGATIVE, F_MINUS,
      F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
      F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW,
      F_NOTOVERFLOW, F_OVERFLOWCLR, F_GT, F_GE, F_LT,
      F_LE, F_HI, F_LS, F_PNZ, F_NJ, F_NM, F_NO_T, F_NULL } },

#define C_AA_ADDR3  (C_CC + 1)
#define C_AA27	    (C_CC + 1)
  { F_CLASS_OPTIONAL | F_CLASS_WB, { F_A3, F_AW3, F_AB3, F_AS3, F_NULL } },
#define C_AA_ADDR9  (C_AA_ADDR3 + 1)
#define C_AA21	     (C_AA_ADDR3 + 1)
  { F_CLASS_OPTIONAL | F_CLASS_WB, { F_A9, F_AW9, F_AB9, F_AS9, F_NULL } },
#define C_AA_ADDR22 (C_AA_ADDR9 + 1)
#define C_AA8	   (C_AA_ADDR9 + 1)
  { F_CLASS_OPTIONAL | F_CLASS_WB, { F_A22, F_AW22, F_AB22, F_AS22, F_NULL } },

#define C_F	    (C_AA_ADDR22 + 1)
  { F_CLASS_OPTIONAL | F_CLASS_F, { F_FLAG, F_NULL } },
#define C_FHARD	    (C_F + 1)
  { F_CLASS_OPTIONAL | F_CLASS_F, { F_FFAKE, F_NULL } },
#define C_AQ	    (C_FHARD + 1)
  { F_CLASS_OPTIONAL, { F_AQ, F_NULL } },

#define C_ATOP      (C_AQ + 1)
  { F_CLASS_REQUIRED, {F_ATO_ADD, F_ATO_OR, F_ATO_AND, F_ATO_XOR, F_ATO_MINU,
		       F_ATO_MAXU, F_ATO_MIN, F_ATO_MAX, F_NULL}},

#define C_T	    (C_ATOP + 1)
  { F_CLASS_OPTIONAL, { F_NT, F_T, F_NULL } },
#define C_D	    (C_T + 1)
  { F_CLASS_OPTIONAL, { F_ND, F_D, F_NULL } },
#define C_DNZ_D     (C_D + 1)
  { F_CLASS_OPTIONAL, { F_DNZ_ND, F_DNZ_D, F_NULL } },

#define C_DHARD	    (C_DNZ_D + 1)
  { F_CLASS_OPTIONAL, { F_DFAKE, F_NULL } },

#define C_DI20	    (C_DHARD + 1)
  { F_CLASS_OPTIONAL, { F_DI11, F_NULL }},
#define C_DI14	    (C_DI20 + 1)
  { F_CLASS_OPTIONAL, { F_DI14, F_NULL }},
#define C_DI16	    (C_DI14 + 1)
  { F_CLASS_OPTIONAL, { F_DI15, F_NULL }},
#define C_DI26	    (C_DI16 + 1)
  { F_CLASS_OPTIONAL, { F_DI5, F_NULL }},

#define C_X25	    (C_DI26 + 1)
  { F_CLASS_OPTIONAL | F_CLASS_X, { F_SIGN6, F_NULL }},
#define C_X15	   (C_X25 + 1)
  { F_CLASS_OPTIONAL | F_CLASS_X, { F_SIGN16, F_NULL }},
#define C_XHARD	   (C_X15 + 1)
#define C_X	   (C_X15 + 1)
  { F_CLASS_OPTIONAL | F_CLASS_X, { F_SIGNX, F_NULL }},

#define C_ZZ13	      (C_X + 1)
  { F_CLASS_OPTIONAL, { F_SIZEB17, F_SIZEW17, F_H17, F_D17 , F_NULL}},
#define C_ZZ23	      (C_ZZ13 + 1)
  { F_CLASS_OPTIONAL, { F_SIZEB7, F_SIZEW7, F_H7, F_D7, F_NULL}},
#define C_ZZ29	      (C_ZZ23 + 1)
  { F_CLASS_OPTIONAL, { F_SIZEB1, F_SIZEW1, F_H1, F_NULL}},

#define C_AS	    (C_ZZ29 + 1)
#define C_AAHARD13  (C_ZZ29 + 1)
  { F_CLASS_OPTIONAL, { F_ASFAKE, F_NULL}},

#define C_NE	    (C_AS + 1)
  { F_CLASS_REQUIRED, { F_NE, F_NULL}}
};

/* List with special cases instructions and the applicable flags. */
const struct arc_flag_special arc_flag_special_cases[] =
{
  { "b", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	   F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	   F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	   F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NJ, F_NM,
	   F_NO_T, F_NULL } },
  { "bl", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	    F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	    F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	    F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  { "br", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	    F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	    F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	    F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  { "j", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	   F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	   F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	   F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  { "jl", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	    F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	    F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	    F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  { "lp", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	    F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	    F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	    F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  { "set", { F_ALWAYS, F_RA, F_EQUAL, F_ZERO, F_NOTEQUAL, F_NOTZERO, F_POZITIVE,
	     F_PL, F_NEGATIVE, F_MINUS, F_CARRY, F_CARRYSET, F_LOWER, F_CARRYCLR,
	     F_NOTCARRY, F_HIGHER, F_OVERFLOWSET, F_OVERFLOW, F_NOTOVERFLOW,
	     F_OVERFLOWCLR, F_GT, F_GE, F_LT, F_LE, F_HI, F_LS, F_PNZ, F_NULL } },
  { "ld", { F_SIZEB17, F_SIZEW17, F_H17, F_NULL } },
  { "st", { F_SIZEB1, F_SIZEW1, F_H1, F_NULL } }
};

const unsigned arc_num_flag_special = ARRAY_SIZE (arc_flag_special_cases);

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
const struct arc_opcode arc_opcodes[] =
{
#define FASTMATH NONE
#include "opcodes-v3.def"
    { NULL, 0, 0, 0, 0, 0, { 0 }, { 0 } }
#undef FASTMATH
};

/* Return length of an opcode in bytes. */
static uint8_t arc_opcode_len(const struct arc_opcode *opcode)
{
    if (opcode->mask < 0x10000ull) {
        return 2;
    }

    if (opcode->mask < 0x100000000ull) {
        return 4;
    }

    if (opcode->mask < 0x1000000000000ull) {
        return 6;
    }

    return 8;
}

/*
 * Calculate the instruction length for an instruction starting with
 * MSB and LSB, the most and least significant byte.  The ISA_MASK is
 * used to filter the instructions considered to only those that are
 * part of the current architecture.
 *
 * The instruction lengths are calculated from the ARC_OPCODE table,
 * and cached for later use.
 */
unsigned int arc_insn_length(uint16_t insn, uint16_t cpu_type)
{
    uint8_t major_opcode;
    uint8_t msb;

    msb = (uint8_t)(insn >> 8);
    major_opcode = msb >> 3;

    switch (cpu_type) {
      case ARC_OPCODE_V3_ARC64:
          if(major_opcode == 0x0b)
            return 4;
          return (major_opcode > 0x7) ? 2 : 4;
          break;

      default:
        g_assert_not_reached();
    }
}

static enum dis_insn_type
arc_opcode_to_insn_type (const struct arc_opcode *opcode)
{
  enum dis_insn_type insn_type;

  switch (opcode->insn_class)
    {
    case BRANCH:
    case BBIT0:
    case BBIT1:
    case BI:
    case BIH:
    case BRCC:
    case EI:
    case JLI:
    case JUMP:
    case LOOP:
      if (!strncmp (opcode->name, "bl", 2)
	  || !strncmp (opcode->name, "jl", 2))
	{
	  if (opcode->subclass == COND)
	    insn_type = dis_condjsr;
	  else
	    insn_type = dis_jsr;
	}
      else
	{
	  if (opcode->subclass == COND)
	    insn_type = dis_condbranch;
	  else
	    insn_type = dis_branch;
	}
      break;
    case LOAD:
    case STORE:
    case MEMORY:
    case ENTER:
    case PUSH:
    case POP:
      insn_type = dis_dref;
      break;
    case LEAVE:
      insn_type = dis_branch;
      break;
    default:
      insn_type = dis_nonbranch;
      break;
    }

  return insn_type;
}

#define REG_PCL    63
#define REG_LIMM   62
#define REG_LIMM_S 30
#define REG_U32    62
#define REG_S32    60

static const struct arc_opcode *find_format(insn_t *pinsn,
                                            uint64_t insn,
                                            uint8_t insn_len,
                                            uint32_t isa_mask)
{
    uint32_t i = 0;
    const struct arc_opcode *opcode = NULL;
    const uint8_t *opidx;
    const uint8_t *flgidx;
    bool has_limm_signed = false;
    bool has_limm_unsigned = false;

    const struct arc_opcode *ret = NULL;

    do {
        bool invalid = false;
        uint32_t noperands = 0;

        opcode = &arc_opcodes[i++];

        if (!(opcode->cpu & isa_mask)) {
            continue;
        }

        if (arc_opcode_len(opcode) != (int) insn_len) {
            continue;
        }

        if ((insn & opcode->mask) != opcode->opcode) {
            continue;
        }

        if(ret != NULL)
          continue;

        memset(pinsn, 0, sizeof (*pinsn));

        has_limm_signed = false;
        has_limm_unsigned = false;

        /* Possible candidate, check the operands. */
        for (opidx = opcode->operands; *opidx; ++opidx) {
            int value, slimmind;
            const struct arc_operand *operand = &arc_operands[*opidx];

            if (operand->flags & ARC_OPERAND_FAKE) {
                continue;
            }

            if (operand->extract) {
                value = (*operand->extract)(insn, &invalid);
            }
            else {
                value = (insn >> operand->shift) & ((1 << operand->bits) - 1);
            }

            /* Check for (short) LIMM indicator.  If it is there, then
               make sure we pick the right format.  */
            slimmind = (isa_mask & (ARC_OPCODE_ARCV2 | ARC_OPCODE_V3_ARC64)) ?
              REG_LIMM_S : REG_LIMM;
            if (operand->flags & ARC_OPERAND_IR
                && !(operand->flags & ARC_OPERAND_LIMM))
              if ((value == REG_LIMM && insn_len == 4)
                  || (value == slimmind && insn_len == 2)
                  || (isa_mask & ARC_OPCODE_V3_ARC64
                      && (value == REG_S32) && (insn_len == 4)))
                {
                  invalid = TRUE;
                  break;
                }



            if (operand->flags & ARC_OPERAND_LIMM &&
                !(operand->flags & ARC_OPERAND_DUPLICATE)) {
                if(operand->flags & ARC_OPERAND_SIGNED)
                  has_limm_signed = true;
                else
                  has_limm_unsigned = true;
            }

            pinsn->operands[noperands].value = value;
            pinsn->operands[noperands].type = operand->flags;
            noperands += 1;
            pinsn->n_ops = noperands;
        }

        /* Preselect the insn class.  */
        enum dis_insn_type insn_type = arc_opcode_to_insn_type (opcode);

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
                        pinsn->cc = flg_operand->code;
                    }
                    else if (cl_flags->flag_class & F_CLASS_WB) {
                        pinsn->aa = flg_operand->code;
                    }
                    else if (cl_flags->flag_class & F_CLASS_ZZ) {
                        pinsn->zz_as_data_size = flg_operand->code;
                    }
                    continue;
                }

                value = (insn >> flg_operand->shift) &
                        ((1 << flg_operand->bits) - 1);
                if (value == flg_operand->code) {
	                if (!special_flag_p (opcode->name, flg_operand->name))
                    { }
                    else if (insn_type == dis_dref)
                    {
                        switch (flg_operand->name[0]) {
                        case 'b':
                            pinsn->zz_as_data_size = 1;
                            break;
                        case 'h':
                        case 'w':
                            pinsn->zz_as_data_size = 2;
                            break;
                        default:
                            pinsn->zz_as_data_size = 0;
                            break;
                        }
                    }

                    /*
                     * TODO: This has a problem: instruction "b label"
                     * sets this to true.
                     */
                    if (cl_flags->flag_class & F_CLASS_D) {
                        pinsn->d = value ? true : false;
                        if (cl_flags->flags[0] == F_DFAKE) {
                            pinsn->d = true;
                        }
                    }
	                if (flg_operand->name[0] == 'd'
			        && flg_operand->name[1] == 0)
			        pinsn->d = true;

                    if (cl_flags->flag_class & F_CLASS_COND) {
                        pinsn->cc = value;
                    }

                    if (cl_flags->flag_class & F_CLASS_WB) {
                        pinsn->aa = value;
                    }

                    if (cl_flags->flag_class & F_CLASS_F) {
                        pinsn->f = true;
                    }

                    if (cl_flags->flag_class & F_CLASS_DI) {
                        pinsn->di = true;
                    }

                    if (cl_flags->flag_class & F_CLASS_X) {
                        pinsn->x = true;
                    }

                    foundA = true;
                }
                if (value) {
                    foundB = true;
                }
            }

            if (!foundA && foundB) {
                invalid = TRUE;
                break;
            }
        }

        if (invalid) {
            continue;
        }

        /* The instruction is valid. */
        pinsn->signed_limm_p = has_limm_signed;
        pinsn->unsigned_limm_p = has_limm_unsigned;
        pinsn->class = (uint32_t) opcode->insn_class;

        /*
         * FIXME: here add extra info about the instruction
         * e.g. delay slot, data size, write back, etc.
         */
        if(ret == NULL)
            ret = opcode;

    } while (opcode->mask);

    if(ret != NULL)
        return ret;

    memset(pinsn, 0, sizeof (*pinsn));
    return NULL;
}

/* Helper to be used by the disassembler. */
const struct arc_opcode *arc_find_format(insn_t *insnd,
                                         uint64_t insn,
                                         uint8_t insn_len,
                                         uint32_t isa_mask)
{
    memset(insnd, 0, sizeof (*insnd));
    return find_format(insnd, insn, insn_len, isa_mask);
}



/*-*-indent-tabs-mode:nil;tab-width:4;indent-line-function:'insert-tab'-*-*/
/* vim: set ts=4 sw=4 et: */
