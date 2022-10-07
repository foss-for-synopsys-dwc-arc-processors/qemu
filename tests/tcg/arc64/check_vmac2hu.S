; check_vmac2hu.S
;
; Tests for vmac2hu: Unsigned multiplication and accumulation of two 16-bit vectors
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

; Set accumulator to "hi:low"
.macro set_accu64 hi, low
  movhl  r58, \hi
  orl    r58, r58, \low
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VMAC2HU ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== vmac2hu a,b,c ===

; vmac2hu a,b,c 00101bbb000111110BBBCCCCCCAAAAAA
;   a.w0 = acclo + (b.h0 * c.h0);
;   a.w1 = acchi + (b.h1 * c.h1);

; Test case 1.1
; Same source and dest operands
; r0 <- r0, r0
;
;   r0.w0 = acclo + (r0.h0 * r0.h0)
;         = 0x0 + (0x1 * 0x1)
;         = 0x1
;   r0.w1 = acchi + (r0.h1 * r0.h1)
;         = 0x0 + (0x2 * 0x2)
;         = 0x4
;
  prep_test_case
  set_accu64 0x0, 0x0
  set_reg64 r0, 0x00040003, 0x00020001
  vmac2hu r0, r0, r0
  check_reg64 r0, 4, 1
  check_reg64 r58, 4, 1

; Test case 1.2
; Different source and dest operands
; r0 <- r1, r2
;
;   r0.w0 = acclo + (r1.h0 * r2.h0)
;         = 0x0 + (0x1 * 0x3)
;         = 0x3
;   r0.w1 = acchi + (r1.h1 * r2.h1)
;         = 0x0 + (0x2 * 0x4)
;         = 0x8
;
  prep_test_case
  set_accu64 0x0, 0x0
  set_reg64 r1, 0x11111111, 0x00020001
  set_reg64 r2, 0x22222222, 0x00040003
  vmac2hu r0, r1, r2
  check_reg64 r0, 8, 3
  check_reg64 r58, 8, 3

; Test case 1.3
; Non-zero accumulator and same source and dest operands.
; r0 <- r0, r1
;
;   r0.w0 = acclo + (r0.h0 * r1.h0)
;         = 0x15 + (0x9 * 0x9)
;         = 0x66
;   r0.w1 = acchi + (r0.h1 * r1.h1)
;         = 0x14 + (0x11 * 0x1c)
;         = 0x1f0
;
  prep_test_case
  set_accu64 20, 21 ; 0x14, 0x15
  set_reg64 r0, 0x11111111, 0x00110009
  set_reg64 r1, 0x22222222, 0x001c0009
  vmac2hu r0, r0, r1
  check_reg64 r0, 0x1f0, 0x66
  check_reg64 r58, 0x1f0, 0x66

; Test case 1.4
; Bigger numbers.
; r0 <- r2, r3
;
;   r0.w0 = acclo + (r2.h0 * r3.h0)
;         = 0x9abcdef0 + (0xba98 * 0x3210)
;         = 0xbf3a3870
;   r0.w1 = acchi + (r2.h1 * r3.h1)
;         = 0x12345678 + (0xfedc * 0x7654)
;         = 0x88015ea8
;
  prep_test_case
  set_accu64 0x12345678, 0x9abcdef0
  set_reg64 r2, 0x11111111, 0xfedcba98
  set_reg64 r3, 0x22222222, 0x76543210
  vmac2hu r0, r2, r3
  check_reg64 r0, 0x88015ea8, 0xbf3a3870
  check_reg64 r58, 0x88015ea8, 0xbf3a3870

