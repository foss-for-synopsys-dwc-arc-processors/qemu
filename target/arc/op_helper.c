/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2020 Synopsys Inc.
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

#include "qemu/osdep.h"
#include "qemu/error-report.h"
#include "cpu.h"
#include "sysemu/runstate.h"
#include "exec/helper-proto.h"
#include "exec/cpu_ldst.h"
#include "exec/ioport.h"
#include "target/arc/regs.h"
#include "mmu.h"
#include "hw/arc/cpudevs.h"
#include "qemu/main-loop.h"
#include "irq.h"
#include "sysemu/sysemu.h"
#include "exec/exec-all.h"
#include "target/arc/arconnect.h"


static target_ulong get_status32(CPUARCState *env)
{
    target_ulong value = pack_status32(&env->stat);

    /* TODO: Implement debug mode */
    if (GET_STATUS_BIT(env->stat, Uf) == 1) {
        value &= 0x00000f00;
    }

    if (env->stopped) {
        value |= BIT(0);
    }

    return value;
}

static void set_status32(CPUARCState *env, target_ulong value)
{
    /* TODO: Implement debug mode. */
    bool debug_mode = false;
    if (GET_STATUS_BIT(env->stat, Uf) == 1) {
        value &= 0x00000f00;
    } else if (!debug_mode) {
        value &= 0xffff6f3f;
    }

    if (GET_STATUS_BIT(env->stat, Uf) != ((value >> 7)  & 0x1)) {
        tlb_flush(env_cpu(env));
    }

    unpack_status32(&env->stat, value);

#ifndef CONFIG_USER_ONLY
    /* Implement HALT functionality.  */
    if (value & 0x01) {
        qemu_system_shutdown_request(SHUTDOWN_CAUSE_GUEST_SHUTDOWN);
    }
#endif
}

target_ulong helper_llock(CPUARCState *env, target_ulong addr)
{
    assert((addr & 0x3) == 0);
    hwaddr haddr;
    CPUState *cs = env_cpu(env);
    arc_get_physical_addr(cs, &haddr, addr, MMU_MEM_READ, false, GETPC());
    struct lpa_lf_entry *entry = &lpa_lfs[LPA_LFS_ENTRY_FOR_PA(haddr)];
    qemu_log_mask(LOG_UNIMP, "0x" TARGET_FMT_lx "LLOCK at addr 0x" TARGET_FMT_lx " at index %d\n",
                  env->pc,
                  (target_ulong) haddr, (int) LPA_LFS_ENTRY_FOR_PA(haddr));
    qemu_mutex_lock(&entry->mutex);
    env->arconnect.locked_mutex = &entry->mutex;
    target_ulong ret = cpu_ldl_data(env, addr);
    env->arconnect.lpa_lf = entry;
    entry->lpa_lf = (haddr & (~LPA_LFS_ALIGNEMENT_MASK));
    entry->lpa_lf += 1;     /* least significant bit is LF flag */
    entry->read_value = ret;
    qemu_mutex_unlock(&entry->mutex);
    return ret;
}
target_ulong helper_scond(CPUARCState *env, target_ulong addr, target_ulong value)
{
    assert((addr & 0x3) == 0);
    hwaddr haddr;
    CPUState *cs = env_cpu(env);
    arc_get_physical_addr(cs, &haddr, addr, MMU_MEM_WRITE, false, GETPC());
    qemu_log_mask(LOG_UNIMP, "0x" TARGET_FMT_lx "SCOND at addr 0x" TARGET_FMT_lx " at index %d\n",
                  env->pc,
                  (target_ulong) haddr, (int) LPA_LFS_ENTRY_FOR_PA(haddr));
    qemu_mutex_lock(env->arconnect.locked_mutex);
    struct lpa_lf_entry *entry = env->arconnect.lpa_lf;
    haddr = (haddr & (~LPA_LFS_ALIGNEMENT_MASK));
    target_ulong ret = 1;
    target_ulong rvalue = cpu_ldl_data(env, addr);
    if(entry->lpa_lf == (haddr + 1) && (entry->read_value == rvalue)) {
        ret  = 0;
        cpu_stl_data(env, addr, value);
        entry->lpa_lf = 0;
        qemu_log_mask(LOG_UNIMP, "SCOND success with value 0x" TARGET_FMT_lx "\n", value);
    }
    else
        {
          qemu_log_mask(LOG_UNIMP, "SCOND fail with value 0x" TARGET_FMT_lx "\n", value);

        }
    qemu_mutex_unlock(&entry->mutex);
    return ret;
}

