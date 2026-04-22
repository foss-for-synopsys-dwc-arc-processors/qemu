#ifndef ARC_CPU_AUX_H
#define ARC_CPU_AUX_H

#include "qemu/osdep.h"
#include "hw/registerfields.h"

/*
 * General AUX registers
 */

#define ARC_AUX_REGNUM_LP_START        0x002
#define ARC_AUX_REGNUM_LP_END          0x003
#define ARC_AUX_REGNUM_IDENTITY        0x004
#define ARC_AUX_REGNUM_PC              0x006
#define ARC_AUX_REGNUM_STATUS32        0x00A
#define ARC_AUX_REGNUM_STATUS32_P0     0x00B
#define ARC_AUX_REGNUM_USER_SP         0x00D
#define ARC_AUX_REGNUM_INT_VECTOR_BASE 0x025
#define ARC_AUX_REGNUM_JLI_BASE        0x290
#define ARC_AUX_REGNUM_LDI_BASE        0x291
#define ARC_AUX_REGNUM_EI_BASE         0x292
#define ARC_AUX_REGNUM_ERET            0x400
#define ARC_AUX_REGNUM_ERBTA           0x401
#define ARC_AUX_REGNUM_ERSTATUS        0x402
#define ARC_AUX_REGNUM_ECR             0x403
#define ARC_AUX_REGNUM_EFA             0x404
#define ARC_AUX_REGNUM_BTA             0x412

/*
 * Core register file configuration register
 * RF_BUILD, 0x6E, R
 *
 *     VERSION[7:0]
 *     PORTS[8] - number of ports
 *     ENTRIES[9] - number of entries
 *     CLEARED_ON_RESET[10] - cleared on reset
 *     BANKS[13:11] - number of register banks
 *     DUPLICATED[15:14] - number of duplicated registers for each RB
 */

FIELD(AUX_RF_BUILD, VERSION,           0, 8)
FIELD(AUX_RF_BUILD, PORTS,             8, 1)
FIELD(AUX_RF_BUILD, ENTRIES,           9, 1)
FIELD(AUX_RF_BUILD, CLEARED_ON_RESET, 10, 1)
FIELD(AUX_RF_BUILD, BANKS,            11, 3)
FIELD(AUX_RF_BUILD, DUPLICATED,       15, 2)

/*
 * Multiplier configuration register
 * MULTIPLY_BUILD, 0x7B, R
 *
 * VERSION32X32[7:0] - version of 32x32 multiply
 * TYPE[9:8]
 * CYC[11:10] - number of cycles
 * DSP[15:12] - DSP capabilities
 * VERSION16X16[23:16] - version of 16x16 multiply
 */

FIELD(AUX_MULTIPLY_BUILD, VERSION32X32,  0, 8)
FIELD(AUX_MULTIPLY_BUILD, TYPE,          8, 2)
FIELD(AUX_MULTIPLY_BUILD, CYC,          10, 2)
FIELD(AUX_MULTIPLY_BUILD, DSP,          12, 4)
FIELD(AUX_MULTIPLY_BUILD, VERSION16X16, 16, 8)

/*
 * Instruction set configuration register
 * ISA_CONFIG, 0xC1, R
 *
 * VERSION[7:0] - always 0x2 for ARCv2
 * PC_SIZE[11:8] - PC size option, always 0x4 (32-bit) for QEMU
 * LPC_SIZE[15:12] - zero overhead loop support, always 0x7 (32-bit) for QEMU
 * ADDR_SIZE[19:16] - always 0x4 (32-bit pointers) for QEMU
 * B[20] - byte order, always 0x0 (little endian) for QEMU
 * A[21] - atomics
 * N[22] - 0x1 if misaligned data access is supported
 * L[23] - support of LDD and STD
 * C[27:24] - version of code density
 * D[32:28] - DIV/REM version
 */

FIELD(AUX_ISA_CONFIG, VERSION,       0, 8)
#if defined(TARGET_ARCV2)
FIELD(AUX_ISA_CONFIG, PC_SIZE,       8, 4)
FIELD(AUX_ISA_CONFIG, LPC_SIZE,     12, 4)
FIELD(AUX_ISA_CONFIG, ADDR_SIZE,    16, 4)
FIELD(AUX_ISA_CONFIG, BIG_ENDIAN,   20, 1)
FIELD(AUX_ISA_CONFIG, ATOMIC,       21, 1)
FIELD(AUX_ISA_CONFIG, NON_ALIGNED,  22, 1)
FIELD(AUX_ISA_CONFIG, LL64,         23, 1)
FIELD(AUX_ISA_CONFIG, CODE_DENSITY, 24, 4)
FIELD(AUX_ISA_CONFIG, DIV_REM,      28, 4)
#elif defined(TARGET_ARCV3_32)
FIELD(AUX_ISA_CONFIG, PC_SIZE,       8, 4)
FIELD(AUX_ISA_CONFIG, LPC_SIZE,     12, 4)
FIELD(AUX_ISA_CONFIG, ADDR_SIZE,    16, 4)
FIELD(AUX_ISA_CONFIG, BIG_ENDIAN,   20, 1)
FIELD(AUX_ISA_CONFIG, ATOMIC,       21, 2)
FIELD(AUX_ISA_CONFIG, NON_ALIGNED,  23, 1)
FIELD(AUX_ISA_CONFIG, LL64,         24, 1)
FIELD(AUX_ISA_CONFIG, CODE_DENSITY, 25, 2)
FIELD(AUX_ISA_CONFIG, DIV_REM,      28, 2)
#elif defined(TARGET_ARCV3_64)
FIELD(AUX_ISA_CONFIG, VA_SIZE,       8, 4)
FIELD(AUX_ISA_CONFIG, PA_SIZE,      16, 4)
FIELD(AUX_ISA_CONFIG, BIG_ENDIAN,   20, 1)
FIELD(AUX_ISA_CONFIG, ATOMIC,       21, 2)
FIELD(AUX_ISA_CONFIG, NON_ALIGNED,  23, 1)
FIELD(AUX_ISA_CONFIG, M128,         24, 2)
FIELD(AUX_ISA_CONFIG, CODE_DENSITY, 26, 2)
FIELD(AUX_ISA_CONFIG, DIV_REM,      28, 2)
#else
#error "Unsupported target"
#endif

