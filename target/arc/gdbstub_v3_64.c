#include "qemu/osdep.h"
#include "exec/gdbstub.h"
#include "gdbstub/helpers.h"
#include "qemu/error-report.h"
#include "cpu.h"
#include "gdbstub.h"

int arc_cpu_gdb_read_register(CPUState *cs, GByteArray *mem_buf, int n)
{
    ARCCPU *cpu = ARC_CPU(cs);
    CPUARCState *env = &cpu->env;
    target_ulong regval = 0;

    switch (n) {
    case 0 ... 31:
        regval = env->gpr[n];
        break;
    case GDB_ARCV3_REG_CORE_R58:
        regval = env->gpr[58];
        break;
    case GDB_ARCV3_REG_CORE_R63:
        regval = env->gpr[63];
        break;
    default:
        g_assert_not_reached();
    }

    return gdb_get_reg64(mem_buf, regval);
}

int arc_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    ARCCPU *cpu = ARC_CPU(cs);
    CPUARCState *env = &cpu->env;
    target_ulong regval = ldq_p(mem_buf);

    switch (n) {
    case 0 ... 31:
        env->gpr[n] = regval;
        break;
    case GDB_ARCV3_REG_CORE_R58:
        env->gpr[58] = regval;
        break;
    default:
        g_assert_not_reached();
    }

    return sizeof(regval);
}

void arc_cpu_register_gdb_regs_for_features(CPUState *cs)
{
}
