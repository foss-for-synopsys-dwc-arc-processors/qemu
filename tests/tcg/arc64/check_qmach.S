; check_qmach.S
;
; Tests for qmach: Quad unsigned 16x16 multiply and accumulate.
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

; branch to exit if the expected condition isnt met
; TODO also validate that QMACH does NOT change any other flags but overflow
; less important because the code to do so simply isnt there but still
; should be checked
.macro check_flgs   expected_cond
  b\expected_cond    @1f
  j @fail
  1:
.endm


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; QMACH ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;=== qmach a,b,c ===
;
; Test 1
; Encoding: qmach<.f> a,b,c 00101bbb00110100FBBBCCCCCCAAAAAA
;
; The following qmach tests rely on:
; setting acc to some value
; using an instruction (qmach, qmach.f, qmac.cc, qmac.f.cc) with
; the given data, and checking if both accumulator and r0
; have the expected data
;
.macro reset_flags
  add.f 0, 0, 1       ; (Z, N, C, V) = (0, 0, 0, 0)
.endm

.macro run_test1X  insn, \
                  acch, accl, \
                  r2h, r2l,   \
                  r4h, r4l,   \
                  exph, expl
  prep_test_case
  set_reg64 r58, \acch, \accl ; Set accumulator
  set_reg64 r2, \r2h, \r2l
  set_reg64 r4, \r4h, \r4l
  \insn r0, r2, r4
  check_reg64 r0, \exph, \expl
  check_reg64 r58, \exph, \expl
  ; Run instruction again to reset flags
  set_reg64 r58, \acch, \accl ; Set accumulator
  set_reg64 r2, \r2h, \r2l
  set_reg64 r4, \r4h, \r4l
  \insn r0, r2, r4
.endm
; 
; Test 1.1
;
; a = 0 + (b.h0 * c.h0) + (b.h1 * c.h1) + (b.h2 * c.h2) + (b.h3 * c.h3)
; acc = a
;
; Validate extraction, sum and multiply by setting bits at the beginning and
; end of the half words, WITHOUT accumulator (set to 0)
;
.macro run_test11   r2h, r2l, r4h, r4l, exph, expl
  reset_flags
  run_test1X qmach, 0x0, 0x0, \r2h, \r2l, \r4h, \r4l, \exph, \expl
.endm

; Basic test, 1 half word non 0 on both registers (1*2 + 0 + 0 + 0)
run_test11 0x0, 0x2, \
           0x0, 0x1, \
           0x0, 0x2

; Basic test, 2 half words non 0 on both registers (1*1 + 1*1 + 0 + 0)
run_test11 0x1, 0x1, \
           0x1, 0x1, \
           0x0, 0x2

; Basic test, 4 half words non 0 on both registers (1*1 + 1*1 + 1*1 + 1*1)
run_test11 0x10001, 0x10001, \
           0x10001, 0x10001, \
           0x0,     0x4

; Test first and last bit of each half word (0x8001 + 0x8001 + 0x8001 + 0x8001)
; Also test overflow from one half word into another one
;
;   (0x8001, 0x8001, 0x8001, 0x8001) [r2]
;       *       *       *       *
;   (0x0001, 0x0001, 0x0001, 0x0001) [r4]
;   --------------------------------
;   (0x0000, 0x0000, 0x0002, 0x0604) [r0]
;   (0x0000, 0x0000, 0x0002, 0x0604) [acc (r58)]
;
run_test11 0x80018001, 0x80018001, \
           0x10001,    0x10001,    \
           0xffffffff, 0xfffe0004
           
; Swap registers
run_test11 0x10001,    0x10001,    \
           0x80018001, 0x80018001, \
           0xffffffff, 0xfffe0004
;
; Test 1.2
;
; a = acc + (b.h0 * c.h0) + (b.h1 * c.h1) + (b.h2 * c.h2) + (b.h3 * c.h3)
; acc = a
;
; Validate extraction, sum and multiply by setting bits at the beginning and
; end of the half words, WITH accumulator
;
.macro run_test12   acch, accl, r2h, r2l, r4h, r4l, exph, expl
  reset_flags
  run_test1X qmach, \acch, \accl, \r2h, \r2l, \r4h, \r4l, \exph, \expl
.endm

run_test12 0x0, 0x2, \
           0x0, 0x0, \
           0x0, 0x0, \
           0x0, 0x2

run_test12 0x80018001, 0x80018001, \
           0x0,        0x0,        \
           0x0,        0x0,        \
           0x80018001, 0x80018001

run_test12 0x2, 0x1, \
           0x0, 0x1, \
           0x0, 0x0, \
           0x2, 0x1
