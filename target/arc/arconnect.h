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
#include "hw/registerfields.h"
#include "target/arc/irq.h"

/*
 * ARC HS3x and HS4x families support up to 4 cores in SMP configuration despite
 * the fact that ARConnect's commands support a larger number of cores. In turn,
 * ARC HS5x and HS6x families support up to 12 cores in SMP configuration.
 */

#define ARCV2_MAX_CORES_NUMBER 4
#define ARCV3_MAX_CORES_NUMBER 12
#define ARC_MAX_CORES_NUMBER ARCV3_MAX_CORES_NUMBER

/*
 * ARConnect's state consist of a set of build configuration registers and
 * other internal registers. Registers that are implemented in QEMU are
 * described in details further. A summary of build configuration registers
 * (they are accessed by the guest through arconnect_regs_get and
 * arconnect_regs_set helpers):
 *
 * Name                     Address     Access  Implemented     ID in QEMU
 * CONNECT_SYSTEM_BUILD     0xD0        R       Yes             AUX_ID_mcip_bcr
 * CONNECT_SEMA_BUILD       0xD1        R       No
 * CONNECT_MESSAGE_BUILD    0xD2        R       No
 * CONNECT_PMU_BUILD        0xD3        R       No
 * CONNECT_IDU_BUILD        0xD5        R       Yes             AUX_ID_mcip_idu_bcr
 * CONNECT_GFRC_BUILD       0xD6        R       Yes             AUX_ID_mcip_gfrc_bcr
 * CONNECT_IVC_BUILD        0xD7        R       No
 * CONNECT_ICI_BUILD        0xE0        R       Yes             AUX_ID_mcip_ici_bcr
 * CONNECT_ICD_BUILD        0xE1        R       No
 * CONNECT_ASI_BUILD        0xE2        R       No
 * CONNECT_PDM_BUILD        0xE3        R       No
 */

typedef struct {
    uint32_t system_build;  /* ARConnect Build Configuration Register, CONNECT_SYSTEM_BUILD */
    uint32_t idu_build;     /* Interrupt Distribution Unit BCR, CONNECT_IDU_BUILD */
    uint32_t gfrc_build;    /* Global Free Running Counter BCR, CONNECT_GFRC_BUILD */
    uint32_t ici_build;     /* Inter-Core Interrupt Unit BCR, CONNECT_ICI_BUILD */
    bool idu_enabled;       /* Internal IDU enable/disable register */
    bool initialized;       /* Is ARConnect initialized? */
    QemuMutex ici_mutex;
} ARCArconnectGlobalState;

extern ARCArconnectGlobalState arconnect_state;

/*
 * LPA/LF hash table.
 */

#define LPA_LFS_ALIGNEMENT_BITS 2 /* Inforced alignement */
#define LPA_LFS_ALIGNEMENT_MASK ((1 << LPA_LFS_ALIGNEMENT_BITS) - 1)
#define LPA_LFS_SIZE_IN_BITS    8 /* Size for concurrent lpa_entries */
#define LPA_LFS_SIZE            (1 << LPA_LFS_SIZE_IN_BITS)
#define LPA_LFS_ENTRY_FOR_PA(PA) \
    (((PA >> LPA_LFS_ALIGNEMENT_BITS)  \
      ^ (PA >> (LPA_LFS_SIZE_IN_BITS + LPA_LFS_ALIGNEMENT_BITS))) \
     & ((1 << LPA_LFS_SIZE_IN_BITS) - 1))

struct lpa_lf_entry {
    QemuMutex mutex;
    target_ulong lpa_lf;
    uint64_t read_value;
};

extern struct lpa_lf_entry lpa_lfs[LPA_LFS_SIZE];

/*
 * Each core has its own set of internal registers associated with ARConnect.
 * A summary of internal control registers (they are accessed by the guest
 * through arconnect_regs_get and arconnect_regs_set helpers too):
 *
 * Name                     Address     Access  Implemented     ID in QEMU
 * CONNECT_CMD              0x600       RW      Yes             AUX_ID_mcip_cmd
 * CONNECT_WDATA            0x601       RW      Yes             AUX_ID_mcip_wdata
 * CONNECT_READBACK         0x602       R       Yes             AUX_ID_mcip_readback
 * CONNECT_READBACK_64      0x603       R       No
 * 
 * An ARConnect command is sent to ARConnect and processed when a command with
 * an optional parameter is saved to CONNECT_CMD auxilary register. Depending
 * on a command, CONNECT_WDATA may be used to send extra data to ARConnect.
 * An answer for ARConnect is stored CONNECT_READBACK.
 *
 * CONNECT_READBACK_64 is used by ARC HS6x family for storing a 64-bit value
 * of GFRC clock, so the value may be read using just one command.
 */

typedef struct {
    uint32_t cmd;           /* ARConnect Command Register, CONNECT_CMD */
    uint32_t wdata;         /* ARConnect Write Data Register, CONNECT_WDATA */
    uint32_t readback;      /* ARConnect Read Data Register, CONNECT_READBACK */

    /*
     * Each core is assigned an internal INTRPT_STATUS register in ICI to
     * record the status of the interrupt (acknowledged or not) from it to all
     * other cores. The CMD_INTRPT_GENERATE_IRQ command sets one bit in the
     * interrupt initiator core’s INTRPT_STATUS register corresponding to the
     * interrupt receiver core.
     *
     * However, it's not efficient to store INTRPT_STATUS register as is.
     * Instead, there is pending_senders variable. N-th bit of the variable
     * corresponds to the N-th core that is waiting for an acknowledgement from
     * the core this variable belongs to.
     *
     * For example, if pending_senders of the core is 0b0000...101 then it
     * means that cores with CORE_ID 0 and 2 sent ICI interrupt to the core and
     * are waiting for an acknowledgement.
     */
    uint32_t pending_senders;

    /*
     * GFRC snapshot is store here after reading a low word using
     * CMD_GFRC_READ_LO ARConnect command.
     */
    uint64_t gfrc_snapshot;

    struct lpa_lf_entry *lpa_lf;
    QemuMutex *locked_mutex;
} ARCArconnectCPUState;

/*
 * Interrupt Distribution Unit (IDU) is a distinct interrupt controller in
 * ARConnect. Interrupts (common IRQs) connected to IDU are shared between
 * cores. In turn, each common IRQ (CIRQ) of IDU is connected to each core
 * starting from IRQ 24 (up to 128 common IRQs).
 *
 * Each common interrupt is assigned a number of internal registers:
 *
 *     MODE - Configures a triggering mode (level triggered or edge triggered)
 *            and a distribution mode (round-robin, first-acknowledge or
 *            all-destination). It's configured by CMD_IDU_SET_MODE command and
 *            read by CMD_IDU_READ_MODE command
 *
 *     DEST - Configures the target cores to receive the specified
 *            common interrupt when it is triggered. It's configured by
 *            CMD_IDU_SET_DEST command and read by CMD_IDU_READ_DEST command
 *
 *     MASK - Each core can use the CMD_IDU_SET_MASK command to mask or unmask
 *            the specified common interrupt. It 1 is stored in MASK, then
 *            the common interrupt is masked. CMD_IDU_READ_MASK is used
 *            to read content of the common interrupts MASK register.
 */

#define FIRST_COMMON_IRQ 24
#define MAX_NR_OF_COMMON_IRQS 128

typedef struct {
    uint32_t mode;
    uint32_t dest;
    uint32_t mask;
} ARCArconnectCommonIRQ;

extern ARCArconnectCommonIRQ arconnect_cirq_array[MAX_NR_OF_COMMON_IRQS];

/*
 * ARConnect Build Configuration Register bits
 * CONNECT_SYSTEM_BUILD, AUX address: 0xD0, access: R
 *
 *   VERSION[7:0]   - 0x1 for the first version, 0x2 for the current version
 *   ASI[8]         - ARConnect Slave Interface unit
 *   ICI[9]         - Inter-Core Interrupt unit
 *   ICS[10]        - Inter-Core Semaphore unit
 *   ICM[11]        - Inter-Core Message unit
 *   PMU[12]        - Power Management unit
 *   ICD[13]        - Inter-Code Debug unit
 *   GFRC[14]       - Global Free-Running Counter
 *   R[15]          - Reserved
 *   CORENUM[21:16] - Number of cores connected to ARConnect
 *   R[22]          - Reserved
 *   IDU[23]        - Interrupt Distribution unit
 *   R[24]          - Reserved
 *   PDM[25]        - Power Domain Management unit
 *   IVC[26]        - Interrupt Vector base Control unit
 *   R[31:27]       - Reserved
 */

