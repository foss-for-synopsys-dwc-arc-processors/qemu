; check_vadd2.S
;
; Tests for vadd2: Dual 32-bit addition
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VADD2 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== vadd2 a,b,c ===

; Test case 1.1
; vadd2 a,b,c 00101bbb001111000BBBCCCCCCAAAAAA
; r0 <- r0, r0
;   r0.w0 = r0.w0 + r0.w0
;   r0.w1 = r0.w1 + r0.w1
; 
;    (0x81111111, 0x82222222) [r0]
;  + (0x81111111, 0x82222222) [r0]
; -----------------------------------
;  = (0x02222222, 0x04444444) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x81111111, 0x82222222
  vadd2 r0, r0, r0
  check_reg64 r0, 0x02222222, 0x04444444

; Test case 1.2
; vadd2 a,b,c 00101bbb001111000BBBCCCCCCAAAAAA
; r0 <- r1, r2
;   r0.w0 = r1.w0 + r2.w0
;   r0.w1 = r1.w1 + r2.w1
; 
;    (0x80000002, 0x80000003) [r1]
;  + (0x80000004, 0x80000005) [r2]
; -----------------------------------
;  = (         6,          8) [r0]
; 
  prep_test_case
  set_reg64 r1, 0x80000002, 0x80000003
  set_reg64 r2, 0x80000004, 0x80000005
  vadd2 r0, r1, r2
  check_reg64 r0, 6, 8

;=== vadd2 0,b,c ===

; Test case 2
; vadd2 0,b,c 00101bbb001111000BBBCCCCCC111110
; 0 <- r1, r2
;   0 = r1.w0 + r2.w0
;   0 = r1.w1 + r2.w1
; 
;    (0x80000002, 0x80000003) [r1]
;  + (0x80000004, 0x80000005) [r2]
; -----------------------------------
;  = null destination         [0]
; 
  prep_test_case
  set_reg64 r1, 0x80000002, 0x80000003
  set_reg64 r2, 0x80000004, 0x80000005
  vadd2 0, r1, r2

;=== vadd2<.cc> b,b,c ===

; Test case 3.1
; vadd2<.cc> b,b,c 00101bbb111111000BBBCCCCCC0QQQQQ
; r0 <- r0, r1 (cc)
; Testing when C=1, so vadd2 is executed
; if (cc) {
;   r0.w0 = r0.w0 + r1.w0
;   r0.w1 = r0.w1 + r1.w1
; }
; 
; [C=1]
;    (0x80000002, 0x80000001) [r0]
;  + (0x80000004, 0x80000003) [r1]
; -----------------------------------
;  = (0x00000006, 0x00000004) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x80000002, 0x80000001
  set_reg64 r1, 0x80000004, 0x80000003
  add.f 0, r0, r0         ; ...C=1
  vadd2.c r0, r0, r1
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r0, 6, 4

; Test case 3.2
; vadd2<.cc> b,b,c 00101bbb111111000BBBCCCCCC0QQQQQ
; r0 <- r0, r1 (cc)
; Testing when C=0, so vadd2 is not executed
; if (cc) {
;   // code not executed
;   r0.w0 = r0.w0 + r1.w0
;   r0.w1 = r0.w1 + r1.w1
; }
; 
; [C=0]
; vadd2.c instruction is not executed
; 
  prep_test_case
  set_reg64 r0, 0x80000002, 0x80000001
  set_reg64 r1, 0x80000004, 0x80000003
  vadd2.c r0, r0, r1
  check_reg64 r0, 0x80000002, 0x80000001

;=== vadd2 a,b,u6 ===

; Test case 4
; vadd2 a,b,u6 00101bbb011111000BBBuuuuuuAAAAAA
; r0 <- r1, u6
;   r0.w0 = r1.w0 + u6
;   r0.w1 = r1.w1 + u6
; 
;    (0xffffffe0, 0x00000000) [r1]
;  + (      0x3f,       0x3f) [u6]
; -----------------------------------
;  = (      0x1f,       0x3f) [r0]
; 
  prep_test_case
  set_reg64 r1, 0xffffffe0, 0x00000000
  vadd2 r0, r1, 0x3f
  check_reg64 r0, 0x1f, 0x3f

;===  vadd2 0,b,u6 ===

