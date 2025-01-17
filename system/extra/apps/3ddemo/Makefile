ifeq ($(ARCH),)
export ARCH=arm
endif

SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/extra
LDFLAGS = -L$(BUILD_DIR)/lib
CFLAGS += -I include -I $(BUILD_DIR)/include
CXXFLAGS += -I include -I $(BUILD_DIR)/include
OBJS = main.o src/engine.o src/camera.o src/clipping.o src/light.o src/matrix.o src/obj.o src/swap.o \
	   src/texture.o src/triangle.o src/vector.o src/display.o

INSTALL_DIR = $(TARGET_DIR)/apps/3ddemo
TARGET = $(INSTALL_DIR)/3ddemo

all: $(INSTALL_DIR) $(TARGET)

$(TARGET): $(OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libopenlibm.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libcxx.a  
	$(LD) -Ttext=100 $(OBJS) -o $(TARGET) $(LDFLAGS) -lx++ -lttf -lfont -lgraph -lbsp -lx -lsconf  -lopenlibm $(EWOK_LIBC)  -lcxx

$(INSTALL_DIR):
	mkdir -p $(INSTALL_DIR)
	cp -r res $(INSTALL_DIR)

clean:
	rm -f $(OBJS)
	rm -fr $(INSTALL_DIR)
