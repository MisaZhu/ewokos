cmd_lib/tstr.o := arm-none-eabi-gcc -Wp,-MD,lib/.tstr.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o lib/tstr.o -c lib/tstr.c

deps_lib/tstr.o := \
  lib/tstr.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/tstr.h \
  include/trunk.h \
  include/types.h \
  include/kstring.h \
  include/types.h \

lib/tstr.o: $(deps_lib/tstr.o)

$(deps_lib/tstr.o):