/*
 * Core identity register
 * IDENTITY, 0x04, r
 *
 * ARCVER[7:0] - ARC baseline instruction set version number
 * ARCNUM[15:8] - a unique ID of a core from 0 to 255 
 */

FIELD(AUX_IDENTITY, ARCVER, 0, 8)
FIELD(AUX_IDENTITY, ARCNUM, 8, 8)

#define ARC_AUX_IDENTITY_VERSION_EM   0x44
#define ARC_AUX_IDENTITY_VERSION_HS   0x54
#define ARC_AUX_IDENTITY_VERSION_HS5X 0x60
#define ARC_AUX_IDENTITY_VERSION_HS6X 0x70

/*
 * Micro architecture configuration register
 * MICRO_ARCH_BUILD, 0xF9, R
 *
 * MINOR_REV[7:0] - minor revision number
 * MAJOR_REV[15:8] - major revision number
 * FAMILY[23:16] - product family
 */

FIELD(AUX_MICRO_ARCH_BUILD, MINOR_REV,  0, 8)
FIELD(AUX_MICRO_ARCH_BUILD, MAJOR_REV,  8, 8)
FIELD(AUX_MICRO_ARCH_BUILD, FAMILY,    16, 8)

#define ARC_AUX_MICRO_ARCH_BUILD_FAMILY_EM   0x1
#define ARC_AUX_MICRO_ARCH_BUILD_FAMILY_HS   0x4
#define ARC_AUX_MICRO_ARCH_BUILD_FAMILY_HS5X 0x5
#define ARC_AUX_MICRO_ARCH_BUILD_FAMILY_HS6X 0x6

/*
 * Interrupt vector base address configuration
 * VECBASE_AC_BUILD, 0x68, R
 *
 * CONFIG[1:0] - configuration of the reset value for INT_VECTOR_BASE
 * VERSION[9:2] - version in the interrupt unit
 * ADDR[31:10] - interrupt vector base address
 */

FIELD(AUX_VECBASE_AC_BUILD, CONFIG,   0,  2)
FIELD(AUX_VECBASE_AC_BUILD, VERSION,  2,  8)
#ifdef TARGET_ARCV3_64
FIELD(AUX_VECBASE_AC_BUILD, ADDR,    11, 21)
#else
FIELD(AUX_VECBASE_AC_BUILD, ADDR,    10, 22)
#endif

#ifdef TARGET_ARCV2
#define AUX_VECBASE_AC_BUILD_VERSION 0x4 /* ARCv2 */
#else
#define AUX_VECBASE_AC_BUILD_VERSION 0x5 /* ARCv3 */
#endif

/*
 * Status register
 * STATUS32, 0x0A, RW
 * ERSTATUS, 0x402, RW
 *
 * H[0] - halt flag (reserved in ERSTATUS)
 * E[4:1] - interrupt priority operating level
 * AE[5] - processor is in an exception state
 * DE[6] - delayed branch is pending
 * U[7] - user mode
 * V[8] - overflow status flag
 * C[9] - carry status flag
 * N[10] - negative status flag
 * Z[11] - zero status flag
 * L[12] - zero-overhead loop disable
 * DZ[13] - EV_DivZero exception enable
 * SC[14] - enable stack checking
 * ES[15] - EI_S table instruction pending
 * RB[18:16] - select a register bank
 * AD[19] - disable alignment checking
 * US[20] - user sleep mode enable
 * IE[31] - interrupt enable
 */

FIELD(AUX_STATUS32, Hf,   0, 1)
FIELD(AUX_STATUS32, Ef,   1, 4)
FIELD(AUX_STATUS32, AEf,  5, 1)
FIELD(AUX_STATUS32, DEf,  6, 1)
FIELD(AUX_STATUS32, Uf,   7, 1)
FIELD(AUX_STATUS32, Vf,   8, 1)
FIELD(AUX_STATUS32, Cf,   9, 1)
FIELD(AUX_STATUS32, Nf,  10, 1)
FIELD(AUX_STATUS32, Zf,  11, 1)
FIELD(AUX_STATUS32, Lf,  12, 1)
FIELD(AUX_STATUS32, DZf, 13, 1)
FIELD(AUX_STATUS32, SCf, 14, 1)
FIELD(AUX_STATUS32, ESf, 15, 1)
FIELD(AUX_STATUS32, RBf, 16, 3)
FIELD(AUX_STATUS32, ADf, 19, 1)
FIELD(AUX_STATUS32, USf, 20, 1)
FIELD(AUX_STATUS32, IEf, 31, 1)

typedef struct {
    uint32_t halt;
    uint32_t interrupt_priority;
    uint32_t exception_state;
    uint32_t delay_slot_pending;
    uint32_t user_mode;
    uint32_t overflow_flag;
    uint32_t carry_flag;
    uint32_t negative_flag;
    uint32_t zero_flag;
    uint32_t disable_zol;
    uint32_t enable_divzero_excp;
    uint32_t enable_stack_checking;
    uint32_t ei_pending;
    uint32_t register_bank;
    uint32_t disable_alignment_checking;
    uint32_t sleep_mode;
    uint32_t enable_interrupt;
} arc_aux_status_t;

