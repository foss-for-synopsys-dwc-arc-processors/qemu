/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2020 Synppsys Inc.
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
#include "translate.h"
#include "qemu/qemu-print.h"
#include "tcg/tcg-op-gvec.h"
#include "target/arc/semfunc.h"
#include "target/arc/arc-common.h"

/* Globals */
TCGv    cpu_S1f;
TCGv    cpu_S2f;
TCGv    cpu_CSf;

TCGv    cpu_pstate;
TCGv    cpu_Vf;
TCGv    cpu_Cf;
TCGv    cpu_Nf;
TCGv    cpu_Zf;

TCGv    cpu_is_delay_slot_instruction;

TCGv    cpu_er_pstate;
TCGv    cpu_er_Vf;
TCGv    cpu_er_Cf;
TCGv    cpu_er_Nf;
TCGv    cpu_er_Zf;

TCGv    cpu_eret;
TCGv    cpu_erbta;
TCGv    cpu_efa;

TCGv    cpu_bta;

TCGv    cpu_pc;
/* replaced by AUX_REG array */
TCGv    cpu_lps;
TCGv    cpu_lpe;

TCGv    cpu_r[64];

TCGv    cpu_intvec;

TCGv    cpu_lock_lf_var;

/* NOTE: Pseudo register required for comparison with lp_end */
TCGv    cpu_npc;

/* Macros */

#include "exec/gen-icount.h"
#define REG(x)  (cpu_r[x])

/* macro used to fix middle-endianess. */
#define ARRANGE_ENDIAN(endianess, buf)                  \
    ((endianess) ? ror32(buf, 16) : bswap32(buf))

static inline bool use_goto_tb(const DisasContext *dc, target_ulong dest)
{
    if (unlikely(dc->base.singlestep_enabled)) {
        return false;
    }
#ifndef CONFIG_USER_ONLY
    return (dc->base.tb->pc & TARGET_PAGE_MASK) == (dest & TARGET_PAGE_MASK);
#else
    return true;
#endif
}

void gen_goto_tb(const DisasContext *ctx, int n, TCGv dest)
{
    tcg_gen_mov_tl(cpu_pc, dest);
    tcg_gen_andi_tl(cpu_pcl, dest, ~((target_ulong) 3));
    if (ctx->base.singlestep_enabled) {
        gen_helper_debug(cpu_env);
    } else {
        tcg_gen_exit_tb(NULL, 0);
    }
}

static void gen_gotoi_tb(const DisasContext *ctx, int n, target_ulong dest)
{
    if (use_goto_tb(ctx, dest)) {
        tcg_gen_goto_tb(n);
        tcg_gen_movi_tl(cpu_pc, dest);
        tcg_gen_movi_tl(cpu_pcl, dest & (~((target_ulong) 3)));
        tcg_gen_exit_tb(ctx->base.tb, n);
    } else {
        tcg_gen_movi_tl(cpu_pc, dest);
        tcg_gen_movi_tl(cpu_pcl, dest & (~((target_ulong) 3)));
        if (ctx->base.singlestep_enabled) {
            gen_helper_debug(cpu_env);
        }
        tcg_gen_exit_tb(NULL, 0);
    }
}

void arc_translate_init(void)
{
    int i;
#define ARC_REG_OFFS(x) offsetof(CPUARCState, x)

#define NEW_ARC_REG(TCGV, FIELD) \
        { &TCGV, offsetof(CPUARCState, FIELD), #FIELD },

    static const struct { TCGv *ptr; int off; const char *name; } r32[] = {
        NEW_ARC_REG(cpu_S1f, macmod.S1)
        NEW_ARC_REG(cpu_S2f, macmod.S2)
        NEW_ARC_REG(cpu_CSf, macmod.CS)

        NEW_ARC_REG(cpu_pstate, stat.pstate)
        NEW_ARC_REG(cpu_Zf, stat.Zf)
        NEW_ARC_REG(cpu_Nf, stat.Nf)
        NEW_ARC_REG(cpu_Cf, stat.Cf)
        NEW_ARC_REG(cpu_Vf, stat.Vf)

        NEW_ARC_REG(cpu_pstate, stat.pstate)
        NEW_ARC_REG(cpu_er_Zf, stat_er.Zf)
        NEW_ARC_REG(cpu_er_Nf, stat_er.Nf)
        NEW_ARC_REG(cpu_er_Cf, stat_er.Cf)
        NEW_ARC_REG(cpu_er_Vf, stat_er.Vf)

        NEW_ARC_REG(cpu_eret, eret)
        NEW_ARC_REG(cpu_erbta, erbta)
        NEW_ARC_REG(cpu_efa, efa)
        NEW_ARC_REG(cpu_bta, bta)
        NEW_ARC_REG(cpu_lps, lps)
        NEW_ARC_REG(cpu_lpe, lpe)
        NEW_ARC_REG(cpu_pc , pc)
        NEW_ARC_REG(cpu_npc, npc)

        NEW_ARC_REG(cpu_intvec, intvec)

        NEW_ARC_REG(cpu_is_delay_slot_instruction,
                    stat.is_delay_slot_instruction)

        NEW_ARC_REG(cpu_lock_lf_var, lock_lf_var)
    };


    for (i = 0; i < ARRAY_SIZE(r32); ++i) {
        *r32[i].ptr = tcg_global_mem_new(cpu_env, r32[i].off, r32[i].name);
    }


    for (i = 0; i < 64; i++) {
        char name[16];

        sprintf(name, "r[%d]", i);
        cpu_r[i] = tcg_global_mem_new(cpu_env,
                                      ARC_REG_OFFS(r[i]),
                                      strdup(name));
    }

#undef ARC_REG_OFFS
#undef NEW_ARC_REG
}

static void arc_tr_init_disas_context(DisasContextBase *dcbase,
                                      CPUState *cs)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    dc->base.is_jmp = DISAS_NEXT;
    dc->mem_idx = dc->base.tb->flags & 1;
    dc->in_delay_slot = false;
}
static void arc_tr_tb_start(DisasContextBase *dcbase, CPUState *cpu)
{
    /* place holder for now */
}

static void arc_tr_insn_start(DisasContextBase *dcbase, CPUState *cpu)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);


    tcg_gen_insn_start(dc->base.pc_next);
    dc->cpc = dc->base.pc_next;

    if (dc->base.num_insns == dc->base.max_insns &&
        (dc->base.tb->cflags & CF_LAST_IO)) {
        gen_io_start();
    }
}

static bool arc_tr_breakpoint_check(DisasContextBase *dcbase, CPUState *cpu,
                                    const CPUBreakpoint *bp)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    tcg_gen_movi_tl(cpu_pc, dc->cpc);
    dc->base.is_jmp = DISAS_NORETURN;
    gen_helper_debug(cpu_env);
    dc->base.pc_next += 2;
    return true;
}

static int arc_gen_INVALID(const DisasContext *ctx)
{
    qemu_log_mask(LOG_UNIMP,
                  "invalid inst @:%08x\n", ctx->cpc);
    arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
    return DISAS_NORETURN;
}

/*
 * Giving a CTX, decode it into an valid OPCODE_P if it
 * exists. Returns TRUE if successfully.
 */
