; check_mmuv48_05.S
;
; Run with
; make ndebug TEST=mmuv48_04 PAGE_SIZE=64K; tail nsim.trace
;
; Bunch of good weather scenario tests for MMUv48 flavor of MMUv6, 64KB page
;  granularity, with overlapping tables and more than one per level.
; In case of a failure, consult the "fail" label section of the code.

  .include "macros.inc"
  .include "mmuv48.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;; Test checking routines ;;;;;;;;;;;;;;;;;;;;;;;;;;

; Test case counter (set in the code execution address so physical address
;  matches virtual address via MMU and we don't need to add extra logic
.equ test_nr, 0x80001000

; Increment the test counter and set (Z,N,C,V) to (0,0,0,0).
.macro prep_test_case
  ld    r13, [test_nr]
  add_s r13, r13, 1       ; increase test case counter
  st    r13, [test_nr]
  add.f 0, 0, 1           ; (Z, N, C, V) = (0, 0, 0, 0)
.endm

; reg = 64bit(hi, lo)
.macro set_reg64 reg, hi, lo
  movhl \reg, \hi
  orl   \reg, \reg, \lo
.endm

; reg == 64bit(hi, lo) AND reg == acc
; Clobbers r11
.macro chk_reg64 reg, hi, lo
  movhl  r11, \hi
  orl    r11, r11,  \lo
  xorl.f 0,   \reg, r11
  bne    @fail
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Tables for Tests ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; If TxSZ are 16, there is a 4-level page table and the regions are:
;
; (T0SZ, T1SZ) = (16, 16)      4-level page table
; VA: 0x0000_0000_0000_0000 -> RTP0          Top 16 bits 0
; VA: 0x0000_FFFF_FFFF_FFFF -> RTP0          Top 16 bits 0
;
; VA: 0xFFFF_0000_0000_0000 -> RTP1          Top 16 bits 1
; VA: 0xFFFF_FFFF_FFFF_FFFF -> RTP1          Top 16 bits 1
;
; VA: 0x0001_0000_0000_0000 -> EV_IMMUFault  Not all top 16 bits 0
; VA: 0xFFFE_0000_0000_0000 -> EV_DMMUFault  Not all top 16 bits 1

;  table1@0x1000_0000 (tarc.ld)
; ,-------------------.
; | [ 0 ]  table 2_1  |
; |  ...   xxxxx0     |
; | [ 32 ] table 2_2  |
; |  ...   xxxxx0     |
; | [ 63 ] xxxxx0     |
; `-------------------'
;
;  table2_1@0x1001_0000
; ,-------------------------------------.
; | [ 0 ]  xxxxxxxxxxxxxxxxxxxxxx0      |
; |  ...   xxxxxxxxxxxxxxxxxxxxxx0      |
; | [ 4 ]  512MB block 0x8000_0000 AF V |
; |  ...   xxxxxxxxxxxxxxxxxxxxxx0      |
; | [8191] xxxxxxxxxxxxxxxxxxxxxx0      |
; `-------------------------------------'
;
;  table2_2@0x1002_0000
; ,-------------------------------------.
; | [ 0 ]  xxxxxxxxxxxxxxxxxxxxxx0      |
; | [ 1 ]  512MB block 0x2000_0000 AF V |
; |  ...   xxxxxxxxxxxxxxxxxxxxxx0      |
; | [ 129 ]  table 3                    |
; |  ...   xxxxxxxxxxxxxxxxxxxxxx0      |
; | [8191] xxxxxxxxxxxxxxxxxxxxxx0      |
; `-------------------------------------'
;
;  table3@0x1003_0000
; ,-------------------------------------.
; | [ 0 ]   64KB page 0x4000_0000 AF V  |
; |  ...    xxxxxxxxxxxxxxxxxxxxxx0     |
; | [ 512 ] 64KB page 0x6000_0000 AF V  |
; `-------------------------------------'
;
;     Code execution translations
;
; Virtual address 0x8000_0000
;    000000,  0000000000100, XXXXXXXXXXXXX, XXXXXXXXXXXXXXXX
;   6-bit:0,       13-bit:4,      13-bit:x,         16-bit:x
;  table1[0] ->  table2_1[4] -> 0x8000_0000 (a 512MB identity block)
; Translates to the physical address 0x8000_0000
; These are code execution addresses
;

.section .data.pagetables
; 0x10000000
.align 0x10000
pt_l1_base:
.8byte 0x10010000 | kTableValid; code execution
.space 31 * 8
.8byte 0x10020000 | kTableValid; test 1/2/3
.space 31 * 8

; 0x10010000
.align 0x10000
pt_l2_1_base:
.space 4 * 8
.8byte 0x80000000 | kAF | kBlockValid; code execution
.space ((8192 - 5) * 8)

; 0x10020000
.align 0x10000
pt_l2_2_base:
.8byte 0
.8byte 0x20000000 | kAF | kBlockValid; test 1
.space 127 * 8
.8byte 0x10030000 | kTableValid; test 2
.space ((8192 - 130) * 8)

; 0x10030000
.align 0x10000
pt_l3_0_base:
.8byte 0x40000000 | kAF | kPageValid
.space (511 * 8)
.8byte 0x60000000 | kAF | kPageValid

;;;;;;;;;;;;;;;;;;;;;;;;;;;; MMUv48_01 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;;;;;;;;;;;;;;;;;;;;;;;;;;;; FOR NOW SKIP THIS TEST;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  b @1f


; Planting data in physical addresess for the tests
  ; ,------.-----------------------.------------------.-----------------------.
  ; | test | virtual address       | physical address | data                  |
  ; |------+-----------------------+------------------+-----------------------|
  ; |  1   | 0x0000_8000_2000_0000 | 0x0000_2000_0000 | 0x0123_4567_89ab_cdef |
  ; |  2   | 0x0000_8010_2000_0000 | 0x0000_4000_0000 | 0x2A                  |
  ; |  3   | 0x0000_8010_2200_0000 | 0x0000_6000_0000 | 0xACDC                |
  ; `------^-----------------------^------------------^-----------------------'

; test 1
  mov       r0, 0x20000000
  set_reg64 r1, 0x01234567, 0x89abcdef
  stl       r1, [r0]

; test 2
  mov       r0, 0x40000000
  mov       r1, 0x2A
  stl       r1, [r0]

; test 3

  mov       r0, 0x60000000
  mov       r1, 0xACDC
  stl       r1, [r0]

  ; write page table base
  mov       r0, pt_l1_base
  sr        r0, [MMU_RTP0]

  ; enable MMUv6
  MMUV48_SET_TSZ_TO_16
  MMUV6_ENABLE

  ; read from a few locations to check translations

; Test 01
;
; Virtual address 0x8000_2000_0000
;    100000,  0000000000001, XXXXXXXXXXXXX, XXXXXXXXXXXXXXXX
;  6-bit:32,       13-bit:1,      13-bit:x,         16-bit:x
; table1[32] ->  table2_2[1] -> 0x2000_0000 (a 512MB block)
; Translates to the physical address 0x2000_0000
;
prep_test_case
  movhl r1, 0x8000
  orl    r1, r1, 0x20000000

  ldl    r2, [r1]
  chk_reg64 r2, 0x01234567, 0x89abcdef


; Test 02
;
; Virtual address 0x8010_2000_0000
;    100000,   0000010000001, 0000000000000, XXXXXXXXXXXXXXXX
;  6-bit:32,      13-bit:129,      13-bit:0,         16-bit:x
; table1[32] -> table2_2[129] ->   table3[0] -> 0x4000_0000 (a 64KB block)
; Translates to the physical address 0x3000_0000
;
prep_test_case
  movhl r1, 0x8010
  orl    r1, r1, 0x20000000

  ldl    r2, [r1]
  chk_reg64 r2, 0x00000000, 0x2A


; Test 03
;
; Virtual address 0x8010_2200_0000
;    100000,   0000010000001, 0001000000000, XXXXXXXXXXXXXXXX
;  6-bit:32,      13-bit:129,    13-bit:512,         16-bit:x
;  table1[1] ->  table2_2[0] ->  table3[512] -> 0x6000_0000 (a 64KB block)
; Translates to the physical address 0x6000_0000
;
prep_test_case
  movhl r1, 0x8010
  orl    r1, r1, 0x22000000

  ldl    r2, [r1]
  chk_reg64 r2, 0x00000000, 0xACDC

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Reporting ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

valhalla:
  MMUV6_DISABLE
  print "[PASS]"
  mov   r1, 0xdecaf                ; for now: good value in nSIM tracing
  b @1f

; If a test fails, it jumps here. Although, for the sake of uniformity,
; the printed output does not say much about which test case failed,
; one can uncomment the print_number line below or set a breakpoint
; here to check the R0 register for the test case number.
fail:
  MMUV6_DISABLE
  print "[FAIL]"
  mov   r1, 0xbadcafe              ; for now: bad value in nSIM tracing
  ld    r0, [test_nr]
  ;print_number r0
1:
  ;flag  0x1                        ; for now: nSIM ends here
  ;brk
  print " MMUv48_05: 64K page size test. Currently disabled until TCG tests support passing the appropriate mmu cpu flag\n"
  end
