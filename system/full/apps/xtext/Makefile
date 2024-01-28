SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XTEXT_OBJS = xtext.o

XTEXT_DIR = $(TARGET_DIR)/apps/xtext
XTEXT = $(XTEXT_DIR)/xtext

all: $(XTEXT_DIR) $(XTEXT)

$(XTEXT): $(XTEXT_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libtextview.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XTEXT_OBJS) -o $(XTEXT) $(LDFLAGS) -lsconf -lx++ -lx -lsconf -lcxx -lttf  -ltextview -lfont -lgraph -lbsp $(EWOK_LIBC)

$(XTEXT_DIR):
	mkdir -p $(XTEXT_DIR)
	cp -r res $(XTEXT_DIR)

clean:
	rm -f $(XTEXT_OBJS)
	rm -fr $(XTEXT_DIR)

