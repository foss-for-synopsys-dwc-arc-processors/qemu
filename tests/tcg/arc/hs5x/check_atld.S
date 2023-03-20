  .include "macros.inc"

.data
.align 0x4
some_data:
  .word 0x0
  .word 0xFFFFFFFF
test_nr:
  .word 0x0


.macro prep_test_case
    ld    r13, [test_nr]
    add_s r13, r13, 1       ; increase test case counter
    st    r13, [test_nr]
    add.f 0, 0, 1           ; (Z, N, C, V) = (0, 0, 0, 0)
.endm

; Setup the following:
;   r0 to have the data memory location
;   r1 to have the register value to use
;   r2 to have the memory value to use
;   [r0] = r2
;
.macro setup_values mem_val, reg_val
    mov r0, @some_data

    ; We set the top bits of the used registers to 1 to catch if the 32 bit
    ;  implementation is erroneously using them
    mov r1, \reg_val
    mov r2, \mem_val

    st r2, [@some_data]
.endm

; Run atld with specified operation and check 32 bit result in both register and
;  memory
.macro run_test op, result
    mov r3, \result

    ld    r4, [r0]

    atld.\op r1, [r0]

    ; Previous memory value was returned in register
    breq r1, r4, @1f
    j @fail

1:
    ; New memory value is as expected
    ld r4, [r0]
    breq r4, r3, @1f
    j @fail
1:
.endm


start

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; ATLD ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; Test case 1
; Basic add
; 1 + 2 = 3
;
prep_test_case

    setup_values 1, 2
    run_test add, 3

    setup_values 1, 2
    run_test add.aq.rl, 3

;
; Test case 2
; Basic or
; OR betwen 0b101010.. and 0b010101.. = 0xffffff
;
prep_test_case

    setup_values 0xaaaaaaaa, 0x55555555
    run_test or, 0xffffffff

    setup_values 0xaaaaaaaa, 0x55555555
    run_test or.aq.rl, 0xffffffff

;
; Test case 3
; Basic and
; AND betwen 0b101010.. and 0b010101.. = 0x000000
;
prep_test_case

    setup_values 0xaaaaaaaa, 0x55555555
    run_test and, 0x0

    setup_values 0xaaaaaaaa, 0x55555555
    run_test and.aq.rl, 0x0

;
; Test case 4
; Basic xor
; XOR betwen 0b101010.. and 0xffffff = 0b010101..
;
prep_test_case

    setup_values 0xaaaaaaaa, 0xffffffff
    run_test xor, 0x55555555

    setup_values 0xaaaaaaaa, 0xffffffff
    run_test xor.aq.rl, 0x55555555

;
; Test case 5
; Basic minu
; A negative number, when seen as unsigned, will always be bigger than positive
;  numbers
;
prep_test_case

    setup_values 1, -1
    run_test minu, 1

    setup_values 1, -1
    run_test minu.aq.rl, 1

;
; Test case 6
; Basic min
; Same input as before should provide opposite result
;
prep_test_case

    setup_values 1, -1
    run_test min, -1

    setup_values 1, -1
    run_test min.aq.rl, -1

;
; Test case 7
; Basic maxu
; A negative number, when seen as unsigned, will always be bigger than positive
;  numbers
;
prep_test_case

    setup_values 1, -1
    run_test maxu, -1

    setup_values 1, -1
    run_test maxu.aq.rl, -1

;
; Test case 8
; Basic max
; Same input as before should provide opposite result
; Swap input locations so that the -1 is now the value in memory
;
prep_test_case

    setup_values -1, 1
    run_test max, 1

    setup_values -1, 1
    run_test max.aq.rl, 1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; ATLDL ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;




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
  print " atld\n"
  flag 1
  end
