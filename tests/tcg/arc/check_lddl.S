; check_lddl.S
;
; Tests lddl: the 128-bit load
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; LDDL ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start
  
; Test case 1
; r1r0 = [r2]
  prep_test_case
  mov     r2, @second
  xor_s   r0, r0, r0
  xor_s   r1, r1, r1
  lddl    r0, [r2]
  check_64bit_result  0x2f2e2d2c, 0x2b2a2928, r1
  check_64bit_result  0x27262524, 0x23222120, r0
  xor.f   0, r2, @second
  bne     @fail

; Test case 2
; r3r2 = [r0, 8]
  prep_test_case
  mov     r0, @second
  xor_s   r2, r2, r2
  xor_s   r3, r3, r3
  lddl    r2, [r0, 16]
  check_64bit_result  0x3f3e3d3c, 0x3b3a3938, r3
  check_64bit_result  0x37363534, 0x33323130, r2
  xor.f   0, r0, @second
  bne     @fail

; Test case 3
; r1r0 = [r0, 16]    --> same source and dest
  prep_test_case
  mov     r0, @second
  xor_s   r1, r1, r1
  lddl    r0, [r0, 16]
  check_64bit_result  0x3f3e3d3c, 0x3b3a3938, r1
  check_64bit_result  0x37363534, 0x33323130, r0

; Test case 4
; lddl r1 (odd) ... --> Raise an illegal instruction exception AND
;                       the pointer base should not be updated
; N.B. if ivt != 0, have the interrupt vector base adjusted
  prep_test_case
  set_except_params @test_04_exception, @test_04_end
  mov     r2, 0x1213141516
test_04_exception:
  ; lddl.aw r3, [r2, 4]
  .byte   0x04,0x12,0xc3,0x06
  b       @fail
test_04_end:
  xor.f   0, r2, 0x1213141516
  bne     @fail

; Test case 5
; r3r2 = pre-increment, [r0, 16]
  prep_test_case
  mov     r0, @first
  xor_s   r2, r2, r2
  xor_s   r3, r3, r3
  lddl.aw r2, [r0, 16]
  check_64bit_result  0x2f2e2d2c, 0x2b2a2928, r3
  check_64bit_result  0x27262524, 0x23222120, r2
  xor.f   0, r0, @second
  bne     @fail

; Test case 6
; r1r0 = pre-increment, [r1, 16]    --> same source and dest
; r1 should hold the loaded data and not the updated pointer.
  prep_test_case
  mov     r1, @third
  xor_s   r0, r0, r0
  lddl.aw r0, [r1, 16]
  check_64bit_result  0x4f4e4d4c, 0x4b4a4948, r1
  check_64bit_result  0x47464544, 0x43424140, r0

; Test case 7
; r3r2 = post-increment, [r0, 16]
  prep_test_case
  mov     r0, @first
  xor_s   r2, r2, r2
  xor_s   r3, r3, r3
  lddl.ab r2, [r0, 16]
  check_64bit_result  0x1f1e1d1c, 0x1b1a1918, r3
  check_64bit_result  0x17161514, 0x13121110, r2
  xor.f   0, r0, @second
  bne     @fail

; Test case 8
; r1r0 = post-increment, [r0, 16]    --> same source and dest
; r0 should hold the loaded data and not the updated pointer.
  prep_test_case
  mov     r0, @third
  xor_s   r1, r1, r1
  lddl.ab r0, [r0, 16]
  check_64bit_result  0x3f3e3d3c, 0x3b3a3938, r1
  check_64bit_result  0x37363534, 0x33323130, r0

; Test case 9
; r1r0 = address scale of 8, [r0, 1]  --> same source and dest
  prep_test_case
  mov     r0, @first
  xor_s   r1, r1, r1
  lddl.as r0, [r0, 1]
  check_64bit_result  0x27262524, 0x23222120, r1
  check_64bit_result  0x1f1e1d1c, 0x1b1a1918, r0

