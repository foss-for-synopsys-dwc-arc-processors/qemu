#include "translate.h"

/*
 * temp = b ; b = *c; *c = temp
 */
static inline void arc_gen_ex(DisasContext *ctx, TCGv_i32 b_dst, TCGv_i32 b_src, TCGv address)
{
    TCGv_i32 tmp = tcg_temp_new_i32();

    tcg_gen_mov_i32(tmp, b_src);
    tcg_gen_atomic_xchg_i32(b_dst, address, tmp, ctx->mem_idx, MO_TEUL | MO_ALIGN_4);
}

/* EX<.di> b,[c] */
static bool trans_ex_sop_reg_reg(DisasContext *ctx, arg_ex_sop_reg_reg *a)
{
    arc_gen_check_not_limm(ctx, a->b);

    TCGv_i32 b_src = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 b_dst = arc_insn_load_dst_reg(ctx, a->b);
    TCGv address = arc_insn_load_src_reg_tl(ctx, a->c);

    arc_gen_ex(ctx, b_dst, b_src, address);

    arc_insn_save_reg(ctx, a->b, b_dst);

    return true;
}

/* EX<.di> b,[u6] */
static bool trans_ex_sop_reg_u6(DisasContext *ctx, arg_ex_sop_reg_u6 *a)
{
    arc_gen_check_not_limm(ctx, a->b);

    TCGv_i32 b_src = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 b_dst = arc_insn_load_dst_reg(ctx, a->b);
    TCGv address = arc_insn_load_imm_tl(a->imm);

    arc_gen_ex(ctx, b_dst, b_src, address);

    arc_insn_save_reg(ctx, a->b, b_dst);

    return true;
}

static inline void arc64_gen_exl(DisasContext *ctx, TCGv_i64 b_dst, TCGv_i64 b_src, TCGv address)
{
    TCGv_i64 tmp = tcg_temp_new_i64();

    tcg_gen_mov_i64(tmp, b_src);
    tcg_gen_atomic_xchg_i64(b_dst, address, tmp, ctx->mem_idx, MO_TEUQ | MO_ALIGN_8);
}

#ifdef TARGET_ARCV3_64
/* TODO: Are there any other EXL formats? PRM is not clear. */

/* EXL<.aq.rl> b,[c] */
static bool trans_exl(DisasContext *ctx, arg_exl *a)
{
    arc_gen_check_not_limm(ctx, a->b);

    TCGv_i64 b_src = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 b_dst = arc64_insn_load_dst_reg(ctx, a->b);
    TCGv address = arc_insn_load_src_reg_tl(ctx, a->c);

    arc64_gen_exl(ctx, b_dst, b_src, address);

    arc64_insn_save_reg(ctx, a->b, b_dst);

    return true;
}
#else
ARC_DEF_TRANS_STUB(exl)
#endif

static bool trans_llock_reg(DisasContext *ctx, arg_llock_reg *a)
{
    TCGv_i32 reg_dest = arc_insn_load_dst_reg(ctx, a->b);
    TCGv reg_address = arc_insn_load_src_reg_tl(ctx, a->c);

    gen_helper_llock(reg_dest, tcg_env, reg_address);

    arc_insn_save_reg(ctx, a->b, reg_dest);

    return true;
}

static bool trans_llock_imm(DisasContext *ctx, arg_llock_imm *a)
{
    TCGv_i32 reg_dest = arc_insn_load_dst_reg(ctx, a->b);
    TCGv reg_address = arc_insn_load_imm_tl(a->imm);

    gen_helper_llock(reg_dest, tcg_env, reg_address);

    arc_insn_save_reg(ctx, a->b, reg_dest);

    return true;
}

static bool trans_scond_reg(DisasContext *ctx, arg_scond_reg *a)
{
    TCGv_i32 reg_src = arc_insn_load_src_reg(ctx, a->b);
    TCGv reg_address = arc_insn_load_src_reg_tl(ctx, a->c);

    gen_helper_scond(tcg_env, reg_address, reg_src);

    return true;
}

static bool trans_scond_imm(DisasContext *ctx, arg_scond_imm *a)
{
    TCGv_i32 reg_src = arc_insn_load_src_reg(ctx, a->b);
    TCGv reg_address = arc_insn_load_imm_tl(a->imm);

    gen_helper_scond(tcg_env, reg_address, reg_src);

    return true;
}

#ifndef TARGET_ARCV3_64
static void arc_gen_llockd(DisasContext *ctx, TCGv dest_lo, TCGv dest_hi, TCGv address)
{
    TCGv_i64 value = tcg_temp_new_i64();

    gen_helper_llockd(value, tcg_env, address);
    tcg_gen_extr_i64_i32(dest_lo, dest_hi, value);
}

