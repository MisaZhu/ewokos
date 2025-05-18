include config.mk

LDS_QEMU = mkos.lds.qemu.S
LDS = mkos.lds.S

ifeq ($(ARCH), aarch64)
	OS_IMG = kernel8.img
	OS_ELF = kernel8.elf
	QEMU_CMD = qemu-system-aarch64
	ARCH_CFLAGS = -Wa,-march=armv8-a
	QEMU_FLAGS = -M $(QEMU_MACHINE) -serial null -serial mon:stdio
else
	OS_IMG = kernel7.img
	OS_ELF = kernel7.elf
	QEMU_CMD = qemu-system-arm
	QEMU_FLAGS = -M $(QEMU_MACHINE) -serial mon:stdio
	ARCH_CFLAGS = -march=armv7ve
	#QEMU_ARGS += -device loader,file=$(OS_IMG),addr=$(LOAD_ADDRESS)
endif

EWOKOS_ROOT_DIR = ../../..
MACHINE=raspix

ifeq ($(QEMU_DISPLAY_OPTS),cocoa)
QEMU_FLAGS += -display cocoa
endif


ARCH_LD_FLAGS = --defsym=start_address=$(LOAD_ADDRESS)

include make.bsp
include $(EWOKOS_ROOT_DIR)/kernel/platform/${ARCH}/make.rule

QEMU_ARGS += -kernel $(OS_IMG)
QEMU_ARGS += -sd $(SYS_FULL_DIR)/root_$(ARCH).ext2

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
