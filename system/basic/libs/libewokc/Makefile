include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iewoksys/include -Iinclude -I $(TARGET_DIR)/include

LIB_EWOKSYS_OBJS = ewoksys/src/syscall.o \
	ewoksys/src/syscall_${ARCH}.o \
	ewoksys/src/mstr.o \
	ewoksys/src/utf8unicode.o \
	ewoksys/src/klog.o \
	ewoksys/src/core.o \
	ewoksys/src/cmain.o \
	ewoksys/src/charbuf.o \
	ewoksys/src/vfs.o \
	ewoksys/src/ipc.o \
	ewoksys/src/signal.o \
	ewoksys/src/interrupt.o \
	ewoksys/src/vdevice.o \
	ewoksys/src/devcmd.o \
	ewoksys/src/proto.o \
	ewoksys/src/semaphore.o \
	ewoksys/src/thread.o \
	ewoksys/src/basic_math.o \
	ewoksys/src/kernel_tic.o \
	ewoksys/src/trunkmem.o \
	ewoksys/src/proc.o \
	ewoksys/src/wait.o \
	ewoksys/src/md5.o \
	ewoksys/src/buffer.o \
	ewoksys/src/mmio.o  \
	ewoksys/src/dma.o  \
	ewoksys/src/session.o  \
	ewoksys/src/hashmap.o  \
	ewoksys/src/timer.o 

LIB_EWOKC_OBJS = \
#	src/string/ctype_.o  \
	src/string/isctype.o  \
	src/string/memchr.o  \
	src/string/memcmp.o  \
	src/string/memcpy.o  \
	src/string/memmove.o  \
	src/string/memset.o  \
	src/string/strcasecmp.o  \
	src/string/strcat.o  \
	src/string/strchr.o  \
	src/string/strcmp.o  \
	src/string/strcpy.o  \
	src/string/strdup.o  \
	src/string/strlen.o  \
	src/string/strncmp.o  \
	src/string/strncpy.o  \
	src/string/strstr.o  \
	src/string/strtol.o  \


LIB_EWOKC_OBJS += \
	src/unistd/chdir.o \
	src/unistd/dup.o \
	src/unistd/dup2.o \
	src/unistd/errno.o \
	src/unistd/fork.o \
	src/unistd/vfork.o \
	src/unistd/getcwd.o \
	src/unistd/getuid.o \
	src/unistd/getgid.o \
	src/unistd/getpid.o \
	src/unistd/lseek.o \
	src/unistd/pipe.o \
	src/unistd/read.o \
	src/unistd/setuid.o \
	src/unistd/setgid.o \
	src/unistd/sleep.o \
	src/unistd/usleep.o \
	src/unistd/unlink.o \
	src/unistd/write.o \
	src/unistd/chown.o \
	src/unistd/access.o \
	\
	src/dirent/opendir.o \
	src/dirent/closedir.o \
	src/dirent/readdir.o \
	src/dirent/rewinddir.o \
	src/dirent/telldir.o \
	src/dirent/seekdir.o \
	\
	src/stdlib/atof.o \
	src/stdlib/atoi.o \
	src/stdlib/atoll.o \
	src/stdlib/calloc.o \
	src/stdlib/execl.o \
	src/stdlib/exit.o \
	src/stdlib/execve.o \
	src/stdlib/malloc.o \
	src/stdlib/free.o \
	src/stdlib/realloc.o \
	src/stdlib/random.o \
	src/stdlib/setenv.o \
	src/stdlib/getenv.o \
	src/stdlib/strtoul.o \
	src/stdlib/rand.o \
	\
	src/pthread/pthread_create.o \
	src/pthread/pthread_self.o \
	src/pthread/pthread_mutex_init.o \
	src/pthread/pthread_mutex_lock.o \
	src/pthread/pthread_mutex_unlock.o \
	src/pthread/pthread_mutex_destroy.o \
	\
	src/fcntl/open.o \
	src/fcntl/close.o \
	src/fcntl/fcntl.o \
	src/fcntl/chmod.o \
	src/fcntl/fchmod.o \
	\
	src/signal/signal.o \
	src/signal/kill.o

LIB_EWOKC_SYS_OBJS = \
	src/sys/ipc/ftok.o \
	\
	src/sys/shm/shmget.o \
	src/sys/shm/shmdt.o \
	src/sys/shm/shmat.o \
	\
	src/sys/stat/stat.o \
	src/sys/stat/fstat.o \
	src/sys/stat/mkdir.o \
	\
	src/stdio/vprintf.o \
	src/stdio/snprintf.o \
	src/stdio/dprintf.o \
	src/stdio/printf.o \
	src/stdio/fopen.o \
	src/stdio/fgetc.o \
	src/stdio/fclose.o \
	src/stdio/fread.o \
	src/stdio/fwrite.o \
	src/stdio/ftell.o \
	src/stdio/fseek.o \
	src/stdio/rewind.o \
	src/stdio/getch.o \
	src/stdio/putch.o \
	src/stdio/puts.o \
	src/stdio/ferror.o \
	src/stdio/fgets.o \
	src/stdio/fputc.o \
	src/stdio/fputs.o \
	src/stdio/putc.o \
	src/stdio/qsort.o \
	src/stdio/sprintf.o \
	src/stdio/setbuf.o
#	src/stdio/fflush.o \
	src/stdio/fprintf.o \
	src/stdio/ungetc.o

EWOKC = $(TARGET_DIR)/lib/libewokc.a

$(EWOKC): $(LIB_EWOKSYS_OBJS) $(LIB_EWOKC_OBJS)  $(LIB_EWOKC_SYS_OBJS) \
			$(TARGET_DIR)/lib/libsoftfloat.a
	$(AR) rT $(EWOKC) $(LIB_EWOKSYS_OBJS) $(LIB_EWOKC_OBJS) $(LIB_EWOKC_SYS_OBJS) $(TARGET_DIR)/lib/libsoftfloat.a
	cp -r ewoksys/include/* include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_EWOKSYS_OBJS) $(LIB_EWOKC_OBJS)
	rm -f $(EWOKC)

