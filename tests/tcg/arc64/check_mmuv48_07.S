; check_mmuv48_07.S
;
; A couple of tests checking EFA values for page fault exceptions triggered
; when executing pushdl/popdl instructions near page boundaries.
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
; | [     0    ]     | | [ 1 ] table 3                   |
; |  ...  xxxxx0     | | [ 2 ] 1G block 0x8000_0000 AF V |
; | [     0    ]     | |  ...  xxxxxxxxxxxxxxxxxxx0      |
; `------------------' `---------------------------------'
; ,------------------. ,----------------------------------.
; | [ 0 ] table 4    | | [ 0 ] 4KB page 0x2000_0000 AF V  |
; | [ 1 ] xxxxx0     | | [ 1 ] 4KB page 0x2000_1000 AF V  |
; | [ 2 ] xxxxx0     | | [ 2 ] 4KB page 0x2000_2000 AF V  |
; | [ 3 ] xxxxx0     | | [ 3 ] page hole                  |
; | [ 4 ] xxxxx0     | | [ 4 ] 4KB page 0x2000_4000 AF V  |
; |  ...  xxxxx0     | |  ...  xxxxxxxxxxxxxxxxxxxx0      |
; | [511] xxxxx0     | | [512] xxxxxxxxxxxxxxxxxxxx0      |
; `------------------' `----------------------------------'
;  table3@0x1000_2000                   table4@0x1000_3000
;
; Important translations are:
; 0x8000_0000 -> 0xBFFF_FFFF - code
; 0x4000_0000 -> 0x4000_5000 - range of stack pointer
;

.section .data.pagetables
; 0x10000000
pt_l1_base:
.8byte 0x10001000 | kTableValid
.space (0x1000 - 1 * 8)

.align 0x1000
; 0x10001000
pt_l2_base:
.8byte 0x00000000 | kAF | kBlockValid
.8byte 0x10002000 | kTableValid; 0x4000_0000 -> 0x7FFF_FFFC
.8byte 0x80000000 | kAF | kBlockValid
.space (0x1000 - 3 * 8)

.align 0x1000
; 0x10002000
pt_l3_base:
.8byte 0x10003000 | kTableValid; 0x4000_0000 -> 0x4001_0000
.space (0x1000 - 1 * 8)

.align 0x1000
; 0x10003000
pt_l4_base:
.8byte 0x20000000 | kAF | kPageValid
.8byte 0x20001000 | kAF | kPageValid
.8byte 0x20002000 | kAF | kPageValid
.8byte 0 ; empty page for boundary tests (0x4000_3000)
.8byte 0x20004000 | kAF | kPageValid
.space (0x1000 - 5 * 8)

.text
GENERAL_EXCEPTION_HANDLING EV_TLBMissD
GENERAL_EXCEPTION_HANDLING EV_Misaligned

;;;;;;;;;;;;;;;;;;;;;;;;;;;; MMUv48_07 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; set up exception handling
  mov	r0, 0x80000000
  sr	r0, [int_vector_base]

  ; write page table base
  mov   r0, pt_l1_base
  sr    r0, [MMU_RTP0]

  MMUV48_SET_TSZ_TO_16

; Test 01
; Normal pushdl/popdl execution inside a mapped region
;
  prep_test_case
  MMUV6_ENABLE

  mov sp, 0x400001000
  pushdl_s r0
  popdl_s r0
  xor.f 0, sp, 0x400001000
  bne @fail

  MMUV6_DISABLE

; Test 02
; popdl at the beginning of empty page
; expected EFA = boundary
  set_except_handler @test_02_tlbmissd_handler

  prep_test_case
  MMUV6_ENABLE

  mov   sp, 0x40003000
test_02_tlbmissd:
  popdl_s r0
  b     @fail

; Expected results:
test_02_tlbmissd_handler:
  movl  r8,  EV_DMMUFAULT_1     ; ecr
  movl  r9,  @test_02_tlbmissd ; eret
  movl  r10, 0x40003000        ; efa
  movl  r11, @test_02_end      ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r10, r11

test_02_end:
  ; Fall through

  MMUV6_DISABLE

; Test 03
; popdl at 4 bytes before beginning of empty page
; expected EFA = boundary
  set_except_handler @test_03_tlbmissd_handler

  prep_test_case
  MMUV6_ENABLE

  mov   sp, 0x40002ffc
test_03_tlbmissd:
  popdl_s r0
  b     @fail

