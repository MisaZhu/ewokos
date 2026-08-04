SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_G2D_OBJS = $(ARCH)/src/g2d.o
LIB_G2D_HEADERS = include/g2d/g2d.h

LIB_G2D = $(TARGET_DIR)/lib/libg2d.a

$(LIB_G2D): $(LIB_G2D_OBJS) $(LIB_G2D_HEADERS)
	$(AR) rcs $(LIB_G2D) $(LIB_G2D_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_G2D_OBJS)
	rm -f $(LIB_G2D)
