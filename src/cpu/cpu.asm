;section .asm
section .text

global _wait
global _pause


_wait:
.hang:
    ;cli   ; retirar no futuro para desabilitar as interrupções. mantido para debug no desenvolvimento
    hlt
    jmp .hang

_pause:
.hang:
    cli
    hlt
    jmp .hang