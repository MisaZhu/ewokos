cmd_arch/arm/mach-versatilepb/sdc.o := arm-none-eabi-gcc -Wp,-MD,arch/arm/mach-versatilepb/.sdc.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o arch/arm/mach-versatilepb/sdc.o -c arch/arm/mach-versatilepb/sdc.c

deps_arch/arm/mach-versatilepb/sdc.o := \
  arch/arm/mach-versatilepb/sdc.c \
    $(wildcard include/config/arm/pl180/mmci/clock/freq.h) \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/system.h \
  include/types.h \
  include/scheduler.h \
  include/mm/mmu.h \
  include/mm/kmalloc.h \
  include/kstring.h \
  include/types.h \
  include/device.h \
  include/dev/basic_dev.h \
  include/sconf.h \
    $(wildcard include/config/h.h) \
  include/sconf_parse.h \
    $(wildcard include/config/parse/h.h) \
  include/tstr.h \
  include/trunk.h \
  include/dev/kevent_dev.h \
  include/kevent.h \

arch/arm/mach-versatilepb/sdc.o: $(deps_arch/arm/mach-versatilepb/sdc.o)

$(deps_arch/arm/mach-versatilepb/sdc.o):
