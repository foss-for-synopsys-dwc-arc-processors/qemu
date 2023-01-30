; check_mmuv48_01.S
;
; Bunch of simple good weather scenario tests for MMUv48 flavor of MMUv6,
;  4KB page granularity.
; In case of a failure, consult the "fail" label section of the code.

  .include "macros.inc"
  .include "mmuv48.inc"

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

  .equ shareable_page_phys_addr,   0x20003000
  .equ shareable_page_virt_addr,   0x40003000

  .equ shareable_page_phys_addr2,  0x20005000
  .equ shareable_page_virt_addr2,  0x40004000

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

; RTP0
;
;  table1@0x1000_0000 (tarc.ld)         table2@0x1000_1000
; ,------------------. ,---------------------------------.
; | [ 0 ] table 2    | | [ 0 ] 1G block 0x0000_0000 AF V |
; | [ 1 ] table 2    | | [ 1 ] table 3                   |
; |  ...  xxxxx0     | | [ 2 ] 1G block 0x8000_0000 AF V |
; | [511] table 2    | |  ...  xxxxxxxxxxxxxxxxxxx0      |
; `------------------' `---------------------------------'
; ,------------------. ,----------------------------------.
; | [ 0 ] table 4    | | [ 0 ] 4KB page 0x2000_0000 AF V  |
; | [ 1 ] xxxxx0     | | [ 1 ] 4KB page 0x2000_1000 AF V  |
; | [ 2 ] xxxxx0     | | [ 2 ] 4KB page 0x2000_2000 AF V  |
; | [ 3 ] xxxxx0     | | [ 3 ] 4KB page 0x2000_3000 AF V  |
; | [ 4 ] xxxxx0     | | [ 4 ] 4KB page 0x2000_4000 AF V  |
; |  ...  xxxxx0     | |  ...  xxxxxxxxxxxxxxxxxxxx0      |
; | [511] xxxxx0     | | [512] xxxxxxxxxxxxxxxxxxxx0      |
; `------------------' `----------------------------------'
;  table3@0x1000_2000                   table4@0x1000_3000
;
;     Code execution translations
;
; Virtual address 0x8000_0000 -> 0xBFFF_FFFC
; 000000000 , 0000000_10 , XXXXXXXXX , XXXXXXXXX , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:2 ,   9-bit:x ,   9-bit:x ,     12-bit:x
;  table1[0] -> table2[2] -> 0x8000_0000 (a 1GB identity block)
; Translates to the physical address 0x8000_0000 -> 0xBFFF_FFFC
;

.section .data.pagetables
; 0x10000000
pt_l1_base:
.8byte 0x10001000 | kTableValid; Tests 01/02 + identity code execution
.8byte 0x10001000 | kTableValid; Tests 03/04
.space (0x1000 - 3 * 8)
.8byte 0x10001000 | kTableValid; Test 05/06

.align 0x1000
; 0x10001000
pt_l2_base:
.8byte 0x00000000 | kAF | kBlockValid; Tests 01/03/04
.8byte 0x10002000 | kTableValid      ; Test 02/05/06
.8byte 0x80000000 | kAF | kBlockValid; identity for code execution
.space (0x1000 - 3 * 8)

.align 0x1000
; 0x10002000
pt_l3_base:
.8byte 0x10003000 | kTableValid; Test 02/05/06
.space (0x1000 - 3 * 8)

.align 0x1000
; 0x10003000
pt_l4_base:
.8byte 0x20000000 | kAF | kPageValid; Tests 02/05
.8byte 0x20001000 | kAF | kPageValid; Test 06
.8byte 0x20002000 | kAF | kPageValid; Test 06
.8byte shareable_page_phys_addr  | kAF | kSH1 | kPageValid; Test 07
.8byte shareable_page_phys_addr2 | kAF | kSH1 | kPageValid; Test 07
.space (0x1000 - 3 * 8)

