SYS_DIR=../../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

EV3_UARTD_OBJS = uartd.o

EV3_UARTD = $(TARGET_DIR)/drivers/ev3/uartd

$(EV3_UARTD): $(EV3_UARTD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(EV3_UARTD_OBJS) -o $(EV3_UARTD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(EV3_UARTD_OBJS) $(EV3_UARTD)