static inline uint32_t arc_pack_status32(const arc_aux_status_t *status)
{
    uint32_t res = 0x0;

    res = FIELD_DP32(res, AUX_STATUS32, Hf, status->halt);
    res = FIELD_DP32(res, AUX_STATUS32, Ef, status->interrupt_priority);
    res = FIELD_DP32(res, AUX_STATUS32, AEf, status->exception_state);
    res = FIELD_DP32(res, AUX_STATUS32, DEf, status->delay_slot_pending);
    res = FIELD_DP32(res, AUX_STATUS32, Uf, status->user_mode);
    res = FIELD_DP32(res, AUX_STATUS32, Vf, status->overflow_flag);
    res = FIELD_DP32(res, AUX_STATUS32, Cf, status->carry_flag);
    res = FIELD_DP32(res, AUX_STATUS32, Nf, status->negative_flag);
    res = FIELD_DP32(res, AUX_STATUS32, Zf, status->zero_flag);
    res = FIELD_DP32(res, AUX_STATUS32, Lf, status->disable_zol);
    res = FIELD_DP32(res, AUX_STATUS32, DZf, status->enable_divzero_excp);
    res = FIELD_DP32(res, AUX_STATUS32, SCf, status->enable_stack_checking);
    res = FIELD_DP32(res, AUX_STATUS32, ESf, status->ei_pending);
    res = FIELD_DP32(res, AUX_STATUS32, RBf, status->register_bank);
    res = FIELD_DP32(res, AUX_STATUS32, ADf, status->disable_alignment_checking);
    res = FIELD_DP32(res, AUX_STATUS32, USf, status->sleep_mode);
    res = FIELD_DP32(res, AUX_STATUS32, IEf, status->enable_interrupt);

    return res;
}

static inline void arc_unpack_status32(arc_aux_status_t *status, uint32_t value)
{
    status->halt  = FIELD_EX32(value, AUX_STATUS32, Hf);
    status->interrupt_priority  = FIELD_EX32(value, AUX_STATUS32, Ef);
    status->exception_state = FIELD_EX32(value, AUX_STATUS32, AEf);
    status->delay_slot_pending = FIELD_EX32(value, AUX_STATUS32, DEf);
    status->user_mode  = FIELD_EX32(value, AUX_STATUS32, Uf);
    status->overflow_flag  = FIELD_EX32(value, AUX_STATUS32, Vf);
    status->carry_flag  = FIELD_EX32(value, AUX_STATUS32, Cf);
    status->negative_flag  = FIELD_EX32(value, AUX_STATUS32, Nf);
    status->zero_flag  = FIELD_EX32(value, AUX_STATUS32, Zf);
    status->disable_zol  = FIELD_EX32(value, AUX_STATUS32, Lf);
    status->enable_divzero_excp = FIELD_EX32(value, AUX_STATUS32, DZf);
    status->enable_stack_checking = FIELD_EX32(value, AUX_STATUS32, SCf);
    status->ei_pending = FIELD_EX32(value, AUX_STATUS32, ESf);
    status->register_bank = FIELD_EX32(value, AUX_STATUS32, RBf);
    status->disable_alignment_checking = FIELD_EX32(value, AUX_STATUS32, ADf);
    status->sleep_mode = FIELD_EX32(value, AUX_STATUS32, USf);
    status->enable_interrupt = FIELD_EX32(value, AUX_STATUS32, IEf);
}

FIELD(AUX_ERSTATUS, Ef,   1, 4)
FIELD(AUX_ERSTATUS, AEf,  5, 1)
FIELD(AUX_ERSTATUS, DEf,  6, 1)
FIELD(AUX_ERSTATUS, Uf,   7, 1)
FIELD(AUX_ERSTATUS, Vf,   8, 1)
FIELD(AUX_ERSTATUS, Cf,   9, 1)
FIELD(AUX_ERSTATUS, Nf,  10, 1)
FIELD(AUX_ERSTATUS, Zf,  11, 1)
FIELD(AUX_ERSTATUS, Lf,  12, 1)
FIELD(AUX_ERSTATUS, DZf, 13, 1)
FIELD(AUX_ERSTATUS, SCf, 14, 1)
FIELD(AUX_ERSTATUS, ESf, 15, 1)
FIELD(AUX_ERSTATUS, RBf, 16, 3)
FIELD(AUX_ERSTATUS, ADf, 19, 1)
FIELD(AUX_ERSTATUS, USf, 20, 1)
FIELD(AUX_ERSTATUS, IEf, 31, 1)

static inline uint32_t arc_pack_aux_erstatus(const arc_aux_status_t *reg)
{
    /* Mask STATUS32.H flag for ERSTATUS */
    return arc_pack_status32(reg) & ~1;
}

static inline void arc_unpack_aux_erstatus(arc_aux_status_t *reg, uint32_t value)
{
    arc_unpack_status32(reg, value);
    reg->halt = 0;
}

/*
 * Exception cause register
 * ECR, 0x403, RW
 *
 *     PARAMETER[7:0]
 *     CAUSE_CODE[15:8] - a code of a subtype of an exception
 *     VECTOR_NUMBER[23:16] - a code of a type of an exception
 *     FROM_USER[30] - whether an exception was entered from User mode
 *     FROM_INT_PROLOGUE[31] - whether an exception was entered in an interrupt prologue
 */

FIELD(AUX_ECR, PARAMETER,          0, 8)
FIELD(AUX_ECR, CAUSE_CODE,         8, 8)
FIELD(AUX_ECR, VECTOR_NUMBER,     16, 8)
FIELD(AUX_ECR, FROM_USER,         30, 1)
FIELD(AUX_ECR, FROM_INT_PROLOGUE, 31, 1)

typedef struct {
    uint32_t parameter;
    uint32_t cause_code;
    uint32_t vector_number;
    uint32_t from_user;
    uint32_t from_int_prologue;
} arc_aux_ecr_t;

static inline uint32_t arc_pack_aux_ecr(const arc_aux_ecr_t *reg)
{
    uint32_t res = 0x0;

    res = FIELD_DP32(res, AUX_ECR, PARAMETER, reg->parameter);
    res = FIELD_DP32(res, AUX_ECR, CAUSE_CODE, reg->cause_code);
    res = FIELD_DP32(res, AUX_ECR, VECTOR_NUMBER, reg->vector_number);
    res = FIELD_DP32(res, AUX_ECR, FROM_USER, reg->from_user);
    res = FIELD_DP32(res, AUX_ECR, FROM_INT_PROLOGUE, reg->from_int_prologue);

    return res;
}

static inline void arc_unpack_aux_ecr(arc_aux_ecr_t *reg, uint32_t value)
{
    reg->parameter = FIELD_EX32(value, AUX_ECR, PARAMETER);
    reg->cause_code = FIELD_EX32(value, AUX_ECR, CAUSE_CODE);
    reg->vector_number = FIELD_EX32(value, AUX_ECR, VECTOR_NUMBER);
    reg->from_user = FIELD_EX32(value, AUX_ECR, FROM_USER);
    reg->from_int_prologue = FIELD_EX32(value, AUX_ECR, FROM_INT_PROLOGUE);
}

/*
 * Timer controls register
 * CONTROL0/1, 0x22/0x101, RW
 *
 * IE[0] - the interrupt enable flag
 * NH[1] - the not halted mode flag
 * W[2] - the watchdog mode flag
 * IP[3] - this bit is set when COUNTn reaches LIMITn
 * TD[4] - this flag controls timers behavior in power-down mode
 */

FIELD(AUX_TIMER_CONTROL, IE, 0, 1)
FIELD(AUX_TIMER_CONTROL, NH, 1, 1)
FIELD(AUX_TIMER_CONTROL, W,  2, 1)
FIELD(AUX_TIMER_CONTROL, IP, 3, 1)
FIELD(AUX_TIMER_CONTROL, TD, 4, 1)

/* Filter only supported fields */
static inline uint32_t arc_filter_aux_timer_control(uint32_t reg)
{
    uint32_t tmp;
    uint32_t res = 0;

    tmp = FIELD_EX32(reg, AUX_TIMER_CONTROL, IE);
    res = FIELD_DP32(res, AUX_TIMER_CONTROL, IE, tmp);

    tmp = FIELD_EX32(reg, AUX_TIMER_CONTROL, IP);
    res = FIELD_DP32(res, AUX_TIMER_CONTROL, IP, tmp);

    return res;
}

/*
 * Timers configuration register
 * TIMER_BUILD, 0x75, R
 *
 * VERSION[7:0] - 0x4 for ARCv2
 * TIMER0[8] - Timer 0 present
 * TIMER1[9] - Timer 1 present
 * RTC[10] - RTC present
 * PRIORITY0[19:16] - the interrupt priority level of Timer 0
 * PRIORITY1[23:20] - the interrupt priority level of Timer 1
 */

FIELD(AUX_TIMER_BUILD, VERSION,    0, 8)
FIELD(AUX_TIMER_BUILD, TIMER0,     8, 1)
FIELD(AUX_TIMER_BUILD, TIMER1,     9, 1)
FIELD(AUX_TIMER_BUILD, RTC,       10, 1)
FIELD(AUX_TIMER_BUILD, PRIORITY0, 16, 4)
FIELD(AUX_TIMER_BUILD, PRIORITY1, 20, 4)

/*
 * Interrupts subsystem architecture
 *
 * General interrupts registers:
 *
 *     IRQ_BUILD
 *     AUX_IRQ_CTRL - controls automated registers saving
 *     AUX_IRQ_ACT - a list of all taken IRQs, inferred from arc_banked_irq_t.taken
 *     IRQ_PRIORITY_PENDING - inferred from arc_banked_irq_t.priority
 *     ICAUSE - a number of an active (taken) IRQ with the highest priority,
                inferred from arc_banked_irq_t.taken
 *     AUX_IRQ_HINT
 *     AUX_USER_SP
 *     STATUS32_P0 (RAZ/WI when FIRQ is disabled)
 *
 * Interrupt vector base registers:
 *
 *     INT_VECTOR_BASE - base address for IRQ handlers
 *     VECBASE_AC_BUILD - a default base address
 *
 * IRQ_SELECT selects this set of banked AUX registers:
 *
 *     IRQ_ENABLE
 *     IRQ_PENDING
 *     IRQ_PRIORITY
 *     IRQ_STATUS
 *     IRQ_TRIGGER (not implemented) - is it a pulse sensitive interrupt?
 *     IRQ_PULSE_CANCEL (not implemented) - cancel a pulse sensitive interrupt
 */

/*
 * Interrupt context saving control register
 * AUX_IRQ_CTRL, 0x0E, RW
 *
 *     NR[4:0] - number of general-purpose register pairs saved
 *     BLINK[9] - save and restore BLINK (ignored if NR >= 16)
 *     LOOP[10] - save and restore LP_COUNT, LP_START and LP_END
 *     USER_STACK[11] - user context is saved to user stack
 *     CODE_DENSITY[13] - save and restore EI_BASE, JLI_BASe and LDI_BASE
 */

FIELD(AUX_IRQ_CTRL, NR,            0, 5)
FIELD(AUX_IRQ_CTRL, BLINK,         9, 1)
FIELD(AUX_IRQ_CTRL, LOOP,         10, 1)
FIELD(AUX_IRQ_CTRL, USER_STACK,   11, 1)
FIELD(AUX_IRQ_CTRL, CODE_DENSITY, 13, 1)

typedef struct {
    uint32_t nr;
    uint32_t blink;
    uint32_t loop;
    uint32_t user_stack;
    uint32_t code_density;
} arc_aux_irq_ctrl_t;

