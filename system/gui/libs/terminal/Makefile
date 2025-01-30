SYS_DIR=../../../
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_TERMINAL_OBJS = src/terminal.o

TERMINAL = $(TARGET_DIR)/lib/libterminal.a

$(TERMINAL): $(LIB_TERMINAL_OBJS)
	$(AR) rcs $(TERMINAL) $(LIB_TERMINAL_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_TERMINAL_OBJS)
	rm -f $(TERMINAL)

