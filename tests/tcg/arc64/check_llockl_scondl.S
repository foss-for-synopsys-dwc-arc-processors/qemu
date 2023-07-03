; check_llockl_scondl.S
;
; Tests for llockl and scondl
; If the test fails, check the end of this file for how to troubleshoot.

  .include "macros.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;; Test checking routines ;;;;;;;;;;;;;;;;;;;;;;;;;;

; Test case counter
.data
test_nr:
  .word 0x0
.align 8
some_data_1:
	.word 0x00000000
	.word 0x00000000
.align 8
some_data_2:
	.word 0x00000000
	.word 0x00000000

.equ  data_1, 0x01234567
.equ  data_2, 0x89abcdef

.equ  data_11, 0x01234567
.equ  data_12, 0xaabbccdd
.equ  data_21, 0x89abcdef
.equ  data_22, 0xeeff0011

; Increment the test counter and set (Z,N,C,V) to (0,0,0,0).
.macro prep_test_case
  ; Reset data
  set_reg64 r0 data_11 data_12
  stl  r0, [@some_data_1]
  set_reg64 r0 data_21 data_22
  stl  r0, [@some_data_2]

  ldl    r13, [test_nr]
  addl   r13, r13, 1       ; increase test case counter
  stl   r13, [test_nr]
  addl.f 0, 0, 1           ; (Z,N,C,V) = (0,0,0,0)
.endm

.macro chk_reg32 reg, val
  xor.f 0, \reg, \val
  bne    @fail
.endm

; reg == 64bit(hi, lo) AND reg == acc
; Clobbers r11
.macro chk_reg64 reg, hi, lo
  movhl  r11, \hi
  orl    r11, r11,  \lo
  xorl.f 0,   \reg, r11
  bne    @fail
.endm

; reg = 64bit(hi, lo)
.macro set_reg64 reg, hi, lo
  movhl \reg, \hi
  orl   \reg, \reg, \lo
.endm
.macro clr_flags
	add.f 0, 1, 0
.endm

; Bit positions for (Z,N,C,V) in STATUS32 register
.equ REG_STAT_V_POS,  8
.equ REG_STAT_C_POS,  9
.equ REG_STAT_N_POS, 10
.equ REG_STAT_Z_POS, 11

; (Z,N,C,V) = (0,1,0,0)
.macro set_n_flag
  add.f 0, -1, 0  ; negative result
.endm

; (Z,N,C,V) = (0,1,0,1)
.macro set_nv_flags
  add.f 0, 0x7fffffff, 1  ; negative result with an overflow
.endm


; The is_x_flag_set, macros use "b @fail" instead of
; "bbit ..., @fail" to support longer range

; (Z,N,C,V) == (1,0,0,0)
.macro is_z_flag_set
  lr    r11, [status32]
  bbit0 r11, REG_STAT_Z_POS, @321f
  bbit1 r11, REG_STAT_N_POS, @321f
  bbit1 r11, REG_STAT_C_POS, @321f
  bbit1 r11, REG_STAT_V_POS, @321f
  b     @123f
321:
  b     @fail
123:
  ; Fall through
.endm


; (Z,N,C,V) == (0,1,0,0)
.macro is_n_flag_set
  lr    r11, [status32]
  bbit1 r11, REG_STAT_Z_POS, @321f
  bbit0 r11, REG_STAT_N_POS, @321f
  bbit1 r11, REG_STAT_C_POS, @321f
  bbit1 r11, REG_STAT_V_POS, @321f
  b     @123f
321:
  b     @fail
123:
  ; Fall through
.endm

; (Z,N,C,V) == (0,0,0,1)
.macro is_v_flag_set
  lr    r11, [status32]
  bbit1 r11, REG_STAT_Z_POS, @321f
  bbit1 r11, REG_STAT_N_POS, @321f
  bbit1 r11, REG_STAT_C_POS, @321f
  bbit0 r11, REG_STAT_V_POS, @321f
  b     @123f
321:
  b     @fail
123:
  ; Fall through
.endm

; (Z,N,C,V) == (0,1,0,1)
.macro is_nv_flags_set
  lr    r11, [status32]
  bbit1 r11, REG_STAT_Z_POS, @321f
  bbit0 r11, REG_STAT_N_POS, @321f
  bbit1 r11, REG_STAT_C_POS, @321f
  bbit0 r11, REG_STAT_V_POS, @321f
  b     @123f
321:
  b     @fail
123:
  ; Fall through
.endm

; (Z,N,C,V) == (0,0,0,0)
.macro is_no_flag_set
  lr    r11, [status32]
  bbit1 r11, REG_STAT_Z_POS, @321f
  bbit1 r11, REG_STAT_N_POS, @321f
  bbit1 r11, REG_STAT_C_POS, @321f
  bbit1 r11, REG_STAT_V_POS, @321f
  b     @123f
321:
  b     @fail
123:
  ; Fall through
.endm
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; LLOCKL/SCONDL ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test 1
; scondl without previous llockl (check initial CPU conditions), should not work
prep_test_case
  mov r0, @some_data_1
; load instead of llock, so no flag has been set since boot
  ldl r1, [r0]
  chk_reg64 r1, data_11, data_12

  add r1, r1, 1
  clr_flags
; scondl wont work
  scondl r1, [r0]
  is_no_flag_set
; nothing changed (scondl failed)
  ldl r2, [r0]
  chk_reg64 r2, data_11, data_12

