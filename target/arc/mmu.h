/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2020 Synppsys Inc.
 * Contributed by Cupertino Miranda <cmiranda@synopsys.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * http://www.gnu.org/licenses/lgpl-2.1.html
 */

#ifndef ARC_MMU_H
#define ARC_MMU_H

#include "target/arc/mmu-common.h"

extern unsigned char mmu_v3_page_size;
#define MMU_V3_PAGE_BITS (mmu_v3_page_size)
#define MMU_V3_PAGE_MASK ((target_ulong) (((target_long) -1) << MMU_V3_PAGE_BITS))

/* PD0 flags */
#define PD0_VPN 0x7ffff000
#define PD0_ASID 0x000000ff
#define PD0_G   0x00000100      /* Global */
#define PD0_V   0x00000200      /* Valid */
#define PD0_SZ  0x00000400      /* Size: Normal or Super Page */
#define PD0_L   0x00000800      /* Lock */
#define PD0_S   0x80000000      /* Shared Library ASID */
#define PD0_FLG (PD0_G | PD0_V | PD0_SZ | PD0_L)

#define PD0_ASID_MATCH 0x0000003f
#define PD0_PID_MATCH  0x000000ff

/* PD1 permission bits */
#define PD1_PPN 0xfffff000      /* Cached */
#define PD1_FC  0x00000001      /* Cached */
#define PD1_XU  0x00000002      /* User Execute */
#define PD1_WU  0x00000004      /* User Write */
#define PD1_RU  0x00000008      /* User Read */
#define PD1_XK  0x00000010      /* Kernel Execute */
#define PD1_WK  0x00000020      /* Kernel Write */
#define PD1_RK  0x00000040      /* Kernel Read */
#define PD1_FLG (PD1_FC | PD1_XU | PD1_WU | PD1_RU | PD1_XK | PD1_WK | PD1_RK)

#define TLBINDEX_INDEX  0x00001fff
#define TLBINDEX_E      0x80000000
#define TLBINDEX_RC        0x70000000

#define TLB_CMD_WRITE   0x1
#define TLB_CMD_WRITENI 0x5
#define TLB_CMD_READ    0x2
#define TLB_CMD_INSERT  0x7
#define TLB_CMD_DELETE  0x8
#define TLB_CMD_IVUTLB  0x6

#define N_SETS          256
#define N_WAYS          4
#define TLB_ENTRIES     (N_SETS * N_WAYS)



struct arc_tlb_e {
    /*
     * TLB entry is {PD0,PD1} tuple, kept "unpacked" to avoid bit fiddling
     * flags includes both PD0 flags and PD1 permissions.
     */
    uint32_t pd0, pd1;
};

struct arc_mmu {
    uint32_t enabled;

    struct arc_tlb_e nTLB[N_SETS][N_WAYS];

    /* insert uses vaddr to find set; way selection could be random/rr/lru */
    uint32_t way_sel[N_SETS];

    /*
     * Current Address Space ID (in whose context mmu lookups done)
     * Note that it is actually present in AUX PID reg, which we don't
     * explicitly maintain, but {re,de}construct as needed by LR/SR insns
     * respectively.
     */
    uint32_t pid_asid;
    uint32_t sasid0;
    uint32_t sasid1;

    uint32_t tlbpd0;
    uint32_t tlbpd1;
    uint32_t tlbpd1_hi;
    uint32_t tlbindex;
    uint32_t tlbcmd;
    uint32_t scratch_data0;
};


void arc_mmu_debug_tlb(CPUARCState *env);
void arc_mmu_debug_tlb_for_vaddr(CPUARCState *env, uint32_t vaddr);


#endif /* ARC_MMU_H */
