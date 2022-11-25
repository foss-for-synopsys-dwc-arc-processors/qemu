; check_qmpyh.S
;
; Tests for qmpyh: qmpyh / qmpyhu
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

; reg = 64bit(hi, lo)
.macro set_reg64 reg, hi, lo
  movhl \reg, \hi
  orl   \reg, \reg, \lo
.endm

; reg == 64bit(hi, lo) AND reg == acc
; Clobbers r11
.macro chk_reg64 reg, hi, lo
  movhl  r11, \hi
  orl    r11, r11,  \lo
  xorl.f   0, \reg, r11
  bne    @fail
  xorl.f   0, \reg, r58
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

; The is_x_flag_set macros use "b @fail" instead of
; "bbit ..., @fail" to afford longer ranges to jump.

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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; QMPYH ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test case 1
; r1 <- r1, r1
; (4   , 3   , 2   , 1  ) *      r1
; (4   , 3   , 2   , 1  ) +      r1
; -----------------------
; (4*4 + 3*3 + 2*2 + 1*1)
; -----------------------
;           30
  prep_test_case
  set_reg64 r58, hi=0xffffffff, lo=0xffffffff; Shouldnt matter
  set_reg64  r1, hi=0x00040003, lo=0x00020001
  qmpyh      r1, r1, r1
  chk_reg64  r1, hi=0, lo=30

; Test case 2
; Small negatives multiplied with big positives
;  -32768   32766  -32766   32764
; (0x8000, 0x7ffe, 0x8002, 0x7ffc) *     r3
; (0x7fff, 0x8001, 0x7ffd, 0x8003) +     r2
;   32767  -32767   32765  -32765
; --------------------------------
; (-1073709056 + -1073643522 + -1073577990 + -1073512460)
; -------------------------------------------------------
;             -4294443028 = 0xffff_ffff_0007_ffec
  prep_test_case
  set_reg64  r3, hi=0x80007ffe, lo=0x80027ffc
  set_reg64  r2, hi=0x7fff8001, lo=0x7ffd8003
  qmpyh      r1, r3, r2
  is_no_flag_set
  chk_reg64  r1, hi=0xffffffff, lo=0x0007ffec

; Test case 3
; Small negatives multiplied with big positives
;   32767  -32767   32765  -32765
; (0x7fff, 0x8001, 0x7ffd, 0x8003) *     r0
; (0x8000, 0x7ffe, 0x8002, 0x7ffc) +     r4
;  -32768   32766  -32766   32764
; --------------------------------
; (-1073709056 + -1073643522 + -1073577990 + -1073512460)
; -------------------------------------------------------
;                   0xffffffff0007ffec
;                          -4294443028
  prep_test_case
  set_reg64  r0, hi=0x7fff8001, lo=0x7ffd8003
  set_reg64  r4, hi=0x80007ffe, lo=0x80027ffc
  qmpyh.f    r4, r0, r4
  is_n_flag_set
  chk_reg64  r4, hi=0xffffffff, lo=0x0007ffec

; Test case 4
; Bunch of smallest negatives
;  -32768  -32768  -32768  -32768
; (0x8000, 0x8000, 0x8000, 0x8000) *     r1
; (0x8000, 0x8000, 0x8000, 0x8000) +     r3
;  -32768  -32768  -32768  -32768
; --------------------------------
; (1073741824  + 1073741824  + 1073741824  + 1073741824)
;  0x4000_0000 + 0x4000_0000 + 0x4000_0000 + 0x4000_0000
; ------------------------------------------------------
;                0x0000_0001_0000_0000
  prep_test_case
  set_reg64  r1, hi=0x80008000, lo=0x80008000
  set_reg64  r3, hi=0x80008000, lo=0x80008000
  qmpyh      r1, r1, r3
  is_no_flag_set
  chk_reg64  r1, hi=0x00000001, lo=0x00000000

; Test case 5
; Biggest positive multiplied with itself
;   32767   32767   32767   32767
; (0x7fff, 0x7fff, 0x7fff, 0x7fff)     r9
; ---------------------------------
; (  1073676289 * 4 = 4294705156  )
;        0x0000_0000_fffc_0004
  prep_test_case
  set_reg64  r9, hi=0x7fff7fff, lo=0x7fff7fff
  qmpyh      r9, r9, r9
  is_no_flag_set
  chk_reg64  r9, hi=0x00000000, lo=0xfffc0004

