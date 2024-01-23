SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XCONSOLE_OBJS = xconsole.o

XCONSOLE_DIR = $(TARGET_DIR)/apps/xconsole
XCONSOLE = $(XCONSOLE_DIR)/xconsole

all: $(XCONSOLE_DIR) $(XCONSOLE)

$(XCONSOLE): $(XCONSOLE_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libconsole.a \
		$(BUILD_DIR)/lib/libtextview.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XCONSOLE_OBJS) -o $(XCONSOLE) $(LDFLAGS) -lsconf -lx++ -lx -lcxx -lttf  -lconsole -ltextview -lfont -lgraph -lbsp -lewoksys -lc -lgloss

$(XCONSOLE_DIR):
	mkdir -p $(XCONSOLE_DIR)
	cp -r res $(XCONSOLE_DIR)

clean:
	rm -f $(XCONSOLE_OBJS)
	rm -fr $(XCONSOLE_DIR)

