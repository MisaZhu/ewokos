include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_TCURSES_OBJS = src/tcurses.o

TCURSES = $(TARGET_DIR)/lib/libtcurses.a

$(TCURSES): $(LIB_TCURSES_OBJS)
	$(AR) rT $(TCURSES) $(LIB_TCURSES_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_TCURSES_OBJS)
	rm -f $(TCURSES)

