include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SESSION_OBJS = session.o

SESSION = $(TARGET_DIR)/bin/session

$(SESSION): $(SESSION_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a 
	$(LD) -Ttext=100 $(SESSION_OBJS) -o $(SESSION) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(SESSION_OBJS)

