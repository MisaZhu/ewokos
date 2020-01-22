cmd_kernel/printk.o := arm-none-eabi-gcc -Wp,-MD,kernel/.printk.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o kernel/printk.o -c kernel/printk.c

deps_kernel/printk.o := \
  kernel/printk.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/printk.h \
  include/vprintf.h \
  include/std/stdarg.h \
  include/types.h \
  include/dev/basic_dev.h \
  include/device.h \
  include/sconf.h \
    $(wildcard include/config/h.h) \
  include/sconf_parse.h \
    $(wildcard include/config/parse/h.h) \
  include/tstr.h \
  include/trunk.h \
  include/dev/kevent_dev.h \
  include/kevent.h \

kernel/printk.o: $(deps_kernel/printk.o)

$(deps_kernel/printk.o):