#if defined(TARGET_ARCV3)
target_ulong helper_llockl(CPUARCState *env, target_ulong addr)
{
    assert((addr & 0x3) == 0);
    target_ulong haddr;
    CPUState *cs = env_cpu(env);
    arc_get_physical_addr(cs, &haddr, addr, MMU_MEM_READ, false, GETPC());
    struct lpa_lf_entry *entry = &lpa_lfs[LPA_LFS_ENTRY_FOR_PA(haddr)];
    qemu_log_mask(LOG_UNIMP, "0x" TARGET_FMT_lx "LLOCKL at addr 0x" TARGET_FMT_lx " at index %d\n",
                  env->pc,
                  (target_ulong) haddr, (int) LPA_LFS_ENTRY_FOR_PA(haddr));
    qemu_mutex_lock(&entry->mutex);
    env->arconnect.locked_mutex = &entry->mutex;
    target_ulong ret = cpu_ldq_data(env, addr);
    env->arconnect.lpa_lf = entry;
    entry->lpa_lf = (haddr & (~LPA_LFS_ALIGNEMENT_MASK));
    entry->lpa_lf += 1;     /* least significant bit is LF flag */
    entry->read_value = ret;
    qemu_mutex_unlock(&entry->mutex);
    return ret;
}
target_ulong helper_scondl(CPUARCState *env, target_ulong addr, target_ulong value)
{
    assert((addr & 0x3) == 0);
    target_ulong haddr;
    CPUState *cs = env_cpu(env);
    arc_get_physical_addr(cs, &haddr, addr, MMU_MEM_WRITE, false, GETPC());
    qemu_log_mask(LOG_UNIMP, "0x" TARGET_FMT_lx "SCONDL at addr 0x" TARGET_FMT_lx " at index %d\n",
                  env->pc,
                  (target_ulong) haddr, (int) LPA_LFS_ENTRY_FOR_PA(haddr));
    struct lpa_lf_entry *entry = env->arconnect.lpa_lf;
    qemu_mutex_lock(env->arconnect.locked_mutex);
    target_ulong ret = 1;
    haddr = (haddr & (~LPA_LFS_ALIGNEMENT_MASK));
    target_ulong rvalue = cpu_ldq_data(env, addr);
    if(entry->lpa_lf == (haddr + 1) && (entry->read_value == rvalue)) {
        ret = 0;
        cpu_stq_data(env, addr, value);
        entry->lpa_lf = 0;
        qemu_log_mask(LOG_UNIMP, "SCONDL success\n");
    }
    qemu_mutex_unlock(&entry->mutex);
    return ret;
}

#elif defined(TARGET_ARCV2)

uint64_t helper_llockd(CPUARCState *env, target_ulong addr)
{
    assert((addr & 0x7) == 0);
    hwaddr haddr;
    CPUState *cs = env_cpu(env);
    arc_get_physical_addr(cs, &haddr, addr, MMU_MEM_READ, false, GETPC());
    struct lpa_lf_entry *entry = &lpa_lfs[LPA_LFS_ENTRY_FOR_PA(haddr)];
    qemu_mutex_lock(&entry->mutex);
    env->arconnect.locked_mutex = &entry->mutex;
    target_ulong read1 = cpu_ldl_data(env, addr);
    target_ulong read2 = cpu_ldl_data(env, addr+4);
    target_ulong ret = ((uint64_t ) read1) | (((uint64_t) read2) << 32);
    env->arconnect.lpa_lf = entry;
    entry->lpa_lf = (haddr & (~LPA_LFS_ALIGNEMENT_MASK));
    entry->lpa_lf += 1;     /* least significant bit is LF flag */
    entry->read_value = read1;
    entry->read_value1 = read2;
    qemu_mutex_unlock(&entry->mutex);
    return ret;
}
target_ulong helper_scondd(CPUARCState *env, target_ulong addr, uint64_t value)
{
    assert((addr & 0x7) == 0);
    hwaddr haddr;
    CPUState *cs = env_cpu(env);
    arc_get_physical_addr(cs, &haddr, addr, MMU_MEM_WRITE, false, GETPC());
    struct lpa_lf_entry *entry = env->arconnect.lpa_lf;
    qemu_mutex_lock(env->arconnect.locked_mutex);
    target_ulong ret = !(entry->lpa_lf & 0x1);
    addr = (addr & (~LPA_LFS_ALIGNEMENT_MASK));
    target_ulong rvalue = cpu_ldq_data(env, addr);
    target_ulong rvalue1 = cpu_ldq_data(env, addr+4);
    if(entry->lpa_lf == (haddr + 1)
       && entry->read_value == rvalue
       && entry->read_value1 == rvalue1
       ) {
        cpu_stl_data(env, addr, (value & 0xffffffff));
        cpu_stl_data(env, addr+4, (value >> 32));
        entry->lpa_lf = 0;
    }
    qemu_mutex_unlock(&entry->mutex);
    return ret;
}
#endif

