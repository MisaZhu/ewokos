SYS_DIR=../../../../..
include $(SYS_DIR)/hardware/arm/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I$(SYS_DIR)/basic/libs/libc/sys/include -Iinclude -I $(TARGET_DIR)/include

LIB_ARCH_RK3326_OBJS = src/framebuffer.o \
	src/dwmmc.o \
	src/sd.o 

ARCH_RK3326 = $(TARGET_DIR)/lib/libarch_rk3326.a

$(ARCH_RK3326): $(LIB_ARCH_RK3326_OBJS)
	$(AR) rT $(ARCH_RK3326) $(LIB_ARCH_RK3326_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_ARCH_RK3326_OBJS)
	rm -f $(ARCH_RK3326)


