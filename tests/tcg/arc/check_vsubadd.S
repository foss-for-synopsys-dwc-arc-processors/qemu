; check_vsubadd.S
;
; Tests for vsubadd: Dual 32-bit SIMD subtract and add.
; If the test fails, check the end of this file for how to troubleshoot.

  .include "macros.inc"

#include "ivt.inc"
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

; Checks if (Z,N,C,V) == (0,0,0,0). This relies on "ADD.F 0,0,1"
; instruction in PREP_TEST_CASE macro. From a PREP_TEST_CASE macro
; in a test case, and thence to a VECTOR instruction and finally to
; this macro, none of the aforementioned flags must have been set,
; because VECTOR instructions aren't supposed to do so.
.macro  check_flags_remained_zero
  lr    r11, [status32]
  mov   r12, REG_STAT_Z
  or    r12, r12, REG_STAT_N
  or    r12, r12, REG_STAT_C
  or    r12, r12, REG_STAT_V
  and   r11, r11, r12
  cmp   r11, 0
  bne   @fail
.endm

; pair(HI, LOW) == pair(REG_HI, REG_LO)
; HI, LO:         32-bit
; REG_HI, REG_LO: 32-bit
.macro  set_64bit_double        reg_hi, reg_lo, hi, low
  mov \reg_hi, \hi
  mov \reg_lo, \low
.endm

; pair(HI, LOW) == pair(REG_HI, REG_LO)
; HI, LO:         32-bit
; REG_HI, REG_LO: 32-bit
.macro  check_64bit_double      reg_hi, reg_lo, hi, low
  check_flags_remained_zero
  mov   r11, \hi
  mov   r10, \low
  cmp   r11, \reg_hi
  bne   @fail
  cmp   r10, \reg_lo
  bne   @fail
.endm

; set Carry equal 1
.macro set_carry
  mov     r9, 0x80000000  ; setting...
  add.f   0,r9,r9         ; ...C=1
.endm

; clear Carry
.macro clear_carry
  add.f   0,0,1           ; so that CHECK_FLAGS_REMAINED_ZERO won't fail.
.endm

;;;;;;;;;;;;;;;;;;;;;;;;; Exception related code ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; parameters that an IllegalInstruction exception may set.
  .align 4
ecr_ref  : .word ILLEGAL_INSTRUCTION
addr_ref : .word 0x0                  ; for both eret and efa
cont_addr: .word 0x0

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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VSUBADD ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== vsubadd a,b,c ===

; vsubadd a,b,c 00101bbb001111110BBBCCCCCCAAAAAA
;   a.w0 = b.w0 - c.w0;
;   a.w1 = b.w1 + c.w1;

; Test case 1.1
; Same source and dest operands
; r0 <- r0, r0
;
;   (0x80000001, 0x00000001) [r1:r0]
;        +           -
;   (0x00000001, 0x00000001) [r1:r0]
;   --------------------------------
;   (0x80000002, 0x00000000) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x80000001, 0x00000001
  vsubadd r0, r0, r0
  check_64bit_double r1, r0, 2, 0

; Test case 1.2
; Different source and dest operands
; r0 <- r2, r4
;
;   (0xfffffff5, 0x00000001) [r3:r2]
;        +           -
;   (0x000000ff, 0x00000003) [r5:r4]
;   --------------------------------
;   (0x000000f4, 0xfffffffe) [r1:r0]
;
  prep_test_case
  set_64bit_double r3, r2, 0xfffffff5, 0x00000001
  set_64bit_double r5, r4, 0x000000ff, 0x00000003
  vsubadd r0, r2, r4
  check_64bit_double r1, r0, 0x000000f4, 0xfffffffe

;=== vsubadd 0,b,c ===

; vsubadd 0,b,c 00101bbb001111110BBBCCCCCC111110
;   0 = b.w0 - c.w0;
;   0 = b.w1 + c.w1;

; Test case 2
; 0 <- r0, r2
;
;   (0xfffffff5, 0x00000001) [r1:r0]
;        +           -
;   (0x000000ff, 0x00000003) [r3:r2]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0xfffffff5, 0x00000001 
  set_64bit_double r3, r2, 0x000000ff, 0x00000003 
  vsubadd 0, r0, r2

;=== vsubadd<.cc> b,b,c ===

; vsubadd<.cc> b,b,c 00101bbb111111110BBBCCCCCC0QQQQQ
;   if (cc) {
;     b.w0 = b.w0 - c.w0;
;     b.w1 = b.w1 + c.w1;
;   }

