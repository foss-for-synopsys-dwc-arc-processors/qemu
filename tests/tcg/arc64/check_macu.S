; check_macu.S
;
; Tests for macu: Unsigned 32x32 multiplication and accumulation.
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

; reg64(hi, low) = (hi, low)
.macro  set_reg64       reg, hi, low
  movhl \reg, \hi
  orl \reg, \reg, \low
.endm

; reg64(hi, low) == (hi, low)
.macro  check_reg64     reg, hi, low
  movhl  r11, \hi
  orl    r11, r11, \low
  xorl.f 0, r11, \reg
  bne    @fail
.endm

; all Z, N, C, V flags are cleared
.macro clear_flags
  ; clearing the Z N C V flags
  mov r11, 1
  add.f r11, r11, r11
.endm

; checks if Z, N, C, and V flags are set correctly
.macro verify_flags z=0, n=0, c=0, v=0, test_num
  assert_flag REG_STAT_Z, \z, \test_num
  assert_flag REG_STAT_N, \n, \test_num
  assert_flag REG_STAT_C, \c, \test_num
  assert_flag REG_STAT_V, \v, \test_num
.endm

.macro set_flag reg_stat_flag
  lr     r11, [REG_STAT]
  or     r11, r11, \reg_stat_flag
  flag   r11 
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; MACU ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; operation: 
;   result = acc + (b * c)
;          = 0 + (4 * 5) = 20 (0x00000000_00000014)
;   a = result.w0 = 20 (0x00000014)
;   acc = result = 20 (0x00000000_00000014)
;   flags: v=0

  prep_test_case
  ; initializing data
  set_reg64 r58, 0, 0
  set_reg64 r0, 0, 4
  set_reg64 r1, 0, 5
  clear_flags
  ; operation under test
  macu.f r0, r0, r1
  ; checking the results
  verify_flags test_num=1
  check_reg64 r58, 0, 20
  check_reg64 r0, 0, 20
  

; operation:
;   result = acc + (b * c)
;          = 20 + (5 * 1) = 25 (0x00000000_00000019)
;   a = result.w0 = 25 (0x00000019)
;   acc = result = 25 (0x00000000_00000019)
;   flags: v=0

  prep_test_case
  ; initializing data
  set_reg64 r58, 0, 20
  set_reg64 r0, 0, 5
  set_reg64 r1, 0, 1
  clear_flags
  ; operation under test
  macu.f r0, r0, r1
  ; checking the results
  verify_flags test_num=2
  check_reg64 r58, 0, 25
  check_reg64 r0, 0, 25


; operation:
;   result = acc + (b * c)
;          = 25 + (0x00000000_ffffffff * 5)
;          = 25 + 21,474,836,475
;          = 21,474,836,500 (0x00000005_00000014)
;   a = result.w0 = 20 (0x00000014)
;   acc = result = 21,474,836,500 (0x00000005_00000014)
;   flags: v=0

  prep_test_case
  ; initializing data
  set_reg64 r58, 0, 25
  set_reg64 r0, 0x0, 0xffffffff
  set_reg64 r1, 0, 5
  clear_flags
  ; operation under test
  macu.f r0, r0, r1
  ; checking the results
  verify_flags test_num=3
  check_reg64 r58, 5, 20
  check_reg64 r0, 0, 20


; operation:
;   result = acc + (b * c) 
;          = 21,474,836,500 + (0x00000000_fffffffd * 9)
;          = 21,474,836,500 + 38,654,705,637
;          = 60,129,542,137 (0x0000000d_fffffff9)
;   a = result.w0 = 4,294,967,289 (0xfffffff9)
;   acc = result = 60,129,542,137 (0x0000000d_fffffff9)
;   flags: v=0

  prep_test_case
  ; initializing data
  set_reg64 r58, 5, 20
  set_reg64 r0, 0x0, 0xfffffffd
  set_reg64 r1, 0, 9
  clear_flags
  ; operation under test
  macu.f r0, r0, r1
  ; checking the results
  verify_flags test_num=4
  check_reg64 r58, 0x0d, 0xfffffff9
  check_reg64 r0, 0, 0xfffffff9