; Test case 6
; Check if N flag is set
; Although the accumulator is already negative, nonetheless the N flag
; must be set.
; (0   , 0   , 0   , 1  ) *      r0
; (0   , 0   , 0   ,-1  ) +      r1
; -----------------------
;           -1
  prep_test_case
  set_reg64  r0, hi=0x00000000, lo=0x0000ffff
  set_reg64  r1, hi=0x00000000, lo=0x00000001
  qmpyh.f    r0, r0, r1
  is_n_flag_set
  chk_reg64  r0, hi=0xffffffff, lo=0xffffffff

; Test case 7
; Check if all flags (Z,N,C,V) remain unset.
; (0   , 0   , 1   , 1  ) *      r5
; (0   , 0   ,-1   , 1  ) +      r6
; -----------------------
;            0
  prep_test_case
  set_reg64  r5, hi=0x0       , lo=0x00010001
  set_reg64  r6, hi=0x0       , lo=0xffff0001
  qmpyh.f    r5, r5, r6
  is_no_flag_set
  chk_reg64  r5, hi=0, lo=0

; Test case 8
; Nothing is set although the result is negative
; (0    , 0   , 0   , 1   ) *     r2
; (0    , 0   , 0   ,-1   ) +     r3
; ---------------------------
;             -1
  prep_test_case
  set_reg64  r2, hi=0x00000000, lo=0x00000001
  set_reg64  r3, hi=0x00000000, lo=0x0000ffff
  qmpyh      r2, r2, r3
  is_no_flag_set
  chk_reg64  r2, hi=0xffffffff, lo=0xffffffff

; Test case 9
; reg <- reg, u6 (u6 is extended to 16 bits and then replicated 4 times)
;     32767     32767     32767     32767
; (  0x7fff,   0x7fff,   0x7fff,   0x7fff) *     r7
; (0b111111, 0b111111, 0b111111, 0b111111) +     u6
;      63  ,     63  ,     63  ,     63
; ----------------------------------------
; (              2064321 * 4             )
; ----------------------------------------
;          0x0000_0000_007d_ff04
;                  8257284
  prep_test_case
  set_reg64  r7, hi=0x7fff7fff, lo=0x7fff7fff
  qmpyh      r6, r7, 63
  is_no_flag_set
  chk_reg64  r6, hi=0x00000000, lo=0x007dff04

; Test case 10
; reg <- reg, s12 (s12 is sign extended to 16 bits and then replicated 4 times)
;   32767   32767    -32768    -32768
; (0x7fff, 0x7fff  , 0x8000  , 0x8000  ) *        r6
; (    -1,     -1  ,     -1  ,     -1  ) +       s12
; --------------------------------------
; (      -32767 * 2 + 32768 * 2        )
; --------------------------------------
;          0x0000_0000_0000_0002
  prep_test_case
  set_reg64  r6, hi=0x7fff7fff, lo=0x80008000
  qmpyh      r6, r6, -1
  is_no_flag_set
  chk_reg64  r6, hi=0x00000000, lo=0x00000002

; Test case 11
; reg <- reg, s12 (smallest negative s12)
;   32767   32767    -32768    -32768
; (0x7fff, 0x7fff  , 0x8000  , 0x8000  ) *        r9
; ( -2048,  -2048  ,  -2048  ,  -2048  ) +       s12
; --------------------------------------
; (   -67106816 * 2 + 67108864 * 2    )
; (      -134213632 + 134217728       )
; -------------------------------------
;       0x0000_0000_0000_1000
;                4096
  prep_test_case
  set_reg64  r9, hi=0x7fff7fff, lo=0x80008000
  qmpyh      r9, r9, -2048
  is_no_flag_set
  chk_reg64  r9, hi=0x00000000, lo=0x00001000

