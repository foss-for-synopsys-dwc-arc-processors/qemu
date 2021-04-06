/*
 *  Emulation of Linux signals
 *
 *  Copyright (c) 2003 Fabrice Bellard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#include "qemu/osdep.h"
#include "qemu.h"
#include "signal-common.h"
#include "linux-user/trace.h"

/* Must be in sync with mcontext_t from sys/ucontext.h. */
struct target_sigcontext {
    abi_ulong pad;
    struct {
        abi_ulong bta;
        abi_ulong lp_start, lp_end, lp_count;
        abi_ulong status32, ret, blink;
        abi_ulong fp, gp;
        abi_ulong r12, r11, r10, r9, r8, r7;
        abi_ulong r6, r5, r4, r3, r2, r1, r0;
        abi_ulong sp;
    } scratch;
    abi_ulong pad2;
    struct {
        abi_ulong r25, r24, r23, r22, r21, r20;
        abi_ulong r19, r18, r17, r16, r15, r14, r13;
    } callee;
    abi_ulong efa;
    abi_ulong stop_pc;
    struct {
        abi_ulong r30, r58, r59;
    } scratch2;
};

/* Must be in sync with ucontext_t from sys/ucontext.h. */
struct target_ucontext {
    abi_ulong                uc_flags;
    abi_ptr                  uc_link; /* struct ucontext_t * */
    target_stack_t           uc_stack;
    struct target_sigcontext uc_mcontext;
    target_sigset_t          uc_sigmask;
};

struct target_rt_sigframe {
    struct target_siginfo info;
    struct target_ucontext uc;

#define MAGIC_SIGALTSTK    0x07302004
    uint32_t  sigret_magic;
};

/* Determine which stack to use. */
static abi_ulong get_sigframe(struct target_sigaction *ka,
                              CPUARCState *env,
                              size_t framesize)
{
    abi_ulong sp = target_sigsp(CPU_SP(env), ka);

    /* No matter what happens, 'sp' must be word
     * aligned otherwise nasty things could happen
     */
    sp = ((sp - framesize) & ~7);

    return sp;
}

static int
stash_usr_regs(struct target_rt_sigframe *sf, CPUARCState *env,
               target_sigset_t *set)
{
    target_ulong status32;
    struct target_ucontext *uc = &(sf->uc);
    struct target_sigcontext *sc = &(uc->uc_mcontext);

    status32 = pack_status32(&(env->stat));
    __put_user(status32, &(sc->scratch.status32));

    __put_user(env->bta, &(sc->scratch.bta));
    __put_user(env->lps, &(sc->scratch.lp_start));
    __put_user(env->lpe, &(sc->scratch.lp_end));
    __put_user(CPU_LP(env), &(sc->scratch.lp_count));
    __put_user(env->pc, &(sc->scratch.ret));
    __put_user(CPU_BLINK(env), &(sc->scratch.blink));
    __put_user(CPU_FP(env), &(sc->scratch.fp));
    __put_user(CPU_GP(env), &(sc->scratch.gp));
    __put_user(CPU_SP(env), &(sc->scratch.sp));
    __put_user(env->r[0], &(sc->scratch.r0));
    __put_user(env->r[1], &(sc->scratch.r1));
    __put_user(env->r[2], &(sc->scratch.r2));
    __put_user(env->r[3], &(sc->scratch.r3));
    __put_user(env->r[4], &(sc->scratch.r4));
    __put_user(env->r[5], &(sc->scratch.r5));
    __put_user(env->r[6], &(sc->scratch.r6));
    __put_user(env->r[7], &(sc->scratch.r7));
    __put_user(env->r[8], &(sc->scratch.r8));
    __put_user(env->r[9], &(sc->scratch.r9));
    __put_user(env->r[10], &(sc->scratch.r10));
    __put_user(env->r[11], &(sc->scratch.r11));
    __put_user(env->r[12], &(sc->scratch.r12));

    __put_user(env->r[30], &(sc->scratch2.r30));
    __put_user(env->r[58], &(sc->scratch2.r58));
    __put_user(env->r[59], &(sc->scratch2.r59));

    int i;
    for (i = 0; i < TARGET_NSIG_WORDS; i++) {
        __put_user(set->sig[i], &(uc->uc_sigmask.sig[i]));
    }

    return 0;
}

