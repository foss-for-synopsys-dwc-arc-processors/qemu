; check_dmpyh.S
;
; Tests for dmpyh
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; DMPYH ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test case 1
; r1 <- r1, r2
; ( 0x0002, 0x0001 ) *      r2
; ( 0x0003, 0x0004 ) +      r1
; ------------------
; (2*3 + 1*4)
; -----------
;  10 (0xa)
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff ; acc should not matter
  set_reg32  r2, 0x00020001
  set_reg32  r1, 0x00030004
  dmpyh      r1, r1, r2
  chk_reg32  r1, 0xa
  chk_reg64 r58, r59, hi=0, lo=0xa
  
; Test case 2
; Small negatives multiplied with big positives
;   -32766        32766
; ( 0x8002     , 0x7ffe     ) *        r3
; ( 0x7fff     , 0x8000     ) +        r2
;    32767       -32768
; ---------------------------
; (-1073643522 + -1073676288)
; -------------------------------------
;  -2147319810 = 0xffff_ffff_8002_7ffe
  prep_test_case
  set_reg32  r3, 0x80027ffe
  set_reg32  r2, 0x7fff8000
  dmpyh      r1, r2, r3
  is_no_flag_set
  chk_reg32  r1, lo=0x80027ffe
  chk_reg64 r58, r59, hi=0xffffffff, lo=0x80027ffe

; Test case 3
; Bunch of smallest negatives
;  -32768  -32768
; (0x8000, 0x8000) *          r1
; (0x8000, 0x8000) +          r3
;  -32768  -32768
; ----------------
; (  2147483648  )
;  0x0000_0000_4000_0000
  prep_test_case
  set_reg32  r1, 0x80008000
  set_reg32  r3, 0x80008000
  dmpyh      r3, r3, r1
  is_no_flag_set
  chk_reg32  r3, lo=0x80000000
  chk_reg64 r58, r59, hi=0x00000000, lo=0x80000000

; Test case 4
; Biggest positives
;    32767      32767
; ( 0x7fff   , 0x7fff   )      r8
; -----------------------
; 2147352578 = 0x7ffe0002
  prep_test_case
  set_reg32  r8, 0x7fff7fff
  dmpyh      r9, r8, r8
  is_no_flag_set
  chk_reg32  r9, 0x7ffe0002
  chk_reg64 r58, r59, hi=0x00000000, lo=0x7ffe0002

; Test case 5
; Check if N flag is set
; (0   ,-1  ) *          r0
; (0   , 1  ) +          r1
; -----------
;     -1
  prep_test_case
  set_reg32  r0, -1
  set_reg32  r1, 1
  dmpyh.f    r0, r1, r0
  is_n_flag_set
  chk_reg32  r0, 0xffffffff
  chk_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff

; Test case 6
; Check if all flags (Z,N,C,V) remain unset.
; (0   ,-1  )       r5
; -----------
;      0
  prep_test_case
  set_reg32  r5, -1
  dmpyh.f    r5, r5, r5
  is_no_flag_set
  chk_reg32  r5, 2
  chk_reg64 r58, r59, hi=0, lo=2

; Test case 7
; reg <- reg, u6 (u6 is extended to 16 bits and then replicated 4 times)
;      32767       32767
; (   0x7fff  ,   0x7fff  ) *        r7
; ( 0b111111  , 0b111111  ) +        u6
;       63          63
; -------------------------
; (       2064321 * 2     )
; ------------------------
; 0x0000_0000_003e_ff82
;               4128642
  prep_test_case
  set_reg32  r7, 0x7fff7fff
  dmpyh      r6, r7, 63
  is_no_flag_set
  chk_reg32  r6, 0x003eff82
  chk_reg64 r58, r59, hi=0x00000000, lo=0x003eff82

; Test case 8
; reg <- reg, s12 (s12 is sign extended to 16 bits and then replicated 4 times)
;     -32768       32767
; (   0x8000  ,   0x7fff  ) *        r6
; (     -1    ,     -1    ) +       s12
; -------------------------
; (     32768 - 32767     )
; -------------------------
;   0x0000_0000_0000_0001
  prep_test_case
  set_reg32  r6, 0x80007fff
  dmpyh      r6, r6, -1
  is_no_flag_set
  chk_reg32  r6, 0x00000001
  chk_reg64 r58, r59, hi=0x00000000, lo=0x00000001