;
; Test first and last bit of each half word (0x8001 + 0x8001 + 0x8001 + 0x8001)
;
;   (0x8001, 0x8001, 0x8001, 0x8001) [r2]
;       *       *       *       *
;   (0x0001, 0x0001, 0x0001, 0x0001) [r4]
;   --------------------------------
;   (0x0000, 0x0000, 0x0002, 0x0604) intermediate
;       +       +       +       +
;   (0x8001, 0x8001, 0x8001, 0x8001) [acc (r58)]
;   --------------------------------
;   (0x8001, 0x8001, 0x8003, 0x8005) [r0]
;   (0x8001, 0x8001, 0x8003, 0x8005) [acc (r58)]
;
run_test12 0x80018001, 0x80018001, \
           0x10001,    0x10001,    \
           0x80018001, 0x80018001, \
           0x80018001, 0x7fff8005

; Test 1.3
; 
; a = acc + (b.h0 * c.h0) + (b.h1 * c.h1) + (b.h2 * c.h2) + (b.h3 * c.h3)
;
; Validate flag set
;
.macro run_test13   acch, accl, r2h, r2l, r4h, r4l, exph, expl
  reset_flags
  run_test1X qmach.f, \acch, \accl, \r2h, \r2l, \r4h, \r4l, \exph, \expl
.endm

run_test13 0xffffffff, 0xffffffff, \
           0x0, 0x0, \
           0x0, 0x0, \
           0xffffffff, 0xffffffff
; Check flags
check_flgs vc
check_flgs n


run_test13 0xffffffff, 0xfffffffe, \
           0x0, 0x1, \
           0x0, 0x1, \
           0xffffffff, 0xffffffff
; Check flags
check_flgs vc
check_flgs n

; Closest accumulator value without overflow
run_test13 0x7fffffff, 0xfffffffe, \
           0x0, 0x01, \
           0x0, 0x01, \
           0x7fffffff, 0xffffffff
; Check flags
check_flgs vc
check_flgs p

; Accumulator overflow from largest positive to largest negative
run_test13 0x7fffffff, 0xffffffff, \
           0x0, 0x01, \
           0x0, 0x01, \
           0x80000000, 0x0
; Check flags
check_flgs vs
check_flgs n

; Accumulator "underflow" from smallest negative to smallest positive
run_test13 0xffffffff, 0xffffffff, \
           0x0, 0x01, \
           0x0, 0x01, \
           0x0, 0x0
; Check flags
check_flgs vc
check_flgs p



; Biggest overflow possible;
run_test13 0xffffffff, 0xffffffff, \
           0xffffffff, 0xffffffff, \
           0xffffffff, 0xffffffff, \
           0x00000000, 0x00000003
; Check flags
check_flgs vc
check_flgs p

run_test13 0x0, 0x0, \
           0xffffffff, 0xffffffff, \
           0xffffffff, 0xffffffff, \
           0x0,        0x4
; Check flags
check_flgs vc
check_flgs p

; Overflow with smallest accumulator value
run_test13 0x7fffffff, 0xfffffffc, \
           0xffffffff, 0xffffffff, \
           0xffffffff, 0xffffffff, \
           0x80000000, 0x0
; Check flags
check_flgs vs
check_flgs n

; Validate that overfow flag isnt changed if there isnt an overflow
set_reg64 r58, 0x0, 0x0
set_reg64 r2,  0x0, 0x0
set_reg64 r4,  0x0, 0x0
qmachu.f r0, r2, r4
check_flgs vs

; Test 2
; Encodings
; qmach<.f> a,b,u6 00101bbb01110101FBBBuuuuuuAAAAAA
; and
; qmach<.f> a,b,c 00101bbb00110101FBBBCCCCCCAAAAAA
; with limm instead of c
;
; With u6 (or s12 for that matter), the value is zero
; extended to 16 bits, and replicated for all 4 16 bit
; blocks in a 64 bir operand
; 
; For limm, as it is 32 bit, the result is instead doubled
; to provide the appropriate 64 bit argument

.macro run_test2X  insn, \
                  acch, accl, \
                  r2h, r2l,   \
                  const,         \
                  exph, expl
  prep_test_case
  set_reg64 r58, \acch, \accl ; Set accumulator
  set_reg64 r2, \r2h, \r2l
  \insn r0, r2, \const
  check_reg64 r0, \exph, \expl
  check_reg64 r58, \exph, \expl
  ; Run instruction again to reset flags
  set_reg64 r58, \acch, \accl ; Set accumulator
  set_reg64 r2, \r2h, \r2l
  \insn r0, r2, \const
.endm
; 
; Test 2.1
.macro run_test21   r2h, r2l, const, exph, expl
  reset_flags
  run_test2X qmach, 0x0, 0x0, \r2h, \r2l, \const, \exph, \expl
.endm

; u6 replicated on 4 halfwords, but only first halfword of register is non 0
run_test21 0x0, 0x2, \
           0x2, \
           0x0, 0x4

; u6 replicated on 4 halfwords, all halfwords of register are 1
run_test21 0x10001, 0x10001, \
           0xf, \
           0x0, 0x3c

; Maximum u6 0b111 111
run_test21 0x10001, 0x10001, \
           0x3f, \
           0x0, 0xfc



; Smallest limm
run_test21 0x10001, 0x10001, \
           0x40, \
           0x0, 0x80


