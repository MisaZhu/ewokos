ifeq ($(ARCH),)
export ARCH=arm
endif

ifeq ($(MARIO_VM),)
MARIO_VM = .
endif

include ../../../hardware/$(ARCH)/make.rule
include $(MARIO_VM)/lang/js/lang.mk

mario_OBJS= $(MARIO_VM)/mario/mario.o

MARIO_OBJS = shell/mariovm.o $(mario_OBJS) $(lang_OBJS) \
		$(NATIVE_OBJS)

ifneq ($(MARIO_DEBUG), no)
CFLAGS += -g -DMARIO_DEBUG
else
CFLAGS += -O2
endif

ifneq ($(MARIO_CACHE), no)
CFLAGS += -DMARIO_CACHE
endif

ifneq ($(MARIO_THREAD), no)
CFLAGS += -DMARIO_THREAD
endif

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include
CFLAGS += -I$(NATIVE_PATH) -I$(MARIO_VM)/mario

MARIO = $(TARGET_DIR)/bin/mario

$(MARIO): $(MARIO_OBJS) \
		$(EWOK_LIBC_A) 
	mkdir -p $(TARGET_DIR)/bin
	$(LD) -Ttext=100 $(MARIO_OBJS) -o $(MARIO) $(LDFLAGS) $(EWOK_LIBC)
	cp -r $(MARIO_VM)/mario/demos/demo_js/js $(TARGET_DIR)/data/js

clean:
	rm -f $(MARIO_OBJS) $(MARIO)
