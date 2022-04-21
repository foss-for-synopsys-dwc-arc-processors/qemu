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



static insn_t
load_insninfo_if_valid_v2(uint64_t insn,
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
                bool tmp = false;
                value = (*cl_flags->extract)(insn, &tmp);
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
const struct arc_opcode *arc_find_format_v2(insn_t *insnd,
                                         uint64_t insn,
                                         uint8_t insn_len,
                                         uint32_t isa_mask)
{
    enum opcode multi_match[10];
    const struct arc_opcode *ret = NULL;

    unsigned char mcount = find_insn_for_opcode(insn, isa_mask, insn_len, multi_match);
    for(unsigned char i = 0; i < mcount; i++) {
        bool invalid = FALSE;
        insn_t tmp = load_insninfo_if_valid_v2(insn, insn_len, isa_mask, &invalid, &arc_opcodes[multi_match[i]]);

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
unsigned int arc_insn_length_v2(uint16_t insn, uint16_t cpu_type)
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

    case ARC_OPCODE_ARC32:
        if(major_opcode == 0x0b)
          return 4;
        return (major_opcode > 0x7) ? 2 : 4;
        break;

    default:
        g_assert_not_reached();
    }
}

#define ARRANGE_ENDIAN(info, buf)                                       \
    (info->endian == BFD_ENDIAN_LITTLE ? bfd_getm32_v2(bfd_getl32(buf))    \
     : bfd_getb32(buf))

/*
 * Helper function to convert middle-endian data to something more
 * meaningful.
 */

static bfd_vma bfd_getm32_v2(unsigned int data)
{
    bfd_vma value = 0;

    value  = (data & 0x0000ffff) << 16;
    value |= (data & 0xffff0000) >> 16;
    return value;
}

/* Helper for printing instruction flags. */

bool special_flag_p_v2(const char *opname, const char *flgname);
bool special_flag_p_v2(const char *opname, const char *flgname)
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

static void print_flags_v2(const struct arc_opcode *opcode,
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
                bool invalid = false;
                value = (*cl_flags->extract)(insn, &invalid);
            } else {
                value = (insn >> flg_operand->shift) &
                        ((1 << flg_operand->bits) - 1);
            }

            if (value == flg_operand->code) {
                /* FIXME!: print correctly nt/t flag. */
                if (!special_flag_p_v2(opcode->name, flg_operand->name)) {
                    (*info->fprintf_func)(info->stream, ".");
                }
                (*info->fprintf_func)(info->stream, "%s", flg_operand->name);
            }
        }
    }
}

static const char *get_auxreg_v2(const struct arc_opcode *opcode,
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

static void print_operands_v2(const struct arc_opcode *opcode,
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
            const char *rname = get_auxreg_v2(opcode, value, isa_mask);

            if (rname && open_braket) {
                (*info->fprintf_func)(info->stream, "%s", rname);
            } else {
                (*info->fprintf_func)(info->stream, "%#x", value);
            }
        } else if (operand->flags & ARC_OPERAND_SIGNED) {
            const char *rname = get_auxreg_v2(opcode, value, isa_mask);
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
                const char *rname = get_auxreg_v2(opcode, value, isa_mask);
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

static int arc_read_mem_v2(bfd_vma memaddr,
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

int print_insn_arc_v2(bfd_vma memaddr, struct disassemble_info *info);
int print_insn_arc_v2(bfd_vma memaddr, struct disassemble_info *info)
{
    const struct arc_opcode *opcode = NULL;
    int insn_len = -1;
    uint64_t insn;
    uint32_t isa_mask;
    insn_t dis_insn;

    insn_len = arc_read_mem_v2(memaddr, &insn, &isa_mask, info);

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

    print_flags_v2(opcode, insn, info);

    if (opcode->operands[0] != 0) {
        (*info->fprintf_func)(info->stream, "\t");
    }

    /* Now extract and print the operands. */
    print_operands_v2(opcode, memaddr, insn, isa_mask, &dis_insn, info);

    /* Say how many bytes we consumed */
    return insn_len;
}

/*-*-indent-tabs-mode:nil;tab-width:4;indent-line-function:'insert-tab'-*-*/
/* vim: set ts=4 sw=4 et: */
