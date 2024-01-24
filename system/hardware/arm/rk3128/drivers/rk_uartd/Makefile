include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

RK_UARTD_OBJS = rk_uartd.o

RK_UARTD = $(TARGET_DIR)/drivers/rk3128/rk_uartd

$(RK_UARTD): $(RK_UARTD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a 
	$(LD) -Ttext=100 $(RK_UARTD_OBJS) -o $(RK_UARTD) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(RK_UARTD_OBJS) $(RK_UARTD)
