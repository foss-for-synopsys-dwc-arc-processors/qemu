#include "translate.h"

/* Support functions for MAC/MPY instructions */

static inline void arc_gen_single_mul_32x32(TCGv_i64 ret, TCGv_i32 a, TCGv_i32 b, bool sign)
{
    TCGv_i32 prod_low = tcg_temp_new_i32();
    TCGv_i32 prod_high = tcg_temp_new_i32();

    if (sign) {
        tcg_gen_muls2_i32(prod_low, prod_high, a, b);
    } else {
        tcg_gen_mulu2_i32(prod_low, prod_high, a, b);
    }

    tcg_gen_concat_i32_i64(ret, prod_low, prod_high);
}

static inline void arc_gen_single_mul_32x32_signed(TCGv_i64 ret, TCGv_i32 a, TCGv_i32 b)
{
    arc_gen_single_mul_32x32(ret, a, b, true);
}

static inline void arc_gen_single_mul_32x32_unsigned(TCGv_i64 ret, TCGv_i32 a, TCGv_i32 b)
{
    arc_gen_single_mul_32x32(ret, a, b, false);
}

static inline void arc_gen_single_mul_16x16(TCGv_i32 ret, TCGv_i32 a, TCGv_i32 b, bool sign)
{
    TCGv_i32 a_low = tcg_temp_new_i32();
    TCGv_i32 b_low = tcg_temp_new_i32();

    if (sign) {
        tcg_gen_sextract_i32(a_low, a, 0, 16);
        tcg_gen_sextract_i32(b_low, b, 0, 16);
    } else {
        tcg_gen_extract_i32(a_low, a, 0, 16);
        tcg_gen_extract_i32(b_low, b, 0, 16);
    }

    tcg_gen_mul_i32(ret, a_low, b_low);
}

static inline void arc_gen_single_mul_16x16_signed(TCGv_i32 ret, TCGv_i32 a, TCGv_i32 b)
{
    arc_gen_single_mul_16x16(ret, a, b, true);
}

static inline void arc_gen_single_mul_16x16_unsigned(TCGv_i32 ret, TCGv_i32 a, TCGv_i32 b)
{
    arc_gen_single_mul_16x16(ret, a, b, false);
}

static inline void arc_gen_dual_mul_16x16(TCGv_i64 ret, TCGv_i32 a, TCGv_i32 b, bool sign)
{
    TCGv_i64 tmp = tcg_temp_new_i64();
    TCGv_i64 a_low = tcg_temp_new_i64();
    TCGv_i64 a_high = tcg_temp_new_i64();
    TCGv_i64 b_low = tcg_temp_new_i64();
    TCGv_i64 b_high = tcg_temp_new_i64();

    if (sign) {
        tcg_gen_extu_i32_i64(tmp, a);
        tcg_gen_sextract_i64(a_low, tmp, 0, 16);
        tcg_gen_sextract_i64(a_high, tmp, 16, 16);

        tcg_gen_extu_i32_i64(tmp, b);
        tcg_gen_sextract_i64(b_low, tmp, 0, 16);
        tcg_gen_sextract_i64(b_high, tmp, 16, 16);
    } else {
        tcg_gen_extu_i32_i64(tmp, a);
        tcg_gen_extract_i64(a_low, tmp, 0, 16);
        tcg_gen_extract_i64(a_high, tmp, 16, 16);

        tcg_gen_extu_i32_i64(tmp, b);
        tcg_gen_extract_i64(b_low, tmp, 0, 16);
        tcg_gen_extract_i64(b_high, tmp, 16, 16);
    }

    tcg_gen_mul_i64(ret, a_low, b_low); /* Products of lows */
    tcg_gen_mul_i64(tmp, a_high, b_high); /* Product of highs */
    tcg_gen_add_i64(ret, ret, tmp); /* Sum of products*/
}

static inline void arc_gen_dual_mul_16x16_signed(TCGv_i64 ret, TCGv_i32 a, TCGv_i32 b)
{
    arc_gen_dual_mul_16x16(ret, a, b, true);
}

static inline void arc_gen_dual_mul_16x16_unsigned(TCGv_i64 ret, TCGv_i32 a, TCGv_i32 b)
{
    arc_gen_dual_mul_16x16(ret, a, b, false);
}

static inline void arc_gen_quad_mul_16x16(TCGv_i64 ret, TCGv_i64 a, TCGv_i64 b, bool sign)
{
    TCGv_i64 tmp = tcg_temp_new_i64();
    TCGv_i64 a_h0 = tcg_temp_new_i64();
    TCGv_i64 a_h1 = tcg_temp_new_i64();
    TCGv_i64 a_h2 = tcg_temp_new_i64();
    TCGv_i64 a_h3 = tcg_temp_new_i64();
    TCGv_i64 b_h0 = tcg_temp_new_i64();
    TCGv_i64 b_h1 = tcg_temp_new_i64();
    TCGv_i64 b_h2 = tcg_temp_new_i64();
    TCGv_i64 b_h3 = tcg_temp_new_i64();

    if (sign) {
        tcg_gen_sextract_i64(a_h0, a, 0, 16);
        tcg_gen_sextract_i64(a_h1, a, 16, 16);
        tcg_gen_sextract_i64(a_h2, a, 32, 16);
        tcg_gen_sextract_i64(a_h3, a, 48, 16);
        tcg_gen_sextract_i64(b_h0, b, 0, 16);
        tcg_gen_sextract_i64(b_h1, b, 16, 16);
        tcg_gen_sextract_i64(b_h2, b, 32, 16);
        tcg_gen_sextract_i64(b_h3, b, 48, 16);
    } else {
        tcg_gen_extract_i64(a_h0, a, 0, 16);
        tcg_gen_extract_i64(a_h1, a, 16, 16);
        tcg_gen_extract_i64(a_h2, a, 32, 16);
        tcg_gen_extract_i64(a_h3, a, 48, 16);
        tcg_gen_extract_i64(b_h0, b, 0, 16);
        tcg_gen_extract_i64(b_h1, b, 16, 16);
        tcg_gen_extract_i64(b_h2, b, 32, 16);
        tcg_gen_extract_i64(b_h3, b, 48, 16);
    }

    tcg_gen_mul_i64(ret, a_h0, b_h0);
    tcg_gen_mul_i64(tmp, a_h1, b_h1);
    tcg_gen_add_i64(ret, ret, tmp);
    tcg_gen_mul_i64(tmp, a_h2, b_h2);
    tcg_gen_add_i64(ret, ret, tmp);
    tcg_gen_mul_i64(tmp, a_h3, b_h3);
    tcg_gen_add_i64(ret, ret, tmp);
}

