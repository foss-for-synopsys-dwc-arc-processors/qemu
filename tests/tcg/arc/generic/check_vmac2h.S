; check_vmac2h.S
;
; Tests for vmac2h: vmac2h vmac2hu
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

; Set accumulator to "high:low"
.macro set_accu high, low
  mov   r58, \low
  mov   r59, \high
.endm

; Compare "reg_hi:reg_lo" against "hi:lo"
.macro compare_64bit reg_hi, reg_lo, hi, lo
  cmp   \reg_lo, \lo
  bne   @fail
  cmp   \reg_hi, \hi
  bne   @fail
.endm

;;;;;;;;;;;;;;;;;;;;;;;;; Exception related code ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; parameters that an Illegal Instruction exception may set.
.data
  .align 4
ecr_ref  : .word ILLEGAL_INSTRUCTION
addr_ref : .word 0x0                  ; for both eret and efa
cont_addr: .word 0x0

; exception: Illegal Instruction
; regs used: r11, r12
;
; A parameterized Illegal Instruction exception that checks the followings:
; ecr  == Illega instruction
; efa  == efa_ref
; eret == eret_ref
; If everything passes, it will jump to 'cont_addr' parameter. The parameters
; must be set beforehand using 'set_except_params' macro.  This requires
; ivt.S file to be compiled and linked.
.text
  .align 4
  .global instruction_error
  .type instruction_error, @function
instruction_error:
  ld   r11, [ecr_ref]
  lr   r12, [ecr]
  cmp  r12, r11
  bne  @fail
  ld   r11, [addr_ref]
  lr   r12, [eret]
  cmp  r12, r11
  bne  @fail
  lr   r12, [efa]
  cmp  r12, r11
  bne  @fail
  ; Success: continuing
  ld   r11, [cont_addr]
  sr   r11, [eret]
  rtie

; macro:      set_except_params
; regs used:  r11
;
; This macro writes the provided parameters to a temporary place holder
; that later will be used by exception above to verify as reference.
.macro set_except_params addr, continue
  mov  r11, \addr
  st   r11, [addr_ref]
  mov  r11, \continue
  st   r11, [cont_addr]
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; vmac2h ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test case 1
; Same source and dest operands
; r0 <- r0, r0
;   r0 = acclo + (r0.h0 * r0.h0);
;   r1 = acchi + (r0.h1 * r0.h1);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r0.h0 * r0.h0)
;      = 0 + (0x0001 * 0x0001)
;      = 0x0001
;   r1 = acchi + (r0.h1 * r0.h1)
;      = 0 + (0x0002 * 0x0002)
;      = 0x0004
;
  prep_test_case
  set_accu 0x0, 0x0
  mov    r0, 0x00020001
  vmac2h r0, r0, r0
  compare_64bit  r1,  r0, 0x04, 0x01
  compare_64bit r59, r58, 0x04, 0x01

; Test case 2
; Different source and dest operands
; r2 <- r0, r1
;   r2 = acclo + (r0.h0 * r1.h0);
;   r3 = acchi + (r0.h1 * r1.h1);
;   acclo = r2;
;   acchi = r3;
;
;   r2 = acclo + (r0.h0 * r1.h0)
;      = 0 + (0x0001 * 0x0003)
;      = 0x0003
;   r3 = acchi + (r0.h1 * r1.h1)
;      = 0 + (0x0002 * 0x0004)
;      = 0x0008
;
  prep_test_case
  set_accu 0x0, 0x0
  mov    r0, 0x00020001
  mov    r1, 0x00040003
  vmac2h r2, r0, r1
  compare_64bit  r3,  r2, 0x08, 0x03
  compare_64bit r59, r58, 0x08, 0x03

