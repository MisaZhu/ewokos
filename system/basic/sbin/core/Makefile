include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

CORE_OBJS = core.o 
	
CORE = $(TARGET_DIR)/$(ROOT_DIR)/sbin/core

$(CORE): $(CORE_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a 
	$(LD) -Ttext=100 $(CORE_OBJS) -o $(CORE) $(LDFLAGS)  -lewoksys -lc -lgloss 

clean:
	rm -fr $(CORE) $(CORE_OBJS)