static inline void arc_gen_quad_mul_16x16_signed(TCGv_i64 ret, TCGv_i64 a, TCGv_i64 b)
{
    arc_gen_quad_mul_16x16(ret, a, b, true);
}

static inline void arc_gen_quad_mul_16x16_unsigned(TCGv_i64 ret, TCGv_i64 a, TCGv_i64 b)
{
    arc_gen_quad_mul_16x16(ret, a, b, false);
}

static inline void arc_gen_dual_mul_32x16(TCGv_i64 ret, TCGv_i64 a, TCGv_i32 b, bool sign)
{
    TCGv_i64 a_low = tcg_temp_new_i64();
    TCGv_i64 a_high = tcg_temp_new_i64();
    TCGv_i64 b_low = tcg_temp_new_i64();
    TCGv_i64 b_high = tcg_temp_new_i64();
    TCGv_i64 tmp = tcg_temp_new_i64();

    if (sign) {
        tcg_gen_sextract_i64(a_low, a, 0, 32);
        tcg_gen_sextract_i64(a_high, a, 32, 32);

        tcg_gen_concat_i32_i64(tmp, b, b);
        tcg_gen_sextract_i64(b_low, tmp, 0, 16);
        tcg_gen_sextract_i64(b_high, tmp, 16, 16);
    } else {
        tcg_gen_extract_i64(a_low, a, 0, 32);
        tcg_gen_extract_i64(a_high, a, 32, 32);

        tcg_gen_concat_i32_i64(tmp, b, b);
        tcg_gen_extract_i64(b_low, tmp, 0, 16);
        tcg_gen_extract_i64(b_high, tmp, 16, 16);
    }

    tcg_gen_mul_i64(ret, a_low, b_low);
    tcg_gen_mul_i64(tmp, a_high, b_high);
    tcg_gen_add_i64(ret, ret, tmp);
}

static inline void arc_gen_dual_mul_32x16_signed(TCGv_i64 ret, TCGv_i64 a, TCGv_i32 b)
{
    arc_gen_dual_mul_32x16(ret, a, b, true);
}

static inline void arc_gen_dual_mul_32x16_unsigned(TCGv_i64 ret, TCGv_i64 a, TCGv_i32 b)
{
    arc_gen_dual_mul_32x16(ret, a, b, false);
}

/* Translator macros for MPY/MAC instructions */

#define ARC_GEN_TRANS_MPY_DOP_REG_REG(name) \
    static bool trans_##name##_dop_reg_reg(DisasContext *ctx, arg_##name##_dop_reg_reg *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->a); \
        TCGv_i32 reg_a = arc_insn_load_dst_reg(ctx, a->a); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c); \
        arc_gen_##name(ctx, reg_a, reg_b, reg_c, a->f); \
        arc_insn_save_reg(ctx, a->a, reg_a); \
        return true; \
    }

#define ARC_GEN_TRANS_MPY_DOP_REG_U6(name) \
    static bool trans_##name##_dop_reg_u6(DisasContext *ctx, arg_##name##_dop_reg_u6 *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->a); \
        TCGv_i32 reg_a = arc_insn_load_dst_reg(ctx, a->a); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, reg_a, reg_b, reg_c, a->f); \
        arc_insn_save_reg(ctx, a->a, reg_a); \
        return true; \
    }

#define ARC_GEN_TRANS_MPY_DOP_REG_S12(name) \
    static bool trans_##name##_dop_reg_s12(DisasContext *ctx, arg_##name##_dop_reg_s12 *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->b); \
        TCGv_i32 reg_a = arc_insn_load_dst_reg(ctx, a->b); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, reg_a, reg_b, reg_c, a->f); \
        arc_insn_save_reg(ctx, a->b, reg_a); \
        return true; \
    }

#define ARC_GEN_TRANS_MPY_DOP_COND_REG(name) \
    static bool trans_##name##_dop_cond_reg(DisasContext *ctx, arg_##name##_dop_cond_reg *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->b); \
        TCGv_i32 reg_a = arc_insn_load_dst_reg(ctx, a->b); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, reg_a, reg_b, reg_c, a->f); \
        arc_insn_save_reg(ctx, a->b, reg_a); \
        gen_set_label(skip); \
        return true; \
    }

#define ARC_GEN_TRANS_MPY_DOP_COND_U6(name) \
    static bool trans_##name##_dop_cond_u6(DisasContext *ctx, arg_##name##_dop_cond_u6 *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->b); \
        TCGv_i32 reg_a = arc_insn_load_dst_reg(ctx, a->b); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_imm(a->imm); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, reg_a, reg_b, reg_c, a->f); \
        arc_insn_save_reg(ctx, a->b, reg_a); \
        gen_set_label(skip); \
        return true; \
    }

#define ARC_GEN_TRANS_MPY_S_DOP(name) \
    static bool trans_##name##_s_dop(DisasContext *ctx, arg_##name##_s_dop *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->b); \
        TCGv_i32 reg_a = arc_insn_load_dst_reg_s(ctx, a->b); \
        TCGv_i32 reg_b = arc_insn_load_src_reg_s(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_src_reg_s(ctx, a->c); \
        arc_gen_##name(ctx, reg_a, reg_b, reg_c, false); \
        arc_insn_save_reg_s(ctx, a->b, reg_a); \
        return true; \
    }

/* Translator macros for MPYD/MACD instructions */

