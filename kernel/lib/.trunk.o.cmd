cmd_lib/trunk.o := arm-none-eabi-gcc -Wp,-MD,lib/.trunk.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o lib/trunk.o -c lib/trunk.c

deps_lib/trunk.o := \
  lib/trunk.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/trunk.h \
  include/types.h \
  include/kstring.h \
  include/types.h \

lib/trunk.o: $(deps_lib/trunk.o)

$(deps_lib/trunk.o):
