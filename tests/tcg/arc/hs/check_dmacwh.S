; check_dmacwh.S
;
; Tests for dmacwh / dmacwhu
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

; reg1 == hi, reg2 == lo AND acchi == hi, acclo == lo
.macro chk_reg64 reg1, reg2, hi, lo
  xor.f 0, \reg1, \lo
  bne    @fail
  xor.f 0, \reg2, \hi
  bne    @fail
  xor.f 0, r58, \lo
  bne    @fail
  xor.f 0, r59, \hi
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


; The is_x_flag_set, macros use "b @fail" instead of
; "bbit ..., @fail" to support longer range

; (Z,N,C,V) == (0,1,0,0)
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

; (Z,N,C,V) == (0,0,0,1)
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

; (Z,N,C,V) == (0,1,0,1)
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

; (Z,N,C,V) == (0,0,0,0)
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; DMACWH ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test case 1
; r1 <- r1, r2
; (     0x0002,     0x0001 ) *      r2 (h)
; ( 0x00000003, 0x00000004 ) +      r1 (w)
; (           0            )       r58 (acc)
; --------------------------
;     (1*3 + 2*4) + 0
; (          0          )
; -----------------------
;         11 (0xa)
  prep_test_case
  set_reg64 r58, r59, hi=0         , lo=0
  set_reg64  r2,  r3, hi=0xffffffff, lo=0x00020001 ; hi shouldnt matter
  set_reg64  r0,  r1, hi=0x00000003, lo=0x00000004
  dmacwh     r0,  r0, r2
  chk_reg64  r0,  r1, hi=0, lo=0xa

; Test case 2
; r3 <- r3, r3
; (     0x0003,     0x0004 ) *      r3 (h)
; ( 0x00010002, 0x00030004 ) +      r3 (w)
;        65538,     196612
; (           42           )       r58 (acc)
; --------------------------
; (65538 * 3 + 196612 * 4) +
; (         42           )
; ------------------------
;    196614 + 786448 + 42
; --------------------------------
;  983104 = 0x0000_0000_000f_0040
;
  prep_test_case
  set_reg64 r58, r59, hi=0         , lo=42
  set_reg64  r4,  r5, hi=0x00010002, lo=0x00030004
  dmacwh     r4,  r4, r4
  chk_reg64  r4,  r5, hi=0, lo=0x000f0040

; Test case 3
; Small negatives multiplied with big positives (acc = 0)
;      -32766       32766
; (    0x8002,     0x7ffe) *        r3 (h)
; (0x7fffffff, 0x80000000) +        r2 (w)
;  2147483647  -2147483648
; (           0          )         r58 (acc)
; ------------------------
; (-70364449210368 + -70364449177602 + 0)
; ---------------------------------------
;     -140728898387970 = 0xffff_8002_0000_7ffe
  prep_test_case
  set_reg64 r58, r59, hi=0         , lo=0
  set_reg64  r4,  r5, hi=0x00000000, lo=0x80027ffe
  set_reg64  r2,  r3, hi=0x7fffffff, lo=0x80000000
  dmacwh     r0,  r2, r4
  is_no_flag_set
  chk_reg64  r0,  r1, hi=0xffff8002, lo=0x00007ffe

; Test case 4
; Small negatives multiplied with big positives (acc = very_big)
;      -32766       32766
; (    0x8002,     0x7ffe ) *         r0 (h)
; (0x7fffffff, 0x80000000 ) +         r4 (w)
;  2147483647  -2147483648
; ( 0x0000_7ffd_ffff_8003 )          r58 (acc)
;      140728898387971
; -------------------------
; (-70364449177602 + -70364449210368 + 140728898387971)
; --------------------------------------------------------------------
;                                 1
  prep_test_case
  set_reg64 r58, r59, hi=0x00007ffd, lo=0xffff8003
  set_reg64  r0,  r1, hi=0x00000000, lo=0x80027ffe
  set_reg64  r4,  r5, hi=0x7fffffff, lo=0x80000000
  dmacwh.f   r0,  r4, r0
  is_no_flag_set
  chk_reg64  r0,  r1, hi=0x00000000, lo=0x00000001