; Test case 5
; vadd2 0,b,u6 00101bbb011111000BBBuuuuuu111110
; 0 <- r1, u6
;   0 = r1.w0 + u6
;   0 = r1.w1 + u6
;
;    (0xffffffe0, 0x00000000) [r1]
;  + (      0x3f,       0x3f) [u6]
; -----------------------------------
;  = null destination         [0]
  prep_test_case
  set_reg64 r1, 0xffffffe0, 0x00000000
  vadd2 0, r1, 0x3f
  
;=== vadd2<.cc> b,b,u6 ===

; Test case 6.1
; vadd2<.cc> b,b,u6 00101bbb111111000BBBuuuuuu1QQQQQ
; r0 <- r0, u6 (cc)
; Testing when C=1, so vadd2 is executed
; if (cc) {
;   r0.w0 = r0.w0 + u6
;   r0.w1 = r0.w1 + u6
; }
; 
; [C=1]
;    (0xffffffe0, 0x80000000) [r0]
;  + (      0x3f,       0x3f) [u6]
; -----------------------------------
;  = (      0x1f, 0x8000003f) [r0]
; 
  prep_test_case
  set_reg64 r0, 0xffffffe0, 0x80000000
  add.f 0, r0, r0         ; ...C=1
  vadd2.c r0, r0, 0x3f
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r0, 0x1f, 0x8000003f

; Test case 6.2
; vadd2<.cc> b,b,u6 00101bbb111111000BBBuuuuuu1QQQQQ
; r0 <- r0, u6 (cc)
; Testing when C=0, so vadd2 is not executed
; if (cc) {
;   // code not executed
;   r0.w0 = r0.w0 + u6
;   r0.w1 = r0.w1 + u6
; }
; 
; [C=0]
; vadd2.c instruction is not executed
; 
  prep_test_case
  set_reg64 r0, 0xffffffe0, 0x80000000
  vadd2.c r0, r0, 0x3f
  check_reg64 r0, 0xffffffe0, 0x80000000

;=== vadd2 b,b,s12 ===

; Test case 7
; vadd2 b,b,s12 00101bbb101111000BBBssssssSSSSSS
; r0 <- r, s12
;   r0.w0 = r0.w0 + s12
;   r0.w1 = r0.w1 + s12
; 
;    ( 2048, 2000004096) [r0]
;  + (-2048,      -2048) [s12]
; -----------------------------------
;  = (    0, 2000002048) [r0]
; 
  prep_test_case
  set_reg64 r0, 2048, 2000004096
  vadd2 r0, r0, -2048
  check_reg64 r0, 0, 2000002048

;=== vadd2 a,limm,c ===

; Test case 8
; vadd2 a,limm,c 00101110001111000111CCCCCCAAAAAA
; r0 <- limm, r1
;   r0.w0 = limm + r1.w0
;   r0.w1 =   0  + r1.w1
; 
;    (          0, 0x12345678) [limm]
;  + ( 0xedcba988, 0xedcba988) [r1]
; -----------------------------------
;  = ( 0xedcba988,        0x0) [r0]
; 
  prep_test_case
  set_reg64 r1, 0xedcba988, 0xedcba988
  vadd2 r0, 0x12345678, r1
  check_reg64 r0, 0xedcba988, 0x0

;=== vadd2 a,b,limm ===

; Test case 9
; vadd2 a,b,limm 00101bbb001111000BBB111110AAAAAA
; r0 <- r1, limm
;   r0.w0 = r1.w0 + limm
;   r0.w1 = r1.w1 + 0
; 
;    ( 0xedcba988, 0xedcba988) [r1]
;  + (          0, 0x12345678) [limm]
; -----------------------------------
;  = ( 0xedcba988,        0x0) [r0]
; 
  prep_test_case
  set_reg64 r1, 0xedcba988, 0xedcba988
  vadd2 r0, r1, 0x12345678
  check_reg64 r0, 0xedcba988, 0x0

;=== vadd2 0,limm,c ===

; Test case 10
; vadd2 0,limm,c 00101110001111000111CCCCCC111110
; 0 <- limm, r1
;   0 = limm + r1.w0
;   0 = 0 + r1.w1
; 
;    (          0, 0x12345678) [limm]
;  + ( 0xedcba988, 0xedcba988) [r1]
; -----------------------------------
;  = null destination          [0]
; 
  prep_test_case
  set_reg64 r1, 0xedcba988, 0xedcba988
  vadd2 0, 0x12345678, r1

