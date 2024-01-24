include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

CAT_OBJS = cat.o

CAT = $(TARGET_DIR)/bin/cat

$(CAT): $(CAT_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a 
	$(LD) -Ttext=100 $(CAT_OBJS) -o $(CAT) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(CAT_OBJS)

