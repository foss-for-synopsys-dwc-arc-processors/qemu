; check_vsubadd2h.S
;
; Tests for vsubadd2h: Dual 16-bit vector subtraction and addition.
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
  add.f 0, 0, 1           ; (Z, N, C, V) = (0, 0, 0, 0)
.endm

; Checks if (Z,N,C,V) == (0,0,0,0). This relies on "ADD.F 0,0,1"
; instruction in PREP_TEST_CASE macro. From a PREP_TEST_CASE macro
; in a test case, and thence to a VECTOR instruction and finally to
; this macro, none of the aforementioned flags must have been set,
; because VECTOR instructions aren't supposed to do so.
.macro  check_flags_remained_zero
  lr    r11, [status32]
  mov   r12, REG_STAT_Z
  or    r12, r12, REG_STAT_N
  or    r12, r12, REG_STAT_C
  or    r12, r12, REG_STAT_V
  and   r11, r11, r12
  cmp   r11, 0
  bne   @fail
.endm

; pair(HI, LOW) == pair(REG_HI, REG_LO)
; HI, LO:         32-bit
; REG_HI, REG_LO: 32-bit
.macro  set_64bit_double        reg_hi, reg_lo, hi, low
  mov \reg_hi, \hi
  mov \reg_lo, \low
.endm

; pair(HI, LOW) == pair(REG_HI, REG_LO)
; HI, LO:         32-bit
; REG_HI, REG_LO: 32-bit
.macro  check_64bit_double      reg_hi, reg_lo, hi, low
  check_flags_remained_zero
  mov   r11, \hi
  mov   r10, \low
  cmp   r11, \reg_hi
  bne   @fail
  cmp   r10, \reg_lo
  bne   @fail
.endm

.macro  check_reg32      reg, val
  check_flags_remained_zero
  cmp   \reg, \val
  bne   @fail
.endm

; set Carry equal 1
.macro set_carry
  mov     r9, 0x80000000  ; setting...
  add.f   0,r9,r9         ; ...C=1
.endm

; clear Carry
.macro clear_carry
  add.f   0,0,1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VSUBADD2H ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== vsubadd2h a,b,c ===

; vsubadd2h a,b,c 00101bbb000101110BBBCCCCCCAAAAAA
;   a.h0 = b.h0 - c.h0;
;   a.h1 = b.h1 + c.h1;

; Test case 1.1
; Same source and dest operands
; r0 <- r0, r0
;
;   (0x8001, 0x8002) [r0]
;      +       -
;   (0x8001, 0x8002) [r0]
;   ----------------
;   (0x0002, 0x0000) [r0]
;
  prep_test_case
  mov r0, 0x80018002
  vsubadd2h r0, r0, r0
  check_reg32 r0, 0x00020000

; Test case 1.2
; Different source and dest operands
; r0 <- r1, r2
;
;   (0xfff5, 0x0001) [r1]
;      +       -
;   (0x00ff, 0x0003) [r2]
;   ----------------
;   (0x00f4, 0xfffe) [r0]
;
  prep_test_case
  mov r1, 0xfff50001
  mov r2, 0x00ff0003
  vsubadd2h r0, r1, r2
  check_reg32 r0, 0x00f4fffe

;=== vsubadd2h 0,b,c ===

; vsubadd2h 0,b,c 00101bbb000101110BBBCCCCCC111110
;   0 = b.h0 - c.h0;
;   0 = b.h1 + c.h1;

; Test case 2
; 0 <- r1, r2
;
;   (0xfff5, 0x0001) [r1]
;      +       -
;   (0x00ff, 0x0003) [r2]
;   ----------------
;   (------, ------) [0]
;
  prep_test_case
  mov r1, 0xfff50001
  mov r2, 0x00ff0003
  vsubadd2h 0, r1, r2

;=== vsubadd2h<.cc> b,b,c ===

; vsubadd2h<.cc> b,b,c 00101bbb110101110BBBCCCCCC0QQQQQ
;   if (cc) {
;     b.h0 = b.h0 - c.h0;
;     b.h1 = b.h1 + c.h1;
;   }

