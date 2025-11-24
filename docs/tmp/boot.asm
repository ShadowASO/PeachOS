ORG 0x7C00
BITS 16

start:
    mov ah, 0eh
    mov al, 'A'
    mov bx, 0
    int 0x10  ; interrupção da BIOS

    jmp $  ;entra em loop, chamando a si mesmo($=linha atual)

times 510-($ - $$) db 0
dw 0xAA55