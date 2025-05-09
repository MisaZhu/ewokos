SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_textgrid_OBJS = src/textgrid.o

textgrid = $(TARGET_DIR)/lib/libtextgrid.a

$(textgrid): $(LIB_textgrid_OBJS)
	$(AR) rcs $(textgrid) $(LIB_textgrid_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_textgrid_OBJS)
	rm -f $(textgrid)

