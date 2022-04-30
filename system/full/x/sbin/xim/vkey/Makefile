include ../../../../../make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CXXFLAGS += -I $(BUILD_DIR)/include

XIM_OBJS = xim.o

XIM = $(TARGET_DIR)/sbin/x/xim_vkey

$(XIM): $(XIM_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XIM_OBJS) -o $(XIM) $(LDFLAGS) -lgraph -lx++  -lcxx -lewokc -lc -lfonts

clean:
	rm -f $(XIM_OBJS)

