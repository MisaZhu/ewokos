SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

TEXT_TCURSES_OBJS = test_tcurses.o

TEXT_TCURSES_DIR = $(TARGET_DIR)/apps/test_tcurses
TEXT_TCURSES = $(TEXT_TCURSES_DIR)/test_tcurses

all: $(TEXT_TCURSES_DIR) $(TEXT_TCURSES)

$(TEXT_TCURSES): $(TEXT_TCURSES_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libtcurses.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(TEXT_TCURSES_OBJS) -o $(TEXT_TCURSES) $(LDFLAGS) -ltcurses -lttf -lfont -lx++ -lx -lupng -lgraph -lbsp -lewoksys -lc -lgloss -lcxx

$(TEXT_TCURSES_DIR):
	mkdir -p $(TEXT_TCURSES_DIR)
	cp -r res $(TEXT_TCURSES_DIR)

clean:
	rm -f $(TEXT_TCURSES_OBJS)
	rm -fr $(TEXT_TCURSES_DIR)

