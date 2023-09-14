/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2021 Synopsys Inc.
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
#include "qemu/log.h"
#include "qemu/error-report.h"
#include "target/arc/regs.h"
#include "target/arc/cpu.h"
#include "target/arc/arconnect.h"
#include "target/arc/irq.h"
#include "hw/irq.h"
#include "exec/exec-all.h"
#include "qemu/main-loop.h"
#include "target/arc/timer.h"

#ifndef CONFIG_USER_ONLY
#include "hw/boards.h"
#endif

/*
 * Definition of ARConnect's state structures: a general state, LPA/LF hash
 * table and an array of common interrupts.
 */

ARCArconnectGlobalState arconnect_state;
ARCArconnectCommonIRQ arconnect_cirq_array[MAX_NR_OF_COMMON_IRQS];
struct lpa_lf_entry lpa_lfs[LPA_LFS_SIZE];

/*
 * A function for initializing of ARConnect state. ARConnect state may be
 * initialized only once.
 */

static void init_arconnect_state(ARCArconnectGlobalState *state)
{
    /*
     * Get a number of cores connected to ARConnect from machine's
     * MachineState structure. It's not available and meaningless in user mode,
     * thus prevent reading MachineState in this mode.
     */
#ifndef CONFIG_USER_ONLY
    uint32_t corenum = MACHINE(qdev_get_machine())->smp.cpus;
#else
    uint32_t corenum = 1;
#endif

    uint32_t reg;
    int i;

    if (!qatomic_xchg(&state->initialized, true)) {
        /* Initialize subsystems' mutexes*/
        qemu_mutex_init(&state->ici_mutex);

        /* Initialize ARConnect BCR */
        reg = FIELD_DP32(0,   CONNECT_SYSTEM_BUILD, VERSION,    0x3);
        reg = FIELD_DP32(reg, CONNECT_SYSTEM_BUILD, ICI,        0x1);
        reg = FIELD_DP32(reg, CONNECT_SYSTEM_BUILD, GFRC,       0x1);
        reg = FIELD_DP32(reg, CONNECT_SYSTEM_BUILD, CORENUM,    corenum);
        reg = FIELD_DP32(reg, CONNECT_SYSTEM_BUILD, IDU,        0x1);
        state->system_build = reg;

        /* Initialize IDU BCR */
        reg = FIELD_DP32(0,   CONNECT_IDU_BUILD, VERSION, ARCONNECT_IDU_VERSION);
        reg = FIELD_DP32(reg, CONNECT_IDU_BUILD, CIRQNUM, ARCONNECT_IDU_CIRQNUM);
        state->idu_build = reg;

        /* Initialize GFRC BCR */
        reg = FIELD_DP32(0, CONNECT_GFRC_BUILD, VERSION, ARCONNECT_GFRC_VERSION);
        state->gfrc_build = reg;

        /* Initialize ICI BCR */
        reg = FIELD_DP32(0, CONNECT_ICI_BUILD, VERSION, ARCONNECT_ICI_VERSION);
        state->ici_build = reg;

        /* By default, IDU is disabled. */
        state->idu_enabled = false;

        /* Initialize all llock/scond lpa entries and respective mutexes */
        for(i = 0; i < LPA_LFS_SIZE; i++) {
            lpa_lfs[i].lpa_lf = 0;
            qemu_mutex_init(&lpa_lfs[i].mutex);
        }
    }
}

/* 
 * Get ARCCPU object core that corresponds to the particular CORE_ID. It's a
 * a useful helper for ARConnect commands since some commands address CPU's by
 * CORE_ID.
 */

static ARCCPU *core_id_to_cpu[ARC_MAX_CORES_NUMBER];

static ARCCPU *get_cpu_for_core(uint8_t core_id)
{
    assert(core_id < ARC_MAX_CORES_NUMBER);

    return core_id_to_cpu[core_id];
}

/*
 * This function is used to initialize per-CPU ARConnect structures. Global
 * ARConnect state is initialized by init_arconnect_state only once.
 */

void arc_arconnect_init(ARCCPU *cpu)
{
    init_arconnect_state(&arconnect_state);

    assert(cpu->core_id < ARC_MAX_CORES_NUMBER);

    core_id_to_cpu[cpu->core_id] = cpu;
    cpu->env.arconnect.pending_senders = 0;
    cpu->env.arconnect.lpa_lf = &(lpa_lfs[0]);
    cpu->env.arconnect.locked_mutex = &(lpa_lfs[0].mutex);
}

