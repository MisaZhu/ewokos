SYS_DIR=../../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

ADCD_OBJS = main.o

ADCD = $(TARGET_DIR)/drivers/ev3/adcd

$(ADCD): $(ADCD_OBJS) \
	   	$(BUILD_DIR)/lib/libarch_ev3.a \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(ADCD_OBJS) -o $(ADCD) -larch_ev3 -lbsp $(BSP_LFLAGS) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(ADCD_OBJS) $(ADCD)
