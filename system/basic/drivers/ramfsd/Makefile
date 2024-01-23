include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

RAMFSD_OBJS = ramfsd.o

RAMFSD = $(TARGET_DIR)/drivers/ramfsd

$(RAMFSD): $(RAMFSD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(RAMFSD_OBJS) -o $(RAMFSD) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(RAMFSD_OBJS)
