include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -Iinclude -I $(BUILD_DIR)/include

SCREEND_OBJS = screend.o

SCREEND = $(TARGET_DIR)/drivers/screend

$(SCREEND): $(SCREEND_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(SCREEND_OBJS) -o $(SCREEND) $(LDFLAGS) -lewokc -lc
	cp -r include/* $(BUILD_DIR)/include

clean:
	rm -f $(SCREEND_OBJS)
