include config.mk

# Use bash so process substitution works for stderr filtering
SHELL := /bin/bash

LDS = mkos.lds.S

# Filter out harmless macOS Input Method Kit (IMK) console spam from QEMU's
# Cocoa display backend, e.g. "error messaging the mach port for
# IMKCFRunLoopWakeUpReliable". The serial console uses stdout, so filtering
# stderr keeps real QEMU errors visible while dropping this noise.
QEMU_STDERR_FILTER = 2> >(grep -v 'IMKCFRunLoopWakeUpReliable' >&2)

ifeq ($(ARCH), aarch64)
	OS_IMG = kernel8.img
	OS_ELF = kernel8.elf
	QEMU_CMD = qemu-system-aarch64
	ARCH_CFLAGS = -Wa,-march=armv8-a
	QEMU_DISPLAY = cocoa,zoom-to-fit=on,zoom-interpolation=on
	QEMU_FLAGS = -M $(QEMU_MACHINE),highmem=off,accel=hvf -cpu host -m 1024 \
		-serial mon:stdio -device ramfb \
		-display $(QEMU_DISPLAY)
else
	OS_IMG = kernel7.img
	OS_ELF = kernel7.elf
	QEMU_CMD = qemu-system-arm
	QEMU_DISPLAY = cocoa,zoom-to-fit=on,zoom-interpolation=on
	QEMU_FLAGS = -M $(QEMU_MACHINE) -cpu cortex-a15 -m 1024 -serial mon:stdio -device ramfb \
		-display $(QEMU_DISPLAY)
	ARCH_CFLAGS = -march=armv7ve -DEWOKOS_MACHINE_VIRT_ARM32
endif

ifeq ($(SMP), yes)
	QEMU_FLAGS += -smp 4
endif

EWOKOS_ROOT_DIR = ../..
MACHINE = virt
SYS_DIR = $(EWOKOS_ROOT_DIR)/system/build/$(MACHINE)

ifeq ($(QEMU_DISPLAY_OPTS),cocoa)
QEMU_DISPLAY = cocoa,zoom-to-fit=on,zoom-interpolation=on
endif

QEMU_AUDIO_ID ?= audio0
QEMU_AUDIODEV ?= coreaudio
QEMU_AUDIO_ARGS = -audiodev $(QEMU_AUDIODEV),id=$(QEMU_AUDIO_ID)
QEMU_NETDEV ?= user,id=net0,hostfwd=tcp:127.0.0.1:2222-:22,hostfwd=tcp:127.0.0.1:2323-:23
QEMU_NET_DEVICE ?= virtio-net-device,netdev=net0


ARCH_LD_FLAGS = --defsym=start_address=$(LOAD_ADDRESS)

include make.bsp
include $(EWOKOS_ROOT_DIR)/kernel/platform/${ARCH}/make.rule

ifeq ($(ARCH),arm)
QEMU_ARGS += -kernel $(OS_ELF)
else
QEMU_ARGS += -kernel $(OS_IMG)
endif
QEMU_ARGS += -drive file=$(SYS_DIR)/root_$(ARCH).ext2,format=raw,id=blk0,if=none -device virtio-blk-device,drive=blk0
QEMU_ARGS += -netdev $(QEMU_NETDEV) -device $(QEMU_NET_DEVICE)
QEMU_ARGS += -device virtio-tablet-device 
QEMU_ARGS += -device virtio-keyboard-device
QEMU_ARGS += $(QEMU_AUDIO_ARGS) -device virtio-sound-device,audiodev=$(QEMU_AUDIO_ID)
QEMU_ARGS += -fsdev local,id=fsdev0,path=$(HOME),security_model=none -device virtio-9p-device,fsdev=fsdev0,mount_tag=hostshare

asm:
	$(OBJDUMP) -D $(BUILD_DIR)/$(OS_ELF) > $(BUILD_DIR)/kernel.asm

run: 
	$(QEMU_CMD) $(QEMU_FLAGS) $(QEMU_ARGS) $(QEMU_STDERR_FILTER)

runasm: 
	$(QEMU_CMD) $(QEMU_FLAGS) $(QEMU_ARGS) -d in_asm $(QEMU_STDERR_FILTER)

debug:
	$(QEMU_CMD) $(QEMU_FLAGS) -gdb tcp::26000 -S $(QEMU_ARGS) $(QEMU_STDERR_FILTER)

debugasm:
	$(QEMU_CMD) $(QEMU_FLAGS) -d in_asm -gdb tcp::26000 -S $(QEMU_ARGS) $(QEMU_STDERR_FILTER)

gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	arm-none-eabi-gdb $(BUILD_DIR)/$(OS_ELF) -x /tmp/gdbinit
	#gdb-multiarch $(BUILD_DIR)/$(OS_ELF) -x /tmp/gdbinit #for ubuntu 18.04 without arm-none-eabi-gdb

dts:
