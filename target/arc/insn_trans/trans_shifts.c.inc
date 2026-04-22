#include "translate.h"

static inline void arc_gen_asl(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, bool set_flags)
{
    TCGv_i32 msb_a = tcg_temp_new_i32();
    TCGv_i32 msb_b = tcg_temp_new_i32();

    tcg_gen_shli_i32(a, b, 1);

    if (set_flags) {
        arc_gen_set_flag_Z(a);
        arc_gen_set_flag_N(a);
    
        tcg_gen_shri_i32(msb_a, a, TARGET_LONG_BITS_32 - 1);
        tcg_gen_shri_i32(msb_b, b, TARGET_LONG_BITS_32 - 1);
        tcg_gen_mov_i32(cpu_Cf, msb_b);
        tcg_gen_setcond_i32(TCG_COND_NE, cpu_Vf, msb_a, msb_b);
    }
}

ARC_DEF_TRANS_SOP_REG_REG(asl)
ARC_DEF_TRANS_SOP_REG_U6(asl)
ARC_DEF_TRANS_S_DOP_2_OPERANDS(asl)

static inline void arc64_gen_asll(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, bool set_flags)
{
    TCGv_i64 msb_a = tcg_temp_new_i64();
    TCGv_i64 msb_b = tcg_temp_new_i64();
    TCGv_i64 temp = tcg_temp_new_i64();

    tcg_gen_shli_i64(a, b, 1);

    if (set_flags) {
        arc64_gen_set_flag_Z(a);
        arc64_gen_set_flag_N(a);
    
        tcg_gen_shri_i64(msb_a, a, TARGET_LONG_BITS_64 - 1);
        tcg_gen_shri_i64(msb_b, b, TARGET_LONG_BITS_64 - 1);

        tcg_gen_extrl_i64_i32(cpu_Cf, msb_b);
        tcg_gen_setcond_i64(TCG_COND_NE, temp, msb_a, msb_b);
        tcg_gen_extrl_i64_i32(cpu_Vf, temp);
    }
}

ARC64_DEF_TRANS_SOP_REG_REG(asll)
ARC64_DEF_TRANS_SOP_REG_U6(asll)

/*
 * ASL (multiple)
 *
 * if (cc) a = (signed) b << c;
 *
 * if cc==true then
 *   dest = src1 << (src2 & 31)
 *   if F==1 then
 *     Z_flag = if dest==0 then 1 else 0
 *     N_flag = dest[31]
 *     C_flag = if src2==0 then 0 else src1[32 - src2]
 */
static inline void arc_gen_aslm(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGLabel *skip = gen_new_label();

    tcg_gen_andi_i32(c, c, 0x1F); /* src2 & 31 */
    tcg_gen_shl_i32(a, b, c); /* src1 << (src2 & 31) */

    if (set_flags) {
        arc_gen_set_flag_Z(a);
        arc_gen_set_flag_N(a);

        /* C_flag = if src2==0 then 0 else src1[32 - src2] */
        tcg_gen_movi_i32(cpu_Cf, 0);
        tcg_gen_brcondi_i32(TCG_COND_EQ, c, 0, skip);
        tcg_gen_subfi_i32(c, TARGET_LONG_BITS_32, c); /* src2 - 32 */
        tcg_gen_shr_i32(cpu_Cf, b, c); /* src1[32 - src2] */
        tcg_gen_andi_i32(cpu_Cf, cpu_Cf, 1);

        gen_set_label(skip);
    }
}

ARC_DEF_TRANS_DOP_REG_REG(aslm)
ARC_DEF_TRANS_DOP_REG_U6(aslm)
ARC_DEF_TRANS_DOP_REG_S12(aslm)
ARC_DEF_TRANS_DOP_COND_REG(aslm)
ARC_DEF_TRANS_DOP_COND_U6(aslm)
ARC_DEF_TRANS_RS32_RS32_RS32_F(aslm, aslm_s_b_b_c, a->b, a->b, a->c, false);
ARC_DEF_TRANS_RS32_RS32_I32_F(aslm, aslm_s_b_b_u5, a->b, a->b, a->imm, false);

#ifdef TARGET_ARCV2
ARC_DEF_TRANS_RS32_RS32_I32_F(aslm, aslm_s_c_b_u3, a->c, a->b, a->imm, false);
#else
ARC_DEF_TRANS_STUB(aslm_s_c_b_u3)
#endif

