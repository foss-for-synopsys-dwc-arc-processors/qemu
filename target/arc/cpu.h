/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2020 Synopsys Inc.
 * Contributed by Cupertino Miranda <cmiranda@synopsys.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2 or later, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CPU_ARC_H
#define CPU_ARC_H

#include "exec/cpu-defs.h"
#include "fpu/softfloat.h"

#include "target/arc/arc-common.h"
#include "target/arc/mmu-v6.h"
#include "target/arc/mmu.h"
#include "target/arc/mpu.h"
#include "target/arc/cache.h"
#include "target/arc/arconnect.h"

#include "hw/registerfields.h"

#define ARC_CPU_TYPE_SUFFIX "-" TYPE_ARC_CPU
#define ARC_CPU_TYPE_NAME(name) (name ARC_CPU_TYPE_SUFFIX)
#define CPU_RESOLVING_TYPE TYPE_ARC_CPU

#define TYPE_ARC_CPU_ANY               ARC_CPU_TYPE_NAME("any")
#define TYPE_ARC_CPU_ARC600            ARC_CPU_TYPE_NAME("arc600")
#define TYPE_ARC_CPU_ARC700            ARC_CPU_TYPE_NAME("arc700")
#define TYPE_ARC_CPU_ARCEM             ARC_CPU_TYPE_NAME("arcem")
#define TYPE_ARC_CPU_ARCHS             ARC_CPU_TYPE_NAME("archs")
#define TYPE_ARC_CPU_HS5X              ARC_CPU_TYPE_NAME("hs5x")
#define TYPE_ARC_CPU_HS6X              ARC_CPU_TYPE_NAME("hs6x")

/* U-Boot - kernel ABI */
#define ARC_UBOOT_CMDLINE 1
#define ARC_UBOOT_DTB     2


#define CPU_GP(env)     ((env)->r[26])
#define CPU_FP(env)     ((env)->r[27])
#define CPU_SP(env)     ((env)->r[28])
#define CPU_ILINK(env)  ((env)->r[29])
#define CPU_ILINK1(env) ((env)->r[29])
#define CPU_ILINK2(env) ((env)->r[30])
#define CPU_BLINK(env)  ((env)->r[31])
#define CPU_LP(env)     ((env)->r[60])
#define CPU_IMM(env)    ((env)->r[62])
#define CPU_PCL(env)    ((env)->r[63])

enum exception_code_list {
    EXCP_NO_EXCEPTION = -1,
    EXCP_RESET = 0,
    EXCP_MEMORY_ERROR,
    EXCP_INST_ERROR,
    EXCP_MACHINE_CHECK,
    EXCP_TLB_MISS_I,
    EXCP_TLB_MISS_D,
    EXCP_PROTV,
    EXCP_PRIVILEGEV,
    EXCP_SWI,
    EXCP_TRAP,
    EXCP_EXTENSION,
    EXCP_DIVZERO,
    EXCP_DCERROR,
    EXCP_MISALIGNED,
    EXCP_IRQ,
    EXCP_LPEND_REACHED = 9000,
    EXCP_FAKE
};

#define EXCP_IMMU_FAULT EXCP_TLB_MISS_I
#define EXCP_DMMU_FAULT EXCP_TLB_MISS_D


/*
 * Status32 register bits
 *   -- Ixxx xxxx xxxU ARRR ESDL ZNCV Udae eeeH --
 *
 *   I = IE - Interrupt Enable
 *   x =    - Reserved
 *   U = US - User sleep mode enable
 *   A = AD - Disable alignment checking
 *   R = RB - Select a register bank
 *   E = ES - EI_S table instruction pending
 *   S = SC - Enable stack checking
 *   D = DZ - RV_DivZero exception enable
 *   L =    - Zero-overhead loop disable
 *   Z =    - Zero status flag
 *   N =    - Negative status flag
 *   C =    - Cary status flag
 *   V =    - Overflow status flag
 *   U =    - User mode
 *   d = DE - Delayed branch is pending
 *   a = AE - Processor is in an exception
 *   e = E  - Interrupt priority operating level`I
 *   H =    - Halt flag
 */

/* Flags in pstate */

FIELD(STATUS32, Hf,  0, 1)
FIELD(STATUS32, Ef,  1, 4)
FIELD(STATUS32, AEf, 5, 1)
FIELD(STATUS32, DEf, 6, 1)
FIELD(STATUS32, Uf,  7, 1)
FIELD(STATUS32, Lf,  12, 1)
FIELD(STATUS32, DZf, 13, 1)
FIELD(STATUS32, SCf, 14, 1)
FIELD(STATUS32, ESf, 15, 1)
FIELD(STATUS32, RBf, 16, 3)
FIELD(STATUS32, ADf, 19, 1)
FIELD(STATUS32, USf, 20, 1)
FIELD(STATUS32, PREVIOUS_IS_DELAYSLOTf, 30, 1)
FIELD(STATUS32, IEf, 31, 1)