static void report_aux_reg_error(target_ulong aux)
{
    if (((aux >= ARC_BCR1_START) && (aux <= ARC_BCR1_END)) ||
        ((aux >= ARC_BCR2_START) && (aux <= ARC_BCR2_END))) {
        qemu_log_mask(LOG_UNIMP, "Undefined BCR 0x" TARGET_FMT_lx "\n", aux);
    }

    qemu_log_mask(LOG_UNIMP, "Undefined aux register with id 0x" TARGET_FMT_lx
                  "\n", aux);
}

void helper_sr(CPUARCState *env, target_ulong val, target_ulong aux)
{
    /* saving return address in case an exception must be raised later */
    ARCCPU *cpu = env_archcpu(env);
    struct arc_aux_reg_detail *aux_reg_detail =
        arc_aux_reg_struct_for_address(aux, cpu->family);

    g_assert(aux_reg_detail != NULL);
    if (aux_reg_detail == NULL) {
        report_aux_reg_error(aux);
        arc_raise_exception(env, GETPC(), EXCP_INST_ERROR);
    }

    if (aux_reg_detail->aux_reg->set_func != NULL) {
        aux_reg_detail->aux_reg->set_func(aux_reg_detail, val,
                                          (void *) env);
    } else {
        qemu_log_mask(LOG_UNIMP, "Undefined set_func for aux register with id 0x" TARGET_FMT_lx
                      "\n", aux);
        arc_raise_exception(env, GETPC(), EXCP_INST_ERROR);
    }
#ifndef CONFIG_USER_ONLY
    cpu_outl(aux, val);
#endif
}


target_ulong helper_lr(CPUARCState *env, target_ulong aux)
{
    ARCCPU *cpu = env_archcpu(env);
    target_ulong result = 0;

    /* saving return address in case an exception must be raised later */
    struct arc_aux_reg_detail *aux_reg_detail =
        arc_aux_reg_struct_for_address(aux, cpu->family);

    if (aux_reg_detail == NULL) {
        report_aux_reg_error(aux);
        arc_raise_exception(env, GETPC(), EXCP_INST_ERROR);
    }

    if (aux_reg_detail->aux_reg->get_func != NULL) {
        result = aux_reg_detail->aux_reg->get_func(aux_reg_detail,
                                                   (void *) env);
    } else {
        qemu_log_mask(LOG_UNIMP, "Undefined get_func for aux register with id 0x" TARGET_FMT_lx
                      "\n", aux);
        arc_raise_exception(env, GETPC(), EXCP_INST_ERROR);
    }

    return result;
}

void QEMU_NORETURN helper_halt(CPUARCState *env, target_ulong npc)
{
    CPUState *cs = env_cpu(env);
    if (GET_STATUS_BIT(env->stat, Uf)) {
        cs->exception_index = EXCP_PRIVILEGEV;
        env->causecode = 0;
        env->param = 0;
         /* Restore PC such that we point at the faulty instruction.  */
        env->eret = env->pc;
    } else {
        env->pc = npc;
        cs->halted = 1;
        cs->exception_index = EXCP_HLT;
    }
    cpu_loop_exit(cs);
}

void helper_rtie(CPUARCState *env)
{
    CPUState *cs = env_cpu(env);
    if (GET_STATUS_BIT(env->stat, Uf)) {
        cs->exception_index = EXCP_PRIVILEGEV;
        env->causecode = 0;
        env->param = 0;
         /* Restore PC such that we point at the faulty instruction.  */
        env->eret = env->pc;
        cpu_loop_exit(cs);
        return;
    }

    if (GET_STATUS_BIT(env->stat, AEf) || (env->aux_irq_act & 0xFFFF) == 0) {
        assert(GET_STATUS_BIT(env->stat, Uf) == 0);

        CPU_PCL(env) = env->eret;
        env->pc = env->eret;

        env->stat = env->stat_er;
        env->bta = env->erbta;

        /* If returning to userland, restore SP.  */
        if (GET_STATUS_BIT(env->stat, Uf)) {
            switchSP(env);
        }

        qemu_log_mask(CPU_LOG_INT, "[EXCP] RTIE @0x" TARGET_FMT_lx
                      " ECR:0x" TARGET_FMT_lx " AE: true\n",
                      (target_ulong) env->r[63], (target_ulong) env->ecr);
    } else {
        arc_rtie_interrupts(env);
        qemu_log_mask(CPU_LOG_INT, "[IRQ] RTIE @0x" TARGET_FMT_lx
                      " STATUS32:0x" TARGET_FMT_lx " AE: false\n",
                      (target_ulong) env->r[63],
                      (target_ulong) pack_status32(&env->stat));
    }

#ifdef TARGET_ARCV2
    helper_zol_verify(env, env->pc);
#endif
}

