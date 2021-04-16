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
#include "target/arc/mmu.h"
#include "target/arc/mmu-v6.h"
#include "target/arc/mpu.h"
#include "target/arc/cache.h"

#define ARC_CPU_TYPE_SUFFIX "-" TYPE_ARC_CPU
#define ARC_CPU_TYPE_NAME(model) model ARC_CPU_TYPE_SUFFIX
#define CPU_RESOLVING_TYPE TYPE_ARC_CPU

enum arc_features {
    ARC_FEATURE_ARC5,
    ARC_FEATURE_ARC600,
    ARC_FEATURE_ARC700,
    no_features,
};

enum arc_endianess {
    ARC_ENDIANNESS_LE = 0,
    ARC_ENDIANNESS_BE,
};

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
#ifdef TARGET_ARCV2
    EXCP_TLB_MISS_I,
    EXCP_TLB_MISS_D,
#elif defined(TARGET_ARCV3)
    EXCP_IMMU_FAULT,
    EXCP_DMMU_FAULT,
#else
    #error "TARGET macro not defined!"
#endif
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
#define Hf_b  (0)
#define AEf_b (5)
#define Uf_b  (7)
#define Lf_b  (12)
#define DZf_b (13)
#define SCf_b (14)
#define ESf_b (15)
#define ADf_b (19)
#define USf_b (20)

/* Flags with their on fields */
#define IEf_b   (31)
#define IEf_bS  (1)

#define Ef_b    (1)
#define Ef_bS   (4)

#define DEf_b   (6)
#define DEf_bS  (1)

#define Vf_b    (8)
#define Vf_bS   (1)
#define Cf_b    (9)
#define Cf_bS   (1)
#define Nf_b    (10)
#define Nf_bS   (1)
#define Zf_b    (11)
#define Zf_bS   (1)

#define RBf_b   (16)
#define RBf_bS  (3)


#define PSTATE_MASK \
     ((1 << Hf_b)  \
    | (1 << AEf_b) \
    | (1 << Uf_b)  \
    | (1 << Lf_b)  \
    | (1 << DZf_b) \
    | (1 << SCf_b) \
    | (1 << ESf_b) \
    | (1 << ADf_b) \
    | (1 << USf_b))

#define GET_STATUS_BIT(STAT, BIT) ((STAT.pstate >> BIT##_b) & 0x1)
#define SET_STATUS_BIT(STAT, BIT, VALUE) { \
    STAT.pstate &= ~(1 << BIT##_b); \
    STAT.pstate |= (VALUE << BIT##_b); \
}

typedef struct {
    target_ulong pstate;

    target_ulong RBf;
    target_ulong Ef;     /* irq priority treshold. */
    target_ulong Vf;     /*  overflow                */
    target_ulong Cf;     /*  carry                   */
    target_ulong Nf;     /*  negative                */
    target_ulong Zf;     /*  zero                    */
    target_ulong DEf;
    target_ulong IEf;

    /* Reserved bits */

    /* Next instruction is a delayslot instruction */
    bool is_delay_slot_instruction;
} ARCStatus;

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

    ARCIrq irq_bank[256]; /* IRQ register bank */
    uint32_t irq_select;     /* AUX register */
    uint32_t aux_irq_act;    /* AUX register */
    uint32_t irq_priority_pending; /* AUX register */
    uint32_t icause[16];     /* Banked cause register */
    uint32_t aux_irq_hint;   /* AUX register, used to trigger soft irq */
    uint32_t aux_user_sp;
    uint32_t aux_irq_ctrl;
    uint32_t aux_rtc_ctrl;
    uint32_t aux_rtc_low;
    uint32_t aux_rtc_high;

    /* Fields required by exception handling. */
    uint32_t causecode;
    uint32_t param;

#ifdef TARGET_ARCV2
    struct arc_mmu mmu;       /* mmu.h */
#elif defined(TARGET_ARCV3)
    struct arc_mmuv6 mmu;       /* mmu.h */
#endif
    ARCMPU mpu;               /* mpu.h */
    struct arc_cache cache;   /* cache.h */

    /* used for propagatinng "hostpc/return address" to sub-functions */
    uintptr_t host_pc;

    bool      stopped;

    /* Fields up to this point are cleared by a CPU reset */
    struct {} end_reset_fields;

    uint64_t last_clk_rtc;

    void *irq[256];
    QEMUTimer *cpu_timer[2]; /* Internal timer. */
    QEMUTimer *cpu_rtc;      /* Internal RTC. */

    const struct arc_boot_info *boot_info;

    bool enabled_interrupts;
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
#ifdef CONFIG_USER_ONLY
    assert(0); /* Not really supported at the moment. */
#else
    *pflags = cpu_mmu_index(env, 0);
#endif
}

void arc_translate_init(void);

void arc_cpu_list(void);
int cpu_arc_exec(CPUState *cpu);
int cpu_arc_signal_handler(int host_signum, void *pinfo, void *puc);
int arc_cpu_memory_rw_debug(CPUState *cs, vaddr address, uint8_t *buf,
                            int len, bool is_write);
void arc_cpu_do_interrupt(CPUState *cpu);

void arc_cpu_dump_state(CPUState *cs, FILE *f, int flags);
hwaddr arc_cpu_get_phys_page_debug(CPUState *cpu, vaddr addr);
int arc_cpu_gdb_read_register(CPUState *cpu, GByteArray *buf, int reg);
int arc_cpu_gdb_write_register(CPUState *cpu, uint8_t *buf, int reg);

void QEMU_NORETURN arc_raise_exception(CPUARCState *env, int32_t excp_idx);

void not_implemented_mmu_init(struct arc_mmu *mmu);
bool not_implemented_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                                  MMUAccessType access_type, int mmu_idx,
                                  bool probe, uintptr_t retaddr);

void arc_mmu_init(CPUARCState *env);
bool arc_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                      MMUAccessType access_type, int mmu_idx,
                      bool probe, uintptr_t retaddr);
hwaddr arc_mmu_debug_translate(CPUARCState *env, vaddr addr);
void arc_mmu_disable(CPUARCState *env);

#include "exec/cpu-all.h"

#endif /* !defined (CPU_ARC_H) */
