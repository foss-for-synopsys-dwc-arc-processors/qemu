/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2022 Synopsys Inc.
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
 * href="http://www.gnu.org/licenses/lgpl-2.1.html
 */

#ifndef ARC_FPU_H
#define ARC_FPU_H

#include "qemu/osdep.h"

enum shuffle_type_enum {
  HEXCH = 0,
  SEXCH,
  DEXCH,
  HUNPKL,
  HUNPKM,
  SUNPKL,
  SUNPKM,
  DUNPKL,
  DUNPKM,
  HPACKL,
  HPACKM,
  SPACKL,
  SPACKM,
  DPACKL,
  DPACKM,
  HBFLYL,
  HBFLYM,
  SBFLYL,
  SBFLYM,
  DBFLYL,
  DBFLYM,
  SHUFFLE_TYPE_SIZE
};

extern uint8_t vfp_width;

void arc_fpu_status_set_interval(CPUARCState *env, target_ulong val);
target_ulong arc_fpu_status_get_internal(CPUARCState *env);
target_ulong arc_fpu_ctrl_get_internal(CPUARCState *env);
void arc_fpu_ctrl_set_internal(CPUARCState *env, target_ulong val);

void init_fpu(bool fp_dp, bool fp_wide, bool fp_hp);

#endif
