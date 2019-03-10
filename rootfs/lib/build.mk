LIB_OBJS = lib/src/syscall.o \
	lib/src/stdlib.o \
	lib/src/dev/mmio.o \
	lib/src/dev/devserv.o \
	lib/src/pmessage.o \
	lib/src/stdio.o \
	lib/src/cmain.o \
	lib/src/unistd.o \
	lib/src/vfs/vfs.o \
	lib/src/vfs/fs.o \
	lib/src/graph/font.o \
	lib/src/graph/graph.o \
	lib/src/shm.o \
	lib/src/proto.o \
	lib/src/package.o \
	lib/src/kserv.o

lib/libewoklibc.a: $(LIB_OBJS)
	mkdir -p build
	$(AR) rT $@ $(LIB_OBJS)	

EXTRA_CLEAN += $(LIB_OBJS) lib/libewoklibc.a
