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

#ifndef QEMU_ARC_CPU_QOM_H
#define QEMU_ARC_CPU_QOM_H

#include "hw/core/cpu.h"

#define TYPE_ARC_CPU            "arc-cpu"

typedef struct ArchCPU ARCCPU;
typedef struct ARCCPUClass ARCCPUClass;
typedef struct CPUArchState CPUARCState;
DECLARE_OBJ_CHECKERS(ARCCPU, ARCCPUClass,
                     ARC_CPU, TYPE_ARC_CPU)

/*
 *  ARCCPUClass:
 *  @parent_realize: The parent class' realize handler.
 *  @parent_reset: The parent class' reset handler.
 *
 *  A ARC CPU model.
 */
typedef struct ARCCPUClass {
    /*< private >*/
    CPUClass parent_class;
    /*< public >*/

    DeviceRealize parent_realize;
    DeviceReset parent_reset;
} ARCCPUClass;

#endif