; Test case 5
; Bunch of smallest negatives (acc = 0)
;      -32768      -32768
; (    0x8000,     0x8000) *          r1 (h)
; (0x80000000, 0x80000000) +          r3 (w)
;  -2147483648 -2147483648
; (               0              )   r58 (acc)
; --------------------------------
; ( -70368744177664 * 2 + 0)
; --------------------------
;      -140737488355328
;   0x0000_8000_0000_0000
  prep_test_case
  set_reg64 r58, r59, hi=0         , lo=0
  set_reg64  r0,  r1, hi=0x00000000, lo=0x80008000
  set_reg64  r4,  r5, hi=0x80000000, lo=0x80000000
  dmacwh     r4,  r4, r0
  is_no_flag_set
  chk_reg64  r4,  r5, hi=0x00008000, lo=0x00000000

; Test case 6
; Bunch of very small negatives (acc = very_small)
;      -32768      -32768
; (    0x8000,     0x8000 ) *        r8 (h)
; (0x80000000, 0x80000000 ) +        r7 (w)
;  -2147483648 -2147483648
; ( 0x8000_0000_0000_0000 )         r58 (acc)
; -------------------------
; ( -70368744177664 * 2 + -9223372036854775808)
; ---------------------------------------------
; -9223512774343131136 = 0x8000_8000_0000_0000
  prep_test_case
  set_reg64 r58, r59, hi=0x80000000, lo=0x00000000
  set_reg64  r8,  r9, hi=0x00000000, lo=0x80008000
  set_reg64  r6,  r7, hi=0x80000000, lo=0x80000000
  dmacwh     r0,  r6, r8
  is_no_flag_set
  chk_reg64  r0,  r1, hi=0x80008000, lo=0x00000000

; Test case 7
; Biggest positive multiplied with itself (acc = non_zero)
;       32767       32767
; (    0x7fff,     0x7fff ) *         r8 (h)
; (0x7fffffff, 0x7fffffff ) +         r7 (w)
;  2147483647  2147483647
; ( 0x8234_5678_9abc_def0 )          r58 (acc)
; -------------------------
; (70366596661249 * 2 + -9064525073997832464)
; -------------------------------------------
; -9064384340804509966 = 0x8234d6779abbdef2
  prep_test_case
  set_reg64 r58, r59, hi=0x82345678, lo=0x9abcdef0
  set_reg64  r8,  r9, hi=0x00000000, lo=0x7fff7fff
  set_reg64  r6,  r7, hi=0x7fffffff, lo=0x7fffffff
  dmacwh     r8,  r6, r8
  is_no_flag_set
  chk_reg64  r8,  r9, hi=0x8234d677, lo=0x9abbdef2

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
  set_reg64  r0,  r1, hi=0x0       , lo=0
  dmacwh.f   r0,  r0, r0
  is_n_flag_set
  chk_reg64  r0,  r1, hi=0xffffffff, lo=0xffffffff

; Test case 9
; Check if all flags (Z,N,C,V) remain unset.
; (0   , 1  ) *          r5
; (0   , 1  ) +          r5
; (   -1    )            r58 (acc)
; -----------
;      0
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff
  set_reg64  r4,  r5, hi=0x0       , lo=0x00000001
  dmacwh.f   r4,  r4, r4
  is_no_flag_set
  chk_reg64  r4,  r5, hi=0, lo=0

; Test case 10
; Check if V flag is set (negative to positive overflow)
; (0         ,  1         ) *       r2  (h)
; (0         , -1         ) +       r3  (w)
; ( 0x8000_0000_0000_0000 )        r58 (acc)
; -------------------------
; ( -1 + -9223372036854775808 )
; -----------------------------
;    0x7fff_ffff_ffff_ffff
;     9223372036854775807
  prep_test_case
  set_reg64 r58, r59, hi=0x80000000, lo=0x00000000
  set_reg64  r2,  r3, hi=0x00000000, lo=0x0000ffff
  set_reg64  r4,  r5, hi=0x00000000, lo=0x00000001
  dmacwh.f   r2, r4, r2
  is_v_flag_set
  chk_reg64  r2,  r3, hi=0x7fffffff, lo=0xffffffff