/*
 * An ARConnect command is sent to ARConnect and processed when a command with
 * an optional parameter is saved to CONNECT_CMD auxilary register. Depending
 * on a command, CONNECT_WDATA may be used to send extra data to ARConnect.
 * An answer for ARConnect is stored CONNECT_READBACK.
 *
 * Each ARConnect subsystem has its own set of ARConnect commands.
 * arconnect_command_handler receives a command and passes it to a corresponding
 * subsystem handler:
 *
 *     ici_command_handler - Inter-Core Interrupt unit commands
 *     idu_command_handler - Interrupt Distribution Unit commands
 *     gfrc_command_handler - Global Free Running Counter commands
 */

#define ARCONNECT_COMMAND(NAME, VALUE) [VALUE] = #NAME,
const char *arc_arconnect_command_name_array[] = {
#include "arconnect_commands.def"
};
#undef ARCONNECT_COMMAND

static void ici_command_handler(CPUARCState *env, uint8_t command, uint16_t parameter);
static void idu_command_handler(CPUARCState *env, uint8_t command, uint16_t parameter);
static void gfrc_command_handler(CPUARCState *env, uint8_t command, uint16_t parameter);

static void arconnect_command_handler(CPUARCState *env)
{
    uint8_t command = FIELD_EX32(env->arconnect.cmd, CONNECT_CMD, COMMAND);
    uint16_t parameter = FIELD_EX32(env->arconnect.cmd, CONNECT_CMD, PARAMETER);
    uint8_t core_id = env_archcpu(env)->core_id;

    qemu_log_mask(CPU_LOG_INT,
            "[ICI %d] Process command %s with param %d.\n",
            core_id, arc_arconnect_command_name_array[command], parameter);

    switch(command) {
    case ARCONNECT_CMD_CHECK_CORE_ID:
        env->arconnect.readback = core_id & 0x1f;
        break;
    case ARCONNECT_CMD_INTRPT_GENERATE_IRQ:
    case ARCONNECT_CMD_INTRPT_GENERATE_ACK:
    case ARCONNECT_CMD_INTRPT_READ_STATUS:
    case ARCONNECT_CMD_INTRPT_CHECK_SOURCE:
    case ARCONNECT_CMD_INTRPT_GENERATE_ACK_BIT_MASK:
    case ARCONNECT_CMD_INTRPT_EXT_MODE:
    case ARCONNECT_CMD_INTRPT_SET_PULSE_CNT:
    case ARCONNECT_CMD_INTRPT_READ_PULSE_CNT:
        ici_command_handler(env, command, parameter);
        break;
    case ARCONNECT_CMD_GFRC_CLEAR:
    case ARCONNECT_CMD_GFRC_READ_LO:
    case ARCONNECT_CMD_GFRC_READ_HI:
    case ARCONNECT_CMD_GFRC_ENABLE:
    case ARCONNECT_CMD_GFRC_DISABLE:
    case ARCONNECT_CMD_GFRC_READ_DISABLE:
    case ARCONNECT_CMD_GFRC_SET_CORE:
    case ARCONNECT_CMD_GFRC_READ_CORE:
    case ARCONNECT_CMD_GFRC_READ_HALT:
    case ARCONNECT_CMD_GFRC_CLK_ENABLE:
    case ARCONNECT_CMD_GFRC_CLK_DISABLE:
    case ARCONNECT_CMD_GFRC_READ_CLK_STATUS:
    case ARCONNECT_CMD_GFRC_READ_FULL:
        gfrc_command_handler(env, command, parameter);
        break;
    case ARCONNECT_CMD_IDU_ENABLE:
    case ARCONNECT_CMD_IDU_DISABLE:
    case ARCONNECT_CMD_IDU_READ_ENABLE:
    case ARCONNECT_CMD_IDU_SET_MODE:
    case ARCONNECT_CMD_IDU_READ_MODE:
    case ARCONNECT_CMD_IDU_SET_DEST:
    case ARCONNECT_CMD_IDU_READ_DEST:
    case ARCONNECT_CMD_IDU_GEN_CIRQ:
    case ARCONNECT_CMD_IDU_ACK_CIRQ:
    case ARCONNECT_CMD_IDU_CHECK_STATUS:
    case ARCONNECT_CMD_IDU_CHECK_SOURCE:
    case ARCONNECT_CMD_IDU_SET_MASK:
    case ARCONNECT_CMD_IDU_READ_MASK:
    case ARCONNECT_CMD_IDU_CHECK_FIRST:
        idu_command_handler(env, command, parameter);
        break;
    case ARCONNECT_CMD_SEMA_CLAIM_AND_READ:
    case ARCONNECT_CMD_SEMA_RELEASE:
    case ARCONNECT_CMD_SEMA_FORCE_RELEASE:
        error_report("Inter-Core Semaphore unit is not supported yet.");
        exit(EXIT_FAILURE);
    case ARCONNECT_CMD_MSG_SRAM_SET_ADDR:
    case ARCONNECT_CMD_MSG_SRAM_READ_ADDR:
    case ARCONNECT_CMD_MSG_SRAM_SET_ADDR_OFFSET:
    case ARCONNECT_CMD_MSG_SRAM_READ_ADDR_OFFSET:
    case ARCONNECT_CMD_MSG_SRAM_WRITE:
    case ARCONNECT_CMD_MSG_SRAM_WRITE_INC:
    case ARCONNECT_CMD_MSG_SRAM_WRITE_IMM:
    case ARCONNECT_CMD_MSG_SRAM_READ:
    case ARCONNECT_CMD_MSG_SRAM_READ_INC:
    case ARCONNECT_CMD_MSG_SRAM_READ_IMM:
    case ARCONNECT_CMD_MSG_SET_ECC_CTRL:
    case ARCONNECT_CMD_MSG_READ_ECC_CTRL:
    case ARCONNECT_CMD_MSG_READ_ECC_SBE_CNT:
    case ARCONNECT_CMD_MSG_CLEAR_ECC_SBE_CNT:
        error_report("Inter-Core Message unit is not supported yet.");
        exit(EXIT_FAILURE);
    case ARCONNECT_CMD_DEBUG_RESET:
    case ARCONNECT_CMD_DEBUG_HALT:
    case ARCONNECT_CMD_DEBUG_RUN:
    case ARCONNECT_CMD_DEBUG_SET_MASK:
    case ARCONNECT_CMD_DEBUG_READ_MASK:
    case ARCONNECT_CMD_DEBUG_SET_SELECT:
    case ARCONNECT_CMD_DEBUG_READ_SELECT:
    case ARCONNECT_CMD_DEBUG_READ_EN:
    case ARCONNECT_CMD_DEBUG_READ_CMD:
    case ARCONNECT_CMD_DEBUG_READ_CORE:
        error_report("Inter-Core Debug unit is not supported yet.");
        exit(EXIT_FAILURE);
    case ARCONNECT_CMD_PDM_SET_PM:
    case ARCONNECT_CMD_PDM_READ_PSTATUS:
        error_report("Power Domain Management commands are not supported yet.");
        exit(EXIT_FAILURE);
    case ARCONNECT_CMD_IVC_SET:
    case ARCONNECT_CMD_IVC_READ:
    case ARCONNECT_CMD_IVC_SET_HI:
    case ARCONNECT_CMD_IVC_READ_HI:
    case ARCONNECT_CMD_IVC_READ_FULL:
        error_report("Interrupt Vector Base Control unit is not supported yet.");
        exit(EXIT_FAILURE);
    case ARCONNECT_CMD_PMU_SET_PUCNT:
    case ARCONNECT_CMD_PMU_READ_PUCNT:
    case ARCONNECT_CMD_PMU_SET_RSTCNT:
    case ARCONNECT_CMD_PMU_READ_RSTCNT:
    case ARCONNECT_CMD_PMU_SET_PDCNT:
    case ARCONNECT_CMD_PMU_READ_PDCNT:
        error_report("Power Management Unit is not supported yet.");
        exit(EXIT_FAILURE);
    default:
        error_report("Unsupported ARConnect command: %d.", command);
        exit(EXIT_FAILURE);
    };
}

