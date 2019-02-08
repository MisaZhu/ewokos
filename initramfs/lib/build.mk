LIB_OBJS = lib/src/syscall.o \
	lib/src/stdlib.o \
	lib/src/vsprintf.o \
	lib/src/pmessage.o \
	lib/src/stdio.o \
	lib/src/unistd.o \
	lib/src/cmain.o \
	lib/src/kserv/fs.o \
	lib/src/kserv/userman.o

lib/libewoklibc.a: $(LIB_OBJS)
	mkdir -p build
	$(AR) rT $@ $(LIB_OBJS)	

EXTRA_CLEAN += $(LIB_OBJS) lib/libewoklibc.a
