/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2023 Synopsys Inc.
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

#ifndef HW_ARC_VIRT_H
#define HW_ARC_VIRT_H

#include "qom/object.h"
#include "hw/qdev-properties.h"

/* Parent virtual machine class data */
struct ARCVirtMachineClass {
    MachineClass parent;
};

/*
 * Parent virtual machine instance data
 * Access via:
 * ARCVirtMachineState *vms = ARC_VIRT_MACHINE(machine_inst);
 * Where machine_inst is a pointer to any type of child instance (i.e. Object*
 * or MachineState*)
 */
struct ARCVirtMachineState {
    MachineState parent;
};

/* Define the type name for the parent virtual machine */
#define TYPE_ARC_VIRT_MACHINE   MACHINE_TYPE_NAME("arc-virt-parent")

/*
 * Instantiate an object ARC_VIRT_MACHINE with ARCVirtMachineState as instance
 * data and ARCVirtMachineClass as class data
 */
OBJECT_DECLARE_TYPE(ARCVirtMachineState, ARCVirtMachineClass, ARC_VIRT_MACHINE)


#endif /* !HW_ARC_VIRT_H */
