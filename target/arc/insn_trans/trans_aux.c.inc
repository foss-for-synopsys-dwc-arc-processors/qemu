#include "translate.h"

static void arc_gen_lr(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c)
{
    gen_helper_aux_reg_read_i32(b, tcg_env, c);
}

static bool trans_lr_dop_reg_reg(DisasContext *ctx, arg_lr_dop_reg_reg *a) {
    TCGv_i32 reg_r0 = arc_insn_load_dst_reg(ctx, a->b);
    TCGv_i32 reg_r1 = arc_insn_load_src_reg(ctx, a->c);

    arc_gen_lr(ctx, reg_r0, reg_r1);
    arc_insn_save_reg(ctx, a->b, reg_r0);

    return true;
}

static bool trans_lr_dop_reg_u6(DisasContext *ctx, arg_lr_dop_reg_u6 *a) {
    TCGv_i32 reg_r0 = arc_insn_load_dst_reg(ctx, a->b);
    TCGv_i32 reg_r1 = arc_insn_load_imm(a->imm);

    arc_gen_lr(ctx, reg_r0, reg_r1);
    arc_insn_save_reg(ctx, a->b, reg_r0);

    return true;
}

static bool trans_lr_dop_reg_s12(DisasContext *ctx, arg_lr_dop_reg_s12 *a) {
    TCGv_i32 reg_r0 = arc_insn_load_dst_reg(ctx, a->b);
    TCGv_i32 reg_r1 = arc_insn_load_imm(a->imm);

    arc_gen_lr(ctx, reg_r0, reg_r1);
    arc_insn_save_reg(ctx, a->b, reg_r0);

    return true;
}

#ifdef TARGET_ARCV3_64
static void arc64_gen_lrl(DisasContext *ctx, TCGv_i64 b, TCGv_i64 c)
{
    gen_helper_aux_reg_read_i64(b, tcg_env, c);
}

static bool trans_lrl_dop_reg_reg(DisasContext *ctx, arg_lrl_dop_reg_reg *a) {
    TCGv_i64 reg_r0 = arc64_insn_load_dst_reg(ctx, a->b);
    TCGv_i64 reg_r1 = arc64_insn_load_src_reg(ctx, a->c);

    arc64_gen_lrl(ctx, reg_r0, reg_r1);
    arc64_insn_save_reg(ctx, a->b, reg_r0);

    return true;
}

static bool trans_lrl_dop_reg_u6(DisasContext *ctx, arg_lrl_dop_reg_u6 *a) {
    TCGv_i64 reg_r0 = arc64_insn_load_dst_reg(ctx, a->b);
    TCGv_i64 reg_r1 = arc64_insn_load_imm(a->imm);

    arc64_gen_lrl(ctx, reg_r0, reg_r1);
    arc64_insn_save_reg(ctx, a->b, reg_r0);

    return true;
}

static bool trans_lrl_dop_reg_s12(DisasContext *ctx, arg_lrl_dop_reg_s12 *a) {
    TCGv_i64 reg_r0 = arc64_insn_load_dst_reg(ctx, a->b);
    TCGv_i64 reg_r1 = arc64_insn_load_imm(a->imm);

    arc64_gen_lrl(ctx, reg_r0, reg_r1);
    arc64_insn_save_reg(ctx, a->b, reg_r0);

    return true;
}
#else
ARC_DEF_TRANS_STUB(lrl_dop_reg_reg)
ARC_DEF_TRANS_STUB(lrl_dop_reg_u6)
ARC_DEF_TRANS_STUB(lrl_dop_reg_s12)
#endif

static void arc_gen_sr(DisasContext *ctx, TCGv_i32 aux_regnum, TCGv_i32 value)
{
    gen_helper_aux_reg_write_i32(tcg_env, aux_regnum, value);

    ctx->base.is_jmp = DISAS_UPDATE;
}

static bool trans_sr_dop_reg_reg(DisasContext *ctx, arg_sr_dop_reg_reg *a)
{
    TCGv_i32 reg_value = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_aux_regnum = arc_insn_load_src_reg(ctx, a->c);

    arc_gen_sr(ctx, reg_aux_regnum, reg_value);

    return true;
}

static bool trans_sr_dop_reg_u6(DisasContext *ctx, arg_sr_dop_reg_u6 *a)
{
    TCGv_i32 reg_value = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_aux_regnum = arc_insn_load_imm(a->imm);

    arc_gen_sr(ctx, reg_aux_regnum, reg_value);

    return true;
}

