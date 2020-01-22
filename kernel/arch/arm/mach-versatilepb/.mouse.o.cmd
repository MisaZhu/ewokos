cmd_arch/arm/mach-versatilepb/mouse.o := arm-none-eabi-gcc -Wp,-MD,arch/arm/mach-versatilepb/.mouse.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o arch/arm/mach-versatilepb/mouse.o -c arch/arm/mach-versatilepb/mouse.c

deps_arch/arm/mach-versatilepb/mouse.o := \
  arch/arm/mach-versatilepb/mouse.c \
    $(wildcard include/config/0x00.h) \
    $(wildcard include/config/t.h) \
    $(wildcard include/config/a.h) \
    $(wildcard include/config/div.h) \
    $(wildcard include/config/.h) \
    $(wildcard include/config/type.h) \
    $(wildcard include/config/rxintren.h) \
    $(wildcard include/config/txintren.h) \
    $(wildcard include/config/en.h) \
    $(wildcard include/config/fd.h) \
    $(wildcard include/config/fc.h) \
    $(wildcard include/config/t/txempty.h) \
    $(wildcard include/config/t/txbusy.h) \
    $(wildcard include/config/t/rxfull.h) \
    $(wildcard include/config/t/rxbusy.h) \
    $(wildcard include/config/t/rxparity.h) \
    $(wildcard include/config/t/ic.h) \
    $(wildcard include/config/t/id.h) \
    $(wildcard include/config//txintr.h) \
    $(wildcard include/config//rxintr.h) \
    $(wildcard include/config/e.h) \
    $(wildcard include/config//size.h) \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/types.h \
  include/mm/mmu.h \
  include/system.h \
  include/scheduler.h \
  include/proc.h \
  include/mm/trunkmalloc.h \
  include/ipc.h \
  include/kfile.h \
  include/fsinfo.h \
  include/procinfo.h \
  include/tstr.h \
  include/trunk.h \
  include/basic_math.h \
  include/dev/basic_dev.h \
  include/device.h \
  include/sconf.h \
    $(wildcard include/config/h.h) \
  include/sconf_parse.h \
    $(wildcard include/config/parse/h.h) \
  include/dev/kevent_dev.h \
  include/kevent.h \

arch/arm/mach-versatilepb/mouse.o: $(deps_arch/arm/mach-versatilepb/mouse.o)

$(deps_arch/arm/mach-versatilepb/mouse.o):