; Test case 3.1
; CC=1
; r2 <- r2, r4 (cc)
;
;   (0xfffffff5, 0x00000001) [r3:r2]
;        +           -
;   (0x000000ff, 0x00000003) [r5:r4]
;   --------------------------------
;   (0x000000f4, 0xfffffffe) [r3:r2]
;
  prep_test_case
  set_64bit_double r3, r2, 0xfffffff5, 0x00000001 
  set_64bit_double r5, r4, 0x000000ff, 0x00000003 
  set_carry
  vsubadd.c r2, r2, r4
  clear_carry
  check_64bit_double r3, r2, 0x000000f4, 0xfffffffe

; Test case 3.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_64bit_double r3, r2, 0xfffffff5, 0x00000001 
  set_64bit_double r5, r4, 0x000000ff, 0x00000003 
  vsubadd.c r2, r2, r4
  check_64bit_double r3, r2, 0xfffffff5, 0x00000001

;=== vsubadd a,b,u6 ===

; vsubadd a,b,u6 00101bbb011111110BBBuuuuuuAAAAAA
;   a.w0 = b.w0 - u6;
;   a.w1 = b.w1 + u6;

; Test case 4
; r0 <- r2, u6
;
;   (0xfffffff5, 0x00000040) [r3:r2]
;        +           -
;   (      0x3f,       0x3f) [u6:u6]
;   --------------------------------
;   (0x00000034, 0x00000001) [r1:r0]
;
  prep_test_case
  set_64bit_double r3, r2, 0xfffffff5, 0x00000040
  vsubadd r0, r2, 0x3f
  check_64bit_double r1, r0, 0x00000034, 0x00000001

;=== vsubadd 0,b,u6 ===

; vsubadd 0,b,u6 00101bbb011111110BBBuuuuuu111110
;   0 = b.w0 - u6;
;   0 = b.w1 + u6;

; Test case 5
; 0 <- r0, u6
;
;   (0xfffffff5, 0x00000040) [r1:r0]
;        +           -
;   (      0x3f,       0x3f) [u6:u6]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0xfffffff5, 0x00000040
  vsubadd 0, r0, 0x3f

;=== vsubadd<.cc> b,b,u6 ===

; vsubadd<.cc> b,b,u6 00101bbb111111110BBBuuuuuu1QQQQQ
;   if (cc) {
;     b.w0 = b.w0 - u6;
;     b.w1 = b.w1 + u6;
;   }

; Test case 6.1
; CC=1
; r0 <- r0, u6 (cc)
;
;   (0xfffffff5, 0x00000040) [r1:r0]
;        +           -
;   (      0x3f,       0x3f) [u6:u6]
;   --------------------------------
;   (0x00000034, 0x00000001) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0xfffffff5, 0x00000040
  set_carry
  vsubadd.c r0, r0, 0x3f
  clear_carry
  check_64bit_double r1, r0, 0x00000034, 0x00000001

; Test case 6.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_64bit_double r1, r0, 0xfffffff5, 0x00000040
  vsubadd.c r0, r0, 0x3f
  check_64bit_double r1, r0, 0xfffffff5, 0x00000040

;=== vsubadd b,b,s12 ===

; vsubadd b,b,s12 00101bbb101111110BBBssssssSSSSSS
;   b.w0 = b.w0 - s12;
;   b.w1 = b.w1 + s12;

; Test case 7.1
; using s12: positive
; r0 <- r0, s12
;
;   (0xfffffff5, 0x00000076) [r1:r0]
;        +           -
;   (      0x75,       0x75) [s12:s12]
;   --------------------------------
;   (0x0000006a, 0x00000001) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0xfffffff5, 0x00000076
  vsubadd r0, r0, 0x75
  check_64bit_double r1, r0, 0x0000006a, 0x00000001

; Test case 7.2
; using s12: negative
; r0 <- r0, s12
;
;   (0x00000003, 0x00000004) [r1:r0]
;        +           -
;   (0xfffffffe, 0xfffffffe) [s12:s12]
;   --------------------------------
;   (0x00000001, 0x00000006) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000003, 0x00000004
  vsubadd r0, r0, -2     ; -2=0xfffffffe 
  check_64bit_double r1, r0, 0x00000001, 0x00000006

;=== vsubadd a,limm,c ===

; vsubadd a,limm,c 00101110001111110111CCCCCCAAAAAA
;   a.w0 = limm - c.w0;
;   a.w1 = 0 + c.w1;

