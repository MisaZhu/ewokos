ifeq ($(ARCH),)
export ARCH=arm
endif

SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/extra
LDFLAGS = -L$(BUILD_DIR)/lib
CFLAGS += -I include -I $(BUILD_DIR)/include
CXXFLAGS += -I include -I $(BUILD_DIR)/include
SAVER_OBJS = main.o

SAVER_DIR = $(TARGET_DIR)/apps/saver
SAVER = $(SAVER_DIR)/saver

all: $(SAVER_DIR) $(SAVER)

$(SAVER): $(SAVER_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libopenlibm.a \
		$(BUILD_DIR)/lib/libferox.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libcxx.a  
	$(LD) -Ttext=100 $(SAVER_OBJS) -o $(SAVER) $(LDFLAGS) -lx++ -lttf -lfont -lgraph -lbsp -lx -lsconf -lferox -lopenlibm $(EWOK_LIBC)  -lcxx

$(SAVER_DIR):
	mkdir -p $(SAVER_DIR)
	cp -r res $(SAVER_DIR)

clean:
	rm -f $(SAVER_OBJS)
	rm -fr $(SAVER_DIR)
