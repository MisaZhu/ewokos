ifeq ($(ARCH),)
export ARCH=arm
endif

ifeq ($(MARIO_VM),)
MARIO_VM = .
endif

include ../../../hardware/$(ARCH)/make.rule
include $(MARIO_VM)/lang/js/lang.mk

mario_OBJS = $(MARIO_VM)/mario/mario.o
platform_OBJS = $(MARIO_VM)/platform/platform.o \
		$(MARIO_VM)/platform/mem.o
mvm_OBJS = bin/mario/main.o bin/lib/mbc.o bin/lib/js.o 
bcasm_OBJS = bin/bcasm/main.o bin/bcasm/dump.o bin/lib/mbc.o bin/lib/js.o 

MARIO_OBJS = $(mario_OBJS) $(mvm_OBJS) $(lang_OBJS) $(platform_OBJS) \
		$(NATIVE_OBJS)

BCASM_OBJS = $(mario_OBJS) $(bcasm_OBJS) $(lang_OBJS) $(platform_OBJS)

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
TARGET_DIR = $(BUILD_DIR)/extra

LDFLAGS = -L$(BUILD_DIR)/lib
HEADS = -I $(BUILD_DIR)/include \
	-I$(NATIVE_PATH_BUILTIN) \
	-I$(NATIVE_PATH_GRAPH) \
	-I$(NATIVE_PATH_X) \
	-I$(MARIO_VM)/mario \
	-I$(MARIO_VM)/bin/lib \
	-I$(MARIO_VM)/platform

CFLAGS += $(HEADS)
CXXFLAGS += $(HEADS)

MARIO = $(TARGET_DIR)/bin/mario
BCASM = $(TARGET_DIR)/bin/bcasm

all: $(MARIO) $(BCASM)
	@echo "done"


$(MARIO): $(MARIO_OBJS) \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libupng.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(EWOK_LIBC_A) 
	mkdir -p $(TARGET_DIR)/bin
	$(LD) -Ttext=100 $(MARIO_OBJS) -o $(MARIO) $(LDFLAGS) -lttf -lfont  -lgraph -lbsp -lupng -lx++ -lx -lsconf  $(EWOK_LIBC) -lcxx

$(BCASM): $(BCASM_OBJS) \
		$(EWOK_LIBC_A) 
	mkdir -p $(TARGET_DIR)/bin
	$(LD) -Ttext=100 $(BCASM_OBJS) -o $(BCASM) $(LDFLAGS) -lbsp  $(EWOK_LIBC)


clean:
	rm -f $(MARIO_OBJS) $(MARIO)
