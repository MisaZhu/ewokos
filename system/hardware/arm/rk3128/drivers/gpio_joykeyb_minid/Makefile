include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

JOY_KEYBD_MINI_OBJS = joykeybd.o

JOY_KEYBD_MINI = $(TARGET_DIR)/drivers/rk3128/gpio_joykeyb_minid

$(JOY_KEYBD_MINI): $(JOY_KEYBD_MINI_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(JOY_KEYBD_MINI_OBJS) -o $(JOY_KEYBD_MINI) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(JOY_KEYBD_MINI_OBJS)