; Test case 3
; Negative numbers sign extension, and same source and dest operands.
; r0 <- r0, r1
;   r0 = acclo + (r0.h0 * r1.h0);
;   r1 = acchi + (r0.h1 * r1.h1);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r0.h0 * r1.h0)
;      = 0 + (0xffff_ffff * 0x0001) <= sign extension
;      = 0xffffffff
;   r1 = acchi + (r0.h1 * r1.h1)
;      = 0 + (0x0001 * 0xffff_8765) <= sign extension
;      = 0xffff8765
;
  prep_test_case
  set_accu 0x0, 0x0
  mov    r0, 0x0001ffff
  mov    r1, 0x87650001
  vmac2h r0, r0, r1
  compare_64bit  r1,  r0, 0xffff8765, 0xffffffff
  compare_64bit r59, r58, 0xffff8765, 0xffffffff

; Test case 4
; Bigger numbers
; r0 <- r2, r3
;   r0 = acclo + (r2.h0 * r3.h0);
;   r1 = acchi + (r2.h1 * r3.h1);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r2.h0 * r3.h0)
;      = 0 + (0x1234 * 0x4321)
;      = 0x4c5f4b4
;   r1 = acchi + (r2.h1 * r3.h1)
;      = 0 + (0x0002 * 0xffff_8765) <= sign extension
;      = 0x1_ffff0eca => 0xffff0eca
;
  prep_test_case
  set_accu 0x0, 0x0
  mov    r2, 0x00021234
  mov    r3, 0x87654321
  vmac2h r0, r2, r3
  compare_64bit  r1,  r0, 0xffff0eca, 0x4c5f4b4
  compare_64bit r59, r58, 0xffff0eca, 0x4c5f4b4

; Test case 5
; Bigger numbers
; r0 <- r2, r3
;   r0 = acclo + (r2.h0 * r3.h0);
;   r1 = acchi + (r2.h1 * r3.h1);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r2.h0 * r3.h0)
;      = 0 + (0x5678 * 0x4321)
;      = 0x16ac8d78
;   r1 = acchi + (r2.h1 * r3.h1)
;      = 0 + (0x1234 * 0xffff_8765) <= sign extension
;      = 0x1233_f76c9a84 => 0xf76c9a84
;
  prep_test_case
  set_accu 0x0, 0x0
  mov    r2, 0x12345678
  mov    r3, 0x87654321
  vmac2h r0, r2, r3
  compare_64bit  r1,  r0, 0xf76c9a84, 0x16ac8d78
  compare_64bit r59, r58, 0xf76c9a84, 0x16ac8d78

; Test case 6
; Use addition
; r4 <- r2, r3
;   r4 = acclo + (r2.h0 * r3.h0);
;   r5 = acchi + (r2.h1 * r3.h1);
;   acclo = r4;
;   acchi = r5;
;
;   r4 = acclo + (r2.h0 * r3.h0)
;      = 0x728c732 + (0x0032 * 0x03e9)
;      = 0x7298ab4 (120162996)
;   r5 = acchi + (r2.h1 * r3.h1)
;      = 0xfc3afd6f + (0xffff_d4fd * 0x10f6) <= sign extension
;      = 0x10f6_f961788d => 0xf961788d (-111052659)
;
  prep_test_case
  set_accu -63242897, 120112946 ; -63242897=0xfc3afd6f, 120112946=0x728c732
  mov    r2, 0xd4fd0032        ; -11011=0xd4fd, 50=0x32
  mov    r3, 0x10f603e9        ; 4342=0x10f6, 1001=0x3e9
  vmac2h r4, r2, r3
  compare_64bit  r5,  r4, -111052659, 120162996
  compare_64bit r59, r58, -111052659, 120162996

; Test case 7
; Push the limits
; r0 <- r2, r3
;   r0 = acclo + (r2.h0 * r3.h0);
;   r1 = acchi + (r2.h1 * r3.h1);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r2.h0 * r3.h0)
;      = 0x80000000 + (0xffff_ffff * 0x0001) <= sign extension
;      = 0x1_7fffffff => 0x7fffffff
;   r1 = acchi + (r2.h1 * r3.h1)
;      = 0x7fffffff + (0xffff_ffff * 0xffff_ffff) <= sign extension
;      = 0xfffffffe_80000000 => 0x80000000
;
  prep_test_case
  set_accu 0x7fffffff, 0x80000000
  mov    r2, 0xffffffff
  mov    r3, 0xffff0001
  vmac2h r0, r2, r3
  compare_64bit  r1,  r0, 0x80000000, 0x7fffffff
  compare_64bit r59, r58, 0x80000000, 0x7fffffff

