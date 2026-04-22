#include "translate.h"

static inline void arc_gen_add(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_add_i32(a, b, c);

    if (set_flags) {
        arc_gen_set_flag_ZNCV_add(a, b, c);
    }
}

ARC_DEF_TRANS_DOP_REG_REG(add)
ARC_DEF_TRANS_DOP_REG_U6(add)
ARC_DEF_TRANS_DOP_REG_S12(add)
ARC_DEF_TRANS_DOP_COND_REG(add)
ARC_DEF_TRANS_DOP_COND_U6(add)

ARC_DEF_TRANS_RS32_RS32_RS32_F(add, add_s_a_b_c, a->a, a->b, a->c, false);
ARC_DEF_TRANS_RS32_RS32_RH32_F(add, add_s_b_b_h, a->b, a->b, a->h, false);
ARC_DEF_TRANS_RH32_RH32_I32_F(add, add_s_h_h_s3, a->h, a->h, a->imm, false);
ARC_DEF_TRANS_RS32_R32_I32_F(add, add_s_b_sp_u7, a->b, ARC_REGNUM_SP, a->imm, false);
ARC_DEF_TRANS_R32_R32_I32_F(add, add_s_sp_sp_u7, ARC_REGNUM_SP, ARC_REGNUM_SP, a->imm, false);
ARC_DEF_TRANS_R32_R32_I32_F(add, add_s_r0_gp_s11, ARC_REGNUM_R0, ARC_REGNUM_GP, a->imm, false);
ARC_DEF_TRANS_RS32_RS32_I32_F(add, add_s_r01_b_u6, a->a, a->b, a->imm, false);

#ifdef TARGET_ARCV2
ARC_DEF_TRANS_RS32_RS32_I32_F(add, add_s_c_b_u3, a->c, a->b, a->imm, false);
ARC_DEF_TRANS_RS32_RS32_I32_F(add, add_s_b_b_u7, a->b, a->b, a->imm, false);
#else
ARC_DEF_TRANS_STUB(add_s_c_b_u3)
ARC_DEF_TRANS_STUB(add_s_b_b_u7)
#endif

static inline void arc64_gen_addl(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    tcg_gen_add_i64(a, b, c);

    if (set_flags) {
        arc64_gen_set_flag_ZNCV_add(a, b, c);
    }
}

ARC64_DEF_TRANS_DOP_REG_REG(addl)
ARC64_DEF_TRANS_DOP_REG_U6(addl)
ARC64_DEF_TRANS_DOP_REG_S12(addl)
ARC64_DEF_TRANS_DOP_COND_REG(addl)
ARC64_DEF_TRANS_DOP_COND_U6(addl)

ARC64_DEF_TRANS_RS64_RS64_RS64_F(addl, addl_s_b_b_c, a->b, a->b, a->c, false);
ARC64_DEF_TRANS_RS64_R64_I64_F(addl, addl_s_b_sp_u7, a->b, ARC_REGNUM_SP, a->imm, false);
ARC64_DEF_TRANS_R64_R64_I64_F(addl, addl_s_sp_sp_u9, ARC_REGNUM_SP,ARC_REGNUM_SP, a->imm, false)
ARC64_DEF_TRANS_R64_R64_I64_F(addl, addl_s_r0_gp_s11, ARC_REGNUM_R0,ARC_REGNUM_GP, a->imm, false)

#ifdef TARGET_ARCV3_64
static bool trans_addl_s_h_h_limm(DisasContext *ctx, arg_addl_s_h_h_limm *a)
{
    TCGv_i64 reg_r0 = arc64_insn_load_dst_reg_h(ctx, a->h);
    TCGv_i64 reg_r1 = arc64_insn_load_src_reg_h(ctx, a->h);
    TCGv_i64 reg_r2 = arc64_insn_load_limm_signed(ctx);
    arc64_gen_addl(ctx, reg_r0, reg_r1, reg_r2, false);
    arc64_insn_save_reg_h(ctx, a->h, reg_r0);
    return true;
}
#else
ARC_DEF_TRANS_STUB(addl_s_h_h_limm)
#endif

