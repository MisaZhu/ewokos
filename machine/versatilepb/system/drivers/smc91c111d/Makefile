SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SMC91C111D_OBJS = smc91c111d.o

SMC91C111D = $(TARGET_DIR)/drivers/versatilepb/smc91c111d

$(SMC91C111D): $(SMC91C111D_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(SMC91C111D_OBJS) -o $(SMC91C111D) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(SMC91C111D_OBJS) $(SMC91C111D)
