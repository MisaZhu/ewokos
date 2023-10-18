include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

VFSD_OBJS = vfsd.o 
	
VFSD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/vfsd

$(VFSD): $(VFSD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libhash.a
	$(LD) -Ttext=100 $(VFSD_OBJS) -o $(VFSD) $(LDFLAGS) -lhash -lewokc -lc

clean:
	rm -fr $(VFSD) $(VFSD_OBJS)