; Test case 8
; 0 as destination
; 0 <- r2, r3
;   0 = acclo + (r2.h0 * r3.h0);
;   0 = acchi + (r2.h1 * r3.h1);
;   acclo = acclo + (r2.h0 * r3.h0);
;   acchi = acchi + (r2.h1 * r3.h1);
;
;   acclo = acclo + (r2.h0 * r3.h0)
;         = 0x1 + (0x1234 * 0x4321)
;         = 0x4c5f4b5
;   acchi = acchi + (r2.h1 * r3.h1)
;         = 0x2 + (0x0002 * 0xffff_8765) <= sign extension
;         = 0x1_ffff0ecc => 0xffff0ecc
;
  prep_test_case
  set_accu 0x2, 0x1
  mov    r2, 0x00021234
  mov    r3, 0x87654321
  vmac2h 0, r2, r3
  compare_64bit r59, r58, 0xffff0ecc, 0x4c5f4b5

; Test case 9
; using u6
; r0 <- r1, u6
;   r0 = acclo + (r1.h0 * u6);
;   r1 = acchi + (r1.h1 * u6);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r1.h0 * u6)
;      = 0x5 + (0x0001 * 0x3)
;      = 0x8
;   r1 = acchi + (r1.h1 * u6)
;      = 0x4 + (0xffff_ffff * 0x3) <= sign extension
;      = 0x3_00000001 => 0x00000001
;
  prep_test_case
  set_accu 0x4, 0x5
  mov    r1, 0xffff0001
  vmac2h r0, r1, 3
  compare_64bit  r1,  r0, 1, 8
  compare_64bit r59, r58, 1, 8

; Test case 10
; using simm12: positive
; r0 <- r0, s12
;   r0 = acclo + (r0.h0 * s12);
;   r1 = acchi + (r0.h1 * s12);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r0.h0 * s12)
;      = 0x30 + (0xffff_fec8 * 0x75) <= sign extension
;      = 0x74_ffff7198 => 0xffff7198 (-36456)
;   r1 = acchi + (r0.h1 * s12)
;      = 0x20 + (0x019e * 0x75)
;      = 0xbd56 (48470)
;
  prep_test_case
  set_accu 0x20, 0x30
  mov    r0, 0x19efec8   ; 414=0x19e, -312=0xfec8
  vmac2h r0, r0, 117     ; 117=0x75
  compare_64bit  r1,  r0, 48470, -36456
  compare_64bit r59, r58, 48470, -36456

; Test case 11
; using simm12: negative
; r2 <- r2, s12
;   r2 = acclo + (r2.h0 * s12);
;   r3 = acchi + (r2.h1 * s12);
;   acclo = r2;
;   acchi = r3;
;
;   r2 = acclo + (r2.h0 * s12)
;      = 0x63 + (0x0002 * 0xfffffffe) <= sign extension
;      = 0x2_0000005f => 0x0000005f (95)
;   r3 = acchi + (r2.h1 * s12)
;      = 0x9 + (0x0001 * 0xfffffffe) <= sign extension
;      = 0x1_00000007 => 0x00000007 (7)
;
  prep_test_case
  set_accu 9, 99      ; 9=0x9, 99=0x63
  mov    r2, 0x00010002 
  vmac2h r2, r2, -2
  compare_64bit  r3,  r2, 7, 95
  compare_64bit r59, r58, 7, 95

