# tools
AR = arm-none-eabi-ar
CC = arm-none-eabi-gcc
CXX = arm-none-eabi-c++
LD = arm-none-eabi-ld
OBJDUMP = arm-none-eabi-objdump

export QEMU_AUDIO_DRV=none

BUILD_DIR = ../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = .

# flags
CFLAGS = -Wstrict-prototypes \
				 -marm \
				 -pedantic -Wall -Wextra -msoft-float -fPIC -mapcs-frame \
         -fno-builtin-printf \
         -fno-builtin-stat \
         -fno-builtin-memset \
         -fno-builtin-calloc \
         -fno-builtin-snprintf \
         -fno-builtin-sprintf \
         -fno-builtin-fprintf \
				 -Wno-overlength-strings \
         -fno-builtin-exit -fno-builtin-stdio \
				 -std=c99 

#CFLAGS +=  -g
CFLAGS +=  -O2

KERNEL_DIR=../../kernel/basic/kernel

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

LIBS_DIR = libs

LIB_INC = -I$(BUILD_DIR)/include \
					-I $(LIBS_DIR)/sys/include \
					-I $(LIBS_DIR)/arch/arm/bcm283x/include \
					-I $(LIBS_DIR)/arch/arm/vpb/include \
				  -I $(LIBS_DIR)/hash/include \
				  -I $(LIBS_DIR)/libc/include
				 
LIB_SYS_DIR = $(LIBS_DIR)/sys
LIB_ARCH_BCM283X_DIR = $(LIBS_DIR)/arch/arm/bcm283x
LIB_ARCH_VPB_DIR = $(LIBS_DIR)/arch/arm/vpb
LIB_HASH_DIR = $(LIBS_DIR)/hash
LIB_LIBC_DIR = $(LIBS_DIR)/libc

CFLAGS += $(LIB_INC)

LIB_HASH_OBJS = $(LIB_HASH_DIR)/src/hashmap.o

LIB_SYS_OBJS = $(LIB_SYS_DIR)/src/syscall.o \
	$(LIB_SYS_DIR)/src/mstr.o \
	$(LIB_SYS_DIR)/src/klog.o \
	$(LIB_SYS_DIR)/src/cmain.o \
	$(LIB_SYS_DIR)/src/charbuf.o \
	$(LIB_SYS_DIR)/src/vfs.o \
	$(LIB_SYS_DIR)/src/ipc.o \
	$(LIB_SYS_DIR)/src/vdevice.o \
	$(LIB_SYS_DIR)/src/proto.o \
	$(LIB_SYS_DIR)/src/thread.o \
	$(LIB_SYS_DIR)/src/basic_math.o \
	$(LIB_SYS_DIR)/src/shm.o \
	$(LIB_SYS_DIR)/src/global.o \
	$(LIB_SYS_DIR)/src/proc.o \
	$(LIB_SYS_DIR)/src/lockc.o \
	$(LIB_SYS_DIR)/src/wait.o \
	$(LIB_SYS_DIR)/src/md5.o \
	$(LIB_SYS_DIR)/src/buffer.o \
	$(LIB_SYS_DIR)/src/mmio.o 

LIB_ARCH_BCM283X_OBJS = $(LIB_ARCH_BCM283X_DIR)/src/gpio.o \
	$(LIB_ARCH_BCM283X_DIR)/src/spi.o \
	$(LIB_ARCH_BCM283X_DIR)/src/pl011_uart.o \
	$(LIB_ARCH_BCM283X_DIR)/src/mini_uart.o \
	$(LIB_ARCH_BCM283X_DIR)/src/mailbox.o \
	$(LIB_ARCH_BCM283X_DIR)/src/mbox_actled.o \
	$(LIB_ARCH_BCM283X_DIR)/src/gpio_actled.o \
	$(LIB_ARCH_BCM283X_DIR)/src/framebuffer.o

LIB_ARCH_VPB_OBJS = $(LIB_ARCH_VPB_DIR)/src/framebuffer.o

