;section .asm
section .text

; paging.asm - rotinas auxiliares para CR3 e CR0
[BITS 32]

global paging_load_directory
global paging_enable
global invalid_tlb


; Carrega CR3 com o endereço físico do diretório de páginas
; USO: void paging_load_directory(uint32_t phys);
paging_load_directory:
    mov eax, [esp + 4]   ; argumento (phys)
    mov cr3, eax
    ret


; Liga o bit PG (bit 31) no CR0
; USO: void paging_enable(void);
paging_enable:
    mov eax, cr0
    or  eax, 0x80000000  ; seta bit PG
    mov cr0, eax
    ret

; USO: void paging_disable(void);
paging_disable:
    mov eax, cr0
    and eax, 0x7FFFFFFF ; Clear the PG bit (bit 31)
    mov cr0, eax
    ret


;// invalida TLB apenas de uma página
; USO: void invalid_tlb(uintptr_t va);
invalid_tlb:
    mov eax, [esp+4]
    invlpg [eax]
    ret

; Devolve o registro CR0
; USO: uint32_t get_cr0(void);
get_cr0:
    mov eax, cr0    
    ret

; x86 (32-bit), NASM
; int paging_is_on(void);
; EAX = 1 se paging ON (CR0.PG=1), EAX = 0 se OFF
global paging_is_on

paging_is_on:
    mov     eax, cr0
    bt      eax, 31          ; testa bit 31 (PG)
    setc    al               ; AL = 1 se PG=1 (carry=1), senão 0
    movzx   eax, al
    ret


section .note.GNU-stack noalloc noexec nowrite progbits