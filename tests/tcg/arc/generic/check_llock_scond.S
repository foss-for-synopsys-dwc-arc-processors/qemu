; check_llock_scond.S
;
; Tests for llock and scond
; If the test fails, check the end of this file for how to troubleshoot.

  .include "macros.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;; Test checking routines ;;;;;;;;;;;;;;;;;;;;;;;;;;

; Test case counter
.data
test_nr:
  .word 0x0
some_data_1:
	.word 0x00000000
some_data_2:
	.word 0x00000000

.equ  data_1, 0x01234567
.equ  data_2, 0x89abcdef

; Increment the test counter and set (Z,N,C,V) to (0,0,0,0).
.macro prep_test_case
  ; Reset data
  mov r0, data_1
  st  r0, [@some_data_1]
  mov r0, data_2
  st  r0, [@some_data_2]

  ld    r13, [test_nr]
  add_s r13, r13, 1       ; increase test case counter
  st    r13, [test_nr]
  add.f 0, 0, 1           ; (Z,N,C,V) = (0,0,0,0)
.endm

; reg1 = lo, reg2 = hi
.macro set_reg64 reg1, reg2, hi, lo
  mov   \reg1, \lo
  mov   \reg2, \hi
.endm

; reg1 == hi, reg2 == lo AND acchi == hi, acclo == lo
.macro chk_reg64 reg1, reg2, hi, lo
  xor.f 0, \reg1, \lo
  bne    @fail
  xor.f 0, \reg2, \hi
  bne    @fail
.endm

.macro chk_reg32 reg, val
  xor.f 0, \reg, \val
  bne    @fail
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
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; LLOCK/SCOND ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test 1
; scond without previous llock (check initial CPU conditions), should not work
prep_test_case
  mov r0, @some_data_1
; load instead of llock, so no flag has been set since boot
  ld r1, [r0]
  chk_reg32 r1, data_1

  add r1, r1, 1
  clr_flags
; scond wont work
  scond r1, [r0]
  is_no_flag_set
; nothing changed (scond failed)
  ld r2, [r0]
  chk_reg32 r2, data_1

; Test 2
; llock and scond to the same address
; Expected to work
prep_test_case
  mov r0, @some_data_1
  llock r1, [r0]

  chk_reg32 r1, data_1

  add r1, r1, 1
  clr_flags
  scond r1, [r0]
  is_z_flag_set

  ld r2, [r0]
  chk_reg32 r2, (data_1 + 1)

; Test 3
; 2 equivalent llocks and scond to the same address
; Expected to work
prep_test_case

  mov r0, @some_data_1
  llock r1, [r0]

  chk_reg32 r1, data_1

  ; Second llock for the same address
  llock r1, [r0]

  add r1, r1, 1
  clr_flags
  scond r1, [r0]
  is_z_flag_set

  ld r2, [r0]
  chk_reg32 r2, (data_1 + 1)

; Test 4
; 2 llocks for different addresses, scond to first llocks' address shouldnt work
prep_test_case

  mov r0, @some_data_1
  mov r3, @some_data_2

  llock r1, [r0]
  chk_reg32 r1, data_1

  ; Second llock for different address
  llock r4, [r3]
  chk_reg32 r4, data_2

  add r1, r1, 1
  clr_flags
; scond wont work
  scond r1, [r0]
  is_no_flag_set

  ld r2, [r0]
  chk_reg32 r2, data_1

; Test 5
; 2 llocks for different addresses, scond to second (last) llocks' address should work
prep_test_case

  mov r0, @some_data_1
  mov r3, @some_data_2

  llock r1, [r0]
  chk_reg32 r1, data_1

  ; Second llock for different address
  llock r4, [r3]
  chk_reg32 r4, data_2

  add r1, r1, 1
  clr_flags
; scond address matches latest llock so it should work
  scond r1, [r3]
  is_z_flag_set

  ld r2, [r3]
  chk_reg32 r2, (data_1 + 1)

; Test 6
; 2 llocks for different addresses, with matching sconds
; interleaved order means both sconds fail
prep_test_case

  mov r0, @some_data_1
  mov r3, @some_data_2

  llock r1, [r0]
  chk_reg32 r1, data_1

  ; Second llock for different address
  llock r4, [r3]
  chk_reg32 r4, data_2

  add r1, r1, 1

  clr_flags
; scond address does not match latest llock so it should not work
  scond r1, [r0]
  is_no_flag_set

  clr_flags
; previous scond unset LF, so this one should not work
  scond r1, [r3]
  is_no_flag_set

; And nothing changed
  ld r2, [r0]
  chk_reg32 r2, data_1

  ld r2, [r3]
  chk_reg32 r2, data_2

; Test 7
; llock, data changes, scond fails
prep_test_case

  mov r0, @some_data_1

  llock r1, [r0]
  chk_reg32 r1, data_1

  add r1, r1, 1

  ; Random store
  mov r4, 0x12344321
  st  r4, [r0]

  clr_flags
  ; Data was written to the address so the scond must fail
  scond r1, [r0]
  is_no_flag_set

  ; And scond changed nothing
  ld r2, [r0]
  chk_reg32 r2, 0x12344321

;  For the sake of speed, QEMU assumes that if the data hasn't changed, there
; were no stores. In general this is a good supposition, but it may not hold up
;  The test below are commented because they validate these cases,  which Qemu
; currently DOES NOT SUPPORT

j @valhalla

; Test 8
; llock, there is a write to the addres but data doesnt change, scond fails
prep_test_case

  mov r0, @some_data_1

  llock r1, [r0]
  chk_reg32 r1, data_1

  ; Redundant store
  mov r1, data_1
  st  r1, [r0]

  add r1, r1, 1

  clr_flags
  ; Data was written to the address so the scond must fail
  scond r1, [r0]
  is_no_flag_set

  ; And scond changed nothing
  ld r2, [r0]
  chk_reg32 r2, data_1

; Test 9
; llock, data changes to different value, and then changes it back, scond should
; still fail
prep_test_case

  mov r0, @some_data_1

  llock r1, [r0]
  chk_reg32 r1, data_1

  ; Random store
  mov r4, 0x12344321
  st  r4, [r0]

  ; Restore data
  st  r1, [r0]

  add r1, r1, 1

  clr_flags
  ; Data was reset in the address but the scond must fail
  scond r1, [r0]
  is_no_flag_set

  ; And nothing changed
  ld r2, [r0]
  chk_reg32 r2, data_1


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
  print " llock/scond\n"
  flag 1
  end
