SYS_DIR=../../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I$(SYS_DIR)/basic/libc/libewoksys/ewoksys/include -Iinclude -I $(TARGET_DIR)/include

LIB_ARCH_EV3_OBJS = src/framebuffer.o \
	src/gpio.o \
	src/gpio.o \
	src/spi.o \
	src/sd.o 

ARCH_EV3 = $(TARGET_DIR)/lib/libarch_ev3.a

$(ARCH_EV3): $(LIB_ARCH_EV3_OBJS)
	$(AR) rcs $(ARCH_EV3) $(LIB_ARCH_EV3_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_ARCH_EV3_OBJS)
	rm -f $(ARCH_EV3)