static inline uint32_t arc_pack_irq_ctrl(const arc_aux_irq_ctrl_t *reg)
{
    uint32_t res = 0x0;

    res = FIELD_DP32(res, AUX_IRQ_CTRL, NR, reg->nr);
    res = FIELD_DP32(res, AUX_IRQ_CTRL, BLINK, reg->blink);
    res = FIELD_DP32(res, AUX_IRQ_CTRL, LOOP, reg->loop);
    res = FIELD_DP32(res, AUX_IRQ_CTRL, USER_STACK, reg->user_stack);
    res = FIELD_DP32(res, AUX_IRQ_CTRL, CODE_DENSITY, reg->code_density);

    return res;
}

static inline void arc_unpack_aux_irq_ctrl(arc_aux_irq_ctrl_t *reg, uint32_t value)
{
    reg->nr = FIELD_EX32(value, AUX_IRQ_CTRL, NR);
    reg->blink = FIELD_EX32(value, AUX_IRQ_CTRL, BLINK);
    reg->loop = FIELD_EX32(value, AUX_IRQ_CTRL, LOOP);
    reg->user_stack = FIELD_EX32(value, AUX_IRQ_CTRL, USER_STACK);
    reg->code_density = FIELD_EX32(value, AUX_IRQ_CTRL, CODE_DENSITY);

    /* This field saturates at 16 for GPR file with 32 registers */
    if (reg->nr >= 16) {
        reg->nr = 16;
    }
}

/*
 * Interrupt build configuration register
 * IRQ_BUILD, 0xF3, R
 *
 *     VERSION[7:0] - version of the Interrupt Controller
 *     IRQS[15:8] - the number of supported interrupts
 *     EXTS[23:16] - the number of supported external interrupts
 *     PRIORITIES[27:24] - contains N-1, when N interrupt priority levels are configured
 *     FIRQ[28] - value of the FIRQ_OPTION configuration option
 *     NMI[30:29] - whether the non-maskable imprecise exception is configured
 */

FIELD(AUX_IRQ_BUILD, VERSION,     0, 8)
FIELD(AUX_IRQ_BUILD, IRQS,        8, 8)
FIELD(AUX_IRQ_BUILD, EXTS,       16, 8)
FIELD(AUX_IRQ_BUILD, PRIORITIES, 24, 4)
FIELD(AUX_IRQ_BUILD, FIRQ,       28, 1)
FIELD(AUX_IRQ_BUILD, NMI,        29, 2)

/*
 * Interrupt status register
 * IRQ_STATUS, 0x40F, R
 *
 * PRIORITY[3:0]
 * ENABLE[4]
 * TRIGGER[5]
 * PENDING[31]
 */

FIELD(AUX_IRQ_STATUS, PRIORITY, 0, 4)
FIELD(AUX_IRQ_STATUS, ENABLE,   4, 1)
FIELD(AUX_IRQ_STATUS, TRIGGER,  5, 1)
FIELD(AUX_IRQ_STATUS, PENDING, 31, 1)

typedef struct {
    uint32_t priority;
    uint32_t enable;
    uint32_t trigger;
    uint32_t pending;
} arc_aux_irq_status_t;

static inline uint32_t arc_pack_irq_status(const arc_aux_irq_status_t *reg)
{
    uint32_t res = 0x0;

    res = FIELD_DP32(res, AUX_IRQ_STATUS, PRIORITY, reg->priority);
    res = FIELD_DP32(res, AUX_IRQ_STATUS, ENABLE, reg->enable);
    res = FIELD_DP32(res, AUX_IRQ_STATUS, TRIGGER, reg->trigger);
    res = FIELD_DP32(res, AUX_IRQ_STATUS, PENDING, reg->pending);

    return res;
}

/*
 * Active interrupts register
 * AUX_IRQ_ACT, 0x43, RW
 *
 * ACTIVE[15:0]
 * FROM_USER[31]
 */

FIELD(AUX_IRQ_ACT, ACTIVE,     0, 16)
FIELD(AUX_IRQ_ACT, FROM_USER, 31,  1)

typedef struct {
    uint32_t active;
    uint32_t from_user;
} arc_aux_irq_act_t;

static inline uint32_t arc_pack_aux_irq_act(const arc_aux_irq_act_t *reg)
{
    uint32_t res = 0x0;

    res = FIELD_DP32(res, AUX_IRQ_ACT, ACTIVE, reg->active);
    res = FIELD_DP32(res, AUX_IRQ_ACT, FROM_USER, reg->from_user);

    return res;
}

static inline void arc_unpack_aux_irq_act(arc_aux_irq_act_t *reg, uint32_t value)
{
    reg->active = FIELD_EX32(value, AUX_IRQ_ACT, ACTIVE);
    reg->from_user = FIELD_EX32(value, AUX_IRQ_ACT, FROM_USER);
}

/*
 * TLB index register
 * TLBIndex, 0x464, RW
 *
 *     INDEX[12:0] - index of a TLB entry for TLBWrite/TLBRead commands
 *     RC[30:28] - result code, set by hardware (read-only)
 *     E[31] - error flag, set by hardware when an error occurred (read-only)
 */

FIELD(AUX_MMU_TLBINDEX, INDEX,  0, 13)
FIELD(AUX_MMU_TLBINDEX, RC,    28,  3)
FIELD(AUX_MMU_TLBINDEX, E,     31,  1)

typedef struct {
    uint32_t index;
    uint32_t rc;    /* result code, set by hardware */
    bool error;     /* error flag, set by hardware */
} arc_aux_mmu_tlbindex_t;

static inline uint32_t arc_pack_aux_mmu_tlbindex(const arc_aux_mmu_tlbindex_t *reg)
{
    uint32_t res = 0x0;

    res = FIELD_DP32(res, AUX_MMU_TLBINDEX, INDEX, reg->index);
    res = FIELD_DP32(res, AUX_MMU_TLBINDEX, RC, reg->rc);
    res = FIELD_DP32(res, AUX_MMU_TLBINDEX, E, reg->error);

    return res;
}

