; check_vaddsub4h.S
;
; Tests for vaddsub4h: Quad 16-bit SIMD add and subtract.
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; VADDSUB4H ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== vaddsub4h a,b,c ===

; vaddsub4h a,b,c 00101bbb001110100BBBCCCCCCAAAAAA
;   a.h0 = b.h0 + c.h0;
;   a.h1 = b.h1 - c.h1;
;   a.h2 = b.h2 + c.h2;
;   a.h3 = b.h3 - c.h3;

; Test case 1.1
; Same source and dest operands
; r0 <- r0, r0
;
;   (0x8004, 0x8003, 0x8002, 0x8001) [r1:r0]
;      -       +       -       +
;   (0x8004, 0x8003, 0x8002, 0x8001) [r1:r0]
;   --------------------------------
;   (0x0000, 0x0006, 0x0000, 0x0002) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x80048003, 0x80028001
  vaddsub4h r0, r0, r0
  check_64bit_double r1, r0, 0x00000006, 0x00000002

; Test case 1.2
; Different source and dest operands
; r0 <- r2, r4
;
;   (0x8005, 0x8003, 0x0001, 0xfff5) [r3:r2]
;      -       +       -       +
;   (0x8004, 0x8003, 0x0003, 0x00ff) [r5:r4]
;   --------------------------------
;   (0x0001, 0x0006, 0xfffe, 0x00f4) [r1:r0]
;
  prep_test_case
  set_64bit_double r3, r2, 0x80058003, 0x0001fff5
  set_64bit_double r5, r4, 0x80048003, 0x000300ff
  vaddsub4h r0, r2, r4
  check_64bit_double r1, r0, 0x00010006, 0xfffe00f4

;=== vaddsub4h 0,b,c ===

; vaddsub4h 0,b,c 00101bbb001110100BBBCCCCCC111110
;   0 = b.h0 + c.h0;
;   0 = b.h1 - c.h1;
;   0 = b.h2 + c.h2;
;   0 = b.h3 - c.h3;

; Test case 2
; 0 <- r0, r2
;
;   (0x8005, 0x8003, 0x0001, 0xfff5) [r1:r0]
;      -       +       -       +
;   (0x8004, 0x8003, 0x0003, 0x00ff) [r3:r2]
;   --------------------------------
;   (------, ------, ------, ------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x80058003, 0x0001fff5
  set_64bit_double r3, r2, 0x80048003, 0x000300ff
  vaddsub4h 0, r0, r2

;=== vaddsub4h<.cc> b,b,c ===

; vaddsub4h<.cc> b,b,c 00101bbb111110100BBBCCCCCC0QQQQQ
;   if (cc) {
;     a.h0 = b.h0 + c.h0;
;     a.h1 = b.h1 - c.h1;
;     a.h2 = b.h2 + c.h2;
;     a.h3 = b.h3 - c.h3;
;   }

; Test case 3.1
; CC=1
; r2 <- r2, r4 (cc)
;
;   (0x8005, 0x8003, 0x0001, 0xfff5) [r3:r2]
;      -       +       -       +
;   (0x8004, 0x8003, 0x0003, 0x00ff) [r5:r4]
;   --------------------------------
;   (0x0001, 0x0006, 0xfffe, 0x00f4) [r3:r2]
;
  prep_test_case
  set_64bit_double r3, r2, 0x80058003, 0x0001fff5
  set_64bit_double r5, r4, 0x80048003, 0x000300ff
  set_carry
  vaddsub4h.c r2, r2, r4
  clear_carry
  check_64bit_double r3, r2, 0x00010006, 0xfffe00f4

; Test case 3.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_64bit_double r3, r2, 0x80058003, 0x0001fff5
  set_64bit_double r5, r4, 0x80048003, 0x000300ff
  vaddsub4h.c r2, r2, r4  ; Z=0 because of PREP_TEST_CASE
  check_64bit_double r3, r2, 0x80058003, 0x0001fff5

;=== vaddsub4h a,b,u6 ===

; vaddsub4h a,b,u6 00101bbb011110100BBBuuuuuuAAAAAA
;   a.h0 = b.h0 + u6;
;   a.h1 = b.h1 - u6;
;   a.h2 = b.h2 + u6;
;   a.h3 = b.h3 - u6;

; Test case 4
; r0 <- r2, u6
;
;   (0x003f, 0x003f, 0x0040, 0xfff5) [r3:r2]
;      -       +       -       +
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6:u6]
;   --------------------------------
;   (0x0000, 0x007e, 0x0001, 0x0034) [r1:r0]
;
  prep_test_case
  set_64bit_double r3, r2, 0x003f003f, 0x0040fff5
  vaddsub4h r0, r2, 0x3f
  check_64bit_double r1, r0, 0x0000007e, 0x00010034

