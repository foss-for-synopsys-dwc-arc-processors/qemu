/*
 *  qemu user cpu loop
 *
 *  Copyright (c) 2003-2008 Fabrice Bellard
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
#include "qemu-common.h"
#include "qemu/error-report.h"
#include "qemu.h"
#include "cpu_loop-common.h"
#include "elf.h"
#include "semihosting/common-semi.h"

void cpu_loop(CPUARCState *env)
{
    CPUState *cs = env_cpu(env);
    int trapnr, signum, sigcode;
    target_ulong sigaddr;
    target_ulong ret;

    for (;;) {
        cpu_exec_start(cs);
        trapnr = cpu_exec(cs);
        cpu_exec_end(cs);
        process_queued_cpu_work(cs);

        signum = 0;
        sigcode = 0;
        sigaddr = 0;

        switch (trapnr) {
          /*
    EXCP_NO_EXCEPTION = -1,
    EXCP_RESET = 0,
    EXCP_MEMORY_ERROR,
    EXCP_INST_ERROR,
    EXCP_MACHINE_CHECK,
#ifdef TARGET_ARCV2
    EXCP_TLB_MISS_I,
    EXCP_TLB_MISS_D,
#elif TARGET_ARCV3
    EXCP_IMMU_FAULT,
    EXCP_DMMU_FAULT,
#else
    #error "TARGET macro not defined!"
#endif
    EXCP_PROTV,
    EXCP_PRIVILEGEV,
    EXCP_SWI,
    EXCP_TRAP,
    EXCP_EXTENSION,
    EXCP_DIVZERO,
    EXCP_DCERROR,
    EXCP_MISALIGNED,
    EXCP_IRQ,
    EXCP_LPEND_REACHED = 9000,
    EXCP_FAKE
    */
        case EXCP_LPEND_REACHED:
        case EXCP_FAKE:
            env->pc = env->param;
            CPU_PCL(env) = env->pc & (~1);  
            break;
        case EXCP_TLB_MISS_I:
        case EXCP_TLB_MISS_D:
            signum = TARGET_SIGSEGV;
            sigcode = TARGET_SEGV_MAPERR;
            sigaddr = env->efa;
            break;
        case EXCP_INTERRUPT:
            /* just indicate that signals should be handled asap */
            break;
        case EXCP_ATOMIC:
            cpu_exec_step_atomic(cs);
            break;
        case EXCP_TRAP:
            env->pc += 2;
            //if (env->r[8] == TARGET_NR_arch_specific_syscall + 15) {
            //    /* riscv_flush_icache_syscall is a no-op in QEMU as
            //       self-modifying code is automatically detected */
            //    ret = 0;
            //} else {
            target_ulong syscall_num = env->r[8];

            switch(syscall_num) {
                case TARGET_NR_arc_settls:
                    env->tls_backup = env->r[0];
                    env->r[0] = 0;
                    break;
                case TARGET_NR_arc_gettls:
                    env->r[0] = env->tls_backup;
                    break;
                case TARGET_NR_arc_cacheflush:
                    break;
                case TARGET_NR_arc_sysfs:
                    break;
                case TARGET_NR_arc_usr_cmpxchg:
                    break;
                default: 
                    {
                        ret = do_syscall(env,
                                         env->r[8],
                                         env->r[0],
                                         env->r[1],
                                         env->r[2],
                                         env->r[3],
                                         env->r[4],
                                         env->r[5],
                                         env->r[6],
                                         env->r[7]);
                        //}
                        if (ret == -TARGET_ERESTARTSYS) {
                            env->pc -= 2;
                        } else if (ret != -TARGET_QEMU_ESIGRETURN) {
                            env->r[0] = ret;
                        }
                        CPU_PCL(env) = env->pc & (~1);  
                        if (cs->singlestep_enabled) {
                            goto gdbstep;
                        }
                    }
                    break;
            }
            break;
//        case RISCV_EXCP_ILLEGAL_INST:
//            signum = TARGET_SIGILL;
//            sigcode = TARGET_ILL_ILLOPC;
//            break;
//        case ARC_EXCP_BREAKPOINT:
//            signum = TARGET_SIGTRAP;
//            sigcode = TARGET_TRAP_BRKPT;
//            sigaddr = env->pc;
//            break;
//        case ARC_EXCP_INST_PAGE_FAULT:
//        case ARC_EXCP_LOAD_PAGE_FAULT:
//        case ARC_EXCP_STORE_PAGE_FAULT:
//            signum = TARGET_SIGSEGV;
//            sigcode = TARGET_SEGV_MAPERR;
//            sigaddr = env->badaddr;
//            break;
//        //case ARC_EXCP_SEMIHOST:
//        //    env->gpr[xA0] = do_common_semihosting(cs);
//        //    env->pc += 4;
//        //    break;
        case EXCP_DEBUG:
        gdbstep:
            signum = TARGET_SIGTRAP;
            sigcode = TARGET_TRAP_BRKPT;
            break;
        default:
            EXCP_DUMP(env, "\nqemu: unhandled CPU exception %#x - aborting\n",
                     trapnr);
            exit(EXIT_FAILURE);
        }

        if (signum) {
            target_siginfo_t info = {
                .si_signo = signum,
                .si_errno = 0,
                .si_code = sigcode,
                ._sifields._sigfault._addr = sigaddr
            };
            queue_signal(env, info.si_signo, QEMU_SI_FAULT, &info);
        }

        process_pending_signals(env);
    }
}

void target_cpu_copy_regs(CPUArchState *env, struct target_pt_regs *regs)
{
    //CPUState *cpu = env_cpu(env);
    //TaskState *ts = cpu->opaque;
    //struct image_info *info = ts->info;

    env->pc = regs->sepc;
    CPU_SP(env) = regs->sp;
    //env->r[8] = info->elf_flags;

//    if ((env->misa & RVE) && !(env->elf_flags & EF_ARC_RVE)) {
//        error_report("Incompatible ELF: ARC cpu requires ARC ABI binary");
//        exit(EXIT_FAILURE);
//    }
}
