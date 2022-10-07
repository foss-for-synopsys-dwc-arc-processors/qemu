; check_vmac2h.S
;
; Tests for vmac2h: Signed multiplication and accumulation of two 16-bit vectors
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VMAC2H ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== vmac2h a,b,c ===

; vmac2h a,b,c 00101bbb000111100BBBCCCCCCAAAAAA
;   a.w0 = acclo + (b.h0 * c.h0);
;   a.w1 = acchi + (b.h1 * c.h1);

; Test case 1.1
; Same source and dest operands
; r0 <- r0, r0
;
;   r0.w0 = acclo + (r0.h0 * r0.h0)
;         = 0 + (0x1 * 0x1)
;         = 0x1
;   r0.w1 = acchi + (r0.h1 * r0.h1)
;         = 0 + (0x2 * 0x2)
;         = 0x4
;
  prep_test_case
  set_accu64 0x0, 0x0
  set_reg64 r0, 0x00040003, 0x00020001
  vmac2h r0, r0, r0
  check_reg64 r0, 4, 1
  check_reg64 r58, 4, 1

; Test case 1.2
; Negative numbers sign extension, and same source and dest operands.
; r0 <- r0, r1
;
;   r0.w0 = acclo + (r0.h0 * r1.h0)
;         = 0 + (0xffffffff * 0x1)
;         = 0xffffffff
;   r0.w1 = acchi + (r0.h1 * r1.h1)
;         = 0 + (0x1 * 0xffff8765)
;         = 0xffff8765
;
  prep_test_case
  set_accu64 0x0, 0x0
  set_reg64 r0, 0x11111111, 0x0001ffff
  set_reg64 r1, 0x22222222, 0x87650001
  vmac2h r0, r0, r1
  check_reg64 r0, 0xffff8765, 0xffffffff
  check_reg64 r58, 0xffff8765, 0xffffffff

; Test case 1.3
; Bigger numbers
; r0 <- r2, r3
;
;   r0.w0 = acclo + (r2.h0 * r3.h0)
;         = 0 + (0x1234 * 0x4321)
;         = 0x4c5f4b4
;   r0.w1 = acchi + (r2.h1 * r3.h1)
;         = 0 + (0x2 * 0xffff8765)
;         = 0x1_ffff0eca => 0xffff0eca
;
  prep_test_case
  set_accu64 0x0, 0x0
  set_reg64 r2, 0x11111111, 0x00021234
  set_reg64 r3, 0x22222222, 0x87654321
  vmac2h r0, r2, r3
  check_reg64 r0, 0xffff0eca, 0x4c5f4b4
  check_reg64 r58, 0xffff0eca, 0x4c5f4b4

; Test case 1.4
; Bigger numbers
; r0 <- r2, r3
;
;   r0.w0 = acclo + (r2.h0 * r3.h0)
;         = 0 + (0x5678 * 0x4321)
;         = 0x16ac8d78
;   r0.w1 = acchi + (r2.h1 * r3.h1)
;         = 0 + (0x1234 * 0xffff8765)
;         = 0x1233_f76c9a84 => 0xf76c9a84
;
  prep_test_case
  set_accu64 0x0, 0x0
  mov    r2, 0x12345678
  mov    r3, 0x87654321
  vmac2h r0, r2, r3
  check_reg64 r0, 0xf76c9a84, 0x16ac8d78
  check_reg64 r58, 0xf76c9a84, 0x16ac8d78

; Test case 1.5
; Use addition
; r4 <- r2, r3
;
;   r4.w0 = acclo + (r2.h0 * r3.h0)
;         = 0x0728c732 + (0x0032 * 0x03e9)
;         = 0x0728c732 + 0xc382
;         = 0x07298ab4
;   r4.w1 = acchi + (r2.h1 * r3.h1)
;         = 0xfc3afd6f + (0xffffd4fd * 0x10f6)
;         = 0xfc3afd6f + (0x10f5_fd267b1e)
;         = 0xfc3afd6f + 0xfd267b1e
;         = 0x1_f961788d => 0xf961788d
;
  prep_test_case
  set_accu64 0xfc3afd6f, 0x0728c732
  set_reg64 r2, 0x11111111, 0xd4fd0032
  set_reg64 r3, 0x22222222, 0x10f603e9
  vmac2h r4, r2, r3
  check_reg64 r4, 0xf961788d, 0x07298ab4
  check_reg64 r58, 0xf961788d, 0x07298ab4

