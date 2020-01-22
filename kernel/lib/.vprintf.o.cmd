cmd_lib/vprintf.o := arm-none-eabi-gcc -Wp,-MD,lib/.vprintf.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o lib/vprintf.o -c lib/vprintf.c

deps_lib/vprintf.o := \
  lib/vprintf.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/vprintf.h \
  include/std/stdarg.h \
  include/types.h \
  include/kstring.h \
  include/types.h \

lib/vprintf.o: $(deps_lib/vprintf.o)

$(deps_lib/vprintf.o):
