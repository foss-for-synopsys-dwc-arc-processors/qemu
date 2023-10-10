/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2020 Cupertino Miranda (cmiranda@synopsys.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * http://www.gnu.org/licenses/lgpl-2.1.html
 */

#ifndef ARC64_MMUV6_H
#define ARC64_MMUV6_H

#include "target/arc/mmu-common.h"
#include "hw/registerfields.h"

/*
 * MMU Control Register bits
 * MMU_CTRL, AUX address: 0x468, access: RW
 *
 *   EN[0]      - Enabled
 *   KU[1]      - Kernel mode access to user mode data
 *   WX[2]      - Writeable pages execution behavior
 *   TCE[3]     - Translation cache enable
 *   R[31:4]   - Reserved
 */

FIELD(MMU_CTRL, EN, 0, 1)
FIELD(MMU_CTRL, KU, 1, 1)
FIELD(MMU_CTRL, WX, 2, 1)
FIELD(MMU_CTRL, TCE, 3, 1)

/*
 * MMU Translation Table Base Control Register bits
 * MMU_TTBC, AUX address: 0x469, access: RW
 *
 *   T0SZ[4:0]      - Size of the virtual region mapped by translation table 0
 *   T0SH[6:5]      - Share attributes for translation table 0
 *   T0C[7]         - Cache attribute for translation table 0
 *   R[14:8]        - Reserved
 *   A1[15]         - When this bit is asserted, the execute permissions are
 *                    not influenced by the Access Permissions bits
 *   T1SZ[20:16]    - Size of the virtual region mapped by translation table 1
 *   T1SH[22:21]    - Share attributes for translation table 1
 *   T1C[23]        - Cache attribute for translation table 1
 *   R[31:24]       - Reserved
 */

FIELD(MMU_TTBC, T0SZ, 0, 5)
FIELD(MMU_TTBC, T0SH, 5, 2)
FIELD(MMU_TTBC, T0C, 7, 1)
FIELD(MMU_TTBC, A1, 15, 1)
FIELD(MMU_TTBC, T1SZ, 16, 5)
FIELD(MMU_TTBC, T1SH, 21, 2)
FIELD(MMU_TTBC, T1C, 23, 1)

struct arc_mmuv6 {
    struct mmuv6_exception {
      int32_t number;
      uint8_t causecode;
      uint8_t parameter;
    } exception;

    uint32_t ctrl;          /* Control Register */
    uint32_t ttbcr;         /* Translation Table Base Control Register */
    uint64_t fault_status;  /* Fault Status Register */
    uint64_t rtp0;          /* Root Translation Pointer0 Register (Low + High) */
    uint64_t rtp1;          /* Root Translation Pointer1 Register (Low + High) */
};

#endif /* ARC64_MMUV6_H */
