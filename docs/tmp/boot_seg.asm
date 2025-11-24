; Para evitar erros, tais como o carregamento dos registros de segmentos com valores inconsistente,
; recomenda-se iniciarlizar os registros de segmento, como abaixo. Colocamos ORG==0 e atribuímos
; o valor inicial dos registros em 0x7C0. Esse é o valor correto a ser atribuído aos registros por-
; que ele será multiplicado por 16: 0x7C0 * 16 = [ 0x7C00]
ORG 0
BITS 16

jmp 0x7C0:start ; O bootloader é sempre carrgado no endereço 0x7c00. Assim, podemos saltar direta
                ; mente do DS:start, sendo DS==0x7c0 e start==endereço no código: 0x7c00


start:

    cli ; desabilita interrupções
    ; código
    mov ax, 0x07C0
    mov ds, ax
    mov es, ax
    ; stack
    mov ax, 0x00
    mov ss, ax
    mov sp, ax
    sti ; habilita interrupções

    mov si, msg
    call print

    jmp $  ;entra em loop, chamando a si mesmo($=linha atual)

print:
    mov bx,0
  .loop:
    lodsb
    cmp al,0
    je .done
    call print_char
    jmp .loop

  .done:
    ret

print_char:
    mov ah, 0eh   
    int 0x10  ; interrupção da BIOS
    ret

msg: db 'Hello World!', 0

times 510-($ - $$) db 0
dw 0xAA55