; check_mmuv48_03.S
;
; Check if the correct MMU_RTPx register is used based on VA.
; Stress test some table limits
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

.section .data.pagetables
; 0x10000000
pt0_l1_base:
.8byte 0x10001000 | kTableValid
.space (0x1000 - 2 * 8)
.8byte 0x10001000 | kTableValid

; 0x10001000
.align 0x1000
pt0_l2_base:
.8byte 0x40000000 | kAF | kBlockValid
.8byte 0x00000000
.8byte 0x10002000 | kTableValid
.space (0x1000 - 4 * 8)
.8byte 0x10002000 | kTableValid

; 0x10002000
.align 0x1000
pt0_l3_base:
.8byte 0x80000000 | kAF | kBlockValid
.space (0x1000 - 2 * 8)
.8byte 0x10003000 | kTableValid

; 0x10003000
.align 0x1000
pt0_l4_base:
.space (0x1000 - 1 * 8)
.8byte 0xD000 | kAF | kPageValid

; RTP1
;
;  table1@0x1000_4000                       table2@0x1000_5000
; ,------------------. ,--------------------------------------.
; | [  0] table2     | | [  0] table3                         |
; | [  1] xxxxx0     | | [  1] xxxxxxxxxxxxxxxxxxxxxxxx0      |
; |  ...  xxxxx0     | |  ...  xxxxxxxxxxxxxxxxxxxxxxxx0      |
; | [511] table2     | | [511] 1G block 0xF000_8000_0000 AF V |
; `------------------' `--------------------------------------'
; ,------------------. ,----------------------------------.
; | [  0] table4     | | [  0] page 0x0000_0000_C000 AF V |
; | [  1] xxxxx0     | | [  1] xxxxxxxxxxxxxxxxxxxx0      |
; |  ...  xxxxx0     | |  ...  xxxxxxxxxxxxxxxxxxxx0      |
; | [511] xxxxx0     | | [512] xxxxxxxxxxxxxxxxxxxx0      |
; `------------------' `----------------------------------'
;  table3@0x1000_6000                   table4@0x1000_7000
;
; 0x10004000
.align 0x1000
pt1_l1_base:
.8byte 0x10005000 | kTableValid
.space (0x1000 - 2 * 8)
.8byte 0x10005000 | kTableValid

; 0x10005000
.align 0x1000
pt1_l2_base:
.8byte 0x10006000 | kTableValid
.space (0x1000 - 2 * 8)
.8byte 0xF00080000000 | kAF | kBlockValid

; 0x10006000
.align 0x1000
pt1_l3_base:
.8byte 0x10007000 | kTableValid
.space (0x1000 - 1 * 8)

; 0x10007000
.align 0x1000
pt1_l4_base:
.8byte 0xC000 | kAF | kPageValid
.space (0x1000 - 1 * 8)

;----------------------------------------------------------------------

; If TxSZ are 25, there is a 3-level page table and the regions are:
;
; (T0SZ, T1SZ) = (25, 25)      3-level page table
; VA: 0x0000_0000_0000_0000 -> RTP0          Top 25 bits 0
; VA: 0x0000_007F_FFFF_FFFF -> RTP0          Top 25 bits 0

; VA: 0xFFFF_FF80_0000_0000 -> RTP1          Top 25 bits 1
; VA: 0xFFFF_FFFF_FFFF_FFFF -> RTP1          Top 25 bits 1

; VA: 0x0000_0080_0000_0000 -> EV_IMMUFault  Not all top 25 bits 0
; VA: 0xFFFF_FF00_0000_0000 -> EV_DMMUFault  Not all top 25 bits 1

; 6 13 13 [16]