#define ARC_GEN_TRANS_MPYD_DOP_REG_REG(name) \
    static bool trans_##name##_dop_reg_reg(DisasContext *ctx, arg_##name##_dop_reg_reg *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->a); \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->a); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c); \
        arc_gen_##name(ctx, reg_a_pair, reg_b, reg_c, a->f); \
        arc_insn_save_reg_pair(ctx, a->a, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_MPYD_DOP_REG_U6(name) \
    static bool trans_##name##_dop_reg_u6(DisasContext *ctx, arg_##name##_dop_reg_u6 *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->a); \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->a); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, reg_a_pair, reg_b, reg_c, a->f); \
        arc_insn_save_reg_pair(ctx, a->a, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_MPYD_DOP_REG_S12(name) \
    static bool trans_##name##_dop_reg_s12(DisasContext *ctx, arg_##name##_dop_reg_s12 *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->b); \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, reg_a_pair, reg_b, reg_c, a->f); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_MPYD_DOP_COND_REG(name) \
    static bool trans_##name##_dop_cond_reg(DisasContext *ctx, arg_##name##_dop_cond_reg *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->b); \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, reg_a_pair, reg_b, reg_c, a->f); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        gen_set_label(skip); \
        return true; \
    }

#define ARC_GEN_TRANS_MPYD_DOP_COND_U6(name) \
    static bool trans_##name##_dop_cond_u6(DisasContext *ctx, arg_##name##_dop_cond_u6 *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->b); \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_imm(a->imm); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, reg_a_pair, reg_b, reg_c, a->f); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        gen_set_label(skip); \
        return true; \
    }

/* Translator macros for DMPYWH/DMACWH instructions */

#define ARC_GEN_TRANS_DMPYWH_DOP_REG_REG(name) \
    static bool trans_##name##_dop_reg_reg(DisasContext *ctx, arg_##name##_dop_reg_reg *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->a); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c, a->f); \
        arc_insn_save_reg_pair(ctx, a->a, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_DMPYWH_DOP_REG_U6(name) \
    static bool trans_##name##_dop_reg_u6(DisasContext *ctx, arg_##name##_dop_reg_u6 *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->a); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i32 reg_c = arc_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c, a->f); \
        arc_insn_save_reg_pair(ctx, a->a, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_DMPYWH_DOP_REG_S12(name) \
    static bool trans_##name##_dop_reg_s12(DisasContext *ctx, arg_##name##_dop_reg_s12 *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i32 reg_c = arc_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c, a->f); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_DMPYWH_DOP_COND_REG(name) \
    static bool trans_##name##_dop_cond_reg(DisasContext *ctx, arg_##name##_dop_cond_reg *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c, a->f); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        gen_set_label(skip); \
        return true; \
    }

#define ARC_GEN_TRANS_DMPYWH_DOP_COND_U6(name) \
    static bool trans_##name##_dop_cond_u6(DisasContext *ctx, arg_##name##_dop_cond_u6 *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i32 reg_c = arc_insn_load_imm(a->imm); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c, a->f); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        gen_set_label(skip); \
        return true; \
    }

/* Translator macros for QMPYH/QMACH instructions */

#define ARC_GEN_TRANS_QMPYH_DOP_REG_REG(name) \
    static bool trans_##name##_dop_reg_reg(DisasContext *ctx, arg_##name##_dop_reg_reg *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->a); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i64 reg_c_pair = arc_insn_load_src_reg_pair(ctx, a->c, ARC_REGPAIR_LIMM_DUPLICATE); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c_pair, a->f); \
        arc_insn_save_reg_pair(ctx, a->a, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_QMPYH_DOP_REG_U6(name) \
    static bool trans_##name##_dop_reg_u6(DisasContext *ctx, arg_##name##_dop_reg_u6 *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->a); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i64 reg_c_pair = arc64_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c_pair, a->f); \
        arc_insn_save_reg_pair(ctx, a->a, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_QMPYH_DOP_REG_S12(name) \
    static bool trans_##name##_dop_reg_s12(DisasContext *ctx, arg_##name##_dop_reg_s12 *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i64 reg_c_pair = arc64_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c_pair, a->f); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_QMPYH_DOP_COND_REG(name) \
    static bool trans_##name##_dop_cond_reg(DisasContext *ctx, arg_##name##_dop_cond_reg *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i64 reg_c_pair = arc_insn_load_src_reg_pair(ctx, a->c, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c_pair, a->f); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        gen_set_label(skip); \
        return true; \
    }

#define ARC_GEN_TRANS_QMPYH_DOP_COND_U6(name) \
    static bool trans_##name##_dop_cond_u6(DisasContext *ctx, arg_##name##_dop_cond_u6 *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i64 reg_c_pair = arc64_insn_load_imm(a->imm); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c_pair, a->f); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        gen_set_label(skip); \
        return true; \
    }

/*
 * MPY, 32 x 32 signed multiply, returning low 32-bit word of the result
 *
 * T = b * c
 * a = T.w0
 *
 * Z - Set when the destination register is zero
 * N - Set when the sign bit of the 64-bit result is set
 * C - Unchanged
 * V - Set when bits 63:32 are not equal to bit 31, the sign bit
 */
static void arc_gen_mpy(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i32 low = tcg_temp_new_i32();
    TCGv_i32 high = tcg_temp_new_i32();
    TCGv_i32 sign = tcg_temp_new_i32();

    tcg_gen_muls2_i32(low, high, b, c);
    tcg_gen_mov_i32(a, low);

    if (set_flags) {
        arc_gen_set_flag_Z(a);
        arc_gen_set_flag_N(high);
        tcg_gen_sari_i32(sign, low, 31);
        tcg_gen_setcond_i32(TCG_COND_NE, cpu_Vf, high, sign);
    }
}

ARC_GEN_TRANS_MPY_DOP_REG_REG(mpy);
ARC_GEN_TRANS_MPY_DOP_REG_U6(mpy);
ARC_GEN_TRANS_MPY_DOP_REG_S12(mpy);
ARC_GEN_TRANS_MPY_DOP_COND_REG(mpy);
ARC_GEN_TRANS_MPY_DOP_COND_U6(mpy);
ARC_GEN_TRANS_MPY_S_DOP(mpy);

