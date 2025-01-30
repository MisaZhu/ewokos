SYS_DIR=../../../
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iewoksys/include -Iinclude -I $(TARGET_DIR)/include

LIB_EWOKSYS_OBJS =  \
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
	ewoksys/src/timer.o \
	ewoksys/src/sys.o \
	\
	src/unistd/getuid.o \
	src/unistd/usleep.o \
	src/unistd/sleep.o \
	src/unistd/getcwd.o \
	src/unistd/getgid.o \
	src/unistd/setgid.o \
	src/unistd/setuid.o \
	src/unistd/chdir.o \
	src/unistd/dup2.o \
	src/unistd/dup.o \
	src/unistd/pipe.o \
	src/unistd/chown.o \
	\
	src/sys/shm/shmget.o \
	src/sys/shm/shmat.o \
	src/sys/shm/shmdt.o \
	\
	src/signal/kill.o \
	src/signal/signal.o \
	\
	src/sys/stat/mkdir.o \
	src/sys/stat/mkfifo.o \
	\
	src/fcntl/fchmod.o \
	src/fcntl/chmod.o \
	src/fcntl/fcntl.o \
	\
	src/stdio/putch.o \
	\
	src/dirent/readdir.o \
	src/dirent/closedir.o \
	src/dirent/opendir.o \
	\
	src/pthread/pthread_create.o \
	src/pthread/pthread_mutex_destroy.o \
	src/pthread/pthread_mutex_init.o \
	src/pthread/pthread_mutex_lock.o \
	src/pthread/pthread_mutex_unlock.o \
	src/pthread/pthread_self.o\
	\
	src/stdlib/exit.o
	
EWOKSYS = $(TARGET_DIR)/lib/libewoksys.a

$(EWOKSYS): $(LIB_EWOKSYS_OBJS)   $(LIB_EWOKSYS_SYS_OBJS)
	$(AR) rcs $(EWOKSYS) $(LIB_EWOKSYS_OBJS) $(TARGET_DIR)/lib/libsoftfloat.a
	cp -r ewoksys/include/* include/* $(TARGET_DIR)/include 

clean:
	rm -f $(LIB_EWOKSYS_OBJS) $(LIB_EWOKSYS_OBJS)
	rm -f $(EWOKSYS)

