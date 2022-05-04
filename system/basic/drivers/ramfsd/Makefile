include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

RAMFSD_OBJS = ramfsd.o

RAMFSD = $(TARGET_DIR)/drivers/ramfsd

$(RAMFSD): $(RAMFSD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(RAMFSD_OBJS) -o $(RAMFSD) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(RAMFSD_OBJS)
