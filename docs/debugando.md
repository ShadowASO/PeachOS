#----------------- Iniciando em modo debugger do kernel

# Primeiro, executa a aplicação, da seguinte forma:

$ qemu-system-i386 -s -S -hda ./bin/os.bin

#--------------- DEBUGANDO
# Abra um outro terminal, na raiz do projeto: "PeachOS"

# 1. Executa o GDB

$ gdb ./bin/kernel.elf

# 2. Dentro do terminal do GDB

(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue

(gdb) next


#------------------------

O kernel.elf foi linkado com KERNEL_VIRT_BASE = 0xC0000000,

E as seções têm os AT() corretos para o físico 0x00100000,

o GDB já sabe:

Que kernel_main está em algo como 0xC000xxxx.

O QEMU, após habilitar paging, vai reportar EIP nesses endereços.

Os breakpoints vão casar exatamente com o que está sendo executado.

Repare: com essa estratégia você não precisa mais de add-symbol-file manualmente. 
O kernel.elf já é o executável “oficial” para o GDB.

#--- IMPRIMIR variáveis
#--conteúdo em HEXA
$ x/64bx <variavel>
ou
$ print <variavel>