FIELD(CONNECT_SYSTEM_BUILD, VERSION, 0, 8)
FIELD(CONNECT_SYSTEM_BUILD, ASI, 8, 1)
FIELD(CONNECT_SYSTEM_BUILD, ICI, 9, 1)
FIELD(CONNECT_SYSTEM_BUILD, ICS, 10, 1)
FIELD(CONNECT_SYSTEM_BUILD, ICM, 11, 1)
FIELD(CONNECT_SYSTEM_BUILD, PMU, 12, 1)
FIELD(CONNECT_SYSTEM_BUILD, ICD, 13, 1)
FIELD(CONNECT_SYSTEM_BUILD, GFRC, 14, 1)
FIELD(CONNECT_SYSTEM_BUILD, CORENUM, 16, 6)
FIELD(CONNECT_SYSTEM_BUILD, IDU, 23, 1)
FIELD(CONNECT_SYSTEM_BUILD, PDM, 25, 1)
FIELD(CONNECT_SYSTEM_BUILD, IVC, 26, 1)

/*
 * ARConnect Interrupt Distribution Unit Build Configuration Register bits
 * CONNECT_IDU_BUILD, AUX address: 0xD5, access: R
 *
 *   VERSION[7:0]   - 0x1 or 0x2
 *   CIRQNUM[10:8]  - Number of common interrupts supported:
 *                      0x0 = 4 common interrupts
 *                      0x1 = 8 common interrupts
 *                      0x2 = 16 common interrupts
 *                      0x3 = 32 common interrupts
 *                      0x4 = 64 common interrupts
 *                      0x5 = 128 common interrupts
 *   R[31:11]       - Reserved
 */

FIELD(CONNECT_IDU_BUILD, VERSION, 0, 8)
FIELD(CONNECT_IDU_BUILD, CIRQNUM, 8, 3)

#define ARCONNECT_IDU_VERSION 0x1
#define ARCONNECT_IDU_CIRQNUM 0x5

/*
 * ARConnect Global Free Running Counter Build Configuration Register bits
 * CONNECT_GFRC_BUILD, AUX address: 0xD6, access: R
 *
 *   VERSION[7:0]   - 0x1, 0x2, 0x3, 0x4 or 0x5
 *   R[31:8]        - Reserved
 */

FIELD(CONNECT_GFRC_BUILD, VERSION, 0, 8)

#define ARCONNECT_GFRC_VERSION 0x2

/*
 * ARConnect Inter-Core Interrupt Unit Build Configuration Register bits
 * CONNECT_ICI_BUILD, AUX address: 0xE0, access: R
 *
 *   VERSION[7:0]   - 0x2, 0x3 or 0x4
 *   R[31:8]        - Reserved
 */

FIELD(CONNECT_ICI_BUILD, VERSION, 0, 8)

#define ARCONNECT_ICI_VERSION 0x2

/*
 * ARConnect Command Register bits
 * CONNECT_CMD, AUX address: 0x600, access: RW
 * 
 * The register is used for sending commands to ARConnect.
 *
 *   COMMAND[7:0]       - Command to ARConnect
 *   PARAMETER[23:8]    - Parameter field of the command to ARConnect
 *   R[31:24]           - Reserved
 */

FIELD(CONNECT_CMD, COMMAND, 0, 8)
FIELD(CONNECT_CMD, PARAMETER, 8, 16)

/*
 * ARConnect Inter-Core Interrupt unit commands use PARAMETER field in
 * COMMAND_CMD register for setting a type of ICI interrupts and a receiver's
 * core number.
 * 
 * if ARCONNECT_CMD_INTRPT_PARAMETER_CORE_ID_MASK bit is set, then a receiver is an
 * external system (not supported yet) and a receiver's CORE_ID, encoded
 * in ARCONNECT_CMD_INTRPT_PARAMETER_EXTERNAL_RECEIVER_MASK bits, is ignored.
 */

#define ARCONNECT_CMD_INTRPT_PARAMETER_CORE_ID_MASK 0x1f
#define ARCONNECT_CMD_INTRPT_PARAMETER_EXTERNAL_RECEIVER_MASK 0x80

/*
 * The enumeration of all ARConnect commands, even unsupported by QEMU.
 */

#define ARCONNECT_COMMAND(NAME, VALUE) ARCONNECT_ ## NAME = VALUE,
enum ARCArconnectCommandEnum {
#include "arconnect_commands.def"
};
#undef ARCONNECT_COMMAND

/*
 * This array matches ARConnect command's number to command's symbolic name.
 */

extern const char *arc_arconnect_command_name_array[];

void arc_arconnect_init(ARCCPU *cpu);

#endif /* __ARC_ARCONNECT_H__ */
