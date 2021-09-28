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
; A simple vmac2h
  prep_test_case
  set_accu 0x0, 0x0
  mov    r0, 0x00020001
  mov    r1, 0x00040003
  vmac2h r2, r0, r1
  compare_64bit r3, r2, 0x08, 0x03

; Test case 2
; Negative numbers sign extension, and same source and dest operands.
; (0xffff_8765, -1) = (1, -1) * (0x8765, 1)
  prep_test_case
  set_accu 0x0, 0x0
  mov    r0, 0x0001ffff
  mov    r1, 0x87650001
  vmac2h r0, r0, r1
  compare_64bit r1, r0, 0xffff8765, 0xffffffff

; Test case 3
; Bigger numbers
; (-61750, 0x4c5_f4b4) = (2, 0x1234) * (-30875, 0x4321)
; n.b.: 0x8765 -> 0xffff8765 -> -30875
  prep_test_case
  set_accu 0x0, 0x0
  mov    r2, 0x00021234
  mov    r3, 0x87654321
  vmac2h r0, r2, r3
  compare_64bit r1, r0, 0xffff0eca, 0x4c5f4b4

; Test case 4
; Bigger numbers
; (0xf76c_9a84, 0x16ac_8d78) = (0x1234, 0x5678) * (0x8765, 0x4321)
  prep_test_case
  set_accu 0x0, 0x0
  mov    r2, 0x12345678
  mov    r3, 0x87654321
  vmac2h r0, r2, r3
  compare_64bit r1, r0, 0xf76c9a84, 0x16ac8d78

; Test case 5
; Use addition
; (-111052659, 120162996) = (-11011, 50) * (4342, 1001) + ACC =
;                               (-47809762, 50050)      + (-63242897, 120112946)
  prep_test_case
  set_accu -63242897, 120112946
  mov    r2, 0xd4fd0032        ; -11011=0xd4fd, 50=0x32
  mov    r3, 0x10f603e9        ; 4342=0x10f6, 1001=0x3e9
  vmac2h r4, r2, r3
  compare_64bit r5, r4, -111052659, 120162996

; Test case 6
; Push the limits
; (0x8000_0000, 0x7fff_ffff) = (0xffff, 0xffff) * (0xffff, 1) + (0x7fff_ffff, 0x8000_0000)
  prep_test_case
  set_accu 0x7fffffff, 0x80000000
  mov    r2, 0xffffffff
  mov    r3, 0xffff0001
  vmac2h r0, r2, r3
  compare_64bit r1, r0, 0x80000000, 0x7fffffff

; Test case 7
; 0 as destination
  prep_test_case
  set_accu 0x2, 0x1
  mov    r2, 0x00021234
  mov    r3, 0x87654321
  vmac2h 0, r2, r3

; Test case 8
; using u6
; (1, 8) = (-1, 1) * (3, 3) + (4, 5)
  prep_test_case
  set_accu 0x4, 0x5
  mov    r1, 0xffff0001
  vmac2h r0, r1, 3
  compare_64bit r1, r0, 1, 8

; Test case 9
; using simm12: positive
; (48470, -36456) = (414, -312) * (117, 117) + (32, 48)
  prep_test_case
  set_accu 0x20, 0x30
  mov    r0, 0x19efec8   ; 414=0x19e, -312=0xfec8
  vmac2h r0, r0, 117
  compare_64bit r1, r0, 48470, -36456

; Test case 10
; using simm12: negative
; (7, 95) = (1, 2) * (-2, -2) + (9, 99)
  prep_test_case
  set_accu 9, 99
  mov    r2, 0x00010002 
  vmac2h r2, r2, -2
  compare_64bit r3, r2, 7, 95

; Test case 11
; using limm: -301 -> 0xffff_fed3 -> (0xffff, 0xfed3)
; (-8024, 2042378) = (12345, -6789) * (-1, -301) + (4321, -1111)
; N.B.: although "-301" can fit in an s12; the "vmac2h b,b,s12"
;       pattern is not applicable in this test, because of
;       "vmac2h, r2, r3, -301".
  prep_test_case
  set_accu 4321, -1111
  mov    r3, 0x3039e57b   ; 12345=0x3039, -6789=0xe57b
  vmac2h r2, r3, -301
  compare_64bit r3, r2, -8024, 2042378

; Test case 12
; vector = limm * u6
; (0xecca_f9ec, 0xdeac_f47b) = (4321, -1234) * (42, 42) + (-0x1337_cafe, 0xdead_beef)
  prep_test_case
  set_accu -0x1337cafe, 0xdeadbeef
  vmac2h r0, 0x10e1fb2e, 42    ; 4321=0x10e1 -1234=0xfb2e
  compare_64bit r1, r0, 0xeccaf9ec, 0xdeacf47b

; Test case 13
; Odd-numbered reg as destination -> Raise illegal exception
  prep_test_case
  set_except_params @test_13_illegal, @test_13_ok
test_13_illegal:
  .word  0x01012a1e  ; vmac2h r1, r2, r4
                     ; the stream should read as 0x2a1e_0101
                     ; mem:            1e 2a  01 01
                     ; middle endian:  2a1e   0101
  b      @fail
test_13_ok:
  ; Fall through

; Test case 14
; CC: true
; (1, 32) = (0, 5) * (0, 6) + (1, 2)
  prep_test_case
  set_accu 1, 2
  mov      r2, 5
  mov      r3, 6
  add.f    0, 0, 0       ; Z=1
  vmac2h.z r2, r2, r3
  compare_64bit r3, r2, 1, 32

