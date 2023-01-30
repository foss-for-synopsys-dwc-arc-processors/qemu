; check_mmuv48_00.S
;
; Check if the MMU configuration is set for MMUv48 of MMUv6.

;;;;;;;;;;;;;;;;;;;;;;;;;;;; MMUv48_00 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;

  .include "macros.inc"

; Let the tests begin
  start

  ; Test 00
  ; MMU_BUILD must reflect MMUv48 flavor of MMUv6
  ; Version -> MMU_BUILD[31..24] = 0x6 (MMU version for HS6x)
  ; Type    -> MMU_BUILD[23..21] = 0x1 (MMUv48_4k)
  lr    r0, [mmu_build]
  lsr   r1, r0, 24                ;
  and   r1, r1, 0xff              ; 1) Version == 6
  cmp   r1, 0x10                  ;
  bne   @fail
  lsr   r1, r0, 21                ;
  and   r1, r1, 0b111             ; 2) Type == 1
  cmp   r1, 0x1                   ;
  bne   @fail
  ; fall-through

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Reporting ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

valhalla:
  print "[PASS]"
  mov   r1, 0xdecaf                ; for now: good value in nSIM tracing
  b @1f

fail:
  mov   r1, 0xbadcafe              ; for now: bad value in nSIM tracing
  print "[FAIL]"

1:
  ;flag  0x1                        ; for now: nSIM ends here
  ;brk
  print " MMUv48_00: MMUv48 type of MMUv6\n"
  end
