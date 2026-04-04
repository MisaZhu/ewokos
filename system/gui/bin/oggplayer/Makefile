SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)/rootfs/bin

TASK = oggplayer
TASK_OBJS = oggplayer.o

CFLAGS += -I $(SYS_DIR)/gui/libs/libogg/include
CFLAGS += -I $(SYS_DIR)/gui/libs/libvorbis/include

LDFLAGS += -L$(SYS_DIR)/build/$(HW)/lib

all: $(TASK)

$(TASK): $(TASK_OBJS)
	mkdir -p $(BUILD_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(BUILD_DIR)/$(TASK) $(LDFLAGS) -lvorbis -logg -lm $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)