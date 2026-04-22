#include "translate.h"

static inline void arc_gen_sub(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_sub_i32(a, b, c);

    if (set_flags) {
        arc_gen_set_flag_ZNCV_sub(a, b, c);
    }
}

ARC_DEF_TRANS_DOP_REG_REG(sub)
ARC_DEF_TRANS_DOP_REG_U6(sub)
ARC_DEF_TRANS_DOP_REG_S12(sub)
ARC_DEF_TRANS_DOP_COND_REG(sub)
ARC_DEF_TRANS_DOP_COND_U6(sub)

ARC_DEF_TRANS_RS32_RS32_RS32_F(sub, sub_s_b_b_c, a->b, a->b, a->c, false)
ARC_DEF_TRANS_RS32_RS32_I32_F(sub, sub_s_b_b_u5, a->b, a->b, a->imm, false)
ARC_DEF_TRANS_R32_R32_I32_F(sub, sub_s_sp_sp_u7, ARC_REGNUM_SP, ARC_REGNUM_SP, a->imm, false)
ARC_DEF_TRANS_RS32_RS32_RS32_F(sub, sub_s_a_b_c, a->a, a->b, a->c, false);

#ifdef TARGET_ARCV2
ARC_DEF_TRANS_RS32_RS32_I32_F(sub, sub_s_c_b_u3, a->c, a->b, a->imm, false)
#else
ARC_DEF_TRANS_STUB(sub_s_c_b_u3)
#endif

