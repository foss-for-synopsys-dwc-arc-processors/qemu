#include "translate.h"

static void arc_gen_b(DisasContext *ctx, int32_t offset, bool delay_slot)
{
    g_assert(arc_aligned_2_bytes(offset));

    target_long target = ctx->pcl + offset;

    if (delay_slot) {
        arc_gen_set_bta_imm(ctx, target);

        ctx->base.is_jmp = DISAS_UPDATE;
    } else {
        ctx->jump_dest = target;
        ctx->base.is_jmp = DISAS_JUMP_DIRECT;
    }
}

static void arc_gen_b_cond(DisasContext *ctx, int32_t offset, int cond,
                           bool delay_slot)
{
    g_assert(arc_aligned_2_bytes(offset));

    target_long target = ctx->pcl + offset;
    TCGLabel *skip = gen_new_label();

    arc_gen_cc_prologue(ctx, cond, skip);

    if (delay_slot) {
        arc_gen_set_bta_imm(ctx, target);

        ctx->base.is_jmp = DISAS_UPDATE;
    } else {
        arc_gen_goto_tb(ctx, 1, target);

        ctx->base.is_jmp = DISAS_JUMP_COND;
    }

    gen_set_label(skip);
}

static bool trans_b_cond(DisasContext *ctx, arg_b_cond *a)
{
    arc_gen_check_not_delay_slot(ctx);

    arc_gen_b_cond(ctx, a->imm, a->cond, a->delay);

    return true;
}

static bool trans_b_far(DisasContext *ctx, arg_b_far *a)
{
    arc_gen_check_not_delay_slot(ctx);

    arc_gen_b(ctx, a->imm, a->delay);

    return true;
}

static bool trans_b_s(DisasContext *ctx, arg_b_s *a)
{
    arc_gen_check_not_delay_slot(ctx);

    arc_gen_b(ctx, a->imm, false);

    return true;
}

static bool trans_beq_s(DisasContext *ctx, arg_beq_s *a)
{
    arc_gen_check_not_delay_slot(ctx);

    arc_gen_b_cond(ctx, a->imm, ARC_CC_EQ, false);

    return true;
}

static bool trans_bne_s(DisasContext *ctx, arg_bne_s *a)
{
    arc_gen_check_not_delay_slot(ctx);

    arc_gen_b_cond(ctx, a->imm, ARC_CC_NE, false);

    return true;
}

static bool trans_bcc_s(DisasContext *ctx, arg_bcc_s *a)
{
    arc_gen_check_not_delay_slot(ctx);

    arc_gen_b_cond(ctx, a->imm, a->cond, false);

    return true;
}

static void arc_gen_bl(DisasContext *ctx, int32_t offset, bool delay_slot)
{
    g_assert(arc_aligned_4_bytes(offset));

    target_long target = ctx->pcl + offset;
    uint32_t delay_slot_size;

    if (delay_slot) {
        delay_slot_size = arc_get_insn_length_by_address(ctx, ctx->base.pc_next);

        arc_gen_set_bta_imm(ctx, target);
        tcg_gen_movi_tl(cpu_gpr[ARC_REGNUM_BLINK], ctx->base.pc_next + delay_slot_size);

        ctx->base.is_jmp = DISAS_UPDATE;
    } else {
        tcg_gen_movi_tl(cpu_gpr[ARC_REGNUM_BLINK], ctx->base.pc_next);

        ctx->jump_dest = target;
        ctx->base.is_jmp = DISAS_JUMP_DIRECT;
    }
}

static void arc_gen_bl_cond(DisasContext *ctx, int32_t offset, int cond,
                            bool delay_slot)
{
    g_assert(arc_aligned_4_bytes(offset));

    target_long target = ctx->pcl + offset;
    TCGLabel *skip = gen_new_label();
    uint32_t delay_slot_size;

    arc_gen_cc_prologue(ctx, cond, skip);

    if (delay_slot) {
        delay_slot_size = arc_get_insn_length_by_address(ctx, ctx->base.pc_next);

        arc_gen_set_bta_imm(ctx, target);
        tcg_gen_movi_tl(cpu_gpr[ARC_REGNUM_BLINK], ctx->base.pc_next + delay_slot_size);

        ctx->base.is_jmp = DISAS_UPDATE;
    } else {
        tcg_gen_movi_tl(cpu_gpr[ARC_REGNUM_BLINK], ctx->base.pc_next);
        arc_gen_goto_tb(ctx, 1, target);

        ctx->base.is_jmp = DISAS_JUMP_COND;
    }

    gen_set_label(skip);
}

static bool trans_bl_cond(DisasContext *ctx, arg_bl_cond *a)
{
    arc_gen_check_not_delay_slot(ctx);

    arc_gen_bl_cond(ctx, a->imm, a->cond, a->delay);

    return true;
}