static inline void arc64_gen_aslml(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 temp = tcg_temp_new_i64();
    TCGLabel *skip = gen_new_label();

    tcg_gen_andi_i64(c, c, 0x3F); /* src2 & 63 */
    tcg_gen_shl_i64(a, b, c); /* src1 << (src2 & 63) */

    if (set_flags) {
        arc64_gen_set_flag_Z(a);
        arc64_gen_set_flag_N(a);

        /* C_flag = if src2==0 then 0 else src1[64 - src2] */
        tcg_gen_movi_i32(cpu_Cf, 0);
        tcg_gen_brcondi_i64(TCG_COND_EQ, c, 0, skip);
        tcg_gen_subfi_i64(c, TARGET_LONG_BITS_64, c); /* src2 - 64 */
        tcg_gen_shr_i64(temp, b, c); /* src1[64 - src2] */
        tcg_gen_extrl_i64_i32(cpu_Cf, temp);
        tcg_gen_andi_i32(cpu_Cf, cpu_Cf, 1);

        gen_set_label(skip);
    }
}

ARC64_DEF_TRANS_DOP_REG_REG(aslml)
ARC64_DEF_TRANS_DOP_REG_U6(aslml)
ARC64_DEF_TRANS_DOP_REG_S12(aslml)
ARC64_DEF_TRANS_DOP_COND_REG(aslml)
ARC64_DEF_TRANS_DOP_COND_U6(aslml)

static inline void arc_gen_asr(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_sari_i32(b, c, 1);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        arc_gen_set_flag_N(b);
        tcg_gen_andi_i32(cpu_Cf, c, 1);
    }
}

ARC_DEF_TRANS_SOP_REG_REG(asr)
ARC_DEF_TRANS_SOP_REG_U6(asr)
ARC_DEF_TRANS_S_DOP_2_OPERANDS(asr)

static inline void arc64_gen_asrl(DisasContext *ctx, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 tmp = tcg_temp_new_i64();

    tcg_gen_sari_i64(b, c, 1);

    if (set_flags) {
        arc64_gen_set_flag_Z(b);
        arc64_gen_set_flag_N(b);
        tcg_gen_andi_i64(tmp, c, 1);
        tcg_gen_extrl_i64_i32(cpu_Cf, tmp);
    }
}

ARC64_DEF_TRANS_SOP_REG_REG(asrl)
ARC64_DEF_TRANS_SOP_REG_U6(asrl)

static inline void arc_gen_asr16(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_sari_i32(b, c, 16);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        arc_gen_set_flag_N(b);
    }
}

ARC_DEF_TRANS_SOP_REG_REG(asr16)
ARC_DEF_TRANS_SOP_REG_U6(asr16)

static inline void arc_gen_asr8(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_sari_i32(b, c, 8);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        arc_gen_set_flag_N(b);
    }
}

ARC_DEF_TRANS_SOP_REG_REG(asr8)
ARC_DEF_TRANS_SOP_REG_U6(asr8)

static inline void arc_gen_asrm(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGLabel *skip = gen_new_label();

    tcg_gen_andi_i32(c, c, 0x1F); /* src2 & 31 */
    tcg_gen_sar_i32(a, b, c); /* ((signed) src1) >> (src2 & 31) */

    if (set_flags) {
        arc_gen_set_flag_Z(a);
        arc_gen_set_flag_N(a);

        /* C_flag = if src2==0 then 0 else src1[src2 - 1] */
        tcg_gen_movi_i32(cpu_Cf, 0);
        tcg_gen_brcondi_i32(TCG_COND_EQ, c, 0, skip);
        tcg_gen_subi_i32(c, c, 1); /* src2 - 1 */
        tcg_gen_shr_i32(cpu_Cf, b, c); /* src1[src2 - 1] */
        tcg_gen_andi_i32(cpu_Cf, cpu_Cf, 1);

        gen_set_label(skip);
    }
}

ARC_DEF_TRANS_DOP_REG_REG(asrm)
ARC_DEF_TRANS_DOP_REG_U6(asrm)
ARC_DEF_TRANS_DOP_REG_S12(asrm)
ARC_DEF_TRANS_DOP_COND_REG(asrm)
ARC_DEF_TRANS_DOP_COND_U6(asrm)
ARC_DEF_TRANS_RS32_RS32_RS32_F(asrm, asrm_s_b_b_c, a->b, a->b, a->c, false);
ARC_DEF_TRANS_RS32_RS32_I32_F(asrm, asrm_s_b_b_u5, a->b, a->b, a->imm, false);

