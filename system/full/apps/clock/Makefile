SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

CLOCK_OBJS = clock.o

CLOCK_DIR = $(TARGET_DIR)/apps/clock
CLOCK = $(CLOCK_DIR)/clock

all: $(CLOCK_DIR) $(CLOCK)

$(CLOCK): $(CLOCK_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libtextview.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(CLOCK_OBJS) -o $(CLOCK) $(LDFLAGS) -lwidget++ -ltextview -lgraph -lbsp -lttf -lfont -lx++ -lx -lupng -lewokstl -lewoksys -lc -lgloss -lcxx

$(CLOCK_DIR):
	mkdir -p $(CLOCK_DIR)
	cp -r res $(CLOCK_DIR)

clean:
	rm -f $(CLOCK_OBJS)
	rm -fr $(CLOCK_DIR)

