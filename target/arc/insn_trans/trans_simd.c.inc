#include "translate.h"
#include "tcg/tcg-op-gvec.h"

/* Translator macros for VADD2, VADD4H, VADDSUB, VADDSUB4H, VSUB2, VSUB4H, VSUBADD and VSUBADD4H instructions */

#define ARC_GEN_TRANS_VADD2_DOP_REG_REG(name) \
    static bool trans_##name##_dop_reg_reg(DisasContext *ctx, arg_##name##_dop_reg_reg *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->a); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i64 reg_c_pair = arc_insn_load_src_reg_pair(ctx, a->c, ARC_REGPAIR_LIMM_DUPLICATE); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c_pair); \
        arc_insn_save_reg_pair(ctx, a->a, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_VADD2_DOP_REG_U6(name) \
    static bool trans_##name##_dop_reg_u6(DisasContext *ctx, arg_##name##_dop_reg_u6 *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->a); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i64 reg_c_pair = arc64_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c_pair); \
        arc_insn_save_reg_pair(ctx, a->a, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_VADD2_DOP_REG_S12(name) \
    static bool trans_##name##_dop_reg_s12(DisasContext *ctx, arg_##name##_dop_reg_s12 *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i64 reg_c_pair = arc64_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c_pair); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_VADD2_DOP_COND_REG(name) \
    static bool trans_##name##_dop_cond_reg(DisasContext *ctx, arg_##name##_dop_cond_reg *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i64 reg_c_pair = arc_insn_load_src_reg_pair(ctx, a->c, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c_pair); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        gen_set_label(skip); \
        return true; \
    }

#define ARC_GEN_TRANS_VADD2_DOP_COND_U6(name) \
    static bool trans_##name##_dop_cond_u6(DisasContext *ctx, arg_##name##_dop_cond_u6 *a) \
    { \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i64 reg_b_pair = arc_insn_load_src_reg_pair(ctx, a->b, ARC_REGPAIR_LIMM_DUPLICATE); \
        TCGv_i64 reg_c_pair = arc64_insn_load_imm(a->imm); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, reg_a_pair, reg_b_pair, reg_c_pair); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        gen_set_label(skip); \
        return true; \
    }

/* Translator macros for VADD2H, VADDSUB2H, VSUB2H and VSUBADD2H */

#define ARC_GEN_TRANS_VADD2H_DOP_REG_REG(name) \
    static bool trans_##name##_dop_reg_reg(DisasContext *ctx, arg_##name##_dop_reg_reg *a) \
    { \
        TCGv_i32 src_a = arc_insn_load_dst_reg(ctx, a->a); \
        TCGv_i32 src_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 src_c = arc_insn_load_src_reg(ctx, a->c); \
        arc_gen_##name(ctx, src_a, src_b, src_c); \
        arc_insn_save_reg(ctx, a->a, src_a); \
        return true; \
    }

#define ARC_GEN_TRANS_VADD2H_DOP_REG_U6(name) \
    static bool trans_##name##_dop_reg_u6(DisasContext *ctx, arg_##name##_dop_reg_u6 *a) \
    { \
        TCGv_i32 src_a = arc_insn_load_dst_reg(ctx, a->a); \
        TCGv_i32 src_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 src_c = arc_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, src_a, src_b, src_c); \
        arc_insn_save_reg(ctx, a->a, src_a); \
        return true; \
    }

#define ARC_GEN_TRANS_VADD2H_DOP_REG_S12(name) \
    static bool trans_##name##_dop_reg_s12(DisasContext *ctx, arg_##name##_dop_reg_s12 *a) \
    { \
        TCGv_i32 src_a = arc_insn_load_dst_reg(ctx, a->b); \
        TCGv_i32 src_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 src_c = arc_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, src_a, src_b, src_c); \
        arc_insn_save_reg(ctx, a->b, src_a); \
        return true; \
    }

#define ARC_GEN_TRANS_VADD2H_DOP_COND_REG(name) \
    static bool trans_##name##_dop_cond_reg(DisasContext *ctx, arg_##name##_dop_cond_reg *a) \
    { \
        TCGv_i32 src_a = arc_insn_load_dst_reg(ctx, a->b); \
        TCGv_i32 src_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 src_c = arc_insn_load_src_reg(ctx, a->c); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, src_a, src_b, src_c); \
        arc_insn_save_reg(ctx, a->b, src_a); \
        gen_set_label(skip); \
        return true; \
    }

