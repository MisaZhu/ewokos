include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SVCINFO_OBJS = svcinfo.o

SVCINFO = $(TARGET_DIR)/bin/svcinfo

$(SVCINFO): $(SVCINFO_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a 
	$(LD) -Ttext=100 $(SVCINFO_OBJS) -o $(SVCINFO) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(SVCINFO_OBJS)