;=== vaddsub4h 0,b,u6 ===

; vaddsub4h 0,b,u6 00101bbb011110100BBBuuuuuu111110
;   0 = b.h0 + u6;
;   0 = b.h1 - u6;
;   0 = b.h2 + u6;
;   0 = b.h3 - u6;

; Test case 5
; 0 <- r0, u6
;
;   (0x003f, 0x003f, 0x0040, 0xfff5) [r1:r0]
;      -       +       -       +
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6:u6]
;   --------------------------------
;   (------, ------, ------, ------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x003f003f, 0x0040fff5
  vaddsub4h 0, r0, 0x3f

;=== vaddsub4h<.cc> b,b,u6 ===

; vaddsub4h<.cc> b,b,u6 00101bbb111110100BBBuuuuuu1QQQQQ
;   b.h0 = b.h0 + u6;
;   b.h1 = b.h1 - u6;
;   b.h2 = b.h2 + u6;
;   b.h3 = b.h3 - u6;

; Test case 6.1
; CC=1
; r0 <- r0, u6
;
;   (0x003f, 0x003f, 0x0040, 0xfff5) [r1:r0]
;      -       +       -       +
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6:u6]
;   --------------------------------
;   (0x0000, 0x007e, 0x0001, 0x0034) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x003f003f, 0x0040fff5
  set_carry
  vaddsub4h.c r0, r0, 0x3f
  clear_carry
  check_64bit_double r1, r0, 0x0000007e, 0x00010034

; Test case 6.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_64bit_double r1, r0, 0x003f003f, 0x0040fff5
  vaddsub4h.c r0, r0, 0x3f
  check_64bit_double r1, r0, 0x003f003f, 0x0040fff5

;=== vaddsub4h b,b,s12 ===

; vaddsub4h b,b,s12 00101bbb101110100BBBssssssSSSSSS
;   b.h0 = b.h0 + s12;
;   b.h1 = b.h1 - s12;
;   b.h2 = b.h2 + s12;
;   b.h3 = b.h3 - s12;

; Test case 7.1
; using s12: positive
; r0 <- r0, s12
;
;   (0x8004, 0x8003, 0x0076, 0xfff5) [r1:r0]
;      -       +       -       +
;   (  0x75,   0x75,   0x75,   0x75) [s12:s12]
;   --------------------------------
;   (0x7f8f, 0x8078, 0x0001, 0x006a) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x80048003, 0x0076fff5
  vaddsub4h r0, r0, 0x75
  check_64bit_double r1, r0, 0x7f8f8078, 0x0001006a

; Test case 7.2
; using s12: negative
; r0 <- r0, s12
;
;   (0xfffe, 0xfffe, 0x0004, 0x0003) [r1:r0]
;      -       +       -       +
;   (0xfffe, 0xfffe, 0xfffe, 0xfffe) [s12:s12]
;   --------------------------------
;   (0x0000, 0xfffc, 0x0006, 0x0001) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0xfffefffe, 0x00040003
  vaddsub4h r0, r0, -2     ; -2=0xfffe 
  check_64bit_double r1, r0, 0x0000fffc, 0x00060001

