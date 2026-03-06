SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XMEM_OBJS = xMem.o

XMEM_DIR = $(TARGET_DIR)/apps/xMem
XMEM = $(XMEM_DIR)/xMem

all: $(XMEM_DIR) $(XMEM)

$(XMEM): $(XMEM_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XMEM_OBJS) -o $(XMEM) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(XMEM_DIR):
	mkdir -p $(XMEM_DIR)
	cp -r res $(XMEM_DIR)

clean:
	rm -f $(XMEM_OBJS)
	rm -fr $(XMEM_DIR)