; Limm with highest bit set and lowest bit set
run_test21 0x10001, 0x10001, \
           0x10000001, \
           0x0, 0x2002

; Biggest limm with highest bit set and lowest bit set
run_test21 0x10001, 0x10001, \
           0xffffffff, \
           0xffffffff, 0xfffffffc

; Test 2.2
; Validate flag set and accumulator (can only overflow with accumulator)
.macro run_test22   acch, accl, r2h, r2l, const, exph, expl
  reset_flags
  run_test2X qmach.f, \acch, \accl, \r2h, \r2l, \const, \exph, \expl
.endm

; Closest to overflow with biggest limm
run_test22 0x80000000, 0x00000004, \
           0x10001, 0x10001, \
           0xffffffff, \
           0x80000000, 0x0
check_flgs vc

; Smallest overflow possible with biggest limm
run_test22 0x80000000, 0x00000003, \
           0x10001, 0x10001, \
           0xffffffff, \
           0x7fffffff, 0xffffffff
check_flgs vs

; Closest to overflow with biggest u6
run_test22 0x7fffffff, 0xffffff03, \
           0x10001, 0x10001, \
           0x3f, \
           0x7fffffff, 0xffffffff
check_flgs vc

; Smallest overflow possible with biggest u6
run_test22 0x7fffffff, 0xffffff04, \
           0x10001, 0x10001, \
           0x3f, \
           0x80000000, 0x0
check_flgs vs

; Test 3
; Encoding: qmach<.f> b,b,s12 00101bbb10110101FBBBssssssSSSSSS
;
; The following qmach tests rely on:
; setting acc to some value
; using an instruction (qmach, qmach.f, qmac.cc, qmac.f.cc) with
; the given data, and checking if both accumulator and r0
; have the expected data

.macro run_test3X  insn, \
                  acch, accl, \
                  r2h, r2l,   \
                  const,         \
                  exph, expl
  prep_test_case
  set_reg64 r58, \acch, \accl ; Set accumulator
  set_reg64 r0, \r2h, \r2l
  \insn r0, r0, \const
  check_reg64 r0, \exph, \expl
  check_reg64 r58, \exph, \expl
  ; Run instruction again to reset flags
  set_reg64 r58, \acch, \accl ; Set accumulator
  set_reg64 r0, \r2h, \r2l
  \insn r0, r0, \const
.endm
; 
; Test 3.1
.macro run_test31   r2h, r2l, const, exph, expl
  reset_flags
  run_test3X qmach, 0x0, 0x0, \r2h, \r2l, \const, \exph, \expl
.endm

; s12 with highest and lowest bits set 0b1 000 000
run_test31 0x10001, 0x10001, \
           0b1000001, \
           0x0, 0x104

; s12 with -1
run_test31 0x10001, 0x10001, \
           -1, \
           0xffffffff, 0xfffffffc

; s12 with lowest possible negative number
run_test31 0x10001, 0x10001, \
           -2048, \
           0xffffffff, 0xffffe000

; s12 with highest possible positive number
run_test31 0x10001, 0x10001, \
           2047, \
           0x0, 0x1ffc

; Test 3.1
; Validate flag set and accumulator (can only overflow with accumulator)
; Bare in mind that this is an unsigned operation so we can
; never have a negative overflow
.macro run_test32   acch, accl, r2h, r2l, const, exph, expl
  reset_flags
  run_test3X qmach.f, \acch, \accl, \r2h, \r2l, \const, \exph, \expl
.endm

; Closest to overflow with biggest positive s12
run_test32 0x7fffffff, 0xffffe003, \
           0x10001, 0x10001, \
           2047, \
           0x7fffffff, 0xffffffff
check_flgs vc

; Closest overflow with biggest negative s12
run_test32 0x80000000, 0x00002000, \
           0x10001, 0x10001, \
           -2048, \
           0x80000000, 0x0
check_flgs vc

; Smallest overflow with biggest s12
run_test32 0x80000000, 0x00001FFF, \
           0x10001, 0x10001, \
           -2048, \
           0x7fffffff, 0xffffffff
check_flgs vs

; Biggest operation with biggest positive s12
run_test32 0xffffffff, 0xffffffff, \
           0x10001, 0x10001, \
           2047, \
           0x0, 0x1ffB
check_flgs vc

; Biggest operation with biggest negative s12
run_test32 0xffffffff, 0xffffffff, \
           0x10001, 0x10001, \
           -2048, \
           0xffffffff, 0xffffdfff
check_flgs vc

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Reporting ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

valhalla:
  print "[PASS]"
  b @1f

; If a test fails, it jumps here. Although, for the sake of uniformity,
; the printed output does not say much about which test case failed,
; one can uncomment the print_number line below or set a breakpoint
; here to check the R0 register for the test case number.
fail:
  ld r1, [test_nr]
  print "\nLast r0 value = "
  print_number r0
  print "\n Test number = "
  print_number r1
  print "\n[FAIL] on "
1:
  print " qmach: Quad unsigned 16x16 multiply and accumulate\n"
  end
