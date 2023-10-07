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

/*
 * Timer Configuration Register bits
 * TIMER_BUILD, AUX address: 0x75, access: R
 *
 *   VERSION[7:0]   - Version of timers:
 *                        0x4 - ARCv2
 *                        0x6 - ARCv2 with support of TD bit for timers
 *                        0x6 - ARCv3 HS5x
 *                        0x7 - ARCv3 HS6x
 *   T0[8]          - Timer 0 present
 *   T1[9]          - Timer 1 present
 *   RTC[10]        - 64-bit RTC present
 *   R[15:11]       - Reserved
 *   P0[19:16]      - Indicates the interrupt priority level of Timer 0
 *   P1[23:20]      - Indicates the interrupt priority level of Timer 1
 *   R[31:24]       - Reserved
 */

FIELD(ARC_TIMER_BUILD, VERSION, 0, 8)
FIELD(ARC_TIMER_BUILD, T0, 8, 1)
FIELD(ARC_TIMER_BUILD, T1, 9, 1)
FIELD(ARC_TIMER_BUILD, RTC, 10, 1)
FIELD(ARC_TIMER_BUILD, P0, 16, 4)
FIELD(ARC_TIMER_BUILD, P1, 20, 4)

/*
 * Real-Time Counter Control Register bits
 * AUX_RTC_CTRL, AUX address: 0x103, access: RW
 *
 *   E[0]       - Enable: 0 - disabled counting , 1 - enabled counting
 *   C[1]       - A value of 1 clears the AUX_RTC_LOW and AUX_RTC_HIGH registers
 *   R[29:2]    - Reserved
 *   A0[30]     - A bit of atomicity of reads for AUX_RTC_LOW
 *   A1[31]     - A bit of atomicity of reads for AUX_RTC_HIGH
 */

FIELD(ARC_RTC_CTRL, ENABLE, 0, 1)
FIELD(ARC_RTC_CTRL, CLEAR, 1, 1)
FIELD(ARC_RTC_CTRL, A0, 30, 1)
FIELD(ARC_RTC_CTRL, A1, 31, 1)


void arc_initializeTIMER(ARCCPU *);
void arc_resetTIMER(ARCCPU *);

/*
 * Return the number of global clock cycles passed for a particular CPU.
 */

uint64_t arc_get_cycles(CPUARCState *env);

/*
 * Return the number of global clock cycles passed common for all CPUs. It's
 * used by Global Free Running Counter (GFRC) subsystem of ARConnect that is
 * used by CPUs the global clock source.
 */

uint64_t arc_get_global_cycles(void);

#endif
