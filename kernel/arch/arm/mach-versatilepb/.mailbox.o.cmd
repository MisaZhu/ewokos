cmd_arch/arm/mach-versatilepb/mailbox.o := arm-none-eabi-gcc -Wp,-MD,arch/arm/mach-versatilepb/.mailbox.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o arch/arm/mach-versatilepb/mailbox.o -c arch/arm/mach-versatilepb/mailbox.c

deps_arch/arm/mach-versatilepb/mailbox.o := \
  arch/arm/mach-versatilepb/mailbox.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/hardware.h \
  include/types.h \
  include/mm/mmu.h \

arch/arm/mach-versatilepb/mailbox.o: $(deps_arch/arm/mach-versatilepb/mailbox.o)

$(deps_arch/arm/mach-versatilepb/mailbox.o):
