/*
 *  QEMU ARC CPU
 *
 *  Copyright (c) 2017 Cupertino Miranda
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, see
 *  <http://www.gnu.org/licenses/lgpl-2.1.html>
 */

#include "qemu/osdep.h"
#include "translate.h"
#include "semfunc.h"
#include "exec/gen-icount.h"
#include "tcg/tcg-op-gvec.h"
#include "fpu.h"

/* FLAG
 *    Variables: @src
 *    Functions: getCCFlag, getRegister, getBit, hasInterrupts, Halt, ReplMask, targetHasOption, setRegister
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      status32 = getRegister (R_STATUS32);
      if(((getBit (@src, 0) == 1) && (getBit (status32, 7) == 0)))
        {
          if((hasInterrupts () > 0))
            {
              status32 = (status32 | 1);
              Halt ();
            };
        }
      else
        {
          ReplMask (status32, @src, 3840);
          if(((getBit (status32, 7) == 0) && (hasInterrupts () > 0)))
            {
              ReplMask (status32, @src, 30);
              if(targetHasOption (DIV_REM_OPTION))
                {
                  ReplMask (status32, @src, 8192);
                };
              if(targetHasOption (STACK_CHECKING))
                {
                  ReplMask (status32, @src, 16384);
                };
              if(targetHasOption (LL64_OPTION))
                {
                  ReplMask (status32, @src, 524288);
                };
              ReplMask (status32, @src, 1048576);
            };
        };
      setRegister (R_STATUS32, status32);
    };
}
 */

int
arc_gen_FLAG (DisasCtxt *ctx, TCGv src)
{
  int ret = DISAS_NEXT;
  TCGv temp_13 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_14 = tcg_temp_local_new();
  TCGv status32 = tcg_temp_local_new();
  TCGv temp_16 = tcg_temp_local_new();
  TCGv temp_15 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_18 = tcg_temp_local_new();
  TCGv temp_17 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_19 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_20 = tcg_temp_local_new();
  TCGv temp_22 = tcg_temp_local_new();
  TCGv temp_21 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_23 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  TCGv temp_12 = tcg_temp_local_new();
  TCGv temp_24 = tcg_temp_local_new();
  TCGv temp_25 = tcg_temp_local_new();
  TCGv temp_26 = tcg_temp_local_new();
  TCGv temp_27 = tcg_temp_local_new();
  TCGv temp_28 = tcg_temp_local_new();
  getCCFlag(temp_13);
  tcg_gen_mov_tl(cc_flag, temp_13);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  getRegister(temp_14, R_STATUS32);
  tcg_gen_mov_tl(status32, temp_14);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_movi_tl(temp_16, 0);
  getBit(temp_15, src, temp_16);
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, temp_15, 1);
  tcg_gen_movi_tl(temp_18, 7);
  getBit(temp_17, status32, temp_18);
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_4, temp_17, 0);
  tcg_gen_and_tl(temp_5, temp_3, temp_4);
  tcg_gen_xori_tl(temp_6, temp_5, 1); tcg_gen_andi_tl(temp_6, temp_6, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_2);;
  TCGLabel *done_3 = gen_new_label();
  hasInterrupts(temp_19);
  tcg_gen_setcondi_tl(TCG_COND_GT, temp_7, temp_19, 0);
  tcg_gen_xori_tl(temp_8, temp_7, 1); tcg_gen_andi_tl(temp_8, temp_8, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_8, arc_true, done_3);;
  tcg_gen_ori_tl(status32, status32, 1);
  Halt();
  gen_set_label(done_3);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_movi_tl(temp_20, 3840);
  ReplMask(status32, src, temp_20);
  TCGLabel *done_4 = gen_new_label();
  tcg_gen_movi_tl(temp_22, 7);
  getBit(temp_21, status32, temp_22);
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_9, temp_21, 0);
  hasInterrupts(temp_23);
  tcg_gen_setcondi_tl(TCG_COND_GT, temp_10, temp_23, 0);
  tcg_gen_and_tl(temp_11, temp_9, temp_10);
  tcg_gen_xori_tl(temp_12, temp_11, 1); tcg_gen_andi_tl(temp_12, temp_12, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_12, arc_true, done_4);;
  tcg_gen_movi_tl(temp_24, 30);
  ReplMask(status32, src, temp_24);
  if (targetHasOption (DIV_REM_OPTION))
    {
    tcg_gen_movi_tl(temp_25, 8192);
  ReplMask(status32, src, temp_25);
;
    }
  else
    {
  ;
    }
  if (targetHasOption (STACK_CHECKING))
    {
    tcg_gen_movi_tl(temp_26, 16384);
  ReplMask(status32, src, temp_26);
;
    }
  else
    {
  ;
    }
  if (targetHasOption (LL64_OPTION))
    {
    tcg_gen_movi_tl(temp_27, 524288);
  ReplMask(status32, src, temp_27);
;
    }
  else
    {
  ;
    }
  tcg_gen_movi_tl(temp_28, 1048576);
  ReplMask(status32, src, temp_28);
  gen_set_label(done_4);
  gen_set_label(done_2);
  setRegister(R_STATUS32, status32);
  gen_set_label(done_1);
  tcg_temp_free(temp_13);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_14);
  tcg_temp_free(status32);
  tcg_temp_free(temp_16);
  tcg_temp_free(temp_15);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_18);
  tcg_temp_free(temp_17);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_19);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_20);
  tcg_temp_free(temp_22);
  tcg_temp_free(temp_21);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_23);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_11);
  tcg_temp_free(temp_12);
  tcg_temp_free(temp_24);
  tcg_temp_free(temp_25);
  tcg_temp_free(temp_26);
  tcg_temp_free(temp_27);
  tcg_temp_free(temp_28);

  return ret;
}





/* KFLAG
 *    Variables: @src
 *    Functions: getCCFlag, getRegister, getBit, hasInterrupts, Halt, ReplMask, targetHasOption, setRegister
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      status32 = getRegister (R_STATUS32);
      if(((getBit (@src, 0) == 1) && (getBit (status32, 7) == 0)))
        {
          if((hasInterrupts () > 0))
            {
              status32 = (status32 | 1);
              Halt ();
            };
        }
      else
        {
          ReplMask (status32, @src, 3840);
          if(((getBit (status32, 7) == 0) && (hasInterrupts () > 0)))
            {
              ReplMask (status32, @src, 62);
              if(targetHasOption (DIV_REM_OPTION))
                {
                  ReplMask (status32, @src, 8192);
                };
              if(targetHasOption (STACK_CHECKING))
                {
                  ReplMask (status32, @src, 16384);
                };
              ReplMask (status32, @src, 65536);
              if(targetHasOption (LL64_OPTION))
                {
                  ReplMask (status32, @src, 524288);
                };
              ReplMask (status32, @src, 1048576);
              ReplMask (status32, @src, 2147483648);
            };
        };
      setRegister (R_STATUS32, status32);
    };
}
 */

int
arc_gen_KFLAG (DisasCtxt *ctx, TCGv src)
{
  int ret = DISAS_NEXT;
  TCGv temp_13 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_14 = tcg_temp_local_new();
  TCGv status32 = tcg_temp_local_new();
  TCGv temp_16 = tcg_temp_local_new();
  TCGv temp_15 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_18 = tcg_temp_local_new();
  TCGv temp_17 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_19 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_20 = tcg_temp_local_new();
  TCGv temp_22 = tcg_temp_local_new();
  TCGv temp_21 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_23 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  TCGv temp_12 = tcg_temp_local_new();
  TCGv temp_24 = tcg_temp_local_new();
  TCGv temp_25 = tcg_temp_local_new();
  TCGv temp_26 = tcg_temp_local_new();
  TCGv temp_27 = tcg_temp_local_new();
  TCGv temp_28 = tcg_temp_local_new();
  TCGv temp_29 = tcg_temp_local_new();
  TCGv temp_30 = tcg_temp_local_new();
  getCCFlag(temp_13);
  tcg_gen_mov_tl(cc_flag, temp_13);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  getRegister(temp_14, R_STATUS32);
  tcg_gen_mov_tl(status32, temp_14);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_movi_tl(temp_16, 0);
  getBit(temp_15, src, temp_16);
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, temp_15, 1);
  tcg_gen_movi_tl(temp_18, 7);
  getBit(temp_17, status32, temp_18);
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_4, temp_17, 0);
  tcg_gen_and_tl(temp_5, temp_3, temp_4);
  tcg_gen_xori_tl(temp_6, temp_5, 1); tcg_gen_andi_tl(temp_6, temp_6, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_2);;
  TCGLabel *done_3 = gen_new_label();
  hasInterrupts(temp_19);
  tcg_gen_setcondi_tl(TCG_COND_GT, temp_7, temp_19, 0);
  tcg_gen_xori_tl(temp_8, temp_7, 1); tcg_gen_andi_tl(temp_8, temp_8, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_8, arc_true, done_3);;
  tcg_gen_ori_tl(status32, status32, 1);
  Halt();
  gen_set_label(done_3);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_movi_tl(temp_20, 3840);
  ReplMask(status32, src, temp_20);
  TCGLabel *done_4 = gen_new_label();
  tcg_gen_movi_tl(temp_22, 7);
  getBit(temp_21, status32, temp_22);
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_9, temp_21, 0);
  hasInterrupts(temp_23);
  tcg_gen_setcondi_tl(TCG_COND_GT, temp_10, temp_23, 0);
  tcg_gen_and_tl(temp_11, temp_9, temp_10);
  tcg_gen_xori_tl(temp_12, temp_11, 1); tcg_gen_andi_tl(temp_12, temp_12, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_12, arc_true, done_4);;
  tcg_gen_movi_tl(temp_24, 62);
  ReplMask(status32, src, temp_24);
  if (targetHasOption (DIV_REM_OPTION))
    {
    tcg_gen_movi_tl(temp_25, 8192);
  ReplMask(status32, src, temp_25);
;
    }
  else
    {
  ;
    }
  if (targetHasOption (STACK_CHECKING))
    {
    tcg_gen_movi_tl(temp_26, 16384);
  ReplMask(status32, src, temp_26);
;
    }
  else
    {
  ;
    }
  tcg_gen_movi_tl(temp_27, 65536);
  ReplMask(status32, src, temp_27);
  if (targetHasOption (LL64_OPTION))
    {
    tcg_gen_movi_tl(temp_28, 524288);
  ReplMask(status32, src, temp_28);
;
    }
  else
    {
  ;
    }
  tcg_gen_movi_tl(temp_29, 1048576);
  ReplMask(status32, src, temp_29);
  tcg_gen_movi_tl(temp_30, 2147483648);
  ReplMask(status32, src, temp_30);
  gen_set_label(done_4);
  gen_set_label(done_2);
  setRegister(R_STATUS32, status32);
  gen_set_label(done_1);
  tcg_temp_free(temp_13);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_14);
  tcg_temp_free(status32);
  tcg_temp_free(temp_16);
  tcg_temp_free(temp_15);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_18);
  tcg_temp_free(temp_17);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_19);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_20);
  tcg_temp_free(temp_22);
  tcg_temp_free(temp_21);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_23);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_11);
  tcg_temp_free(temp_12);
  tcg_temp_free(temp_24);
  tcg_temp_free(temp_25);
  tcg_temp_free(temp_26);
  tcg_temp_free(temp_27);
  tcg_temp_free(temp_28);
  tcg_temp_free(temp_29);
  tcg_temp_free(temp_30);

  return ret;
}





/* ADD
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64, getFFlag, setZFlag, setNFlag32, setCFlag, CarryADD32, setVFlag, OverflowADD32
--- code ---
{
  cc_flag = getCCFlag ();
  lb = se32to64 (@b);
  lc = se32to64 (@c);
  if((cc_flag == true))
    {
      @a = ((lb + lc) & 4294967295);
      @a = (@a & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
          setCFlag (CarryADD32 (@a, lb, lc));
          setVFlag (OverflowADD32 (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_ADD (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  se32to64(temp_4, b);
  tcg_gen_mov_tl(lb, temp_4);
  se32to64(temp_5, c);
  tcg_gen_mov_tl(lc, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_add_tl(temp_6, lb, lc);
  tcg_gen_andi_tl(a, temp_6, 4294967295);
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag32(a);
  CarryADD32(temp_8, a, lb, lc);
  tcg_gen_mov_tl(temp_7, temp_8);
  setCFlag(temp_7);
  OverflowADD32(temp_10, a, lb, lc);
  tcg_gen_mov_tl(temp_9, temp_10);
  setVFlag(temp_9);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_4);
  tcg_temp_free(lb);
  tcg_temp_free(temp_5);
  tcg_temp_free(lc);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);

  return ret;
}




/*
 * ADD1
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32, setCFlag, CarryADD32,
 *               setVFlag, OverflowADD32, se32to64
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = se32to64 (@b);
 *   lc = se32to64 (@c << 1);
 *   if((cc_flag == true))
 *     {
 *       @a = (@b + lc) & 0xffffffff;
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag32 (@a);
 *           setCFlag (CarryADD32 (@a, lb, lc));
 *           setVFlag (OverflowADD32 (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_ADD1(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    se32to64(lb, b);
    tcg_gen_shli_tl(temp_4, c, 1);
    se32to64(lc, temp_4);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_add_tl(temp_5, b, lc);
    tcg_gen_andi_tl(a, temp_5, 4294967295);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag32(a);
        CarryADD32(temp_6, a, lb, lc);
        setCFlag(temp_6);
        OverflowADD32(temp_7, a, lb, lc);
        setVFlag(temp_7);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(temp_4);
    tcg_temp_free(lc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_7);

    return ret;
}



/*
 * ADD2
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32, setCFlag, CarryADD32,
 *               setVFlag, OverflowADD32, se32to64
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = se32to64 (@b);
 *   lc = se32to64 (@c << 2);
 *   if((cc_flag == true))
 *     {
 *       @a = (@b + lc) & 0xffffffff;
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag32 (@a);
 *           setCFlag (CarryADD32 (@a, lb, lc));
 *           setVFlag (OverflowADD32 (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_ADD2(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    se32to64(lb, b);
    tcg_gen_shli_tl(temp_4, c, 2);
    se32to64(lc, temp_4);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_add_tl(temp_5, b, lc);
    tcg_gen_andi_tl(a, temp_5, 4294967295);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag32(a);
        CarryADD32(temp_6, a, lb, lc);
        setCFlag(temp_6);
        OverflowADD32(temp_7, a, lb, lc);
        setVFlag(temp_7);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(temp_4);
    tcg_temp_free(lc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_7);

    return ret;
}




/*
 * ADD3
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32, setCFlag, CarryADD32,
 *               setVFlag, OverflowADD32, se32to64
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = se32to64 (@b);
 *   lc = se32to64 (@c << 3);
 *   if((cc_flag == true))
 *     {
 *       @a = (@b + lc) & 0xffffffff;
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag32 (@a);
 *           setCFlag (CarryADD32 (@a, lb, lc));
 *           setVFlag (OverflowADD32 (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_ADD3(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    se32to64(lb, b);
    tcg_gen_shli_tl(temp_4, c, 3);
    se32to64(lc, temp_4);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_add_tl(temp_5, b, lc);
    tcg_gen_andi_tl(a, temp_5, 4294967295);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag32(a);
        CarryADD32(temp_6, a, lb, lc);
        setCFlag(temp_6);
        OverflowADD32(temp_7, a, lb, lc);
        setVFlag(temp_7);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(temp_4);
    tcg_temp_free(lc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_6);
    tcg_temp_free(temp_7);

    return ret;
}







/* ADC
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64, getCFlag, getFFlag, setZFlag, setNFlag32, setCFlag, CarryADD32, setVFlag, OverflowADD32
--- code ---
{
  cc_flag = getCCFlag ();
  lb = se32to64 (@b);
  lc = se32to64 (@c);
  if((cc_flag == true))
    {
      @a = ((lb + lc) + getCFlag ());
      @a = (@a & 4294967295);
      @a = (@a & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
          setCFlag (CarryADD32 (@a, lb, lc));
          setVFlag (OverflowADD32 (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_ADC (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_12 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  se32to64(temp_4, b);
  tcg_gen_mov_tl(lb, temp_4);
  se32to64(temp_5, c);
  tcg_gen_mov_tl(lc, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_add_tl(temp_6, lb, lc);
  getCFlag(temp_8);
  tcg_gen_mov_tl(temp_7, temp_8);
  tcg_gen_add_tl(a, temp_6, temp_7);
  tcg_gen_andi_tl(a, a, 4294967295);
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag32(a);
  CarryADD32(temp_10, a, lb, lc);
  tcg_gen_mov_tl(temp_9, temp_10);
  setCFlag(temp_9);
  OverflowADD32(temp_12, a, lb, lc);
  tcg_gen_mov_tl(temp_11, temp_12);
  setVFlag(temp_11);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_4);
  tcg_temp_free(lb);
  tcg_temp_free(temp_5);
  tcg_temp_free(lc);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_12);
  tcg_temp_free(temp_11);

  return ret;
}





/* SBC
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64, getCFlag, getFFlag, setZFlag, setNFlag32, setCFlag, CarrySUB32, setVFlag, OverflowSUB32
--- code ---
{
  cc_flag = getCCFlag ();
  lb = se32to64 (@b);
  lc = se32to64 (@c);
  if((cc_flag == true))
    {
      @a = ((lb - lc) - getCFlag ());
      @a = (@a & 4294967295);
      @a = (@a & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
          setCFlag (CarrySUB32 (@a, lb, lc));
          setVFlag (OverflowSUB32 (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_SBC (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_12 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  se32to64(temp_4, b);
  tcg_gen_mov_tl(lb, temp_4);
  se32to64(temp_5, c);
  tcg_gen_mov_tl(lc, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_sub_tl(temp_6, lb, lc);
  getCFlag(temp_8);
  tcg_gen_mov_tl(temp_7, temp_8);
  tcg_gen_sub_tl(a, temp_6, temp_7);
  tcg_gen_andi_tl(a, a, 4294967295);
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag32(a);
  CarrySUB32(temp_10, a, lb, lc);
  tcg_gen_mov_tl(temp_9, temp_10);
  setCFlag(temp_9);
  OverflowSUB32(temp_12, a, lb, lc);
  tcg_gen_mov_tl(temp_11, temp_12);
  setVFlag(temp_11);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_4);
  tcg_temp_free(lb);
  tcg_temp_free(temp_5);
  tcg_temp_free(lc);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_12);
  tcg_temp_free(temp_11);

  return ret;
}





/* NEG
 *    Variables: @b, @a
 *    Functions: getCCFlag, se32to64, getFFlag, setZFlag, setNFlag32, setCFlag, CarrySUB32, setVFlag, OverflowSUB32
--- code ---
{
  cc_flag = getCCFlag ();
  lb = se32to64 (@b);
  if((cc_flag == true))
    {
      @a = (0 - @b);
      @a = (@a & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
          setCFlag (CarrySUB32 (@a, 0, lb));
          setVFlag (OverflowSUB32 (@a, 0, lb));
        };
    };
}
 */

int
arc_gen_NEG (DisasCtxt *ctx, TCGv b, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  se32to64(temp_4, b);
  tcg_gen_mov_tl(lb, temp_4);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_subfi_tl(a, 0, b);
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag32(a);
  tcg_gen_movi_tl(temp_7, 0);
  CarrySUB32(temp_6, a, temp_7, lb);
  tcg_gen_mov_tl(temp_5, temp_6);
  setCFlag(temp_5);
  tcg_gen_movi_tl(temp_10, 0);
  OverflowSUB32(temp_9, a, temp_10, lb);
  tcg_gen_mov_tl(temp_8, temp_9);
  setVFlag(temp_8);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_4);
  tcg_temp_free(lb);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);

  return ret;
}





/* SUB
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64, getFFlag, setZFlag, setNFlag32, setCFlag, CarrySUB32, setVFlag, OverflowSUB32
--- code ---
{
  cc_flag = getCCFlag ();
  lb = se32to64 (@b);
  if((cc_flag == true))
    {
      lc = se32to64 (@c);
      @a = ((lb - lc) & 4294967295);
      @a = (@a & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
          setCFlag (CarrySUB32 (@a, lb, lc));
          setVFlag (OverflowSUB32 (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_SUB (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  se32to64(temp_4, b);
  tcg_gen_mov_tl(lb, temp_4);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  se32to64(temp_5, c);
  tcg_gen_mov_tl(lc, temp_5);
  tcg_gen_sub_tl(temp_6, lb, lc);
  tcg_gen_andi_tl(a, temp_6, 4294967295);
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag32(a);
  CarrySUB32(temp_8, a, lb, lc);
  tcg_gen_mov_tl(temp_7, temp_8);
  setCFlag(temp_7);
  OverflowSUB32(temp_10, a, lb, lc);
  tcg_gen_mov_tl(temp_9, temp_10);
  setVFlag(temp_9);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_4);
  tcg_temp_free(lb);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_5);
  tcg_temp_free(lc);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);

  return ret;
}





/* SUB1
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64, getFFlag, setZFlag, setNFlag32, setCFlag, CarrySUB32, setVFlag, OverflowSUB32
--- code ---
{
  cc_flag = getCCFlag ();
  lb = se32to64 (@b);
  if((cc_flag == true))
    {
      lc = (se32to64 (@c) << 1);
      @a = ((lb - lc) & 4294967295);
      @a = (@a & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
          setCFlag (CarrySUB32 (@a, lb, lc));
          setVFlag (OverflowSUB32 (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_SUB1 (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  se32to64(temp_4, b);
  tcg_gen_mov_tl(lb, temp_4);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  se32to64(temp_6, c);
  tcg_gen_mov_tl(temp_5, temp_6);
  tcg_gen_shli_tl(lc, temp_5, 1);
  tcg_gen_sub_tl(temp_7, lb, lc);
  tcg_gen_andi_tl(a, temp_7, 4294967295);
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag32(a);
  CarrySUB32(temp_9, a, lb, lc);
  tcg_gen_mov_tl(temp_8, temp_9);
  setCFlag(temp_8);
  OverflowSUB32(temp_11, a, lb, lc);
  tcg_gen_mov_tl(temp_10, temp_11);
  setVFlag(temp_10);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_4);
  tcg_temp_free(lb);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(lc);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_11);
  tcg_temp_free(temp_10);

  return ret;
}





/* SUB2
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64, getFFlag, setZFlag, setNFlag32, setCFlag, CarrySUB32, setVFlag, OverflowSUB32
--- code ---
{
  cc_flag = getCCFlag ();
  lb = se32to64 (@b);
  if((cc_flag == true))
    {
      lc = (se32to64 (@c) << 2);
      @a = ((lb - lc) & 4294967295);
      @a = (@a & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
          setCFlag (CarrySUB32 (@a, lb, lc));
          setVFlag (OverflowSUB32 (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_SUB2 (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  se32to64(temp_4, b);
  tcg_gen_mov_tl(lb, temp_4);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  se32to64(temp_6, c);
  tcg_gen_mov_tl(temp_5, temp_6);
  tcg_gen_shli_tl(lc, temp_5, 2);
  tcg_gen_sub_tl(temp_7, lb, lc);
  tcg_gen_andi_tl(a, temp_7, 4294967295);
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag32(a);
  CarrySUB32(temp_9, a, lb, lc);
  tcg_gen_mov_tl(temp_8, temp_9);
  setCFlag(temp_8);
  OverflowSUB32(temp_11, a, lb, lc);
  tcg_gen_mov_tl(temp_10, temp_11);
  setVFlag(temp_10);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_4);
  tcg_temp_free(lb);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(lc);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_11);
  tcg_temp_free(temp_10);

  return ret;
}





/* SUB3
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64, getFFlag, setZFlag, setNFlag32, setCFlag, CarrySUB32, setVFlag, OverflowSUB32
--- code ---
{
  cc_flag = getCCFlag ();
  lb = se32to64 (@b);
  if((cc_flag == true))
    {
      lc = (se32to64 (@c) << 3);
      @a = ((lb - lc) & 4294967295);
      @a = (@a & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
          setCFlag (CarrySUB32 (@a, lb, lc));
          setVFlag (OverflowSUB32 (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_SUB3 (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  se32to64(temp_4, b);
  tcg_gen_mov_tl(lb, temp_4);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  se32to64(temp_6, c);
  tcg_gen_mov_tl(temp_5, temp_6);
  tcg_gen_shli_tl(lc, temp_5, 3);
  tcg_gen_sub_tl(temp_7, lb, lc);
  tcg_gen_andi_tl(a, temp_7, 4294967295);
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag32(a);
  CarrySUB32(temp_9, a, lb, lc);
  tcg_gen_mov_tl(temp_8, temp_9);
  setCFlag(temp_8);
  OverflowSUB32(temp_11, a, lb, lc);
  tcg_gen_mov_tl(temp_10, temp_11);
  setVFlag(temp_10);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_4);
  tcg_temp_free(lb);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(lc);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_11);
  tcg_temp_free(temp_10);

  return ret;
}





/* MAX
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64, getFFlag, setZFlag, setNFlag32, setCFlag, CarrySUB32, setVFlag, OverflowSUB32
--- code ---
{
  cc_flag = getCCFlag ();
  lb = se32to64 (@b);
  if((cc_flag == true))
    {
      lc = se32to64 (@c);
      alu = (lb - lc);
      if((lc >= lb))
        {
          @a = lc;
        }
      else
        {
          @a = lb;
        };
      alu = (alu & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (alu);
          setNFlag32 (alu);
          setCFlag (CarrySUB32 (@a, lb, lc));
          setVFlag (OverflowSUB32 (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_MAX (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv alu = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  se32to64(temp_6, b);
  tcg_gen_mov_tl(lb, temp_6);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  se32to64(temp_7, c);
  tcg_gen_mov_tl(lc, temp_7);
  tcg_gen_sub_tl(alu, lb, lc);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_GE, temp_3, lc, lb);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_mov_tl(a, lc);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_mov_tl(a, lb);
  gen_set_label(done_2);
  tcg_gen_andi_tl(alu, alu, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(alu);
  setNFlag32(alu);
  CarrySUB32(temp_9, a, lb, lc);
  tcg_gen_mov_tl(temp_8, temp_9);
  setCFlag(temp_8);
  OverflowSUB32(temp_11, a, lb, lc);
  tcg_gen_mov_tl(temp_10, temp_11);
  setVFlag(temp_10);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_6);
  tcg_temp_free(lb);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_7);
  tcg_temp_free(lc);
  tcg_temp_free(alu);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_11);
  tcg_temp_free(temp_10);

  return ret;
}





/* MIN
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64, getFFlag, setZFlag, setNFlag32, setCFlag, CarrySUB32, setVFlag, OverflowSUB32
--- code ---
{
  cc_flag = getCCFlag ();
  lb = se32to64 (@b);
  if((cc_flag == true))
    {
      lc = se32to64 (@c);
      alu = (lb - lc);
      if((lc <= lb))
        {
          @a = lc;
        }
      else
        {
          @a = lb;
        };
      alu = (alu & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (alu);
          setNFlag32 (alu);
          setCFlag (CarrySUB32 (@a, lb, lc));
          setVFlag (OverflowSUB32 (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_MIN (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv alu = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  se32to64(temp_6, b);
  tcg_gen_mov_tl(lb, temp_6);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  se32to64(temp_7, c);
  tcg_gen_mov_tl(lc, temp_7);
  tcg_gen_sub_tl(alu, lb, lc);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_LE, temp_3, lc, lb);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_mov_tl(a, lc);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_mov_tl(a, lb);
  gen_set_label(done_2);
  tcg_gen_andi_tl(alu, alu, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(alu);
  setNFlag32(alu);
  CarrySUB32(temp_9, a, lb, lc);
  tcg_gen_mov_tl(temp_8, temp_9);
  setCFlag(temp_8);
  OverflowSUB32(temp_11, a, lb, lc);
  tcg_gen_mov_tl(temp_10, temp_11);
  setVFlag(temp_10);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_6);
  tcg_temp_free(lb);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_7);
  tcg_temp_free(lc);
  tcg_temp_free(alu);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_11);
  tcg_temp_free(temp_10);

  return ret;
}





/* CMP
 *    Variables: @b, @c
 *    Functions: getCCFlag, setZFlag, setNFlag32, setCFlag, CarrySUB32, setVFlag, OverflowSUB32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      v = 4294967295;
      lb = (@b & v);
      lc = (@c & v);
      alu = (lb - lc);
      alu = (alu & 4294967295);
      setZFlag (alu);
      setNFlag32 (alu);
      setCFlag (CarrySUB32 (alu, lb, lc));
      setVFlag (OverflowSUB32 (alu, lb, lc));
    };
}
 */

int
arc_gen_CMP (DisasCtxt *ctx, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv v = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv alu = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_movi_tl(v, 4294967295);
  tcg_gen_and_tl(lb, b, v);
  tcg_gen_and_tl(lc, c, v);
  tcg_gen_sub_tl(alu, lb, lc);
  tcg_gen_andi_tl(alu, alu, 4294967295);
  setZFlag(alu);
  setNFlag32(alu);
  CarrySUB32(temp_5, alu, lb, lc);
  tcg_gen_mov_tl(temp_4, temp_5);
  setCFlag(temp_4);
  OverflowSUB32(temp_7, alu, lb, lc);
  tcg_gen_mov_tl(temp_6, temp_7);
  setVFlag(temp_6);
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(v);
  tcg_temp_free(lb);
  tcg_temp_free(lc);
  tcg_temp_free(alu);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);

  return ret;
}





/* AND
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = (@b & @c);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_AND (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_and_tl(a, b, c);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);

  return ret;
}





/* OR
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = (@b | @c);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_OR (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_or_tl(a, b, c);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);

  return ret;
}





/* XOR
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = (@b ^ @c);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_XOR (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_xor_tl(a, b, c);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);

  return ret;
}





/* MOV
 *    Variables: @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = @b;
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_MOV (DisasCtxt *ctx, TCGv a, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(a, b);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);

  return ret;
}





/* ASL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32, setCFlag, getBit, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lb = (@b & 4294967295);
      lc = (@c & 31);
      la = (lb << lc);
      la = (la & 4294967295);
      @a = la;
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
          if((lc == 0))
            {
              setCFlag (0);
            }
          else
            {
              setCFlag (getBit (lb, (32 - lc)));
            };
          if((@c == 268435457))
            {
              t1 = getBit (la, 31);
              t2 = getBit (lb, 31);
              if((t1 == t2))
                {
                  setVFlag (0);
                }
              else
                {
                  setVFlag (1);
                };
            };
        };
    };
}
 */

int
arc_gen_ASL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_9 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv la = tcg_temp_local_new();
  int f_flag;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_13 = tcg_temp_local_new();
  TCGv temp_12 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_15 = tcg_temp_local_new();
  TCGv temp_14 = tcg_temp_local_new();
  TCGv t1 = tcg_temp_local_new();
  TCGv temp_17 = tcg_temp_local_new();
  TCGv temp_16 = tcg_temp_local_new();
  TCGv t2 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_18 = tcg_temp_local_new();
  TCGv temp_19 = tcg_temp_local_new();
  getCCFlag(temp_9);
  tcg_gen_mov_tl(cc_flag, temp_9);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lb, b, 4294967295);
  tcg_gen_andi_tl(lc, c, 31);
  tcg_gen_shl_tl(la, lb, lc);
  tcg_gen_andi_tl(la, la, 4294967295);
  tcg_gen_mov_tl(a, la);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, lc, 0);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_movi_tl(temp_10, 0);
  setCFlag(temp_10);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_subfi_tl(temp_13, 32, lc);
  getBit(temp_12, lb, temp_13);
  tcg_gen_mov_tl(temp_11, temp_12);
  setCFlag(temp_11);
  gen_set_label(done_2);
  TCGLabel *done_3 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_5, c, 268435457);
  tcg_gen_xori_tl(temp_6, temp_5, 1); tcg_gen_andi_tl(temp_6, temp_6, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, done_3);;
  tcg_gen_movi_tl(temp_15, 31);
  getBit(temp_14, la, temp_15);
  tcg_gen_mov_tl(t1, temp_14);
  tcg_gen_movi_tl(temp_17, 31);
  getBit(temp_16, lb, temp_17);
  tcg_gen_mov_tl(t2, temp_16);
  TCGLabel *else_4 = gen_new_label();
  TCGLabel *done_4 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_7, t1, t2);
  tcg_gen_xori_tl(temp_8, temp_7, 1); tcg_gen_andi_tl(temp_8, temp_8, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_8, arc_true, else_4);;
  tcg_gen_movi_tl(temp_18, 0);
  setVFlag(temp_18);
  tcg_gen_br(done_4);
  gen_set_label(else_4);
  tcg_gen_movi_tl(temp_19, 1);
  setVFlag(temp_19);
  gen_set_label(done_4);
  gen_set_label(done_3);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_9);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lb);
  tcg_temp_free(lc);
  tcg_temp_free(la);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_13);
  tcg_temp_free(temp_12);
  tcg_temp_free(temp_11);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_15);
  tcg_temp_free(temp_14);
  tcg_temp_free(t1);
  tcg_temp_free(temp_17);
  tcg_temp_free(temp_16);
  tcg_temp_free(t2);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_18);
  tcg_temp_free(temp_19);

  return ret;
}