; producing a result that sets both acch and accl
; operation:
;   result = acc + (b * c)
;          = 0 + (0x00000000_7fffffff * 0x00000000_7fffffff)
;          = 0x3fffffff_00000001
;   a = result.w0 = 0x00000001
;   acc = result = 0x3fffffff_00000001
;   flags: v=0

  prep_test_case
  ; initializing data
  set_reg64 r58, 0, 0
  set_reg64 r0, 0, 0x7fffffff
  set_reg64 r1, 0, 0x7fffffff
  clear_flags
  ; operation under test
  macu.f r0, r0, r1
  ; checking the results
  verify_flags test_num=5
  check_reg64 r58, 0x3fffffff, 0x1
  check_reg64 r0, 0, 0x1


; acc is 0x3fffffff_00000001
; operation:
;   result = acc + (b * c)
;          = 0x3fffffff_00000001 + (0x00000000_80000000 * 0x00000000_80000000)
;          = 0x7fffffff_00000001
;   a = result.w0 = 0x00000001
;   acc = result = 0x7fffffff_00000001
;   flags: v=0

  prep_test_case
  ; initializing data
  set_reg64 r58, 0x3fffffff, 0x1
  set_reg64 r0, 0, 0x80000000
  set_reg64 r1, 0, 0x80000000
  clear_flags
  ; operation under test
  macu.f r0, r0, r1
  ; checking the results
  verify_flags test_num=6
  check_reg64 r58, 0x7fffffff, 0x1
  check_reg64 r0, 0, 0x1


; acc is 0x7fffffff_00000001
; operation:
;   result = acc + (b * c)
;          = 0x7fffffff_00000001 + (0x00000000_12344321 * 0x00000000_56788654)
;          = 0x7fffffff_00000001 + 0x6262099_e1c14cd4
;          = 0x86262098_e1c14cd5
;   a = result.w0 = 0xe1c14cd5
;   acc = result = 0x86262098_e1c14cd5
;   flags: v=0

  prep_test_case
  ; initializing data
  set_reg64 r58, 0x7fffffff, 0x1
  set_reg64 r0, 0, 0x12344321
  set_reg64 r1, 0, 0x56788654
  clear_flags
  ; operation under test
  macu.f r0, r0, r1
  ; checking the results
  verify_flags v=0 test_num=7
  check_reg64 r58, 0x86262098, 0xe1c14cd5
  check_reg64 r0, 0, 0xe1c14cd5


; cause an overflow
; operation:
;   result = acc + (b * c)
;          = 0xffffffff_ffffffff + (1 * 1)
;          = 0xffffffff_ffffffff + 1
;          = 0x1_00000000_00000000 => 0x00000000_00000000
;   a = result.w0 = 0x00000000
;   acc = result = 0x00000000_00000000
;   flags: v=1

  prep_test_case
  ; initializing data
  set_reg64 r58, 0xffffffff, 0xffffffff
  set_reg64 r0, 0, 0x1
  set_reg64 r1, 0, 0x1
  clear_flags
  ; operation under test
  macu.f r0, r0, r1
  ; checking the results
  verify_flags v=1 test_num=8
  check_reg64 r58, 0, 0
  check_reg64 r0, 0, 0


; cause an overflow, (without .f) => v flag unchanged
; operation:
;   result = acc + (b * c)
;          = 0xffffffff_ffffffff + (1 * 1)
;          = 0xffffffff_ffffffff + 1
;          = 0x1_00000000_00000000 => 0x00000000_00000000
;   a = result.w0 = 0x00000000
;   acc = result = 0x00000000_00000000
;   flags: unchanged

  prep_test_case
  ; initializing data
  set_reg64 r58, 0xffffffff, 0xffffffff
  set_reg64 r0, 0, 0x1
  set_reg64 r1, 0, 0x1
  clear_flags
  ; operation under test
  macu r0, r0, r1
  ; checking the results
  verify_flags test_num=9
  check_reg64 r58, 0, 0
  check_reg64 r0, 0, 0


; "macu" is not supposed to clear the overflow bit
; precondition: v=1 
; operation:
;   result = acc + (b * c)
;          = 0 + (0 * 0)
;          = 0
;   a = result.w0 = 0x00000000
;   acc = result = 0x00000000_00000000
;   flags: v=1

  prep_test_case
  ; initializing data
  set_reg64 r58, 0, 0
  set_reg64 r0, 0, 0
  set_reg64 r1, 0, 0
  clear_flags
  set_flag REG_STAT_V
  ; operation under test
  verify_flags v=1 test_num=10
  macu.f r0, r0, r1
  ; checking the results
  verify_flags v=1 test_num=10
  check_reg64 r58, 0, 0
  check_reg64 r0, 0, 0

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
  print " macu: Unsigned 32x32 multiplication and accumulation\n"
  end
