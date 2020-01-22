cmd_mm/shm.o := arm-none-eabi-gcc -Wp,-MD,mm/.shm.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o mm/shm.o -c mm/shm.c

deps_mm/shm.o := \
  mm/shm.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/mm/shm.h \
  include/types.h \
  include/mm/kalloc.h \
  include/mm/kmalloc.h \
  include/mm/mmu.h \
  include/kstring.h \
  include/types.h \
  include/kernel.h \
  include/hardware.h \
  include/proc.h \
  include/mm/trunkmalloc.h \
  include/ipc.h \
  include/kfile.h \
  include/fsinfo.h \
  include/procinfo.h \
  include/tstr.h \
  include/trunk.h \
  include/system.h \
  include/scheduler.h \
  include/printk.h \

mm/shm.o: $(deps_mm/shm.o)

$(deps_mm/shm.o):