#ifdef TARGET_ARCV2
ARC_DEF_TRANS_RS32_RS32_I32_F(asrm, asrm_s_c_b_u3, a->c, a->b, a->imm, false);
#else
ARC_DEF_TRANS_STUB(asrm_s_c_b_u3)
#endif

static inline void arc64_gen_asrml(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 tmp = tcg_temp_new_i64();
    TCGLabel *skip = gen_new_label();

    tcg_gen_andi_i64(c, c, 0x3F); /* src2 & 63 */
    tcg_gen_sar_i64(a, b, c); /* ((signed) src1) >> (src2 & 63) */

    if (set_flags) {
        arc64_gen_set_flag_Z(a);
        arc64_gen_set_flag_N(a);

        /* C_flag = if src2==0 then 0 else src1[src2 - 1] */
        tcg_gen_movi_i32(cpu_Cf, 0);
        tcg_gen_brcondi_i64(TCG_COND_EQ, c, 0, skip);
        tcg_gen_subi_i64(c, c, 1); /* src2 - 1 */
        tcg_gen_shr_i64(tmp, b, c); /* src1[src2 - 1] */
        tcg_gen_extrl_i64_i32(cpu_Cf, tmp);
        tcg_gen_andi_i32(cpu_Cf, cpu_Cf, 1);

        gen_set_label(skip);
    }
}

ARC64_DEF_TRANS_DOP_REG_REG(asrml)
ARC64_DEF_TRANS_DOP_REG_U6(asrml)
ARC64_DEF_TRANS_DOP_REG_S12(asrml)
ARC64_DEF_TRANS_DOP_COND_REG(asrml)
ARC64_DEF_TRANS_DOP_COND_U6(asrml)

static inline void arc_gen_lsl16(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_shli_i32(b, c, 16);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        arc_gen_set_flag_N(b);
    }
}

ARC_DEF_TRANS_SOP_REG_REG(lsl16)
ARC_DEF_TRANS_SOP_REG_U6(lsl16)

static inline void arc_gen_lsl8(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_shli_i32(b, c, 8);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        arc_gen_set_flag_N(b);
    }
}

ARC_DEF_TRANS_SOP_REG_REG(lsl8)
ARC_DEF_TRANS_SOP_REG_U6(lsl8)

/*
 * b = (unsigned) c >> 1
 *
 * dest = src >> 1
 *   dest[31] = 0
 *   if F==1 then
 *     Z_flag = if dest==0 then 1 else 0
 *     N_flag = dest[31]
 *     C_flag = src[0]
 */
static inline void arc_gen_lsr(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_shri_i32(b, c, 1);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        arc_gen_set_flag_N(b);
        tcg_gen_andi_i32(cpu_Cf, c, 1);
    }
}

ARC_DEF_TRANS_SOP_REG_REG(lsr)
ARC_DEF_TRANS_SOP_REG_U6(lsr)
ARC_DEF_TRANS_S_DOP_2_OPERANDS(lsr)

static inline void arc64_gen_lsrl(DisasContext *ctx, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i32 tmp = tcg_temp_new_i32();

    tcg_gen_shri_i64(b, c, 1);

    if (set_flags) {
        arc64_gen_set_flag_Z(b);
        arc64_gen_set_flag_N(b);
        tcg_gen_extrl_i64_i32(tmp, c);
        tcg_gen_andi_i32(cpu_Cf, tmp, 1);
    }
}

ARC64_DEF_TRANS_SOP_REG_REG(lsrl)
ARC64_DEF_TRANS_SOP_REG_U6(lsrl)

/*
 * if (cc) a = (unsigned) b >> c;
 *
 * if cc==true then
 *   dest = src1 >> (src2 & 31)
 *   if F==1 then
 *     Z_flag = if dest==0 then 1 else 0
 *     N_flag = dest[31]
 *     C_flag = if src2==0 then 0 else src1[sr2 - 1]
 */
static inline void arc_gen_lsrm(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_andi_i32(c, c, 0x1F); /* src2 & 31 */
    tcg_gen_shr_i32(a, b, c); /* ((unsigned) src1) >> (src2 & 31) */

    if (set_flags) {
        arc_gen_set_flag_Z(a);
        arc_gen_set_flag_N(a);

        /* C_flag = if src2==0 then 0 else src1[src2 - 1] */
        TCGLabel *skip = gen_new_label();

        tcg_gen_movi_i32(cpu_Cf, 0);
        tcg_gen_brcondi_i32(TCG_COND_EQ, c, 0, skip);
        tcg_gen_subi_i32(c, c, 1); /* src2 - 1 */
        tcg_gen_shr_i32(cpu_Cf, b, c); /* src1[src2 - 1] */
        tcg_gen_andi_i32(cpu_Cf, cpu_Cf, 1);

        gen_set_label(skip);
    }
}

