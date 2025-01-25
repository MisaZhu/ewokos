include ../../../make.inc

EXTRA_LIBS_DIR = ../../../libs

RPI_LCDD_OBJS = rpi_lcdd.o \
		$(EXTRA_LIBS_DIR)/xpt2046/src/xpt2046.o \
		$(EXTRA_LIBS_DIR)/ili9486/src/ili9486.o

CFLAGS += -I $(EXTRA_LIBS_DIR)/ili9486/include \
		-I $(EXTRA_LIBS_DIR)/xpt2046/include 

RPI_LCDD = $(TARGET_DIR)/drivers/$(MF)/rpi_lcdd

$(RPI_LCDD): $(RPI_LCDD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libfbd.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libupng.a
	$(LD) -Ttext=100 $(RPI_LCDD_OBJS) -o $(RPI_LCDD) $(LDFLAGS) -larch_bcm283x -lfbd -lupng -lgraph -lbsp -lsconf $(EWOK_LIBC)

clean:
	rm -f $(RPI_LCDD_OBJS)