; Test case 3.1
; CC=1
; r2 <- r2, r3 (cc)
;
;   (0xfff5, 0x0001) [r2]
;      +       -
;   (0x00ff, 0x0003) [r3]
;   ----------------
;   (0x00f4, 0xfffe) [r2]
;
  prep_test_case
  mov r2, 0xfff50001
  mov r3, 0x00ff0003
  set_carry
  vsubadd2h.c r2, r2, r3
  clear_carry
  check_reg32 r2, 0x00f4fffe

; Test case 3.2
; CC=0
; Instruction is not executed
  prep_test_case
  mov r2, 0xfff50001
  mov r3, 0x00ff0003
  vsubadd2h.c r2, r2, r3
  check_reg32 r2, 0xfff50001

;=== vsubadd2h a,b,u6 ===

; vsubadd2h a,b,u6 00101bbb010101110BBBuuuuuuAAAAAA
;   a.h0 = b.h0 - u6;
;   a.h1 = b.h1 + u6;

; Test case 4
; r0 <- r1, u6
;
;   (0xfff5, 0x0040) [r1]
;      +       -
;   (  0x3f,   0x3f) [u6]
;   --------------------------------
;   (0x0034, 0x0001) [r0]
;
  prep_test_case
  mov r1, 0xfff50040
  vsubadd2h r0, r1, 0x3f
  check_reg32 r0, 0x00340001

;=== vsubadd2h 0,b,u6 ===

; vsubadd2h 0,b,u6 00101bbb010101110BBBuuuuuu111110
;   0 = b.h0 - u6;
;   0 = b.h1 + u6;

; Test case 5
; 0 <- r1, u6
;
;   (0xfff5, 0x0040) [r1]
;      +       -
;   (  0x3f,   0x3f) [u6]
;   --------------------------------
;   (------, ------) [0]
;
  prep_test_case
  mov r1, 0xfff50040
  vsubadd2h 0, r1, 0x3f

;=== vsubadd2h<.cc> b,b,u6 ===

; vsubadd2h<.cc> b,b,u6 00101bbb110101110BBBuuuuuu1QQQQQ
;   if (cc) {
;     b.h0 = b.h0 - u6;
;     b.h1 = b.h1 + u6;
;   }

; Test case 6.1
; CC=1
; r1 <- r1, u6 (cc)
;
;   (0xfff5, 0x0040) [r1]
;      +       -
;   (  0x3f,   0x3f) [u6]
;   --------------------------------
;   (0x0034, 0x0001) [r1]
;
  prep_test_case
  mov r1, 0xfff50040
  set_carry
  vsubadd2h.c r1, r1, 0x3f
  clear_carry
  check_reg32 r1, 0x00340001

; Test case 6.2
; CC=0
; Instruction is not executed
  prep_test_case
  mov r1, 0xfff50040
  vsubadd2h.c r1, r1, 0x3f
  check_reg32 r1, 0xfff50040

;=== vsubadd2h b,b,s12 ===

; vsubadd2h b,b,s12 00101bbb100101110BBBssssssSSSSSS
;   b.h0 = b.h0 - s12;
;   b.h1 = b.h1 + s12;

; Test case 7.1
; using s12: positive
; r0 <- r0, s12
;
;   (0xfff5, 0x0076) [r0]
;      +       -
;   (  0x75,   0x75) [s12]
;   ----------------
;   (0x006a, 0x0001) [r0]
;
  prep_test_case
  mov r0, 0xfff50076
  vsubadd2h r0, r0, 0x75
  check_reg32 r0, 0x006a0001

; Test case 7.2
; using s12: negative
; r0 <- r0, s12
;
;   (0x0003, 0x0004) [r0]
;      +       -
;   (0xfffe, 0xfffe) [s12]
;   ----------------
;   (0x0001, 0x0006) [r0]
;
  prep_test_case
  mov r0, 0x00030004
  vsubadd2h r0, r0, -2     ; -2=0xfffe 
  check_reg32 r0, 0x00010006

;=== vsubadd2h a,limm,c ===

; vsubadd2h a,limm,c 00101110000101110111CCCCCCAAAAAA
;   a.h0 = limm.h0 - c.h0;
;   a.h1 = limm.h1 + c.h1;

