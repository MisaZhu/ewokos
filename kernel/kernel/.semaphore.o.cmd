cmd_kernel/semaphore.o := arm-none-eabi-gcc -Wp,-MD,kernel/.semaphore.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o kernel/semaphore.o -c kernel/semaphore.c

deps_kernel/semaphore.o := \
  kernel/semaphore.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/semaphore.h \
  include/types.h \
  include/system.h \
  include/scheduler.h \
  include/proc.h \
  include/mm/mmu.h \
  include/mm/trunkmalloc.h \
  include/ipc.h \
  include/kfile.h \
  include/fsinfo.h \
  include/procinfo.h \
  include/tstr.h \
  include/trunk.h \

kernel/semaphore.o: $(deps_kernel/semaphore.o)

$(deps_kernel/semaphore.o):
