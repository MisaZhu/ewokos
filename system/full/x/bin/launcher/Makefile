include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CXXFLAGS += -I $(BUILD_DIR)/include

LAUNCHER_OBJS = launcher.o

LAUNCHER = $(TARGET_DIR)/bin/x/launcher

$(LAUNCHER): $(LAUNCHER_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(LAUNCHER_OBJS) -o $(LAUNCHER) $(LDFLAGS)  -lgraph -lttf -lfont -lupng -lx++ -lx -lsconf -lewokc -lc -lcxx

clean:
	rm -f $(LAUNCHER_OBJS)