; Test case 11
; Check if N and V flags are set (positive to negative overflow)
; (0         ,  1         ) *       r2  (h)
; (0         ,  1         ) +       r2  (w)
; ( 0x7fff_ffff_ffff_ffff )        r58 (acc)
; -------------------------
; ( 1 + 9223372036854775807 )
; ---------------------------
;    0x8000_0000_0000_0000
;    -9223372036854775808
  prep_test_case
  set_reg64 r58, r59, hi=0x7fffffff, lo=0xffffffff
  set_reg64  r2,  r3, hi=0x00000000, lo=0x00000001
  dmacwh.f   r2,  r2, r2
  is_nv_flags_set
  chk_reg64  r2,  r3, hi=0x80000000, lo=0x00000000


; Test case 12
; Nothing is set although there really is an overflow and negative
; (0         ,  1         ) *       r2  (h)
; (0         ,  1         ) +       r2  (w)
; ( 0x7fff_ffff_ffff_ffff )        r58 (acc)
; -------------------------
; ( 1 + 9223372036854775807 )
; ---------------------------
;    0x8000_0000_0000_0000
;    -9223372036854775808
  prep_test_case
  set_reg64 r58, r59, hi=0x7fffffff, lo=0xffffffff
  set_reg64  r2,  r3, hi=0x00000000, lo=0x00000001
  dmacwh     r2,  r2, r2
  is_no_flag_set
  chk_reg64  r2,  r3, hi=0x80000000, lo=0x00000000

; Test case 13
; reg <- reg, u6 (u6 is extended to 16 bits and then replicated 4 times)
;  2147483647  2147483647
; (0x7fffffff, 0x7fffffff) *        r7 (w)
; (  0b111111,   0b111111) +        u6 (h)
;       63          63
; ( 0x0000_0000_0000_0000 )        r58 (acc)
; -------------------------
; ( 135291469761 * 2 + 0 )
; ------------------------
; 0x0000_003e_ffff_ff82
;          270582939522
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg64  r6,  r7, hi=0x7fffffff, lo=0x7fffffff
  dmacwh     r6,  r6, 63
  is_no_flag_set
  chk_reg64  r6,  r7, hi=0x0000003e, lo=0xffffff82

; Test case 14
; reg <- reg, s12 (s12 is sign extended to 16 bits and then replicated 4 times)
; -2147483648  2147483647
; ( 0x80000000, 0x7fffffff) *        r6 (w)
; ( -1        , -1        ) +       s12 (h)
; ( 0x0000_0000_0000_0000 )         r58 (acc)
; -------------------------
; ( 2147483648 - 2147483647 )
; ---------------------------
;   0x0000_0000_0000_0001
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg64  r6,  r7, hi=0x80000000, lo=0x7fffffff
  dmacwh     r6,  r6, -1
  is_no_flag_set
  chk_reg64  r6,  r7, hi=0x00000000, lo=0x00000001

; Test case 15
; reg <- reg, s12 (smallest negative s12)
; -2147483648  2147483647
; (0x80000000, 0x7fffffff ) *        r9 (w)
; ( -2048    , -2048      ) +       s12 (h)
; ( 0x0000_0000_0000_0000 )         r58 (acc)
; -------------------------
; ( 4398046511104 + -4398046509056 )
; ----------------------------------
;       0x0000_0000_0000_0800
;                        2048
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg64 r12, r13, hi=0x80000000, lo=0x7fffffff
  dmacwh    r12, r12, -2048
  is_no_flag_set
  chk_reg64 r12, r13, hi=0x00000000, lo=0x00000800

; Test case 16
; reg <- reg, s12 (biggest positive s12)
; -2147483648  2147483647
; (0x80000000, 0x7fffffff ) *         r9 (w)
; (      2047,       2047 ) +        s12 (h)
; ( 0x0000_0000_0000_0000 )         r58 (acc)
; --------------------------------
; ( 4395899025409 + -4395899027456 )
; ----------------------------------
;      0xffff_ffff_ffff_f801
;                      -2047
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg64  r8,  r9, hi=0x80000000, lo=0x7fffffff
  dmacwh     r8,  r8, 2047
  is_no_flag_set
  chk_reg64  r8,  r9, hi=0xffffffff, lo=0xfffff801