; RTP0
; VA(0x0000_0000_4020_1000) -> PA(0x0000_0020_0000)
; VA(0x0000_007F_BFDF_E000) -> PA(0x0000_0000_B000)
;
;  table1@0x1000_8000                  table2@0x1000_9000
; ,------------------. ,---------------------------------.
; | [ 0 ] xxxxx0     | | [ 0 ] 2M block 0x8000_0000 AF V |
; | [ 1 ] table2     | | [ 1 ] 2M block 0x0020_0000 AF V |
; | [ 2 ] table2     | | [ 2 ] xxxxxxxxxxxxxxxxxxx0      |
; |  ...  xxxxx0     | |  ...  xxxxxxxxxxxxxxxxxxx0      |
; | [510] table2     | | [510] table3                    |
; | [511] xxxxx0     | | [511] xxxxxxxxxxxxxxxxxxx0      |
; `------------------' `---------------------------------'
; ,--------------------------------------. 
; | [ 0 ] xxxxxxxxxxxxxxxxxxxx0          |
; | [ 1 ] xxxxxxxxxxxxxxxxxxxx0          |
; |  ...  xxxxxxxxxxxxxxxxxxxx0          |
; | [510] 4KB page 0x0000_0000_B000 AF V |
; | [511] xxxxxxxxxxxxxxxxxxxx0          |
; `--------------------------------------'
;  table3@0x1000_a000
;
;     Code execution translations
;
; Virtual address 0x800X_XXXX -> 0x801X_XXXX
; 000000001 ,  000000000 , XXXXXXXXX , XXXXXXXXXXXX
;   9-bit:1 ,    9-bit:0 ,   9-bit:x ,     12-bit:x
;  table1[1] -> table2[0] -> 0x8000_0000 (a 2MB identity block)
; Translates to the physical address 0x800X_XXXX -> 0x801X_XXXX


; 0x10008000
.section .data.pagetables
.align 0x1000
pt2_l1_base:
.8byte 0x0
.8byte 0x10009000 | kTableValid
.8byte 0x10009000 | kTableValid
.space (0x1000 - 5 * 8)
.8byte 0x10009000 | kTableValid
.8byte 0x0

; 0x10009000
.align 0x1000
pt2_l2_base:
.8byte 0x80000000 | kAF | kBlockValid
.8byte 0x00200000 | kAF | kBlockValid
.space (0x1000 - 4 * 8)
.8byte 0x1000A000 | kTableValid
.8byte 0x0

; 0x1000A000
.align 0x1000
pt2_l3_base:
.space (0x1000 - 2 * 8)
.8byte 0xB000 | kAF | kPageValid
.8byte 0x0

; RTP1
; VA(0xFFFF_FF80_4020_1000) -> PA(0x0000_0000_A000)
; VA(0xFFFF_FFFF_FFFF_FFF8) -> PA(0xEE01_0040_0000)
;
;  table1@0x1000_b000                       table2@0x1000_c000
; ,-----------------. ,-----------------.
; | [  0] xxxxx0    | | [  0] xxxxx0    |
; | [  1] table2    | | [  1] table3    |
; |  ...  xxxxx0    | |  ...  xxxxx0    |
; | [510] xxxxx0    | | [510] xxxxx0    |
; | [511] table2    | | [511] xxxxx0    |
; `-----------------' `-----------------'
; ,-------------------------------------.
; | [  0] xxxxxxxxxxxxxxxxxxxx0         |
; | [  1] 2M page 0x0000_0000_A000 AF V |
; |  ...  xxxxxxxxxxxxxxxxxxxx0         |
; | [511] 2M page 0xEE01_0040_0000 AF V |
; `-------------------------------------'
;  table3@0x1000_d000
;
; 0x1000B000
.section .data.pagetables
.align 0x1000
pt3_l1_base:
.8byte 0x0
.8byte 0x1000C000 | kTableValid
.space (0x1000 - 3 * 8)
.8byte 0x1000c000 | kTableValid

; 0x1000C000
.align 0x1000
pt3_l2_base:
.8byte 0x0
.8byte 0x1000D000 | kTableValid
.space (0x1000 - 3 * 8)
.8byte 0x1000d000 | kTableValid

; 0x1000D000
.align 0x1000
pt3_l3_base:
.8byte 0x0
.8byte 0xA000 | kAF | kPageValid
.space (0x1000 - 3 * 8)
.8byte 0xEE0100400000 | kAF | kPageValid


;;;;;;;;;;;;;;;;;;;;;;;;;;;; MMUv48_03 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Let the tests begin
  start

  ; Planting data in physical addresess for the tests
  ; ,------.-----------------------.------------------.-----------------------.
  ; | test | virtual address       | physical address | data                  |
  ; |------+-----------------------+------------------+-----------------------|
  ; |  1a  | 0x0000_0000_0000_0000 | 0x0000_4000_0000 | 0x0123_4567_89ab_cdef |
  ; |  1b  | 0x0000_FFFF_FFFF_FFFc | 0x0000_0000_DFFc | 0x2A                  |
  ; |------+-----------------------+------------------+-----------------------|
  ; |  2a  | 0xFFFF_0000_0000_0000 | 0x0000_0000_C000 | 0xfedc_ba98_7654_3210 |
  ; |  2b  | 0xFFFF_FFFF_FFFF_FFFc | 0xF000_BFFF_FFFc | 0x42                  |
  ; |------+-----------------------+------------------+-----------------------|
  ; |  3a  | 0x0000_0000_4020_0000 | 0x0000_0020_0000 | 0x0000_1111_2222_3333 |
  ; |  3b  | 0x0000_007F_BFDF_E000 | 0x0000_0000_B000 | 0x3333_2222_1111_0000 |
  ; |------+-----------------------+------------------+-----------------------|
  ; |  4a  | 0xFFFF_FF80_4020_1004 | 0x0000_0000_A004 | 0x4444_5555           |
  ; |  4b  | 0xFFFF_FFFF_FFFF_FFF8 | 0xEE01_0040_0FF8 | 0x7777_6666_5555_4444 |
  ; `------^-----------------------^------------------^-----------------------'

  ; Test 1a data
  movhl r0, 0x01234567
  orl_s r0, r0, 0x89ABCDEF
  mov   r1, 0x40000000
  stl   r0, [r1]

  ; Test 1b data
  mov   r0, 0x2A
  mov   r1, 0xDFFC
  stb   r0, [r1]


  ; Test 2a data
  movhl r0, 0xFEDCBA98
  orl_s r0, r0, 0x76543210
  mov   r1, 0xC000
  stl   r0, [r1]

  ; Test 2b data
  mov   r0, 0x42
  movhl r1, 0xF000
  orl_s r1, r1, 0xBFFFFFFC
  stb   r0, [r1]


  ; Test 3a data
  movhl r0, 0x00001111
  orl_s r0, r0, 0x22223333
  mov   r1, 0x200000
  stl   r0, [r1]

  ; Test 3b data
  movhl r0, 0x33332222
  orl_s r0, r0, 0x11110000
  mov   r1, 0xB000
  stl   r0, [r1]


  ; Test 4a data
  mov   r0, 0x44445555
  mov   r1, 0xA004
  st    r0, [r1]

  ; Test 4b data
  movhl r0, 0x77776666
  orl_s r0, r0, 0x55554444
  movhl r1, 0xEE01
  orl_s r1, r1, 0x400FF8
  stl   r0, [r1]

  ; write page table base
  mov   r0, pt0_l1_base
  sr    r0, [MMU_RTP0]

  mov   r0, pt1_l1_base
  sr    r0, [MMU_RTP1]

  ; IVT is going to be loaded at 0x8000_0000
  mov   r0, 0x80000000
  sr    r0, [int_vector_base]

  MMUV48_SET_TSZ_TO_16
  MMUV6_ENABLE

