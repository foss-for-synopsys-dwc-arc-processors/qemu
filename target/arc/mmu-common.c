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

#include "qemu/osdep.h"
#include "target/arc/regs.h"
//#include "qemu/osdep.h"
#include "cpu.h"
#include "exec/exec-all.h"
#include "mmu-common.h"


/* MMU Callback funtions struture */
struct mmu_callbacks_struct {
  void (*arc_mmu_init_cb)(CPUARCState *env);
  bool (*arc_get_physical_addr_cb)(struct CPUState *cs, hwaddr *paddr, vaddr addr,
				enum mmu_access_type rwe, bool probe,
				uintptr_t retaddr);
  bool (*arc_cpu_tlb_fill_cb)(CPUState *cs, vaddr address, int size,
			      MMUAccessType access_type, int mmu_idx,
			      bool probe, uintptr_t retaddr);
  hwaddr (*arc_mmu_debug_translate_cb)(CPUARCState *env, vaddr addr);
  void (*arc_mmu_disable_cb)(CPUARCState *env);
};

enum mmu_version {
  MMU_VERSION_INVALID = -1,
  MMU_VERSION_3,
  MMU_VERSION_6,
  MMU_VERSION_LAST
};

#ifdef CONFIG_USER_ONLY
#define arc_mmu_debug_translate_v6 NULL
#define arc_mmu_debug_translate_v3 NULL
#endif

const struct mmu_callbacks_struct mmu_callbacks[MMU_VERSION_LAST] = {
  [MMU_VERSION_3] = {
    .arc_mmu_init_cb = arc_mmu_init_v3,
    .arc_get_physical_addr_cb = arc_get_physical_addr_v3,
    .arc_cpu_tlb_fill_cb = arc_cpu_tlb_fill_v3,
    .arc_mmu_debug_translate_cb = arc_mmu_debug_translate_v3,
    .arc_mmu_disable_cb = arc_mmu_disable_v3
  },
  [MMU_VERSION_6] = {
    .arc_mmu_init_cb = arc_mmu_init_v6,
    .arc_get_physical_addr_cb = arc_get_physical_addr_v6,
    .arc_cpu_tlb_fill_cb = arc_cpu_tlb_fill_v6,
    .arc_mmu_debug_translate_cb = arc_mmu_debug_translate_v6,
    .arc_mmu_disable_cb = arc_mmu_disable_v6
  }
};

static enum mmu_version get_mmu_version(void) {
#if defined(TARGET_ARC32)
  // TODO: Make this correct for arc32.
  return MMU_VERSION_3;
#elif defined(TARGET_ARC64)
  return MMU_VERSION_6;
#else
#error "NOT POSSIBLE!!!!"
#endif
}

#define MMU_CALLBACK(NAME, ...) \
  mmu_callbacks[get_mmu_version()].NAME##_cb(__VA_ARGS__);

/* TODO: Fill in for v3 mmu as well */

void arc_mmu_init(CPUARCState *env) {
  MMU_CALLBACK(arc_mmu_init, env);
}

bool
arc_get_physical_addr(struct CPUState *cs, hwaddr *paddr, vaddr addr,
                  enum mmu_access_type rwe, bool probe,
                  uintptr_t retaddr)
{
    return MMU_CALLBACK(arc_get_physical_addr, cs, paddr, addr, rwe, probe, retaddr);
}

bool arc_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                      MMUAccessType access_type, int mmu_idx,
                      bool probe, uintptr_t retaddr)
{
  return MMU_CALLBACK(arc_cpu_tlb_fill, cs, address, size, access_type, mmu_idx, probe, retaddr);
}

#ifndef CONFIG_USER_ONLY
hwaddr arc_mmu_debug_translate(CPUARCState *env, vaddr addr)
{
  return MMU_CALLBACK(arc_mmu_debug_translate, env, addr);
}
#endif

void arc_mmu_disable(CPUARCState *env)
{
  MMU_CALLBACK(arc_mmu_disable, env);
}