ARC_DEF_TRANS_DOP_REG_REG(lsrm)
ARC_DEF_TRANS_DOP_REG_U6(lsrm)
ARC_DEF_TRANS_DOP_REG_S12(lsrm)
ARC_DEF_TRANS_DOP_COND_REG(lsrm)
ARC_DEF_TRANS_DOP_COND_U6(lsrm)
ARC_DEF_TRANS_S_DOP_3_OPERANDS(lsrm)
ARC_DEF_TRANS_RS32_RS32_I32_F(lsrm, lsrm_s_b_b_u5, a->b, a->b, a->imm, false)

static inline void arc64_gen_lsrml(DisasContext *ctx, TCGv_i64 a, TCGv_i64 b, TCGv_i64 c, bool set_flags)
{
    TCGv_i64 tmp = tcg_temp_new_i64();

    tcg_gen_andi_i64(c, c, 0x3F); /* src2 & 63 */
    tcg_gen_shr_i64(a, b, c); /* ((unsigned) src1) >> (src2 & 63) */

    if (set_flags) {
        arc64_gen_set_flag_Z(a);
        arc64_gen_set_flag_N(a);

        /* C_flag = if src2==0 then 0 else src1[src2 - 1] */
        TCGLabel *skip = gen_new_label();

        tcg_gen_movi_i32(cpu_Cf, 0);
        tcg_gen_brcondi_i64(TCG_COND_EQ, c, 0, skip);
        tcg_gen_subi_i64(c, c, 1); /* src2 - 1 */
        tcg_gen_shr_i64(tmp, b, c); /* src1[src2 - 1] */
        tcg_gen_extrl_i64_i32(cpu_Cf, tmp);
        tcg_gen_andi_i32(cpu_Cf, cpu_Cf, 1);

        gen_set_label(skip);
    }
}

ARC64_DEF_TRANS_DOP_REG_REG(lsrml)
ARC64_DEF_TRANS_DOP_REG_U6(lsrml)
ARC64_DEF_TRANS_DOP_REG_S12(lsrml)
ARC64_DEF_TRANS_DOP_COND_REG(lsrml)
ARC64_DEF_TRANS_DOP_COND_U6(lsrml)

static void arc_gen_lsr16(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_shri_i32(b, c, 16);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        arc_gen_set_flag_N_clear();
    }
}

ARC_DEF_TRANS_SOP_REG_REG(lsr16)
ARC_DEF_TRANS_SOP_REG_U6(lsr16)

static void arc_gen_lsr8(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_shri_i32(b, c, 8);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        arc_gen_set_flag_N_clear();
    }
}

ARC_DEF_TRANS_SOP_REG_REG(lsr8)
ARC_DEF_TRANS_SOP_REG_U6(lsr8)

/*
 * Rotate left
 *
 * b = (c << 1) | (c >> 31)
 *
 * dest = (src << 1) | (src >> 31)
 * if F==1 then
 *     Z_flag = if dest==0 then 1 else 0
 *     N_flag = dest[31]
 *     C_flag = src[31]
 */
static void arc_gen_rol(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_rotli_i32(b, c, 1);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        arc_gen_set_flag_N(b);
        tcg_gen_shri_i32(cpu_Cf, c, TARGET_LONG_BITS_32 - 1);
    }
}

ARC_DEF_TRANS_SOP_REG_REG(rol)
ARC_DEF_TRANS_SOP_REG_U6(rol)

/*
 * Rotate left 8
 *
 * dest = (src << 8) | (src >> 24)
 *
 * if F==1 then
 *   Z_flag = if dest==0 then 1 else 0
 *   N_flag = dest[31]
 */
static void arc_gen_rol8(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_rotli_i32(b, c, 8);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        arc_gen_set_flag_N(b);
    }
}

ARC_DEF_TRANS_SOP_REG_REG(rol8)
ARC_DEF_TRANS_SOP_REG_U6(rol8)

/*
 * Rotate right
 *
 * C = c & 1; b = c>>1 | c<<31
 *
 * dest = src >> 1
 * dest[31] = src[0]
 * if F==1 then
 *     Z_flag = if dest==0 then 1 else 0
 *     N_flag = dest[31]
 *     C_flag = src[0]
 */
