# 2. Solução profissional: gerar um kernel.elf e um kernel.bin

# O padrão “profissional” é:

# Gerar um ELF com símbolos para o GDB (kernel.elf).

# Gerar a versão binária crua (kernel.bin) para o bootloader a partir desse ELF (via objcopy).

# Cross-compiler
CROSS_PATH = ../../cross-compiler/ia32/bin

CC      := $(CROSS_PATH)/i686-pc-linux-gnu-gcc
LD      := $(CROSS_PATH)/i686-pc-linux-gnu-ld
OBJCOPY := $(CROSS_PATH)/i686-pc-linux-gnu-objcopy
READELF := $(CROSS_PATH)/i686-pc-linux-gnu-readelf

# Diretórios principais
BINDIR   = ./bin
BUILDDIR = ./build

INCLUDES = -I./src

# Flags de compilação (kernel freestanding)
CFLAGS := -g -O0 -Wall -Werror \
	-ffreestanding -fno-builtin \
	-fno-omit-frame-pointer \
	-fno-asynchronous-unwind-tables -fno-unwind-tables \
	-fno-pic -fno-plt \
	-m32 -march=i686 \
	-std=gnu99 \
	-Wno-unused-function -Wno-unused-label -Wno-unused-parameter -Wno-cpp

# Flags do assembler (NASM)
ASFLAGS := -g

# Flags de link (ELF do kernel)
LDSCRIPT := ./src/linker.ld
LDFLAGS  := -m elf_i386 -T $(LDSCRIPT) -nostdlib -z noexecstack
# ---------------------------------------------------------------------
#   DETECÇÃO AUTOMÁTICA DE FONTES
# ---------------------------------------------------------------------

# Bootloader é especial
BOOT1_SRC = ./src/boot/boot1.asm
BOOT2_SRC = ./src/boot/boot2.asm

# Kernel.asm movido para src/asm/
KERNEL_ASM = ./src/asm/kernel.asm
KERNEL_ASM_OBJ = ./build/asm/kernel.o

# ★ ADICIONADO — stub baixo

# Detecta automaticamente todos os .c em src/ e subpastas
C_SRC := $(wildcard ./src/*.c ./src/*/*.c ./src/*/*/*.c)
C_OBJ := $(patsubst ./src/%.c,./build/%.o,$(C_SRC))

# Detecta todos os .asm (exceto boot.asm, kernel.asm e kernel_low.asm)
ASM_SRC := $(filter-out $(BOOT1_SRC) $(BOOT2_SRC)  $(KERNEL_ASM) , \
           $(wildcard ./src/*.asm ./src/*/*.asm ./src/*/*/*.asm))

ASM_OBJ := $(patsubst ./src/%.asm,./build/%.o,$(ASM_SRC))

# ★ ADICIONADO — inclui kernel_low.asm
ASM_OBJ +=  $(KERNEL_ASM_OBJ)

#Configuração do QEMU
QEMU_FLAGS = -s -S \
	-machine pc,accel=kvm \
	-cpu host \
	-m 4096 \
	-drive file=$(BINDIR)/os.bin,format=raw,if=ide,index=0,media=disk \
	-boot c \
	-display gtk \
	-serial stdio

# ---------------------------------------------------------------------
# TARGETS PRINCIPAIS
# ---------------------------------------------------------------------

.PHONY: all dirs clean run run64 inspect

all: dirs $(BINDIR)/boot1.bin $(BINDIR)/boot2.bin $(BINDIR)/kernel.bin $(BINDIR)/kernel.elf
	rm -f $(BINDIR)/os.bin
	dd if=$(BINDIR)/boot1.bin  >> $(BINDIR)/os.bin
	dd if=$(BINDIR)/boot2.bin  >> $(BINDIR)/os.bin
	dd if=$(BINDIR)/kernel.bin >> $(BINDIR)/os.bin
	dd if=/dev/zero bs=1048576 count=16 >> $(BINDIR)/os.bin


copyfile:
	@sudo mkdir -p /mnt/d
	sudo mount -t vfat ./bin/os.bin /mnt/d
	sudo cp ./hello.txt /mnt/d
	sudo cp ./hello2.txt /mnt/d
	sudo umount /mnt/d

dirs:
	mkdir -p $(BINDIR) $(BUILDDIR)

# Bootloader
$(BINDIR)/boot1.bin: $(BOOT1_SRC)
	nasm -f bin $(BOOT1_SRC) -o $(BINDIR)/boot1.bin

# Bootloader2
$(BINDIR)/boot2.bin: $(BOOT2_SRC)
	nasm -f bin $(BOOT2_SRC) -o $(BINDIR)/boot2.bin

# Kernel ELF com símbolos
$(BINDIR)/kernel.elf: $(ASM_OBJ) $(C_OBJ)
	$(LD) $(LDFLAGS) -o $@ $(ASM_OBJ) $(C_OBJ)

# Binário cru para o disco
$(BINDIR)/kernel.bin: $(BINDIR)/kernel.elf
#	objcopy -O binary $(BINDIR)/kernel.elf $(BINDIR)/kernel.bin
	$(OBJCOPY) --strip-debug -O binary $< $@

# ---------------------------------------------------------------------
# REGRAS GENÉRICAS
# ---------------------------------------------------------------------

./build/%.o: ./src/%.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

./build/%.o: ./src/%.asm
	mkdir -p $(dir $@)
	nasm -f elf $(ASFLAGS) $< -o $@

# Kernel.asm (src/asm/kernel.asm → build/asm/kernel.o)
$(KERNEL_ASM_OBJ): $(KERNEL_ASM)
	mkdir -p $(dir $@)
	nasm -f elf $(ASFLAGS) $< -o $@

# ---------------------------------------------------------------------
# UTILITÁRIOS
# ---------------------------------------------------------------------

inspect: $(BINDIR)/kernel.elf
	$(READELF) -S $(BINDIR)/kernel.elf
	$(READELF) -l $(BINDIR)/kernel.elf

run: 	
#	qemu-system-i386 -s -S -hda $(BINDIR)/os.bin
	qemu-system-i386 $(QEMU_FLAGS)

run64:
	qemu-system-x86_64 -s -S -hda $(BINDIR)/os.bin

# ---------------------------------------------------------------------
# LIMPEZA
# ---------------------------------------------------------------------

clean:
	rm -rf $(BINDIR) $(BUILDDIR)