static bool trans_sub_s_ne_b_b_b(DisasContext *ctx, arg_sub_s_ne_b_b_b *a)
{
    TCGv_i32 reg_a = arc_insn_load_src_reg_s(ctx, a->b);
    TCGv_i32 reg_b = arc_insn_load_src_reg_s(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_src_reg_s(ctx, a->b);
    TCGLabel *skip = gen_new_label();

    arc_gen_cc_prologue(ctx, ARC_CC_NE, skip);
    arc_gen_sub(ctx, reg_a, reg_b, reg_c, false);
    arc_insn_save_reg_s(ctx, a->b, reg_a);
    gen_set_label(skip);

    return true;
}

static inline void arc64_gen_subl(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    tcg_gen_sub_i64(a, b, c);

    if (set_flags) {
        arc64_gen_set_flag_ZNCV_sub(a, b, c);
    }
}

ARC64_DEF_TRANS_DOP_REG_REG(subl)
ARC64_DEF_TRANS_DOP_REG_U6(subl)
ARC64_DEF_TRANS_DOP_REG_S12(subl)
ARC64_DEF_TRANS_DOP_COND_REG(subl)
ARC64_DEF_TRANS_DOP_COND_U6(subl)
ARC64_DEF_TRANS_RS64_RS64_RS64_F(subl, subl_s_b_b_c, a->b, a->b, a->c, false);
ARC64_DEF_TRANS_R64_R64_I64_F(subl, subl_s_sp_sp_u9, ARC_REGNUM_SP,ARC_REGNUM_SP, a->imm, false)

static inline void arc_gen_rsub(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    arc_gen_sub(ctx, a, c, b, set_flags);
}

ARC_DEF_TRANS_DOP_REG_REG(rsub)
ARC_DEF_TRANS_DOP_REG_U6(rsub)
ARC_DEF_TRANS_DOP_REG_S12(rsub)
ARC_DEF_TRANS_DOP_COND_REG(rsub)
ARC_DEF_TRANS_DOP_COND_U6(rsub)

static inline void arc64_gen_rsubl(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    arc64_gen_subl(ctx, a, c, b, set_flags);
}

ARC64_DEF_TRANS_DOP_REG_REG(rsubl)
ARC64_DEF_TRANS_DOP_REG_U6(rsubl)
ARC64_DEF_TRANS_DOP_REG_S12(rsubl)
ARC64_DEF_TRANS_DOP_COND_REG(rsubl)
ARC64_DEF_TRANS_DOP_COND_U6(rsubl)

static inline void arc_gen_sub_shifted(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags, int shift)
{
    tcg_gen_shli_i32(c, c, shift);
    tcg_gen_sub_i32(a, b, c);

    if (set_flags) {
        arc_gen_set_flag_ZNCV_sub(a, b, c);
    }
}

static inline void arc64_gen_subl_shifted(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags, int shift)
{
    tcg_gen_shli_i64(c, c, shift);
    tcg_gen_sub_i64(a, b, c);

    if (set_flags) {
        arc64_gen_set_flag_ZNCV_sub(a, b, c);
    }
}

static inline void arc_gen_sub1(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    arc_gen_sub_shifted(ctx, a, b, c, set_flags, 1);
}

ARC_DEF_TRANS_DOP_REG_REG(sub1)
ARC_DEF_TRANS_DOP_REG_U6(sub1)
ARC_DEF_TRANS_DOP_REG_S12(sub1)
ARC_DEF_TRANS_DOP_COND_REG(sub1)
ARC_DEF_TRANS_DOP_COND_U6(sub1)

static inline void arc64_gen_sub1l(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    arc64_gen_subl_shifted(ctx, a, b, c, set_flags, 1);
}

ARC64_DEF_TRANS_DOP_REG_REG(sub1l)
ARC64_DEF_TRANS_DOP_REG_U6(sub1l)
ARC64_DEF_TRANS_DOP_REG_S12(sub1l)
ARC64_DEF_TRANS_DOP_COND_REG(sub1l)
ARC64_DEF_TRANS_DOP_COND_U6(sub1l)

static inline void arc_gen_sub2(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    arc_gen_sub_shifted(ctx, a, b, c, set_flags, 2);
}

ARC_DEF_TRANS_DOP_REG_REG(sub2)
ARC_DEF_TRANS_DOP_REG_U6(sub2)
ARC_DEF_TRANS_DOP_REG_S12(sub2)
ARC_DEF_TRANS_DOP_COND_REG(sub2)
ARC_DEF_TRANS_DOP_COND_U6(sub2)

static inline void arc64_gen_sub2l(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    arc64_gen_subl_shifted(ctx, a, b, c, set_flags, 2);
}

ARC64_DEF_TRANS_DOP_REG_REG(sub2l)
ARC64_DEF_TRANS_DOP_REG_U6(sub2l)
ARC64_DEF_TRANS_DOP_REG_S12(sub2l)
ARC64_DEF_TRANS_DOP_COND_REG(sub2l)
ARC64_DEF_TRANS_DOP_COND_U6(sub2l)

static inline void arc_gen_sub3(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    arc_gen_sub_shifted(ctx, a, b, c, set_flags, 3);
}

ARC_DEF_TRANS_DOP_REG_REG(sub3)
ARC_DEF_TRANS_DOP_REG_U6(sub3)
ARC_DEF_TRANS_DOP_REG_S12(sub3)
ARC_DEF_TRANS_DOP_COND_REG(sub3)
ARC_DEF_TRANS_DOP_COND_U6(sub3)

static inline void arc64_gen_sub3l(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    arc64_gen_subl_shifted(ctx, a, b, c, set_flags, 3);
}

ARC64_DEF_TRANS_DOP_REG_REG(sub3l)
ARC64_DEF_TRANS_DOP_REG_U6(sub3l)
ARC64_DEF_TRANS_DOP_REG_S12(sub3l)
ARC64_DEF_TRANS_DOP_COND_REG(sub3l)
ARC64_DEF_TRANS_DOP_COND_U6(sub3l)

static inline void arc_gen_sbc(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_sub_i32(a, b, c);
    tcg_gen_sub_i32(a, a, cpu_Cf);

    if (set_flags) {
        arc_gen_set_flag_ZNCV_sub(a, b, c);
    }
}

ARC_DEF_TRANS_DOP_REG_REG(sbc)
ARC_DEF_TRANS_DOP_REG_U6(sbc)
ARC_DEF_TRANS_DOP_REG_S12(sbc)
ARC_DEF_TRANS_DOP_COND_REG(sbc)
ARC_DEF_TRANS_DOP_COND_U6(sbc)

static inline void arc64_gen_sbcl(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 tmp = tcg_temp_new_i64();

    tcg_gen_sub_i64(a, b, c);
    tcg_gen_extu_i32_i64(tmp, cpu_Cf);
    tcg_gen_sub_i64(a, a, tmp);

    if (set_flags) {
        arc64_gen_set_flag_ZNCV_sub(a, b, c);
    }
}

ARC64_DEF_TRANS_DOP_REG_REG(sbcl)
ARC64_DEF_TRANS_DOP_REG_U6(sbcl)
ARC64_DEF_TRANS_DOP_REG_S12(sbcl)
ARC64_DEF_TRANS_DOP_COND_REG(sbcl)
ARC64_DEF_TRANS_DOP_COND_U6(sbcl)

/*
 * if (cc) a = 0 - b
 *
 * if cc==true then
 *   dest = 0 - src1
 *   if F==1 then
 *     Z_flag = if dest==0 then 1 else 0
 *     N_flag = dest[31]
 *     C_flag = Carry()
 *     V_flag = Overflow()
 */
static bool trans_neg(DisasContext *ctx, arg_neg *a)
{
    TCGv_i32 reg_a = arc_insn_load_dst_reg(ctx, a->a);
    TCGv_i32 reg_b = tcg_constant_i32(0);
    TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->b);

    arc_gen_sub(ctx, reg_a, reg_b, reg_c, a->f);
    arc_insn_save_reg(ctx, a->a, reg_a);

    return true;
}

static bool trans_neg_cond(DisasContext *ctx, arg_neg_cond *a)
{
    TCGv_i32 reg_a = arc_insn_load_dst_reg(ctx, a->b);
    TCGv_i32 reg_b = tcg_constant_i32(0);
    TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->b);
    TCGLabel *skip = gen_new_label();

    arc_gen_cc_prologue(ctx, a->cond, skip);
    arc_gen_sub(ctx, reg_a, reg_b, reg_c, a->f);
    arc_insn_save_reg(ctx, a->b, reg_a);
    gen_set_label(skip);

    return true;
}

static bool trans_neg_s_dop(DisasContext *ctx, arg_neg_s_dop *a)
{
    TCGv_i32 reg_a = arc_insn_load_dst_reg_s(ctx, a->b);
    TCGv_i32 reg_b = tcg_constant_i32(0);
    TCGv_i32 reg_c = arc_insn_load_src_reg_s(ctx, a->c);

    arc_gen_sub(ctx, reg_a, reg_b, reg_c, false);
    arc_insn_save_reg_s(ctx, a->b, reg_a);

    return true;
}
