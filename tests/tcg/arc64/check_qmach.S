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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; QMACH ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; ALTERS THE FLAGS!!
.macro check_reg_acc reg, high, low
    check_reg_64val_ww  \reg, \high, \low
    check_accumulator \high, \low
.endm
;
; General operation:
;   a = 0 + (b.w0 * c.h0) + (b.w1 * c.h1)
;   acc = a
;

; ==== First test set: Test operations on expected operands ====
; acc <- 0
; r0  <- r2, r4, acc
; acc <- ro

; Basic operation test, 1 non zero word, 1 non zero half word
;    (0x0000, 0x0000, 0x0000, 0x0002)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0000, 0x0000, 0x0000, 0x0001)           (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x00000000, 0x00000000)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
;    (0x00000000, 0x00000002) acc/r0
;
next_test_case
set_reg_64val_ww    r2, 0x00000000, 0x00000002
set_reg_64val_ww    r4, 0x00000000, 0x00000001
set_accumulator         0x00000000, 0x00000000
qmach r0, r2, r4
check_reg_acc  r0, 0x00000000, 0x00000002

; Basic operation test, each (half) word at 1
;    (0x0000, 0x0001, 0x0000, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0000, 0x0001, 0x0000, 0x0001)           (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x00000000, 0x00000000)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
;    (0x00000000, 0x00000002) acc/r0
;
next_test_case
set_reg_64val_ww    r2, 0x00000001, 0x00000001
set_reg_64val_ww    r4, 0x00000001, 0x00000001
set_accumulator         0x00000000, 0x00000000
qmach r0, r2, r4
check_reg_acc  r0, 0x00000000, 0x00000002

; First and last bit sets on each half word of r2
;
;    (0x8001, 0x8001, 0x8001, 0x8001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0001, 0x0001, 0x0001, 0x0001)           (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x00000000, 0x00000000)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
;    (0xffffffff, 0xfffe0004) acc/r0
;
next_test_case
set_reg_64val_ww    r2, 0x80018001, 0x80018001
set_reg_64val_ww    r4, 0x00010001, 0x00010001
set_accumulator         0x00000000, 0x00000000
qmach r0, r2, r4
check_reg_acc  r0, 0xffffffff, 0xfffe0004

; First and last bit sets on each word of r2
;
;    (0x8000, 0x0001, 0x8000, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0001, 0x0001, 0x0001, 0x0001)           (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x00000000, 0x00000000)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
;    (0xffffffff, 0xfff00002) acc/r0
;
next_test_case
set_reg_64val_ww    r2, 0x80000001, 0x80000001
set_reg_64val_ww    r4, 0x000100001, 0x000100001
set_accumulator         0x00000000, 0x00000000
qmach r0, r2, r4
check_reg_acc  r0, 0xffffffff, 0xfff00002

; First and last bit sets on each half word of r4
;
;    (0x0000, 0x0001, 0x0000, 0x0001)          (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x8001, 0x8001, 0x8001, 0x8001)          (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x00000001, 0x00000002)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
;    (0xffffffff, 0xffff0002) acc/r0
;
next_test_case
set_reg_64val_ww    r2, 0x00000001, 0x00000001
set_reg_64val_ww    r4, 0x80018001, 0x80018001
set_accumulator         0x00000000, 0x00000000
qmach r0, r2, r4
check_reg_acc  r0, 0xffffffff, 0xffff0002

; Simple accumulator value test
;
;    (0x0000, 0x0000, 0x0000, 0x0000)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0000, 0x0000, 0x0000, 0x0000)           (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x80000001, 0x80000001)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
;    (0x80000001, 0x80000001) acc/r0
;
next_test_case
set_reg_64val_ww    r2, 0x00000000, 0x00000000
set_reg_64val_ww    r4, 0x00000000, 0x00000000
set_accumulator         0x80000001, 0x80000001
qmach r0, r2, r4
check_reg_acc  r0, 0x80000001, 0x80000001

; ==== Second test set: Test operations and overflow with accumulator ====
; acc <- x
; r0  <- r2, r4, acc
; acc <- ro

; Simple accumulator operation
;
;    (0x0000, 0x0001, 0x0000, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0000, 0x0001, 0x0000, 0x0001)           (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x00000000, 0x00000002)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
;    (0x00000000, 0x00000004) acc/r0
;
next_test_case
set_reg_64val_ww    r2, 0x00000001, 0x000000001
set_reg_64val_ww    r4, 0x00000001, 0x000000001
set_accumulator         0x00000000, 0x00000002
qmach r0, r2, r4
check_reg_acc  r0, 0x00000000, 0x00000004