static bool trans_bl_far(DisasContext *ctx, arg_bl_far *a)
{
    arc_gen_check_not_delay_slot(ctx);

    arc_gen_bl(ctx, a->imm, a->delay);

    return true;
}

static bool trans_bl_s(DisasContext *ctx, arg_bl_s *a)
{
    arc_gen_check_not_delay_slot(ctx);

    arc_gen_bl(ctx, a->imm, false);

    return true;
}

#ifdef TARGET_ARCV3_64
static bool trans_bl_s_limm(DisasContext *ctx, arg_bl_s_limm *a)
{
    int64_t offset;

    arc_gen_check_not_delay_slot(ctx);

    arc_insn_fetch_limm(ctx);
    offset = ((int64_t)((int32_t)ctx->limm)) << 2;

    tcg_gen_movi_tl(cpu_gpr[ARC_REGNUM_BLINK], ctx->base.pc_next);
    ctx->jump_dest = ctx->pcl + offset;
    ctx->base.is_jmp = DISAS_JUMP_DIRECT;

    return true;
}
#else
ARC_DEF_TRANS_STUB(bl_s_limm)
#endif

static bool trans_bi(DisasContext *ctx, arg_bi *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv target = tcg_temp_new();
    TCGv c = arc_insn_load_src_reg_tl(ctx, a->c);

#ifdef TARGET_ARCV3_64
    tcg_gen_ext32u_i64(c, c);
#endif

    tcg_gen_shli_tl(target, c, 2);
    tcg_gen_addi_tl(cpu_pc, target, ctx->base.pc_next);

    ctx->base.is_jmp = DISAS_JUMP_INDIRECT;

    return true;
}

static bool trans_bih(DisasContext *ctx, arg_bih *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv target = tcg_temp_new();
    TCGv c = arc_insn_load_src_reg_tl(ctx, a->c);

#ifdef TARGET_ARCV3_64
    tcg_gen_ext32u_i64(c, c);
#endif

    tcg_gen_shli_tl(target, c, 1);
    tcg_gen_addi_tl(cpu_pc, target, ctx->base.pc_next);

    ctx->base.is_jmp = DISAS_JUMP_INDIRECT;

    return true;
}


static void arc64_gen_bbitl(DisasContext *ctx, TCGv_i64 b, TCGv_i64 c, target_ulong target, bool delay_slot, bool check_bit_set)
{
    TCGv_i64 bit_number = tcg_temp_new_i64();
    TCGv_i64 cond = tcg_temp_new_i64();
    TCGLabel *skip = gen_new_label();

    tcg_gen_andi_i64(bit_number, c, 0x3F);
    tcg_gen_shr_i64(cond, b, bit_number);
    tcg_gen_andi_i64(cond, cond, 1);

    if (check_bit_set) {
        tcg_gen_brcondi_i64(TCG_COND_EQ, cond, 0, skip);
    } else {
        tcg_gen_brcondi_i64(TCG_COND_EQ, cond, 1, skip);
    }

    if (delay_slot) {
        arc_gen_set_bta_imm(ctx, target);

        ctx->base.is_jmp = DISAS_UPDATE;
    } else {
        arc_gen_goto_tb(ctx, 1, target);

        ctx->base.is_jmp = DISAS_JUMP_COND;
    }

    gen_set_label(skip);
}

static void arc_gen_bbit(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, target_ulong target, bool delay_slot, bool check_bit_set)
{
    TCGv_i64 b_i64 = tcg_temp_new_i64();
    TCGv_i64 c_i64 = tcg_temp_new_i64();

    tcg_gen_extu_i32_i64(b_i64, b);
    tcg_gen_extu_i32_i64(c_i64, c);
    tcg_gen_andi_i64(c_i64, c_i64, 0x1F);

    arc64_gen_bbitl(ctx, b_i64, c_i64, target, delay_slot, check_bit_set);
}

/* BBIT0<.d><.T> b,c,s9 */
static bool trans_bbit0_reg(DisasContext *ctx, arg_bbit0_reg *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c);
    target_ulong target = ctx->pcl + a->offset;

    if (ctx->has_limm && a->delay) {
        arc_gen_exception_illegal_instruction(ctx);
    }

    arc_gen_bbit(ctx, reg_b, reg_c, target, a->delay, false);

    return true;
}

/* BBIT0<.d><.T> b,u6,s9 */
static bool trans_bbit0_imm(DisasContext *ctx, arg_bbit0_imm *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_imm(a->imm);
    target_ulong target = ctx->pcl + a->offset;

    if (ctx->has_limm && a->delay) {
        arc_gen_exception_illegal_instruction(ctx);
    }

    arc_gen_bbit(ctx, reg_b, reg_c, target, a->delay, false);

    return true;
}