; Test case 12
; using limm: -301 -> 0xffff_fed3 -> (0xffff, 0xfed3)
; r2 <- r3, limm
;   r2 = acclo + (r3.h0 * limm.h0);
;   r3 = acchi + (r3.h1 * limm.h1);
;   acclo = r2;
;   acchi = r3;
;
;   r2 = acclo + (r3.h0 * limm.h0)
;      = 0xfffffba9 + (0xffff_e57b * 0xffff_fed3) <= sign extension
;      = 0xffffe44f_001f2a0a => 0x001f2a0a (2042378)
;   r3 = acchi + (r3.h1 * limm.h1)
;      = 0x10e1 + (0x3039 * 0xffff_ffff) <= sign extension
;      = 0x3038_FFFFE0A8 => 0xffffe0a8 (-8024)
;
  prep_test_case
  set_accu 4321, -1111    ; 4321=0x10e1, -1111=0xfffffba9
  mov    r3, 0x3039e57b   ; 12345=0x3039, -6789=0xe57b
  vmac2h r2, r3, -301     ; -301=0xfffffed3
  compare_64bit  r3,  r2, -8024, 2042378
  compare_64bit r59, r58, -8024, 2042378 ;1F2A0A

; Test case 13
; r0 <- limm * u6
;   r0 = acclo + (limm.h0 * u6);
;   r1 = acchi + (limm.h1 * u6);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (limm.h0 * u6)
;      = 0xdeadbeef + (0xffff_fb2e * 0x2a) <= sign extension
;      = 0x2a_deacf47b => 0xdeacf47b
;   r1 = acchi + (limm.h1 * u6)
;      = 0xecc83502 + (0x10e1 * 0x2a) <= sign extension
;      = 0xeccaf9ec
;
  prep_test_case
  set_accu -0x1337cafe, 0xdeadbeef
  vmac2h r0, 0x10e1fb2e, 42    ; 42=0x2a
  compare_64bit  r1,  r0, 0xeccaf9ec, 0xdeacf47b
  compare_64bit r59, r58, 0xeccaf9ec, 0xdeacf47b

; Test case 14
; CC: true
; r2 <- r2, r3
;   r2 = acclo + (r2.h0 * r3.h0);
;   r3 = acchi + (r2.h1 * r3.h1);
;   acclo = r2;
;   acchi = r3;
;
;   r2 = acclo + (r2.h0 * r3.h0)
;      = 0x2 + (0x5 * 0x6) <= sign extension
;      = 0x20 (32)
;   r3 = acchi + (r2.h1 * r3.h1)
;      = 0x1 + (0x0 * 0x0) <= sign extension
;      = 0x1 (1)
;s
  prep_test_case
  set_accu 1, 2
  mov      r2, 5
  mov      r3, 6
  add.f    0, 0, 0       ; Z=1
  vmac2h.z r2, r2, r3
  compare_64bit  r3,  r2, 1, 32
  compare_64bit r59, r58, 1, 32

; Test case 15
; CC: false
; Instruction is not executed
  prep_test_case
  set_accu 1, 2
  mov      r2, 5
  mov      r3, 6
  vmac2h.z r2, r2, r3    ; Z=0 because of PREP_TEST_CASE
  compare_64bit  r3,  r2, 6, 5
  compare_64bit r59, r58, 1, 2

;; Test case 16
;; Odd-numbered reg as destination -> Raise illegal exception
;  prep_test_case
;  set_accu 13, 14
;  mov      r0, 0x13
;  mov      r1, r0
;  set_except_params @test_13_illegal, @test_13_check_no_commit
;test_13_illegal:
;  .word  0x01012a1e  ; vmac2h r1, r2, r4
;                     ; the stream should read as 0x2a1e_0101
;                     ; mem:            1e 2a  01 01
;                     ; middle endian:  2a1e   0101
;  b      @fail
;test_13_check_no_commit:
;  cmp      r0, 0x13
;  bne      @fail
;  cmp      r0, r1
;  bne      @fail
;  cmp      r59, 13
;  bne      @fail
;  cmp      r58, 14
;  bne      @fail
;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; vmac2hu ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Test case 17
; Same source and dest operands
; r0 <- r0, r0
;   r0 = acclo + (r0.h0 * r0.h0);
;   r1 = acchi + (r0.h1 * r0.h1);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r0.h0 * r0.h0)
;      = 0 + (0x0001 * 0x0001)
;      = 0x0001
;   r1 = acchi + (r0.h1 * r0.h1)
;      = 0 + (0x0002 * 0x0002)
;      = 0x0004
;
  prep_test_case
  set_accu 0x0, 0x0
  mov     r0, 0x00020001
  vmac2hu r0, r0, r0
  compare_64bit  r1,  r0, 0x04, 0x01
  compare_64bit r59, r58, 0x04, 0x01