/*
 * Inter-Core Interrupt (ICI) unit allows sending and interrupt from one core
 * to another to notify about an event.
 *
 * Each core is assigned an internal INTRPT_STATUS register. CMD_INTRPT_GENERATE_IRQ
 * command sets one bit in the interrupt initiator core's INTRPT_STATUS register
 * corresponding to the interrupt receiver core.
 *
 * The interrupt line of the interrupt receiver core is asserted after
 * ARConnect issues this command and is asserted until the interrupt receiver
 * core acknowledges all the inter-core interrupts it received.
 *
 * Implemented commands:
 *
 *     CMD_INTRPT_GENERATE_IRQ
 *
 *         Command field value: 0x01
 *
 *         Parameter field value:
 *
 *             PARAMETER[4:0] (PARAMETER[5:0] for ARCv3) specifies the CORE_ID
 *             of the receiver core. PARAMETER[8] defines whether the interrupt
 *             is generated to the external system (in QEMU it must be 0).
 *
 *         Description:
 *
 *             Generate an interrupt to another core or to the external system.
 *             If PARAMETER[8] is set then CORE_ID value is ignored (this case
 *             is not considered by QEMU). The command sets one bit in the
 *             interrupt initiator core's INTRPT_STATUS register corresponding
 *             to the interrupt receiver core
 *
 *     CMD_INTRPT_GENERATE_ACK
 *
 *         Command field value: 0x02
 *
 *         Parameter field value:
 *
 *             PARAMETER[4:0] (PARAMETER[5:0] for ARCv3) specifies the CORE_ID
 *             of the interrupt initiator core. PARAMETER[8] defines whether
 *             the interrupt is generated to the external system (in QEMU it
 *             must be 0).
 *
 *         Description:
 *
 *             Clear the bit that corresponds to the interrupt receiver core
 *             in the interrupt initiator core's INTRPT_STATUS register.
 *             If PARAMETER[8] is set then CORE_ID value is ignored (this case
 *             is not considered by QEMU)
 *
 *     CMD_INTRPT_READ_STATUS
 *
 *         Command field value: 0x03
 *
 *         Parameter field value:
 *
 *             PARAMETER[4:0] (PARAMETER[5:0] for ARCv3) specifies the CORE_ID
 *             of the interrupt receiver core.
 *
 *         Description:
 *
 *             The CONNECT_READBACK register returns the value of the
 *             INTRPT_STATUS[CORE_ID] bit.
 *
 *     CMD_INTRPT_CHECK_SOURCE
 *
 *         Command field value: 0x04
 *
 *         Description:
 *
 *             The CONNECT_READBACK register returns a vector gathered from
 *             the internal INTRPT_STATUS registers of all interrupt initiator
 *             cores.
 *
 * Not implemented commands:
 *
 *     Name                                 Command field value
 *     CMD_INTRPT_GENERATE_ACK_BIT_MASK     0x05
 *     CMD_INTRPT_EXT_MODE                  0x06
 *     CMD_INTRPT_SET_PULSE_CNT             0x07
 */

