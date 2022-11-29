; check_dmach.S
;
; Tests for dmach
; If the test fails, check the end of this file for how to troubleshoot.

  .include "macros.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;; Test checking routines ;;;;;;;;;;;;;;;;;;;;;;;;;;

; Test case counter
.data
test_nr:
  .word 0x0

; Increment the test counter and set (Z,N,C,V) to (0,0,0,0).
.macro prep_test_case
  ld    r13, [test_nr]
  add_s r13, r13, 1       ; increase test case counter
  st    r13, [test_nr]
  add.f 0, 0, 1           ; (Z,N,C,V) = (0,0,0,0)
.endm

; reg1 = lo, reg2 = hi
.macro set_reg64 reg1, reg2, hi, lo
  mov   \reg1, \lo
  mov   \reg2, \hi
.endm

; reg = lo
; macro mainly for alignment
.macro set_reg32 reg, lo
  mov \reg, \lo
.endm

.macro chk_reg32 reg, lo
  xor.f 0,   \reg, \lo
  bne    @fail
.endm

; reg1 == hi, reg2 == lo AND acchi == hi, acclo == lo
.macro chk_reg64 reg1, reg2, hi, lo
  xor.f 0, \reg1, \lo
  bne    @fail
  xor.f 0, \reg2, \hi
  bne    @fail
.endm

; Bit positions for (Z,N,C,V) in STATUS32 register
.equ REG_STAT_V_POS,  8
.equ REG_STAT_C_POS,  9
.equ REG_STAT_N_POS, 10
.equ REG_STAT_Z_POS, 11

; (Z,N,C,V) = (0,1,0,0)
.macro set_n_flag
    add.f 0, -1, 0  ; negative result
.endm

; (Z,N,C,V) = (0,1,0,1)
.macro set_nv_flags
    add.f 0, 0x7fffffff, 1  ; negative result with an overflow
.endm


; The is_x_flag_set / are_xy_flags_set, macros use "b @fail" instead of
; "bbit ..., @fail" to support longer range

; (Z,N,C,V) = (0,1,0,0)
.macro is_n_flag_set
  lr    r11, [status32]
  bbit1 r11, REG_STAT_Z_POS, @321f
  bbit0 r11, REG_STAT_N_POS, @321f
  bbit1 r11, REG_STAT_C_POS, @321f
  bbit1 r11, REG_STAT_V_POS, @321f
  b     @123f
321:
  b     @fail
123:
  ; Fall through
.endm

; (Z,N,C,V) = (0,0,0,1)
.macro is_v_flag_set
  lr    r11, [status32]
  bbit1 r11, REG_STAT_Z_POS, @321f
  bbit1 r11, REG_STAT_N_POS, @321f
  bbit1 r11, REG_STAT_C_POS, @321f
  bbit0 r11, REG_STAT_V_POS, @321f
  b     @123f
321:
  b     @fail
123:
  ; Fall through
.endm

; (Z,N,C,V) = (0,1,0,1)
.macro is_nv_flags_set
  lr    r11, [status32]
  bbit1 r11, REG_STAT_Z_POS, @321f
  bbit0 r11, REG_STAT_N_POS, @321f
  bbit1 r11, REG_STAT_C_POS, @321f
  bbit0 r11, REG_STAT_V_POS, @321f
  b     @123f
321:
  b     @fail
123:
  ; Fall through
.endm

; (Z,N,C,V) = (0,0,0,0)
.macro is_no_flag_set
  lr    r11, [status32]
  bbit1 r11, REG_STAT_Z_POS, @321f
  bbit1 r11, REG_STAT_N_POS, @321f
  bbit1 r11, REG_STAT_C_POS, @321f
  bbit1 r11, REG_STAT_V_POS, @321f
  b     @123f
321:
  b     @fail
123:
  ; Fall through
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; DMACH ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test case 1
; r1 <- r1, r2
; ( 0x0002, 0x0001 ) *      r2
; ( 0x0003, 0x0004 ) +      r0
; (       0        )       r58 (acc)
; -------------------
; (1*3 + 2*4) + 0
; (      0      )
; ----------------
;     11 (0xa)
  prep_test_case
  set_reg64 r58, r59, hi=0         , lo=0
  set_reg64  r2, r3, hi=0xffffffff, lo=0x00020001 ; hi shouldnt matter
  set_reg64  r0, r1, hi=0xffffffff, lo=0x00030004 ; hi shouldnt matter
  dmach      r0, r0, r2
  chk_reg32  r0, 0xa
  chk_reg64 r58, r59, hi=0, lo=0xa
  
