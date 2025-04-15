SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_GPIO_OBJS = src/gpio.o

GPIO = $(TARGET_DIR)/lib/libgpio.a

$(GPIO): $(LIB_GPIO_OBJS)
	$(AR) rcs $(GPIO) $(LIB_GPIO_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_GPIO_OBJS)
	rm -f $(GPIO)

