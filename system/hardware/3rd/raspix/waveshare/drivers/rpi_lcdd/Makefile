include ../../make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

RPI_LCDD_OBJS = rpi_lcdd.o

RPI_LCDD = $(TARGET_DIR)/drivers/raspix/rpi_lcdd

$(RPI_LCDD): $(RPI_LCDD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libfbd.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libili9486.a  \
		$(BUILD_DIR)/lib/libxpt2046.a 
	$(LD) -Ttext=100 $(RPI_LCDD_OBJS) -o $(RPI_LCDD) $(LDFLAGS) -lxpt2046 -lili9486 -larch_bcm283x -lfbd -lgraph -lewokc -lc

clean:
	rm -f $(RPI_LCDD_OBJS)