#define ARC_GEN_TRANS_VADD2H_DOP_COND_U6(name) \
    static bool trans_##name##_dop_cond_u6(DisasContext *ctx, arg_##name##_dop_cond_u6 *a) \
    { \
        TCGv_i32 src_a = arc_insn_load_dst_reg(ctx, a->b); \
        TCGv_i32 src_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 src_c = arc_insn_load_imm(a->imm); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, src_a, src_b, src_c); \
        arc_insn_save_reg(ctx, a->b, src_a); \
        gen_set_label(skip); \
        return true; \
    }

/* Translator macros for VMAC2H, VMAC2HU, VMPY2H and VMPY2HU instructions */

#define ARC_GEN_TRANS_VMPY2H_DOP_REG_REG(name) \
    static bool trans_##name##_dop_reg_reg(DisasContext *ctx, arg_##name##_dop_reg_reg *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->a); \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->a); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c); \
        arc_gen_##name(ctx, reg_a_pair, reg_b, reg_c); \
        arc_insn_save_reg_pair(ctx, a->a, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_VMPY2H_DOP_REG_U6(name) \
    static bool trans_##name##_dop_reg_u6(DisasContext *ctx, arg_##name##_dop_reg_u6 *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->a); \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->a); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, reg_a_pair, reg_b, reg_c); \
        arc_insn_save_reg_pair(ctx, a->a, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_VMPY2H_DOP_REG_S12(name) \
    static bool trans_##name##_dop_reg_s12(DisasContext *ctx, arg_##name##_dop_reg_s12 *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->b); \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_imm(a->imm); \
        arc_gen_##name(ctx, reg_a_pair, reg_b, reg_c); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        return true; \
    }

#define ARC_GEN_TRANS_VMPY2H_DOP_COND_REG(name) \
    static bool trans_##name##_dop_cond_reg(DisasContext *ctx, arg_##name##_dop_cond_reg *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->b); \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, reg_a_pair, reg_b, reg_c); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        gen_set_label(skip); \
        return true; \
    }

#define ARC_GEN_TRANS_VMPY2H_DOP_COND_U6(name) \
    static bool trans_##name##_dop_cond_u6(DisasContext *ctx, arg_##name##_dop_cond_u6 *a) \
    { \
        arc_gen_check_not_lp_count(ctx, a->b); \
        TCGv_i64 reg_a_pair = arc_insn_load_dst_reg_pair(ctx, a->b); \
        TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b); \
        TCGv_i32 reg_c = arc_insn_load_imm(a->imm); \
        TCGLabel *skip = gen_new_label(); \
        arc_gen_cc_prologue(ctx, a->cond, skip); \
        arc_gen_##name(ctx, reg_a_pair, reg_b, reg_c); \
        arc_insn_save_reg_pair(ctx, a->b, reg_a_pair); \
        gen_set_label(skip); \
        return true; \
    }

/*
 * VADD2, dual 32-bit vector addition
 *
 * A.w0 = B.w0 + C.w0
 * A.w1 = B.w1 + C.w1
 */
static void arc_gen_vadd2(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c)
{
    tcg_gen_vec_add32_i64(a, b, c);
}

ARC_GEN_TRANS_VADD2_DOP_REG_REG(vadd2);
ARC_GEN_TRANS_VADD2_DOP_REG_U6(vadd2);
ARC_GEN_TRANS_VADD2_DOP_REG_S12(vadd2);
ARC_GEN_TRANS_VADD2_DOP_COND_REG(vadd2);
ARC_GEN_TRANS_VADD2_DOP_COND_U6(vadd2);

/*
 * VADDSUB, dual 32-bit vector add and subtract
 *
 * A.w0 = B.w0 + C.w0
 * A.w1 = B.w1 - C.w1
 */
static void arc_gen_vaddsub(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c)
{
    TCGv_i64 sum = tcg_temp_new_i64();
    TCGv_i64 diff = tcg_temp_new_i64();

    tcg_gen_vec_add32_i64(sum, b, c);
    tcg_gen_vec_sub32_i64(diff, b, c);

    tcg_gen_deposit_i64(a, diff, sum, 0, 32);
}

ARC_GEN_TRANS_VADD2_DOP_REG_REG(vaddsub);
ARC_GEN_TRANS_VADD2_DOP_REG_U6(vaddsub);
ARC_GEN_TRANS_VADD2_DOP_REG_S12(vaddsub);
ARC_GEN_TRANS_VADD2_DOP_COND_REG(vaddsub);
ARC_GEN_TRANS_VADD2_DOP_COND_U6(vaddsub);