; Test case 1.6
; Push the limits
; r0 <- r2, r3
;
;   r0.w0 = acclo + (r2.h0 * r3.h0)
;         = 0x80000000 + (0xffffffff * 0x0001)
;         = 0x80000000 + 0xffffffff
;         = 0x1_7fffffff => 0x7fffffff
;   r0.w1 = acchi + (r2.h1 * r3.h1)
;         = 0x7fffffff + (0xffffffff * 0xffffffff)
;         = 0x7fffffff + 0x00000001
;         = 0x80000000
;
  prep_test_case
  set_accu64 0x7fffffff, 0x80000000
  set_reg64 r2, 0x11111111, 0xffffffff
  set_reg64 r3, 0x22222222, 0xffff0001
  vmac2h r0, r2, r3
  check_reg64 r0, 0x80000000, 0x7fffffff
  check_reg64 r58, 0x80000000, 0x7fffffff

;=== vmac2h 0,b,c ===

; vmac2h 0,b,c 00101bbb000111100BBBCCCCCC111110
;   0.w0 = acclo + (b.h0 * c.h0)
;   0.w1 = acchi + (b.h1 * c.h1)

; Test case 2
; 0 <- r2, r3
; 
;   0.w0 = acclo + (r2.h0 * r3.h0)
;        = 1 + (0x1234 * 0x4321)
;        = 0x04c5f4b5
;   0.w1 = acchi + (r2.h1 * r3.h1)
;        = 2 + (0x0002 * 0xffff8765)
;        = 0x1_ffff0ecc => 0xffff0ecc
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r2, 0x11111111, 0x00021234
  set_reg64 r3, 0x22222222, 0x87654321
  vmac2h 0, r2, r3
  check_reg64 r58, 0xffff0ecc, 0x04c5f4b5

;=== vmac2h<.cc> b,b,c ===

; vmac2h<.cc> b,b,c 00101bbb110111100BBBCCCCCC0QQQQQ
;   if (cc) {
;     b.w0 = acclo + (b.h0 * c.h0)
;     b.w1 = acchi + (b.h1 * c.h1)
;   }

; Test case 3.1
; r2 <- r2, r3 (cc)
; Testing when C=1 => so vmac2h is executed
;
;   r2.w0 = acclo + (r2.h0 * r3.h0)
;         = 1 + (0x1234 * 0x4321)
;         = 0x04c5f4b5
;   r2.w1 = acchi + (r2.h1 * r3.h1)
;         = 2 + (0x0002 * 0xffff8765)
;         = 0x1_ffff0ecc => 0xffff0ecc
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r2, 0x11111111, 0x00021234
  set_reg64 r3, 0x22222222, 0x87654321
  set_reg64 r4, 0x80000000, 0x80000000
  add.f 0, r4, r4         ; ...C=1
  vmac2h.c r2, r2, r3
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r2, 0xffff0ecc, 0x04c5f4b5
  check_reg64 r58, 0xffff0ecc, 0x04c5f4b5

; Test case 3.2
; r2 <- r2, r3 (cc)
; Testing when C=0 => vmac2h is not executed
; 
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r2, 0x11111111, 0x00021234
  set_reg64 r3, 0x22222222, 0x87654321
  vmac2h.c r2, r2, r3
  check_reg64 r2, 0x11111111, 0x00021234
  check_reg64 r58, 0x2, 0x1

;=== vmac2h a,b,u6 ===

; vmac2h a,b,u6 00101bbb010111100BBBuuuuuuAAAAAA
;   a.w0 = acclo + (b.h0 * u6)
;   a.w1 = acchi + (b.h1 * u6)

