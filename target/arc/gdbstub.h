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

#define GDB_ARCV2_CORE_XML    "arc-v2-core.xml"
#define GDB_ARCV3_32_CORE_XML "arc-v3_32-core.xml"
#define GDB_ARCV3_64_CORE_XML "arc-v3_64-core.xml"

#define GDB_ARCV2_AUX_XML     "arc-v2-aux.xml"
#define GDB_ARCV3_32_AUX_XML  "arc-v3_32-aux.xml"
#define GDB_ARCV3_64_AUX_XML  "arc-v3_64-aux.xml"
#define GDB_ARCV3_64_FPU_XML  "arc-v3_64-fpu.xml"

#define GDB_ARCV2_ARCH        "arc:ARCv2"
#define GDB_ARCV3_32_ARCH     "arc64:32"
#define GDB_ARCV3_64_ARCH     "arc64:64"

/* The order here is strictly tied with gdb-xml/arc-v2-core.xml. */
enum gdb_v2_core_regs {
    V2_CORE_R0 = 0,
    V2_CORE_R1,
    V2_CORE_R2,
    V2_CORE_R3,
    V2_CORE_R4,
    V2_CORE_R5,
    V2_CORE_R6,
    V2_CORE_R7,
    V2_CORE_R8,
    V2_CORE_R9,
    V2_CORE_R10,
    V2_CORE_R11,
    V2_CORE_R12,
    V2_CORE_R13,
    V2_CORE_R14,
    V2_CORE_R15,
    V2_CORE_R16,
    V2_CORE_R17,
    V2_CORE_R18,
    V2_CORE_R19,
    V2_CORE_R20,
    V2_CORE_R21,
    V2_CORE_R22,
    V2_CORE_R23,
    V2_CORE_R24,
    V2_CORE_R25,
    V2_CORE_R26,         /* GP                         */
    V2_CORE_R27,         /* FP                         */
    V2_CORE_R28,         /* SP                         */
    V2_CORE_R29,         /* ILINK                      */
    V2_CORE_R30,         /* R30                        */
    V2_CORE_R31,         /* BLINK                      */
    V2_CORE_R58,         /* little_endian? ACCL : ACCH */
    V2_CORE_R59,         /* little_endian? ACCH : ACCL */
    V2_CORE_R60,         /* LP                         */
    V2_CORE_R63,         /* PCL                        */

    GDB_ARCV2_CORE_LAST
};

