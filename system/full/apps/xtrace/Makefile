SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

XTRACE_OBJS = xtrace.o

XTRACE_DIR = $(TARGET_DIR)/apps/xtrace
XTRACE = $(XTRACE_DIR)/xtrace

all: $(XTRACE_DIR) $(XTRACE)

$(XTRACE): $(XTRACE_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XTRACE_OBJS) -o $(XTRACE) $(LDFLAGS) -lttf -lfont -lwidget++ -lewokstl -lx++ -lx -lgraph -lbsp -lsconf -lupng $(EWOK_LIBC) -lcxx

$(XTRACE_DIR):
	mkdir -p $(XTRACE_DIR)
	cp -r res $(XTRACE_DIR)

clean:
	rm -f $(XTRACE_OBJS)
	rm -fr $(XTRACE_DIR)

