; check_vsubadd.S
;
; Tests for vsubadd: Dual 32-bit SIMD subtract and add.
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

; reg64(hi, low) = (hi, low)
.macro  set_reg64       reg, hi, low
  movhl \reg, \hi
  orl \reg, \reg, \low
.endm

; reg64(hi, low) == (hi, low)
.macro  check_reg64     reg, hi, low
  movhl  r11, \hi
  orl    r11, r11, \low
  xorl.f 0, r11, \reg
  bne    @fail
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VSUBADD ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== vsubadd a,b,c ===

; vsubadd a,b,c 00101bbb001111110BBBCCCCCCAAAAAA
;   a.w0 = b.w0 - c.w0;
;   a.w1 = b.w1 + c.w1;

; Test case 1.1
; Same source and dest operands
; r0 <- r0, r0
;
;   (0x80000001, 0x00000001) [r0]
;        +           -
;   (0x00000001, 0x00000001) [r0]
;   --------------------------------
;   (0x80000002, 0x00000000) [r0]
;
  prep_test_case
  set_reg64 r0, 0x80000001, 0x00000001
  vsubadd r0, r0, r0
  check_reg64 r0, 2, 0

; Test case 1.2
; Different source and dest operands
; r0 <- r1, r2
;
;   (0xfffffff5, 0x00000001) [r1]
;        +           -
;   (0x000000ff, 0x00000003) [r2]
;   --------------------------------
;   (0x000000f4, 0xfffffffe) [r0]
;
  prep_test_case
  set_reg64 r1, 0xfffffff5, 0x00000001
  set_reg64 r2, 0x000000ff, 0x00000003
  vsubadd r0, r1, r2
  check_reg64 r0, 0x000000f4, 0xfffffffe

;=== vsubadd 0,b,c ===

; vsubadd 0,b,c 00101bbb001111110BBBCCCCCC111110
;   0 = b.w0 - c.w0;
;   0 = b.w1 + c.w1;

; Test case 2
; 0 <- r1, r2
;
;   (0xfffffff5, 0x00000001) [r1]
;        +           -
;   (0x000000ff, 0x00000003) [r2]
;   --------------------------------
;   (----------, ----------) [0]
;
  prep_test_case
  set_reg64 r1, 0xfffffff5, 0x00000001 
  set_reg64 r2, 0x000000ff, 0x00000003 
  vsubadd 0, r1, r2

;=== vsubadd<.cc> b,b,c ===

; vsubadd<.cc> b,b,c 00101bbb111111110BBBCCCCCC0QQQQQ
;   if (cc) {
;     b.w0 = b.w0 - c.w0;
;     b.w1 = b.w1 + c.w1;
;   }

; Test case 3.1
; CC=1
; r2 <- r2, r3 (cc)
;
;   (0xfffffff5, 0x00000001) [r2]
;        +           -
;   (0x000000ff, 0x00000003) [r3]
;   --------------------------------
;   (0x000000f4, 0xfffffffe) [r2]
;
  prep_test_case
  set_reg64 r2, 0xfffffff5, 0x00000001 
  set_reg64 r3, 0x000000ff, 0x00000003 
  add.f    0, 0, 0       ; Z=1
  vsubadd.z r2, r2, r3
  check_reg64 r2, 0x000000f4, 0xfffffffe

; Test case 3.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_reg64 r2, 0xfffffff5, 0x00000001
  set_reg64 r3, 0x000000ff, 0x00000003
  vsubadd.z r2, r2, r3    ; Z=0 because of PREP_TEST_CASE
  check_reg64 r2, 0xfffffff5, 0x00000001

;=== vsubadd a,b,u6 ===

; vsubadd a,b,u6 00101bbb011111110BBBuuuuuuAAAAAA
;   a.w0 = b.w0 - u6;
;   a.w1 = b.w1 + u6;

