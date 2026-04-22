#include "translate.h"

/* TODO: Add 2-bytes alignment checks for all J instructions */

#define GEN_TRANS_J(name, handler, target) \
    static bool trans_##name(DisasContext *ctx, arg_##name *a) \
    { \
        arc_gen_check_not_delay_slot(ctx); \
        TCGv reg_target = target; \
        handler(ctx, reg_target, a->delay); \
        return true; \
    }

#define GEN_TRANS_JCC(name, handler, target) \
    static bool trans_##name(DisasContext *ctx, arg_##name *a) \
    { \
        arc_gen_check_not_delay_slot(ctx); \
        TCGv reg_target = target; \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_branch_fallthrough(ctx); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        handler(ctx, reg_target, a->delay); \
        gen_set_label(skip); \
        return true; \
    }

#define GEN_TRANS_JCC_EQ_NE(name, handler, target, cond) \
    static bool trans_##name(DisasContext *ctx, arg_##name *a) \
    { \
        arc_gen_check_not_delay_slot(ctx); \
        TCGv reg_target = target; \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_branch_fallthrough(ctx); \
        arc_gen_cc_prologue(ctx, cond, skip); \
        handler(ctx, reg_target, false); \
        gen_set_label(skip); \
        return true; \
    }

static void arc_gen_j(DisasContext *ctx, TCGv target, bool delay_slot)
{
    if (delay_slot) {
        arc_gen_set_bta(ctx, target);

        ctx->base.is_jmp = DISAS_UPDATE;
    } else {
        tcg_gen_mov_tl(cpu_pc, target);

        ctx->base.is_jmp = DISAS_JUMP_INDIRECT;
    }
}

static void arc_gen_jl(DisasContext *ctx, TCGv target, bool delay_slot)
{
    if (delay_slot) {
        uint32_t delay_slot_size = arc_get_insn_length_by_address(ctx, ctx->base.pc_next);

        arc_gen_set_bta(ctx, target);
        tcg_gen_movi_tl(cpu_gpr[ARC_REGNUM_BLINK], ctx->base.pc_next + delay_slot_size);

        ctx->base.is_jmp = DISAS_UPDATE;
    } else {
        tcg_gen_mov_tl(cpu_pc, target);
        tcg_gen_movi_tl(cpu_gpr[ARC_REGNUM_BLINK], ctx->base.pc_next);

        ctx->base.is_jmp = DISAS_JUMP_INDIRECT;
    }
}

GEN_TRANS_J(j_reg, arc_gen_j, arc_insn_load_src_reg_tl(ctx, a->c))
GEN_TRANS_J(j_u6, arc_gen_j, tcg_constant_tl(a->imm))
GEN_TRANS_J(j_s12, arc_gen_j, tcg_constant_tl(a->imm))
GEN_TRANS_JCC(jcc_reg, arc_gen_j, arc_insn_load_src_reg_tl(ctx, a->c))
GEN_TRANS_JCC(jcc_u6, arc_gen_j, tcg_constant_tl(a->imm))
GEN_TRANS_J(j_s_reg, arc_gen_j, arc_insn_load_src_reg_s_tl(ctx, a->b))
GEN_TRANS_J(j_s_blink, arc_gen_j, cpu_gpr[ARC_REGNUM_BLINK])
GEN_TRANS_JCC_EQ_NE(jeq_s_blink, arc_gen_j, cpu_gpr[ARC_REGNUM_BLINK], ARC_CC_EQ)
GEN_TRANS_JCC_EQ_NE(jne_s_blink, arc_gen_j, cpu_gpr[ARC_REGNUM_BLINK], ARC_CC_NE)

GEN_TRANS_J(jl_reg, arc_gen_jl, arc_insn_load_src_reg_tl(ctx, a->c))
GEN_TRANS_J(jl_u6, arc_gen_jl, tcg_constant_tl(a->imm))
GEN_TRANS_J(jl_s12, arc_gen_jl, tcg_constant_tl(a->imm))
GEN_TRANS_JCC(jlcc_reg, arc_gen_jl, arc_insn_load_src_reg_tl(ctx, a->c))
GEN_TRANS_JCC(jlcc_u6, arc_gen_jl, tcg_constant_tl(a->imm))
GEN_TRANS_J(jl_s_reg, arc_gen_jl, arc_insn_load_src_reg_s_tl(ctx, a->b))

/*
 * JLI_S u10
 *
 * BLINK = PC + 2
 * PC = JLI_BASE + (u10 << 2)
 */
#ifdef TARGET_ARCV3_64
static bool trans_jli_s_64(DisasContext *ctx, arg_jli_s_64 *a)
#else
static bool trans_jli_s(DisasContext *ctx, arg_jli_s *a)
#endif
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv base = tcg_temp_new();
    TCGv offset = tcg_constant_tl(a->offset << 2);
    TCGv aux_regnum = tcg_constant_tl(ARC_AUX_REGNUM_JLI_BASE);
    gen_helper_aux_reg_read_tl(base, tcg_env, aux_regnum);

    tcg_gen_movi_tl(cpu_gpr[ARC_REGNUM_BLINK], ctx->base.pc_next);
    tcg_gen_add_tl(cpu_pc, base, offset);

    ctx->base.is_jmp = DISAS_JUMP_INDIRECT;

    return true;
}

#ifdef TARGET_ARCV3_64
ARC_DEF_TRANS_STUB(jli_s)
#else
ARC_DEF_TRANS_STUB(jli_s_64)
#endif
