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

#ifndef ARC_MMU_COMMON_H
#define ARC_MMU_COMMON_H

#include "target/arc/cpu-qom.h"

#define PAGE_SHIFT      TARGET_PAGE_BITS
#define PAGE_SIZE       (1 << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE - 1))

enum mmu_version {
  MMU_VERSION_INVALID = -1,
  MMU_VERSION_3,
  MMU_VERSION_6,
  MMU_VERSION_LAST
};

enum mmu_version get_mmu_version(CPUARCState *env);

/* NOTE: Do not reorder, this is casted in tbl_fill function. */
enum mmu_access_type {
    MMU_MEM_READ = 0,
    MMU_MEM_WRITE,
    MMU_MEM_FETCH,  /* Read for execution. */
    MMU_MEM_ATTOMIC,
    MMU_MEM_IRRELEVANT_TYPE,
};

#define RWE_STRING(RWE) \
    (RWE == MMU_MEM_READ ? "MEM_READ" : \
     (RWE == MMU_MEM_WRITE ? "MEM_WRITE" : \
      (RWE == MMU_MEM_ATTOMIC ? "MEM_ATTOMIC" : \
       (RWE == MMU_MEM_FETCH ? "MEM_FETCH" : \
        (RWE == MMU_MEM_IRRELEVANT_TYPE ? "MEM_IRRELEVANT" \
         : "NOT_VALID_RWE")))))


#define CAUSE_CODE(ENUM) \
    ((ENUM == MMU_MEM_FETCH) ? 0 : \
     ((ENUM == MMU_MEM_READ) ? 1 : \
       ((ENUM == MMU_MEM_WRITE) ? 2 : 3)))


#define RAISE_MMU_EXCEPTION(ENV) { \
    do_exception_no_delayslot(ENV, \
                              ENV->mmu.exception.number, \
                              ENV->mmu.exception.causecode, \
                              ENV->mmu.exception.parameter); \
}

struct mem_exception {
  int32_t number;
  uint8_t causecode;
  uint8_t parameter;
};

#define SET_MEM_EXCEPTION(EXCP, N, C, P) { \
  (EXCP).number = N; \
  (EXCP).causecode = C; \
  (EXCP).parameter = P; \
}

struct CPUARCState;

/* ARCv2 MMU functions */
void arc_mmu_init_v3(CPUARCState *env);
bool
arc_get_physical_addr_v3(struct CPUState *cs, hwaddr *paddr, vaddr addr,
                  enum mmu_access_type rwe, bool probe,
                  uintptr_t retaddr);
bool arc_cpu_tlb_fill_v3(CPUState *cs, vaddr address, int size,
                      MMUAccessType access_type, int mmu_idx,
                      bool probe, uintptr_t retaddr);
hwaddr arc_mmu_debug_translate_v3(CPUARCState *env, vaddr addr);
void arc_mmu_disable_v3(CPUARCState *env);

/* ARCv3 MMU functions */
void arc_mmu_init_v6(CPUARCState *env);
bool
arc_get_physical_addr_v6(struct CPUState *cs, hwaddr *paddr, vaddr addr,
                  enum mmu_access_type rwe, bool probe,
                  uintptr_t retaddr);
bool arc_cpu_tlb_fill_v6(CPUState *cs, vaddr address, int size,
                      MMUAccessType access_type, int mmu_idx,
                      bool probe, uintptr_t retaddr);
hwaddr arc_mmu_debug_translate_v6(CPUARCState *env, vaddr addr);
void arc_mmu_disable_v6(CPUARCState *env);

#endif /* ARC_MMU_COMMON_H */