; Test case 1.5
; Bigger numbers: enter the would-be-negative range of numbers.
; The addition should also cause an overflow (no flag is ever set).
; r0 <- r2, r3
;
;   r0.w0 = acclo + (r2.h0 * r3.h0)
;         = 0x80000000 + (0xbbbb * 0xbbbb)
;         = 0x1_09aaba99 => 0x09aaba99
;   r0.w1 = acchi + (r2.h1 * r3.h1)
;         = 0x44556677 + (0xffee * 0xccdd)
;         = 0x1_1123feed => 0x1123feed
;
  prep_test_case
  set_accu64 0x44556677, 0x80000000
  set_reg64 r2, 0x11111111, 0xffeebbbb
  set_reg64 r3, 0x22222222, 0xccddbbbb
  vmac2hu r0, r2, r3
  check_reg64 r0, 0x1123feed, 0x09aaba99
  check_reg64 r58, 0x1123feed, 0x09aaba99

; Test case 1.6
; Pushing the limits.
; r0 <- r0, r0
;
;   r0.w0 = acclo + (r0.h0 * r0.h0)
;         = 0xffffffff + (0x1 * 0x1)
;         = 0x1_00000000 => 0x0
;   r0.w1 = acchi + (r0.h1 * r0.h1)
;         = 0x0001ffff + (0xffff * 0xffff)
;         = 0x1_00000000 => 0x0
;
  prep_test_case
  set_accu64 0x0001ffff, 0xffffffff
  set_reg64 r0, 0x11111111, 0xffff0001
  vmac2hu r0, r0, r0
  check_reg64 r0, 0, 0
  check_reg64 r58, 0, 0

;=== vmac2hu 0,b,c ===

; vmac2hu 0,b,c 00101bbb000111110BBBCCCCCC111110
;   0.w0 = acclo + (b.h0 * c.h0)
;   0.w1 = acchi + (b.h1 * c.h1)

; Test case 2
; 0 <- r2, r3
;
;   0.w0 = acclo + (r2.h0 * r3.h0)
;        = 0x1 + (0x1234 * 0x4321)
;        = 0x04c5f4b5
;   0.w1 = acchi + (r2.h1 * r3.h1)
;        = 0x2 + (0x0002 * 0x8765)
;        = 0x00010ecc
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r2, 0x11111111, 0x00021234
  set_reg64 r3, 0x22222222, 0x87654321
  vmac2hu 0, r2, r3
  check_reg64 r58, 0x10ecc, 0x4c5f4b5

;=== vmac2hu<.cc> b,b,c ===

; vmac2hu<.cc> b,b,c 00101bbb110111110BBBCCCCCC0QQQQQ
;   if (cc) {
;     b.w0 = acclo + (b.h0 * c.h0)
;     b.w1 = acchi + (b.h1 * c.h1)
;   }

; Test case 3.1
; r2 <- r2, r3 (cc)
; Testing when C=1 => vmac2hu is executed
;
;   r2.w0 = acclo + (r2.h0 * r3.h0)
;         = 0x1 + (0x1234 * 0x4321)
;         = 0x4c5f4b5
;   r2.w1 = acchi + (r2.h1 * r3.h1)
;         = 0x2 + (0x0002 * 0x8765)
;         = 0x10ecc
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r2, 0x11111111, 0x00021234
  set_reg64 r3, 0x22222222, 0x87654321
  set_reg64 r4, 0x80000000, 0x80000000
  add.f 0, r4, r4         ; ...C=1
  vmac2hu.c r2, r2, r3
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r2, 0x10ecc, 0x4c5f4b5
  check_reg64 r58, 0x10ecc, 0x4c5f4b5

; Test case 3.2
; r2 <- r2, r3 (cc)
; Testing when C=0 => vmac2hu is not executed
; 
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r2, 0x11111111, 0x00021234
  set_reg64 r3, 0x22222222, 0x87654321
  vmac2hu.c r2, r2, r3
  check_reg64 r2, 0x11111111, 0x00021234
  check_reg64 r58, 0x2, 0x1

;=== vmac2hu a,b,u6 ===

; vmac2hu a,b,u6 00101bbb010111110BBBuuuuuuAAAAAA
;   a.w0 = acclo + (b.h0 * u6)
;   a.w1 = acchi + (b.h1 * u6)

