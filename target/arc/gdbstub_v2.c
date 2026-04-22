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
    case GDB_ARCV2_REG_CORE_R58:
        regval = env->gpr[58];
        break;
    case GDB_ARCV2_REG_CORE_R59:
        regval = env->gpr[59];
        break;
    case GDB_ARCV2_REG_CORE_R60:
        regval = env->gpr[60];
        break;
    case GDB_ARCV2_REG_CORE_R63:
        regval = env->gpr[63];
        break;
    default:
        g_assert_not_reached();
    }

    return gdb_get_reg32(mem_buf, regval);
}

int arc_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    ARCCPU *cpu = ARC_CPU(cs);
    CPUARCState *env = &cpu->env;
    target_ulong regval = ldl_p(mem_buf);

    switch (n) {
    case 0 ... 31:
        env->gpr[n] = regval;
        break;
    case GDB_ARCV2_REG_CORE_R58:
        env->gpr[58] = regval;
        break;
    case GDB_ARCV2_REG_CORE_R59:
        env->gpr[59] = regval;
        break;
    case GDB_ARCV2_REG_CORE_R60:
        env->gpr[60] = regval;
        break;
    case GDB_ARCV2_REG_CORE_R63:
        env->gpr[63] = regval;
        break;
    default:
        g_assert_not_reached();
    }

    return sizeof(regval);
}

static int arc_cpu_gdb_read_aux(CPUState *cs, GByteArray *mem_buf, int regnum)
{
    ARCCPU *cpu = ARC_CPU(cs);
    CPUARCState *env = &cpu->env;
    target_ulong regval = 0;

    switch (regnum) {
    case GDB_ARCV2_REG_AUX_PC:
        regval = env->pc;
        break;
    case GDB_ARCV2_REG_AUX_LP_START:
        regval = env->aux_lp_start;
        break;
    case GDB_ARCV2_REG_AUX_LP_END:
        regval = env->aux_lp_end;
        break;
    case GDB_ARCV2_REG_AUX_STATUS32:
        regval = arc_pack_status32(&env->aux_status32);
        break;
    case GDB_ARCV2_REG_AUX_BTA:
        regval = env->aux_bta;
        break;
    case GDB_ARCV2_REG_AUX_ERSTATUS:
        regval = arc_pack_aux_erstatus(&env->aux_erstatus);
        break;
    case GDB_ARCV2_REG_AUX_ERBTA:
        regval = env->aux_erbta;
        break;
    case GDB_ARCV2_REG_AUX_ECR:
        regval = arc_pack_aux_ecr(&env->aux_ecr);
        break;
    case GDB_ARCV2_REG_AUX_ERET:
        regval = env->aux_eret;
        break;
    case GDB_ARCV2_REG_AUX_EFA:
        regval = env->aux_efa;
        break;
    default:
        error_report("ARC: GDB: unsupported AUX register: " TARGET_FMT_lx "\n", regnum);
        exit(EXIT_FAILURE);
    }

    return gdb_get_reg32(mem_buf, regval);
}

static int arc_cpu_gdb_write_aux(CPUState *cs, uint8_t *mem_buf, int regnum)
{
    return 4;
}

void arc_cpu_register_gdb_regs_for_features(CPUState *cs)
{
    gdb_register_coprocessor(cs, arc_cpu_gdb_read_aux, arc_cpu_gdb_write_aux, gdb_find_static_feature(GDB_ARCV2_REG_AUX_XML), 0);
}