static inline void arc_unpack_aux_mmu_tlbindex(arc_aux_mmu_tlbindex_t *reg, uint32_t value)
{
    reg->index = FIELD_EX32(value, AUX_MMU_TLBINDEX, INDEX);
    reg->rc = FIELD_EX32(value, AUX_MMU_TLBINDEX, RC);
    reg->error = FIELD_EX32(value, AUX_MMU_TLBINDEX, E);
}

/*
 * TLB command register
 * TLBCommand, 0x465, W
 *
 *     CMD[5:0] - the TLB maintenance command to execute (see the codes below)
 */

FIELD(AUX_MMU_TLBCOMMAND, CMD, 0, 6)

#ifdef TARGET_ARCV2
/*
 * MMU build configuration register
 * MMU_BUILD, 0x6F, R
 *
 *     DTLB[2:0] - number of mDTLB entries
 *     ITLB[5:3] - number of mITLB entries
 *     JES[7:6] - joint super page TLB (STLB) entries
 *     JE[9:8] - joint normal page TLB (NTLB) size
 *     JA[11:10] - joint NTLB associativity (number of ways)
 *     PAE[12] - physical address extension (40-bit) enable
 *     CT[13] - CCM translation support
 *     DL[14] - dynamic loading support
 *     PSZ0[18:15] - normal page size
 *     PSZ1[22:19] - super page size
 *     SL[23] - shared library ASID (SASID) registers present
 *     VERSION[31:24] - MMU version
 */

FIELD(AUX_MMU_BUILD, DTLB,     0, 3)
FIELD(AUX_MMU_BUILD, ITLB,     3, 3)
FIELD(AUX_MMU_BUILD, JES,      6, 2)
FIELD(AUX_MMU_BUILD, JE,       8, 2)
FIELD(AUX_MMU_BUILD, JA,      10, 2)
FIELD(AUX_MMU_BUILD, PAE,     12, 1)
FIELD(AUX_MMU_BUILD, CT,      13, 1)
FIELD(AUX_MMU_BUILD, DL,      14, 1)
FIELD(AUX_MMU_BUILD, PSZ0,    15, 4)
FIELD(AUX_MMU_BUILD, PSZ1,    19, 4)
FIELD(AUX_MMU_BUILD, SL,      23, 1)
FIELD(AUX_MMU_BUILD, VERSION, 24, 8)

#define ARC_MMU_BUILD_DTLB_ENTRIES_8  0x2
#define ARC_MMU_BUILD_DTLB_ENTRIES_16 0x3
#define ARC_MMU_BUILD_ITLB_ENTRIES_4  0x1
#define ARC_MMU_BUILD_ITLB_ENTRIES_16 0x3

#define ARC_MMU_BUILD_PAGE_SIZE_NONE  0x0
#define ARC_MMU_BUILD_PAGE_SIZE_4K    0x3
#define ARC_MMU_BUILD_PAGE_SIZE_8K    0x4
#define ARC_MMU_BUILD_PAGE_SIZE_16K   0x5
#define ARC_MMU_BUILD_PAGE_SIZE_32K   0x6
#define ARC_MMU_BUILD_PAGE_SIZE_64K   0x7
#define ARC_MMU_BUILD_PAGE_SIZE_128K  0x8
#define ARC_MMU_BUILD_PAGE_SIZE_256K  0x9
#define ARC_MMU_BUILD_PAGE_SIZE_512K  0xA
#define ARC_MMU_BUILD_PAGE_SIZE_1024K 0xB
#define ARC_MMU_BUILD_PAGE_SIZE_2M    0xC
#define ARC_MMU_BUILD_PAGE_SIZE_4M    0xD
#define ARC_MMU_BUILD_PAGE_SIZE_8M    0xE
#define ARC_MMU_BUILD_PAGE_SIZE_16M   0xF

#define ARC_MMU_BUILD_VERSION_V4 0x4
#define ARC_MMU_BUILD_VERSION_V5 0x5

/*
 * TLB page descriptor register for 32-bit virtual address
 * TLBPD0, 0x460, RW
 *
 *     A[7:0] - address space identifier (ASID)
 *     G[8] - whether the page is global
 *     V[9] - whether the pase is valid (1) or can be discarded (0)
 *     SZ[10] - 0 for normal page size and 1 for super page size
 *     L[11] - locking bit, present only in MMUv5
 *     VPN[30:12] - virtual page number
 *     S[31] - shared library ASID (not present without SASID)
 */

FIELD(AUX_MMU_TLBPD0, A,    0,  8)
FIELD(AUX_MMU_TLBPD0, G,    8,  1)
FIELD(AUX_MMU_TLBPD0, V,    9,  1)
FIELD(AUX_MMU_TLBPD0, SZ,  10,  1)
FIELD(AUX_MMU_TLBPD0, L,   11,  1)
FIELD(AUX_MMU_TLBPD0, VPN, 12, 19)
FIELD(AUX_MMU_TLBPD0, S,   31,  1)

/*
 * TLB page descriptor register for 32-bit physical address
 * TLBPD1, 0x461, RW
 *
 *     FC[0] - 0 if access must be uncached and 1 otherwise
 *     EU[1] - user execute permission
 *     WU[2] - user write permission
 *     RU[3] - user read permission
 *     EK[4] - kernel execute permission
 *     WK[5] - kernel write permission
 *     RK[6] - kernel read permission
 *     PPN[31:12] - physical page nnumber
 */

