/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2021 Synppsys Inc.
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

#ifndef __ARC_ARCONNECT_H__
#define __ARC_ARCONNECT_H__

#include "cpu-qom.h"
#include "exec/cpu-defs.h"

#define ICI_IRQ 19
#define MCIP_IRQ 19
#define EXT_IRQ 24

#define MAX_IDU_CIRQS 128
#define MAX_CORES     32

#define IDU_DEST_MASK (0x0f) /* TODO: Make it general for diffenet number of smp cores. */
#define IDU_MODE_MASK (0x13) /* TODO: Make it general for diffenet number of smp cores. */

#define IDU_MODE_ENUM_MASK (0x3)
enum idu_mode_enum {
  ROUND_ROBIN = 0x0,
  FIRST_ACKNOWLEDGE = 0x1,
  ALL_DESTINATION = 0x2
};

struct lpa_lf_entry {
    QemuMutex mutex;
    target_ulong lpa_lf;
    uint64_t read_value;
};

struct arc_arcconnect_info {
    uint64_t intrpt_status;
    uint32_t wdata;

    /* IDU */
    bool idu_enabled;
    struct {
	bool mask;
	bool dest;
	bool mode;
	uint8_t counter;
	bool first_knowl_requested;
    } idu_data[MAX_IDU_CIRQS];
    uint64_t gfrc_snapshots[MAX_CORES];

    struct lpa_lf_entry *lpa_lf;
    QemuMutex *locked_mutex;
};

#define LPA_LFS_ALIGNEMENT_BITS 2 /* Inforced alignement */
#define LPA_LFS_ALIGNEMENT_MASK ((1 << LPA_LFS_ALIGNEMENT_BITS) - 1)
#define LPA_LFS_SIZE_IN_BITS    8 /* Size for concurrent lpa_entries */
#define LPA_LFS_SIZE            (1 << LPA_LFS_SIZE_IN_BITS)
#define LPA_LFS_ENTRY_FOR_PA(PA) \
    (((PA >> LPA_LFS_ALIGNEMENT_BITS)  \
      ^ (PA >> (LPA_LFS_SIZE_IN_BITS + LPA_LFS_ALIGNEMENT_BITS))) \
     & ((1 << LPA_LFS_SIZE_IN_BITS) - 1))

#define LPA_LF_GET_ENTRY_FOR_VADDR(VADDR) \
    
    
extern struct lpa_lf_entry lpa_lfs[LPA_LFS_SIZE];

void arc_arconnect_init(ARCCPU *cpu);

#endif /* __ARC_ARCONNECT_H__ */
