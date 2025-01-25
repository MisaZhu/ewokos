SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XPROCS_OBJS = xprocs.o

XPROCS_DIR = $(TARGET_DIR)/apps/xprocs
XPROCS = $(XPROCS_DIR)/xprocs

all: $(XPROCS_DIR) $(XPROCS)

$(XPROCS): $(XPROCS_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XPROCS_OBJS) -o $(XPROCS) $(LDFLAGS) -lwidget++ -lgraph -lbsp -lttf -lfont -lx++ -lx -lsconf -lupng -lewokstl $(EWOK_LIBC) -lcxx

$(XPROCS_DIR):
	mkdir -p $(XPROCS_DIR)
	cp -r res $(XPROCS_DIR)

clean:
	rm -f $(XPROCS_OBJS)
	rm -fr $(XPROCS_DIR)

