#include "translate.h"

/*
 * TCG op signature shared by the four scalar div/rem primitives:
 *
 *   tcg_gen_div_tl   - signed division
 *   tcg_gen_divu_tl  - unsigned division
 *   tcg_gen_rem_tl   - signed remainder (C99 trunc-towards-zero semantics)
 *   tcg_gen_remu_tl  - unsigned remainder
 */
typedef void (*ArcDivremOp)(TCGv_i32, TCGv_i32, TCGv_i32);

/*
 * Shared back-end for the four DIV / REM flavors.
 *
 *   1. src2 == 0 && STATUS32.DZ == 1: raise EV_DivZero exception
 *   2. src2 == 0 (DZ == 0): goto guard_fail
 *   3. signed-only: also bypass src1 == INT_MIN && src2 == -1
 *   4. guard-pass:  dst = op(src1, src2), .f gives standard Z/N/V update
 *   5. guard-fail:  dst = 0, .f gives Z := 0, N := 0, V := 1 (C kept)
 */
static void arc_gen_divrem_common(DisasContext *ctx, TCGv_i32 dst,
                              TCGv_i32 src1, TCGv_i32 src2,
                              bool sign, ArcDivremOp op,
                              bool flag_setting)
{
    TCGLabel *guard_fail = gen_new_label();
    TCGLabel *end = gen_new_label();

    /* (1) src2 != 0 guard. On src2 == 0, consult DZf. */
    TCGLabel *src2_nonzero = gen_new_label();
    tcg_gen_brcondi_i32(TCG_COND_NE, src2, 0, src2_nonzero);

    /* src2 == 0: take EV_DivZero if STATUS32.DZ == 1, else guard_fail. */
    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_DZf, 0, guard_fail);

    arc_gen_exception_divzero(ctx);

    gen_set_label(src2_nonzero);

    if (sign) {
        /*
         * (3) signed-overflow guard. We only need the src2 == -1 test
         * when src1 actually is INT_MIN; the do_op fallthrough label
         * skips it otherwise.
         */
        TCGLabel *do_op = gen_new_label();
        tcg_gen_brcondi_i32(TCG_COND_NE, src1, INT32_MIN, do_op);
        tcg_gen_brcondi_i32(TCG_COND_EQ, src2, (int32_t)-1, guard_fail);
        gen_set_label(do_op);
    }

    /* (4) Guard passed. */
    op(dst, src1, src2);
    if (flag_setting) {
        tcg_gen_setcondi_i32(TCG_COND_EQ, cpu_Zf, dst, 0);
        if (sign) {
            tcg_gen_shri_i32(cpu_Nf, dst, TARGET_LONG_BITS_32 - 1);
        } else {
            tcg_gen_movi_i32(cpu_Nf, 0);
        }
        tcg_gen_movi_i32(cpu_Vf, 0);
    }
    tcg_gen_br(end);

    /* (5) Guard failed without exception. */
    gen_set_label(guard_fail);
    tcg_gen_movi_i32(dst, 0);
    if (flag_setting) {
        tcg_gen_movi_i32(cpu_Zf, 0);
        tcg_gen_movi_i32(cpu_Nf, 0);
        tcg_gen_movi_i32(cpu_Vf, 1);
    }

    gen_set_label(end);
}

static void arc_gen_div(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool flag_setting)
{
    arc_gen_divrem_common(ctx, a, b, c, true, tcg_gen_div_i32, flag_setting);
}

ARC_DEF_TRANS_DOP_REG_REG(div)
ARC_DEF_TRANS_DOP_REG_U6(div)
ARC_DEF_TRANS_DOP_REG_S12(div)
ARC_DEF_TRANS_DOP_COND_REG(div)
ARC_DEF_TRANS_DOP_COND_U6(div)

static void arc_gen_divu(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool flag_setting)
{
    arc_gen_divrem_common(ctx, a, b, c, false, tcg_gen_divu_i32, flag_setting);
}

ARC_DEF_TRANS_DOP_REG_REG(divu)
ARC_DEF_TRANS_DOP_REG_U6(divu)
ARC_DEF_TRANS_DOP_REG_S12(divu)
ARC_DEF_TRANS_DOP_COND_REG(divu)
ARC_DEF_TRANS_DOP_COND_U6(divu)

static void arc_gen_rem(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool flag_setting)
{
    arc_gen_divrem_common(ctx, a, b, c, true, tcg_gen_rem_i32, flag_setting);
}

ARC_DEF_TRANS_DOP_REG_REG(rem)
ARC_DEF_TRANS_DOP_REG_U6(rem)
ARC_DEF_TRANS_DOP_REG_S12(rem)
ARC_DEF_TRANS_DOP_COND_REG(rem)
ARC_DEF_TRANS_DOP_COND_U6(rem)

static void arc_gen_remu(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool flag_setting)
{
    arc_gen_divrem_common(ctx, a, b, c, false, tcg_gen_remu_i32, flag_setting);
}

ARC_DEF_TRANS_DOP_REG_REG(remu)
ARC_DEF_TRANS_DOP_REG_U6(remu)
ARC_DEF_TRANS_DOP_REG_S12(remu)
ARC_DEF_TRANS_DOP_COND_REG(remu)
ARC_DEF_TRANS_DOP_COND_U6(remu)

#ifdef TARGET_ARCV3_64
typedef void (*ArcDivremOp64)(TCGv_i64, TCGv_i64, TCGv_i64);