; Test case 4
; r0 <- r1, u6
; 
;   r0.w0 = acclo + (r1.h0 * u6)
;         = 0x5 + (0x03e8 * 3)
;         = 0xbbd [3005]
;   r0.w1 = acchi + (r1.h1 * u6)
;         = 0x4 + (0x0064 * 3)
;         = 0x130 [304]
;
  prep_test_case
  set_accu64 0x4, 0x5
  set_reg64 r1, 0x11111111, 0x006403e8
  vmac2hu r0, r1, 3
  check_reg64 r0, 304, 3005
  check_reg64 r58, 304, 3005

;=== vmac2hu 0,b,u6 ===

; vmac2hu 0,b,u6 00101bbb010111110BBBuuuuuu111110
;   0.w0 = acclo + (b.h0 * u6)
;   0.w1 = acchi + (b.h1 * u6)

; Test case 4
; 0 <- r1, u6
; 
;   0.w0 = acclo + (r1.h0 * u6)
;        = 0x5 + (0x03e8 * 3)
;        = 0xbbd [3005]
;   0.w1 = acchi + (r1.h1 * u6)
;        = 0x4 + (0x0064 * 3)
;        = 0x130 [304]
;
  prep_test_case
  set_accu64 0x4, 0x5
  set_reg64 r1, 0x11111111, 0x006403e8
  vmac2hu 0, r1, 3
  check_reg64 r58, 304, 3005

;=== vmac2hu<.cc> b,b,u6 ===

; vmac2hu<.cc> b,b,u6 00101bbb110111110BBBuuuuuu1QQQQQ
;   if (cc) {
;     b.w0 = acclo + (b.h0 * u6)
;     b.w1 = acchi + (b.h1 * u6)
;   }

; Test case 5.1
; r1 <- r1, u6 (cc)
; Testing when C=1 => vmac2hu is executed
;
;   r1.w0 = acclo + (r1.h0 * u6)
;         = 0x5 + (0x03e8 * 3)
;         = 0xbbd [3005]
;   r1.w1 = acchi + (r1.h1 * u6)
;         = 0x4 + (0x0064 * 3)
;         = 0x130 [304]
;
  prep_test_case
  set_accu64 0x4, 0x5
  set_reg64 r1, 0x11111111, 0x006403e8
  set_reg64 r4, 0x80000000, 0x80000000
  add.f 0, r4, r4         ; ...C=1
  vmac2hu.c r1, r1, 3
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r1, 304, 3005
  check_reg64 r58, 304, 3005

; Test case 5.2
; r1 <- r1, u6 (cc)
; Testing when C=0 => vmac2hu is not executed
;
  prep_test_case
  set_accu64 0x4, 0x5
  set_reg64 r1, 0x11111111, 0x006403e8
  vmac2hu.c r1, r1, 3
  check_reg64 r1, 0x11111111, 0x006403e8
  check_reg64 r58, 0x4, 0x5

;=== vmac2hu b,b,s12 ===

; vmac2hu b,b,s12 00101bbb100111110BBBssssssSSSSSS
;   b.w0 = acclo + (b.h0 * s12)
;   b.w1 = acchi + (b.h1 * s12)

; Test case 7.1
; using simm12: positive
; r0 <- r0, s12
; 
;   r0.w0 = acclo + (r0.h0 * s12)
;         = 0x30 + (0x7fff * 0x1FF)
;         = 0xff7e31 [16743985]
;   r0.w1 = acchi + (r0.h1 * s12)
;         = 0x20 + (0xffff * 0x1FF)
;         = 0x1fefe21 [33488417]
;
  prep_test_case
  set_accu64 0x20, 0x30
  set_reg64 r0, 0x11111111, 0xffff7fff
  vmac2hu r0, r0, 511
  check_reg64 r0, 33488417, 16743985
  check_reg64 r58, 33488417, 16743985