#ifdef TARGET_ARCV3_64
static bool trans_addl_s_h_pcl_limm(DisasContext *ctx, arg_addl_s_h_pcl_limm *a)
{
    TCGv_i64 reg_r0 = arc64_insn_load_dst_reg_h(ctx, a->h);
    TCGv_i64 reg_r1 = arc64_insn_load_pcl(ctx);
    TCGv_i64 reg_r2 = arc64_insn_load_limm_signed(ctx);
    arc64_gen_addl(ctx, reg_r0, reg_r1, reg_r2, false);
    arc64_insn_save_reg_h(ctx, a->h, reg_r0);
    return true;
}
#else
ARC_DEF_TRANS_STUB(addl_s_h_pcl_limm)
#endif

static inline void arc64_gen_addhl(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    tcg_gen_shli_i64(c, c, 32);
    tcg_gen_add_i64(a, b, c);
}

ARC64_DEF_TRANS_DOP_REG_REG(addhl)
ARC64_DEF_TRANS_DOP_REG_U6(addhl)
ARC64_DEF_TRANS_DOP_REG_S12(addhl)
ARC64_DEF_TRANS_DOP_COND_REG(addhl)
ARC64_DEF_TRANS_DOP_COND_U6(addhl)

#ifdef TARGET_ARCV3_64
static bool trans_addhl_s_h_h_limm(DisasContext *ctx, arg_addhl_s_h_h_limm *a)
{
    TCGv_i64 reg_r0 = arc64_insn_load_dst_reg_h(ctx, a->h);
    TCGv_i64 reg_r1 = arc64_insn_load_src_reg_h(ctx, a->h);
    TCGv_i64 reg_r2 = arc64_insn_load_limm_signed(ctx);
    arc64_gen_addhl(ctx, reg_r0, reg_r1, reg_r2, false);
    arc64_insn_save_reg_h(ctx, a->h, reg_r0);
    return true;
}
#else
ARC_DEF_TRANS_STUB(addhl_s_h_h_limm)
#endif

#ifdef TARGET_ARCV3_64
static bool trans_addhl_s_h_pcl_limm(DisasContext *ctx, arg_addl_s_h_pcl_limm *a)
{
    TCGv_i64 reg_r0 = arc64_insn_load_dst_reg_h(ctx, a->h);
    TCGv_i64 reg_r1 = arc64_insn_load_pcl(ctx);
    TCGv_i64 reg_r2 = arc64_insn_load_limm_signed(ctx);
    arc64_gen_addhl(ctx, reg_r0, reg_r1, reg_r2, false);
    arc64_insn_save_reg_h(ctx, a->h, reg_r0);
    return true;
}
#else
ARC_DEF_TRANS_STUB(addhl_s_h_pcl_limm)
#endif

static inline void arc_gen_add_shifted(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags, int shift)
{
    tcg_gen_shli_i32(c, c, shift);
    tcg_gen_add_i32(a, b, c);

    if (set_flags) {
        arc_gen_set_flag_ZNCV_add(a, b, c);
    }
}

static inline void arc64_gen_addl_shifted(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags, int shift)
{
    tcg_gen_shli_i64(c, c, shift);
    tcg_gen_add_i64(a, b, c);

    if (set_flags) {
        arc64_gen_set_flag_ZNCV_add(a, b, c);
    }
}

static inline void arc_gen_add1(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    arc_gen_add_shifted(ctx, a, b, c, set_flags, 1);
}

ARC_DEF_TRANS_DOP_REG_REG(add1)
ARC_DEF_TRANS_DOP_REG_U6(add1)
ARC_DEF_TRANS_DOP_REG_S12(add1)
ARC_DEF_TRANS_DOP_COND_REG(add1)
ARC_DEF_TRANS_DOP_COND_U6(add1)
ARC_DEF_TRANS_S_DOP_3_OPERANDS(add1)

