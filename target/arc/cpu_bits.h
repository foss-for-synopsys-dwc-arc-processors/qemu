#ifndef ARC_CPU_BITS_H
#define ARC_CPU_BITS_H

typedef enum {
    ARC_REGNUM_R0,
    ARC_REGNUM_R1,
    ARC_REGNUM_R2,
    ARC_REGNUM_R3,
    ARC_REGNUM_R4,
    ARC_REGNUM_R5,
    ARC_REGNUM_R6,
    ARC_REGNUM_R7,
    ARC_REGNUM_R8,
    ARC_REGNUM_R9,
    ARC_REGNUM_R10,
    ARC_REGNUM_R11,
    ARC_REGNUM_R12,
    ARC_REGNUM_R13,
    ARC_REGNUM_R14,
    ARC_REGNUM_R15,
    ARC_REGNUM_R16,
    ARC_REGNUM_R17,
    ARC_REGNUM_R18,
    ARC_REGNUM_R19,
    ARC_REGNUM_R20,
    ARC_REGNUM_R21,
    ARC_REGNUM_R22,
    ARC_REGNUM_R23,
    ARC_REGNUM_R24,
    ARC_REGNUM_R25,
    ARC_REGNUM_R26,
    ARC_REGNUM_R27,
    ARC_REGNUM_R28,
    ARC_REGNUM_R29,
    ARC_REGNUM_R30,
    ARC_REGNUM_R31,
    ARC_REGNUM_R32,
    /* R32-R57 are unsupported APEX registers */
    ARC_REGNUM_R56 = 56,
    ARC_REGNUM_R57,
    ARC_REGNUM_R58,
    ARC_REGNUM_R59,
    ARC_REGNUM_R60,
    ARC_REGNUM_R61,
    ARC_REGNUM_R62,
    ARC_REGNUM_R63,
    ARC_REGNUM_END,

#if defined(TARGET_ARCV2)
    ARC_REGNUM_GP = ARC_REGNUM_R26,
#endif

    ARC_REGNUM_FP = ARC_REGNUM_R27,
    ARC_REGNUM_SP = ARC_REGNUM_R28,
    ARC_REGNUM_ILINK = ARC_REGNUM_R29,

#if defined(TARGET_ARCV3_32) || defined(TARGET_ARCV3_64)
    ARC_REGNUM_GP = ARC_REGNUM_R30,
#endif

    ARC_REGNUM_LIMM_H = ARC_REGNUM_R30,
    ARC_REGNUM_BLINK = ARC_REGNUM_R31,
    ARC_REGNUM_APEX_FIRST = ARC_REGNUM_R32,
    ARC_REGNUM_APEX_LAST = ARC_REGNUM_R57,

#if defined(TARGET_ARCV3_64)
    ARC_REGNUM_ACC0 = ARC_REGNUM_R58,
#else
    ARC_REGNUM_ACCL = ARC_REGNUM_R58,
    ARC_REGNUM_ACCH = ARC_REGNUM_R59,
#endif

#if defined(TARGET_ARCV2)
    ARC_REGNUM_LP_COUNT = ARC_REGNUM_R60,
#endif

#if defined(TARGET_ARCV3_64)
    ARC_REGNUM_SLIMM = ARC_REGNUM_R60,
    ARC_REGNUM_ULIMM = ARC_REGNUM_R62,
#endif

    ARC_REGNUM_LIMM = ARC_REGNUM_R62,
    ARC_REGNUM_PCL = ARC_REGNUM_R63,
} ARCRegNum;

#if defined(TARGET_ARCV3_64)
#define arc_regnum_reserved(regnum) ((regnum) == ARC_REGNUM_R59) || ((regnum) == ARC_REGNUM_R61)
#define arc_regnum_limm(regnum) (((regnum) == ARC_REGNUM_SLIMM) || ((regnum) == ARC_REGNUM_ULIMM))
#elif defined(TARGET_ARCV3_32)
#define arc_regnum_reserved(regnum) ((regnum) == ARC_REGNUM_R60) || ((regnum) == ARC_REGNUM_R61)
#define arc_regnum_limm(regnum) ((regnum) == ARC_REGNUM_LIMM)
#elif defined(TARGET_ARCV2)
#define arc_regnum_reserved(regnum) ((regnum) == ARC_REGNUM_R61)
#define arc_regnum_limm(regnum) ((regnum) == ARC_REGNUM_LIMM)
#else
#error "Unsupported target"
#endif

