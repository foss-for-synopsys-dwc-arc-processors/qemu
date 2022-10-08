; check_vaddsub4h.S
;
; Tests for vaddsub4h: Quad 16-bit SIMD add and subtract.
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VADDSUB4H ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== vaddsub4h a,b,c ===

; vaddsub4h a,b,c 00101bbb001110100BBBCCCCCCAAAAAA
;   a.h0 = b.h0 + c.h0;
;   a.h1 = b.h1 - c.h1;
;   a.h2 = b.h2 + c.h2;
;   a.h3 = b.h3 - c.h3;

; Test case 1.1
; Same source and dest operands
; r0 <- r0, r0
;
;   (0x8004, 0x8003, 0x8002, 0x8001) [r0]
;      -       +       -       +
;   (0x8004, 0x8003, 0x8002, 0x8001) [r0]
;   --------------------------------
;   (0x0000, 0x0006, 0x0000, 0x0002) [r0]
;
  prep_test_case
  set_reg64 r0, 0x80048003, 0x80028001
  vaddsub4h r0, r0, r0
  check_reg64 r0, 0x00000006, 0x00000002

; Test case 1.2
; Different source and dest operands
; r0 <- r1, r2
;
;   (0x8005, 0x8003, 0x0001, 0xfff5) [r1]
;      -       +       -       +
;   (0x8004, 0x8003, 0x0003, 0x00ff) [r2]
;   --------------------------------
;   (0x0001, 0x0006, 0xfffe, 0x00f4) [r0]
;
  prep_test_case
  set_reg64 r1, 0x80058003, 0x0001fff5
  set_reg64 r2, 0x80048003, 0x000300ff
  vaddsub4h r0, r1, r2
  check_reg64 r0, 0x00010006, 0xfffe00f4

;=== vaddsub4h 0,b,c ===

; vaddsub4h 0,b,c 00101bbb001110100BBBCCCCCC111110
;   0 = b.h0 + c.h0;
;   0 = b.h1 - c.h1;
;   0 = b.h2 + c.h2;
;   0 = b.h3 - c.h3;

; Test case 2
; 0 <- r1, r2
;
;   (0x8005, 0x8003, 0x0001, 0xfff5) [r1]
;      -       +       -       +
;   (0x8004, 0x8003, 0x0003, 0x00ff) [r2]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  set_reg64 r1, 0x80058003, 0x0001fff5
  set_reg64 r2, 0x80048003, 0x000300ff
  vaddsub4h 0, r1, r2

;=== vaddsub4h<.cc> b,b,c ===

; vaddsub4h<.cc> b,b,c 00101bbb111110100BBBCCCCCC0QQQQQ
;   if (cc) {
;     a.h0 = b.h0 + c.h0;
;     a.h1 = b.h1 - c.h1;
;     a.h2 = b.h2 + c.h2;
;     a.h3 = b.h3 - c.h3;
;   }

; Test case 3.1
; CC=1
; r2 <- r2, r3 (cc)
;
;   (0x8005, 0x8003, 0x0001, 0xfff5) [r2]
;      -       +       -       +
;   (0x8004, 0x8003, 0x0003, 0x00ff) [r3]
;   --------------------------------
;   (0x0001, 0x0006, 0xfffe, 0x00f4) [r2]
;
  prep_test_case
  set_reg64 r2, 0x80058003, 0x0001fff5
  set_reg64 r3, 0x80048003, 0x000300ff
  add.f    0, 0, 0       ; Z=1
  vaddsub4h.z r2, r2, r3
  check_reg64 r2, 0x00010006, 0xfffe00f4

; Test case 3.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_reg64 r2, 0x80058003, 0x0001fff5
  set_reg64 r3, 0x80048003, 0x000300ff
  vaddsub4h.z r2, r2, r3  ; Z=0 because of PREP_TEST_CASE
  check_reg64 r2, 0x80058003, 0x0001fff5

;=== vaddsub4h a,b,u6 ===

; vaddsub4h a,b,u6 00101bbb011110100BBBuuuuuuAAAAAA
;   a.h0 = b.h0 + u6;
;   a.h1 = b.h1 - u6;
;   a.h2 = b.h2 + u6;
;   a.h3 = b.h3 - u6;