; Test case 9
; reg <- reg, s12 (smallest negative s12)
;     -32768       32767
; (   0x8000  ,   0x7fff  ) *        r9
; (   -2048   ,   -2048   ) +       s12
; -------------------------
; (  67108864 + -67106816 )
; -------------------------
;     0x0000_0000_0000_0800
;                      2048
  prep_test_case
  set_reg32  r9, 0x80007fff
  dmpyh      r9, r9, -2048
  is_no_flag_set
  chk_reg32  r9, 0x00000800
  chk_reg64 r58, r59, hi=0x00000000, lo=0x00000800

; Test case 10
; reg <- reg, s12 (biggest positive s12)
;     -32768       32767
; (   0x8000  ,   0x7fff  ) *        r9
; (     2047  ,     2047  ) +       s12
; -------------------------
; ( -67076096 + 67074049  )
; -------------------------
;    0xffff_ffff_ffff_f801
;                    -2047
  prep_test_case
  set_reg32  r7, 0x80007fff
  dmpyh      r7, r7, 2047
  is_no_flag_set
  chk_reg32  r7, 0xfffff801
  chk_reg64 r58, r59, hi=0xffffffff, lo=0xfffff801

; Test case 11
; check .cc when it is not met, with .f
; ( 0  , 0  ) *         r7
; ( 0  , 0  ) +         u6
; -----------
; set n flag
; ---------------------------
;  0 * 2 = 0 and n flag unset
; ---------------------------
; conditions not met, n flag should still be set and v not cleared
;
prep_test_case
  set_reg64 r58, r59, hi=0x01234567, lo=0x89abcdef
  set_reg32  r7, 0
  set_nv_flags
  dmpyh.p.f  r7, r7, 0
  is_nv_flags_set
  chk_reg64 r58, r59, hi=0x01234567, lo=0x89abcdef ; acc isnt changed

; Test case 12
; check .cc when it is met, as well as .f
; ( 0  , 0  ) *         r7
; ( 0  , 0  ) +         u6
; -----------
; set n flag
; ---------------------------
;  0 * 2 = 0 and n flag unset
; ---------------------------
; conditions are met, n flag should not be set and v cleared
;
prep_test_case
  set_reg32  r7, 0
  set_nv_flags
  dmpyh.n.f  r7, r7, 0
  is_no_flag_set
  chk_reg32  r7, 0
  chk_reg64 r58, r59, hi=0, lo=0

; Test case 13
; check 0 as register and .f flag
; ( 0  , 0  ) *         r1
; ( 0  , 0  ) +         u6
; -----------
; set n flag
; -------------------------------
;  0 * 2 + 1 = 1 and n flag unset
; -------------------------------
; n flag should not be set anymore
;
prep_test_case
  set_reg32  r1, 0x00000000
  set_n_flag
  dmpyh.f    0, r1, 0
  is_no_flag_set

; Test case 14
; check basic limm (limm is duplicated)
;    4096       1
; ( 0x1000    , 0x0001    ) *         r0
; ( 0x1000    , 0x0001    ) +       limm
; -------------------------
;  0x1000 * 0x1000 + 0x1
; -----------------------
;   0x1000011 = 16777233
prep_test_case
  set_reg32  r0, lo=0x10000001
  dmpyh      r0, r0, 0x10000001
  is_no_flag_set
  chk_reg32  r0, 0x01000001
  chk_reg64 r58, r59, hi=0x00000000, lo=0x01000001

; Test case 15
; check double limm (limm is duplicated)
;    4096       1
; ( 0x1000    , 0x0001    ) *      limm
; ( 0x1000    , 0x0001    ) +      limm
; -------------------------
;  0x1000 * 0x1000 + 0x1
; -----------------------
;  0x1000011 = 16777233
;
prep_test_case
  dmpyh      r0, 0x10000001, 0x10000001
  is_no_flag_set
  chk_reg32  r0, 0x01000001
  chk_reg64 r58, r59, hi=0x00000000, lo=0x01000001

