; check_vsub2h.S
;
; Tests for vsub2h: Dual 16-bit vector subtraction
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VSUB2H ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== vsub2h a,b,c ===

; Test case 1.1
; vsub2h a,b,c 00101bbb000101010BBBCCCCCCAAAAAA
; r0 <- r0, r0
;   r0.h0 = r0.h0 - r0.h0
;   r0.h1 = r0.h1 - r0.h1
; 
;    (------, ------, 0x8000, 0xffff) [r0]
;  - (------, ------, 0x8000, 0xffff) [r0]
; -----------------------------------
;  = (0x8000, 0xffff,    0x0,    0x0) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x8000ffff, 0x8000ffff
  vsub2h r0, r0, r0
  check_reg64 r0, 0x8000ffff, 0x0

; Test case 1.2
; vsub2h a,b,c 00101bbb000101010BBBCCCCCCAAAAAA
; r0 <- r1, r2
;   r0.h0 = r1.h0 - r2.h0
;   r0.h1 = r1.h1 - r2.h1
; 
;    (------, ------, 0xffff, 0x7fff) [r1]
;  - (------, ------, 0x8000, 0xffff) [r2]
; -----------------------------------
;  = (0x1234, 0x5678, 0x7fff, 0x8000) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x12345678, 0x12345678
  set_reg64 r1, 0xffff7fff, 0xffff7fff
  set_reg64 r2, 0x8000ffff, 0x8000ffff
  vsub2h r0, r1, r2
  check_reg64 r0, 0x12345678, 0x7fff8000

;=== vsub2h 0,b,c ===

; Test case 2
; vsub2h 0,b,c 00101bbb000101010BBBCCCCCC111110
; 0 <- r1, r2
;   0 = r1.h0 - r2.h0
;   0 = r1.h1 - r2.h1
; 
;    (------, ------, 0xffff, 0x7fff) [r1]
;  - (------, ------, 0x8000, 0xffff) [r2]
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r1, 0xffff7fff, 0xffff7fff
  set_reg64 r2, 0x8000ffff, 0x8000ffff
  vsub2h 0, r1, r2

;=== vsub2h<.cc> b,b,c ===

; Test case 3.1
; vsub2h<.cc> b,b,c 00101bbb110101010BBBCCCCCC0QQQQQ
; r0 <- r0, r1 (cc)
; Testing when C=1, so vsub2h is executed
; if (cc) {
;   r0.h0 = r0.h0 - r1.h0
;   r0.h1 = r0.h1 - r1.h1
; }
; 
; [C=1]
;    (------, ------, 0x8002, 0x8001) [r0]
;  - (------, ------, 0x8001, 0x8002) [r1]
; -----------------------------------
;  = (0x8005, 0x8004, 0x0001, 0xffff) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x80028001
  set_reg64 r1, 0x80058004, 0x80018002
  add.f 0, r0, r0         ; ...C=1
  vsub2h.c r0, r0, r1
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r0, 0x80058004, 0x0001ffff

; Test case 3.2
; vsub2h<.cc> b,b,c 00101bbb110101010BBBCCCCCC0QQQQQ
; r0 <- r0, r1 (cc)
; Testing when C=0, so vsub2h is not executed
; if (cc) {
;   // code not executed
;   r0.h0 = r0.h0 - r1.h0
;   r0.h1 = r0.h1 - r1.h1
; }
; 
; [C=0]
; vsub2h.c instruction is not executed
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x80028001
  set_reg64 r1, 0x80058004, 0x80018002
  vsub2h.c r0, r0, r1
  check_reg64 r0, 0x80058004, 0x80028001

;=== vsub2h a,b,u6 ===

; Test case 4
; vsub2h a,b,u6 00101bbb010101010BBBuuuuuuAAAAAA
; r0 <- r1, u6
;   r0.h0 = r1.h0 - u6
;   r0.h1 = r1.h1 - u6
; 
;    (------, ------, 0x807e, 0x003f) [r1]
;  - (------, ------,   0x3f,   0x3f) [u6]
; -----------------------------------
;  = (0x1234, 0x5678, 0x803f,    0x0) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x12345678, 0x12345678
  set_reg64 r1, 0x12345678, 0x807e003f
  vsub2h r0, r1, 0x3f
  check_reg64 r0, 0x12345678, 0x803f0000