;=== vaddsub4h a,limm,c ===

; vaddsub4h a,limm,c 00101110001110100111CCCCCCAAAAAA
;   a.h0 = limm.h0 + c.h0;
;   a.h1 = limm.h1 - c.h1;
;   a.h2 = 0 + c.h2;
;   a.h3 = 0 - c.h3;

; Test case 8
; r0 <- limm, r2
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;      -       +       -       +
;   (0x0001, 0x0001, 0x1234, 0xa988) [r3:r2]
;   --------------------------------
;   (0xffff, 0x0001, 0x0000, 0x0000) [r1:r0]
;
  prep_test_case
  set_64bit_double r3, r2, 0x00010001, 0x1234a988
  vaddsub4h r0, 0x12345678, r2 
  check_64bit_double r1, r0, 0xffff0001, 0x00000000

;=== vaddsub4h a,b,limm ===

; vaddsub4h a,b,limm 00101bbb001110100BBB111110AAAAAA
;   a.h0 = b.h0 + limm.h0;
;   a.h1 = b.h1 - limm.h1;
;   a.h2 = b.h2 + 0;
;   a.h3 = b.h3 - 0;

; Test case 9
; r0 <- r2, limm
;
;   (0x0001, 0x0001, 0x1234, 0xa988) [r3:r2]
;      -       +       -       +
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;   --------------------------------
;   (0x0001, 0x0001, 0x0000, 0x0000) [r1:r0]
;
  prep_test_case
  set_64bit_double r3, r2, 0x00010001, 0x1234a988
  vaddsub4h r0, r2, 0x12345678 
  check_64bit_double r1, r0, 0x00010001, 0x00000000

;=== vaddsub4h 0,limm,c ===

; vaddsub4h 0,limm,c 00101110001110100111CCCCCC111110
;   0 = limm.h0 + c.h0;
;   0 = limm.h1 - c.h1;
;   0 = 0 + c.h2;
;   0 = 0 - c.h3;