; Test case 4
; r0 <- r1, u6
;
;   (0xfffffff5, 0x00000040) [r1]
;        +           -
;   (      0x3f,       0x3f) [u6]
;   --------------------------------
;   (0x00000034, 0x00000001) [r0]
;
  prep_test_case
  set_reg64 r1, 0xfffffff5, 0x00000040
  vsubadd r0, r1, 0x3f
  check_reg64 r0, 0x00000034, 0x00000001

;=== vsubadd 0,b,u6 ===

; vsubadd 0,b,u6 00101bbb011111110BBBuuuuuu111110
;   0 = b.w0 - u6;
;   0 = b.w1 + u6;

; Test case 5
; 0 <- r1, u6
;
;   (0xfffffff5, 0x00000040) [r1]
;        +           -
;   (      0x3f,       0x3f) [u6]
;   --------------------------------
;   (----------, ----------) [0]
;
  prep_test_case
  set_reg64 r1, 0xfffffff5, 0x00000040
  vsubadd 0, r1, 0x3f

;=== vsubadd<.cc> b,b,u6 ===

; vsubadd<.cc> b,b,u6 00101bbb111111110BBBuuuuuu1QQQQQ
;   if (cc) {
;     b.w0 = b.w0 - u6;
;     b.w1 = b.w1 + u6;
;   }

; Test case 6.1
; CC=1
; r0 <- r0, u6 (cc)
;
;   (0xfffffff5, 0x00000040) [r0]
;        +           -
;   (      0x3f,       0x3f) [u6]
;   --------------------------------
;   (0x00000034, 0x00000001) [r0]
;
  prep_test_case
  set_reg64 r0, 0xfffffff5, 0x00000040
  add.f    0, 0, 0       ; Z=1
  vsubadd.z r0, r0, 0x3f
  check_reg64 r0, 0x00000034, 0x00000001

; Test case 6.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_reg64 r0, 0xfffffff5, 0x00000040 
  vsubadd.z r0, r0, 0x3f
  check_reg64 r0, 0xfffffff5, 0x00000040 

;=== vsubadd b,b,s12 ===

; vsubadd b,b,s12 00101bbb101111110BBBssssssSSSSSS
;   b.w0 = b.w0 - s12;
;   b.w1 = b.w1 + s12;

; Test case 7.1
; using s12: positive
; r0 <- r0, s12
;
;   (0xfffffff5, 0x00000076) [r0]
;        +           -
;   (      0x75,       0x75) [s12]
;   --------------------------------
;   (0x0000006a, 0x00000001) [r0]
;
  prep_test_case
  set_reg64 r0, 0xfffffff5, 0x00000076
  vsubadd r0, r0, 0x75
  check_reg64 r0, 0x0000006a, 0x00000001

; Test case 7.2
; using s12: negative
; r0 <- r0, s12
;
;   (0x00000003, 0x00000004) [r0]
;        +           -
;   (0xfffffffe, 0xfffffffe) [s12]
;   --------------------------------
;   (0x00000001, 0x00000006) [r0]
;
  prep_test_case
  set_reg64 r0, 0x00000003, 0x00000004
  vsubadd r0, r0, -2     ; -2=0xfffffffe 
  check_reg64 r0, 0x00000001, 0x00000006

;=== vsubadd a,limm,c ===

; vsubadd a,limm,c 00101110001111110111CCCCCCAAAAAA
;   a.w0 = limm - c.w0;
;   a.w1 = 0 + c.w1;

; Test case 8
; r0 <- limm, r1
;
;   (         0, 0x12345678) [limm]
;        +           -
;   (0x00000001, 0x00000002) [r1]
;   --------------------------------
;   (0x00000001, 0x12345676) [r0]
;
  prep_test_case
  set_reg64 r1, 0x00000001, 0x00000002
  vsubadd r0, 0x12345678, r1 
  check_reg64 r0, 0x00000001, 0x12345676

;=== vsubadd a,b,limm ===

