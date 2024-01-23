include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -Iinclude -I $(BUILD_DIR)/include

DISPLAYD_OBJS = displayd.o

DISPLAYD = $(TARGET_DIR)/drivers/displayd

$(DISPLAYD): $(DISPLAYD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(DISPLAYD_OBJS) -o $(DISPLAYD) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(DISPLAYD_OBJS)