; Test case 8
; r0 <- limm, r2
;
;   (         0, 0x12345678) [0:limm]
;        +           -
;   (0x00000001, 0x00000002) [r3:r2]
;   --------------------------------
;   (0x00000001, 0x12345676) [r1:r0]
;
  prep_test_case
  set_64bit_double r3, r2, 0x00000001, 0x00000002
  vsubadd r0, 0x12345678, r2 
  check_64bit_double r1, r0, 0x00000001, 0x12345676

;=== vsubadd a,b,limm ===

; vsubadd a,b,limm 00101bbb001111110BBB111110AAAAAA
;   a.w0 = b.w0 - limm;
;   a.w1 = b.w1 + 0;

; Test case 9
; r0 <- r2, limm
;
;   (0x00000001, 0x00000002) [r3:r2]
;        +           -
;   (         0, 0x12345678) [0:limm]
;   --------------------------------
;   (       0x1, 0xedcba98a) [r1:r0]
;
  prep_test_case
  set_64bit_double r3, r2, 0x00000001, 0x00000002
  vsubadd r0, r2, 0x12345678 
  check_64bit_double r1, r0, 0x1, 0xedcba98a

;=== vsubadd 0,limm,c ===

; vsubadd 0,limm,c 00101110001111110111CCCCCC111110
;   0 = limm - c.w0;
;   0 = 0 + c.w1;

; Test case 10
; 0 <- limm, r0
;
;   (         0, 0x12345678) [0:limm]
;        +           -
;   (0x00000001, 0x00000002) [r1:r0]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0x00000002
  vsubadd 0, 0x12345678, r0 

;=== vsubadd 0,b,limm ===

; vsubadd 0,b,limm 00101bbb001111110BBB111110111110
;   0 = b.w0 - limm;
;   0 = b.w1 + 0;

; Test case 11
; 0 <- r0, limm
;
;   (0x00000001, 0x00000002) [r1:r0]
;        +           -
;   (         0, 0x12345678) [0:limm]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0x00000002
  vsubadd 0, r0, 0x12345678 

;=== vsubadd<.cc> b,b,limm ===

; vsubadd<.cc> b,b,limm 00101bbb111111110BBB1111100QQQQQ
;   if (cc) {
;     b.w0 = b.w0 - limm;
;     b.w1 = b.w1 + 0;
;   }

; Test case 12.1
; CC=1
; r0 <- r0, limm (cc)
;
;   (0x00000001, 0x00000002) [r1:r0]
;        +           -
;   (         0, 0x12345678) [0:limm]
;   --------------------------------
;   (       0x1, 0xedcba98a) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0x00000002
  set_carry
  vsubadd.c r0, r0, 0x12345678 
  clear_carry
  check_64bit_double r1, r0, 0x1, 0xedcba98a

; Test case 12.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0x00000002
  vsubadd.c r0, r0, 0x12345678 
  check_64bit_double r1, r0, 0x00000001, 0x00000002

;=== vsubadd<.cc> 0,limm,c ===

; vsubadd<.cc> 0,limm,c 00101110111111110111CCCCCC0QQQQQ
;   if (cc) {
;     a.w0 = b.w0 - c.w0;
;     a.w1 = b.w1 + c.w1;
;   }

; Test case 13.1
; CC=1
; 0 <- limm, r0 (cc)
;
;   (         0, 0x12345678) [0:limm]
;        +           -
;   (0x00000001, 0x00000002) [r1:r0]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0x00000002
  set_carry
  vsubadd.c 0, 0x12345678, r0
  clear_carry

; Test case 13.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0x00000002
  vsubadd.c 0, 0x12345678, r0

;=== vsubadd a,limm,u6 ===

; vsubadd a,limm,u6 00101110011111110111uuuuuuAAAAAA
;   a.w0 = limm - u6;
;   a.w1 = 0 + u6;

; Test case 14
; r0 <- limm, u6
;
;   (         0, 0x12345678) [0:limm]
;        +           -
;   (      0x3f,       0x3f) [u6:u6]
;   --------------------------------
;   (      0x3f, 0x12345639) [r1:r0]
;
  prep_test_case
  vsubadd r0, 0x12345678, 0x3f 
  check_64bit_double r1, r0, 0x3f, 0x12345639

;=== vsubadd 0,limm,u6 ===

; vsubadd 0,limm,u6 00101110011111110111uuuuuu111110
;   0 = limm - u6;
;   0 = 0 + u6;

; Test case 15
; 0 <- limm, u6
;
;   (         0, 0x12345678) [0:limm]
;        +           -
;   (      0x3f,       0x3f) [u6:u6]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  vsubadd 0, 0x12345678, 0x3f 