/*
 * MPYU, 32 x 32 unsigned multiply, returning low 32-bit word of the result
 *
 * T = b * c
 * a = T.w0
 *
 * Z - Set when the destination register is zero
 * N - Cleared
 * C - Unchanged
 * V - Set when the result cannot be represented as a 32-bit unsigned integer
 */
static void arc_gen_mpyu(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i32 low = tcg_temp_new_i32();
    TCGv_i32 high = tcg_temp_new_i32();

    tcg_gen_mulu2_i32(low, high, b, c);
    tcg_gen_mov_i32(a, low);

    if (set_flags) {
        arc_gen_set_flag_Z(a);
        arc_gen_set_flag_N_clear();
        tcg_gen_setcondi_i32(TCG_COND_NE, cpu_Vf, high, 0);
    }
}

ARC_GEN_TRANS_MPY_DOP_REG_REG(mpyu);
ARC_GEN_TRANS_MPY_DOP_REG_U6(mpyu);
ARC_GEN_TRANS_MPY_DOP_REG_S12(mpyu);
ARC_GEN_TRANS_MPY_DOP_COND_REG(mpyu);
ARC_GEN_TRANS_MPY_DOP_COND_U6(mpyu);

/*
 * MPYM, 32 x 32 signed multiply, returning high 32-bit word of the result
 *
 * T = b * c
 * a = T.w1
 *
 * Z - Set when the destination register is zero
 * N - Set if the result is negative
 * C - Unchanged
 * V - Cleared
 */
static void arc_gen_mpym(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i32 low = tcg_temp_new_i32();
    TCGv_i32 high = tcg_temp_new_i32();

    tcg_gen_muls2_i32(low, high, b, c);
    tcg_gen_mov_i32(a, high);

    if (set_flags) {
        arc_gen_set_flag_Z(a);
        arc_gen_set_flag_N(a);
        arc_gen_set_flag_V_clear();
    }
}

ARC_GEN_TRANS_MPY_DOP_REG_REG(mpym);
ARC_GEN_TRANS_MPY_DOP_REG_U6(mpym);
ARC_GEN_TRANS_MPY_DOP_REG_S12(mpym);
ARC_GEN_TRANS_MPY_DOP_COND_REG(mpym);
ARC_GEN_TRANS_MPY_DOP_COND_U6(mpym);

/*
 * MPYMU, 32 x 32 unsigned multiply, returning high 32-bit word of the result
 *
 * T = b * c
 * a = T.w1
 *
 * Z - Set when the destination register is zero
 * N - Cleared
 * C - Unchanged
 * V - Cleared
 */
static void arc_gen_mpymu(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i32 low = tcg_temp_new_i32();
    TCGv_i32 high = tcg_temp_new_i32();

    tcg_gen_mulu2_i32(low, high, b, c);
    tcg_gen_mov_i32(a, high);

    if (set_flags) {
        arc_gen_set_flag_Z(a);
        arc_gen_set_flag_N_clear();
        arc_gen_set_flag_V_clear();
    }
}

ARC_GEN_TRANS_MPY_DOP_REG_REG(mpymu);
ARC_GEN_TRANS_MPY_DOP_REG_U6(mpymu);
ARC_GEN_TRANS_MPY_DOP_REG_S12(mpymu);
ARC_GEN_TRANS_MPY_DOP_COND_REG(mpymu);
ARC_GEN_TRANS_MPY_DOP_COND_U6(mpymu);

/*
 * MPYW, 16 x 16 signed multiply
 *
 * a = b.h0 * c.h0
 *
 * Z - Set when the destination register is zero
 * N - Set when the sign bit of the 32-bit result is set
 * C - Unchanged
 * V - Cleared
 */
static void arc_gen_mpyw(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    arc_gen_single_mul_16x16_signed(a, b, c);

    if (set_flags) {
        arc_gen_set_flag_Z(a);
        arc_gen_set_flag_N(a);
        arc_gen_set_flag_V_clear();
    }
}

ARC_GEN_TRANS_MPY_DOP_REG_REG(mpyw);
ARC_GEN_TRANS_MPY_DOP_REG_U6(mpyw);
ARC_GEN_TRANS_MPY_DOP_REG_S12(mpyw);
ARC_GEN_TRANS_MPY_DOP_COND_REG(mpyw);
ARC_GEN_TRANS_MPY_DOP_COND_U6(mpyw);
ARC_GEN_TRANS_MPY_S_DOP(mpyw);

/*
 * MPYUW 16 x 16 unsigned multiply
 *
 * a = b.h0 * c.h0
 *
 * Z - Set when the destination register is zero
 * N - Cleared
 * C - Unchanged
 * V - Cleared
 */
static void arc_gen_mpyuw(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    arc_gen_single_mul_16x16_unsigned(a, b, c);

    if (set_flags) {
        arc_gen_set_flag_Z(a);
        arc_gen_set_flag_N_clear();
        arc_gen_set_flag_V_clear();
    }
}

ARC_GEN_TRANS_MPY_DOP_REG_REG(mpyuw);
ARC_GEN_TRANS_MPY_DOP_REG_U6(mpyuw);
ARC_GEN_TRANS_MPY_DOP_REG_S12(mpyuw);
ARC_GEN_TRANS_MPY_DOP_COND_REG(mpyuw);
ARC_GEN_TRANS_MPY_DOP_COND_U6(mpyuw);
ARC_GEN_TRANS_MPY_S_DOP(mpyuw);

/*
 * MPYD, 32 x 32 signed multiply, returning a 64-bit result
 *
 * A = b * c
 * ACC = A
 *
 * Z - Unchanged
 * N - Set to the resulting sign of the accumulator
 * C - Unchanged
 * V - Cleared
 */
static void arc_gen_mpyd(DisasContext *ctx, TCGv_i64 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    arc_gen_single_mul_32x32_signed(a, b, c);
    arc_gen_save_accumulator(a);

    if (set_flags) {
        arc_gen_set_flag_N_accumulator();
        arc_gen_set_flag_V_clear();
    }
}

