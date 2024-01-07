include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SESSIOND_OBJS = sessiond.o 
	
SESSIOND = $(TARGET_DIR)/$(ROOT_DIR)/sbin/sessiond

$(SESSIOND): $(SESSIOND_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libhash.a \
		$(BUILD_DIR)/lib/libbsp.a \
		$(BUILD_DIR)/lib/libsd.a \
		$(BUILD_DIR)/lib/libext2.a 
	$(LD) -Ttext=100 $(SESSIOND_OBJS) -o $(SESSIOND) $(LDFLAGS) -lbsp -lsd -lext2  -lhash -lewokc -lc_nano -lnosys -lnosys

clean:
	rm -fr $(SESSIOND) $(SESSIOND_OBJS)
