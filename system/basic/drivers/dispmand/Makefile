include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -Iinclude -I $(BUILD_DIR)/include

DISPMAND_OBJS = dispmand.o

DISPMAND = $(TARGET_DIR)/drivers/dispmand

$(DISPMAND): $(DISPMAND_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a 
	$(LD) -Ttext=100 $(DISPMAND_OBJS) -o $(DISPMAND) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(DISPMAND_OBJS)
