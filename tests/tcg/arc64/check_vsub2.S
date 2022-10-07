; check_vsub2.S
;
; Tests for vsub2: Dual 32-bit SIMD subtract
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VSUB2 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== vsub2 a,b,c ===

; Test case 1.1
; vsub2 a,b,c 00101bbb001111010BBBCCCCCCAAAAAA
; r0 <- r0, r0
;   r0.w0 = r0.w0 - r0.w0
;   r0.w1 = r0.w1 - r0.w1
; 
;    (0x80000000, 0xffffffff) [r0]
;  - (0x80000000, 0xffffffff) [r0]
; -----------------------------------
;  = (         0,          0) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x80000000, 0xffffffff
  vsub2 r0, r0, r0
  check_reg64 r0, 0, 0

; Test case 1.2
; vsub2 a,b,c 00101bbb001111010BBBCCCCCCAAAAAA
; r0 <- r1, r2
;   r0.w0 = r1.w0 - r2.w0
;   r0.w1 = r1.w1 - r2.w1
; 
;    (0xffffffff, 0x7fffffff) [r1]
;  - (0x80000000, 0xffffffff) [r2]
; -----------------------------------
;  = (0x7fffffff, 0x80000000) [r0]
; 
  prep_test_case
  set_reg64 r1, 0xffffffff, 0x7fffffff
  set_reg64 r2, 0x80000000, 0xffffffff
  vsub2 r0, r1, r2
  check_reg64 r0, 0x7fffffff, 0x80000000

;=== vsub2 0,b,c ===

; Test case 2
; vsub2 0,b,c 00101bbb001111010BBBCCCCCC111110
; 0 <- r1, r2
;   0 = r1.w0 - r2.w0
;   0 = r1.w1 - r2.w1
; 
;    (0xffffffff, 0x7fffffff) [r1]
;  - (0x80000000, 0xffffffff) [r2]
; -----------------------------------
;  = null destination         [0]
; 
  prep_test_case
  set_reg64 r1, 0xffffffff, 0x7fffffff
  set_reg64 r2, 0x80000000, 0xffffffff
  vsub2 0, r1, r2

;=== vsub2<.cc> b,b,c ===

; Test case 3.1
; vsub2<.cc> b,b,c 00101bbb111111010BBBCCCCCC0QQQQQ
; r0 <- r0, r1 (cc)
; Testing when C=1, so vsub2 is executed
; if (cc) {
;   r0.w0 = r0.w0 - r1.w0
;   r0.w1 = r0.w1 - r1.w1
; }
; 
; [C=1]
;    (0x80000002, 0x80000001) [r0]
;  - (0x80000001, 0x80000002) [r1]
; -----------------------------------
;  = (0x00000001, 0xffffffff) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x80000002, 0x80000001
  set_reg64 r1, 0x80000001, 0x80000002
  add.f 0, r0, r0         ; ...C=1
  vsub2.c r0, r0, r1
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r0, 0x00000001, 0xffffffff

; Test case 3.2
; vsub2<.cc> b,b,c 00101bbb111111010BBBCCCCCC0QQQQQ
; r0 <- r0, r1 (cc)
; Testing when C=0, so vsub2 is not executed
; if (cc) {
;   // code not executed
;   r0.w0 = r0.w0 - r1.w0
;   r0.w1 = r0.w1 - r1.w1
; }
; 
; [C=0]
; vsub2.c instruction is not executed
; 
  prep_test_case
  set_reg64 r0, 0x80000002, 0x80000001
  set_reg64 r1, 0x80000001, 0x80000002
  vsub2.c r0, r0, r1
  check_reg64 r0, 0x80000002, 0x80000001

;=== vsub2 a,b,u6 ===

; Test case 4
; vsub2 a,b,u6 00101bbb011111010BBBuuuuuuAAAAAA
; r0 <- r1, u6
;   r0.w0 = r1.w0 - u6
;   r0.w1 = r1.w1 - u6
; 
;    (0x8000007e, 0x0000003f) [r1]
;  - (      0x3f,       0x3f) [u6]
; -----------------------------------
;  = (0x8000003f,        0x0) [r0]
; 
  prep_test_case
  set_reg64 r1, 0x8000007e, 0x0000003f
  vsub2 r0, r1, 0x3f
  check_reg64 r0, 0x8000003f, 0x0

