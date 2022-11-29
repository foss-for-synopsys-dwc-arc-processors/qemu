; check_vmpy2h.S
;
; Tests for vmpy2h / vmpy2hu
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VMPY2H ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test case 1
; r1 <- r1, r2
; ( 0x0003, 0x0004 ) *      r1
; ( 0x0003, 0x0004 ) +      r1
; ------------------
;   3 * 3 ,  4 * 4
; ----------------
;     9       16
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff ; acc shouldnt matter
  set_reg64  r0, r1, hi=0xffffffff, lo=0x00030004 ; hi shouldnt matter
  vmpy2h     r0, r0, r0
  chk_reg64  r0, r1, hi=0x9, lo=0x10

; Test case 2
; Small negatives multiplied with big positives
;        -32766         32766
; (      0x8002 ,      0x7ffe  ) *      r3
; (      0x7fff ,      0x8000  ) +      r2
;         32767        -32768
; -----------------------------
; 32767 * -32766, -32768 * 32766
;    -1073643522,    -1073676288
;     0xc0017ffe,     0xc0010000
  prep_test_case
  set_reg32  r3, 0x80027ffe
  set_reg32  r2, 0x7fff8000
  vmpy2h     r0, r2, r3
  chk_reg64  r0, r1, hi=0xc0017ffe, lo=0xc0010000

; Test case 3
; Bunch of smallest negatives
;   -32768     -32768
; ( 0x8000   , 0x8000   ) *          r1
; ( 0x8000   , 0x8000   ) +          r3
; -----------------------
; 1073741824 , 1073741824
; 0x40000000 , 0x40000000
  prep_test_case
  set_reg32  r1, 0x80008000
  set_reg32  r3, 0x80008000
  vmpy2h     r2, r3, r1
  chk_reg64  r2, r3, hi=0x40000000, lo=0x40000000

; Test case 4
; Biggest positive multiplied with itself
;    32767     32767
; ( 0x7fff  , 0x7fff   ) *         r8
; ( 0x7fff  , 0x7fff   ) +         r7
; ----------------------
; 1073676289, 1073676289
; 0x3fff0001, 0x3fff0001
  prep_test_case
  set_reg32  r8, 0x7fff7fff
  set_reg32  r7, 0x7fff7fff
  vmpy2h     r8, r7, r8
  chk_reg64  r8, r9, hi=0x3fff0001, lo=0x3fff0001

; Test case 5
; reg <- reg, u6 (u6 is extended to 16 bits and then replicated 4 times)
;       32767       32767
; (    0x7fff,     0x7fff) *        r7
; (  0b111111,   0b111111) +        u6
;       63          63
; -------------------------
; 32767 * 63 ,  32767 * 63
;    2064321 ,     2064321
; 0x001f7fc1 ,  0x001f7fc1
  prep_test_case
  set_reg32  r7, 0x7fff7fff
  vmpy2h     r6, r7, 63
  chk_reg64  r6, r7, hi=0x001f7fc1, lo=0x001f7fc1

; Test case 6
; reg <- reg, s12 (s12 is sign extended to 16 bits and then replicated 4 times)
;   -32768   32767
; ( 0x8000, 0x7fff) *        r6
; ( -1    , -1    ) +       s12
; -------------------------
;    32768, -32767
  prep_test_case
  set_reg32  r6, 0x80007fff
  vmpy2h     r6, r6, -1
  chk_reg64  r6, r7, hi=0x00008000, lo=0xffff8001

; Test case 7
; reg <- reg, s12 (smallest negative s12)
;    -32768      32767
; (  0x8000  ,  0x7fff  ) *        r9
; (   -2048  ,   -2048  ) +       s12
; -----------------------
;  67108864  , -67106816
; 0x04000000 , 0xfc000800
  prep_test_case
  set_reg32  r12, 0x80007fff
  vmpy2h     r12, r12, -2048
  chk_reg64  r12, r13, hi=0x04000000, lo=0xfc000800

; Test case 8
; reg <- reg, s12 (biggest positive s12)
;    -32768      32767
; (  0x8000  ,  0x7fff  ) *        r9
; (    2047  ,    2047  ) +        s12
; -----------------------
; -67076096  ,  67074049
; 0xfc008000 , 0x03ff7801
  prep_test_case
  set_reg32  r6, lo=0x80007fff
  vmpy2h     r6, r6, 2047
  chk_reg64  r6, r7, hi=0xfc008000, lo=0x03ff7801

; Test case 9
; check .cc when it is not met
; ( 0         , 1         ) *         r7
; ( 2         , 2         ) +        s12
; -------------------------
; set n flag
; ----------------
; ( 0, 2 ) if n flag isnt set
prep_test_case
  set_reg64  r4, r5, hi=0x00000000, lo=0x00000001
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000001
  set_n_flag
  vmpy2h.p   r4, r4, 2
  chk_reg64  r4, r5, hi=0x00000000, lo=0x00000001

; Test case 10
; check .cc when it is met, as well as .f
; ( 0         , 1         ) *        r7
; ( 2         , 2         ) +       s12
; -------------------------
;  0 * 2 + 1 = 1 and n flag unset
; -------------------------------
; conditions are met, n flag should not be set
;
prep_test_case
  set_reg64 r4, r5, hi=0x00000000, lo=0x00000001
  set_n_flag
  vmpy2h.n  r4, r4, 2
  chk_reg64 r4, r5, hi=0x00000000, lo=0x00000002

; Test case 11
; check basic limm (limm is duplicated)
;    4096       1
; ( 0x1000 , 0x0001 ) *         r0
; ( 0x1000 , 0x0001 ) +       limm
; -------------------
;  16777216,     1
; 0x1000000,   0x1
prep_test_case
  set_reg32  r0, 0x10000001
  vmpy2h     r0, r0, 0x10000001
  chk_reg64  r0, r1, hi=0x01000000, lo=0x00000001

; Test case 12
; check double limm (limm is duplicated)
; (0x1000, 0x0001 ) *       limm
; (0x1000, 0x0001 ) +       limm
; --------------------------------
;  0x1000 * 0x10000001 + 0x10000001 + 0x10
; ----------------
;  0x0000010010001011
;
prep_test_case
  vmpy2h      r0, 0x10000001, 0x10000001
  chk_reg64  r0, r1, hi=0x01000000, lo=0x00000001

; Test case 13
; 0 target register with 2 numbers (only use for this encoding)
; ( 0         , 1         ) *       limm
; ( 0         , 1         ) +       limm
; -------------------------
;   0         , 1
;
prep_test_case
  vmpy2h    r0, 1, 1
  ; Validate that the accumulator was set accordingly
  mov       r11, 0x1
  xor.f      0,   r58, r11
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
  print " vmpy2h / vmpy2hu\n"
  flag 1
  end