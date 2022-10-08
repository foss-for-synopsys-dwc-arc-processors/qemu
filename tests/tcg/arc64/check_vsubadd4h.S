; check_vsubadd4h.S
;
; Tests for vsubadd4h: Quad16-bit SIMD subtract and add.
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VSUBADD4H ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== vsubadd4h a,b,c ===

; vsubadd4h a,b,c 00101bbb001110110BBBCCCCCCAAAAAA
;   a.h0 = b.h0 - c.h0;
;   a.h1 = b.h1 + c.h1;
;   a.h2 = b.h2 - c.h2;
;   a.h3 = b.h3 + c.h3;

; Test case 1.1
; Same source and dest operands
; r0 <- r0, r0
;
;   (0x8003, 0x8004, 0x8001, 0x8002) [r0]
;      +       -       +       -
;   (0x8003, 0x8004, 0x8001, 0x8002) [r0]
;   --------------------------------
;   (0x0006, 0x0000, 0x0002, 0x0000) [r0]
;
  prep_test_case
  set_reg64 r0, 0x80038004, 0x80018002
  vsubadd4h r0, r0, r0
  check_reg64 r0, 0x00060000, 0x00020000

; Test case 1.2
; Different source and dest operands
; r0 <- r1, r2
;
;   (0x8003, 0x8005, 0xfff5, 0x0001) [r1]
;      +       -       +       -
;   (0x8003, 0x8004, 0x00ff, 0x0003) [r2]
;   --------------------------------
;   (0x0006, 0x0001, 0x00f4, 0xfffe) [r0]
;
  prep_test_case
  set_reg64 r1, 0x80038005, 0xfff50001
  set_reg64 r2, 0x80038004, 0x00ff0003
  vsubadd4h r0, r1, r2
  check_reg64 r0, 0x00060001, 0x00f4fffe

;=== vsubadd4h 0,b,c ===

; vsubadd4h 0,b,c 00101bbb001110110BBBCCCCCC111110
;   0 = b.h0 - c.h0;
;   0 = b.h1 + c.h1;
;   0 = b.h2 - c.h2;
;   0 = b.h3 + c.h3;

; Test case 2
; 0 <- r1, r2
;
;   (0x8003, 0x8005, 0xfff5, 0x0001) [r1]
;      +       -       +       -
;   (0x8003, 0x8004, 0x00ff, 0x0003) [r2]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  set_reg64 r1, 0x80038005, 0xfff50001
  set_reg64 r2, 0x80038004, 0x00ff0003
  vsubadd4h 0, r1, r2

;=== vsubadd4h<.cc> b,b,c ===

; vsubadd4h<.cc> b,b,c 00101bbb111110110BBBCCCCCC0QQQQQ
;   if (cc) {
;     b.h0 = b.h0 - c.h0;
;     b.h1 = b.h1 + c.h1;
;     b.h2 = b.h2 - c.h2;
;     b.h3 = b.h3 + c.h3;
;   }

; Test case 3.1
; CC=1
; r2 <- r2, r3 (cc)
;
;   (0x8003, 0x8005, 0xfff5, 0x0001) [r2]
;      +       -       +       -
;   (0x8003, 0x8004, 0x00ff, 0x0003) [r3]
;   --------------------------------
;   (0x0006, 0x0001, 0x00f4, 0xfffe) [r2]
;
  prep_test_case
  set_reg64 r2, 0x80038005, 0xfff50001
  set_reg64 r3, 0x80038004, 0x00ff0003
  add.f    0, 0, 0       ; Z=1
  vsubadd4h.z r2, r2, r3
  check_reg64 r2, 0x00060001, 0x00f4fffe

; Test case 3.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_reg64 r2, 0x80038005, 0xfff50001
  set_reg64 r3, 0x80038004, 0x00ff0003
  vsubadd4h.z r2, r2, r3  ; Z=0 because of PREP_TEST_CASE
  check_reg64 r2, 0x80038005, 0xfff50001

;=== vsubadd4h a,b,u6 ===

; vsubadd4h a,b,u6 00101bbb011110110BBBuuuuuuAAAAAA
;   a.h0 = b.h0 - u6;
;   a.h1 = b.h1 + u6;
;   a.h2 = b.h2 - u6;
;   a.h3 = b.h3 + u6;