; Test case 17
; check .cc when it is not met, with .f
; ( 0         , 0         ) *         r7 (w)
; ( 0         , 0         ) +        s12 (h)
; ( 0x0000_0000_0000_0001 )          r58 (acc)
; -------------------------
; set n flag
; ----------------
;  0 * 2 + 1 = 1 and n flag unset
; -------------------------------
; conditions not met, n flag should still be set
;
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000001
  set_reg64  r6,  r7, hi=0x00000000, lo=0x00000000
  set_n_flag
  dmacwh.p.f r6,  r6, 0
  is_n_flag_set

; Test case 18
; check .cc when it is met, as well as .f
; ( 0         , 0         ) *        r7 (w)
; ( 0         , 0         ) +       s12 (h)
; ( 0x0000_0000_0000_0001 )         r58 (acc)
; -------------------------
; set n flag
; ----------
;  0 * 2 + 1 = 1 and n flag unset
; -------------------------------
; conditions are met, n flag should not be set
;
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000001
  set_reg64  r6,  r7, hi=0x00000000, lo=0x00000000
  set_n_flag
  dmacwh.n.f r6,  r6, 0
  is_no_flag_set
  chk_reg64  r6,  r7, hi=0x00000000, lo=0x00000001

; Test case 19
; check 0 as register and .f flag
; ( 0         , 0         ) *        r1 (w)
; ( 0         , 0         ) +       s12 (h)
; ( 0x0000_0000_0000_0001 )         r58 (acc)
; -------------------------
; set n flag
; ----------
;  0 * 2 + 1 = 1 and n flag unset
; -------------------------------
; n flag should not be set anymore
;
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000001
  set_reg64  r0,  r1, hi=0x00000000, lo=0x00000000
  set_n_flag
  dmacwh.f    0, r0, 0
  is_no_flag_set

; Test case 20
; check basic limm (limm is duplicated)
; (0x00010001, 0x00010001 ) *         r0 (w)
; (    0x1000,     0x0001 ) +       limm (h)
;       65537       65537
; ( 0x0000_0000_0000_0010 )          r58 (acc)
; -------------------------
;  0x1000 * 0x00010001 + 0x00010001 + 0x10
; ----------------------------------------
;      0x0000000010011011 = 268505089
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000010
  set_reg64  r0,  r1, hi=0x00010001, lo=0x00010001
  dmacwh     r0,  r0, 0x10000001
  is_no_flag_set
  chk_reg64  r0,  r1, hi=0x00000000, lo=0x10011011

; Test case 21
; check double limm (limm is duplicated)
; (    0x1000,     0x0001 ) *       limm
; (0x10000001, 0x10000001 ) +       limm
; ( 0x0000_0000_0000_0010 )         r58 (acc)
; --------------------------------
;  0x1000 * 0x10000001 + 0x10000001 + 0x10
; ----------------
;  0x0000010010001011
;
prep_test_case
  set_reg64  r58, r59, hi=0x00000000, lo=0x00000010
  dmacwh      r0, 0x10000001, 0x10000001
  is_no_flag_set
  chk_reg64   r0,  r1, hi=0x00000100, lo=0x10001011

; Test case 22
; 0 target register with 2 numbers, set flags (only use for this encoding)
; ( 1         , 1         ) *       limm
; ( 1         , 1         ) +       limm
; ( 0x7fff_ffff_ffff_fffe )          r58 (acc)
; -------------------------
;  1 * 1 + 0x7ffffffffffffffe
; ---------------------------
;
prep_test_case
  set_reg64 r58, r59, hi=0x7fffffff, lo=0xfffffffe
  dmacwh.f    0, 1, 1
  is_nv_flags_set
  ; Validate that the accumulator was set accordingly
  xor.f      0,  r58, 0
  bne @fail
  xor.f      0,  r59, 0x80000000
  bne @fail

