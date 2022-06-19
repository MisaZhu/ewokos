include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

SDFSD_OBJS = sdfsd.o 
	
SDFSD = $(TARGET_DIR)/$(ROOT_DIR)/sbin/sdfsd

$(SDFSD): $(SDFSD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libhash.a \
		$(BUILD_DIR)/lib/libsd.a \
		$(BUILD_DIR)/lib/libbsp.a \
		$(BUILD_DIR)/lib/libext2.a 
	$(LD) -Ttext=100 $(SDFSD_OBJS) -o $(SDFSD) $(LDFLAGS) -lbsp -lsd -lext2 -lhash -lewokc -lc -lnosys
	$(OBJDUMP) -D $(SDFSD) > $(BUILD_DIR)/asm/sdfsd.asm

clean:
	rm -fr $(SDFSD) $(SDFSD_OBJS)
