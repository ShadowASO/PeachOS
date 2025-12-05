; ------------------------------------------------------------------------------------------
; File: boot1.asm.
; Data: 26-11-2025
;
; O Boot1 é limitado a 512 bytes e é carregador automaticamente pela BIOS. 
; Ele existe apenas para carregar o boot2, que não está limintado a qualquer tamanho máximo.
; Isso permite que possamos realizar diversas verificações utilizando o BIOS para atualizar a 
; situação do processador.
;------------------------------------------------------------------------------------------

[BITS 16]
ORG 0x7c00


_start:
    jmp short stage0
    nop

;BIOS Parameter block
times 33 db 0
;------------------------------------

stage0:
    jmp 0x00:stage1

stage1:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [BootDrive], dl       ; salvar drive de boot (BIOS passa em DL)

    ; Mensagem de estágio 1
    mov si, msgStage1
    call print

    ; ----------------------------------------------------------------
    ; Carregar o segundo estágio a partir do disco
    ; Vamos assumir:
    ;  - Segundo estágio começa no setor 2 (CHS: cilindro 0, cabeça 0, setor 2)
    ;  - Tamanho: 4 setores (ajuste conforme o tamanho do seu stage2)
    ;  - Endereço de carga: 0000:8000 (0x8000 físico)
    ; ----------------------------------------------------------------
    mov ax, 0x0000
    mov es, ax
    mov bx, 0x8000            ; ES:BX = 0000:8000 (destino na RAM)

    mov ah, 0x02              ; função 02h: ler setores
    mov al, 2                 ; número de setores a ler (ajuste depois)
    mov ch, 0                 ; cilindro 0
    mov dh, 0                 ; cabeça 0
    mov cl, 2                 ; setor inicial = 2

    mov dl, [BootDrive]       ; drive de boot original
    int 0x13
    jc disk_error             ; se CF=1, erro de disco

    ; Se deu tudo certo, pula pro segundo estágio
    jmp 0x0000:0x8000         ; far jump para 0000:8000

disk_error:
    mov si, msgDiskError
    call print
.hang:
    hlt
    jmp .hang

; ------------------------------------------------
; Rotina print (string ASCIIZ em DS:SI)
; ------------------------------------------------
print:
.loop:
    lodsb
    test al, al
    jz .done
    call print_char
    jmp .loop
.done:
    ret

; ------------------------------------------------
; print_char: imprime AL na tela (modo texto)
; ------------------------------------------------
print_char:
    mov ah, 0x0E
    int 0x10
    ret

; ------------------------------------------------
; Dados
; ------------------------------------------------
msgStage1:    db 'Primeiro Estagio: carregando Stage 2...', 0x0D, 0x0A, 0
msgDiskError: db 'Erro ao ler o segundo estagio!', 0x0D, 0x0A, 0

BootDrive:    db 0

; ------------------------------------------------
; Boot signature (setor de 512 bytes)
; ------------------------------------------------
times 510 - ($ - $$) db 0
dw 0xAA55

