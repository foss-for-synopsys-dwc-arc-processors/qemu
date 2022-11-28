; check_dmpywh.S
;
; Tests for dmpywh / dmpywhu
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
.macro set_reg32 reg, lo
  mov   \reg, \lo
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; DMPYWH ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test case 1
; r1 <- r1, r2
; (     0x0002,     0x0001 ) *      r2 (h)
; ( 0x00000003, 0x00000004 ) +      r1 (w)
; --------------------------
;        (1*3 + 2*4)
; -----------------------
;         11 (0xa)
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff ; acc shouldnt matter
  set_reg64  r2,  r3, hi=0xffffffff, lo=0x00020001 ; hi shouldnt matter
  set_reg64  r0,  r1, hi=0x00000003, lo=0x00000004
  dmpywh     r0,  r0, r2
  chk_reg64  r0,  r1, hi=0, lo=0xa

; Test case 2
; r3 <- r3, r3
; (     0x0003,     0x0004 ) *      r3 (h)
; ( 0x00010002, 0x00030004 ) +      r3 (w)
;        65538,     196612
; --------------------------
; (65538 * 3 + 196612 * 4)
; ------------------------
;    196614 + 786448
; --------------------------------
;  983104 = 0x0000_0000_000f_0040
;
  prep_test_case
  set_reg64  r4, r5, hi=0x00010002, lo=0x00030004
  dmpywh     r4, r4, r4
  chk_reg64  r4, r5, hi=0, lo=0x000f0016

; Test case 3
; Small negatives multiplied with big positives
;      -32766       32766
; (    0x8002,     0x7ffe) *        r3 (h)
; (0x7fffffff, 0x80000000) +        r2 (w)
;  2147483647  -2147483648
; ------------------------
; (-70364449210368 + -70364449177602)
; ----------------------------------------
; -140728898387970 = 0xffff_8002_0000_7ffe
  prep_test_case
  set_reg64  r4, r5, hi=0x00000000, lo=0x80027ffe
  set_reg64  r2, r3, hi=0x7fffffff, lo=0x80000000
  dmpywh     r0, r2, r4
  is_no_flag_set
  chk_reg64  r0, r1, hi=0xffff8002, lo=0x00007ffe

; Test case 4
; Bunch of smallest negatives
;      -32768      -32768
; (    0x8000,     0x8000) *          r1 (h)
; (0x80000000, 0x80000000) +          r3 (w)
;  -2147483648 -2147483648
; ------------------------
; ( -70368744177664 * 2 )
; -----------------------
;    -140737488355328
; 0x0000_8000_0000_0000
  prep_test_case
  set_reg64  r0, r1, hi=0x00000000, lo=0x80008000
  set_reg64  r4, r5, hi=0x80000000, lo=0x80000000
  dmpywh     r4, r4, r0
  is_no_flag_set
  chk_reg64  r4, r5, hi=0x00008000, lo=0x00000000

; Test case 5
; Biggest positive multiplied with itself
;       32767       32767
; (    0x7fff,     0x7fff ) *         r8 (h)
; (0x7fffffff, 0x7fffffff ) +         r7 (w)
;  2147483647  2147483647
; -------------------------
; (2147483647 * 32767 * 2)
; -------------------------------------------
; 140733193322498 = 0x0000_7ffe_ffff_0002
  prep_test_case
  set_reg32  r8, 0x7fff7fff
  set_reg64  r6, r7, hi=0x7fffffff, lo=0x7fffffff
  dmpywh     r8, r6, r8
  is_no_flag_set
  chk_reg64  r8, r9, hi=0x00007ffe, lo=0xffff0002

; Test case 6
; Check if N flag is set
; (0   , 1  ) *          r0
; (0   ,-1  ) +          r1
; -----------
;     -1
  prep_test_case
  set_reg64  r0, r1, hi=0x0       , lo=0x00000001
  set_reg64  r2, r3, hi=0x0       , lo=0x0000ffff
  dmpywh.f   r0, r0, r2
  is_n_flag_set
  chk_reg64  r0, r1, hi=0xffffffff, lo=0xffffffff

