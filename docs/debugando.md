#----------------- Iniciando em modo debugger

Executa a aplicação, da seguinte forma:

$ qemu-system-i386 -s -S -hda ./bin/os.bin

$#---------------  DEBUGGER
$ gdb

$ file build/kernelfull.o 0x100000

$ add-symbol-file build/kernelfull.o 0x100000

$ target remote localhost:1234

$ break _start

$ continue