static bool read_and_decode_context(DisasContext *ctx,
                                    const struct arc_opcode **opcode_p)
{
    uint16_t buffer[2];
    uint8_t length;
    uint64_t insn;
    ARCCPU *cpu = env_archcpu(ctx->env);

    /* Read the first 16 bits, figure it out what kind of instruction it is. */
    buffer[0] = cpu_lduw_code(ctx->env, ctx->cpc);
    length = arc_insn_length(buffer[0], cpu->family);

    switch (length) {
    case 2:
        /* 16-bit instructions. */
        insn = (uint64_t) buffer[0];
        break;
    case 4:
        /* 32-bit instructions. */
        buffer[1] = cpu_lduw_code(ctx->env, ctx->cpc + 2);
        uint32_t buf = (buffer[0] << 16) | buffer[1];
        insn = buf;
        break;
    default:
        g_assert_not_reached();
    }

    /*
     * Now, we have read the entire opcode, decode it and place the
     * relevant info into opcode and ctx->insn.
     */
    *opcode_p = arc_find_format(&ctx->insn, insn, length, cpu->family);

    if (*opcode_p == NULL) {
        return false;
    }

    /*
     * If the instruction requires long immediate, read the extra 4
     * bytes and initialize the relevant fields.
     */
    if (ctx->insn.limm_p) {
        ctx->insn.limm = ARRANGE_ENDIAN(true,
                                        cpu_ldl_code(ctx->env,
                                        ctx->cpc + length));
        length += 4;
#ifdef TARGET_ARCV3
    } else if(ctx->insn.signed_limm_p) {
        ctx->insn.limm = ARRANGE_ENDIAN(true,
                                        cpu_ldl_code (ctx->env,
                                        ctx->cpc + length));
        if(ctx->insn.limm & 0x80000000)
          ctx->insn.limm += 0xffffffff00000000;
        length += 4;
#endif
    } else {
        ctx->insn.limm = 0;
    }

    /* Update context. */
    ctx->insn.len = length;
    ctx->npc = ctx->cpc + length;
    ctx->pcl = ctx->cpc & (~((target_ulong) 3));

    return true;
}

/* Check if OPR is a register _and_ an odd numbered one. */
static inline bool is_odd_numbered_register(const operand_t opr)
{
   return (opr.type & ARC_OPERAND_IR) && (opr.value & 1);
}

enum arc_opcode_map {
    MAP_NONE = -1,
#define SEMANTIC_FUNCTION(...)
#define CONSTANT(...)
#define MAPPING(MNEMONIC, NAME, NOPS, ...) MAP_##MNEMONIC##_##NAME,
#include "target/arc/semfunc_mapping.def"
#include "target/arc/extra_mapping.def"
#undef MAPPING
#undef CONSTANT
#undef SEMANTIC_FUNCTION
    /* Add some include to generated files */
    MAP_LAST
};

const char number_of_ops_semfunc[MAP_LAST + 1] = {
#define SEMANTIC_FUNCTION(...)
#define CONSTANT(...)
#define MAPPING(MNEMONIC, NAME, NOPS, ...) NOPS,
#include "target/arc/semfunc_mapping.def"
#include "target/arc/extra_mapping.def"
#undef MAPPING
#undef CONSTANT
#undef SEMANTIC_FUNCTION
    2
};