;=== vsub2h 0,b,u6 ===

; Test case 5
; vsub2h 0,b,u6 00101bbb010101010BBBuuuuuu111110
; 0 <- r1, u6
;   0 = r1.h0 - u6
;   0 = r1.h1 - u6
; 
;    (------, ------, 0x807e, 0x003f) [r1]
;  - (------, ------,   0x3f,   0x3f) [u6]
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r1, 0x12345678, 0x807e003f
  vsub2h 0, r1, 0x3f

;=== vsub2h<.cc> b,b,u6 ===

; Test case 6.1
; vsub2h<.cc> b,b,u6 00101bbb110101010BBBuuuuuu1QQQQQ
; r0 <- r0, u6 (cc)
; Testing when C=1, so vsub2h is executed
; if (cc) {
;   r0.h0 = r0.h0 - u6
;   r0.h1 = r0.h1 - u6
; }
; 
; [C=1]
;    (------, ------, 0x807e, 0x003f) [r0]
;  - (------, ------,   0x3f,   0x3f) [u6]
; -----------------------------------
;  = (0x8005, 0x8004, 0x803f,    0x0) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x807e003f
  add.f 0, r0, r0         ; ...C=1
  vsub2h.c r0, r0, 0x3f
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r0, 0x80058004, 0x803f0000

; Test case 6.2
; vsub2h<.cc> b,b,u6 00101bbb110101010BBBuuuuuu1QQQQQ
; r0 <- r0, u6 (cc)
; Testing when C=0, so vsub2h is not executed
; if (cc) {
;   // code not executed
;   r0.h0 = r0.h0 - u6
;   r0.h1 = r0.h1 - u6
; }
; 
; [C=0]
; vsub2h.c instruction is not executed
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x807e003f
  vsub2h.c r0, r0, 0x3f
  check_reg64 r0, 0x80058004, 0x807e003f

;=== vsub2h b,b,s12 ===

; Test case 7
; vsub2h b,b,s12 00101bbb100101010BBBssssssSSSSSS

; r0 = r0, s12
;   r0.h0 = r0.h0 - s12
;   r0.h1 = r0.h1 - s12
; 
;    (------, ------,   2048, 0xf800) [r0]
;  - (------, ------,  -2048,  -2048) [s12]
; -----------------------------------
;  = (0x2000, 0x1800,   4096,      0) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x20001800, 0x0800f800
  vsub2h r0, r0, -2048
  check_reg64 r0, 0x20001800, 0x10000000

;=== vsub2h a,limm,c ===

; Test case 8
; vsub2h a,limm,c 00101110000101010111CCCCCCAAAAAA
; r0 <- limm, r1
;   r0.h0 = limm.h0 - r1.h0
;   r0.h1 = limm.h1 - r1.h1
; 
;    (------, ------, 0x1234, 0x5678) [limm]
;  - (------, ------, 0x1235, 0x5678) [r1]
; -----------------------------------
;  = (0x1234, 0x5678, 0xffff, 0x0000) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x12345678, 0x12345678
  set_reg64 r1, 0x80058004, 0x12355678
  vsub2h r0, 0x12345678, r1
  check_reg64 r0, 0x12345678, 0xffff0000

;=== vsub2h a,b,limm ===

; Test case 9
; vsub2h a,b,limm 00101bbb000101010BBB111110AAAAAA
; r0 <- r1, limm
;   r0.h0 = r1.h0 - limm.h0
;   r0.h1 = r1.h1 - limm.h1
; 
;    (------, ------, 0x1233, 0x5678) [r1]
;  - (------, ------, 0x1234, 0x5678) [limm]
; -----------------------------------
;  = (0x1234, 0x5678, 0xffff, 0x0000) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x12345678, 0x12345678
  set_reg64 r1, 0x80058004, 0x12335678
  vsub2h r0, r1, 0x12345678
  check_reg64 r0, 0x12345678, 0xffff0000

;=== vsub2h 0,limm,c ===

