include ../../../make.inc

EXTRA_LIBS_DIR = ../../../libs
CFLAGS +=  -I $(EXTRA_LIBS_DIR)/ili9486/include

ILI9486D_OBJS = ili9486d.o \
		$(EXTRA_LIBS_DIR)/ili9486/src/ili9486.o

ILI9486D = $(TARGET_DIR)/drivers/$(MF)/ili9486d

$(ILI9486D): $(ILI9486D_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libfbd.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libupng.a 
	$(LD) -Ttext=100 $(ILI9486D_OBJS) -o $(ILI9486D) $(LDFLAGS) -larch_bcm283x -lfbd -lupng -lgraph -lbsp -lsconf $(EWOK_LIBC)

clean:
	rm -f $(ILI9486D_OBJS)
