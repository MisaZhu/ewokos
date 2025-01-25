include ../../../../platform/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XSESSION_OBJS = xsession.o

XSESSION = $(TARGET_DIR)/bin/x/xsession

$(XSESSION): $(XSESSION_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libupng.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XSESSION_OBJS) -o $(XSESSION) $(LDFLAGS)  -lgraph -lbsp -lupng -lx++ -lewokstl -lx -lsconf -lttf -lfont $(EWOK_LIBC) -lcxx

clean:
	rm -f $(XSESSION_OBJS)

