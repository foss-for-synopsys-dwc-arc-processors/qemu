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

#ifndef ARC_GDBSTUB_H
#define ARC_GDBSTUB_H

#ifdef TARGET_ARC32

#define GDB_TARGET_STRING "arc:ARCv2"

/* The order here is strictly tied with gdb-xml/arc-v2-core.xml. */
enum gdb_regs {
    GDB_REG_0 = 0,
    GDB_REG_1,
    GDB_REG_2,
    GDB_REG_3,
    GDB_REG_4,
    GDB_REG_5,
    GDB_REG_6,
    GDB_REG_7,
    GDB_REG_8,
    GDB_REG_9,
    GDB_REG_10,
    GDB_REG_11,
    GDB_REG_12,
    GDB_REG_13,
    GDB_REG_14,
    GDB_REG_15,
    GDB_REG_16,
    GDB_REG_17,
    GDB_REG_18,
    GDB_REG_19,
    GDB_REG_20,
    GDB_REG_21,
    GDB_REG_22,
    GDB_REG_23,
    GDB_REG_24,
    GDB_REG_25,
    GDB_REG_26,         /* GP                         */
    GDB_REG_27,         /* FP                         */
    GDB_REG_28,         /* SP                         */
    GDB_REG_29,         /* ILINK                      */
    GDB_REG_30,         /* R30                        */
    GDB_REG_31,         /* BLINK                      */
    GDB_REG_58,         /* little_endian? ACCL : ACCH */
    GDB_REG_59,         /* little_endian? ACCH : ACCL */
    GDB_REG_60,         /* LP                         */
    GDB_REG_63,         /* PCL                        */

    GDB_REG_LAST
};

