SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XFONT_OBJS = xfont.o

XFONT_DIR = $(TARGET_DIR)/apps/xfont
XFONT = $(XFONT_DIR)/xfont

all: $(XFONT_DIR) $(XFONT)

$(XFONT): $(XFONT_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XFONT_OBJS) -o $(XFONT) $(LDFLAGS) -lwidget++ -lgraph -lbsp -lttf -lfont -lx++ -lx -lsconf -lupng -lewokstl $(EWOK_LIBC) -lcxx

$(XFONT_DIR):
	mkdir -p $(XFONT_DIR)
	cp -r res $(XFONT_DIR)

clean:
	rm -f $(XFONT_OBJS)
	rm -fr $(XFONT_DIR)