static bool trans_sr_dop_reg_s12(DisasContext *ctx, arg_sr_dop_reg_s12 *a)
{
    TCGv_i32 reg_value = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_aux_regnum = arc_insn_load_imm(a->imm);

    arc_gen_sr(ctx, reg_aux_regnum, reg_value);

    return true;
}

#ifdef TARGET_ARCV3_64
static void arc64_gen_srl(DisasContext *ctx, TCGv_i64 aux_regnum, TCGv_i64 value)
{
    gen_helper_aux_reg_write_i64(tcg_env, aux_regnum, value);

    ctx->base.is_jmp = DISAS_UPDATE;
}

static bool trans_srl_dop_reg_reg(DisasContext *ctx, arg_srl_dop_reg_reg *a)
{
    TCGv_i64 reg_value = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_aux_regnum = arc64_insn_load_src_reg(ctx, a->c);

    arc64_gen_srl(ctx, reg_aux_regnum, reg_value);

    return true;
}

static bool trans_srl_dop_reg_u6(DisasContext *ctx, arg_srl_dop_reg_u6 *a)
{
    TCGv_i64 reg_value = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_aux_regnum = arc64_insn_load_imm(a->imm);

    arc64_gen_srl(ctx, reg_aux_regnum, reg_value);

    return true;
}

static bool trans_srl_dop_reg_s12(DisasContext *ctx, arg_srl_dop_reg_s12 *a)
{
    TCGv_i64 reg_value = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_aux_regnum = arc64_insn_load_imm(a->imm);

    arc64_gen_srl(ctx, reg_aux_regnum, reg_value);

    return true;
}
#else
ARC_DEF_TRANS_STUB(srl_dop_reg_reg)
ARC_DEF_TRANS_STUB(srl_dop_reg_u6)
ARC_DEF_TRANS_STUB(srl_dop_reg_s12)
#endif

static void arc_gen_aex(DisasContext *ctx, TCGv_i32 gpr_reg, TCGv_i32 aux_regnum, int cond)
{
    TCGv_i32 tmp = tcg_temp_new_i32();
    TCGLabel *skip = gen_new_label();

    arc_gen_cc_prologue(ctx, cond, skip);
    gen_helper_aux_reg_read_i32(tmp, tcg_env, aux_regnum);
    gen_helper_aux_reg_write_i32(tcg_env, aux_regnum, gpr_reg);
    tcg_gen_mov_i32(gpr_reg, tmp);
    gen_set_label(skip);

    ctx->base.is_jmp = DISAS_UPDATE;
}

/* AEX b,[c] */
static bool trans_aex_dop_reg_reg(DisasContext *ctx, arg_aex_dop_reg_reg *a)
{
    arc_gen_check_not_limm(ctx, a->b);

    TCGv_i32 reg_b = arc_insn_load_dst_reg(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c);

    arc_gen_aex(ctx, reg_b, reg_c, ARC_CC_AL);

    arc_insn_save_reg(ctx, a->b, reg_b);

    return true;
}

