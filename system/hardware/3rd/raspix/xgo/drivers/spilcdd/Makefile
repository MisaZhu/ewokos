include ../../make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SPILCDD_OBJS = lcd_reg.o spilcdd.o

SPILCDD = $(TARGET_DIR)/drivers/raspix/spilcdd

$(SPILCDD): $(SPILCDD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libfbd.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libupng.a
	$(LD) -Ttext=100 $(SPILCDD_OBJS) -o $(SPILCDD) $(LDFLAGS)  -larch_bcm283x -lfbd -lupng -lgraph -lbsp -lsconf $(EWOK_LIBC)

clean:
	rm -f $(SPILCDD_OBJS)