static void arc_gen_ror(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_rotri_i32(b, c, 1);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        tcg_gen_andi_i32(cpu_Nf, c, 1);
        tcg_gen_andi_i32(cpu_Cf, c, 1);
    }
}

ARC_DEF_TRANS_SOP_REG_REG(ror)
ARC_DEF_TRANS_SOP_REG_U6(ror)

/*
 * Rotate right 8
 *
 * dest = (src >> 8) | (src << 24)
 *
 * if F==1 then
 *   Z_flag = if dest==0 then 1 else 0
 *   N_flag = dest[31]
 */
static void arc_gen_ror8(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_rotri_i32(b, c, 8);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        arc_gen_set_flag_N(b);
    }
}

ARC_DEF_TRANS_SOP_REG_REG(ror8)
ARC_DEF_TRANS_SOP_REG_U6(ror8)

/*
 * Rotate left through carry
 *
 * b = c << 1; b = b | C; C = c >> 31
 *
 * dest = src << 1
 * dest[0] = C_flag
 * if F==1 then
 *     Z_flag = if dest==0 then 1 else 0
 *     N_flag = dest[31]
 *     C_flag = src[31]
 *     V_flag = UNDEFINED
 */
static void arc_gen_rlc(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_shli_i32(b, c, 1);
    tcg_gen_or_i32(b, b, cpu_Cf);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        arc_gen_set_flag_N(b);
        tcg_gen_shri_i32(cpu_Cf, c, TARGET_LONG_BITS_32 - 1);
    }
}

ARC_DEF_TRANS_SOP_REG_REG(rlc)
ARC_DEF_TRANS_SOP_REG_U6(rlc)

/*
 * Rotate right through carry
 *
 * b = c >> 1 | C << 31; C = c & 1
 *
 * dest = src >> 1
 * dest[31] = C_flag
 * if F==1 then
 *     Z_flag = if dest==0 then 1 else 0
 *     N_flag = dest[31]
 *     C_flag = src[0]
 */
static void arc_gen_rrc(DisasContext *ctx, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    tcg_gen_andi_i32(b, c, ~1);
    tcg_gen_or_i32(b, b, cpu_Cf);
    tcg_gen_rotri_i32(b, b, 1);

    if (set_flags) {
        arc_gen_set_flag_Z(b);
        tcg_gen_mov_i32(cpu_Nf, cpu_Cf);
        tcg_gen_andi_i32(cpu_Cf, c, 1);
    }
}

ARC_DEF_TRANS_SOP_REG_REG(rrc)
ARC_DEF_TRANS_SOP_REG_U6(rrc)

/*
 * Multiple rotate right
 *
 * if (cc) {C = b >> (c-1) & 1; a = b >> c | b << (31-c);}
 *
 * if cc=true then
 *   dest = src1 >> (src2 & 31)
 *   dest [31:(31-src2)] = src1 [(src2-1):0]
 *   if F==1 then
 *     Z_flag = if dest==0 then 1 else 0
 *     N_flag = dest[31]
 *     C_flag = if src2==0 then 0 else src1[src2-1]
 */
static void arc_gen_rorm(DisasContext *ctx, TCGv_i32 a, TCGv_i32 b, TCGv_i32 c, bool set_flags)
{
    TCGLabel *skip = gen_new_label();

    tcg_gen_andi_i32(c, c, 0x1F);
    tcg_gen_rotr_i32(a, b, c);

    if (set_flags) {
        arc_gen_set_flag_Z(a);
        arc_gen_set_flag_N(a);

        /* C_flag = if src2==0 then 0 else src1[src2 - 1] */
        tcg_gen_movi_i32(cpu_Cf, 0);
        tcg_gen_brcondi_i32(TCG_COND_EQ, c, 0, skip);
        tcg_gen_subi_i32(c, c, 1); /* src2 - 1 */
        tcg_gen_shr_i32(cpu_Cf, b, c); /* src1[src2 - 1] */
        tcg_gen_andi_i32(cpu_Cf, cpu_Cf, 1);

        gen_set_label(skip);
    }
}

ARC_DEF_TRANS_DOP_REG_REG(rorm)
ARC_DEF_TRANS_DOP_REG_U6(rorm)
ARC_DEF_TRANS_DOP_REG_S12(rorm)
ARC_DEF_TRANS_DOP_COND_REG(rorm)
ARC_DEF_TRANS_DOP_COND_U6(rorm)