/* AEX b,[u6] */
static bool trans_aex_dop_reg_u6(DisasContext *ctx, arg_aex_dop_reg_u6 *a)
{
    arc_gen_check_not_limm(ctx, a->b);

    TCGv_i32 reg_b = arc_insn_load_dst_reg(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_imm(a->imm);

    arc_gen_aex(ctx, reg_b, reg_c, ARC_CC_AL);

    arc_insn_save_reg(ctx, a->b, reg_b);

    return true;
}

/* AEX b,[s12] */
static bool trans_aex_dop_reg_s12(DisasContext *ctx, arg_aex_dop_reg_s12 *a)
{
    arc_gen_check_not_limm(ctx, a->b);

    TCGv_i32 reg_b = arc_insn_load_dst_reg(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_imm(a->imm);

    arc_gen_aex(ctx, reg_b, reg_c, ARC_CC_AL);

    arc_insn_save_reg(ctx, a->b, reg_b);

    return true;
}

/* AEX<.cc> b,[c] */
static bool trans_aex_dop_cond_reg(DisasContext *ctx, arg_aex_dop_cond_reg *a)
{
    arc_gen_check_not_limm(ctx, a->b);

    TCGv_i32 reg_b = arc_insn_load_dst_reg(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c);

    arc_gen_aex(ctx, reg_b, reg_c, a->cond);

    arc_insn_save_reg(ctx, a->b, reg_b);

    return true;
}

/* AEX<.cc> b,[u6] */
static bool trans_aex_dop_cond_u6(DisasContext *ctx, arg_aex_dop_cond_u6 *a)
{
    arc_gen_check_not_limm(ctx, a->b);

    TCGv_i32 reg_b = arc_insn_load_dst_reg(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_imm(a->imm);

    arc_gen_aex(ctx, reg_b, reg_c, a->cond);

    arc_insn_save_reg(ctx, a->b, reg_b);

    return true;
}

#ifdef TARGET_ARCV3_64
static void arc64_gen_aexl(DisasContext *ctx, TCGv_i64 gpr_reg, TCGv_i64 aux_regnum, int cond)
{
    TCGv_i64 tmp = tcg_temp_new_i64();
    TCGLabel *skip = gen_new_label();

    arc_gen_cc_prologue(ctx, cond, skip);
    gen_helper_aux_reg_read_i64(tmp, tcg_env, aux_regnum);
    gen_helper_aux_reg_write_i64(tcg_env, aux_regnum, gpr_reg);
    tcg_gen_mov_i64(gpr_reg, tmp);
    gen_set_label(skip);

    ctx->base.is_jmp = DISAS_UPDATE;
}

/* AEX b,[c] */
static bool trans_aexl_dop_reg_reg(DisasContext *ctx, arg_aexl_dop_reg_reg *a)
{
    arc_gen_check_not_limm(ctx, a->b);

    TCGv_i64 reg_b = arc64_insn_load_dst_reg(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_src_reg(ctx, a->c);

    arc64_gen_aexl(ctx, reg_b, reg_c, ARC_CC_AL);

    arc64_insn_save_reg(ctx, a->b, reg_b);

    return true;
}

/* AEX b,[u6] */
static bool trans_aexl_dop_reg_u6(DisasContext *ctx, arg_aexl_dop_reg_u6 *a)
{
    arc_gen_check_not_limm(ctx, a->b);

    TCGv_i64 reg_b = arc64_insn_load_dst_reg(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_imm(a->imm);

    arc64_gen_aexl(ctx, reg_b, reg_c, ARC_CC_AL);

    arc64_insn_save_reg(ctx, a->b, reg_b);

    return true;
}

/* AEX b,[s12] */
static bool trans_aexl_dop_reg_s12(DisasContext *ctx, arg_aexl_dop_reg_s12 *a)
{
    arc_gen_check_not_limm(ctx, a->b);

    TCGv_i64 reg_b = arc64_insn_load_dst_reg(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_imm(a->imm);

    arc64_gen_aexl(ctx, reg_b, reg_c, ARC_CC_AL);

    arc64_insn_save_reg(ctx, a->b, reg_b);

    return true;
}

/* AEX<.cc> b,[c] */
static bool trans_aexl_dop_cond_reg(DisasContext *ctx, arg_aexl_dop_cond_reg *a)
{
    arc_gen_check_not_limm(ctx, a->b);

    TCGv_i64 reg_b = arc64_insn_load_dst_reg(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_src_reg(ctx, a->c);

    arc64_gen_aexl(ctx, reg_b, reg_c, a->cond);

    arc64_insn_save_reg(ctx, a->b, reg_b);

    return true;
}

/* AEX<.cc> b,[u6] */
static bool trans_aexl_dop_cond_u6(DisasContext *ctx, arg_aexl_dop_cond_u6 *a)
{
    arc_gen_check_not_limm(ctx, a->b);

    TCGv_i64 reg_b = arc64_insn_load_dst_reg(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_imm(a->imm);

    arc64_gen_aexl(ctx, reg_b, reg_c, a->cond);

    arc64_insn_save_reg(ctx, a->b, reg_b);

    return true;
}
#else
ARC_DEF_TRANS_STUB(aexl_dop_reg_reg)
ARC_DEF_TRANS_STUB(aexl_dop_reg_u6)
ARC_DEF_TRANS_STUB(aexl_dop_reg_s12)
ARC_DEF_TRANS_STUB(aexl_dop_cond_reg)
ARC_DEF_TRANS_STUB(aexl_dop_cond_u6)
#endif
