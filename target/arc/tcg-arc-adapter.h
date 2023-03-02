/*
 * QEMU ARC CPU
 *
 * Copyright (c) 2020 Synppsys Inc.
 * Contributed by Cupertino Miranda <cmiranda@synopsys.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * http://www.gnu.org/licenses/lgpl-2.1.html
 */

#ifndef TCG_ARC_ADAPTER_H_
#define TCG_ARC_ADAPTER_H_

#include "translate.h"
#include "tcg/tcg.h"
#include "qemu/bitops.h"

typedef enum ARC_COND {
    ARC_COND_AL      = 0x00,
    ARC_COND_RA      = 0x00,
    ARC_COND_EQ      = 0x01,
    ARC_COND_Z       = 0x01,
    ARC_COND_NE      = 0x02,
    ARC_COND_NZ      = 0x02,
    ARC_COND_PL      = 0x03,
    ARC_COND_P       = 0x03,
    ARC_COND_MI      = 0x04,
    ARC_COND_N       = 0x04,
    ARC_COND_CS      = 0x05,
    ARC_COND_C       = 0x05,
    ARC_COND_LO      = 0x05,
    ARC_COND_CC      = 0x06,
    ARC_COND_NC      = 0x06,
    ARC_COND_HS      = 0x06,
    ARC_COND_VS      = 0x07,
    ARC_COND_V       = 0x07,
    ARC_COND_VC      = 0x08,
    ARC_COND_NV      = 0x08,
    ARC_COND_GT      = 0x09,
    ARC_COND_GE      = 0x0a,
    ARC_COND_LT      = 0x0b,
    ARC_COND_LE      = 0x0c,
    ARC_COND_HI      = 0x0d,
    ARC_COND_LS      = 0x0e,
    ARC_COND_PNZ     = 0x0f,
} ARC_COND;

#define ARC_HELPER(NAME, RET, ...) \
    gen_helper_##NAME(RET, cpu_env, __VA_ARGS__)


enum arc_registers {
    R_SP = 0,
    R_STATUS32,
    R_ACCLO,
    R_ACCHI
};

enum target_options {
    INVALID_TARGET_OPTIONS = -1,
    DIV_REM_OPTION,
    STACK_CHECKING,
    LL64_OPTION
};

void arc_gen_verifyCCFlag(const DisasCtxt *ctx, TCGv ret);

void arc_gen_set_memory(
        const DisasCtxt *ctx, TCGv addr, int size, TCGv src, bool sign_extend);

void arc_gen_get_memory(
        const DisasCtxt *ctx, TCGv ret, TCGv addr, int size, bool sign_extend);

void arc_gen_no_further_loads_pending(const DisasCtxt *ctx, TCGv ret);

void arc_gen_set_debug(const DisasCtxt *ctx, bool value);

void arc_has_interrupts(const DisasCtxt *ctx, TCGv ret);

/* Statically inferred return function */

TCGv arc_gen_next_reg(const DisasCtxt *ctx, TCGv reg, bool fail);

bool arc_target_has_option(enum target_options option);

bool arc_is_instruction_operand_a_register(const DisasCtxt *ctx, int nop);

void arc_gen_get_register(TCGv ret, enum arc_registers reg);

void arc_gen_set_register(enum arc_registers reg, TCGv value);

void to_implement(const DisasCtxt *ctx);
void to_implement_wo_abort(const DisasCtxt *ctx);

/*       Flag operations       */
#define getFlagX()  (ctx->insn.x)
#define getZZFlag() (ctx->insn.zz)
#define getAAFlag() (ctx->insn.aa)

#define getCCFlag(R)    arc_gen_verifyCCFlag(ctx, R)

#define getFFlag(R) ((int) ctx->insn.f)

/*       Function wrappers       */
#define NoFurtherLoadsPending(R)    arc_gen_no_further_loads_pending(ctx, R)

#define setDebugLD(A)   arc_gen_set_debug(ctx, A)

#define setMemory(ADDRESS, SIZE, VALUE) \
    arc_gen_set_memory(ctx, ADDRESS, SIZE, VALUE, getFlagX())

