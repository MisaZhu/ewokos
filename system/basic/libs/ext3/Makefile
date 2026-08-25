SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_EXT3_OBJS = $(ARCH)/src/ext3fs.o $(ARCH)/src/jbd.o

EXT3 = $(TARGET_DIR)/lib/libext3.a

$(EXT3): $(LIB_EXT3_OBJS)
	$(AR) rcs $(EXT3) $(LIB_EXT3_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_EXT3_OBJS)
	rm -f $(EXT3)