static void arc_gen_vsub2(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c)
{
    tcg_gen_vec_sub32_i64(a, b, c);
}

ARC_GEN_TRANS_VADD2_DOP_REG_REG(vsub2);
ARC_GEN_TRANS_VADD2_DOP_REG_U6(vsub2);
ARC_GEN_TRANS_VADD2_DOP_REG_S12(vsub2);
ARC_GEN_TRANS_VADD2_DOP_COND_REG(vsub2);
ARC_GEN_TRANS_VADD2_DOP_COND_U6(vsub2);

static void arc_gen_vsubadd(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c)
{
    TCGv_i64 sum = tcg_temp_new_i64();
    TCGv_i64 diff = tcg_temp_new_i64();

    tcg_gen_vec_add32_i64(sum, b, c);
    tcg_gen_vec_sub32_i64(diff, b, c);

    tcg_gen_deposit_i64(a, sum, diff, 0, 32);
}

ARC_GEN_TRANS_VADD2_DOP_REG_REG(vsubadd);
ARC_GEN_TRANS_VADD2_DOP_REG_U6(vsubadd);
ARC_GEN_TRANS_VADD2_DOP_REG_S12(vsubadd);
ARC_GEN_TRANS_VADD2_DOP_COND_REG(vsubadd);
ARC_GEN_TRANS_VADD2_DOP_COND_U6(vsubadd);

static void arc_gen_vadd2h(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c)
{
    tcg_gen_vec_add16_i32(a, b, c);
}

ARC_GEN_TRANS_VADD2H_DOP_REG_REG(vadd2h);
ARC_GEN_TRANS_VADD2H_DOP_REG_U6(vadd2h);
ARC_GEN_TRANS_VADD2H_DOP_REG_S12(vadd2h);
ARC_GEN_TRANS_VADD2H_DOP_COND_REG(vadd2h);
ARC_GEN_TRANS_VADD2H_DOP_COND_U6(vadd2h);

static void arc_gen_vaddsub2h(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c)
{
    TCGv_i32 sum = tcg_temp_new_i32();
    TCGv_i32 diff = tcg_temp_new_i32();

    tcg_gen_vec_add16_i32(sum, b, c);
    tcg_gen_vec_sub16_i32(diff, b, c);

    tcg_gen_deposit_i32(a, diff, sum, 0, 16);
}

ARC_GEN_TRANS_VADD2H_DOP_REG_REG(vaddsub2h);
ARC_GEN_TRANS_VADD2H_DOP_REG_U6(vaddsub2h);
ARC_GEN_TRANS_VADD2H_DOP_REG_S12(vaddsub2h);
ARC_GEN_TRANS_VADD2H_DOP_COND_REG(vaddsub2h);
ARC_GEN_TRANS_VADD2H_DOP_COND_U6(vaddsub2h);

static void arc_gen_vsub2h(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c)
{
    tcg_gen_vec_sub16_i32(a, b, c);
}

ARC_GEN_TRANS_VADD2H_DOP_REG_REG(vsub2h);
ARC_GEN_TRANS_VADD2H_DOP_REG_U6(vsub2h);
ARC_GEN_TRANS_VADD2H_DOP_REG_S12(vsub2h);
ARC_GEN_TRANS_VADD2H_DOP_COND_REG(vsub2h);
ARC_GEN_TRANS_VADD2H_DOP_COND_U6(vsub2h);

static void arc_gen_vsubadd2h(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c)
{
    TCGv_i32 sum = tcg_temp_new_i32();
    TCGv_i32 diff = tcg_temp_new_i32();

    tcg_gen_vec_add16_i32(sum, b, c);
    tcg_gen_vec_sub16_i32(diff, b, c);

    tcg_gen_deposit_i32(a, sum, diff, 0, 16);
}

ARC_GEN_TRANS_VADD2H_DOP_REG_REG(vsubadd2h);
ARC_GEN_TRANS_VADD2H_DOP_REG_U6(vsubadd2h);
ARC_GEN_TRANS_VADD2H_DOP_REG_S12(vsubadd2h);
ARC_GEN_TRANS_VADD2H_DOP_COND_REG(vsubadd2h);
ARC_GEN_TRANS_VADD2H_DOP_COND_U6(vsubadd2h);

static void arc_gen_vadd4h(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c)
{
    tcg_gen_vec_add16_i64(a, b, c);
}