; Test case 8
; r0 <- limm, r1
;
;   (0x1234, 0x5678) [limm]
;      +       -
;   (0x0001, 0x0002) [r1]
;   ----------------
;   (0x1235, 0x5676) [r0]
;
  prep_test_case
  mov r1, 0x00010002
  vsubadd2h r0, 0x12345678, r1 
  check_reg32 r0, 0x12355676

;=== vsubadd2h a,b,limm ===

; vsubadd2h a,b,limm 00101bbb000101110BBB111110AAAAAA
;   a.h0 = b.h0 - limm.h0;
;   a.h1 = b.h1 + limm.h1;

; Test case 9
; r0 <- r1, limm
;
;   (0x0001, 0x0002) [r1]
;      +       -
;   (0x1234, 0x5678) [limm]
;   ----------------
;   (0x1235, 0xa98a) [r0]
;
  prep_test_case
  mov r1, 0x00010002
  vsubadd2h r0, r1, 0x12345678 
  check_reg32 r0, 0x1235a98a

;=== vsubadd2h 0,limm,c ===

; vsubadd2h 0,limm,c 00101110000101110111CCCCCC111110
;   0 = limm.h0 - c.h0;
;   0 = limm.h1 + c.h1;

; Test case 10
; 0 <- limm, r1
;
;   (0x1234, 0x5678) [limm]
;      +       -
;   (0x0001, 0x0002) [r1]
;   ----------------
;   (------, ------) [0]
;
  prep_test_case
  mov r1, 0x00010002
  vsubadd2h 0, 0x12345678, r1 

;=== vsubadd2h 0,b,limm ===

; vsubadd2h 0,b,limm 00101bbb000101110BBB111110111110
;   0 = b.h0 - limm.h0;
;   0 = b.h1 + limm.h1;

; Test case 11
; 0 <- r1, limm
;
;   (0x0001, 0x0002) [r1]
;      +       -
;   (0x1234, 0x5678) [limm]
;   ----------------
;   (------, ------) [0]
;
  prep_test_case
  mov r1, 0x00010002
  vsubadd2h 0, r1, 0x12345678 

;=== vsubadd2h<.cc> b,b,limm ===

; vsubadd2h<.cc> b,b,limm 00101bbb110101110BBB1111100QQQQQ
;   if (cc) {
;     b.h0 = b.h0 - limm.h0;
;     b.h1 = b.h1 + limm.h1;
;   }

; Test case 12.1
; CC=1
; r0 <- r0, limm (cc)
;
;   (0x0001, 0x0002) [r0]
;      +       -
;   (0x1234, 0x5678) [limm]
;   --------------------------------
;   (0x1235, 0xa98a) [r0]
;
  prep_test_case
  mov r0, 0x00010002
  set_carry
  vsubadd2h.c r0, r0, 0x12345678
  clear_carry
  check_reg32 r0, 0x1235a98a

; Test case 12.2
; CC=0
; Instruction is not executed
  prep_test_case
  mov r0, 0x00010002
  vsubadd2h.c r0, r0, 0x12345678
  check_reg32 r0, 0x00010002

;=== vsubadd2h<.cc> 0,limm,c ===

; vsubadd2h<.cc> 0,limm,c 00101110110101110111CCCCCC0QQQQQ
;   if (cc) {
;     0 = limm.h0 - c.h0;
;     0 = limm.h1 + c.h1;
;   }

; Test case 13.1
; CC=1
; 0 <- limm, r0 (cc)
;
;   (0x1234, 0x5678) [limm]
;      +       -
;   (0x0001, 0x0002) [r0]
;   ----------------
;   (------, ------) [0]
;
  prep_test_case
  mov r0, 0x00010002
  set_carry
  vsubadd2h.c 0, 0x12345678, r0
  clear_carry

; Test case 13.2
; CC=0
; Instruction is not executed
  prep_test_case
  mov r0, 0x00010002
  vsubadd2h.c 0, 0x12345678, r0

;=== vsubadd2h a,limm,u6 ===

; vsubadd2h a,limm,u6 00101110010101110111uuuuuuAAAAAA
;   a.h0 = limm.h0 - u6;
;   a.h1 = limm.h1 + u6;