; Test case 10
; r3r2 = address scale of 8, [r10, 2]
  prep_test_case
  mov     r10, @second
  xor_s   r2, r2, r2
  xor_s   r3, r3, r3
  lddl.as r2, [r10, 2]
  check_64bit_result  0x3f3e3d3c, 0x3b3a3938, r3
  check_64bit_result  0x37363534, 0x33323130, r2
  xor.f   0, r10, @second
  bne     @fail

; Test case 11
; if "base" is a number, no increment can happen -> exception
  prep_test_case
  set_except_params @test_11_exception, @test_11_end
  mov     r2, 11
test_11_exception:
  ;lddl.aw    r2, [0, r1]
  .byte 0xf3,0x24,0x42,0x70,0x00,0x00,0x00,0x00
  b       @fail
test_11_end:
  xor.f   0, r2, 11
  bne     @fail

; Test case 12
; lddl r, [limm, r]
  prep_test_case
  mov     r1, 16
  xor_s   r0, r0, r0
  lddl    r0, [@first, r1]
  check_64bit_result  0x2f2e2d2c, 0x2b2a2928, r1
  check_64bit_result  0x27262524, 0x23222120, r0

; Test case 12
; lddl r, [ximm, r]
  prep_test_case
  mov     r2, @third+1000
  xor_s   r3, r3, r3
  lddl    r2, [-1016, r2]
  check_64bit_result  0x2f2e2d2c, 0x2b2a2928, r3
  check_64bit_result  0x27262524, 0x23222120, r2

; Test case 13
; lddl r, [limm]
  prep_test_case
  xor_s   r0, r0, r0
  xor_s   r1, r1, r1
  lddl    r0, [@fourth]
  check_64bit_result  0x4f4e4d4c, 0x4b4a4948, r1
  check_64bit_result  0x47464544, 0x43424140, r0

; Test case 14
; lddl.as r, [limm, r]
  prep_test_case
  xor     r6, r6, r6
  mov     r7, 2
  lddl.as r6, [@second, r7]
  check_64bit_result  0x3f3e3d3c, 0x3b3a3938, r7
  check_64bit_result  0x37363534, 0x33323130, r6

; Test case 15
; lddl    r, [r, r]
  prep_test_case
  mov     r14, @first-42
  mov     r15, 42
  xor     r4, r4, r4
  xor     r5, r5, r5
  lddl    r4, [r14, r15]
  check_64bit_result  0x1f1e1d1c, 0x1b1a1918, r5
  check_64bit_result  0x17161514, 0x13121110, r4
  xor.f   0, r14, @first-42
  bne     @fail

; Test case 16
; lddl.ab r, [r, r]
  prep_test_case
  mov     r14, @second
  mov     r15, 42
  xor     r8, r8, r8
  xor     r9, r9, r9
  lddl.ab r8, [r14, r15]
  check_64bit_result  0x2f2e2d2c, 0x2b2a2928, r9
  check_64bit_result  0x27262524, 0x23222120, r8
  xor.f   0, r14, @second+42
  bne     @fail

; Test case 17
; lddl.aw r, [r, r]
  prep_test_case
  mov     r14, @third-42
  mov     r15, 42
  xor     r8, r8, r8
  xor     r9, r9, r9
  lddl.aw r8, [r14, r15]
  check_64bit_result  0x3f3e3d3c, 0x3b3a3938, r9
  check_64bit_result  0x37363534, 0x33323130, r8
  xor.f   0, r14, @third
  bne     @fail

; Test case 18
; lddl.as r, [r, r]
  prep_test_case
  mov     r14, @fourth-32
  mov     r15, 4
  xor     r8, r8, r8
  xor     r9, r9, r9
  lddl.as r8, [r14, r15]
  check_64bit_result  0x4f4e4d4c, 0x4b4a4948, r9
  check_64bit_result  0x47464544, 0x43424140, r8
  xor.f   0, r14, @fourth-32
  bne     @fail

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
  print " lddl: the 128-bit load\n"
  end