static enum arc_opcode_map arc_map_opcode(const struct arc_opcode *opcode)
{
#define SEMANTIC_FUNCTION(...)
#define CONSTANT(...)
#define MAPPING(MNEMONIC, NAME, ...)         \
    if (strcmp(opcode->name, #MNEMONIC) == 0) \
        return MAP_##MNEMONIC##_##NAME;
#include "target/arc/semfunc_mapping.def"
#include "target/arc/extra_mapping.def"
#undef MAPPING
#undef CONSTANT
#undef SEMANTIC_FUNCTION

    return MAP_NONE;
}

/* Code support for constant values coming from semantic function mapping. */
struct constant_operands {
    uint8_t operand_number;
    uint32_t default_value;
    struct constant_operands *next;
};

struct constant_operands *map_constant_operands[MAP_LAST];

static void add_constant_operand(enum arc_opcode_map mapping,
                                 uint8_t operand_number,
                                 uint32_t value)
{
    struct constant_operands **t = &(map_constant_operands[mapping]);
    while (*t != NULL) {
        t = &((*t)->next);
    }
    *t = (struct constant_operands *) g_new(struct constant_operands, 1);

    (*t)->operand_number = operand_number;
    (*t)->default_value = value;
    (*t)->next = NULL;
}

static struct constant_operands *
constant_entry_for(enum arc_opcode_map mapping,
                   uint8_t operand_number)
{
    struct constant_operands *t = map_constant_operands[mapping];
    while (t != NULL) {
        if (t->operand_number == operand_number) {
            return t;
        }
        t = t->next;
    }
    return NULL;
}

static void init_constants(void)
{
#define SEMANTIC_FUNCTION(...)
#define MAPPING(...)
#define CONSTANT(NAME, MNEMONIC, OP_NUM, VALUE) \
  add_constant_operand(MAP_##MNEMONIC##_##NAME, OP_NUM, VALUE);
#include "target/arc/semfunc_mapping.def"
#include "target/arc/extra_mapping.def"
#undef MAPPING
#undef CONSTANT
#undef SEMANTIC_FUNCTION
}

static void arc_debug_opcode(const struct arc_opcode *opcode,
                             DisasContext *ctx,
                             const char *msg)
{
    qemu_log_mask(LOG_UNIMP,
                  "%s for %s at pc=0x%08x\n",
                  msg, opcode->name, ctx->cpc);
}

static TCGv arc_decode_operand(const struct arc_opcode *opcode,
                               DisasContext *ctx,
                               unsigned char nop,
                               enum arc_opcode_map mapping)
{
    TCGv ret;

    if (nop >= ctx->insn.n_ops) {
        struct constant_operands *co = constant_entry_for(mapping, nop);
        assert(co != NULL);
        ret = tcg_const_local_tl(co->default_value);
        return ret;
    } else {
        operand_t operand = ctx->insn.operands[nop];

        if (operand.type & ARC_OPERAND_IR) {
            ret = cpu_r[operand.value];
            if (operand.value == 63) {
                tcg_gen_movi_tl(cpu_pcl, ctx->pcl);
            }
        } else {
            int64_t limm = operand.value;
            if (operand.type & ARC_OPERAND_LIMM) {
                limm = ctx->insn.limm;
                tcg_gen_movi_tl(cpu_limm, limm);
                ret = cpu_r[62];
            } else {
                ret = tcg_const_local_tl(limm);
            }
        }
      }

  return ret;
}

/* See translate.h. */
void arc_gen_excp(const DisasCtxt *ctx,
                  uint32_t index,
                  uint32_t causecode,
                  uint32_t param)
{
    TCGv tcg_index = tcg_const_tl(index);
    TCGv tcg_cause = tcg_const_tl(causecode);
    TCGv tcg_param = tcg_const_tl(param);

    tcg_gen_movi_tl(cpu_pc, ctx->cpc);
    tcg_gen_movi_tl(cpu_eret, ctx->cpc);
    tcg_gen_movi_tl(cpu_erbta, ctx->npc);

    gen_helper_raise_exception(cpu_env, tcg_index, tcg_cause, tcg_param);

    tcg_temp_free(tcg_index);
    tcg_temp_free(tcg_cause);
    tcg_temp_free(tcg_param);
}

/* Wrapper around tcg_gen_exit_tb that handles single stepping */
static void exit_tb(const DisasContext *ctx)
{
    if (ctx->base.singlestep_enabled) {
        gen_helper_debug(cpu_env);
    } else {
        tcg_gen_exit_tb(NULL, 0);
    }
}

/*
 * throw "illegal instruction" exception if more than available
 * registers are asked to be saved/restore.
 */
static bool check_enter_leave_nr_regs(const DisasCtxt *ctx,
                                      uint8_t      regs)
{
    const uint8_t rgf_num_regs = env_archcpu(ctx->env)->cfg.rgf_num_regs;
    if ((rgf_num_regs == 32 && regs > 14) ||
        (rgf_num_regs == 16 && regs >  3)) {

        TCGv tcg_index = tcg_const_tl(EXCP_INST_ERROR);
        TCGv tcg_cause = tcg_const_tl(0);
        TCGv tcg_param = tcg_const_tl(0);

        tcg_gen_movi_tl(cpu_eret, ctx->cpc);
        tcg_gen_movi_tl(cpu_erbta, ctx->npc);

        gen_helper_raise_exception(cpu_env, tcg_index, tcg_cause, tcg_param);

        tcg_temp_free(tcg_index);
        tcg_temp_free(tcg_cause);
        tcg_temp_free(tcg_param);
        return false;
    }
    return true;
}

/*
 * throw "illegal instruction sequence" exception if we are in a
 * delay/execution slot.
 */
static bool check_delay_or_execution_slot(const DisasCtxt *ctx)
{
    if (ctx->in_delay_slot) {
        TCGv tcg_index = tcg_const_tl(EXCP_INST_ERROR);
        TCGv tcg_cause = tcg_const_tl(0x1);
        TCGv tcg_param = tcg_const_tl(0x0);

        tcg_gen_mov_tl(cpu_eret, cpu_pc);
        tcg_gen_mov_tl(cpu_erbta, cpu_bta);

        gen_helper_raise_exception(cpu_env, tcg_index, tcg_cause, tcg_param);

        tcg_temp_free(tcg_index);
        tcg_temp_free(tcg_cause);
        tcg_temp_free(tcg_param);
        return false;
    }
    return true;
}

/*
 * Throw "misaligned" exception if 'addr' is not 32-bit aligned.
 * This check is done irrelevant of status32.AD bit.
 */
static void check_addr_is_word_aligned(const DisasCtxt *ctx,
                                       TCGv addr)
{
    TCGLabel *l1 = gen_new_label();
    TCGv tmp = tcg_temp_local_new();

    tcg_gen_andi_tl(tmp, addr, 0x3);
    tcg_gen_brcondi_tl(TCG_COND_EQ, tmp, 0, l1);

    tcg_gen_mov_tl(cpu_efa, addr);
    tcg_gen_movi_tl(cpu_eret, ctx->cpc);
    tcg_gen_mov_tl(cpu_erbta, cpu_bta);

    TCGv tcg_index = tcg_const_tl(EXCP_MISALIGNED);
    TCGv tcg_cause = tcg_const_tl(0x0);
    TCGv tcg_param = tcg_const_tl(0x0);

    gen_helper_raise_exception(cpu_env, tcg_index, tcg_cause, tcg_param);

    gen_set_label(l1);

    tcg_temp_free(tcg_index);
    tcg_temp_free(tcg_cause);
    tcg_temp_free(tcg_param);
    tcg_temp_free(tmp);
}



/*
 * enter_s instruction.
 * after we are done, stack layout would be:
 * ,- top -.
 * | blink |
 * | r13   |
 * | r14   |
 * | ...   |
 * | r26   |
 * | fp    |
 * `-------'
 */
int arc_gen_ENTER(DisasContext *ctx)
{
    int ret = DISAS_NEXT;
    uint32_t u6 = ctx->insn.operands[0].value;

    if (!u6) {
        return ret;
    }

    uint8_t regs       = u6 & 0x0f; /* u[3:0] determines registers to save */
    bool    save_fp    = u6 & 0x10; /* u[4] indicates if fp must be saved  */
    bool    save_blink = u6 & 0x20; /* u[5] indicates saving of blink      */
    uint8_t stack_size = 4 * (regs + save_fp + save_blink);

    /* number of regs to be saved must be sane */
    if (!check_enter_leave_nr_regs(ctx, regs)) {
        return ret;
    }

    /* this cannot be executed in a delay/execution slot */
    if (!check_delay_or_execution_slot(ctx)) {
        return ret;
    }

    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_sp = tcg_temp_local_new();

    /* stack must be a multiple of 4 (32 bit aligned) */
    tcg_gen_subi_tl(temp_1, cpu_sp, stack_size);
    check_addr_is_word_aligned(ctx, temp_1);

    /*
     * Backup SP. SP should only be written in the end of the execution to
     * allow to correctly recover from exceptions the might happen in the
     * middle of the instruction execution.
     */
    tcg_gen_mov_tl(temp_sp, cpu_sp);

    if (save_fp) {
        tcg_gen_subi_tl(temp_sp, temp_sp, 4);
        tcg_gen_qemu_st_tl(cpu_fp, temp_sp, ctx->mem_idx, MO_UL);
    }

    for (uint8_t gpr = regs; gpr >= 1; --gpr) {
        tcg_gen_subi_tl(temp_sp, temp_sp, 4);
        tcg_gen_qemu_st_tl(cpu_r[13 + gpr - 1], temp_sp, ctx->mem_idx, MO_UL);
    }

    if (save_blink) {
        tcg_gen_subi_tl(temp_sp, temp_sp, 4);
        tcg_gen_qemu_st_tl(cpu_blink, temp_sp, ctx->mem_idx, MO_UL);
    }

    tcg_gen_mov_tl(cpu_sp, temp_sp);

    /* now that sp has been allocated, shall we write it to fp? */
    if (save_fp) {
        tcg_gen_mov_tl(cpu_fp, cpu_sp);
    }

    tcg_temp_free(temp_sp);
    tcg_temp_free(temp_1);

    return ret;
}

/*
 * helper for leave_s instruction.
 * a stack layout of below is assumed:
 * ,- top -.
 * | blink |
 * | r13   |
 * | r14   |
 * | ...   |
 * | r26   |
 * | fp    |
 * `-------'
 */
int arc_gen_LEAVE(DisasContext *ctx)
{
    int ret = DISAS_NEXT;
    uint32_t u7 = ctx->insn.operands[0].value;

    /* nothing to do? then bye-bye! */
    if (!u7) {
        return ret;
    }

    uint8_t regs       = u7 & 0x0f; /* u[3:0] determines registers to save */
    bool restore_fp    = u7 & 0x10; /* u[4] indicates if fp must be saved  */
    bool restore_blink = u7 & 0x20; /* u[5] indicates saving of blink      */
    bool jump_to_blink = u7 & 0x40; /* u[6] should we jump to blink?       */

    /* number of regs to be saved must be sane */
    if (!check_enter_leave_nr_regs(ctx, regs)) {
        return ret;
    }

    /* this cannot be executed in a delay/execution slot */
    if (!check_delay_or_execution_slot(ctx)) {
        return ret;
    }

    TCGv temp_1 = tcg_temp_local_new();
    /*
     * stack must be a multiple of 4 (32 bit aligned). we must take into
     * account if sp is going to use fp's value or not.
     */
    if (restore_fp) {
        tcg_gen_mov_tl(temp_1, cpu_fp);
    } else {
        tcg_gen_mov_tl(temp_1, cpu_sp);
    }
    check_addr_is_word_aligned(ctx, temp_1);

    TCGv temp_sp = tcg_temp_local_new();
    /*
     * if fp is in the picture, then first we have to use the current
     * fp as the stack pointer for restoring.
     */
    if (restore_fp) {
        tcg_gen_mov_tl(cpu_sp, cpu_fp);
    }

    tcg_gen_mov_tl(temp_sp, cpu_sp);

    if (restore_blink) {
        tcg_gen_qemu_ld_tl(cpu_blink, temp_sp, ctx->mem_idx, MO_UL);
        tcg_gen_addi_tl(temp_sp, temp_sp, 4);
    }

    for (uint8_t gpr = 0; gpr < regs; ++gpr) {
        tcg_gen_qemu_ld_tl(cpu_r[13 + gpr], temp_sp, ctx->mem_idx, MO_UL);
        tcg_gen_addi_tl(temp_sp, temp_sp, 4);
    }

    if (restore_fp) {
        tcg_gen_qemu_ld_tl(cpu_fp, temp_sp, ctx->mem_idx, MO_UL);
        tcg_gen_addi_tl(temp_sp, temp_sp, 4);
    }

    tcg_gen_mov_tl(cpu_sp, temp_sp);

    /* now that we are done, should we jump to blink? */
    if (jump_to_blink) {
        gen_goto_tb(ctx, 1, cpu_blink);
        ret = DISAS_NORETURN;
    }

    tcg_temp_free(temp_sp);
    tcg_temp_free(temp_1);

    return ret;
}

/* SR tcg translator */
int
arc_gen_SR(DisasCtxt *ctx, TCGv src2, TCGv src1)
{
    int ret = DISAS_NEXT;

#if defined(TARGET_ARCV2)
    writeAuxReg(src2, src1);
#elif defined(TARGET_ARCV3)
    TCGv temp = tcg_temp_local_new();
    tcg_gen_andi_tl(temp, src1, 0xffffffff);
    writeAuxReg(src2, src1);
    tcg_temp_free(temp);
#endif
    return ret;
}
int
arc_gen_SRL(DisasCtxt *ctx, TCGv src2, TCGv src1)
{
    int ret = DISAS_NEXT;

    writeAuxReg(src2, src1);
    return ret;
}

/* SYNC tcg translator */
int
arc_gen_SYNC(DisasCtxt *ctx)
{
    int ret = DISAS_NEXT;

    syncReturnDisasUpdate();
    return ret;
}


#ifdef TARGET_ARCV3
/*
 * The mpyl instruction is a 64x64 signed multipler that produces
 * a 64-bit product (the lower 64-bit of the actual prodcut).
 */
int
arc_gen_MPYL(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    if ((getFFlag () == true)) {
        arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
        return DISAS_NEXT;
    }

    TCGLabel *done = gen_new_label();

    if (ctx->insn.cc) {
        TCGv cc = tcg_temp_local_new();
        arc_gen_verifyCCFlag(ctx, cc);
        tcg_gen_brcondi_tl(TCG_COND_NE, cc, 1, done);
        tcg_temp_free(cc);
    }

    TCGv_i64 lo = tcg_temp_local_new_i64();
    TCGv_i64 hi = tcg_temp_local_new_i64();

    tcg_gen_muls2_i64(lo, hi, b, c);
    tcg_gen_mov_tl(a, lo);

    tcg_temp_free_i64(hi);
    tcg_temp_free_i64(lo);
    gen_set_label(done);

    return DISAS_NEXT;
}

/*
 * The mpyml instruction is a 64x64 signed multipler that produces
 * a 64-bit product (the higher 64-bit of the actual prodcut).
 */
int
arc_gen_MPYML(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    if ((getFFlag () == true)) {
        arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
        return DISAS_NEXT;
    }

    TCGLabel *done = gen_new_label();

    if (ctx->insn.cc) {
        TCGv cc = tcg_temp_local_new();
        arc_gen_verifyCCFlag(ctx, cc);
        tcg_gen_brcondi_tl(TCG_COND_NE, cc, 1, done);
        tcg_temp_free(cc);
    }

    TCGv lo = tcg_temp_local_new();
    TCGv hi = tcg_temp_local_new();
    tcg_gen_muls2_i64(lo, hi, b, c);
    tcg_gen_mov_tl(a, hi);

    tcg_temp_free(hi);
    tcg_temp_free(lo);
    gen_set_label(done);

    return DISAS_NEXT;
}

/*
 * The mpymul instruction is a 64x64 unsigned multipler that produces
 * a 64-bit product (the higher 64-bit of the actual prodcut).
 */
int
arc_gen_MPYMUL(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    if ((getFFlag () == true)) {
        arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
        return DISAS_NEXT;
    }

    TCGLabel *done = gen_new_label();

    if (ctx->insn.cc) {
        TCGv cc = tcg_temp_local_new();
        arc_gen_verifyCCFlag(ctx, cc);
        tcg_gen_brcondi_tl(TCG_COND_NE, cc, 1, done);
        tcg_temp_free(cc);
    }

    TCGv lo = tcg_temp_local_new();
    TCGv hi = tcg_temp_local_new();

    tcg_gen_mulu2_i64(lo, hi, b, c);
    tcg_gen_mov_tl(a, hi);

    tcg_temp_free(hi);
    tcg_temp_free(lo);
    gen_set_label(done);

    return DISAS_NEXT;
}

/*
 * The mpymsul instruction is a 64x64 signedxunsigned multipler that
 * produces * a 64-bit product (the higher 64-bit of the actual prodcut).
 */
int
arc_gen_MPYMSUL(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    if ((getFFlag () == true)) {
        arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
        return DISAS_NEXT;
    }

    TCGLabel *done = gen_new_label();

    if (ctx->insn.cc) {
        TCGv cc = tcg_temp_local_new();
        arc_gen_verifyCCFlag(ctx, cc);
        tcg_gen_brcondi_tl(TCG_COND_NE, cc, 1, done);
        tcg_temp_free(cc);
    }

    TCGv lo = tcg_temp_local_new();
    TCGv hi = tcg_temp_local_new();
    tcg_gen_mulsu2_tl(lo, hi, b, c);
    tcg_gen_mov_tl(a, hi);

    tcg_temp_free(hi);
    tcg_temp_free(lo);
    gen_set_label(done);

    return DISAS_NEXT;
}

/*
 * a = b + (c << 32)
 */
int
arc_gen_ADDHL(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    TCGLabel *done = gen_new_label();

    if (ctx->insn.cc) {
        TCGv cc = tcg_temp_local_new();
        arc_gen_verifyCCFlag(ctx, cc);
        tcg_gen_brcondi_tl(TCG_COND_NE, cc, 1, done);
        tcg_temp_free(cc);
    }

    TCGv shifted = tcg_temp_local_new();
    tcg_gen_shli_tl(shifted, c, 32);
    tcg_gen_add_tl(a, b, shifted);

    tcg_temp_free(shifted);
    gen_set_label(done);

    return DISAS_NEXT;
}

/*
 * 128-bit load.
 * FIXME: There is a mixture of decoder stuffs in here.
 *        The logic must be moved to a decoding layer.
 */
int
arc_gen_LDDL(DisasCtxt *ctx, TCGv base, TCGv offset, TCGv dest_lo)
{
    TCGv dest_hi = NULL;

    /*
     * The destiantion can be a register or immediate (0).
     * Look for next register pair if "dest" is a register.
     * Will raise an exception if "dest" is an odd register.
     */
    if (ctx->insn.operands[0].type & ARC_OPERAND_IR) {
        dest_hi = nextReg(dest_lo);
        if (dest_hi == NULL) {
            return DISAS_NORETURN;
        }
    }

    /*
     * Address writebacks for 128-bit loads.
     * ,----.----------.
     * | aa | mnemonic |
     * |----+----------|
     * | 0  | none     |
     * | 1  | as       |
     * | 2  | a/aw     |
     * | 3  | ab       |
     * `----^----------'
     */
    /* A non-register operand cannot be incremented. */
    if (ctx->insn.aa == 2 || ctx->insn.aa == 3)
    {
        if (!(ctx->insn.operands[1].type & ARC_OPERAND_IR)) {
            arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
            return DISAS_NORETURN;
      }
    }

    /* Only defined after possible exception routine codes. */
    TCGv addr = tcg_temp_local_new();
    TCGv data_hi = tcg_temp_local_new();
    TCGv data_lo = tcg_temp_local_new();

    switch (ctx->insn.aa) {
    case 0:  /* Simple base+offset access. */
        tcg_gen_add_tl(addr, base, offset);
        break;
    case 1:  /* Address scaling. */
        tcg_gen_shli_tl(offset, offset, 3);
        tcg_gen_add_tl(addr, base, offset);
        break;
    case 2:  /* Pre memory access increment. */
        tcg_gen_add_tl(addr, base, offset);
        tcg_gen_mov_tl(base, addr);
        break;
    case 3:  /* Post memory access increment. */
        tcg_gen_mov_tl(addr, base);
        break;
    default:
        g_assert_not_reached();
    }

    /* Load the data. */
    if (ctx->insn.operands[0].type & ARC_OPERAND_IR) {
        tcg_gen_qemu_ld_tl(data_lo, addr, ctx->mem_idx, MO_Q);
        tcg_gen_addi_tl(addr, addr, 8);
        tcg_gen_qemu_ld_tl(data_hi, addr, ctx->mem_idx, MO_Q);
    }

    /*
     * If "dest" and "base" are pointing to the same register,
     * the register will end up with the loaded data; not the
     * updated pointer. Therefore the "base" must be updated
     * before the "dest".
     */
    if (ctx->insn.aa == 3) { /* Post-memory access increment. */
        tcg_gen_add_tl(addr, base, offset);
        tcg_gen_mov_tl(base, addr);
    }

    if (ctx->insn.operands[0].type & ARC_OPERAND_IR) {
        tcg_gen_mov_tl(dest_lo, data_lo);
        tcg_gen_mov_tl(dest_hi, data_hi);
    }

    tcg_temp_free(data_lo);
    tcg_temp_free(data_hi);
    tcg_temp_free(addr);

    return DISAS_NEXT;
}

#endif
#ifdef TARGET_ARCV2

/*
 * Function to add boiler plate code for conditional execution.
 * It will add tcg_gen codes only if there is a condition to
 * be checked (ctx->insn.cc != 0).
 * Remember to pair it with gen_cc_epilogue(ctx) macro.
 */
static void gen_cc_prologue(DisasCtxt *ctx)
{
    ctx->tmp_reg = tcg_temp_local_new();
    ctx->label = gen_new_label();
    if (ctx->insn.cc) {
        arc_gen_verifyCCFlag(ctx, ctx->tmp_reg);
        tcg_gen_brcondi_tl(TCG_COND_NE, ctx->tmp_reg, 1, ctx->label);
    }
}

/*
 * The finishing counter part of gen_cc_prologue. This is supposed
 * to be put at the end of the function using it.
 */
static void gen_cc_epilogue(const DisasCtxt *ctx)
{
    if (ctx->insn.cc) {
        gen_set_label(ctx->label);
    }
    tcg_temp_free(ctx->tmp_reg);
}

/*
 * Verifies if the destination operand (operand 0) is a register
 * then it is an even numbered one. Else, an exception is put in
 * the generated code and FALSE is returned.
 */
static bool verify_dest_reg_is_even(const DisasCtxt *ctx)
{
    if (is_odd_numbered_register(ctx->insn.operands[0])) {
        arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
        return false;
    }
    return true;
}


/* accumulator = b32 * c32 (signed multiplication). */
int
arc_gen_MPYD(DisasCtxt *ctx, TCGv_i32 dest,
              TCGv_i32 b32, TCGv_i32 c32)
{
    if (verify_dest_reg_is_even(ctx)) {
        gen_cc_prologue(ctx);
        tcg_gen_muls2_i32(cpu_acclo, cpu_acchi, b32, c32);
        if (ctx->insn.operands[0].type & ARC_OPERAND_IR) {
            tcg_gen_mov_tl(arc_gen_next_reg(ctx, dest), cpu_acchi);
            tcg_gen_mov_tl(dest, cpu_acclo);
        }
        if (ctx->insn.f) {
            setNFlag(cpu_acchi);
            tcg_gen_movi_tl(cpu_Vf, 0);
        }
        gen_cc_epilogue(ctx);
    }
    return DISAS_NEXT;
}

/* accumulator = b32 * c32 (unsigned multiplication). */
int
arc_gen_MPYDU(DisasCtxt *ctx, TCGv_i32 dest,
              TCGv_i32 b32, TCGv_i32 c32)
{
    if (verify_dest_reg_is_even(ctx)) {
        gen_cc_prologue(ctx);
        tcg_gen_mulu2_i32(cpu_acclo, cpu_acchi, b32, c32);
        if (ctx->insn.operands[0].type & ARC_OPERAND_IR) {
            tcg_gen_mov_tl(arc_gen_next_reg(ctx, dest), cpu_acchi);
            tcg_gen_mov_tl(dest, cpu_acclo);
        }
        if (ctx->insn.f) {
            tcg_gen_movi_tl(cpu_Vf, 0);
        }
        gen_cc_epilogue(ctx);
    }
    return DISAS_NEXT;
}

/*
 * Populates a 64-bit vector with register pair:
 *   vec64=(REGn+1,REGn)=(REGn+1_hi,REGn+1_lo,REGn_hi,REGn_lo)
 * REG must be refering to an even numbered register.
 * Do not forget to free the returned TCGv_i64 when done!
 */
static TCGv_i64 pair_reg_to_i64(const DisasCtxt *ctx, TCGv_i32 reg)
{
    TCGv_i64 vec64 = tcg_temp_new_i64();
    tcg_gen_concat_i32_i64(vec64, reg, arc_gen_next_reg(ctx, reg));
    return vec64;
}

/*
 * Populates a 32-bit vector with repeating SHIMM:
 *   vec32=(0000000000u6,0000000000u6)
 *   vec32=(sssss12,sssss12)
 * It's crucial that the s12 part of an encoding is in signed
 * integer form while passed along in SHIMM, e.g:
 *   s12 = -125 (0xf803) --> 0xfffff803
 * Do not forget to free the returned TCGv_i32 when done!
 */
static TCGv_i32 dup_shimm_to_i32(int16_t shimm)
{
    TCGv_i32 vec32 = tcg_temp_new_i32();
    int32_t val = shimm;
    val = ((val << 16) & 0xffff0000) | (val & 0xffff);
    tcg_gen_movi_i32(vec32, val);
    return vec32;
}

/*
 * Populates a 64-bit vector with repeating LIMM:
 *   vec64=(limm,limm)=(limm_hi,limm_lo,limm_hi,limm_lo)
 * Do not forget to free the returned TCGv_i64 when done!
 */
static TCGv_i64 dup_limm_to_i64(int32_t limm)
{
    TCGv_i64 vec64 = tcg_temp_new_i64();
    int64_t val = limm;
    val = (val << 32) | (val & 0xffffffff);
    tcg_gen_movi_i64(vec64, val);
    return vec64;
}

/*
 * Populates a 64-bit vector with four SHIMM (u6 or s12):
 *   vec64=(0000000000u6,0000000000u6,0000000000u6,0000000000u6)
 *   vec64=(sssss12,sssss12,sssss12,sssss12)
 * It's crucial that the s12 part of an encoding is in signed
 * integer form while passed along in SHIMM, e.g:
 *   s12 = -125 (0xf803) --> 0xfffff803
 * Do not forget to free the returned TCGv_i64 when done!
 */
static TCGv_i64 quad_shimm_to_i64(int16_t shimm)
{
    TCGv_i64 vec64 = tcg_temp_new_i64();
    int64_t val = shimm;
    val = (val << 48) | ((val << 32) & 0x0000ffff00000000) |
          ((val << 16) & 0x00000000ffff0000) | (val & 0xffff);
    tcg_gen_movi_i64(vec64, val);
    return vec64;
}

/*
 * gen_vec_op2 emits instructions to perform the desired operation,
 * defined by OP, on the inputs (B32 and C32) and returns the
 * result in DEST.
 *
 * vector size:     64-bit
 * vector elements: 2
 * element size:    32-bit
 *
 * (A1, A0) = (B1, B0) op (C1, C0)
 */
static void gen_vec_op2(const DisasCtxt *ctx,
                        void (*OP)(TCGv_i64, TCGv_i64, TCGv_i64),
                        TCGv_i32 dest,
                        TCGv_i32 b32,
                        TCGv_i32 c32)
{
    TCGv_i64 d64, b64, c64;

    /* If no real register for result, then this a nop. Bail out! */
    if (!(ctx->insn.operands[0].type & ARC_OPERAND_IR)) {
        return;
    }

    /* Extend B32 to B64 based on its type: {reg, limm}. */
    if (ctx->insn.operands[1].type & ARC_OPERAND_IR) {
        b64 = pair_reg_to_i64(ctx, b32);
    } else if (ctx->insn.operands[1].type & ARC_OPERAND_LIMM) {
        b64 = dup_limm_to_i64(ctx->insn.limm);
    } else {
        g_assert_not_reached();
    }
    /* Extend C32 to C64 based on its type: {reg, limm, shimm}. */
    if (ctx->insn.operands[2].type & ARC_OPERAND_IR) {
        c64 = pair_reg_to_i64(ctx, c32);
    } else if (ctx->insn.operands[2].type & ARC_OPERAND_LIMM) {
        c64 = dup_limm_to_i64(ctx->insn.limm);
    } else if (ctx->insn.operands[2].type & ARC_OPERAND_SHIMM) {
        /* At this point SHIMM is extended like LIMM. */
        c64 = dup_limm_to_i64(ctx->insn.operands[2].value);
    } else {
        g_assert_not_reached();
    }
    d64 = tcg_temp_new_i64();

    (*OP)(d64, b64, c64);
    tcg_gen_extrl_i64_i32(dest, d64);
    tcg_gen_extrh_i64_i32(arc_gen_next_reg(ctx, dest), d64);

    tcg_temp_free_i64(d64);
    tcg_temp_free_i64(c64);
    tcg_temp_free_i64(b64);
    return;
}

/*
 * gen_vec_op2h emits instructions to perform the desired operation,
 * defined by OP, on the inputs (B32 and C32) and returns the
 * result in DEST.
 *
 * vector size:     32-bit
 * vector elements: 2
 * element size:    16-bit
 *
 * (a1, a0) = (b1, b0) op (c1, c0)
 */
static void gen_vec_op2h(const DisasCtxt *ctx,
                         void (*OP)(TCGv, TCGv, TCGv),
                         TCGv_i32 dest,
                         TCGv_i32 b32,
                         TCGv_i32 c32)
{
    TCGv_i32 t0, t1;

    /* If no real register for result, then this a nop. Bail out! */
    if (!(ctx->insn.operands[0].type & ARC_OPERAND_IR)) {
        return;
    }

    t0 = tcg_temp_new();
    tcg_gen_mov_i32(t0, b32);
    /*
     * If the last operand is a u6/s12, say 63, there is no "HI" in it.
     * Instead, it must be duplicated to form a pair; e.g.: (63, 63).
     */
    if (ctx->insn.operands[2].type & ARC_OPERAND_SHIMM) {
        t1 = dup_shimm_to_i32(ctx->insn.operands[2].value);
    } else {
        t1 = tcg_temp_new();
        tcg_gen_mov_i32(t1, c32);
    }

    (*OP)(dest, t0, t1);

    tcg_temp_free(t1);
    tcg_temp_free(t0);
}


/*
 * gen_vec_op4h emits instructions to perform the desired operation,
 * defined by OP, on the inputs (B32 and C32) and returns the
 * result in DEST.
 *
 * vector size:     64-bit
 * vector elements: 4
 * element size:    16-bit
 *
 * (a3, a2, a1, a0) = (b3, b2, b1, b0) op (c3, c2, c1, c0)
 */
static void gen_vec_op4h(const DisasCtxt *ctx,
                         void (*op)(TCGv_i64, TCGv_i64, TCGv_i64),
                         TCGv_i32 dest,
                         TCGv_i32 b32,
                         TCGv_i32 c32)
{
    TCGv_i64 d64, b64, c64;

    /* If no real register for result, then this a nop. Bail out! */
    if (!(ctx->insn.operands[0].type & ARC_OPERAND_IR)) {
        return;
    }

    /* Extend B32 to B64 based on its type: {reg, limm}. */
    if (ctx->insn.operands[1].type & ARC_OPERAND_IR) {
        b64 = pair_reg_to_i64(ctx, b32);
    } else if (ctx->insn.operands[1].type & ARC_OPERAND_LIMM) {
        b64 = dup_limm_to_i64(ctx->insn.limm);
    } else {
        g_assert_not_reached();
    }
    /* Extend C32 to C64 based on its type: {reg, limm, shimm}. */
    if (ctx->insn.operands[2].type & ARC_OPERAND_IR) {
        c64 = pair_reg_to_i64(ctx, c32);
    } else if (ctx->insn.operands[2].type & ARC_OPERAND_LIMM) {
        c64 = dup_limm_to_i64(ctx->insn.limm);
    } else if (ctx->insn.operands[2].type & ARC_OPERAND_SHIMM) {
        c64 = quad_shimm_to_i64(ctx->insn.operands[2].value);
    } else {
        g_assert_not_reached();
    }
    d64 = tcg_temp_new_i64();

    (*op)(d64, b64, c64);
    tcg_gen_extrl_i64_i32(dest, d64);
    tcg_gen_extrh_i64_i32(arc_gen_next_reg(ctx, dest), d64);

    tcg_temp_free_i64(d64);
    tcg_temp_free_i64(c64);
    tcg_temp_free_i64(b64);
    return;
}

/*
 * To use a 32-bit adder to sum two 16-bit numbers:
 * 1) Mask out the 16th bit in both operands to cause no carry.
 * 2) Add the numbers.
 * 3) Put back the 16th bit sum: T0[15] ^ T1[15] ^ CARRY[14]
 *    (ignoring the possible carry generated)
 * T0 and T1 values will change. Use temporary ones.
 */
static void gen_add16(TCGv_i32 dest, TCGv_i32 t0, TCGv_i32 t1)
{
    TCGv_i32 tmp = tcg_temp_new_i32();
    tcg_gen_xor_i32(tmp, t0, t1);
    tcg_gen_andi_i32(tmp, tmp, 0x8000);
    tcg_gen_andi_i32(t0, t0, ~0x8000);
    tcg_gen_andi_i32(t1, t1, ~0x8000);
    tcg_gen_add_i32(t0, t0, t1);
    tcg_gen_xor_i32(dest, t0, tmp);
    tcg_temp_free_i32(tmp);
}

/*
 * To use a 32-bit subtracter to subtract two 16-bit numbers:
 * 0) Record how T0[15]-T1[15] would result without other bits.
 * 1) Make the 16th bit for the first operand 1 and the second
 *    operand 0. This combination of (1 - 0) will absorb any
 *    possible borrow that may come from the 15th bit.
 * 2) Subtract the numbers.
 * 3) Correct the 16th bit result (1 - 0 - B):
 *    If the 16th bit is 1 --> no borrow was asked.
 *    If the 16th bit is 0 --> a  borrow was asked.
 *    and if a borrow was asked, the result of step 0 must be
 *    inverted (0 -> 1 and 1 -> 0). If not, the result of step
 *    0 can be used readily:
 *     STEP2[15] | T0[15]-T1[15] | DEST[15]
 *     ----------+---------------+---------
 *         0     |       0       |    1
 *         0     |       1       |    0
 *         1     |       0       |    0
 *         1     |       1       |    1
 *    This is a truth table for XNOR(a,b):
 *      NOT(XOR(a,b))=XOR(XOR(a,b),1)
 * This approach might seem pedantic, but it generates one less
 * instruction than the obvious mask-and-sub approach and requires
 * two less TCG variables.
 * T0 and T1 values will change. Use temporary ones.
 */
static void gen_sub16(TCGv_i32 dest, TCGv_i32 t0, TCGv_i32 t1)
{
    TCGv_i32 tmp = tcg_temp_new_i32();
    tcg_gen_xor_i32(tmp, t0, t1);          /* step 0 */
    tcg_gen_andi_i32(tmp, tmp, 0x8000);    /* step 0 */
    tcg_gen_ori_i32(t0, t0, 0x8000);       /* step 1 */
    tcg_gen_andi_i32(t1, t1, ~0x8000);     /* step 1 */
    tcg_gen_sub_i32(t0, t0, t1);           /* step 2 */
    tcg_gen_xor_i32(dest, t0, tmp);        /* step 3 */
    tcg_gen_xori_i32(dest, dest, 0x8000);  /* step 3 */
    tcg_temp_free_i32(tmp);
}

/*
 * Going through every operand, if any of those is a register
 * it is verified to be an even numbered register. Else, an
 * exception is put in the generated code and FALSE is returned.
 */
static bool verify_all_regs_are_even(const DisasCtxt *ctx)
{
    for (int nop = 0; nop < ctx->insn.n_ops; ++nop) {
        if (is_odd_numbered_register(ctx->insn.operands[nop])) {
            arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
            return false;
        }
    }
    return true;
}


int
arc_gen_VADD2(DisasCtxt *ctx, TCGv dest, TCGv_i32 b, TCGv_i32 c)
{
    if (verify_all_regs_are_even(ctx)) {
        gen_cc_prologue(ctx);
        gen_vec_op2(ctx, tcg_gen_vec_add32_i64, dest, b, c);
        gen_cc_epilogue(ctx);
    }
    return DISAS_NEXT;
}

int
arc_gen_VADD2H(DisasCtxt *ctx, TCGv dest, TCGv_i32 b, TCGv_i32 c)
{
    gen_cc_prologue(ctx);
    gen_vec_op2h(ctx, gen_add16, dest, b, c);
    gen_cc_epilogue(ctx);
    return DISAS_NEXT;
}

int
arc_gen_VADD4H(DisasCtxt *ctx, TCGv dest, TCGv_i32 b, TCGv_i32 c)
{
    if (verify_all_regs_are_even(ctx)) {
        gen_cc_prologue(ctx);
        gen_vec_op4h(ctx, tcg_gen_vec_add16_i64, dest, b, c);
        gen_cc_epilogue(ctx);
    }
    return DISAS_NEXT;
}

int
arc_gen_VSUB2(DisasCtxt *ctx, TCGv dest, TCGv_i32 b, TCGv_i32 c)
{
    if (verify_all_regs_are_even(ctx)) {
        gen_cc_prologue(ctx);
        gen_vec_op2(ctx, tcg_gen_vec_sub32_i64, dest, b, c);
        gen_cc_epilogue(ctx);
    }
    return DISAS_NEXT;
}

int
arc_gen_VSUB2H(DisasCtxt *ctx, TCGv dest, TCGv_i32 b, TCGv_i32 c)
{
    gen_cc_prologue(ctx);
    gen_vec_op2h(ctx, gen_sub16, dest, b, c);
    gen_cc_epilogue(ctx);
    return DISAS_NEXT;
}

int
arc_gen_VSUB4H(DisasCtxt *ctx, TCGv dest, TCGv_i32 b, TCGv_i32 c)
{
    if (verify_all_regs_are_even(ctx)) {
        gen_cc_prologue(ctx);
        gen_vec_op4h(ctx, tcg_gen_vec_sub16_i64, dest, b, c);
        gen_cc_epilogue(ctx);
    }
    return DISAS_NEXT;
}
#endif

int
arc_gen_SWI(DisasCtxt *ctx, TCGv a)
{
    TCGv tcg_index = tcg_const_tl(EXCP_SWI);
    TCGv tcg_cause = tcg_const_tl(0);

    tcg_gen_movi_tl(cpu_pc, ctx->cpc);
    tcg_gen_movi_tl(cpu_eret, ctx->cpc);
    tcg_gen_movi_tl(cpu_erbta, ctx->npc);

    gen_helper_raise_exception(cpu_env, tcg_index, tcg_cause, a);

    tcg_temp_free(tcg_index);
    tcg_temp_free(tcg_cause);
    return DISAS_NEXT;
}
int
arc_gen_TRAP(DisasCtxt *ctx, TCGv a)
{
    TCGv tcg_index = tcg_const_tl(EXCP_TRAP);
    TCGv tcg_cause = tcg_const_tl(0);

    tcg_gen_movi_tl(cpu_pc, ctx->cpc);
    tcg_gen_movi_tl(cpu_eret, ctx->npc);
    tcg_gen_mov_tl(cpu_erbta, cpu_bta);

    gen_helper_raise_exception(cpu_env, tcg_index, tcg_cause, a);

    tcg_temp_free(tcg_index);
    tcg_temp_free(tcg_cause);

    return DISAS_NORETURN;
}
int
arc_gen_RTIE(DisasCtxt *ctx)
{
    tcg_gen_movi_tl(cpu_pc, ctx->cpc);
    gen_helper_rtie(cpu_env);
    tcg_gen_mov_tl(cpu_pc, cpu_pcl);
    exit_tb(ctx); /* no chaining */
    return DISAS_NORETURN;
}

/* Generate sleep insn. */
int
arc_gen_SLEEP(DisasCtxt *ctx, TCGv a)
{
    uint32_t param = 0;

    if (ctx->insn.operands[0].type & ARC_OPERAND_IR) {
        TCGv tmp3 = tcg_temp_local_new();
        TCGv tmp4 = tcg_temp_local_new();
        TCGLabel *done_L = gen_new_label();

        tcg_gen_andi_tl(tmp3, a, 0x10);
        tcg_gen_brcondi_tl(TCG_COND_NE, tmp3, 0x10, done_L);

        tcg_gen_andi_tl(tmp4, a, 0x0f);
        TCG_SET_STATUS_FIELD_VALUE(cpu_pstate, Ef, tmp4);
        TCG_SET_STATUS_FIELD_BIT(cpu_pstate, IEf);
        gen_set_label(done_L);

        tcg_temp_free(tmp3);
        tcg_temp_free(tmp4);
    } else {
        param = ctx->insn.operands[0].value;
        if (param & 0x10) {
            TCG_SET_STATUS_FIELD_BIT(cpu_pstate, IEf);
            TCG_SET_STATUS_FIELD_IVALUE(cpu_pstate, Ef, param & 0x0f);
        }
    }
    /* FIXME: setup debug registers as well. */

    TCGv npc = tcg_temp_local_new();
    tcg_gen_movi_tl(npc, ctx->npc);
    gen_helper_halt(cpu_env, npc);
    tcg_temp_free(npc);
    return DISAS_NEXT;
}

/* Given a CTX, generate the relevant TCG code for the given opcode. */
static int arc_decode(DisasContext *ctx, const struct arc_opcode *opcode)
{
    int ret = DISAS_NEXT;
    enum arc_opcode_map mapping;
    static bool initialized;

    if (initialized == false) {
        init_constants();
        initialized = true;
    }

    /* Do the mapping. */
    mapping = arc_map_opcode(opcode);
    if (mapping != MAP_NONE) {
        TCGv ops[10];
        int i;
        for (i = 0; i < number_of_ops_semfunc[mapping]; i++) {
            ops[i] = arc_decode_operand(opcode, ctx, i, mapping);
        }

        /*
         * Store some elements statically to implement less dynamic
         * features of instructions.  Started by the need to keep a
         * static reference to LP_START and LP_END.
         */

#define SEMANTIC_FUNCTION_CALL_0(NAME, A)       \
            arc_gen_##NAME(ctx);
#define SEMANTIC_FUNCTION_CALL_1(NAME, A)       \
            arc_gen_##NAME(ctx, ops[A]);
#define SEMANTIC_FUNCTION_CALL_2(NAME, A, B)            \
            arc_gen_##NAME(ctx, ops[A], ops[B]);
#define SEMANTIC_FUNCTION_CALL_3(NAME, A, B, C)                 \
            arc_gen_##NAME(ctx, ops[A], ops[B], ops[C]);
#define SEMANTIC_FUNCTION_CALL_4(NAME, A, B, C, D)                      \
            arc_gen_##NAME(ctx, ops[A], ops[B], ops[C], ops[D]);

#define SEMANTIC_FUNCTION(...)
#define CONSTANT(...)
#define MAPPING(MNEMONIC, NAME, NOPS, ...)                              \
            case MAP_##MNEMONIC##_##NAME:                               \
                ret = SEMANTIC_FUNCTION_CALL_##NOPS(NAME, __VA_ARGS__); \
                break;
        switch (mapping) {
#include "target/arc/semfunc_mapping.def"
#include "target/arc/extra_mapping.def"

        default:
            arc_debug_opcode(opcode, ctx, "No handle for map opcode");
            arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
        }
#undef MAPPING
#undef CONSTANT
#undef SEMANTIC_FUNCTION
#undef SEMANTIC_FUNCTION_CALL_0
#undef SEMANTIC_FUNCTION_CALL_1
#undef SEMANTIC_FUNCTION_CALL_2
#undef SEMANTIC_FUNCTION_CALL_3

        for (i = 0; i < number_of_ops_semfunc[mapping]; i++) {
            operand_t operand = ctx->insn.operands[i];
            if (!(operand.type & ARC_OPERAND_LIMM) &&
                !(operand.type & ARC_OPERAND_IR)) {
                tcg_temp_free(ops[i]);
            }
        }

    } else {
        arc_debug_opcode(opcode, ctx, "No mapping for opcode");
        arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
    }

    return ret;
}

void decode_opc(CPUARCState *env, DisasContext *ctx)
{
    ctx->env = env;

    env->enabled_interrupts = false;

    const struct arc_opcode *opcode = NULL;
    if (!read_and_decode_context(ctx, &opcode)) {
        ctx->base.is_jmp = arc_gen_INVALID(ctx);
        return;
    }

    ctx->base.is_jmp = arc_decode(ctx, opcode);

    TCGv npc = tcg_const_local_tl(ctx->npc);
    gen_helper_zol_verify(cpu_env, npc);
    tcg_temp_free(npc);

    env->enabled_interrupts = true;
}

static void arc_tr_translate_insn(DisasContextBase *dcbase, CPUState *cpu)
{
    bool in_a_delayslot_instruction = false;
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    CPUARCState *env = cpu->env_ptr;

    /* TODO (issue #62): these must be removed */
    dc->zero = tcg_const_local_tl(0);
    dc->one  = tcg_const_local_tl(1);

    if (env->stat.is_delay_slot_instruction == 1) {
        in_a_delayslot_instruction = true;
    }

    dc->cpc = dc->base.pc_next;
    decode_opc(env, dc);

    dc->base.pc_next = dc->npc;
    tcg_gen_movi_tl(cpu_npc, dc->npc);

    if (in_a_delayslot_instruction == true) {
        TCGv temp_DEf = tcg_temp_local_new();
        dc->base.is_jmp = DISAS_NORETURN;

        /* Post execution delayslot logic. */
        TCGLabel *DEf_not_set_label1 = gen_new_label();
        TCG_GET_STATUS_FIELD_MASKED(temp_DEf, cpu_pstate, DEf);
        tcg_gen_brcondi_tl(TCG_COND_EQ, temp_DEf, 0, DEf_not_set_label1);
        TCG_CLR_STATUS_FIELD_BIT(cpu_pstate, DEf);
        gen_goto_tb(dc, 1, cpu_bta);
        gen_set_label(DEf_not_set_label1);
        env->stat.is_delay_slot_instruction = 0;

        tcg_temp_free(temp_DEf);
    }

    if (dc->base.is_jmp == DISAS_NORETURN) {
        gen_gotoi_tb(dc, 0, dc->npc);
    } else if (dc->base.is_jmp == DISAS_NEXT) {
        target_ulong page_start;

        page_start = dc->base.pc_first & TARGET_PAGE_MASK;
        if (dc->base.pc_next - page_start >= TARGET_PAGE_SIZE) {
            dc->base.is_jmp = DISAS_TOO_MANY;
        }
    }

    /* TODO (issue #62): these must be removed. */
    tcg_temp_free(dc->zero);
    tcg_temp_free(dc->one);

    /* verify if there is any TCG temporaries leakge */
    translator_loop_temp_check(dcbase);
}

static void arc_tr_tb_stop(DisasContextBase *dcbase, CPUState *cpu)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    switch (dc->base.is_jmp) {
    case DISAS_TOO_MANY:
    case DISAS_UPDATE:
        gen_gotoi_tb(dc, 0, dc->base.pc_next);
        break;
    case DISAS_BRANCH_IN_DELAYSLOT:
    case DISAS_NORETURN:
        break;
    default:
         g_assert_not_reached();
    }

    if (dc->base.num_insns == dc->base.max_insns &&
        (dc->base.tb->cflags & CF_LAST_IO)) {
        gen_io_end();
    }
}

static void arc_tr_disas_log(const DisasContextBase *dcbase, CPUState *cpu)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    qemu_log("IN: %s\n", lookup_symbol(dc->base.pc_first));
    log_target_disas(cpu, dc->base.pc_first, dc->base.tb->size);
}


static const TranslatorOps arc_translator_ops = {
    .init_disas_context = arc_tr_init_disas_context,
    .tb_start           = arc_tr_tb_start,
    .insn_start         = arc_tr_insn_start,
    .breakpoint_check   = arc_tr_breakpoint_check,
    .translate_insn     = arc_tr_translate_insn,
    .tb_stop            = arc_tr_tb_stop,
    .disas_log          = arc_tr_disas_log,
};

/* generate intermediate code for basic block 'tb'. */
void gen_intermediate_code(CPUState *cpu,
                           TranslationBlock *tb,
                           int max_insns)
{
    DisasContext dc;
    const TranslatorOps *ops = &arc_translator_ops;
    translator_loop(ops, &dc.base, cpu, tb, max_insns);
}

void restore_state_to_opc(CPUARCState *env,
                          TranslationBlock *tb,
                          target_ulong *data)
{
    env->pc = data[0];
}

void arc_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    ARCCPU *cpu = ARC_CPU(cs);
    CPUARCState *env = &cpu->env;
    int i;


