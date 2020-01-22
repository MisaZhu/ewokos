cmd_drivers/basic_dev.o := arm-none-eabi-gcc -Wp,-MD,drivers/.basic_dev.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o drivers/basic_dev.o -c drivers/basic_dev.c

deps_drivers/basic_dev.o := \
  drivers/basic_dev.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/dev/basic_dev.h \
  include/types.h \
  include/device.h \
  include/sconf.h \
    $(wildcard include/config/h.h) \
  include/sconf_parse.h \
    $(wildcard include/config/parse/h.h) \
  include/tstr.h \
  include/trunk.h \
  include/dev/kevent_dev.h \
  include/kevent.h \
  include/proc.h \
  include/mm/mmu.h \
  include/mm/trunkmalloc.h \
  include/ipc.h \
  include/kfile.h \
  include/fsinfo.h \
  include/procinfo.h \

drivers/basic_dev.o: $(deps_drivers/basic_dev.o)

$(deps_drivers/basic_dev.o):