; Test case 18
; Different source and dest operands
; r2 <- r0, r1
;   r2 = acclo + (r0.h0 * r1.h0);
;   r3 = acchi + (r0.h1 * r1.h1);
;   acclo = r2;
;   acchi = r3;
;
;   r2 = acclo + (r0.h0 * r1.h0)
;      = 0 + (0x0001 * 0x0003)
;      = 0x0003
;   r3 = acchi + (r0.h1 * r1.h1)
;      = 0 + (0x0002 * 0x0004)
;      = 0x0008
;
  prep_test_case
  set_accu 0x0, 0x0
  mov     r0, 0x00020001
  mov     r1, 0x00040003
  vmac2hu r2, r0, r1
  compare_64bit  r3,  r2, 0x08, 0x03
  compare_64bit r59, r58, 0x08, 0x03

; Test case 19
; Non-zero accumulator and same source and dest operands.
; r0 <- r0, r1
;   r0 = acclo + (r0.h0 * r1.h0);
;   r1 = acchi + (r0.h1 * r1.h1);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r0.h0 * r1.h0)
;      = 0x15 + (0x0009 * 0x0009)
;      = 0x66 (102)
;   r1 = acchi + (r0.h1 * r1.h1)
;      = 0x14 + (0x0011 * 0x001c)
;      = 0x1f0 (496)
;
  prep_test_case
  set_accu 20, 21         ; 20=0x14, 21=0x15
  mov     r0, 0x00110009
  mov     r1, 0x001c0009
  vmac2hu r0, r0, r1
  compare_64bit  r1,  r0, 496, 102
  compare_64bit r59, r58, 496, 102

; Test case 20
; Bigger numbers.
; r0 <- r2, r3
;   r0 = acclo + (r2.h0 * r3.h0);
;   r1 = acchi + (r2.h1 * r3.h1);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r2.h0 * r3.h0)
;      = 0x9abcdef0 + (0xba98 * 0x3210)
;      = 0xbf3a3870
;   r1 = acchi + (r2.h1 * r3.h1)
;      = 0x12345678 + (0xfedc * 0x7654)
;      = 0x88015ea8
;
  prep_test_case
  set_accu 0x12345678, 0x9abcdef0
  mov     r2, 0xfedcba98
  mov     r3, 0x76543210
  vmac2hu r0, r2, r3
  compare_64bit  r1,  r0, 0x88015ea8, 0xbf3a3870
  compare_64bit r59, r58, 0x88015ea8, 0xbf3a3870

; Test case 21
; Bigger numbers: enter the would-be-negative range of numbers.
; The addition should also cause an overflow (no flag is ever set).
; r0 <- r2, r3
;   r0 = acclo + (r2.h0 * r3.h0);
;   r1 = acchi + (r2.h1 * r3.h1);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r2.h0 * r3.h0)
;      = 0x80000000 + (0xbbbb * 0xbbbb)
;      = 0x1_09aaba99 => 0x09aaba99
;   r1 = acchi + (r2.h1 * r3.h1)
;      = 0x44556677 + (0xffee * 0xccdd)
;      = 0x1_1123feed => 0x1123feed
;
  prep_test_case
  set_accu 0x44556677, 0x80000000
  mov     r2, 0xffeebbbb
  mov     r3, 0xccddbbbb
  vmac2hu r0, r2, r3
  compare_64bit  r1,  r0, 0x1123feed, 0x9aaba99
  compare_64bit r59, r58, 0x1123feed, 0x9aaba99