; Test case 7.2
; using simm12: negative
; r0 <- r0, s12
; 
;   r0.w0 = acclo + (r0.h0 * s12)
;         = 0x63 + (0x0002 * 0xf800)
;         = 0x1f063
;   r0.w1 = acchi + (r0.h1 * s12)
;         = 0x9 + (0x0001 * 0xf800)
;         = 0xf809
;
  prep_test_case
  set_accu64 9, 99  ; 0x9, 0x63
  set_reg64 r0, 0x11111111, 0x00010002 
  vmac2hu r0, r0, -2048
  check_reg64  r0, 0xf809, 0x1f063
  check_reg64 r58, 0xf809, 0x1f063

;=== vmac2hu a,limm,c ===

; vmac2hu a,limm,c 00101110000111110111CCCCCCAAAAAA
;   a.w0 = acclo + (limm.ho * c.h0)
;   a.w1 = acchi + (limm.h1 * c.h1)

; Test case 8
; r2 <- limm, r3
; 
;   r2.w0 = acclo + (limm.h0 * r3.h0)
;         = 0x457 + (0x7c65 * 0x1a85)
;         = 0xce2e6d0
;   r2.w1 = acchi + (limm.h1 * r3.h1)
;         = 0x10E1 + (0x83cd * 0x3039)
;         = 0x18d3d986
;
  prep_test_case
  set_accu64 4321, 1111 ; 0x10E1, 0x457
  set_reg64 r3, 0x11111111, 0x30391a85
  vmac2hu r2, 0x83cd7c65, r3
  check_reg64 r2, 0x18d3d986, 0xce2e6d0
  check_reg64 r58, 0x18d3d986, 0xce2e6d0

;=== vmac2hu a,b,limm ===

; vmac2hu a,b,limm 00101bbb000111110BBB111110AAAAAA
;   a.w0 = acclo + (b.h0 * limm.ho)
;   a.w1 = acchi + (b.h1 * limm.h1)

; Test case 9
; r2 <- r3, limm
; 
;   r2.w0 = acclo + (r3.h0 * limm.h0)
;         = 0x457 + (0x1a85 * 0x7c65)
;         = 0xce2e6d0
;   r2.w1 = acchi + (r3.h1 * limm.h1)
;         = 0x10E1 + (0x3039 * 0x83cd)
;         = 0x18d3d986
;
  prep_test_case
  set_accu64 4321, 1111 ; 0x10E1, 0x457
  set_reg64 r3, 0x11111111, 0x30391a85
  vmac2hu r2, r3, 0x83cd7c65
  check_reg64 r2, 0x18d3d986, 0xce2e6d0
  check_reg64 r58, 0x18d3d986, 0xce2e6d0

;=== vmac2hu 0,limm,c ===

; vmac2hu 0,limm,c 00101110000111110111CCCCCC111110
;   0.w0 = acclo + (limm.ho * c.h0)
;   0.w1 = acchi + (limm.h1 * c.h1)

; Test case 10
; 0 <- limm, r3
; 
;   0.w0 = acclo + (limm.h0 * r3.h0)
;        = 0x457 + (0x7c65 * 0x1a85)
;        = 0xce2e6d0
;   0.w1 = acchi + (limm.h1 * r3.h1)
;        = 0x10E1 + (0x83cd * 0x3039)
;        = 0x18d3d986
;
  prep_test_case
  set_accu64 4321, 1111 ; 0x10E1, 0x457
  set_reg64 r3, 0x11111111, 0x30391a85
  vmac2hu 0, 0x83cd7c65, r3
  check_reg64 r58, 0x18d3d986, 0xce2e6d0

;=== vmac2hu 0,b,limm ===

; vmac2hu 0,b,limm 00101bbb000111110BBB111110111110
;   0.w0 = acclo + (b.h0 * limm.ho)
;   0.w1 = acchi + (b.h1 * limm.h1)

