include config.mk

LDS_QEMU = mkos.lds.qemu.S
LDS = mkos.lds.S

OS_IMG = kernel7.img
OS_ELF = kernel7.elf
OS_QEMU_IMG = kernel7.qemu.img
OS_QEMU_ELF = kernel7.qemu.elf

EWOKOS_ROOT_DIR = ../../../..
MACHINE=raspix

include make.bsp

ifeq ($(PI4),yes)
include make.bsp.pi4
else
include make.bsp.pi2.3
endif

include $(EWOKOS_ROOT_DIR)/kernel/platform/arm/make.rule

QEMU_ARGS += -sd $(SYS_FULL_DIR)/root.ext2

asm:
	$(OBJDUMP) -D $(BUILD_DIR)/$(OS_ELF) > $(BUILD_DIR)/kernel.asm

run: 
	$(QEMU_CMD) $(QEMU_FLAGS) -kernel $(BUILD_DIR)/$(OS_QEMU_IMG) $(QEMU_ARGS)

runasm: 
	$(QEMU_CMD) $(QEMU_FLAGS) -kernel $(BUILD_DIR)/$(OS_QEMU_IMG) $(QEMU_ARGS) -d in_asm

debug:
	$(QEMU_CMD) $(QEMU_FLAGS) -gdb tcp::26000 -S -kernel $(BUILD_DIR)/$(OS_QEMU_IMG) $(QEMU_ARGS)

debugasm:
	$(QEMU_CMD) $(QEMU_FLAGS) -d in_asm -gdb tcp::26000 -S -kernel $(BUILD_DIR)/$(OS_QEMU_IMG) $(QEMU_ARGS)

gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	arm-none-eabi-gdb $(BUILD_DIR)/$(OS_QEMU_ELF) -x /tmp/gdbinit
	#gdb-multiarch $(BUILD_DIR)/$(OS_QEMU_ELF) -x /tmp/gdbinit #for ubuntu 18.04 without arm-none-eabi-gdb