static void ici_command_handler(CPUARCState *env, uint8_t command, uint16_t parameter)
{
    ARCCPU *cpu = env_archcpu(env);
    uint8_t parameter_core_id = parameter & ARCONNECT_CMD_INTRPT_PARAMETER_CORE_ID_MASK;
    uint8_t current_core_id = cpu->core_id;
    uint32_t *senders;

    switch(command) {
    case ARCONNECT_CMD_INTRPT_GENERATE_IRQ:
        /* We don't support external systems for ICI */
        if (parameter & ARCONNECT_CMD_INTRPT_PARAMETER_EXTERNAL_RECEIVER_MASK) {
            error_report("CMD_INTRPT_GENERATE_IRQ command is not support for external systems yet.");
            exit(EXIT_FAILURE);
        }

        /* Do nothing if ICI is sent to the same core. */
        if (parameter_core_id == current_core_id) {
            break;
        }

        qemu_mutex_lock(&arconnect_state.ici_mutex);
        senders = &(get_cpu_for_core(parameter_core_id)->env.arconnect.pending_senders);
        *senders |= 1 << current_core_id;
        qemu_mutex_lock_iothread();
        qemu_irq_raise(get_cpu_for_core(parameter_core_id)->env.irq[ICI_IRQ]);
        qemu_mutex_unlock_iothread();
        qemu_mutex_unlock(&arconnect_state.ici_mutex);
        break;
    case ARCONNECT_CMD_INTRPT_GENERATE_ACK:
        qemu_mutex_lock(&arconnect_state.ici_mutex);
        senders = &env->arconnect.pending_senders;
        *senders &= ~(1 << parameter_core_id);

        /*
         * If all ICI interrupts are processed by the current core then the
         * interrupt line may be safely deasserted.
         */
        if ((*senders) == 0) {
            qemu_mutex_lock_iothread();
            qemu_irq_lower(env->irq[ICI_IRQ]);
            qemu_mutex_unlock_iothread();
        }

        qemu_mutex_unlock(&arconnect_state.ici_mutex);
        break;
    case ARCONNECT_CMD_INTRPT_READ_STATUS:
        qemu_mutex_lock(&arconnect_state.ici_mutex);
        senders = &(get_cpu_for_core(parameter_core_id)->env.arconnect.pending_senders);
        env->arconnect.readback = ((*senders) >> current_core_id) & 0x1;
        qemu_mutex_unlock(&arconnect_state.ici_mutex);
        break;
    case ARCONNECT_CMD_INTRPT_CHECK_SOURCE:
        qemu_mutex_lock(&arconnect_state.ici_mutex);
        env->arconnect.readback = env->arconnect.pending_senders;
        qemu_mutex_unlock(&arconnect_state.ici_mutex);
        break;
    default:
        error_report("ICI command %s (0x%x) is not implemented yet.",
            arc_arconnect_command_name_array[command], command);
        exit(EXIT_FAILURE);
    };
}