#define getMemory(R, ADDRESS, SIZE) \
    arc_gen_get_memory(ctx, R, ADDRESS, SIZE, getFlagX())

/*       Other helping macros       */

#define getRegIndex(R, ID)  tcg_gen_movi_tl(R, (int) ID)

#define readAuxReg(R, A)    gen_helper_lr(R, cpu_env, A)
/*
 * Here, by returning DISAS_UPDATE we are making SR the end
 * of a Translation Block (TB). This is necessary because
 * sometimes writing to control registers updates how a TB is
 * handled, like enabling MMU/MPU. If SR is not marked as the
 * end, the next instructions are fetched and generated and
 * the updated outcome (page/region permissions) is not taken
 * into account.
 */
#define writeAuxReg(NAME, B)             \
    do {                                 \
        gen_helper_sr(cpu_env, B, NAME); \
        ret = DISAS_UPDATE;              \
    } while (0)

#define setBLINK(BLINK_ADDR) \
  tcg_gen_mov_tl(cpu_blink, BLINK_ADDR);

#define executeDelaySlot(bta, take_branch) \
    ctx->env->in_delayslot_instruction = false; \
    ctx->env->next_insn_is_delayslot = true; \
    TCG_SET_STATUS_FIELD_VALUE(cpu_pstate, DEf, take_branch); \
    TCG_SET_STATUS_FIELD_BIT(cpu_pstate, PREVIOUS_IS_DELAYSLOTf);

#define shouldExecuteDelaySlot()    (ctx->insn.d != 0)

#define nextInsnAddressAfterDelaySlot(R) \
  { \
    ARCCPU *cpu = env_archcpu(ctx->env); \
    uint16_t delayslot_buffer[2]; \
    uint8_t delayslot_length; \
    ctx->env->pc = ctx->cpc; \
    delayslot_buffer[0] = cpu_lduw_code(ctx->env, ctx->npc); \
    delayslot_length = arc_insn_length(delayslot_buffer[0], cpu->family); \
    tcg_gen_movi_tl(R, ctx->npc + delayslot_length); \
  }

#define nextInsnAddress(R)  tcg_gen_movi_tl(R, ctx->npc)
#define getPCL(R)           tcg_gen_movi_tl(R, ctx->pcl)

#define setPC(NEW_PC)                                   \
    do {                                                \
        if(ctx->insn.d == 0) {                          \
            gen_goto_tb(ctx, 1, NEW_PC);                    \
            ret = ret == DISAS_NEXT ? DISAS_NORETURN : ret; \
        } else {                                        \
            tcg_gen_mov_tl(cpu_bta, NEW_PC);            \
        }                                               \
    } while (0)


/* TODO: Change this to allow something else then ARC HS. */
#define LP_START    \
    (arc_aux_reg_address_for(AUX_ID_lp_start, ARC_OPCODE_ARCv2HS))
#define LP_END      \
    (arc_aux_reg_address_for(AUX_ID_lp_end, ARC_OPCODE_ARCv2HS))

/* TODO: To implement */
#define Halt()

#define doNothing()


#define nextReg(A) arc_gen_next_reg(ctx, A, true)
#define nextRegWithNull(A) arc_gen_next_reg(ctx, A, false)

/* TODO (issue #62): This must be removed. */
#define Zero()  (ctx->zero)

#define getRegister(R, REG) arc_gen_get_register(R, REG)

#define setRegister(REG, VALUE) \
    arc_gen_set_register(REG, VALUE); \
    if (REG == R_STATUS32) { \
        ret = DISAS_NORETURN; \
    } \

#define inKernelMode(R) { \
  TCG_GET_STATUS_FIELD_MASKED(R, cpu_pstate, Uf); \
  tcg_gen_setcondi_tl(TCG_COND_EQ, R, R, 0); \
}
/* TODO: This is going to be revisited. */
#define throwExcpPriviledgeV() \
    arc_gen_excp(ctx, EXCP_PRIVILEGEV, 0, 0);



#endif