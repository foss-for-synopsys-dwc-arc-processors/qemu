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

struct lpa_lf_entry {
    QemuMutex mutex;
    target_ulong lpa_lf;
};

struct arc_arcconnect_info {
    uint64_t intrpt_status;

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

void arc_arconnect_init(struct ARCCPU *cpu);

#endif /* __ARC_ARCONNECT_H__ */
