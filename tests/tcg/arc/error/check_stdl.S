; check_stdl.S
;
; Tests stdl: the 128-bit store
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

; reg <- (hi,lo)
.macro mov64 reg, hi, lo
  mov     \reg, \lo
  addhl   \reg, \reg, \hi
.endm

; [mem] == (lohi, lolo, hihi, hilo)
.macro check_128bit_mem hihi, hilo, lohi, lolo, mem
  mov64  r11, \hihi, \hilo
  lddl   r12, [\mem]
  xor.f  0, r11, r13
  bne    @fail
  mov64  r11, \lohi, \lolo
  xor.f  0, r11, r12
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; STDL ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start
  
; Test case 1
; r1r0 --> [r2]
  .equ    t01_hihi, 0x1f1e1d1c
  .equ    t01_hilo, 0x1b1a1918
  .equ    t01_lohi, 0x17161514
  .equ    t01_lolo, 0x13121110
  prep_test_case
  mov64   r0, t01_lohi, t01_lolo
  mov64   r1, t01_hihi, t01_hilo
  mov     r2, @first
  stdl    r0, [r2]
  check_128bit_mem  t01_hihi, t01_hilo, t01_lohi, t01_lolo, @first
  xor.f   0, r2, @first
  bne     @fail

; Test case 2
; r1r0 --> [r2, 16]
  .equ    t02_hihi, 0x2f2e2d2c
  .equ    t02_hilo, 0x2b2a2928
  .equ    t02_lohi, 0x27262524
  .equ    t02_lolo, 0x23222120
  prep_test_case
  mov64   r0, t02_lohi, t02_lolo
  mov64   r1, t02_hihi, t02_hilo
  mov     r2, @first
  stdl    r0, [r2, 16]
  check_128bit_mem  t02_hihi, t02_hilo, t02_lohi, t02_lolo, @second
  xor.f   0, r2, @first
  bne     @fail

; Test case 3
; r1r0 --> pre-increment [r2, 16]
  .equ    t03_hihi, 0x3f3e3d3c
  .equ    t03_hilo, 0x3b3a3938
  .equ    t03_lohi, 0x37363534
  .equ    t03_lolo, 0x33323130
  prep_test_case
  mov64   r0, t03_lohi, t03_lolo
  mov64   r1, t03_hihi, t03_hilo
  mov     r2, @second
  stdl.aw r0, [r2, 16]
  check_128bit_mem  t03_hihi, t03_hilo, t03_lohi, t03_lolo, @third
  xor.f   0, r2, @third
  bne     @fail

; Test case 4
; r9r8 --> post-increment [r2, 16]
  .equ    t04_hihi, 0x4f4e4d4c
  .equ    t04_hilo, 0x4b4a4948
  .equ    t04_lohi, 0x47464544
  .equ    t04_lolo, 0x43424140
  prep_test_case
  mov64   r8, t04_lohi, t04_lolo
  mov64   r9, t04_hihi, t04_hilo
  mov     r2, @fourth
  stdl.ab r8, [r2, 16]
  check_128bit_mem  t04_hihi, t04_hilo, t04_lohi, t04_lolo, @fourth
  xor.f   0, r2, @fourth+16
  bne     @fail

; Test case 5
; r7r6 --> address scaling [r4, 4]
  .equ    t05_hihi, 0x5f5e5d5c
  .equ    t05_hilo, 0x5b5a5958
  .equ    t05_lohi, 0x57565554
  .equ    t05_lolo, 0x53525150
  prep_test_case
  mov64   r8, t05_lohi, t05_lolo
  mov64   r9, t05_hihi, t05_hilo
  mov     r4, @first
  stdl.as r8, [r4, 4]
  check_128bit_mem  t05_hihi, t05_hilo, t05_lohi, t05_lolo, @third
  xor.f   0, r4, @first
  bne     @fail

; Test case 6
; r5r4 --> [addr]
  .equ    t06_hihi, 0x6f6e6d6c
  .equ    t06_hilo, 0x6b6a6968
  .equ    t06_lohi, 0x67666564
  .equ    t06_lolo, 0x63626160
  prep_test_case
  mov64   r4, t06_lohi, t06_lolo
  mov64   r5, t06_hihi, t06_hilo
  stdl    r4, [@first]
  check_128bit_mem  t06_hihi, t06_hilo, t06_lohi, t06_lolo, @first

; Test case 7
; limm --> address scale of 8, [r1, 2]
  .equ    t07_val, 0x77777777
  prep_test_case
  mov     r1, @first
  stdl.as t07_val, [r1, 2]
  check_128bit_mem  0x0, 0x0, 0x0, t07_val, @second
  xor.f   0, r1, @first
  bne     @fail