; First and last bit on each half word, including in the accumulator
;
;    (0x8001, 0x8001, 0x8001, 0x8001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x8001, 0x8001, 0x8001, 0x8001)           (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x80018001, 0x80018001)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
;    (0x80018002, 0x7ffd8005) acc/r0
;
next_test_case
set_reg_64val_ww    r2, 0x80018001, 0x80018001
set_reg_64val_ww    r4, 0x80018001, 0x80018001
set_accumulator         0x80018001, 0x80018001
qmach r0, r2, r4
check_reg_acc  r0, 0x80018002, 0x7ffd8005

; Near overflow accumulator, with no operation
;
;    (0x0000, 0x0000, 0x0000, 0x0000)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0000, 0x0000, 0x0000, 0x0000)           (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x7fffffff, 0xffffffff)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
;    (0x7fffffff, 0xffffffff) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r2, 0x00000000, 0x00000000
set_reg_64val_ww    r4, 0x00000000, 0x00000000
set_accumulator         0x7fffffff, 0xffffffff
reset_flags
qmach.f r0, r2, r4
check_flag vc
check_reg_acc  r0, 0x7fffffff, 0xffffffff

; Near overflow accumulator, with an operation
;
;    (0x0000, 0x0000, 0x0000, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0000, 0x0000, 0x0000, 0x0001)           (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x7fffffff, 0xfffffffe)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
;    (0x7fffffff, 0xffffffff) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r2, 0x00000000, 0x00000001
set_reg_64val_ww    r4, 0x00000000, 0x00000001
set_accumulator         0x7fffffff, 0xfffffffe
reset_flags
qmach.f r0, r2, r4
check_flag vc
check_reg_acc  r0, 0x7fffffff, 0xffffffff

; Smallest overflow accumulator
;
;    (0x0000, 0x0000, 0x0000, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0000, 0x0000, 0x0000, 0x0001)           (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x7fffffff, 0xffffffff)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
;    (0x80000000, 0x00000000) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r2, 0x00000000, 0x00000001
set_reg_64val_ww    r4, 0x00000000, 0x00000001
set_accumulator         0x7fffffff, 0xffffffff
reset_flags
qmach.f r0, r2, r4
check_flag vs
check_reg_acc  r0, 0x80000000, 0x00000000

; Biggest overflow accumulator
;
;    (0xffff, 0xffff, 0xffff, 0xffff)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0000, 0xffff, 0x0000, 0xffff)           (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x7fffffff, 0xffffffff)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
;    (0x80000000, 0x00000001) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r2, 0xffffffff, 0xffffffff
set_reg_64val_ww    r4, 0x0000ffff, 0x0000ffff
set_accumulator         0x7fffffff, 0xffffffff
reset_flags
qmach.f r0, r2, r4
check_flag vs
check_reg_acc  r0, 0x80000000, 0x00000001

; ==== Third test set: Test operations with u6, limm and s12 ====

; Smallest u6, being replicated
;
;    (0x0001, 0x0001, 0x0001, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0002, 0x0002, 0x0002, 0x0002)           (u6, u6)
;    (0x00000000, 0x00000000)                   acc
;    (r2.h0 * u6) + (r2.h1 * u6) + (r2.h2 * u6) + (r2.h3 * u6) + acc
; -------------------------------------------
;    (0x00000000, 0x00000008) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r2, 0x00010001, 0x00010001
set_accumulator         0x00000000, 0x00000000
qmach r0, r2,         0x0002
check_reg_acc  r0, 0x00000000, 0x00000008

; Biggest u6, being replicated
;
;    (0x0000, 0x0001, 0x0000, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x003f, 0x003f, 0x003f, 0x003f)           (u6, u6)
;    (0x00000000, 0x00000000)                   acc
;    (r2.h0 * u6) + (r2.h1 * u6) + (r2.h2 * u6) + (r2.h3 * u6) + acc
; -------------------------------------------
;    (0x00000000, 0x0000007e) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r2, 0x00000001, 0x00000001
set_accumulator         0x00000000, 0x00000000
qmach r0, r2,         0x003f
check_reg_acc  r0, 0x00000000, 0x0000007e