; Test case 4
; r0 <- r1, u6
;
;   (0x003f, 0x003f, 0xfff5, 0x0040) [r1]
;      +       -       +       -
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
;   --------------------------------
;   (0x007e, 0x0000, 0x0034, 0x0001) [r0]
;
  prep_test_case
  set_reg64 r1, 0x003f003f, 0xfff50040
  vsubadd4h r0, r1, 0x3f
  check_reg64 r0, 0x007e0000, 0x00340001

;=== vsubadd4h 0,b,u6 ===

; vsubadd4h 0,b,u6 00101bbb011110110BBBuuuuuu111110
;   0 = b.h0 - u6;
;   0 = b.h1 + u6;
;   0 = b.h2 - u6;
;   0 = b.h3 + u6;

; Test case 5
; 0 <- r1, u6
;
;   (0x003f, 0x003f, 0xfff5, 0x0040) [r1]
;      +       -       +       -
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  set_reg64 r1, 0x003f003f, 0xfff50040
  vsubadd4h 0, r1, 0x3f

;=== vsubadd4h<.cc> b,b,u6 ===

; vsubadd4h<.cc> b,b,u6 00101bbb111110110BBBuuuuuu1QQQQQ
;   if (cc) {
;     b.h0 = b.h0 - u6;
;     b.h1 = b.h1 + u6;
;     b.h2 = b.h2 - u6;
;     b.h3 = b.h3 + u6;
;   }

; Test case 6.1
; CC=1
; r0 <- r0, u6
;
;   (0x003f, 0x003f, 0xfff5, 0x0040) [r0]
;      +       -       +       -
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
;   --------------------------------
;   (0x007e, 0x0000, 0x0034, 0x0001) [r0]
;
  prep_test_case
  set_reg64 r0, 0x003f003f, 0xfff50040
  add.f    0, 0, 0       ; Z=1
  vsubadd4h.z r0, r0, 0x3f
  check_reg64 r0, 0x007e0000, 0x00340001

; Test case 6.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_reg64 r0, 0x003f003f, 0xfff50040
  vsubadd4h.z r0, r0, 0x3f
  check_reg64 r0, 0x003f003f, 0xfff50040

;=== vsubadd4h b,b,s12 ===

; vsubadd4h b,b,s12 00101bbb101110110BBBssssssSSSSSS
;   b.h0 = b.h0 - s12;
;   b.h1 = b.h1 + s12;
;   b.h2 = b.h2 - s12;
;   b.h3 = b.h3 + s12;

; Test case 7.1
; using s12: positive
; r0 <- r0, s12
;
;   (0x8003, 0x8004, 0xfff5, 0x0076) [r0]
;      +       -       +       -
;   (  0x75,   0x75,   0x75,   0x75) [s12]
;   --------------------------------
;   (0x8078, 0x7f8f, 0x006a, 0x0001) [r0]
;
  prep_test_case
  set_reg64 r0, 0x80038004, 0xfff50076
  vsubadd4h r0, r0, 0x75
  check_reg64 r0, 0x80787f8f, 0x006a0001

; Test case 7.2
; using s12: negative
; r0 <- r0, s12
;
;   (0xfffe, 0xfffe, 0x0003, 0x0004) [r0]
;      +       -       +       -
;   (0xfffe, 0xfffe, 0xfffe, 0xfffe) [s12]
;   --------------------------------
;   (0xfffc, 0x0000, 0x0001, 0x0006) [r0]
;
  prep_test_case
  set_reg64 r0, 0xfffefffe, 0x00030004
  vsubadd4h r0, r0, -2     ; -2=0xfffe 
  check_reg64 r0, 0xfffc0000, 0x00010006

;=== vsubadd4h a,limm,c ===

; vsubadd4h a,limm,c 00101110001110110111CCCCCCAAAAAA
;   a.h0 = limm.h0 - c.h0;
;   a.h1 = limm.h1 + c.h1;
;   a.h2 = 0 - c.h2;
;   a.h3 = 0 + c.h3;