static bool trans_llockd_reg(DisasContext *ctx, arg_llockd_reg *a)
{
    TCGv reg_dest_lo = arc_insn_load_dst_reg(ctx, a->b);
    TCGv reg_dest_hi = NULL;
    TCGv reg_address = arc_insn_load_src_reg(ctx, a->c);

    if (a->b == ARC_REGNUM_LIMM) {
        reg_dest_hi = tcg_temp_new();
    } else {
        arc_gen_check_even_regnum(ctx, a->b);
        reg_dest_hi = arc_insn_load_dst_reg(ctx, a->b + 1);
    }

    arc_gen_llockd(ctx, reg_dest_lo, reg_dest_hi, reg_address);

    arc_insn_save_reg(ctx, a->b, reg_dest_lo);
    arc_insn_save_reg(ctx, a->b + 1, reg_dest_hi);

    return true;
}

static bool trans_llockd_imm(DisasContext *ctx, arg_llockd_imm *a)
{
    TCGv reg_dest_lo = arc_insn_load_dst_reg(ctx, a->b);
    TCGv reg_dest_hi = NULL;
    TCGv reg_address = tcg_constant_i32(a->imm);

    if (a->b == ARC_REGNUM_LIMM) {
        reg_dest_hi = tcg_temp_new();
    } else {
        arc_gen_check_even_regnum(ctx, a->b);
        reg_dest_hi = arc_insn_load_dst_reg(ctx, a->b + 1);
    }

    arc_gen_llockd(ctx, reg_dest_lo, reg_dest_hi, reg_address);

    arc_insn_save_reg(ctx, a->b, reg_dest_lo);
    arc_insn_save_reg(ctx, a->b + 1, reg_dest_hi);

    return true;
}

static void arc_gen_scondd(DisasContext *ctx, TCGv src_lo, TCGv src_hi, TCGv address)
{
    TCGv_i64 value = tcg_temp_new_i64();

    tcg_gen_concat_i32_i64(value, src_lo, src_hi);
    gen_helper_scondd(tcg_env, address, value);
}

static bool trans_scondd_reg(DisasContext *ctx, arg_scondd_reg *a)
{
    TCGv reg_src_lo = arc_insn_load_src_reg(ctx, a->b);
    TCGv reg_src_hi = NULL;
    TCGv reg_address = arc_insn_load_src_reg(ctx, a->c);

    if (a->b == ARC_REGNUM_LIMM) {
        reg_src_hi = tcg_temp_new();
        tcg_gen_sari_tl(reg_src_hi, reg_src_lo, 31);
    } else {
        arc_gen_check_even_regnum(ctx, a->b);
        reg_src_hi = arc_insn_load_src_reg(ctx, a->b + 1);
    }

    arc_gen_scondd(ctx, reg_src_lo, reg_src_hi, reg_address);

    return true;
}

static bool trans_scondd_imm(DisasContext *ctx, arg_scondd_imm *a)
{
    TCGv reg_src_lo = arc_insn_load_src_reg(ctx, a->b);
    TCGv reg_src_hi = NULL;
    TCGv reg_address = tcg_constant_i32(a->imm);

    if (a->b == ARC_REGNUM_LIMM) {
        reg_src_hi = tcg_temp_new();
        tcg_gen_sari_tl(reg_src_hi, reg_src_lo, 31);
    } else {
        arc_gen_check_even_regnum(ctx, a->b);
        reg_src_hi = arc_insn_load_src_reg(ctx, a->b + 1);
    }

    arc_gen_scondd(ctx, reg_src_lo, reg_src_hi, reg_address);

    return true;
}

ARC_DEF_TRANS_STUB(llockl)
ARC_DEF_TRANS_STUB(scondl)
#else
static bool trans_llockl(DisasContext *ctx, arg_llockl *a)
{
    TCGv_i64 reg_dest = arc64_insn_load_dst_reg(ctx, a->b);
    TCGv reg_address = arc_insn_load_src_reg_tl(ctx, a->c);

    gen_helper_llockd(reg_dest, tcg_env, reg_address);

    arc64_insn_save_reg(ctx, a->b, reg_dest);

    return true;
}

static bool trans_scondl(DisasContext *ctx, arg_scondl *a)
{
    TCGv_i64 reg_src = arc64_insn_load_src_reg(ctx, a->b);
    TCGv reg_address = arc_insn_load_src_reg_tl(ctx, a->c);

    gen_helper_scondd(tcg_env, reg_address, reg_src);

    return true;
}

ARC_DEF_TRANS_STUB(llockd_reg)
ARC_DEF_TRANS_STUB(llockd_imm)
ARC_DEF_TRANS_STUB(scondd_reg)
ARC_DEF_TRANS_STUB(scondd_imm)
#endif
