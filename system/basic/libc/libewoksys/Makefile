SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iewoksys/include -Iinclude -I $(TARGET_DIR)/include

LIB_EWOKSYS_OBJS =  \
	$(ARCH)/ewoksys/src/syscall_$(ARCH).o \
	$(ARCH)/ewoksys/src/setjmp_$(ARCH).o \
	$(ARCH)/ewoksys/src/mstr.o \
	$(ARCH)/ewoksys/src/utf8unicode.o \
	$(ARCH)/ewoksys/src/klog.o \
	$(ARCH)/ewoksys/src/core.o \
	$(ARCH)/ewoksys/src/cmain.o \
	$(ARCH)/ewoksys/src/charbuf.o \
	$(ARCH)/ewoksys/src/vfs.o \
	$(ARCH)/ewoksys/src/ipc.o \
	$(ARCH)/ewoksys/src/ipc_serv.o \
	$(ARCH)/ewoksys/src/signal.o \
	$(ARCH)/ewoksys/src/interrupt.o \
	$(ARCH)/ewoksys/src/vdevice.o \
	$(ARCH)/ewoksys/src/devcmd.o \
	$(ARCH)/ewoksys/src/proto.o \
	$(ARCH)/ewoksys/src/semaphore.o \
	$(ARCH)/ewoksys/src/thread.o \
	$(ARCH)/ewoksys/src/basic_math.o \
	$(ARCH)/ewoksys/src/kernel_tic.o \
	$(ARCH)/ewoksys/src/trunkmem.o \
	$(ARCH)/ewoksys/src/proc.o \
	$(ARCH)/ewoksys/src/wait.o \
	$(ARCH)/ewoksys/src/md5.o \
	$(ARCH)/ewoksys/src/buffer.o \
	$(ARCH)/ewoksys/src/mmio.o  \
	$(ARCH)/ewoksys/src/dma.o  \
	$(ARCH)/ewoksys/src/shm.o  \
	$(ARCH)/ewoksys/src/shm_pipe.o  \
	$(ARCH)/ewoksys/src/session.o  \
	$(ARCH)/ewoksys/src/hashmap.o  \
	$(ARCH)/ewoksys/src/queue.o  \
	$(ARCH)/ewoksys/src/timer.o \
	$(ARCH)/ewoksys/src/sys.o \
	$(ARCH)/ewoksys/src/tty.o \
	\
	$(ARCH)/src/unistd/getuid.o \
	$(ARCH)/src/unistd/usleep.o \
	$(ARCH)/src/unistd/sleep.o \
	$(ARCH)/src/unistd/getcwd.o \
	$(ARCH)/src/unistd/getgid.o \
	$(ARCH)/src/unistd/getpid.o \
	$(ARCH)/src/unistd/getppid.o \
	$(ARCH)/src/unistd/gettid.o \
	$(ARCH)/src/unistd/ioctl.o \
	$(ARCH)/src/unistd/exec.o \
	$(ARCH)/src/unistd/setgid.o \
	$(ARCH)/src/unistd/setuid.o \
	$(ARCH)/src/unistd/chdir.o \
	$(ARCH)/src/unistd/dup2.o \
	$(ARCH)/src/unistd/dup.o \
	$(ARCH)/src/unistd/pipe.o \
	$(ARCH)/src/unistd/chown.o \
	$(ARCH)/src/unistd/rmdir.o \
	$(ARCH)/src/unistd/rename.o \
	$(ARCH)/src/unistd/errno.o \
	$(ARCH)/src/unistd/ftruncate.o \
	$(ARCH)/src/unistd/truncate.o \
	$(ARCH)/src/unistd/fsync.o \
	$(ARCH)/src/unistd/pread.o \
	$(ARCH)/src/unistd/dup3.o \
	$(ARCH)/src/unistd/sysconf.o \
	$(ARCH)/src/unistd/hostname.o \
	$(ARCH)/src/unistd/daemon.o \
	$(ARCH)/src/unistd/ttyname.o \
	$(ARCH)/src/unistd/ids.o \
	$(ARCH)/src/unistd/alarm.o \
	$(ARCH)/src/unistd/readlink.o \
	$(ARCH)/src/unistd/get_current_dir_name.o \
	\
	$(ARCH)/src/libgen/basename.o \
	$(ARCH)/src/libgen/dirname.o \
	\
	$(ARCH)/src/sched/sched.o \
	\
	$(ARCH)/src/sys/shm/shmget.o \
	$(ARCH)/src/sys/shm/shmat.o \
	$(ARCH)/src/sys/shm/shmdt.o \
	$(ARCH)/src/sys/shm/shmctl.o \
	\
	$(ARCH)/src/signal/kill.o \
	$(ARCH)/src/signal/raise.o \
	$(ARCH)/src/signal/signal.o \
	$(ARCH)/src/signal/sigaction.o \
	\
	$(ARCH)/src/err/err.o \
	\
	$(ARCH)/src/sys/stat/mkdir.o \
	$(ARCH)/src/sys/stat/lstat.o \
	$(ARCH)/src/sys/stat/mkfifo.o \
	$(ARCH)/src/sys/select/select.o \
	$(ARCH)/src/sys/utsname/uname.o \
	$(ARCH)/src/sys/wait/wait.o \
	$(ARCH)/src/sys/uio/readv.o \
	$(ARCH)/src/sys/statvfs/statvfs.o \
	$(ARCH)/src/sys/resource/resource.o \
	$(ARCH)/src/sys/time/utimes.o \
	\
	$(ARCH)/src/fcntl/fchmod.o \
	$(ARCH)/src/fcntl/chmod.o \
	$(ARCH)/src/fcntl/fcntl.o \
	\
	$(ARCH)/src/string/strcspn.o \
	$(ARCH)/src/string/strnlen.o \
	$(ARCH)/src/string/strpbrk.o \
	$(ARCH)/src/string/strspn.o \
	$(ARCH)/src/string/strsep.o \
	$(ARCH)/src/string/strlcpy.o \
	$(ARCH)/src/string/strlcat.o \
	$(ARCH)/src/string/stpcpy.o \
	$(ARCH)/src/string/memrchr.o \
	$(ARCH)/src/string/strcasestr.o \
	$(ARCH)/src/string/bstrings.o \
	\
	$(ARCH)/src/stdio/putch.o \
	$(ARCH)/src/stdio/fputs.o \
	$(ARCH)/src/stdio/setvbuf.o \
	$(ARCH)/src/stdio/ftello.o \
	$(ARCH)/src/stdio/tmpfile.o \
	$(ARCH)/src/stdio/popen.o \
	\
	$(ARCH)/src/dirent/readdir.o \
	$(ARCH)/src/dirent/closedir.o \
	$(ARCH)/src/dirent/opendir.o \
	$(ARCH)/src/dirent/dirfd.o \
	$(ARCH)/src/dirent/rewinddir.o \
	$(ARCH)/src/dirent/seekdir.o \
	$(ARCH)/src/dirent/telldir.o \
	\
	$(ARCH)/src/pthread/pthread_create.o \
	$(ARCH)/src/pthread/pthread_key.o \
	$(ARCH)/src/pthread/pthread_exit.o \
	$(ARCH)/src/pthread/pthread_mutex_destroy.o \
	$(ARCH)/src/pthread/pthread_mutex_init.o \
	$(ARCH)/src/pthread/pthread_mutex_lock.o \
	$(ARCH)/src/pthread/pthread_mutex_unlock.o \
	$(ARCH)/src/pthread/pthread_mutex_timedlock.o\
	$(ARCH)/src/pthread/pthread_self.o\
	$(ARCH)/src/pthread/pthread_equal.o\
	$(ARCH)/src/pthread/pthread_join.o\
	$(ARCH)/src/pthread/pthread_detach.o\
	$(ARCH)/src/pthread/pthread_cond.o\
	$(ARCH)/src/pthread/pthread_once.o\
	$(ARCH)/src/pthread/pthread_key.o\
	$(ARCH)/src/pthread/pthread_exit.o\
	$(ARCH)/src/pthread/pthread_attr.o\
	$(ARCH)/src/pthread/pthread_attr_obj.o\
	$(ARCH)/src/pthread/pthread_cancel.o\
	$(ARCH)/src/pthread/pthread_rwlock.o\
	$(ARCH)/src/pthread/pthread_barrier.o\
	\
	$(ARCH)/src/semaphore/sem_init.o\
	$(ARCH)/src/semaphore/sem_wait.o\
	$(ARCH)/src/semaphore/sem_post.o\
	\
	$(ARCH)/src/termios/termios.o \
	$(ARCH)/src/fnmatch/fnmatch.o \
	$(ARCH)/src/glob/glob.o \
	$(ARCH)/src/pwd/pwd.o \
	$(ARCH)/src/grp/grp.o \
	$(ARCH)/src/sys/inotify/inotify.o \
	\
	$(ARCH)/src/poll/poll.o \
	\
	$(ARCH)/src/stdlib/assert.o \
	$(ARCH)/src/stdlib/exit.o \
	$(ARCH)/src/stdlib/llabs.o \
	$(ARCH)/src/stdlib/mkstemp.o \
	$(ARCH)/src/stdlib/setenv.o \
	$(ARCH)/src/stdlib/tempnam.o \
	$(ARCH)/src/stdlib/mkdtemp.o \
	$(ARCH)/src/stdlib/realpath.o \
	\
	$(ARCH)/src/time/strftime.o \
	$(ARCH)/src/time/nanosleep.o \
	$(ARCH)/src/time/clock_gettime.o \
	$(ARCH)/src/time/clock_getres.o \
	$(ARCH)/src/time/clock_settime.o \
	$(ARCH)/src/time/strptime.o
	
EWOKSYS = $(TARGET_DIR)/lib/libewoksys.a
OBJ_DIRS = $(sort $(dir $(LIB_EWOKSYS_OBJS)))

all: prep_obj_dirs $(EWOKSYS) install_headers

prep_obj_dirs:
	@mkdir -p $(OBJ_DIRS)

$(EWOKSYS): $(LIB_EWOKSYS_OBJS)   $(LIB_EWOKSYS_SYS_OBJS)
	$(AR) rcs $(EWOKSYS) $(LIB_EWOKSYS_OBJS) 

install_headers:
	mkdir -p $(TARGET_DIR)/include/ewoksys $(TARGET_DIR)/include/sys
	cp -r ewoksys/include/ewoksys/* $(TARGET_DIR)/include/ewoksys/
	cp -r include/sys/* $(TARGET_DIR)/include/sys/
	cp include/*.h $(TARGET_DIR)/include/

clean:
	rm -rf $(ARCH)
	rm -f $(EWOKSYS)
