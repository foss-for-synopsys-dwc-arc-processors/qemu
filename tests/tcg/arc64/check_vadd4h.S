; check_vadd4h.S
;
; Tests for vadd4h: Quad 16-bit SIMD addition
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VADD4H ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== vadd4h a,b,c ===

; Test case 1.1
; vadd4h a,b,c 00101bbb001110000BBBCCCCCCAAAAAA
; r0 <- r0, r0
;   r0.h0 = r0.h0 + r0.h0
;   r0.h1 = r0.h1 + r0.h1
;   r0.h2 = r0.h2 + r0.h2
;   r0.h3 = r0.h3 + r0.h3
; 
;    (0x8005, 0x8004, 0x8003, 0x8002) [r0]
;  + (0x8005, 0x8004, 0x8003, 0x8002) [r0]
; -----------------------------------
;  = (0x000a, 0x0008, 0x0006, 0x0004) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x80038002 
  vadd4h r0, r0, r0
  check_reg64 r0, 0x000a0008, 0x00060004

; Test case 1.2
; vadd4h a,b,c 00101bbb001110000BBBCCCCCCAAAAAA
; r0 <- r1, r2
;   r0.h0 = r1.h0 + r2.h0
;   r0.h1 = r1.h1 + r2.h1
;   r0.h2 = r1.h2 + r2.h2
;   r0.h3 = r1.h3 + r2.h3
; 
;    (0x8005, 0x8004, 0x8003, 0x8002) [r1]
;  + (0x8005, 0x8004, 0x0003, 0x0002) [r2]
; -----------------------------------
;  = (0x000a, 0x0008, 0x8006, 0x8004) [r0]
; 
  prep_test_case
  set_reg64 r1, 0x80058004, 0x80038002
  set_reg64 r2, 0x80058004, 0x00030002
  vadd4h r0, r1, r2
  check_reg64 r0, 0x000a0008, 0x80068004

;=== vadd4h 0,b,c ===

; Test case 2
; vadd4h 0,b,c 00101bbb001110000BBBCCCCCC111110
; 0 <- r1, r2
;   0 = r1.h0 + r2.h0
;   0 = r1.h1 + r2.h1
;   0 = r1.h2 + r2.h2
;   0 = r1.h3 + r2.h3
; 
;    (0x8005, 0x8004, 0x8003, 0x8002) [r1]
;  + (0x8005, 0x8004, 0x0003, 0x0002) [r2]
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r1, 0x80058004, 0x80038002
  set_reg64 r2, 0x80058004, 0x00030002
  vadd4h 0, r1, r2

;=== vadd4h<.cc> b,b,c ===

; Test case 3.1
; vadd4h<.cc> b,b,c 00101bbb111110000BBBCCCCCC0QQQQQ
; r0 <- r0, r1 (cc)
; Testing when C=1, so vadd4h is executed
; if (cc) {
;   r0.h0 = r0.h0 + r1.h0
;   r0.h1 = r0.h1 + r1.h1
;   r0.h2 = r0.h2 + r1.h2
;   r0.h3 = r0.h3 + r1.h3
; }
; 
; [C=1]
;    (0x8005, 0x8004, 0x8003, 0x8002) [r0]
;  + (0x8005, 0x8004, 0x0003, 0x0002) [r1]
; -----------------------------------
;  = (0x000, 0xa0008, 0x8006, 0x8004) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x80038002
  set_reg64 r1, 0x80058004, 0x00030002
  add.f 0, r0, r0         ; ...C=1
  vadd4h.c r0, r0, r1
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r0, 0x000a0008, 0x80068004

