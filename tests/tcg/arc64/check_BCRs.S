; check_BCRs
; Check if the BCRs exist by loading all relevant ranges without exceptions
; being thrown
; As per the PRM
; "Auxiliary registers, in the range 0x60 to 0x7F, 0xC0 to 0xFF, and 0xF60 and
; 0xFFF are assumed to be BCRs.
; In kernel mode, any read from a non-existent build configuration register in
; these ranges returns 0, and no exception is generated. This design enables
; the kernel-mode code to detect the presence or absence of a BCR because all
; BCRs that are present in a system contain non-zero values."
;

.include "macros.inc"

.data
test_nr:
  .word 0x0

; Simple tests that registers exist
.macro test_bcr_from_to lower, upper
mov r0, \upper + 1
mov r1, \upper - \lower + 1

Loop_\lower:

sub r0, r0, 1
lr r2, [r0]

dbnz r1, @Loop_\lower

.endm

start

test_bcr_from_to 0x60 0x7F
test_bcr_from_to 0xC0 0xFF
; The quote above states "and 0xF60 and 0xFFF" but as the following is stated
;  as well, we assume 0xF60 and 0xFFF are limits in a range and not two
;  single registers
; "6.5.1 Build Configuration Registers Version, BCR_VER
; ...
;   0x3 = Indicates that the BCR Region 0x60-0x7F, 0xC0-0xFF,and 0xF60 – 0xFFF"

test_bcr_from_to 0xF60 0xFFF


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; ATLD ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


valhalla:
  print "[PASS]"
  b @1f

; If a test fails, it jumps here. Although, for the sake of uniformity,
; the printed output does not say much about which test case failed,
; one can uncomment the print_number line below or set a breakpoint
; here to check the R0 register for the test case number.
fail:
  ld  r0, [test_nr]
  ;print_number r0
  print "[FAIL]"
1:
  print " BCR check\n"
  flag 1
  end
