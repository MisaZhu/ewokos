include config.mk

LDS_QEMU = mkos.lds.qemu.S
LDS = mkos.lds.S

OS_IMG = kernel8.img
OS_ELF = kernel8.elf
OS_QEMU_IMG = kernel8.qemu.img
OS_QEMU_ELF = kernel8.qemu.elf

QEMU_CMD = qemu-system-aarch64
MACHINE=raspix-aarch64

EWOKOS_ROOT_DIR = ../../..

include make.bsp
include $(EWOKOS_ROOT_DIR)/kernel/platform/aarch64/make.rule

#QEMU_ARGS += -drive file=$(SYS_FULL_DIR)/root.ext2,if=virtio,format=raw
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