; Test case 16
; 0 target register with 2 numbers, set flags (only use for this encoding)
; (-1 ,-1 ) *       limm
; ( 1 , 1 ) +       limm
; ---------
;    -2
;
prep_test_case
  set_nv_flags
  dmpyh.f    0, -1, 1
  is_n_flag_set
  ; Validate that the accumulator was set accordingly
  xor.f      0, r58, -2
  bne @fail

; Test case 17
; Check if dmpyh unsets n flag as well as v flag
; ( 0x7fff , 0x7fff ) *      limm
; ( 0x7fff , 0x7fff ) +      limm
; -------------------
;  32767*32767 +1
; -------------------------
;  0, n and v flags cleared
;
 prep_test_case
  set_nv_flags
  dmpyh.f    0, 0x7fff7fff, 0x7fff7fff
  is_no_flag_set
  ; Validate that the accumulator was set accordingly
  xor.f      0,   r58, 0x7ffe0002
  bne @fail
  xor.f      0,   r59, 0x00000000
  bne @fail

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; DMPYHU ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Test case 18
; r1 <- r1, r2
; ( 0x0002, 0x0001 ) *      r2
; ( 0x0003, 0x0004 ) +      r1
; ------------------
; (2*3 + 1*4)
; -----------
;  10 (0xa)
  prep_test_case
  set_reg64 r58, r59, hi=0xffffffff, lo=0xffffffff ; acc should not matter
  set_reg32  r2, 0x00020001
  set_reg32  r1, 0x00030004
  dmpyhu     r1, r1, r2
  chk_reg32  r1, 0xa
  chk_reg64 r58, r59, hi=0, lo=0xa

; Test case 19
; Biggest positives
;    65535      65535
; ( 0xffff   , 0xffff   )      r8
; -----------------------
; 8589672450 = 0x7ffe0002
  prep_test_case
  set_reg32  r8, 0xffffffff
  dmpyhu     r9, r8, r8
  is_no_flag_set
  chk_reg32  r9, 0xfffc0002
  chk_reg64 r58, r59, hi=0x00000001, lo=0xfffc0002

; Test case 20
; reg <- reg, u6 (u6 is extended to 16 bits and then replicated 4 times)
;      65535       65535
; (   0xffff  ,   0xffff  ) *        r7
; ( 0b111111  , 0b111111  ) +        u6
;       63          63
; -------------------------
;      65535 * 63 * 2
; -------------------------
; 0x0000_0000_007d_ff82
;               8257410
  prep_test_case
  set_reg32  r7, 0xffffffff
  dmpyhu     r6, r7, 63
  is_no_flag_set
  chk_reg32  r6, 0x007dff82
  chk_reg64 r58, r59, hi=0x00000000, lo=0x007dff82

; Test case 21
; reg <- reg, s12 (s12 is sign extended to 16 bits and then replicated 4 times)
;      65535       65535
; (   0xffff  ,   0xffff  ) *        r6
; (     -1    ,     -1    ) +       s12
; -------------------------
; (   65535 * 65535 * 2   )
; -------------------------
;   0x0000_0001_fffc_0002
  prep_test_case
  set_reg32  r6, 0xffffffff
  dmpyhu     r6, r6, -1
  is_no_flag_set
  chk_reg32  r6, 0xfffc0002
  chk_reg64 r58, r59, hi=0x00000001, lo=0xfffc0002

; Test case 22
; reg <- reg, s12 (smallest negative s12)
;      65535       65535
; (   0xffff  ,   0xffff  ) *        r9
; (   -2048   ,   -2048   ) +       s12
; -------------------------
; (   65535 * 63488 * 2   )
; -------------------------
;     0x0000_0001_effe_1000
;                      2048
  prep_test_case
  set_reg32  r9, 0xffffffff
  dmpyhu      r9, r9, -2048
  is_no_flag_set
  chk_reg32  r9, 0xeffe1000
  chk_reg64 r58, r59, hi=0x00000001, lo=0xeffe1000

; Test case 23
; reg <- reg, s12 (biggest positive s12)
;      65535       65535
; (   0xffff  ,   0xffff  ) *        r9
; (     2047  ,     2047  ) +       s12
; -------------------------
; (     65535*5047*2      )
; -------------------------
;    0x0000_0000_0ffd_f002
;                    -2047
  prep_test_case
  set_reg32  r7, 0xffffffff
  dmpyhu      r7, r7, 2047
  is_no_flag_set
  chk_reg32  r7, 0x0ffdf002
  chk_reg64 r58, r59, hi=0x00000000, lo=0x0ffdf002