; Test case 10
; 0 <- limm, r0
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;      -       +       -       +
;   (0x0001, 0x0001, 0x1234, 0xa988) [r1:r0]
;   --------------------------------
;   (------, ------, ------, ------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00010001, 0x1234a988
  vaddsub4h 0, 0x12345678, r0 

;=== vaddsub4h 0,b,limm ===

; vaddsub4h 0,b,limm 00101bbb001110100BBB111110111110
;   0 = b.h0 + limm.h0;
;   0 = b.h1 - limm.h1;
;   0 = b.h2 + 0;
;   0 = b.h3 - 0;

; Test case 11
; 0 <- r0, limm
;
;   (0x0001, 0x0001, 0x1234, 0xa988) [r1:r0]
;      -       +       -       +
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;   --------------------------------
;   (------, ------, ------, ------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00010001, 0x1234a988
  vaddsub4h 0, r0, 0x12345678 

;=== vaddsub4h<.cc> b,b,limm ===

; vaddsub4h<.cc> b,b,limm 00101bbb111110100BBB1111100QQQQQ
;   b.h0 = b.h0 + limm.h0;
;   b.h1 = b.h1 - limm.h1;
;   b.h2 = b.h2 + 0;
;   b.h3 = b.h3 - 0;

; Test case 12.1
; CC=1
; r0 <- r0, limm (cc)
;
;   (0x0001, 0x0001, 0x1234, 0xa988) [r1:r0]
;      -       +       -       +
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;   --------------------------------
;   (0x0001, 0x0001, 0x0000, 0x0000) [r1:r0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00010001, 0x1234a988
  set_carry
  vaddsub4h.c r0, r0, 0x12345678
  clear_carry
  check_64bit_double r1, r0, 0x00010001, 0x00000000

; Test case 12.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_64bit_double r1, r0, 0x00010001, 0x1234a988
  vaddsub4h.c r0, r0, 0x12345678 
  check_64bit_double r1, r0, 0x00010001, 0x1234a988

;=== vaddsub4h<.cc> 0,limm,c ===

; vaddsub4h<.cc> 0,limm,c 00101110111110100111CCCCCC0QQQQQ
;   0 = limm.h0 + c.h0;
;   0 = limm.h1 - c.h1;
;   0 = 0 + c.h2;
;   0 = 0 - c.h3;

; Test case 13.1
; CC=1
; 0 <- limm, r0 (cc)
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;      -       +       -       +
;   (0x0001, 0x0001, 0x1234, 0xa988) [r1:r0]
;   --------------------------------
;   (------, ------, ------, ------) [0:0]
;
  prep_test_case
  set_64bit_double r1, r0, 0x00010001, 0x1234a988
  set_carry
  vaddsub4h.c 0, 0x12345678, r0
  clear_carry

; Test case 13.2
; CC=0
; Instruction is not executed
  prep_test_case
  set_64bit_double r1, r0, 0x00010001, 0x1234a988
  vaddsub4h.c 0, 0x12345678, r0
  
;=== vaddsub4h a,limm,u6 ===

; vaddsub4h a,limm,u6 00101110011110100111uuuuuuAAAAAA
;   a.h0 = limm.h0 + u6;
;   a.h1 = limm.h1 - u6;
;   a.h2 = 0 + u6;
;   a.h3 = 0 - u6;

; Test case 14
; r0 <- limm, u6
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;      -       +       -       +
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6:u6]
;   --------------------------------
;   (0xffc1, 0x003f, 0x11f5, 0x56b7) [r1:r0]
;
  prep_test_case
  vaddsub4h r0, 0x12345678, 0x3f 
  check_64bit_double r1, r0, 0xffc1003f, 0x11f556b7

;=== vaddsub4h 0,limm,u6 ===

; vaddsub4h 0,limm,u6 00101110011110100111uuuuuu111110
;   0 = limm.h0 + u6;
;   0 = limm.h1 - u6;
;   0 = 0 + u6;
;   0 = 0 - u6;

; Test case 15
; 0 <- limm, u6
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;      -       +       -       +
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6:u6]
;   --------------------------------
;   (------, ------, ------, ------) [0:0]
;
  prep_test_case
  vaddsub4h 0, 0x12345678, 0x3f 

;=== vaddsub4h<.cc> 0,limm,u6 ===

; vaddsub4h<.cc> 0,limm,u6 00101110111110100111uuuuuu1QQQQQ
;   0 = limm.h0 + u6;
;   0 = limm.h1 - u6;
;   0 = 0 + u6;
;   0 = 0 - u6;

; Test case 16.1
; CC=1
; 0 <- limm, u6 (cc)
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;      -       +       -       +
;   (  0x3f,   0x3f,   0x3f,   0x3f) [u6:u6]
;   --------------------------------
;   (------, ------, ------, ------) [0:0]
;
  prep_test_case
  set_carry
  vaddsub4h.c 0, 0x12345678, 0x3f
  clear_carry

; Test case 16.2
; CC=0
; Instruction is not executed
  prep_test_case
  vaddsub4h.c 0, 0x12345678, 0x3f

;=== vaddsub4h 0,limm,s12 ===

; vaddsub4h 0,limm,s12 00101110101110100111ssssssSSSSSS
;   0 = limm.h0 + s12;
;   0 = limm.h1 - s12;
;   0 = 0 + s12;
;   0 = 0 - s12;

; Test case 17
; 0 <- limm, s12
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;      -       +       -       +
;   (0xfffe, 0xfffe, 0xfffe, 0xfffe) [s12:s12]
;   --------------------------------
;   (------, ------, ------, ------) [0:0]
;
  prep_test_case
  vaddsub4h 0, 0x12345678, -2   ; -2=0xfffffffe

;=== vaddsub4h a,limm,limm ===

; vaddsub4h a,limm,limm 00101110001110100111111110AAAAAA
;   a.h0 = limm.h0 + limm.h0;
;   a.h1 = limm.h1 - limm.h1;
;   a.h2 = 0 + 0;
;   a.h3 = 0 - 0;

; Test case 18
; r0 <- limm, limm
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;      -       +       -       +
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;   --------------------------------
;   (0x0000, 0x0000, 0x0000, 0xacf0) [r1:r0]
;
  prep_test_case
  vaddsub4h r0, 0x12345678, 0x12345678
  check_64bit_double r1, r0, 0x00000000, 0x0000acf0

;=== vaddsub4h 0,limm,limm ===

; vaddsub4h 0,limm,limm 00101110001110100111111110111110
;   0 = limm.h0 + limm.h0;
;   0 = limm.h1 - limm.h1;
;   0 = 0 + 0;
;   0 = 0 - 0;

; Test case 19
; 0 <- limm, limm
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;      -       +       -       +
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;   --------------------------------
;   (------, ------, ------, ------) [0:0]
;
  prep_test_case
  vaddsub4h 0, 0x12345678, 0x12345678

;=== vaddsub4h<.cc> 0,limm,limm ===

; vaddsub4h<.cc> 0,limm,limm 001011101111101001111111100QQQQQ
;   if (cc) {
;     0 = limm.h0 + limm.h0;
;     0 = limm.h1 - limm.h1;
;     0 = 0 + 0;
;     0 = 0 - 0;
;   }

; Test case 20.1
; CC=1
; 0 <- limm, limm (cc)
;
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;      -       +       -       +
;   (0x0000, 0x0000, 0x1234, 0x5678) [0:limm]
;   --------------------------------
;   (------, ------, ------, ------) [0:0]
;
  prep_test_case
  set_carry
  vaddsub4h.c 0, 0x12345678, 0x12345678
  clear_carry

; Test case 20.2
; CC=0
; Instruction is not executed
  prep_test_case
  vaddsub4h.c 0, 0x12345678, 0x12345678

;=== exceptions ===

; ; Test case 21.1
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; Destination register is an odd register
;   prep_test_case
;   set_except_params @test_21_1_exception, @test_21_1_end
; test_21_1_exception:
;   vaddsub4h r1, r2, r4
;   b     @fail
; test_21_1_end:
;   ; Fall through

; ; Test case 21.2
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; First source register is an odd register
;   prep_test_case
;   set_except_params @test_21_2_exception, @test_21_2_end
; test_21_2_exception:
;   vaddsub4h r0, r1, r4
;   b     @fail
; test_21_2_end:
;   ; Fall through

; ; Test case 21.3
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; Second source register is an odd register
;   prep_test_case
;   set_except_params @test_21_3_exception, @test_21_3_end
; test_21_3_exception:
;   vaddsub4h r0, r2, r5
;   b     @fail
; test_21_3_end:
;   ; Fall through

; ; Test case 21.4
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; Even if there is no register to save the result to.
;   prep_test_case
;   set_except_params @test_21_4_exception, @test_21_4_end
; test_21_4_exception:
;   vaddsub4h 0, r3, r0
;   b     @fail
; test_21_4_end:
;   ; Fall through

; ; Test case 21.5
; ; Raise an Illegal Instruction exception if an odd register is used.
; ; The exception should be made even if the CC indicates no execution.
;   prep_test_case        ; (Z,N,C,V)=(0,0,0,0)
;   set_except_params @test_21_5_exception, @test_21_5_end
; test_21_5_exception:
;   vaddsub4h.z r5, r5, r0
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
  print " vaddsub4h: Quad 16-bit SIMD add and subtract\n"
  end