; Test case 10
; vsub2h 0,limm,c 00101110000101010111CCCCCC111110
; 0 <- limm, r1
;   0 = limm.h0 - r1.h0
;   0 = limm.h1 - r1.h1
; 
;    (------, ------, 0x1234, 0x5678) [limm]
;  - (------, ------, 0x1235, 0x5678) [r1]
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r1, 0x80058004, 0x12355678
  vsub2h 0, 0x12345678, r1
  
;=== vsub2h 0,b,limm ===

; Test case 11
; vsub2h 0,b,limm 00101bbb000101010BBB111110111110
; 0 <- r1, limm
;   0 = r1.h0 - limm.h0
;   0 = r1.h1 - limm.h1
; 
;    (------, ------, 0x1233, 0x5678) [r1]
;  - (------, ------, 0x1234, 0x5678) [limm]
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r1, 0x80058004, 0x12335678
  vsub2h 0, r1, 0x12345678

;=== vsub2h<.cc> b,b,limm ===

; Test case 12.1
; vsub2h<.cc> b,b,limm 00101bbb110101010BBB1111100QQQQQ
; r0 <- r0, limm (cc)
; Testing when C=1, so vsub2h is executed
; if (cc) {
;   r0.h0 = r0.h0 - limm.h0
;   r0.h1 = r0.h1 - limm.h1
; }
; 
; [C=1]
;    (------, ------, 0x1233, 0x5678) [r0]
;  - (------, ------, 0x1234, 0x5678) [limm] 
; -----------------------------------
;  = (0x8005, 0x8004, 0xffff, 0x0000) [0]
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x12335678
  set_reg64 r1, 0x80000000, 0x80000000
  add.f 0, r1, r1         ; ...C=1
  vsub2h.c r0, r0, 0x12345678
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r0, 0x80058004, 0xffff0000

; Test case 12.2
; vsub2h<.cc> b,b,limm 00101bbb110101010BBB1111100QQQQQ
; r0 <- r0, limm (cc)
; Testing when C=0, so vsub2h is not executed
; if (cc) {
;   // code not executed
;   r0.h0 = r0.h0 - limm.h0
;   r0.h1 = r0.h1 - limm.h1
; }
; 
; [C=0]
; vsub2h.c instruction is not executed
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x12335678
  vsub2h.c r0, r0, 0x12345678
  check_reg64 r0, 0x80058004, 0x12335678

;=== vsub2h<.cc> 0,limm,c ===

; Test case 13.1
; vsub2h<.cc> 0,limm,c 00101110110101010111CCCCCC0QQQQQ
; 0 <- limm, r0 (cc)
; Testing when C=1, so vsub2h is executed
; if (cc) {
;   0 = limm.h0 - r0.h0
;   0 = limm.h1 - r0.h1
; }
; 
;    (------, ------, 0x1234, 0x5678) [limm]
;  - (------, ------, 0x1235, 0x5678) [r0]
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x12345678
  set_reg64 r1, 0x80000000, 0x80000000
  add.f 0, r1, r1         ; ...C=1
  vsub2h.c 0, 0x12345678, r0
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.

; Test case 13.2
; vsub2h<.cc> 0,limm,c 00101110110101010111CCCCCC0QQQQQ
; 0 <- limm, r0 (cc)
; Testing when C=0, so vsub2h is not executed
; if (cc) {
;   // code not executed
;   0 = limm.h0 - r0.h0
;   0 = limm.h1 - r0.h1
; }
; 
; [C=0]
; vsub2h.c instruction is not executed
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x12345678
  vsub2h.c 0, 0x12345678, r0

;=== vsub2h a,limm,u6 ===

; Test case 14
; vsub2h a,limm,u6 00101110010101010111uuuuuuAAAAAA
; r0 <- limm, u6
;   r0.h0 = limm.h0 - u6
;   r0.h1 = limm.h1 - u6
; 
;    (------, ------, 0x1234, 0x5678) [limm]
;  - (------, ------,   0x3f,   0x3f) [u6] 
; -----------------------------------
;  = (0x1234, 0x5678, 0x11f5, 0x5639) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x12345678, 0x12345678
  vsub2h r0, 0x12345678, 0x3f 
  check_reg64 r0, 0x12345678, 0x11f55639

;=== vsub2h 0,limm,u6 ===