; 1a
;
; Virtual address 0x0000_0000 -> 0x3FFF_FFFC
; 000000000 ,  000000000 , XXXXXXXXX , XXXXXXXXX , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:0 ,   9-bit:x ,   9-bit:x ,     12-bit:x
;  table1[0] -> table2[0] -> 0x4000_0000 (a 1GB identity block)
; Translates to the physical address 0x4000_0000 -> 0x7FFF_FFFC
;
prep_test_case
  xor   r1, r1, r1
  set_reg64 r0, 0x00000000, 0x00000000
  ldl    r1, [r0]
  chk_reg64 r1, 0x01234567, 0x89ABCDEF

j @fail
; 1b
;
; Virtual address 0x0000_FFFF_FFFF_F000 -> 0x0000_FFFF_FFFF_FFFC
;  111111111 ,    111111111 ,    111111111 , 111111111 , XXXXXXXXXXXX
;  9-bit:511 ,    9-bit:511 ,    9-bit:511 , 9-bit:511 ,     12-bit:x
; table1[511] -> table2[511] -> table3[511] -> 0x0000_D000 (a 4KB page)
; Translates to the physical address 0x0000_D000 -> 0x0000_DFFC
;
prep_test_case
  xor   r1, r1, r1
  set_reg64 r0, 0x0000FFFF, 0xfffffffc
  ld    r1, [r0]
  chk_reg64 r1, 0x00000000, 0x0000002A

; 2a
;
; Virtual address 0xFFFF_0000_0000_0000 -> 0xFFFF_0000_0000_0FFC (RTP1)
; 000000000 ,  000000000 ,  000000000 ,  000000000 , XXXXXXXXXXXX
;   9-bit:0 ,    9-bit:0 ,    9-bit:0 ,    9-bit:0 ,     12-bit:x
;  table1[0] -> table2[0] -> table3[0] -> table4[0] -> 0xC000 (a 4KB page)
; Translates to the physical address 0x0000_C000 -> 0x0000_CFFC
;
prep_test_case
  xor   r1, r1, r1
  set_reg64 r0, 0xFFFF0000, 0x00000000
  ldl    r1, [r0]
  chk_reg64 r1, 0xFEDCBA98, 0x76543210