; Test case 11
; r2 <- r3, limm
; 
;   0.w0 = acclo + (r3.h0 * limm.h0)
;        = 0x457 + (0x1a85 * 0x7c65)
;        = 0xce2e6d0
;   0.w1 = acchi + (r3.h1 * limm.h1)
;        = 0x10E1 + (0x3039 * 0x83cd)
;        = 0x18d3d986
;
  prep_test_case
  set_accu64 4321, 1111 ; 0x10E1, 0x457
  set_reg64 r3, 0x11111111, 0x30391a85
  vmac2hu 0, r3, 0x83cd7c65
  check_reg64 r58, 0x18d3d986, 0xce2e6d0

;=== vmac2hu<.cc> b,b,limm ===

; vmac2hu<.cc> b,b,limm 00101bbb110111110BBB1111100QQQQQ
;   if (cc) {
;     b.w0 = acclo + (b.h0 * limm.h0)
;     b.w1 = acchi + (b.h1 * limm.h1)
;   }

; Test case 12.1
; r2 <- r2, limm (cc)
; Testing when C=1 => vmac2hu is executed
;
;   r2.w0 = acclo + (r3.h0 * limm.h0)
;         = 0x457 + (0x1a85 * 0x7c65)
;         = 0xce2e6d0
;   r2.w1 = acchi + (r3.h1 * limm.h1)
;         = 0x10E1 + (0x3039 * 0x83cd)
;         = 0x18d3d986
;
  prep_test_case
  set_accu64 4321, 1111 ; 0x10E1, 0x457
  set_reg64 r2, 0x11111111, 0x30391a85
  set_reg64 r4, 0x80000000, 0x80000000
  add.f 0, r4, r4         ; ...C=1
  vmac2hu.c r2, r2, 0x83cd7c65
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r2, 0x18d3d986, 0xce2e6d0
  check_reg64 r58, 0x18d3d986, 0xce2e6d0

; Test case 12.2
; r2 <- r2, limm (cc)
; Testing when C=0 => vmac2hu is not executed
;
  prep_test_case
  set_accu64 4321, 1111 ; 0x10E1, 0x457
  set_reg64 r2, 0x11111111, 0x30391a85
  vmac2hu.c r2, r2, 0x83cd7c65
  check_reg64 r2, 0x11111111, 0x30391a85
  check_reg64 r58, 4321, 1111

;=== vmac2hu<.cc> 0,limm,c ===

; vmac2hu<.cc> 0,limm,c 00101110110111110111CCCCCC0QQQQQ
;   if (cc) {
;     0.w0 = acclo + (limm.h0 * b.h0)
;     0.w1 = acchi + (limm.h1 * b.h1)
;   }

; Test case 13.1
; 0 <- limm, r3 (cc)
; Testing when C=1 => vmac2hu is executed
;
;   0.w0 = acclo + (limm.h0 * r3.h0)
;        = 0x457 + (0x7c65 * 0x1a85)
;        = 0xce2e6d0
;   0.w1 = acchi + (limm.h1 * r3.h1)
;        = 0x10E1 + (0x83cd * 0x3039)
;        = 0x18d3d986
;
  prep_test_case
  set_accu64 4321, 1111 ; 0x10E1, 0x457
  set_reg64 r3, 0x11111111, 0x30391a85
  set_reg64 r4, 0x80000000, 0x80000000
  add.f 0, r4, r4         ; ...C=1
  vmac2hu.c 0, 0x83cd7c65, r3
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r58, 0x18d3d986, 0xce2e6d0

; Test case 13.2
; 0 <- limm, r3 (cc)
; Testing when C=0 => vmac2hu is not executed
;
  prep_test_case
  set_accu64 4321, 1111 ; 0x10E1, 0x457
  set_reg64 r3, 0x11111111, 0x30391a85
  vmac2hu.c 0, 0x83cd7c65, r3
  check_reg64 r58, 4321, 1111

;=== vmac2hu a,limm,u6 ===

