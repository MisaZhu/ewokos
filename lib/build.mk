LIB_OBJS = lib/src/string.o \
	lib/src/syscall.o \
	lib/src/fork.o \
	lib/src/malloc.o \
	lib/src/serv.o \
	lib/src/vsprintf.o \
	lib/src/stdio.o

lib/libewok.a: $(LIB_OBJS)
	mkdir -p build
	$(AR) rT $@ $(LIB_OBJS)	

EXTRA_CLEAN += $(LIB_OBJS) lib/libewok.a