;=== vsubadd<.cc> 0,limm,u6 ===

; vsubadd<.cc> 0,limm,u6 00101110111111110111uuuuuu1QQQQQ
;   if (cc) {
;     0 = limm - u6;
;     0 = 0 + u6;
;   }

; Test case 16.1
; CC=1
; 0 <- limm, u6 (cc)
;
;   (         0, 0x12345678) [0:limm]
;        +           -
;   (      0x3f,       0x3f) [u6:u6]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_carry
  vsubadd.c 0, 0x12345678, 0x3f
  clear_carry

; Test case 16.2
; CC=0
; Instruction is not executed
  prep_test_case
  vsubadd.c 0, 0x12345678, 0x3f 

;=== vsubadd 0,limm,s12 ===

; vsubadd 0,limm,s12 00101110101111110111ssssssSSSSSS
;   0 = limm - s12;
;   0 = 0 + s12;

; Test case 17
; 0 <- limm, s12
;
;   (         0, 0x12345678) [0:limm]
;        +           -
;   (0xfffffffe, 0xfffffffe) [s12:s12]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  vsubadd 0, 0x12345678, -2   ; -2=0xfffffffe

;=== vsubadd a,limm,limm ===

; vsubadd a,limm,limm 00101110001111110111111110AAAAAA
;   a.w0 = limm - limm;
;   a.w1 = 0 + 0;

; Test case 18
; r0 <- limm, limm
;
;   (         0, 0x12345678) [0:limm]
;        +           -
;   (         0, 0x12345678) [0:limm]
;   --------------------------------
;   (         0,          0) [r1:r0]
;
  prep_test_case
  vsubadd r0, 0x12345678, 0x12345678
  check_64bit_double r1, r0, 0, 0

;=== vsubadd 0,limm,limm ===

; vsubadd 0,limm,limm 00101110001111110111111110111110
;   0 = limm - limm;
;   0 = 0 + 0;

; Test case 19
; 0 <- limm, limm
;
;   (         0, 0x12345678) [0:limm]
;        +           -
;   (         0, 0x12345678) [0:limm]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  vsubadd 0, 0x12345678, 0x12345678

;=== vsubadd<.cc> 0,limm,limm ===

; vsubadd<.cc> 0,limm,limm 001011101111111101111111100QQQQQ
;   if (cc) {
;     0 = limm - limm;
;     0 = 0 + 0;
;   }

; Test case 20.1
; CC=1
;
;   (         0, 0x12345678) [0:limm]
;        +           -
;   (         0, 0x12345678) [0:limm]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_carry
  vsubadd.c 0, 0x12345678, 0x12345678
  clear_carry

; Test case 20.2
; CC=0
; Instruction is not executed
  prep_test_case
  vsubadd.c 0, 0x12345678, 0x12345678

;=== exceptions ===

; ; Test case 21.1
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; Destination register is an odd register
;   prep_test_case
;   set_except_params @test_21_1_exception, @test_21_1_end
; test_21_1_exception:
;   vsubadd r1, r2, r4
;   b     @fail
; test_21_1_end:
;   ; Fall through

; ; Test case 21.2
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; First source register is an odd register
;   prep_test_case
;   set_except_params @test_21_2_exception, @test_21_2_end
; test_21_2_exception:
;   vsubadd r0, r1, r4
;   b     @fail
; test_21_2_end:
;   ; Fall through

; ; Test case 21.3
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; Second source register is an odd register
;   prep_test_case
;   set_except_params @test_21_3_exception, @test_21_3_end
; test_21_3_exception:
;   vsubadd r0, r2, r5
;   b     @fail
; test_21_3_end:
;   ; Fall through

; ; Test case 21.4
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; Even if there is no register to save the result to.
;   prep_test_case
;   set_except_params @test_21_4_exception, @test_21_4_end
; test_21_4_exception:
;   vsubadd 0, r3, r0
;   b     @fail
; test_21_4_end:
;   ; Fall through

; ; Test case 21.5
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; The exception should be made even if the CC indicates no execution.
;   prep_test_case        ; (Z,N,C,V)=(0,0,0,0)
;   set_except_params @test_21_5_exception, @test_21_5_end
; test_21_5_exception:
;   vsubadd.z r5, r5, r0
;   b       @fail
; test_21_5_end:
;   ; Fall through

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
  ;print_number r0
  print "[FAIL]"
1:
  print " vsubadd: Dual 32-bit SIMD subtract and add\n"
  end