; Test case 8
; ximm --> pre-increment [r0, 7]
  .equ    t08_val, -8
  prep_test_case
  mov     r0, @second-7
  stdl.a  t08_val, [r0, 7]
  check_128bit_mem  -1, -1, -1, t08_val, @second
  xor.f   0, r0, @second
  bne     @fail

; Test case 9
; r3r2 --> [r2, 16]                   same source and dest
  .equ   t09_hihi, 0x99999999
  .equ   t09_hilo, 0x98989898
  prep_test_case
  mov    r2, @second
  mov64  r3, t09_hihi, t09_hilo
  stdl   r2, [r2, 16]
  check_128bit_mem  t09_hihi, t09_hilo, 0, @second, @third
  xor.f  0, r2, @second
  bne    @fail

; Test case 10
; r5r4 --> pre-increment [r4, 16]         same source and dest
; the data to be written is whatever was in "r4" before any increment
  prep_test_case
  .equ    t10_hihi, 0x10101010
  .equ    t10_hilo, 0x01010101
  mov64   r5, t10_hihi, t10_hilo
  mov     r4, @third
  stdl.a  r4, [r4, 16]
  check_128bit_mem  t10_hihi, t10_hilo, 0, @third, @fourth
  xor.f   0, r4, @fourth
  bne     @fail

; Test case 11
; r3r2 --> post-increment [r3, 32]        same source and dest
; the data to be written is whatever was in "r3" before any increment
  prep_test_case
  .equ    t11_lohi, 0x11111111
  .equ    t11_lolo, 0x12111112
  mov64   r2, t11_lohi, t11_lolo
  mov     r3, @first
  stdl.ab r2, [r3, 32]
  check_128bit_mem  0, @first, t11_lohi, t11_lolo, @first
  xor.f   0, r3, @third
  bne     @fail

; Test case 12
; r1r0 --> address scale of 8, [r1, 32]   same source and dest
  prep_test_case
  .equ    t12_lohi, 0x12121212
  .equ    t12_lolo, 0x21212121
  mov64   r0, t12_lohi, t12_lolo
  mov     r1, @first
  stdl.as r0, [r1, 6]
  check_128bit_mem  0, @first, t12_lohi, t12_lolo, @fourth
  xor.f   0, r1, @first
  bne     @fail

; Test case 13
; stdl r1 (odd) ... --> Raise an illegal instruction exception AND
;                       the base pointer should not be updated
; N.B. if ivt != 0, have the interrupt vector base adjusted
  prep_test_case
  set_except_params @test_13_exception, @test_13_end
  .equ    t13_hihi, 0x13133333
  .equ    t13_hilo, 0x13132222
  .equ    t13_lohi, 0x13131111
  .equ    t13_lolo, 0x13130000
  mov     r2, @fourth
  mov64   r0, t13_lohi, t13_lolo
  mov64   r1, t13_hihi, t13_hilo
  stdl    r0, [r2]
test_13_exception:
  ; stdl.aw r5, [r2, 4]
  .byte   0x04,0x1a,0x67,0x01
  b       @fail
test_13_end:
  xor.f   0, r2, @fourth
  bne     @fail
  lddl    r2, [@fourth]
  xor.f   0, r2, r0
  bne     @fail
  xor.f   0, r3, r1
  bne     @fail

; Test case 14
; stdl.aw r1, [imm,s9]) ... --> Raise an illegal instruction exception
  prep_test_case
  set_except_params @test_14_exception, @test_14_end
  .equ    t14_hihi, 0x14143333
  .equ    t14_hilo, 0x14142222
  .equ    t14_lohi, 0x14141111
  .equ    t14_lolo, 0x14140000
  mov64   r2, t14_lohi, t14_lolo
  mov64   r3, t14_hihi, t14_hilo
test_14_exception:
  ; stdl.aw r2, [0, 0]
  .byte   0x00,0x1e,0xaf,0x70, 0x00,0x00,0x00,0x00
  b       @fail
test_14_end:
  lddl    r0, [0x00]
  xor.f   0, r0, r2
  beq     @fail
  xor.f   0, r1, r3
  beq     @fail

; Test case 15
; 1 --> [r1]
  .equ    t15_val, 0x01
  prep_test_case
  mov     r1, @first
  stdl    t15_val, [r1]
  check_128bit_mem  0x0, 0x0, 0x0, t15_val, @first
  xor.f   0, r1, @first
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
  print " stdl: the 128-bit store\n"
  end
