include ../../../platform/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I $(TARGET_DIR)/include -Iinclude

LIB_CONSOLE_OBJS = src/console.o

CONSOLE = $(TARGET_DIR)/lib/libconsole.a

$(CONSOLE): $(LIB_CONSOLE_OBJS)
	$(AR) rcs $(CONSOLE) $(LIB_CONSOLE_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_CONSOLE_OBJS)
	rm -f $(CONSOLE)

