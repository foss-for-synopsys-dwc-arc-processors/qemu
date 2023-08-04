/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2020 Synppsys Inc.
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

#ifndef __ARC_TIMER_H__
#define __ARC_TIMER_H__

#include "qemu/osdep.h"
#include "qemu/timer.h"

#define FREQ_HZ (env_archcpu(env)->freq_hz)

#define CYCLES_TO_NS(VAL) (muldiv64(VAL, NANOSECONDS_PER_SECOND, FREQ_HZ))
#define NS_TO_CYCLE(VAL)  (muldiv64(VAL, FREQ_HZ, NANOSECONDS_PER_SECOND))

void arc_initializeTIMER(ARCCPU *);
void arc_resetTIMER(ARCCPU *);
uint64_t get_global_cycles(void);

#endif
