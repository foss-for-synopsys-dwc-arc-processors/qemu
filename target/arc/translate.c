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

/* LLOCK and SCOND support values */
TCGv cpu_exclusive_addr;
TCGv cpu_exclusive_val;
TCGv cpu_exclusive_val_hi;

/* Macros */

#include "exec/gen-icount.h"
#define REG(x)  (cpu_r[x])

/* macro used to fix middle-endianess. */
#define ARRANGE_ENDIAN(endianess, buf)                  \
    ((endianess) ? ror32(buf, 16) : bswap32(buf))

void gen_goto_tb(const DisasContext *ctx, int n, TCGv dest)
{
    tcg_gen_mov_tl(cpu_pc, dest);
    tcg_gen_andi_tl(cpu_pcl, dest, ~((target_ulong) 3));
    /* TODO: is this really needed !!! */
    if (ctx->base.singlestep_enabled) {
        gen_helper_debug(cpu_env);
    } else {
        tcg_gen_exit_tb(NULL, 0);
    }
}

static void gen_gotoi_tb(DisasContext *ctx, int n, target_ulong dest)
{
    if (translator_use_goto_tb(&ctx->base, dest)) {
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

        NEW_ARC_REG(cpu_er_pstate, stat_er.pstate)
        NEW_ARC_REG(cpu_er_Zf, stat_er.Zf)
        NEW_ARC_REG(cpu_er_Nf, stat_er.Nf)
        NEW_ARC_REG(cpu_er_Cf, stat_er.Cf)
        NEW_ARC_REG(cpu_er_Vf, stat_er.Vf)

        NEW_ARC_REG(cpu_eret, eret)
        NEW_ARC_REG(cpu_erbta, erbta)
        NEW_ARC_REG(cpu_efa, efa)
        NEW_ARC_REG(cpu_bta, bta)
#if defined(TARGET_ARC32)
        NEW_ARC_REG(cpu_lps, lps)
        NEW_ARC_REG(cpu_lpe, lpe)
#endif
        NEW_ARC_REG(cpu_pc , pc)
        NEW_ARC_REG(cpu_npc, npc)

        NEW_ARC_REG(cpu_intvec, intvec)

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

    cpu_exclusive_addr = tcg_global_mem_new(cpu_env,
        offsetof(CPUARCState, exclusive_addr), "exclusive_addr");
    cpu_exclusive_val = tcg_global_mem_new(cpu_env,
        offsetof(CPUARCState, exclusive_val), "exclusive_val");
    cpu_exclusive_val_hi = tcg_global_mem_new(cpu_env,
        offsetof(CPUARCState, exclusive_val_hi), "exclusive_val_hi");
}

static void arc_tr_init_disas_context(DisasContextBase *dcbase,
                                      CPUState *cs)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);

    dc->base.is_jmp = DISAS_NEXT;
    dc->mem_idx = dc->base.tb->flags & 1;
}
static void arc_tr_tb_start(DisasContextBase *dcbase, CPUState *cpu)
{
    /* place holder for now */
    /* TODO: Make sure you really need to guard it. */
    //DisasContext *dc = container_of(dcbase, DisasContext, base);
    //dc->possible_delayslot_instruction = dc->env->stat.DEf;

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

static int arc_gen_INVALID(const DisasContext *ctx)
{
    qemu_log_mask(LOG_UNIMP,
                  "invalid inst @:" TARGET_FMT_lx "\n", ctx->cpc);
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


    //qemu_log_mask(LOG_UNIMP, "-- 0x%016lx --\n", insn);
    //qemu_log_mask(LOG_UNIMP, "Tree Decoded format at 0x" TARGET_FMT_lx " (0x" TARGET_FMT_lx ") - %d - %s\n",
    //              ctx->cpc, insn, opcode_id, opcode_name_str[opcode_id]);

    /*
     * Now, we have read the entire opcode, decode it and place the
     * relevant info into opcode and ctx->insn.
     */


    //opcode_id = OPCODE_INVALID;
    *opcode_p = arc_find_format(&ctx->insn, insn, length, cpu->family);

    //if(opcode_id != OPCODE_INVALID)
    //  qemu_log_mask(LOG_UNIMP, "Linear decoder format at 0x" TARGET_FMT_lx " (0x%08lx) - %d - %s\n",
    //                ctx->cpc, insn, opcode_id, opcode_name_str[opcode_id]);


    if (*opcode_p == NULL) {
        return false;
    }

    /*
     * If the instruction requires long immediate, read the extra 4
     * bytes and initialize the relevant fields.
     */

#if defined(TARGET_ARC32)
    if (ctx->insn.limm_p) {
        ctx->insn.limm = ARRANGE_ENDIAN(true,
                                        cpu_ldl_code(ctx->env,
                                        ctx->cpc + length));
        length += 4; 
#elif defined(TARGET_ARC64)
    if (ctx->insn.unsigned_limm_p) {
        ctx->insn.limm = ARRANGE_ENDIAN(true,
                                        cpu_ldl_code(ctx->env,
                                        ctx->cpc + length));
        length += 4;
    } else if (ctx->insn.signed_limm_p) {
        ctx->insn.limm = ARRANGE_ENDIAN(true,
                                        cpu_ldl_code (ctx->env,
                                        ctx->cpc + length));
        if (ctx->insn.limm & 0x80000000)
          ctx->insn.limm += 0xffffffff00000000;
        length += 4;
#endif
    } else {
        ctx->insn.limm = 0;
    }

#if defined(TARGET_ARC32)
    if(ctx->insn.limm_split_16_p) {
        ctx->insn.limm &= 0x0000ffff;
        ctx->insn.limm |= (ctx->insn.limm << 16);
    }
#elif defined(TARGET_ARC64)
    if(ctx->insn.limm_split_16_p) {
        ctx->insn.limm &= 0x0000ffff;
        ctx->insn.limm |= (ctx->insn.limm << 16);
        ctx->insn.limm |= (ctx->insn.limm << 32);
    }
#endif

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
#include "target/arc/semfunc-mapping.def"
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
#include "target/arc/semfunc-mapping.def"
#undef MAPPING
#undef CONSTANT
#undef SEMANTIC_FUNCTION
    2
};

#if 0
static void arc_list_unimplemented_mnemonics(void)
{
    int i;
    bool implemented[MNEMONIC_SIZE];
    memset(implemented, 0, sizeof(implemented));

#define SEMANTIC_FUNCTION(...)
#define CONSTANT(...)
#define MAPPING(INSN_NAME, NAME, ...)         \
    implemented[MNEMONIC_##INSN_NAME] = true;
#include "target/arc/semfunc-mapping.def"

#undef MAPPING
#undef CONSTANT
#undef SEMANTIC_FUNCTION
    for(i = 0; i < MNEMONIC_SIZE; i++) {
        if(implemented[i] == 0) {
            qemu_log_mask(LOG_UNIMP,
                  "Instruction %s is not mapped\n",
                  insn_mnemonic_str[i]);
        }
    }
}
#endif

static enum arc_opcode_map arc_map_opcode(const struct arc_opcode *opcode)
{
    switch(opcode->mnemonic) {
#define SEMANTIC_FUNCTION(...)
#define CONSTANT(...)
#define MAPPING(INSN_NAME, NAME, ...)         \
    case MNEMONIC_##INSN_NAME: \
        return MAP_##INSN_NAME##_##NAME;
#include "target/arc/semfunc-mapping.def"
#undef MAPPING
#undef CONSTANT
#undef SEMANTIC_FUNCTION
    default:
        /* TODO: This should be later taken out since default behaviour should
           be an invalid instruction exception. */
        qemu_log_mask(LOG_UNIMP,
                      "Instruction %s is not mapped to semantic function.\n",
                      opcode->name);
        assert("This should not happen" == 0);
    }

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
#include "target/arc/semfunc-mapping.def"
#undef MAPPING
#undef CONSTANT
#undef SEMANTIC_FUNCTION
}

static void arc_debug_opcode(const struct arc_opcode *opcode,
                             DisasContext *ctx,
                             const char *msg)
{
    qemu_log_mask(LOG_UNIMP,
                  "%s for %s at pc=0x" TARGET_FMT_lx "\n",
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
                if (operand.type & ARC_OPERAND_ALIGNED32)
                    limm <<= 2; // Only used by BL_S LIMM instructions
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
                  target_ulong index,
                  target_ulong causecode,
                  target_ulong param)
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
    if (ctx->env->in_delayslot_instruction) {
        TCGv tcg_index = tcg_const_tl(EXCP_INST_ERROR);
        TCGv tcg_cause = tcg_const_tl(0x1);
        TCGv tcg_param = tcg_const_tl(0x0);

        tcg_gen_movi_tl(cpu_efa,  ctx->cpc);
        tcg_gen_movi_tl(cpu_eret, ctx->cpc);
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
int arc_gen_ENTER(DisasCtxt *ctx)
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

    if (tb_cflags(ctx->base.tb) & CF_USE_ICOUNT) {
	    gen_io_start();
    }

#if defined(TARGET_ARC32)
    writeAuxReg(src2, src1);
#elif defined(TARGET_ARC64)
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
    int ret = DISAS_NORETURN;

    if (tb_cflags(ctx->base.tb) & CF_USE_ICOUNT) {
	    gen_io_start();
    }

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


int
arc_gen_HALT(DisasCtxt *ctx)
{
    int ret = DISAS_NEXT;
    TCGv npc = tcg_const_local_tl(ctx->cpc);
    gen_helper_halt(cpu_env, npc);
    tcg_temp_free(npc);
    return ret;
}

#ifdef TARGET_ARC64
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
        tcg_gen_qemu_ld_tl(data_lo, addr, ctx->mem_idx, MO_UQ);
        tcg_gen_addi_tl(addr, addr, 8);
        tcg_gen_qemu_ld_tl(data_hi, addr, ctx->mem_idx, MO_UQ);
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

/*
 * 128-bit store.
 * FIXME: There is a mixture of decoder stuffs in here.
 *        The logic must be moved to a decoding layer.
 */
int
arc_gen_STDL(DisasCtxt *ctx, TCGv base, TCGv offset, TCGv src)
{
    /*
     * Address writebacks for 128-bit loads.
     * ,----.----------.
     * | aa | mnemonic |
     * |----+----------|
     * | 0  | none     |
     * | 1  | a/aw     |
     * | 2  | ab       |
     * | 3  | as       |
     * `----^----------'
     */
    /* A non-register operand cannot be incremented. */
    if (ctx->insn.aa == 1 || ctx->insn.aa == 2)
    {
        if (!(ctx->insn.operands[1].type & ARC_OPERAND_IR)) {
            arc_gen_excp(ctx, EXCP_INST_ERROR, 0, 0);
            return DISAS_NORETURN;
      }
    }

    TCGv data_hi = NULL;
    bool  free_data_hi = false;

    /*
     * The "src" can be a register, w6 or immediate.
     * Look for next register pair if "src" is a register.
     * Will raise an exception if "src" is an odd register.
     */
    if (ctx->insn.operands[0].type & ARC_OPERAND_IR) {
        data_hi = nextReg(src);
        if (data_hi == NULL) {
            return DISAS_NORETURN;
        }
    } else if (ctx->insn.operands[0].type & ARC_OPERAND_LIMM
               || ctx->insn.operands[0].type & ARC_OPERAND_SIGNED) { /* w6 */
        /* Dealing with an immediate to store. */
        data_hi = tcg_temp_local_new();
        free_data_hi = true;

        if (ctx->insn.operands[0].type & ARC_OPERAND_SIGNED) {
            /* Sign extend. */
            tcg_gen_sari_i64(data_hi, src, 63);
        } else { /* By default, LIMM is UNSIGNED. */
            /* Just use "0" for the upper part. */
            tcg_gen_movi_tl(data_hi, 0);
        }
    } else {
        /* The type of operand to store is not as expected. */
        g_assert_not_reached();
    }

    /* Only defined after possible exception routine codes. */
    TCGv data_lo = tcg_temp_local_new();
    TCGv addr = tcg_temp_local_new();
    /* Caputre the data before it possibly changes (src = base). */
    tcg_gen_mov_tl(data_lo, src);

    switch (ctx->insn.aa) {
    case 0:  /* Simple base+offset access. */
        tcg_gen_add_tl(addr, base, offset);
        break;
    case 1:  /* Pre memory access increment. */
        tcg_gen_add_tl(addr, base, offset);
        tcg_gen_mov_tl(base, addr);
        break;
    case 2:  /* Post memory access increment. */
        tcg_gen_mov_tl(addr, base);
        break;
    case 3:  /* Address scaling. */
        tcg_gen_shli_tl(offset, offset, 3);
        tcg_gen_add_tl(addr, base, offset);
        break;
    default:
        g_assert_not_reached();
    }

    /* Store the data. */
    tcg_gen_qemu_st_tl(data_lo, addr, ctx->mem_idx, MO_UQ);
    tcg_gen_addi_tl(addr, addr, 8);
    tcg_gen_qemu_st_tl(data_hi, addr, ctx->mem_idx, MO_UQ);

    if (ctx->insn.aa == 2) { /* Post-memory access increment. */
        tcg_gen_add_tl(addr, base, offset);
        tcg_gen_mov_tl(base, addr);
    }

    if (free_data_hi) {
        tcg_temp_free(data_hi);
    }
    tcg_temp_free(data_lo);
    tcg_temp_free(addr);

    return DISAS_NEXT;
}

#endif
#ifdef TARGET_ARC32

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
            tcg_gen_mov_tl(nextReg(dest), cpu_acchi);
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
            tcg_gen_mov_tl(nextReg(dest), cpu_acchi);
            tcg_gen_mov_tl(dest, cpu_acclo);
        }
        if (ctx->insn.f) {
            tcg_gen_movi_tl(cpu_Vf, 0);
        }
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
    if (tb_cflags(ctx->base.tb) & CF_USE_ICOUNT) {
	    gen_io_start();
    }

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
#include "target/arc/semfunc-mapping.def"

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

    const struct arc_opcode *opcode = NULL;
    if (!read_and_decode_context(ctx, &opcode)) {
        ctx->base.is_jmp = arc_gen_INVALID(ctx);
        return;
    }

    if(env->next_insn_is_delayslot == true) {
        env->in_delayslot_instruction = true;
        env->next_insn_is_delayslot = false;
    }


    ctx->base.is_jmp = arc_decode(ctx, opcode);

    /*
     * Either decoder knows that this is a delayslot
     * or it is a return from exception and at decode time
     * DEf flag is set as a delayslot.
     * The PREVIOUS_IS_DELAYSLOTf flag is used for to mark delayslot
     * instructions even when the branch is not taken.
     * DEf is only set when the branch is taken. This is always
     * set.
     */
    if(env->in_delayslot_instruction == true
       || GET_STATUS_BIT(env->stat, PREVIOUS_IS_DELAYSLOTf)) {
        TCGv temp_DEf = tcg_temp_local_new();
        ctx->base.is_jmp = DISAS_NORETURN;

        TCG_CLR_STATUS_FIELD_BIT(cpu_pstate, PREVIOUS_IS_DELAYSLOTf);

        /* Post execution delayslot logic. */
        TCGLabel *DEf_not_set_label1 = gen_new_label();
        TCG_GET_STATUS_FIELD_MASKED(temp_DEf, cpu_pstate, DEf);
        tcg_gen_brcondi_tl(TCG_COND_EQ, temp_DEf, 0, DEf_not_set_label1);
        TCG_CLR_STATUS_FIELD_BIT(cpu_pstate, DEf);
        gen_goto_tb(ctx, 1, cpu_bta);
        gen_set_label(DEf_not_set_label1);

        tcg_temp_free(temp_DEf);
        env->in_delayslot_instruction = false;

    }

    if(env->next_insn_is_delayslot == true) {
      SET_STATUS_BIT(env->stat, PREVIOUS_IS_DELAYSLOTf, 1);
    }


/* #define ZOL_RUNTIME_SIMULATION */
#ifdef ZOL_RUNTIME_SIMULATION
    TCGv npc = tcg_const_local_tl(ctx->npc);
    gen_helper_zol_verify(cpu_env, npc);
    tcg_temp_free(npc);
#else
    if(1 && env->lpe == ctx->npc) {

        DisasJumpType ret = ctx->base.is_jmp;

        TCGLabel *zol_end = gen_new_label();
        TCGLabel *zol_else = gen_new_label();
        TCGv lps = tcg_temp_local_new();

        tcg_gen_brcondi_tl(TCG_COND_GTU, cpu_lpc, 1, zol_else);
          tcg_gen_movi_tl(cpu_lpc, 0);
          tcg_gen_br(zol_end);
        gen_set_label(zol_else);
          tcg_gen_subi_tl(cpu_lpc, cpu_lpc, 1);
          tcg_gen_movi_tl(lps, env->lps);
          setPC(lps);
        gen_set_label(zol_end);

        ctx->base.is_jmp = DISAS_NORETURN;

        tcg_temp_free(lps);
    }
#endif
}

static void arc_tr_translate_insn(DisasContextBase *dcbase, CPUState *cpu)
{
    DisasContext *dc = container_of(dcbase, DisasContext, base);
    CPUARCState *env = cpu->env_ptr;


    /* TODO (issue #62): these must be removed */
    dc->zero = tcg_const_local_tl(0);
    dc->one  = tcg_const_local_tl(1);

    dc->cpc = dc->base.pc_next;
    decode_opc(env, dc);

    dc->base.pc_next = dc->npc;
    tcg_gen_movi_tl(cpu_npc, dc->npc);

    if (dc->base.is_jmp == DISAS_NORETURN) {
        gen_gotoi_tb(dc, 0, dc->npc);
    }

    target_ulong page_start;
    page_start = dc->base.pc_first & TARGET_PAGE_MASK;

    /* Break TB is dealing with instructions in the boundaries of a page.
     *   size of 8 is the size of the biggest delayslot capable instruction.
     */
    if (dc->base.is_jmp == DISAS_NEXT
        && dc->base.pc_next - page_start >= TARGET_PAGE_SIZE-8) {
        dc->base.is_jmp = DISAS_TOO_MANY;
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
        dc->base.is_jmp = DISAS_NORETURN;
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