;=== vadd2 0,b,limm ===

; Test case 11
; vadd2 0,b,limm 00101bbb001111000BBB111110111110
; 0 <- r1, limm
;   0 = r1.w0 + limm
;   0 = r1.w1 + 0
; 
;    ( 0xedcba988, 0xedcba988) [r1]
;  + (          0, 0x12345678) [limm]
; -----------------------------------
;  = null destination          [0]
; 
  prep_test_case
  set_reg64 r1, 0xedcba988, 0xedcba988
  vadd2 0, r1, 0x12345678

;=== vadd2<.cc> b,b,limm ===

; Test case 12.1
; vadd2<.cc> b,b,limm 00101bbb111111000BBB1111100QQQQQ
; r0 <- r0, limm (cc)
; Testing when C=1, so vadd2 is executed
; if (cc) {
;   r0.w0 = r0.w0 + limm
;   r0.w1 = r0.w1 + 0
; }
; 
; [C=1]
;    (0xffffffe0, 0x80000000) [r0]
;  + (         0, 0x12345678) [limm]
; -----------------------------------
;  = (0xffffffe0, 0x92345678) [r0]
; 
  prep_test_case
  set_reg64 r0, 0xffffffe0, 0x80000000
  add.f 0, r0, r0         ; ...C=1
  vadd2.c r0, r0, 0x12345678
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r0, 0xffffffe0, 0x92345678

; Test case 12.2
; vadd2<.cc> b,b,limm 00101bbb111111000BBB1111100QQQQQ
; r0 <- r0, limm (cc)
; Testing when C=0, so vadd2 is not executed
; if (cc) {
;   // code not executed
;   r0.w0 = r0.w0 + limm
;   r0.w1 = r0.w1 + 0
; }
; 
; [C=0]
; vadd2.c instruction is not executed
;
  prep_test_case
  set_reg64 r0, 0xffffffe0, 0x80000000
  vadd2.c r0, r0, 0x12345678
  check_reg64 r0, 0xffffffe0, 0x80000000

;=== vadd2<.cc> 0,limm,c ===

; Test case 13.1
; vadd2<.cc> 0,limm,c 00101110111111000111CCCCCC0QQQQQ
; 0 <- limm, r0 (cc)
; Testing when C=1, so vadd2 is executed
; if (cc) {
;   0 = limm + r0.w0
;   0 = 0 + r0.w1
; }
; 
; [C=1]
;    (         0, 0x12345678) [limm]
;  + (0xffffffe0, 0x80000000) [r0]
; -----------------------------------
;  = null destination         [0]
; 
  prep_test_case
  set_reg64 r0, 0xffffffe0, 0x80000000
  add.f 0, r0, r0         ; ...C=1
  vadd2.c 0, 0x12345678, r0
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.

; Test case 13.2
; vadd2<.cc> 0,limm,c 00101110111111000111CCCCCC0QQQQQ
; 0 <- limm, r0 (cc)
; Testing when C=0, so vadd2 is not executed
; if (cc) {
;   // code not executed
;   0 = limm + r0.w0
;   0 = 0 + r0.w1
; }
; 
; [C=0]
; vadd2.c instruction is not executed
;
  prep_test_case
  set_reg64 r0, 0xffffffe0, 0x80000000
  vadd2.c 0, 0x12345678, r0

;=== vadd2 a,limm,u6 ===

; Test case 14
; vadd2 a,limm,u6 00101110011111000111uuuuuuAAAAAA
; r0 <- limm, u6
;   r0.w0 = limm + u6
;   r0.w1 = 0 + u6
; 
;    (         0, 0x12345678) [limm]
;  + (      0x3f,       0x3f) [u6]
; -----------------------------------
;  = (      0x3f, 0x123456B7) [r0]
; 
  prep_test_case
  vadd2 r0, 0x12345678, 0x3f
  check_reg64 r0, 0x3f, 0x123456B7

;;=== vadd2 0,limm,u6 ===

