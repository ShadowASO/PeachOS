section .asm

global _wait

_wait:
.hang:
    hlt
    jmp .hang

_pause:
.hang:
    hlt
    jmp .hang