; Test case 24
; check .cc when it is not met, with .f
; ( 0  , 0  ) *         r7
; ( 0  , 0  ) +         u6
; -----------
; set n flag
; -----------------------------
;  0 * 2 = 0 and v flag cleared
; -----------------------------
; conditions not met, v not cleared
;
prep_test_case
  set_reg64 r58, r59, hi=0x01234567, lo=0x89abcdef
  set_reg32  r7, 0
  set_nv_flags
  dmpyhu.p.f  r7, r7, 0
  is_nv_flags_set
  chk_reg64 r58, r59, hi=0x01234567, lo=0x89abcdef ; acc isnt changed

; Test case 25
; check .cc when it is met, as well as .f
; ( 0  , 0  ) *         r7
; ( 0  , 0  ) +         u6
; -----------
; set n flag
; -----------------------------
;  0 * 2 = 0 and v flag cleared
; -----------------------------
; conditions are met, v cleared
;
prep_test_case
  set_reg32  r7, 0
  set_nv_flags
  dmpyhu.n.f  r7, r7, 0
  is_n_flag_set
  chk_reg32  r7, 0
  chk_reg64 r58, r59, hi=0, lo=0

; Test case 26
; check 0 as register and .f flag
; ( 0  , 0  ) *         r1
; ( 0  , 0  ) +         u6
; -----------
; set n flag
; -----------------------------
;  0 * 2 = 0 and v flag cleared
; -------------------------------
; v flag should not be set anymore
;
prep_test_case
  set_reg64 r58, r59, hi=0x00000000, lo=0x00000001
  set_reg32  r1, 0x00000000
  set_nv_flags
  dmpyhu.f    0, r1, 0
  is_n_flag_set

; Test case 27
; check basic limm (limm is duplicated)
;    4096       1
; ( 0x1000    , 0x0001    ) *         r0
; ( 0x1000    , 0x0001    ) +       limm
; -------------------------
;  0x1000 * 0x1000 + 0x1
; -----------------------
;   0x1000001 = 16777217
prep_test_case
  set_reg32  r0, lo=0x10000001
  dmpyhu     r0, r0, 0x10000001
  is_no_flag_set
  chk_reg32  r0, 0x01000001
  chk_reg64 r58, r59, hi=0x00000000, lo=0x01000001

; Test case 28
; check double limm (limm is duplicated)
;    4096       1
; ( 0x1000    , 0x0001    ) *      limm
; ( 0x1000    , 0x0001    ) +      limm
; -------------------------
;  0x1000 * 0x1000 + 0x1
; -----------------------
;   0x1000001 = 16777217
;
prep_test_case
  dmpyhu     r0, 0x10000001, 0x10000001
  is_no_flag_set
  chk_reg32  r0, 0x01000001
  chk_reg64 r58, r59, hi=0x00000000, lo=0x01000001

; Test case 29
; 0 target register with 2 numbers, set flags (only use for this encoding)
; (-1 ,-1  ) *       limm
; ( 1 , 1  ) +       limm
; ----------
; 0xffff * 2
; 0x0001fffe
;
prep_test_case
  set_nv_flags
  dmpyhu.f    0, -1, 1
  is_n_flag_set
  ; Validate that the accumulator was set accordingly
  xor.f      0, r58, 0x0001fffe
  bne @fail

; Test case 30
; Check if dmpyhu unsets v flag
;    65535    65535
; ( 0xffff , 0xffff ) *      limm
; ( 0xffff , 0xffff ) +      limm
; -------------------
;  65535*65535 * 2
; -------------------------
;  0, v flag cleared
;
 prep_test_case
  set_nv_flags
  dmpyhu.f    0, 0xffffffff, 0xffffffff
  is_n_flag_set
  ; Validate that the accumulator was set accordingly
  xor.f       0, r59, 0x00000001
  bne @fail
  xor.f       0, r58, 0xfffc0002
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
  print " dmpyh / dmpyhu\n"
  flag 1
  end