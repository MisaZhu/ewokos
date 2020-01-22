cmd_init/kernel.o := arm-none-eabi-gcc -Wp,-MD,init/.kernel.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o init/kernel.o -c init/kernel.c

deps_init/kernel.o := \
  init/kernel.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/printk.h \
  include/mm/mmu.h \
  include/types.h \
  include/mm/kalloc.h \
  include/mm/kmalloc.h \
  include/mm/shm.h \
  include/hardware.h \
  include/kernel.h \
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
  include/irq.h \
  include/kstring.h \
  include/types.h \
  include/timer.h \
  include/dev/basic_dev.h \
  include/device.h \
  include/sconf.h \
    $(wildcard include/config/h.h) \
  include/sconf_parse.h \
    $(wildcard include/config/parse/h.h) \
  include/dev/kevent_dev.h \
  include/kevent.h \
  include/sdread.h \

init/kernel.o: $(deps_init/kernel.o)

$(deps_init/kernel.o):