; Test case 7
; Check if all flags (Z,N,C,V) remain unset.
; ( 1  , 1  ) *          r5
; (-1  , 1  ) +          r6
; -----------
;      0
  prep_test_case
  set_reg64  r6, r7, hi=0x00000000, lo=0x00010001
  set_reg64  r8, r9, hi=0xffffffff, lo=0x00000001
  dmpywh.f   r8, r8, r6
  is_no_flag_set
  chk_reg64  r8, r9, hi=0, lo=0

; Test case 8
; Nothing is set although the result is negative
; (0         ,  1         ) *       r2  (h)
; (0         , -1         ) +       r3  (w)
; -------------------------
;           -1
  prep_test_case
  set_reg64  r2, r3, hi=0x00000000, lo=0x00000001
  set_reg64  r4, r5, hi=0x00000000, lo=0xffffffff
  dmpywh     r2, r2, r4
  is_no_flag_set
  chk_reg64  r2, r3, hi=0xffffffff, lo=0xffffffff

; Test case 9
; reg <- reg, u6 (u6 is extended to 16 bits and then replicated 4 times)
;  2147483647  2147483647
; (0x7fffffff, 0x7fffffff) *        r7 (w)
; (  0b111111,   0b111111) +        u6 (h)
;       63          63
; -------------------------
; (   135291469761 * 2   )
; ------------------------
; 0x0000_003e_ffff_ff82
;          270582939522
  prep_test_case
  set_reg64  r6, r7, hi=0x7fffffff, lo=0x7fffffff
  dmpywh     r6, r6, 63
  is_no_flag_set
  chk_reg64  r6, r7, hi=0x0000003e, lo=0xffffff82

; Test case 10
; reg <- reg, s12 (s12 is sign extended to 16 bits and then replicated 4 times)
; -2147483648  2147483647
; ( 0x80000000, 0x7fffffff) *        r6 (w)
; ( -1        , -1        ) +       s12 (h)
; -------------------------
; ( 2147483648 - 2147483647 )
; ---------------------------
;   0x0000_0000_0000_0001
  prep_test_case
  set_reg64  r6, r7, hi=0x80000000, lo=0x7fffffff
  dmpywh     r6, r6, -1
  is_no_flag_set
  chk_reg64  r6, r7, hi=0x00000000, lo=0x00000001

; Test case 11
; reg <- reg, s12 (smallest negative s12)
; -2147483648  2147483647
; (0x80000000, 0x7fffffff ) *        r9 (w)
; ( -2048    , -2048      ) +       s12 (h)
; -------------------------
; ( 4398046511104 + -4398046509056 )
; ----------------------------------
;       0x0000_0000_0000_0800
;                        2048
  prep_test_case
  set_reg64  r12, r13, hi=0x80000000, lo=0x7fffffff
  dmpywh     r12, r12, -2048
  is_no_flag_set
  chk_reg64  r12, r13, hi=0x00000000, lo=0x00000800

; Test case 12
; reg <- reg, s12 (biggest positive s12)
; -2147483648  2147483647
; (0x80000000, 0x7fffffff ) *         r9 (w)
; (      2047,       2047 ) +        s12 (h)
; --------------------------------
; ( 4395899025409 + -4395899027456 )
; ----------------------------------
;      0xffff_ffff_ffff_f801
;                      -2047
  prep_test_case
  set_reg64  r6, r7, hi=0x80000000, lo=0x7fffffff
  dmpywh     r6, r6, 2047
  is_no_flag_set
  chk_reg64  r6, r7, hi=0xffffffff, lo=0xfffff801