; Test case 4
; r0 <- r1, u6
; 
;   r0.w0 = acclo + (r1.h0 * u6)
;         = 1 + (0x2 * 0x3f)
;         = 0x7f
;   r0.w1 = acchi + (r1.h1 * u6)
;         = 2 + (0x3 * 0x3f)
;         = 0xbf
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r1, 0x11111111, 0x00030002
  vmac2h r0, r1, 0x3f
  check_reg64 r0, 0xbf, 0x7f
  check_reg64 r58, 0xbf, 0x7f

;=== vmac2h 0,b,u6 ===

; vmac2h 0,b,u6 00101bbb010111100BBBuuuuuu111110
;   0.w0 = acclo + (b.h0 * u6)
;   0.w1 = acchi + (b.h1 * u6)

; Test case 5
; 0 <- r1, u6
; 
;   0.w0 = acclo + (r1.h0 * u6)
;        = 1 + (0x2 * 0x3f)
;        = 0x7f
;   0.w1 = acchi + (r1.h1 * u6)
;        = 2 + (0x3 * 0x3f)
;        = 0xbf
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r1, 0x11111111, 0x00030002
  vmac2h 0, r1, 0x3f
  check_reg64 r58, 0xbf, 0x7f

;=== vmac2h<.cc> b,b,u6 ===

; vmac2h<.cc> b,b,u6 00101bbb110111100BBBuuuuuu1QQQQQ
;   if (cc) {
;     b.w0 = acclo + (b.h0 * u6)
;     b.w1 = acchi + (b.h1 * u6)
;   }

; Test case 6.1
; r1 <- r1, u6 (cc)
; Testing when C=1 => vmac2h is executed
;
;   r1.w0 = acclo + (r1.h0 * u6)
;         = 1 + (0x2 * 0x3f)
;         = 0x7f
;   r1.w1 = acchi + (r1.h1 * u6)
;         = 2 + (0x3 * 0x3f)
;         = 0xbf
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r1, 0x11111111, 0x00030002
  set_reg64 r2, 0x80000000, 0x80000000
  add.f 0, r2, r2         ; ...C=1
  vmac2h.c r1, r1, 0x3f
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r1, 0xbf, 0x7f
  check_reg64 r58, 0xbf, 0x7f

; Test case 6.2
; r1 <- r1, u6 (cc)
; Testing when C=0 => vmac2h is not executed
; 
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r1, 0x11111111, 0x00030002
  vmac2h.c r1, r1, 0x3f
  check_reg64 r1, 0x11111111, 0x00030002
  check_reg64 r58, 0x2, 0x1

;=== vmac2h b,b,s12 ===

; vmac2h b,b,s12 00101bbb100111100BBBssssssSSSSSS
;   b.w0 = acclo + (b.h0 * s12)
;   b.w1 = acchi + (b.h1 * s12)

; Test case 7
; r1 <- r1, s12
; 
;   r1.w0 = acclo + (r1.h0 * u6)
;         = 1 + (0x2 * -2048)
;         = 0x1_fffff001 => 0xfffff001
;   r1.w1 = acchi + (r1.h1 * u6)
;         = 2 + (0x3 * -2048)
;         = 0x2_ffffe802 => 0xffffe802
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r1, 0x11111111, 0x00030002
  vmac2h r1, r1, -2048
  check_reg64 r1, 0xffffe802, 0xfffff001
  check_reg64 r58, 0xffffe802, 0xfffff001

;=== vmac2h a,limm,c ===

; vmac2h a,limm,c 00101110000111100111CCCCCCAAAAAA
;   a.w0 = acclo + (limm.ho * c.h0)
;   a.w1 = acchi + (limm.h1 * c.h1)

