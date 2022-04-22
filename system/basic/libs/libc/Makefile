include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I $(TARGET_DIR)/include -Iinclude -Isys/include

LIB_EWOKSYS_OBJS = sys/src/syscall.o \
	sys/src/mstr.o \
	sys/src/klog.o \
	sys/src/cmain.o \
	sys/src/charbuf.o \
	sys/src/vfs.o \
	sys/src/ipc.o \
	sys/src/signal.o \
	sys/src/interrupt.o \
	sys/src/vdevice.o \
	sys/src/proto.o \
	sys/src/thread.o \
	sys/src/basic_math.o \
	sys/src/basic_math_asm.o \
	sys/src/shm.o \
	sys/src/core.o \
	sys/src/proc.o \
	sys/src/wait.o \
	sys/src/md5.o \
	sys/src/buffer.o \
	sys/src/mmio.o 

LIB_EWOKC_OBJS = src/unistd/chdir.o \
	src/unistd/dup.o \
	src/unistd/dup2.o \
	src/unistd/errno.o \
	src/unistd/exec.o \
	src/unistd/fork.o \
	src/unistd/vfork.o \
	src/unistd/getcwd.o \
	src/unistd/getuid.o \
	src/unistd/getpid.o \
	src/unistd/lseek.o \
	src/unistd/pipe.o \
	src/unistd/read.o \
	src/unistd/setuid.o \
	src/unistd/sleep.o \
	src/unistd/usleep.o \
	src/unistd/unlink.o \
	src/unistd/write.o \
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
	src/stdlib/free.o \
	src/stdlib/malloc.o \
	src/stdlib/realloc.o \
	src/stdlib/random.o \
	src/stdlib/setenv.o \
	src/stdlib/getenv.o \
	\
	src/pthread/pthread_create.o \
	src/pthread/pthread_self.o \
	\
	src/stdio/vprintf.o \
	src/stdio/snprintf.o \
	src/stdio/dprintf.o \
	src/stdio/printf.o \
	src/stdio/fprintf.o \
	src/stdio/fopen.o \
	src/stdio/fclose.o \
	src/stdio/fread.o \
	src/stdio/fwrite.o \
	src/stdio/ftell.o \
	src/stdio/fseek.o \
	src/stdio/rewind.o \
	src/stdio/getch.o \
	src/stdio/putch.o \
	src/stdio/puts.o \
	\
	src/fcntl/open.o \
	src/fcntl/close.o \
	src/fcntl/stat.o \
	src/fcntl/fcntl.o \
	\
	src/signal/signal.o \
	src/signal/kill.o

EWOKC = $(TARGET_DIR)/lib/libewokc.a

$(EWOKC): $(LIB_EWOKSYS_OBJS) $(LIB_EWOKC_OBJS)
	$(AR) rT $(EWOKC) $(LIB_EWOKSYS_OBJS) $(LIB_EWOKC_OBJS)
	cp -r sys/include/* include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_EWOKSYS_OBJS) $(LIB_EWOKC_OBJS)
	rm -f $(EWOKC)