; Test case 3.2
; vadd4h<.cc> b,b,c 00101bbb111110000BBBCCCCCC0QQQQQ
; r0 <- r0, r1 (cc)
; Testing when C=0, so vadd4h is not executed
; if (cc) {
;   // code not executed
;   r0.h0 = r0.h0 + r1.h0
;   r0.h1 = r0.h1 + r1.h1
;   r0.h2 = r0.h2 + r1.h2
;   r0.h3 = r0.h3 + r1.h3
; }
; 
; [C=0]
; vadd4h.c instruction is not executed
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x80038002
  set_reg64 r1, 0x80058004, 0x00030002
  vadd4h.c r0, r0, r1
  check_reg64 r0, 0x80058004, 0x80038002

;=== vadd4h a,b,u6 ===

; Test case 4
; vadd4h a,b,u6 00101bbb011110000BBBuuuuuuAAAAAA
; r0 <- r1, u6
;   r0.h0 = r1.h0 + u6
;   r0.h1 = r1.h1 + u6
;   r0.h2 = r1.h2 + u6
;   r0.h3 = r1.h3 + u6
; 
;    (0x8005, 0x8004, 0xffc1, 0x0000) [r1]
;  + (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
; -----------------------------------
;  = (0x8044, 0x8043, 0x0000, 0x003f) [r0]
; 
  prep_test_case
  set_reg64 r1, 0x80058004, 0xffc10000
  vadd4h r0, r1, 0x3f
  check_reg64 r0, 0x80448043, 0x0000003f

;=== vadd4h 0,b,u6 ===

; Test case 5
; vadd4h 0,b,u6 00101bbb011110000BBBuuuuuu111110
; 0 <- r1, u6
;   0 = r1.h0 + u6
;   0 = r1.h1 + u6
;   0 = r1.h2 + u6
;   0 = r1.h3 + u6
; 
;    (0x8005, 0x8004, 0xffc1, 0x0000) [r1]
;  + (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r1, 0x80058004, 0xffc10000
  vadd4h 0, r1, 0x3f

;=== vadd4h<.cc> b,b,u6 ===

; Test case 6.1
; vadd4h<.cc> b,b,u6 00101bbb111110000BBBuuuuuu1QQQQQ
; r0 <- r0, u6 (cc)
; Testing when C=1, so vadd4h is executed
; if (cc) {
;   r0.h0 = r0.h0 + u6
;   r0.h1 = r0.h1 + u6
;   r0.h2 = r0.h2 + u6
;   r0.h3 = r0.h3 + u6
; }
; 
; [C=1]
;    (0x8005, 0x8004, 0xffc1, 0x0000) [r0]
;  + (  0x3f,   0x3f,   0x3f,   0x3f) [u6]
; -----------------------------------
;  = (0x8044, 0x8043, 0x0000, 0x003f) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0xffc10000
  add.f 0, r0, r0         ; ...C=1
  vadd4h.c r0, r0, 0x3f
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r0, 0x80448043, 0x0000003f

; Test case 6.2
; vadd4h<.cc> b,b,u6 00101bbb111110000BBBuuuuuu1QQQQQ
; r0 <- r0, u6 (cc)
; Testing when C=0, so vadd4h is not executed
; if (cc) {
;   // code not executed
;   r0.h0 = r0.h0 + u6
;   r0.h1 = r0.h1 + u6
;   r0.h2 = r0.h2 + u6
;   r0.h3 = r0.h3 + u6
; }
; 
; [C=0]
; vadd4h.c instruction is not executed
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0xffc10000
  vadd4h.c r0, r0, 0x3f
  check_reg64 r0, 0x80058004, 0xffc10000

;=== vadd4h b,b,s12 ===

; Test case 7
; vadd4h b,b,s12 00101bbb101110000BBBssssssSSSSSS
; r0 = r0, s12
;   r0.h0 = r0.h0 + s12
;   r0.h1 = r0.h1 + s12
;   r0.h2 = r0.h2 + s12
;   r0.h3 = r0.h3 + s12
; 
;    (0x2000, 0x1800, 0x1000, 0x0800) [r0]
;  + ( -2048,  -2048,  -2048,  -2048) [s12]
; -----------------------------------
;  = (0x1800, 0x1000, 0x0800, 0x0000) [r0]
; 
  prep_test_case
  set_reg64 r0, 0x20001800, 0x10000800
  vadd4h r0, r0, -2048
  check_reg64 r0, 0x18001000, 0x08000000

;=== vadd4h a,limm,c ===

; Test case 8
; vadd4h a,limm,c 00101110001110000111CCCCCCAAAAAA
; r0 <- limm, r1
;   r0.h0 = limm.h0 + r1.h0
;   r0.h1 = limm.h1 + r1.h1
;   r0.h2 = 0 + r1.h2
;   r0.h3 = 0 + r1.h3
; 
;    (     0,      0, 0xf800, 0xf800) [limm]
;  + (0x8005, 0x8004, 0x1000, 0x0800) [r1]
; -----------------------------------
;  = (0x8005, 0x8004, 0x0800, 0x0000) [r0]
; 
  prep_test_case
  set_reg64 r1, 0x80058004, 0x10000800
  vadd4h r0, 0xf800f800, r1
  check_reg64 r0, 0x80058004, 0x08000000

;=== vadd4h a,b,limm ===

; Test case 9
; vadd4h a,b,limm 00101bbb001110000BBB111110AAAAAA
; r0 <- r1, limm
;   r0.h0 = r1.h0 + limm.h0
;   r0.h1 = r1.h1 + limm.h1
;   r0.h2 = r1.h2 + 0
;   r0.h3 = r1.h3 + 0
; 
;    (0x8005, 0x8004, 0x1000, 0x0800) [r1]
;  + (     0,      0, 0xf800, 0xf800) [limm] 
; -----------------------------------
;  = (0x8005, 0x8004, 0x0800, 0x0000) [r0]
; 
  prep_test_case
  set_reg64 r1, 0x80058004, 0x10000800
  vadd4h r0, r1, 0xf800f800
  check_reg64 r0, 0x80058004, 0x08000000

;=== vadd4h 0,limm,c ===

; Test case 10
; vadd4h 0,limm,c 00101110001110000111CCCCCC111110
; 0 <- limm, r1
;   0 = limm.h0 + r1.h0
;   0 = limm.h1 + r1.h1
;   0 = 0 + r1.h2
;   0 = 0 + r1.h3
; 
;    (     0,      0, 0xf800, 0xf800) [limm]
;  + (0x8005, 0x8004, 0x1000, 0x0800) [r1]
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r1, 0x80058004, 0x10000800
  vadd4h 0, 0xf800f800, r1
  
;=== vadd4h 0,b,limm ===

; Test case 11
; vadd4h 0,b,limm 00101bbb001110000BBB111110111110
; 0 <- r1, limm
;   0 = r1.h0 + limm.h0
;   0 = r1.h1 + limm.h1
;   0 = r1.h2 + 0
;   0 = r1.h3 + 0
; 
;    (0x8005, 0x8004, 0x1000, 0x0800) [r1]
;  + (     0,      0, 0xf800, 0xf800) [limm] 
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r1, 0x80058004, 0x10000800
  vadd4h 0, r1, 0xf800f800

;=== vadd4h<.cc> b,b,limm ===

; Test case 12.1
; vadd4h<.cc> b,b,limm 00101bbb111110000BBB1111100QQQQQ
; r0 <- r0, limm (cc)
; Testing when C=1, so vadd4h is executed
; if (cc) {
;   r0.h0 = r0.h0 + limm.h0
;   r0.h1 = r0.h1 + limm.h1
;   r0.h2 = r0.h2 + 0
;   r0.h3 = r0.h3 + 0
; }
; 
; [C=1]
;    (0x8005, 0x8004, 0x8000, 0x0800) [r0]
;  + (     0,      0, 0x8000, 0xf800) [limm] 
; -----------------------------------
;  = (0x8005, 0x8004, 0x0000, 0x0000) [0]
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x80000800
  add.f 0, r0, r0         ; ...C=1
  vadd4h.c r0, r0, 0x8000f800
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r0, 0x80058004, 0x00000000

; Test case 12.2
; vadd4h<.cc> b,b,limm 00101bbb111110000BBB1111100QQQQQ
; r0 <- r0, limm (cc)
; Testing when C=0, so vadd4h is not executed
; if (cc) {
;   // code not executed
;   r0.h0 = r0.h0 + limm.h0
;   r0.h1 = r0.h1 + limm.h1
;   r0.h2 = r0.h2 + 0
;   r0.h3 = r0.h3 + 0
; }
; 
; [C=0]
; vadd4h.c instruction is not executed
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x80000800
  vadd4h.c r0, r0, 0x8000f800
  check_reg64 r0, 0x80058004, 0x80000800

;=== vadd4h<.cc> 0,limm,c ===

; Test case 13.1
; vadd4h<.cc> 0,limm,c 00101110111110000111CCCCCC0QQQQQ
; 0 <- limm, r0 (cc)
; Testing when C=1, so vadd4h is executed
; if (cc) {
;   0 = limm.h0 + r0.h0
;   0 = limm.h1 + r0.h1
;   0 = 0 + r0.h2
;   0 = 0 + r0.h3
; }
; 
; [C=1]
;    (     0,      0, 0x8000, 0xf800) [limm]
;  + (0x8005, 0x8004, 0x8000, 0x0800) [r0] 
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x80000800
  add.f 0, r0, r0         ; ...C=1
  vadd4h.c 0, 0x8000f800, r0
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.

; Test case 13.2
; vadd4h<.cc> 0,limm,c 00101110111110000111CCCCCC0QQQQQ
; 0 <- limm, r0 (cc)
; Testing when C=0, so vadd4h is not executed
; if (cc) {
;   // code not executed
;   0 = limm.h0 + r0.h0
;   0 = limm.h1 + r0.h1
;   0 = 0 + r0.h2
;   0 = 0 + r0.h3
; }
; 
; [C=0]
; vadd4h.c instruction is not executed
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x80000800
  vadd4h.c 0, 0x8000f800, r0

;=== vadd4h a,limm,u6 ===

; Test case 14
; vadd4h a,limm,u6 00101110011110000111uuuuuuAAAAAA
; r0 <- limm, u6
;   r0.h0 = limm.h0 + u6
;   r0.h1 = limm.h1 + u6
;   r0.h2 = 0 + u6
;   r0.h3 = 0 + u6
; 
;    (     0,      0, 0xffc1, 0x0000) [limm]
;  + (  0x3f,   0x3f,   0x3f,   0x3f) [u6] 
; -----------------------------------
;  = (0x003f, 0x003f, 0x0000, 0x003f) [r0]
; 
  prep_test_case
  vadd4h r0, 0xffc10000, 0x3f 
  check_reg64 r0, 0x003f003f, 0x0000003f

;=== vadd4h 0,limm,u6 ===

; Test case 15
; vadd4h 0,limm,u6 00101110011110000111uuuuuu111110
; 0 <- limm, u6
;   0 = limm.h0 + u6
;   0 = limm.h1 + u6
;   0 = 0 + u6
;   0 = 0 + u6
; 
;    (     0,      0, 0xffc1, 0x0000) [limm]
;  + (  0x3f,   0x3f,   0x3f,   0x3f) [u6] 
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  vadd4h 0, 0xffc10000, 0x3f 

;=== vadd4h<.cc> 0,limm,u6 ===

; Test case 16.1
; vadd4h<.cc> 0,limm,u6 00101110111110000111uuuuuu1QQQQQ
; 0 <- limm, u6 (cc)
; Testing when C=1, so vadd4h is executed
; if (cc) {
;   0 = limm.h0 + u6
;   0 = limm.h1 + u6
;   0 = 0 + u6
;   0 = 0 + u6
; }
; 
; [C=1]
;    (     0,      0, 0xffc1, 0x0000) [limm]
;  + (  0x3f,   0x3f,   0x3f,   0x3f) [u6] 
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x80000800
  add.f 0, r0, r0         ; ...C=1
  vadd4h.c 0, 0xffc10000, 0x3f
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.

; Test case 16.2
; vadd4h<.cc> 0,limm,u6 00101110111110000111uuuuuu1QQQQQ
; 0 <- limm, u6 (cc)
; Testing when C=0, so vadd4h is not executed
; if (cc) {
;   // code not executed
;   0 = limm.h0 + u6
;   0 = limm.h1 + u6
;   0 = 0 + u6
;   0 = 0 + u6
; }
; 
; [C=0]
; vadd4h.c instruction is not executed
; 
  prep_test_case
  vadd4h.c 0, 0xffc10000, 0x3f

;=== vadd4h 0,limm,s12 ===

; Test case 17
; vadd4h 0,limm,s12 00101110101110000111ssssssSSSSSS
; 0 <- limm, s12
;   0 = limm.h0 + s12
;   0 = limm.h1 + s12
;   0 = 0 + s12
;   0 = 0 + s12
; 
;    (     0,      0, 0xffc1, 0x0000) [limm]
;  + ( -2048,  -2048,  -2048,  -2048) [s12] 
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  vadd4h 0, 0xffc10000, -2048 

;=== vadd4h a,limm,limm ===

; Test case 18
; vadd4h a,limm,limm 00101110001110000111111110AAAAAA
; r0 <- limm, limm
;   r0 = limm.h0 + limm.h0
;   r0 = limm.h1 + limm.h1
;   r0 = 0
;   r0 = 0
; 
;    (     0,      0, 0x8003, 0x8002) [limm]
;  + (     0,      0, 0x8003, 0x8002) [limm]
; -----------------------------------
;  = (0x0000, 0x0000, 0x0006, 0x0004) [r0]
; 
  prep_test_case
  vadd4h r0, 0x80038002, 0x80038002
  check_reg64 r0, 0x00000000, 0x00060004

;=== vadd4h 0,limm,limm ===

; Test case 19
; vadd4h 0,limm,limm 00101110001110000111111110111110
; 0 <- limm, limm
;   0 = limm.h0 + limm.h0
;   0 = limm.h1 + limm.h1
;   0 = 0
;   0 = 0
; 
;    (     0,      0, 0x8003, 0x8002) [limm]
;  + (     0,      0, 0x8003, 0x8002) [limm]
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  vadd4h 0, 0x80038002, 0x80038002

;=== vadd4h<.cc> 0,limm,limm ===

; Test case 20.1
; vadd4h<.cc> 0,limm,limm 001011101111100001111111100QQQQQ
; 0 <- limm, limm (cc)
; Testing when C=1, so vadd4h is executed
; if (cc) {
;   0 = limm.h0 + limm.h0
;   0 = limm.h1 + limm.h1
;   0 = 0
;   0 = 0
; }
; 
; [C=1]
;    (     0,      0, 0x8003, 0x8002) [limm]
;  + (     0,      0, 0x8003, 0x8002) [limm]
; -----------------------------------
;  = null destination                 [0]
; 
  prep_test_case
  set_reg64 r0, 0x80058004, 0x80000800
  add.f 0, r0, r0         ; ...C=1
  vadd4h.c 0, 0x80038002, 0x80038002
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.

; Test case 20.2
; vadd4h<.cc> 0,limm,limm 001011101111100001111111100QQQQQ
; 0 <- limm, limm (cc)
; Testing when C=0, so vadd4h is not executed
; if (cc) {
;   // code not executed
;   0 = limm.h0 + limm.h0
;   0 = limm.h1 + limm.h1
;   0 = 0
;   0 = 0
; }
; 
; [C=0]
; vadd4h.c instruction is not executed
; 
  prep_test_case
  vadd4h.c 0, 0x80038002, 0x80038002
  
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
  print " vadd4h: Quad 16-bit SIMD addition\n"
  end