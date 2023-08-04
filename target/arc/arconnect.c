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
#include "qemu/main-loop.h"
#include "qemu/log.h"
#include "qemu/error-report.h"
#include "target/arc/regs.h"
#include "target/arc/cpu.h"
#include "target/arc/arconnect.h"
#include "hw/irq.h"
#include "target/arc/timer.h"

struct lpa_lf_entry lpa_lfs[LPA_LFS_SIZE];
static QemuMutex idu_mutex;
static QemuMutex gfrc_mutex;
static uint64_t first_acknowledge_status[MAX_IDU_CIRQS];

enum arconnect_commands {
    CMD_CHECK_CORE_ID = 0x0,
    CMD_INTRPT_GENERATE_IRQ = 0x1,
    CMD_INTRPT_GENERATE_ACK,
    CMD_INTRPT_READ_STATUS,
    CMD_INTRPT_CHECK_SOURCE,
    CMD_SEMA_CLAIM_AND_READ = 0x11,
    CMD_SEMA_SEMA_RELEASE,
    CMD_SEMA_FORCE_RELEASE,
    CMD_MSG_SRAM_SET_ADDR = 0x21,
    CMD_MSG_SRAM_READ_ADDR,
    CMD_MSG_SRAM_SET_ADDR_OFFSET,
    CMD_MSG_SRAM_READ_ARRR_OFFSET,
    CMD_MSG_SRAM_WRITE,
    CMD_MSG_SRAM_WRITE_INC,
    CMD_MSG_SRAM_WRITE_IMM,
    CMD_MSG_SRAM_READ,
    CMD_MSG_SRAM_READ_INC,
    CMD_MSG_SRAM_READ_IMM,
    CMD_MSG_SET_ECC_CTRL,
    CMD_MSG_READ_ECC_CTRL,
    CMD_MSG_READ_ECC_SBE_CNT,
    CMD_MSG_CLEAR_ECC_SBE_CNT,
    CMD_DEBUG_RESET = 0x31,
    CMD_DEBUG_HALT,
    CMD_DEBUG_RUN,
    CMD_DEBUG_SET_MASK,
    CMD_DEBUG_READ_MASK,
    CMD_DEBUG_SET_SELECT,
    CMD_DEBUG_READ_SELECT,
    CMD_DEBUG_READ_EN,
    CMD_DEBUG_READ_CMD,
    CMD_DEBUG_READ_CORE,
    CMD_GFRC_CLEAR = 0x41,
    CMD_GFRC_READ_LO,
    CMD_GFRC_READ_HI,
    CMD_GFRC_ENABLE,
    CMD_GFRC_DISABLE,
    CMD_GFRC_READ_DISABLE,
    CMD_GFRC_SET_CORE,
    CMD_GFRC_READ_CORE,
    CMD_GFRC_READ_HALT,
    CMD_PMU_SET_PUCNT = 0x51,
    CMD_PMU_READ_PICNT,
    CMD_PMU_SET_RSTCNT,
    CMD_PMU_READ_RSTCNT,
    CMD_PMU_SET_PDCNT,
    CMD_PMU_READ_PDCNT,
    CMD_IDU_ENABLE = 0x71,
    CMD_IDU_DISABLE,
    CMD_IDU_READ_ENABLE,
    CMD_IDU_SET_MODE,
    CMD_IDU_READ_MODE,
    CMD_IDU_SET_DEST,
    CMD_IDU_READ_DEST,
    CMD_IDU_GEN_CIRQ,
    CMD_IDU_ACK_CIRQ,
    CMD_IDU_CHECK_STATUS,
    CMD_IDU_CHECK_SOURCE,
    CMD_IDU_SET_MASK,
    CMD_IDU_READ_MASK,
    CMD_IDU_CHECK_FIRST,
    CMD_IDU_SET_PM = 0x81,
    CMD_IDU_READ_PSTATUS
};

/*
 * Setup SMP (arconnect) related data structures
 */
void arc_arconnect_init(ARCCPU *cpu)
{
    int i;

    /* Initialize all llock/scond lpa entries and respective mutexes */
    for(i = 0; i < LPA_LFS_SIZE; i++) {
        lpa_lfs[i].lpa_lf = 0;
        qemu_mutex_init(&lpa_lfs[i].mutex);
    }

    cpu->env.arconnect.intrpt_status = 0;
    cpu->env.arconnect.lpa_lf = &(lpa_lfs[0]);
    cpu->env.arconnect.locked_mutex = &(lpa_lfs[0].mutex);
    cpu->env.arconnect.idu_enabled = false;
    memset(cpu->env.arconnect.idu_data, 0, sizeof(cpu->env.arconnect.idu_data));
    memset(first_acknowledge_status, 0, sizeof(first_acknowledge_status));
    qemu_mutex_init(&idu_mutex);
    qemu_mutex_init(&gfrc_mutex);
}

