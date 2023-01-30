;;;;;;;;;;;;;;;;;;;;;;;;;;;; Constants for MMUv48 ;;;;;;;;;;;;;;;;;;;;;;;;;;;

  .equ kBlockValid , 0b01
  .equ kTableValid , 0b11
  .equ kPageValid  , 0b11

  .equ kAF         , (0x1 << 10)

  .equ kAP0        , (0x1 << 6)
  .equ kAP1        , (0x1 << 7)

  .equ kSH0        , (0x1 << 8)
  .equ kSH1        , (0x1 << 9)

  .equ MMU_RTP0         , 0x460
  .equ MMU_RTP1         , 0x462
  .equ MMU_CMD          , 0x465
  .equ MMU_CTRL         , 0x468
  .equ MMU_TTBC         , 0x469
  .equ MMU_FAULT_STATUS , 0x46C

; Exceptions
.equ ILLEGAL_INSTRUCTION         , 0x00020000
.equ ILLEGAL_INSTRUCTION_SEQUENCE, 0x00020100
.equ MACHINE_CHECK               , 0x00030000
.equ TLB_MISS_I                  , 0x00040000
.equ TLB_MISS_D_READ             , 0x00050100
.equ MMU_PROTECTION_VIOLATION    , 0x00060008
.equ PRIVILEGE_VIOLATION         , 0x00070000
.equ SOFTWARE_INTERRUPT          , 0x00080000
.equ MISALIGNED_DATA_ACCESS      , 0x000D0000

;       Exception Vectors

; EV_MachineCheck
; Double Fault
.equ EV_MachineCheck_0  ,  0x030000
; Overlapping TLB Entries
.equ EV_MachineCheck_1  ,  0x030100
; Fatal TLB Error
.equ EV_MachineCheck_2  ,  0x030200
; Fatal Cache Error
.equ EV_MachineCheck_3  ,  0x030300
; Internal Memory Error on Instruction Fetch
.equ EV_MachineCheck_4  ,  0x030400
; ....


;  EV_Memory_Error
; Data access spanning multiple data memory targets
.equ EV_MEMORY_ERROR_12  ,  0x011200
; ....

;  EV_IMMUFAULT
; Translation fault exception on Instruction Fetch
.equ EV_IMMUFAULT_0      ,  0x040000
; Translation fault exception on Invalid Instruction Address
.equ EV_IMMUFAULT_8      ,  0x040800
; Access flag exception on Instruction Fetch
.equ EV_IMMUFAULT_10     ,  0x041000
; Illegal Instruction Fetch ICCM translation
.equ EV_IMMUFAULT_20     ,  0x042000


;  EV_DMMUFAULT
; Translation fault exception on Data Memory Read
.equ EV_DMMUFAULT_1      ,  0x050100
; Translation fault exception on Data Memory Write
.equ EV_DMMUFAULT_2      ,  0x050200
; Translation fault exception on Data Memory Read-Modify-Write (EX/AMOs)
.equ EV_DMMUFAULT_3      ,  0x050300
; Translation fault exception on Invalid Data Address
.equ EV_DMMUFAULT_8      ,  0x050800
; Access flag exception on Data Access
.equ EV_DMMUFAULT_10     ,  0x051000
; Illegal Data Access ICCM translation
.equ EV_DMMUFAULT_20     ,  0x052000
; Illegal Data Access ICCM translation
.equ EV_DMMUFAULT_30     ,  0x053000

; EV_PROTV
; Instruction Fetch ProtectionViolation in MPU
.equ EV_PROTV_4      ,  0x060004
; Instruction Fetch ProtectionViolation in MMU
.equ EV_PROTV_8      ,  0x060008
; Memory Read (LD, POP, LEAVE, interrupt exit, LLOCK) protection
; violation in code protection scheme (parameter code 0x01)
.equ EV_PROTV_101    ,  0x060101
; ....

;;;;;;;;;;;;;;;;;;;;;;;;;; Macros related to MMUv6 ;;;;;;;;;;;;;;;;;;;;;;;;;;

; enable MMUv6. Clobbers r12
.macro MMUV6_ENABLE
  movl  r12, 0b101         ; WX=1, Enable=1
  sr    r12, [MMU_CTRL]
.endm

; disable MMUv6. Clobbers r12
.macro MMUV6_DISABLE
  movl  r12, 0
  sr    r12, [MMU_CTRL]
.endm

; Invalidate all TLB entries. Clobbers r12
.macro MMUV6_CLEARTLB
  movl  r12, 0x1
  sr    r12, [MMU_CMD]
.endm

; set both t0sz and t1sz to 16. Clobbers r12
.macro MMUV48_SET_TSZ_TO_16
  movl  r12, 0x100010       ; T0SZ=16
  sr    r12, [MMU_TTBC]      ; T1SZ=16
.endm

; set both t0sz and t1sz to 25. Clobbers r12
.macro MMUV48_SET_TSZ_TO_25
  movl  r12, 0x190019       ; T0SZ=25
  sr    r12, [MMU_TTBC]      ; T1SZ=25
.endm


; Basic exception handling code. Clobbers r12
;   Jumps to address stored in handler_phys_addr
;   Require ivt.S file to be compiled and linked.
.equ handler_phys_addr,       0x70400000

;;;;;;;;;;;;;;;;;;;;;;;;;;;; Exception handler ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; An exception handler routine that merely jumps to whatever address
; it was told to by the test. See set_except_handler macro
.macro set_except_handler   phys_addr
  mov  r11, \phys_addr
  st   r11, [handler_phys_addr]
.endm

.macro GENERAL_EXCEPTION_HANDLING name
  .align 8
  .global \name
  .type \name @function
\name:
  MMUV6_DISABLE
  ld    r12, [handler_phys_addr]
  j    [r12]
.endm

; Clobbers r12
.macro BASE_EXCEPTION_CHECK expect_ecr_reg expect_eret_reg expect_efa_reg target_ret_reg
  ; Check ecr value
  lr    r12, [ecr]
  cmp   r12, \expect_ecr_reg
  bne   @fail

  ; Check eret value
  lrl    r12, [eret]
  cmpl   r12, \expect_eret_reg
  bne   @fail

  ; Check efa value
  lrl   r12, [efa]
  cmpl  r12, \expect_efa_reg
  bne  @fail

  ; Success, return via target eret
  sr    \target_ret_reg, [eret]
  rtie
.endm


