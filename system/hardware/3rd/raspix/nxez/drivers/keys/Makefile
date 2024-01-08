include ../../make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

KEYSD_OBJS = keysd.o

KEYSD = $(TARGET_DIR)/drivers/raspix/nxez_keysd

$(KEYSD): $(KEYSD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(KEYSD_OBJS) -o $(KEYSD) $(LDFLAGS) -larch_bcm283x -lewokc -lc_nano -lnosys

clean:
	rm -f $(KEYSD_OBJS)