FIELD(AUX_MMU_TLBPD1, FC,   0,  1)
FIELD(AUX_MMU_TLBPD1, EU,   1,  1)
FIELD(AUX_MMU_TLBPD1, WU,   2,  1)
FIELD(AUX_MMU_TLBPD1, RU,   3,  1)
FIELD(AUX_MMU_TLBPD1, EK,   4,  1)
FIELD(AUX_MMU_TLBPD1, WK,   5,  1)
FIELD(AUX_MMU_TLBPD1, RK,   6,  1)
FIELD(AUX_MMU_TLBPD1, PPN, 12, 20)
/*
 * Process identity register
 * PID, 0x468, RW
 *
 *     ASID[7:0] - address space identifier of the current process
 *     T[31] - global TLB enable (the MMU is enabled when set)
 *
 * The S[30] bit (shared library ASID enable) is not present without SASID.
 */

FIELD(AUX_MMU_PID, ASID,  0,  8)
FIELD(AUX_MMU_PID, T,    31,  1)

typedef struct {
    uint32_t asid;  /* address space identifier of the current process */
    bool enabled;   /* T: the MMU is enabled */
} arc_aux_mmu_pid_t;

static inline uint32_t arc_pack_aux_mmu_pid(const arc_aux_mmu_pid_t *reg)
{
    uint32_t res = 0x0;

    res = FIELD_DP32(res, AUX_MMU_PID, ASID, reg->asid);
    res = FIELD_DP32(res, AUX_MMU_PID, T, reg->enabled);

    return res;
}

static inline void arc_unpack_aux_mmu_pid(arc_aux_mmu_pid_t *reg, uint32_t value)
{
    reg->asid = FIELD_EX32(value, AUX_MMU_PID, ASID);
    reg->enabled = FIELD_EX32(value, AUX_MMU_PID, T);
}

typedef enum {
    ARC_MMU_TLB_CMD_WRITE    = 0x1, /* write entry at TLBIndex, flush uTLBs */
    ARC_MMU_TLB_CMD_READ     = 0x2, /* read entry at TLBIndex into TLBPD0/1 */
    ARC_MMU_TLB_CMD_GETINDEX = 0x3, /* return a victim index for TLBPD0 */
    ARC_MMU_TLB_CMD_PROBE    = 0x4, /* return the index matching TLBPD0 */
    ARC_MMU_TLB_CMD_WRITENI  = 0x5, /* write entry at TLBIndex, keep uTLBs */
    ARC_MMU_TLB_CMD_IVUTLB   = 0x6, /* invalidate uTLBs */
    ARC_MMU_TLB_CMD_INSERT   = 0x7, /* insert entry for the VPN in TLBPD0 */
    ARC_MMU_TLB_CMD_DELETE   = 0x8, /* delete entry for the VPN in TLBPD0 */
} ARCMMUTLBCommand;

/*
 * Joint TLB geometry. HS4x uses a 4-way set associative TLB with 1024
 * normal-page entries (256 sets). Super pages (STLB) are not supported.
 */
#define ARC_MMU_TLB_WAYS    4
#define ARC_MMU_TLB_SETS    256
#define ARC_MMU_TLB_ENTRIES (ARC_MMU_TLB_SETS * ARC_MMU_TLB_WAYS)

/* A single TLB entry is the {TLBPD0, TLBPD1} pair, kept in packed form. */
typedef struct {
    uint32_t pd0;
    uint32_t pd1;
} arc_mmu_tlb_entry_t;
#endif

#if defined(TARGET_ARCV3_32) || defined(TARGET_ARCV3_64)
/*
 * MMU build configuration register
 * MMU_BUILD, 0x6F, R
 *
 *     DTLB[2:0] - the number of mDTLB entries
 *     ITLB[5:3] - the number of mITLB entries
 *     L2TLB[8:6] - the number of entries in the four-way associative L2 TLB
 *     TC[9] - 0x1 if MMU translation cache is available
 *     TYPE[23:21]
 *     VERSION[31:24]
 */

FIELD(AUX_MMU_BUILD, DTLB,     0, 3)
FIELD(AUX_MMU_BUILD, ITLB,     3, 3)
FIELD(AUX_MMU_BUILD, L2TLB,    6, 3)
FIELD(AUX_MMU_BUILD, TC,       9, 1)
FIELD(AUX_MMU_BUILD, TYPE,    21, 3)
FIELD(AUX_MMU_BUILD, VERSION, 24, 8)

typedef enum {
    ARC_MMU_TLB_CMD_INVALIDATE_ALL    = 0x1,
    ARC_MMU_TLB_CMD_READ              = 0x2,
    ARC_MMU_TLB_CMD_INVALIDATE_ASID   = 0x3,
    ARC_MMU_TLB_CMD_INVALIDATE_ADDR   = 0x4,
    ARC_MMU_TLB_CMD_INVALIDATE_REGION = 0x5
} ARCMMUTLBCommand;

FIELD(AUX_MMU_TTBC, T0SZ,  0, 5)
FIELD(AUX_MMU_TTBC, A1,   15, 1)
FIELD(AUX_MMU_TTBC, T1SZ, 16, 5)

/*
 * MMU control register
 * MMU_CTRL, 0x468, RW
 *
 *     EN[0] - enable MMU
 *     KU[1] - kernel mode access to user mode data
 *     WX[2] - writeable pages execution behavior
 *     TCE[3] - translation cache enable
 */

FIELD(AUX_MMU_CTRL, EN,  0, 1)
FIELD(AUX_MMU_CTRL, KU,  1, 1)
FIELD(AUX_MMU_CTRL, WX,  2, 1)
FIELD(AUX_MMU_CTRL, TCE, 3, 1)

#endif

/*
 * Instruction cache configuration register
 * I_CACHE_BUILD, 0x77, R
 *
 * VERSION[7:0] - 0x4 for ARCv2
 * WAYS[11:8] - cache associativity
 * SIZE[15:12] - cache capacity
 * BSIZE[19:16] - cache block size in bytes
 * FL[21:20] - feature level, always 0x2
 * D[22] - if IC is disabled on reset
 */

