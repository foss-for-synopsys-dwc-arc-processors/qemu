.include "macros.inc"

.equ  IVT_ADDR,   0x0000000 ;IVTaddress
.equ  IRQ_Number, 32

start

mov r0, IVT_ADDR
sr  r0, [REG_IVT_BASE]
add r0, r0, IRQ_Number*4
mov r1, @IRQHandle
st  r1, [r0]

mov r2, IRQ_Number

sr   r2, [REG_IRQ_SELECT]
mov r0, 0
sr   r0, [REG_IRQ_PRIORITY]

seti
lr   r10,[status32]
sr   r2,[REG_IRQ_HINT]

mov r0,0
mov r1,2

.Lloop:
cmp r0, r1
beq.d    @valhalla
add r0, r0, 1
b    @.Lloop

j    @valhalla

IRQHandle:
;Deactivate interrupt and return
clri
mov r0, 0
sr   r0, [REG_IRQ_HINT]
rtie

valhalla:
print "[PASS]"
b    @1f

;If a test fails, it jumps here
fail:
print "[FAIL]"
1:
print "interrupt before delayslot\n"
flag 1
end