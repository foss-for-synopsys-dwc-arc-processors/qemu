; check_mmuv48_02.S
;
; Bad-weather MMU Exception checks
;

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
; ,---------------------------------. ,------------------.
; | [ 0 ] table 2                   | | [ 0 ] xxxxx0     |
; |  ...  xxxxxxxxxxxxxxxxxxx0      | | [ 1 ] xxxxx0     |
; |  ...  xxxxxxxxxxxxxxxxxxx0      | | [ 2 ] table 3    |
; |  ...  xxxxxxxxxxxxxxxxxxx0      | |  ...  xxxxx0     |
; | [511] xxxxxxxxxxxxxxxxxxx0      | | [511] xxxxx0     |
; `---------------------------------' `------------------'
; ,--------------------------------------. ,---------------.
; | [ 0 ] 2MB page 0x8000_0000 AF AP1 V  | | [ 0 ] xxxxx0  |
; | [ 1 ] xxxxxxxxxxxxxxxxxxx0           | | [ 1 ] xxxxx0  |
; | [ 2 ] xxxxxxxxxxxxxxxxxxx0           | | [ 2 ] xxxxx0  |
; | [ 3 ] xxxxxxxxxxxxxxxxxxx0           | | [ 2 ] xxxxx0  |
; | [ 4 ] 2MB page 0x0040_0000 AF V      | | [ 4 ] xxxxx0  |
; | [ 5 ] 2MB page 0x0060_0000 V         | | [ 3 ] xxxxx0  |
; | [ 6 ] 2MB page 0x0260_0000 AF SH1 V  | | [ 3 ] xxxxx0  |
; | [ 7 ] 2MB page 0x0160_0000 AF V      | | [ 3 ] xxxxx0  |
; |  ...  xxxxxxxxxxxxxxxxxxx0           | |  ...  xxxxx0  |
; | [511] xxxxxxxxxxxxxxxxxxx0           | | [511] xxxxx0  |
; `--------------------------------------' `---------------'
;  table3@0x1000_2000                   table4@0x1000_3000
;
;     Code execution translations
;
  .equ new_ivt_phys_addr,            0x80000000
  .equ new_ivt_virt_addr,            0x80000000
;
; Virtual address 0x8000_0000 -> 0x801F_FFFC
; 000000000 , 0000000_10 ,  000000000 , XXXXXXXXX , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:2 ,    9-bit:0 ,   9-bit:x ,     12-bit:x
;  table1[0] -> table2[2] -> table2[3] -> 0x8000_0000 (a 2MB identity block)
; Translates to the physical address 0x8000_0000 -> 0x801F_FFFC
;
;

  .equ rw_noexec_phys_addr,          0x00400000
  .equ rw_noexec_virt_addr,          0x80800000

  .equ noaccess_phys_addr,           0x00600000
  .equ noaccess_virt_addr,           0x80a00000

  .equ shareable_page_phys_addr,     0x02600000
  .equ shareable_page_virt_addr,     0x80c00000

  .equ nonshareable_page_phys_addr,  0x01600000
  .equ nonshareable_page_virt_addr,  0x80e00000

.data
.section .data.pagetables
; 0x10000000
pt_l1_base:
.8byte 0x10001000 | kTableValid
.space (0x1000 - 1 * 8)

.align 0x1000
; 0x10001000
pt_l2_base:
.8byte 0x0
.8byte 0x0
.8byte 0x10002000 | kTableValid
.space (0x1000 - 3 * 8)

.align 0x1000
; 0x10002000
pt_l3_base:
; IVT
; kAP1 required to make page non writable in user mode, and therefore be executable
; if WX is clear in CTRL register (and therefore exceptions handlers can run)
.8byte new_ivt_phys_addr | kAP1 | kAF | kBlockValid
.8byte 0x00000000
.8byte 0x00000000
.8byte 0x00000000
.8byte rw_noexec_phys_addr  | kAF  | kBlockValid
.8byte noaccess_phys_addr   | kBlockValid
.8byte shareable_page_phys_addr    | kAF | kSH1 | kBlockValid
.8byte nonshareable_page_phys_addr | kAF | kBlockValid
.space (0x1000 - 6 * 8)


.text
; Exception handler routines that jump to the address stored in handler_phys_addr
GENERAL_EXCEPTION_HANDLING EV_DMMUFault
GENERAL_EXCEPTION_HANDLING EV_IMMUFault
GENERAL_EXCEPTION_HANDLING EV_ProtV
GENERAL_EXCEPTION_HANDLING EV_MemoryError
GENERAL_EXCEPTION_HANDLING EV_MachineCheck

