; check_qmachu.S
;
; Tests for qmachu: Quad unsigned 16x16 multiply and accumulate.
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

; branch to exit if th expected condition isnt met
.macro check_flgs   expected_cond
  b\expected_cond    @1f
  j @fail
  1:
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; QMACHU ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; The following qmachu tests rely on:
; setting acc to some value
; using an instruction (qmachu, qmachu.f, qmac.cc, qmac.f.cc) with
; the given data, and checking if both accumulator and r0
; have the expected data
.macro run_testX  insn, \
                  acch, accl, \
                  r2h, r2l,   \
                  r4h, r4l,   \
                  exph, expl
  prep_test_case
  set_reg64 r58, \acch, \accl ; Set accumulator
  set_reg64 r2, \r2h, \r2l
  set_reg64 r4, \r4h, \r4l
  \insn r0, r4, r2
  check_reg64 r0, \exph, \expl
  check_reg64 r58, \exph, \expl
  ; Run instruction again to reset flags
  set_reg64 r58, \acch, \accl ; Set accumulator
  set_reg64 r2, \r2h, \r2l
  set_reg64 r4, \r4h, \r4l
  \insn r0, r4, r2
.endm
;=== qmachu a,b,c ===
;
;
; Test 1
; Encoding: qmachu a,b,c 00101bbb00110100FBBBCCCCCCAAAAAA
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
  add.f 0, 0, 1           ; (Z, N, C, V) = (0, 0, 0, 0)
  run_testX qmachu, 0x0, 0x0, \r2h, \r2l, \r4h, \r4l, \exph, \expl
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
           0x0,        0x20004
; Swap registers
run_test11 0x10001,    0x10001,    \
           0x80018001, 0x80018001, \
           0x0,        0x20004
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
  add.f 0, 0, 1           ; (Z, N, C, V) = (0, 0, 0, 0)
  run_testX qmachu, \acch, \accl, \r2h, \r2l, \r4h, \r4l, \exph, \expl
.endm

run_test12 0x0, 0x2, \
           0x0, 0x0, \
           0x0, 0x0, \
           0x0, 0x2

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
           0x80018001, 0x80038005

; Test 1.3
; 
; a = acc + (b.h0 * c.h0) + (b.h1 * c.h1) + (b.h2 * c.h2) + (b.h3 * c.h3)
;
; Validate flag set
;
.macro run_test13   acch, accl, r2h, r2l, r4h, r4l, exph, expl, expected_cond
  add.f 0, 0, 1           ; (Z, N, C, V) = (0, 0, 0, 0)
  run_testX qmachu.f, \acch, \accl, \r2h, \r2l, \r4h, \r4l, \exph, \expl
  check_flgs \expected_cond
.endm
;
run_test13 0xffffffff, 0xffffffff, \
           0x0, 0x0, \
           0x0, 0x0, \
           0xffffffff, 0xffffffff, \
           vc

run_test13 0xffffffff, 0xfffffffe, \
           0x0, 0x1, \
           0x0, 0x1, \
           0xffffffff, 0xffffffff, \
           vc

; Accumulator overflow
run_test13 0xffffffff, 0xffffffff, \
           0x0, 0x1, \
           0x0, 0x1, \
           0x0, 0x0, \
           vs

; Biggest overflow possible
run_test13 0xffffffff, 0xffffffff, \
           0xffffffff, 0xffffffff, \
           0xffffffff, 0xffffffff, \
           0x00000003, 0xFFF80003, \
           vs

; Closest accumulator value without overflow
run_test13 0xfffffffc, 0x0007fffb, \
           0xffffffff, 0xffffffff, \
           0xffffffff, 0xffffffff, \
           0xffffffff, 0xffffffff, \
           vc

; Overflow with smallest accumulator value
run_test13 0xfffffffc, 0x0007fffc, \
           0xffffffff, 0xffffffff, \
           0xffffffff, 0xffffffff, \
           0x0,        0x0,        \
           vs

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
  print_number r0
  print "[FAIL]"
1:
  print " qmachu: Quad unsigned 16x16 multiply and accumulate\n"
  end