; Test case 15
; CC: false
  prep_test_case
  set_accu 1, 2
  mov      r2, 5
  mov      r3, 6
  vmac2h.z r2, r2, r3    ; Z=0 because of PREP_TEST_CASE
  compare_64bit r3, r2, 6, 5

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; vmac2hu ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Test case 16
; A simple vmac2hu
  prep_test_case
  set_accu 0x0, 0x0
  mov     r0, 0x00020001
  mov     r1, 0x00040003
  vmac2hu r2, r0, r1
  compare_64bit r3, r2, 0x08, 0x03

; Test case 17
; Non-zero accumulator and same source and dest operands.
; (496, 102) = (17, 09) * (28, 9) + (20, 21)
  prep_test_case
  set_accu 20, 21
  mov     r0, 0x00110009
  mov     r1, 0x001c0009
  vmac2hu r0, r0, r1
  compare_64bit r1, r0, 496, 102

; Test case 18
; Bigger numbers.
; (0x8801_5ea8, 0xbf3a_3870) = (0xfedc, 0xba98) * (0x7654, 0x3210) + (0x12345678, 0x9abcdef0)
  prep_test_case
  set_accu 0x12345678, 0x9abcdef0
  mov     r2, 0xfedcba98
  mov     r3, 0x76543210
  vmac2hu r0, r2, r3
  compare_64bit r1, r0, 0x88015ea8, 0xbf3a3870

; Test case 19
; Bigger numbers: enter the would-be-negative range of numbers.
; The addition should also cause an overflow (no flag is ever set).
; (0x1123_feed, 0x9aaba99) = (0xffee, 0xbbbb) * (0xccdd, 0xbbbb) + (0x44556677, 0x80000000)
  prep_test_case
  set_accu 0x44556677, 0x80000000
  mov     r2, 0xffeebbbb
  mov     r3, 0xccddbbbb
  vmac2hu r0, r2, r3
  compare_64bit r1, r0, 0x1123feed, 0x9aaba99

; Test case 20
; Pushing the limits.
; (0, 0) = (0xffff, 0x1) * (0xffff, 0x1) + (0x1_ffff, 0xffff_ffff)
  prep_test_case
  set_accu 0x1ffff, 0xffffffff
  mov      r0, 0xffff0001
  vmac2hu  r0, r0, r0
  compare_64bit r1, r0, 0, 0

; Test case 21
; 0 as destination
  prep_test_case
  set_accu 0x2, 0x1
  mov     r2, 0x00021234
  mov     r3, 0x87654321
  vmac2hu 0, r2, r3

; Test case 22
; using u6
; (304, 3005) = (100, 1000) * (3, 3) + (4, 5)
  prep_test_case
  set_accu 0x4, 0x5
  mov     r1, 0x006403e8
  vmac2hu r0, r1, 3
  compare_64bit r1, r0, 304, 3005

; Test case 23
; using simm12: positive
; (33488417, 16743985) = (0xffff, 0x7fff) * (511, 511) + (32, 48)
  prep_test_case
  set_accu 0x20, 0x30
  mov     r0, 0xffff7fff
  vmac2hu r0, r0, 511
  compare_64bit r1, r0, 33488417, 16743985

; Test case 24
; using simm12: negative
; (0xf809, 0x1f063) = (1, 2) * (-2048, -2048) + (9, 99)
; n.b.: s12 -> s16 is _always_ sign-extended
; -2048 (s12) -> 0x800 (s12) -> 0xf800 (s16)   
; 1 * 0xf800 +  9 = 0xf809
; 2 * 0xf800 + 99 = 0x1f063
  prep_test_case
  set_accu 9, 99
  mov     r2, 0x00010002 
  vmac2hu r2, r2, -2048
  compare_64bit r3, r2, 0xf809, 0x1f063

; Test case 25
; using limm: 2211282021 -> 0x83cd_7c65 -> (0x83cd, 0x7c65)
; (0x18d3_d986, 0xce2_e6d0) = (12345, 6789) * (0x83cd, 0x7c65) + (4321, 1111)
  prep_test_case
  set_accu 4321, 1111
  mov     r3, 0x30391a85   ; 12345=0x3039, 6789=0x1a85
  vmac2hu r2, r3, 0x83cd7c65
  compare_64bit r3, r2, 0x18d3d986, 0xce2e6d0

; Test case 26
; vector = limm * u6
; (0x1353_7c22, 0xdeb3_5cf1) = (43210, 8765) * (42, 42) + (0x1337_cafe, 0xdead_beef)
  prep_test_case
  set_accu 0x1337cafe, 0xdeadbeef
  vmac2hu r0, 0xa8ca223d, 42    ; 43210=0xa8ca 8765=0x223d
  compare_64bit r1, r0, 0x13537c22, 0xdeb35cf1

; Test case 27
; Odd-numbered reg as destination -> Raise illegal exception
  prep_test_case
  set_except_params @test_27_illegal, @test_27_ok
test_27_illegal:
  .word  0x01c32c5f  ; vmac2hu r3, r4, 7
                     ; the stream should read as 0x2c5f_01c3
                     ; mem:            5f 2c  c3 01
                     ; middle endian:  2c5f   01c3
  b      @fail
test_27_ok:
  ; Fall through

; Test case 28
; CC: true
; (1, 32) = (0, 5) * (0, 6) + (1, 2)
  prep_test_case
  set_accu 1, 2
  mov       r2, 5
  mov       r3, 6
  add.f     0, 0, 0       ; Z=1
  vmac2hu.z r2, r2, r3
  compare_64bit r3, r2, 1, 32

; Test case 29
; CC: false
  prep_test_case
  set_accu  1, 2
  mov       r2, 5
  mov       r3, 6
  vmac2hu.z r2, r2, r3    ; Z=0 because of PREP_TEST_CASE
  compare_64bit r3, r2, 6, 5

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
