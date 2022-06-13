include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CXXFLAGS += -I $(BUILD_DIR)/include

FINDER_OBJS = finder.o

FINDER = $(TARGET_DIR)/bin/x/finder

$(FINDER): $(FINDER_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(FINDER_OBJS) -o $(FINDER) $(LDFLAGS)  -lgraph -lfonts -lupng -lx++ -lx -lsconf -lewokc -lc -lcxx

clean:
	rm -f $(FINDER_OBJS)

