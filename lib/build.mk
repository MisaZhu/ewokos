LIB_OBJS = lib/src/string.o \
	lib/src/sramdisk.o \
	lib/src/package.o 

lib/libewok.a: $(LIB_OBJS)
	mkdir -p build
	$(AR) rT $@ $(LIB_OBJS)	

EXTRA_CLEAN += $(LIB_OBJS) lib/libewok.a
