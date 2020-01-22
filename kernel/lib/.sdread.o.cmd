cmd_lib/sdread.o := arm-none-eabi-gcc -Wp,-MD,lib/.sdread.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o lib/sdread.o -c lib/sdread.c

deps_lib/sdread.o := \
  lib/sdread.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/kstring.h \
  include/types.h \
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
  include/mm/kmalloc.h \
  include/sdread.h \

lib/sdread.o: $(deps_lib/sdread.o)

$(deps_lib/sdread.o):