/* The order here is strictly tied with gdb-xml/arc-v2-aux.xml. */
enum gdb_v2_aux_regs {
    V2_AUX_PC = 0,        /* program counter */
    V2_AUX_LPS,           /* loop body start */
    V2_AUX_LPE,           /* loop body end   */
    V2_AUX_STATUS,        /* status flag     */
    V2_AUX_BTA,           /* branch target address */
    /* builds */
    V2_AUX_TIMER_BUILD,   /* timer build                */
    V2_AUX_IRQ_BUILD,     /* irq build                  */
    V2_AUX_MPY_BUILD,     /* multiply configuration     */
    V2_AUX_VECBASE_BUILD, /* vector base address config */
    V2_AUX_ISA_CONFIG,    /* isa config                 */
    V2_AUX_MPU_BUILD,     /* MPU build           */
    /* timers */
    V2_AUX_TIMER_CNT0,    /* timer 0 counter */
    V2_AUX_TIMER_CTRL0,   /* timer 0 control */
    V2_AUX_TIMER_LIM0,    /* timer 0 limit   */
    V2_AUX_TIMER_CNT1,    /* timer 1 counter */
    V2_AUX_TIMER_CTRL1,   /* timer 1 control */
    V2_AUX_TIMER_LIM1,    /* timer 1 limit   */
    /* mmuv4 */
    V2_AUX_PID,           /* process identity  */
    V2_AUX_TLBPD0,        /* page descriptor 0 */
    V2_AUX_TLBPD1,        /* page descriptor 1 */
    V2_AUX_TLB_INDEX,     /* tlb index         */
    V2_AUX_TLB_CMD,       /* tlb command       */
    /* mpu */
    V2_AUX_MPU_EN,        /* MPU enable          */
    V2_AUX_MPU_ECR,       /* MPU exception cause */
    V2_AUX_MPU_BASE0,     /* MPU base 0          */
    V2_AUX_MPU_BASE1,     /* MPU base 1          */
    V2_AUX_MPU_BASE2,     /* MPU base 2          */
    V2_AUX_MPU_BASE3,     /* MPU base 3          */
    V2_AUX_MPU_BASE4,     /* MPU base 4          */
    V2_AUX_MPU_BASE5,     /* MPU base 5          */
    V2_AUX_MPU_BASE6,     /* MPU base 6          */
    V2_AUX_MPU_BASE7,     /* MPU base 7          */
    V2_AUX_MPU_BASE8,     /* MPU base 8          */
    V2_AUX_MPU_BASE9,     /* MPU base 9          */
    V2_AUX_MPU_BASE10,    /* MPU base 10         */
    V2_AUX_MPU_BASE11,    /* MPU base 11         */
    V2_AUX_MPU_BASE12,    /* MPU base 12         */
    V2_AUX_MPU_BASE13,    /* MPU base 13         */
    V2_AUX_MPU_BASE14,    /* MPU base 14         */
    V2_AUX_MPU_BASE15,    /* MPU base 15         */
    V2_AUX_MPU_PERM0,     /* MPU permission 0    */
    V2_AUX_MPU_PERM1,     /* MPU permission 1    */
    V2_AUX_MPU_PERM2,     /* MPU permission 2    */
    V2_AUX_MPU_PERM3,     /* MPU permission 3    */
    V2_AUX_MPU_PERM4,     /* MPU permission 4    */
    V2_AUX_MPU_PERM5,     /* MPU permission 5    */
    V2_AUX_MPU_PERM6,     /* MPU permission 6    */
    V2_AUX_MPU_PERM7,     /* MPU permission 7    */
    V2_AUX_MPU_PERM8,     /* MPU permission 8    */
    V2_AUX_MPU_PERM9,     /* MPU permission 9    */
    V2_AUX_MPU_PERM10,    /* MPU permission 10   */
    V2_AUX_MPU_PERM11,    /* MPU permission 11   */
    V2_AUX_MPU_PERM12,    /* MPU permission 12   */
    V2_AUX_MPU_PERM13,    /* MPU permission 13   */
    V2_AUX_MPU_PERM14,    /* MPU permission 14   */
    V2_AUX_MPU_PERM15,    /* MPU permission 15   */
    /* excpetions */
    V2_AUX_ERSTATUS,      /* exception return status  */
    V2_AUX_ERBTA,         /* exception return BTA     */
    V2_AUX_ECR,           /* exception cause register */
    V2_AUX_ERET,          /* exception return address */
    V2_AUX_EFA,           /* exception fault address  */
    /* irq */
    V2_AUX_ICAUSE,        /* interrupt cause        */
    V2_AUX_IRQ_CTRL,      /* context saving control */
    V2_AUX_IRQ_ACT,       /* active                 */
    V2_AUX_IRQ_PRIO_PEND, /* priority pending       */
    V2_AUX_IRQ_HINT,      /* hint                   */
    V2_AUX_IRQ_SELECT,    /* select                 */
    V2_AUX_IRQ_ENABLE,    /* enable                 */
    V2_AUX_IRQ_TRIGGER,   /* trigger                */
    V2_AUX_IRQ_STATUS,    /* status                 */
    V2_AUX_IRQ_PULSE,     /* pulse cancel           */
    V2_AUX_IRQ_PENDING,   /* pending                */
    V2_AUX_IRQ_PRIO,      /* priority               */

    GDB_ARCV2_AUX_LAST
};

/* The order here is strictly tied with gdb-xml/arc-v3_{32,64}-core.xml. */
enum gdb_v3_core_regs {
    V3_CORE_R0 = 0,
    V3_CORE_R1,
    V3_CORE_R2,
    V3_CORE_R3,
    V3_CORE_R4,
    V3_CORE_R5,
    V3_CORE_R6,
    V3_CORE_R7,
    V3_CORE_R8,
    V3_CORE_R9,
    V3_CORE_R10,
    V3_CORE_R11,
    V3_CORE_R12,
    V3_CORE_R13,
    V3_CORE_R14,
    V3_CORE_R15,
    V3_CORE_R16,
    V3_CORE_R17,
    V3_CORE_R18,
    V3_CORE_R19,
    V3_CORE_R20,
    V3_CORE_R21,
    V3_CORE_R22,
    V3_CORE_R23,
    V3_CORE_R24,
    V3_CORE_R25,
    V3_CORE_R26,
    V3_CORE_R27,         /* FP          */
    V3_CORE_R28,         /* SP          */
    V3_CORE_R29,         /* ILINK       */
    V3_CORE_R30,         /* GP          */
    V3_CORE_R31,         /* BLINK       */
    V3_CORE_R58,         /* ACC0        */
    V3_CORE_R63,         /* PCL         */

