include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

PNG_OBJS = png.o

PNG = $(TARGET_DIR)/bin/x/png

$(PNG): $(PNG_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libupng.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(PNG_OBJS) -o $(PNG) $(LDFLAGS)  -lgraph -lbsp -lupng -lx++ -lx  -lttf -lfont $(EWOK_LIBC) -lcxx

clean:
	rm -f $(PNG_OBJS)

