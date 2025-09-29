SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SESSIOND_OBJS = sessiond.o 
	
SESSIOND = $(TARGET_DIR)/$(ROOT_DIR)/sbin/sessiond

$(SESSIOND): $(SESSIOND_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libbsp.a \
		$(BUILD_DIR)/lib/libsd.a \
		$(BUILD_DIR)/lib/libext2.a 
	$(LD) -Ttext=100 $(SESSIOND_OBJS) -o $(SESSIOND) $(LDFLAGS)  $(BSP_LFLAGS) -lsd -lext2  $(EWOK_LIBC) 

clean:
	rm -fr $(SESSIOND) $(SESSIOND_OBJS)