ARC_GEN_TRANS_MPYD_DOP_REG_REG(mpyd);
ARC_GEN_TRANS_MPYD_DOP_REG_U6(mpyd);
ARC_GEN_TRANS_MPYD_DOP_REG_S12(mpyd);
ARC_GEN_TRANS_MPYD_DOP_COND_REG(mpyd);
ARC_GEN_TRANS_MPYD_DOP_COND_U6(mpyd);

/*
 * MPYDU, 32 x 32 unsigned multiply, returning a 64-bit result
 *
 * A = b * c
 * ACC = A
 *
 * Z - Unchanged
 * N - Unchanged
 * C - Unchanged
 * V - Cleared
 */
static void arc_gen_mpydu(DisasContext *ctx, TCGv_i64 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    arc_gen_single_mul_32x32_unsigned(a, b, c);
    arc_gen_save_accumulator(a);

    if (set_flags) {
        arc_gen_set_flag_V_clear();
    }
}

ARC_GEN_TRANS_MPYD_DOP_REG_REG(mpydu);
ARC_GEN_TRANS_MPYD_DOP_REG_U6(mpydu);
ARC_GEN_TRANS_MPYD_DOP_REG_S12(mpydu);
ARC_GEN_TRANS_MPYD_DOP_COND_REG(mpydu);
ARC_GEN_TRANS_MPYD_DOP_COND_U6(mpydu);

static inline void arc64_gen_mpyl(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 tmp = tcg_temp_new_i64();

    tcg_gen_muls2_i64(a, tmp, b, c);
}

ARC64_DEF_TRANS_DOP_REG_REG(mpyl)
ARC64_DEF_TRANS_DOP_REG_U6(mpyl)
ARC64_DEF_TRANS_DOP_REG_S12(mpyl)
ARC64_DEF_TRANS_DOP_COND_REG(mpyl)
ARC64_DEF_TRANS_DOP_COND_U6(mpyl)

static inline void arc64_gen_mpyml(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 tmp = tcg_temp_new_i64();

    tcg_gen_muls2_i64(tmp, a, b, c);
}

ARC64_DEF_TRANS_DOP_REG_REG(mpyml)
ARC64_DEF_TRANS_DOP_REG_U6(mpyml)
ARC64_DEF_TRANS_DOP_REG_S12(mpyml)
ARC64_DEF_TRANS_DOP_COND_REG(mpyml)
ARC64_DEF_TRANS_DOP_COND_U6(mpyml)

static inline void arc64_gen_mpymul(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 tmp = tcg_temp_new_i64();

    tcg_gen_mulu2_i64(tmp, a, b, c);
}

ARC64_DEF_TRANS_DOP_REG_REG(mpymul)
ARC64_DEF_TRANS_DOP_REG_U6(mpymul)
ARC64_DEF_TRANS_DOP_REG_S12(mpymul)
ARC64_DEF_TRANS_DOP_COND_REG(mpymul)
ARC64_DEF_TRANS_DOP_COND_U6(mpymul)

static inline void arc64_gen_mpymsul(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 tmp = tcg_temp_new_i64();

    tcg_gen_mulsu2_i64(tmp, a, c, b);
}

ARC64_DEF_TRANS_DOP_REG_REG(mpymsul)
ARC64_DEF_TRANS_DOP_REG_U6(mpymsul)
ARC64_DEF_TRANS_DOP_REG_S12(mpymsul)
ARC64_DEF_TRANS_DOP_COND_REG(mpymsul)
ARC64_DEF_TRANS_DOP_COND_U6(mpymsul)

/*
 * DMPYH, sum of dual signed 16x16 multiplications
 *
 * T = (b.h0 * c.h0) + (b.h1 * c.h1);
 * a = T.w0
 * ACC = T
 *
 * Z - Unchanged
 * N - Set to the resulting sign of the accumulator
 * C - Unchanged
 * V - Cleared
 */
static void arc_gen_dmpyh(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i64 result = tcg_temp_new_i64();

    arc_gen_dual_mul_16x16_signed(result, b, c);
    tcg_gen_extrl_i64_i32(a, result);
    arc_gen_save_accumulator(result);

    if (set_flags) {
        arc_gen_set_flag_N_accumulator();
        arc_gen_set_flag_V_clear();
    }
}

ARC_GEN_TRANS_MPY_DOP_REG_REG(dmpyh);
ARC_GEN_TRANS_MPY_DOP_REG_U6(dmpyh);
ARC_GEN_TRANS_MPY_DOP_REG_S12(dmpyh);
ARC_GEN_TRANS_MPY_DOP_COND_REG(dmpyh);
ARC_GEN_TRANS_MPY_DOP_COND_U6(dmpyh);

/*
 * DMPYHU, sum of dual unsigned 16x16 multiplications
 *
 * T = (b.h0 * c.h0) + (b.h1 * c.h1);
 * a = T.w0
 * ACC = T
 *
 * Z - Unchanged
 * N - Unchanged
 * C - Unchanged
 * V - Cleared
 */
static void arc_gen_dmpyhu(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i64 result = tcg_temp_new_i64();

    arc_gen_dual_mul_16x16_unsigned(result, b, c);

    tcg_gen_extrl_i64_i32(a, result);
    arc_gen_save_accumulator(result);

    if (set_flags) {
        tcg_gen_movi_i32(cpu_Vf, 0);
    }
}

ARC_GEN_TRANS_MPY_DOP_REG_REG(dmpyhu);
ARC_GEN_TRANS_MPY_DOP_REG_U6(dmpyhu);
ARC_GEN_TRANS_MPY_DOP_REG_S12(dmpyhu);
ARC_GEN_TRANS_MPY_DOP_COND_REG(dmpyhu);
ARC_GEN_TRANS_MPY_DOP_COND_U6(dmpyhu);

/*
 * DMPYWH, sum of dual signed 32x16 multiplication
 *
 * A = (B.w0 * c.h0) + (B.w1 * c.h1);
 * ACC = A
 *
 * Z - Unchanged
 * N - Set to the resulting sign of the accumulator
 * C - Unchanged
 * V - Cleared
 */