; Test case 2
; r3 <- r3, r3
; ( 0x0003, 0x0004 ) *      r3
; ( 0x0003, 0x0004 ) +      r3
; (       42       )       r58 (acc)
; --------------------------
; (3 * 3 + 4 * 4) +
; (     42      )
; -----------------
;    67 = 0x43
;
  prep_test_case
  set_reg64 r58, r59, hi=0, lo=42
  set_reg32  r3, 0x00030004
  dmach      r3, r3, r3
  chk_reg32  r3, 0x43
  chk_reg64  r58, r59, hi=0, lo=0x43

; Test case 3
; Small negatives multiplied with big positives (acc = 0)
;  -32766   32766
; (0x8002, 0x7ffe) *        r4
; (0x7fff, 0x8000) +        r2
;   32767  -32768
; (       0      )         r58 (acc)
; -----------------
; (-1073643522 + -1073676288 + 0)
; ---------------------------------------
;     -2147319810 = 0xffff_ffff_8002_7ffe
  prep_test_case
  set_reg64 r58, r59, hi=0, lo=0
  set_reg32  r1, 0x0
  set_reg32  r4, 0x80027ffe
  set_reg32  r2, 0x7fff8000
  dmach      r0, r2, r4
  is_no_flag_set
  chk_reg32  r0, 0x80027ffe
  chk_reg64 r58, r59, hi=0xffffffff, lo=0x80027ffe

; Test case 4
; Small negatives multiplied with big positives (acc = very_big)
;  -32766   32766
; ( 0x8002    , 0x7ffe    ) *         r0
; ( 0x7fff    , 0x8000    ) +         r4
;   32767  -32768
; ( 0x0000_0000_7ffd_8003 )          r58 (acc)
;       2147319811
; -------------------------
; (-1073643522 + -1073643522 + 2147319810)
; -----------------------------------------
;                   1
  prep_test_case
  set_reg64 r58, r59, hi=0, lo=0x7ffd8003
  set_reg32  r0, 0x80027ffe
  set_reg32  r4, 0x7fff8000
  dmach.f    r0, r4, r0
  is_no_flag_set
  chk_reg32  r0, 0x00000001
  chk_reg64 r58, r59, hi=0x00000000, lo=0x00000001

; Test case 5
; Bunch of smallest negatives (acc = 0)
;  -32768  -32768
; (0x8000, 0x8000) *          r1
; (0x8000, 0x8000) +          r3
;  -32768  -32768
; (       0      )           r58 (acc)
; -----------------------
; (    1073741824 + 0   )
;  0x0000_0000_4000_0000
  prep_test_case
  set_reg64 r58, r59, hi=0, lo=0
  set_reg32  r1, 0x80008000
  set_reg32  r3, 0x80000000
  dmach      r3, r3, r1
  is_no_flag_set
  chk_reg32  r3, lo=0x40000000
  chk_reg64 r58, r59, hi=0x00000000, lo=0x40000000

; Test case 6
; Bunch of very small negatives (acc = very_small)
;  -32768  -32768
; (0x8000, 0x8000 ) *        r8
; (0x8000, 0x8000 ) +        r7
;  -32768  -32768
; ( 0x8000_0000_0000_0000 ) r58 (acc)
; -------------------------
; ( 2147483648 + -9223372036854775808)
; --------------------------------------------
; -9223512774343131136 = 0x8000_0000_8000_0000
  prep_test_case
  set_reg64 r58, r59, hi=0x80000000, lo=0x00000000
  set_reg32  r8, 0x80008000
  set_reg32  r7, 0x80008000
  dmach      r0, r7, r8
  is_no_flag_set
  chk_reg32  r0, 0x80000000
  chk_reg64 r58, r59, hi=0x80000000, lo=0x80000000