; Test case 22
; Pushing the limits.
; r0 <- r0, r0
;   r0 = acclo + (r0.h0 * r0.h0);
;   r1 = acchi + (r0.h1 * r0.h1);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r0.h0 * r0.h0)
;      = 0xffffffff + (0x0001 * 0x0001)
;      = 0x1_00000000 => 0x00000000
;   r1 = acchi + (r0.h1 * r0.h1)
;      = 0x1ffff + (0xffff * 0xffff)
;      = 0x1_00000000 => 0x00000000
;
  prep_test_case
  set_accu 0x1ffff, 0xffffffff
  mov      r0, 0xffff0001
  vmac2hu  r0, r0, r0
  compare_64bit  r1,  r0, 0, 0
  compare_64bit r59, r58, 0, 0

; Test case 23
; 0 as destination
; 0 <- r2, r3
;   0 = acclo + (r2.h0 * r3.h0);
;   0 = acchi + (r2.h1 * r3.h1);
;   acclo = acclo + (r2.h0 * r3.h0);
;   acchi = acchi + (r2.h1 * r3.h1);
;
;   acclo = acclo + (r2.h0 * r3.h0)
;         = 0x1 + (0x1234 * 0x4321)
;         = 0x4c5f4b5
;   acchi = acchi + (r2.h1 * r3.h1)
;         = 0x2 + (0x0002 * 0x8765)
;         = 0x10ecc
;
  prep_test_case
  set_accu 0x2, 0x1
  mov     r2, 0x00021234
  mov     r3, 0x87654321
  vmac2hu 0, r2, r3
  compare_64bit r59, r58, 0x10ecc, 0x4c5f4b5

; Test case 24
; using u6
; r0 <- r1, u6
;   r0 = acclo + (r1.h0 * u6);
;   r1 = acchi + (r1.h1 * u6);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r1.h0 * u6)
;      = 0x5 + (0x03e8 * 0x3)
;      = 0xbbd (3005)
;   r1 = acchi + (r1.h1 * u6)
;      = 0x4 + (0x0064 * 0x3)
;      = 0x130 (304)
;
  prep_test_case
  set_accu 0x4, 0x5
  mov     r1, 0x006403e8
  vmac2hu r0, r1, 3
  compare_64bit  r1,  r0, 304, 3005
  compare_64bit r59, r58, 304, 3005

; Test case 25
; using simm12: positive
; r0 <- r0, s12
;   r0 = acclo + (r0.h0 * s12);
;   r1 = acchi + (r0.h1 * s12);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (r0.h0 * s12)
;      = 0x30 + (0x7fff * 0x1ff)
;      = 0xff7e31 (16743985)
;   r1 = acchi + (r0.h1 * s12)
;      = 0x20 + (0xffff * 0x1ff)
;      = 0x1fefe21 (33488417)
;
  prep_test_case
  set_accu 0x20, 0x30
  mov     r0, 0xffff7fff
  vmac2hu r0, r0, 511   ; 511=0x1ff
  compare_64bit  r1,  r0, 33488417, 16743985
  compare_64bit r59, r58, 33488417, 16743985

; Test case 26
; using simm12: negative
; r2 <- r2, s12
;   r2 = acclo + (r2.h0 * s12);
;   r3 = acchi + (r2.h1 * s12);
;   acclo = r2;
;   acchi = r3;
;
;   r2 = acclo + (r2.h0 * s12)
;      = 0x63 + (0x0002 * 0xf800)
;      = 0x1f063
;   r3 = acchi + (r2.h1 * s12)
;      = 0x9 + (0x0001 * 0xf800)
;      = 0xf809
;
  prep_test_case
  set_accu 9, 99    ;9=0x9, 99=0x63
  mov     r2, 0x00010002 
  vmac2hu r2, r2, -2048    ; -2048=0xfffff800
  compare_64bit  r3,  r2, 0xf809, 0x1f063
  compare_64bit r59, r58, 0xf809, 0x1f063

