SYS_DIR=../../../../..
include $(SYS_DIR)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I$(SYS_DIR)/basic/libs/libc/sys/include -Iinclude -I $(TARGET_DIR)/include

LIB_ARCH_NEZHA_OBJS = src/sd.o 

ARCH_NEZHA = $(TARGET_DIR)/lib/libarch_nezha.a

$(ARCH_NEZHA): $(LIB_ARCH_NEZHA_OBJS)
	$(AR) rT $(ARCH_NEZHA) $(LIB_ARCH_NEZHA_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_ARCH_NEZHA_OBJS)
	rm -f $(ARCH_NEZHA)