; Test case 7
; Biggest positive multiplied with itself (acc = non_zero)
;   32767   32767
; (  0x7fff   , 0x7fff    ) *         r8
; (  0x7fff   , 0x7fff    ) +         r7
;     32767      32767
; ( 0x8234_5678_9abc_def0 )          r58 (acc)
; -------------------------
; (1073676289 * 2 + -9064525073997832464)
; -----------------------------------------
; -9382218997564366574 = 0x8234d6779abbdef2
  prep_test_case
  set_reg64 r58, r59, hi=0x12345678, lo=0x9abcdef0
  set_reg32  r8, 0x7fff7fff
  set_reg32  r7, 0x7fff7fff
  dmach      r9, r7, r8
  is_no_flag_set
  chk_reg32  r9, 0x1abadef2
  chk_reg64 r58, r59, hi=0x12345679, lo=0x1abadef2

; Test case 8
; Check if N flag is set
; Although the accumulator is already negative, nonetheless the N flag
; must be set.
; (0   , 0  ) *          r0
; (0   , 0  ) +          r0
; (   -1    )            r58 (acc)
; -----------
;     -1
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff
  set_reg32  r0, 0
  dmach.f    r0, r0, r0
  is_n_flag_set
  chk_reg32  r0, 0xffffffff
  chk_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff

; Test case 9
; Check if all flags (Z,N,C,V) remain unset.
; (0   , 1  ) *          r5
; (0   , 1  ) +          r5
; (   -1    )            r58 (acc)
; -----------
;      0
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff
  set_reg32  r5, 0x00000001
  dmach.f    r5, r5, r5
  is_no_flag_set
  chk_reg32  r5, 0
  chk_reg64 r58, r59, hi=0, lo=0

; Test case 10
; Check if V flag is set (negative to positive overflow)
; (0         ,  1         ) *       r2
; (0         , -1         ) +       r3
; ( 0x8000_0000_0000_0000 )        r58 (acc)
; -------------------------
; ( -1 + -9223372036854775808 )
; -----------------------------
;    0x7fff_ffff_ffff_ffff
;     9223372036854775807
  prep_test_case
  set_reg64 r58, r59, hi=0x80000000, lo=0x00000000
  set_reg32  r2, 0x0000ffff
  set_reg32  r3, 0x00000001
  dmach.f    r2, r3, r2
  is_v_flag_set
  chk_reg32  r2, 0xffffffff
  chk_reg64 r58, r59, hi=0x7fffffff, lo=0xffffffff


; Test case 11
; Check if N and V flags are set (positive to negative overflow)
; (0         ,  1         ) *       r2
; (0         ,  1         ) +       r2
; ( 0x7fff_ffff_ffff_ffff )        r58 (acc)
; -------------------------
; ( 1 + 9223372036854775807 )
; ---------------------------
;    0x8000_0000_0000_0000
;    -9223372036854775808
  prep_test_case
  set_reg64 r58, r59, hi=0x7fffffff, lo=0xffffffff
  set_reg32  r2, 0x00000001
  dmach.f    r2, r2, r2
  is_nv_flags_set
  chk_reg32  r2, 0x00000000
  chk_reg64 r58, r59, hi=0x80000000, lo=0x00000000


; Test case 12
; Nothing is set although there really is an overflow and negative
; (0         ,  1         ) *       r2
; (0         ,  1         ) +       r2
; ( 0x7fff_ffff_ffff_ffff )        r58 (acc)
; -------------------------
; ( 1 + 9223372036854775807 )
; ---------------------------
;    0x8000_0000_0000_0000
;    -9223372036854775808
  prep_test_case
  set_reg64 r58, r59, hi=0x7fffffff, lo=0xffffffff
  set_reg32  r2, 0x00000001
  dmach      r2, r2, r2
  is_no_flag_set
  chk_reg32  r2, 0x00000000
  chk_reg64 r58, r59, hi=0x80000000, lo=0x00000000

; Test case 13
; reg <- reg, u6 (u6 is extended to 16 bits and then replicated 4 times)
;      32767       32767
; (   0x7fff  ,   0x7fff  ) *        r7
; ( 0b111111  , 0b111111  ) +        u6
;      63           63
; ( 0x0000_0000_0000_0000 )         r58 (acc)
; -------------------------
; ( 2064321 * 2 + 0 )
; ------------------------
; 0x0000_0000_003e_ff82
;               4128642
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg32  r7, 0x7fff7fff
  dmach      r6, r7, 63
  is_no_flag_set
  chk_reg32  r6, 0x003eff82
  chk_reg64 r58, r59, hi=0x00000000, lo=0x003eff82

