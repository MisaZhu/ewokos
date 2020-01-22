cmd_mm/kmalloc.o := arm-none-eabi-gcc -Wp,-MD,mm/.kmalloc.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o mm/kmalloc.o -c mm/kmalloc.c

deps_mm/kmalloc.o := \
  mm/kmalloc.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/mm/kmalloc.h \
  include/types.h \
  include/mm/trunkmalloc.h \
  include/mm/mmu.h \
  include/kernel.h \
  include/printk.h \

mm/kmalloc.o: $(deps_mm/kmalloc.o)

$(deps_mm/kmalloc.o):
