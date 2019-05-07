#
# arch/arm/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 2010 by Lv Zetalog Zheng

OBJCOPYFLAGS	:=-O binary -R .note -R .note.gnu.build-id -R .comment -S
CFLAGS		+=-ffixed-r8
AFLAGS		+=
LDFLAGS_ewokos	:=--gc-sections -p --no-undefined -X

# Never generate .eh_frame
CFLAGS		+= $(call cc-option,-fno-dwarf2-cfi-asm)

# Select a platform tht is kept up-to-date
KBUILD_DEFCONFIG := versatilepb_defconfig

ifeq ($(CONFIG_FRAME_POINTER),y)
CFLAGS		+=-fno-omit-frame-pointer -mapcs -mno-sched-prolog
endif
ifeq ($(CONFIG_CC_GEN_EXTRA_WARNINGS),y)
CFLAGS		+=-Wextra -Wstrict-prototypes -Wmissing-prototypes
endif

ifeq ($(CONFIG_CPU_BIG_ENDIAN),y)
CPPFLAGS	+= -mbig-endian
AS		+= -EB
LD		+= -EB
else
CPPFLAGS	+= -mlittle-endian
AS		+= -EL
LD		+= -EL
endif

comma = ,

# This selects which instruction set is used.
# Note that GCC does not numerically define an architecture version
# macro, but instead defines a whole series of macros which makes
# testing for a specific architecture or later rather impossible.
arch-$(CONFIG_CPU_32v7M)	:=-D__LINUX_ARM_ARCH__=7 -march=armv7-m -Wa,-march=armv7-m
arch-$(CONFIG_CPU_32v7)		:=-D__LINUX_ARM_ARCH__=7 $(call cc-option,-march=armv7-a,-march=armv5t -Wa$(comma)-march=armv7-a)
arch-$(CONFIG_CPU_32v6KZ)	:=-D__LINUX_ARM_ARCH__=6 $(call cc-option,-march=armv6kz,-march=armv5t -Wa$(comma)-march=armv6kz)
arch-$(CONFIG_CPU_32v6T2)	:=-D__LINUX_ARM_ARCH__=6 -march=armv6t -Wa,-march=armv6t2
arch-$(CONFIG_CPU_32v6K)	:=-D__LINUX_ARM_ARCH__=6 $(call cc-option,-march=armv6k,-march=armv5t -Wa$(comma)-march=armv6k)
arch-$(CONFIG_CPU_32v6)		:=-D__LINUX_ARM_ARCH__=6 $(call cc-option,-march=armv6,-march=armv5t -Wa$(comma)-march=armv6)
arch-$(CONFIG_CPU_32v5TEJ)	:=-D__LINUX_ARM_ARCH__=5 $(call cc-option,-march=armv5te,-march=armv4t)
arch-$(CONFIG_CPU_32v5TE)	:=-D__LINUX_ARM_ARCH__=5 $(call cc-option,-march=armv5te,-march=armv4t)
arch-$(CONFIG_CPU_32v5T)	:=-D__LINUX_ARM_ARCH__=5 $(call cc-option,-march=armv5t,-march=armv4t)
arch-$(CONFIG_CPU_32v4T)	:=-D__LINUX_ARM_ARCH__=4 -march=armv4t
arch-$(CONFIG_CPU_32v4)		:=-D__LINUX_ARM_ARCH__=4 -march=armv4

# This selects how we optimise for the processor.
tune-$(CONFIG_CPU_CORTEXM3)	:=-mcpu=cortex-m3

ifeq ($(CONFIG_ARM_EABI),y)
CFLAGS_ABI	:=-mabi=aapcs-linux -mno-thumb-interwork
else
CFLAGS_ABI	:=$(call cc-option,-mapcs-32,-mabi=apcs-gnu) $(call cc-option,-mno-thumb-interwork,)
endif

ifeq ($(CONFIG_ARM_THUMB2),y)
AFLAGS_AUTOIT	:=$(call as-option,-Wa$(comma)-mimplicit-it=always,-Wa$(comma)-mauto-it)
AFLAGS_NOWARN	:=$(call as-option,-Wa$(comma)-mno-warn-deprecated,-Wa$(comma)-W)
CFLAGS_ISA	:=-mthumb $(AFLAGS_AUTOIT) $(AFLAGS_NOWARN)
AFLAGS_ISA	:=$(CFLAGS_ISA) -Wa$(comma)-mthumb
else
CFLAGS_ISA	:=$(call cc-option,-marm,)
AFLAGS_ISA	:=$(CFLAGS_ISA) $(call cc-option,-marm,)
endif