; Test case 23
; Check if dmacwh unsets n flag but doesnt unset the v flag
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
   dmacwh.f    0, 0, 0
   is_v_flag_set
  ; Validate that the accumulator was set accordingly
  xor.f       0,   r58, 0x89abcdef
  bne @fail
  xor.f       0,   r59, 0x01234567
  bne @fail
; Fall through

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; DMACWHU ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Test case 24
; r1 <- r1, r2
; (     0x0002,     0x0001 ) *      r2, r3 (h)
; ( 0x00000003, 0x00000004 ) +      r0, r1 (w)
; (            0           )       r58, r59 (acc)
; --------------------------
;     (1*3 + 2*4) + 0
; (          0          )
; -----------------------
;         11 (0xa)
  prep_test_case
  set_reg64 r58, r59, hi=0         , lo=0
  set_reg64  r2, r3, hi=0xffffffff, lo=0x00020001 ; hi shouldnt matter
  set_reg64  r0, r1, hi=0x00000003, lo=0x00000004
  dmacwhu    r0, r0, r2
  chk_reg64  r0, r1, hi=0, lo=0xa

; Test case 25
; r3 <- r3, r3
; (     0x0003,     0x0004 ) *      r4, r5 (h)
; ( 0x00010002, 0x00030004 ) +      r4, r5 (w)
;        65538,     196612
; (           42           )       r58, r59 (acc)
; --------------------------
; (65538 * 3 + 196612 * 4) +
; (          42          )
; ------------------------
;    196614 + 786448 + 42
; --------------------------------
;  983104 = 0x0000_0000_000f_0040
;
  prep_test_case
  set_reg64 r58, r59, hi=0         , lo=42
  set_reg64  r4, r5, hi=0x00010002, lo=0x00030004
  dmacwhu    r4, r4, r4
  chk_reg64  r4, r5, hi=0, lo=0x000f0040

; Test case 26
; Big positives multiplied with themselves (acc = 0)
;       65534       65534
; (    0xfffe,     0xfffe) *          r4, r5 (h)
; (0xffffffff, 0xffffffff) +          r2, r3 (w)
;  4294967295  4294967295
; (           0          )           r58, r59 (acc)
; ------------------------
; (281466386710530 * 2 + 0)
; ---------------------------------------
; 562932773421060 = 0x0001_fffb_fffe_0004
  prep_test_case
  set_reg64 r58, r59, hi=0         , lo=0
  set_reg64  r4, r5, hi=0x00000000, lo=0xfffefffe
  set_reg64  r2, r3, hi=0xffffffff, lo=0xffffffff
  dmacwhu    r0, r2, r4
  is_no_flag_set
  chk_reg64  r0, r1, hi=0x0001fffb, lo=0xfffe0004

; Test case 27
; Small negatives multiplied with big positives (acc = very_big)
; Big positives multiplied with themselves (acc = 0)
;       65534       65534
; (    0xfffe,     0xfffe ) *          r0, r1 (h)
; (0xffffffff, 0xffffffff ) +          r4, r5 (w)
;  4294967295  4294967295
; ( 0xfffe_0004_0001_fffd )           r58, r59 (acc)
; ------------------------
; (281466386710530 * 2 + 18446181140936130557)
; ---------------------------------------
; 18446744073709551617 = 0x10000000000000001
; 0x10000000000000001 -> 0x0000000000000001
  prep_test_case
  set_reg64 r58, r59, hi=0xfffe0004, lo=0x0001fffd
  set_reg64  r0, r1, hi=0x00000000, lo=0xfffefffe
  set_reg64  r4, r5, hi=0xffffffff, lo=0xffffffff
  dmacwhu    r0, r4, r0
  is_no_flag_set
  chk_reg64  r0, r1, hi=0x00000000, lo=0x00000001

; Test case 28
; Check if all flags (Z,N,C,V) remain unset.
; ( 0  , 1  ) *           r4, r5
; ( 0  , 1  ) +           r4, r5
; (   -1    )            r58, r59 (acc)
; -----------
;      0
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xfffffffe
  set_reg64  r4, r5, hi=0x0       , lo=0x00000001
  dmacwhu.f  r4, r4, r4
  is_no_flag_set
  chk_reg64  r4, r5, hi=0xffffffff, lo=0xffffffff