; vmac2hu a,limm,u6 00101110010111110111uuuuuuAAAAAA
;   a.w0 = acclo + (limm.h0 * u6)
;   a.w1 = acchi + (limm.h1 * u6)

; Test case 14
; r0 <- limm, u6
; 
;   r0.w0 = acclo + (limm.h0 * u6)
;         = 0xdeadbeef + (0x223d * 0x2a)
;         = 0xdeb35cf1
;   r0.w1 = acchi + (limm.h1 * u6)
;         = 0x1337cafe + (0xa8ca * 0x2a)
;         = 0x13537c22
;
  prep_test_case
  set_accu64 0x1337cafe, 0xdeadbeef
  vmac2hu r0, 0xa8ca223d, 42 ; 0x2a
  check_reg64 r0, 0x13537c22, 0xdeb35cf1
  check_reg64 r58, 0x13537c22, 0xdeb35cf1

;=== vmac2hu 0,limm,u6 ===

; vmac2hu 0,limm,u6 00101110010111110111uuuuuu111110
;   0.w0 = acclo + (limm.h0 * u6)
;   0.w1 = acchi + (limm.h1 * u6)

; Test case 15
; 0 <- limm, u6
; 
;   0.w0 = acclo + (limm.h0 * u6)
;        = 0xdeadbeef + (0x223d * 0x2a)
;        = 0xdeb35cf1
;   0.w1 = acchi + (limm.h1 * u6)
;        = 0x1337cafe + (0xa8ca * 0x2a)
;        = 0x13537c22
;
  prep_test_case
  set_accu64 0x1337cafe, 0xdeadbeef
  vmac2hu 0, 0xa8ca223d, 42 ; 0x2a
  check_reg64 r58, 0x13537c22, 0xdeb35cf1

;=== vmac2hu<.cc> 0,limm,u6 ===

; vmac2hu<.cc> 0,limm,u6 00101110110111110111uuuuuu1QQQQQ
;   if (cc) {
;     0.w0 = acclo + (limm.h0 * u6)
;     0.w1 = acchi + (limm.h1 * u6)
;   }

; Test case 16.1
; 0 <- limm, u6 (cc)
; Testing when C=1 => vmac2hu is executed
; 
;   0.w0 = acclo + (limm.h0 * u6)
;        = 0xdeadbeef + (0x223d * 0x2a)
;        = 0xdeb35cf1
;   0.w1 = acchi + (limm.h1 * u6)
;        = 0x1337cafe + (0xa8ca * 0x2a)
;        = 0x13537c22
;
  prep_test_case
  set_accu64 0x1337cafe, 0xdeadbeef
  set_reg64 r4, 0x80000000, 0x80000000
  add.f 0, r4, r4         ; ...C=1
  vmac2hu.c 0, 0xa8ca223d, 42 ; 0x2a
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r58, 0x13537c22, 0xdeb35cf1

; Test case 16.2
; 0 <- limm, u6 (cc)
; Testing when C=0 => vmac2hu is not executed
  prep_test_case
  set_accu64 0x1337cafe, 0xdeadbeef
  vmac2hu.c 0, 0xa8ca223d, 42 ; 0x2a
  check_reg64 r58, 0x1337cafe, 0xdeadbeef

;=== vmac2hu 0,limm,s12 ===

; vmac2hu 0,limm,s12 00101110100111110111ssssssSSSSSS
;   0.w0 = acclo + (limm.h0 * s12)
;   0.w1 = acchi + (limm.h1 * s12)

; Test case 17.1
; using simm12: positive
; 0 <- limm, s12
; 
;   0.w0 = acclo + (limm.h0 * s12)
;        = 0x30 + (0x223d * 0x1ff)
;        = 0x4457f3 [4478963]
;   0.w1 = acchi + (limm.h1 * s12)
;        = 0x20 + (0xa8ca * 0x1ff)
;        = 0x150eb56 [22080342]
;
  prep_test_case
  set_accu64 0x20, 0x30
  vmac2hu 0, 0xa8ca223d, 511 ; 0x1ff
  check_reg64 r58, 22080342, 4478963