/* BBIT1<.d><.T> b,c,s9 */
static bool trans_bbit1_reg(DisasContext *ctx, arg_bbit1_reg *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c);
    target_ulong target = ctx->pcl + a->offset;

    if (ctx->has_limm && a->delay) {
        arc_gen_exception_illegal_instruction(ctx);
    }

    arc_gen_bbit(ctx, reg_b, reg_c, target, a->delay, true);

    return true;
}

/* BBIT1<.d><.T> b,u6,s9 */
static bool trans_bbit1_imm(DisasContext *ctx, arg_bbit1_imm *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_imm(a->imm);
    target_ulong target = ctx->pcl + a->offset;

    if (ctx->has_limm && a->delay) {
        arc_gen_exception_illegal_instruction(ctx);
    }

    arc_gen_bbit(ctx, reg_b, reg_c, target, a->delay, true);

    return true;
}

#ifdef TARGET_ARCV3_64
/* BBIT0L<.d> b,c,s9 */
static bool trans_bbit0l_reg(DisasContext *ctx, arg_bbit0l_reg *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv_i64 reg_b = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_src_reg(ctx, a->c);
    target_ulong target = ctx->pcl + a->offset;

    if (ctx->has_limm && a->delay) {
        arc_gen_exception_illegal_instruction(ctx);
    }

    arc64_gen_bbitl(ctx, reg_b, reg_c, target, a->delay, false);

    return true;
}

/* BBIT0L<.d> b,u6,s9 */
static bool trans_bbit0l_imm(DisasContext *ctx, arg_bbit0l_imm *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv_i64 reg_b = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_imm(a->imm);
    target_ulong target = ctx->pcl + a->offset;

    if (ctx->has_limm && a->delay) {
        arc_gen_exception_illegal_instruction(ctx);
    }
    
    arc64_gen_bbitl(ctx, reg_b, reg_c, target, a->delay, false);

    return true;
}

/* BBIT1L<.d> b,c,s9 */
static bool trans_bbit1l_reg(DisasContext *ctx, arg_bbit1l_reg *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv_i64 reg_b = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_src_reg(ctx, a->c);
    target_ulong target = ctx->pcl + a->offset;

    if (ctx->has_limm && a->delay) {
        arc_gen_exception_illegal_instruction(ctx);
    }

    arc64_gen_bbitl(ctx, reg_b, reg_c, target, a->delay, true);

    return true;
}

/* BBIT1L<.d> b,u6,s9 */
static bool trans_bbit1l_imm(DisasContext *ctx, arg_bbit1l_imm *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv_i64 reg_b = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_imm(a->imm);
    target_ulong target = ctx->pcl + a->offset;

    if (ctx->has_limm && a->delay) {
        arc_gen_exception_illegal_instruction(ctx);
    }

    arc64_gen_bbitl(ctx, reg_b, reg_c, target, a->delay, true);

    return true;
}
#else
ARC_DEF_TRANS_STUB(bbit0l_reg)
ARC_DEF_TRANS_STUB(bbit0l_imm)
ARC_DEF_TRANS_STUB(bbit1l_reg)
ARC_DEF_TRANS_STUB(bbit1l_imm)
#endif

/*
 * if (cc(b,c)) PC = PCL + s9
 *
 * BREQ     Branch if Equal
 *          if (b==c) PC = (PCL +s9)
 * BRNE     Branch if Not Equal
 *          if (b!=c) PC = (PCL + s9)
 * BRLT     Branch if Less Than (Signed)
 *          if (b<c) PC = (PCL + s9)
 * BRGE     Branch if Greater Than or Equal (Signed)
 *          if (b>=c) PC = (PCL + s9)
 * BRLO     Branch if Lower Than (Unsigned)
 *          if (b<c) PC = (PCL + s9)
 * BRHS     Branch if Higher Than or Same (Unsigned)
 *          if (b>=c) PC = (PCL + s9)
 */
static void arc64_gen_brccl(DisasContext *ctx, TCGv_i64 b, TCGv_i64 c, int32_t offset, int cond, bool delay)
{
    g_assert(arc_aligned_2_bytes(offset));

    TCGLabel *skip = gen_new_label();
    target_long target = ctx->pcl + offset;

    switch (cond) {
    case ARC_CC_EQ:
        tcg_gen_brcond_i64(TCG_COND_NE, b, c, skip);
        break;
    case ARC_CC_NE:
        tcg_gen_brcond_i64(TCG_COND_EQ, b, c, skip);
        break;
    case ARC_CC_LT:
        tcg_gen_brcond_i64(TCG_COND_GE, b, c, skip);
        break;
    case ARC_CC_GE:
        tcg_gen_brcond_i64(TCG_COND_LT, b, c, skip);
        break;
    case ARC_CC_LO:
        tcg_gen_brcond_i64(TCG_COND_GEU, b, c, skip);
        break;
    case ARC_CC_HS:
        tcg_gen_brcond_i64(TCG_COND_LTU, b, c, skip);
        break;
    default:
        g_assert_not_reached();
    }

    if (delay) {
        arc_gen_set_bta_imm(ctx, target);

        ctx->base.is_jmp = DISAS_UPDATE;
    } else {
        arc_gen_goto_tb(ctx, 1, target);

        ctx->base.is_jmp = DISAS_JUMP_COND;
    }

    gen_set_label(skip);
}