/* The order here is strictly tied with gdb-xml/arc-v2-aux.xml. */
enum gdb_aux_regs {
    GDB_AUX_REG_PC = 0,        /* program counter */
    GDB_AUX_REG_LPS,           /* loop body start */
    GDB_AUX_REG_LPE,           /* loop body end   */
    GDB_AUX_REG_STATUS,        /* status flag     */
    GDB_AUX_REG_BTA,           /* branch target address */
    /* builds */
    GDB_AUX_REG_TIMER_BUILD,   /* timer build                */
    GDB_AUX_REG_IRQ_BUILD,     /* irq build                  */
    GDB_AUX_REG_MPY_BUILD,     /* multiply configuration     */
    GDB_AUX_REG_VECBASE_BUILD, /* vector base address config */
    GDB_AUX_REG_ISA_CONFIG,    /* isa config                 */
    GDB_AUX_REG_MPU_BUILD,     /* MPU build           */
    /* timers */
    GDB_AUX_REG_TIMER_CNT0,    /* timer 0 counter */
    GDB_AUX_REG_TIMER_CTRL0,   /* timer 0 control */
    GDB_AUX_REG_TIMER_LIM0,    /* timer 0 limit   */
    GDB_AUX_REG_TIMER_CNT1,    /* timer 1 counter */
    GDB_AUX_REG_TIMER_CTRL1,   /* timer 1 control */
    GDB_AUX_REG_TIMER_LIM1,    /* timer 1 limit   */
    /* mmuv4 */
    GDB_AUX_REG_PID,           /* process identity  */
    GDB_AUX_REG_TLBPD0,        /* page descriptor 0 */
    GDB_AUX_REG_TLBPD1,        /* page descriptor 1 */
    GDB_AUX_REG_TLB_INDEX,     /* tlb index         */
    GDB_AUX_REG_TLB_CMD,       /* tlb command       */
    /* mpu */
    GDB_AUX_REG_MPU_EN,        /* MPU enable          */
    GDB_AUX_REG_MPU_ECR,       /* MPU exception cause */
    GDB_AUX_REG_MPU_BASE0,     /* MPU base 0          */
    GDB_AUX_REG_MPU_BASE1,     /* MPU base 1          */
    GDB_AUX_REG_MPU_BASE2,     /* MPU base 2          */
    GDB_AUX_REG_MPU_BASE3,     /* MPU base 3          */
    GDB_AUX_REG_MPU_BASE4,     /* MPU base 4          */
    GDB_AUX_REG_MPU_BASE5,     /* MPU base 5          */
    GDB_AUX_REG_MPU_BASE6,     /* MPU base 6          */
    GDB_AUX_REG_MPU_BASE7,     /* MPU base 7          */
    GDB_AUX_REG_MPU_BASE8,     /* MPU base 8          */
    GDB_AUX_REG_MPU_BASE9,     /* MPU base 9          */
    GDB_AUX_REG_MPU_BASE10,    /* MPU base 10         */
    GDB_AUX_REG_MPU_BASE11,    /* MPU base 11         */
    GDB_AUX_REG_MPU_BASE12,    /* MPU base 12         */
    GDB_AUX_REG_MPU_BASE13,    /* MPU base 13         */
    GDB_AUX_REG_MPU_BASE14,    /* MPU base 14         */
    GDB_AUX_REG_MPU_BASE15,    /* MPU base 15         */
    GDB_AUX_REG_MPU_PERM0,     /* MPU permission 0    */
    GDB_AUX_REG_MPU_PERM1,     /* MPU permission 1    */
    GDB_AUX_REG_MPU_PERM2,     /* MPU permission 2    */
    GDB_AUX_REG_MPU_PERM3,     /* MPU permission 3    */
    GDB_AUX_REG_MPU_PERM4,     /* MPU permission 4    */
    GDB_AUX_REG_MPU_PERM5,     /* MPU permission 5    */
    GDB_AUX_REG_MPU_PERM6,     /* MPU permission 6    */
    GDB_AUX_REG_MPU_PERM7,     /* MPU permission 7    */
    GDB_AUX_REG_MPU_PERM8,     /* MPU permission 8    */
    GDB_AUX_REG_MPU_PERM9,     /* MPU permission 9    */
    GDB_AUX_REG_MPU_PERM10,    /* MPU permission 10   */
    GDB_AUX_REG_MPU_PERM11,    /* MPU permission 11   */
    GDB_AUX_REG_MPU_PERM12,    /* MPU permission 12   */
    GDB_AUX_REG_MPU_PERM13,    /* MPU permission 13   */
    GDB_AUX_REG_MPU_PERM14,    /* MPU permission 14   */
    GDB_AUX_REG_MPU_PERM15,    /* MPU permission 15   */
    /* excpetions */
    GDB_AUX_REG_ERSTATUS,      /* exception return status  */
    GDB_AUX_REG_ERBTA,         /* exception return BTA     */
    GDB_AUX_REG_ECR,           /* exception cause register */
    GDB_AUX_REG_ERET,          /* exception return address */
    GDB_AUX_REG_EFA,           /* exception fault address  */
    /* irq */
    GDB_AUX_REG_ICAUSE,        /* interrupt cause        */
    GDB_AUX_REG_IRQ_CTRL,      /* context saving control */
    GDB_AUX_REG_IRQ_ACT,       /* active                 */
    GDB_AUX_REG_IRQ_PRIO_PEND, /* priority pending       */
    GDB_AUX_REG_IRQ_HINT,      /* hint                   */
    GDB_AUX_REG_IRQ_SELECT,    /* select                 */
    GDB_AUX_REG_IRQ_ENABLE,    /* enable                 */
    GDB_AUX_REG_IRQ_TRIGGER,   /* trigger                */
    GDB_AUX_REG_IRQ_STATUS,    /* status                 */
    GDB_AUX_REG_IRQ_PULSE,     /* pulse cancel           */
    GDB_AUX_REG_IRQ_PENDING,   /* pending                */
    GDB_AUX_REG_IRQ_PRIO,      /* priority               */

    GDB_AUX_REG_LAST
};

/* ARCv3_64 */
#else

//#define GDB_TARGET_STRING "arc:ARCv3_64"
#define GDB_TARGET_STRING "arc64:64"

/* The order here is strictly tied with gdb-xml/arc-v3_{32,64}-core.xml. */
enum gdb_regs {
    GDB_REG_0 = 0,
    GDB_REG_1,
    GDB_REG_2,
    GDB_REG_3,
    GDB_REG_4,
    GDB_REG_5,
    GDB_REG_6,
    GDB_REG_7,
    GDB_REG_8,
    GDB_REG_9,
    GDB_REG_10,
    GDB_REG_11,
    GDB_REG_12,
    GDB_REG_13,
    GDB_REG_14,
    GDB_REG_15,
    GDB_REG_16,
    GDB_REG_17,
    GDB_REG_18,
    GDB_REG_19,
    GDB_REG_20,
    GDB_REG_21,
    GDB_REG_22,
    GDB_REG_23,
    GDB_REG_24,
    GDB_REG_25,
    GDB_REG_26,
    GDB_REG_27,         /* FP          */
    GDB_REG_28,         /* SP          */
    GDB_REG_29,         /* ILINK       */
    GDB_REG_30,         /* GP          */
    GDB_REG_31,         /* BLINK       */
    GDB_REG_58,         /* ACC0        */
    GDB_REG_63,         /* PCL         */

    GDB_REG_LAST
};