; Test case 29
; Check if V flag is set
; ( 0        , 1          ) *        r2, r3  (h)
; ( 0        , 1          ) +        r4, r5  (w)
; ( 0xffff_ffff_ffff_ffff )         r58, r59 (acc)
; -------------------------
; (  1 + 18446744073709551615    )
; -------------------------------
;         0
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff
  set_reg64  r2, r3, hi=0x00000000, lo=0x00000001
  set_reg64  r4, r5, hi=0x00000000, lo=0x00000001
  dmacwhu.f  r2, r4, r2
  is_v_flag_set
  chk_reg64  r2, r3, hi=0x0, lo=0x0

; Test case 30
; Nothing is set although there really is an overflow
; ( 0         , 1         ) *          r2, r3 (h)
; ( 0         , 1         ) +          r2, r3 (w)
; ( 0xffff_ffff_ffff_ffff )           r58, r59 (acc)
; -------------------------
; ( 1 + 18446744073709551615 )
; ----------------------------
;    0x8000_0000_0000_0000
;    -9223372036854775808
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff
  set_reg64  r2, r3, hi=0x00000000, lo=0x00000001
  dmacwhu    r2, r2, r2
  is_no_flag_set
  chk_reg64  r2, r3, hi=0x00000000, lo=0x00000000

; Test case 31
; reg <- reg, u6 (u6 is extended to 16 bits and then replicated 4 times)
;  2147483647  2147483647
; (0xffffffff, 0xffffffff ) *            r6, r7
; (  0b111111,   0b111111 ) +            u6
;       63          63
; ( 0x0000_0000_0000_0000 )             r58, r59 (acc)
; -------------------------
; (   135291469761 * 2 + 0    )
; -----------------------------
;   0x0000_007d_ffff_ff82
;            270582939522
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg64  r6, r7, hi=0xffffffff, lo=0xffffffff
  dmacwhu    r6, r6, 63
  is_no_flag_set
  chk_reg64  r6, r7, hi=0x0000007d, lo=0xffffff82

; Test case 32
; reg <- reg, s12 (s12 is sign extended to 16 bits and then replicated 4 times)
;           1  4294967295
; (0x00000001, 0xffffffff ) *             r6
; (        -1,         -1 ) +            s12
; ( 0x0000_0000_0000_0000 )              r58, r59 (acc)
; -------------------------
; 0xffffffff * 0xffffffff + 0xffffffff
; ------------------------------------
;        0x0000_ffff_0000_0000
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg64  r6, r7, hi=0x00000001, lo=0xffffffff
  dmacwhu    r6, r6, -1
  is_no_flag_set
  chk_reg64  r6, r7, hi=0x0000ffff, lo=0x00000000

; Test case 33
; reg <- reg, s12 (smallest negative s12)
;              4294967295
; (0x00000001, 0xffffffff ) *       r12, r13
; (     -2048,      -2048 ) +       s12
; ( 0x0000_0000_0000_0000 )         r58, r59 (acc)
; -------------------------
; ( 0xf800 * 0xffffffff + 0xf800 )
; -------------------------------
;    0x0000_f800_0000_0000
;          272678883688448
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg64  r12, r13, hi=0x00000001, lo=0xffffffff
  dmacwhu    r12, r12, -2048
  is_no_flag_set
  chk_reg64  r12, r13, hi=0x0000f800, lo=0x00000000

; Test case 34
; reg <- reg, s12 (biggest positive s12)
;              4294967295
; (0x00000001, 0xffffffff ) *          r6, r7
; (      2047,       2047 ) +        s12
; ( 0x0000_0000_0000_0000 )          r58, r59 (acc)
; -------------------------
; ( 2047 * 4294967295 + 2047 )
; ----------------------------
;    0x0000_07ff_0000_0000
;            8791798054912
  prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000000
  set_reg64  r6, r7, hi=0x00000001, lo=0xffffffff
  dmacwhu    r6, r6, 2047
  is_no_flag_set
  chk_reg64  r6, r7, hi=0x000007ff, lo=0x00000000

