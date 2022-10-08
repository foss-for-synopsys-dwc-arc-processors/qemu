; check_vaddsub.S
;
; Tests for vaddsub: Dual 32-bit SIMD add and subtract.
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VADDSUB ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== vaddsub a,b,c ===

; vaddsub a,b,c 00101bbb001111100BBBCCCCCCAAAAAA
;   a.w0 = b.w0 + c.w0;
;   a.w1 = b.w1 - c.w1;

; Test case 1.1
; Same source and dest operands
; r0 <- r0, r0
;
;   (0x00000001, 0x80000001) [r1:r0]
;        -           +
;   (0x00000001, 0x80000001) [r1:r0]
;   --------------------------------
;   (0x00000000, 0x0000002) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0x80000001
  vaddsub r0, r0, r0
  check_64bit_double r1, r0, 0, 2

; Test case 1.2
; Different source and dest operands
; r0 <- r2, r4
;
;   (0x00000001, 0xfffffff5) [r3:r2]
;        -           +
;   (0x00000003, 0x000000ff) [r5:r4]
;   --------------------------------
;   (0xfffffffe, 0x000000f4) [r1:r0]
;
  prep_test_case
  set_64bit_double r3, r2, 0x00000001, 0xfffffff5
  set_64bit_double r5, r4, 0x00000003, 0x000000ff
  vaddsub r0, r2, r4
  check_64bit_double r1, r0, 0xfffffffe, 0x000000f4

;=== vaddsub 0,b,c ===

; vaddsub 0,b,c 00101bbb001111100BBBCCCCCC111110
;   0 = b.w0 + c.w0;
;   0 = b.w1 - c.w1;

; Test case 2
; 0 <- r0, r2
;
;   (0x00000001, 0xfffffff5) [r1:r0]
;        -           +
;   (0x00000003, 0x000000ff) [r3:r2]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0xfffffff5
  set_64bit_double r3, r2, 0x00000003, 0x000000ff
  vaddsub 0, r0, r2

;=== vaddsub<.cc> b,b,c ===

; vaddsub<.cc> b,b,c 00101bbb111111100BBBCCCCCC0QQQQQ
;   if (cc) {
;     b.w0 = b.w0 + c.w0;
;     b.w1 = b.w1 - c.w1;
;   }
;

; Test case 3.1
; CC=1
; r2 <- r2, r4 (cc)
;
;   (0x00000001, 0xfffffff5) [r3:r2]
;        -           +
;   (0x00000003, 0x000000ff) [r5:r4]
;   --------------------------------
;   (0xfffffffe, 0x000000f4) [r3:r2]
;
  prep_test_case
  set_64bit_double r3, r2, 0x00000001, 0xfffffff5
  set_64bit_double r5, r4, 0x00000003, 0x000000ff
  set_carry
  vaddsub.c r2, r2, r4
  clear_carry
  check_64bit_double r3, r2, 0xfffffffe, 0x000000f4

; Test case 3.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_64bit_double r3, r2, 0x00000001, 0xfffffff5
  set_64bit_double r5, r4, 0x00000003, 0x000000ff
  vaddsub.c r2, r2, r4
  check_64bit_double r3, r2, 0x00000001, 0xfffffff5

;=== vaddsub a,b,u6 ===

; vaddsub a,b,u6 00101bbb011111100BBBuuuuuuAAAAAA
;   a.w0 = b.w0 + u6;
;   a.w1 = b.w1 - u6;

; Test case 4
; r0 <- r2, u6
;
;   (0x00000040, 0xfffffff5) [r3:r2]
;        -           +
;   (      0x3f,       0x3f) [u6:u6]
;   --------------------------------
;   (0x00000001, 0x00000034) [r1:r0]
;
  prep_test_case
  set_64bit_double r3, r2, 0x00000040, 0xfffffff5
  vaddsub r0, r2, 0x3f
  check_64bit_double r1, r0, 0x00000001, 0x00000034

;=== vaddsub 0,b,u6 ===

