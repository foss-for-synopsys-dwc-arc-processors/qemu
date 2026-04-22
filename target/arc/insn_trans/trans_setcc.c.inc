#include "translate.h"

static inline void arc_gen_setcc(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, int cond, bool flag_setting)
{
    TCGv_i32 result = tcg_temp_new_i32();
    TCGv_i32 tmp = tcg_temp_new_i32();
    TCGCond tcg_cond;

    switch (cond) {
    case ARC_CC_EQ:
        tcg_cond = TCG_COND_EQ;
        break;
    case ARC_CC_NE:
        tcg_cond = TCG_COND_NE;
        break;
    case ARC_CC_LT:
        tcg_cond = TCG_COND_LT;
        break;
    case ARC_CC_GE:
        tcg_cond = TCG_COND_GE;
        break;
    case ARC_CC_LO:
        tcg_cond = TCG_COND_LTU;
        break;
    case ARC_CC_HS:
        tcg_cond = TCG_COND_GEU;
        break;
    case ARC_CC_LE:
        tcg_cond = TCG_COND_LE;
        break;
    case ARC_CC_GT:
        tcg_cond = TCG_COND_GT;
        break;
    default:
        g_assert_not_reached();
    }

    tcg_gen_setcond_i32(tcg_cond, result, b, c);

    if (flag_setting) {
        tcg_gen_sub_i32(tmp, b, c);
        arc_gen_set_flag_ZNCV_sub(tmp, b, c);
    }

    tcg_gen_mov_i32(a, result);
}

static bool trans_setcc_reg_reg(DisasContext *ctx, arg_setcc_reg_reg *a)
{
    TCGv_i32 reg_a = arc_insn_load_dst_reg(ctx, a->a);
    TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c);

    arc_gen_setcc(ctx, reg_a, reg_b, reg_c, a->op, a->f);
    arc_insn_save_reg(ctx, a->a, reg_a);

    return true;
}

static bool trans_setcc_reg_u6(DisasContext *ctx, arg_setcc_reg_u6 *a)
{
    TCGv_i32 reg_a = arc_insn_load_dst_reg(ctx, a->a);
    TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_c = tcg_constant_i32(a->imm);

    arc_gen_setcc(ctx, reg_a, reg_b, reg_c, a->op, a->f);
    arc_insn_save_reg(ctx, a->a, reg_a);

    return true;
}

static bool trans_setcc_reg_s12(DisasContext *ctx, arg_setcc_reg_s12 *a)
{
    TCGv_i32 reg_a = arc_insn_load_dst_reg(ctx, a->b);
    TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_c = tcg_constant_i32(a->imm);

    arc_gen_setcc(ctx, reg_a, reg_b, reg_c, a->op, a->f);
    arc_insn_save_reg(ctx, a->b, reg_a);

    return true;
}

static bool trans_setcc_cond_reg(DisasContext *ctx, arg_setcc_cond_reg *a)
{
    TCGv_i32 reg_a = arc_insn_load_dst_reg(ctx, a->b);
    TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c);
    TCGLabel *skip = gen_new_label();

    arc_gen_cc_prologue(ctx, a->cond, skip);

    arc_gen_setcc(ctx, reg_a, reg_b, reg_c, a->op, a->f);
    arc_insn_save_reg(ctx, a->b, reg_a);

    gen_set_label(skip);

    return true;
}

static bool trans_setcc_cond_u6(DisasContext *ctx, arg_setcc_cond_u6 *a)
{
    TCGv_i32 reg_a = arc_insn_load_dst_reg(ctx, a->b);
    TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_c = tcg_constant_i32(a->imm);
    TCGLabel *skip = gen_new_label();

    arc_gen_cc_prologue(ctx, a->cond, skip);

    arc_gen_setcc(ctx, reg_a, reg_b, reg_c, a->op, a->f);
    arc_insn_save_reg(ctx, a->b, reg_a);

    gen_set_label(skip);

    return true;
}

