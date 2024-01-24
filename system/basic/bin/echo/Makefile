include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

ECHO_OBJS = echo.o

ECHO = $(TARGET_DIR)/bin/echo

$(ECHO): $(ECHO_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a 
	$(LD) -Ttext=100 $(ECHO_OBJS) -o $(ECHO) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(ECHO_OBJS)