;=== vsub2 0,b,u6 ===

; Test case 5
; vsub2 0,b,u6 00101bbb011111010BBBuuuuuu111110
; 0 <- r1, u6
;   0 = r1.w0 - u6
;   0 = r1.w1 - u6
; 
;    (0x8000007e, 0x0000003f) [r1]
;  - (      0x3f,       0x3f) [u6]
; -----------------------------------
;  = null destination         [0]
;
  prep_test_case
  set_reg64 r1, 0x8000007e, 0x0000003f
  vsub2 0, r1, 0x3f

;=== vsub2<.cc> b,b,u6 ===

; Test case 6.1
; vsub2<.cc> b,b,u6 00101bbb111111010BBBuuuuuu1QQQQQ
; r0 <- r0, u6 (cc)
; Testing when C=1, so vsub2 is executed
; if (cc) {
;   r0.w0 = r0.w0 - u6
;   r0.w1 = r0.w1 - u6
; }
; 
; [C=1]
;    (0x8000007e, 0x8000003f) [r0]
;  - (      0x3f,       0x3f) [u6]
; -----------------------------------
;  = (0x8000003f, 0x80000000) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x8000007e, 0x8000003f
  add.f 0, r0, r0         ; ...C=1
  vsub2.c r0, r0, 0x3f
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r0, 0x8000003f, 0x80000000

; Test case 6.2
; vsub2<.cc> b,b,u6 00101bbb111111010BBBuuuuuu1QQQQQ
; r0 <- r0, u6 (cc)
; Testing when C=0, so vsub2 is not executed
; if (cc) {
;   // code not executed
;   r0.w0 = r0.w0 - u6
;   r0.w1 = r0.w1 - u6
; }
; 
; [C=0]
; vsub2.c instruction is not executed
; 
  prep_test_case
  set_reg64 r0, 0x8000007e, 0x8000003f
  vsub2.c r0, r0, 0x3f
  check_reg64 r0, 0x8000007e, 0x8000003f

;=== vsub2 b,b,s12 ===

; Test case 7
; vsub2 b,b,s12 00101bbb101111010BBBssssssSSSSSS
; r0 <- r, s12
;   r0.w0 = r0.w0 - s12
;   r0.w1 = r0.w1 - s12
; 
;    ( 2048, 0xfffff800) [r0]
;  - (-2048,      -2048) [s12]
; -----------------------------------
;  = ( 4096,          0) [r0]
; 
  prep_test_case
  set_reg64 r0, 2048, 0xfffff800
  vsub2 r0, r0, -2048
  check_reg64 r0, 4096, 0

;=== vsub2 a,limm,c ===

; Test case 8
; vsub2 a,limm,c 00101110001111010111CCCCCCAAAAAA
; r0 <- limm, r1
;   r0.w0 = limm - r1.w0
;   r0.w1 =   0  - r1.w1
; 
;    (          0, 0x12345678) [limm]
;  - (          1, 0x12345678) [r1]
; -----------------------------------
;  = ( 0xffffffff,        0x0) [r0]
; 
  prep_test_case
  set_reg64 r1, 1, 0x12345678
  vsub2 r0, 0x12345678, r1
  check_reg64 r0, 0xffffffff, 0x0

;=== vsub2 a,b,limm ===

; Test case 9
; vsub2 a,b,limm 00101bbb001111010BBB111110AAAAAA
; r0 <- r1, limm
;   r0.w0 = r1.w0 - limm
;   r0.w1 = r1.w1 - 0
; 
;    (          1, 0x12345678) [r1]
;  - (          0, 0x12345678) [limm]
; -----------------------------------
;  = (          1,        0x0) [r0]
; 
  prep_test_case
  set_reg64 r1, 1, 0x12345678
  vsub2 r0, r1, 0x12345678
  check_reg64 r0, 1, 0x0

;=== vsub2 0,limm,c ===

; Test case 10
; vsub2 0,limm,c 00101110001111010111CCCCCC111110
; 0 <- limm, r1
;   0 = limm - r1.w0
;   0 = 0 - r1.w1
; 
;    (          0, 0x12345678) [limm]
;  - (          1, 0x12345678) [r1]
; -----------------------------------
;  = null destination          [0]
; 
  prep_test_case
  set_reg64 r1, 1, 0x12345678
  vsub2 0, 0x12345678, r1

