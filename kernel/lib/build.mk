LIB_OBJS = kernel/lib/src/string.o \
	kernel/lib/src/sramdisk.o \
	kernel/lib/src/package.o 

kernel/lib/libewok.a: $(LIB_OBJS)
	mkdir -p build
	$(AR) rT $@ $(LIB_OBJS)	

EXTRA_CLEAN += $(LIB_OBJS) kernel/lib/libewok.a