/* ASR
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, arithmeticShiftRight32, getFFlag, setZFlag, setNFlag32, setCFlag, getBit
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lb = (@b & 4294967295);
      lc = (@c & 31);
      @a = arithmeticShiftRight32 (lb, lc);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
          if((lc == 0))
            {
              setCFlag (0);
            }
          else
            {
              setCFlag (getBit (lb, (lc - 1)));
            };
        };
    };
}
 */

int
arc_gen_ASR (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  int f_flag;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lb, b, 4294967295);
  tcg_gen_andi_tl(lc, c, 31);
  arithmeticShiftRight32(temp_6, lb, lc);
  tcg_gen_mov_tl(a, temp_6);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, lc, 0);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_movi_tl(temp_7, 0);
  setCFlag(temp_7);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_subi_tl(temp_10, lc, 1);
  getBit(temp_9, lb, temp_10);
  tcg_gen_mov_tl(temp_8, temp_9);
  setCFlag(temp_8);
  gen_set_label(done_2);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lb);
  tcg_temp_free(lc);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);

  return ret;
}





/* ASR8
 *    Variables: @b, @a
 *    Functions: getCCFlag, arithmeticShiftRight32, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lb = (@b & 4294967295);
      @a = arithmeticShiftRight32 (lb, 8);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_ASR8 (DisasCtxt *ctx, TCGv b, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lb, b, 4294967295);
  tcg_gen_movi_tl(temp_5, 8);
  arithmeticShiftRight32(temp_4, lb, temp_5);
  tcg_gen_mov_tl(a, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lb);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* ASR16
 *    Variables: @b, @a
 *    Functions: getCCFlag, arithmeticShiftRight32, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lb = (@b & 4294967295);
      @a = arithmeticShiftRight32 (lb, 16);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_ASR16 (DisasCtxt *ctx, TCGv b, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lb, b, 4294967295);
  tcg_gen_movi_tl(temp_5, 16);
  arithmeticShiftRight32(temp_4, lb, temp_5);
  tcg_gen_mov_tl(a, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lb);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* LSL16
 *    Variables: @b, @a
 *    Functions: getCCFlag, logicalShiftLeft, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lb = (@b & 4294967295);
      @a = logicalShiftLeft (lb, 16);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_LSL16 (DisasCtxt *ctx, TCGv b, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lb, b, 4294967295);
  tcg_gen_movi_tl(temp_5, 16);
  logicalShiftLeft(temp_4, lb, temp_5);
  tcg_gen_mov_tl(a, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lb);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* LSL8
 *    Variables: @b, @a
 *    Functions: getCCFlag, logicalShiftLeft, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lb = (@b & 4294967295);
      @a = logicalShiftLeft (lb, 8);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_LSL8 (DisasCtxt *ctx, TCGv b, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lb, b, 4294967295);
  tcg_gen_movi_tl(temp_5, 8);
  logicalShiftLeft(temp_4, lb, temp_5);
  tcg_gen_mov_tl(a, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lb);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* LSR
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, logicalShiftRight, getFFlag, setZFlag, setNFlag32, setCFlag, getBit
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lb = (@b & 4294967295);
      lc = (@c & 31);
      @a = logicalShiftRight (lb, lc);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
          if((lc == 0))
            {
              setCFlag (0);
            }
          else
            {
              setCFlag (getBit (lb, (lc - 1)));
            };
        };
    };
}
 */

int
arc_gen_LSR (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  int f_flag;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lb, b, 4294967295);
  tcg_gen_andi_tl(lc, c, 31);
  logicalShiftRight(temp_6, lb, lc);
  tcg_gen_mov_tl(a, temp_6);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, lc, 0);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_movi_tl(temp_7, 0);
  setCFlag(temp_7);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_subi_tl(temp_10, lc, 1);
  getBit(temp_9, lb, temp_10);
  tcg_gen_mov_tl(temp_8, temp_9);
  setCFlag(temp_8);
  gen_set_label(done_2);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lb);
  tcg_temp_free(lc);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);

  return ret;
}





/* LSR16
 *    Variables: @b, @a
 *    Functions: getCCFlag, logicalShiftRight, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lb = (@b & 4294967295);
      @a = logicalShiftRight (lb, 16);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_LSR16 (DisasCtxt *ctx, TCGv b, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lb, b, 4294967295);
  tcg_gen_movi_tl(temp_5, 16);
  logicalShiftRight(temp_4, lb, temp_5);
  tcg_gen_mov_tl(a, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lb);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* LSR8
 *    Variables: @b, @a
 *    Functions: getCCFlag, logicalShiftRight, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lb = (@b & 4294967295);
      @a = logicalShiftRight (lb, 8);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_LSR8 (DisasCtxt *ctx, TCGv b, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lb, b, 4294967295);
  tcg_gen_movi_tl(temp_5, 8);
  logicalShiftRight(temp_4, lb, temp_5);
  tcg_gen_mov_tl(a, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lb);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* BIC
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = (@b & ~@c);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_BIC (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_not_tl(temp_4, c);
  tcg_gen_and_tl(a, b, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);

  return ret;
}





/* BCLR
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp = (1 << (@c & 31));
      @a = (@b & ~tmp);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_BCLR (DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv tmp = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(temp_4, c, 31);
  tcg_gen_shlfi_tl(tmp, 1, temp_4);
  tcg_gen_not_tl(temp_5, tmp);
  tcg_gen_and_tl(a, b, temp_5);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);
  tcg_temp_free(tmp);
  tcg_temp_free(temp_5);

  return ret;
}





/* BMSK
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp1 = ((@c & 31) + 1);
      if((tmp1 == 32))
        {
          tmp2 = 0xffffffff;
        }
      else
        {
          tmp2 = ((1 << tmp1) - 1);
        };
      @a = (@b & tmp2);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_BMSK (DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv tmp1 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv tmp2 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(temp_6, c, 31);
  tcg_gen_addi_tl(tmp1, temp_6, 1);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, tmp1, 32);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_movi_tl(tmp2, 0xffffffff);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_shlfi_tl(temp_7, 1, tmp1);
  tcg_gen_subi_tl(tmp2, temp_7, 1);
  gen_set_label(done_2);
  tcg_gen_and_tl(a, b, tmp2);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(tmp1);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(tmp2);
  tcg_temp_free(temp_7);

  return ret;
}





/* BMSKN
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp1 = ((@c & 31) + 1);
      if((tmp1 == 32))
        {
          tmp2 = 0xffffffff;
        }
      else
        {
          tmp2 = ((1 << tmp1) - 1);
        };
      @a = (@b & ~tmp2);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_BMSKN (DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv tmp1 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv tmp2 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(temp_6, c, 31);
  tcg_gen_addi_tl(tmp1, temp_6, 1);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, tmp1, 32);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_movi_tl(tmp2, 0xffffffff);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_shlfi_tl(temp_7, 1, tmp1);
  tcg_gen_subi_tl(tmp2, temp_7, 1);
  gen_set_label(done_2);
  tcg_gen_not_tl(temp_8, tmp2);
  tcg_gen_and_tl(a, b, temp_8);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(tmp1);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(tmp2);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_8);

  return ret;
}





/* BSET
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp = (1 << (@c & 31));
      @a = (@b | tmp);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_BSET (DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv tmp = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(temp_4, c, 31);
  tcg_gen_shlfi_tl(tmp, 1, temp_4);
  tcg_gen_or_tl(a, b, tmp);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);
  tcg_temp_free(tmp);

  return ret;
}





/* BXOR
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp = (1 << @c);
      @a = (@b ^ tmp);
      f_flag = getFFlag ();
      @a = (@a & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
        };
    };
}
 */

int
arc_gen_BXOR (DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv tmp = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_shlfi_tl(tmp, 1, c);
  tcg_gen_xor_tl(a, b, tmp);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag32(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(tmp);

  return ret;
}





/* ROL
 *    Variables: @src, @n, @dest
 *    Functions: getCCFlag, rotateLeft32, getFFlag, setZFlag, setNFlag32, setCFlag, extractBits
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lsrc = (@src & 4294967295);
      ln = (@n & 31);
      @dest = rotateLeft32 (lsrc, ln);
      f_flag = getFFlag ();
      @dest = (@dest & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag32 (@dest);
          setCFlag (extractBits (lsrc, 31, 31));
        };
    };
}
 */

int
arc_gen_ROL (DisasCtxt *ctx, TCGv src, TCGv n, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lsrc = tcg_temp_local_new();
  TCGv ln = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lsrc, src, 4294967295);
  tcg_gen_andi_tl(ln, n, 31);
  rotateLeft32(temp_4, lsrc, ln);
  tcg_gen_mov_tl(dest, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
  tcg_gen_movi_tl(temp_8, 31);
  tcg_gen_movi_tl(temp_7, 31);
  extractBits(temp_6, lsrc, temp_7, temp_8);
  tcg_gen_mov_tl(temp_5, temp_6);
  setCFlag(temp_5);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lsrc);
  tcg_temp_free(ln);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);

  return ret;
}





/* ROL8
 *    Variables: @src, @dest
 *    Functions: getCCFlag, rotateLeft32, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lsrc = (@src & 4294967295);
      @dest = rotateLeft32 (lsrc, 8);
      f_flag = getFFlag ();
      @dest = (@dest & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag32 (@dest);
        };
    };
}
 */

int
arc_gen_ROL8 (DisasCtxt *ctx, TCGv src, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lsrc = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lsrc, src, 4294967295);
  tcg_gen_movi_tl(temp_5, 8);
  rotateLeft32(temp_4, lsrc, temp_5);
  tcg_gen_mov_tl(dest, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lsrc);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* ROR
 *    Variables: @src, @n, @dest
 *    Functions: getCCFlag, rotateRight32, getFFlag, setZFlag, setNFlag32, setCFlag, extractBits
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lsrc = (@src & 4294967295);
      ln = (@n & 31);
      @dest = rotateRight32 (lsrc, ln);
      f_flag = getFFlag ();
      @dest = (@dest & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag32 (@dest);
          setCFlag (extractBits (lsrc, (ln - 1), (ln - 1)));
        };
    };
}
 */

int
arc_gen_ROR (DisasCtxt *ctx, TCGv src, TCGv n, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lsrc = tcg_temp_local_new();
  TCGv ln = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lsrc, src, 4294967295);
  tcg_gen_andi_tl(ln, n, 31);
  rotateRight32(temp_4, lsrc, ln);
  tcg_gen_mov_tl(dest, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
  tcg_gen_subi_tl(temp_8, ln, 1);
  tcg_gen_subi_tl(temp_7, ln, 1);
  extractBits(temp_6, lsrc, temp_7, temp_8);
  tcg_gen_mov_tl(temp_5, temp_6);
  setCFlag(temp_5);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lsrc);
  tcg_temp_free(ln);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);

  return ret;
}





/* ROR8
 *    Variables: @src, @dest
 *    Functions: getCCFlag, rotateRight32, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lsrc = (@src & 4294967295);
      @dest = rotateRight32 (lsrc, 8);
      f_flag = getFFlag ();
      @dest = (@dest & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag32 (@dest);
        };
    };
}
 */

int
arc_gen_ROR8 (DisasCtxt *ctx, TCGv src, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lsrc = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lsrc, src, 4294967295);
  tcg_gen_movi_tl(temp_5, 8);
  rotateRight32(temp_4, lsrc, temp_5);
  tcg_gen_mov_tl(dest, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lsrc);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* RLC
 *    Variables: @src, @dest
 *    Functions: getCCFlag, getCFlag, getFFlag, setZFlag, setNFlag32, setCFlag, extractBits
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lsrc = (@src & 4294967295);
      @dest = (lsrc << 1);
      @dest = (@dest | getCFlag ());
      f_flag = getFFlag ();
      @dest = (@dest & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag32 (@dest);
          setCFlag (extractBits (lsrc, 31, 31));
        };
    };
}
 */

int
arc_gen_RLC (DisasCtxt *ctx, TCGv src, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lsrc = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lsrc, src, 4294967295);
  tcg_gen_shli_tl(dest, lsrc, 1);
  getCFlag(temp_5);
  tcg_gen_mov_tl(temp_4, temp_5);
  tcg_gen_or_tl(dest, dest, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
  tcg_gen_movi_tl(temp_9, 31);
  tcg_gen_movi_tl(temp_8, 31);
  extractBits(temp_7, lsrc, temp_8, temp_9);
  tcg_gen_mov_tl(temp_6, temp_7);
  setCFlag(temp_6);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lsrc);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);

  return ret;
}





/* RRC
 *    Variables: @src, @dest
 *    Functions: getCCFlag, getCFlag, getFFlag, setZFlag, setNFlag32, setCFlag, extractBits
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lsrc = (@src & 4294967295);
      @dest = (lsrc >> 1);
      @dest = (@dest | (getCFlag () << 31));
      f_flag = getFFlag ();
      @dest = (@dest & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag32 (@dest);
          setCFlag (extractBits (lsrc, 0, 0));
        };
    };
}
 */

int
arc_gen_RRC (DisasCtxt *ctx, TCGv src, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lsrc = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(lsrc, src, 4294967295);
  tcg_gen_shri_tl(dest, lsrc, 1);
  getCFlag(temp_6);
  tcg_gen_mov_tl(temp_5, temp_6);
  tcg_gen_shli_tl(temp_4, temp_5, 31);
  tcg_gen_or_tl(dest, dest, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
  tcg_gen_movi_tl(temp_10, 0);
  tcg_gen_movi_tl(temp_9, 0);
  extractBits(temp_8, lsrc, temp_9, temp_10);
  tcg_gen_mov_tl(temp_7, temp_8);
  setCFlag(temp_7);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lsrc);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);

  return ret;
}





/* SEXB
 *    Variables: @dest, @src
 *    Functions: getCCFlag, arithmeticShiftRight, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @dest = arithmeticShiftRight ((@src << 56), 56);
      f_flag = getFFlag ();
      @dest = (@dest & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag32 (@dest);
        };
    };
}
 */

int
arc_gen_SEXB (DisasCtxt *ctx, TCGv dest, TCGv src)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_movi_tl(temp_6, 56);
  tcg_gen_shli_tl(temp_5, src, 56);
  arithmeticShiftRight(temp_4, temp_5, temp_6);
  tcg_gen_mov_tl(dest, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(dest);
    setNFlag32(dest);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* SEXH
 *    Variables: @dest, @src
 *    Functions: getCCFlag, arithmeticShiftRight, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @dest = arithmeticShiftRight ((@src << 48), 48);
      f_flag = getFFlag ();
      @dest = (@dest & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag32 (@dest);
        };
    };
}
 */

int
arc_gen_SEXH (DisasCtxt *ctx, TCGv dest, TCGv src)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_movi_tl(temp_6, 48);
  tcg_gen_shli_tl(temp_5, src, 48);
  arithmeticShiftRight(temp_4, temp_5, temp_6);
  tcg_gen_mov_tl(dest, temp_4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* EXTB
 *    Variables: @dest, @src
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @dest = (@src & 255);
      f_flag = getFFlag ();
      @dest = (@dest & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag32 (@dest);
        };
    };
}
 */

int
arc_gen_EXTB (DisasCtxt *ctx, TCGv dest, TCGv src)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(dest, src, 255);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);

  return ret;
}





/* EXTH
 *    Variables: @dest, @src
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @dest = (@src & 65535);
      f_flag = getFFlag ();
      @dest = (@dest & 4294967295);
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag32 (@dest);
        };
    };
}
 */

int
arc_gen_EXTH (DisasCtxt *ctx, TCGv dest, TCGv src)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(dest, src, 65535);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);

  return ret;
}





/* BTST
 *    Variables: @c, @b
 *    Functions: getCCFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp = (1 << (@c & 31));
      alu = (@b & tmp);
      alu = (alu & 4294967295);
      setZFlag (alu);
      setNFlag32 (alu);
    };
}
 */

int
arc_gen_BTST (DisasCtxt *ctx, TCGv c, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv tmp = tcg_temp_local_new();
  TCGv alu = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(temp_4, c, 31);
  tcg_gen_shlfi_tl(tmp, 1, temp_4);
  tcg_gen_and_tl(alu, b, tmp);
  tcg_gen_andi_tl(alu, alu, 4294967295);
  setZFlag(alu);
  setNFlag32(alu);
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);
  tcg_temp_free(tmp);
  tcg_temp_free(alu);

  return ret;
}





/* TST
 *    Variables: @b, @c
 *    Functions: getCCFlag, setZFlag, setNFlag32
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      alu = (@b & @c);
      alu = (alu & 4294967295);
      setZFlag (alu);
      setNFlag32 (alu);
    };
}
 */

int
arc_gen_TST (DisasCtxt *ctx, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv alu = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_and_tl(alu, b, c);
  tcg_gen_andi_tl(alu, alu, 4294967295);
  setZFlag(alu);
  setNFlag32(alu);
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(alu);

  return ret;
}





/* XBFU
 *    Variables: @src2, @src1, @dest
 *    Functions: getCCFlag, extractBits, getFFlag, setZFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      N = extractBits (@src2, 4, 0);
      M = (extractBits (@src2, 9, 5) + 1);
      tmp1 = (@src1 >> N);
      tmp2 = ((1 << M) - 1);
      @dest = (tmp1 & tmp2);
      @dest = (@dest & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (@dest);
        };
    };
}
 */

int
arc_gen_XBFU (DisasCtxt *ctx, TCGv src2, TCGv src1, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv N = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv M = tcg_temp_local_new();
  TCGv tmp1 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  TCGv tmp2 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_movi_tl(temp_6, 0);
  tcg_gen_movi_tl(temp_5, 4);
  extractBits(temp_4, src2, temp_5, temp_6);
  tcg_gen_mov_tl(N, temp_4);
  tcg_gen_movi_tl(temp_10, 5);
  tcg_gen_movi_tl(temp_9, 9);
  extractBits(temp_8, src2, temp_9, temp_10);
  tcg_gen_mov_tl(temp_7, temp_8);
  tcg_gen_addi_tl(M, temp_7, 1);
  tcg_gen_shr_tl(tmp1, src1, N);
  tcg_gen_shlfi_tl(temp_11, 1, M);
  tcg_gen_subi_tl(tmp2, temp_11, 1);
  tcg_gen_and_tl(dest, tmp1, tmp2);
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(dest);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(N);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);
  tcg_temp_free(M);
  tcg_temp_free(tmp1);
  tcg_temp_free(temp_11);
  tcg_temp_free(tmp2);

  return ret;
}





/* AEX
 *    Variables: @src2, @b
 *    Functions: getCCFlag, readAuxReg, writeAuxReg
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp = readAuxReg (@src2);
      writeAuxReg (@src2, @b);
      @b = tmp;
    };
}
 */

int
arc_gen_AEX (DisasCtxt *ctx, TCGv src2, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv tmp = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  readAuxReg(temp_4, src2);
  tcg_gen_mov_tl(tmp, temp_4);
  writeAuxReg(src2, b);
  tcg_gen_mov_tl(b, tmp);
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);
  tcg_temp_free(tmp);

  return ret;
}





/* LR
 *    Variables: @dest, @src
 *    Functions: readAuxReg
--- code ---
{
  @dest = readAuxReg (@src);
}
 */

int
arc_gen_LR (DisasCtxt *ctx, TCGv dest, TCGv src)
{
  int ret = DISAS_NORETURN;

  if (tb_cflags(ctx->base.tb) & CF_USE_ICOUNT) {
      gen_io_start();
  }

  TCGv temp_1 = tcg_temp_local_new();
  readAuxReg(temp_1, src);
  tcg_gen_andi_tl(temp_1, temp_1, 0xffffffff);
  tcg_gen_mov_tl(dest, temp_1);
  tcg_temp_free(temp_1);

  return ret;
}





/* CLRI
 *    Variables: @c
 *    Functions: getRegister, setRegister, inKernelMode
--- code ---
{
  in_kernel_mode = inKernelMode();
  if(in_kernel_mode != 1)
    {
      throwExcpPriviledgeV();
    }
  status32 = getRegister (R_STATUS32);
  ie = (status32 & 2147483648);
  ie = (ie >> 27);
  e = ((status32 & 30) >> 1);
  a = 32;
  @c = ((ie | e) | a);
  mask = 2147483648;
  mask = ~mask;
  status32 = (status32 & mask);
  setRegister (R_STATUS32, status32);
}
 */

int
arc_gen_CLRI (DisasCtxt *ctx, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_1 = tcg_temp_local_new();
  TCGv status32 = tcg_temp_local_new();
  TCGv ie = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv e = tcg_temp_local_new();
  TCGv a = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv mask = tcg_temp_local_new();
  TCGv in_kernel_mode = tcg_temp_local_new();
  inKernelMode(in_kernel_mode);
  TCGLabel *done_in_kernel_mode = gen_new_label();
  tcg_gen_brcondi_tl(TCG_COND_EQ, in_kernel_mode, 1, done_in_kernel_mode);
  throwExcpPriviledgeV();
  gen_set_label(done_in_kernel_mode);
  getRegister(temp_1, R_STATUS32);
  tcg_gen_mov_tl(status32, temp_1);
  tcg_gen_andi_tl(ie, status32, 2147483648);
  tcg_gen_shri_tl(ie, ie, 27);
  tcg_gen_andi_tl(temp_2, status32, 30);
  tcg_gen_shri_tl(e, temp_2, 1);
  tcg_gen_movi_tl(a, 32);
  tcg_gen_or_tl(temp_3, ie, e);
  tcg_gen_or_tl(c, temp_3, a);
  tcg_gen_movi_tl(mask, 2147483648);
  tcg_gen_not_tl(mask, mask);
  tcg_gen_and_tl(status32, status32, mask);
  setRegister(R_STATUS32, status32);
  tcg_temp_free(temp_1);
  tcg_temp_free(status32);
  tcg_temp_free(ie);
  tcg_temp_free(temp_2);
  tcg_temp_free(e);
  tcg_temp_free(a);
  tcg_temp_free(temp_3);
  tcg_temp_free(mask);
  tcg_temp_free(in_kernel_mode);

  return ret;
}





/* SETI
 *    Variables: @c
 *    Functions: getRegister, setRegister, inKernelMode
--- code ---
{
  in_kernel_mode = inKernelMode();
  if(in_kernel_mode != 1)
    {
      throwExcpPriviledgeV();
    }
  status32 = getRegister (R_STATUS32);
  if((temp1 != 0))
    {
      status32 = ((status32 & e_mask) | e_value);
      ie_mask = 2147483648;
      ie_mask = ~ie_mask;
      ie_value = ((@c & 16) << 27);
      status32 = ((status32 & ie_mask) | ie_value);
    }
  else
    {
      status32 = (status32 | 2147483648);
      temp2 = (@c & 16);
      if((temp2 != 0))
        {
          status32 = ((status32 & e_mask) | e_value);
        };
    };
  setRegister (R_STATUS32, status32);
}
 */

int
arc_gen_SETI (DisasCtxt *ctx, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv status32 = tcg_temp_local_new();
  TCGv e_mask = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv e_value = tcg_temp_local_new();
  TCGv temp1 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv ie_mask = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv ie_value = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp2 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv in_kernel_mode = tcg_temp_local_new();
  inKernelMode(in_kernel_mode);
  TCGLabel *done_in_kernel_mode = gen_new_label();
  tcg_gen_brcondi_tl(TCG_COND_EQ, in_kernel_mode, 1, done_in_kernel_mode);
  throwExcpPriviledgeV();
  gen_set_label(done_in_kernel_mode);
  getRegister(temp_5, R_STATUS32);
  tcg_gen_mov_tl(status32, temp_5);
  tcg_gen_movi_tl(e_mask, 30);
  tcg_gen_not_tl(e_mask, e_mask);
  tcg_gen_andi_tl(temp_6, c, 15);
  tcg_gen_shli_tl(e_value, temp_6, 1);
  tcg_gen_andi_tl(temp1, c, 32);
  TCGLabel *else_1 = gen_new_label();
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_1, temp1, 0);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_1);;
  tcg_gen_and_tl(temp_7, status32, e_mask);
  tcg_gen_or_tl(status32, temp_7, e_value);
  tcg_gen_movi_tl(ie_mask, 2147483648);
  tcg_gen_not_tl(ie_mask, ie_mask);
  tcg_gen_andi_tl(temp_8, c, 16);
  tcg_gen_shli_tl(ie_value, temp_8, 27);
  tcg_gen_and_tl(temp_9, status32, ie_mask);
  tcg_gen_or_tl(status32, temp_9, ie_value);
  tcg_gen_br(done_1);
  gen_set_label(else_1);
  tcg_gen_ori_tl(status32, status32, 2147483648);
  tcg_gen_andi_tl(temp2, c, 16);
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_3, temp2, 0);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, done_2);;
  tcg_gen_and_tl(temp_10, status32, e_mask);
  tcg_gen_or_tl(status32, temp_10, e_value);
  gen_set_label(done_2);
  gen_set_label(done_1);
  setRegister(R_STATUS32, status32);
  tcg_temp_free(temp_5);
  tcg_temp_free(status32);
  tcg_temp_free(e_mask);
  tcg_temp_free(temp_6);
  tcg_temp_free(e_value);
  tcg_temp_free(temp1);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_7);
  tcg_temp_free(ie_mask);
  tcg_temp_free(temp_8);
  tcg_temp_free(ie_value);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp2);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_10);
  tcg_temp_free(in_kernel_mode);

  return ret;
}





/* NOP
 *    Variables:
 *    Functions: doNothing
--- code ---
{
  doNothing ();
}
 */

int
arc_gen_NOP (DisasCtxt *ctx)
{
  int ret = DISAS_NEXT;

  return ret;
}





/* PREALLOC
 *    Variables:
 *    Functions: doNothing
--- code ---
{
  doNothing ();
}
 */

int
arc_gen_PREALLOC (DisasCtxt *ctx)
{
  int ret = DISAS_NEXT;

  return ret;
}





/* PREFETCH
 *    Variables: @src1, @src2
 *    Functions: getAAFlag, doNothing
--- code ---
{
  AA = getAAFlag ();
  if(((AA == 1) || (AA == 2)))
    {
      @src1 = (@src1 + @src2);
    }
  else
    {
      doNothing ();
    };
}
 */

int
arc_gen_PREFETCH (DisasCtxt *ctx, TCGv src1, TCGv src2)
{
  int ret = DISAS_NEXT;
  int AA;
  AA = getAAFlag ();
  if (((AA == 1) || (AA == 2)))
    {
    tcg_gen_add_tl(src1, src1, src2);
;
    }
  else
    {
    doNothing();
;
    }

  return ret;
}





/* MPY
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, HELPER, setZFlag, setNFlag32, setVFlag
 *  HAND_TUNED FUNCTION - not really optimized yet
 */

int
arc_gen_MPY(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv _b = tcg_temp_local_new();
    TCGv _c = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv high_part = tcg_temp_local_new();
    TCGv tmp1 = tcg_temp_local_new();
    TCGv tmp2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_andi_tl(_b, b, 0xffffffff);
    tcg_gen_andi_tl(_c, c, 0xffffffff);
    tcg_gen_ext32s_tl(_b, _b);
    tcg_gen_ext32s_tl(_c, _c);
    tcg_gen_mul_tl(temp_4, _b, _c);
    tcg_gen_andi_tl(a, temp_4, 4294967295);
    if ((getFFlag () == true)) {
	tcg_gen_sari_tl(high_part, temp_4, 32);
        tcg_gen_sari_tl(tmp2, temp_4, 31);
        setZFlag(a);
        setNFlag(high_part);
        tcg_gen_setcond_tl(TCG_COND_NE, temp_5, high_part, tmp2);
        setVFlag(temp_5);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(_b);
    tcg_temp_free(_c);
    tcg_temp_free(temp_4);
    tcg_temp_free(high_part);
    tcg_temp_free(tmp1);
    tcg_temp_free(tmp2);
    tcg_temp_free(temp_5);

    return ret;
}





/* MPYMU
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, HELPER, getFFlag, setZFlag, setNFlag32, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = HELPER (mpymu, @b, @c);
      @a = (@a & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag32 (0);
          setVFlag (0);
        };
    };
}
 */

int
arc_gen_MPYMU (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  ARC_HELPER(mpymu, a, b, c);
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  tcg_gen_movi_tl(temp_4, 0);
  setNFlag32(temp_4);
  tcg_gen_movi_tl(temp_5, 0);
  setVFlag(temp_5);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);

  return ret;
}





/* MPYM
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, HELPER, getFFlag, setZFlag, setNFlag32, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = HELPER (mpym, @b, @c);
      @a = (@a & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
          setVFlag (0);
        };
    };
}
 */

int
arc_gen_MPYM (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  ARC_HELPER(mpym, a, b, c);
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag32(a);
  tcg_gen_movi_tl(temp_4, 0);
  setVFlag(temp_4);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);

  return ret;
}





/* MPYU
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, HELPER, setZFlag, setNFlag32, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      _b = @b;
      _c = @c;
      @a = ((_b * _c) & 4294967295);
      @a = (@a & 4294967295);
      if((getFFlag () == true))
        {
          high_part = HELPER (mpymu, _b, _c);
          setZFlag (@a);
          setNFlag32 (0);
          setVFlag ((high_part != 0));
        };
    };
}
 */

int
arc_gen_MPYU (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv _b = tcg_temp_local_new();
  TCGv _c = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv high_part = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(_b, b);
  tcg_gen_mov_tl(_c, c);
  tcg_gen_mul_tl(temp_4, _b, _c);
  tcg_gen_andi_tl(a, temp_4, 4294967295);
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((getFFlag () == true))
    {
    ARC_HELPER(mpym, high_part, _b, _c);
  setZFlag(a);
  tcg_gen_movi_tl(temp_5, 0);
  setNFlag32(temp_5);
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_6, high_part, 0);
  setVFlag(temp_6);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(_b);
  tcg_temp_free(_c);
  tcg_temp_free(temp_4);
  tcg_temp_free(high_part);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);

  return ret;
}





/* MPYUW
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag32, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = ((@b & 65535) * (@c & 65535));
      @a = (@a & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag32 (0);
          setVFlag (0);
        };
    };
}
 */

int
arc_gen_MPYUW (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(temp_5, c, 65535);
  tcg_gen_andi_tl(temp_4, b, 65535);
  tcg_gen_mul_tl(a, temp_4, temp_5);
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  tcg_gen_movi_tl(temp_6, 0);
  setNFlag32(temp_6);
  tcg_gen_movi_tl(temp_7, 0);
  setVFlag(temp_7);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_7);

  return ret;
}





/* MPYW
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, arithmeticShiftRight, getFFlag, setZFlag, setNFlag32, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = (arithmeticShiftRight ((@b << 48), 48) * arithmeticShiftRight ((@c << 48), 48));
      @a = (@a & 4294967295);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag32 (@a);
          setVFlag (0);
        };
    };
}
 */

int
arc_gen_MPYW (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_12 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_movi_tl(temp_11, 48);
  tcg_gen_shli_tl(temp_10, c, 48);
  tcg_gen_movi_tl(temp_7, 48);
  tcg_gen_shli_tl(temp_6, b, 48);
  arithmeticShiftRight(temp_5, temp_6, temp_7);
  tcg_gen_mov_tl(temp_4, temp_5);
  arithmeticShiftRight(temp_9, temp_10, temp_11);
  tcg_gen_mov_tl(temp_8, temp_9);
  tcg_gen_mul_tl(a, temp_4, temp_8);
  tcg_gen_andi_tl(a, a, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag32(a);
  tcg_gen_movi_tl(temp_12, 0);
  setVFlag(temp_12);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_11);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_12);

  return ret;
}





/* DIV
 *    Variables: @src2, @src1, @dest
 *    Functions: getCCFlag, divSigned32, getFFlag, setZFlag, setNFlag32, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      if(((@src2 != 0) && ((@src1 != 2147483648) || (@src2 != 4294967295))))
        {
          @dest = divSigned32 (@src1, @src2);
          if((getFFlag () == true))
            {
              setZFlag (@dest);
              setNFlag32 (@dest);
              setVFlag (0);
            };
        }
      else
        {
        };
    };
}
 */

int
arc_gen_DIV (DisasCtxt *ctx, TCGv src2, TCGv src1, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_9 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  getCCFlag(temp_9);
  tcg_gen_mov_tl(cc_flag, temp_9);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_3, src2, 0);
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_4, src1, 2147483648);
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_5, src2, 4294967295);
  tcg_gen_or_tl(temp_6, temp_4, temp_5);
  tcg_gen_and_tl(temp_7, temp_3, temp_6);
  tcg_gen_xori_tl(temp_8, temp_7, 1); tcg_gen_andi_tl(temp_8, temp_8, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_8, arc_true, else_2);;
  divSigned32(temp_10, src1, src2);
  tcg_gen_mov_tl(dest, temp_10);
  if ((getFFlag () == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
  tcg_gen_movi_tl(temp_11, 0);
  setVFlag(temp_11);
;
    }
  else
    {
  ;
    }
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_9);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_11);

  return ret;
}





/* DIVU
 *    Variables: @src2, @dest, @src1
 *    Functions: getCCFlag, divUnsigned32, getFFlag, setZFlag, setNFlag32, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      if((@src2 != 0))
        {
          @dest = divUnsigned32 (@src1, @src2);
          if((getFFlag () == true))
            {
              setZFlag (@dest);
              setNFlag32 (0);
              setVFlag (0);
            };
        }
      else
        {
        };
    };
}
 */

int
arc_gen_DIVU (DisasCtxt *ctx, TCGv src2, TCGv dest, TCGv src1)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_3, src2, 0);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  divUnsigned32(temp_6, src1, src2);
  tcg_gen_mov_tl(dest, temp_6);
  if ((getFFlag () == true))
    {
    setZFlag(dest);
  tcg_gen_movi_tl(temp_7, 0);
  setNFlag32(temp_7);
  tcg_gen_movi_tl(temp_8, 0);
  setVFlag(temp_8);
;
    }
  else
    {
  ;
    }
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_8);

  return ret;
}





/* REM
 *    Variables: @src2, @src1, @dest
 *    Functions: getCCFlag, divRemainingSigned32, getFFlag, setZFlag, setNFlag32, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      if(((@src2 != 0) && ((@src1 != 2147483648) || (@src2 != 4294967295))))
        {
          @dest = divRemainingSigned32 (@src1, @src2);
          if((getFFlag () == true))
            {
              setZFlag (@dest);
              setNFlag32 (@dest);
              setVFlag (0);
            };
        }
      else
        {
        };
    };
}
 */

int
arc_gen_REM (DisasCtxt *ctx, TCGv src2, TCGv src1, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_9 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  getCCFlag(temp_9);
  tcg_gen_mov_tl(cc_flag, temp_9);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_3, src2, 0);
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_4, src1, 2147483648);
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_5, src2, 4294967295);
  tcg_gen_or_tl(temp_6, temp_4, temp_5);
  tcg_gen_and_tl(temp_7, temp_3, temp_6);
  tcg_gen_xori_tl(temp_8, temp_7, 1); tcg_gen_andi_tl(temp_8, temp_8, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_8, arc_true, else_2);;
  divRemainingSigned32(temp_10, src1, src2);
  tcg_gen_mov_tl(dest, temp_10);
  if ((getFFlag () == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
  tcg_gen_movi_tl(temp_11, 0);
  setVFlag(temp_11);
;
    }
  else
    {
  ;
    }
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_9);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_11);

  return ret;
}





/* REMU
 *    Variables: @src2, @dest, @src1
 *    Functions: getCCFlag, divRemainingUnsigned32, getFFlag, setZFlag, setNFlag32, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      if((@src2 != 0))
        {
          @dest = divRemainingUnsigned32 (@src1, @src2);
          if((getFFlag () == true))
            {
              setZFlag (@dest);
              setNFlag32 (0);
              setVFlag (0);
            };
        }
      else
        {
        };
    };
}
 */

int
arc_gen_REMU (DisasCtxt *ctx, TCGv src2, TCGv dest, TCGv src1)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_3, src2, 0);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  divRemainingUnsigned32(temp_6, src1, src2);
  tcg_gen_mov_tl(dest, temp_6);
  if ((getFFlag () == true))
    {
    setZFlag(dest);
  tcg_gen_movi_tl(temp_7, 0);
  setNFlag32(temp_7);
  tcg_gen_movi_tl(temp_8, 0);
  setVFlag(temp_8);
;
    }
  else
    {
  ;
    }
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_8);

  return ret;
}

static void
arc_gen_mac_op_i64(TCGv mul_bc, TCGv a, TCGv b, TCGv c,
                   void (*OP)(TCGv, TCGv,
                              unsigned int ofs,
                              unsigned int len),
                   int w0)
{
    TCGv b_w0, c_w0;

    b_w0 = tcg_temp_new();
    c_w0 = tcg_temp_new();

    OP(b_w0, b, 0, 32);
    OP(c_w0, c, 0, 32);
    tcg_gen_mul_tl(mul_bc, b_w0, c_w0);
    tcg_gen_add_tl(cpu64_acc, cpu64_acc, mul_bc);
    tcg_gen_mov_tl(a, cpu64_acc);
    if (w0)
      tcg_gen_andi_tl(a, a, 0xffffffffull);

    tcg_temp_free(c_w0);
    tcg_temp_free(b_w0);
}

/*
 * MAC - CODED BY HAND
 */

int
arc_gen_MAC(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    TCGv old_acc, mul_bc;

    TCGv cc_temp = tcg_temp_local_new();
    TCGLabel *cc_done = gen_new_label();

    /* Conditional execution */
    getCCFlag(cc_temp);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

    old_acc = tcg_temp_new();
    mul_bc = tcg_temp_new();

    tcg_gen_mov_tl(old_acc, cpu64_acc);
    arc_gen_mac_op_i64(mul_bc, a, b, c,
                       tcg_gen_sextract_tl, TRUE);

    if (getFFlag()) { // F flag is set, affect the flags
        TCGLabel *vf_done = gen_new_label();
        TCGv vf_temp = tcg_temp_new();

        setNFlag(cpu64_acc);

        tcg_gen_mov_tl(vf_temp, cpu_Vf);
        OverflowADD(cpu_Vf, cpu64_acc, old_acc, mul_bc);
        tcg_gen_brcondi_tl(TCG_COND_EQ, vf_temp, 0, vf_done);
        tcg_gen_movi_tl(cpu_Vf, 1);
        gen_set_label(vf_done);

        tcg_temp_free(vf_temp);
    }

    tcg_temp_free(mul_bc);
    tcg_temp_free(old_acc);

    /* Conditional execution end. */
    gen_set_label(cc_done);
    tcg_temp_free(cc_temp);

    return DISAS_NEXT;
}

/*
 * MACU - CODED BY HAND
 */

int
arc_gen_MACU(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    TCGv old_acc, mul_bc;

    TCGv cc_temp = tcg_temp_local_new();
    TCGLabel *cc_done = gen_new_label();

    /* Conditional execution */
    getCCFlag(cc_temp);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

    old_acc = tcg_temp_new();
    mul_bc = tcg_temp_new();

    tcg_gen_mov_tl(old_acc, cpu64_acc);
    arc_gen_mac_op_i64(mul_bc, a, b, c,
                       tcg_gen_extract_tl, TRUE);

    if (getFFlag()) { // F flag is set, affect the flags
        TCGLabel *vf_done = gen_new_label();
        TCGv vf_temp = tcg_temp_new();

        tcg_gen_mov_tl(vf_temp, cpu_Vf);
        CarryADD(cpu_Vf, cpu64_acc, old_acc, mul_bc);
        tcg_gen_brcondi_tl(TCG_COND_EQ, vf_temp, 0, vf_done);
        tcg_gen_movi_tl(cpu_Vf, 1);
        gen_set_label(vf_done);

        tcg_temp_free(vf_temp);
    }

    tcg_temp_free(mul_bc);
    tcg_temp_free(old_acc);

    /* Conditional execution end. */
    gen_set_label(cc_done);
    tcg_temp_free(cc_temp);

    return DISAS_NEXT;
}

/*
 * MACD - CODED BY HAND
 */

int
arc_gen_MACD(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    TCGv old_acc, mul_bc;

    TCGv cc_temp = tcg_temp_local_new();
    TCGLabel *cc_done = gen_new_label();

    /* Conditional execution */
    getCCFlag(cc_temp);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

    old_acc = tcg_temp_new();
    mul_bc = tcg_temp_new();

    tcg_gen_mov_tl(old_acc, cpu64_acc);
    arc_gen_mac_op_i64(mul_bc, a, b, c,
                       tcg_gen_sextract_tl, FALSE);

    if (getFFlag()) { // F flag is set, affect the flags
        TCGLabel *vf_done = gen_new_label();
        TCGv vf_temp = tcg_temp_new();

        setNFlag(cpu64_acc);

        tcg_gen_mov_tl(vf_temp, cpu_Vf);
        OverflowADD(cpu_Vf, cpu64_acc, old_acc, mul_bc);
        tcg_gen_brcondi_tl(TCG_COND_EQ, vf_temp, 0, vf_done);
        tcg_gen_movi_tl(cpu_Vf, 1);
        gen_set_label(vf_done);

        tcg_temp_free(vf_temp);
    }

    tcg_temp_free(mul_bc);
    tcg_temp_free(old_acc);

    /* Conditional execution end. */
    gen_set_label(cc_done);
    tcg_temp_free(cc_temp);

    return DISAS_NEXT;
}

/*
 * MACDU - CODED BY HAND
 */

int
arc_gen_MACDU(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    TCGv old_acc, mul_bc;

    TCGv cc_temp = tcg_temp_local_new();
    TCGLabel *cc_done = gen_new_label();

    /* Conditional execution */
    getCCFlag(cc_temp);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

    old_acc = tcg_temp_new();
    mul_bc = tcg_temp_new();

    tcg_gen_mov_tl(old_acc, cpu64_acc);
    arc_gen_mac_op_i64(mul_bc, a, b, c,
                       tcg_gen_extract_tl, FALSE);

    if (getFFlag()) { // F flag is set, affect the flags
        TCGLabel *vf_done = gen_new_label();
        TCGv vf_temp = tcg_temp_new();

        tcg_gen_mov_tl(vf_temp, cpu_Vf);
        CarryADD(cpu_Vf, cpu64_acc, old_acc, mul_bc);
        tcg_gen_brcondi_tl(TCG_COND_EQ, vf_temp, 0, vf_done);
        tcg_gen_movi_tl(cpu_Vf, 1);
        gen_set_label(vf_done);

        tcg_temp_free(vf_temp);
    }

    tcg_temp_free(mul_bc);
    tcg_temp_free(old_acc);

    /* Conditional execution end. */
    gen_set_label(cc_done);
    tcg_temp_free(cc_temp);

    return DISAS_NEXT;
}

/* ABS
 *    Variables: @src, @dest
 *    Functions: se32to64, Carry32, getFFlag, setZFlag, setNFlag32, setCFlag, Zero, setVFlag, getNFlag
--- code ---
{
  lsrc = se32to64 (@src);
  alu = (0 - lsrc);
  if((Carry32 (lsrc) == 1))
    {
      @dest = alu;
    }
  else
    {
      @dest = lsrc;
    };
  @dest = (@dest & 4294967295);
  if((getFFlag () == true))
    {
      setZFlag (@dest);
      setNFlag32 (@dest);
      setCFlag (Zero ());
      setVFlag (getNFlag ());
    };
}
 */

int
arc_gen_ABS (DisasCtxt *ctx, TCGv src, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv lsrc = tcg_temp_local_new();
  TCGv alu = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  se32to64(temp_3, src);
  tcg_gen_mov_tl(lsrc, temp_3);
  tcg_gen_subfi_tl(alu, 0, lsrc);
  TCGLabel *else_1 = gen_new_label();
  TCGLabel *done_1 = gen_new_label();
  Carry32(temp_4, lsrc);
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_1, temp_4, 1);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_1);;
  tcg_gen_mov_tl(dest, alu);
  tcg_gen_br(done_1);
  gen_set_label(else_1);
  tcg_gen_mov_tl(dest, lsrc);
  gen_set_label(done_1);
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((getFFlag () == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
  tcg_gen_mov_tl(temp_5, Zero());
  setCFlag(temp_5);
  tcg_gen_mov_tl(temp_6, getNFlag());
  setVFlag(temp_6);
;
    }
  else
    {
  ;
    }
  tcg_temp_free(temp_3);
  tcg_temp_free(lsrc);
  tcg_temp_free(alu);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);

  return ret;
}





/* SWAP
 *    Variables: @src, @dest
 *    Functions: getFFlag, setZFlag, setNFlag32
--- code ---
{
  tmp1 = (@src << 16);
  tmp2 = ((@src >> 16) & 65535);
  @dest = (tmp1 | tmp2);
  f_flag = getFFlag ();
  @dest = (@dest & 4294967295);
  if((f_flag == true))
    {
      setZFlag (@dest);
      setNFlag32 (@dest);
    };
}
 */

int
arc_gen_SWAP (DisasCtxt *ctx, TCGv src, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv tmp1 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv tmp2 = tcg_temp_local_new();
  int f_flag;
  tcg_gen_shli_tl(tmp1, src, 16);
  tcg_gen_shri_tl(temp_1, src, 16);
  tcg_gen_andi_tl(tmp2, temp_1, 65535);
  tcg_gen_or_tl(dest, tmp1, tmp2);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
;
    }
  else
    {
  ;
    }
  tcg_temp_free(tmp1);
  tcg_temp_free(temp_1);
  tcg_temp_free(tmp2);

  return ret;
}





/* SWAPE
 *    Variables: @src, @dest
 *    Functions: getFFlag, setZFlag, setNFlag32
--- code ---
{
  tmp1 = ((@src << 24) & 4278190080);
  tmp2 = ((@src << 8) & 16711680);
  tmp3 = ((@src >> 8) & 65280);
  tmp4 = ((@src >> 24) & 255);
  @dest = (((tmp1 | tmp2) | tmp3) | tmp4);
  f_flag = getFFlag ();
  @dest = (@dest & 4294967295);
  if((f_flag == true))
    {
      setZFlag (@dest);
      setNFlag32 (@dest);
    };
}
 */

int
arc_gen_SWAPE (DisasCtxt *ctx, TCGv src, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_1 = tcg_temp_local_new();
  TCGv tmp1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv tmp2 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv tmp3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv tmp4 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  int f_flag;
  tcg_gen_shli_tl(temp_1, src, 24);
  tcg_gen_andi_tl(tmp1, temp_1, 4278190080);
  tcg_gen_shli_tl(temp_2, src, 8);
  tcg_gen_andi_tl(tmp2, temp_2, 16711680);
  tcg_gen_shri_tl(temp_3, src, 8);
  tcg_gen_andi_tl(tmp3, temp_3, 65280);
  tcg_gen_shri_tl(temp_4, src, 24);
  tcg_gen_andi_tl(tmp4, temp_4, 255);
  tcg_gen_or_tl(temp_6, tmp1, tmp2);
  tcg_gen_or_tl(temp_5, temp_6, tmp3);
  tcg_gen_or_tl(dest, temp_5, tmp4);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
;
    }
  else
    {
  ;
    }
  tcg_temp_free(temp_1);
  tcg_temp_free(tmp1);
  tcg_temp_free(temp_2);
  tcg_temp_free(tmp2);
  tcg_temp_free(temp_3);
  tcg_temp_free(tmp3);
  tcg_temp_free(temp_4);
  tcg_temp_free(tmp4);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);

  return ret;
}





/* NOT
 *    Variables: @dest, @src
 *    Functions: getFFlag, setZFlag, setNFlag32
--- code ---
{
  @dest = ~@src;
  f_flag = getFFlag ();
  @dest = (@dest & 4294967295);
  if((f_flag == true))
    {
      setZFlag (@dest);
      setNFlag32 (@dest);
    };
}
 */

int
arc_gen_NOT (DisasCtxt *ctx, TCGv dest, TCGv src)
{
  int ret = DISAS_NEXT;
  int f_flag;
  tcg_gen_not_tl(dest, src);
  f_flag = getFFlag ();
  tcg_gen_andi_tl(dest, dest, 4294967295);
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag32(dest);
;
    }
  else
    {
  ;
    }

  return ret;
}





/* BI
 *    Variables: @c
 *    Functions: setPC, nextInsnAddress
--- code ---
{
  setPC ((nextInsnAddress () + (@c << 2)));
}
 */

int
arc_gen_BI (DisasCtxt *ctx, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  tcg_gen_shli_tl(temp_4, c, 2);
  nextInsnAddress(temp_3);
  tcg_gen_mov_tl(temp_2, temp_3);
  tcg_gen_add_tl(temp_1, temp_2, temp_4);
  setPC(temp_1);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_1);

  return ret;
}


/*
 * BIH
 * --- code ---
 * target = ctx->npc + (c << 1);
 */
int
arc_gen_BIH(DisasCtxt *ctx, TCGv c)
{
    TCGv target = tcg_temp_local_new();
    TCGv addendum = tcg_temp_local_new();

    tcg_gen_movi_tl(target, ctx->npc);
    tcg_gen_shli_tl(addendum, c, 1);
    tcg_gen_add_tl(target, target, addendum);
    gen_goto_tb(ctx, target);

    tcg_temp_free(addendum);
    tcg_temp_free(target);

    return DISAS_NORETURN;
}


/*
 * B
 * --- code ---
 * target = cpl + offset
 *
 * if (cc_flag == true)
 *   gen_branchi(target)
 */
int
arc_gen_B(DisasCtxt *ctx, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[0].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    arc_gen_verifyCCFlag(ctx, cond);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cond, 0, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/*
 * BBIT0
 * --- code ---
 * target = cpl + offset
 *
 * _c = @c & 31
 * msk = 1 << _c
 * bit = @b & msk
 * if (bit == 0)
 *   gen_branchi(target)
 */
int
arc_gen_BBIT0(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv _c = tcg_temp_new();
    TCGv msk = tcg_const_tl(1);
    TCGv bit = tcg_temp_new();

    update_delay_flag(ctx);

    /* if ((b & (1 << (c & 31))) == 0) */
    tcg_gen_andi_tl(_c, c, 31);
    tcg_gen_shl_tl(msk, msk, _c);
    tcg_gen_and_tl(bit, b, msk);
    tcg_gen_brcondi_tl(TCG_COND_NE, bit, 0, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(bit);
    tcg_temp_free(msk);
    tcg_temp_free(_c);

    return DISAS_NORETURN;
}


/*
 * BBIT1
 * --- code ---
 * target = cpl + offset
 *
 * _c = @c & 31
 * msk = 1 << _c
 * bit = @b & msk
 * if (bit != 0)
 *   gen_branchi(target)
 */
int
arc_gen_BBIT1(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv _c = tcg_temp_new();
    TCGv msk = tcg_const_tl(1);
    TCGv bit = tcg_temp_new();

    update_delay_flag(ctx);

    /* if ((b & (1 << (c & 31))) != 0) */
    tcg_gen_andi_tl(_c, c, 31);
    tcg_gen_shl_tl(msk, msk, _c);
    tcg_gen_and_tl(bit, b, msk);
    tcg_gen_brcondi_tl(TCG_COND_EQ, bit, 0, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(bit);
    tcg_temp_free(msk);
    tcg_temp_free(_c);

    return DISAS_NORETURN;
}


/*
 * BL
 * --- code ---
 * target = cpl + offset
 *
 * save_addr = ctx->npc
 * if (ctx->insn.d)
 *   save_addr = ctx->npc + insn_len(ctx->npc)
 *
 * if (cc_flag == true)
 * {
 *   blink = save_addr
 *   gen_branchi(target)
 * }
 */
int
arc_gen_BL(DisasCtxt *ctx, TCGv offset ATTRIBUTE_UNUSED)
{
    target_ulong target;
    target_ulong save_addr = ctx->npc;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    /*
     * The decoder provides inputs in two places:
     *   .operand[0].value
     *   .limm
     * The limm version has an implicit lower bits of 00,
     * which makes the final offset 4-byte aligned.
     */
    if (ctx->insn.signed_limm_p) {
	    target = ctx->pcl + (ctx->insn.limm << 2);
    } else {
	    target = ctx->pcl + ctx->insn.operands[0].value;
    }

    /*
     * According to hardware team, fetching the delay slot, if any, happens
     * irrespective of the CC flag.
     */
    if (ctx->insn.d) {
        const ARCCPU *cpu = env_archcpu(ctx->env);
        uint16_t ds_insn;
        uint8_t ds_len;

        /* Save the PC, in case cpu_lduw_code() rasises an exception. */
        updatei_pcs(ctx->cpc);
        ds_insn = (uint16_t) cpu_lduw_code(ctx->env, ctx->npc);
        ds_len = arc_insn_length(ds_insn, cpu->family);
        save_addr = ctx->npc + ds_len;
    }

    arc_gen_verifyCCFlag(ctx, cond);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cond, 0, do_not_branch);

    tcg_gen_movi_tl(cpu_blink, save_addr);
    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/*
 * J
 * --- code ---
 * if (cc_flag == true)
 *   gen_branch(target)
 */
int
arc_gen_J(DisasCtxt *ctx, TCGv target)
{
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    arc_gen_verifyCCFlag(ctx, cond);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cond, 0, do_not_branch);

    gen_branch(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/*
 * JL
 * --- code ---
 * save_addr = ctx->npc
 * if (ctx->insn.d)
 *   save_addr = ctx->npc + insn_len(ctx->npc)
 *
 * if (cc_flag == true)
 * {
 *   _target = target
 *   blink = save_addr
 *   gen_branch(_target)
 * }
 */
int
arc_gen_JL(DisasCtxt *ctx, TCGv target)
{
    target_ulong save_addr = ctx->npc;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv _target = tcg_temp_new();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    /*
     * According to hardware team, fetching the delay slot, if any, happens
     * irrespective of the CC flag.
     */
    if (ctx->insn.d) {
        const ARCCPU *cpu = env_archcpu(ctx->env);
        uint16_t ds_insn;
        uint8_t ds_len;

        /* Save the PC, in case cpu_lduw_code() rasises an exception. */
        updatei_pcs(ctx->cpc);
        ds_insn = (uint16_t) cpu_lduw_code(ctx->env, ctx->npc);
        ds_len = arc_insn_length(ds_insn, cpu->family);
        save_addr = ctx->npc + ds_len;
    }

    arc_gen_verifyCCFlag(ctx, cond);
    tcg_gen_brcondi_tl(TCG_COND_EQ, cond, 0, do_not_branch);

    /*
     * In case blink and target registers are the same, i.e.:
     *   jl   [blink]
     * We have to use the blink's value before it is overwritten.
     */
    tcg_gen_mov_tl(_target, target);
    tcg_gen_movi_tl(cpu_blink, save_addr);
    gen_branch(ctx, _target);

    gen_set_label(do_not_branch);

    tcg_temp_free(cond);
    tcg_temp_free(_target);

    return DISAS_NORETURN;
}


/* SETEQ
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = (@b & 4294967295);
      p_c = (@c & 4294967295);
      p_b = se32to64 (p_b);
      p_c = se32to64 (p_c);
      take_branch = false;
      if((p_b == p_c))
        {
        }
      else
        {
        };
      if((p_b == p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    };
}
 */

int
arc_gen_SETEQ (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_7 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv take_branch = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_7);
  tcg_gen_mov_tl(cc_flag, temp_7);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(p_b, b, 4294967295);
  tcg_gen_andi_tl(p_c, c, 4294967295);
  se32to64(temp_8, p_b);
  tcg_gen_mov_tl(p_b, temp_8);
  se32to64(temp_9, p_c);
  tcg_gen_mov_tl(p_c, temp_9);
  tcg_gen_mov_tl(take_branch, arc_false);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_3, p_b, p_c);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  TCGLabel *else_3 = gen_new_label();
  TCGLabel *done_3 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_5, p_b, p_c);
  tcg_gen_xori_tl(temp_6, temp_5, 1); tcg_gen_andi_tl(temp_6, temp_6, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_3);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_3);
  gen_set_label(else_3);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_3);
  gen_set_label(done_1);
  tcg_temp_free(temp_7);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(p_b);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_9);
  tcg_temp_free(take_branch);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);

  return ret;
}


/*
 * BREQ
 * --- code ---
 * target = cpl + offset
 * b32 = b & 0xFFFF_FFFF;
 * c32 = c & 0xFFFF_FFFF;
 *
 * if (b32 == c32)
 *   gen_branchi(target)
 */
int
arc_gen_BREQ(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv b32 = tcg_temp_new();
    TCGv c32 = tcg_temp_new();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_andi_tl(b32, b, 0xffffffff);
    tcg_gen_andi_tl(c32, c, 0xffffffff);
    tcg_gen_brcond_tl(TCG_COND_NE, b32, c32, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);

    tcg_temp_free(cond);
    tcg_temp_free(c32);
    tcg_temp_free(b32);

    return DISAS_NORETURN;
}


/* SETNE
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = (@b & 4294967295);
      p_c = (@c & 4294967295);
      p_b = se32to64 (p_b);
      p_c = se32to64 (p_c);
      take_branch = false;
      if((p_b != p_c))
        {
        }
      else
        {
        };
      if((p_b != p_c))
        {
	  @a = 0;
          @a = true;
        }
      else
        {
          @a = false;
        };
    };
}
 */

int
arc_gen_SETNE (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_7 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv take_branch = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_7);
  tcg_gen_mov_tl(cc_flag, temp_7);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(p_b, b, 4294967295);
  tcg_gen_andi_tl(p_c, c, 4294967295);
  se32to64(temp_8, p_b);
  tcg_gen_mov_tl(p_b, temp_8);
  se32to64(temp_9, p_c);
  tcg_gen_mov_tl(p_c, temp_9);
  tcg_gen_mov_tl(take_branch, arc_false);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_NE, temp_3, p_b, p_c);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  TCGLabel *else_3 = gen_new_label();
  TCGLabel *done_3 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_NE, temp_5, p_b, p_c);
  tcg_gen_xori_tl(temp_6, temp_5, 1); tcg_gen_andi_tl(temp_6, temp_6, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_3);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_3);
  gen_set_label(else_3);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_3);
  gen_set_label(done_1);
  tcg_temp_free(temp_7);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(p_b);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_9);
  tcg_temp_free(take_branch);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);

  return ret;
}


/*
 * BRNE
 * --- code ---
 * target = cpl + offset
 * b32 = b & 0xFFFF_FFFF;
 * c32 = c & 0xFFFF_FFFF;
 *
 * if (b32 != c32)
 *   gen_branchi(target)
 */
int
arc_gen_BRNE(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv b32 = tcg_temp_new();
    TCGv c32 = tcg_temp_new();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_andi_tl(b32, b, 0xffffffff);
    tcg_gen_andi_tl(c32, c, 0xffffffff);
    tcg_gen_brcond_tl(TCG_COND_EQ, b32, c32, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);

    tcg_temp_free(cond);
    tcg_temp_free(c32);
    tcg_temp_free(b32);

    return DISAS_NORETURN;
}


/* SETLT
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = (@b & 4294967295);
      p_c = (@c & 4294967295);
      p_b = se32to64 (p_b);
      p_c = se32to64 (p_c);
      take_branch = false;
      if((p_b < p_c))
        {
        }
      else
        {
        };
      if((p_b < p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    };
}
 */

int
arc_gen_SETLT (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_7 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv take_branch = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_7);
  tcg_gen_mov_tl(cc_flag, temp_7);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(p_b, b, 4294967295);
  tcg_gen_andi_tl(p_c, c, 4294967295);
  se32to64(temp_8, p_b);
  tcg_gen_mov_tl(p_b, temp_8);
  se32to64(temp_9, p_c);
  tcg_gen_mov_tl(p_c, temp_9);
  tcg_gen_mov_tl(take_branch, arc_false);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_LT, temp_3, p_b, p_c);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  TCGLabel *else_3 = gen_new_label();
  TCGLabel *done_3 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_LT, temp_5, p_b, p_c);
  tcg_gen_xori_tl(temp_6, temp_5, 1); tcg_gen_andi_tl(temp_6, temp_6, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_3);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_3);
  gen_set_label(else_3);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_3);
  gen_set_label(done_1);
  tcg_temp_free(temp_7);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(p_b);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_9);
  tcg_temp_free(take_branch);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);

  return ret;
}


/*
 * BRLT
 * --- code ---
 * target = cpl + offset
 * b32 = (b << 32) s>> 32;
 * c32 = (b << 32) s>> 32;
 *
 * if (b32 s< c32)
 *   gen_branchi(target)
 * }
 */
int
arc_gen_BRLT(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv b32 = tcg_temp_new();
    TCGv c32 = tcg_temp_new();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_ext32s_tl(b32, b);
    tcg_gen_ext32s_tl(c32, c);
    tcg_gen_brcond_tl(TCG_COND_GE, b32, c32, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);
    tcg_temp_free(c32);
    tcg_temp_free(b32);

    return DISAS_NORETURN;
}


/* SETGE
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = (@b & 4294967295);
      p_c = (@c & 4294967295);
      p_b = se32to64 (p_b);
      p_c = se32to64 (p_c);
      take_branch = false;
      if((p_b >= p_c))
        {
        }
      else
        {
        };
      if((p_b >= p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    };
}
 */

int
arc_gen_SETGE (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_7 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv take_branch = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_7);
  tcg_gen_mov_tl(cc_flag, temp_7);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(p_b, b, 4294967295);
  tcg_gen_andi_tl(p_c, c, 4294967295);
  se32to64(temp_8, p_b);
  tcg_gen_mov_tl(p_b, temp_8);
  se32to64(temp_9, p_c);
  tcg_gen_mov_tl(p_c, temp_9);
  tcg_gen_mov_tl(take_branch, arc_false);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_GE, temp_3, p_b, p_c);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  TCGLabel *else_3 = gen_new_label();
  TCGLabel *done_3 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_GE, temp_5, p_b, p_c);
  tcg_gen_xori_tl(temp_6, temp_5, 1); tcg_gen_andi_tl(temp_6, temp_6, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_3);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_3);
  gen_set_label(else_3);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_3);
  gen_set_label(done_1);
  tcg_temp_free(temp_7);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(p_b);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_9);
  tcg_temp_free(take_branch);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);

  return ret;
}


/*
 * BRGE
 * --- code ---
 * target = cpl + offset
 * b32 = (b << 32) s>> 32;
 * c32 = (c << 32) s>> 32;
 *
 * if (b32 s>= c32)
 *   gen_branchi(target)
 */
int
arc_gen_BRGE(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv b32 = tcg_temp_new();
    TCGv c32 = tcg_temp_new();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_ext32s_tl(b32, b);
    tcg_gen_ext32s_tl(c32, c);
    tcg_gen_brcond_tl(TCG_COND_LT, b32, c32, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);

    tcg_temp_free(cond);
    tcg_temp_free(c32);
    tcg_temp_free(b32);

    return DISAS_NORETURN;
}


/* SETLE
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = (@b & 4294967295);
      p_c = (@c & 4294967295);
      p_b = se32to64 (p_b);
      p_c = se32to64 (p_c);
      take_branch = false;
      if((p_b <= p_c))
        {
        }
      else
        {
        };
      if((p_b <= p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    };
}
 */

int
arc_gen_SETLE (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_7 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv take_branch = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_7);
  tcg_gen_mov_tl(cc_flag, temp_7);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(p_b, b, 4294967295);
  tcg_gen_andi_tl(p_c, c, 4294967295);
  se32to64(temp_8, p_b);
  tcg_gen_mov_tl(p_b, temp_8);
  se32to64(temp_9, p_c);
  tcg_gen_mov_tl(p_c, temp_9);
  tcg_gen_mov_tl(take_branch, arc_false);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_LE, temp_3, p_b, p_c);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  TCGLabel *else_3 = gen_new_label();
  TCGLabel *done_3 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_LE, temp_5, p_b, p_c);
  tcg_gen_xori_tl(temp_6, temp_5, 1); tcg_gen_andi_tl(temp_6, temp_6, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_3);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_3);
  gen_set_label(else_3);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_3);
  gen_set_label(done_1);
  tcg_temp_free(temp_7);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(p_b);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_9);
  tcg_temp_free(take_branch);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);

  return ret;
}





/* SETGT
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, se32to64
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = (@b & 4294967295);
      p_c = (@c & 4294967295);
      p_b = se32to64 (p_b);
      p_c = se32to64 (p_c);
      take_branch = false;
      if((p_b > p_c))
        {
        }
      else
        {
        };
      if((p_b > p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    };
}
 */

int
arc_gen_SETGT (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_7 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv take_branch = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_7);
  tcg_gen_mov_tl(cc_flag, temp_7);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(p_b, b, 4294967295);
  tcg_gen_andi_tl(p_c, c, 4294967295);
  se32to64(temp_8, p_b);
  tcg_gen_mov_tl(p_b, temp_8);
  se32to64(temp_9, p_c);
  tcg_gen_mov_tl(p_c, temp_9);
  tcg_gen_mov_tl(take_branch, arc_false);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_GT, temp_3, p_b, p_c);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  TCGLabel *else_3 = gen_new_label();
  TCGLabel *done_3 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_GT, temp_5, p_b, p_c);
  tcg_gen_xori_tl(temp_6, temp_5, 1); tcg_gen_andi_tl(temp_6, temp_6, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_6, arc_true, else_3);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_3);
  gen_set_label(else_3);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_3);
  gen_set_label(done_1);
  tcg_temp_free(temp_7);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(p_b);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_9);
  tcg_temp_free(take_branch);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);

  return ret;
}

/*
 * BRLO
 * --- code ---
 * target = cpl + offset
 * b32 = b & 0xFFFF_FFFF
 * c32 = c & 0xFFFF_FFFF
 *
 * if (b32 u< c32)
 *   gen_branchi(target)
 */
int
arc_gen_BRLO(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv b32 = tcg_temp_new();
    TCGv c32 = tcg_temp_new();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_andi_tl(b32, b, 0xffffffff);
    tcg_gen_andi_tl(c32, c, 0xffffffff);
    tcg_gen_brcond_tl(TCG_COND_GEU, b32, c32, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);

    tcg_temp_free(cond);
    tcg_temp_free(c32);
    tcg_temp_free(b32);

    return DISAS_NORETURN;
}


/* SETLO
 *    Variables: @b, @c, @a
 *    Functions: se32to64, unsignedLT
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = se32to64 (@b);
      p_c = se32to64 (@c);
      take_branch = false;
      if(unsignedLT (p_b, p_c))
        {
        }
      else
        {
        };
      if(unsignedLT (p_b, p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    }
}
 */

int
arc_gen_SETLO (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv take_branch = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv cc_temp_1 = tcg_temp_local_new();
  getCCFlag(cc_flag);
  TCGLabel *done_cc = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, cc_temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(cc_temp_1, cc_temp_1, 1); tcg_gen_andi_tl(cc_temp_1, cc_temp_1, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, cc_temp_1, arc_true, done_cc);;
  se32to64(temp_3, b);
  tcg_gen_mov_tl(p_b, temp_3);
  se32to64(temp_4, c);
  tcg_gen_mov_tl(p_c, temp_4);
  tcg_gen_mov_tl(take_branch, arc_false);
  TCGLabel *else_1 = gen_new_label();
  TCGLabel *done_1 = gen_new_label();
  unsignedLT(temp_5, p_b, p_c);
  tcg_gen_xori_tl(temp_1, temp_5, 1); tcg_gen_andi_tl(temp_1, temp_1, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_1, arc_true, else_1);;
  tcg_gen_br(done_1);
  gen_set_label(else_1);
  gen_set_label(done_1);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  unsignedLT(temp_6, p_b, p_c);
  tcg_gen_xori_tl(temp_2, temp_6, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_2);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_2);
  gen_set_label(done_cc);
  tcg_temp_free(temp_3);
  tcg_temp_free(p_b);
  tcg_temp_free(temp_4);
  tcg_temp_free(p_c);
  tcg_temp_free(take_branch);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_2);
  tcg_temp_free(cc_temp_1);
  tcg_temp_free(cc_flag);

  return ret;
}


/*
 * BRHS
 * --- code ---
 * target = cpl + offset
 * b32 = b & 0xFFFF_FFFF
 * c32 = c & 0xFFFF_FFFF
 *
 * if (b32 u>= c32)
 *   gen_branchi(target)
 */
int
arc_gen_BRHS(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv b32 = tcg_temp_new();
    TCGv c32 = tcg_temp_new();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_andi_tl(b32, b, 0xffffffff);
    tcg_gen_andi_tl(c32, c, 0xffffffff);
    tcg_gen_brcond_tl(TCG_COND_LTU, b32, c32, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);

    tcg_temp_free(cond);
    tcg_temp_free(c32);
    tcg_temp_free(b32);

    return DISAS_NORETURN;
}


/* SETHS
 *    Variables: @b, @c, @a
 *    Functions: se32to64, unsignedGE
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = se32to64 (@b);
      p_c = se32to64 (@c);
      take_branch = false;
      if(unsignedGE (p_b, p_c))
        {
        }
      else
        {
        };
      if(unsignedGE (p_b, p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    }
}
 */

int
arc_gen_SETHS (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv take_branch = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv cc_temp_1 = tcg_temp_local_new();
  getCCFlag(cc_flag);
  TCGLabel *done_cc = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, cc_temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(cc_temp_1, cc_temp_1, 1); tcg_gen_andi_tl(cc_temp_1, cc_temp_1, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, cc_temp_1, arc_true, done_cc);;
  se32to64(temp_3, b);
  tcg_gen_mov_tl(p_b, temp_3);
  se32to64(temp_4, c);
  tcg_gen_mov_tl(p_c, temp_4);
  tcg_gen_mov_tl(take_branch, arc_false);
  TCGLabel *else_1 = gen_new_label();
  TCGLabel *done_1 = gen_new_label();
  unsignedGE(temp_5, p_b, p_c);
  tcg_gen_xori_tl(temp_1, temp_5, 1); tcg_gen_andi_tl(temp_1, temp_1, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_1, arc_true, else_1);;
  tcg_gen_br(done_1);
  gen_set_label(else_1);
  gen_set_label(done_1);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  unsignedGE(temp_6, p_b, p_c);
  tcg_gen_xori_tl(temp_2, temp_6, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_2);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_2);
  gen_set_label(done_cc);
  tcg_temp_free(temp_3);
  tcg_temp_free(p_b);
  tcg_temp_free(temp_4);
  tcg_temp_free(p_c);
  tcg_temp_free(take_branch);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_2);
  tcg_temp_free(cc_temp_1);
  tcg_temp_free(cc_flag);

  return ret;
}





/*
 * EX - CODED BY HAND
 */

int
arc_gen_EX (DisasCtxt *ctx, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp = tcg_temp_local_new();
  tcg_gen_mov_tl(temp, b);
  tcg_gen_atomic_xchg_tl(b, c, temp, ctx->mem_idx, MO_UL);
  tcg_temp_free(temp);

  return ret;
}


#undef ARM_LIKE_LLOCK_SCOND

extern TCGv cpu_exclusive_addr;
extern TCGv cpu_exclusive_val;

/*
 * LLOCK -- CODED BY HAND
 */

int
arc_gen_LLOCK(DisasCtxt *ctx, TCGv dest, TCGv src)
{
    int ret = DISAS_NEXT;
#ifndef ARM_LIKE_LLOCK_SCOND
    gen_helper_llock(dest, cpu_env, src);
#else
    tcg_gen_qemu_ld_tl(cpu_exclusive_val, src, ctx->mem_idx, MO_UL);
    tcg_gen_mov_tl(dest, cpu_exclusive_val);
    tcg_gen_mov_tl(cpu_exclusive_addr, src);
#endif

    return ret;
}

/*
 * LLOCKL -- CODED BY HAND
 */

int
arc_gen_LLOCKL(DisasCtxt *ctx, TCGv dest, TCGv src)
{
    int ret = DISAS_NEXT;
#ifndef ARM_LIKE_LLOCK_SCOND
    gen_helper_llockl(dest, cpu_env, src);
#else
    tcg_gen_qemu_ld_tl(cpu_exclusive_val, src, ctx->mem_idx, MO_UQ);
    tcg_gen_mov_tl(dest, cpu_exclusive_val);
    tcg_gen_mov_tl(cpu_exclusive_addr, src);
#endif

    return ret;
}


/*
 * SCOND -- CODED BY HAND
 */

int
arc_gen_SCOND(DisasCtxt *ctx, TCGv addr, TCGv value)
{
    int ret = DISAS_NEXT;
#ifndef ARM_LIKE_LLOCK_SCOND
    TCGv temp_4 = tcg_temp_local_new();
    gen_helper_scond(temp_4, cpu_env, addr, value);
    setZFlag(temp_4);
    tcg_temp_free(temp_4);
#else
    TCGLabel *fail_label = gen_new_label();
    TCGLabel *done_label = gen_new_label();
    TCGv tmp;

    tcg_gen_brcond_tl(TCG_COND_NE, addr, cpu_exclusive_addr, fail_label);
    tmp = tcg_temp_new();

    tcg_gen_atomic_cmpxchg_tl(tmp, cpu_exclusive_addr, cpu_exclusive_val,
                               value, ctx->mem_idx,
                               MO_UL | MO_ALIGN);
    tcg_gen_setcond_tl(TCG_COND_NE, tmp, tmp, cpu_exclusive_val);

    setZFlag(tmp);

    tcg_temp_free(tmp);
    tcg_gen_br(done_label);

    gen_set_label(fail_label);
    tcg_gen_movi_tl(cpu_Zf, 1);
    gen_set_label(done_label);
    tcg_gen_movi_tl(cpu_exclusive_addr, -1);
#endif

    return ret;
}


/*
 * SCONDL -- CODED BY HAND
 */

int
arc_gen_SCONDL(DisasCtxt *ctx, TCGv addr, TCGv value)
{
    int ret = DISAS_NEXT;
#ifndef ARM_LIKE_LLOCK_SCOND
    TCGv temp_4 = tcg_temp_local_new();
    gen_helper_scondl(temp_4, cpu_env, addr, value);
    setZFlag(temp_4);
    tcg_temp_free(temp_4);
#else
    TCGLabel *fail_label = gen_new_label();
    TCGLabel *done_label = gen_new_label();
    TCGv tmp;

    tcg_gen_brcond_tl(TCG_COND_NE, addr, cpu_exclusive_addr, fail_label);
    tmp = tcg_temp_new();

    tcg_gen_atomic_cmpxchg_tl(tmp, cpu_exclusive_addr, cpu_exclusive_val,
                               value, ctx->mem_idx,
                               MO_Q | MO_ALIGN);
    tcg_gen_setcond_tl(TCG_COND_NE, tmp, tmp, cpu_exclusive_val);

    setZFlag(tmp);

    tcg_temp_free(tmp);
    tcg_gen_br(done_label);

    gen_set_label(fail_label);
    tcg_gen_movi_tl(cpu_Zf, 1);
    gen_set_label(done_label);
    tcg_gen_movi_tl(cpu_exclusive_addr, -1);
#endif

    return ret;
}





/* DMB - HAND MADE
 */

int
arc_gen_DMB (DisasCtxt *ctx, TCGv a)
{
  int ret = DISAS_NEXT;

  TCGBar bar = 0;
  switch(ctx->insn.operands[0].value & 7) {
    case 1:
      bar |= TCG_BAR_SC | TCG_MO_LD_LD | TCG_MO_LD_ST;
      break;
    case 2:
      bar |= TCG_BAR_SC | TCG_MO_ST_ST;
      break;
    default:
      bar |= TCG_BAR_SC | TCG_MO_ALL;
      break;
  }
  tcg_gen_mb(bar);

  return ret;
}





/* LD
 *    Variables: @src1, @src2, @dest
 *    Functions: getAAFlag, getZZFlag, setDebugLD, getMemory, getFlagX, SignExtend, NoFurtherLoadsPending
--- code ---
{
  AA = getAAFlag ();
  ZZ = getZZFlag ();
  address = 0;
  if(((AA == 0) || (AA == 1)))
    {
      address = (@src1 + @src2);
    };
  if((AA == 2))
    {
      address = @src1;
    };
  if(((AA == 3) && ((ZZ == 0) || (ZZ == 3))))
    {
      address = (@src1 + (@src2 << 2));
    };
  if(((AA == 3) && (ZZ == 2)))
    {
      address = (@src1 + (@src2 << 1));
    };
  l_src1 = @src1;
  l_src2 = @src2;
  setDebugLD (1);
  new_dest = getMemory (address, ZZ);
  if(((AA == 1) || (AA == 2)))
    {
      @src1 = (l_src1 + l_src2);
    };
  if((getFlagX () == 1))
    {
      new_dest = SignExtend (new_dest, ZZ);
    };
  if(NoFurtherLoadsPending ())
    {
      setDebugLD (0);
    };
  @dest = new_dest;
}
 */

int
arc_gen_LD (DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
  int ret = DISAS_NEXT;
  int AA;
  int ZZ;
  TCGv address = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv l_src1 = tcg_temp_local_new();
  TCGv l_src2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv new_dest = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  AA = getAAFlag ();
  ZZ = getZZFlag ();
  tcg_gen_movi_tl(address, 0);
  if (((AA == 0) || (AA == 1)))
    {
      ARC64_ADDRESS_ADD(address, src1, src2);
      //tcg_gen_add_tl(address, src1, src2);
    }
  if ((AA == 2))
    {
      tcg_gen_mov_tl(address, src1);
    }
  if (((AA == 3) && ((ZZ == 0) || (ZZ == 3))))
    {
      tcg_gen_shli_tl(temp_2, src2, 2);
      ARC64_ADDRESS_ADD(address, src1, temp_2);
      //tcg_gen_add_tl(address, src1, temp_2);
    }
  if (((AA == 3) && (ZZ == 2)))
    {
      tcg_gen_shli_tl(temp_3, src2, 1);
      ARC64_ADDRESS_ADD(address, src1, temp_3);
      //tcg_gen_add_tl(address, src1, temp_3);
    }
  tcg_gen_mov_tl(l_src1, src1);
  tcg_gen_mov_tl(l_src2, src2);
  tcg_gen_movi_tl(temp_4, 1);
  setDebugLD(temp_4);
  getMemory(temp_5, address, ZZ);
  tcg_gen_mov_tl(new_dest, temp_5);
  if (((AA == 1) || (AA == 2)))
    {
      //tcg_gen_add_tl(src1, l_src1, l_src2);
      ARC64_ADDRESS_ADD(src1, l_src1, l_src2);
    }
  if ((getFlagX () == 1))
    {
      new_dest = SignExtend (new_dest, ZZ);
    }
  TCGLabel *done_1 = gen_new_label();
  NoFurtherLoadsPending(temp_6);
  tcg_gen_xori_tl(temp_1, temp_6, 1); tcg_gen_andi_tl(temp_1, temp_1, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_1, arc_true, done_1);;
  tcg_gen_movi_tl(temp_7, 0);
  setDebugLD(temp_7);
  gen_set_label(done_1);
  tcg_gen_mov_tl(dest, new_dest);
  tcg_temp_free(address);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_3);
  tcg_temp_free(l_src1);
  tcg_temp_free(l_src2);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(new_dest);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_7);

  return ret;
}





/* LDD
 *    Variables: @src1, @src2, @dest
 *    Functions: getAAFlag, getZZFlag, setDebugLD, getMemory, nextReg, NoFurtherLoadsPending
--- code ---
{
  AA = getAAFlag ();
  ZZ = getZZFlag ();
  address = 0;
  if(((AA == 0) || (AA == 1)))
    {
      address = (@src1 + @src2);
    };
  if((AA == 2))
    {
      address = @src1;
    };
  if(((AA == 3) && ((ZZ == 0) || (ZZ == 3))))
    {
      address = (@src1 + (@src2 << 2));
    };
  if(((AA == 3) && (ZZ == 2)))
    {
      address = (@src1 + (@src2 << 1));
    };
  l_src1 = @src1;
  l_src2 = @src2;
  setDebugLD (1);
  new_dest = getMemory (address, LONG);
  pair = nextReg (dest);
  pair = getMemory ((address + 4), LONG);
  if(((AA == 1) || (AA == 2)))
    {
      @src1 = (l_src1 + l_src2);
    };
  if(NoFurtherLoadsPending ())
    {
      setDebugLD (0);
    };
  @dest = new_dest;
}
 */

int
arc_gen_LDD (DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
  int ret = DISAS_NEXT;
  int AA;
  int ZZ;
  TCGv address = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv l_src1 = tcg_temp_local_new();
  TCGv l_src2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv new_dest = tcg_temp_local_new();
  TCGv pair = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  AA = getAAFlag ();
  ZZ = getZZFlag ();
  tcg_gen_movi_tl(address, 0);
  if (((AA == 0) || (AA == 1)))
    {
      ARC64_ADDRESS_ADD(address, src1, src2);
    }
  if ((AA == 2))
    {
    tcg_gen_mov_tl(address, src1);
    }
  if (((AA == 3) && ((ZZ == 0) || (ZZ == 3))))
    {
      tcg_gen_shli_tl(temp_2, src2, 2);
      ARC64_ADDRESS_ADD(address, src1, temp_2);
    }
  if (((AA == 3) && (ZZ == 2)))
    {
      tcg_gen_shli_tl(temp_3, src2, 1);
      ARC64_ADDRESS_ADD(address, src1, temp_3);
    }
  tcg_gen_mov_tl(l_src1, src1);
  tcg_gen_mov_tl(l_src2, src2);
  tcg_gen_movi_tl(temp_4, 1);
  setDebugLD(temp_4);
  getMemory(temp_5, address, LONG);
  tcg_gen_mov_tl(new_dest, temp_5);
  pair = nextReg (dest);
  tcg_gen_addi_tl(temp_7, address, 4);
  getMemory(temp_6, temp_7, LONG);
  tcg_gen_mov_tl(pair, temp_6);
  if (((AA == 1) || (AA == 2)))
    {
      ARC64_ADDRESS_ADD(src1, l_src1, l_src2);
    }
  TCGLabel *done_1 = gen_new_label();
  NoFurtherLoadsPending(temp_8);
  tcg_gen_xori_tl(temp_1, temp_8, 1); tcg_gen_andi_tl(temp_1, temp_1, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_1, arc_true, done_1);;
  tcg_gen_movi_tl(temp_9, 0);
  setDebugLD(temp_9);
  gen_set_label(done_1);
  tcg_gen_mov_tl(dest, new_dest);
  tcg_temp_free(address);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_3);
  tcg_temp_free(l_src1);
  tcg_temp_free(l_src2);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(new_dest);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_9);

  return ret;
}





/* ST
 *    Variables: @src1, @src2, @dest
 *    Functions: getAAFlag, getZZFlag, setMemory
--- code ---
{
  AA = getAAFlag ();
  ZZ = getZZFlag ();
  address = 0;
  if(((AA == 0) || (AA == 1)))
    {
      address = (@src1 + @src2);
    };
  if((AA == 2))
    {
      address = @src1;
    };
  if(((AA == 3) && ((ZZ == 0) || (ZZ == 3))))
    {
      address = (@src1 + (@src2 << 2));
    };
  if(((AA == 3) && (ZZ == 2)))
    {
      address = (@src1 + (@src2 << 1));
    };
  setMemory (address, ZZ, @dest);
  if(((AA == 1) || (AA == 2)))
    {
      @src1 = (@src1 + @src2);
    };
}
 */

int
arc_gen_ST (DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
  int ret = DISAS_NEXT;
  int AA;
  int ZZ;
  TCGv address = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  AA = getAAFlag ();
  ZZ = getZZFlag ();
  tcg_gen_movi_tl(address, 0);
  if (((AA == 0) || (AA == 1)))
    {
      ARC64_ADDRESS_ADD(address, src1, src2);
    }
  if ((AA == 2))
    {
      tcg_gen_mov_tl(address, src1);
    }
  if (((AA == 3) && ((ZZ == 0) || (ZZ == 3))))
    {
      tcg_gen_shli_tl(temp_1, src2, 2);
      ARC64_ADDRESS_ADD(address, src1, temp_1);
    }
  if (((AA == 3) && (ZZ == 2)))
    {
      tcg_gen_shli_tl(temp_2, src2, 1);
      ARC64_ADDRESS_ADD(address, src1, temp_2);
    }
  setMemory(address, ZZ, dest);
  if (((AA == 1) || (AA == 2)))
    {
      ARC64_ADDRESS_ADD(src1, src1, src2);
    }
  tcg_temp_free(address);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);

  return ret;
}





/* STD
 *    Variables: @src1, @src2, @dest
 *    Functions: getAAFlag, getZZFlag, setMemory, instructionHasRegisterOperandIn, nextReg, getBit
--- code ---
{
  AA = getAAFlag ();
  ZZ = getZZFlag ();
  address = 0;
  if(((AA == 0) || (AA == 1)))
    {
      address = (@src1 + @src2);
    };
  if((AA == 2))
    {
      address = @src1;
    };
  if(((AA == 3) && ((ZZ == 0) || (ZZ == 3))))
    {
      address = (@src1 + (@src2 << 2));
    };
  if(((AA == 3) && (ZZ == 2)))
    {
      address = (@src1 + (@src2 << 1));
    };
  setMemory (address, LONG, @dest);
  if(instructionHasRegisterOperandIn (0))
    {
      pair = nextReg (dest);
    }
  else
    {
      if((getBit (@dest, 31) == 1))
        {
          pair = 4294967295;
        }
      else
        {
          pair = 0;
        };
    };
  setMemory ((address + 4), LONG, pair);
  if(((AA == 1) || (AA == 2)))
    {
      @src1 = (@src1 + @src2);
    };
}
 */

int
arc_gen_STD (DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
  int ret = DISAS_NEXT;
  int AA;
  int ZZ;
  TCGv address = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv pair = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  AA = getAAFlag ();
  ZZ = getZZFlag ();
  tcg_gen_movi_tl(address, 0);
  if (((AA == 0) || (AA == 1)))
    {
      ARC64_ADDRESS_ADD(address, src1, src2);
    }
  if ((AA == 2))
    {
      tcg_gen_mov_tl(address, src1);
    }
  if (((AA == 3) && ((ZZ == 0) || (ZZ == 3))))
    {
      tcg_gen_shli_tl(temp_3, src2, 2);
      ARC64_ADDRESS_ADD(address, src1, temp_3);
    }
  if (((AA == 3) && (ZZ == 2)))
    {
      tcg_gen_shli_tl(temp_4, src2, 1);
      ARC64_ADDRESS_ADD(address, src1, temp_4);
    }
  setMemory(address, LONG, dest);
  if (instructionHasRegisterOperandIn (0))
    {
    pair = nextReg (dest);
    }
  else
    {
    TCGLabel *else_1 = gen_new_label();
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_movi_tl(temp_6, 31);
  getBit(temp_5, dest, temp_6);
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_1, temp_5, 1);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_1);;
  tcg_gen_movi_tl(pair, 4294967295);
  tcg_gen_br(done_1);
  gen_set_label(else_1);
  tcg_gen_movi_tl(pair, 0);
  gen_set_label(done_1);
;
    }
  tcg_gen_addi_tl(temp_7, address, 4);
  setMemory(temp_7, LONG, pair);
  if (((AA == 1) || (AA == 2)))
    {
      ARC64_ADDRESS_ADD(src1, src1, src2);
    }
  tcg_temp_free(address);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_7);

  return ret;
}





/* POP
 *    Variables: @dest
 *    Functions: getMemory, getRegister, setRegister
--- code ---
{
  new_dest = getMemory (getRegister (R_SP), LONG);
  setRegister (R_SP, (getRegister (R_SP) + 4));
  @dest = new_dest;
}
 */

int
arc_gen_POP (DisasCtxt *ctx, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv new_dest = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  getRegister(temp_3, R_SP);
  tcg_gen_mov_tl(temp_2, temp_3);
  getMemory(temp_1, temp_2, LONG);
  tcg_gen_mov_tl(new_dest, temp_1);
  getRegister(temp_6, R_SP);
  tcg_gen_mov_tl(temp_5, temp_6);
  tcg_gen_addi_tl(temp_4, temp_5, 4);
  setRegister(R_SP, temp_4);
  tcg_gen_mov_tl(dest, new_dest);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_1);
  tcg_temp_free(new_dest);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* PUSH
 *    Variables: @src
 *    Functions: setMemory, getRegister, setRegister
--- code ---
{
  local_src = @src;
  setMemory ((getRegister (R_SP) - 4), LONG, local_src);
  setRegister (R_SP, (getRegister (R_SP) - 4));
}
 */

int
arc_gen_PUSH (DisasCtxt *ctx, TCGv src)
{
  int ret = DISAS_NEXT;
  TCGv local_src = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  tcg_gen_mov_tl(local_src, src);
  getRegister(temp_3, R_SP);
  tcg_gen_mov_tl(temp_2, temp_3);
  tcg_gen_subi_tl(temp_1, temp_2, 4);
  setMemory(temp_1, LONG, local_src);
  getRegister(temp_6, R_SP);
  tcg_gen_mov_tl(temp_5, temp_6);
  tcg_gen_subi_tl(temp_4, temp_5, 4);
  setRegister(R_SP, temp_4);
  tcg_temp_free(local_src);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* LP
 *    Variables: @rd
 *    Functions: getCCFlag, getRegIndex, writeAuxReg, nextInsnAddress, getPCL, setPC
--- code ---
{
  if((getCCFlag () == true))
    {
      lp_start_index = getRegIndex (LP_START);
      lp_end_index = getRegIndex (LP_END);
      writeAuxReg (lp_start_index, nextInsnAddress ());
      writeAuxReg (lp_end_index, (getPCL () + @rd));
    }
  else
    {
      setPC ((getPCL () + @rd));
    };
}
 */

int
arc_gen_LP (DisasCtxt *ctx, TCGv rd)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv lp_start_index = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv lp_end_index = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_13 = tcg_temp_local_new();
  TCGv temp_12 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  TCGLabel *else_1 = gen_new_label();
  TCGLabel *done_1 = gen_new_label();
  getCCFlag(temp_3);
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, temp_3, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_1);;
  getRegIndex(temp_4, LP_START);
  tcg_gen_mov_tl(lp_start_index, temp_4);
  getRegIndex(temp_5, LP_END);
  tcg_gen_mov_tl(lp_end_index, temp_5);
  nextInsnAddress(temp_7);
  tcg_gen_mov_tl(temp_6, temp_7);
  writeAuxReg(lp_start_index, temp_6);
  getPCL(temp_10);
  tcg_gen_mov_tl(temp_9, temp_10);
  tcg_gen_add_tl(temp_8, temp_9, rd);
  writeAuxReg(lp_end_index, temp_8);
  tcg_gen_br(done_1);
  gen_set_label(else_1);
  getPCL(temp_13);
  tcg_gen_mov_tl(temp_12, temp_13);
  tcg_gen_add_tl(temp_11, temp_12, rd);
  setPC(temp_11);
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);
  tcg_temp_free(lp_start_index);
  tcg_temp_free(temp_5);
  tcg_temp_free(lp_end_index);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_13);
  tcg_temp_free(temp_12);
  tcg_temp_free(temp_11);

  return ret;
}




/*
 * NORM
 *    Variables: @src, @dest
 *    Functions: CRLSB, getFFlag, setZFlag, setNFlag32
 * --- code ---
 * {
 *   psrc = @src;
 *   psrc = SignExtend16to32 (psrc);
 *   @dest = 32 - CRLSB (psrc);
 *   if((getFFlag () == true))
 *     {
 *       setZFlag (psrc);
 *       setNFlag32 (psrc);
 *     };
 * }
 */

int
arc_gen_NORM(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv psrc = tcg_temp_local_new();
    tcg_gen_mov_tl(psrc, src);
    tcg_gen_ext32s_tl(psrc, psrc);
    tcg_gen_clrsb_tl(dest, psrc);
    tcg_gen_subi_tl(dest, dest, 32);
    if ((getFFlag () == true)) {
        setZFlag(psrc);
        setNFlag32(psrc);
    }
    tcg_temp_free(psrc);

    return ret;
}


/*
 * NORMH
 *    Variables: @src, @dest
 *    Functions: SignExtend16to32, CRLSB, getFFlag, setZFlag, setNFlagByNum
 * --- code ---
 * {
 *   psrc = (@src & 65535);
 *   psrc = SignExtend16to32 (psrc);
 *   @dest = CRLSB (psrc);
 *   @dest = (@dest - 16);
 *   if((getFFlag () == true))
 *     {
 *       setZFlagByNum (psrc, 16);
 *       setNFlagByNum (psrc, 16);
 *     };
 * }
 */

int
arc_gen_NORMH(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv psrc = tcg_temp_local_new();
    tcg_gen_andi_tl(psrc, src, 65535);
    tcg_gen_ext16s_tl(psrc, psrc);
    tcg_gen_clrsb_tl(dest, psrc);
    tcg_gen_subi_tl(dest, dest, 16);
    if ((getFFlag () == true)) {
        setZFlagByNum(psrc, 16);
        setNFlagByNum(psrc, 16);
    }
    tcg_temp_free(psrc);

    return ret;
}


/*
 * FLS
 *    Variables: @src, @dest
 *    Functions: CLZ, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   psrc = @src & 0xffffffff;
 *   if((psrc == 0))
 *     {
 *       @dest = 0;
 *     }
 *   else
 *     {
 *       @dest = 63 - CLZ (psrc, 32);
 *     };
 *   if((getFFlag () == true))
 *     {
 *       setZFlag (psrc);
 *       setNFlag32 (psrc);
 *     };
 * }
 */

int
arc_gen_FLS(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv psrc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    tcg_gen_andi_tl(psrc, src, 0xffffffff);
    TCGLabel *else_1 = gen_new_label();
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_1, psrc, 0);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_1);
    tcg_gen_movi_tl(dest, 0);
    tcg_gen_br(done_1);
    gen_set_label(else_1);
    tcg_gen_movi_tl(temp_5, 32);
    tcg_gen_clz_tl(temp_4, psrc, temp_5);
    tcg_gen_mov_tl(temp_3, temp_4);
    tcg_gen_subfi_tl(dest, 63, temp_3);
    gen_set_label(done_1);
    if ((getFFlag () == true)) {
        setZFlag(psrc);
        setNFlag32(psrc);
    }
    tcg_temp_free(psrc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_3);

    return ret;
}





/*
 * FFS
 *    Variables: @src, @dest
 *    Functions: CTZ, getFFlag, setZFlag, setNFlag32
 * --- code ---
 * {
 *   psrc = @src & 0xffffffff;
 *   if((psrc == 0))
 *     {
 *       @dest = 31;
 *     }
 *   else
 *     {
 *       @dest = CTZ (psrc, 32);
 *     };
 *   if((getFFlag () == true))
 *     {
 *       setZFlag (psrc);
 *       setNFlag32 (psrc);
 *     };
 * }
 */

int
arc_gen_FFS(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv psrc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    tcg_gen_andi_tl(psrc, src, 0xffffffff);
    TCGLabel *else_1 = gen_new_label();
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_1, psrc, 0);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_1);
    tcg_gen_movi_tl(dest, 31);
    tcg_gen_br(done_1);
    gen_set_label(else_1);
    tcg_gen_movi_tl(temp_4, 32);
    tcg_gen_ctz_tl(temp_3, psrc, temp_4);
    tcg_gen_mov_tl(dest, temp_3);
    gen_set_label(done_1);
    if ((getFFlag () == true)) {
        setZFlag(psrc);
        setNFlag32(psrc);
    }
    tcg_temp_free(psrc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_3);

    return ret;
}



/*
 *  Long instructions
 */


/* ADDL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarryADD, setVFlag, OverflowADD
--- code ---
{
  cc_flag = getCCFlag ();
  lb = @b;
  lc = @c;
  if((cc_flag == true))
    {
      lb = @b;
      lc = @c;
      @a = (@b + @c);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag (@a);
          setCFlag (CarryADD (@a, lb, lc));
          setVFlag (OverflowADD (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_ADDL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  tcg_gen_mov_tl(lb, b);
  tcg_gen_mov_tl(lc, c);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(lb, b);
  tcg_gen_mov_tl(lc, c);
  tcg_gen_add_tl(a, b, c);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag(a);
  CarryADD(temp_5, a, lb, lc);
  tcg_gen_mov_tl(temp_4, temp_5);
  setCFlag(temp_4);
  OverflowADD(temp_7, a, lb, lc);
  tcg_gen_mov_tl(temp_6, temp_7);
  setVFlag(temp_6);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(lb);
  tcg_temp_free(lc);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);

  return ret;
}




/*
 * ADD1L
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarryADD,
 *               setVFlag, OverflowADD
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   lc = @c << 1;
 *   if((cc_flag == true))
 *     {
 *       @a = (@b + lc);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setCFlag (CarryADD (@a, lb, lc));
 *           setVFlag (OverflowADD (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_ADD1L(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_shli_tl(lc, c, 1);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_add_tl(a, b, lc);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        CarryADD(temp_5, a, lb, lc);
        tcg_gen_mov_tl(temp_4, temp_5);
        setCFlag(temp_4);
        OverflowADD(temp_7, a, lb, lc);
        tcg_gen_mov_tl(temp_6, temp_7);
        setVFlag(temp_6);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(lc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);

    return ret;
}
/*
 * ADD2L
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarryADD,
 *               setVFlag, OverflowADD
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   lc = @c << 2;
 *   if((cc_flag == true))
 *     {
 *       @a = (@b + lc);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setCFlag (CarryADD (@a, lb, lc));
 *           setVFlag (OverflowADD (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_ADD2L(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_shli_tl(lc, c, 2);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_add_tl(a, b, lc);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        CarryADD(temp_5, a, lb, lc);
        tcg_gen_mov_tl(temp_4, temp_5);
        setCFlag(temp_4);
        OverflowADD(temp_7, a, lb, lc);
        tcg_gen_mov_tl(temp_6, temp_7);
        setVFlag(temp_6);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(lc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);

    return ret;
}




/*
 * ADD3L
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarryADD,
 *               setVFlag, OverflowADD
 * --- code ---
 * {
 *   cc_flag = getCCFlag ();
 *   lb = @b;
 *   lc = @c << 3;
 *   if((cc_flag == true))
 *     {
 *       @a = (@b + lc);
 *       if((getFFlag () == true))
 *         {
 *           setZFlag (@a);
 *           setNFlag (@a);
 *           setCFlag (CarryADD (@a, lb, lc));
 *           setVFlag (OverflowADD (@a, lb, lc));
 *         };
 *     };
 * }
 */

int
arc_gen_ADD3L(DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
    int ret = DISAS_NEXT;
    TCGv temp_3 = tcg_temp_local_new();
    TCGv cc_flag = tcg_temp_local_new();
    TCGv lb = tcg_temp_local_new();
    TCGv lc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_7 = tcg_temp_local_new();
    TCGv temp_6 = tcg_temp_local_new();
    getCCFlag(temp_3);
    tcg_gen_mov_tl(cc_flag, temp_3);
    tcg_gen_mov_tl(lb, b);
    tcg_gen_shli_tl(lc, c, 3);
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);
    tcg_gen_add_tl(a, b, lc);
    if ((getFFlag () == true)) {
        setZFlag(a);
        setNFlag(a);
        CarryADD(temp_5, a, lb, lc);
        tcg_gen_mov_tl(temp_4, temp_5);
        setCFlag(temp_4);
        OverflowADD(temp_7, a, lb, lc);
        tcg_gen_mov_tl(temp_6, temp_7);
        setVFlag(temp_6);
    }
    gen_set_label(done_1);
    tcg_temp_free(temp_3);
    tcg_temp_free(cc_flag);
    tcg_temp_free(lb);
    tcg_temp_free(lc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_7);
    tcg_temp_free(temp_6);

    return ret;
}






/* ADCL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarryADD, setVFlag, OverflowADD
--- code ---
{
  cc_flag = getCCFlag ();
  lb = @b;
  lc = @c;
  if((cc_flag == true))
    {
      lb = @b;
      lc = @c;
      @a = ((@b + @c) + getCFlag ());
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag (@a);
          setCFlag (CarryADD (@a, lb, lc));
          setVFlag (OverflowADD (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_ADCL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  tcg_gen_mov_tl(lb, b);
  tcg_gen_mov_tl(lc, c);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(lb, b);
  tcg_gen_mov_tl(lc, c);
  tcg_gen_add_tl(temp_4, b, c);
  getCFlag(temp_6);
  tcg_gen_mov_tl(temp_5, temp_6);
  tcg_gen_add_tl(a, temp_4, temp_5);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag(a);
  CarryADD(temp_8, a, lb, lc);
  tcg_gen_mov_tl(temp_7, temp_8);
  setCFlag(temp_7);
  OverflowADD(temp_10, a, lb, lc);
  tcg_gen_mov_tl(temp_9, temp_10);
  setVFlag(temp_9);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(lb);
  tcg_temp_free(lc);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);

  return ret;
}





/* SBCL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB, setVFlag, OverflowSUB
--- code ---
{
  cc_flag = getCCFlag ();
  lb = @b;
  lc = @c;
  if((cc_flag == true))
    {
      lb = @b;
      lc = @c;
      @a = ((@b - @c) - getCFlag ());
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag (@a);
          setCFlag (CarrySUB (@a, lb, lc));
          setVFlag (OverflowSUB (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_SBCL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  tcg_gen_mov_tl(lb, b);
  tcg_gen_mov_tl(lc, c);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(lb, b);
  tcg_gen_mov_tl(lc, c);
  tcg_gen_sub_tl(temp_4, b, c);
  getCFlag(temp_6);
  tcg_gen_mov_tl(temp_5, temp_6);
  tcg_gen_sub_tl(a, temp_4, temp_5);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag(a);
  CarrySUB(temp_8, a, lb, lc);
  tcg_gen_mov_tl(temp_7, temp_8);
  setCFlag(temp_7);
  OverflowSUB(temp_10, a, lb, lc);
  tcg_gen_mov_tl(temp_9, temp_10);
  setVFlag(temp_9);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(lb);
  tcg_temp_free(lc);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);

  return ret;
}





/* SUBL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB, setVFlag, OverflowSUB
--- code ---
{
  cc_flag = getCCFlag ();
  lb = @b;
  if((cc_flag == true))
    {
      lb = @b;
      lc = @c;
      @a = (@b - @c);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag (@a);
          setCFlag (CarrySUB (@a, lb, lc));
          setVFlag (OverflowSUB (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_SUBL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  tcg_gen_mov_tl(lb, b);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(lb, b);
  tcg_gen_mov_tl(lc, c);
  tcg_gen_sub_tl(a, b, c);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag(a);
  CarrySUB(temp_5, a, lb, lc);
  tcg_gen_mov_tl(temp_4, temp_5);
  setCFlag(temp_4);
  OverflowSUB(temp_7, a, lb, lc);
  tcg_gen_mov_tl(temp_6, temp_7);
  setVFlag(temp_6);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(lb);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lc);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);

  return ret;
}





/* SUB1L
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB, setVFlag, OverflowSUB
--- code ---
{
  cc_flag = getCCFlag ();
  lb = @b;
  if((cc_flag == true))
    {
      lb = @b;
      lc = (@c << 1);
      @a = (@b - lc);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag (@a);
          setCFlag (CarrySUB (@a, lb, lc));
          setVFlag (OverflowSUB (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_SUB1L (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  tcg_gen_mov_tl(lb, b);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(lb, b);
  tcg_gen_shli_tl(lc, c, 1);
  tcg_gen_sub_tl(a, b, lc);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag(a);
  CarrySUB(temp_5, a, lb, lc);
  tcg_gen_mov_tl(temp_4, temp_5);
  setCFlag(temp_4);
  OverflowSUB(temp_7, a, lb, lc);
  tcg_gen_mov_tl(temp_6, temp_7);
  setVFlag(temp_6);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(lb);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lc);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);

  return ret;
}





/* SUB2L
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB, setVFlag, OverflowSUB
--- code ---
{
  cc_flag = getCCFlag ();
  lb = @b;
  if((cc_flag == true))
    {
      lb = @b;
      lc = (@c << 2);
      @a = (@b - lc);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag (@a);
          setCFlag (CarrySUB (@a, lb, lc));
          setVFlag (OverflowSUB (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_SUB2L (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  tcg_gen_mov_tl(lb, b);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(lb, b);
  tcg_gen_shli_tl(lc, c, 2);
  tcg_gen_sub_tl(a, b, lc);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag(a);
  CarrySUB(temp_5, a, lb, lc);
  tcg_gen_mov_tl(temp_4, temp_5);
  setCFlag(temp_4);
  OverflowSUB(temp_7, a, lb, lc);
  tcg_gen_mov_tl(temp_6, temp_7);
  setVFlag(temp_6);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(lb);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lc);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);

  return ret;
}





/* SUB3L
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB, setVFlag, OverflowSUB
--- code ---
{
  cc_flag = getCCFlag ();
  lb = @b;
  if((cc_flag == true))
    {
      lb = @b;
      lc = (@c << 3);
      @a = (@b - lc);
      if((getFFlag () == true))
        {
          setZFlag (@a);
          setNFlag (@a);
          setCFlag (CarrySUB (@a, lb, lc));
          setVFlag (OverflowSUB (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_SUB3L (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  tcg_gen_mov_tl(lb, b);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(lb, b);
  tcg_gen_shli_tl(lc, c, 3);
  tcg_gen_sub_tl(a, b, lc);
  if ((getFFlag () == true))
    {
    setZFlag(a);
  setNFlag(a);
  CarrySUB(temp_5, a, lb, lc);
  tcg_gen_mov_tl(temp_4, temp_5);
  setCFlag(temp_4);
  OverflowSUB(temp_7, a, lb, lc);
  tcg_gen_mov_tl(temp_6, temp_7);
  setVFlag(temp_6);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(lb);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lc);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);

  return ret;
}





/* MAXL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB, setVFlag, OverflowSUB
--- code ---
{
  cc_flag = getCCFlag ();
  lb = @b;
  if((cc_flag == true))
    {
      lb = @b;
      lc = @c;
      alu = (lb - lc);
      if((lc >= lb))
        {
          @a = lc;
        }
      else
        {
          @a = lb;
        };
      if((getFFlag () == true))
        {
          setZFlag (alu);
          setNFlag (alu);
          setCFlag (CarrySUB (@a, lb, lc));
          setVFlag (OverflowSUB (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_MAXL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv alu = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  tcg_gen_mov_tl(lb, b);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(lb, b);
  tcg_gen_mov_tl(lc, c);
  tcg_gen_sub_tl(alu, lb, lc);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_GE, temp_3, lc, lb);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_mov_tl(a, lc);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_mov_tl(a, lb);
  gen_set_label(done_2);
  if ((getFFlag () == true))
    {
    setZFlag(alu);
  setNFlag(alu);
  CarrySUB(temp_7, a, lb, lc);
  tcg_gen_mov_tl(temp_6, temp_7);
  setCFlag(temp_6);
  OverflowSUB(temp_9, a, lb, lc);
  tcg_gen_mov_tl(temp_8, temp_9);
  setVFlag(temp_8);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(lb);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lc);
  tcg_temp_free(alu);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);

  return ret;
}





/* MINL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, CarrySUB, setVFlag, OverflowSUB
--- code ---
{
  cc_flag = getCCFlag ();
  lb = @b;
  if((cc_flag == true))
    {
      lb = @b;
      lc = @c;
      alu = (lb - lc);
      if((lc <= lb))
        {
          @a = lc;
        }
      else
        {
          @a = lb;
        };
      if((getFFlag () == true))
        {
          setZFlag (alu);
          setNFlag (alu);
          setCFlag (CarrySUB (@a, lb, lc));
          setVFlag (OverflowSUB (@a, lb, lc));
        };
    };
}
 */

int
arc_gen_MINL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv alu = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  tcg_gen_mov_tl(lb, b);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(lb, b);
  tcg_gen_mov_tl(lc, c);
  tcg_gen_sub_tl(alu, lb, lc);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_LE, temp_3, lc, lb);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_mov_tl(a, lc);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_mov_tl(a, lb);
  gen_set_label(done_2);
  if ((getFFlag () == true))
    {
    setZFlag(alu);
  setNFlag(alu);
  CarrySUB(temp_7, a, lb, lc);
  tcg_gen_mov_tl(temp_6, temp_7);
  setCFlag(temp_6);
  OverflowSUB(temp_9, a, lb, lc);
  tcg_gen_mov_tl(temp_8, temp_9);
  setVFlag(temp_8);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(lb);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lc);
  tcg_temp_free(alu);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);

  return ret;
}





/* CMPL
 *    Variables: @b, @c
 *    Functions: getCCFlag, setZFlag, setNFlag, setCFlag, CarrySUB, setVFlag, OverflowSUB
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      alu = (@b - @c);
      setZFlag (alu);
      setNFlag (alu);
      setCFlag (CarrySUB (alu, @b, @c));
      setVFlag (OverflowSUB (alu, @b, @c));
    };
}
 */

int
arc_gen_CMPL (DisasCtxt *ctx, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv alu = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_sub_tl(alu, b, c);
  setZFlag(alu);
  setNFlag(alu);
  CarrySUB(temp_5, alu, b, c);
  tcg_gen_mov_tl(temp_4, temp_5);
  setCFlag(temp_4);
  OverflowSUB(temp_7, alu, b, c);
  tcg_gen_mov_tl(temp_6, temp_7);
  setVFlag(temp_6);
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(alu);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);

  return ret;
}





/* ANDL
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = (@b & @c);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
        };
    };
}
 */

int
arc_gen_ANDL (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_and_tl(a, b, c);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);

  return ret;
}





/* ORL
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = (@b | @c);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
        };
    };
}
 */

int
arc_gen_ORL (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_or_tl(a, b, c);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);

  return ret;
}





/* XORL
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = (@b ^ @c);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
        };
    };
}
 */

int
arc_gen_XORL (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_xor_tl(a, b, c);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);

  return ret;
}





/* MOVL
 *    Variables: @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = @b;
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
        };
    };
}
 */

int
arc_gen_MOVL (DisasCtxt *ctx, TCGv a, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(a, b);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);

  return ret;
}

/* MOVHL
 *    Variables: @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp = @b << 32;
      @a = @b
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
        };
    };
}
 */

int
arc_gen_MOVHL (DisasCtxt *ctx, TCGv a, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;

      //tmp = @b << 32;
      //@a = tmp;

  tcg_gen_shli_tl(temp_4, b, 32);
  tcg_gen_mov_tl(a, temp_4);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);

  return ret;
}





/* ASLL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag, setCFlag, getBit
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lb = @b;
      lc = (@c & 63);
      @a = (lb << lc);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
          if((lc == 0))
            {
              setCFlag (0);
            }
          else
            {
              setCFlag (getBit (lb, (64 - lc)));
            };
        };
    };
}
 */

int
arc_gen_ASLL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  int f_flag;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(lb, b);
  tcg_gen_andi_tl(lc, c, 63);
  tcg_gen_shl_tl(a, lb, lc);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, lc, 0);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_movi_tl(temp_6, 0);
  setCFlag(temp_6);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_subfi_tl(temp_9, 64, lc);
  getBit(temp_8, lb, temp_9);
  tcg_gen_mov_tl(temp_7, temp_8);
  setCFlag(temp_7);
  gen_set_label(done_2);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lb);
  tcg_temp_free(lc);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);

  return ret;
}





/* ASRL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, arithmeticShiftRight, getFFlag, setZFlag, setNFlag, setCFlag, getBit
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lb = @b;
      lc = (@c & 63);
      @a = arithmeticShiftRight (lb, lc);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
          if((lc == 0))
            {
              setCFlag (0);
            }
          else
            {
              setCFlag (getBit (lb, (lc - 1)));
            };
        };
    };
}
 */

int
arc_gen_ASRL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  int f_flag;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(lb, b);
  tcg_gen_andi_tl(lc, c, 63);
  arithmeticShiftRight(temp_6, lb, lc);
  tcg_gen_mov_tl(a, temp_6);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, lc, 0);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_movi_tl(temp_7, 0);
  setCFlag(temp_7);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_subi_tl(temp_10, lc, 1);
  getBit(temp_9, lb, temp_10);
  tcg_gen_mov_tl(temp_8, temp_9);
  setCFlag(temp_8);
  gen_set_label(done_2);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lb);
  tcg_temp_free(lc);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);

  return ret;
}





/* LSRL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag, logicalShiftRight, getFFlag, setZFlag, setNFlag, setCFlag, getBit
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lb = @b;
      lc = (@c & 63);
      @a = logicalShiftRight (lb, lc);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
          if((lc == 0))
            {
              setCFlag (0);
            }
          else
            {
              setCFlag (getBit (lb, (lc - 1)));
            };
        };
    };
}
 */

int
arc_gen_LSRL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lb = tcg_temp_local_new();
  TCGv lc = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  int f_flag;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(lb, b);
  tcg_gen_andi_tl(lc, c, 63);
  logicalShiftRight(temp_6, lb, lc);
  tcg_gen_mov_tl(a, temp_6);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, lc, 0);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_movi_tl(temp_7, 0);
  setCFlag(temp_7);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_subi_tl(temp_10, lc, 1);
  getBit(temp_9, lb, temp_10);
  tcg_gen_mov_tl(temp_8, temp_9);
  setCFlag(temp_8);
  gen_set_label(done_2);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lb);
  tcg_temp_free(lc);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);

  return ret;
}






/* BICL
 *    Variables: @a, @b, @c
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @a = (@b & ~@c);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
        };
    };
}
 */

int
arc_gen_BICL (DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_not_tl(temp_4, c);
  tcg_gen_and_tl(a, b, temp_4);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);

  return ret;
}





/* BCLRL
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp = (1 << (@c & 63));
      @a = (@b & ~tmp);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
        };
    };
}
 */

int
arc_gen_BCLRL (DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv tmp = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(temp_4, c, 63);
  tcg_gen_shlfi_tl(tmp, 1, temp_4);
  tcg_gen_not_tl(temp_5, tmp);
  tcg_gen_and_tl(a, b, temp_5);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);
  tcg_temp_free(tmp);
  tcg_temp_free(temp_5);

  return ret;
}





/* BMSKL
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp1 = ((@c & 63) + 1);
      if((tmp1 == 64))
        {
          tmp2 = 0xffffffffffffffff;
        }
      else
        {
          tmp2 = ((1 << tmp1) - 1);
        };
      @a = (@b & tmp2);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
        };
    };
}
 */

int
arc_gen_BMSKL (DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv tmp1 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv tmp2 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(temp_6, c, 63);
  tcg_gen_addi_tl(tmp1, temp_6, 1);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, tmp1, 64);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_movi_tl(tmp2, 0xffffffffffffffff);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_shlfi_tl(temp_7, 1, tmp1);
  tcg_gen_subi_tl(tmp2, temp_7, 1);
  gen_set_label(done_2);
  tcg_gen_and_tl(a, b, tmp2);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(tmp1);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(tmp2);
  tcg_temp_free(temp_7);

  return ret;
}





/* BMSKNL
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp1 = ((@c & 63) + 1);
      if((tmp1 == 64))
        {
          tmp2 = 0xffffffffffffffff;
        }
      else
        {
          tmp2 = ((1 << tmp1) - 1);
        };
      @a = (@b & ~tmp2);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
        };
    };
}
 */

int
arc_gen_BMSKNL (DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv tmp1 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv tmp2 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(temp_6, c, 63);
  tcg_gen_addi_tl(tmp1, temp_6, 1);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_3, tmp1, 64);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_movi_tl(tmp2, 0xffffffffffffffff);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_shlfi_tl(temp_7, 1, tmp1);
  tcg_gen_subi_tl(tmp2, temp_7, 1);
  gen_set_label(done_2);
  tcg_gen_not_tl(temp_8, tmp2);
  tcg_gen_and_tl(a, b, temp_8);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(tmp1);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(tmp2);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_8);

  return ret;
}





/* BSETL
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp = (1 << (@c & 63));
      @a = (@b | tmp);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
        };
    };
}
 */

int
arc_gen_BSETL (DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv tmp = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(temp_4, c, 63);
  tcg_gen_shlfi_tl(tmp, 1, temp_4);
  tcg_gen_or_tl(a, b, tmp);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);
  tcg_temp_free(tmp);

  return ret;
}





/* BXORL
 *    Variables: @c, @a, @b
 *    Functions: getCCFlag, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp = (1 << @c);
      @a = (@b ^ tmp);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@a);
          setNFlag (@a);
        };
    };
}
 */

int
arc_gen_BXORL (DisasCtxt *ctx, TCGv c, TCGv a, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv tmp = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_shlfi_tl(tmp, 1, c);
  tcg_gen_xor_tl(a, b, tmp);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(a);
  setNFlag(a);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(tmp);

  return ret;
}





/* ROLL
 *    Variables: @src, @dest
 *    Functions: getCCFlag, rotateLeft, getFFlag, setZFlag, setNFlag, setCFlag, extractBits
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      lsrc = @src;
      @dest = rotateLeft (lsrc, 1);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag (@dest);
          setCFlag (extractBits (lsrc, 63, 63));
        };
    };
}
 */

int
arc_gen_ROLL (DisasCtxt *ctx, TCGv src, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv lsrc = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(lsrc, src);
  tcg_gen_movi_tl(temp_5, 1);
  rotateLeft(temp_4, lsrc, temp_5);
  tcg_gen_mov_tl(dest, temp_4);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag(dest);
  tcg_gen_movi_tl(temp_9, 63);
  tcg_gen_movi_tl(temp_8, 63);
  extractBits(temp_7, lsrc, temp_8, temp_9);
  tcg_gen_mov_tl(temp_6, temp_7);
  setCFlag(temp_6);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(lsrc);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);

  return ret;
}





/* SEXBL
 *    Variables: @dest, @src
 *    Functions: getCCFlag, arithmeticShiftRight, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @dest = arithmeticShiftRight ((@src << 24), 24);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag (@dest);
        };
    };
}
 */

int
arc_gen_SEXBL (DisasCtxt *ctx, TCGv dest, TCGv src)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_movi_tl(temp_6, 56);
  tcg_gen_shli_tl(temp_5, src, 56);
  arithmeticShiftRight(temp_4, temp_5, temp_6);
  tcg_gen_mov_tl(dest, temp_4);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag(dest);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* SEXHL
 *    Variables: @dest, @src
 *    Functions: getCCFlag, arithmeticShiftRight, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @dest = arithmeticShiftRight ((@src << 16), 16);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag (@dest);
        };
    };
}
 */

int
arc_gen_SEXHL (DisasCtxt *ctx, TCGv dest, TCGv src)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_movi_tl(temp_6, 48);
  tcg_gen_shli_tl(temp_5, src, 48);
  arithmeticShiftRight(temp_4, temp_5, temp_6);
  tcg_gen_mov_tl(dest, temp_4);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag(dest);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}


/* SEXWL
 *    Variables: @dest, @src
 *    Functions: getCCFlag, arithmeticShiftRight, getFFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      @dest = arithmeticShiftRight ((@src << 32), 32);
      f_flag = getFFlag ();
      if((f_flag == true))
        {
          setZFlag (@dest);
          setNFlag (@dest);
        };
    };
}
 */

int
arc_gen_SEXWL (DisasCtxt *ctx, TCGv dest, TCGv src)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  int f_flag;
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_movi_tl(temp_6, 32);
  tcg_gen_shli_tl(temp_5, src, 32);
  arithmeticShiftRight(temp_4, temp_5, temp_6);
  tcg_gen_mov_tl(dest, temp_4);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag(dest);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* BTSTL
 *    Variables: @c, @b
 *    Functions: getCCFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp = (1 << (@c & 63));
      alu = (@b & tmp);
      setZFlag (alu);
      setNFlag (alu);
    };
}
 */

int
arc_gen_BTSTL (DisasCtxt *ctx, TCGv c, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv tmp = tcg_temp_local_new();
  TCGv alu = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_andi_tl(temp_4, c, 63);
  tcg_gen_shlfi_tl(tmp, 1, temp_4);
  tcg_gen_and_tl(alu, b, tmp);
  setZFlag(alu);
  setNFlag(alu);
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);
  tcg_temp_free(tmp);
  tcg_temp_free(alu);

  return ret;
}





/* TSTL
 *    Variables: @b, @c
 *    Functions: getCCFlag, setZFlag, setNFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      alu = (@b & @c);
      setZFlag (alu);
      setNFlag (alu);
    };
}
 */

int
arc_gen_TSTL (DisasCtxt *ctx, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv alu = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_and_tl(alu, b, c);
  setZFlag(alu);
  setNFlag(alu);
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(alu);

  return ret;
}





/* XBFUL
 *    Variables: @src2, @src1, @dest
 *    Functions: getCCFlag, extractBits, getFFlag, setZFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      N = extractBits (@src2, 5, 0);
      M = (extractBits (@src2, 11, 6) + 1);
      tmp1 = (@src1 >> N);
      tmp2 = ((1 << M) - 1);
      @dest = (tmp1 & tmp2);
      if((getFFlag () == true))
        {
          setZFlag (@dest);
        };
    };
}
 */

int
arc_gen_XBFUL (DisasCtxt *ctx, TCGv src2, TCGv src1, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv N = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv M = tcg_temp_local_new();
  TCGv tmp1 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  TCGv tmp2 = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_movi_tl(temp_6, 0);
  tcg_gen_movi_tl(temp_5, 5);
  extractBits(temp_4, src2, temp_5, temp_6);
  tcg_gen_mov_tl(N, temp_4);
  tcg_gen_movi_tl(temp_10, 6);
  tcg_gen_movi_tl(temp_9, 11);
  extractBits(temp_8, src2, temp_9, temp_10);
  tcg_gen_mov_tl(temp_7, temp_8);
  tcg_gen_addi_tl(M, temp_7, 1);
  tcg_gen_shr_tl(tmp1, src1, N);
  tcg_gen_shlfi_tl(temp_11, 1, M);
  tcg_gen_subi_tl(tmp2, temp_11, 1);
  tcg_gen_and_tl(dest, tmp1, tmp2);
  if ((getFFlag () == true))
    {
    setZFlag(dest);
;
    }
  else
    {
  ;
    }
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);
  tcg_temp_free(N);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_7);
  tcg_temp_free(M);
  tcg_temp_free(tmp1);
  tcg_temp_free(temp_11);
  tcg_temp_free(tmp2);

  return ret;
}





/* AEXL
 *    Variables: @src2, @b
 *    Functions: getCCFlag, readAuxReg, writeAuxReg
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      tmp = readAuxReg (@src2);
      writeAuxReg (@src2, @b);
      @b = tmp;
    };
}
 */

int
arc_gen_AEXL (DisasCtxt *ctx, TCGv src2, TCGv b)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv tmp = tcg_temp_local_new();
  getCCFlag(temp_3);
  tcg_gen_mov_tl(cc_flag, temp_3);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  readAuxReg(temp_4, src2);
  tcg_gen_mov_tl(tmp, temp_4);
  writeAuxReg(src2, b);
  tcg_gen_mov_tl(b, tmp);
  gen_set_label(done_1);
  tcg_temp_free(temp_3);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);
  tcg_temp_free(tmp);

  return ret;
}





/* LRL
 *    Variables: @dest, @src
 *    Functions: readAuxReg
--- code ---
{
  @dest = readAuxReg (@src);
}
 */

int
arc_gen_LRL (DisasCtxt *ctx, TCGv dest, TCGv src)
{
  int ret = DISAS_NORETURN;

  if (tb_cflags(ctx->base.tb) & CF_USE_ICOUNT) {
      gen_io_start();
  }

  TCGv temp_1 = tcg_temp_local_new();
  readAuxReg(temp_1, src);
  tcg_gen_mov_tl(dest, temp_1);
  tcg_temp_free(temp_1);

  return ret;
}





/* DIVL
 *    Variables: @src2, @src1, @dest
 *    Functions: getCCFlag, divSigned, getFFlag, setZFlag, setNFlag, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      if(((@src2 != 0) && ((@src1 != 2147483648) || (@src2 != 4294967295))))
        {
          @dest = divSigned (@src1, @src2);
          if((getFFlag () == true))
            {
              setZFlag (@dest);
              setNFlag (@dest);
              setVFlag (0);
            };
        }
      else
        {
        };
    };
}
 */

int
arc_gen_DIVL (DisasCtxt *ctx, TCGv src2, TCGv src1, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_9 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  getCCFlag(temp_9);
  tcg_gen_mov_tl(cc_flag, temp_9);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_3, src2, 0);
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_4, src1, 2147483648);
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_5, src2, 4294967295);
  tcg_gen_or_tl(temp_6, temp_4, temp_5);
  tcg_gen_and_tl(temp_7, temp_3, temp_6);
  tcg_gen_xori_tl(temp_8, temp_7, 1); tcg_gen_andi_tl(temp_8, temp_8, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_8, arc_true, else_2);;
  divSigned(temp_10, src1, src2);
  tcg_gen_mov_tl(dest, temp_10);
  if ((getFFlag () == true))
    {
    setZFlag(dest);
  setNFlag(dest);
  tcg_gen_movi_tl(temp_11, 0);
  setVFlag(temp_11);
;
    }
  else
    {
  ;
    }
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_9);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_11);

  return ret;
}





/* DIVUL
 *    Variables: @src2, @dest, @src1
 *    Functions: getCCFlag, divUnsigned, getFFlag, setZFlag, setNFlag, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      if((@src2 != 0))
        {
          @dest = divUnsigned (@src1, @src2);
          if((getFFlag () == true))
            {
              setZFlag (@dest);
              setNFlag (0);
              setVFlag (0);
            };
        }
      else
        {
        };
    };
}
 */

int
arc_gen_DIVUL (DisasCtxt *ctx, TCGv src2, TCGv dest, TCGv src1)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_3, src2, 0);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  divUnsigned(temp_6, src1, src2);
  tcg_gen_mov_tl(dest, temp_6);
  if ((getFFlag () == true))
    {
    setZFlag(dest);
  tcg_gen_movi_tl(temp_7, 0);
  setNFlag(temp_7);
  tcg_gen_movi_tl(temp_8, 0);
  setVFlag(temp_8);
;
    }
  else
    {
  ;
    }
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_8);

  return ret;
}





/* REML
 *    Variables: @src2, @src1, @dest
 *    Functions: getCCFlag, divRemainingSigned, getFFlag, setZFlag, setNFlag, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      if(((@src2 != 0) && ((@src1 != 2147483648) || (@src2 != 4294967295))))
        {
          @dest = divRemainingSigned (@src1, @src2);
          if((getFFlag () == true))
            {
              setZFlag (@dest);
              setNFlag (@dest);
              setVFlag (0);
            };
        }
      else
        {
        };
    };
}
 */

int
arc_gen_REML (DisasCtxt *ctx, TCGv src2, TCGv src1, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_9 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv temp_10 = tcg_temp_local_new();
  TCGv temp_11 = tcg_temp_local_new();
  getCCFlag(temp_9);
  tcg_gen_mov_tl(cc_flag, temp_9);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_3, src2, 0);
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_4, src1, 2147483648);
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_5, src2, 4294967295);
  tcg_gen_or_tl(temp_6, temp_4, temp_5);
  tcg_gen_and_tl(temp_7, temp_3, temp_6);
  tcg_gen_xori_tl(temp_8, temp_7, 1); tcg_gen_andi_tl(temp_8, temp_8, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_8, arc_true, else_2);;
  divRemainingSigned(temp_10, src1, src2);
  tcg_gen_mov_tl(dest, temp_10);
  if ((getFFlag () == true))
    {
    setZFlag(dest);
  setNFlag(dest);
  tcg_gen_movi_tl(temp_11, 0);
  setVFlag(temp_11);
;
    }
  else
    {
  ;
    }
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_9);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_8);
  tcg_temp_free(temp_10);
  tcg_temp_free(temp_11);

  return ret;
}





/* REMUL
 *    Variables: @src2, @dest, @src1
 *    Functions: getCCFlag, divRemainingUnsigned, getFFlag, setZFlag, setNFlag, setVFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      if((@src2 != 0))
        {
          @dest = divRemainingUnsigned (@src1, @src2);
          if((getFFlag () == true))
            {
              setZFlag (@dest);
              setNFlag (0);
              setVFlag (0);
            };
        }
      else
        {
        };
    };
}
 */

int
arc_gen_REMUL (DisasCtxt *ctx, TCGv src2, TCGv dest, TCGv src1)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcondi_tl(TCG_COND_NE, temp_3, src2, 0);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  divRemainingUnsigned(temp_6, src1, src2);
  tcg_gen_mov_tl(dest, temp_6);
  if ((getFFlag () == true))
    {
    setZFlag(dest);
  tcg_gen_movi_tl(temp_7, 0);
  setNFlag(temp_7);
  tcg_gen_movi_tl(temp_8, 0);
  setVFlag(temp_8);
;
    }
  else
    {
  ;
    }
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_8);

  return ret;
}





/* ABSL
 *    Variables: @src, @dest
 *    Functions: Carry, getFFlag, setZFlag, setNFlag, setCFlag, Zero, setVFlag, getNFlag
--- code ---
{
  lsrc = @src;
  alu = (0 - lsrc);
  if((Carry (lsrc) == 1))
    {
      @dest = alu;
    }
  else
    {
      @dest = lsrc;
    };
  if((getFFlag () == true))
    {
      setZFlag (@dest);
      setNFlag (@dest);
      setCFlag (Zero ());
      setVFlag (getNFlag ());
    };
}
 */

int
arc_gen_ABSL (DisasCtxt *ctx, TCGv src, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv lsrc = tcg_temp_local_new();
  TCGv alu = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  tcg_gen_mov_tl(lsrc, src);
  tcg_gen_subfi_tl(alu, 0, lsrc);
  TCGLabel *else_1 = gen_new_label();
  TCGLabel *done_1 = gen_new_label();
  Carry(temp_3, lsrc);
  tcg_gen_setcondi_tl(TCG_COND_EQ, temp_1, temp_3, 1);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_1);;
  tcg_gen_mov_tl(dest, alu);
  tcg_gen_br(done_1);
  gen_set_label(else_1);
  tcg_gen_mov_tl(dest, lsrc);
  gen_set_label(done_1);
  if ((getFFlag () == true))
    {
    setZFlag(dest);
  setNFlag(dest);
  tcg_gen_mov_tl(temp_4, Zero());
  setCFlag(temp_4);
  tcg_gen_mov_tl(temp_5, getNFlag());
  setVFlag(temp_5);
;
    }
  else
    {
  ;
    }
  tcg_temp_free(lsrc);
  tcg_temp_free(alu);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);

  return ret;
}





/* SWAP
 *    Variables: @src, @dest
 *    Functions: getFFlag, setZFlag, setNFlag
--- code ---
{
  tmp1 = (@src << 32);
  tmp2 = ((@src >> 32) & 0xFFFFFFFF);
  @dest = (tmp1 | tmp2);
  f_flag = getFFlag ();
  if((f_flag == true))
    {
      setZFlag (@dest);
      setNFlag (@dest);
    };
}
 */

int
arc_gen_SWAPL (DisasCtxt *ctx, TCGv src, TCGv dest)
{
    const int ret = DISAS_NEXT;
    TCGv hi = tcg_temp_local_new();
    TCGv lo = tcg_temp_local_new();

    tcg_gen_shli_tl(lo, src, 32);
    tcg_gen_shri_tl(hi, src, 32);
    tcg_gen_andi_tl(hi, hi, 0xFFFFFFFF);
    tcg_gen_or_tl(dest, hi, lo);

    if (getFFlag() == true) {
        setZFlag(dest);
        setNFlag(dest);
    }

    tcg_temp_free(hi);
    tcg_temp_free(lo);

    return ret;
}





/* SWAPE
 *    Variables: @src, @dest
 *    Functions: getFFlag, setZFlag, setNFlag
--- code ---
{
  @dest = tcg_gen_bswap64_tl(@src);
  f_flag = getFFlag ();
  if((f_flag == true))
    {
      setZFlag (@dest);
      setNFlag (@dest);
    };
}
 */

int
arc_gen_SWAPEL (DisasCtxt *ctx, TCGv src, TCGv dest)
{
    const int ret = DISAS_NEXT;

    tcg_gen_bswap64_tl(dest, src);

    if (getFFlag() == true) {
        setZFlag(dest);
        setNFlag(dest);
    }

    return ret;
}





/* NOTL
 *    Variables: @dest, @src
 *    Functions: getFFlag, setZFlag, setNFlag
--- code ---
{
  @dest = ~@src;
  f_flag = getFFlag ();
  if((f_flag == true))
    {
      setZFlag (@dest);
      setNFlag (@dest);
    };
}
 */

int
arc_gen_NOTL (DisasCtxt *ctx, TCGv dest, TCGv src)
{
  int ret = DISAS_NEXT;
  int f_flag;
  tcg_gen_not_tl(dest, src);
  f_flag = getFFlag ();
  if ((f_flag == true))
    {
    setZFlag(dest);
  setNFlag(dest);
;
    }
  else
    {
  ;
    }

  return ret;
}





/* SETEQL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = @b;
      p_c = @c;
      if((p_b == p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    };
}
 */

int
arc_gen_SETEQL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(p_b, b);
  tcg_gen_mov_tl(p_c, c);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_3, p_b, p_c);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(p_b);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);

  return ret;
}


/*
 * BREQL
 * --- code ---
 * target = cpl + offset
 *
 * if (b == c)
 *   gen_branchi(target)
 */
int
arc_gen_BREQL(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_brcond_tl(TCG_COND_NE, b, c, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}



/* SETNEL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = @b;
      p_c = @c;
      if((p_b != p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    };
}
 */

int
arc_gen_SETNEL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(p_b, b);
  tcg_gen_mov_tl(p_c, c);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_NE, temp_3, p_b, p_c);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(p_b);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);

  return ret;
}


/*
 * BRNEL
 * --- code ---
 * target = cpl + offset
 *
 * if (b != c)
 *   gen_branchi(target)
 */
int
arc_gen_BRNEL(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_brcond_tl(TCG_COND_EQ, b, c, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}



/* SETLTL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = @b;
      p_c = @c;
      if((p_b < p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    };
}
 */

int
arc_gen_SETLTL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(p_b, b);
  tcg_gen_mov_tl(p_c, c);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_LT, temp_3, p_b, p_c);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(p_b);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);

  return ret;
}


/*
 * BRLTL
 * --- code ---
 * target = cpl + offset
 *
 * if (b s< c)
 *   gen_branchi(target)
 * }
 */
int
arc_gen_BRLTL(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_brcond_tl(TCG_COND_GE, b, c, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/* SETGEL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = @b;
      p_c = @c;
      if((p_b >= p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    };
}
 */

int
arc_gen_SETGEL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv v = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(p_b, b);
  tcg_gen_mov_tl(p_c, c);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_GE, temp_3, p_b, p_c);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(v);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);
  tcg_temp_free(p_b);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);

  return ret;
}


/*
 * BRGEL
 * --- code ---
 * target = cpl + offset
 *
 * if (b s>= c)
 *   gen_branchi(target)
 */
int
arc_gen_BRGEL(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_brcond_tl(TCG_COND_LT, b, c, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}



/* SETLEL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = @b;
      p_c = @c;
      if((p_b <= p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    };
}
 */

int
arc_gen_SETLEL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv v = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(p_b, b);
  tcg_gen_mov_tl(p_c, c);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_LE, temp_3, p_b, p_c);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(v);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);
  tcg_temp_free(p_b);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);

  return ret;
}





/* SETGTL
 *    Variables: @b, @c, @a
 *    Functions: getCCFlag
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = @b;
      p_c = @c;
      if((p_b > p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    };
}
 */

int
arc_gen_SETGTL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv temp_5 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv v = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv p_b = tcg_temp_local_new();
  TCGv temp_9 = tcg_temp_local_new();
  TCGv temp_8 = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  getCCFlag(temp_5);
  tcg_gen_mov_tl(cc_flag, temp_5);
  TCGLabel *done_1 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(temp_2, temp_1, 1); tcg_gen_andi_tl(temp_2, temp_2, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, done_1);;
  tcg_gen_mov_tl(p_b, b);
  tcg_gen_mov_tl(p_c, c);
  TCGLabel *else_2 = gen_new_label();
  TCGLabel *done_2 = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_GT, temp_3, p_b, p_c);
  tcg_gen_xori_tl(temp_4, temp_3, 1); tcg_gen_andi_tl(temp_4, temp_4, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_4, arc_true, else_2);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_2);
  gen_set_label(else_2);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_2);
  gen_set_label(done_1);
  tcg_temp_free(temp_5);
  tcg_temp_free(cc_flag);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);
  tcg_temp_free(v);
  tcg_temp_free(temp_7);
  tcg_temp_free(temp_6);
  tcg_temp_free(p_b);
  tcg_temp_free(temp_9);
  tcg_temp_free(temp_8);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_4);

  return ret;
}


/*
 * BRLOL
 * --- code ---
 * target = cpl + offset
 *
 * if (b u< c)
 *   gen_branchi(target)
 */
int
arc_gen_BRLOL(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_brcond_tl(TCG_COND_GEU, b, c, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/* SETLOL
 *    Variables: @b, @c, @a
 *    Functions: unsignedLT
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = @b;
      p_c = @c;
      if(unsignedLT (p_b, p_c))
	{
	  @a = true;
	}
      else
	{
	  @a = false;
	};
    }
}
 */

int
arc_gen_SETLOL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv p_b = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv cc_temp_1 = tcg_temp_local_new();
  getCCFlag(cc_flag);
  TCGLabel *done_cc = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, cc_temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(cc_temp_1, cc_temp_1, 1); tcg_gen_andi_tl(cc_temp_1, cc_temp_1, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, cc_temp_1, arc_true, done_cc);;
  tcg_gen_mov_tl(p_b, b);
  tcg_gen_mov_tl(p_c, c);
  TCGLabel *else_1 = gen_new_label();
  TCGLabel *done_1 = gen_new_label();
  unsignedLT(temp_2, p_b, p_c);
  tcg_gen_xori_tl(temp_1, temp_2, 1); tcg_gen_andi_tl(temp_1, temp_1, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_1, arc_true, else_1);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_1);
  gen_set_label(else_1);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_1);
  gen_set_label(done_cc);
  tcg_temp_free(p_b);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_1);
  tcg_temp_free(cc_temp_1);
  tcg_temp_free(cc_flag);

  return ret;
}


/*
 * BRHSL
 * --- code ---
 * target = cpl + offset
 *
 * if (b u>= c)
 *   gen_branchi(target)
 */
int
arc_gen_BRHSL(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv cond = tcg_temp_local_new();

    update_delay_flag(ctx);

    tcg_gen_brcond_tl(TCG_COND_LTU, b, c, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(cond);

    return DISAS_NORETURN;
}


/* SETHSL
 *    Variables: @b, @c, @a
 *    Functions: unsignedGE
--- code ---
{
  cc_flag = getCCFlag ();
  if((cc_flag == true))
    {
      p_b = @b;
      p_c = @c;
      if(unsignedGE (p_b, p_c))
        {
          @a = true;
        }
      else
        {
          @a = false;
        };
    }
}
 */

int
arc_gen_SETHSL (DisasCtxt *ctx, TCGv b, TCGv c, TCGv a)
{
  int ret = DISAS_NEXT;
  TCGv p_b = tcg_temp_local_new();
  TCGv p_c = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv cc_flag = tcg_temp_local_new();
  TCGv cc_temp_1 = tcg_temp_local_new();
  getCCFlag(cc_flag);
  TCGLabel *done_cc = gen_new_label();
  tcg_gen_setcond_tl(TCG_COND_EQ, cc_temp_1, cc_flag, arc_true);
  tcg_gen_xori_tl(cc_temp_1, cc_temp_1, 1); tcg_gen_andi_tl(cc_temp_1, cc_temp_1, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, cc_temp_1, arc_true, done_cc);;
  tcg_gen_mov_tl(p_b, b);
  tcg_gen_mov_tl(p_c, c);
  TCGLabel *else_1 = gen_new_label();
  TCGLabel *done_1 = gen_new_label();
  unsignedGE(temp_2, p_b, p_c);
  tcg_gen_xori_tl(temp_1, temp_2, 1); tcg_gen_andi_tl(temp_1, temp_1, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_1, arc_true, else_1);;
  tcg_gen_mov_tl(a, arc_true);
  tcg_gen_br(done_1);
  gen_set_label(else_1);
  tcg_gen_mov_tl(a, arc_false);
  gen_set_label(done_1);
  gen_set_label(done_cc);
  tcg_temp_free(p_b);
  tcg_temp_free(p_c);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_1);
  tcg_temp_free(cc_temp_1);
  tcg_temp_free(cc_flag);

  return ret;
}





/*
 * EXL - CODED BY HAND
 */

int
arc_gen_EXL (DisasCtxt *ctx, TCGv b, TCGv c)
{
  int ret = DISAS_NEXT;
  TCGv temp = tcg_temp_local_new();
  tcg_gen_mov_tl(temp, b);
  tcg_gen_atomic_xchg_tl(b, c, temp, ctx->mem_idx, MO_UQ);
  tcg_temp_free(temp);

  return ret;
}





/* LDL
 *    Variables: @src1, @src2, @dest
 *    Functions: getAAFlag, getZZFlag, setDebugLD, getMemory, getFlagX, SignExtend, NoFurtherLoadsPending
--- code ---
{
  AA = getAAFlag ();
  ZZ = getZZFlag ();
  address = 0;
  if(((AA == 0) || (AA == 1)))
    {
      address = (@src1 + @src2);
    };
  if((AA == 2))
    {
      address = @src1;
    };
  if(((AA == 3) && ((ZZ == 0) || (ZZ == 3))))
    {
      address = (@src1 + (@src2 << 2));
    };
  if(((AA == 3) && (ZZ == 2)))
    {
      address = (@src1 + (@src2 << 1));
    };
  l_src1 = @src1;
  l_src2 = @src2;
  setDebugLD (1);
  new_dest = getMemory (address, ZZ);
  if(((AA == 1) || (AA == 2)))
    {
      @src1 = (l_src1 + l_src2);
    };
  if((getFlagX () == 1))
    {
      new_dest = SignExtend (new_dest, ZZ);
    };
  if(NoFurtherLoadsPending ())
    {
      setDebugLD (0);
    };
  @dest = new_dest;
}
 */

int
arc_gen_LDL (DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
  int ret = DISAS_NEXT;
  int AA;
  int ZZ;
  TCGv address = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv l_src1 = tcg_temp_local_new();
  TCGv l_src2 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv new_dest = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_7 = tcg_temp_local_new();
  AA = getAAFlag ();
  ZZ = getZZFlag ();
  tcg_gen_movi_tl(address, 0);
  if (((AA == 0) || (AA == 1)))
    {
    tcg_gen_add_tl(address, src1, src2);
;
    }
  else
    {
  ;
    }
  if ((AA == 2))
    {
    tcg_gen_mov_tl(address, src1);
;
    }
  else
    {
  ;
    }
  if (((AA == 3) && ((ZZ == 0) || (ZZ == 3))))
    {
    tcg_gen_shli_tl(temp_2, src2, 3);
  tcg_gen_add_tl(address, src1, temp_2);
;
    }
  else
    {
  ;
    }
  if (((AA == 3) && (ZZ == 2)))
    {
    tcg_gen_shli_tl(temp_3, src2, 1);
  tcg_gen_add_tl(address, src1, temp_3);
;
    }
  else
    {
  ;
    }
  tcg_gen_mov_tl(l_src1, src1);
  tcg_gen_mov_tl(l_src2, src2);
  tcg_gen_movi_tl(temp_4, 1);
  setDebugLD(temp_4);
  getMemory(temp_5, address, ZZ);
  tcg_gen_mov_tl(new_dest, temp_5);
  if (((AA == 1) || (AA == 2)))
    {
    tcg_gen_add_tl(src1, l_src1, l_src2);
;
    }
  else
    {
  ;
    }
  if ((getFlagX () == 1))
    {
    new_dest = SignExtend (new_dest, ZZ);
;
    }
  else
    {
  ;
    }
  TCGLabel *done_1 = gen_new_label();
  NoFurtherLoadsPending(temp_6);
  tcg_gen_xori_tl(temp_1, temp_6, 1); tcg_gen_andi_tl(temp_1, temp_1, 1);;
  tcg_gen_brcond_tl(TCG_COND_EQ, temp_1, arc_true, done_1);;
  tcg_gen_movi_tl(temp_7, 0);
  setDebugLD(temp_7);
  gen_set_label(done_1);
  tcg_gen_mov_tl(dest, new_dest);
  tcg_temp_free(address);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_3);
  tcg_temp_free(l_src1);
  tcg_temp_free(l_src2);
  tcg_temp_free(temp_4);
  tcg_temp_free(temp_5);
  tcg_temp_free(new_dest);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_7);

  return ret;
}





/* STL
 *    Variables: @src1, @src2, @dest
 *    Functions: getAAFlag, getZZFlag, setMemory
--- code ---
{
  AA = getAAFlag ();
  ZZ = getZZFlag ();
  address = 0;
  if(((AA == 0) || (AA == 1)))
    {
      address = (@src1 + @src2);
    };
  if((AA == 2))
    {
      address = @src1;
    };
  if(((AA == 3) && ((ZZ == 0) || (ZZ == 3))))
    {
      address = (@src1 + (@src2 << 2));
    };
  if(((AA == 3) && (ZZ == 2)))
    {
      address = (@src1 + (@src2 << 1));
    };
  setMemory (address, ZZ, @dest);
  if(((AA == 1) || (AA == 2)))
    {
      @src1 = (@src1 + @src2);
    };
}
 */

int
arc_gen_STL (DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
  int ret = DISAS_NEXT;
  int AA;
  int ZZ;
  TCGv address = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  AA = getAAFlag ();
  ZZ = getZZFlag ();
  tcg_gen_movi_tl(address, 0);
  if (((AA == 0) || (AA == 1)))
    {
    tcg_gen_add_tl(address, src1, src2);
;
    }
  else
    {
  ;
    }
  if ((AA == 2))
    {
    tcg_gen_mov_tl(address, src1);
;
    }
  else
    {
  ;
    }
  if (((AA == 3) && ((ZZ == 0) || (ZZ == 3))))
    {
    tcg_gen_shli_tl(temp_1, src2, 3);
  tcg_gen_add_tl(address, src1, temp_1);
;
    }
  else
    {
  ;
    }
  if (((AA == 3) && (ZZ == 2)))
    {
    tcg_gen_shli_tl(temp_2, src2, 1);
  tcg_gen_add_tl(address, src1, temp_2);
;
    }
  else
    {
  ;
    }
  setMemory(address, ZZ, dest);
  if (((AA == 1) || (AA == 2)))
    {
    tcg_gen_add_tl(src1, src1, src2);
;
    }
  else
    {
  ;
    }
  tcg_temp_free(address);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_2);

  return ret;
}





/* POPL
 *    Variables: @dest
 *    Functions: getMemory, getRegister, setRegister
--- code ---
{
  new_dest = getMemory (getRegister (R_SP), LONG);
  setRegister (R_SP, (getRegister (R_SP) + 4));
  @dest = new_dest;
}
 */

int
arc_gen_POPL (DisasCtxt *ctx, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv new_dest = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  getRegister(temp_3, R_SP);
  tcg_gen_mov_tl(temp_2, temp_3);
  getMemory(temp_1, temp_2, LONGLONG);
  tcg_gen_mov_tl(new_dest, temp_1);
  getRegister(temp_6, R_SP);
  tcg_gen_mov_tl(temp_5, temp_6);
  tcg_gen_addi_tl(temp_4, temp_5, 8);
  setRegister(R_SP, temp_4);
  tcg_gen_mov_tl(dest, new_dest);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_1);
  tcg_temp_free(new_dest);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* PUSHL
 *    Variables: @src
 *    Functions: setMemory, getRegister, setRegister
--- code ---
{
  local_src = @src;
  setMemory ((getRegister (R_SP) - 8), LONG, local_src);
  setRegister (R_SP, (getRegister (R_SP) - 8));
}
 */

int
arc_gen_PUSHL (DisasCtxt *ctx, TCGv src)
{
  int ret = DISAS_NEXT;
  TCGv local_src = tcg_temp_local_new();
  TCGv temp_3 = tcg_temp_local_new();
  TCGv temp_2 = tcg_temp_local_new();
  TCGv temp_1 = tcg_temp_local_new();
  TCGv temp_6 = tcg_temp_local_new();
  TCGv temp_5 = tcg_temp_local_new();
  TCGv temp_4 = tcg_temp_local_new();
  tcg_gen_mov_tl(local_src, src);
  getRegister(temp_3, R_SP);
  tcg_gen_mov_tl(temp_2, temp_3);
  tcg_gen_subi_tl(temp_1, temp_2, 8);
  setMemory(temp_1, LONGLONG, local_src);
  getRegister(temp_6, R_SP);
  tcg_gen_mov_tl(temp_5, temp_6);
  tcg_gen_subi_tl(temp_4, temp_5, 8);
  setRegister(R_SP, temp_4);
  tcg_temp_free(local_src);
  tcg_temp_free(temp_3);
  tcg_temp_free(temp_2);
  tcg_temp_free(temp_1);
  tcg_temp_free(temp_6);
  tcg_temp_free(temp_5);
  tcg_temp_free(temp_4);

  return ret;
}





/* NORML
 *    Variables: @src, @dest
 *    Functions: HELPER, getFFlag, setZFlag, setNFlag
--- code ---
{
  psrc = @src;
  i = HELPER (norml, psrc);
  @dest = (63 - i);
  if((getFFlag () == true))
    {
      setZFlag (psrc);
      setNFlag (psrc);
    };
}
 */

int
arc_gen_NORML (DisasCtxt *ctx, TCGv src, TCGv dest)
{
  int ret = DISAS_NEXT;
  TCGv psrc = tcg_temp_local_new();
  TCGv i = tcg_temp_local_new();
  tcg_gen_mov_tl(psrc, src);
  ARC_HELPER(norml, i, psrc);
  tcg_gen_subfi_tl(dest, 63, i);
  if ((getFFlag () == true))
    {
    setZFlag(psrc);
  setNFlag(psrc);
;
    }
  else
    {
  ;
    }
  tcg_temp_free(psrc);
  tcg_temp_free(i);

  return ret;
}





/*
 * FLSL
 *    Variables: @src, @dest
 *    Functions: CLZ, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   psrc = @src;
 *   if((psrc == 0))
 *     {
 *       @dest = 0;
 *     }
 *   else
 *     {
 *       @dest = 63 - CLZ (psrc, 64);
 *     };
 *   if((getFFlag () == true))
 *     {
 *       setZFlag (psrc);
 *       setNFlag (psrc);
 *     };
 * }
 */

int
arc_gen_FLSL(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv psrc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_5 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    tcg_gen_mov_tl(psrc, src);
    TCGLabel *else_1 = gen_new_label();
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_1, psrc, 0);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_1);
    tcg_gen_movi_tl(dest, 0);
    tcg_gen_br(done_1);
    gen_set_label(else_1);
    tcg_gen_movi_tl(temp_5, 64);
    tcg_gen_clz_tl(temp_4, psrc, temp_5);
    tcg_gen_mov_tl(temp_3, temp_4);
    tcg_gen_subfi_tl(dest, 63, temp_3);
    gen_set_label(done_1);
    if ((getFFlag () == true)) {
        setZFlag(psrc);
        setNFlag(psrc);
    }
    tcg_temp_free(psrc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_5);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_3);

    return ret;
}





/*
 * FFSL
 *    Variables: @src, @dest
 *    Functions: CTZ, getFFlag, setZFlag, setNFlag
 * --- code ---
 * {
 *   psrc = @src;
 *   if((psrc == 0))
 *     {
 *       @dest = 63;
 *     }
 *   else
 *     {
 *       @dest = CTZ (psrc, 64);
 *     };
 *   if((getFFlag () == true))
 *     {
 *       setZFlag (psrc);
 *       setNFlag (psrc);
 *     };
 * }
 */

int
arc_gen_FFSL(DisasCtxt *ctx, TCGv src, TCGv dest)
{
    int ret = DISAS_NEXT;
    TCGv psrc = tcg_temp_local_new();
    TCGv temp_1 = tcg_temp_local_new();
    TCGv temp_2 = tcg_temp_local_new();
    TCGv temp_4 = tcg_temp_local_new();
    TCGv temp_3 = tcg_temp_local_new();
    tcg_gen_mov_tl(psrc, src);
    TCGLabel *else_1 = gen_new_label();
    TCGLabel *done_1 = gen_new_label();
    tcg_gen_setcondi_tl(TCG_COND_EQ, temp_1, psrc, 0);
    tcg_gen_xori_tl(temp_2, temp_1, 1);
    tcg_gen_andi_tl(temp_2, temp_2, 1);
    tcg_gen_brcond_tl(TCG_COND_EQ, temp_2, arc_true, else_1);
    tcg_gen_movi_tl(dest, 63);
    tcg_gen_br(done_1);
    gen_set_label(else_1);
    tcg_gen_movi_tl(temp_4, 64);
    tcg_gen_ctz_tl(temp_3, psrc, temp_4);
    tcg_gen_mov_tl(dest, temp_3);
    gen_set_label(done_1);
    if ((getFFlag () == true)) {
        setZFlag(psrc);
        setNFlag(psrc);
    }
    tcg_temp_free(psrc);
    tcg_temp_free(temp_1);
    tcg_temp_free(temp_2);
    tcg_temp_free(temp_4);
    tcg_temp_free(temp_3);

    return ret;
}





/*
 * DBNZ
 * --- code ---
 * target = cpl + offset
 *
 * @a = @a - 1
 * if (@a != 0)
 *   gen_branchi(target)
 */
int
arc_gen_DBNZ(DisasCtxt *ctx, TCGv a, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[1].value;
    TCGLabel *do_not_branch = gen_new_label();

    update_delay_flag(ctx);

    /* if (--a != 0) */
    tcg_gen_subi_tl(a, a, 1);
    tcg_gen_brcondi_tl(TCG_COND_EQ, a, 0, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);

    return DISAS_NORETURN;
}

/*
 * BBIT0L
 * --- code ---
 * target = cpl + offset
 *
 * _c = @c & 63
 * msk = 1 << _c
 * bit = @b & msk
 * if (bit == 0)
 *   gen_branchi(target)
 */
