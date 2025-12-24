;section .asm
section .text

extern int21h_handler
extern no_interrupt_handler

global idt_load


idt_load:
    push ebp
    mov ebp, esp ; o ponteiro da stack aponta para o endereço de retorno

    mov ebx, [ebp+8] ; volta 8 bytes na stack e pegar o primeiro argumento passado(endereço da IDT)
    lidt [ebx]
    pop ebp
    ret

