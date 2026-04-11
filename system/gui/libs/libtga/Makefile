SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I $(TARGET_DIR)/include

LIBTGA = $(TARGET_DIR)/lib/libtga.a

all: $(LIBTGA)

OBJS = tga.o

$(LIBTGA): $(OBJS) Makefile
	$(AR) rcs $@ $(OBJS)
	cp tga.h $(TARGET_DIR)/include

%.o: %.c
	$(CC) $(CFLAGS) -std=c99 -c -o $@ $<

clean:
	rm -f *.o
	rm -f $(LIBTGA)