    GDB_ARCV3_CORE_LAST
};

/* The order here is strictly tied with gdb-xml/arc-v3_{32,64}-aux.xml. */
enum gdb_v3_aux_regs {
    V3_AUX_PC = 0,        /* program counter */
    V3_AUX_STATUS,        /* status flag     */
    V3_AUX_BTA,           /* branch target address */
    /* builds */
    V3_AUX_TIMER_BUILD,   /* timer build                */
    V3_AUX_IRQ_BUILD,     /* irq build                  */
    V3_AUX_VECBASE_BUILD, /* vector base address config */
    V3_AUX_ISA_CONFIG,    /* isa config                 */
    /* timers */
    V3_AUX_TIMER_CNT0,    /* timer 0 counter */
    V3_AUX_TIMER_CTRL0,   /* timer 0 control */
    V3_AUX_TIMER_LIM0,    /* timer 0 limit   */
    V3_AUX_TIMER_CNT1,    /* timer 1 counter */
    V3_AUX_TIMER_CTRL1,   /* timer 1 control */
    V3_AUX_TIMER_LIM1,    /* timer 1 limit   */
    /* excpetions */
    V3_AUX_ERSTATUS,      /* exception return status  */
    V3_AUX_ERBTA,         /* exception return BTA     */
    V3_AUX_ECR,           /* exception cause register */
    V3_AUX_ERET,          /* exception return address */
    V3_AUX_EFA,           /* exception fault address  */
    /* irq */
    V3_AUX_ICAUSE,        /* interrupt cause        */
    V3_AUX_IRQ_CTRL,      /* context saving control */
    V3_AUX_IRQ_ACT,       /* active                 */
    V3_AUX_IRQ_PRIO_PEND, /* priority pending       */
    V3_AUX_IRQ_HINT,      /* hint                   */
    V3_AUX_IRQ_SELECT,    /* select                 */
    V3_AUX_IRQ_ENABLE,    /* enable                 */
    V3_AUX_IRQ_TRIGGER,   /* trigger                */
    V3_AUX_IRQ_STATUS,    /* status                 */
    V3_AUX_IRQ_PULSE,     /* pulse cancel           */
    V3_AUX_IRQ_PRIO,      /* priority               */
    /* mmuv6 */
    V3_AUX_MMU_CTRL,      /* mmuv6 control */
    V3_AUX_RTP0,          /* region 0 ptr  */
    V3_AUX_RTP1,          /* region 1 ptr  */

    GDB_ARCV3_AUX_LAST
};

/* The order here is strictly tied with gdb-xml/arc-v3_64-fpu.xml. */
enum gdb_v3_fpu_regs {
    V3_FPU_0 = 0,
    V3_FPU_1,
    V3_FPU_2,
    V3_FPU_3,
    V3_FPU_4,
    V3_FPU_5,
    V3_FPU_6,
    V3_FPU_7,
    V3_FPU_8,
    V3_FPU_9,
    V3_FPU_10,
    V3_FPU_11,
    V3_FPU_12,
    V3_FPU_13,
    V3_FPU_14,
    V3_FPU_15,
    V3_FPU_16,
    V3_FPU_17,
    V3_FPU_18,
    V3_FPU_19,
    V3_FPU_20,
    V3_FPU_21,
    V3_FPU_22,
    V3_FPU_23,
    V3_FPU_24,
    V3_FPU_25,
    V3_FPU_26,
    V3_FPU_27,
    V3_FPU_28,
    V3_FPU_29,
    V3_FPU_30,
    V3_FPU_31,
    V3_FPU_BUILD,
    V3_FPU_CTRL,
    V3_FPU_STATUS,

    GDB_ARCV3_FPU_LAST
};

/* add auxiliary registers to set of supported registers for GDB */
void arc_cpu_register_gdb_regs_for_features(ARCCPU *cpu);

#endif /* ARC_GDBSTUB_H */
