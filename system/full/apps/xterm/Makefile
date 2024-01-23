SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XTERM_OBJS = xterm.o

XTERM_DIR = $(TARGET_DIR)/apps/xterm
XTERM = $(XTERM_DIR)/xterm

all: $(XTERM_DIR) $(XTERM)

$(XTERM): $(XTERM_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libterminal.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XTERM_OBJS) -o $(XTERM) $(LDFLAGS) -lsconf -lx++ -lx -lcxx -lttf  -lterminal -lfont -lgraph -lbsp -lewoksys -lc -lgloss -lsoftfloat

$(XTERM_DIR):
	mkdir -p $(XTERM_DIR)
	cp -r res $(XTERM_DIR)

clean:
	rm -f $(XTERM_OBJS)
	rm -fr $(XTERM_DIR)

