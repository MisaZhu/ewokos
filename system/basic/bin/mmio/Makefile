SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

MMIO_OBJS = mmio.o

MMIO = $(TARGET_DIR)/bin/mmio

$(MMIO): $(MMIO_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(MMIO_OBJS) -o $(MMIO) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(MMIO_OBJS)