static void arc_gen_dmpywh(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i64 result = tcg_temp_new_i64();

    arc_gen_dual_mul_32x16_signed(result, b, c);

    tcg_gen_mov_i64(a, result);
    arc_gen_save_accumulator(result);

    if (set_flags) {
        arc_gen_set_flag_N_accumulator();
        arc_gen_set_flag_V_clear();
    }
}

ARC_GEN_TRANS_DMPYWH_DOP_REG_REG(dmpywh);
ARC_GEN_TRANS_DMPYWH_DOP_REG_U6(dmpywh);
ARC_GEN_TRANS_DMPYWH_DOP_REG_S12(dmpywh);
ARC_GEN_TRANS_DMPYWH_DOP_COND_REG(dmpywh);
ARC_GEN_TRANS_DMPYWH_DOP_COND_U6(dmpywh);

/*
 * DMPYWHU, sum of dual unsigned 32x16 multiplication
 *
 * A = (B.w0 * c.h0) + (B.w1 * c.h1);
 * ACC = A
 *
 * Z - Unchanged
 * N - Unchanged
 * C - Unchanged
 * V - Cleared
 */
static void arc_gen_dmpywhu(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i64 result = tcg_temp_new_i64();

    arc_gen_dual_mul_32x16_unsigned(result, b, c);

    tcg_gen_mov_i64(a, result);
    arc_gen_save_accumulator(result);

    if (set_flags) {
        arc_gen_set_flag_V_clear();
    }
}

ARC_GEN_TRANS_DMPYWH_DOP_REG_REG(dmpywhu);
ARC_GEN_TRANS_DMPYWH_DOP_REG_U6(dmpywhu);
ARC_GEN_TRANS_DMPYWH_DOP_REG_S12(dmpywhu);
ARC_GEN_TRANS_DMPYWH_DOP_COND_REG(dmpywhu);
ARC_GEN_TRANS_DMPYWH_DOP_COND_U6(dmpywhu);

/*
 * QMPYH, quad signed 16x16 multiplication
 *
 * A = (B.h0 * C.h0) + (B.h1 * C.h1) + (B.h2 * C.h2) + (B.h3 * C.h3)
 * ACC = A
 *
 * Z - Unchanged
 * N - Set to the resulting sign of the accumulator
 * C - Unchanged
 * V - Cleared
 */
static void arc_gen_qmpyh(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 result = tcg_temp_new_i64();

    arc_gen_quad_mul_16x16_signed(result, b, c);

    tcg_gen_mov_i64(a, result);
    arc_gen_save_accumulator(result);

    if (set_flags) {
        arc_gen_set_flag_N_accumulator();
        arc_gen_set_flag_V_clear();
    }
}

ARC_GEN_TRANS_QMPYH_DOP_REG_REG(qmpyh);
ARC_GEN_TRANS_QMPYH_DOP_REG_U6(qmpyh);
ARC_GEN_TRANS_QMPYH_DOP_REG_S12(qmpyh);
ARC_GEN_TRANS_QMPYH_DOP_COND_REG(qmpyh);
ARC_GEN_TRANS_QMPYH_DOP_COND_U6(qmpyh);

/*
 * QMPYHU, quad unsigned 16x16 multiplication
 *
 * A = (B.h0 * C.h0) + (B.h1 * C.h1) + (B.h2 * C.h2) + (B.h3 * C.h3)
 * ACC = A
 *
 * Z - Unchanged
 * N - Unchanged
 * C - Unchanged
 * V - Cleared
 */
static void arc_gen_qmpyhu(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 result = tcg_temp_new_i64();

    arc_gen_quad_mul_16x16_unsigned(result, b, c);

    tcg_gen_mov_i64(a, result);
    arc_gen_save_accumulator(result);

    if (set_flags) {
        arc_gen_set_flag_V_clear();
    }
}

ARC_GEN_TRANS_QMPYH_DOP_REG_REG(qmpyhu);
ARC_GEN_TRANS_QMPYH_DOP_REG_U6(qmpyhu);
ARC_GEN_TRANS_QMPYH_DOP_REG_S12(qmpyhu);
ARC_GEN_TRANS_QMPYH_DOP_COND_REG(qmpyhu);
ARC_GEN_TRANS_QMPYH_DOP_COND_U6(qmpyhu);

/*
 * MAC, 32 x 32 signed multiply and accumulate
 *
 * T = ACC + (b * c);
 * a = T.w0;
 * ACC = T;
 *
 * Z - Unchanged
 * N - Set if the accumulator is negative
 * C - Unchanged
 * V - Set if an overflow occurs during accumulation. This instruction never clears this flag.
 */
static void arc_gen_mac(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i32 low = tcg_temp_new_i32();
    TCGv_i32 high = tcg_temp_new_i32();
    TCGv_i64 prod = tcg_temp_new_i64();
    TCGv_i64 result = tcg_temp_new_i64();
    TCGv_i64 acc = arc_gen_load_accumulator();

    tcg_gen_muls2_i32(low, high, b, c);
    tcg_gen_concat_i32_i64(prod, low, high);
    tcg_gen_add_i64(result, acc, prod);

    arc_gen_save_accumulator(result);
    tcg_gen_extrl_i64_i32(a, result);

    if (set_flags) {
        arc_gen_set_flag_N_accumulator();
        arc_gen_set_flag_V_mac(result, acc, prod);
    }
}

ARC_GEN_TRANS_MPY_DOP_REG_REG(mac);
ARC_GEN_TRANS_MPY_DOP_REG_U6(mac);
ARC_GEN_TRANS_MPY_DOP_REG_S12(mac);
ARC_GEN_TRANS_MPY_DOP_COND_REG(mac);
ARC_GEN_TRANS_MPY_DOP_COND_U6(mac);

/*
 * MACU, 32 x 32 unsigned multiply and accumulate
 *
 * T = ACC + (b * c);
 * a = T.w0;
 * acc = T;
 *
 * Z - Unchanged
 * N - Unchanged
 * C - Unchanged
 * V - Set if the accumulator overflows. This instruction never clears this flag.
 */
static void arc_gen_macu(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i32 low = tcg_temp_new_i32();
    TCGv_i32 high = tcg_temp_new_i32();
    TCGv_i64 result = tcg_temp_new_i64();
    TCGv_i64 acc = arc_gen_load_accumulator();

    tcg_gen_mulu2_i32(low, high, b, c);
    tcg_gen_concat_i32_i64(result, low, high);
    tcg_gen_add_i64(result, acc, result);

    arc_gen_save_accumulator(result);
    tcg_gen_extrl_i64_i32(a, result);

    if (set_flags) {
        arc_gen_set_flag_V_macu(result, acc);
    }
}

ARC_GEN_TRANS_MPY_DOP_REG_REG(macu);
ARC_GEN_TRANS_MPY_DOP_REG_U6(macu);
ARC_GEN_TRANS_MPY_DOP_REG_S12(macu);
ARC_GEN_TRANS_MPY_DOP_COND_REG(macu);
ARC_GEN_TRANS_MPY_DOP_COND_U6(macu);

/*
 * MACD, 32 x 32 signed multiply and accumulate, returning a 64-bit result
 *
 * A = ACC + (b * c);
 * ACC = A;
 *
 * Z - Unchanged
 * N - Set to the resulting sign of the accumulator
 * C - Unchanged
 * V - Set if an overflow occurs during accumulation. This instruction never clears this flag.
 */
static void arc_gen_macd(DisasContext *ctx, TCGv_i64 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i32 prod_low = tcg_temp_new_i32();
    TCGv_i32 prod_high = tcg_temp_new_i32();
    TCGv_i64 prod = tcg_temp_new_i64();
    TCGv_i64 acc = arc_gen_load_accumulator();

    tcg_gen_muls2_i32(prod_low, prod_high, b, c);
    tcg_gen_concat_i32_i64(prod, prod_low, prod_high);
    tcg_gen_add_i64(a, acc, prod);

    arc_gen_save_accumulator(a);

    if (set_flags) {
        arc_gen_set_flag_N_accumulator();
        arc_gen_set_flag_V_mac(a, acc, prod);
    }
}

ARC_GEN_TRANS_MPYD_DOP_REG_REG(macd);
ARC_GEN_TRANS_MPYD_DOP_REG_U6(macd);
ARC_GEN_TRANS_MPYD_DOP_REG_S12(macd);
ARC_GEN_TRANS_MPYD_DOP_COND_REG(macd);
ARC_GEN_TRANS_MPYD_DOP_COND_U6(macd);

/*
 * MACDU, 32 x 32 unsigned multiply and accumulate, returning a 64-bit result
 *
 * T = ACC + (b * c);
 * A = T.w0;
 * ACC = T;
 *
 * Z - Unchanged
 * N - Unchanged
 * C - Unchanged
 * V - Set if the accumulator overflows. This instruction never clears this flag.
 */
static void arc_gen_macdu(DisasContext *ctx, TCGv_i64 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i32 prod_low = tcg_temp_new_i32();
    TCGv_i32 prod_high = tcg_temp_new_i32();
    TCGv_i64 prod = tcg_temp_new_i64();
    TCGv_i64 acc = arc_gen_load_accumulator();

    tcg_gen_mulu2_i32(prod_low, prod_high, b, c);
    tcg_gen_concat_i32_i64(prod, prod_low, prod_high);
    tcg_gen_add_i64(a, acc, prod);

    arc_gen_save_accumulator(a);

    if (set_flags) {
        arc_gen_set_flag_V_macu(a, acc);
    }
}

ARC_GEN_TRANS_MPYD_DOP_REG_REG(macdu);
ARC_GEN_TRANS_MPYD_DOP_REG_U6(macdu);
ARC_GEN_TRANS_MPYD_DOP_REG_S12(macdu);
ARC_GEN_TRANS_MPYD_DOP_COND_REG(macdu);
ARC_GEN_TRANS_MPYD_DOP_COND_U6(macdu);

/*
 * DMACH, sum of dual signed 16x16 multiplications and accumulate
 *
 * T = ACC + (b.h0 * c.h0) + (b.h1 * c.h1);
 * a = T.w0
 * ACC = T
 *
 * Z - Unchanged
 * N - Set to the resulting sign of the accumulator
 * C - Unchanged
 * V - Set if an overflow occurs during accumulation. This instruction never clears this flag.
 */
static void arc_gen_dmach(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i64 prod = tcg_temp_new_i64();
    TCGv_i64 result = tcg_temp_new_i64();
    TCGv_i64 acc = arc_gen_load_accumulator();

    arc_gen_dual_mul_16x16_signed(prod, b, c);
    tcg_gen_add_i64(result, prod, acc);

    arc_gen_save_accumulator(result);
    tcg_gen_extrl_i64_i32(a, result);

    if (set_flags) {
        arc_gen_set_flag_N_accumulator();
        arc_gen_set_flag_V_mac(result, acc, prod);
    }
}

ARC_GEN_TRANS_MPY_DOP_REG_REG(dmach);
ARC_GEN_TRANS_MPY_DOP_REG_U6(dmach);
ARC_GEN_TRANS_MPY_DOP_REG_S12(dmach);
ARC_GEN_TRANS_MPY_DOP_COND_REG(dmach);
ARC_GEN_TRANS_MPY_DOP_COND_U6(dmach);

/*
 * DMACHU, sum of dual unsigned 16x16 multiplications and accumulate
 *
 * T = ACC + (b.h0 * c.h0) + (b.h1 * c.h1);
 * a = T.w0
 * ACC = T
 *
 * Z - Unchanged
 * N - Unchanged
 * C - Unchanged
 * V - Set if an overflow occurs during accumulation. This instruction never clears this flag.
 */
static void arc_gen_dmachu(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i64 result = tcg_temp_new_i64();
    TCGv_i64 acc = arc_gen_load_accumulator();

    arc_gen_dual_mul_16x16_unsigned(result, b, c);
    tcg_gen_add_i64(result, result, acc);

    arc_gen_save_accumulator(result);
    tcg_gen_extrl_i64_i32(a, result);

    if (set_flags) {
        arc_gen_set_flag_V_macu(result, acc);
    }
}

ARC_GEN_TRANS_MPY_DOP_REG_REG(dmachu);
ARC_GEN_TRANS_MPY_DOP_REG_U6(dmachu);
ARC_GEN_TRANS_MPY_DOP_REG_S12(dmachu);
ARC_GEN_TRANS_MPY_DOP_COND_REG(dmachu);
ARC_GEN_TRANS_MPY_DOP_COND_U6(dmachu);

/*
 * DMACWH, sum of dual signed 32x16 multiplication and accumulate
 *
 * A = ACC + (B.w0 * c.h0) + (B.w1 * c.h1);
 * ACC = A
 *
 * Z - Unchanged
 * N - Set to the resulting sign of the accumulator
 * C - Unchanged
 * V - Set if an overflow occurs during accumulation. This instruction never clears this flag.
 */
static void arc_gen_dmacwh(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i64 acc = arc_gen_load_accumulator();
    TCGv_i64 result = tcg_temp_new_i64();
    TCGv_i64 prod = tcg_temp_new_i64();

    arc_gen_dual_mul_32x16_signed(prod, b, c);
    tcg_gen_add_i64(result, prod, acc);

    tcg_gen_mov_i64(a, result);
    arc_gen_save_accumulator(result);

    if (set_flags) {
        arc_gen_set_flag_N_accumulator();
        arc_gen_set_flag_V_mac(result, acc, prod);
    }
}

ARC_GEN_TRANS_DMPYWH_DOP_REG_REG(dmacwh);
ARC_GEN_TRANS_DMPYWH_DOP_REG_U6(dmacwh);
ARC_GEN_TRANS_DMPYWH_DOP_REG_S12(dmacwh);
ARC_GEN_TRANS_DMPYWH_DOP_COND_REG(dmacwh);
ARC_GEN_TRANS_DMPYWH_DOP_COND_U6(dmacwh);

/*
 * DMACWHU, sum of dual unsigned 32x16 multiplication and accumulate
 *
 * A = ACC + (B.w0 * c.h0) + (B.w1 * c.h1);
 * ACC = A
 *
 * Z - Unchanged
 * N - Unchanged
 * C - Unchanged
 * V - Set if an overflow occurs during accumulation. This instruction never clears this flag.
 */
static void arc_gen_dmacwhu(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i32 c, bool set_flags)
{
    TCGv_i64 acc = arc_gen_load_accumulator();
    TCGv_i64 result = tcg_temp_new_i64();

    arc_gen_dual_mul_32x16_unsigned(result, b, c);
    tcg_gen_add_i64(result, result, acc);

    tcg_gen_mov_i64(a, result);
    arc_gen_save_accumulator(result);

    if (set_flags) {
        arc_gen_set_flag_V_macu(result, acc);
    }
}

ARC_GEN_TRANS_DMPYWH_DOP_REG_REG(dmacwhu);
ARC_GEN_TRANS_DMPYWH_DOP_REG_U6(dmacwhu);
ARC_GEN_TRANS_DMPYWH_DOP_REG_S12(dmacwhu);
ARC_GEN_TRANS_DMPYWH_DOP_COND_REG(dmacwhu);
ARC_GEN_TRANS_DMPYWH_DOP_COND_U6(dmacwhu);

/*
 * QMACH, quad signed 16x16 multiplication and accumulate
 *
 * A = ACC + (B.h0 * C.h0) + (B.h1 * C.h1) + (B.h2 * C.h2) + (B.h3 * C.h3)
 * ACC = A
 *
 * Z - Unchanged
 * N - Set to the resulting sign of the accumulator
 * C - Unchanged
 * V - Set if the an overflow occurs during accumulation. This instruction never clears this flag.
 */
static void arc_gen_qmach(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 acc = arc_gen_load_accumulator();
    TCGv_i64 result = tcg_temp_new_i64();
    TCGv_i64 prod = tcg_temp_new_i64();

    arc_gen_quad_mul_16x16_signed(prod, b, c);
    tcg_gen_add_i64(result, prod, acc);

    tcg_gen_mov_i64(a, result);
    arc_gen_save_accumulator(result);

    if (set_flags) {
        arc_gen_set_flag_N_accumulator();
        arc_gen_set_flag_V_mac(result, acc, prod);
    }
}

ARC_GEN_TRANS_QMPYH_DOP_REG_REG(qmach);
ARC_GEN_TRANS_QMPYH_DOP_REG_U6(qmach);
ARC_GEN_TRANS_QMPYH_DOP_REG_S12(qmach);
ARC_GEN_TRANS_QMPYH_DOP_COND_REG(qmach);
ARC_GEN_TRANS_QMPYH_DOP_COND_U6(qmach);

/*
 * QMACHU, quad unsigned 16x16 multiplication and accumulate
 *
 * A = ACC + (B.h0 * C.h0) + (B.h1 * C.h1) + (B.h2 * C.h2) + (B.h3 * C.h3)
 * ACC = A
 *
 * Z - Unchanged
 * N - Unchanged
 * C - Unchanged
 * V - Set if the an overflow occurs during accumulation. This instruction never clears this flag.
 */
static void arc_gen_qmachu(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 acc = arc_gen_load_accumulator();
    TCGv_i64 result = tcg_temp_new_i64();
    TCGv_i64 prod = tcg_temp_new_i64();

    arc_gen_quad_mul_16x16_unsigned(prod, b, c);
    tcg_gen_add_i64(result, prod, acc);

    tcg_gen_mov_i64(a, result);
    arc_gen_save_accumulator(result);

    if (set_flags) {
        arc_gen_set_flag_V_macu(result, acc);
    }
}

ARC_GEN_TRANS_QMPYH_DOP_REG_REG(qmachu);
ARC_GEN_TRANS_QMPYH_DOP_REG_U6(qmachu);
ARC_GEN_TRANS_QMPYH_DOP_REG_S12(qmachu);
ARC_GEN_TRANS_QMPYH_DOP_COND_REG(qmachu);
ARC_GEN_TRANS_QMPYH_DOP_COND_U6(qmachu);