; Closest to overflow with biggest u6
;
;    (0x7fff, 0xffff, 0xffff, 0xffff)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x003f, 0x003f, 0x003f, 0x003f)           (u6, u6)
;    (0x7fffffff, 0xff0400fb)                   acc
;    (r2.h0 * u6) + (r2.h1 * u6) + (r2.h2 * u6) + (r2.h3 * u6) + acc
; -------------------------------------------
;    (0x7fffffff, 0xffffffff) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r2, 0x7fffffff, 0xffffffff
set_accumulator         0x7fffffff, 0xffe080fb
reset_flags
qmach.f r0, r2,       0x003f
check_flag vc
check_reg_acc  r0, 0x7fffffff, 0xffffffff

; Smallest overflow with biggest u6
;
;    (0x0001, 0x0001, 0x0001, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x003f, 0x003f, 0x003f, 0x003f)           (u6, u6)
;    (0x7fffffff, 0xffffff04)                   acc
;    (r2.h0 * u6) + (r2.h1 * u6) + (r2.h2 * u6) + (r2.h3 * u6) + acc
; -------------------------------------------
;    (0x80000000, 0x00000000) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r2, 0x00010001, 0x00010001
set_accumulator         0x7fffffff, 0xffffff04
reset_flags
qmach.f r0, r2,       0x003f
check_flag vs
check_reg_acc  r0, 0x80000000, 0x00000000

; Biggest overflow with biggest u6
;
;    (0x7fff, 0xffff, 0xffff, 0xffff)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x003f, 0x003f, 0x003f, 0x003f)           (u6, u6)
;    (0x7fffffff, 0xffffffff)                   acc
;    (r2.h0 * u6) + (r2.h1 * u6) + (r2.h2 * u6) + (r2.h3 * u6) + acc
; -------------------------------------------
;    (0x80000000, 0x001f7f03) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r2, 0x7fffffff, 0xffffffff
set_accumulator         0x7fffffff, 0xffffffff
reset_flags
qmach.f r0, r2,       0x003f
check_flag vs
check_reg_acc  r0, 0x80000000, 0x001f7f03

; Simple limm, being replicated
;
;    (0x0000, 0x0001, 0x0000, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0040, 0x0040, 0x0040, 0x0040)           (limm.h0, limm.h1)
;    (0x00000000, 0x00000000)                   acc
;    (r2.h0 * limm.h0) + (r2.h1 * limm.h1) + (r2.h2 * limm.h0) +
;    (r2.h3 * limm.h1) + acc
; -------------------------------------------
;    (0x00000000, 0x00000080) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r2, 0x00000001, 0x00000001
set_accumulator         0x00000000, 0x00000000
qmach r0, r2,         0x00400040
check_reg_acc  r0, 0x00000000, 0x00000080

; Biggest limm, being replicated
;
;    (0x0000, 0x0001, 0x0000, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0xffff, 0xffff, 0xffff, 0xffff)           (limm.h0, limm.h1)
;    (0x00000000, 0x00000000)                   acc
;    (r2.h0 * limm.h0) + (r2.h1 * limm.h1) + (r2.h2 * limm.h0) +
;    (r2.h3 * limm.h1) + acc
; -------------------------------------------
;    (0xffffffff, 0xfffffffe) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r2, 0x00000001, 0x00000001
set_accumulator         0x00000000, 0x00000000
qmach r0, r2,         0xffffffff
check_reg_acc  r0, 0xffffffff, 0xfffffffe

; Smallest overflow with biggest (positive) limm
;
;    (0x0000, 0x0001, 0x0000, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x7fff, 0xffff, 0xffff, 0xffff)           (limm.h0, limm.h1)
;    (0x7fffffff, 0xfffe0002)                   acc
;    (r2.h0 * limm.h0) + (r2.h1 * limm.h1) + (r2.h2 * limm.h0) +
;    (r2.h3 * limm.h1) + acc
; -------------------------------------------
;    (0x80000000, 0x00000000) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r2, 0x00010001, 0x00010001
set_accumulator         0x7fffffff, 0xffff0004
reset_flags
qmach.f r0, r2,       0x7fffffff
check_flag vs
check_reg_acc  r0, 0x80000000, 0x00000000

; Biggest overflow with biggest (positive) limm
;
;    (0x7fff, 0x7fff, 0x7fff, 0x7fff)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x7fff, 0xffff, 0xffff, 0xffff)           (limm.h0, limm.h1)
;    (0xffffffff, 0xffffffff)                   acc
;    (r2.h0 * limm.h0) + (r2.h1 * limm.h1) + (r2.h2 * limm.h0) +
;    (r2.h3 * limm.h1) + acc
; -------------------------------------------
;    (0x80000000, 0x7ffd0003) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r2, 0x7fff7fff, 0x7fff7fff
set_accumulator         0x7fffffff, 0xffffffff
reset_flags
qmach.f r0, r2,       0x7fffffff
check_flag vs
check_reg_acc  r0, 0x80000000, 0x7ffd0003