void helper_flush(CPUARCState *env)
{
    tb_flush((CPUState *) env_cpu(env));
}

/*
 * This should only be called from translate, via gen_raise_exception.
 * We expect that ENV->PC has already been updated.
 */

void QEMU_NORETURN helper_raise_exception(CPUARCState *env,
                                          target_ulong index,
                                          target_ulong causecode,
                                          target_ulong param)
{
    CPUState *cs = env_cpu(env);
    cs->exception_index = index;
    env->causecode = causecode;
    env->param = param;
    cpu_loop_exit_restore(cs, GETPC());
}

void helper_zol_verify(CPUARCState *env, target_ulong npc)
{
    CPUState *cs = env_cpu(env);
    if (npc == env->lpe) {
        if (env->r[60] > 1) {
            env->r[60] -= 1;

            /*
             * Raise exception in case where Zero-overhead-loops needs
             * to jump.
             */
            cs->exception_index = EXCP_LPEND_REACHED;
            env->causecode = 0;
            env->param = env->lps;
            cpu_loop_exit(cs);
        } else {
            env->r[60] = 0;
        }
    }
}
void helper_fake_exception(CPUARCState *env, target_ulong pc)
{
    helper_raise_exception(env, (target_ulong) EXCP_FAKE, 0, pc);
}

target_ulong helper_get_status32(CPUARCState *env)
{
    return get_status32(env);
}

void helper_set_status32(CPUARCState *env, target_ulong value)
{
    set_status32(env, value);
}

void helper_set_status32_bit(CPUARCState *env, target_ulong bit,
                             target_ulong value)
{
    target_ulong bit_mask = (1 << bit);
    /* Verify i changing bit is in pstate. Assert otherwise. */
    assert((bit_mask & PSTATE_MASK) == 0);

    env->stat.pstate &= ~bit_mask;
    env->stat.pstate |= (value << bit);
}

static inline target_ulong
carry_add_flag(target_ulong dest, target_ulong b, target_ulong c, uint8_t size)
{
    target_ulong t1, t2, t3;

    t1 = b & c;
    t2 = b & (~dest);
    t3 = c & (~dest);
    t1 = t1 | t2 | t3;
    return (t1 >> (size - 1)) & 1;
}

target_ulong helper_carry_add_flag(target_ulong dest, target_ulong b,
                                   target_ulong c) {
    return carry_add_flag(dest, b, c, TARGET_LONG_BITS);
}

static inline target_ulong
overflow_add_flag(target_ulong dest, target_ulong b, target_ulong c,
                  uint8_t size)
{
    dest >>= (size - 1);
    b >>= (size - 1);
    c >>= (size - 1);
    if ((dest == 0 && b == 1 && c == 1)
        || (dest == 1 && b == 0 && c == 0)) {
        return 1;
    } else {
        return 0;
    }
}
target_ulong helper_overflow_add_flag(target_ulong dest, target_ulong b,
                                      target_ulong c) {
    return overflow_add_flag(dest, b, c, TARGET_LONG_BITS);
}

static inline target_ulong
overflow_sub_flag(target_ulong dest, target_ulong b, target_ulong c,
                  uint8_t size)
{
    dest >>= (size - 1);
    b >>= (size - 1);
    c >>= (size - 1);
    if ((dest == 1 && b == 0 && c == 1)
        || (dest == 0 && b == 1 && c == 0)) {
        return 1;
    } else {
        return 0;
    }
}
target_ulong helper_overflow_sub_flag(target_ulong dest, target_ulong b,
                                      target_ulong c) {
    return overflow_sub_flag(dest, b, c, TARGET_LONG_BITS);
}

target_ulong helper_repl_mask(target_ulong dest, target_ulong src,
                              target_ulong mask)
{
    target_ulong ret = dest & (~mask);
    ret |= (src & mask);

    return ret;
}