int
arc_gen_BBIT0L(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv _c = tcg_temp_new();
    TCGv msk = tcg_const_tl(1);
    TCGv bit = tcg_temp_new();

    update_delay_flag(ctx);

    /* if ((b & (1 << (c & 63))) == 0) */
    tcg_gen_andi_tl(_c, c, 63);
    tcg_gen_shl_tl(msk, msk, _c);
    tcg_gen_and_tl(bit, b, msk);
    tcg_gen_brcondi_tl(TCG_COND_NE, bit, 0, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(bit);
    tcg_temp_free(msk);
    tcg_temp_free(_c);

    return DISAS_NORETURN;
}


/*
 * BBIT1L
 * --- code ---
 * target = cpl + offset
 *
 * _c = @c & 63
 * msk = 1 << _c
 * bit = @b & msk
 * if (bit != 0)
 *   gen_branchi(target)
 */
int
arc_gen_BBIT1L(DisasCtxt *ctx, TCGv b, TCGv c, TCGv offset ATTRIBUTE_UNUSED)
{
    const target_ulong target = ctx->pcl + ctx->insn.operands[2].value;
    TCGLabel *do_not_branch = gen_new_label();
    TCGv _c = tcg_temp_new();
    TCGv msk = tcg_const_tl(1);
    TCGv bit = tcg_temp_new();

    update_delay_flag(ctx);

    /* if ((b & (1 << (c & 63))) != 0) */
    tcg_gen_andi_tl(_c, c, 63);
    tcg_gen_shl_tl(msk, msk, _c);
    tcg_gen_and_tl(bit, b, msk);
    tcg_gen_brcondi_tl(TCG_COND_EQ, bit, 0, do_not_branch);

    gen_branchi(ctx, target);

    gen_set_label(do_not_branch);
    tcg_temp_free(bit);
    tcg_temp_free(msk);
    tcg_temp_free(_c);

    return DISAS_NORETURN;
}

int
arc_gen_VPACK4HL(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  TCGv b_h0 = tcg_temp_new();
  TCGv b_h2 = tcg_temp_new();
  TCGv c_h0 = tcg_temp_new();
  TCGv c_h2 = tcg_temp_new();

  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  /* Conditional execution */
  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  /* Instruction code */

  tcg_gen_sextract_tl(b_h0, b, 0, 16);
  tcg_gen_sextract_tl(b_h2, b, 32, 16);
  tcg_gen_sextract_tl(c_h0, c, 0, 16);
  tcg_gen_sextract_tl(c_h2, c, 32, 16);

  tcg_gen_mov_tl(a, b_h0);
  tcg_gen_deposit_tl(a, a, b_h2, 16, 16);
  tcg_gen_deposit_tl(a, a, c_h0, 32, 16);
  tcg_gen_deposit_tl(a, a, c_h2, 48, 16);

  /* Conditional execution end. */
  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  tcg_temp_free(b_h0);
  tcg_temp_free(b_h2);
  tcg_temp_free(c_h0);
  tcg_temp_free(c_h2);

  return DISAS_NEXT;
}

int
arc_gen_VPACK4HM(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  TCGv b_h1 = tcg_temp_new();
  TCGv b_h3 = tcg_temp_new();
  TCGv c_h1 = tcg_temp_new();
  TCGv c_h3 = tcg_temp_new();

  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  /* Conditional execution */
  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  /* Instruction code */

  tcg_gen_sextract_tl(b_h1, b, 16, 16);
  tcg_gen_sextract_tl(b_h3, b, 48, 16);
  tcg_gen_sextract_tl(c_h1, c, 16, 16);
  tcg_gen_sextract_tl(c_h3, c, 48, 16);

  tcg_gen_mov_tl(a, b_h1);
  tcg_gen_deposit_tl(a, a, b_h3, 16, 16);
  tcg_gen_deposit_tl(a, a, c_h1, 32, 16);
  tcg_gen_deposit_tl(a, a, c_h3, 48, 16);

  /* Conditional execution end. */
  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  tcg_temp_free(b_h1);
  tcg_temp_free(b_h3);
  tcg_temp_free(c_h1);
  tcg_temp_free(c_h3);

  return DISAS_NEXT;
}

int
arc_gen_VPACK2WL(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  TCGv b_w0 = tcg_temp_new();
  TCGv c_w0 = tcg_temp_new();

  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  /* Conditional execution */
  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  /* Instruction code */

  tcg_gen_sextract_tl(b_w0, b, 0, 32);
  tcg_gen_sextract_tl(c_w0, c, 0, 32);

  tcg_gen_mov_tl(a, b_w0);
  tcg_gen_deposit_tl(a, a, c_w0, 32, 32);

  /* Conditional execution end. */
  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  tcg_temp_free(b_w0);
  tcg_temp_free(c_w0);

  return DISAS_NEXT;
}

int
arc_gen_VPACK2WM(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  TCGv b_w1 = tcg_temp_new();
  TCGv c_w1 = tcg_temp_new();

  TCGv cc_temp = tcg_temp_local_new();
  TCGLabel *cc_done = gen_new_label();

  /* Conditional execution */
  getCCFlag(cc_temp);
  tcg_gen_brcondi_tl(TCG_COND_EQ, cc_temp, 0, cc_done);

  /* Instruction code */

  tcg_gen_sextract_tl(b_w1, b, 32, 32);
  tcg_gen_sextract_tl(c_w1, c, 32, 32);

  tcg_gen_mov_tl(a, b_w1);
  tcg_gen_deposit_tl(a, a, c_w1, 32, 32);

  /* Conditional execution end. */
  gen_set_label(cc_done);
  tcg_temp_free(cc_temp);

  tcg_temp_free(b_w1);
  tcg_temp_free(c_w1);

  return DISAS_NEXT;
}

#define VEC_ADD16_SUB16_I64_W0(NAME, OP)                 \
static void                                              \
arc_gen_vec_##NAME##16_i64_w0(TCGv dest, TCGv b, TCGv c) \
{                                                        \
  TCGv t1 = tcg_temp_new();                              \
                                                         \
  OP(t1, b, c);                                          \
  tcg_gen_deposit_i64(dest, dest, t1, 0, 32);            \
                                                         \
  tcg_temp_free(t1);                                     \
}

VEC_ADD16_SUB16_I64_W0(add, tcg_gen_vec_add16_i64)
VEC_ADD16_SUB16_I64_W0(sub, tcg_gen_vec_sub16_i64)

#define VEC_ADD_SUB(INSN, OP, FIELD_SIZE)                     \
int                                                           \
arc_gen_##INSN(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)     \
{                                                             \
    ARC_GEN_SEMFUNC_INIT();                                   \
                                                              \
    ARC_GEN_VEC_FIRST_OPERAND(operand_##FIELD_SIZE, i64, b);  \
    ARC_GEN_VEC_SECOND_OPERAND(operand_##FIELD_SIZE, i64, c); \
                                                              \
    /* Instruction code */                                    \
    OP(dest, b, c);                                           \
                                                              \
    ARC_GEN_SEMFUNC_DEINIT();                                 \
    return DISAS_NEXT;                                        \
}

VEC_ADD_SUB(VADD2,  tcg_gen_vec_add32_i64,    32bit)
VEC_ADD_SUB(VADD2H, arc_gen_vec_add16_i64_w0, 16bit)
VEC_ADD_SUB(VADD4H, tcg_gen_vec_add16_i64,    16bit)
VEC_ADD_SUB(VSUB2,  tcg_gen_vec_sub32_i64,    32bit)
VEC_ADD_SUB(VSUB2H, arc_gen_vec_sub16_i64_w0, 16bit)
VEC_ADD_SUB(VSUB4H, tcg_gen_vec_sub16_i64,    16bit)



static void
arc_gen_vmac2_op(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c,
                 void (*OP)(TCGv, TCGv, unsigned int, unsigned int))
{
  TCGv t1 = tcg_temp_new();
  TCGv t2 = tcg_temp_new();
  TCGv t3 = tcg_temp_new();
  TCGv t4 = tcg_temp_new();

  OP(t1, b, 0, 16);                           /* t1 = b.h0 */
  OP(t2, c, 0, 16);                           /* t2 = c.h0 */
  tcg_gen_mul_tl(t3, t1, t2);                 /* t3 = b.h0 * c.h0 */
  tcg_gen_extract_tl(t2, cpu64_acc, 0, 32);   /* t2 = acclo */
  tcg_gen_add_tl(t3, t3, t2);                 /* t3 = (b.h0 * c.h0) + acclo */

  OP(t1, b, 16, 16);                          /* t1 = b.h1 */
  OP(t2, c, 16, 16);                          /* t2 = c.h1 */
  tcg_gen_mul_tl(t4, t1, t2);                 /* t4 = b.h1 * c.h1 */
  tcg_gen_extract_tl(t2, cpu64_acc, 32, 32);  /* t2 = acchi */
  tcg_gen_add_tl(t4, t4, t2);                 /* t4 = (b.h1 * c.h1) + acchi */

  tcg_gen_deposit_tl(dest, dest, t3, 0, 32);  /* dest.w0 = (b.h0 * c.h0) + acclo */
  tcg_gen_deposit_tl(dest, dest, t4, 32, 32); /* dest.w1 = (b.h1 * c.h1) + acchi */
  tcg_gen_mov_tl(cpu64_acc, dest);            /* acc = dest */

  tcg_temp_free(t1);
  tcg_temp_free(t2);
  tcg_temp_free(t3);
  tcg_temp_free(t4);
}

#define ARC_GEN_VEC_MAC2H(INSN, OP, FIELD_SIZE)               \
int                                                           \
arc_gen_##INSN(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)     \
{                                                             \
    ARC_GEN_SEMFUNC_INIT();                                   \
                                                              \
    ARC_GEN_VEC_FIRST_OPERAND(operand_##FIELD_SIZE, i64, b);  \
    ARC_GEN_VEC_SECOND_OPERAND(operand_##FIELD_SIZE, i64, c); \
                                                              \
    arc_gen_vmac2_op(ctx, dest, b, c, OP);                    \
                                                              \
    ARC_GEN_SEMFUNC_DEINIT();                                 \
                                                              \
    return DISAS_NEXT;                                        \
}

ARC_GEN_VEC_MAC2H(VMAC2H, tcg_gen_sextract_tl, 16bit)
ARC_GEN_VEC_MAC2H(VMAC2HU, tcg_gen_extract_tl, 16bit)

#define ARC_GEN_VEC_ADD_SUB(INSN, FIELD, OP, FIELD_SIZE)      \
int                                                           \
arc_gen_##INSN(DisasCtxt *ctx, TCGv dest, TCGv b, TCGv c)     \
{                                                             \
    TCGv t1;                                                  \
    ARC_GEN_SEMFUNC_INIT();                                   \
                                                              \
    ARC_GEN_VEC_FIRST_OPERAND(operand_##FIELD_SIZE, i64, b);  \
    ARC_GEN_VEC_SECOND_OPERAND(operand_##FIELD_SIZE, i64, c); \
                                                              \
    t1 = tcg_temp_new();                                      \
    ARC_GEN_CMPL2_##FIELD##_I64(t1, c);                       \
    OP(dest, b, t1);                                          \
    tcg_temp_free(t1);                                        \
                                                              \
    ARC_GEN_SEMFUNC_DEINIT();                                 \
                                                              \
    return DISAS_NEXT;                                        \
}


ARC_GEN_VEC_ADD_SUB(VADDSUB, W1, tcg_gen_vec_add32_i64,      32bit)
ARC_GEN_VEC_ADD_SUB(VADDSUB2H, H1, arc_gen_vec_add16_w0_i64, 16bit)
ARC_GEN_VEC_ADD_SUB(VADDSUB4H, H1_H3, tcg_gen_vec_add16_i64, 16bit)
ARC_GEN_VEC_ADD_SUB(VSUBADD, W0, tcg_gen_vec_add32_i64,      32bit)
ARC_GEN_VEC_ADD_SUB(VSUBADD2H, H0, arc_gen_vec_add16_w0_i64, 16bit)
ARC_GEN_VEC_ADD_SUB(VSUBADD4H, H0_H2, tcg_gen_vec_add16_i64, 16bit)

int
arc_gen_QMACH(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_qmach_base_i64(ctx, a, b, c, cpu64_acc, true, \
                           tcg_gen_sextract_i64, \
                           arc_gen_add_signed_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

int
arc_gen_QMACHU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_qmach_base_i64(ctx, a, b, c, cpu64_acc, false, \
                           tcg_gen_extract_i64, \
                           arc_gen_add_unsigned_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

int
arc_gen_DMACWH(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_dmacwh_base_i64(ctx, a, b, c, cpu64_acc, true, \
                            tcg_gen_sextract_i64, \
                            arc_gen_add_signed_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

int
arc_gen_DMACWHU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_dmacwh_base_i64(ctx, a, b, c, cpu64_acc, false, \
                            tcg_gen_extract_i64, \
                            arc_gen_add_unsigned_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

int
arc_gen_DMACH(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  ARC_GEN_SEMFUNC_INIT();

  arc_gen_dmach_base_i64(ctx, a, b, c, cpu64_acc, true, \
                         tcg_gen_sextract_i64, \
                         arc_gen_add_signed_overflow_i64);

  ARC_GEN_SEMFUNC_DEINIT();

  return DISAS_NEXT;
}

int
arc_gen_DMACHU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
  ARC_GEN_SEMFUNC_INIT();

  arc_gen_dmach_base_i64(ctx, a, b, c, cpu64_acc, false, \
                         tcg_gen_extract_i64, \
                         arc_gen_add_unsigned_overflow_i64);

  ARC_GEN_SEMFUNC_DEINIT();

  return DISAS_NEXT;
}

int
arc_gen_DMPYH(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_dmpyh_base_i64(ctx, a, b, c, cpu64_acc, true, \
                            tcg_gen_sextract_i64, \
                            arc_gen_add_signed_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

int
arc_gen_DMPYHU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_dmpyh_base_i64(ctx, a, b, c, cpu64_acc, false, \
                            tcg_gen_extract_i64, \
                            arc_gen_add_unsigned_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

int
arc_gen_QMPYH(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_qmpyh_base_i64(ctx, a, b, c, cpu64_acc, true, \
                            tcg_gen_sextract_i64, \
                            arc_gen_add_signed_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

int
arc_gen_QMPYHU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_qmpyh_base_i64(ctx, a, b, c, cpu64_acc, false, \
                            tcg_gen_extract_i64, \
                            arc_gen_add_unsigned_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

int
arc_gen_DMPYWH(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_dmpywh_base_i64(ctx, a, b, c, cpu64_acc, true, \
                            tcg_gen_sextract_i64, \
                            arc_gen_add_signed_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

int
arc_gen_DMPYWHU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_dmpywh_base_i64(ctx, a, b, c, cpu64_acc, false, \
                            tcg_gen_extract_i64, \
                            arc_gen_add_unsigned_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}


/*
 * Compare the 32-bit signed vector elements of the 64-bit operands b and c.
 * The minimum value of the two elements is stored in the corresponding
 * element of the destination operand a.
 * This instruction does not update any STATUS32 flags.
 */
int
arc_gen_VMIN2(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    TCGv_i64 b_h0 = tcg_temp_new_i64();
    TCGv_i64 b_h1 = tcg_temp_new_i64();

    TCGv_i64 c_h0 = tcg_temp_new_i64();
    TCGv_i64 c_h1 = tcg_temp_new_i64();

    /* Instruction code */

    ARC_GEN_VEC_FIRST_OPERAND(operand_32bit, i64, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_32bit, i64, c);

    tcg_gen_sextract_i64(b_h0, b, 0, 32);
    tcg_gen_sextract_i64(b_h1, b, 32, 32);

    tcg_gen_sextract_i64(c_h0, c, 0, 32);
    tcg_gen_sextract_i64(c_h1, c, 32, 32);

    tcg_gen_smin_i64(b_h0, b_h0, c_h0);
    tcg_gen_smin_i64(b_h1, b_h1, c_h1);

    /* Assemble final result */

    tcg_gen_extract_i64(a, b_h0, 0, 32);
    tcg_gen_shli_i64(b_h1, b_h1, 32);
    tcg_gen_or_i64(a, a, b_h1);

    tcg_temp_free_i64(b_h0);
    tcg_temp_free_i64(b_h1);

    tcg_temp_free_i64(c_h0);
    tcg_temp_free_i64(c_h1);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * Compare the 32-bit signed vector elements of the 64-bit operands b and c.
 * The maximum value of the two elements is stored in the corresponding
 * element of the destination operand a.
 * This instruction does not update any STATUS32 flags.
 */
int
arc_gen_VMAX2(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    TCGv_i64 b_h0 = tcg_temp_new_i64();
    TCGv_i64 b_h1 = tcg_temp_new_i64();

    TCGv_i64 c_h0 = tcg_temp_new_i64();
    TCGv_i64 c_h1 = tcg_temp_new_i64();

    /* Instruction code */

    ARC_GEN_VEC_FIRST_OPERAND(operand_32bit, i64, b);
    ARC_GEN_VEC_SECOND_OPERAND(operand_32bit, i64, c);

    tcg_gen_sextract_i64(b_h0, b, 0, 32);
    tcg_gen_sextract_i64(b_h1, b, 32, 32);

    tcg_gen_sextract_i64(c_h0, c, 0, 32);
    tcg_gen_sextract_i64(c_h1, c, 32, 32);

    tcg_gen_smax_i64(b_h0, b_h0, c_h0);
    tcg_gen_smax_i64(b_h1, b_h1, c_h1);

    /* Assemble final result */

    tcg_gen_extract_i64(a, b_h0, 0, 32);
    tcg_gen_shli_i64(b_h1, b_h1, 32);
    tcg_gen_or_i64(a, a, b_h1);

    tcg_temp_free_i64(b_h0);
    tcg_temp_free_i64(b_h1);

    tcg_temp_free_i64(c_h0);
    tcg_temp_free_i64(c_h1);


    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}


int
arc_gen_VMPY2H(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_vmpy2h_base_i64(ctx, a, b, c, cpu64_acc, true, \
                            tcg_gen_sextract_i64, \
                            arc_gen_add_signed_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

int
arc_gen_VMPY2HU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_vmpy2h_base_i64(ctx, a, b, c, cpu64_acc, false, \
                            tcg_gen_extract_i64, \
                            arc_gen_add_unsigned_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

int
arc_gen_MPYD(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_mpyd_base_i64(ctx, a, b, c, cpu64_acc, true, \
                          tcg_gen_sextract_i64, \
                          arc_gen_add_signed_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

int
arc_gen_MPYDU(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)
{
    ARC_GEN_SEMFUNC_INIT();

    arc_gen_mpyd_base_i64(ctx, a, b, c, cpu64_acc, false, \
                          tcg_gen_extract_i64, \
                          arc_gen_add_unsigned_overflow_i64);

    ARC_GEN_SEMFUNC_DEINIT();

    return DISAS_NEXT;
}

/*
 * ATLD
 *    Variables: @b, @c
 *    Functions: arc_gen_atld_op
 * --- code ---
 * {
 *   b_32 = extract_32_from_64(b)
 *   op = arc_gen_atld_op(b_32, c)
 *   if (IsSigned(op)) {
 *     signed_extract(b, b_32)
 *   } else {
 *     unsigned_extract(b, b_32)
 *   }
 * }
 */
int
arc_gen_ATLD(DisasCtxt *ctx, TCGv b, TCGv c)
{

    TCGv_i32 b_32 = tcg_temp_new_i32();
    tcg_gen_extrl_i64_i32(b_32, b);

    MemOp mop = arc_gen_atld_op(ctx, b_32, c);

    if(mop & MO_SIGN) {
        tcg_gen_ext_i32_i64(b, b_32);
    } else {
        tcg_gen_extu_i32_i64(b, b_32);
    }

    tcg_temp_free_i32(b_32);

    return DISAS_NEXT;
}

/*
 * ATLDL
 *    Variables: @b, @c
 * --- code ---
 * {
 *   operations = [add, or, and, xor, umin, umax, min, max]
 *   operation = operations[instruction.op]
 *
 *   if (instruction.aq_rl_flag) {
 *     DMB(ALL_OP_TYPES | FORWARDS_AND_BACKWARDS)
 *   }
 *
 *   operation(b, c)
 *
 *   if (instruction.aq_rl_flag) {
 *     DMB(ALL_OP_TYPES | FORWARDS_AND_BACKWARDS)
 *   }
 * }
 */
int
arc_gen_ATLDL(DisasCtxt *ctx, TCGv b, TCGv c)
{
    void (*atomic_fn)(TCGv_i64, TCGv, TCGv_i64, TCGArg, MemOp);

    MemOp mop = MO_64 | MO_ALIGN;
    atomic_fn = NULL;

    switch(ctx->insn.op) {
    case ATO_ADD:
        atomic_fn = tcg_gen_atomic_fetch_add_i64;
        break;

    case ATO_OR:
        atomic_fn = tcg_gen_atomic_fetch_or_i64;
        break;

    case ATO_AND:
        atomic_fn = tcg_gen_atomic_fetch_and_i64;
        break;

    case ATO_XOR:
        atomic_fn = tcg_gen_atomic_fetch_xor_i64;
        break;

    case ATO_MINU:
        atomic_fn = tcg_gen_atomic_fetch_umin_i64;
        break;

    case ATO_MAXU:
        atomic_fn = tcg_gen_atomic_fetch_umax_i64;
        break;

    case ATO_MIN:
        atomic_fn = tcg_gen_atomic_fetch_smin_i64;
        mop |= MO_SIGN;
        break;

    case ATO_MAX:
        atomic_fn = tcg_gen_atomic_fetch_smax_i64;
        mop |= MO_SIGN;
        break;

    default:
        assert("Invalid atldl operation");
        break;
    }

    if (ctx->insn.aq) {
        tcg_gen_mb(TCG_BAR_SC | TCG_MO_ALL);
    }

    atomic_fn(b, c, b, ctx->mem_idx, mop);

    if (ctx->insn.rl) {
        tcg_gen_mb(TCG_BAR_SC | TCG_MO_ALL);
    }

    return DISAS_NEXT;
}

/* Floating point instructions */

/*
 * FMVL2D, FMVD2L, FDMOV, FMVI2S, FMVS2I, FSMOV, FHMOV
 *    Variables: @a, @b
 *    Functions: getCCFlag
 * --- code ---
 * {
 *   if((getCCFlag () == true))
 *     {
 *       a <= b & (( 1 << OP_SIZE) - 1)
 *     };
 * }
 */
#define FLOAT_MV_DIRECT(NAME, SIZE)                               \
inline int arc_gen_##NAME(DisasCtxt *ctx, TCGv a, TCGv b)         \
{                                                                 \
    int ret = DISAS_NEXT;                                         \
    TCGv cc_flag = tcg_temp_new();                                \
    getCCFlag(cc_flag);                                           \
    TCGLabel *done_1 = gen_new_label();                           \
    tcg_gen_brcond_tl(TCG_COND_NE, cc_flag, arc_true, done_1);;   \
    tcg_gen_andi_tl(a, b, SIZE < 64 ? (1ull << SIZE) - 1 : -1ll); \
    gen_set_label(done_1);                                        \
    tcg_temp_free(cc_flag);                                       \
    return ret;                                                   \
}

FLOAT_MV_DIRECT(FMVL2D, 64)
FLOAT_MV_DIRECT(FMVD2L, 64)
FLOAT_MV_DIRECT(FDMOV, 64)

FLOAT_MV_DIRECT(FMVI2S, 32)
FLOAT_MV_DIRECT(FMVS2I, 32)
FLOAT_MV_DIRECT(FSMOV, 32)

FLOAT_MV_DIRECT(FHMOV, 16)

/*
 * Sets up memory `address` according to AA and ZZ flags, before the
 * operation takes place.
 * ZZ can be any of the following: 2: byte, 1: half-word, 0: word
 */
static void
arc_gen_ldst_pre(DisasCtxt *ctx, TCGv address, TCGv src1, TCGv src2, int ZZ)
{
    int AA = getAAFlag ();
    tcg_gen_movi_tl(address, 0);
    /* No write back || .AW, address = Reg + Offset */
    if (((AA == 0) || (AA == 1))) {
        tcg_gen_add_tl(address, src1, src2);
    }

    /* .AB, address = Reg */
    if ((AA == 2)) {
        tcg_gen_mov_tl(address, src1);
    }

    /* .AS, address = Reg + (offset << ZZ) */
    if (AA == 3) {
        if (ZZ == 0) {
            tcg_gen_shli_tl(address, src2, 2);
            tcg_gen_add_tl(address, src1, address);
        }

        if (ZZ == 1 || ZZ == 3) {
            tcg_gen_shli_tl(address, src2, 3);
            tcg_gen_add_tl(address, src1, address);
        }

        if (ZZ == 2) {
            tcg_gen_shli_tl(address, src2, 1);
            tcg_gen_add_tl(address, src1, address);
        }
    }
}

/*
 * Sets up memory address according to AA and ZZ flags, after the
 * operation takes place
 */
static void
arc_gen_ldst_post(DisasCtxt *ctx, TCGv dest, TCGv src1, TCGv src2)
{
    int AA = getAAFlag ();
    /* .AW || .AB write address back */
    if (((AA == 1) || (AA == 2))) {
        tcg_gen_add_tl(dest, src1, src2);
    }
}

/*
 * FLD16
 *    Variables: @src1, @src2, @dest
 *    Functions: arc_gen_get_memory
 * --- code ---
 * {
 *   addr = shift_scale(src1, src2)
 *   dest <= mem(addr)[0:16]
 * }
 */
int
arc_gen_FLD16(DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
    int ret = DISAS_NEXT;
    int ZZ = 2; /* 16bit */
    TCGv address = tcg_temp_local_new();
    TCGv l_src1 = tcg_temp_local_new();
    TCGv l_src2 = tcg_temp_local_new();
    TCGv new_dest = tcg_temp_local_new();

    arc_gen_ldst_pre(ctx, address, src1, src2, ZZ);

    tcg_gen_mov_tl(l_src1, src1);
    tcg_gen_mov_tl(l_src2, src2);

    getMemory(new_dest, address, ZZ);

    arc_gen_ldst_post(ctx, src1, l_src1, l_src2);

    tcg_gen_mov_tl(dest, new_dest);

    tcg_temp_free(address);
    tcg_temp_free(l_src1);
    tcg_temp_free(l_src2);
    tcg_temp_free(new_dest);

    return ret;
}

/*
 * FST16
 *    Variables: @src1, @src2, @dest
 *    Functions: arc_gen_set_memory
 * --- code ---
 * {
 *   addr = shift_scale(src1, src2)
 *   mem(addr) <= dest[0:16]
 * }
 */
int
arc_gen_FST16(DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
    int ret = DISAS_NEXT;
    int ZZ = 2; /* 32bit */
    TCGv address = tcg_temp_local_new();
    TCGv l_src1 = tcg_temp_local_new();
    TCGv l_src2 = tcg_temp_local_new();

    arc_gen_ldst_pre(ctx, address, src1, src2, ZZ);

    tcg_gen_mov_tl(l_src1, src1);
    tcg_gen_mov_tl(l_src2, src2);

    setMemory(address, ZZ, dest);

    arc_gen_ldst_post(ctx, src1, l_src1, l_src2);

    tcg_temp_free(address);
    tcg_temp_free(l_src1);
    tcg_temp_free(l_src2);

    return ret;
}

/*
 * FLD32
 *    Variables: @src1, @src2, @dest
 *    Functions: arc_gen_get_memory
 * --- code ---
 * {
 *   addr = shift_scale(src1, src2)
 *   dest <= mem(addr)[0:32]
 * }
 */
int
arc_gen_FLD32(DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
    int ret = DISAS_NEXT;
    int ZZ = 0; /* 32bit */
    TCGv address = tcg_temp_local_new();
    TCGv l_src1 = tcg_temp_local_new();
    TCGv l_src2 = tcg_temp_local_new();
    TCGv new_dest = tcg_temp_local_new();

    arc_gen_ldst_pre(ctx, address, src1, src2, ZZ);

    tcg_gen_mov_tl(l_src1, src1);
    tcg_gen_mov_tl(l_src2, src2);

    getMemory(new_dest, address, ZZ);

    arc_gen_ldst_post(ctx, src1, l_src1, l_src2);

    tcg_gen_mov_tl(dest, new_dest);

    tcg_temp_free(address);
    tcg_temp_free(l_src1);
    tcg_temp_free(l_src2);
    tcg_temp_free(new_dest);

    return ret;
}

/*
 * FST32
 *    Variables: @src1, @src2, @dest
 *    Functions: arc_gen_set_memory
 * --- code ---
 * {
 *   addr = shift_scale(src1, src2)
 *   mem(addr) <= dest[0:32]
 * }
 */
int
arc_gen_FST32(DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
    int ret = DISAS_NEXT;
    int ZZ = 0; /* 32bit */
    TCGv address = tcg_temp_local_new();
    TCGv l_src1 = tcg_temp_local_new();
    TCGv l_src2 = tcg_temp_local_new();

    arc_gen_ldst_pre(ctx, address, src1, src2, ZZ);

    tcg_gen_mov_tl(l_src1, src1);
    tcg_gen_mov_tl(l_src2, src2);

    setMemory(address, ZZ, dest);

    arc_gen_ldst_post(ctx, src1, l_src1, l_src2);

    tcg_temp_free(address);
    tcg_temp_free(l_src1);
    tcg_temp_free(l_src2);

    return ret;
}

/*
 * FLD64
 *    Variables: @src1, @src2, @dest
 *    Functions: arc_gen_get_memory
 * --- code ---
 * {
 *   addr = shift_scale(src1, src2)
 *   dest <= mem(addr)[0:64]
 * }
 */
int
arc_gen_FLD64(DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
    int ret = DISAS_NEXT;
    int ZZ = 3; /* 64 bit */
    TCGv address = tcg_temp_local_new();
    TCGv l_src1 = tcg_temp_local_new();
    TCGv l_src2 = tcg_temp_local_new();
    TCGv new_dest = tcg_temp_local_new();

    arc_gen_ldst_pre(ctx, address, src1, src2, ZZ);

    tcg_gen_mov_tl(l_src1, src1);
    tcg_gen_mov_tl(l_src2, src2);

    getMemory(new_dest, address, ZZ);

    arc_gen_ldst_post(ctx, src1, l_src1, l_src2);

    tcg_gen_mov_tl(dest, new_dest);

    tcg_temp_free(address);
    tcg_temp_free(l_src1);
    tcg_temp_free(l_src2);
    tcg_temp_free(new_dest);

    return ret;
}

/*
 * FST64
 *    Variables: @src1, @src2, @dest
 *    Functions: arc_gen_set_memory
 * --- code ---
 * {
 *   addr = shift_scale(src1, src2)
 *   mem(addr) <= dest[0:32]
 * }
 */
int
arc_gen_FST64(DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
    int ret = DISAS_NEXT;
    int ZZ = 3; /* 64 bit */
    TCGv address = tcg_temp_local_new();
    TCGv l_src1 = tcg_temp_local_new();
    TCGv l_src2 = tcg_temp_local_new();

    arc_gen_ldst_pre(ctx, address, src1, src2, ZZ);

    tcg_gen_mov_tl(l_src1, src1);
    tcg_gen_mov_tl(l_src2, src2);

    setMemory(address, ZZ, dest);

    arc_gen_ldst_post(ctx, src1, l_src1, l_src2);

    tcg_temp_free(address);
    tcg_temp_free(l_src1);
    tcg_temp_free(l_src2);

    return ret;
}

/*
 * FLDD64
 *    Variables: @src1, @src2, @dest
 *    Functions: arc_gen_get_memory, nextFPURegWithNull
 * --- code ---
 * {
 *   addr = shift_scale(src1, src2)
 *   dest <= mem(addr)[0:64]
 *   dest2 = nextFPUReg(dest)
 *   dest2 <= mem(addr + 8)[0:64]
 * }
 */
int
arc_gen_FLDD64(DisasCtxt *ctx, TCGv src1, TCGv src2, TCGv dest)
{
    int ret = DISAS_NEXT;
    int ZZ = 3; /* 64 bit */
    TCGv address = tcg_temp_local_new();
    TCGv l_src1 = tcg_temp_local_new();
    TCGv l_src2 = tcg_temp_local_new();
    TCGv new_dest = tcg_temp_local_new();
    TCGv new_dest_hi = tcg_temp_local_new();
    TCGv address_high = tcg_temp_local_new();

    arc_gen_ldst_pre(ctx, address, src1, src2, ZZ);

    tcg_gen_mov_tl(l_src1, src1);
    tcg_gen_mov_tl(l_src2, src2);

    getMemory(new_dest, address, ZZ);
    tcg_gen_addi_tl(address_high, address, 8);
    getMemory(new_dest_hi, address_high, ZZ);

    arc_gen_ldst_post(ctx, src1, l_src1, l_src2);

    tcg_gen_mov_tl(dest, new_dest);
    tcg_gen_mov_tl(nextFPURegWithNull(ctx, dest), new_dest_hi);

    tcg_temp_free(address_high);
    tcg_temp_free(address);
    tcg_temp_free(l_src1);
    tcg_temp_free(l_src2);
    tcg_temp_free(new_dest);
    tcg_temp_free(new_dest_hi);

    return ret;
}

/*
 * FSTD64
 *    Variables: @src1, @src2, @dest
 *    Functions: arc_gen_set_memory, nextFPURegWithNull
 * --- code ---
 * {
 *   addr = shift_scale(src1, src2)
 *   mem(addr) <= dest[0:64]
 *   dest2 = nextFPUReg(dest)
 *   mem(addr + 8) <= dest2[0:64]
 * }
 */
int
arc_gen_FSTD64(DisasCtxt *ctx, TCGv data_reg, TCGv dest, TCGv offset)
{
    int ret = DISAS_NEXT;
    int ZZ = 3; /* 64 bit */
    TCGv address = tcg_temp_local_new();
    TCGv l_dest = tcg_temp_local_new();
    TCGv l_offset = tcg_temp_local_new();
    TCGv address_high = tcg_temp_local_new();

    arc_gen_ldst_pre(ctx, address, dest, offset, ZZ);

    tcg_gen_mov_tl(l_dest, dest);
    tcg_gen_mov_tl(l_offset, offset);

    setMemory(address, ZZ, data_reg);
    tcg_gen_addi_tl(address_high, address, 8);
    setMemory(address_high, ZZ, nextFPURegWithNull(ctx, data_reg));

    arc_gen_ldst_post(ctx, dest, l_dest, l_offset);

    tcg_temp_free(address_high);
    tcg_temp_free(address);
    tcg_temp_free(l_dest);
    tcg_temp_free(l_offset);

    return ret;
}

/*
 * Operations: ADD/SUB
 * FDMADD  FDMSUB  : Double-precision Multiply, Operation
 * FDNMADD FDNMSUB : Negated Double-precision Multiply, Operation
 * FSMADD  FSMSUB  : Single-precision Multiply, Operation
 * FSNMADD FSNMSUB : Negated Single-precision Multiply, Operation
 * FHMADD  FHMSUB  : Half-precision Multiply, Operation
 * FHNMADD FHNMSUB : Negated Half-precision Multiply, Operation
 *    Variables: @a, @b, @c, @d
 *    Functions: gen_helper_fdmadd gen_helper_fdmsub gen_helper_fdnmadd
 *      gen_helper_fdnmsub gen_helper_fsmadd gen_helper_fsmsub
 *      gen_helper_fsnmadd gen_helper_fsnmsub gen_helper_fhmadd
 *      gen_helper_fhmsub gen_helper_fhnmadd gen_helper_fhnmsub
 * --- code ---
 * {
 *   a <= helper(b, c, d)
 * }
 */
#define FLOAT_INSTRUCTION4(NAME, HELPER)                        \
inline int                                                      \
arc_gen_##NAME(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c, TCGv d)  \
{                                                               \
    int ret = DISAS_NEXT;                                       \
    gen_helper_##HELPER(a, cpu_env, b, c, d);                   \
    return ret;                                                 \
}

FLOAT_INSTRUCTION4(FDMADD, fdmadd)
FLOAT_INSTRUCTION4(FDMSUB, fdmsub)
FLOAT_INSTRUCTION4(FDNMADD, fdnmadd)
FLOAT_INSTRUCTION4(FDNMSUB, fdnmsub)

FLOAT_INSTRUCTION4(FSMADD, fsmadd)
FLOAT_INSTRUCTION4(FSMSUB, fsmsub)
FLOAT_INSTRUCTION4(FSNMADD, fsnmadd)
FLOAT_INSTRUCTION4(FSNMSUB, fsnmsub)

FLOAT_INSTRUCTION4(FHMADD, fhmadd)
FLOAT_INSTRUCTION4(FHMSUB, fhmsub)
FLOAT_INSTRUCTION4(FHNMADD, fhnmadd)
FLOAT_INSTRUCTION4(FHNMSUB, fhnmsub)

/*
 * Operations: ADD/SUB/MUL/DIV/MIN/MAX
 * FDADD FDSUB FDMUL FDDIV FDMIN FDMAX Double-precision Operation
 * FSADD FSSUB FSMUL FSDIV FSMIN FSMAX Single-precision Operation
 * FHADD FHSUB FHMUL FHDIV FHMIN FHMAX Half-precision Operation
 *    Variables: @a, @b, @c
 *    Functions: gen_helper_fdadd gen_helper_fdsub gen_helper_fdmul
 *       gen_helper_fddiv gen_helper_fdmin gen_helper_fdmax gen_helper_fsadd
 *       gen_helper_fssub gen_helper_fsmul gen_helper_fsdiv gen_helper_fsmin
 *       gen_helper_fsmax gen_helper_fhadd gen_helper_fhsub gen_helper_fhmul
 *       gen_helper_fhdiv gen_helper_fhmin gen_helper_fhmax
 * --- code ---
 * {
 *   a <= helper(b, c)
 * }
 */



#define FLOAT_INSTRUCTION3(NAME, HELPER)                \
int                                                     \
arc_gen_##NAME(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)  \
{                                                       \
    int ret = DISAS_NEXT;                               \
    gen_helper_##HELPER(a, cpu_env, b, c);              \
    return ret;                                         \
}

FLOAT_INSTRUCTION3(FDADD, fdadd)
FLOAT_INSTRUCTION3(FDSUB, fdsub)
FLOAT_INSTRUCTION3(FDMUL, fdmul)
FLOAT_INSTRUCTION3(FDDIV, fddiv)
FLOAT_INSTRUCTION3(FDMIN, fdmin)
FLOAT_INSTRUCTION3(FDMAX, fdmax)

FLOAT_INSTRUCTION3(FSADD, fsadd)
FLOAT_INSTRUCTION3(FSSUB, fssub)
FLOAT_INSTRUCTION3(FSMUL, fsmul)
FLOAT_INSTRUCTION3(FSDIV, fsdiv)
FLOAT_INSTRUCTION3(FSMIN, fsmin)
FLOAT_INSTRUCTION3(FSMAX, fsmax)

FLOAT_INSTRUCTION3(FHADD, fhadd)
FLOAT_INSTRUCTION3(FHSUB, fhsub)
FLOAT_INSTRUCTION3(FHMUL, fhmul)
FLOAT_INSTRUCTION3(FHDIV, fhdiv)
FLOAT_INSTRUCTION3(FHMIN, fhmin)
FLOAT_INSTRUCTION3(FHMAX, fhmax)

/*
 * FDCMP  Double-Precision comparison
 * FDCMPF Double-Precision comparison - IEEE 754 flag generation
 * FSCMP  Single-Precision comparison
 * FSCMPF Single-Precision comparison - IEEE 754 flag generation
 * FHCMP  Half-Precision comparison
 * FHCMPF Half-Precision comparison - IEEE 754 flag generation
 *    Variables: @b, @c
 *    Functions: gen_helper_fdcmp gen_helper_fdcmpf gen_helper_fscmp
 *       gen_helper_fscmpf gen_helper_fhcmp gen_helper_fhcmpf
 * --- code ---
 * {
 *   helper(b, c)
 * }
 */
#define FLOAT_INSTRUCTION2(NAME, HELPER)        \
inline int                                      \
arc_gen_##NAME(DisasCtxt *ctx, TCGv b, TCGv c)  \
{                                               \
    int ret = DISAS_NEXT;                       \
    gen_helper_##HELPER(cpu_env, b, c);         \
    return ret;                                 \
}

FLOAT_INSTRUCTION2(FDCMP, fdcmp)
FLOAT_INSTRUCTION2(FDCMPF, fdcmpf)
FLOAT_INSTRUCTION2(FSCMP, fscmp)
FLOAT_INSTRUCTION2(FSCMPF, fscmpf)
FLOAT_INSTRUCTION2(FHCMP, fhcmp)
FLOAT_INSTRUCTION2(FHCMPF, fhcmpf)

/*
 * FCVT32_64
 * FINT2D, FUINT2D, FS2L, FS2L_RZ, FS2UL, FS2UL_RZ, FS2D
 * Convert between 32-bit data formats to 64-bit data formats
 *
 * FCVT64_32
 * FD2INT, FD2UINT, FL2S, FUL2S, FD2S, FD2INT_RZ, FD2UINT_RZ
 * Convert between 64-bit data formats to 32-bit data formats
 *
 * FCVT64
 * Convert between two 64-bit data formats
 * FD2L, FD2UL, FL2D, FUL2D, FD2L_RZ, FD2UL_RZ
 *
 * FCVT32
 * Convert between two 32-bit data formats
 * FS2INT, FS2UINT, FS2INT_RZ, FS2UINT_RZ, FINT2S, FUINT2S,
 * FS2H_RZ, FH2S, FS2H
 *
 * FDSQRT, FSSQRT, FHSQRT, FDRND, FDRND_RZ, FSRND, FSRND_RZ
 *
 *    Variables: @a, @b
 *    Functions: gen_helper_fint2d gen_helper_fuint2d gen_helper_fs2l
 *       gen_helper_fs2l_rz gen_helper_fs2ul gen_helper_fs2ul_rz
 *       gen_helper_fs2d gen_helper_fd2int gen_helper_fd2uint gen_helper_fl2s
 *       gen_helper_ful2s gen_helper_fd2s gen_helper_fd2int_rz
 *       gen_helper_fd2uint_rz gen_helper_fd2l gen_helper_fd2ul gen_helper_fl2d
 *       gen_helper_ful2d gen_helper_fd2l_rz gen_helper_fd2ul_rz
 *       gen_helper_fs2int gen_helper_fs2uint gen_helper_fs2int_rz
 *       gen_helper_fs2uint_rz gen_helper_fint2s gen_helper_fuint2s
 *       gen_helper_fs2h_rz gen_helper_fh2s gen_helper_fs2h gen_helper_fdsqrt
 *       gen_helper_fssqrt gen_helper_fhsqrt gen_helper_fdrnd
 *       gen_helper_fdrnd_rz gen_helper_fsrnd gen_helper_fsrnd_rz
 * --- code ---
 * {
 *   a <= helper(b)
 * }
 */
#define FLOAT_INSTRUCTION2_WRET(NAME, HELPER)   \
inline int                                      \
arc_gen_##NAME(DisasCtxt *ctx, TCGv a, TCGv b)  \
{                                               \
    int ret = DISAS_NEXT;                       \
    gen_helper_##HELPER(a, cpu_env, b);         \
    return ret;                                 \
}

FLOAT_INSTRUCTION2_WRET(FINT2D, fint2d)
FLOAT_INSTRUCTION2_WRET(FUINT2D, fuint2d)
FLOAT_INSTRUCTION2_WRET(FS2L,     fs2l)
FLOAT_INSTRUCTION2_WRET(FS2L_RZ,  fs2l_rz)
FLOAT_INSTRUCTION2_WRET(FS2UL,    fs2ul)
FLOAT_INSTRUCTION2_WRET(FS2UL_RZ, fs2ul_rz)
FLOAT_INSTRUCTION2_WRET(FS2D, fs2d)

FLOAT_INSTRUCTION2_WRET(FD2INT, fd2int)
FLOAT_INSTRUCTION2_WRET(FD2UINT, fd2uint)
FLOAT_INSTRUCTION2_WRET(FL2S,     fl2s)
FLOAT_INSTRUCTION2_WRET(FUL2S,    ful2s)
FLOAT_INSTRUCTION2_WRET(FD2S, fd2s)
FLOAT_INSTRUCTION2_WRET(FD2INT_RZ, fd2int_rz)
FLOAT_INSTRUCTION2_WRET(FD2UINT_RZ, fd2uint_rz)

FLOAT_INSTRUCTION2_WRET(FD2L, fd2l)
FLOAT_INSTRUCTION2_WRET(FD2UL, fd2ul)
FLOAT_INSTRUCTION2_WRET(FL2D, fl2d)
FLOAT_INSTRUCTION2_WRET(FUL2D, ful2d)
FLOAT_INSTRUCTION2_WRET(FD2L_RZ, fd2l_rz)
FLOAT_INSTRUCTION2_WRET(FD2UL_RZ, fd2ul_rz)



FLOAT_INSTRUCTION2_WRET(FS2INT,     fs2int)
FLOAT_INSTRUCTION2_WRET(FS2UINT,    fs2uint)
FLOAT_INSTRUCTION2_WRET(FS2INT_RZ,  fs2int_rz)
FLOAT_INSTRUCTION2_WRET(FS2UINT_RZ, fs2uint_rz)
FLOAT_INSTRUCTION2_WRET(FINT2S,     fint2s)
FLOAT_INSTRUCTION2_WRET(FUINT2S,    fuint2s)

FLOAT_INSTRUCTION2_WRET(FS2H_RZ, fs2h_rz)
FLOAT_INSTRUCTION2_WRET(FH2S, fh2s)
FLOAT_INSTRUCTION2_WRET(FS2H, fs2h)


FLOAT_INSTRUCTION2_WRET(FDSQRT, fdsqrt)
FLOAT_INSTRUCTION2_WRET(FSSQRT, fssqrt)
FLOAT_INSTRUCTION2_WRET(FHSQRT, fhsqrt)


FLOAT_INSTRUCTION2_WRET(FDRND, fdrnd)
FLOAT_INSTRUCTION2_WRET(FDRND_RZ, fdrnd_rz)
FLOAT_INSTRUCTION2_WRET(FSRND, fsrnd)
FLOAT_INSTRUCTION2_WRET(FSRND_RZ, fsrnd_rz)

/*
 * FDSGNJ FSSGNJ FHSGNJ
 *    Variables: @a, @b, @c
 * --- code ---
 * {
 *   a.{s, e, m} <= { c.s, b.e, b.m }
 * }
 */
#define SGNJ(NAME, SIZE)                                          \
int arc_gen_##NAME##SGNJ(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)  \
{                                                                 \
    int ret = DISAS_NEXT;                                         \
    TCGv l_b = tcg_temp_new();                                    \
    TCGv l_c = tcg_temp_new();                                    \
                                                                  \
    tcg_gen_andi_tl(l_c, c, (1ull << (SIZE - 1)));                \
    tcg_gen_andi_tl(l_b, b, ~(1ull << (SIZE - 1)));               \
                                                                  \
    tcg_gen_or_tl(a, l_b, l_c);                                   \
                                                                  \
    tcg_temp_free(l_b);                                           \
    tcg_temp_free(l_c);                                           \
                                                                  \
    return ret;                                                   \
}
SGNJ(FD, 64)
SGNJ(FS, 32)
SGNJ(FH, 16)

/*
 * FDSGNJN FSSGNJN FHSGNJN
 *    Variables: @a, @b, @c
 * --- code ---
 * {
 *   a.{s, e, m} <= { not(c.s), b.e, b.m }
 * }
 */
#define SGNJN(NAME, SIZE)                                          \
int arc_gen_##NAME##SGNJN(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)  \
{                                                                  \
    int ret = DISAS_NEXT;                                          \
    TCGv l_b = tcg_temp_new();                                     \
    TCGv l_c = tcg_temp_new();                                     \
                                                                   \
    tcg_gen_andi_tl(l_c, c,   (1ull << (SIZE - 1)));               \
    tcg_gen_xori_tl(l_c, l_c, (1ull << (SIZE - 1)));               \
                                                                   \
    tcg_gen_andi_tl(l_b, b, ~(1ull << (SIZE - 1)));                \
                                                                   \
    tcg_gen_or_tl(a, l_b, l_c);                                    \
                                                                   \
    tcg_temp_free(l_b);                                            \
    tcg_temp_free(l_c);                                            \
                                                                   \
    return ret;                                                    \
}
SGNJN(FD, 64)
SGNJN(FS, 32)
SGNJN(FH, 16)

/*
 * FDSGNJX FSSGNJX FHSGNJX
 *    Variables: @a, @b, @c
 * --- code ---
 * {
 *   a.{s, e, m} <= { xor(b.s, c.s), b.e, b.m}
 * }
 */
#define SGNJX(NAME, SIZE)                                          \
int arc_gen_##NAME##SGNJX(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)  \
{                                                                  \
    int ret = DISAS_NEXT;                                          \
    TCGv l_b = tcg_temp_new();                                     \
    TCGv l_c = tcg_temp_new();                                     \
                                                                   \
    tcg_gen_andi_tl(l_b, b, (1ull << (SIZE - 1)));                 \
    tcg_gen_andi_tl(l_c, c, (1ull << (SIZE - 1)));                 \
    tcg_gen_xor_tl(l_c, l_b, l_c);                                 \
                                                                   \
    tcg_gen_andi_tl(l_b, b, ~(1ull << (SIZE - 1)));                \
                                                                   \
    tcg_gen_or_tl(a, l_b, l_c);                                    \
                                                                   \
    tcg_temp_free(l_b);                                            \
    tcg_temp_free(l_c);                                            \
                                                                   \
    return ret;                                                    \
}
SGNJX(FD, 64)
SGNJX(FS, 32)
SGNJX(FH, 16)

/*
 * VFHINS VFSINS VFDINS
 * Insert half/single/double precision elements into vector at a literal
 * index or variable index
 *    Variables: @a, @b, @c
 *    Functions: gen_helper_vfins
 * --- code ---
 * {
 *  if(b < mid_index)
 *    {
 *      a <= gen_helper_vfins(a, b, c)
 *    }
 *  else
 *    {
 *      a2 = nextFPUReg(a)
 *      index = b - mid_index
 *      a2 <= gen_helper_vfins(a2, index, c)
 *    }
 * }
 */
#define VFINS(TYPE, SIZE)                                         \
int arc_gen_VF##TYPE##INS(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c) \
{                                                                 \
    int ret = DISAS_NEXT;                                         \
    int mid_index = (sizeof(target_ulong) * 8) / SIZE;            \
                                                                  \
    TCGLabel *do_next_reg = gen_new_label();                      \
    TCGLabel *did_first_reg = gen_new_label();                    \
                                                                  \
    tcg_gen_brcondi_tl(TCG_COND_GE, b, mid_index, do_next_reg);   \
                                                                  \
    TCGv size = tcg_const_tl(SIZE);                               \
    gen_helper_vfins(a, cpu_env, a, b, c, size);                  \
    tcg_temp_free(size);                                          \
                                                                  \
    tcg_gen_br(did_first_reg);                                    \
    gen_set_label(do_next_reg);                                   \
                                                                  \
    TCGv index = tcg_temp_new();                                  \
    tcg_gen_subi_tl(index, b, mid_index);                         \
                                                                  \
    TCGv reg = nextFPURegWithNull(ctx, a);                        \
    TCGv size1 = tcg_const_tl(SIZE);                              \
                                                                  \
    gen_helper_vfins(reg, cpu_env, reg, index, c, size1);         \
                                                                  \
    tcg_temp_free(index);                                         \
    tcg_temp_free(size1);                                         \
                                                                  \
    gen_set_label(did_first_reg);                                 \
                                                                  \
    return ret;                                                   \
}
VFINS(H, 16)
VFINS(S, 32)
VFINS(D, 64)

/*
 * VFHEXT VFSEXT VFDEXT
 * Extract half/single/double precision element from vector at a literal index
 * or variable index
 *    Variables: @a, @b, @c
 *    Functions: gen_helper_vfext
 * --- code ---
 * {
 *  if(c < mid_index)
 *    {
 *      a <= gen_helper_vfext(a, b, c)
 *    }
 *  else
 *    {
 *      b2 = nextFPUReg(b)
 *      index = c - mid_index
 *      a <= gen_helper_vfext(b2, index)
 *    }
 * }
 */
#define VFEXT(TYPE, SIZE)                                         \
int arc_gen_VF##TYPE##EXT(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c) \
{                                                                 \
    int ret = DISAS_NEXT;                                         \
    int mid_index = (sizeof(target_ulong) * 8) / SIZE;            \
                                                                  \
    TCGLabel *do_next_reg = gen_new_label();                      \
    TCGLabel *did_first_reg = gen_new_label();                    \
                                                                  \
    tcg_gen_brcondi_tl(TCG_COND_GE, c, mid_index, do_next_reg);   \
                                                                  \
    TCGv size = tcg_const_tl(SIZE);                               \
    gen_helper_vfext(a, cpu_env, b, c, size);                     \
    tcg_temp_free(size);                                          \
                                                                  \
    tcg_gen_br(did_first_reg);                                    \
    gen_set_label(do_next_reg);                                   \
                                                                  \
    TCGv index = tcg_temp_new();                                  \
    tcg_gen_subi_tl(index, c, mid_index);                         \
                                                                  \
    TCGv reg = nextFPUReg(ctx, b);                                \
    TCGv size1 = tcg_const_tl(SIZE);                              \
                                                                  \
    gen_helper_vfext(a, cpu_env, reg, index, size1);              \
                                                                  \
    tcg_temp_free(index);                                         \
    tcg_temp_free(size1);                                         \
                                                                  \
    gen_set_label(did_first_reg);                                 \
                                                                  \
    return ret;                                                   \
}
VFEXT(H, 16)
VFEXT(S, 32)
VFEXT(D, 64)


/*
 * VFHREP VFSREP VFDREP
 * Replicate half/single/double precision element from vector at a literal index
 * or variable index
 *    Variables: @a, @b
 *    Functions: gen_helper_vfrep
 * --- code ---
 * {
 *  a <= gen_helper_vfrep(b)
 *  if(vfp_width > 64)
 *    {
 *      a2 = nextFPUReg(a)
 *      a2 <= gen_helper_vfrep(b)
 *    }
 * }
 */
#define VFREP(TYPE, SIZE)                                 \
int arc_gen_VF##TYPE##REP(DisasCtxt *ctx, TCGv a, TCGv b) \
{                                                         \
    int ret = DISAS_NEXT;                                 \
                                                          \
    TCGv size = tcg_const_tl(SIZE);                       \
    gen_helper_vfrep(a, cpu_env, b, size);                \
                                                          \
    if (vfp_width > 64) {                                 \
        TCGv na = nextFPURegWithNull(ctx, a);             \
        gen_helper_vfrep(na, cpu_env, b, size);           \
    }                                                     \
                                                          \
    tcg_temp_free(size);                                  \
                                                          \
    return ret;                                           \
}
VFREP(H, 16)
VFREP(S, 32)
VFREP(D, 64)

/*
 * VFHMOV VFSMOV VFDMOV
 * Move contents from all the half/single/double precision elements in the
 * source vector to the destination vector
 *    Variables: @a, @b
 * --- code ---
 * {
 *  a <= b
 *  if(vfp_width > 64)
 *    {
 *      a2 = nextFPUReg(a)
 *      b2 = nextFPUReg(b)
 *      a2 <= b2
 *    }
 * }
 */
int arc_gen_VFMOV(DisasCtxt *ctx, TCGv a, TCGv b)
{
    int ret = DISAS_NEXT;

    /* For the next current register */
    tcg_gen_mov_tl(a, b);

    if (vfp_width > 64) {
        TCGv na = nextFPURegWithNull(ctx, a);
        TCGv nb = nextFPUReg(ctx, b);
        tcg_gen_mov_tl(na, nb);
    }
    return ret;
}

/*
 * VFHSQRT VFSSQRT VFDSQRT
 * Half/Single/Double precision floating point square root for all elements in a
 * vector
 *    Variables: @a, @b
 *    Functions: gen_helper_vfhsqrt gen_helper_vfssqrt gen_helper_vfdsqrt
 * --- code ---
 * {
 *  a <= helper(b)
 *  if(vfp_width > 64)
 *    {
 *      a2 = nextFPUReg(a)
 *      b2 = nextFPUReg(b)
 *      a2 <= helper(b2)
 *    }
 * }
 */
#define VEC_FLOAT2(NAME, HELPERFN)                 \
int arc_gen_##NAME(DisasCtxt *ctx, TCGv a, TCGv b) \
{                                                  \
    int ret = DISAS_NEXT;                          \
                                                   \
    gen_helper_##HELPERFN(a, cpu_env, b);          \
                                                   \
    if (vfp_width > 64) {                          \
        TCGv na = nextFPURegWithNull(ctx, a);      \
        TCGv nb = nextFPUReg(ctx, b);              \
        gen_helper_##HELPERFN(na, cpu_env, nb);    \
    }                                              \
    return ret;                                    \
}

VEC_FLOAT2(VFHSQRT, vfhsqrt)
VEC_FLOAT2(VFSSQRT, vfssqrt)
VEC_FLOAT2(VFDSQRT, vfdsqrt)

/*
 * Operations: ADD/SUB/MUL/DIV/ADDSUB/SUBADD
 * VFHMUL VFHDIV VFHADD VFHSUB VFHADDSUB VFHSUBADD
 * VFSMUL VFSDIV VFSADD VFSSUB VFSADDSUB VFSSUBADD
 * VFDMUL VFDDIV VFDADD VFDSUB VFDADDSUB VFDSUBADD
 * Half/Single/Double precision floating point operation for all elements in a
 * vector
 *    Variables: @a, @b, @c
 *    Functions: gen_helper_vfhsqrt gen_helper_vfssqrt gen_helper_vfdsqrt
 * --- code ---
 * {
 *  a <= helper(b, c)
 *  if(vfp_width > 64)
 *    {
 *      a2 = nextFPUReg(a)
 *      b2 = nextFPUReg(b)
 *      c2 = nextFPUReg(c)
 *      a2 <= helper(b2, c2)
 *    }
 * }
 */
#define VEC_FLOAT3(NAME, HELPERFN) \
int arc_gen_##NAME(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c) \
{                                                          \
    int ret = DISAS_NEXT;                                  \
                                                           \
    gen_helper_##HELPERFN(a, cpu_env, b, c);               \
                                                           \
    if (vfp_width > 64) {                                  \
        TCGv na = nextFPURegWithNull(ctx, a);              \
        TCGv nb = nextFPUReg(ctx, b);                      \
        TCGv nc = nextFPUReg(ctx, c);                      \
        gen_helper_##HELPERFN(na, cpu_env, nb, nc);        \
    }                                                      \
    return ret;                                            \
}

VEC_FLOAT3(VFHMUL, vfhmul)
VEC_FLOAT3(VFSMUL, vfsmul)
VEC_FLOAT3(VFDMUL, vfdmul)

VEC_FLOAT3(VFHDIV, vfhdiv)
VEC_FLOAT3(VFSDIV, vfsdiv)
VEC_FLOAT3(VFDDIV, vfddiv)

VEC_FLOAT3(VFHADD, vfhadd)
VEC_FLOAT3(VFSADD, vfsadd)
VEC_FLOAT3(VFDADD, vfdadd)

VEC_FLOAT3(VFHSUB, vfhsub)
VEC_FLOAT3(VFSSUB, vfssub)
VEC_FLOAT3(VFDSUB, vfdsub)

VEC_FLOAT3(VFHADDSUB, vfhaddsub)
VEC_FLOAT3(VFSADDSUB, vfsaddsub)
VEC_FLOAT3(VFDADDSUB, vfdaddsub)

VEC_FLOAT3(VFHSUBADD, vfhsubadd)
VEC_FLOAT3(VFSSUBADD, vfssubadd)
VEC_FLOAT3(VFDSUBADD, vfdsubadd)

/*
 * Operations: ADD/SUB/MUL/DIV
 * VFHMULS VFHDIVS VFHADDS VFHSUBS
 * VFSMULS VFSDIVS VFSADDS VFSSUBS
 * VFDMULS VFDDIVS VFDADDS VFDSUBS
 * Half/Single/Double precision floating point operation for all elements in a
 * vector
 *    Variables: @a, @b, @c
 *    Functions: gen_helper_vfhsqrt gen_helper_vfssqrt gen_helper_vfdsqrt
 * --- code ---
 * {
 *  a <= helper(b, c)
 *  if(vfp_width > 64)
 *    {
 *      a2 = nextFPUReg(a)
 *      b2 = nextFPUReg(b)
 *      a2 <= helper(b2, c)
 *    }
 * }
 */
#define VEC_FLOAT3_SCALARC(NAME, HELPERFN) \
int arc_gen_##NAME(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c) \
{                                                          \
    int ret = DISAS_NEXT;                                  \
                                                           \
    gen_helper_##HELPERFN(a, cpu_env, b, c);               \
                                                           \
    if (vfp_width > 64) {                                  \
        TCGv na = nextFPURegWithNull(ctx, a);              \
        TCGv nb = nextFPUReg(ctx, b);                      \
        gen_helper_##HELPERFN(na, cpu_env, nb, c);         \
    }                                                      \
    return ret;                                            \
}

VEC_FLOAT3_SCALARC(VFHMULS, vfhmuls)
VEC_FLOAT3_SCALARC(VFSMULS, vfsmuls)
VEC_FLOAT3_SCALARC(VFDMULS, vfdmuls)

VEC_FLOAT3_SCALARC(VFHDIVS, vfhdivs)
VEC_FLOAT3_SCALARC(VFSDIVS, vfsdivs)
VEC_FLOAT3_SCALARC(VFDDIVS, vfddivs)

VEC_FLOAT3_SCALARC(VFHADDS, vfhadds)
VEC_FLOAT3_SCALARC(VFSADDS, vfsadds)
VEC_FLOAT3_SCALARC(VFDADDS, vfdadds)

VEC_FLOAT3_SCALARC(VFHSUBS, vfhsubs)
VEC_FLOAT3_SCALARC(VFSSUBS, vfssubs)
VEC_FLOAT3_SCALARC(VFDSUBS, vfdsubs)

/*
 * Operations: ADD/SUB/MADD/MSUB
 * VFHMADD VFHMSUB VFHNMADD VFHNMSUB
 * VFSMADD VFSMSUB VFSNMADD VFSNMSUB
 * VFDMADD VFDMSUB VFDNMADD VFDNMSUB
 * Half/Single/Double precision floating point operation for all elements in a
 * vector
 *    Variables: @a, @b, @c
 *    Functions: gen_helper_vfhsqrt gen_helper_vfssqrt gen_helper_vfdsqrt
 * --- code ---
 * {
 *  a <= helper(b, c, d)
 *  if(vfp_width > 64)
 *    {
 *      a2 = nextFPUReg(a)
 *      b2 = nextFPUReg(b)
 *      c2 = nextFPUReg(c)
 *      d2 = nextFPUReg(d)
 *      a2 <= helper(b2, c2, d2)
 *    }
 * }
 */
#define VEC_FLOAT4(NAME, HELPERFN)                                 \
int arc_gen_##NAME(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c, TCGv d) \
{                                                                  \
    int ret = DISAS_NEXT;                                          \
                                                                   \
    gen_helper_##HELPERFN(a, cpu_env, b, c, d);                    \
                                                                   \
    if (vfp_width > 64) {                                          \
        TCGv na = nextFPURegWithNull(ctx, a);                      \
        TCGv nb = nextFPUReg(ctx, b);                              \
        TCGv nc = nextFPUReg(ctx, c);                              \
        TCGv nd = nextFPUReg(ctx, d);                              \
        gen_helper_##HELPERFN(na, cpu_env, nb, nc, nd);            \
    }                                                              \
    return ret;                                                    \
}

VEC_FLOAT4(VFHMADD, vfhmadd)
VEC_FLOAT4(VFSMADD, vfsmadd)
VEC_FLOAT4(VFDMADD, vfdmadd)

VEC_FLOAT4(VFHMSUB, vfhmsub)
VEC_FLOAT4(VFSMSUB, vfsmsub)
VEC_FLOAT4(VFDMSUB, vfdmsub)

VEC_FLOAT4(VFHNMADD, vfhnmadd)
VEC_FLOAT4(VFSNMADD, vfsnmadd)
VEC_FLOAT4(VFDNMADD, vfdnmadd)

VEC_FLOAT4(VFHNMSUB, vfhnmsub)
VEC_FLOAT4(VFSNMSUB, vfsnmsub)
VEC_FLOAT4(VFDNMSUB, vfdnmsub)

/*
 * Operations: MADDS/MSUBS/NMADDS/NMSUBS
 * VFHMADDS VFHMSUBS VFHNMADDS VFHNMSUBS
 * VFSMADDS VFSMSUBS VFSNMADDS VFSNMSUBS
 * VFDMADDS VFDMSUBS VFDNMADDS VFDNMSUBS
 * Half/Single/Double precision floating point operation for all elements in a
 * vector
 *    Variables: @a, @b, @c
 *    Functions: gen_helper_vfhsqrt gen_helper_vfssqrt gen_helper_vfdsqrt
 * --- code ---
 * {
 *  a <= helper(b, c, d)
 *  if(vfp_width > 64)
 *    {
 *      a2 = nextFPUReg(a)
 *      b2 = nextFPUReg(b)
 *      c2 = nextFPUReg(c)
 *      a2 <= helper(b2, c2, d)
 *    }
 * }
 */
#define VEC_FLOAT4_SCALARD(NAME, HELPERFN) \
int arc_gen_##NAME(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c, TCGv d) \
{                                                                  \
    int ret = DISAS_NEXT;                                          \
                                                                   \
    gen_helper_##HELPERFN(a, cpu_env, b, c, d);                    \
                                                                   \
    if (vfp_width > 64) {                                          \
        TCGv na = nextFPURegWithNull(ctx, a);                      \
        TCGv nb = nextFPUReg(ctx, b);                              \
        TCGv nc = nextFPUReg(ctx, c);                              \
        gen_helper_##HELPERFN(na, cpu_env, nb, nc, d);             \
    }                                                              \
    return ret;                                                    \
}

VEC_FLOAT4_SCALARD(VFHMADDS, vfhmadds)
VEC_FLOAT4_SCALARD(VFSMADDS, vfsmadds)
VEC_FLOAT4_SCALARD(VFDMADDS, vfdmadds)

VEC_FLOAT4_SCALARD(VFHMSUBS, vfhmsubs)
VEC_FLOAT4_SCALARD(VFSMSUBS, vfsmsubs)
VEC_FLOAT4_SCALARD(VFDMSUBS, vfdmsubs)

VEC_FLOAT4_SCALARD(VFHNMADDS, vfhnmadds)
VEC_FLOAT4_SCALARD(VFSNMADDS, vfsnmadds)
VEC_FLOAT4_SCALARD(VFDNMADDS, vfdnmadds)

VEC_FLOAT4_SCALARD(VFHNMSUBS, vfhnmsubs)
VEC_FLOAT4_SCALARD(VFSNMSUBS, vfsnmsubs)
VEC_FLOAT4_SCALARD(VFDNMSUBS, vfdnmsubs)

/*
 * VFHEXCH VFSEXCH VFDEXCH
 * Half/Single/Double precision floating point vector exchange permutation
 * operation for all elements in a vector
 *    Variables: @a, @b, @c
 *    Functions: gen_helper_vfhsqrt gen_helper_vfssqrt gen_helper_vfdsqrt
 * --- code ---
 * {
 *  a <= helper(b, c, d)
 *  if(vfp_width > 64)
 *    {
 *      a2 = nextFPUReg(a)
 *      b2 = nextFPUReg(b)
 *      tmp <= gen_helper_vector_shuffle(0, b2, b, 0, 0)
 *      a2 <=  gen_helper_vector_shuffle(1, b2, b, 0, 0)
 *       a <= tmp
 *    }
 *   else
 *    {
 *      a <=  gen_helper_vector_shuffle(0, 0, b, 0, 0)
 *    }
 * }
 */
#define VEC_SHUFFLE2_INSN(NAME, TYPE)                                          \
int arc_gen_##NAME(DisasCtxt *ctx, TCGv a, TCGv b)                             \
{                                                                              \
    int ret = DISAS_NEXT;                                                      \
    TCGv zero = tcg_const_tl(0);                                               \
    TCGv one = tcg_const_tl(1);                                                \
    TCGv type = tcg_const_tl(TYPE);                                            \
    if (vfp_width > 64) {                                                      \
        TCGv tmp = tcg_temp_new();                                             \
        TCGv na = nextFPURegWithNull(ctx, a);                                  \
        TCGv nb = nextFPUReg(ctx, b);                                          \
        gen_helper_vector_shuffle(tmp,  cpu_env, type, zero,                   \
                                  nb, b, zero, zero);                          \
        gen_helper_vector_shuffle(na, cpu_env, type, one,  nb, b, zero, zero); \
        tcg_gen_mov_tl(a, tmp);                                                \
        tcg_temp_free(tmp);                                                    \
    } else {                                                                   \
        gen_helper_vector_shuffle(a,  cpu_env, type, zero,                     \
                                  zero, b, zero, zero);                        \
    }                                                                          \
    tcg_temp_free(zero);                                                       \
    tcg_temp_free(one);                                                        \
    tcg_temp_free(type);                                                       \
    return ret;                                                                \
}

VEC_SHUFFLE2_INSN(VFHEXCH, HEXCH)
VEC_SHUFFLE2_INSN(VFSEXCH, SEXCH)
VEC_SHUFFLE2_INSN(VFDEXCH, DEXCH)

/*
 * VFHUNPKL VFHUNPKM VFSUNPKL VFSUNPKM VFDUNPKL VFDUNPKM
 *   Vector unpack (inverse shuffle source register elements into the
 *   destination register, and store the Least/Most-significant half result)
 * VFHPACKL VFHPACKM VFSPACKL VFSPACKM VFDPACKL VFDPACKM
 *   Vector pack (shuffle source register elements into the destination
 *   register, and store the Least/Most-significant half result)
 * VFHBFLYL VFHBFLYM VFSBFLYL VFSBFLYM VFDBFLYL VFDBFLYM
 *   Vector pack (shuffle source register elements into the destination
 *   register, and store the Least/Most-significant half result)
 *    Variables: @a, @b, @c
 *    Functions: gen_helper_vfhsqrt gen_helper_vfssqrt gen_helper_vfdsqrt
 * --- code ---
 * {
 *  a <= helper(b, c, d)
 *  if(vfp_width > 64)
 *    {
 *      a2 = nextFPUReg(a)
 *      b2 = nextFPUReg(b)
 *      c2 = nextFPUReg(c)
 *      tmp <= gen_helper_vector_shuffle(0, b2, b, c2, c)
 *      a2 <=  gen_helper_vector_shuffle(1, b2, b, c2, c)
 *       a <= tmp
 *    }
 *   else
 *    {
 *      tmp <= gen_helper_vector_shuffle(0, 0, b, 0, c)
 *    }
 * }
 */
#define VEC_SHUFFLE3_INSN(NAME, TYPE) \
int arc_gen_##NAME(DisasCtxt *ctx, TCGv a, TCGv b, TCGv c)                    \
{                                                                             \
    int ret = DISAS_NEXT;                                                     \
    TCGv zero = tcg_const_tl(0);                                              \
    TCGv one = tcg_const_tl(1);                                               \
    TCGv type = tcg_const_tl(TYPE);                                           \
    if (vfp_width > 64) {                                                     \
        TCGv tmp = tcg_temp_new();                                            \
        TCGv na = nextFPURegWithNull(ctx, a);                                 \
        TCGv nb = nextFPUReg(ctx, b);                                         \
        TCGv nc = nextFPUReg(ctx, c);                                         \
        gen_helper_vector_shuffle(tmp,  cpu_env, type, zero, nb, b, nc, c);   \
        gen_helper_vector_shuffle(na, cpu_env, type, one,  nb, b, nc, c);     \
        tcg_gen_mov_tl(a, tmp);                                               \
        tcg_temp_free(tmp);                                                   \
    } else {                                                                  \
        gen_helper_vector_shuffle(a,  cpu_env, type, zero, zero, b, zero, c); \
    }                                                                         \
    tcg_temp_free(zero);                                                      \
    tcg_temp_free(one);                                                       \
    tcg_temp_free(type);                                                      \
    return ret;                                                               \
}

VEC_SHUFFLE3_INSN(VFHUNPKL, HUNPKL)
VEC_SHUFFLE3_INSN(VFHUNPKM, HUNPKM)
VEC_SHUFFLE3_INSN(VFSUNPKL, SUNPKL)
VEC_SHUFFLE3_INSN(VFSUNPKM, SUNPKM)
VEC_SHUFFLE3_INSN(VFDUNPKL, DUNPKL)
VEC_SHUFFLE3_INSN(VFDUNPKM, DUNPKM)
VEC_SHUFFLE3_INSN(VFHPACKL, HPACKL)
VEC_SHUFFLE3_INSN(VFHPACKM, HPACKM)
VEC_SHUFFLE3_INSN(VFSPACKL, SPACKL)
VEC_SHUFFLE3_INSN(VFSPACKM, SPACKM)
VEC_SHUFFLE3_INSN(VFDPACKL, DPACKL)
VEC_SHUFFLE3_INSN(VFDPACKM, DPACKM)
VEC_SHUFFLE3_INSN(VFHBFLYL, HBFLYL)
VEC_SHUFFLE3_INSN(VFHBFLYM, HBFLYM)
VEC_SHUFFLE3_INSN(VFSBFLYL, SBFLYL)
VEC_SHUFFLE3_INSN(VFSBFLYM, SBFLYM)
VEC_SHUFFLE3_INSN(VFDBFLYL, DBFLYL)
VEC_SHUFFLE3_INSN(VFDBFLYM, DBFLYM)