; Test case 8
; r0 <- limm, r1
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      +       -       +       -
;   (0x0001, 0x0001, 0xedcc, 0x5678) [r1]
;   --------------------------------
;   (0x0001, 0xffff, 0x0000, 0x0000) [r0]
;
  prep_test_case
  set_reg64 r1, 0x00010001, 0xedcc5678
  vsubadd4h r0, 0x12345678, r1 
  check_reg64 r0, 0x0001ffff, 0x00000000

;=== vsubadd4h a,b,limm ===

; vsubadd4h a,b,limm 00101bbb001110110BBB111110AAAAAA
;   a.h0 = b.h0 - limm.h0;
;   a.h1 = b.h1 + limm.h1;
;   a.h2 = b.h2 - 0;
;   a.h3 = b.h3 + 0;

; Test case 9
; r0 <- r1, limm
;
;   (0x0001, 0x0001, 0xedcc, 0x5678) [r1]
;      +       -       +       -
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;   --------------------------------
;   (0x0001, 0x0001, 0x0000, 0x0000) [r0]
;
  prep_test_case
  set_reg64 r1, 0x00010001, 0xedcc5678
  vsubadd4h r0, r1, 0x12345678 
  check_reg64 r0, 0x00010001, 0x00000000

;=== vsubadd4h 0,limm,c ===

; vsubadd4h 0,limm,c 00101110001110110111CCCCCC111110
;   0 = limm.h0 - c.h0;
;   0 = limm.h1 + c.h1;
;   0 = 0 - c.h2;
;   0 = 0 + c.h3;

; Test case 10
; 0 <- limm, r1
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      +       -       +       -
;   (0x0001, 0x0001, 0xedcc, 0x5678) [r1]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  set_reg64 r1, 0x00010001, 0xedcc5678
  vsubadd4h 0, 0x12345678, r1 

;=== vsubadd4h 0,b,limm ===

; vsubadd4h 0,b,limm 00101bbb001110110BBB111110111110
;   0 = b.h0 - limm.h0;
;   0 = b.h1 + limm.h1;
;   0 = b.h2 - 0;
;   0 = b.h3 + 0;

; Test case 11
; 0 <- r1, limm
;
;   (0x0001, 0x0001, 0xedcc, 0x5678) [r1]
;      +       -       +       -
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  set_reg64 r1, 0x00010001, 0xedcc5678
  vsubadd4h 0, r1, 0x12345678 

;=== vsubadd4h<.cc> b,b,limm ===

; vsubadd4h<.cc> b,b,limm 00101bbb111110110BBB1111100QQQQQ
;   if (cc) {
;     b.h0 = b.h0 - limm.h0;
;     b.h1 = b.h1 + limm.h1;
;     b.h2 = b.h2 - 0;
;     b.h3 = b.h3 + 0;
;   }

; Test case 12.1
; CC=1
; r0 <- r0, limm (cc)
;
;   (0x0001, 0x0001, 0xedcc, 0x5678) [r0]
;      +       -       +       -
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;   --------------------------------
;   (0x0001, 0x0001, 0x0000, 0x0000) [r0]
;
  prep_test_case
  set_reg64 r0, 0x00010001, 0xedcc5678
  add.f    0, 0, 0       ; Z=1
  vsubadd4h.z r0, r0, 0x12345678 
  check_reg64 r0, 0x00010001, 0x00000000

; Test case 12.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_reg64 r0, 0x00010001, 0xedcc5678
  vsubadd4h.z r0, r0, 0x12345678 
  check_reg64 r0, 0x00010001, 0xedcc5678

;=== vsubadd4h<.cc> 0,limm,c ===

; vsubadd4h<.cc> 0,limm,c 00101110111110110111CCCCCC0QQQQQ
;   if (cc) {
;     0.h0 = limm.h0 - c.h0;
;     0.h1 = limm.h1 + c.h1;
;     0.h2 = 0 - c.h2;
;     0.h3 = 0 + c.h3;
;   }

; Test case 13.1
; CC=1
; 0 <- limm, r1 (cc)
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      +       -       +       -
;   (0x0001, 0x0001, 0xedcc, 0x5678) [r1]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  set_reg64 r1, 0x00010001, 0xedcc5678
  add.f    0, 0, 0       ; Z=1
  vsubadd4h.z 0, 0x12345678, r1

; Test case 13.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_reg64 r1, 0x00010001, 0xedcc5678
  vsubadd4h.z 0, 0x12345678, r1

