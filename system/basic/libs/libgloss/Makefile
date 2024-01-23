include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include
 
LIBGLOSS_OBJS = syscalls.o

LIBGLOSS = $(TARGET_DIR)/lib/libgloss.a

$(LIBGLOSS): $(LIBGLOSS_OBJS)  \
		$(TARGET_DIR)/lib/libsoftfloat.a
	$(AR) rT $(LIBGLOSS) $(LIBGLOSS_OBJS) $(TARGET_DIR)/lib/libsoftfloat.a

clean:
	rm -f $(LIBGLOSS_OBJS)
	rm -f $(LIBGLOSS)