/* The order here is strictly tied with gdb-xml/arc-v3_{32,64}-aux.xml. */
enum gdb_aux_regs {
    GDB_AUX_REG_PC = 0,        /* program counter */
    GDB_AUX_REG_STATUS,        /* status flag     */
    GDB_AUX_REG_BTA,           /* branch target address */
    /* builds */
    GDB_AUX_REG_TIMER_BUILD,   /* timer build                */
    GDB_AUX_REG_IRQ_BUILD,     /* irq build                  */
    GDB_AUX_REG_VECBASE_BUILD, /* vector base address config */
    GDB_AUX_REG_ISA_CONFIG,    /* isa config                 */
    /* timers */
    GDB_AUX_REG_TIMER_CNT0,    /* timer 0 counter */
    GDB_AUX_REG_TIMER_CTRL0,   /* timer 0 control */
    GDB_AUX_REG_TIMER_LIM0,    /* timer 0 limit   */
    GDB_AUX_REG_TIMER_CNT1,    /* timer 1 counter */
    GDB_AUX_REG_TIMER_CTRL1,   /* timer 1 control */
    GDB_AUX_REG_TIMER_LIM1,    /* timer 1 limit   */
    /* excpetions */
    GDB_AUX_REG_ERSTATUS,      /* exception return status  */
    GDB_AUX_REG_ERBTA,         /* exception return BTA     */
    GDB_AUX_REG_ECR,           /* exception cause register */
    GDB_AUX_REG_ERET,          /* exception return address */
    GDB_AUX_REG_EFA,           /* exception fault address  */
    /* irq */
    GDB_AUX_REG_ICAUSE,        /* interrupt cause        */
    GDB_AUX_REG_IRQ_CTRL,      /* context saving control */
    GDB_AUX_REG_IRQ_ACT,       /* active                 */
    GDB_AUX_REG_IRQ_PRIO_PEND, /* priority pending       */
    GDB_AUX_REG_IRQ_HINT,      /* hint                   */
    GDB_AUX_REG_IRQ_SELECT,    /* select                 */
    GDB_AUX_REG_IRQ_ENABLE,    /* enable                 */
    GDB_AUX_REG_IRQ_TRIGGER,   /* trigger                */
    GDB_AUX_REG_IRQ_STATUS,    /* status                 */
    GDB_AUX_REG_IRQ_PULSE,     /* pulse cancel           */
    GDB_AUX_REG_IRQ_PRIO,      /* priority               */
    /* mmuv6 */
    GDB_AUX_REG_MMU_CTRL,      /* mmuv6 control */
    GDB_AUX_REG_RTP0,          /* region 0 ptr  */
    GDB_AUX_REG_RTP1,          /* region 1 ptr  */

    GDB_AUX_REG_LAST
};

/* The order here is strictly tied with gdb-xml/arc-v3_64-fpu.xml. */
enum gdb_fpu_regs {
    GDB_FPU_REG_F0 = 0,
    GDB_FPU_REG_F1,
    GDB_FPU_REG_F2,
    GDB_FPU_REG_F3,
    GDB_FPU_REG_F4,
    GDB_FPU_REG_F5,
    GDB_FPU_REG_F6,
    GDB_FPU_REG_F7,
    GDB_FPU_REG_F8,
    GDB_FPU_REG_F9,
    GDB_FPU_REG_F10,
    GDB_FPU_REG_F11,
    GDB_FPU_REG_F12,
    GDB_FPU_REG_F13,
    GDB_FPU_REG_F14,
    GDB_FPU_REG_F15,
    GDB_FPU_REG_F16,
    GDB_FPU_REG_F17,
    GDB_FPU_REG_F18,
    GDB_FPU_REG_F19,
    GDB_FPU_REG_F20,
    GDB_FPU_REG_F21,
    GDB_FPU_REG_F22,
    GDB_FPU_REG_F23,
    GDB_FPU_REG_F24,
    GDB_FPU_REG_F25,
    GDB_FPU_REG_F26,
    GDB_FPU_REG_F27,
    GDB_FPU_REG_F28,
    GDB_FPU_REG_F29,
    GDB_FPU_REG_F30,
    GDB_FPU_REG_F31,
    GDB_FPU_REG_BUILD,
    GDB_FPU_REG_CTRL,
    GDB_FPU_REG_STATUS,

    GDB_FPU_REG_LAST
};

#endif /* ARCv2 or ARCv3_64 */

/* add auxiliary registers to set of supported registers for GDB */
void arc_cpu_register_gdb_regs_for_features(ARCCPU *cpu);

#endif /* ARC_GDBSTUB_H */