ARC_GEN_TRANS_VADD2_DOP_REG_REG(vadd4h);
ARC_GEN_TRANS_VADD2_DOP_REG_U6(vadd4h);
ARC_GEN_TRANS_VADD2_DOP_REG_S12(vadd4h);
ARC_GEN_TRANS_VADD2_DOP_COND_REG(vadd4h);
ARC_GEN_TRANS_VADD2_DOP_COND_U6(vadd4h);

/*
 * VADDSUB4H, quad 16-bit vector add and subtract
 *
 * A.h0 = B.h0 + C.h0
 * A.h1 = B.h1 - C.h1
 * A.h2 = B.h2 + C.h2
 * A.h3 = B.h3 - C.h3
 */
static void arc_gen_vaddsub4h(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c)
{
    TCGv_i64 sum = tcg_temp_new_i64();
    TCGv_i64 diff = tcg_temp_new_i64();

    tcg_gen_vec_add16_i64(sum, b, c);
    tcg_gen_vec_sub16_i64(diff, b, c);

    tcg_gen_andi_i64(sum, sum, 0x0000ffff0000ffffULL);
    tcg_gen_andi_i64(diff, diff, 0xffff0000ffff0000ULL);
    tcg_gen_or_i64(a, sum, diff);
}

ARC_GEN_TRANS_VADD2_DOP_REG_REG(vaddsub4h);
ARC_GEN_TRANS_VADD2_DOP_REG_U6(vaddsub4h);
ARC_GEN_TRANS_VADD2_DOP_REG_S12(vaddsub4h);
ARC_GEN_TRANS_VADD2_DOP_COND_REG(vaddsub4h);
ARC_GEN_TRANS_VADD2_DOP_COND_U6(vaddsub4h);

static void arc_gen_vsub4h(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c)
{
    tcg_gen_vec_sub16_i64(a, b, c);
}

ARC_GEN_TRANS_VADD2_DOP_REG_REG(vsub4h);
ARC_GEN_TRANS_VADD2_DOP_REG_U6(vsub4h);
ARC_GEN_TRANS_VADD2_DOP_REG_S12(vsub4h);
ARC_GEN_TRANS_VADD2_DOP_COND_REG(vsub4h);
ARC_GEN_TRANS_VADD2_DOP_COND_U6(vsub4h);

/*
 * VSUBADD4H, quad 16-bit vector subtract and add
 *
 * A.h0 = B.h0 - C.h0
 * A.h1 = B.h1 + C.h1
 * A.h2 = B.h2 - C.h2
 * A.h3 = B.h3 + C.h3
 */
static void arc_gen_vsubadd4h(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c)
{
    TCGv_i64 sum = tcg_temp_new_i64();
    TCGv_i64 diff = tcg_temp_new_i64();

    tcg_gen_vec_add16_i64(sum, b, c);
    tcg_gen_vec_sub16_i64(diff, b, c);

    tcg_gen_andi_i64(diff, diff, 0x0000ffff0000ffffULL);
    tcg_gen_andi_i64(sum, sum, 0xffff0000ffff0000ULL);
    tcg_gen_or_i64(a, sum, diff);
}

ARC_GEN_TRANS_VADD2_DOP_REG_REG(vsubadd4h);
ARC_GEN_TRANS_VADD2_DOP_REG_U6(vsubadd4h);
ARC_GEN_TRANS_VADD2_DOP_REG_S12(vsubadd4h);
ARC_GEN_TRANS_VADD2_DOP_COND_REG(vsubadd4h);
ARC_GEN_TRANS_VADD2_DOP_COND_U6(vsubadd4h);

/*
 * Compute the two 16x16 products of a dual 16-bit vector multiply.
 *
 * p0 = b.h0 * c.h0
 * p1 = b.h1 * c.h1
 */
static void arc_gen_vmpy2h_products(TCGv_i32 p0, TCGv_i32 p1,
                                    TCGv_i32 b, TCGv_i32 c, bool sign)
{
    TCGv_i32 bh = tcg_temp_new_i32();
    TCGv_i32 ch = tcg_temp_new_i32();

    if (sign) {
        tcg_gen_sextract_i32(bh, b, 0, 16);
        tcg_gen_sextract_i32(ch, c, 0, 16);
    } else {
        tcg_gen_extract_i32(bh, b, 0, 16);
        tcg_gen_extract_i32(ch, c, 0, 16);
    }
    tcg_gen_mul_i32(p0, bh, ch);

    if (sign) {
        tcg_gen_sextract_i32(bh, b, 16, 16);
        tcg_gen_sextract_i32(ch, c, 16, 16);
    } else {
        tcg_gen_extract_i32(bh, b, 16, 16);
        tcg_gen_extract_i32(ch, c, 16, 16);
    }
    tcg_gen_mul_i32(p1, bh, ch);
}

