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
  print "  EXPECTED: "
  print_number_hex \reg_hi
  print ","
  print_number_hex \reg_lo
  print "  REG: "
  print_number_hex \hi
  print ","
  print_number_hex \lo
  print "\n"

  cmp   \reg_lo, \lo
  bne   @fail
  cmp   \reg_hi, \hi
  bne   @fail
.endm

; Compare "reg_hi:reg_lo" against "hi:lo"
.macro compare_32bit reg, lo
  print "  EXPECTED: "
  print_number_hex \reg
  print "  REG: "
  print_number_hex \lo
  print "\n"
  cmp   \reg, \lo
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; dmachu ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test case 1
; A simple dmachu
  prep_test_case
  set_accu 0x0, 0x0
  mov    r0, 0x00020001
  mov    r1, 0x00040003
  dmachu r2, r0, r1
  compare_32bit  r2, 0xb
  compare_64bit r59, r58, 0x0, 0x0b

; Test case 2
; Negative numbers sign extension, and same source and dest operands.
; (0xffff_8765, -1) = (1, -1) * (0x8765, 1)
  prep_test_case
  set_accu 0x0, 0x0
  mov    r0, 0x0001ffff
  mov    r1, 0x87650001
  dmach r0, r0, r1
  compare_32bit  r0, 0xffff8764
  compare_64bit r59, r58, 0xffffffff, 0xffff8764

; Test case 3
; Negative numbers sign extension, and same source and dest operands.
; (0xffff_8765, -1) = (1, -1) * (0x8765, 1)
  prep_test_case
  set_accu 0x0, 0x0
  mov    r0, 0x0001ffff
  mov    r1, 0x87650001
  dmachu  r0, r0, r1
  compare_32bit  r0, 0x18764
  compare_64bit r59, r58, 0x0, 0x18764


; Test case 3
; Bigger numbers
; (-61750, 0x4c5_f4b4) = (2, 0x1234) * (-30875, 0x4321)
; n.b.: 0x8765 -> 0xffff8765 -> -30875
  prep_test_case
  set_accu 0x0, 0x0
  mov    r2, 0x00021234
  mov    r3, 0x87654321
  vmac2h r0, r2, r3
  compare_64bit  r1,  r0, 0xffff0eca, 0x4c5f4b4
  compare_64bit r59, r58, 0xffff0eca, 0x4c5f4b4

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
  print " dmach: dmachu dmach\n"
  end