; Test case 14
; reg <- reg, s12 (s12 is sign extended to 16 bits and then replicated 4 times)
;     -32768       32767
; (   0x8000  ,   0x7fff  ) *        r6
; (     -1    ,     -1    ) +       s12
; ( 0x0000_0000_0000_0000 )         r58 (acc)
; -------------------------
; (     32768 - 32767     )
; -------------------------
;   0x0000_0000_0000_0001
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg32  r6, 0x80007fff
  dmach      r6, r6, -1
  is_no_flag_set
  chk_reg32  r6, 0x00000001
  chk_reg64 r58, r59, hi=0x00000000, lo=0x00000001

; Test case 15
; reg <- reg, s12 (smallest negative s12)
;     -32768       32767
; (   0x8000  ,   0x7fff  ) *        r9
; (   -2048   ,   -2048   ) +       s12
; ( 0x0000_0000_0000_0000 )         r58 (acc)
; -------------------------
; (  67108864 + -67106816 )
; -------------------------
;     0x0000_0000_0000_0800
;                      2048
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg32  r9, 0x80007fff
  dmach      r9, r9, -2048
  is_no_flag_set
  chk_reg32  r9, 0x00000800
  chk_reg64 r58, r59, hi=0x00000000, lo=0x00000800

; Test case 16
; reg <- reg, s12 (biggest positive s12)
;     -32768       32767
; (   0x8000  ,   0x7fff  ) *        r9
; (     2047  ,     2047  ) +       s12
; ( 0x0000_0000_0000_0000 )         r58 (acc)
; -------------------------
; ( -67076096 + 67074049  )
; -------------------------
;    0xffff_ffff_ffff_f801
;                    -2047
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg32  r7, 0x80007fff
  dmach      r7, r7, 2047
  is_no_flag_set
  chk_reg32  r7, 0xfffff801
  chk_reg64 r58, r59, hi=0xffffffff, lo=0xfffff801

; Test case 17
; check .cc when it is not met, with .f
; ( 0         , 0         ) *         r7
; ( 0         , 0         ) +        s12
; ( 0x0000_0000_0000_0001 )          r58 (acc)
; -------------------------
; set n flag
; -------------------------------
;  0 * 2 + 1 = 1 and n flag unset
; -------------------------------
; conditions not met, n flag should still be set
;
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000001
  set_reg32  r7, 0x00000000
  set_n_flag
  dmach.p.f r7, r7, 0
  is_n_flag_set

; Test case 18
; check .cc when it is met, as well as .f
; ( 0         , 0         ) *        r7
; ( 0         , 0         ) +       s12
; ( 0x0000_0000_0000_0001 )         r58 (acc)
; -------------------------
; set n flag
; -------------------------------
;  0 * 2 + 1 = 1 and n flag unset
; -------------------------------
; conditions are met, n flag should not be set
;
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000001
  set_reg32  r7, 0x00000000
  set_n_flag
  dmach.n.f r7, r7, 0
  is_no_flag_set
  chk_reg32  r7, 0x00000001
  chk_reg64 r58, r59, hi=0x00000000, lo=0x00000001

; Test case 19
; check 0 as register and .f flag
; ( 0         , 0         ) *        r1
; ( 0         , 0         ) +       s12
; ( 0x0000_0000_0000_0001 )         r58 (acc)
; -------------------------
; set n flag
; -------------------------------
;  0 * 2 + 1 = 1 and n flag unset
; -------------------------------
; n flag should not be set anymore
;
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000001
  set_reg32  r1, 0x00000000
  set_n_flag
  dmach.f    0, r1, 0
  is_no_flag_set

; Test case 20
; check basic limm (limm is duplicated)
;    4096       1
; ( 0x1000    , 0x0001    ) *         r0
; ( 0x1000    , 0x0001    ) +       limm
; ( 0x0000_0000_0000_0010 )          r58 (acc)
; -------------------------
;  0x1000 * 0x1000 + 0x1 + 0x10
; -----------------------------
;      0x1000011 = 16777233
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000010
  set_reg32  r0, lo=0x10000001
  dmach      r0, r0, 0x10000001
  is_no_flag_set
  chk_reg32  r0, 0x01000011
  chk_reg64 r58, r59, hi=0x00000000, lo=0x01000011

