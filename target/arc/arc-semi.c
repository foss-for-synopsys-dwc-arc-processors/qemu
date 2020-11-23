#include "qemu/osdep.h"
#include "cpu.h"
#include "qemu/log.h"
#include "chardev/char-fe.h"
#include "qapi/error.h"
#include "exec/helper-proto.h"
#include "semihosting/semihost.h"

enum {
    TARGET_SYS_exit = 1,
    TARGET_SYS_read = 3,
    TARGET_SYS_write = 4,
    TARGET_SYS_open = 5,
    TARGET_SYS_close = 6,
    TARGET_SYS_unlink = 10,
    TARGET_SYS_time = 13,
    TARGET_SYS_lseek = 19,
    TARGET_SYS_times = 43,
    TARGET_SYS_gettimeofday = 78,
    TARGET_SYS_stat = 106, /* nsim stat's is corupted.  */
    TARGET_SYS_fstat = 108,

    TARGET_SYS_argc = 1000,
    TARGET_SYS_argv_sz = 1001,
    TARGET_SYS_argv = 1002,
    TARGET_SYS_memset = 1004,
};

typedef struct ArcSimConsole {
    CharBackend be;
    struct {
        char buffer[16];
        size_t offset;
    } input;
} ArcSimConsole;

static ArcSimConsole *sim_console;

static IOCanReadHandler sim_console_can_read;

static int sim_console_can_read(void *opaque)
{
    ArcSimConsole *p = opaque;

    return sizeof(p->input.buffer) - p->input.offset;
}

static IOReadHandler sim_console_read;

static void sim_console_read(void *opaque, const uint8_t *buf, int size)
{
    ArcSimConsole *p = opaque;
    size_t copy = sizeof(p->input.buffer) - p->input.offset;

    if (size < copy) {
        copy = size;
    }
    memcpy(p->input.buffer + p->input.offset, buf, copy);
    p->input.offset += copy;
}


void arc_sim_open_console(Chardev *chr)
{
    static ArcSimConsole console;

    qemu_chr_fe_init(&console.be, chr, &error_abort);
    qemu_chr_fe_set_handlers(&console.be,
                             sim_console_can_read,
                             sim_console_read,
                             NULL, NULL, &console,
                             NULL, true);
    sim_console = &console;
}

/*
 * SYSCALL0:
 *  - Input: R8 syscall name;
 *  - Output: R0 return code.
 *
 * SYSCALL1:
 *  -Input: R0 Arg0, R8 syscall name;
 *  -OutputL R0 return code.
 */