/*
 * Interrupt Distribution Unit (IDU) is a distinct interrupt controller in
 * ARConnect. Interrupts (common IRQs) connected to IDU are shared between
 * cores. In turn, each common IRQ (CIRQ) of IDU is connected to each core
 * starting from IRQ 24 (up to 128 common IRQs).
 *
 * ARConnect commands may be used to control distributing of common interrupts
 * between cores. However, there is no a distinct IDU interrupt controller in
 * QEMU yet - all devices are connected to the first core. All implemented IDE
 * are just stubs for setting/reading internal common IRQs' registers (these
 * registers are described in arconnect.h) to keep Linux working without errors.
 * Actually, Linux works well with such implementation of IDU until a full
 * hardware emulation is needed.
 *
 * Implemented commands stubs:
 *
 *     Name                         Command field value
 *     CMD_IDU_ENABLE               0x71
 *     CMD_IDU_DISABLE              0x72
 *     CMD_IDU_READ_ENABLE          0x73
 *     CMD_IDU_SET_MODE             0x74
 *     CMD_IDU_READ_MODE            0x75
 *     CMD_IDU_SET_DEST             0x76
 *     CMD_IDU_READ_DEST            0x77
 *     CMD_IDU_ACK_CIRQ             0x79
 *     CMD_IDU_SET_MASK             0x7C
 *     CMD_IDU_READ_MASK            0x7D
 *
 * Not implemented commands:
 *
 *     Name                         Command field value
 *     CMD_IDU_GEN_CIRQ             0x78
 *     CMD_IDU_CHECK_STATUS         0x7A
 *     CMD_IDU_CHECK_SOURCE         0x7B
 *     CMD_IDU_CHECK_FIRST          0x7E
 */

static void idu_command_handler(CPUARCState *env, uint8_t command, uint16_t parameter)
{
    switch(command) {
    case ARCONNECT_CMD_IDU_SET_MASK:
        qatomic_set(&arconnect_cirq_array[parameter].mask, env->arconnect.wdata);
        break;
    case ARCONNECT_CMD_IDU_READ_MASK:
        env->arconnect.readback = arconnect_cirq_array[parameter].mask;
        break;
    case ARCONNECT_CMD_IDU_SET_DEST:
        qatomic_set(&arconnect_cirq_array[parameter].dest, env->arconnect.wdata);
        break;
    case ARCONNECT_CMD_IDU_READ_DEST:
        env->arconnect.readback = arconnect_cirq_array[parameter].dest;
        break;
    case ARCONNECT_CMD_IDU_SET_MODE:
        qatomic_set(&arconnect_cirq_array[parameter].mode, env->arconnect.wdata);
        break;
    case ARCONNECT_CMD_IDU_READ_MODE:
        env->arconnect.readback = arconnect_cirq_array[parameter].mode;
        break;
    case ARCONNECT_CMD_IDU_ENABLE:
        qatomic_set(&arconnect_state.idu_enabled, true);
        break;
    case ARCONNECT_CMD_IDU_DISABLE:
        qatomic_set(&arconnect_state.idu_enabled, false);
        break;
    case ARCONNECT_CMD_IDU_READ_ENABLE:
        env->arconnect.readback = arconnect_state.idu_enabled;
        break;
    case ARCONNECT_CMD_IDU_ACK_CIRQ:
        /* 
         * TODO: This command must be processed by IDU interrupt controller.
         * Anyway, it must be processed by QEMU to conform a standard common
         * IRQ handling flow.
         */
        break;
    default:
        error_report("IDU command %s (0x%x) is not implemented yet.",
            arc_arconnect_command_name_array[command], command);
    }
}

