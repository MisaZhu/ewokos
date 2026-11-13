include config.mk

# Use bash so process substitution works for stderr filtering
SHELL := /bin/bash

LDS = mkos.lds.S

# Detect the host OS so QEMU is given host-appropriate parameters.  macOS
# (Darwin) uses the Cocoa display, Hypervisor.framework (hvf) acceleration and
# the coreaudio backend; Linux and other hosts use GTK, KVM and PulseAudio.
# HOST_OS is "macos" on Darwin, otherwise the lowercased `uname -s` (e.g.
# "linux").  Every derived default below stays `?=`-overridable on the command
# line, so e.g. cross-arch emulation on Linux can still force tcg:
#   make run QEMU_RUN_ACCEL=tcg QEMU_RUN_CPU=cortex-a72
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
HOST_OS := macos
else
HOST_OS := $(shell echo '$(UNAME_S)' | tr 'A-Z' 'a-z')
endif

# Host-appropriate defaults for the GUI display backend, the audio backend and
# (in the aarch64 block below) the hardware accelerator + CPU model.
ifeq ($(HOST_OS),macos)
QEMU_GUI_DISPLAY ?= cocoa,zoom-to-fit=on,zoom-interpolation=on
QEMU_AUDIODEV ?= coreaudio
QEMU_HOST_ACCEL ?= hvf
QEMU_HOST_CPU ?= host
QEMU_MEM ?= 8192
else
QEMU_GUI_DISPLAY ?= gtk,zoom-to-fit=on
QEMU_AUDIODEV ?= pa
# KVM is not always usable on a Linux/other host: it needs the host CPU arch to
# match the aarch64 guest, an accessible /dev/kvm (user in the kvm group) and,
# inside a VM, nested virtualisation.  When any of those is missing QEMU aborts
# with "could not access kvm kernel module".  List tcg as a fallback so QEMU
# tries kvm first and silently drops to TCG otherwise.  -cpu max is valid under
# both accelerators (unlike -cpu host, which is KVM-only), so the same command
# line works whether or not kvm is available.
QEMU_HOST_ACCEL ?= kvm:tcg
QEMU_HOST_CPU ?= max
# 8 GB of guest RAM is fine under macOS hvf, but a TCG fallback on a Linux host
# must reserve the whole guest RAM in host memory up front and often fails with
# "cannot set up guest memory 'mach-virt.ram': Cannot allocate memory".  Default
# to a smaller 2 GB there; raise it (e.g. once KVM works, or on a big host) with
#   make run QEMU_MEM=8192
QEMU_MEM ?= 2048
endif

# Filter out harmless macOS Input Method Kit (IMK) console spam from QEMU's
# Cocoa display backend, e.g. "error messaging the mach port for
# IMKCFRunLoopWakeUpReliable". The serial console uses stdout, so filtering
# stderr keeps real QEMU errors visible while dropping this noise.  Only
# relevant on macOS; left empty elsewhere so no grep subprocess is spawned.
ifeq ($(HOST_OS),macos)
QEMU_STDERR_FILTER = 2> >(grep -v 'IMKCFRunLoopWakeUpReliable' >&2)
else
QEMU_STDERR_FILTER =
endif

ifeq ($(ARCH), aarch64)
	OS_IMG = kernel8.img
	OS_ELF = kernel8.elf
	QEMU_CMD = qemu-system-aarch64
	ARCH_CFLAGS = -Wa,-march=armv8-a -DMAX_USABLE_MEM_SIZE='(8ull*GB)'
	ifeq ($(PAGE_SIZE),64k)
		QEMU_RUN_ACCEL ?= tcg
		QEMU_RUN_CPU ?= cortex-a72
		QEMU_GUI_ACCEL ?= tcg
		QEMU_GUI_CPU ?= cortex-a72
	else
		QEMU_RUN_ACCEL ?= $(QEMU_HOST_ACCEL)
		QEMU_RUN_CPU ?= $(QEMU_HOST_CPU)
		QEMU_GUI_ACCEL ?= $(QEMU_HOST_ACCEL)
		QEMU_GUI_CPU ?= $(QEMU_HOST_CPU)
	endif
	QEMU_RUN_FLAGS = -M $(QEMU_MACHINE),highmem=on,accel=$(QEMU_RUN_ACCEL) -cpu $(QEMU_RUN_CPU) -m $(QEMU_MEM) \
		-serial mon:stdio -nographic
	QEMU_GUI_FLAGS = -M $(QEMU_MACHINE),highmem=on,accel=$(QEMU_GUI_ACCEL) -cpu $(QEMU_GUI_CPU) -m $(QEMU_MEM) \
		-serial mon:stdio -device ramfb \
		-display $(QEMU_GUI_DISPLAY)
else
	OS_IMG = kernel7.img
	OS_ELF = kernel7.elf
	QEMU_CMD = qemu-system-arm
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
FINAL_SYS_DIR = ../system

ifeq ($(QEMU_DISPLAY_OPTS),cocoa)
QEMU_GUI_DISPLAY = cocoa,zoom-to-fit=on,zoom-interpolation=on
endif

QEMU_AUDIO_ENABLE ?= yes
# Persist rootfs writes by default so ext2 changes made in the guest survive
# reboot. Use `make ... QEMU_SNAPSHOT=yes` when a disposable session is needed.
QEMU_SNAPSHOT ?= no
QEMU_AUDIO_ID ?= audio0
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
QEMU_ARGS += -drive file=$(FINAL_SYS_DIR)/root_$(ARCH).img,format=raw,id=blk0,if=none -device virtio-blk-device,drive=blk0
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