; Test case 21
; check double limm (limm is duplicated)
;    4096       1
; ( 0x1000    , 0x0001    ) *      limm
; ( 0x1000    , 0x0001    ) +      limm
; ( 0x0000_0000_0000_0010 )         r58 (acc)
; -------------------------
;  0x1000 * 0x1000 + 0x1 + 0x10
; -----------------------------
;      0x1000011 = 16777233
;
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000010
  dmach      r0, 0x10000001, 0x10000001
  is_no_flag_set
  chk_reg32  r0, 0x01000011
  chk_reg64 r58, r59, hi=0x00000000, lo=0x01000011

; Test case 22
; 0 target register with 2 numbers, set flags (only use for this encoding)
; ( 1         , 1         ) *       limm
; ( 1         , 1         ) +       limm
; ( 0x7fff_ffff_ffff_ffff )          r58 (acc)
; -------------------------
;  1 * 1 + 0x7fffffffffffffff
; ---------------------------
;
prep_test_case
  set_reg64 r58, r59, hi=0x7fffffff, lo=0xffffffff
  dmach.f    0, 1, 1
  is_nv_flags_set
  ; Validate that the accumulator was set accordingly
  xor.f      0,  r58, 0
  bne @fail
  xor.f      0,  r59, 0x80000000
  bne @fail

; Test case 23
; Check if dmach unsets n flag but doesnt unset the v flag
; ( 0x0000    , 0x0000    ) *      limm
; ( 0x0000    , 0x0000    ) +      limm
; ( 0x0000_0000_0000_0000 )         r58 (acc)
; -------------------------
;  0x0000 * 4 + 0x0x0123456789abcdef
; ----------------------------------
;  0, n flag cleared but v flag untouched
;
 prep_test_case
   set_reg64 r58, r59, hi=0x01234567, lo=0x89abcdef
   set_nv_flags
   dmach.f    0, 0, 0
   is_v_flag_set
  ; Validate that the accumulator was set accordingly
  xor.f       0,   r58, 0x89abcdef
  bne @fail
  xor.f       0,   r59, 0x01234567
  bne @fail

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; DMACHU ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Test case 1
; r1 <- r1, r2
; ( 0x0002, 0x0001 ) *      r2
; ( 0x0003, 0x0004 ) +      r1
; (       0        )       r58 (acc)
; -------------------
; (1*3 + 2*4) + 0
; (      0      )
; ----------------
;     11 (0xa)
  prep_test_case
  set_reg64 r58, r59, hi=0         , lo=0
  set_reg64  r2, r3,hi=0xffffffff, lo=0x00020001 ; hi shouldnt matter
  set_reg64  r0, r1, hi=0xffffffff, lo=0x00030004 ; hi shouldnt matter
  dmachu     r0, r0, r2
  chk_reg32  r0, 0xa
  chk_reg64 r58, r59, hi=0, lo=0xa
  
; Test case 2
; r3 <- r3, r3
; ( 0x0003, 0x0004 ) *      r3
; ( 0x0003, 0x0004 ) +      r3
; (       42       )       r58 (acc)
; --------------------------
; (3 * 3 + 4 * 4) +
; (     42      )
; -----------------
;    67 = 0x43
;
  prep_test_case
  set_reg64 r58, r59, hi=0, lo=42
  set_reg32  r3, 0x00030004
  dmachu      r3, r3, r3
  chk_reg32  r3, 0x43
  chk_reg64 r58, r59, hi=0, lo=0x43

; Test case 7
; Biggest positive multiplied with itself (acc = non_zero)
;     65535      65535
; (  0xffff   , 0xffff    ) *         r8
; (  0xffff   , 0xffff    ) +         r7
; ( 0x1234_5678_9abc_def0 )          r58 (acc)
; ------------------------------------------
; (65535 * 65535 * 2 + 1311768467463790320)
; ------------------------------------------
; 1311768476053462770 = 0x1234567a9ab8def2
  prep_test_case
  set_reg64 r58, r59, hi=0x12345678, lo=0x9abcdef0
  set_reg32  r8, 0xffffffff
  set_reg32  r7, 0xffffffff
  dmachu      r9, r7, r8
  is_no_flag_set
  chk_reg32  r9, 0x9ab8def2
  chk_reg64 r58, r59, hi=0x1234567a, lo=0x9ab8def2