; vsubadd a,b,limm 00101bbb001111110BBB111110AAAAAA
;   a.w0 = b.w0 - limm;
;   a.w1 = b.w1 + 0;

; Test case 9
; r0 <- r1, limm
;
;   (0x00000001, 0x00000002) [r1]
;        +           -
;   (         0, 0x12345678) [limm]
;   --------------------------------
;   (       0x1, 0xedcba98a) [r0]
;
  prep_test_case
  set_reg64 r1, 0x00000001, 0x00000002
  vsubadd r0, r1, 0x12345678 
  check_reg64 r0, 0x1, 0xedcba98a

;=== vsubadd 0,limm,c ===

; vsubadd 0,limm,c 00101110001111110111CCCCCC111110
;   0 = limm - c.w0;
;   0 = 0 + c.w1;

; Test case 10
; 0 <- limm, r0
;
;   (         0, 0x12345678) [limm]
;        +           -
;   (0x00000001, 0x00000002) [r0]
;   --------------------------------
;   (----------, ----------) [0]
;
  prep_test_case
  set_reg64 r0, 0x00000001, 0x00000002
  vsubadd 0, 0x12345678, r0 

;=== vsubadd 0,b,limm ===

; vsubadd 0,b,limm 00101bbb001111110BBB111110111110
;   0 = b.w0 - limm;
;   0 = b.w1 + 0;

; Test case 11
; 0 <- r0, limm
;
;   (0x00000001, 0x00000002) [r0]
;        +           -
;   (         0, 0x12345678) [limm]
;   --------------------------------
;   (----------, ----------) [0]
;
  prep_test_case
  set_reg64 r0, 0x00000001, 0x00000002
  vsubadd 0, r0, 0x12345678 

;=== vsubadd<.cc> b,b,limm ===

; vsubadd<.cc> b,b,limm 00101bbb111111110BBB1111100QQQQQ
;   if (cc) {
;     b.w0 = b.w0 - limm;
;     b.w1 = b.w1 + 0;
;   }

; Test case 12.1
; CC=1
; r0 <- r0, limm (cc)
;
;   (0x00000001, 0x00000002) [r0]
;        +           -
;   (         0, 0x12345678) [limm]
;   --------------------------------
;   (       0x1, 0xedcba98a) [r0]
;
  prep_test_case
  set_reg64 r0, 0x00000001, 0x00000002
  add.f    0, 0, 0       ; Z=1
  vsubadd.z r0, r0, 0x12345678 
  check_reg64 r0, 0x1, 0xedcba98a

; Test case 12.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_reg64 r0, 0x00000001, 0x00000002
  vsubadd.z r0, r0, 0x12345678 
  check_reg64 r0, 0x00000001, 0x00000002

;=== vsubadd<.cc> 0,limm,c ===

; vsubadd<.cc> 0,limm,c 00101110111111110111CCCCCC0QQQQQ
;   if (cc) {
;     a.w0 = b.w0 - c.w0;
;     a.w1 = b.w1 + c.w1;
;   }

; Test case 13.1
; CC=1
; 0 <- limm, r0 (cc)
;
;   (         0, 0x12345678) [limm]
;        +           -
;   (0x00000001, 0x00000002) [r0]
;   --------------------------------
;   (----------, ----------) [r0]
;
  prep_test_case
  set_reg64 r0, 0x00000001, 0x00000002
  add.f    0, 0, 0       ; Z=1
  vsubadd.z 0, 0x12345678, r0 

; Test case 13.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_reg64 r0, 0x00000001, 0x00000002
  vsubadd.z 0, 0x12345678, r0 

;=== vsubadd a,limm,u6 ===

; vsubadd a,limm,u6 00101110011111110111uuuuuuAAAAAA
;   a.w0 = limm - u6;
;   a.w1 = 0 + u6;

