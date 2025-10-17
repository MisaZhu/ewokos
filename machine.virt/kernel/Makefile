include config.mk

LDS = mkos.lds.S

ifeq ($(ARCH), aarch64)
	OS_IMG = kernel8.img
	OS_ELF = kernel8.elf
	QEMU_CMD = qemu-system-aarch64
	ARCH_CFLAGS = -Wa,-march=armv8-a
	QEMU_FLAGS = -M $(QEMU_MACHINE),highmem=off,accel=hvf -cpu host -m 512 -serial mon:stdio -device ramfb
	#QEMU_FLAGS = -M $(QEMU_MACHINE),highmem=off,secure=on -cpu cortex-a76 -m 512 -serial mon:stdio -device ramfb
else
	OS_IMG = kernel7.img
	OS_ELF = kernel7.elf
	QEMU_CMD = qemu-system-arm
	QEMU_FLAGS = -S -M $(QEMU_MACHINE) -cpu cortex-a15 -m 512 -serial mon:stdio -nographic
	ARCH_CFLAGS = -march=armv7ve
endif

ifeq ($(SMP), yes)
	QEMU_FLAGS += -smp 4
endif

EWOKOS_ROOT_DIR = ../..
SYS_DIR=../system
MACHINE=virt

ifeq ($(QEMU_DISPLAY_OPTS),cocoa)
QEMU_FLAGS += -display cocoa
endif


ARCH_LD_FLAGS = --defsym=start_address=$(LOAD_ADDRESS)

include make.bsp
include $(EWOKOS_ROOT_DIR)/kernel/platform/${ARCH}/make.rule

QEMU_ARGS += -kernel $(OS_IMG)
QEMU_ARGS += -drive file=$(SYS_DIR)/root_$(ARCH).ext2,format=raw,id=blk0,if=none -device virtio-blk-device,drive=blk0
QEMU_ARGS += -device virtio-tablet-device 
QEMU_ARGS += -device virtio-keyboard-device
QEMU_ARGS += -fsdev local,id=fsdev0,path=$(HOME),security_model=none -device virtio-9p-device,fsdev=fsdev0,mount_tag=hostshare

asm:
	$(OBJDUMP) -D $(BUILD_DIR)/$(OS_ELF) > $(BUILD_DIR)/kernel.asm

run: 
	$(QEMU_CMD) $(QEMU_FLAGS) $(QEMU_ARGS)

runasm: 
	$(QEMU_CMD) $(QEMU_FLAGS) $(QEMU_ARGS) -d in_asm

debug:
	$(QEMU_CMD) $(QEMU_FLAGS) -gdb tcp::26000 -S $(QEMU_ARGS)

debugasm:
	$(QEMU_CMD) $(QEMU_FLAGS) -d in_asm -gdb tcp::26000 -S $(QEMU_ARGS)

gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	arm-none-eabi-gdb $(BUILD_DIR)/$(OS_ELF) -x /tmp/gdbinit
	#gdb-multiarch $(BUILD_DIR)/$(OS_ELF) -x /tmp/gdbinit #for ubuntu 18.04 without arm-none-eabi-gdb

dts:
