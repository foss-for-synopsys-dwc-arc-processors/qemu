#ifndef ARC_CPU_EXTRACTORS_H
#define ARC_CPU_EXTRACTORS_H

static int32_t ex_insn16_simm3(DisasContext *ctx, int32_t simm) {
    if (simm == 7) {
        return -1;
    } else {
        return simm;
    }
}

static int32_t ex_shl2(DisasContext *ctx, int32_t simm) {
    return simm << 2;
}

static int32_t ex_shl1(DisasContext *ctx, int32_t simm) {
    return simm << 1;
}

static int ex_bcc_s_cond(DisasContext *ctx, int imm)
{
    int cond = ARC_CC_AL;

    switch (imm) {
    case 0x0:
        cond = ARC_CC_GT;
        break;
    case 0x1:
        cond = ARC_CC_GE;
        break;
    case 0x2:
        cond = ARC_CC_LT;
        break;
    case 0x3:
        cond = ARC_CC_LE;
        break;
    case 0x4:
        cond = ARC_CC_HI;
        break;
    case 0x5:
        cond = ARC_CC_HS;
        break;
    case 0x6:
        cond = ARC_CC_LO;
        break;
    case 0x7:
        cond = ARC_CC_LS;
        break;
    default:
        g_assert_not_reached();
    }

    return cond;
}

static int ex_brcc_cond(DisasContext *ctx, int imm)
{
    int cond = ARC_CC_AL;

    switch (imm) {
    case 0x0:
        cond = ARC_CC_EQ;
        break;
    case 0x1:
        cond = ARC_CC_NE;
        break;
    case 0x2:
        cond = ARC_CC_LT;
        break;
    case 0x3:
        cond = ARC_CC_GE;
        break;
    case 0x4:
        cond = ARC_CC_LO;
        break;
    case 0x5:
        cond = ARC_CC_HS;
        break;
    default:
        g_assert_not_reached();
    }

    return cond;
}

static int ex_brcc_s_cond(DisasContext *ctx, int imm)
{
    int cond = ARC_CC_AL;

    switch (imm) {
    case 0x0:
        cond = ARC_CC_EQ;
        break;
    case 0x1:
        cond = ARC_CC_NE;
        break;
    default:
        g_assert_not_reached();
    }

    return cond;
}

static int ex_setcc_op(DisasContext *ctx, int imm)
{
    int cond = ARC_CC_AL;

    switch (imm) {
    case 0x0:
        cond = ARC_CC_EQ;
        break;
    case 0x1:
        cond = ARC_CC_NE;
        break;
    case 0x2:
        cond = ARC_CC_LT;
        break;
    case 0x3:
        cond = ARC_CC_GE;
        break;
    case 0x4:
        cond = ARC_CC_LO;
        break;
    case 0x5:
        cond = ARC_CC_HS;
        break;
    case 0x6:
        cond = ARC_CC_LE;
        break;
    case 0x7:
        cond = ARC_CC_GT;
        break;
    default:
        g_assert_not_reached();
    }

    return cond;
}

static int64_t ex_imm_dword(DisasContext *ctx, int64_t imm)
{
    int64_t word = imm & ((uint64_t) 0xFFFFFFFFU);
    return (word << 32) | word;
}

static int ex_imm_dhalf(DisasContext *ctx, int imm)
{
    int half = imm & 0xFFFF;
    return (half << 16) | half;
}

static int64_t ex_imm_qhalf(DisasContext *ctx, int64_t imm)
{
    int64_t half = imm & 0xFFFF;
    int64_t result = half;

    result = deposit64(result, 16, 16, half);
    result = deposit64(result, 32, 16, half);
    result = deposit64(result, 48, 16, half);

    return result;
}

#endif
