include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

INIT_OBJS = init.o 
	
INIT = $(TARGET_DIR)/$(ROOT_DIR)/sbin/init

$(INIT): $(INIT_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libhash.a \
		$(BUILD_DIR)/lib/libbsp.a \
		$(BUILD_DIR)/lib/libsd.a \
		$(BUILD_DIR)/lib/libext2.a 
	$(LD) -Ttext=100 $(INIT_OBJS) -o $(INIT) $(LDFLAGS) -lbsp -lsd -lext2  -lhash -lewokc -lc -lnosys

clean:
	rm -fr $(INIT) $(INIT_OBJS)
