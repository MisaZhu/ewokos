SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -Llib 

CFLAGS += -I $(BUILD_DIR)/include

PMU_OBJS = powerd.o

PMU = $(TARGET_DIR)/drivers/versatilepb/powerd

$(PMU): $(PMU_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_vpb.a
	$(LD) -Ttext=100 $(PMU_OBJS) -o $(PMU) $(LDFLAGS) -larch_vpb $(EWOK_LIBC)

clean:
	rm -f $(PMU_OBJS)