; Test case 13
; check .cc when it is not met, with .f
; ( 0         , 1         ) *         r7 (w)
; ( 1         , 1         ) +        s12 (h)
; ( 0x0000_0000_0000_0001 )          r58 (acc)
; -------------------------
; set n flag
; ----------------
;  0 * 2 + 1 = 1 and n flag unset
; -------------------------------
; conditions not met, n flag should still be set
;
prep_test_case
  set_reg64  r8, r9, hi=0x00000000, lo=0x00000001
  set_n_flag
  dmpywh.p.f r8, r8, 1
  is_n_flag_set

; Test case 14
; check .cc when it is met, as well as .f
; ( 0         , 1         ) *        r7 (w)
; ( 1         , 1         ) +       s12 (h)
; ( 0x0000_0000_0000_0001 )         r58 (acc)
; -------------------------
; set n flag
; ----------
;  0 * 2 + 1 = 1 and n flag unset
; -------------------------------
; conditions are met, n flag should not be set
;
prep_test_case
  set_reg64  r8, r9, hi=0x00000000, lo=0x00000001
  set_n_flag
  dmpywh.n.f r8, r8, 1
  is_no_flag_set
  chk_reg64  r8, r9, hi=0x00000000, lo=0x00000001

; Test case 15
; check 0 as register and .f flag
; ( 0         , 0         ) *        r1 (w)
; ( 0         , 0         ) +       s12 (h)
; -------------------------
; set n flag
; ----------
;  0 * 2 + 1 = 1 , n and v flags unset
; -------------------------------
; n flag should not be set anymore
;
prep_test_case
  set_reg64  r0, r1, hi=0x00000000, lo=0x00000000
  set_nv_flags
  dmpywh.f    0, r0, 0
  is_no_flag_set

; Test case 16
; check basic limm (limm is duplicated)
; (0x00010001, 0x00010001 ) *         r0 (w)
; (    0x1000,     0x0001 ) +       limm (h)
;       65537       65537
; -------------------------
;  0x1000 * 0x00010001 + 0x00010001
; ---------------------------------
;   0x0000000010011011 = 268505089
prep_test_case
  set_reg64  r0, r1, hi=0x00010001, lo=0x00010001
  dmpywh     r0, r0, 0x10000001
  is_no_flag_set
  chk_reg64  r0, r1, hi=0x00000000, lo=0x10011001

; Test case 17
; check double limm (limm is duplicated)
; (    0x1000,     0x0001 ) *       limm
; (0x10000001, 0x10000001 ) +       limm
; --------------------------------
;  0x1000 * 0x10000001 + 0x10000001 + 0x10
; ----------------
;  0x0000010010001011
;
prep_test_case
  dmpywh      r0, 0x10000001, 0x10000001
  is_no_flag_set
  chk_reg64   r0, r1, hi=0x00000100, lo=0x10001001

; Test case 18
; 0 target register with 2 numbers, set flags (only use for this encoding)
; ( 1         , 1         ) *       limm
; ( 1         , 1         ) +       limm
; -------------------------
;         1 * 1 * 2
;
prep_test_case
  dmpywh.f    0, 1, 1
  is_no_flag_set
  ; Validate that the accumulator was set accordingly
  xor.f      0,   r58, 2
  bne @fail

; Test case 19
; Check if dmpywh unsets n flag but doesnt unset the v flag
; ( 1         , 1         ) *       limm
; ( 1         , 1         ) +       limm
; -----------------------------------------------
; 1 * 1 * 2 , n and flags cleared
;
prep_test_case
  set_nv_flags
  dmpywh.f    0, 1, 1
  is_no_flag_set
  ; Validate that the accumulator was set accordingly
  xor.f       0,   r58, 2
  bne @fail

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; DMPYWHU ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Test case 20
; r1 <- r1, r2
; (     0x0002,     0x0001 ) *      r2 (h)
; ( 0x00000003, 0x00000004 ) +      r1 (w)
; --------------------------
;        (1*3 + 2*4)
; -----------------------
;         11 (0xa)
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff ; acc shouldnt matter
  set_reg64  r2, r3, hi=0xffffffff, lo=0x00020001 ; hi shouldnt matter
  set_reg64  r0, r1, hi=0x00000003, lo=0x00000004
  dmpywhu    r0, r0, r2
  chk_reg64  r0, r1, hi=0, lo=0xa

