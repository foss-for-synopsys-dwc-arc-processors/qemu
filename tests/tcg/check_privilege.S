; check_privilege.S
;
; Privileged instructions in user space: clri, seti
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

;;;;;;;;;;;;;;;;;;;;;;;;; Exception related code ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; parameters that an IllegalInstruction exception may set.
.data
  .align 4
ecr_ref  : .word PRIVILEGE_VIOLATION
addr_ref : .word 0x0                  ; for both eret and efa
cont_addr: .word 0x0

; exception: Privilege Violation
; regs used: r11, r12
;
; A parameterized Privilege Violation exception that checks the followings:
; ecr  == Privilege violation
; efa  == efa_ref
; eret == eret_ref
; If everything passes, it will jump to 'cont_addr' parameter. The parameters
; must be set beforehand using 'set_except_params' macro.  This requires
; ivt.S file to be compiled and linked.
.text
  .align 4
  .global EV_PrivilegeV
  .type EV_PrivilegeV, @function
EV_PrivilegeV:
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
  ; Success: going back to the given address in kernel mode
  ld   r11, [cont_addr]
  sr   r11, [eret]
  lr   r11, [erstatus]
  bclr r11, r11, 7                  ; clear user mode bit
  sr   r11, [erstatus]
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; CLRI ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test case 1
; Using "clri" in user mode should raise an exception
  prep_test_case
  set_except_params @test_1_unprivileged, @test_1_ok
  clri                  ; should not be troublesome in kernel mode
  enter_user_mode @test_1_usermode
test_1_usermode:
  lr   r0, [status32]
test_1_unprivileged:
  clri r0
  b    @fail
test_1_ok:
  ; Fall through

; Test case 2
; Using "seti" in user mode should raise an exception
  prep_test_case
  set_except_params @test_2_unprivileged, @test_2_ok
  seti                  ; should not be troublesome in kernel mode
  enter_user_mode @test_2_usermode
test_2_usermode:
  lr   r1, [status32]
test_2_unprivileged:
  seti r1
  b    @fail
test_2_ok:
  ; Fall through

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
  print " Privileged instructions in user space: clri, seti\n"
  end
