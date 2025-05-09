SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I$(SYS_DIR)/basic/libc/libewoksys/ewoksys/include -Iinclude -I $(TARGET_DIR)/include

LIB_ARCH_BCM283X_OBJS = src/framebuffer.o \
	src/gpio_actled.o \
	src/gpio.o \
	src/mailbox.o \
	src/mbox_actled.o \
	src/mini_uart.o \
	src/pl011_uart.o \
	src/sd.o \
	src/i2c.o \
	src/spi.o \
	src/mmc.o \
	src/emmc.o \
	src/sdhost.o \
	src/auxspi.o 

ARCH_BCM283X = $(TARGET_DIR)/lib/libarch_bcm283x.a

$(ARCH_BCM283X): $(LIB_ARCH_BCM283X_OBJS)
	$(AR) rs $(ARCH_BCM283X) $(LIB_ARCH_BCM283X_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_ARCH_BCM283X_OBJS)
	rm -f $(ARCH_BCM283X)ewoksys