; s12 with highest and lowest bits set
;
;    (0x0001, 0x0001, 0x0001, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;(0b1000001, 0b1000001, 0b1000001, 0b1000001)   (s12, s12)
;    (0x00000000, 0x00000000)                   acc
;    (r2.w0 * s12) + (r2.w1 * s12) + acc
; -------------------------------------------
;    (0x00000000, 0x00000104) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r0, 0x00010001, 0x00010001
set_accumulator         0x00000000, 0x00000000
qmach r0, r0,         0b1000001
check_reg_acc  r0, 0x00000000, 0x00000104

; Smallest negative s12 (-1)
;
;    (0x0001, 0x0001, 0x0001, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0xffff, 0xffff, 0xffff, 0xffff)           (s12, s12)
;    (0x00000000, 0x00000000)                   acc
;    (r2.w0 * s12) + (r2.w1 * s12) + acc
; -------------------------------------------
;    (0xffffffff, 0xfffffffc) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r0, 0x00010001, 0x00010001
set_accumulator         0x00000000, 0x00000000
qmach r0, r0,         -1
check_reg_acc  r0, 0xffffffff, 0xfffffffc

; Lowest negative s12 (-2048)
;
;    (0x0001, 0x0001, 0x0001, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0xf800, 0xf800, 0xf800, 0xf800)           (s12, s12)
;    (0x00000000, 0x00000000)                   acc
;    (r2.w0 * s12) + (r2.w1 * s12) + acc
; -------------------------------------------
;    (0xffffffff, 0xffffe000) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r0, 0x00010001, 0x00010001
set_accumulator         0x00000000, 0x00000000
qmach r0, r0,         -2048
check_reg_acc  r0, 0xffffffff, 0xffffe000

; Biggest positive s12 (2047)
;
;    (0x0001, 0x0001, 0x0001, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x07ff, 0x07ff, 0x07ff, 0x07ff)           (s12, s12)
;    (0x00000000, 0x00000000)                   acc
;    (r2.w0 * s12) + (r2.w1 * s12) + acc
; -------------------------------------------
;    (0x00000000, 0x00001ffc) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r0, 0x00010001, 0x00010001
set_accumulator         0x00000000, 0x00000000
qmach r0, r0,         2047
check_reg_acc  r0, 0x00000000, 0x00001ffc

; Closest to overflow with biggest positive s12 (2047)
;
;    (0x0001, 0x0001, 0x0001, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x07ff, 0x07ff, 0x07ff, 0x07ff)           (s12, s12)
;    (0x7fffffff, 0xffffe003)                   acc
;    (r2.w0 * s12) + (r2.w1 * s12) + acc
; -------------------------------------------
;    (0x7fffffff, 0xffffffff) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r0, 0x00010001, 0x00010001
set_accumulator         0x7fffffff, 0xffffe003
reset_flags
qmach.f r0, r0,       2047
check_flag vc
check_reg_acc  r0, 0x7fffffff, 0xffffffff

; Closest to overflow with biggest negative s12 (-2048)
;
;    (0x0001, 0x0001, 0x0001, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0xf800, 0xf800, 0xf800, 0xf800)           (s12, s12)
;    (0x80000000, 0x00002000)                   acc
;    (r2.w0 * s12) + (r2.w1 * s12) + acc
; -------------------------------------------
;    (0x80000000, 0x00000000) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r0, 0x00010001, 0x00010001
set_accumulator         0x80000000, 0x00002000
reset_flags
qmach.f r0, r0,       -2048
check_flag vc
check_reg_acc  r0, 0x80000000, 0x00000000

; Smallest overflow with biggest positive s12 (2047)
;
;    (0x0001, 0x0001, 0x0001, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x07ff, 0x07ff, 0x07ff, 0x07ff)           (s12, s12)
;    (0x7fffffff, 0xffffe004)                   acc
;    (r2.w0 * s12) + (r2.w1 * s12) + acc
; -------------------------------------------
;    (0x80000000, 0x00000000) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r0, 0x00010001, 0x00010001
set_accumulator         0x7fffffff, 0xffffe004
reset_flags
qmach.f r0, r0,       2047
check_flag vs
check_reg_acc  r0, 0x80000000, 0x00000000

; Smallest overflow with biggest negative s12 (-2048)
;
;    (0x0001, 0x0001, 0x0001, 0x0001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0xf800, 0xf800, 0xf800, 0xf800)           (s12, s12)
;    (0x80000000, 0x00001fff)                   acc
;    (r2.w0 * s12) + (r2.w1 * s12) + acc
; -------------------------------------------
;    (0x7fffffff, 0xffffffff) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r0, 0x00010001, 0x00010001
set_accumulator         0x80000000, 0x00001fff
reset_flags
qmach.f r0, r0,       -2048
check_flag vs
check_reg_acc  r0, 0x7fffffff, 0xffffffff

; Biggest overflow with biggest positive s12 (2047)
;
;    (0x7fff, 0x7fff, 0x7fff, 0x7fff)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x07ff, 0x07ff, 0x07ff, 0x07ff)           (s12, s12)
;    (0x7fffffff, 0xffffffff)                   acc
;    (r2.w0 * s12) + (r2.w1 * s12) + acc
; -------------------------------------------
;    (0x80000000, 0x0ffde003) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r0, 0x7fff7fff, 0x7fff7fff
set_accumulator         0x7fffffff, 0xffffffff
reset_flags
qmach.f r0, r0,       2047
check_flag vs
check_reg_acc  r0, 0x80000000, 0x0ffde003

