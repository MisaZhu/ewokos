include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

WHOAMI_OBJS = whoami.o

WHOAMI = $(TARGET_DIR)/bin/whoami

$(WHOAMI): $(WHOAMI_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(WHOAMI_OBJS) -o $(WHOAMI) $(LDFLAGS) -lewoksys -lc -lgloss -lsoftfloat

clean:
	rm -f $(WHOAMI_OBJS)

