/*
 *  Common header file to be used by cpu and disassembler.
 *  Copyright (C) 2017 Free Software Foundation, Inc.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GAS or GDB; see the file COPYING3. If not, write to
 *  the Free Software Foundation, 51 Franklin Street - Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#ifndef ARC_COMMON_H
#define ARC_COMMON_H


/* CPU combi. */
#define ARC_OPCODE_ARCALL  (ARC_OPCODE_ARC600 | ARC_OPCODE_ARC700       \
                            | ARC_OPCODE_ARCv2EM | ARC_OPCODE_ARCv2HS   \
                            | ARC_OPCODE_V3_ARC32 | ARC_OPCODE_V3_ARC64)
#define ARC_OPCODE_ARCFPX  (ARC_OPCODE_ARC700 | ARC_OPCODE_ARCv2EM)
#define ARC_OPCODE_ARCV1   (ARC_OPCODE_ARC700 | ARC_OPCODE_ARC600)
#define ARC_OPCODE_ARCV2   (ARC_OPCODE_ARCv2EM | ARC_OPCODE_ARCv2HS)
#define ARC_OPCODE_ARCMPY6E  (ARC_OPCODE_ARC700 | ARC_OPCODE_ARCV2)

#define ARC_OPCODE_V3_ALL   (ARC_OPCODE_V3_ARC64 | ARC_OPCODE_V3_ARC32)

#define ARC_OPCODE_V2_V3    (ARC_OPCODE_V3_ALL | ARC_OPCODE_ARCV2)
#define ARC_OPCODE_ARCv2HS_AND_V3    (ARC_OPCODE_V3_ALL | ARC_OPCODE_ARCv2HS)

enum arc_cpu_family {
    ARC_OPCODE_NONE    = 0,

    ARC_OPCODE_DEFAULT = 1 << 0,
    ARC_OPCODE_ARC600  = 1 << 1,
    ARC_OPCODE_ARC700  = 1 << 2,
    ARC_OPCODE_ARCv2EM = 1 << 3,
    ARC_OPCODE_ARCv2HS = 1 << 4,
    ARC_OPCODE_V3_ARC32  = 1 << 5,
    ARC_OPCODE_V3_ARC64  = 1 << 6
};

typedef struct {
    uint64_t value;
    uint32_t type;
} operand_t;

typedef struct {
    uint32_t class;
    uint64_t limm;
    uint8_t len;
    bool limm_p;
    bool limm_split_16_p;
#define unsigned_limm_p limm_p
    bool signed_limm_p;
    operand_t operands[3];
    uint8_t n_ops;
    uint8_t cc;
    uint8_t aa;
    uint8_t zz;
#define zz_as_data_size zz
    bool d;
    bool f;
    bool di;
    bool x;
} insn_t;

#endif
