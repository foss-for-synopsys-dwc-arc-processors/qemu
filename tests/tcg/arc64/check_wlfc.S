; check_wlfc.S
;
; Tests for wlfc: wlfc / wlfcu
; If the test fails, check the end of this file for how to troubleshoot.

  .include "macros.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;; Test checking routines ;;;;;;;;;;;;;;;;;;;;;;;;;;

; Test case counter
.data
test_nr:
  .word 0x0

; Increment the test counter and set (Z,N,C,V) to (0,0,0,0).
.macro prep_test_case
  ; Stop TIMER0
  sr  0x0,[control0]

  ld    r13, [test_nr]
  add_s r13, r13, 1       ; increase test case counter
  st    r13, [test_nr]
  add.f 0, 0, 1           ; (Z,N,C,V) = (0,0,0,0)

  ; Setup TIMER 0
  mov   r13,  0x5ffff
  sr    r13, [limit0]
  sr    0,   [count0]
  seti
  ; Setup r0
  mov r0, 1
  ; Start timer
  sr  0x01,[control0]

.endm

.align 4
.global IRQ_Timer0
.type IRQ_Timer0, @function
IRQ_Timer0:
    clri
    mov r0, 0
    ; stop timer
    sr  0,[control0]
    rtie

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; WLFC ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; enable usermode sleep
lr r0, [status32]
bset  r0, r0, STATUS32_US_BIT
flag  r0

; Test case 1
; wlfc does NOT sleep if lf is NOT set
;
; Scenario:
; 1. Set the timer to a "large enough" value,
; 2. Set r0 to 1
; 3. Setup the Timer handle to set r0 to 0
; 4. Start the timer
; 5. wlfc
; 6. Save (mov) r0 into r1
;
; The expected result is:
; 1. wlfc does NOT halt CPU
; 2. mov runs before the timer hits
; 3. r1 == 1

prep_test_case

  wlfc 0
  mov  r1, r0  ; save r0 into r1
  ; stop timer
  sr  0x0,[control0]

; If the timer hit, r1 won't be 1
brne  r1, 1, @fail


; Test case 2
; wlfc sleeps due to lf being set
; wakes up after timer interrupt
;
; Scenario:
; Same as before, with the exception of a 'random' llock right before wlfc
;
; The expected result is:
; 1. wlfc HALTS CPU
; 2. timer hits
; 3. r0 = 0
; 4. wlfc wakes up
; 3. r1 == 0

prep_test_case
  llock r0, [@IRQ_Timer0]

  wlfc 0
  mov  r1, r0  ; save r0 into r1
  ; stop timer
  sr  0x0,[control0]

; If the timer didnt hit, r1 won't be 0
brne  r1, 0, @fail


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
  ; print_number r0
  print "[FAIL]"
1:
  print " wlfc\n"
  flag 1
  end