/* Flags with their own fields */
FIELD(STATUS32, Vf,  8,  1)
FIELD(STATUS32, Cf,  9,  1)
FIELD(STATUS32, Nf,  10, 1)
FIELD(STATUS32, Zf,  11, 1)

#define FIELD_MASK(reg, field) R_ ## reg ## _ ## field ## _MASK

#define PSTATE_MASK \
     (FIELD_MASK(STATUS32, Hf)  \
    | FIELD_MASK(STATUS32, AEf) \
    | FIELD_MASK(STATUS32, Uf)  \
    | FIELD_MASK(STATUS32, Lf)  \
    | FIELD_MASK(STATUS32, DZf) \
    | FIELD_MASK(STATUS32, SCf) \
    | FIELD_MASK(STATUS32, ESf) \
    | FIELD_MASK(STATUS32, ADf) \
    | FIELD_MASK(STATUS32, DEf) \
    | FIELD_MASK(STATUS32, IEf) \
    | FIELD_MASK(STATUS32, RBf) \
    | FIELD_MASK(STATUS32, Ef) \
    | FIELD_MASK(STATUS32, USf) \
    | FIELD_MASK(STATUS32, PREVIOUS_IS_DELAYSLOTf))

/* TODO: Replace those all over the code. */
#define GET_STATUS_BIT(STAT, FIELD) ((target_ulong) FIELD_EX32(STAT.pstate, STATUS32, FIELD))
#define SET_STATUS_BIT(STAT, FIELD, VALUE) \
     STAT.pstate = (FIELD_DP32(STAT.pstate, STATUS32, FIELD, VALUE))