; Test case 9
; Check if all flags (Z,N,C,V) remain unset.
; (   0       ,   1       ) *          r5
; (   0       ,   1       ) +          r5
; ( 0xffff_ffff_ffff_ffff )            r58 (acc)
; -------------------------
;            0
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xfffffffe
  set_reg32  r5, 0x00000001
  dmachu.f    r5, r5, r5
  is_no_flag_set
  chk_reg32  r5, 0xffffffff
  chk_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff

; Test case 11
; Check if N and V flags are set (positive to negative overflow)
; (0         ,  1         ) *       r2
; (0         ,  1         ) +       r2
; ( 0xffff_ffff_ffff_ffff )        r58 (acc)
; ----------------------------
; ( 1 + 18446744073709551615 )
; ----------------------------
;   0x1_0000_0000_0000_0000
;  18446744073709551616 = 0
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff
  set_reg32  r2, 0x00000001
  dmachu.f    r2, r2, r2
  is_v_flag_set
  chk_reg32  r2, 0
  chk_reg64 r58, r59, hi=0, lo=0


; Test case 12
; Nothing is set although there really is an overflow
; (0         ,  1         ) *       r2
; (0         ,  1         ) +       r2
; ( 0xffff_ffff_ffff_ffff )        r58 (acc)
; -------------------------
; ( 1 + 18446744073709551615 )
; ----------------------------
;   0x1_0000_0000_0000_0000
;  18446744073709551616 = 0
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff
  set_reg32  r2, 0x00000001
  dmachu      r2, r2, r2
  is_no_flag_set
  chk_reg32  r2, 0
  chk_reg64 r58, r59, hi=0, lo=0

; Test case 13
; reg <- reg, u6 (u6 is extended to 16 bits and then replicated 4 times)
;      65535       65535
; (   0xffff  ,   0xffff  ) *        r7
; ( 0b111111  , 0b111111  ) +        u6
;      63           63
; ( 0x0000_0000_0000_0000 )         r58 (acc)
; -------------------------
; ( 4128705 * 2 + 0 )
; ------------------------
; 0x0000_0000_007d_ff82
;               8257410
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg32  r7, 0xffffffff
  dmachu      r6, r7, 63
  is_no_flag_set
  chk_reg32  r6, 0x007dff82
  chk_reg64 r58, r59, hi=0x00000000, lo=0x007dff82

; Test case 14
; reg <- reg, s12 (s12 is sign extended to 16 bits and then replicated 4 times)
;      65535       65535
; (   0xffff  ,   0xffff  ) *        r6
; (     -1    ,     -1    ) +       s12
;      65535       65535
; ( 0x0000_0000_0000_0000 )         r58 (acc)
; -------------------------
; (   65535 * 65535 * 2   )
; -------------------------
;   0x0000_0001_fffc_0002
;              8589672450
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg32  r6, 0xffffffff
  dmachu      r6, r6, -1
  is_no_flag_set
  chk_reg32  r6, 0xfffc0002
  chk_reg64 r58, r59, hi=0x00000001, lo=0xfffc0002

; Test case 15
; reg <- reg, s12 (smallest negative s12)
;      65535       65535
; (   0xffff  ,   0xffff  ) *        r9
; (   -2048   ,   -2048   ) +       s12
;     63488       63488
; ( 0x0000_0000_0000_0000 )         r58 (acc)
; -------------------------
; (   65535 * 63488 * 2   )
; -------------------------
;     0x0000_0001_effe_1000
;                8321372160
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg32  r9, 0xffffffff
  dmachu      r9, r9, -2048
  is_no_flag_set
  chk_reg32  r9, 0xeffe1000
  chk_reg64 r58, r59, hi=0x00000001, lo=0xeffe1000

; Test case 16
; reg <- reg, s12 (biggest positive s12)
;      65535       65535
; (   0xffff  ,   0xffff  ) *        r9
; (     2047  ,     2047  ) +       s12
; ( 0x0000_0000_0000_0000 )         r58 (acc)
; -------------------------
; (    65535 * 2047 * 2   )
; -------------------------
;    0x0000_0000_0ffd_f002
;                268300290
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg32  r7, 0xffffffff
  dmachu      r7, r7, 2047
  is_no_flag_set
  chk_reg32  r7, 0x0ffdf002
  chk_reg64 r58, r59, hi=0x00000000, lo=0x0ffdf002

