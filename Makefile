ARCH = x86_64

LD = ld
CC = gcc
AS = nasm

# QEMU = qemu-system-$(ARCH) -hdd $(HDD) -smp 1 -M q35 -soundhw pcspk -serial stdio
QEMU = qemu-system-$(ARCH) -hdd $(HDD) -smp 2 -M q35 -soundhw pcspk -serial stdio
QEMU_DOCKER = $(QEMU) -curses

HDD = mandelbrotos.hdd
KERNEL = $(BUILD_DIRECTORY)/mandelbrotos.elf

ASFLAGS = -f elf64 -O3

CFLAGS := \
	-mcmodel=kernel \
	-ffreestanding \
	-Isrc/include \
	-Wall \
	-Wextra \
	-Werror \
	-lm \
	-std=gnu99 \
	-O3 \
	-Isrc/include \
	-mgeneral-regs-only \
	-mno-red-zone \
	-pipe \
        -fno-pic -no-pie \
	-fno-stack-protector \
	-Wno-implicit-fallthrough \
	-Wno-maybe-uninitialized \
	-Wno-error=unused-parameter

LDFLAGS := \
	-static \
	-Tresources/linker.ld \
	-nostdlib \
	-z max-page-size=0x1000


CFILES := $(shell find src/ -name '*.c')
ASFILES := $(shell find src/ -name '*.asm')

all: clean $(OS) qemu
OBJ = $(patsubst %.c, $(BUILD_DIRECTORY)/%.c.o, $(CFILES)) \
        $(patsubst %.asm, $(BUILD_DIRECTORY)/%.asm.o, $(ASFILES))

BUILD_DIRECTORY := build
DIRECTORY_GUARD = @mkdir -p $(@D)

all: $(HDD) qemu

$(HDD): $(KERNEL)
	@ echo "[HDD]"
	@ dd if=/dev/zero of=$@ bs=1M seek=64 count=0
	@ parted -s $@ mklabel gpt
	@ parted -s $@ mkpart primary 2048s 100%
	@ ./resources/echfs-utils -g -p0 $@ quick-format 512
	@ ./resources/echfs-utils -g -p0 $@ import resources/limine.cfg boot/limine.cfg
	@ ./resources/echfs-utils -g -p0 $@ import resources/limine.sys boot/limine.sys
	@ ./resources/echfs-utils -g -p0 $@ import $< boot/kernel
	@ ./resources/limine-install $@

$(KERNEL): $(OBJ)
	@ echo "[LD] $^"
	@ $(LD) $(LDFLAGS) $^ -o $@

$(BUILD_DIRECTORY)/%.c.o: %.c
	@ echo "[CC] $^"	
	@ $(DIRECTORY_GUARD)
	@ $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIRECTORY)/%.asm.o: %.asm
	@ echo "[AS] $^"
	@ $(DIRECTORY_GUARD)
	@ $(AS) $(ASFLAGS) $< -o $@

clean:
	@ rm -rf $(BUILD_DIRECTORY) $(HDD)

qemu: $(HDD)
	@ $(QEMU)

qemu_docker: $(HDD)
	@ $(QEMU_DOCKER)

toolchain:
	# Limine
	@ echo "[BUILD LIMINE]"
	@ cp -R thirdparty/limine/limine-install-linux-x86_64 resources/limine-install
	@ cp -R thirdparty/limine/limine.sys resources
	@ cp -R thirdparty/limine/BOOTX64.EFI resources
	 
	# Echfs
	@ echo "[BUILD ECHFS]"
	@ cd thirdparty/echfs; make echfs-utils; make mkfs.echfs
	@ cp -R thirdparty/echfs/echfs-utils resources

toolchain_nonnative:
	# Limine
	@ echo "[BUILD LIMINE]"
	@ cp -R thirdparty/limine/limine-install-linux-x86_64 resources/limine-install
	@ cp -R thirdparty/limine/limine.sys resources
	@ cp -R thirdparty/limine/BOOTX64.EFI resources
	 
	# Echfs
	@ echo "[BUILD ECHFS]"
	@ cd thirdparty/echfs; make echfs-utils; make mkfs.echfs
	@ cp -R thirdparty/echfs/echfs-utils resources
	 
	# Cross compiler
	@ echo "[BUILD CROSS COMPILER]"
	@ ./cross-compiler.sh
	 
	# Set compiler 
	@ echo "[SET MAKEFILE]"
	@ sed -i 's/CC = gcc/CC = cross/bin/$(ARCH)-elf-gcc/g' Makefile
	@ sed -i 's/LD = ld/LD = cross/bin/$(ARCH)-elf-ld/g' Makefile

docker_build:
	@ docker build -f buildenv/Dockerfile . -t mandelbrot
	@ docker run -it mandelbrot make qemu_docker 
