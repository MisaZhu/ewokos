include ../../../platform/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

VFSD_OBJS = vfsd.o 
	
VFSD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/vfsd

$(VFSD): $(VFSD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(VFSD_OBJS) -o $(VFSD) $(LDFLAGS)  $(EWOK_LIBC) 

clean:
	rm -fr $(VFSD) $(VFSD_OBJS)