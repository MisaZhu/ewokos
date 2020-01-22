cmd_kernel/syscalls.o := arm-none-eabi-gcc -Wp,-MD,kernel/.syscalls.o.d  -nostdinc -nostdlib -D__KERNEL__ -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -mlittle-endian -Wall -DKBUILD_CONF -Iinclude -Iinclude/std -include /home/misa/work/EwokOS/kernel/include/target/config.h  -ffixed-r8 -fno-dwarf2-cfi-asm -mabi=aapcs-linux -mno-thumb-interwork -marm    -msoft-float -Uarm -mcpu=arm926ej-s -Os -fomit-frame-pointer    -o kernel/syscalls.o -c kernel/syscalls.c

deps_kernel/syscalls.o := \
  kernel/syscalls.c \
  /home/misa/work/EwokOS/kernel/include/target/config.h \
    $(wildcard include/config/h/include//.h) \
    $(wildcard include/config/sys/noirq.h) \
    $(wildcard include/config/sys/poll/rt.h) \
    $(wildcard include/config/sys/task.h) \
  include/syscallcode.h \
  include/system.h \
  include/types.h \
  include/scheduler.h \
  include/syscalls.h \
  include/types.h \
  include/proc.h \
  include/mm/mmu.h \
  include/mm/trunkmalloc.h \
  include/ipc.h \
  include/kfile.h \
  include/fsinfo.h \
  include/procinfo.h \
  include/tstr.h \
  include/trunk.h \
  include/kernel.h \
  include/kstring.h \
  include/mm/kmalloc.h \
  include/mm/shm.h \
  include/kserv.h \
  include/hardware.h \
  include/semaphore.h \
  include/dev/basic_dev.h \
  include/device.h \
  include/sconf.h \
    $(wildcard include/config/h.h) \
  include/sconf_parse.h \
    $(wildcard include/config/parse/h.h) \
  include/dev/kevent_dev.h \
  include/kevent.h \

kernel/syscalls.o: $(deps_kernel/syscalls.o)

$(deps_kernel/syscalls.o):