; Test case 12
; reg <- reg, s12 (biggest positive s12)
;    32767  32767    -32768    -32768
; (0x7fff, 0x7fff  , 0x8000  , 0x8000  ) *      r7
; (  2047,   2047  ,   2047  ,   2047  ) +     s12
; --------------------------------------
; (   67074049 * 2 + -67076096 * 2    )
; (      -134213632 + 134217728       )
; -------------------------------------
;        0xffff_ffff_ffff_f002
;               -4096
  prep_test_case
  set_reg64  r7, hi=0x7fff7fff, lo=0x80008000
  qmpyh      r7, r7, 2047
  is_no_flag_set
  chk_reg64  r7, hi=0xffffffff, lo=0xfffff002

; Test case 13
; check .cc when it is not met, with .f
; (0   , 0    , 0   , 1   ) *     r7
; (1   , 1    , 1   , 1   ) +     u6
; -------------------------
; set n flag
; -------------------------------
;  1 * 1 + 0 * 3 = 1 and n flag unset
; -------------------------------
; conditions not met, n flag should still be set
prep_test_case
  set_reg64  r7, hi=0x00000000, lo=0x00000001
  set_n_flag
  qmpyh.p.f  r7, r7, 1
  is_n_flag_set

; Test case 14
; check .cc when it is met, as well as .f
; (0   , 0    , 0   , 1   ) *      r7
; (1   , 1    , 1   , 1   ) +      u6
; -------------------------
; set n flag
; -------------------------------
;  1 * 1 + 0 * 3 = 1 and n flag unset
; -------------------------------
; conditions are met, n flag should not be set
prep_test_case
  set_reg64  r7, hi=0x00000000, lo=0x00000001
  set_n_flag
  qmpyh.n.f  r7, r7, 1
  is_no_flag_set
  chk_reg64  r7, hi=0x00000000, lo=0x00000001

; Test case 15
; check 0 as register and .f flag
; (0    , 0   , 0   , 0   ) *       r1
; (0    , 0   , 0   , 0   ) +       u6
; -------------------------
; set n flag
; --------------------------------
;  0 * 4 = 0 and n flag unset
; --------------------------------
; n flag should not be set anymore
prep_test_case
  set_reg64  r1, hi=0x00000000, lo=0x00000000
  set_n_flag
  qmpyh.f    0, r1, 0
  is_no_flag_set

; Test case 16
; check basic limm (limm is duplicated)
; (     1,      1,      1,      1) *       r0
; (0x1000, 0x0001, 0x1000, 0x0001) +     limm
; --------------------------------
;      0x1000 * 2 + 0x0001 * 2
; --------------------------------
;               0x2002
prep_test_case
  set_reg64  r0, hi=0x00010001, lo=0x00010001
  qmpyh      r0, r0, 0x10000001
  is_no_flag_set
  chk_reg64  r0, hi=0x00000000, lo=0x00002002

; Test case 17
; check double limm (limm is duplicated)
; (0x1000, 0x0001, 0x1000, 0x0001) *     limm
; (0x1000, 0x0001, 0x1000, 0x0001) +     limm
; --------------------------------
;  0x1000 * 0x1000 * 2 + 0x0001 * 0x0001 * 2
; -------------------------------------------------
;               0x0000000002000002
prep_test_case
  set_reg64 r58, hi=0x00000000, lo=0x00000010
  qmpyh      r0, 0x10000001, 0x10000001
  is_no_flag_set
  chk_reg64  r0, hi=0x00000000, lo=0x02000002

; Test case 18
; 0 target register with 2 numbers, qmpyh should clear v flag and unset n flag
; (0x0000, 0x0001, 0x0000, 0x0001) *     limm
; (0x0000, 0x0001, 0x0000, 0x0001) +     limm
; --------------------------------
;    1 * 1 * 2 = 2
prep_test_case
  qmpyh.f     0, 1, 1
  is_no_flag_set
  ; Validate that the accumulator was set accordingly
  xor.f      0, r58, 0x2
  bne @fail
; Fall through

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Reporting ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

valhalla:
  print "[PASS]"
  b @1f

; If a test fails, it jumps here. Although, for the sake of uniformity,
; the printed output does not say much about which test case failed,
; one can uncomment the print_number line below or set a breakpoint
; here to check the R0 register for the test case number.
fail:
  ld r0, [test_nr]
  ; print_number r0
  print "[FAIL]"
1:
  print " qmpyh\n"
  flag 1
  end
