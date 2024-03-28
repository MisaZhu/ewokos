include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

CORE_OBJS = core.o 
	
CORE = $(TARGET_DIR)/$(ROOT_DIR)/sbin/core

$(CORE): $(CORE_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(CORE_OBJS) -o $(CORE) $(LDFLAGS)  $(EWOK_LIBC) 

clean:
	rm -fr $(CORE) $(CORE_OBJS)