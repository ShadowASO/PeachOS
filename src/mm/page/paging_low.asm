section .asm

; paging.asm - rotinas auxiliares para CR3 e CR0
[BITS 32]

global paging_load_directory
global paging_enable

; void paging_load_directory(uint32_t phys);
; Carrega CR3 com o endereço físico do diretório de páginas
paging_load_directory:
    mov eax, [esp + 4]   ; argumento (phys)
    mov cr3, eax
    ret

; void paging_enable(void);
; Liga o bit PG (bit 31) no CR0
paging_enable:
    mov eax, cr0
    or  eax, 0x80000000  ; seta bit PG
    mov cr0, eax
    ret

section .note.GNU-stack noalloc noexec nowrite progbits