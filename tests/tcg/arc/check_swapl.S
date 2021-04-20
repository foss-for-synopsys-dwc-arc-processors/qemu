; check_swap.S
;
; Swap instructions: swapl, swapel
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; SWAP ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test case 1
; swapl: 0x08070605_04030201 -> 0x04030201_08070605
  .equ   TEST_1_NUMBER_HI, 0x08070605
  .equ   TEST_1_NUMBER_LO, 0x04030201
  .equ   TEST_1_EXPCTD_HI, 0x04030201
  .equ   TEST_1_EXPCTD_LO, 0x08070605
  prep_test_case
  mov    r8, TEST_1_NUMBER_LO
  addhl  r8, r8, TEST_1_NUMBER_HI
  mov    r6, TEST_1_EXPCTD_LO
  addhl  r6, r6, TEST_1_EXPCTD_HI
  swapl  r8, r8
  brne   r8, r6, @fail

; Test case 2
; swapel: 0x08070605_04030201 -> 0x01020304_05060708
  .equ   TEST_2_NUMBER_HI, 0x08070605
  .equ   TEST_2_NUMBER_LO, 0x04030201
  .equ   TEST_2_EXPCTD_HI, 0x01020304
  .equ   TEST_2_EXPCTD_LO, 0x05060708
  prep_test_case
  mov    r8, TEST_2_NUMBER_LO
  addhl  r8, r8, TEST_2_NUMBER_HI
  mov    r6, TEST_2_EXPCTD_LO
  addhl  r6, r6, TEST_2_EXPCTD_HI
  swapel r8, r8
  brne   r8, r6, @fail

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
  print " Swap instructions: swapl, swapel\n"
  end