; Test case 4
; r0 <- r1, u6
;
;   (0x003f, 0x003f, 0x0040, 0xfff5) [r1]
;      -       +       -       +
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
;   --------------------------------
;   (0x0000, 0x007e, 0x0001, 0x0034) [r0]
;
  prep_test_case
  set_reg64 r1, 0x003f003f, 0x0040fff5
  vaddsub4h r0, r1, 0x3f
  check_reg64 r0, 0x0000007e, 0x00010034

;=== vaddsub4h 0,b,u6 ===

; vaddsub4h 0,b,u6 00101bbb011110100BBBuuuuuu111110
;   0 = b.h0 + u6;
;   0 = b.h1 - u6;
;   0 = b.h2 + u6;
;   0 = b.h3 - u6;

; Test case 5
; 0 <- r1, u6
;
;   (0x003f, 0x003f, 0x0040, 0xfff5) [r1]
;      -       +       -       +
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  set_reg64 r1, 0x003f003f, 0x0040fff5
  vaddsub4h 0, r1, 0x3f

;=== vaddsub4h<.cc> b,b,u6 ===

; vaddsub4h<.cc> b,b,u6 00101bbb111110100BBBuuuuuu1QQQQQ
;   b.h0 = b.h0 + u6;
;   b.h1 = b.h1 - u6;
;   b.h2 = b.h2 + u6;
;   b.h3 = b.h3 - u6;

; Test case 6.1
; CC=1
; r0 <- r0, u6
;
;   (0x003f, 0x003f, 0x0040, 0xfff5) [r0]
;      -       +       -       +
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
;   --------------------------------
;   (0x0000, 0x007e, 0x0001, 0x0034) [r0]
;
  prep_test_case
  set_reg64 r0, 0x003f003f, 0x0040fff5
  add.f    0, 0, 0       ; Z=1
  vaddsub4h.z r0, r0, 0x3f
  check_reg64 r0, 0x0000007e, 0x00010034

; Test case 6.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_reg64 r0, 0x003f003f, 0x0040fff5
  vaddsub4h.z r0, r0, 0x3f
  check_reg64 r0, 0x003f003f, 0x0040fff5

;=== vaddsub4h b,b,s12 ===

; vaddsub4h b,b,s12 00101bbb101110100BBBssssssSSSSSS
;   b.h0 = b.h0 + s12;
;   b.h1 = b.h1 - s12;
;   b.h2 = b.h2 + s12;
;   b.h3 = b.h3 - s12;

; Test case 7.1
; using s12: positive
; r0 <- r0, s12
;
;   (0x8004, 0x8003, 0x0076, 0xfff5) [r0]
;      -       +       -       +
;   (  0x75,   0x75,   0x75,   0x75) [s12]
;   --------------------------------
;   (0x7f8f, 0x8078, 0x0001, 0x006a) [r0]
;
  prep_test_case
  set_reg64 r0, 0x80048003, 0x0076fff5
  vaddsub4h r0, r0, 0x75
  check_reg64 r0, 0x7f8f8078, 0x0001006a

; Test case 7.2
; using s12: negative
; r0 <- r0, s12
;
;   (0xfffe, 0xfffe, 0x0004, 0x0003) [r0]
;      -       +       -       +
;   (0xfffe, 0xfffe, 0xfffe, 0xfffe) [s12]
;   --------------------------------
;   (0x0000, 0xfffc, 0x0006, 0x0001) [r0]
;
  prep_test_case
  set_reg64 r0, 0xfffefffe, 0x00040003
  vaddsub4h r0, r0, -2     ; -2=0xfffe 
  check_reg64 r0, 0x0000fffc, 0x00060001

;=== vaddsub4h a,limm,c ===

; vaddsub4h a,limm,c 00101110001110100111CCCCCCAAAAAA
;   a.h0 = limm.h0 + c.h0;
;   a.h1 = limm.h1 - c.h1;
;   a.h2 = 0 + c.h2;
;   a.h3 = 0 - c.h3;

; Test case 8
; r0 <- limm, r1
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      -       +       -       +
;   (0x0001, 0x0001, 0x1234, 0xa988) [r1]
;   --------------------------------
;   (0xffff, 0x0001, 0x0000, 0x0000) [r0]
;
  prep_test_case
  set_reg64 r1, 0x00010001, 0x1234a988
  vaddsub4h r0, 0x12345678, r1 
  check_reg64 r0, 0xffff0001, 0x00000000