; Test case 14
; r0 <- limm, u6
;
;   (0x1234, 0x5678) [limm]
;      +       -
;   (  0x3f,   0x3f) [u6]
;   ----------------
;   (0x1273, 0x5639) [r0]
;
  prep_test_case
  vsubadd2h r0, 0x12345678, 0x3f 
  check_reg32 r0, 0x12735639

;=== vsubadd2h 0,limm,u6 ===

; vsubadd2h 0,limm,u6 00101110010101110111uuuuuu111110
;   0 = limm.h0 - u6;
;   0 = limm.h1 + u6;

; Test case 15
; 0 <- limm, u6
;
;   (0x1234, 0x5678) [limm]
;      +       -
;   (  0x3f,   0x3f) [u6]
;   ----------------
;   (------, ------) [0]
;
  prep_test_case
  vsubadd2h 0, 0x12345678, 0x3f

;=== vsubadd2h<.cc> 0,limm,u6 ===

; vsubadd2h<.cc> 0,limm,u6 00101110110101110111uuuuuu1QQQQQ
;   if (cc) {
;     0 = limm.h0 - u6;
;     0 = limm.h1 + u6;
;   }

; Test case 16.1
; CC=1
; 0 <- limm, u6 (cc)
;
;   (0x1234, 0x5678) [limm]
;      +       -
;   (  0x3f,   0x3f) [u6]
;   ----------------
;   (------, ------) [0]
;
  prep_test_case
  set_carry
  vsubadd2h.c 0, 0x12345678, 0x3f
  clear_carry

; Test case 16.2
; CC=0
; Instruction is not executed
  prep_test_case
  vsubadd2h.c 0, 0x12345678, 0x3f

;=== vsubadd2h 0,limm,s12 ===

; vsubadd2h 0,limm,s12 00101110100101110111ssssssSSSSSS
;   0 = limm.h0 - s12;
;   0 = limm.h1 + s12;

; Test case 17
; 0 <- limm, s12
;
;   (0x1234, 0x5678) [limm]
;      +       -
;   (0xfffe, 0xfffe) [s12]
;   ----------------
;   (------, ------) [0]
;
  prep_test_case
  vsubadd2h 0, 0x12345678, -2   ; -2=0xfffe

;=== vsubadd2h a,limm,limm ===

; vsubadd2h a,limm,limm 00101110000101110111111110AAAAAA
;   a.h0 = limm.h0 - limm.h0;
;   a.h1 = limm.h1 + limm.h1;

; Test case 18
; r0 <- limm, limm
;
;   (0x1234, 0x5678) [limm]
;      +       -
;   (0x1234, 0x5678) [limm]
;   ----------------
;   (0x2468, 0x0000) [r0]
;
  prep_test_case
  mov r0, 0x00010002
  vsubadd2h r0, 0x12345678, 0x12345678
  check_reg32 r0, 0x24680000

;=== vsubadd2h 0,limm,limm ===

; vsubadd2h 0,limm,limm 00101110000101110111111110111110
;   0 = limm.h0 - limm.h0;
;   0 = limm.h1 + limm.h1;

; Test case 19
; 0 <- limm, limm
;
;   (0x1234, 0x5678) [limm]
;      +       -
;   (0x1234, 0x5678) [limm]
;   ----------------
;   (------, ------) [0]
;
  prep_test_case
  vsubadd2h 0, 0x12345678, 0x12345678

;=== vsubadd2h<.cc> 0,limm,limm ===

; vsubadd2h<.cc> 0,limm,limm 001011101101011101111111100QQQQQ
;   if (cc) {
;     0 = limm.h0 - limm.h0;
;     0 = limm.h1 + limm.h1;
;   }

; Test case 20.1
; CC=1
; 0 <- limm, limm (cc)
;
;   (0x1234, 0x5678) [limm]
;      +       -
;   (0x1234, 0x5678) [limm]
;   ----------------
;   (------, ------) [0]
;
  prep_test_case
  set_carry
  vsubadd2h.c 0, 0x12345678, 0x12345678
  clear_carry

; Test case 20.2
; CC=0
; Instruction is not executed
  prep_test_case
  vsubadd2h.c 0, 0x12345678, 0x12345678

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
  ;print_number r0
  print "[FAIL]"
1:
  print " vsubadd2h: Dual 16-bit vector subtraction and addition\n"
  end