; 2b
;
; Virtual address 0xFFFF_FFFF_C000_0000 -> 0xFFFF_FFFF_FFFF_FFFC (RTP1)
;   111111111 ,     111111111 ,  xxxxxxxxx , xxxxxxxxx , XXXXXXXXXXXX
;   9-bit:511 ,     9-bit:511 ,    9-bit:x ,   9-bit:x ,     12-bit:x
;  table1[511] -> table2[511] -> 0xF000_8000_0000 (a 1GB block)
; Translates to the physical address 0xF000_8000_0000 -> 0xF000_BFFF_FFFC
;
prep_test_case
  xor   r1, r1, r1
  set_reg64 r0, 0xFFFFFFFF, 0xFFFFFFFc
  ld    r1, [r0]
  dmach  r2, r3,r4
  chk_reg64 r1, 0x00000000, 0x42


  MMUV6_DISABLE
  ; Swap RTP0 and RTP1
  mov   r0, pt2_l1_base
  sr    r0, [MMU_RTP0]

  mov   r0, pt3_l1_base
  sr    r0, [MMU_RTP1]

  ; Set new TSZ
  MMUV48_SET_TSZ_TO_25
  ; Clear cache
  MMUV6_CLEARTLB

  MMUV6_ENABLE

; 3a
;
; Virtual address 0x4020_0000 -> 0x403F_FFFC
; 000000001 ,  000000001 , XXXXXXXXX , XXXXXXXXXXXX
;   9-bit:1 ,    9-bit:1 ,   9-bit:x ,     12-bit:x
;  table1[1] -> table2[1] -> 0x0020_0000 (2MB block)
; Translates to the physical address 0x0020_0000 -> 0x003F_FFFC
;
prep_test_case
  xor   r1, r1, r1
  set_reg64 r0, 0x00000000, 0x40200000
  ldl    r1, [r0]
  chk_reg64 r1, 0x00001111, 0x22223333

; 3b
;
; Virtual address 0x007F_BFDF_E000 -> 0x007F_BFDF_EFFC
;  111111110 ,    111111110 , 111111110 , XXXXXXXXXXXX
;  9-bit:510 ,    9-bit:510 , 9-bit:510 ,     12-bit:x
; table1[510] -> table2[510] -> table3[510] -> 0x0000_B000 (4KB page)
; Translates to the physical address 0x0000_B000 -> 0x0000_BFFC
;
prep_test_case
  xor   r1, r1, r1
  set_reg64 r0, 0x0000007F, 0xBFDFE000
  ldl    r1, [r0]
  chk_reg64 r1, 0x33332222, 0x11110000

; 4a
;
; Virtual address 0xFFFF_FF80_4020_1000 -> 0xFFFF_FF80_4020_1FFC (RTP1)
; 000000001 ,  000000001 ,  000000001 ,  XXXXXXXXXXXX
;   9-bit:1 ,    9-bit:1 ,    9-bit:1 ,      12-bit:x
;  table1[1] -> table2[1] -> table3[1] -> 0xA000 (a 4KB page)
; Translates to the physical address 0x0000_A000 -> 0x0000_AFFC
;
prep_test_case
  xor   r1, r1, r1
  set_reg64 r0, 0xFFFFFF80, 0x40201004
  ldl    r1, [r0]
  chk_reg64 r1, 0x00000000, 0x44445555

; 4b
;
; Virtual address 0xFFFF_FFFF_FFFF_F000 -> 0xFFFF_FFFF_FFFF_FFFC (RTP1)
;   111111111 ,    111111111 ,    111111111, XXXXXXXXXXXX
;   9-bit:511 ,    9-bit:511 ,    9-bit:511,     12-bit:x
;  table1[511] -> table2[511] -> table3[511] -> 0xEE01_0040_0000 (a 4KB page)
; Translates to the physical address 0xEE01_0040_0000 -> 0xEE01_0040_0FFC
;
prep_test_case
  xor   r1, r1, r1
  set_reg64 r0, 0xFFFFFFFF, 0xFFFFFFF8
  ldl    r1, [r0]
  chk_reg64 r1, 0x77776666, 0x55554444


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
  print " MMUv48_03: Choosing the MMU_RTPx register based on VA\n"
  end
