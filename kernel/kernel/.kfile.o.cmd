cmd_kernel/kfile.o := arm-none-eabi-gcc -Wp,-MD,kernel/.kfile.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o kernel/kfile.o -c kernel/kfile.c

deps_kernel/kfile.o := \
  kernel/kfile.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/kfile.h \
  include/types.h \
  include/fsinfo.h \
  include/kstring.h \
  include/types.h \
  include/system.h \
  include/scheduler.h \
  include/proc.h \
  include/mm/mmu.h \
  include/mm/trunkmalloc.h \
  include/ipc.h \
  include/procinfo.h \
  include/tstr.h \
  include/trunk.h \
  include/mm/kmalloc.h \

kernel/kfile.o: $(deps_kernel/kfile.o)

$(deps_kernel/kfile.o):