; vaddsub 0,b,u6 00101bbb011111100BBBuuuuuu111110
;   0 = b.w0 + u6;
;   0 = b.w1 - u6;

; Test case 5
; 0 <- r2, u6
;
;   (0x00000040, 0xfffffff5) [r3:r2]
;        -           +
;   (      0x3f,       0x3f) [u6:u6]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_64bit_double r3, r2, 0x00000040, 0xfffffff5
  vaddsub 0, r2, 0x3f

;=== vaddsub<.cc> b,b,u6 ===

; vaddsub<.cc> b,b,u6 00101bbb111111100BBBuuuuuu1QQQQQ
;   if (cc) {
;     b.w0 = b.w0 + u6;
;     b.w1 = b.w1 - u6;
;   }

; Test case 6.1
; CC=1
; r0 <- r0, u6 (cc)
;
;   (0x00000040, 0xfffffff5) [r1:r0]
;        -           +
;   (      0x3f,       0x3f) [u6:u6]
;   --------------------------------
;   (0x00000001, 0x00000034) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000040, 0xfffffff5
  set_carry
  vaddsub.c r0, r0, 0x3f
  clear_carry
  check_64bit_double r1, r0, 0x00000001, 0x00000034 

; Test case 6.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_64bit_double r1, r0, 0x00000040, 0xfffffff5
  vaddsub.c r0, r0, 0x3f
  check_64bit_double r1, r0, 0x00000040, 0xfffffff5

;=== vaddsub b,b,s12 ===

; vaddsub b,b,s12 00101bbb101111100BBBssssssSSSSSS
;   b.w0 = b.w0 + s12;
;   b.w1 = b.w1 - s12;

; Test case 7.1
; using s12: positive
; r0 <- r0, s12
;
;   (0x00000076, 0xfffffff5) [r1:r0]
;        -           +
;   (      0x75,       0x75) [s12:s12]
;   --------------------------------
;   (0x00000001, 0x0000006a) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000076, 0xfffffff5
  vaddsub r0, r0, 0x75
  check_64bit_double r1, r0, 0x00000001, 0x0000006a

; Test case 7.2
; using s12: negative
; r0 <- r0, s12
;
;   (0x00000004, 0x00000003) [r1:r0]
;        -           +
;   (0xfffffffe, 0xfffffffe) [s12:s12]
;   --------------------------------
;   (0x00000006, 0x00000001) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000004, 0x00000003
  vaddsub r0, r0, -2     ; -2=0xfffffffe 
  check_64bit_double r1, r0, 0x00000006, 0x00000001

;=== vaddsub a,limm,c ===

; vaddsub a,limm,c 00101110001111100111CCCCCCAAAAAA
;   a.w0 = limm + c.w0;
;   a.w1 = 0 - c.w1;

; Test case 8
; r0 <- limm, r2
;
;   (         0, 0x12345678) [0:limm]
;        -           +
;   (0x00000001, 0x00000002) [r3:r2]
;   --------------------------------
;   (0xffffffff, 0x1234567a) [r1:r0]
;
  prep_test_case
  set_64bit_double r3, r2, 0x00000001, 0x00000002
  vaddsub r0, 0x12345678, r2 
  check_64bit_double r1, r0, 0xffffffff, 0x1234567a

;=== vaddsub a,b,limm ===

; vaddsub a,b,limm 00101bbb001111100BBB111110AAAAAA
;   a.w0 = b.w0 + limm;
;   a.w1 = b.w1 - 0;

; Test case 9
; r0 <- r2, limm
;
;   (0x00000001, 0x00000002) [r3:r2]
;        -           +
;   (         0, 0x12345678) [0:limm]
;   --------------------------------
;   (       0x1, 0x1234567a) [r1:r0]
;
  prep_test_case
  set_64bit_double r3, r2, 0x00000001, 0x00000002
  vaddsub r0, r2, 0x12345678 
  check_64bit_double r1, r0, 0x1, 0x1234567a