FIELD(AUX_I_CACHE_BUILD, VERSION,            0, 8)
FIELD(AUX_I_CACHE_BUILD, WAYS,               8, 4)
FIELD(AUX_I_CACHE_BUILD, SIZE,              12, 4)
FIELD(AUX_I_CACHE_BUILD, BLOCK_SIZE,        16, 4)
FIELD(AUX_I_CACHE_BUILD, FEATURE_LEVEL,     20, 2)
FIELD(AUX_I_CACHE_BUILD, DISABLED_ON_RESET, 22, 1)

/*
 * Instruction cache configuration register
 * D_CACHE_BUILD, 0x72, R
 *
 * VERSION[7:0] - 0x4 for ARCv2
 * WAYS[11:8] - cache associativity
 * SIZE[15:12] - cache capacity
 * BSIZE[19:16] - cache block size in bytes
 * FL[21:20] - feature level, always 0x2
 * UNCACHED[26] - whether the data cache contains uncached regions
 * CYCLES[29:27]
 */

FIELD(AUX_D_CACHE_BUILD, VERSION,        0, 8)
FIELD(AUX_D_CACHE_BUILD, WAYS,           8, 4)
FIELD(AUX_D_CACHE_BUILD, SIZE,          12, 4)
FIELD(AUX_D_CACHE_BUILD, BLOCK_SIZE,    16, 4)
FIELD(AUX_D_CACHE_BUILD, FEATURE_LEVEL, 20, 2)
FIELD(AUX_D_CACHE_BUILD, UNCACHED,      26, 1)
FIELD(AUX_D_CACHE_BUILD, CYCLES,        27, 3)

/*
 * Instruction cache control
 * IC_CTRL, 0x11, RW
 *
 * DC[0] - 1 if cache is disabled
 * SB[3] - success of last cache operation
 * AT[5] - address debug type
 */

FIELD(AUX_IC_CTRL, DISABLE,            0, 1)
FIELD(AUX_IC_CTRL, SUCCESS,            3, 1)
FIELD(AUX_IC_CTRL, ADDRESS_DEBUG_TYPE, 5, 1)

typedef struct {
    uint32_t disable;
    uint32_t success;
    uint32_t address_debug_type;
} arc_aux_ic_ctrl_t;

static inline uint32_t arc_pack_aux_ic_ctrl(const arc_aux_ic_ctrl_t *reg)
{
    uint32_t res = 0x0;

    res = FIELD_DP32(res, AUX_IC_CTRL, DISABLE, reg->disable);
    res = FIELD_DP32(res, AUX_IC_CTRL, SUCCESS, reg->success);
    res = FIELD_DP32(res, AUX_IC_CTRL, ADDRESS_DEBUG_TYPE, reg->address_debug_type);

    return res;
}

static inline void arc_unpack_aux_ic_ctrl(arc_aux_ic_ctrl_t *reg, uint32_t value)
{
    reg->disable = FIELD_EX32(value, AUX_IC_CTRL, DISABLE);
    reg->success = FIELD_EX32(value, AUX_IC_CTRL, SUCCESS);
    reg->address_debug_type = FIELD_EX32(value, AUX_IC_CTRL, ADDRESS_DEBUG_TYPE);
}

/*
 * Instruction cache control
 * DC_CTRL, 0x11, RW
 *
 * DC[0] - 1 if cache is disabled
 * SB[2] - success of last cache operation
 * AT[5] - address debug type
 * ...
 */

FIELD(AUX_DC_CTRL, DISABLE,            0, 1)
FIELD(AUX_DC_CTRL, SUCCESS,            2, 1)
FIELD(AUX_DC_CTRL, ADDRESS_DEBUG_TYPE, 5, 1)
FIELD(AUX_DC_CTRL, INVALIDATE_MODE,    6, 1)
FIELD(AUX_DC_CTRL, LOCK_MODE,          7, 1)
FIELD(AUX_DC_CTRL, FLUSH_STATUS,       8, 1)
FIELD(AUX_DC_CTRL, RGN_OP,             9, 3)

/* Ignore flush status - it's always 0 */
typedef struct {
    uint32_t disable;
    uint32_t success;
    uint32_t address_debug_type;
    uint32_t invalidate_mode;
    uint32_t lock_mode;
    uint32_t region_operation;
} arc_aux_dc_ctrl_t;

static inline uint32_t arc_pack_aux_dc_ctrl(const arc_aux_dc_ctrl_t *reg)
{
    uint32_t res = 0x0;

    res = FIELD_DP32(res, AUX_DC_CTRL, DISABLE, reg->disable);
    res = FIELD_DP32(res, AUX_DC_CTRL, SUCCESS, reg->success);
    res = FIELD_DP32(res, AUX_DC_CTRL, ADDRESS_DEBUG_TYPE, reg->address_debug_type);
    res = FIELD_DP32(res, AUX_DC_CTRL, INVALIDATE_MODE, reg->invalidate_mode);
    res = FIELD_DP32(res, AUX_DC_CTRL, LOCK_MODE, reg->lock_mode);
    res = FIELD_DP32(res, AUX_DC_CTRL, RGN_OP, reg->region_operation);

    return res;
}

static inline void arc_unpack_aux_dc_ctrl(arc_aux_dc_ctrl_t *reg, uint32_t value)
{
    reg->disable = FIELD_EX32(value, AUX_DC_CTRL, DISABLE);
    reg->success = FIELD_EX32(value, AUX_DC_CTRL, SUCCESS);
    reg->address_debug_type = FIELD_EX32(value, AUX_DC_CTRL, ADDRESS_DEBUG_TYPE);
    reg->invalidate_mode = FIELD_EX32(value, AUX_DC_CTRL, INVALIDATE_MODE);
    reg->lock_mode = FIELD_EX32(value, AUX_DC_CTRL, LOCK_MODE);
    reg->region_operation = FIELD_EX32(value, AUX_DC_CTRL, RGN_OP);
}

#endif
