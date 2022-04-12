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
    "r26",
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

/* Extract address writeback mode for 128-bit loads.  */

static long long
extract_qq (unsigned long long  insn)
{
  long long value;
  value = ((insn & 0x800) >> 11) | ((insn & 0x40) >> (6-1));
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

/*
 * Table of the flag classes.
 *
 * The format of the table is
 * CLASS {FLAG_CODE}.
 */

#define FLAG_CLASS(NAME, CLASS, EXTRACT_FN, ...) { \
  .flag_class = CLASS, \
  .extract = EXTRACT_FN, \
  .flags = { __VA_ARGS__, F_NULL } \
},
const struct arc_flag_class arc_flag_classes[F_CLASS_SIZE] = {
  #include "flag-classes.def"
};
#undef FLAG_CLASS


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
  { __VA_ARGS__ , 0 },
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
    #include "target/arc/opcodes.def"
};
#undef OPCODE
#undef OPERANDS_LIST
#undef FLAGS_LIST
#undef MNEMONIC

#define OPCODE(NAME, OPCODE, MASK, ARCH, CLASS, SUBCLASS, OPS, FLGS, ...) \
{ \
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
    { "INVALID", 0, 0, 0, 0, 0, 0, 0 }
};
#undef OPCODE
#undef OPERANDS_LIST
#undef FLAGS_LIST
#undef MNEMONIC

enum arc_decoder {
  DECODER_INVALID = -1,
  ARCV2_DECODER,
  ARCV3_DECODER,
  DECODER_LAST
};


struct arc_decoder_cb_struct {
  unsigned int (*arc_insn_length)(uint16_t insn, uint16_t cpu_type);
  const struct arc_opcode *(*arc_find_format)(insn_t *insnd, uint64_t insn,
					      uint8_t insn_len,
					      uint32_t isa_mask);
};

const struct arc_decoder_cb_struct arc_decoder_cb[DECODER_LAST] = {
  [ARCV2_DECODER] = {
    .arc_insn_length = arc_insn_length_v2,
    .arc_find_format = arc_find_format_v2,
  },
  [ARCV3_DECODER] = {
    .arc_insn_length = arc_insn_length_v3,
    .arc_find_format = arc_find_format_v3,
  }
};

static enum arc_decoder arc_get_decoder(void) {
#if defined(TARGET_ARC32)
  return ARCV2_DECODER;
#elif defined(TARGET_ARC64)
  return ARCV3_DECODER;
#else
#error "Should not be reached!!!!"
#endif
}

#define ARC_DECODER_CALLBACK(NAME, ...) \
  arc_decoder_cb[arc_get_decoder()].NAME(__VA_ARGS__)

unsigned int arc_insn_length(uint16_t insn, uint16_t cpu_type)
{
  return ARC_DECODER_CALLBACK(arc_insn_length, insn, cpu_type);
}

const struct arc_opcode *arc_find_format(insn_t *insnd, uint64_t insn,
                                         uint8_t insn_len, uint32_t isa_mask)
{
  return ARC_DECODER_CALLBACK(arc_find_format, insnd, insn, insn_len, isa_mask);
}

#define MATCH_PATTERN(PATTERN) \
  switch(((uint32_t) opcode) & PATTERN) {
#define END_MATCH_PATTERN(PATTERN) \
    default: \
      return OPCODE_INVALID; \
  }

#define MATCH_VALUE(VALUE) \
    case VALUE: {
#define END_MATCH_VALUE(VALUE) \
    } \
    break;

#define RETURN_MATCH(MATCH) \
    multi_match[0] = MATCH; \
    return 1;

/* TODO: This must be redefined since those are conflicting rules */
#define MULTI_MATCH(MATCH) \
    if(((((uint32_t) opcode) >> (8 * shift)) & arc_opcodes[MATCH].mask) == arc_opcodes[MATCH].opcode) { \
      multi_match[multi_match_count++] = (enum opcode) MATCH; \
    }

unsigned char
find_insn_for_opcode(uint64_t insn, uint16_t cpu_type, unsigned int len, enum opcode *multi_match)
{
  unsigned int shift = (len % 4);
  uint32_t opcode = insn << (8 * shift);
  int multi_match_count = 0;

  switch(cpu_type) {
#if defined TARGET_ARC32
    case ARC_OPCODE_ARCv2HS:
      #include "v2_hs_dtree.def"
      break;
    case ARC_OPCODE_ARCv2EM:
      #include "v2_em_dtree.def"
      break;
    case ARC_OPCODE_ARC32:
      #include "v3_hs5x_dtree.def"
      break;
#elif defined TARGET_ARC64
    case ARC_OPCODE_ARC64:
      #include "v3_hs6x_dtree.def"
      break;
#else
#error "TARGET macro not defined!"
#endif
    default:
      assert("Should not happen" == 0);
      break;
  }

  return multi_match_count;
}

#undef MATCH_PATTERN
#undef END_MATCH_PATTERN
#undef MATCH_VALUE
#undef MATCH_VALUE
#undef RETURN_MATCH
#undef MULTI_MATCH
