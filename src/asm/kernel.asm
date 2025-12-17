
[BITS 32]

%include "./src/asm/config.inc"

global _kernel_start

;extern do_e820
extern kernel_main
extern video_init
extern idt_init
extern _kernel_phys_base
extern _kernel_virt_base
; Deve ser utilizada para encontrar a localização na memória física e só deve ser utilizada
; enquanto não ativada a paginação virtual. É necessário também o uso de high helf memóry
extern _kernel_high_offset
; ----------------------------------------------------------------------------------------
extern _kernel_stack_top
extern _kernel_stack_top_phys
;
extern page_directory
extern page_directory_phys
extern page_table_identity
extern page_table_identity_phys
extern page_table_high
extern page_table_high_phys

CODE_SEG equ 0x08
DATA_SEG equ 0x10
PDE_HIGH equ (0xC0000000 >> 22)   ; 768

;Buffer utilizado por E820 - Endereço linear no modo protegido
E820_BASE_LINEAR    equ 0x00090000 ; buffer do E820

; ------------------------------------------------------------
; Ponto de entrada baixo (1 MiB físico, mas linkado em alto)
; ------------------------------------------------------------
_kernel_start:     
    cli ;Desabilita as interrupções  até uma IDT seja instalada
    
    ; -----------------------------------------------------------------
    ; Inicializa segmentos
    ; -----------------------------------------------------------------
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
   
    ; -----------------------------------------------------------------
    ; Usa a pilha física contida n kernel e fornecida pelo linker
    ; -----------------------------------------------------------------
    mov esp, _kernel_stack_top_phys
    mov ebp, esp

    ; -----------------------------------------------------------------
    ; Debug: exibir endereço de qualquer rótulo ou rotina (paging desativada)
    ; ----------------------------------------------------------------- 
    ;mov eax, init_paging   
    ;mov ebx, print_debug_hex
    ;call ebx
             
    ;jmp $
    ; =================================================================
    ; INICIALIZA PAGING (execução física)
    ; =================================================================
    mov eax, init_paging    
    call eax

    ; -----------------------------------------------------------------
    ; Carrega CR3 com endereço físico do diretório de páginas
    ; -----------------------------------------------------------------
    mov eax, page_directory_phys
    mov cr3, eax

    ; -----------------------------------------------------------------
    ; Ativa paging
    ; -----------------------------------------------------------------
    mov eax, cr0
    or  eax, 0x80000001   ; PE já está 1, mas ok
    mov cr0, eax
           
    ; =================================================================
    ; SALTO FAR → Entrada em HIGH-HALF
    ; =================================================================   
    jmp CODE_SEG:high_entry

.hang:
    hlt
    jmp .hang
; ------------------------------------------------------------
; Entrada em high-half (agora executando em 0xC0xxxxxx)
; ------------------------------------------------------------
 high_entry:
          
    ; Ajusta a pilha para um endereço virtual alto, se quiser
    ; (assumindo que essa região está mapeada via .stack + paging)
    mov esp, _kernel_stack_top
    mov ebp, esp

    ; O Buffer de E820 está em local conhecido: 0x00090000. Salvo na stack 
    ; para uso pelo kernel
    mov eax, E820_BASE_LINEAR   ;0x00090000
    push eax   
       
    ;Chama a função inicial em "c"
    call kernel_main
    ;--------------------------------------------------
    ; Se voltar, apenas trave
.hang:
    hlt
    jmp .hang

; =====================================================================
; Rotina de criação das tabelas de página
; =====================================================================
init_paging: equ $ - KERNEL_OFFSET

    ; -------------------------------------------------
    ; Zera o diretório (físico)
    ; -------------------------------------------------
    mov edi, page_directory_phys
    mov ecx, 1024
    xor eax, eax
    rep stosd

    ; -------------------------------------------------
    ; Identity map 0–4 MiB
    ; -------------------------------------------------
    mov edi, page_table_identity_phys
    mov eax, 0x00000003      ; RW + PRESENT, base = 0
    mov ecx, 1024

.fill_identity:
    mov [edi], eax
    add eax, 0x1000
    add edi, 4
    loop .fill_identity

    ; PDE[0] = identity PT
    mov eax, page_table_identity_phys
    or eax, 0x3
    mov ebx, page_directory_phys
    mov [ebx + 0*4], eax

    ; -------------------------------------------------
    ; High-half mapeando kernel físico para 0xC0000000 - 0xC0400000(4MB)
    ; -------------------------------------------------
    mov edi, page_table_high_phys
    mov eax, _kernel_phys_base
    ;mov eax, 0x0  ; correção de um erro que deu muito trabalho
    or eax, 0x3
    mov ecx, 1024

.fill_high:
    mov [edi], eax
    add eax, 0x1000
    add edi, 4
    loop .fill_high

    ; PDE[768] = page_table_high
    mov eax, page_table_high_phys
    or eax, 0x3
    mov ebx, page_directory_phys
    mov [ebx + PDE_HIGH*4], eax

    ret


; =====================================================================
; Rotina de debug (escrita física, sem paging)
; =====================================================================
print_debug: equ $ - KERNEL_OFFSET
    pusha
    mov ebx, msg_debug
    mov edx, 0xb8000
.loop:
    mov al, [ebx]
    mov ah, 0x0F
    cmp al, 0
    je .done
    mov [edx], ax
    add ebx, 1
    add edx, 2
    jmp .loop

.done:
    popa
    ret

msg_debug: equ $ - KERNEL_OFFSET
         db "Debug - OK",0

; =====================================================================
; Debug (escrita física, sem paging) - Imprime números e emdereços HEXA
; =====================================================================
print_debug_hex:  equ $ - KERNEL_OFFSET
    pusha
     mov edx, 0xb8000
    mov ecx, 8         ; 8 dígitos hex
.hex_loop:  
    rol eax, 4
    mov bl, al
    and bl, 0xF        ; nibble
    cmp bl, 9
    jbe .digit
    add bl, 'A' - 10
    jmp .store
.digit:  
    add bl, '0'
.store:  
    mov [edx], bl
    add edx, 2
    loop .hex_loop
    popa
    ret

section .note.GNU-stack noalloc noexec nowrite progbits