; Test case 21
; r3 <- r3, r3
; (     0x0003,     0x0004 ) *      r3 (h)
; ( 0x00010002, 0x00030004 ) +      r3 (w)
;        65538,     196612
; --------------------------
; (65538 * 3 + 196612 * 4)
; ------------------------
;    196614 + 786448
; --------------------------------
;  983104 = 0x0000_0000_000f_0040
;
  prep_test_case
  set_reg64  r4, r5, hi=0x00010002, lo=0x00030004
  dmpywhu    r4, r4, r4
  chk_reg64  r4, r5, hi=0, lo=0x000f0016

; Test case 22
; Biggest positive multiplied with itself
;       65535       65535
; (    0xffff,     0xffff ) *         r8 (h)
; (0xffffffff, 0xffffffff ) +         r7 (w)
;  4294967295  4294967295
; -------------------------
; (4294967295 * 65535 * 2)
; -------------------------------------------
; 562941363355650 = 0x0001_fffd_fffe_0002
  prep_test_case
  set_reg32  r9, lo=0xffffffff
  set_reg64  r6, r7, hi=0xffffffff, lo=0xffffffff
  dmpywhu    r8, r6, r9
  is_no_flag_set
  chk_reg64  r8, r9, hi=0x0001fffd, lo=0xfffe0002

; Test case 23
; reg <- reg, u6 (u6 is extended to 16 bits and then replicated 4 times)
;  4294967295  4294967295
; (0xffffffff, 0xffffffff) *        r7 (w)
; (  0b111111,   0b111111) +        u6 (h)
;       63          63
; -------------------------
; (  4294967295 * 63 * 2  )
; -------------------------
; 0x0000_007d_ffff_ff82
;          541165879170
  prep_test_case
  set_reg64  r6, r7, hi=0xffffffff, lo=0xffffffff
  dmpywhu    r6, r6, 63
  is_no_flag_set
  chk_reg64  r6, r7, hi=0x0000007d, lo=0xffffff82

; Test case 24
; reg <- reg, s12 (s12 is sign extended to 16 bits and then replicated 4 times)
;   4294967295  4294967295
; ( 0xffffffff, 0xffffffff) *        r6 (w)
; ( -1        , -1        ) +       s12 (h)
;   65535       65535
; -------------------------
; ( 4294967295 * 65535 * 2 )
; ---------------------------
;   0x0001_fffd_fffe_0002
;         562941363355650
  prep_test_case
  set_reg64  r6, r7, hi=0xffffffff, lo=0xffffffff
  dmpywhu    r6, r6, -1
  is_no_flag_set
  chk_reg64  r6, r7, hi=0x0001fffd, lo=0xfffe0002

; Test case 25
; reg <- reg, s12 (smallest negative s12)
;   4294967295  4294967295
; ( 0xffffffff, 0xffffffff) *        r9 (w)
; ( -2048    , -2048      ) +       s12 (h)
;   63488      63488
; --------------------------
; ( 4294967295 * 63488 * 2 )
; --------------------------
;   0x0001_efff_fffe_1000
;         545357767249920
  prep_test_case
  set_reg64  r8, r9, hi=0xffffffff, lo=0xffffffff
  dmpywhu    r8, r8, -2048
  is_no_flag_set
  chk_reg64  r8, r9, hi=0x0001efff, lo=0xfffe1000

; Test case 26
; reg <- reg, s12 (biggest positive s12)
;   4294967295  4294967295
; ( 0xffffffff, 0xffffffff) *        r9 (w)
; (      2047,       2047 ) +        s12 (h)
; -------------------------
; ( 4294967295 * 2047 * 2 )
; -------------------------
;     0x0000_0ffd_ffff_f002
;            17583596105730
  prep_test_case
  set_reg64  r6, r7, hi=0xffffffff, lo=0xffffffff
  dmpywhu    r6, r6, 2047
  is_no_flag_set
  chk_reg64  r6, r7, hi=0x00000ffd, lo=0xfffff002

