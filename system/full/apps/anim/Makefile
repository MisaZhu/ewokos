SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

ANIM_OBJS = anim.o

ANIM_DIR = $(TARGET_DIR)/apps/anim
ANIM = $(ANIM_DIR)/anim

all: $(ANIM_DIR) $(ANIM)

$(ANIM): $(ANIM_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(ANIM_OBJS) -o $(ANIM) $(LDFLAGS) -lgraph -lbsp -lx++ -lx  -lttf -lfont -lupng $(EWOK_LIBC) -lcxx

$(ANIM_DIR):
	mkdir -p $(ANIM_DIR)
	cp -r res $(ANIM_DIR)

clean:
	rm -f $(ANIM_OBJS)
	rm -fr $(ANIM_DIR)