;=== vsub2 0,b,limm ===

; Test case 11
; vsub2 0,b,limm 00101bbb001111010BBB111110111110
; 0 <- r1, limm
;   0 = r1.w0 - limm
;   0 = r1.w1 - 0
; 
;    (          1, 0x12345678) [r1]
;  - (          0, 0x12345678) [limm]
; -----------------------------------
;  = null destination          [0]
; 
  prep_test_case
  set_reg64 r1, 1, 0x12345678
  vsub2 0, r1, 0x12345678

;=== vsub2<.cc> b,b,limm ===

; Test case 12.1
; vsub2<.cc> b,b,limm 00101bbb111111010BBB1111100QQQQQ
; r0 <- r0, limm (cc)
; Testing when C=1, so vsub2 is executed
; if (cc) {
;   r0.w0 = r0.w0 - limm
;   r0.w1 = r0.w1 - 0
; }
; 
; [C=1]
;    (0x80000000, 0x12345678) [r0]
;  - (         0, 0x12345678) [limm]
; -----------------------------------
;  = (0x80000000,        0x0) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x80000000, 0x12345678
  set_reg64 r1, 0x80000000, 0x80000000
  add.f 0, r1, r1         ; ...C=1
  vsub2.c r0, r0, 0x12345678
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r0, 0x80000000, 0x0

; Test case 12.2
; vsub2<.cc> b,b,limm 00101bbb111111010BBB1111100QQQQQ
; r0 <- r0, limm (cc)
; Testing when C=0, so vsub2 is not executed
; if (cc) {
;   // code not executed
;   r0.w0 = r0.w0 - limm
;   r0.w1 = r0.w1 - 0
; }
; 
; [C=0]
; vsub2.c instruction is not executed
;
  prep_test_case
  set_reg64 r0, 0x80000000, 0x12345678
  vsub2.c r0, r0, 0x12345678
  check_reg64 r0, 0x80000000, 0x12345678

;=== vsub2<.cc> 0,limm,c ===

; Test case 13.2
; vsub2<.cc> 0,limm,c 00101110111111010111CCCCCC0QQQQQ
; 0 <- limm, r0 (cc)
; Testing when C=1, so vsub2 is executed
; if (cc) {
;   0 = limm - r0.w0
;   0 = 0 - r0.w1
; }
; 
; [C=1]
;    (          0, 0x12345678) [limm]
;  - (          1, 0x12345678) [r0]
; -----------------------------------
;  = null destination         [0]
; 
  prep_test_case
  set_reg64 r0, 1, 0x12345678
  set_reg64 r1, 0x80000000, 0x80000000
  add.f 0, r1, r1         ; ...C=1
  vsub2.c 0, 0x12345678, r0
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.

; Test case 13.2
; vsub2<.cc> 0,limm,c 00101110111111010111CCCCCC0QQQQQ
; 0 <- limm, r0 (cc)
; Testing when C=0, so vadd2 is not executed
; if (cc) {
;   // code not executed
;   0 = limm - r0.w0
;   0 = 0 - r0.w1
; }
; 
; [C=0]
; vsub2.c instruction is not executed
;
  prep_test_case
  set_reg64 r0, 1, 0x12345678
  vsub2.c 0, 0x12345678, r0

;=== vsub2 a,limm,u6 ===

; Test case 14
; vsub2 a,limm,u6 00101110011111010111uuuuuuAAAAAA
; r0 <- limm, u6
;   r0.w0 = limm - u6
;   r0.w1 = 0 - u6
; 
;    (         0, 0x12345678) [limm]
;  - (      0x3f,       0x3f) [u6]
; -----------------------------------
;  = (0xffffffc1, 0x12345639) [r0]
; 
  prep_test_case
  vsub2 r0, 0x12345678, 0x3f
  check_reg64 r0, 0xffffffc1, 0x12345639

;=== vsub2 0,limm,u6 ===