;=== vaddsub 0,limm,c ===

; vaddsub 0,limm,c 00101110001111100111CCCCCC111110
;   0 = limm + c.w0;
;   0 = 0 + c.w1;

; Test case 10
; 0 <- limm, r0
;
;   (         0, 0x12345678) [0:limm]
;        -           +
;   (0x00000001, 0x00000002) [r1:r0]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0x00000002
  vaddsub 0, 0x12345678, r0 

;=== vaddsub 0,b,limm ===

; vaddsub 0,b,limm 00101bbb001111100BBB111110111110
;   0 = b.w0 + limm;
;   0 = b.w1 - 0;

; Test case 11
; 0 <- r0, limm
;
;   (0x00000001, 0x00000002) [r1:r0]
;        -           +
;   (         0, 0x12345678) [0:limm]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0x00000002
  vaddsub 0, r0, 0x12345678 

;=== vaddsub<.cc> b,b,limm ===

; vaddsub<.cc> b,b,limm 00101bbb111111100BBB1111100QQQQQ
; Test case 12.1
; CC=1
; r0 <- r0, limm (cc)
;
;   (0x00000001, 0x00000002) [r1:r0]
;        -           +
;   (         0, 0x12345678) [0:limm]
;   --------------------------------
;   (       0x1, 0x1234567a) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0x00000002
  set_carry
  vaddsub.c r0, r0, 0x12345678 
  clear_carry
  check_64bit_double r1, r0, 0x1, 0x1234567a

; Test case 12.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0x00000002
  vaddsub.c r0, r0, 0x12345678 
  check_64bit_double r1, r0, 0x00000001, 0x00000002

;=== vaddsub<.cc> 0,limm,c ===

; vaddsub<.cc> 0,limm,c 00101110111111100111CCCCCC0QQQQQ
;   if (cc) {
;     0 = limm + c.w0;
;     0 = 0 - c.w1;
;   }

; Test case 13.1
; CC=1
; 0 <- limm, r0 (cc)
;
;   (         0, 0x12345678) [0:limm]
;        -           +
;   (0x00000001, 0x00000002) [r1:r0]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0x00000002
  set_carry
  vaddsub.c 0, 0x12345678, r0 
  clear_carry

; Test case 13.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_64bit_double r1, r0, 0x00000001, 0x00000002
  vaddsub.c 0, 0x12345678, r0 

;=== vaddsub a,limm,u6 ===

; vaddsub a,limm,u6 00101110011111100111uuuuuuAAAAAA
;   a.w0 = limm + u6;
;   a.w1 = 0 - u6;

; Test case 14
; r0 <- limm, u6
;
;   (         0, 0x12345678) [0:limm]
;        -           +
;   (      0x3f,       0x3f) [u6:u6]
;   --------------------------------
;   (0xffffffc1, 0x123456b7) [r1:r0]
;
  prep_test_case
  vaddsub r0, 0x12345678, 0x3f 
  check_64bit_double r1, r0, 0xffffffc1, 0x123456b7

;=== vaddsub 0,limm,u6 ===

; vaddsub 0,limm,u6 00101110011111100111uuuuuu111110
;   0 = limm + u6;
;   0 = 0 - u6;

; Test case 15
; 0 <- limm, u6
;
;   (         0, 0x12345678) [0:limm]
;        -           +
;   (      0x3f,       0x3f) [u6:u6]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  vaddsub 0, 0x12345678, 0x3f 

;=== vaddsub<.cc> 0,limm,u6 ===

; vaddsub<.cc> 0,limm,u6 00101110111111100111uuuuuu1QQQQQ
;   if (cc) {
;     0 = limm + u6;
;     0 = 0 - u6;
;   }

; Test case 16.1
; CC=1
; 0 <- limm, u6 (cc)
;
;   (         0, 0x12345678) [0:limm]
;        -           +
;   (      0x3f,       0x3f) [u6:u6]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_carry
  vaddsub.c 0, 0x12345678, 0x3f 
  clear_carry

