; check_mmuv48_06.S
;
; Test MMU TLB update sequence

  .include "macros.inc"
  .include "mmuv48.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;; Test checking routines ;;;;;;;;;;;;;;;;;;;;;;;;;;

; Test case counter (set in the code execution address so physical address
;  matches virtual address via MMU and we don't need to add extra logic
.equ test_nr, 0x80001000

; Increment the test counter and set (Z,N,C,V) to (0,0,0,0).
.macro prep_test_case
  ld    r13, [test_nr]
  add   r13, r13, 1       ; increase test case counter
  st    r13, [test_nr]
  add.f 0, 0, 1           ; (Z, N, C, V) = (0, 0, 0, 0)
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Tables for Tests ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  .equ table_copy_phys_addr,     0x00400000
  .equ data_phys_addr_1,         0x20000000
  .equ data_phys_addr_2,         0x30000000
  .equ data_phys_addr_3,         0x40000000
  .equ data_virt_addr,           0x80200000
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
; VA(0x0000_0000_0000_0000) -> PA(0x0000_4000_0000)
; VA(0x0000_FFFF_FFFF_FFFF) -> PA(0x0000_0000_D000)
;
;  table1@0x1000_0000                  table2@0x1000_1000
; ,------------------. ,---------------------------------.
; | [ 0 ] table2     | | [ 0 ] 1G block 0x4000_0000 AF V |
; | [ 1 ] xxxxx0     | | [ 1 ] xxxxxxxxxxxxxxxxxxx0      |
; | [ 2 ] xxxxx0     | | [ 2 ] table 3                   |
; |  ...  xxxxx0     | |  ...  xxxxxxxxxxxxxxxxxxx0      |
; | [511] table2     | | [511] table3                    |
; `------------------' `---------------------------------'
; ,---------------------------------. ,----------------------------------.
; | [  0] 2MB page 0x8000_0000 AF V | | [  0] xxxxxxxxxxxxxxxxxxxx0      |
; | [  1] xxxxxxxxxxxxxxxxxxx0      | | [  1] xxxxxxxxxxxxxxxxxxxx0      |
; |  ...  xxxxxxxxxxxxxxxxxxx0      | |  ...  xxxxxxxxxxxxxxxxxxxx0      |
; | [511] table4                    | | [512] page 0x0000_0000_D000 AF V |
; `---------------------------------' `----------------------------------'
;  table3@0x1000_2000                                   table4@0x1000_3000
;
;     Code execution translations
;
; Virtual address 0x800X_XXXX -> 0x801X_XXXX
; 000000000 , 0000000_10 ,  000000000 , XXXXXXXXX , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:2 ,    9-bit:0 ,   9-bit:x ,     12-bit:x
;  table1[0] -> table2[2] -> table3[0] -> 0x8000_0000 (a 2MB identity block)
; Translates to the physical address 0x800X_XXXX -> 0x801X_XXXX
; These are code execution addresses
;
; Virtual address 0x0XXX_XXXX -> 0xbXXX_XXXX
; 000000000 ,  000000000 , XXXXXXXXX , XXXXXXXXX , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:0 ,   9-bit:x ,   9-bit:x ,     12-bit:x
;  table1[0] -> table2[2] -> 0x5000_0000 (a 1GB block)
; Translates to the physical address 0x0XXX_XXXX -> 0x3XXX_XXXX
;

; 0x1000_0000
.section .data.pagetables
pt0_l1_base:
.8byte 0x10001000 | kTableValid
.space (512 - 1) * 8

; 0x1000_1000
.align 0x1000
pt0_l2_base:
.8byte 0x00000000
.8byte 0x00000000
.8byte 0x10002000 | kTableValid
.space (512 - 3) * 8

; 0x1000_2000
.align 0x1000
pt0_l3_base:
.8byte 0x80000000 | kAF | kBlockValid
.8byte data_phys_addr_1 | kAF | kBlockValid
.space (512 - 2) * 8

;
; Table copy. Only difference is the use of data_phys_addr_2 instead of
; data_phys_addr_1
;
; 0x1000_3000
.align 0x1000
pt1_l1_base:
.8byte 0x10004000 | kTableValid
.space (512 - 1) * 8

; 0x1000_4000
.align 0x1000
pt1_l2_base:
.8byte 0x00000000
.8byte 0x00000000
.8byte 0x10005000 | kTableValid
.space (512 - 3) * 8

; 0x1000_5000
.align 0x1000
pt1_l3_base:
.8byte 0x80000000 | kAF | kBlockValid
pt1_l3_data_address:
.8byte data_phys_addr_2 | kAF | kBlockValid
.space (512 - 2) * 8


;;;;;;;;;;;;;;;;;;;;;;;;;;;; MMUv48_03 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

; Test preparation

  ; IVT is going to be loaded at 0x8000_0000
  mov   r0, 0x80000000
  sr    r0, [int_vector_base]

; Write data to physical addresses
  set_reg64 r0, 0x01234567, 0x89abcdef
  stl        r0, [data_phys_addr_1]

  set_reg64 r0, 0xfedcba98, 0x76543210
  stl        r0, [data_phys_addr_2]

  set_reg64 r0, 0xBABABABA, 0xDCDCDCDC
  stl        r0, [data_phys_addr_3]

; Write page table base for initial table
  mov   r0, pt0_l1_base
  sr    r0, [MMU_RTP0]

  MMUV48_SET_TSZ_TO_16

; Test 01
; Normal MMU access
; Required for TLB to store translation
prep_test_case
  MMUV6_ENABLE

  ldl r1, [data_virt_addr]
  ; Validate value of physical address 1 via virtual address
  chk_reg64 r1, 0x01234567, 0x89abcdef

  MMUV6_DISABLE

; Test 02
; Change the root but dont reset TLB
  mov   r0, pt1_l1_base
  sr    r0, [MMU_RTP0]

prep_test_case
  MMUV6_ENABLE

  ; Verify that TLB is holding the cached physical address 1
  ldl r1, [data_virt_addr]
  ; Validate value of physical address 1 via virtual address
  chk_reg64 r1, 0x01234567, 0x89abcdef


; Test 03
; Reset TLB, physical address 2 should replace physical address 1
prep_test_case
  MMUV6_CLEARTLB
  sync

  ; TLB flushed, should now access physical address 2
  ldl r1, [data_virt_addr]
  ; Validate value of physical address 2 via virtual address
  chk_reg64 r1, 0xfedcba98, 0x76543210

  MMUV6_DISABLE

; Test 04
; Change entry (not root), but dont clear TLB
prep_test_case

  ; We forced addr3 > addr2, so we can just apply a `diff` to change the entry
  ; without worrying about the flags
  ld    r0, [pt1_l3_data_address]
  add   r0, r0, (data_phys_addr_3 - data_phys_addr_2)
  st    r0, [pt1_l3_data_address]

  MMUV6_ENABLE

  ; Verify that TLB is holding the cached physical address 2
  ldl r1, [data_virt_addr]
  ; Validate value of physical address 2 via virtual address
  chk_reg64 r1, 0xfedcba98, 0x76543210

; Test 05
; Reset TLB, physical address 3 should replace physical address 2
prep_test_case
  MMUV6_CLEARTLB
  sync

  ; TLB flushed, should now access physical addr3
  ldl r1, [data_virt_addr]
  ; Validate value of physical address 3 via virtual address
  chk_reg64 r1, 0xBABABABA, 0xDCDCDCDC

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
  print " MMUv48_06: Test TLB update sequence\n"
  end
