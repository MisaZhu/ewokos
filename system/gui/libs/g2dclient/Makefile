SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_G2D_CLIENT_OBJS = $(ARCH)/src/g2dclient.o
LIB_G2D_CLIENT_HEADERS = include/g2dclient/g2dclient.h

LIB_G2D_CLIENT = $(TARGET_DIR)/lib/libg2dclient.a

$(LIB_G2D_CLIENT): $(LIB_G2D_CLIENT_OBJS) $(LIB_G2D_CLIENT_HEADERS)
	$(AR) rcs $(LIB_G2D_CLIENT) $(LIB_G2D_CLIENT_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_G2D_CLIENT_OBJS)
	rm -f $(LIB_G2D_CLIENT)