;;;;;;;;;;;;;;;;;;;;;;;;;;;; MMUv48_02 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Let the tests begin
  start

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Configuration step for all tests ;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
  ; IVT is going to be loaded at 0x8000_0000
  mov   r0, new_ivt_phys_addr
  sr    r0, [int_vector_base]

  ; write page table base
  mov   r0, pt_l1_base
  sr    r0, [MMU_RTP0]

  MMUV48_SET_TSZ_TO_16


; Test 01
; Invalid in RTPx Instruction Fetch
; VA: 0x0001_0000_0000_0000 -> Not all top 16 bits 0/1
;
  set_except_handler @test_01_tlbmissd_handler

  prep_test_case

  MMUV6_ENABLE

  movhl r0, 0xFFFE0000
test_01_tlbmissd:
  j     [r0]
  b     @fail

; Expected results:
test_01_tlbmissd_handler:
  movl  r8,  EV_IMMUFAULT_8    ; ecr
  movhl r9,  0xFFFE0000        ; eret/efa
  movl  r10, @test_01_end      ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r9, r10

test_01_end:
  ; Fall through


; Test 02
; Invalid in RTPx Data Read Access
; VA: 0x0001_0000_0000_0000 -> Not all top 16 bits 0/1
;
  set_except_handler @test_02_tlbmissd_handler

  prep_test_case

  MMUV6_ENABLE

  movhl r0, 0x00010000
test_02_tlbmissd:
  ld    r0, [r0]
  b     @fail

; Expected results:
test_02_tlbmissd_handler:
  movl  r8,  EV_DMMUFAULT_8     ; ecr
  movl  r9,  @test_02_tlbmissd ; eret
  movhl r10, 0x00010000        ; efa
  movl  r11, @test_02_end      ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r10, r11

test_02_end:
  ; Fall through



; Test 03
; Translation Fault Exception on Data Read
;
; Virtual address 0x1_0000_0000
; 000000000 ,  000000100 , 000000000 , 000000000 , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:4 ,   9-bit:0 ,   9-bit:x ,     12-bit:x
;  table1[0] -> table2[4] -> No translation possible -> Exception
;
;  Translation Fault Exception on Data Address


  set_except_handler @test_03_tlbmissd_handler

  prep_test_case

  MMUV6_ENABLE

  movhl r0, 0x1
test_03_tlbmissd:
  ld    r0, [r0]
  b     @fail

; Expected results:
test_03_tlbmissd_handler:
  movl  r8,  EV_DMMUFAULT_1    ; ecr
  movl  r9,  @test_03_tlbmissd ; eret
  movhl r10, 0x01              ; efa
  movl  r11, @test_03_end      ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r10, r11

test_03_end:
  ; Fall through



; Test 04
; Translation Fault Exception on Instruction Fetch
; Invalid in RTPx
;
; Virtual address 0x1_0000_0000
; 000000000 ,  000000100 , 000000000 , 000000000 , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:4 ,   9-bit:0 ,   9-bit:x ,     12-bit:x
;  table1[0] -> table2[4] -> No translation possible -> Exception
;
;  Translation Fault Exception on Instruction Address

  set_except_handler @test_04_tlbmissi_handler

  prep_test_case

  MMUV6_ENABLE

test_04_tlbmissi:
  movhl  r0, 0x1                  ; dest (invalid translation)
  j      [r0]

; Expected results:
test_04_tlbmissi_handler:
  movl  r8,  TLB_MISS_I        ; ecr
  movhl r9,  0x1               ; eret (virtual address)
  movl  r11, @test_04_end      ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r9, r11

test_04_end:
  ; Fall through




; Test 05
; Access Flag Exception on Data Access
;
; Virtual address 0x80A0_0000 -> 0x80BF_FFFF
; 000000000 ,  000000010 ,  000000101 , XXXXXXXXX , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:2 ,    9-bit:5 ,   9-bit:x ,     12-bit:x
;  table1[0] -> table2[2] -> table2[5] -> 0x00600000
; Translates to the physical address 0x0060_0000 -> 0x007f_ffff
;
  set_except_handler @test_05_tlbmissd_handler

  prep_test_case

  MMUV6_ENABLE

test_05:
  ld    r0, [noaccess_virt_addr]
  b     @fail

; Expected results:
test_05_tlbmissd_handler:
  movl  r8,  EV_DMMUFAULT_10     ; ecr
  movl  r9,  test_05             ; eret
  movl  r10, noaccess_virt_addr  ; efa
  movl  r11, @test_05_end        ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r10, r11

test_05_end:
  ; Fall through



