include ../../make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

JOYKEYBD_OBJS = joykeybd.o

JOYKEYBD = $(TARGET_DIR)/drivers/raspix/hat13_joykeybd

$(JOYKEYBD): $(JOYKEYBD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(JOYKEYBD_OBJS) -o $(JOYKEYBD) $(LDFLAGS) -larch_bcm283x -lewokc -lc_nano -lnosys

clean:
	rm -f $(JOYKEYBD_OBJS)
