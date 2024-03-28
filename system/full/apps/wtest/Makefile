SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

WTEST_OBJS = wtest.o

WTEST_DIR = $(TARGET_DIR)/apps/wtest
WTEST = $(WTEST_DIR)/wtest

all: $(WTEST_DIR) $(WTEST)

$(WTEST): $(WTEST_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(WTEST_OBJS) -o $(WTEST) $(LDFLAGS) -lwidget++ -lgraph -lbsp -lttf -lfont -lx++ -lx -lsconf -lupng -lewokstl $(EWOK_LIBC) -lcxx

$(WTEST_DIR):
	mkdir -p $(WTEST_DIR)
	cp -r res $(WTEST_DIR)

clean:
	rm -f $(WTEST_OBJS)
	rm -fr $(WTEST_DIR)