/*
 * Global Free Running Counter (GFRC) is a clock that is shared between all
 * cores connected to ARConnect.
 *
 * Implemented commands:
 *
 *     CMD_GFRC_READ_LO
 *
 *         Command field value: 0x42
 *
 *         Description:
 *
 *             Read the lower 32-bits of the GFRC counter. Result is saved in
 *             CONNECT_READBACK register.
 *
 *     CMD_GFRC_READ_HI
 *
 *         Command field value: 0x43
 *
 *         Description:
 *
 *             Read the higher 32-bits of the GFRC counter. Result is saved in
 *             CONNECT_READBACK register.
 *
 * Not implemented commands:
 *
 *     Name                         Command field value
 *     CMD_GFRC_ENABLE              0x44
 *     CMD_GFRC_DISABLE             0x45
 *     CMD_GFRC_READ_DISABLE        0x46
 *     CMD_GFRC_SET_CORE            0x47
 *     CMD_GFRC_READ_CORE           0x48
 *     CMD_GFRC_READ_HALT           0x49
 *     CMD_GFRC_CLK_ENABLE          0x4A
 *     CMD_GFRC_CLK_DISABLE         0x4B
 *     CMD_GFRC_READ_CLK_STATUS     0x4C
 *     CMD_GFRC_READ_FULL (HS6x)    0x4D
 */

static void gfrc_command_handler(CPUARCState *env, uint8_t command, uint16_t parameter)
{
    switch(command) {
    case ARCONNECT_CMD_GFRC_READ_LO:
        env->arconnect.gfrc_snapshot = arc_get_global_cycles();
        env->arconnect.readback = (uint32_t) (env->arconnect.gfrc_snapshot & 0xffffffff);
        break;
    case ARCONNECT_CMD_GFRC_READ_HI:
        env->arconnect.readback = (uint32_t) ((env->arconnect.gfrc_snapshot >> 32) & 0xffffffff);
        break;
    default:
        error_report("GFRC command %s (0x%x) is not implemented yet.",
            arc_arconnect_command_name_array[command], command);
    }
}

/*
 * There is a set of auxilary registers in ARConnect. For a detailed information
 * refer to arconnect.h header file that contains a description for implemented
 * registers.
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
 * CONNECT_CMD              0x600       RW      Yes             AUX_ID_mcip_cmd
 * CONNECT_WDATA            0x601       RW      Yes             AUX_ID_mcip_wdata
 * CONNECT_READBACK         0x602       R       Yes             AUX_ID_mcip_readback
 */

/*
 * A helper for getting ARConnect auxilary registers
 */

target_ulong arconnect_regs_get(const struct arc_aux_reg_detail *aux_reg_detail,
                                void *data)
{
    CPUARCState *env = (CPUARCState *) data;

    switch(aux_reg_detail->id) {
    case AUX_ID_mcip_bcr:
        return arconnect_state.system_build;
    case AUX_ID_mcip_idu_bcr:
        return arconnect_state.idu_build;
    case AUX_ID_mcip_gfrc_bcr:
        return arconnect_state.gfrc_build;
    case AUX_ID_mcip_ici_bcr:
        return arconnect_state.ici_build;
    case AUX_ID_mcip_cmd:
        return env->arconnect.cmd;
    case AUX_ID_mcip_wdata:
        return env->arconnect.wdata;
    case AUX_ID_mcip_readback:
        return env->arconnect.readback;
    default:
        error_report("Unsupported ARConnect AUX register: 0x%x.", aux_reg_detail->id);
        exit(EXIT_FAILURE);
    }

    return 0;
}

/*
 * A helper for setting ARConnect auxilary registers
 */

void arconnect_regs_set(const struct arc_aux_reg_detail *aux_reg_detail,
                        target_ulong val, void *data)
{
    CPUARCState *env = (CPUARCState *) data;

    switch(aux_reg_detail->id) {
    case AUX_ID_mcip_cmd:
        env->arconnect.cmd = val;
        arconnect_command_handler(env);
        break;
    case AUX_ID_mcip_wdata:
        env->arconnect.wdata = val;
        break;
    default:
        error_report("Unsupported ARConnect AUX register: 0x%x.", aux_reg_detail->id);
        exit(EXIT_FAILURE);
    }
}
