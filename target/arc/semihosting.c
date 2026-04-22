#include "qemu/osdep.h"
#include "cpu.h"
#include "qemu/log.h"
#include "semihosting/syscalls.h"
#include "semihosting/semihost.h"

enum {
    TARGET_SYS_exit  = 1,
    TARGET_SYS_read  = 3,
    TARGET_SYS_write = 4,
    TARGET_SYS_open  = 5,
    TARGET_SYS_close = 6,
    TARGET_SYS_unlink = 10,
    TARGET_SYS_lseek = 19,
    TARGET_SYS_fstat = 108
};

/*
 * Callback invoked by the common semihosting layer once the host syscall
 * completes (immediately for native mode, or after a GDB reply).
 * Write the result back into r0.
 */
static void arc_semi_cb(CPUState *cs, uint64_t ret, int err)
{
    CPUARCState *env = cpu_env(cs);

    env->gpr[0] = err ? (uint32_t)-1 : (uint32_t)ret;
}

static const char *semihosting_syscall_to_name(int syscall)
{
    switch (syscall) {
    case TARGET_SYS_exit:
        return "exit(code)";
    case TARGET_SYS_read:
        return "read(fd, buf, len)";
    case TARGET_SYS_write:
        return "write(fd, buf, len)";
    case TARGET_SYS_open:
        return "open(fname, fname_len, flags, mode)";
    case TARGET_SYS_close:
        return "close(fd)";
    case TARGET_SYS_unlink:
        return "unlink(fname)";
    case TARGET_SYS_lseek:
        return "lseek(fd, offset, whence)";
    case TARGET_SYS_fstat:
        return "fstat(fd, addr)";
    default:
        return "<unknown semihosting syscall>";
    }
}

enum {
    ARC_NSIM_O_RDONLY = 0,
    ARC_NSIM_O_WRONLY = 1,
    ARC_NSIM_O_RDWR = 2,
    ARC_NSIM_O_CREAT = 0x0040,
    ARC_NSIM_O_APPEND = 0x0400,
    ARC_NSIM_O_TRUNC = 0x0200,
    ARC_NSIM_O_EXCL = 0x0080
};

static uint32_t nsim_flags_to_gdb_flags(uint32_t nsim_flags)
{
    uint32_t gdb_flags = 0;

    gdb_flags |= (nsim_flags & ARC_NSIM_O_RDONLY) ? GDB_O_RDONLY : 0;
    gdb_flags |= (nsim_flags & ARC_NSIM_O_WRONLY) ? GDB_O_WRONLY : 0;
    gdb_flags |= (nsim_flags & ARC_NSIM_O_RDWR) ? GDB_O_RDWR : 0;
    gdb_flags |= (nsim_flags & ARC_NSIM_O_CREAT) ? GDB_O_CREAT : 0;
    gdb_flags |= (nsim_flags & ARC_NSIM_O_APPEND) ? GDB_O_APPEND : 0;
    gdb_flags |= (nsim_flags & ARC_NSIM_O_TRUNC) ? GDB_O_TRUNC : 0;
    gdb_flags |= (nsim_flags & ARC_NSIM_O_EXCL) ? GDB_O_EXCL : 0;

    return gdb_flags;
}

void do_arc_semihosting(CPUARCState *env)
{
    CPUState *cs = env_cpu(env);
    target_ulong *regs = env->gpr;

    switch (regs[8]) {
    case TARGET_SYS_exit:
        qemu_log_mask(CPU_LOG_INT, "arc: semihosting: exiting through semihosting with exit code " TARGET_FMT_ld "\n", regs[0]);
        gdb_exit(regs[0]);
        exit(regs[0]);
        break;
    case TARGET_SYS_read:
        /* read(fd, buf, len) */
        semihost_sys_read(cs, arc_semi_cb, regs[0], regs[1], regs[2]);
        break;
    case TARGET_SYS_write:
        /* write(fd, buf, len) */
        semihost_sys_write(cs, arc_semi_cb, regs[0], regs[1], regs[2]);
        break;
    case TARGET_SYS_open:
        /* open(fname, fname_len, flags, mode) */
        semihost_sys_open(cs, arc_semi_cb, regs[0], 0, nsim_flags_to_gdb_flags(regs[1]), regs[2]);
        break;
    case TARGET_SYS_close:
        /* close(fd) */
        semihost_sys_close(cs, arc_semi_cb, regs[0]);
        break;
    case TARGET_SYS_unlink:
        /* unlink(fname) */
        semihost_sys_remove(cs, arc_semi_cb, regs[0], 0);
        break;
    case TARGET_SYS_lseek:
        /* lseek(fd, offset, whence) */
        semihost_sys_lseek(cs, arc_semi_cb, regs[0], regs[1], regs[2]);
        break;
    case TARGET_SYS_fstat:
        /* fstat(fd, addr) */
        semihost_sys_fstat(cs, arc_semi_cb, regs[0], regs[1]);
        break;
    default:
        qemu_log_mask(LOG_GUEST_ERROR,
                      "arc: semihosting: unimplemented semihosting syscall r8=" TARGET_FMT_lu "\n", regs[8]);
        regs[0] = (target_ulong)-1;
        break;
    }

    qemu_log_mask(CPU_LOG_INT, "arc: semihosting: semihosting syscall %s at pc=0x" TARGET_FMT_lx "\n", semihosting_syscall_to_name(regs[8]), env->pc);
}