; Biggest overflow with biggest negative s12 (-2048)
;
;    (0x8000, 0x8000, 0x8000, 0x8000)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0xf800, 0xf800, 0xf800, 0xf800)           (s12, s12)
;    (0x7fffffff, 0xffffffff)                   acc
;    (r2.w0 * s12) + (r2.w1 * s12) + acc
; -------------------------------------------
;    (0x80000000, 0x0fffffff) acc/r0
; Overflow flag cleared
;
next_test_case
set_reg_64val_ww    r0, 0x80008000, 0x80008000
set_accumulator         0x7fffffff, 0xffffffff
reset_flags
qmach.f r0, r0,       -2048
check_flag vs
check_reg_acc  r0, 0x80000000, 0x0fffffff

; ==== Fifth test set: Test operations with permitted operands ====

; Validate dest register 0 and .f flag
;    (0x7fff, 0x7fff, 0x7fff, 0x7fff)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0000, 0x7fff, 0x0000, 0x7fff)           (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x7fffffff, 0xffffffff)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
; Overflow flag is set
;
next_test_case
set_reg_64val_ww    r2, 0x7fff7fff, 0x7fff7fff
set_reg_64val_ww    r4, 0x7fff,     0x7fff
set_accumulator         0x7fffffff, 0xffffffff
reset_flags
qmach.f 0, r2, r4
check_flag vs

; Validate the cc flag (if not working, overflow flag will be set)
;    (0x7fff, 0x7fff, 0x7fff, 0x7fff)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x0000, 0x7fff, 0x0000, 0x7fff)           (r4.h0, r4.h1, r4.h2, r4.h3)
;    (0x7fffffff, 0xffffffff)                   acc
;  (r2.h0 * r4.h0) + (r2.h1 * r4.h1)  + (r2.h2 * r4.h2)  + (r2.h3 * r4.h3) + acc
; -------------------------------------------
; Overflow flag set if zero flag is set (it isn't)
;
next_test_case
set_reg_64val_ww    r2, 0x7fff7fff, 0x7fff7fff
set_reg_64val_ww    r4, 0x7fff,     0x7fff
set_accumulator         0x7fffffff, 0xffffffff
reset_flags
unset_z_flag
qmach.z.f r2, r2, r4
check_flag vc

; Validate that the overflow flag is only set, never cleared
; Also validate, same register in all operands
;    (0x8001, 0x8001, 0x8001, 0x8001)           (r2.h0, r2.h1, r2.h2, r2.h3)
;    (0x00000000, 0x00000000)                   acc
;  (r2.h0 * r2.h0) + (r2.h1 * r2.h1)  + (r2.h2 * r2.h2)  + (r2.h3 * r2.h3) + acc
; -------------------------------------------
; Overflow flag is not cleared
;
next_test_case
set_reg_64val_ww    r2, 0x80018001, 0x80018001
set_accumulator         0x00000000, 0x00000000
reset_flags
set_v_flag
qmach.f r2, r2, r2
check_flag vs
check_reg_acc  r2, 0x00000000, 0xfffc0004

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