; Test 06
; Access Flag Exception on Instruction Fetch.
;
; Virtual address 0x80A0_0000 -> 0x80BF_FFFF
; 000000000 ,  000000010 ,  000000101 , XXXXXXXXX , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:2 ,    9-bit:5 ,   9-bit:x ,     12-bit:x
;  table1[0] -> table2[2] -> table2[5] -> 0x00600000
; Translates to the physical address 0x0060_0000 -> 0x007f_ffff
;
  set_except_handler @test_06_tlbmissd_handler

  prep_test_case

  MMUV6_ENABLE

test_06:
  j    @noaccess_virt_addr
  b    @fail

; Expected results:
test_06_tlbmissd_handler:
  movl  r8,  EV_IMMUFAULT_10     ; ecr
  movl  r9,  noaccess_virt_addr  ; eret
  movl  r10, noaccess_virt_addr  ; efa
  movl  r11, @test_06_end        ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r10, r11

test_06_end:
  ; Fall through



; Test 07
; Protection Violation Instruction Fetch
;  Target page is user read/writable, so clearing WX when writing to CTRL
;  register should lead to an exception when jumping to that page
;
; Virtual address 0x8080_0000 -> 0x809F_FFFF
; 000000000 ,  000000010 ,  000000100 , XXXXXXXXX , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:2 ,    9-bit:4 ,   9-bit:x ,     12-bit:x
;  table1[0] -> table2[2] -> table2[4] -> 0x0040_0000
; Translates to the physical address 0x0040_0000 -> 0x005f_ffff
;
  set_except_handler @test_07_tlbmissd_handler

  prep_test_case

test_07:
; Copy code into writable page, with AP1 at 0
  mov    r1, rw_noexec_phys_addr  ; dest
  mov    r2, test_07_code         ; source
  ld     r3, [test_07_code_size]  ; size to copy

  COPY r1, r2, r3

  mov    r0, 0b001         ; WX=0, Enable=1
  sr     r0, [MMU_CTRL]     ; Enable MMU again


; Try to fetch instruction from writable page, with WX=0
  j rw_noexec_virt_addr

test_07_code:
  j     @fail

test_07_code_size:
  .word . - @test_07_code

; Expected results:
test_07_tlbmissd_handler:
  movl  r8,  EV_PROTV_8          ; ecr
  movl  r9,  rw_noexec_virt_addr ; eret/efa
  movl  r11, @test_07_end        ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r9, r11

test_07_end:
  ; Fall through


; Test 08
; Data Access Spanning Multiple Data Memory Targets
; First page is shareable, but the one that follows in the table isn't
;
; Virtual address 0x80C0_0000 -> 0x80DF_FFFF
; 000000000 ,  000000010 ,  000000110 , XXXXXXXXX , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:2 ,    9-bit:6 ,   9-bit:x ,     12-bit:x
;  table1[0] -> table2[2] -> table2[6] -> 0x02600000
; Translates to the physical address 0x0260_0000 -> 0x0261_ffff
;

  set_except_handler @test_08_tlbmissd_handler

  prep_test_case

  MMUV6_ENABLE

  mov r0, shareable_page_virt_addr
  add r0, r0, 0x1FFFFC

test_08:
  ; Load crosses page boundary into non-shareable page
  ldl  r1, [r0]

  b    @fail

; Expected results:
test_08_tlbmissd_handler:
  movl  r8,  EV_MEMORY_ERROR_12  ; ecr
  movl  r9,  test_08             ; eret
  movl  r10, r0                  ; efa
  movl  r11, @test_08_end        ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r10, r11

test_08_end:
  ; Fall through



; Test 09
; Double MMU exception leads to MachineCheck

  ; Setup exception throwing exception handler
  set_except_handler @test_09_exception_thrower

  prep_test_case

  MMUV6_ENABLE

test_09:
  ; Throw invalid address exception
  movhl  r0, 0x1                  ; dest (invalid translation)
  j      [r0]
  b    @fail

test_09_exception_thrower:
  ; Set non faulty handler
  set_except_handler @test_09_tlbmissd_handler

  ; Rethrow exception
  MMUV6_ENABLE
  lrl   r12, [efa]
  j     [r12]

; Expected results:
test_09_tlbmissd_handler:
  movl  r8,  EV_MachineCheck_0  ; ecr
  movl  r9,  r0                 ; eret/efa
  movl  r11, @test_09_end       ; new eret

  BASE_EXCEPTION_CHECK  r8, r9, r9, r11

  ; In a Machine Check, MMU should become disabled
	

test_09_end:
  ; Fall through


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
  print " MMUv48_02: bunch of bad weather scenario tests for MMUv48\n"
  end
