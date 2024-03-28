SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

FINDER_OBJS = finder.o

FINDER_DIR = $(TARGET_DIR)/apps/finder
FINDER = $(FINDER_DIR)/finder

all: $(FINDER_DIR) $(FINDER)

$(FINDER): $(FINDER_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libelf.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libtinyjson.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(FINDER_OBJS) -o $(FINDER) $(LDFLAGS)  -lgraph -lbsp -lttf -lfont -lupng -lx++ -lx -ltinyjson -lsconf -lelf $(EWOK_LIBC) -lcxx

$(FINDER_DIR):
	mkdir -p $(FINDER_DIR)
	cp -r res $(FINDER_DIR)

clean:
	rm -f $(FINDER_OBJS)
	rm -fr $(FINDER_DIR)