/*
 * VMPY2H, dual signed 16-bit SIMD multiplication
 *
 * acc.w0 = A.w0 = b.h0 * c.h0
 * acc.w1 = A.w1 = b.h1 * c.h1
 */
static void arc_gen_vmpy2h(DisasContext *ctx, TCGv_i64 a, TCGv_i32 b, TCGv_i32 c)
{
    TCGv_i32 p0 = tcg_temp_new_i32();
    TCGv_i32 p1 = tcg_temp_new_i32();

    arc_gen_vmpy2h_products(p0, p1, b, c, true);

    tcg_gen_concat_i32_i64(a, p0, p1);
    arc_gen_save_accumulator(a);
}

ARC_GEN_TRANS_VMPY2H_DOP_REG_REG(vmpy2h);
ARC_GEN_TRANS_VMPY2H_DOP_REG_U6(vmpy2h);
ARC_GEN_TRANS_VMPY2H_DOP_REG_S12(vmpy2h);
ARC_GEN_TRANS_VMPY2H_DOP_COND_REG(vmpy2h);
ARC_GEN_TRANS_VMPY2H_DOP_COND_U6(vmpy2h);

/*
 * VMPY2HU, dual unsigned 16-bit SIMD multiplication
 *
 * acc.w0 = A.w0 = b.h0 * c.h0
 * acc.w1 = A.w1 = b.h1 * c.h1
 */
static void arc_gen_vmpy2hu(DisasContext *ctx, TCGv_i64 a, TCGv_i32 b, TCGv_i32 c)
{
    TCGv_i32 p0 = tcg_temp_new_i32();
    TCGv_i32 p1 = tcg_temp_new_i32();

    arc_gen_vmpy2h_products(p0, p1, b, c, false);

    tcg_gen_concat_i32_i64(a, p0, p1);
    arc_gen_save_accumulator(a);
}

ARC_GEN_TRANS_VMPY2H_DOP_REG_REG(vmpy2hu);
ARC_GEN_TRANS_VMPY2H_DOP_REG_U6(vmpy2hu);
ARC_GEN_TRANS_VMPY2H_DOP_REG_S12(vmpy2hu);
ARC_GEN_TRANS_VMPY2H_DOP_COND_REG(vmpy2hu);
ARC_GEN_TRANS_VMPY2H_DOP_COND_U6(vmpy2hu);

/*
 * Dual 16-bit multiply and accumulate
 *
 * A.w0 = acc.w0 += b.h0 * c.h0
 * A.w1 = acc.w1 += b.h1 * c.h1
 */
static void arc_gen_vmac2h_common(DisasContext *ctx, TCGv_i64 a, TCGv_i32 b,
                           TCGv_i32 c, bool sign)
{
    TCGv_i32 p0 = tcg_temp_new_i32();
    TCGv_i32 p1 = tcg_temp_new_i32();
    TCGv_i32 acc0 = tcg_temp_new_i32();
    TCGv_i32 acc1 = tcg_temp_new_i32();
    TCGv_i64 acc = arc_gen_load_accumulator();

    arc_gen_vmpy2h_products(p0, p1, b, c, sign);

    tcg_gen_extrl_i64_i32(acc0, acc);
    tcg_gen_extrh_i64_i32(acc1, acc);
    tcg_gen_add_i32(acc0, acc0, p0);
    tcg_gen_add_i32(acc1, acc1, p1);

    tcg_gen_concat_i32_i64(a, acc0, acc1);
    arc_gen_save_accumulator(a);
}

/*
 * VMAC2H, dual signed 16-bit multiply and accumulate
 */
static void arc_gen_vmac2h(DisasContext *ctx, TCGv_i64 a, TCGv_i32 b, TCGv_i32 c)
{
    arc_gen_vmac2h_common(ctx, a, b, c, true);
}

ARC_GEN_TRANS_VMPY2H_DOP_REG_REG(vmac2h);
ARC_GEN_TRANS_VMPY2H_DOP_REG_U6(vmac2h);
ARC_GEN_TRANS_VMPY2H_DOP_REG_S12(vmac2h);
ARC_GEN_TRANS_VMPY2H_DOP_COND_REG(vmac2h);
ARC_GEN_TRANS_VMPY2H_DOP_COND_U6(vmac2h);

