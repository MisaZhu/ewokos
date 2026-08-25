SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

FAT32FSD_OBJS = $(ARCH)/fat32fsd.o

FAT32FSD = $(TARGET_DIR)/drivers/$(HW)/usbfat32fsd

$(FAT32FSD): $(FAT32FSD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a  \
		$(BUILD_DIR)/lib/libsd.a \
		$(BUILD_DIR)/lib/libbsp.a \
		$(BUILD_DIR)/lib/libfat32.a 
	$(LD) -Ttext=100 $(FAT32FSD_OBJS) -o $(FAT32FSD) $(LDFLAGS)  $(BSP_LFLAGS) -lsd -lfat32  $(EWOK_LIBC) 

clean:
	rm -fr $(FAT32FSD) $(FAT32FSD_OBJS)
