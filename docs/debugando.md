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

#Verificando as seções do kernel

../../cross-compiler/ia32/bin/i686-pc-linux-gnu-readelf -S ./bin/kernel.elf


#Verificar uma FAT

dd if=/dev/zero of=fat.bin bs=1M count=16

mkfs.vfat \
  -F 16 \
  -n PEACHOS \
  -s 4 \
  -r 512 \
  -i 0x0000D105 \
  fat.bin


sudo mount -t vfat ./fat.bin /mnt/d
	sudo cp ./hello.txt /mnt/d	
	sudo umount /mnt/d

$ fsck.vfat -v os.bin

fsck.fat 4.2 (2021-01-31)
Checking we can access the last sector of the filesystem
Boot sector contents:
System ID "PEACHOS "
Media byte 0xf8 (hard disk)
       512 bytes per logical sector
      2048 bytes per cluster
       100 reserved sectors
First FAT starts at byte 51200 (sector 100)
         2 FATs, 16 bit entries
    131072 bytes per FAT (= 256 sectors)
Root directory starts at byte 313344 (sector 612)
       512 root directory entries
Data area starts at byte 329728 (sector 644)
      8031 data clusters (16447488 bytes)
63 sectors/track, 255 heads
         0 hidden sectors
     32768 sectors total
Both FATs appear to be corrupt. Giving up. Run fsck.fat with non-zero -F option.
aldenor@aldenor-MS-7E51:bin$ dd if=/dev/zero of=fat.bin bs=1M count=16
16+0 records in
16+0 records out
16777216 bytes (17 MB, 16 MiB) copied, 0,00808627 s, 2,1 GB/s
