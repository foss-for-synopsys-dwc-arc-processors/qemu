; check_popdl.S
;
; Tests for the popdl_s instruction: the 128-bit pop
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
  mov	sp, 0x1000	  ; reset sp
.endm

; 64-bit(hi, low) == 64-bit(reg)
.macro  check_64bit_result      hi, low, reg
  movhl r11, \hi
  orl   r11, r11, \low
  xor.f 0, r11, \reg
  bne   @fail
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
  ld    r11, [cont_addr]
  sr    r11, [eret]
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
first:  .byte 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
        .byte 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f

second: .byte 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27
        .byte 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f

third:  .byte 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37
        .byte 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f

fourth: .byte 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47
        .byte 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; POPDL ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test case 1
; r1r0 = popdl_s
  prep_test_case
  lddl r2r3, [@second]
  stdl.aw r2r3, [sp, -16]
  xor_s   r0, r0, r0
  xor_s   r1, r1, r1
  popdl_s    r0
  check_64bit_result 0x2f2e2d2c, 0x2b2a2928, r1
  check_64bit_result 0x27262524, 0x23222120, r0

; Test case 2
; r29sp = popdl_s
  prep_test_case
  lddl     r2r3, [@second]
  stdl.aw  r2r3, [sp, -16]
  xor_s   r0, r0, r0
  xor_s   r1, r1, r1
  popdl_s    sp
  check_64bit_result 0x2f2e2d2c, 0x2b2a2928, ilink
  check_64bit_result 0x27262524, 0x23222120, sp

; Test case 3
; r3 = popdl_s
; Raise an illegal instruction exception
  prep_test_case
  set_except_params @test_03_exception, @test_03_end
  mov r2, 0x1213141516
test_03_exception:
  ; popdl_s r3
  .byte 0xd1,0xc3
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
  print " popdl: the 128-bit pop\n"
  end