static inline void arc64_gen_setccl(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, int cond, bool flag_setting)
{
    TCGv_i64 result = tcg_temp_new_i64();
    TCGv_i64 tmp = tcg_temp_new_i64();
    TCGCond tcg_cond;

    switch (cond) {
    case ARC_CC_EQ:
        tcg_cond = TCG_COND_EQ;
        break;
    case ARC_CC_NE:
        tcg_cond = TCG_COND_NE;
        break;
    case ARC_CC_LT:
        tcg_cond = TCG_COND_LT;
        break;
    case ARC_CC_GE:
        tcg_cond = TCG_COND_GE;
        break;
    case ARC_CC_LO:
        tcg_cond = TCG_COND_LTU;
        break;
    case ARC_CC_HS:
        tcg_cond = TCG_COND_GEU;
        break;
    case ARC_CC_LE:
        tcg_cond = TCG_COND_LE;
        break;
    case ARC_CC_GT:
        tcg_cond = TCG_COND_GT;
        break;
    default:
        g_assert_not_reached();
    }

    tcg_gen_setcond_i64(tcg_cond, result, b, c);

    if (flag_setting) {
        tcg_gen_sub_i64(tmp, b, c);
        arc64_gen_set_flag_ZNCV_sub(tmp, b, c);
    }

    tcg_gen_mov_i64(a, result);
}

#ifdef TARGET_ARCV3_64
static bool trans_setccl_reg_reg(DisasContext *ctx, arg_setccl_reg_reg *a)
{
    TCGv_i64 reg_a = arc64_insn_load_dst_reg(ctx, a->a);
    TCGv_i64 reg_b = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_src_reg(ctx, a->c);

    arc64_gen_setccl(ctx, reg_a, reg_b, reg_c, a->op, a->f);
    arc64_insn_save_reg(ctx, a->a, reg_a);

    return true;
}

static bool trans_setccl_reg_u6(DisasContext *ctx, arg_setccl_reg_u6 *a)
{
    TCGv_i64 reg_a = arc64_insn_load_dst_reg(ctx, a->a);
    TCGv_i64 reg_b = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_c = tcg_constant_i64(a->imm);

    arc64_gen_setccl(ctx, reg_a, reg_b, reg_c, a->op, a->f);
    arc64_insn_save_reg(ctx, a->a, reg_a);

    return true;
}

static bool trans_setccl_reg_s12(DisasContext *ctx, arg_setccl_reg_s12 *a)
{
    TCGv_i64 reg_a = arc64_insn_load_dst_reg(ctx, a->b);
    TCGv_i64 reg_b = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_c = tcg_constant_i64(a->imm);

    arc64_gen_setccl(ctx, reg_a, reg_b, reg_c, a->op, a->f);
    arc64_insn_save_reg(ctx, a->b, reg_a);

    return true;
}

static bool trans_setccl_cond_reg(DisasContext *ctx, arg_setccl_cond_reg *a)
{
    TCGv_i64 reg_a = arc64_insn_load_dst_reg(ctx, a->b);
    TCGv_i64 reg_b = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_src_reg(ctx, a->c);
    TCGLabel *skip = gen_new_label();

    arc_gen_cc_prologue(ctx, a->cond, skip);

    arc64_gen_setccl(ctx, reg_a, reg_b, reg_c, a->op, a->f);
    arc64_insn_save_reg(ctx, a->b, reg_a);

    gen_set_label(skip);

    return true;
}

static bool trans_setccl_cond_u6(DisasContext *ctx, arg_setccl_cond_u6 *a)
{
    TCGv_i64 reg_a = arc64_insn_load_dst_reg(ctx, a->b);
    TCGv_i64 reg_b = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_c = tcg_constant_i64(a->imm);
    TCGLabel *skip = gen_new_label();

    arc_gen_cc_prologue(ctx, a->cond, skip);

    arc64_gen_setccl(ctx, reg_a, reg_b, reg_c, a->op, a->f);
    arc64_insn_save_reg(ctx, a->b, reg_a);

    gen_set_label(skip);

    return true;
}
#else
ARC_DEF_TRANS_STUB(setccl_reg_reg)
ARC_DEF_TRANS_STUB(setccl_reg_u6)
ARC_DEF_TRANS_STUB(setccl_reg_s12)
ARC_DEF_TRANS_STUB(setccl_cond_reg)
ARC_DEF_TRANS_STUB(setccl_cond_u6)
#endif