; Expected results:
test_03_tlbmissd_handler:
  movl  r8,  EV_DMMUFAULT_1     ; ecr
  movl  r9,  @test_03_tlbmissd ; eret
  movl  r10, 0x40003000        ; efa
  movl  r11, @test_03_end      ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r10, r11

test_03_end:
  ; Fall through

  MMUV6_DISABLE

; Test 04
; popdl at 8 bytes before beginning of empty page
; expected EFA = boundary
  set_except_handler @test_04_tlbmissd_handler

  prep_test_case
  MMUV6_ENABLE

  mov   sp, 0x40002ff8
test_04_tlbmissd:
  popdl_s r0
  b     @fail

; Expected results:
test_04_tlbmissd_handler:
  movl  r8,  EV_DMMUFAULT_1     ; ecr
  movl  r9,  @test_04_tlbmissd ; eret
  movl  r10, 0x40003000        ; efa
  movl  r11, @test_04_end      ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r10, r11

test_04_end:
  ; Fall through

  MMUV6_DISABLE

; Test 05
; popdl at 12 bytes before beginning of empty page
; expected EFA = boundary
  set_except_handler @test_05_tlbmissd_handler

  prep_test_case
  MMUV6_ENABLE

  mov   sp, 0x40002ff4
test_05_tlbmissd:
  popdl_s r0
  b     @fail

; Expected results:
test_05_tlbmissd_handler:
  movl  r8,  EV_DMMUFAULT_1     ; ecr
  movl  r9,  @test_05_tlbmissd ; eret
  movl  r10, 0x40003000        ; efa
  movl  r11, @test_05_end      ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r10, r11

test_05_end:
  ; Fall through

  MMUV6_DISABLE

; Test 06
; pushdl 4 bytes after beginning of empty page
; expected EFA = boundary-12
  set_except_handler @test_06_tlbmissd_handler

  prep_test_case
  MMUV6_ENABLE

  mov   sp, 0x40004004
test_06_tlbmissd:
  pushdl_s r0
  b     @fail

; Expected results:
test_06_tlbmissd_handler:
  movl  r8,  EV_DMMUFAULT_2     ; ecr
  movl  r9,  @test_06_tlbmissd ; eret
  movl  r10, 0x40003ff4        ; efa
  movl  r11, @test_06_end      ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r10, r11

test_06_end:
  ; Fall through

  MMUV6_DISABLE

; Test 07
; pushdl 8 bytes after beginning of empty page
; expected EFA = boundary-8
  set_except_handler @test_07_tlbmissd_handler

  prep_test_case
  MMUV6_ENABLE

  mov   sp, 0x40004008
test_07_tlbmissd:
  pushdl_s r0
  b     @fail

; Expected results:
test_07_tlbmissd_handler:
  movl  r8,  EV_DMMUFAULT_2     ; ecr
  movl  r9,  @test_07_tlbmissd ; eret
  movl  r10, 0x40003ff8        ; efa
  movl  r11, @test_07_end      ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r10, r11

test_07_end:
  ; Fall through

  MMUV6_DISABLE

; Test 08
; pushdl 12 bytes after beginning of empty page
; expected EFA = boundary-4
  set_except_handler @test_08_tlbmissd_handler

  prep_test_case
  MMUV6_ENABLE

  mov   sp, 0x4000400c
test_08_tlbmissd:
  pushdl_s r0
  b     @fail

; Expected results:
test_08_tlbmissd_handler:
  movl  r8,  EV_DMMUFAULT_2     ; ecr
  movl  r9,  @test_08_tlbmissd ; eret
  movl  r10, 0x40003ffc        ; efa
  movl  r11, @test_08_end      ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r10, r11

test_08_end:
  ; Fall through

  MMUV6_DISABLE

; Test 09
; pushdl 1 byte after beginning of empty page
; MisalignedAccess exception
  set_except_handler @test_09_mc_handler

  prep_test_case
  MMUV6_ENABLE

  mov   sp, 0x40004001
test_09_mc:
  pushdl_s r0
  b     @fail

; Expected results:
test_09_mc_handler:
  movl  r8,  MISALIGNED_DATA_ACCESS    ; ecr
  movl  r9,  @test_09_mc               ; eret
  movl  r10, 0x40003ff1                ; efa
  movl  r11, @test_09_end              ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r10, r11

test_09_end:
  ; Fall through

  MMUV6_DISABLE

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
  print " MMUv48_07: pushdl/popdl MMU tests\n"
  end