    qemu_fprintf(f,
                 "STATUS:  [ %c %c %c %c %c %c %s %s %s %s %s %s %c]\n",
                 GET_STATUS_BIT(env->stat, Lf)  ? 'L' : '-',
                 env->stat.Zf ? 'Z' : '-',
                 env->stat.Nf ? 'N' : '-',
                 env->stat.Cf ? 'C' : '-',
                 env->stat.Vf ? 'V' : '-',
                 GET_STATUS_BIT(env->stat, Uf)  ? 'U' : '-',
                 GET_STATUS_BIT(env->stat, DEf) ? "DE" : "--",
                 GET_STATUS_BIT(env->stat, AEf) ? "AE" : "--",
                 GET_STATUS_BIT(env->stat, Ef)  ? "E" : "--",
                 GET_STATUS_BIT(env->stat, DZf) ? "DZ" : "--",
                 GET_STATUS_BIT(env->stat, SCf) ? "SC" : "--",
                 GET_STATUS_BIT(env->stat, IEf) ? "IE" : "--",
                 GET_STATUS_BIT(env->stat, Hf)  ? 'H' : '-'
                 );

    qemu_fprintf(f, "\n");
    for (i = 0; i < ARRAY_SIZE(env->r); i++) {
        qemu_fprintf(f, "R[%02d]:  " TARGET_FMT_lx "   ", i, env->r[i]);

        if ((i % 8) == 7) {
            qemu_fprintf(f, "\n");
        }
    }
}


/*-*-indent-tabs-mode:nil;tab-width:4;indent-line-function:'insert-tab'-*-*/
/* vim: set ts=4 sw=4 et: */