;;;;;;;;;;;;;;;;;;;;;;;;;;;; MMUv48_01 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

                                  ;
  mov   r0, 0x20000000            ; ,-------------.-------------.
  st    r0, [r0]                  ; | phys. addr. |   val.      |
                                  ; |---------------------------|
  mov   r0, 0x20001000            ; | 0x2000_0000:  0x2000_0000 |
  st    r0, [r0]                  ; |                           |
                                  ; | 0x2000_1000:  0x2000_1000 |
  mov   r0, 0x20002000            ; |                           |
  st    r0, [r0]                  ; | 0x2000_2000:  0x2000_2000 |
                                  ; |                           |
  mov   r0, 0x20003FFC            ; | 0x2000_3FFC:  0x2000_3FFC |
  st    r0, [r0]                  ; |                           |
                                  ; | 0x2000_5000:  0x2000_5000 |
  mov   r0, 0x20005000            ; `---------------------------'
  st    r0, [r0]                  ;

  ; write page table base
  mov   r0, pt_l1_base
  sr    r0, [MMU_RTP0]

  ; enable MMUv6
  MMUV48_SET_TSZ_TO_16
  MMUV6_ENABLE

  ; read from a few locations to check translations

; Test 01
; Block address translation
;
; Virtual address 0x0000_0000 -> 0x3FFF_FFFC
; 000000000 ,   00000000 , XXXXXXXXX , XXXXXXXXX , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:0 ,   9-bit:x ,   9-bit:x ,     12-bit:x
;  table1[0] -> table2[0] -> 0x0000_0000 (a 1GB identity block)
; Translates to the physical address 0x0000_0000 -> 0x3FFF_FFFC
;
  prep_test_case
  xor   r1, r1, r1
  mov   r0, 0x20000000
  ld    r1, [r0]
  cmp   r1, 0x20000000
  bne   @fail

; Test 02
; Page address translation
;
; Virtual address 0x4000_0000 -> 0x4000_0FFC
; 000000000 ,   00000001 ,  000000000 ,  000000000 , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:1 ,    9-bit:0 ,    9-bit:0 ,     12-bit:x
;  table1[0] -> table2[1] -> table3[0] -> table4[0] -> 0x2000_0000 (a 4KB page)
; Translates to the physical address 0x2000_0000 -> 0x2000_0FFC
;
  prep_test_case
  mov   r0, 0x40000000
  ld    r0, [r0]
  cmp   r0, 0x20000000
  bne   @fail

; Test 03
; Block address translation, different index in tables
;
; Virtual address 0x0080_2000_0000 -> 0x0080_3FFF_FFFC
; 000000000 ,   00000000 ,  xxxxxxxxx ,  xxxxxxxxx , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:0 ,    9-bit:x ,    9-bit:x ,     12-bit:x
;  table1[0] -> table2[0] -> 0x0000_0000 (a 1GB block)
; Translates to the physical address 0x0000_0000 -> 0x1FFF_FFFC
;
  prep_test_case
  movhl r0, 0x80
  orl   r0, r0, 0x20000000
  ld    r0, [r0]
  cmp   r0, 0x20000000
  bne   @fail

; Test 04
; Block address translation, non 0 untranslated bits
;
; Virtual address 0x0080_2000_0000 -> 0x0080_3FFF_FFFC
; 000000000 ,   00000000 ,  xxxxxxxxx ,  xxxxxxxxx , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:0 ,    9-bit:x ,    9-bit:x ,     12-bit:x
;  table1[0] -> table2[0] -> 0x0000_0000 (a 1GB block)
; Translates to the physical address 0x0000_0000 -> 0x1FFF_FFFC
;
  prep_test_case
  movhl r0, 0x80
  orl   r0, r0, 0x20001000
  ld    r0, [r0]
  cmp   r0, 0x20001000
  bne   @fail


; Test 05
; Page address translation, different index path in tables
;
; Virtual address 0xff80_4000_0000 -> 0xff80_4000_0FFC
;  111111111 ,   00000001 ,  000000000 ,  000000000 , XXXXXXXXXXXX
;  9-bit:511 ,    9-bit:1 ,    9-bit:0 ,    9-bit:0 ,     12-bit:x
; table1[511] -> table2[1] -> table3[0] -> table4[0] -> 0x2000_0000 (a 4KB page)
; Translates to the physical address 0x2000_0000 -> 0x2000_0FFC
;
  prep_test_case
  movhl r0, 0xff80
  orl   r0, r0, 0x40000000
  ld    r0, [r0]
  cmp   r0, 0x20000000
  bne   @fail

; Test 06
; Page address translation, different index path in tables
;
; Virtual address 0xff80_4000_2000 -> 0xff80_4000_2FFC
;  111111111 ,   00000001 ,  000000000 ,  000000010 , XXXXXXXXXXXX
;  9-bit:511 ,    9-bit:1 ,    9-bit:0 ,    9-bit:2 ,     12-bit:x
; table1[511] -> table2[1] -> table3[0] -> table4[2] -> 0x2000_2000 (a 4KB page)
; Translates to the physical address 0x2000_2000 -> 0x2000_2FFC
;
  prep_test_case
  movhl r0, 0xff80
  orl   r0, r0, 0x40002000
  ld    r0, [r0]
  cmp   r0, 0x20002000
  bne   @fail

; Test 07
; Access spanning two, shareable pages
;
; Virtual address 0x4000_3000 -> 0x4000_3FFC
; 000000000 ,   00000001 ,  000000000 ,  000000011 , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:1 ,    9-bit:0 ,    9-bit:3 ,     12-bit:x
;  table1[0] -> table2[1] -> table3[0] -> table4[3] -> 0x2003_0000 (a 4KB page)
; Translates to the physical address 0x2000_3000 -> 0x2000_3FFC
;
; Virtual address 0x4000_4000 -> 0x4000_4FFC
; 000000000 ,   00000001 ,  000000000 ,  000000100 , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:1 ,    9-bit:0 ,    9-bit:4 ,     12-bit:x
;  table1[0] -> table2[1] -> table3[0] -> table4[4] -> 0x2003_0000 (a 4KB page)
; Translates to the physical address 0x2000_4000 -> 0x2000_4FFC
;
prep_test_case
  mov r0, shareable_page_virt_addr
  add r0, r0, 0xFFC
  ldl  r1, [r0]
; Read value is expected to be last 4 bytes of the first page concatenated with
; the first 4 bytes of the second page, even if the pages don't follow each other
; in physical memory
  movhl r2, 0x20005000
  orl   r2, r2, 0x20003ffc
  cmpl   r1, r2
  bne   @fail


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
  print " MMUv48_01: bunch of good weather scenario tests for MMUv48\n"
  end
