cmd_kernel/ipc.o := arm-none-eabi-gcc -Wp,-MD,kernel/.ipc.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o kernel/ipc.o -c kernel/ipc.c

deps_kernel/ipc.o := \
  kernel/ipc.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/ipc.h \
  include/types.h \
  include/kstring.h \
  include/types.h \
  include/proc.h \
  include/mm/mmu.h \
  include/mm/trunkmalloc.h \
  include/kfile.h \
  include/fsinfo.h \
  include/procinfo.h \
  include/tstr.h \
  include/trunk.h \
  include/mm/kalloc.h \
  include/system.h \
  include/scheduler.h \
  include/printk.h \
  include/mm/shm.h \

kernel/ipc.o: $(deps_kernel/ipc.o)

$(deps_kernel/ipc.o):