;=== vsubadd4h a,limm,u6 ===

; vsubadd4h a,limm,u6 00101110011110110111uuuuuuAAAAAA
;   a.h0 = limm.h0 - u6;
;   a.h1 = limm.h1 + u6;
;   a.h2 = 0 - u6;
;   a.h3 = 0 + u6;

; Test case 14
; r0 <- limm, u6
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      +       -       +       -
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
;   --------------------------------
;   (0x003f, 0xffc1, 0x1273, 0x5639) [r0]
;
  prep_test_case
  vsubadd4h r0, 0x12345678, 0x3f 
  check_reg64 r0, 0x003fffc1, 0x12735639

;=== vsubadd4h 0,limm,u6 ===

; vsubadd4h 0,limm,u6 00101110011110110111uuuuuu111110
;   0 = limm.h0 - u6;
;   0 = limm.h1 + u6;
;   0 = 0 - u6;
;   0 = 0 + u6;

; Test case 15
; 0 <- limm, u6
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      +       -       +       -
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  vsubadd4h 0, 0x12345678, 0x3f 

;=== vsubadd4h<.cc> 0,limm,u6 ===

; vsubadd4h<.cc> 0,limm,u6 00101110111110110111uuuuuu1QQQQQ
;   if (cc) {
;     0 = limm.h0 - u6;
;     0 = limm.h1 + u6;
;     0 = 0 - u6;
;     0 = 0 + u6;
;   }

; Test case 16.1
; CC=1
; 0 <- limm, u6 (cc)
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      +       -       +       -
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  add.f    0, 0, 0       ; Z=1
  vsubadd4h 0, 0x12345678, 0x3f 

; Test case 16.2
; CC=0
; Instruction is not executed
  prep_test_case
  vsubadd4h 0, 0x12345678, 0x3f 

;=== vsubadd4h 0,limm,s12 ===

; vsubadd4h 0,limm,s12 00101110101110110111ssssssSSSSSS
;   0 = limm.h0 - s12;
;   0 = limm.h1 + s12;
;   0 = 0 - s12;
;   0 = 0 + s12;

; Test case 17
; 0 <- limm, s12
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      +       -       +       -
;   (0xfffe, 0xfffe, 0xfffe, 0xfffe) [s12]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  vsubadd4h 0, 0x12345678, -2   ; -2=0xfffffffe

;=== vsubadd4h a,limm,limm ===

; vsubadd4h a,limm,limm 00101110001110110111111110AAAAAA
;   a.h0 = limm.h0 - limm.h0;
;   a.h1 = limm.h1 + limm.h1;
;   a.h2 = 0 - 0;
;   a.h3 = 0 + 0;

; Test case 18
; r0 <- limm, limm
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      +       -       +       -
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;   --------------------------------
;   (0x0000, 0x0000, 0x2468, 0x0000) [r0]
;
  prep_test_case
  vsubadd4h r0, 0x12345678, 0x12345678
  check_reg64 r0, 0x00000000, 0x24680000

;=== vsubadd4h 0,limm,limm ===

; vsubadd4h 0,limm,limm 00101110001110110111111110111110
;   0 = limm.h0 - limm.h0;
;   0 = limm.h1 + limm.h1;
;   0 = 0 - 0;
;   0 = 0 + 0;

; Test case 19
; 0 <- limm, limm
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      +       -       +       -
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  vsubadd4h 0, 0x12345678, 0x12345678

;=== vsubadd4h<.cc> 0,limm,limm ===

; vsubadd4h<.cc> 0,limm,limm 001011101111101101111111100QQQQQ
;   if (cc) {
;     0 = limm.h0 - limm.h0;
;     0 = limm.h1 + limm.h1;
;     0 = 0 - 0;
;     0 = 0 + 0;
;   }

; Test case 20.1
; CC=1
; 0 <- limm, limm (cc)
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      +       -       +       -
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  add.f    0, 0, 0       ; Z=1
  vsubadd4h.z 0, 0x12345678, 0x12345678

; Test case 20.2
; CC=0
; Instruction is not executed
  prep_test_case
  vsubadd4h.z 0, 0x12345678, 0x12345678

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
  print " vsubadd4h: Quad16-bit SIMD subtract and add\n"
  end
