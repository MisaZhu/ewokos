SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

.DEFAULT_GOAL := all

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

ABI_HEADERS = $(BUILD_DIR)/include/sysinfo.h $(BUILD_DIR)/include/procinfo.h

GCORES_OBJS = xcores.o

GCORES_DIR = $(TARGET_DIR)/apps/xcores
GCORES = $(GCORES_DIR)/xcores

xcores.o: xcores.cc $(ABI_HEADERS)

all: $(GCORES_DIR) $(GCORES)

$(GCORES): $(GCORES_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(GCORES_OBJS) -o $(GCORES) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(GCORES_DIR):
	mkdir -p $(GCORES_DIR)
	cp -r res $(GCORES_DIR)

clean:
	rm -f $(GCORES_OBJS)
	rm -fr $(GCORES_DIR)
