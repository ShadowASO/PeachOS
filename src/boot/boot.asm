; Para evitar erros, tais como o carregamento dos registros de segmentos com valores inconsistente,
; recomenda-se iniciarlizar os registros de segmento, como abaixo. Colocamos ORG==0 e atribuímos
; o valor inicial dos registros em 0x7C0. Esse é o valor correto a ser atribuído aos registros por-
; que ele será multiplicado por 16: 0x7C0 * 16 = [ 0x7C00]

[BITS 16]
ORG 0x7c00

CODE_SEG equ gdt_code-gdt_start
DATA_SEG equ gdt_data-gdt_start

; ------------------------------------------------------------
; Área fixa em 0x9000:0000 para informações do E820
; ------------------------------------------------------------
E820_BASE_REAL    equ 0x9000 ; counter
; E820_BASE_LINEAR    equ 0x00090000 ; buf


    jmp short start
    nop

times 33 db 0

start:
    jmp 0x0:step2 ; O bootloader é sempre carrgado no endereço 0x7c00. Assim, podemos saltar direta
                ; mente do DS:start, sendo DS==0x7c0 e start==endereço no código: 0x7c00

step2:

    cli ; Clear Interrupts

    cld 

    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
      
    sti ; Enables Interrupts


.load_protected:

    cli ; Clear Interrupts

    ; Limpo a região E820 em 0x9000:0000 (2 bytes count + 1024 bytes buffer)
    mov ax, E820_BASE_REAL
    mov es, ax
    xor di, di              ; ES:DI = 0x9000:0000
    mov cx, 1024 + 2        ; count (2 bytes) + buffer (1024)
    call memzero

    ;Chama rotina x820 para gerar o mapa de memória
    call e820_collect
          
    mov si, msgProtegido
    call print

    ; Entra no modo protegido
    lgdt[gdt_descriptor]
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
        
    jmp CODE_SEG:load32  ;far jump para recarregar o CS

.hang:
    hlt
    jmp .hang


[BITS 32]

load32:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov ss, ax
                    
    ;Enable the A20 line
    in al, 0x92
    or al, 2
    out 0x92, al
    
    ;Carrega o kernel LBA
    mov eax, 1          ; Setor: número do setor de início (0 é o bootloader)
    mov ecx, 100        ; Total: total de setores para ler
    mov edi, 0x0100000  ; Endereço na memória para carregar os setores / 1MB
    call ata_lba_read

    ; Salta para o endereço de memória onde o kernel foi carregado   
    jmp CODE_SEG:0x0100000

.hang:
    hlt
    jmp .hang

ata_lba_read:
    mov ebx, eax    ; backup do LBA
    shr eax, 24
    or eax, 0xE0    ; Seleciona o master driver
    mov dx, 0x1F6
    out dx, al

    mov eax, ecx    ; total de setores para ler
    mov dx, 0x1F2
    out dx, al      ; finaliza enviando o total de setores a ler

    mov eax, ebx    ; restaura o backup do LBA
    mov dx, 0x1F3
    out dx, al      ; finaliza

    mov dx, 0x1F4
    mov eax, ebx
    shr eax, 8
    out dx, al

    mov dx, 0x1F5
    mov eax, ebx
    shr eax, 16
    out dx, al

    mov dx, 0x1F7
    mov al, 0x20
    out dx, al

.next_sector:
    push ecx

.try_again:
    mov dx, 0x1F7
    in al, dx
    test al, 8
    jz .try_again

    ; A leitura é feita a 256 words por vez
    mov ecx, 256
    mov dx, 0x1F0
    rep insw
    pop ecx
    loop .next_sector

    ret

[BITS 16]

; ============================================================
; Rotina E820 100% compatível (QEMU / BOCHS / HARDWARE REAL)
; Preenche mapa em ES:DI
; Formato da memória:
;   ES:0000 → uint16 count
;   ES:0002 → uint16 entry_size  (20 ou 24)
;   ES:0010 → primeira entrada (16-byte aligned)
; ============================================================

e820_collect:
    mov ax, E820_BASE_REAL
    mov es, ax

    ; zera header
    mov word [es:0], 0      ; count
    mov word [es:2], 0      ; entry_size

    ; endereço alinhado a 16 bytes
    mov di, 0x10          ; 16 decimal
    
    xor ebx, ebx            ; EBX=0 → primeira chamada

.e820_next:
    mov eax, 0xE820
    mov edx, 0x534D4150     ; "SMAP"
    mov ecx, 24             ; pedimos até 24 bytes (max ACPI 3.0)

    int 0x15
    jc .done                ; erro ou fim

    cmp eax, 0x534D4150
    jne .done               ; assinatura incorreta

    ; BIOS retornou ECX real
    ; --------------------------------------------------------
    ; normaliza ECX: mínimo 20, máximo 24
    ; --------------------------------------------------------
    cmp cx, 20
    jae .chk_up
    mov cx, 20
    jmp .sz_ok
.chk_up:
    cmp cx, 24
    jbe .sz_ok
    mov cx, 24
.sz_ok:

    ; --------------------------------------------------------
    ; salva entry_size apenas uma vez
    ; --------------------------------------------------------
    cmp word [es:2], 0
    jne .skip_entry_store
    mov [es:2], cx
.skip_entry_store:

    ; --------------------------------------------------------
    ; BIOS preencheu ES:DI com CX bytes
    ; --------------------------------------------------------
    add di, cx

    ; incrementa contador
    inc word [es:0]

    ; EBX=0 → acabou
    test ebx, ebx
    jnz .e820_next

.done:
    ret

.hang:
    hlt
    jmp .hang

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

msgReal: db 'Primeiro Estagio: OS em Modo Real!', 0x0D, 0x0A, 0
msgProtegido: db 'OS em Modo Protegido!', 0x0D, 0x0A, 0

; -------------------------------------------------------
; Zera CX bytes começando em ES:DI
; -------------------------------------------------------
memzero:
    xor ax, ax        ; AL = 0
    rep stosb         ; escreve AL em ES:DI, incrementa DI
    ret

;Tabela GDT
gdt_start:
gdt_null:
    dd 0x0
    dd 0x0

;offset 0x8
gdt_code:           ; CS SHOULD POINT TO THIS
    dw 0xffff       ; segment limit
    dw 0            ; base first 0-15 bits
    db 0            ; base 16-23 bits
    db 0x9a         ; acess byte
    db 11001111b    ; high 4 bits flags and the low 4 bits flags
    db 0            ; base 24-31 bits

;offset 0X10
gdt_data:           ; DS, SS, ES, FS, GS
    dw 0xffff       ; Segment limit first 0-15 bits
    dw 0            ; Base first 0-15 bits
    db 0            ; Base 16-23 bits
    db 0x92         ; Access byte
    db 11001111b    ; High 4 bits flags and the low 4 bits flags
    db 0            ; Base 24-31 bits

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start -1
    dd gdt_start



times 510-($ - $$) db 0
dw 0xAA55