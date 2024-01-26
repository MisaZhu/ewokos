include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

PS_OBJS = ps.o

PS = $(TARGET_DIR)/bin/ps

$(PS): $(PS_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(PS_OBJS) -o $(PS) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(PS_OBJS)