; Test case 27
; using limm: 2211282021 -> 0x83cd_7c65 -> (0x83cd, 0x7c65)
; r2 <- r3, limm
;   r2 = acclo + (r3.h0 * limm.h0);
;   r3 = acchi + (r3.h1 * limm.h1);
;   acclo = r2;
;   acchi = r3;
;
;   r2 = acclo + (r3.h0 * limm.h0)
;      = 0x457 + (0x1a85 * 0x7c65)
;      = 0xce2e6d0
;   r3 = acchi + (r3.h1 * limm.h1)
;      = 0x10e1 + (0x3039 * 0x83cd)
;      = 0x18d3d986
;
  prep_test_case
  set_accu 4321, 1111   ; 4321=0x10e1, 1111=0x457
  mov     r3, 0x30391a85   ; 12345=0x3039, 6789=0x1a85
  vmac2hu r2, r3, 0x83cd7c65
  compare_64bit  r3,  r2, 0x18d3d986, 0xce2e6d0
  compare_64bit r59, r58, 0x18d3d986, 0xce2e6d0

; Test case 28
; r0 <- limm * u6
;   r0 = acclo + (limm.h0 * u6);
;   r1 = acchi + (limm.h1 * u6);
;   acclo = r0;
;   acchi = r1;
;
;   r0 = acclo + (limm.h0 * u6)
;      = 0xdeadbeef + (0x223d * 0x2a)
;      = 0xdeb35cf1
;   r1 = acchi + (limm.h1 * u6)
;      = 0x1337cafe + (0xa8ca * 0x2a)
;      = 0x13537c22
;
  prep_test_case
  set_accu 0x1337cafe, 0xdeadbeef
  vmac2hu r0, 0xa8ca223d, 42    ; 43210=0xa8ca 8765=0x223d, 42=0x2a
  compare_64bit  r1,  r0, 0x13537c22, 0xdeb35cf1
  compare_64bit r59, r58, 0x13537c22, 0xdeb35cf1

; Test case 29
; CC: true
; r2 <- r2 * r3
;   r2 = acclo + (r2.h0 * r3.h0);
;   r3 = acchi + (r2.h1 * r3.h1);
;   acclo = r2;
;   acchi = r3;
;
;   r2 = acclo + (r2.h0 * r3.h0)
;      = 0x2 + (0x5 * 0x6)
;      = 0x20 (32)
;   r3 = acchi + (r2.h1 * r3.h1)
;      = 0x1 + (0x0 * 0x0)
;      = 0x1 (1)
;
  prep_test_case
  set_accu 1, 2
  mov       r2, 5
  mov       r3, 6
  add.f     0, 0, 0       ; Z=1
  vmac2hu.z r2, r2, r3
  compare_64bit  r3,  r2, 1, 32
  compare_64bit r59, r58, 1, 32

; Test case 30
; CC: false
; Instruction is not executed
  prep_test_case
  set_accu  1, 2
  mov       r2, 5
  mov       r3, 6
  vmac2hu.z r2, r2, r3    ; Z=0 because of PREP_TEST_CASE
  compare_64bit r3, r2, 6, 5
  compare_64bit r59, r58, 1, 2

;; Test case 31
;; Odd-numbered reg as destination -> Raise illegal exception
;  prep_test_case
;  set_accu 27, 28
;  mov      r3, 0x27
;  mov      r4, r3
;  set_except_params @test_27_illegal, @test_27_check_no_commit
;test_27_illegal:
;  .word  0x01c32c5f  ; vmac2hu r3, r4, 7
;                     ; the stream should read as 0x2c5f_01c3
;                     ; mem:            5f 2c  c3 01
;                     ; middle endian:  2c5f   01c3
;  b      @fail
;test_27_check_no_commit:
;  cmp      r3, 0x27
;  bne      @fail
;  cmp      r3, r4
;  bne      @fail
;  cmp      r59, 27
;  bne      @fail
;  cmp      r58, 28
;  bne      @fail

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Reporting ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

valhalla:
  print "[PASS]"
  b @1f

; If a test fails, it jumps here. Although, for the sake of uniformity,
; the printed output does not say much about which test case failed,
; one can uncomment the print_number line below or set a breakpoint
; here to check the R0 register for the test case number.
fail:
  ld  r0, [test_nr]
  ;print_number r0
  print "[FAIL]"
1:
  print " vmac2h: vmac2h vmac2hu\n"
  end
