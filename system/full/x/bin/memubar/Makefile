include ../../../../platform/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

MENU_BAR_OBJS = menubar.o

MENU_BAR = $(TARGET_DIR)/bin/x/menubar

$(MENU_BAR): $(MENU_BAR_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libupng.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(MENU_BAR_OBJS) -o $(MENU_BAR) $(LDFLAGS) -lwidget++ -lgraph -lbsp -lupng -lx++ -lx -lsconf -lttf -lfont -lewokstl $(EWOK_LIBC) -lcxx

clean:
	rm -f $(MENU_BAR_OBJS)