; Test case 15
; vadd2 0,limm,u6 00101110011111000111uuuuuu111110
; 0 <- limm, u6
;   0 = limm + u6
;   0 = 0 + u6
; 
;    (         0, 0x12345678) [limm]
;  + (      0x3f,       0x3f) [u6]
; -----------------------------------
;  = null destination         [0]
; 
  prep_test_case
  vadd2 0, 0x12345678, 0x3f 

;=== vadd2<.cc> 0,limm,u6 ===

; Test case 16.1
; vadd2<.cc> 0,limm,u6 00101110111111000111uuuuuu1QQQQQ
; 0 <- limm, u6 (cc)
; Testing when C=1, so vadd2 is executed
; if (cc) {
;   0 = limm + u6
;   0 = 0 + u6
; }
; 
; [C=1]
;    (   0, 0x12345678) [limm]
;  + (0x3f,       0x3f) [u6] 
; -----------------------------------
;  = null destination   [0]
; 
  prep_test_case
  set_reg64 r0, 0xffffffe0, 0x80000000
  add.f 0, r0, r0         ; ...C=1
  vadd2.c 0, 0x12345678, 0x3f
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.

; Test case 16.2
; 0 <- limm, u6 (cc)
; vadd2<.cc> 0,limm,u6 00101110111111000111uuuuuu1QQQQQ
; Testing when C=0, so vadd2 is not executed
; if (cc) {
;   // code not executed
;   0 = limm + u6
;   0 = 0 + u6
; }
; 
; [C=0]
; vadd2.c instruction is not executed
;
  prep_test_case
  vadd2.c 0, 0x12345678, 0x3f

;=== vadd2 0,limm,s12 ===

; Test case 17
; vadd2 0,limm,s12 00101110101111000111ssssssSSSSSS
; 0 <- limm, s12
;   0 = limm + s12
;   0 = 0 + s12
; 
;    (    0, 0x12345678) [limm]
;  + (-2048,      -2048) [s12] 
; -----------------------------------
;  = null destination                 [0]
;
  prep_test_case
  vadd2 0, 0x12345678, -2048

;=== vadd2 a,limm,limm ===

; Test case 18
; vadd2 a,limm,limm 00101110001111000111111110AAAAAA
; r0 <- limm, limm
;   r0.w0 = limm + limm
;   r0.w1 = 0 + 0
; 
;    (  0, 0x12345678) [limm]
;  + (  0, 0x12345678) [limm]
; -----------------------------------
;  = (0x0, 0x2468ACF0) [r0]
; 
  prep_test_case
  vadd2 r0, 0x12345678, 0x12345678
  check_reg64 r0, 0x0, 0x2468ACF0

;=== vadd2 0,limm,limm ===

; Test case 19
; vadd2 0,limm,limm 00101110001111000111111110111110
; 0 <- limm, limm
;   0 = limm + limm
;   0 = 0 + 0 
; 
;    (0, 0x12345678)  [limm]
;  + (0, 0x12345678)  [limm]
; -----------------------------------
;  = null destination [0]
;
  prep_test_case
  vadd2 0, 0x12345678, 0x12345678

;=== vadd2<.cc> 0,limm,limm ===

; Test case 20.1
; vadd2<.cc> 0,limm,limm 001011101111110001111111100QQQQQ
; 0 <- limm, limm (cc)
; Testing when C=1, so vadd2 is executed
; if (cc) {
;   0 = limm + limm
;   0 = 0 + 0
; }
; 
; [C=1]
;    (0, 0x12345678)  [limm]
;  + (0, 0x12345678)  [limm]
; -----------------------------------
;  = null destination [0]
; 
  prep_test_case
  set_reg64 r0, 0xffffffe0, 0x80000000
  add.f 0, r0, r0         ; ...C=1
  vadd2.c 0, 0x12345678, 0x12345678
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.

; Test case 20.2
; vadd2<.cc> 0,limm,limm 001011101111110001111111100QQQQQ
; 0 <- limm, limm (cc)
; Testing when C=0, so vadd2 is not executed
; if (cc) {
;   // code not executed
;   0 = limm + limm
;   0 = 0 + 0
; }
; 
; [C=0]
; vadd2.c instruction is not executed
;
  prep_test_case
  vadd2.c 0, 0x12345678, 0x12345678

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
  print " vadd2: Dual 32-bit addition\n"
  end