/* TODO: Find a better way to get cpu for core. */
static ARCCPU *get_cpu_for_core(uint8_t core_id)
{
    CPUState *cs;
    ARCCPU *ret = NULL;
    CPU_FOREACH(cs) {
        if(ARC_CPU(cs)->core_id == core_id) {
            ret = ARC_CPU(cs);
        }
    }
    return ret;
}
static void arcon_status_set(CPUARCState *env, uint8_t status_core_id, uint8_t sender_core_id)
{
    ARCCPU *cpu = env_archcpu(env);
    qemu_log_mask(CPU_LOG_INT,
            "[ICI %d] Set intrpt_status in core %d for sender %d\n",
            cpu->core_id, status_core_id, sender_core_id);
    qatomic_or(&(get_cpu_for_core(status_core_id)->env.arconnect.intrpt_status),
               1 << sender_core_id);
}
static void arcon_status_clr(CPUARCState *env, uint8_t status_core_id, uint8_t sender_core_id)
{
    ARCCPU *cpu = env_archcpu(env);
    qemu_log_mask(CPU_LOG_INT,
            "[ICI %d] Clear intrpt_status in core %d for sender %d\n",
            cpu->core_id, status_core_id, sender_core_id);
    qatomic_and(&(get_cpu_for_core(status_core_id)->env.arconnect.intrpt_status),
                ~(1 << sender_core_id));
}
static uint64_t arcon_status_read(CPUARCState *env, uint8_t core_id)
{
    ARCCPU *cpu = env_archcpu(env);
    uint64_t ret = qatomic_read(&(get_cpu_for_core(core_id)->env.arconnect.intrpt_status));
    qemu_log_mask(CPU_LOG_INT,
            "[ICI %d] Reading intrpt_status in core %d. (read: 0x%lx)\n",
            cpu->core_id, core_id, ret);
    return ret;
}


#define CMD_COREID(V) ((V >> 8) & 0xff)

static void arconnect_intercore_intr_unit_cmd(CPUARCState *env, enum arconnect_commands cmd, uint16_t param)
{
    ARCCPU *cpu = env_archcpu(env);
    uint8_t core_id;

    switch(cmd) {
    case CMD_INTRPT_GENERATE_IRQ:
        if(((param & 0x80) == 0) && ((param & 0x1f) != cpu->core_id)) {
            uint8_t core_id = param & 0x1f;
            arcon_status_set(env, core_id, cpu->core_id);
            qemu_irq_raise(get_cpu_for_core(core_id)->env.irq[MCIP_IRQ]);
        }
        break;
    case CMD_INTRPT_GENERATE_ACK:
        core_id = param & 0x1f;
        arcon_status_clr(env, cpu->core_id, core_id);
        qemu_irq_lower(env->irq[MCIP_IRQ]);
        break;
    case CMD_INTRPT_READ_STATUS:
        core_id = param & 0x1f;
        env->readback = (arcon_status_read(env, core_id) >> cpu->core_id) & 0x1;
        break;
    case CMD_INTRPT_CHECK_SOURCE:
        env->readback = arcon_status_read(env, cpu->core_id);
        break;
    default:
        assert(0);
    };
}

#define ARCON_COMMAND(V) (V & 0xff)
#define ARCON_PARAM(V) ((V >> 8) & 0xffff)

static void arc_cirq_raise(CPUARCState *env, uint16_t cirq)
{
    unsigned int max_cpus = 4; /* TODO: make this variable */
    enum idu_mode_enum mode = env->arconnect.idu_data[cirq].mode;
    uint8_t counter = env->arconnect.idu_data[cirq].counter;
    int dest = env->arconnect.idu_data[cirq].dest;
    bool is_first_acknoledge = false;
    uint8_t core_id = 0;

    switch(mode) {
    case ROUND_ROBIN:
        /* Round robin */
        do {
            core_id = (counter + cirq) % max_cpus;
            counter++;
        } while(((env->arconnect.idu_data[cirq].dest >> core_id) & 0x1) == 0);
        qemu_irq_raise(get_cpu_for_core(core_id)->env.irq[EXT_IRQ + cirq]);
        break;
    case FIRST_ACKNOWLEDGE:
        is_first_acknoledge = true;
        dest = env->arconnect.idu_data[cirq].dest;
        qemu_mutex_lock(&idu_mutex);
        for(core_id = 0; dest != 0 && is_first_acknoledge; core_id++) {
            if((dest & 0x1) != 0) {
                get_cpu_for_core(core_id)->env.arconnect.idu_data[cirq].first_knowl_requested = true;
            }
            dest >>= 1;
        }
        qemu_mutex_unlock(&idu_mutex);
        // fall through
    case ALL_DESTINATION:
        core_id = 0;
        for(core_id = 0; dest != 0; core_id++) {
            if((dest & 0x1) != 0) {
                qemu_irq_raise(get_cpu_for_core(core_id)->env.irq[EXT_IRQ + cirq]);
            }
            dest >>= 1;
        }
        break;
    default:
        assert(0);
        break;
    }
}

