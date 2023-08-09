; check_pushdl.S
;
; Tests pushdl: the 128-bit push
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
  mov sp, 0x1000 	  ; reset sp
.endm

; reg <- (hi,lo)
.macro mov64 reg, hi, lo
  movhl  \reg, \hi
  orl    \reg, \reg, \lo
.endm

; [mem] == (lohi, lolo, hihi, hilo)
.macro check_128bit_mem hihi, hilo, lohi, lolo, mem
  mov64  r11, \hihi, \hilo
  lddl   r12, [\mem]
  xorl.f  0, r11, r13
  bne    @fail
  mov64  r11, \lohi, \lolo
  xorl.f  0, r11, r12
  bne    @fail
.endm

;;;;;;;;;;;;;;;;;;;;;;;;; Exception related code ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.data
; parameters that an IllegalInstruction exception may set.
  .align 4
ecr_ref  : .word ILLEGAL_INSTRUCTION
addr_ref : .word 0x0                  ; for both eret and efa
cont_addr: .word 0x0

.text
; exception: IllegalInstruction
; regs used: r11, r12
;
; A parameterized IllegalInstruction exception that checks the followings:
; ecr  == Illegal instruction
; efa  == efa_ref
; eret == eret_ref
; If everything passes, it will jump to 'cont_addr' parameter. The parameters
; must be set beforehand using 'set_except_params' macro.  This requires
; ivt.S file to be compiled and linked.
IVT_ENTRY instruction_error
  ld    r11, [ecr_ref]
  lr    r12, [ecr]
  xor.f 0, r12, r11
  bne   @fail
  ld    r11, [addr_ref]
  lr    r12, [eret]
  xor.f 0, r12, r11
  bne   @fail
  lr    r12, [efa]
  xor.f 0, r12, r11
  bne   @fail
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Data ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.data
first:  .skip 16
second: .skip 16
third:  .skip 16
fourth: .skip 16

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; PUSHDL ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test case 1
; pushdl_s r0
  .equ    t01_hihi, 0x1f1e1d1c
  .equ    t01_hilo, 0x1b1a1918
  .equ    t01_lohi, 0x17161514
  .equ    t01_lolo, 0x13121110
  prep_test_case
  mov64   r0, t01_lohi, t01_lolo
  mov64   r1, t01_hihi, t01_hilo
  pushdl_s r0
  check_128bit_mem  t01_hihi, t01_hilo, t01_lohi, t01_lolo, sp

; Test case 2
; pushdl_s sp
  .equ    t02_hihi, 0x2f2e2d2c
  .equ    t02_hilo, 0x2b2a2928
  .equ    t02_lohi, 0x0
  .equ    t02_lolo, 0x2000
  prep_test_case
  mov64   sp, t02_lohi, t02_lolo
  mov64   ilink, t02_hihi, t02_hilo
  pushdl_s sp
  check_128bit_mem  t02_hihi, t02_hilo, t02_lohi, t02_lolo, sp

; Test case 3
; pushdl_s r3
; Raise an illegal instruction exception
  prep_test_case
  set_except_params @test_03_exception, @test_03_end
  mov r2, 0x1213141516
test_03_exception:
  ; pushdl r3
  .byte 0xf1,0xc3
test_03_end:
  xor.f 0, r2, 0x1213141516
  bne @fail

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
  print " pushdl: the 128-bit push\n"
  end
