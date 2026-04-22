#include "translate.h"

static inline void arc_gen_mov(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_mov_i32(b, c);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        arc_gen_set_flag_N(b);
    }
}

ARC_DEF_TRANS_DOP_2OP_REG_REG(mov)
ARC_DEF_TRANS_DOP_2OP_REG_U6(mov)
ARC_DEF_TRANS_DOP_2OP_REG_S12(mov)
ARC_DEF_TRANS_DOP_2OP_COND_REG(mov)
ARC_DEF_TRANS_DOP_2OP_COND_U6(mov)

static bool trans_mov_s_b_h(DisasContext *ctx, arg_mov_s_b_h *a)
{
    TCGv_i32 reg_a = arc_insn_load_dst_reg_s(ctx, a->b);
    TCGv_i32 reg_b = arc_insn_load_src_reg_h(ctx, a->h);
    TCGLabel *skip = gen_new_label();

    arc_gen_cc_prologue(ctx, ARC_CC_NE, skip);
    tcg_gen_mov_i32(reg_a, reg_b);
    gen_set_label(skip);

    arc_insn_save_reg_s(ctx, a->b, reg_a);

    return true;
}

static bool trans_mov_s_h_s3(DisasContext *ctx, arg_mov_s_h_s3 *a)
{
    TCGv_i32 reg_a = arc_insn_load_dst_reg_h(ctx, a->h);
    TCGv_i32 reg_b = arc_insn_load_imm(a->imm);

    tcg_gen_mov_i32(reg_a, reg_b);

    arc_insn_save_reg_h(ctx, a->h, reg_a);

    return true;
}

static bool trans_mov_s_g_h(DisasContext *ctx, arg_mov_s_g_h *a)
{
    TCGv_i32 reg_a = arc_insn_load_dst_reg_h(ctx, a->g);
    TCGv_i32 reg_b = arc_insn_load_src_reg_h(ctx, a->h);

    tcg_gen_mov_i32(reg_a, reg_b);

    arc_insn_save_reg_h(ctx, a->g, reg_a);

    return true;
}

static bool trans_mov_s_b_u8(DisasContext *ctx, arg_mov_s_b_u8 *a)
{
    TCGv_i32 reg_a = arc_insn_load_dst_reg_s(ctx, a->b);
    TCGv_i32 reg_b = arc_insn_load_imm(a->imm);

    tcg_gen_mov_i32(reg_a, reg_b);

    arc_insn_save_reg_s(ctx, a->b, reg_a);

    return true;
}

static inline void arc64_gen_movl(DisasContext *ctx, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    tcg_gen_mov_i64(b, c);

    if (set_flags) {
        arc64_gen_set_flag_Z(b);
        arc64_gen_set_flag_N(b);
    }
}

ARC64_DEF_TRANS_DOP_2OP_REG_REG(movl)
ARC64_DEF_TRANS_DOP_2OP_REG_U6(movl)
ARC64_DEF_TRANS_DOP_2OP_REG_S12(movl)
ARC64_DEF_TRANS_DOP_2OP_COND_REG(movl)
ARC64_DEF_TRANS_DOP_2OP_COND_U6(movl)

#ifdef TARGET_ARCV3_64
static bool trans_movl_s_g_h(DisasContext *ctx, arg_movl_s_g_h *a)
{
    TCGv_i64 reg_a = arc64_insn_load_dst_reg_h(ctx, a->g);
    TCGv_i64 reg_b = arc64_insn_load_src_reg_h(ctx, a->h);

    tcg_gen_mov_i64(reg_a, reg_b);

    arc64_insn_save_reg_h(ctx, a->g, reg_a);

    return true;
}
#else
ARC_DEF_TRANS_STUB(movl_s_g_h)
#endif

static inline void arc64_gen_movhl(DisasContext *ctx, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    tcg_gen_shli_i64(c, c, 32);
    tcg_gen_mov_i64(b, c);
}

ARC64_DEF_TRANS_DOP_2OP_REG_REG(movhl)
ARC64_DEF_TRANS_DOP_2OP_REG_U6(movhl)
ARC64_DEF_TRANS_DOP_2OP_REG_S12(movhl)
ARC64_DEF_TRANS_DOP_2OP_COND_REG(movhl)
ARC64_DEF_TRANS_DOP_2OP_COND_U6(movhl)

#ifdef TARGET_ARCV3_64
static bool trans_movhl_s_h_limm(DisasContext *ctx, arg_movhl_s_h_limm *a)
{
    TCGv_i64 reg_a = arc64_insn_load_dst_reg_h(ctx, a->h);
    TCGv_i64 reg_b = arc64_insn_load_limm_unsigned(ctx);

    arc64_gen_movhl(ctx, reg_a, reg_b, false);

    arc64_insn_save_reg_h(ctx, a->h, reg_a);

    return true;
}
#else
ARC_DEF_TRANS_STUB(movhl_s_h_limm)
#endif
