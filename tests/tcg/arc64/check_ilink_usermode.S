.include "macros.inc"

.data
.align 4
return_addr:
    .word @fail

.text
    .align 4
    .global EV_PrivilegeV
    .type EV_PrivilegeV, @function
EV_PrivilegeV:
    ld  r0, [@return_addr]
    sr  r0, [eret]
    rtie

start

; Test 1
; ilink change in kernel mode (initial mode) is allowed
; CPU exception not triggered
mov r0, @fail
st  r0, [@return_addr]

mov ilink, 0x91


; Test 2
; ilink change in user mode not allowed
; CPU exception is triggered
mov r0, @success
st  r0, [@return_addr]

enter_user_mode @usermode

j @fail

usermode:
    mov ilink, 0x91
    j @fail

success:
    mov r0, 0xdecaf
    print "[PASS] "
    b @1f

; If a test fails, it jumps here. Although, for the sake of uniformity,
; the printed output does not say much about which test case failed,
; one can uncomment the print_number line below or set a breakpoint
; here to check the R0 register for the test case number.
fail:
    mov r0, 0xbadc0ffe
    print "[FAIL] "
1:
    print " ilink in usermode\n"

end
