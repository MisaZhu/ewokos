include ../../../platform/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_SOCKET_OBJS = src/socket.o

SOCKET = $(TARGET_DIR)/lib/libsocket.a


$(SOCKET): $(LIB_SOCKET_OBJS)
	$(AR) rT $(SOCKET) $(LIB_SOCKET_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_SOCKET_OBJS)
	rm -f $(SOCKET)