static void arc_gen_brcc(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, int32_t offset, int cond, bool delay)
{
    TCGv_i64 b_i64 = tcg_temp_new_i64();
    TCGv_i64 c_i64 = tcg_temp_new_i64();

    tcg_gen_ext_i32_i64(b_i64, b);
    tcg_gen_ext_i32_i64(c_i64, c);

    arc64_gen_brccl(ctx, b_i64, c_i64, offset, cond, delay);
}

/* BRCC<.d><.T> b,c,s9 */
static bool trans_brcc_reg(DisasContext *ctx, arg_brcc_reg *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_src_reg(ctx, a->c);

    arc_gen_brcc(ctx, reg_b, reg_c, a->offset, a->cond, a->delay);

    return true;
}

/* BRCC<.d><.T> b,u6,s9 */
static bool trans_brcc_u6(DisasContext *ctx, arg_brcc_u6 *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv_i32 reg_b = arc_insn_load_src_reg(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_imm(a->imm);

    arc_gen_brcc(ctx, reg_b, reg_c, a->offset, a->cond, a->delay);

    return true;
}

#ifdef TARGET_ARCV3_64
static bool trans_brccl_reg(DisasContext *ctx, arg_brccl_reg *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv_i64 reg_b = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_src_reg(ctx, a->c);

    arc64_gen_brccl(ctx, reg_b, reg_c, a->offset, a->cond, a->delay);

    return true;
}

static bool trans_brccl_u6(DisasContext *ctx, arg_brccl_u6 *a)
{
    arc_gen_check_not_delay_slot(ctx);

    TCGv_i64 reg_b = arc64_insn_load_src_reg(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_imm(a->imm);

    arc64_gen_brccl(ctx, reg_b, reg_c, a->offset, a->cond, a->delay);

    return true;
}
#else
ARC_DEF_TRANS_STUB(brccl_reg)
ARC_DEF_TRANS_STUB(brccl_u6)
#endif

/* BRCC_S b,0,s8 (BRCCL_S for HS6x) */
static bool trans_brcc_s(DisasContext *ctx, arg_brcc_s *a)
{
    arc_gen_check_not_delay_slot(ctx);

#ifdef TARGET_ARCV3_64
    TCGv_i64 reg_b = arc64_insn_load_src_reg_s(ctx, a->b);
    TCGv_i64 reg_c = arc64_insn_load_imm(0);

    arc64_gen_brccl(ctx, reg_b, reg_c, a->offset, a->cond, false);
#else
    TCGv_i32 reg_b = arc_insn_load_src_reg_s(ctx, a->b);
    TCGv_i32 reg_c = arc_insn_load_imm(0);

    arc_gen_brcc(ctx, reg_b, reg_c, a->offset, a->cond, false);
#endif

    return true;
}

static void arc_gen_dbnz(DisasContext *ctx, TCGv b, int regnum,
                         int32_t offset, bool delay)
{
    g_assert(arc_aligned_2_bytes(offset));

    target_long target = ctx->pcl + offset;
    TCGLabel *skip = gen_new_label();

    tcg_gen_subi_tl(b, b, 1);
    arc_insn_save_reg_tl(ctx, regnum, b);
    tcg_gen_brcondi_tl(TCG_COND_EQ, b, 0, skip);

    if (delay) {
        arc_gen_set_bta_imm(ctx, target);

        ctx->base.is_jmp = DISAS_UPDATE;
    } else {
        arc_gen_goto_tb(ctx, 1, target);

        ctx->base.is_jmp = DISAS_JUMP_COND;
    }

    gen_set_label(skip);
}

static bool trans_dbnz(DisasContext *ctx, arg_dbnz *a)
{
    arc_gen_check_not_delay_slot(ctx);

    if (arc_regnum_limm(a->b) || a->b == ARC_REGNUM_PCL) {
        arc_gen_exception_illegal_instruction(ctx);
    }

    TCGv reg_b = arc_insn_load_dst_reg_tl(ctx, a->b);

    arc_gen_dbnz(ctx, reg_b, a->b, a->imm, a->delay);

    return true;
}