LIB_LIBC_OBJS = $(LIB_LIBC_DIR)/src/unistd/chdir.o \
	$(LIB_LIBC_DIR)/src/unistd/dup.o \
	$(LIB_LIBC_DIR)/src/unistd/dup2.o \
	$(LIB_LIBC_DIR)/src/unistd/errno.o \
	$(LIB_LIBC_DIR)/src/unistd/exec.o \
	$(LIB_LIBC_DIR)/src/unistd/fork.o \
	$(LIB_LIBC_DIR)/src/unistd/vfork.o \
	$(LIB_LIBC_DIR)/src/unistd/getcwd.o \
	$(LIB_LIBC_DIR)/src/unistd/getuid.o \
	$(LIB_LIBC_DIR)/src/unistd/getpid.o \
	$(LIB_LIBC_DIR)/src/unistd/lseek.o \
	$(LIB_LIBC_DIR)/src/unistd/pipe.o \
	$(LIB_LIBC_DIR)/src/unistd/read.o \
	$(LIB_LIBC_DIR)/src/unistd/setuid.o \
	$(LIB_LIBC_DIR)/src/unistd/sleep.o \
	$(LIB_LIBC_DIR)/src/unistd/usleep.o \
	$(LIB_LIBC_DIR)/src/unistd/unlink.o \
	$(LIB_LIBC_DIR)/src/unistd/write.o \
	\
	$(LIB_LIBC_DIR)/src/dirent/opendir.o \
	$(LIB_LIBC_DIR)/src/dirent/closedir.o \
	$(LIB_LIBC_DIR)/src/dirent/readdir.o \
	$(LIB_LIBC_DIR)/src/dirent/rewinddir.o \
	$(LIB_LIBC_DIR)/src/dirent/telldir.o \
	$(LIB_LIBC_DIR)/src/dirent/seekdir.o \
	\
	$(LIB_LIBC_DIR)/src/stdlib/atof.o \
	$(LIB_LIBC_DIR)/src/stdlib/atoi.o \
	$(LIB_LIBC_DIR)/src/stdlib/atoll.o \
	$(LIB_LIBC_DIR)/src/stdlib/calloc.o \
	$(LIB_LIBC_DIR)/src/stdlib/execl.o \
	$(LIB_LIBC_DIR)/src/stdlib/exit.o \
	$(LIB_LIBC_DIR)/src/stdlib/free.o \
	$(LIB_LIBC_DIR)/src/stdlib/malloc.o \
	$(LIB_LIBC_DIR)/src/stdlib/realloc.o \
	$(LIB_LIBC_DIR)/src/stdlib/random.o \
	$(LIB_LIBC_DIR)/src/stdlib/setenv.o \
	$(LIB_LIBC_DIR)/src/stdlib/getenv.o \
	\
	$(LIB_LIBC_DIR)/src/pthread/pthread_create.o \
	\
	$(LIB_LIBC_DIR)/src/stdio/vprintf.o \
	$(LIB_LIBC_DIR)/src/stdio/snprintf.o \
	$(LIB_LIBC_DIR)/src/stdio/dprintf.o \
	$(LIB_LIBC_DIR)/src/stdio/printf.o \
	$(LIB_LIBC_DIR)/src/stdio/fprintf.o \
	$(LIB_LIBC_DIR)/src/stdio/fopen.o \
	$(LIB_LIBC_DIR)/src/stdio/fclose.o \
	$(LIB_LIBC_DIR)/src/stdio/fread.o \
	$(LIB_LIBC_DIR)/src/stdio/fwrite.o \
	$(LIB_LIBC_DIR)/src/stdio/ftell.o \
	$(LIB_LIBC_DIR)/src/stdio/fseek.o \
	$(LIB_LIBC_DIR)/src/stdio/rewind.o \
	$(LIB_LIBC_DIR)/src/stdio/getch.o \
	$(LIB_LIBC_DIR)/src/stdio/putch.o \
	$(LIB_LIBC_DIR)/src/stdio/puts.o \
	\
	$(LIB_LIBC_DIR)/src/fcntl/open.o \
	$(LIB_LIBC_DIR)/src/fcntl/close.o \
	$(LIB_LIBC_DIR)/src/fcntl/stat.o \
	$(LIB_LIBC_DIR)/src/fcntl/fcntl.o