static void clear_first_acknowledge(CPUARCState *env, uint16_t cirq)
{
    int dest = env->arconnect.idu_data[cirq].dest;
    int core_id;

    qemu_mutex_lock(&idu_mutex);
    env->readback = env->arconnect.idu_data[cirq].first_knowl_requested;
    for(core_id = 0; dest != 0; core_id++) {
        if((dest & 0x1) != 0) {
            get_cpu_for_core(core_id)->env.arconnect.idu_data[cirq].first_knowl_requested = false;
        }
        dest >>= 1;
    }
    qemu_mutex_unlock(&idu_mutex);
}

static void arconnect_idu_cmd(CPUARCState *env, enum arconnect_commands cmd, uint16_t param)
{
    switch(cmd) {
    case CMD_IDU_SET_MASK:
        env->arconnect.idu_data[param].mask = env->arconnect.wdata;
        break;
    case CMD_IDU_READ_MASK:
        env->readback = env->arconnect.idu_data[param].mask;
        break;
    case CMD_IDU_SET_DEST:
        env->arconnect.idu_data[param].dest = env->arconnect.wdata & IDU_DEST_MASK;
        break;
    case CMD_IDU_READ_DEST:
        env->readback = env->arconnect.idu_data[param].dest & IDU_DEST_MASK;
        break;
    case CMD_IDU_SET_MODE:
        env->arconnect.idu_data[param].mode = env->arconnect.wdata;
        break;
    case CMD_IDU_READ_MODE:
        env->readback = env->arconnect.idu_data[param].mode;
        break;
    case CMD_IDU_ENABLE:
        env->arconnect.idu_enabled = true;
        break;
    case CMD_IDU_DISABLE:
        env->arconnect.idu_enabled = false;
        memset(env->arconnect.idu_data, 0, sizeof(env->arconnect.idu_data));
        memset(first_acknowledge_status, 0, sizeof(first_acknowledge_status));
        break;
    case CMD_IDU_GEN_CIRQ:
        arc_cirq_raise(env, param);
        break;
    case CMD_IDU_ACK_CIRQ:
        qemu_irq_lower(env->irq[EXT_IRQ + param]);
        break;
    case CMD_IDU_CHECK_FIRST:
        clear_first_acknowledge(env, param);
        break;
    default:
        assert(0);
    }
}

static void arconnect_gfrc_cmd(CPUARCState *env, enum arconnect_commands cmd, uint16_t param)
{
    ARCCPU *cpu = env_archcpu(env);
    static uint64_t offset_timer = 0;
    static bool disabled = false;
    static uint32_t core = 0;

    switch(cmd) {
    case CMD_GFRC_CLEAR:
        qemu_mutex_lock(&gfrc_mutex);
        offset_timer += get_global_cycles();
        qemu_mutex_unlock(&gfrc_mutex);
        break;
    case CMD_GFRC_READ_LO:
        qemu_mutex_lock(&gfrc_mutex);
        env->arconnect.gfrc_snapshots[cpu->core_id] = get_global_cycles() - offset_timer;
        qemu_mutex_unlock(&gfrc_mutex);
        env->readback = (uint32_t) (env->arconnect.gfrc_snapshots[cpu->core_id] & 0xffffffff);
        break;
    case CMD_GFRC_READ_HI:
        env->readback = (uint32_t) ((env->arconnect.gfrc_snapshots[cpu->core_id] >> 32) & 0xffffffff);
        break;
    case CMD_GFRC_ENABLE:
        qemu_mutex_lock(&gfrc_mutex);
        disabled = false;
        qemu_mutex_unlock(&gfrc_mutex);
        break;
    case CMD_GFRC_DISABLE:
        qemu_mutex_lock(&gfrc_mutex);
        disabled = true;
        qemu_mutex_unlock(&gfrc_mutex);
        break;
    case CMD_GFRC_READ_DISABLE:
        qemu_mutex_lock(&gfrc_mutex);
        env->readback = disabled;
        qemu_mutex_unlock(&gfrc_mutex);
        break;
    case CMD_GFRC_SET_CORE:
        qemu_mutex_lock(&gfrc_mutex);
        core = env->arconnect.wdata;
        qemu_mutex_unlock(&gfrc_mutex);
        break;
    case CMD_GFRC_READ_CORE:
        qemu_mutex_lock(&gfrc_mutex);
        env->readback = core;
        qemu_mutex_unlock(&gfrc_mutex);
        break;
    case CMD_GFRC_READ_HALT:
        qemu_mutex_lock(&gfrc_mutex);
        env->readback = (core >> cpu->core_id) & 0x1;
        qemu_mutex_unlock(&gfrc_mutex);
        break;
    default:
        assert(0);
    }
}

