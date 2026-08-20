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
	ARCH_CFLAGS = -Wa,-march=armv8-a -DMAX_USABLE_MEM_SIZE='(8ull*GB)'
	QEMU_GUI_DISPLAY ?= cocoa,zoom-to-fit=on,zoom-interpolation=on
	ifeq ($(PAGE_SIZE),64k)
		QEMU_RUN_ACCEL ?= tcg
		QEMU_RUN_CPU ?= cortex-a72
		QEMU_GUI_ACCEL ?= tcg
		QEMU_GUI_CPU ?= cortex-a72
	else
		QEMU_RUN_ACCEL ?= hvf
		QEMU_RUN_CPU ?= host
		QEMU_GUI_ACCEL ?= hvf
		QEMU_GUI_CPU ?= host
	endif
	QEMU_RUN_FLAGS = -M $(QEMU_MACHINE),highmem=on,accel=$(QEMU_RUN_ACCEL) -cpu $(QEMU_RUN_CPU) -m 8192 \
		-serial mon:stdio -nographic
	QEMU_GUI_FLAGS = -M $(QEMU_MACHINE),highmem=on,accel=$(QEMU_GUI_ACCEL) -cpu $(QEMU_GUI_CPU) -m 8192 \
		-serial mon:stdio -device ramfb \
		-display $(QEMU_GUI_DISPLAY)
else
	OS_IMG = kernel7.img
	OS_ELF = kernel7.elf
	QEMU_CMD = qemu-system-arm
	QEMU_GUI_DISPLAY ?= cocoa,zoom-to-fit=on,zoom-interpolation=on
	QEMU_RUN_FLAGS = -M $(QEMU_MACHINE) -cpu cortex-a15 -m 1024 -serial mon:stdio -nographic
	QEMU_GUI_FLAGS = -M $(QEMU_MACHINE) -cpu cortex-a15 -m 1024 -serial mon:stdio -device ramfb \
		-display $(QEMU_GUI_DISPLAY)
	ARCH_CFLAGS = -march=armv7ve -DEWOKOS_MACHINE_VIRT_ARM32
endif

ifeq ($(SMP), yes)
	QEMU_RUN_FLAGS += -smp 4
	QEMU_GUI_FLAGS += -smp 4
endif

EWOKOS_ROOT_DIR = ../..
MACHINE = virt
SYS_DIR = $(EWOKOS_ROOT_DIR)/system/build/$(MACHINE)

ifeq ($(QEMU_DISPLAY_OPTS),cocoa)
QEMU_GUI_DISPLAY = cocoa,zoom-to-fit=on,zoom-interpolation=on
endif

QEMU_AUDIO_ENABLE ?= yes
# Persist rootfs writes by default so ext2 changes made in the guest survive
# reboot. Use `make ... QEMU_SNAPSHOT=yes` when a disposable session is needed.
QEMU_SNAPSHOT ?= no
QEMU_AUDIO_ID ?= audio0
QEMU_AUDIODEV ?= coreaudio
QEMU_AUDIO_ARGS = -audiodev $(QEMU_AUDIODEV),id=$(QEMU_AUDIO_ID)
QEMU_HOSTFWD ?= hostfwd=tcp:127.0.0.1:2222-:22
ifeq ($(strip $(QEMU_HOSTFWD)),)
QEMU_NETDEV ?= user,id=net0
else
QEMU_NETDEV ?= user,id=net0,$(QEMU_HOSTFWD)
endif
QEMU_NET_DEVICE ?= virtio-net-device,netdev=net0


ARCH_LD_FLAGS = --defsym=start_address=$(LOAD_ADDRESS)
ifeq ($(ARCH),arm)
ARCH_LD_FLAGS += --defsym=kernel_base=0x80000000
else
ARCH_LD_FLAGS += --defsym=kernel_base=0x4000000000
endif
ifeq ($(PAGE_SIZE),16k)
ARCH_LD_FLAGS += --defsym=page_align_size=0x4000
else ifeq ($(PAGE_SIZE),64k)
ARCH_LD_FLAGS += --defsym=page_align_size=0x10000
else
ARCH_LD_FLAGS += --defsym=page_align_size=0x1000
endif

include make.bsp
include $(EWOKOS_ROOT_DIR)/kernel/platform/${ARCH}/make.rule

ifeq ($(ARCH),arm)
QEMU_ARGS += -kernel $(OS_ELF)
else
QEMU_ARGS += -kernel $(OS_IMG)
endif
ifeq ($(QEMU_SNAPSHOT),yes)
QEMU_ARGS += -snapshot
endif
QEMU_ARGS += -drive file=$(SYS_DIR)/root_$(ARCH).img,format=raw,id=blk0,if=none -device virtio-blk-device,drive=blk0
QEMU_ARGS += -netdev $(QEMU_NETDEV) -device $(QEMU_NET_DEVICE)
QEMU_ARGS += -fsdev local,id=fsdev0,path=$(HOME),security_model=none -device virtio-9p-device,fsdev=fsdev0,mount_tag=hostshare

QEMU_GUI_ARGS = $(QEMU_ARGS) -device virtio-tablet-device -device virtio-keyboard-device
ifeq ($(QEMU_AUDIO_ENABLE),yes)
QEMU_GUI_ARGS += $(QEMU_AUDIO_ARGS) -device virtio-sound-device,audiodev=$(QEMU_AUDIO_ID)
endif

asm:
	$(OBJDUMP) -D $(BUILD_DIR)/$(OS_ELF) > $(BUILD_DIR)/kernel.asm

run: 
	$(QEMU_CMD) $(QEMU_GUI_FLAGS) $(QEMU_GUI_ARGS) $(QEMU_STDERR_FILTER)

run-headless:
	$(QEMU_CMD) $(QEMU_RUN_FLAGS) $(QEMU_ARGS) $(QEMU_STDERR_FILTER)

run-gui:
	$(QEMU_CMD) $(QEMU_GUI_FLAGS) $(QEMU_GUI_ARGS) $(QEMU_STDERR_FILTER)

run-gui-hvf:
	$(MAKE) run-gui QEMU_GUI_ACCEL=hvf QEMU_GUI_CPU=host

runasm: 
	$(QEMU_CMD) $(QEMU_RUN_FLAGS) $(QEMU_ARGS) -d in_asm $(QEMU_STDERR_FILTER)

debug:
	$(QEMU_CMD) $(QEMU_RUN_FLAGS) -gdb tcp::26000 -S $(QEMU_ARGS) $(QEMU_STDERR_FILTER)

debugasm:
	$(QEMU_CMD) $(QEMU_RUN_FLAGS) -d in_asm -gdb tcp::26000 -S $(QEMU_ARGS) $(QEMU_STDERR_FILTER)

gdb: 
	echo "target remote :26000" > /tmp/gdbinit
	arm-none-eabi-gdb $(BUILD_DIR)/$(OS_ELF) -x /tmp/gdbinit
	#gdb-multiarch $(BUILD_DIR)/$(OS_ELF) -x /tmp/gdbinit #for ubuntu 18.04 without arm-none-eabi-gdb

dts:
