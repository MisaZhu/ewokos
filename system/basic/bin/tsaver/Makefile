include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

TSAVER_OBJS = tsaver.o

TSAVER = $(TARGET_DIR)/bin/tsaver

$(TSAVER): $(TSAVER_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a  \
		$(BUILD_DIR)/lib/libgloss.a
		$(LD) -Ttext=100 $(TSAVER_OBJS) -o $(TSAVER) $(LDFLAGS) -lewoksys -lc -lgloss 

clean:
	rm -f $(TSAVER_OBJS)