/*
 * VMAC2HU, dual unsigned 16-bit multiply and accumulate
 */
static void arc_gen_vmac2hu(DisasContext *ctx, TCGv_i64 a, TCGv_i32 b, TCGv_i32 c)
{
    arc_gen_vmac2h_common(ctx, a, b, c, false);
}

ARC_GEN_TRANS_VMPY2H_DOP_REG_REG(vmac2hu);
ARC_GEN_TRANS_VMPY2H_DOP_REG_U6(vmac2hu);
ARC_GEN_TRANS_VMPY2H_DOP_REG_S12(vmac2hu);
ARC_GEN_TRANS_VMPY2H_DOP_COND_REG(vmac2hu);
ARC_GEN_TRANS_VMPY2H_DOP_COND_U6(vmac2hu);

#if defined(TARGET_ARCV3_32) || defined(TARGET_ARCV3_64)
/*
 * VMAX2, dual 32-bit signed vector maximum
 *
 * A.w0 = max(B.w0, C.w0)
 * A.w1 = max(B.w1, C.w1)
 */
static void arc_gen_vmax2(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c)
{
    TCGv_i64 b0 = tcg_temp_new_i64();
    TCGv_i64 b1 = tcg_temp_new_i64();
    TCGv_i64 c0 = tcg_temp_new_i64();
    TCGv_i64 c1 = tcg_temp_new_i64();

    tcg_gen_sextract_i64(b0, b, 0, 32);
    tcg_gen_sextract_i64(b1, b, 32, 32);
    tcg_gen_sextract_i64(c0, c, 0, 32);
    tcg_gen_sextract_i64(c1, c, 32, 32);

    tcg_gen_smax_i64(b0, b0, c0);
    tcg_gen_smax_i64(b1, b1, c1);

    tcg_gen_deposit_i64(a, b0, b1, 32, 32);
}

ARC_GEN_TRANS_VADD2_DOP_REG_REG(vmax2);
ARC_GEN_TRANS_VADD2_DOP_REG_U6(vmax2);
ARC_GEN_TRANS_VADD2_DOP_REG_S12(vmax2);
ARC_GEN_TRANS_VADD2_DOP_COND_REG(vmax2);
ARC_GEN_TRANS_VADD2_DOP_COND_U6(vmax2);

/*
 * VMIN2, dual 32-bit signed vector minimum
 *
 * A.w0 = min(B.w0, C.w0)
 * A.w1 = min(B.w1, C.w1)
 */
static void arc_gen_vmin2(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c)
{
    TCGv_i64 b0 = tcg_temp_new_i64();
    TCGv_i64 b1 = tcg_temp_new_i64();
    TCGv_i64 c0 = tcg_temp_new_i64();
    TCGv_i64 c1 = tcg_temp_new_i64();

    tcg_gen_sextract_i64(b0, b, 0, 32);
    tcg_gen_sextract_i64(b1, b, 32, 32);
    tcg_gen_sextract_i64(c0, c, 0, 32);
    tcg_gen_sextract_i64(c1, c, 32, 32);

    tcg_gen_smin_i64(b0, b0, c0);
    tcg_gen_smin_i64(b1, b1, c1);

    tcg_gen_deposit_i64(a, b0, b1, 32, 32);
}

ARC_GEN_TRANS_VADD2_DOP_REG_REG(vmin2);
ARC_GEN_TRANS_VADD2_DOP_REG_U6(vmin2);
ARC_GEN_TRANS_VADD2_DOP_REG_S12(vmin2);
ARC_GEN_TRANS_VADD2_DOP_COND_REG(vmin2);
ARC_GEN_TRANS_VADD2_DOP_COND_U6(vmin2);

/* a.h0 = c.h0; a.h1 = b.h0 */
static void arc_gen_vpack2hl(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_andi_i32(a, c, 0xFFFF);
    tcg_gen_shli_i32(b, b, 16);
    tcg_gen_or_i32(a, a, b);
}

ARC_DEF_TRANS_DOP_REG_REG(vpack2hl)
ARC_DEF_TRANS_DOP_REG_U6(vpack2hl)
ARC_DEF_TRANS_DOP_REG_S12(vpack2hl)
ARC_DEF_TRANS_DOP_COND_REG(vpack2hl)
ARC_DEF_TRANS_DOP_COND_U6(vpack2hl)

