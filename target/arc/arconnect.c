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
#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/error-report.h"
#include "target/arc/regs.h"

enum arconnect_commands {
    ARCON_CMD_CHECK_CORE_ID = 0x0,
    ARCON_CMD_INTRPT_GENERATE_IRQ = 0x1,
    ARCON_CMD_INTRPT_GENERATE_ACK,
    ARCON_CMD_INTRPT_READ_STATUS,
    ARCON_CMD_INTRPT_CHECK_SOURCE,
    ARCON_CMD_SEMA_CLAIM_AND_READ = 0x11,
    ARCON_CMD_SEMA_SEMA_RELEASE,
    ARCON_CMD_SEMA_FORCE_RELEASE,
    ARCON_CMD_MSG_SRAM_SET_ADDR = 0x21,
    ARCON_CMD_MSG_SRAM_READ_ADDR,
    ARCON_CMD_MSG_SRAM_SET_ADDR_OFFSET,
    ARCON_CMD_MSG_SRAM_READ_ARRR_OFFSET,
    ARCON_CMD_MSG_SRAM_WRITE,
    ARCON_CMD_MSG_SRAM_WRITE_INC,
    ARCON_CMD_MSG_SRAM_WRITE_IMM,
    ARCON_CMD_MSG_SRAM_READ,
    ARCON_CMD_MSG_SRAM_READ_INC,
    ARCON_CMD_MSG_SRAM_READ_IMM,
    ARCON_CMD_MSG_SET_ECC_CTRL,
    ARCON_CMD_MSG_READ_ECC_CTRL,
    ARCON_CMD_MSG_READ_ECC_SBE_CNT,
    ARCON_CMD_MSG_CLEAR_ECC_SBE_CNT,
    ARCON_CMD_DEBUG_RESET = 0x31,
    ARCON_CMD_DEBUG_HALT,
    ARCON_CMD_DEBUG_RUN,
    ARCON_CMD_DEBUG_SET_MASK,
    ARCON_CMD_DEBUG_READ_MASK,
    ARCON_CMD_DEBUG_SET_SELECT,
    ARCON_CMD_DEBUG_READ_SELECT,
    ARCON_CMD_DEBUG_READ_EN,
    ARCON_CMD_DEBUG_READ_CMD,
    ARCON_CMD_DEBUG_READ_CORE,
    ARCON_CMD_GFRC_CLEAR = 0x41,
    ARCON_CMD_GFRC_READ_LO,
    ARCON_CMD_GFRC_READ_HI,
    ARCON_CMD_GFRC_ENABLE,
    ARCON_CMD_GFRC_DISABLE,
    ARCON_CMD_GFRC_READ_DISABLE,
    ARCON_CMD_GFRC_SET_CORE,
    ARCON_CMD_GFRC_READ_CORE,
    ARCON_CMD_GFRC_READ_HALT,
    ARCON_CMD_PMU_SET_PUCNT = 0x51,
    ARCON_CMD_PMU_READ_PICNT,
    ARCON_CMD_PMU_SET_RSTCNT,
    ARCON_CMD_PMU_READ_RSTCNT,
    ARCON_CMD_PMU_SET_PDCNT,
    ARCON_CMD_PMU_READ_PDCNT,
    ARCON_CMD_IDU_ENABLE = 0x71,
    ARCON_CMD_IDU_DISABLE,
    ARCON_CMD_IDU_READ_ENABLE,
    ARCON_CMD_IDU_SET_MODE,
    ARCON_CMD_IDU_READ_MODE,
    ARCON_CMD_IDU_SET_DEST,
    ARCON_CMD_IDU_READ_DEST,
    ARCON_CMD_IDU_GEN_CIRQ,
    ARCON_CMD_IDU_ACK_CIRQ,
    ARCON_CMD_IDU_CHECK_STATUS,
    ARCON_CMD_IDU_CHECK_SOURCE,
    ARCON_CMD_IDU_SET_MASK,
    ARCON_CMD_IDU_READ_MASK,
    ARCON_CMD_IDU_CHECK_FIRST,
    ARCON_CMD_IDU_SET_PM = 0x81,
    ARCON_CMD_IDU_READ_PSTATUS
};

static void arconnect_intercore_intr_unit_cmd(enum arconnect_commands cmd)
{
    switch(cmd) {
    case ARCON_CMD_INTRPT_GENERATE_IRQ:
    case ARCON_CMD_INTRPT_GENERATE_ACK:
    case ARCON_CMD_INTRPT_READ_STATUS:
    case ARCON_CMD_INTRPT_CHECK_SOURCE:
        arconnect_intercore_intr_unit_cmd(cmd);
        break;
    default:
        break;
    };
}

static void arconnect_command_process(enum arconnect_commands cmd)
{
    switch(cmd) {
    case ARCON_CMD_INTRPT_GENERATE_IRQ:
    case ARCON_CMD_INTRPT_GENERATE_ACK:
    case ARCON_CMD_INTRPT_READ_STATUS:
    case ARCON_CMD_INTRPT_CHECK_SOURCE:
        arconnect_intercore_intr_unit_cmd(cmd);
        break;
    default:
        break;
    };
}

target_ulong
arconnect_regs_get(const struct arc_aux_reg_detail *aux_reg_detail,
                          void *data)
{
    return 0;
}
void
arconnect_regs_set(const struct arc_aux_reg_detail *aux_reg_detail,
                     target_ulong val, void *data)
{
    switch(aux_reg_detail->id) {
    case AUX_ID_mcip_cmd:
        arconnect_command_process((enum arconnect_commands) val);
        break;
    case AUX_ID_mcip_wdata:
        break;
    case AUX_ID_mcip_readback:
        break;

    default:
        assert(0);
        break;
    }
}