#define arc_regnum_valid(regnum) (((regnum) >= 0) && ((regnum) < ARC_REGNUM_END))
#define arc_regnum_valid_h(regnum) (((regnum) >= 0) && ((regnum) <= ARC_REGNUM_R31))
#define arc_regnum_valid_s(regnum) (((regnum) >= 0) && ((regnum) <= ARC_REGNUM_R7))
#define arc_regnum_from_s(regnum) (((regnum) > 3) ? ((regnum) + 8) : (regnum))
#define arc_regnum_apex(regnum) ((regnum) >= ARC_REGNUM_APEX_FIRST) && ((regnum) <= ARC_REGNUM_APEX_LAST)
#define arc_regnum_unsupported(regnum) ((arc_regnum_reserved(regnum)) || (arc_regnum_apex(regnum)))
#define arc_regnum_supported(regnum) (!arc_regnum_unsupported(regnum))
#define arc_regnum_odd(regnum) ((regnum) & 1)
#define arc_regnum_even(regnum) (!(arc_regnum_odd(regnum)))

/* Data size (<.zz> field) for LD/ST instructions */
typedef enum {
    ARC_ZZ_WORD   = 0, /* 32-bit */
    ARC_ZZ_BYTE   = 1, /* 8-bit */
    ARC_ZZ_HALF   = 2, /* 16-bit */
    ARC_ZZ_DOUBLE = 3, /* 64-bit */
    ARC_ZZ_QUAD   = 4  /* 128-bit */
} ARCInsnDataSize;

/* Write back mode (<.aa> field) for LD/ST instructions */
typedef enum {
    ARC_AA_NONE   = 0,            /* no write-back;  EA = b + offset */
    ARC_AA_PRE    = 1,            /* pre-increment;  EA = b + offset, b = b + offset */
    ARC_AA_AW     = ARC_AA_PRE,
    ARC_AA_POST   = 2,            /* post-increment; EA = b, b = b + offset */
    ARC_AA_AB     = ARC_AA_POST,
    ARC_AA_SCALED = 3,            /* scaled index; EA = b + (index << scale), no write-back */
    ARC_AA_AS     = ARC_AA_SCALED /* scale factor: 2 for word, 1 for halfword, undefined for byte */
} ARCInsnWriteBackMode;

/* Condition codes for conditional formats */
typedef enum {
    ARC_CC_AL  = 0x00,     /* always */
    ARC_CC_RA  = ARC_CC_AL,
    ARC_CC_EQ  = 0x01,     /* equal / zero */
    ARC_CC_Z   = ARC_CC_EQ,
    ARC_CC_NE  = 0x02,     /* not equal / not zero */
    ARC_CC_NZ  = ARC_CC_NE,
    ARC_CC_PL  = 0x03,     /* positive / plus */
    ARC_CC_P   = ARC_CC_PL,
    ARC_CC_MI  = 0x04,     /* minus / negative */
    ARC_CC_N   = ARC_CC_MI,
    ARC_CC_CS  = 0x05,     /* carry set / lower than (unsigned) */
    ARC_CC_C   = ARC_CC_CS,
    ARC_CC_LO  = ARC_CC_CS,
    ARC_CC_CC  = 0x06,     /* carry clear / higher or same (unsigned) */
    ARC_CC_NC  = ARC_CC_CC,
    ARC_CC_HS  = ARC_CC_CC,
    ARC_CC_VS  = 0x07,     /* overflow set */
    ARC_CC_V   = ARC_CC_VS,
    ARC_CC_VC  = 0x08,     /* overflow clear */
    ARC_CC_NV  = ARC_CC_VC,
    ARC_CC_GT  = 0x09,     /* greater than (signed) */
    ARC_CC_GE  = 0x0A,     /* greater than or equal (signed) */
    ARC_CC_LT  = 0x0B,     /* less than (signed) */
    ARC_CC_LE  = 0x0C,     /* less than or equal (signed) */
    ARC_CC_HI  = 0x0D,     /* higher than (unsigned) */
    ARC_CC_LS  = 0x0E,     /* lower or same (unsigned) */
    ARC_CC_PNZ = 0x0F,     /* positive non-zero */
    ARC_CC_END,
} ARCInsnConditionCode;

#endif
