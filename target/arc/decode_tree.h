/*
 * QEMU Decoder for the ARC.
 * Copyright (C) 2021 Free Software Foundation, Inc.

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

#ifndef __DECODE_TREE_H__
#define __DECODE_TREE_H__

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

static unsigned char
find_insn_for_opcode(uint64_t insn, uint16_t cpu_type, unsigned int len, enum opcode *multi_match)
{
  unsigned int shift = (len % 4);
  uint32_t opcode = insn << (8 * shift);
  int multi_match_count = 0;

  switch(cpu_type) {
#if defined(TARGET_ARCV2)
    case ARC_OPCODE_ARCv2HS:
#include "v2_hs_dtree.def"
      break;
    case ARC_OPCODE_ARCv2EM:
#include "v2_em_dtree.def"
      break;
#elif defined(TARGET_ARCV3)
    case ARC_OPCODE_V3_ARC32:
    case ARC_OPCODE_V3_ARC64:
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

#endif /* __DECODE_TREE_H__ */