; Test case 16.2
; CC=0
; Instruction is not executed
  prep_test_case
  vaddsub.c 0, 0x12345678, 0x3f 

;=== vaddsub 0,limm,s12 ===

; vaddsub 0,limm,s12 00101110101111100111ssssssSSSSSS
;   0 = limm + s12;
;   0 = 0 - s12;

; Test case 17
; 0 <- limm, s12
;
;   (         0, 0x12345678) [0:limm]
;        -           +
;   (0xfffffffe, 0xfffffffe) [s12:s12]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  vaddsub 0, 0x12345678, -2   ; -2=0xfffffffe

;=== vaddsub a,limm,limm ===

; vaddsub a,limm,limm 00101110001111100111111110AAAAAA
;   a.w0 = limm + limm;
;   a.w1 = 0 - 0;

; Test case 18
; r0 <- limm, limm
;
;   (         0, 0x12345678) [0:limm]
;        -           +
;   (         0, 0x12345678) [0:limm]
;   --------------------------------
;   (         0, 0x2468acf0) [r1:r0]
;
  prep_test_case
  vaddsub r0, 0x12345678, 0x12345678
  check_64bit_double r1, r0, 0, 0x2468acf0

;=== vaddsub 0,limm,limm ===

; vaddsub 0,limm,limm 00101110001111100111111110111110
;   0 = limm + limm;
;   0 = 0 - 0;

; Test case 19
; 0 <- limm, limm
;
;   (         0, 0x12345678) [0:limm]
;        -           +
;   (         0, 0x12345678) [0:limm]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  vaddsub 0, 0x12345678, 0x12345678


;=== vaddsub<.cc> 0,limm,limm ===

; vaddsub<.cc> 0,limm,limm 001011101111111001111111100QQQQQ
;   if (cc) {
;     0 = limm + limm;
;     0 = 0 - 0;
;   }

; Test case 20.1
; CC=1
; 0 <- limm, limm (cc)
;
;   (         0, 0x12345678) [0:limm]
;        -           +
;   (         0, 0x12345678) [0:limm]
;   --------------------------------
;   (----------, ----------) [0:0]
;
  prep_test_case
  set_carry
  vaddsub.c 0, 0x12345678, 0x12345678
  clear_carry

; Test case 20.2
; CC=0
; Instruction is not executed
  prep_test_case
  vaddsub.c 0, 0x12345678, 0x12345678

;=== exceptions ===

; ; Test case 21.1
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; Destination register is an odd register
;   prep_test_case
;   set_except_params @test_21_1_exception, @test_21_1_end
; test_21_1_exception:
;   vaddsub r1, r2, r4
;   b     @fail
; test_21_1_end:
;   ; Fall through

; ; Test case 21.2
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; First source register is an odd register
;   prep_test_case
;   set_except_params @test_21_2_exception, @test_21_2_end
; test_21_2_exception:
;   vaddsub r0, r1, r4
;   b     @fail
; test_21_2_end:
;   ; Fall through

; ; Test case 21.3
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; Second source register is an odd register
;   prep_test_case
;   set_except_params @test_21_3_exception, @test_21_3_end
; test_21_3_exception:
;   vaddsub r0, r2, r5
;   b     @fail
; test_21_3_end:
;   ; Fall through

; ; Test case 21.4
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; Even if there is no register to save the result to.
;   prep_test_case
;   set_except_params @test_21_4_exception, @test_21_4_end
; test_21_4_exception:
;   vaddsub 0, r3, r0
;   b     @fail
; test_21_4_end:
;   ; Fall through

; ; Test case 21.5
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; The exception should be made even if the CC indicates no execution.
;   prep_test_case        ; (Z,N,C,V)=(0,0,0,0)
;   set_except_params @test_21_5_exception, @test_21_5_end
; test_21_5_exception:
;   vaddsub.z r5, r5, r0
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
  print " vaddsub: Dual 32-bit SIMD add and subtract\n"
  end