; Test case 8
; r0 <- limm, r1
; 
;   r0.w0 = acclo + (limm.h0 * r1.h0)
;         = 1 + (0x5678 * 0x2)
;         = 0xacf1
;   r0.w1 = acchi + (limm.h1 * r1.h1)
;         = 2 + (0x1234 * 0x3)
;         = 0x369e
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r1, 0x11111111, 0x00030002
  vmac2h r0, 0x12345678, r1
  check_reg64 r0, 0x369e, 0xacf1
  check_reg64 r58, 0x369e, 0xacf1

;=== vmac2h a,b,limm ===

; vmac2h a,b,limm 00101bbb000111100BBB111110AAAAAA
;   a.w0 = acclo + (b.h0 * limm.ho)
;   a.w1 = acchi + (b.h1 * limm.h1)

; Test case 9
; r0 <- r1, limm
; 
;   r0.w0 = acclo + (r1.h0 * limm.h0)
;         = 1 + (0x2 * 0x5678)
;         = 0xacf1
;   r0.w1 = acchi + (r1.h1 * limm.h1)
;         = 2 + (0x3 * 0x1234)
;         = 0x369e
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r1, 0x11111111, 0x00030002
  vmac2h r0, r1, 0x12345678
  check_reg64 r0, 0x369e, 0xacf1
  check_reg64 r58, 0x369e, 0xacf1

;=== vmac2h 0,limm,c ===

; vmac2h 0,limm,c 00101110000111100111CCCCCC111110
;   0.w0 = acclo + (limm.ho * c.h0)
;   0.w1 = acchi + (limm.h1 * c.h1)

; Test case 10
; 0 <- limm, r1
; 
;   0.w0 = acclo + (limm.h0 * r1.h0)
;        = 1 + (0x5678 * 0x2)
;        = 0xacf1
;   0.w1 = acchi + (limm.h1 * r1.h1)
;        = 2 + (0x1234 * 0x3)
;        = 0x369e
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r1, 0x11111111, 0x00030002
  vmac2h 0, 0x12345678, r1
  check_reg64 r58, 0x369e, 0xacf1

;=== vmac2h 0,b,limm ===

; vmac2h 0,b,limm 00101bbb000111100BBB111110111110
;   0.w0 = acclo + (b.h0 * limm.ho)
;   0.w1 = acchi + (b.h1 * limm.h1)

; Test case 11
; 0 <- r1, limm
; 
;   0.w0 = acclo + (r1.h0 * limm.h0)
;        = 1 + (0x2 * 0x5678)
;        = 0xacf1
;   0.w1 = acchi + (r1.h1 * limm.h1)
;        = 2 + (0x3 * 0x1234)
;        = 0x369e
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r1, 0x11111111, 0x00030002
  vmac2h 0, r1, 0x12345678
  check_reg64 r58, 0x369e, 0xacf1

;=== vmac2h<.cc> b,b,limm ===

; vmac2h<.cc> b,b,limm 00101bbb110111100BBB1111100QQQQQ
;   if (cc) {
;     b.w0 = acclo + (b.h0 * limm.h0)
;     b.w1 = acchi + (b.h1 * limm.h1)
;   }

; Test case 12.1
; r1 <- r1, limm (cc)
; Testing when C=1 => vmac2h is executed
;
;   r1.w0 = acclo + (r1.h0 * limm.h0)
;         = 1 + (0x2 * 0x5678)
;         = 0xacf1
;   r1.w1 = acchi + (r1.h1 * limm.h1)
;         = 2 + (0x3 * 0x1234)
;         = 0x369e
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r1, 0x11111111, 0x00030002
  set_reg64 r2, 0x80000000, 0x80000000
  add.f 0, r2, r2         ; ...C=1
  vmac2h.c r1, r1, 0x12345678
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r1, 0x369e, 0xacf1
  check_reg64 r58, 0x369e, 0xacf1

; Test case 12.2
; r1 <- r1, limm (cc)
; Testing when C=0 => vmac2h is not executed
; 
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r1, 0x11111111, 0x00030002
  vmac2h.c r1, r1, 0x12345678
  check_reg64 r1, 0x11111111, 0x00030002
  check_reg64 r58, 0x2, 0x1

;=== vmac2h<.cc> 0,limm,c ===