; Test 2
; llockl and scondl to the same address
; Expected to work
prep_test_case
  movl r0, @some_data_1
  llockl r1, [r0]
  chk_reg64 r1, data_11, data_12

  addl r1, r1, 1
  clr_flags
  scondl r1, [r0]
  is_z_flag_set

  ldl r2, [r0]
  chk_reg64 r2, data_11 , (data_12 + 1)

; Test 3
; 2 equivalent llocks and scondl to the same address
; Expected to work
prep_test_case

  movl r0, @some_data_1
  llockl r1, [r0]

  chk_reg64 r1, data_11, data_12

  ; Second llockl for the same address
  llockl r1, [r0]

  addl r1, r1, 1
  clr_flags
  scondl r1, [r0]
  is_z_flag_set

  ldl r2, [r0]
  chk_reg64 r2, data_11 , (data_12 + 1)

; Test 4
; 2 llocks for different addresses, scondl to first llocks' address shouldnt work
prep_test_case

  movl r0, @some_data_1
  movl r3, @some_data_2

  llockl r1, [r0]
  chk_reg64 r1, data_11, data_12

  ; Second llockl for different address
  llockl r4, [r3]
  chk_reg64 r4, data_21, data_22

  addl r1, r1, 1
  clr_flags
; scondl wont work
  scondl r4, [r0]
  is_no_flag_set

  ldl r2, [r0]
  chk_reg64 r2, data_11 , data_12

; Test 5
; 2 llocks for different addresses, scondl to second (last) llocks' address should work
prep_test_case

  movl r0, @some_data_1
  movl r3, @some_data_2

  llockl r1, [r0]
  chk_reg64 r1, data_11, data_12

  ; Second llockl for different address
  llockl r4, [r3]
  chk_reg64 r4, data_21, data_22

  addl r1, r1, 1
  clr_flags
; scondl address matches latest llockl so it should work
  scondl r1, [r3]
  is_z_flag_set

  ldl r2, [r3]
  chk_reg64 r2, data_11 , (data_12 + 1)

; Test 6
; 2 llocks for different addresses, with matching sconds
; interleaved order means both sconds fail
prep_test_case

  movl r0, @some_data_1
  movl r3, @some_data_2

  llockl r1, [r0]
  chk_reg64 r1, data_11, data_12

  ; Second llockl for different address
  llockl r4, [r3]
  chk_reg64 r4, data_21, data_22

  addl r1, r1, 1

  clr_flags
; scondl address does not match latest llockl so it should not work
  scondl r1, [r0]
  is_no_flag_set

  clr_flags
; previous scondl unset LF, so this one should not work
  scondl r1, [r3]
  is_no_flag_set

; And nothing changed
  ldl r2, [r0]
  chk_reg64 r2, data_11, data_12

  ldl r2, [r3]
  chk_reg64 r2, data_21, data_22

; Test 7
; llockl, data changes, scondl fails
prep_test_case

  movl r0, @some_data_1

  llockl r1, [r0]
  chk_reg64 r1, data_11, data_12

  addl r1, r1, 1

  ; Random store
  set_reg64 r4, 0x12344321, 0x56788765
  stl  r4, [r0]

  clr_flags
  ; Data was written to the address so the scondl must fail
  scondl r1, [r0]
  is_no_flag_set

  ; And scondl changed nothing
  ldl r2, [r0]
  chk_reg64 r2, 0x12344321, 0x56788765

;  For the sake of speed, QEMU assumes that if the data hasn't changed, there
; were no stores. In general this is a good supposition, but it may not hold up
;  The test below are commented because they validate these cases,  which Qemu
; currently DOES NOT SUPPORT

j @valhalla

; Test 8
; llockl, there is a write to the addres but data doesnt change, scondl fails
prep_test_case

  movl r0, @some_data_1

  llockl r1, [r0]
  chk_reg64 r1, data_11, data_12

  ; Redundant store
  stl  r1, [r0]

  add r1, r1, 1

  clr_flags
  ; Data was written to the address so the scondl must fail
  scondl r1, [r0]
  is_no_flag_set

  ; And scondl changed nothing
  ldl r2, [r0]
  chk_reg64 r2, data_11, data_12

; Test 9
; llockl, data changes to different value, and then changes it back, scondl should
; still fail
prep_test_case

  movl r0, @some_data_1

  llockl r1, [r0]
  chk_reg64 r1, data_11, data_12

  ; Random store
  set_reg64 r4, 0x12344321, 0x56788765
  stl  r4, [r0]

  ; Restore data
  stl  r1, [r0]

  addl r1, r1, 1

  clr_flags
  ; Data was reset in the address but the scondl must fail
  scondl r1, [r0]
  is_no_flag_set

  ; And nothing changed
  ldl r2, [r0]
  chk_reg64 r2, data_11, data_12


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Reporting ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

valhalla:
  print "[PASS]"
  mov r0, 0xdecaf
  b @1f

; If a test fails, it jumps here. Although, for the sake of uniformity,
; the printed output does not say much about which test case failed,
; one can uncomment the print_number line below or set a breakpoint
; here to check the R0 register for the test case number.
fail:
  ld r0, [test_nr]
  ;print_number r0
  print "[FAIL]"
  mov r0, 0xbadcafe
1:
  print " llockl/scondl\n"
  flag 1
  end