; Test case 15
; vsub2h 0,limm,u6 00101110010101010111uuuuuu111110
; 0 <- limm, u6
;   0 = limm.h0 - u6
;   0 = limm.h1 - u6
; 
;    (------, ------, 0x1234, 0x5678) [limm]
;  - (------, ------,   0x3f,   0x3f) [u6] 
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  vsub2h 0, 0x12345678, 0x3f 

;=== vsub2h<.cc> 0,limm,u6 ===

; Test case 16.1
; vsub2h<.cc> 0,limm,u6 00101110110101010111uuuuuu1QQQQQ
; 0 <- limm, u6 (cc)
; Testing when C=1, so vsub2h is executed
; if (cc) {
;   0 = limm.h0 - u6
;   0 = limm.h1 - u6
; }
; 
;    (------, ------, 0x1234, 0x5678) [limm]
;  - (------, ------,   0x3f,   0x3f) [u6] 
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r1, 0x80000000, 0x80000000
  add.f 0, r1, r1         ; ...C=1
  vsub2h 0, 0x12345678, 0x3f 
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.

; Test case 16.2
; vsub2h<.cc> 0,limm,u6 00101110110101010111uuuuuu1QQQQQ
; 0 <- limm, u6 (cc)
; Testing when C=0, so vsub2h is not executed
; if (cc) {
;   // code not executed
;   0 = limm.h0 - u6
;   0 = limm.h1 - u6
; }
; 
; [C=0]
; vsub2h.c instruction is not executed
; 
  prep_test_case
  vsub2h 0, 0x12345678, 0x3f 

;=== vsub2h 0,limm,s12 ===

; Test case 17
; vsub2h 0,limm,s12 00101110100101010111ssssssSSSSSS
; 0 <- limm, s12
;   0 = limm.h0 - s12
;   0 = limm.h1 - s12
; 
;    (------, ------, 0xffc1, 0x0000) [limm]
;  - (------, ------,  -2048,  -2048) [s12] 
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  vsub2h 0, 0xffc10000, -2048 

;=== vsub2h a,limm,limm ===

; Test case 18
; vsub2h a,limm,limm 00101110000101010111111110AAAAAA
; r0 <- limm, limm
;   r0 = limm.h0 - limm.h0
;   r0 = limm.h1 - limm.h1
; 
;    (------, ------, 0x8003, 0x8002) [limm]
;  - (------, ------, 0x8003, 0x8002) [limm]
; -----------------------------------
;  = (0x1234, 0x5678,    0x0,    0x0) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x12345678, 0x12345678
  vsub2h r0, 0x80038002, 0x80038002
  check_reg64 r0, 0x12345678, 0x0

;=== vsub2h 0,limm,limm ===

; Test case 19
; vsub2h 0,limm,limm 00101110000101010111111110111110
; 0 <- limm, limm
;   0 = limm.h0 - limm.h0
;   0 = limm.h1 - limm.h1
; 
;    (------, ------, 0x8003, 0x8002) [limm]
;  - (------, ------, 0x8003, 0x8002) [limm]
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  vsub2h 0, 0x80038002, 0x80038002

;=== vsub2h<.cc> 0,limm,limm ===

; Test case 20.1
; vsub2h<.cc> 0,limm,limm 001011101101010101111111100QQQQQ
; 0 <- limm, limm (cc)
; Testing when C=1, so vsub2h is executed
; if (cc) {
;   0 = limm.h0 - limm.h0
;   0 = limm.h1 - limm.h1
; }
; 
;    (------, ------, 0x8003, 0x8002) [limm]
;  - (------, ------, 0x8003, 0x8002) [limm]
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r1, 0x80000000, 0x80000000
  add.f 0, r1, r1         ; ...C=1
  vsub2h.c 0, 0x80038002, 0x80038002
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.

; Test case 20.2
; vsub2h<.cc> 0,limm,limm 001011101101010101111111100QQQQQ
; 0 <- limm, limm (cc)
; Testing when C=0, so vsub2h is not executed
; if (cc) {
;   // code not executed
;   0 = limm.h0 - limm.h0
;   0 = limm.h1 - limm.h1
; }
; 
; [C=0]
; vsub2h.c instruction is not executed
; 
  prep_test_case
  vsub2h.c 0, 0x80038002, 0x80038002

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
  print " vsub2h: Dual 16-bit vector subtraction\n"
  end