; vmac2h<.cc> 0,limm,c 00101110110111100111CCCCCC0QQQQQ
;   if (cc) {
;     0.w0 = acclo + (limm.h0 * b.h0)
;     0.w1 = acchi + (limm.h1 * b.h1)
;   }

; Test case 13.1
; 0 <- limm, r1 (cc)
; Testing when C=1 => vmac2h is executed
;
;   0.w0 = acclo + (limm.h0 * r1.h0)
;        = 1 + (0x5678 * 0x2)
;        = 0xacf1
;   0.w1 = acchi + (limm.h1 * r1.h1)
;        = 2 + (0x1234 * 0x3)
;        = 0x369e
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r1, 0x11111111, 0x00030002
  set_reg64 r2, 0x80000000, 0x80000000
  add.f 0, r2, r2         ; ...C=1
  vmac2h.c 0, 0x12345678, r1 
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r58, 0x369e, 0xacf1

; Test case 13.2
; 0 <- limm, r1 (cc)
; Testing when C=0 => vmac2h is not executed
; 
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r1, 0x11111111, 0x00030002
  vmac2h.c 0, 0x12345678, r1
  check_reg64 r58, 0x2, 0x1

;=== vmac2h a,limm,u6 ===

; vmac2h a,limm,u6 00101110010111100111uuuuuuAAAAAA
;   a.w0 = acclo + (limm.h0 * u6)
;   a.w1 = acchi + (limm.h1 * u6)

; Test case 14
; r0 <- limm, u6
; 
;   r0.w0 = acclo + (limm.h0 * u6)
;         = 1 + (0x5678 * 0x3f)
;         = 0x154789
;   r0.w1 = acchi + (limm.h1 * u6)
;         = 2 + (0x1234 * 0x3f)
;         = 0x47ace
;
  prep_test_case
  set_accu64 0x2, 0x1
  vmac2h r0, 0x12345678, 0x3f
  check_reg64 r0, 0x47ace, 0x154789
  check_reg64 r58, 0x47ace, 0x154789

;=== vmac2h 0,limm,u6 ===

; vmac2h 0,limm,u6 00101110010111100111uuuuuu111110
;   0.w0 = acclo + (limm.h0 * u6)
;   0.w1 = acchi + (limm.h1 * u6)

; Test case 15
; 0 <- limm, u6
; 
;   0.w0 = acclo + (limm.h0 * u6)
;        = 1 + (0x5678 * 0x3f)
;        = 0x154789
;   0.w1 = acchi + (limm.h1 * u6)
;        = 2 + (0x1234 * 0x3f)
;        = 0x47ace
;
  prep_test_case
  set_accu64 0x2, 0x1
  vmac2h 0, 0x12345678, 0x3f
  check_reg64 r58, 0x47ace, 0x154789

;=== vmac2h<.cc> 0,limm,u6 ===

; vmac2h<.cc> 0,limm,u6 00101110110111100111uuuuuu1QQQQQ
;   if (cc) {
;     0.w0 = acclo + (limm.h0 * u6)
;     0.w1 = acchi + (limm.h1 * u6)
;   }

; Test case 16.1
; 0 <- limm, u6 (cc)
; Testing when C=1 => vmac2h is executed
;
;   0.w0 = acclo + (limm.h0 * u6)
;        = 1 + (0x5678 * 0x3f)
;        = 0x154789
;   0.w1 = acchi + (limm.h1 * u6)
;        = 2 + (0x1234 * 0x3f)
;        = 0x47ace
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r2, 0x80000000, 0x80000000
  add.f 0, r2, r2         ; ...C=1
  vmac2h.c 0, 0x12345678, 0x3f 
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r58, 0x47ace, 0x154789

; Test case 16.2
; 0 <- limm, u6 (cc)
; Testing when C=0 => vmac2h is not executed
; 
  prep_test_case
  set_accu64 0x2, 0x1
  vmac2h.c 0, 0x12345678, 0x3f 
  check_reg64 r58, 0x2, 0x1

