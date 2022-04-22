include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CXXFLAGS += -I $(BUILD_DIR)/include

XCONSOLE_OBJS = xconsole.o

XCONSOLE = $(TARGET_DIR)/bin/x/xconsole

$(XCONSOLE): $(XCONSOLE_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(XCONSOLE_OBJS) -o $(XCONSOLE) $(LDFLAGS) -lsconf -lx++ -lgraphxx -lewokc -lc -lcxx -lconsole

clean:
	rm -f $(XCONSOLE_OBJS)

