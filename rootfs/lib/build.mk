LIB_OBJS = lib/src/syscall.o \
	lib/src/stdlib.o \
	lib/src/dev/devserv.o \
	lib/src/ipc.o \
	lib/src/stdio.o \
	lib/src/cmain.o \
	lib/src/unistd.o \
	lib/src/vfs/vfs.o \
	lib/src/vfs/fs.o \
	lib/src/graph/font.o \
	lib/src/graph/font4x6.o \
	lib/src/graph/font5x12.o \
	lib/src/graph/font6x8.o \
	lib/src/graph/font7x9.o \
	lib/src/graph/font8x8.o \
	lib/src/graph/font9x8.o \
	lib/src/graph/font8x10.o \
	lib/src/graph/font8x16.o \
	lib/src/graph/font9x16.o \
	lib/src/graph/font12x16.o \
	lib/src/graph/font10x20.o \
	lib/src/graph/font12x24.o \
	lib/src/graph/font16x32.o \
	lib/src/graph/graph.o \
	lib/src/shm.o \
	lib/src/proto.o \
	lib/src/package.o \
	lib/src/thread.o \
	lib/src/semaphore.o \
	lib/src/sdread.o \
	lib/src/sconf.o \
	lib/src/kserv.o

lib/libewoklibc.a: $(LIB_OBJS)
	mkdir -p build
	$(AR) rT $@ $(LIB_OBJS)	

EXTRA_CLEAN += $(LIB_OBJS) lib/libewoklibc.a
