SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

pwrinfo_OBJS = pwrinfo.o

pwrinfo_DIR = $(TARGET_DIR)/apps/pwrinfo
pwrinfo = $(pwrinfo_DIR)/pwrinfo

all: $(pwrinfo_DIR) $(pwrinfo)

$(pwrinfo): $(pwrinfo_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(pwrinfo_OBJS) -o $(pwrinfo) $(LDFLAGS) -lfont -lttf -lgraph -lbsp -lx++ -lx -lupng -lewoksys -lc -lgloss -lcxx

$(pwrinfo_DIR):
	mkdir -p $(pwrinfo_DIR)
	cp -r res $(pwrinfo_DIR)

clean:
	rm -f $(pwrinfo_OBJS)
	rm -fr $(pwrinfo_DIR)