static inline void arc64_gen_add1l(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    arc64_gen_addl_shifted(ctx, a, b, c, set_flags, 1);
}

ARC64_DEF_TRANS_DOP_REG_REG(add1l)
ARC64_DEF_TRANS_DOP_REG_U6(add1l)
ARC64_DEF_TRANS_DOP_REG_S12(add1l)
ARC64_DEF_TRANS_DOP_COND_REG(add1l)
ARC64_DEF_TRANS_DOP_COND_U6(add1l)

static inline void arc_gen_add2(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    arc_gen_add_shifted(ctx, a, b, c, set_flags, 2);
}

ARC_DEF_TRANS_DOP_REG_REG(add2)
ARC_DEF_TRANS_DOP_REG_U6(add2)
ARC_DEF_TRANS_DOP_REG_S12(add2)
ARC_DEF_TRANS_DOP_COND_REG(add2)
ARC_DEF_TRANS_DOP_COND_U6(add2)
ARC_DEF_TRANS_S_DOP_3_OPERANDS(add2)

static inline void arc64_gen_add2l(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    arc64_gen_addl_shifted(ctx, a, b, c, set_flags, 2);
}

ARC64_DEF_TRANS_DOP_REG_REG(add2l)
ARC64_DEF_TRANS_DOP_REG_U6(add2l)
ARC64_DEF_TRANS_DOP_REG_S12(add2l)
ARC64_DEF_TRANS_DOP_COND_REG(add2l)
ARC64_DEF_TRANS_DOP_COND_U6(add2l)

static inline void arc_gen_add3(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    arc_gen_add_shifted(ctx, a, b, c, set_flags, 3);
}

ARC_DEF_TRANS_DOP_REG_REG(add3)
ARC_DEF_TRANS_DOP_REG_U6(add3)
ARC_DEF_TRANS_DOP_REG_S12(add3)
ARC_DEF_TRANS_DOP_COND_REG(add3)
ARC_DEF_TRANS_DOP_COND_U6(add3)
ARC_DEF_TRANS_S_DOP_3_OPERANDS(add3)

static inline void arc64_gen_add3l(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    arc64_gen_addl_shifted(ctx, a, b, c, set_flags, 3);
}

ARC64_DEF_TRANS_DOP_REG_REG(add3l)
ARC64_DEF_TRANS_DOP_REG_U6(add3l)
ARC64_DEF_TRANS_DOP_REG_S12(add3l)
ARC64_DEF_TRANS_DOP_COND_REG(add3l)
ARC64_DEF_TRANS_DOP_COND_U6(add3l)

static inline void arc_gen_adc(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_add_i32(a, b, c);
    tcg_gen_add_i32(a, a, cpu_Cf);

    if (set_flags) {
        arc_gen_set_flag_ZNCV_add(a, b, c);
    }
}

ARC_DEF_TRANS_DOP_REG_REG(adc)
ARC_DEF_TRANS_DOP_REG_U6(adc)
ARC_DEF_TRANS_DOP_REG_S12(adc)
ARC_DEF_TRANS_DOP_COND_REG(adc)
ARC_DEF_TRANS_DOP_COND_U6(adc)

static inline void arc64_gen_adcl(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 tmp = tcg_temp_new_i64();

    tcg_gen_add_i64(a, b, c);
    tcg_gen_extu_i32_i64(tmp, cpu_Cf);
    tcg_gen_add_i64(a, a, tmp);

    if (set_flags) {
        arc64_gen_set_flag_ZNCV_add(a, b, c);
    }
}

ARC64_DEF_TRANS_DOP_REG_REG(adcl)
ARC64_DEF_TRANS_DOP_REG_U6(adcl)
ARC64_DEF_TRANS_DOP_REG_S12(adcl)
ARC64_DEF_TRANS_DOP_COND_REG(adcl)
ARC64_DEF_TRANS_DOP_COND_U6(adcl)
