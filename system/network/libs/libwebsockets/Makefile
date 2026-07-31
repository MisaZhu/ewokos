SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I$(TARGET_DIR)/include \
	-I$(SYS_DIR)/network/libs/socket/include \
	-I$(SYS_DIR)/network/libs/wolfssl/include \
	-I$(SYS_DIR)/network/libs/wolfssl \
	-DWOLFSSL_USER_SETTINGS \
	-Wno-unused-parameter -Wno-sign-compare

LIB_LWS_OBJS = \
	$(ARCH)/src/sha-1.o \
	$(ARCH)/src/base64.o \
	$(ARCH)/src/context.o \
	$(ARCH)/src/service.o \
	$(ARCH)/src/handshake.o \
	$(ARCH)/src/websocket.o \
	$(ARCH)/src/client.o \
	$(ARCH)/src/misc.o \
	$(ARCH)/src/tls.o

LWS = $(TARGET_DIR)/lib/libwebsockets.a

all: $(LWS)

$(LWS): $(LIB_LWS_OBJS)
	$(AR) rcs $(LWS) $(LIB_LWS_OBJS)
	mkdir -p $(TARGET_DIR)/include/libwebsockets
	cp -r include/libwebsockets/* $(TARGET_DIR)/include/libwebsockets/

clean:
	rm -f $(LIB_LWS_OBJS)
	rm -f $(LWS)