static void arc64_gen_divrem_common(DisasContext *ctx, TCGv_i64 dst,
                                    TCGv_i64 src1, TCGv_i64 src2,
                                    bool sign, ArcDivremOp64 op,
                                    bool flag_setting)
{
    TCGLabel *guard_fail = gen_new_label();
    TCGLabel *end = gen_new_label();
    TCGLabel *src2_nonzero = gen_new_label();

    tcg_gen_brcondi_i64(TCG_COND_NE, src2, 0, src2_nonzero);
    tcg_gen_brcondi_i32(TCG_COND_EQ, cpu_DZf, 0, guard_fail);
    arc_gen_exception_divzero(ctx);
    gen_set_label(src2_nonzero);

    if (sign) {
        TCGLabel *do_op = gen_new_label();

        tcg_gen_brcondi_i64(TCG_COND_NE, src1, INT64_MIN, do_op);
        tcg_gen_brcondi_i64(TCG_COND_EQ, src2, -1, guard_fail);
        gen_set_label(do_op);
    }

    op(dst, src1, src2);
    if (flag_setting) {
        arc64_gen_set_flag_Z(dst);
        if (sign) {
            arc64_gen_set_flag_N(dst);
        } else {
            tcg_gen_movi_i32(cpu_Nf, 0);
        }
        tcg_gen_movi_i32(cpu_Vf, 0);
    }
    tcg_gen_br(end);

    gen_set_label(guard_fail);
    tcg_gen_movi_i64(dst, 0);
    if (flag_setting) {
        tcg_gen_movi_i32(cpu_Zf, 0);
        tcg_gen_movi_i32(cpu_Nf, 0);
        tcg_gen_movi_i32(cpu_Vf, 1);
    }

    gen_set_label(end);
}

static void arc64_gen_divl(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                           bool set_flags)
{
    arc64_gen_divrem_common(ctx, a, b, c, true, tcg_gen_div_i64, set_flags);
}

static void arc64_gen_divul(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                            bool set_flags)
{
    arc64_gen_divrem_common(ctx, a, b, c, false, tcg_gen_divu_i64, set_flags);
}

static void arc64_gen_reml(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                           bool set_flags)
{
    arc64_gen_divrem_common(ctx, a, b, c, true, tcg_gen_rem_i64, set_flags);
}

static void arc64_gen_remul(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c,
                            bool set_flags)
{
    arc64_gen_divrem_common(ctx, a, b, c, false, tcg_gen_remu_i64, set_flags);
}

ARC64_DEF_TRANS_DOP_REG_REG(divl)
ARC64_DEF_TRANS_DOP_REG_U6(divl)
ARC64_DEF_TRANS_DOP_REG_S12(divl)
ARC64_DEF_TRANS_DOP_COND_REG(divl)
ARC64_DEF_TRANS_DOP_COND_U6(divl)

ARC64_DEF_TRANS_DOP_REG_REG(divul)
ARC64_DEF_TRANS_DOP_REG_U6(divul)
ARC64_DEF_TRANS_DOP_REG_S12(divul)
ARC64_DEF_TRANS_DOP_COND_REG(divul)
ARC64_DEF_TRANS_DOP_COND_U6(divul)

ARC64_DEF_TRANS_DOP_REG_REG(reml)
ARC64_DEF_TRANS_DOP_REG_U6(reml)
ARC64_DEF_TRANS_DOP_REG_S12(reml)
ARC64_DEF_TRANS_DOP_COND_REG(reml)
ARC64_DEF_TRANS_DOP_COND_U6(reml)

ARC64_DEF_TRANS_DOP_REG_REG(remul)
ARC64_DEF_TRANS_DOP_REG_U6(remul)
ARC64_DEF_TRANS_DOP_REG_S12(remul)
ARC64_DEF_TRANS_DOP_COND_REG(remul)
ARC64_DEF_TRANS_DOP_COND_U6(remul)
#else
ARC_DEF_TRANS_STUB(divl_dop_reg_reg)
ARC_DEF_TRANS_STUB(divl_dop_reg_u6)
ARC_DEF_TRANS_STUB(divl_dop_reg_s12)
ARC_DEF_TRANS_STUB(divl_dop_cond_reg)
ARC_DEF_TRANS_STUB(divl_dop_cond_u6)

ARC_DEF_TRANS_STUB(divul_dop_reg_reg)
ARC_DEF_TRANS_STUB(divul_dop_reg_u6)
ARC_DEF_TRANS_STUB(divul_dop_reg_s12)
ARC_DEF_TRANS_STUB(divul_dop_cond_reg)
ARC_DEF_TRANS_STUB(divul_dop_cond_u6)

ARC_DEF_TRANS_STUB(reml_dop_reg_reg)
ARC_DEF_TRANS_STUB(reml_dop_reg_u6)
ARC_DEF_TRANS_STUB(reml_dop_reg_s12)
ARC_DEF_TRANS_STUB(reml_dop_cond_reg)
ARC_DEF_TRANS_STUB(reml_dop_cond_u6)

ARC_DEF_TRANS_STUB(remul_dop_reg_reg)
ARC_DEF_TRANS_STUB(remul_dop_reg_u6)
ARC_DEF_TRANS_STUB(remul_dop_reg_s12)
ARC_DEF_TRANS_STUB(remul_dop_cond_reg)
ARC_DEF_TRANS_STUB(remul_dop_cond_u6)
#endif