; Test case 27
; check .cc when it is not met, with .f
; ( 0         , 1         ) *         r7 (w)
; ( 1         , 1         ) +        s12 (h)
; -------------------------
; set n flag
; ----------------
;  0 * 2 + 1 = 1 and n flag unset
; -------------------------------
; conditions not met, n flag should still be set
;
prep_test_case
  set_reg64  r6, r7, hi=0x00000000, lo=0x00000001
  set_nv_flags
  dmpywhu.p.f r6, r6, 1
  is_nv_flags_set

; Test case 28
; check .cc when it is met, as well as .f
; ( 0         , 1         ) *        r7 (w)
; ( 1         , 1         ) +       s12 (h)
; -------------------------
; set n flag
; ----------
;  0 * 2 + 1 = 1 and n flag unset
; -------------------------------
; conditions are met, n flag should not be set
;
prep_test_case
  set_reg64  r6, r7, hi=0x00000000, lo=0x00000001
  set_nv_flags
  dmpywhu.n.f r6, r6, 1
  is_n_flag_set
  chk_reg64  r6, r7, hi=0x00000000, lo=0x00000001

; Test case 29
; check 0 as register and .f flag
; ( 0         , 0         ) *        r1 (w)
; ( 0         , 0         ) +       s12 (h)
; ( 0x0000_0000_0000_0001 )         r58 (acc)
; -------------------------
; set n flag
; ----------
;  0 * 2 + 1 = 1 , v flag unset
; -------------------------------
; v flag should not be set anymore
;
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000001
  set_reg64  r0, r1, hi=0x00000000, lo=0x00000000
  set_nv_flags
  dmpywhu.f    0, r0, 0
  is_n_flag_set

; Test case 30
; check basic limm (limm is duplicated)
; (0x00010001, 0x00010001 ) *         r0 (w)
; (    0x1000,     0x0001 ) +       limm (h)
;       65537       65537
; -------------------------
;  0x1000 * 0x00010001 + 0x00010001
; ---------------------------------
;   0x0000000010011011 = 268505089
prep_test_case
  set_reg64  r0, r1, hi=0x00010001, lo=0x00010001
  dmpywhu    r0, r0, 0x10000001
  is_no_flag_set
  chk_reg64  r0, r1, hi=0x00000000, lo=0x10011001

; Test case 31
; check double limm (limm is duplicated)
; (    0x1000,     0x0001 ) *       limm
; (0x10000001, 0x10000001 ) +       limm
; --------------------------------
;  0x1000 * 0x10000001 + 0x10000001
; ----------------
;  0x0000010010001001
;
prep_test_case
  dmpywhu     r0, 0x10000001, 0x10000001
  is_no_flag_set
  chk_reg64   r0, r1, hi=0x00000100, lo=0x10001001

; Test case 32
; 0 target register with 2 numbers, set flags (only use for this encoding)
; ( 1         , 1         ) *       limm
; ( 1         , 1         ) +       limm
; -------------------------
;         1 * 1 * 2
;
prep_test_case
  dmpywhu.f    0, 1, 1
  is_no_flag_set
  ; Validate that the accumulator was set accordingly
  xor.f      0,   r58, 2
  bne @fail

; Test case 33
; Check if dmpywhu unsets the v flag
; ( 1         , 1         ) *       limm
; ( 1         , 1         ) +       limm
; -----------------------------------------------
; 1 * 1 * 2 , n and flags cleared
;
prep_test_case
  set_nv_flags
  dmpywhu.f    0, 1, 1
  is_n_flag_set
  ; Validate that the accumulator was set accordingly
  xor.f       0,   r58, 2
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
  print " dmpywh / dmpywhu\n"
  flag 1
  end