void do_arc_semihosting(CPUARCState *env)
{
    CPUState *cs = env_cpu(env);
    target_ulong *regs = env->r;

    switch (regs[8]) {
    case TARGET_SYS_exit:
        exit(regs[0]);
        break;

    case TARGET_SYS_read:
    case TARGET_SYS_write:
        {
            bool is_write = regs[8] == TARGET_SYS_write;
            uint32_t fd = (uint32_t) regs[0];
            target_ulong vaddr = regs[1];
            uint32_t len = (uint32_t) regs[2];
            uint32_t len_done = 0;

            while (len > 0) {
                hwaddr paddr = cpu_get_phys_page_debug(cs, vaddr);
                uint32_t page_left =
                    TARGET_PAGE_SIZE - (vaddr & (TARGET_PAGE_SIZE - 1));
                uint32_t io_sz = page_left < len ? page_left : len;
                hwaddr sz = io_sz;
                void *buf = cpu_physical_memory_map(paddr, &sz, !is_write);
                uint32_t io_done;
                bool error = false;

                if (buf) {
                    vaddr += io_sz;
                    len -= io_sz;
                    if (fd < 3 && sim_console) {
                        if (is_write && (fd == 1 || fd == 2)) {
                            /* stdout, stderr */
                            io_done = qemu_chr_fe_write_all(&sim_console->be,
                                                            buf, io_sz);
                        } else if (!is_write && fd == 0) {
                            /* stdin */
                            if (sim_console->input.offset) {
                                io_done = sim_console->input.offset;
                                if (io_sz < io_done) {
                                    io_done = io_sz;
                                }
                                memcpy(buf, sim_console->input.buffer, io_done);
                                memmove(sim_console->input.buffer,
                                        sim_console->input.buffer + io_done,
                                        sim_console->input.offset - io_done);
                                sim_console->input.offset -= io_done;
                                qemu_chr_fe_accept_input(&sim_console->be);
                            } else {
                                io_done = -1;
                            }
                        } else {
                            qemu_log_mask(LOG_GUEST_ERROR,
                                          "%s fd %d is not supported with chardev console\n",
                                          is_write ?
                                          "writing to" : "reading from", fd);
                            io_done = -1;
                        }
                    } else {
                        io_done = is_write ?
                            write(fd, buf, io_sz) :
                            read(fd, buf, io_sz);
                    }
                    if (io_done == -1) {
                        error = true;
                        io_done = 0;
                    }
                    cpu_physical_memory_unmap(buf, sz, !is_write, io_done);
                } else {
                    error = true;
                    break;
                }
                if (error) {
                    if (!len_done) {
                        len_done = -1;
                    }
                    break;
                }
                len_done += io_done;
                if (io_done < io_sz) {
                    break;
                }
            }
            regs[0] = len_done;
        }
        break;

    case TARGET_SYS_open:
        {
            char name[1024];
            int rc;
            int i;

            for (i = 0; i < ARRAY_SIZE(name); ++i) {
                rc = cpu_memory_rw_debug(cs, regs[0] + i,
                                         (uint8_t *)name + i, 1, 0);
                if (rc != 0 || name[i] == 0) {
                    break;
                }
            }

            if (rc == 0 && i < ARRAY_SIZE(name)) {
                regs[0] = open(name, regs[1], regs[2]);
            } else {
                regs[0] = -1;
            }
        }
        break;

    case TARGET_SYS_close:
        if (regs[0] < 3) {
            /* Ignore attempts to close stdin/out/err. */
            regs[0] = 0;
        } else {
            regs[0] = close(regs[0]);
        }
        break;

    case TARGET_SYS_lseek:
        regs[0] = lseek(regs[0], (off_t)(int32_t)regs[1], regs[2]);
        break;

    case TARGET_SYS_times:
    case TARGET_SYS_time:
        regs[0] = time (NULL);
        break;

    case TARGET_SYS_gettimeofday:
    {
        qemu_timeval tv;
        struct timeval p;
        uint32_t result = qemu_gettimeofday(&tv);
        target_ulong base = regs[0];
        uint32_t sz = sizeof (struct timeval);
        hwaddr len = sz;
        void *buf = cpu_physical_memory_map(base, &len, 1);

        p.tv_sec = tv.tv_sec;
        p.tv_usec = tv.tv_usec;
        if (buf)
        {
            memcpy(buf, &p, sizeof (struct timeval));
            cpu_physical_memory_unmap(buf, len, 1, len);
        }
        regs[0] = result;
        break;
    }

    case TARGET_SYS_fstat:
    {
        struct stat sbuf;
        memset(&sbuf, 0, sizeof(sbuf));
        regs[0] = fstat(regs[0], &sbuf);

        hwaddr len = sizeof(sbuf);
        void *buf = cpu_physical_memory_map(regs[1], &len, 1);
        if (buf)
        {
            memcpy(buf, &sbuf, sizeof (sbuf));
            cpu_physical_memory_unmap(buf, len, 1, len);
        }
        break;
    }

#if 0
    case TARGET_SYS_argc:
        regs[2] = semihosting_get_argc();
        regs[3] = 0;
        break;

    case TARGET_SYS_argv_sz:
        {
            int argc = semihosting_get_argc();
            int sz = (argc + 1) * sizeof(uint32_t);
            int i;

            for (i = 0; i < argc; ++i) {
                sz += 1 + strlen(semihosting_get_arg(i));
            }
            regs[2] = sz;
            regs[3] = 0;
        }
        break;

    case TARGET_SYS_argv:
        {
            int argc = semihosting_get_argc();
            int str_offset = (argc + 1) * sizeof(uint32_t);
            int i;
            uint32_t argptr;

            for (i = 0; i < argc; ++i) {
                const char *str = semihosting_get_arg(i);
                int str_size = strlen(str) + 1;

                argptr = tswap32(regs[3] + str_offset);

                cpu_memory_rw_debug(cs,
                                    regs[3] + i * sizeof(uint32_t),
                                    (uint8_t *)&argptr, sizeof(argptr), 1);
                cpu_memory_rw_debug(cs,
                                    regs[3] + str_offset,
                                    (uint8_t *)str, str_size, 1);
                str_offset += str_size;
            }
            argptr = 0;
            cpu_memory_rw_debug(cs,
                                regs[3] + i * sizeof(uint32_t),
                                (uint8_t *)&argptr, sizeof(argptr), 1);
            regs[3] = 0;
        }
        break;

    case TARGET_SYS_memset:
        {
            uint32_t base = regs[3];
            uint32_t sz = regs[5];

            while (sz) {
                hwaddr len = sz;
                void *buf = cpu_physical_memory_map(base, &len, 1);

                if (buf && len) {
                    memset(buf, regs[4], len);
                    cpu_physical_memory_unmap(buf, len, 1, len);
                } else {
                    len = 1;
                }
                base += len;
                sz -= len;
            }
            regs[2] = regs[3];
            regs[3] = 0;
        }
        break;
#endif

    default:
        qemu_log_mask(LOG_GUEST_ERROR, "%s(%d): not implemented\n", __func__,
                      (uint32_t) regs[8]);
        regs[0] = -1;
        break;
    }
}
