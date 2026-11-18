SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

MULTITOUCH_OBJS = $(ARCH)/multitouch.o

MULTITOUCH_DIR = $(TARGET_DIR)/apps/multitouch
MULTITOUCH = $(MULTITOUCH_DIR)/multitouch

all: $(MULTITOUCH_DIR) $(MULTITOUCH)

$(MULTITOUCH): $(MULTITOUCH_OBJS) \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(MULTITOUCH_OBJS) -o $(MULTITOUCH) $(LDFLAGS) -lx -ltinyjson $(EWOK_LIB_GRAPH) $(EWOK_LIBC)
	cp -r res $(MULTITOUCH_DIR)

$(ARCH)/multitouch.o: multitouch.c
	@mkdir -p $(ARCH)
	$(CC) $(CFLAGS) -c $< -o $@

$(MULTITOUCH_DIR):
	mkdir -p $(MULTITOUCH_DIR)

clean:
	rm -f $(MULTITOUCH_OBJS)
	rm -fr $(MULTITOUCH_DIR)