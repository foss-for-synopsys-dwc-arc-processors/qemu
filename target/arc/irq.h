/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2020 Synopsys Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License) any later version.
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

#ifndef __IRQ_H__
#define __IRQ_H__

#include "target/arc/regs.h"
#include "cpu.h"

bool arc_cpu_exec_interrupt(CPUState *, int);
bool arc_rtie_interrupts(CPUARCState *);
void switchSP(CPUARCState *);
void arc_initializeIRQ(ARCCPU *);
void arc_resetIRQ(ARCCPU *);
uint32_t pack_status32(ARCStatus *);
void unpack_status32(ARCStatus *, uint32_t);

#ifdef TARGET_ARCV2
#define OFFSET_FOR_VECTOR(VECNO) (VECNO << 2)
#elif defined(TARGET_ARCV3)
#define OFFSET_FOR_VECTOR(VECNO) (VECNO << 3)
#else
#error Should never be reached
#endif


#endif
