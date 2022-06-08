; check_vpack2hm.S
;
; Tests vpack2hm: VPACK2HM a, b, c 
;   Function:
;     Compose the destination operand from the higher 16-bits of the 
;     source operands
;   Operation:
;     a = (b.h1 << 16) | c.h1
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

; 32-bit
.macro check_32bit_result      val, reg
  sub.f     0, \reg, \val
  bne       @fail
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VPACK2HM ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start
  
; Test case 1
; VPACK2HM r0, r1, r2
  prep_test_case
  mov r1, 0x12345678
  mov r2, 0x9abcdef1
  vpack2hm r0, r1, r2
  check_32bit_result 0x12349abc, r0
	
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
  print " vpack2hm: pack the higher 16-bits of the source operands\n"
  end