;=== vmac2h 0,limm,s12 ===

; vmac2h 0,limm,s12 00101110100111100111ssssssSSSSSS
;   0.w0 = acclo + (limm.h0 * s12)
;   0.w1 = acchi + (limm.h1 * s12)

; Test case 17
; 0 <- limm, s12
; 
;   0.w0 = acclo + (limm.h0 * s12)
;        = 1 + (0x5678 * -2048)
;        = 0x5677_fd4c4001 => 0xfd4c4001
;   0.w1 = acchi + (limm.h1 * s12)
;        = 2 + (0x1234 * -2048)
;        = 0x1233_ff6e6002 => 0xff6e6002
;
  prep_test_case
  set_accu64 0x2, 0x1
  vmac2h 0, 0x12345678, -2048
  check_reg64 r58, 0xff6e6002, 0xfd4c4001

;=== vmac2h a,limm,limm ===

; vmac2h a,limm,limm 00101110000111100111111110AAAAAA
;   r0.w0 = acclo + (limm.h0 * limm.h0)
;   r0.w1 = acchi + (limm.h1 * limm.h1)

; Test case 18
; r0 <- limm, limm
; 
;   r0.w0 = acclo + (limm.h0 * limm.h0)
;         = 1 + (0x5678 * 0x5678)
;         = 0x1d34d841
;   r0.w1 = acchi + (limm.h1 * limm.h1)
;         = 2 + (0x1234 * 0x1234)
;         = 0x014b5a92
;
  prep_test_case
  set_accu64 0x2, 0x1
  vmac2h r0, 0x12345678, 0x12345678
  check_reg64 r0, 0x014b5a92, 0x1d34d841
  check_reg64 r58, 0x014b5a92, 0x1d34d841
  
;=== vmac2h 0,limm,limm ===

; vmac2h 0,limm,limm 00101110000111100111111110111110
;   0.w0 = acclo + (limm.h0 * limm.h0)
;   0.w1 = acchi + (limm.h1 * limm.h1)

; Test case 19
; 0 <- limm, limm
; 
;   0.w0 = acclo + (limm.h0 * limm.h0)
;        = 1 + (0x5678 * 0x5678)
;        = 0x1d34d841
;   0.w1 = acchi + (limm.h1 * limm.h1)
;        = 2 + (0x1234 * 0x1234)
;        = 0x014b5a92
;
  prep_test_case
  set_accu64 0x2, 0x1
  vmac2h 0, 0x12345678, 0x12345678
  check_reg64 r58, 0x014b5a92, 0x1d34d841

;=== vmac2h<.cc> 0,limm,limm ===

; vmac2h<.cc> 0,limm,limm 001011101101111001111111100QQQQQ
;   if (cc) {
;     0.w0 = acclo + (limm.h0 * limm.h0)
;     0.w1 = acchi + (limm.h1 * limm.h1)
;   }

; Test case 20.1
; 0 <- limm, limm (cc)
; Testing when C=1 => vmac2h is executed
;
;   0.w0 = acclo + (limm.h0 * limm.h0)
;        = 1 + (0x5678 * 0x5678)
;        = 0x1d34d841
;   0.w1 = acchi + (limm.h1 * limm.h1)
;        = 2 + (0x1234 * 0x1234)
;        = 0x014b5a92
;
  prep_test_case
  set_accu64 0x2, 0x1
  set_reg64 r2, 0x80000000, 0x80000000
  add.f 0, r2, r2         ; ...C=1
  vmac2h.c 0, 0x12345678, 0x12345678
  add.f 0, 0, 1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
  check_reg64 r58, 0x014b5a92, 0x1d34d841

; Test case 20.2
; 0 <- limm, limm (cc)
; Testing when C=0 => vmac2h is not executed
; 
  prep_test_case
  set_accu64 0x2, 0x1
  vmac2h.c 0, 0x12345678, 0x12345678
  check_reg64 r58, 0x2, 0x1

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
  print " vmac2h: Signed multiplication and accumulation of two 16-bit vectors\n"
  end