; Test case 17
; check .cc when it is not met, with .f
; ( 0         , 1         ) *         r7
; ( 1         , 1         ) +        s12
; ( 0xffff_ffff_ffff_ffff )          r58 (acc)
; -------------------------
; set n flag
; --------------------------------------------
;  1 + 0 + 18446744073709551615 and v flag set
;                      0
; --------------------------------------------
; conditions are met, n and v flag should be set
;
prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff
  set_reg32  r7, 0x00000001
  set_n_flag
  dmachu.p.f  r7, r7, 1
  is_n_flag_set

; Test case 18
; check .cc when it is met, as well as .f
; ( 0         , 1         ) *        r7
; ( 1         , 1         ) +       s12
; ( 0xffff_ffff_ffff_ffff )         r58 (acc)
; -------------------------
; set n flag
; --------------------------------------------
;  1 + 0 + 18446744073709551615 and v flag set
;                      0
; --------------------------------------------
; conditions are met, n and v flag should be set
;
prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff
  set_reg32  r7, 0x00000001
  set_n_flag
  dmachu.n.f r7, r7, 1
  is_nv_flags_set
  chk_reg32  r7, 0x00000000
  chk_reg64 r58, r59, hi=0x00000000, lo=0x00000000

; Test case 20
; check basic limm (limm is duplicated)
;    4096       1
; ( 0x1000    , 0x0001    ) *         r0
; ( 0x1000    , 0x0001    ) +       limm
; ( 0x0000_0000_0000_0010 )          r58 (acc)
; -------------------------
;  0x1000 * 0x1000 + 0x1 + 0x10
; -----------------------------
;      0x1000011 = 16777233
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000010
  set_reg32  r0, lo=0x10000001
  dmachu     r0, r0, 0x10000001
  is_no_flag_set
  chk_reg32  r0, 0x01000011
  chk_reg64 r58, r59, hi=0x00000000, lo=0x01000011

; Test case 21
; check double limm (limm is duplicated)
;    4096       1
; ( 0x1000    , 0x0001    ) *      limm
; ( 0x1000    , 0x0001    ) +      limm
; ( 0x0000_0000_0000_0010 )         r58 (acc)
; -------------------------
;  0x1000 * 0x1000 + 0x1 + 0x10
; -----------------------------
;      0x1000011 = 16777233
;
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000010
  dmachu      r0, 0x10000001, 0x10000001
  is_no_flag_set
  chk_reg32  r0, 0x01000011
  chk_reg64 r58, r59, hi=0x00000000, lo=0x01000011

; Test case 22
; 0 target register with 2 numbers, set flags (only use for this encoding)
; ( 1         , 1         ) *       limm
; ( 1         , 1         ) +       limm
; ( 0xffff_ffff_ffff_ffff )          r58 (acc)
; -------------------------
;  1 * 1 + 0xffffffffffffffff
; ---------------------------
;
prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff
  dmachu.f    0, 1, 1
  is_v_flag_set
  ; Validate that the accumulator was set accordingly
  xor.f      0,   r58, 0
  bne @fail

; Test case 23
; Check if dmachu unsets n flag but doesnt unset the v flag
; ( 0x0000    , 0x0000    ) *      limm
; ( 0x0000    , 0x0000    ) +      limm
; ( 0x0000_0000_0000_0000 )         r58 (acc)
; -------------------------
;  0x0000 * 4 + 0x0x0123456789abcdef
; ----------------------------------
;  0, no flag touched
;
 prep_test_case
   set_reg64 r58, r59, hi=0x01234567, lo=0x89abcdef
   set_nv_flags
   dmachu.f    0, 0, 0
   is_nv_flags_set
  ; Validate that the accumulator was set accordingly
  xor.f       0,   r58, 0x89abcdef
  bne @fail
  xor.f       0,   r59, 0x01234567
  bne @fail
; Fall through

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Reporting ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

valhalla:
  print "[PASS] "
  b @1f

; If a test fails, it jumps here. Although, for the sake of uniformity,
; the printed output does not say much about which test case failed,
; one can uncomment the print_number line below or set a breakpoint
; here to check the R0 register for the test case number.
fail:
  ld r0, [test_nr]
  ; print_number r0
  print "[FAIL] "
1:
  print " dmach / dmachu\n"
  flag 1
  end