target_ulong helper_mpymu(CPUARCState *env, target_ulong b, target_ulong c)
{
    uint64_t _b = (uint64_t) (b & 0xffffffff);
    uint64_t _c = (uint64_t) (c & 0xffffffff);

    return (uint32_t) ((_b * _c) >> 32);
}

target_ulong helper_mpym(CPUARCState *env, target_ulong b, target_ulong c)
{
    int64_t _b = (int64_t) ((int32_t) b);
    int64_t _c = (int64_t) ((int32_t) c);

    /*
     * fprintf(stderr, "B = 0x%llx, C = 0x%llx, result = 0x%llx\n",
     *         _b, _c, _b * _c);
     */
    return (_b * _c) >> 32;
}

target_ulong
arc_status_regs_get(const struct arc_aux_reg_detail *aux_reg_detail,
                    void *data)
{
    CPUARCState *env = (CPUARCState *) data;
    target_ulong reg = 0;

    switch (aux_reg_detail->id) {
    case AUX_ID_status32:
        reg = get_status32(env);
        break;

    case AUX_ID_erstatus:
        if (is_user_mode(env)) {
            arc_raise_exception(env, GETPC(), EXCP_PRIVILEGEV);
        }
        reg = pack_status32(&env->stat_er);
        break;

    default:
        break;
    }

    return reg;
}

void
arc_status_regs_set(const struct arc_aux_reg_detail *aux_reg_detail,
                    target_ulong val, void *data)
{
    CPUARCState *env = (CPUARCState *) data;

    switch (aux_reg_detail->id) {

    case AUX_ID_status32:
        set_status32(env, val);
        break;

    case AUX_ID_erstatus:
        unpack_status32(&env->stat_er, val);
        break;

    default:
        break;
    }
}

#ifdef TARGET_ARCV3
uint64_t helper_carry_add_flag32(uint64_t dest, uint64_t b, uint64_t c) {
    return carry_add_flag(dest, b, c, 32);
}

target_ulong helper_overflow_add_flag32(target_ulong dest, target_ulong b, target_ulong c) {
    return overflow_add_flag(dest, b, c, 32);
}

target_ulong helper_overflow_sub_flag32(target_ulong dest, target_ulong b, target_ulong c) {
    dest = dest & 0xffffffff;
    b = b & 0xffffffff;
    c = c & 0xffffffff;
    return overflow_sub_flag(dest, b, c, 32);
}

uint64_t helper_carry_sub_flag32(uint64_t dest, uint64_t b, uint64_t c)
{
    uint32_t t1, t2, t3;

    t1 = ~b;
    t2 = t1 & c;
    t3 = (t1 | c) & dest;

    t2 = t2 | t3;
    return (t2 >> 31) & 1;
}

uint64_t helper_rotate_left32(uint64_t orig, uint64_t n)
{
    uint64_t t;
    uint64_t dest = (orig << n) & ((0xffffffff << n) & 0xffffffff);

    t = (orig >> (32 - n)) & ((1 << n) - 1);
    dest |= t;

    return dest;
}

uint64_t helper_rotate_right32(uint64_t orig, uint64_t n)
{
    uint64_t t;
    uint64_t dest = (orig >> n) & (0xffffffff >> n);

    t = ((orig & ((1 << n) - 1)) << (32 - n));
    dest |= t;

    return dest;
}

uint64_t helper_asr_32(uint64_t b, uint64_t c)
{
  uint64_t t;
  c = c & 31;
  t = b;
  for(int i = 0; i < c; i++) {
    t >>= 1;
    if((b & 0x80000000) != 0)
      t |= 0x80000000;
  }
      //t |= ((1 << (c+1)) - 1) << (32 - c);

  return t;
}

target_ulong helper_ffs32(CPUARCState *env, uint64_t src)
{
    int i;
    if (src == 0) {
      return 31;
    }
    for (i = 0; i <= 31; i++) {
      if (((src >> i) & 1) != 0) {
        break;
      }
    }
    return i;
}

target_ulong helper_norml(CPUARCState *env, uint64_t src1)
{
    int i;
    int64_t tmp = (int64_t) src1;
    if (tmp == 0 || tmp == -1) {
      return 0;
    }
    for (i = 0; i <= 63; i++) {
      if ((tmp >> i) == 0) {
          break;
      }
      if ((tmp >> i) == -1) {
          break;
      }
    }
    return i;
}
#endif

/*-*-indent-tabs-mode:nil;tab-width:4;indent-line-function:'insert-tab'-*-*/
/* vim: set ts=4 sw=4 et: */