/* a.h0 = c.h1; a.h1 = b.h1 */
static void arc_gen_vpack2hm(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_andi_i32(b, b, 0xFFFF0000);
    tcg_gen_shri_i32(c, c, 16);
    tcg_gen_or_i32(a, b, c);
}

ARC_DEF_TRANS_DOP_REG_REG(vpack2hm)
ARC_DEF_TRANS_DOP_REG_U6(vpack2hm)
ARC_DEF_TRANS_DOP_REG_S12(vpack2hm)
ARC_DEF_TRANS_DOP_COND_REG(vpack2hm)
ARC_DEF_TRANS_DOP_COND_U6(vpack2hm)
#else
ARC_DEF_TRANS_STUB(vmax2_dop_reg_reg)
ARC_DEF_TRANS_STUB(vmax2_dop_reg_u6)
ARC_DEF_TRANS_STUB(vmax2_dop_reg_s12)
ARC_DEF_TRANS_STUB(vmax2_dop_cond_reg)
ARC_DEF_TRANS_STUB(vmax2_dop_cond_u6)

ARC_DEF_TRANS_STUB(vmin2_dop_reg_reg)
ARC_DEF_TRANS_STUB(vmin2_dop_reg_u6)
ARC_DEF_TRANS_STUB(vmin2_dop_reg_s12)
ARC_DEF_TRANS_STUB(vmin2_dop_cond_reg)
ARC_DEF_TRANS_STUB(vmin2_dop_cond_u6)

ARC_DEF_TRANS_STUB(vpack2hl_dop_reg_reg)
ARC_DEF_TRANS_STUB(vpack2hl_dop_reg_u6)
ARC_DEF_TRANS_STUB(vpack2hl_dop_reg_s12)
ARC_DEF_TRANS_STUB(vpack2hl_dop_cond_reg)
ARC_DEF_TRANS_STUB(vpack2hl_dop_cond_u6)

ARC_DEF_TRANS_STUB(vpack2hm_dop_reg_reg)
ARC_DEF_TRANS_STUB(vpack2hm_dop_reg_u6)
ARC_DEF_TRANS_STUB(vpack2hm_dop_reg_s12)
ARC_DEF_TRANS_STUB(vpack2hm_dop_cond_reg)
ARC_DEF_TRANS_STUB(vpack2hm_dop_cond_u6)
#endif

#if defined(TARGET_ARCV3_64)
static void arc64_gen_vpack4hl(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 bh0 = tcg_temp_new_i64();
    TCGv_i64 bh2 = tcg_temp_new_i64();
    TCGv_i64 ch0 = tcg_temp_new_i64();
    TCGv_i64 ch2 = tcg_temp_new_i64();

    tcg_gen_extract_i64(bh0, b, 0, 16);
    tcg_gen_extract_i64(bh2, b, 32, 16);
    tcg_gen_extract_i64(ch0, c, 0, 16);
    tcg_gen_extract_i64(ch2, c, 32, 16);

    tcg_gen_mov_i64(a, bh0);
    tcg_gen_deposit_i64(a, a, bh2, 16, 16);
    tcg_gen_deposit_i64(a, a, ch0, 32, 16);
    tcg_gen_deposit_i64(a, a, ch2, 48, 16);
}

ARC64_DEF_TRANS_DOP_REG_REG(vpack4hl)
ARC64_DEF_TRANS_DOP_REG_U6(vpack4hl)
ARC64_DEF_TRANS_DOP_REG_S12(vpack4hl)
ARC64_DEF_TRANS_DOP_COND_REG(vpack4hl)
ARC64_DEF_TRANS_DOP_COND_U6(vpack4hl)

static void arc64_gen_vpack4hm(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 bh1 = tcg_temp_new_i64();
    TCGv_i64 bh3 = tcg_temp_new_i64();
    TCGv_i64 ch1 = tcg_temp_new_i64();
    TCGv_i64 ch3 = tcg_temp_new_i64();

    tcg_gen_extract_i64(bh1, b, 16, 16);
    tcg_gen_extract_i64(bh3, b, 48, 16);
    tcg_gen_extract_i64(ch1, c, 16, 16);
    tcg_gen_extract_i64(ch3, c, 48, 16);

    tcg_gen_mov_i64(a, bh1);
    tcg_gen_deposit_i64(a, a, bh3, 16, 16);
    tcg_gen_deposit_i64(a, a, ch1, 32, 16);
    tcg_gen_deposit_i64(a, a, ch3, 48, 16);
}