static int
restore_usr_regs(CPUARCState *env, struct target_rt_sigframe *sf)
{
    target_ulong status32;
    sigset_t blocked;
    target_sigset_t target_set;
    int i;
    struct target_ucontext *uc = &(sf->uc);
    struct target_sigcontext *sc = &(uc->uc_mcontext);

    __get_user(status32, &(sc->scratch.status32));
    unpack_status32(&(env->stat), status32);

    __get_user(env->bta, &(sc->scratch.bta));
    __get_user(env->lps, &(sc->scratch.lp_start));
    __get_user(env->lpe, &(sc->scratch.lp_end));
    __get_user(CPU_LP(env), &(sc->scratch.lp_count));
    __get_user(env->pc, &(sc->scratch.ret));
    CPU_PCL(env) = env->pc & 0xfffffffe;
    __get_user(CPU_BLINK(env), &(sc->scratch.blink));
    __get_user(CPU_FP(env), &(sc->scratch.fp));
    __get_user(CPU_GP(env), &(sc->scratch.gp));
    __get_user(CPU_SP(env), &(sc->scratch.sp));
    __get_user(env->r[0], &(sc->scratch.r0));
    __get_user(env->r[1], &(sc->scratch.r1));
    __get_user(env->r[2], &(sc->scratch.r2));
    __get_user(env->r[3], &(sc->scratch.r3));
    __get_user(env->r[4], &(sc->scratch.r4));
    __get_user(env->r[5], &(sc->scratch.r5));
    __get_user(env->r[6], &(sc->scratch.r6));
    __get_user(env->r[7], &(sc->scratch.r7));
    __get_user(env->r[8], &(sc->scratch.r8));
    __get_user(env->r[9], &(sc->scratch.r9));
    __get_user(env->r[10], &(sc->scratch.r10));
    __get_user(env->r[11], &(sc->scratch.r11));
    __get_user(env->r[12], &(sc->scratch.r12));

    __get_user(env->r[30], &(sc->scratch2.r30));
    __get_user(env->r[58], &(sc->scratch2.r58));
    __get_user(env->r[59], &(sc->scratch2.r59));

    /* Unlock blocked signals.  */
    target_sigemptyset(&target_set);
    for (i = 0; i < TARGET_NSIG_WORDS; i++) {
        __get_user(target_set.sig[i], &(uc->uc_sigmask.sig[i]));
    }

    target_to_host_sigset_internal(&blocked, &target_set);
    set_sigmask(&blocked);

    return 0;
}

void setup_rt_frame(int sig, struct target_sigaction *ka,
                    target_siginfo_t *info,
                    target_sigset_t *set, CPUARCState *env)
{
    struct target_rt_sigframe *sf;
    abi_ulong sf_addr;
    uint32_t magic = 0;

    sf_addr = get_sigframe(ka, env, sizeof(*sf));

    trace_user_setup_rt_frame(env, sf_addr);

    if (!lock_user_struct(VERIFY_WRITE, sf, sf_addr, 0))
        goto badfrm;

    stash_usr_regs(sf, env, set);

    if (ka->sa_flags & TARGET_SA_SIGINFO) {
        tswap_siginfo(&sf->info, info);
        __put_user(0, &sf->uc.uc_flags);
        __put_user(0, &sf->uc.uc_link);
        target_save_altstack(&sf->uc.uc_stack, env);

        /* setup args 2 and 3 for user mode handler */
        env->r[1] = sf_addr + offsetof(struct target_rt_sigframe, info);
        env->r[2] = sf_addr + offsetof(struct target_rt_sigframe, uc);

        /*
        * small optim to avoid unconditionally calling do_sigaltstack
        * in sigreturn path, now that we only have rt_sigreturn
        */
        magic = MAGIC_SIGALTSTK;
    }
    __put_user(magic, &sf->sigret_magic);

    env->r[0] = sig;

    env->pc = ka->_sa_handler;
    CPU_PCL(env) = env->pc & (~((target_ulong) 3));

    CPU_SP(env) = sf_addr;

    SET_STATUS_BIT(env->stat, DEf, 0);

    /* libc must provide restorer for us. */
    if (!(ka->sa_flags & TARGET_SA_RESTORER)) {
        goto badfrm;
    }

    CPU_BLINK(env) = ka->sa_restorer;

    unlock_user_struct(sf, sf_addr, 1);
    return;
badfrm:
    unlock_user_struct(sf, sf_addr, 1);
    force_sigsegv(sig);
}

long do_rt_sigreturn(CPUARCState *env)
{
    abi_ulong sf_addr;
    struct target_rt_sigframe *sf;
    unsigned int magic;

    sf_addr = CPU_SP(env);

    trace_user_do_rt_sigreturn(env, sf_addr);

    if (sf_addr & 7)
        goto badfrm;

    if (!lock_user_struct(VERIFY_READ, sf, sf_addr, 1))
        goto badfrm;

    __get_user(magic, &sf->sigret_magic);

    if (MAGIC_SIGALTSTK == magic) {
        if (do_sigaltstack(sf_addr +
                           offsetof(struct target_rt_sigframe, uc.uc_stack),
                           0,
                           get_sp_from_cpustate(env)) == -TARGET_EFAULT) {
            goto badfrm;
        }
    }

    restore_usr_regs(env, sf);

    unlock_user_struct(sf, sf_addr, 0);

    return -TARGET_QEMU_ESIGRETURN;
badfrm:
    unlock_user_struct(sf, sf_addr, 0);
    force_sig(TARGET_SIGSEGV);

    return 0;
}
