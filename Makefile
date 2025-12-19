# 2. Solução profissional: gerar um kernel.elf e um kernel.bin

# O padrão “profissional” é:

# Gerar um ELF com símbolos para o GDB (kernel.elf).

# Gerar a versão binária crua (kernel.bin) para o bootloader a partir desse ELF (via objcopy).

# Cross-compiler
CROSS_PATH = ../../cross-compiler/ia32/bin

LD = $(CROSS_PATH)/i686-pc-linux-gnu-ld
CC = $(CROSS_PATH)/i686-pc-linux-gnu-gcc

# Diretórios principais
BINDIR   = ./bin
BUILDDIR = ./build

INCLUDES = -I./src

FLAGS = -g -ffreestanding -falign-jumps -falign-functions \
-falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer \
-finline-functions -Wno-unused-function -fno-builtin -Werror \
-Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib \
-nostartfiles -nodefaultlibs -Wall -O0 -Iinc

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

all: dirs $(BINDIR)/boot1.bin $(BINDIR)/boot2.bin $(BINDIR)/kernel.bin 
	rm -f $(BINDIR)/os.bin
	dd if=$(BINDIR)/boot1.bin  >> $(BINDIR)/os.bin
	dd if=$(BINDIR)/boot2.bin  >> $(BINDIR)/os.bin
	dd if=$(BINDIR)/kernel.bin >> $(BINDIR)/os.bin
#	dd if=/dev/zero bs=512 count=100 >> $(BINDIR)/os.bin	
	dd if=/dev/zero bs=1048576 count=16 >> $(BINDIR)/os.bin
	
copyfile:
	@sudo mkdir -p /mnt/d
	sudo mount -t vfat ./bin/os.bin /mnt/d
	sudo cp ./hello.txt /mnt/d
	sudo umount /mnt/d

dirs:
	mkdir -p $(BINDIR) $(BUILDDIR)

# Bootloader
$(BINDIR)/boot1.bin: $(BOOT1_SRC)
	nasm -f bin $(BOOT1_SRC) -o $(BINDIR)/boot1.bin

# Bootloader2
$(BINDIR)/boot2.bin: $(BOOT2_SRC)
	nasm -f bin $(BOOT2_SRC) -o $(BINDIR)/boot2.bin

# Gera o kernel (2-pass linking)
# $(BINDIR)/kernel.bin: $(ASM_OBJ) $(C_OBJ)
# 	$(LD) -g -relocatable $(ASM_OBJ) $(C_OBJ) -o $(BUILDDIR)/kernelfull.o
# 	$(CC) $(FLAGS) -T ./src/linker.ld -o $(BINDIR)/kernel.bin $(BUILDDIR)/kernelfull.o

# Kernel ELF com símbolos
$(BINDIR)/kernel.elf: $(ASM_OBJ) $(C_OBJ)
	$(LD) -g $(ASM_OBJ) $(C_OBJ) -T ./src/linker.ld -o $(BINDIR)/kernel.elf

# Binário cru para o disco
$(BINDIR)/kernel.bin: $(BINDIR)/kernel.elf
	objcopy -O binary $(BINDIR)/kernel.elf $(BINDIR)/kernel.bin


# ---------------------------------------------------------------------
# REGRAS GENÉRICAS
# ---------------------------------------------------------------------

./build/%.o: ./src/%.c
	mkdir -p $(dir $@)
	$(CC) $(INCLUDES) $(FLAGS) -std=gnu99 -c $< -o $@

./build/%.o: ./src/%.asm
	mkdir -p $(dir $@)
	nasm -f elf -g $< -o $@

# Kernel.asm (src/asm/kernel.asm → build/asm/kernel.o)
$(KERNEL_ASM_OBJ): $(KERNEL_ASM)
	mkdir -p $(dir $@)
	nasm -f elf -g $< -o $@


# ---------------------------------------------------------------------
# EXECUÇÃO EM QEMU
# ---------------------------------------------------------------------

run: 	
#	qemu-system-i386 -s -S -hda $(BINDIR)/os.bin
	qemu-system-i386 $(QEMU_FLAGS)

run64:
	qemu-system-x86_64 -s -S -hda $(BINDIR)/os.bin

# ---------------------------------------------------------------------
# LIMPEZA
# ---------------------------------------------------------------------

clean:
	rm -rf $(BINDIR)/*.bin
	rm -rf $(BINDIR)/os.bin
	rm -rf $(BUILDDIR)