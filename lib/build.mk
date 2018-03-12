LIB_OBJS = lib/src/string.o \
	lib/src/kdb.o \
	lib/src/syscall.o \
	lib/src/stdlib.o \
	lib/src/vsprintf.o \
	lib/src/pmessage.o \
	lib/src/sramdisk.o \
	lib/src/stdio.o \
	lib/src/package.o \
	lib/src/unistd.o \
	lib/src/cmain.o \
	lib/src/kserv/fs.o 

lib/libewok.a: $(LIB_OBJS)
	mkdir -p build
	$(AR) rT $@ $(LIB_OBJS)	

EXTRA_CLEAN += $(LIB_OBJS) lib/libewok.a