static void arconnect_command_process(CPUARCState *env, uint32_t data)
{
    enum arconnect_commands cmd = ARCON_COMMAND(data);
    uint16_t param = ARCON_PARAM(data);
    ARCCPU *cpu = env_archcpu(env);

    qemu_log_mask(CPU_LOG_INT,
                  "[ICI %d] Process command %d with param %d.\n",
                  cpu->core_id, cmd, param);

    switch(ARCON_COMMAND(cmd)) {
    case CMD_CHECK_CORE_ID:
        env->readback = cpu->core_id & 0x1f;
        break;
    case CMD_INTRPT_GENERATE_IRQ:
    case CMD_INTRPT_GENERATE_ACK:
    case CMD_INTRPT_READ_STATUS:
    case CMD_INTRPT_CHECK_SOURCE:
        arconnect_intercore_intr_unit_cmd(env, cmd, param);
        break;
    case CMD_IDU_ENABLE:
    case CMD_IDU_DISABLE:
    case CMD_IDU_READ_ENABLE:
    case CMD_IDU_SET_MODE:
    case CMD_IDU_READ_MODE:
    case CMD_IDU_SET_DEST:
    case CMD_IDU_READ_DEST:
    case CMD_IDU_GEN_CIRQ:
    case CMD_IDU_ACK_CIRQ:
    case CMD_IDU_CHECK_STATUS:
    case CMD_IDU_CHECK_SOURCE:
    case CMD_IDU_SET_MASK:
    case CMD_IDU_READ_MASK:
    case CMD_IDU_CHECK_FIRST:
    case CMD_IDU_SET_PM:
    case CMD_IDU_READ_PSTATUS:
        arconnect_idu_cmd(env, cmd, param);
        break;
    case CMD_GFRC_CLEAR:
    case CMD_GFRC_READ_LO:
    case CMD_GFRC_READ_HI:
    case CMD_GFRC_ENABLE:
    case CMD_GFRC_DISABLE:
    case CMD_GFRC_READ_DISABLE:
    case CMD_GFRC_SET_CORE:
    case CMD_GFRC_READ_CORE:
    case CMD_GFRC_READ_HALT:
        arconnect_gfrc_cmd(env, cmd, param);
        break;
    default:
        assert(0);
    };
}

target_ulong arconnect_regs_get(const struct arc_aux_reg_detail *aux_reg_detail,
                                void *data)
{
    CPUARCState *env = (CPUARCState *) data;
    target_ulong reg = 0;

    switch(aux_reg_detail->id) {
    case AUX_ID_mcip_bcr:
        reg = 0x00800000 /* IDU */
              | 0x00040000 /* Number of cores */
              | (1 << 14); /* GFRC */
        break;
    case AUX_ID_mcip_idu_bcr:
        reg = 0x300;
        break;
    case AUX_ID_mcip_cmd:
        assert(0); /* TODO: raise exception */
        break;
    case AUX_ID_mcip_wdata:
        break;
    case AUX_ID_mcip_readback:
        reg = env->readback;
        break;
    default:
        assert(0);
        break;
    }

    return reg;
}
void arconnect_regs_set(const struct arc_aux_reg_detail *aux_reg_detail,
                        target_ulong val, void *data)
{
    CPUARCState *env = (CPUARCState *) data;
    bool unlocked = !qemu_mutex_iothread_locked();

    if (unlocked) {
        qemu_mutex_lock_iothread();
    }

    switch(aux_reg_detail->id) {
    case AUX_ID_mcip_cmd:
        arconnect_command_process(env, val);
        break;
    case AUX_ID_mcip_wdata:
        env->arconnect.wdata = val;
        break;
    case AUX_ID_mcip_readback:
        assert(0); /* TODO: raise exception */
        break;
    default:
        assert(0);
        break;
    }

    if (unlocked) {
        qemu_mutex_unlock_iothread();
    }
}