# Need -Uarm for gcc < 3.x
CFLAGS		+=$(CFLAGS_ABI) $(CFLAGS_ISA) $(arch-y) $(tune-y) $(call cc-option,-mshort-load-bytes,$(call cc-option,-malignment-traps,)) -msoft-float -Uarm
AFLAGS		+=$(CFLAGS_ABI) $(AFLAGS_ISA) $(arch-y) $(tune-y) -include asm/unified.h -msoft-float

CHECKFLAGS	+= -D__arm__

# Default program entry
ifeq ($(CONFIG_ARM_THUMB2),y)
 head-y		:= arch/arm/common/boot.o
else
 head-y		:= arch/arm/common/boot.o
endif

ifeq ($(CONFIG_ARCH_VERSATILEPB),y)
 machine-$(CONFIG_ARCH_VERSATILEPB) := versatilepb
 incdir-$(CONFIG_ARCH_VERSATILEPB)  := versatilepb
 CFLAGS +=-mcpu=arm926ej-s
endif
ifeq ($(CONFIG_ARCH_RASPI2),y)
 machine-$(CONFIG_ARCH_RASPI2) := raspi2
 incdir-$(CONFIG_ARCH_RASPI2)  := raspi2
 CFLAGS +=-mcpu=cortex-a7
endif

ifeq ($(incdir-y),)
incdir-y := $(machine-y)
endif
INCDIR   := mach-$(incdir-y)

ifneq ($(machine-y),)
MACHINE  := arch/arm/mach-$(machine-y)/
include arch/arm/mach-$(machine-y)/config.mk
else
MACHINE  :=
endif

LDFLAGS_ewokos +=-L $(MACHINE)
machdirs := $(patsubst %,arch/arm/mach-%/,$(machine-y))
platdirs := $(patsubst %,arch/arm/plat-%/,$(plat-y))

export	GZFLAGS

# If we have a machine-specific directory, then include it in the build.
core-y				+= arch/arm/common/
drivers-y			+= $(MACHINE)

# Default target when executing plain make
KBUILD_IMAGE := ewokos.bin

all:	$(KBUILD_IMAGE)

boot := arch/arm/boot

#	Update machine arch and proc symlinks if something which affects
#	them changed.  We use .arch to indicate when they were updated
#	last, otherwise make uses the target directory mtime.

include/asm/mach: $(wildcard include/config/arch/*.h) \
		include/config/auto.conf include/asm
	@echo '  SYMLINK include/asm/mach -> include/asm/$(INCDIR)'
	$(Q)$(SYMLINK) $(TOPDIR)/include/asm/$(INCDIR) $@

archprepare: maketools

PHONY += maketools FORCE
maketools: include/asm/mach FORCE

ewokos.bin: ewokos
	$(Q)$(MAKE) $(build)=$(boot) MACHINE=$(MACHINE) $(boot)/$@

CLEAN_FILES += arch/$(ARCH)/include/asm/mach-types.h
CLEAN_FILES += arch/$(ARCH)/boot/ewokos.bin

# We use MRPROPER_FILES and CLEAN_FILES now
archclean:
	$(Q)$(MAKE) $(clean)=$(boot)

run:
	qemu-system-arm $(QEMU_FLAGS) \
	-kernel arch/arm/boot/ewokos.bin \
	-serial mon:stdio -sd ../rootfs/rootfs.ext2
runasm:
	qemu-system-arm $(QEMU_FLAGS) -d in_asm \
	-kernel arch/arm/boot/ewokos.bin \
	-serial mon:stdio -sd ../rootfs/rootfs.ext2
debug:
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S \
	-kernel arch/arm/boot/ewokos.bin \
	-serial mon:stdio -sd ../rootfs/rootfs.ext2
debugasm:
	qemu-system-arm $(QEMU_FLAGS) -gdb tcp::26000 -S -d in_asm \
	-kernel arch/arm/boot/ewokos.bin \
	-serial mon:stdio -sd ../rootfs/rootfs.ext2
gdb:
	echo "target remote :26000" > /tmp/gdbinit
	arm-none-eabi-gdb ewokos -x /tmp/gdbinit

define archhelp
  echo  '  ewokos.bin     - Binary image (arch/$(ARCH)/boot/ewokos.bin)'
  echo  '  run/runasm     - Run (arch/$(ARCH)/boot/ewokos.bin)'
  echo  '  debug/debugasm - Run (arch/$(ARCH)/boot/ewokos.bin) in debug mode'
  echo  '  gdb            - Attach debug mode EwokOS'
endef