ARC64_DEF_TRANS_DOP_REG_REG(vpack4hm)
ARC64_DEF_TRANS_DOP_REG_U6(vpack4hm)
ARC64_DEF_TRANS_DOP_REG_S12(vpack4hm)
ARC64_DEF_TRANS_DOP_COND_REG(vpack4hm)
ARC64_DEF_TRANS_DOP_COND_U6(vpack4hm)

/* a = (c.w0 << 32) | b.w0 */
static void arc64_gen_vpack2wl(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    tcg_gen_andi_i64(a, b, (target_ulong) 0xFFFFFFFF);
    tcg_gen_shli_i64(c, c, 32);
    tcg_gen_or_i64(a, a, c);
}

ARC64_DEF_TRANS_DOP_REG_REG(vpack2wl)
ARC64_DEF_TRANS_DOP_REG_U6(vpack2wl)
ARC64_DEF_TRANS_DOP_REG_S12(vpack2wl)
ARC64_DEF_TRANS_DOP_COND_REG(vpack2wl)
ARC64_DEF_TRANS_DOP_COND_U6(vpack2wl)

/* a = (c.w1 << 32) | b.w1 */
static void arc64_gen_vpack2wm(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    tcg_gen_andi_i64(c, c, 0xFFFFFFFF00000000ULL);
    tcg_gen_shri_i64(b, b, 32);
    tcg_gen_or_i64(a, b, c);
}

ARC64_DEF_TRANS_DOP_REG_REG(vpack2wm)
ARC64_DEF_TRANS_DOP_REG_U6(vpack2wm)
ARC64_DEF_TRANS_DOP_REG_S12(vpack2wm)
ARC64_DEF_TRANS_DOP_COND_REG(vpack2wm)
ARC64_DEF_TRANS_DOP_COND_U6(vpack2wm)
#else
ARC_DEF_TRANS_STUB(vpack4hl_dop_reg_reg)
ARC_DEF_TRANS_STUB(vpack4hl_dop_reg_u6)
ARC_DEF_TRANS_STUB(vpack4hl_dop_reg_s12)
ARC_DEF_TRANS_STUB(vpack4hl_dop_cond_reg)
ARC_DEF_TRANS_STUB(vpack4hl_dop_cond_u6)

ARC_DEF_TRANS_STUB(vpack4hm_dop_reg_reg)
ARC_DEF_TRANS_STUB(vpack4hm_dop_reg_u6)
ARC_DEF_TRANS_STUB(vpack4hm_dop_reg_s12)
ARC_DEF_TRANS_STUB(vpack4hm_dop_cond_reg)
ARC_DEF_TRANS_STUB(vpack4hm_dop_cond_u6)

ARC_DEF_TRANS_STUB(vpack2wl_dop_reg_reg)
ARC_DEF_TRANS_STUB(vpack2wl_dop_reg_u6)
ARC_DEF_TRANS_STUB(vpack2wl_dop_reg_s12)
ARC_DEF_TRANS_STUB(vpack2wl_dop_cond_reg)
ARC_DEF_TRANS_STUB(vpack2wl_dop_cond_u6)

ARC_DEF_TRANS_STUB(vpack2wm_dop_reg_reg)
ARC_DEF_TRANS_STUB(vpack2wm_dop_reg_u6)
ARC_DEF_TRANS_STUB(vpack2wm_dop_reg_s12)
ARC_DEF_TRANS_STUB(vpack2wm_dop_cond_reg)
ARC_DEF_TRANS_STUB(vpack2wm_dop_cond_u6)
#endif

#ifdef TARGET_ARCV3_32
/* b = (c << 16) | (c & 0x0000_ffff); */
static void arc_gen_vrep2hl(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c)
{
    tcg_gen_andi_i32(c, c, 0xFFFF);
    tcg_gen_shli_i32(b, c, 16);
    tcg_gen_or_i32(b, b, c);
}

ARC_DEF_TRANS_R32_R32(vrep2hl, vrep2hl_dop_reg_reg, a->b, a->c)
ARC_DEF_TRANS_R32_I32(vrep2hl, vrep2hl_dop_reg_u6, a->b, a->imm)
#else
ARC_DEF_TRANS_STUB(vrep2hl_dop_reg_reg)
ARC_DEF_TRANS_STUB(vrep2hl_dop_reg_u6)
#endif