; Test case 35
; check .cc when it is not met, with .f
; ( 0         , 1         )           r6, r7
; ( 0x0000_0000_0000_0001 )         r58, r59 (acc)
; -------------------------
;  1 + 0xffff_ffff_ffff_ffff = 0 and v flag set
; ---------------------------------------------
; conditions not met, v flag should not be set
;
prep_test_case
  set_reg64  r58, r59, hi=0xffffffff, lo=0xffffffff
  set_reg64   r6, r7, hi=0x00000000, lo=0x00000001
  set_n_flag
  dmacwhu.p.f r6, r6, r6
  is_n_flag_set

; Test case 36
; check .cc when it is met, as well as .f
; ( 0         , 0         ) *          r6
; ( 0         , 0         ) +         s12
; ( 0x0000_0000_0000_0001 )           r58, r59 (acc)
; -------------------------
; set n flag
; ----------
;  1 + 0xffff_ffff_ffff_ffff = 0 and v flag set
; ---------------------------------------------
; conditions are met, v flag should be set, n flag should still be set
;
prep_test_case
  set_reg64  r58, r59, hi=0xffffffff, lo=0xffffffff
  set_reg64   r6, r7, hi=0x00000000, lo=0x00000001
  set_n_flag
  dmacwhu.n.f r6, r6, 1
  is_nv_flags_set
  chk_reg64   r6, r7, hi=0x00000000, lo=0x00000000

; Test case 37
; check basic limm (limm is duplicated)
; (    0x1000,     0x0001 ) *    limm (h)
; (0x00010001, 0x00010001 ) +      r0   (w)
;       65537       65537
; ( 0x0000_0000_0000_0010 )       r58, r59 (acc)
; -------------------------
;  0x1000 * 0x00010001 + 0x00010001 + 0x10
; ----------------------------------------
; 0x0000000010011011 = 268505089
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000010
  set_reg64  r0, r1, hi=0x00010001, lo=0x00010001
  dmacwhu    r0, r0, 0x10000001
  is_no_flag_set
  chk_reg64  r0, r1, hi=0x00000000, lo=0x10011011

; Test case 38
; check double limm (limm is duplicated)
; (    0x1000,     0x0001 ) *     limm
; (0x10000001, 0x10000001 ) +     limm
; ( 0x0000_0000_0000_0010 )        r58, r59 (acc)
; -------------------------
;  0x1000 * 0x10000001 + 0x10000001 + 0x10
; ----------------------------------------
;  0x0000010010001011
;
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000010
  dmacwhu    r0, 0x10000001, 0x10000001
  is_no_flag_set
  chk_reg64  r0, r1, hi=0x00000100, lo=0x10001011

; Test case 39
; 0 target register with 2 numbers, set flags (only use for this encoding)
; ( 1         , 1         ) *     limm
; ( 1         , 1         ) +     limm
; ( 0xffff_ffff_ffff_fffe )        r58, r59 (acc)
; -------------------------
;  1 * 1 + 0xfffffffffffffffe
; ---------------------------
;  0x0
prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xfffffffe
  dmacwhu.f   0, 1, 1
  is_v_flag_set
  ; Validate that the accumulator was set accordingly
  xor.f      0,  r58, 0
  bne @fail
  xor.f      0,  r59, 0
  bne @fail

; Test case 40
; Check if qmachu doesn't unset n flag nor v flag
; ( 0x0000    , 0x0000    )   limm
; ( 0x0000    , 0x0000    )   limm
; ( 0x0123_4567_89ab_cdef )   r58, r59 (acc)
; -------------------------
;  0x0000 * 4 + 0x0123456789abcdef
; --------------------------------
;  0, n flag cleared but v flag untouched
;
 prep_test_case
  set_reg64 r58, r59, hi=0x01234567, lo=0x89abcdef
  set_nv_flags
  dmacwhu.f   0, 0, 0
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
  print " dmacwh / dmacwhu\n"
  flag 1
  end