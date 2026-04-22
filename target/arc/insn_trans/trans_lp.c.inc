#include "translate.h"

#ifdef TARGET_ARCV2
static void arc_gen_lp(DisasContext *ctx, target_ulong offset)
{
    g_assert(arc_aligned_2_bytes(offset));

    tcg_gen_movi_i32(cpu_lp_start, ctx->base.pc_next);
    tcg_gen_movi_i32(cpu_lp_end, ctx->pcl + offset);
    tcg_gen_movi_i32(cpu_pc, ctx->base.pc_next);

    /*
     * L is cleared when a loop instruction (LPcc) is executed according to PRM.
     * This is not mentioned in instruction's pseudocode. May be critical for
     * loops inside of exceptions and interrupts since STATUS32.L is set to 1
     * in these cases.
     */
     tcg_gen_movi_i32(cpu_Lf, 0);

    ctx->base.is_jmp = DISAS_JUMP_INDIRECT;
}
#endif

static bool trans_lp_u7(DisasContext *ctx, arg_lp_u7 *a)
{
#ifdef TARGET_ARCV2
    arc_gen_check_not_delay_slot(ctx);

    arc_gen_lp(ctx, a->imm);

    return true;
#else
    return false;
#endif
}

static bool trans_lp_u7_cond(DisasContext *ctx, arg_lp_u7_cond *a)
{
#ifdef TARGET_ARCV2
    arc_gen_check_not_delay_slot(ctx);

    TCGLabel *skip = gen_new_label();

    /* If condition is not taken then jump to the end of a loop. */
    tcg_gen_movi_i32(cpu_pc, ctx->pcl + a->imm);

    arc_gen_cc_prologue(ctx, a->cond, skip);
    arc_gen_lp(ctx, a->imm);
    gen_set_label(skip);

    return true;
#else
    return false;
#endif
}

static bool trans_lp_s13(DisasContext *ctx, arg_lp_s13 *a)
{
#ifdef TARGET_ARCV2
    arc_gen_check_not_delay_slot(ctx);

    arc_gen_lp(ctx, a->imm);

    return true;
#else
    return false;
#endif
}