; Test case 15
; vsub2 0,limm,u6 00101110011111010111uuuuuu111110
; 0 <- limm, u6
;   0 = limm - u6
;   0 = 0 - u6
; 
;    (         0, 0x12345678) [limm]
;  - (      0x3f,       0x3f) [u6]
; -----------------------------------
;  = null destination         [0]
; 
  prep_test_case
  vsub2 0, 0x12345678, 0x3f

;=== vsub2<.cc> 0,limm,u6 ===

; Test case 16.1
; vsub2<.cc> 0,limm,u6 00101110111111010111uuuuuu1QQQQQ
; 0 <- limm, u6 (cc)
; Testing when C=1, so vsub2 is executed
; if (cc) {
;   0 = limm - u6
;   0 = 0 - u6
; }
; 
; [C=1]
;    (         0, 0x12345678) [limm]
;  - (      0x3f,       0x3f) [u6]
; -----------------------------------
;  = null destination         [0]
; 
  prep_test_case
  set_reg64 r1, 0x80000000, 0x80000000
  add.f 0, r1, r1         ; ...C=1
  vsub2.c 0, 0x12345678, 0x3f
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.

; Test case 16.2
; vsub2<.cc> 0,limm,u6 00101110111111010111uuuuuu1QQQQQ
; 0 <- limm, u6 (cc)
; Testing when C=0, so vadd2 is not executed
; if (cc) {
;   // code not executed
;   0 = limm - u6
;   0 = 0 - u6
; }
; 
; [C=0]
; vsub2.c instruction is not executed
;
  prep_test_case
  vsub2.c 0, 0x12345678, 0x3f

;=== vsub2 0,limm,s12 ===

; Test case 17
; vsub2 0,limm,s12 00101110101111010111ssssssSSSSSS
; 0 <- limm, s12
;   0 = limm - s12
;   0 = 0 - s12
; 
;    (    0, 0x12345678) [limm]
;  - ( 2048,       2048) [s12] 
; -----------------------------------
;  = null destination                 [0]
;
  prep_test_case
  vsub2 0, 0x12345678, -2048

;=== vsub2 a,limm,limm ===

; Test case 18
; vsub2 a,limm,limm 00101110001111010111111110AAAAAA
; r0 <- limm, limm
;   r0.w0 = limm - limm
;   r0.w1 = 0 - 0
; 
;    (  0, 0x12345678) [limm]
;  - (  0, 0x12345678) [limm]
; -----------------------------------
;  = (0x0,        0x0) [r0]
; 
  prep_test_case
  vsub2 r0, 0x12345678, 0x12345678
  check_reg64 r0, 0x0, 0x0

;=== vsub2 0,limm,limm ===

; Test case 19
; vsub2 0,limm,limm 00101110001111010111111110111110
; 0 <- limm, limm
;   0 = limm - limm
;   0 = 0 - 0
; 
;    (  0, 0x12345678) [limm]
;  - (  0, 0x12345678) [limm]
; -----------------------------------
;  = null destination  [0]
; 
  prep_test_case
  vsub2 0, 0x12345678, 0x12345678

;=== vsub2<.cc> 0,limm,limm ===

; Test case 20
; vsub2<.cc> 0,limm,limm 001011101111110101111111100QQQQQ
; 0 <- limm, limm (cc)
; Testing when C=1, so vsub2 is executed
; if (cc) {
;   0 = limm - limm
;   0 = 0 - 0
; }
; 
; [C=1]
;    (         0, 0x12345678) [limm]
;  - (         0, 0x12345678) [limm]
; -----------------------------------
;  = null destination         [0]
; 
  prep_test_case
  set_reg64 r1, 0x80000000, 0x80000000
  add.f 0, r1, r1         ; ...C=1
  vsub2.c 0, 0x12345678, 0x12345678
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.

; Test case 16.2
; vsub2<.cc> 0,limm,limm 001011101111110101111111100QQQQQ
; 0 <- limm, limm (cc)
; Testing when C=0, so vadd2 is not executed
; if (cc) {
;   // code not executed
;   0 = limm - limm
;   0 = 0 - 0
; }
; 
; [C=0]
; vsub2.c instruction is not executed
;
  prep_test_case
  vsub2.c 0, 0x12345678, 0x12345678

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
  print " vsub2: Dual 32-bit SIMD subtract\n"
  end
