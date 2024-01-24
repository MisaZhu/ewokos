include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

RM_OBJS = rm.o

RM = $(TARGET_DIR)/bin/rm

$(RM): $(RM_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a 
	$(LD) -Ttext=100 $(RM_OBJS) -o $(RM) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(RM_OBJS)

