SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_FAT32_OBJS = $(ARCH)/src/fat32fs.o

FAT32 = $(TARGET_DIR)/lib/libfat32.a

$(FAT32): $(LIB_FAT32_OBJS)
	$(AR) rcs $(FAT32) $(LIB_FAT32_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_FAT32_OBJS)
	rm -f $(FAT32)