; Test case 14
; r0 <- limm, u6
;
;   (         0, 0x12345678) [limm]
;        +           -
;   (      0x3f,       0x3f) [u6]
;   --------------------------------
;   (      0x3f, 0x12345639) [r0]
;
  prep_test_case
  vsubadd r0, 0x12345678, 0x3f 
  check_reg64 r0, 0x3f, 0x12345639

;=== vsubadd 0,limm,u6 ===

; vsubadd 0,limm,u6 00101110011111110111uuuuuu111110
;   0 = limm - u6;
;   0 = 0 + u6;

; Test case 15
; 0 <- limm, u6
;
;   (         0, 0x12345678) [limm]
;        +           -
;   (      0x3f,       0x3f) [u6]
;   --------------------------------
;   (----------, ----------) [0]
;
  prep_test_case
  vsubadd 0, 0x12345678, 0x3f 

;=== vsubadd<.cc> 0,limm,u6 ===

; vsubadd<.cc> 0,limm,u6 00101110111111110111uuuuuu1QQQQQ
;   if (cc) {
;     0 = limm - u6;
;     0 = 0 + u6;
;   }

; Test case 16.1
; CC=1
; 0 <- limm, u6 (cc)
;
;   (         0, 0x12345678) [limm]
;        +           -
;   (      0x3f,       0x3f) [u6]
;   --------------------------------
;   (----------, ----------) [0]
;
  prep_test_case
  add.f    0, 0, 0       ; Z=1
  vsubadd 0, 0x12345678, 0x3f 

; Test case 16.2
; CC=0
; Instruction is not executed
  prep_test_case
  vsubadd 0, 0x12345678, 0x3f 

;=== vsubadd 0,limm,s12 ===

; vsubadd 0,limm,s12 00101110101111110111ssssssSSSSSS
;   0 = limm - s12;
;   0 = 0 + s12;

; Test case 17
; 0 <- limm, s12
;
;   (         0, 0x12345678) [limm]
;        +           -
;   (0xfffffffe, 0xfffffffe) [s12]
;   --------------------------------
;   (----------, ----------) [0]
;
  prep_test_case
  vsubadd 0, 0x12345678, -2   ; -2=0xfffffffe

;=== vsubadd a,limm,limm ===

; vsubadd a,limm,limm 00101110001111110111111110AAAAAA
;   a.w0 = limm - limm;
;   a.w1 = 0 + 0;

; Test case 18
; r0 <- limm, limm
;
;   (         0, 0x12345678) [limm]
;        +           -
;   (         0, 0x12345678) [limm]
;   --------------------------------
;   (         0,          0) [r0]
;
  prep_test_case
  vsubadd r0, 0x12345678, 0x12345678
  check_reg64 r0, 0, 0

;=== vsubadd 0,limm,limm ===

; vsubadd 0,limm,limm 00101110001111110111111110111110
;   0 = limm - limm;
;   0 = 0 + 0;

; Test case 19
; 0 <- limm, limm
;
;   (         0, 0x12345678) [limm]
;        +           -
;   (         0, 0x12345678) [limm]
;   --------------------------------
;   (----------, ----------) [0]
;
  prep_test_case
  vsubadd 0, 0x12345678, 0x12345678

;=== vsubadd<.cc> 0,limm,limm ===

; vsubadd<.cc> 0,limm,limm 001011101111111101111111100QQQQQ
;   if (cc) {
;     0 = limm - limm;
;     0 = 0 + 0;
;   }

; Test case 20.1
; CC=1
;
;   (         0, 0x12345678) [limm]
;        +           -
;   (         0, 0x12345678) [limm]
;   --------------------------------
;   (----------, ----------) [0]
;
  prep_test_case
  add.f    0, 0, 0       ; Z=1
  vsubadd 0, 0x12345678, 0x12345678

; Test case 20.2
; CC=0
; Instruction is not executed
  prep_test_case
  vsubadd 0, 0x12345678, 0x12345678

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
  print " vsubadd: Dual 32-bit SIMD subtract and add\n"
  end