#define TCG_SET_STATUS_FIELD_BIT(STAT_REG, FIELD) { \
    assert(R_STATUS32_ ## FIELD ## _LENGTH == 1); \
    tcg_gen_ori_tl(STAT_REG, STAT_REG, (R_STATUS32_ ## FIELD ## _MASK)); \
}
#define TCG_CLR_STATUS_FIELD_BIT(STAT_REG, FIELD) { \
    assert(R_STATUS32_ ## FIELD ## _LENGTH == 1); \
    tcg_gen_andi_tl(STAT_REG, STAT_REG, ~(R_STATUS32_ ## FIELD ## _MASK)); \
}
#define TCG_SET_STATUS_FIELD_VALUE(STAT_REG, FIELD, VALUE) { \
    TCGv temp = tcg_temp_new(); \
    tcg_gen_shli_tl(temp, VALUE, R_STATUS32_ ## FIELD ## _SHIFT); \
    tcg_gen_andi_tl(temp, temp, R_STATUS32_ ## FIELD ## _MASK); \
    tcg_gen_andi_tl(STAT_REG, STAT_REG, ~R_STATUS32_ ## FIELD ## _MASK); \
    tcg_gen_or_tl(STAT_REG, STAT_REG, temp); \
    tcg_temp_free(temp); \
}
#define TCG_SET_STATUS_FIELD_IVALUE(STAT_REG, FIELD, IVALUE) { \
    TCGv temp = tcg_const_tl((IVALUE << R_STATUS32_ ## FIELD ## _SHIFT) \
                             & R_STATUS32_ ## FIELD ## _MASK); \
    tcg_gen_andi_tl(STAT_REG, STAT_REG, ~R_STATUS32_ ## FIELD ## _MASK); \
    tcg_gen_or_tl(STAT_REG, STAT_REG, temp); \
    tcg_temp_free(temp); \
}
#define TCG_GET_STATUS_FIELD_MASKED(RET, STAT_REG, FIELD) { \
    tcg_gen_andi_tl(RET, STAT_REG, R_STATUS32_ ## FIELD ## _MASK); \
}

typedef struct {
    target_ulong pstate;

    target_ulong Vf;     /*  overflow                */
    target_ulong Cf;     /*  carry                   */
    target_ulong Nf;     /*  negative                */
    target_ulong Zf;     /*  zero                    */
} ARCStatus;

uint32_t pack_status32(ARCStatus *status_r);
void unpack_status32(ARCStatus *status_r, uint32_t value);

/* ARC processor timer module. */
typedef struct {
    target_ulong T_Cntrl;
    target_ulong T_Limit;
    uint64_t last_clk;
} ARCTimer;

/* ARC PIC interrupt bancked regs. */
typedef struct {
    target_ulong priority;
    target_ulong trigger;
    target_ulong pulse_cancel;
    target_ulong enable;
    target_ulong pending;
    target_ulong status;
} ARCIrq;

typedef struct CPUARCState {
    target_ulong        r[64];
    uint64_t            fpr[32];      /* assume both F and D extensions. */

    /* floating point auxiliary registers. */
    uint32_t fp_ctrl;
    uint32_t fp_status;

    ARCStatus stat, stat_l1, stat_er;

    struct {
        target_ulong    S2;
        target_ulong    S1;
        target_ulong    CS;
    } macmod;

    target_ulong intvec;

    target_ulong eret;
    target_ulong erbta;
    target_ulong ecr;
    target_ulong efa;
    target_ulong bta;
    target_ulong bta_l1;
    target_ulong bta_l2;

    target_ulong pc;     /*  program counter         */
    target_ulong lps;    /*  loops start             */
    target_ulong lpe;    /*  loops end               */

    target_ulong npc;    /* required for LP - zero overhead loops. */

    target_ulong lock_lf_var;

#define TMR_IE  (1 << 0)
#define TMR_NH  (1 << 1)
#define TMR_W   (1 << 2)
#define TMR_IP  (1 << 3)
#define TMR_PD  (1 << 4)
    ARCTimer timer[2];    /* ARC CPU-Timer 0/1 */

    /* TODO: Verify correctness of this types for both ARCv2 and v3. */
    ARCIrq irq_bank[256]; /* IRQ register bank */
    uint32_t irq_select;     /* AUX register */
    uint32_t aux_irq_act;    /* AUX register */
    uint32_t irq_priority_pending; /* AUX register */
    uint32_t icause[16];     /* Banked cause register */
    uint32_t aux_irq_hint;   /* AUX register, used to trigger soft irq */
    target_ulong aux_user_sp;
    uint32_t aux_irq_ctrl;
    uint32_t aux_rtc_ctrl;
    uint32_t aux_rtc_low;
    uint32_t aux_rtc_high;

    /* TODO: This one in particular. */
    /* Fields required by exception handling. */
    target_ulong causecode;
    target_ulong param;

    union {
      struct arc_mmu v3;
      struct arc_mmuv6 v6;
    } mmu;
    ARCMPU mpu;               /* mpu.h */
    struct arc_arcconnect_info arconnect;
    struct arc_cache cache;   /* cache.h */

    bool      stopped;

    /* Fields up to this point are cleared by a CPU reset */
    struct {} end_reset_fields;

    uint64_t last_clk_rtc;

    void *irq[256];
    QEMUTimer *cpu_timer[2]; /* Internal timer. */
    QEMUTimer *cpu_rtc;      /* Internal RTC. */

    const struct arc_boot_info *boot_info;

    bool in_delayslot_instruction;
    bool next_insn_is_delayslot;

#ifdef CONFIG_USER_ONLY
    target_ulong tls_backup;
#endif

    target_ulong readback;

    target_ulong exclusive_addr;
    target_ulong exclusive_val;
    target_ulong exclusive_val_hi;

} CPUARCState;

/*
 * ArcCPU:
 * @env: #CPUMBState
 *
 * An ARC CPU.
 */
struct ARCCPU {
    /*< private >*/
    CPUState parent_obj;

    /*< public >*/

    /* ARC Configuration Settings. */
    struct {
        uint32_t addr_size;
        uint32_t br_bc_entries;
        uint32_t br_pt_entries;
        uint32_t br_bc_tag_size;
        uint32_t external_interrupts;
        uint32_t intvbase_preset;
        uint32_t lpc_size;
        uint32_t mmu_page_size_sel0;
        uint32_t mmu_page_size_sel1;
        uint32_t mmu_pae_enabled;
        uint32_t ntlb_num_entries;
        uint32_t num_actionpoints;
        uint32_t number_of_interrupts;
        uint32_t number_of_levels;
        uint32_t pct_counters;
        uint32_t pct_interrupt;
        uint32_t pc_size;
        uint32_t rgf_num_regs;
        uint32_t rgf_banked_regs;
        uint32_t rgf_num_banks;
        uint32_t rtt_feature_level;
        uint32_t smar_stack_entries;
        uint32_t smart_implementation;
        uint32_t stlb_num_entries;
        uint32_t slc_size;
        uint32_t slc_line_size;
        uint32_t slc_ways;
        uint32_t slc_tag_banks;
        uint32_t slc_tram_delay;
        uint32_t slc_dbank_width;
        uint32_t slc_data_banks;
        uint32_t slc_dram_delay;
        uint32_t slc_ecc_option;
        uint32_t freq_hz; /* CPU frequency in hz, needed for timers. */
        uint8_t  br_rs_entries;
        uint8_t  br_tosq_entries;
        uint8_t  br_fb_entries;
        uint8_t  dccm_mem_cycles;
        uint8_t  dccm_mem_bancks;
        uint8_t  dc_mem_cycles;
        uint8_t  ecc_option;
        uint8_t  mpu_num_regions;
        uint8_t  mpy_option;
        bool     aps_feature;
        bool     byte_order;
        bool     bitscan_option;
        bool     br_bc_full_tag;
        bool     code_density;
        bool     code_protect;
        bool     dccm_posedge;
        bool     dc_posedge;
        bool     dmp_unaligned;
        bool     ecc_exception;
        bool     firq_option;
        bool     fpu_dp_option;
        bool     fpu_fma_option;
        bool     fpu_div_option;
        bool     has_actionpoints;
        bool     has_fpu;
        bool     has_interrupts;
        bool     has_mmu;
        bool     has_mpu;
        bool     has_timer_0;
        bool     has_timer_1;
        bool     has_pct;
        bool     has_rtt;
        bool     has_smart;
        bool     rtc_option;
        bool     stack_checking;
        bool     swap_option;
        bool     slc_mem_bus_width;
        bool     slc_data_halfcycle_steal;
        bool     slc_data_add_pre_pipeline;
        bool     uaux_option;
    } cfg;

    uint32_t family;

    /* Build AUX regs. */
#define TIMER0_IRQ 16
#define TIMER1_IRQ 17
#define TB_T0  (1 << 8)
#define TB_T1  (1 << 9)
#define TB_RTC (1 << 10)
#define TB_P0_MSK (0x0f0000)
#define TB_P1_MSK (0xf00000)
    uint32_t freq_hz; /* CPU frequency in hz, needed for timers. */

    uint32_t timer_build;   /* Timer configuration AUX register. */
    uint32_t irq_build;     /* Interrupt Build Configuration Register. */
    uint32_t vecbase_build; /* Interrupt Vector Base Address Configuration. */
    uint32_t mpy_build;     /* Multiply configuration register. */
    uint32_t isa_config;    /* Instruction Set Configuration Register. */

    uint8_t core_id;        /* Core id holder. */

    CPUNegativeOffsetState neg;
    CPUARCState env;
};

/* are we in user mode? */
static inline bool is_user_mode(const CPUARCState *env)
{
    return GET_STATUS_BIT(env->stat, Uf) != 0;
}

#define cpu_list            arc_cpu_list
#define cpu_signal_handler  cpu_arc_signal_handler
#define cpu_init(cpu_model) cpu_generic_init(TYPE_ARC_CPU, cpu_model)

typedef CPUARCState CPUArchState;
typedef ARCCPU ArchCPU;

#include "exec/cpu-all.h"

static inline int cpu_mmu_index(const CPUARCState *env, bool ifetch)
{
    return GET_STATUS_BIT(env->stat, Uf) != 0 ? 1 : 0;
}

static inline void cpu_get_tb_cpu_state(CPUARCState *env, target_ulong *pc,
                                        target_ulong *cs_base,
                                        uint32_t *pflags)
{
    *pc = env->pc;
    *cs_base = 0;
#ifndef CONFIG_USER_ONLY
    *pflags = cpu_mmu_index(env, 0);
#endif
}

#define IS_ARCV3(CPU) \
  ((cpu->family & ARC_OPCODE_V3_ALL) != 0)

void arc_translate_init(void);

void arc_cpu_list(void);
int cpu_arc_exec(CPUState *cpu);
int cpu_arc_signal_handler(int host_signum, void *pinfo, void *puc);
int arc_cpu_memory_rw_debug(CPUState *cs, vaddr address, uint8_t *buf,
                            int len, bool is_write);
void arc_cpu_do_interrupt(CPUState *cpu);

void arc_cpu_dump_state(CPUState *cs, FILE *f, int flags);
hwaddr arc_cpu_get_phys_page_debug(CPUState *cpu, vaddr addr);
int gdb_v2_core_read(CPUState *cpu, GByteArray *buf, int reg);
int gdb_v2_core_write(CPUState *cpu, uint8_t *buf, int reg);
int gdb_v3_core_read(CPUState *cpu, GByteArray *buf, int reg);
int gdb_v3_core_write(CPUState *cpu, uint8_t *buf, int reg);
void do_arc_semihosting(CPUARCState *env);
void arc_sim_open_console(Chardev *chr);

void QEMU_NORETURN arc_raise_exception(CPUARCState *env, uintptr_t host_pc, int32_t excp_idx);

void arc_mmu_init(CPUARCState *env);
bool arc_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                      MMUAccessType access_type, int mmu_idx,
                      bool probe, uintptr_t retaddr);
bool
arc_get_physical_addr(struct CPUState *env, hwaddr *paddr, vaddr addr,
                  enum mmu_access_type rwe, bool probe,
                  uintptr_t retaddr);
hwaddr arc_mmu_debug_translate(CPUARCState *env, vaddr addr);
void arc_mmu_disable(CPUARCState *env);

#define MMU_USER_IDX 1

#include "exec/cpu-all.h"

#endif /* !defined (CPU_ARC_H) */