LIB_OBJS = $(LIB_SYS_OBJS) \
	$(LIB_ARCH_BCM283X_OBJS) \
	$(LIB_ARCH_VPB_OBJS) \
	$(LIB_SD_OBJS) \
	$(LIB_LIBC_OBJS) \
	$(LIB_GRAPH_OBJS) \
	$(LIB_CONSOLE_OBJS) \
	$(LIB_SCONF_OBJS) \
	$(LIB_HASH_OBJS) \
	$(LIB_TINYJSON_OBJS) \
	$(LIB_EXT2_OBJS)

CLEAN = $(LIB_OBJS)

all: mkdirs libs builtin_apps
	@echo "all done."

KERNEL_H = \
	$(KERNEL_DIR)/include/usinterrupt.h \
	$(KERNEL_DIR)/include/syscalls.h \
	$(KERNEL_DIR)/include/sysinfo.h \
	$(KERNEL_DIR)/include/kevent.h \
	$(KERNEL_DIR)/include/procinfo.h

libs: $(LIB_OBJS)
	$(AR) rT $(BUILD_DIR)/lib/libewokc.a $(LIB_SYS_OBJS)  $(LIB_LIBC_OBJS)
	@cp -r $(LIB_SYS_DIR)/include/* $(LIB_LIBC_DIR)/include/* $(BUILD_DIR)/include/
	$(AR) rT $(BUILD_DIR)/lib/libarch_bcm283x.a $(LIB_ARCH_BCM283X_OBJS)
	@cp -r $(LIB_ARCH_BCM283X_DIR)/include/* $(BUILD_DIR)/include/
	$(AR) rT $(BUILD_DIR)/lib/libarch_vpb.a $(LIB_ARCH_VPB_OBJS)
	@cp -r $(LIB_ARCH_VPB_DIR)/include/* $(BUILD_DIR)/include/
	$(AR) rT $(BUILD_DIR)/lib/libhash.a $(LIB_HASH_OBJS)
	@cp -r $(LIB_HASH_DIR)/include/* $(BUILD_DIR)/include/

include $(ROOT_DIR)/sbin/init/build.mk

include $(ROOT_DIR)/drivers/nulld/build.mk
include $(ROOT_DIR)/drivers/stdiod/build.mk
include $(ROOT_DIR)/drivers/romfsd/build.mk
include $(ROOT_DIR)/drivers/ramfsd/build.mk
include $(ROOT_DIR)/drivers/arch/arm/versatilepb/ttyd/build.mk
include $(ROOT_DIR)/drivers/arch/arm/raspix/pl011_uartd/build.mk
include $(ROOT_DIR)/drivers/arch/arm/raspix/mini_uartd/build.mk

include $(ROOT_DIR)/bin/shell/build.mk
include $(ROOT_DIR)/bin/cat/build.mk
include $(ROOT_DIR)/bin/ps/build.mk
include $(ROOT_DIR)/bin/ls/build.mk
include $(ROOT_DIR)/bin/login/build.mk
include $(ROOT_DIR)/bin/session/build.mk
include $(ROOT_DIR)/bin/mount/build.mk
include $(ROOT_DIR)/bin/uname/build.mk
include $(ROOT_DIR)/bin/pwd/build.mk

mkdirs:
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/drivers/arm/versatilepb
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/drivers/arm/raspix
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/bin
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/dev
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/home/root
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/home/guest
	@mkdir -p ${BUILD_DIR}/asm
	@mkdir -p ${BUILD_DIR}/include
	@mkdir -p ${BUILD_DIR}/lib
	@cp -r $(KERNEL_H) $(BUILD_DIR)/include/
	@cp -r ${ROOT_DIR}/etc ${TARGET_DIR}/$(ROOT_DIR)

builtin_apps: ${PROGS}

clean:
	rm -f $(CLEAN)
	rm -fr $(BUILD_DIR)
