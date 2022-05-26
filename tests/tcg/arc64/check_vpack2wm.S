; check_vpack2wm.S
;
; Tests vpack2wm: VPACK2WM a, b, c 
;   Function:
;     Compose the destination result using the odd-numbered words
;     from the source operands
;   Operation:
;     a = (c.w1 << 32) | b.w1
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

; 64-bit(hi, low) == 64-bit(reg)
.macro  check_64bit_result      hi, low, reg
  movhl  r11, \hi
  orl    r11, r11, \low
  xorl.f 0, r11, \reg
  bne    @fail
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VPACK2WM ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start
  
; Test case 1
; VPACK2WM r0, r1, r2
  prep_test_case
  movhl r1, 0x01020304
  orl r1, r1, 0x05060708
  movhl r2, 0x090a0b0c
  orl r2, r2, 0x0d0e0f10
  vpack2wm r0, r1, r2
  check_64bit_result 0x090a0b0c, 0x01020304, r0
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Reporting ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

valhalla:
  ;mov r0, 0x1337            ; for nsim enthusiasts
  ;brk                       ; for nsim enthusiasts
  print "[PASS]"
  b @1f

; If a test fails, it jumps here. Although, for the sake of uniformity,
; the printed output does not say much about which test case failed,
; one can uncomment the print_number line below or set a breakpoint
; here to check the R0 register for the test case number.
fail:
  ld r0, [test_nr]
  ;mov r0, 0xbadcafe         ; for nsim enthusiasts
  ;brk                       ; for nsim enthusiasts
  ;print_number r0
  print "[FAIL]"
1:
  print " vpack2wm: pack odd-numbered words from the source operands\n"
  end