; Test case 17.2
; using simm12: negative
; 0 <- limm, s12
; 
;   0.w0 = acclo + (limm.h0 * s12)
;        = 0x63 + (0x223d * 0xf800)
;        = 0x212b1863
;   0.w1 = acchi + (limm.h1 * s12)
;        = 0x9 + (0xa8ca * 0xf800)
;        = 0xa383b009
;
  prep_test_case
  set_accu64 9, 99  ; 0x9, 0x63
  vmac2hu 0, 0xa8ca223d, -2048
  check_reg64 r58, 0xa383b009, 0x212b1863

;=== vmac2hu a,limm,limm ===

; vmac2hu a,limm,limm 00101110000111110111111110AAAAAA
;   r0.w0 = acclo + (limm.h0 * limm.h0)
;   r0.w1 = acchi + (limm.h1 * limm.h1)

; Test case 18
; r0 <- limm, limm
; 
;   r0.w0 = acclo + (limm.h0 * limm.h0)
;         = 0x30 + (0x223d * 0x223d)
;         = 0x49442b9
;   r0.w1 = acchi + (limm.h1 * limm.h1)
;         = 0x20 + (0xa8ca * 0xa8ca)
;         = 0x6f49bf84
;
  prep_test_case
  set_accu64 0x20, 0x30
  vmac2hu r0, 0xa8ca223d, 0xa8ca223d
  check_reg64 r0, 0x6f49bf84, 0x49442b9
  check_reg64 r58, 0x6f49bf84, 0x49442b9

;=== vmac2hu 0,limm,limm ===

; vmac2hu 0,limm,limm 00101110000111110111111110111110
;   0.w0 = acclo + (limm.h0 * limm.h0)
;   0.w1 = acchi + (limm.h1 * limm.h1)

; Test case 19
; 0 <- limm, limm
; 
;   0.w0 = acclo + (limm.h0 * limm.h0)
;        = 0x30 + (0x223d * 0x223d)
;        = 0x49442b9
;   0.w1 = acchi + (limm.h1 * limm.h1)
;        = 0x20 + (0xa8ca * 0xa8ca)
;        = 0x6f49bf84
;
  prep_test_case
  set_accu64 0x20, 0x30
  vmac2hu 0, 0xa8ca223d, 0xa8ca223d
  check_reg64 r58, 0x6f49bf84, 0x49442b9

;=== vmac2hu<.cc> 0,limm,limm ===

; vmac2hu<.cc> 0,limm,limm 001011101101111101111111100QQQQQ
;   if (cc) {
;     0.w0 = acclo + (limm.h0 * limm.h0)
;     0.w1 = acchi + (limm.h1 * limm.h1)
;   }

; Test case 20.1
; 0 <- limm, limm (cc)
; Testing when C=1 => vmac2hu is executed
; 
;   0.w0 = acclo + (limm.h0 * limm.h0)
;        = 0x30 + (0x223d * 0x223d)
;        = 0x49442b9
;   0.w1 = acchi + (limm.h1 * limm.h1)
;        = 0x20 + (0xa8ca * 0xa8ca)
;        = 0x6f49bf84
;
  prep_test_case
  set_accu64 0x20, 0x30
  set_reg64 r4, 0x80000000, 0x80000000
  add.f 0, r4, r4 
  vmac2hu.c 0, 0xa8ca223d, 0xa8ca223d
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r58, 0x6f49bf84, 0x49442b9

; Test case 20.2
; 0 <- limm, limm (cc)
; Testing when C=0 => vmac2hu is not executed
  prep_test_case
  set_accu64 0x20, 0x30
  vmac2hu.c 0, 0xa8ca223d, 0xa8ca223d
  check_reg64 r58, 0x20, 0x30

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
  print " vmac2hu: Unsigned multiplication and accumulation of two 16-bit vectors\n"
  end
