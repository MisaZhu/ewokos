cmd_mm/kalloc.o := arm-none-eabi-gcc -Wp,-MD,mm/.kalloc.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o mm/kalloc.o -c mm/kalloc.c

deps_mm/kalloc.o := \
  mm/kalloc.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/mm/kalloc.h \
  include/types.h \
  include/mm/mmu.h \
  include/system.h \
  include/scheduler.h \

mm/kalloc.o: $(deps_mm/kalloc.o)

$(deps_mm/kalloc.o):