;=== vaddsub4h a,b,limm ===

; vaddsub4h a,b,limm 00101bbb001110100BBB111110AAAAAA
;   a.h0 = b.h0 + limm.h0;
;   a.h1 = b.h1 - limm.h1;
;   a.h2 = b.h2 + 0;
;   a.h3 = b.h3 - 0;

; Test case 9
; r0 <- r1, limm
;
;   (0x0001, 0x0001, 0x1234, 0xa988) [r1]
;      -       +       -       +
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;   --------------------------------
;   (0x0001, 0x0001, 0x0000, 0x0000) [r0]
;
  prep_test_case
  set_reg64 r1, 0x00010001, 0x1234a988
  vaddsub4h r0, r1, 0x12345678 
  check_reg64 r0, 0x00010001, 0x00000000

;=== vaddsub4h 0,limm,c ===

; vaddsub4h 0,limm,c 00101110001110100111CCCCCC111110
;   0 = limm.h0 + c.h0;
;   0 = limm.h1 - c.h1;
;   0 = 0 + c.h2;
;   0 = 0 - c.h3;

; Test case 10
; 0 <- limm, r1
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      -       +       -       +
;   (0x0001, 0x0001, 0x1234, 0xa988) [r1]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  set_reg64 r1, 0x00010001, 0x1234a988
  vaddsub4h 0, 0x12345678, r1 

;=== vaddsub4h 0,b,limm ===

; vaddsub4h 0,b,limm 00101bbb001110100BBB111110111110
;   0 = b.h0 + limm.h0;
;   0 = b.h1 - limm.h1;
;   0 = b.h2 + 0;
;   0 = b.h3 - 0;

; Test case 11
; 0 <- r1, limm
;
;   (0x0001, 0x0001, 0x1234, 0xa988) [r1]
;      -       +       -       +
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  set_reg64 r1, 0x00010001, 0x1234a988
  vaddsub4h 0, r1, 0x12345678 

;=== vaddsub4h<.cc> b,b,limm ===

; vaddsub4h<.cc> b,b,limm 00101bbb111110100BBB1111100QQQQQ
;   b.h0 = b.h0 + limm.h0;
;   b.h1 = b.h1 - limm.h1;
;   b.h2 = b.h2 + 0;
;   b.h3 = b.h3 - 0;

; Test case 12.1
; CC=1
; r0 <- r0, limm (cc)
;
;   (0x0001, 0x0001, 0x1234, 0xa988) [r0]
;      -       +       -       +
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;   --------------------------------
;   (0x0001, 0x0001, 0x0000, 0x0000) [r0]
;
  prep_test_case
  set_reg64 r0, 0x00010001, 0x1234a988
  add.f    0, 0, 0       ; Z=1
  vaddsub4h.z r0, r0, 0x12345678 
  check_reg64 r0, 0x00010001, 0x00000000

; Test case 12.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_reg64 r0, 0x00010001, 0x1234a988
  vaddsub4h.z r0, r0, 0x12345678 
  check_reg64 r0, 0x00010001, 0x1234a988

;=== vaddsub4h<.cc> 0,limm,c ===

; vaddsub4h<.cc> 0,limm,c 00101110111110100111CCCCCC0QQQQQ
;   0 = limm.h0 + c.h0;
;   0 = limm.h1 - c.h1;
;   0 = 0 + c.h2;
;   0 = 0 - c.h3;

; Test case 13.1
; CC=1
; 0 <- limm, r1 (cc)
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      -       +       -       +
;   (0x0001, 0x0001, 0x1234, 0xa988) [r1]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  set_reg64 r1, 0x00010001, 0x1234a988
  add.f    0, 0, 0       ; Z=1
  vaddsub4h.z 0, 0x12345678, r1

; Test case 13.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_reg64 r1, 0x00010001, 0x1234a988
  vaddsub4h.z 0, 0x12345678, r1

;=== vaddsub4h a,limm,u6 ===

