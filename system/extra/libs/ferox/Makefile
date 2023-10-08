include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -Iinclude -Isrc -Isrc/external -I $(TARGET_DIR)/include

LIB_FEROX_OBJS = src/broadphase.o \
	src/collision.o \
	src/dynamics.o \
	src/geometry.o \
	src/timer.o \
	src/utils.o \
	src/world.o

FEROX = $(TARGET_DIR)/lib/libferox.a

$(FEROX): $(LIB_FEROX_OBJS)
	$(AR) rT $(FEROX) $(LIB_FEROX_OBJS)
	cp -r include/* src/external/*.h $(TARGET_DIR)/include


clean:
	rm -f $(LIB_FEROX_OBJS)
	rm -f $(FEROX)