; vaddsub4h a,limm,u6 00101110011110100111uuuuuuAAAAAA
;   a.h0 = limm.h0 + u6;
;   a.h1 = limm.h1 - u6;
;   a.h2 = 0 + u6;
;   a.h3 = 0 - u6;

; Test case 14
; r0 <- limm, u6
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      -       +       -       +
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
;   --------------------------------
;   (0xffc1, 0x003f, 0x11f5, 0x56b7) [r0]
;
  prep_test_case
  vaddsub4h r0, 0x12345678, 0x3f 
  check_reg64 r0, 0xffc1003f, 0x11f556b7

;=== vaddsub4h 0,limm,u6 ===

; vaddsub4h 0,limm,u6 00101110011110100111uuuuuu111110
;   0 = limm.h0 + u6;
;   0 = limm.h1 - u6;
;   0 = 0 + u6;
;   0 = 0 - u6;

; Test case 15
; 0 <- limm, u6
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      -       +       -       +
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  vaddsub4h 0, 0x12345678, 0x3f 

;=== vaddsub4h<.cc> 0,limm,u6 ===

; vaddsub4h<.cc> 0,limm,u6 00101110111110100111uuuuuu1QQQQQ
;   0 = limm.h0 + u6;
;   0 = limm.h1 - u6;
;   0 = 0 + u6;
;   0 = 0 - u6;

; Test case 16.1
; CC=1
; 0 <- limm, u6 (cc)
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      -       +       -       +
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  add.f    0, 0, 0       ; Z=1
  vaddsub4h.z 0, 0x12345678, 0x3f

; Test case 16.2
; CC=0
; Instruction is not executed
  prep_test_case
  vaddsub4h.z 0, 0x12345678, 0x3f

;=== vaddsub4h 0,limm,s12 ===

; vaddsub4h 0,limm,s12 00101110101110100111ssssssSSSSSS
;   0 = limm.h0 + s12;
;   0 = limm.h1 - s12;
;   0 = 0 + s12;
;   0 = 0 - s12;

; Test case 17
; 0 <- limm, s12
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      -       +       -       +
;   (0xfffe, 0xfffe, 0xfffe, 0xfffe) [s12]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  vaddsub4h 0, 0x12345678, -2   ; -2=0xfffffffe

;=== vaddsub4h a,limm,limm ===

; vaddsub4h a,limm,limm 00101110001110100111111110AAAAAA
;   a.h0 = limm.h0 + limm.h0;
;   a.h1 = limm.h1 - limm.h1;
;   a.h2 = 0 + 0;
;   a.h3 = 0 - 0;

; Test case 18
; r0 <- limm, limm
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      -       +       -       +
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;   --------------------------------
;   (0x0000, 0x0000, 0x0000, 0xacf0) [r0]
;
  prep_test_case
  vaddsub4h r0, 0x12345678, 0x12345678
  check_reg64 r0, 0x00000000, 0x0000acf0

;=== vaddsub4h 0,limm,limm ===

; vaddsub4h 0,limm,limm 00101110001110100111111110111110
;   0 = limm.h0 + limm.h0;
;   0 = limm.h1 - limm.h1;
;   0 = 0 + 0;
;   0 = 0 - 0;

; Test case 19
; 0 <- limm, limm
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      -       +       -       +
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  vaddsub4h 0, 0x12345678, 0x12345678

;=== vaddsub4h<.cc> 0,limm,limm ===

; vaddsub4h<.cc> 0,limm,limm 001011101111101001111111100QQQQQ
;   if (cc) {
;     0 = limm.h0 + limm.h0;
;     0 = limm.h1 - limm.h1;
;     0 = 0 + 0;
;     0 = 0 - 0;
;   }

; Test case 20.1
; CC=1
; 0 <- limm, limm (cc)
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;      -       +       -       +
;   (0x0000, 0x0000, 0x1234, 0x5678) [limm]
;   --------------------------------
;   (------, ------, ------, ------) [0]
;
  prep_test_case
  add.f    0, 0, 0       ; Z=1
  vaddsub4h.z 0, 0x12345678, 0x12345678

; Test case 20.2
; CC=0
; Instruction is not executed
  prep_test_case
  vaddsub4h.z 0, 0x12345678, 0x12345678

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
  print " vaddsub4h: Quad 16-bit SIMD add and subtract\n"
  end
